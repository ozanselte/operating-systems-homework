#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <list>
#include <vector>

using std::list;
using std::vector;

#include "pageTable.h"

struct VConfig config;
struct PTable table;
struct TStats *stats;
static pthread_t thrs[4];
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static list<uint32_t> *lst;
static vector<uint32_t> *vec;
static uint32_t (*algoFunc)(int64_t);
int fd;
int *mem;

void setLastTime()
{
    if (-1 == gettimeofday(&table.lastClock, NULL)) {
        fprintf(stderr, "setLastTime, gettimeofday\n");
        exit(EXIT_FAILURE);
    }
}

void checkClock()
{
    struct timeval now;
    if (-1 == gettimeofday(&now, NULL)) {
        fprintf(stderr, "checkClock, gettimeofday\n");
        exit(EXIT_FAILURE);
    }
    time_t sDiff = now.tv_sec - table.lastClock.tv_sec;
    suseconds_t uDiff = now.tv_usec - table.lastClock.tv_usec;
    if (sDiff > 0 or CLOCK_INT > uDiff) {
        return;
    }
    for (uint32_t i = 0; i < table.vcount; ++i) {
        if (0 == table.arr[i]->isPresent) continue;
        table.arr[i]->refs >>= 1;
        if (0 == table.arr[i]->refs && 0 < stats[table.arr[i]->thread].actives) {
            stats[table.arr[i]->thread].actives--;
        }
    }
    memcpy(&table.lastClock, &now, sizeof(now));
}

void initTable()
{
    memset(&table, '\0', sizeof(table));
    table.vcount = pow(2, config.numVirtual);
    table.pcount = pow(2, config.numPhysical);
    table.tcount = (8 < table.pcount) ? table.pcount : 8;
    table.size = pow(2, config.frameSize);
    if (WSC == config.algo){
        table.lst = new vector<uint32_t>;
        vec = (vector<uint32_t> *)table.lst;
        vec->push_back(1);
    } else {
        table.lst = new list<uint32_t>;
        lst = (list<uint32_t> *)table.lst;
    }
    stats = (struct TStats *)calloc(table.tcount, sizeof(struct TStats));
    if (NULL == stats) {
        fprintf(stderr, "initTable, calloc\n");
        exit(EXIT_FAILURE);
    }
    table.arr = (struct PTEntry **)calloc(table.vcount, sizeof(struct PTEntry *));
    if (NULL == table.arr) {
        fprintf(stderr, "initTable, calloc\n");
        exit(EXIT_FAILURE);
    }
    for (uint32_t i = 0; i < table.vcount; ++i) {
        table.arr[i] = (struct PTEntry *)calloc(1, sizeof(struct PTEntry));
        if (NULL == table.arr[i]) {
            fprintf(stderr, "initTable, for, calloc\n");
            exit(EXIT_FAILURE);
        }
    }
    setLastTime();
    table.printCounter = 0;
    switch (config.algo) {
        case NRU: algoFunc = &nruAlgorithm; break;
        case FIFO: algoFunc = &fifoAlgorithm; break;
        case SC: algoFunc = &scAlgorithm; break;
        case LRU: algoFunc = &lruAlgorithm; break;
        case WSC: algoFunc = &wscAlgorithm; break;
        default:
            fprintf(stderr, "initTable, switch\n");
            exit(EXIT_FAILURE);
    }
}

void initVirtualMemory()
{
    fd = open(config.path, O_RDWR | O_CREAT | O_SYNC, 0644);
    if (-1 == fd) {
        fprintf(stderr, "initVirtualMemory, open\n");
        exit(EXIT_FAILURE);
    }
    if (-1 == ftruncate(fd, 0)) {
        fprintf(stderr, "initVirtualMemory, ftruncate\n");
        exit(EXIT_FAILURE);
    }
    if (-1 == ftruncate(fd, sizeof(int) * table.size * table.vcount)) {
        fprintf(stderr, "initVirtualMemory, ftruncate\n");
        exit(EXIT_FAILURE);
    }
    for (uint32_t i = 0; i < table.vcount; ++i) {
        table.arr[i]->isPresent = 0;
        table.arr[i]->isMod = 0;
        table.arr[i]->refs = 0;
        table.arr[i]->phys = 0;
        table.arr[i]->thread = 0;
    }
    setLastTime();
}

void initPhysicalMemory()
{
    mem = (int *)calloc(table.pcount * table.size, sizeof(int));
    if (NULL == mem) {
        fprintf(stderr, "initPhysicalMemory, calloc\n");
        exit(EXIT_FAILURE);
    }
    setLastTime();
}

