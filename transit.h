#ifndef TRANSIT_H
#define TRANSIT_H

#include <stdint.h>

#include "uds.h"

using namespace std;

enum TransitBus {
    HS_CAN,
    MS_CAN,
};

struct PIDs {
    TransitBus bus;
    uint8_t service_mode; // OBD_SID or UDS_SID
    uint16_t pid;
    int refresh_ms;
    const char *short_name;
    const char *long_name;
    const char *unit;
};

static const PIDs pids[] = {
    { HS_CAN, 0x22, 0xDD01, 1000,     "PCM.ODO",        "Odometer",                         "km/h" },
};

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
};

#endif