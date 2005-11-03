/* -------------- text.c -------------- */

#include "dflat32/dflat.h"

int TextProc(DFWINDOW wnd, DFMESSAGE msg, PARAM p1, PARAM p2)
{
	int i, len;
	CTLWINDOW *ct = GetControl(wnd);
	char *cp, *cp2 = ct->itext;
	char *ptr;

	switch (msg)
	{
		case SETFOCUS:
			return TRUE;

		case LEFT_BUTTON:
			return TRUE;

		case PAINT:
			if (ct == NULL ||
			    ct->itext == NULL ||
			    GetText(wnd) != NULL)
				break;
			len = min(ct->dwnd.h, MsgHeight(cp2));

			ptr = DFmalloc (strlen (cp2) + 1);
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
				while ((cp1=strchr(cp1,SHORTCUTCHAR)) != NULL)
				{
					mlen += 3;
					cp1++;
				}

				if (np != NULL)
					*np = '\n';
				txt = DFmalloc(mlen+1);
				CopyCommand(txt, cp, FALSE, WndBackground(wnd));
				txt[mlen] = '\0';
				DfSendMessage(wnd, ADDTEXT, (PARAM)txt, 0);
				if ((cp = strchr(cp, '\n')) != NULL)
					cp++;
				free(txt);
			}
			free (ptr);
			break;

		default:
			break;
	}

	return BaseWndProc(TEXT, wnd, msg, p1, p2);
}

/* EOF */
