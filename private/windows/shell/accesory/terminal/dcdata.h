

procPtr pProcTable[] = 
{
   /* 0 */    pNullState,          /* 00h */
   /* 1 */    pCursorState,        /* 01h */
   /* 2 */    pCursHome,           /* 02h */
   /* 3 */    pClrScr,             /* 03h */
   /* 4 */    pClrEol,             /* 04h */
   /* 5 */    pClrEop,             /* 05h */
   /* 6 */    pLinIns,             /* 06h */
   /* 7 */    pLinDel,             /* 07h */
   /* 8 */    pCursToggle,         /* 08h */
   /* 9 */    pCursUp,             /* 09h */
   /* 10 */   pCursDn,             /* 0ah */
   /* 11 */   pCursRt,             /* 0bh */
   /* 12 */   pCursLt,             /* 0ch */
   /* 13 */   pCursOff,            /* 0dh */
   /* 14 */   pCursOn,             /* 0eh */
   /* 15 */   pNullState,          /* 0fh */
   /* 16 */   pReverseOff,         /* 10h */
   /* 17 */   pReverseOn,          /* 11h */
   /* 18 */   pCmpSrvResponse,     /* 12h */
   /* 19 */   pSndCursor,          /* 13h */
   /* 20 */   pInquire,            /* 14h */
   /* 21 */   pNextLine,           /* 15h */
   /* 22 */   pRevIndex,           /* 16h */
   /* 23 */   pDecScs,             /* 17h */
   /* 24 */   pSetMode,            /* 18h */
   /* 25 */   pSetTab,             /* 19h */
   /* 26 */   pClearTab,           /* 1ah */
   /* 27 */   pTab,                /* 1bh */
   /* 28 */   aSetScrRgn,          /* 1ch */
   /* 29 */   pCharDel,            /* 1dh */
   /* 30 */   pInsChar,            /* 1eh */
   /* 31 */   pClearAllTabs,       /* 1fh */
   /* 32 */   pEscSkip,            /* 20h */
   /* 33 */   pVPosState,          /* 21h */
   /* 34 */   pHPosState,          /* 22h */
   /* 35 */   pLAttrState,         /* 23h */
   /* 36 */   pClrBop,             /* 24h */
   /* 37 */   pClrBol,             /* 25h */
   /* 38 */   pClrLine,            /* 26h */
   /* 39 */   pNullState,          /* 27h */
   /* 40 */   pSaveCursorPos,      /* 28h */
   /* 41 */   pRestoreCursorPos,   /* 29h */
   /* 42 */   pAnsiState,          /* 2ah */
   /* 43 */   pGrState,            /* 2bh */
   /* 44 */   pVT100H,             /* 2ch */
   /* 45 */   pVT100D,             /* 2dh */
   /* 46 */   pVT100M,             /* 2eh */
   /* 47 */   pVT100c,             /* 2fh */
   /* 48 */   pDCSTerminate,       /* 30h */
   /* 49 */   pProtOn,             /* 31h */
   /* 50 */   pProtOff,            /* 32h */
   /* 51 */   pClrAll,             /* 33h */
   /* 52 */   pPrintOn,            /* 34h */
   /* 53 */   pPrintOff,           /* 35h */
   /* 54 */   pVT100P,             /* 36h */
   /* 55 */   pVideoAttribState,   /* 37h */
   /* 56 */   pCursorOnOffState,   /* 38h */
   /* 57 */   pAnswerBack,         /* 39h */
   /* 58 */   pEchoOff,            /* 3Ah */
   /* 59 */   pEchoOn,             /* 3Bh */
   /* 60 */   pCR,                 /* 3Ch */
   /* 61 */   pLF,                 /* 3Dh */
   /* 62 */   pBackSpace,          /* 3Eh */
   /* 63 */   pBeep,               /* 3Fh */
   /* 64 */   pBegProtect,         /* 40h */          /* mbbx 1.03: TV925... */
   /* 65 */   pEndProtect,         /* 41h */
   /* 66 */   pBegGraphics,        /* 42h */
   /* 67 */   pEndGraphics,        /* 43h */
   /* 68 */   pSetStatusLine,      /* 44h */
   /* 69 */   pNullState,          /* 45h */          /* mbbx 1.10: VT220 8BIT */
   /* 70 */   pSetCtrlBits,        /* 46h */          /* mbbx 1.10: VT220 8BIT */
   /* 71 */   pTransPrint,         /* 47h */          /* mbbx 2.01.32 */
};


