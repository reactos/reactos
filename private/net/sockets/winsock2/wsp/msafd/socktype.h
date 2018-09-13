/*++

Copyright (c) 1992-1996 Microsoft Corporation

Module Name:

    SockType.h

Abstract:

    Contains type definitions for the WinSock DLL.

Author:

    David Treadwell (davidtr)    25-Feb-1992

Revision History:

--*/

#ifndef _SOCKTYPE_
#define _SOCKTYPE_

//
// SOCKET_STATE defines the various states a socket may have.  Note that
// not all states are valid for all types of sockets.
//

typedef enum _SOCKET_STATE {
    SocketStateOpen,
    SocketStateBound,
    SocketStateBoundSpecific,           // Datagram only
    SocketStateConnected,               // VC only
    SocketStateClosing
} SOCKET_STATE, *PSOCKET_STATE;

//
// WINSOCK_HELPER_DLL_INFO contains all the necessary information about
// a socket's helper DLL.
//

typedef struct _WINSOCK_HELPER_DLL_INFO {

    LIST_ENTRY HelperDllListEntry;
	LONG RefCount;
    HANDLE DllHandle;
    INT MinSockaddrLength;
    INT MaxSockaddrLength;
    INT MinTdiAddressLength;
    INT MaxTdiAddressLength;
    INT UseDelayedAcceptance;
    PWINSOCK_MAPPING Mapping;

    PWSH_OPEN_SOCKET WSHOpenSocket;
    PWSH_OPEN_SOCKET2 WSHOpenSocket2;
    PWSH_JOIN_LEAF WSHJoinLeaf;
    PWSH_NOTIFY WSHNotify;
    PWSH_GET_SOCKET_INFORMATION WSHGetSocketInformation;
    PWSH_SET_SOCKET_INFORMATION WSHSetSocketInformation;
    PWSH_GET_SOCKADDR_TYPE WSHGetSockaddrType;
    PWSH_GET_WILDCARD_SOCKADDR WSHGetWildcardSockaddr;
    PWSH_GET_BROADCAST_SOCKADDR WSHGetBroadcastSockaddr;
    PWSH_ADDRESS_TO_STRING WSHAddressToString;
    PWSH_STRING_TO_ADDRESS WSHStringToAddress;
    PWSH_IOCTL WSHIoctl;

    WCHAR   TransportName[1];

} WINSOCK_HELPER_DLL_INFO, *PWINSOCK_HELPER_DLL_INFO;

typedef struct _PROTOCOL_INFO_ENTRY {
    LIST_ENTRY          Link;
    WSAPROTOCOL_INFOW   Info;
} PROTOCOL_INFO_ENTRY, *PPROTOCOL_INFO_ENTRY;

typedef struct _SOCK_ASYNC_CONNECT_CONTEXT 
                    SOCK_ASYNC_CONNECT_CONTEXT,
                    *PSOCK_ASYNC_CONNECT_CONTEXT;


#ifdef _AFD_SAN_SWITCH_
typedef struct _SOCK_SAN_INFORMATION SOCK_SAN_INFORMATION, *PSOCK_SAN_INFORMATION;
#endif //_AFD_SAN_SWITCH_

