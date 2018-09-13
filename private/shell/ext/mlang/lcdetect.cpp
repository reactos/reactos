/*
 * Automatic language and codepage detector
 * 
 * Copyright (C) 1996, 1997, Microsoft Corp.  All rights reserved.
 * 
 *  History:    1-Feb-97    BobP      Created
 *              5-Aug-97    BobP      Added Unicode support and rewrote
 *                                    scoring to use vector math.
 * 
 * This is the runtime detector.
 * 
 * See the comments in lcdcomp.cpp for a description of the compilation
 * process and training data format.
 *
 * See design.txt for a description of the detection and scoring algorithm.
 *
 * Performance note:  60-80% of execution time in this code is AddVector(),
 * which is probably memory-cycle bound by its random data access, but is
 * still a candidate for further optimizing with an intrinsic vector operator,
 * should one become available.
 * 
 * to-do (as needed):
 * - Adjust 7-bit and 8-bit scores to make them more comparable
 * - detect UTF-8 in the SBCS entry point, via heuristic and via
 *   subdetection as 7-bit lang and as Unicode.
 */

#include "private.h"

// This is all the global (per-process) state
//
// It is set at DLL process init and its contents are const after that.

LCDetect * g_pLCDetect;

#ifdef DEBUG_LCDETECT
int g_fDebug;
#endif

/****************************************************************/

static inline unsigned int
FindHighIdx (const int *pn, unsigned int n)
//
// Return the INDEX of the highest-valued integer in the given array.
{
	int nMax = 0;
	unsigned int nIdx = 0;

	for (unsigned int i = 0; i < n; i++)
	{
		if (pn[i] > nMax)
		{
			nMax = pn[i];
			nIdx = i;
		}
	}

	return nIdx;
}

/****************************************************************/

void
CScores::SelectCodePages (void)
//
// Find the highest scoring code page for each language, and remove
// all the other scores from the array such that the array contains
// exactly one score per detected language instead of one score per
// code page per language.
//
// When multiple scores are present for different code pages of the same
// language, this function combines the scores into a single score.
// The resulting entry will have the code page of the top-scoring code page
// for the various entries for that language, and the score and char count
// will be the SUM of the scores and char counts for ALL the entries for
// that language.
//
// For example, if the input contains:
//		Lang		Codepage	Score	Char count
//		Russian		1251		42		200
//		Russian		20866		69		300
//
// Then on output, the array will contain only one score for Russian:
//		Russian		20866		111		500
//
// This overwrites the entries in place, and sets m_nUsed to the resulting
// number of active slots.
//
// The scores are already grouped by language, no need to sort by language.
//
// After return, the score array must NOT be referenced via ScoreIdx()
// because the index of the entries has changed.
{
	// The score indices no longer matter, remove slots that scored zero.

	RemoveZeroScores ();

	if (m_nUsed == 0)
		return;

	// Select top score per language.  This is fundamentally dependent
	// on the score array already being ordered by language.  This won't
	// combine scores for the same language as both a 7-bit and 8-bit lang,
	// but that's not worth fixing.

	int maxscore = 0;					// highest score for a given language
	int totalscore = m_p[0].GetScore();	// sum of scores  " "
	int totalchars = m_p[0].GetCharCount();// sum of character counts  " "

	int nReturned = 0;			// index and ultimate count of elts returned
	unsigned int maxscoreidx = 0; // array index of the top-scoring code page,
								  // *** for the current language ***

	for (unsigned int i = 1; i < m_nUsed; i++) {
		if (m_p[i-1].GetLang() != m_p[i].GetLang())
		{
			// [i] indicates a different language from the previous entry
			
			// Add the entry for the previous language to the result
			// by copying the slot for its highest-scoring code page,
			// and overwriting its score and char count with the sum counts.

			m_p[maxscoreidx].SetScore(totalscore);
			m_p[maxscoreidx].SetCharCount(totalchars);
			m_p[nReturned++] = m_p[maxscoreidx];

			// Start remembering the top and total scores for the new lang.

			maxscore = m_p[i].GetScore();
			totalscore = m_p[i].GetScore();
			totalchars = m_p[i].GetCharCount();
			maxscoreidx = i;		// remember which [] had the top score
		}
		else 
		{
			// Accumulate more scores for the same language

			if (m_p[i].GetScore() > maxscore) {
				maxscore = m_p[i].GetScore();
				maxscoreidx = i;
			}
			totalscore += m_p[i].GetScore();
			totalchars += m_p[i].GetCharCount();
		}
	}

	// Process the the last language.  Return the slot from its
	// highest-scoring code page.

	if (m_nUsed > 0)
	{
		m_p[maxscoreidx].SetScore(totalscore);
		m_p[maxscoreidx].SetCharCount(totalchars);
		m_p[nReturned++] = m_p[maxscoreidx];
	}

	m_nUsed = nReturned;
}

/****************************************************************/

