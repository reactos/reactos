//+---------------------------------------------------------------------------
//
//  Maintained by: Jerry, Terry and Ted
//
//  Microsoft DataDocs
//  Copyright (C) Microsoft Corporation, 1994-1996
//  All rights Reserved.
//  Information contained herein is Proprietary and Confidential.
//
//  File:       site\dbind\convert.cxx
//
//  Contents:   Core conversion code.
//
//  Classes:    CTypeCoerce
//
//  Functions:  None.
//

#include "headers.hxx"

#ifndef X_DBINDING_HXX_
#define X_DBINDING_HXX_
#include "dbinding.hxx"
#endif

/////////////////////////////////////////////////////////////////////////////
//  Steps to add a new type for conversion:
//
//      1.  Add a new canonical type to CAN_TYPE list.
//
//      2.  Modify CanonicalizeType to canonicalize a new DBTYPE (VARTYPE) to
//          the CAN_TYPE value.  NOTE: Be careful of any overlap between DBTYPE
//          and VARTYPE, currently there are none.
//
//      3.  Update the JumpTable to include the address of routines to do the
//          actual conversion.
//
//      4.  Add CallStr entry for new TYPE_xxxx (convert from type to string)
//          add CallThru entry for new TYPE_xxxx.
//
//      5.  Test it.
//
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//
//  The following the conversion matrix of database types to OLE types and
//  vice versa.
//
//
// FROM                                    TO
// ====                                    ==
//
//         Null  I2    I4    R4    R8    CY    DATE  BSTR  BOOL  VARI 
//         ===== ===== ===== ===== ===== ===== ===== ===== ===== =====
//  Empty  *NuNu XNuI2 XNuI4 XNuR4 XNuR8 XNuCY XNuDT XNuBS XNuBO XNuVA
//  I2     XI2Nu *I2I2 XI2I4 XI2R4 XI2R8  I2CY  I2DT  I2BS  I2BO  I2VA
//  I4     XI4Nu  I4I2 *I4I4  I4R4 XI4R8  I4CY  I4DT  I4BS  I4B0  I4VA
//  R4     XR4Nu  R4I2  R4I4 *R4R4 XR4R8  R4CY  R4DT  R4BS  R4B0  R4VA
//  R8     XR8Nu  R8I2  R8I4  R8R4 *R8R8  R8CY  R8DT  R8BS  R8BO  R8VA
//  CY     XCYNu  CYI2  CYI4  CYR4  CYR8 *CYCY  CYDT  CYBS  CYBO  CYVA
//  DATE   XDTNu  DTI2  DTI4  DTR4  DTR8  DTCY *DTDT  DTBS  DTBO  DTVA
//  BSTR   XBSNu  BSI2  BSI4  BSR4  BSR8  BSCY  BSDT *BSBS  BSBP  BSVA
//  BOOL   XBONu XBOI2 XBOI4 XBOR4 XBOR8  BOCY  BODT  BOBS *BOBO  BOVA
//  VARI   XVANu  VAI2  VAI4  VAR4  VAR8  VACY  VADT  VABS  VABO *VAVA
//
//
//  Key
//  ===
//
//  *     = copy                  Nu    null
//  Xxxxx = upcast                I2    short
//                                I4    long
//                                R4    float
//                                R8    double
//                                CY    currency
//                                DATE  date
//                                BSTR  BSTR
//                                BOOL  boolean
//
// NOTE:
// =====
//
//      The above table is a static array named JumpTable located in the routine
//      ConvertData.
//
////////////////////////////////////////////////////////////////////////////////

// NO_CONVERSION  - Signals that the types are identical.
// PROCESS_UPCAST - Signals that the type can be upcast to the to type w/o worry
//                  of overflow checking.
// PROCESS_NOCALL - Signals processing must be done but the there is no routine
//                  to call.
#define NO_CONVERSION   ((void *)0)
#define PROCESS_UPCAST  ((void *)1)
#define PROCESS_NOCALL  ((void *)~0)



