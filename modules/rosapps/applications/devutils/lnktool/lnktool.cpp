/*
 * PROJECT:     lnktool
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     ShellLink (.lnk) utility
 * COPYRIGHT:   Copyright 2025 Whindmar Saksit <whindsaks@proton.me>
 */

#define INDENT " "
static const char g_Usage[] = ""
    "lnktool <Create|Dump|RemoveDatablock> <LnkFile> [Options]\n"
    "\n"
    "Create Options:\n"
    "/Pidl <Path>\n"
    "/Path <TargetPath>\n"
    "/Icon <Path,Index>\n"
    "/Arguments <String>\n"
    "/RelativePath <LnkPath>\n"
    "/WorkingDir <Path>\n"
    "/Comment <String>\n"
    "/ShowCmd <Number>\n"
    "/SpecialFolderOffset <CSIDL> <Offset|-Count>\n"
    "/AddExp <TargetPath>\n"
    "/AddExpIcon <Path>\n"
    "/RemoveExpIcon\n" // Removes the EXP_SZ_ICON block and flag
    "/AddSLDF <Flag>\n"
    "/RemoveSLDF <Flag>\n";

#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <wchar.h>
#include <stdio.h> // For shellutils.h (sprintf)
#include <shellutils.h>
#include <shlwapi_undoc.h>
#include <undocshell.h> // SHELL_LINK_HEADER

enum { slgp_relativepriority = 0x08 };
#define PT_DESKTOP_REGITEM      0x1F
#define PT_COMPUTER_REGITEM     0x2E
#define PT_FS                   0x30
#define PT_FS_UNICODE_FLAG      0x04

static const struct { UINT Flag; PCSTR Name; } g_SLDF[] =
{
    { SLDF_HAS_ID_LIST, "IDLIST" },
    { SLDF_HAS_LINK_INFO, "LINKINFO" },
    { SLDF_HAS_NAME, "NAME" },
    { SLDF_HAS_RELPATH, "RELPATH" },
    { SLDF_HAS_WORKINGDIR, "WORKINGDIR" },
    { SLDF_HAS_ARGS, "ARGS" },
    { SLDF_HAS_ICONLOCATION, "ICONLOCATION" },
    { SLDF_UNICODE, "UNICODE" },
    { SLDF_FORCE_NO_LINKINFO, "FORCE_NO_LINKINFO" },
    { SLDF_HAS_EXP_SZ, "EXP_SZ" },
    { SLDF_RUN_IN_SEPARATE, "RUN_IN_SEPARATE" },
    { 0x00000800, "LOGO3ID" },
    { SLDF_HAS_DARWINID, "DARWINID" },
    { SLDF_RUNAS_USER, "RUNAS_USER" },
    { SLDF_HAS_EXP_ICON_SZ, "EXP_ICON_SZ" },
    { SLDF_NO_PIDL_ALIAS, "NO_PIDL_ALIAS" },
    { SLDF_FORCE_UNCNAME, "FORCE_UNCNAME" },
    { SLDF_RUN_WITH_SHIMLAYER, "RUN_WITH_SHIMLAYER" },
    { 0x00040000, "FORCE_NO_LINKTRACK" },
    { 0x00080000, "ENABLE_TARGET_METADATA" },
    { 0x00100000, "DISABLE_LINK_PATH_TRACKING" },
    { 0x00200000, "DISABLE_KNOWNFOLDER_RELATIVE_TRACKING" },
    { 0x00400000, "NO_KF_ALIAS" },
    { 0x00800000, "ALLOW_LINK_TO_LINK" },
    { 0x01000000, "UNALIAS_ON_SAVE" },
    { 0x02000000, "PREFER_ENVIRONMENT_PATH" },
    { 0x04000000, "KEEP_LOCAL_IDLIST_FOR_UNC_TARGET" },
    { 0x08000000, "PERSIST_VOLUME_ID_RELATIVE" },
};

static const struct { UINT Flag; PCSTR Name; } g_DBSig[] =
{
    { EXP_SZ_LINK_SIG, "SZ_LINK" },
    { EXP_SZ_ICON_SIG, "SZ_ICON" },
    { EXP_SPECIAL_FOLDER_SIG, "SPECIALFOLDER" },
    { EXP_TRACKER_SIG, "TRACKER" },
    { 0xA0000009, "PROPERTYSTORAGE" },
    { EXP_KNOWN_FOLDER_SIG, "KNOWNFOLDER" },
    { EXP_VISTA_ID_LIST_SIG, "VISTAPIDL" },
};

static LONG StrToNum(PCWSTR in)
{
    PWCHAR end;
    LONG v = wcstol(in, &end, 0);
    if (v == LONG_MAX)
        v = wcstoul(in, &end, 0);
    return (end > in) ? v : 0;
}

template<class T>
static UINT MapToNumber(PCWSTR Name, const T &Map)
{
    CHAR buf[200];
    WideCharToMultiByte(CP_ACP, 0, Name, -1, buf, _countof(buf), NULL, NULL);
    buf[_countof(buf) - 1] = ANSI_NULL;
    for (UINT i = 0; i < _countof(Map); ++i)
    {
        if (!StrCmpIA(buf, Map[i].Name))
            return Map[i].Flag;
    }
    return StrToNum(Name);
}

