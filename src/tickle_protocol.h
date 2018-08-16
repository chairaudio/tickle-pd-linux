#pragma once

#include "./isoc_frame.h"

#define MINI_BUFFER_SIZE 16
typedef struct MiniBuffer_ {
    uint8_t data[MINI_BUFFER_SIZE];
} MiniBuffer;

#define SMALL_BUFFER_SIZE 32
#define SMALL_BUFFER_MAX_DATA_BYTES 30
typedef struct SmallBuffer_ {
    uint16_t size;
    uint8_t data[SMALL_BUFFER_MAX_DATA_BYTES];
} SmallBuffer;

#define BIG_BUFFER_SIZE 256
typedef struct BigBuffer_ {
    uint8_t data[BIG_BUFFER_SIZE];
} BigBuffer;

#define TICKLE_IOC_MAGIC 0xca
#define TICKLE_IOC_PING _IO(TICKLE_IOC_MAGIC, 0)
#define TICKLE_IOC_GET_KERNEL_MODULE_VERSION \
    _IOR(TICKLE_IOC_MAGIC, 1, SmallBuffer)
#define TICKLE_IOC_READ_FRAME _IOR(TICKLE_IOC_MAGIC, 2, BigBuffer)
#define TICKLE_IOC_CLAIM_DEVICE _IOR(TICKLE_IOC_MAGIC, 3, SmallBuffer)
#define TICKLE_IOC_GET_DEVICE_VERSION _IOR(TICKLE_IOC_MAGIC, 4, SmallBuffer)
#define TICKLE_IOC_GET_DEVICE_SERIAL _IOR(TICKLE_IOC_MAGIC, 5, SmallBuffer)

#define TICKLE_IOC_SET_COLOR _IOW(TICKLE_IOC_MAGIC, 6, MiniBuffer)
