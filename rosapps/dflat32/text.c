/* -------------- text.c -------------- */

#include "dflat.h"

int DfTextProc(DFWINDOW wnd, DFMESSAGE msg, DF_PARAM p1, DF_PARAM p2)
{
	int i, len;
	DF_CTLWINDOW *ct = DfGetControl(wnd);
	char *cp, *cp2 = ct->itext;
	char *ptr;

	switch (msg)
	{
		case DFM_SETFOCUS:
			return TRUE;

		case DFM_LEFT_BUTTON:
			return TRUE;

		case DFM_PAINT:
			if (ct == NULL ||
			    ct->itext == NULL ||
			    DfGetText(wnd) != NULL)
				break;
			len = min(ct->dwnd.h, DfMsgHeight(cp2));

			ptr = DfMalloc (strlen (cp2) + 1);
			strcpy (ptr, cp2);
			cp = ptr;
			for (i = 0; i < len; i++)
			{
				int mlen;
				char *txt = cp;
				char *cp1 = cp;
				char *np = strchr(cp, '\n');
				if (np != NULL)
					*np = '\0';
				mlen = strlen(cp);
				while ((cp1=strchr(cp1,DF_SHORTCUTCHAR)) != NULL)
				{
					mlen += 3;
					cp1++;
				}

				if (np != NULL)
					*np = '\n';
				txt = DfMalloc(mlen+1);
				DfCopyCommand(txt, cp, FALSE, DfWndBackground(wnd));
				txt[mlen] = '\0';
				DfSendMessage(wnd, DFM_ADDTEXT, (DF_PARAM)txt, 0);
				if ((cp = strchr(cp, '\n')) != NULL)
					cp++;
				free(txt);
			}
			free (ptr);
			break;

		default:
			break;
	}

	return DfBaseWndProc(DF_TEXT, wnd, msg, p1, p2);
}

/* EOF */
