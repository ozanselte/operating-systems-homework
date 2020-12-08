#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>

#include "filesystem.h"
#include "fileSystemOper.h"

int main(int argc, char *argv[])
{
    if (3 > argc || 5 < argc) {
        fprintf(stderr, "Invalid usage!\n");
        exit(EXIT_FAILURE);
    }
    argc--; argv++;
    char *fileName = *argv;
    FILE *file;
    uint8_t filesystem[MB_1] = {0};
    file = fopen(fileName, "rb");
    if (NULL == file) {
        fprintf(stderr, "fopen error!\n");
        exit(EXIT_FAILURE);
    }
    if (1 != fread(filesystem, MB_1, 1, file)) {
        fprintf(stderr, "fread error!\n");
        exit(EXIT_FAILURE);
    }
    fclose(file);

    argc--; argv++;
    char *path1=NULL, *path2=NULL;
    if (1 < argc) {
        path1 = argv[1];
    }
    if (2 < argc) {
        path2 = argv[2];
    }
    if (!strcmp(*argv, "list")) {
        if (NULL == path1) {
            fprintf(stderr, "Invalid usage!\n");
            exit(EXIT_FAILURE);
        }
        listCmd(filesystem, path1);
        printf("LIST END\n");
    } else if (!strcmp(*argv, "mkdir")) {
        if (NULL == path1) {
            fprintf(stderr, "Invalid usage!\n");
            exit(EXIT_FAILURE);
        }
        mkdirCmd(filesystem, path1);
        printf("MKDIR END\n");
    } else if (!strcmp(*argv, "rmdir")) {
        if (NULL == path1) {
            fprintf(stderr, "Invalid usage!\n");
            exit(EXIT_FAILURE);
        }
        rmdirCmd(filesystem, path1);
        printf("RMDIR END\n");
    } else if (!strcmp(*argv, "dumpe2fs")) {
        dumpe2fsCmd(filesystem);
        printf("DUMPE2FS END\n");
    } else if (!strcmp(*argv, "write")) {
        if (NULL == path1 || NULL == path2) {
            fprintf(stderr, "Invalid usage!\n");
            exit(EXIT_FAILURE);
        }
        writeCmd(filesystem, path1, path2);
        printf("WRITE END\n");
    } else if (!strcmp(*argv, "read")) {
        if (NULL == path1 || NULL == path2) {
            fprintf(stderr, "Invalid usage!\n");
            exit(EXIT_FAILURE);
        }
        readCmd(filesystem, path1, path2);
        printf("READ END\n");
    } else if (!strcmp(*argv, "del")) {
        if (NULL == path1) {
            fprintf(stderr, "Invalid usage!\n");
            exit(EXIT_FAILURE);
        }
        delCmd(filesystem, path1);
        printf("DEL END\n");
    } else if (!strcmp(*argv, "ln")) {
        if (NULL == path1 || NULL == path2) {
            fprintf(stderr, "Invalid usage!\n");
            exit(EXIT_FAILURE);
        }
        lnCmd(filesystem, path1, path2);
        printf("LN END\n");
    } else if (!strcmp(*argv, "lnsym")) {
        if (NULL == path1 || NULL == path2) {
            fprintf(stderr, "Invalid usage!\n");
            exit(EXIT_FAILURE);
        }
        lnsymCmd(filesystem, path1, path2);
        printf("LNSYM END\n");
    } else if (!strcmp(*argv, "fsck")) {
        fsckCmd(filesystem);
        printf("FSCK END\n");
    }

    file = fopen(fileName, "wb+");
    if (NULL == file) {
        perror("fopen");
    }
    if (1 != fwrite(filesystem, MB_1, 1, file)) {
        fprintf(stderr, "fwrite error!\n");
        exit(EXIT_FAILURE);
    }
    fclose(file);
    return EXIT_SUCCESS;
}

struct Inode *getInode(uint8_t *fs, uint16_t num)
{
    struct SuperBlock *s = (struct SuperBlock *)fs;
    uint32_t location = num * sizeof(struct Inode);
    return (struct Inode *)(fs + s->firstInodeAddr + location);
}

uint8_t *getBlock(uint8_t *fs, uint16_t num)
{
    struct SuperBlock *s = (struct SuperBlock *)fs;
    uint32_t location = num * s->blockSize * KB_1;
    return (uint8_t *)(fs + s->firstBlockAddr + location);
}

uint16_t firstEmptyBlock(uint8_t *fs)
{
    struct SuperBlock *s = (struct SuperBlock *)fs;
    for (size_t i = 0; i < s->blocksCount; ++i) {
        if (0 == GET_BIT(s->blocksBitmap, i)) {
            return i;
        }
    }
    fprintf(stderr, "Cannot find an empty block!\n");
    exit(EXIT_FAILURE);
}

uint16_t firstEmptyInode(uint8_t *fs)
{
    struct SuperBlock *s = (struct SuperBlock *)fs;
    for (size_t i = 0; i < s->inodesCount; ++i) {
        if (0 == getInode(fs, i)->linkCount) {
            return i;
        }
    }
    fprintf(stderr, "Cannot find an empty inode!\n");
    exit(EXIT_FAILURE);
}

