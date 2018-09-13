/*++

Copyright (c) 1996-1998 Microsoft Corporation

Module Name:

    fileqcb.c

Abstract:

    Routines to call out to file queue callbacks, translating
    character types as necessary.

Author:

    Ted Miller (tedm) Feb-1996

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


#ifdef UNICODE

//
// Define structure that describes, for a given structure, how to
// thunk it back and forth between ANSI and Unicode.
//
typedef struct _STRUCT_THUNK_DATA {
    //
    // Size of the structure
    //
    unsigned StructureSize;
    //
    // Offsets of members that are pointers to strings
    // that need conversion before calling the callback function.
    // A -1 terminates the list.
    //
    int StringMemberOffsets[5];
    //
    // Offsets of DWORD members that need to be copied back from
    // the temporary structure back into the caller's one.
    //
    int OutputDwordOffsets[2];
    //
    // Offsets of strings that need to be converted in place
    // after the callback has occurred.
    //
    int OutputStringOffsets[2];

} STRUCT_THUNK_DATA, *PSTRUCT_THUNK_DATA;

//
// Define enum for data types we care about for the setup message
// notification mechanism.
//
typedef enum {
    FileMsgFilepaths,           // FILEPATHS
    FileMsgSourcemedia,         // SOURCE_MEDIA
    FileMsgCabinetinfo,         // CABINET_INFO
    FileMsgFileincabinfo,       // FILE_IN_CABINET_INFO
    FileMsgNone,                // No translation (special case)
    FileMsgString,              // Plain string (special case)
    FileMsgStringOut            // String written by callback (special case)
} FileMsgStruct;

//
// Instantiate structure thunk data for structures we care about.
//
STRUCT_THUNK_DATA StructThunkData[] = {

                //
                // FILEPATHS structure
                //
                {
                    sizeof(FILEPATHS),

                    {
                        offsetof(FILEPATHS,Target),
                        offsetof(FILEPATHS,Source),
                        -1
                    },

                    { offsetof(FILEPATHS,Win32Error),-1 }, { -1 }
                },

                //
                // SOURCE_MEDIA structure
                //
                {
                    sizeof(SOURCE_MEDIA),

                    {
                        offsetof(SOURCE_MEDIA,Tagfile),
                        offsetof(SOURCE_MEDIA,Description),
                        offsetof(SOURCE_MEDIA,SourcePath),
                        offsetof(SOURCE_MEDIA,SourceFile),
                        -1
                    },

                    { -1 }, { -1 }
                },

                //
                // CABINET_INFO structure
                //
                {
                    sizeof(CABINET_INFO),

                    {
                        offsetof(CABINET_INFO,CabinetPath),
                        offsetof(CABINET_INFO,CabinetFile),
                        offsetof(CABINET_INFO,DiskName),
                        -1
                    },

                    { -1 }, { -1 }
                },

                //
                // FILE_IN_CABINET_INFO structure
                //
                {
                    sizeof(FILE_IN_CABINET_INFO),

                    {
                        offsetof(FILE_IN_CABINET_INFO,NameInCabinet),
                        -1
                    },

                    { offsetof(FILE_IN_CABINET_INFO,Win32Error),-1 },
                    { offsetof(FILE_IN_CABINET_INFO,FullTargetName),-1 }
                }
            };


//
// Define structure that describes how to translate messages
// from ANSI<-->Unicode for all notification messages that we send out.
//
typedef struct _MSG_THUNK_DATA {
    DWORD Notification;
    BOOL UseMask;
    FileMsgStruct Param1Type;
    FileMsgStruct Param2Type;
    UINT OutOfMemoryReturn;
} MSG_THUNK_DATA, *PMSG_THUNK_DATA;

//
// Instantiate message thunk data.
//
// (jamiehun) fixed bug (ENDDELETE etc were not flagged as FileMsgFilepaths)
MSG_THUNK_DATA MsgThunkData[] =
{
 { SPFILENOTIFY_STARTQUEUE,    FALSE,FileMsgNone         ,FileMsgNone     ,FALSE                   },
 { SPFILENOTIFY_ENDQUEUE,      FALSE,FileMsgNone         ,FileMsgNone     ,0                       },
 { SPFILENOTIFY_STARTSUBQUEUE, FALSE,FileMsgNone         ,FileMsgNone     ,FALSE                   },
 { SPFILENOTIFY_ENDSUBQUEUE,   FALSE,FileMsgNone         ,FileMsgNone     ,FALSE                   },
 { SPFILENOTIFY_STARTDELETE,   FALSE,FileMsgFilepaths    ,FileMsgNone     ,FILEOP_ABORT            },
 { SPFILENOTIFY_ENDDELETE,     FALSE,FileMsgFilepaths    ,FileMsgNone     ,0                       },
 { SPFILENOTIFY_DELETEERROR,   FALSE,FileMsgFilepaths    ,FileMsgNone     ,FILEOP_ABORT            },
 { SPFILENOTIFY_STARTRENAME,   FALSE,FileMsgFilepaths    ,FileMsgNone     ,FILEOP_ABORT            },
 { SPFILENOTIFY_ENDRENAME,     FALSE,FileMsgFilepaths    ,FileMsgNone     ,0                       },
 { SPFILENOTIFY_RENAMEERROR,   FALSE,FileMsgFilepaths    ,FileMsgNone     ,FILEOP_ABORT            },
 { SPFILENOTIFY_STARTCOPY,     FALSE,FileMsgFilepaths    ,FileMsgNone     ,FILEOP_ABORT            },
 { SPFILENOTIFY_ENDCOPY,       FALSE,FileMsgFilepaths    ,FileMsgNone     ,0                       },
 { SPFILENOTIFY_COPYERROR,     FALSE,FileMsgFilepaths    ,FileMsgStringOut,FILEOP_ABORT            },
 { SPFILENOTIFY_NEEDMEDIA,     FALSE,FileMsgSourcemedia  ,FileMsgStringOut,FILEOP_ABORT            },
 { SPFILENOTIFY_QUEUESCAN,     FALSE,FileMsgString       ,FileMsgNone     ,ERROR_NOT_ENOUGH_MEMORY },
 { SPFILENOTIFY_QUEUESCAN_EX,  FALSE,FileMsgFilepaths    ,FileMsgNone     ,ERROR_NOT_ENOUGH_MEMORY },
 { SPFILENOTIFY_CABINETINFO,   FALSE,FileMsgCabinetinfo  ,FileMsgNone     ,ERROR_NOT_ENOUGH_MEMORY },
 { SPFILENOTIFY_FILEINCABINET, FALSE,FileMsgFileincabinfo,FileMsgString   ,FILEOP_ABORT            },
 { SPFILENOTIFY_NEEDNEWCABINET,FALSE,FileMsgCabinetinfo  ,FileMsgStringOut,ERROR_NOT_ENOUGH_MEMORY },
 { SPFILENOTIFY_FILEEXTRACTED, FALSE,FileMsgFilepaths    ,FileMsgNone     ,ERROR_NOT_ENOUGH_MEMORY },
 { SPFILENOTIFY_FILEOPDELAYED, FALSE,FileMsgFilepaths    ,FileMsgNone     ,0                       },
 { SPFILENOTIFY_STARTBACKUP,   FALSE,FileMsgFilepaths    ,FileMsgNone     ,FILEOP_ABORT            },
 { SPFILENOTIFY_ENDBACKUP,     FALSE,FileMsgFilepaths    ,FileMsgNone     ,0                       },
 { SPFILENOTIFY_BACKUPERROR,   FALSE,FileMsgFilepaths    ,FileMsgNone     ,FILEOP_ABORT            },

 { SPFILENOTIFY_LANGMISMATCH
 | SPFILENOTIFY_TARGETEXISTS
 | SPFILENOTIFY_TARGETNEWER,   TRUE ,FileMsgFilepaths    ,FileMsgNone     ,FALSE                   }
};



//
// Forward references.
//
BOOL
pSetupConvertMsgHandlerArgs(
    IN  UINT  Notification,
    IN  UINT_PTR Param1,
    IN  UINT_PTR Param2,
    OUT PUINT_PTR NewParam1,
    OUT PUINT_PTR NewParam2,
    IN  BOOL  ToAnsi,
    OUT PMSG_THUNK_DATA *ThunkData
    );

BOOL
pThunkSetupMsgParam(
    IN  FileMsgStruct StructType,
    IN  UINT_PTR      Param,
    OUT UINT_PTR     *NewParam,
    IN  BOOL          ToAnsi
    );

VOID
pUnthunkSetupMsgParam(
    IN     FileMsgStruct StructType,
    IN OUT UINT_PTR      OriginalParam,
    IN OUT UINT_PTR      ThunkedParam,
    IN     BOOL          FreeOnly,
    IN     BOOL          ThunkedToAnsi
    );


UINT
pSetupCallMsgHandler(
    IN PVOID MsgHandler,
    IN BOOL  MsgHandlerIsNativeCharWidth,
    IN PVOID Context,
    IN UINT  Notification,
    IN UINT_PTR Param1,
    IN UINT_PTR Param2
    )

/*++

Routine Description:

    Call out to a SP_FILE_CALLBACK routine, translating arguments from
    Unicode to ANSI as necessary, and marshalling data back into Unicode
    as necessary.

    Conversions and marshalling occur only for messages we recognize
    (ie, that are in the MsgThunkData array). Unrecognized messages
    are assumed to be private and are passed through unchanged.

    If a Unicode->ANSI conversion fails due to an out of memory condition,
    this routine sets last error to ERROR_NOT_ENOUGH_MEMORY and returns
    the value specified in the relevent MsgThunkData structure.

Arguments:

    MsgHandler - supplies pointer to callback routine. Can be either
        a routine expecting ANSI args or Unicode args, as specified
        by MsgHandlerIsNativeCharWidth.

    MsgHandlerIsNativeCharWidth - supplies flag indicating whether callback
        functionexpects Unicode (TRUE) or ANSI (FALSE) arguments.

    Context - supplies context data meaningful to the callback
        routine. Not interpreted by this routine, merely passed on.

    Notification - supplies notification code to be passed to the callback.

    Param1 - supplies first notification-specific parameter to be passed
        to the callback.

    Param2 - supplies second notification-specific parameter to be passed
        to the callback.

Return Value:

    Return code specific to the notification.

--*/

