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
    char hs_can_interface[33] = "";
    char ms_can_interface[33] = "";
    uint32_t response;
    char vin[18] = "";
    Transit tr;

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGHUP, sig_handler);

    // NEW CODE

    if ( (argc < 2) || (argc > 3) ) {
        printf("USAGE: %s <HS-CAN-INTERFACE> <MS-CAN-INTERFACE>\n", argv[0]);
        printf("Using default 'HS-CAN <--> can0'.  Specify a different interface as an argument if desired.\n");
        strcpy(hs_can_interface, "can0");
    }
    else if (argc == 2) {
        strncpy(hs_can_interface, argv[1], sizeof(hs_can_interface)-1);
    }
    else if (argc == 3) {
        strncpy(hs_can_interface, argv[1], sizeof(hs_can_interface)-1);
        strncpy(ms_can_interface, argv[2], sizeof(ms_can_interface)-1);
    }

    if(!tr.initialize(hs_can_interface, ms_can_interface)) {
        printf("ERROR: Initialization failed\n");
        exit(1);
    }

    if(tr.get_vin(vin)) {
        printf("VIN: %s\n", vin);
    }
    else {
        printf("ERROR: Could not retrieve VIN\n");
    }
/*
    if(tr.get_odometer(response)) {
        printf("Odometer: %d\n", response);
    }
    else {
        printf("ERROR: Could not retrieve odometer\n");
    }

    if(tr.control_rpm(true, 1500)) {
        sleep(15);
        tr.control_rpm(false, 0);
    }
    else {
        printf("ERROR: Could not control RPM\n");
    }
*/
    // CLEANUP CODE AFTER THIS
    tr.finalize();

/*
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
*/

}
