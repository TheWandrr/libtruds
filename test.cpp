#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include "uds.h"
#include "truds.h"
#include "transit.h"

bool running = true;

void sig_handler(int sig) {
    running = false;
}

int main(int argc, char **argv) {
    char hs_can_interface[33];
    char ms_can_interface[33];
    byte32_t response;
    size_t response_size;

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGHUP, sig_handler);

    if (argc < 2) {
        printf("USAGE: %s <HS-CAN-INTERFACE> <MS-CAN-INTERFACE>\n", argv[0]);
        printf("Using default 'HS-CAN <--> can0'.  Specify a different interface as an argument if desired.\n");
        strcpy(hs_can_interface, "can0");
        strcpy(ms_can_interface, "");
    }
    else if (argc == 2) {
        strncpy(hs_can_interface, argv[1], sizeof(hs_can_interface)-1);
        strcpy(ms_can_interface, "");
    }
    else if (argc == 3) {
        strncpy(hs_can_interface, argv[1], sizeof(hs_can_interface)-1);
        strncpy(ms_can_interface, argv[2], sizeof(ms_can_interface)-1);
    }

    Transit tr(hs_can_interface, ms_can_interface);

/*
    init_can(can_interface); // Exits program on error

    // OBDII Requests
    response_size = request_uds((uint8_t *)&response, sizeof(response), 0x7DF, SID_SHOW_CURR_DATA, 1, 0x05);
    printf("OBD2 Engine Coolant Temperature: %d\n", response.val - 40);

    response_size = request_uds((uint8_t *)&response, sizeof(response), 0x7DF, SID_SHOW_CURR_DATA, 1, 0x2F);
    printf("OBD2 Fuel Level: %0.1f\n", (100.0/255.0) * (uint8_t)response.val);

    // TODO: Fix processing of the return, needs two bytes. Others will vary as well!
#warning "This one returns two bytes but they are not currently being handled properly!"
    response_size = request_uds((uint8_t *)&response, sizeof(response), 0x7DF, SID_SHOW_CURR_DATA, 1, 0x0C);
    printf("OBD2 Engine Speed: %0.1f\n", response.val / 4.0);

    response_size = request_uds((uint8_t *)&response, sizeof(response.val), 0x7DF, SID_SHOW_CURR_DATA, 1, 0x0D);
    printf("OBD2 Vehicle Speed: %d\n", response.val);

    // Ford Proprietary Requests
//    response_size = request_uds((uint8_t *)&response, sizeof(response.val), 0x7E0, SID_RD_DATA_ID, 1, 0x1E04);
//    printf("IN_GEAR: %d\n", (response.val == 0xA0) ? 0 : 1);

    response_size = request_uds((uint8_t *)&response, sizeof(response.val), 0x7E0, SID_RD_DATA_ID, 1, 0x1E23); // PCM.TR
    printf("IN_PARK: %d\n", (response.val == 0x46) ? 1 : 0);

    response_size = request_uds((uint8_t *)&response, sizeof(response.val), 0x7E0, SID_RD_DATA_ID, 1, 0x057D);
    printf("AAT_1: %d\n", response.val - 83);

    response_size = request_uds((uint8_t *)&response, sizeof(response.val), 0x7E0, SID_RD_DATA_ID, 1, 0xF446);
    printf("AAT_2: %d\n", response.val - 40);

    response_size = request_uds((uint8_t *)&response, sizeof(response.val), 0x7E0, SID_RD_DATA_ID, 1, 0xF405);
    printf("ECT: %d\n", response.val - 40);

#warning "This one returns two bytes but they are not currently being handled properly!"
    response_size = request_uds((uint8_t *)&response, sizeof(response.val), 0x7E0, SID_RD_DATA_ID, 1, 0xF40C);
    printf("RPM: %0.1f\n", response.val / 4.0);

#warning "This one returns four bytes but they are not currently being handled properly!"
    response_size = request_uds((uint8_t *)&response, sizeof(response), 0x7E0, SID_RD_DATA_ID, 1, 0xDD01);
    printf("Odometer: %d\n", response.val);
*/

    /*
    // Raise RPM for 10 seconds
    if(begin_session_uds(0x7E0, UDS_DIAG_EXTENDED)) {
        if(request_security(0x7E0)) {

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
        }

        end_session_uds(0x7E0);
    }
    */

    //end_can();
}