{
    PSP_FILE_CALLBACK_A MsgHandlerA;
    PSP_FILE_CALLBACK_W MsgHandlerW;
    UINT rc,ec;
    UINT_PTR Param1A,Param2A;
    BOOL b;
    PMSG_THUNK_DATA ThunkData;

    //
    // If already unicode, nothing to do, just call the msghandler.
    //
    if(MsgHandlerIsNativeCharWidth) {
        MsgHandlerW = MsgHandler;
        return(MsgHandlerW(Context,Notification,Param1,Param2));
    }

    //
    // Optimization: if we are going to call the ANSI version of the
    // default queue callback routine SetupDefaultQueueCallbackA(),
    // we can avoid converting the args, since they'll just be converted
    // right back again by that API as a prelude to calling the
    // actual implementation SetupDefaultQueueCallbackW().
    //
    if(MsgHandler == SetupDefaultQueueCallbackA) {
        return(SetupDefaultQueueCallbackW(Context,Notification,Param1,Param2));
    }

    //
    // Target callback function is expecting ANSI arguments.
    //
    b = pSetupConvertMsgHandlerArgs(
            Notification,
            Param1,
            Param2,
            &Param1A,
            &Param2A,
            TRUE,
            &ThunkData
            );

    if(!b) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(ThunkData->OutOfMemoryReturn);
    }

    //
    // Agrs are ready; call out to the ANSI callback.
    //
    MsgHandlerA = MsgHandler;
    rc = MsgHandlerA(Context,Notification,Param1A,Param2A);
    ec = GetLastError();

    //
    // Free the temporary thunk structs and marshall data back into
    // the original structures as necessary.
    //
    if(ThunkData) {
        pUnthunkSetupMsgParam(ThunkData->Param1Type,Param1,Param1A,FALSE,TRUE);
        pUnthunkSetupMsgParam(ThunkData->Param2Type,Param2,Param2A,FALSE,TRUE);
    }

    SetLastError(ec);
    return(rc);
}


