/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS xml to sdb converter
 * FILE:        sdk/tools/xml2sdb/xml2sdb.cpp
 * PURPOSE:     Main conversion functions from xml -> db
 * PROGRAMMERS: Mark Jansen
 *
 */

#include "xml2sdb.h"
#include "sdbpapi.h"
#include "tinyxml2.h"
#include <time.h>
#include <algorithm>

using tinyxml2::XMLText;

static const GUID GUID_NULL = { 0 };

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

bool IsEmptyGuid(GUID& g)
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

            /* Read two hex digits as a byte value */
            if (ch >= '0' && ch <= '9')
                ch = ch - '0';
            else if (ch >= 'a' && ch <= 'f')
                ch = ch - 'a' + 10;
            else if (ch >= 'A' && ch <= 'F')
                ch = ch - 'A' + 10;
            else
                return false;

            if (ch2 >= '0' && ch2 <= '9')
                ch2 = ch2 - '0';
            else if (ch2 >= 'a' && ch2 <= 'f')
                ch2 = ch2 - 'a' + 10;
            else if (ch2 >= 'A' && ch2 <= 'F')
                ch2 = ch2 - 'A' + 10;
            else
                return false;

            byte = ch << 4 | ch2;

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

bool ReadBinaryNode(XMLHandle dbNode, const char* nodeName, GUID& guid)
{
    std::string value = ReadStringNode(dbNode, nodeName);
    if (!StringToGuid(value, guid))
    {
        memset(&guid, 0, sizeof(GUID));
        return false;
    }
    return true;
}


/***********************************************************************
 *   InExclude
 */

bool InExclude::fromXml(XMLHandle dbNode)
{
    Module = ReadStringNode(dbNode, "MODULE");
    if (!Module.empty())
    {
        Include = dbNode.FirstChildElement("INCLUDE").ToNode() != NULL;
        if (!Include)
        {
            tinyxml2::XMLElement* elem = dbNode.ToElement();
            if (elem)
            {
                Include |= (elem->Attribute("INCLUDE") != NULL);
            }
        }
        // $ = ??
        // *
        return true;
    }
    return false;
}

bool InExclude::toSdb(PDB pdb, Database& db)
{
    TAGID tagid = db.BeginWriteListTag(pdb, TAG_INEXCLUD);
    db.WriteString(pdb, TAG_MODULE, Module);
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
    ReadGeneric(dbNode, InExcludes, "INEXCLUDE");
    return !Name.empty();
}

bool ShimRef::toSdb(PDB pdb, Database& db)
{
    TAGID tagid = db.BeginWriteListTag(pdb, TAG_SHIM_REF);
    db.WriteString(pdb, TAG_NAME, Name);
    if (!CommandLine.empty())
        db.WriteString(pdb, TAG_COMMAND_LINE, CommandLine);

    if (!ShimTagid)
        ShimTagid = db.FindShimTagid(Name);
    SdbWriteDWORDTag(pdb, TAG_SHIM_TAGID, ShimTagid);
    return !!db.EndWriteListTag(pdb, tagid);
}


/***********************************************************************
 *   Shim
 */

