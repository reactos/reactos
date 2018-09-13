//--------------------------------------------------------------------------------
// Copyright (C) Micorosoft Confidential 1997
// Author: RameshV
// Description: Option related registry handling -- common between NT and VxD
//--------------------------------------------------------------------------------

#include <wininetp.h>
#include "aproxp.h"

#ifndef  OPTREG_H
#define  OPTREG_H

//--------------------------------------------------------------------------------
// function definitions
//--------------------------------------------------------------------------------

POPTION                                           // option from which more appends can occur
DhcpAppendSendOptions(                            // append all configured options
    IN OUT  PDHCP_CONTEXT          DhcpContext,   // this is the context to append for
    IN      PLIST_ENTRY            SendOptionsList,
    IN      LPBYTE                 ClassName,     // current class
    IN      DWORD                  ClassLen,      // len of above in bytes
    IN      LPBYTE                 BufStart,      // start of buffer
    IN      LPBYTE                 BufEnd,        // how far can we go in this buffer
    IN OUT  LPBYTE                 SentOptions,   // BoolArray[OPTION_END+1] to avoid repeating options
    IN OUT  LPBYTE                 VSentOptions,  // to avoid repeating vendor specific options
    IN OUT  LPBYTE                 VendorOpt,     // Buffer[OPTION_END+1] Holding Vendor specific options
    OUT     LPDWORD                VendorOptLen   // the # of bytes filled into that
);

DWORD                                             // status
DhcpDestroyOptionsList(                           // destroy a list of options, freeing up memory
    IN OUT  PLIST_ENTRY            OptionsList,   // this is the list of options to destroy
    IN      PLIST_ENTRY            ClassesList    // this is where to remove classes off
);

DWORD                                             // win32 status
DhcpClearAllOptions(                              // remove all turds from off registry
    IN OUT  PDHCP_CONTEXT          DhcpContext    // the context to clear for
);


#endif OPTREG_H

// internal private function that takes the lock on OPTIONS_LIST

DWORD                                             // status
DhcpRegClearOptDefs(                              // clear all standard options
    IN      LPTSTR                 AdapterName    // clear for this adapter
);


//
// options related lists
//


LIST_ENTRY DhcpGlobalRecvFromList;
LPSTR   DhcpGlobalClientClassInfo = NULL;


