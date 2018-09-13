#include <stdafx.h>
#include "Errors.h"
#include "Parser.h"
#include "Registry.h"
#include "EventCmd.h"

COperation::COperation(tOperation operation)
{
    m_Operation = operation;
    m_pNextOperation = NULL;
}

COperation::~COperation()
{
    if (m_pNextOperation)
        delete m_pNextOperation;
}

COperation* COperation::Insert(COperation *pOperation)
{
    if (pOperation->m_Operation < m_Operation)
    {
        pOperation->m_pNextOperation = this;
        return pOperation;
    }

    if (m_pNextOperation == NULL)
        m_pNextOperation = pOperation;
    else
        m_pNextOperation = m_pNextOperation->Insert(pOperation);

    return this;
}

COperation* COperation::GetNextOp()
{
    return m_pNextOperation;
}

DWORD COperation::CheckedStrToDword(DWORD & dwDword, char *szDword)
{
    dwDword = 0;
    while (*szDword != '\0')
    {
        if (*szDword < '0' || *szDword > '9')
            return ERROR_BAD_FORMAT;
        dwDword *= 10;
        dwDword += (*szDword++ - '0');
    }

    return ERROR_SUCCESS;
}

COpEvents::COpEvents(tOperation operation) : COperation(operation)
{
    m_szEventSource = NULL;
    m_dwEventID = 0;
    m_dwCount = 1;
    m_dwTime = 0;
}

COpEvents::~COpEvents()
{
    if (m_szEventSource != NULL)
        delete m_szEventSource;
}

DWORD COpEvents::ParseCmdArgs()
{
    DWORD retCode;
    char szToken[INPUT_TOKEN_SZ];

    if ((retCode = gParser.GetNextToken(szToken, INPUT_TOKEN_SZ)) != ERROR_SUCCESS ||
        (retCode = gParser.CheckUnGetToken(KEYWORD_PRAGMA, szToken)) != ERROR_SUCCESS)
    {
        _W(WARN_ALERT, IDS_ALRT_WRN16,
           gParser.m_nLineNo,
           gParser.m_nTokenNo);
        return retCode;
    }

    if ((retCode = gParser.GetNextToken(szToken, INPUT_TOKEN_SZ)) != ERROR_SUCCESS ||
        (retCode = gParser.CheckUnGetToken(KEYWORD_PRAGMA, szToken)) != ERROR_SUCCESS)
    {
        _W(WARN_ALERT, IDS_ALRT_WRN17,
           gParser.m_nLineNo,
           gParser.m_nTokenNo);
        return retCode;
    }
    if (m_szEventSource != NULL)
        delete m_szEventSource;
    m_szEventSource = new char[sizeof(szToken) + 1];
    strcpy(m_szEventSource, szToken);

    if ((retCode = gParser.GetNextToken(szToken, INPUT_TOKEN_SZ)) != ERROR_SUCCESS ||
        (retCode = gParser.CheckUnGetToken(KEYWORD_PRAGMA, szToken)) != ERROR_SUCCESS ||
        (retCode = CheckedStrToDword(m_dwEventID, szToken)) != ERROR_SUCCESS)
    {
        _W(WARN_ALERT, IDS_ALRT_WRN18,
           gParser.m_nLineNo,
           gParser.m_nTokenNo);
        return retCode;
    }

    if ((retCode = gParser.GetNextToken(szToken, INPUT_TOKEN_SZ)) == ERROR_SUCCESS)
    {
        if (gParser.CheckUnGetToken(KEYWORD_PRAGMA, szToken) != ERROR_SUCCESS)
            goto done;

        if ((retCode = CheckedStrToDword(m_dwCount, szToken)) != ERROR_SUCCESS)
        {
            _W(WARN_ALERT, IDS_ALRT_WRN19,
               gParser.m_nLineNo,
               gParser.m_nTokenNo);
            return retCode;
        }
    }

    if ((retCode = gParser.GetNextToken(szToken, INPUT_TOKEN_SZ)) == ERROR_SUCCESS)
    {
        if (gParser.CheckUnGetToken(KEYWORD_PRAGMA, szToken) != ERROR_SUCCESS)
            goto done;

        if ((retCode = CheckedStrToDword(m_dwTime, szToken)) != ERROR_SUCCESS)
        {
            _W(WARN_ALERT, IDS_ALRT_WRN20,
               gParser.m_nLineNo,
               gParser.m_nTokenNo);
            return retCode;
        }
    }

    if (m_dwCount <= 1 && m_dwTime > 0)
    {
        _W(WARN_ALERT, IDS_ALRT_WRN21,
           gParser.m_nLineNo,
           gParser.m_nTokenNo,
           m_dwTime);
        m_dwTime = 0;
    }

    if (m_dwCount >= 2 && m_dwTime == 0)
    {
        _W(WARN_ALERT, IDS_ALRT_WRN22,
           gParser.m_nLineNo,
           gParser.m_nTokenNo,
           m_dwCount,
           m_dwTime);
    }

done:
    retCode = ERROR_SUCCESS;

    if (m_dwCount >= 2 && m_dwTime == 0)
        m_dwTime = 1;

    _W(WARN_ATTENTION, IDS_ALRT_WRN23,
        m_Operation == OP_ADD_EVENT ? "ADD" : "DELETE",
        m_szEventSource,
        m_dwEventID,
        m_dwCount,
        m_dwTime);

    return retCode;
};