bool Shim::fromXml(XMLHandle dbNode)
{
    Name = ReadStringNode(dbNode, "NAME");
    DllFile = ReadStringNode(dbNode, "DLLFILE");
    ReadBinaryNode(dbNode, "FIX_ID", FixID);
    // GENERAL ?
    // DESCRIPTION_RC_ID
    ReadGeneric(dbNode, InExcludes, "INEXCLUDE");
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
 *   Layer
 */

bool Layer::fromXml(XMLHandle dbNode)
{
    Name = ReadStringNode(dbNode, "NAME");
    ReadGeneric(dbNode, ShimRefs, "SHIM_REF");
    return true;
}

bool Layer::toSdb(PDB pdb, Database& db)
{
    Tagid = db.BeginWriteListTag(pdb, TAG_LAYER);
    db.WriteString(pdb, TAG_NAME, Name);
    if (!WriteGeneric(pdb, ShimRefs, db))
        return false;
    return !!db.EndWriteListTag(pdb, Tagid);
}


/***********************************************************************
 *   MatchingFile
 */

bool MatchingFile::fromXml(XMLHandle dbNode)
{
    Name = ReadStringNode(dbNode, "NAME");
    CompanyName = ReadStringNode(dbNode, "COMPANY_NAME");
    ProductName = ReadStringNode(dbNode, "PRODUCT_NAME");
    ProductVersion = ReadStringNode(dbNode, "PRODUCT_VERSION");
    BinFileVersion = ReadStringNode(dbNode, "BIN_FILE_VERSION");
    LinkDate = ReadStringNode(dbNode, "LINK_DATE");
    VerLanguage = ReadStringNode(dbNode, "VER_LANGUAGE");
    FileDescription = ReadStringNode(dbNode, "FILE_DESCRIPTION");
    OriginalFilename = ReadStringNode(dbNode, "ORIGINAL_FILENAME");
    UptoBinFileVersion = ReadStringNode(dbNode, "UPTO_BIN_FILE_VERSION");
    LinkerVersion = ReadStringNode(dbNode, "LINKER_VERSION");
    return true;
}

bool MatchingFile::toSdb(PDB pdb, Database& db)
{
    TAGID tagid = db.BeginWriteListTag(pdb, TAG_MATCHING_FILE);
    SHIM_ERR("Unimplemented\n");

    return !!db.EndWriteListTag(pdb, tagid);
}


/***********************************************************************
 *   Exe
 */

bool Exe::fromXml(XMLHandle dbNode)
{
    Name = ReadStringNode(dbNode, "NAME");
    AppName = ReadStringNode(dbNode, "APP_NAME");
    Vendor = ReadStringNode(dbNode, "VENDOR");

    ReadGeneric(dbNode, ShimRefs, "SHIM_REF");

    return !Name.empty();
}

bool Exe::toSdb(PDB pdb, Database& db)
{
    Tagid = db.BeginWriteListTag(pdb, TAG_EXE);
    SHIM_ERR("Unimplemented\n");

    return !!db.EndWriteListTag(pdb, Tagid);
}


/***********************************************************************
 *   Database
 */

void Database::WriteBinary(PDB pdb, TAG tag, const GUID& guid)
{
    SdbWriteBinaryTag(pdb, tag, (BYTE*)&guid, sizeof(GUID));
}

void Database::WriteString(PDB pdb, TAG tag, const sdbstring& str)
{
    SdbWriteStringTag(pdb, tag, (LPCWSTR)str.c_str());
}

void Database::WriteString(PDB pdb, TAG tag, const std::string& str)
{
    WriteString(pdb, tag, sdbstring(str.begin(), str.end()));
}

TAGID Database::BeginWriteListTag(PDB db, TAG tag)
{
    return SdbBeginWriteListTag(db, tag);
}

BOOL Database::EndWriteListTag(PDB db, TAGID tagid)
{
    return SdbEndWriteListTag(db, tagid);
}

bool Database::fromXml(XMLHandle dbNode)
{
    Name = ReadStringNode(dbNode, "NAME");
    ReadBinaryNode(dbNode, "DATABASE_ID", ID);

    XMLHandle libChild = dbNode.FirstChildElement("LIBRARY").FirstChild();
    while (libChild.ToNode())
    {
        std::string NodeName = ToNodeName(libChild);
        if (NodeName == "SHIM")
        {
            Shim shim;
            if (shim.fromXml(libChild))
                Library.push_back(shim);
        }
        else if (NodeName == "FLAG")
        {
            SHIM_ERR("Unhanled FLAG type\n");
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
    WriteString(pdb, TAG_COMPILER_VERSION, "1.0.0.0");
    SdbWriteDWORDTag(pdb, TAG_OS_PLATFORM, 1);
    WriteString(pdb, TAG_NAME, Name);
    WriteBinary(pdb, TAG_DATABASE_ID, ID);
    TAGID tidLibrary = BeginWriteListTag(pdb, TAG_LIBRARY);
    if (!WriteGeneric(pdb, Library, *this))
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


void Database::InsertShimTagid(const sdbstring& name, TAGID tagid)
{
    sdbstring nameLower = name;
    std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
    if (KnownShims.find(nameLower) != KnownShims.end())
    {
        std::string nameA(name.begin(), name.end());
        SHIM_WARN("Shim '%s' redefined\n", nameA.c_str());
        return;
    }
    KnownShims[nameLower] = tagid;
}

void Database::InsertShimTagid(const std::string& name, TAGID tagid)
{
    InsertShimTagid(sdbstring(name.begin(), name.end()), tagid);
}

TAGID Database::FindShimTagid(const sdbstring& name)
{
    sdbstring nameLower = name;
    std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
    std::map<sdbstring, TAGID>::iterator it = KnownShims.find(nameLower);
    if (it == KnownShims.end())
        return 0;
    return it->second;
}

TAGID Database::FindShimTagid(const std::string& name)
{
    return FindShimTagid(sdbstring(name.begin(), name.end()));
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
