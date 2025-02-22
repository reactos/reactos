/*
 * PROJECT:     ReactOS Picture and Fax Viewer
 * LICENSE:     GPL-2.0 (https://spdx.org/licenses/GPL-2.0)
 * PURPOSE:     Image file browsing and manipulation
 * COPYRIGHT:   Copyright 2025 Whindmar Saksit <whindsaks@proton.me>
 */

#include <windows.h>
#include <objbase.h>
#include <gdiplus.h>
using namespace Gdiplus;
#include "shimgvw.h"

#define HResultFromWin32 SHIMGVW_HResultFromWin32

static HRESULT Read(HANDLE hFile, void* Buffer, DWORD Size)
{
    DWORD Transferred;
    if (!ReadFile(hFile, Buffer, Size, &Transferred, NULL))
        return HResultFromWin32(GetLastError());
    return Size == Transferred ? S_OK : HResultFromWin32(ERROR_HANDLE_EOF);
}

struct IMAGESTATS
{
    UINT w, h;
    BYTE bpp;
};

class BitmapInfoHeader : public BITMAPINFOHEADER
{
public:
    BitmapInfoHeader() {}
    BitmapInfoHeader(const void* pbmiHeader) { Initialize(pbmiHeader); }

    void Initialize(const void* pbmiHeader)
    {
        BITMAPINFOHEADER& bih = *(BITMAPINFOHEADER*)pbmiHeader;
        if (bih.biSize >= sizeof(BITMAPINFOHEADER))
        {
            CopyMemory(this, &bih, min(bih.biSize, sizeof(*this)));
        }
        else
        {
            ZeroMemory(this, sizeof(*this));
            BITMAPCOREHEADER& bch = *(BITMAPCOREHEADER*)pbmiHeader;
            if (bih.biSize >= sizeof(BITMAPCOREHEADER))
            {
                biSize = bch.bcSize;
                biWidth = bch.bcWidth;
                biHeight = bch.bcHeight;
                biPlanes = bch.bcPlanes;
                biBitCount = bch.bcBitCount;
                biCompression = BI_RGB;
            }
        }
    }
};

#include <pshpack1.h>
union PNGSIGNATURE { UINT64 number; BYTE bytes[8]; };
struct PNGCHUNKHEADER { UINT length, type; };
struct PNGCHUNKFOOTER { UINT crc; };
struct PNGIHDR { UINT w, h; BYTE depth, type, compression, filter, interlace; };
struct PNGSIGANDIHDR
{
    PNGSIGNATURE sig;
    PNGCHUNKHEADER chunkheader;
    PNGIHDR ihdr;
    PNGCHUNKFOOTER chunkfooter;
};
struct PNGFOOTER { PNGCHUNKHEADER chunkheader; PNGCHUNKFOOTER footer; };
#include <poppack.h>

static inline bool IsPngSignature(const void* buffer)
{
    const BYTE* p = (BYTE*)buffer;
    return p[0] == 0x89 && p[1] == 'P' && p[2] == 'N' && p[3] == 'G' &&
           p[4] == 0x0D && p[5] == 0x0A && p[6] == 0x1A && p[7] == 0x0A;
}

static inline bool IsPngSignature(const void* buffer, SIZE_T size)
{
    return size >= sizeof(PNGSIGNATURE) && IsPngSignature(buffer);
}

static BYTE GetPngBppFromIHDRData(const void* buffer)
{
    static const BYTE channels[] = { 1, 0, 3, 1, 2, 0, 4 };
    const BYTE* p = (BYTE*)buffer, depth = p[8], type = p[8 + 1];
    return (depth <= 16 && type <= 6) ? channels[type] * depth : 0;
}

static bool GetInfoFromPng(const void* file, SIZE_T size, IMAGESTATS& info)
{
    C_ASSERT(sizeof(PNGSIGNATURE) == 8);
    C_ASSERT(sizeof(PNGSIGANDIHDR) == 8 + (4 + 4 + (4 + 4 + 5) + 4));

    if (size > sizeof(PNGSIGANDIHDR) + sizeof(PNGFOOTER) && IsPngSignature(file))
    {
        const UINT PNGIHDRSIG = 0x52444849; // Note: Big endian
        const UINT* chunkhdr = (UINT*)((char*)file + sizeof(PNGSIGNATURE));
        if (BigToHost32(chunkhdr[0]) >= sizeof(PNGIHDR) && chunkhdr[1] == PNGIHDRSIG)
        {
            info.w = BigToHost32(chunkhdr[2]);
            info.h = BigToHost32(chunkhdr[3]);
            info.bpp = GetPngBppFromIHDRData(&chunkhdr[2]);
            return info.bpp != 0;
        }
    }
    return false;
}

static bool GetInfoFromBmp(const void* pBitmapInfo, IMAGESTATS& info)
{
    BitmapInfoHeader bih(pBitmapInfo);
    info.w = bih.biWidth;
    info.h = abs((int)bih.biHeight);
    UINT bpp = bih.biBitCount * bih.biPlanes;
    info.bpp = LOBYTE(bpp);
    return info.w && bpp == info.bpp;
}

static bool GetInfoFromIcoBmp(const void* pBitmapInfo, IMAGESTATS& info)
{
    bool ret = GetInfoFromBmp(pBitmapInfo, info);
    info.h /= 2; // Don't include mask
    return ret && info.h;
}

EXTERN_C PCWSTR GetExtraExtensionsGdipList(VOID)
{
    return L"*.CUR"; // "*.FOO;*.BAR" etc.
}

