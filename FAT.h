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
#include <string.h>
#ifndef FAT32READER_FAT_H
#define FAT32READER_FAT_H

class FSState {
    ssize_t mmap_size;
    char *currPath;
    DirectoryEntry current_dir;
    BootRecord *bR;
    uint32_t* FAT;
    char *fs_mmap;
    ssize_t cluster_size;
public:
    FSState(ssize_t mmap_size, char *currPath,  DirectoryEntry &current_dir, BootRecord *bR, uint32_t *FAT,
            char *fs_mmap, ssize_t cluster_size);
    FSState(ssize_t i, char *string, BootRecord *ptr, char *string1);
    DirectoryEntry &getCurrent_dir()  {
        return current_dir;
    }
    BootRecord *getBR()  {
        return bR;
    }
    uint32_t *getFAT()  {
        return FAT;
    }
    char *getFs_mmap()  {
        return fs_mmap;
    }
    ssize_t getCluster_size()  {
        return cluster_size;
    }
};

class DirectoryIterator{
    FSState * fsState;
    ssize_t clusterNumber;
    DirectoryEntry *currentDirectory;
public:
    DirectoryIterator(FSState *fsState, DirectoryEntry *dir);
    DirectoryEntry *getNextDir();
    ssize_t getNextCluster(FSState *fsState, uint32_t cluster_number);
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
    bool isItFile(DirectoryEntry* dir) { return false;}
    int compareFileAndDirecrtoryName(DirectoryEntry *dir, char *name);
    char *getFileName(DirectoryEntry *dir);
    char *readFile(DirectoryEntry *fsState);
    DirectoryEntry *getInnerDirectories(FSState *fsState, DirectoryEntry *directories);
    void getFirstLevelFile(char **path, char *first_level_file);
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
