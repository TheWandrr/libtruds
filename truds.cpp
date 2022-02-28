#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdarg.h>
#include <fcntl.h>
#include <time.h>


#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <linux/can.h>
#include <linux/can/raw.h>

#include "uds.h"
#include "truds.h"

// TODO: Major cleanup after restructuring, remove unused
// TODO: Refactor as object to handle connection to multiple CAN busses

//------------- OLD COMMENTS BELOW HERE -----------------------

// TODO: *** Modify add_pid_request (change name) to take different formats of requests: security, tester present, session, pid, etc. ***

// TODO: Add PIDS (0x22) and (0x09) from config file
// TODO: Add PIDS from UI with popup form
// TODO: Save PIDS to config file

// TODO: Parse received OBDII messages https://en.wikipedia.org/wiki/OBD-II_PIDs

// TODO: Try building and installing hardware filter MCP2515 driver for RPi - https://github.com/TheWandrr/mcp251x

// TODO: Figure out what you want on screen and the layout (start simple with a few live PIDs
// TODO: Config file, consider https://github.com/hyperrealm/libconfig
// TODO: Think about interface requirements for toggling on/off values and entering PID control data

//node_t *head_pid_request = NULL;
//node_t *head_pid_response = NULL;
//node_t *head_display_field = NULL;

//pthread_mutex_t lock_pid_request_list_access;

bool _running;

static int cansocket;
static bool uds_busy;
static uds_response_t uds_response;
static uint16_t request_timeout_ms;

static pthread_t rx_can_thread;

#warning Need to send periodic tester-present messages when in session?

// Returns milliseconds past the epoch
uint64_t timestamp(void) {
    struct timespec spec;

    clock_gettime(CLOCK_REALTIME, &spec);

    return ( spec.tv_sec + (spec.tv_nsec / 1.0e9) ) * 1000ull;
}

void replace(char *s, const char find, const char replace, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (s[i] == find) {
            s[i] = replace;
        }
    }
}

void print_can_frame(struct can_frame *frame) {
    printf("0x%03X [%d] ", frame->can_id, frame->can_dlc);

    for (int i = 0; i < frame->can_dlc; i++)
        printf("%02X ",frame->data[i]);

    printf("\n");
    fflush(NULL);
}

