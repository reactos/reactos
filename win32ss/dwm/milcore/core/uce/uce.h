// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/*++



Module Name:

    The uce directory contains all rendering/composition device code. This
    file contains all the includes containing exported functionality for the
    rest of the engine, but none of the imported functionality from
    elsewhere - see precomp.hpp for those.

Environment:

    User mode only.

--*/

#define ENABLE_PARTITION_MANAGER_LOG 1
#include <wmistr.h>
#include <evntrace.h>

#include <avrt.h>

#include "handletable.h"
#include "cmdbatch.h"
#include "global.h"
#include "clientchanneltables.h"
#include "serverchanneltables.h"
#include "connectioncontext.h"
#include "connection.h"
#include "htmaster.h"
#include "clientchannel.h"
#include "serverchannel.h"

#include "resslave.h"
#include "resources\valueres.h"
#include "idrawingcontext.h"
#include "schedulemanager.h"

#include "htslave.h"
#include "generated_resource_factory.h"

#include "partition.h"
#include "partitionmanager.h"
#include "partitionthread.h"
#include "composition.h"
#include "crossthreadcomposition.h"
#include "samethreadcomposition.h"

#include "glyphcacheslave.h"

//
// Rendering layer.
//

#include "clipstack.h"

//
// Compositing layer.
//

#include "UceTypes.h"
#include "resources\brushcontext.h"

#include "graphwalker.h"

#include "RenderDataBounder.h"
#include "AlphamaskWrapper.h"

#include "dirtyregion.h"
#include "precompctx.h"

#include "ZOrderedRect.h"

#include "prerender3dcontext.h"
#include "render3dcontext.h"
#include "drawingcontext.h"

#include "rendertarget.h"
#include "hwndtarget.h"
#include "printtarget.h"
#include "rendertargetmanager.h"

#include "rsapi.h"

#include "VisualCache.h"
#include "VisualCacheSet.h"
#include "VisualCacheManager.h"


