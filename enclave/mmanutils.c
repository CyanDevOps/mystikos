#include <oel/mmanutils.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

static oel_mman_t _mman;
static void* _mman_start;
static void* _mman_end;

static uint8_t GUARD_CHAR = 0xAA;

static int _check_guard(const void* p)
{
    for (size_t i = 0; i < PAGE_SIZE; i++)
    {
        if (((uint8_t*)p)[i] != GUARD_CHAR)
            return -1;
    }

    return 0;
}

static void _dump(uint8_t* p, size_t n)
{
    while (n--)
        printf("%02X", *p++);

    printf("\n");
}

int oel_setup_mman(size_t size)
{
    int ret = -1;
    void* base;
    void* ptr;

    /* Allocate aligned pages */
    if (!(ptr = memalign(OE_PAGE_SIZE, PAGE_SIZE + size + PAGE_SIZE)))
        goto done;

    base = ptr + PAGE_SIZE;

    _mman_start = base;
    _mman_end = base + size;

    /* Set the guard pages */
    memset(_mman_start - PAGE_SIZE, GUARD_CHAR, PAGE_SIZE);
    memset(_mman_end, GUARD_CHAR, PAGE_SIZE);

    if (oel_mman_init(&_mman, (uintptr_t)base, size) != 0)
        goto done;

    _mman.scrub = true;

    oel_mman_set_sanity(&_mman, true);

    ret = 0;

done:
    return ret;
}

int oel_teardown_mman(void)
{
    assert(oel_mman_is_sane(&_mman));

    /* Check the start guard page */
    if (_check_guard(_mman_start - PAGE_SIZE) != 0)
    {
        fprintf(stderr, "bad mman start guard page\n");
        _dump(_mman_start - PAGE_SIZE, PAGE_SIZE);
        assert(false);
    }

    /* Check the end guard page */
    if (_check_guard(_mman_end) != 0)
    {
        fprintf(stderr, "bad mman end guard page\n");
        _dump(_mman_end, PAGE_SIZE);
        assert(false);
    }

    free((void*)_mman.base - PAGE_SIZE);
}

#if 0
static void _write_file(const char* path, const void* data, size_t size)
{
    int fd;
    const uint8_t* p = (const uint8_t*)data;
    size_t r = size;
    ssize_t n;

    if ((fd = open(path, O_CREAT|O_WRONLY|O_TRUNC, 0666)) < 0)
    {
        fprintf(stderr, "open failed: %s\n", path);
        exit(1);
    }

    while ((n = write(fd, p, r)) > 0)
    {
        p += n;
        r -= n;
    }

    if (r != 0)
    {
        fprintf(stderr, "write failed: %s\n", path);
        exit(1);
    }

    close(fd);
}
#endif

static ssize_t _map_file_onto_memory(
    int fd,
    off_t offset,
    void* data,
    size_t size)
{
    ssize_t ret = -1;
    ssize_t bytes_read = 0;
    off_t save_pos;

    if (fd < 0 || !data || !size)
        goto done;

    /* save the current file position */
    if ((save_pos = lseek(fd, 0, SEEK_CUR)) == (off_t)-1)
        goto done;

    /* seek start of file */
    if (lseek(fd, offset, SEEK_SET) == (off_t)-1)
        goto done;

    /* read file onto memory */
    {
        char buf[BUFSIZ];
        ssize_t n;
        uint8_t* p = data;
        size_t r = size;

        while ((n = read(fd, buf, sizeof buf)) > 0)
        {
#if 1
            /* if copy would write past end of data */
            if (r < n)
            {
                memcpy(p, buf, r);
                break;
            }
#endif

            memcpy(p, buf, n);
            p += n;
            r -= n;
            bytes_read += n;
        }
    }

    /* restore the file position */
    if (lseek(fd, save_pos, SEEK_SET) == (off_t)-1)
        goto done;

    ret = bytes_read;

done:
    return ret;
}

void* oel_mmap(
    void *addr,
    size_t length,
    int prot,
    int flags,
    int fd,
    off_t offset)
{
    void* ptr = (void*)-1;

    if (fd >= 0 && addr)
    {
        ssize_t n;

#if 0
        printf("addr: [%016lX][%016lX]\n", (long)addr, length);
#endif

        if ((n = _map_file_onto_memory(fd, offset, addr, length)) < 0)
            return (void*)-1;

        void* end = addr + length;
        assert(addr >= _mman_start && addr <= _mman_end);
        assert(end >= _mman_start && end <= _mman_end);

        // ISSUE: call mmap or mremap here so that this range refers to
        // a mapped region.

        return addr;
    }

    int tflags = OEL_MAP_ANONYMOUS | OEL_MAP_PRIVATE;

    if (oel_mman_map(&_mman, addr, length, prot, tflags, &ptr) != 0)
    {
        printf("oel_mman_map: error: %s\n", _mman.err);
        return (void*)-1;
    }

    if (fd >= 0 && !addr)
    {
        ssize_t n;

        if ((n = _map_file_onto_memory(fd, offset, ptr, length)) < 0)
        {
            return (void*)-1;
        }
    }

    void* end = ptr + length;
    assert(ptr >= _mman_start && ptr <= _mman_end);
    assert(end >= _mman_start && end <= _mman_end);

    return ptr;
}