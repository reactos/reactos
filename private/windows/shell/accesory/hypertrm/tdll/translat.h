/*	File: D:\WACKER\translat.h (Created: 24-Aug-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:39p $
 */

HTRANSLATE CreateTranslateHandle(HSESSION hSession);
int InitTranslateHandle(HTRANSLATE hTranslate);
int LoadTranslateHandle(HTRANSLATE hTranslate);
int SaveTranslateHandle(HTRANSLATE hTranslate);
int DestroyTranslateHandle(HTRANSLATE hTranslate);
