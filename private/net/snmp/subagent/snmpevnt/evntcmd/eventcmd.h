#ifndef _EVENTCMD_H
#define _EVENTCMD_H

#define CMDLINE_DELIM               "-/"
#define CMDLINE_OPTION_HELP         "hH?"
#define CMDLINE_OPTION_VERBOSE      "vV"
#define CMDLINE_OPTION_SYSTEM       "sS"
#define CMDLINE_OPTION_NORESTART    "nN"

#define CMDLINE_FLG_NORESTART       1

#define IDS_MSG_HELP                100
#define IDS_MSG_CPYRGHT             101

#define IDS_CHKP_WRN01              111
#define IDS_CHKP_WRN02              112
#define IDS_CHKP_WRN03              113
#define IDS_CHKP_WRN04              114
#define IDS_ALRT_WRN05              115
#define IDS_ATTN_WRN06              116
#define IDS_CHKP_WRN07              117
#define IDS_CHKP_WRN08              118
#define IDS_CHKP_WRN09              119
#define IDS_ATTN_WRN10              120
#define IDS_ATTN_WRN11              121
#define IDS_ALRT_WRN12              122
#define IDS_ALRT_WRN13              123
#define IDS_ATTN_WRN14              124
#define IDS_ALRT_WRN15              125
#define IDS_ALRT_WRN16              126
#define IDS_ALRT_WRN17              127
#define IDS_ALRT_WRN18              128
#define IDS_ALRT_WRN19              129
#define IDS_ALRT_WRN20              130
#define IDS_ALRT_WRN21              131
#define IDS_ALRT_WRN22              132
#define IDS_ALRT_WRN23              133
#define IDS_ALRT_WRN24              134
#define IDS_ALRT_WRN25              135
#define IDS_ATTN_WRN26              136
#define IDS_TRCK_WRN27              137
#define IDS_TRCK_WRN28              138
#define IDS_ERRO_WRN29              139
#define IDS_ALRT_WRN30              140
#define IDS_ALRT_WRN31              141
#define IDS_ALRT_WRN32              142
#define IDS_TRCK_WRN33              143
#define IDS_ATTN_WRN34              144
#define IDS_ATTN_WRN35              145
#define IDS_ATTN_WRN36              146
#define IDS_ATTN_WRN37              147
#define IDS_ATTN_WRN38              148
#define IDS_TRCK_WRN39              149
#define IDS_TRCK_WRN40              150
#define IDS_TRCK_WRN41              151
#define IDS_ATTN_WRN42              152
#define IDS_ATTN_WRN43              153
#define IDS_ATTN_WRN44              154
#define IDS_ATTN_WRN45              155
#define IDS_ERRO_WRN46              156
#define IDS_TRCK_WRN47              157
#define IDS_TRCK_WRN48              158
#define IDS_TRCK_WRN49              159
#define IDS_TRCK_WRN50              160
#define IDS_ERR01               500
#define IDS_ERR02               501
#define IDS_ERR03               503
#define IDS_ERR04               504
#define IDS_ERR05               505
#define IDS_ERR06               506
#define IDS_ERR07               507
#define IDS_ERR08               508
#define IDS_ERR09               509
#define IDS_ERR10               510
#define IDS_ERR11               511
#define IDS_ERR12               512
#define IDS_ERR13               513
#define IDS_ERR14               514
#define IDS_ERR15               515
#define IDS_ERR16               516
#define IDS_ERR17               517
#define IDS_ERR18               518
#define IDS_ERR19               519
#define IDS_ERR20               520
#define IDS_ERR21               521
#define IDS_ERR22               522
#define IDS_ERR23               523
#define IDS_ERR24               524
#define IDS_ERR25               525
#define IDS_ERR26               526
#define IDS_ERR27               527
#define IDS_ERR28               528
#define IDS_ERR29               529
#define IDS_ERR30               530
#define IDS_ERR31               531

class CCommandLine
{
public:
    char    *m_szFileName;
    char    *m_szSystem;
    DWORD   m_nVerboseLevel;
    DWORD   m_nFlags;

    CCommandLine();
    ~CCommandLine();

    DWORD ParseCmdLine(int argc, char *argv[]);
    DWORD GetVerboseLevel();
};

extern CString      gStrMessage;
extern CCommandLine gCommandLine;

#endif
