#ifndef __TYPES_H
#define __TYPES_H

#define BOOLEAN_TYPE       (0x100)
#define BYTE_TYPE          (0x200)
#define CHAR_TYPE          (0x400)
#define DOUBLE_TYPE        (0x800)
#define ERROR_STATUS_TYPE  (0x1000)
#define FLOAT_TYPE         (0x2000)
#define HANDLE_TYPE        (0x4000)
#define HYPER_TYPE         (0x8000)
#define INT_TYPE           (0x10000)
#define INT32_TYPE         (0x20000)
#define INT32OR64_TYPE     (0x40000)
#define INT64_TYPE         (0x80000)
#define LONG_TYPE          (0x100000)
#define SHORT_TYPE         (0x200000)
#define SMALL_TYPE         (0x400000)
#define VOID_TYPE          (0x800000)
#define WCHAR_TYPE         (0x1000000)

#define UNSIGNED_TYPE_OPTION      (0x1)
#define SIGNED_TYPE_OPTION        (0x2)
#define POINTER_TYPE_OPTION       (0x4)

#define IN_TYPE_OPTION            (0x8)
#define OUT_TYPE_OPTION           (0x10)
#define STRING_TYPE_OPTION        (0x20)

int token_to_type(char* token);
void print_type(int tval);

#endif
