
#include <stdio.h>

#ifdef __GNUC__
#endif

__declspec(dllimport) int __cdecl CdeclFunc0(void);
__declspec(dllimport) int __cdecl CdeclFunc1(char* a1);
__declspec(dllimport) int __cdecl CdeclFunc2(char* a1);
__declspec(dllimport) int __cdecl CdeclFunc3(char* a1);

__declspec(dllimport) int __stdcall StdcallFunc0(void);
__declspec(dllimport) int __stdcall StdcallFunc1(char* a1);
__declspec(dllimport) int __stdcall StdcallFunc2(char* a1);
__declspec(dllimport) int __stdcall StdcallFunc3(char* a1);
__declspec(dllimport) int __stdcall StdcallFunc4(char* a1);
__declspec(dllimport) int __stdcall StdcallFunc5(char* a1);

__declspec(dllimport) int __stdcall DecoratedStdcallFunc1(char* a1);
__declspec(dllimport) int __stdcall DecoratedStdcallFunc2(char* a1);
__declspec(dllimport) int __stdcall DecoratedStdcallFunc3(char* a1);
__declspec(dllimport) int __stdcall DecoratedStdcallFunc4(char* a1);
__declspec(dllimport) int __stdcall DecoratedStdcallFunc5(char* a1);

__declspec(dllimport) int __fastcall FastcallFunc0(void);
__declspec(dllimport) int __fastcall FastcallFunc1(char* a1);
__declspec(dllimport) int __fastcall FastcallFunc2(char* a1);
__declspec(dllimport) int __fastcall FastcallFunc3(char* a1);
__declspec(dllimport) int __fastcall FastcallFunc4(char* a1);
__declspec(dllimport) int __fastcall FastcallFunc5(char* a1);

__declspec(dllimport) int __fastcall DecoratedFastcallFunc1(char* a1);
__declspec(dllimport) int __fastcall DecoratedFastcallFunc2(char* a1);
__declspec(dllimport) int __fastcall DecoratedFastcallFunc3(char* a1);
__declspec(dllimport) int __fastcall DecoratedFastcallFunc4(char* a1);
__declspec(dllimport) int __fastcall DecoratedFastcallFunc5(char* a1);

__declspec(dllimport) extern int DataItem1;
__declspec(dllimport) extern int DataItem2;
__declspec(dllimport) extern int DataItem3;

#define ok_int(a, b) \
    if ((a) != (b)) { printf("wrong result in line %d, expected 0x%x, got 0x%x\n", __LINE__, (b), (a)); }

int main(int argc, char *argv[])
{
    char* str = "defaultstring";

    if (argc > 2)
        str = argv[1];

    ok_int(CdeclFunc0(), 0);
    ok_int(CdeclFunc1(str), 1);
    ok_int(CdeclFunc2(str), 1);
    ok_int(CdeclFunc3(str), 0x10001);

    ok_int(StdcallFunc0(), 0x10);
    ok_int(StdcallFunc1(str), 0x11);
    ok_int(StdcallFunc2(str), 0x11);
    ok_int(StdcallFunc3(str), 0x10011);
#ifdef _M_IX86
    ok_int(StdcallFunc4(str), 0x21);
#endif
    ok_int(StdcallFunc5(str), 0x10021);

#ifdef _M_IX86
    ok_int(DecoratedStdcallFunc1(str), 0x21);
    ok_int(DecoratedStdcallFunc2(str), 0x11);
    //ok_int(DecoratedStdcallFunc3(str), 11);
    ok_int(DecoratedStdcallFunc4(str), 0x21);
    ok_int(DecoratedStdcallFunc5(str), 0x10021);
#endif
    ok_int(FastcallFunc0(), 0x30);
    ok_int(FastcallFunc1(str), 0x31);
    ok_int(FastcallFunc2(str), 0x31);
    ok_int(FastcallFunc3(str), 0x10031);
#ifdef _M_IX86
    ok_int(FastcallFunc4(str), 0x42);
    ok_int(FastcallFunc5(str), 0x10041);
#endif
#ifdef _M_IX86
    ok_int(DecoratedFastcallFunc1(str), 0x42);
    ok_int(DecoratedFastcallFunc2(str), 0x31);
    //ok_int(DecoratedFastcallFunc3(str), 11);
    ok_int(DecoratedFastcallFunc4(str), 0x42);
    ok_int(DecoratedFastcallFunc5(str), 0x10041);
#endif
    ok_int(DataItem1, 0x51);
    ok_int(DataItem2, 0x51);
    ok_int(DataItem3, 0x10051);

    printf("done.\n");

    return 0;
}