LPBYTE                                            // ptr to buf loc where more appends can occur
DhcpAppendParamRequestList(                       // append the param request list option
    IN      PDHCP_CONTEXT          DhcpContext,   // the context to append for
    IN      PLIST_ENTRY            SendOptionsList,// look thru this list
    IN      LPBYTE                 ClassName,     // which class does this belong to?
    IN      DWORD                  ClassLen,      // size of above in bytes
    IN      LPBYTE                 BufStart,      // where to start adding this option
    IN      LPBYTE                 BufEnd         // limit for this option
) {
    BYTE                           Buffer[OPTION_END+1];
    LPBYTE                         Tmp;
    DWORD                          FirstSize;
    DWORD                          Size;
    PLIST_ENTRY                    ThisEntry;
    PDHCP_OPTION                   ThisOpt;
    DWORD                          i, j;

    Size = FirstSize = 0;
    Buffer[Size++] = OPTION_SUBNET_MASK;          // standard requested options
    Buffer[Size++] = OPTION_DOMAIN_NAME;
    Buffer[Size++] = OPTION_ROUTER_ADDRESS;
    Buffer[Size++] = OPTION_DOMAIN_NAME_SERVERS;
    Buffer[Size++] = OPTION_NETBIOS_NAME_SERVER;
    Buffer[Size++] = OPTION_NETBIOS_NODE_TYPE;
    Buffer[Size++] = OPTION_NETBIOS_SCOPE_OPTION;
    Buffer[Size++] = OPTION_VENDOR_SPEC_INFO;
    Buffer[Size++] = OPTION_USER_CLASS;
    Buffer[Size++] = OPTION_WPAD_URL;

    ThisEntry = SendOptionsList->Flink;
    while( ThisEntry != SendOptionsList ) {
        ThisOpt = CONTAINING_RECORD(ThisEntry, DHCP_OPTION, OptionList);
        ThisEntry = ThisEntry->Flink;

        if( ThisOpt->IsVendor) continue;

        if( ThisOpt->ClassLen ) {
            if( ThisOpt->ClassLen != ClassLen ) continue;
            if( ThisOpt->ClassName != ClassName )
                continue;                         // this option is not used for this client
        }

        if( OPTION_PARAMETER_REQUEST_LIST != ThisOpt->OptionId ) {
            //
            // only if the option is param_request_list do we request..
            //
            continue;
        }

        for( i = 0; i < ThisOpt->DataLen ; i ++ ) {
            for( j = 0; j < Size; j ++ )
                if( ThisOpt->Data[i] == Buffer[j] ) break;
            if( j < Size ) continue;              // option already plugged in
            Buffer[Size++] = ThisOpt->Data[i]; // add this option
        }

        if( 0 == FirstSize ) FirstSize = Size;
    }

    if( 0 == FirstSize ) FirstSize = Size;

    Tmp = BufStart;
    BufStart = (LPBYTE)DhcpAppendOption(          // now add the param request list
        (POPTION)BufStart,
        (BYTE)OPTION_PARAMETER_REQUEST_LIST,
        Buffer,
        (BYTE)Size,
        BufEnd
    );

    if( Tmp == BufStart ) {                       // did not really add the option
        BufStart = (LPBYTE)DhcpAppendOption(      // now try adding the first request we saw instead of everything
            (POPTION)BufStart,
            (BYTE)OPTION_PARAMETER_REQUEST_LIST,
            Buffer,
            (BYTE)FirstSize,
            BufEnd
        );
    }

    return BufStart;
}

POPTION                                           // option from which more appends can occur
DhcpAppendSendOptions(                            // append all configured options
    IN OUT  PDHCP_CONTEXT          DhcpContext,   // this is the context to append for
    IN      PLIST_ENTRY            SendOptionsList,
    IN      LPBYTE                 ClassName,     // current class
    IN      DWORD                  ClassLen,      // len of above in bytes
    IN      LPBYTE                 BufStart,      // start of buffer
    IN      LPBYTE                 BufEnd,        // how far can we go in this buffer
    IN OUT  LPBYTE                 SentOptions,   // BoolArray[OPTION_END+1] to avoid repeating options
    IN OUT  LPBYTE                 VSentOptions,  // to avoid repeating vendor specific options
    IN OUT  LPBYTE                 VendorOpt,     // Buffer[OPTION_END+1] Holding Vendor specific options
    OUT     LPDWORD                VendorOptLen   // the # of bytes filled into that
) {
    PLIST_ENTRY                    ThisEntry;
    PDHCP_OPTION                  ThisOpt;

    DhcpAssert(FALSE == SentOptions[OPTION_PARAMETER_REQUEST_LIST]);
    BufStart = DhcpAppendParamRequestList(
        DhcpContext,
        SendOptionsList,
        ClassName,
        ClassLen,
        BufStart,
        BufEnd
    );
    SentOptions[OPTION_PARAMETER_REQUEST_LIST] = TRUE;

    ThisEntry = SendOptionsList->Flink;
    while( ThisEntry != SendOptionsList ) {
        ThisOpt = CONTAINING_RECORD(ThisEntry, DHCP_OPTION, OptionList);
        ThisEntry = ThisEntry->Flink;

        if( ThisOpt->IsVendor ? VSentOptions[ThisOpt->OptionId] : SentOptions[ThisOpt->OptionId] )
            continue;

        // if( ThisOpt->IsVendor) continue;       // No vendor specific information this round through
        if( ThisOpt->ClassLen ) {
            if( ThisOpt->ClassLen != ClassLen ) continue;
            if( ThisOpt->ClassName != ClassName )
                continue;                         // this option is not used for this client
        }

        if( !ThisOpt->IsVendor ) {                // easy to add non-vendor spec options
            SentOptions[ThisOpt->OptionId] = TRUE;
            BufStart = (LPBYTE)DhcpAppendOption(
                (POPTION)BufStart,
                ThisOpt->OptionId,
                ThisOpt->Data,
                (BYTE)ThisOpt->DataLen,
                BufEnd
            );
        } else {                                  // ENCAPSULATE vendor specific options
            if( SentOptions[OPTION_VENDOR_SPEC_INFO] )
                continue;                         // Vendor spec info already added

            VSentOptions[ThisOpt->OptionId] = TRUE;

            if( ThisOpt->DataLen + 2 + *VendorOptLen > OPTION_END )
                continue;                         // this option overflows the buffer

            VendorOpt[(*VendorOptLen)++] = ThisOpt->OptionId;
            VendorOpt[(*VendorOptLen)++] = (BYTE)ThisOpt->DataLen;
            memcpy(&VendorOpt[*VendorOptLen], ThisOpt->Data, ThisOpt->DataLen);
            (*VendorOptLen) += ThisOpt->DataLen;
        }
    }
    return (POPTION)BufStart;
}


