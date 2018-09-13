//==========================================================================================
// HISTDATA helper (should be a static member)
//==========================================================================================


#define CUrlHistoryProp IntsiteProp

//==========================================================================================
// IntsiteProp class implementation
//==========================================================================================

#ifdef DEBUG

/*----------------------------------------------------------
Purpose: Dump the properties in this object

*/
STDMETHODIMP_(void)
    CUrlHistoryProp::Dump(void)
{
    if (IsFlagSet(g_dwDumpFlags, DF_URLPROP))
    {
        TraceMsg(TF_ALWAYS, "  Intsite Property obj: %s", m_szURL);
        URLProp::Dump();
    }
}

#endif


/*----------------------------------------------------------
Purpose: Constructor for URLProp 

*/
CUrlHistoryProp::CUrlHistoryProp(void)
{
    // Don't validate this until after construction.
    
    // This object should only be allocated, not used on the stack,
    // because we don't zero-initialize the member variables.
    // Here's a sanity assertion.
    
    ASSERT(NULL == m_pintshcut);
    
    ASSERT(IS_VALID_STRUCT_PTR(this, CIntsiteProp));
    
    return;
}


/*----------------------------------------------------------
Purpose: Destructor for CUrlHistoryProp

*/
CUrlHistoryProp::~CUrlHistoryProp(void)
{
    if (m_pintshcut)
    {
        if (!m_fPrivate)
            m_pintshcut->Release();
        m_pintshcut = NULL;
    }
    
    return;
}

STDAPI CIntsiteProp_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppvOut)
{
    HRESULT hres;
    
    *ppvOut = NULL;
    
    ASSERT(punkOuter==NULL)

    IUnknown * piunk = (IUnknown *)new IntsiteProp;
    if ( !piunk ) 
    {
        hres = E_OUTOFMEMORY;
    }
    else
    {
        hres = piunk->QueryInterface(riid, ppvOut);
        piunk->Release();
    }
    
    return hres;        // S_OK or E_NOINTERFACE
}

HRESULT CUrlHistoryProp::Init(void)
{
    return URLProp::Init();
}

STDMETHODIMP CUrlHistoryProp::InitFromDB(LPCTSTR pszURL, Intshcut *pintshcut, BOOL fPrivObj)
{
    // TraceMsg(DM_HISTPROP, "CUHP::InitFromDB called for %s", pszURL);

    // Initialize the in-memory property storage from the file
    // and database
    
    HRESULT hres = Init();
    if (SUCCEEDED(hres))
    {
        if (NULL == m_pintshcut)
        {
            m_fPrivate = fPrivObj;
            if (!m_fPrivate)
                pintshcut->AddRef();
            m_pintshcut = pintshcut;
        }
        else
        {
            // We can't switch from Private->Public or visaversa.
            ASSERT(fPrivObj == m_fPrivate);
        }
        
        if (pszURL)
        {
            // Is this really a URL?? 
            PARSEDURL pu;
            
            pu.cbSize = SIZEOF(pu);
            hres = ParseURL(pszURL, &pu);
            
            if (S_OK == hres)
            {
                // Yes; go ahead and initialize
                StrCpyN(m_szURL, pszURL, SIZECHARS(m_szURL));

                hres = LoadFromDB(pszURL);
            }
        }
    }
    
    return hres;
}