struct File *getFileInDir(uint8_t *fs, struct Inode *inode, uint64_t num)
{
    struct SuperBlock *s = (struct SuperBlock *)fs;
    uint8_t *block = NULL;
    uint64_t iC = (s->blockSize * KB_1) / ADDR_SIZE;
    uint64_t c = s->filesCount;
    uint64_t sC = c * iC;
    uint64_t dC = sC * iC;
    uint64_t tC = dC * iC;
    uint64_t cap = c*DRCT_CNT + sC*IND1_CNT + dC*IND2_CNT + tC*IND3_CNT;
    if (cap <= num) {
        fprintf(stderr, "Inode is at full capacity!\n");
        exit(EXIT_FAILURE);
    }
    cap -= tC * IND3_CNT;
    if (cap <= num) {
        num -= cap;
        block = getBlock(fs, inode->tripleI[num/tC]);
        num %= tC;
    }
    cap -= dC * IND2_CNT;
    if (cap <= num) {
        if (NULL == block) {
            num -= cap;
            block = getBlock(fs, inode->doubleI[num/dC]);
        } else {
            uint16_t *shifter = (uint16_t *)block;
            shifter += num / dC;
            block = getBlock(fs, *shifter);
        }
        num %= dC;
    }
    cap -= sC * IND1_CNT;
    if (cap <= num) {
        if (NULL == block) {
            num -= cap;
            block = getBlock(fs, inode->singleI[num/sC]);
        } else {
            uint16_t *shifter = (uint16_t *)block;
            shifter += num / sC;
            block = getBlock(fs, *shifter);
        }
        num %= sC;
    }
    if (NULL == block) {
        block = getBlock(fs, inode->direct[num/c]);
    } else {
        uint16_t *shifter = (uint16_t *)block;
        shifter += num / c;
        block = getBlock(fs, *shifter);
    }
    num %= c;
    struct File *file = (struct File *)(block + num*sizeof(struct File));
    return file;
}

struct File *firstEmptySlot(uint8_t *fs, struct Inode *inode)
{
    uint64_t num = inode->size / sizeof(struct File);
    struct SuperBlock *s = (struct SuperBlock *)fs;
    uint8_t *block = NULL;
    uint64_t iC = (s->blockSize * KB_1) / ADDR_SIZE;
    uint64_t c = s->filesCount;
    uint64_t sC = c * iC;
    uint64_t dC = sC * iC;
    uint64_t tC = dC * iC;
    uint64_t cap = c*DRCT_CNT + sC*IND1_CNT + dC*IND2_CNT + tC*IND3_CNT;
    if (cap <= num) {
        fprintf(stderr, "Inode is at full capacity!\n");
        exit(EXIT_FAILURE);
    }
    cap -= tC * IND3_CNT;
    if (cap <= num) {
        num -= cap;
        if (0 == num % tC) {
            inode->tripleI[num/tC] = firstEmptyBlock(fs);
            SET_BIT(s->blocksBitmap, inode->tripleI[num/tC]);
        }
        block = getBlock(fs, inode->tripleI[num/tC]);
        num %= tC;
    }
    cap -= dC * IND2_CNT;
    if (cap <= num) {
        if (NULL == block) {
            num -= cap;
            if (0 == num % dC) {
                inode->doubleI[num/dC] = firstEmptyBlock(fs);
                SET_BIT(s->blocksBitmap, inode->doubleI[num/dC]);
            }
            block = getBlock(fs, inode->doubleI[num/dC]);
        } else {
            uint16_t *shifter = (uint16_t *)block;
            shifter += num / dC;
            if (0 == num % dC) {
                *shifter = firstEmptyBlock(fs);
                SET_BIT(s->blocksBitmap, *shifter);
            }
            block = getBlock(fs, *shifter);
        }
        num %= dC;
    }
    cap -= sC * IND1_CNT;
    if (cap <= num) {
        if (NULL == block) {
            num -= cap;
            if (0 == num % sC) {
                inode->singleI[num/sC] = firstEmptyBlock(fs);
                SET_BIT(s->blocksBitmap, inode->singleI[num/sC]);
            }
            block = getBlock(fs, inode->singleI[num/sC]);
        } else {
            uint16_t *shifter = (uint16_t *)block;
            shifter += num / sC;
            if (0 == num % sC) {
                *shifter = firstEmptyBlock(fs);
                SET_BIT(s->blocksBitmap, *shifter);
            }
            block = getBlock(fs, *shifter);
        }
        num %= sC;
    }
    if (NULL == block) {
        if (0 == num % c) {
            inode->direct[num/c] = firstEmptyBlock(fs);
            SET_BIT(s->blocksBitmap, inode->direct[num/c]);
        }
        block = getBlock(fs, inode->direct[num/c]);
    } else {
        uint16_t *shifter = (uint16_t *)block;
        shifter += num / c;
        if (0 == num % c) {
            *shifter = firstEmptyBlock(fs);
            SET_BIT(s->blocksBitmap, *shifter);
        }
        block = getBlock(fs, *shifter);
    }
    num %= c;
    struct File *file = (struct File *)(block + num*sizeof(struct File));
    return file;
}

struct File *lastFullSlot(uint8_t *fs, struct Inode *inode)
{
    uint64_t num = inode->size / sizeof(struct File);
    if (0 == num) {
        fprintf(stderr, "Inode error!\n");
        exit(EXIT_FAILURE);
    }
    return getFileInDir(fs, inode, num-1);
}

