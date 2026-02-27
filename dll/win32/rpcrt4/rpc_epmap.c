/*
 * RPC endpoint mapper
 *
 * Copyright 2002 Greg Turner
 * Copyright 2001 Ove Kåven, TransGaming Technologies
 * Copyright 2008 Robert Shearman (for CodeWeavers)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winsvc.h"

#include "rpc.h"

#include "wine/debug.h"
#include "wine/exception.h"

#include "rpc_binding.h"
#include "epm.h"
#include "epm_towers.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

/* The "real" RPC portmapper endpoints that I know of are:
 *
 *  ncadg_ip_udp: 135
 *  ncacn_ip_tcp: 135
 *  ncacn_np: \\pipe\epmapper
 *  ncalrpc: epmapper
 *  ncacn_http: 593
 *
 * If the user's machine ran a DCE RPC daemon, it would
 * probably be possible to connect to it, but there are many
 * reasons not to, like:
 *  - the user probably does *not* run one, and probably
 *    shouldn't be forced to run one just for local COM
 *  - very few Unix systems use DCE RPC... if they run a RPC
 *    daemon at all, it's usually Sun RPC
 *  - DCE RPC registrations are persistent and saved on disk,
 *    while MS-RPC registrations are documented as non-persistent
 *    and stored only in RAM, and auto-destroyed when the process
 *    dies (something DCE RPC can't do)
 *
 * Of course, if the user *did* want to run a DCE RPC daemon anyway,
 * there would be interoperability advantages, like the possibility
 * of running a fully functional DCOM server using Wine...
 */

static const struct epm_endpoints
{
    const char *protseq;
    const char *endpoint;
} epm_endpoints[] =
{
    { "ncacn_np", "\\pipe\\epmapper" },
    { "ncacn_ip_tcp", "135" },
    { "ncacn_ip_udp", "135" },
    { "ncalrpc", "epmapper" },
    { "ncacn_http", "593" },
};

static BOOL start_rpcss(void)
{
    SC_HANDLE scm, service;
    SERVICE_STATUS_PROCESS status;
    BOOL ret = FALSE;

    TRACE("\n");

    if (!(scm = OpenSCManagerW( NULL, NULL, 0 )))
    {
        ERR( "failed to open service manager\n" );
        return FALSE;
    }
    if (!(service = OpenServiceW( scm, L"RpcSs", SERVICE_START | SERVICE_QUERY_STATUS )))
    {
        ERR( "failed to open RpcSs service\n" );
        CloseServiceHandle( scm );
        return FALSE;
    }
    if (StartServiceW( service, 0, NULL ) || GetLastError() == ERROR_SERVICE_ALREADY_RUNNING)
    {
        ULONGLONG start_time = GetTickCount64();
        do
        {
            DWORD dummy;

            if (!QueryServiceStatusEx( service, SC_STATUS_PROCESS_INFO,
                                       (BYTE *)&status, sizeof(status), &dummy ))
                break;
            if (status.dwCurrentState == SERVICE_RUNNING)
            {
                ret = TRUE;
                break;
            }
            if (GetTickCount64() - start_time > 30000) break;
            Sleep( 100 );

        } while (status.dwCurrentState == SERVICE_START_PENDING);

        if (status.dwCurrentState != SERVICE_RUNNING)
            WARN( "RpcSs failed to start %lu\n", status.dwCurrentState );
    }
    else ERR( "failed to start RpcSs service\n" );

    CloseServiceHandle( service );
    CloseServiceHandle( scm );
    return ret;
}

static inline BOOL is_epm_destination_local(RPC_BINDING_HANDLE handle)
{
    RpcBinding *bind = handle;
    const char *protseq = bind->Protseq;
    const char *network_addr = bind->NetworkAddr;

    return (!strcmp(protseq, "ncalrpc") ||
           (!strcmp(protseq, "ncacn_np") &&
                (!network_addr || !strcmp(network_addr, "."))));
}

