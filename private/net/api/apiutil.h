/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

/* NOTE:  The following definitions already exist in NETCONS.H
 *	  and should be removed from here at some point in the future.
 *	  We are not doing it now because it would make APIUTIL.H
 *	  dependent upon NETCONS.H, and thus requires editing a
 *	  number of source files.
 *
 *	  -- DannyGl, 29 Mar 1990
 */
#ifndef API_RET_TYPE
#define API_RET_TYPE unsigned
#endif

#ifndef API_FUNCTION
#define API_FUNCTION API_RET_TYPE far pascal
#endif

/*
 * The below definitions of time_t, struct tm are as in <time.h>.
 * If <time.h> has already been included they will be ignored.
 */
#ifndef _TIME_T_DEFINED
typedef long time_t;		/* time value */
#define _TIME_T_DEFINED		/* avoid multiple def's of time_t */
#endif

#ifndef _TM_DEFINED
struct tm {
    int tm_sec;		/* seconds after the minute - [0,59] */
    int tm_min;		/* minutes after the hour - [0,59] */
    int tm_hour;	/* hours since midnight - [0,23] */
    int tm_mday;	/* day of the month - [1,31] */
    int tm_mon;		/* months since January - [0,11] */
    int tm_year;	/* years since 1900 */
    int tm_wday;	/* days since Sunday - [0,6] */
    int tm_yday;	/* days since January 1 - [0,365] */
    int tm_isdst;	/* daylight savings time flag */
    };
#define _TM_DEFINED
#endif


API_RET_TYPE far ChangeLoggedonPassword(char far *, char far *);

int far net_gmtime( time_t far *, struct tm far * );
unsigned long far time_now( void );

/* the following block of functions now live in \NET\API\UTIL */
int far cdecl make_lanman_filename(const char far *, char far *);
int far cdecl i_make_lanman_filename(const char far *, char far *, const char far *);
int far cdecl make_lanman_filename_x(const char far *, const char far *, const char far *, char far *);
int far cdecl i_make_lanman_filename_x(const char far *, const char far *, const char far *, char far *, const char far *);
int far cdecl rdr_open(int far *);
API_FUNCTION nb_open_r_2(unsigned short far *, unsigned short far *);
API_FUNCTION nb_close_2(unsigned short far *, unsigned short);
int far cdecl nb_open_r(unsigned short far *);
int far cdecl nb_close(unsigned short);
int far cdecl rdr_get_callgate(unsigned long far *);
API_RET_TYPE far cdecl isWkstaStarted( void );
API_RET_TYPE far cdecl get_wseg_ptr( char far * far * );
API_RET_TYPE far cdecl free_wseg_ptr( char far * );

#define FPF_SEG(fp) (*((unsigned far *)&(fp) + 1))
#define FPF_OFF(fp) (*((unsigned far *)&(fp)))


#define BUFCHECK_READ(buf,len) \
	{ \
		volatile char bufcheck_temp; \
		if (len != 0) \
			bufcheck_temp = buf[0] + buf[len-1]; \
	}

#define BUFCHECK_WRITE(buf,len) \
	{ \
		if (len != 0) \
		{ \
			buf[0] = 0; \
			buf[len-1] = 0; \
		} \
	}

#define BUFCHECK_WRITE_P(buf,len) \
	{ \
		volatile char bufcheck_temp_1, bufcheck_temp_2; \
		if (len != 0) \
		{ \
			bufcheck_temp_1 = buf[0]; \
			bufcheck_temp_2 = buf[len-1]; \
			buf[0] = 0; \
			buf[len-1] = 0; \
			buf[0] = bufcheck_temp_1; \
			buf[len-1] = bufcheck_temp_2; \
		} \
	}
