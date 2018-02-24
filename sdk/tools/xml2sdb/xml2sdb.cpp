/*
 * PROJECT:     xml2sdb
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Conversion functions from xml -> db
 * COPYRIGHT:   Copyright 2016-2018 Mark Jansen (mark.jansen@reactos.org)
 */

#include "xml2sdb.h"
#include "sdbpapi.h"
#include "tinyxml2.h"
#include <time.h>
#include <algorithm>

using tinyxml2::XMLText;

static const GUID GUID_NULL = { 0 };
static const char szCompilerVersion[] = "1.7.0.1";

#if !defined(C_ASSERT)
#define C_ASSERT(expr) extern char (*c_assert(void)) [(expr) ? 1 : -1]
#endif


C_ASSERT(sizeof(GUID) == 16);
C_ASSERT(sizeof(ULONG) == 4);
C_ASSERT(sizeof(LARGE_INTEGER) == 8);
C_ASSERT(sizeof(WCHAR) == 2);
C_ASSERT(sizeof(wchar_t) == 2);
C_ASSERT(sizeof(TAG) == 2);
C_ASSERT(sizeof(TAGID) == 4);


extern "C"
VOID NTAPI RtlSecondsSince1970ToTime(IN ULONG SecondsSince1970,
                          OUT PLARGE_INTEGER Time);


/***********************************************************************
 *   Helper functions
 */


// Convert utf8 to utf16:
// http://stackoverflow.com/a/7154226/4928207

bool IsEmptyGuid(const GUID& g)
{
    return !memcmp(&g, &GUID_NULL, sizeof(GUID));
}

void RandomGuid(GUID& g)
{
    BYTE* p = (BYTE*)&g;
    for (size_t n = 0; n < sizeof(GUID); ++n)
        p[n] = (BYTE)(rand() % 0xff);
}

// Given a node, return the node value (safe)
std::string ToString(XMLHandle node)
{
    XMLText* txtNode = node.FirstChild().ToText();
    const char* txt = txtNode ? txtNode->Value() : NULL;
    if (txt)
        return std::string(txt);
    return std::string();
}

// Given a node, return the node name (safe)
std::string ToNodeName(XMLHandle node)
{
    tinyxml2::XMLNode* raw = node.ToNode();
    const char* txt = raw ? raw->Value() : NULL;
    if (txt)
        return std::string(txt);
    return std::string();
}

// Read either an attribute, or a child node
std::string ReadStringNode(XMLHandle dbNode, const char* nodeName)
{
    tinyxml2::XMLElement* elem = dbNode.ToElement();
    if (elem)
    {
        const char* rawVal = elem->Attribute(nodeName);
        if (rawVal)
            return std::string(rawVal);
    }
    return ToString(dbNode.FirstChildElement(nodeName));
}

DWORD ReadQWordNode(XMLHandle dbNode, const char* nodeName)
{
    std::string value = ReadStringNode(dbNode, nodeName);
    int base = 10;
    if (value.size() > 2 && value[0] == '0' && value[1] == 'x')
    {
        base = 16;
        value = value.substr(2);
    }
    return static_cast<QWORD>(strtoul(value.c_str(), NULL, base));
}

DWORD ReadDWordNode(XMLHandle dbNode, const char* nodeName)
{
    return static_cast<DWORD>(ReadQWordNode(dbNode, nodeName));
}

unsigned char char2byte(char hexChar, bool* success = NULL)
{
    if (hexChar >= '0' && hexChar <= '9')
        return hexChar - '0';
    if (hexChar >= 'A' && hexChar <= 'F')
        return hexChar - 'A' + 10;
    if (hexChar >= 'a' && hexChar <= 'f')
        return hexChar - 'a' + 10;

    if (success)
        *success = false;
    return 0;
}

