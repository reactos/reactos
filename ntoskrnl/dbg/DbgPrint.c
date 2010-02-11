#include <dbg.h>

ULONG (_CDECL *DbgPrintf)(PCSTR fmt, ...) = DbgPrintfSer1;
ULONG (_CDECL *DbgPrintfv)(PCSTR fmt, va_list ap) = DbgPrintfvSer1;
ULONG (NTAPI *DbgPrintExwp)
	(IN PCCH Prefix, IN ULONG ComponentId, IN ULONG Level, IN PCCH Format, IN va_list ap, IN BOOLEAN HandleBreakpoint)
	= DbgPrintExwpSer1;

NTSYSAPI ULONG NTAPI vDbgPrintExWithPrefix(PCCH Prefix, ULONG ComponentId, ULONG Level, PCCH Format, va_list arglist)
{
	return DbgPrintExwp(Prefix, ComponentId, Level, Format, arglist, TRUE);
}

NTSYSAPI ULONG NTAPI vDbgPrintEx(ULONG ComponentId, ULONG Level, PCCH Format, va_list arglist)
{
	return DbgPrintExwp("", ComponentId, Level, Format, arglist, TRUE);
}

ULONG _CDECL DbgPrint(PCSTR Format, ...)
{
	ULONG Status;
    va_list ap;

    va_start(ap, Format);
	Status = DbgPrintfv(Format, ap);
    va_end(ap);
	return Status;
}

NTSYSAPI ULONG _CDECL DbgPrintEx(ULONG ComponentId, ULONG Level, PCSTR Format, ...)
{
    ULONG Status;
    va_list ap;

    va_start(ap, Format);
    Status = DbgPrintExwp("", ComponentId, Level, Format, ap, TRUE);
    va_end(ap);
    return Status;
}

NTSYSAPI ULONG _CDECL DbgPrintReturnControlC(PCCH Format, ...)
{
    ULONG Status;
    va_list ap;

    va_start(ap, Format);
    Status = DbgPrintExwp("", -1, DPFLTR_ERROR_LEVEL, Format, ap, FALSE);
    va_end(ap);
    return Status;
}
