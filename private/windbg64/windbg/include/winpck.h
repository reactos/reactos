/*++ BUILD Version: 0001    // Increment this if a change has global effects
 * WINPCK.H
 *
 * Description of callback packets.
 *
 * Module History:
 *  10-Jan-91   [mannyv]    Adapted it from CVPACK
 *  26-Dec-90   [mannyv]    Created it.
 */


/*
 * Packet header.  All packets contain this information at the beginning
 */

typedef struct
{
    short       iType;      // packet type
    short       cb;         // number of bytes in packet (include header)

} PCKHDR;


/*
 * Packet types (values for iType field in header)
 */

enum
{
    QW_VERSION=0,       // reports version of app
    QW_END,             // signals end of execution of app
    QW_ERROR,           // signals error
    QW_WERROR,          // signals windows error
    QW_STATUS           // reports progress
};


/*
 * Version packet
 */

typedef struct
{
    PCKHDR      hdr;

    char far *  pszTitle;
    char far *  pszCopyright;
    short       Major;
    short       Minor;
    short       Internal;

} PCKVER;


/*
 * Error packet.  Note: the string pointed to by pszError in the
 * packet is only valid during the callback.  The callback function
 * should copy the string if it needs it later.
 *
 * This same packe is used for both QW_ERROR and QW_WERROR packets.
 */

typedef struct
{
    PCKHDR      hdr;

    char far *  pszError;

} PCKERR;


/*
 * End packet
 */

typedef struct
{
    PCKHDR      hdr;

    short       RetCode;

} PCKEND;


/*
 * Proress report packet.
 */

typedef struct
{
    PCKHDR      hdr;

    char far *  pszMsg;

} PCKSTS;
