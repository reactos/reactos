/* npdate - Code for getting and inserting current date and time.
 *   Copyright (C) 1984-1995 Microsoft Inc.
 */

#include "precomp.h"

/* ** Replace current selection with date/time string.
 *    if fCrlf is true, date/time string should begin
 *    and end with crlf
*/
VOID FAR InsertDateTime (BOOL fCrlf)
{
   SYSTEMTIME time ;
   TCHAR szDate[80] ;
   TCHAR szTime[80] ;
   TCHAR szDateTime[sizeof(szDate) + sizeof(szTime) + 10] = TEXT("");
   DWORD locale;
   BOOL bMELocale;
   DWORD dwFlags = DATE_SHORTDATE;

   //  See if the user locale id is Arabic or Hebrew.
   locale    = GetUserDefaultLCID();
   bMELocale = ((PRIMARYLANGID(LANGIDFROMLCID(locale)) == LANG_ARABIC) ||
                (PRIMARYLANGID(LANGIDFROMLCID(locale)) == LANG_HEBREW));

   locale = MAKELCID( MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), SORT_DEFAULT) ;

   // Get the time
   GetLocalTime( &time ) ;

   if (bMELocale)
   {
       //Get the date format that matches the edit control reading direction.
       if (GetWindowLong(hwndEdit, GWL_EXSTYLE) & WS_EX_RTLREADING) {
           dwFlags |= DATE_RTLREADING;
           lstrcat(szDateTime, TEXT("\x200F")); // RLM
       } else {
           dwFlags |= DATE_LTRREADING;
           lstrcat(szDateTime, TEXT("\x200E")); // LRM
       }
   }

   // Format date and time
   GetDateFormat(locale,dwFlags, &time,NULL,szDate,CharSizeOf(szDate));
   GetTimeFormat(locale,TIME_NOSECONDS,&time,NULL,szTime,CharSizeOf(szTime));

   if( fCrlf )
       lstrcat(szDateTime, TEXT("\r\n"));


   lstrcat(szDateTime, szTime);
   lstrcat(szDateTime, TEXT(" "));
   lstrcat(szDateTime, szDate);

   if( fCrlf )
        lstrcat(szDateTime, TEXT("\r\n"));

   // send it in one shot; this is also useful for undo command
   // so that user can undo the date-time.
   SendMessage(hwndEdit, EM_REPLACESEL, TRUE, (LPARAM)szDateTime);

}