uint8_t *emptyFileBlock(uint8_t *fs, struct Inode *inode)
{
    struct SuperBlock *s = (struct SuperBlock *)fs;
    uint64_t num = inode->size / (s->blockSize * KB_1);
    uint8_t *block = NULL;
    uint64_t iC = (s->blockSize * KB_1) / ADDR_SIZE;
    uint64_t sC = iC;
    uint64_t dC = sC * iC;
    uint64_t tC = dC * iC;
    uint64_t cap = DRCT_CNT + sC*IND1_CNT + dC*IND2_CNT + tC*IND3_CNT;
    if (cap <= num) {
        fprintf(stderr, "Inode is at full capacity!\n");
        exit(EXIT_FAILURE);
    }
    cap -= tC * IND3_CNT;
    if (cap <= num) {
        num -= cap;
        if (0 == num % tC) {
            inode->tripleI[num/tC] = firstEmptyBlock(fs);
            SET_BIT(s->blocksBitmap, inode->tripleI[num/tC]);
        }
        block = getBlock(fs, inode->tripleI[num/tC]);
        num %= tC;
    }
    cap -= dC * IND2_CNT;
    if (cap <= num || block != NULL) {
        if (NULL == block) {
            num -= cap;
            if (0 == num % dC) {
                inode->doubleI[num/dC] = firstEmptyBlock(fs);
                SET_BIT(s->blocksBitmap, inode->doubleI[num/dC]);
            }
            block = getBlock(fs, inode->doubleI[num/dC]);
        } else {
            uint16_t *shifter = (uint16_t *)block;
            shifter += num / dC;
            if (0 == num % dC) {
                *shifter = firstEmptyBlock(fs);
                SET_BIT(s->blocksBitmap, *shifter);
            }
            block = getBlock(fs, *shifter);
        }
        num %= dC;
    }
    cap -= sC * IND1_CNT;
    if (cap <= num || block != NULL) {
        if (NULL == block) {
            num -= cap;
            if (0 == num % sC) {
                inode->singleI[num/sC] = firstEmptyBlock(fs);
                SET_BIT(s->blocksBitmap, inode->singleI[num/sC]);
            }
            block = getBlock(fs, inode->singleI[num/sC]);
        } else {
            uint16_t *shifter = (uint16_t *)block;
            shifter += num / sC;
            if (0 == num % sC) {
                *shifter = firstEmptyBlock(fs);
                SET_BIT(s->blocksBitmap, *shifter);
            }
            block = getBlock(fs, *shifter);
        }
        num %= sC;
    }
    if (NULL == block) {
        inode->direct[num] = firstEmptyBlock(fs);
        SET_BIT(s->blocksBitmap, inode->direct[num]);
        block = getBlock(fs, inode->direct[num]);
    } else {
        uint16_t *shifter = (uint16_t *)block;
        shifter += num;
        *shifter = firstEmptyBlock(fs);
        SET_BIT(s->blocksBitmap, *shifter);
        block = getBlock(fs, *shifter);
    }
    return block;
}

uint8_t *fileBlock(uint8_t *fs, struct Inode *inode, uint64_t num)
{
    struct SuperBlock *s = (struct SuperBlock *)fs;
    uint8_t *block = NULL;
    uint64_t iC = (s->blockSize * KB_1) / ADDR_SIZE;
    uint64_t sC = iC;
    uint64_t dC = sC * iC;
    uint64_t tC = dC * iC;
    uint64_t cap = DRCT_CNT + sC*IND1_CNT + dC*IND2_CNT + tC*IND3_CNT;
    if (cap <= num) {
        fprintf(stderr, "Inode is at full capacity!\n");
        exit(EXIT_FAILURE);
    }
    cap -= tC * IND3_CNT;
    if (cap <= num) {
        num -= cap;
        block = getBlock(fs, inode->tripleI[num/tC]);
        num %= tC;
    }
    cap -= dC * IND2_CNT;
    if (cap <= num || block != NULL) {
        if (NULL == block) {
            num -= cap;
            block = getBlock(fs, inode->doubleI[num/dC]);
        } else {
            uint16_t *shifter = (uint16_t *)block;
            shifter += num / dC;
            block = getBlock(fs, *shifter);
        }
        num %= dC;
    }
    cap -= sC * IND1_CNT;
    if (cap <= num || block != NULL) {
        if (NULL == block) {
            num -= cap;
            block = getBlock(fs, inode->singleI[num/sC]);
        } else {
            uint16_t *shifter = (uint16_t *)block;
            shifter += num / sC;
            block = getBlock(fs, *shifter);
        }
        num %= sC;
    }
    if (NULL == block) {
        block = getBlock(fs, inode->direct[num]);
    } else {
        uint16_t *shifter = (uint16_t *)block;
        shifter += num;
        block = getBlock(fs, *shifter);
    }
    return block;
}

uint8_t fileBlockNum(uint8_t *fs, struct Inode *inode, uint64_t num, uint16_t *res)
{
    struct SuperBlock *s = (struct SuperBlock *)fs;
    uint8_t cnt = 0;
    uint8_t *block = NULL;
    uint64_t iC = (s->blockSize * KB_1) / ADDR_SIZE;
    uint64_t sC = iC;
    uint64_t dC = sC * iC;
    uint64_t tC = dC * iC;
    uint64_t cap = DRCT_CNT + sC*IND1_CNT + dC*IND2_CNT + tC*IND3_CNT;
    if (cap <= num) {
        fprintf(stderr, "Inode is at full capacity!\n");
        exit(EXIT_FAILURE);
    }
    cap -= tC * IND3_CNT;
    if (cap <= num) {
        num -= cap;
        block = getBlock(fs, inode->tripleI[num/tC]);
        if (0 == num % tC) {
            res[cnt++] = inode->tripleI[num/tC];
        }
        num %= tC;
    }
    cap -= dC * IND2_CNT;
    if (cap <= num || block != NULL) {
        if (NULL == block) {
            num -= cap;
            block = getBlock(fs, inode->doubleI[num/dC]);
            if (0 == num % dC) {
                res[cnt++] = inode->doubleI[num/dC];
            }
        } else {
            uint16_t *shifter = (uint16_t *)block;
            shifter += num / dC;
            block = getBlock(fs, *shifter);
            if (0 == num % dC) {
                res[cnt++] = *shifter;
            }
        }
        num %= dC;
    }
    cap -= sC * IND1_CNT;
    if (cap <= num || block != NULL) {
        if (NULL == block) {
            num -= cap;
            block = getBlock(fs, inode->singleI[num/sC]);
            if (0 == num % sC) {
                res[cnt++] = inode->singleI[num/sC];
            }
        } else {
            uint16_t *shifter = (uint16_t *)block;
            shifter += num / sC;
            block = getBlock(fs, *shifter);
            if (0 == num % sC) {
                res[cnt++] = *shifter;
            }
        }
        num %= sC;
    }
    if (NULL == block) {
        res[cnt++] = inode->direct[num];
    } else {
        uint16_t *shifter = (uint16_t *)block;
        shifter += num;
        res[cnt++] = *shifter;
    }
    return cnt;
}