// adapted from wine's ntdll\rtlstr.c rev 1.45
static bool StringToGuid(const std::string& str, GUID& guid)
{
    const char *lpszGUID = str.c_str();
    BYTE* lpOut = (BYTE*)&guid;
    int i = 0;
    bool expectBrace = true;
    while (i <= 37)
    {
        switch (i)
        {
        case 0:
            if (*lpszGUID != '{')
            {
                i++;
                expectBrace = false;
                continue;
            }
            break;

        case 9:
        case 14:
        case 19:
        case 24:
            if (*lpszGUID != '-')
                return false;
            break;

        case 37:
            return expectBrace == (*lpszGUID == '}');

        default:
        {
            CHAR ch = *lpszGUID, ch2 = lpszGUID[1];
            unsigned char byte;
            bool converted = true;

            byte = char2byte(ch, &converted) << 4 | char2byte(ch2, &converted);
            if (!converted)
                return false;

            switch (i)
            {
#ifndef WORDS_BIGENDIAN
                /* For Big Endian machines, we store the data such that the
                 * dword/word members can be read as DWORDS and WORDS correctly. */
                /* Dword */
            case 1:
                lpOut[3] = byte;
                break;
            case 3:
                lpOut[2] = byte;
                break;
            case 5:
                lpOut[1] = byte;
                break;
            case 7:
                lpOut[0] = byte;
                lpOut += 4;
                break;
                /* Word */
            case 10:
            case 15:
                lpOut[1] = byte;
                break;
            case 12:
            case 17:
                lpOut[0] = byte;
                lpOut += 2;
                break;
#endif
                /* Byte */
            default:
                lpOut[0] = byte;
                lpOut++;
                break;
            }

            lpszGUID++; /* Skip 2nd character of byte */
            i++;
        }
        }

        lpszGUID++;
        i++;
    }
    return false;
}

bool ReadGuidNode(XMLHandle dbNode, const char* nodeName, GUID& guid)
{
    std::string value = ReadStringNode(dbNode, nodeName);
    if (!StringToGuid(value, guid))
    {
        memset(&guid, 0, sizeof(GUID));
        return false;
    }
    return true;
}

bool ReadBinaryNode(XMLHandle dbNode, const char* nodeName, std::vector<BYTE>& data)
{
    std::string value = ReadStringNode(dbNode, nodeName);
    value.erase(std::remove_if(value.begin(), value.end(), ::isspace), value.end());

    size_t length = value.size() / 2;
    if (length * 2 != value.size())
        return false;

    data.resize(length);
    for (size_t n = 0; n < length; ++n)
    {
        data[n] = (BYTE)(char2byte(value[n * 2]) << 4 | char2byte(value[(n * 2) + 1]));
    }
    return true;
}


/***********************************************************************
 *   InExclude
 */

bool InExclude::fromXml(XMLHandle dbNode)
{
    Module = ReadStringNode(dbNode, "MODULE");
    // Special module names: '$' and '*'
    if (!Module.empty())
    {
        Include = ToNodeName(dbNode) == "INCLUDE";
        return true;
    }
    return false;
}

bool InExclude::toSdb(PDB pdb, Database& db)
{
    TAGID tagid = db.BeginWriteListTag(pdb, TAG_INEXCLUD);
    db.WriteString(pdb, TAG_MODULE, Module, true);
    if (Include)
        SdbWriteNULLTag(pdb, TAG_INCLUDE);
    return !!db.EndWriteListTag(pdb, tagid);
}


template<typename T>
void ReadGeneric(XMLHandle dbNode, std::list<T>& result, const char* nodeName)
{
    XMLHandle node = dbNode.FirstChildElement(nodeName);
    while (node.ToNode())
    {
        T object;
        if (object.fromXml(node))
            result.push_back(object);

        node = node.NextSiblingElement(nodeName);
    }
}

template<typename T>
bool WriteGeneric(PDB pdb, std::list<T>& data, Database& db)
{
    for (typename std::list<T>::iterator it = data.begin(); it != data.end(); ++it)
    {
        if (!it->toSdb(pdb, db))
            return false;
    }
    return true;
}


/***********************************************************************
 *   ShimRef
 */

bool ShimRef::fromXml(XMLHandle dbNode)
{
    Name = ReadStringNode(dbNode, "NAME");
    CommandLine = ReadStringNode(dbNode, "COMMAND_LINE");
    ReadGeneric(dbNode, InExcludes, "INCLUDE");
    ReadGeneric(dbNode, InExcludes, "EXCLUDE");
    return !Name.empty();
}

