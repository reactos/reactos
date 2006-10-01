#ifndef REACTOS_PPCBOOT_H
#define REACTOS_PPCBOOT_H

typedef struct _boot_infos_t {
    int dispDeviceRect[4];
    int dispDeviceRowBytes;
    int dispDeviceDepth;
    void *dispDeviceBase;
} boot_infos_t;

#endif/*REACTOS_PPCBOOT_H*/
