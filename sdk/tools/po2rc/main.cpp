/*
 * PROJECT:     ReactOS po-rc creation tool
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Combine RC and PO files into RC files usable by the compiler
 * COPYRIGHT:   Copyright Rafał Harabień (rafalh@reactos.org)
 *              Copyright Mark Jansen (mark.jansen@reactos.org)
 */

#include <cstring>
#include <fstream>
#include <map>
#include <string>
#include <iostream>
#include <string>

using namespace std;

struct SLangInfo
{
    const char *pszIdentifier;
    const char *pszLang;
    const char *pszSublang;
} g_Languages[] = {
    { "ar-DZ", "LANG_ARABIC", "SUBLANG_DEFAULT" },
    { "bg-BG", "LANG_BULGARIAN", "SUBLANG_DEFAULT" },
    { "ca-ES", "LANG_CATALAN", "SUBLANG_DEFAULT" },
    { "cs-CZ", "LANG_CZECH", "SUBLANG_DEFAULT" },
    { "da-DK", "LANG_DANISH", "SUBLANG_DEFAULT" },
    { "de-DE", "LANG_GERMAN", "SUBLANG_NEUTRAL" },
    { "el-GR", "LANG_GREEK", "SUBLANG_DEFAULT" },
    { "en-GB", "LANG_ENGLISH", "SUBLANG_ENGLISH_UK" },
    //{ "en-US", "LANG_ENGLISH", "SUBLANG_DEFAULT" },
    { "en-US", "LANG_ENGLISH", "SUBLANG_ENGLISH_US" },
    { "es-ES", "LANG_SPANISH", "SUBLANG_SPANISH" },
    //{ "es-ES", "LANG_SPANISH", "SUBLANG_NEUTRAL" },
    { "eu-ES", "LANG_BASQUE", "SUBLANG_DEFAULT" },
    { "fi-FI", "LANG_FINNISH", "SUBLANG_DEFAULT" },
    { "fr-CA", "LANG_FRENCH", "SUBLANG_FRENCH_CANADIAN" },
    //{ "fr-FR", "LANG_FRENCH", "SUBLANG_DEFAULT" },
    { "fr-FR", "LANG_FRENCH", "SUBLANG_FRENCH" },
    //{ "fr-FR", "LANG_FRENCH", "SUBLANG_NEUTRAL" },
    { "he-IL", "LANG_HEBREW", "SUBLANG_DEFAULT" },
    { "hu-HU", "LANG_HUNGARIAN", "SUBLANG_DEFAULT" },
    { "hy-AM", "LANG_ARMENIAN", "SUBLANG_DEFAULT" },
    { "id-ID", "LANG_INDONESIAN", "SUBLANG_DEFAULT" },
    { "it-IT", "LANG_ITALIAN", "SUBLANG_DEFAULT" },
    //{ "it-IT", "LANG_ITALIAN", "SUBLANG_NEUTRAL" },
    { "ja-JP", "LANG_JAPANESE", "SUBLANG_DEFAULT" },
    { "ko-KR", "LANG_KOREAN", "SUBLANG_DEFAULT" },
    { "lt-LT", "LANG_LITHUANIAN", "SUBLANG_DEFAULT" },
    //{ "lt-LT", "LANG_LITHUANIAN", "SUBLANG_NEUTRAL" },
    { "ms-MY", "LANG_MALAY", "SUBLANG_DEFAULT" },
    { "nl-NL", "LANG_DUTCH", "SUBLANG_NEUTRAL" },
    //{ "no-NO", "LANG_NORWEGIAN", "SUBLANG_NEUTRAL" },
    { "no-NO", "LANG_NORWEGIAN", "SUBLANG_NORWEGIAN_BOKMAL" },
    { "pl-PL", "LANG_POLISH", "SUBLANG_DEFAULT" },
    //{ "pl-PL", "LANG_POLISH", "SUBLANG_NEUTRAL" },
    //{ "pt-BR", "LANG_PORTUGUESE", "SUBLANG_NEUTRAL" },
    { "pt-BR", "LANG_PORTUGUESE", "SUBLANG_PORTUGUESE_BRAZILIAN" },
    //{ "pt-PT", "LANG_PORTUGUESE", "SUBLANG_NEUTRAL" },
    { "pt-PT", "LANG_PORTUGUESE", "SUBLANG_PORTUGUESE" },
    //{ "pt-PT", "LANG_PORTUGUESE", "SUBLANG_PORTUGUESE_BRAZILIAN" },
    { "ro-RO", "LANG_ROMANIAN", "SUBLANG_DEFAULT" },
    //{ "ro-RO", "LANG_ROMANIAN", "SUBLANG_NEUTRAL" },
    { "ru-RU", "LANG_RUSSIAN", "SUBLANG_DEFAULT" },
    //{ "ru-RU", "LANG_RUSSIAN", "SUBLANG_NEUTRAL" },
    { "sk-SK", "LANG_SLOVAK", "SUBLANG_DEFAULT" },
    { "sl-SI", "LANG_SLOVENIAN", "SUBLANG_DEFAULT" },
    { "sq-AL", "LANG_ALBANIAN", "SUBLANG_NEUTRAL" },
    { "sr-SP", "LANG_SERBIAN", "SUBLANG_SERBIAN_CYRILLIC" },
    { "sv-SE", "LANG_SWEDISH", "SUBLANG_NEUTRAL" },
    { "th-TH", "LANG_THAI", "SUBLANG_DEFAULT" },
    { "tr-TR", "LANG_TURKISH", "SUBLANG_DEFAULT" },
    { "tr-TR", "LANG_TURKISH", "SUBLANG_NEUTRAL" },
    { "uk-UA", "LANG_UKRAINIAN", "SUBLANG_DEFAULT" },
    { "uz-UZ", "LANG_UZBEK", "SUBLANG_UZBEK_LATIN" },
    { "zh-CN", "LANG_CHINESE", "SUBLANG_CHINESE_SIMPLIFIED" },
    { "zh-TW", "LANG_CHINESE", "SUBLANG_CHINESE_TRADITIONAL" },
};

