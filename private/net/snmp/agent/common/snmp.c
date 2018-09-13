/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    snmp.c

Abstract:

    Contains routines and definitions that are independant to any SNMP API.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/

#ifdef _SNMPDLL_
//--------------------------- WINDOWS DEPENDENCIES --------------------------

#ifndef DOS
#include <nt.h>
#include <windef.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#endif

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <winsock.h>
#include <wsipx.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <ctype.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

#include <snmp.h>
#include <snmputil.h>
#include <evtlog.h>
#include <wellknow.h>
#include <regconf.h>

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

DWORD g_uptimeReference = 0;
DWORD g_platformId = 0;

INT g_nLogType  = SNMP_OUTPUT_TO_DEBUGGER;  
INT g_nLogLevel = SNMP_LOG_SILENT;    

AsnObjectIdentifier * g_enterprise = NULL;

#if DBG
DWORD g_nBytesTotal = 0;
#endif

//--------------------------- PRIVATE CONSTANTS -----------------------------

#define MAX_DEBUG_LEN 512

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

static UINT idsWindowsNTWorkstation[] = {1,3,6,1,4,1,311,1,1,3,1,1};
static UINT idsWindowsNTServer[]      = {1,3,6,1,4,1,311,1,1,3,1,2};
static UINT idsWindowsNTDC[]          = {1,3,6,1,4,1,311,1,1,3,1,3};
static UINT idsWindows[]              = {1,3,6,1,4,1,311,1,1,3,2};

static AsnObjectIdentifier oidWindowsNTWorkstation = { 
    sizeof(idsWindowsNTWorkstation)/sizeof(UINT), 
    idsWindowsNTWorkstation 
    };

static AsnObjectIdentifier oidWindowsNTServer = { 
    sizeof(idsWindowsNTServer)/sizeof(UINT), 
    idsWindowsNTServer 
    };

static AsnObjectIdentifier oidWindowsNTDC = { 
    sizeof(idsWindowsNTDC)/sizeof(UINT), 
    idsWindowsNTDC 
    };

static AsnObjectIdentifier oidWindows = { 
    sizeof(idsWindows)/sizeof(UINT), 
    idsWindows 
    };

//--------------------------- PRIVATE MACROS --------------------------------

#define bcopy(slp, dlp, size)   (void)memcpy(dlp, slp, size)

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

AsnObjectIdentifier *
InitializeEnterpriseOID(
    )
{
    // default to generic windows enterprise oid
    AsnObjectIdentifier * enterprise = &oidWindows;

    // check to see if the platform is winnt
    if (g_platformId == VER_PLATFORM_WIN32_NT) {

        HMODULE NtDllHandle = NULL;
        FARPROC lpfnRtlGetNtProductType = NULL;
        NT_PRODUCT_TYPE NtProductType;

        // assume this is just a workstation        
        enterprise = &oidWindowsNTWorkstation;

        // load the internal utility library
        NtDllHandle = LoadLibrary(TEXT("ntdll.dll"));

        // validate handle        
        if (NtDllHandle) {

            // resolve address
            lpfnRtlGetNtProductType = GetProcAddress(
                                            NtDllHandle,
                                            TEXT("RtlGetNtProductType")
                                            );
            // validate pointer
            if (lpfnRtlGetNtProductType) {

                // let the system determine product type
                (*lpfnRtlGetNtProductType)(&NtProductType);

                // point to the correct enterprise oid
                if (NtProductType == NtProductServer) {

                    // this is a stand-alone server
                    enterprise = &oidWindowsNTServer;

                } else if (NtProductType == NtProductLanManNt) {

                    // this is a PDC or a BDC
                    enterprise = &oidWindowsNTDC;
                }
            }

            // unload library 
            FreeLibrary(NtDllHandle);
        }
    }

    SNMPDBG((
        SNMP_LOG_TRACE, 
        "SNMP: INIT: enterprise is %s.\n", 
        SnmpUtilOidToA(enterprise)
        ));

    return enterprise;
}