struct File *getFile(uint8_t *fs, struct Inode *inode, char *name)
{
    struct File *file = NULL;
    uint64_t num = inode->size / sizeof(struct File);
    for (uint64_t i = 0; i < num; ++i) {
        file = getFileInDir(fs, inode, i);
        if (!strcmp(file->name, name)) {
            return file;
        }
    }
    return NULL;
}

void findUpperPath(char *dest, char *src)
{
    strcpy(dest, src);
    size_t i = strlen(dest) - 1;
    if ('/' == dest[i]) {
        fprintf(stderr, "Invalid path!\n");
        exit(EXIT_FAILURE);
    }
    for (; 0 < i; --i) {
        if ('/' != dest[i]) {
            dest[i] = '\0';
        } else {
            dest[i] = '\0';
            break;
        }
    }
    if (KEY_LEN <= strlen(src)-strlen(dest)) {
        fprintf(stderr, "Too long file name!\n");
        exit(EXIT_FAILURE);
    }
}

struct Inode *findPathInode(uint8_t *fs, char *path)
{
    if ('/' != path[0]) {
        fprintf(stderr, "Path noth found!\n");
        exit(EXIT_FAILURE);
    }
    path++;
    char name[KB_1];
    uint16_t inodeNum = 0;
    struct Inode *inode = getInode(fs, inodeNum);
    struct File *file = NULL;
    while (strlen(path)) {
        size_t keylen = singleLength(path);
        if (KEY_LEN <= keylen) {
            fprintf(stderr, "Too long file name!\n");
            exit(EXIT_FAILURE);
        }
        switch (inode->type) {
            case T_FIL:
                fprintf(stderr, "Path not found!\n");
                exit(EXIT_FAILURE);
                break;
            case T_DIR:
                strncpy(name, path, keylen);
                name[keylen] = '\0';
                file = getFile(fs, inode, name);
                if (NULL == file) {
                    fprintf(stderr, "%s not found!\n", path);
                    exit(EXIT_FAILURE);
                }
                inodeNum = file->inode;
                break;
            case T_SYM:
                inodeNum = getFileInDir(fs, findPathInode(fs, (char *)fileBlock(fs, inode, 0)), 0)->inode;
                if (0 == getInode(fs, inodeNum)->linkCount) {
                    fprintf(stderr, "Symbolic linked file is not found!\n");
                    exit(EXIT_FAILURE);
                }
                break;
            default:
                fprintf(stderr, "Unknown file type!\n");
                exit(EXIT_FAILURE);
        }
        inode = getInode(fs, inodeNum);
        path += keylen;
        if (0 < strlen(path)) {
            path++;
        }
    }
    return inode;
}

size_t singleLength(char *path)
{
    size_t len = 0;
    while ('/' != path[len] && '\0' != path[len])
    {
        ++len;
    }
    return len;
}

void freeFileBlocks(uint8_t *fs, struct File *file)
{
    struct SuperBlock *s = (struct SuperBlock *)fs;
    struct Inode *delInode = getInode(fs, file->inode);
    uint64_t iC = (s->blockSize * KB_1) / ADDR_SIZE;
    uint64_t blockCount = (delInode->size + (s->blockSize*KB_1) - 1) / (s->blockSize * KB_1);
    for (uint64_t i = 0; i < DRCT_CNT && 0 < blockCount; ++i) {
        CLR_BIT(s->blocksBitmap, delInode->direct[i]);
        blockCount--;
    }
    for (uint64_t i = 0; i < IND1_CNT && 0 < blockCount; ++i) {
        uint16_t *sAddr = (uint16_t *)getBlock(fs, delInode->singleI[i]);
        for (uint64_t j = 0; j < iC && 0 < blockCount; ++j) {
            CLR_BIT(s->blocksBitmap, *sAddr);
            blockCount--;
            sAddr++;
        }
        CLR_BIT(s->blocksBitmap, delInode->singleI[i]);
    }
    for (uint64_t i = 0; i < IND2_CNT && 0 < blockCount; ++i) {
        uint16_t *dAddr = (uint16_t *)getBlock(fs, delInode->doubleI[i]);
        for (uint64_t j = 0; j < iC && 0 < blockCount; ++j) {
            uint16_t *sAddr = (uint16_t *)getBlock(fs, *dAddr);
            for (uint64_t k = 0; k < iC && 0 < blockCount; ++k) {
                CLR_BIT(s->blocksBitmap, *sAddr);
                blockCount--;
                sAddr++;
            }
            CLR_BIT(s->blocksBitmap, *dAddr);
            dAddr++;
        }
        CLR_BIT(s->blocksBitmap, delInode->doubleI[i]);
    }
    for (uint64_t i = 0 ; i < IND3_CNT && 0 < blockCount; ++i) {
        uint16_t *tAddr = (uint16_t *)getBlock(fs, delInode->tripleI[i]);
        for (uint64_t j = 0; j < iC && 0 < blockCount; ++j) {
            uint16_t *dAddr = (uint16_t *)getBlock(fs, *tAddr);
            for (uint64_t k = 0; k < iC && 0 < blockCount; ++k) {
                uint16_t *sAddr = (uint16_t *)getBlock(fs, *dAddr);
                for (uint64_t m = 0; m < iC && 0 < blockCount; ++m) {
                    CLR_BIT(s->blocksBitmap, *sAddr);
                    blockCount--;
                    sAddr++;
                }
                CLR_BIT(s->blocksBitmap, *dAddr);
                dAddr++;
            }
            CLR_BIT(s->blocksBitmap, *tAddr);
            tAddr++;
        }
        CLR_BIT(s->blocksBitmap, delInode->tripleI[i]);
    }
    delInode->size = 0;
}

