// welcome.h: interface for the CDataSource class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _MENUDATA_H_
#define _MENUDATA_H_

#include "dataitem.h"

#define WELCOME_INI TEXT("welcome.ini")

class CDataSource
{
public:
    CDataItem   m_data[7];
    int         m_iItems;
    int         m_iCurItem;
    bool        m_bCheckState;  // initial value of the checkbox

    CDataSource();
    ~CDataSource();

    bool Init();
    CDataItem & operator [] ( int i );
    void Invoke( int i, HWND hwnd );
    void Uninit( DWORD dwData );

protected:
    void ReadItemsFromResources();
    void ReadItemsFromINIFile();
    void RemoveCompletedItems();
    void SetCheckboxState();
};

#endif
