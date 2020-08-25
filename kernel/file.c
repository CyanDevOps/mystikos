#include <libos/file.h>
#include <libos/syscall.h>
#include <libos/strings.h>
#include <libos/trace.h>
#include <libos/buf.h>
#include <stdio.h>
#include <stdlib.h>
#include <libos/eraise.h>
#include <libos/deprecated.h>
#include <libos/strings.h>
#include <libos/malloc.h>

int libos_creat(const char* pathname, mode_t mode)
{
    return (int)libos_syscall_ret(libos_syscall_creat(pathname, mode));
}

int libos_open(const char* pathname, int flags, mode_t mode)
{
    return (int)libos_syscall_ret(libos_syscall_open(pathname, flags, mode));
}

off_t libos_lseek(int fd, off_t offset, int whence)
{
    return (off_t)libos_syscall_ret(libos_syscall_lseek(fd, offset, whence));
}

int libos_close(int fd)
{
    return (int)libos_syscall_ret(libos_syscall_close(fd));
}

ssize_t libos_read(int fd, void* buf, size_t count)
{
    return (ssize_t)libos_syscall_ret(libos_syscall_read(fd, buf, count));
}

ssize_t libos_write(int fd, const void* buf, size_t count)
{
    return (ssize_t)libos_syscall_ret(libos_syscall_write(fd, buf, count));
}

ssize_t libos_readv(int fd, const struct iovec* iov, int iovcnt)
{
    return (ssize_t)libos_syscall_ret(libos_syscall_readv(fd, iov, iovcnt));
}

ssize_t libos_writev(int fd, const struct iovec* iov, int iovcnt)
{
    return (ssize_t)libos_syscall_ret(libos_syscall_writev(fd, iov, iovcnt));
}

int libos_stat(const char* pathname, struct stat* statbuf)
{
    return (int)libos_syscall_ret(libos_syscall_stat(pathname, statbuf));
}

int libos_lstat(const char* pathname, struct stat* statbuf)
{
    return (int)libos_syscall_ret(libos_syscall_lstat(pathname, statbuf));
}

int libos_fstat(int fd, struct stat* statbuf)
{
    return (int)libos_syscall_ret(libos_syscall_fstat(fd, statbuf));
}

int libos_mkdir(const char *pathname, mode_t mode)
{
    return (int)libos_syscall_ret(libos_syscall_mkdir(pathname, mode));
}

int libos_rmdir(const char* pathname)
{
    return (int)libos_syscall_ret(libos_syscall_rmdir(pathname));
}

int libos_getdents64(int fd, struct dirent* dirp, size_t count)
{
    return (int)libos_syscall_ret(libos_syscall_getdents64(fd, dirp, count));
}

int libos_link(const char* oldpath, const char* newpath)
{
    return (int)libos_syscall_ret(libos_syscall_link(oldpath, newpath));
}

int libos_unlink(const char* pathname)
{
    return (int)libos_syscall_ret(libos_syscall_unlink(pathname));
}

int libos_access(const char* pathname, int mode)
{
    return (int)libos_syscall_ret(libos_syscall_access(pathname, mode));
}

int libos_rename(const char* oldpath, const char* newpath)
{
    return (int)libos_syscall_ret(libos_syscall_rename(oldpath, newpath));
}

int libos_truncate(const char* path, off_t length)
{
    return (int)libos_syscall_ret(libos_syscall_truncate(path, length));
}

int libos_ftruncate(int fd, off_t length)
{
    return (int)libos_syscall_ret(libos_syscall_ftruncate(fd, length));
}

ssize_t libos_readlink(const char* pathname, char* buf, size_t bufsiz)
{
    return (int)libos_syscall_ret(libos_syscall_readlink(pathname, buf, bufsiz));
}

int libos_symlink(const char* target, const char* linkpath)
{
    return (int)libos_syscall_ret(libos_syscall_symlink(target, linkpath));
}

int libos_mkdirhier(const char* pathname, mode_t mode)
{
    int ret = 0;
    char** toks = NULL;
    size_t ntoks;
    char path[PATH_MAX];
    const bool trace = libos_get_trace();
    struct stat buf;

    libos_set_trace(false);

    if (!pathname)
        ERAISE(-EINVAL);

    /* If the directory already exists, stop here */
    if (libos_stat(pathname, &buf) == 0 && S_ISDIR(buf.st_mode))
        goto done;

    ECHECK(libos_strsplit(pathname, "/", &toks, &ntoks));

    *path = '\0';

    for (size_t i = 0; i < ntoks; i++)
    {
        if (LIBOS_STRLCAT(path, "/") >= PATH_MAX)
            ERAISE(-ENAMETOOLONG);

        if (LIBOS_STRLCAT(path, toks[i]) >= PATH_MAX)
            ERAISE(-ENAMETOOLONG);

        if (libos_stat(path, &buf) == 0)
        {
            if (!S_ISDIR(buf.st_mode))
                ERAISE(-ENOTDIR);
        }
        else
        {
            ECHECK(libos_mkdir(path, mode));
        }
    }

    if (libos_stat(pathname, &buf) != 0 || !S_ISDIR(buf.st_mode))
        ERAISE(-EPERM);

done:

    if (toks)
        libos_free(toks);

    libos_set_trace(trace);

    return ret;
}

int libos_load_file(const char* path, void** data, size_t* size)
{
    int ret = 0;
    ssize_t n;
    struct stat st;
    libos_buf_t buf = LIBOS_BUF_INITIALIZER;
    char block[512];
    int fd = -1;

    if (data)
        *data = NULL;

    if (size)
        *size = 0;

    if (!path || !data || !size)
        ERAISE(-EINVAL);

    if ((fd = libos_open(path, O_RDONLY, 0)) < 0)
        ERAISE(-ENOENT);

    if (libos_fstat(fd, &st) != 0)
        ERAISE(-EINVAL);

    if (libos_buf_reserve(&buf, (size_t)st.st_size) != 0)
        ERAISE(-ENOMEM);

    while ((n = libos_read(fd, block, sizeof(block))) > 0)
    {
        if (libos_buf_append(&buf, block, (size_t)n) != 0)
            ERAISE(-ENOMEM);
    }

    *data = buf.data;
    buf.data = NULL;
    *size = buf.size;

done:

    if (fd >= 0)
        libos_close(fd);

    if (buf.data)
        libos_free(buf.data);

    return ret;
}

int libos_write_file(int fd, const void* data, size_t size)
{
    int ret = 0;
    const uint8_t* p = (const uint8_t*)data;
    size_t r = size;
    ssize_t n;

    if (fd < 0 || !data)
        ERAISE(-EINVAL);

    while (r > 0)
    {
        if ((n = libos_write(fd, p, r)) == 0)
            break;

        if (n < 0)
            ERAISE(-errno);

        p += n;
        r -= (size_t)n;
    }

    if (r != 0)
        ERAISE(-EIO);

done:
    return ret;
}
