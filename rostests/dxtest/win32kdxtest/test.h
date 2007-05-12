

HANDLE test_NtGdiDdCreateDirectDrawObject();
void test_NtGdiDdDeleteDirectDrawObject(HANDLE hDirectDrawLocal);
void test_NtGdiDdQueryDirectDrawObject( HANDLE hDirectDrawLocal);
void test_NtGdiDdGetScanLine(HANDLE hDirectDrawLocal);
void test_NtGdiDdWaitForVerticalBlank(HANDLE hDirectDrawLocal);
void test_NtGdiDdCanCreateSurface(HANDLE hDirectDrawLocal);


void dump_halinfo(DD_HALINFO *pHalInfo, char *text);
void dump_CallBackFlags(DWORD *pCallBackFlags, char *text);



#define testing_eq(input,value,counter,text) \
        if (input == value) \
        { \
            counter++; \
            printf("FAIL ret=%s, %d != %d )\n",text,(int)input,(int)value); \
        }


#define testing_noteq(input,value,counter,text) \
        if (input != value) \
        { \
            counter++; \
            printf("FAIL ret=%s, %d == %d )\n",text,(int)input,(int)value); \
        }


#define show_status(counter, text) \
        if (counter == 0) \
        { \
            printf("End testing of %s Status : ok\n\n",text); \
        } \
        else \
        { \
            printf("End testing of %s Status : fail\n\n",text); \
        }


#if !defined(__REACTOS__)

#define win_syscall(inValue,outValue,syscallid) \
                    __asm { mov     eax, syscallid }; \
                    __asm { lea     edx, [inValue] }; \
                    __asm { int     0x2E }; \
                    __asm { mov outValue,eax};



#endif