static RPC_STATUS get_epm_handle_client(RPC_BINDING_HANDLE handle, RPC_BINDING_HANDLE *epm_handle)
{
    RpcBinding *bind = handle;
    const char * pszEndpoint = NULL;
    RPC_STATUS status;
    RpcBinding* epm_bind;
    unsigned int i;

    if (bind->server)
        return RPC_S_INVALID_BINDING;

    for (i = 0; i < ARRAY_SIZE(epm_endpoints); i++)
        if (!strcmp(bind->Protseq, epm_endpoints[i].protseq))
            pszEndpoint = epm_endpoints[i].endpoint;

    if (!pszEndpoint)
    {
        FIXME("no endpoint for the endpoint-mapper found for protseq %s\n", debugstr_a(bind->Protseq));
        return RPC_S_PROTSEQ_NOT_SUPPORTED;
    }

    status = RpcBindingCopy(handle, epm_handle);
    if (status != RPC_S_OK) return status;

    epm_bind = *epm_handle;
    if (epm_bind->AuthInfo)
    {
        /* don't bother with authenticating against the EPM by default
        * (see EnableAuthEpResolution registry value) */
        RpcAuthInfo_Release(epm_bind->AuthInfo);
        epm_bind->AuthInfo = NULL;
    }
    RPCRT4_ResolveBinding(epm_bind, pszEndpoint);
    TRACE("RPC_S_OK\n");
    return RPC_S_OK;
}

static RPC_STATUS get_epm_handle_server(RPC_BINDING_HANDLE *epm_handle)
{
    unsigned char string_binding[] = "ncacn_np:.[\\\\pipe\\\\epmapper]";

    return RpcBindingFromStringBindingA(string_binding, epm_handle);
}

static LONG WINAPI rpc_filter(EXCEPTION_POINTERS *__eptr)
{
    switch (__eptr->ExceptionRecord->ExceptionCode)
    {
        case EXCEPTION_ACCESS_VIOLATION:
        case EXCEPTION_ILLEGAL_INSTRUCTION:
            return EXCEPTION_CONTINUE_SEARCH;
        default:
            return EXCEPTION_EXECUTE_HANDLER;
    }
}

static RPC_STATUS epm_register( RPC_IF_HANDLE IfSpec, RPC_BINDING_VECTOR *BindingVector,
                                UUID_VECTOR *UuidVector, RPC_CSTR Annotation, BOOL replace )
{
  PRPC_SERVER_INTERFACE If = IfSpec;
  ULONG i;
  RPC_STATUS status = RPC_S_OK;
  error_status_t status2;
  ept_entry_t *entries;
  handle_t handle;

  TRACE("(%p,%p,%p,%s) replace=%d\n", IfSpec, BindingVector, UuidVector, debugstr_a((char*)Annotation), replace);
  TRACE(" ifid=%s\n", debugstr_guid(&If->InterfaceId.SyntaxGUID));
  for (i=0; i<BindingVector->Count; i++) {
    RpcBinding* bind = BindingVector->BindingH[i];
    TRACE(" protseq[%ld]=%s\n", i, debugstr_a(bind->Protseq));
    TRACE(" endpoint[%ld]=%s\n", i, debugstr_a(bind->Endpoint));
  }
  if (UuidVector) {
    for (i=0; i<UuidVector->Count; i++)
      TRACE(" obj[%ld]=%s\n", i, debugstr_guid(UuidVector->Uuid[i]));
  }

  if (!BindingVector->Count) return RPC_S_OK;

  entries = calloc(BindingVector->Count * (UuidVector ? UuidVector->Count : 1), sizeof(*entries));
  if (!entries)
      return RPC_S_OUT_OF_MEMORY;

  status = get_epm_handle_server(&handle);
  if (status != RPC_S_OK)
  {
    free(entries);
    return status;
  }

  for (i = 0; i < BindingVector->Count; i++)
  {
      unsigned j;
      RpcBinding* bind = BindingVector->BindingH[i];
      for (j = 0; j < (UuidVector ? UuidVector->Count : 1); j++)
      {
          status = TowerConstruct(&If->InterfaceId, &If->TransferSyntax,
                                  bind->Protseq, bind->Endpoint,
                                  bind->NetworkAddr,
                                  &entries[i*(UuidVector ? UuidVector->Count : 1) + j].tower);
          if (status != RPC_S_OK) break;

          if (UuidVector)
              memcpy(&entries[i * UuidVector->Count].object, &UuidVector->Uuid[j], sizeof(GUID));
          else
              memset(&entries[i].object, 0, sizeof(entries[i].object));
          if (Annotation)
              memcpy(entries[i].annotation, Annotation,
                     min(strlen((char *)Annotation) + 1, ept_max_annotation_size));
      }
  }

  if (status == RPC_S_OK)
  {
      while (TRUE)
      {
          __TRY
          {
              ept_insert(handle, BindingVector->Count * (UuidVector ? UuidVector->Count : 1),
                         entries, replace, &status2);
          }
          __EXCEPT(rpc_filter)
          {
              status2 = GetExceptionCode();
          }
          __ENDTRY
          if (status2 == RPC_S_SERVER_UNAVAILABLE &&
              is_epm_destination_local(handle))
          {
              if (start_rpcss())
                  continue;
          }
          if (status2 != RPC_S_OK)
              ERR("ept_insert failed with error %ld\n", status2);
          status = status2; /* FIXME: convert status? */
          break;
      }
  }
  RpcBindingFree(&handle);

  for (i = 0; i < BindingVector->Count; i++)
  {
      unsigned j;
      for (j = 0; j < (UuidVector ? UuidVector->Count : 1); j++)
          I_RpcFree(entries[i*(UuidVector ? UuidVector->Count : 1) + j].tower);
  }

  free(entries);

  return status;
}

