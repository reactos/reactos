// Structs related to .cab extraction
// FIXME: they should belong to exports of cabinet.dll
#pragma once

struct ERF
{
    INT erfOper;
    INT erfType;
    BOOL fError;
};

struct FILELIST
{
    LPSTR FileName;
    FILELIST *next;
    BOOL DoExtract;
};

struct SESSION
{
    INT FileSize;
    ERF Error;
    FILELIST *FileList;
    INT FileCount;
    INT Operation;
    CHAR Destination[MAX_PATH];
    CHAR CurrentFile[MAX_PATH];
    CHAR Reserved[MAX_PATH];
    FILELIST *FilterList;
};

typedef HRESULT(WINAPI *fnExtract)(SESSION *dest, LPCSTR szCabName);
