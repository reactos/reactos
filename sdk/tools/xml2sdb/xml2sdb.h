/*
 * PROJECT:     xml2sdb
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Define mapping of all shim database types to xml
 * COPYRIGHT:   Copyright 2016-2019 Mark Jansen (mark.jansen@reactos.org)
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

struct InExclude
{
    InExclude() : Include(false) { ; }
    bool fromXml(XMLHandle dbNode);
    bool toSdb(PDB pdb, Database& db);

    std::string Module;
    bool Include;
};

struct ShimRef
{
    ShimRef() : ShimTagid(0) { ; }

    bool fromXml(XMLHandle dbNode);
    bool toSdb(PDB pdb, Database& db);

    std::string Name;
    std::string CommandLine;
    TAGID ShimTagid;
    std::list<InExclude> InExcludes;
};

struct FlagRef
{
    FlagRef() : FlagTagid(0) { ; }

    bool fromXml(XMLHandle dbNode);
    bool toSdb(PDB pdb, Database& db);

    std::string Name;
    TAGID FlagTagid;
};

struct Shim
{
    Shim() : Tagid(0) { ; }

    bool fromXml(XMLHandle dbNode);
    bool toSdb(PDB pdb, Database& db);

    std::string Name;
    std::string DllFile;
    GUID FixID;
    TAGID Tagid;
    std::list<InExclude> InExcludes;
};

struct Flag
{
    Flag() : Tagid(0), KernelFlags(0), UserFlags(0), ProcessParamFlags(0) { ; }

    bool fromXml(XMLHandle dbNode);
    bool toSdb(PDB pdb, Database& db);

    std::string Name;
    TAGID Tagid;
    QWORD KernelFlags;
    QWORD UserFlags;
    QWORD ProcessParamFlags;
};


struct Data
{
    Data() : Tagid(0), DataType(0), DWordData(0), QWordData(0) { ; }

    bool fromXml(XMLHandle dbNode);
    bool toSdb(PDB pdb, Database& db);

    std::string Name;
    TAGID Tagid;
    DWORD DataType;

    std::string StringData;
    DWORD DWordData;
    QWORD QWordData;
};

struct Layer
{
    Layer() : Tagid(0) { ; }

    bool fromXml(XMLHandle dbNode);
    bool toSdb(PDB pdb, Database& db);

    std::string Name;
    TAGID Tagid;
    std::list<ShimRef> ShimRefs;
    std::list<FlagRef> FlagRefs;
    std::list<Data> Datas;
};

struct MatchingFile
{
    MatchingFile() : Size(0), Checksum(0), LinkDate(0), LinkerVersion(0) {;}

    bool fromXml(XMLHandle dbNode);
    bool toSdb(PDB pdb, Database& db);

    std::string Name;
    DWORD Size;
    DWORD Checksum;
    std::string CompanyName;
    std::string InternalName;
    std::string ProductName;
    std::string ProductVersion;
    std::string FileVersion;
    std::string BinFileVersion;
    DWORD LinkDate;
    std::string VerLanguage;
    std::string FileDescription;
    std::string OriginalFilename;
    std::string UptoBinFileVersion;
    DWORD LinkerVersion;
};

struct Exe
{
    Exe() : Tagid(0) { ; }

    bool fromXml(XMLHandle dbNode);
    bool toSdb(PDB pdb, Database& db);

    std::string Name;
    GUID ExeID;
    std::string AppName;
    std::string Vendor;
    TAGID Tagid;
    std::list<MatchingFile> MatchingFiles;
    std::list<ShimRef> ShimRefs;
    std::list<FlagRef> FlagRefs;
};

struct Library
{
    std::list<InExclude> InExcludes;
    std::list<Shim> Shims;
    std::list<Flag> Flags;
};

struct Database
{

    bool fromXml(const char* fileName);
    bool fromXml(XMLHandle dbNode);
    bool toSdb(LPCWSTR path);

    void WriteString(PDB pdb, TAG tag, const sdbstring& str, bool always = false);
    void WriteString(PDB pdb, TAG tag, const std::string& str, bool always = false);
    void WriteBinary(PDB pdb, TAG tag, const GUID& guid, bool always = false);
    void WriteBinary(PDB pdb, TAG tag, const std::vector<BYTE>& data, bool always = false);
    void WriteDWord(PDB pdb, TAG tag, DWORD value, bool always = false);
    void WriteQWord(PDB pdb, TAG tag, QWORD value, bool always = false);
    TAGID BeginWriteListTag(PDB pdb, TAG tag);
    BOOL EndWriteListTag(PDB pdb, TAGID tagid);

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
    GUID ID;

    struct Library Library;
    std::list<Layer> Layers;
    std::list<Exe> Exes;

private:
    std::map<sdbstring, TAGID> KnownShims;
    std::map<sdbstring, TAGID> KnownPatches;
    std::map<sdbstring, TAGID> KnownFlags;
};