procPtr aProcTable[] = 
{
   /* 0 */    pNullState,     /* 00h */
   /* 1 */    aCursor,        /* 01h */
   /* 2 */    pNullState,     /* 02h */
   /* 3 */    pNullState,     /* 03h */
   /* 4 */    aClrEol,        /* 04h */
   /* 5 */    aClrEop,        /* 05h */
   /* 6 */    aInsLin,        /* 06h */
   /* 7 */    aDelLin,        /* 07h */
   /* 8 */    pNullState,     /* 08h */
   /* 9 */    aCursUp,        /* 09h */
   /* 10 */   aCursDn,        /* 0ah */
   /* 11 */   aCursRt,        /* 0bh */
   /* 12 */   aCursLt,        /* 0ch */
   /* 13 */   pNullState,     /* 0dh */
   /* 14 */   pNullState,     /* 0eh */
   /* 15 */   aVideo,         /* 0fh */
   /* 16 */   pNullState,     /* 10h */
   /* 17 */   pNullState,     /* 11h */
   /* 18 */   pNullState,     /* 12h */
   /* 19 */   aReport,        /* 13h */
   /* 20 */   pInquire,       /* 14h */
   /* 21 */   pNullState,     /* 15h */
   /* 22 */   pNullState,     /* 16h */
   /* 23 */   pNullState,     /* 17h */
   /* 24 */   aSetMode,       /* 18h */
   /* 25 */   pNullState,     /* 19h */
   /* 26 */   aClearTabs,     /* 1ah */
   /* 27 */   pNullState,     /* 1bh */
   /* 28 */   aSetScrRgn,     /* 1ch */
   /* 29 */   aDelChar,       /* 1dh */
   /* 30 */   pNullState,     /* 1eh */
   /* 31 */   pNullState,     /* 1fh */
   /* 32 */   pEscSkip,       /* 20h */
   /* 33 */   pNullState,     /* 21h */
   /* 34 */   pNullState,     /* 22h */
   /* 35 */   pNullState,     /* 23h */
   /* 36 */   pNullState,     /* 24h */
   /* 37 */   pNullState,     /* 25h */
   /* 38 */   pNullState,     /* 26h */
   /* 39 */   pNullState,     /* 27h */
   /* 40 */   pNullState,     /* 28h */
   /* 41 */   pNullState,     /* 29h */
   /* 42 */   pNullState,     /* 2ah */
   /* 43 */   pNullState,     /* 2bh */
   /* 44 */   pVT100H,        /* 2ch */
   /* 45 */   pVT100D,        /* 2dh */
   /* 46 */   pVT100M,        /* 2eh */
   /* 47 */   pVT100c,        /* 2fh */
   /* 48 */   pDCSTerminate,       /* 30h */     /* mbbx: are these ANSI ??? */
   /* 49 */   pProtOn,             /* 31h */
   /* 50 */   pProtOff,            /* 32h */
   /* 51 */   pClrAll,             /* 33h */
   /* 52 */   pPrintOn,            /* 34h */
   /* 53 */   pPrintOff,           /* 35h */
   /* 54 */   pVT100P,             /* 36h */
   /* 55 */   pVideoAttribState,   /* 37h */
   /* 56 */   pCursorOnOffState,   /* 38h */
   /* 57 */   pNullState,          /* 39h */
   /* 58 */   pNullState,          /* 3Ah */
   /* 59 */   pNullState,          /* 3Bh */
   /* 60 */   pNullState,          /* 3Ch */
   /* 61 */   pNullState,          /* 3Dh */
   /* 62 */   pNullState,          /* 3Eh */
   /* 63 */   pNullState,          /* 3Fh */
   /* 64 */   pNullState,          /* 40h */          /* mbbx 1.03: TV925... */
   /* 65 */   pNullState,          /* 41h */
   /* 66 */   pNullState,          /* 42h */
   /* 67 */   pNullState,          /* 43h */
   /* 68 */   pNullState,          /* 44h */
   /* 69 */   aSetCompLevel,       /* 45h */          /* mbbx 1.10: VT220 8BIT */
   /* 70 */   pNullState,          /* 46h */          /* mbbx 1.10: VT220 8BIT */
   /* 71 */   pTransPrint,         /* 47h */          /* mbbx 2.01.32 */
};