uint16_t emptyBlocksCount(uint8_t *fs)
{
    struct SuperBlock *s = (struct SuperBlock *)fs;
    uint16_t count = s->blocksCount;
    for (uint16_t i = 0; i < s->blocksCount; ++i) {
        if (GET_BIT(s->blocksBitmap, i)) {
            count--;
        }
    }
    return count;
}

uint16_t emptyInodesCount(uint8_t *fs)
{
    struct SuperBlock *s = (struct SuperBlock *)fs;
    uint16_t count = s->inodesCount;
    for (uint16_t i = 0; i < s->inodesCount; ++i) {
        if (0 < getInode(fs, i)->linkCount) {
            count--;
        }
    }
    return count;
}

void checkCapacities(uint8_t *fs, uint8_t checkBlocks, uint8_t checkInodes)
{
    if (emptyBlocksCount(fs) < checkBlocks) {
        fprintf(stderr, "Cannot find enough empty blocks %u/%u!\n",
            checkBlocks, emptyBlocksCount(fs));
        exit(EXIT_FAILURE);
    }
    if (emptyInodesCount(fs) < checkInodes) {
        fprintf(stderr, "Cannot find enough empty inodes %u/%u!\n",
            checkInodes, emptyInodesCount(fs));
        exit(EXIT_FAILURE);
    }
}

void listCmd(uint8_t *fs, char *path)
{
    struct Inode *inode = findPathInode(fs, path);
    if (T_DIR != inode->type) {
        fprintf(stderr, "%s is not a directory!\n", path);
        exit(EXIT_FAILURE);
    }
    struct File *file = NULL;
    uint64_t num = inode->size / sizeof(struct File);
    for (uint64_t i = 0; i < num; ++i) {
        file = getFileInDir(fs, inode, i);
        struct Inode *finode = getInode(fs, file->inode);
        time_t t = (time_t)(finode->lastTime);
        char *timeStr = asctime(gmtime(&t));
        timeStr[strlen(timeStr)-1] = '\0';
        printf("%3s %7u %s %s\n", T2S(finode->type), finode->size, timeStr, file->name);
    }
}

void mkdirCmd(uint8_t *fs, char *path)
{
    char str[KB_1];
    findUpperPath(str, path);
    struct Inode *inode = findPathInode(fs, str);
    path += strlen(str);
    if (strlen(str) != 1) {
        path++;
    }
    if (T_DIR != inode->type) {
        fprintf(stderr, "%s is not a directory!\n", str);
        exit(EXIT_FAILURE);
    }
    struct SuperBlock *s = (struct SuperBlock *)fs;
    if (NULL != getFile(fs, inode, path)) {
        fprintf(stderr, "%s is already using!\n", path);
        exit(EXIT_FAILURE);
    }
    checkCapacities(fs, 1, 1);
    struct File *file = (struct File *)getBlock(fs, inode->direct[0]);
    uint16_t block = firstEmptyBlock(fs);
    SET_BIT(s->blocksBitmap, block);
    struct File *newDir = (struct File *)getBlock(fs, block);
    memset(newDir, '\0', s->blockSize*KB_1);
    strcpy(newDir->name, ".");
    newDir->inode = firstEmptyInode(fs);
    strcpy((newDir+1)->name, "..");
    (newDir+1)->inode = file->inode;

    file = firstEmptySlot(fs, inode);
    strcpy(file->name, path);
    file->inode = newDir->inode;
    inode->size += sizeof(struct File);
    inode->lastTime = (int32_t)time(NULL);

    inode = getInode(fs, file->inode);
    memset(inode, '\0', sizeof(struct Inode));
    inode->linkCount = 1;
    inode->type = T_DIR;
    inode->size = 2 * sizeof(struct File);
    inode->lastTime = (int32_t)time(NULL);
    inode->direct[0] = block;
}

