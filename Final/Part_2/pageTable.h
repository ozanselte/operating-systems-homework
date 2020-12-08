#pragma once

#include <stdint.h>
#include <pthread.h>
#include <sys/time.h>

#define PATH_LEN (1006)
#define NAME_LEN (32)
#define CLOCK_INT (20000)

#define GET_BIT(a,n) (((a) >> (13 - (n))) & 1)
#define SET_BIT(a,n) ((a) |= 1 << (13 - (n)))
#define CLR_BIT(a,n) ((a) &= ~(1 << (13 - (n))))

enum RAlgo {
    RALGO_NONE=0, NRU, FIFO, SC, LRU, WSC
} __attribute__ ((__packed__));

enum APol {
    APOL_NONE=0, GLOBAL, LOCAL
} __attribute__ ((__packed__));

struct TStats {
    char name[NAME_LEN];
    uint32_t actives;
    size_t gt, st;
    size_t pm, pr;
    size_t rd, wr;
};

struct VConfig {
    uint32_t frameSize;
    uint32_t numPhysical;
    uint32_t numVirtual;
    enum RAlgo algo;
    enum APol pol;
    uint32_t printInt;
    char path[PATH_LEN];
} __attribute__ ((__packed__));

struct PTEntry {
    uint32_t isPresent : 1;
    uint32_t isMod : 1;
    uint32_t refs : 14;
    uint32_t phys : 16;
    uint32_t thread;
} __attribute__ ((__packed__));

struct PTable {
    uint32_t vcount;
    uint32_t pcount;
    uint32_t size;
    uint32_t tcount;
    struct PTEntry **arr;
    struct timeval lastClock;
    void *lst;
    uint64_t printCounter;
};

void setLastTime();
void checkClock();
void initTable();
void initVirtualMemory();
void initPhysicalMemory();
void waitAlgos();
void freeResources();
void fillAll();

void checkSorting();
void createThreads();
void *bubbleMain(void *data);
int quickPartition(int low, int high, char *name);
void quickSort(int low, int high, char *name);
void *quickMain(void *data);
void mergeTwo(int low, int mid, int high, char *name);
void mergeSort(int low, int high, char *name);
void *mergeMain(void *data);

uint32_t physicalFrame(uint32_t vFrame, int64_t tid);
uint32_t pageFault(uint32_t vFrame, int64_t tid);
void setPage(uint32_t vFrame, uint32_t pFrame, int64_t tid);
int64_t findEmpty();
int64_t findThread(const char *tName);
uint32_t countTid(int64_t tid);
uint32_t countNames();
uint32_t countActiveThreads();
int64_t biggestTid();
uint32_t physicalAddress(unsigned int index, const char *tName, uint8_t mod);
void printTable();
void printStats();

void set(unsigned int index, int value, const char *tName);
int get(unsigned int index, const char *tName);
uint32_t nruAlgorithm(int64_t tid);
uint32_t fifoAlgorithm(int64_t tid);
uint32_t scAlgorithm(int64_t tid);
uint32_t lruAlgorithm(int64_t tid);
uint32_t wscAlgorithm(int64_t tid);