/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    uniconv.c

Abstract:

    Routine to convert UNICODE to ASCII.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/

//--------------------------- WINDOWS DEPENDENCIES --------------------------

#include <nt.h>
#include <ntdef.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <snmp.h>
#include <snmputil.h>

#ifndef CHICAGO
#include <ntrtl.h>
#else
#include <stdio.h>
#endif

#include <string.h>
#include <stdlib.h>


//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

INT 
SNMP_FUNC_TYPE
SnmpUtilStrlenW(
    LPWSTR  uni_string )

{
   int length;

   length = -1;
   while(uni_string[++length] != TEXT('\0'));
   return length ;

}

// The return code matches what Uni->Str uses
LONG 
SNMP_FUNC_TYPE
SnmpUtilUnicodeToAnsi(
    LPSTR   *ansi_string,
    LPWSTR  uni_string,
    BOOLEAN alloc_it)

{
   int   result;
   LPSTR new_string;

   unsigned short length;
   unsigned short maxlength;

   // initialize
   if (alloc_it) {
       *ansi_string = NULL;
   }

   length    = SnmpUtilStrlenW(uni_string);
   maxlength = length + 1;

   if (alloc_it) {
       new_string = SnmpUtilMemAlloc(maxlength * sizeof(CHAR));
   } else {
       new_string = *ansi_string;
   }

   if (new_string == NULL) {
       return(-1);
   }

   if (length) {

       result = WideCharToMultiByte(
                    CP_ACP,
                    0,
                    uni_string,
                    length,
                    new_string,
                    maxlength,
                    NULL,
                    NULL
                    );
   } else {

      *new_string = '\0';

      result = 1; // converted terminating character
   }

   if (alloc_it) {

       if (result) {

           *ansi_string = new_string;

       } else {

           SNMPDBG((
                SNMP_LOG_ERROR,
                "SNMP: API: WideCharToMultiByte returns 0x%08lx.\n",
                GetLastError()
                ));

           SnmpUtilMemFree(new_string);
       }
   }

   return (result > 0) ? 0 : -1;
}


// The return code matches what Uni->Str uses
LONG 
SNMP_FUNC_TYPE
SnmpUtilAnsiToUnicode(
    LPWSTR  *uni_string,
    LPSTR   ansi_string,
    BOOLEAN alloc_it)

{
   int   result;
   LPWSTR new_string;

   unsigned short length;
   unsigned short maxlength;

   // initialize
   if (alloc_it) {
       *uni_string = NULL;
   }

   length    = strlen(ansi_string);
   maxlength = length + 1;

   if (alloc_it) {
       new_string = SnmpUtilMemAlloc(maxlength * sizeof(WCHAR));
   } else {
       new_string = *uni_string;
   }

   if (new_string == NULL) {
       return(-1);
   }

   if (length) {

       result = MultiByteToWideChar(
                    CP_ACP,
                    MB_PRECOMPOSED,
                    ansi_string,
                    length,
                    new_string,
                    maxlength
                    );

   } else {

      *new_string = L'\0';

      result = 1; // converted terminating character
   }

   if (alloc_it) {

       if (result) {

           *uni_string = new_string;

       } else {

           SNMPDBG((
                SNMP_LOG_ERROR,
                "SNMP: API: MultiByteToWideChar returns 0x%08lx.\n",
                GetLastError()
                ));

           SnmpUtilMemFree(new_string);
       }
   }

   return (result > 0) ? 0 : -1;
}

//-------------------------------- END --------------------------------------
