// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _LIBOS_FDOPS_H
#define _LIBOS_FDOPS_H

#include <stddef.h>
#include <sys/stat.h>

typedef struct libos_fdops libos_fdops_t;

struct libos_fdops
{
    ssize_t (*fd_read)(void* device, void* object, void* buf, size_t count);

    ssize_t (
        *fd_write)(void* device, void* object, const void* buf, size_t count);

    ssize_t (*fd_readv)(
        void* device,
        void* object,
        const struct iovec* iov,
        int iovcnt);

    ssize_t (*fd_writev)(
        void* device,
        void* object,
        const struct iovec* iov,
        int iovcnt);

    int (*fd_fstat)(void* device, void* object, struct stat* statbuf);

    int (*fd_fcntl)(void* device, void* object, int cmd, long arg);

    int (*fd_close)(void* device, void* object);
};

#endif /* _LIBOS_FDOPS_H */