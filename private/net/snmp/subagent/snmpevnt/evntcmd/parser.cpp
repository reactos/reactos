#include <stdafx.h>
#include "Parser.h"
#include "EventCmd.h"
#include "Errors.h"

CParser gParser;

CParser::CParser()
{
    m_pOperList = NULL;
    m_fdInput   = -1;
    m_nLineNo   = 1;
    m_nTokenNo  = 0;
    m_pInput    = m_szInput;
}

CParser::~CParser()
{
    if (m_fdInput != -1)
        _close(m_fdInput);
    if (m_pOperList != NULL)
        delete m_pOperList;
}

DWORD CParser::OpenInputFile()
{
    DWORD retCode;

    m_fdInput = _open(gCommandLine.m_szFileName, _O_RDONLY | _O_BINARY);
    retCode = GetLastError();
    if (retCode != ERROR_SUCCESS)
        _E(retCode, IDS_ERR08, gCommandLine.m_szFileName);
    else
        retCode = ReloadInputBuffer();

    return retCode;
}

DWORD CParser::ReloadInputBuffer()
{
    DWORD retCode;
    int   nBuf;

    m_pInput = m_szInput;
    nBuf = _read(m_fdInput, m_szInput, INPUT_BUFFER_SZ);
    retCode = GetLastError();
    if (retCode != ERROR_SUCCESS)
        _E(retCode, IDS_ERR09, m_nLineNo);
    else if (nBuf < INPUT_BUFFER_SZ)
        m_szInput[nBuf] = '\0';
    return retCode;
}

DWORD CParser::AdvanceInputPointer()
{
    DWORD retCode = ERROR_SUCCESS;

    if (*m_pInput == '\0')
    {
        _W(WARN_TRACK, IDS_TRCK_WRN27, m_nLineNo, m_nTokenNo);
        return ERROR_END_OF_MEDIA;
    }

    if (*m_pInput == TOKEN_CH_NL)
    {
        m_nLineNo++;
        m_nTokenNo;
        m_nTokenNo = 0;
    }

    if (m_pInput < m_szInput + INPUT_BUFFER_SZ - 1)
        m_pInput++;
    else
        retCode = ReloadInputBuffer();

    if (retCode == ERROR_SUCCESS && *m_pInput == '\0')
    {
        _W(WARN_TRACK, IDS_TRCK_WRN28, m_nLineNo, m_nTokenNo);
        retCode = ERROR_END_OF_MEDIA;
    }

    return retCode;
}

DWORD CParser::GetNextToken(char * pToken, int nSizeToken)
{
    int   i;
    DWORD nLineStrDelim;
    DWORD nTokenStrDelim;
    enum
    {
        STATE_BLANKS,
        STATE_COMMENT,
        STATE_TOKEN
    } state = STATE_BLANKS;
    DWORD retCode = ERROR_SUCCESS;

    while (retCode == ERROR_SUCCESS)
    {
        if (state == STATE_BLANKS)
        {
            if (*m_pInput == TOKEN_CH_COM_DELIM)
                state = STATE_COMMENT;
            else if (strchr(TOKEN_SZ_BLANKS, *m_pInput) == NULL)
                break;
        }
        else if (state == STATE_COMMENT)
        {
            if (*m_pInput == TOKEN_CH_NL)
                state = STATE_BLANKS;
        }

        retCode = AdvanceInputPointer();
    }

    i = 0;
    state = STATE_BLANKS;
    m_nTokenNo++;

    while (retCode == ERROR_SUCCESS &&
           *m_pInput != TOKEN_CH_COM_DELIM)
    {
        if (state == STATE_BLANKS)
        {
            if (strchr(TOKEN_SZ_BLANKS, *m_pInput) != NULL)
                break;
            if (*m_pInput == TOKEN_CH_STR_DELIM)
            {
                state = STATE_TOKEN;
                nLineStrDelim = m_nLineNo;
                nTokenStrDelim = m_nTokenNo;
                retCode = AdvanceInputPointer();
                continue;
            }
        }
        if (state == STATE_TOKEN)
        {
            if (*m_pInput == TOKEN_CH_STR_DELIM)
            {
                state = STATE_BLANKS;
                retCode = AdvanceInputPointer();
                break;
            }
        }
        pToken[i++] = *m_pInput;
        if (i >= nSizeToken)
        {
            return _E(ERROR_BUFFER_OVERFLOW, IDS_ERR04,
                      m_nLineNo, m_nTokenNo, nSizeToken);
        }
        retCode = AdvanceInputPointer();
    }

    pToken[i] = '\0';

    if (state == STATE_TOKEN)
    {
        _W(WARN_ERROR, IDS_ERRO_WRN29,
           nLineStrDelim, nTokenStrDelim);
    }

    if (i > 0 && retCode == ERROR_END_OF_MEDIA)
        retCode = ERROR_SUCCESS;

    return retCode;
}