procPtr ansiParseTable[] =
{
      ansiArgument,          /*  '0'  */
      ansiArgument,          /*  '1'  */
      ansiArgument,          /*  '2'  */
      ansiArgument,          /*  '3'  */
      ansiArgument,          /*  '4'  */
      ansiArgument,          /*  '5'  */
      ansiArgument,          /*  '6'  */
      ansiArgument,          /*  '7'  */
      ansiArgument,          /*  '8'  */
      ansiArgument,          /*  '9'  */
      pNullState,            /*  ':'  */
      ansiDelimiter,         /*  ';'  */
      pNullState,            /*  '<'  */
      pNullState,            /*  '='  */
      ansiHeathPrivate,      /*  '>'  */
      ansiDecPrivate,        /*  '?'  */
};


BYTE ansiXlateTable[256] =                   /* mbbx 1.06A: ics new xlate... */
{
   0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,    /* ANSI -> ASCII */
   0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
   0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
   0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
   0xA0, 0xAD, 0x9B, 0x9C, 0xA4, 0x9D, 0xA6, 0xA7,
   0xA8, 0xA9, 0xA6, 0xAE, 0xAC, 0xAD, 0xAE, 0xAF,
   0xF8, 0xF1, 0xFD, 0xB3, 0xB4, 0xE6, 0xB6, 0xFA,
   0xB8, 0xB9, 0xA7, 0xAF, 0xAC, 0xAB, 0xBE, 0xA8,
   0xC0, 0xC1, 0xC2, 0xC3, 0x8E, 0x8F, 0x92, 0x80,
   0xC8, 0x90, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
   0xD0, 0xA5, 0xD2, 0xD3, 0xD4, 0xD5, 0x99, 0xD7,
   0xED, 0xD9, 0xDA, 0xDB, 0x9A, 0xDD, 0xDE, 0xE1,
   0x85, 0xA0, 0x83, 0xE3, 0x84, 0x86, 0x91, 0x87,
   0x8A, 0x82, 0x88, 0x89, 0x8D, 0xA1, 0x8C, 0x8B,
   0xEB, 0xA4, 0x95, 0xA2, 0x93, 0xF5, 0x94, 0xF7,
   0xF8, 0x97, 0xA3, 0x96, 0x81, 0xFD, 0xFE, 0x98,

   0xC7, 0xFC, 0xE9, 0xE2, 0xE4, 0xE0, 0xE5, 0xE7,    /* ASCII -> ANSI */
   0xEA, 0xEB, 0xE8, 0xEF, 0xEE, 0xEC, 0xC4, 0xC5,
   0xC9, 0xE6, 0xC6, 0xF4, 0xF6, 0xF2, 0xFB, 0xF9,
   0xFF, 0xD6, 0xDC, 0xA2, 0xA3, 0xA5, 0x7F, 0x7F,
   0xE1, 0xED, 0xF3, 0xFA, 0xF1, 0xD1, 0xAA, 0xBA,
   0xBF, 0xA9, 0xAA, 0xBD, 0xBC, 0xA1, 0xAB, 0xBB,
   0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7,
   0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
   0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
   0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
   0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7,
   0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
   0xE0, 0xDF, 0xE2, 0xE3, 0xE4, 0xE5, 0xB5, 0xE7,
   0xE8, 0xE9, 0xEA, 0xF0, 0xEC, 0xD8, 0xEE, 0xEF,
   0xF0, 0xB1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
   0xB0, 0xF9, 0xB7, 0xFB, 0xFC, 0xB2, 0xFE, 0xFF,
};