//
// Part of socket information that needs to shared
// between processes.
//
typedef struct _SOCK_SHARED_INFO {
    SOCKET_STATE State;
    INT AddressFamily;
    INT SocketType;
    INT Protocol;
    INT LocalAddressLength;
    INT RemoteAddressLength;

    //
    // Socket options controlled by getsockopt(), setsockopt().
    //

    struct linger LingerInfo;
    ULONG SendTimeout;
    ULONG ReceiveTimeout;
    ULONG ReceiveBufferSize;
    ULONG SendBufferSize;
    struct {
        BOOLEAN Listening:1;
        BOOLEAN Broadcast:1;
        BOOLEAN Debug:1;
        BOOLEAN OobInline:1;
        BOOLEAN ReuseAddresses:1;
        BOOLEAN ExclusiveAddressUse:1;
        BOOLEAN NonBlocking:1;
        BOOLEAN DontUseWildcard:1;

        //
        // Shutdown state.
        //

        BOOLEAN ReceiveShutdown:1;
        BOOLEAN SendShutdown:1;

        BOOLEAN ConditionalAccept:1;

#ifdef _AFD_SAN_SWITCH_
		BOOLEAN IsSANSocket:1;	// for socket duplication
#endif
    };

    //
    // Snapshot of several parameters passed into WSPSocket() when
    // creating this socket. We need these when creating accept()ed
    // sockets.
    //

    DWORD CreationFlags;
    DWORD CatalogEntryId;
    DWORD ServiceFlags1;
    DWORD ProviderFlags;

    //
    // Group/QOS stuff.
    //

    GROUP GroupID;
    AFD_GROUP_TYPE GroupType;
    INT GroupPriority;

    //
    // Last error set on this socket, used to report the status of
    // non-blocking connects.
    //

    INT LastError;

    //
    // Info stored for WSAAsyncSelect().
    // (This sounds failry redicilous that someone can
    // share this info between processes, but it's been
    // that way since day one, so I won't risk changing it, VadimE).
    //

    union {
        HWND AsyncSelecthWnd;
        ULONGLONG AsyncSelectWnd64; // Hack for 64 bit compatibility
                                    // We shouldn't be passing this between
                                    // processes (see the comment above).
    };
    DWORD AsyncSelectSerialNumber;
    UINT AsyncSelectwMsg;
    LONG AsyncSelectlEvent;
    LONG DisabledAsyncSelectEvents;


} SOCK_SHARED_INFO, *PSOCK_SHARED_INFO;


//
// SOCKET_INFORMATION stores information for each socket opened by a
// process.  These structures are stored in the generic table
// SocketTable.  Different fields of this structure are filled in during
// different APIs.
//

typedef struct _SOCKET_INFORMATION {
    union {
		//
		// Caveat. The order of fields in the the structures of
		// union should match!!!
		// 
        WSHANDLE_CONTEXT    HContext;
        struct {
            ULONG  ReferenceCount;
            SOCKET Handle;
        };
    };
    union {
        ULONG   SharedInfo;
        SOCK_SHARED_INFO ;
    };
    //  Helper DLL info

    DWORD HelperDllNotificationEvents;
    PWINSOCK_HELPER_DLL_INFO HelperDll;
    PVOID HelperDllContext;

    PSOCKADDR LocalAddress;
    PSOCKADDR RemoteAddress;

    HANDLE TdiAddressHandle;
    HANDLE TdiConnectionHandle;

    PSOCK_ASYNC_CONNECT_CONTEXT AsyncConnectContext;

    //
    // Info stored for WSAEventSelect().
    //

    HANDLE EventSelectEventObject;
    LONG EventSelectlNetworkEvents;


    // A critical section to synchronize access to the socket.
    //

    CRITICAL_SECTION Lock;
#ifdef _AFD_SAN_SWITCH_
    PSOCK_SAN_INFORMATION   SanSocket;
	BOOL	DontTrySAN;			// TRUE if non-blocking connect thru SAN failed
#endif

} SOCKET_INFORMATION, *PSOCKET_INFORMATION;

//
// A typedef for blocking hook functions.
//

typedef
BOOL
(*PBLOCKING_HOOK) (
    VOID
    );

//
// Structures, etc. to support per-thread variable storage in this DLL.
//

#define MAXALIASES      35
#define MAXADDRS        35

#define HOSTDB_SIZE     (_MAX_PATH + 7)   // 7 == strlen("\\hosts") + 1

typedef struct _WINSOCK_TLS_DATA {
//	This functionality is now handled by WS2_32.DLL
//    BOOLEAN IsBlocking;	
    HANDLE EventHandle;
    SOCKET SocketHandle;
#ifdef _AFD_SAN_SWITCH_
    union {
        PAFD_SAN_ACCEPT_INFO AcceptInfo; // Info of socket accepted in
								    // the current thread for quering
								    // connect data from the helper
								    // DLL.
        DWORD_PTR           Context;
    };
	DWORD	UpcallLevel;
	LPWSAOVERLAPPED UpcallOvListHead;
	LPWSAOVERLAPPED UpcallOvListTail;
#else //_AFD_SAN_SWITCH_
    PAFD_ACCEPT_INFO AcceptInfo; // Info of socket accepted in
								// the current thread for quering
								// connect data from the helper
								// DLL.
#endif //_AFD_SAN_SWITCH_
    LONG PendingAPCCount;
    BOOLEAN IoCancelled;
#if DBG
    ULONG IndentLevel;
#endif
} WINSOCK_TLS_DATA, *PWINSOCK_TLS_DATA;

