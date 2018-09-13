#ifndef LSFFI_DEFINED
#define LSFFI_DEFINED


/* Line Services "format flags" (from LSPAP ) */

/* Visi flags	*/
#define fFmiVisiCondHyphens			0x00000001L
#define fFmiVisiParaMarks			0x00000002L
#define fFmiVisiSpaces				0x00000004L
#define fFmiVisiTabs				0x00000008L
#define fFmiVisiSplats				0x00000010L
#define fFmiVisiBreaks				0x00000020L


/* Advanced typography enabling    */
#define fFmiPunctStartLine			0x00000040L
#define fFmiHangingPunct			0x00000080L
#define fFmiApplyBreakingRules		0x00000100L

/* WYSIWYG flags */
#define fFmiPresSuppressWiggle		0x00000200L
#define fFmiPresExactSync			0x00000400L

/* Autonumbering flags */
#define fFmiAnm						0x00000800L

/* Miscellaneous flags */
#define fFmiAutoDecimalTab			0x00001000L
#define fFmiUnderlineTrailSpacesRM	0x00002000L

#define fFmiSpacesInfluenceHeight	0x00004000L

#define fFmiIgnoreSplatBreak		0x00010000L
#define fFmiLimSplat				0x00020000L
#define fFmiAllowSplatLine			0x00040000L

#define	fFmiForceBreakAsNext		0x00080000L
#define fFmiFCheckTruncateBefore	0x00100000L

#define fFmiDoHyphenation			0x00200000L

#define fFmiDrawInCharCodes			0x00400000L

#define	fFmiTreatHyphenAsRegular	0x00800000L
#define fFmiWrapTrailingSpaces		0x01000000L
#define fFmiWrapAllSpaces			0x02000000L

/* Compatibility flags for bugs in older versions of WORD */
#define fFmiForgetLastTabAlignment	0x10000000L
#define fFmiIndentChangesHyphenZone	0x20000000L
#define fFmiNoPunctAfterAutoNumber	0x40000000L
#define fFmiResolveTabsAsWord97		0x80000000L

#endif /* !LSFFI_DEFINED */