void waitAlgos()
{
    void *res;
    for (int i = 0; i < 4; ++i) {
        if (0 != pthread_join(thrs[i], &res)) {
            fprintf(stderr, "freeResources, pthread_join\n");
            exit(EXIT_FAILURE);
        }
    }
}

void freeResources()
{
    for (uint32_t i = 0; i < table.vcount; ++i) {
        if (table.arr[i]->isPresent && table.arr[i]->isMod) {
            struct PTEntry *entry = table.arr[i];
            uint32_t pFrame = entry->phys;
            ssize_t size = sizeof(int) * table.size;
            off_t loc = size * i;
            int *ptr = mem;
            ptr += table.size * pFrame;
            if (0 != entry->isMod) {
                if (size != pwrite(fd, ptr, size, loc)) {
                    fprintf(stderr, "freeResources, pwrite\n");
                    exit(EXIT_FAILURE);
                }
                stats[entry->thread].wr++;
            }
        }
        free(table.arr[i]);
    }
    free(stats);
    free(table.arr);
    free(mem);
    if (WSC == config.algo) {
        delete (vector<uint32_t> *)table.lst;
    } else {
        delete (list<uint32_t> *)table.lst;
    }
    if (-1 == close(fd)) {
        fprintf(stderr, "freeResources, close\n");
        exit(EXIT_FAILURE);
    }
}

void fillAll()
{
    srand(1000);
    size_t size = table.vcount * table.size;
    for (size_t i = 0; i < size; ++i) {
        set(i, rand(), "filler");
    }
}

void checkSorting()
{
    char nm[] = "check";
    size_t size = table.size * (table.vcount / 4);
    int small, large, err;
    for (uint32_t i = 0; i < 4; ++i) {
        err = 0;
        small = get(size*i, nm);
        for (uint32_t j = 1; j < size; ++j) {
            large = get(size*i+j, nm);
            if (small >= large) {
                err = 1;
                break;
            }
            small = large;
        }
        printf("%u. part of 4 has ", i);
        if (err) {
            printf("NOT ");
        }
        printf("passed the sorting tests!\n");
    }
}

void createThreads()
{
    if (0 != pthread_create(&thrs[0], NULL, bubbleMain, (void *)(uintptr_t)0)) {
        fprintf(stderr, "createThreads, pthread_create\n");
        exit(EXIT_FAILURE);
    }
    if (0 != pthread_create(&thrs[1], NULL, quickMain, (void *)(uintptr_t)1)) {
        fprintf(stderr, "createThreads, pthread_create\n");
        exit(EXIT_FAILURE);
    }
    if (0 != pthread_create(&thrs[2], NULL, mergeMain, (void *)(uintptr_t)2)) {
        fprintf(stderr, "createThreads, pthread_create\n");
        exit(EXIT_FAILURE);
    }
    if (0 != pthread_create(&thrs[3], NULL, mergeMain, (void *)(uintptr_t)3)) {
        fprintf(stderr, "createThreads, pthread_create\n");
        exit(EXIT_FAILURE);
    }
}

void *bubbleMain(void *data)
{
    uint32_t idx = (uintptr_t)data;
    char nm[NAME_LEN];
    sprintf(nm, "bubble-%u", idx);
    size_t size = table.size * (table.vcount / 4);
    idx *= size;
    int a, b;
    for (uint32_t i = 0; i < size-1; ++i) {
        for (uint32_t j = 0; j < size-i-1; ++j) {
            a = get(idx+j, nm);
            b = get(idx+j+1, nm);
            if (a > b) {
                set(idx+j, b, nm);
                set(idx+j+1, a, nm);
            }
        }
    }
    return NULL;
}

int quickPartition(int low, int high, char *name)
{
    int pivot = get(high, name);
    int i = low - 1;
    int a, b;
    for (int j = low; j <= high-1; ++j) {
        a = get(j, name);
        if (a < pivot) {
            i++;
            b = get(i, name);
            set(i, a, name);
            set(j, b, name);
        }
    }
    a = get(high, name);
    b = get(i+1, name);
    set(i+1, a, name);
    set(high, b, name);
    return i + 1;
}

void quickSort(int low, int high, char *name)
{
    if (low >= high) {
        return;
    }
    int par = quickPartition(low, high, name);
    quickSort(low, par-1, name);
    quickSort(par+1, high, name);
}

void *quickMain(void *data)
{
    uint32_t idx = (uintptr_t)data;
    char nm[NAME_LEN];
    sprintf(nm, "quick-%u", idx);
    size_t size = table.size * (table.vcount / 4);
    idx *= size;
    quickSort(idx, idx+size-1, nm);
    return NULL;
}