struct _SOCK_ASYNC_CONNECT_CONTEXT {
    PSOCKET_INFORMATION Socket;
#ifdef _AFD_SAN_SWITCH_
    union {
        PSOCK_ASYNC_CONNECT_CONTEXT Next;
        PSOCKET_INFORMATION RootSocket;
    };
#else //_AFD_SAN_SWITCH_
    PSOCKET_INFORMATION RootSocket;
#endif // AFD_SAN_SWITCH_
    IO_STATUS_BLOCK IoStatusBlock;
    ULONG   ConnectInfoLength;
    ULONG   IoctlCode;
#ifdef _AFD_SAN_SWITCH_
    AFD_SAN_CONNECT_JOIN_INFO ConnectInfo;
#else //_AFD_SAN_SWITCH_
    AFD_CONNECT_JOIN_INFO ConnectInfo;
#endif // AFD_SAN_SWITCH_
};

//
// Typedef for async completion routine.
//

typedef VOID (*PASYNC_COMPLETION_PROC) (
                    PVOID               Context,
                    PIO_STATUS_BLOCK    IoStatus
                    );

//
// Special context to terminate async thread.
//
#define ASYNC_TERMINATION_PROC ((PASYNC_COMPLETION_PROC)-1)

//
// Set the following to 1 to enable critical section debugging, even on
// retail builds (SET USER_C_FLAGS=/DDEBUG_LOCKS=1).
//

#if DBG
    #ifndef DEBUG_LOCKS
    #define DEBUG_LOCKS 1
    #endif
#else
    #ifndef DEBUG_LOCKS
    #define DEBUG_LOCKS 0
    #endif
#endif

#if DEBUG_LOCKS

#define MAX_RW_LOCK_DEBUG  256

typedef struct _SOCK_RW_LOCK_DEBUG_INFO {

    struct {
        LONG    Operation:8;
#define RW_LOCK_READ_ACQUIRE    1
#define RW_LOCK_READ_RELEASE    2
#define RW_LOCK_WRITE_ACQUIRE   3
#define RW_LOCK_WRITE_RELEASE   4

        LONG    Count:24;
    };
    HANDLE  ThreadId;
    PVOID   Caller;
    PVOID   CallersCaller;
} SOCK_RW_LOCK_DEBUG_INFO, *PSOCK_RW_LOCK_DEBUG_INFO;


#endif  // DEBUG_LOCKS


typedef struct SOCK_RW_LOCK {
    volatile LONG           ReaderCount;
    HANDLE                  WriterWaitEvent;
#if DBG
    HANDLE                  WriterId;   // from the thread's
                // ClientId->UniqueThread. This is not the same as the one
                // the one maintained by critical section as readers do
                // not set this field while owning critical section.
#endif
#if DEBUG_LOCKS
    PSOCK_RW_LOCK_DEBUG_INFO DebugInfo;
    LONG                     DebugInfoIdx;
#endif
    RTL_CRITICAL_SECTION    Lock;
} SOCK_RW_LOCK, *PSOCK_RW_LOCK;


#ifdef _AFD_SAN_SWITCH_

typedef struct _WSPEXTPROC_TABLE {
    LPFN_WSPREGISTERMEMORY      lpWSPRegisterMemory;
    LPFN_WSPDEREGISTERMEMORY    lpWSPDeregisterMemory;
    LPFN_WSPREGISTERRDMAMEMORY  lpWSPRegisterRdmaMemory;
    LPFN_WSPDEREGISTERRDMAMEMORY lpWSPDeregisterRdmaMemory;
    LPFN_WSPRDMAWRITE           lpWSPRdmaWrite;
    LPFN_WSPRDMAREAD            lpWSPRdmaRead;
} WSPEXTPROC_TABLE, *PWSPEXTPROC_TABLE;


