/*
 *  HISTORY.C - command line history.
 *
 *
 *  History:
 *
 *    14/01/95 (Tim Norman)
 *        started.
 *
 *    08/08/95 (Matt Rains)
 *        i have cleaned up the source code. changes now bring this source
 *        into guidelines for recommended programming practice.
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *    25-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Cleanup!
 *        Unicode and redirection safe!
 *
 *    25-Jan-1999 (Paolo Pantaleo <paolopan@freemail.it>)
 *        Added lots of comments (beginning studying the source)
 *        Added command.com's F3 support (see cmdinput.c)
 *
 */



/*
 *  HISTORY.C - command line history. Second version
 *
 *
 *  History:
 *
 *    06/12/99 (Paolo Pantaleo <paolopan@freemail.it>)
 *        started.
 *
 */ 






#include "config.h"


#ifdef FEATURE_HISTORY

#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <stdlib.h>

#include "cmd.h"


typedef struct tagHISTORY
{
	struct tagHISTORY *prev;
	struct tagHISTORY *next;
	LPTSTR string;
} HIST_ENTRY, * LPHIST_ENTRY;

static INT size,
	max_size=10; 
	/*for now not configurable*/


static LPHIST_ENTRY Top;
static LPHIST_ENTRY Bottom;


static LPHIST_ENTRY curr_ptr=0;


VOID InitHistory(VOID);
VOID History_move_to_bottom(VOID);
VOID History (INT dir, LPTSTR commandline);
VOID CleanHistory(VOID);
VOID History_del_current_entry(LPTSTR str);

/*service functions*/
VOID del(LPHIST_ENTRY item);
VOID add_at_bottom(LPTSTR string);



VOID InitHistory(VOID)
{
	size=0;
	
	
	Top = malloc(sizeof(HIST_ENTRY));
	Bottom = malloc(sizeof(HIST_ENTRY));	


	Top->prev = Bottom;
	Top->next = NULL;
	Top->string = NULL;

	
	Bottom->prev = NULL;
	Bottom->next = Top;	
	Bottom->string = NULL;

	curr_ptr=Bottom;
}




VOID CleanHistory(VOID)
{
	
	while (Bottom->next!=Top)
		del(Bottom->next);

	free(Top);
	free(Bottom);

}


VOID History_del_current_entry(LPTSTR str)
{
	LPHIST_ENTRY tmp;
	
	if (size==0)
		return;

	if(curr_ptr==Bottom)
		curr_ptr=Bottom->next;

	if(curr_ptr==Top)
		curr_ptr=Top->prev;


	tmp=curr_ptr;		
	curr_ptr=curr_ptr->prev;
	del(tmp);
	History(-1,str);

}


static
VOID del(LPHIST_ENTRY item)
{

	if( item==NULL || item==Top || item==Bottom )
	{
#ifdef _DEBUG
		DebugPrintf("del in " __FILE__  ": retrning\n"
			"item is 0x%08x (Bottom is0x%08x)\n",
			item, Bottom);			

#endif
		return;
	}



	/*free string's mem*/
	if (item->string)
		free(item->string);
	




	/*set links in prev and next item*/
	item->next->prev=item->prev;
	item->prev->next=item->next;	

	free(item);

	size--;

}


static
VOID add_at_bottom(LPTSTR string)
{	
	{

		LPHIST_ENTRY tmp;

		
		/*delete first entry if maximum number of entries is reached*/
		if(size==max_size)
			del(Top->prev);


		
		/*fill bottom with string*/		
		Bottom->string=malloc(_tcslen(string)+1);
		_tcscpy(Bottom->string,string);		

		/*save Bottom value*/
		tmp=Bottom;


		/*create new void Bottom*/
		Bottom=malloc(sizeof(HIST_ENTRY));		
		Bottom->next=tmp;
		Bottom->prev=NULL;
		Bottom->string=NULL;

		tmp->prev=Bottom;

		/*set new size*/
		size++;

	}

}



