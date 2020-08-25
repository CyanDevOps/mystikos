// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#include <stdlib.h>
#include <string.h>

#include <libos/strarr.h>
#include <libos/deprecated.h>
#include <libos/strings.h>
#include <libos/malloc.h>

static size_t _CAPACITY = 32;

void libos_strarr_release(libos_strarr_t* self)
{
    size_t i;

    for (i = 0; i < self->size; i++)
        libos_free(self->data[i]);

    libos_free(self->data);
    libos_memset(self, 0, sizeof(libos_strarr_t));
}

int libos_strarr_append(libos_strarr_t* self, const char* str)
{
    /* Increase the capacity */
    if (self->size == self->capacity)
    {
        size_t capacity;
        void* data;

        if (self->capacity)
            capacity = self->capacity * 2;
        else
            capacity = _CAPACITY;

        if (!(data = libos_realloc(self->data, sizeof(char*) * capacity)))
            return -1;

        self->data = data;
        self->capacity = capacity;
    }

    /* Append the new string */
    {
        char* newStr;

        if (str)
        {
            if (!(newStr = libos_strdup(str)))
                return -1;
        }
        else
        {
            newStr = NULL;
        }

        self->data[self->size++] = newStr;
    }

    return 0;
}

int libos_strarr_remove(libos_strarr_t* self, size_t index)
{
    size_t i;

    /* Check for out of bounds */
    if (index >= self->size)
        return -1;

    libos_free(self->data[index]);

    /* Remove the element */
    for (i = index; i < self->size - 1; i++)
    {
        self->data[i] = self->data[i + 1];
    }

    self->size--;
    return 0;
}

void libos_strarr_sort(libos_strarr_t* self)
{
    size_t i;
    size_t j;
    size_t n;

    if (self->size == 0)
        return;

    n = self->size - 1;

    for (i = 0; i < self->size - 1; i++)
    {
        bool swapped = false;

        for (j = 0; j < n; j++)
        {
            if (libos_strcmp(self->data[j], self->data[j + 1]) > 0)
            {
                char* tmp = self->data[j];
                self->data[j] = self->data[j + 1];
                self->data[j + 1] = tmp;
                swapped = true;
            }
        }

        if (!swapped)
            break;

        n--;
    }
}
