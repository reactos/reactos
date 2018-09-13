/*
 * Automatic language and codepage detector
 * 
 * Bob Powell, 2/97
 * Copyright (C) 1996, 1997, Microsoft Corp.  All rights reserved.
 * 
 *  History:    1-Feb-97    BobP      Created
 *              5-Aug-97    BobP      Unicode support; Charmaps in data file.
 */
#include "private.h"
/****************************************************************/



Histogram::Histogram (const PFileHistogramSection pHS, const PHIdx pMap)
: m_nDimensionality((UCHAR)pHS->m_dwDimensionality),
  m_nEdgeSize((UCHAR)pHS->m_dwEdgeSize),
  m_nCodePage((USHORT)pHS->m_dwCodePage),
  m_pMap(pMap),
  m_panElts((HElt *)&pHS[1])	// table follows header struct  in the file
{
	// #elements = #unique character values ^ #dimensions

	m_nElts = 1;
	for (UCHAR i = 0; i < m_nDimensionality; i++)
		m_nElts *= m_nEdgeSize;
}

DWORD
Histogram::Validate (DWORD nBytes) const
{
	if ( nBytes < m_nElts * sizeof(HElt) ||
		 m_nDimensionality > 4 )
	{
		return ERROR_INTERNAL_DB_CORRUPTION;
	}

	return NO_ERROR;
}

Histogram::Histogram (const Histogram &H, const PHIdx pMap)
: m_nDimensionality(H.m_nDimensionality),
  m_nEdgeSize(H.m_nEdgeSize),
  m_nCodePage(H.m_nCodePage),
  m_nElts(H.m_nElts),
  m_pMap(pMap),
  m_panElts(H.m_panElts)
//
// Clone a histogram but use a different Charmap.
{
}

Histogram::~Histogram (void)
//
// The pointer members point to the mapped file and do not need to be freed.
{
}

/****************************************************************/

Language::Language (PLCDetect pL, int nLangID, int nCodePages, int nRangeID)
: m_pLC(pL),
  m_nLangID(nLangID),
  m_nCodePages(nCodePages),
  m_nRangeID(nRangeID)
{
}

Language7Bit::Language7Bit (PLCDetect pL, int nLangID, int nCodePages)
: Language(pL, nLangID, nCodePages),
  m_pLangHistogram(NULL)
{
	memset ((void *)m_ppCodePageHistogram, 0, sizeof(m_ppCodePageHistogram));
}

Language7Bit::~Language7Bit (void)
{
	if (m_pLangHistogram)
		delete m_pLangHistogram;

	for (int i = 0; i < MAXSUBLANG; i++)
		if (m_ppCodePageHistogram[i])
			delete m_ppCodePageHistogram[i];
}

DWORD
Language7Bit::AddHistogram (PFileHistogramSection pHS, DWORD nBytes, int nIdx)
//
// Add the raw histogram at *pHS in the mapped file to this language object.  
// The histograms must be for 7-bit detection.
{
	DWORD hr = NO_ERROR;

	PHIdx pMap = m_pLC->GetMap( pHS->m_dwMappingID );

	if (nIdx == 0)
	{
		// The first histogram for a language is its language-detection table.

		if ( (m_pLangHistogram = new Histogram (pHS, pMap)) == NULL)
			return ERROR_OUTOFMEMORY;

		if ((hr = m_pLangHistogram->Validate (nBytes)) != NO_ERROR)
			return hr;
	}
	else
	{
		// Each subsequent histogram is a code page detection table.

		if (nIdx - 1 >= m_nCodePages)
			return ERROR_INTERNAL_DB_CORRUPTION;

		Histogram *pH;

		if ((pH = new Histogram (pHS, pMap)) == NULL)
			return ERROR_OUTOFMEMORY;

		if ((hr = pH->Validate (nBytes)) != NO_ERROR)
			return hr;

		m_ppCodePageHistogram[nIdx - 1] = pH;

		// Cache for the scoring vector math

		m_paHElt[nIdx - 1] = pH->Array();
	}

	return hr;
}

/****************************************************************/

Language8Bit::Language8Bit (PLCDetect pL, int nLangID, int nCodePages)
: Language(pL, nLangID, nCodePages)
{
	memset ((void *)m_ppHistogram, 0, sizeof(m_ppHistogram));
}

Language8Bit::~Language8Bit (void)
{
	for (int i = 0; i < MAXSUBLANG; i++)
		if (m_ppHistogram[i])
			delete m_ppHistogram[i];
}

