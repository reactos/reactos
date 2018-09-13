class PAGE_DEF 
{
public:
    HWND                m_hDlg;
    PAGEID              m_nPageId;
    int                 m_nDlgRsrcId;
    HPROPSHEETPAGE      m_hpsp;

    static BOOL         m_bDisplayingMoreInfo;
    BOOL                m_bMoreInfoPage;
    PAGEID              m_nMoreInfo_PageId;

    // String resources
    int                 m_nHeaderTitleStrId;
    int                 m_nHeaderSubTitleStrId;

    virtual int DlgProc(HWND, UINT, WPARAM, LPARAM);
    virtual PAGEID GetNextPage(BOOL bCalcNextPage = FALSE);
    virtual void CreatePropPage();

    PAGE_DEF(
        PAGEID      nPageId,
        int         nDlgRsrcId,
        PAGEID      nMoreInfo_PageId,
        int         nHeaderTitleStrId,
        int         nHeaderSubTitleStrId,
        BOOL        bMoreInfoPage = FALSE);
};


class TEXT_ONLY_PAGE_DEF : public PAGE_DEF 
{
public:
    PAGEID      m_nNextPageId;

    int         m_nMainTextStrId;    

    virtual int DlgProc(HWND, UINT, WPARAM, LPARAM);
    virtual PAGEID GetNextPage(BOOL bCalcNextPage = FALSE);

    TEXT_ONLY_PAGE_DEF(
        PAGEID      nPageId,
        int         nDlgRsrcId,
        PAGEID      nMoreInfo_PageId,
        int         nHeaderTitleStrId,
        int         nHeaderSubTitleStrId,
        int         nTextStrId,
        PAGEID      nNextPageId,
        BOOL        bMoreInfoPage = FALSE);
};


class WELCOME_PAGE_DEF : public TEXT_ONLY_PAGE_DEF 
{
public:
    virtual int DlgProc(HWND, UINT, WPARAM, LPARAM);

    WELCOME_PAGE_DEF(
        PAGEID      nPageId,
        int         nDlgRsrcId,
        PAGEID      nMoreInfo_PageId,
        int         nHeaderTitleStrId,
        int         nHeaderSubTitleStrId,
        int         nTextStrId,
        PAGEID      nNextPageId,
        BOOL        bMoreInfoPage = FALSE);
};

class DISPLAY_SUMMARY_INFO_PAGE_DEF : public TEXT_ONLY_PAGE_DEF 
{
public:
    virtual int DlgProc(HWND, UINT, WPARAM, LPARAM);

    DISPLAY_SUMMARY_INFO_PAGE_DEF(
        PAGEID      nPageId,
        int         nDlgRsrcId,
        PAGEID      nMoreInfo_PageId,
        int         nHeaderTitleStrId,
        int         nHeaderSubTitleStrId,
        int         nTextStrId,
        PAGEID      nNextPageId,
        BOOL        bMoreInfoPage = FALSE);
};



class SELECT_PORT_BAUD_PAGE_DEF : public TEXT_ONLY_PAGE_DEF
{
public:
    static COMPORT_INFO     m_rgComPortInfo[100];
    static DWORD            m_dwNumComPortsFound;
    // LoWord is index into m_rgComPortInfo
    // HiWord is the bit flag that indicates the speed
    static DWORD            m_dwSelectedCommPort;


    virtual int DlgProc(HWND, UINT, WPARAM, LPARAM);
    virtual PAGEID GetNextPage(BOOL bCalcNextPage = FALSE);

    SELECT_PORT_BAUD_PAGE_DEF(
        PAGEID      nPageId,
        int         nDlgRsrcId,
        PAGEID      nMoreInfo_PageId,
        int         nHeaderTitleStrId,
        int         nHeaderSubTitleStrId,
        int         nTextStrId,
        PAGEID      nNextPageId,
        BOOL        bMoreInfoPage = FALSE);
};


class SELECT_PORT_BAUD_PIPE_COMPNAME_PAGE_DEF : public SELECT_PORT_BAUD_PAGE_DEF
{
public:
    BOOL m_bSerial;
    PSTR m_pszCompName;
    PSTR m_pszPipeName;

    virtual int DlgProc(HWND, UINT, WPARAM, LPARAM);
    virtual void GrayDlgItems();

    SELECT_PORT_BAUD_PIPE_COMPNAME_PAGE_DEF(
        PAGEID      nPageId,
        int         nDlgRsrcId,
        PAGEID      nMoreInfo_PageId,
        int         nHeaderTitleStrId,
        int         nHeaderSubTitleStrId,
        int         nTextStrId,
        PAGEID      nNextPageId,
        BOOL        bMoreInfoPage = FALSE);
};


class HELP_TEXT_ONLY_PAGE_DEF : public TEXT_ONLY_PAGE_DEF
{
public:
    HELP_TEXT_ONLY_PAGE_DEF(
        PAGEID      nPageId,
        int         nHeaderTitleStrId,
        int         nHeaderSubTitleStrId,
        int         nTextStrId,
        PAGEID      nNextPageId);
};