static void OverrideFileContent(HGLOBAL& hMem, DWORD& Size)
{
    PBYTE buffer = (PBYTE)GlobalLock(hMem);
    if (!buffer)
        return;

    // TODO: We could try to load an ICO/PNG/BMP resource from a PE file here into buffer

    // ICO/CUR
    struct ICOHDR { WORD Sig, Type, Count; };
    ICOHDR* pIcoHdr = (ICOHDR*)buffer;
    if (Size > sizeof(ICOHDR) && !pIcoHdr->Sig && pIcoHdr->Type > 0 && pIcoHdr->Type < 3 && pIcoHdr->Count)
    {
        const UINT minbmp = sizeof(BITMAPCOREHEADER) + 1, minpng = sizeof(PNGSIGANDIHDR);
        const UINT minfile = min(minbmp, minpng), count = pIcoHdr->Count;
        struct ICOENTRY { BYTE w, h, pal, null; WORD planes, bpp; UINT size, offset; };
        ICOENTRY* entries = (ICOENTRY*)&pIcoHdr[1];
        if (Size - sizeof(ICOHDR) > (sizeof(ICOENTRY) + minfile) * count)
        {
            UINT64 best = 0;
            int bestindex = -1;
            // Inspect all the images and find the "best" image
            for (UINT i = 0; i < count; ++i)
            {
                BOOL valid = FALSE;
                IMAGESTATS info;
                const BYTE* data = buffer + entries[i].offset;
                if (IsPngSignature(data, entries[i].size))
                    valid = GetInfoFromPng(data, entries[i].size, info);
                else
                    valid = GetInfoFromIcoBmp(data, info);

                if (valid)
                {
                    // Note: This treats bpp as more important compared to LookupIconIdFromDirectoryEx
                    UINT64 score = UINT64(info.w) * info.h * info.bpp;
                    if (score > best)
                    {
                        best = score;
                        bestindex = i;
                    }
                }
            }
            if (bestindex >= 0)
            {
                if (pIcoHdr->Type == 2)
                {
                    // GDI+ does not support .cur files, convert to .ico
                    pIcoHdr->Type = 1;
#if 0               // Because we are already overriding the order, we don't need to correct the ICOENTRY lookup info
                    for (UINT i = 0; i < count; ++i)
                    {
                        BitmapInfoHeader bih;
                        const BYTE* data = buffer + entries[i].offset;
                        if (IsPngSignature(data, entries[i].size))
                        {
                            IMAGESTATS info;
                            if (!GetInfoFromPng(data, entries[i].size, info))
                                continue;
                            bih.biPlanes = 1;
                            bih.biBitCount = info.bpp;
                            entries[i].pal = 0;
                        }
                        else
                        {
                            bih.Initialize(data);
                            entries[i].pal = bih.biPlanes * bih.biBitCount <= 8 ? bih.biClrUsed : 0;
                        }
                        entries[i].planes = (WORD)bih.biPlanes;
                        entries[i].bpp = (WORD)bih.biBitCount;
                    }
#endif
                }
#if 0
                // Convert to a .ico with a single image
                pIcoHdr->Count = 1;
                const BYTE* data = buffer + entries[bestindex].offset;
                entries[0] = entries[bestindex];
                entries[0].offset = (UINT)UINT_PTR((PBYTE)&entries[1] - buffer);
                MoveMemory(buffer + entries[0].offset, data, entries[0].size);
                Size = entries[0].offset + entries[0].size;
#else
                // Place the best image first, GDI+ will return the first image
                ICOENTRY temp = entries[0];
                entries[0] = entries[bestindex];
                entries[bestindex] = temp;
#endif
            }
        }
    }

    GlobalUnlock(hMem);
}

static HRESULT LoadImageFromStream(IStream* pStream, GpImage** ppImage)
{
    Status status = DllExports::GdipLoadImageFromStream(pStream, ppImage);
    return HResultFromGdiplus(status);
}

static HRESULT LoadImageFromFileHandle(HANDLE hFile, GpImage** ppImage)
{
    DWORD size = GetFileSize(hFile, NULL);
    if (!size || size == INVALID_FILE_SIZE)
        return HResultFromWin32(ERROR_NOT_SUPPORTED);

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size);
    if (!hMem)
        return HResultFromWin32(ERROR_OUTOFMEMORY);
    HRESULT hr = E_FAIL;
    void* buffer = GlobalLock(hMem);
    if (buffer)
    {
        hr = Read(hFile, buffer, size);
        GlobalUnlock(hMem);
        if (SUCCEEDED(hr))
        {
            OverrideFileContent(hMem, size);
            IStream* pStream;
            if (SUCCEEDED(hr = CreateStreamOnHGlobal(hMem, TRUE, &pStream)))
            {
                // CreateStreamOnHGlobal does not know the real size, we do
                pStream->SetSize(MakeULargeInteger(size));
                hr = LoadImageFromStream(pStream, ppImage);
                pStream->Release(); // Calls GlobalFree
                return hr;
            }
        }
    }
    GlobalFree(hMem);
    return hr;
}

EXTERN_C HRESULT LoadImageFromPath(LPCWSTR Path, GpImage** ppImage)
{
    // NOTE: GdipLoadImageFromFile locks the file.
    //       Avoid file locking by using GdipLoadImageFromStream and memory stream.

    HANDLE hFile = CreateFileW(Path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE,
                               NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        HRESULT hr = LoadImageFromFileHandle(hFile, ppImage);
        CloseHandle(hFile);
        return hr;
    }
    return HResultFromWin32(GetLastError());
}