DWORD
Language8Bit::AddHistogram (PFileHistogramSection pHS, DWORD nBytes, int nIdx)
//
// Add the raw histogram at *pHS to this language object.  
// This language is known to use 8-bit detection.
{
	DWORD hr = NO_ERROR;

	PHIdx pMap = m_pLC->GetMap( pHS->m_dwMappingID );

	// The histograms are the direct language-code page tables

	if (nIdx >= m_nCodePages)
		return ERROR_INTERNAL_DB_CORRUPTION;

	Histogram *pH;

	if ((pH = new Histogram (pHS, pMap)) == NULL)
		return ERROR_OUTOFMEMORY;

	if ((hr = pH->Validate (nBytes)) != NO_ERROR)
		return hr;

	m_ppHistogram[nIdx] = pH;

	return hr;
}

/****************************************************************/

LanguageUnicode::LanguageUnicode (PLCDetect pL, int nLangID, 
	int nSubLangs, int nRangeID)
: Language(pL, nLangID, nSubLangs, nRangeID)
{
	memset ((void *)m_ppSubLangHistogram, 0, sizeof(m_ppSubLangHistogram));
}

LanguageUnicode::~LanguageUnicode (void)
{
	for (int i = 0; i < MAXSUBLANG; i++)
		if (m_ppSubLangHistogram[i])
			delete m_ppSubLangHistogram[i];
}

DWORD
LanguageUnicode::AddHistogram (PFileHistogramSection pHS, DWORD nBytes, int nIdx)
{
	DWORD hr = NO_ERROR;

	// All histograms for are sublanguage detection

	if (nIdx >= m_nSubLangs)
		return ERROR_INTERNAL_DB_CORRUPTION;

	// Get the custom charmap used for scoring this sublanguage group

	PHIdx pMap = m_pLC->GetMap( pHS->m_dwMappingID );

	Histogram *pH;

	if ((pH = new Histogram (pHS, pMap)) == NULL)
		return ERROR_OUTOFMEMORY;

	if ((hr = pH->Validate (nBytes)) != NO_ERROR)
		return hr;

	m_ppSubLangHistogram[nIdx] = pH;

	m_paHElt[nIdx] = pH->Array();

	return hr;
}

/****************************************************************/

LCDetect::LCDetect (HMODULE hM)
: m_hModule(hM),
  m_nCharmaps(0),
  m_n7BitLanguages(0),
  m_n8BitLanguages(0),
  m_nUnicodeLanguages(0),
  m_n7BitLangsRead(0),
  m_n8BitLangsRead(0),
  m_nUnicodeLangsRead(0),
  m_nMapsRead(0),
  m_nHistogramsRead(0),
  m_nScoreIdx(0),
  m_pp7BitLanguages(NULL),
  m_pp8BitLanguages(NULL),
  m_ppUnicodeLanguages(NULL),
  m_ppCharmaps(NULL),
  m_pv(NULL),
  m_hmap(0),
  m_hf(0),
  m_pHU27Bit(0)
{
}

LCDetect::~LCDetect ()
{
	delete m_pHU27Bit;

	for (unsigned int i = 0; i < m_n7BitLanguages; i++)
		delete m_pp7BitLanguages[i];
	delete m_pp7BitLanguages;

	for (i = 0; i < m_n8BitLanguages; i++)
		delete m_pp8BitLanguages[i];
	delete m_pp8BitLanguages;

	for (i = 0; i < m_nUnicodeLanguages; i++)
		delete m_ppUnicodeLanguages[i];
	delete m_ppUnicodeLanguages;

	for (i = 0; i < m_nCharmaps; i++)
		delete m_ppCharmaps[i];
	delete m_ppCharmaps;

	UnmapViewOfFile (m_pv);
	CloseHandle (m_hmap);
	CloseHandle (m_hf);
}