static void __fastcall
AddVector (int *pS, const PHElt *pH, int idx, unsigned int nScores)
//
// Add the score vector for a single n-gram to the running sum score
// vector at pS.
//
// On return, paS[0..nScores-1] is filled with the sum scores for each
// language.
//
// **** PERFORMANCE NOTE ****
//
// This is the critical inner-loop of the entire subsystem.
// 
// Code generation and performance have been checked for various code
// organization.  Ironically, making AddVector() a true function is
// FASTER than inlining it because when inlined, the registers are used
// for the OUTER loop variables and the inner loop here does approximately
// twice as many memory references per pass.
//
// On x86, all four loop variables are registered, and each pass makes only
// three memory references, which is optimal for the given representation.
// 
// Future note: the histogram tables could be pivoted to collect all the
// scores for each n-gram in a block; that would eliminate the double 
// indirection through ph and reduce the memory refs to two per pass.
{
	nScores++;		// makes faster end-test

	while (--nScores != 0)
		*pS++ += (*pH++)[idx];
}

static inline void
ScoreUnigramVector (LPCSTR pcsz, int nCh, PHistogram pH,
	int *paS, const PHElt *paH, unsigned int nScores)
//
// Score this text for a unigram histogram.  Each individual character is
// mapped to a histogram slot to yield a score for that character in each
// language.
{
	if (nCh < 1)
		return;

	const PHIdx pMap = pH->GetMap();

	unsigned char *p = (unsigned char *)pcsz;

	while (nCh-- > 0)
		AddVector (paS, paH, pMap[*p++], nScores);
}

static inline void
ScoreUnigramVectorW (LPCWSTR pcwsz, int nCh, PHistogram pH,
	int *paS, const PHElt *paH, unsigned int nScores)
//
// WCHAR version.  Only difference is the use of a map that maps the
// full 64K WCHAR space into the histogram index range.
{
	if (nCh < 1)
		return;

	const PHIdx pMap = pH->GetMap();

	while (nCh-- > 0)
		AddVector (paS, paH, pMap[*pcwsz++], nScores);
}

static inline void
ScoreDigramVector (LPCSTR pcsz, int nCh, PHistogram pH,
	int *paS, const PHElt *paH, unsigned int nScores)
//
// Score this text for a digram histogram.  Each adjacent pair of characters
// are mapped to the index range and the mapped values combined to form an
// array index unique to that digram.  The scores for that array slot are
// summed for each language.
{
	if (nCh < 2)
		return;

	unsigned char *p = (unsigned char *)pcsz;

	const PHIdx pMap = pH->GetMap();

	unsigned char ch1 = pMap[*p++];

	while (nCh-- > 1)
	{
		unsigned char ch2 = pMap[*p++];

		AddVector (paS, paH, ch1 * pH->EdgeSize() + ch2, nScores);

		ch1 = ch2;
	}
}

static inline void
ScoreTrigramVector (LPCSTR pcsz, int nCh, PHistogram pH,
	int *paS, const PHElt *paH, unsigned int nScores)
//
// Score this text for a trigram histogram.  Each adjacent three-letter set 
// of characters are mapped to the index range and the mapped values combined
// to form an array index unique to that trgram.
{
	if (nCh < 3)
		return;

	unsigned char *p = (unsigned char *)pcsz;

	const PHIdx pMap = pH->GetMap();

	unsigned char ch1 = pMap[*p++];
	unsigned char ch2 = pMap[*p++];

	while (nCh-- > 2)
	{
		unsigned char ch3 = pMap[*p++];
		debug(printf("  '%c%c%c':",unmapch(ch1),unmapch(ch2),unmapch(ch3)));

		int idx = ((ch1 * pH->EdgeSize()) + ch2) * pH->EdgeSize() + ch3;
		ch1 = ch2;
		ch2 = ch3;

		AddVector (paS, paH, idx, nScores);

		debug(for (UINT i = 0; i < nScores; i++) printf(" %3d", paH[i][idx]));
		debug(printf("\n"));
	}
}

static inline void
ScoreTrigramVectorW (LPCWSTR pcwsz, int nCh, PHistogram pH,
	int *paS, const PHElt *paH, unsigned int nScores)
//
// WCHAR version.
{
	if (nCh < 3)
		return;

	const PHIdx pMap = pH->GetMap();

	unsigned char ch1 = pMap[*pcwsz++];
	unsigned char ch2 = pMap[*pcwsz++];

	while (nCh-- > 2)
	{
		unsigned char ch3 = pMap[*pcwsz++];

		int idx = ((ch1 * pH->EdgeSize()) + ch2) * pH->EdgeSize() + ch3;
		ch1 = ch2;
		ch2 = ch3;

		AddVector (paS, paH, idx, nScores);
	}
}

static inline void
ScoreNgramVector (LPCSTR pcsz, int nCh, PHistogram pH,
	int *paS, const PHElt *paH, unsigned int nScores)
