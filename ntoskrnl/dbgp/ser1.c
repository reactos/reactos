NTSYSAPI VOID NTAPI RtlRaiseException(PEXCEPTION_RECORD ExceptionRecord);

void _CDECL DbgPrintcSer1(char c)
{
	_asm {
		push edx
		mov dx, 0x2f8
		mov al, c
		out dx, al
		add dl, 5
l1:
		in al, dx
		test al, 0x20
		jz l1
		pop edx
	}
}

void _CDECL DbgPrintsSer1(char *str)
{
	char c;

	while(c = *str++)
	{
		DbgPrintcSer1(c);
	}
	_asm {
		push edx
		mov dx, 0x2f8+5
l1:
		in al, dx
		test al, 0x40
		jz l1
		pop edx
	}
}

ULONG _CDECL DbgPrintfSer1(PCSTR fmt, ...)
{
    va_list ap;
	int len;
	char buf[0x400];

	va_start(ap, fmt);
	len = _vsnprintf(buf, 0x3FF, fmt, ap);
	buf[0x3FF] = 0;
	DbgPrintsSer1(buf);
    va_end(ap);
	return 0;
}

ULONG _CDECL DbgPrintfvSer1(PCSTR fmt, va_list ap)
{
	int len;
	char buf[0x400];

	len = _vsnprintf(buf, 0x3FF, fmt, ap);
	buf[0x3FF] = 0;
	DbgPrintsSer1(buf);
	return 0;
}

ULONG NTAPI DbgPrintExwpSer1(IN PCCH Prefix, IN ULONG ComponentId, IN ULONG Level, IN PCCH Format, IN va_list ap, IN BOOLEAN HandleBreakpoint)
{
    STRING DebugString;
    CHAR Buffer[512];
    ULONG Length, PrefixLength;

	/* Check if we should print it or not */
    if ((ComponentId != MAXULONG) &&
        (DbgQueryDebugFilterState(ComponentId, Level)) != TRUE)
    {
        /* This message is masked */
        return STATUS_SUCCESS;
    }

	/* Guard against incorrect pointers */
    _SEH2_TRY
    {
        /* Get the length and normalize it */
        PrefixLength = strlen(Prefix);
        if (PrefixLength > sizeof(Buffer)) PrefixLength = sizeof(Buffer);

        /* Copy it */
        strncpy(Buffer, Prefix, PrefixLength);

        /* Do the printf */
        Length = _vsnprintf(Buffer + PrefixLength,
                            sizeof(Buffer) - PrefixLength,
                            Format,
                            ap);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Fail */
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    /* Check if we went past the buffer */
    if(Length == MAXULONG)
    {
        /* Terminate it if we went over-board */
        Buffer[sizeof(Buffer) - 1] = '\n';

        /* Put maximum */
        Length = sizeof(Buffer);
    }
    else
    {
        /* Add the prefix */
        Length += PrefixLength;
    }

    /* Build the string */
    DebugString.Length = (USHORT)Length;
    DebugString.Buffer = Buffer;

	/* Call the Debug Print routine */
    // Status = DebugPrint(&DebugString, ComponentId, Level);
	DbgPrintfSer1("%s", DebugString.Buffer);

	/* Return */
    return STATUS_SUCCESS;
}
