// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#include <openenclave/enclave.h>
#include <libos/ramfs.h>
#include <libos/mount.h>
#include <libos/file.h>
#include <libos/trace.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "run_t.h"

const char alpha[] = "abcdefghijklmnopqrstuvwxyz";
const char ALPHA[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

static size_t _fsize(const char* path)
{
    struct stat buf;

    if (libos_stat(path, &buf) != 0)
        return (size_t)-1;

    return (size_t)buf.st_size;
}

static size_t _fdsize(int fd)
{
    struct stat buf;

    if (libos_fstat(fd, &buf) != 0)
        return (size_t)-1;

    return (size_t)buf.st_size;
}

void test_misc()
{
    libos_fs_t* fs;
    libos_file_t* file = NULL;

    if (libos_init_ramfs(&fs) != 0)
    {
        fprintf(stderr, "libos_init_ramfs() failed\n");
        abort();
    }

    assert(fs != NULL);

    /* Open the root directory */
    {
        if ((*fs->fs_open)(fs, "/", O_RDONLY, 0, &file) != 0)
        {
            fprintf(stderr, "fs_open() failed\n");
            abort();
        }

        assert(file != NULL);
    }

    /* Read the directory entries in the root directory */
    {
        size_t i = 0;
        ssize_t n;
        struct dirent buf;

        while ((n = (*fs->fs_read)(fs, file, &buf, sizeof(buf))) > 0)
        {
            assert(n == sizeof(buf));

            if (i == 0)
            {
                assert(strcmp(buf.d_name, ".") == 0);
            }
            else if (i == 1)
            {
                assert(strcmp(buf.d_name, "..") == 0);
            }

            i++;
        }
    }

    /* Close the root directory */
    {
        if ((*fs->fs_close)(fs, file) != 0)
        {
            fprintf(stderr, "fs_close() failed\n");
            abort();
        }

        file = NULL;
    }

    /* Open a new file */
    {
        int flags = O_WRONLY | O_CREAT;

        if ((*fs->fs_open)(fs, "/file", flags, 0, &file) != 0)
        {
            fprintf(stderr, "fs_open() failed\n");
            abort();
        }
    }

    /* Write to the new file */
    {
        if ((*fs->fs_write)(fs, file, alpha, sizeof(alpha)) != sizeof(alpha))
        {
            fprintf(stderr, "fs_write() failed\n");
            abort();
        }

        if ((*fs->fs_write)(fs, file, ALPHA, sizeof(ALPHA)) != sizeof(ALPHA))
        {
            fprintf(stderr, "fs_write() failed\n");
            abort();
        }
    }

    /* Close the file */
    if ((*fs->fs_close)(fs, file) != 0)
    {
        fprintf(stderr, "fs_close() failed\n");
        abort();
    }

    /* Reopen the file */
    if ((*fs->fs_open)(fs, "/file", O_RDONLY, 0, &file) != 0)
    {
        fprintf(stderr, "fs_open() failed\n");
        abort();
    }

    /* Read the file */
    {
        char buf[sizeof(alpha)];

        if ((*fs->fs_read)(fs, file, buf, sizeof(buf)) != sizeof(buf))
        {
            fprintf(stderr, "fs_read() failed\n");
            abort();
        }

        assert(strcmp(buf, alpha) == 0);

        if ((*fs->fs_read)(fs, file, buf, sizeof(buf)) != sizeof(buf))
        {
            fprintf(stderr, "fs_read() failed\n");
            abort();
        }

        assert(strcmp(buf, ALPHA) == 0);
    }

    /* test stat() */
    {
        struct stat buf;

        if ((*fs->fs_fstat)(fs, file, &buf) != 0)
        {
            fprintf(stderr, "fs_stat() failed\n");
            abort();
        }

        assert(buf.st_size = sizeof(alpha) + sizeof(ALPHA));
        assert(buf.st_blksize == 512);
        assert(buf.st_blocks == 1);
    }

    if ((*fs->fs_close)(fs, file) != 0)
    {
        fprintf(stderr, "fs_close() failed\n");
        abort();
    }

    /* Release the file system */
    if ((*fs->fs_release)(fs) != 0)
    {
        fprintf(stderr, "fs_close() failed\n");
        abort();
    }
}

void test_readv(void)
{
    int fd;
    char buf[sizeof(ALPHA)];

    assert((fd = libos_open("/test_readv", O_CREAT | O_WRONLY, 0)) >= 0);
    assert(libos_write(fd, alpha, sizeof(alpha)) == sizeof(alpha));
    assert(libos_lseek(fd, 0, SEEK_CUR) == sizeof(alpha));
    assert(libos_write(fd, ALPHA, sizeof(ALPHA)) == sizeof(ALPHA));
    assert(libos_lseek(fd, 0, SEEK_CUR) == sizeof(alpha) + sizeof(ALPHA));
    assert(libos_close(fd) == 0);

    assert((fd = libos_open("/test_readv", O_RDONLY, 0)) >= 0);
    assert(libos_lseek(fd, 0, SEEK_CUR) == 0);
    assert(libos_read(fd, buf, sizeof(buf)) == sizeof(buf));
    assert(libos_lseek(fd, 0, SEEK_CUR) == sizeof(alpha));
    assert(strcmp(buf, alpha) == 0);

    /* Test readv() */
    {
        struct iovec iov[2];
        uint8_t buf1[20];
        uint8_t buf2[7];
        iov[0].iov_base = buf1;
        iov[0].iov_len = sizeof(buf1);
        iov[1].iov_base = buf2;
        iov[1].iov_len = sizeof(buf2);
        assert(libos_readv(fd, iov, 2) == sizeof(ALPHA));
        assert(memcmp(buf1, ALPHA, sizeof(buf1)) == 0);
        assert(memcmp(buf2, ALPHA + sizeof(buf1), sizeof(buf2)) == 0);
    }

    /* Test readv() */
    {
        struct iovec iov[2];
        uint8_t buf1[20];
        uint8_t buf2[7];
        iov[0].iov_base = buf1;
        iov[0].iov_len = sizeof(buf1);
        iov[1].iov_base = buf2;
        iov[1].iov_len = sizeof(buf2);
        assert(libos_lseek(fd, 0, SEEK_SET) == 0);
        assert(libos_readv(fd, iov, 2) == sizeof(alpha));
        assert(memcmp(buf1, alpha, sizeof(buf1)) == 0);
        assert(memcmp(buf2, alpha + sizeof(buf1), sizeof(buf2)) == 0);
    }

    assert(libos_lseek(fd, sizeof(alpha), SEEK_SET) == sizeof(alpha));
    assert(libos_lseek(fd, 0, SEEK_CUR) == sizeof(alpha));

    assert(libos_close(fd) == 0);
}

void test_writev(void)
{
    int fd;
    char buf[sizeof(ALPHA)];

    assert((fd = libos_open("/test_writev", O_CREAT | O_WRONLY, 0)) >= 0);

    struct iovec iov[2];
    iov[0].iov_base = (void*)alpha;
    iov[0].iov_len = sizeof(alpha);
    iov[1].iov_base = (void*)ALPHA;
    iov[1].iov_len = sizeof(ALPHA);

    assert(libos_writev(fd, iov, 2) == sizeof(alpha) + sizeof(ALPHA));
    assert(libos_close(fd) == 0);

    assert((fd = libos_open("/test_writev", O_RDONLY, 0)) >= 0);
    assert(libos_read(fd, buf, sizeof(buf)) == sizeof(buf));
    assert(strcmp(buf, alpha) == 0);
    assert(libos_read(fd, buf, sizeof(buf)) == sizeof(buf));
    assert(strcmp(buf, ALPHA) == 0);

    assert(libos_close(fd) == 0);
}

void test_stat()
{
    struct stat buf;

    assert(libos_stat("/test_readv", &buf) == 0);
    assert(buf.st_size == sizeof(alpha) + sizeof(ALPHA));
}

static uint64_t _nlink(const char* path)
{
    struct stat buf;
    assert(libos_stat(path, &buf) == 0);
    return buf.st_nlink;
}

void test_mkdir()
{
    struct stat buf;
    int fd;

    assert(libos_mkdir("/a", 0777) == 0);
    assert(libos_stat("/a", &buf) == 0);
    assert(S_ISDIR(buf.st_mode));

    assert(libos_mkdir("/a/bb", 0777) == 0);
    assert(libos_mkdir("/a/bb/ccc", 0777) == 0);

    assert(libos_stat("/a", &buf) == 0);
    assert(S_ISDIR(buf.st_mode));

    assert(libos_stat("/a/bb", &buf) == 0);
    assert(S_ISDIR(buf.st_mode));

    assert(libos_stat("/a/bb/ccc", &buf) == 0);
    assert(S_ISDIR(buf.st_mode));

    for (size_t i = 0; i < 2; i++)
    {
        assert((fd = libos_creat("/a/bb/ccc/file", 0666)) >= 0);
        assert(libos_stat("/a/bb/ccc/file", &buf) == 0);
        assert(S_ISREG(buf.st_mode));
        assert(libos_close(fd) == 0);
    }

    assert(_nlink("/a") == 2);
    assert(_nlink("/a/bb") == 2);
    assert(_nlink("/a/bb/ccc") == 2);
    assert(_nlink("/a/bb/ccc/file") == 1);
}

void test_rmdir()
{
    struct stat buf;

    assert(libos_mkdir("/rmdir", 0777) == 0);
    assert(libos_mkdir("/rmdir/rmdir", 0777) == 0);
    assert(libos_mkdir("/rmdir/rmdir/rmdir", 0777) == 0);

    assert(libos_rmdir("/rmdir/rmdir/rmdir") == 0);
    assert(libos_rmdir("/rmdir/rmdir") == 0);
    assert(libos_rmdir("/rmdir") == 0);
    assert(libos_stat("/", &buf) == 0);
}

void test_readdir()
{
    int fd;
    DIR* dir;
    struct dirent* ent;
    const struct
    {
        const char* name;
        unsigned char type;
    }
    entries[] =
    {
        { ".", DT_DIR },
        { "..", DT_DIR },
        { "dir1", DT_DIR },
        { "dir2", DT_DIR },
        { "file1", DT_REG },
        { "file2", DT_REG },
    };
    const size_t nentries = sizeof(entries) / sizeof(entries[0]);
    size_t i = 0;
    off_t off = 0;

    assert(libos_mkdir("/readdir", 0777) == 0);
    assert(libos_mkdir("/readdir/dir1", 0777) == 0);
    assert(libos_mkdir("/readdir/dir2", 0777) == 0);
    assert((fd = libos_creat("/readdir/file1", 0666)) >= 0);
    assert(libos_close(fd) == 0);
    assert((fd = libos_creat("/readdir/file2", 0666)) >= 0);
    assert(libos_close(fd) == 0);

    assert(dir = libos_opendir("/readdir"));

    while ((ent = libos_readdir(dir)))
    {
        assert(ent->d_ino != 0);
        assert(strcmp(ent->d_name, entries[i].name) == 0);
        assert(ent->d_type == entries[i].type);
        assert(ent->d_off == off);
        assert(ent->d_reclen == sizeof(struct dirent));
        i++;
        off += (off_t)sizeof(struct dirent);
    }

    assert(i == nentries);
    assert(libos_closedir(dir) == 0);
}

void dump_dirents(const char* path)
{
    DIR* dir;
    struct dirent* ent;

    assert((dir = libos_opendir(path)));

    printf("=== dump_dirents(%s)\n", path);

    while ((ent = libos_readdir(dir)))
    {
        printf("name=\"%s\": ino=%lx type=%u off=%ld reclen=%ld\n",
            ent->d_name, ent->d_ino, ent->d_type, ent->d_off, ent->d_off);
    }

    libos_closedir(dir);
}

void test_link()
{
    int fd;

    assert(libos_mkdir("/link", 0777) == 0);
    assert(libos_mkdir("/link/dir", 0777) == 0);

    assert((fd = libos_creat("/link/file1", 0666)) >= 0);
    assert(libos_close(fd) == 0);

    assert(_nlink("/link/file1") == 1);
    assert(libos_link("/link/file1", "/link/file2") == 0);
    assert(_nlink("/link/file1") == 2);
    assert(_nlink("/link/file2") == 2);
    assert(libos_link("/link/file2", "/link/dir/file3") == 0);
    assert(_nlink("/link/file1") == 3);
    assert(_nlink("/link/file2") == 3);
    assert(_nlink("/link/dir/file3") == 3);

    assert(libos_unlink("/link/file1") == 0);
    assert(_nlink("/link/dir/file3") == 2);
    assert(libos_unlink("/link/file2") == 0);
    assert(_nlink("/link/dir/file3") == 1);
    assert(libos_unlink("/link/dir/file3") == 0);
}

static int _touch(const char* pathname, mode_t mode)
{
    int fd;

    if ((fd = libos_creat(pathname, mode)) < 0)
        return -1;

    if (libos_close(fd) != 0)
        return -1;

    return 0;
}

void test_access()
{
    assert(libos_mkdir("/access", 777) == 0);
    assert(_touch("/access/r", S_IRUSR) == 0);
    assert(_touch("/access/w", S_IWUSR) == 0);
    assert(_touch("/access/x", S_IXUSR) == 0);

    assert(libos_access("/access/r", F_OK) == 0);
    assert(libos_access("/access/w", F_OK) == 0);
    assert(libos_access("/access/x", F_OK) == 0);

    assert(libos_access("/access/r", R_OK) == 0);
    assert(libos_access("/access/w", W_OK) == 0);
    assert(libos_access("/access/x", X_OK) == 0);

    libos_set_trace(false);
    assert(libos_access("/access/r", X_OK) != 0);
    assert(libos_access("/access/w", R_OK) != 0);
    assert(libos_access("/access/x", W_OK) != 0);
    libos_set_trace(true);
}

void test_rename(void)
{
    assert(libos_mkdir("/rename", 0777) == 0);
    assert(_touch("/rename/file1", 0400) == 0);
    assert(libos_rename("/rename/file1", "/rename/file2") == 0);
    libos_set_trace(false);
    assert(libos_access("/rename/file1", R_OK) != 0);
    libos_set_trace(true);
    assert(libos_access("/rename/file2", R_OK) == 0);
}

void test_truncate(void)
{
    int fd;

    assert(libos_mkdir("/truncate", 777) == 0);

    assert((fd = libos_open("/truncate/alpha", O_CREAT | O_WRONLY, 0)) >= 0);
    assert(libos_write(fd, alpha, sizeof(alpha)) == sizeof(alpha));
    assert(libos_close(fd) == 0);

    {
        const off_t new_size = sizeof(alpha) * 2;
        assert(_fsize("/truncate/alpha") == sizeof(alpha));
        assert(libos_truncate("/truncate/alpha", new_size) == 0);
        assert(_fsize("/truncate/alpha") == (size_t)new_size);
        assert(libos_truncate("/truncate/alpha", sizeof(alpha)) == 0);
        assert(_fsize("/truncate/alpha") == sizeof(alpha));
        assert(libos_truncate("/truncate/alpha", 0) == 0);
        assert(_fsize("/truncate/alpha") == 0);
    }

    {
        assert((fd = libos_open("/truncate/alpha", O_RDWR, 0)) >= 0);
        assert(_fdsize(fd) == 0);
        assert(libos_ftruncate(fd, sizeof(alpha)) == 0);
        assert(_fdsize(fd) == sizeof(alpha));
        assert(libos_ftruncate(fd, 2*sizeof(alpha)) == 0);
        assert(_fdsize(fd) == 2*sizeof(alpha));
        assert(libos_ftruncate(fd, 0) == 0);
        assert(_fdsize(fd) == 0);
        assert(libos_close(fd) == 0);
    }
}

int run_ecall(void)
{
    libos_fs_t* fs;

    test_misc();

    assert(libos_init_ramfs(&fs) == 0);
    assert(libos_mount(fs, "/") == 0);

    assert(_nlink("/") == 1);

    test_readv();
    test_writev();
    test_stat();
    test_mkdir();
    test_rmdir();
    test_readdir();
    test_link();
    test_access();
    test_rename();
    test_truncate();

    assert((*fs->fs_release)(fs) == 0);

    return 0;
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* Debug */
    16*4096, /* NumHeapPages */
    4096, /* NumStackPages */
    2);   /* NumTCS */