/* --------------- lists.c -------------- */

#include "dflat.h"

/* ----- set focus to the next sibling ----- */
void SetNextFocus (void)
{
	if (inFocus != NULL)
	{
		DFWINDOW wnd1 = inFocus, pwnd;
		while (TRUE)
		{
			pwnd = GetParent(wnd1);
			if (NextWindow(wnd1) != NULL)
				wnd1 = NextWindow(wnd1);
			else if (pwnd != NULL)
				wnd1 = FirstWindow(pwnd);
			if (wnd1 == NULL || wnd1 == inFocus)
			{
				wnd1 = pwnd;
				break;
			}
			if (GetClass(wnd1) == STATUSBAR || GetClass(wnd1) == MENUBAR)
				continue;
			if (isVisible(wnd1))
				break;
		}
		if (wnd1 != NULL)
		{
			while (wnd1->childfocus != NULL)
				wnd1 = wnd1->childfocus;
			if (wnd1->condition != ISCLOSING)
				DfSendMessage(wnd1, SETFOCUS, TRUE, 0);
		}
	}
}

/* ----- set focus to the previous sibling ----- */
void SetPrevFocus(void)
{
	if (inFocus != NULL)
	{
		DFWINDOW wnd1 = inFocus, pwnd;
		while (TRUE)
		{
			pwnd = GetParent(wnd1);
			if (PrevWindow(wnd1) != NULL)
				wnd1 = PrevWindow(wnd1);
			else if (pwnd != NULL)
				wnd1 = LastWindow(pwnd);
			if (wnd1 == NULL || wnd1 == inFocus)
			{
				wnd1 = pwnd;
				break;
			}
			if (GetClass(wnd1) == STATUSBAR)
				continue;
			if (isVisible(wnd1))
				break;
		}
		if (wnd1 != NULL)
		{
			while (wnd1->childfocus != NULL)
				wnd1 = wnd1->childfocus;
			if (wnd1->condition != ISCLOSING)
				DfSendMessage(wnd1, SETFOCUS, TRUE, 0);
		}
	}
}

/* ------- move a window to the end of its parents list ----- */
void ReFocus(DFWINDOW wnd)
{
	if (GetParent(wnd) != NULL)
	{
		RemoveWindow(wnd);
		AppendWindow(wnd);
		ReFocus(GetParent(wnd));
	}
}

/* ---- remove a window from the linked list ---- */
void RemoveWindow(DFWINDOW wnd)
{
	if (wnd != NULL)
	{
		DFWINDOW pwnd = GetParent(wnd);

		if (PrevWindow(wnd) != NULL)
			NextWindow(PrevWindow(wnd)) = NextWindow(wnd);
		if (NextWindow(wnd) != NULL)
			PrevWindow(NextWindow(wnd)) = PrevWindow(wnd);
		if (pwnd != NULL)
		{
			if (wnd == FirstWindow(pwnd))
				FirstWindow(pwnd) = NextWindow(wnd);
			if (wnd == LastWindow(pwnd))
				LastWindow(pwnd) = PrevWindow(wnd);
		}
	}
}

/* ---- append a window to the linked list ---- */
void AppendWindow(DFWINDOW wnd)
{
	if (wnd != NULL)
	{
		DFWINDOW pwnd = GetParent(wnd);
		if (pwnd != NULL)
		{
			if (FirstWindow(pwnd) == NULL)
			{
				FirstWindow(pwnd) = wnd;
				LastWindow(pwnd) = wnd;
				PrevWindow(wnd) = NULL;
			}
			else
			{
				NextWindow(LastWindow(pwnd)) = wnd;
				PrevWindow(wnd) = LastWindow(pwnd);
				LastWindow(pwnd) = wnd;
			}
		}
		NextWindow(wnd) = NULL;
	}
}

/*
 * if document windows and statusbar or menubar get the focus,
 * pass it on
 */
void SkipApplicationControls(void)
{
	BOOL EmptyAppl = FALSE;
	int ct = 0;
	while (!EmptyAppl && inFocus != NULL)
	{
		DFCLASS cl = GetClass(inFocus);
		if (cl == MENUBAR || cl == STATUSBAR)
		{
			SetPrevFocus();
			EmptyAppl = (cl == MENUBAR && ct++);
		}
		else
			break;
	}
}

/* EOF */