UINT
pSetupCallDefaultMsgHandler(
    IN PVOID Context,
    IN UINT  Notification,
    IN UINT_PTR Param1,
    IN UINT_PTR Param2
    )
{
    UINT_PTR Param1U,Param2U;
    BOOL b;
    PMSG_THUNK_DATA ThunkData;
    UINT rc,ec;

    //
    // Thunk args to Unicode.
    //
    b = pSetupConvertMsgHandlerArgs(
            Notification,
            Param1,
            Param2,
            &Param1U,
            &Param2U,
            FALSE,
            &ThunkData
            );

    if(!b) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(ThunkData->OutOfMemoryReturn);
    }

    //
    // Agrs are ready; call the default queue callback.
    //
    rc = SetupDefaultQueueCallbackW(Context,Notification,Param1U,Param2U);
    ec = GetLastError();

    //
    // Free the temporary thunk structs and marshall data back into
    // the original structures as necessary.
    //
    if(ThunkData) {
        pUnthunkSetupMsgParam(ThunkData->Param1Type,Param1,Param1U,FALSE,FALSE);
        pUnthunkSetupMsgParam(ThunkData->Param2Type,Param2,Param2U,FALSE,FALSE);
    }

    SetLastError(ec);
    return(rc);
}



BOOL
pSetupConvertMsgHandlerArgs(
    IN  UINT  Notification,
    IN  UINT_PTR Param1,
    IN  UINT_PTR Param2,
    OUT PUINT_PTR NewParam1,
    OUT PUINT_PTR NewParam2,
    IN  BOOL  ToAnsi,
    OUT PMSG_THUNK_DATA *ThunkData
    )

