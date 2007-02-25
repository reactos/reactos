/*
 * CSRSRV Status 
 */

/* Organization
 *
 * api.c     - Handles the LPC Reply/Request Threads which wait on Sb and Csr APIs.
 *             Also in charge of creating those threads and setting up the ports.
 *             Finally, it provides external APIs for validating the API buffers
 *             and doing server-to-server API calls.
 *
 * init.c    - Handles initialization of CSRSRV, including command-line parsing,
 *             loading the Server DLLs, creating the Session Directories, setting
 *             up the DosDevices Object Directory, and initializing each component.
 *
 * process.c - Handles all internal functions dealing with the CSR Process Object,
 *             including de/allocation, de/referencing, un/locking, prority, and 
 *             lookups. Also handles all external APIs which touch the CSR Process Object.
 *
 * server.c  - Handles all internal functions related to loading and managing Server
 *             DLLs, as well as the routines handling the Shared Static Memory Section.
 *             Holds the API Dispatch/Valid/Name Tables and the public CSR_SERVER API
 *             interface. Also home of the SEH handler.
 *
 * session.c - Handles all internal functions dealing with the CSR Session Object,
 *             including de/allocation, de/referencing, and un/locking. Holds the SB API
 *             Dispatch/Name Tables and the public CsrSv API Interface for commmunication
 *             with the Session Manager.
 *
 * thread.c  - Handles all internal functions dealing with the CSR Thread Object,
 *             including de/allocation, de/referencing, un/locking, impersonation, and
 *             lookups. Also handles all external APIs which touch the CSR Thread Object.
 *
 * wait.c   -  Handles all internal functions dealing with the CSR Wait Object,
 *             including de/allocation, de/referencing and un/locking. Also implements
 *             the external Wait API for creating, removing and/or notifying waits.
 */

/* Exported APIs, their location, and their status
 * CsrAddStaticServerThread    753E679E 1  - server.c  - IMPLEMENTED
 * CsrCallServerFromServer     753E4FD9 2  - api.c     - IMPLEMENTED
 * CsrConnectToUser            753E4E48 3  - api.c     - IMPLEMENTED
 * CsrCreateProcess            753E6FD3 4  - process.c - IMPLEMENTED
 * CsrCreateRemoteThread       753E73BD 5  - thread.c  - IMPLEMENTED
 * CsrCreateThread             753E72DA 6  - thread.c  - IMPLEMENTED
 * CsrCreateWait               753E770E 7  - wait.c    - IMPLEMENTED
 * CsrDebugProcess             753E7682 8  - process.c - IMPLEMENTED
 * CsrDebugProcessStop         753E768A 9  - process.c - IMPLEMENTED
 * CsrDereferenceProcess       753E6281 10 - process.c - IMPLEMENTED
 * CsrDereferenceThread        753E6964 11 - thread.c  - IMPLEMENTED
 * CsrDereferenceWait          753E7886 12 - wait.c    - IMPLEMENTED
 * CsrDestroyProcess           753E7225 13 - process.c - IMPLEMENTED
 * CsrDestroyThread            753E7478 14 - thread.c  - IMPLEMENTED
 * CsrExecServerThread         753E6841 15 - thread.c  - IMPLEMENTED
 * CsrGetProcessLuid           753E632F 16 - process.c - IMPLEMENTED
 * CsrImpersonateClient        753E60F8 17 - thread.c  - IMPLEMENTED
 * CsrLockProcessByClientId    753E668F 18 - process.c - IMPLEMENTED
 * CsrLockThreadByClientId     753E6719 19 - thread.c  - IMPLEMENTED
 * CsrMoveSatisfiedWait        753E7909 20 - wait.c    - IMPLEMENTED
 * CsrNotifyWait               753E782F 21 - wait.c    - IMPLEMENTED
 * CsrPopulateDosDevices       753E37A5 22 - init.c    - IMPLEMENTED
 * CsrQueryApiPort             753E4E42 23 - api.c     - IMPLEMENTED
 * CsrReferenceThread          753E61E5 24 - thread.c  - IMPLEMENTED
 * CsrRevertToSelf             753E615A 25 - thread.c  - IMPLEMENTED
 * CsrServerInitialization     753E3D75 26 - server.c  - IMPLEMENTED
 * CsrSetBackgroundPriority    753E5E87 27 - process.c - IMPLEMENTED
 * CsrSetCallingSpooler        753E6425 28 - server.c  - IMPLEMENTED
 * CsrSetForegroundPriority    753E5E67 29 - process.c - IMPLEMENTED
 * CsrShutdownProcesses        753E7547 30 - process.c - IMPLEMENTED
 * CsrUnhandledExceptionFilter 753E3FE3 31 - server.c  - IMPLEMENTED
 * CsrUnlockProcess            753E66FD 32 - process.c - IMPLEMENTED
 * CsrUnlockThread             753E7503 33 - thread.c  - IMPLEMENTED
 * CsrValidateMessageBuffer    753E528D 34 - api.c     - IMPLEMENTED
 * CsrValidateMessageString    753E5323 35 - api.c     - UNIMPLEMENTED
 */

/* Public CSR API Interface Status (server.c)
 * CsrSrvClientConnect                                 - IMPLEMENTED
 * CsrSrvUnusedFunction                                - IMPLEMENTED
 * CsrSrvIdentifyAlertableThread                       - IMPLEMENTED
 * CsrSrvSetPriorityClass                              - IMPLEMENTED
 */

/* Public SB API Interface Status (session.c)
 * CsrSbCreateSession                                  - IMPLEMENTED
 * CsrSbForeignSessionComplete                         - IMPLEMENTED
 * CsrSbTerminateSession                               - IMPLEMENTED
 * CsrSbCreateProcess                                  - UNIMPLEMENTED
 */

/* What's missing:
 *
 * - SMSS needs to be partly re-written to match some things done here.
 *   Among other things, SmConnectToSm, SmCompleteSession and the other
 *   Sm* Exported APIs have to be properly implemented, as well as the
 *   callback calling and SM LPC APIs. [NOT DONE]
 *
 * - NTDLL needs to get the Csr* routines properly implemented. [DONE!]
 *
 * - KERNEL32, USER32 need to register with their servers properly.
 *   Additionally, user32 needs to have ClientThreadStartup implemented
 *   properly and do the syscall NtUserInitialize (I think) which also
 *   needs to be implemented in win32k.sys. All this should be less then
 *   100 lines of code. [KERNEL32 50% DONE, USER32 NOT DONE]
 *
 * - The skeleton code for winsrv and basesrv which connects with CSR/CSRSRV
 *   needs to be written. [NOT DONE]
 *
 * - The kernel's LPC implementation needs to be made compatible. [NOT DONE]
 */