DWORD                                             // status
DhcpDestroyOptionsList(                           // destroy a list of options, freeing up memory
    IN OUT  PLIST_ENTRY            OptionsList,   // this is the list of options to destroy
    IN      PLIST_ENTRY            ClassesList    // this is where to remove classes off
) {
    PLIST_ENTRY                    ThisEntry;
    PDHCP_OPTION                   ThisOption;
    DWORD                          Error;
    DWORD                          LastError;

    LastError = ERROR_SUCCESS;
    while(!IsListEmpty(OptionsList) ) {           // for each element of this list
        ThisEntry  = RemoveHeadList(OptionsList);
        ThisOption = CONTAINING_RECORD(ThisEntry, DHCP_OPTION, OptionList);

        if( NULL != ThisOption->ClassName ) {     // if there is a class, deref it
            Error = DhcpDelClass(
                ClassesList,
                ThisOption->ClassName,
                ThisOption->ClassLen
            );
            if( ERROR_SUCCESS != Error ) {
                DhcpAssert( ERROR_SUCCESS == Error);
                LastError = Error;
            }
        }

        DhcpFreeMemory(ThisOption);               // now really free this
    }
    return LastError;
}

DWORD                                             // win32 status
DhcpClearAllOptions(                              // clear all the options information
    IN OUT  PDHCP_CONTEXT          DhcpContext    // the context to clear for
) {
    PLIST_ENTRY                    ThisEntry;
    PDHCP_OPTION                   ThisOption;
    DWORD                          LocalError;

    //(void) DhcpRegClearOptDefs(DhcpContext->AdapterName));

    ThisEntry = DhcpContext->RecdOptionsList.Flink;
    while(ThisEntry != &DhcpContext->RecdOptionsList) {
        ThisOption = CONTAINING_RECORD(ThisEntry, DHCP_OPTION, OptionList);
        ThisEntry  = ThisEntry->Flink;

        // bug bug, need to return space?
        ThisOption->Data = NULL;
        ThisOption->DataLen = 0;

        //LocalError = DhcpMarkParamChangeRequests(
        //    DhcpContext->AdapterName,
        //    ThisOption->OptionId,
        //    ThisOption->IsVendor,
        //    ThisOption->ClassName
        //);
        DhcpAssert(ERROR_SUCCESS == LocalError);
    }
    return ERROR_SUCCESS;
}

POPTION                                           // buffer after filling option
DhcpAppendClassIdOption(                          // fill class id if exists
    IN OUT  PDHCP_CONTEXT          DhcpContext,   // the context to fillfor
    OUT     LPBYTE                 BufStart,      // start of message buffer
    IN      LPBYTE                 BufEnd         // end of message buffer
) {
    DWORD                          Size;

    Size = (DWORD)(BufEnd - BufStart);

    if( DhcpContext->ClassId ) {
        DhcpAssert(DhcpContext->ClassIdLength);
        BufStart = (LPBYTE)DhcpAppendOption(
            (POPTION)BufStart,
            OPTION_USER_CLASS,
            DhcpContext->ClassId,
            (BYTE)DhcpContext->ClassIdLength,
            BufEnd
        );
    }

    return (POPTION) BufStart;
}

