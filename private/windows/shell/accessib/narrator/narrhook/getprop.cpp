/*************************************************************************
	Project:    Narrator
    Module:     getprop.cpp

    Author:     Charles Oppermann (ChuckOp)   
    Date:       24 October 1996
    
    Notes:      Gets Object Information

    Copyright (C) 1996 by Microsoft Corporation.  All rights reserved.
    See bottom of file for disclaimer
    
    History: Clean up buffer problems : a-anilk

*************************************************************************/
#define STRICT
#include <windows.h>
#include <windowsx.h>
#include <winerror.h>

#include <oleacc.h>

#include "..\Narrator\Narrator.h"
#include "getprop.h"

void GetObjectProperty(IAccessible*, long, int, LPTSTR, UINT);
void VariantMyInit(VARIANT *pv);

// --------------------------------------------------------------------------
//
//  GetObjectProperty
//
// --------------------------------------------------------------------------
void GetObjectProperty(IAccessible * pobj, LONG idChild, int idProperty,
    LPTSTR lpszName,  UINT cchName)
{
    HRESULT hr=0;
    VARIANT varChild;
    BSTR bstr;
    RECT rc;
    VARIANT varResult;
	TCHAR bigbuf[2048] = TEXT("");


	if (!lpszName || cchName<1) return;
    *lpszName = 0;

    //
    // Clear out the possible return value objects
    //
    bstr = NULL;
    SetRectEmpty(&rc);
    VariantMyInit(&varResult);

    //
    // Setup the VARIANT child to pass in to the property
    //
    VariantMyInit(&varChild);
    varChild.vt = VT_I4;
    varChild.lVal = idChild;

    
    //
    // Get the property
    //
    switch (idProperty)
    {
        case ID_NAME:
                hr = pobj->get_accName(varChild, &bstr);
            break;

        case ID_DESCRIPTION:
                hr = pobj->get_accDescription(varChild, &bstr);
            break;

        case ID_VALUE:
                hr = pobj->get_accValue(varChild, &bstr);
            break;

        case ID_HELP:
            // Future enhancement:  Try help file instead if failure
               hr = pobj->get_accHelp(varChild, &bstr);
            break;

        case ID_SHORTCUT:
                hr = pobj->get_accKeyboardShortcut(varChild, &bstr);
            break;

        case ID_DEFAULT:
                hr = pobj->get_accDefaultAction(varChild, &bstr);
            break;

        case ID_ROLE:
                hr = pobj->get_accRole(varChild, &varResult);
            break;

        case ID_STATE:
                hr = pobj->get_accState(varChild, &varResult);
            break;

        case ID_LOCATION:
                hr = pobj->accLocation(&rc.left, &rc.top, &rc.right, &rc.bottom, varChild);
            break;
        case ID_CHILDREN:
//            hr = GetObjectChildren(pobj, idChild, lpszName, cchName);
            break;

        case ID_SELECTION:
//            hr = GetObjectSelection(pobj, idChild, lpszName, cchName);
            break;

        case ID_PARENT:
//            hr = GetObjectParent(pobj, idChild, lpszName, cchName);
            break;

        case ID_WINDOW:
//            hr = GetObjectWindow(pobj, lpszName, cchName);
            break;

    }

    // Return if the IAccessible call failed.
    if (!SUCCEEDED(hr))
    {
#ifdef _DEBUG
        // Pass back the error string to be displayed

        LPVOID  lpMsgBuf;
        LPTSTR  lpTChar;
        int     length;
     
        length = FormatMessage( 
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            hr,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPTSTR) &lpMsgBuf,
            0,
            NULL 
        );
        lpTChar = (LPTSTR)lpMsgBuf;
        // remove the \r\n at the end of the string
        if (length > 2)
            if (lpTChar[length-2] == '\r')
                lpTChar[length-2] = 0;

      	wsprintf(bigbuf, TEXT("['%s' hr 0x%lx idProperty %d pobj 0x%lx idChild 0x%lx]"), lpMsgBuf,hr, idProperty, pobj, idChild);
		bigbuf[cchName-1]=0;
		lstrcpy(lpszName, bigbuf);

        // Free the buffer.
        LocalFree( lpMsgBuf );
#endif
        return;
    }

    //
    // Convert it to a display string
    //
    switch (idProperty)
    {
        // These are the cases already taken care of.
        case ID_SELECTION:
        case ID_CHILDREN:
        case ID_PARENT:
        case ID_WINDOW:
            break;

        // These are the cases where we got unicode string back
        case ID_NAME:
        case ID_DESCRIPTION:
        case ID_VALUE:
        case ID_HELP:
        case ID_SHORTCUT:
        case ID_DEFAULT:
            if (bstr)
            {
#ifdef UNICODE
			lstrcpyn(bigbuf, bstr, sizeof(bigbuf));
#else
            WideCharToMultiByte(CP_ACP, 0, bstr, -1, (LPBYTE)bigbuf, cchName, NULL, NULL);
#endif
           
			SysFreeString(bstr);

            }
            break;

        case ID_LOCATION:
            wsprintf(bigbuf, TEXT("{%04d, %04d, %04d, %04d}"), rc.left, rc.top,
                rc.left + rc.right, rc.top + rc.bottom);
            break;

        case ID_ROLE: // Role can be either I4 or BSTR
            break;

        case ID_STATE: // State can either be I4 or BSTR
            if (varResult.vt == VT_BSTR)
            {
#ifdef UNICODE
			lstrcpyn(bigbuf, varResult.bstrVal, sizeof(bigbuf));
#else
                // If we got back a string, use that.
            WideCharToMultiByte(CP_ACP, 0, varResult.bstrVal, -1,
                    bigbuf, cchName, NULL, NULL);
#endif
            }
            else if (varResult.vt == VT_I4)
            {
                int     iStateBit;
                DWORD   lStateBits;
                LPTSTR  lpszT;
                UINT    cchT;

                // We have a mask of standard states.  Make a string.
                // Separate the states with ",".
                lpszT = bigbuf;

                for (iStateBit = 0, lStateBits = 1; iStateBit < 32; iStateBit++, (lStateBits <<= 1))
                {
                    if (varResult.lVal & lStateBits)
                    {
                        cchT = GetStateText(lStateBits, lpszT, cchName);

						// If it is link, say so
						// if (lStateBits==STATE_SYSTEM_LINKED) Say(SAY_ALWAYS, lpszT);

                        lpszT += cchT;
                        cchName -= cchT;

                        *lpszT++ = ',';
                        *lpszT++ = ' ';
                    }
                }

                //
                // Clip off final ", "
                //
                if (varResult.lVal)
                {
                    *(lpszT-2) = 0;
                    *(lpszT-1) = 0;
                }
                else
                    GetStateText(0, bigbuf, cchName);
            }

            VariantClear(&varResult);
            break;

        default:
            DebugBreak();
    }

	bigbuf[cchName-1]=0;
	lstrcpy(lpszName, bigbuf);
	return;
}


