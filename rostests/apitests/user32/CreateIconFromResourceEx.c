
#include <apitest.h>

#include <winuser.h>

START_TEST(CreateIconFromResourceEx)
{
    HCURSOR hcur1, hcur2;
    HMODULE hMod;
    HRSRC hResource;    // handle to FindResource  
    HRSRC hMem;         // handle to LoadResource 
    BYTE *lpResource;   // pointer to resource data  
    
    hMod = GetModuleHandle(NULL);
    ok(hMod != NULL, "\n");
    /* Create a shared cursor */
    hcur1 = LoadCursor(hMod, "TESTCURSOR");
    ok(hcur1 != NULL, "\n");
    
    /* Create it manually using CreateIconFromResourceEx */
    hResource = FindResourceA(hMod, 
                            "TESTCURSOR", 
                            RT_GROUP_CURSOR);
    ok(hResource != NULL, "\n");
    
    hMem = LoadResource(hMod, hResource); 
    ok(hMem != NULL, "\n");
 
    lpResource = LockResource(hMem); 
    ok(lpResource != NULL, "\n");
    
    /* MSDN states that LR_SHARED permits to not load twice the same cursor again.
     * But CreateIconFromResourceEx still returns two different handles */
    hcur2 = CreateIconFromResourceEx(lpResource, SizeofResource(hMod, hResource), FALSE, 0x00030000, 0, 0, LR_SHARED);
    ok(hcur2 != NULL, "\n");
    ok(hcur2 != hcur1, "\n");
    hcur1 = CreateIconFromResourceEx(lpResource, SizeofResource(hMod, hResource), FALSE, 0x00030000, 0, 0, LR_SHARED);
    ok(hcur1 != NULL, "\n");
    ok(hcur2 != hcur1, "\n");
    
    /* Try to destroy them multiple times (see DestroyCursor test) */
    ok(DestroyCursor(hcur1), "\n");
    ok(DestroyCursor(hcur1), "\n");
    ok(DestroyCursor(hcur2), "\n");
    ok(DestroyCursor(hcur2), "\n");
    
    FreeResource(hResource);
}