bool ShimRef::toSdb(PDB pdb, Database& db)
{
    TAGID tagid = db.BeginWriteListTag(pdb, TAG_SHIM_REF);
    db.WriteString(pdb, TAG_NAME, Name, true);
    db.WriteString(pdb, TAG_COMMAND_LINE, CommandLine);

    if (!ShimTagid)
        ShimTagid = db.FindShimTagid(Name);
    SdbWriteDWORDTag(pdb, TAG_SHIM_TAGID, ShimTagid);
    return !!db.EndWriteListTag(pdb, tagid);
}



/***********************************************************************
 *   FlagRef
 */

bool FlagRef::fromXml(XMLHandle dbNode)
{
    Name = ReadStringNode(dbNode, "NAME");
    return !Name.empty();
}

bool FlagRef::toSdb(PDB pdb, Database& db)
{
    TAGID tagid = db.BeginWriteListTag(pdb, TAG_FLAG_REF);
    db.WriteString(pdb, TAG_NAME, Name, true);

    if (!FlagTagid)
        FlagTagid = db.FindFlagTagid(Name);
    SdbWriteDWORDTag(pdb, TAG_FLAG_TAGID, FlagTagid);
    return !!db.EndWriteListTag(pdb, tagid);
}


/***********************************************************************
 *   Shim
 */

bool Shim::fromXml(XMLHandle dbNode)
{
    Name = ReadStringNode(dbNode, "NAME");
    DllFile = ReadStringNode(dbNode, "DLLFILE");
    ReadGuidNode(dbNode, "FIX_ID", FixID);
    // GENERAL ?
    // DESCRIPTION_RC_ID
    ReadGeneric(dbNode, InExcludes, "INCLUDE");
    ReadGeneric(dbNode, InExcludes, "EXCLUDE");
    return !Name.empty() && !DllFile.empty();
}

bool Shim::toSdb(PDB pdb, Database& db)
{
    Tagid = db.BeginWriteListTag(pdb, TAG_SHIM);
    db.InsertShimTagid(Name, Tagid);
    db.WriteString(pdb, TAG_NAME, Name);
    db.WriteString(pdb, TAG_DLLFILE, DllFile);
    if (IsEmptyGuid(FixID))
        RandomGuid(FixID);
    db.WriteBinary(pdb, TAG_FIX_ID, FixID);
    if (!WriteGeneric(pdb, InExcludes, db))
        return false;
    return !!db.EndWriteListTag(pdb, Tagid);
}


/***********************************************************************
 *   Flag
 */

bool Flag::fromXml(XMLHandle dbNode)
{
    Name = ReadStringNode(dbNode, "NAME");

    KernelFlags = ReadQWordNode(dbNode, "FLAG_MASK_KERNEL");
    UserFlags = ReadQWordNode(dbNode, "FLAG_MASK_USER");
    ProcessParamFlags = ReadQWordNode(dbNode, "FLAG_PROCESSPARAM");

    return !Name.empty();
}

bool Flag::toSdb(PDB pdb, Database& db)
{
    Tagid = db.BeginWriteListTag(pdb, TAG_FLAG);
    db.InsertFlagTagid(Name, Tagid);
    db.WriteString(pdb, TAG_NAME, Name, true);

    db.WriteQWord(pdb, TAG_FLAG_MASK_KERNEL, KernelFlags);
    db.WriteQWord(pdb, TAG_FLAG_MASK_USER, UserFlags);
    db.WriteQWord(pdb, TAG_FLAG_PROCESSPARAM, ProcessParamFlags);

    return !!db.EndWriteListTag(pdb, Tagid);
}


/***********************************************************************
 *   Data
 */

#ifndef REG_SZ
#define REG_SZ 1
//#define REG_BINARY 3
#define REG_DWORD 4
#define REG_QWORD 11
#endif


bool Data::fromXml(XMLHandle dbNode)
{
    Name = ReadStringNode(dbNode, "NAME");

    StringData = ReadStringNode(dbNode, "DATA_STRING");
    if (!StringData.empty())
    {
        DataType = REG_SZ;
        return !Name.empty();
    }
    DWordData = ReadDWordNode(dbNode, "DATA_DWORD");
    if (DWordData)
    {
        DataType = REG_DWORD;
        return !Name.empty();
    }
    QWordData = ReadQWordNode(dbNode, "DATA_QWORD");
    if (QWordData)
    {
        DataType = REG_QWORD;
        return !Name.empty();
    }

    SHIM_ERR("Data node (%s) without value!\n", Name.c_str());
    return false;
}

