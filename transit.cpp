#include <string.h>

#include "uds.h"
#include "transit.h"
#include "truds.h"

Transit::Transit(char *hs_can_interface, char *ms_can_interface)
{
    strcpy(ms_can, "");
    strcpy(hs_can, "");

    // TODO: Check that interfaces are valid, signal failure and don't continue
    if (!strcmp(hs_can_interface, "")) {
//        strncpy(hs_can, hs_can_interface, sizeof(hs_can) - 1);
//        hs_can[sizeof(hs_can)-1] = '\0';
        init_can(hs_can);
    }

    if (!strcmp(ms_can_interface, "")) {
//        strncpy(ms_can, ms_can_interface, sizeof(ms_can) - 1);
//        ms_can[sizeof(ms_can)-1] = '\0';
        init_can(ms_can);
    }

}

Transit::~Transit()
{
    end_can();
}
