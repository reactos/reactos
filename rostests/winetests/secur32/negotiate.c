/*
 * Tests for the Negotiate security provider
 *
 * Copyright 2005, 2006 Kai Blin
 * Copyright 2012 Hans Leidekker for CodeWeavers
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
#include <stdio.h>
#include <windef.h>
#include <winbase.h>
#define SECURITY_WIN32
#include <sspi.h>
#include <rpc.h>
#include <rpcdce.h>
#include <secext.h>

#include "wine/test.h"

static HMODULE hsecur32;
static SECURITY_STATUS (SEC_ENTRY * pAcceptSecurityContext)(PCredHandle, PCtxtHandle,
    PSecBufferDesc, ULONG, ULONG, PCtxtHandle, PSecBufferDesc, PULONG, PTimeStamp);
static SECURITY_STATUS (SEC_ENTRY * pAcquireCredentialsHandleA)(SEC_CHAR *, SEC_CHAR *,
    ULONG, PLUID, PVOID, SEC_GET_KEY_FN, PVOID, PCredHandle, PTimeStamp);
static SECURITY_STATUS (SEC_ENTRY * pCompleteAuthToken)(PCtxtHandle, PSecBufferDesc);
static SECURITY_STATUS (SEC_ENTRY * pDecryptMessage)(PCtxtHandle, PSecBufferDesc, ULONG, PULONG);
static SECURITY_STATUS (SEC_ENTRY * pDeleteSecurityContext)(PCtxtHandle);
static SECURITY_STATUS (SEC_ENTRY * pEncryptMessage)(PCtxtHandle, ULONG, PSecBufferDesc, ULONG);
static SECURITY_STATUS (SEC_ENTRY * pFreeContextBuffer)(PVOID);
static SECURITY_STATUS (SEC_ENTRY * pFreeCredentialsHandle)(PCredHandle);
static SECURITY_STATUS (SEC_ENTRY * pInitializeSecurityContextA)(PCredHandle, PCtxtHandle,
    SEC_CHAR *, ULONG, ULONG, ULONG, PSecBufferDesc, ULONG, PCtxtHandle, PSecBufferDesc,
    PULONG, PTimeStamp);
static PSecurityFunctionTableA (SEC_ENTRY * pInitSecurityInterfaceA)(void);
static SECURITY_STATUS (SEC_ENTRY * pMakeSignature)(PCtxtHandle, ULONG, PSecBufferDesc, ULONG);
static SECURITY_STATUS (SEC_ENTRY * pQueryContextAttributesA)(PCtxtHandle, ULONG, PVOID);
static SECURITY_STATUS (SEC_ENTRY * pQuerySecurityPackageInfoA)(SEC_CHAR *, PSecPkgInfoA *);
static SECURITY_STATUS (SEC_ENTRY * pVerifySignature)(PCtxtHandle, PSecBufferDesc, ULONG, PULONG);

static void init_function_ptrs(void)
{
    if (!(hsecur32 = LoadLibraryA("secur32.dll"))) return;
    pAcceptSecurityContext      = (void *)GetProcAddress(hsecur32, "AcceptSecurityContext");
    pAcquireCredentialsHandleA  = (void *)GetProcAddress(hsecur32, "AcquireCredentialsHandleA");
    pCompleteAuthToken          = (void *)GetProcAddress(hsecur32, "CompleteAuthToken");
    pDecryptMessage             = (void *)GetProcAddress(hsecur32, "DecryptMessage");
    pDeleteSecurityContext      = (void *)GetProcAddress(hsecur32, "DeleteSecurityContext");
    pEncryptMessage             = (void *)GetProcAddress(hsecur32, "EncryptMessage");
    pFreeContextBuffer          = (void *)GetProcAddress(hsecur32, "FreeContextBuffer");
    pFreeCredentialsHandle      = (void *)GetProcAddress(hsecur32, "FreeCredentialsHandle");
    pInitializeSecurityContextA = (void *)GetProcAddress(hsecur32, "InitializeSecurityContextA");
    pInitSecurityInterfaceA     = (void *)GetProcAddress(hsecur32, "InitSecurityInterfaceA");
    pMakeSignature              = (void *)GetProcAddress(hsecur32, "MakeSignature");
    pQueryContextAttributesA    = (void *)GetProcAddress(hsecur32, "QueryContextAttributesA");
    pQuerySecurityPackageInfoA  = (void *)GetProcAddress(hsecur32, "QuerySecurityPackageInfoA");
    pVerifySignature            = (void *)GetProcAddress(hsecur32, "VerifySignature");
}

#define NEGOTIATE_BASE_CAPS ( \
    SECPKG_FLAG_INTEGRITY  | \
    SECPKG_FLAG_PRIVACY    | \
    SECPKG_FLAG_CONNECTION | \
    SECPKG_FLAG_MULTI_REQUIRED | \
    SECPKG_FLAG_EXTENDED_ERROR | \
    SECPKG_FLAG_IMPERSONATION  | \
    SECPKG_FLAG_ACCEPT_WIN32_NAME | \
    SECPKG_FLAG_NEGOTIABLE        | \
    SECPKG_FLAG_GSS_COMPATIBLE    | \
    SECPKG_FLAG_LOGON )

#define NTLM_BASE_CAPS ( \
    SECPKG_FLAG_INTEGRITY  | \
    SECPKG_FLAG_PRIVACY    | \
    SECPKG_FLAG_TOKEN_ONLY | \
    SECPKG_FLAG_CONNECTION | \
    SECPKG_FLAG_MULTI_REQUIRED    | \
    SECPKG_FLAG_IMPERSONATION     | \
    SECPKG_FLAG_ACCEPT_WIN32_NAME | \
    SECPKG_FLAG_NEGOTIABLE        | \
    SECPKG_FLAG_LOGON )

struct sspi_data
{
    CredHandle cred;
    CtxtHandle ctxt;
    PSecBufferDesc in_buf;
    PSecBufferDesc out_buf;
    PSEC_WINNT_AUTH_IDENTITY_A id;
    ULONG max_token;
};

static void cleanup_buffers( struct sspi_data *data )
{
    unsigned int i;

    if (data->in_buf)
    {
        for (i = 0; i < data->in_buf->cBuffers; ++i)
            HeapFree( GetProcessHeap(), 0, data->in_buf->pBuffers[i].pvBuffer );
        HeapFree( GetProcessHeap(), 0, data->in_buf->pBuffers );
        HeapFree( GetProcessHeap(), 0, data->in_buf );
    }
    if (data->out_buf)
    {
        for (i = 0; i < data->out_buf->cBuffers; ++i)
            HeapFree( GetProcessHeap(), 0, data->out_buf->pBuffers[i].pvBuffer );
        HeapFree( GetProcessHeap(), 0, data->out_buf->pBuffers );
        HeapFree( GetProcessHeap(), 0, data->out_buf );
    }
}

static void setup_buffers( struct sspi_data *data, SecPkgInfoA *info )
{
    SecBuffer *buffer = HeapAlloc( GetProcessHeap(), 0, sizeof(SecBuffer) );

    data->in_buf  = HeapAlloc( GetProcessHeap(), 0, sizeof(SecBufferDesc) );
    data->out_buf = HeapAlloc( GetProcessHeap(), 0, sizeof(SecBufferDesc) );
    data->max_token = info->cbMaxToken;

    data->in_buf->ulVersion = SECBUFFER_VERSION;
    data->in_buf->cBuffers  = 1;
    data->in_buf->pBuffers  = buffer;

    buffer->cbBuffer   = info->cbMaxToken;
    buffer->BufferType = SECBUFFER_TOKEN;
    buffer->pvBuffer   = HeapAlloc( GetProcessHeap(), 0, info->cbMaxToken );

    buffer = HeapAlloc( GetProcessHeap(), 0, sizeof(SecBuffer) );

    data->out_buf->ulVersion = SECBUFFER_VERSION;
    data->out_buf->cBuffers  = 1;
    data->out_buf->pBuffers  = buffer;

    buffer->cbBuffer   = info->cbMaxToken;
    buffer->BufferType = SECBUFFER_TOKEN;
    buffer->pvBuffer   = HeapAlloc( GetProcessHeap(), 0, info->cbMaxToken );
}

static SECURITY_STATUS setup_client( struct sspi_data *data, SEC_CHAR *provider )
{
    SECURITY_STATUS ret;
    SecPkgInfoA *info;
    TimeStamp ttl;

    trace( "setting up client\n" );

    ret = pQuerySecurityPackageInfoA( provider, &info );
    ok( ret == SEC_E_OK, "QuerySecurityPackageInfo returned %08x\n", ret );

    setup_buffers( data, info );
    pFreeContextBuffer( info );

    ret = pAcquireCredentialsHandleA( NULL, provider, SECPKG_CRED_OUTBOUND, NULL,
                                      data->id, NULL, NULL, &data->cred, &ttl );
    ok( ret == SEC_E_OK, "AcquireCredentialsHandleA returned %08x\n", ret );
    return ret;
}

static SECURITY_STATUS setup_server( struct sspi_data *data, SEC_CHAR *provider )
{
    SECURITY_STATUS ret;
    SecPkgInfoA *info;
    TimeStamp ttl;

    trace( "setting up server\n" );

    ret = pQuerySecurityPackageInfoA( provider, &info );
    ok( ret == SEC_E_OK, "QuerySecurityPackageInfo returned %08x\n", ret );

    setup_buffers( data, info );
    pFreeContextBuffer( info );

    ret = pAcquireCredentialsHandleA( NULL, provider, SECPKG_CRED_INBOUND, NULL,
                                      NULL, NULL, NULL, &data->cred, &ttl );
    ok( ret == SEC_E_OK, "AcquireCredentialsHandleA returned %08x\n", ret );
    return ret;
}

static SECURITY_STATUS run_client( struct sspi_data *data, BOOL first )
{
    SECURITY_STATUS ret;
    TimeStamp ttl;
    ULONG attr;

    trace( "running client for the %s time\n", first ? "first" : "second" );

    data->out_buf->pBuffers[0].cbBuffer = data->max_token;
    data->out_buf->pBuffers[0].BufferType = SECBUFFER_TOKEN;

    ret = pInitializeSecurityContextA( first ? &data->cred : NULL, first ? NULL : &data->ctxt,
                                       NULL, 0, 0, SECURITY_NETWORK_DREP, first ? NULL : data->in_buf,
                                       0, &data->ctxt, data->out_buf, &attr, &ttl );
    if (ret == SEC_I_COMPLETE_AND_CONTINUE || ret == SEC_I_COMPLETE_NEEDED)
    {
        pCompleteAuthToken( &data->ctxt, data->out_buf );
        if (ret == SEC_I_COMPLETE_AND_CONTINUE)
            ret = SEC_I_CONTINUE_NEEDED;
        else if (ret == SEC_I_COMPLETE_NEEDED)
            ret = SEC_E_OK;
    }
    ok( data->out_buf->pBuffers[0].BufferType == SECBUFFER_TOKEN,
        "buffer type changed from SECBUFFER_TOKEN to %u\n", data->out_buf->pBuffers[0].BufferType );
    ok( data->out_buf->pBuffers[0].cbBuffer < data->max_token,
        "InitializeSecurityContext didn't change buffer size\n" );
    return ret;
}

static SECURITY_STATUS run_server( struct sspi_data *data, BOOL first )
{
    SECURITY_STATUS ret;
    TimeStamp ttl;
    ULONG attr;

    trace( "running server for the %s time\n", first ? "first" : "second" );

    ret = pAcceptSecurityContext( &data->cred, first ? NULL : &data->ctxt,
                                  data->in_buf, 0, SECURITY_NETWORK_DREP,
                                  &data->ctxt, data->out_buf, &attr, &ttl );
    if (ret == SEC_I_COMPLETE_AND_CONTINUE || ret == SEC_I_COMPLETE_NEEDED)
    {
        pCompleteAuthToken( &data->ctxt, data->out_buf );
        if (ret == SEC_I_COMPLETE_AND_CONTINUE)
            ret = SEC_I_CONTINUE_NEEDED;
        else if (ret == SEC_I_COMPLETE_NEEDED)
            ret = SEC_E_OK;
    }
    return ret;
}

static void communicate( struct sspi_data *from, struct sspi_data *to )
{
    trace( "running communicate\n" );
    memset( to->in_buf->pBuffers[0].pvBuffer, 0, to->max_token );
    memcpy( to->in_buf->pBuffers[0].pvBuffer, from->out_buf->pBuffers[0].pvBuffer,
            from->out_buf->pBuffers[0].cbBuffer );
    to->in_buf->pBuffers[0].cbBuffer = from->out_buf->pBuffers[0].cbBuffer;
    memset( from->out_buf->pBuffers[0].pvBuffer, 0, from->max_token );
}

static void test_authentication(void)
{
    SECURITY_STATUS status_c = SEC_I_CONTINUE_NEEDED,
                    status_s = SEC_I_CONTINUE_NEEDED, status;
    struct sspi_data client, server;
    SEC_WINNT_AUTH_IDENTITY_A id;
    SecPkgContext_NegotiationInfoA info;
    SecPkgContext_Sizes sizes;
    SecPkgInfoA *pi;
    BOOL first = TRUE;

    memset(&client, 0, sizeof(client));
    memset(&server, 0, sizeof(server));

    id.User = (unsigned char *)"user";
    id.UserLength = strlen( "user" );
    id.Domain = (unsigned char *)"domain";
    id.DomainLength = strlen( "domain" );
    id.Password = (unsigned char *)"password";
    id.PasswordLength = strlen( "password" );
    id.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;

    client.id = &id;
    if ((status = setup_client( &client, (SEC_CHAR *)"Negotiate" )))
    {
        skip( "setup_client returned %08x, skipping test\n", status );
        return;
    }
    if ((status = setup_server( &server, (SEC_CHAR *)"Negotiate" )))
    {
        skip( "setup_server returned %08x, skipping test\n", status );
        pFreeCredentialsHandle( &client.cred );
        return;
    }

    while (status_c == SEC_I_CONTINUE_NEEDED && status_s == SEC_I_CONTINUE_NEEDED)
    {
        status_c = run_client( &client, first );
        ok( status_c == SEC_E_OK || status_c == SEC_I_CONTINUE_NEEDED,
            "client returned %08x, more tests will fail\n", status_c );

        communicate( &client, &server );

        status_s = run_server( &server, first );
        ok( status_s == SEC_E_OK || status_s == SEC_I_CONTINUE_NEEDED ||
            status_s == SEC_E_LOGON_DENIED,
            "server returned %08x, more tests will fail\n", status_s );

        communicate( &server, &client );
        trace( "looping\n");
        first = FALSE;
    }
    if (status_c != SEC_E_OK)
    {
        skip( "authentication failed, skipping remaining tests\n" );
        goto done;
    }

    sizes.cbMaxToken        = 0xdeadbeef;
    sizes.cbMaxSignature    = 0xdeadbeef;
    sizes.cbSecurityTrailer = 0xdeadbeef;
    sizes.cbBlockSize       = 0xdeadbeef;
    status_c = pQueryContextAttributesA( &client.ctxt, SECPKG_ATTR_SIZES, &sizes );
    ok( status_c == SEC_E_OK, "pQueryContextAttributesA returned %08x\n", status_c );
    ok( sizes.cbMaxToken == 2888 || sizes.cbMaxToken == 1904,
        "expected 2888 or 1904, got %u\n", sizes.cbMaxToken );
    ok( sizes.cbMaxSignature == 16, "expected 16, got %u\n", sizes.cbMaxSignature );
    ok( sizes.cbSecurityTrailer == 16, "expected 16, got %u\n", sizes.cbSecurityTrailer );
    ok( !sizes.cbBlockSize, "expected 0, got %u\n", sizes.cbBlockSize );

    memset( &info, 0, sizeof(info) );
    status_c = pQueryContextAttributesA( &client.ctxt, SECPKG_ATTR_NEGOTIATION_INFO, &info );
    ok( status_c == SEC_E_OK, "pQueryContextAttributesA returned %08x\n", status_c );

    pi = info.PackageInfo;
    ok( info.NegotiationState == SECPKG_NEGOTIATION_COMPLETE, "got %u\n", info.NegotiationState );
    ok( pi != NULL, "expected non-NULL PackageInfo\n" );
    if (pi)
    {
        ok( pi->fCapabilities == NTLM_BASE_CAPS ||
            pi->fCapabilities == (NTLM_BASE_CAPS|SECPKG_FLAG_READONLY_WITH_CHECKSUM) ||
            pi->fCapabilities == (NTLM_BASE_CAPS|SECPKG_FLAG_RESTRICTED_TOKENS) ||
            pi->fCapabilities == (NTLM_BASE_CAPS|SECPKG_FLAG_RESTRICTED_TOKENS|
                                  SECPKG_FLAG_APPCONTAINER_CHECKS),
            "got %08x\n", pi->fCapabilities );
        ok( pi->wVersion == 1, "got %u\n", pi->wVersion );
        ok( pi->wRPCID == RPC_C_AUTHN_WINNT, "got %u\n", pi->wRPCID );
        ok( !lstrcmpA( pi->Name, "NTLM" ), "got %s\n", pi->Name );
    }

done:
    cleanup_buffers( &client );
    cleanup_buffers( &server );

    if (client.ctxt.dwLower || client.ctxt.dwUpper)
    {
        status_c = pDeleteSecurityContext( &client.ctxt );
        ok( status_c == SEC_E_OK, "DeleteSecurityContext returned %08x\n", status_c );
    }

    if (server.ctxt.dwLower || server.ctxt.dwUpper)
    {
        status_s = pDeleteSecurityContext( &server.ctxt );
        ok( status_s == SEC_E_OK, "DeleteSecurityContext returned %08x\n", status_s );
    }

    if (client.cred.dwLower || client.cred.dwUpper)
    {
        status_c = pFreeCredentialsHandle( &client.cred );
        ok( status_c == SEC_E_OK, "FreeCredentialsHandle returned %08x\n", status_c );
    }

    if (server.cred.dwLower || server.cred.dwUpper)
    {
        status_s = pFreeCredentialsHandle(&server.cred);
        ok( status_s == SEC_E_OK, "FreeCredentialsHandle returned %08x\n", status_s );
    }
}

START_TEST(negotiate)
{
    SecPkgInfoA *info;

    init_function_ptrs();

    if (!pFreeCredentialsHandle || !pAcquireCredentialsHandleA || !pQuerySecurityPackageInfoA ||
        !pFreeContextBuffer)
    {
        win_skip("functions are not available\n");
        return;
    }
    if (pQuerySecurityPackageInfoA( (SEC_CHAR *)"Negotiate", &info ))
    {
        ok( 0, "Negotiate package not installed, skipping test\n" );
        return;
    }
    ok( info->fCapabilities == NEGOTIATE_BASE_CAPS ||
        info->fCapabilities == (NEGOTIATE_BASE_CAPS|SECPKG_FLAG_READONLY_WITH_CHECKSUM) ||
        info->fCapabilities == (NEGOTIATE_BASE_CAPS|SECPKG_FLAG_RESTRICTED_TOKENS) ||
        info->fCapabilities == (NEGOTIATE_BASE_CAPS|SECPKG_FLAG_RESTRICTED_TOKENS|
                                SECPKG_FLAG_APPCONTAINER_CHECKS),
        "got %08x\n", info->fCapabilities );
    ok( info->wVersion == 1, "got %u\n", info->wVersion );
    ok( info->wRPCID == RPC_C_AUTHN_GSS_NEGOTIATE, "got %u\n", info->wRPCID );
    ok( !lstrcmpA( info->Name, "Negotiate" ), "got %s\n", info->Name );
    pFreeContextBuffer( info );

    test_authentication();
}
