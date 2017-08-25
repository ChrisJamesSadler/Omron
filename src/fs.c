#include <common.h>
#include <fs.h>
#include <hal.h>
#include <ata.h>

void cache_search_directory(device_t* dev, uint32_t lba, uint32_t dataStart, uint32_t sectors_per_cluster, vfs_directory_t* dir)
{
    fat_directory_entry dirent[16];
    ata_read((uint8_t*)&dirent[0], lba, 1, dev);
    for(uint32_t i = 0; i < 16; i++)
    {
        if(dirent[i].name[0] == 0)
        {
            break;
        }
        if(dirent[i].name[0] == '.')
        {
            continue;
        }
        if(strlen((char*)dirent[i].ext) == 0)
        {
            continue;
        }
        char* foo = "        \0";
        for(uint32_t j = 0; j < 8; j++)
        {
            foo[j] = dirent[i].name[j];
        }
        strtrim(foo, ' ');
        uint32_t start = ((uint32_t)dirent[i].first_cluster_hi) << 16 | ((uint32_t)dirent[i].first_cluster_low);
        if((dirent[i].attributes & 0x10) == 0x10)
        {
            start = dataStart + sectors_per_cluster * (start - 2);
            vfs_directory_t* a = malloc(sizeof(vfs_directory_t));
            a->dirs = makelist(8);
            a->files = makelist(8);
            a->name = malloc(strlen(foo) + 1);
            memcpy(a->name, foo, strlen(foo));
            a->parent = dir;
            a->lba = start;
            listadd(dir->dirs, (uint32_t)a);
            cache_search_directory(dev, start, dataStart, sectors_per_cluster, a);
        }
        else
        {
            vfs_file_t* a = malloc(sizeof(vfs_file_t));
            a->name = malloc(strlen(foo) + 1);
            memcpy(a->name, foo, strlen(foo));
            a->parent = dir;
            a->lba = start;
            a->size = dirent[i].size;
            listadd(dir->files, (uint32_t)a);
        }
    }
}

char mps = 'a';
void fs_init()
{
    vfs_root_node = malloc(sizeof(vfs_directory_t));
    vfs_root_node->name = "/";
    vfs_root_node->dirs = makelist(10);
    vfs_root_node->files = makelist(10);
    vfs_directory_t* mnt = malloc(sizeof(vfs_directory_t));
    listadd(vfs_root_node->dirs, (uint32_t)mnt);
    mnt->name = "mnt";
    mnt->parent = vfs_root_node;
    mnt->dirs = makelist(10);
    mnt->files = makelist(10);
    for(int32_t i = 0; i < listlength(devices); i++)
    {
        device_t* dev;
        peekitem(devices, i, (uint32_t*)&dev);
        if(dev->id == DEVICE_ID_ATA)
        {
            mbr_t mbr;
            ata_read((uint8_t*)&mbr, 0, 1, dev);
            if(mbr.magic != 0xAA55)
            {
                return;
            }
            for(uint32_t i = 0; i < 4; i++)
            {
                if(mbr.partitions[i].partition_id == 0)
                {
                    continue;
                }
                bios_param_block_t bpb;
                ata_read((uint8_t*)&bpb, mbr.partitions[i].start_lba, 1, dev);
                vfs_directory_t* mountpoint = malloc(sizeof(vfs_directory_t));
                mountpoint->parent = mnt;
                mountpoint->name = malloc(4);
                strcpy(mountpoint->name, "hd");
                mountpoint->name[2] = mps;
                mountpoint->dirs = makelist(8);
                mountpoint->files = makelist(8);
                mountpoint->mount_info = malloc(sizeof(vfs_tree_t));
                mountpoint->mount_info->fatStart = mbr.partitions[i].start_lba + bpb.reserved_sectors;
                mountpoint->mount_info->fatSize = bpb.table_size;
                mountpoint->mount_info->dataStart = mountpoint->mount_info->fatStart + mountpoint->mount_info->fatSize*bpb.fat_copies;
                mountpoint->mount_info->rootStart = mountpoint->mount_info->dataStart + bpb.sectors_per_cluster * (bpb.root_cluster - 2);
                mountpoint->mount_info->sectors_per_cluster = bpb.sectors_per_cluster;
                mountpoint->mount_info->host = dev;
                listadd(mnt->dirs, (uint32_t)mountpoint);
                cache_search_directory(dev, mountpoint->mount_info->rootStart, mountpoint->mount_info->dataStart, mountpoint->mount_info->sectors_per_cluster, mountpoint);
                mps++;
            }
        }
    }
}

