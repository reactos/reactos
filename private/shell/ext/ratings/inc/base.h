/*****************************************************************/ 
/**				Microsoft Windows for Workgroups				**/
/**			Copyright (C) Microsoft Corp., 1991-1992			**/
/*****************************************************************/ 

/*
    base.h
    Universal base class for error cascading and debugging information

    FILE HISTORY
	beng	    09-Jul-1990     created
	beng	    17-Jul-1990     added standard comment header to BASE
	beng	    31-Jul-1991     added FORWARDING_BASE
	rustanl     11-Sep-1991     Added DECLARE_OUTLINE_NEWBASE,
				    DECLARE_MI_NEWBASE, DEFINE_MI2_NEWBASE,
				    DEFINE_MI3_NEWBASE, and DEFINE_MI4_NEWBASE
	gregj		22-Mar-1993		Ported to Chicago environment
*/


#ifndef _BASE_HXX_
#define _BASE_HXX_

/*************************************************************************

    NAME:	BASE (base)

    SYNOPSIS:	Universal base object, root of every class.
		It contains universal error status and debugging
		support.

    INTERFACE:	ReportError()	- report an error on the object from
				  within the object.

		QueryError()	- return the current error state,
				  or 0 if no error outstanding.

		operator!()	- return TRUE if an error is outstanding.
				  Typically means that construction failed.

    CAVEATS:	This sort of error reporting is safe enough in a single-
		threaded system, but loses robustness when multiple threads
		access shared objects.	Use it for constructor-time error
		handling primarily.

    NOTES:	A class which inherits BASE through a private class should
		use the NEWBASE macro (q.v.) in its definition; otherwise
		its clients will lose the use of ! and QueryError.

    HISTORY:
	rustanl     07-Jun-1990     Created as part of LMOD
	beng	    09-Jul-1990     Gutted, removing LMOD methods
	beng	    17-Jul-1990     Added USHORT error methods
	beng	    19-Oct-1990     Finally, removed BOOL error methods
	johnl	    14-Nov-1990     Changed QueryError to be a const method
	beng	    25-Jan-1991     Added the ! Boolean operator and NEWBASE
	beng	    31-Jul-1991     Made FORWARDING_BASE a friend
	gregj		22-Mar-1993		Ported to Chicago (removed excess baggage)

*************************************************************************/

class BASE
{
private:
    UINT _err;

protected:
    BASE() { _err = 0; }
    VOID    ReportError( WORD err ) { _err = err; }

public:
    UINT	QueryError() const { return _err; }
    BOOL    operator!() const  { return (_err != 0); }
};

#endif // _BASE_HXX_