//
// Score this text for any dimension of n-gram.  Get "N" from the
// dimensionality of the histogram.
//
//  Each adjacent n-letter set of characters are mapped to the index range
// and the scores the reference summed for each language.  This code is
// never used for the current data file, instead an optimized scoring
// loop exists for each existing case.  This exists to enable trying
// different dimension scoring without requiring a new DLL.
{
	if (nCh < pH->Dimensionality())
		return;

	unsigned char *p = (unsigned char *)pcsz;

	const PHIdx pMap = pH->GetMap();

	// Fill the pipeline

	int idx = 0;
	if (pH->Dimensionality() >= 2)
		idx = idx * pH->EdgeSize() + pMap[*p++];
	if (pH->Dimensionality() >= 3)
		idx = idx * pH->EdgeSize() + pMap[*p++];
	if (pH->Dimensionality() >= 4)
		idx = idx * pH->EdgeSize() + pMap[*p++];

	unsigned int nLoopCount = nCh - (pH->Dimensionality() - 1);

	while (nLoopCount-- > 0)
	{
		idx = (idx * pH->EdgeSize() + pMap[*p++]) % pH->NElts();

		AddVector (paS, paH, idx, nScores);
	}
}

static inline void
ScoreNgramVectorW (LPCWSTR pcwsz, int nCh, PHistogram pH,
	int *paS, const PHElt *paH, unsigned int nScores)
//
// WCHAR version.
{
	if (nCh < pH->Dimensionality())
		return;

	const PHIdx pMap = pH->GetMap();

	// Fill the pipeline

	int idx = 0;
	if (pH->Dimensionality() >= 2)
		idx = idx * pH->EdgeSize() + pMap[*pcwsz++];
	if (pH->Dimensionality() >= 3)
		idx = idx * pH->EdgeSize() + pMap[*pcwsz++];
	if (pH->Dimensionality() >= 4)
		idx = idx * pH->EdgeSize() + pMap[*pcwsz++];

	unsigned int nLoopCount = nCh - (pH->Dimensionality() - 1);

	while (nLoopCount-- > 0)
	{
		idx = (idx * pH->EdgeSize() + pMap[*pcwsz++]) % pH->NElts();

		AddVector (paS, paH, idx, nScores);
	}
}

void
ScoreVector (LPCSTR pcsz, int nCh, PHistogram pH,
	int *paS, const PHElt *paH, unsigned int nScores)
//
// Score a string into an array of scores using an array of histograms
//
// Each character n-gram is mapped to a histogram slot to yield a score
// for that character in each array at paH.
//
// On return, paS[0..nScores-1] is filled with the sum scores.
{
	memset (paS, 0, sizeof(int) * nScores);

	switch (pH->Dimensionality()) 
	{
	case 1:
		ScoreUnigramVector (pcsz, nCh, pH, paS, paH, nScores);
		break;

	case 2:
		ScoreDigramVector (pcsz, nCh, pH, paS, paH, nScores);
		break;

	case 3:
		ScoreTrigramVector (pcsz, nCh, pH, paS, paH, nScores);
		break;

	default:
		ScoreNgramVector (pcsz, nCh, pH, paS, paH, nScores);
		break;
	}
}

void
ScoreVectorW (LPCWSTR pcwsz, int nCh, PHistogram pH,
	int *paS, const PHElt *paH, unsigned int nScores)
//
// Score a string into an array of scores using an array of histograms.
{
	memset (paS, 0, sizeof(int) * nScores);

	switch (pH->Dimensionality()) 
	{
	case 1:
		ScoreUnigramVectorW (pcwsz, nCh, pH, paS, paH, nScores);
		break;

	case 3:
		ScoreTrigramVectorW (pcwsz, nCh, pH, paS, paH, nScores);
		break;

	default:
		ScoreNgramVectorW (pcwsz, nCh, pH, paS, paH, nScores);
		break;
	}
}

void
LCDetect::Score7Bit (LPCSTR pcszText, int nChars, CScores &S) const
//
// Do 7-bit language detection.  Compute scores for all 7-bit languages
// and store the raw language score in S at the language's base score-idx.
//
// Fill in only the first score slot per language.  Uses ScoreIdx() for
// the first code page, but does not detect or set the code page.
{
	const PHistogram pH = Get7BitLanguage(0)->GetLangHistogram();

	debug(printf("       "));
	debug(for(unsigned int x=0;x<GetN7BitLanguages();x++)printf(" %3d", Get7BitLanguage(x)->LanguageID()));
	debug(printf("\n"));

	int sc[MAXSCORES];

	// Compute the raw score vector

	ScoreVector (pcszText, nChars, pH, sc, m_paHElt7Bit, GetN7BitLanguages());


	// Fill in the CScores array from it

	for (unsigned int i = 0; i < GetN7BitLanguages(); i++)
	{
		PLanguage7Bit pL = Get7BitLanguage(i);

		CScore &s = S.Ref(pL->GetScoreIdx());

		s.SetLang(pL);
		s.SetCodePage(0);
		s.SetScore(sc[i]);
		s.SetCharCount(nChars);
	}
}

