/*
 *  CMDINPUT.C - handles command input (tab completion, history, etc.).
 *
 *
 *  History:
 *
 *    01/14/95 (Tim Norman)
 *        started.
 *
 *    08/08/95 (Matt Rains)
 *        i have cleaned up the source code. changes now bring this source
 *        into guidelines for recommended programming practice.
 *        i have added some constants to help making changes easier.
 *
 *    12/12/95 (Tim Norman)
 *        added findxy() function to get max x/y coordinates to display
 *        correctly on larger screens
 *
 *    12/14/95 (Tim Norman)
 *        fixed the Tab completion code that Matt Rains broke by moving local
 *        variables to a more global scope and forgetting to initialize them
 *        when needed
 *
 *    8/1/96 (Tim Norman)
 *        fixed a bug in tab completion that caused filenames at the beginning
 *        of the command-line to have their first letter truncated
 *
 *    9/1/96 (Tim Norman)
 *        fixed a silly bug using printf instead of fputs, where typing "%i"
 *        confused printf :)
 *
 *    6/14/97 (Steffan Kaiser)
 *        ctrl-break checking
 *
 *    6/7/97 (Marc Desrochers)
 *        recoded everything! now properly adjusts when text font is changed.
 *        removed findxy(), reposition(), and reprint(), as these functions
 *        were inefficient. added goxy() function as gotoxy() was buggy when
 *        the screen font was changed. the printf() problem with %i on the
 *        command line was fixed by doing printf("%s",str) instead of
 *        printf(str). Don't ask how I find em just be glad I do :)
 *
 *    7/12/97 (Tim Norman)
 *        Note: above changes pre-empted Steffan's ctrl-break checking.
 *
 *    7/7/97 (Marc Desrochers)
 *        rewrote a new findxy() because the new dir() used it.  This
 *        findxy() simply returns the values of *maxx *maxy.  In the
 *        future, please use the pointers, they will always be correct
 *        since they point to BIOS values.
 *
 *    7/8/97 (Marc Desrochers)
 *        once again removed findxy(), moved the *maxx, *maxy pointers
 *        global and included them as externs in command.h.  Also added
 *        insert/overstrike capability
 *
 *    7/13/97 (Tim Norman)
 *        added different cursor appearance for insert/overstrike mode
 *
 *    7/13/97 (Tim Norman)
 *        changed my code to use _setcursortype until I can figure out why
 *        my code is crashing on some machines.  It doesn't crash on mine :)
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *    28-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        put ifdef's around filename completion code.
 *
 *    30-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        moved filename completion code to filecomp.c
 *        made second TAB display list of filename matches
 *
 *    31-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        Fixed bug where if you typed something, then hit HOME, then tried
 *        to type something else in insert mode, it crashed.
 *
 *    07-Aug-1998 (John P Price <linux-guru@gcfl.net>)
 *        Fixed carrage return output to better match MSDOS with echo
 *        on or off.(marked with "JPP 19980708")
 *
 *    13-Dec-1998 (Eric Kohl)
 *        Added insert/overwrite cursor.
 *
 *    25-Jan-1998 (Eric Kohl)
 *        Replaced CRT io functions by Win32 console io functions.
 *        This can handle <Shift>-<Tab> for 4NT filename completion.
 *        Unicode and redirection safe!
 *
 *    04-Feb-1999 (Eric Kohl)
 *        Fixed input bug. A "line feed" character remained in the keyboard
 *        input queue when you pressed <RETURN>. This sometimes caused
 *        some very strange effects.
 *        Fixed some command line editing annoyances.
 *
 *    30-Apr-2004 (Filip Navara <xnavara@volny.cz>)
 *        Fixed problems when the screen was scrolled away.
 */

#include <precomp.h>
#include "resource.h"


SHORT maxx;
SHORT maxy;

/*
 * global command line insert/overwrite flag
 */
static BOOL bInsert = TRUE;


static VOID
ClearCommandLine (LPTSTR str, INT maxlen, SHORT orgx, SHORT orgy)
{
	INT count;

	SetCursorXY (orgx, orgy);
	for (count = 0; count < (INT)_tcslen (str); count++)
		ConOutChar (_T(' '));
	_tcsnset (str, _T('\0'), maxlen);
	SetCursorXY (orgx, orgy);
}


