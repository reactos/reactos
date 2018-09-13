//
// Generating script: unicodepartition_makeheader.pl
// Generated on Mon Nov 16 12:10:33 1998
//
// This is a generated file.  Do not modify by hand.
//

#ifndef I__UNIPART_H_
#define I__UNIPART_H_
#pragma INCMSG("--- Beg 'unipart.h'")

typedef BYTE CHAR_CLASS;

#define WOB_ 0      //   1 - Open Brackets for inline-note (JIS 1 or 19)
#define NOPP 1      //   2 - Open parenthesis (JIS 1)
#define NOPA 2      //   2 - Open parenthesis (JIS 1)
#define NOPW 3      //   2 - Open parenthesis (JIS 1)
#define HOP_ 4      //   3 - Open parenthesis (JIS 1)
#define WOP_ 5      //   4 - Open parenthesis (JIS 1)
#define WOP5 6      //   5 - Open parenthesis, Big 5 (JIS 1)
#define NOQW 7      //   6 - Open quotes (JIS 1)
#define AOQW 8      //   7 - Open quotes (JIS 1)
#define WOQ_ 9      //   8 - Open quotes (JIS 1)
#define WCB_ 10     //   9 - Close brackets for inline-note (JIS 2 or 20)
#define NCPP 11     //  10 - Close parenthesis (JIS 2)
#define NCPA 12     //  10 - Close parenthesis (JIS 2)
#define NCPW 13     //  10 - Close parenthesis (JIS 2)
#define HCP_ 14     //  11 - Close parenthesis (JIS 2)
#define WCP_ 15     //  12 - Close parenthesis (JIS 2)
#define WCP5 16     //  13 - Close parenthesis, Big 5 (JIS 2)
#define NCQW 17     //  14 - Close quotes (JIS 2)
#define ACQW 18     //  15 - Close quotes (JIS 2)
#define WCQ_ 19     //  16 - Close quotes (JIS 2)
#define ARQW 20     //  17 - Right single quotation mark (JIS 2)
#define NCSA 21     //  18 - Comma (JIS 2 or 15)
#define HCO_ 22     //  19 - Comma (JIS 2 or 15)
#define WC__ 23     //  20 - Comma (JIS 2)
#define WCS_ 24     //  20 - Comma (JIS 2)
#define WC5_ 25     //  21 - Comma, Big 5 (JIS 2)
#define WC5S 26     //  21 - Comma, Big 5 (JIS 2)
#define NKS_ 27     //  22 - Kana sound marks (JIS 3)
#define WKSM 28     //  23 - Kana sound marks (JIS 3)
#define WIM_ 29     //  24 - Iteration marks (JIS 3)
#define NSSW 30     //  25 - Symbols which cannot start a line (JIS 3)
#define WSS_ 31     //  26 - Symbols that cannot start a line (JIS 3)
#define WHIM 32     //  27 - Hiragana iteration marks (JIS 3)
#define WKIM 33     //  28 - Katakana iteration marks (JIS 3)
#define NKSL 34     //  29 - Katakana that cannot start a line (JIS 3)
#define WKS_ 35     //  30 - Katakana that cannot start a line (JIS 3)
#define WKSC 36     //  30 - Katakana that cannot start a line (JIS 3)
#define WHS_ 37     //  31 - Hiragana that cannot start a line (JIS 3)
#define NQFP 38     //  32 - Question/Exclamation (JIS 4)
#define NQFA 39     //  32 - Question/Exclamation (JIS 4)
#define WQE_ 40     //  33 - Question/Exclamation (JIS 4)
#define WQE5 41     //  34 - Question/Exclamation, Big 5 (JIS 4)
#define NKCC 42     //  35 - Kana centered characters (JIS 5)
#define WKC_ 43     //  36 - Kana centered characters (JIS 5)
#define NOCP 44     //  37 - Other centered characters (JIS 5)
#define NOCA 45     //  37 - Other centered characters (JIS 5)
#define NOCW 46     //  37 - Other centered characters (JIS 5)
#define WOC_ 47     //  38 - Other centered characters (JIS 5)
#define WOCS 48     //  38 - Other centered characters (JIS 5)
#define WOC5 49     //  39 - Other centered characters, Big 5 (JIS 5)
#define WOC6 50     //  39 - Other centered characters, Big 5 (JIS 5)
#define AHPW 51     //  40 - Hyphenation point (JIS 5)
#define NPEP 52     //  41 - Period (JIS 6 or 15)
#define NPAR 53     //  41 - Period (JIS 6 or 15)
#define HPE_ 54     //  42 - Period (JIS 6 or 15)
#define WPE_ 55     //  43 - Period (JIS 6)
#define WPES 56     //  43 - Period (JIS 6)
#define WPE5 57     //  44 - Period, Big 5 (JIS 6)
#define NISW 58     //  45 - Inseparable characters (JIS 7)
#define AISW 59     //  46 - Inseparable characters (JIS 7)
#define NQCS 60     //  47 - Glue characters (no JIS)
#define NQCW 61     //  47 - Glue characters (no JIS)
#define NQCC 62     //  47 - Glue characters (no JIS)
#define NPTA 63     //  48 - Prefix currencies and symbols (JIS 8)
#define NPNA 64     //  48 - Prefix currencies and symbols (JIS 8)
#define NPEW 65     //  48 - Prefix currencies and symbols (JIS 8)
#define NPEH 66     //  48 - Prefix currencies and symbols (JIS 8)
#define APNW 67     //  49 - Prefix currencies and symbols (JIS 8)
#define HPEW 68     //  50 - Prefix currencies and symbols (JIS 8)
#define WPR_ 69     //  51 - Prefix currencies and symbols (JIS 8)
#define NQEP 70     //  52 - Postfix currencies and symbols (JIS 9)
#define NQEW 71     //  52 - Postfix currencies and symbols (JIS 9)
#define NQNW 72     //  52 - Postfix currencies and symbols (JIS 9)
#define AQEW 73     //  53 - Postfix currencies and symbols (JIS 9)
#define AQNW 74     //  53 - Postfix currencies and symbols (JIS 9)
#define AQLW 75     //  53 - Postfix currencies and symbols (JIS 9)
#define WQO_ 76     //  54 - Postfix currencies and symbols (JIS 9)
#define NSBL 77     //  55 - Space(JIS 15 or 17)
#define WSP_ 78     //  56 - Space (JIS 10)
#define WHI_ 79     //  57 - Hiragana except small letters (JIS 11)
#define NKA_ 80     //  58 - Katakana except small letters Ideographic (JIS 12)
#define WKA_ 81     //  59 - Katakana except small letters (JIS 12)
#define ASNW 82     //  60 - Ambiguous symbols (JIS 12 or 18) 
#define ASEW 83     //  60 - Ambiguous symbols (JIS 12 or 18) 
#define ASRN 84     //  60 - Ambiguous symbols (JIS 12 or 18) 
#define ASEN 85     //  60 - Ambiguous symbols (JIS 12 or 18) 
#define ALA_ 86     //  61 - Ambiguous Latin (JIS 12 or 18) 
#define AGR_ 87     //  62 - Ambiguous Greek (JIS 12 or 18) 
#define ACY_ 88     //  63 - Ambiguous Cyrillic (JIS 12 or 18) 
#define WID_ 89     //  64 - Han Ideographs (JIS 12, 14S or 14D)
#define WPUA 90     //  65 - End user defined characters (JIS 12, 14S or 14D)
#define NHG_ 91     //  66 - Hangul Ideographs (JIS 12)
#define WHG_ 92     //  67 - Hangul Ideographs (JIS 12)
#define WCI_ 93     //  68 - Compatibility Ideographs (JIS 12)
#define NOI_ 94     //  69 - Other Ideographs (JIS 12)
#define WOI_ 95     //  70 - Other Ideographs (JIS 12)
#define WOIC 96     //  70 - Other Ideographs (JIS 12)
#define WOIL 97     //  70 - Other Ideographs (JIS 12)
#define WOIS 98     //  70 - Other Ideographs (JIS 12)
#define WOIT 99     //  70 - Other Ideographs (JIS 12)
#define NSEN 100    //  71 - Superscript/Subscript/Attachments (JIS 13)
#define NSET 101    //  71 - Superscript/Subscript/Attachments (JIS 13)
#define NSNW 102    //  71 - Superscript/Subscript/Attachments (JIS 13)
#define ASAN 103    //  72 - Superscript/Subscript/Attachments (JIS 13)
#define ASAE 104    //  72 - Superscript/Subscript/Attachments (JIS 13)
#define NDEA 105    //  73 - Digits (JIS 15 or 18)
#define WD__ 106    //  74 - Digits (JIS 15 or 18)
#define NLLA 107    //  75 - Basic Latin (JIS 16 or 18)
#define WLA_ 108    //  76 - Basic Latin (JIS 16 or 18)
#define NWBL 109    //  77 - Word breaking Spaces (JIS 17)
#define NWZW 110    //  77 - Word breaking Spaces (JIS 17)
#define NPLW 111    //  78 - Punctuation in Text (JIS 18)
#define NPZW 112    //  78 - Punctuation in Text (JIS 18)
#define NPF_ 113    //  78 - Punctuation in Text (JIS 18)
#define NPFL 114    //  78 - Punctuation in Text (JIS 18)
#define NPNW 115    //  78 - Punctuation in Text (JIS 18)
#define APLW 116    //  79 - Punctuation in text (JIS 12 or 18)
#define APCO 117    //  79 - Punctuation in text (JIS 12 or 18)
#define ASYW 118    //  80 - Soft Hyphen (JIS 12 or 18)
#define NHYP 119    //  81 - Hyphen (JIS 18)
#define NHYW 120    //  81 - Hyphen (JIS 18)
#define AHYW 121    //  82 - Hyphen (JIS 12 or 18)
#define NAPA 122    //  83 - Apostrophe (JIS 18)
#define NQMP 123    //  84 - Quotation mark (JIS 18)
#define NSLS 124    //  85 - Slash (JIS 18)
#define NSF_ 125    //  86 - Non space word break (JIS 18)
#define NSBS 126    //  86 - Non space word break (JIS 18)
#define NLA_ 127    //  87 - Latin (JIS 18)
#define NLQ_ 128    //  88 - Latin Punctuation in text (JIS 18)
#define NLQN 129    //  88 - Latin Punctuation in text (JIS 18)
#define ALQ_ 130    //  89 - Latin Punctuation in text (JIS 12 or 18)
#define NGR_ 131    //  90 - Greek (JIS 18)
#define NGRN 132    //  90 - Greek (JIS 18)
#define NGQ_ 133    //  91 - Greek Punctuation in text (JIS 18)
#define NGQN 134    //  91 - Greek Punctuation in text (JIS 18)
#define NCY_ 135    //  92 - Cyrillic (JIS 18)
#define NCYP 136    //  93 - Cyrillic Punctuation in text (JIS 18)
#define NCYC 137    //  93 - Cyrillic Punctuation in text (JIS 18)
#define NAR_ 138    //  94 - Armenian (JIS 18)
#define NAQN 139    //  95 - Armenian Punctuation in text (JIS 18)
#define NHB_ 140    //  96 - Hebrew (JIS 18)
#define NHBC 141    //  96 - Hebrew (JIS 18)
#define NHBW 142    //  96 - Hebrew (JIS 18)
#define NHBR 143    //  96 - Hebrew (JIS 18)
#define NASR 144    //  97 - Arabic (JIS 18)
#define NAAR 145    //  97 - Arabic (JIS 18)
#define NAAC 146    //  97 - Arabic (JIS 18)
#define NAAD 147    //  97 - Arabic (JIS 18)
#define NAED 148    //  97 - Arabic (JIS 18)
#define NANW 149    //  97 - Arabic (JIS 18)
#define NAEW 150    //  97 - Arabic (JIS 18)
#define NAAS 151    //  97 - Arabic (JIS 18)
#define NHI_ 152    //  98 - Devanagari (JIS 18)
#define NHIN 153    //  98 - Devanagari (JIS 18)
#define NHIC 154    //  98 - Devanagari (JIS 18)
#define NHID 155    //  98 - Devanagari (JIS 18)
#define NBE_ 156    //  99 - Bengali (JIS 18)
#define NBEC 157    //  99 - Bengali (JIS 18)
#define NBED 158    //  99 - Bengali (JIS 18)
#define NGM_ 159    // 100 - Gurmukhi (JIS 18)
#define NGMC 160    // 100 - Gurmukhi (JIS 18)
#define NGMD 161    // 100 - Gurmukhi (JIS 18)
#define NGJ_ 162    // 101 - Gujarati (JIS 18)
#define NGJC 163    // 101 - Gujarati (JIS 18)
#define NGJD 164    // 101 - Gujarati (JIS 18)
#define NOR_ 165    // 102 - Oriya (JIS 18)
#define NORC 166    // 102 - Oriya (JIS 18)
#define NORD 167    // 102 - Oriya (JIS 18)
#define NTA_ 168    // 103 - Tamil (JIS 18)
#define NTAC 169    // 103 - Tamil (JIS 18)
#define NTAD 170    // 103 - Tamil (JIS 18)
#define NTE_ 171    // 104 - Telugu (JIS 18)
#define NTEC 172    // 104 - Telugu (JIS 18)
#define NTED 173    // 104 - Telugu (JIS 18)
#define NKD_ 174    // 105 - Kannada (JIS 18)
#define NKDC 175    // 105 - Kannada (JIS 18)
#define NKDD 176    // 105 - Kannada (JIS 18)
#define NMA_ 177    // 106 - Malayalam (JIS 18)
#define NMAC 178    // 106 - Malayalam (JIS 18)
#define NMAD 179    // 106 - Malayalam (JIS 18)
#define NTH_ 180    // 107 - Thai (JIS 18) 
#define NTHC 181    // 107 - Thai (JIS 18) 
#define NTHD 182    // 107 - Thai (JIS 18) 
#define NTHT 183    // 107 - Thai (JIS 18) 
#define NLO_ 184    // 108 - Lao (JIS 18)
#define NLOC 185    // 108 - Lao (JIS 18)
#define NLOD 186    // 108 - Lao (JIS 18)
#define NTI_ 187    // 109 - Tibetan (JIS 18)
#define NTIC 188    // 109 - Tibetan (JIS 18)
#define NTID 189    // 109 - Tibetan (JIS 18)
#define NGE_ 190    // 110 - Georgian (JIS 18)
#define NGEQ 191    // 111 - Georgian Punctuation in text (JIS 18)
#define NBO_ 192    // 112 - Bopomofo (JIS 18)
#define NBSP 193    // 113 - No Break space (no JIS) 
#define NOF_ 194    // 114 - Other symbols (JIS 18)
#define NOBS 195    // 114 - Other symbols (JIS 18)
#define NOEA 196    // 114 - Other symbols (JIS 18)
#define NONA 197    // 114 - Other symbols (JIS 18)
#define NONP 198    // 114 - Other symbols (JIS 18)
#define NOEP 199    // 114 - Other symbols (JIS 18)
#define NONW 200    // 114 - Other symbols (JIS 18)
#define NOEW 201    // 114 - Other symbols (JIS 18)
#define NOLW 202    // 114 - Other symbols (JIS 18)
#define NOCO 203    // 114 - Other symbols (JIS 18)
#define NOSP 204    // 114 - Other symbols (JIS 18)
#define NOEN 205    // 114 - Other symbols (JIS 18)
#define NET_ 206    // 115 - Ethiopic
#define NCA_ 207    // 116 - Canadian Syllabics
#define NCH_ 208    // 117 - Cherokee
#define WYI_ 209    // 118 - Yi
#define NBR_ 210    // 119 - Braille
#define NRU_ 211    // 120 - Runic
#define NOG_ 212    // 121 - Ogham
#define NSI_ 213    // 122 - Sinhala
#define NSIC 214    // 122 - Sinhala
#define NTN_ 215    // 123 - Thaana
#define NTNC 216    // 123 - Thaana
#define NKH_ 217    // 124 - Khmer
#define NKHC 218    // 124 - Khmer
#define NKHD 219    // 124 - Khmer
#define NBU_ 220    // 125 - Burmese/Myanmar
#define NBUC 221    // 125 - Burmese/Myanmar
#define NBUD 222    // 125 - Burmese/Myanmar
#define NSY_ 223    // 126 - Syriac
#define NSYC 224    // 126 - Syriac
#define NSYW 225    // 126 - Syriac
#define NMO_ 226    // 127 - Mongolian
#define NMOC 227    // 127 - Mongolian
#define NMOD 228    // 127 - Mongolian
#define NHS_ 229    // 128 - High Surrogate
#define WHT_ 230    // 129 - High Surrogate
#define LS__ 231    // 130 - Low Surrogate
#define XNW_ 232    // 131 - Unassigned

