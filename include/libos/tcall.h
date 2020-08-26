// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _LIBOS_TCALL_H
#define _LIBOS_TCALL_H

#include <libos/defs.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

typedef enum libos_tcall_number
{
    LIBOS_TCALL_RANDOM = 2048,
    LIBOS_TCALL_THREAD_SELF,
    LIBOS_TCALL_ALLOCATE,
    LIBOS_TCALL_DEALLOCATE,
    LIBOS_TCALL_VSNPRINTF,
    LIBOS_TCALL_WRITE_CONSOLE,
    LIBOS_TCALL_GEN_CREDS,
    LIBOS_TCALL_FREE_CREDS,
    LIBOS_TCALL_CLOCK_GETTIME,
    LIBOS_TCALL_ISATTY,
    LIBOS_TCALL_ADD_SYMBOL_FILE,
    LIBOS_TCALL_LOAD_SYMBOLS,
    LIBOS_TCALL_UNLOAD_SYMBOLS,
}
libos_tcall_number_t;

long libos_tcall(long n, long params[6]);

typedef long (*libos_tcall_t)(long n, long params[6]);

LIBOS_INLINE long libos_tcall_random(void* data, size_t size)
{
    long params[6] = { (long)data, (long)size };
    return libos_tcall(LIBOS_TCALL_RANDOM, params);
}

LIBOS_INLINE long libos_tcall_thread_self(void)
{
    long params[6] = { 0 };
    return libos_tcall(LIBOS_TCALL_THREAD_SELF, params);
}

LIBOS_INLINE long libos_tcall_vsnprintf(
    char* str,
    size_t size,
    const char* format,
    va_list ap)
{
    long params[6] = { 0 };
    params[0] = (long)str;
    params[1] = (long)size;
    params[2] = (long)format;
    params[3] = (long)ap;
    return libos_tcall(LIBOS_TCALL_VSNPRINTF, params);
}

LIBOS_INLINE long libos_tcall_write_console(
    int fd,
    const void* buf,
    size_t count)
{
    long params[6] = { 0 };
    params[0] = (long)fd;
    params[1] = (long)buf;
    params[2] = (long)count;
    return libos_tcall(LIBOS_TCALL_WRITE_CONSOLE, params);
}

#endif /* _LIBOS_TCALL_H */