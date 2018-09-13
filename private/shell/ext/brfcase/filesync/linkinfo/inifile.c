/*
 * inifile.c - Initialization file processing module.
 */


/* Headers
 **********/

#include "project.h"
#pragma hdrstop


/* Constants
 ************/

/* maximum length of .ini switch RHS */

#define MAX_INI_SWITCH_RHS_LEN      MAX_PATH_LEN


/* Module Variables
 *******************/

#ifdef DEBUG

#pragma data_seg(DATA_SEG_READ_ONLY)

/* Boolean TRUE strings used by IsIniYes() (comparison is case-insensitive) */

PRIVATE_DATA const LPCTSTR MrgcpcszTrue[] =
{
   TEXT("1"),
   TEXT("On"),
   TEXT("True"),
   TEXT("Y"),
   TEXT("Yes")
};

/* Boolean FALSE strings used by IsIniYes() (comparison is case-insensitive) */

PRIVATE_DATA const LPCTSTR MrgcpcszFalse[] =
{
   TEXT("0"),
   TEXT("Off"),
   TEXT("False"),
   TEXT("N"),
   TEXT("No")
};

#pragma data_seg()

#endif


/***************************** Private Functions *****************************/

/* Module Prototypes
 ********************/

#ifdef DEBUG

PRIVATE_CODE BOOL SetBOOLIniSwitch(PCBOOLINISWITCH);
PRIVATE_CODE BOOL SetDecimalIntIniSwitch(PCDECINTINISWITCH);
PRIVATE_CODE BOOL SetIniSwitch(PCVOID);
PRIVATE_CODE BOOL IsYesString(LPCTSTR);
PRIVATE_CODE BOOL IsNoString(LPCTSTR);
PRIVATE_CODE BOOL IsStringInList(LPCTSTR, const LPCTSTR *, UINT);
PRIVATE_CODE BOOL IsValidPCBOOLINISWITCH(PCBOOLINISWITCH);
PRIVATE_CODE BOOL IsValidPCDECINTINISWITCH(PCDECINTINISWITCH);
PRIVATE_CODE BOOL IsValidPCUNSDECINTINISWITCH(PCUNSDECINTINISWITCH);

#endif


#ifdef DEBUG

/*
** SetBOOLIniSwitch()
**
** 
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL SetBOOLIniSwitch(PCBOOLINISWITCH pcbis)
{
   DWORD dwcbKeyLen;
   TCHAR rgchRHS[MAX_INI_SWITCH_RHS_LEN];

   ASSERT(IS_VALID_STRUCT_PTR(pcbis, CBOOLINISWITCH));

   /* Set boolean .ini switch. */

   dwcbKeyLen = GetPrivateProfileString(GpcszIniSection, pcbis->pcszKeyName,
                                        TEXT(""), rgchRHS, ARRAYSIZE(rgchRHS),
                                        GpcszIniFile);

   /* Is the .ini switch set? */

   if (rgchRHS[0])
   {
      /* Yes.  Set or clear flag? */

      if (IsYesString(rgchRHS))
      {
         /* Set flag. */

         if (IS_FLAG_CLEAR(*(pcbis->pdwParentFlags), pcbis->dwFlag))
         {
            SET_FLAG(*(pcbis->pdwParentFlags), pcbis->dwFlag);

            WARNING_OUT((TEXT("SetBOOLIniSwitch(): %s set in %s![%s]."),
                         pcbis->pcszKeyName,
                         GpcszIniFile,
                         GpcszIniSection));
         }
      }
      else if (IsNoString(rgchRHS))
      {
         /* Clear flag. */

         if (IS_FLAG_SET(*(pcbis->pdwParentFlags), pcbis->dwFlag))
         {
            CLEAR_FLAG(*(pcbis->pdwParentFlags), pcbis->dwFlag);

            WARNING_OUT((TEXT("SetBOOLIniSwitch(): %s cleared in %s![%s]."),
                         pcbis->pcszKeyName,
                         GpcszIniFile,
                         GpcszIniSection));
         }
      }
      else
         /* Unknown flag. */
         WARNING_OUT((TEXT("SetBOOLIniSwitch(): Found unknown Boolean RHS %s for %s in %s![%s]."),
                      rgchRHS,
                      pcbis->pcszKeyName,
                      GpcszIniFile,
                      GpcszIniSection));
   }

   return(TRUE);
}


