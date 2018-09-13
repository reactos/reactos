/*****************************************************************/
/**                  Microsoft LAN Manager                      **/
/**            Copyright(c) Microsoft Corp., 1990               **/
/*****************************************************************/
/***    apistruc.h
 *
 *      This file contains the structure definitions used to pass parameters
 *      to the loadable APIs in the REDIR 1.5 project
 *
 *      CONTENTS        tr_packet
 *                      NetWkstaSetUIDStruc
 *                      NetWkstaLogonStruc
 *                      NetWkstaReLogonStruc
 *                      NetSpecialSMBStruc
 *                      NetRemoteCopyStruc
 *                      NetMessageBufferSendStruc
 *                      NetMessageNameGetInfoStruc
 *                      NetServiceControlStruc
 *                      NetUseGetInfoStruc
 */

struct tr_packet {
        char FAR *      tr_name;        /* UNC Machine/Transaction name */
        char FAR *      tr_passwd;      /* password */
        char FAR *      tr_spbuf;       /* Send parameter buffer address */
        char FAR *      tr_sdbuf;       /* Send data buffer address */
        char FAR *      tr_rsbuf;       /* Receive set up buffer address */
        char FAR *      tr_rpbuf;       /* Receive parameter buffer address */
        char FAR *      tr_rdbuf;       /* Receive data buffer address */
        unsigned short  tr_splen;       /* Number of send parameter bytes */
        unsigned short  tr_sdlen;       /* Number of send data bytes */
        unsigned short  tr_rplen;       /* Number of receive parameter bytes */
        unsigned short  tr_rdlen;       /* Number of receive data bytes */
        unsigned short  tr_rslen;       /* Number of receive set up bytes */
        unsigned short  tr_flags;       /* Flags */
        unsigned long   tr_timeout;     /* Timeout */
        unsigned short  tr_resvd;       /* RESERVED (MBZ) */
        unsigned short  tr_sslen;       /* Number of send set up bytes */
};/* tr_packet */

/* data structure to simulate Transaction2 SMB.
 */
struct tr2_packet {
        char FAR *      tr2_name;       /* UNC Machine/Transaction name */
        char FAR *      tr2_passwd;     /* password */
        char FAR *      tr2_spbuf;      /* Send parameter buffer address */
        char FAR *      tr2_sdbuf;      /* Send data buffer address */
        char FAR *      tr2_rsbuf;      /* Receive set up buffer address */
        char FAR *      tr2_rpbuf;      /* Receive parameter buffer address */
        char FAR *      tr2_rdbuf;      /* Receive data buffer address */
        unsigned short  tr2_splen;      /* Number of send parameter bytes */
        unsigned short  tr2_sdlen;      /* Number of send data bytes */
        unsigned short  tr2_rplen;      /* Number of receive parameter bytes */
        unsigned short  tr2_rdlen;      /* Number of receive data bytes */
        unsigned short  tr2_rslen;      /* Number of receive set up bytes */
        unsigned short  tr2_flags;      /* Flags */
        unsigned long   tr2_timeout;    /* Timeout */
        unsigned short  tr2_resvd;      /* RESERVED (MBZ) */
        unsigned short  tr2_sslen;      /* Number of send set up bytes */
        unsigned short  tr2_trancode;   /* Transaction code for T2 SMB */
};/* tr2_packet */

struct NetWkstaSetUIDStruc {
        const char FAR *        su_username; /* username to log on/off */
        const char FAR *        su_password; /* password */
        const char FAR *        su_parms; /* OEM-specific parameter string */
};/* NetWkstaSetUIDStruc */

struct NetWkstaLogonStruc {
        char FAR *      ln_username;    /* new user name */
        char FAR *      ln_password;    /* new password */
        char FAR *      ln_parms;       /* OEM-specific parameter string */
        long FAR *      ln_uid;         /* UID returned here */
        char FAR *      ln_buffer;      /* buffer for passkey */
        unsigned short  ln_buflen;      /* length of passkey buffer */
};/* NetWkstaLogonStruc */

struct NetWkstaReLogonStruc {
        char FAR *      rl_username;    /* user name to re-log on */
        char FAR *      rl_password;    /* password to use in re-log on */
        char FAR *      rl_parms;       /* OEM-specific parameters */
        char FAR *      rl_buffer;      /* passkey buffer */
        unsigned short  rl_buflen;      /* length of passkey */
};/* NetWkstaReLogonStruc */

struct NetSpecialSMBStruc {
        char FAR *      sp_uncname;     /* UNC session name for SMB */
        char FAR *      sp_reqbuf;      /* Send SMB request buffer */
        unsigned short  sp_reqlen;      /* Length of send buffer */
        char FAR *      sp_rspbuf;      /* Receive SMB response buffer */
        unsigned short  sp_rsplen;      /* Length of receive buffer */
};/* NetSpecialSMBStruc */

struct NetRemoteCopyStruc {
        char FAR *      sourcepath;     /* ASCIIZ fully specified source path */
        char FAR *      destpath;       /* ASCIIZ fully specified dest path */
        char FAR *      sourcepass;     /* password for source path (NULL for default) */
        char FAR *      destpass;       /* password for dest path (NULL for default) */
        unsigned short  openflags;      /* flags for open of destpath */
        unsigned short  copyflags;      /* flags to control the copy */
        char FAR *      buf;            /* buffer to return error text in */
        unsigned short  buflen;         /* size of buffer on call */
};/* NetRemoteCopyStruc */


struct NetMessageBufferSendStruc {
    char FAR *          NMBSS_NetName;  /* asciz net name. */
    char FAR *          NMBSS_Buffer;   /* pointer to buffer. */
    unsigned int        NMBSS_BufSize;  /* size of buffer. */

}; /* NetMessageBufferSendStruc */

struct NetMessageNameGetInfoStruc {
    const char FAR *    NMNGIS_NetName; /* ASCIZ net name */
    char FAR *          NMNGIS_Buffer;  /* Pointer to buffer */
    unsigned int        NMNGIS_BufSize; /* Buffer size */
}; /* NetMessageNameGetInfoStruc */

struct NetServiceControlStruc {
    char FAR *          NSCS_Service;   /* Service name */
    unsigned short      NSCS_BufLen;    /* Buffer length */
    char FAR *          NSCS_BufferAddr;/* Buffer address */
};      /* NetServiceControlStruc */

struct NetUseGetInfoStruc {
        const char FAR* NUGI_usename;   /* ASCIZ redirected device name */
        short           NUGI_level;     /* level of info */
        char FAR*       NUGI_buffer;    /* buffer for returned info */
        unsigned short  NUGI_buflen;    /* size of buffer */
}; /* NetUseGetInfoStruc */

struct  DosWriteMailslotStruct {
    unsigned long DWMS_Timeout;         /* Timeout value of search */
    const char FAR *DWMS_Buffer;        /* Buffer address for mailslot write*/
}; /* DosWriteMailslotStruct */

struct  NetServerEnum2Struct {
    short          NSE_level;   /* level of information to be returned */
    char FAR      *NSE_buf;     /* buffer to contain returned info */
    unsigned short NSE_buflen;  /* number of bytes available in buffer */
    unsigned long  NSE_type;    /* bitmask of types to find */
    char FAR      *NSE_domain;  /* return servers in this domain */
}; /* NetServerEnum2Struct */

struct I_CDNames {
    char FAR      *CDN_pszComputer;
    char FAR      *CDN_pszPrimaryDomain;
    char FAR      *CDN_pszLogonDomain;
}; /* I_CDNames */


