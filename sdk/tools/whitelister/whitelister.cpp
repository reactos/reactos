/*
* PROJECT:     ReactOS host tools
* LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
* PURPOSE:     Support tool for whitelisting tests
* COPYRIGHT:   Copyright 2021 Timo Kreuzer <timo.kreuzer@reactos.org>
*/

#include <cstdio>
#include <cstdarg>
#include <array>
#include <fstream>
#include <streambuf>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <direct.h>

#define USE_WHITELIST
#include "../include/reactos/wine/test.h"

using namespace std;

// Command to get whitelist entry:
// reactos>git blame -L <Line>,<Line> --show-number --show-name <File Path>
// e.g.:
// c:\reactos>git blame -L 699,699 --show-number --show-name modules\rostests\apitests\advapi32\LockServiceDatabase.c
// 8400fdc0786b rostests/apitests/advapi32/LockDatabase.c 701 (Thomas Faber 2012-06-29 11:48:35 +0000 699)     ok_err(ERROR_SUCCESS);
//
// To reverse it use
// reactos>git blame -L <Old Line>,<Old Line>  -s --reverse --show-number --show-name <old hash> <old file>
// e.g.
// c:\reactos>git blame -L 701,701 --reverse 8400fdc0786b rostests\apitests\advapi32\LockDatabase.c
// f2bc1f0e1118 modules/rostests/apitests/advapi32/LockServiceDatabase.c (Roman Masanin 2021-07-24 21:23:58 +0300 701)     ok_err(ERROR_SUCCESS);
//

#if 0
#define Trace(...) fprintf(stderr, __VA_ARGS__)
#else
#define Trace(...)
#endif

struct WL_LINE_ENTRY
{
    int Line;
    uint32_t Attributes;
    string Reason;
};

struct WL_ENTRY
{
    string FullFileName;
    string ShortFileName;
    vector<WL_LINE_ENTRY> Lines;
};

string
Execute(const char* cmd)
{
    array<char, 128> buffer;
    string result;

    Trace("Execute(\"%s\")\n", cmd);

    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd, "r"), _pclose);
    if (pipe == nullptr)
    {
        throw std::runtime_error("popen() failed!");
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    {
        result += buffer.data();
    }

    return result;
}

string
ExecuteCmd(const char* Format, ...)
{
    array<char, _MAX_PATH> buffer;
    va_list argptr;

    va_start(argptr, Format);
    _vsnprintf(buffer.data(), buffer.size(), Format, argptr);
    va_end(argptr);

    return Execute(buffer.data());
}

string
git_blame(string &File, int Line)
{
    return ExecuteCmd("git blame -L %u,%u --show-number --show-name %s",
                      Line, Line, File.c_str());
}

string
git_rev_blame(string &OldHash, string &OldFile, int OldLine)
{
    return ExecuteCmd("git blame -L %u,%u -s --reverse --show-number --show-name %s %s",
                      OldLine, OldLine, OldHash.c_str(), OldFile.c_str());
}

string
git_rev_parse_head(void)
{
    string result = ExecuteCmd("git rev-parse @{0}");
    return result.substr(0, 12);
}

bool
git_diff_check(string &File)
{
    return ExecuteCmd("git diff --name-only %s", File.c_str()).length() != 0;
}

int
AddWhitelistLine(const string &WhitelistFileName, const string &SourceFileName, int Line)
{
    return 0;
}

