#ifndef __UDS_H
#define __UDS_H

#define PCM_CAN_ID 0x7E0
#define BROADCAST_CAN_ID 0x7DF

enum OBD_SID {
    SID_SHOW_CURR_DATA =        0x01,
    SID_SHOW_FF_DATA =          0x02,
    SID_SHOW_DTCS =             0x03,
    SID_CLEAR_DTCS =            0x04,
    SID_TEST_RES_NOTCAN =       0x05,
    SID_TEST_RES_CAN =          0x06,
    SID_SHOW_PENDING_DTCS =     0x07,
    SID_CTRL_ONBOARD_SYS =      0x08,
    SID_RQ_VEH_INFO =           0x09,
    SID_PERM_DTCS =             0x0A,
};

// Non-OBD services begin at 0x10
enum UDS_SID {
    SID_DIAG_SESS_CTRL =        0x10,
    SID_ECU_RESET =             0x11,
    SID_SEC_ACCESS =            0x27,
    SID_COMM_CTRL =             0x28,
    SID_AUTH =                  0x29,
    SID_TESTER_PRESENT =        0x3E,
    SID_ACCESS_TIM_PARAM =      0x83,
    SID_SEC_DATA_TX =           0x84,
    SID_CTRL_DTC_SETTING =      0x85,
    SID_RESPONSE_EVENT =        0x86,
    SID_LINK_CTRL =             0x87,
    SID_RD_DATA_ID =            0x22,
    SID_RD_MEM_ADDR =           0x23,
    SID_RD_SCAL_DATA_ID =       0x24,
    SID_RD_DATA_ID_PERIOD =     0x2A,
    SID_DYN_DEF_DATA_ID =       0x2C,
    SID_WR_DATA_ID =            0x2E,
    SID_WR_MEM_ADDR =           0x3D,
    SID_CLR_DIAG_INFO =         0x14,
    SID_RD_DTC_INFO =           0x19,
    SID_IO_CTRL_ID =            0x2F,
    SID_ROUTINE_CTRL =          0x31,
    SID_RQ_DOWNLOAD =           0x34,
    SID_RQ_UPLOAD =             0x35,
    SID_TFR_DATA =              0x36,
    SID_REQ_TFR_EXIT =          0x37,
    SID_REQ_FILE_TFR =          0x38,
};

enum OBD_SID09_PID {
    SID_09_SUPP_PID =           0x00, // Request supported PIDS
    SID_09_VIN_CNT =            0x01, // VIN Message count in PID 02
    SID_09_VIN =                0x02, // Vehicle Information Number(s)
    SID_09_CID_CNT =            0x03, // Calibration ID message count in PID 04
    SID_09_CID =                0x04, // Calibration Identification Number(s)
    SID_09_CVN_CNT =            0x05, // Calibration verification number count in PID 06
    SID_09_CVN =                0x06, // Calibration Verification Number(s)
    SID_09_PERF_TRK_CNT =       0x07, // In use performance tracking message count for PID 08
    SID_09_PERF_TRK =           0x08, // In use performance tracking for spark ignition vehicles
    SID_09_ECU_NAME_CNT =       0x09, // ECU name message count for PID 0A
    SID_09_ECU_NAME =           0x0A, // ECU name(s)
    SID_09_PERF_TRK_COMP =      0x0B, // In use performance tracking for compression ignition vehicles
};

enum UDS_DIAG_SESS_TYPE {
    UDS_DIAG_DEFAULT =          0x01,
    UDS_DIAG_PROGRAM =          0x02,
    UDS_DIAG_EXTENDED =         0x03,
    UDS_DIAG_SAFETY =           0x04,
};

enum UDS_ERR {
    // 0x01..0x0F Reserved by ISO SAE
    UDS_ERR_GR =                0x10, // General reject
    UDS_ERR_SNS =               0x11, // Service not supported
    UDS_ERR_SFNS =              0x12, // Sub-function not supported
    UDS_ERR_IMLOIF =            0x13, // Incorrect message length or invalid format
    UDS_ERR_RTL =               0x14, // Response too long
    // 0x15..0x20 Reserved by ISO SAE
    UDS_ERR_BRR =               0x21, // Busy repeat request
    UDS_ERR_CNC =               0x22, // Conditions not correct
    // 0x23 Reserved by ISO SAE
    UDS_ERR_RSE =               0x24, // Reset sequence error
    UDS_ERR_NRFSC =             0x25, // No response from sub-net component
    UDS_ERR_FPEORA =            0x26, // Failure prevents execution of requested action
    // 0x27..0x30 Reserved by ISO SAE
    UDS_ERR_ROOR =              0x31, // Request out of range
    // 0x32 Reserved by ISO SAE
    UDS_ERR_SAD =               0x33, // Security access denied
    // 0x34 Reserved by ISO SAE
    UDS_ERR_IK =                0x35, // Invalid key
    UDS_ERR_ENOA =              0x36, // Exceeded number of attempts
    UDS_ERR_RTDNE =             0x37, // Required time delay not expired
    // 0x38..0x4F Reserved by extended data link security document
    UDS_ERR_UDNA =              0x70, // Upload/download not accepted
    UDS_ERR_TDS =               0x71, // Transfer data suspended
    UDS_ERR_GPF =               0x72, // General programming failure
    UDS_ERR_WBSC =              0x73, // Wrong block sequence counter
    // 0x74..0x77 Reserved by ISO SAE
    UDS_ERR_RCRRP =             0x78, // Request received correctly but response is pending
    // 0x79..0x7D Reserved by ISO SAE
    UDS_ERR_SFNSIAS =           0x7E, // Subfunction not supported in active session
    UDS_ERR_SNSIAS =            0x7F, // Service not supported in active session
    // 0x80 Reserved by ISO SAE
    UDS_ERR_RPMHIGH =           0x81, // RPM too high
    UDS_ERR_RPMLOW =            0x82, // RPM too low
    UDS_ERR_ENGRUN =            0x83, // Engine is running
    UDS_ERR_ENGSTOP =           0x84, // Engine is not running
    UDS_ERR_ENGRTLOW =          0x85, // Engine run time too low
    UDS_ERR_TEMPHIGH =          0x86, // Temperature is too high
    UDS_ERR_TEMPLOW =           0x87, // Temperature is too low
    UDS_ERR_SPDHIGH =           0x88, // Vehicle speed too high
    UDS_ERR_SPDLOW =            0x89, // Vehicle speed too low
    UDS_ERR_THRHIGH =           0x8A, // Throttle pedal is too high
    UDS_ERR_THRLOW =            0x8A, // Throttle pedal is too low
    UDS_ERR_TRNNTRL =           0x8B, // Transmission range not in neutral
    UDS_ERR_TRNGEAR =           0x8B, // Transmission range not in gear
    // 0x8E Reserved by ISO SAE
    UDS_ERR_BRKOFF =            0x8F, // Brake pedal not applied
    UDS_ERR_TRNPARK =           0x90, // Transmission range not in park
    UDS_ERR_TCLOCK =            0x91, // Torque converter is locked
    UDS_ERR_VHIGH =             0x92, // Voltage is too high
    UDS_ERR_VLOW =              0x93, // Voltage is too low
    // 0x94..0xEF Reserved for specific conditions not correct
    // 0xF0..0xFE Vehicle manufacturer specific conditions not correct
    // 0xFF Reserved by ISO SAE
};

#endif /* __UDS_H */
