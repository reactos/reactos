// installwv.h : Installs files from a resource
 
#ifndef __INSTALL_H_
#define __INSTALL_H_

typedef struct tagINSTALL_INFO {
    LPTSTR szSource;
    LPTSTR szDest;
    DWORD dwDestAttrib;
} INSTALL_INFO;

HRESULT InstallFileFromResource(HINSTANCE      hInstResource, 
                                INSTALL_INFO   *piiFile,
                                LPTSTR         pszDestDir);

HRESULT InstallWebViewFiles(HINSTANCE hInstResource);

#endif // __INSTALL_H_
