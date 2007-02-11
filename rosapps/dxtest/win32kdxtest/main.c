
#include <stdio.h>
#include <windows.h>


#include "test.h"
#include "Windows2000Sp4.h"
#include "sysreactos.h"

int main(int argc, char **argv)
{

    test_NtGdiDdCreateDirectDrawObject();
    return 0;
}


void
test_NtGdiDdCreateDirectDrawObject()
{
    HANDLE retValue=0;
    int fails=0;
    HDC hdc=CreateDCW(L"Display",NULL,NULL,NULL);

    printf("Start testing of NtGdiDdCreateDirectDrawObject(NULL)\n");
    
    retValue = sysNtGdiDdCreateDirectDrawObject(NULL);

    if (retValue != NULL) 
    {
        printf("FAIL NtGdiDdCreateDirectDrawObject(NULL) != 0\n");
        fails++;
    }

    retValue = sysNtGdiDdCreateDirectDrawObject(hdc);
    if (retValue == NULL) 
    {
        printf("FAIL NtGdiDdCreateDirectDrawObject(NULL) == 0)\n");
        fails++;
    }

    if (fails == 0)
    {
        printf("End testing of NtGdiDdCreateDirectDrawObject Status : ok\n"); 
    }
    else
    {
       printf("End testing of NtGdiDdCreateDirectDrawObject Status : fail\n"); 
    }
}

