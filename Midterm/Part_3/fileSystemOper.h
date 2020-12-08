#pragma once

#include <stdio.h>
#include <stdint.h>

#include "filesystem.h"

struct Inode *getInode(uint8_t *fs, uint16_t num);
uint8_t *getBlock(uint8_t *fs, uint16_t num);
uint16_t firstEmptyBlock(uint8_t *fs);
uint16_t firstEmptyInode(uint8_t *fs);
struct File *getFileInDir(uint8_t *fs, struct Inode *inode, uint64_t num);
struct File *firstEmptySlot(uint8_t *fs, struct Inode *inode);
struct File *lastFullSlot(uint8_t *fs, struct Inode *inode);
uint8_t *emptyFileBlock(uint8_t *fs, struct Inode *inode);
uint8_t *fileBlock(uint8_t *fs, struct Inode *inode, uint64_t num);
uint8_t fileBlockNum(uint8_t *fs, struct Inode *inode, uint64_t num, uint16_t *res);
void findUpperPath(char *dest, char *str);
struct File *getFile(uint8_t *fs, struct Inode *inode, char *name);
struct Inode *findPathInode(uint8_t *fs, char *path);
size_t singleLength(char *path);
void freeFileBlocks(uint8_t *fs, struct File *file);
uint16_t emptyBlocksCount(uint8_t *fs);
uint16_t emptyInodesCount(uint8_t *fs);
void checkCapacities(uint8_t *fs, uint8_t checkBlocks, uint8_t checkInodes);
void listCmd(uint8_t *fs, char *path);
void mkdirCmd(uint8_t *fs, char *path);
void rmdirCmd(uint8_t *fs, char *path);
void dumpe2fsCmd(uint8_t *fs);
void writeCmd(uint8_t *fs, char *path, char *linuxFile);
void readCmd(uint8_t *fs, char *path, char *linuxFile);
void delCmd(uint8_t *fs, char *path);
void lnCmd(uint8_t *fs, char *filePath, char *linkPath);
void lnsymCmd(uint8_t *fs, char *filePath, char *linkPath);
void fsckCmd(uint8_t *fs);
void traverseDirs(uint8_t *fs, uint8_t *vMap, uint8_t *counts, struct Inode *inode);
void printInfos(uint8_t *fs, uint16_t *counts, uint8_t *vMap, struct Inode *inode);
void printInode(uint8_t *fs, uint16_t num);