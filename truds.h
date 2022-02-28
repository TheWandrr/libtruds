#ifndef __TRUDS_H
#define __TRUDS_H

#include <linux/can.h>
#include <stdint.h>
#include <stdbool.h>

extern "C" bool request_security(canid_t can_id);
extern "C" bool init_can(char *can_interface);
extern "C" int begin_can(char *can_iface_name);
extern "C" void end_can(void);
extern "C" int send_tester_present_uds(canid_t can_id);
extern "C" int request_uds(uint8_t *buff, size_t buff_max, canid_t can_id, uint8_t sid, size_t n, ...);
extern "C" bool begin_session_uds(canid_t can_id, uint8_t type);
extern "C" bool end_session_uds(canid_t can_id);
extern "C" bool is_busy_uds(void);
extern "C" void set_request_timeout_uds(uint16_t new_timeout_ms);
extern "C" void print_can_frame(struct can_frame *frame);
extern "C" uint64_t timestamp(void);
extern "C" void replace(char *s, const char find, const char replace, size_t n);

#define HBYTE16(val) ((uint8_t)((val) >> 8))
#define LBYTE16(val) ((uint8_t)(val))

#define UINT16(h_byte, l_byte) ((uint16_t)((h_byte << 8) | (l_byte)))

#define HNYBBLE8(val) ((uint8_t)(((val) >> 4) & 0x0F))
#define LNYBBLE8(val) ((uint8_t)((val) & 0x0F))

#define max(a,b) \
    ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
       _a > _b ? _a : _b; })

#define min(a,b) \
    ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
       _a > _b ? _b : _a; })

typedef union {
    uint32_t val;
    uint8_t byte[4];
} byte32_t;

typedef struct {
    uint16_t can_id;
    uint8_t sid;
    uint16_t pid;
    size_t data_size;
    size_t data_received;
    uint8_t frame_index;
    uint8_t data[4096];
} uds_response_t;

enum ERR_REQ_UDS {
    ERR_REQ_UDS_BUSY = -1,
    ERR_REQ_UDS_TIMEOUT = -2,
    ERR_REQ_UDS_UNH_SID = -3,
    ERR_REQ_UDS_CAN_WR = -4,
};
/*
typedef struct {
    uint16_t can_id;
    uint8_t sid;
    uint16_t pid;
    bool timeout;
    bool repeat;
    uint16_t timeout_ms;
    uint16_t repeat_ms;
    uint64_t request_sent_ms;
} pid_request_t;

#warning remove pid_response_t after restructuring
typedef struct {
    uint16_t can_id;
    uint8_t sid;
    uint16_t pid;
    size_t data_size;
    size_t data_received;
    uint8_t frame_index;
    uint8_t *data;
    bool garbage;
} pid_response_t;
*/
#endif /* __TRUDS_H */
