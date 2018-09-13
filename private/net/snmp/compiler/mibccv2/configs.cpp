#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <iostream.h>
#include <fstream.h>
#include <afx.h>
#include <afxtempl.h>
#include <objbase.h>
#include <afxwin.h>
#include <afxole.h>
#include <afxmt.h>
#include <wchar.h>
#include <process.h>
#include <objbase.h>
#include <initguid.h>

#include "Configs.hpp"
#include "Debug.hpp"

Configs theConfigs;

Configs::Configs()
{
	m_pszOutputFilename = NULL;
	m_nMaxErrors = CFG_DEF_MAXERRORS;
	m_dwFlags = CFG_PRINT_LOGO;

}

Configs::~Configs()
{
	if (m_pszOutputFilename != NULL)
		delete m_pszOutputFilename;
}