//--------------------------- PUBLIC PROCEDURES -----------------------------
#else  // _SNMPDLL_
#include <snmp.h>
#endif // _SNMPDLL_

#ifdef _SNMPDLL_

BOOLEAN
InitializeDLL(
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN LPVOID lpReserved OPTIONAL
    )
{
    WSADATA wsaData;

    if (Reason == DLL_PROCESS_ATTACH) 
        {
        OSVERSIONINFO osInfo;    

        osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

        if (!GetVersionEx(&osInfo))
            return FALSE;

        if ((osInfo.dwPlatformId != VER_PLATFORM_WIN32_NT) &&
            (osInfo.dwPlatformId != VER_PLATFORM_WIN32_WINDOWS))
            return FALSE;

        g_platformId = osInfo.dwPlatformId;
        g_enterprise = InitializeEnterpriseOID();

        DisableThreadLibraryCalls(DllHandle);

        if (WSAStartup(0x0101, &wsaData))
            {
            WSACleanup();
            return FALSE;
            }
        }
    else if (Reason == DLL_PROCESS_DETACH)
        {
        WSACleanup();
        }

    return TRUE;

} // InitializeDLL

//
// SnmpSvcInitUptime
//    Initializes global shared variable.
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
DWORD
SNMP_FUNC_TYPE SnmpSvcInitUptime(
        )
{
    g_uptimeReference = GetCurrentTime();
    return g_uptimeReference;

} // SnmpSvcInitUptime

//
// SnmpSvcGetUptime
//    Determine uptime via global shared variable.
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
DWORD
SNMP_FUNC_TYPE SnmpSvcGetUptime(
        )
{
    return ((GetCurrentTime() - g_uptimeReference) / 10);

} // SnmpSvcGetUptime

//
// SnmpSvcBufRevAndCpy
//    Copy the contents of the specified buffer into the new buffer, and
//    reverse the contents.
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
VOID 
SNMP_FUNC_TYPE SnmpSvcBufRevAndCpy(
        OUT BYTE *szDest,  // Destination buffer
        IN BYTE *szSource, // Source buffer
        IN UINT nLen       // Length of buffers
        )

{
UINT I;


   I = 0;
   while ( I < nLen )
      {
      szDest[I] = szSource[nLen - I - 1];
      I++;
      }
} // SnmpSvcBufRevAndCpy



//
// SnmpSvcBufRevInPlace
//    Reverse the contents of the specified buffer.
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
VOID 
SNMP_FUNC_TYPE SnmpSvcBufRevInPlace(
        IN OUT BYTE *szStr, // Buffer to reverse
        IN UINT nLen        // Length of buffer
        )

{
UINT I;
BYTE nTemp;

   for ( I=0;I < nLen/2;I++ )
      {
      nTemp           = szStr[I];
      szStr[I]        = szStr[nLen - I - 1];
      szStr[nLen - I - 1] = nTemp;
      }
} // SnmpSvcBufRevInPlace

#endif // _SNMPDLL_


//
// SnmpUtilOidCpy
//    Copy an object identifier.
//
// Notes:
//    This routine is responsible for allocating enough memory to contain
//    the new identifier.
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    SNMP_MEM_ALLOC_ERROR
//
SNMPAPI
SNMP_FUNC_TYPE SnmpUtilOidCpy(
           OUT AsnObjectIdentifier *DestObjId, // Destination OID
           IN AsnObjectIdentifier *SrcObjId    // Source OID
           )

{
SNMPAPI nResult;


   // Alloc space for new object id
   if ( NULL ==
        (DestObjId->ids = (UINT *) SnmpUtilMemAlloc((sizeof(UINT) * SrcObjId->idLength))) )
      {
      SetLastError( SNMP_MEM_ALLOC_ERROR );

      nResult = SNMPAPI_ERROR;
      goto Exit;
      }

   // Set length
   DestObjId->idLength = SrcObjId->idLength;

   // Copy the object id elements
   memcpy( DestObjId->ids, SrcObjId->ids, DestObjId->idLength * sizeof(UINT) );

   nResult = SNMPAPI_NOERROR;

Exit:
   return nResult;
} // SnmpUtilOidCpy