DWORD
LCDetect::Initialize7BitLanguage (PFileLanguageSection pLS, PLanguage *ppL)
//
// Set *ppL to the Language object created from this section.
{
	// nRecordCount is lang histogram (1) + # of code page histograms

	if ( m_n7BitLangsRead >= m_n7BitLanguages || pLS->m_dwRecordCount < 1)
		return ERROR_INTERNAL_DB_CORRUPTION;

	PLanguage7Bit pL = new Language7Bit (this, pLS->m_dwLangID, pLS->m_dwRecordCount - 1);

	if (pL == NULL)
		return ERROR_OUTOFMEMORY;


	// Each 7-bit lang uses one score index slot per code page.
	// The range starts with the 7-bit langs, since both the 8-bit
	// and Unicode langs follow it.

	if (m_n7BitLangsRead == 0 && m_nScoreIdx != 0)
		return ERROR_INTERNAL_DB_CORRUPTION;;

	pL->SetScoreIdx(m_nScoreIdx);

	m_nScoreIdx += pLS->m_dwRecordCount - 1;	// skip 1st record (Language)

	m_pp7BitLanguages[ m_n7BitLangsRead++ ] = pL;

	*ppL = pL;

	return NO_ERROR;
}

DWORD
LCDetect::Initialize8BitLanguage (PFileLanguageSection pLS, Language **ppL)
//
// Set *ppL to the Language object created from this section.
{
	// nRecordCount is # of combined language / code page histograms

	if ( m_n8BitLangsRead >= m_n8BitLanguages || pLS->m_dwRecordCount < 1)
		return ERROR_INTERNAL_DB_CORRUPTION;

	PLanguage8Bit pL = new Language8Bit (this, pLS->m_dwLangID, pLS->m_dwRecordCount);

	if (pL == NULL)
		return ERROR_OUTOFMEMORY;


	// The 8-bit score indices follow the 7-bit languages

	// Each 8-bit lang uses a score index slot for each of its code pages,
	// since all the code pages are scored in the initial scoring pass.
	// The number of slots is the number of code page histograms, which is
	// one less than the number of records following this language.

	pL->SetScoreIdx(m_nScoreIdx);
	m_nScoreIdx += pLS->m_dwRecordCount;


	m_pp8BitLanguages[ m_n8BitLangsRead++ ] = pL;

	*ppL = pL;

	return NO_ERROR;
}

DWORD
LCDetect::InitializeUnicodeLanguage (PFileLanguageSection pLS, Language **ppL)
//
// Set *ppL to the Language object created from this section.
{
	// nRecordCount is # of sublanguage histograms

	if ( m_nUnicodeLangsRead >= m_nUnicodeLanguages ||
		 pLS->m_dwUnicodeRangeID >= m_nUnicodeLanguages )
	{
		return ERROR_INTERNAL_DB_CORRUPTION;
	}

	PLanguageUnicode pL = new LanguageUnicode (this, pLS->m_dwLangID, 
						pLS->m_dwRecordCount, pLS->m_dwUnicodeRangeID);

	if (pL == NULL)
		return ERROR_OUTOFMEMORY;


	// The Unicode score indices follow the 7-bit languages, and overlay the
	// 8-bit slots since they aren't used at the same time.

	if (m_nUnicodeLangsRead == 0 && GetN8BitLanguages() > 0)
		m_nScoreIdx = Get8BitLanguage(0)->GetScoreIdx();

	// Each Unicode entry uses exactly one score index.  SBCS subdetection
	// (Latin group) uses the slots for the corresponding 7-bit languages,
	// and Unicode subdetection (CJK) uses the slots already defined for the
	// Unicode sub-languages.

	pL->SetScoreIdx(m_nScoreIdx);

	m_nScoreIdx++;

	// For Unicode, the range ID is used as the Language array index.

	m_ppUnicodeLanguages[ pLS->m_dwUnicodeRangeID ] = pL;
	m_nUnicodeLangsRead++;

	*ppL = pL;

	return NO_ERROR;
}

DWORD
LCDetect::LoadLanguageSection (void *pv, int nSectionSize, PLanguage *ppL)
//
// A language section begins the definition of data for a language.
// Each language has exactly one of these records.  One or more
// histogram sections follow each language, and are always associated
// with the language of the preceding language section.
//
// Set *ppL to the Language object created from this section.
{
	DWORD hr = NO_ERROR;

	PFileLanguageSection pLS;

	pLS = (PFileLanguageSection)&((char *)pv)[sizeof(FileSection)];

	switch ( pLS->m_dwDetectionType ) {

	case DETECT_7BIT:
		hr = Initialize7BitLanguage (pLS, ppL);
		break;

	case DETECT_8BIT:
		hr = Initialize8BitLanguage (pLS, ppL);
		break;

	case DETECT_UNICODE:
		hr = InitializeUnicodeLanguage (pLS, ppL);
		break;
	}

	return hr;
}