/*++

Routine Description:

    Locate thunk description data for a given notification and convert
    parameters from Unicode to ANSI or ANSI to Unicode.

Arguments:

    Notification - supplies notification code to be passed to the callback.

    Param1 - supplies first notification-specific parameter to be passed
        to the callback, which is to be converted.

    Param2 - supplies second notification-specific parameter to be passed
        to the callback, which is to be converted.

    NewParam1 - receives first notification-specific parameter to be passed
        to the callback.

    NewParam2 - receives second notification-specific parameter to be passed
        to the callback.

    ToAnsi - supplies flag indicating whether parameters are to be converted
        from ANSI to Unicode or Unicode to ANSI.

    ThunkData - if the Notification is recognized, receives a pointer to
        the MSG_THUNK_DATA for the given Notification. If not recognized,
        receives NULL.

Return Value:

    Boolean value indicating whether conversion was successful.
    If FALSE, the caller can assume out of memory.

--*/

{
    unsigned u;
    PMSG_THUNK_DATA thunkData;
    BOOL KnownMessage;

    //
    // Locate the msg-specific thunk data descriptor.
    //
    KnownMessage = FALSE;
    for(u=0; !KnownMessage && (u<(sizeof(MsgThunkData)/sizeof(MsgThunkData[0]))); u++) {

        thunkData = &MsgThunkData[u];

        if(thunkData->UseMask) {
            KnownMessage = ((thunkData->Notification & Notification) != 0);
        } else {
            KnownMessage = (thunkData->Notification == Notification);
        }
    }

    if(!KnownMessage) {
        //
        // Unknown message; must be private. Just pass args on as-is.
        //
        *NewParam1 = Param1;
        *NewParam2 = Param2;
        *ThunkData = NULL;
    } else {
        //
        // Got a message we understand. Thunk it.
        //
        *ThunkData = thunkData;

        if(!pThunkSetupMsgParam(thunkData->Param1Type,Param1,NewParam1,ToAnsi)) {
            return(FALSE);
        }

        if(!pThunkSetupMsgParam(thunkData->Param2Type,Param2,NewParam2,ToAnsi)) {
            pUnthunkSetupMsgParam(thunkData->Param1Type,Param1,*NewParam1,TRUE,ToAnsi);
            return(FALSE);
        }
    }

    return(TRUE);
}


