/* --------------- lists.c -------------- */

#include "dflat.h"

/* ----- set focus to the next sibling ----- */
void DfSetNextFocus (void)
{
	if (DfInFocus != NULL)
	{
		DFWINDOW wnd1 = DfInFocus, pwnd;
		while (TRUE)
		{
			pwnd = DfGetParent(wnd1);
			if (DfNextWindow(wnd1) != NULL)
				wnd1 = DfNextWindow(wnd1);
			else if (pwnd != NULL)
				wnd1 = DfFirstWindow(pwnd);
			if (wnd1 == NULL || wnd1 == DfInFocus)
			{
				wnd1 = pwnd;
				break;
			}
			if (DfGetClass(wnd1) == DF_STATUSBAR || DfGetClass(wnd1) == DF_MENUBAR)
				continue;
			if (DfIsVisible(wnd1))
				break;
		}
		if (wnd1 != NULL)
		{
			while (wnd1->childfocus != NULL)
				wnd1 = wnd1->childfocus;
			if (wnd1->condition != DF_ISCLOSING)
				DfSendMessage(wnd1, DFM_SETFOCUS, TRUE, 0);
		}
	}
}

/* ----- set focus to the previous sibling ----- */
void DfSetPrevFocus(void)
{
	if (DfInFocus != NULL)
	{
		DFWINDOW wnd1 = DfInFocus, pwnd;
		while (TRUE)
		{
			pwnd = DfGetParent(wnd1);
			if (DfPrevWindow(wnd1) != NULL)
				wnd1 = DfPrevWindow(wnd1);
			else if (pwnd != NULL)
				wnd1 = DfLastWindow(pwnd);
			if (wnd1 == NULL || wnd1 == DfInFocus)
			{
				wnd1 = pwnd;
				break;
			}
			if (DfGetClass(wnd1) == DF_STATUSBAR)
				continue;
			if (DfIsVisible(wnd1))
				break;
		}
		if (wnd1 != NULL)
		{
			while (wnd1->childfocus != NULL)
				wnd1 = wnd1->childfocus;
			if (wnd1->condition != DF_ISCLOSING)
				DfSendMessage(wnd1, DFM_SETFOCUS, TRUE, 0);
		}
	}
}

/* ------- move a window to the end of its parents list ----- */
void DfReFocus(DFWINDOW wnd)
{
	if (DfGetParent(wnd) != NULL)
	{
		DfRemoveWindow(wnd);
		DfAppendWindow(wnd);
		DfReFocus(DfGetParent(wnd));
	}
}

/* ---- remove a window from the linked list ---- */
void DfRemoveWindow(DFWINDOW wnd)
{
	if (wnd != NULL)
	{
		DFWINDOW pwnd = DfGetParent(wnd);

		if (DfPrevWindow(wnd) != NULL)
			DfNextWindow(DfPrevWindow(wnd)) = DfNextWindow(wnd);
		if (DfNextWindow(wnd) != NULL)
			DfPrevWindow(DfNextWindow(wnd)) = DfPrevWindow(wnd);
		if (pwnd != NULL)
		{
			if (wnd == DfFirstWindow(pwnd))
				DfFirstWindow(pwnd) = DfNextWindow(wnd);
			if (wnd == DfLastWindow(pwnd))
				DfLastWindow(pwnd) = DfPrevWindow(wnd);
		}
	}
}

/* ---- append a window to the linked list ---- */
void DfAppendWindow(DFWINDOW wnd)
{
	if (wnd != NULL)
	{
		DFWINDOW pwnd = DfGetParent(wnd);
		if (pwnd != NULL)
		{
			if (DfFirstWindow(pwnd) == NULL)
			{
				DfFirstWindow(pwnd) = wnd;
				DfLastWindow(pwnd) = wnd;
				DfPrevWindow(wnd) = NULL;
			}
			else
			{
				DfNextWindow(DfLastWindow(pwnd)) = wnd;
				DfPrevWindow(wnd) = DfLastWindow(pwnd);
				DfLastWindow(pwnd) = wnd;
			}
		}
		DfNextWindow(wnd) = NULL;
	}
}

/*
 * if document windows and statusbar or menubar get the focus,
 * pass it on
 */
void DfSkipApplicationControls(void)
{
	BOOL EmptyAppl = FALSE;
	int ct = 0;
	while (!EmptyAppl && DfInFocus != NULL)
	{
		DFCLASS cl = DfGetClass(DfInFocus);
		if (cl == DF_MENUBAR || cl == DF_STATUSBAR)
		{
			DfSetPrevFocus();
			EmptyAppl = (cl == DF_MENUBAR && ct++);
		}
		else
			break;
	}
}

/* EOF */
