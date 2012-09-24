/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/eng/engmisc.c
 * PURPOSE:         Miscellaneous Support Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

VOID
APIENTRY
EngSort(IN OUT PBYTE Buffer,
        IN ULONG ElemSize,
        IN ULONG ElemCount,
        IN SORTCOMP CompFunc)
{
    /* Forward to the CRT */
    qsort(Buffer, ElemCount, ElemSize, CompFunc);
}

/* DX stubs */
PVOID
APIENTRY
EngAllocPrivateUserMem(PDD_SURFACE_LOCAL  psl,
                       SIZE_T  cj,
                       ULONG  tag)
{
    UNIMPLEMENTED;
    return NULL;
}

VOID
APIENTRY
EngFreePrivateUserMem(PDD_SURFACE_LOCAL  psl,
                      PVOID  pv)
{
    UNIMPLEMENTED;
}

FLATPTR
APIENTRY
HeapVidMemAllocAligned(LPVIDMEM lpVidMem,
                       DWORD dwWidth,
                       DWORD dwHeight,
                       LPSURFACEALIGNMENT lpAlignment,
                       LPLONG lpNewPitch)
{
    UNIMPLEMENTED;
    return 0;
}

VOID
APIENTRY
VidMemFree(LPVMEMHEAP pvmh,
           FLATPTR ptr)
{
    UNIMPLEMENTED;
    return;
}

DWORD
APIENTRY
EngDxIoctl(ULONG ulIoctl,
           PVOID pBuffer,
           ULONG ulBufferSize)
{
    UNIMPLEMENTED;
    return 0;
}

PDD_SURFACE_LOCAL
APIENTRY
EngLockDirectDrawSurface(HANDLE hSurface)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
EngUnlockDirectDrawSurface(PDD_SURFACE_LOCAL pSurface)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * Blatantly stolen from ldr/utils.c in ntdll.  I can't link ntdll from
 * here, though.
 */
NTSTATUS APIENTRY
LdrGetProcedureAddress (IN PVOID BaseAddress,
                        IN PANSI_STRING Name,
                        IN ULONG Ordinal,
                        OUT PVOID *ProcedureAddress)
{
   PIMAGE_EXPORT_DIRECTORY ExportDir;
   PUSHORT OrdinalPtr;
   PULONG NamePtr;
   PULONG AddressPtr;
   ULONG i = 0;

   DPRINT("LdrGetProcedureAddress (BaseAddress %x Name %Z Ordinal %lu ProcedureAddress %x)\n",
          BaseAddress, Name, Ordinal, ProcedureAddress);

   /* Get the pointer to the export directory */
   ExportDir = (PIMAGE_EXPORT_DIRECTORY)
                RtlImageDirectoryEntryToData (BaseAddress,
                                              TRUE,
                                              IMAGE_DIRECTORY_ENTRY_EXPORT,
                                              &i);

   DPRINT("ExportDir %x i %lu\n", ExportDir, i);

   if (!ExportDir || !i || !ProcedureAddress)
     {
        return STATUS_INVALID_PARAMETER;
     }

   AddressPtr = (PULONG)((ULONG_PTR)BaseAddress + (ULONG)ExportDir->AddressOfFunctions);
   if (Name && Name->Length)
     {
        /* by name */
        OrdinalPtr = (PUSHORT)((ULONG_PTR)BaseAddress + (ULONG)ExportDir->AddressOfNameOrdinals);
        NamePtr = (PULONG)((ULONG_PTR)BaseAddress + (ULONG)ExportDir->AddressOfNames);
        for( i = 0; i < ExportDir->NumberOfNames; i++, NamePtr++, OrdinalPtr++)
          {
             if (!strcmp(Name->Buffer, (char*)((ULONG_PTR)BaseAddress + *NamePtr)))
               {
                  *ProcedureAddress = (PVOID)((ULONG_PTR)BaseAddress + (ULONG)AddressPtr[*OrdinalPtr]);
                  return STATUS_SUCCESS;
               }
          }
        DPRINT1("LdrGetProcedureAddress: Can't resolve symbol '%Z'\n", Name);
     }
   else
     {
        /* by ordinal */
        Ordinal &= 0x0000FFFF;
        if (Ordinal - ExportDir->Base < ExportDir->NumberOfFunctions)
          {
             *ProcedureAddress = (PVOID)((ULONG_PTR)BaseAddress + (ULONG_PTR)AddressPtr[Ordinal - ExportDir->Base]);
             return STATUS_SUCCESS;
          }
        DPRINT1("LdrGetProcedureAddress: Can't resolve symbol @%d\n", Ordinal);
  }

   return STATUS_PROCEDURE_NOT_FOUND;
}

