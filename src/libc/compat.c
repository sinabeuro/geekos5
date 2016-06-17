#include <conio.h>
#include <stddef.h>

void Init_Heap(size_t start, size_t size)
{
    /*Print("Creating user heap: start=%lx, size=%ld\n", start, size);*/
    bpool((void*) start, size);
}

void* Malloc(size_t size)
{
    void *result;

    assert(size > 0);
    result = bget(size);

    return result;
}

void* Calloc(size_t count, size_t size)
{
    void *result;

    assert(size > 0);
    result = bgetz(count* size);

    return result;
}

void* Realloc(void* buf, size_t size)
{
    void *result;

    assert(size > 0);
    result = bgetr(buf, size);

    return result;
}

void Free(void* buf)
{
    brel(buf);
}


