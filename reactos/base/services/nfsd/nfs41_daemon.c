/* NFSv4.1 client for Windows
 * Copyright © 2012 The Regents of the University of Michigan
 *
 * Olga Kornievskaia <aglo@umich.edu>
 * Casey Bodley <cbodley@umich.edu>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * without any warranty; without even the implied warranty of merchantability
 * or fitness for a particular purpose.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 */

#include <windows.h>
#include <process.h>
#include <tchar.h>
#include <stdio.h>

#include <devioctl.h>
#include <lmcons.h> /* UNLEN for GetUserName() */
#include <iphlpapi.h> /* for GetNetworkParam() */
#include "nfs41_driver.h" /* for NFS41_USER_DEVICE_NAME_A */
#include "nfs41_np.h" /* for NFS41NP_SHARED_MEMORY */

#include "idmap.h"
#include "daemon_debug.h"
#include "upcall.h"
#include "util.h"

#define MAX_NUM_THREADS 128
DWORD NFS41D_VERSION = 0;

static const char FILE_NETCONFIG[] = "C:\\ReactOS\\System32\\drivers\\etc\\netconfig";

/* Globals */
char localdomain_name[NFS41_HOSTNAME_LEN];
int default_uid = 666;
int default_gid = 777;

#ifndef STANDALONE_NFSD //make sure to define it in "sources" not here
#include "service.h"
HANDLE  stop_event = NULL;
#endif
typedef struct _nfs41_process_thread {
    HANDLE handle;
    uint32_t tid;
} nfs41_process_thread;

static int map_user_to_ids(nfs41_idmapper *idmapper, uid_t *uid, gid_t *gid)
{
    char username[UNLEN + 1];
    DWORD len = UNLEN + 1;
    int status = NO_ERROR;

    if (!GetUserNameA(username, &len)) {
        status = GetLastError();
        eprintf("GetUserName() failed with %d\n", status);
        goto out;
    }
    dprintf(1, "map_user_to_ids: mapping user %s\n", username);

    if (nfs41_idmap_name_to_ids(idmapper, username, uid, gid)) {
        /* instead of failing for auth_sys, fall back to 'nobody' uid/gid */
        *uid = default_uid;
        *gid = default_gid;
    }
out:
    return status;
}

static unsigned int WINAPI thread_main(void *args) 
{
    nfs41_idmapper *idmapper = (nfs41_idmapper*)args;
    DWORD status = 0;
    HANDLE pipe;
    // buffer used to process upcall, assumed to be fixed size. 
    // if we ever need to handle non-cached IO, need to make it dynamic
    unsigned char outbuf[UPCALL_BUF_SIZE], inbuf[UPCALL_BUF_SIZE]; 
    DWORD inbuf_len = UPCALL_BUF_SIZE, outbuf_len;
    nfs41_upcall upcall;

    pipe = CreateFile(NFS41_USER_DEVICE_NAME_A, GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
        0, NULL);
    if (pipe == INVALID_HANDLE_VALUE)
    {
        eprintf("Unable to open upcall pipe %d\n", GetLastError());
        return GetLastError();
    }

    while(1) {
        status = DeviceIoControl(pipe, IOCTL_NFS41_READ, NULL, 0,
            outbuf, UPCALL_BUF_SIZE, (LPDWORD)&outbuf_len, NULL);
        if (!status) {
            eprintf("IOCTL_NFS41_READ failed %d\n", GetLastError());
            continue;
        }

        status = upcall_parse(outbuf, (uint32_t)outbuf_len, &upcall);
        if (status) {
            upcall.status = status;
            goto write_downcall;
        }

        /* map username to uid/gid */
        status = map_user_to_ids(idmapper, &upcall.uid, &upcall.gid);
        if (status) {
            upcall.status = status;
            goto write_downcall;
        }

        if (upcall.opcode == NFS41_SHUTDOWN) {
            printf("Shutting down..\n");
            exit(0);
        }

        status = upcall_handle(&upcall);

write_downcall:
        dprintf(1, "writing downcall: xid=%lld opcode=%s status=%d "
            "get_last_error=%d\n", upcall.xid, opcode2string(upcall.opcode),
            upcall.status, upcall.last_error);

        upcall_marshall(&upcall, inbuf, (uint32_t)inbuf_len, (uint32_t*)&outbuf_len);

        dprintf(2, "making a downcall: outbuf_len %ld\n\n", outbuf_len);
        status = DeviceIoControl(pipe, IOCTL_NFS41_WRITE,
            inbuf, inbuf_len, NULL, 0, (LPDWORD)&outbuf_len, NULL);
        if (!status) {
            eprintf("IOCTL_NFS41_WRITE failed with %d xid=%lld opcode=%s\n", 
                GetLastError(), upcall.xid, opcode2string(upcall.opcode));
            upcall_cancel(&upcall);
        }
        if (upcall.status != NFSD_VERSION_MISMATCH)
            upcall_cleanup(&upcall);
    }
    CloseHandle(pipe);

    return GetLastError();
}

