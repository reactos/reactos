#pragma once

typedef struct INTENG_ENTER_LEAVE_TAG
  {
  /* Contents is private to EngEnter/EngLeave */
  SURFOBJ *DestObj;
  SURFOBJ *OutputObj;
  HBITMAP OutputBitmap;
  CLIPOBJ *TrivialClipObj;
  RECTL DestRect;
  BOOL ReadOnly;
  } INTENG_ENTER_LEAVE, *PINTENG_ENTER_LEAVE;

extern BOOL APIENTRY IntEngEnter(PINTENG_ENTER_LEAVE EnterLeave,
                                SURFOBJ *DestObj,
                                RECTL *DestRect,
                                BOOL ReadOnly,
                                POINTL *Translate,
                                SURFOBJ **OutputObj);

extern BOOL APIENTRY IntEngLeave(PINTENG_ENTER_LEAVE EnterLeave);

extern HGDIOBJ StockObjects[];
extern SHORT gusLanguageID;

SHORT FASTCALL IntGdiGetLanguageID(VOID);
DWORD APIENTRY IntGetQueueStatus(BOOL ClearChanges);
VOID FASTCALL IntUserManualGuiCheck(LONG Check);
PVOID APIENTRY HackSecureVirtualMemory(IN PVOID,IN SIZE_T,IN ULONG,OUT PVOID *);
VOID APIENTRY HackUnsecureVirtualMemory(IN PVOID);

BOOL
NTAPI
RegReadUserSetting(
    IN PCWSTR pwszKeyName,
    IN PCWSTR pwszValueName,
    IN ULONG ulType,
    OUT PVOID pvData,
    IN ULONG cbDataSize);

BOOL
NTAPI
RegWriteUserSetting(
    IN PCWSTR pwszKeyName,
    IN PCWSTR pwszValueName,
    IN ULONG ulType,
    OUT PVOID pvData,
    IN ULONG cbDataSize);