bool Data::toSdb(PDB pdb, Database& db)
{
    Tagid = db.BeginWriteListTag(pdb, TAG_DATA);
    db.WriteString(pdb, TAG_NAME, Name, true);
    db.WriteDWord(pdb, TAG_DATA_VALUETYPE, DataType, true);
    switch (DataType)
    {
    case REG_SZ:
        db.WriteString(pdb, TAG_DATA_STRING, StringData);
        break;
    case REG_DWORD:
        db.WriteDWord(pdb, TAG_DATA_DWORD, DWordData);
        break;
    case REG_QWORD:
        db.WriteQWord(pdb, TAG_DATA_QWORD, QWordData);
        break;
    default:
        SHIM_ERR("Data node (%s) with unknown type (0x%x)\n", Name.c_str(), DataType);
        return false;
    }

    return !!db.EndWriteListTag(pdb, Tagid);
}

/***********************************************************************
 *   Layer
 */

bool Layer::fromXml(XMLHandle dbNode)
{
    Name = ReadStringNode(dbNode, "NAME");
    ReadGeneric(dbNode, ShimRefs, "SHIM_REF");
    ReadGeneric(dbNode, FlagRefs, "FLAG_REF");
    ReadGeneric(dbNode, Datas, "DATA");
    return true;
}

bool Layer::toSdb(PDB pdb, Database& db)
{
    Tagid = db.BeginWriteListTag(pdb, TAG_LAYER);
    db.WriteString(pdb, TAG_NAME, Name, true);
    if (!WriteGeneric(pdb, ShimRefs, db))
        return false;
    if (!WriteGeneric(pdb, FlagRefs, db))
        return false;
    if (!WriteGeneric(pdb, Datas, db))
        return false;
    return !!db.EndWriteListTag(pdb, Tagid);
}


/***********************************************************************
 *   MatchingFile
 */

bool MatchingFile::fromXml(XMLHandle dbNode)
{
    Name = ReadStringNode(dbNode, "NAME");
    Size = ReadDWordNode(dbNode, "SIZE");
    Checksum = ReadDWordNode(dbNode, "CHECKSUM");
    CompanyName = ReadStringNode(dbNode, "COMPANY_NAME");
    InternalName = ReadStringNode(dbNode, "INTERNAL_NAME");
    ProductName = ReadStringNode(dbNode, "PRODUCT_NAME");
    ProductVersion = ReadStringNode(dbNode, "PRODUCT_VERSION");
    FileVersion = ReadStringNode(dbNode, "FILE_VERSION");
    BinFileVersion = ReadStringNode(dbNode, "BIN_FILE_VERSION");
    LinkDate = ReadDWordNode(dbNode, "LINK_DATE");
    VerLanguage = ReadStringNode(dbNode, "VER_LANGUAGE");
    FileDescription = ReadStringNode(dbNode, "FILE_DESCRIPTION");
    OriginalFilename = ReadStringNode(dbNode, "ORIGINAL_FILENAME");
    UptoBinFileVersion = ReadStringNode(dbNode, "UPTO_BIN_FILE_VERSION");
    LinkerVersion = ReadDWordNode(dbNode, "LINKER_VERSION");
    return true;
}

bool MatchingFile::toSdb(PDB pdb, Database& db)
{
    TAGID tagid = db.BeginWriteListTag(pdb, TAG_MATCHING_FILE);

    db.WriteString(pdb, TAG_NAME, Name, true);
    db.WriteDWord(pdb, TAG_SIZE, Size);
    db.WriteDWord(pdb, TAG_CHECKSUM, Checksum);
    db.WriteString(pdb, TAG_COMPANY_NAME, CompanyName);
    db.WriteString(pdb, TAG_INTERNAL_NAME, InternalName);
    db.WriteString(pdb, TAG_PRODUCT_NAME, ProductName);
    db.WriteString(pdb, TAG_PRODUCT_VERSION, ProductVersion);
    db.WriteString(pdb, TAG_FILE_VERSION, FileVersion);
    if (!BinFileVersion.empty())
        SHIM_ERR("TAG_BIN_FILE_VERSION Unimplemented\n"); //db.WriteQWord(pdb, TAG_BIN_FILE_VERSION, BinFileVersion);
    db.WriteDWord(pdb, TAG_LINK_DATE, LinkDate);
    if (!VerLanguage.empty())
        SHIM_ERR("TAG_VER_LANGUAGE Unimplemented\n"); //db.WriteDWord(pdb, TAG_VER_LANGUAGE, VerLanguage);
    db.WriteString(pdb, TAG_FILE_DESCRIPTION, FileDescription);
    db.WriteString(pdb, TAG_ORIGINAL_FILENAME, OriginalFilename);
    if (!UptoBinFileVersion.empty())
        SHIM_ERR("TAG_UPTO_BIN_FILE_VERSION Unimplemented\n"); //db.WriteQWord(pdb, TAG_UPTO_BIN_FILE_VERSION, UptoBinFileVersion);
    db.WriteDWord(pdb, TAG_LINKER_VERSION, LinkerVersion);

    return !!db.EndWriteListTag(pdb, tagid);
}