/*
** SetDecimalIntIniSwitch()
**
** 
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL SetDecimalIntIniSwitch(PCDECINTINISWITCH pcdiis)
{
   INT nNewValue;

   ASSERT(IS_VALID_STRUCT_PTR(pcdiis, CDECINTINISWITCH));

   /* Get decimal integer .ini switch. */

   nNewValue = GetPrivateProfileInt(GpcszIniSection, pcdiis->pcszKeyName,
                                    *(pcdiis->pnValue), GpcszIniFile);

   /* New value? */

   if (nNewValue != *(pcdiis->pnValue))
   {
      /* Yes. */

      *(pcdiis->pnValue) = nNewValue;

      WARNING_OUT((TEXT("SetDecimalIntIniSwitch(): %s set to %d in %s![%s]."),
                   pcdiis->pcszKeyName,
                   *(pcdiis->pnValue),
                   GpcszIniFile,
                   GpcszIniSection));
   }

   return(TRUE);
}


/*
** SetUnsignedDecimalIntIniSwitch()
**
** 
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL SetUnsignedDecimalIntIniSwitch(PCUNSDECINTINISWITCH pcudiis)
{
   INT nNewValue;

   ASSERT(IS_VALID_STRUCT_PTR(pcudiis, CUNSDECINTINISWITCH));

   /* Get unsigned decimal integer .ini switch as signed decimal integer. */

   ASSERT(*(pcudiis->puValue) <= INT_MAX);

   nNewValue = GetPrivateProfileInt(GpcszIniSection, pcudiis->pcszKeyName,
                                    *(pcudiis->puValue), GpcszIniFile);

   if (nNewValue >= 0)
   {
      if ((UINT)nNewValue != *(pcudiis->puValue))
      {
         /* New non-negative value. */

         *(pcudiis->puValue) = nNewValue;

         WARNING_OUT((TEXT("SetUnsignedDecimalIntIniSwitch(): %s set to %u in %s![%s]."),
                      pcudiis->pcszKeyName,
                      *(pcudiis->puValue),
                      GpcszIniFile,
                      GpcszIniSection));
      }
   }
   else
      /* Negative value. */
      WARNING_OUT((TEXT("SetUnsignedDecimalIntIniSwitch(): Unsigned value %s set to %d in %s![%s].  Ignored."),
                   pcudiis->pcszKeyName,
                   nNewValue,
                   GpcszIniFile,
                   GpcszIniSection));

   return(TRUE);
}


/*
** SetIniSwitch()
**
** 
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL SetIniSwitch(PCVOID pcvIniSwitch)
{
   BOOL bResult;

   ASSERT(IS_VALID_READ_PTR((PCINISWITCHTYPE)pcvIniSwitch, CINISWITCHTYPE));

   /* Set .ini switch based upon type. */

   switch (*(PCINISWITCHTYPE)pcvIniSwitch)
   {
      case IST_BOOL:
         bResult = SetBOOLIniSwitch(pcvIniSwitch);
         break;

      case IST_DEC_INT:
         bResult = SetDecimalIntIniSwitch(pcvIniSwitch);
         break;

      case IST_UNS_DEC_INT:
         bResult = SetUnsignedDecimalIntIniSwitch(pcvIniSwitch);
         break;

      default:
         ERROR_OUT((TEXT("SetIniSwitch(): Unrecognized .ini switch type %d."),
                    *(PCINISWITCHTYPE)pcvIniSwitch));
         bResult = FALSE;
         break;
   }

   return(bResult);
}


