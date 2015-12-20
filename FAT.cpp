#include <malloc.h>
#include <string.h>
#include "FAT.h"
#define FILE_PATH_MAX_LEN 256
#define FILE_NAME_MAX_LEN 8
#define FILE_EXTENSIO_MAX_LEN 3
#define DEFAULT_OFFSET_TO_BOOT_RECORD 0//32256


int startsWith(const char *pre, const char *str) {
    size_t lenpre = strlen(pre),
            lenstr = strlen(str);
    return lenstr < lenpre ? 0 : strncmp(pre, str, lenpre) == 0;
}

void FAT32Reader::initFSState(char *fs_mmap, ssize_t mmap_size, char *path, BootRecord *bR){
    fsState = new FSState(mmap_size, path, bR, fs_mmap);

}

DirectoryEntry * FAT32Reader::getPtrToDirectory(char *path, DirectoryEntry*directory) {
    if (path[0]=='/' ) {
        while (path[0] == '/') {
            path++;
        }
        directory = &fsState->getCurrent_dir();
        if ( strlen(path)==0){
            return directory;
        }
    }
    if (directory == NULL){
        directory = &fsState->getCurrent_dir();
        if( strlen(path)==0){
            return directory;
        }
    }
    char first_level_file[FILE_PATH_MAX_LEN];
    bzero(first_level_file, FILE_PATH_MAX_LEN);
    getFirstLevelFile(&path, first_level_file);

    DirectoryEntry* next_dir = getFileWithNameInDirectory(directory, first_level_file);
    if (  next_dir!= NULL && next_dir->starting_cluster_lw == 0 &&  next_dir->starting_cluster_hw == 0)
        next_dir = &fsState->getCurrent_dir();
    if ( next_dir!= NULL){ // directory ( file) was not found
        if ( strlen(path) == 0){ // if it is last part of the path.
            return next_dir;
        }
        return getPtrToDirectory(path, next_dir );//if not
    }
    return NULL;
}
void FAT32Reader::getFirstLevelFile(char **path, char*first_level_file) {
    int i = 0;
    while( (*path)[0] !='/' && (*path)[0] != 0 && i < FILE_PATH_MAX_LEN){
        first_level_file[i] = (*path)[0];
        (*path)++;
        i++;
    }
    while ((*path)[0]=='/'){
        (*path)++;
    }
}

char *getPtrToFile( FSState* fsState, uint32_t cluster_number) {
    BootRecord *bR = fsState->getBR();
    return ((char*)bR) +
           (bR->reserved_sectors +
            bR->number_of_copies_of_fat * bR->number_of_sectors_per_fat +
                   //TODO
            (cluster_number - 2) * bR->sectors_per_cluster
           ) * bR->bytes_per_sector;
}

DirectoryEntry *FAT32Reader::getInnerDirectories(FSState* fsState, DirectoryEntry *directories) {
    uint32_t next_dir_cluster_number = directories->starting_cluster_hw;
    next_dir_cluster_number <<= 16;
    next_dir_cluster_number += directories->starting_cluster_lw;
    return (DirectoryEntry*) getPtrToFile(fsState, next_dir_cluster_number);
}

ssize_t DirectoryIterator::getNextCluster( FSState* fsState, uint32_t cluster_number){
    ssize_t next_cluster_number = fsState->getFAT()[cluster_number];
    return next_cluster_number;
}

DirectoryIterator::DirectoryIterator(FSState* fsState, DirectoryEntry* dir){
    this->fsState = fsState;
    this->clusterNumber =(dir->starting_cluster_hw << 16) + dir->starting_cluster_lw;
    this->currentDirectory = ( DirectoryEntry* )getPtrToFile(fsState, this->clusterNumber );
}

void destroyDirectoryIterator(DirectoryIterator* dirIter){
    if ( dirIter != NULL){
        free( dirIter);
    }
}

DirectoryEntry* DirectoryIterator::getNextDir(){
    if ( this==NULL || this->currentDirectory==NULL){
        return NULL;
    }
    while( 1 ){
        this->currentDirectory++;
        if ( ((char*)this->currentDirectory)[0] == 0){
            this->currentDirectory = NULL;
            return NULL;
        }
        if ( this->currentDirectory->glags==0x0F){
            continue;
        }
        if ( ((uint8_t*) this->currentDirectory)[0] !=0xE5){
            return this->currentDirectory;
        }
    }
    return NULL;
}

char* FAT32Reader::getFileName( DirectoryEntry* dir){
    int i;
    char fname[FILE_NAME_MAX_LEN+1];
    char fname_extension[FILE_EXTENSIO_MAX_LEN+1];
    bzero(fname, FILE_NAME_MAX_LEN+1);
    bzero(fname_extension, FILE_EXTENSIO_MAX_LEN+1);
    memcpy(fname, dir->fname, FILE_NAME_MAX_LEN);
    memcpy(fname_extension, dir->fname_extension, FILE_EXTENSIO_MAX_LEN);
    for( i =0 ; fname[i]!=' ' && i <= FILE_NAME_MAX_LEN; i++);
    fname[i]=0;
    for( i =0 ; fname_extension[i]!=' ' && i <= FILE_EXTENSIO_MAX_LEN; i++);
    fname_extension[i]=0;
    char* file_name = (char*) malloc(FILE_NAME_MAX_LEN+FILE_EXTENSIO_MAX_LEN+2);
    if ( strlen(fname_extension)==0 ){
        sprintf(file_name, "%s", fname);
    }else{
        sprintf(file_name, "%s.%s", fname, fname_extension);
    }
    return file_name;
}

