#ifndef __TYPES_H
#define __TYPES_H

#define BASE_TYPE_MASK     (~0xff)

#define BOOLEAN_TYPE       (0x100)
#define BYTE_TYPE          (0x200)
#define CHAR_TYPE          (0x300)
#define DOUBLE_TYPE        (0x400)
#define ERROR_STATUS_TYPE  (0x500)
#define FLOAT_TYPE         (0x600)
#define HANDLE_TYPE        (0x700)
#define HYPER_TYPE         (0x800)
#define INT_TYPE           (0x900)
#define INT32_TYPE         (0xA00)
#define INT32OR64_TYPE     (0xB00)
#define INT64_TYPE         (0xC00)
#define LONG_TYPE          (0xD00)
#define SHORT_TYPE         (0xE00)
#define SMALL_TYPE         (0xF00)
#define VOID_TYPE          (0x1000)
#define WCHAR_TYPE         (0x1100)

#define UNSIGNED_TYPE_OPTION      (0x1)
#define SIGNED_TYPE_OPTION        (0x2)
#define POINTER_TYPE_OPTION       (0x4)

#define IN_TYPE_OPTION            (0x8)
#define OUT_TYPE_OPTION           (0x10)
#define STRING_TYPE_OPTION        (0x20)

int token_to_type(char* token);
void print_type(int tval);
void add_typedef(char* name, int type);

void start_struct(char* tag);
void add_struct_member(char* name, unsigned int type);
unsigned int end_struct(void);

unsigned int struct_to_type(char* tag);

#endif
