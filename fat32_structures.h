#ifndef __FAT32_STRUCTURES__
#define __FAT32_STRUCTURES__
#include <stdio.h>
#include <stdint.h>

/*
 * Offset for first directory entry:
 * first MasterBootRecord->PartitionEntry->offset_to_the_first_sector it is BootRecord
 * in sectors ( 512 bytes)
 * cluster_begin_lba = . + reserved_sectors + ( number_of_copies_of_fat*number_of_sectors_per_fat)
 * BootRecord:  cluster_begin_lba + ( cluster_number_of_the_root_directory - 2)*sectors_per_cluster=root_directory
 * end of the directory - first byte is zero
 * ubused directory record starts with 0xe5
 */


/*
 * This structure is the part of MasterBootRecord
 * It specifies partition
 */
typedef struct __attribute__((__packed__)) {
    uint8_t current_state_of_partition;
    uint8_t beginning_of_partition_head;
    uint16_t beginning_of_partition_cylinder;
    uint8_t type_of_partition;
    uint8_t end_of_partition_head;
    uint16_t end_of_partition_cylinder;
    //this is te offset to the very first sector of the partition
    //
    uint32_t offset_to_the_first_sector;
    uint32_t number_of_sectors;
}PartitionEntry; // mbr

/*
 * First record in the fat fs
 * This record first of all specifies partitions
 * offsets
 */
typedef struct __attribute__((__packed__)) {
    char executabla_code[446];
    PartitionEntry partition_entry_1[16];
    PartitionEntry partition_entry_2[16];
    PartitionEntry partition_entry_3[16];
    PartitionEntry partition_entry_4[16];
    uint16_t signature; // 55hAAh
}MasterBootRecord;

typedef struct __attribute__((__packed__)) {
    char jump_code[3];
    char oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t number_of_copies_of_fat;
    uint16_t max_root_dir_entries;
    uint16_t number_of_sectors_in_partition_smaller_then_3mb;
    uint8_t media_descriptor;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t number_of_heads;
    uint32_t number_of_hidden_sectors;
    uint32_t number_of_sectors;
    uint32_t number_of_sectors_per_fat;
   /*28h*/ uint16_t flags;
    uint16_t version_of_fat32;
    uint32_t cluster_number_of_the_root_directory; //root cluster
    uint16_t sector_number_of_fs_information_sector;
    uint16_t sector_number_of_the_backup_boot_sector;
    char reserved[12];
    uint8_t logical_drive_number_of_partition;
    uint8_t unused_2;
    uint8_t extended_signature; // 29h
    uint32_t serial_number_of_partition;
    char voluma_name_of_partition[11];
    char fat_name[8];
    char executable_code[420];
    uint16_t signature_2; // 55hAAh
}BootRecord;

typedef struct __attribute__((__packed__)) {
    uint8_t first_signature; // 52h 52h 61h 41h
    char unknown[480]; 
    uint32_t signature; // 72h 72h 41h 61h
    uint32_t number_of_free_clusters;
    uint32_t recently_allocated_cluster_number;
    char reserver[12];
    char unknown_2[2];
    uint16_t signature_2; // 55hAAh
}FSInfoSector;

typedef struct __attribute__((__packed__)) {
    char fname[8];
    char fname_extension[3];
    uint8_t glags;
    char unused[8];
    uint16_t starting_cluster_hw;
    uint16_t time;
    uint16_t date;
    uint16_t starting_cluster_lw;
    uint32_t file_size;
}DirectoryEntry;

#endif //__FAT32_STRUCTURES__