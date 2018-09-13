#include "pch.hxx" // pch
#pragma hdrstop

#include "resource.h"
#include "pgGenric.h"


CGenericWizPg::CGenericWizPg( 
    LPPROPSHEETPAGE ppsp,
	DWORD dwPageId,
	int nIdTitle /* = IDS_GENERICPAGETITLE */,
	int nIdSubTitle /* = IDS_GENERICPAGESUBTITLE */
    ) : WizardPage(ppsp, nIdTitle, nIdSubTitle)
{
	m_dwPageId = dwPageId;
    ppsp->pszTemplate = MAKEINTRESOURCE(m_dwPageId);
}


CGenericWizPg::~CGenericWizPg(
    VOID
    )
{
}



