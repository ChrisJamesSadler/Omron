#ifndef __FS_H__
#define __FS_H__

typedef struct file_t
{
    char* name;
    uint32_t size;
    uint32_t pointer;
    uint32_t buffer;
}file_t;

extern file_t* fopen(char* name, ...);
extern uint32_t fread(file_t* afile, uint8_t* buffer, uint32_t count);
extern void fwrite(file_t* afile, uint8_t* buffer, uint32_t count);
extern void fclose(file_t* afile);

#endif