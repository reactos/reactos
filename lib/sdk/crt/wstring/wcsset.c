/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crt/??????
 * PURPOSE:     Unknown
 * PROGRAMER:   Unknown
 * UPDATE HISTORY:
 *              25/11/05: Added license header
 */

#include <precomp.h>

/*
 * @implemented
 */
wchar_t* _wcsnset (wchar_t* wsToFill, wchar_t wcFill, size_t sizeMaxFill)
{
	wchar_t *t = wsToFill;
	size_t i = 0;
	while( *wsToFill != 0 && i < sizeMaxFill)
	{
		*wsToFill = wcFill;
		wsToFill++;
		i++;

	}
	return t;
}

/*
 * @implemented
 */
wchar_t* _wcsset (wchar_t* wsToFill, wchar_t wcFill)
{
	wchar_t *t = wsToFill;
	while( *wsToFill != 0 )
	{
		*wsToFill = wcFill;
		wsToFill++;

	}
	return t;
}