typedef struct _SOCK_SAN_PROVIDER {
    LIST_ENTRY              Link;           // Link in the provider list
    LONG                    RefCount;       // Current reference count
    ULONG                   RdmaHandleSize; // Config parameter
    HINSTANCE               HInstance;      // Provider DLL handle
    DWORD                   CatId;          // Winsock catalog ID for provider
    GUID                    Guid;           // Provider's GUID
    WSPPROC_TABLE           SpiProcTable;   // WS SPI Entry points
    WSPEXTPROC_TABLE        SanExtProcTable;// WSDP extensions.
                                // Pre-allocated address info fields.
	DWORD					MaxRdmaSize;	// in number of bytes
#if DBG
    WCHAR                   DllName[MAX_PATH];
#endif
} SOCK_SAN_PROVIDER, *PSOCK_SAN_PROVIDER;

typedef struct _SOCK_SAN_SUBNET_MAP {
    ULONG       Address;    // IP address
    ULONG       Mask;       // IP address mask
    ULONG       Index;      // If index - for qiuck change validation.
    PSOCK_SAN_PROVIDER  Provider;
} SOCK_SAN_SUBNET_MAP, *PSOCK_SAN_SUBNET_MAP;


// values for Receiver's Mode (RsMode)
#define DISCOVERY_MODE				0	// don't change this to non-zero
#define SMALL_RECV_LARGE_RECV_MODE	1
#define LARGE_RECV_ONLY_MODE		2
#define	SMALL_RECV_ONLY_MODE		3
#define	INVALID_MODE				255

//#define	RDMA_CACHE					1
#define RDMA_CACHE_SIZE				8

#define CONNECTED 					1
#define NON_BLOCKING_IN_PROGRESS	2
#define BLOCKING_IN_PROGRESS		3


// different states for socket duplication
#define SUSPENDING_COMMUNICATION	1  // suspending all data transfers (source proc)
#define SOCK_MIGRATED				2  // sock now fully owned by some other proc
#define	COMM_SUSPENDED				3  // remote peer has requested suspension
#define IMPORTING_SOCK				4  // getting sock from source process

typedef struct _DuplicationContext {
	union {
		DWORD		ProviderReserved; // from WSAPROTOCL_INFOW (to create new sock)
		PSOCK_SAN_INFORMATION SwitchSocket;	// in case of same process
	};
	struct {
		BOOLEAN		ListeningSock:1;  	// TRUE when socket is listening
		BOOLEAN		SameProcess:1; 		// Socket is in same process (just switch pointers)
		BOOLEAN		LocalClose:1; 		// Local side has closed socket 
		BOOLEAN		LocalAborted:1;	    // Local side did abort
		BOOLEAN		RemoteReset:1; 		// Connection reset by remote
	};
} SOCK_SAN_DUPLICATION_CONTEXT, *PSOCK_SAN_DUPLICATION_CONTEXT;


typedef enum {
	PlaceHolder1,				// to make other values non-zero
	NonBlockingConnectState,
	ListenState,
	AcceptInProgress
} SpecialSocketStates;


typedef struct _AcceptContext {
    PSOCK_SAN_INFORMATION ListenSwitchSocket;
    PSOCK_SAN_INFORMATION AcceptSwitchSocket;
    AFD_SWITCH_ACCEPT_INFO  AcceptInfo;
} ACCEPT_CONTEXT, *PACCEPT_CONTEXT;


typedef struct _AcceptContextIoStatus {
	LIST_ENTRY	ListEntry;
	ACCEPT_CONTEXT	AcceptContext;
	IO_STATUS_BLOCK	IoStatusBlock;
	AFD_SWITCH_CONNECT_INFO ConnectInfo;
    UCHAR   Addresses[2*sizeof (TA_IP_ADDRESS)-sizeof (TA_ADDRESS)];
} ACCEPT_CONTEXT_IO_STATUS, *PACCEPT_CONTEXT_IO_STATUS;

