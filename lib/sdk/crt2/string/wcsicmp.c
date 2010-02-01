/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */

/*
 * @implemented
 */
int _CDECL _wcsicmp(const wchar_t* cs,const wchar_t * ct)
{
	while (towlower(*cs) == towlower(*ct))
	{
		if (!*cs)
			return 0;
		cs++;
		ct++;
	}
	return towlower(*cs) - towlower(*ct);
}