/***********************************************************************
 *             RpcEpRegisterA (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcEpRegisterA( RPC_IF_HANDLE IfSpec, RPC_BINDING_VECTOR *BindingVector,
                                  UUID_VECTOR *UuidVector, RPC_CSTR Annotation )
{
    return epm_register(IfSpec, BindingVector, UuidVector, Annotation, TRUE);
}

/***********************************************************************
 *             RpcEpRegisterNoReplaceA (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcEpRegisterNoReplaceA( RPC_IF_HANDLE IfSpec, RPC_BINDING_VECTOR *BindingVector,
                                           UUID_VECTOR *UuidVector, RPC_CSTR Annotation )
{
    return epm_register(IfSpec, BindingVector, UuidVector, Annotation, FALSE);
}

/***********************************************************************
 *             RpcEpRegisterW (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcEpRegisterW( RPC_IF_HANDLE IfSpec, RPC_BINDING_VECTOR *BindingVector,
                                  UUID_VECTOR *UuidVector, RPC_WSTR Annotation )
{
  LPSTR annA = RPCRT4_strdupWtoA(Annotation);
  RPC_STATUS status;

  status = epm_register(IfSpec, BindingVector, UuidVector, (RPC_CSTR)annA, TRUE);

  free(annA);
  return status;
}

/***********************************************************************
 *             RpcEpRegisterNoReplaceW (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcEpRegisterNoReplaceW( RPC_IF_HANDLE IfSpec, RPC_BINDING_VECTOR *BindingVector,
                                           UUID_VECTOR *UuidVector, RPC_WSTR Annotation )
{
  LPSTR annA = RPCRT4_strdupWtoA(Annotation);
  RPC_STATUS status;

  status = epm_register(IfSpec, BindingVector, UuidVector, (RPC_CSTR)annA, FALSE);

  free(annA);
  return status;
}

/***********************************************************************
 *             RpcEpUnregister (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcEpUnregister( RPC_IF_HANDLE IfSpec, RPC_BINDING_VECTOR *BindingVector,
                                   UUID_VECTOR *UuidVector )
{
  PRPC_SERVER_INTERFACE If = IfSpec;
  ULONG i;
  RPC_STATUS status = RPC_S_OK;
  error_status_t status2;
  ept_entry_t *entries;
  handle_t handle;

  TRACE("(%p,%p,%p)\n", IfSpec, BindingVector, UuidVector);
  TRACE(" ifid=%s\n", debugstr_guid(&If->InterfaceId.SyntaxGUID));
  for (i=0; i<BindingVector->Count; i++) {
    RpcBinding* bind = BindingVector->BindingH[i];
    TRACE(" protseq[%ld]=%s\n", i, debugstr_a(bind->Protseq));
    TRACE(" endpoint[%ld]=%s\n", i, debugstr_a(bind->Endpoint));
  }
  if (UuidVector) {
    for (i=0; i<UuidVector->Count; i++)
      TRACE(" obj[%ld]=%s\n", i, debugstr_guid(UuidVector->Uuid[i]));
  }

  entries = calloc(BindingVector->Count * (UuidVector ? UuidVector->Count : 1), sizeof(*entries));
  if (!entries)
      return RPC_S_OUT_OF_MEMORY;

  status = get_epm_handle_server(&handle);
  if (status != RPC_S_OK)
  {
    free(entries);
    return status;
  }

  for (i = 0; i < BindingVector->Count; i++)
  {
      unsigned j;
      RpcBinding* bind = BindingVector->BindingH[i];
      for (j = 0; j < (UuidVector ? UuidVector->Count : 1); j++)
      {
          status = TowerConstruct(&If->InterfaceId, &If->TransferSyntax,
                                  bind->Protseq, bind->Endpoint,
                                  bind->NetworkAddr,
                                  &entries[i*(UuidVector ? UuidVector->Count : 1) + j].tower);
          if (status != RPC_S_OK) break;

          if (UuidVector)
              memcpy(&entries[i * UuidVector->Count + j].object, &UuidVector->Uuid[j], sizeof(GUID));
          else
              memset(&entries[i].object, 0, sizeof(entries[i].object));
      }
  }

  if (status == RPC_S_OK)
  {
      __TRY
      {
          ept_insert(handle, BindingVector->Count * (UuidVector ? UuidVector->Count : 1),
                     entries, TRUE, &status2);
      }
      __EXCEPT(rpc_filter)
      {
          status2 = GetExceptionCode();
      }
      __ENDTRY
      if (status2 == RPC_S_SERVER_UNAVAILABLE)
          status2 = EPT_S_NOT_REGISTERED;
      if (status2 != RPC_S_OK)
          ERR("ept_insert failed with error %ld\n", status2);
      status = status2; /* FIXME: convert status? */
  }
  RpcBindingFree(&handle);

  for (i = 0; i < BindingVector->Count; i++)
  {
      unsigned j;
      for (j = 0; j < (UuidVector ? UuidVector->Count : 1); j++)
          I_RpcFree(entries[i*(UuidVector ? UuidVector->Count : 1) + j].tower);
  }

  free(entries);

  return status;
}

