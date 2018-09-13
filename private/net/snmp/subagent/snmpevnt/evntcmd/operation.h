#ifndef _EVENTOP_H
#define _EVENTOP_H

typedef enum
{
    OP_ADD_EVENT,
    OP_DEL_EVENT,
    OP_ADD_TRAP,
    OP_DEL_TRAP
} tOperation;

class COperation
{
protected:
    tOperation m_Operation;
    COperation *m_pNextOperation;

    DWORD CheckedStrToDword(DWORD & dwDword, char *szDword);
public:
    COperation(tOperation operation);
    virtual ~COperation();

    virtual DWORD   ParseCmdArgs() = 0;
    COperation*     Insert(COperation *pOperation);
    COperation*     GetNextOp();

    virtual DWORD   ProcessCommand() = 0;
};

class COpEvents : public COperation
{
    char    *m_szEventSource;
    DWORD   m_dwEventID;
    DWORD   m_dwCount;
    DWORD   m_dwTime;
public:
    COpEvents(tOperation operation);
    ~COpEvents();

    DWORD   ParseCmdArgs();
    DWORD   ProcessCommand();
};

class COpTraps : public COperation
{
    char    *m_szCommunity;
    char    *m_szAddress;
public:
    COpTraps(tOperation operation);
    ~COpTraps();

    DWORD   ParseCmdArgs();
    DWORD   ProcessCommand();
};

#endif
