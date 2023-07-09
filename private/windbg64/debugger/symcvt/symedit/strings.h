#if ! defined( _RES_STR_ )
#define _RES_STR_

#ifdef RESOURCES
#define RES_STR(a, b, c) b, c
STRINGTABLE
LANGUAGE LANG_ENGLISH, SUBLANG_ENGLISH_US
BEGIN
#else

enum _RESOURCEIDS {
#define RES_STR(a, b, c) a = b,
#endif


RES_STR(ERR_OPEN_INPUT_FILE,    1,      "The file '%s' could not be openned for reading")
RES_STR(ERR_INVALID_PE,         2,      "'%s' is not a valid PE exe file with debug info")
RES_STR(ERR_NO_DEST,            3,      "Need to specify a destination for the converted debug information")
RES_STR(ERR_OPEN_WRITE_FILE,    4,      "Cannot open the file '%s' for writing")
RES_STR(ERR_EDIT_DBG_FILE,      5,      "Cannot edit name file in DBG file")
RES_STR(ERR_MAP_FILE,           6,      "Cannot map the file '%s'")
RES_STR(ERR_NO_COFF,            7,      "No COFF debug information present to be converted")
RES_STR(ERR_NOT_MAPPED,         8,      "Cannot add CV info unless debug information is mapped")
RES_STR(ERR_COFF_TO_CV,         9,      "Cannot convert COFF debug information to CodeView debug information")
RES_STR(ERR_OP_UNKNOWN,         10,     "Operation '%s' is unknown")
RES_STR(ERR_NO_MEMORY,          11,     "Out of memory")
RES_STR(ERR_FILE_PTRS,          12,     "INTERNAL: cannot set file pointers")
RES_STR(ERR_SET_EOF,            13,     "INTERNAL: cannot set the end of file markder")
RES_STR(ERR_CHECKSUM_CALC,      14,     "INTERNAL: cannot compute the image checksum")

#ifdef RESOURCES
END
#else
};
#endif

#endif // _RES_STR_
