#include <stdio.h>
#define WIN32_NO_STATUS
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/sefuncs.h>
#include <ndk/rtlfuncs.h>
#include <userenv.h>
#include <sddl.h>
#include <shlobj.h>

#include "internal.h"
#include "resources.h"