LPOPTION
DhcpAppendOption(
    LPOPTION Option,
    BYTE OptionType,
    PVOID OptionValue,
    ULONG OptionLength,
    LPBYTE OptionEnd
)
/*++

Routine Description:

    This function writes a DHCP option to message buffer.

Arguments:

    Option - A pointer to a message buffer.

    OptionType - The option number to append.

    OptionValue - A pointer to the option data.

    OptionLength - The lenght, in bytes, of the option data.

    OptionEnd - End of Option Buffer.

Return Value:

    A pointer to the end of the appended option.

--*/
{
    DWORD  i;

    if ( OptionType == OPTION_END ) {

        //
        // we should alway have atleast one BYTE space in the buffer
        // to append this option.
        //

        DhcpAssert( (LPBYTE)Option < OptionEnd );


        Option->OptionType = OPTION_END;
        return( (LPOPTION) ((LPBYTE)(Option) + 1) );

    }

    if ( OptionType == OPTION_PAD ) {

        //
        // add this option only iff we have enough space in the buffer.
        //

        if(((LPBYTE)Option + 1) < (OptionEnd - 1) ) {
            Option->OptionType = OPTION_PAD;
            return( (LPOPTION) ((LPBYTE)(Option) + 1) );
        }

        DhcpPrint(("DhcpAppendOption failed to append Option "
                    "%ld, Buffer too small.\n", OptionType ));
        return Option;
    }


    //
    // add this option only iff we have enough space in the buffer.
    //

    if(((LPBYTE)Option + 2 + OptionLength) >= (OptionEnd - 1) ) {
        DhcpPrint(("DhcpAppendOption failed to append Option "
                    "%ld, Buffer too small.\n", OptionType ));
        return Option;
    }

    if( OptionLength <= 0xFF ) {
        // simple option.. no need to use OPTION_MSFT_CONTINUED
        Option->OptionType = OptionType;
        Option->OptionLength = (BYTE)OptionLength;
        memcpy( Option->OptionValue, OptionValue, OptionLength );
        return( (LPOPTION) ((LPBYTE)(Option) + Option->OptionLength + 2) );
    }

    // option size is > 0xFF --> need to continue it using multiple ones..
    // there are OptionLenght / 0xFF occurances using 0xFF+2 bytes + one
    // using 2 + (OptionLength % 0xFF ) space..

    // check to see if we have the space first..

    if( 2 + (OptionLength%0xFF) + 0x101*(OptionLength/0xFF)
        + (LPBYTE)Option >= (OptionEnd - 1) ) {
        DhcpPrint(("DhcpAppendOption failed to append Option "
                    "%ld, Buffer too small.\n", OptionType ));
        return Option;
    }

    // first finish off all chunks of 0xFF size that we can do..

    i = OptionLength/0xFF;
    while(i) {
        Option->OptionType = OptionType;
        Option->OptionLength = 0xFF;
        memcpy(Option->OptionValue, OptionValue, 0xFF);
        OptionValue = 0xFF+(LPBYTE)OptionValue;
        Option = (LPOPTION)(0x101 + (LPBYTE)Option);
        OptionType = OPTION_MSFT_CONTINUED;       // all but the first use this ...
        OptionLength -= 0xFF;
    }

    // now finish off the remaining stuff..
    DhcpAssert(OptionLength <= 0xFF);
    Option->OptionType = OPTION_MSFT_CONTINUED;
    Option->OptionLength = (BYTE)OptionLength;
    memcpy(Option->OptionValue, OptionValue, OptionLength);
    Option = (LPOPTION)(2 + OptionLength + (LPBYTE)Option);
    DhcpAssert((LPBYTE)Option < OptionEnd);

    return Option;
}


