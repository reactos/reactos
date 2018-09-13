/*
 * Automatic language and codepage detector
 * 
 * Bob Powell, 2/97
 * Copyright (C) 1996, 1997, Microsoft Corp.  All rights reserved.
 */

#ifdef  __cplusplus

#include <wtypes.h>
#include <limits.h>

#include "lcdetect.h"
#include "lccommon.h"

#include "tqsort.h"


// Turn this on in SOURCES to enable debug output
#ifdef DEBUG_LCDETECT
#include <stdio.h>
extern int g_fDebug;
#define debug(x) { if (g_fDebug) { x; }}
#define unmapch(x) ((x) >= 2 ? (x)+'a'-2 : ' ')
#else
#define debug(x)
#endif

class LCDetect;
typedef LCDetect *PLCDetect;

class Language;
class Language7Bit;
class Language8Bit;
class LanguageUnicode;
typedef Language *PLanguage;
typedef Language7Bit *PLanguage7Bit;
typedef Language8Bit *PLanguage8Bit;
typedef LanguageUnicode *PLanguageUnicode;

class CScore;
class CScores;

/****************************************************************/

#define MAXSCORES 50			// Max possible simultaneous # of scores

#define MINRAWSCORE 100			// Score threshhold (weight * char count) 
								// for further processing

/****************************************************************/

// Histograms

// A histogram stores an array of n-gram occurrence counts.  
// HElt stores the count, at present this is an unsigned char.

// The in-memory structure is similar to the file.
// The histogram array pointers m_panElts point into the mapped file image.

class Histogram {

public:
	Histogram (const PFileHistogramSection pHS, const PHIdx pMap);
	Histogram (const Histogram &H, const PHIdx pMap);
	virtual ~Histogram (void);

	DWORD Validate (DWORD nBytes) const;

	UCHAR Dimensionality (void) { return m_nDimensionality; }
	UCHAR EdgeSize (void) { return m_nEdgeSize; }
	USHORT CodePage (void) { return m_nCodePage; }
	USHORT GetRangeID (void) { return m_nRangeID; }
	USHORT NElts (void) { return m_nElts; }
	PHIdx GetMap (void) { return m_pMap; }

	HElt Ref (USHORT i1) const { return m_panElts[i1]; }
	HElt Ref (UCHAR i1, UCHAR i2) const {
		return m_panElts[(i1 * m_nEdgeSize) + i2]; }
	HElt Ref (UCHAR i1, UCHAR i2, UCHAR i3) const {
		return m_panElts[((i1 * m_nEdgeSize) + i2) * m_nEdgeSize + i3]; }

	HElt *Array (void) { return m_panElts; }

protected:
	UCHAR m_nDimensionality;		// 1=unigram, 2=digram etc.
	UCHAR m_nEdgeSize;				// edge size (is a function of char map)
	union {
		USHORT m_nCodePage;			// For 7 and 8-bit, is code page
		USHORT m_nRangeID;			// For Unicode, is sub-language range ID
	};
	USHORT m_nElts;					// (edge size ^ dimensionality)
	PHIdx m_pMap;					// char/WCHAR to histogram idx mapping

	HElt *m_panElts;				// array of elements / counts
};
typedef Histogram *PHistogram;

/****************************************************************/

// A Language object stores all the detection state for a given language,
// i.e. primary language ID.

class Language {
public:
	// nCodePages is same as nSubLangs
	Language (PLCDetect pL, int nLangID, int nCodePages, int nRangeID = 0);
	virtual ~Language (void) { }

	virtual DWORD AddHistogram (PFileHistogramSection pHS, DWORD nBytes, int nIdx) = 0;

	// Score the code pages for this language
	virtual void ScoreCodePage (LPCSTR, int nCh, CScore &S, int &idx) const;

	int LanguageID (void) const { return m_nLangID; }
	int NCodePages (void) const { return m_nCodePages; }
	int NSubLangs (void) const { return m_nSubLangs; }
	int RangeID (void) const { return m_nRangeID; }
	int GetScoreIdx (void) const { return m_nScoreIdx; }
	void SetScoreIdx (int nScoreIdx) { m_nScoreIdx = nScoreIdx; }

	virtual int GetCodePage (int n) const { return 0; }
	virtual int GetSublangRangeID (int n) const { return 0; }
	virtual int GetSublangID (int n) const { return 0; }

