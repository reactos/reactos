/* linelist.c
 *
 * Copyright (c) 1996-2001 Mike Gleason, NCEMRSoft.
 * All rights reserved.
 *
 */

#include "syshdrs.h"

/* Dynamically make a copy of a string. */
char *
StrDup(const char *buf)
{
	char *cp;
	size_t len;

	if (buf == NULL)
		return (NULL);

	len = strlen(buf) + 1;
	cp = (char *) malloc(len);
	if (cp != NULL)
		(void) memcpy(cp, buf, len);
	return (cp);
}	/* StrDup */



/* Disposes each node of a LineList.  Does a few extra things
 * so the disposed memory won't be very useful after it is freed.
 */
void
DisposeLineListContents(LineListPtr list)
{
	LinePtr lp, lp2;
	
	for (lp = list->first; lp != NULL; ) {
		lp2 = lp;
		lp = lp->next;
		if (lp2->line != NULL) {
			lp2->line[0] = '\0';
			free(lp2->line);
		}
		free(lp2);
	}
	/* Same as InitLineList. */
	(void) memset(list, 0, sizeof(LineList));
}	/* DisposeLineListContents */




void
InitLineList(LineListPtr list)
{
	(void) memset(list, 0, sizeof(LineList));
}	/* InitLineList */




LinePtr
RemoveLine(LineListPtr list, LinePtr killMe)
{
	LinePtr nextLine, prevLine;
	
	nextLine = killMe->next;	
	prevLine = killMe->prev;	
	if (killMe->line != NULL) {
		killMe->line[0] = '\0';		/* Make it useless just in case. */
		free(killMe->line);
	}

	if (list->first == killMe)
		list->first = nextLine;
	if (list->last == killMe)
		list->last = prevLine;

	if (nextLine != NULL)
		nextLine->prev = prevLine;
	if (prevLine != NULL)
		prevLine->next = nextLine;

	free(killMe);	
	list->nLines--;
	return (nextLine);
}	/* RemoveLine */




/* Adds a string to the LineList specified. */
LinePtr
AddLine(LineListPtr list, const char *buf1)
{
	LinePtr lp;
	char *buf;
	
	lp = (LinePtr) malloc(sizeof(Line));
	if (lp != NULL) {
		buf = StrDup(buf1);
		if (buf == NULL) {
			free(lp);
			lp = NULL;
		} else {
			lp->line = buf;
			lp->next = NULL;
			if (list->first == NULL) {
				list->first = list->last = lp;
				lp->prev = NULL;
				list->nLines = 1;
			} else {
				lp->prev = list->last;
				list->last->next = lp;
				list->last = lp;
				list->nLines++;
			}
		}
	}
	return lp;
}	/* AddLine */




int
CopyLineList(LineListPtr dst, LineListPtr src)
{
	LinePtr lp, lp2;
	
	InitLineList(dst);
	for (lp = src->first; lp != NULL; ) {
		lp2 = lp;
		lp = lp->next;
		if (lp2->line != NULL) {
			if (AddLine(dst, lp2->line) == NULL) {
				DisposeLineListContents(dst);
				return (-1);
			}
		}
	}
	return (0);
}	/* CopyLineList */




/* Disposes each node of a FileInfoList.  Does a few extra things
 * so the disposed memory won't be very useful after it is freed.
 */
void
DisposeFileInfoListContents(FileInfoListPtr list)
{
	FileInfoPtr lp, lp2;
	
	for (lp = list->first; lp != NULL; ) {
		lp2 = lp;
		lp = lp->next;
		if (lp2->relname != NULL) {
			lp2->relname[0] = '\0';
			free(lp2->relname);
		}
		if (lp2->lname != NULL) {
			lp2->lname[0] = '\0';
			free(lp2->lname);
		}
		if (lp2->rname != NULL) {
			lp2->rname[0] = '\0';
			free(lp2->rname);
		}
		if (lp2->rlinkto != NULL) {
			lp2->rlinkto[0] = '\0';
			free(lp2->rlinkto);
		}
		if (lp2->plug != NULL) {
			lp2->plug[0] = '\0';
			free(lp2->plug);
		}
		free(lp2);
	}

	if (list->vec != NULL)
		free(list->vec);

	/* Same as InitFileInfoList. */
	(void) memset(list, 0, sizeof(FileInfoList));
}	/* DisposeFileInfoListContents */




