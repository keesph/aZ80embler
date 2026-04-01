#ifndef ALLOC_W_H
#define ALLOC_W_H

#include <stddef.h>

void *calloc_w(size_t count, size_t size);
void *malloc_w(size_t size);
void *strdup_w(char *);
int asprintf_w(char **buffer, char *format, ...);

#endif