#ifndef STANDALONE_NFSD
VOID ServiceStop()
{
   if (stop_event)
      SetEvent(stop_event);
}
#endif

typedef struct _nfsd_args {
    bool_t ldap_enable;
    int debug_level;
} nfsd_args;

static bool_t check_for_files()
{
    FILE *fd;
     
    fd = fopen(FILE_NETCONFIG, "r");
    if (fd == NULL) {
        fprintf(stderr,"nfsd() failed to open file '%s'\n", FILE_NETCONFIG);
        return FALSE;
    }
    fclose(fd);
    return TRUE;
}

static void PrintUsage()
{
    fprintf(stderr, "Usage: nfsd.exe -d <debug_level> --noldap "
        "--uid <non-zero value> --gid\n");
}
static bool_t parse_cmdlineargs(int argc, TCHAR *argv[], nfsd_args *out)
{
    int i;

    /* set defaults. */
    out->debug_level = 1;
    out->ldap_enable = TRUE;

    /* parse command line */
    for (i = 1; i < argc; i++) {
        if (argv[i][0] == TEXT('-')) {
            if (_tcscmp(argv[i], TEXT("-h")) == 0) { /* help */
                PrintUsage();
                return FALSE;
            }
            else if (_tcscmp(argv[i], TEXT("-d")) == 0) { /* debug level */
                ++i;
                if (i >= argc) {
                    fprintf(stderr, "Missing debug level value\n");
                    PrintUsage();
                    return FALSE;
                } 
                out->debug_level = _ttoi(argv[i]);
            }
            else if (_tcscmp(argv[i], TEXT("--noldap")) == 0) { /* no LDAP */
                out->ldap_enable = FALSE;
            }
            else if (_tcscmp(argv[i], TEXT("--uid")) == 0) { /* no LDAP, setting default uid */
                ++i;
                if (i >= argc) {
                    fprintf(stderr, "Missing uid value\n");
                    PrintUsage();
                    return FALSE;
                }
                default_uid = _ttoi(argv[i]);
                if (!default_uid) {
                    fprintf(stderr, "Invalid (or missing) anonymous uid value of %d\n", 
                        default_uid);
                    return FALSE;
                }
            }
            else if (_tcscmp(argv[i], TEXT("--gid")) == 0) { /* no LDAP, setting default gid */
                ++i;
                if (i >= argc) {
                    fprintf(stderr, "Missing gid value\n");
                    PrintUsage();
                    return FALSE;
                }
                default_gid = _ttoi(argv[i]);
            }
            else
                fprintf(stderr, "Unrecognized option '%s', disregarding.\n", argv[i]);
        }
    }
    fprintf(stdout, "parse_cmdlineargs: debug_level %d ldap is %d\n", 
        out->debug_level, out->ldap_enable);
    return TRUE;
}

