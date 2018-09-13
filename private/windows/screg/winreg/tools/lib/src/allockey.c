/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Allockey.c

Abstract:

    This module contains the AllocateKey function which is part of the
    Configuration Registry Tools (CRTools) library.

Author:

    David J. Gilman (davegi) 02-Jan-1992

Environment:

    Windows, Crt - User Mode

--*/

#include <stdlib.h>

#include "crtools.h"

PKEY
AllocateKey(
    IN PSTR MachineName,
    IN PKEY Parent,
    IN PSTR SubKeyName
    )


/*++

Routine Description:

    Allocates memory for a KEY structure and initializes it with the
    supplied parent PKEY and a copy of the sub key name.

Arguments:

    Parent - Supplies a PKEY which is the parent of the new Key.

    SubKeyName - Supplies a pointer to a string which is the name of this
        sub key. If the pointer is NULL then Parent refers to a predefined
        key.

    MachineName - Supplies an optional machine name whose Registry is to
        be accessed.

Return Value:

    PKEY - Returns a pointer to the newly allocated and initialized
        sub-key.

--*/

{
    LONG    Error;
    PKEY    Key;
    HKEY    Handle;

    ASSERT( ARGUMENT_PRESENT( Parent ));

    //
    // If a machine name was supplied, connect to that machine and replace
    // the predefined handle with the remote handle.
    //

    if( ARGUMENT_PRESENT( MachineName )) {

        Error = RegConnectRegistry(
                    MachineName,
                    Parent->KeyHandle,
                    &Parent->KeyHandle
                    );

        if( Error != ERROR_SUCCESS ) {

            ASSERT_MESSAGE( FALSE, "RegConnectRegistry - " );
            return FALSE;
        }
    }

    //
    // Check for a NULL sub key name and a parent that is a predefined key.
    //

    if( SubKeyName == NULL ) {
#if 0
    if((( SubKeyName == NULL )
        && (( Parent->KeyHandle == HKEY_CLASSES_ROOT )
        ||  ( Parent->KeyHandle == HKEY_CURRENT_USER )
        ||  ( Parent->KeyHandle == HKEY_LOCAL_MACHINE )
        ||  ( Parent->KeyHandle == HKEY_USERS )))) {
#endif
        //
        // There is no sub-key so the handle to open and the KEY object
        // to return is the parent.
        //

        Handle = Parent->KeyHandle;
        Key = Parent;

    } else {


        //
        // Allocate space for the new KEY.
        //

        Key = ( PKEY ) malloc( sizeof( KEY ));

        if( Key == NULL ) {
            ASSERT_MESSAGE( FALSE, "malloc of Key - " );
            return NULL;
        }

        //
        // Allocate space for the new KEY's full name.
        //

        Key->SubKeyFullName = ( PSTR ) malloc(
                                    strlen( SubKeyName ) + 1
                                    + strlen( Parent->SubKeyFullName )
                                    + sizeof(( TCHAR ) '\\' )
                                    );

        if( Key->SubKeyFullName == NULL ) {
            ASSERT_MESSAGE( FALSE, "malloc of SubKeyFullName - " );
            return NULL;
        }

        //
        // Capture the full name.
        //

        strcpy( Key->SubKeyFullName, Parent->SubKeyFullName );
        strcat( Key->SubKeyFullName, "\\" );
        strcat( Key->SubKeyFullName, SubKeyName );

        //
        // Allocate space for the new KEY's name.
        //

        Key->SubKeyName = ( PSTR ) malloc( strlen( SubKeyName ) + 1 );

        if( Key->SubKeyName == NULL ) {
            ASSERT_MESSAGE( FALSE, "malloc of SubKeyName - " );
            return NULL;
        }

        //
        // Capture the name.
        //

        strcpy( Key->SubKeyName, SubKeyName );

        //
        // Initialize the KEY's parent.
        //

        Key->Parent = Parent;

        //
        // Initialize the KEY's signature if under DBG control.
        //

#if DBG

        Key->Signature = KEY_SIGNATURE;

#endif // DBG

        //
        // Attempt to open the sub key.
        //

        Error = RegOpenKeyEx(
            Parent->KeyHandle,
            Key->SubKeyName,
            0,
            KEY_READ,
            &Key->KeyHandle
            );

        if( Error != ERROR_SUCCESS ) {

            ASSERT_MESSAGE( FALSE, "RegOpenKey - " );
            return FALSE;
        }

        //
        // Record the handle so that the following query works for either
        // the parent or the child.
        //

        Handle = Key->KeyHandle;
    }

    //
    // At this point Key/Handle either both refer to the Parent or to the
    // newly created subkey.
    //

    ASSERT( Handle == Key->KeyHandle );

    //
    // Query how many bytes are need for the class string. The expected
    // result is to get an ERROR_INVALID_PARAMETER error returned with
    // the ClassLength parameter filled in.
    //

    Key->ClassLength = 0;

    Error = RegQueryInfoKey(
        Handle,
        Key->ClassName,
        &Key->ClassLength,
        NULL,
        &Key->NumberOfSubKeys,
        &Key->MaxSubKeyNameLength,
        &Key->MaxSubKeyClassLength,
        &Key->NumberOfValues,
        &Key->MaxValueNameLength,
        &Key->MaxValueDataLength,
        &Key->SecurityDescriptorLength,
        &Key->LastWriteTime
        );

#if 0

    // BUGBUG This seems inconsistent - need bryanwi to clean-up?

    if( Error != ERROR_INVALID_PARAMETER ) {

        ASSERT_MESSAGE( FALSE, "RegQueryInfoKey( class length ) - " );
        return FALSE;
    }
#endif // 0
    //
    // If there is no class string set it to NULL.
    //

    if( Key->ClassLength == 0 ) {

        Key->ClassName = NULL;

    } else {

        //
        // Allocate space for the class string and get all of the info
        // for this key.
        //

        Key->ClassLength++;
        Key->ClassName = ( PSTR ) malloc( Key->ClassLength );

        if( Key->ClassName == NULL ) {

            ASSERT_MESSAGE( FALSE, "malloc of ClassName - " );
            return FALSE;
        }

        Error = RegQueryInfoKey(
            Key->KeyHandle,
            Key->ClassName,
            &Key->ClassLength,
            NULL,
            &Key->NumberOfSubKeys,
            &Key->MaxSubKeyNameLength,
            &Key->MaxSubKeyClassLength,
            &Key->NumberOfValues,
            &Key->MaxValueNameLength,
            &Key->MaxValueDataLength,
            &Key->SecurityDescriptorLength,
            &Key->LastWriteTime
            );

        if( Error != ERROR_SUCCESS ) {

            ASSERT_MESSAGE( FALSE, "Could not query all info the sub key" );
            return FALSE;
        }
    }

    return Key;
}
