#ifndef PRECOMP_H__
#define PRECOMP_H__

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <windows.h>
#include <setupapi.h>
#include <olectl.h>
#include <unknwn.h>
#include <dsound.h>
#include <debug.h>


/* factory method */
typedef HRESULT (CALLBACK *LPFNCREATEINSTANCE)(IUnknown* pUnkOuter, REFIID riid, LPVOID* ppvObject);

/* factory table */
typedef struct
{
    REFIID riid;
    LPFNCREATEINSTANCE lpfnCI;
} INTERFACE_TABLE;

/* globals */
extern HINSTANCE dsound_hInstance;


#endif
