#define RTL_FOREACH_LIST(PLIST_HEAD__, TYPE__, FIELD__) \
{ \
 PLIST_ENTRY _RTL_FOREACH_LIST_END = (PLIST_HEAD__); \
 PLIST_ENTRY _RTL_FOREACH_LIST_CUR = _RTL_FOREACH_LIST_END->Flink; \
 TYPE__ * _RTL_ITEM = \
  CONTAINING_RECORD(_RTL_FOREACH_LIST_CUR, TYPE__, FIELD__); \
 \
 for \
 ( \
  ; \
  _RTL_FOREACH_LIST_CUR != _RTL_FOREACH_LIST_END; \
  _RTL_ITEM = CONTAINING_RECORD(_RTL_FOREACH_LIST_CUR, TYPE__, FIELD__) \
 )

#define RTL_FOREACH_END }

NTSTATUS STDCALL LdrpRosTlsAction
(
 IN ULONG Reason
)
{
 typedef struct _LDRP_TLS_ENTRY
 {
  LIST_ENTRY TlsList;
  PIMAGE_TLS_DIRECTORY TlsDir;
  PLDR_MODULE LdrModule;
 }
 LDRP_TLS_ENTRY, * PLDRP_TLS_ENTRY;

 static LIST_ENTRY s_leTlsList;
 static SIZE_T s_nTlsImagesCount;

 switch(Reason)
 {
  case DLL_PROCESS_ATTACH:
  {
   PPEB_LDR_DATA pLdr = NtCurrentPeb()->Ldr;

   InitializeListHead(&s_leTlsList);
   s_nTlsImagesCount = 0;

   RTL_FOREACH_LIST
   (
    &pLdr->InLoadOrderModuleList,
    LDR_MODULE,
    InLoadOrderModuleList
   )
   {
    SIZE_T nSize;
    PIMAGE_TLS_DIRECTORY pitdTlsDir = RtlImageDirectoryEntryToData
    (
     _RTL_ITEM->BaseAddress,
     TRUE,
     IMAGE_DIRECTORY_ENTRY_TLS,
     &nSize
    );

    if(pitdTlsDir == NULL || nSize < sizeof(IMAGE_TLS_DIRECTORY)) continue;

    PLDRP_TLS_ENTRY plteCur =
     RtlAllocateHeap(pPeb->ProcessHeap, 0, sizeof(LDRP_TLS_ENTRY));

    if(plteCur == NULL) return STATUS_NO_MEMORY;

    InsertTailList(&s_leTlsList, &plteCur->TlsList);

    _RTL_ITEM->TlsIndex = s_nTlsImagesCount;
    ++ s_nTlsImagesCount; /* TODO: check for out-of-bounds index */

    plteCur->LdrModule = _RTL_ITEM;
    plteCur->TlsDir = pitdTlsDir;

    *pitdTlsDir->AddressOfIndex = _RTL_ITEM->TlsIndex;
   }
   RTL_FOREACH_END;

   RtlSetBits(pPeb->TlsBitmap, 0, s_nTlsImagesCount);

   /* fall through */
  }

  case DLL_THREAD_ATTACH:
  {
   RTL_FOREACH_LIST(&s_leTlsList, LDRP_TLS_ENTRY, TlsList)
   {
    PBYTE pcTlsData;
    PIMAGE_TLS_DIRECTORY pitdTlsDir = _RTL_ITEM->TlsDir;
    SIZE_T nInitDataSize =
     pitdTlsDir->EndAddressOfRawData - pitdTlsDir->StartAddressOfRawData;

    PIMAGE_TLS_CALLBACK * pitcCallbacks =
     (PIMAGE_TLS_CALLBACK *)_RTL_ITEM->TlsDir->AddressOfCallbacks;

    pcTlsData = RtlAllocateHeap
    (
     pPeb->ProcessHeap,
     0,
     nInitDataSize + pitdTlsDir->SizeOfZeroFill
    );

    if(pcTlsData == NULL) return STATUS_NO_MEMORY;

    pTeb->TlsSlots[_RTL_ITEM->LdrModule->TlsIndex] = pcTlsData;

    RtlMoveMemory(pcTlsData, pitdTlsDir->StartAddressOfRawData, nInitDataSize);
    RtlZeroMemory(pcTlsData + nInitDataSize, pitdTlsDir->SizeOfZeroFill);

    if(pitcCallbacks)
     for(; *pitcCallbacks; ++ pitcCallbacks)
      (*pitcCallbacks)(_RTL_ITEM->LdrModule->BaseAddress, Reason, NULL);
   }
   RTL_FOREACH_END;

   return STATUS_SUCCESS;
  }

  case DLL_THREAD_DETACH:
  {
  }

  case DLL_PROCESS_DETACH:
  {
  }

  default: return STATUS_SUCCESS;
 }
}
