
#include <roscompat.h>

extern ROSCOMPAT_DESCRIPTOR __roscompat_descriptor__;

void* g_ForceLink = &__roscompat_descriptor__;
