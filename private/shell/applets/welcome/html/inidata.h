// IniData.h: interface for the CRegData class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _MENUDATA_H_
#define _MENUDATA_H_

#define WELCOME_INI TEXT("welcome.ini")

class CIniData  
{
public:
    class CDataItem
    {
    public:
        TCHAR * title;
        TCHAR * menuname;
        TCHAR * description;
        TCHAR * cmdline;
        TCHAR * args;
        // flags
        //
        // This var is a bit mask of the following values
        //  PERUSER     True if item must be completed on a per user basis
        //              False if it's per machine
        //  ADMINONLY   True if this item can only be run by an admin
        //              False if all users should do this
        //  REQUIRED    IF true, this item must be completed before the "don't run this
        //              again" checkbox will be displayed.
        DWORD   flags;
        int     imgindex;
        bool    completed;

        CDataItem()
        {
            title = menuname = description = cmdline = args = NULL;
            flags = 0;
            completed = false;
        }
        ~CDataItem()
        {
            if ( title )
                delete [] title;
            if ( menuname )
                delete [] menuname;
            if ( description )
                delete [] description;
            if ( cmdline )
                delete [] cmdline;
            if ( args )
                delete [] args;
        }
    };
    enum {
        WF_PERUSER = 1,         // item is per user as opposed to per machine
        WF_ADMINONLY = 4,       // only show item if user is an admin
        WF_ALTERNATECOLOR = 8,  // show menu item text in the "visited" color
    };

    CDataItem   m_data[7];
    int         m_iItems;
    int         m_iCurItem;
    bool        m_bCheckState;  // initial value of the checkbox

    CIniData();
    ~CIniData();
    void Init();
    CDataItem & operator [] ( int i );
    void HackHackPreventRunningTwice();
    void UpdateAndRun( int i );
    void SetRunKey( DWORD dwData );

protected:
    void Add( LPTSTR szTitle, LPTSTR szMenu, LPTSTR szDesc, LPTSTR szCmd, LPTSTR szArgs, DWORD dwFlags, int iImgIndex );
    void InitalizePostSetupConfig();
    void ReadItemsFromINIFile();
    void RemoveCompletedItems();
    void SetCheckboxState();
};

#endif
