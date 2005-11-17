
#include "config.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "wincrypt.h"
#include "winreg.h"
#include "snmp.h"
#include "winnls.h"
#include "mssip.h"
#include "crypt32_private.h"
#include "wine/debug.h"

#define NTOS_MODE_USER
#include <ndk/ntndk.h>
