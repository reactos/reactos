/* 
 * PROJECT    : ReactOS
 * FILE       : wintrust.h
 * DESCRIPTION: ReactOS wintrust lib
 * DATE       : 25.08.2004 (My birthday!)
 * AUTHOR     : Semyon Novikov <tappak@freemail.ru>
 *
 * --------------------------------------------------------------------
 * Copyright (c) 1998, 2004
 *	ReactOS developers team.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the ReactOS developers team
 * 4. Neither the name of project nor the names of its developers
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

typedef struct _WINTRUST_DATA 
{
  DWORD cbStruct;
  LPVOID pPolicyCallbackData;
  LPVOID pSIPClientData;
  DWORD dwUIChoice;
  DWORD fdwRevocationChecks;
  DWORD dwUnionChoice;

   union{
	    struct WINTRUST_FILE_INFO_* pFile;
	    struct WINTRUST_CATALOG_INFO_* pCatalog;
	    struct WINTRUST_BLOB_INFO_* pBlob;
	    struct WINTRUST_SGNR_INFO_* pSgnr;
	    struct WINTRUST_CERT_INFO_* pCert;
	 };

  DWORD dwStateAction;
  HANDLE hWVTStateData;
  WCHAR* pwszURLReference;
  DWORD dwProvFlags;
  DWORD dwUIContext;

} WINTRUST_DATA,*PWINTRUST_DATA;