template<class T>
static PCSTR MapToName(UINT Value, const T &Map, PCSTR Fallback = NULL)
{
    for (UINT i = 0; i < _countof(Map); ++i)
    {
        if (Map[i].Flag == Value)
            return Map[i].Name;
    }
    return Fallback;
}

#define GetSLDF(Name) MapToNumber((Name), g_SLDF)
#define GetDatablockSignature(Name) MapToNumber((Name), g_DBSig)

static int ErrMsg(int Error)
{
    WCHAR buf[400];
    for (UINT e = Error, cch; ;)
    {
        lstrcpynW(buf, L"?", _countof(buf));
        cch = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, e, 0, buf, _countof(buf), NULL);
        while (cch && buf[cch - 1] <= ' ')
            buf[--cch] = UNICODE_NULL; // Remove trailing newlines
        if (cch || HIWORD(e) != HIWORD(HRESULT_FROM_WIN32(1)))
            break;
        e = HRESULT_CODE(e); // "WIN32_FROM_HRESULT"
    }
    wprintf(Error < 0 ? L"Error 0x%.8X %s\n" : L"Error %d %s\n", Error, buf);
    return Error;
}

static int SuccessOrReportError(int Error)
{
    return Error ? ErrMsg(Error) : Error;
}

static inline HRESULT Seek(IStream *pStrm, int Move, int Origin = FILE_CURRENT, ULARGE_INTEGER *pNewPos = NULL)
{
    LARGE_INTEGER Pos;
    Pos.QuadPart = Move;
    return pStrm->Seek(Pos, Origin, pNewPos);
}

static HRESULT IL_LoadFromStream(IStream *pStrm, PIDLIST_ABSOLUTE *ppidl)
{
    HMODULE hShell32 = LoadLibraryA("SHELL32");
    HRESULT (WINAPI*ILLFS)(IStream*, PIDLIST_ABSOLUTE*);
    (FARPROC&)ILLFS = GetProcAddress(hShell32, (char*)26); // NT5
    if (!ILLFS)
        (FARPROC&)ILLFS = GetProcAddress(hShell32, (char*)846); // NT6
    return ILLFS(pStrm, ppidl);
}

static HRESULT SHParseName(PCWSTR Path, PIDLIST_ABSOLUTE *ppidl)
{
    HMODULE hShell32 = LoadLibraryA("SHELL32");
    int (WINAPI*SHPDN)(PCWSTR, void*, PIDLIST_ABSOLUTE*, UINT, UINT*);
    (FARPROC&)SHPDN = GetProcAddress(hShell32, "SHParseDisplayName");
    if (SHPDN)
        return SHPDN(Path, NULL, ppidl, 0, NULL);
    return SHILCreateFromPath(Path, ppidl, NULL);
}

static HRESULT SHParseNameEx(PCWSTR Path, PIDLIST_ABSOLUTE *ppidl, bool Simple)
{
    if (!Simple)
        return SHParseName(Path, ppidl);
    *ppidl = SHSimpleIDListFromPath(Path);
    return *ppidl ? S_OK : E_FAIL;
}

static INT_PTR SHGetPidlInfo(LPCITEMIDLIST pidl, SHFILEINFOW &shfi, UINT Flags, UINT Depth = 0)
{
    LPITEMIDLIST pidlDup = NULL, pidlChild;
    if (Depth > 0)
    {
        if ((pidl = pidlDup = pidlChild = ILClone(pidl)) == NULL)
            return 0;
        while (Depth--)
        {
            if (LPITEMIDLIST pidlNext = ILGetNext(pidlChild))
                pidlChild = pidlNext;
        }
        pidlChild->mkid.cb = 0;
    }
    INT_PTR ret = SHGetFileInfoW((PWSTR)pidl, 0, &shfi, sizeof(shfi), Flags | SHGFI_PIDL);
    ILFree(pidlDup);
    return ret;
}

static void PrintOffsetString(PCSTR Name, LPCVOID Base, UINT Min, UINT Ansi, UINT Unicode, PCSTR Indent = "")
{
    if (Unicode && Unicode >= Min)
        wprintf(L"%hs%hs=%ls", Indent, Name, (PWSTR)((BYTE*)Base + Unicode));
    else
        wprintf(L"%hs%hs=%hs", Indent, Name, Ansi >= Min ? (char*)Base + Ansi : "");
    wprintf(L"\n"); // Separate function call in case the (untrusted) input ends with a DEL character
}

static void Print(PCSTR Name, REFGUID Guid, PCSTR Indent = "")
{
    WCHAR Buffer[39];
    StringFromGUID2(Guid, Buffer, _countof(Buffer));
    wprintf(L"%hs%hs=%ls\n", Indent, Name, Buffer);
}

template<class V, class T>
static void DumpFlags(V Value, T *pInfo, UINT Count, PCSTR Prefix = NULL)
{
    if (!Prefix)
        Prefix = "";
    for (SIZE_T i = 0; i < Count; ++i)
    {
        if (Value & pInfo[i].Flag)
            wprintf(L"%hs%#.8x:%hs\n", Prefix, pInfo[i].Flag, const_cast<PCSTR>(pInfo[i].Name));
        Value &= ~pInfo[i].Flag;
    }
    if (Value)
        wprintf(L"%hs%#.8x:%hs\n", Prefix, Value, "?");
}

