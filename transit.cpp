#include <string.h>
#include <stdio.h>
#include <unistd.h>
//#include <thread>

#include "uds.h"
#include "transit.h"
#include "truds.h"

Transit::Transit()
{
    initialized = false;
    strcpy(hs_can, "");
    strcpy(ms_can, "");
}

Transit::~Transit()
{
}

bool Transit::initialize(const char *hs_can_interface, const char *ms_can_interface)
{
    // TODO: Add physical hardware and code for MS-CAN (250kbps) and I-CAN(500kbps)

    //printf("hs_can_interface: [%s], ms_can_interface: [%s]\n", hs_can_interface, ms_can_interface);

    // TODO: Check that interfaces are valid, signal failure and don't continue
    // TODO: Set CAN speed to match the spec for HS/MS
    if (strcmp(hs_can_interface, "") != 0) {
        strncpy(hs_can, hs_can_interface, sizeof(hs_can) - 1);
        hs_can[sizeof(hs_can)-1] = 0;
        //printf("Initializing HS-CAN on %s\n", hs_can);

        if(begin_can(hs_can) <= 0) {
            printf("Error initializing HS-CAN\n");
            return false;
        }

        //printf("returned from begin_can\n");
    }

    // TODO: Add in the future when we connect hardware for this. Major restructuring may need to be performed.
    //if (strcmp(ms_can_interface, "") != 0) {
    //    printf("Initializing MS-CAN on %s\n", ms_can_interface);
    //    strncpy(ms_can, ms_can_interface, sizeof(ms_can) - 1);
    //    ms_can[sizeof(ms_can)-1] = 0;
    //    init_can(ms_can);
    //}

    set_request_timeout_uds(250);

    initialized = true;
    return true;
}

void Transit::finalize()
{
    // TODO: Need to track each session and only end a session when still open
    //end_session_uds(modules[TM_PCM].can_id);
    end_can();
}

//////////////////////// GET ////////////////////////

bool Transit::get_vin(char *result)
{
    const size_t VIN_LENGTH = 17;
    int response_size;

    response_size = request_uds((uint8_t *)result, VIN_LENGTH, transit_module[TM_PCM].can_id, SID_RD_DATA_ID, 1, 0xF190);

    if (initialized && (response_size = VIN_LENGTH)) {
        result[VIN_LENGTH] = '\0';
        return true;
    }
    else {
        strcpy(result, "");
        return false;
    }
}

bool Transit::get_odometer(uint32_t &result)
{
    byte32_t response;
    int response_size;

    response_size = request_uds((uint8_t *)&response, sizeof(response), transit_module[TM_PCM].can_id, SID_RD_DATA_ID, 1, 0xDD01);

    if (initialized && (response_size > 0)) {
        result = response.val;
        return true;
    }
    else {
        result = 0;
        return false;
    }
}

bool Transit::get_rpm(uint32_t &result)
{
    byte32_t response;
    int response_size;

    response_size = request_uds((uint8_t *)&response, sizeof(response), BROADCAST_CAN_ID, SID_SHOW_CURR_DATA, 1, 0x0C);

    if (initialized && (response_size > 0)) {
        result = response.val / 4;
        return true;
    }
    else {
        result = 0;
        return false;
    }
}

// Returns true if we're initialized, request succeeded, and data size returned is the same as requested
bool Transit::get_pid(uint16_t pid, uint8_t *buff)
{
    int response_size;
    TransitPid tp;

    // TODO: Replace with a lookup function?
    if (pid < (sizeof(transit_pid) / sizeof(TransitPid))) {
        tp = transit_pid[pid];
    }
    else {
        // DEBUG
        printf("Bounds check failed for transit_pid[] failed\n");
        // DEBUG

        return -1;
    }

    switch (tp.access_type) {
        case MAT_OBDII:
            response_size = -1;
            printf("CODE STUB - MAT_OBDII\n");
            break;

        case MAT_UDS_DEFAULT:
            response_size = request_uds(buff, tp.data_size, tp.server.can_id, SID_RD_DATA_ID, 1, tp.addr);
            break;

        case MAT_UDS_EXTENDED:
            response_size = -1;
            printf("CODE STUB - MAT_UDS_EXTENDED\n");
            break;

        default:
            printf("Unhandled access type (%d) in get_pid()\n", (uint16_t)(tp.access_type));
    }

    if (initialized && (response_size == tp.data_size)) {

        if(tp.data_type == TPT_STRING) {
            buff[response_size] = '\0';
        }

        return true;
    }
    else {
        // DEBUG
        printf("Call to get_pid() failed\n");
        // DEBUG

        memset(buff, 0, sizeof(buff));
        return false;
    }
}



//////////////////////// SET ////////////////////////

bool Transit::control_rpm(bool enabled, uint16_t rpm_desired)
{
    byte32_t response;
    int response_size;

    if (enabled) {
        // TODO: Need to detect or keep track of session status, automatically begin the extended session and unlock it if necessary.
        // ?? Keep a list of requests as they are made, and whether they're repeating or one-shot ??

        begin_session_uds(transit_module[TM_PCM].can_id, UDS_DIAG_EXTENDED);
        //if (begin_session_uds(transit_modules[TM_PCM].can_id, UDS_DIAG_EXTENDED)) {
            if (request_security_uds(transit_module[TM_PCM].can_id)) {
                usleep(10000); // Determined a delay is required to prevent failure on next request. Timing is guessed.

                send_tester_present_uds(transit_module[TM_PCM].can_id);

                response_size = request_uds(NULL, 0, transit_module[TM_PCM].can_id,  SID_IO_CTRL_ID, 4, 0x0308, 0x03, HBYTE16(rpm_desired), LBYTE16(rpm_desired) );

                usleep(10000); // Determined a delay is required to prevent failure on next request. Timing is guessed.

                set_tester_present(true, 500);

                return initialized && (response_size >= 0);
            }
            else {
                printf("ERROR: Security request failed\n");
                return false;
            }
        //}
        //else {
        //    printf("ERROR: Begin session failed\n");
        //    return false;
        //}
    }
    else {
        // TODO: As above, needs to automatically end the extended session and turn off tester_present when no longer needed.
        set_tester_present(false, 0);
        end_session_uds(transit_module[TM_PCM].can_id);
        return true;
    }
}
