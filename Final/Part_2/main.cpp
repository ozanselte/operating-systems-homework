#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pageTable.h"
#include "main.h"

extern struct VConfig config;

int main(int argc, char *argv[])
{
    memset(&config, '\0', sizeof(config));
    argsParser(argc, argv);
    initTable();
    initVirtualMemory();
    initPhysicalMemory();
    fillAll();
    createThreads();
    waitAlgos();
    checkSorting();
    printStats();
    freeResources();
    return EXIT_SUCCESS;
}

void argsParser(int argc, char *argv[])
{
    if (ARGV_COUNT != argc) {
        printUsage(argv[0]);
    }
    int temp;
    temp = atoi(argv[1]);
    if (0 > temp) {
        printUsage(argv[0]);
    }
    config.frameSize = temp;
    temp = atoi(argv[2]);
    if (0 > temp) {
        printUsage(argv[0]);
    }
    config.numPhysical = temp;
    temp = atoi(argv[3]);
    if (0 > temp) {
        printUsage(argv[0]);
    }
    config.numVirtual = temp;
    if (!strcmp(argv[4], "NRU")) {
        config.algo = NRU;
    } else if (!strcmp(argv[4], "FIFO")) {
        config.algo = FIFO;
    } else if (!strcmp(argv[4], "SC")) {
        config.algo = SC;
    } else if (!strcmp(argv[4], "LRU")) {
        config.algo = LRU;
    } else if (!strcmp(argv[4], "WSClock")) {
        config.algo = WSC;
    } else {
        printUsage(argv[0]);
    }
    if (!strcmp(argv[5], "global")) {
        config.pol = GLOBAL;
    } else if (!strcmp(argv[5], "local")) {
        config.pol = LOCAL;
        if (2 > config.numPhysical || 2 > config.numVirtual) {
            printUsage(argv[0]);
        }
    } else {
        printUsage(argv[0]);
    }
    config.printInt = atoi(argv[6]);
    strcpy(config.path, argv[7]);
}

void printUsage(char *name)
{
    fprintf(stderr, "Invalid usage! If LOCAL:\n");
    fprintf(stderr, "Physical and Virtual page count must be larger than 2.\n");
    exit(EXIT_FAILURE);
}