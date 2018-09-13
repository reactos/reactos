/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    Crdel.c

Abstract:

Author:

    David J. Gilman (davegi) 20-Dec-1991

Environment:

    Windows, Crt - User Mode

--*/

#include <ctype.h>
#include <stdlib.h>
#include <windows.h>

#include "crtools.h"

//
// KEY_ELEMENT is used to maintain a list of KEYs.
//

typedef struct _KEY_ELEMENT
    KEY_ELEMENT,
    *PKEY_ELEMENT;

struct _KEY_ELEMENT {
    PKEY            Key;
    PKEY_ELEMENT    NextKeyElement;
    };

//
// Error and informational messages.
//

PSTR    UsageMessage =

    "Usage: crdel [-?] [-q] key...\n";

PSTR    HelpMessage =

    "\n  where:\n"                                                          \
      "    -?   - display this message.\n"                                  \
      "    -q   - quiet mode\n"                                             \
      "    key  - name(s) of the key(s) to dump.\n"                         \
    "\n  A key is formed by specifying one of the predefined handles:\n"    \
    "\n         - HKEY_LOCAL_MACHINE\n"                                     \
      "         - HKEY_CLASSES_ROOT\n"                                      \
      "         - HKEY_CURRENT_USER\n"                                      \
      "         - HKEY_USERS\n"                                             \
    "\n  followed by a sub-key name.\n"                                     \
    "\n  An environment variable can be used as shorthand for the\n"        \
    "  predefined handles.  For example,\n"                                 \
    "\n    crdel HKEY_USERS\\davegi\n"                                      \
    "\n  is equivalent to\n"                                                \
    "\n    set HKEY_USERS=hu\n"                                             \
      "    crdel hu\\davegi\n";


PSTR    InvalidKeyMessage =

    "Invalid key - %s\n";

PSTR    InvalidSwitchMessage =

    "Invalid switch - %s\n";

PSTR    DeletingTreeMessage =

    "Deleteing tree %s\n";

VOID
DeleteTree(
    IN HKEY KeyHandle
    )

{
    LONG        Error;
    DWORD       Index;
    HKEY        ChildHandle;


    TSTR        KeyName[ MAX_PATH ];
    DWORD       KeyNameLength;
    TSTR        ClassName[ MAX_PATH ];
    DWORD       ClassNameLength;
    DWORD       TitleIndex;
    DWORD       NumberOfSubKeys;
    DWORD       MaxSubKeyLength;
    DWORD       MaxClassLength;
    DWORD       NumberOfValues;
    DWORD       MaxValueNameLength;
    DWORD       MaxValueDataLength;
    DWORD       SecurityDescriptorLength;
    FILETIME    LastWriteTime;

    ClassNameLength = MAX_PATH;

    Error = RegQueryInfoKey(
                KeyHandle,
                ClassName,
                &ClassNameLength,
                &TitleIndex,
                &NumberOfSubKeys,
                &MaxSubKeyLength,
                &MaxClassLength,
                &NumberOfValues,
                &MaxValueNameLength,
                &MaxValueDataLength,
                &SecurityDescriptorLength,
                &LastWriteTime
                );
    REG_API_SUCCESS( RegQueryInfoKey );

    for( Index = 0; Index < NumberOfSubKeys; Index++ ) {

        KeyNameLength = MAX_PATH;

        Error = RegEnumKey(
                    KeyHandle,
                    0,
                    KeyName,
                    KeyNameLength
                    );
        REG_API_SUCCESS( RegEnumKey );

        Error = RegOpenKeyEx(
                    KeyHandle,
                    KeyName,
                    REG_OPTION_RESERVED,
                    KEY_ALL_ACCESS,
                    &ChildHandle
                    );
        REG_API_SUCCESS( RegOpenKey );

        DeleteTree( ChildHandle );

        Error = RegCloseKey(
                    ChildHandle
                    );
        REG_API_SUCCESS( RegCloseKey );

        Error = RegDeleteKey(
                    KeyHandle,
                    KeyName
                    );
        REG_API_SUCCESS( RegDeleteKey );
    }
}
VOID
DeleteKey(
    IN PKEY     Key
    )