class CPoParser
{
public:
    CPoParser(ifstream &Stream)
    {
        Load(Stream);
    }

    void Load(ifstream &Stream)
    {
        m_Line = 0;
        m_szBuf = "";
        m_Ptr = m_szBuf.c_str();

        while (Stream.good())
        {
            string strName, strMsgId, strCtxt, strMsg;

            if (!GetEntry(Stream, strName, strMsgId))
                break;

            if (strName == "msgctxt")
            {
                strCtxt = strMsgId;
                if (!GetEntry(Stream, strName, strMsgId))
                    break;
            }

            if (strName != "msgid")
            {
                cerr << "Expected msgid at line " << m_Line << ", got " << strName << "\n";
                continue;
            }

            if (!GetEntry(Stream, strName, strMsg) || strName != "msgstr")
            {
                cerr << "Expected msgstr at line " << m_Line << ", got " << strName << "\n";
                continue;
            }

            if (!strCtxt.empty())
                strMsgId = string("#msgctxt#") + strCtxt + "#" + strMsgId;
            m_Messages[strMsgId] = strMsg;
        }
    }

    const char *GetMsg(const char *pszMsgId)
    {
        map<string, string>::iterator it = m_Messages.find(pszMsgId);
        if (it == m_Messages.end())
            return NULL;
        return it->second.c_str();
    }

private:
    map<string, string> m_Messages;
    unsigned m_Line;
    string m_szBuf;
    const char *m_Ptr;

    bool GetEntry(ifstream &Stream, string &strName, string &strValue)
    {
        if (!GetToken(Stream, strName, false))
            return false;

        if (!GetToken(Stream, strValue, true))
        {
            cerr << "Expected string at line " << m_Line << ", got " << m_Ptr << "\n";
            return false;
        }

        string strTemp;
        while (GetToken(Stream, strTemp, true))
            strValue += strTemp;

        return true;
    }

    bool LoadNextLine(ifstream &Stream)
    {
        while (Stream.good())
        {
            std::getline(Stream, m_szBuf);
            ++m_Line;
            m_Ptr = m_szBuf.c_str();

            IgnoreWhiteChars();
            if (m_Ptr[0] != '#')
                return true;
        }

        return false;
    }

    void IgnoreWhiteChars()
    {
        while (isspace(*m_Ptr))
            ++m_Ptr;
    }

    bool GetToken(ifstream &Stream, string &strResult, bool bStr)
    {
        IgnoreWhiteChars();
        if (!m_Ptr[0] && !LoadNextLine(Stream))
            return false;

        bool bIsStr = (*m_Ptr == '"');
        if (bStr != bIsStr)
            return false;

        strResult = "";

        if (bIsStr)
        {
            ++m_Ptr;
            while (*m_Ptr)
            {
                if (*m_Ptr == '"')
                {
                    ++m_Ptr;
                    break;
                }
                if (m_Ptr[0] == '\\' && m_Ptr[1] == '"')
                {
                    strResult += '"';
                    m_Ptr += 2;
                }
                else
                    strResult += *(m_Ptr++);
            }
        }
        else
        {
            while (*m_Ptr && !isspace(*m_Ptr))
                strResult += *(m_Ptr++);
        }

        return true;
    }
};