static void print_getaddrinfo(struct addrinfo *ptr)
{
    char ipstringbuffer[46];
    DWORD ipbufferlength = 46;

    dprintf(1, "getaddrinfo response flags: 0x%x\n", ptr->ai_flags);
    switch (ptr->ai_family) {
    case AF_UNSPEC: dprintf(1, "Family: Unspecified\n"); break;
    case AF_INET:
        dprintf(1, "Family: AF_INET IPv4 address %s\n",
            inet_ntoa(((struct sockaddr_in *)ptr->ai_addr)->sin_addr));
        break;
    case AF_INET6:
        if (WSAAddressToString((LPSOCKADDR)ptr->ai_addr, (DWORD)ptr->ai_addrlen, 
                NULL, ipstringbuffer, &ipbufferlength))
            dprintf(1, "WSAAddressToString failed with %u\n", WSAGetLastError());
        else    
            dprintf(1, "Family: AF_INET6 IPv6 address %s\n", ipstringbuffer);
        break;
    case AF_NETBIOS: dprintf(1, "AF_NETBIOS (NetBIOS)\n"); break;
    default: dprintf(1, "Other %ld\n", ptr->ai_family); break;
    }
    dprintf(1, "Canonical name: %s\n", ptr->ai_canonname);
}

static int getdomainname()
{
    int status = 0;
    PFIXED_INFO net_info = NULL;
    DWORD size = 0;
    BOOLEAN flag = FALSE;

    status = GetNetworkParams(net_info, &size);
    if (status != ERROR_BUFFER_OVERFLOW) {
        eprintf("getdomainname: GetNetworkParams returned %d\n", status);
        goto out;
    }
    net_info = calloc(1, size);
    if (net_info == NULL) {
        status = GetLastError();
        goto out;
    }
    status = GetNetworkParams(net_info, &size);
    if (status) {
        eprintf("getdomainname: GetNetworkParams returned %d\n", status);
        goto out_free;
    }

    if (net_info->DomainName[0] == '\0') {
        struct addrinfo *result = NULL, *ptr = NULL, hints = { 0 };
        char hostname[NI_MAXHOST], servInfo[NI_MAXSERV];

        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        status = getaddrinfo(net_info->HostName, NULL, &hints, &result);
        if (status) {
            status = WSAGetLastError();
            eprintf("getdomainname: getaddrinfo failed with %d\n", status);
            goto out_free;
        } 

        for (ptr=result; ptr != NULL; ptr=ptr->ai_next) {
            print_getaddrinfo(ptr);

            switch (ptr->ai_family) {
            case AF_INET6:
            case AF_INET:
                status = getnameinfo((struct sockaddr *)ptr->ai_addr,
                            (socklen_t)ptr->ai_addrlen, hostname, NI_MAXHOST, 
                            servInfo, NI_MAXSERV, NI_NAMEREQD);
                if (status)
                    dprintf(1, "getnameinfo failed %d\n", WSAGetLastError());
                else {
                    size_t i, len = strlen(hostname);
                    char *p = hostname;
                    dprintf(1, "getdomainname: hostname %s %d\n", hostname, len);
                    for (i = 0; i < len; i++)
                        if (p[i] == '.')
                            break;
                    if (i == len)
                        break;
                    flag = TRUE;
                    memcpy(localdomain_name, &hostname[i+1], len-i);
                    dprintf(1, "getdomainname: domainname %s %d\n", 
                            localdomain_name, strlen(localdomain_name));
                    goto out_loop;
                }
                break;
            default:
                break;
            }
        }
out_loop:
        if (!flag) {
            status = ERROR_INTERNAL_ERROR;
            eprintf("getdomainname: unable to get a domain name. "
                "Set this machine's domain name:\n"
                "System > ComputerName > Change > More > mydomain\n");
        }
        freeaddrinfo(result);
    } else {
        dprintf(1, "domain name is %s\n", net_info->DomainName);
        memcpy(localdomain_name, net_info->DomainName, 
                strlen(net_info->DomainName));
        localdomain_name[strlen(net_info->DomainName)] = '\0';
    }
out_free:
    free(net_info);
out:
    return status;
}