static void Dump(LPITEMIDLIST pidl, PCSTR Heading = NULL)
{
    struct GUIDPIDL { WORD cb; BYTE Type, Unknown; GUID guid; };
    struct DRIVE { BYTE cb1, cb2, Type; char Name[4]; BYTE Unk[18]; };
    struct FS95 { WORD cb; BYTE Type, Unknown, Data[4+2+2+2]; char Name[1]; };
    if (Heading)
        wprintf(L"%hs ", Heading);
    if (!pidl || !pidl->mkid.cb)
        wprintf(L"[Desktop (%hs)]", pidl ? "Empty" : "NULL");
    for (UINT i = 0; pidl && pidl->mkid.cb; ++i, pidl = ILGetNext(pidl))
    {
        SHFILEINFOW shfi;
        PWSTR buf = shfi.szDisplayName;
        GUIDPIDL *pidlGUID = (GUIDPIDL*)pidl;
        UINT cb = pidl->mkid.cb;
        UINT type = cb >= 3 ? (pidlGUID->Type & ~0x80) : 0, folder = type & 0x70;
        if (i)
            wprintf(L" ");
        if (cb < 3)
        {
            wprintf(L"[? %ub]", cb);
        }
        else if (i == 0 && cb == sizeof(GUIDPIDL) && type == PT_DESKTOP_REGITEM) guiditem:
        {
            if (!SHGetPidlInfo(pidl, shfi, SHGFI_DISPLAYNAME, 1))
                StringFromGUID2(*(GUID*)((char*)pidl + cb - 16), buf, 39);
            wprintf(L"[%.2X %ub %s]", pidl->mkid.abID[0], cb, buf);
        }
        else if (i == 1 && cb == sizeof(GUIDPIDL) && type == PT_COMPUTER_REGITEM)
        {
            goto guiditem;
        }
        else if (i == 1 && cb == sizeof(DRIVE) && folder == 0x20)
        {
            DRIVE *p = (DRIVE*)pidl;
            wprintf(L"[%.2X %ub \"%.3hs\"]", p->Type, cb, p->Name);
        }
        else if (cb > FIELD_OFFSET(FS95, Name) && folder == PT_FS)
        {
            FS95 *p = (FS95*)pidl;
            const BOOL wide = type & PT_FS_UNICODE_FLAG;
            wprintf(wide ? L"[%.2X %ub \"%.256ls\"]" : L"[%.2X %ub \"%.256hs\"]", p->Type, cb, p->Name);
        }
        else
        {
            wprintf(L"[%.2X %ub ?]", pidl->mkid.abID[0], cb);
        }
    }
    wprintf(L"\n");
}

static HRESULT Save(IShellLink *pSL, PCWSTR LnkPath)
{
    HRESULT hr;
    IPersistFile *pPF;
    if (SUCCEEDED(hr = pSL->QueryInterface(IID_PPV_ARG(IPersistFile, &pPF))))
    {
        if (SUCCEEDED(hr = pPF->Save(LnkPath, FALSE)) && hr != S_OK)
            hr = E_FAIL;
        pPF->Release();
    }
    return hr;
}

static HRESULT Load(IUnknown *pUnk, IStream *pStream)
{
    IPersistStream *pPS;
    HRESULT hr = pUnk->QueryInterface(IID_PPV_ARG(IPersistStream, &pPS));
    if (SUCCEEDED(hr))
    {
        hr = pPS->Load(pStream);
        pPS->Release();
    }
    return hr;
}

static HRESULT Open(PCWSTR Path, IStream **ppStream, IShellLink **ppLink, UINT Mode = STGM_READ)
{
    if (Mode & (STGM_WRITE | STGM_READWRITE))
        Mode = (Mode & ~STGM_WRITE) | STGM_READWRITE | STGM_SHARE_DENY_WRITE;
    else
        Mode |= STGM_READ | STGM_SHARE_DENY_NONE;
    IStream *pStream;
    HRESULT hr = SHCreateStreamOnFileW(Path, Mode, &pStream);
    if (SUCCEEDED(hr) && ppLink)
    {
        hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IShellLink, ppLink));
        if (SUCCEEDED(hr) && !(Mode & STGM_CREATE))
        {
            if (FAILED(hr = Load(*ppLink, pStream)))
            {
                (*ppLink)->Release();
                *ppLink = NULL;
            }
            IStream_Reset(pStream);
        }
    }

    if (SUCCEEDED(hr) && ppStream)
        *ppStream = pStream;
    else if (pStream)
        pStream->Release();
    return hr;
}

#define FreeBlock SHFree
static inline LPDATABLOCK_HEADER NextBlock(LPDATABLOCK_HEADER pdbh)
{
    return (LPDATABLOCK_HEADER)((char*)pdbh + pdbh->cbSize);
}

