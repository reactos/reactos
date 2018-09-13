/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    inflogcf.c

Abstract:

    Routines to parse logical configuration sections in
    win95-style INF files, and place the output in the registry.

Author:

    Ted Miller (tedm) 8-Mar-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


PCTSTR pszHexDigits = TEXT("0123456789ABCDEF");

#define INFCHAR_SIZE_SEP            TEXT('@')
#define INFCHAR_RANGE_SEP           TEXT('-')
#define INFCHAR_ALIGN_SEP           TEXT('%')
#define INFCHAR_ATTR_START          TEXT('(')
#define INFCHAR_ATTR_END            TEXT(')')
#define INFCHAR_MEMATTR_READ        TEXT('R')
#define INFCHAR_MEMATTR_WRITE       TEXT('W')
#define INFCHAR_MEMATTR_PREFETCH    TEXT('F')
#define INFCHAR_MEMATTR_COMBINEDWRITE TEXT('C')
#define INFCHAR_MEMATTR_CACHEABLE   TEXT('H')
#define INFCHAR_MEMATTR_DWORD       TEXT('D')
#define INFCHAR_MEMATTR_ATTRIBUTE   TEXT('A')
#define INFCHAR_DECODE_START        TEXT('(')
#define INFCHAR_DECODE_END          TEXT(')')
#define INFCHAR_DECODE_SEP          TEXT(':')
#define INFCHAR_IRQATTR_SEP         TEXT(':')
#define INFCHAR_IRQATTR_SHARE       TEXT('S')
#define INFCHAR_IRQATTR_LEVEL       TEXT('L')
#define INFCHAR_DMAWIDTH_NARROW     TEXT('N')   // i.e., 8-bit
#define INFCHAR_DMAWIDTH_WORD       TEXT('W')   // i.e., 16-bit
#define INFCHAR_DMAWIDTH_DWORD      TEXT('D')   // i.e., 32-bit
#define INFCHAR_DMA_BUSMASTER       TEXT('M')
#define INFCHAR_DMATYPE_A           TEXT('A')
#define INFCHAR_DMATYPE_B           TEXT('B')
#define INFCHAR_DMATYPE_F           TEXT('F')
#define INFCHAR_IOATTR_MEMORY       TEXT('M')
#define INFCHAR_PCCARD_IOATTR_WORD  TEXT('W')
#define INFCHAR_PCCARD_IOATTR_SRC   TEXT('S')
#define INFCHAR_PCCARD_IOATTR_Z8    TEXT('Z')
#define INFCHAR_PCCARD_ATTR_WAIT    TEXT('X')
#define INFCHAR_PCCARD_ATTR_WAITI   TEXT('I')
#define INFCHAR_PCCARD_ATTR_WAITM   TEXT('M')
#define INFCHAR_PCCARD_MEMATTR_WORD TEXT('M')
#define INFCHAR_PCCARD_MEM_ISATTR   TEXT('A')
#define INFCHAR_PCCARD_MEM_ISCOMMON TEXT('C')
#define INFCHAR_PCCARD_SEP          TEXT(':')
#define INFCHAR_PCCARD_ATTR_SEP     TEXT(' ')
#define INFCHAR_MFCARD_AUDIO_ATTR   TEXT('A')

#define INFLOGCONF_IOPORT_10BIT_DECODE    0x000003ff
#define INFLOGCONF_IOPORT_12BIT_DECODE    0x00000fff
#define INFLOGCONF_IOPORT_16BIT_DECODE    0x0000ffff
#define INFLOGCONF_IOPORT_POSITIVE_DECODE 0x00000000

#define DEFAULT_IOPORT_DECODE             INFLOGCONF_IOPORT_10BIT_DECODE

#define DEFAULT_MEMORY_ALIGNMENT    0xfffffffffffff000  // 4K-aligned (a'la Win9x)
#define DEFAULT_IOPORT_ALIGNMENT    0xffffffffffffffff  // byte-aligned
#define DEFAULT_IRQ_AFFINITY        0xffffffff          // use any processor


//
// Mapping between registry key specs in an inf file
// and predefined registry handles.
//
STRING_TO_DATA InfPrioritySpecToPriority[] = {  INFSTR_CFGPRI_HARDWIRED   , LCPRI_HARDWIRED,
                                                INFSTR_CFGPRI_DESIRED     , LCPRI_DESIRED,
                                                INFSTR_CFGPRI_NORMAL      , LCPRI_NORMAL,
                                                INFSTR_CFGPRI_SUBOPTIMAL  , LCPRI_SUBOPTIMAL,
                                                INFSTR_CFGPRI_DISABLED    , LCPRI_DISABLED,
                                                INFSTR_CFGPRI_RESTART     , LCPRI_RESTART,
                                                INFSTR_CFGPRI_REBOOT      , LCPRI_REBOOT,
                                                INFSTR_CFGPRI_POWEROFF    , LCPRI_POWEROFF,
                                                INFSTR_CFGPRI_HARDRECONFIG, LCPRI_HARDRECONFIG,
                                                INFSTR_CFGPRI_FORCECONFIG , LCPRI_FORCECONFIG,
                                                NULL                      , 0
                                             };


STRING_TO_DATA InfConfigSpecToConfig[] = {  INFSTR_CFGTYPE_BASIC   , BASIC_LOG_CONF,
                                            INFSTR_CFGTYPE_FORCED  , FORCED_LOG_CONF,
                                            INFSTR_CFGTYPE_OVERRIDE, OVERRIDE_LOG_CONF,
                                            NULL                   , 0
                                         };

//
// Declare strings used in processing INF LogConfigs.
//
// These strings are defined in infstr.h:
//
CONST TCHAR pszMemConfig[]      = INFSTR_KEY_MEMCONFIG,
            pszIOConfig[]       = INFSTR_KEY_IOCONFIG,
            pszIRQConfig[]      = INFSTR_KEY_IRQCONFIG,
            pszDMAConfig[]      = INFSTR_KEY_DMACONFIG,
            pszPcCardConfig[]   = INFSTR_KEY_PCCARDCONFIG,
            pszMfCardConfig[]   = INFSTR_KEY_MFCARDCONFIG,
            pszConfigPriority[] = INFSTR_KEY_CONFIGPRIORITY,
            pszDriverVer[]      = INFSTR_DRIVERVERSION_SECTION;


BOOL
pHexToScalar(
    IN  PCTSTR     FieldStart,
    IN  PCTSTR     FieldEnd,
    OUT PDWORDLONG Value,
    IN  BOOL       Want64Bits
    )
{
    UINT DigitCount;
    UINT i;
    DWORDLONG Accum;
    WORD Types[16];

    //
    // Make sure the number is in range by checking the number
    // of hex digits.
    //
    DigitCount = (UINT)(FieldEnd - FieldStart);
    if((DigitCount == 0)
    || (DigitCount > (UINT)(Want64Bits ? 16 : 8))
    || !GetStringTypeEx(LOCALE_SYSTEM_DEFAULT,CT_CTYPE1,FieldStart,DigitCount,Types)) {
        return(FALSE);
    }

    Accum = 0;
    for(i=0; i<DigitCount; i++) {
        if(!(Types[i] & C1_XDIGIT)) {
            return(FALSE);
        }
        Accum *= 16;
        Accum += _tcschr(pszHexDigits,(TCHAR)CharUpper((PTSTR)FieldStart[i])) - pszHexDigits;
    }

    *Value = Accum;
    return(TRUE);
}


BOOL
pHexToUlong(
    IN  PCTSTR FieldStart,
    IN  PCTSTR FieldEnd,
    OUT PDWORD Value
    )

/*++

Routine Description:

    Convert a sequence of unicode hex digits into an
    unsigned 32-bit number. Digits are validated.

Arguments:

    FieldStart - supplies pointer to unicode digit sequence.

    FieldEnd - supplies pointer to first character beyond the
        digit sequence.

    Value - receives 32-bit number

Return Value:

    TRUE if the number is in range and valid. FALSE otherwise.

--*/

{
    DWORDLONG x;
    BOOL b;

    if(b = pHexToScalar(FieldStart,FieldEnd,&x,FALSE)) {
        *Value = (DWORD)x;
    }
    return(b);
}


BOOL
pHexToUlonglong(
    IN  PCTSTR     FieldStart,
    IN  PCTSTR     FieldEnd,
    OUT PDWORDLONG Value
    )

/*++

Routine Description:

    Convert a sequence of unicode hex digits into an
    unsigned 64-bit number. Digits are validated.

Arguments:

    FieldStart - supplies pointer to unicode digit sequence.

    FieldEnd - supplies pointer to first character beyond the
        digit sequence.

    Value - receives 64-bit number

Return Value:

    TRUE if the number is in range and valid. FALSE otherwise.

--*/

{
    return(pHexToScalar(FieldStart,FieldEnd,Value,TRUE));
}


BOOL
pDecimalToUlong(
    IN  PCTSTR Field,
    OUT PDWORD Value
    )