void
LCDetect::Score8Bit (LPCSTR pcszText, int nChars, CScores &S) const
//
// Do 8-bit detection.  Compute a combined language / code page score
// for each trained language / code page combination for the 8-bit languages.
// Store all the raw scores in S at the language+each codepage score-idx.
//
// May store multiple entries in S for each language, one per code page.
{
	const PHistogram pH = Get8BitLanguage(0)->GetHistogram(0);

	int sc[MAXSCORES];

	// Compute the raw score vector

	ScoreVector (pcszText, nChars, pH, sc, m_paHElt8Bit, m_nHElt8Bit);

	// Fill in the CScores array from it

	int nSc = 0;
	for (unsigned int i = 0; i < GetN8BitLanguages(); i++)
	{
		PLanguage8Bit pL = Get8BitLanguage(i);

		for (int j = 0; j < pL->NCodePages(); j++)
		{
			CScore &s = S.Ref(pL->GetScoreIdx() + j);

			s.SetLang(pL);
			s.SetCodePage(pL->GetCodePage(j));
			s.SetScore( sc[ nSc++ ] );
			s.SetCharCount(nChars);
		}
	}
}

void
LCDetect::ScoreLanguageAsSBCS (LPCWSTR wcs, int nch, CScores &S) const
//
// This scores Unicode text known to contain mostly characters in the
// script ranges used for 7-bit languages.  This uses a special mapping,
// m_pH727Bit, that converts n-grams in the WCHAR text directly to the same
// mapping output space used for 7-bit language detection.  It is then scored
// using the same language-only histograms used for 7-bit SBCS detection.
//
// The output is the same as if Score7Bit() had been called on the SBCS
// equivalent to this text.  The same slots in S are filled in, using the
// 7-bit score indices, NOT the Unicode language score indices.
{
	debug(printf("    scoring as SBCS\n"));

	debug(printf("       "));
	debug(for(unsigned int x=0;x<GetN7BitLanguages();x++)printf(" %3d", Get7BitLanguage(x)->LanguageID()));
	debug(printf("\n"));

	// Call ScoreVectorW(), passing the histogram set up or the WCHAR map.

	int sc[MAXSCORES];

	// Compute the raw score vector

	ScoreVectorW (wcs, nch, m_pHU27Bit, sc, m_paHElt7Bit,GetN7BitLanguages());


	// Fill in the CScores array from it

	for (unsigned int i = 0; i < GetN7BitLanguages(); i++)
	{
		PLanguage7Bit pL = Get7BitLanguage(i);

		CScore &s = S.Ref(pL->GetScoreIdx());

		s.SetLang(pL);
		s.SetCodePage(0);
		s.SetScore(sc[i]);
		s.SetCharCount(nch);
	}
}

////////////////////////////////////////////////////////////////

void
Language::ScoreCodePage (LPCSTR, int nCh, CScore &S, int &idx) const 
//
// The default handler for scoring the code page for text for which the
// language is already known.  Initially used only for Unicode.
{
	idx = 0; 
	S.SetCodePage(0); 
}

void
Language7Bit::ScoreCodePage (LPCSTR pStr, int nCh, CScore &S, int &idx) const
//
// Detect the code page for text whose language has already been detected
// and is indicated in S.  Set S.CodePage(), do not change other
// fields of S.  
//
// Set idx to the index of the high-scoring code page.  The caller uses this
// to place the score in the correct ScoreIdx slot.
//
// Note that the arg is a single CScore, not an array.  The CScore S is
// filled in with the score of the high-scoring code page, and no information
// about the other code pages is returned.
{
	if (NCodePages() == 1)
	{
		// If lang is trained with only one codepage, just return it.

		idx = 0;
		S.SetCodePage(GetCodePage(0));

		debug(printf("  score code page: only one; cp=%d\n",GetCodePage(0)));
	}

	debug(printf("scoring 7-bit code pages: "));

	int sc[MAXSUBLANG];

	// Compute the raw score vector

	ScoreVector (pStr, nCh, GetCodePageHistogram(0),
			sc, GetPHEltArray(), NCodePages());

	// Find the high-scoring code page and fill in S with its values

	idx = FindHighIdx (sc, NCodePages());

	debug(printf("selecting cp=%d idx=%d\n", GetCodePage(idx), idx));

	S.SetCodePage (GetCodePage(idx));
}

void
LanguageUnicode::ScoreSublanguages (LPCWSTR wcs, int nch, CScores &S) const
//
// Score wcs for each sub-language and add the raw scores to S.
// The scores are not qualified at this time.
// 
// Relevant only for Unicode language groups that require subdetection,
// initially CJK.
{
	if (m_nSubLangs == 0)
		return;

	debug(printf("    scoring Unicode sublanguages:\n"));

	int sc[MAXSUBLANG];

	// Compute the raw score vector

	ScoreVectorW (wcs, nch, GetHistogram(0), sc, m_paHElt, m_nSubLangs);

	// Fill in the CScores array from it

	for (int i = 0; i < NSubLangs(); i++)
	{
		PLanguageUnicode pSL = GetSublanguage(i);

		CScore &s = S.Ref(pSL->GetScoreIdx());
		s.SetLang (pSL);
		s.SetScore (sc[i]);
		s.SetCharCount (nch);
		s.SetCodePage (0);

		debug(printf("      lang=%d score=%d\n", pSL->LanguageID(), sc[i]));
	}
}

