#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

//#include "uds.h"
//#include "truds.h"
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
    uint64_t start1_ms, start2_ms;
    uint64_t now_ms;
    bool high_idle = false;
    Transit tr;

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGHUP, sig_handler);

    // NEW CODE

    if ( (argc < 2) || (argc > 3) ) {
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

    if(!tr.initialize(hs_can_interface, ms_can_interface)) {
        printf("ERROR: Initialization failed\n");
        exit(1);
    }
    //printf("tr.initialize completed without error\n");
    /*

    start1_ms = timestamp();

    while(running) {


        // This block is executed every 1000ms
        if( ( (now_ms = timestamp()) - start1_ms ) >= 1000) {
            //printf("[%lld] test.c main loop running\n", now_ms);

            uint32_t rpm;
            if (tr.get_rpm(rpm)) {
                printf("RPM: %d\n", rpm);
            }
            else {
                printf("ERROR: RPM not available\n");
            }

            start1_ms = now_ms;
        }

        // This block is executed every 10000ms
        if( ( (now_ms = timestamp()) - start2_ms ) >= 10000) {
            printf("[%lld] %s high idle\n", now_ms, high_idle ? "clearing" : "setting");

            uint16_t rpm_desired = 1250;

            if(!high_idle) {
                if (!tr.control_rpm(true, rpm_desired)) {
                    printf("ERROR: Could not control RPM to %d\n", rpm_desired);
                }
            }
            else {
                if (!tr.control_rpm(false, 0)) {
                    printf("ERROR: Could not stop RPM control\n");
                }
            }

            high_idle = !high_idle;
            start2_ms = now_ms;
        }
    }

    tr.finalize();
*/



// OLD CODE - KEEP UNTIL OTHER IS WORKING
    //char can_interface[33];
    //strcpy(can_interface,  "can0");
    //init_can(can_interface); // Exits program on error


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

    // UNTESTED (alternative OBD2 0x0C)
    response_size = request_uds((uint8_t *)&response, sizeof(response.val), 0x7E0, SID_RD_DATA_ID, 1, 0x1505);
    printf("VSS: %d\n", response.val);

    response_size = request_uds((uint8_t *)&response, sizeof(response.val), 0x7E0, SID_RD_DATA_ID, 1, 0xF40C);
    printf("RPM: %0.1f\n", response.val / 4.0);

    response_size = request_uds((uint8_t *)&response, sizeof(response), 0x7E0, SID_RD_DATA_ID, 1, 0xDD01);
    printf("Odometer: %d\n", response.val);


    // Raise RPM for 10 seconds
    if(begin_session_uds(0x7E0, UDS_DIAG_EXTENDED)) {
        if(request_security_uds(0x7E0)) {

            uint64_t start_ms;
            const uint32_t runtime_ms = 10000;

            // NOTE: Timing and presence of tester_present seems to be picky. Haven't found a standard yet.
            // May also work if reading a pid frequently, but also sending less frequent tester_present (4 second interval??)
            send_tester_present_uds(0x7E0);
            usleep(10000); // This might be needed to prevent requests too quickly
            request_uds(NULL, 0, 0x7E0, SID_IO_CTRL_ID, 4,
                        0x0308, 0x03, 0x04, 0xD2);

            while ((timestamp() - start_ms) < 1) {};
            send_tester_present_uds(0x7E0);

            start_ms = timestamp();
            start1_ms = timestamp();

            while ((timestamp() - start_ms) < runtime_ms) {
                send_tester_present_uds(0x7E0);
                usleep(100);

                // Fetch and display actual RPM
                if( ( (now_ms = timestamp()) - start1_ms ) >= 1000) {
                    response_size = request_uds((uint8_t *)&response, sizeof(response.val), 0x7E0, SID_RD_DATA_ID, 1, 0xF40C);
                    printf("RPM: %0.1f\n", response.val / 4.0);

                    start1_ms = now_ms;
                }
            }
        }
        else {
            printf("ERROR: request_security_uds() failed\n");
        }

        end_session_uds(0x7E0);
    }
    else {
        printf("ERROR: begin_session_uds() failed\n");
    }

    end_can();
}