BOOL
pThunkSetupMsgParam(
    IN  FileMsgStruct StructType,
    IN  UINT_PTR      Param,
    OUT UINT_PTR     *NewParam,
    IN  BOOL          ToAnsi
    )

/*++

Routine Description:

    Convert a parameter to a setup notification callback from ANSI to
    Unicode or Unicode to ANSI as necessary. Allocates all required memory
    and performs conversions.

Arguments:

    StructType - supplies type of data represented by Param.

    Param - supplies parameter to be converted.

    NewParam - receives new parameter. Caller should free via
        pUnthunkSetupMsgParam when done.

    ToAnsi - if FALSE, Param is to be converted from ANSI to Unicode.
        If TRUE, Param is to be converted from Unicode to ANSI.

Return Value:

    Boolean value indicating whether conversion occured successfully.
    If FALSE, the caller can assume out of memory.

--*/

{
    unsigned u,v;
    PUCHAR newStruct;
    PVOID OldString;
    PVOID NewString;

    //
    // Handle special cases here.
    //
    switch(StructType) {

    case FileMsgNone:
        *NewParam = Param;
        return(TRUE);

    case FileMsgStringOut:
        //
        // Callee will write string data which we will convert later.
        //
        if(*NewParam = (UINT_PTR)MyMalloc(MAX_PATH * (ToAnsi ? sizeof(CHAR) : sizeof(WCHAR)))) {
            if(ToAnsi) {
                *(PCHAR)(*NewParam) = 0;
            } else {
                *(PWCHAR)(*NewParam) = 0;
            }
        }
        return(*NewParam != 0);

    case FileMsgString:
        if(ToAnsi) {
            *NewParam = (UINT_PTR)UnicodeToAnsi((PCWSTR)Param);
        } else {
            *NewParam = (UINT_PTR)AnsiToUnicode((PCSTR)Param);
        }
        return(*NewParam != 0);
    }

    newStruct = MyMalloc(StructThunkData[StructType].StructureSize);
    if(!newStruct) {
        return(FALSE);
    }

    CopyMemory(newStruct,(PVOID)Param,StructThunkData[StructType].StructureSize);

    for(u=0; StructThunkData[StructType].StringMemberOffsets[u] != -1; u++) {

        OldString = *(PVOID *)((PUCHAR)Param + StructThunkData[StructType].StringMemberOffsets[u]);

        if(OldString) {
            if(ToAnsi) {
                NewString = UnicodeToAnsi(OldString);
            } else {
                NewString = AnsiToUnicode(OldString);
            }

            if(!NewString) {
                for(v=0; v<u; v++) {
                    MyFree(*(PVOID *)(newStruct + StructThunkData[StructType].StringMemberOffsets[v]));
                }

                MyFree(newStruct);
                return(FALSE);
            }

            *(PVOID *)(newStruct + StructThunkData[StructType].StringMemberOffsets[u]) = NewString;
        }
    }

    *NewParam = (UINT_PTR)newStruct;
    return(TRUE);
}


VOID
pUnthunkSetupMsgParam(
    IN     FileMsgStruct StructType,
    IN OUT UINT_PTR      OriginalParam,
    IN OUT UINT_PTR      ThunkedParam,
    IN     BOOL          FreeOnly,
    IN     BOOL          ThunkedToAnsi
    )