static void *rx_can(void *p) {

    struct can_frame frame;
    int nbytes;
    size_t data_bytes_left;
    size_t frame_data_offset;;
    uint8_t prev_frame_index;
    uint64_t start_ms;
    const unsigned int period_ms = 2000;

    //printf("rx_can thread started\n");

    start_ms = timestamp();

    while(_running) {
        // DEBUG - Print a status message every (period_ms) milliseconds
        if ((timestamp() - start_ms) >= period_ms) {
            //printf("[%lld] rx_can thread is active\n", timestamp());
            start_ms = timestamp();
        }
        // DEBUG

        nbytes = read(cansocket, &frame, sizeof(struct can_frame));

        if( (nbytes > 0) &&
                ( (frame.can_id & 0x700) == 0x700) ) { // Workaround for filter letting initial unmatching frame though

            // DEBUG //
            print_can_frame(&frame);
            // DEBUG //

            // If request was OBD2 broadcast address (0x7DF) then valid response is in the range 0x7E8-0x7EF
            if (uds_response.can_id == 0x7DF) {
                if ( (frame.can_id < 0x7E8) || (frame.can_id > 0x7EF) ) {
                    printf("ERROR: Response to broadcast request out of range\n");
                    return NULL;
                }
            }
            // Response address = request address + 8
            else if ((frame.can_id - 8) != uds_response.can_id) { 
                printf("ERROR: Response address did not match request address\n");
                return NULL;
            }

            switch ( HNYBBLE8(frame.data[0]) ) {
            case 0: // Single frame contains complete response
            case 1: // First frame of multi-frame response

                // Common to case 0 and 1
                uds_response.data_size = 0;

                // Single frame contains complete response
                if ( HNYBBLE8(frame.data[0]) == 0 ) {

                    // DEBUG //
                    //printf("ISO-TP-0 Single frame response\n"); fflush(NULL);
                    // DEBUG //

                    if (frame.data[1] == SID_NEG_RESP) {
#warning "Add detailed error handling"
                        // TODO: Process the rest of this code to determine more detailed information about the error.
                        // See softing poster?

                        printf("ERROR: Request could not be satisfied\n");
                        fflush(NULL);
                        return NULL;
                    }

                    // Convert response SID into original request SID
                    uds_response.sid = frame.data[1] & 0b10111111;

                    // SID dictates interpretation of response?
                    switch(uds_response.sid) {

                    case SID_DIAG_SESS_CTRL:
                    case SID_SEC_ACCESS:
                    case SID_TESTER_PRESENT:
                        frame_data_offset = 3;
                        uds_response.pid = frame.data[1];
                        break;

#warning "Different pids return different number bytes result"
                    case SID_SHOW_CURR_DATA:
                        frame_data_offset = 3;
                        uds_response.pid = frame.data[3];
                        break;

                    case SID_IO_CTRL_ID:
                    case SID_RD_DATA_ID:
                        frame_data_offset = 4;
                        uds_response.pid = UINT16(frame.data[2], frame.data[3]);
                        break;

                    default:
                        printf("[0] Unhandled SID: 0x%02X\n", uds_response.sid);
                        return NULL;
                    }

                    uds_response.data_size = LNYBBLE8(frame.data[0]) - frame_data_offset + 1;
                    memcpy(uds_response.data, frame.data + frame_data_offset, uds_response.data_size);
                    uds_response.data_received = uds_response.data_size;

                    // DEBUG
                    //printf("frame.data --> ");
                    //for (int i = 0; i < 8; i++) {
                    //    printf("%0X", frame.data[i]);
                    //}
                    //printf("\n");
                    //printf("uds_response.data --> ");
                    //for (int i = 0; i < uds_response.data_size; i++) {
                    //    printf("%0X", uds_response.data[i]);
                    //}
                    //printf("\n");
                    // DEBUG

                }

                // First frame of multi-frame response
                else {

                    // DEBUG //
                    //printf("ISO-TP-1 First frame of multi-frame response\n"); fflush(NULL);
                    // DEBUG //

                    // Convert response SID into request SID
                    uds_response.sid = frame.data[2] & 0b10111111;

                    // SID dictates interpretation of response?
                    switch(uds_response.sid) {

                    case SID_RQ_VEH_INFO:
                        frame_data_offset = 5;
                        uds_response.pid = frame.data[3];
                        uds_response.data_size = UINT16(LNYBBLE8(frame.data[0]), frame.data[1]) - frame_data_offset + 2;
                        break;

                    case SID_RD_DATA_ID:
                        frame_data_offset = 5;
                        uds_response.pid = UINT16(frame.data[3], frame.data[4]);
                        uds_response.data_size = UINT16(LNYBBLE8(frame.data[0]), frame.data[1]) - frame_data_offset + 1;
                        break;

                    default:
                        printf("[1] Unhandled SID: 0x%02X\n", uds_response.sid);
                        return NULL;
                    }

                    memcpy(uds_response.data, frame.data + frame_data_offset, 8 - frame_data_offset);
                    uds_response.data_received = 8 - frame_data_offset;
                }

                // Common to case 0 and 1
                uds_response.frame_index = 0; // May not be required if this is correctly initialized

                // First frame of multi-frame response
                if ( HNYBBLE8(frame.data[0]) == 1 ) {
                    // Send flow control response directing ECU to send all remaining frames without delay

                    // TODO: Replace with call to request_uds(), re-entrant??
                    // TODO: Either reset the timeout on receipt of each consecutive frame, or require that the timeout be set longer for multi-frame responses that take some time, if it's even a problem with the default timeout for single frame responses
                    struct can_frame frame;
                    frame.can_id = uds_response.can_id - 8;
                    frame.can_dlc = 8;
                    frame.data[0] = 0x30;
                    memset(frame.data+1, 0, 7);
                    write(cansocket, &frame, sizeof(struct can_frame));
                }

                //return NULL;

                break;

            case 2: // Consecutive frame of multi-frame response

                // DEBUG //
                //printf("ISO-TP-2 consecutive frame\n"); fflush(NULL);
                // DEBUG //

                if ( (uds_response.can_id == frame.can_id) &&
                        (uds_response.frame_index == prev_frame_index ) &&
                        (uds_response.data_received < uds_response.data_size) )
                {

                    // DEBUG //
                    //printf("ISO-TP-2: Matched consecutive frame with previous\n");
                    // DEBUG //

                    data_bytes_left = uds_response.data_size - uds_response.data_received;

                    if( data_bytes_left >= 7 ) {
                        memcpy(uds_response.data + uds_response.data_received, frame.data + 1, 7);
                        uds_response.data_received += 7;
                    }
                    else {
                        memcpy(uds_response.data + uds_response.data_received, frame.data + 1, data_bytes_left);
                        uds_response.data_received += data_bytes_left;
                    }

                    uds_response.frame_index = LNYBBLE8(frame.data[0]);

                    break;
                }

                break;

            case 3: // Flow control frame



                break;

            default:
                return NULL; // Other codes are reserved and we'll ignore them
            }


            if(uds_response.data_received == uds_response.data_size) {

                // DEBUG //
                //printf("ISO-TP: Received all expected data\n");
                // DEBUG //

                uds_busy = false;
            }
/////////////////////////////////////////////////////////////////////////////////

        }
        else {
            usleep(10000); // Yield for at least 10 ms - improves CPU usage?
        }
    }
}

int send_tester_present_uds(canid_t can_id) {
    byte32_t response;

    return request_uds((uint8_t *)&response, sizeof(response), can_id, SID_TESTER_PRESENT, 1, 0);
}

bool begin_session_uds(canid_t can_id, uint8_t type) {
    byte32_t response;
    int response_size;

    // TODO: Decode/use response bytes
    response_size = request_uds((uint8_t *)&response, sizeof(response), can_id, SID_DIAG_SESS_CTRL, 1, UDS_DIAG_EXTENDED);

    if ((response_size == 4) &&
            (response.byte[0] != SID_NEG_RESP)) {

#warning CODE STUB - create node in session tracking list

        send_tester_present_uds(can_id);
        return true;
    }
    else {
        return false;
    }
}

