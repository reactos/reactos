// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/*++



Abstract:

    Value resources.

--*/

#include "precomp.hpp"

MtDefine(TMilSlaveValue, Mem, "TMilSlaveValue");

//
// Explicit instantiations
//

template class TMilSlaveValue<double,               MILCMD_DOUBLERESOURCE,          TYPE_DOUBLERESOURCE>;
template class TMilSlaveValue<MilColorF,           MILCMD_COLORRESOURCE,           TYPE_COLORRESOURCE>;
template class TMilSlaveValue<MilPoint2D,         MILCMD_POINTRESOURCE,           TYPE_POINTRESOURCE>;
template class TMilSlaveValue<MilPointAndSizeD,            MILCMD_RECTRESOURCE,            TYPE_RECTRESOURCE>;
template class TMilSlaveValue<MilSizeD,            MILCMD_SIZERESOURCE,            TYPE_SIZERESOURCE>;
template class TMilSlaveValue<MilMatrix3x2D,       MILCMD_MATRIXRESOURCE,          TYPE_MATRIXRESOURCE>;
template class TMilSlaveValue<MilPoint3F,         MILCMD_POINT3DRESOURCE,         TYPE_POINT3DRESOURCE>;
template class TMilSlaveValue<MilPoint3F,         MILCMD_VECTOR3DRESOURCE,        TYPE_VECTOR3DRESOURCE>;