DWORD COpEvents::ProcessCommand()
{
    DWORD retCode;

    switch(m_Operation)
    {
    case OP_ADD_EVENT:
        retCode = gRegistry.AddEvent(m_szEventSource, m_dwEventID, m_dwCount, m_dwTime);
        break;
    case OP_DEL_EVENT:
        retCode = gRegistry.DelEvent(m_szEventSource, m_dwEventID);
        break;
    default:
        return _E(ERROR_INTERNAL_ERROR, IDS_ERR02, m_Operation);
    }

    return retCode;
}

COpTraps::COpTraps(tOperation operation) : COperation(operation)
{
    m_szCommunity = NULL;
    m_szAddress = NULL;
}

COpTraps::~COpTraps()
{
    if (m_szCommunity != NULL)
        delete m_szCommunity;
    if (m_szAddress != NULL)
        delete m_szAddress;
}

DWORD COpTraps::ParseCmdArgs()
{
    DWORD retCode;
    char szToken[INPUT_TOKEN_SZ];

    if ((retCode = gParser.GetNextToken(szToken, INPUT_TOKEN_SZ)) != ERROR_SUCCESS ||
        (retCode = gParser.CheckUnGetToken(KEYWORD_PRAGMA, szToken)) != ERROR_SUCCESS)
    {
        _W(WARN_ALERT, IDS_ALRT_WRN24,
               gParser.m_nLineNo,
               gParser.m_nTokenNo);
        return retCode;
    }
    if (m_szCommunity != NULL)
        delete m_szCommunity;
    m_szCommunity = new char[strlen(szToken)+1];
    if (m_szCommunity == NULL)
        return _E(ERROR_OUTOFMEMORY, IDS_ERR01);
    strcpy(m_szCommunity, szToken);

    if ((retCode = gParser.GetNextToken(szToken, INPUT_TOKEN_SZ)) != ERROR_SUCCESS ||
        (retCode = gParser.CheckUnGetToken(KEYWORD_PRAGMA, szToken)) != ERROR_SUCCESS)
    {
        _W(WARN_ALERT, IDS_ALRT_WRN25,
           gParser.m_nLineNo,
           gParser.m_nTokenNo);
    }
    if (m_szAddress != NULL)
        delete m_szAddress;
    m_szAddress = new char [strlen(szToken) + 1];
    if (m_szAddress == NULL)
        return _E(ERROR_OUTOFMEMORY, IDS_ERR01);
    strcpy(m_szAddress, szToken);

    _W(WARN_ATTENTION, IDS_ATTN_WRN26,
        m_Operation == OP_ADD_TRAP ? "ADD_TRAP_DEST" : "DELETE_TRAP_DEST",
        m_szCommunity,
        m_szAddress);
    
    return ERROR_SUCCESS;
}

DWORD COpTraps::ProcessCommand()
{
    DWORD retCode;

    switch(m_Operation)
    {
    case OP_ADD_TRAP:
        retCode = gRegistry.AddTrap(m_szCommunity, m_szAddress);
        break;
    case OP_DEL_TRAP:
        retCode = gRegistry.DelTrap(m_szCommunity, m_szAddress);
        break;
    default:
        return _E(ERROR_INTERNAL_ERROR, IDS_ERR03, m_Operation);
    }

    return retCode;
}
