/*
 * PROJECT:     xml2sdb
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Define mapping of all shim database types to xml
 * COPYRIGHT:   Copyright 2016-2025 Mark Jansen <mark.jansen@reactos.org>
 */

#pragma once

#include <string>
#include <list>
#include <vector>
#include <map>

#include <typedefs.h>
#include <guiddef.h>
#include <sdbtypes.h>
#include "sdbwrite.h"
#include <sdbtagid.h>

namespace tinyxml2
{
class XMLHandle;
}
using tinyxml2::XMLHandle;

typedef std::basic_string<WCHAR> sdbstring;

struct Database;

enum PlatformType
{
    PLATFORM_NONE = 0,

    // This is all we support for now
    PLATFORM_X86 = 1,
    // There is another platform here, but we don't support it yet
    // https://www.geoffchappell.com/studies/windows/km/ntoskrnl/api/kshim/drvmain.htm
    PLATFORM_AMD64 = 4,

    PLATFORM_ANY = PLATFORM_X86 | PLATFORM_AMD64
};

struct str_to_flag
{
    const char *name;
    DWORD flag;
};

extern str_to_flag platform_to_flag[];

DWORD str_to_enum(const std::string &str, const str_to_flag *table);

struct InExclude
{
    bool fromXml(XMLHandle dbNode);
    bool toSdb(Database& db);

    std::string Module;
    bool Include = false;
};

struct ShimRef
{
    bool fromXml(XMLHandle dbNode);
    bool toSdb(Database& db);

    std::string Name;
    std::string CommandLine;
    TAGID ShimTagid = 0;
    std::list<InExclude> InExcludes;
};

struct FlagRef
{
    bool fromXml(XMLHandle dbNode);
    bool toSdb(Database& db);

    std::string Name;
    TAGID FlagTagid = 0;
};

struct Shim
{
    bool fromXml(XMLHandle dbNode);
    bool toSdb(Database& db);

    std::string Name;
    std::string DllFile;
    GUID FixID = {};
    TAGID Tagid = 0;
    std::list<InExclude> InExcludes;
    PlatformType Platform = PLATFORM_ANY;
};

struct Flag
{
    bool fromXml(XMLHandle dbNode);
    bool toSdb(Database& db);

    std::string Name;
    TAGID Tagid = 0;
    QWORD KernelFlags = 0;
    QWORD UserFlags = 0;
    QWORD ProcessParamFlags = 0;
};


struct Data
{
    bool fromXml(XMLHandle dbNode);
    bool toSdb(Database& db);

    std::string Name;
    TAGID Tagid = 0;
    DWORD DataType = 0;

    std::string StringData;
    DWORD DWordData = 0;
    QWORD QWordData = 0;
};

struct Layer
{
    bool fromXml(XMLHandle dbNode);
    bool toSdb(Database& db);

    std::string Name;
    TAGID Tagid = 0;
    std::list<ShimRef> ShimRefs;
    std::list<FlagRef> FlagRefs;
    std::list<Data> Datas;
    PlatformType Platform = PLATFORM_ANY;
};

struct MatchingFile
{
    bool fromXml(XMLHandle dbNode);
    bool toSdb(Database& db);

    std::string Name;
    DWORD Size = 0;
    DWORD Checksum = 0;
    std::string CompanyName;
    std::string InternalName;
    std::string ProductName;
    std::string ProductVersion;
    std::string FileVersion;
    std::string BinFileVersion;
    DWORD LinkDate = 0;
    std::string VerLanguage;
    std::string FileDescription;
    std::string OriginalFilename;
    std::string UptoBinFileVersion;
    DWORD LinkerVersion = 0;
};

struct Exe
{
    bool fromXml(XMLHandle dbNode);
    bool toSdb(Database& db);

    std::string Name;
    GUID ExeID = {};
    std::string AppName;
    std::string Vendor;
    TAGID Tagid = 0;
    std::list<MatchingFile> MatchingFiles;
    std::list<ShimRef> ShimRefs;
    std::list<FlagRef> FlagRefs;
    PlatformType Platform = PLATFORM_ANY;
};

struct Library
{
    std::list<InExclude> InExcludes;
    std::list<Shim> Shims;
    std::list<Flag> Flags;
};

struct Database
{
    bool fromXml(const char* fileName, PlatformType platform);
    bool fromXml(XMLHandle dbNode);
    bool toSdb(LPCWSTR path);

    void WriteString(TAG tag, const sdbstring& str, bool always = false);
    void WriteString(TAG tag, const std::string& str, bool always = false);
    void WriteBinary(TAG tag, const GUID& guid, bool always = false);
    void WriteBinary(TAG tag, const std::vector<BYTE>& data, bool always = false);
    void WriteDWord(TAG tag, DWORD value, bool always = false);
    void WriteQWord(TAG tag, QWORD value, bool always = false);
    void WriteNull(TAG tag);
    TAGID BeginWriteListTag(TAG tag);
    BOOL EndWriteListTag(TAGID tagid);

    void InsertShimTagid(const sdbstring& name, TAGID tagid);
    inline void InsertShimTagid(const std::string& name, TAGID tagid)
    {
        InsertShimTagid(sdbstring(name.begin(), name.end()), tagid);
    }
    TAGID FindShimTagid(const sdbstring& name);
    inline TAGID FindShimTagid(const std::string& name)
    {
        return FindShimTagid(sdbstring(name.begin(), name.end()));
    }


    void InsertPatchTagid(const sdbstring& name, TAGID tagid);
    inline void InsertPatchTagid(const std::string& name, TAGID tagid)
    {
        InsertPatchTagid(sdbstring(name.begin(), name.end()), tagid);
    }
    TAGID FindPatchTagid(const sdbstring& name);
    inline TAGID FindPatchTagid(const std::string& name)
    {
        return FindPatchTagid(sdbstring(name.begin(), name.end()));
    }

    void InsertFlagTagid(const sdbstring& name, TAGID tagid);
    inline void InsertFlagTagid(const std::string& name, TAGID tagid)
    {
        InsertFlagTagid(sdbstring(name.begin(), name.end()), tagid);
    }
    TAGID FindFlagTagid(const sdbstring& name);
    inline TAGID FindFlagTagid(const std::string& name)
    {
        return FindFlagTagid(sdbstring(name.begin(), name.end()));
    }

    std::string Name;
    GUID ID = {};

    struct Library Library;
    std::list<Layer> Layers;
    std::list<Exe> Exes;

private:
    std::map<sdbstring, TAGID> KnownShims;
    std::map<sdbstring, TAGID> KnownPatches;
    std::map<sdbstring, TAGID> KnownFlags;
    PDB pdb = nullptr;
    PlatformType platform = PLATFORM_ANY;
};