template<class T>
static HRESULT ReadBlock(IStream *pStream, T **ppData = NULL)
{
    DWORD cb = 0;
    HRESULT hr = IStream_Read(pStream, &cb, sizeof(cb));
    if (SUCCEEDED(hr))
    {
        UINT Remain = cb - min(sizeof(DWORD), cb);
        if (!ppData)
            return Seek(pStream, Remain);
        *ppData = (T*)SHAlloc(Remain + sizeof(DWORD));
        if (!*ppData)
            return E_OUTOFMEMORY;
        ((DATABLOCK_HEADER*)*ppData)->cbSize = cb;
        if (SUCCEEDED(hr = IStream_Read(pStream, &((DATABLOCK_HEADER*)*ppData)->dwSignature, Remain)))
            return cb;
    }
    return hr;
}

struct DataBlockEnum
{
    DataBlockEnum(LPDATABLOCK_HEADER pHead) : m_It(pHead) {}

    LPDATABLOCK_HEADER Get()
    {
        if (m_It && m_It->cbSize >= FIELD_OFFSET(DATABLOCK_HEADER, dwSignature))
            return m_It + (m_It->dwSignature == ~0UL); // SHFindDataBlock compatible
        return NULL;
    }
    void Next()
    {
        if (m_It && m_It->cbSize)
            m_It = NextBlock(m_It);
    }
    LPDATABLOCK_HEADER m_It;
};

static HRESULT ReadAndDumpString(PCSTR Name, UINT Id, const SHELL_LINK_HEADER &slh, IStream *pStream)
{
    if (!(Id & slh.dwFlags))
        return S_OK;
    WORD cch;
    HRESULT hr = IStream_Read(pStream, &cch, sizeof(cch));
    if (FAILED(hr))
        return hr;
    UINT cb = (UINT)cch * ((slh.dwFlags & SLDF_UNICODE) ? sizeof(WCHAR) : sizeof(CHAR));
    void *data = SHAlloc(cb);
    if (!data)
        return E_OUTOFMEMORY;
    if (FAILED(hr = IStream_Read(pStream, data, cb)))
        return hr;
    if (slh.dwFlags & SLDF_UNICODE)
        wprintf(L"%hs=%.*ls\n", Name, cch, (PWSTR)data);
    else
        wprintf(L"%hs=%.*hs\n", Name, cch, (PCSTR)data);
    SHFree(data);
    return S_OK;
}

