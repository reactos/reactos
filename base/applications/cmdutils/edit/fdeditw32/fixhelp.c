/* ------ fixhelp.c ------ */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "htree.h"
#define FIXHELP
#include "helpbox.h"

#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

static FILE *helpfp;
static char hline [160];

static struct helps *FirstHelp;
static struct helps *LastHelp;
static struct helps *ThisHelp;

static void WriteText(char *);

/* ---- compute the displayed length of a help text line --- */
static int HelpLength(char *s)
{
    int len=strlen(s);
    char *cp=strchr(s, '[');

    while (cp != NULL)
        {
        len -= 4;
        cp=strchr(cp+1, '[');
        }

    cp=strchr(s, '<');
    while (cp != NULL)
        {
        char *cp1=strchr(cp, '>');

        if (cp1 != NULL)
            len -= (int) (cp1-cp)+1;

        cp=strchr(cp1, '<');
        }

    return len;

}

int FindHelp(char *nm)
{
    int hlp=0;
    struct helps *thishelp=FirstHelp;

    if (nm == NULL)
        return -1;

    while (thishelp != NULL)
        {
        if (strcmp(nm, thishelp->hname)==0)
            break;

        hlp++;
        thishelp=thishelp->NextHelp;
	}

    return thishelp ? hlp : -1;

}

void BuildFileName(char *fn, const char *fname, const char *ext)
{
    strcpy(fn, fname);
    strcat(fn, ext);
}

static void WriteText(char *text)
{
    char *np=text ? text : "";
    int len=strlen(np);

    fwrite(&len, sizeof(int), 1, helpfp);
    if (len)
        fwrite(np, len+1, 1, helpfp);

}

int main(int argc, char *argv[])
{
    char *cp;
    int HelpCount=0;
    long where;

    if (argc < 2)
        return -1;

    if ((helpfp=OpenHelpFile(argv[1], "r+b"))==NULL)
        return -1;

    *hline='\0';
    while (*hline != '<')
        {
        if (GetHelpLine(hline)==NULL)
            {
            fclose(helpfp);
            return -1;
            }

        }

    while (*hline=='<')
        {
        if (strncmp(hline, "<end>", 5)==0)
            break;

        /* Parse the help window's text name */
        if ((cp=strchr(hline, '>')) != NULL)
            {
            ThisHelp=calloc(1, sizeof(struct helps));
            if (FirstHelp==NULL)
                FirstHelp=ThisHelp;

            *cp='\0';
            ThisHelp->hname=malloc(strlen(hline+1)+1);
            strcpy(ThisHelp->hname, hline+1);
            HelpFilePosition(&ThisHelp->hptr, &ThisHelp->bit);
            if (GetHelpLine(hline)==NULL)
                break;

            /* Build the help linked list entry */
            while (*hline=='[')
                {
                HelpFilePosition(&ThisHelp->hptr, &ThisHelp->bit);

                /* Parse a comment */
                if (strncmp(hline, "[*]", 3)==0)
                    {
                    ThisHelp->comment=malloc(strlen(hline+3)+1);
                    strcpy(ThisHelp->comment, hline+3);
                    if (GetHelpLine(hline)==NULL)
                        break;

                    continue;
                    }

                /* Parse the <<prev button pointer */
                if (strncmp(hline, "[<<]", 4)==0)
                    {
                    char *cp=strchr(hline+4, '<');
                    if (cp != NULL)
                        {
                        char *cp1=strchr(cp, '>');
                        if (cp1 != NULL)
                            {
                            int len=(int) (cp1-cp);

                            ThisHelp->PrevName=calloc(1,len);
                            strncpy(ThisHelp->PrevName, cp+1,len-1);
                            }

                        }

                    if (GetHelpLine(hline)==NULL)
                        break;

                    continue;
                    }
                /* Parse the next>> button pointer */
                else if (strncmp(hline, "[>>]", 4)==0)
                    {
                    char *cp=strchr(hline+4, '<');

                    if (cp != NULL)
                        {
                        char *cp1=strchr(cp, '>');

                        if (cp1 != NULL)
                            {
                            int len=(int) (cp1-cp);

                            ThisHelp->NextName=calloc(1,len);
                            strncpy(ThisHelp->NextName,cp+1,len-1);
                            }

                        }

                    if (GetHelpLine(hline)==NULL)
                        break;

                    continue;
                    }
                else
                    break;

                }

            ThisHelp->hheight=0;
            ThisHelp->hwidth=0;
            ThisHelp->NextHelp=NULL;

            /* Append entry to the linked list */
            if (LastHelp != NULL)
                LastHelp->NextHelp=ThisHelp;

            LastHelp=ThisHelp;
            HelpCount++;
            }

        /* Move to the next <helpname> token */
        if (GetHelpLine(hline)==NULL)
            strcpy(hline, "<end>");

        while (*hline != '<')
            {
            ThisHelp->hwidth=max(ThisHelp->hwidth, HelpLength(hline));
            ThisHelp->hheight++;
            if (GetHelpLine(hline)==NULL)
                strcpy(hline, "<end>");

            }

        }

    /* Append the help structures to the file */
    fseek(helpfp, 0L, SEEK_END);
    where=ftell(helpfp);
    ThisHelp=FirstHelp;
    fwrite(&HelpCount, sizeof(HelpCount), 1, helpfp);
    while (ThisHelp != NULL)
        {
        ThisHelp->nexthlp=FindHelp(ThisHelp->NextName);
        ThisHelp->prevhlp=FindHelp(ThisHelp->PrevName);
        WriteText(ThisHelp->hname);
        WriteText(ThisHelp->comment);
        fwrite(&ThisHelp->hptr, sizeof(int)*5+sizeof(long), 1, helpfp);
        ThisHelp=ThisHelp->NextHelp;
	}

    fwrite(&where, sizeof(long), 1, helpfp);
    fclose(helpfp);
    return 0;

}