enum Context
{
    Invalid,
    None,
    Dialog_Begin,
    Dialog,
    Stringtable_Begin,
    Stringtable,
    Menu_Begin,
    Menu,
};


class CRcTranslator
{
public:
    void Define(const char *pszName, const char *pszValue)
    {
        m_Defines[pszName] = pszValue;
    }

    void Process(istream &Input, ostream &Output, CPoParser &PoParser)
    {
        LoadNextLine(Input);
        string strTemp;

        while (Input.good())
        {
            if (!m_Ptr[0])
            {
                Output << "\n";
                if (!LoadNextLine(Input))
                    break;
                continue;
            }

            if (IgnoreShortComment(Input, Output))
                continue;

            if (IgnoreLongComment(Input, Output))
                continue;

            if (GetString(Input, strTemp))
            {
                OutputTranslatedString(Output, PoParser, strTemp.c_str());
                continue;
            }

            if (GetIdentifier(Input, strTemp))
            {
                map<string, string>::iterator it = m_Defines.find(strTemp);
                if (it == m_Defines.end())
                {
                    Output << strTemp;
                    UpdateActiveContext(strTemp);
                }
                else
                {
                    Output << it->second;
                }
                continue;
            }

            Output << *(m_Ptr++);
        }
    }

private:
    char m_szBuf[256], *m_Ptr;
    map<string, string> m_Defines;
    Context m_Context;
    string m_ContextName;
    string m_PreviousToken;

    bool IgnoreShortComment(istream &Input, ostream &Output)
    {
        if (m_Ptr[0] != '/' || m_Ptr[1] != '/')
            return false;

        Output << m_Ptr << "\n";
        LoadNextLine(Input);
        return true;
    }

    bool IgnoreLongComment(istream &Input, ostream &Output)
    {
        if (m_Ptr[0] != '/' || m_Ptr[1] != '*')
            return false;

        m_Ptr += 2;
        Output << "/*";

        while (true)
        {
            if (!m_Ptr[0])
            {
                Output << "\n";
                if (!LoadNextLine(Input))
                    break;
            }
            else if (m_Ptr[0] == '*' && m_Ptr[1] == '/')
            {
                m_Ptr += 2;
                break;
            }
            else
                Output << *(m_Ptr++);
        }
        Output << "*/";
        return true;
    }

    bool GetString(istream &Input, string &strResult)
    {
        if (m_Ptr[0] != '"')
            return false;

        ++m_Ptr;
        strResult = "";
        while (true)
        {
            if (m_Ptr[0] == '\\' && !m_Ptr[1])
            {
                if (!LoadNextLine(Input))
                    break;
            }
            else if (!m_Ptr[0])
                return false;
            else if (m_Ptr[0] == '"')
            {
                m_Ptr++;
                break;
            }
            else
                strResult += *(m_Ptr++);
        }

        return true;
    }

    bool GetIdentifier(istream &Input, string &strResult)
    {
        strResult = "";
        while (isalnum(*m_Ptr) || *m_Ptr == '_')
            strResult += *(m_Ptr++);

        return !strResult.empty();
    }

    bool LoadNextLine(istream &Stream)
    {
        if (!Stream.good())
            return false;
        Stream.getline(m_szBuf, sizeof(m_szBuf));
        m_Ptr = m_szBuf;
        return true;
    }

    void OutputTranslatedString(ostream &Output, CPoParser &PoParser, const char *pszStr)
    {
        const char *pszMsg = pszStr[0] ? PoParser.GetMsg(pszStr) : NULL;
        if (!pszMsg && pszStr[0])
        {
            string tmp = "#msgctxt#" + m_ContextName;
            if (m_Context == Dialog_Begin || m_Context == Stringtable)
                tmp += "." + m_PreviousToken;
            tmp += "#";
            tmp += pszStr;
            pszMsg = PoParser.GetMsg(tmp.c_str());
        }
        if (!pszMsg || (pszStr[0] && !pszMsg[0]))
            pszMsg = pszStr;

        static char szSpecialCtxt[] = "#msgctxt#do not translate#";
        if (strncmp(pszMsg, szSpecialCtxt, sizeof(szSpecialCtxt) - 1) == 0)
            pszMsg += sizeof(szSpecialCtxt) - 1;
        Output << '"';
        Output << pszMsg;
        Output << '"';
    }

