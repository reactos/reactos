

#if !defined(__REACTOS__)

    HANDLE sysNtGdiDdCreateDirectDrawObject(HDC hdc)
    {
       INT retValue;
       _asm
       {
           mov     eax, 0x1039
           lea     edx, [hdc]
           int     0x2E
           mov [retValue],eax
       }
       return retValue;
    }

#endif


