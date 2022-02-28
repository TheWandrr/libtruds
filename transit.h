#ifndef TRANSIT_H
#define TRANSIT_H

#include <stdint.h>

#include "uds.h"

using namespace std;

#define PCM_CAN_ID 0x7E0
#define BROADCAST_CAN_ID 0x7DF

enum TransitBus {
    HS_CAN,
    MS_CAN,
};

/*
enum TransitPIDs {
    PID_PCM_ODOM,
};

struct PIDs {
    TransitPIDs tp_enum;
    TransitBus bus;
    uint8_t service_mode; // OBD_SID or UDS_SID
    uint16_t pid;
    int refresh_ms;
    const char *short_name;
    const char *long_name;
    const char *unit;
};

static const PIDs pids[] = {
    { PID_PCM_ODOM,         HS_CAN, 0x22, 0xDD01, 1000,     "PCM.ODO",        "Odometer",                         "km" },
};
*/

class Transit
{

private:
    bool initialized;
    char ms_can[32];
    char hs_can[32];

public:
    Transit();
    ~Transit();

    bool initialize(const char *hs_can_interface, const char *ms_can_interface);
    void finalize();

    bool get_odometer(uint32_t &result);
};

#endif