//
// SnmpUtilOidAppend
//    Append source OID to destination OID
//
// Notes:
//    If memory cannot be allocated to accommodate the extended OID, then
//    the original OID is lost.
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    SNMP_MEM_ALLOC_ERROR
//    SNMP_BERAPI_OVERFLOW
//
SNMPAPI
SNMP_FUNC_TYPE SnmpUtilOidAppend(
           OUT AsnObjectIdentifier *DestObjId, // Destination OID
           IN AsnObjectIdentifier *SrcObjId    // Source OID
           )

{
SNMPAPI nResult;


   // Check for OID overflow
   if ( (ULONG)DestObjId->idLength +
        (ULONG)SrcObjId->idLength > SNMP_MAX_OID_LEN )
      {
      SetLastError( SNMP_BERAPI_OVERFLOW );

      nResult = SNMPAPI_ERROR;
      goto Exit;
      }

   // Alloc space for new object id
   if ( NULL ==
        (DestObjId->ids = (UINT *) SnmpUtilMemReAlloc(DestObjId->ids, (sizeof(UINT) *
                                    (SrcObjId->idLength+DestObjId->idLength)))) )
      {
      SetLastError( SNMP_MEM_ALLOC_ERROR );

      nResult = SNMPAPI_ERROR;
      goto Exit;
      }

   // Append the source to destination
   memcpy( &DestObjId->ids[DestObjId->idLength],
           SrcObjId->ids, SrcObjId->idLength * sizeof(UINT) );

   // Calculate length
   DestObjId->idLength += SrcObjId->idLength;

   nResult = SNMPAPI_NOERROR;

Exit:
   return nResult;
} // SnmpUtilOidAppend



//
// SnmpUtilOidNCmp
//    Compares two object identifiers.
//
// Notes:
//    The object ids are compared from left to right (starting at the root.)
//    At most the maximum length of OID sub-ids are compared.
//
// Return Codes:
//    < 0   First parameter is 'less' than second.
//      0   Parameters are equal
//    > 0   First parameter is 'greater' than second.
//
// Error Codes:
//    None.
//
SNMPAPI
SNMP_FUNC_TYPE SnmpUtilOidNCmp(
       IN AsnObjectIdentifier *A, // First OID
       IN AsnObjectIdentifier *B, // Second OID
       IN UINT Len                // Max len to compare
       )

{
UINT I;
int  nResult;


   I       = 0;
   nResult = 0;
   while ( !nResult && I < min(Len, min(A->idLength, B->idLength)) )
      {
      nResult = A->ids[I] - B->ids[I++];
      }

   // Check for one being a subset of the other
   if ( !nResult && I < Len )
      {
      nResult = A->idLength - B->idLength;
      }

return nResult;
} // SnmpUtilOidNCmp


//
// SnmpUtilOidCmp
//    Compares two object identifiers.
//
// Notes:
//    The object ids are compared from left to right (starting at the root.)
//
// Return Codes:
//    < 0   First parameter is 'less' than second.
//      0   Parameters are equal
//    > 0   First parameter is 'greater' than second.
//
// Error Codes:
//    None.
//
SNMPAPI
SNMP_FUNC_TYPE SnmpUtilOidCmp(
       IN AsnObjectIdentifier *A, // First OID
       IN AsnObjectIdentifier *B  // Second OID
       )

{
    return SnmpUtilOidNCmp(A,B,max(A->idLength,B->idLength));

} // SnmpUtilOidCmp


//
// SnmpUtilOidFree
//    Free an Object Identifier
//
// Notes:
//
// Return Codes:
//
// Error Codes:
//    None.
//
VOID
SNMP_FUNC_TYPE SnmpUtilOidFree(
        IN OUT AsnObjectIdentifier *Obj // OID to free
        )

{
   SnmpUtilMemFree( Obj->ids );

   Obj->ids      = NULL;
   Obj->idLength = 0;
} // SnmpUtilOidFree



