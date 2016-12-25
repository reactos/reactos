
int __cdecl CdeclFunc0(void)
{
    return 0x10000;
}

int __cdecl CdeclFunc1(char *p1)
{
    return 0x10001;
}

int __stdcall StdcallFunc0(void)
{
    return 0x10010;
}

int __stdcall StdcallFunc1(char *p1)
{
    return 0x10011;
}

int __stdcall DecoratedStdcallFunc1(char *p1)
{
    return 0x10021;
}

int __fastcall FastcallFunc0(void)
{
    return 0x10030;
}

int __fastcall FastcallFunc1(char *p1)
{
    return 0x10031;
}

int __fastcall DecoratedFastcallFunc1(char *p1)
{
    return 0x10041;
}

int DataItem1 = 0x10051;
int DataItem2 = 0x10052;