typedef struct _HungSendTracker {
	LIST_ENTRY				ListEntry;
	PSOCK_SAN_INFORMATION	SwitchSocket;
	BOOL					LargeSendPending;
	BOOL					SendHung; // marked by CheckForHungLargeSends(); cleared when
								      // we are able to send data
	BOOL					UnhangInProgress;// CheckForHungLargeSends has initiated unhang
	BOOL  					PostponedSend;	// Switch was forced to postpone a send due to
								            // provider deadlock
	LONG					TrackerDeletionCount;// synchronize deletion of tracker
} HUNG_SEND_TRACKER, *PHUNG_SEND_TRACKER;

#define WSA_BUF_ARRAY_SIZE		3

typedef struct _SwitchWsaBuf {
	LIST_ENTRY	ListEntry;
	WSABUF		WsaBufArray[WSA_BUF_ARRAY_SIZE];
} SWITCH_WSA_BUF, *PSWITCH_WSA_BUF;

#define WSA_BUF_EX_ARRAY_SIZE		3

typedef struct _SwitchWsaBufEx {
	LIST_ENTRY	ListEntry;
	WSABUFEX	WsaBufExArray[WSA_BUF_EX_ARRAY_SIZE];
} SWITCH_WSA_BUF_EX, *PSWITCH_WSA_BUF_EX;

typedef struct _RegisteredBuffer *PREGISTERED_BUFFER; // defined in flow.h
typedef struct _ApplicationBufferHeader *PAPPLICATION_BUFFER_HEADER;

typedef struct _FCEntry {
	DWORD	Code;
	DWORD 	BufferSize;			// size of (send or recv) buffer
	DWORD	LastSmallMsgSeqNum;	// seq number of last small msg seen before FC 
	                            //   info was sent.
	char 	RdmaHandle[1];		// actual handle length determined at run time
} FC_ENTRY, *PFC_ENTRY;

struct _SOCK_SAN_INFORMATION {
    PSOCKET_INFORMATION     Socket;
	PSOCKET_INFORMATION     RefSocket; // all are ref counting done on this Socket
    PSOCK_SAN_PROVIDER      SanProvider;
    PSOCK_SAN_INFORMATION   Next;

    BOOL        IsOverlapped;    // This is an overlapped socket
    CRITICAL_SECTION CritSec;
    LIST_ENTRY 	SockListEntry; // list of socks which haven't been closed
	
	//
	// Above variables are NOT reset when TransmitFile does a REUSE.
	// All variables below ARE reset
	//

    SOCKET      SanSocket;   // This handle is user internally to talk to SAN

	ULONG_PTR	SanContext;

    UINT         ReceiveBytesBuffered; // num normal bytes buffered
    UINT         ExpeditedBytesBuffered;	// num OOB bytes buffered
    UINT         SendBytesBuffered; // send bytes buffered
	

    int         Error;              // The last SAN error on this socket.

    //
    //  Flags indicating the state of the socket.
    //

    LONG        RemoteReset;  	 // Remote end of the socket has been closed abortively.
	BOOLEAN		FlowControlInitialized;
    BOOLEAN     TempSocket;     // Temporary socket in the service process.
	BOOLEAN  	StayInSmallRecvLargeRecvMode;  // Set to TRUE if receiving too many
								// small sends and too few large sends

	HANDLE		Event;			 // To save event on which blocking connect() blocks 
	DWORD		IsConnected;	 // 0 or CONNECTED or NON_BLOCKING_IN_PROGRESS

	ULONG		ConnectTime;	 // used for SO_CONNECTTIME option handling

	LONG		IsClosing;		 // TRUE if CloseSocketExt has been called 
	LONG		CloseCount;	 	 // When reaches 2, then close this socket
	DWORD		State1;			 // to track if non-blocking connect in progress or
								 // socket is in listen state
	int			waitId;			 // valid if socket is in listening state

	DWORD		SockDupState;	 // Tracks different states for socket duplication
	PVOID 		DuplicationRequestContext; // AFD's context for sock dup
	DWORD		DestinationProcessId; // destination process for sock duplication
	HANDLE		ReclaimDupSock;	 // used to get back a just-duplicated sock 