VOID History_move_to_bottom(VOID)
{
	curr_ptr=Bottom;

}


VOID History (INT dir, LPTSTR commandline)
{
	
	if(dir==0)
	{
		add_at_bottom(commandline);
		curr_ptr=Bottom;
		return;
	}

	if (size==0)
	{
		commandline[0]=_T('\0');
		return;
	}


	if(dir<0)/*key up*/
	{
		if (curr_ptr->next==Top || curr_ptr==Top)
		{
#ifdef WRAP_HISTORY			
			curr_ptr=Bottom;			
#else			
			curr_ptr=Top;
			commandline[0]=_T('\0');
			return;
#endif
		}

		
		curr_ptr = curr_ptr->next;
		if(curr_ptr->string)
			_tcscpy(commandline,curr_ptr->string);

	}
		
		



	if(dir>0)
	{

		if (curr_ptr->prev==Bottom || curr_ptr==Bottom)
		{
#ifdef WRAP_HISTORY			
			curr_ptr=Top;
#else
			curr_ptr=Bottom;
			commandline[0]=_T('\0');
			return;
#endif
		}
		
		curr_ptr=curr_ptr->prev;		
		if(curr_ptr->string)
			_tcscpy(commandline,curr_ptr->string);		
		
	}
}






#if 0

LPTSTR history = NULL;	/*buffer to sotre all the lines*/
LPTSTR lines[MAXLINES];	/*array of pointers to each line(entry)*/
						/*located in history buffer*/
	
INT curline = 0;		/*the last line recalled by user*/
INT numlines = 0;		/*number of entries, included the last*/
						/*empty one*/

INT maxpos = 0;			/*index of last byte of last entry*/



VOID History (INT dir, LPTSTR commandline)
{
	
	INT count;						/*used in for loops*/
	INT length;						/*used in the same loops of count*/
									/*both to make room when is full
									either history or lines*/

	/*first time History is called allocate mem*/
	if (!history)
	{
		history = malloc (history_size * sizeof (TCHAR));
		lines[0] = history;
		history[0] = 0;
	}

	if (dir > 0)
	{
		/* next command */
		if (curline < numlines)
		{
			curline++;
		}

		if (curline == numlines)
		{
			commandline[0] = 0;
		}
		else
		{
			_tcscpy (commandline, lines[curline]);
		}
	}
	else if (dir < 0)
	{
		/* prev command */
		if (curline > 0)
		{
			curline--;
		}

		_tcscpy (commandline, lines[curline]);
	}
	else
	{
		/* add to history */
		/* remove oldest string until there's enough room for next one */
		/* strlen (commandline) must be less than history_size! */
		while ((maxpos + (INT)_tcslen (commandline) + 1 > history_size) || (numlines >= MAXLINES))
		{
			length = _tcslen (lines[0]) + 1;

			for (count = 0; count < maxpos && count + (lines[1] - lines[0]) < history_size; count++)
			{
				history[count] = history[count + length];
			}

			maxpos -= length;

			for (count = 0; count <= numlines && count < MAXLINES; count++)
			{
				lines[count] = lines[count + 1] - length;
			}

			numlines--;
#ifdef DEBUG
			ConOutPrintf (_T("Reduced size:  %ld lines\n"), numlines);

			for (count = 0; count < numlines; count++)
			{
				ConOutPrintf (_T("%d: %s\n"), count, lines[count]);
			}
#endif
		}

		/*copy entry in the history bufer*/
		_tcscpy (lines[numlines], commandline);
		numlines++;
		
		/*set last lines[numlines] pointer next the end of last, valid,
		just setted entry (the two lines above)*/
		lines[numlines] = lines[numlines - 1] + _tcslen (commandline) + 1;
		maxpos += _tcslen (commandline) + 1;
		/* last line, empty */

		curline = numlines;
	}

	return;
}

#endif

#endif //#if 0