int FAT32Reader::compareFileAndDirecrtoryName(DirectoryEntry *dir, char *name){
    char* file_name = getFileName( dir);
    int result = strcmp( file_name, name);
    free( file_name);
    return result;
}

DirectoryEntry *FAT32Reader::getFileWithNameInDirectory(DirectoryEntry* dir, char *name) {
    DirectoryIterator *dirIter = new DirectoryIterator(fsState, dir);

    DirectoryEntry *nextDir = NULL;
    while ((nextDir = dirIter->getNextDir()) != NULL){
        if (compareFileAndDirecrtoryName(nextDir, name) == 0) {
            break;
        }
    }
    destroyDirectoryIterator(dirIter);
    return nextDir;
}

char * FAT32Reader::readFile(FSState* fsState, DirectoryEntry *dir) {
    ssize_t size_to_copy = dir->file_size;
    uint32_t cluster_number = (dir->starting_cluster_hw << 16) + dir->starting_cluster_lw;
    char *data = (char*) malloc(size_to_copy);
    char* file_block_ptr = (char*)getInnerDirectories( fsState, dir);
    while (size_to_copy > 0){
        ssize_t offset = data - fsState->getFs_mmap();
        ssize_t memory_to_copy = size_to_copy > fsState->getCluster_size() ? fsState->getCluster_size() : size_to_copy;
        memcpy( data + dir->file_size - size_to_copy, file_block_ptr, memory_to_copy);
        size_to_copy -= memory_to_copy;
        cluster_number = fsState->getFAT()[cluster_number];
        file_block_ptr = (char*)getPtrToFile(fsState, cluster_number);
    }
    return data;
}

int Command::performCommand(char *line) {
    if (startsWith("ls", line)) {
        line += 2;
        while (line[0] == ' ') line++;
        line[ strlen(line) == 0? 0 : strlen(line) -1  ] = 0;
        DirectoryEntry *dir = fat32Reader->getPtrToDirectory(line , NULL);
        if (fat32Reader->isItFile(dir)) {
            printf("is not directory\n");
            return 0;
        }
        if (dir != NULL) {
            DirectoryIterator *dirIter = new DirectoryIterator(fat32Reader->getFsState(), dir);
            DirectoryEntry *nextDir = NULL;
            while ((nextDir = dirIter->getNextDir()) != NULL){
                char* fname = fat32Reader->getFileName(nextDir);
                int day_of_month = (nextDir->date & 0x1f);
                int month = ((nextDir->date >> 5) & 0x0f);
                int year = (nextDir->date >> 9) & 0x7f;
                int seconds = (nextDir->time & 0x1f) * 2;
                int minutes = (nextDir->time >> 5) & 0x3f;
                int hours = (nextDir->time >> 11) & 0x1f;
                char date[20];
                sprintf(date, "%d.%02d.%02d %02d:%02d:%02d",year+1980, month, day_of_month, hours,minutes,seconds );
                printf("%-15s%-10u%-23s%-5o\n" ,fname ,nextDir->file_size, date, nextDir->glags);
                free(fname);
            }
            destroyDirectoryIterator(dirIter);
        } else {
            printf("cannot find dir\n");
        }
    }
    if (startsWith("cat ", line)) {
        char* data;
        line += 3;
        while (line[0] == ' ') line++;
        line[ strlen(line) == 0 ? 0 : strlen(line) -1  ] = 0;
        DirectoryEntry *dir = fat32Reader->getPtrToDirectory(line , NULL);
        if (dir != NULL) {
            data = fat32Reader->readFile(fat32Reader->getFsState(),dir);
            ssize_t  i = 0;
            for( ; i < dir->file_size; i++){
                //if ( data[i] <= 'Z' && data[i]>= '0') {// need filter
                printf("%c", data[i]);
                fflush(stdout); // to avoid some problems
                //}
            }
        } else {
            printf("Can not find this directory");
        }

    }
    return 0;
}

Command::Command() {
    fat32Reader = new FAT32Reader();
}

int Command::initFAT32Reader(char *filename) {

    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        return  -1;
    }
    fstat(fd, &sb);
    if (sb.st_size == 0) {
        return  -1;
    }
    mmap_start = (char *) mmap(NULL, sb.st_size, PROT_READ,
                                MAP_SHARED, fd, 0);
    if (mmap_start == NULL) {
        perror("mmap:");
        return -1;
    }
    char *boot_record_start_byte = DEFAULT_OFFSET_TO_BOOT_RECORD + mmap_start;
    BootRecord *bR = (BootRecord *) boot_record_start_byte;
    fat32Reader->initFSState(mmap_start, sb.st_size, "/", bR);
    return 0;
}

Command::~Command() {
    munmap(mmap_start, sb.st_size);
    delete(fat32Reader);
    close(fd);
}

FSState::FSState(ssize_t mmap_size, char *path, BootRecord *bR, char *fs_mmap) {
    this->bR = bR;
    this->currPath = path;
    this->mmap_size = mmap_size;
    this->fs_mmap = fs_mmap;
    this->FAT = (uint32_t*) ( (char*) bR + bR->reserved_sectors* bR->bytes_per_sector);
    this->cluster_size = bR->bytes_per_sector * bR->sectors_per_cluster;
    this->current_dir.starting_cluster_hw = bR->cluster_number_of_the_root_directory>>16;
    this->current_dir.starting_cluster_lw = bR->cluster_number_of_the_root_directory & 0xFFFF;
}
