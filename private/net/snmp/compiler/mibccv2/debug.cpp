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

#include <bool.hpp>
#include <nString.hpp>

#include <ui.hpp>
#include <symbol.hpp>
#include <type.hpp>
#include <value.hpp>
#include <valueRef.hpp>
#include <typeRef.hpp>
#include <oidValue.hpp>
#include <objType.hpp>
#include <objTypV1.hpp>
#include <objTypV2.hpp>
#include <objId.hpp>
#include <trapType.hpp>
#include <notType.hpp>
#include <group.hpp>
#include <notGroup.hpp>
#include <module.hpp>
#include <sValues.hpp>
#include <lex_yy.hpp>
#include <ytab.hpp>
#include <errorMsg.hpp>
#include <errorCon.hpp>
#include <scanner.hpp>
#include <parser.hpp>
#include <apTree.hpp>
#include <oidTree.hpp>
#include <pTree.hpp>

#include "Debug.hpp"
#include "Configs.hpp"

SIMCErrorContainer errorContainer;
extern Configs theConfigs;

void _coreASSERT(const char *filename, int line, const char *errMsg, void (*cleanup)())
{
	const char *name;
	name = strrchr(filename, '\\');
	if (name != NULL)
		filename = name+1;
	cout << "Err [" << filename << ":" << line << "] - " << errMsg << "\n";
	if (cleanup != NULL)
		(*cleanup)();
}

void dumpOnBuild()
{
	SIMCErrorMessage errorMessage;

	if ( (theConfigs.m_dwFlags & (CFG_VERB_ERROR | CFG_VERB_WARNING)) == 0 )
		return;
	for (errorContainer.MoveToFirstMessage();
	     errorContainer.GetNextMessage(errorMessage) && theConfigs.m_nMaxErrors > 0;
		)
	{
		switch (errorMessage.GetSeverityLevel())
		{
		case 0:
			if (theConfigs.m_dwFlags & CFG_VERB_ERROR)
			{
				cout << errorMessage;
				theConfigs.m_nMaxErrors--;
			}
			break;
		case 1:
			if (theConfigs.m_dwFlags & CFG_VERB_WARNING)
			{
				cout << errorMessage;
				theConfigs.m_nMaxErrors--;
			}
			break;
		}
	}
}