vfs_tree_t* find_mount_info(vfs_directory_t* start)
{
    vfs_tree_t* ptr;
    do
    {
        if(start->mount_info != null)
        {
            ptr = start->mount_info;
            break;
        }
        start = start->parent;
    }
    while(start != null);
    return ptr;
}

file_t* fopen(char* name, ...)
{
    list_t* parts = strsplit(name, '/');
    char* currenttarget = null;
    vfs_directory_t* currentdir = vfs_root_node;
    vfs_directory_t* tmp;
    searchnext:;
    if(listlength(parts) > 1)
    {
        popfirst(parts, (uint32_t*)&currenttarget);
        for(int32_t i = 0; i < listlength(currentdir->dirs); i++)
        {
            peekitem(currentdir->dirs, i, (uint32_t*)&tmp);
            if(strcmp(tmp->name, currenttarget))
            {
                currentdir = tmp;
                free(currenttarget);
                goto searchnext;
            }
        }
    }
    file_t* found = null;
    free(currenttarget);
    popfirst(parts, (uint32_t*)&currenttarget);
    for(int i = 0 ; i < listlength(currentdir->files); i++)
    {
        vfs_file_t* file;
        peekitem(currentdir->files, i, (uint32_t*)&file);
        if(strcmp(currenttarget, file->name))
        {
            found = malloc(sizeof(file_t));
            found->name = file->name;
            found->size = file->size;
            found->location = currentdir;
            found->lba = file->lba;
        }
    }
    free(currenttarget);
    while(listlength(parts) > 0)
    {
        popfirst(parts, (uint32_t*)&currenttarget);
        free(currenttarget);
    }
    return found;
}

uint32_t fread(void* buffer, int32_t count, file_t* stream)
{
    if(!stream)
    {
        return 0;
    }
    memset(buffer, 0, count);
    if(stream->progress + count >= stream->size)
    {
        count = stream->size - stream->progress;
    }
    vfs_tree_t* mp = find_mount_info(stream->location);
    if(mp == null)
    {
        return 0;
    }
    uint32_t readBytes = 0;
    uint32_t firstFileCluster = stream->lba;
    int32_t SIZE = stream->size;
    uint32_t nextFileCluster = firstFileCluster;
    uint8_t* buf = malloc(513);
    uint8_t* fatbuf = malloc(513);
    int32_t skipblock = 0;
    if(stream->progress != 0)
    {
        skipblock = -1;
        for(int32_t i = stream->progress; i > 0; i -= 512)
        {
            skipblock++;
        }
    }
    if(stream->progress >= SIZE)
    {
        SIZE = 0;
    }
    while(SIZE > 0)
    {
        uint32_t fileSector = mp->dataStart + mp->sectors_per_cluster * (nextFileCluster - 2);
        uint32_t sectorOffset = 0;
        for(; SIZE > 0; SIZE -= 512)
        {
            if(skipblock <= 0)
            {
                uint32_t toread = count;
                if(stream->progress == 0)
                {
                    if(count > 512)
                    {
                        toread = 512;
                    }
                }
                else
                {
                    if((512 % stream->progress) + count > 512)
                    {
                        toread = ((512 % stream->progress) + count) - 512;
                    }
                }
                count -= toread;
                readBytes += toread;
                uint32_t offset = stream->progress;
                while(offset > 512)
                {
                    offset -= 512;
                }
                ata_read(buf, fileSector + sectorOffset, 1, mp->host);
                memcpy(buffer, buf + offset, toread);
                buffer += toread;
                stream->progress += toread;
            }
            if(count <= 0)
            {
                SIZE = 0;
                break;
            }
            skipblock--;
            if(++sectorOffset > mp->sectors_per_cluster)
            {
                break;
            }
        }
        uint32_t fatSectorForCurrentCluster = nextFileCluster / (512 / sizeof(uint32_t));
        ata_read(fatbuf, mp->fatStart + fatSectorForCurrentCluster, 1, mp->host);
        uint32_t fatOffsetInSectorForCurrentCluster = nextFileCluster % (512 / sizeof(uint32_t));
        nextFileCluster = ((uint32_t*)&fatbuf)[fatOffsetInSectorForCurrentCluster] & 0x0FFFFFFF;
    }
    free(buf);
    free(fatbuf);
    return readBytes;
}

