#ifndef BISON_COND_TAB_H
# define BISON_COND_TAB_H

#ifndef YYSTYPE
typedef union
{
    struct cond_str str;
    LPWSTR    string;
    INT       value;
    comp_int  fn_comp_int;
    comp_str  fn_comp_str;
    comp_m1   fn_comp_m1;
    comp_m2   fn_comp_m2;
} yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
# define	COND_SPACE	257
# define	COND_EOF	258
# define	COND_OR	259
# define	COND_AND	260
# define	COND_NOT	261
# define	COND_LT	262
# define	COND_GT	263
# define	COND_EQ	264
# define	COND_LPAR	265
# define	COND_RPAR	266
# define	COND_TILDA	267
# define	COND_PERCENT	268
# define	COND_DOLLARS	269
# define	COND_QUESTION	270
# define	COND_AMPER	271
# define	COND_EXCLAM	272
# define	COND_IDENT	273
# define	COND_NUMBER	274
# define	COND_LITER	275
# define	COND_ERROR	276


#endif /* not BISON_COND_TAB_H */