static HRESULT DumpCommand(PCWSTR Path)
{
    IStream *pStream;
    HRESULT hr = Open(Path, &pStream, NULL);
    if (FAILED(hr))
        return hr;

    LPITEMIDLIST pidl;
    SHELL_LINK_HEADER slh;
    if (SUCCEEDED(hr = IStream_Read(pStream, &slh, sizeof(slh))))
    {
        #define DUMPSLH(name, fmt, field) wprintf(L"%hs=" fmt L"\n", (name), (slh).field)
        DUMPSLH("Flags", L"%#.8x", dwFlags);
        DumpFlags(slh.dwFlags, g_SLDF, _countof(g_SLDF), INDENT);
        DUMPSLH("FileAttributes", L"%#.8x", dwFileAttributes);
        DUMPSLH("Size", L"%u", nFileSizeLow);
        DUMPSLH("IconIndex", L"%d", nIconIndex);
        DUMPSLH("ShowCommand", L"%d", nShowCommand);
        DUMPSLH("HotKey", L"%#.4x", wHotKey);

        if (SUCCEEDED(hr) && (slh.dwFlags & SLDF_HAS_ID_LIST))
        {
            if (SUCCEEDED(hr = IL_LoadFromStream(pStream, &pidl)))
            {
                Dump(pidl, "PIDL:");
                ILFree(pidl);
            }
        }
        if (SUCCEEDED(hr) && (slh.dwFlags & SLDF_HAS_LINK_INFO))
        {
            int offset;
            SHELL_LINK_INFOW *p;
            if ((hr = ReadBlock(pStream, &p)) >= (signed)sizeof(SHELL_LINK_INFOA))
            {
                wprintf(L"%hs:\n", "LINKINFO");
                wprintf(L"%hs%hs=%#.8x\n", INDENT, "Flags", p->dwFlags);
                if (p->dwFlags & SLI_VALID_LOCAL)
                {
                    if (p->cbVolumeIDOffset)
                    {
                        SHELL_LINK_INFO_VOLUME_IDW *pVol = (SHELL_LINK_INFO_VOLUME_IDW*)((char*)p + p->cbVolumeIDOffset);
                        wprintf(L"%hs%hs=%d\n", INDENT, "DriveType", pVol->dwDriveType);
                        wprintf(L"%hs%hs=%#.8x\n", INDENT, "Serial", pVol->nDriveSerialNumber);
                        offset = pVol->cbVolumeLabelOffset == 0x14 ? pVol->cbVolumeLabelUnicodeOffset : 0;
                        if (offset || pVol->cbVolumeLabelOffset != 0x14) // 0x14 from [MS-SHLLINK] documentation
                            PrintOffsetString("Label", pVol, 0x10, pVol->cbVolumeLabelOffset, offset, INDENT);
                    }
                    offset = p->cbHeaderSize >= sizeof(SHELL_LINK_INFOW) ? p->cbCommonPathSuffixUnicodeOffset : 0;
                    PrintOffsetString("CommonSuffix", p, sizeof(SHELL_LINK_INFOA), p->cbCommonPathSuffixOffset, offset, INDENT);

                    offset = p->cbHeaderSize >= sizeof(SHELL_LINK_INFOW) ? p->cbLocalBasePathUnicodeOffset : 0;
                    PrintOffsetString("LocalBase", p, sizeof(SHELL_LINK_INFOA), p->cbLocalBasePathOffset, offset, INDENT);
                }
                SHELL_LINK_INFO_CNR_LINKW *pCNR = (SHELL_LINK_INFO_CNR_LINKW*)((char*)p + p->cbCommonNetworkRelativeLinkOffset);
                if ((p->dwFlags & SLI_VALID_NETWORK) && p->cbCommonNetworkRelativeLinkOffset)
                {
                    wprintf(L"%hs%hs=%#.8x\n", INDENT, "CNR", pCNR->dwFlags);
                    wprintf(L"%hs%hs=%#.8x\n", INDENT, "Provider", pCNR->dwNetworkProviderType); // WNNC_NET_*

                    offset = pCNR->cbNetNameOffset > 0x14 ? pCNR->cbNetNameUnicodeOffset : 0;
                    PrintOffsetString("NetName", pCNR, sizeof(SHELL_LINK_INFO_CNR_LINKA), pCNR->cbNetNameOffset, offset, INDENT);

                    offset = pCNR->cbNetNameOffset > 0x14 ? pCNR->cbDeviceNameUnicodeOffset : 0;
                    if (pCNR->dwFlags & SLI_CNR_VALID_DEVICE)
                        PrintOffsetString("DeviceName", pCNR, sizeof(SHELL_LINK_INFO_CNR_LINKA), pCNR->cbDeviceNameOffset, offset, INDENT);
                }
                FreeBlock(p);
            }
        }
        if (SUCCEEDED(hr))
            hr = ReadAndDumpString("NAME", SLDF_HAS_NAME, slh, pStream);
        if (SUCCEEDED(hr))
            hr = ReadAndDumpString("RELPATH", SLDF_HAS_RELPATH, slh, pStream);
        if (SUCCEEDED(hr))
            hr = ReadAndDumpString("WORKINGDIR", SLDF_HAS_WORKINGDIR, slh, pStream);
        if (SUCCEEDED(hr))
            hr = ReadAndDumpString("ARGS", SLDF_HAS_ARGS, slh, pStream);
        if (SUCCEEDED(hr))
            hr = ReadAndDumpString("ICONLOCATION", SLDF_HAS_ICONLOCATION, slh, pStream);

        LPDATABLOCK_HEADER pDBListHead, pDBH;
        if (SUCCEEDED(hr) && SUCCEEDED(hr = SHReadDataBlockList(pStream, &pDBListHead)))
        {
            for (DataBlockEnum it(pDBListHead); (pDBH = it.Get()) != NULL; it.Next())
            {
                PCSTR SigName = MapToName(pDBH->dwSignature, g_DBSig, NULL);
                wprintf(L"DataBlock: %.8X %ub", pDBH->dwSignature, pDBH->cbSize);
                void *pFirstMember = ((EXP_SZ_LINK*)pDBH)->szTarget;
                if (pDBH->dwSignature == EXP_SZ_LINK_SIG && pDBH->cbSize > FIELD_OFFSET(EXP_SZ_LINK, szwTarget))
                {
                    wprintf(L" %hs\n", "SZ_LINK");
                printdbpaths:
                    EXP_SZ_LINK *p = (EXP_SZ_LINK*)pDBH;
                    wprintf(L"%hs%hs=%.260hs\n", INDENT, "Ansi", p->szTarget);
                    wprintf(L"%hs%hs=%.260ls\n", INDENT, "Wide", p->szwTarget);
                }
                else if (pDBH->dwSignature == EXP_SZ_ICON_SIG && pDBH->cbSize > FIELD_OFFSET(EXP_SZ_LINK, szwTarget))
                {
                    wprintf(L" %hs\n", "SZ_ICON/LOGO3");
                    goto printdbpaths;
                }
                else if (pDBH->dwSignature == EXP_DARWIN_ID_SIG && pDBH->cbSize == sizeof(EXP_DARWIN_LINK))
                {
                    wprintf(L" %hs\n", "DARWIN_LINK");
                    goto printdbpaths;
                }
                else if (pDBH->dwSignature == NT_CONSOLE_PROPS_SIG && pDBH->cbSize >= sizeof(NT_CONSOLE_PROPS))
                {
                    wprintf(L" %hs\n", "NT_CONSOLE_PROPS");
                    NT_CONSOLE_PROPS *p = (NT_CONSOLE_PROPS*)pDBH;
                    wprintf(L"%hsInsert=%d Quick=%d %ls\n", INDENT, p->bInsertMode, p->bQuickEdit, p->FaceName);
                }
                else if (pDBH->dwSignature == EXP_SPECIAL_FOLDER_SIG && pDBH->cbSize == sizeof(EXP_SPECIAL_FOLDER))
                {
                    wprintf(L" %hs\n", SigName);
                    EXP_SPECIAL_FOLDER *p = (EXP_SPECIAL_FOLDER*)pDBH;
                    wprintf(L"%hsCSIDL=%#x Offset=%#x\n", INDENT, p->idSpecialFolder, p->cbOffset);
                }
                else if (pDBH->dwSignature == EXP_TRACKER_SIG)
                {
                    wprintf(L" %hs\n", SigName);
                    EXP_TRACKER *p = (EXP_TRACKER*)pDBH;
                    UINT len = FIELD_OFFSET(EXP_TRACKER, nLength) + p->nLength;
                    if (len >= FIELD_OFFSET(EXP_TRACKER, szMachineID))
                        wprintf(L"%hsVersion=%d\n", INDENT, p->nVersion);
                    if (len >= FIELD_OFFSET(EXP_TRACKER, guidDroidVolume))
                        wprintf(L"%hsMachine=%hs\n", INDENT, p->szMachineID);
                    if (len >= FIELD_OFFSET(EXP_TRACKER, guidDroidObject))
                        Print("Volume", p->guidDroidVolume, INDENT);
                    if (len >= FIELD_OFFSET(EXP_TRACKER, guidDroidBirthVolume))
                        Print("Object", p->guidDroidObject, INDENT);
                    if (len >= FIELD_OFFSET(EXP_TRACKER, guidDroidBirthObject))
                        Print("BirthVolume", p->guidDroidBirthVolume, INDENT);
                    if (len >= sizeof(EXP_TRACKER))
                        Print("BirthObject", p->guidDroidBirthObject, INDENT);
                }
                else if (pDBH->dwSignature == EXP_SHIM_SIG)
                {
                    wprintf(L" %hs\n", SigName ? SigName : "SHIM");
                    wprintf(L"%hs%ls\n", INDENT, (PWSTR)pFirstMember);
                }
                else if (pDBH->dwSignature == EXP_VISTA_ID_LIST_SIG && pDBH->cbSize >= 8 + 2)
                {
                    wprintf(L" %hs\n", SigName);
                    Dump((LPITEMIDLIST)pFirstMember, INDENT + 1);
                }
                else
                {
                    wprintf(SigName ? L" %hs\n" : L"\n", SigName);
                }
            }
            SHFreeDataBlockList(pDBListHead);
        }
    }
    pStream->Release();

    // Now dump using the API
    HRESULT hr2;
    WCHAR buf[MAX_PATH * 2];
    IShellLink *pSL;
    if (FAILED(hr2 = Open(Path, NULL, &pSL)))
        return hr2;
    wprintf(L"\n");

    if (SUCCEEDED(hr2 = pSL->GetIDList(&pidl)))
    {
        Dump(pidl, "GetIDList:");
        ILFree(pidl);
    }
    else
    {
        wprintf(L"%hs: %#x\n", "GetIDList", hr2);
    }

    static const BYTE GetPathFlags[] = { 0, SLGP_SHORTPATH, SLGP_RAWPATH, slgp_relativepriority };
    for (UINT i = 0; i < _countof(GetPathFlags); ++i)
    {
        if (SUCCEEDED(hr2 = pSL->GetPath(buf, _countof(buf), NULL, GetPathFlags[i])))
            wprintf(L"GetPath(%#.2x): %ls\n", GetPathFlags[i], buf);
        else
            wprintf(L"GetPath(%#.2x): %#x\n", GetPathFlags[i], hr2);
    }

    if (SUCCEEDED(hr2 = pSL->GetWorkingDirectory(buf, _countof(buf))))
        wprintf(L"%hs: %ls\n", "GetWorkingDirectory", buf);
    else
        wprintf(L"%hs: %#x\n", "GetWorkingDirectory", hr2);

    if (SUCCEEDED(hr2 = pSL->GetArguments(buf, _countof(buf))))
        wprintf(L"%hs: %ls\n", "GetArguments", buf);
    else
        wprintf(L"%hs: %#x\n", "GetArguments", hr2);

    if (SUCCEEDED(hr2 = pSL->GetDescription(buf, _countof(buf))))
        wprintf(L"%hs: %ls\n", "GetDescription", buf);
    else
        wprintf(L"%hs: %#x\n", "GetDescription", hr2);

    int index = 123456789;
    if (SUCCEEDED(hr2 = pSL->GetIconLocation(buf, _countof(buf), &index)))
        wprintf(L"%hs: %ls,%d\n", "GetIconLocation", buf, index);
    else
        wprintf(L"%hs: %#x\n", "GetIconLocation", hr2);

    IExtractIconW *pEI;
    if (SUCCEEDED(pSL->QueryInterface(IID_PPV_ARG(IExtractIconW, &pEI))))
    {
        index = 123456789;
        UINT flags = 0;
        if (SUCCEEDED(hr2 = pEI->GetIconLocation(0, buf, _countof(buf), &index, &flags)))
            wprintf(L"%hs: %#x %ls,%d %#.4x\n", "EI:GetIconLocation", hr2, buf, index, flags);
        else
            wprintf(L"%hs: %#x %#.4x\n", "EI:GetIconLocation", hr2, flags);
        pEI->Release();
    }

    pSL->Release();
    return hr;
}