/***********************************************************************
 *             RpcEpResolveBinding (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcEpResolveBinding( RPC_BINDING_HANDLE Binding, RPC_IF_HANDLE IfSpec )
{
  PRPC_CLIENT_INTERFACE If = IfSpec;
  RpcBinding* bind = Binding;
  RPC_STATUS status;
  error_status_t status2;
  handle_t handle;
  ept_lookup_handle_t entry_handle = NULL;
  twr_t *tower;
  twr_t *towers[4] = { NULL };
  unsigned32 num_towers, i;
  GUID uuid = GUID_NULL;
  char *resolved_endpoint = NULL;

  TRACE("(%p,%p)\n", Binding, IfSpec);
  TRACE(" protseq=%s\n", debugstr_a(bind->Protseq));
  TRACE(" obj=%s\n", debugstr_guid(&bind->ObjectUuid));
  TRACE(" networkaddr=%s\n", debugstr_a(bind->NetworkAddr));
  TRACE(" ifid=%s\n", debugstr_guid(&If->InterfaceId.SyntaxGUID));

  /* just return for fully bound handles */
  if (bind->Endpoint && (bind->Endpoint[0] != '\0'))
    return RPC_S_OK;

  status = get_epm_handle_client(Binding, &handle);
  if (status != RPC_S_OK) return status;
  
  status = TowerConstruct(&If->InterfaceId, &If->TransferSyntax, bind->Protseq,
                          ((RpcBinding *)handle)->Endpoint,
                          bind->NetworkAddr, &tower);
  if (status != RPC_S_OK)
  {
      WARN("couldn't get tower\n");
      RpcBindingFree(&handle);
      return status;
  }

  while (TRUE)
  {
    __TRY
    {
      ept_map(handle, &uuid, tower, &entry_handle, ARRAY_SIZE(towers), &num_towers, towers, &status2);
      /* FIXME: translate status2? */
    }
    __EXCEPT(rpc_filter)
    {
      status2 = GetExceptionCode();
    }
    __ENDTRY
    if (status2 == RPC_S_SERVER_UNAVAILABLE &&
        is_epm_destination_local(handle))
    {
      if (start_rpcss())
        continue;
    }
    break;
  };

  RpcBindingFree(&handle);
  I_RpcFree(tower);

  if (status2 != RPC_S_OK)
  {
    ERR("ept_map failed for ifid %s, protseq %s, networkaddr %s\n", debugstr_guid(&If->TransferSyntax.SyntaxGUID), bind->Protseq, bind->NetworkAddr);
    return status2;
  }

  for (i = 0; i < num_towers; i++)
  {
    /* only parse the tower if we haven't already found a suitable
    * endpoint, otherwise just free the tower */
    if (!resolved_endpoint)
    {
      status = TowerExplode(towers[i], NULL, NULL, NULL, &resolved_endpoint, NULL);
      TRACE("status = %ld\n", status);
    }
    I_RpcFree(towers[i]);
  }

  if (resolved_endpoint)
  {
    RPCRT4_ResolveBinding(Binding, resolved_endpoint);
    I_RpcFree(resolved_endpoint);
    return RPC_S_OK;
  }

  WARN("couldn't find an endpoint\n");
  return EPT_S_NOT_REGISTERED;
}