void mergeTwo(int low, int mid, int high, char *name)
{
    int i, j, k;
    int n1 = mid - low + 1;
    int n2 = high - mid;
    int *L, *R;
    L = (int *)malloc(n1 * sizeof(int));
    R = (int *)malloc(n2 * sizeof(int));
    if (NULL == L || NULL == R) {
        fprintf(stderr, "mergeTwo, malloc\n");
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < n1; ++i) {
        L[i] = get(low+i, name);
    }
    for (i = 0; i < n2; ++i) {
        R[i] = get(mid+i+1, name);
    }
    i = 0;
    j = 0;
    k = low;
    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) {
            set(k, L[i], name);
            i++;
        } else {
            set(k, R[j], name);
            j++;
        }
        k++;
    }
    while (i < n1) {
        set(k, L[i], name);
        i++;
        k++;
    }
    while (j < n2) {
        set(k, R[j], name);
        j++;
        k++;
    }
    free(L);
    free(R);
}

void mergeSort(int low, int high, char *name)
{
    if (low >= high) {
        return;
    }
    int mid = low + (high - low) / 2;
    mergeSort(low, mid, name);
    mergeSort(mid+1, high, name);
    mergeTwo(low, mid, high, name);
}

void *mergeMain(void *data)
{
    uint32_t idx = (uintptr_t)data;
    char nm[NAME_LEN];
    sprintf(nm, "merge-%u", idx);
    size_t size = table.size * (table.vcount / 4);
    idx *= size;
    mergeSort(idx, idx+size-1, nm);
    return NULL;
}

uint32_t physicalFrame(uint32_t vFrame, int64_t tid)
{
    if (-1 == tid) {
        tid = countNames();
    }
    if (0 != table.arr[vFrame]->isPresent) {
        table.arr[vFrame]->thread = tid;
        return table.arr[vFrame]->phys;
    }
    stats[tid].pm++;
    int64_t pFrame = findEmpty();
    if (GLOBAL == config.pol) {
        if (-1 == pFrame) {
            stats[tid].pr++;
            pFrame = pageFault(vFrame, tid);
        }
    } else if (LOCAL == config.pol) {
        uint32_t activesCount = countActiveThreads();
        uint32_t count = countTid(tid);
        if (0 == activesCount) {
            if (-1 == pFrame) {
                config.pol = GLOBAL;
                stats[tid].pr++;
                pFrame = pageFault(vFrame, tid);
                config.pol = LOCAL;
            }
        } else if ((table.pcount / activesCount) > count) {
            if (-1 == pFrame) {
                stats[tid].pr++;
                int64_t badTid = biggestTid();
                pFrame = pageFault(vFrame, badTid);
            }
        } else {
            if (0 == (table.pcount % activesCount) || -1 == pFrame) {
                stats[tid].pr++;
                pFrame = pageFault(vFrame, tid);
            }
        }
    }
    setPage(vFrame, pFrame, tid);
    return pFrame;
}

uint32_t pageFault(uint32_t vFrame, int64_t tid)
{
    uint32_t oldVFrame = algoFunc(tid);
    struct PTEntry *entry = table.arr[oldVFrame];
    uint32_t pFrame = entry->phys;
    ssize_t size = sizeof(int) * table.size;
    off_t loc = size * oldVFrame;
    int *ptr = mem;
    ptr += table.size * pFrame;
    if (0 != entry->isMod) {
        if (size != pwrite(fd, ptr, size, loc)) {
            fprintf(stderr, "pageFault, pwrite\n");
            exit(EXIT_FAILURE);
        }
        stats[tid].wr++;
    }
    entry->isPresent = 0;
    entry->isMod = 0;
    entry->refs = 0;
    if (0 < stats[entry->thread].actives) {
        stats[entry->thread].actives--;
    }
    return pFrame;
}

void setPage(uint32_t vFrame, uint32_t pFrame, int64_t tid)
{
    ssize_t size = sizeof(int) * table.size;
    off_t loc = size * vFrame;
    int *ptr = mem;
    ptr += table.size * pFrame;
    if (size != pread(fd, ptr, size, loc)) {
        fprintf(stderr, "setPage, pread\n");
        exit(EXIT_FAILURE);
    }
    struct PTEntry *entry = table.arr[vFrame];
    entry->isPresent = 1;
    entry->isMod = 0;
    entry->refs = 0;
    entry->phys = pFrame;
    if (-1 == tid) {
        uint32_t count = countNames();
        stats[count].actives++;
        tid = count;
    }
    entry->thread = tid;
    stats[tid].rd++;
    if (WSC == config.algo) {
        if (vec->size() < 1 + table.pcount) {
            vec->push_back(vFrame);
        } else if (1 == (*vec)[0]) {
            (*vec)[vec->size()-1] = vFrame;
        } else {
            (*vec)[(*vec)[0]-1] = vFrame;
        }
    }
}