#define RemoveSLDF(pSL, Flag) RemoveDatablock((pSL), 0, (Flag))
#define AddSLDF(pSL, Flag) RemoveDatablock((pSL), 0, 0, (Flag))

static HRESULT RemoveDatablock(IShellLinkW *pSL, UINT Signature, UINT KillFlag = 0, UINT AddFlag = 0)
{
    IShellLinkDataList *pSLDL;
    HRESULT hr = pSL->QueryInterface(IID_PPV_ARG(IShellLinkDataList, &pSLDL));
    if (SUCCEEDED(hr))
    {
        if (Signature)
            hr = pSLDL->RemoveDataBlock(Signature);
        DWORD OrgFlags;
        if ((AddFlag | KillFlag) && SUCCEEDED(pSLDL->GetFlags(&OrgFlags)))
            pSLDL->SetFlags((OrgFlags & ~KillFlag) | AddFlag);
        pSLDL->Release();
    }
    return hr;
}

template<class T>
static HRESULT AddDataBlock(IShellLinkW *pSL, T &DataBlock)
{
    IShellLinkDataList *pSLDL;
    HRESULT hr = pSL->QueryInterface(IID_PPV_ARG(IShellLinkDataList, &pSLDL));
    if (SUCCEEDED(hr))
    {
        hr = pSLDL->AddDataBlock(&DataBlock);
        pSLDL->Release();
    }
    return hr;
}