void
InitFileInfoList(FileInfoListPtr list)
{
	(void) memset(list, 0, sizeof(FileInfoList));
}	/* InitFileInfoList */




static int
TimeCmp(const void *a, const void *b)
{
	FileInfoPtr *fipa, *fipb;

	fipa = (FileInfoPtr *) a;
	fipb = (FileInfoPtr *) b;
	if ((**fipb).mdtm == (**fipa).mdtm)
		return (0);
	else if ((**fipb).mdtm < (**fipa).mdtm)
		return (-1);
	return (1);
}	/* TimeCmp */




static int
ReverseTimeCmp(const void *a, const void *b)
{
	FileInfoPtr *fipa, *fipb;

	fipa = (FileInfoPtr *) a;
	fipb = (FileInfoPtr *) b;
	if ((**fipa).mdtm == (**fipb).mdtm)
		return (0);
	else if ((**fipa).mdtm < (**fipb).mdtm)
		return (-1);
	return (1);
}	/* ReverseTimeCmp */




static int
SizeCmp(const void *a, const void *b)
{
	FileInfoPtr *fipa, *fipb;

	fipa = (FileInfoPtr *) a;
	fipb = (FileInfoPtr *) b;
	if ((**fipb).size == (**fipa).size)
		return (0);
	else if ((**fipb).size < (**fipa).size)
		return (-1);
	return (1);
}	/* SizeCmp */




static int
ReverseSizeCmp(const void *a, const void *b)
{
	FileInfoPtr *fipa, *fipb;

	fipa = (FileInfoPtr *) a;
	fipb = (FileInfoPtr *) b;
	if ((**fipa).size == (**fipb).size)
		return (0);
	else if ((**fipa).size < (**fipb).size)
		return (-1);
	return (1);
}	/* ReverseSizeCmp */




static int
ReverseNameCmp(const void *a, const void *b)
{
	FileInfoPtr *fipa, *fipb;

	fipa = (FileInfoPtr *) a;
	fipb = (FileInfoPtr *) b;
#ifdef HAVE_SETLOCALE
	return (strcoll((**fipb).relname, (**fipa).relname));
#else
	return (strcmp((**fipb).relname, (**fipa).relname));
#endif
}	/* ReverseNameCmp */




static int
NameCmp(const void *a, const void *b)
{
	FileInfoPtr *fipa, *fipb;

	fipa = (FileInfoPtr *) a;
	fipb = (FileInfoPtr *) b;
#ifdef HAVE_SETLOCALE
	return (strcoll((**fipa).relname, (**fipb).relname));
#else
	return (strcmp((**fipa).relname, (**fipb).relname));
#endif
}	/* NameCmp */




static int
BreadthFirstCmp(const void *a, const void *b)
{
	FileInfoPtr *fipa, *fipb;
	char *cp, *cpa, *cpb;
	int depth, deptha, depthb;
	int c;

	fipa = (FileInfoPtr *) a;
	fipb = (FileInfoPtr *) b;

	cpa = (**fipa).relname;
	cpb = (**fipb).relname;

	for (cp = cpa, depth = 0;;) {
		c = *cp++;
		if (c == '\0')
			break;
		if ((c == '/') || (c == '\\')) {
			depth++;
		}
	}
	deptha = depth;

	for (cp = cpb, depth = 0;;) {
		c = *cp++;
		if (c == '\0')
			break;
		if ((c == '/') || (c == '\\')) {
			depth++;
		}
	}
	depthb = depth;

	if (deptha < depthb)
		return (-1);
	else if (deptha > depthb)
		return (1);

#ifdef HAVE_SETLOCALE
	return (strcoll(cpa, cpb));
#else
	return (strcmp(cpa, cpb));
#endif
}	/* BreadthFirstCmp */




