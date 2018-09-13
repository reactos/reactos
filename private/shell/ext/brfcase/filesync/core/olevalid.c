/*
 * olevalid.c - OLE validation functions module.
 */


/* Headers
 **********/

#include "project.h"
#pragma hdrstop



/****************************** Public Functions *****************************/


#if defined(DEBUG) || defined(VSTF)

/*
** IsValidPCGUID()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidPCGUID(PCGUID pcguid)
{
   /* All values are valid GUIDs. */

   return(IS_VALID_READ_PTR(pcguid, CGUID));
}


/*
** IsValidPCCLSID()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidPCCLSID(PCCLSID pcclsid)
{
   return(IS_VALID_STRUCT_PTR(pcclsid, CGUID));
}


/*
** IsValidPCIID()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidPCIID(PCIID pciid)
{
   return(IS_VALID_STRUCT_PTR(pciid, CGUID));
}


/*
** IsValidREFIID()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidREFIID(REFIID riid)
{
   return(IS_VALID_STRUCT_PTR(riid, CIID));
}


/*
** IsValidREFCLSID()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidREFCLSID(REFCLSID rclsid)
{
   return(IS_VALID_STRUCT_PTR(rclsid, CCLSID));
}


/*
** IsValidPCInterface()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidPCInterface(PCVOID pcvi)
{
   return(IS_VALID_READ_PTR((PROC *)pcvi, PROC) &&
          IS_VALID_CODE_PTR(*((PROC *)pcvi), Method));
}


/*
** IsValidPCIClassFactory()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidPCIClassFactory(PCIClassFactory pcicf)
{
   return(IS_VALID_READ_PTR(pcicf, CIClassFactory) &&
          IS_VALID_READ_PTR(pcicf->lpVtbl, sizeof(*(pcicf->lpVtbl))) &&
          IS_VALID_STRUCT_PTR((PCIUnknown)pcicf, CIUnknown) &&
          IS_VALID_CODE_PTR(pcicf->lpVtbl->CreateInstance, CreateInstance) &&
          IS_VALID_CODE_PTR(pcicf->lpVtbl->LockServer, LockServer));
}


/*
** IsValidPCIDataObject()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidPCIDataObject(PCIDataObject pcido)
{
   return(IS_VALID_READ_PTR(pcido, CIDataObject) &&
          IS_VALID_READ_PTR(pcido->lpVtbl, sizeof(*(pcido->lpVtbl))) &&
          IS_VALID_STRUCT_PTR((PCIUnknown)pcido, CIUnknown) &&
          IS_VALID_CODE_PTR(pcido->lpVtbl->GetData, GetData) &&
          IS_VALID_CODE_PTR(pcido->lpVtbl->GetDataHere, GetDataHere) &&
          IS_VALID_CODE_PTR(pcido->lpVtbl->QueryGetData, QueryGetData) &&
          IS_VALID_CODE_PTR(pcido->lpVtbl->GetCanonicalFormatEtc, GetCanonicalFormatEtc) &&
          IS_VALID_CODE_PTR(pcido->lpVtbl->SetData, SetData) &&
          IS_VALID_CODE_PTR(pcido->lpVtbl->EnumFormatEtc, EnumFormatEtc) &&
          IS_VALID_CODE_PTR(pcido->lpVtbl->DAdvise, DAdvise) &&
          IS_VALID_CODE_PTR(pcido->lpVtbl->DUnadvise, DUnadvise) &&
          IS_VALID_CODE_PTR(pcido->lpVtbl->EnumDAdvise, EnumDAdvise));
}


/*
** IsValidPCIMalloc()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidPCIMalloc(PCIMalloc pcimalloc)
{
   return(IS_VALID_READ_PTR(pcimalloc, CIMalloc) &&
          IS_VALID_READ_PTR(pcimalloc->lpVtbl, sizeof(*(pcimalloc->lpVtbl))) &&
          IS_VALID_STRUCT_PTR((PCIUnknown)pcimalloc, CIUnknown) &&
          IS_VALID_CODE_PTR(pcimalloc->lpVtbl->Alloc, Alloc) &&
          IS_VALID_CODE_PTR(pcimalloc->lpVtbl->Realloc, Realloc) &&
          IS_VALID_CODE_PTR(pcimalloc->lpVtbl->Free, Free) &&
          IS_VALID_CODE_PTR(pcimalloc->lpVtbl->GetSize, GetSize) &&
          IS_VALID_CODE_PTR(pcimalloc->lpVtbl->DidAlloc, DidAlloc) &&
          IS_VALID_CODE_PTR(pcimalloc->lpVtbl->HeapMinimize, HeapMinimize));
}


/*
** IsValidPCIMoniker()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidPCIMoniker(PCIMoniker pcimk)
{
   return(IS_VALID_READ_PTR(pcimk, CIMoniker) &&
          IS_VALID_READ_PTR(pcimk->lpVtbl, sizeof(*(pcimk->lpVtbl))) &&
          IS_VALID_STRUCT_PTR((PCIPersistStream)pcimk, CIPersistStream) &&
          IS_VALID_CODE_PTR(pcimk->lpVtbl->BindToObject, BindToObject) &&
          IS_VALID_CODE_PTR(pcimk->lpVtbl->BindToStorage, BindToStorage) &&
          IS_VALID_CODE_PTR(pcimk->lpVtbl->Reduce, Reduce) &&
          IS_VALID_CODE_PTR(pcimk->lpVtbl->ComposeWith, ComposeWith) &&
          IS_VALID_CODE_PTR(pcimk->lpVtbl->Enum, Enum) &&
          IS_VALID_CODE_PTR(pcimk->lpVtbl->IsEqual, IsEqual) &&
          IS_VALID_CODE_PTR(pcimk->lpVtbl->Hash, Hash) &&
          IS_VALID_CODE_PTR(pcimk->lpVtbl->IsRunning, IsRunning) &&
          IS_VALID_CODE_PTR(pcimk->lpVtbl->GetTimeOfLastChange, GetTimeOfLastChange) &&
          IS_VALID_CODE_PTR(pcimk->lpVtbl->Inverse, Inverse) &&
          IS_VALID_CODE_PTR(pcimk->lpVtbl->CommonPrefixWith, CommonPrefixWith) &&
          IS_VALID_CODE_PTR(pcimk->lpVtbl->RelativePathTo, RelativePathTo) &&
          IS_VALID_CODE_PTR(pcimk->lpVtbl->GetDisplayName, GetDisplayName) &&
          IS_VALID_CODE_PTR(pcimk->lpVtbl->ParseDisplayName, ParseDisplayName) &&
          IS_VALID_CODE_PTR(pcimk->lpVtbl->IsSystemMoniker, IsSystemMoniker));
}


/*
** IsValidPCIPersist()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidPCIPersist(PCIPersist pcip)
{
   return(IS_VALID_READ_PTR(pcip, CIUnknown) &&
          IS_VALID_READ_PTR(pcip->lpVtbl, sizeof(*(pcip->lpVtbl))) &&
          IS_VALID_STRUCT_PTR((PCIUnknown)pcip, CIUnknown) &&
          IS_VALID_CODE_PTR(pcip->lpVtbl->GetClassID, GetClassID));
}


/*
** IsValidPCIPersistFile()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidPCIPersistFile(PCIPersistFile pcipfile)
{
   return(IS_VALID_READ_PTR(pcipfile, CIPersistFile) &&
          IS_VALID_READ_PTR(pcipfile->lpVtbl, sizeof(*(pcipfile->lpVtbl))) &&
          IS_VALID_STRUCT_PTR((PCIPersist)pcipfile, CIPersist) &&
          IS_VALID_CODE_PTR(pcipfile->lpVtbl->IsDirty, IsDirty) &&
          IS_VALID_CODE_PTR(pcipfile->lpVtbl->Load, Load) &&
          IS_VALID_CODE_PTR(pcipfile->lpVtbl->Save, Save) &&
          IS_VALID_CODE_PTR(pcipfile->lpVtbl->SaveCompleted, SaveCompleted) &&
          IS_VALID_CODE_PTR(pcipfile->lpVtbl->GetCurFile, GetCurFile));
}


/*
** IsValidPCIPersistStorage()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidPCIPersistStorage(PCIPersistStorage pcipstg)
{
   return(IS_VALID_READ_PTR(pcipstg, CIPersistStorage) &&
          IS_VALID_READ_PTR(pcipstg->lpVtbl, sizeof(*(pcipstg->lpVtbl))) &&
          IS_VALID_STRUCT_PTR((PCIPersist)pcipstg, CIPersist) &&
          IS_VALID_CODE_PTR(pcipstg->lpVtbl->IsDirty, IsDirty) &&
          IS_VALID_CODE_PTR(pcipstg->lpVtbl->InitNew, InitNew) &&
          IS_VALID_CODE_PTR(pcipstg->lpVtbl->Load, Load) &&
          IS_VALID_CODE_PTR(pcipstg->lpVtbl->Save, Save) &&
          IS_VALID_CODE_PTR(pcipstg->lpVtbl->SaveCompleted, SaveCompleted) &&
          IS_VALID_CODE_PTR(pcipstg->lpVtbl->HandsOffStorage, HandsOffStorage));
}


/*
** IsValidPCIPersistStream()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidPCIPersistStream(PCIPersistStream pcipstr)
{
   return(IS_VALID_READ_PTR(pcipstr, CIPersistStream) &&
          IS_VALID_READ_PTR(pcipstr->lpVtbl, sizeof(*(pcipstr->lpVtbl))) &&
          IS_VALID_STRUCT_PTR((PCIPersist)pcipstr, CIPersist) &&
          IS_VALID_CODE_PTR(pcipstr->lpVtbl->IsDirty, IsDirty) &&
          IS_VALID_CODE_PTR(pcipstr->lpVtbl->Load, Load) &&
          IS_VALID_CODE_PTR(pcipstr->lpVtbl->Save, Save) &&
          IS_VALID_CODE_PTR(pcipstr->lpVtbl->GetSizeMax, GetSizeMax));
}


/*
** IsValidPCIStorage()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidPCIStorage(PCIStorage pcistg)
{
   return(IS_VALID_READ_PTR(pcistg, CIStorage) &&
          IS_VALID_READ_PTR(pcistg->lpVtbl, sizeof(*(pcistg->lpVtbl))) &&
          IS_VALID_STRUCT_PTR((PCIUnknown)pcistg, CIUnknown) &&
          IS_VALID_CODE_PTR(pcistg->lpVtbl->CreateStream, CreateStream) &&
          IS_VALID_CODE_PTR(pcistg->lpVtbl->OpenStream, OpenStream) &&
          IS_VALID_CODE_PTR(pcistg->lpVtbl->CreateStorage, CreateStorage) &&
          IS_VALID_CODE_PTR(pcistg->lpVtbl->OpenStorage, OpenStorage) &&
          IS_VALID_CODE_PTR(pcistg->lpVtbl->CopyTo, CopyTo) &&
          IS_VALID_CODE_PTR(pcistg->lpVtbl->MoveElementTo, MoveElementTo) &&
          IS_VALID_CODE_PTR(pcistg->lpVtbl->Commit, Commit) &&
          IS_VALID_CODE_PTR(pcistg->lpVtbl->Revert, Revert) &&
          IS_VALID_CODE_PTR(pcistg->lpVtbl->EnumElements, EnumElements) &&
          IS_VALID_CODE_PTR(pcistg->lpVtbl->DestroyElement, DestroyElement) &&
          IS_VALID_CODE_PTR(pcistg->lpVtbl->RenameElement, RenameElement) &&
          IS_VALID_CODE_PTR(pcistg->lpVtbl->SetElementTimes, SetElementTimes) &&
          IS_VALID_CODE_PTR(pcistg->lpVtbl->SetClass, SetClass) &&
          IS_VALID_CODE_PTR(pcistg->lpVtbl->SetStateBits, SetStateBits) &&
          IS_VALID_CODE_PTR(pcistg->lpVtbl->Stat, Stat));
}


/*
** IsValidPCIStream()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidPCIStream(PCIStream pcistr)
{
   return(IS_VALID_READ_PTR(pcistr, CIStorage) &&
          IS_VALID_READ_PTR(pcistr->lpVtbl, sizeof(*(pcistr->lpVtbl))) &&
          IS_VALID_STRUCT_PTR((PCIUnknown)pcistr, CIUnknown) &&
          IS_VALID_CODE_PTR(pcistr->lpVtbl->Read, Read) &&
          IS_VALID_CODE_PTR(pcistr->lpVtbl->Write, Write) &&
          IS_VALID_CODE_PTR(pcistr->lpVtbl->Seek, Seek) &&
          IS_VALID_CODE_PTR(pcistr->lpVtbl->SetSize, SetSize) &&
          IS_VALID_CODE_PTR(pcistr->lpVtbl->CopyTo, CopyTo) &&
          IS_VALID_CODE_PTR(pcistr->lpVtbl->Commit, Commit) &&
          IS_VALID_CODE_PTR(pcistr->lpVtbl->Revert, Revert) &&
          IS_VALID_CODE_PTR(pcistr->lpVtbl->LockRegion, LockRegion) &&
          IS_VALID_CODE_PTR(pcistr->lpVtbl->UnlockRegion, UnlockRegion) &&
          IS_VALID_CODE_PTR(pcistr->lpVtbl->Stat, Stat) &&
          IS_VALID_CODE_PTR(pcistr->lpVtbl->Clone, Clone));
}


/*
** IsValidPCIUnknown()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidPCIUnknown(PCIUnknown pciunk)
{
   return(IS_VALID_READ_PTR(pciunk, CIUnknown) &&
          IS_VALID_READ_PTR(pciunk->lpVtbl, sizeof(*(pciunk->lpVtbl))) &&
          IS_VALID_CODE_PTR(pciunk->lpVtbl->QueryInterface, QueryInterface) &&
          IS_VALID_CODE_PTR(pciunk->lpVtbl->AddRef, AddRef) &&
          IS_VALID_CODE_PTR(pciunk->lpVtbl->Release, Release));
}

#endif