    void UpdateActiveContext(const string& token)
    {
        Context ctx = Invalid;
        string ctxname = m_ContextName;

        if (token == "DIALOG" || token == "DIALOGEX")
        {
            ctx = Dialog_Begin;
            ctxname = token + "." + m_PreviousToken;
        }
        else if (token == "MENU" || token == "MENUEX")
        {
            ctx = Menu_Begin;
            ctxname = m_PreviousToken;
        }
        if (ctx == Invalid)
        {
            if (token == "STRINGTABLE")
            {
                ctx = Stringtable_Begin;
                ctxname = token;
            }
            else if (token == "BEGIN")
            {
                switch (m_Context)
                {
                case Dialog_Begin:
                    ctx = Dialog;
                    break;
                case Stringtable_Begin:
                    ctx = Stringtable;
                    break;
                case Menu_Begin:
                    ctx = Menu;
                    break;
                default:
                    cerr << "Unexpected BEGIN tag" << std::endl;
                    break;
                }
            }
            else if (token == "END")
            {
                switch (m_Context)
                {
                case Dialog:
                case Stringtable:
                case Menu:
                    ctx = None;
                    ctxname.clear();
                    break;
                default:
                    cerr << "Unexpected END tag" << std::endl;
                    break;
                }
            }
        }

        if (ctx != Invalid)
        {
            m_Context = ctx;
            m_ContextName = ctxname;
        }

        m_PreviousToken = token;
    }

};

int main(int argc, const char *argv[])
{
    const char *pszPoPath = NULL, *pszTemplatePath = NULL, *pszOutputPath = "output.rc";
    bool bUsage = false;
    CRcTranslator Translator;

    for (int i = 1; i < argc; ++i)
    {
        const char *pszArg = argv[i];
        if (pszArg[0] == '/' || pszArg[0] == '-')
            ++pszArg;

        char chOpt = tolower(pszArg[0]);
        if (chOpt && pszArg[1])
            chOpt = 0;

        if (chOpt == 'p' && i + 1 < argc)
            pszPoPath = argv[++i];
        else if (chOpt == 't' && i + 1 < argc)
            pszTemplatePath = argv[++i];
        else if (chOpt == 'o' && i + 1 < argc)
            pszOutputPath = argv[++i];
        else if (chOpt == 'l' && i + 1 < argc)
        {
            const char* templateLang = argv[++i];
            bool found = false;
            for (size_t n = 0; n < (sizeof(g_Languages) / sizeof(g_Languages[0])); ++n)
            {
                if (!strcmp(g_Languages[n].pszIdentifier, templateLang))
                {
                    Translator.Define("LANG_PLACEHOLDER", g_Languages[n].pszLang);
                    Translator.Define("SUBLANG_PLACEHOLDER", g_Languages[n].pszSublang);
                    found = true;
                    break;
                }
            }
            if (!found)
                cerr << "Identifier " << templateLang << " unknown" << std::endl;
        }
        else if (chOpt == 'h')
            bUsage = true;
        else if (tolower(pszArg[0]) == 'd')
        {
            const char *pszDef = pszArg + 1;
            if (!pszDef[0] && i + 1 < argc)
                pszDef = argv[++i];

            const char *pszVal = strchr(pszDef, '=');
            string strName;
            if (pszVal)
            {
                string strName(pszDef, pszVal - pszDef);
                Translator.Define(strName.c_str(), pszVal + 1);
            }
            else
                Translator.Define(pszDef, "1");
        }
        else
            cerr << "Warning! Unknown option: " << argv[i] << ".\n";
    }

    if (!pszPoPath || !pszTemplatePath || !pszOutputPath || bUsage)
    {
        cout << "Usage:\n";
        cout << argv[0] << " -t template_path -p po_path [-o output_path] [-Dname=value]\n";
        cout << "\t-r rc_path\tSets path of rc file to process\n";
        cout << "\t-p po_path\tSets path to a po file for translation\n";
        cout << "\t-o output_path\tSets path to output rc file\n";
        cout << "\t-d name=value\tReplaces specified identifier in rc file\n";
        cout << "\t-l Use the specified language identifier (f.e. en-US)\n";

        return 0;
    }

    ifstream RcFile(pszTemplatePath);
    if (!RcFile.good())
    {
        cerr << "Error! Cannot open " << pszTemplatePath << ".\n";
        return -1;
    }

    ifstream PoFile(pszPoPath);
    if (!PoFile.good())
    {
        cerr << "Error! Cannot open " << pszPoPath << ".\n";
        return -1;
    }

    ofstream OutputFile(pszOutputPath);
    if (!RcFile.good())
    {
        cerr << "Error! Cannot open " << pszOutputPath << ".\n";
        return -1;
    }

    CPoParser PoParser(PoFile);
    Translator.Process(RcFile, OutputFile, PoParser);

    return 0;
}