int
LCDetect::ChooseDetectionType (LPCSTR pcszText, int nChars) const
//
// Histogram the raw char values to determine whether to use 7-bit or
// 8-bit detection for this block.
{
	// Count the proportion of chars < vs. >= 0x80

	int nHi = 0;

	for (int i = nChars; i-- > 0; )
		nHi += ((unsigned char)*pcszText++) & 0x80;

	nHi /= 0x80;
	int nLo = nChars - nHi;

	// Make sure there is sufficient data to make a good choice

	// work here -- try  if abs(nHi - nLo) < 10

	if (nHi + nLo < 10)
		return DETECT_NOTDEFINED;

	if (nHi * 2 > nLo)
		return DETECT_8BIT;
	else
		return DETECT_7BIT;
}

void
LCDetect::ScoreLanguageA (LPCSTR pStr, int nChars, CScores &S) const
//
//
// Score the text at pStr for each language that it potentially contains.
//
// Add the scores to S at the ScoreIdx() for each language and codepage
// combination.
// 
// This adds all the raw scores for either all the 7-bit or all the
// 8-bit entries, depending on which category the rough initial analysis
// indicates.  At this time, there are no entries for which both methods
// are required.
//
// For 7-bit detection, code page is always set to 0 and the language's score
// is placed in the 0'th slot for each language.  The caller later scores
// code pages if needed, and fills the remaining slots.
// 
// For 8-bit detection, scores are generated for each code page and all
// ScoreIdx() slots are used.
{
	switch (ChooseDetectionType (pStr, nChars)) {

	case DETECT_7BIT:
		Score7Bit (pStr, nChars, S);
		break;

	case DETECT_8BIT:
		Score8Bit (pStr, nChars, S);
		break;
	}
}

void
LCDetect::ScoreLanguageW (LPCWSTR wcs, int nch, CScores &S, PCLCDConfigure pC) const
//
// Score the text at wcs for each language that it potentially contains.
//
// Add the scores to S at the ScoreIdx() for each language.
// 
// This first determines the Unicode script groups represented in wcs.
// Each WCHAR is mapped through CHARMAP_UNICODE to yield its "language group
// ID".  The IDs for each char are counted and the top scoring IDs indicate
// the probable languages or language groups.  Note that unlike all other
// use of n-gram scoring, NO WEIGHTS are associated with the IDs -- whichever
// group contains the most raw chars, wins.
// 
// Some languages are indicated by presence of characters in a particular
// script group; these scores are immediately added to S.
// 
// For script groups that indicate multiple languages, subdetection within
// the group is done only when the score for the group exceeds a threshhold
// that indicates the sub-detected languages are likely to be included in
// the final result.  This is purely a performance optimization, not to
// be confused with the uniform score threshhold applied by the caller.
//
// The "Group" entries themselves are never included in the result; they
// exist only to invoke subdetection.
//
// In many cases even a single Unicode character provides sufficient
// identification of script and language, so there is no minimum
// qualification for scores in the script ranges that indicate a
// specific language by range alone.
{
	// Score the chars according to the Unicode script group they belong to.
	// The array indices are the raw outputs of the primary Unicode Charmap
	// NOT to be confused with the ScoreIdx() of each language.  Further,
	// the scores are the simple count of the characters in each script
	// range, and are NOT weighted by any histogram.
	
	// In this initial step, the simple majority of characters per range
	// determines which further detection steps to take.

	const PHIdx map = GetMap (CHARMAP_UNICODE);

	int anScore[MAXSCORES];
	memset (anScore, 0, sizeof(int) * GetNUnicodeLanguages());

	for (int x = 0; x < nch; x++)
		anScore[map[wcs[x]]]++;

	debug(printf("    char_ignore score=%d\n",anScore[HIDX_IGNORE]));

	// Ignore scores for chars that correlate with no language

	anScore[HIDX_IGNORE] = 0;


	// Identify the scores that qualify a language for immediate inclusion
	// in the result, or that qualify a language group for further detection.


	// Find the high score to use as a relative threshhold for inclusion.
	
	int nMaxScore = 0;

	for (unsigned int i = 0; i < GetNUnicodeLanguages(); i++)
	{
		if (anScore[i] > nMaxScore)
			nMaxScore = anScore[i];
	}

	debug(printf("  unicode range max score=%d\n",nMaxScore));

	// Process all individual and group scores above a threshhold.

	// The threshhold logic is different from the logic for SBCS/DBCS
	// detection, because presence of even a single character in certain
	// Unicode script ranges can be a strong correct indicator for a
	// specific language.  The threshhold for subdetected scores is
	// higher, since that is a statistical result; single characters
	// are not as strong an indicator.

	// Set the threshhold for subdetecting.

	int nRelThresh = 1 + (nMaxScore * pC->nRelativeThreshhold) / 100;


	for (i = 0; i < GetNUnicodeLanguages(); i++)
	{
		// Threshhold for any range is at least this many raw chars in range.

		if (anScore[i] >= 2)
		{
			PLanguageUnicode pL = GetUnicodeLanguage(i);

			debug(printf("  using lang=%d score=%d:\n", pL->LanguageID(), anScore[i]));

			if (pL->LanguageID() == LANGID_UNKNOWN)
			{
				// DO NOTHING -- text is an unknown language

				debug(printf("    lang=unknown\n"));

			}
			else if (pL->NSubLangs() > 0)
			{
				// Subdetect language within a Unicode group, and add all the
				// unqualified raw scores directly to S.

				pL->ScoreSublanguages (wcs, nch, S);
			}
			else if ( pL->LanguageID() == LANGID_LATIN_GROUP &&
				      anScore[i] >= nRelThresh )
			{
				// Subdetect Latin/Western languages, and add all the
				// unqualified raw scores to S.
				
				ScoreLanguageAsSBCS (wcs, nch, S);
			} 
			else
			{
				debug(printf("    range identifies language\n"));

				// This range identifies a specific language; add it.

				CScore &s = S.Ref(pL->GetScoreIdx());
				s.SetLang (pL);
				s.SetScore (anScore[i] * UNICODE_DEFAULT_CHAR_SCORE);
				s.SetCharCount (nch);
				s.SetCodePage (0);
			}
		}
	}
}

