#include <dflat32/dflat.h>

DFWINDOW ApplicationWindow; //applicat.c

void ClearDialogBoxes(void) // no clue
{
}


// from normal.c

/* -------- array of class definitions -------- */
CLASSDEFS classdefs[] = {
    #undef ClassDef
    #define ClassDef(c,b,p,a) {b,p,a},
//    #include <dflat32/classes.h>
};

BOOL isAncestor(DFWINDOW wnd, DFWINDOW awnd)
{
	while (wnd != NULL)	{
		if (wnd == awnd)
			return TRUE;
		wnd = GetParent(wnd);
	}
	return FALSE;
}

BOOL isVisible(DFWINDOW wnd)
{
    while (wnd != NULL)    {
        if (isHidden(wnd))
            return FALSE;
        wnd = GetParent(wnd);
    }
    return TRUE;
}


void BuildTextPointers(DFWINDOW wnd)
{
}


// log.c
void LogMessages (DFWINDOW wnd, DFMESSAGE msg, PARAM p1, PARAM p2)
{
}