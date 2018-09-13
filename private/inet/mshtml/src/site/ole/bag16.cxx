//bag16.cxx
#include "headers.hxx"

#ifndef X_DOCGLBS_HXX_
#define X_DOCGLBS_HXX_
#include "docglbs.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_EOBJECT_HXX_
#define X_EOBJECT_HXX_
#include "eobject.hxx"
#endif

#ifndef X_MIME64_HXX_
#define X_MIME64_HXX_
#include "mime64.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifndef X_PROPBAG_HXX_
#define X_PROPBAG_HXX_
#include "propbag.hxx"
#endif

#ifndef X_PERHIST_HXX_
#define X_PERHIST_HXX_
#include "perhist.hxx"
#endif

#ifndef X_BINDER_HXX_
#define X_BINDER_HXX_
#include <binder.hxx>       // for cdatasourceprovider
#endif

#ifndef X_ADO_ADOID_H_
#define X_ADO_ADOID_H_
#include <ado/adoid.h>
#endif

#ifndef X_UWININET_H_
#define X_UWININET_H_
#include "uwininet.h"
#endif

#include "object.hdl"

#pragma warning (disable: 4702)

#define      MAX_PROXY     1024  


static const char aJavaKey[] = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\JavaVM";
static const char aProxyKey[] = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings";


static const char* aProxyPropName[] = {"proxyhttp" , 
                                       "proxyssl", 
                                       "proxygopher",
                                       "proxyftp",
                                        NULL };

static const char* aProtocol[] = {"http", "https" , "gopher" , "ftp" , NULL};
static const char* aProtocolEq[] = {"http=", "https=" , "gopher=" , "ftp=" , "socks=", NULL};


char aClassPath[MAX_PATH];
char aDefaultOptions[MAX_PATH];
char aBaseUrl[MAX_PATH];
char aProxyString[MAX_PROXY];
char aProxyBuffer[MAX_PATH];


/*-----------------------------------------------------
** IsProxyEnabled
** 
** determine whether ProxyEnabled key in registry exists and if its value is nonzero
**-----------------------------------------------------
*/
BOOL IsProxyEnabled()
{
    DWORD dwData = 0;
    DWORD dwResult = 0;
    HKEY hKey = 0;
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, &aProxyKey[0], 0 , NULL , &hKey))
    {   
        //BUGBUG? - this appears in the registry as a binary value 
        dwData = 4L;
        RegQueryValueEx(hKey,"ProxyEnable" ,NULL, NULL , (LPBYTE)&dwResult, &dwData); 
        RegCloseKey(hKey);
    } 
    
    return (dwResult != 0);
} 


/*------------------------------------------------------
** UseSameProxies
** 
** determine whether proxy settings string begins with a protocol name.  If not
** by convention the proxy string is assumed to be the proxy setting used for all protocols
**------------------------------------------------------
*/

BOOL UseSameProxies(LPSTR lpProxyString)
{   
    //if string begins with "socks" from the point of view of JavaVM, no proxies are enabled  
    if (strlen(lpProxyString) >= 5 && strnicmp(lpProxyString , "socks" , 5) == 0)
    {
        return FALSE;
    }
    
    char* lpProtocol;  
    int i;
    for (i = 0 , lpProtocol = (char*)aProtocolEq[0]; lpProtocol; i++ , lpProtocol = (char*)aProtocol[i])

    {
        if (strlen(lpProxyString) >= strlen(lpProtocol) &&  
            strnicmp(lpProxyString , lpProtocol, strlen(lpProtocol)) == 0) 
            return FALSE;
    }
    
    return TRUE;
}
            


   

/*
**----------------------------------------------
** GetProxyString   
**
** static helper to parse value of given proxy property from string in registry
**----------------------------------------------
*/