DWORD
LCDetect::LoadHistogramSection (void *pv, int nSectionSize, Language *pL)
{
	PFileHistogramSection pHS;

	pHS = (PFileHistogramSection)&((char *)pv)[sizeof(FileSection)];

	int nBytes = nSectionSize - sizeof(FileSection) - sizeof(*pHS);

	return pL->AddHistogram ( pHS, nBytes, m_nHistogramsRead++);
}

DWORD
LCDetect::LoadMapSection (void *pv, int nSectionSize)
{
	PFileMapSection pMS;

	pMS = (PFileMapSection)&((char *)pv)[sizeof(FileSection)];

	int nBytes = nSectionSize - sizeof(FileSection) - sizeof(*pMS);

	if (m_nMapsRead >= m_nCharmaps)
		return ERROR_INTERNAL_DB_CORRUPTION;

	PCharmap pM = new Charmap (pMS);

	if (pM == NULL)
		return ERROR_OUTOFMEMORY;

	m_ppCharmaps[ m_nMapsRead++ ]  = pM;

	return NO_ERROR;
}

DWORD
LCDetect::BuildState (DWORD nFileSize)
//
// Build the detection structures from the mapped training file image at *m_pv
{
	PLanguage pL;
	PFileHeader pFH;
	PFileSection pFS;

	DWORD hr = NO_ERROR;

	// Validate header

	pFH = (PFileHeader) m_pv;

	if ( nFileSize < sizeof(*pFH) || 
		 pFH->m_dwAppSig != APP_SIGNATURE ||
		 pFH->m_dwVersion != APP_VERSION ||
		 pFH->m_dwHdrSizeBytes >= nFileSize ||
		 pFH->m_dwN7BitLanguages == 0 ||
		 pFH->m_dwN8BitLanguages == 0 ||
		 pFH->m_dwNUnicodeLanguages == 0 ||
		 pFH->m_dwNCharmaps == 0 )
	{
		return ERROR_INTERNAL_DB_CORRUPTION;
	}

	// Allocate language pointer table per header

	m_n7BitLanguages = pFH->m_dwN7BitLanguages;
	m_pp7BitLanguages = new PLanguage7Bit [m_n7BitLanguages];

	m_n8BitLanguages = pFH->m_dwN8BitLanguages;
	m_pp8BitLanguages = new PLanguage8Bit [m_n8BitLanguages];

	m_nUnicodeLanguages = pFH->m_dwNUnicodeLanguages;
	m_ppUnicodeLanguages = new PLanguageUnicode [m_nUnicodeLanguages];

	// Clear, because not all slots may be assigned
	memset (m_ppUnicodeLanguages, 0, sizeof(PLanguageUnicode) * m_nUnicodeLanguages);

	m_nCharmaps = pFH->m_dwNCharmaps;
	m_ppCharmaps = new PCharmap [m_nCharmaps];

	if ( m_pp7BitLanguages == NULL || 
		 m_pp8BitLanguages == NULL || 
		 m_ppUnicodeLanguages == NULL ||
		 m_ppCharmaps == NULL )
	{
		return ERROR_OUTOFMEMORY;
	}

	// Remember other header info

	m_LCDConfigureDefault.nMin7BitScore = pFH->m_dwMin7BitScore;
	m_LCDConfigureDefault.nMin8BitScore = pFH->m_dwMin8BitScore;
	m_LCDConfigureDefault.nMinUnicodeScore = pFH->m_dwMinUnicodeScore;
	m_LCDConfigureDefault.nRelativeThreshhold = pFH->m_dwRelativeThreshhold;
	m_LCDConfigureDefault.nDocPctThreshhold = pFH->m_dwDocPctThreshhold;
	m_LCDConfigureDefault.nChunkSize = pFH->m_dwChunkSize;

	// Position to first section

	pFS = (PFileSection) &((char *)m_pv)[pFH->m_dwHdrSizeBytes];

	// Read and process each file section

	while ( hr == NO_ERROR ) {

		// check alignment

		if (((DWORD_PTR)pFS & 3) != 0) {
			hr = ERROR_INTERNAL_DB_CORRUPTION;
			break;
		}

		// zero-length section marks end of data

		if (pFS->m_dwSizeBytes == 0)
			break;

		if ( &((char *)pFS)[pFS->m_dwSizeBytes] >= &((char *)m_pv)[nFileSize]) {
			hr = ERROR_INTERNAL_DB_CORRUPTION;
			break;
		}

		switch ( pFS->m_dwType ) {

		case SECTION_TYPE_LANGUAGE:								// sets pL
			hr = LoadLanguageSection ((void*)pFS, pFS->m_dwSizeBytes, &pL);
			m_nHistogramsRead = 0;
			break;

		case SECTION_TYPE_HISTOGRAM:							// uses pL
			hr = LoadHistogramSection ((void*)pFS, pFS->m_dwSizeBytes, pL);
			break;

		case SECTION_TYPE_MAP:
			hr = LoadMapSection ((void*)pFS, pFS->m_dwSizeBytes);
			break;

		default:					// ignore unrecognized sections
			break;
		}

		pFS = (PFileSection) &((char *)pFS)[pFS->m_dwSizeBytes];
	}

	if (hr != NO_ERROR)
		return hr;

	if ( m_nMapsRead != m_nCharmaps )
		return ERROR_INTERNAL_DB_CORRUPTION;


	// Set up quick-reference arrays used by the scoring inner loops

	for (unsigned int i = 0; i < GetN7BitLanguages(); i++)
		m_paHElt7Bit[i] = Get7BitLanguage(i)->GetLangHistogram()->Array();

	m_nHElt8Bit = 0;
	for (i = 0; i < GetN8BitLanguages(); i++) 
	{
		PLanguage8Bit pL = Get8BitLanguage(i);

		for (int j = 0; j < pL->NCodePages(); j++)
			m_paHElt8Bit[m_nHElt8Bit++] = pL->GetHistogram(j)->Array();
	}

	// Set up the Histogram used for ScoreVectorW() for scoring Unicode
	// text for 7-bit language detection.  Clone the first 7-bit language
	// histogram and replace its map with CHARMAP_U27BIT.

	m_pHU27Bit = new Histogram ( *Get7BitLanguage(0)->GetLangHistogram(),
								 GetMap(CHARMAP_U27BIT));

	return hr;
}