void
SortFileInfoList(FileInfoListPtr list, int sortKey, int sortOrder)
{
	FileInfoVec fiv;
	FileInfoPtr fip;
	int i, j, n, n2;

	fiv = list->vec;
	if (fiv == NULL)
		return;

	if (list->sortKey == sortKey) {
		if (list->sortOrder == sortOrder)
			return;		/* Already sorted they you want. */

		/* Reverse the sort. */
		n = list->nFileInfos;
		if (n > 1) {
			n2 = n / 2;
			for (i=0; i<n2; i++) {
				j = n - i - 1;
				fip = fiv[i];
				fiv[i] = fiv[j];
				fiv[j] = fip;
			}
		}

		list->sortOrder = sortOrder;
	} else if ((sortKey == 'n') && (sortOrder == 'a')) {
		qsort(fiv, (size_t) list->nFileInfos, sizeof(FileInfoPtr),
			NameCmp);
		list->sortKey = sortKey;
		list->sortOrder = sortOrder;
	} else if ((sortKey == 'n') && (sortOrder == 'd')) {
		qsort(fiv, (size_t) list->nFileInfos, sizeof(FileInfoPtr),
			ReverseNameCmp);
		list->sortKey = sortKey;
		list->sortOrder = sortOrder;
	} else if ((sortKey == 't') && (sortOrder == 'a')) {
		qsort(fiv, (size_t) list->nFileInfos, sizeof(FileInfoPtr),
			TimeCmp);
		list->sortKey = sortKey;
		list->sortOrder = sortOrder;
	} else if ((sortKey == 't') && (sortOrder == 'd')) {
		qsort(fiv, (size_t) list->nFileInfos, sizeof(FileInfoPtr),
			ReverseTimeCmp);
		list->sortKey = sortKey;
		list->sortOrder = sortOrder;
	} else if ((sortKey == 's') && (sortOrder == 'a')) {
		qsort(fiv, (size_t) list->nFileInfos, sizeof(FileInfoPtr),
			SizeCmp);
		list->sortKey = sortKey;
		list->sortOrder = sortOrder;
	} else if ((sortKey == 's') && (sortOrder == 'd')) {
		qsort(fiv, (size_t) list->nFileInfos, sizeof(FileInfoPtr),
			ReverseSizeCmp);
		list->sortKey = sortKey;
		list->sortOrder = sortOrder;
	} else if (sortKey == 'b') {
		/* This is different from the rest. */
		list->sortKey = sortKey;
		list->sortOrder = sortOrder;
		qsort(fiv, (size_t) list->nFileInfos, sizeof(FileInfoPtr),
			BreadthFirstCmp);
	}
}	/* SortFileInfoList */




void
VectorizeFileInfoList(FileInfoListPtr list)
{
	FileInfoVec fiv;
	FileInfoPtr fip;
	int i;

	fiv = (FileInfoVec) calloc((size_t) (list->nFileInfos + 1), sizeof(FileInfoPtr));
	if (fiv != (FileInfoVec) 0) {
		for (i = 0, fip = list->first; fip != NULL; fip = fip->next, i++)
			fiv[i] = fip;
		list->vec = fiv;
	}
}	/* VectorizeFileInfoList */




void
UnvectorizeFileInfoList(FileInfoListPtr list)
{
	FileInfoVec fiv;
	FileInfoPtr fip;
	int i, n;

	fiv = list->vec;
	if (fiv != (FileInfoVec) 0) {
		list->first = fiv[0];
		n = list->nFileInfos;
		if (n > 0) {
			list->last = fiv[n - 1];
			fip = fiv[0];
			fip->prev = NULL;
			fip->next = fiv[1];
			for (i = 1; i < n; i++) {
				fip = fiv[i];
				fip->prev = fiv[i - 1];
				fip->next = fiv[i + 1];
			}
		}
		free(fiv);
		list->vec = (FileInfoVec) 0;
	}
}	/* UnvectorizeFileInfoList */




void
InitFileInfo(FileInfoPtr fip)
{
	(void) memset(fip, 0, sizeof(FileInfo));
	fip->type = '-';
	fip->size = kSizeUnknown;
	fip->mdtm = kModTimeUnknown;
}	/* InitFileInfoList */