uint32_t
GetWhiteListAttributesFromReason(string& Reason)
{
    uint32_t attributes = 0;

    if (Reason.find("#ROS") != string::npos)
    {
        attributes |= WLA_BROKEN_ROS;
    }
    if (Reason.find("#WIN2003") != string::npos)
    {
        attributes |= WLA_BROKEN_WIN2003;
        attributes |= WLA_BROKEN_ROS;
    }
    if (Reason.find("#VISTA") != string::npos)
    {
        attributes |= WLA_BROKEN_WINVISTA;
    }
    if (Reason.find("#WIN7") != string::npos)
    {
        attributes |= WLA_BROKEN_WIN7;
    }
    if (Reason.find("#WIN8") != string::npos)
    {
        attributes |= WLA_BROKEN_WIN8;
    }
    if (Reason.find("#WIN81") != string::npos)
    {
        attributes |= WLA_BROKEN_WIN81;
    }
    if (Reason.find("#WIN10") != string::npos)
    {
        attributes |= WLA_BROKEN_WIN10;
    }

    // When nothing is specified, we assume it's just broken
    if (attributes == 0)
    {
        attributes = WLA_BROKEN_ROS |
                     WLA_BROKEN_WIN2003 |
                     WLA_BROKEN_WINVISTA |
                     WLA_BROKEN_WIN7 |
                     WLA_BROKEN_WIN8 |
                     WLA_BROKEN_WIN81 |
                     WLA_BROKEN_WIN10;
    }

    // Optional architecture
    if (Reason.find("#X86") != string::npos)
    {
        attributes |= WLA_ARCH_X86;
    }
    if (Reason.find("#X64") != string::npos)
    {
        attributes |= WLA_ARCH_X64;
    }

    // When no arch is specified, we assume it's all
    if (!(attributes & WLA_ARCH_ANY))
    {
        attributes |= WLA_ARCH_ANY;
    }

    // Optional specifier for flaky tests
    if (Reason.find("#FLAKY") != string::npos)
    {
        attributes |= WLA_FLAKY;
    }

    Trace("Attributes = 0x%x\n", attributes);
    return attributes;
}

void
AddWhitelistEntry(vector<WL_ENTRY> &Whitelist, string& Line, string& HeadHash, const string& WhiteListFileName)
{
    size_t start, length;

    // Format:
    // <Hash> <original file name> <original line> "<reason>"
    // 8400fdc0786b rostests/apitests/advapi32/LockDatabase.c 701 "Fails on WHS"

    // Start with the hash, which is 12 characters
    string oldHash = Line.substr(0, 12);

    // Get the original file name
    start = 13;
    length = Line.substr(start).find(' ');
    string oldFileName = Line.substr(start, length);
    Trace("oldFileName = '%s'\n", oldFileName.c_str());

    // Get the original line
    start += length + 1;
    length = Line.substr(start).find(' ');
    int oldLine = stoi(Line.substr(start, length));

    // Get the reason
    start += length + 1;
    length = Line.length() - start - 2;
    string reason = Line.substr(start + 1, length);

    // Get attributes from reason
    uint32_t attributes = GetWhiteListAttributesFromReason(reason);

    // Translate the information to how it looks like at the current head
    string translated = git_rev_blame(oldHash, oldFileName, oldLine);
    Trace("Translated: \"%s\"\n", translated.c_str());

    // Format:
    // // <new hash> <new file name> <new line> <original line>) <text>
    // f2bc1f0e1118 modules/rostests/apitests/advapi32/LockServiceDatabase.c 699 701)     ok_err(ERROR_SUCCESS);

    // get the new hash
    string newHash = translated.substr(0, 12);

    // Get the new file name
    start = 13;
    length = translated.substr(start).find(' ');
    string newFileName = translated.substr(start, length);
    Trace("newFileName = '%s'\n", newFileName.c_str());

    // Get the new line
    start += length + 1;
    length = translated.substr(start).find(' ');
    int newLine = stoi(translated.substr(start, length));

    // Try to find the file in the whitelist
    for (auto& wlEntry : Whitelist)
    {
        if (newFileName == wlEntry.FullFileName)
        {
            wlEntry.Lines.push_back(WL_LINE_ENTRY{ newLine, attributes, reason });
            return;
        }
    }

    // Check if the file has local changes
    if (git_diff_check(newFileName))
    {
        string message = "ERROR: File with white-list entries '" + newFileName;
        message += "' Has uncommitted changes in the working tree.\n";
        message += "Please commit your changes first!";
        throw runtime_error(message.c_str());
    }

    // Check if the hash matches HEAD
    if (newHash != HeadHash)
    {
        // We can't check against a modified source. To no accudentally break build, just warn and ignore.
        string message = "Whitelist entry not found in HEAD (" + HeadHash + ").\n";
        message += "Last revision is " + newHash + ".\n";
        message += "Please update white list file '" + WhiteListFileName + "'\n";
        throw runtime_error(message.c_str());
    }

    // Get the short file name
    size_t pathLength = newFileName.rfind('/');
    string shortFileName = newFileName.substr(pathLength + 1);

    // File is not in the list yet, add it
    WL_ENTRY wlEntry;
    wlEntry.FullFileName = newFileName;
    wlEntry.ShortFileName = shortFileName;
    wlEntry.Lines.push_back(WL_LINE_ENTRY{ newLine, attributes, reason });
    Whitelist.push_back(wlEntry);
}

