#ifndef _DFCMD_H_
#define _DFCMD_H_

#include "docfind.h"

class CDFCommand;
class CDFFolder;

HRESULT CDFCommand_Create(CDFFolder *pdfc, IDocFindFileFilter *pdfff, void **ppvObj);

#endif