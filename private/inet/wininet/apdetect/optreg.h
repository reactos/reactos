//--------------------------------------------------------------------------------
// Copyright (C) Micorosoft Confidential 1997
// Author: RameshV
// Description: Option related registry handling -- common between NT and VxD
//--------------------------------------------------------------------------------
#ifndef  OPTREG_H
#define  OPTREG_H
//--------------------------------------------------------------------------------
// Exported functions: Caller must take locks and any lists accessed
//--------------------------------------------------------------------------------

POPTION                                           // option from which more appends can occur
DhcpAppendSendOptions(                            // append all configured options
    IN OUT  PDHCP_CONTEXT          DhcpContext,   // this is the context to append for
    IN      PLIST_ENTRY            SendOptionsList,
    IN      LPBYTE                 ClassName,     // current class
    IN      DWORD                  ClassLen,      // len of above in bytes
    IN      LPBYTE                 BufStart,      // start of buffer
    IN      LPBYTE                 BufEnd,        // how far can we go in this buffer
    IN OUT  LPBYTE                 SentOptions,   // BoolArray[OPTION_END+1] to avoid repeating options
    IN OUT  LPBYTE                 VSentOptions,  // to avoid repeating vendor specific options
    IN OUT  LPBYTE                 VendorOpt,     // Buffer[OPTION_END+1] Holding Vendor specific options
    OUT     LPDWORD                VendorOptLen   // the # of bytes filled into that
);

DWORD                                             // status
DhcpDestroyOptionsList(                           // destroy a list of options, freeing up memory
    IN OUT  PLIST_ENTRY            OptionsList,   // this is the list of options to destroy
    IN      PLIST_ENTRY            ClassesList    // this is where to remove classes off
);

DWORD                                             // win32 status
DhcpClearAllOptions(                              // remove all turds from off registry
    IN OUT  PDHCP_CONTEXT          DhcpContext    // the context to clear for
);


POPTION                                           // buffer after filling option
DhcpAppendClassIdOption(                          // fill class id if exists
    IN OUT  PDHCP_CONTEXT          DhcpContext,   // the context to fillfor
    OUT     LPBYTE                 BufStart,      // start of message buffer
    IN      LPBYTE                 BufEnd         // end of message buffer
);


#endif OPTREG_H
