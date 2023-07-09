/*
 *  @doc    INTERNAL
 *
 *  @module UNIPROP.CXX -- Miscellaneous Unicode partition properties
 *
 *
 *  Owner: <nl>
 *      Michael Jochimsen <nl>
 *
 *  History: <nl>
 *      11/30/98     mikejoch created
 *
 *  Copyright (c) 1997-1998 Microsoft Corporation. All rights reserved.
 */

#include "headers.hxx"

#ifndef X__UNIPROP_H_
#define X__UNIPROP_H_
#include "uniprop.h"
#endif

#ifndef X__UNIPART_H
#define X__UNIPART_H
#include <unipart.h>
#endif

const UNIPROP s_aPropBitsFromCharClass[CHAR_CLASS_MAX] =
{
    // CC               fNeedsGlyphing  fCombiningMark  fZeroWidth
    /* WOB_   1*/   {   FALSE,          FALSE,          FALSE,  },
    /* NOPP   2*/   {   FALSE,          FALSE,          FALSE,  },
    /* NOPA   2*/   {   FALSE,          FALSE,          FALSE,  },
    /* NOPW   2*/   {   FALSE,          FALSE,          FALSE,  },
    /* HOP_   3*/   {   FALSE,          FALSE,          FALSE,  },
    /* WOP_   4*/   {   FALSE,          FALSE,          FALSE,  },
    /* WOP5   5*/   {   FALSE,          FALSE,          FALSE,  },
    /* NOQW   6*/   {   FALSE,          FALSE,          FALSE,  },
    /* AOQW   7*/   {   FALSE,          FALSE,          FALSE,  },
    /* WOQ_   8*/   {   FALSE,          FALSE,          FALSE,  },
    /* WCB_   9*/   {   FALSE,          FALSE,          FALSE,  },
    /* NCPP  10*/   {   FALSE,          FALSE,          FALSE,  },
    /* NCPA  10*/   {   FALSE,          FALSE,          FALSE,  },
    /* NCPW  10*/   {   FALSE,          FALSE,          FALSE,  },
    /* HCP_  11*/   {   FALSE,          FALSE,          FALSE,  },
    /* WCP_  12*/   {   FALSE,          FALSE,          FALSE,  },
    /* WCP5  13*/   {   FALSE,          FALSE,          FALSE,  },
    /* NCQW  14*/   {   FALSE,          FALSE,          FALSE,  },
    /* ACQW  15*/   {   FALSE,          FALSE,          FALSE,  },
    /* WCQ_  16*/   {   FALSE,          FALSE,          FALSE,  },
    /* ARQW  17*/   {   FALSE,          FALSE,          FALSE,  },
    /* NCSA  18*/   {   FALSE,          FALSE,          FALSE,  },
    /* HCO_  19*/   {   FALSE,          FALSE,          FALSE,  },
    /* WC__  20*/   {   FALSE,          FALSE,          FALSE,  },
    /* WCS_  20*/   {   FALSE,          FALSE,          FALSE,  },
    /* WC5_  21*/   {   FALSE,          FALSE,          FALSE,  },
    /* WC5S  21*/   {   FALSE,          FALSE,          FALSE,  },
    /* NKS_  22*/   {   FALSE,          FALSE,          FALSE,  },
    /* WKSM  23*/   {   FALSE,          FALSE,          FALSE,  },
    /* WIM_  24*/   {   FALSE,          FALSE,          FALSE,  },
    /* NSSW  25*/   {   FALSE,          FALSE,          FALSE,  },
    /* WSS_  26*/   {   FALSE,          FALSE,          FALSE,  },
    /* WHIM  27*/   {   FALSE,          FALSE,          FALSE,  },
    /* WKIM  28*/   {   FALSE,          FALSE,          FALSE,  },
    /* NKSL  29*/   {   FALSE,          FALSE,          FALSE,  },
    /* WKS_  30*/   {   FALSE,          FALSE,          FALSE,  },
    /* WKSC  30*/   {   FALSE,          TRUE,           TRUE,   },
    /* WHS_  31*/   {   FALSE,          FALSE,          FALSE,  },
    /* NQFP  32*/   {   FALSE,          FALSE,          FALSE,  },
    /* NQFA  32*/   {   FALSE,          FALSE,          FALSE,  },
    /* WQE_  33*/   {   FALSE,          FALSE,          FALSE,  },
    /* WQE5  34*/   {   FALSE,          FALSE,          FALSE,  },
    /* NKCC  35*/   {   FALSE,          FALSE,          FALSE,  },
    /* WKC_  36*/   {   FALSE,          FALSE,          FALSE,  },
    /* NOCP  37*/   {   FALSE,          FALSE,          FALSE,  },
    /* NOCA  37*/   {   FALSE,          FALSE,          FALSE,  },
    /* NOCW  37*/   {   FALSE,          FALSE,          FALSE,  },
    /* WOC_  38*/   {   FALSE,          FALSE,          FALSE,  },
    /* WOCS  38*/   {   FALSE,          FALSE,          FALSE,  },
    /* WOC5  39*/   {   FALSE,          FALSE,          FALSE,  },
    /* WOC6  39*/   {   FALSE,          FALSE,          FALSE,  },
    /* AHPW  40*/   {   FALSE,          FALSE,          FALSE,  },
    /* NPEP  41*/   {   FALSE,          FALSE,          FALSE,  },
    /* NPAR  41*/   {   FALSE,          FALSE,          FALSE,  },
    /* HPE_  42*/   {   FALSE,          FALSE,          FALSE,  },
    /* WPE_  43*/   {   FALSE,          FALSE,          FALSE,  },
    /* WPES  43*/   {   FALSE,          FALSE,          FALSE,  },
    /* WPE5  44*/   {   FALSE,          FALSE,          FALSE,  },
    /* NISW  45*/   {   FALSE,          FALSE,          FALSE,  },
    /* AISW  46*/   {   FALSE,          FALSE,          FALSE,  },
    /* NQCS  47*/   {   FALSE,          FALSE,          FALSE,  },
    /* NQCW  47*/   {   FALSE,          FALSE,          TRUE,   },
    /* NQCC  47*/   {   TRUE,           TRUE,           TRUE,   },
    /* NPTA  48*/   {   FALSE,          FALSE,          FALSE,  },
    /* NPNA  48*/   {   FALSE,          FALSE,          FALSE,  },
    /* NPEW  48*/   {   FALSE,          FALSE,          FALSE,  },
    /* NPEH  48*/   {   FALSE,          FALSE,          FALSE,  },
    /* APNW  49*/   {   FALSE,          FALSE,          FALSE,  },
    /* HPEW  50*/   {   FALSE,          FALSE,          FALSE,  },
    /* WPR_  51*/   {   FALSE,          FALSE,          FALSE,  },
    /* NQEP  52*/   {   FALSE,          FALSE,          FALSE,  },
    /* NQEW  52*/   {   FALSE,          FALSE,          FALSE,  },
    /* NQNW  52*/   {   FALSE,          FALSE,          FALSE,  },
    /* AQEW  53*/   {   FALSE,          FALSE,          FALSE,  },
    /* AQNW  53*/   {   FALSE,          FALSE,          FALSE,  },
    /* AQLW  53*/   {   FALSE,          FALSE,          FALSE,  },
    /* WQO_  54*/   {   FALSE,          FALSE,          FALSE,  },
    /* NSBL  55*/   {   FALSE,          FALSE,          FALSE,  },
    /* WSP_  56*/   {   FALSE,          FALSE,          FALSE,  },
    /* WHI_  57*/   {   FALSE,          FALSE,          FALSE,  },
    /* NKA_  58*/   {   FALSE,          FALSE,          FALSE,  },
    /* WKA_  59*/   {   FALSE,          FALSE,          FALSE,  },
    /* ASNW  60*/   {   FALSE,          FALSE,          FALSE,  },
    /* ASEW  60*/   {   FALSE,          FALSE,          FALSE,  },
    /* ASRN  60*/   {   FALSE,          FALSE,          FALSE,  },
    /* ASEN  60*/   {   FALSE,          FALSE,          FALSE,  },
    /* ALA_  61*/   {   FALSE,          FALSE,          FALSE,  },
    /* AGR_  62*/   {   FALSE,          FALSE,          FALSE,  },
    /* ACY_  63*/   {   FALSE,          FALSE,          FALSE,  },
    /* WID_  64*/   {   FALSE,          FALSE,          FALSE,  },
    /* WPUA  65*/   {   FALSE,          FALSE,          FALSE,  },
    /* NHG_  66*/   {   FALSE,          FALSE,          FALSE,  },
    /* WHG_  67*/   {   FALSE,          FALSE,          FALSE,  },
    /* WCI_  68*/   {   FALSE,          FALSE,          FALSE,  },
    /* NOI_  69*/   {   FALSE,          FALSE,          FALSE,  },
    /* WOI_  70*/   {   FALSE,          FALSE,          FALSE,  },
    /* WOIC  70*/   {   FALSE,          TRUE,           TRUE,   },
    /* WOIL  70*/   {   FALSE,          FALSE,          FALSE,  },
    /* WOIS  70*/   {   FALSE,          FALSE,          FALSE,  },
    /* WOIT  70*/   {   FALSE,          FALSE,          FALSE,  },
    /* NSEN  71*/   {   FALSE,          FALSE,          FALSE,  },
    /* NSET  71*/   {   FALSE,          FALSE,          FALSE,  },
    /* NSNW  71*/   {   FALSE,          FALSE,          FALSE,  },
    /* ASAN  72*/   {   FALSE,          FALSE,          FALSE,  },
    /* ASAE  72*/   {   FALSE,          FALSE,          FALSE,  },
    /* NDEA  73*/   {   FALSE,          FALSE,          FALSE,  },
    /* WD__  74*/   {   FALSE,          FALSE,          FALSE,  },
    /* NLLA  75*/   {   FALSE,          FALSE,          FALSE,  },
    /* WLA_  76*/   {   FALSE,          FALSE,          FALSE,  },
    /* NWBL  77*/   {   FALSE,          FALSE,          FALSE,  },
    /* NWZW  77*/   {   FALSE,          FALSE,          TRUE,   },
    /* NPLW  78*/   {   FALSE,          FALSE,          FALSE,  },
    /* NPZW  78*/   {   TRUE,           FALSE,          TRUE,   },
    /* NPF_  78*/   {   TRUE,           FALSE,          TRUE,   },
    /* NPFL  78*/   {   TRUE,           FALSE,          TRUE,   },
    /* NPNW  78*/   {   FALSE,          FALSE,          FALSE,  },
    /* APLW  79*/   {   FALSE,          FALSE,          FALSE,  },
    /* APCO  79*/   {   TRUE,           TRUE,           TRUE,   },
    /* ASYW  80*/   {   FALSE,          FALSE,          FALSE,  },
    /* NHYP  81*/   {   FALSE,          FALSE,          FALSE,  },
    /* NHYW  81*/   {   FALSE,          FALSE,          FALSE,  },
    /* AHYW  82*/   {   FALSE,          FALSE,          FALSE,  },
    /* NAPA  83*/   {   FALSE,          FALSE,          FALSE,  },
    /* NQMP  84*/   {   FALSE,          FALSE,          FALSE,  },
    /* NSLS  85*/   {   FALSE,          FALSE,          FALSE,  },
    /* NSF_  86*/   {   FALSE,          FALSE,          FALSE,  },
    /* NSBS  86*/   {   FALSE,          FALSE,          FALSE,  },
    /* NLA_  87*/   {   FALSE,          FALSE,          FALSE,  },
    /* NLQ_  88*/   {   FALSE,          FALSE,          FALSE,  },
    /* NLQN  88*/   {   FALSE,          FALSE,          FALSE,  },
    /* ALQ_  89*/   {   FALSE,          FALSE,          FALSE,  },
    /* NGR_  90*/   {   FALSE,          FALSE,          FALSE,  },
    /* NGRN  90*/   {   FALSE,          FALSE,          FALSE,  },
    /* NGQ_  91*/   {   FALSE,          FALSE,          FALSE,  },
    /* NGQN  91*/   {   FALSE,          FALSE,          FALSE,  },
    /* NCY_  92*/   {   FALSE,          FALSE,          FALSE,  },
    /* NCYP  93*/   {   FALSE,          FALSE,          FALSE,  },
    /* NCYC  93*/   {   FALSE,          TRUE,           TRUE,   },
    /* NAR_  94*/   {   FALSE,          FALSE,          FALSE,  },
    /* NAQN  95*/   {   FALSE,          FALSE,          FALSE,  },
    /* NHB_  96*/   {   TRUE,           FALSE,          FALSE,  },
    /* NHBC  96*/   {   TRUE,           TRUE,           TRUE,   },
    /* NHBW  96*/   {   TRUE,           FALSE,          FALSE,  },
    /* NHBR  96*/   {   TRUE,           FALSE,          FALSE,  },
    /* NASR  97*/   {   TRUE,           FALSE,          FALSE,  },
    /* NAAR  97*/   {   TRUE,           FALSE,          FALSE,  },
    /* NAAC  97*/   {   TRUE,           TRUE,           TRUE,   },
    /* NAAD  97*/   {   TRUE,           FALSE,          FALSE,  },
    /* NAED  97*/   {   TRUE,           FALSE,          FALSE,  },
    /* NANW  97*/   {   TRUE,           FALSE,          FALSE,  },
    /* NAEW  97*/   {   TRUE,           FALSE,          FALSE,  },
    /* NAAS  97*/   {   TRUE,           FALSE,          FALSE,  },
    /* NHI_  98*/   {   TRUE,           FALSE,          FALSE,  },
    /* NHIN  98*/   {   TRUE,           FALSE,          FALSE,  },
    /* NHIC  98*/   {   TRUE,           TRUE,           TRUE,   },
    /* NHID  98*/   {   TRUE,           FALSE,          FALSE,  },
    /* NBE_  99*/   {   TRUE,           FALSE,          FALSE,  },
    /* NBEC  99*/   {   TRUE,           TRUE,           TRUE,   },
    /* NBED  99*/   {   TRUE,           FALSE,          FALSE,  },
    /* NGM_ 100*/   {   TRUE,           FALSE,          FALSE,  },
    /* NGMC 100*/   {   TRUE,           TRUE,           TRUE,   },
    /* NGMD 100*/   {   TRUE,           FALSE,          FALSE,  },
    /* NGJ_ 101*/   {   TRUE,           FALSE,          FALSE,  },
    /* NGJC 101*/   {   TRUE,           TRUE,           TRUE,   },
    /* NGJD 101*/   {   TRUE,           FALSE,          FALSE,  },
    /* NOR_ 102*/   {   TRUE,           FALSE,          FALSE,  },
    /* NORC 102*/   {   TRUE,           TRUE,           TRUE,   },
    /* NORD 102*/   {   TRUE,           FALSE,          FALSE,  },
    /* NTA_ 103*/   {   TRUE,           FALSE,          FALSE,  },
    /* NTAC 103*/   {   TRUE,           TRUE,           TRUE,   },
    /* NTAD 103*/   {   TRUE,           FALSE,          FALSE,  },
    /* NTE_ 104*/   {   TRUE,           FALSE,          FALSE,  },
    /* NTEC 104*/   {   TRUE,           TRUE,           TRUE,   },
    /* NTED 104*/   {   TRUE,           FALSE,          FALSE,  },
    /* NKD_ 105*/   {   TRUE,           FALSE,          FALSE,  },
    /* NKDC 105*/   {   TRUE,           TRUE,           TRUE,   },
    /* NKDD 105*/   {   TRUE,           FALSE,          FALSE,  },
    /* NMA_ 106*/   {   TRUE,           FALSE,          FALSE,  },
    /* NMAC 106*/   {   TRUE,           TRUE,           TRUE,   },
    /* NMAD 106*/   {   TRUE,           FALSE,          FALSE,  },
    /* NTH_ 107*/   {   TRUE,           FALSE,          FALSE,  },
    /* NTHC 107*/   {   TRUE,           TRUE,           TRUE,   },
    /* NTHD 107*/   {   TRUE,           FALSE,          FALSE,  },
    /* NTHT 107*/   {   TRUE,           FALSE,          FALSE,  },
    /* NLO_ 108*/   {   TRUE,           FALSE,          FALSE,  },
    /* NLOC 108*/   {   TRUE,           TRUE,           TRUE,   },
    /* NLOD 108*/   {   TRUE,           FALSE,          FALSE,  },
    /* NTI_ 109*/   {   TRUE,           FALSE,          FALSE,  },
    /* NTIC 109*/   {   TRUE,           TRUE,           TRUE,   },
    /* NTID 109*/   {   TRUE,           FALSE,          FALSE,  },
    /* NGE_ 110*/   {   FALSE,          FALSE,          FALSE,  },
    /* NGEQ 111*/   {   FALSE,          FALSE,          FALSE,  },
    /* NBO_ 112*/   {   FALSE,          FALSE,          FALSE,  },
    /* NBSP 113*/   {   FALSE,          FALSE,          FALSE,  },
    /* NOF_ 114*/   {   FALSE,          FALSE,          FALSE,  },
    /* NOBS 114*/   {   FALSE,          FALSE,          FALSE,  },
    /* NOEA 114*/   {   FALSE,          FALSE,          FALSE,  },
    /* NONA 114*/   {   FALSE,          FALSE,          FALSE,  },
    /* NONP 114*/   {   FALSE,          FALSE,          FALSE,  },
    /* NOEP 114*/   {   FALSE,          FALSE,          FALSE,  },
    /* NONW 114*/   {   FALSE,          FALSE,          FALSE,  },
    /* NOEW 114*/   {   FALSE,          FALSE,          FALSE,  },
    /* NOLW 114*/   {   FALSE,          FALSE,          FALSE,  },
    /* NOCO 114*/   {   FALSE,          FALSE,          FALSE,  },
    /* NOSP 114*/   {   TRUE,           FALSE,          TRUE,   },
    /* NOEN 114*/   {   FALSE,          FALSE,          FALSE,  },
    /* NET_ 115*/   {   FALSE,          FALSE,          FALSE,  }, // Check on Ethiopic
    /* NCA_ 116*/   {   FALSE,          FALSE,          FALSE,  }, // Check on Canadian Syllabics
    /* NCH_ 117*/   {   FALSE,          FALSE,          FALSE,  }, // Check on Cherokee
    /* WYI_ 118*/   {   FALSE,          FALSE,          FALSE,  }, // Check on Yi
    /* NBR_ 119*/   {   FALSE,          FALSE,          FALSE,  },
    /* NRU_ 120*/   {   FALSE,          FALSE,          FALSE,  },
    /* NOG_ 121*/   {   FALSE,          FALSE,          FALSE,  }, // Check on Ogham
    /* NSI_ 122*/   {   TRUE,           FALSE,          FALSE,  }, // Check on Sinhala
    /* NSIC 122*/   {   TRUE,           TRUE,           TRUE,   }, // Check on Sinhala
    /* NTN_ 123*/   {   TRUE,           FALSE,          FALSE,  }, // Check on Thaana
    /* NTNC 123*/   {   TRUE,           TRUE,           TRUE,   }, // Check on Thaana
    /* NKH_ 124*/   {   TRUE,           FALSE,          FALSE,  }, // Check on Khmer
    /* NKHC 124*/   {   TRUE,           TRUE,           TRUE,   }, // Check on Khmer
    /* NKHD 124*/   {   TRUE,           FALSE,          FALSE,  }, // Check on Khmer
    /* NBU_ 125*/   {   TRUE,           FALSE,          FALSE,  }, // Check on Burmese/Myanmar
    /* NBUC 125*/   {   TRUE,           TRUE,           TRUE,   }, // Check on Burmese/Myanmar
    /* NBUD 125*/   {   TRUE,           FALSE,          FALSE,  }, // Check on Burmese/Myanmar
    /* NSY_ 126*/   {   TRUE,           FALSE,          FALSE,  }, // Check on Syriac
    /* NSYC 126*/   {   TRUE,           TRUE,           TRUE,   }, // Check on Syriac
    /* NSYW 126*/   {   TRUE,           FALSE,          FALSE,  }, // Check on Syriac
    /* NMO_ 127*/   {   TRUE,           FALSE,          FALSE,  }, // Check on Mongolian
    /* NMOC 127*/   {   TRUE,           TRUE,           TRUE,   }, // Check on Mongolian
    /* NMOD 127*/   {   TRUE,           FALSE,          FALSE,  }, // Check on Mongolian
    /* NHS_ 128*/   {   TRUE,           FALSE,          FALSE,  }, // High surrogate (1)
    /* WHT_ 129*/   {   TRUE,           FALSE,          FALSE,  }, // High surrogate (2)
    /* LS__ 130*/   {   FALSE,          FALSE,          FALSE,  }, // Low surrogate
    /* XNW_ 131*/   {   FALSE,          FALSE,          FALSE,  },
};

