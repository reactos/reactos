#include	"mktyplib.h"

#include	<malloc.h>
#include	<stdio.h>

#ifndef WIN32
#include	<ole2.h>
#include	"dispatch.h"
#endif //!WIN32

extern "C" {
#include	"tlviewer.hxx"

extern VOID FAR DisplayLine (CHAR * lpszMsg) ;

WORD fInParams = FALSE;
char szSilent[50];

VOID FAR DumpTypeLib (LPSTR szTypeLib)
{
	 osStrCpy ( (LPSTR)&szDumpInputFile, szTypeLib);
	 osStrCpy ( (LPSTR)&szDumpOutputFile, defaultOutput ) ;
	 osStrCpy ( (LPSTR)&szSilent, "" ) ;
	 ProcessInput () ;		     // read input file
}


VOID NEAR ProcessInput(VOID)
  {
      HRESULT	   hRes ;		     // return code

      hRes = NOERROR;		     // no ole initialization
      if ( !hRes )
	{				     // load the file
	   hRes = LoadTypeLib((LPSTR) szDumpInputFile, &ptLib) ;
	   OutToFile (hRes) ;		     // print result to the
					     // output file
	   //OleUninitialize ();

	   if ( osStrCmp((LPSTR)szSilent, (LPSTR)"silent") != 0 )
	       osMessage ((LPSTR)"Output is created successfully", (LPSTR)"Tlviewer") ;
	}
      else
	   osMessage ((LPSTR)"OleInitialize fails", (LPSTR)"Tlviewer") ;
   }

VOID NEAR OutToFile(HRESULT hRes)
   {
      FILE  *hFile ;			     // file handle
      UINT  tInfoCount ;		     // total number of type info
      UINT  i ;
      char  szTmp[fMaxBuffer] ;

#ifdef WIN16
      // convert szDumpOutputFile in-place to OEM char set
      AnsiToOem(szDumpOutputFile, szDumpOutputFile);
#endif // WIN16

      hFile = fopen(szDumpOutputFile, fnWrite);  // open output file

#ifdef WIN16
      // convert back to ANSI
      OemToAnsi(szDumpOutputFile, szDumpOutputFile);
#endif // WIN16

      if (hFile == NULL)
	{
	   osStrCpy((LPSTR)&szTmp, "Fail to open input file") ;
	   osStrCat((LPSTR)&szTmp, (LPSTR)szDumpOutputFile) ;
	   osMessage ((LPSTR)szTmp, (LPSTR)"Tlviewer") ;
	}
      else
	{
	 WriteOut(hFile, (LPSTR)szFileHeader) ;  // output file header
	 WriteOut(hFile, (LPSTR)szDumpInputFile) ;
	 WriteOut(hFile, (LPSTR)szEndStr) ;

	 if ( LOWORD(hRes) )		     // if it is not a valid type ****
	    WriteOut(hFile, szInputInvalid) ;// library
	 else
	   {
	     if ( fOutLibrary(hFile) )
	       {
		 tInfoCount = ptLib->GetTypeInfoCount() ;
		 for (i = 0 ; i < tInfoCount ; i++)
		   {
		      if ( LOWORD(ptLib->GetTypeInfo(i, &ptInfo)) )
			{
			   WriteOut(hFile, (LPSTR)szReadFail) ;
			   WriteOut(hFile, (LPSTR)"type info\n\n") ;
			}
		      else
			{
			  if ( LOWORD(ptInfo->GetTypeAttr(&lpTypeAttr)) )
			    {
			      WriteOut(hFile, (LPSTR)szReadFail) ;
			      WriteOut(hFile, (LPSTR)"attributes of type info\n\n") ;
			    }
			  else
			    {
			      switch (lpTypeAttr->typekind)
				{
				  case TKIND_ENUM:
				    tOutEnum(hFile, i) ;
				    break ;

				  case TKIND_RECORD:
				    tOutRecord(hFile, i) ;
				    break ;

				  case TKIND_MODULE:
				    tOutModule(hFile, i) ;
				    break ;

				  case TKIND_INTERFACE:
				    tOutInterface(hFile, i) ;
				    break ;

				  case TKIND_DISPATCH:
				    tOutDispatch(hFile, i) ;
				    break ;

				  case TKIND_COCLASS:
				    tOutCoclass(hFile, i) ;
				    break ;

				  case TKIND_ALIAS:
				    tOutAlias(hFile, i) ;
				    break ;

				  case TKIND_UNION:
				    tOutUnion(hFile, i) ;
				    break ;

				  default:
				    WriteOut(hFile, (LPSTR) "Type of definition is unknown") ;
				}	     // switch
			       ptInfo->ReleaseTypeAttr(lpTypeAttr) ;
			    }		     // if gettypeattr
			  ptInfo->Release() ;// release the current TypeInfo
			}		     // if gettypeinfo
		   }			     // for i
		 WriteOut(hFile, (LPSTR)"}\n") ; // output the closing }
					     // if fOutLibrary
		 ptLib->Release();	     // clean up before exit
	       }
	    }

	 fclose(hFile);			     // finish writing to the output
	 hFile = NULL;			     // close done

	}
  }


BOOL NEAR fOutLibrary(FILE *hFile)
  {
      TLIBATTR FAR *lpLibAttr ;		     // attributes of the library
      char     szTmp[16] ;
      BOOL     retval = FALSE ;

      if ( LOWORD( ptLib->GetLibAttr(&lpLibAttr) ) )	// *****

	{
	   WriteOut(hFile, (LPSTR)szReadFail) ;
	   WriteOut(hFile, (LPSTR)"attributes of library\n\n") ;
	}
      else
	{				     // output documentational
	   tOutAttr(hFile, -1) ;	     // attributes first
					     // output id-related attributes
	   _ltoa((long)lpLibAttr->lcid, szTmp, 10) ; // output lcid;
	   WriteAttr(hFile, attrLcid, (LPSTR)szTmp, numValue) ; // default is 0
	   GetVerNumber (lpLibAttr->wMajorVerNum, lpLibAttr->wMinorVerNum, szTmp) ;
	   WriteAttr(hFile, attrVer, (LPSTR)szTmp, numValue) ; // output version
	   tOutUUID(hFile, lpLibAttr->guid) ;
	   if ( endAttrFlag )
	     {
	       WriteOut(hFile, fInParams ? szEndAttr : szEndAttrNl) ;
	       endAttrFlag = FALSE ;
	     }

	   ptLib->ReleaseTLibAttr(lpLibAttr) ;	// de-allocate attribute

	   WriteOut(hFile, (LPSTR)"\nlibrary ") ;
	   tOutName(hFile, -1) ;		// output name of library
	   WriteOut(hFile, (LPSTR)"{\n\n") ;
	   retval = TRUE ;
	}					// if GetLibAttributes
      return (retval) ; 			// before exit
  }

VOID NEAR tOutEnum (FILE *hFile, UINT iTypeId)
   {
      WriteOut(hFile, (LPSTR) "\ntypedef\n");// output typedef first
      tOutAttr(hFile, (int)iTypeId) ;	     // output attribute
      tOutMoreAttr(hFile) ;
      WriteOut(hFile, (LPSTR) "\nenum {\n") ;
      tOutVar(hFile) ;			     // output enum members

      WriteOut(hFile, (LPSTR) "} ") ;	     // close the definition and
      tOutName(hFile, iTypeId) ;	     // output name of the enum type
      WriteOut(hFile, (LPSTR) " ;\n\n") ;
    }

VOID NEAR tOutRecord (FILE *hFile, UINT iTypeId)
   {
      WriteOut(hFile, (LPSTR) "\ntypedef\n");// output typedef first
      tOutAttr(hFile, (int)iTypeId) ;	     // output attribute
      tOutMoreAttr(hFile) ;
      WriteOut(hFile, (LPSTR) "\nstruct {\n") ;
      tOutVar (hFile) ; 		     // output members

      WriteOut(hFile, (LPSTR) "} ") ;
      tOutName(hFile, iTypeId) ;
      WriteOut(hFile, (LPSTR) " ;\n\n") ;
   }

VOID  NEAR tOutModule	(FILE *hFile, UINT iTypeId)
   {
      tOutAttr(hFile, (int)iTypeId) ;	     // output attribute first
      tOutMoreAttr(hFile) ;
      WriteOut(hFile, (LPSTR) "\nmodule ") ;
      tOutName(hFile, iTypeId) ;
      WriteOut(hFile, " {\n") ;

      tOutVar (hFile) ; 		     // output each const

      tOutFunc (hFile) ;		     // output each member function
      WriteOut(hFile, (LPSTR) "}\n\n") ;
    }

VOID  NEAR tOutInterface(FILE *hFile, UINT iTypeId)
   {
      HREFTYPE	phRefType ;

      tOutAttr(hFile, (int)iTypeId) ;	     // output attribute first
      tOutMoreAttr(hFile) ;
      WriteOut(hFile, (LPSTR) "\ninterface ") ;
      tOutName(hFile, iTypeId) ;
					     // find out if the interface
      if ( !LOWORD(ptInfo->GetRefTypeOfImplType(0, &phRefType)) )
	 {
	   isInherit = TRUE ;
	   tOutAliasName(hFile, phRefType) ; // is inherited from some other
	   isInherit = FALSE ;		     // interface
	 }
      WriteOut(hFile, " {\n") ;

      tOutFunc (hFile) ;		     // output each member function
      WriteOut(hFile, (LPSTR) "}\n\n") ;
    }

VOID  NEAR tOutDispatch	(FILE *hFile, UINT iTypeId)
   {
      HREFTYPE	phRefType ;

      tOutAttr(hFile, (int)iTypeId) ;	    // output attribute first
      tOutMoreAttr(hFile) ;
      WriteOut(hFile, (LPSTR) "\ndispinterface ") ;
      tOutName(hFile, iTypeId) ;

      if ( !LOWORD(ptInfo->GetRefTypeOfImplType(0, &phRefType)) )
	 {
	   isInherit = TRUE ;
	   tOutAliasName(hFile, phRefType) ; // is inherited from some other
	   isInherit = FALSE ;		     // interface
	 }
      WriteOut(hFile, (LPSTR) " {\n") ;

      WriteOut(hFile, (LPSTR) "\nproperties:\n") ;
      tOutVar (hFile) ; 		    // output each date member

      WriteOut(hFile, (LPSTR) "\nmethods:\n") ;
      tOutFunc (hFile) ;		    // output each member function

      WriteOut(hFile, (LPSTR) "}\n\n") ;
    }

VOID  NEAR tOutCoclass	(FILE *hFile, UINT iTypeId)
   {

      HREFTYPE	phRefType ;
      WORD	i ;

      tOutAttr(hFile, (int)iTypeId) ;	    // output attribute first
					    // output appobject attribute if
      if ( lpTypeAttr->wTypeFlags == TYPEFLAG_FAPPOBJECT )  // it is set
	   WriteAttr(hFile, attrAppObj, NULL, noValue) ;
      tOutMoreAttr(hFile) ;

      WriteOut(hFile, (LPSTR) "\ncoclass ") ;
      tOutName(hFile, iTypeId) ;
      WriteOut(hFile, (LPSTR) " {\n") ;

	for ( i = 0 ; i < lpTypeAttr->cImplTypes; i++ )
	{
	  if ( LOWORD(ptInfo->GetRefTypeOfImplType(i, &phRefType)) )
	    {
	       WriteOut(hFile, szReadFail) ;
	       WriteOut(hFile, "GetRefTypeOfImpType\n") ;
	     }
	  else
	      tOutAliasName(hFile, phRefType) ;
	}

      WriteOut(hFile, (LPSTR) "}\n\n") ;
    }

VOID  NEAR tOutAlias	(FILE *hFile, UINT iTypeId)
   {
      char     szTmp[16] ;

      WriteOut(hFile, (LPSTR) "\ntypedef ") ;
      tOutAttr(hFile, (int)iTypeId) ;	    // output attribute first
      WriteAttr(hFile, attrPublic, (LPSTR)szTmp, noValue) ; // public attr
      tOutMoreAttr(hFile) ;

      tOutType(hFile, lpTypeAttr->tdescAlias) ;  // output name of base-type

      tOutName(hFile, iTypeId) ;		 // output name of new type
      WriteOut(hFile, (LPSTR) ";\n\n") ;
    }

VOID NEAR tOutUnion (FILE *hFile, UINT iTypeId)
   {
      WriteOut(hFile, (LPSTR) "\ntypedef\n"); // output typedef first
      tOutAttr(hFile, (int)iTypeId) ;	    // output attribute
      tOutMoreAttr(hFile) ;
      WriteOut(hFile, (LPSTR) "\nunion {\n") ;
      tOutVar (hFile) ; 		    // output members

      WriteOut(hFile, (LPSTR) "} ") ;
      tOutName(hFile, iTypeId) ;
      WriteOut(hFile, (LPSTR) " ;\n\n") ;
   }


VOID NEAR tOutEncunion (FILE *hFile, UINT iTypeId)
   {
      WriteOut(hFile, (LPSTR) "\ntypedef\n"); // output typedef first
      tOutAttr(hFile, (int)iTypeId) ;	    // output attribute
      tOutMoreAttr(hFile) ;
      WriteOut(hFile, (LPSTR) "\nencunion {\n") ;
      tOutVar (hFile) ; 		    // output members

      WriteOut(hFile, (LPSTR) "} ") ;
      tOutName(hFile, iTypeId) ;
      WriteOut(hFile, (LPSTR) " ;\n\n") ;
   }


VOID NEAR tOutName (FILE *hFile, UINT iTypeId)
   {
      BSTR bstrName ;

      if ( LOWORD(ptLib->GetDocumentation(iTypeId, &bstrName, NULL, NULL, NULL)) )
	{
	   WriteOut(hFile, szReadFail) ;
	   WriteOut(hFile, "name of type definition") ;
	}
      else
	{
	   WriteOut(hFile, (LPSTR) bstrName) ;
	   WriteOut(hFile, (LPSTR) " ") ;

	   if ( iTypeId == -1 ) 	    // record name of the library
	     osStrCpy((LPSTR)&szLibName, (LPSTR)bstrName) ;

	   SysFreeString(bstrName) ;
	}
   }

VOID NEAR tOutType (FILE *hFile, TYPEDESC tdesc)
   {
      char	szTmp[40] ;

      switch (tdesc.vt)
	{
	  case VT_EMPTY:
	    osStrCpy ( (LPSTR)&szTmp, (LPSTR) "notSpec " ) ;
	    break ;
	  case VT_NULL:
	    osStrCpy ( (LPSTR)&szTmp, (LPSTR) "NULL " ) ;
	    break ;
	  case VT_I2:
	    osStrCpy ( (LPSTR)&szTmp, (LPSTR) "short " ) ;
	    break ;
	  case VT_I4:
	    osStrCpy ( (LPSTR)&szTmp, (LPSTR) "long " ) ;
	    break ;
	  case VT_R4:
	    osStrCpy ( (LPSTR)&szTmp, (LPSTR) "float " ) ;
	    break ;
	  case VT_R8:
	    osStrCpy ( (LPSTR)&szTmp, (LPSTR) "double " ) ;
	    break ;
	  case VT_CY:
	    osStrCpy ( (LPSTR)&szTmp, (LPSTR) "CURRENCY " ) ;
	    break ;
	  case VT_DATE:
	    osStrCpy ( (LPSTR)&szTmp, (LPSTR) "DATE " ) ;
	    break ;
	  case VT_BSTR:
	    osStrCpy ( (LPSTR)&szTmp, (LPSTR) "BSTR " ) ;
	    break ;
	  case VT_DISPATCH:
	    osStrCpy ( (LPSTR)&szTmp, (LPSTR) "IDispatch * (VT_DISPATCH) " ) ;
	    break ;
	  case VT_ERROR:
	    osStrCpy ( (LPSTR)&szTmp, (LPSTR) "scode " ) ;
	    break ;
	  case VT_BOOL:
	    osStrCpy ( (LPSTR)&szTmp, (LPSTR) "boolean " ) ;
	    break ;
	  case VT_VARIANT:
	    osStrCpy ( (LPSTR)&szTmp, (LPSTR) "VARIANT " ) ;
	    break ;
	  case VT_UNKNOWN:
	    osStrCpy ( (LPSTR)&szTmp, (LPSTR) "IUnknown * (VT_UNKNOWN) " ) ;
	    break ;
	  case VT_I1:
	    osStrCpy ( (LPSTR)&szTmp, (LPSTR) "char " ) ;
	    break ;
	  case VT_UI1:
	    osStrCpy ( (LPSTR)&szTmp, (LPSTR) "unsigned char " ) ;
	    break ;
	  case VT_UI2:
	    osStrCpy ( (LPSTR)&szTmp, (LPSTR) "unsigned short " ) ;
	    break ;
	  case VT_UI4:
	    osStrCpy ( (LPSTR)&szTmp, (LPSTR) "unsigned long " ) ;
	    break ;
	  case VT_I8:
	    osStrCpy ( (LPSTR)&szTmp, (LPSTR) "long long " ) ;
	    break ;
	  case VT_UI8:
	    osStrCpy ( (LPSTR)&szTmp, (LPSTR) "unsigned long long " ) ;
	    break ;
	  case VT_INT:
	    osStrCpy ( (LPSTR)&szTmp, (LPSTR) "int " ) ;
	    break ;
	  case VT_UINT:
	    osStrCpy ( (LPSTR)&szTmp, (LPSTR) "unsigned int " ) ;
	    break ;
	  case VT_VOID:
	    osStrCpy ( (LPSTR)&szTmp, (LPSTR) "void " ) ;
	    break ;
	  case VT_HRESULT:
	    osStrCpy ( (LPSTR)&szTmp, (LPSTR) "HRESULT " ) ;
	    break ;
	  case VT_PTR:
	    tOutType (hFile, *(tdesc.lptdesc)) ;
	    osStrCpy ( (LPSTR)&szTmp, (LPSTR) "* " ) ;
	    break ;
	  case VT_SAFEARRAY:
	    if ( endAttrFlag )
	      {
		WriteOut(hFile, fInParams ? szEndAttr : szEndAttrNl) ;
		endAttrFlag = FALSE ;
	      }
	    WriteOut(hFile, (LPSTR)"SAFEARRAY ( ") ;
	    tOutType (hFile, *(tdesc.lptdesc)) ;
	    break ;
	  case VT_CARRAY:
	    cArrFlag = (SHORT)tdesc.lpadesc->cDims ;  // get dimemsion of array
	    tOutType (hFile, tdesc.lpadesc->tdescElem) ;
	    break ;
	  case VT_USERDEFINED:
	    if ( endAttrFlag )
	      {
		WriteOut(hFile, fInParams ? szEndAttr : szEndAttrNl) ;
		endAttrFlag = FALSE ;
	      }
	    tOutAliasName (hFile, tdesc.hreftype) ;
	    break ;			    // output name of the user-defined type
	  case VT_LPSTR:
	    osStrCpy ( (LPSTR)&szTmp, (LPSTR) "LPSTR " ) ;
	    break ;
	  default:
	    osStrCpy ( (LPSTR)&szTmp, (LPSTR) "unknown type " ) ;
	}

      if ( endAttrFlag )
	{
	  WriteOut(hFile, fInParams ? szEndAttr : szEndAttrNl) ;
	  endAttrFlag = FALSE ;
	}

      if ( tdesc.vt != VT_CARRAY && tdesc.vt != VT_USERDEFINED && tdesc.vt != VT_SAFEARRAY )
	WriteOut(hFile, (LPSTR)szTmp) ;

      if ( tdesc.vt == VT_SAFEARRAY )
	WriteOut(hFile, ") ") ;

   }

VOID  NEAR tOutCDim (FILE *hFile, TYPEDESC tdesc)
   {
      USHORT i ;
      ULONG  l ;
      char   szTmp[16] ;

      for ( i = 0 ; i < cArrFlag ; i++ )
	 {
	   l = tdesc.lpadesc->rgbounds[i].cElements ;
	   _ltoa(l, szTmp, 10) ;
	   WriteOut(hFile, (LPSTR)"[") ;
	   WriteOut(hFile, (LPSTR)szTmp) ;
	   WriteOut(hFile, (LPSTR)"]") ;
	 }

      cArrFlag = -1 ;
   }

VOID NEAR tOutAliasName (FILE *hFile, HREFTYPE phRefType)
   {
      ITypeInfo FAR *lpInfo ;		    // pointer to the type definition
      ITypeLib	FAR *lpLib ;		    // pointer to a type library
      TYPEATTR	FAR *lptAttr ;
      BSTR	bstrName ;
      UINT	iTypeId ;

      if ( LOWORD(ptInfo->GetRefTypeInfo(phRefType, &lpInfo)) )
	{				    // get TypeInfo of the alias
	  WriteOut(hFile, szReadFail) ;
	  WriteOut(hFile, "GetRefTypeInfo\n") ;
	}
      else
	{
	  if ( lpTypeAttr->typekind == TKIND_COCLASS )
	    {
	      if ( LOWORD(lpInfo->GetTypeAttr(&lptAttr)) )
		{
		  WriteOut(hFile, (LPSTR)szReadFail) ;
		  WriteOut(hFile, (LPSTR)"attribute of reftype\n\n") ;
		}
	      else
		{
		   if ( lptAttr->typekind == TKIND_INTERFACE )
		      WriteOut(hFile, (LPSTR)"interface ") ;
		   else
		      if ( lptAttr->typekind == TKIND_DISPATCH )
			WriteOut(hFile, (LPSTR)"dispinterface ") ;

		   lpInfo->ReleaseTypeAttr(lptAttr) ;
		 }
	    }
	  else				    // output name of base-interface
	    if ( isInherit &&
		(lpTypeAttr->typekind == TKIND_INTERFACE ||
		 lpTypeAttr->typekind == TKIND_DISPATCH) )
	       WriteOut(hFile, (LPSTR)" : ") ;


	  if ( LOWORD(lpInfo->GetContainingTypeLib(&lpLib, &iTypeId)) )
	    {				    // get id of the alias
	      WriteOut(hFile, (LPSTR)szReadFail) ;
	      WriteOut(hFile, (LPSTR)"GetAlias: containing typelib\n\n") ;
	    }
	  else
	    {				    // check origin of the alias
	      if ( LOWORD(lpLib->GetDocumentation(-1, &bstrName, NULL, NULL, NULL)) )
		{
		  WriteOut(hFile, szReadFail) ;
		  WriteOut(hFile, "name of import library") ;
		}
	      else
		{			    // if it is not defined locally
		  if ( osStrCmp((LPSTR)szLibName, (LPSTR)bstrName) != 0 )
		    {			    // i.e. name of origin is diff
		       WriteOut(hFile, (LPSTR) bstrName) ;
		       WriteOut(hFile, (LPSTR)".") ;
		    }			    // from the name of library;
					    // output its origin
		  SysFreeString(bstrName) ;
		}

	      if ( LOWORD(lpLib->GetDocumentation(iTypeId, &bstrName, NULL, NULL, NULL)) )
		{			    // retrieve name of the alias
		  WriteOut(hFile, szReadFail) ;
		  WriteOut(hFile, "name of alias") ;
		}
	      else
		{
		  WriteOut(hFile, (LPSTR)bstrName) ;

		  if ( lpTypeAttr->typekind == TKIND_COCLASS )
		     WriteOut(hFile, (LPSTR)" ;\n") ;
		  else
		     WriteOut(hFile, (LPSTR)" ") ;

		  SysFreeString(bstrName) ;
		}
	    }
	}
   }

VOID  NEAR tOutValue(FILE *hFile, BSTR bstrName, VARDESC FAR *lpVarDesc)
  {
       VARTYPE vvt ;
       char    szTmp[16] ;

       if ( endAttrFlag )
	 {
	    WriteOut(hFile, fInParams ? szEndAttr : szEndAttrNl) ;
	    endAttrFlag = FALSE ;
	 }

       if ( lpTypeAttr->typekind == TKIND_MODULE )
	{
	  WriteOut(hFile, (LPSTR)"const ") ; // output the const keyword
	  tOutType(hFile, lpVarDesc->elemdescVar.tdesc) ; // output its type
	}

       WriteOut(hFile, (LPSTR)bstrName) ; // output name of member
       WriteOut(hFile, (LPSTR)" = ") ;

       vvt = lpVarDesc->lpvarValue->vt ;
       switch ( vvt )
	{
	  case VT_I2:
	  case VT_BOOL:
	    _itoa((int)lpVarDesc->lpvarValue->iVal, szTmp, 10) ;
	    break ;
	  default:
	    _ltoa((long)lpVarDesc->lpvarValue->lVal, szTmp, 10) ;
	    break ;
	 }
       WriteOut(hFile, (LPSTR)szTmp) ;	// output value of member

       if ( lpTypeAttr->typekind == TKIND_MODULE )
	 WriteOut(hFile, (LPSTR)" ;\n") ;
       else
	 WriteOut(hFile, (LPSTR)" ,\n") ;
}


VOID  NEAR tOutMember(FILE *hFile, LONG idMember, BSTR bstrName, TYPEDESC tdesc)
  {
       char szTmp[16] ;

#if 0	// UNDONE: TEMPORARY -- id's for everybody
       if ( lpTypeAttr->typekind == TKIND_DISPATCH )
#endif
	 {
	    szTmp[0] = '0';
	    szTmp[1] = 'x';
	   _ultoa(idMember, szTmp+2, 16) ;	    // output id in hex
	   WriteAttr(hFile, attrId, (LPSTR)szTmp, numValue) ;
	 }
					    // output name of base-type
       tOutType(hFile, tdesc) ;
       WriteOut(hFile, (LPSTR)bstrName) ;  // output name of member
       if ( cArrFlag != -1 )		   // it is a c-array; output
	 tOutCDim (hFile, tdesc) ;
					   // dimensions of the array
       WriteOut(hFile, (LPSTR)" ;\n") ;
  }

VOID  NEAR tOutVar(FILE *hFile)
   {
      VARDESC  FAR *ptVarDesc ;
      BSTR     bstrName ;		    // name of member
      BSTR     bstrDoc	;		    // file string
      DWORD    hContext ;		    // help context
      char     szTmp[16] ;
      WORD     i ;
      LONG     idMember ;
      BSTR     rgNames[MAX_NAMES];
      UINT     cNames, j ;

	for (i = 0 ; i < lpTypeAttr->cVars; i++) // for every member
	{
	   if ( LOWORD(ptInfo->GetVarDesc(i, &ptVarDesc)) )
	     {
		WriteOut(hFile, szReadFail) ;
		WriteOut(hFile, "variables\n") ;
	     }
	   else
	     {
		idMember = ptVarDesc->memid ;
						  // this is readonly var
		if ( ptVarDesc->wVarFlags == VARFLAG_FREADONLY )
		   WriteAttr(hFile, attrReadonly, NULL, noValue) ;

		if ( LOWORD(ptInfo->GetDocumentation(idMember, &bstrName, &bstrDoc, &hContext, NULL)) )
		  {
		     WriteOut(hFile, szReadFail) ;
		     WriteOut(hFile, "attributes of variable\n") ;
		  }
		else
		  {
		     _ltoa((long)hContext, szTmp, 10) ;// output helpcontext; default is 0
		     WriteAttr(hFile, attrHelpCont, (LPSTR)szTmp, numValue) ;

		     if ( bstrDoc != NULL )	  // output helpstring if exists
		       WriteAttr(hFile, attrHelpStr, (LPSTR)bstrDoc, strValue) ;
						  // typedef enum or const in module
		     if ( lpTypeAttr->typekind == TKIND_ENUM || lpTypeAttr->typekind == TKIND_MODULE )
		       tOutValue (hFile, bstrName, ptVarDesc) ;
		     else			  // typedef struct or dispinterface
		       tOutMember (hFile, idMember, bstrName, ptVarDesc->elemdescVar.tdesc) ;

		     SysFreeString(bstrDoc) ;	   // release local bstr
						   // also checking the name
		     if ( LOWORD(ptInfo->GetNames(idMember, rgNames, MAX_NAMES, &cNames)) )
		       {			   // with GetNames
			 WriteOut(hFile, szReadFail) ;
			 WriteOut(hFile, "name of variable\n") ;
		       }
		     else
		       {
			 if ( cNames != 1 )
			   {
			     WriteOut(hFile, szReadFail) ;
			     WriteOut(hFile, "GetNames return more than one name\n") ;
			   }
			 else
			   {
			     if ( osStrCmp((LPSTR)rgNames[0], (LPSTR)bstrName) != 0 )
			       {
				 WriteOut(hFile, szReadFail) ;
				 WriteOut(hFile, "name of variable inconsistent\n") ;
			       }
			   }

			 for ( j = 0 ; j < cNames ; j++ )
			   SysFreeString(rgNames[j]) ;
		       }

		     SysFreeString(bstrName) ;
		  }
	     }
	   ptInfo->ReleaseVarDesc(ptVarDesc) ;
	   ptVarDesc = NULL ;
	}				   // for i
   }


VOID  NEAR tOutFuncAttr(FILE *hFile, FUNCDESC FAR *lpFuncDesc, DWORD hContext, BSTR bstrDoc)
  {
      char szTmp[16] ;

      _ltoa((long)hContext, szTmp, 10) ;   // output helpcontext; default is 0
      WriteAttr(hFile, attrHelpCont, (LPSTR)szTmp, numValue) ;

      if ( bstrDoc != NULL )		   // output helpstring if exists
	WriteAttr(hFile, attrHelpStr, (LPSTR)bstrDoc, strValue) ;
					   // output restricted attribute
      if ( lpFuncDesc->wFuncFlags == FUNCFLAG_FRESTRICTED )
	WriteAttr(hFile, attrRestrict, NULL, noValue) ;
					   // last parm is optional array
      if ( lpFuncDesc->cParamsOpt == -1 )  // of Variants
	WriteAttr(hFile, attrVar, NULL, noValue) ;

#if 0	// UNDONE: TEMPORARY -- id's for everybody
      if ( lpTypeAttr->typekind == TKIND_DISPATCH || lpTypeAttr->typekind == TKIND_INTERFACE)
#endif
	 {
	    szTmp[0] = '0';
	    szTmp[1] = 'x';
	    _ultoa(lpFuncDesc->memid, szTmp+2, 16) ;	 // output id in hex
	    WriteAttr(hFile, attrId, (LPSTR)szTmp, numValue) ;
	 }
#if 0
      else					 // DISPID designates the
	if ( lpFuncDesc->memid == DISPID_VALUE ) // default function
	  WriteAttr(hFile, attrDefault, NULL, noValue) ;
#endif //0

      switch ( lpFuncDesc->invkind )	   // Note: if one of these
	{				   // flag is set, name of
	  case INVOKE_FUNC:		   // parm can't be set: i.e.
//	     WriteAttr(hFile, (LPSTR)"invoke_func", NULL, noValue) ;
	     break ;			   // GetNames only returns name
	  case INVOKE_PROPERTYGET:	   // of the function
	     WriteAttr(hFile, attrPropget, NULL, noValue) ;
	     break ;
	  case INVOKE_PROPERTYPUT:
	     WriteAttr(hFile, attrPropput, NULL, noValue) ;
	     break ;
	  case INVOKE_PROPERTYPUTREF:
	     WriteAttr(hFile, attrProppr, NULL, noValue) ;
	     break ;
	  default:
	     WriteAttr(hFile, (LPSTR)"unknown invkind", NULL, noValue) ;
	}
   }

VOID  NEAR tOutCallConv(FILE *hFile, FUNCDESC FAR *lpFuncDesc)
   {
      switch ( lpFuncDesc->callconv )
	{
	  case CC_MSCPASCAL:
	     WriteOut(hFile, (LPSTR)"__pascal ") ;
	     break ;
	  case CC_MACPASCAL:
	     WriteOut(hFile, (LPSTR)"pascal ") ;
	     break ;
	  case CC_STDCALL:
	     WriteOut(hFile, (LPSTR)"__stdcall ") ;
	     break ;
/* this has been cut
	  case CC_THISCALL:
	     WriteOut(hFile, (LPSTR)"__thiscall ") ;
	     break ;
*/
	  case CC_CDECL:
	     WriteOut(hFile, (LPSTR)"__cdecl ") ;
	     break ;
	  default:
	     WriteOut(hFile, (LPSTR)"unknown calling convention ") ;
	     break ;
	}
    }

VOID  NEAR tOutParams(FILE *hFile, FUNCDESC FAR *lpFuncDesc, BSTR bstrName)
   {
      BSTR     rgNames[MAX_NAMES];
      UINT     cNames ;
      SHORT    i ;

      fInParams = TRUE;

      WriteOut(hFile, (LPSTR)"(") ;

      if ( lpFuncDesc->cParams == 0 )
	WriteOut(hFile, (LPSTR)"void") ;
      else
	{
	  if ( LOWORD(ptInfo->GetNames(lpFuncDesc->memid, rgNames, MAX_NAMES, &cNames)) )
	    {
	      WriteOut(hFile, szReadFail) ;
	      WriteOut(hFile, "parm of func in definition\n") ;
	    }
	  else
	    {
	      if ( osStrCmp((LPSTR)rgNames[0], (LPSTR)bstrName) != 0 )
		{
		  WriteOut(hFile, szReadFail) ;
		  WriteOut(hFile, "name of function inconsistent\n") ;
		}
	      SysFreeString(rgNames[0]) ;  // release name of function

	      for (i = 1; i <= lpFuncDesc->cParams; i++)
		{
		  if ( i != 1 )
		    WriteOut(hFile, (LPSTR)", ") ;
					   // output in/out attribute
		  if ( lpFuncDesc->lprgelemdescParam[i-1].idldesc.wIDLFlags != 0 )
		    {
		      if ( ( lpFuncDesc->lprgelemdescParam[i-1].idldesc.wIDLFlags & IDLFLAG_FIN ) == IDLFLAG_FIN )
			WriteAttr(hFile, attrIn, NULL, noValue) ;

		      if ( ( lpFuncDesc->lprgelemdescParam[i-1].idldesc.wIDLFlags & IDLFLAG_FOUT ) == IDLFLAG_FOUT )
			WriteAttr(hFile, attrOut, NULL, noValue) ;
		    }
					   // check for optional parm
		  if ( ( lpFuncDesc->cParamsOpt + i ) > lpFuncDesc->cParams )
		    WriteAttr(hFile, attrOption, NULL, noValue) ;
					   // and output optional attr
					   // output name of base-type
		  tOutType(hFile, lpFuncDesc->lprgelemdescParam[i-1].tdesc) ;
		  if ( i < (SHORT) cNames )// output name of parm if its is
		    {			   // not property-accessor function
		      WriteOut(hFile, (LPSTR)rgNames[i]) ;
		      SysFreeString(rgNames[i]) ;  // release name of parm's
		    }
		  else
		    WriteOut(hFile, (LPSTR)"PseudoName") ;

		  if ( cArrFlag != -1 )	   // it is a c-array; output
		    tOutCDim (hFile, lpFuncDesc->lprgelemdescParam[i-1].tdesc) ;
					   // dimension of the array
		}			   // for i = 1
	    }				   // GetNames

	}				   // if (ptFunDesc->cParams)

      WriteOut(hFile, (LPSTR)") ;\n") ;
      fInParams = FALSE;
   }


VOID  NEAR tOutFunc(FILE *hFile)
   {
      FUNCDESC FAR *ptFuncDesc ;
      BSTR     bstrName ;		    // name of member
      BSTR     bstrDoc	;		    // file string
      DWORD    hContext ;		    // help context
      WORD     i ;
   // VOID     FAR *lpvoid ;
      char     szTmp[16] ;

      for (i = 0 ; i < lpTypeAttr->cFuncs; i++) // for every member function
	{
	   if ( LOWORD(ptInfo->GetFuncDesc(i, &ptFuncDesc)) )
	     {
		WriteOut(hFile, szReadFail) ;
		WriteOut(hFile, "function of definition\n") ;
	     }
	   else
	     {
		if ( LOWORD(ptInfo->GetDocumentation(ptFuncDesc->memid, &bstrName, &bstrDoc, &hContext, NULL)) )
		  {
		     WriteOut(hFile, szReadFail) ;
		     WriteOut(hFile, "attributes of function\n") ;
		  }
		else
		  {
	 	     BSTR bstrDllName;
	 	     BSTR bstrEntryName;
		     WORD wOrdinal;
#if 0
		     VOID FAR * lpVoid;
#endif //0

		     if(!LOWORD(ptInfo->GetDllEntry(ptFuncDesc->memid,
						ptFuncDesc->invkind,
						&bstrDllName,
						&bstrEntryName,
						&wOrdinal)))
			{

#if 0	//UNDONE: TEMPORARY
		            ptInfo->AddressOfMember(ptFuncDesc->memid,
						ptFuncDesc->invkind,
						&lpVoid);
#endif //1

			    WriteOut(hFile, szRefDll);
			    WriteOut(hFile, "DLL name = ");
			    WriteOut(hFile, (LPSTR) bstrDllName);
		     	    SysFreeString(bstrDllName) ;
 
			    WriteOut(hFile, "\nDLL entry point = ");
			    if ((LPSTR)bstrEntryName != NULL)
				{
				WriteOut(hFile, (LPSTR) bstrEntryName);
		   		SysFreeString(bstrEntryName) ;
				}
			    else
				{
				WriteOut(hFile, "ordinal ");
				_itoa(wOrdinal, szTmp, 10) ;
				WriteOut(hFile, szTmp);
				
				}
			    WriteOut(hFile, "\n");
			}
						   // output attr for function
		     tOutFuncAttr(hFile, ptFuncDesc, hContext, bstrDoc) ;
						  // output return type
		     tOutType(hFile, ptFuncDesc->elemdescFunc.tdesc) ;
						  // output calling convention
		     tOutCallConv(hFile, ptFuncDesc) ;
		     WriteOut(hFile, (LPSTR)bstrName) ; // output name of member function
		     tOutParams(hFile, ptFuncDesc, bstrName) ;
							// output parameters
		     SysFreeString(bstrDoc) ;		// release local bstr's
		     SysFreeString(bstrName) ;
		  }
	     }
	   ptInfo->ReleaseFuncDesc(ptFuncDesc) ;
	   ptFuncDesc = NULL ;
	}				   // for i
    }

VOID  NEAR tOutUUID (FILE *hFile, GUID inGuid)
   {
      char  szTmp[50] ;
      int   i ;
					    // get a string representation
					    // for the incoming Guid value
      if ( !(i = osRetrieveGuid ((LPSTR)szTmp, inGuid)) )
	 { WriteOut(hFile, szReadFail) ;
	   WriteOut(hFile, (LPSTR)"insufficient memory") ;
	 }
      else
	 {	    // string is in {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
	   szTmp[37] = '\0' ;	    // format, need to remove the {}
	   WriteAttr(hFile, attrUuid, (LPSTR)&szTmp[1], numValue) ;
	 }
   }

VOID NEAR tOutAttr (FILE *hFile, int iTypeId)
   {
      BSTR     bstrDoc	;		    // file string
      BSTR     bstrHelp ;		    // name of help file
      DWORD    hContext ;		    // help context
      char     szTmp[16] ;


     if ( LOWORD(ptLib->GetDocumentation(iTypeId, NULL, &bstrDoc, &hContext, &bstrHelp)) )
	{
	  WriteOut(hFile, (LPSTR)szReadFail) ;
	  WriteOut(hFile, (LPSTR)"documentational attribute\n\n") ;
	}
      else
	{
	  _ltoa((long)hContext, szTmp, 10) ;// output helpcontext; default is 0
	  WriteAttr(hFile, attrHelpCont, (LPSTR)szTmp, numValue) ;

	  if ( bstrDoc != NULL )	    // output helpstring if exists
	    WriteAttr(hFile, attrHelpStr, (LPSTR)bstrDoc, strValue) ;

	  if ( bstrHelp != NULL )	    // output helpfile if exists
	    WriteAttr(hFile, attrHelpFile, (LPSTR)bstrHelp, strValue) ;

	  SysFreeString(bstrDoc) ;	    // release local bstr's
	  SysFreeString(bstrHelp) ;
	}
   }

VOID NEAR tOutMoreAttr (FILE *hFile)
   {
      char szTmp[16] ;

      GetVerNumber (lpTypeAttr->wMajorVerNum, lpTypeAttr->wMinorVerNum, szTmp) ;
      WriteAttr(hFile, attrVer, (LPSTR)szTmp, numValue) ; // output version
      tOutUUID(hFile, lpTypeAttr->guid) ;
      if ( endAttrFlag )
	 {
	   WriteOut(hFile, fInParams ? szEndAttr : szEndAttrNl) ;
	   endAttrFlag = FALSE ;
	 }
   }


VOID NEAR WriteAttr(FILE *hFile, LPSTR lpszAttr, LPSTR lpszStr, int ivalType)
   {
       BOOL firstAttr = FALSE ;

       if ( !endAttrFlag )
	  {
	    if (!fInParams)
	        WriteOut(hFile, "\n") ;		// output NL for readablilty

	    WriteOut(hFile, szBeginAttr) ;	// output "[" first
	    endAttrFlag = TRUE ;
	    firstAttr = TRUE ;
	  }
						// this is not the first
       if ( !firstAttr ) {			// attribute to be written
	  if (fInParams)
	    WriteOut(hFile, (LPSTR)", ") ;	// need to put a , before
	  else
	    WriteOut(hFile, (LPSTR)",\n") ;
	}
	    
						// output name of attribute
       WriteOut(hFile, lpszAttr) ;
       if ( ivalType != noValue )		// attribute has a value
	 {
	    WriteOut(hFile, (LPSTR)"(") ;
	    if ( ivalType != numValue )		// value is a string
	       WriteOut(hFile, (LPSTR)"\"") ;

	    WriteOut(hFile, lpszStr) ;		// output value of attribute
	    if ( ivalType != numValue )		// close the string value
	       WriteOut(hFile, (LPSTR)"\"") ;
	    WriteOut(hFile, (LPSTR)")") ;
	 }
   }


VOID NEAR GetVerNumber (WORD wMajorNum, WORD wMinorNum, char *szVersion)
  {
      char szTmp[6] ;

      _ltoa((long)wMajorNum, szVersion, 10) ;
      _ltoa((long)wMinorNum, szTmp, 10) ;

      osStrCat((LPSTR)szVersion, (LPSTR)".") ;	  // major.
      osStrCat((LPSTR)szVersion, (LPSTR)szTmp) ; // major.minor
   }

VOID NEAR WriteOut(FILE *hFile, LPSTR lpszData)
  {
      char szBuffer[fMaxBuffer];

      osStrCpy((LPSTR)&szBuffer, lpszData) ;

      if (fputs(szBuffer, hFile) < 0)	    // write the data
	 {
	   osStrCpy((LPSTR)&szBuffer, "Fail to write to file ") ;
	   osStrCat((LPSTR)&szBuffer, lpszData) ;
	   osMessage ((LPSTR)szBuffer, (LPSTR)"Tlviewer") ;
	 }
  }

/* -------------------------------------------------------------------------
  Test: 	OSUTIL
 
                Copyright (C) 1991, Microsoft Corporation
 
  Component:	OLE Programmability
 
  Major Area:	Type Information Interface
 
  Sub Area:	ITypeInfo
 
  Test Area:
 
  Keyword:	Win16
 
 ---------------------------------------------------------------------------
  Purpose:	Library routines for programs that run under Win16
 
  Scenarios:

  Abstract:
 
 ---------------------------------------------------------------------------
  Category:
 
  Product:
 
  Related Files: osutil.hxx

  Notes:
 ---------------------------------------------------------------------------
  Revision History:
 
	[ 0]	09-Mar-1993		     Angelach: Created Test
	[ 1]	dd-mmm-yyyy		     changed made, by whom, and why
 ---------------------------------------------------------------------------
*/


/*---------------------------------------------------------------------------
 NAME	    : osAllocSpaces

 PURPOSE    : obtains some spaces from the far heap

 INPUTS     : nSize - no of bytes to be allocated

 OUTPUT     : pointer to the allocated space

 NOTES	    : caller is responsible to free up the memory after used

---------------------------------------------------------------------------*/

char FAR * osAllocSpaces(WORD nSize)
  {
     return ( (char FAR * )_fmalloc(nSize) ) ;
  }


/*---------------------------------------------------------------------------
 NAME	    : osDeAllocSpaces

 PURPOSE    : returns spaces that have been obtained from the far heap

 INPUTS     : lpsz - pointer to the spaces to be returned

 OUTPUT     : none

 NOTES	    : caller is responsible to free up the memory after used

---------------------------------------------------------------------------*/

VOID FAR osDeAllocSpaces(char FAR *lpsz)
  {
     _ffree( lpsz ) ;
  }


/*---------------------------------------------------------------------------
 NAME	    : osStrCmp

 PURPOSE    : Calls lstrcmp to perform comparsion on strings

 INPUTS     : lpszIn, lpszExp; strings to be compared

 OUTPUT     : none

 NOTES	    :

---------------------------------------------------------------------------*/

int FAR osStrCmp (LPSTR lpszIn,	LPSTR lpszExp)
  {
     return ( _fstrcmp(lpszIn, lpszExp) ) ;
  }


/*---------------------------------------------------------------------------
 NAME	    : osStrCpy

 PURPOSE    : Calls lstrcpy to perform string copy

 INPUTS     : lpszDest - destination for string copy
	      lpszSrc  - source string for string copy

 OUTPUT     : none

 NOTES	    :

---------------------------------------------------------------------------*/

VOID FAR osStrCpy (LPSTR lpszDest, LPSTR lpszSrc)
  {
      _fstrcpy(lpszDest, lpszSrc) ;
  }


/*---------------------------------------------------------------------------
 NAME	    : osStrCat

 PURPOSE    : Calls lstrcat to perform string concatenation

 INPUTS     : lpszDest - destination for string concatenation
	      lpszSrc  - source string for concatenation

 OUTPUT     : none

 NOTES	    :

---------------------------------------------------------------------------*/

VOID FAR osStrCat (LPSTR lpszDest, LPSTR lpszSrc)
  {
      _fstrcat(lpszDest, lpszSrc) ;
  }

/*---------------------------------------------------------------------------
 NAME	    : osCreateGuid

 PURPOSE    : Converts a GUID value from string to GUID format

 INPUTS     : lpszGuid - string contains the desired GUID value

 OUTPUT     : pointer to the GUID structure

 NOTES	    : caller is responsible to free up the memory after used

---------------------------------------------------------------------------*/

GUID FAR * osCreateGuid(LPSTR lpszGuid)
   {

     GUID    FAR * lpGuid ;
     HRESULT hRes ;

     lpGuid = (GUID FAR *) osAllocSpaces(sizeof(GUID)) ;// allocate space
							// for the Guid
     if ( lpGuid )
       {					// convert string to GUID format
	  hRes = CLSIDFromString(lpszGuid, (LPCLSID)lpGuid);
	  if ( LOWORD (hRes) )
	    {
	      osDeAllocSpaces ((char FAR *)lpGuid) ;// release space before exit
	      return NULL ;
	    }
	  else
	      return lpGuid ;			// return pointer to the
       }					// GUID structure
     else
       return NULL ;				// no space is allocated

   }


/*---------------------------------------------------------------------------
 NAME	    : osRetrieveGuid

 PURPOSE    : Converts a GUID structure to a readable string format

 INPUTS     : lpszGuid - string representation of the GUID will be returned
	      GUID     - the GUID structure in concern

 OUTPUT     : True if conversion is succeed

 NOTES	    :

---------------------------------------------------------------------------*/

BOOL FAR osRetrieveGuid (LPSTR lpszGuid, GUID inGuid)
   {
      char FAR * lpszTmp ;
      HRESULT hRes ;

      if ( lpszGuid )
	 {				    // convert GUID to it string
	   hRes = StringFromCLSID((REFCLSID) inGuid, &lpszTmp) ;
	   if ( LOWORD (hRes) ) 	    // representation
	     {
	       return FALSE ;
	     }
	   else
	     {
	       _fstrcpy (lpszGuid, (LPSTR)lpszTmp) ;
	       // UNDONE: should free the memory with OLE's iMalloc
	       //osDeAllocSpaces (lpszTmp) ;// release space before exit
	       return TRUE ;
	     }
	 }
      else
	return FALSE ;
   }

/*---------------------------------------------------------------------------
 NAME	    : osGetSize

 PURPOSE    : returns size of the input data

 INPUTS     : inVT - data type; WORD

 OUTPUT     : size of inVT; WORD

 NOTES	    :

---------------------------------------------------------------------------*/

WORD FAR osGetSize (WORD inVT)
   {
      WORD tSize ;

      switch ( inVT )
       {
	 case VT_I2:
	   tSize = sizeof(short) ;
	   break ;
	 case VT_I4:
	   tSize = sizeof(long) ;
	   break ;
	 case VT_R4:
	   tSize = sizeof(float) ;
	   break ;
	 case VT_R8:
	   tSize = sizeof(double) ;
	   break ;
	 case VT_CY:
	   tSize = sizeof(CY) ;
	   break ;
	 case VT_DATE:
	   tSize = sizeof(DATE) ;
	   break ;
	 case VT_BSTR:
	   tSize = sizeof(BSTR) ;
	   break ;
	 case VT_ERROR:
	   tSize = sizeof(SCODE) ;
	   break ;
	 case VT_BOOL:
	   tSize = sizeof(VARIANT_BOOL) ;
	   break ;
	 case VT_VARIANT:
	   tSize = sizeof(VARIANT) ;
	   break ;
	 case VT_I1:
	   tSize = sizeof(char) ;
	   break ;
	 case VT_UI1:
	   tSize = sizeof(char) ;
	   break ;
	 case VT_UI2:
	   tSize = sizeof(short) ;
	   break ;
	 case VT_UI4:
	   tSize = sizeof(long) ;
	   break ;
	 case VT_I8:
	   tSize = sizeof(long)*2 ;
	   break ;
	 case VT_UI8:
	   tSize = sizeof(long)*2 ;
	   break ;
	 case VT_INT:
	   tSize = sizeof(int) ;
	   break ;
	 case VT_UINT:
	   tSize = sizeof(int) ;
	   break ;
	 case VT_VOID:
	   tSize = 0 ;
	   break ;
	 case VT_HRESULT:
	   tSize = sizeof(HRESULT) ;
	   break ;
	 case VT_LPSTR:
	   tSize = sizeof(LPSTR) ;
	   break ;
	 case VT_PTR:
	   tSize = 4 ;
	   break ;
	 case VT_SAFEARRAY:
	   tSize = sizeof(ARRAYDESC FAR *) ;
	   break ;
	 default:
	   tSize = 1 ;
	   break ;
       }

      return tSize ;
}

/*---------------------------------------------------------------------------
 NAME	    : osGetMemberSize

 PURPOSE    : returns size of the member, type of which is inVT, of a struct

 INPUTS     : pSize - max size of the previous members; WORD
	      inVT  - type of the current member; WORD

 OUTPUT     : size of the current member; WORD

 NOTES	    : value is machine dependent:
		 Win16 = 1 (everything is packed -> always = 1)
		 Win32 = natural alignment (everything is on the even-byte
			 boundary; size of every member is based on the biggest
			 size of the members
		 mac   = everything is on the even-byte boundary
	      see silver\cl\clutil.cxx for a table of the alignement information
---------------------------------------------------------------------------*/

WORD FAR osGetMemberSize (WORD *pSize, WORD inVT)
   {
     WORD aSize ;

     aSize = osGetSize(inVT) ;

     return aSize ;
   }

/*---------------------------------------------------------------------------
 NAME	    : osGetAlignment

 PURPOSE    : returns value of the alignment

 INPUTS     : none

 OUTPUT     : value of the aliangment; WORD

 NOTES	    : value is machine dependent:
		 Win16 = 1 (everything is packed -> always = 1)
		 Win32 = natural alignment (everything is on the even-byte
			 boundary)
		 mac   = everything is on the even-byte boundary
	      see silver\cl\clutil.cxx for a table of the alignement information
---------------------------------------------------------------------------*/

WORD FAR osGetAlignment ()
  {
     return 1 ;
  }



/*---------------------------------------------------------------------------
 NAME	    : osMessage

 PURPOSE    : Displays a MessageBox

 INPUTS     : Message to be displayed; a string of characters

 OUTPUT     : none

 NOTES	    :

---------------------------------------------------------------------------*/

VOID FAR osMessage (LPSTR lpszMsg, LPSTR lpszTitle)
  {
#ifdef MAC
    DisplayLine (lpszMsg) ;
#else
    MessageBox (NULL, lpszMsg, lpszTitle, MB_OK) ;
#endif
  }
 }
