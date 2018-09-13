//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//


//
// Define the severity codes
//


//
// MessageId: IPP_E_FIRST
//
// MessageText:
//
//  Internet Publishing Provider first error message
//
#define IPP_E_FIRST                      0x40048000L

//
// MessageId: IPP_E_SYNCCONFLICT
//
// MessageText:
//
//  The server resource has changed since the local copy on your computer was obtained.
//
#define IPP_E_SYNCCONFLICT               0xC0048003L

//
// MessageId: IPP_E_FILENOTDIRTY
//
// MessageText:
//
//  The copy of the resource on your computer has not been modified since it was downloaded from the server.
//
#define IPP_E_FILENOTDIRTY               0xC0048004L

//
// MessageId: IPP_E_MARKFOROFFLINE_FAILED
//
// MessageText:
//
//  The attempt to mark or unmark the resource for offline use failed.
//
#define IPP_E_MARKFOROFFLINE_FAILED      0xC0048006L

//
// MessageId: IPP_E_OFFLINE
//
// MessageText:
//
//  The requested operation could not be completed because the resource is offline.
//
#define IPP_E_OFFLINE                    0xC0048007L

//
// MessageId: IPP_E_UNSYNCHRONIZED
//
// MessageText:
//
//  The requested operation could not be completed because the resource has been modified
//  on your computer but has not been synchronized with the server.
//
#define IPP_E_UNSYNCHRONIZED             0xC0048008L

//
// MessageId: IPP_E_SERVERTYPE_NOT_SUPPORTED
//
// MessageText:
//
//  This server type is not currently supported.
//
#define IPP_E_SERVERTYPE_NOT_SUPPORTED   0xC004800AL

//
// MessageId: IPP_E_MDAC_VERSION
//
// MessageText:
//
//  The Microsoft Data Access Components (MDAC) are either not present on this computer or are an old version. (MSDAIPP 1.0 requires MDAC 2.1)
//
#define IPP_E_MDAC_VERSION               0xC004800DL

//
// MessageId: IPP_E_COLLECTIONEXISTS
//
// MessageText:
//
//  The move or copy operation failed because a collection with that name already exists.
//
#define IPP_E_COLLECTIONEXISTS           0xC004800EL

//
// MessageId: IPP_E_CANNOTCREATEOFFLINE
//
// MessageText:
//
//  The requested resource could not be created because parent cache entry does not exist.
//
#define IPP_E_CANNOTCREATEOFFLINE        0xC004800FL

//
// MessageId: IPP_E_STATUS_CANNOTCOMPLETE
//
// MessageText:
//
//  This is an internal MSDAIPP.DLL error.
//
#define IPP_E_STATUS_CANNOTCOMPLETE      0xC0048101L

//
// MessageId: IPP_E_RESELECTPROVIDER
//
// MessageText:
//
//  This is an internal MSDAIPP.DLL error.
//
#define IPP_E_RESELECTPROVIDER           0xC0048102L

//
// MessageId: IPP_E_CLIENTMUSTEMULATE
//
// MessageText:
//
//  This is an internal MSDAIPP.DLL error.
//
#define IPP_E_CLIENTMUSTEMULATE          0xC0048103L

//
// MessageId: IPP_S_WEAKRESERVE
//
// MessageText:
//
//  This is an internal MSDAIPP.DLL error.
//
#define IPP_S_WEAKRESERVE                0x00048104L

//
// MessageId: IPP_S_TRUNCATED
//
// MessageText:
//
//  This is an internal MSDAIPP.DLL error.
//
#define IPP_S_TRUNCATED                  0x00048105L

//
// MessageId: IPP_E_LAST
//
// MessageText:
//
//  Internet Publishing Provider last error message
//
#define IPP_E_LAST                       0x40048106L