BOOL GetProxyString(LPSTR lpKey , LPSTR lpString, LPSTR lpRet, int iMaxChars)
{   
    if (!IsProxyEnabled()) 
    { 
        return FALSE;
    }
    
    if (!lpString || !lpString[0])
    {
        return FALSE;
    }
    
    if (UseSameProxies(lpString))
    {   
        //enough room in output buffer?
        if (iMaxChars < strlen(lpString) + strlen(lpKey) + 5)
        {
            return FALSE;
        }
        
        //change setting string of form "itgproxy:80" to one of form "http://itgproxy:80/"
        if (strstr(lpString, "://") == 0)
        {
            strcpy(lpRet,lpKey);
            strcat(lpRet,"://");
            strcat(lpRet,lpString);
        }
        else
        {
            strcpy(lpRet, lpString);
        }
        if (*(lpRet + strlen(lpRet) - 1) != '/')
        {
            strcat(lpRet,"/");
        }
        
        return TRUE;
    }    

    //copy data since strtok is destructive
    char* lpTemp = new char[strlen(lpString) + 1];
    strcpy(lpTemp , lpString);
    LPSTR lpNext = strtok(lpTemp , "=;" );
    while (lpNext)
    {
        if (strcmp(lpNext, lpKey) == 0)
        {
            LPSTR lpSetting = strtok(NULL , "=;");
            if (!lpSetting) 
            {    
                delete [] lpTemp;
                return FALSE;
            }

            if (iMaxChars < strlen(lpSetting) + strlen(lpKey) + 5) 
            {
                delete lpTemp;
                return FALSE;
            }
            
            //change setting string of form "itgproxy:80" to one of form "http://itgproxy:80/"
            if (strstr(lpSetting, "://") == 0)
            {
                strcpy(lpRet,lpKey);
                strcat(lpRet,"://");
                strcat(lpRet,lpSetting);
            }
            else
            {
                strcpy(lpRet, lpSetting);
            }
            if (*(lpRet + strlen(lpRet) - 1) != '/')
            {
                strcat(lpRet,"/");
            }

            delete [] lpTemp;
            return TRUE;
        }

        lpNext = strtok( NULL  , "=;");
    }

    //not found
    delete [] lpTemp;
    return FALSE;
    
}


HRESULT CObjectElement::SaveWin16AppletProps(IPropertyBag* pBag)
{
    HKEY hKey;
    HRESULT hr = S_OK;

    DWORD dwData;
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, &aJavaKey[0], 0 , NULL , &hKey))
    {

        dwData = sizeof(aClassPath);
        if (ERROR_SUCCESS != RegQueryValueEx(hKey, 
                                             "ClassPath" , 
                                             NULL, 
                                             NULL , 
                                             (LPBYTE)&aClassPath[0], &dwData))
        {
            goto FAILED;
        }

        dwData = sizeof(aDefaultOptions);
        if (ERROR_SUCCESS != RegQueryValueEx(hKey, 
                        "DefaultOptions",
                        NULL,
                        NULL, 
                        (LPBYTE)&aDefaultOptions[0], 
                        &dwData))

    
        {
            strcpy(aDefaultOptions , "-mx600k -ss8k msjava16");
        }
        
        RegCloseKey(hKey);
        hKey = 0;
    }
    else
    {
        goto FAILED;
    }

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, &aProxyKey[0], 0 , NULL , &hKey))
    {
        dwData = sizeof(aProxyString);
        aProxyString[0] = '\0';
        RegQueryValueEx(hKey, "ProxyServer", NULL, NULL , (LPBYTE)&aProxyString[0], &dwData);
        RegCloseKey(hKey);
    }

    //drop data in propertybag
    VARIANT var;
    VariantInit(&var);
    if (pBag->Read("documentbase", &var, NULL) == E_INVALIDARG)
    {
        char* pBaseURL;  
        char pDocBaseURL[pdlUrlLen];
        char *pDocBaseSlash;
        DWORD dwDocBase;
        
        hr = _pDoc->GetBaseUrl(&pBaseURL, this); 
        if (hr)
            goto FAILED;
        
        dwDocBase = pdlUrlLen;
        if (!InternetCombineUrl(pBaseURL, "java", pDocBaseURL, &dwDocBase, 0))
            goto FAILED;
        char chSlash = strnicmp(pDocBaseURL, "file://", 7) ? '/' : '\\';
        if (!(pDocBaseSlash = strrchr(pDocBaseURL, chSlash)))
                goto FAILED;   

        if (_stricmp(pDocBaseSlash + 1, "java"))
            goto FAILED;
        *(pDocBaseSlash + 1) = '\0';  

        var.vt = VT_BSTR;
        var.bstrVal = SysAllocString(pDocBaseURL);  
        hr = pBag->Write("documentbase", &var);  
        VariantClear(&var);
        if (hr) goto FAILED;
    }
    
    var.vt = VT_BSTR;
    var.bstrVal = SysAllocString(&aClassPath[0]);
    hr = pBag->Write("classpath", &var);
    VariantClear(&var);
    if (hr) goto FAILED;
   
    var.vt = VT_BSTR;
    var.bstrVal = SysAllocString(&aDefaultOptions[0]);
    hr = pBag->Write("defaultoptions", &var); 
    VariantClear(&var);
    if (hr) goto FAILED;

    if (aProxyString[0])
    {
        for (int i = 0; aProtocol[i]; i++)
        {
            if (GetProxyString((LPSTR)aProtocol[i] , 
                               aProxyString, 
                               aProxyBuffer, 
                               sizeof(aProxyBuffer)))
            {
                var.vt = VT_BSTR; 
            
                var.bstrVal = SysAllocString(&aProxyBuffer[0]);
                hr = pBag->Write(aProxyPropName[i], &var);
                VariantClear(&var);
                if (hr) goto FAILED;
            
            }
        }

    }

    return S_OK;

FAILED:
    return S_FALSE;
}





