#pragma once

#include <string>
#include <list>
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
    bool fromXml(XMLHandle dbNode);
    bool toSdb(PDB pdb, Database& db);

    std::string Name;
    std::string CompanyName;
    std::string ProductName;
    std::string ProductVersion;
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
    std::string AppName;
    std::string Vendor;
    TAGID Tagid;
    std::list<ShimRef> ShimRefs;
};

struct Database
{

    bool fromXml(const char* fileName);
    bool fromXml(XMLHandle dbNode);
    bool toSdb(LPCWSTR path);

    void WriteString(PDB pdb, TAG tag, const sdbstring& str);
    void WriteString(PDB pdb, TAG tag, const std::string& str);
    void WriteBinary(PDB pdb, TAG tag, const GUID& guid);
    TAGID BeginWriteListTag(PDB db, TAG tag);
    BOOL EndWriteListTag(PDB db, TAGID tagid);

    void InsertShimTagid(const sdbstring& name, TAGID tagid);
    void InsertShimTagid(const std::string& name, TAGID tagid);
    TAGID FindShimTagid(const sdbstring& name);
    TAGID FindShimTagid(const std::string& name);

    std::string Name;
    GUID ID;

    std::list<Shim> Library;
    std::list<Layer> Layers;
    std::list<Exe> Exes;

private:
    std::map<sdbstring, TAGID> KnownShims;
};