	virtual DetectionType Type (void) = 0;
	virtual Language7Bit const * GetLanguage7Bit (void) const { return NULL; }
	virtual Language8Bit const * GetLanguage8Bit (void) const { return NULL; }
	virtual LanguageUnicode const * GetLanguageUnicode (void) const { return NULL; }

protected:
	PLCDetect m_pLC;

	int m_nLangID;		// Win32 primary language ID
	int m_nRangeID;		// Unicode range ID, for Unicode langs
	union {
		int m_nCodePages;	// # of code pages trained for this language
		int m_nSubLangs;
	};
	int m_nScoreIdx;	// Used to create a unique index into the score arrays
						// for each lang + cp combination, to eliminate the
						// need to search the arrays to merge scores.  Add
						// the code page index to this to get the array index.
};

////////////////////////////////////////////////////////////////

class Language7Bit : public Language {
public:
	Language7Bit (PLCDetect pL, int nLangID, int nCodePages);
	~Language7Bit (void);

	DWORD AddHistogram (PFileHistogramSection pHS, DWORD nBytes, int nIdx);

	void ScoreCodePage (LPCSTR, int nCh, CScore &S, int &idx) const;

	int GetCodePage (int n) const { return m_ppCodePageHistogram[n]->CodePage();}
	virtual DetectionType Type (void) { return DETECT_7BIT; }

	PHistogram GetLangHistogram (void) const { return m_pLangHistogram; }
	PHistogram GetCodePageHistogram (int i) const { 
		return m_ppCodePageHistogram[i]; }

	virtual Language7Bit const * GetLanguage7Bit (void) const { return this; }

	const PHElt * GetPHEltArray (void) const { return m_paHElt; }

private:
	PHistogram m_pLangHistogram;
	PHistogram m_ppCodePageHistogram[MAXSUBLANG];

	PHElt m_paHElt[MAXSUBLANG];
};

////////////////////////////////////////////////////////////////

class Language8Bit : public Language {
public:
	Language8Bit (PLCDetect pL, int nLangID, int nCodePages);
	~Language8Bit (void);

	DWORD AddHistogram (PFileHistogramSection pHS, DWORD nBytes, int nIdx);

	int GetCodePage (int n) const { return m_ppHistogram[n]->CodePage(); }

	virtual DetectionType Type (void) { return DETECT_8BIT; }

	PHistogram GetHistogram (int i) const { return m_ppHistogram[i]; }

	virtual Language8Bit const * GetLanguage8Bit (void) const { return this; }

private:
	PHistogram m_ppHistogram[MAXSUBLANG];
};

////////////////////////////////////////////////////////////////

class LanguageUnicode : public Language {
public:
	LanguageUnicode (PLCDetect pL, int nLangID, int nRecordCount, int nRangeID);
	~LanguageUnicode (void);
	
	DWORD AddHistogram (PFileHistogramSection pHS, DWORD nBytes, int nIdx);
	
	void ScoreSublanguages (LPCWSTR wcs, int nch, CScores &S) const;

	int GetSublangRangeID (int i) const{return GetHistogram(i)->GetRangeID();}
	PLanguageUnicode GetSublanguage (int n) const;

	virtual DetectionType Type (void) { return DETECT_UNICODE; }

	PHistogram GetHistogram (int i) const { return m_ppSubLangHistogram[i]; }

	virtual LanguageUnicode const * GetLanguageUnicode (void) const { 
		return this; 
	}

	const PHElt * GetPHEltArray (void) const { return m_paHElt; }

private:
	PHistogram m_ppSubLangHistogram[MAXSUBLANG];

	PHElt m_paHElt[MAXSUBLANG];
};

/****************************************************************/

class Charmap {

public:
	Charmap (PFileMapSection pMS) :	m_nID(pMS->m_dwID),	m_nSize(pMS->m_dwSize),
		m_nUnique(pMS->m_dwNUnique), m_pElts( (PHIdx) (&pMS[1]) ) { }

//	int ID (void) const { return m_nID; }
	int Size (void) const { return m_nSize; }
	int NUnique (void) const { return m_nUnique; }
	PHIdx Map (void) const { return m_pElts; }
	HIdx Map (WCHAR x) const { return m_pElts[x]; }

private:
	int m_nID;			// ID by which hardwired code finds the table
	int	m_nSize;		// size of table (256 or 65536)
	int m_nUnique;		// # of unique output values
	
