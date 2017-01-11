/* @(#)match.h	1.18 16/12/12 joerg */
/*
 * 27th March 1996. Added by Jan-Piet Mens for matching regular expressions
 *                  in paths.
 *
 * Conversions to make the code more portable May 2000 .. March 2004
 * Copyright (c) 2000-2016 J. Schilling
 */

#include <schily/fnmatch.h>

#ifdef	SORTING
#include <schily/limits.h>
#define	NOT_SORTED INT_MIN

#ifdef	MAX				/* May be defined in param.h */
#undef	MAX
#endif
#define	MAX(A, B)	(A) > (B) ? (A) : (B)
#endif

#define	EXCLUDE		0		/* Exclude file completely */
#define	I_HIDE		1		/* ISO9660/Rock Ridge hide */
#define	J_HIDE		2		/* Joliet hide */
#define	U_HIDE		3		/* UDF hide */
#define	H_HIDE		4		/* ISO9660 hidden bit set */

#ifdef	APPLE_HYB
#define	HFS_HIDE	5		/* HFS hide */
#define	MAX_MAT		6
#else
#define	MAX_MAT		5
#endif /* APPLE_HYB */

extern int	gen_add_match	__PR((char *fn, int n));
extern int	add_match	__PR((char *fn));
extern int	i_add_match	__PR((char *fn));
extern int	h_add_match	__PR((char *fn));
extern int	j_add_match	__PR((char *fn));
extern int	u_add_match	__PR((char *fn));
extern int	hfs_add_match	__PR((char *fn));
extern int	gen_matches	__PR((char *fn, int n));
extern void	gen_add_list	__PR((char *fn, int n));
extern int	add_list	__PR((char *fn));
extern int	i_add_list	__PR((char *fn));
extern int	h_add_list	__PR((char *fn));
extern int	j_add_list	__PR((char *fn));
extern int	u_add_list	__PR((char *fn));
extern int	hfs_add_list	__PR((char *fn));
extern int	gen_ishidden	__PR((int n));
extern void	gen_del_match	__PR((int n));

#ifdef SORTING
extern int	add_sort_match	__PR((char *fn, int val));
extern int	add_sort_list	__PR((const char *fn, void *valp,
					int *pac, char *const **pav,
					const char *opt));
extern int	sort_matches	__PR((char *fn, int val));
extern void	del_sort	__PR((void));
#endif /* SORTING */

extern int	match_igncase;

/*
 * The following are for compatiblity with the separate routines - the
 * main code should be changed to call the generic routines directly
 */

/* filenames to be excluded */
#define	matches(FN)	gen_matches((FN), EXCLUDE)

/* ISO9660/Rock Ridge filenames to be hidden */
#define	i_matches(FN)	gen_matches((FN), I_HIDE)
#define	i_ishidden()	gen_ishidden(I_HIDE)

/* Joliet filenames to be hidden */
#define	j_matches(FN)	gen_matches((FN), J_HIDE)
#define	j_ishidden()	gen_ishidden(J_HIDE)

/* UDF filenames to be hidden */
#define	u_matches(FN)	gen_matches((FN), U_HIDE)
#define	u_ishidden()	gen_ishidden(U_HIDE)

/* ISO9660 "hidden" files */
#define	h_matches(FN)	gen_matches((FN), H_HIDE)
#define	h_ishidden()	gen_ishidden(H_HIDE)

#ifdef APPLE_HYB
/* HFS filenames to be hidden */
#define	hfs_matches(FN)	  gen_matches((FN), HFS_HIDE)
#define	hfs_ishidden()	  gen_ishidden(HFS_HIDE)
#endif /* APPLE_HYB */
