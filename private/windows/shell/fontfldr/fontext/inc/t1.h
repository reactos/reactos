// --------------------------------------------------------------------
//    T1.h
//
//    Type 1 utilities.
//
// --------------------------------------------------------------------

#if !defined(__T1_H__)
#define __T1_H__

// define this in priv.h for T1->TT support. (NB: It's not finished, yet.)
//
#if defined(T1_SUPPORT)


// --------------------------------------------------------------------

typedef struct {
    // The following are filled in for a Type 1 font.
    //
    FullPathName_t  pfm;
    FullPathName_t  pfb;
    FullPathName_t  ttf;
    BOOL            bCreatedPFM;
} T1_INFO, FAR * LPT1_INFO;


BOOL NEAR PASCAL bIsType1 (
    LPTSTR        lpFile, 
    FontDesc_t *  lpDesc,                 
    LPT1_INFO     lpInfo = NULL );


BOOL NEAR PASCAL bConvertT1( LPT1_INFO lpInfo );


#endif

// --------------------------------------------------------------------


#endif    // __T1_H__ 
