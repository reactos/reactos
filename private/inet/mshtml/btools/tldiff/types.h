#ifndef types_h
#define types_h 1

//
//  Types of description blocks that can be hit.
//
enum BLOCK_TYPE
{
        BLK_NONE,       //single line information. comments, empty lines, [...] headers
        BLK_DISPINT,    //dispatch interface description block
        BLK_INTERFACE,  //interface description block
        BLK_COCLASS,    //coclass description block
        BLK_TYPEDEF,    //typedef description block
        BLK_ATTR        //attribute block for interface, coclass or typedef
};

//
//  The structure used for block index records.
//
typedef struct {
    BLOCK_TYPE  blockType;
    unsigned long ulStartPos;
    unsigned long ulEndPos;
    unsigned long ulAttrStartPos;
    unsigned long ulAttrEndPos;
    bool          fCopied;
}INDEX;

//
// The structure used for making it faster to access and compare 
// data about the methods in an interface.
//
typedef struct {
    unsigned long ulAttrStart;
    unsigned long ulAttrEnd;
    unsigned long ulNameStart;
    unsigned long ulNameEnd;
    unsigned long ulParamStart;
    unsigned long ulParamEnd;
    unsigned long ulMethodNameStart;
    bool          fUsed;
}LINEINFO;

typedef struct {
    unsigned long   ulAttrStart;
    unsigned long   ulAttrLength;
    bool            fUsed;
}ATTRINFO;

typedef struct {
    unsigned long   ulTypeStart;
    unsigned long   ulTypeLength;
    unsigned long   ulNameStart;
    unsigned long   ulNameLength;
    unsigned long   ulParamLength;
    bool            fUsed;
}PARAMINFO;

//
//  lengths of keywords that are used, to ease maintenance
//
#define LEN_DISPINT     13  //dispinterface
#define LEN_INTERFACE   9   //interface
#define LEN_COCLASS     7   //coclass
#define LEN_TYPEDEF     7   //typedef

//
//  Granularity for memory allocation.
//
#define SZ  512


//
//  Return value flags for the application
//
#define CHANGE_ADDINTERFACE         0x00000001  //  A new interface was added 
#define CHANGE_ADDDISPINT           0x00000002  //  A new dispinterface was added
#define CHANGE_ADDCOCLASS           0x00000004  //  A new coclass was added
#define CHANGE_ADDATTRIBUTE         0x00000008  //  A new attribute was added
#define CHANGE_REMOVEFROMINT        0x00000010  //  A method/property was removed from an interface
#define CHANGE_REMOVEFROMDISPINT    0x00000020  //  A method/property was removed from a dispinterface
#define CHANGE_METHODONINT          0x00000040  //  A method/property was added to an interface
#define CHANGE_METHODONDISPINT      0x00000080  //  A method/property was added to an dispinterface
#define CHANGE_PARAMCHANGE          0x00000100  //  A method's parameters were changed.
#define CHANGE_ATTRCHANGE           0x00000200  //  A method's attributes were changed
#define CHANGE_REMOVEFROMCOCLASS    0x00000400  //  An interface was removed from a coclass
#define CHANGE_ADDTOCOCLASS         0x00000800  //  An interface was added to a coclass
#define CHANGE_RETVALCHANGE         0x00001000  //  A  method's return value has been modified
#define CHANGE_BLOCKREMOVED         0x00002000  //  A coclass was removed
#define CHANGE_DUALATTRADDED        0x00004000  //  the attribute 'dual' was added to an interface/dispinterface
#define CHANGE_DUALATTRREMOVED      0x00008000  //  the attribute 'dual' was removed from an interface/dispinterface
#define CHANGE_UUIDHASCHANGED       0x00010000  //  GUID was changed for a coclass or an interface

#endif  // def types_h