//+---------------------------------------------------------------------------
//
//  Member:     CanonicalizeType (public static member)
//
//  Synopsis:   Canonicalize the DBTYPE and VARTYPES.  Notice that the types
//              match one for one except for types > DBTYPE_STR which are than
//              canonicalized.
//
//              The canonicalizing is based on the values for DBTYPE_xxxx (the
//              VT_xxxx values <= 13 match exactly the DBTYPEs the VTs largest
//              value is 72).  We are interested in I2, I4, R4, R8, CY, DATE,
//              BSTR, BOOL, and VARIANT
//
//                  DBTYPE_EMPTY          0     (Remove Group 1)
//                  DBTYPE_NULL           1     (Group 1)
//                  DBTYPE_I2             2
//                  DBTYPE_I4             3
//                  DBTYPE_R4             4
//                  DBTYPE_R8             5
//                  DBTYPE_CY             6
//                  DBTYPE_DATE           7
//                  DBTYPE_BSTR           8
//                  DBTYPE_DISPATCH       9     (Remove Group 2)
//                  DBTYPE_ERROR         10     (Remove)
//                  DBTYPE_BOOL          11
//                  DBTYPE_VARIANT       12
//                  DBTYPE_UNKNOWN       13     (Remove Group 3)
//                  DBTYPE_ARRAY       8192     (Remove)
//                  DBTYPE_BYREF      16384     (Remove)
//                  DBTYPE_RESERVED   32768     (Remove)
//                  DBTYPE_UI1          128     (Remove)
//                  DBTYPE_I8            20     (Remove)
//                  DBTYPE_GUID          72     (Remove)
//                  DBTYPE_VECTOR      4096     (Remove)
//                  DBTYPE_STR          129     (Remove)
//                  DBTYPE_WSTR         130     (Remove)
//                  DBTYPE_NUMERIC      131     (Remove)
//
//  Arguments:  None
//
//  Returns:    Returns the canonical type (CAN_TYPE)
//

