#pragma once

#include <string>
#include <list>
#include <vector>
#include <map>

#include <typedefs.h>
#include <guiddef.h>
#include "sdbtypes.h"
#include "sdbwrite.h"
#include "sdbtagid.h"

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

struct Layer
{
    Layer() : Tagid(0) { ; }

    bool fromXml(XMLHandle dbNode);
    bool toSdb(PDB pdb, Database& db);

    std::string Name;
    TAGID Tagid;
    std::list<ShimRef> ShimRefs;
};

struct MatchingFile
{
    MatchingFile() : Size(0), Checksum(0) {;}

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
    std::string LinkDate;
    std::string VerLanguage;
    std::string FileDescription;
    std::string OriginalFilename;
    std::string UptoBinFileVersion;
    std::string LinkerVersion;
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
};

struct Library
{
    std::list<InExclude> InExcludes;
    std::list<Shim> Shims;
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
    TAGID BeginWriteListTag(PDB db, TAG tag);
    BOOL EndWriteListTag(PDB db, TAGID tagid);

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

    std::string Name;
    GUID ID;

    struct Library Library;
    std::list<Layer> Layers;
    std::list<Exe> Exes;

private:
    std::map<sdbstring, TAGID> KnownShims;
    std::map<sdbstring, TAGID> KnownPatches;
};

