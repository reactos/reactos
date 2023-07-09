#ifdef __cplusplus
extern "C" {
#endif

TYPENAME (str001, T_ABS,     9, "absolute ")
TYPENAME (str002, T_NOTYPE, 10, "<no type> ")
TYPENAME (str003, T_SEGMENT, 9, "_segment ")
TYPENAME (str004, T_CHAR,    5, "char ")
TYPENAME (str005, T_SHORT,   6, "short ")
TYPENAME (str006, T_LONG,    5, "long ")
TYPENAME (str007, T_UCHAR,  14, "unsigned char ")
TYPENAME (str008, T_USHORT, 15, "unsigned short ")
TYPENAME (str009, T_ULONG,  14, "unsigned long ")
TYPENAME (str010, T_REAL32,  6, "float ")
TYPENAME (str011, T_REAL64,  7, "double ")
TYPENAME (str012, T_REAL80, 12, "long double ")
TYPENAME (str013, T_VOID,    5, "void ")
TYPENAME (str014, T_INT2,    4, "int ")
TYPENAME (str015, T_UINT2,  13, "unsigned int ")
TYPENAME (str016, T_INT4,    4, "int ")
TYPENAME (str017, T_UINT4,  13, "unsigned int ")
TYPENAME (str018, T_RCHAR,   5, "char ")
TYPENAME (str019, T_UQUAD,  17, "unsigned __int64 ")
TYPENAME (str020, T_UINT8,  17, "unsigned __int64 ")
TYPENAME (str021, T_QUAD,    8, "__int64 ")
TYPENAME (str022, T_INT8,    8, "__int64 ")

MODENAME (str200, CV_TM_DIRECT, 0, "")
MODENAME (str201, CV_TM_NPTR,   2, "* ")
MODENAME (str202, CV_TM_FPTR,   6, "far * ")
MODENAME (str203, CV_TM_HPTR,   7, "huge * ")
MODENAME (str204, CV_TM_NPTR32, 2, "* ")
MODENAME (str205, CV_TM_FPTR32, 6, "far * ")
MODENAME (str206, CV_TM_NPTR64, 2, "* ")

PTRNAME (str300, "\x007""near * ")
PTRNAME (str301, "\x006""far * ")
PTRNAME (str302, "\x007""huge * ")
PTRNAME (str303, "\x00c""based seg * ")
PTRNAME (str304, "\x00c""based val * ")
PTRNAME (str305, "\x00f""based segval * ")
PTRNAME (str306, "\x00d""based addr * ")
PTRNAME (str307, "\x010""based segaddr * ")
PTRNAME (str308, "\x00d""based type * ")
PTRNAME (str309, "\x00d""based self * ")
PTRNAME (str310, "\x002""* ")
PTRNAME (str311, "\x006""far * ")
PTRNAME (str312, "\x002""& ")

#ifdef __cplusplus
} // extern "C" {
#endif
