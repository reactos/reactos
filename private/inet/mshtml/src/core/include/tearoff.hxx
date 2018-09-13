//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       tearoff.hxx
//
//  Contents:   Tearoff Interface Utilities
//
//----------------------------------------------------------------------------

#ifndef I_TEAROFF_HXX_
#define I_TEAROFF_HXX_
#pragma INCMSG("--- Beg 'tearoff.hxx'")

#define cat(a,b)        a##b

struct TEAROFF_THUNK
{
    void *      papfnVtblThis;     // Thunk's vtable
    ULONG       ulRef;             // Reference count for this thunk.
    IID const * const * apIID;     // Short circuit QI using these IIDs.
    void *      pvObject1;         // Delegate other methods to this object using...
    const void * apfnVtblObject1;  // ...this array of pointers to member functions.
    void *      pvObject2;         // Delegate methods to this object using...
    void *      apfnVtblObject2;   // ...this array of pointers to member functions...
    DWORD       dwMask;            // ...the index of the method is set in the mask.
    DWORD       n;                 // index of method into vtbl
    void *      apVtblPropDesc;    // array of propdescs in Vtbl order
};

#ifdef _MAC
asm TEAROFF_THUNK* GetThunk();
#endif

#if defined(_M_ALPHA) || defined(_M_IA64)
    extern "C" void * _stdcall _GetTearoff();
#elif defined (UNIX)
    extern "C" void * _GetTearoff();
#endif

// ALPHAHACK
// macro for accessing tearoff thunks from interface methods
#if defined(_M_IX86) && !defined(WIN16)
#define GET_TEAROFF_THUNK       \
    TEAROFF_THUNK *pthunk;      \
{                               \
    __asm mov pthunk, eax       \
}
#elif defined(_M_IX86) && defined(WIN16)
#define GET_TEAROFF_THUNK           \
    /* To be filled later       */  \
    TEAROFF_THUNK *pthunk = NULL;
#elif defined(_M_ALPHA) || defined(_M_AXP64) || defined(_M_IA64) || defined(UNIX)
#define GET_TEAROFF_THUNK           \
    TEAROFF_THUNK *pthunk = (TEAROFF_THUNK *)_GetTearoff();
#elif defined(_MAC)
#define GET_TEAROFF_THUNK           \
    TEAROFF_THUNK *pthunk = GetThunk();
#endif

// macro for accessing propdesc from tearoff thunks in interface methods
#define FIRST_INTERFACE_METHOD    7

#define GET_THUNK_PROPDESC          \
    GET_TEAROFF_THUNK               \
    const PROPERTYDESC *pPropDesc;  \
    Assert(pthunk);                 \
    Assert(pthunk->apVtblPropDesc); \
    pPropDesc = ((const PROPERTYDESC * const *)pthunk->apVtblPropDesc)[pthunk->n - FIRST_INTERFACE_METHOD]; \
    Assert(pPropDesc);              \


#define THUNK_ARRAY_3_TO_15(x) \
 cat(THUNK_,x)(3)   cat(THUNK_,x)(4)   cat(THUNK_,x)(5)   cat(THUNK_,x)(6)   cat(THUNK_,x)(7)   cat(THUNK_,x)(8)   cat(THUNK_,x)(9)   cat(THUNK_,x)(10)  cat(THUNK_,x)(11)  cat(THUNK_,x)(12)  cat(THUNK_,x)(13)  \
 cat(THUNK_,x)(14)  cat(THUNK_,x)(15)
    
#ifdef _MAC