bool end_session_uds(canid_t can_id) {
    byte32_t response;
    int response_size;

    // TODO: Decode/use response bytes
    response_size = request_uds((uint8_t *)&response, sizeof(response), can_id, SID_DIAG_SESS_CTRL, 1, UDS_DIAG_DEFAULT);

    if ((response_size == 4) &&
            (response.byte[0] != SID_NEG_RESP)) {

#warning CODE STUB - remove node in session tracking list

        return true;
    }
    else {
        return false;
    }
}

bool is_busy_uds(void) {
    return uds_busy;
}

void set_request_timeout_uds(uint16_t new_request_timeout_ms) {
    request_timeout_ms = new_request_timeout_ms;
}

void init_response_uds(canid_t can_id, uint8_t sid, uint16_t pid) {
    uds_response.can_id = can_id;
    uds_response.sid = sid;
    uds_response.pid = pid;

    uds_response.data_size = 0;
    uds_response.data_received = 0;
    uds_response.frame_index = 0;
}

// NOTE: Only one request can be active at any time.  Requests to multiple modules are not allowed.  If this is later found to be needed then the functionality will be added.
// BLOCKS until all response frames have been received or timeout
// Data is returned in user-allocated buffer
// RETURN:
//      >= 0 = data size
//      < 0 = error code
int request_uds(uint8_t *buff, size_t buff_max, canid_t can_id, uint8_t sid, size_t n, ...) {
    va_list args;
    struct can_frame frame;
    uint16_t pid = 0;
    bool expect_response = true;
    int response_size;
    int vardata[8] = {0,0,0,0,0,0,0,0};
    uint64_t request_start_ms;
    bool timeout;

    if (!_running || uds_busy) {
        // DEBUG //
        printf("ERROR: UDS busy\n");
        // DEBUG //
        return ERR_REQ_UDS_BUSY;
    }

    // Copy up to 8 var_args into temp array
    va_start(args, n);
    for (int i = 0; i <= n; i++) {
        if (i > 7) {
            break;
        }

        vardata[i] = va_arg(args, int);
    }
    va_end(args);

    memset(frame.data, 0, sizeof(frame.data));
    frame.can_id = can_id;
    frame.can_dlc = 8;

    // Data format to send and whether a reply is expected is dependent on sid and possibly some data bytes
    switch(sid) {

    case SID_SEC_ACCESS:
        pid = vardata[0];
        frame.data[0] = 2;
        frame.data[1] = sid;
        frame.data[2] = pid;

        if(pid == 4) {
            frame.data[0] += 3;

            frame.data[3] = vardata[1];
            frame.data[4] = vardata[2];
            frame.data[5] = vardata[3];
        }

        break;

#warning "TODO Number of bytes likely varies depending on PID"
    // TODO: Maybe some range checking here too...
    case SID_IO_CTRL_ID:
        pid = vardata[0];
        frame.data[0] = 6;
        frame.data[1] = sid;
        frame.data[2] = HBYTE16(pid);
        frame.data[3] = LBYTE16(pid);
        frame.data[4] = vardata[1];
        frame.data[5] = vardata[2];
        frame.data[6] = vardata[3];
        break;

    case SID_TESTER_PRESENT:
    case SID_RQ_VEH_INFO:
    case SID_DIAG_SESS_CTRL:
    case SID_SHOW_CURR_DATA:
        pid = vardata[0];
        frame.data[0] = 2;
        frame.data[1] = sid;
        frame.data[2] = pid;
        break;

    case SID_RD_DATA_ID:
        pid = vardata[0];
        frame.data[0] = 3;
        frame.data[1] = sid;
        frame.data[2] = HBYTE16(pid);
        frame.data[3] = LBYTE16(pid);
        break;

    default:
        // DEBUG //
        printf("ERROR: [2] Unhandled SID\n");
        // DEBUG //
        return ERR_REQ_UDS_UNH_SID;
    }

    init_response_uds(can_id, sid, pid);
    uds_busy = expect_response;
    timeout = false;
    request_start_ms = timestamp();

    if ( write(cansocket, &frame, sizeof(struct can_frame)) != sizeof(struct can_frame) ) {
        uds_busy = false;
        // DEBUG //
        printf("ERROR: Unhandled CAN write failed\n");
        // DEBUG //
        return ERR_REQ_UDS_CAN_WR;
    }

    // Non-blocking wait for rx_can thread to receive response, or timeout
    while( (_running) &&
            (uds_busy) &&
            (!timeout) ) {
        usleep(1000);
        timeout = (timestamp() - request_start_ms) > request_timeout_ms;
    }

    if (timeout) {
        // DEBUG //
        printf("ERROR: UDS request timeout\n");
        // DEBUG //
        return ERR_REQ_UDS_TIMEOUT;
    }

    if ( (_running) &&
            (buff != NULL) ) {
        response_size = min(buff_max, uds_response.data_size);
        memset(buff, 0, buff_max);
        // TODO: Investigate whether there's a better way to swap the byte order here
        //memcpy(buff, uds_response.data, response_size);
        for (int i = 0; i < response_size; i++) {
            buff[i] = uds_response.data[response_size - i - 1];
        }

        // DEBUG
        //printf("buff: ");
        //for (int i = 0; i < response_size; i++) {
        //    printf("%0X ", buff[i]);
        //}
        //printf("\n");
        //printf("uds_response.data: ");
        //for (int i = 0; i < response_size; i++) {
        //    printf("%0X ", uds_response.data[i]);
        //}
        //printf("\n");
        // DEBUG

        return response_size;
    }
    else {
        return 0;
    }
}