/*++

Routine Description:

    Convert a nul-terminated sequence of unicode decimal digits into an
    unsigned 32-bit number. Digits are validated.

Arguments:

    Field - supplies pointer to unicode digit sequence.

    Value - receives DWORD number

Return Value:

    TRUE if the number is in range and valid. FALSE otherwise.

--*/

{
    UINT DigitCount;
    UINT i;
    DWORDLONG Accum;
    WORD Types[10];

    DigitCount = lstrlen(Field);
    if((DigitCount == 0) || (DigitCount > 10)
    || !GetStringTypeEx(LOCALE_SYSTEM_DEFAULT,CT_CTYPE1,Field,DigitCount,Types)) {
        return(FALSE);
    }

    Accum = 0;
    for(i=0; i<DigitCount; i++) {
        if(!(Types[i] & C1_DIGIT)) {
            return(FALSE);
        }
        Accum *= 10;
        Accum += _tcschr(pszHexDigits,(TCHAR)CharUpper((PTSTR)Field[i])) - pszHexDigits;

        //
        // Check overflow
        //
        if(Accum > 0xffffffff) {
            return(FALSE);
        }
    }

    *Value = (DWORD)Accum;
    return(TRUE);
}


DWORD
pSetupProcessMemConfig(
    IN LOG_CONF    LogConfig,
    IN PINFCONTEXT InfLine,
    IN HMACHINE        hMachine
    )

/*++

Routine Description:

    Process a MemConfig line in a Win95 INF. Such lines specify
    memory requirements for a device. Each line is expected to be
    in the form

    MemConfig = <start>-<end>[(<attr>)],<start>-<end>[(<attr>)],...

    <start> is the start of a memory range (64-bit hex)
    <end>   is the end of a memory range   (64-bit hex)
    <attr>  if present is a string of 0 or more chars from
            C - memory is combined-write
            D - memory is 32-bit, otherwise 24-bit.
            F - memory is prefetchable
            H - memory is cacheable
            R - memory is read-only
            W - memory is write-only
                (If R and W are specified or neither is specified the memory
                is read/write)

    or

    MemConfig = <size>@<min>-<max>[%align][(<attr>)],...

    <size>  is the size of a memory range (32-bit hex)
    <min>   is the minimum address where the memory range can be (64-bit hex)
    <max>   is the maximum address where the memory range can be (64-bit hex)
    <align> (if specified) is the alignment mask for the addresses (32-bit hex)
    <attr>  as above.

    ie, 8000@C0000-D7FFF%F0000 says the device needs a 32K memory window
    starting at any 64K-aligned address between C0000 and D7FFF.

    The default memory alignment is 4K (FFFFF000).

Arguments:

Return Value:

--*/