//
// SnmpUtilVarBindCpy
//    Copies the variable binding.
//
// Notes:
//    Does not allocate any memory for OID's or STRINGS.  Only the pointer
//    to the storage is copied.
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
SNMPAPI
SNMP_FUNC_TYPE SnmpUtilVarBindCpy(
           RFC1157VarBind *dst, // Destination var bind
           RFC1157VarBind *src  // Source var bind
           )

{
SNMPAPI nResult;


   // Init destination
   dst->value.asnType = ASN_NULL;

   // Copy var bind OID name
   if ( SNMPAPI_ERROR == (nResult = SnmpUtilOidCpy(&dst->name, &src->name)) )
      {
      goto Exit;
      }

   // If the value is a var bind or a string, special handling
   switch ( src->value.asnType )
      {
      case ASN_OBJECTIDENTIFIER:
         if ( SNMPAPI_ERROR ==
              (nResult = SnmpUtilOidCpy(&dst->value.asnValue.object,
                                     &src->value.asnValue.object)) )
            {
            goto Exit;
            }
         break;

      case ASN_RFC1155_IPADDRESS:
      case ASN_RFC1155_OPAQUE:
      case ASN_OCTETSTRING:
         // Alloc storage for string
         if ( NULL ==
              (dst->value.asnValue.string.stream =
               SnmpUtilMemAlloc((src->value.asnValue.string.length * sizeof(BYTE)))) )
            {
            SetLastError( SNMP_MEM_ALLOC_ERROR );

            nResult = SNMPAPI_ERROR;
            goto Exit;
            }

         // Copy string
         dst->value.asnValue.string.length = src->value.asnValue.string.length;
         memcpy( dst->value.asnValue.string.stream,
                 src->value.asnValue.string.stream,
                 dst->value.asnValue.string.length );
         dst->value.asnValue.string.dynamic = TRUE;
         break;

      default:
         // Copy var bind value.
         //    This is a non-standard copy
         dst->value = src->value;
      }

   // Set type of asn value
   dst->value.asnType = src->value.asnType;

   nResult = SNMPAPI_NOERROR;

Exit:
   if ( SNMPAPI_ERROR == nResult )
      {
      SnmpUtilVarBindFree( dst );
      }

   return nResult;
} // SnmpUtilVarBindCpy



//
// SnmpUtilVarBindListCpy
//    Copies the var bind list referenced by the source into the destination.
//
// Notes:
//    Creates memory as needed for the list.
//
//    If an error occurs, memory is freed and the destination points to NULL.
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    SNMP_MEM_ALLOC_ERROR
//
SNMPAPI
SNMP_FUNC_TYPE SnmpUtilVarBindListCpy(
           RFC1157VarBindList *dst, // Destination var bind
           RFC1157VarBindList *src  // Source var bind
           )

{
UINT     I;
SNMPAPI  nResult;


   // Initialize
   dst->len = 0;

   // Alloc memory for new var bind list
   if ( NULL == (dst->list = SnmpUtilMemAlloc((src->len * sizeof(RFC1157VarBind)))) )
      {
      SetLastError( SNMP_MEM_ALLOC_ERROR );

      nResult = SNMPAPI_ERROR;
      goto Exit;
      }

   // Copy contents of each element in list
   for ( I=0;I < src->len;I++ )
      {
      if ( SNMPAPI_ERROR ==
           (nResult = SnmpUtilVarBindCpy(&dst->list[I], &src->list[I])) )
         {
         goto Exit;
         }

      // Increment successful copy count
      dst->len++;
      }

   nResult = SNMPAPI_NOERROR;

Exit:
   if ( nResult == SNMPAPI_ERROR )
      {
      SnmpUtilVarBindListFree( dst );
      }

   return nResult;
} // SnmpUtilVarBindListCpy



//
// SnmpUtilVarBindFree
//    Releases memory associated with a particular variable binding.
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
VOID
SNMP_FUNC_TYPE SnmpUtilVarBindFree(
        RFC1157VarBind *VarBind // Variable binding to free
        )

