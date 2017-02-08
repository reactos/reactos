/*
 * PROJECT:     ReactOS Applications
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/msconfig_new/fileutils.h
 * PURPOSE:     File Utility Functions
 * COPYRIGHT:   Copyright 2011-2012 Hermes BELUSCA - MAITO <hermes.belusca@sfr.fr>
 */

#ifndef __FILEUTILS_H__
#define __FILEUTILS_H__

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

//
// NOTE: A function called "FileExists" with the very same prototype
// already exists in the PSDK headers (in setupapi.h)
//
BOOL
MyFileExists(IN  LPCWSTR           lpszFilePath,
             OUT PWIN32_FIND_DATAW pFindData OPTIONAL);

////////////////////////////////////////////////////////////////////////////////
typedef LRESULT
(*PQUERY_FILES_TABLE_ROUTINE)(IN LPCWSTR Path,
                              IN LPCWSTR FileNamesQuery,
                              IN LPCWSTR ExpandedFileNamesQuery,
                              IN PWIN32_FIND_DATAW pfind_data,
                              IN PVOID   Context,
                              IN PVOID   EntryContext);

#define QUERY_FILES_TABLE_ROUTINE(fnName)               \
    LRESULT (fnName)(IN LPCWSTR Path,                   \
                     IN LPCWSTR FileNamesQuery,         \
                     IN LPCWSTR ExpandedFileNamesQuery, \
                     IN PWIN32_FIND_DATAW pfind_data,   \
                     IN PVOID   Context,                \
                     IN PVOID   EntryContext)

typedef struct __tagQUERY_FILES_TABLE
{
    PQUERY_FILES_TABLE_ROUTINE QueryRoutine;
    PVOID EntryContext;
    // Other fields ?
} QUERY_FILES_TABLE, *PQUERY_FILES_TABLE;

LRESULT
FileQueryFiles(IN LPCWSTR Path,
               IN LPCWSTR FileNamesQuery,
               IN PQUERY_FILES_TABLE QueryTable,
               IN PVOID   Context);

////////////////////////////////////////////////////////////////////////////////

BOOL BackupIniFile(IN LPCWSTR lpszIniFile);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __FILEUTILS_H__