/*****************************************************************************
 * TowerExplode (RPCRT4.@)
 */
RPC_STATUS WINAPI TowerExplode(
    const twr_t *tower, PRPC_SYNTAX_IDENTIFIER object, PRPC_SYNTAX_IDENTIFIER syntax,
    char **protseq, char **endpoint, char **address)
{
    size_t tower_size;
    RPC_STATUS status;
    const unsigned char *p;
    u_int16 floor_count;
    const twr_uuid_floor_t *object_floor;
    const twr_uuid_floor_t *syntax_floor;

    TRACE("(%p, %p, %p, %p, %p, %p)\n", tower, object, syntax, protseq,
          endpoint, address);

    if (protseq)
        *protseq = NULL;
    if (endpoint)
        *endpoint = NULL;
    if (address)
        *address = NULL;

    tower_size = tower->tower_length;

    if (tower_size < sizeof(u_int16))
        return EPT_S_NOT_REGISTERED;

    p = &tower->tower_octet_string[0];

    floor_count = *(const u_int16 *)p;
    p += sizeof(u_int16);
    tower_size -= sizeof(u_int16);
    TRACE("floor_count: %d\n", floor_count);
    /* FIXME: should we do something with the floor count? at the moment we don't */

    if (tower_size < sizeof(*object_floor) + sizeof(*syntax_floor))
        return EPT_S_NOT_REGISTERED;

    object_floor = (const twr_uuid_floor_t *)p;
    p += sizeof(*object_floor);
    tower_size -= sizeof(*object_floor);
    syntax_floor = (const twr_uuid_floor_t *)p;
    p += sizeof(*syntax_floor);
    tower_size -= sizeof(*syntax_floor);

    if ((object_floor->count_lhs != sizeof(object_floor->protid) +
        sizeof(object_floor->uuid) + sizeof(object_floor->major_version)) ||
        (object_floor->protid != EPM_PROTOCOL_UUID) ||
        (object_floor->count_rhs != sizeof(object_floor->minor_version)))
        return EPT_S_NOT_REGISTERED;

    if ((syntax_floor->count_lhs != sizeof(syntax_floor->protid) +
        sizeof(syntax_floor->uuid) + sizeof(syntax_floor->major_version)) ||
        (syntax_floor->protid != EPM_PROTOCOL_UUID) ||
        (syntax_floor->count_rhs != sizeof(syntax_floor->minor_version)))
        return EPT_S_NOT_REGISTERED;

    status = RpcTransport_ParseTopOfTower(p, tower_size, protseq, address, endpoint);
    if ((status == RPC_S_OK) && syntax && object)
    {
        syntax->SyntaxGUID = syntax_floor->uuid;
        syntax->SyntaxVersion.MajorVersion = syntax_floor->major_version;
        syntax->SyntaxVersion.MinorVersion = syntax_floor->minor_version;
        object->SyntaxGUID = object_floor->uuid;
        object->SyntaxVersion.MajorVersion = object_floor->major_version;
        object->SyntaxVersion.MinorVersion = object_floor->minor_version;
    }
    return status;
}