{
   // Free Var Bind name
   SnmpUtilOidFree( &VarBind->name );

   // Free any data in the varbind value
   switch ( VarBind->value.asnType )
      {
      case ASN_OBJECTIDENTIFIER:
         SnmpUtilOidFree( &VarBind->value.asnValue.object );
         break;

      case ASN_RFC1155_IPADDRESS:
      case ASN_RFC1155_OPAQUE:
      case ASN_OCTETSTRING:
         if ( VarBind->value.asnValue.string.dynamic == TRUE )
            {
            SnmpUtilMemFree( VarBind->value.asnValue.string.stream );
            }
         break;

      default:
         break;
         // Purposefully do nothing, because no storage alloc'ed for others
      }

   // Set type to NULL
   VarBind->value.asnType = ASN_NULL;
} // SnmpUtilVarBindFree



//
// SnmpUtilVarBindListFree
//    Frees any memory kept by the var binds list, including object ids that
//    may be in the value part of the var bind.
//
// Notes:
//    The calling routines do not need to call this routine if the var binds
//    list has not been set before.  This is common sense, but just a reminder
//    since it will be called by Release PDU and Release Trap.
//
//    Even if the list has length 0, the list pointer should be set to NULL,
//    if calling this routine.
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
VOID
SNMP_FUNC_TYPE SnmpUtilVarBindListFree(
        RFC1157VarBindList *VarBindList // Variable bindings list to free
        )

{
UINT I;


   // Free items in varBinds list
   for ( I=0;I < VarBindList->len;I++ )
      {
      SnmpUtilVarBindFree( &VarBindList->list[I] );
      }

   SnmpUtilMemFree( VarBindList->list );

   VarBindList->list = NULL;
   VarBindList->len  = 0;
} // SnmpUtilVarBindListFree


#ifdef _SNMPDLL_

//
//
// Internal functions only after this point.
//    The prototypes are in UTIL.H
//
//

//
// SnmpUtilPrintOid
//    Display the SUBID's of an object identifier.
//
// Notes:
//    This routine is responsible for allocating enough memory to contain
//    the new identifier.
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    SNMP_MEM_ALLOC_ERROR
//
VOID 
SNMP_FUNC_TYPE SnmpUtilPrintOid(
        IN AsnObjectIdentifier *Oid // OID to display
        )

{
UINT I;


   // Loop through OID
   for ( I=0;I < Oid->idLength;I++ )
      {
      if ( I )
         {
         printf( ".%d", Oid->ids[I] );
         }
      else
         {
         printf( "%d", Oid->ids[I] );
         }
      }
} // SnmpUtilPrintOid


//
// SnmpUtilPrintAsnAny
//    Prints the value of a variable declared as type AsnAny.
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
VOID
SNMP_FUNC_TYPE SnmpUtilPrintAsnAny(
        IN AsnAny *Any
        )

