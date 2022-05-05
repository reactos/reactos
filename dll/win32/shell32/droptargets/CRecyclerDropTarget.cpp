/*
 * Trash virtual folder support. The trashing engine is implemented in trash.c
 *
 * Copyright (C) 2006 Mikolaj Zalewski
 * Copyright (C) 2009 Andrew Hill
 * Copyright (C) 2017 Giannis Adamopoulos
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <precomp.h>

WINE_DEFAULT_DEBUG_CHANNEL (shell);

class CRecyclerDropTarget :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IDropTarget
{
    private:
        BOOL fAcceptFmt;       /* flag for pending Drop */
        UINT cfShellIDList;

        static HRESULT _DoDeleteDataObject(IDataObject *pda, DWORD fMask)
        {
            HRESULT hr;
            STGMEDIUM medium;
            FORMATETC formatetc;
            InitFormatEtc (formatetc, CF_HDROP, TYMED_HGLOBAL);
            hr = pda->GetData(&formatetc, &medium);
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;

            LPDROPFILES lpdf = (LPDROPFILES) GlobalLock(medium.hGlobal);
            if (!lpdf)
            {
                ERR("Error locking global\n");
                return E_FAIL;
            }

            /* Delete them */
            SHFILEOPSTRUCTW FileOp;
            ZeroMemory(&FileOp, sizeof(FileOp));
            FileOp.wFunc = FO_DELETE;
            FileOp.pFrom = (LPWSTR) (((byte*) lpdf) + lpdf->pFiles);;
            if ((fMask & CMIC_MASK_SHIFT_DOWN) == 0)
                FileOp.fFlags = FOF_ALLOWUNDO;
            TRACE("Deleting file (just the first) = %s, allowundo: %d\n", debugstr_w(FileOp.pFrom), (FileOp.fFlags == FOF_ALLOWUNDO));

            int res = SHFileOperationW(&FileOp);
            if (res)
            {
                ERR("SHFileOperation failed with 0x%x\n", res);
                hr = E_FAIL;
            }

            ReleaseStgMedium(&medium);

            return hr;
        }

        struct DeleteThreadData {
            IStream *s;
            DWORD fMask;
        };

        static DWORD WINAPI _DoDeleteThreadProc(LPVOID lpParameter)
        {
            DeleteThreadData *data = static_cast<DeleteThreadData*>(lpParameter);
            CoInitialize(NULL);
            IDataObject *pDataObject;
            HRESULT hr = CoGetInterfaceAndReleaseStream (data->s, IID_PPV_ARG(IDataObject, &pDataObject));
            if (SUCCEEDED(hr))
            {
                _DoDeleteDataObject(pDataObject, data->fMask);
            }
            pDataObject->Release();
            CoUninitialize();
            HeapFree(GetProcessHeap(), 0, data);
            return 0;
        }

        static void _DoDeleteAsync(IDataObject *pda, DWORD fMask)
        {
            DeleteThreadData *data = static_cast<DeleteThreadData*>(HeapAlloc(GetProcessHeap(), 0, sizeof(DeleteThreadData)));
            data->fMask = fMask;
            CoMarshalInterThreadInterfaceInStream(IID_IDataObject, pda, &data->s);
            SHCreateThread(_DoDeleteThreadProc, data, NULL, NULL);
        }

    public:

        CRecyclerDropTarget()
        {
            fAcceptFmt = FALSE;
            cfShellIDList = RegisterClipboardFormatW(CFSTR_SHELLIDLIST);
        }

        HRESULT WINAPI DragEnter(IDataObject *pDataObject,
                                            DWORD dwKeyState, POINTL pt, DWORD *pdwEffect)
        {
            TRACE("Recycle bin drag over (%p)\n", this);
            /* The recycle bin accepts pretty much everything, and sets a CSIDL flag. */
            fAcceptFmt = TRUE;

            *pdwEffect = DROPEFFECT_MOVE;
            return S_OK;
        }

        HRESULT WINAPI DragOver(DWORD dwKeyState, POINTL pt, DWORD *pdwEffect)
        {
            TRACE("(%p)\n", this);

            if (!pdwEffect)
                return E_INVALIDARG;

            *pdwEffect = DROPEFFECT_MOVE;

            return S_OK;
        }

        HRESULT WINAPI DragLeave()
        {
            TRACE("(%p)\n", this);

            fAcceptFmt = FALSE;

            return S_OK;
        }

        HRESULT WINAPI Drop(IDataObject *pDataObject,
                            DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
        {
            TRACE("(%p) object dropped on recycle bin, effect %u\n", this, *pdwEffect);

            FORMATETC fmt;
            TRACE("(%p)->(DataObject=%p)\n", this, pDataObject);
            InitFormatEtc (fmt, cfShellIDList, TYMED_HGLOBAL);

            /* Handle cfShellIDList Drop objects here, otherwise send the appropriate message to other software */
            if (SUCCEEDED(pDataObject->QueryGetData(&fmt)))
            {
                DWORD fMask = 0;

                if ((grfKeyState & MK_SHIFT) == MK_SHIFT)
                    fMask |= CMIC_MASK_SHIFT_DOWN;

                _DoDeleteAsync(pDataObject, fMask);
            }
            else
            {
                /*
                 * TODO call SetData on the data object with format CFSTR_TARGETCLSID
                 * set to the Recycle Bin's class identifier CLSID_RecycleBin.
                 */
            }
            return S_OK;
        }

        DECLARE_NOT_AGGREGATABLE(CRecyclerDropTarget)

        DECLARE_PROTECT_FINAL_CONSTRUCT()

        BEGIN_COM_MAP(CRecyclerDropTarget)
        COM_INTERFACE_ENTRY_IID(IID_IDropTarget, IDropTarget)
        END_COM_MAP()
};

HRESULT CRecyclerDropTarget_CreateInstance(REFIID riid, LPVOID * ppvOut)
{
    return ShellObjectCreator<CRecyclerDropTarget>(riid, ppvOut);
}
