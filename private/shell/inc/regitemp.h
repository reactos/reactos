//
//  RegItemP.h  - header for regitem IDLists
//  
//  NOTE - these structures cannot be changed for any reason.
//

#ifndef _REGITEMP_H_
#define _REGITEMP_H_

#ifndef NOPRAGMAS
#pragma pack(1)
#endif

typedef struct _IDREGITEM
{
    WORD    cb;
    BYTE    bFlags;
    BYTE    bOrder;
    CLSID   clsid;
} IDREGITEM;
typedef UNALIGNED IDREGITEM *LPIDREGITEM;
typedef const UNALIGNED IDREGITEM *LPCIDREGITEM;


typedef struct
{
    IDREGITEM       idri;
    USHORT          cbNext;
} IDLREGITEM;           // "RegItem" IDList
typedef const UNALIGNED IDLREGITEM *LPCIDLREGITEM;

#ifndef NOPRAGMAS
#pragma pack()
#endif


// stolen from shell32\shitemid.h
#ifndef SHID_ROOTEDREGITEM
#define SHID_ROOTEDREGITEM       0x1e    //
#endif //SHID_ROOTEDREGITEM

// stolen from shell32\shitemid.h
#ifndef SHID_ROOT_REGITEM
#define SHID_ROOT_REGITEM        0x1f    //
#endif //SHID_ROOT_REGITEM

#endif // _REGITEMP_H_