void rmdirCmd(uint8_t *fs, char *path)
{
    char str[KB_1];
    findUpperPath(str, path);
    struct Inode *inode = findPathInode(fs, str);
    path += strlen(str);
    if (strlen(str) != 1) {
        path++;
    }
    if (T_DIR != inode->type) {
        fprintf(stderr, "%s is not a directory!\n", str);
        exit(EXIT_FAILURE);
    }
    struct File *file = (struct File *)getFile(fs, inode, path);
    if (NULL == file) {
        fprintf(stderr, "%s is not found!\n", path);
        exit(EXIT_FAILURE);
    }
    struct Inode *delInode = getInode(fs, file->inode);
    if (T_DIR != delInode->type) {
        fprintf(stderr, "%s is not a directory!\n", path);
        exit(EXIT_FAILURE);
    }
    if (2 * sizeof(struct File) != delInode->size) {
        fprintf(stderr, "%s is not an empty directory!\n", path);
        exit(EXIT_FAILURE);
    }

    delInode = getInode(fs, file->inode);
    if (0 == --(delInode->linkCount)) {
        freeFileBlocks(fs, file);
        getInode(fs, file->inode)->linkCount = 0;
    }
    struct File *full = lastFullSlot(fs, inode);
    memcpy(file, full, sizeof(struct File));
    memset(full, '\0', sizeof(struct File));
    inode->size -= sizeof(struct File);
    inode->lastTime = (uint32_t)time(NULL);
}

void dumpe2fsCmd(uint8_t *fs)
{
    struct SuperBlock *s = (struct SuperBlock *)fs;
    uint16_t emptyBlocks = emptyBlocksCount(fs);
    uint16_t emptyInodes = emptyInodesCount(fs);
    printf("Block Size(KB) :   %8u\n", s->blockSize);
    printf("Blocks(Free)   :   %8u\n", emptyBlocks);
    printf("Blocks(Total)  :   %8u\n", s->blocksCount);
    printf("Inodes(Free)   :   %8u\n", emptyInodes);
    printf("Inodes(Total)  :   %8u\n", s->inodesCount);

    struct Inode *root = getInode(fs, 0);
    printf("--- --- --- --- ---\n");
    printf("Inode:\t%4u %3s\n", 0, T2S(getInode(fs, 0)->type));
    printf("Names:\t(root)");
    printf("\nBlocks:\t");
    uint64_t blockCount = (root->size + (s->blockSize*KB_1) - 1) / (s->blockSize * KB_1);
    for (uint64_t i = 0; i < blockCount; ++i) {
        uint16_t res[4] = {0};
        uint16_t cnt = fileBlockNum(fs, root, i, res);
        for (uint16_t j = 0; j < cnt; ++j) {
            printf("%u, ", res[j]);
        }
    }
    printf("\n");

    uint16_t typeCounts[3] = {0};
    uint8_t *visited = (uint8_t *)calloc((s->inodesCount+7)/8, sizeof(uint8_t));
    SET_BIT(visited, 0);
    printInfos(fs, typeCounts, visited, root);
    free(visited);
    printf("--- --- --- --- ---\n");
    printf("Files          :   %8u\n", typeCounts[T_FIL]);
    printf("Directories    :   %8u\n", typeCounts[T_DIR]);
    printf("Symbolic Links :   %8u\n", typeCounts[T_SYM]);
    printf("Free Blocks    :   ");
    for (uint16_t i = 0; i < s->blocksCount; ++i) {
        if (0 == GET_BIT(s->blocksBitmap, i)) {
            printf("%u, ", i);
        }
    }
    printf("\nFree Inodes    :   ");
    for (uint16_t i = 0; i < s->inodesCount; ++i) {
        if (0 == getInode(fs, i)->linkCount) {
            printf("%u, ", i);
        }
    }
    printf("\n");
}

void writeCmd(uint8_t *fs, char *path, char *linuxFile)
{
    struct SuperBlock *s = (struct SuperBlock *)fs;
    char str[KB_1];
    findUpperPath(str, path);
    struct Inode *dInode = findPathInode(fs, str);
    while (T_SYM == dInode->type) {
        dInode = findPathInode(fs, (char *)fileBlock(fs, dInode, 0));
    }
    path += strlen(str);
    if (strlen(str) != 1) {
        path++;
    }
    if (T_DIR != dInode->type) {
        fprintf(stderr, "%s is not a directory!\n", str);
        exit(EXIT_FAILURE);
    }
    struct File *file = NULL;
    if (NULL != getFile(fs, dInode, path)) {
        file = getFile(fs, dInode, path);
        struct Inode *tmp = getInode(fs, file->inode);
        while (T_SYM == tmp->type) {
            tmp = findPathInode(fs, (char *)fileBlock(fs, tmp, 0));
        }
        if (T_FIL != tmp->type) {
            fprintf(stderr, "%s is not a file!\n", path);
            exit(EXIT_FAILURE);
        }
        freeFileBlocks(fs, file);
    }

    uint8_t rawFile[MB_1] = {0};
    FILE *fp = fopen(linuxFile, "rb");
    if (NULL == fp) {
        fprintf(stderr, "fopen error!\n");
        exit(EXIT_FAILURE);
    }
    uint32_t size = (uint32_t)fread(rawFile, 1, MB_1, fp);
    fclose(fp);
    if (0 == size) {
        fprintf(stderr, "fread error!\n");
        exit(EXIT_FAILURE);
    }
    checkCapacities(fs, (size+KB_1*s->blockSize-1)/(KB_1*s->blockSize), 0);

    if (NULL == file) {
        checkCapacities(fs, 0, 1);
        uint16_t inodeNum = firstEmptyInode(fs);
        struct Inode *fInode = getInode(fs, inodeNum);
        memset(fInode, '\0', sizeof(struct Inode));
        fInode->linkCount = 1;
        fInode->type = T_FIL;
        fInode->size = 0;
        fInode->lastTime = (int32_t)time(NULL);
        for (uint32_t i = 0; 0 < size; ++i) {
            uint8_t *block = emptyFileBlock(fs, fInode);
            uint32_t bSize = s->blockSize * KB_1;
            if (bSize > size) {
                bSize = size;
            }
            memcpy(block, rawFile+fInode->size, bSize);
            fInode->size += bSize;
            size -= bSize;
        }

        file = firstEmptySlot(fs, dInode);
        strcpy(file->name, path);
        file->inode = inodeNum;
        dInode->size += sizeof(struct File);
        dInode->lastTime = (int32_t)time(NULL);
    } else {
        uint16_t inodeNum = file->inode;
        struct Inode *fInode = getInode(fs, inodeNum);
        fInode->lastTime = (int32_t)time(NULL);
        for (uint32_t i = 0; 0 < size; ++i) {
            uint8_t *block = emptyFileBlock(fs, fInode);
            uint32_t bSize = s->blockSize * KB_1;
            if (bSize > size) {
                bSize = size;
            }
            memcpy(block, rawFile+fInode->size, bSize);
            fInode->size += bSize;
            size -= bSize;
        }
        dInode->lastTime = (int32_t)time(NULL);
    }
}

