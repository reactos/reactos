//================================================================================
//  Copyright (C) Microsoft Corporation 1997.
//  Author: RameshV
//  Date: 09-Sep-97 06:20
//  Description: Manages the class-id and options information
//================================================================================

#ifndef OPTIONS_H
#define OPTIONS_H

#define MAX_DATA_LEN               255            // atmost 255 bytes for an option

typedef struct _DHCP_CLASSES {                    // common pool of class names
    LIST_ENTRY                     ClassList;     // global list of classes
    LPBYTE                         ClassName;     // name of the class
    DWORD                          ClassLen;      // # of bytes in class name
    DWORD                          RefCount;      // # of references to this
} DHCP_CLASSES, *LPDHCP_CLASSES, *PDHCP_CLASSES;

typedef struct _DHCP_OPTION  {                    // list of options
    LIST_ENTRY                     OptionList;    // the fwd/back ptrs
    BYTE                           OptionId;      // the option value
    BOOL                           IsVendor;      // is this vendor specific
    LPBYTE                         ClassName;     // the class of this option
    DWORD                          ClassLen;      // the length of above option
    time_t                         ExpiryTime;    // when this option expires
    LPBYTE                         Data;          // the data value for this option
    DWORD                          DataLen;       // the # of bytes of above
} DHCP_OPTION , *LPDHCP_OPTION , *PDHCP_OPTION ;

typedef struct _DHCP_OPTION_DEF {
    LIST_ENTRY                     OptionDefList; // list of option definitions
    BYTE                           OptionId;      // the option id
    BOOL                           IsVendor;      // is this vendor specific?
    LPBYTE                         ClassName;     // the class this belongs to
    DWORD                          ClassLen;      // the size of above in bytes

    LPWSTR                         RegSendLoc;    // where is the info about sending this out
    LPWSTR                         RegSaveLoc;    // where is this option going to be stored?
    DWORD                          RegValueType;  // as what value should this be stored?
} DHCP_OPTION_DEF, *LPDHCP_OPTION_DEF, *PDHCP_OPTION_DEF;


//================================================================================
//  exported functions classes
//================================================================================

//--------------------------------------------------------------------------------
// In all of the following functions, ClassesList is unprotected within the fn.
// Caller has to take a lock on it.
//--------------------------------------------------------------------------------
LPBYTE                                            // data bytes, or NULL (no mem)
DhcpAddClass(                                     // add a new class
    IN OUT  PLIST_ENTRY            ClassesList,   // list to add to
    IN      LPBYTE                 Data,          // input class name
    IN      DWORD                  Len            // # of bytes of above
);  // Add the new class into the list or bump up ref count if already there

DWORD                                             // status (FILE_NOT_FOUND => no such class)
DhcpDelClass(                                     // de-refernce a class
    IN OUT  PLIST_ENTRY            ClassesList,   // the list to delete off
    IN      LPBYTE                 Data,          // the data ptr
    IN      DWORD                  Len            // the # of bytes of above
);  // decrease refcount in the list and if becomes zero, free the struct

VOID                                              // always succeeds
DhcpFreeAllClasses(                               // free each elt of the list
    IN OUT  PLIST_ENTRY            ClassesList    // input list of classes
);  // free every class in the list

//--------------------------------------------------------------------------------
// In all the following functions, OptionsList is unprotected within the fn.
// Caller has to take a lock on it.
//--------------------------------------------------------------------------------

PDHCP_OPTION                                      // the reqd structure or NULL
DhcpFindOption(                                   // find a specific option
    IN OUT  PLIST_ENTRY            OptionsList,   // the list of options to search
    IN      BYTE                   OptionId,      // the option id to search for
    IN      BOOL                   IsVendor,      // is it vendor specific?
    IN      LPBYTE                 ClassName,     // is there a class associated?
    IN      DWORD                  ClassLen       // # of bytes of above parameter
);  // search for the required option in the list, return NULL if not found

DWORD                                             // status or ERROR_FILE_NOT_FOUND
DhcpDelOption(                                    // remove a particular option
    IN      PDHCP_OPTION           Option2Delete  // delete this option
);  // delete an existing option in the list, and free up space used

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
);  // replace or add new option to the list.  fail if not enough memory

VOID                                              // always succeeds
DhcpFreeAllOptions(                               // frees all the options
    IN OUT  PLIST_ENTRY            OptionsList    // input list of options
);  // free every option in the list

time_t                                            // 0 || time for next expiry (absolute)
DhcpGetExpiredOptions(                            // delete all expired options
    IN OUT  PLIST_ENTRY            OptionsList,   // list to search frm
    OUT     PLIST_ENTRY            ExpiredOptions // o/p list of expired options
);  // move expired options between lists and return timer. 0 => switch off timer.

//--------------------------------------------------------------------------------
//  In all the following functions, OptionsDefList is unprotected.  Caller has
//  to take a lock on it.
//--------------------------------------------------------------------------------

DWORD                                             // status
DhcpAddOptionDef(                                 // add a new option definition
    IN OUT  PLIST_ENTRY            OptionDefList, // input list of options to add to
    IN      BYTE                   OptionId,      // option to add
    IN      BOOL                   IsVendor,      // is it vendor specific
    IN      LPBYTE                 ClassName,     // name of class it belongs to
    IN      DWORD                  ClassLen,      // the size of above in bytes
    IN      LPWSTR                 RegSendLoc,    // where to get info about sending this out
    IN      LPWSTR                 RegSaveLoc,    // where to get info about saving this
    IN      DWORD                  ValueType      // what is the type when saving it?
);

PDHCP_OPTION_DEF                                  // NULL, or requested option def
DhcpFindOptionDef(                                // search for a particular option
    IN      PLIST_ENTRY            OptionDefList, // list to search in
    IN      BYTE                   OptionId,      // the option id to search for
    IN      BOOL                   IsVendor,      // is it vendor specific
    IN      LPBYTE                 ClassName,     // the class, if one exists
    IN      DWORD                  ClassLen       // # of bytes of class name
);

DWORD                                             // status
DhcpDelOptionDef(                                 // delete a particular option def
    IN      PLIST_ENTRY            OptionDefList, // list to delete from
    IN      BYTE                   OptionId,      // the option id to delete
    IN      BOOL                   IsVendor,      // is it vendor specific
    IN      LPBYTE                 ClassName,     // the class, if one exists
    IN      DWORD                  ClassLen       // # of bytes of class name
);

VOID
DhcpFreeAllOptionDefs(                            // free each element of a list
    IN OUT  PLIST_ENTRY            OptionDefList, // the list to free
    IN OUT  PLIST_ENTRY            ClassesList    // classes to de-ref off
);

BOOL                                              // TRUE==>found..
DhcpOptionsFindDomain(                            // find the domain name option values
    IN OUT  PDHCP_CONTEXT          DhcpContext,   // for this adapter
    OUT     LPBYTE                *Data,          // fill this ptr up
    OUT     LPDWORD                DataLen
);

#endif  OPTIONS_H
