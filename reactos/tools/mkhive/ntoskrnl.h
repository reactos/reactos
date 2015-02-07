/*
 * This header is used together with cmindex.c and cmname.c
 */

#define NDEBUG
#include "mkhive.h"

PVOID
NTAPI
CmpAllocate(
    IN SIZE_T Size,
    IN BOOLEAN Paged,
    IN ULONG Tag
);
