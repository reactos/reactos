#ifndef __ATLMEM_H__
#define __ATLMEM_H__

#pragma once
#include <atlcore.h>

namespace ATL
{

__interface __declspec(uuid("654F7EF5-CFDF-4df9-A450-6C6A13C622C0")) IAtlMemMgr
{
public:
    _Ret_maybenull_ _Post_writable_byte_size_(SizeBytes) void* Allocate(
        _In_ size_t SizeBytes
        );

    void Free(
        _Inout_opt_ void* Buffer
        );

    _Ret_maybenull_ _Post_writable_byte_size_(SizeBytes) void* Reallocate(
        _Inout_updates_bytes_opt_(SizeBytes) void* Buffer,
        _In_ size_t SizeBytes
        );

    size_t GetSize(
        _In_ void* Buffer
        );
};

class CWin32Heap : public IAtlMemMgr
{
public:
    HANDLE m_hHeap;

public:
    CWin32Heap() :
        m_hHeap(NULL)
    {
    }

    CWin32Heap(_In_ HANDLE hHeap) :
        m_hHeap(hHeap)
    {
        ATLASSERT(hHeap != NULL);
    }

    virtual ~CWin32Heap()
    {
    }


    // IAtlMemMgr
    _Ret_maybenull_ _Post_writable_byte_size_(SizeBytes) virtual void* Allocate(
        _In_ size_t SizeBytes
        )
    {
        return ::HeapAlloc(m_hHeap, 0, SizeBytes);
    }

    virtual void Free(
        _In_opt_ void* Buffer
        )
    {
        if (Buffer)
        {
            BOOL FreeOk;
            FreeOk = ::HeapFree(m_hHeap, 0, Buffer);
            ATLASSERT(FreeOk == TRUE);
        }
    }

    _Ret_maybenull_ _Post_writable_byte_size_(SizeBytes) virtual void* Reallocate(
        _In_opt_ void* Buffer,
        _In_ size_t SizeBytes
        )
    {
        if (SizeBytes == 0)
        {
            Free(Buffer);
            return NULL;
        }

        if (Buffer == NULL)
        {
            return Allocate(SizeBytes);
        }

        return ::HeapReAlloc(m_hHeap, 0, Buffer, SizeBytes);
    }

    virtual size_t GetSize(
        _Inout_ void* Buffer
        )
    {
        return ::HeapSize(m_hHeap, 0, Buffer);
    }
};

}

#endif
