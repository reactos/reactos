/*
 * Resource definitions
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_RESOURCE_H
#define __WINE_RESOURCE_H

#include <windows.h>

//#ifndef __WRC_RSC_H
//#include "wrc_rsc.h"
//#endif

/*
 * BS: I comment this out to catch all occurences
 * of reference to this structure which is now
 * rendered obsolete.
 *
 * struct resource
 * {
 *     int id;
 *     int type;
 *     const char *name;
 *     const unsigned char* bytes;
 *     unsigned size;
 * };
 */

/* Built-in resources */
typedef enum
{
    SYSRES_MENU_SYSMENU,
    SYSRES_MENU_EDITMENU,
    SYSRES_DIALOG_MSGBOX,
    SYSRES_DIALOG_SHELL_ABOUT_MSGBOX,
    SYSRES_DIALOG_OPEN_FILE,
    SYSRES_DIALOG_SAVE_FILE,
    SYSRES_DIALOG_PRINT,
    SYSRES_DIALOG_PRINT_SETUP,
    SYSRES_DIALOG_CHOOSE_FONT,
    SYSRES_DIALOG_CHOOSE_COLOR,
    SYSRES_DIALOG_FIND_TEXT,
    SYSRES_DIALOG_REPLACE_TEXT
} SYSTEM_RESOURCE;

//extern void LIBRES_RegisterResources(const wrc_resource32_t * const * Res);

#if defined(__GNUC__) && (__GNUC__ == 2) && (__GNUC_MINOR__ >= 7)
#define WINE_CONSTRUCTOR  __attribute__((constructor))
#define HAVE_WINE_CONSTRUCTOR
#else
#define WINE_CONSTRUCTOR
#undef HAVE_WINE_CONSTRUCTOR
#endif

HGLOBAL SYSRES_LoadResource( SYSTEM_RESOURCE id );
void SYSRES_FreeResource( HGLOBAL handle );
LPCVOID SYSRES_GetResPtr( SYSTEM_RESOURCE id );

#endif /* __WINE_RESOURCE_H */