#ifdef STANDALONE_NFSD
void __cdecl _tmain(int argc, TCHAR *argv[])
#else
VOID ServiceStart(DWORD argc, LPTSTR *argv)
#endif
{
    DWORD status = 0, len;
    // handle to our drivers
    HANDLE pipe;
    nfs41_process_thread tids[MAX_NUM_THREADS];
    nfs41_idmapper *idmapper = NULL;
    int i;
    nfsd_args cmd_args;

    if (!check_for_files())
        exit(0);
    if (!parse_cmdlineargs(argc, argv, &cmd_args)) 
        exit(0);
    set_debug_level(cmd_args.debug_level);
    open_log_files();

#ifdef __REACTOS__
    /* Start the kernel part */
    {
        HANDLE hSvcMan = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
        if (hSvcMan)
        {
            HANDLE hSvc = OpenService(hSvcMan, "nfs41_driver", SERVICE_ALL_ACCESS);
            if (hSvc)
            {
                SERVICE_STATUS SvcSt;
                QueryServiceStatus(hSvc, &SvcSt);
                if (SvcSt.dwCurrentState != SERVICE_RUNNING)
                {
                    if (StartService(hSvc, 0, NULL))
                    {
                        dprintf(1, "NFS41 driver started\n");
                    }
                    else
                    {
                        eprintf("Driver failed to start: %d\n", GetLastError());
                    }
                }
                else
                {
                    eprintf("Driver in state: %x\n", SvcSt.dwCurrentState);
                }

                CloseServiceHandle(hSvc);
            }
            else
            {
                eprintf("Failed to open service: %d\n", GetLastError());
            }

            CloseServiceHandle(hSvcMan);
        }
        else
        {
            eprintf("Failed to open service manager: %d\n", GetLastError());
        }
    }
#endif

#ifdef _DEBUG
    /* dump memory leaks to stderr on exit; this requires the debug heap,
    /* available only when built in debug mode under visual studio -cbodley */
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
#pragma warning (push)
#pragma warning (disable : 4306) /* conversion from 'int' to '_HFILE' of greater size */
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
#pragma warning (pop)
    dprintf(1, "debug mode. dumping memory leaks to stderr on exit.\n");
#endif
    /* acquire and store in global memory current dns domain name.
     * needed for acls */
    if (getdomainname())
        exit(0);

    nfs41_server_list_init();

    if (cmd_args.ldap_enable) {
        status = nfs41_idmap_create(&idmapper);
        if (status) {
            eprintf("id mapping initialization failed with %d\n", status);
            goto out_logs;
        }
    }

    NFS41D_VERSION = GetTickCount();
    dprintf(1, "NFS41 Daemon starting: version %d\n", NFS41D_VERSION);

    pipe = CreateFile(NFS41_USER_DEVICE_NAME_A, GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
        0, NULL);
    if (pipe == INVALID_HANDLE_VALUE)
    {
        eprintf("Unable to open upcall pipe %d\n", GetLastError());
        goto out_idmap;
    }

    dprintf(1, "starting nfs41 mini redirector\n");
    status = DeviceIoControl(pipe, IOCTL_NFS41_START,
        &NFS41D_VERSION, sizeof(DWORD), NULL, 0, (LPDWORD)&len, NULL);
    if (!status) {
        eprintf("IOCTL_NFS41_START failed with %d\n", 
                GetLastError());
        goto out_pipe;
    }

#ifndef STANDALONE_NFSD
    stop_event = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (stop_event == NULL)
      goto out_pipe;
#endif

    for (i = 0; i < MAX_NUM_THREADS; i++) {
        tids[i].handle = (HANDLE)_beginthreadex(NULL, 0, thread_main, 
                idmapper, 0, &tids[i].tid);
        if (tids[i].handle == INVALID_HANDLE_VALUE) {
            status = GetLastError();
            eprintf("_beginthreadex failed %d\n", status);
            goto out_pipe;
        }
    }
#ifndef STANDALONE_NFSD
    // report the status to the service control manager.
    if (!ReportStatusToSCMgr(SERVICE_RUNNING, NO_ERROR, 0))
        goto out_pipe;
    WaitForSingleObject(stop_event, INFINITE);
#else
    //This can be changed to waiting on an array of handles and using waitformultipleobjects
    dprintf(1, "Parent waiting for children threads\n");
    for (i = 0; i < MAX_NUM_THREADS; i++)
        WaitForSingleObject(tids[i].handle, INFINITE );
#endif
    dprintf(1, "Parent woke up!!!!\n");

out_pipe:
    CloseHandle(pipe);
out_idmap:
    if (idmapper) nfs41_idmap_free(idmapper);
out_logs:
#ifndef STANDALONE_NFSD
    close_log_files();
#endif
    return;
}
