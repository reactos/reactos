//------------------------------------------------------------------------------
// CLSTYPES.H
//
// Structures and typedefs to ease the life of a class file data reader...
//------------------------------------------------------------------------------
#ifndef _CLSTYPES_INCLUDED
#define _CLSTYPES_INCLUDED

//------------------------------------------------------------------------------
// Access flags, taken directly from Sun's Java VM Specification (Aug 21 1995)
//------------------------------------------------------------------------------
#define ACC_PUBLIC          0x0001
#define ACC_PRIVATE         0x0002
#define ACC_PROTECTED       0x0004
#define ACC_STATIC          0x0008
#define ACC_FINAL           0x0010
#define ACC_SYNCHRONIZED    0x0020
#define ACC_VOLATILE        0x0040
#define ACC_TRANSIENT       0x0080
#define ACC_NATIVE          0x0100
#define ACC_INTERFACE       0x0200
#define ACC_ABSTRACT        0x0400

//------------------------------------------------------------------------------
// Constant tags, also taken from the VM spec
//------------------------------------------------------------------------------
#define CONSTANT_Class                  7
#define CONSTANT_Fieldref               9
#define CONSTANT_Methodref              10
#define CONSTANT_InterfaceMethodref     11
#define CONSTANT_String                 8
#define CONSTANT_Integer                3
#define CONSTANT_Float                  4
#define CONSTANT_Long                   5
#define CONSTANT_Double                 6
#define CONSTANT_NameAndType            12
#define CONSTANT_Utf8                   1
#define CONSTANT_Unicode                2

//------------------------------------------------------------------------------
// Basic types
//------------------------------------------------------------------------------
typedef unsigned char       U1;
typedef unsigned short      U2;
typedef unsigned long       U4;
typedef unsigned __int64    U8;

//------------------------------------------------------------------------------
// Helper structs
//------------------------------------------------------------------------------
struct longdbl
{
    union
    {
        U8      iValue;
        double  fValue;
        struct
        {
            U4  iLow;
            U4  iHigh;
        };
    };
};

//------------------------------------------------------------------------------
// CPINFO -- constant pool entry structure (union).  Note that all values are
// swapped into little-endian for you, but strings are still UTF8...
//------------------------------------------------------------------------------
struct cp_info
{
    U1      iTag;       // CONSTANT_* tag
    union
    {
        struct
        {
            U2      iName;
        } Class;                // CONSTANT_Class;
        struct
        {
            U2      iClass;
            U2      iNameAndType;
        } Fieldref;             // CONSTANT_Fieldref
        struct
        {
            U2      iClass;
            U2      iNameAndType;
        } Methodref;            // CONSTANT_Methodref
        struct
        {
            U2      iClass;
            U2      iNameAndType;
        } InterfaceMethodref;   // CONSTANT_InterfaceMethodref
        struct
        {
            U2      iIndex;
        } String;               // CONSTANT_String
        struct
        {
            U4      iValue;
        } Integer;              // CONSTANT_Integer
        struct
        {
            float   fValue;
        } Float;                // CONSTANT_Float
        struct
        {
            longdbl *pVal;
        } Long;                 // CONSTANT_Long
        struct _DoubleStruct
        {
            longdbl *pVal;
        } Double;               // CONSTANT_Double
        struct
        {
            U2      iName;
            U2      iSignature;
        } NameAndType;          // CONSTANT_NameAndType
        struct
        {
            U2      iLength;
            U1      *pBytes;
        } Utf8;                 // CONSTANT_Utf8
        struct
        {
            U2      iLength;
            U2      *pBytes;
        } Unicode;              // CONSTANT_Unicode
    };
};

typedef cp_info CPOOLINFO, *LPCPOOLINFO;

//------------------------------------------------------------------------------
// ATTRINFO -- attribute information
//------------------------------------------------------------------------------
struct attribute_info
{
    struct attribute_info   *pNext;
    U2                      iName;
    U4                      iLength;
    U1                      rgBytes[1];
};

typedef attribute_info ATTRINFO, *LPATTRINFO;

//------------------------------------------------------------------------------
// MEMBERINFO -- member information
//------------------------------------------------------------------------------
struct member_info
{
    U2          iAccessFlags;
    U2          iName;
    U2          iSignature;
    LPATTRINFO  pAttrList;
};

typedef member_info MEMBERINFO, *LPMEMBERINFO;


//------------------------------------------------------------------------------
// METHODINFO -- method information
//------------------------------------------------------------------------------
struct method_info : public member_info
{
};

typedef method_info METHODINFO, *LPMETHODINFO;

//------------------------------------------------------------------------------
// FIELDINFO -- field information
//------------------------------------------------------------------------------
struct field_info : public member_info
{
};

typedef field_info FIELDINFO, *LPFIELDINFO;


#endif // #ifndef _CLSTYPES_INCLUDED