LPBYTE
DhcpAppendMagicCookie(
    LPBYTE Option,
    LPBYTE OptionEnd
    )
/*++

Routine Description:

    This routine appends magic cookie to a DHCP message.

Arguments:

    Option - A pointer to the place to append the magic cookie.

    OptionEnd - End of Option buffer.

Return Value:

    A pointer to the end of the appended cookie.

    Note : The magic cookie is :

     --------------------
    | 99 | 130 | 83 | 99 |
     --------------------

--*/
{
    DhcpAssert( (Option + 4) < (OptionEnd - 1) );
    if( (Option + 4) < (OptionEnd - 1) ) {
        *Option++ = (BYTE)DHCP_MAGIC_COOKIE_BYTE1;
        *Option++ = (BYTE)DHCP_MAGIC_COOKIE_BYTE2;
        *Option++ = (BYTE)DHCP_MAGIC_COOKIE_BYTE3;
        *Option++ = (BYTE)DHCP_MAGIC_COOKIE_BYTE4;
    }

    return( Option );
}

LPOPTION
DhcpAppendClientIDOption(
    LPOPTION Option,
    BYTE ClientHWType,
    LPBYTE ClientHWAddr,
    BYTE ClientHWAddrLength,
    LPBYTE OptionEnd

    )
/*++

Routine Description:

    This routine appends client ID option to a DHCP message.

History:
    8/26/96 Frankbee    Removed 16 byte limitation on the hardware
                        address

Arguments:

    Option - A pointer to the place to append the option request.

    ClientHWType - Client hardware type.

    ClientHWAddr - Client hardware address

    ClientHWAddrLength - Client hardware address length.

    OptionEnd - End of Option buffer.

Return Value:

    A pointer to the end of the newly appended option.

    Note : The client ID option will look like as below in the message:

     -----------------------------------------------------------------
    | OpNum | Len | HWType | HWA1 | HWA2 | .....               | HWAn |
     -----------------------------------------------------------------

--*/
{
#pragma warning(disable : 4200) // disable 0-sized array warning

    struct _CLIENT_ID {
        BYTE    bHardwareAddressType;
        BYTE    pbHardwareAddress[0];
    } *pClientID;


    LPOPTION lpNewOption;

    pClientID = (_CLIENT_ID *) DhcpAllocateMemory( sizeof( struct _CLIENT_ID ) + ClientHWAddrLength );

    //
    // currently there is no way to indicate failure.  simply return unmodified option
    // list
    //

    if ( !pClientID )
        return Option;

    pClientID->bHardwareAddressType    = ClientHWType;
    memcpy( pClientID->pbHardwareAddress, ClientHWAddr, ClientHWAddrLength );

    lpNewOption =  DhcpAppendOption(
                         Option,
                         OPTION_CLIENT_ID,
                         (LPBYTE)pClientID,
                         (BYTE)(ClientHWAddrLength + sizeof(BYTE)),
                         OptionEnd );

    DhcpFreeMemory( pClientID );

    return lpNewOption;
}

// data locks on ClassesList must be taken before calling this function
PDHCP_CLASSES PRIVATE                             // the required classes struct
DhcpFindClass(                                    // find a specified class
    IN OUT  PLIST_ENTRY            ClassesList,   // list of classes to srch in
    IN      LPBYTE                 Data,          // non-NULL data bytes
    IN      DWORD                  Len            // # of bytes of above, > 0
) {
    PLIST_ENTRY                    ThisEntry;
    PDHCP_CLASSES                  ThisClass;

    ThisEntry = ClassesList->Flink;               // first element in list
    while( ThisEntry != ClassesList ) {           // search the full list
        ThisClass = CONTAINING_RECORD( ThisEntry, DHCP_CLASSES, ClassList );
        ThisEntry = ThisEntry->Flink;

        if( ThisClass->ClassLen == Len ) {        // lengths must match
            if( ThisClass->ClassName == Data )    // data ptrs can match OR data can match
                return ThisClass;
            if( 0 == memcmp(ThisClass->ClassName, Data, Len) )
                return ThisClass;
        }
    }
    return NULL;
}

