#include <string.h>
#include <stdio.h>

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

    set_request_timeout_uds(750);

    initialized = true;
    return true;
}

void Transit::finalize()
{
    end_session_uds(TM_PCM_ID);
    end_can();
}

bool Transit::get_vin(char *result)
{
    #warning "CODE STUB"
}

bool Transit::get_odometer(uint32_t &result)
{
    byte32_t response;
    int response_size;

    result = 0;
    response_size = request_uds((uint8_t *)&response, sizeof(response), TM_PCM_ID, SID_RD_DATA_ID, 1, 0xDD01);
    result = response.val;

    return initialized && (response_size >= 0);
}

bool Transit::get_rpm(uint32_t &result)
{
    byte32_t response;
    int response_size;

    result = 0;
    response_size = request_uds((uint8_t *)&response, sizeof(response), BROADCAST_CAN_ID, SID_SHOW_CURR_DATA, 1, 0x0C);
    result = response.val;

    return initialized && (response_size >= 0);
}

bool Transit::control_rpm(bool enabled, uint16_t rpm_desired)
{
    byte32_t response;
    int response_size;

    if (enabled) {
        if (begin_session_uds(TM_PCM_ID, UDS_DIAG_EXTENDED)) {
            if (request_security_uds(TM_PCM_ID)) {
                set_tester_present(true, 1000); // TODO: Period?? Guess!

                response_size = request_uds(NULL, 0, TM_PCM_ID,  SID_IO_CTRL_ID, 4, 0x0308, 0x03, HBYTE16(rpm_desired), LBYTE16(rpm_desired) );

                return initialized && (response_size >= 0);
            }
            else {
                printf("ERROR: Security request failed\n");
            }
        }
        else {
            printf("ERROR: Begin session failed\n");
        }
    }
    else {
        set_tester_present(false, 0);
        end_session_uds(TM_PCM_ID);
    }

}