	PHIdx m_pElts;
};
typedef Charmap *PCharmap;

/****************************************************************/

// class CScore -- score for one lang and/or code page, variously used for
// individual chunks and also for an entire document.

class CScore {
public:
	// Only these two slots need to be initialized
	CScore (void) : m_nScore(0), m_nChars(0) {}
	~CScore (void) { }
	
	const PLanguage GetLang (void) const { return m_pLang; }
	int GetScore (void) const { return m_nScore; }
	unsigned short GetCodePage (void) const { return m_nCodePage; }
	unsigned short GetCharCount (void) const { return m_nChars; }

	void SetLang (PLanguage p) { m_pLang = p; }
	void SetScore (int x) { m_nScore = x; }
	void SetCharCount (unsigned x) { m_nChars = (unsigned short)x; }
	void SetCodePage (unsigned x) { m_nCodePage = (unsigned short)x; }

	void Add (CScore &S) { 
		SetLang(S.GetLang());
		SetCodePage(S.GetCodePage());
		SetScore(GetScore() + S.GetScore());
		SetCharCount(GetCharCount() + S.GetCharCount());
	}
	CScore & operator += (CScore &S) { Add (S); return *this; }

	int operator <= (CScore &S) {
		// Special:  always put 8-bit langs first since the code page
		// matters more for them.
		if (GetLang()->Type() != S.GetLang()->Type())
			return GetLang()->Type() == DETECT_8BIT ? -1 : 1;
		return GetScore() <= S.GetScore();
	}

#ifdef DEBUG_LCDETECT
	void Print(void) {
		printf("Lang=%d CodePage=%d Score=%d NChars=%d\n",
			GetLang() ? GetLang()->LanguageID() : -1, 
			GetCodePage(), GetScore(), GetCharCount());
	}
#endif

private:
	PLanguage m_pLang;
	int m_nScore;
	unsigned short m_nCodePage;
	unsigned short m_nChars;
};
typedef CScore *PScore;

////////////////////////////////////////////////////////////////

// class CScores
//
// For SBCS detection, the index e.g. Ref(i) is the language+codepage index,
// one of a contiguous set of values which identifies each unique supported
// language and codepage combination.
//
// For DBCS detection, the index is just the Unicode language group.

class CScores {
public:
	CScores (int nAlloc, PScore p) : m_nAlloc(nAlloc), m_nUsed(0), m_p(p) { }
	virtual ~CScores (void) { }

	void Reset (void) {
		memset ((void *)m_p, 0, sizeof(CScore) * m_nUsed);
		m_nUsed = 0;
	}

	unsigned int &NElts (void) { return m_nUsed; }
	CScore &Ref (unsigned int n) {
		if (m_nUsed <= n)
			m_nUsed = n + 1; 
		return m_p[n]; 
	}

	void SelectCodePages (void);

	void RemoveZeroScores (void) {
		for (unsigned int i = 0, j = 0; i < m_nUsed; i++)
		{
			if (m_p[i].GetScore() > MINRAWSCORE)
				m_p[j++] = m_p[i];
		}
		m_nUsed = j;
	}

	// Sort by decreasing score.
	// Instantiates template qsort using CScore::operator <=

	void SortByScore (void) {
		RemoveZeroScores ();
		if (m_nUsed)
			QSort (m_p, m_nUsed, FALSE);
	}

	CScore & FindHighScore (void) {
		int highscore = 0;
		for (unsigned int i = 0, highidx = 0; i < m_nUsed; i++) {
			if (m_p[i].GetScore() > highscore)
			{
				highscore = m_p[i].GetScore();
				highidx = i;
			}
		}
		return m_p[highidx];
	}

protected:
	unsigned int m_nAlloc;
	unsigned int m_nUsed;	// high water mark to optimize NElts(), Reset()
	PScore m_p;				// score array, typically per TScores<NNN>
};

template<ULONG Size>class TScores : public CScores {

public:
	TScores (void) : CScores (Size, m_S) { }
	virtual ~TScores (void) { }

private:
	CScore m_S[Size];
};

////////////////////////////////////////////////////////////////

