#ifndef TRANSIT_H
#define TRANSIT_H

#include <stdint.h>

#include "uds.h"
#include "truds.h"

using namespace std;

enum TransitBus {
    HS_CAN,
    MS_CAN,
};

enum TransitModuleAddr {
    TM_APIM_ID = 0x7D0,
    TM_ABS_ID = 0x760,
    TM_ACM_ID = 0x727,
    TM_BCM_ID = 0x726,
    TM_FCIM_ID = 0x7A7,
    TM_FCDIM_ID = 0x7A5,
    TM_GPSM_ID = 0x701,
    TM_IPC_ID = 0x720,
    TM_IPMA_ID = 0x706,
    TM_PAM_ID = 0x736,
    TM_PCM_ID = 0x7E0,
    TM_RCM_ID = 0x737,
    TM_RBM_ID = 0x766,
    TM_SASM_ID = 0x797,
    TM_TBM_ID = 0x757,
    TM_TRM_ID = 0x791,
    TM_TCM_ID = 0x7E1,
};

struct TransitModule {
    TransitModuleAddr address;
    const char *short_name[10];
    const char *long_name[256];
};

const TransitModule modules[] = {
    {TM_APIM_ID,   "APIM",     "Accessory Protocol Interface Module (SYNC)"},
    {TM_ABS_ID,    "ABS",      "Anti-lock Braking System Module"},
    {TM_BCM_ID,    "BCM",      "Body Control Module"},
    {TM_FCIM_ID,   "FCIM",     "Front Controls Interface Module"},
    {TM_FCDIM_ID,  "FCDIM",    "Front Control Display Interface Module"},
    {TM_GPSM_ID,   "GPSM",     "Global Positioning System Module"},
    {TM_IPC_ID,    "IPC",      "Instrument Panel Cluster Module"},
    {TM_IPMA_ID,   "IPMA",     "Image Processing Module A"},
    {TM_PAM_ID,    "PAM",      "Parking Aid Module"},
    {TM_PCM_ID,    "PCM",      "Powertrain Control Module"},
    {TM_RCM_ID,    "RCM",      "Restraint Control Module"},
    {TM_RBM_ID,    "RBM",      "Running Board Control Module"},
    {TM_SASM_ID,   "SASM",     "Steering Angle Sensor Module"},
    {TM_TBM_ID,    "TBM",      "Trailer Brake Control Module"},
    {TM_TRM_ID,    "TRM",      "Trailer Module"},
    {TM_TCM_ID,    "TCM",      "Transmission Control Module"},
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

    // TODO: Getters/setters should return more detailed status codes
    bool get_vin(char *result);
    bool get_odometer(uint32_t &result);
    bool get_rpm(uint32_t &result);

    bool control_rpm(bool enabled, uint16_t desired_rpm);
    bool control_rpm(bool enabled);
};

#endif
