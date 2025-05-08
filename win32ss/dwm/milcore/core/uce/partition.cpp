// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  Abstract:
//      Definitions for the partition and partition state.
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

//+-----------------------------------------------------------------------------
//
//    Member:
//        Partition::SetStateFlags
// 
//    Synopsis:
//        Ensures that the specified set of flags is set for this partition.
// 
//------------------------------------------------------------------------------

void 
Partition::SetStateFlags(PartitionState flags)
{
    m_state = static_cast<PartitionState>(m_state | flags);

#if ENABLE_PARTITION_MANAGER_LOG
    CPartitionManager::LogEvent(PartitionManagerEvent::SetFlags, static_cast<DWORD>(flags));
#endif /* ENABLE_PARTITION_MANAGER_LOG */
}


//+-----------------------------------------------------------------------------
//
//    Member:
//        Partition::ClearStateFlags
// 
//    Synopsis:
//        Ensures that the specified set of flags is not set for this partition.
// 
//------------------------------------------------------------------------------

void 
Partition::ClearStateFlags(PartitionState flags)
{
    m_state = static_cast<PartitionState>(m_state & ~flags);

#if ENABLE_PARTITION_MANAGER_LOG
    CPartitionManager::LogEvent(PartitionManagerEvent::ClearedFlags, static_cast<DWORD>(flags));
#endif /* ENABLE_PARTITION_MANAGER_LOG */
}



