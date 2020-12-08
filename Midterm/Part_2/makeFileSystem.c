#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>

#include "filesystem.h"

int main(int argc, char *argv[])
{
    if (4 != argc) {
        fprintf(stderr, "Invalid usage!\n");
        return EXIT_FAILURE;
    }
    uint16_t blockSize = atoi(argv[1]);
    uint16_t inodesCount = atoi(argv[2]) + 1;
    char *fileName = argv[3];
    if (MB_1 <= KB_1 * (blockSize + 1) + inodesCount * sizeof(struct Inode)) {
        fprintf(stderr, "1 MB space is not enough for these numbers!\n");
        return EXIT_FAILURE;
    }

    size_t leftBytes = MB_1;
    leftBytes -= sizeof(struct SuperBlock);
    leftBytes -= inodesCount * sizeof(struct Inode);

    uint8_t filesystem[MB_1] = {0};
    struct SuperBlock *s = (struct SuperBlock *)filesystem;
    s->blockSize = blockSize;
    s->blocksCount = leftBytes / (blockSize * KB_1);
    s->inodesCount = inodesCount;
    s->filesCount = blockSize * (KB_1 / sizeof(struct File));
    s->firstBlockAddr = MB_1 - (s->blocksCount * s->blockSize * KB_1);
    s->firstInodeAddr = s->firstBlockAddr - (inodesCount * sizeof(struct Inode));
    SET_BIT(s->blocksBitmap, 0);

    struct Inode *r = (struct Inode *)(filesystem + s->firstInodeAddr);
    r->linkCount = 1;
    r->type = T_DIR;
    r->size = 2 * sizeof(struct File);
    r->lastTime = (int32_t)time(NULL);
    r->direct[0] = 0;
    struct File *f = (struct File *)(filesystem + s->firstBlockAddr);
    f->inode = 0;
    strcpy(f->name, ".");
    f++;
    f->inode = 0;
    strcpy(f->name, "..");

    FILE *file;
    file = fopen(fileName, "wb+");
    if (NULL == file) {
        perror("fopen");
    }
    if (1 != fwrite(filesystem, MB_1, 1, file)) {
        fprintf(stderr, "fwrite error!\n");
        return EXIT_FAILURE;
    }
    fclose(file);
    return EXIT_SUCCESS;
}