class LCDetect {

public:
	LCDetect (HMODULE hM);
	~LCDetect (void);

	unsigned int GetNCharmaps() const { return m_nCharmaps; }
	unsigned int GetN7BitLanguages() const { return m_n7BitLanguages; }
	unsigned int GetN8BitLanguages() const { return m_n8BitLanguages; }
	unsigned int GetNUnicodeLanguages() const { return m_nUnicodeLanguages; }

	PLanguage7Bit Get7BitLanguage (int i) const { return m_pp7BitLanguages[i]; }
	PLanguage8Bit Get8BitLanguage (int i) const { return m_pp8BitLanguages[i]; }
	PLanguageUnicode GetUnicodeLanguage (int i) const { return m_ppUnicodeLanguages[i]; }

	PHIdx GetMap (int i) const { return m_ppCharmaps[i]->Map(); }

	const LCDConfigure &GetConfig () const { return m_LCDConfigureDefault; }

	DWORD LoadState (void);

	DWORD DetectA (LPCSTR pStr, int nChars, PLCDScore paScores, 
							int *pnScores, PCLCDConfigure pLCDC) const;

	DWORD DetectW (LPCWSTR wcs, int nInputChars, PLCDScore paScores, 
							int *pnScores, PCLCDConfigure pLCDC) const;

private:
	DWORD Initialize7BitLanguage (PFileLanguageSection pLS, PLanguage *ppL);
	DWORD Initialize8BitLanguage (PFileLanguageSection pLS, Language **ppL);
	DWORD InitializeUnicodeLanguage (PFileLanguageSection pLS,Language **ppL);
	DWORD LoadLanguageSection (void *pv, int nSectionSize, PLanguage *ppL);
	DWORD LoadHistogramSection (void *pv, int nSectionSize, Language *pL);
	DWORD LoadMapSection (void *pv, int nSectionSize);
	DWORD BuildState (DWORD nFileSize);

	void Score7Bit (LPCSTR pcszText, int nChars, CScores &S) const;
	void Score8Bit (LPCSTR pcszText, int nChars, CScores &S) const;
	int ScoreCodePage (LPCSTR pStr, int nChars, CScore &S) const;
	int ChooseDetectionType (LPCSTR pcszText, int nChars) const;
	void ScoreLanguageA (LPCSTR pStr, int nChars, CScores &S) const;
	void ScoreLanguageW (LPCWSTR wcs, int nChars, CScores &S, PCLCDConfigure) const;
	void ScoreLanguageAsSBCS (LPCWSTR wcs, int nch, CScores &S) const;
	void ScoreUnicodeSublanguages (PLanguageUnicode pL, LPCWSTR wcs, 
			int nch, CScores &S) const;

private:
	// Language training info virtual-mapped in training file

	unsigned int m_nCharmaps;
	unsigned int m_n7BitLanguages;
	unsigned int m_n8BitLanguages;
	unsigned int m_nUnicodeLanguages;

	PCharmap *m_ppCharmaps;
	PLanguage7Bit *m_pp7BitLanguages;
	PLanguage8Bit *m_pp8BitLanguages;
	PLanguageUnicode *m_ppUnicodeLanguages;

	// Cached information for the optimized scoring inner-loops.

	PHElt m_paHElt7Bit[MAX7BITLANG];
	PHElt m_paHElt8Bit[MAXSCORES];
	int m_nHElt8Bit;

	// Special 7-bit lang histogram for ScoreLanguageAsSBCS()

	PHistogram m_pHU27Bit;

	// Initialization state variables

	unsigned int m_n7BitLangsRead;
	unsigned int m_n8BitLangsRead;
	unsigned int m_nUnicodeLangsRead;
	unsigned int m_nMapsRead;
	int m_nHistogramsRead;
	int m_nScoreIdx;

	// Default configuration to use when NULL parameter passed to detect

	LCDConfigure m_LCDConfigureDefault;

	// File mapping information for the training data file

	HANDLE m_hf;
	HANDLE m_hmap;
	void *m_pv;

	HMODULE m_hModule;
};

////////////////////////////////////////////////////////////////

inline PLanguageUnicode 
LanguageUnicode::GetSublanguage (int n) const 
{ 
	return m_pLC->GetUnicodeLanguage(GetSublangRangeID(n));
}

#endif  // __cplusplus
