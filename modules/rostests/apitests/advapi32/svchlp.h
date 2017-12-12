/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Support helpers for embedded services inside api tests.
 * PROGRAMMERS:     Jacek Caban for CodeWeavers
 *                  Thomas Faber <thomas.faber@reactos.org>
 *                  Hermes Belusca-Maito
 */

#pragma once

/**********  S E R V I C E   ( C L I E N T )   M O D U L E   S I D E  *********/

void send_msg(const char *type, const char *msg);
void service_trace(const char *msg, ...);
void service_ok(int cnd, const char *msg, ...);
void service_process(BOOL (*start_service)(PCSTR, PCWSTR), int argc, char** argv);


/***********  T E S T E R   ( S E R V E R )   M O D U L E   S I D E  **********/

SC_HANDLE register_service_exA(
    SC_HANDLE scm_handle,
    PCSTR test_name,
    PCSTR service_name, // LPCSTR lpServiceName,
    PCSTR extra_args OPTIONAL,
    DWORD dwDesiredAccess,
    DWORD dwServiceType,
    DWORD dwStartType,
    DWORD dwErrorControl,
    LPCSTR lpLoadOrderGroup OPTIONAL,
    LPDWORD lpdwTagId OPTIONAL,
    LPCSTR lpDependencies OPTIONAL,
    LPCSTR lpServiceStartName OPTIONAL,
    LPCSTR lpPassword OPTIONAL);

SC_HANDLE register_service_exW(
    SC_HANDLE scm_handle,
    PCWSTR test_name,
    PCWSTR service_name, // LPCWSTR lpServiceName,
    PCWSTR extra_args OPTIONAL,
    DWORD dwDesiredAccess,
    DWORD dwServiceType,
    DWORD dwStartType,
    DWORD dwErrorControl,
    LPCWSTR lpLoadOrderGroup OPTIONAL,
    LPDWORD lpdwTagId OPTIONAL,
    LPCWSTR lpDependencies OPTIONAL,
    LPCWSTR lpServiceStartName OPTIONAL,
    LPCWSTR lpPassword OPTIONAL);

SC_HANDLE register_serviceA(
    SC_HANDLE scm_handle,
    PCSTR test_name,
    PCSTR service_name,
    PCSTR extra_args OPTIONAL);

SC_HANDLE register_serviceW(
    SC_HANDLE scm_handle,
    PCWSTR test_name,
    PCWSTR service_name,
    PCWSTR extra_args OPTIONAL);

#ifdef UNICODE
#define register_service_ex register_service_exW
#define register_service    register_serviceW
#else
#define register_service_ex register_service_exA
#define register_service    register_serviceA
#endif

void test_runner(void (*run_test)(PCSTR, PCWSTR, void*), void *param);
