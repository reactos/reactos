#include "Operation.h"

#ifndef _PARSER_H
#define _PARSER_H

#define INPUT_BUFFER_SZ     256
#define INPUT_TOKEN_SZ      80

#define TOKEN_SZ_BLANKS        " \t\n\r"
#define TOKEN_CH_COM_DELIM     ';'
#define TOKEN_CH_NL            '\n'
#define TOKEN_CH_STR_DELIM     '\"'

#define KEYWORD_PRAGMA         "#pragma"
#define KEYWORD_CMD_ADD_EVENT  "ADD"
#define KEYWORD_CMD_DEL_EVENT  "DELETE"
#define KEYWORD_CMD_ADD_TRAP   "ADD_TRAP_DEST"
#define KEYWORD_CMD_DEL_TRAP   "DELETE_TRAP_DEST"

class COperation;

class CParser
{
    COperation  *m_pOperList;

    int          m_fdInput;
    char         m_szInput[INPUT_BUFFER_SZ];
    char        *m_pInput;

    DWORD OpenInputFile();
    DWORD ReloadInputBuffer();
    DWORD AdvanceInputPointer();
public:
    DWORD        m_nLineNo;
    DWORD        m_nTokenNo;

    CParser();
    ~CParser();

    DWORD GetNextToken(char *pToken, int nSizeToken);
    DWORD UnGetToken(char *szToken);
    DWORD CheckUnGetToken(char *pMatchToken, char *pToken);

    DWORD ParseInputFile();
    DWORD ParseCommand(tOperation opType);

    DWORD ProcessCommands();
};

extern CParser gParser;

#endif