/***********************************************************************
 *             TowerConstruct (RPCRT4.@)
 */
RPC_STATUS WINAPI TowerConstruct(
    const RPC_SYNTAX_IDENTIFIER *object, const RPC_SYNTAX_IDENTIFIER *syntax,
    const char *protseq, const char *endpoint, const char *address,
    twr_t **tower)
{
    size_t tower_size;
    RPC_STATUS status;
    unsigned char *p;
    twr_uuid_floor_t *object_floor;
    twr_uuid_floor_t *syntax_floor;

    TRACE("(%p, %p, %s, %s, %s, %p)\n", object, syntax, debugstr_a(protseq),
          debugstr_a(endpoint), debugstr_a(address), tower);

    *tower = NULL;

    status = RpcTransport_GetTopOfTower(NULL, &tower_size, protseq, address, endpoint);

    if (status != RPC_S_OK)
        return status;

    tower_size += sizeof(u_int16) + sizeof(*object_floor) + sizeof(*syntax_floor);
    *tower = I_RpcAllocate(FIELD_OFFSET(twr_t, tower_octet_string[tower_size]));
    if (!*tower)
        return RPC_S_OUT_OF_RESOURCES;

    (*tower)->tower_length = tower_size;
    p = &(*tower)->tower_octet_string[0];
    *(u_int16 *)p = 5; /* number of floors */
    p += sizeof(u_int16);
    object_floor = (twr_uuid_floor_t *)p;
    p += sizeof(*object_floor);
    syntax_floor = (twr_uuid_floor_t *)p;
    p += sizeof(*syntax_floor);

    object_floor->count_lhs = sizeof(object_floor->protid) + sizeof(object_floor->uuid) +
                              sizeof(object_floor->major_version);
    object_floor->protid = EPM_PROTOCOL_UUID;
    object_floor->count_rhs = sizeof(object_floor->minor_version);
    object_floor->uuid = object->SyntaxGUID;
    object_floor->major_version = object->SyntaxVersion.MajorVersion;
    object_floor->minor_version = object->SyntaxVersion.MinorVersion;

    syntax_floor->count_lhs = sizeof(syntax_floor->protid) + sizeof(syntax_floor->uuid) +
                              sizeof(syntax_floor->major_version);
    syntax_floor->protid = EPM_PROTOCOL_UUID;
    syntax_floor->count_rhs = sizeof(syntax_floor->minor_version);
    syntax_floor->uuid = syntax->SyntaxGUID;
    syntax_floor->major_version = syntax->SyntaxVersion.MajorVersion;
    syntax_floor->minor_version = syntax->SyntaxVersion.MinorVersion;

    status = RpcTransport_GetTopOfTower(p, &tower_size, protseq, address, endpoint);
    if (status != RPC_S_OK)
    {
        I_RpcFree(*tower);
        *tower = NULL;
        return status;
    }
    return RPC_S_OK;
}

void __RPC_FAR * __RPC_USER MIDL_user_allocate(SIZE_T len)
{
    return malloc(len);
}

void __RPC_USER MIDL_user_free(void __RPC_FAR * ptr)
{
    free(ptr);
}