/****************************************************************/

DWORD
LCDetect::DetectA (LPCSTR pStr, int nInputChars, 
	PLCDScore paScores, int *pnScores,
	PCLCDConfigure pLCDC) const
//
// Do SBCS / DBCS detection.  Detect language and code page of pStr, 
// fill paScores[] with the result and set *pnScores to the result count.
// On input, *pnScores is the available capacity of paScores.
//
// The text at pStr is broken into chunks, typically several hundred
// bytes.  
//
// In the first phase, each chunk is scored by language.  The scores for 
// a single chunk are qualified by both an absolute threshhold and by a 
// threshhold based on the high score of just that chunk.  Scores exceeding
// the threshhold are remembered towards the second phase; other scores
// are discarded.  
//
// For each score that will be remembered, if a code page is not already
// known for it then the code page for the chunk is determined and included
// with the score.  Note that the score refers only to the language, NOT
// to the confidence of the code page.
// 
// In the second phase, the combined scores for all chunks are examined.
// The scores are further qualified by a relative threshhold.  Only
// languages with scores exceeding the threshhold are included in the
// final result; the remainder are discarded.
//
// The two-step process is designed to yield good results for input containing
// text in multiple languages, or containing a high portion of whitespace or
// symbol characters that correlate with no language.  It also is designed
// to optimally handle tie-cases whether due to similar languages or to
// mixed-language input, and to avoid applying threshholds based on
// absolute scores.
//
// The presumption is that each chunk, generally, represents text in a single
// language, and no matter what the absolute high score is, its high score
// most likely is for that language.  The point of the first phase is to
// identify all the languages that are known with some confidence to be
// represented in the text.  For a given chunk, multiple languages scores may
// meet this criteria and be remembered towards the result.  Specifically,
// when a tie occurs, BOTH scores are always included.  (Choosing just one
// would be wrong too often to be worthwhile.)
//
// The point of the second phase is to filter out the noise allowed by the
// first phase.
{
	TScores<MAXSCORES> SChunk;		// Scores for one chunk at a time
	TScores<MAXSCORES> SAll;		// Qualified scores for ultimate result

	if (pLCDC == NULL)				// Use the default config if not specified
		pLCDC = &m_LCDConfigureDefault;

	if (*pnScores == 0)
		return NO_ERROR;

#define MAX_INPUT (USHRT_MAX-1)
	// CScore.NChars() is a USHORT to save space+time, so only this # of chars
	// can be accepted per call or the scoring would overflow.

	nInputChars = min (nInputChars, MAX_INPUT);
	debug(printf("LCD_Detect: detecting %d chars\n", nInputChars));

	// The first loop processed fixed-size chunks and accumulates all the
	// credibly-detected languages in SAll.  This is the "coarse" accuracy
	// qualification:  detect the language of text blocks small enough to
	// typically be in *one* language, and remember only the highest scoring
	// language for that chunk.  Then generate a multivalued result that
	// shows the distribution of language in the doc, instead of simply
	// returning the dominant language.  This is necessary because it is
	// much harder to determine the sole language than to determine the
	// multivalued result.

	int nProcessed = 0;

	while (nProcessed < nInputChars)
	{
		SChunk.Reset();				// reset is cheaper than constructing

		// Process nChunkSize worth of text if that will leave at least
		// another nChunkSize piece for the final pass.  If that would
		// leave a smaller final chunk, go ahead and process the entire
		// remaining input.

		int nch = nInputChars - nProcessed;

		if (nch >= pLCDC->nChunkSize * 2)
			nch = pLCDC->nChunkSize;


		debug(printf("\nStarting chunk: %d ch\n\"%.*s\"\n", nch, nch, &pStr[nProcessed]));

		ScoreLanguageA (&pStr[nProcessed], nch, SChunk);

		// Compute the threshhold for inclusion of each score in the
		// overall result.

		int nRelThresh = 1 + (SChunk.FindHighScore().GetScore() * pLCDC->nRelativeThreshhold) / 100;
		int nThresh7 = max (pLCDC->nMin7BitScore * nch, nRelThresh);
		int nThresh8 = max (pLCDC->nMin8BitScore * nch, nRelThresh);

		debug(printf("high score=%d min7=%d thresh7=%d thresh8=%d\n", SChunk.FindHighScore().GetScore(),pLCDC->nMin7BitScore*nch,nThresh7,nThresh8));

		// Qualify each score, remember only scores well-above the noise.

		for (unsigned int i = 0; i < SChunk.NElts(); i++)
		{
			CScore &s = SChunk.Ref(i);
			PLanguage pL = s.GetLang();

//			debug(if (s.GetScore()) printf("  raw: lang=%d score=%d cp=%d\n",pL->LanguageID(),s.GetScore(),s.GetCodePage()));

			if ( (s.GetScore() >= nThresh7 && pL->Type() == DETECT_7BIT) ||
				 (s.GetScore() >= nThresh8 && pL->Type() == DETECT_8BIT) )
			{
				debug(printf("    qual: lang=%d score=%d cp=%d\n",pL->LanguageID(),s.GetScore(),s.GetCodePage()));

				// If code page is not already set, detect it, and store
				// the score for this language using the scoreidx slot 
				// for that code page.  Store no score in the slots for
				// other code pages for the same language.

				int idx = 0;

				if (s.GetCodePage() == 0)
					pL->ScoreCodePage (&pStr[nProcessed], nch, s, idx);

				// Remember this score for the overall results

				SAll.Ref(i + idx) += s;
			}
		}

		nProcessed += nch;
	}

	// SAll has entries for each unique { lang ID, code page }
	// with the char count and total raw score (not normalized per char) 
	// for those chunks whose score qualifies as a confident result and
	// that contributed to the entry.

	// Select the top-scoring code page for each language
	// and remove all other code page scores.
	
	debug(printf("Selecting top-scoring code pages\n"));

	SAll.SelectCodePages ();

	// Sort by decreasing score

	SAll.SortByScore ();

	// Build the client return structure
	//		Language ID
	//		Code page
	//		Doc percent 0-100
	//		Confidence 0-100

	int nScoresReturned = 0;

	for (unsigned i = 0; i < SAll.NElts() && nScoresReturned < *pnScores; i++)
	{
		CScore &s = SAll.Ref(i);

		LCDScore R;

		R.nLangID = s.GetLang()->LanguageID();
		R.nCodePage = s.GetCodePage();

		// Percent of doc for which this language scored above the
		// confidence threshhold, even if not 1st place for that chunk.

		R.nDocPercent = (s.GetCharCount() * 100) / nProcessed;

		debug(printf("s.CharCount=%d nProcessed=%d\n", s.GetCharCount(), nProcessed));

		// Confidence is the raw score for all the chunks for which this
		// language was detected above the confidence threshhold, divided
		// by the number of characters in those chunks.
		
		R.nConfidence = s.GetScore() / s.GetCharCount();

		debug(printf("Examining: lang=%d cp=%d docpct=%d\n", R.nLangID, R.nCodePage, R.nDocPercent));

		// Return only scores for languages detected in over a
		// minimum % of the doc.

		if (R.nDocPercent > pLCDC->nDocPctThreshhold)
		{
			debug(printf("  returning score\n"));
			paScores[nScoresReturned++] = R;
		}
	}

	debug(printf("Returning %d scores\n", nScoresReturned));

	*pnScores = nScoresReturned;

	return NO_ERROR;
}