#define THUNK_ARRAY_16_TO_101(x) \
cat(THUNK_,x)(16)  cat(THUNK_,x)(17)  cat(THUNK_,x)(18)  cat(THUNK_,x)(19)  cat(THUNK_,x)(20)  cat(THUNK_,x)(21)  cat(THUNK_,x)(22)  cat(THUNK_,x)(23)  cat(THUNK_,x)(24)  \
cat(THUNK_,x)(25)  cat(THUNK_,x)(26)  cat(THUNK_,x)(27)  cat(THUNK_,x)(28)  cat(THUNK_,x)(29)  cat(THUNK_,x)(30)  cat(THUNK_,x)(31)  cat(THUNK_,x)(32)  cat(THUNK_,x)(33)  cat(THUNK_,x)(34)  cat(THUNK_,x)(35)  \
cat(THUNK_,x)(36)  cat(THUNK_,x)(37)  cat(THUNK_,x)(38)  cat(THUNK_,x)(39)  cat(THUNK_,x)(40)  cat(THUNK_,x)(41)  cat(THUNK_,x)(42)  cat(THUNK_,x)(43)  cat(THUNK_,x)(44)  cat(THUNK_,x)(45)  cat(THUNK_,x)(46)  \
cat(THUNK_,x)(47)  cat(THUNK_,x)(48)  cat(THUNK_,x)(49)  cat(THUNK_,x)(50)  cat(THUNK_,x)(51)  cat(THUNK_,x)(52)  cat(THUNK_,x)(53)  cat(THUNK_,x)(54)  cat(THUNK_,x)(55)  cat(THUNK_,x)(56)  cat(THUNK_,x)(57)  \
cat(THUNK_,x)(58)  cat(THUNK_,x)(59)  cat(THUNK_,x)(60)  cat(THUNK_,x)(61)  cat(THUNK_,x)(62)  cat(THUNK_,x)(63)  cat(THUNK_,x)(64)  cat(THUNK_,x)(65)  cat(THUNK_,x)(66)  cat(THUNK_,x)(67)  cat(THUNK_,x)(68)  \
cat(THUNK_,x)(69)  cat(THUNK_,x)(70)  cat(THUNK_,x)(71)  cat(THUNK_,x)(72)  cat(THUNK_,x)(73)  cat(THUNK_,x)(74)  cat(THUNK_,x)(75)  cat(THUNK_,x)(76)  cat(THUNK_,x)(77)  cat(THUNK_,x)(78)  cat(THUNK_,x)(79)  
#define THUNK_ARRAY_102_TO_145(x) \
cat(THUNK_,x)(80)  cat(THUNK_,x)(81)  cat(THUNK_,x)(82)  cat(THUNK_,x)(83)  cat(THUNK_,x)(84)  cat(THUNK_,x)(85)  cat(THUNK_,x)(86)  cat(THUNK_,x)(87)  cat(THUNK_,x)(88)  cat(THUNK_,x)(89)  cat(THUNK_,x)(90)  \
cat(THUNK_,x)(91)  cat(THUNK_,x)(92)  cat(THUNK_,x)(93)  cat(THUNK_,x)(94)  cat(THUNK_,x)(95)  cat(THUNK_,x)(96)  cat(THUNK_,x)(97)  cat(THUNK_,x)(98)  cat(THUNK_,x)(99)  cat(THUNK_,x)(100) cat(THUNK_,x)(101) \
cat(THUNK_,x)(102) cat(THUNK_,x)(103) cat(THUNK_,x)(104) cat(THUNK_,x)(105) cat(THUNK_,x)(106) cat(THUNK_,x)(107) cat(THUNK_,x)(108) cat(THUNK_,x)(109) cat(THUNK_,x)(110) cat(THUNK_,x)(111) cat(THUNK_,x)(112) \
cat(THUNK_,x)(113) cat(THUNK_,x)(114) cat(THUNK_,x)(115) cat(THUNK_,x)(116) cat(THUNK_,x)(117) cat(THUNK_,x)(118) cat(THUNK_,x)(119) cat(THUNK_,x)(120) cat(THUNK_,x)(121) cat(THUNK_,x)(122) cat(THUNK_,x)(123) \
cat(THUNK_,x)(124) cat(THUNK_,x)(125) cat(THUNK_,x)(126) cat(THUNK_,x)(127) cat(THUNK_,x)(128) cat(THUNK_,x)(129) cat(THUNK_,x)(130) cat(THUNK_,x)(131) cat(THUNK_,x)(132) cat(THUNK_,x)(133) cat(THUNK_,x)(134) \
cat(THUNK_,x)(135) cat(THUNK_,x)(136) cat(THUNK_,x)(137) cat(THUNK_,x)(138) cat(THUNK_,x)(139) cat(THUNK_,x)(140) cat(THUNK_,x)(141) cat(THUNK_,x)(142) cat(THUNK_,x)(143) cat(THUNK_,x)(144) cat(THUNK_,x)(145) 
#define THUNK_ARRAY_146_AND_UP(x) \
cat(THUNK_,x)(146) cat(THUNK_,x)(147) cat(THUNK_,x)(148) cat(THUNK_,x)(149) cat(THUNK_,x)(150) cat(THUNK_,x)(151) cat(THUNK_,x)(152) cat(THUNK_,x)(153) cat(THUNK_,x)(154) cat(THUNK_,x)(155) cat(THUNK_,x)(156) \
cat(THUNK_,x)(157) cat(THUNK_,x)(158) cat(THUNK_,x)(159) cat(THUNK_,x)(160) cat(THUNK_,x)(161) cat(THUNK_,x)(162) cat(THUNK_,x)(163) cat(THUNK_,x)(164) cat(THUNK_,x)(165) cat(THUNK_,x)(166) cat(THUNK_,x)(167) \
cat(THUNK_,x)(168) cat(THUNK_,x)(169) cat(THUNK_,x)(170) cat(THUNK_,x)(171) cat(THUNK_,x)(172) cat(THUNK_,x)(173) cat(THUNK_,x)(174) cat(THUNK_,x)(175) cat(THUNK_,x)(176) cat(THUNK_,x)(177) cat(THUNK_,x)(178) \
cat(THUNK_,x)(179) cat(THUNK_,x)(180) cat(THUNK_,x)(181) cat(THUNK_,x)(182) cat(THUNK_,x)(183) cat(THUNK_,x)(184) cat(THUNK_,x)(185) cat(THUNK_,x)(186) cat(THUNK_,x)(187) cat(THUNK_,x)(188) cat(THUNK_,x)(189) \
cat(THUNK_,x)(190) cat(THUNK_,x)(191) cat(THUNK_,x)(192) cat(THUNK_,x)(193) cat(THUNK_,x)(194) cat(THUNK_,x)(195) cat(THUNK_,x)(196) cat(THUNK_,x)(197) cat(THUNK_,x)(198) cat(THUNK_,x)(199)        

