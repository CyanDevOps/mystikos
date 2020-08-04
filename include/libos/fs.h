#ifndef _LIBOS_FS_H
#define _LIBOS_FS_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/uio.h>
#include <dirent.h>

typedef struct libos_fs libos_fs_t;

typedef struct libos_file libos_file_t;

struct libos_fs
{
    int (*fs_release)(libos_fs_t* fs);

    int (*fs_creat)(
        libos_fs_t* fs,
        const char* pathname,
        mode_t mode,
        libos_file_t** file);

    int (*fs_open)(
        libos_fs_t* fs,
        const char* pathname,
        int flags,
        mode_t mode,
        libos_file_t** file);

    off_t (*fs_lseek)(
        libos_fs_t* fs,
        libos_file_t* file,
        off_t offset,
        int whence);

    ssize_t (*fs_read)(
        libos_fs_t* fs,
        libos_file_t* file,
        void* buf,
        size_t count);

    ssize_t (*fs_write)(
        libos_fs_t* fs,
        libos_file_t* file,
        const void* buf,
        size_t count);

    ssize_t (*fs_readv)(
        libos_fs_t* fs,
        libos_file_t* file,
        const struct iovec* iov,
        int iovcnt);

    ssize_t (*fs_writev)(
        libos_fs_t* fs,
        libos_file_t* file,
        const struct iovec* iov,
        int iovcnt);

    int (*fs_close)(libos_fs_t* fs, libos_file_t* file);

    int (*fs_access)(libos_fs_t* fs, const char* pathname, int mode);

    int (*fs_stat)(libos_fs_t* fs, const char* pathname, struct stat* statbuf);

    int (*fs_fstat)(libos_fs_t* fs, libos_file_t* file, struct stat* statbuf);

    int (*fs_link)(libos_fs_t* fs, const char* oldpath, const char* newpath);

    int (*fs_unlink)(libos_fs_t* fs, const char* pathname);

    int (*fs_rename)(libos_fs_t* fs, const char* oldpath, const char* newpath);

    int (*fs_truncate)(libos_fs_t* fs, const char* path, off_t length);

    int (*fs_ftruncate)(libos_fs_t* fs, libos_file_t* file, off_t length);

    int (*fs_mkdir)(libos_fs_t* fs, const char* pathname, mode_t mode);

    int (*fs_rmdir)(libos_fs_t* fs, const char* pathname);

    int (*fs_getdents64)(
        libos_fs_t* fs,
        libos_file_t* file,
        struct dirent* dirp,
        size_t count);
};

#endif /* _LIBOS_FS_H */