{
   switch ( Any->asnType )
      {
      case ASN_INTEGER:
         printf( "INTEGER - %ld\n", Any->asnValue.number );
         break;

      case ASN_OCTETSTRING:
         {
         UINT J;
         BOOL IsDisplayString = TRUE;
         LPSTR StringFormat;

         for ( J=0; J < Any->asnValue.string.length && IsDisplayString; J++ )
            {
            IsDisplayString = isprint( Any->asnValue.string.stream[J] );
            }
         StringFormat = IsDisplayString ? "%c" : "<0x%02x>" ;

         printf( "OCTET STRING - " );
         for ( J=0; J < Any->asnValue.string.length; J++ )
            {
            printf( StringFormat, Any->asnValue.string.stream[J] );
            }
         putchar( '\n' );
         }
         break;

      case ASN_OBJECTIDENTIFIER:
         {
         UINT J;

         printf( "OBJECT IDENTIFIER - " );
         for ( J=0; J < Any->asnValue.object.idLength; J++ )
            {
            printf( ".%d", Any->asnValue.object.ids[J] );
            }
         putchar( '\n' );
         }
         break;

      case ASN_NULL:
         printf( "NULL - NULL\n" );
         break;

      case ASN_RFC1155_IPADDRESS:
         {
         UINT J;

         printf( "IpAddress - " );
         printf( "%d.%d.%d.%d ",
            Any->asnValue.string.stream[0] ,
            Any->asnValue.string.stream[1] ,
            Any->asnValue.string.stream[2] ,
            Any->asnValue.string.stream[3] );
         putchar( '\n' );
         }
         break;

      case ASN_RFC1155_COUNTER:
         printf( "Counter - %lu\n", Any->asnValue.number );
         break;

      case ASN_RFC1155_GAUGE:
         printf( "Guage - %lu\n", Any->asnValue.number );
         break;

      case ASN_RFC1155_TIMETICKS:
         printf( "TimeTicks - %lu\n", Any->asnValue.number );
         break;

      case ASN_RFC1155_OPAQUE:
         {
         UINT J;

         printf( "Opaque - " );
         for ( J=0; J < Any->asnValue.string.length; J++ )
            {
            printf( "0x%x ", Any->asnValue.string.stream[J] );
            }
         putchar( '\n' );
         }
         break;

      default:
         printf( "Invalid Type\n" );
         }
} // SnmpUtilPrintAsnAny



// logging and debugging output routines 

VOID 
SNMP_FUNC_TYPE
SnmpSvcSetLogLevel(
        IN INT nLevel
        )
{
    g_nLogLevel = nLevel; 
}

VOID 
SNMP_FUNC_TYPE
SnmpSvcSetLogType(
        IN INT nOutput
        )
{
    g_nLogType = nOutput;
}

VOID 
SNMP_FUNC_TYPE 
SnmpUtilOutput(INT nOutputType, LPSTR szBuffer)
    {
    static FILE *fd = NULL;

    if (nOutputType & SNMP_OUTPUT_TO_CONSOLE)
        {
        printf("%s", szBuffer);
        fflush(stdout);
        }

    if (nOutputType & SNMP_OUTPUT_TO_LOGFILE)
        {
        if (!fd)
            {
            fd = fopen("snmpdbg.log", "w");
            }
        if (fd)
            {
            fprintf(fd, "%s", szBuffer);
            fflush(fd);
            }
        }

    if (nOutputType & SNMP_OUTPUT_TO_EVENTLOG)
        {
        SnmpSvcReportEvent(SNMP_EVENT_DEBUG_TRACE, 1, &szBuffer, NO_ERROR);
        }

    if (nOutputType & SNMP_OUTPUT_TO_DEBUGGER)
        {
        OutputDebugString(szBuffer);
        }

    } 

VOID 
SNMP_FUNC_TYPE 
SnmpUtilDbgPrint(INT nLevel, LPSTR szFormat, ...)
    {
    va_list arglist;
    char szBuffer[MAX_DEBUG_LEN];

    if (nLevel <= g_nLogLevel)
        {
        va_start(arglist, szFormat);

        vsprintf(szBuffer, szFormat, arglist);

        SnmpUtilOutput(g_nLogType, szBuffer);
        }
    }

LPSTR
SNMP_FUNC_TYPE
SnmpUtilOidToA(AsnObjectIdentifier *Oid)
    {
    return SnmpUtilIdsToA(Oid->ids, Oid->idLength); 
    } 

LPSTR
SNMP_FUNC_TYPE
SnmpUtilIdsToA(UINT *Oid, UINT OidLen)
    {
    UINT I;
    UINT J;
    static char szBuffer[MAX_DEBUG_LEN];

    if (OidLen)
       {
       J = sprintf(szBuffer, "%d", Oid[0]);

       for ( I=1;I < OidLen;I++ )
          {
          J += sprintf(&szBuffer[J], ".%d", Oid[I]);
          }
       }
    else
       {
       sprintf(szBuffer, "NUL");
       }

    return szBuffer;
    } 