class BROWSE_PATH_PAGE_DEF : public TEXT_ONLY_PAGE_DEF 
{
public:
    PSTR m_pszPath;

    virtual int DlgProc(HWND, UINT, WPARAM, LPARAM);
    virtual PAGEID GetNextPage(BOOL bCalcNextPage = FALSE);

    BROWSE_PATH_PAGE_DEF(
        PAGEID      nPageId,
        int         nDlgRsrcId,
        PAGEID      nMoreInfo_PageId,
        int         nHeaderTitleStrId,
        int         nHeaderSubTitleStrId,
        int         nTextStrId,
        PAGEID      nNextPageId);
};


class DESKTOP_SHORTCUT_PAGE_DEF : public BROWSE_PATH_PAGE_DEF 
{
public:
    virtual int DlgProc(HWND, UINT, WPARAM, LPARAM);
    virtual PAGEID GetNextPage(BOOL bCalcNextPage = FALSE);

    DESKTOP_SHORTCUT_PAGE_DEF(
        PAGEID      nPageId,
        int         nDlgRsrcId,
        PAGEID      nMoreInfo_PageId,
        int         nHeaderTitleStrId,
        int         nHeaderSubTitleStrId,
        int         nTextStrId,
        PAGEID      nNextPageId);
};



class COPY_SYMS_PAGE_DEF : public TEXT_ONLY_PAGE_DEF 
{
public:
    enum { nMAX_NUM_MSGS = 4 };

    PSTR            m_rgpszMsgs[nMAX_NUM_MSGS];

    virtual int DlgProc(HWND, UINT, WPARAM, LPARAM);

    BOOL Copy(
        HWND hwnd,
        PSTR pszDest,
        PSTR pszSrc,
        PSTR pszVolumeName,
        DWORD dwSerialNumber,
        UINT uDriveType);

    COPY_SYMS_PAGE_DEF(
        PAGEID      nPageId,
        int         nDlgRsrcId,
        PAGEID      nMoreInfo_PageId,
        int         nHeaderTitleStrId,
        int         nHeaderSubTitleStrId,
        int         nTextStrId,
        PAGEID      nNextPageId);

    virtual ~COPY_SYMS_PAGE_DEF();
};


class ADV_COPY_SYMS_PAGE_DEF : public PAGE_DEF 
{
public:
    BOOL        m_bCopySymbols;
    PAGEID      m_nNextPageId;

    virtual int DlgProc(HWND, UINT, WPARAM, LPARAM);
    virtual PAGEID GetNextPage(BOOL bCalcNextPage = FALSE);
    virtual void GrayDlgItems();


    ADV_COPY_SYMS_PAGE_DEF(
        PAGEID      nPageId,
        int         nDlgRsrcId,
        PAGEID      nMoreInfo_PageId,
        int         nHeaderTitleStrId,
        int         nHeaderSubTitleStrId,
        PAGEID      nNextPageId);
};



template<const DWORD m_dwNumOptions>
class MULTI_OPT_PAGE_DEF : public PAGE_DEF 
{
public:
    int         m_nMainTextStrId;

    int         m_nOptDefault;

    PAGEID      m_rgnOpts_PageId[m_dwNumOptions];
    int         m_rgnOpts_StrId[m_dwNumOptions];
    int         m_rgnOpts_CtrlId[m_dwNumOptions];

    virtual int DlgProc(HWND, UINT, WPARAM, LPARAM);
    virtual PAGEID GetNextPage(BOOL bCalcNextPage = FALSE);

    MULTI_OPT_PAGE_DEF(
        PAGEID      nPageId,
        int         nDlgRsrcId,
        PAGEID      nMoreInfo_PageId,
        int         nHeaderTitleStrId,
        int         nHeaderSubTitleStrId,
        
        int         nMainTextStrId,

        int         nOptDefault,

        int         rgnOpt_StrId[],
        UINT        uStrIdSize,
        PAGEID      rgnOpt_PageId[],
        UINT        uPageIdSize);
};


template<const DWORD m_dwNumOptions>
class MULTI_OPT_BROWSE_PATH_PAGE_DEF : public MULTI_OPT_PAGE_DEF<m_dwNumOptions>
{
public:
    PSTR    m_pszPath;

    virtual int DlgProc(HWND, UINT, WPARAM, LPARAM);
    virtual void GrayDlgItems();
    virtual PAGEID GetNextPage(BOOL bCalcNextPage = FALSE);

    PAGEID GetDriveInfo(
        BOOL bCalcNextPage,
        PSTR & pszPath,
        PSTR & pszVolumeLabel,
        DWORD & dwSerialNumber,
        UINT & uDriveType
        );

    MULTI_OPT_BROWSE_PATH_PAGE_DEF(
        PAGEID      nPageId,
        int         nDlgRsrcId,
        PAGEID      nMoreInfo_PageId,
        int         nHeaderTitleStrId,
        int         nHeaderSubTitleStrId,
        
        int         nMainTextStrId,

        int         nOptDefault,

        int         rgnOpt_StrId[],
        UINT        uStrIdSize,
        PAGEID      rgnOpt_PageId[],
        UINT        uPageIdSize);
};


