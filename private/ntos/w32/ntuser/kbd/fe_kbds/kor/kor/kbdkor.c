/***************************************************************************\
* Module Name: kbdkor.c (Type A)
*
* Copyright (c) 1985-92, Microsoft Corporation
*
* Keyboard Type A :  Hangeul Toggle : Right Alt
*                    Junja   Toggle : Left  Alt + '='
*                    Hanja   Toggle : Right Ctrl
\***************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <ime.h>
#include "vkoem.h"

#include "..\101a\kbd101a.c"