/* read in a command line */
VOID ReadCommand (LPTSTR str, INT maxlen)
{
	SHORT orgx;			/* origin x/y */
	SHORT orgy;
	SHORT curx;			/*current x/y cursor position*/
	SHORT cury;
	SHORT tempscreen;
	INT   count;		/*used in some for loops*/
	INT   current = 0;	/*the position of the cursor in the string (str)*/
	INT   charcount = 0;/*chars in the string (str)*/
	INPUT_RECORD ir;
	WORD   wLastKey = 0;
	TCHAR  ch;
	BOOL bContinue=FALSE;/*is TRUE the second case will not be executed*/
    BOOL bReturn = FALSE;
    BOOL bCharInput;
    #ifdef FEATURE_4NT_FILENAME_COMPLETION
    TCHAR szPath[MAX_PATH];
    #endif

	/* get screen size */
	GetScreenSize (&maxx, &maxy);

	/* JPP 19980807 - if echo off, don't print prompt */
	if (bEcho)
		PrintPrompt();

	GetCursorXY (&orgx, &orgy);
	GetCursorXY (&curx, &cury);

	memset (str, 0, maxlen * sizeof (TCHAR));

	SetCursorType (bInsert, TRUE);

	do
	{
    
        bReturn = FALSE;
        
		ConInKey (&ir);

		if (ir.Event.KeyEvent.dwControlKeyState &
			(RIGHT_ALT_PRESSED|RIGHT_ALT_PRESSED|
			RIGHT_CTRL_PRESSED|LEFT_CTRL_PRESSED) )
		{

			switch (ir.Event.KeyEvent.wVirtualKeyCode)
			{

#ifdef FEATURE_HISTORY

				case 'K':
					/*add the current command line to the history*/
					if (ir.Event.KeyEvent.dwControlKeyState &
						(LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED))
					{

						if (str[0])
							History(0,str);

						ClearCommandLine (str, maxlen, orgx, orgy);
						current = charcount = 0;
						curx = orgx;
						cury = orgy;
						bContinue=TRUE;
						break;
					}

				case 'D':
					/*delete current history entry*/
					if (ir.Event.KeyEvent.dwControlKeyState &
						(LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED))
					{
						ClearCommandLine (str, maxlen, orgx, orgy);
						History_del_current_entry(str);
						current = charcount = _tcslen (str);
						ConOutPrintf (_T("%s"), str);
						GetCursorXY (&curx, &cury);
						bContinue=TRUE;
						break;
					}

#endif/*FEATURE_HISTORY*/
			}




		}

		//if (bContinue)
		//	continue;


        bCharInput = FALSE;

		switch (ir.Event.KeyEvent.wVirtualKeyCode)
		{
			case VK_BACK:
				/* <BACKSPACE> - delete character to left of cursor */
				if (current > 0 && charcount > 0)
				{
					if (current == charcount)
					{
						/* if at end of line */
						str[current - 1] = _T('\0');
						if (GetCursorX () != 0)
						{
							ConOutPrintf (_T("\b \b"));
							curx--;
						}
						else
						{
							SetCursorXY ((SHORT)(maxx - 1), (SHORT)(GetCursorY () - 1));
							ConOutChar (_T(' '));
							SetCursorXY ((SHORT)(maxx - 1), (SHORT)(GetCursorY () - 1));
							cury--;
							curx = maxx - 1;
						}
					}
					else
					{
						for (count = current - 1; count < charcount; count++)
							str[count] = str[count + 1];
						if (GetCursorX () != 0)
						{
							SetCursorXY ((SHORT)(GetCursorX () - 1), GetCursorY ());
							curx--;
						}
						else
						{
							SetCursorXY ((SHORT)(maxx - 1), (SHORT)(GetCursorY () - 1));
							cury--;
							curx = maxx - 1;
						}
						GetCursorXY (&curx, &cury);
						ConOutPrintf (_T("%s "), &str[current - 1]);
						SetCursorXY (curx, cury);
					}
					charcount--;
					current--;
				}
				break;

			case VK_INSERT:
				/* toggle insert/overstrike mode */
				bInsert ^= TRUE;
				SetCursorType (bInsert, TRUE);
				break;

			case VK_DELETE:
				/* delete character under cursor */
				if (current != charcount && charcount > 0)
				{
					for (count = current; count < charcount; count++)
						str[count] = str[count + 1];
					charcount--;
					GetCursorXY (&curx, &cury);
					ConOutPrintf (_T("%s "), &str[current]);
					SetCursorXY (curx, cury);
				}
				break;

			case VK_HOME:
				/* goto beginning of string */
				if (current != 0)
				{
					SetCursorXY (orgx, orgy);
					curx = orgx;
					cury = orgy;
					current = 0;
				}
				break;

			case VK_END:
				/* goto end of string */
				if (current != charcount)
				{
					SetCursorXY (orgx, orgy);
					ConOutPrintf (_T("%s"), str);
					GetCursorXY (&curx, &cury);
					current = charcount;
				}
				break;

			case VK_TAB:
#ifdef FEATURE_UNIX_FILENAME_COMPLETION
				/* expand current file name */
				if ((current == charcount) ||
				    (current == charcount - 1 &&
				     str[current] == _T('"'))) /* only works at end of line*/
				{
					if (wLastKey != VK_TAB)
					{
						/* if first TAB, complete filename*/
						tempscreen = charcount;
						CompleteFilename (str, charcount);
						charcount = _tcslen (str);
						current = charcount;

						SetCursorXY (orgx, orgy);
						ConOutPrintf (_T("%s"), str);

						if (tempscreen > charcount)
						{
							GetCursorXY (&curx, &cury);
							for (count = tempscreen - charcount; count--; )
								ConOutChar (_T(' '));
							SetCursorXY (curx, cury);
						}
						else
						{
							if (((charcount + orgx) / maxx) + orgy > maxy - 1)
								orgy += maxy - ((charcount + orgx) / maxx + orgy + 1);
						}

						/* set cursor position */
						SetCursorXY ((orgx + current) % maxx,
							     orgy + (orgx + current) / maxx);
						GetCursorXY (&curx, &cury);
					}
					else
					{
						/*if second TAB, list matches*/
						if (ShowCompletionMatches (str, charcount))
						{
							PrintPrompt ();
							GetCursorXY (&orgx, &orgy);
							ConOutPrintf (_T("%s"), str);

							/* set cursor position */
							SetCursorXY ((orgx + current) % maxx,
								     orgy + (orgx + current) / maxx);
							GetCursorXY (&curx, &cury);
						}

					}
				}
				else
				{
					MessageBeep (-1);
				}
#endif
#ifdef FEATURE_4NT_FILENAME_COMPLETION
				
				/* used to later see if we went down to the next line */
				tempscreen = charcount;
				szPath[0]=_T('\0');

				/* str is the whole things that is on the current line 
				   that is and and out.  arg 2 is weather it goes back
					one file or forward one file */
				CompleteFilename(str, !(ir.Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED), szPath, current);
				/* Attempt to clear the line */
				ClearCommandLine (str, maxlen, orgx, orgy);
				curx = orgx;
				cury = orgy;
				current = charcount = 0;				
        //str[0]=_T('\0');
				
				/* Everything is deleted, lets add it back in */
				_tcscpy(str,szPath);
        
				/* Figure out where cusor is going to be after we print it */
				charcount = _tcslen (str);
				current = charcount;

				SetCursorXY (orgx, orgy);
				/* Print out what we have now */
				ConOutPrintf (_T("%s"), str);

				/* Move cursor accordingly */
				if(tempscreen > charcount)
				{
					GetCursorXY (&curx, &cury);
					for(count = tempscreen - charcount; count--; )
						ConOutChar (_T(' '));
					SetCursorXY (curx, cury);
				}
				else
				{
					if(((charcount + orgx) / maxx) + orgy > maxy - 1)
						orgy += maxy - ((charcount + orgx) / maxx + orgy + 1);
				}
				SetCursorXY((short)(((int)orgx + current) % maxx), (short)((int)orgy + ((int)orgx + current) / maxx));
				GetCursorXY(&curx, &cury);

#endif
				break;
            
            case _T('M'):
            case _T('C'):
                /* ^M does the same as return */
                bCharInput = TRUE;
                if(!(ir.Event.KeyEvent.dwControlKeyState &
                   (RIGHT_CTRL_PRESSED|LEFT_CTRL_PRESSED)))
                {
                    break;
                }
                    
			case VK_RETURN:
				/* end input, return to main */
#ifdef FEATURE_HISTORY
				/* add to the history */
				if (str[0])
					History (0, str);
#endif
				ConInDummy ();
				ConOutChar (_T('\n'));
                bReturn = TRUE;
				break;

			case VK_ESCAPE:
				/* clear str  Make this callable! */
				ClearCommandLine (str, maxlen, orgx, orgy);
				curx = orgx;
				cury = orgy;
				current = charcount = 0;
				break;

#ifdef FEATURE_HISTORY
			case VK_F3:
				History_move_to_bottom();
#endif
			case VK_UP:
#ifdef FEATURE_HISTORY
				/* get previous command from buffer */
				ClearCommandLine (str, maxlen, orgx, orgy);
				History (-1, str);
				current = charcount = _tcslen (str);
				if (((charcount + orgx) / maxx) + orgy > maxy - 1)
					orgy += maxy - ((charcount + orgx) / maxx + orgy + 1);
				ConOutPrintf (_T("%s"), str);
				GetCursorXY (&curx, &cury);
#endif
				break;

			case VK_DOWN:
#ifdef FEATURE_HISTORY
				/* get next command from buffer */
				ClearCommandLine (str, maxlen, orgx, orgy);
				History (1, str);
				current = charcount = _tcslen (str);
				if (((charcount + orgx) / maxx) + orgy > maxy - 1)
					orgy += maxy - ((charcount + orgx) / maxx + orgy + 1);
				ConOutPrintf (_T("%s"), str);
				GetCursorXY (&curx, &cury);
#endif
				break;

			case VK_LEFT:
				/* move cursor left */
				if (current > 0)
				{
					current--;
					if (GetCursorX () == 0)
					{
						SetCursorXY ((SHORT)(maxx - 1), (SHORT)(GetCursorY () - 1));
						curx = maxx - 1;
						cury--;
					}
					else
					{
						SetCursorXY ((SHORT)(GetCursorX () - 1), GetCursorY ());
						curx--;
					}
				}
				else
				{
					MessageBeep (-1);
				}
				break;

			case VK_RIGHT:
				/* move cursor right */
				if (current != charcount)
				{
					current++;
					if (GetCursorX () == maxx - 1)
					{
						SetCursorXY (0, (SHORT)(GetCursorY () + 1));
						curx = 0;
						cury++;
					}
					else
					{
						SetCursorXY ((SHORT)(GetCursorX () + 1), GetCursorY ());
						curx++;
					}
				}
				break;

            default:
                /* This input is just a normal char */
                bCharInput = TRUE;

			}
#ifdef _UNICODE
            ch = ir.Event.KeyEvent.uChar.UnicodeChar;
            if (ch >= 32 && (charcount != (maxlen - 2)) && bCharInput)
#else
            ch = ir.Event.KeyEvent.uChar.AsciiChar;
            if ((UCHAR)ch >= 32 && (charcount != (maxlen - 2)) && bCharInput)
#endif /* _UNICODE */
            {
                /* insert character into string... */
                if (bInsert && current != charcount)
                {
                        /* If this character insertion will cause screen scrolling,
                                                                 * adjust the saved origin of the command prompt. */
                        tempscreen = _tcslen(str + current) + curx;
                    if ((tempscreen % maxx) == (maxx - 1) &&
                        (tempscreen / maxx) + cury == (maxy - 1))
                    {
                        orgy--;
                        cury--;
                    }

                    for (count = charcount; count > current; count--)
                        str[count] = str[count - 1];
                    str[current++] = ch;
                    if (curx == maxx - 1)
                        curx = 0, cury++;
                    else
                        curx++;
                    ConOutPrintf (_T("%s"), &str[current - 1]);
                    SetCursorXY (curx, cury);
                    charcount++;
                }
                else
                {
                    if (current == charcount)
                        charcount++;
                    str[current++] = ch;
                    if (GetCursorX () == maxx - 1 && GetCursorY () == maxy - 1)
                        orgy--, cury--;
                    if (GetCursorX () == maxx - 1)
                        curx = 0, cury++;
                    else
                        curx++;
                    ConOutChar (ch);
                }
            }
		
		wLastKey = ir.Event.KeyEvent.wVirtualKeyCode;
	}
	while (!bReturn);

	SetCursorType (bInsert, TRUE);
}