HRESULT
CTypeCoerce::CanonicalizeType (DBTYPE dbType, CAN_TYPE & canType)
{
    static const CAN_TYPE aTypeSimple[] =
    {
        /* DBTYPE_EMPTY     */ TYPE_END,
        /* DBTYPE_NULL      */ TYPE_NULL,
        /* DBTYPE_I2        */ TYPE_I2,
        /* DBTYPE_I4        */ TYPE_I4,
        /* DBTYPE_R4        */ TYPE_R4,
        /* DBTYPE_R8        */ TYPE_R8,
        /* DBTYPE_CY        */ TYPE_CY,
        /* DBTYPE_DATE      */ TYPE_DATE,
        /* DBTYPE_BSTR      */ TYPE_BSTR,
        /* DBTYPE_IDISPATCH */ TYPE_END,
        /* DBTYPE_ERROR     */ TYPE_END,
        /* DBTYPE_BOOL      */ TYPE_BOOL,
        /* DBTYPE_VARIANT   */ TYPE_VARIANT,
    };
    
    Assert(DBTYPE_EMPTY == 0);
    Assert(DBTYPE_NULL == DBTYPE_EMPTY+1);
    Assert(DBTYPE_I2 == DBTYPE_NULL + 1);
    Assert(DBTYPE_I4 == DBTYPE_I2 + 1);
    Assert(DBTYPE_R4 == DBTYPE_I4 + 1);
    Assert(DBTYPE_R8 == DBTYPE_R4 + 1);
    Assert(DBTYPE_CY == DBTYPE_R8 + 1);
    Assert(DBTYPE_DATE == DBTYPE_CY + 1);
    Assert(DBTYPE_BSTR == DBTYPE_DATE + 1);
    Assert(DBTYPE_IDISPATCH == DBTYPE_BSTR + 1);
    Assert(DBTYPE_ERROR == DBTYPE_IDISPATCH + 1);
    Assert(DBTYPE_BOOL == DBTYPE_ERROR + 1);
    Assert(DBTYPE_VARIANT == DBTYPE_BOOL + 1);
    Assert(DBTYPE_IUNKNOWN == DBTYPE_VARIANT + 1);
    Assert(DBTYPE_WSTR >  DBTYPE_IUNKNOWN);
    Assert(DBTYPE_HCHAPTER >  DBTYPE_IUNKNOWN);
    

    HRESULT     hr = S_OK;
    
    canType = TYPE_END;
    
    if (dbType < ARRAY_SIZE(aTypeSimple))
    {
        canType = aTypeSimple[dbType];
    }
    else if (dbType == DBTYPE_HCHAPTER)
    {
        canType = TYPE_CHAPTER;
    }

    if (canType == TYPE_END)
    {
        hr = OLE_E_CANTCONVERT;
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     BoolFromStr, private helper (static)
//
//  Synopsis:   used instead of OLE Automation's normal VARIANTBoolFromStr
//              because OLE's conversions aren't compatible enough with
//              Netscape's javascript conversions.
//
//  Arguments:  OLECHAR *       String to convert to BOOL
//
//  Returns:    VARIANT_BOOL    VB_TRUE (0xffff) or VB_FALSE (0x0000)
//                              (be careful, VARIANT_BOOLs are only 16 bits,
//                              normal Win32 Bools are 32 bits!)
//
VARIANT_BOOL
CTypeCoerce::BoolFromStr(OLECHAR *pValueIn)
{
    // The NULL string is FALSE, all other strings are TRUE
    return (pValueIn==NULL || *pValueIn==0) ? VB_FALSE : VB_TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CallStr (private static member)
//
//  Synopsis:   Call low-level OLE conversion VarBstrFromXXXX.
//
//  Arguments:  pCall   - Function to call.
//              inType  - Type of pIn
//              pIn     - Value of data
//              outType - Type of pOut
//              pOut    - Value of data
//
//  Returns:    Returns HRESULT from VarBstrFromXXXX.
//

// The generic function pointer types which CallStr uses.
typedef HRESULT (STDMETHODCALLTYPE *BstrFrmI2)(short, LCID, unsigned long, BSTR FAR*);
typedef HRESULT (STDMETHODCALLTYPE *BstrFrmI4)(long, LCID, unsigned long, BSTR FAR*);
typedef HRESULT (STDMETHODCALLTYPE *BstrFrmR4)(float, LCID, unsigned long, BSTR FAR*);
typedef HRESULT (STDMETHODCALLTYPE *BstrFrmR8)(double, LCID, unsigned long, BSTR FAR*);
typedef HRESULT (STDMETHODCALLTYPE *BstrFrmCy)(CY, LCID, unsigned long, BSTR FAR*);
typedef HRESULT (STDMETHODCALLTYPE *BstrFrmDate)(DATE, LCID, unsigned long, BSTR FAR*);
typedef HRESULT (STDMETHODCALLTYPE *BstrFrmBool)(VARIANT_BOOL, LCID, unsigned long,
                                         BSTR FAR*);

HRESULT
CTypeCoerce::CallStr (void *pCall,
                      CAN_TYPE inType, void * pIn,
                      void * pOut)
{
    HRESULT hr = E_UNEXPECTED;

    BSTR bstr = 0;

    switch (inType)
    {
        case TYPE_I2:
            Assert(pCall != PROCESS_NOCALL);
            hr = (*(BstrFrmI2)pCall) (*(short *)pIn, LOCALE_USER_DEFAULT, 0,
                                      &bstr);
            break;
        case TYPE_I4:
            Assert(pCall != PROCESS_NOCALL);
            hr = (*(BstrFrmI4)pCall) (*(long *)pIn, LOCALE_USER_DEFAULT, 0,
                                      &bstr);
            break;
        case TYPE_R4:
            Assert(pCall != PROCESS_NOCALL);
            hr = (*(BstrFrmR4)pCall) (*(float *)pIn, LOCALE_USER_DEFAULT, 0,
                                      &bstr);
            break;
        case TYPE_R8:
            Assert(pCall != PROCESS_NOCALL);
            hr = (*(BstrFrmR8)pCall) (*(double *)pIn, LOCALE_USER_DEFAULT, 0,
                                      &bstr);
            break;
        case TYPE_CY:
            Assert(pCall != PROCESS_NOCALL);
            hr = (*(BstrFrmCy)pCall) (*(CY *)pIn, LOCALE_NOUSEROVERRIDE, 0,
                                      &bstr);
            break;
        case TYPE_DATE:
            Assert(pCall != PROCESS_NOCALL);
            hr = (*(BstrFrmDate)pCall) (*(DATE *)pIn, LOCALE_USER_DEFAULT, 0,
                                        &bstr);
            break;
        case TYPE_BOOL:
            Assert(pCall != PROCESS_NOCALL);
            // We actually ignore what's in the table and special case this
            // to do sort of Netscape compatible conversions.
#ifdef NEVER            
            // THIS CODE IS IFDEF'D NEVER UNTIL WE'RE SURE PM WON'T CHANGE
            // THEIR MIND AGAIN ABOUT HOW THIS CONVERSION WORKS
            bstr =  SysAllocString(*(VARIANT_BOOL *)pIn ?
                                   _T("true") : NULL);
            // BUGBUG:: Should we consider trying to detect a rather
            // unlikely E_NOMEMORY on allocating "true", here?
            hr = S_OK;
#else
            hr = VarBstrFromBool(*(VARIANT_BOOL *)pIn, LOCALE_USER_DEFAULT, 0, &bstr);
#endif
            break;
        default:
            Assert(!"In type is unknown, or conversion unnecessary!");
    }

    if (!hr)
    {
        *(BSTR *)pOut = bstr;
    }

    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//
//  Member:     CallThru (private static member)
//
//  Synopsis:   Call low-level OLE conversion VarXXXXFromXXXX where the to type
//              is never Bstr.
//
//  Arguments:  pCall   - Function to call.
//              inType  - Type of pIn
//              pIn     - Value of data
//              pOut    - Value of data
//
//  Returns:    Returns HRESULT from VarXXXXFromXXXX.
//

// The generic function pointer types which CallThru uses.
typedef HRESULT (STDMETHODCALLTYPE *FromI2)(short, void *);
typedef HRESULT (STDMETHODCALLTYPE *FromI4)(long, void *);
typedef HRESULT (STDMETHODCALLTYPE *FromR4)(float, void *);
typedef HRESULT (STDMETHODCALLTYPE *FromR8)(double, void *);
typedef HRESULT (STDMETHODCALLTYPE *FromDate)(DATE, void *);
typedef HRESULT (STDMETHODCALLTYPE *FromCy)(CY, void *);
typedef HRESULT (STDMETHODCALLTYPE *FromBool)(VARIANT_BOOL, void *);
typedef HRESULT (STDMETHODCALLTYPE *FromBStr)(OLECHAR *, LCID, unsigned long, void *);

HRESULT
CTypeCoerce::CallThru (void * pCall, CAN_TYPE inType, void * pIn, void * pOut)
{
    Assert(pCall && pCall != PROCESS_NOCALL);

    HRESULT hr;

    switch (inType)
    {
        case TYPE_I2:
            hr = (*(FromI2)pCall) (*(short *)pIn, pOut);
            break;
        case TYPE_I4:
            hr = (*(FromI4)pCall) (*(int *)pIn, pOut);
            break;
        case TYPE_R4:
            hr = (*(FromR4)pCall) (*(float *)pIn, pOut);
            break;
        case TYPE_R8:
            hr = (*(FromR8)pCall) (*(double *)pIn, pOut);
            break;
        case TYPE_CY:
            hr = (*(FromCy)pCall) (*(CY *)pIn, pOut);
            break;
        case TYPE_DATE:
            hr = (*(FromDate)pCall) (*(DATE *)pIn, pOut);
            break;
        case TYPE_BSTR:
            hr = (*(FromBStr)pCall) (*(BSTR *)pIn, LOCALE_USER_DEFAULT, 0,
                                     pOut);
            break;
        case TYPE_BOOL:
            hr = (*(FromBool)pCall) (*(VARIANT_BOOL *)pIn, pOut);
            break;
        default:
            Assert(!"In type is unknown.");
            hr = OLE_E_CANTCONVERT;
    }

    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//
//  Member:     ConvertData (public static member)
//
//  Synopsis:   Given two buffer and type pairs convert the input type to the
//              output type.
//
//  Arguments:  typeIn      - type of data in input buffer
//              pValueIn    - input buffer (data to convert)
//              typeOut     - type of data in output buffer after conversion
//              pValueOut   - output buffer (data to convert to)
//
//  Returns:    S_OK        everything is fine
//              E_xxxx      any VarNNNFromNNN low-level OLE conversion functions
//

HRESULT
CTypeCoerce::ConvertData (CAN_TYPE typeIn, void * pValueIn,
                          CAN_TYPE typeOut, void * pValueOut)
{
    // This array is static and located in this routine so it is only initial-
    // ized once -- the first time this routine is called.
    static void * JumpTable[MAX_CAN_TYPES][MAX_CAN_TYPES] =
    {
        //  *NuNu XNuI2 XNuI4 XNuR4 XNuR8 XNuCY XNuDT XNuBS XNuBO XNuVA
        {   NO_CONVERSION,   PROCESS_UPCAST,  PROCESS_UPCAST,  PROCESS_UPCAST,
            PROCESS_UPCAST,  PROCESS_UPCAST,  PROCESS_UPCAST,  PROCESS_UPCAST,
            PROCESS_UPCAST,  PROCESS_UPCAST      },

        //  XI2Nu  *I2I2 XI2I4 XI2R4 XI2R8  I2CY  I2DT  I2BS  I2BO  I2VA
        {   PROCESS_UPCAST,  NO_CONVERSION,   PROCESS_UPCAST,  PROCESS_UPCAST,
            PROCESS_UPCAST,  &VarCyFromI2,    &VarDateFromI2,  &VarBstrFromI2,
            &VarBoolFromI2,  PROCESS_UPCAST      },

        //  XI4Nu  I4I2 *I4I4  I4R4 XI4R8  I4CY  I4DT  I4BS   I4B0  I4BA
        {   PROCESS_UPCAST,  &VarI2FromI4,    NO_CONVERSION,   &VarR4FromI4,
            PROCESS_UPCAST,  &VarCyFromI4,    &VarDateFromI4,  &VarBstrFromI4,
            &VarBoolFromI4,  PROCESS_UPCAST      },

        //  XR4Nu  R4I2  R4I4 *R4R4 XR4R8  R4CY  R4DT  R4BS  R4B0  R4VA
        {   PROCESS_UPCAST,  &VarI2FromR4,    &VarI4FromR4,    NO_CONVERSION,
            PROCESS_UPCAST,  &VarCyFromR4,    &VarDateFromR4,  &VarBstrFromR4,
            &VarBoolFromR4,  PROCESS_UPCAST      } ,

        //  XR8Nu  R8I2  R8I4  R8R4 *R8R8  R8CY  R8DT  R8BS  R8BO  R8VA
        {   PROCESS_UPCAST,  &VarI2FromR8,    &VarI4FromR8,    &VarR4FromR8,
            NO_CONVERSION,   &VarCyFromR8,    &VarDateFromR8,  &VarBstrFromR8,
            &VarBoolFromR8,  PROCESS_UPCAST      },

        //  XCYNu  CYI2  CYI4  CYR4  CYR8 *CYCY  CYDT  CYBS  CYBO  CYVA
        {   PROCESS_UPCAST,  &VarI2FromCy,    &VarI4FromCy,    &VarR4FromCy,
            &VarR8FromCy,    NO_CONVERSION,   &VarDateFromCy,  &VarBstrFromCy,
            &VarBoolFromCy,  PROCESS_UPCAST      },

        //  XDTNu  DTI2  DTI4  DTR4  DTR8  DTCY *DTDT  DTBS  DTBO  DTVA
        {   PROCESS_UPCAST,  &VarI2FromDate,  &VarI4FromDate,  &VarR4FromDate,
            &VarR8FromDate,  &VarCyFromDate,  NO_CONVERSION,   &VarBstrFromDate,
            &VarBoolFromDate,PROCESS_UPCAST    },

        //  XBSNu  BSI2  BSI4  BSR4  BSR8  BSCY  BSDT *BSBS  BSBP  BSVA
        {   PROCESS_UPCAST,  &VarI2FromStr,   &VarI4FromStr,   &VarR4FromStr,
            &VarR8FromStr,   &VarCyFromStr,   &VarDateFromStr, NO_CONVERSION,
            PROCESS_UPCAST, PROCESS_UPCAST      },

        //  XBONu  XBOI2 XBOI4 XBOR4 XBOR8  BOCY  BODT  BOBS *BOBO  BOVA
        {   PROCESS_UPCAST,  PROCESS_UPCAST, PROCESS_UPCAST,  PROCESS_UPCAST,
            PROCESS_UPCAST,  &VarCyFromBool, &VarDateFromBool,&VarBstrFromBool,
            NO_CONVERSION,   PROCESS_UPCAST     },

        //  XVANu   VAI2  VAI4  VAR4  VAR8  VACY  VADT  VABS  VABO *VAVA
        {   PROCESS_UPCAST,  PROCESS_UPCAST, PROCESS_UPCAST,  PROCESS_UPCAST,
            PROCESS_UPCAST,  PROCESS_UPCAST, PROCESS_UPCAST,  PROCESS_UPCAST,
            PROCESS_UPCAST,  NO_CONVERSION       }
    };

    HRESULT     hr = OLE_E_CANTCONVERT;
    VARIANT     *pVarIn = (VARIANT *)pValueIn;
    VARIANT     *pVarOut = (VARIANT *)pValueOut;

    Assert(typeIn != TYPE_CHAPTER);
    Assert(typeOut != TYPE_CHAPTER);
    Assert(pValueIn && pValueOut);

    void * pFunc = JumpTable[typeIn][typeOut];

    // How do we convert?
    if (pFunc == PROCESS_UPCAST)
    {
        // Upcast.
        switch (typeIn)
        {
        case TYPE_NULL:
            hr = S_OK;

            switch (typeOut)
            {
            case TYPE_I2:
                *(short *)pValueOut = 0;
                break;
            case TYPE_I4:
                *(long *)pValueOut = 0;
                break;
            case TYPE_R4:
                *(float *)pValueOut = 0.0F;
                break;
            case TYPE_R8:
                *(double *)pValueOut = 0;
                break;
            case TYPE_CY:
                ((CY *)pValueOut)->Lo = 0;
                ((CY *)pValueOut)->Hi = 0;
                break;
            case TYPE_DATE:
                *(DATE *)pValueOut = 0;
                break;
            case TYPE_BSTR:
                *(BSTR *)pValueOut = 0;
                break;
            case TYPE_BOOL:
                *(VARIANT_BOOL *)pValueOut = 0;
                break;
            case TYPE_VARIANT:
                pVarOut->vt = VT_EMPTY;
                break;
            default:
                Assert(!"Can't cast this empty");
                break;            
            }
            break;
        
        case TYPE_I2:
            if (typeOut == TYPE_I4)
                *(long *)pValueOut = *(short *)pValueIn;
            else if (typeOut == TYPE_R4)
                *(float *)pValueOut = *(short *)pValueIn;
            else if (typeOut == TYPE_R8)
                *(double *)pValueOut = *(short *)pValueIn;
            else if (typeOut == TYPE_VARIANT)
            {
                pVarOut->vt = VT_I2;
                pVarOut->iVal = *(short *)pValueIn;
            }
            else if (typeOut != TYPE_NULL)
            {
                Assert(!"Can't cast this short");
                break;
            }

            hr = S_OK;
            break;

        case TYPE_I4:
            if (typeOut == TYPE_R8)
                *(double *)pValueOut = *(long *)pValueIn;
            else if (typeOut == TYPE_VARIANT)
            {
                pVarOut->vt = VT_I4;
                pVarOut->lVal = *(long *)pValueIn;
            }
            else if (typeOut != TYPE_NULL)
            {
                Assert(!"Can't cast this long");
                break;
            }

            hr = S_OK;
            break;

        case TYPE_R4:
            if (typeOut == TYPE_R8)
                *(double *)pValueOut = *(float *)pValueIn;
            else if (typeOut == TYPE_VARIANT)
            {
                pVarOut->vt = VT_R4;
                pVarOut->fltVal = *(float *)pValueIn;
            }
            else if (typeOut != TYPE_NULL)
            {
                Assert(!"Can't cast this float");
                break;
            }

            hr = S_OK;
            break;

        case TYPE_R8:
            if (typeOut == TYPE_VARIANT)
            {
                pVarOut->vt = VT_R8;
                pVarOut->dblVal = *(double *)pValueIn;
            }
            else if (typeOut != TYPE_NULL)
            {
                Assert(!"Can't cast this double");
                break;
            }

            hr = S_OK;
            break;

        case TYPE_CY:
            if (typeOut == TYPE_VARIANT)
            {
                pVarOut->vt = VT_CY;
                pVarOut->cyVal = *(CY *)pValueIn;
            }
            else if (typeOut != TYPE_NULL)
            {
                Assert(!"Can't cast this CY");
                break;
            }

            hr = S_OK;
            break;

        case TYPE_DATE:
            if (typeOut == TYPE_VARIANT)
            {
                pVarOut->vt = VT_DATE;
                pVarOut->date = *(DATE *)pValueIn;
            }
            else if (typeOut != TYPE_NULL)
            {
                Assert(!"Can't cast this date");
                break;
            }

            hr = S_OK;
            break;

        case TYPE_BSTR:
            if (typeOut == TYPE_VARIANT)
            {
                pVarOut->vt = VT_BSTR;
                pVarOut->bstrVal = *(BSTR*)pValueIn;
            }
            else if (typeOut == TYPE_BOOL)
            {
                // Special conversion rules for Checkbox-like elements
                // BUGBUG:: 0 is probably not the proper Locale, &
                // NOUSEROVERRIDE is probably wrong.  -cfranks 27 Mar 1997
                hr = VarBoolFromStr(*(OLECHAR **)pValueIn, LOCALE_USER_DEFAULT, 0,
                               (VARIANT_BOOL *)pValueOut);
                if (hr)
                {
                    *(VARIANT_BOOL *)pValueOut = BoolFromStr(*(OLECHAR **)pValueIn);
                    hr = S_OK;
                }
            }
            else if (typeOut != TYPE_NULL)
            {
                Assert(!"Can't cast this BSTR");
                break;
            }

            hr = S_OK;
            break;

        case TYPE_BOOL:
            if (typeOut == TYPE_I2)
                *(short *)pValueOut = *(VARIANT_BOOL *)pValueIn;
            else if (typeOut == TYPE_I4)
                *(long *)pValueOut = *(VARIANT_BOOL *)pValueIn;
            else if (typeOut == TYPE_R4)
                *(float *)pValueOut = *(VARIANT_BOOL *)pValueIn;
            else if (typeOut == TYPE_R8)
                *(double *)pValueOut = *(VARIANT_BOOL *)pValueIn;
            else if (typeOut == TYPE_VARIANT)
            {
                pVarOut->vt = VT_BOOL;
                pVarOut->boolVal = *(VARIANT_BOOL *)pValueIn;
            }
            else if (typeOut != TYPE_NULL)
            {
                Assert(!"Can't cast this boolean");
                break;
            }

            hr = S_OK;
            break;

        case TYPE_VARIANT:
        {
            hr = S_OK;

            if ((typeOut == TYPE_I2) && (pVarIn->vt == VT_I2))
                *(short *)pValueOut = ((VARIANT *)pValueIn)->iVal;
            else if ((typeOut == TYPE_I4) && (pVarIn->vt == VT_I4))
                *(long *)pValueOut = ((VARIANT *)pValueIn)->lVal;
            else if ((typeOut == TYPE_R4) && (pVarIn->vt == VT_R4))
                *(float *)pValueOut = ((VARIANT *)pValueIn)->fltVal;
            else if ((typeOut == TYPE_R8) && (pVarIn->vt == VT_R8))
                *(double *)pValueOut = ((VARIANT *)pValueIn)->dblVal;
            else if ((typeOut == TYPE_CY) && (pVarIn->vt == VT_CY))
                *(CY *)pValueOut = ((VARIANT *)pValueIn)->cyVal;
            else if ((typeOut == TYPE_DATE) && (pVarIn->vt == VT_DATE))
                *(DATE *)pValueOut = ((VARIANT *)pValueIn)->date;
            else if ((typeOut == TYPE_BSTR) && (pVarIn->vt == VT_BSTR))
                *(BSTR *)pValueOut = ((VARIANT *)pValueIn)->bstrVal;
            else if ((typeOut == TYPE_BOOL) && (pVarIn->vt == VT_BOOL))
                *(VARIANT_BOOL *)pValueOut = ((VARIANT *)pValueIn)->boolVal;
            else if (typeOut == TYPE_NULL)
            {
                // Do nothing.
            }
            else
            {
                // Indirect the input to convert the data in the variant to our
                // destination type.
                CAN_TYPE inType;

                hr = CanonicalizeType(pVarIn->vt, inType);
                if (!hr)
                {
                    hr = ConvertData(inType, &pVarIn->iVal, typeOut, pValueOut);
                }
            }
            break;
        }

        default:
            Assert(!"Can't cast up");
            break;
        }
    }
    else if (pFunc != NO_CONVERSION)
    {
        // We got a function adress or PROCESS_NOCALL.
        if (typeOut == TYPE_BSTR)       // BSTR?
        {
            hr = CallStr(pFunc, typeIn, pValueIn, pValueOut);
        }
        else
        {
            hr = CallThru(pFunc, typeIn, pValueIn, pValueOut);
        }
    }
    else
    {
        // pFunc == NO_CONVERSION
        Assert(!"We NEVER want to copy the same exact type \n"
                "GetValue/SetValue should have done it.");
        hr = E_INVALIDARG;
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     FVariantType
//
//  Synopsis:   Returns wehter the given DBTYPE is legal in an automation
//              VARIANT.
//
//  Arguments:  dbType  - the database type
//
//  Returns:    TRUE        OK in variant
//              FALSE       not OK in variant
//

BOOL
CTypeCoerce::FVariantType(DBTYPE dbType)
{
    // Variant.h header file show that all acceptable base types
    //   are < 0x20.
    if (dbType & ~(VT_ARRAY|VT_BYREF|0x1F))
    {
        return FALSE;
    }
    // the bitmask below was built by hand from comments in Variant.h
    //
    return ((1 << (dbType &0x1f)) & 0xcf7fff) != 0;
}


//+--------------------------------------------------------------------------
//
//  Function:   IsNearlyEqual (local helper)
//
//  Synopsis:   Determines whether two floating-point numbers are "equal"
//              within a tolerance allowing for the usual floating-point
//              vagaries (drift, loss of precision, roundoff, etc)

static BOOL
IsNearlyEqual(double d1, double d2)
{
    if (d1 == 0.0)
    {
        return (d1 == d2);
    }
    else
    {
        double dRelError = (d1-d2)/d1;
        if (dRelError < 0)
            dRelError = -dRelError;
        return (dRelError < 1.0e-6);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     IsEqual, static
//
//  Synopsis:   For two values of the same type VariantType, determines 
//              whether or no their values match.  Used by databinding
//              code to compare contents of element with contents of
//              databas, and to decide whether or not to actually save
//              changed data to the data source.
//
//  Arguments:  vt  - data type of values being compared
//              pv1 - pointer to data of first value
//              pv2 - pointer to data of second value
//
//  Returns:    S_OK        The two values match
//              S_FALSE     The values don't match
//              E_*         Something went wrong, assume failure.
//

HRESULT
CTypeCoerce::IsEqual(VARTYPE vt, void *pv1, void *pv2)
{
    HRESULT hr = S_OK;
    
    switch (vt)
    {
    case VT_BSTR:
        if (FormsStringCmp(* (BSTR *) pv1, * (BSTR *) pv2))
        {
            hr = S_FALSE;
        }
        break;
    case VT_VARIANT:
        if (((VARIANT *) pv1)->vt != ((VARIANT *) pv2)->vt)
        {
            hr = S_FALSE;
        }
        else
        {
            hr = IsEqual(((VARIANT *) pv1)->vt,
                         & ((VARIANT *) pv1)->iVal,
                         & ((VARIANT *) pv2)->iVal);
        }
        break;

    case VT_BOOL:
        // In theory, these should be VARIANT_BOOLs with only two possible
        //  values: -1 and 0.  However, to be cautious, we'll consider any
        //  two non-zero values to match.
        // However, I've changed these to VARIANT_BOOL rather than BOOL.
        // Since most of our code only assigns the first 16 bits, we should
        // only look at the first 16 bits.  -cfranks 10 Jan 97
        if (! * (VARIANT_BOOL *) pv1 != ! * (VARIANT_BOOL *) pv2)
        {
            hr = S_FALSE;
        }
        break;
        
    case VT_NULL:
    case VT_EMPTY:
        // we violate conventional database rules, and say that NULL==NULL.
        //  This behavior is required by our clients, which use this routine
        //  to figure out if an element's contents differ from that in the
        //  database.
        break;
        
    case VT_I2:
        if (* (WORD *) pv1 != * (WORD *) pv2)
        {
            hr = S_FALSE;
        }
        break;
        
    case VT_I4:
        if (* (DWORD *) pv1 != * (DWORD *) pv2)
        {
            hr = S_FALSE;
        }
        break;

    case VT_R4:
        if (!IsNearlyEqual(* (float *) pv1, * (float *) pv2))
        {
            hr = S_FALSE;
        }
        break;
        
    case VT_R8:
        if (!IsNearlyEqual(* (double *) pv1, * (double *) pv2))
        {
            hr = S_FALSE;
        }
        break;

    case VT_DATE:
        if (* (double *) pv1 != * (double *) pv2)
        {
            hr = S_FALSE;
        }
        break;

    case VT_CY:
#ifdef UNIX
        if (CY_INT64(pv1) != CY_INT64(pv2))
#else
        if (((CY *) pv1)->int64 != ((CY *) pv2)->int64)
#endif
        {
            hr = S_FALSE;
        }
        break;

    case VT_UNKNOWN:
    case VT_DISPATCH:
        // BUGBUG: should probably call IsSameObject
        if (((VARIANT *) pv1)->punkVal != ((VARIANT *) pv2)->punkVal)
        {
            hr = S_FALSE;
        }
        break;

    default:
        Assert(!"comparing unexpected databinding type.");
        hr = E_FAIL;
        break;
    }
    RRETURN1(hr, S_FALSE);
}