/*
** IsYesString()
**
** 
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsYesString(LPCTSTR pcsz)
{
   ASSERT(IS_VALID_STRING_PTR(pcsz, CSTR));

   return(IsStringInList(pcsz, MrgcpcszTrue, ARRAY_ELEMENTS(MrgcpcszTrue)));
}


/*
** IsNoString()
**
** 
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsNoString(LPCTSTR pcsz)
{
   ASSERT(IS_VALID_STRING_PTR(pcsz, CSTR));

   return(IsStringInList(pcsz, MrgcpcszFalse, ARRAY_ELEMENTS(MrgcpcszFalse)));
}


/*
** IsStringInList()
**
** Determines whether or not a given string matches a string in a list of
** strings.
**
** Arguments:     pcsz - pointer to string to be checked
**
** Returns:       
**
** Side Effects:  none
**
** N.b., string comparison is case-insensitive.
*/
PRIVATE_CODE BOOL IsStringInList(LPCTSTR pcsz, const LPCTSTR *pcpcszList,
                            UINT ucbStrings)
{
   UINT u;
   BOOL bFound = FALSE;

   ASSERT(IS_VALID_STRING_PTR(pcsz, CSTR));
   ASSERT(IS_VALID_READ_BUFFER_PTR(pcpcszList, LPCTSTR, ucbStrings * sizeof(*pcpcszList)));

   /* Search the list for the given string. */

   for (u = 0; u < ucbStrings; u++)
   {
      ASSERT(IS_VALID_STRING_PTR(pcpcszList[u], CSTR));

      if (! lstrcmpi(pcsz, pcpcszList[u]))
      {
         bFound = TRUE;
         break;
      }
   }

   return(bFound);
}


/*
** IsValidPCBOOLINIKEY()
**
** 
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCBOOLINISWITCH(PCBOOLINISWITCH pcbis)
{
   return(IS_VALID_READ_PTR(pcbis, CBOOLINISWITCH) &&
          EVAL(pcbis->istype == IST_BOOL) &&
          IS_VALID_STRING_PTR(pcbis->pcszKeyName, CSTR) &&
          IS_VALID_WRITE_PTR(pcbis->pdwParentFlags, DWORD) &&
          EVAL(pcbis->dwFlag));
}


/*
** IsValidPCDECINTINISWITCH()
**
** 
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCDECINTINISWITCH(PCDECINTINISWITCH pcdiis)
{
   return(IS_VALID_READ_PTR(pcdiis, CDECINTINISWITCH) &&
          EVAL(pcdiis->istype == IST_DEC_INT) &&
          IS_VALID_STRING_PTR(pcdiis->pcszKeyName, CSTR) &&
          IS_VALID_WRITE_PTR(pcdiis->pnValue, INT));
}


/*
** IsValidPCUNSDECINTINISWITCH()
**
** 
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCUNSDECINTINISWITCH(PCUNSDECINTINISWITCH pcudiis)
{
   return(IS_VALID_READ_PTR(pcudiis, CUNSDECINTINISWITCH) &&
          EVAL(pcudiis->istype == IST_UNS_DEC_INT) &&
          IS_VALID_STRING_PTR(pcudiis->pcszKeyName, CSTR) &&
          IS_VALID_WRITE_PTR(pcudiis->puValue, UINT));
}

#endif


/****************************** Public Functions *****************************/


#ifdef DEBUG

/*
** SetIniSwitches()
**
** Set flags from initialization file.
**
** Arguments:     ppcvIniSwitches - pointer to array of pointers to .ini switch
**                                  structures describing .ini switches to set
**                ucSwitches - number of .ini switch pointers in
**                             ppcvIniSwitches array
**
** Returns:       TRUE if .ini switch processing is successful.  FALSE if not.
**
** Side Effects:  none
**
** N.b, the global variables GpcszIniFile and GpcszIniSection must be filled in
** before calling SetIniSwitches().
*/
PUBLIC_CODE BOOL SetIniSwitches(const PCVOID *pcpcvIniSwitches, UINT ucSwitches)
{
   BOOL bResult = TRUE;
   UINT u;

   ASSERT(IS_VALID_READ_BUFFER_PTR(pcpcvIniSwitches, const PCVOID, ucSwitches * sizeof(*pcpcvIniSwitches)));

   /* Process .ini switches. */

   for (u = 0; u < ucSwitches; u++)
      bResult = SetIniSwitch(pcpcvIniSwitches[u]) && bResult;

   return(bResult);
}

#endif