VOID 
SNMP_FUNC_TYPE
SnmpSvcReportEvent(DWORD nMsgId, DWORD cSubStrings, LPSTR *SubStrings, DWORD nErrorCode)
    {
    HANDLE lh;
    WORD wEventType;
    LPVOID lpData;
    WORD cbData;

    // determine type of event from message id.  note that
    // all debug messages regardless of their severity are
    // listed under SNMP_EVENT_DEBUG_TRACE (informational).  
    // see evtlog.h for the entire list of event messages.
    switch ( nMsgId >> 30 )
        {
        case STATUS_SEVERITY_INFORMATIONAL:
        case STATUS_SEVERITY_SUCCESS:
            wEventType = EVENTLOG_INFORMATION_TYPE;
            break;
        case STATUS_SEVERITY_WARNING:
            wEventType = EVENTLOG_WARNING_TYPE;
            break;
        case STATUS_SEVERITY_ERROR:
        default:
            wEventType = EVENTLOG_ERROR_TYPE;
            break;
        }

    cbData = (nErrorCode == NO_ERROR) ? 0 : sizeof(DWORD);
    lpData = (nErrorCode == NO_ERROR) ? NULL : &nErrorCode;

    if (lh = RegisterEventSource(NULL, "SNMP"))
        {
        ReportEvent(
           lh,             
           wEventType,     
           0,                  // event category
           nMsgId,         
           NULL,               // user sids
           (WORD)cSubStrings,    
           cbData,              
           SubStrings,     
           lpData);
        
        DeregisterEventSource(lh);
        }
    }

#endif // _SNMPDLL_

VOID
SNMP_FUNC_TYPE SnmpUtilMemFree(
    IN void *x
    )
{
    if (x != NULL) {

#if defined(DBG) && defined(_SNMPDLL_)

        g_nBytesTotal -= GlobalSize(x);

        SNMPDBG((
            SNMP_LOG_VERBOSE,
            "SNMP: MEM: releasing 0x%08lx (%d bytes, %d total).\n",
            x, GlobalSize(x), g_nBytesTotal
            ));

#endif

        GlobalFree( (HGLOBAL) x );
    }

    return;
}

LPVOID
SNMP_FUNC_TYPE SnmpUtilMemAlloc(
    IN unsigned int x
    )
{
    void *addr;
    addr = GlobalAlloc( GMEM_FIXED | GMEM_ZEROINIT, (DWORD)x );

#if defined(DBG) && defined(_SNMPDLL_)

    g_nBytesTotal += (addr ? GlobalSize(addr) : 0);

    SNMPDBG((
        SNMP_LOG_VERBOSE,
        "SNMP: MEM: allocated 0x%08lx (%d bytes, %d total).\n",
        addr, addr ? GlobalSize(addr) : 0, g_nBytesTotal
        ));

#endif

    return(addr);
}

LPVOID
SNMP_FUNC_TYPE SnmpUtilMemReAlloc(
    IN void *x,
    IN unsigned int y
    )
{
    void *addr;

    if (x == NULL) {

        addr = SnmpUtilMemAlloc( y );

    } else {

#if defined(DBG) && defined(_SNMPDLL_)

        SNMPDBG((
            SNMP_LOG_VERBOSE,
            "SNMP: MEM: expanding 0x%08lx (%d bytes) to %d bytes.\n",
            x, GlobalSize(x), y
            ));

        g_nBytesTotal -= GlobalSize(x);

#endif

        addr = GlobalReAlloc( (HGLOBAL) x, (DWORD)y, GMEM_MOVEABLE | GMEM_ZEROINIT);

#if defined(DBG) && defined(_SNMPDLL_)

        g_nBytesTotal += (addr ? GlobalSize(addr) : 0);

        SNMPDBG((
            SNMP_LOG_VERBOSE,
            "SNMP: MEM: allocated 0x%08lx (%d bytes, %d total).\n",
            addr, addr ? GlobalSize(addr) : 0, g_nBytesTotal
            ));

#endif

    }
    
    return(addr);
}


#ifdef _SNMPDLL_

// return true if the string contains only hex digits