void readCmd(uint8_t *fs, char *path, char *linuxFile)
{
    struct SuperBlock *s = (struct SuperBlock *)fs;
    struct Inode *inode = findPathInode(fs, path);
    while (T_SYM == inode->type) {
        inode = findPathInode(fs, (char *)fileBlock(fs, inode, 0));
    }
    if (T_FIL != inode->type) {
        fprintf(stderr, "%s is not a file!\n", path);
        exit(EXIT_FAILURE);
    }
    uint8_t rawFile[MB_1] = {0};
    uint8_t *ptr = rawFile;
    uint32_t size = inode->size;

    for (uint32_t i = 0; 0 < size; ++i) {
        uint8_t *block = fileBlock(fs, inode, i);
        uint32_t bSize = s->blockSize * KB_1;
        if (bSize > size) {
            bSize = size;
        }
        memcpy(ptr, block, bSize);
        size -= bSize;
        ptr += bSize;
    }

    FILE *fp = fopen(linuxFile, "wb+");
    if (NULL == fp) {
        fprintf(stderr, "fopen error!\n");
        exit(EXIT_FAILURE);
    }
    if (1 != fwrite(rawFile, inode->size, 1, fp)) {
        fprintf(stderr, "fwrite error!\n");
        exit(EXIT_FAILURE);
    }
    fclose(fp);
}

void delCmd(uint8_t *fs, char *path)
{
    char str[KB_1];
    findUpperPath(str, path);
    struct Inode *dInode = findPathInode(fs, str);
    path += strlen(str);
    if (strlen(str) != 1) {
        path++;
    }
    if (T_DIR != dInode->type) {
        fprintf(stderr, "%s is not a directory!\n", str);
        exit(EXIT_FAILURE);
    }
    struct File *file = (struct File *)getFile(fs, dInode, path);
    if (NULL == file) {
        fprintf(stderr, "%s is not found!\n", path);
        exit(EXIT_FAILURE);
    }
    struct Inode *fInode = getInode(fs, file->inode);
    if (T_FIL != fInode->type && T_SYM != fInode->type) {
        fprintf(stderr, "%s is not a file!\n", path);
        exit(EXIT_FAILURE);
    }
    struct Inode *delInode = getInode(fs, file->inode);
    if (0 == --(delInode->linkCount)) {
        freeFileBlocks(fs, file);
        getInode(fs, file->inode)->linkCount = 0;
    }
    struct File *full = lastFullSlot(fs, dInode);
    memcpy(file, full, sizeof(struct File));
    memset(full, '\0', sizeof(struct File));
    dInode->size -= sizeof(struct File);
    dInode->lastTime = (uint32_t)time(NULL);
}

void lnCmd(uint8_t *fs, char *filePath, char *linkPath)
{
    char str[KB_1];
    findUpperPath(str, linkPath);
    linkPath += strlen(str);
    if (strlen(str) != 1) {
        linkPath++;
    }
    struct Inode *fInode = findPathInode(fs, filePath);
    struct Inode *dInode = findPathInode(fs, str);
    if (T_DIR != dInode->type) {
        fprintf(stderr, "%s is not a directory!\n", str);
        exit(EXIT_FAILURE);
    }
    if (NULL != getFile(fs, dInode, linkPath)) {
        fprintf(stderr, "%s is already using!\n", linkPath);
        exit(EXIT_FAILURE);
    }
    if (255 <= fInode->linkCount) {
        fprintf(stderr, "%s has already maximum count of links!\n", filePath);
        exit(EXIT_FAILURE);
    }
    struct File *file = firstEmptySlot(fs, dInode);
    dInode->size += sizeof(struct File);
    dInode->lastTime = (int32_t)time(NULL);
    fInode->linkCount++;
    strcpy(file->name, linkPath);

    findUpperPath(str, filePath);
    filePath += strlen(str);
    if (strlen(str) != 1) {
        filePath++;
    }
    dInode = findPathInode(fs, str);
    file->inode = getFile(fs, dInode, filePath)->inode;
}

void lnsymCmd(uint8_t *fs, char *filePath, char *linkPath)
{
    char str[KB_1];
    findUpperPath(str, linkPath);
    linkPath += strlen(str);
    if (strlen(str) != 1) {
        linkPath++;
    }
    checkCapacities(fs, 1, 1);
    struct Inode *fInode, *dInode;
    fInode = findPathInode(fs, filePath);
    dInode = findPathInode(fs, str);
    struct File *file = firstEmptySlot(fs, dInode);
    dInode->size += sizeof(struct File);
    strcpy(file->name, linkPath);
    file->inode = firstEmptyInode(fs);
    fInode = getInode(fs, file->inode);
    memset(fInode, '\0', sizeof(struct Inode));
    char *block = (char *)emptyFileBlock(fs, fInode);
    strcpy(block, filePath);
    block[strlen(filePath)] = '\0';

    fInode->linkCount = 1;
    fInode->type = T_SYM;
    fInode->size = strlen(filePath)+1;
    fInode->lastTime = (int32_t)time(NULL);
}