int64_t findEmpty()
{
    static int emptyFulled = 0;
    if (emptyFulled) {
        return -1;
    }
    int64_t *arr = (int64_t *)calloc(table.pcount, sizeof(int64_t));
    if (NULL == arr) {
        fprintf(stderr, "findFree, calloc\n");
        exit(EXIT_FAILURE);
    }
    for (uint32_t i = 0; i < table.pcount; ++i) {
        arr[i] = -1;
    }
    for (uint32_t i = 0; i < table.vcount; ++i) {
        if (table.arr[i]->isPresent) {
            arr[table.arr[i]->phys] = i;
        }
    }
    int64_t res = -1;
    for (uint32_t i = 0; i < table.pcount; ++i) {
        if (-1 == arr[i]) {
            res = i;
            break;
        }
    }
    free(arr);
    if (-1 == res) {
        emptyFulled = 1;
    }
    return res;
}

int64_t findThread(const char *tName)
{
    for (uint32_t i = 0; i < table.tcount; ++i) {
        if (!strcmp(tName, stats[i].name)) {
            return i;
        }
    }
    return -1;
}

uint32_t countTid(int64_t tid)
{
    if (0 > tid) {
        return 0;
    }
    return stats[tid].actives;
}

uint32_t countNames()
{
    uint32_t res = 0;
    for (uint32_t i = 0; i < table.tcount; ++i) {
        if('\0' == stats[i].name[0]) {
            break;
        }
        res++;
    }
    if (res == table.tcount) {
        stats = (struct TStats *)realloc(stats, sizeof(struct TStats)*table.tcount*2);
        if (NULL == stats) {
            fprintf(stderr, "countNames, realloc\n");
            exit(EXIT_FAILURE);
        }
        memset(stats+table.tcount, '\0', sizeof(struct TStats)*table.tcount);
        table.tcount *= 2;
    }
    return res;
}

uint32_t countActiveThreads()
{
    uint32_t res = 0;
    for (uint32_t i = 0; i < table.tcount; ++i) {
        if('\0' == stats[i].name[0]) {
            break;
        }
        if (stats[i].actives) {
            res++;
        }
    }
    return res;
}

int64_t biggestTid()
{
    uint32_t count = 0;
    int64_t tid = -1;
    for (int64_t i = 0; i < countNames(); ++i) {
        if (count <= stats[i].actives) {
            count = stats[i].actives;
            tid = i;
        }
    }
    return tid;
}

uint32_t physicalAddress(unsigned int index, const char *tName, uint8_t mod)
{
    uint32_t vFrame = index >> config.frameSize;
    if (vFrame >= table.vcount) {
        fprintf(stderr, "physicalAddress, vFrame, index(%u)\n", vFrame);
        exit(EXIT_FAILURE);
    }
    uint32_t addr = index - (vFrame << config.frameSize);
    int64_t tid = findThread(tName);
    uint32_t pFrame = physicalFrame(vFrame, tid);
    if (-1 == tid) {
        tid = countNames();
        strncpy(stats[tid].name, tName, NAME_LEN-1);
    }
    switch (config.algo) {
        case NRU:
        case LRU:
        case WSC:
            checkClock();
            break;
        case FIFO:
        case SC:
            lst->push_back(vFrame);
            break;
        default:
            fprintf(stderr, "physicalAddress, switch\n");
            exit(EXIT_FAILURE);
    }
    SET_BIT(table.arr[vFrame]->refs, 0);
    if (mod) {
        table.arr[vFrame]->isMod = 1;
        stats[tid].st++;
    } else {
        stats[tid].gt++;
    }
    addr += (pFrame << config.frameSize);
    table.printCounter++;
    if (table.printCounter >= config.printInt) {
        printTable();
        table.printCounter -= config.printInt;
    }
    return addr;
}

void printTable()
{
    struct PTEntry *e;
    printf("* * *   PAGE  TABLE   * * *\n");
    printf("P M VIRTUAL PHYSICAL THREAD\n");
    for (uint32_t i = 0; i < table.vcount; ++i) {
        e = table.arr[i];
        printf("%1u %1u %7u %6u %s\n",
            e->isPresent,
            e->isMod,
            i,
            e->phys,
            stats[e->thread].name
        );
    }
}