PVOID
APIENTRY
EngFindImageProcAddress(IN HANDLE hModule,
                        IN PSTR ProcName)
{
    ANSI_STRING ProcNameString;
    PVOID Function;
    NTSTATUS Status;
    ULONG i;
    static struct
    {
        PCSTR ProcName;
        PVOID ProcAddress;
    } Win32kExports[] =
    {
        { "BRUSHOBJ_hGetColorTransform",    BRUSHOBJ_hGetColorTransform    },
        { "EngAlphaBlend",                  EngAlphaBlend                  },
        { "EngClearEvent",                  EngClearEvent                  },
        { "EngControlSprites",              EngControlSprites              },
        { "EngCreateEvent",                 EngCreateEvent                 },
        { "EngDeleteEvent",                 EngDeleteEvent                 },
        { "EngDeleteFile",                  EngDeleteFile                  },
        { "EngDeleteSafeSemaphore",         EngDeleteSafeSemaphore         },
        { "EngDeleteWnd",                   EngDeleteWnd                   },
        { "EngDitherColor",                 EngDitherColor                 },
        { "EngGetPrinterDriver",            EngGetPrinterDriver            },
        { "EngGradientFill",                EngGradientFill                },
        { "EngHangNotification",            EngHangNotification            },
        { "EngInitializeSafeSemaphore",     EngInitializeSafeSemaphore     },
        { "EngLockDirectDrawSurface",       EngLockDirectDrawSurface       },
        { "EngLpkInstalled",                EngLpkInstalled                },
        { "EngMapEvent",                    EngMapEvent                    },
        { "EngMapFile",                     EngMapFile                     },
        { "EngMapFontFileFD",               EngMapFontFileFD               },
        { "EngModifySurface",               EngModifySurface               },
        { "EngMovePointer",                 EngMovePointer                 },
        { "EngPlgBlt",                      EngPlgBlt                      },
        { "EngQueryDeviceAttribute",        EngQueryDeviceAttribute        },
        { "EngQueryPalette",                EngQueryPalette                },
        { "EngQuerySystemAttribute",        EngQuerySystemAttribute        },
        { "EngReadStateEvent",              EngReadStateEvent              },
        { "EngRestoreFloatingPointState",   EngRestoreFloatingPointState   },
        { "EngSaveFloatingPointState",      EngSaveFloatingPointState      },
        { "EngSetEvent",                    EngSetEvent                    },
        { "EngSetPointerShape",             EngSetPointerShape             },
        { "EngSetPointerTag",               EngSetPointerTag               },
        { "EngStretchBltROP",               EngStretchBltROP               },
        { "EngTransparentBlt",              EngTransparentBlt              },
        { "EngUnlockDirectDrawSurface",     EngUnlockDirectDrawSurface     },
        { "EngUnmapEvent",                  EngUnmapEvent                  },
        { "EngUnmapFile",                   EngUnmapFile                   },
        { "EngUnmapFontFileFD",             EngUnmapFontFileFD             },
        { "EngWaitForSingleObject",         EngWaitForSingleObject         },
        { "FONTOBJ_pfdg",                   FONTOBJ_pfdg                   },
        { "FONTOBJ_pjOpenTypeTablePointer", FONTOBJ_pjOpenTypeTablePointer },
        { "FONTOBJ_pQueryGlyphAttrs",       FONTOBJ_pQueryGlyphAttrs       },
        { "FONTOBJ_pwszFontFilePaths",      FONTOBJ_pwszFontFilePaths      },
        { "HeapVidMemAllocAligned",         HeapVidMemAllocAligned         },
        { "HT_Get8BPPMaskPalette",          HT_Get8BPPMaskPalette          },
        { "STROBJ_bEnumPositionsOnly",      STROBJ_bEnumPositionsOnly      },
        { "STROBJ_bGetAdvanceWidths",       STROBJ_bGetAdvanceWidths       },
        { "STROBJ_fxBreakExtra",            STROBJ_fxBreakExtra            },
        { "STROBJ_fxCharacterExtra",        STROBJ_fxCharacterExtra        },
        { "VidMemFree",                     VidMemFree                     },
        { "XLATEOBJ_hGetColorTransform",    XLATEOBJ_hGetColorTransform    }
    };

    /* If hModule is NULL, look in our own exports */
    if (!hModule)
    {
        DPRINT("Looking for win32k export %s\n", ProcName);

        /* Go through all our exports */
        for (i = 0; i < sizeof(Win32kExports) / sizeof(Win32kExports[0]); i++)
        {
            /* Compare by name */
            if (!strcmp(ProcName, Win32kExports[i].ProcName))
            {
                DPRINT("Found it index %u address %p\n", i, Win32kExports[i].ProcName);

                /* Found it, return its address */
                return Win32kExports[i].ProcAddress;
            }
        }

        /* Nothing found */
        return NULL;
    }

    /* Convert proc name to ANSI_STRING */
    RtlInitAnsiString(&ProcNameString, ProcName);

    /* Get procedure address of a module */
    Status = LdrGetProcedureAddress(hModule,
                                    &ProcNameString,
                                    0,
                                    &Function);

    if (!NT_SUCCESS(Status)) return NULL;

    /* Return function pointer if it's been found */
    return Function;
}

PVOID
APIENTRY
EngFindResource(IN HANDLE hModule,
                IN INT iName,
                IN INT iType,
                OUT PULONG pulSize)
{
    UNIMPLEMENTED;
	return NULL;
}

BOOL
APIENTRY
EngLpkInstalled(VOID)
{
    UNIMPLEMENTED;
	return FALSE;
}

INT
APIENTRY
EngMulDiv(IN INT a,
          IN INT  b,
          IN INT  c)
{
    UNIMPLEMENTED;
	return 0;
}

HANDLE
APIENTRY
EngGetCurrentProcessId(VOID)
{
    UNIMPLEMENTED;
	return NULL;
}

HANDLE
APIENTRY
EngGetCurrentThreadId(VOID)
{
    UNIMPLEMENTED;
	return NULL;
}

HANDLE
APIENTRY
EngGetProcessHandle(VOID)
{
    /* Deprecated */
	return NULL;
}

ULONGLONG
APIENTRY
EngGetTickCount(VOID)
{
    ULONG Multiplier;
    LARGE_INTEGER TickCount;

    /* Get the multiplier and current tick count */
    KeQueryTickCount(&TickCount);
    Multiplier = SharedUserData->TickCountMultiplier;

    /* Convert to milliseconds and return */
    return (Int64ShrlMod32(UInt32x32To64(Multiplier, TickCount.LowPart), 24) +
            (Multiplier * (TickCount.HighPart << 8)));
}