// locks on ClassesList should be taken when using this function
LPBYTE                                            // data bytes, or NULL
DhcpAddClass(                                     // add a new class
    IN OUT  PLIST_ENTRY            ClassesList,   // list to add to
    IN      LPBYTE                 Data,          // input class name
    IN      DWORD                  Len            // # of bytes of above
) {
    PDHCP_CLASSES                  Class;
    DWORD                          MemSize;       // amt of memory reqd

    if( NULL == ClassesList ) {                   // invalid parameter
        DhcpAssert( NULL != ClassesList );
        return NULL;
    }

    if( 0 == Len || NULL == Data ) {              // invalid parameters
        DhcpAssert(0 != Len && NULL != Data );
        return NULL;
    }

    Class = DhcpFindClass(ClassesList,Data,Len);  // already there in list?
    if(NULL != Class) {                           // yes, found
        Class->RefCount++;                        // increase ref-count
        return Class->ClassName;
    }

    MemSize = sizeof(*Class)+Len;                 // amt of memory reqd
    Class = (PDHCP_CLASSES) DhcpAllocateMemory(MemSize);
    if( NULL == Class ) {                         // not enough memory
        DhcpAssert( NULL != Class);
        return NULL;
    }

    Class->ClassLen = Len;
    Class->RefCount = 1;
    Class->ClassName = ((LPBYTE)Class) + sizeof(*Class);
    memcpy(Class->ClassName, Data, Len);

    InsertHeadList(ClassesList, &Class->ClassList);

    return Class->ClassName;
}

// locks on ClassesList must be taken before calling this function
DWORD                                             // status
DhcpDelClass(                                     // de-refernce a class
    IN OUT  PLIST_ENTRY            ClassesList,   // the list to delete off
    IN      LPBYTE                 Data,          // the data ptr
    IN      DWORD                  Len            // the # of bytes of above
) {
    PDHCP_CLASSES                  Class;

    if( NULL == ClassesList ) {
        DhcpAssert( NULL != ClassesList );
        return ERROR_INVALID_PARAMETER;
    }

    if( 0 == Len || NULL == Data ) {              // invalid parameter
        DhcpAssert( 0 != Len && NULL != Data );
        return ERROR_INVALID_PARAMETER;
    }

    Class = DhcpFindClass(ClassesList,Data,Len);
    if( NULL == Class ) {                         // did not find this class?
        DhcpAssert( NULL != Class );
        return ERROR_FILE_NOT_FOUND;
    }

    Class->RefCount --;
    if( 0 == Class->RefCount ) {                  // all references removed
        RemoveEntryList( &Class->ClassList );     // remove this from the list
        DhcpFreeMemory(Class);                    // free it
    }

    return ERROR_SUCCESS;
}

// locks on ClassesList must be taken before calling this function
VOID                                              // always succeed
DhcpFreeAllClasses(                               // free each elt of the list
    IN OUT  PLIST_ENTRY            ClassesList    // input list of classes
) {
    PDHCP_CLASSES                  ThisClass;
    PLIST_ENTRY                    ThisEntry;

    if( NULL == ClassesList ) {
        DhcpAssert( NULL != ClassesList && "DhcpFreeAllClasses" );
        return ;
    }

    while( !IsListEmpty(ClassesList) ) {
        ThisEntry = RemoveHeadList(ClassesList);
        ThisClass = CONTAINING_RECORD(ThisEntry, DHCP_CLASSES, ClassList);

        if( ThisClass->RefCount ) {
            DhcpPrint(("Freeing with refcount = %ld\n", ThisClass->RefCount));
        }

        DhcpFreeMemory(ThisClass);
    }

    InitializeListHead(ClassesList);
}

//--------------------------------------------------------------------------------
// exported functions, options
//--------------------------------------------------------------------------------

