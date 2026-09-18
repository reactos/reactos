// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_text
//      $Keywords:
//
//  $Description:
//      Contains lockable D3D texture class declaration and the stack based lock
//      helper class
//
//      Contains also CD3DLockableTexturePair class declaration and stack based
//      lock helper class for it.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CD3DLockableTexture);
MtExtern(D3DResource_LockableTexture);

class CD3DTexture;

class CD3DLockableTexture : public CD3DTexture
{
protected:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CD3DLockableTexture));

public:
    DEFINE_RESOURCE_REF_COUNT_BASE

    static HRESULT Create(
        __inout_ecount(1) CD3DResourceManager *pResourceManager,
        __in_ecount(1) IDirect3DTexture9 *pD3DTexture,
        __deref_out_ecount(1) CD3DLockableTexture **ppD3DLockableTexture
        );

    // Only call this method if you are certain that we have a system memory
    // texture.
    HRESULT LockRect(
        __out_ecount(1) D3DLOCKED_RECT* pLockedRect, 
        __in_ecount(1) CONST RECT* pRect,
        DWORD dwFlags
        );

    HRESULT UnlockRect();

    HRESULT AddDirtyRect(
        __in_ecount(1) const RECT &rc
        );

protected:
    CD3DLockableTexture();
    ~CD3DLockableTexture();

    HRESULT Init(
        __inout_ecount(1) CD3DResourceManager *pResourceManager,
        __in_ecount(1) IDirect3DTexture9 *pD3DTexture
        );

#if PERFMETER
    virtual PERFMETERTAG GetPerfMeterTag() const
    {
        return Mt(D3DResource_LockableTexture);
    }
#endif
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CD3DLockableTexturePair
//
//  Synopsis:
//      Holds one or two lockable textures. Normally, only the main one is used;
//      the second is involved only for text rendering in clear type mode. 
//      Clear type requires 6 components per texel: three colors and three
//      alphas (separate alpha for each color component). As far as DX allows
//      only four components, we store alphas separately in m_pAuxTexture, while
//      m_pMainTexture stores colors.
//
//------------------------------------------------------------------------------
class CD3DLockableTexturePair
{
public:
    CD3DLockableTexturePair();
    ~CD3DLockableTexturePair();

    VOID InitMain(
        __in_ecount(1) CD3DLockableTexture *pTexture
        );

    VOID InitAux(
        __in_ecount(1) CD3DLockableTexture *pTexture
        );

    HRESULT Draw(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice,
        __in_ecount(1) const MilPointAndSizeL &rc,
        bool fUseAux
        );

private:
    friend class CD3DLockableTexturePairLock;
    CD3DLockableTexture *m_pTextureMain;
    CD3DLockableTexture *m_pTextureAux;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CD3DLockableTexturePairLock
//
//  Synopsis:
//      The helper class for CD3DLockableTexturePair locking/unlocking. This is
//      short-term life time object supposed to live in stack frame.
//
//------------------------------------------------------------------------------
class CD3DLockableTexturePairLock
{
public:
    CD3DLockableTexturePairLock(
        __in_ecount(1) const CD3DLockableTexturePair* pTexturePair
        )
        : m_texturePair(*pTexturePair)
    {
        m_fMainLocked = false;
        m_fAuxLocked = false;
    }

    struct LockData
    {
        BYTE *pMainBits;
        BYTE *pAuxBits;
        INT uPitch;

    #if DBG_ANALYSIS
        UINT m_uDbgAnalysisLockedWidth;
        UINT m_uDbgAnalysisLockedHeight;
    #endif
    };

    HRESULT Lock(
        UINT uWidth,
        UINT uHeight,
        __out_ecount(1) LockData &lockData,
        bool fUseAux = false
        );

    ~CD3DLockableTexturePairLock()
    {
        if (m_fMainLocked)
        {
            IGNORE_HR(m_texturePair.m_pTextureMain->UnlockRect());
        }
        if (m_fAuxLocked)
        {
            IGNORE_HR(m_texturePair.m_pTextureAux->UnlockRect());
        }
    }

private:
    static HRESULT LockOne(
        __in_ecount(1) CD3DLockableTexture* pTexture,
        UINT uWidth,
        UINT uHeight,
        __out_ecount(1) D3DLOCKED_RECT* pD3DLockedRect
        );

private:
    CD3DLockableTexturePair const& m_texturePair;
    bool m_fMainLocked;
    bool m_fAuxLocked;
};


