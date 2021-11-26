/*
 * CSRSRV Status
 */

/* Organization
 *
 * api.c      - Handles the LPC Reply/Request Threads which wait on Sb and Csr APIs.
 *              Also in charge of creating those threads and setting up the ports.
 *              Finally, it provides external APIs for validating the API buffers
 *              and doing server-to-server API calls.
 *
 * init.c     - Handles initialization of CSRSRV, including command-line parsing,
 *              loading the Server DLLs, creating the Session Directories, setting
 *              up the DosDevices Object Directory, and initializing each component.
 *
 * procsup.c  - Handles all internal functions dealing with the CSR Process Object,
 *              including de/allocation, de/referencing, un/locking, prority, and
 *              lookups. Also handles all external APIs which touch the CSR Process Object.
 *
 * server.c   - Handles all internal functions related to loading and managing Server
 *              DLLs, as well as the routines handling the Shared Static Memory Section.
 *              Holds the API Dispatch/Valid/Name Tables and the public CSR_SERVER API
 *              interface. Also home of the SEH handler.
 *
 * session.c  - Handles all internal functions dealing with the CSR Session Object,
 *              including de/allocation, de/referencing, and un/locking. Holds the SB API
 *              Dispatch/Name Tables and the public CsrSv API Interface for commmunication
 *              with the Session Manager.
 *
 * thredsup.c - Handles all internal functions dealing with the CSR Thread Object,
 *              including de/allocation, de/referencing, un/locking, impersonation, and
 *              lookups. Also handles all external APIs which touch the CSR Thread Object.
 *
 * wait.c     - Handles all internal functions dealing with the CSR Wait Object,
 *              including de/allocation, de/referencing and un/locking. Also implements
 *              the external Wait API for creating, removing and/or notifying waits.
 */

/* Exported APIs                     Location     Status
 *------------------------------------------------------------
 * CsrAddStaticServerThread     1  - server.c   - IMPLEMENTED
 * CsrCallServerFromServer      2  - api.c      - IMPLEMENTED
 * CsrConnectToUser             3  - api.c      - IMPLEMENTED
 * CsrCreateProcess             4  - procsup.c  - IMPLEMENTED
 * CsrCreateRemoteThread        5  - thredsup.c - IMPLEMENTED
 * CsrCreateThread              6  - thredsup.c - IMPLEMENTED
 * CsrCreateWait                7  - wait.c     - IMPLEMENTED
 * CsrDebugProcess              8  - procsup.c  - IMPLEMENTED
 * CsrDebugProcessStop          9  - procsup.c  - IMPLEMENTED
 * CsrDereferenceProcess        10 - procsup.c  - IMPLEMENTED
 * CsrDereferenceThread         11 - thredsup.c - IMPLEMENTED
 * CsrDereferenceWait           12 - wait.c     - IMPLEMENTED
 * CsrDestroyProcess            13 - procsup.c  - IMPLEMENTED
 * CsrDestroyThread             14 - thredsup.c - IMPLEMENTED
 * CsrExecServerThread          15 - thredsup.c - IMPLEMENTED
 * CsrGetProcessLuid            16 - procsup.c  - IMPLEMENTED
 * CsrImpersonateClient         17 - thredsup.c - IMPLEMENTED
 * CsrLockProcessByClientId     18 - procsup.c  - IMPLEMENTED
 * CsrLockThreadByClientId      19 - thredsup.c - IMPLEMENTED
 * CsrMoveSatisfiedWait         20 - wait.c     - IMPLEMENTED
 * CsrNotifyWait                21 - wait.c     - IMPLEMENTED
 * CsrPopulateDosDevices        22 - init.c     - IMPLEMENTED
 * CsrQueryApiPort              23 - api.c      - IMPLEMENTED
 * CsrReferenceThread           24 - thredsup.c - IMPLEMENTED
 * CsrRevertToSelf              25 - thredsup.c - IMPLEMENTED
 * CsrServerInitialization      26 - server.c   - IMPLEMENTED
 * CsrSetBackgroundPriority     27 - procsup.c  - IMPLEMENTED
 * CsrSetCallingSpooler         28 - server.c   - IMPLEMENTED
 * CsrSetForegroundPriority     29 - procsup.c  - IMPLEMENTED
 * CsrShutdownProcesses         30 - procsup.c  - IMPLEMENTED
 * CsrUnhandledExceptionFilter  31 - server.c   - IMPLEMENTED
 * CsrUnlockProcess             32 - procsup.c  - IMPLEMENTED
 * CsrUnlockThread              33 - thredsup.c - IMPLEMENTED
 * CsrValidateMessageBuffer     34 - api.c      - IMPLEMENTED
 * CsrValidateMessageString     35 - api.c      - IMPLEMENTED
 */

/* Public CSR API Interface Status (server.c)
 * CsrSrvClientConnect                          - IMPLEMENTED
 * CsrSrvUnusedFunction                         - IMPLEMENTED
 */

/* Public SB API Interface Status (session.c)
 * CsrSbCreateSession                           - IMPLEMENTED
 * CsrSbForeignSessionComplete                  - IMPLEMENTED
 * CsrSbTerminateSession                        - UNIMPLEMENTED
 * CsrSbCreateProcess                           - UNIMPLEMENTED
 */

/* What's missing:
 *
 * - SMSS needs to be partly re-written to match some things done here.
 *   Among other things, SmConnectToSm, SmCompleteSession and the other
 *   Sm* Exported APIs have to be properly implemented, as well as the
 *   callback calling and SM LPC APIs. [PARTLY DONE!]
 *
 * - USER32 needs to register with winsrv properly. Additionally, it
 *   needs to have ClientThreadStartup implemented properly and do the
 *   syscall NtUserInitialize (I think) which also needs to be implemented
 *   in win32k.sys. All this should be less then 100 lines of code.
 *   [NOT DONE]
 */
