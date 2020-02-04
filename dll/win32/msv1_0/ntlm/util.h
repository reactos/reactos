#ifndef _UTIL_H_
#define _UTIL_H_

VOID
NtlmInitExtStrWFromUnicodeString(
    OUT PEXT_STRING_W Dest,
    IN PUNICODE_STRING Src);

VOID
NtlmInitUnicodeStringFromExtStrW(
    OUT PUNICODE_STRING Dest,
    IN PEXT_STRING_W Src);

#endif
