#ifndef _FONTLINK_H_

#define SPACE_CHAR       0x20
#define EURODOLLAR_CHAR  0x20A0 // New Euro dollar symbol
#define CAPZCARON_CHAR   0x017D
#define SMALLZCARON_CHAR 0x017E

//
//  Unicode SubRange (USR) definitions
//
#define usrBasicLatin           0       // 0x20->0x7f
#define usrLatin1               1       // 0xa0->0xff
#define usrLatinXA              2       // 0x100->0x17f
#define usrLatinXB              3       // 0x180->0x24f
#define usrIPAExtensions        4       // 0x250->0x2af
#define usrSpacingModLetters    5       // 0x2b0->0x2ff
#define usrCombDiacritical      6       // 0x300->0x36f
#define usrBasicGreek           7       // 0x370->0x3cf
#define usrGreekSymbolsCop      8       // 0x3d0->0x3ff
#define usrCyrillic             9       // 0x400->0x4ff
#define usrArmenian             10      // 0x500->0x58f
#define usrBasicHebrew          11      // 0x5d0->0x5ff
#define usrHebrewXA             12      // 0x590->0x5cf
#define usrBasicArabic          13      // 0x600->0x652
#define usrArabicX              14      // 0x653->0x6ff
#define usrDevangari            15      // 0x900->0x97f
#define usrBengali              16      // 0x980->0x9ff
#define usrGurmukhi             17      // 0xa00->0xa7f
#define usrGujarati             18      // 0xa80->0xaff
#define usrOriya                19      // 0xb00->0xb7f
#define usrTamil                20      // 0x0B80->0x0BFF
#define usrTelugu               21      // 0x0C00->0x0C7F
#define usrKannada              22      // 0x0C80->0x0CFF
#define usrMalayalam            23      // 0x0D00->0x0D7F
#define usrThai                 24      // 0x0E00->0x0E7F
#define usrLao                  25      // 0x0E80->0x0EFF
#define usrBasicGeorgian        26      // 0x10D0->0x10FF
#define usrGeorgianExtended     27      // 0x10A0->0x10CF
#define usrHangulJamo           28      // 0x1100->0x11FF
#define usrLatinExtendedAdd     29      // 0x1E00->0x1EFF
#define usrGreekExtended        30      // 0x1F00->0x1FFF
#define usrGeneralPunct         31      // 0x2000->0x206F
#define usrSuperAndSubscript    32      // 0x2070->0x209F
#define usrCurrencySymbols      33      // 0x20A0->0x20CF
#define usrCombDiacriticsS      34      // 0x20D0->0x20FF   
#define usrLetterlikeSymbols    35      // 0x2100->0x214F   
#define usrNumberForms          36      // 0x2150->0x218F   
#define usrArrows               37      // 0x2190->0x21FF   
#define usrMathematicalOps      38      // 0x2200->0x22FF   
#define usrMiscTechnical        39      // 0x2300->0x23FF   
#define usrControlPictures      40      // 0x2400->0x243F   
#define usrOpticalCharRecog     41      // 0x2440->0x245F   
#define usrEnclosedAlphanum     42      // 0x2460->0x24FF   
#define usrBoxDrawing           43      // 0x2500->0x257F   
#define usrBlockElements        44      // 0x2580->0x259F   
#define usrGeometricShapes      45      // 0x25A0->0x25FF   
#define usrMiscDingbats         46      // 0x2600->0x26FF   
#define usrDingbats             47      // 0x2700->0x27BF   
#define usrCJKSymAndPunct       48      // 0x3000->0x303F   
#define usrHiragana             49      // 0x3040->0x309F   
#define usrKatakana             50      // 0x30A0->0x30FF   
#define usrBopomofo             51      // 0x3100->0x312F   
#define usrHangulCompatJamo     52      // 0x3130->0x318F   
#define usrCJKMisc              53      // 0x3190->0x319F   
#define usrEnclosedCJKLtMnth    54      // 0x3200->0x32FF   
#define usrCJKCompatibility     55      // 0x3300->0x33FF   
#define usrHangul               56      // 0xac00->0xd7a3
#define usrReserved1            57
#define usrReserved2            58
#define usrCJKUnifiedIdeo       59      // 0x4E00->0x9FFF   
#define usrPrivateUseArea       60      // 0xE000->0xF8FF   
#define usrCJKCompatibilityIdeographs   61      // 0xF900->0xFAFF   
#define usrAlphaPresentationForms       62      // 0xFB00->0xFB4F   
#define usrArabicPresentationFormsA     63      // 0xFB50->0xFDFF   
#define usrCombiningHalfMarks           64      // 0xFE20->0xFE2F   
#define usrCJKCompatForms               65      // 0xFE30->0xFE4F   
#define usrSmallFormVariants            66      // 0xFE50->0xFE6F   
#define usrArabicPresentationFormsB     67      // 0xFE70->0xFEFE   
#define usrHFWidthForms                 68      // 0xFF00->0xFFEF   
#define usrSpecials                     69      // 0xFFF0->0xFFFD   
#define usrMax                          70

#define FBetween(a, b, c)  (((unsigned)((a) - (b))) <= (c) - (b))

#endif  // _FONTLINK_H_