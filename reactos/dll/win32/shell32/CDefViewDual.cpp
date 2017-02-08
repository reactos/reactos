/*
 *    ShellFolderViewDual
 *
 *    Copyright 2016         Mark Jansen
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
 *
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

class CDefViewDual :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IDispatchImpl<IShellFolderViewDual2, &IID_IShellFolderViewDual2>
{
    public:
        CDefViewDual()
        {
        }

        ~CDefViewDual()
        {
        }

        HRESULT STDMETHODCALLTYPE Initialize()
        {
            // Nothing to do for now..
            return S_OK;
        }

        // *** IShellFolderViewDual methods ***
        virtual HRESULT STDMETHODCALLTYPE get_Application(IDispatch **app) override
        {
            if (!app) return E_INVALIDARG;

            return CShellDispatch_Constructor(IID_IDispatch, (LPVOID*)app);
        }

        virtual HRESULT STDMETHODCALLTYPE get_Parent(IDispatch **parent) override
        {
            if (!parent) return E_INVALIDARG;
            *parent = NULL;
            FIXME("CDefViewDual::get_Parent is UNIMPLEMENTED (%p, %p)\n", this, parent);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE get_Folder(Folder **folder) override
        {
            if (!folder) return E_INVALIDARG;
            *folder = NULL;
            FIXME("CDefViewDual::get_Folder is UNIMPLEMENTED (%p, %p)\n", this, folder);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE SelectedItems(FolderItems **items) override
        {
            if (!items) return E_INVALIDARG;
            *items = NULL;
            FIXME("CDefViewDual::SelectedItems is UNIMPLEMENTED (%p, %p)\n", this, items);
            return E_NOTIMPL;
        }

        virtual  HRESULT STDMETHODCALLTYPE get_FocusedItem(FolderItem **item) override
        {
            if (!item) return E_INVALIDARG;
            *item = NULL;
            FIXME("CDefViewDual::get_FocusedItem is UNIMPLEMENTED (%p, %p)\n", this, item);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE SelectItem(VARIANT *item, int flags) override
        {
            FIXME("CDefViewDual::SelectItem is UNIMPLEMENTED (%p, %p, %i)\n", this, item, flags);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE PopupItemMenu(FolderItem *item, VARIANT vx, VARIANT vy, BSTR *command) override
        {
            FIXME("CDefViewDual::PopupItemMenu is UNIMPLEMENTED (%p, %p, %s, %s, %p)\n", this, item, wine_dbgstr_variant(&vx), wine_dbgstr_variant(&vy), command);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE get_Script(IDispatch **script) override
        {
            FIXME("CDefViewDual::get_Script is UNIMPLEMENTED (%p, %p)\n", this, script);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE get_ViewOptions(long *options) override
        {
            FIXME("CDefViewDual::get_ViewOptions is UNIMPLEMENTED (%p, %p)\n", this, options);
            return E_NOTIMPL;
        }

        // *** IShellFolderViewDual2 methods ***
        virtual HRESULT STDMETHODCALLTYPE get_CurrentViewMode(UINT *mode) override
        {
            FIXME("CDefViewDual::get_CurrentViewMode is UNIMPLEMENTED (%p, %p)\n", this, mode);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE put_CurrentViewMode(UINT mode) override
        {
            FIXME("CDefViewDual::put_CurrentViewMode is UNIMPLEMENTED (%p, %u)\n", this, mode);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE SelectItemRelative(int relative) override
        {
            FIXME("CDefViewDual::SelectItemRelative is UNIMPLEMENTED (%p, %i)\n", this, relative);
            return E_NOTIMPL;
        }

    BEGIN_COM_MAP(CDefViewDual)
        COM_INTERFACE_ENTRY_IID(IID_IDispatch, IDispatch)
        COM_INTERFACE_ENTRY_IID(IID_IShellFolderViewDual, IShellFolderViewDual)
        COM_INTERFACE_ENTRY_IID(IID_IShellFolderViewDual2, IShellFolderViewDual2)
    END_COM_MAP()
};

/**********************************************************
 *    CDefViewDual_Constructor
 */

HRESULT WINAPI CDefViewDual_Constructor(REFIID riid, LPVOID * ppvOut)
{
    return ShellObjectCreatorInit<CDefViewDual>(riid, ppvOut);
}
