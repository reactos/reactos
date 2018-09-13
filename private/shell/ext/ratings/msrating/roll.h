//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1996                    **
//*********************************************************************

#ifndef _ROLL_H_
#define _ROLL_H_


/*Includes-----------------------------------------------------------*/

/*Classes------------------------------------------------------------*/
/*
The format of the local list file is as follows:

	BATCAVE_LOCAL_LIST_MAGIC_COOKIE
	LocalListRecordHeader_1 pUrl_1 pRating_1
	.
	.
	.
	.
	LocalListRecordHeader_n pUrl_n pRating_n

The file is binary.

Entries should be sorted based on pUrl

pUrl and pRating are both strings whose size is determined
by the record header.  They are NOT null terminated!
*/

#define BATCAVE_LOCAL_LIST_MAGIC_COOKIE 0x4e4f5845

//BUG BUG should either be inside of registry or user profile
#define FILE_NAME_LIST  "ratings.lst"

struct LocalListRecordHeader{
	int     nUrl;
	int     nRating;
	HRESULT hrRet;
};

/*Prototypes---------------------------------------------------------*/
HRESULT RatingHelperProcLocalList(LPCTSTR pszTargetUrl, HANDLE hAbortEvent, void* (WINAPI *MemAlloc)(long size), char **ppRatingOut);

#endif 
//_ROLL_H_

