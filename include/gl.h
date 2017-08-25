#ifndef __GL_H__
#define __GL_H__

#include <common.h>

typedef struct bitmap_t
{
    uint32_t* buffer;
    uint32_t width;
    uint32_t height;
}bitmap_t;

typedef bitmap_t* (*gl_init)(uint32_t width, uint32_t height);
typedef void (*gl_delete)(bitmap_t* target);
typedef void (*gl_clear)(bitmap_t* target, uint32_t colour);

#endif