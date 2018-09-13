//+----------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation
//
//  File:       dfsext.h
//
//  Contents:   Code to see if a path refers to a Dfs path.
//
//  Classes:    None
//
//  Functions:  IsThisADfsPath
//
//  History:    March 11, 1996  Milans created
//
//-----------------------------------------------------------------------------

#ifndef _DFS_EXT_
#define _DFS_EXT_

BOOL
IsThisADfsPath(
    IN LPCWSTR pwszPath,
    IN DWORD cwPath OPTIONAL);

#endif // _DFS_EXT_


