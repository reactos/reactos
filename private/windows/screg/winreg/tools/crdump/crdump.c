/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    Crdump.c

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

    "Usage: crdump [-?] [-d] [-v] [-r] key...\n";

PSTR    HelpMessage =

    "\n  where:\n"                                                          \
      "    -?   - display this message.\n"                                  \
      "    -d   - dump all data (implies -v).\n"                            \
      "    -v   - dump all values.\n"                                       \
      "    -r   - recurse through sub keys.\n"                              \
      "    key  - name(s) of the key(s) to dump.\n"                         \
    "\n  A key is formed by specifying one of the predefined handles:\n"    \
    "\n         - HKEY_LOCAL_MACHINE\n"                                     \
      "         - HKEY_CLASSES_ROOT\n"                                      \
      "         - HKEY_CURRENT_USER\n"                                      \
      "         - HKEY_USERS\n"                                             \
    "\n  followed by a sub-key name.\n"                                     \
    "\n  An environment variable can be used as shorthand for the\n"        \
    "  predefined handles.  For example,\n"                                 \
    "\n    crdump HKEY_USERS\\davegi\n"                                     \
    "\n  is equivalent to\n"                                                \
    "\n    set HKEY_USERS=hu\n"                                             \
      "    crdump hu\\davegi\n";


PSTR    InvalidKeyMessage =

    "Invalid key - %s\n";

PSTR    InvalidSwitchMessage =

    "Invalid switch - %s\n";

PSTR    DisplayKeyFailMessage =

    "Could not display key %s\n";

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
    BOOL            Values;
    BOOL            Data;
    BOOL            Recurse;
    PKEY            ParsedKey;
    PKEY_ELEMENT    ParsedKeyElement;
    PKEY_ELEMENT    ParsedKeysHead;
    PKEY_ELEMENT    ParsedKeysTail;
    KEY_ELEMENT     Dummy = { NULL, NULL };

    //
    // If CrDump is invoked without any command line options, display
    // the usage message.
    //

    if( argc < 2 ) {

        DisplayMessage( TRUE, UsageMessage );
    }

    //
    // By default, no sub keys, values or data are displayed.
    //

    Recurse = FALSE;
    Values  = FALSE;
    Data    = FALSE;

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
            // Display data - implies display values.

            case 'd':

                Values  = TRUE;
                Data    = TRUE;
                break;

            //
            // Display sub keys.
            //

            case 'r':

                Recurse = TRUE;
                break;

            //
            // Display values.
            //

            case 'v':

                Values = TRUE;
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
    // Command line parsing is complete. Display the requested keys
    // skipping the Dummy KEY_ELEMENT structure.
    //

    while( ParsedKeysHead = ParsedKeysHead->NextKeyElement ) {

        DisplayKeys( ParsedKeysHead->Key, Values, Data, Recurse );

        //
        // Once the KEY structure's Key is displayed both the KEY and
        // KEY_ELEMENT can be freed.
        //

        FreeKey( ParsedKeysHead->Key );
        free( ParsedKeysHead );
    }
}