static BOOL TryGetUpdatedPidl(IShellLinkW *pSL, PIDLIST_ABSOLUTE &pidl)
{
    PIDLIST_ABSOLUTE pidlNew;
    if (pSL->GetIDList(&pidlNew) == S_OK)
    {
        ILFree(pidl);
        pidl = pidlNew;
    }
    return pidl != NULL;
}

static HRESULT CreateCommand(PCWSTR LnkPath, UINT argc, WCHAR **argv)
{
    IShellLinkW *pSL;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IShellLinkW, &pSL));
    if (FAILED(hr))
        return hr;

    UINT TargetCount = 0, DumpResult = 0, ForceAddSLDF = 0, ForceRemoveSLDF = 0;
    PIDLIST_ABSOLUTE pidlTarget = NULL;
    for (UINT i = 0; i < argc && SUCCEEDED(hr); ++i)
    {
        hr = E_INVALIDARG;
        if (!StrCmpIW(argv[i], L"/Pidl") && i + 1 < argc)
        {
            bool Simple = !StrCmpIW(argv[i + 1], L"/Force") && i + 2 < argc && ++i;
            if (SUCCEEDED(hr = SHParseNameEx(argv[++i], &pidlTarget, Simple)))
                TargetCount += SUCCEEDED(hr = pSL->SetIDList(pidlTarget));
        }
        else if (!StrCmpIW(argv[i], L"/Path") && i + 1 < argc)
        {
            TargetCount += SUCCEEDED(hr = pSL->SetPath(argv[++i]));
        }
        else if (!StrCmpIW(argv[i], L"/Icon") && i + 1 < argc)
        {
            int index = PathParseIconLocation(argv[++i]);
            hr = pSL->SetIconLocation(argv[i], index);
        }
        else if (!StrCmpIW(argv[i], L"/Arguments") && i + 1 < argc)
        {
            hr = pSL->SetArguments(argv[++i]);
        }
        else if (!StrCmpIW(argv[i], L"/RelativePath") && i + 1 < argc)
        {
            hr = pSL->SetRelativePath(argv[++i], 0);
        }
        else if (!StrCmpIW(argv[i], L"/WorkingDir") && i + 1 < argc)
        {
            hr = pSL->SetWorkingDirectory(argv[++i]);
        }
        else if (!StrCmpIW(argv[i], L"/Comment") && i + 1 < argc)
        {
            hr = pSL->SetDescription(argv[++i]);
        }
        else if (!StrCmpIW(argv[i], L"/ShowCmd") && i + 1 < argc)
        {
            if (int sw = StrToNum(argv[++i])) // Don't allow SW_HIDE
                hr = pSL->SetShowCmd(sw);
        }
        else if (!StrCmpIW(argv[i], L"/AddExp") && ++i < argc)
        {
            EXP_SZ_LINK db = { sizeof(db), EXP_SZ_LINK_SIG };
            WideCharToMultiByte(CP_ACP, 0, argv[i], -1, db.szTarget, _countof(db.szTarget), NULL, NULL);
            lstrcpynW(db.szwTarget, argv[i], _countof(db.szwTarget));
            if (SUCCEEDED(hr = AddDataBlock(pSL, db)))
                TargetCount += SUCCEEDED(hr = AddSLDF(pSL, SLDF_HAS_EXP_SZ));
        }
        else if (!StrCmpIW(argv[i], L"/AddExpIcon") && ++i < argc)
        {
            EXP_SZ_LINK db = { sizeof(db), EXP_SZ_ICON_SIG };
            WideCharToMultiByte(CP_ACP, 0, argv[i], -1, db.szTarget, _countof(db.szTarget), NULL, NULL);
            lstrcpynW(db.szwTarget, argv[i], _countof(db.szwTarget));
            if (SUCCEEDED(hr = AddDataBlock(pSL, db)))
                hr = AddSLDF(pSL, SLDF_HAS_EXP_ICON_SZ);
        }
        else if (!StrCmpIW(argv[i], L"/RemoveExpIcon"))
        {
            hr = RemoveDatablock(pSL, EXP_SZ_ICON_SIG, SLDF_HAS_EXP_ICON_SZ);
        }
        else if (!StrCmpIW(argv[i], L"/SpecialFolderOffset") && i + 2 < argc)
        {
            EXP_SPECIAL_FOLDER db = { sizeof(db), EXP_SPECIAL_FOLDER_SIG };
            if ((db.idSpecialFolder = StrToNum(argv[++i])) == 0)
            {
                if (!StrCmpIW(argv[i], L"Windows"))
                    db.idSpecialFolder = CSIDL_WINDOWS;
                if (!StrCmpIW(argv[i], L"System"))
                    db.idSpecialFolder = CSIDL_SYSTEM;
            }
            db.cbOffset = StrToNum(argv[++i]);
            if ((signed)db.cbOffset < 0 && TryGetUpdatedPidl(pSL, pidlTarget))
            {
                UINT i = 0, c = -(signed)db.cbOffset;
                db.cbOffset = 0;
                for (PIDLIST_ABSOLUTE pidl = pidlTarget; i < c && pidl->mkid.cb; ++i, pidl = ILGetNext(pidl))
                    db.cbOffset += pidl->mkid.cb;
            }
            hr = AddDataBlock(pSL, db);
        }
        else if (!StrCmpIW(argv[i], L"/RemoveDatablock"))
        {
            if (UINT sig = GetDatablockSignature(argv[++i]))
                hr = RemoveDatablock(pSL, sig);
        }
        else if (!StrCmpIW(argv[i], L"/AddSLDF") && i + 1 < argc)
        {
            bool Force = !StrCmpIW(argv[i + 1], L"/Force") && i + 2 < argc && ++i;
            UINT Flag = GetSLDF(argv[++i]);
            if (Flag > SLDF_UNICODE)
                hr = AddSLDF(pSL, Flag);
            if (Force)
                ForceAddSLDF |= Flag;
        }
        else if (!StrCmpIW(argv[i], L"/RemoveSLDF") && i + 1 < argc)
        {
            bool Force = !StrCmpIW(argv[i + 1], L"/Force") && i + 2 < argc && ++i;
            UINT Flag = GetSLDF(argv[++i]);
            if (Flag)
                hr = RemoveSLDF(pSL, Flag);
            if (Force)
                ForceRemoveSLDF |= Flag;
        }
        else if (!StrCmpIW(argv[i], L"/Dump"))
        {
            DumpResult++;
            hr = S_OK;
        }
        else if (!StrCmpIW(argv[i], L"/ForceCreate"))
        {
            TargetCount++;
            hr = S_OK;
        }
        else
        {
            wprintf(L"%hsUnable to parse \"%ls\"!\n", "Error: ", argv[i]);
        }
    }

    if (SUCCEEDED(hr))
    {
        if (TargetCount)
        {
            if (SUCCEEDED(hr = Save(pSL, LnkPath)) && (ForceAddSLDF | ForceRemoveSLDF))
            {
                IStream *pStream;
                if (SUCCEEDED(Open(LnkPath, &pStream, NULL, STGM_READWRITE)))
                {
                    SHELL_LINK_HEADER slh;
                    if (SUCCEEDED(IStream_Read(pStream, &slh, sizeof(slh))))
                    {
                        slh.dwFlags = (slh.dwFlags & ~ForceRemoveSLDF) | ForceAddSLDF;
                        if (SUCCEEDED(Seek(pStream, 0, FILE_BEGIN)))
                            IStream_Write(pStream, &slh, sizeof(slh));
                    }
                    pStream->Release();
                }
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        }

        if (SUCCEEDED(hr) && DumpResult)
            DumpCommand(LnkPath);
    }
    pSL->Release();
    ILFree(pidlTarget);
    return hr;
}

