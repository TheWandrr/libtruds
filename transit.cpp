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
    //printf("hs_can_interface: [%s], ms_can_interface: [%s]\n", hs_can_interface, ms_can_interface);

    // TODO: Check that interfaces are valid, signal failure and don't continue
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

    initialized = true;
    return true;
}

void Transit::finalize()
{
    end_can();
    //printf("returned from end_can\n");
}

bool Transit::get_odometer(uint32_t &result)
{
    byte32_t response;
    int response_size;

    result = 0;
    response_size = request_uds((uint8_t *)&response, sizeof(response), PCM_CAN_ID, SID_RD_DATA_ID, 1, 0xDD01);
    result = response.val;

    return initialized && (response_size >= 0);
}
