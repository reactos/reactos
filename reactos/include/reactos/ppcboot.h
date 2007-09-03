#ifndef REACTOS_PPCBOOT_H
#define REACTOS_PPCBOOT_H

typedef char font_char[57];
typedef struct _boot_infos_t {
    void *loaderBlock;
    int dispDeviceRect[4];
    int dispDeviceRowBytes;
    int dispDeviceDepth;
    void *dispDeviceBase;
    font_char *dispFont;
} boot_infos_t;

#endif/*REACTOS_PPCBOOT_H*/
