#pragma once

#include <stdint.h>

#define KB_1 (1024)
#define MB_1 (1024*KB_1)
#define DRCT_CNT (8)
#define IND1_CNT (1)
#define IND2_CNT (1)
#define IND3_CNT (1)
#define ADDR_SIZE (sizeof(uint16_t))
#define KEY_LEN (30)
#define T_FIL (0)
#define T_DIR (1)
#define T_SYM (2)
#define T2S(t) (T_FIL==(t) ? "FIL" : (T_DIR==(t) ? "DIR" : (T_SYM==(t) ? "SYM" : "NIL")))

#define GET_BIT(a,n) (((a)[(n)/8] >> (7 - ((n)%8))) & 1)
#define SET_BIT(a,n) ((a)[(n)/8] |= 1 << (7 - ((n)%8)))
#define CLR_BIT(a,n) ((a)[(n)/8] &= ~(1 << (7 - ((n)%8))))

struct File {
    uint16_t inode;
    char name[KEY_LEN];
} __attribute__ ((__packed__));

struct Inode {
    uint8_t linkCount;
    uint8_t type;
    uint32_t size;
    int32_t lastTime;
    uint16_t direct[DRCT_CNT];
    uint16_t singleI[IND1_CNT];
    uint16_t doubleI[IND2_CNT];
    uint16_t tripleI[IND3_CNT];
} __attribute__ ((__packed__));

struct SuperBlock {
    uint16_t blockSize;
    uint16_t blocksCount;
    uint16_t inodesCount;
    uint16_t filesCount;
    uint32_t firstBlockAddr;
    uint32_t firstInodeAddr;
    uint8_t blocksBitmap[128];
} __attribute__ ((__packed__));