//
// Created by artem on 29.11.15.
//
#include <linux/msdos_fs.h>
#include <unistd.h>
#include <stdint.h>
#include "fat32_structures.h"
#include <list>
#include <sys/syslog.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <iostream>
#include <string>
#ifndef FAT32READER_FAT_H
#define FAT32READER_FAT_H
class FSState {
public:
    ssize_t mmap_size;
    char *currPath;
    DirectoryEntry virtualRootDir;
    DirectoryEntry *currDir;
    BootRecord *bR;
    uint32_t* FAT;
    char *fs_mmap;
    ssize_t cluster_size;
};

class DirectoryIterator{
public:
    DirectoryIterator(FSState *fsState, DirectoryEntry *dir);

    FSState * fsState;
    ssize_t clusterNumber;
    DirectoryEntry *currentDirectory;
    DirectoryEntry *directory;
    DirectoryEntry *firstDirecrotyInCluster;

    DirectoryEntry *getNextDir();
};



class FAT32Reader {
    DirectoryEntry *getFileWithNameInDirectory(DirectoryEntry *dir, char *name);
    FSState *fsState;
public:
    FSState *getFsState() const {
        return fsState;
    }
    DirectoryEntry *getPtrToDirectory(char *path, DirectoryEntry *directory);
    void initFSState(char *fs_mmap, ssize_t mmap_size, char *path, BootRecord *bR);
};


class Command {
    FAT32Reader *fat32Reader;
    struct stat sb;
    char *mmap_start;
    int fd;
public:
    int initFAT32Reader(char *filename);
    Command();
    ~Command();
    int performCommand(char*line);
};




#endif //FAT32READER_FAT_H
