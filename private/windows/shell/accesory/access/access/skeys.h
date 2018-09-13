/*--------------------------------------------------------------
 *
 * FILE:			SKEYS.H
 *
 * PURPOSE:			The file contains data structures for the 
 *					transmission of information between the 
 *					Serial Keys Application and and the DLL.
 *
 * CREATION:		June 1994
 *
 * COPYRIGHT:		Black Diamond Software (C) 1994
 *
 * AUTHOR:			Ronald Moak 
 *
 * $Header: %Z% %F% %H% %T% %I%
 *
 *------------------------------------------------------------*/

#define SK_SPI_INITUSER -1

BOOL APIENTRY SKEY_SystemParametersInfo(
    UINT uAction, UINT uParam, LPSERIALKEYS lpvParam, BOOL fWinIni);
