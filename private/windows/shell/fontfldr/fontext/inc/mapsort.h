/***************************************************************************
 * mapsort.h -
 *
 * $keywords: mapsort.h 1.0  7-Mar-94 2:39:49 PM$
 *
 * Copyright (C) 1992-93 ElseWare Corporation.    All rights reserved.
 ***************************************************************************/

#ifndef __MAPSORT_H_
#define __MAPSORT_H_


/* API to the Map/Sort module
 */

extern RC      FAR PASCAL rcMPInit( );
extern VOID    FAR PASCAL vMPLoadListBox( );
extern VOID    FAR PASCAL vMPSort( FontClass* lpFontRec );
extern WORD    FAR PASCAL wMPGetEquivList( FontClass* lpFontRec,
                                           WORD       wRequestedHits,
                                           FontClass* * pEquiv );

#endif /* __MAPSORT_H_ */



/****************************************************************************
 * $lgb$
 * 1.0     7-Mar-94   eric Initial revision.
 * $lge$
 *
 ****************************************************************************/