/***********************************************************************
 *   Exe
 */

bool Exe::fromXml(XMLHandle dbNode)
{
    Name = ReadStringNode(dbNode, "NAME");
    ReadGuidNode(dbNode, "EXE_ID", ExeID);
    AppName = ReadStringNode(dbNode, "APP_NAME");
    Vendor = ReadStringNode(dbNode, "VENDOR");

    ReadGeneric(dbNode, MatchingFiles, "MATCHING_FILE");

    ReadGeneric(dbNode, ShimRefs, "SHIM_REF");
    ReadGeneric(dbNode, FlagRefs, "FLAG_REF");

    return !Name.empty();
}

bool Exe::toSdb(PDB pdb, Database& db)
{
    Tagid = db.BeginWriteListTag(pdb, TAG_EXE);

    db.WriteString(pdb, TAG_NAME, Name, true);
    if (IsEmptyGuid(ExeID))
        RandomGuid(ExeID);
    db.WriteBinary(pdb, TAG_EXE_ID, ExeID);


    db.WriteString(pdb, TAG_APP_NAME, AppName);
    db.WriteString(pdb, TAG_VENDOR, Vendor);

    if (!WriteGeneric(pdb, MatchingFiles, db))
        return false;
    if (!WriteGeneric(pdb, ShimRefs, db))
        return false;
    if (!WriteGeneric(pdb, FlagRefs, db))
        return false;

    return !!db.EndWriteListTag(pdb, Tagid);
}


/***********************************************************************
 *   Database
 */

void Database::WriteBinary(PDB pdb, TAG tag, const GUID& guid, bool always)
{
    if (always || !IsEmptyGuid(guid))
        SdbWriteBinaryTag(pdb, tag, (BYTE*)&guid, sizeof(GUID));
}

void Database::WriteBinary(PDB pdb, TAG tag, const std::vector<BYTE>& data, bool always)
{
    if (always || !data.empty())
    SdbWriteBinaryTag(pdb, tag, data.data(), data.size());
}

void Database::WriteString(PDB pdb, TAG tag, const sdbstring& str, bool always)
{
    if (always || !str.empty())
        SdbWriteStringTag(pdb, tag, (LPCWSTR)str.c_str());
}

void Database::WriteString(PDB pdb, TAG tag, const std::string& str, bool always)
{
    WriteString(pdb, tag, sdbstring(str.begin(), str.end()), always);
}

void Database::WriteDWord(PDB pdb, TAG tag, DWORD value, bool always)
{
    if (always || value)
        SdbWriteDWORDTag(pdb, tag, value);
}

void Database::WriteQWord(PDB pdb, TAG tag, QWORD value, bool always)
{
    if (always || value)
        SdbWriteQWORDTag(pdb, tag, value);
}

TAGID Database::BeginWriteListTag(PDB pdb, TAG tag)
{
    return SdbBeginWriteListTag(pdb, tag);
}

BOOL Database::EndWriteListTag(PDB pdb, TAGID tagid)
{
    return SdbEndWriteListTag(pdb, tagid);
}

