#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include <cstdint>

struct __attribute__((packed)) DataToTransfer {
    uint8_t drone_id;
    float pos_x;
    float pos_y;
    float pos_z;
    float vel_x;
    float vel_y;
    float vel_z;
    uint32_t timestamp_ms;
};

#endif
