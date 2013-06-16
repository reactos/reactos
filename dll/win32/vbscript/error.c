/*
 * Copyright 2011 Jacek Caban for CodeWeavers
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

#include "vbscript.h"
#include "vbscript_defs.h"

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(vbscript);

static HRESULT Err_Description(vbdisp_t *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Err_HelpContext(vbdisp_t *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Err_HelpFile(vbdisp_t *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Err_Number(vbdisp_t *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Err_Source(vbdisp_t *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Err_Clear(vbdisp_t *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Err_Raise(vbdisp_t *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static const builtin_prop_t err_props[] = {
    {DISPID_ERR_DESCRIPTION,  Err_Description, BP_GETPUT},
    {DISPID_ERR_HELPCONTEXT,  Err_HelpContext, BP_GETPUT},
    {DISPID_ERR_HELPFILE,     Err_HelpFile, BP_GETPUT},
    {DISPID_ERR_NUMBER,       Err_Number, BP_GETPUT},
    {DISPID_ERR_SOURCE,       Err_Source, BP_GETPUT},
    {DISPID_ERR_CLEAR,        Err_Clear},
    {DISPID_ERR_RAISE,        Err_Raise, 0, 5},
};

HRESULT init_err(script_ctx_t *ctx)
{
    HRESULT hres;

    ctx->err_desc.ctx = ctx;
    ctx->err_desc.builtin_prop_cnt = sizeof(err_props)/sizeof(*err_props);
    ctx->err_desc.builtin_props = err_props;

    hres = get_typeinfo(ErrObj_tid, &ctx->err_desc.typeinfo);
    if(FAILED(hres))
        return hres;

    return create_vbdisp(&ctx->err_desc, &ctx->err_obj);
}