bool Database::fromXml(XMLHandle dbNode)
{
    Name = ReadStringNode(dbNode, "NAME");
    ReadGuidNode(dbNode, "DATABASE_ID", ID);

    XMLHandle libChild = dbNode.FirstChildElement("LIBRARY").FirstChild();
    while (libChild.ToNode())
    {
        std::string NodeName = ToNodeName(libChild);
        if (NodeName == "SHIM")
        {
            Shim shim;
            if (shim.fromXml(libChild))
                Library.Shims.push_back(shim);
        }
        else if (NodeName == "FLAG")
        {
            Flag flag;
            if (flag.fromXml(libChild))
                Library.Flags.push_back(flag);
        }
        else if (NodeName == "INCLUDE" || NodeName == "EXCLUDE")
        {
            InExclude inex;
            if (inex.fromXml(libChild))
                Library.InExcludes.push_back(inex);
        }
        libChild = libChild.NextSibling();
    }

    ReadGeneric(dbNode, Layers, "LAYER");
    ReadGeneric(dbNode, Exes, "EXE");
    return true;
}

bool Database::fromXml(const char* fileName)
{
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError err = doc.LoadFile(fileName);
    XMLHandle dbHandle = tinyxml2::XMLHandle(&doc).FirstChildElement("SDB").FirstChildElement("DATABASE");
    return fromXml(dbHandle);
}

bool Database::toSdb(LPCWSTR path)
{
    PDB pdb = SdbCreateDatabase(path, DOS_PATH);
    TAGID tidDatabase = BeginWriteListTag(pdb, TAG_DATABASE);
    LARGE_INTEGER li = { 0 };
    RtlSecondsSince1970ToTime(time(0), &li);
    SdbWriteQWORDTag(pdb, TAG_TIME, li.QuadPart);
    WriteString(pdb, TAG_COMPILER_VERSION, szCompilerVersion);
    SdbWriteDWORDTag(pdb, TAG_OS_PLATFORM, 1);
    WriteString(pdb, TAG_NAME, Name, true);
    if (IsEmptyGuid(ID))
    {
        SHIM_WARN("DB has empty ID!\n");
        RandomGuid(ID);
    }
    WriteBinary(pdb, TAG_DATABASE_ID, ID);
    TAGID tidLibrary = BeginWriteListTag(pdb, TAG_LIBRARY);
    if (!WriteGeneric(pdb, Library.InExcludes, *this))
        return false;
    if (!WriteGeneric(pdb, Library.Shims, *this))
        return false;
    if (!WriteGeneric(pdb, Library.Flags, *this))
        return false;
    EndWriteListTag(pdb, tidLibrary);
    if (!WriteGeneric(pdb, Layers, *this))
        return false;
    if (!WriteGeneric(pdb, Exes, *this))
        return false;
    EndWriteListTag(pdb, tidDatabase);

    SdbCloseDatabaseWrite(pdb);
    return true;
}

static void InsertTagid(const sdbstring& name, TAGID tagid, std::map<sdbstring, TAGID>& lookup, const char* type)
{
    sdbstring nameLower = name;
    std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
    if (lookup.find(nameLower) != lookup.end())
    {
        std::string nameA(name.begin(), name.end());
        SHIM_WARN("%s '%s' redefined\n", type, nameA.c_str());
        return;
    }
    lookup[nameLower] = tagid;
}

static TAGID FindTagid(const sdbstring& name, const std::map<sdbstring, TAGID>& lookup)
{
    sdbstring nameLower = name;
    std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
    std::map<sdbstring, TAGID>::const_iterator it = lookup.find(nameLower);
    if (it == lookup.end())
        return 0;
    return it->second;
}

void Database::InsertShimTagid(const sdbstring& name, TAGID tagid)
{
    InsertTagid(name, tagid, KnownShims, "Shim");
}

TAGID Database::FindShimTagid(const sdbstring& name)
{
    return FindTagid(name, KnownShims);
}

void Database::InsertPatchTagid(const sdbstring& name, TAGID tagid)
{
    InsertTagid(name, tagid, KnownPatches, "Patch");
}

TAGID Database::FindPatchTagid(const sdbstring& name)
{
    return FindTagid(name, KnownPatches);
}

void Database::InsertFlagTagid(const sdbstring& name, TAGID tagid)
{
    InsertTagid(name, tagid, KnownFlags, "Flag");
}

TAGID Database::FindFlagTagid(const sdbstring& name)
{
    return FindTagid(name, KnownFlags);
}


bool xml_2_db(const char* xml, const WCHAR* sdb)
{
    Database db;
    if (db.fromXml(xml))
    {
        return db.toSdb((LPCWSTR)sdb);
    }
    return false;
}
