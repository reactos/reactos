
int __cdecl CdeclFunc0(void)
{
    return 0x0;
}

int __cdecl CdeclFunc1(char *p1)
{
    return 0x1;
}

int __stdcall StdcallFunc0(void)
{
    return 0x10;
}

int __stdcall StdcallFunc1(char *p1)
{
    return 0x11;
}

int __stdcall DecoratedStdcallFunc1(char *p1)
{
    return 0x21;
}

int __fastcall FastcallFunc0(void)
{
    return 0x30;
}

int __fastcall FastcallFunc1(char *p1)
{
    return 0x31;
}

int __fastcall DecoratedFastcallFunc1(char *p1)
{
    return 0x42;
}

int __stdcall ExportByOrdinal1(char *p1)
{
    return 0x11;
}

int DataItem1 = 0x51;
int DataItem2 = 0x52;