DWORD
LCDetect::DetectW (LPCWSTR pwStr, int nInputChars,
	PLCDScore paScores, int *pnScores, PCLCDConfigure pLCDC) const
// 
// WCHAR (Unicode) version of LCD_Detect.  Score into paScores, one score
// per language.
{
	if (pLCDC == NULL)				// Use the default config if not specified
		pLCDC = &m_LCDConfigureDefault;

	if (*pnScores == 0)
		return NO_ERROR;

	// CScore.NChars() is a USHORT to save space+time, so only this # of chars
	// can be accepted per call or the scoring would overflow.

	nInputChars = min (nInputChars, MAX_INPUT);
	debug(printf("LCD_DetectW: detecting %d chars\n", nInputChars));

	TScores<MAXSCORES> SChunk;		// Raw score for one chunk at a time
	TScores<MAXSCORES> SAll;		// Qualifying scores for final result

	// SChunk is defined outside the loop since it's cheaper to Reset() it
	// than to reconstruct it each time.

	int nProcessed = 0;

	// Process one chunk of the input per loop

	while (nProcessed < nInputChars)
	{
		SChunk.Reset();


		// Process nChunkSize worth of text if that will leave at least
		// another nChunkSize piece for the final pass.  If that would
		// leave a smaller final chunk, go ahead and process the entire
		// remaining input.

		int nch = nInputChars - nProcessed;

		if (nch >= pLCDC->nChunkSize * 2)
			nch = pLCDC->nChunkSize;


		debug(printf("\nStarting chunk: %d ch\n", nch));

		// Compute the raw scores for the chunk.
		// This automatically includes the sub-detected language scores
		// for the Latin/Western group and Unicode groups, <<< when the 
		// group itself >>> scores above the inclusion threshhold.
		// But, the sub-detected scores themselves still need to be
		// qualified.

		ScoreLanguageW (&pwStr[nProcessed], nch, SChunk, pLCDC);

		// Compute the threshhold for inclusion of each score in the
		// overall result.

		int nRelThresh = 1 + (SChunk.FindHighScore().GetScore() * pLCDC->nRelativeThreshhold) / 100;
		int nThresh7 = max (pLCDC->nMin7BitScore * nch, nRelThresh);
		int nThreshU = max (pLCDC->nMinUnicodeScore * nch, nRelThresh);

		debug(printf("scores: nElts=%d rel=%d%% high=%d min=%d min7=%d minU=%d\n", SChunk.NElts(), pLCDC->nRelativeThreshhold, SChunk.FindHighScore().GetScore(), nRelThresh,nThresh7,nThreshU));

		// Qualify each score, remember only scores well-above the noise.

		for (unsigned int i = 0; i < SChunk.NElts(); i++)
		{
			CScore &s = SChunk.Ref(i);
			PLanguage pL = s.GetLang();

			if ( (s.GetScore() >= nThresh7 && pL->Type() == DETECT_7BIT) ||
				 (s.GetScore() >= nThreshU && pL->Type() == DETECT_UNICODE) )
			{
				debug(printf("    using lang=%d score=%d nch=%d\n",pL->LanguageID(),s.GetScore(),s.GetCharCount()));

				// Remember this score for the overall results

				SAll.Ref(i) += s;
			}
		}

		nProcessed += nch;
	}

	// SAll has entries for each unique language with char count and total
	// raw score (not normalized per char) for those chunks whose score
	// qualifies as a confident result.

	// SAll may contain entries only for 7-bit and Unicode languages,
	// at most one entry per unique Win32 language ID
	
	debug(printf("Selecting scores for result:\n"));

	// Sort by decreasing score

	SAll.SortByScore ();

	// Build the client return structure
	//		Language ID
	//		Code page
	//		Doc percent 0-100
	//		Confidence 0-100

	int nScoresReturned = 0;

	for (unsigned i = 0; i < SAll.NElts() && nScoresReturned < *pnScores; i++)
	{
		CScore &s = SAll.Ref(i);

		LCDScore R;

		R.nLangID = s.GetLang()->LanguageID();
		R.nCodePage = s.GetCodePage();

		// Percent of doc for which this language scored above the
		// confidence threshhold, even if not 1st place for that chunk.

		R.nDocPercent = (s.GetCharCount() * 100) / nProcessed;

		// Confidence is the raw score for all the chunks for which this
		// language was detected above the confidence threshhold, divided
		// by the number of characters in those chunks.
		
		R.nConfidence = s.GetScore() / s.GetCharCount();

		debug(printf("  testing: lang=%d nch=%d docpct=%d\n", R.nLangID,s.GetCharCount(),R.nDocPercent));

		// Return only scores for languages detected in over a
		// minimum % of the doc.

		if (R.nDocPercent > pLCDC->nDocPctThreshhold)
		{
			debug(printf("  returning score\n"));
			paScores[nScoresReturned++] = R;
		}
	}

	debug(printf("Returning %d scores\n", nScoresReturned));

	*pnScores = nScoresReturned;

	return NO_ERROR;
}