FileInfoPtr
RemoveFileInfo(FileInfoListPtr list, FileInfoPtr killMe)
{
	FileInfoPtr nextFileInfo, prevFileInfo;
	
	nextFileInfo = killMe->next;	
	prevFileInfo = killMe->prev;	
	if (killMe->lname != NULL) {
		killMe->lname[0] = '\0';		/* Make it useless just in case. */
		free(killMe->lname);
	}
	if (killMe->relname != NULL) {
		killMe->relname[0] = '\0';
		free(killMe->relname);
	}
	if (killMe->rname != NULL) {
		killMe->rname[0] = '\0';
		free(killMe->rname);
	}
	if (killMe->rlinkto != NULL) {
		killMe->rlinkto[0] = '\0';
		free(killMe->rlinkto);
	}
	if (killMe->plug != NULL) {
		killMe->plug[0] = '\0';
		free(killMe->plug);
	}

	if (list->first == killMe)
		list->first = nextFileInfo;
	if (list->last == killMe)
		list->last = prevFileInfo;

	if (nextFileInfo != NULL)
		nextFileInfo->prev = prevFileInfo;
	if (prevFileInfo != NULL)
		prevFileInfo->next = nextFileInfo;

	free(killMe);	
	list->nFileInfos--;
	return (nextFileInfo);
}	/* RemoveFileInfo */




/* Adds a string to the FileInfoList specified. */
FileInfoPtr
AddFileInfo(FileInfoListPtr list, FileInfoPtr src)
{
	FileInfoPtr lp;
	
	lp = (FileInfoPtr) malloc(sizeof(FileInfo));
	if (lp != NULL) {
		(void) memcpy(lp, src, sizeof(FileInfo));
		lp->next = NULL;
		if (list->first == NULL) {
			list->first = list->last = lp;
			lp->prev = NULL;
			list->nFileInfos = 1;
		} else {
			lp->prev = list->last;
			list->last->next = lp;
			list->last = lp;
			list->nFileInfos++;
		}
	}
	return lp;
}	/* AddFileInfo */




int
ConcatFileInfoList(FileInfoListPtr dst, FileInfoListPtr src)
{
	FileInfoPtr lp, lp2;
	FileInfo newfi;
	
	for (lp = src->first; lp != NULL; lp = lp2) {
		lp2 = lp->next;
		newfi = *lp;
		newfi.relname = StrDup(lp->relname);
		newfi.lname = StrDup(lp->lname);
		newfi.rname = StrDup(lp->rname);
		newfi.rlinkto = StrDup(lp->rlinkto);
		newfi.plug = StrDup(lp->plug);
		if (AddFileInfo(dst, &newfi) == NULL)
			return (-1);
	}
	return (0);
}	/* ConcatFileInfoList */




int
ComputeRNames(FileInfoListPtr dst, const char *dstdir, int pflag, int nochop)
{
	FileInfoPtr lp, lp2;
	char *buf;
	char *cp;

	if (dstdir == NULL)
		dstdir = ".";

	for (lp = dst->first; lp != NULL; lp = lp2) {
		lp2 = lp->next;

		buf = NULL;
		if (nochop != 0) {
			if ((dstdir[0] != '\0') && (strcmp(dstdir, "."))) {
				if (Dynscat(&buf, dstdir, "/", lp->relname, 0) == NULL)
					goto memerr;

				if (pflag != 0) {
					/* Init lname to parent dir name of remote dir */
					cp = strrchr(dstdir, '/');
					if (cp == NULL)
						cp = strrchr(dstdir, '\\');
					if (cp != NULL) {
						if (Dynscat(&lp->lname, cp + 1, 0) == NULL)
							goto memerr;
						TVFSPathToLocalPath(lp->lname);
					}
				}
			} else {
				if (Dynscat(&buf, lp->relname, 0) == NULL)
					goto memerr;
			}
		} else {
			if ((dstdir[0] != '\0') && (strcmp(dstdir, "."))) {
				cp = strrchr(lp->relname, '/');
				if (cp == NULL)
					cp = strrchr(lp->relname, '\\');
				if (cp != NULL) {
					cp++;
				} else {
					cp = lp->relname;
				}
				if (Dynscat(&buf, dstdir, "/", cp, 0) == NULL)
					goto memerr;

				if (pflag != 0) {
					/* Init lname to parent dir name of remote dir */
					cp = strrchr(dstdir, '/');
					if (cp == NULL)
						cp = strrchr(dstdir, '\\');
					if (cp != NULL) {
						if (Dynscat(&lp->lname, cp + 1, 0) == NULL)
							goto memerr;
						TVFSPathToLocalPath(lp->lname);
					}
				}
			} else {
				cp = strrchr(lp->relname, '/');
				if (cp == NULL)
					cp = strrchr(lp->relname, '\\');
				if (cp != NULL) {
					cp++;
				} else {
					cp = lp->relname;
				}
				if (Dynscat(&buf, cp, 0) == NULL)
					goto memerr;
			}
		}
		lp->rname = buf;
		if (lp->rname == NULL) {
memerr:
			return (-1);
		}
		LocalPathToTVFSPath(lp->rname);
	}
	return (0);
}	/* ComputeRNames */




