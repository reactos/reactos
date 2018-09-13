/*      File: D:\WACKER\tdll\features.h (Created: 24-Aug-1994)
 *
 *      Copyright 1994, 1998 by Hilgraeve Inc. -- Monroe, MI
 *      All rights reserved
 *
 *      $Revision: 8 $
 *      $Date: 9/24/99 5:01p $
 */


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 *                           R E A D   M E
 *
 * This file is a replacement for the INC.H file that was used in HAWIN
 * and HA/5.  It controls optional features that may or may not be built
 * into this product.  This file CANNOT have anything except defines in
 * it.  It is for control and configuration only.  Violate this rule at
 * your peril.  (Please ?)
 *
 *=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 *                           R E A D   M E
 *
 * This file has been organized into sections based on language.  To find
 * which features are enabled search for the language you are building.
 *
 * The following section contains descriptions of the settings currently
 * available for each language.
 *
 * The end of the file contains a series of test to verify that required
 * settings have been set.
 *
 *=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/


#if !defined(FEATURES_H_INCLUDED)
#define FEATURES_H_INCLUDED

#if defined(EXTENDED_FEATURES)
#define INCL_ZMODEM_CRASH_RECOVERY
#define INCL_REDIAL_ON_BUSY
#define INCL_USE_TERMINAL_FONT
#define INCL_SPINNING_GLOBE
#define INCL_PRIVATE_EDITION_BANNER
#define USE_PRIVATE_EDITION_3_BANNER
#define INCL_WINSOCK
#define INCL_USER_DEFINED_BACKSPACE_AND_TELNET_TERMINAL_ID
#define INCL_CALL_ANSWERING
#define INCL_DEFAULT_TELNET_APP
#define INCL_VT100COLORS
#define INCL_EXIT_ON_DISCONNECT
#define INCL_VT220                              // Added 20-Jan-98. rde
#if defined(INCL_VT220)                         // The 320 requires the 220 be defined.
//#define INCL_VT320                            // Added 24-Jan-98. rde
#endif

// A customer specific version. Added 16 Feb 98. rde
//#define INCL_ULTC_VERSION                        

//Private Edition 4 features
#define INCL_TERMINAL_SIZE_AND_COLORS
#define INCL_KEY_MACROS
#define INCL_TERMINAL_CLEAR
#define INCL_PRINT_PASSTHROUGH
//#define INCL_USE_HTML_HELP //removed due to a requirement to redistribute a 404k support program
#define INCL_NAG_SCREEN

#endif

#if defined(NT_EDITION)
#define INCL_ZMODEM_CRASH_RECOVERY
#define INCL_REDIAL_ON_BUSY
#define INCL_USE_TERMINAL_FONT
#define INCL_SPINNING_GLOBE
#define INCL_PRIVATE_EDITION_BANNER 
#define USE_PRIVATE_EDITION_3_BANNER
#define INCL_WINSOCK
#define INCL_USER_DEFINED_BACKSPACE_AND_TELNET_TERMINAL_ID
#define INCL_CALL_ANSWERING
#define INCL_DEFAULT_TELNET_APP
//mpt:08-22-97 added HTML help for Microsoft's version
#define INCL_USE_HTML_HELP
//mpt:04-29-98 added new Printing Common Dialogs for Microsoft
#define INCL_USE_NEWPRINTDLG
//mpt:09-24-99 added new browse dialog for Microsoft
#define INCL_USE_NEWFOLDERDLG
#define INCL_64BIT
#endif


/*
 * Minitel and Prestel terminals are now included standard
 */
#define INCL_MINITEL
#define INCL_VIEWDATA

/*
 * This feature is used in the FAR EAST version.  It provides code to
 * support an optional character translation DLL.  This DLL is called to
 * translate the data stream on both input and output.  It does not
 * translate the underlying character values (at the present time), only
 * the encoding method.  The initial version of this will only translate
 * between JIS and Shift-JIS.  For commercial release, additional operations
 * such as JIS escape recovery, UNICODE, and EUC encoding can be added just
 * by changing the DLL.  In fact, the new DLL can be offered as an upgrade
 * to the lower version of the product.
 *
 * #define CHARACTER_TRANSLATION
 */

/*
 * These next feature sets are used to control the character width that we
 * are compiling for.  There are 3 disjoint choices.  Only one can be selected.
 * One MUST be selected.  We will have code in here to cause noise if none or
 * more than one is selected.
 *
 * The 3 choices are:
 *
 *      CHAR_NARROW
 *      This is what we use for the U.S. and European versions.  It means all
 *      characters are a single "byte" and each one takes up 8 bits.
 *
 *      CHAR_WIDE
 *      This is what we will use for the UNICODE version if and when it ever
 *      decides to become an option.  It means that all characters are a single
 *      "byte" and each one takes up 16 (for now) bits.
 *
 *      CHAR_MIXED
 *      This is what we use for the FAR EAST (DBCS) version.  It means that the
 *      characters may take up one or two  "bytes" and each one takes up
 *      8 or 16 bits.
 * #define  CHAR_NARROW
 * #define  CHAR_WIDE
 * #define  CHAR_MIXED
 */

#if defined(USA)
    #define  CHAR_NARROW

#elif defined(JAPANESE)
	#define FAR_EAST
	#define INCL_VT100J
	#define INCL_ANSIW
    #define CHAR_MIXED
    #define CHARACTER_TRANSLATION

#endif

/* ************* THIS SECTION MUST BE AT THE END OF THE FILE*************
 *
 * It tests to see if required settings have been set for the current build
 *
 */
#if 0
#if !defined(CHAR_NARROW) && !defined(CHAR_WIDE) && !defined(CHAR_MIXED)
#error  Remember, one of these must be defined.
#endif

#if defined(CHAR_NARROW) && defined(CHAR_WIDE)
#error  Remember, only one of these can be defined.
#endif

#if defined(CHAR_NARROW) && defined(CHAR_MIXED)
#error  Remember, only one of these can be defined.
#endif

#if defined(CHAR_WIDE) && defined(CHAR_MIXED)
#error  Remember, only one of these can be defined.
#endif
#endif //0
#endif
