#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include "uds.h"
#include "truds.h"

bool running = true;

void sig_handler(int sig) {
    running = false;
}

int main(int argc, char **argv) {
    char can_interface[65];

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGHUP, sig_handler);

    if (argc != 2) {
        printf("USAGE: %s <CAN-INTERFACE>\n", argv[0]);
        printf("Using default 'can0'.  Specify a different interface as an argument if desired.\n");
        strcpy(can_interface, "can0");
    }
    else {
        strncpy(can_interface, argv[1], sizeof(can_interface)-1);
    }

    init_can(can_interface); // Exits program on error

    if(begin_session_uds(0x7E0, UDS_DIAG_EXTENDED)) {
        request_security(0x7E0);

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

        end_session_uds(0x7E0);
    }
}
