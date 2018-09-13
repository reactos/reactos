/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    pdkeys.c

Abstract:

    This module contains the KEY definitions for the the predefined Key
    handles.  It is part of the Configuration Registry Tools (CRTools) library.

Author:

    David J. Gilman (davegi) 09-Jan-1992

Environment:

    Windows, Crt - User Mode

--*/

#include "crtools.h"

KEY  KeyClassesRoot     =   {
                            NULL,
                            HKEY_CLASSES_ROOT,
                            HKEY_CLASSES_ROOT_STRING,
                            HKEY_CLASSES_ROOT_STRING,
                            NULL,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            { 0, 0 }
#if DBG
                            , KEY_SIGNATURE
#endif
                        };



KEY  KeyCurrentUser     =   {
                            NULL,
                            HKEY_CURRENT_USER,
                            HKEY_CURRENT_USER_STRING,
                            HKEY_CURRENT_USER_STRING,
                            NULL,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            { 0, 0 }
#if DBG
                            , KEY_SIGNATURE
#endif
                        };

KEY  KeyLocalMachine    =   {
                            NULL,
                            HKEY_LOCAL_MACHINE,
                            HKEY_LOCAL_MACHINE_STRING,
                            HKEY_LOCAL_MACHINE_STRING,
                            NULL,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            { 0, 0 }
#if DBG
                            , KEY_SIGNATURE
#endif
                        };

KEY  KeyUsers           =   {
                            NULL,
                            HKEY_USERS,
                            HKEY_USERS_STRING,
                            HKEY_USERS_STRING,
                            NULL,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            { 0, 0 }
#if DBG
                            , KEY_SIGNATURE
#endif
                        };