DWORD
LCDetect::LoadState (void)
//
// Overall initialization and state loading.  Open the compiled training
// file from its fixed location in the System32 directory, and assemble
// in-memory detection tables from its contents.
{
	DWORD hr = NO_ERROR;
	DWORD nFileSize;
#define MODULENAMELEN 100
	char szFilename[MODULENAMELEN+50], *p;

	// Find out if NT or Windows

	OSVERSIONINFOA OSVersionInfo;
	int nOSWinNT = 0;
	OSVersionInfo.dwOSVersionInfoSize = sizeof( OSVERSIONINFOA );
	if ( GetVersionExA( &OSVersionInfo ) )
		nOSWinNT = OSVersionInfo.dwPlatformId;

	// Open the training data file,
	// look in the directory that contains the DLL.

	if (GetModuleFileNameA (m_hModule, szFilename, MODULENAMELEN) == 0)
		return GetLastError();

	if ( (p = strrchr (szFilename, '\\')) != NULL ||
		 (p = strrchr (szFilename, ':')) != NULL )
	{
		*++p = 0;
	}
	else
		*szFilename = 0;
	strcat (szFilename, DETECTION_DATA_FILENAME);

    if ((m_hf = CreateFileA (szFilename, GENERIC_READ, FILE_SHARE_READ, 
                    NULL, OPEN_EXISTING, 
                    FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) 
    {
        return E_FAIL;
    }

	if ((nFileSize = GetFileSize (m_hf, NULL)) == 0xffffffff) {
		hr = GetLastError();
		CloseHandle (m_hf);
		return hr;
	}

	// Virtual-map the file

	if ( nOSWinNT == VER_PLATFORM_WIN32_NT )
		m_hmap = CreateFileMapping (m_hf, NULL, PAGE_READONLY, 0, nFileSize, NULL);
	else
		m_hmap = CreateFileMappingA (m_hf, NULL, PAGE_READONLY, 0, nFileSize, NULL);

	if (m_hmap == NULL) {
		hr = GetLastError();
		CloseHandle (m_hf);
		return hr;
	}

	if ((m_pv = MapViewOfFile (m_hmap, FILE_MAP_READ, 0, 0, 0 )) == NULL) {
		hr = GetLastError();
		CloseHandle (m_hmap);
		CloseHandle (m_hf);
		return hr;
	}
		
	// Build the in-memory structures from the file

	hr = BuildState (nFileSize);

	return hr;
}

/****************************************************************/
