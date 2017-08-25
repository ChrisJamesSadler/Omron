#ifndef __FS_H__
#define __FS_H__

#include <common.h>
#include <hal.h>

typedef struct partition_t
{
    uint8_t bootable;
    uint8_t start_head;
    uint8_t start_sector : 6;
    uint16_t start_cylinder : 10;
    uint8_t partition_id;
    uint8_t end_head;
    uint8_t end_sector : 6;
    uint16_t end_cylinder : 10;
    uint32_t start_lba;
    uint32_t length;
}__attribute__((packed))partition_t;

typedef struct mbr_t
{
    uint8_t bootloader[440];
    uint32_t signature;
    uint16_t unused;
    partition_t partitions[4];
    uint16_t magic;
}__attribute__((packed))mbr_t;

typedef struct bios_param_block_t
{
    uint8_t jump[3];
    uint8_t soft_name[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fat_copies;
    uint16_t fat_dir_entries;
    uint16_t total_sectors;
    uint8_t media_type;
    uint16_t fat_sector_count;
    uint16_t sectors_per_track;
    uint16_t head_count;
    uint32_t hidden_sectors;
    uint32_t total_sector_count;

    uint32_t table_size;
    uint16_t ext_flags;
    uint16_t fat_version;
    uint32_t root_cluster;
    uint16_t fat_into;
    uint16_t backup_sector;
    uint8_t reserved0[12];
    uint8_t drive_number;
    uint8_t reserved;
    uint8_t boot_signature;
    uint32_t volume_id;
    uint8_t volume_label[11];
    uint8_t fat_type_label[8];
}__attribute__((packed))bios_param_block_t;

typedef struct fat_directory_entry
{
    uint8_t name[8];
    uint8_t ext[3];
    uint8_t attributes;
    uint8_t reserved;
    uint8_t c_time_tenth;
    uint16_t c_time;
    uint16_t c_date;
    uint16_t a_time;
    uint16_t first_cluster_hi;
    uint16_t w_time;
    uint16_t w_date;
    uint16_t first_cluster_low;
    uint32_t size;
}__attribute__((packed))fat_directory_entry;

typedef struct vfs_directory_t
{
    char* name;
    struct vfs_directory_t* parent;
    uint32_t lba;
    list_t* dirs;
    list_t* files;
    struct vfs_tree_t* mount_info;
}vfs_directory_t;

typedef struct vfs_file_t
{
    char* name;
    struct vfs_directory_t* parent;
    uint32_t lba;
    uint32_t size;
}vfs_file_t;

typedef struct vfs_tree_t
{
    vfs_directory_t* root;
    device_t* host;
    uint32_t fatStart;
    uint32_t fatSize;
    uint32_t dataStart;
    uint32_t rootStart;
    uint32_t sectors_per_cluster;
}vfs_tree_t;

typedef struct file_t
{
    char* name;
    vfs_directory_t* location;
    uint32_t lba;
    int32_t size;
    int32_t progress;
}file_t;

typedef struct directory_t
{
    char* name;
    list_t* files;
}directory_t;

vfs_directory_t* vfs_root_node;
extern void fs_init();
extern file_t* fopen(char* name, ...);
extern uint32_t fread(void* buffer, int32_t count, file_t* stream);
extern void fwrite(uint8_t* buffer, int32_t count, file_t* stream);
extern void fclose(file_t* stream);
extern void fdirs(char* cdir);
extern uint32_t fexists(char* name);
extern int32_t fseek(file_t* stream, int32_t offset);
extern void flist(vfs_directory_t* dir);


#endif