/****************************************************************/
/****************************************************************/

#if 0
// Export functions

BOOL APIENTRY 
DllMain (HANDLE hM, DWORD ul_reason, LPVOID lpReserved)
{
	switch (ul_reason) {

	case DLL_PROCESS_ATTACH:
		{
			DisableThreadLibraryCalls( (HINSTANCE)hM );

			LCDetect *pLC = new LCDetect ( (HMODULE)hM );
			if (pLC == NULL)
				return FALSE;

			if (pLC->LoadState() != NO_ERROR)
			{
				delete pLC;
				return FALSE;
			}

			g_pLCDetect = pLC;
		}
		return TRUE;

	case DLL_PROCESS_DETACH:
		if (g_pLCDetect != NULL)
			delete (LCDetect *)g_pLCDetect;
		g_pLCDetect = NULL;
		return TRUE;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}

	return TRUE;
}
#endif 

extern "C" void WINAPI 
LCD_GetConfig (PLCDConfigure pLCDC)
{
	if (g_pLCDetect)
		*pLCDC = g_pLCDetect->GetConfig();
}

extern "C" DWORD WINAPI
LCD_Detect (LPCSTR pStr, int nInputChars, 
	PLCDScore paScores, int *pnScores,
	PCLCDConfigure pLCDC)
// 
// Score into paScores, one score per language, "qualifying" scores only.
// Return ranked by decreasing score.
{
	if (g_pLCDetect == NULL)
		return ERROR_INVALID_FUNCTION;

	return g_pLCDetect->DetectA(pStr, nInputChars, paScores, pnScores, pLCDC);
}

extern "C" DWORD WINAPI
LCD_DetectW (LPCWSTR wcs, int nInputChars,
	PLCDScore paScores, int *pnScores,
	PCLCDConfigure pLCDC)
{
	if (g_pLCDetect == NULL)
		return ERROR_INVALID_FUNCTION;

	return g_pLCDetect->DetectW(wcs, nInputChars, paScores, pnScores, pLCDC);
}

extern "C" void WINAPI
LCD_SetDebug (int f)
{
#ifdef DEBUG_LCDETECT
	g_fDebug = f;
#endif
}
