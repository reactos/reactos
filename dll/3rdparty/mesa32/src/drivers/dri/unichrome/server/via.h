#ifndef __VIA_H__
#define __VIA_H__

typedef struct VIAInfo
{
    size_t registerSize;
    void * registerHandle;
    void * data;
} * VIAInfoPtr;

#endif /* __VIA_H__ */
