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
 *    25-Jan-1999 (Eric Kohl)
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

#include "precomp.h"

#ifdef FEATURE_HISTORY

typedef struct tagHISTORY
{
    struct tagHISTORY *prev;
    struct tagHISTORY *next;
    LPTSTR string;
} HIST_ENTRY, * LPHIST_ENTRY;

static INT size, max_size = 100;

static LPHIST_ENTRY Top = NULL;
static LPHIST_ENTRY Bottom = NULL;

static LPHIST_ENTRY curr_ptr = NULL;

VOID InitHistory(VOID);
VOID History_move_to_bottom(VOID);
VOID History(INT dir, LPTSTR commandline);
VOID CleanHistory(VOID);
VOID History_del_current_entry(LPTSTR str);

/*service functions*/
static VOID del(LPHIST_ENTRY item);
static VOID add_at_bottom(LPTSTR string);
/*VOID add_before_last(LPTSTR string);*/
VOID set_size(INT new_size);


INT CommandHistory(LPTSTR param)
{
    LPTSTR tmp;
    INT tmp_int;
    LPHIST_ENTRY h_tmp;
    TCHAR szBuffer[2048];

    tmp=_tcschr(param,_T('/'));

    if (tmp)
    {
        param=tmp;
        switch (_totupper(param[1]))
        {
            case _T('F'):/*delete history*/
                CleanHistory();InitHistory();
                break;

            case _T('R'):/*read history from standard in*/
                for(;;)
                {
                    ConInString(szBuffer,sizeof(szBuffer)/sizeof(TCHAR));
                    if (*szBuffer!=_T('\0'))
                        History(0,szBuffer);
                    else
                        break;
                }
                break;

            case _T('A'):/*add an antry*/
                History(0,param+2);
                break;

            case _T('S'):/*set history size*/
                if ((tmp_int=_ttoi(param+2)))
                    set_size(tmp_int);
                break;

            default:
                return 1;
        }
    }
    else
    {
        for (h_tmp = Top->prev; h_tmp != Bottom; h_tmp = h_tmp->prev)
            ConErrPuts(h_tmp->string);
    }
    return 0;
}

VOID set_size(INT new_size)
{
    ASSERT(Top && Bottom);

    while (new_size<size)
        del(Top->prev);

    max_size=new_size;
}


VOID InitHistory(VOID)
{
    size = 0;

    Top = cmd_alloc(sizeof(HIST_ENTRY));
    if (!Top)
    {
        WARN("Cannot allocate memory for Top!\n");
        return;
    }
    Bottom = cmd_alloc(sizeof(HIST_ENTRY));
    if (!Bottom)
    {
        WARN("Cannot allocate memory for Bottom!\n");
        cmd_free(Top);
        Top = NULL;
        return;
    }

    Top->prev = Bottom;
    Top->next = NULL;
    Top->string = NULL;

    Bottom->prev = NULL;
    Bottom->next = Top;
    Bottom->string = NULL;

    curr_ptr = Bottom;
}


VOID CleanHistory(VOID)
{
    ASSERT(Top && Bottom);

    while (Bottom->next != Top)
        del(Bottom->next);

    cmd_free(Top);
    cmd_free(Bottom);
}


VOID History_del_current_entry(LPTSTR str)
{
    LPHIST_ENTRY tmp;

    ASSERT(Top && Bottom);

    if (size == 0)
        return;

    if (curr_ptr == Bottom)
        curr_ptr = Bottom->next;

    if (curr_ptr == Top)
        curr_ptr = Top->prev;


    tmp = curr_ptr;
    curr_ptr = curr_ptr->prev;
    del(tmp);
    History(-1, str);
}


static
VOID del(LPHIST_ENTRY item)
{
    ASSERT(Top && Bottom);

    if (item==NULL || item==Top || item==Bottom)
    {
        TRACE ("del in " __FILE__ ": returning\n"
                "item is 0x%08x (Bottom is0x%08x)\n",
                item, Bottom);
        return;
    }

    /*free string's mem*/
    if (item->string)
        cmd_free(item->string);

    /*set links in prev and next item*/
    item->next->prev=item->prev;
    item->prev->next=item->next;

    cmd_free(item);

    size--;
}

static
VOID add_at_bottom(LPTSTR string)
{
    LPHIST_ENTRY tmp;

    ASSERT(Top && Bottom);

    /*delete first entry if maximum number of entries is reached*/
    while (size>=max_size)
        del(Top->prev);

    while (_istspace(*string))
        string++;

    if (*string==_T('\0'))
        return;

    /*if new entry is the same than the last do not add it*/
    if (size)
    {
        if (_tcscmp(string,Bottom->next->string)==0)
            return;
    }

    /*create new empty Bottom*/
    tmp = cmd_alloc(sizeof(HIST_ENTRY));
    if (!tmp)
    {
        WARN("Cannot allocate memory for new Bottom!\n");
        return;
    }

    /*fill old bottom with string, it will become new Bottom->next*/
    Bottom->string = cmd_alloc((_tcslen(string)+1)*sizeof(TCHAR));
    if (!Bottom->string)
    {
        WARN("Cannot allocate memory for Bottom->string!\n");
        cmd_free(tmp);
        return;
    }
    _tcscpy(Bottom->string,string);

    tmp->next = Bottom;
    tmp->prev = NULL;
    tmp->string = NULL;

    Bottom->prev = tmp;

    /*save the new Bottom value*/
    Bottom = tmp;

    /*set new size*/
    size++;
}


VOID History_move_to_bottom(VOID)
{
    ASSERT(Top && Bottom);

    curr_ptr = Bottom;
}

LPCTSTR PeekHistory(INT dir)
{
    LPHIST_ENTRY entry = curr_ptr;

    ASSERT(Top && Bottom);

    if (dir == 0)
        return NULL;

    if (dir < 0)
    {
        /* key up */
        if (entry->next == Top || entry == Top)
        {
#ifdef WRAP_HISTORY
            entry = Bottom;
#else
            return NULL;
#endif
        }
        entry = entry->next;
    }
    else
    {
        /* key down */
        if (entry->prev == Bottom || entry == Bottom)
        {
#ifdef WRAP_HISTORY
            entry = Top;
#else
            return NULL;
#endif
        }
        entry = entry->prev;
    }

    return entry->string;
}

VOID History(INT dir, LPTSTR commandline)
{
    ASSERT(Top && Bottom);

    if (dir==0)
    {
        add_at_bottom(commandline);
        curr_ptr = Bottom;
        return;
    }

    if (size==0)
    {
        commandline[0]=_T('\0');
        return;
    }

    if (dir<0)/*key up*/
    {
        if (curr_ptr->next==Top || curr_ptr==Top)
        {
#ifdef WRAP_HISTORY
            curr_ptr = Bottom;
#else
            curr_ptr = Top;
            commandline[0]=_T('\0');
            return;
#endif
        }

        curr_ptr = curr_ptr->next;
        if (curr_ptr->string)
            _tcscpy(commandline,curr_ptr->string);
    }

    if (dir>0)
    {
        if (curr_ptr->prev==Bottom || curr_ptr==Bottom)
        {
#ifdef WRAP_HISTORY
            curr_ptr = Top;
#else
            curr_ptr = Bottom;
            commandline[0]=_T('\0');
            return;
#endif
        }

        curr_ptr = curr_ptr->prev;
        if (curr_ptr->string)
            _tcscpy(commandline,curr_ptr->string);
    }
}

#endif //#if FEATURE_HISTORY