HRESULT CUrlHistoryProp::LoadFromDB(
    IN LPCTSTR pszURL)
{
    TraceMsg(DM_HISTPROP, "CUHP::LoadFromDB called for %s", pszURL);

    CEI_PREALLOC buf;
    CUrlHistory::s_ConvertToPrefixedUrlW(pszURL, buf.szPrefixedUrl, ARRAYSIZE(buf.szPrefixedUrl), &buf.pszFragment);
    CUrlHistory::s_RetrievePrefixedUrlInfo(buf.szPrefixedUrl, &buf);

    //
    //	if there is already an entry for this Url, then we will reuse some of the
    //	settings.  retrieve the relevant info if possible.
    //
    if (buf.pcei) {
        CHistoryData* phdPrev =  CHistoryData::s_GetHistoryData(buf.pcei);
        if (phdPrev) {
            //
            // Initialize non-string properties first
            //
            const static PROPSPEC c_aprspec[] = {
                { PRSPEC_PROPID, PID_INTSITE_FLAGS },
                { PRSPEC_PROPID, PID_INTSITE_LASTVISIT },
                { PRSPEC_PROPID, PID_INTSITE_LASTMOD },
                { PRSPEC_PROPID, PID_INTSITE_WATCH },
            };
            PROPVARIANT apropvar[ARRAYSIZE(c_aprspec)] = { 0 };

            apropvar[0].vt = VT_UI4;
            apropvar[0].lVal = phdPrev->dwFlags;
            apropvar[1].vt = VT_FILETIME;
            apropvar[1].filetime = buf.pcei->LastAccessTime;
            apropvar[2].vt = VT_FILETIME;
            apropvar[2].filetime = buf.pcei->LastModifiedTime;
            apropvar[3].vt = VT_UI4;
            apropvar[3].lVal = phdPrev->dwWatch;

            TraceMsg(DM_HISTPROP, "CUHP::InitFromDB calling WriteMultiple (wFlags=%x)", phdPrev->dwFlags);
      	    WriteMultiple(ARRAYSIZE(c_aprspec), c_aprspec, apropvar, 0);
            PropStg_DirtyMultiple(m_hstg, ARRAYSIZE(c_aprspec), c_aprspec, FALSE);

            //
            // Then, initialize others
            //
            PROPSPEC prspec;
            prspec.ulKind = PRSPEC_PROPID;

            for (const HISTEXTRA* phextPrev = phdPrev->_GetExtra();
                 !phextPrev->IsTerminator();
                 phextPrev = phextPrev->GetNextFast())
            {
                TraceMsg(DM_HISTPROP, "CUHP::InitFromDB found HISTEXTRA (id=%d, vt=%d)",
                         phextPrev->idExtra, phextPrev->cbExtra);

                WCHAR wszBuf[MAX_URL_STRING]; // NOTES: scope must be right
                apropvar[0].vt = phextPrev->vtExtra;

                switch(phextPrev->vtExtra) {
                case VT_LPWSTR:
                    apropvar[0].pwszVal = (LPWSTR)phextPrev->abExtra;
                    break;

                case VT_LPSTR:
                    //
                    // Notice that we always convert it to LPWSTR
                    //
                    {
                        apropvar[0].pwszVal = wszBuf;
                        LPCSTR pszExtra = (LPCSTR)phextPrev->abExtra;
                        AnsiToUnicode(pszExtra, wszBuf, ARRAYSIZE(wszBuf));
                        apropvar[0].vt = VT_LPWSTR;
                    }
                    break;

                case VT_UI4:
                case VT_I4:
                    apropvar[0].lVal = *(DWORD*)phextPrev->abExtra;
                    break;

                case VT_NULL:
                    ASSERT(phextPrev->idExtra == PID_INTSITE_FRAGMENT);
                    continue;

                default:
                    ASSERT(0);
                    continue;
                }

                prspec.propid = phextPrev->idExtra;
                WriteMultiple(1, &prspec, apropvar, 0);
                PropStg_DirtyMultiple(m_hstg, 1, &prspec, FALSE);
            }
        } else {
            TraceMsg(DM_HISTPROP, "CUHP::LoadFromDB can't get phdPrev");
        }
    } else {
        TraceMsg(DM_HISTPROP, "CUHP::LoadFromDB can't get pcei");
    }

    return S_OK;
}


struct URLHIST_ENUMPARAM {
    CUrlHistoryProp* that;
    INTERNET_CACHE_ENTRY_INFO cei;
    LPINTERNET_CACHE_ENTRY_INFO pceiPrev;
    CHistoryData* phdPrev;
    LPHISTEXTRA phextCur;
    UINT cbHistExtra;
    BOOL fDirty;
};

STDAPI s_CommitHistItem(
    IN PROPID        propid,
    IN PROPVARIANT * ppropvar,
    IN LPARAM        lParam)
{
    URLHIST_ENUMPARAM* peparam = (URLHIST_ENUMPARAM*)lParam;
    CHistoryData* phdNew = (CHistoryData*)peparam->cei.lpHeaderInfo;

    TraceMsg(DM_HISTEXTRA, "CUHP::s_CommitHistItem called for id=%d vt=%d (phextCur=%x)",
                propid, ppropvar->vt, peparam->phextCur);

    UINT cbExtra = 0;
    UINT cbRequired;

    switch(propid) {
    case PID_INTSITE_FLAGS:
        if (ppropvar->vt == VT_UI4 && phdNew) {
            TraceMsg(DM_HISTPROP, "CUHP::s_CommitHistItem updating PID_INSITE_FLAGS (%x to %x)",
                     phdNew->dwFlags, ppropvar->lVal);
            phdNew->dwFlags = ppropvar->lVal;
            peparam->fDirty = TRUE;
        }
        break;

    case PID_INTSITE_WATCH:
        if (ppropvar->vt == VT_UI4 && phdNew) {
            TraceMsg(DM_HISTPROP, "CUHP::s_CommitHistItem updating PID_INSITE_WATCH (%x to %x)",
                     phdNew->dwFlags, ppropvar->lVal);
            phdNew->dwWatch = ppropvar->lVal;
            peparam->fDirty = TRUE;
        }
        break;

    case PID_INTSITE_LASTVISIT:
    case PID_INTSITE_LASTMOD:
        // They are read-only. We can change it if we want.
        ASSERT(0);
        break;

    default:
        switch(ppropvar->vt) {
        case VT_UI4:
        case VT_I4:
            cbExtra = DW_ALIGNED(SIZEOF(HISTEXTRA)-SIZEOF(peparam->phextCur->abExtra)+SIZEOF(UINT));
            if (peparam->phextCur) {
                peparam->phextCur->cbExtra = cbExtra;
                peparam->phextCur->idExtra = propid;
                peparam->phextCur->vtExtra = ppropvar->vt;
                *(DWORD*)peparam->phextCur->abExtra = ppropvar->lVal;
                peparam->fDirty = TRUE;
            }
            break;

        case VT_LPWSTR:
            cbRequired = WideCharToMultiByte(CP_ACP, 0, ppropvar->pwszVal, -1,
                                NULL, 0, NULL, NULL);
            cbExtra = DW_ALIGNED(SIZEOF(HISTEXTRA) + cbRequired);
            if (peparam->phextCur)
            {
                peparam->phextCur->cbExtra = cbExtra;
                peparam->phextCur->idExtra = propid;
                peparam->phextCur->vtExtra = VT_LPSTR;
                WideCharToMultiByte(CP_ACP, 0, ppropvar->pwszVal, -1, 
                    (LPSTR)peparam->phextCur->abExtra, cbRequired, NULL, NULL);
                peparam->fDirty = TRUE;
            }
            break;

        case VT_EMPTY:
            if (peparam->phextCur) {
                peparam->fDirty = TRUE;
            }
            break;

        default:
            ASSERT(0);
            break;
        }

        if (peparam->phextCur) {
            // We are saving the data, move the write pointer.
            TraceMsg(DM_HISTEXTRA, "s_CommitHistItem moving phextCur forward %d bytes",
                     peparam->phextCur->cbExtra);
            peparam->phextCur = peparam->phextCur->GetNextFastForSave();
            ASSERT(peparam->phextCur->cbExtra == 0);
        } else {
            // We are calcurating the required size, just add the size.
            TraceMsg(DM_HISTEXTRA, "s_CommitHistItem adding %d", cbExtra);
            peparam->cbHistExtra += cbExtra;

            // Remove existing one. 
            if (peparam->phdPrev) {
                // BUGBUG: Bad const to non-const cast
                HISTEXTRA* phextPrev =
                    (HISTEXTRA*)peparam->phdPrev->_FindExtra(propid);
    
                if (phextPrev) {
                    TraceMsg(DM_HISTEXTRA, "s_CommitHistItem invalidate an old one id=%d %d bytes",
                              phextPrev->idExtra, phextPrev->cbExtra);
                    phextPrev->vtExtra = VT_EMPTY;
                }
            }
	}
    }
    return S_OK;
};