{
    UINT FieldCount,i;
    PCTSTR Field;
    DWORD d;
    PTCHAR p;
    INT u;
    UINT Attributes;
    DWORD RangeSize;
    ULARGE_INTEGER Align;
    DWORDLONG Start,End;
    PMEM_RESOURCE MemRes;
    PMEM_RANGE MemRange;
    RES_DES ResDes;
    PVOID q;
    BOOL bReadFlag = FALSE, bWriteFlag = FALSE;

    FieldCount = SetupGetFieldCount(InfLine);
    if (!FieldCount && GetLastError() != NO_ERROR) {
        return GetLastError();
    }

    if(MemRes = MyMalloc(offsetof(MEM_RESOURCE,MEM_Data))) {

        ZeroMemory(MemRes,offsetof(MEM_RESOURCE,MEM_Data));
        MemRes->MEM_Header.MD_Type = MType_Range;

        d = NO_ERROR;

    } else {
        d = ERROR_NOT_ENOUGH_MEMORY;
    }

    for(i=1; (d==NO_ERROR) && (i<=FieldCount); i++) {

        Field = pSetupGetField(InfLine,i);

        Attributes = 0;
        RangeSize = 0;
        Align.QuadPart = DEFAULT_MEMORY_ALIGNMENT;

        //
        // See if this is in the start-end or size@min-max format.
        // If we have a size, use it.
        //
        if(p = _tcschr(Field,INFCHAR_SIZE_SEP)) {
            if(pHexToUlong(Field,p,&RangeSize)) {
                Field = ++p;
            } else {
                d = ERROR_INVALID_INF_LOGCONFIG;
            }
        }

        //
        // We should now have a x-y which is either start/end or min/max.
        //
        if((d == NO_ERROR)                              // no err so far
        && (p = _tcschr(Field,INFCHAR_RANGE_SEP))       // Field: start of min; p: end of min
        && pHexToUlonglong(Field,p,&Start)              // get min
        && (Field = p+1)                                // Field: start of max
        && (   (p = _tcschr(Field,INFCHAR_ALIGN_SEP))
            || (p = _tcschr(Field,INFCHAR_ATTR_START))
            || (p = _tcschr(Field,0)))                  // p: end of max
        && pHexToUlonglong(Field,p,&End)) {             // get max
            //
            // If we get here Field is pointing either at the end of the field,
            // at the % that starts the alignment mask spec, or at the
            // ( that starts the attributes spec.
            //
            Field = p;
            if(*Field == INFCHAR_ALIGN_SEP) {
                Field++;
                p = _tcschr(Field,INFCHAR_ATTR_START);
                if(!p) {
                    p = _tcschr(Field,0);
                }
                if(pHexToUlonglong(Field, p, &(Align.QuadPart))) {
                    //
                    // NOTE:  Since these mask values are actually stored in a WDM
                    // resource list (i.e., IO_RESOURCE_REQUIREMENTS_LIST), there's
                    // no way to specify an alignment greater than 32 bits.  However,
                    // since the alignment value was implemented as a mask (for
                    // compatibility with Win9x), we must specify it as a 64-bit
                    // quantity, since it is applied to a 64-bit value.  We will check
                    // below to ensure that the most significant DWORD is all ones.
                    //
                    // Also, we must handle alignment values such as 000F0000, 00FF0000,
                    // 0FFF0000, and FFFF0000.  These all specify 64K alignment (depending
                    // on the min and max addresses, the INF writer might not need to
                    // specify all the 1 bits in the 32-bit value).
                    // Thus we perform an ersatz sign extension of sorts -- we
                    // find the highest 1 bit and replicate it into all the
                    // more significant bits in the value.
                    //
                    for(u=31; u>=0; u--) {
                        if(Align.HighPart & (1 << u)) {
                            break;
                        }
                        Align.HighPart |= (1 << u);
                    }
                    //
                    // Make sure that all the bits in the most-significant DWORD are set,
                    // because we can't express this alignment otherwise (as discussed
                    // above).  Also, make sure that if we encountered a '1' in the high
                    // dword, then the high bit of the low dword is '1' as well.
                    //
                    if((Align.HighPart ^ 0xffffffff) ||
                       ((u >= 0) && !(Align.LowPart & 0x80000000))) {

                        d = ERROR_INVALID_INF_LOGCONFIG;

                    } else {
                        //
                        // Do the sign extension for the low dword.
                        //
                        for(u=31; u>=0; u--) {
                            if(Align.LowPart & (1 << u)) {
                                break;
                            }
                            Align.LowPart |= (1 << u);
                        }
                    }

                } else {
                    d = ERROR_INVALID_INF_LOGCONFIG;
                }
            }

            //
            // See if we have attributes.
            //
            if((d == NO_ERROR) && (*p == INFCHAR_ATTR_START)) {
                Field = ++p;
                if(p = _tcschr(Field,INFCHAR_ATTR_END)) {
                    //
                    // C for combined-write
                    // D for 32-bit memory
                    // F for prefetchable
                    // H for cacheable
                    // R for readable
                    // W for writeable
                    // RW (or neither) means read/write
                    //
                    while((d == NO_ERROR) && (Field < p)) {

                        switch((TCHAR)CharUpper((PTSTR)(*Field))) {
                        case INFCHAR_MEMATTR_READ:
                            bReadFlag = TRUE;
                            break;
                        case INFCHAR_MEMATTR_WRITE:
                            bWriteFlag = TRUE;
                            break;
                        case INFCHAR_MEMATTR_PREFETCH:
                            Attributes |= fMD_PrefetchAllowed;
                            break;
                        case INFCHAR_MEMATTR_COMBINEDWRITE:
                            Attributes |= fMD_CombinedWriteAllowed;
                            break;
                        case INFCHAR_MEMATTR_DWORD:
                            Attributes |= fMD_32;
                            break;
                        case INFCHAR_MEMATTR_CACHEABLE:
                            Attributes |= fMD_Cacheable;
                            break;
                        default:
                            d = ERROR_INVALID_INF_LOGCONFIG;
                            break;
                        }

                        Field++;
                    }

                } else {
                    d = ERROR_INVALID_INF_LOGCONFIG;
                }
            }
        } else {
            d = ERROR_INVALID_INF_LOGCONFIG;
        }

        if(d == NO_ERROR) {
            //
            // If no range size was specified, then calculate it from
            // the given start and end addresses. Since this happens
            // when the memory requirement was an absolute start/end,
            // there is no alignment requirement.
            //
            if(RangeSize == 0) {
                RangeSize = (DWORD)(End-Start)+1;
                Align.QuadPart = DEFAULT_MEMORY_ALIGNMENT;
            }

            //
            // Slam values into the header part of the memory descriptor.
            // These will be ignored unless we're setting a forced config.
            // Note that the inf had better have specified forced mem configs
            // in a 'simple' form, since we throw away alignment, etc.
            //
            if (bWriteFlag && bReadFlag) {
                Attributes |=  fMD_ReadAllowed | fMD_RAM;       // read-write
            } else if (bWriteFlag && !bReadFlag) {
                Attributes |= fMD_ReadDisallowed | fMD_RAM;     // write only
            } else if (!bWriteFlag && bReadFlag) {
                Attributes |= fMD_ReadAllowed | fMD_ROM;        // read-only
            } else {
                Attributes |=  fMD_ReadAllowed | fMD_RAM;       // read-write
            }

            MemRes->MEM_Header.MD_Alloc_Base = Start;
            MemRes->MEM_Header.MD_Alloc_End = Start + RangeSize - 1;
            MemRes->MEM_Header.MD_Flags = Attributes;

            //
            // Add this guy into the descriptor we're building up.
            //
            q = MyRealloc(
                    MemRes,
                      offsetof(MEM_RESOURCE,MEM_Data)
                    + (sizeof(MEM_RANGE)*(MemRes->MEM_Header.MD_Count+1))
                    );

            if(q) {
                MemRes = q;
                MemRange = &MemRes->MEM_Data[MemRes->MEM_Header.MD_Count++];

                MemRange->MR_Align = Align.QuadPart;
                MemRange->MR_nBytes = RangeSize;
                MemRange->MR_Min = Start;
                MemRange->MR_Max = End;
                MemRange->MR_Flags = Attributes;
                MemRange->MR_Reserved = 0;

            } else {
                d = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
    }

    if((d == NO_ERROR) && MemRes->MEM_Header.MD_Count) {

        d = CM_Add_Res_Des_Ex(
                &ResDes,
                LogConfig,
                ResType_Mem,
                MemRes,
                offsetof(MEM_RESOURCE,MEM_Data) + (sizeof(MEM_RANGE) * MemRes->MEM_Header.MD_Count),
                0,
                hMachine);

        d = MapCrToSpError(d, ERROR_INVALID_DATA);

        if(d == NO_ERROR) {
            CM_Free_Res_Des_Handle(ResDes);
        }
    }

    if(MemRes) {
        MyFree(MemRes);
    }

    return(d);
}


DWORD
pSetupProcessIoConfig(
    IN LOG_CONF    LogConfig,
    IN PINFCONTEXT InfLine,
    IN HMACHINE        hMachine
    )

/*++

Routine Description:

    Process an IOConfig line in a Win95 INF. Such lines specify
    IO port requirements for a device. Each line is expected to be
    in the form

    IOConfig = <start>-<end>[(<decodemask>:<aliasoffset>:<attr>)],...

    <start> is the start of a port range (64-bit hex)

    <end> is the end of a port range (64-bit hex)

    <decodemask> defines the alias type, and may be one of the following combinations:

        3ff    10-bit decode,   IOR_Alias is 0x04
        fff    12-bit decode,   IOR_Alias is 0x10
        ffff   16-bit decode,   IOR_Alias is 0x00
        0      positive decode, IOR_Alias is 0xFF

    <aliasoffset> is ignored.

    <attr> if 'M', specifies port is a memory address, otherwise port is an IO address.

    or

    IOConfig = <size>@<min>-<max>[%align][(<decodemask>:<aliasoffset>:<attr>)],...

    <size>  is the size of a port range (32-bit hex)
    <min>   is the minimum port where the memory range can be (64-bit hex)
    <max>   is the maximum port where the memory range can be (64-bit hex)
    <align> (if specified) is the alignment mask for the ports (32-bit hex)
    <decodemask>, <aliasoffset>,<attr> as above

    ie, IOConfig = 1F8-1FF(3FF::),2F8-2FF(3FF::),3F8-3FF(3FF::)
        IOConfig = 8@300-32F%FF8(3FF::)
        IOConfig = 2E8-2E8(3FF:8000:)

Arguments:

Return Value:

--*/

{
    UINT FieldCount,i;
    PCTSTR Field;
    DWORD d;
    PTCHAR p;
    INT u;
    DWORD RangeSize;
    ULARGE_INTEGER Align;
    DWORDLONG Decode;
    DWORDLONG Start,End;
    BOOL GotSize;
    PIO_RESOURCE IoRes;
    PIO_RANGE IoRange;
    RES_DES ResDes;
    PVOID q;
    UINT Attributes = 0;
    PTCHAR Attr;

    FieldCount = SetupGetFieldCount(InfLine);
    if (!FieldCount && GetLastError() != NO_ERROR) {
        return GetLastError();
    }

    if(IoRes = MyMalloc(offsetof(IO_RESOURCE,IO_Data))) {

        ZeroMemory(IoRes,offsetof(IO_RESOURCE,IO_Data));
        IoRes->IO_Header.IOD_Type = IOType_Range;

        d = NO_ERROR;

    } else {
        d = ERROR_NOT_ENOUGH_MEMORY;
    }

    for(i=1; (d==NO_ERROR) && (i<=FieldCount); i++) {

        Field = pSetupGetField(InfLine,i);

        Attributes = fIOD_IO;
        Decode = DEFAULT_IOPORT_DECODE;
        RangeSize = 0;
        Align.QuadPart = DEFAULT_IOPORT_ALIGNMENT;

        //
        // See if this is in the start-end or size@min-max format.
        // If we have a size, use it.
        //
        if(p = _tcschr(Field,INFCHAR_SIZE_SEP)) {
            if(pHexToUlong(Field,p,&RangeSize)) {
                Field = ++p;
            } else {
                d = ERROR_INVALID_INF_LOGCONFIG;
            }
        }

        //
        // We should now have a x-y which is either start/end or min/max.
        //
        if((d == NO_ERROR)                              // no err so far
        && (p = _tcschr(Field,INFCHAR_RANGE_SEP))       // Field: start of min; p: end of min
        && pHexToUlonglong(Field,p,&Start)              // get min
        && (Field = p+1)                                // Field: start of max
        && (   (p = _tcschr(Field,INFCHAR_ALIGN_SEP))
            || (p = _tcschr(Field,INFCHAR_DECODE_START))
            || (p = _tcschr(Field,0)))                  // p: end of max
        && pHexToUlonglong(Field,p,&End)) {             // get max
            //
            // If we get here Field is pointing either at the end of the field,
            // or at the % that starts the alignment mask spec,
            // or at the ( that starts the decode stuff.
            //
            Field = p;
            switch(*Field) {
            case INFCHAR_ALIGN_SEP:
                Field++;


                p = _tcschr(Field,INFCHAR_ATTR_START);
                if(!p) {
                    p = _tcschr(Field,0);
                }
                if(pHexToUlonglong(Field, p, &(Align.QuadPart))) {
                    //
                    // NOTE:  Since these mask values are actually stored in a WDM
                    // resource list (i.e., IO_RESOURCE_REQUIREMENTS_LIST), there's
                    // no way to specify an alignment greater than 32 bits.  However,
                    // since the alignment value was implemented as a mask (for
                    // compatibility with Win9x), we must specify it as a 64-bit
                    // quantity, since it is applied to a 64-bit value.  We will check
                    // below to ensure that the most significant DWORD is all ones.
                    //
                    // Also, we must handle alignment values such as 000F0000, 00FF0000,
                    // 0FFF0000, and FFFF0000.  These all specify 64K alignment (depending
                    // on the min and max addresses, the INF writer might not need to
                    // specify all the 1 bits in the 32-bit value).
                    // Thus we perform an ersatz sign extension of sorts -- we
                    // find the highest 1 bit and replicate it into all the
                    // more significant bits in the value.
                    //
                    for(u=31; u>=0; u--) {
                        if(Align.HighPart & (1 << u)) {
                            break;
                        }
                        Align.HighPart |= (1 << u);
                    }
                    //
                    // Make sure that all the bits in the most-significant DWORD are set,
                    // because we can't express this alignment otherwise (as discussed
                    // above).  Also, make sure that if we encountered a '1' in the high
                    // dword, then the high bit of the low dword is '1' as well.
                    //
                    if((Align.HighPart ^ 0xffffffff) ||
                       ((u >= 0) && !(Align.LowPart & 0x80000000))) {

                        d = ERROR_INVALID_INF_LOGCONFIG;

                    } else {
                        //
                        // Do the sign extension for the low dword.
                        //
                        for(u=31; u>=0; u--) {
                            if(Align.LowPart & (1 << u)) {
                                break;
                            }
                            Align.LowPart |= (1 << u);
                        }
                    }

                } else {
                    d = ERROR_INVALID_INF_LOGCONFIG;
                }
                break;

            case INFCHAR_DECODE_START:
                //
                // Get decode value (this determines the IOR_Alias that gets filled
                // in for the resdes.
                //
                Field++;
                p = _tcschr(Field,INFCHAR_DECODE_SEP);
                if (p) {
                    if (Field != p) {
                        pHexToUlonglong(Field,p,&Decode);     // got decode value
                    }
                    Field = p+1;
                    p = _tcschr(Field,INFCHAR_DECODE_SEP);
                    if (p) {
                        //
                        // Ignore alias field.
                        //
                        Field = p+1;
                        p = _tcschr(Field,INFCHAR_DECODE_END);
                        if (p) {
                            if (Field != p) {
                                if (*Field == INFCHAR_IOATTR_MEMORY) {
                                    Attributes = fIOD_Memory; // got attribute value
                                }
                            }
                        } else {
                            d = ERROR_INVALID_INF_LOGCONFIG;
                        }
                    } else {
                        d = ERROR_INVALID_INF_LOGCONFIG;
                    }
                } else {
                    d = ERROR_INVALID_INF_LOGCONFIG;
                }
                break;
            }
        } else {
            d = ERROR_INVALID_INF_LOGCONFIG;
        }

        if(d == NO_ERROR) {
            //
            // If no range size was specified, then calculate it from
            // the given start and end addresses. Since this happens
            // when the port requirement was an absolute start/end,
            // there is no alignment requirement (i.e., the default
            // byte-alignment should be specified).
            //
            if(RangeSize == 0) {
                RangeSize = (DWORD)(End-Start)+1;
                Align.QuadPart = DEFAULT_IOPORT_ALIGNMENT;
            }

            //
            // Create an alternate decode flag
            //
            switch(Decode) {

                case INFLOGCONF_IOPORT_10BIT_DECODE:
                    Attributes |= fIOD_10_BIT_DECODE;
                    break;

                case INFLOGCONF_IOPORT_12BIT_DECODE:
                    Attributes |= fIOD_12_BIT_DECODE;
                    break;

                case INFLOGCONF_IOPORT_16BIT_DECODE:
                    Attributes |= fIOD_16_BIT_DECODE;
                    break;

                case INFLOGCONF_IOPORT_POSITIVE_DECODE:
                    Attributes |= fIOD_POSITIVE_DECODE;
                    break;
            }
            //
            // Slam values into the header part of the i/o descriptor.
            // These will be ignored unless we're setting a forced config.
            // Note that the inf had better have specified forced i/o configs
            // in a 'simple' form, since we throw away alignment, etc.
            //
            IoRes->IO_Header.IOD_Alloc_Base = Start;
            IoRes->IO_Header.IOD_Alloc_End = Start + RangeSize - 1;
            IoRes->IO_Header.IOD_DesFlags = Attributes;

            //
            // Add this guy into the descriptor we're building up.
            //
            q = MyRealloc(
                    IoRes,
                      offsetof(IO_RESOURCE,IO_Data)
                    + (sizeof(IO_RANGE)*(IoRes->IO_Header.IOD_Count+1))
                    );

            if(q) {
                IoRes = q;
                IoRange = &IoRes->IO_Data[IoRes->IO_Header.IOD_Count++];

                IoRange->IOR_Align = Align.QuadPart;
                IoRange->IOR_nPorts = RangeSize;
                IoRange->IOR_Min = Start;
                IoRange->IOR_Max = End;
                IoRange->IOR_RangeFlags = Attributes;

                switch(Decode) {

                    case INFLOGCONF_IOPORT_10BIT_DECODE:
                        IoRange->IOR_Alias = IO_ALIAS_10_BIT_DECODE;
                        break;

                    case INFLOGCONF_IOPORT_12BIT_DECODE:
                        IoRange->IOR_Alias = IO_ALIAS_12_BIT_DECODE;
                        break;

                    case INFLOGCONF_IOPORT_16BIT_DECODE:
                        IoRange->IOR_Alias = IO_ALIAS_16_BIT_DECODE;
                        break;

                    case INFLOGCONF_IOPORT_POSITIVE_DECODE:
                        IoRange->IOR_Alias = IO_ALIAS_POSITIVE_DECODE;
                        break;

                    default:
                        d = ERROR_INVALID_INF_LOGCONFIG;
                        break;
                }

            } else {
                d = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
    }

    if((d == NO_ERROR) && IoRes->IO_Header.IOD_Count) {

        d = CM_Add_Res_Des_Ex(
                &ResDes,
                LogConfig,
                ResType_IO,
                IoRes,
                offsetof(IO_RESOURCE,IO_Data) + (sizeof(IO_RANGE) * IoRes->IO_Header.IOD_Count),
                0,
                hMachine);

        d = MapCrToSpError(d, ERROR_INVALID_DATA);

        if(d == NO_ERROR) {
            CM_Free_Res_Des_Handle(ResDes);
        }
    }

    if(IoRes) {
        MyFree(IoRes);
    }

    return(d);
}


DWORD
pSetupProcessIrqConfig(
    IN LOG_CONF    LogConfig,
    IN PINFCONTEXT InfLine,
    IN HMACHINE    hMachine
    )

/*++

Routine Description:

    Process an IRQConfig line in a Win95 INF. Such lines specify
    IRQ requirements for a device. Each line is expected to be
    in the form

    IRQConfig = [[S][L]:]<IRQNum>,...

    S: if present indicates that the interrupt is shareable
    L: if present indicates that the interrupt is Level sensitive,
       otherwise it is assumed to be edge sensitive.
    IRQNum is the IRQ number in decimal.

Arguments:

Return Value:

--*/

{
    UINT FieldCount,i;
    PCTSTR Field;
    DWORD d;
    BOOL Shareable;
    BOOL Level;
    DWORD Irq;
    PIRQ_RESOURCE IrqRes;
    PIRQ_RANGE IrqRange;
    RES_DES ResDes;
    PVOID q;

    FieldCount = SetupGetFieldCount(InfLine);
    if (!FieldCount && GetLastError() != NO_ERROR) {
        return GetLastError();
    }

    if(IrqRes = MyMalloc(offsetof(IRQ_RESOURCE,IRQ_Data))) {

        ZeroMemory(IrqRes,offsetof(IRQ_RESOURCE,IRQ_Data));
        IrqRes->IRQ_Header.IRQD_Type = IRQType_Range;

        d = NO_ERROR;

    } else {
        d = ERROR_NOT_ENOUGH_MEMORY;
    }

    Shareable = FALSE;
    Level = FALSE;
    for(i=1; (d==NO_ERROR) && (i<=FieldCount); i++) {

        Field = pSetupGetField(InfLine,i);

        //
        // For first field, see if we have S: by itself...
        //
        if((i == 1)
        &&((TCHAR)CharUpper((PTSTR)Field[0]) == INFCHAR_IRQATTR_SHARE)
        && (Field[1] == INFCHAR_IRQATTR_SEP)) {

            Shareable = TRUE;
            Field+=2;
        }

        //
        // ... see if we have an L: by itself...
        //
        if((i == 1)
        &&((TCHAR)CharUpper((PTSTR)Field[0]) == INFCHAR_IRQATTR_LEVEL)
        && (Field[1] == INFCHAR_IRQATTR_SEP)) {

            Level = TRUE;
            Field+=2;
        }

        //
        // ... see if we have both attributes.
        //
        if((i == 1)
        && (Field[2] == INFCHAR_IRQATTR_SEP)) {

            if (((TCHAR)CharUpper((PTSTR)Field[0]) == INFCHAR_IRQATTR_SHARE)
            ||   (TCHAR)CharUpper((PTSTR)Field[1]) == INFCHAR_IRQATTR_SHARE) {

                Shareable = TRUE;
            }

            if (((TCHAR)CharUpper((PTSTR)Field[0]) == INFCHAR_IRQATTR_LEVEL)
            ||   (TCHAR)CharUpper((PTSTR)Field[1]) == INFCHAR_IRQATTR_LEVEL) {

                Level = TRUE;
            }
            Field+=3;
        }

        if(pDecimalToUlong(Field,&Irq)) {

            //
            // Slam values into the header part of the irq descriptor.
            // These will be ignored unless we're setting a forced config.
            //
            IrqRes->IRQ_Header.IRQD_Flags = Shareable ? fIRQD_Share : fIRQD_Exclusive;
            IrqRes->IRQ_Header.IRQD_Flags |= Level ? fIRQD_Level : fIRQD_Edge;
            IrqRes->IRQ_Header.IRQD_Alloc_Num = Irq;
            IrqRes->IRQ_Header.IRQD_Affinity = DEFAULT_IRQ_AFFINITY;

            //
            // Add this guy into the descriptor we're building up.
            //
            q = MyRealloc(
                    IrqRes,
                      offsetof(IRQ_RESOURCE,IRQ_Data)
                    + (sizeof(IRQ_RANGE)*(IrqRes->IRQ_Header.IRQD_Count+1))
                    );

            if(q) {
                IrqRes = q;
                IrqRange = &IrqRes->IRQ_Data[IrqRes->IRQ_Header.IRQD_Count++];

                IrqRange->IRQR_Min = Irq;
                IrqRange->IRQR_Max = Irq;
                IrqRange->IRQR_Flags = Shareable ? fIRQD_Share : fIRQD_Exclusive;
                IrqRange->IRQR_Flags |= Level ? fIRQD_Level : fIRQD_Edge;

            } else {
                d = ERROR_NOT_ENOUGH_MEMORY;
            }
        } else {
            d = ERROR_INVALID_INF_LOGCONFIG;
        }
    }

    if((d == NO_ERROR) && IrqRes->IRQ_Header.IRQD_Count) {

        d = CM_Add_Res_Des_Ex(
                &ResDes,
                LogConfig,
                ResType_IRQ,
                IrqRes,
                offsetof(IRQ_RESOURCE,IRQ_Data) + (sizeof(IRQ_RANGE) * IrqRes->IRQ_Header.IRQD_Count),
                0,
                hMachine);

        d = MapCrToSpError(d, ERROR_INVALID_DATA);

        if(d == NO_ERROR) {
            CM_Free_Res_Des_Handle(ResDes);
        }
    }

    if(IrqRes) {
        MyFree(IrqRes);
    }

    return(d);
}


DWORD
pSetupProcessDmaConfig(
    IN LOG_CONF    LogConfig,
    IN PINFCONTEXT InfLine,
    IN HMACHINE    hMachine
    )

/*++

Routine Description:

    Process a DMAConfig line in a Win95 INF. Such lines specify
    DMA requirements for a device. Each line is expected to be
    in the form

    DMAConfig = [<attrs>:]<DMANum>,...

    if <attrs> is present it can be
        D - 32-bit DMA channel
        W - 16-bit DMA channel
        N - 8-bit DMA channel (default).  Specify both W and N if 8- and 16-bit DMA is supported.
        M - Bus Mastering
        A - Type-A DMA channel
        B - Type-B DMA channel
        F - Type-F DMA channel
        (If none of A, B, or F are specified, then standard DMA is assumed)

    DMANum is the DMA channel number in decimal.

Arguments:

Return Value:

--*/

{
    UINT FieldCount,i;
    PCTSTR Field;
    DWORD d;
    DWORD Dma;
    INT ChannelSize;       // fDD_ xxx flags for channel width
    INT DmaType;           // fDD_ xxx flags for DMA type
    PDMA_RESOURCE DmaRes;
    PDMA_RANGE DmaRange;
    RES_DES ResDes;
    PVOID q;
    PTCHAR p;
    BOOL BusMaster;
    ULONG DmaFlags;

    ChannelSize = -1;
    BusMaster = FALSE;
    DmaType = -1;

    FieldCount = SetupGetFieldCount(InfLine);
    if (!FieldCount && GetLastError() != NO_ERROR) {
        return GetLastError();
    }

    if(DmaRes = MyMalloc(offsetof(DMA_RESOURCE,DMA_Data))) {

        ZeroMemory(DmaRes,offsetof(DMA_RESOURCE,DMA_Data));
        DmaRes->DMA_Header.DD_Type = DType_Range;

        d = NO_ERROR;

    } else {
        d = ERROR_NOT_ENOUGH_MEMORY;
    }

    for(i=1; (d==NO_ERROR) && (i<=FieldCount); i++) {

        Field = pSetupGetField(InfLine,i);

        //
        // For first field, see if we have attribute spec.
        //
        if(i == 1) {

            if(p = _tcschr(Field, INFCHAR_IRQATTR_SEP)) {

                for( ;((d == NO_ERROR) && (Field < p)); Field++) {

                    switch((TCHAR)CharUpper((PTSTR)(*Field))) {

                        //
                        // Channel size can be both 8 and 16 (i.e., both 'W' and 'N'), but
                        // you can't mix these with 'D'.
                        //
                        case INFCHAR_DMAWIDTH_WORD:
                            if(ChannelSize == fDD_DWORD) {
                                d = ERROR_INVALID_INF_LOGCONFIG;
                            } else if(ChannelSize == fDD_BYTE) {
                                ChannelSize = fDD_BYTE_AND_WORD;
                            } else {
                                ChannelSize = fDD_WORD;
                            }
                            break;

                        case INFCHAR_DMAWIDTH_DWORD:
                            if((ChannelSize != -1) && (ChannelSize != fDD_DWORD)) {
                                d = ERROR_INVALID_INF_LOGCONFIG;
                            } else {
                                ChannelSize = fDD_DWORD;
                            }
                            break;

                        case INFCHAR_DMAWIDTH_NARROW:
                            if(ChannelSize == fDD_DWORD) {
                                d = ERROR_INVALID_INF_LOGCONFIG;
                            } else if(ChannelSize == fDD_WORD) {
                                ChannelSize = fDD_BYTE_AND_WORD;
                            } else {
                                ChannelSize = fDD_BYTE;
                            }
                            break;

                        case INFCHAR_DMA_BUSMASTER:
                            BusMaster = TRUE;
                            break;

                        //
                        // The DMA types are mutually exclusive...
                        //
                        case INFCHAR_DMATYPE_A:
                            if((DmaType != -1) && (DmaType != fDD_TypeA)) {
                                d = ERROR_INVALID_INF_LOGCONFIG;
                            } else {
                                DmaType = fDD_TypeA;
                            }
                            break;

                        case INFCHAR_DMATYPE_B:
                            if((DmaType != -1) && (DmaType != fDD_TypeB)) {
                                d = ERROR_INVALID_INF_LOGCONFIG;
                            } else {
                                DmaType = fDD_TypeB;
                            }
                            break;

                        case INFCHAR_DMATYPE_F:
                            if((DmaType != -1) && (DmaType != fDD_TypeF)) {
                                d = ERROR_INVALID_INF_LOGCONFIG;
                            } else {
                                DmaType = fDD_TypeF;
                            }
                            break;

                        default:
                            d = ERROR_INVALID_INF_LOGCONFIG;
                            break;
                    }
                }

                Field++;    // skip over separator character
            }

            if(ChannelSize == -1) {
                DmaFlags = fDD_BYTE; // default is 8-bit DMA
            } else {
                DmaFlags = (ULONG)ChannelSize;
            }

            if(BusMaster) {
                DmaFlags |= fDD_BusMaster;
            }

            if(DmaType != -1) {
                DmaFlags |= DmaType;
            }
        }

        if(d == NO_ERROR) {
            if(pDecimalToUlong(Field,&Dma)) {

                //
                // Slam values into the header part of the dma descriptor.
                // These will be ignored unless we're setting a forced config.
                //
                DmaRes->DMA_Header.DD_Flags = DmaFlags;
                DmaRes->DMA_Header.DD_Alloc_Chan = Dma;

                //
                // Add this guy into the descriptor we're building up.
                //
                q = MyRealloc(
                        DmaRes,
                          offsetof(DMA_RESOURCE,DMA_Data)
                        + (sizeof(DMA_RANGE)*(DmaRes->DMA_Header.DD_Count+1))
                        );

                if(q) {
                    DmaRes = q;
                    DmaRange = &DmaRes->DMA_Data[DmaRes->DMA_Header.DD_Count++];

                    DmaRange->DR_Min = Dma;
                    DmaRange->DR_Max = Dma;
                    DmaRange->DR_Flags = DmaFlags;

                } else {
                    d = ERROR_NOT_ENOUGH_MEMORY;
                }
            } else {
                d = ERROR_INVALID_INF_LOGCONFIG;
            }
        }
    }

    if((d == NO_ERROR) && DmaRes->DMA_Header.DD_Count) {

        d = CM_Add_Res_Des_Ex(
                &ResDes,
                LogConfig,
                ResType_DMA,
                DmaRes,
                offsetof(DMA_RESOURCE,DMA_Data) + (sizeof(DMA_RANGE) * DmaRes->DMA_Header.DD_Count),
                0,
                hMachine);

        d = MapCrToSpError(d, ERROR_INVALID_DATA);

        if(d == NO_ERROR) {
            CM_Free_Res_Des_Handle(ResDes);
        }
    }

    if(DmaRes) {
        MyFree(DmaRes);
    }

    return(d);
}


DWORD
pSetupProcessPcCardConfig(
    IN LOG_CONF    LogConfig,
    IN PINFCONTEXT InfLine,
    IN HMACHINE    hMachine
    )

/*++

Routine Description:

    Process a PcCardConfig line in a Win95 INF.  Such lines specify
    PC Card (PCMCIA) configuration information necessary for a device.
    Each line is expected to be in the form

    PcCardConfig = <ConfigIndex>[:[<MemoryCardBase1>][:<MemoryCardBase2>]][(<attrs>)]

    where

        <ConfigIndex> is the 8-bit PCMCIA configuration index

        <MemoryCardBase1> is the (optional) 32-bit 1st memory base address

        <MemoryCardBase2> is the (optional) 32-bit 2nd memory base address

        <attrs> is a combination of attribute specifiers optionally separated by
                spaces. The attribute string is processed from left to right,
                and an invalid attribute specifier aborts the entire PcCardConfig
                directive. Attributes may be specified in any order except for the
                positional attributes 'A' and 'C', which are described below.

                Accepted attribute specifiers are as follows:

                W   - 16-bit I/O data path (default: 16-bit)

                Sn  - ~IOCS16 source. If n is zero, ~IOCS16 is based on the value of
                      the datasize bit. If n is one, ~IOCS16 is based on the ~IOIS16
                      signal from the device. (default: 1)

                Zn  - I/O 8-bit zero wait state. If n is one, 8-bit I/O accesses occur
                      with zero additional wait states. If n is zero, access will
                      occur with additional wait states. This flag has no meaning for
                      16-bit I/O. (default: 0)

                XIn - I/O wait states. If n is one, 16-bit system accesses occur with
                      1 additional wait state. (default: 1)

                M   - 16-bit Memory (default: 8-bit)

                XMn - Memory wait states, where n can be 0, 1, 2 or 3. This value
                      determines the number of additional wait states for 16-bit
                      accesses to a memory window. (default: 3)

                    NOTE: The following two attributes relate positionally to memory
                    windows resources. That is, the first 'A' or 'C' specified in the
                    attribute string (reading from left to right) corresponds to the
                    first memory resource in the device's resource list. The next
                    'A' or 'C' corresponds to the second memory resource. Subsequent
                    attribute/common memory specifiers are ignored.

                A - Memory range to be mapped as Attribute memory
                C - Memory range to be mapped as Common Memory (default)

                Example:
                (W CA M XM1 XI0) translates to:
                    I/O 16bit
                    1st memory window is common
                    2nd memory window is attribute
                    Memory 16 bit
                    one wait state on memory windows
                    zero wait states on i/o windows

    All numeric values are assumed to be in hexadecimal format.

Arguments:

Return Value:

--*/

{
    PCCARD_RESOURCE PcCardResource;
    PCTSTR Field, p;
    DWORD ConfigIndex, i, d;
    DWORD MemoryCardBase[2];
    DWORD IoIs8or16Bits, MemIs8or16Bits;
    DWORD IoSource16, IoZeroWait8, IoWaitState16, MemWaitState16;
    DWORD MemBaseAttributes;
    RES_DES ResDes;

    //
    // Assume failure
    //
    d = ERROR_INVALID_INF_LOGCONFIG;

    //
    // We should have one field (not counting the line's key)
    //
    if(SetupGetFieldCount(InfLine) != 1) {
        goto clean0;
    } else {
        Field = pSetupGetField(InfLine, 1);
    }

    //
    // Retrieve the ConfigIndex.  It may be terminated by either a colon ':',
    // an open paren '(', or eol.
    //
    if(!(p = _tcschr(Field, INFCHAR_PCCARD_SEP)) && !(p = _tcschr(Field, INFCHAR_ATTR_START))) {
        p = Field + lstrlen(Field);
    }

    if(!pHexToUlong(Field, p, &ConfigIndex) || (ConfigIndex > 255)) {
        goto clean0;
    }

    //
    // Process the two (optional) memory card base addresses
    //
    for(i = 0; i < 2; i++) {

        if(*p == INFCHAR_PCCARD_SEP) {

            Field = p + 1;
            if(!(p = _tcschr(Field, INFCHAR_PCCARD_SEP)) && !(p = _tcschr(Field, INFCHAR_ATTR_START))) {
                p = Field + lstrlen(Field);
            }

            //
            // Allow an empty field.
            //
            if(Field == p) {
                MemoryCardBase[i] = 0;
            } else if(!pHexToUlong(Field, p, &(MemoryCardBase[i]))) {
                goto clean0;
            }

        } else {
            MemoryCardBase[i] = 0;
        }
    }

    //
    // Default to 8-bit I/O unless otherwise specified...
    //
    IoIs8or16Bits = fPCD_IO_8;
    //
    // Default to 8-bit memory unless otherwise specified...
    //
    MemIs8or16Bits = fPCD_MEM_8;
    //
    // Default to both memory windows being common memory
    //
    MemBaseAttributes = 0;

    IoSource16 = fPCD_IO_SRC_16;
    IoZeroWait8 = 0;
    IoWaitState16 = fPCD_IO_WS_16;
    MemWaitState16 = fPCD_MEM_WS_THREE;

    if(*p && (*p == INFCHAR_ATTR_START)) {
        UINT memindex = 1;

        //
        // Read the attributes.
        //  W   - 16-bit I/O data path
        //  Sn  - ~IOCS16 source.
        //  Zn  - I/O 8-bit zero wait state.
        //  XIn - I/O wait states.
        //  M   - 16-bit Memory
        //  XMn - Memory wait states
        //  A   - Attribute Memory
        //  C   - Common Memory
        //

        Field = ++p;
        if(!(p = _tcschr(Field,INFCHAR_ATTR_END))) {
            goto clean0;
        }

        while(Field < p) {

            switch((TCHAR)CharUpper((PTSTR)(*Field))) {

            case INFCHAR_PCCARD_IOATTR_WORD:
                IoIs8or16Bits = fPCD_IO_16;
                break;

            case INFCHAR_PCCARD_MEMATTR_WORD:
                MemIs8or16Bits = fPCD_MEM_16;
                break;

            case INFCHAR_PCCARD_MEM_ISATTR:
                if (memindex == 1) {
                    MemBaseAttributes |= fPCD_MEM1_A;
                } else if (memindex == 2) {
                    MemBaseAttributes |= fPCD_MEM2_A;
                }
                memindex++;
                break;

            case INFCHAR_PCCARD_MEM_ISCOMMON:
                //
                // only need to increment index, since common is default
                //
                memindex++;
                break;

            case INFCHAR_PCCARD_IOATTR_SRC:
                if (++Field < p) {
                    if (*Field == TEXT('0')) {
                        IoSource16 = 0;
                    } else if (*Field == TEXT('1')) {
                        IoSource16 = fPCD_IO_SRC_16;
                    } else {
                        goto clean0;
                    }
                }
                break;

            case INFCHAR_PCCARD_IOATTR_Z8:
                if (++Field < p) {
                    if (*Field == TEXT('0')) {
                        IoZeroWait8 = 0;
                    } else if (*Field == TEXT('1')) {
                        IoZeroWait8 = fPCD_IO_ZW_8;
                    } else {
                        goto clean0;
                    }
                }
                break;

            case INFCHAR_PCCARD_ATTR_WAIT:
                if (++Field < p) {

                    switch((TCHAR)CharUpper((PTSTR)(*Field))) {

                    case INFCHAR_PCCARD_ATTR_WAITI:
                        if (++Field < p) {
                            if (*Field == TEXT('0')) {
                                IoWaitState16 = 0;
                            } else if (*Field == TEXT('1')) {
                                IoWaitState16 = fPCD_IO_WS_16;
                            } else {
                                goto clean0;
                            }
                        }
                        break;

                    case INFCHAR_PCCARD_ATTR_WAITM:
                        if (++Field < p) {
                            if (*Field == TEXT('0')) {
                                MemWaitState16 = 0;
                            } else if (*Field == TEXT('1')) {
                                MemWaitState16 = fPCD_MEM_WS_ONE;
                            } else if (*Field == TEXT('2')) {
                                MemWaitState16 = fPCD_MEM_WS_TWO;
                            } else if (*Field == TEXT('3')) {
                                MemWaitState16 = fPCD_MEM_WS_THREE;
                            } else {
                                goto clean0;
                            }
                        }
                        break;

                    default:
                        goto clean0;
                    }
                }
                break;

            case INFCHAR_PCCARD_ATTR_SEP:
                break;

            default:
                // unknown character
                goto clean0;
            }
            if (Field < p) {
                Field++;
            }
        }
    }

    //
    // If we get to here, then we've successfully retrieved all the necessary information
    // needed to initialize the PC Card configuration resource descriptor.
    //
    ZeroMemory(&PcCardResource, sizeof(PcCardResource));

    PcCardResource.PcCard_Header.PCD_Count = 1;
    PcCardResource.PcCard_Header.PCD_Flags = IoIs8or16Bits | MemIs8or16Bits | MemBaseAttributes |
                                             IoSource16 | IoZeroWait8 | IoWaitState16 | MemWaitState16;
    PcCardResource.PcCard_Header.PCD_ConfigIndex = (BYTE)ConfigIndex;
    PcCardResource.PcCard_Header.PCD_MemoryCardBase1 = MemoryCardBase[0];
    PcCardResource.PcCard_Header.PCD_MemoryCardBase2 = MemoryCardBase[1];

    d = CM_Add_Res_Des_Ex(
            &ResDes,
            LogConfig,
            ResType_PcCardConfig,
            &PcCardResource,
            sizeof(PcCardResource),
            0,
            hMachine);

    d = MapCrToSpError(d, ERROR_INVALID_DATA);

    if(d == NO_ERROR) {
        CM_Free_Res_Des_Handle(ResDes);
    }

clean0:
    return d;
}


DWORD
pSetupProcessMfCardConfig(
    IN LOG_CONF    LogConfig,
    IN PINFCONTEXT InfLine,
    IN HMACHINE    hMachine
    )

/*++

Routine Description:

    Process a MfCardConfig line in a Win95 INF.  Such lines specify
    PCMCIA Multifunction card configuration information necessary for a device.
    There should normally be one MfCardConfig line per function. Each line is expected
    to be in the form:

    MfCardConfig = <ConfigRegBase>:<ConfigOptions>[:<IoResourceIndex>][(<attrs>)]

    where

        <ConfigRegBase> is the attribute offset of this function's
                        configuration registers

        <ConfigOptions> is the 8-bit PCMCIA configuration option register

        <IoResourceIndex> is the (optional) index to the Port Io resource descriptor
                          which will be used to program the configuration I/O base
                          and limit registers

        <attrs> is the optional set of attribute flags which can consist of:

                A - Audio enable should be set on in the configuration and status register

    All numeric values are assumed to be in hexadecimal format.

Arguments:

Return Value:

--*/

{
    MFCARD_RESOURCE MfCardResource;
    PCTSTR Field, p;
    DWORD ConfigOptions, i;
    DWORD ConfigRegisterBase, IoResourceIndex;
    DWORD Flags = 0;
    DWORD d = NO_ERROR;
    RES_DES ResDes;


    //
    // We should have one field (not counting the line's key)
    //
    if(SetupGetFieldCount(InfLine) != 1) {
        d = ERROR_INVALID_INF_LOGCONFIG;
    }

    if (d == NO_ERROR) {
        //
        // Retrieve the ConfigRegisterBase. It must be terminated by a colon.
        //
        Field = pSetupGetField(InfLine, 1);

        p = _tcschr(Field, INFCHAR_PCCARD_SEP);

        if(!p || !pHexToUlong(Field, p, &ConfigRegisterBase)) {
            d = ERROR_INVALID_INF_LOGCONFIG;
        }
    }

    if (d == NO_ERROR) {
        //
        // Retrieve the ConfigOptions.  It may be terminated by either a colon ':',
        // an open paren '(', or eol.
        //
        Field = p + 1;

        if(!(p = _tcschr(Field, INFCHAR_PCCARD_SEP)) && !(p = _tcschr(Field, INFCHAR_ATTR_START))) {
            p = Field + lstrlen(Field);
        }

        if(!pHexToUlong(Field, p, &ConfigOptions) || (ConfigOptions > 255)) {
            d = ERROR_INVALID_INF_LOGCONFIG;
        }
    }

    if ((d == NO_ERROR) && (*p == INFCHAR_PCCARD_SEP)) {
        //
        // Retrieve the IoResourceIndex. It may be terminated by either
        // an open paren '(', or eol.
        //

        Field = p + 1;
        if(!(p = _tcschr(Field, INFCHAR_ATTR_START))) {
            p = Field + lstrlen(Field);
        }
        if(!pHexToUlong(Field, p, &IoResourceIndex) || (IoResourceIndex > 255)) {
            d = ERROR_INVALID_INF_LOGCONFIG;
        }
    }


    if ((d == NO_ERROR) && (*p == INFCHAR_ATTR_START)) {
        //
        // Retrieve the attributes.
        //
        while (TRUE) {
            p++;

            if (!*p) {
                // Didn't find a close paren
                d = ERROR_INVALID_INF_LOGCONFIG;
                break;
            }

            if (*p == INFCHAR_ATTR_END) {
                if (*(p+1)) {
                    // found garbage after the close paren
                    d = ERROR_INVALID_INF_LOGCONFIG;
                }
                break;
            }

            if ((TCHAR)CharUpper((PTSTR)*p) == INFCHAR_MFCARD_AUDIO_ATTR) {
                Flags |= fPMF_AUDIO_ENABLE;
            } else {
                // bad flag
                d = ERROR_INVALID_INF_LOGCONFIG;
                break;
            }
        }
    }

    if(d == NO_ERROR) {
        //
        // If we get to here, then we've successfully retrieved all the necessary information
        // needed to initialize the multifunction PC Card configuration resource descriptor.
        //
        ZeroMemory(&MfCardResource, sizeof(MfCardResource));

        MfCardResource.MfCard_Header.PMF_Count = 1;
        MfCardResource.MfCard_Header.PMF_Flags = Flags;
        MfCardResource.MfCard_Header.PMF_ConfigOptions = (BYTE)ConfigOptions;
        MfCardResource.MfCard_Header.PMF_IoResourceIndex = (BYTE)IoResourceIndex;
        MfCardResource.MfCard_Header.PMF_ConfigRegisterBase = ConfigRegisterBase;

        d = CM_Add_Res_Des_Ex(
                &ResDes,
                LogConfig,
                ResType_MfCardConfig,
                &MfCardResource,
                sizeof(MfCardResource),
                0,
                hMachine);

        d = MapCrToSpError(d, ERROR_INVALID_DATA);

        if(d == NO_ERROR) {
            CM_Free_Res_Des_Handle(ResDes);
        }
    }
    return d;
}

#if 0
DWORD
pSetupProcessLogConfigLines(
    IN PVOID    Inf,
    IN PCTSTR   SectionName,
    IN PCTSTR   KeyName,
    IN DWORD    (*CallbackFunc)(LOG_CONFIG,PINFCONTEXT,HMACHINE),
    IN LOG_CONF LogConfig,
    IN HMACHINE hMachine
    )
{
    BOOL b;
    DWORD d;
    INFCONTEXT InfLine;

    b = SetupFindFirstLine(Inf,SectionName,KeyName,&InfLine);
    d = NO_ERROR;
    //
    // Process each line with a key that matches.
    //
    while(b && (d == NO_ERROR)) {

        d = CallbackFunc(LogConfig,&InfLine, hMachine);

        if(d == NO_ERROR) {
            b = SetupFindNextMatchLine(&InfLine,KeyName,&InfLine);
        }
    }

    return(d);
}

#endif

DWORD
pSetupProcessConfigPriority(
    IN  PVOID     Inf,
    IN  PCTSTR    SectionName,
    IN  LOG_CONF  LogConfig,
    OUT PRIORITY *PriorityValue,
    OUT DWORD    *ConfigType,
    IN  DWORD     Flags
    )
{
    INFCONTEXT InfLine;
    PCTSTR PrioritySpec;
    PCTSTR ConfigSpec;
    DWORD d = NO_ERROR;

    //
    // We only need to fetch one of these lines and look at the
    // first value on it.
    //
    if(SetupFindFirstLine(Inf,SectionName,pszConfigPriority,&InfLine)
       && (PrioritySpec = pSetupGetField(&InfLine,1))) {

        if(!LookUpStringInTable(InfPrioritySpecToPriority,PrioritySpec,PriorityValue)) {
            d = ERROR_INVALID_INF_LOGCONFIG;
        } else {
            //
            // The second value is optional and specifies whether the config is forced,
            // standard (i.e., basic), or override. If the value isn't specified then
            // assume basic, unless the Flags tell us otherwise.
            //
            ConfigSpec = pSetupGetField(&InfLine,2);
            if(!ConfigSpec || !*ConfigSpec) {

                if(Flags & SPINST_LOGCONFIG_IS_FORCED) {
                    *ConfigType = FORCED_LOG_CONF;
                } else if(Flags & SPINST_LOGCONFIGS_ARE_OVERRIDES) {
                    *ConfigType = OVERRIDE_LOG_CONF;
                } else {
                    *ConfigType = BASIC_LOG_CONF;
                }

            } else {

                if(LookUpStringInTable(InfConfigSpecToConfig, ConfigSpec, ConfigType)) {
                    //
                    // A valid ConfigType was specified.  Let's make sure it doesn't disagree
                    // with any flags that were passed in to this routine.
                    //
                    if(Flags & SPINST_LOGCONFIG_IS_FORCED) {
                        if(*ConfigType != FORCED_LOG_CONF) {
                            d = ERROR_INVALID_INF_LOGCONFIG;
                        }
                    } else if(Flags & SPINST_LOGCONFIGS_ARE_OVERRIDES) {
                        if(*ConfigType != OVERRIDE_LOG_CONF) {
                            d = ERROR_INVALID_INF_LOGCONFIG;
                        }
                    }

                } else {
                    d = ERROR_INVALID_INF_LOGCONFIG;
                }
            }
        }

        //
        // If we successfully determined the LogConfig type as FORCED_LOG_CONF, then
        // set the priority to LCPRI_FORCECONFIG.
        //
        if((d == NO_ERROR) && (*ConfigType == FORCED_LOG_CONF)) {
            *PriorityValue = LCPRI_FORCECONFIG;
        }


    } else {

        *PriorityValue = (Flags & SPINST_LOGCONFIG_IS_FORCED) ? LCPRI_FORCECONFIG : LCPRI_NORMAL;

        if(Flags & SPINST_LOGCONFIG_IS_FORCED) {
            *ConfigType = FORCED_LOG_CONF;
        } else if(Flags & SPINST_LOGCONFIGS_ARE_OVERRIDES) {
            *ConfigType = OVERRIDE_LOG_CONF;
        } else {
            *ConfigType = BASIC_LOG_CONF;
        }
    }

    return d;
}


DWORD
pSetupProcessLogConfigSection(
    IN PVOID   Inf,
    IN PCTSTR  SectionName,
    IN DEVINST DevInst,
    IN DWORD   Flags,
    IN HMACHINE hMachine
    )
{
    DWORD d;
    LOG_CONF LogConfig;
    PRIORITY Priority;
    DWORD ConfigType;
    CONFIGRET cr;

    DWORD LineIndex = 0;
    INFCONTEXT InfLine;
    TCHAR Key[MAX_LOGCONFKEYSTR_LEN];

    //
    // Process config priority values.
    //

    //
    // BUGBUG:  LogConfig is used before initialized in following call.
    //          It doesn't matter to function being called, but 6.0 compiler
    //          doesn't like it, so init to 0.
    //

    LogConfig = 0;

    d = pSetupProcessConfigPriority(Inf,SectionName,LogConfig,&Priority,&ConfigType,Flags);
    if(d != NO_ERROR) {
        goto c0;
    }

    //
    // Now that we know the priority we can create an empty log config.
    //
    d = MapCrToSpError(CM_Add_Empty_Log_Conf_Ex(&LogConfig,DevInst,Priority,ConfigType,hMachine),
                       ERROR_INVALID_DATA
                      );

    if(d != NO_ERROR) {
        goto c0;
    }

    //
    // Iterate over the lines in the section adding entries to the log config in
    // the same order as they are found.
    //

    if (SetupFindFirstLine(Inf,SectionName,NULL,&InfLine)) {

        do {

            //
            // Get the key.
            //

            if (!SetupGetStringField(&InfLine,
                                     0, // Index 0 is the key field
                                     Key,
                                     MAX_LOGCONFKEYSTR_LEN,
                                     NULL
                                     )) {
                //
                // Either we didn't have a key or its longer than the longest
                // valid key - either way its invalid
                //

                d = ERROR_INVALID_INF_LOGCONFIG;
                goto c1;
            }

            if (!lstrcmpi(Key, pszMemConfig)) {

                //
                // Process MemConfig lines
                //

                d = pSetupProcessMemConfig(LogConfig, &InfLine, hMachine);

            } else if (!lstrcmpi(Key, pszIOConfig)) {

                //
                // Process IoConfig lines
                //

                d = pSetupProcessIoConfig(LogConfig, &InfLine, hMachine);

            } else if (!lstrcmpi(Key, pszIRQConfig)) {

                //
                // Process IRQConfig lines
                //

                d = pSetupProcessIrqConfig(LogConfig, &InfLine, hMachine);

            } else if (!lstrcmpi(Key, pszDMAConfig)) {

                //
                // Process DMAConfig lines
                //

                d = pSetupProcessDmaConfig(LogConfig, &InfLine, hMachine);

            } else if (!lstrcmpi(Key, pszPcCardConfig)) {

                //
                // Process PcCardConfig lines
                //

                d = pSetupProcessPcCardConfig(LogConfig, &InfLine, hMachine);

            } else if (!lstrcmpi(Key, pszMfCardConfig)) {

                //
                // Process MfCardConfig lines
                //

                d = pSetupProcessMfCardConfig(LogConfig, &InfLine, hMachine);

            } else {

                //
                // If we don't understand the line skip it
                //

                d = NO_ERROR;
            }

        } while (d == NO_ERROR && SetupFindNextMatchLine(&InfLine,NULL,&InfLine));
    }

#if 0
    //
    // Process MemConfig lines
    //
    d = pSetupProcessLogConfigLines(
            Inf,
            SectionName,
            pszMemConfig,
            pSetupProcessMemConfig,
            LogConfig,
            hMachine
            );

    if(d != NO_ERROR) {
        goto c1;
    }

    //
    // Process IOConfig lines
    //
    d = pSetupProcessLogConfigLines(
            Inf,
            SectionName,
            pszIOConfig,
            pSetupProcessIoConfig,
            LogConfig,
            hMachine
            );

    if(d != NO_ERROR) {
        goto c1;
    }

    //
    // Process IRQConfig lines
    //
    d = pSetupProcessLogConfigLines(
            Inf,
            SectionName,
            pszIRQConfig,
            pSetupProcessIrqConfig,
            LogConfig,
            hMachine
            );

    if(d != NO_ERROR) {
        goto c1;
    }

    //
    // Process DMAConfig lines
    //
    d = pSetupProcessLogConfigLines(
            Inf,
            SectionName,
            pszDMAConfig,
            pSetupProcessDmaConfig,
            LogConfig,
            hMachine
            );

    if(d != NO_ERROR) {
        goto c1;
    }

    //
    // Process PcCardConfig lines
    //
    d = pSetupProcessLogConfigLines(
            Inf,
            SectionName,
            pszPcCardConfig,
            pSetupProcessPcCardConfig,
            LogConfig,
            hMachine
            );
#endif

c1:
    if(d != NO_ERROR) {
        CM_Free_Log_Conf(LogConfig,0);
    }
    CM_Free_Log_Conf_Handle(LogConfig);
c0:
    return(d);
}


DWORD
pSetupInstallLogConfig(
    IN HINF    Inf,
    IN PCTSTR  SectionName,
    IN DEVINST DevInst,
    IN DWORD   Flags,
    IN HMACHINE hMachine
    )

/*++

Routine Description:

    Look for logical configuration directives within an inf section
    and parse them. Each value on the LogConf= line is taken to be
    the name of a logical config section.

Arguments:

    Inf - supplies inf handle for inf containing the section indicated
        by SectionName.

    SectionName - supplies name of install section.

    DevInst - device instance handle for log configs.

    Flags - supplies flags that modify the behavior of this routine.  The
        following flags are payed attention to, everything else is ignored:

        SPINST_SINGLESECTION - if this bit is set, then the specified section
                               is a LogConf section, instead of an install
                               section containing LogConf entries.

        SPINST_LOGCONFIG_IS_FORCED - if this bit is set, then the LogConfigs
                                     to be written out are forced configs.
                                     If the ConfigType field of the ConfigPriority
                                     entry is present, and specifies something
                                     other than FORCED, this routine will fail
                                     with ERROR_INVALID_INF_LOGCONFIG.

        SPINST_LOGCONFIGS_ARE_OVERRIDES - if this bit is set, then the LogConfigs
                                          to be written out are override configs.
                                          If the ConfigType field of the ConfigPriority
                                          entry is present, and specifies something
                                          other than OVERRIDE, this routine will fail
                                          with ERROR_INVALID_INF_LOGCONFIG.

Return Value:

    Win32 error code indicating outcome.

--*/

{
    INFCONTEXT LineContext;
    DWORD rc = NO_ERROR;
    DWORD FieldCount;
    DWORD Field;
    PCTSTR SectionSpec;

    if(Flags & SPINST_SINGLESECTION) {
        //
        // Process the specific LogConf section the caller specified.
        //
        if(SetupGetLineCount(Inf, SectionName) == -1) {
            rc = ERROR_SECTION_NOT_FOUND;
        } else {
            rc = pSetupProcessLogConfigSection(Inf, SectionName, DevInst, Flags,hMachine);
        }
    } else {
        //
        // Find the relevant line in the given install section.
        // If not present then we're done with this operation.
        //
        if(SetupFindFirstLine(Inf,SectionName,SZ_KEY_LOGCONFIG,&LineContext)) {

            do {
                //
                // Each value on the line in the given install section
                // is the name of a logical config section.
                //
                FieldCount = SetupGetFieldCount(&LineContext);
                for(Field=1; (rc==NO_ERROR) && (Field<=FieldCount); Field++) {

                    if((SectionSpec = pSetupGetField(&LineContext,Field))
                    && (SetupGetLineCount(Inf,SectionSpec) > 0)) {

                        rc = pSetupProcessLogConfigSection(Inf,SectionSpec,DevInst,Flags,hMachine);
                    } else {
                        rc = ERROR_SECTION_NOT_FOUND;
                    }
                    if (rc != NO_ERROR) {
                        pSetupLogSectionError(Inf,NULL,NULL,SectionSpec,MSG_LOG_SECT_ERROR,rc,SZ_KEY_LOGCONFIG);
                    }
                }

            } while((rc == NO_ERROR) && SetupFindNextMatchLine(&LineContext,SZ_KEY_LOGCONFIG,&LineContext));
        }
    }

    return rc;
}

