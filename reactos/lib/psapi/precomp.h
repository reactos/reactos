#define NTOS_MODE_USER
#include <windows.h>
#include <psapi.h>
#include <ntos.h>
#include "internal.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ddk/ntddk.h>
#include <napi/teb.h>
#include <ntos/heap.h>
#include <ntdll/ldr.h>

