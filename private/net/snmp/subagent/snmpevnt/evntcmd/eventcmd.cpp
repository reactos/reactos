#include <stdafx.h>
#include "EventCmd.h"
#include "Errors.h"
#include "Parser.h"
#include "Registry.h"
#include "SNMPCtrl.h"

CString      gStrMessage;
CCommandLine gCommandLine;

void PrintUsage(char *szCmdName)
{
    gStrMessage.LoadString(100);
    gStrMessage.AnsiToOem();
    printf("%s", gStrMessage);
}

void PrintBanner()
{
    gStrMessage.LoadString(101);
    gStrMessage.AnsiToOem();
    printf("%s", gStrMessage);
}

int _cdecl main(int argc, char *argv[])
{
    int retCode;
    PrintBanner();

    retCode = gCommandLine.ParseCmdLine(argc, argv);
    if (retCode != ERROR_SUCCESS)
        return retCode;
    _W(WARN_CHECKPOINT, IDS_CHKP_WRN01);

    retCode = gParser.ParseInputFile();
    if (retCode != ERROR_SUCCESS)
        return retCode;
    _W(WARN_CHECKPOINT, IDS_CHKP_WRN02, gCommandLine.m_szFileName);

    retCode = gRegistry.Connect();
    if (retCode != ERROR_SUCCESS)
        return retCode;
    _W(WARN_CHECKPOINT, IDS_CHKP_WRN03, gCommandLine.m_szSystem == NULL ? "localhost" : gCommandLine.m_szSystem);

    retCode = gParser.ProcessCommands();
    if (retCode != ERROR_SUCCESS)
        return retCode;
    _W(WARN_CHECKPOINT, IDS_CHKP_WRN04);

    if (gCommandLine.m_nFlags & CMDLINE_FLG_NORESTART)
    {
        if (gRegistry.m_dwFlags & REG_FLG_NEEDRESTART)
            _W(WARN_ALERT, IDS_ALRT_WRN05);
    }
    else
    {
        if (gRegistry.m_dwFlags & REG_FLG_NEEDRESTART)
        {
            if (!gSNMPController.IsSNMPRunning())
                _W(WARN_ATTENTION, IDS_ATTN_WRN06);

            else
            {
                _W(WARN_CHECKPOINT, IDS_CHKP_WRN07);
                retCode = gSNMPController.StopSNMP();
                if (retCode != ERROR_SUCCESS)
                    return retCode;

                _W(WARN_CHECKPOINT, IDS_CHKP_WRN08);
                retCode = gSNMPController.StartSNMP();
                if (retCode != ERROR_SUCCESS)
                    return retCode;

                _W(WARN_CHECKPOINT, IDS_CHKP_WRN09);
            }
        }
        else
            _W(WARN_ATTENTION, IDS_ATTN_WRN10);
    }

    return retCode;
}

CCommandLine::CCommandLine()
{
    m_szFileName = NULL;
    m_szSystem = NULL;
    m_nVerboseLevel = WARN_CHECKPOINT;
    m_nFlags = 0;
}

CCommandLine::~CCommandLine()
{
    if (m_szFileName)
        delete m_szFileName;
    if (m_szSystem)
        delete m_szSystem;
}

DWORD CCommandLine::ParseCmdLine(int argc, char *argv[])
{
    enum
    {
        STATE_ANY,
        STATE_ARG_VERBOSE,
        STATE_ARG_SYSTEM
    } state = STATE_ANY;

    for (int i=1; i<argc; i++)
    {
        switch(state)
        {
        case STATE_ANY:
            if (strchr(CMDLINE_DELIM,argv[i][0]) != NULL)
            {
                if (strchr(CMDLINE_OPTION_HELP, argv[i][1]) != NULL &&
                    argv[i][2] == '\0')
                {
                    PrintUsage(argv[0]);
                    return ERROR_NO_DATA;
                }
                if (strchr(CMDLINE_OPTION_VERBOSE, argv[i][1]) != NULL)
                {
                    if (argv[i][2] != '\0')
                    {
                        m_nVerboseLevel = atoi(argv[i]+2);
                        _W(WARN_ATTENTION,IDS_ATTN_WRN11, m_nVerboseLevel);
                    }
                    else
                        state = STATE_ARG_VERBOSE;
                    break;
                }
                if (strchr(CMDLINE_OPTION_SYSTEM, argv[i][1]) != NULL &&
                    argv[i][2] == '\0')
                {
                    state = STATE_ARG_SYSTEM;
                    break;
                }
                if (strchr(CMDLINE_OPTION_NORESTART, argv[i][1]) != NULL &&
                    argv[i][2] == '\0')
                {
                    m_nFlags |= CMDLINE_FLG_NORESTART;
                    break;
                }
                else
                    _W(WARN_ALERT,IDS_ALRT_WRN12, argv[i]);
            }
            else
            {
                if (m_szFileName != NULL)
                {
                    _W(WARN_ALERT,
                       IDS_ALRT_WRN13,
                       argv[i]);
                    delete m_szFileName;
                }
                m_szFileName = new char[strlen(argv[i])+1];
                if (m_szFileName == NULL)
                    return _E(ERROR_OUTOFMEMORY, IDS_ERR01);
                strcpy(m_szFileName, argv[i]);
            }
            break;
        case STATE_ARG_VERBOSE:
            m_nVerboseLevel = atoi(argv[i]);
            _W(WARN_ATTENTION,IDS_ATTN_WRN14, m_nVerboseLevel);
            state = STATE_ANY;
            break;
        case STATE_ARG_SYSTEM:
            if (m_szSystem != NULL)
            {
                _W(WARN_ALERT,
                   IDS_ALRT_WRN15,
                   argv[i]);
                delete m_szSystem;
            }
            m_szSystem = new char[strlen(argv[i])+1];
            if (m_szSystem == NULL)
                return _E(ERROR_OUTOFMEMORY, IDS_ERR01);
            strcpy(m_szSystem, argv[i]);
            state = STATE_ANY;
            break;
        }
    }

    if (m_szFileName == NULL)
    {
        PrintUsage(argv[0]);
        return ERROR_NO_DATA;
    }

    return ERROR_SUCCESS;
}

DWORD CCommandLine::GetVerboseLevel()
{
    return m_nVerboseLevel;
}