BOOL isHex(LPSTR str, int strLen)
    {
    int ii;

    for (ii=0; ii < strLen; ii++)
        if (isxdigit(*str))
            str++;
        else
            return FALSE;

    return TRUE;
    }

unsigned int toHex(unsigned char x)
    {
    if (x >= '0' && x <= '9')
        return x - '0';
    else if (x >= 'A' && x <= 'F')
        return x - 'A' + 10;
    else if (x >= 'a' && x <= 'f')
        return x - 'a' + 10;
    else
        return 0;
    }

// convert str to hex number of NumDigits (must be even) into pNum
void atohex(IN LPSTR str, IN int NumDigits, OUT unsigned char *pNum)
    {
    int i, j;

    j=0;
    for (i=0; i < (NumDigits>>1) ; i++)
        {
        pNum[i] = (toHex(str[j]) << 4) + toHex(str[j+1]);
        j+=2;
        }
    }



// return true if addrText is of the form 123456789ABC or
// 000001.123456789abc
// if pNetNum is not null, upon successful return, pNetNum = network number
// if pNodeNum is not null, upon successful return, pNodeNum = node number

BOOL 
SNMP_FUNC_TYPE 
SnmpSvcAddrIsIpx(
    IN  LPSTR addrText,
    OUT char pNetNum[4],
    OUT char pNodeNum[6])
    {
    int addrTextLen;

    addrTextLen = strlen(addrText);
    if (addrTextLen == 12 && isHex(addrText, 12))
        {
            if (pNetNum)
                *((UNALIGNED unsigned long *) pNetNum) = 0L;
            if (pNodeNum)
                atohex(addrText, 12, pNodeNum);
            return TRUE;
        }
    else if (addrTextLen == 21 && addrText[8] == '.' && isHex(addrText, 8) &&
            isHex(addrText+9, 12))
        {
            if (pNetNum)
                atohex(addrText, 8, pNetNum);
            if (pNodeNum)
                atohex(addrText+9, 12, pNodeNum);
            return TRUE;
        }
    else
        return FALSE;
    }

BOOL 
SNMP_FUNC_TYPE
SnmpSvcAddrToSocket(
    LPSTR addrText,
    struct sockaddr *addrEncoding)
    {

// --------- BEGIN: PROTOCOL SPECIFIC SOCKET CODE BEGIN... ---------

    SOCKADDR_IPX mgrAddr_ipx;

    if (SnmpSvcAddrIsIpx(addrText, mgrAddr_ipx.sa_netnum, mgrAddr_ipx.sa_nodenum))
        {
        // currently, we don't/can't do gethostbyname on IPX, so no IPX
        // host name allowed.
        mgrAddr_ipx.sa_family = AF_IPX;
        bcopy(&mgrAddr_ipx, addrEncoding, sizeof(mgrAddr_ipx));
        }
    else        // if not IPX, must be INET
        {
        struct hostent *hp;
        unsigned long addr;
        struct sockaddr_in mgrAddr_in;

        if ((long)(addr = inet_addr(addrText)) == -1)
            {
            if ((hp = gethostbyname(addrText)) == NULL)
                {
                return FALSE;
                }
            else
                {
                bcopy((char *)hp->h_addr, (char *)&mgrAddr_in.sin_addr,
                      sizeof(unsigned long));
                }
            }
        else
            {
            bcopy((char *)&addr, (char *)&mgrAddr_in.sin_addr,
                  sizeof(unsigned long));
            }

        mgrAddr_in.sin_family = AF_INET;
        mgrAddr_in.sin_port = htons(WKSN_UDP_TRAP);
        bcopy(&mgrAddr_in, addrEncoding, sizeof(mgrAddr_in));
        }

// --------- END: PROTOCOL SPECIFIC SOCKET CODE END. ---------------

    return TRUE;
    } // end SnmpSvcAddrToSocket()


AsnObjectIdentifier *
SNMP_FUNC_TYPE
SnmpSvcGetEnterpriseOID(
    )
{
    // just return oid
    return g_enterprise;
}

#endif // _SNMPDLL_

//-------------------------------- END --------------------------------------