/*void fwrite(file_t* stream, uint8_t* buffer, int32_t count)
{

}*/

void fclose(file_t* stream)
{
    free(stream);
}

void fdirs(char* cdir)
{
    list_t* parts = strsplit(cdir, '/');
    vfs_directory_t* currentdir = vfs_root_node;
    char* currenttarget = null;
    vfs_directory_t* tmp;
    searchNextDir:;
    if(listlength(parts) > 0)
    {
        popfirst(parts, (uint32_t*)&currenttarget);
        for(int32_t i = 0; i < listlength(currentdir->dirs); i++)
        {
            peekitem(currentdir->dirs, i, (uint32_t*)&tmp);
            if(strcmp(tmp->name, currenttarget))
            {
                currentdir = tmp;
                free(currenttarget);
                goto searchNextDir;
            }
        }
        free(currenttarget);
        while(listlength(parts) > 0)
        {
            popfirst(parts, (uint32_t*)&currenttarget);
            free(currenttarget);
        }
        deletelist(parts);
        return;
    }
    popfirst(parts, (uint32_t*)&currenttarget);
    for(int i = 0 ; i < listlength(currentdir->dirs); i++)
    {
        vfs_directory_t* dir;
        peekitem(currentdir->dirs, i, (uint32_t*)&dir);
        printf("%s\n", dir->name);
    }
    for(int i = 0 ; i < listlength(currentdir->files); i++)
    {
        vfs_file_t* file;
        peekitem(currentdir->files, i, (uint32_t*)&file);
        printf("%s\n", file->name);
    }
}

uint32_t fexists(char* name)
{
    list_t* parts = strsplit(name, '/');
    char* currenttarget = null;
    vfs_directory_t* currentdir = vfs_root_node;
    vfs_directory_t* tmp;
    searchnext:;
    if(listlength(parts) > 1)
    {
        popfirst(parts, (uint32_t*)&currenttarget);
        for(int32_t i = 0; i < listlength(currentdir->dirs); i++)
        {
            peekitem(currentdir->dirs, i, (uint32_t*)&tmp);
            if(strcmp(tmp->name, currenttarget))
            {
                currentdir = tmp;
                free(currenttarget);
                goto searchnext;
            }
        }
    }
    uint32_t found = 0;
    free(currenttarget);
    popfirst(parts, (uint32_t*)&currenttarget);
    for(int i = 0 ; i < listlength(currentdir->dirs); i++)
    {
        vfs_directory_t* dir;
        peekitem(currentdir->dirs, i, (uint32_t*)&dir);
        if(strcmp(currenttarget, dir->name))
        {
            found = 1;
        }
    }
    for(int i = 0 ; i < listlength(currentdir->files); i++)
    {
        vfs_file_t* file;
        peekitem(currentdir->files, i, (uint32_t*)&file);
        if(strcmp(currenttarget, file->name))
        {
            found = 2;
        }
    }
    free(currenttarget);
    while(listlength(parts) > 0)
    {
        popfirst(parts, (uint32_t*)&currenttarget);
        free(currenttarget);
    }
    return found;
}

int32_t fseek(file_t* stream, int32_t offset)
{
    if(!stream)
    {
        return -1;
    }
    if(offset > stream->progress)
    {
        offset = stream->progress;
    }
    if(offset < 0)
    {
        offset = 0;
    }
    stream->progress = offset;
    return stream->progress;
}

void print_tree(vfs_directory_t* dir, uint32_t depth)
{
    if(dir->parent == null)
    {
        printf("%s\n", dir->name);
    }
    depth += 2;
    for(int32_t i = 0; i < listlength(dir->dirs); i++)
    {
        vfs_directory_t* a;
        peekitem(dir->dirs, i, (uint32_t*)&a);
        for(uint32_t d = 0; d < depth; d++)
        {
            printf(" ");
        }
        printf("%s\n", a->name);
        print_tree(a, depth);
    }
    for(int32_t i = 0; i < listlength(dir->files); i++)
    {
        vfs_file_t* a;
        peekitem(dir->files, i, (uint32_t*)&a);
        for(uint32_t d = 0; d < depth; d++)
        {
            printf(" ");
        }
        printf("%s\n", a->name);
    }
}

void flist(vfs_directory_t* dir)
{
    print_tree(dir, 0);
}