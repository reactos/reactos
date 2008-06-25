/*
 * IWineD3DQuery implementation
 *
 * Copyright 2005 Oliver Stieber
 *
 *
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


#include "config.h"
#include "wined3d_private.h"

/*
 * http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/programmingguide/advancedtopics/Queries.asp
 *
 * Occlusion Queries:
 * http://www.gris.uni-tuebingen.de/~bartz/Publications/paper/hww98.pdf
 * http://oss.sgi.com/projects/ogl-sample/registry/ARB/occlusion_query.txt
 */

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
#define GLINFO_LOCATION This->wineD3DDevice->adapter->gl_info

/* *******************************************
   IWineD3DQuery IUnknown parts follow
   ******************************************* */
static HRESULT  WINAPI IWineD3DQueryImpl_QueryInterface(IWineD3DQuery *iface, REFIID riid, LPVOID *ppobj)
{
    IWineD3DQueryImpl *This = (IWineD3DQueryImpl *)iface;
    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppobj);
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IWineD3DBase)
        || IsEqualGUID(riid, &IID_IWineD3DQuery)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG  WINAPI IWineD3DQueryImpl_AddRef(IWineD3DQuery *iface) {
    IWineD3DQueryImpl *This = (IWineD3DQueryImpl *)iface;
    TRACE("(%p) : AddRef increasing from %d\n", This, This->ref);
    return InterlockedIncrement(&This->ref);
}