#define CHAR_CLASS_MAX 233

#define __WOB_ (CHAR_CLASS *)WOB_
#define __NOPP (CHAR_CLASS *)NOPP
#define __NOPA (CHAR_CLASS *)NOPA
#define __NOPW (CHAR_CLASS *)NOPW
#define __HOP_ (CHAR_CLASS *)HOP_
#define __WOP_ (CHAR_CLASS *)WOP_
#define __WOP5 (CHAR_CLASS *)WOP5
#define __NOQW (CHAR_CLASS *)NOQW
#define __AOQW (CHAR_CLASS *)AOQW
#define __WOQ_ (CHAR_CLASS *)WOQ_
#define __WCB_ (CHAR_CLASS *)WCB_
#define __NCPP (CHAR_CLASS *)NCPP
#define __NCPA (CHAR_CLASS *)NCPA
#define __NCPW (CHAR_CLASS *)NCPW
#define __HCP_ (CHAR_CLASS *)HCP_
#define __WCP_ (CHAR_CLASS *)WCP_
#define __WCP5 (CHAR_CLASS *)WCP5
#define __NCQW (CHAR_CLASS *)NCQW
#define __ACQW (CHAR_CLASS *)ACQW
#define __WCQ_ (CHAR_CLASS *)WCQ_
#define __ARQW (CHAR_CLASS *)ARQW
#define __NCSA (CHAR_CLASS *)NCSA
#define __HCO_ (CHAR_CLASS *)HCO_
#define __WC__ (CHAR_CLASS *)WC__
#define __WCS_ (CHAR_CLASS *)WCS_
#define __WC5_ (CHAR_CLASS *)WC5_
#define __WC5S (CHAR_CLASS *)WC5S
#define __NKS_ (CHAR_CLASS *)NKS_
#define __WKSM (CHAR_CLASS *)WKSM
#define __WIM_ (CHAR_CLASS *)WIM_
#define __NSSW (CHAR_CLASS *)NSSW
#define __WSS_ (CHAR_CLASS *)WSS_
#define __WHIM (CHAR_CLASS *)WHIM
#define __WKIM (CHAR_CLASS *)WKIM
#define __NKSL (CHAR_CLASS *)NKSL
#define __WKS_ (CHAR_CLASS *)WKS_
#define __WKSC (CHAR_CLASS *)WKSC
#define __WHS_ (CHAR_CLASS *)WHS_
#define __NQFP (CHAR_CLASS *)NQFP
#define __NQFA (CHAR_CLASS *)NQFA
#define __WQE_ (CHAR_CLASS *)WQE_
#define __WQE5 (CHAR_CLASS *)WQE5
#define __NKCC (CHAR_CLASS *)NKCC
#define __WKC_ (CHAR_CLASS *)WKC_
#define __NOCP (CHAR_CLASS *)NOCP
#define __NOCA (CHAR_CLASS *)NOCA
#define __NOCW (CHAR_CLASS *)NOCW
#define __WOC_ (CHAR_CLASS *)WOC_
#define __WOCS (CHAR_CLASS *)WOCS
#define __WOC5 (CHAR_CLASS *)WOC5
#define __WOC6 (CHAR_CLASS *)WOC6
#define __AHPW (CHAR_CLASS *)AHPW
#define __NPEP (CHAR_CLASS *)NPEP
#define __NPAR (CHAR_CLASS *)NPAR
#define __HPE_ (CHAR_CLASS *)HPE_
#define __WPE_ (CHAR_CLASS *)WPE_
#define __WPES (CHAR_CLASS *)WPES
#define __WPE5 (CHAR_CLASS *)WPE5
#define __NISW (CHAR_CLASS *)NISW
#define __AISW (CHAR_CLASS *)AISW
#define __NQCS (CHAR_CLASS *)NQCS
#define __NQCW (CHAR_CLASS *)NQCW
#define __NQCC (CHAR_CLASS *)NQCC
#define __NPTA (CHAR_CLASS *)NPTA
#define __NPNA (CHAR_CLASS *)NPNA
#define __NPEW (CHAR_CLASS *)NPEW
#define __NPEH (CHAR_CLASS *)NPEH
#define __APNW (CHAR_CLASS *)APNW
#define __HPEW (CHAR_CLASS *)HPEW
#define __WPR_ (CHAR_CLASS *)WPR_
#define __NQEP (CHAR_CLASS *)NQEP
#define __NQEW (CHAR_CLASS *)NQEW
#define __NQNW (CHAR_CLASS *)NQNW
#define __AQEW (CHAR_CLASS *)AQEW
#define __AQNW (CHAR_CLASS *)AQNW
#define __AQLW (CHAR_CLASS *)AQLW
#define __WQO_ (CHAR_CLASS *)WQO_
#define __NSBL (CHAR_CLASS *)NSBL
#define __WSP_ (CHAR_CLASS *)WSP_
#define __WHI_ (CHAR_CLASS *)WHI_
#define __NKA_ (CHAR_CLASS *)NKA_
#define __WKA_ (CHAR_CLASS *)WKA_
#define __ASNW (CHAR_CLASS *)ASNW
#define __ASEW (CHAR_CLASS *)ASEW
#define __ASRN (CHAR_CLASS *)ASRN
#define __ASEN (CHAR_CLASS *)ASEN
#define __ALA_ (CHAR_CLASS *)ALA_
#define __AGR_ (CHAR_CLASS *)AGR_
#define __ACY_ (CHAR_CLASS *)ACY_
#define __WID_ (CHAR_CLASS *)WID_
#define __WPUA (CHAR_CLASS *)WPUA
#define __NHG_ (CHAR_CLASS *)NHG_
#define __WHG_ (CHAR_CLASS *)WHG_
#define __WCI_ (CHAR_CLASS *)WCI_
#define __NOI_ (CHAR_CLASS *)NOI_
#define __WOI_ (CHAR_CLASS *)WOI_
#define __WOIC (CHAR_CLASS *)WOIC
#define __WOIL (CHAR_CLASS *)WOIL
#define __WOIS (CHAR_CLASS *)WOIS
#define __WOIT (CHAR_CLASS *)WOIT
#define __NSEN (CHAR_CLASS *)NSEN
#define __NSET (CHAR_CLASS *)NSET
#define __NSNW (CHAR_CLASS *)NSNW
#define __ASAN (CHAR_CLASS *)ASAN
#define __ASAE (CHAR_CLASS *)ASAE
#define __NDEA (CHAR_CLASS *)NDEA
#define __WD__ (CHAR_CLASS *)WD__
#define __NLLA (CHAR_CLASS *)NLLA
#define __WLA_ (CHAR_CLASS *)WLA_
#define __NWBL (CHAR_CLASS *)NWBL
#define __NWZW (CHAR_CLASS *)NWZW
#define __NPLW (CHAR_CLASS *)NPLW
#define __NPZW (CHAR_CLASS *)NPZW
#define __NPF_ (CHAR_CLASS *)NPF_
#define __NPFL (CHAR_CLASS *)NPFL
#define __NPNW (CHAR_CLASS *)NPNW
#define __APLW (CHAR_CLASS *)APLW
#define __APCO (CHAR_CLASS *)APCO
#define __ASYW (CHAR_CLASS *)ASYW
#define __NHYP (CHAR_CLASS *)NHYP
#define __NHYW (CHAR_CLASS *)NHYW
#define __AHYW (CHAR_CLASS *)AHYW
#define __NAPA (CHAR_CLASS *)NAPA
#define __NQMP (CHAR_CLASS *)NQMP
#define __NSLS (CHAR_CLASS *)NSLS
#define __NSF_ (CHAR_CLASS *)NSF_
#define __NSBS (CHAR_CLASS *)NSBS
#define __NLA_ (CHAR_CLASS *)NLA_
#define __NLQ_ (CHAR_CLASS *)NLQ_
#define __NLQN (CHAR_CLASS *)NLQN
#define __ALQ_ (CHAR_CLASS *)ALQ_
#define __NGR_ (CHAR_CLASS *)NGR_
#define __NGRN (CHAR_CLASS *)NGRN
#define __NGQ_ (CHAR_CLASS *)NGQ_
#define __NGQN (CHAR_CLASS *)NGQN
#define __NCY_ (CHAR_CLASS *)NCY_
#define __NCYP (CHAR_CLASS *)NCYP
#define __NCYC (CHAR_CLASS *)NCYC
#define __NAR_ (CHAR_CLASS *)NAR_
#define __NAQN (CHAR_CLASS *)NAQN
#define __NHB_ (CHAR_CLASS *)NHB_
#define __NHBC (CHAR_CLASS *)NHBC
#define __NHBW (CHAR_CLASS *)NHBW
#define __NHBR (CHAR_CLASS *)NHBR
#define __NASR (CHAR_CLASS *)NASR
#define __NAAR (CHAR_CLASS *)NAAR
#define __NAAC (CHAR_CLASS *)NAAC
#define __NAAD (CHAR_CLASS *)NAAD
#define __NAED (CHAR_CLASS *)NAED
#define __NANW (CHAR_CLASS *)NANW
#define __NAEW (CHAR_CLASS *)NAEW
#define __NAAS (CHAR_CLASS *)NAAS
#define __NHI_ (CHAR_CLASS *)NHI_
#define __NHIN (CHAR_CLASS *)NHIN
#define __NHIC (CHAR_CLASS *)NHIC
#define __NHID (CHAR_CLASS *)NHID
#define __NBE_ (CHAR_CLASS *)NBE_
#define __NBEC (CHAR_CLASS *)NBEC
#define __NBED (CHAR_CLASS *)NBED
#define __NGM_ (CHAR_CLASS *)NGM_
#define __NGMC (CHAR_CLASS *)NGMC
#define __NGMD (CHAR_CLASS *)NGMD
#define __NGJ_ (CHAR_CLASS *)NGJ_
#define __NGJC (CHAR_CLASS *)NGJC
#define __NGJD (CHAR_CLASS *)NGJD
#define __NOR_ (CHAR_CLASS *)NOR_
#define __NORC (CHAR_CLASS *)NORC
#define __NORD (CHAR_CLASS *)NORD
#define __NTA_ (CHAR_CLASS *)NTA_
#define __NTAC (CHAR_CLASS *)NTAC
#define __NTAD (CHAR_CLASS *)NTAD
#define __NTE_ (CHAR_CLASS *)NTE_
#define __NTEC (CHAR_CLASS *)NTEC
#define __NTED (CHAR_CLASS *)NTED
#define __NKD_ (CHAR_CLASS *)NKD_
#define __NKDC (CHAR_CLASS *)NKDC
#define __NKDD (CHAR_CLASS *)NKDD
#define __NMA_ (CHAR_CLASS *)NMA_
#define __NMAC (CHAR_CLASS *)NMAC
#define __NMAD (CHAR_CLASS *)NMAD
#define __NTH_ (CHAR_CLASS *)NTH_
#define __NTHC (CHAR_CLASS *)NTHC
#define __NTHD (CHAR_CLASS *)NTHD
#define __NTHT (CHAR_CLASS *)NTHT
#define __NLO_ (CHAR_CLASS *)NLO_
#define __NLOC (CHAR_CLASS *)NLOC
#define __NLOD (CHAR_CLASS *)NLOD
#define __NTI_ (CHAR_CLASS *)NTI_
#define __NTIC (CHAR_CLASS *)NTIC
#define __NTID (CHAR_CLASS *)NTID
#define __NGE_ (CHAR_CLASS *)NGE_
#define __NGEQ (CHAR_CLASS *)NGEQ
#define __NBO_ (CHAR_CLASS *)NBO_
#define __NBSP (CHAR_CLASS *)NBSP
#define __NOF_ (CHAR_CLASS *)NOF_
#define __NOBS (CHAR_CLASS *)NOBS
#define __NOEA (CHAR_CLASS *)NOEA
#define __NONA (CHAR_CLASS *)NONA
#define __NONP (CHAR_CLASS *)NONP
#define __NOEP (CHAR_CLASS *)NOEP
#define __NONW (CHAR_CLASS *)NONW
#define __NOEW (CHAR_CLASS *)NOEW
#define __NOLW (CHAR_CLASS *)NOLW
#define __NOCO (CHAR_CLASS *)NOCO
#define __NOSP (CHAR_CLASS *)NOSP
#define __NOEN (CHAR_CLASS *)NOEN
#define __NET_ (CHAR_CLASS *)NET_
#define __NCA_ (CHAR_CLASS *)NCA_
#define __NCH_ (CHAR_CLASS *)NCH_
#define __WYI_ (CHAR_CLASS *)WYI_
#define __NBR_ (CHAR_CLASS *)NBR_
#define __NRU_ (CHAR_CLASS *)NRU_
#define __NOG_ (CHAR_CLASS *)NOG_
#define __NSI_ (CHAR_CLASS *)NSI_
#define __NSIC (CHAR_CLASS *)NSIC
#define __NTN_ (CHAR_CLASS *)NTN_
#define __NTNC (CHAR_CLASS *)NTNC
#define __NKH_ (CHAR_CLASS *)NKH_
#define __NKHC (CHAR_CLASS *)NKHC
#define __NKHD (CHAR_CLASS *)NKHD
#define __NBU_ (CHAR_CLASS *)NBU_
#define __NBUC (CHAR_CLASS *)NBUC
#define __NBUD (CHAR_CLASS *)NBUD
#define __NSY_ (CHAR_CLASS *)NSY_
#define __NSYC (CHAR_CLASS *)NSYC
#define __NSYW (CHAR_CLASS *)NSYW
#define __NMO_ (CHAR_CLASS *)NMO_
#define __NMOC (CHAR_CLASS *)NMOC
#define __NMOD (CHAR_CLASS *)NMOD
#define __NHS_ (CHAR_CLASS *)NHS_
#define __WHT_ (CHAR_CLASS *)WHT_
#define __LS__ (CHAR_CLASS *)LS__
#define __XNW_ (CHAR_CLASS *)XNW_

extern const CHAR_CLASS acc_00[256];

#pragma INCMSG("--- End 'unipart.h'")
#else
#pragma INCMSG("*** Dup 'unipart.h'")
#endif
