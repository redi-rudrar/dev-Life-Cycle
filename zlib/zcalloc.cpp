
extern "C"
{
#include "zutil.h"
};

typedef void *voidpf;

#ifdef BAOHACK
// Using new/delete reduces page fault in C++ apps
voidpf zcalloc (voidpf opaque, unsigned items, unsigned size)
{
    if (opaque) items += size - size; /* make compiler happy */
	return new char[items *size];
}

void  zcfree (voidpf opaque, voidpf ptr)
{
    delete ptr;
}
#endif