static ULONG  WINAPI IWineD3DQueryImpl_Release(IWineD3DQuery *iface) {
    IWineD3DQueryImpl *This = (IWineD3DQueryImpl *)iface;
    ULONG ref;
    TRACE("(%p) : Releasing from %d\n", This, This->ref);
    ref = InterlockedDecrement(&This->ref);
    if (ref == 0) {
        if(This->type == WINED3DQUERYTYPE_EVENT) {
            if(GL_SUPPORT(APPLE_FENCE)) {
                GL_EXTCALL(glDeleteFencesAPPLE(1, &((WineQueryEventData *)(This->extendedData))->fenceId));
                checkGLcall("glDeleteFencesAPPLE");
            } else if(GL_SUPPORT(NV_FENCE)) {
                GL_EXTCALL(glDeleteFencesNV(1, &((WineQueryEventData *)(This->extendedData))->fenceId));
                checkGLcall("glDeleteFencesNV");
            }
        } else if(This->type == WINED3DQUERYTYPE_OCCLUSION && GL_SUPPORT(ARB_OCCLUSION_QUERY)) {
            GL_EXTCALL(glDeleteQueriesARB(1, &((WineQueryOcclusionData *)(This->extendedData))->queryId));
            checkGLcall("glDeleteQueriesARB");
        }

        HeapFree(GetProcessHeap(), 0, This->extendedData);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* *******************************************
   IWineD3DQuery IWineD3DQuery parts follow
   ******************************************* */
static HRESULT  WINAPI IWineD3DQueryImpl_GetParent(IWineD3DQuery *iface, IUnknown** parent){
    IWineD3DQueryImpl *This = (IWineD3DQueryImpl *)iface;

    *parent= (IUnknown*) parent;
    IUnknown_AddRef(*parent);
    TRACE("(%p) : returning %p\n", This, *parent);
    return WINED3D_OK;
}

static HRESULT  WINAPI IWineD3DQueryImpl_GetDevice(IWineD3DQuery* iface, IWineD3DDevice **pDevice){
    IWineD3DQueryImpl *This = (IWineD3DQueryImpl *)iface;
    IWineD3DDevice_AddRef((IWineD3DDevice *)This->wineD3DDevice);
    *pDevice = (IWineD3DDevice *)This->wineD3DDevice;
    TRACE("(%p) returning %p\n", This, *pDevice);
    return WINED3D_OK;
}


static HRESULT  WINAPI IWineD3DQueryImpl_GetData(IWineD3DQuery* iface, void* pData, DWORD dwSize, DWORD dwGetDataFlags){
    IWineD3DQueryImpl *This = (IWineD3DQueryImpl *)iface;
    HRESULT res = S_OK;

    TRACE("(%p) : type %#x, pData %p, dwSize %#x, dwGetDataFlags %#x\n", This, This->type, pData, dwSize, dwGetDataFlags);

    if(dwSize == 0){
        /*you can use this method to poll the resource for the query status*/
        /*We return success(S_OK) if we support a feature, and faikure(S_FALSE) if we don't, just return success and fluff it for now*/
        return S_OK;
    }else{
    }

    switch (This->type){

    case WINED3DQUERYTYPE_VCACHE:
    {

        WINED3DDEVINFO_VCACHE *data = (WINED3DDEVINFO_VCACHE *)pData;
        FIXME("(%p): Unimplemented query WINED3DQUERYTYPE_VCACHE\n", This);
        data->Pattern     = WINEMAKEFOURCC('C','A','C','H');
        data->OptMethod   = 0; /*0 get longest strips, 1 optimize vertex cache*/
        data->CacheSize   = 0; /*cache size, only required if OptMethod == 1*/
        data->MagicNumber = 0; /*only required if OptMethod == 1 (used internally)*/

    }
    break;
    case WINED3DQUERYTYPE_RESOURCEMANAGER:
    {
        WINED3DDEVINFO_RESOURCEMANAGER *data = (WINED3DDEVINFO_RESOURCEMANAGER *)pData;
        int i;
        FIXME("(%p): Unimplemented query WINED3DQUERYTYPE_RESOURCEMANAGER\n", This);
        for(i = 0; i < WINED3DRTYPECOUNT; i++){
            /*I'm setting the default values to 1 so as to reduce the risk of a div/0 in the caller*/
            /*  isTextureResident could be used to get some of this infomration  */
            data->stats[i].bThrashing            = FALSE;
            data->stats[i].ApproxBytesDownloaded = 1;
            data->stats[i].NumEvicts             = 1;
            data->stats[i].NumVidCreates         = 1;
            data->stats[i].LastPri               = 1;
            data->stats[i].NumUsed               = 1;
            data->stats[i].NumUsedInVidMem       = 1;
            data->stats[i].WorkingSet            = 1;
            data->stats[i].WorkingSetBytes       = 1;
            data->stats[i].TotalManaged          = 1;
            data->stats[i].TotalBytes            = 1;
        }

    }
    break;
    case WINED3DQUERYTYPE_VERTEXSTATS:
    {
        WINED3DDEVINFO_VERTEXSTATS *data = (WINED3DDEVINFO_VERTEXSTATS *)pData;
        FIXME("(%p): Unimplemented query WINED3DQUERYTYPE_VERTEXSTATS\n", This);
        data->NumRenderedTriangles      = 1;
        data->NumExtraClippingTriangles = 1;

    }
    break;
    case WINED3DQUERYTYPE_EVENT:
    {
        BOOL* data = pData;
        WineD3DContext *ctx = ((WineQueryEventData *)This->extendedData)->ctx;
        if(ctx != This->wineD3DDevice->activeContext || ctx->tid != GetCurrentThreadId()) {
            /* See comment in IWineD3DQuery::Issue, event query codeblock */
            WARN("Query context not active, reporting GPU idle\n");
            *data = TRUE;
        } else if(GL_SUPPORT(APPLE_FENCE)) {
            *data = GL_EXTCALL(glTestFenceAPPLE(((WineQueryEventData *)This->extendedData)->fenceId));
            checkGLcall("glTestFenceAPPLE");
        } else if(GL_SUPPORT(NV_FENCE)) {
            *data = GL_EXTCALL(glTestFenceNV(((WineQueryEventData *)This->extendedData)->fenceId));
            checkGLcall("glTestFenceNV");
        } else {
            WARN("(%p): reporting GPU idle\n", This);
            *data = TRUE;
        }
    }
    break;
    case WINED3DQUERYTYPE_OCCLUSION:
    {
        DWORD* data = pData;
        if (GL_SUPPORT(ARB_OCCLUSION_QUERY) &&
            ((WineQueryOcclusionData *)This->extendedData)->ctx == This->wineD3DDevice->activeContext &&
            This->wineD3DDevice->activeContext->tid == GetCurrentThreadId()) {
            GLuint available;
            GLuint samples;
            GLuint queryId = ((WineQueryOcclusionData *)This->extendedData)->queryId;

            GL_EXTCALL(glGetQueryObjectuivARB(queryId, GL_QUERY_RESULT_AVAILABLE_ARB, &available));
            checkGLcall("glGetQueryObjectuivARB(GL_QUERY_RESULT_AVAILABLE)\n");
            TRACE("(%p) : available %d.\n", This, available);

            if (available) {
                GL_EXTCALL(glGetQueryObjectuivARB(queryId, GL_QUERY_RESULT_ARB, &samples));
                checkGLcall("glGetQueryObjectuivARB(GL_QUERY_RESULT)\n");
                TRACE("(%p) : Returning %d samples.\n", This, samples);
                *data = samples;
                res = S_OK;
            } else {
                res = S_FALSE;
            }
        } else {
            WARN("(%p) : Occlusion queries not supported, or wrong context. Returning 1.\n", This);
            *data = 1;
            res = S_OK;
        }
    }
    break;
    case WINED3DQUERYTYPE_TIMESTAMP:
    {
        UINT64* data = pData;
        FIXME("(%p): Unimplemented query WINED3DQUERYTYPE_TIMESTAMP\n", This);
        *data = 1; /*Don't know what this is supposed to be*/
    }
    break;
    case WINED3DQUERYTYPE_TIMESTAMPDISJOINT:
    {
        BOOL* data = pData;
        FIXME("(%p): Unimplemented query WINED3DQUERYTYPE_TIMESTAMPDISJOINT\n", This);
        *data = FALSE; /*Don't know what this is supposed to be*/
    }
    break;
    case WINED3DQUERYTYPE_TIMESTAMPFREQ:
    {
        UINT64* data = pData;
        FIXME("(%p): Unimplemented query WINED3DQUERYTYPE_TIMESTAMPFREQ\n", This);
        *data = 1; /*Don't know what this is supposed to be*/
    }
    break;
    case WINED3DQUERYTYPE_PIPELINETIMINGS:
    {
        WINED3DDEVINFO_PIPELINETIMINGS *data = (WINED3DDEVINFO_PIPELINETIMINGS *)pData;
        FIXME("(%p): Unimplemented query WINED3DQUERYTYPE_PIPELINETIMINGS\n", This);

        data->VertexProcessingTimePercent    =   1.0f;
        data->PixelProcessingTimePercent     =   1.0f;
        data->OtherGPUProcessingTimePercent  =  97.0f;
        data->GPUIdleTimePercent             =   1.0f;
    }
    break;
    case WINED3DQUERYTYPE_INTERFACETIMINGS:
    {
        WINED3DDEVINFO_INTERFACETIMINGS *data = (WINED3DDEVINFO_INTERFACETIMINGS *)pData;
        FIXME("(%p): Unimplemented query WINED3DQUERYTYPE_INTERFACETIMINGS\n", This);

        data->WaitingForGPUToUseApplicationResourceTimePercent =   1.0f;
        data->WaitingForGPUToAcceptMoreCommandsTimePercent     =   1.0f;
        data->WaitingForGPUToStayWithinLatencyTimePercent      =   1.0f;
        data->WaitingForGPUExclusiveResourceTimePercent        =   1.0f;
        data->WaitingForGPUOtherTimePercent                    =  96.0f;
    }

    break;
    case WINED3DQUERYTYPE_VERTEXTIMINGS:
    {
        WINED3DDEVINFO_STAGETIMINGS *data = (WINED3DDEVINFO_STAGETIMINGS *)pData;
        FIXME("(%p): Unimplemented query WINED3DQUERYTYPE_VERTEXTIMINGS\n", This);

        data->MemoryProcessingPercent      = 50.0f;
        data->ComputationProcessingPercent = 50.0f;

    }
    break;
    case WINED3DQUERYTYPE_PIXELTIMINGS:
    {
        WINED3DDEVINFO_STAGETIMINGS *data = (WINED3DDEVINFO_STAGETIMINGS *)pData;
        FIXME("(%p): Unimplemented query WINED3DQUERYTYPE_PIXELTIMINGS\n", This);

        data->MemoryProcessingPercent      = 50.0f;
        data->ComputationProcessingPercent = 50.0f;
    }
    break;
    case WINED3DQUERYTYPE_BANDWIDTHTIMINGS:
    {
        WINED3DDEVINFO_BANDWIDTHTIMINGS *data = (WINED3DDEVINFO_BANDWIDTHTIMINGS *)pData;
        FIXME("(%p): Unimplemented query WINED3DQUERYTYPE_BANDWIDTHTIMINGS\n", This);

        data->MaxBandwidthUtilized                =  1.0f;
        data->FrontEndUploadMemoryUtilizedPercent =  1.0f;
        data->VertexRateUtilizedPercent           =  1.0f;
        data->TriangleSetupRateUtilizedPercent    =  1.0f;
        data->FillRateUtilizedPercent             = 97.0f;
    }
    break;
    case WINED3DQUERYTYPE_CACHEUTILIZATION:
    {
        WINED3DDEVINFO_CACHEUTILIZATION *data = (WINED3DDEVINFO_CACHEUTILIZATION *)pData;
        FIXME("(%p): Unimplemented query WINED3DQUERYTYPE_CACHEUTILIZATION\n", This);

        data->TextureCacheHitRate             = 1.0f;
        data->PostTransformVertexCacheHitRate = 1.0f;
    }


    break;
    default:
        FIXME("(%p) Unhandled query type %d\n",This , This->type);

    };

    /*dwGetDataFlags = 0 || D3DGETDATA_FLUSH
    D3DGETDATA_FLUSH may return WINED3DERR_DEVICELOST if the device is lost
    */
    return res; /* S_OK if the query data is available*/
}


static DWORD  WINAPI IWineD3DQueryImpl_GetDataSize(IWineD3DQuery* iface){
    IWineD3DQueryImpl *This = (IWineD3DQueryImpl *)iface;
    int dataSize = 0;
    TRACE("(%p) : type %#x\n", This, This->type);
    switch(This->type){
    case WINED3DQUERYTYPE_VCACHE:
        dataSize = sizeof(WINED3DDEVINFO_VCACHE);
        break;
    case WINED3DQUERYTYPE_RESOURCEMANAGER:
        dataSize = sizeof(WINED3DDEVINFO_RESOURCEMANAGER);
        break;
    case WINED3DQUERYTYPE_VERTEXSTATS:
        dataSize = sizeof(WINED3DDEVINFO_VERTEXSTATS);
        break;
    case WINED3DQUERYTYPE_EVENT:
        dataSize = sizeof(BOOL);
        break;
    case WINED3DQUERYTYPE_OCCLUSION:
        dataSize = sizeof(DWORD);
        break;
    case WINED3DQUERYTYPE_TIMESTAMP:
        dataSize = sizeof(UINT64);
        break;
    case WINED3DQUERYTYPE_TIMESTAMPDISJOINT:
        dataSize = sizeof(BOOL);
        break;
    case WINED3DQUERYTYPE_TIMESTAMPFREQ:
        dataSize = sizeof(UINT64);
        break;
    case WINED3DQUERYTYPE_PIPELINETIMINGS:
        dataSize = sizeof(WINED3DDEVINFO_PIPELINETIMINGS);
        break;
    case WINED3DQUERYTYPE_INTERFACETIMINGS:
        dataSize = sizeof(WINED3DDEVINFO_INTERFACETIMINGS);
        break;
    case WINED3DQUERYTYPE_VERTEXTIMINGS:
        dataSize = sizeof(WINED3DDEVINFO_STAGETIMINGS);
        break;
    case WINED3DQUERYTYPE_PIXELTIMINGS:
        dataSize = sizeof(WINED3DDEVINFO_STAGETIMINGS);
        break;
    case WINED3DQUERYTYPE_BANDWIDTHTIMINGS:
        dataSize = sizeof(WINED3DQUERYTYPE_BANDWIDTHTIMINGS);
        break;
    case WINED3DQUERYTYPE_CACHEUTILIZATION:
        dataSize = sizeof(WINED3DDEVINFO_CACHEUTILIZATION);
        break;
    default:
       FIXME("(%p) Unhandled query type %d\n",This , This->type);
       dataSize = 0;
    }
    return dataSize;
}


static WINED3DQUERYTYPE  WINAPI IWineD3DQueryImpl_GetType(IWineD3DQuery* iface){
    IWineD3DQueryImpl *This = (IWineD3DQueryImpl *)iface;
    return This->type;
}


static HRESULT  WINAPI IWineD3DQueryImpl_Issue(IWineD3DQuery* iface,  DWORD dwIssueFlags){
    IWineD3DQueryImpl *This = (IWineD3DQueryImpl *)iface;

    TRACE("(%p) : dwIssueFlags %#x, type %#x\n", This, dwIssueFlags, This->type);

    switch (This->type) {
        case WINED3DQUERYTYPE_OCCLUSION:
            if (GL_SUPPORT(ARB_OCCLUSION_QUERY)) {
                WineD3DContext *ctx = ((WineQueryOcclusionData *)This->extendedData)->ctx;

                if(ctx != This->wineD3DDevice->activeContext || ctx->tid != GetCurrentThreadId()) {
                    WARN("Not the owning context, can't start query\n");
                } else {
                    if (dwIssueFlags & WINED3DISSUE_BEGIN) {
                        GL_EXTCALL(glBeginQueryARB(GL_SAMPLES_PASSED_ARB, ((WineQueryOcclusionData *)This->extendedData)->queryId));
                        checkGLcall("glBeginQuery()");
                    }
                    if (dwIssueFlags & WINED3DISSUE_END) {
                        GL_EXTCALL(glEndQueryARB(GL_SAMPLES_PASSED_ARB));
                        checkGLcall("glEndQuery()");
                    }
                }
            } else {
                FIXME("(%p) : Occlusion queries not supported\n", This);
            }
            break;

        case WINED3DQUERYTYPE_EVENT: {
            if (dwIssueFlags & WINED3DISSUE_END) {
                WineD3DContext *ctx = ((WineQueryEventData *)This->extendedData)->ctx;
                if(ctx != This->wineD3DDevice->activeContext || ctx->tid != GetCurrentThreadId()) {
                    /* GL fences can be used only from the context that created them,
                     * so if a different context is active, don't bother setting the query. The penalty
                     * of a context switch is most likely higher than the gain of a correct query result
                     *
                     * If the query is used from a different thread, don't bother creating a multithread
                     * context - there's no point in doing that as the query would be unusable anyway
                     */
                    WARN("Query context not active\n");
                } else if(GL_SUPPORT(APPLE_FENCE)) {
                    GL_EXTCALL(glSetFenceAPPLE(((WineQueryEventData *)This->extendedData)->fenceId));
                    checkGLcall("glSetFenceAPPLE");
                } else if (GL_SUPPORT(NV_FENCE)) {
                    GL_EXTCALL(glSetFenceNV(((WineQueryEventData *)This->extendedData)->fenceId, GL_ALL_COMPLETED_NV));
                    checkGLcall("glSetFenceNV");
                }
            } else if(dwIssueFlags & WINED3DISSUE_BEGIN) {
                /* Started implicitly at device creation */
                ERR("Event query issued with START flag - what to do?\n");
            }
        }

        default:
            /* The fixme is printed when the app asks for the resulting data */
            WARN("(%p) : Unhandled query type %#x\n", This, This->type);
            break;
    }

    return WINED3D_OK; /* can be WINED3DERR_INVALIDCALL.    */
}


/**********************************************************
 * IWineD3DQuery VTbl follows
 **********************************************************/

const IWineD3DQueryVtbl IWineD3DQuery_Vtbl =
{
    /*** IUnknown methods ***/
    IWineD3DQueryImpl_QueryInterface,
    IWineD3DQueryImpl_AddRef,
    IWineD3DQueryImpl_Release,
     /*** IWineD3Dquery methods ***/
    IWineD3DQueryImpl_GetParent,
    IWineD3DQueryImpl_GetDevice,
    IWineD3DQueryImpl_GetData,
    IWineD3DQueryImpl_GetDataSize,
    IWineD3DQueryImpl_GetType,
    IWineD3DQueryImpl_Issue
};
