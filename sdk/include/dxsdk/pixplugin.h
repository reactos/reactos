

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#define PIX_PLUGIN_SYSTEM_VERSION 0x101

    enum PIXCOUNTERDATATYPE
{
    PCDT_RESERVED,
    PCDT_FLOAT,
    PCDT_INT,
    PCDT_INT64,
    PCDT_STRING,
};

typedef int PIXCOUNTERID;

struct PIXCOUNTERINFO
{
  PIXCOUNTERID counterID;
  WCHAR* pstrName;
  PIXCOUNTERDATATYPE pcdtDataType;
};

struct PIXPLUGININFO
{
  HINSTANCE hinst;
  WCHAR* pstrPluginName;
  int iPluginVersion;
  int iPluginSystemVersion;
};

BOOL WINAPI PIXBeginExperiment( PIXCOUNTERID id, const WCHAR* pstrApplication );
BOOL WINAPI PIXEndFrame( PIXCOUNTERID id, UINT iFrame, DWORD* pdwReturnBytes, BYTE** ppReturnData );
BOOL WINAPI PIXEndExperiment( PIXCOUNTERID id );
BOOL WINAPI PIXGetCounterInfo( DWORD* pdwReturnCounters, PIXCOUNTERINFO** ppCounterInfoList );
BOOL WINAPI PIXGetCounterDesc( PIXCOUNTERID id, WCHAR** ppstrCounterDesc );
BOOL WINAPI PIXGetPluginInfo( PIXPLUGININFO* pPIXPluginInfo );

#ifdef __cplusplus
};
#endif