vector<WL_ENTRY>
ReadWhitelist(const string& WhitelistFileName)
{
    vector<WL_ENTRY> whitelist;

    string headHash = git_rev_parse_head();

    ifstream fileStream(WhitelistFileName);
    for (string line; getline(fileStream, line); )
    {
        AddWhitelistEntry(whitelist, line, headHash, WhitelistFileName);
    }

    return whitelist;
}

string
CreateLinesTableCode(WL_ENTRY& WlEntry)
{
    string code;

    // Get file name without extension
    size_t nameLength = WlEntry.ShortFileName.rfind('.');
    string sourceFileNameNoExt = WlEntry.ShortFileName.substr(0, nameLength);

    code = "static WHITELIST_ENTRY s_Whitelist_" + sourceFileNameNoExt + "[] =\n";
    code += "{\n";
    for (auto& lineEntry : WlEntry.Lines)
    {
        code += "    { ";
        code += to_string(lineEntry.Line) + ", ";
        code += to_string(lineEntry.Attributes) + ", ";
        code += "\"" + lineEntry.Reason + "\"},\n";
    }
    code += "};\n\n";

    return code;
}

string
GenerateWhitelistCode(vector<WL_ENTRY>& Whitelist)
{
    string code;

    Trace("size of whitelist = %Iu\n", Whitelist.size());

    code = "#include <wine/test.h>\n\n";

    for (auto& wlEntry : Whitelist)
    {
        string lineTableCode = CreateLinesTableCode(wlEntry);
        code += lineTableCode;
    }

    code += "WHITELIST_FILE_ENTRY g_Whitelist[] =\n";
    code += "{\n";
    for (auto& wlEntry : Whitelist)
    {
        Trace("Size of whitelist entry = %Iu\n", wlEntry.Lines.size());

        // Get file name without extension
        size_t nameLength = wlEntry.ShortFileName.rfind('.');
        string sourceFileNameNoExt = wlEntry.ShortFileName.substr(0, nameLength);

        string fileWhitelistName = "s_Whitelist_" + sourceFileNameNoExt;
        code += "    { \"" + wlEntry.ShortFileName + "\", " + fileWhitelistName + ", _countof(" + fileWhitelistName + ")},\n";
    }

    code += "};\n\n";
    code += "int g_WhitelistLength = _countof(g_Whitelist);\n";

    return code;
}

int
GenerateTableCode(const string &WhitelistFileName, const string &OutputFileName)
{
    vector<WL_ENTRY> whitelist = ReadWhitelist(WhitelistFileName);
    string code = GenerateWhitelistCode(whitelist);
    ofstream outputFile(OutputFileName);
    outputFile << code;
    return 0;
}

void Usage(void)
{
    printf("Usage: whitelister --add-entry <whitelist-file> <source-file> <line>\n");
    printf("Usage: whitelister --gen-tables <whitelist-file> <output-file> <reactos source root>\n");
}

int main(int argc, const char* argv[])
{
    char oldpath[_MAX_PATH];
    int result;

    if (argc < 4)
    {
        Usage();
        return 1;
    }

    if (string(argv[1]) == "--add-entry")
    {
        if (argc != 5)
        {
            printf("Usage: whitelister --add-entry <whitelist-file> <source-file> <line>\n");
            return 1;
        }

        return AddWhitelistLine(string{ argv[2] }, string{ argv[3] }, atoi(argv[4]));
    }

    if (string(argv[1]) == "--gen-tables")
    {
        if (argc != 5)
        {
            printf("Usage: whitelister --gen-tables <whitelist-file> <output-file> <reactos source root>\n");
            return 1;
        }

        try
        {
            _getcwd(oldpath, sizeof(oldpath));
            _chdir(argv[4]);
            result = GenerateTableCode(string(argv[2]), string(argv[3]));
        }
        catch(exception &e)
        {
            fprintf(stderr, e.what());
            result = -1;
        }

        _chdir(oldpath);
        return result;
    }

    Usage();
    return 1;
}