/*++

Routine Description:

    Marshal data output by a callback function back into the original
    Unicode or ANSI structure. Also, free the temporary structure and all
    its resources.

Arguments:

    StructType - supplies type of data being operated on.

    OriginalParam - supplies original parameter. DWORD fields and
        in-place strings in this structure will be updated by this routine.

    ThunkedParam - supplies temporary ANSI or Unicode parameter.

    FreeOnly - if TRUE, no marshalling occurs but ThunkedParam will be freed.

Return Value:

    None.

--*/

{
    unsigned u;
    PVOID String;
    PVOID SourceString;
    DWORD d;

    //
    // Special cases here.
    //
    switch(StructType) {

    case FileMsgNone:
        //
        // Nothing to do for this one.
        //
        return;

    case FileMsgStringOut:
        //
        // Callee wrote string data; convert as appropriate.
        //
        if(!FreeOnly) {

            if(ThunkedToAnsi) {

                MultiByteToWideChar(
                    CP_ACP,
                    0,
                    (PCSTR)ThunkedParam,
                    -1,
                    (PWCHAR)OriginalParam,
                    MAX_PATH
                    );

            } else {

                WideCharToMultiByte(
                    CP_ACP,
                    0,
                    (PCWSTR)ThunkedParam,
                    -1,
                    (PCHAR)OriginalParam,
                    MAX_PATH,
                    NULL,
                    NULL
                    );
            }
        }
        MyFree((PVOID)ThunkedParam);
        return;

    case FileMsgString:
        //
        // Simple string.
        //
        MyFree((PVOID)ThunkedParam);
        return;
    }

    //
    // Free all strings.
    //
    for(u=0; StructThunkData[StructType].StringMemberOffsets[u] != -1; u++) {

        String = *(PVOID *)((PUCHAR)ThunkedParam + StructThunkData[StructType].StringMemberOffsets[u]);

        if(String) {
            MyFree(String);
        }
    }

    //
    // Marshall data back to Unicode structure
    //
    if(!FreeOnly) {
        //
        // Copy DWORD data from the thunk struct back to the original struct.
        //
        for(u=0; StructThunkData[StructType].OutputDwordOffsets[u] != -1; u++) {

            d = *(DWORD *)((PUCHAR)ThunkedParam + StructThunkData[StructType].OutputDwordOffsets[u]);

            *(DWORD *)((PUCHAR)OriginalParam + StructThunkData[StructType].OutputDwordOffsets[u]) = d;
        }

        //
        // Convert output strings.
        //
        for(u=0; StructThunkData[StructType].OutputStringOffsets[u] != -1; u++) {

            SourceString = (PUCHAR)ThunkedParam  + StructThunkData[StructType].OutputStringOffsets[u];
            String       = (PUCHAR)OriginalParam + StructThunkData[StructType].OutputStringOffsets[u];

            if(ThunkedToAnsi) {
                MultiByteToWideChar(
                    CP_ACP,
                    0,
                    SourceString,
                    -1,
                    String,
                    MAX_PATH
                    );
            } else {
                WideCharToMultiByte(
                    CP_ACP,
                    0,
                    SourceString,
                    -1,
                    String,
                    MAX_PATH,
                    NULL,
                    NULL
                    );
            }
        }
    }

    MyFree((PVOID)ThunkedParam);
}



#else

UINT
pSetupCallMsgHandler(
    IN PVOID MsgHandler,
    IN BOOL  MsgHandlerIsNativeCharWidth,
    IN PVOID Context,
    IN UINT  Notifiction,
    IN UINT_PTR Param1,
    IN UINT_PTR Param2
    )
{
    PSP_FILE_CALLBACK_A msghandler;

    UNREFERENCED_PARAMETER(MsgHandlerIsNativeCharWidth);
    MYASSERT(MsgHandlerIsNativeCharWidth);

    //
    // ANSI version is a no-op
    //
    msghandler = MsgHandler;
    return(msghandler(Context,Notifiction,Param1,Param2));
}

#endif