DWORD CParser::UnGetToken(char *szToken)
{
    int nTokenLen;
    char *pInsertion;

    nTokenLen = strlen(szToken) + 2;

    if (m_pInput - m_szInput < nTokenLen)
    {
        int nDrift = nTokenLen - (int)(m_pInput - m_szInput);

        memmove(m_pInput + nDrift, m_pInput, INPUT_BUFFER_SZ - (size_t)(m_pInput - m_szInput) - nDrift);
        m_pInput += nDrift;

        if (_lseek(m_fdInput, -nDrift, SEEK_CUR) == -1)
        {
            return _E(GetLastError(), IDS_ERR10);
        }
    }

    pInsertion = m_pInput = m_pInput - nTokenLen;
    *pInsertion++ = '\"';
    strcpy(pInsertion, szToken);
    pInsertion += nTokenLen - 2;
    *pInsertion = '\"';
    m_nTokenNo--;
    return ERROR_SUCCESS;
}

DWORD CParser::CheckUnGetToken(char *pMatchToken, char *pToken)
{
    DWORD retCode = ERROR_SUCCESS;

    if (strcmp(pMatchToken, pToken) == 0 &&
        (retCode = UnGetToken(pToken)) == ERROR_SUCCESS)
        retCode = ERROR_INVALID_PARAMETER;
    return retCode;
}

DWORD CParser::ParseInputFile()
{
    DWORD retCode;
    DWORD dwSkipLine;
    enum
    {
        STATE_READ,
        STATE_SKIP
    } state = STATE_READ;


    retCode = OpenInputFile();

    if (retCode == ERROR_SUCCESS)
    {
        while(1)
        {
            char  szToken[INPUT_TOKEN_SZ];

            retCode = GetNextToken(szToken, INPUT_TOKEN_SZ);
            if (retCode != ERROR_SUCCESS)
                break;

            if (state == STATE_SKIP)
            {
                if (m_nLineNo == dwSkipLine)
                    continue;
                state = STATE_READ;
            }

            if (state == STATE_READ)
            {
                if (strcmp(szToken, KEYWORD_PRAGMA) != 0)
                {
                    _W(WARN_ALERT, IDS_ALRT_WRN30, m_nLineNo, m_nTokenNo);
                    dwSkipLine = m_nLineNo;
                    state = STATE_SKIP;
                    continue;
                }

                retCode = GetNextToken(szToken, INPUT_TOKEN_SZ);
                if (retCode != ERROR_SUCCESS)
                {
                    _W(WARN_ALERT, IDS_ALRT_WRN31, m_nLineNo, m_nTokenNo);
                    dwSkipLine = m_nLineNo;
                    state = STATE_SKIP;
                    continue;
                }

                if (_stricmp(szToken, KEYWORD_CMD_ADD_EVENT) == 0)
                    retCode = ParseCommand(OP_ADD_EVENT);
                else if (_stricmp(szToken, KEYWORD_CMD_DEL_EVENT) == 0)
                    retCode = ParseCommand(OP_DEL_EVENT);
                else if (_stricmp(szToken, KEYWORD_CMD_ADD_TRAP) == 0)
                    retCode = ParseCommand(OP_ADD_TRAP);
                else if (_stricmp(szToken, KEYWORD_CMD_DEL_TRAP) == 0)
                    retCode = ParseCommand(OP_DEL_TRAP);
                else
                {
                    retCode = ERROR_INVALID_OPERATION;
                    _W(WARN_ALERT, IDS_ALRT_WRN32,
                       m_nLineNo, m_nTokenNo, szToken);
                }

                if (retCode != ERROR_SUCCESS)
                {
                    dwSkipLine = m_nLineNo;
                    state = STATE_SKIP;
                    retCode = ERROR_SUCCESS;
                    continue;
                }
            }
        }
    }

    if (retCode == ERROR_END_OF_MEDIA)
        retCode = ERROR_SUCCESS;
    return retCode;
}

DWORD CParser::ParseCommand(tOperation opType)
{
    DWORD       retCode = ERROR_SUCCESS;
    COperation  *pOperation;

    switch(opType)
    {
    case OP_ADD_EVENT:
    case OP_DEL_EVENT:
        pOperation = new COpEvents(opType);
        break;
    case OP_ADD_TRAP:
    case OP_DEL_TRAP:
        pOperation = new COpTraps(opType);
        break;
    }
    if (pOperation == NULL)
        return _E(ERROR_OUTOFMEMORY, IDS_ERR01);

    retCode = pOperation->ParseCmdArgs();
    if (retCode == ERROR_SUCCESS)
    {
        if (m_pOperList == NULL)
            m_pOperList = pOperation;
        else
            m_pOperList = m_pOperList->Insert(pOperation);
    }
    else
        delete pOperation;

    return retCode;
}

DWORD CParser::ProcessCommands()
{
    DWORD retCode = ERROR_SUCCESS;
    COperation *pOperation;

    for (pOperation = m_pOperList;
         retCode == ERROR_SUCCESS && pOperation != NULL;
         pOperation = pOperation->GetNextOp())
        retCode = pOperation->ProcessCommand();

    return retCode;
}