int begin_can(char *can_iface_name) {
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_filter filter[1];

    _running = false;
    uds_busy = false;
    cansocket = -1;

    set_request_timeout_uds(250);

    if ((cansocket = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
        cansocket = -1;
    }
    else {
        strcpy(ifr.ifr_name, can_iface_name);
        ioctl(cansocket, SIOCGIFINDEX, &ifr);
        fcntl(cansocket, F_SETFL, O_NONBLOCK);

        memset(&addr, 0, sizeof(addr));
        addr.can_family = AF_CAN;
        addr.can_ifindex = ifr.ifr_ifindex;

        if (bind(cansocket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            printf("Error binding socket\n");
            cansocket = -1;
            return -1;
        }

        filter[0].can_id = 0x700;
        filter[0].can_mask = 0x700;
        setsockopt(cansocket, SOL_CAN_RAW, CAN_RAW_FILTER, &filter, sizeof(filter));

        pthread_create(&rx_can_thread, NULL, rx_can, NULL);
        //printf("returned from pthread_create\n");
        _running = true;
    }

    return cansocket;
}

void end_can(void) {
    _running = false;

    pthread_join(rx_can_thread, NULL);

    if(cansocket > 0) {
        close(cansocket);
    }
}







































/*
void string_callback(char *s, size_t max, node_data_t *node_data) {
    pid_request_t *pid_request;
    pid_response_t *pid_response;
    display_field_t *display_field;
    char data_string[1025];
    int j = 0;

    switch(node_data->id) {
        case LDID_PID_REQUEST:
            pid_request = (pid_request_t *)(node_data->data);
            snprintf(s, max, "[PID Request] { can_id: %04X, sid: %02X, pid: %04X, timeout_ms: %d, repeat_ms: %d, request_sent_ms: %llu }",
                     pid_request->can_id, pid_request->sid, pid_request->pid,
                     pid_request->timeout_ms, pid_request->repeat_ms, pid_request->request_sent_ms);
        break;

        case LDID_PID_RESPONSE:
            pid_response = (pid_response_t *)(node_data->data);

            for (int i = 0; i < pid_response->data_size; i++) {
                sprintf(data_string + j, "%2X", pid_response->data[i]);
                j += 2;

                if(i < pid_response->data_size - 1) {
                    sprintf(data_string + j, ", ");
                    j += 2;
                }
            }
            data_string[j] = '\0';

            switch (pid_response->sid) {

                case SID_RQ_VEH_INFO: // OBD MODE 9

                    switch (pid_response->pid) {

                        case SID_09_VIN:
                        case SID_09_ECU_NAME:
                            memcpy(data_string, pid_response->data, pid_response->data_size);
                            replace(data_string, '\0', ',', pid_response->data_size);
                            data_string[pid_response->data_size] = '\0';
                        break;

                    }

                    snprintf(s, max, "[PID Response] { can_id: %04X, sid: %02X, pid: %02X, data_size: %d, data: [%s] }",
                             pid_response->can_id, pid_response->sid, pid_response->pid, pid_response->data_size, data_string);

                break;

                case SID_RD_DATA_ID: // 16-bit PID
                    snprintf(s, max, "[PID Response] { can_id: %04X, sid: %02X, pid: %04X, data_size: %d, data: [%s] }",
                             pid_response->can_id, pid_response->sid, pid_response->pid, pid_response->data_size, data_string);
                break;
            }
        break;

        case LDID_DISPLAY_FIELD:
            display_field = (display_field_t *)(node_data->data);
            snprintf(s, max, "[Display Field] { name: %s, pid: %04X, data: %08llX }", display_field->pid.name, display_field->pid.pid, display_field->pid.data);
        break;

        default:
            snprintf(s, max, "Undefined list data ID");
    }
}
*/

void parse_pid_data(uint16_t pid, uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7) {
    printf("parse_pid_data() ... PID %04X: %2X %2X %2X %2X\n", pid, d4, d5, d6, d7);
}

/*
bool init_can(char *can_interface) {
    struct can_filter filter[1];

    if ( (cansocket = begin_can(can_interface)) == -1 ) {
        printf("Could not start CAN interface '%s'\n", can_interface);
        return false;
    }

    filter[0].can_id = 0x700;
    filter[0].can_mask = 0x700;
    setsockopt(cansocket, SOL_CAN_RAW, CAN_RAW_FILTER, &filter, sizeof(filter));

    return true;
}
*/

// TODO: Probably only one pending request per module is supported, so this can be redesigned a little...
//          ... use reply_pending flag?  Only allow another send to a module if no replies pending from it?

// TODO
// *** Instead of keeping the requests in the list, consider it a FIFO - requests are pushed in one end and
//     when they're dealt with they're popped out the other end.  If it's a repeat, it gets re-added like it
//     was a new request to be handled.  This should also avoid the need for MUTEX locking, but double check
//     this assumption.  Multiple processes should be able to add to one end but only one should remove from
//     the other.
//
//     Consider a toggling "list_state" variable that is read at the beginning of a read operation and compared
//     throughout the operation.  List traverse must restart if it changes.  .... hmmm
//
//     OK, CHANGING, CHANGED ?
//
// TODO

/*
void *process_pid_requests(void *param) {
    node_t *node;
    pid_request_t *pid_request;
    uint64_t now;
    bool success;
    uint8_t response[1025];
    int result;

    while(running) {

        pthread_mutex_lock(&lock_pid_request_list_access);

        node = head_pid_request;

        while (node != NULL) {
            if ( ( node->node_data != NULL ) &&
                 ( node->node_data->data != NULL ) &&
                 ( node->node_data->id == LDID_PID_REQUEST ) )
            {
                pid_request = (pid_request_t *)(node->node_data->data);

                // DEBUG //
                //printf("FOUND: ");
                //list_node_print(node);
                //fflush(NULL);
                // DEBUG //

                now = timestamp();

                if( (pid_request->request_sent_ms != 0) &&
                    (
                      (pid_request->timeout) &&
                      (now - pid_request->request_sent_ms) >= pid_request->timeout_ms
                    )
                   ) { // Request timed out before response received

                    // DEBUG //
                    printf("DELETE EXPIRED (%d ms): ", (int)(timestamp() - pid_request->request_sent_ms));
                    list_node_print(node);
                    fflush(NULL);
                    // DEBUG //

                    list_delete(&head_pid_request, node);
                }
                else {
                    if( ( pid_request->request_sent_ms == 0 ) || // First request
                        (
                          (pid_request->repeat) &&
                          (now - pid_request->request_sent_ms) >= pid_request->repeat_ms
                        )
                       ) { // Or time to repeat



                        //switch(pid_request->sid) {

                        //    case SID_RD_DATA_ID: // 16-bit PID
                        //        success = tx_can(pid_request->can_id, 3, pid_request->sid,
                        //                         HBYTE16(pid_request->pid), LBYTE16(pid_request->pid));
                        //    break;

                        //    case SID_RQ_VEH_INFO: // 8-bit byte PID
                        //        success = tx_can(pid_request->can_id, 2, pid_request->sid, pid_request->pid);
                        //    break;

                        //}
                        result = request_uds(response, sizeof(response), pid_request->can_id, pid_request->sid, 1, pid_request->pid);



                        pid_request->request_sent_ms = timestamp();

                        // DEBUG //
                        printf("SEND: ");
                        list_node_print(node);
                        fflush(NULL);
                        // DEBUG //

                        if(success) {
                            // Transmission of request suceeded
                        }
                        else {
                            // Transmission of request failed
                        }

                    }
                }
            }

            node = node->next;
            usleep(50000); // Until we know otherwise, limit the time bwtween requests to 50ms
        }

        pthread_mutex_unlock(&lock_pid_request_list_access);

        usleep(10000);
    }

}
*/

// TODO: Clean this up, maybe make it more efficient
// TODO: How to store these codes in an encrypted or obfuscated way?
int32_t key_from_seed(canid_t can_id, int32_t seed) {
    typedef struct {
        canid_t can_id;
        const uint8_t s[5];
    } secret_t;

    static const secret_t secret[] = {
        { 0x726, {0x3F, 0x9E, 0x78, 0xC5, 0x96} },
        { 0x733, {0xAA, 0xBB, 0xCC, 0xDD, 0xEE} },
        { 0x736, {0x08, 0x30, 0x61, 0x55, 0xAA} },
        //{ 0x737, {0x52, 0x6F, 0x77, 0x61, 0x6E} },
        { 0x737, {0x5A, 0x89, 0xE4, 0x41, 0x72} },
        { 0x760, {0x5B, 0x41, 0x74, 0x65, 0x7D} },
        { 0x765, {0x96, 0xA2, 0x3B, 0x83, 0x9B} },
        { 0x7A6, {0x50, 0xC8, 0x6A, 0x49, 0xF1} },
        //{ 0x7E0, {0x08, 0x30, 0x61, 0xA4, 0xC5} },
        { 0x7E0, {0x44, 0x49, 0x4F, 0x44, 0x45} },
    };

    int32_t result;

    int32_t c_seed;
    int32_t a_bit;
    int32_t v8, v9, v10, v12, v13, v14;
    int32_t or_ed_seed;
    int32_t mval;
    int32_t i, j;

    uint8_t s[5] = {0,0,0,0,0};

    for (int k = 0; k < ( sizeof(secret) / sizeof(secret_t) ); k++) {
        if (secret[k].can_id == can_id) {
            memcpy(s, secret[k].s, sizeof(s));
            // DEBUG //
            //printf("Matched with secret %2X%2X%2X%2X%2X\n", s[0], s[1], s[2], s[3], s[4]);
            // DEBUG //
            break;
        }
    }

    if ( (s[0] == 0) && (s[1] == 0) && (s[2] == 0) && (s[3] == 0) && (s[4] == 0) ) {
        // DEBUG //
        //printf("ERROR: No secret for this can_id\n");
        // DEBUG //
        return 0;
    }

    c_seed = seed;
    or_ed_seed = ((c_seed & 0xFF0000) >> 16) | (uint16_t)(seed & 0xFF00) | (s[0] << 24) | ((uint8_t)seed << 16);
    mval = 0xC541A9;

    for (i = 0; i < 32; ++i) {
        a_bit = ((or_ed_seed >> i) & 1 ^ mval & 1) << 23;
        v8 = v9 = v10 = a_bit | (mval >> 1);
        mval = v10 & 0xEF6FD7 | ((((v9 & 0x100000) >> 20) ^ ((v8 & 0x800000) >> 23)) << 20) | (((((mval >> 1) & 0x8000) >> 15) ^ ((v8 & 0x800000) >> 23)) << 15) | (((((mval >> 1) & 0x1000) >> 12) ^ ((v8 & 0x800000) >> 23)) << 12) | 32 * ((((mval >> 1) & 0x20) >> 5) ^ ((v8 & 0x800000) >> 23)) | 8 * ((((mval >> 1) & 8) >> 3) ^ ((v8 & 0x800000) >> 23));
    }

    for (j = 0; j < 32; ++j) {
        a_bit = ((((s[4] << 24) | (s[3] << 16) | s[1] | (s[2] << 8)) >> j) & 1 ^ mval & 1) << 23;
        v12 = v13 = v14 = a_bit | (mval >> 1);
        mval = v14 & 0xEF6FD7 | ((((v13 & 0x100000) >> 20) ^ ((v12 & 0x800000) >> 23)) << 20) | (((((mval >> 1) & 0x8000) >> 15) ^ ((v12 & 0x800000) >> 23)) << 15) | (((((mval >> 1) & 0x1000) >> 12) ^ ((v12 & 0x800000) >> 23)) << 12) | 32 * ((((mval >> 1) & 0x20) >> 5) ^ ((v12 & 0x800000) >> 23)) | 8 * ((((mval >> 1) & 8) >> 3) ^ ((v12 & 0x800000) >> 23));
    }

    result = ((mval & 0xF0000) >> 16) | 16 * (mval & 0xF) | ((((mval & 0xF00000) >> 20) | ((mval & 0xF000) >> 8)) << 8) | ((mval & 0xFF0) >> 4 << 16);

    // DEBUG //
    //printf("key_from_seed(0x%3X, 0x%06lX) = 0x%06lX\n", can_id, seed & 0x00FFFFFF, result & 0x00FFFFFF); fflush(NULL);
    // DEBUG //

    return result;
}

bool request_security(canid_t can_id) {
    byte32_t response;
    int result;
    byte32_t key;
    byte32_t seed;

    // TODO: Check if session already started before requesting this
//    if(!begin_session_uds(can_id, UDS_DIAG_EXTENDED)) {
//        return false;
//    }

    result = request_uds((uint8_t *)&response, sizeof(response), can_id, SID_SEC_ACCESS, 1, 3);

    if ((result == 3) &&
            (response.byte[0] != SID_NEG_RESP)) {

        seed.byte[3] = 0;
        seed.byte[2] = response.byte[0];
        seed.byte[1] = response.byte[1];
        seed.byte[0] = response.byte[2];

        // DEBUG //
        //printf("SECURITY CHALLENGE: %06X\n", seed.val);
        // DEBUG //

        key.val = key_from_seed(can_id, seed.val);

        // DEBUG //
        //printf("SECURITY RESPONSE: %06X\n", key.val);
        // DEBUG //

        result = request_uds((uint8_t *)&response, sizeof(response), can_id, SID_SEC_ACCESS, 4,
                             4, key.byte[2], key.byte[1], key.byte[0]);

#warning TODO Check what negative security access response looks like
        if (result == 0) {
            return true;
        }
        else {
            return false;
        }
    }
}

/*
void add_pid_request(uint16_t can_id, uint8_t sid, uint16_t pid,
                     uint16_t timeout_ms, uint16_t repeat_ms) {

    pid_request_t *pid_request = (pid_request_t *)malloc(sizeof(pid_request_t));
    pid_request->can_id = can_id;
    pid_request->sid = sid;
    pid_request->pid = pid;
    pid_request->timeout_ms = timeout_ms;
    pid_request->repeat_ms = repeat_ms;
    pid_request->timeout = (timeout_ms > 0) ? true : false;
    pid_request->repeat = (repeat_ms > 0) ? true : false;
    pid_request->request_sent_ms = 0;

    node_data_t *node_data = (node_data_t *)malloc(sizeof(node_data_t));
    node_data->data = pid_request;
    node_data->id = LDID_PID_REQUEST;

    node_data->to_string_callback = string_callback;

    pthread_mutex_lock(&lock_pid_request_list_access);
    list_push(&head_pid_request, node_data);
    pthread_mutex_unlock(&lock_pid_request_list_access);
}

void repeat(char *s, char c, int n) {
    while (n >= 0) {
        *s = c;
        s++;
        n--;
    }
    *s = '\n';
}
*/

/*
// Accepts up to three strings and returns a string with them neatly formatted in the
// left, middle and right justified positions for the specified window width
void draw_status_line(WINDOW *w, const int y, const char *l, const char *m, const char *r, const int pad) {
	int maxx;

	maxx = getmaxx(stdscr);
	mvwprintw(w, y, pad, l);
	mvwprintw(w, y, (maxx - strlen(m)) / 2, m);
	mvwprintw(w, y, maxx - strlen(r) - pad, r);
}
*/

/*
void draw_screen(void) {
	int stdscr_maxy;
    char strL[65];
    //char strM[65];
    char strR[65];
    node_t *node;
    display_field_t *display_field;
	uint8_t row = 2;

    // Draw window borders and titles
    box(window[0], 0 , 0);
    box(window[1], 0 , 0);

	wattron(window[0], COLOR_PAIR(6));
	wattron(window[1], COLOR_PAIR(6));
	mvwprintw(window[0], 0, 4, "  Live PIDS  ");
	mvwprintw(window[1], 0, 4, "  Vehicle Info  ");
	wattroff(window[1], COLOR_PAIR(6));
	wattroff(window[0], COLOR_PAIR(6));

    wnoutrefresh(window[0]);
    wnoutrefresh(window[1]);

	// Draw contents of top window
	// TODO: Improve this to print in nice rows and columns, maybe even scrolling
	// TODO: Separate label and data drawing so that we're only updating what's changing
	wattron(window[0], COLOR_PAIR(3));

    node = head_display_field;
    while (node != NULL) {
        if ( ( node->node_data != NULL ) &&
             ( node->node_data->data != NULL ) &&
             ( node->node_data->id == LDID_DISPLAY_FIELD ) ) {
            display_field = (display_field_t *)(node->node_data->data);

		    mvwprintw(window[0], row, 2, "%s", display_field->pid.name);
    		row++;
        }
        node = node->next;
	}
	wattroff(window[0], COLOR_PAIR(3));

	stdscr_maxy = getmaxy(stdscr);

	draw_status_line(stdscr, 0, "Unified Diagnostic Services", "", can_interface, 1);

    sprintf(strL, "[SP] Monitoring %s", active_monitoring ? "ON " : "OFF");
    sprintf(strR, "%llu", timestamp());
    attron(COLOR_PAIR(2));
	//draw_status_line(stdscr, stdscr_maxy - 1, strL, "[`Q`]uit  [`A`]dd  [`L`]oad  [`S`]ave", strR, 1);
	draw_status_line(stdscr, stdscr_maxy - 1, strL, "[Q]uit  [A]dd  [L]oad  [S]ave", strR, 1);
    attroff(COLOR_PAIR(2));

	refresh();
}
*/

/*
void add_display_field(void) {
    node_data_t *node_data = (node_data_t *)malloc(sizeof(node_data_t));
    display_field_t *display_field = (display_field_t *)malloc(sizeof(display_field_t));

	// TODO: Present dialog for data entry, if not cancelled, copy data to new pid/field structs

	sprintf(display_field->pid.name, "TEST NAME");
	display_field->pid.pid = 0xBABE;
	display_field->pid.data = 0xDEADBEEF;
    //display_field->pid.update_ms = 0; // TODO: CODE STUB

    display_field->y = 0; // TODO: CODE STUB
    display_field->x = 0; // TODO: CODE STUB
    display_field->len = 0; // TODO: CODE STUB

    node_data->data = display_field;
    node_data->id = LDID_DISPLAY_FIELD;
    node_data->to_string_callback = string_callback;

    list_push(&head_display_field, node_data);

	need_redraw = true;
}
*/

/*
void toggle_monitoring(void) {
    active_monitoring = !active_monitoring;
	need_redraw = true;
}
*/

/*
void *periodic_refresh(void *param) {
    while(running) {
        usleep(250);
        need_redraw = true;
    }

    return 0;
}
*/

/*
void sig_handler(int sig) {
	running = false;
}
*/

/*
void sigwinch(int sig) {
	resized = true;
}
*/

/*
void process_input(void) {
	int c;

	switch(c = getch()) {

		case 'q':
		case 'Q': running = 0; break;

		case 'a':
		case 'A': add_display_field(); break;

        case ' ': toggle_monitoring(); break;
	}
}
*/

/*
void process_output(void) {
	if(resized) {
		end_curses();
		refresh();
		begin_curses();
		resized = false;
		need_redraw = true;
	}

	if(need_redraw) {
		draw_screen();
        need_redraw = false;
	}
}
*/

/*
int main(int argc, char **argv) {
    pthread_t periodic_refresh_thread;
    pthread_t process_pid_requests_thread;

    bool use_terminal_ui = false; // Temporary to diesable ncurses style interface while building
//    bool use_terminal_ui = true; // Temporary to diesable ncurses style interface while building

	signal(SIGWINCH, sigwinch);
	signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGHUP, sig_handler);

    if (argc != 2) {
        printf("USAGE: %s <CAN-INTERFACE>\n", argv[0]);
		printf("Using default 'can0'.  Specify a different interface as an argument if desired.\n");
		strcpy(can_interface, "can0");
		//usleep(8000000);
        }
	else {
		strcpy(can_interface, argv[1]);
	}

    init_can(can_interface);

    if (pthread_mutex_init(&lock_pid_request_list_access, NULL) != 0)
    {
        printf("Mutex lock initialization failed\n");
        return 1;
    }

    if(use_terminal_ui) {
    	begin_curses();
    	draw_screen();

        pthread_create(&periodic_refresh_thread, NULL, periodic_refresh, NULL);

    }

    pthread_create(&process_pid_requests_thread, NULL, process_pid_requests, NULL);



/////////////////////////////////////////
//

//    add_pid_request(0x7E0, SID_RD_DATA_ID, 0xF42F, 0, 5000); // Fuel level
//    add_pid_request(0x7E0, SID_RD_DATA_ID, 0xF433, 250, 10000); // Barometric pressure

//    add_pid_request(0x7E0, SID_RD_DATA_ID, 0x054F, 0, 500); // Battery voltage
//    add_pid_request(0x7E0, SID_RD_DATA_ID, 0x1505, 100, 500); // Vehicle speed

//    add_pid_request(0x7E0, SID_RQ_VEH_INFO, SID_09_VIN, 0, 0); // VIN, one-shot

//    add_pid_request(0x7E0, SID_RQ_VEH_INFO, SID_09_ECU_NAME_CNT, 0, 0); // ECU (7E0) name count, one-shot

//    add_pid_request(0x7DF, SID_RQ_VEH_INFO, SID_09_ECU_NAME, 0, 0); // ECU name, one-shot
//    add_pid_request(0x7E0, SID_RQ_VEH_INFO, SID_09_ECU_NAME, 0, 0); // ECU name, one-shot

//    add_pid_request(0x7E0, SID_RD_DATA_ID, 0x063A, 0, 0); // Throttle cleaning cycle details, one-shot


//    key_from_seed(0x7E0, 0xBC259E); // = ?
//    key_from_seed(0x7E0, 0x701A5F); // = 0x929D50
//    key_from_seed(0x7E0, 0xE2F59A); // = 0xF56272
//    key_from_seed(0x7E0, 0x35678F); // = 0xB13DC4
//    key_from_seed(0x7E0, 0x9C84A2); // = 0xF7D9F0




#warning "Need to prevent ignoring program shutdown with CTRL-C when request fails"
#warning "Need to figure out how to do a reset on the fly after error SID 0x7F 0x22"


    if(begin_session_uds(0x7E0, UDS_DIAG_EXTENDED)) {
        request_security(0x7E0);

        //////////////////////////////////////////////////////////////////////
        // Fuel pump on?  Note different response: 06 6F 03 07 03 60 00 00
        //request_uds(NULL, 0, 0x7E0, SID_IO_CTRL_ID, 4,
        //            0x0307, 0x03, 0x3D, 0x70);

        //sleep(1);

        // Fuel pump off? Note different response: 06 6F 03 07 03 3D 70 00
        //request_uds(NULL, 0, 0x7E0, SID_IO_CTRL_ID, 4,
        //            0x0307, 0x03, 0x60, 0x00);
        //////////////////////////////////////////////////////////////////////


        //////////////////////////////////////////////////////////////////////
        // Set engine RPM for x milliseconds
        uint64_t start_ms;
        const uint32_t runtime_ms = 10000;

        // NOTE: Timing and presence of tester_present seems to be picky. Haven't found a standard yet.
        send_tester_present_uds(0x7E0);
        request_uds(NULL, 0, 0x7E0, SID_IO_CTRL_ID, 4,
                    0x0308, 0x03, 0x04, 0xD2);

        while ((timestamp() - start_ms) < 1) {};
        send_tester_present_uds(0x7E0);

        start_ms = timestamp();
        while ((timestamp() - start_ms) < runtime_ms) {
            send_tester_present_uds(0x7E0);
            usleep(100);
        }
        //////////////////////////////////////////////////////////////////////

        end_session_uds(0x7E0);
    }

//
/////////////////////////////////////////

   	while(running) {

        if(use_terminal_ui) {
            process_input();
            process_output();
        }

  		usleep(50);
   	}

    printf("Waiting for threads to terminate...\n"); fflush(NULL);

    pthread_join(process_pid_requests_thread, NULL);

    if(use_terminal_ui) {
        pthread_join(periodic_refresh_thread, NULL);

    	end_curses();
    }

    pthread_mutex_destroy(&lock_pid_request_list_access);

    printf("Cleaning up memory...\n"); fflush(NULL);

    //DEBUG//
    list_print(&head_pid_request);
    list_print(&head_display_field);
    //DEBUG//

    list_free(&head_pid_request);
    list_free(&head_display_field);

    printf("Shutting down CAN library and interface...\n"); fflush(NULL);
	end_can();

    printf("Exiting...\n"); fflush(NULL);
	exit(0);
}

*/