void printStats()
{
    printf("      Thread     #Rds     #Wrs     #PMs     #PRp     #Pwr     #Prd\n");
    for (uint32_t i = 0; i < table.tcount; ++i) {
        if ('\0' == stats[i].name[0]) {
            break;
        }
        printf("%12s %8zu %8zu %8zu %8zu %8zu %8zu\n",
            stats[i].name,
            stats[i].gt,
            stats[i].st,
            stats[i].pm,
            stats[i].pr,
            stats[i].wr,
            stats[i].rd
        );
    }
}

void set(unsigned int index, int value, const char *tName)
{
    if (0 != pthread_mutex_lock(&mtx)) {
        fprintf(stderr, "set, pthread_mutex_lock\n");
        exit(EXIT_FAILURE);
    }
    uint32_t addr = physicalAddress(index, tName, 1);
    mem[addr] = value;
    if (0 != pthread_mutex_unlock(&mtx)) {
        fprintf(stderr, "set, pthread_mutex_unlock\n");
        exit(EXIT_FAILURE);
    }
}

int get(unsigned int index, const char *tName)
{
    if (0 != pthread_mutex_lock(&mtx)) {
        fprintf(stderr, "get, pthread_mutex_lock\n");
        exit(EXIT_FAILURE);
    }
    uint32_t addr = physicalAddress(index, tName, 0);
    int res = mem[addr];
    if (0 != pthread_mutex_unlock(&mtx)) {
        fprintf(stderr, "get, pthread_mutex_unlock\n");
        exit(EXIT_FAILURE);
    }
    return res;
}

uint32_t nruAlgorithm(int64_t tid)
{
    struct PTEntry *entry;
    uint32_t selected;
    uint8_t ref, mod, point=255, temp;
    for (uint32_t i = 0; i < table.vcount; ++i) {
        entry = table.arr[i];
        if (0 == entry->isPresent || (LOCAL == config.pol && entry->thread != tid)) {
            continue;
        }
        ref = GET_BIT(entry->refs, 0);
        mod = entry->isMod;
        temp = (2 * mod) + ref;
        if (temp < point) {
            point = temp;
            selected = i;
            if (0 == temp) {
                break;
            }
        }
    }
    return selected;
}

uint32_t fifoAlgorithm(int64_t tid)
{
    struct PTEntry *entry;
    uint32_t selected;
    list<uint32_t>::iterator it; 
    for (it = lst->begin(); it != lst->end(); ++it) {
        entry = table.arr[*it];
        if (LOCAL == config.pol && entry->thread != tid) {
            continue;
        }
        selected = *it;
        break;
    }
    lst->remove(selected);
    return selected;
}

uint32_t scAlgorithm(int64_t tid)
{
    struct PTEntry *entry;
    int64_t selected = -1;
    list<uint32_t>::iterator it; 
    do {
        for (it = lst->begin(); it != lst->end(); ++it) {
            entry = table.arr[*it];
            if (LOCAL == config.pol && entry->thread != tid) {
                continue;
            }
            selected = *it;
            if (0 != GET_BIT(entry->refs, 0)) {
                CLR_BIT(entry->refs, 0);
                lst->remove(selected);
                lst->push_back(selected);
                selected = -1;
            }
            break;
        }
    } while (-1 == selected);
    lst->remove(selected);
    return selected;
}

uint32_t lruAlgorithm(int64_t tid)
{
    struct PTEntry *entry;
    uint32_t selected;
    uint16_t ref, mod, point=65535, temp;
    for (uint32_t i = 0; i < table.vcount; ++i) {
        entry = table.arr[i];
        if (0 == entry->isPresent || (LOCAL == config.pol && entry->thread != tid)) {
            continue;
        }
        ref = entry->refs;
        mod = entry->isMod;
        temp = mod | (ref << 1);
        if (temp < point) {
            point = temp;
            selected = i;
            if (0 == temp) {
                break;
            }
        }
    }
    return selected;
}

uint32_t wscAlgorithm(int64_t tid)
{
    struct PTEntry *entry;
    uint32_t selected=999999, pos;
    uint32_t i = (*vec)[0];
    for (;;) {
        pos = (*vec)[i];
        entry = table.arr[pos];
        i++;
        if (vec->size() <= i) {
            i = 1;
        }
        if (LOCAL == config.pol && entry->thread != tid) {
            continue;
        }
        if (0 == GET_BIT(entry->refs, 0)) {
            selected = pos;
            break;
        }
        CLR_BIT(entry->refs, 0);
    }
    (*vec)[0] = i;
    return selected;
}