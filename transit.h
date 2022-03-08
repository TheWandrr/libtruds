#ifndef TRANSIT_H
#define TRANSIT_H

#include <stdint.h>
//#include <thread>

#include "uds.h"
//#include "truds.h"

using namespace std;

class Transit
{

public:

    // Must synchronize: TransitModuleIdx, TransitModuleId, modules[]
    enum TransitModuleIdx {
        TM_APIM,
        TM_ABS,
        TM_ACM,
        TM_BCM,
        TM_FCIM,
        TM_FCDIM,
        TM_GPSM,
        TM_IPC,
        TM_IPMA,
        TM_PAM,
        TM_PCM,
        TM_RCM,
        TM_RBM,
        TM_SASM,
        TM_TBM,
        TM_TRM,
        TM_TCM,
    };

    Transit();
    ~Transit();

    bool initialize(const char *hs_can_interface, const char *ms_can_interface);
    void finalize();

    // TODO: Getters/setters should return more detailed status codes
    bool get_vin(char *result);
    bool get_odometer(uint32_t &result);
    bool get_rpm(uint32_t &result);
    bool get_coolant_temp(uint8_t &result);
    bool get_fuel_level(uint8_t &result);
    bool get_vehicle_speed(double &result);
    bool get_in_park(bool &result);

    bool control_rpm(bool enabled, uint16_t desired_rpm);
    bool control_rpm(bool enabled);

private:

    enum TransitBus {
        HS_CAN,
        MS_CAN,
    };

    struct TransitModule {
        uint16_t can_id;
        const char *short_name[10];
        const char *long_name[256];
    };

    // Must synchronize: TransitModuleIdx, TransitModuleId, modules[]
    static constexpr TransitModule modules[17] = {
        {0x7D0, "APIM",     "Accessory Protocol Interface Module (SYNC)"},
        {0x760, "ABS",      "Anti-lock Braking System Module"},
        {0x727, "ACM",      "Audio Control Module"},
        {0x726, "BCM",      "Body Control Module"},
        {0x7A7, "FCIM",     "Front Controls Interface Module"},
        {0x7A5, "FCDIM",    "Front Control Display Interface Module"},
        {0x701, "GPSM",     "Global Positioning System Module"},
        {0x720, "IPC",      "Instrument Panel Cluster Module"},
        {0x706, "IPMA",     "Image Processing Module A"},
        {0x736, "PAM",      "Parking Aid Module"},
        {0x7E0, "PCM",      "Powertrain Control Module"},
        {0x737, "RCM",      "Restraint Control Module"},
        {0x766, "RBM",      "Running Board Control Module"},
        {0x797, "SASM",     "Steering Angle Sensor Module"},
        {0x757, "TBM",      "Trailer Brake Control Module"},
        {0x791, "TRM",      "Trailer Module"},
        {0x7E1, "TCM",      "Transmission Control Module"},
    };

    bool initialized;
    char ms_can[32];
    char hs_can[32];

};

#endif