static HRESULT RemoveDatablockCommand(PCWSTR LnkPath, UINT argc, WCHAR **argv)
{
    IShellLink *pSL;
    HRESULT hr = Open(LnkPath, NULL, &pSL, STGM_READWRITE);
    if (FAILED(hr))
        return hr;

    hr = E_INVALIDARG;
    for (UINT i = 0; i < argc; ++i)
    {
        UINT Sig = GetDatablockSignature(argv[i]);
        if (!Sig)
        {
            hr = E_INVALIDARG;
            break;
        }
        hr = RemoveDatablock(pSL, Sig);
    }

    if (SUCCEEDED(hr))
        hr = Save(pSL, LnkPath);
    pSL->Release();
    return hr;
}

static int ProcessCommandLine(int argc, WCHAR **argv)
{
    if (argc < 3)
    {
        wprintf(L"%hs", g_Usage);
        return argc < 2 ? 0 : ERROR_INVALID_PARAMETER;
    }

    if (!StrCmpIW(argv[1], L"Dump"))
        return SuccessOrReportError(DumpCommand(argv[2]));
    if (!StrCmpIW(argv[1], L"Create"))
        return SuccessOrReportError(CreateCommand(argv[2], argc - 3, &argv[3]));
    if (!StrCmpIW(argv[1], L"RemoveDatablock"))
        return SuccessOrReportError(RemoveDatablockCommand(argv[2], argc - 3, &argv[3]));

    return SuccessOrReportError(ERROR_INVALID_PARAMETER);
}

EXTERN_C int wmain(int argc, WCHAR **argv)
{
    HRESULT hrInit = OleInitialize(NULL);
    int result = ProcessCommandLine(argc, argv);
    if (SUCCEEDED(hrInit))
        OleUninitialize();
    return result;
}