{
    LONG    Error;

    ASSERT( Key != NULL );
    ASSERT( Key->KeyHandle != NULL );

    DeleteTree( Key->KeyHandle );

    Error = RegDeleteKey(
                Key->Parent->KeyHandle,
                Key->SubKeyName
                );
    REG_API_SUCCESS( RegDeleteKey );
}

VOID
main(
    INT     argc,
    PCHAR   argv[ ]
    )

/*++

Routine Description:


Arguments:


Return Value:

    None.

--*/

{
    BOOL            Quiet;
    PKEY            ParsedKey;
    PKEY_ELEMENT    ParsedKeyElement;
    PKEY_ELEMENT    ParsedKeysHead;
    PKEY_ELEMENT    ParsedKeysTail;
    KEY_ELEMENT     Dummy = { NULL, NULL };

    //
    // If CrDel is invoked without any command line options, display
    // the usage message.
    //

    if( argc < 2 ) {

        DisplayMessage( TRUE, UsageMessage );
    }

    //
    // By default the user is prompted.
    //

    Quiet   = FALSE;

    //
    // Use a Dummy KEY structure to simplify the list management.
    // Initialize the head and tail pointers to point to the Dummy.
    //

    ParsedKeysHead = &Dummy;
    ParsedKeysTail = &Dummy;

    //
    // Initialize options based on the command line.
    //

    while( *++argv ) {

        //
        // If the command line argument is a switch character...
        //

        if( isswitch(( *argv )[ 0 ] )) {

            switch( tolower(( *argv )[ 1 ] )) {

            //
            // Display the detailed help message and quit.
            //

            case '?':

                DisplayMessage( FALSE, UsageMessage );
                DisplayMessage( TRUE, HelpMessage );
                break;

            //
            // Do not prompt user for delete.
            //

            case 'q':

                Quiet = TRUE;
                break;

            //
            // Display invalid switch message and quit.
            //

            default:

                DisplayMessage( FALSE, InvalidSwitchMessage, *argv );
                DisplayMessage( TRUE, UsageMessage );
            }
        } else {

            //
            // The command line argument was not a switch so attempt to parse
            // it into a predefined handle and a sub key.
            //

            ParsedKey = ParseKey( *argv );

            if( ParsedKey ) {

                //
                // If the command line argument was succesfully parsed,
                // allocate and initialize a KEY_ELEMENT, add it to the
                // list and update the tail pointer.
                //

                ParsedKeyElement = ( PKEY_ELEMENT ) malloc(
                                    sizeof( KEY_ELEMENT )
                                    );
                ASSERT( ParsedKeyElement );

                ParsedKeyElement->Key               = ParsedKey;
                ParsedKeyElement->NextKeyElement    = NULL;

                ParsedKeysTail->NextKeyElement = ParsedKeyElement;
                ParsedKeysTail = ParsedKeyElement;


            } else {

                //
                // The command line argument was not succesfully parsed,
                // so display an invalid key message and continue.
                //

                DisplayMessage( FALSE, InvalidKeyMessage, *argv );
            }
        }
    }

    //
    // Command line parsing is complete. Delete the requested keys
    // skipping the Dummy KEY_ELEMENT structure.
    //

    while( ParsedKeysHead = ParsedKeysHead->NextKeyElement ) {

        if( ! Quiet ) {

            DisplayMessage(
                FALSE,
                DeletingTreeMessage,
                ParsedKeysHead->Key->SubKeyFullName
                );
        }

        DeleteKey( ParsedKeysHead->Key );

        //
        // Once the KEY structure's Key is deleted both the KEY and
        // KEY_ELEMENT can be freed.
        //

        FreeKey( ParsedKeysHead->Key );
        free( ParsedKeysHead );
    }
}