// data locks need to be taken on OptionsList before calling this function
PDHCP_OPTION                                     // the reqd structure or NULL
DhcpFindOption(                                   // find a specific option
    IN OUT  PLIST_ENTRY            OptionsList,   // the list of options to search
    IN      BYTE                   OptionId,      // the option id to search for
    IN      BOOL                   IsVendor,      // is it vendor specific?
    IN      LPBYTE                 ClassName,     // is there a class associated?
    IN      DWORD                  ClassLen       // # of bytes of above parameter
) {
    PLIST_ENTRY                    ThisEntry;
    PDHCP_OPTION                  ThisOption;

    if( NULL == OptionsList ) {
        DhcpAssert( NULL != OptionsList );
        return NULL;
    }

    ThisEntry = OptionsList->Flink;
    while( ThisEntry != OptionsList ) {           // search the set of options
        ThisOption = CONTAINING_RECORD( ThisEntry, DHCP_OPTION, OptionList );
        ThisEntry  = ThisEntry->Flink;

        if( ThisOption->OptionId != OptionId ) continue;
        if( ThisOption->IsVendor != IsVendor ) continue;
        if( ThisOption->ClassLen != ClassLen ) continue;
        if( ClassLen && ThisOption->ClassName != ClassName )
            continue;                             // mismatched so far

        return ThisOption;                        // found the option
    }

    return NULL;                                  // did not find any match
}

// locks on OptionsList need to be taken before calling this function
DWORD                                             // status
DhcpDelOption(                                    // remove a particular option
    IN      PDHCP_OPTION           ThisOption     // option to delete
) {
    if( NULL == ThisOption)                       // nope, did not find this option
        return ERROR_FILE_NOT_FOUND;

    RemoveEntryList( &ThisOption->OptionList);    // found it.  remove and free
    DhcpFreeMemory(ThisOption);

    return ERROR_SUCCESS;
}

// locks on OptionsList need to be taken before calling this function
DWORD                                             // status
DhcpAddOption(                                    // add a new option
    IN OUT  PLIST_ENTRY            OptionsList,   // list to add to
    IN      BYTE                   OptionId,      // option id to add
    IN      BOOL                   IsVendor,      // is it vendor specific?
    IN      LPBYTE                 ClassName,     // what is the class?
    IN      DWORD                  ClassLen,      // size of above in bytes
    IN      LPBYTE                 Data,          // data for this option
    IN      DWORD                  DataLen,       // # of bytes of above
    IN      time_t                 ExpiryTime     // when the option expires
) {
    PDHCP_OPTION                  ThisOption;
    DWORD                          MemSize;

    if( NULL == OptionsList ) {
        DhcpAssert( NULL != OptionsList && "DhcpAddOption" );
        return ERROR_INVALID_PARAMETER;
    }

    if( 0 != ClassLen && NULL == ClassName ) {
        DhcpAssert( 0 == ClassLen || NULL != ClassName && "DhcpAddOption" );
        return ERROR_INVALID_PARAMETER;
    }

    if( 0 != DataLen && NULL == Data ) {
        DhcpAssert( 0 == DataLen || NULL != Data && "DhcpAddOption" );
        return ERROR_INVALID_PARAMETER;
    }

    MemSize = sizeof(DHCP_OPTION) + DataLen ;
    ThisOption = (PDHCP_OPTION) DhcpAllocateMemory(MemSize);
    if( NULL == ThisOption )                      // could not allocate memory
        return ERROR_NOT_ENOUGH_MEMORY;

    ThisOption->OptionId   = OptionId;
    ThisOption->IsVendor   = IsVendor;
    ThisOption->ClassName  = ClassName;
    ThisOption->ClassLen   = ClassLen;
    ThisOption->ExpiryTime = ExpiryTime;
    ThisOption->DataLen    = DataLen;
    ThisOption->Data       = ((LPBYTE)ThisOption) + sizeof(DHCP_OPTION);
    memcpy(ThisOption->Data, Data, DataLen);

    InsertHeadList( OptionsList, &ThisOption->OptionList );

    return ERROR_SUCCESS;
}



//================================================================================
//   end of file
//================================================================================