#else

#define THUNK_ARRAY_16_AND_UP(x) \
cat(THUNK_,x)(16)  cat(THUNK_,x)(17)  cat(THUNK_,x)(18)  cat(THUNK_,x)(19)  cat(THUNK_,x)(20)  cat(THUNK_,x)(21)  cat(THUNK_,x)(22)  cat(THUNK_,x)(23)  cat(THUNK_,x)(24)  \
cat(THUNK_,x)(25)  cat(THUNK_,x)(26)  cat(THUNK_,x)(27)  cat(THUNK_,x)(28)  cat(THUNK_,x)(29)  cat(THUNK_,x)(30)  cat(THUNK_,x)(31)  cat(THUNK_,x)(32)  cat(THUNK_,x)(33)  cat(THUNK_,x)(34)  cat(THUNK_,x)(35)  \
cat(THUNK_,x)(36)  cat(THUNK_,x)(37)  cat(THUNK_,x)(38)  cat(THUNK_,x)(39)  cat(THUNK_,x)(40)  cat(THUNK_,x)(41)  cat(THUNK_,x)(42)  cat(THUNK_,x)(43)  cat(THUNK_,x)(44)  cat(THUNK_,x)(45)  cat(THUNK_,x)(46)  \
cat(THUNK_,x)(47)  cat(THUNK_,x)(48)  cat(THUNK_,x)(49)  cat(THUNK_,x)(50)  cat(THUNK_,x)(51)  cat(THUNK_,x)(52)  cat(THUNK_,x)(53)  cat(THUNK_,x)(54)  cat(THUNK_,x)(55)  cat(THUNK_,x)(56)  cat(THUNK_,x)(57)  \
cat(THUNK_,x)(58)  cat(THUNK_,x)(59)  cat(THUNK_,x)(60)  cat(THUNK_,x)(61)  cat(THUNK_,x)(62)  cat(THUNK_,x)(63)  cat(THUNK_,x)(64)  cat(THUNK_,x)(65)  cat(THUNK_,x)(66)  cat(THUNK_,x)(67)  cat(THUNK_,x)(68)  \
cat(THUNK_,x)(69)  cat(THUNK_,x)(70)  cat(THUNK_,x)(71)  cat(THUNK_,x)(72)  cat(THUNK_,x)(73)  cat(THUNK_,x)(74)  cat(THUNK_,x)(75)  cat(THUNK_,x)(76)  cat(THUNK_,x)(77)  cat(THUNK_,x)(78)  cat(THUNK_,x)(79)  \
cat(THUNK_,x)(80)  cat(THUNK_,x)(81)  cat(THUNK_,x)(82)  cat(THUNK_,x)(83)  cat(THUNK_,x)(84)  cat(THUNK_,x)(85)  cat(THUNK_,x)(86)  cat(THUNK_,x)(87)  cat(THUNK_,x)(88)  cat(THUNK_,x)(89)  cat(THUNK_,x)(90)  \
cat(THUNK_,x)(91)  cat(THUNK_,x)(92)  cat(THUNK_,x)(93)  cat(THUNK_,x)(94)  cat(THUNK_,x)(95)  cat(THUNK_,x)(96)  cat(THUNK_,x)(97)  cat(THUNK_,x)(98)  cat(THUNK_,x)(99)  cat(THUNK_,x)(100) cat(THUNK_,x)(101) \
cat(THUNK_,x)(102) cat(THUNK_,x)(103) cat(THUNK_,x)(104) cat(THUNK_,x)(105) cat(THUNK_,x)(106) cat(THUNK_,x)(107) cat(THUNK_,x)(108) cat(THUNK_,x)(109) cat(THUNK_,x)(110) cat(THUNK_,x)(111) cat(THUNK_,x)(112) \
cat(THUNK_,x)(113) cat(THUNK_,x)(114) cat(THUNK_,x)(115) cat(THUNK_,x)(116) cat(THUNK_,x)(117) cat(THUNK_,x)(118) cat(THUNK_,x)(119) cat(THUNK_,x)(120) cat(THUNK_,x)(121) cat(THUNK_,x)(122) cat(THUNK_,x)(123) \
cat(THUNK_,x)(124) cat(THUNK_,x)(125) cat(THUNK_,x)(126) cat(THUNK_,x)(127) cat(THUNK_,x)(128) cat(THUNK_,x)(129) cat(THUNK_,x)(130) cat(THUNK_,x)(131) cat(THUNK_,x)(132) cat(THUNK_,x)(133) cat(THUNK_,x)(134) \
cat(THUNK_,x)(135) cat(THUNK_,x)(136) cat(THUNK_,x)(137) cat(THUNK_,x)(138) cat(THUNK_,x)(139) cat(THUNK_,x)(140) cat(THUNK_,x)(141) cat(THUNK_,x)(142) cat(THUNK_,x)(143) cat(THUNK_,x)(144) cat(THUNK_,x)(145) \
cat(THUNK_,x)(146) cat(THUNK_,x)(147) cat(THUNK_,x)(148) cat(THUNK_,x)(149) cat(THUNK_,x)(150) cat(THUNK_,x)(151) cat(THUNK_,x)(152) cat(THUNK_,x)(153) cat(THUNK_,x)(154) cat(THUNK_,x)(155) cat(THUNK_,x)(156) \
cat(THUNK_,x)(157) cat(THUNK_,x)(158) cat(THUNK_,x)(159) cat(THUNK_,x)(160) cat(THUNK_,x)(161) cat(THUNK_,x)(162) cat(THUNK_,x)(163) cat(THUNK_,x)(164) cat(THUNK_,x)(165) cat(THUNK_,x)(166) cat(THUNK_,x)(167) \
cat(THUNK_,x)(168) cat(THUNK_,x)(169) cat(THUNK_,x)(170) cat(THUNK_,x)(171) cat(THUNK_,x)(172) cat(THUNK_,x)(173) cat(THUNK_,x)(174) cat(THUNK_,x)(175) cat(THUNK_,x)(176) cat(THUNK_,x)(177) cat(THUNK_,x)(178) \
cat(THUNK_,x)(179) cat(THUNK_,x)(180) cat(THUNK_,x)(181) cat(THUNK_,x)(182) cat(THUNK_,x)(183) cat(THUNK_,x)(184) cat(THUNK_,x)(185) cat(THUNK_,x)(186) cat(THUNK_,x)(187) cat(THUNK_,x)(188) cat(THUNK_,x)(189) \
cat(THUNK_,x)(190) cat(THUNK_,x)(191) cat(THUNK_,x)(192) cat(THUNK_,x)(193) cat(THUNK_,x)(194) cat(THUNK_,x)(195) cat(THUNK_,x)(196) cat(THUNK_,x)(197) cat(THUNK_,x)(198) cat(THUNK_,x)(199)        

#endif // _MAC

#pragma INCMSG("--- End 'tearoff.hxx'")
#else
#pragma INCMSG("*** Dup 'tearoff.hxx'")
#endif