int
ComputeLNames(FileInfoListPtr dst, const char *srcdir, const char *dstdir, int nochop)
{
	FileInfoPtr lp, lp2;
	char *buf;
	char *cp;

	if (srcdir != NULL) {
		cp = strrchr(srcdir, '/');
		if (cp == NULL)
			cp = strrchr(srcdir, '\\');
		if (cp != NULL)
			srcdir = cp + 1;
	}
	if (dstdir == NULL)
		dstdir = ".";

	for (lp = dst->first; lp != NULL; lp = lp2) {
		lp2 = lp->next;

		buf = NULL;
		if (nochop != 0) {
			if ((dstdir[0] != '\0') && (strcmp(dstdir, "."))) {
				if (Dynscat(&buf, dstdir, "/", 0) == NULL)
					goto memerr;
			}
			if (lp->lname != NULL) {
				if (Dynscat(&buf, lp->lname, "/", 0) == NULL)
					goto memerr;
			} else if (srcdir != NULL) {
				if (Dynscat(&buf, srcdir, "/", 0) == NULL)
					goto memerr;
			}
			if (Dynscat(&buf, lp->relname, 0) == NULL)
				goto memerr;
		} else {
			if ((dstdir[0] != '\0') && (strcmp(dstdir, "."))) {
				cp = strrchr(lp->relname, '/');
				if (cp == NULL)
					cp = strrchr(lp->relname, '\\');
				if (cp == NULL) {
					cp = lp->relname;
				} else {
					cp++;
				}
				if (Dynscat(&buf, dstdir, "/", cp, 0) == NULL)
					goto memerr;
			} else {
				cp = strrchr(lp->relname, '/');
				if (cp == NULL)
					cp = strrchr(lp->relname, '\\');
				if (cp == NULL) {
					cp = lp->relname;
				} else {
					cp++;
				}
				if (Dynscat(&buf, cp, 0) == NULL)
					goto memerr;
			}
		}
		if (buf == NULL) {
memerr:
			return (-1);
		}
		if (lp->lname != NULL) {
			free(lp->lname);
			lp->lname = NULL;
		}
		lp->lname = buf;
		TVFSPathToLocalPath(lp->lname);
	}
	return (0);
}	/* ComputeLNames */




int
ConcatFileToFileInfoList(FileInfoListPtr dst, char *rfile)
{
	FileInfo newfi;

	InitFileInfo(&newfi);	/* Use defaults. */
	newfi.relname = StrDup(rfile);
	newfi.rname = NULL;
	newfi.lname = NULL;

	if (AddFileInfo(dst, &newfi) == NULL)
		return (-1);
	return (0);
}	/* ConcatFileToFileInfoList */




int
LineListToFileInfoList(LineListPtr src, FileInfoListPtr dst)
{
	LinePtr lp, lp2;

	InitFileInfoList(dst);
	for (lp = src->first; lp != NULL; lp = lp2) {
		lp2 = lp->next;
		if (ConcatFileToFileInfoList(dst, lp->line) < 0)
			return (-1);
	}
	return (0);
}	/* LineListToFileList */




int
LineToFileInfoList(LinePtr lp, FileInfoListPtr dst)
{
	InitFileInfoList(dst);
	if (ConcatFileToFileInfoList(dst, lp->line) < 0)
		return (-1);
	return (0);
}	/* LineToFileInfoList */

/* eof */