void fsckCmd(uint8_t *fs)
{
    struct SuperBlock *s = (struct SuperBlock *)fs;
    uint8_t *blockUsed = calloc(s->blocksCount, sizeof(uint8_t));
    for (uint16_t i = 0; i < s->inodesCount; ++i) {
        struct Inode *inode = getInode(fs, i);
        uint32_t size = inode->size;
        for (uint32_t j = 0; 0 < size; ++j) {
            uint16_t res[4] = {0};
            uint16_t cnt = fileBlockNum(fs, inode, j, res);
            uint32_t bSize = s->blockSize * KB_1;
            if (bSize > size) {
                bSize = size;
            }
            size -= bSize;
            for (uint16_t k = 0; k < cnt; ++k) {
                blockUsed[res[k]]++;
            }
        }
    }
    for (uint16_t i = 0; i < s->blocksCount; ++i) {
        if (0 == i % 32) {
            if (0 == i) {
                printf("Blocks in use:");
            }
            printf("\n0x%03X ", i/32);
        }
        printf("%2u ", blockUsed[i]);
    }
    free(blockUsed);
    printf("\n");
    for (uint16_t i = 0; i < s->blocksCount; ++i) {
        if (0 == i % 32) {
            if (0 == i) {
                printf("Free blocks:");
            }
            printf("\n0x%03X ", i/32);
        }
        printf("%2u ", !GET_BIT(s->blocksBitmap, i));
    }
    printf("\n");

    struct Inode *root = getInode(fs, 0);
    uint8_t *visited = (uint8_t *)calloc((s->inodesCount+7)/8, sizeof(uint8_t));
    uint8_t *inodeUsed = (uint8_t *)calloc(s->inodesCount, sizeof(uint8_t));
    SET_BIT(visited, 0);
    inodeUsed[0] = 1;
    traverseDirs(fs, visited, inodeUsed, root);
    for (uint16_t i = 0; i < s->inodesCount; ++i) {
        if (0 == i % 32) {
            if (0 == i) {
                printf("Inodes in use:");
            }
            printf("\n0x%03X ", i/32);
        }
        printf("%2u ", inodeUsed[i]);
    }
    free(inodeUsed);
    free(visited);
    printf("\n");
    for (uint16_t i = 0; i < s->inodesCount; ++i) {
        if (0 == i % 32) {
            if (0 == i) {
                printf("Free inodes:");
            }
            printf("\n0x%03X ", i/32);
        }
        printf("%2u ", 0==getInode(fs, i)->linkCount);
    }
    printf("\n");
}

void traverseDirs(uint8_t *fs, uint8_t *vMap, uint8_t *counts, struct Inode *inode)
{
    if (T_DIR != inode->type) {
        return;
    }
    struct File *file = NULL;
    uint64_t num = inode->size / sizeof(struct File);
    for (uint64_t i = 2; i < num; ++i) {
        file = getFileInDir(fs, inode, i);
        counts[file->inode]++;
        if (!GET_BIT(vMap, file->inode)) {
            SET_BIT(vMap, file->inode);
            traverseDirs(fs, vMap, counts, getInode(fs, file->inode));
        }
    }
}

void printInfos(uint8_t *fs, uint16_t *counts, uint8_t *vMap, struct Inode *inode)
{
    counts[inode->type]++;
    struct File *file = NULL;
    if (T_DIR != inode->type) {
        return;
    }
    uint64_t num = inode->size / sizeof(struct File);
    for (uint64_t i = 2; i < num; ++i) {
        file = getFileInDir(fs, inode, i);
        if (!GET_BIT(vMap, file->inode)) {
            SET_BIT(vMap, file->inode);
            printInode(fs, file->inode);
            printInfos(fs, counts, vMap, getInode(fs, file->inode));
        }
    }
}

void printInode(uint8_t *fs, uint16_t num)
{
    printf("--- --- --- --- ---\n");
    printf("Inode:\t%4u %3s\n", num, T2S(getInode(fs, num)->type));
    struct SuperBlock *s = (struct SuperBlock *)fs;
    printf("Names:\t");
    for (uint16_t i = 0; i < s->inodesCount; ++i) {
        struct Inode *inode = getInode(fs, i);
        if (0 == inode->linkCount || T_DIR != inode->type) {
            continue;
        }
        uint64_t count = inode->size / sizeof(struct File);
        for (uint64_t j = 2; j < count; ++j) {
            struct File *file = getFileInDir(fs, inode, j);
            if (file->inode == num) {
                printf("%s, ", file->name);
            }
        }
    }
    printf("\nBlocks:\t");
    struct Inode *inode = getInode(fs, num);
    uint64_t blockCount = (inode->size + (s->blockSize*KB_1) - 1) / (s->blockSize * KB_1);
    for (uint64_t i = 0; i < blockCount; ++i) {
        uint16_t res[4] = {0};
        uint16_t cnt = fileBlockNum(fs, inode, i, res);
        for (uint16_t j = 0; j < cnt; ++j) {
            printf("%u, ", res[j]);
        }
    }
    printf("\n");
}