HRESULT CUrlHistoryProp::Commit(IN DWORD dwFlags)
{
    TraceMsg(DM_HISTPROP, "CUHP::Commit called for %s", m_szURL);

    CEI_PREALLOC buf;
    CUrlHistory::s_ConvertToPrefixedUrlW(m_szURL, buf.szPrefixedUrl, ARRAYSIZE(buf.szPrefixedUrl), &buf.pszFragment);
    CUrlHistory::s_RetrievePrefixedUrlInfo(buf.szPrefixedUrl, &buf);

    HRESULT hres;
    URLHIST_ENUMPARAM eparam = { this };
    ASSERT(eparam.fDirty == FALSE);

    eparam.pceiPrev = buf.pcei;
    if (eparam.pceiPrev) {
        eparam.cei = *eparam.pceiPrev;
        eparam.phdPrev =  CHistoryData::s_GetHistoryData(eparam.pceiPrev);
    }

    // First, enemerate once to get the size for extra. 
    eparam.cei.lpHeaderInfo = NULL;
    eparam.cbHistExtra = 0;
    hres = PropStg_Enum(m_hstg, PSTGEF_DIRTY, s_CommitHistItem, (LPARAM)&eparam);

    DWORD dwFlagsPrev = 0;
    if (eparam.phdPrev) {
        eparam.cbHistExtra += eparam.phdPrev->GetTotalExtraSize();
        dwFlagsPrev = eparam.phdPrev->dwFlags;
    }

    TraceMsg(DM_HISTEXTRA, "CUHP::Commit total size is %d", eparam.cbHistExtra);

    CHistoryData* phdNew = CHistoryData::s_AllocateHeaderInfo(
                                eparam.cbHistExtra, eparam.phdPrev,
                                &eparam.cei.dwHeaderInfoSize);

    if (phdNew) {
        eparam.cei.lpHeaderInfo = (LPTSTR)phdNew;
        eparam.phextCur = phdNew->_GetExtra();

        // Enumerate again to fill the extra data. 
        hres = PropStg_Enum(m_hstg, PSTGEF_DIRTY, s_CommitHistItem, (LPARAM)&eparam);

        if (eparam.fDirty)
        {
            if (eparam.phdPrev) {
                eparam.phdPrev->CopyExtra(eparam.phextCur);
            }

            TraceMsg(DM_HISTPROP, "CUHP::Commit It's dirty. save it (header = %d bytes)",
                     eparam.cei.dwHeaderInfoSize);

            ASSERT(eparam.cbHistExtra == phdNew->GetTotalExtraSize());

            CUrlHistory::s_CommitUrlCacheEntry(buf.szPrefixedUrl, &eparam.cei);

            if ((dwFlagsPrev & PIDISF_RECENTLYCHANGED)
                    != (phdNew->dwFlags & PIDISF_RECENTLYCHANGED))
            {
                // Yes; update the images
                CUrlHistory::s_UpdateIcon(m_pintshcut, dwFlagsPrev);
            }
        }
        LocalFree(phdNew);
    }

    return hres;
}