	LIST_ENTRY	NonPostedRecvQueue;	  // registered recv buffers not yet posted as WSPRecv
 
	AFD_SWITCH_CONTEXT SelectContext; // shared between msafd and afd

    //
    //  Flow control queues and data.
    //

	//
	// Count variables to serialize execution of certain routines; light-weight
	// synchronization without using critical sections because we can't hold
	// a critical section when calling the provider
	//

	LONG CheckPendingAppRecvsCount;
	LONG RestartBlockedSendsCount;
	DWORD SendLargeSendFirstMsgCount;
	LONG HandleCompletedRecvCount;
	LONG PostReceiveCount;
	LONG DataAvailableDoRecvInProgress;
	LONG ChangeModeToLargeRecvOnly;	// signal from HandleControlMessage() to 
								    // CheckPendingAppRecvs()

    //  A list of allocated buffer sets
    LIST_ENTRY AllocatedBufferSetList;

    //  A list of posted receives
    LIST_ENTRY ReadyRecvQueue;

    //  A list of application posted receive buffers
    LIST_ENTRY ApplicationRecvQueue;

    //  A list of completed receives, waiting for the application
    LIST_ENTRY CompletedRecvQueue;

	// For a multi-threaded app, small messages can get out-of-order (because
	// we don't hold any locks when calling Provider's WSPSend). Receiver
	// uses this list to get data back in order based on SendMessageNumber in
	// the message header.
	LIST_ENTRY ReceivedOutOfOrderQueue;

    //  A list of buffer available for sends
    LIST_ENTRY ReadySendQueue;

    //  A list of posted sends
    LIST_ENTRY SendInProgressQueue;

    //  A list of sends waiting for quota
    LIST_ENTRY BlockedSendQueue;

    //  A list of control messages waiting for quota
    LIST_ENTRY BlockedControlMessageQueue;

    //  The send credit available for us to send
    DWORD SendCredit;

    //  The send credit that the receiver has
    DWORD ReceiversSendCredit;

    DWORD SendMessageNumber;
    DWORD ReceiveMessageNumber;
    DWORD NumberReceivesPosted;
	DWORD NumberSendBuffersFree; // length of ReadySendQueue
	DWORD LastSmallMessageSent;	// pertains to small msgs with data (NOT control msgs)
	DWORD LastSmallMessageReceived;// pertains to small msgs w/ data (NOT control msgs)
	DWORD SendCreditReservation;

	// Receiver's mode variables for bulk transfer flow control
	DWORD RsMode;					// what sender thinks R's mode is
	DWORD TmpRsMode;				// used while in transition
	DWORD TmpRsModeCount;
	DWORD RsCopyOfSsRsMode;			// receiver's copy of sender's R's mode
	DWORD xRsMode;					// Used for delayed setting of RsMode
	DWORD xBeginRecvFCInfo;			// Delay setting RsMode till BeginRecvFCInfo reaches
								    // this value
	DWORD LargeRecvCount;			// For LARGE_RECV_ONLY_MODE
	DWORD SmallRecvCount;		    // For LARGE_RECV_ONLY_MODE

	PAPPLICATION_BUFFER_HEADER LatestSmallRecv;	// non-null if we have a non-RDMA 
								    // recv pending in ApplicationRecvQueue

	PHUNG_SEND_TRACKER	Tracker;    // Used to detect hung large sends 

	HANDLE 		RegisteredSendMemoryHandle;	// handle of memory registered with
								            // provider for doing small sends

	LIST_ENTRY 	WsaBufListHead;		// cache of WSABUFs for overlapped send()'s
	LIST_ENTRY 	WsaBufExListHead;	// cache of WSABUFEXs for overlapped send()'s