/*************************************************************************
    Function:   GetObjectName
    Purpose:    
    Inputs:     
    Returns:    
    History:
*************************************************************************/
/* DWORD GetObjectName(LPOBJINFO poiObj,LPTSTR lpszBuf, int cchBuf)
{
    BSTR  bszName;
    IAccessible* pIAcc;
    long* pl;

    bszName = NULL;

    // Get the object out of the struct
    pl = poiObj->plObj;

    pIAcc =(IAccessible*)pl;

	GetObjectProperty(pIAcc, poiObj->varChild.lVal, ID_NAME, lpszBuf, cchBuf);
	return(0);
}
*/
/*
DWORD GetObjectDescription(LPOBJINFO poiObj,LPTSTR lpszBuf, int cchBuf)
{
    DWORD dwRetVal;
    BSTR  bszDesc;
    IAccessible* pIAcc;
    long* pl;

    bszDesc = NULL;

    // Get the object out of the struct
    pl = poiObj->plObj;

    pIAcc =(IAccessible*)pl;

    // Get the object's name
    pIAcc->get_accDescription(poiObj->varChild, &bszDesc);
    
    // Did we get name string?
    if (bszDesc)
        {
        // Convert from OLE Unicode
        if (WideCharToMultiByte(CP_ACP, 0, 
                            bszDesc,
                            WC_SEPCHARS, // -1
                            lpszBuf,
                            cchBuf,
                            NULL, NULL))
            {
            SysFreeString(bszDesc);

            dwRetVal = NO_ERROR;
            }
        else
            {
            dwRetVal = GetLastError();
            }
        return(dwRetVal);
        }
        
    
    // Need general failure handling routine
    MessageBeep(MB_ICONEXCLAMATION);
    
    return(ERROR_INVALID_FUNCTION);
}
*/
/*
DWORD GetObjectValue(LPOBJINFO poiObj, LPTSTR lpszBuf, int cchBuf)
{
    DWORD dwRetVal;
    BSTR  bszValue;
    IAccessible* pIAcc;
    long* pl;

    bszValue = NULL;

    // Get the object out of the struct
    pl = poiObj->plObj;

    pIAcc =(IAccessible*)pl;

    // Get the object's name
    pIAcc->get_accValue(poiObj->varChild, &bszValue);
    
    // Did we get name string?
    if (bszValue)
        {
        // Convert from OLE Unicode
        if (WideCharToMultiByte(CP_ACP, 0, 
                            bszValue,
                            WC_SEPCHARS, // -1
                            lpszBuf,
                            cchBuf,
                            NULL, NULL))
            {
            SysFreeString(bszValue);

            dwRetVal = NO_ERROR;
            }
        else
            {
            dwRetVal = GetLastError();
            }
        return(dwRetVal);
        }
        
    
    // Need general failure handling routine
    MessageBeep(MB_ICONEXCLAMATION);
    
    return(ERROR_INVALID_FUNCTION);
}
*/

void VariantMyInit(VARIANT *pv)
{
	VariantInit(pv);
	pv->lVal=0;
}

/*************************************************************************
    THE INFORMATION AND CODE PROVIDED HEREUNDER (COLLECTIVELY REFERRED TO
    AS "SOFTWARE") IS PROVIDED AS IS WITHOUT WARRANTY OF ANY KIND, EITHER
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. IN
    NO EVENT SHALL MICROSOFT CORPORATION OR ITS SUPPLIERS BE LIABLE FOR
    ANY DAMAGES WHATSOEVER INCLUDING DIRECT, INDIRECT, INCIDENTAL,
    CONSEQUENTIAL, LOSS OF BUSINESS PROFITS OR SPECIAL DAMAGES, EVEN IF
    MICROSOFT CORPORATION OR ITS SUPPLIERS HAVE BEEN ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGES. SOME STATES DO NOT ALLOW THE EXCLUSION OR
    LIMITATION OF LIABILITY FOR CONSEQUENTIAL OR INCIDENTAL DAMAGES SO THE
    FOREGOING LIMITATION MAY NOT APPLY.
*************************************************************************/