	PFC_ENTRY	LargeRecvFCInfo;	// recvs available on remote side (per socket)
	DWORD		BeginRecvFCInfo;	// index of first valid entry 
	DWORD		EndRecvFCInfo;		// index of first free entry
	DWORD		NumRecvFCEntries; 	// number of valid entries in the ring
	DWORD		NumStale;  			// number of entries that can be stale
	DWORD 		LastLastSMSent;		// Value of LastSmallMessageSent last time 
								    // NumPossiblyStale was calculated
	DWORD		LastLastSMSeqNum;  	// Value of FC->LastSmallMsgSeqNum last time
	PFC_ENTRY	LargeSendFCInfo;	// sends available on remote side (per socket)
								    // NumPossiblyStale was calculated
	DWORD		BeginSendFCInfo;	// index of first valid entry 
	DWORD		EndSendFCInfo;		// index of first free entry
	DWORD		NumSendFCEntries; 	// number of valid entries in the ring 

	WSABUF wsaBuf;				// persistent WSABUF for accepting socket to do recv()

	// For doing overlapped receive for AcceptEx(). Need persistent space
	OVERLAPPED 			Overlapped;

	union {
		int				MaxListenBacklog;      // We'll buffer this many incoming connects 
		LONG			ContinuingAcceptProcessing; // Used for accepted socket
	};
	union {
		int				CurrentListenBacklog;  // We have this many buffered(for listen sock)
		ULONG			HalfAcceptRefCount;	   // ref count for accept socket
	};

	// For completion port signalling of accept.  Need persistent space
	union {
		LIST_ENTRY		AcceptContextIoStatusList; // for listening socket
		PACCEPT_CONTEXT_IO_STATUS PBlock;          // for accepting socket
	};
	PCHAR				AcceptContextMemBlock; // memory allocated for above list

	// socket options
	BOOLEAN				keepalive;
	BOOLEAN				dontroute;

	// buffers for SO_SNDBUF and SO_RCVBUF buffering
	PCHAR				NormalDataRegBuf;
	PCHAR				NormalDataRegBufEnd;
	PCHAR				NormalFreeStart;
	PCHAR				NormalInUseStart;
	PREGISTERED_BUFFER  LastSpecialRecvBuf;	// last special buf in CompletedRecvQueue
	BOOLEAN				NormalDataRecvsAvailAfterBuffering;		// for wrap-around
	
	LIST_ENTRY	AppBufferPool;
	LIST_ENTRY	RdmaDataPool;
	LIST_ENTRY	ControlMessageStorePool;

    struct {
	    // for graceful disconnect and WSPShutdown
        BOOLEAN	GracefulDisconnect:1;// WSPClosesocket has started graceful disconnect
        BOOLEAN	LocalClose:1;		// local side has called WSPCloseSocket or Shutdown
        BOOLEAN	LocalAborted:1;		// local side closed connection due to some error
        BOOLEAN	CloseMsgSent:1;		// we've told remote side of (graceful) close
        BOOLEAN	TransmitFileReuse:1;// True when graceful disconnect initiated by TF, i.e.,
								    // the TCP socket should not be closed
		BOOLEAN	SanSockClosing:1;   // SockSanCloseSocket() has been called
    };
    LONG	RemoteClose;		    // Remote side has told us of a (graceful) close
	LONG 	ContinueGracefulDisconnectCount; // synchronize graceful disconnect
	HANDLE	CloseEvent;			    // For lingering
	LPOVERLAPPED TFOv;			    // for REUSE

	LONG AbnormalCloseCount;        // Used by DeleteSwitchSocketNoFlowControl().

#if RDMA_CACHE
	WSABUFEX 	RdmaBuf[RDMA_CACHE_SIZE]; // cache for RDMA registrations
	PHYSICAL_ADDRESS PhysAddr[RDMA_CACHE_SIZE]; // physical addr of cached buffer
	LONG		RdmaBufStatus[RDMA_CACHE_SIZE]; // >0 if RDMA underway; ==0 if completed
#endif

	PSOCK_SAN_INFORMATION NextSwitchSocket;	// for linked lists
	PSOCK_SAN_INFORMATION PrevSwitchSocket;	// for linked lists

};


#define MAX_FAST_SAN_ADDRESSES  1
#define SOCK_SAN_ON_TCP_KEY \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Winsock\\Parameters\\TCP on SAN"

#endif //_AFD_SAN_SWITCH_


#endif // ndef _SOCKTYPE_
