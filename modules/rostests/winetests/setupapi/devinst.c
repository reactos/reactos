/*
 * Devinst tests
 *
 * Copyright 2006 Christian Gmeiner
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
#include <time.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winnls.h"
#include "winuser.h"
#include "winreg.h"
#include "ntsecapi.h"
#include "wincrypt.h"
#include "mscat.h"
#include "devguid.h"
#include "initguid.h"
#include "devpkey.h"
#include "setupapi.h"
#include "cfgmgr32.h"
#include "cguid.h"
#include "fci.h"

#include "wine/test.h"
#include "wine/mssign.h"

/* This is a unique guid for testing purposes */
static GUID guid = {0x6a55b5a4, 0x3f65, 0x11db, {0xb7,0x04,0x00,0x11,0x95,0x5c,0x2b,0xdb}};
static GUID guid2 = {0x6a55b5a5, 0x3f65, 0x11db, {0xb7,0x04,0x00,0x11,0x95,0x5c,0x2b,0xdb}};
static GUID iface_guid = {0xdeadbeef, 0x3f65, 0x11db, {0xb7,0x04,0x00,0x11,0x95,0x5c,0x2b,0xdb}};
static GUID iface_guid2 = {0xdeadf00d, 0x3f65, 0x11db, {0xb7,0x04,0x00,0x11,0x95,0x5c,0x2b,0xdb}};

static HRESULT (WINAPI *pDriverStoreAddDriverPackageA)(const char *inf_path, void *unk1,
        void *unk2, WORD architecture, char *ret_path, DWORD *ret_len);
static HRESULT (WINAPI *pDriverStoreDeleteDriverPackageA)(const char *path, void *unk1, void *unk2);
static HRESULT (WINAPI *pDriverStoreFindDriverPackageA)(const char *inf_path, void *unk1,
        void *unk2, WORD architecture, void *unk4, char *ret_path, DWORD *ret_len);
static BOOL (WINAPI *pSetupDiSetDevicePropertyW)(HDEVINFO, SP_DEVINFO_DATA *, const DEVPROPKEY *, DEVPROPTYPE, const BYTE *, DWORD, DWORD);
static BOOL (WINAPI *pSetupDiGetDevicePropertyW)(HDEVINFO, SP_DEVINFO_DATA *, const DEVPROPKEY *, DEVPROPTYPE *, BYTE *, DWORD, DWORD *, DWORD);
static BOOL (WINAPI *pSetupQueryInfOriginalFileInformationA)(SP_INF_INFORMATION *, UINT, SP_ALTPLATFORM_INFO *, SP_ORIGINAL_FILE_INFO_A *);
static BOOL (WINAPI *pSetupDiGetDevicePropertyKeys)(HDEVINFO, PSP_DEVINFO_DATA, DEVPROPKEY *,DWORD, DWORD *, DWORD);

static BOOL wow64;

static void create_directory(const char *name)
{
    BOOL ret = CreateDirectoryA(name, NULL);
    ok(ret, "Failed to create %s, error %lu.\n", name, GetLastError());
}

static void create_file(const char *name, const char *data)
{
    HANDLE file;
    DWORD size;
    BOOL ret;

    file = CreateFileA(name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "Failed to create %s, error %lu.\n", name, GetLastError());
    ret = WriteFile(file, data, strlen(data), &size, NULL);
    ok(ret && size == strlen(data), "Failed to write %s, error %lu.\n", name, GetLastError());
    CloseHandle(file);
}

static void delete_directory(const char *name)
{
    BOOL ret = RemoveDirectoryA(name);
    ok(ret, "Failed to delete %s, error %lu.\n", name, GetLastError());
}

static void delete_file(const char *name)
{
    BOOL ret = DeleteFileA(name);
    ok(ret, "Failed to delete %s, error %lu.\n", name, GetLastError());
}

static void load_resource(const char *name, const char *filename)
{
    DWORD written;
    HANDLE file;
    HRSRC res;
    void *ptr;

    file = CreateFileA(filename, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "file creation failed, at %s, error %ld\n", filename, GetLastError());

    res = FindResourceA(NULL, name, "TESTDLL");
    ok( res != 0, "couldn't find resource\n" );
    ptr = LockResource( LoadResource( GetModuleHandleA(NULL), res ));
    WriteFile( file, ptr, SizeofResource( GetModuleHandleA(NULL), res ), &written, NULL );
    ok( written == SizeofResource( GetModuleHandleA(NULL), res ), "couldn't write resource\n" );
    CloseHandle( file );
}

struct testsign_context
{
    HCRYPTPROV provider;
    const CERT_CONTEXT *cert, *root_cert, *publisher_cert;
    HCERTSTORE root_store, publisher_store;
};

static BOOL testsign_create_cert(struct testsign_context *ctx)
{
    BYTE encoded_name[100], encoded_key_id[200], public_key_info_buffer[1000];
    WCHAR container_name[26];
    BYTE hash_buffer[16], cert_buffer[1000], provider_nameA[100], serial[16];
    CERT_PUBLIC_KEY_INFO *public_key_info = (CERT_PUBLIC_KEY_INFO *)public_key_info_buffer;
    CRYPT_KEY_PROV_INFO provider_info = {0};
    CRYPT_ALGORITHM_IDENTIFIER algid = {0};
    CERT_AUTHORITY_KEY_ID_INFO key_info;
    CERT_INFO cert_info = {0};
    WCHAR provider_nameW[100];
    CERT_EXTENSION extension;
    HCRYPTKEY key;
    DWORD size;
    BOOL ret;

    memset(ctx, 0, sizeof(*ctx));

    srand(time(NULL));
    swprintf(container_name, ARRAY_SIZE(container_name), L"wine_testsign%u", rand());

    ret = CryptAcquireContextW(&ctx->provider, container_name, NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET);
    ok(ret, "Failed to create container, error %#lx\n", GetLastError());

    ret = CryptGenKey(ctx->provider, AT_SIGNATURE, CRYPT_EXPORTABLE, &key);
    ok(ret, "Failed to create key, error %#lx\n", GetLastError());
    ret = CryptDestroyKey(key);
    ok(ret, "Failed to destroy key, error %#lx\n", GetLastError());
    ret = CryptGetUserKey(ctx->provider, AT_SIGNATURE, &key);
    ok(ret, "Failed to get user key, error %#lx\n", GetLastError());
    ret = CryptDestroyKey(key);
    ok(ret, "Failed to destroy key, error %#lx\n", GetLastError());

    size = sizeof(encoded_name);
    ret = CertStrToNameA(X509_ASN_ENCODING, "CN=winetest_cert", CERT_X500_NAME_STR, NULL, encoded_name, &size, NULL);
    ok(ret, "Failed to convert name, error %#lx\n", GetLastError());
    key_info.CertIssuer.cbData = size;
    key_info.CertIssuer.pbData = encoded_name;

    size = sizeof(public_key_info_buffer);
    ret = CryptExportPublicKeyInfo(ctx->provider, AT_SIGNATURE, X509_ASN_ENCODING, public_key_info, &size);
    ok(ret, "Failed to export public key, error %#lx\n", GetLastError());
    cert_info.SubjectPublicKeyInfo = *public_key_info;

    size = sizeof(hash_buffer);
    ret = CryptHashPublicKeyInfo(ctx->provider, CALG_MD5, 0, X509_ASN_ENCODING, public_key_info, hash_buffer, &size);
    ok(ret, "Failed to hash public key, error %#lx\n", GetLastError());

    key_info.KeyId.cbData = size;
    key_info.KeyId.pbData = hash_buffer;

    RtlGenRandom(serial, sizeof(serial));
    key_info.CertSerialNumber.cbData = sizeof(serial);
    key_info.CertSerialNumber.pbData = serial;

    size = sizeof(encoded_key_id);
    ret = CryptEncodeObject(X509_ASN_ENCODING, X509_AUTHORITY_KEY_ID, &key_info, encoded_key_id, &size);
    ok(ret, "Failed to convert name, error %#lx\n", GetLastError());

    extension.pszObjId = (char *)szOID_AUTHORITY_KEY_IDENTIFIER;
    extension.fCritical = TRUE;
    extension.Value.cbData = size;
    extension.Value.pbData = encoded_key_id;

    cert_info.dwVersion = CERT_V3;
    cert_info.SerialNumber = key_info.CertSerialNumber;
    cert_info.SignatureAlgorithm.pszObjId = (char *)szOID_RSA_SHA1RSA;
    cert_info.Issuer = key_info.CertIssuer;
    GetSystemTimeAsFileTime(&cert_info.NotBefore);
    GetSystemTimeAsFileTime(&cert_info.NotAfter);
    cert_info.NotAfter.dwHighDateTime += 1;
    cert_info.Subject = key_info.CertIssuer;
    cert_info.cExtension = 1;
    cert_info.rgExtension = &extension;
    algid.pszObjId = (char *)szOID_RSA_SHA1RSA;
    size = sizeof(cert_buffer);
    ret = CryptSignAndEncodeCertificate(ctx->provider, AT_SIGNATURE, X509_ASN_ENCODING,
            X509_CERT_TO_BE_SIGNED, &cert_info, &algid, NULL, cert_buffer, &size);
    ok(ret, "Failed to create certificate, error %#lx\n", GetLastError());

    ctx->cert = CertCreateCertificateContext(X509_ASN_ENCODING, cert_buffer, size);
    ok(!!ctx->cert, "Failed to create context, error %#lx\n", GetLastError());

    size = sizeof(provider_nameA);
    ret = CryptGetProvParam(ctx->provider, PP_NAME, provider_nameA, &size, 0);
    ok(ret, "Failed to get prov param, error %#lx\n", GetLastError());
    MultiByteToWideChar(CP_ACP, 0, (char *)provider_nameA, -1, provider_nameW, ARRAY_SIZE(provider_nameW));

    provider_info.pwszContainerName = (WCHAR *)container_name;
    provider_info.pwszProvName = provider_nameW;
    provider_info.dwProvType = PROV_RSA_FULL;
    provider_info.dwKeySpec = AT_SIGNATURE;
    ret = CertSetCertificateContextProperty(ctx->cert, CERT_KEY_PROV_INFO_PROP_ID, 0, &provider_info);
    ok(ret, "Failed to set provider info, error %#lx\n", GetLastError());

    ctx->root_store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY_A, 0, 0, CERT_SYSTEM_STORE_LOCAL_MACHINE, "root");
    if (!ctx->root_store && GetLastError() == ERROR_ACCESS_DENIED)
    {
        skip("Failed to open root store.\n");

        ret = CertFreeCertificateContext(ctx->cert);
        ok(ret, "Failed to free certificate, error %lu\n", GetLastError());
        ret = CryptReleaseContext(ctx->provider, 0);
        ok(ret, "failed to release context, error %lu\n", GetLastError());

        return FALSE;
    }
    ok(!!ctx->root_store, "Failed to open store, error %lu\n", GetLastError());
    ret = CertAddCertificateContextToStore(ctx->root_store, ctx->cert, CERT_STORE_ADD_ALWAYS, &ctx->root_cert);
    if (!ret && GetLastError() == ERROR_ACCESS_DENIED)
    {
        skip("Failed to add self-signed certificate to store.\n");

        ret = CertFreeCertificateContext(ctx->cert);
        ok(ret, "Failed to free certificate, error %lu\n", GetLastError());
        ret = CertCloseStore(ctx->root_store, CERT_CLOSE_STORE_CHECK_FLAG);
        ok(ret, "Failed to close store, error %lu\n", GetLastError());
        ret = CryptReleaseContext(ctx->provider, 0);
        ok(ret, "failed to release context, error %lu\n", GetLastError());

        return FALSE;
    }
    ok(ret, "Failed to add certificate, error %lu\n", GetLastError());

    ctx->publisher_store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY_A, 0, 0,
            CERT_SYSTEM_STORE_LOCAL_MACHINE, "trustedpublisher");
    ok(!!ctx->publisher_store, "Failed to open store, error %lu\n", GetLastError());
    ret = CertAddCertificateContextToStore(ctx->publisher_store, ctx->cert,
            CERT_STORE_ADD_ALWAYS, &ctx->publisher_cert);
    ok(ret, "Failed to add certificate, error %lu\n", GetLastError());

    return TRUE;
}

static void testsign_cleanup(struct testsign_context *ctx)
{
    BOOL ret;

    ret = CertFreeCertificateContext(ctx->cert);
    ok(ret, "Failed to free certificate, error %lu\n", GetLastError());

    ret = CertFreeCertificateContext(ctx->root_cert);
    ok(ret, "Failed to free certificate context, error %lu\n", GetLastError());
    ret = CertCloseStore(ctx->root_store, CERT_CLOSE_STORE_CHECK_FLAG);
    ok(ret, "Failed to close store, error %lu\n", GetLastError());

    ret = CertFreeCertificateContext(ctx->publisher_cert);
    ok(ret, "Failed to free certificate context, error %lu\n", GetLastError());
    ret = CertCloseStore(ctx->publisher_store, CERT_CLOSE_STORE_CHECK_FLAG);
    ok(ret, "Failed to close store, error %lu\n", GetLastError());

    ret = CryptReleaseContext(ctx->provider, 0);
    ok(ret, "failed to release context, error %lu\n", GetLastError());
}

static void testsign_sign(struct testsign_context *ctx, const WCHAR *filename)
{
    static HRESULT (WINAPI *pSignerSign)(SIGNER_SUBJECT_INFO *subject, SIGNER_CERT *cert,
            SIGNER_SIGNATURE_INFO *signature, SIGNER_PROVIDER_INFO *provider,
            const WCHAR *timestamp, CRYPT_ATTRIBUTES *attr, void *sip_data);

    SIGNER_ATTR_AUTHCODE authcode = {sizeof(authcode)};
    SIGNER_SIGNATURE_INFO signature = {sizeof(signature)};
    SIGNER_SUBJECT_INFO subject = {sizeof(subject)};
    SIGNER_CERT_STORE_INFO store = {sizeof(store)};
    SIGNER_CERT cert_info = {sizeof(cert_info)};
    SIGNER_FILE_INFO file = {sizeof(file)};
    DWORD index = 0;
    HRESULT hr;

    if (!pSignerSign)
        pSignerSign = (void *)GetProcAddress(LoadLibraryA("mssign32"), "SignerSign");

    subject.dwSubjectChoice = 1;
    subject.pdwIndex = &index;
    subject.pSignerFileInfo = &file;
    file.pwszFileName = (WCHAR *)filename;
    cert_info.dwCertChoice = 2;
    cert_info.pCertStoreInfo = &store;
    store.pSigningCert = ctx->cert;
    store.dwCertPolicy = 0;
    signature.algidHash = CALG_SHA_256;
    signature.dwAttrChoice = SIGNER_AUTHCODE_ATTR;
    signature.pAttrAuthcode = &authcode;
    authcode.pwszName = L"";
    authcode.pwszInfo = L"";
    hr = pSignerSign(&subject, &cert_info, &signature, NULL, NULL, NULL, NULL);
    todo_wine ok(hr == S_OK || broken(hr == NTE_BAD_ALGID) /* < 7 */, "Failed to sign, hr %#lx\n", hr);
}

static void add_file_to_catalog(HANDLE catalog, const WCHAR *file)
{
    SIP_SUBJECTINFO subject_info = {sizeof(SIP_SUBJECTINFO)};
    SIP_INDIRECT_DATA *indirect_data;
    CRYPTCATMEMBER *member;
    WCHAR hash_buffer[100];
    GUID subject_guid;
    unsigned int i;
    DWORD size;
    BOOL ret;

    ret = CryptSIPRetrieveSubjectGuidForCatalogFile(file, NULL, &subject_guid);
    todo_wine ok(ret, "Failed to get subject guid, error %lu\n", GetLastError());

    size = 0;
    subject_info.pgSubjectType = &subject_guid;
    subject_info.pwsFileName = file;
    subject_info.DigestAlgorithm.pszObjId = (char *)szOID_OIWSEC_sha1;
    ret = CryptSIPCreateIndirectData(&subject_info, &size, NULL);
    todo_wine ok(ret, "Failed to get indirect data size, error %lu\n", GetLastError());

    indirect_data = malloc(size);
    ret = CryptSIPCreateIndirectData(&subject_info, &size, indirect_data);
    todo_wine ok(ret, "Failed to get indirect data, error %lu\n", GetLastError());
    if (ret)
    {
        memset(hash_buffer, 0, sizeof(hash_buffer));
        for (i = 0; i < indirect_data->Digest.cbData; ++i)
            swprintf(&hash_buffer[i * 2], 2, L"%02X", indirect_data->Digest.pbData[i]);

        member = CryptCATPutMemberInfo(catalog, (WCHAR *)file,
                hash_buffer, &subject_guid, 0, size, (BYTE *)indirect_data);
        ok(!!member, "Failed to write member, error %lu\n", GetLastError());
    }

    free(indirect_data);
}

static void test_create_device_list_ex(void)
{
    static const WCHAR machine[] = { 'd','u','m','m','y',0 };
    static const WCHAR empty[] = { 0 };
    static char notnull[] = "NotNull";
    HDEVINFO set;
    BOOL ret;

    SetLastError(0xdeadbeef);
    set = SetupDiCreateDeviceInfoListExW(NULL, NULL, NULL, notnull);
    ok(set == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    set = SetupDiCreateDeviceInfoListExW(NULL, NULL, machine, NULL);
    ok(set == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_MACHINENAME
            || GetLastError() == ERROR_MACHINE_UNAVAILABLE
            || GetLastError() == ERROR_CALL_NOT_IMPLEMENTED,
            "Got unexpected error %#lx.\n", GetLastError());

    set = SetupDiCreateDeviceInfoListExW(NULL, NULL, NULL, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());

    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list, error %#lx.\n", GetLastError());

    set = SetupDiCreateDeviceInfoListExW(NULL, NULL, empty, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());

    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list, error %#lx.\n", GetLastError());
}

static void test_open_class_key(void)
{
    static const char guidstr[] = "{6a55b5a4-3f65-11db-b704-0011955c2bdb}";
    HKEY root_key, class_key;
    LONG res;

    SetLastError(0xdeadbeef);
    class_key = SetupDiOpenClassRegKeyExA(&guid, KEY_ALL_ACCESS, DIOCR_INSTALLER, NULL, NULL);
    ok(class_key == INVALID_HANDLE_VALUE, "Expected failure.\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_CLASS, "Got unexpected error %#lx.\n", GetLastError());

    root_key = SetupDiOpenClassRegKey(NULL, KEY_ALL_ACCESS);
    ok(root_key != INVALID_HANDLE_VALUE, "Failed to open root key, error %#lx.\n", GetLastError());

    res = RegCreateKeyA(root_key, guidstr, &class_key);
    ok(!res, "Failed to create class key, error %#lx.\n", GetLastError());
    RegCloseKey(class_key);

    SetLastError(0xdeadbeef);
    class_key = SetupDiOpenClassRegKeyExA(&guid, KEY_ALL_ACCESS, DIOCR_INSTALLER, NULL, NULL);
    ok(class_key != INVALID_HANDLE_VALUE, "Failed to open class key, error %#lx.\n", GetLastError());
    RegCloseKey(class_key);

    RegDeleteKeyA(root_key, guidstr);
    RegCloseKey(root_key);
}

static void get_temp_filename(LPSTR path)
{
    static char curr[MAX_PATH] = { 0 };
    char temp[MAX_PATH];
    LPSTR ptr;

    if (!*curr)
        GetCurrentDirectoryA(MAX_PATH, curr);
    GetTempFileNameA(curr, "set", 0, temp);
    ptr = strrchr(temp, '\\');

    lstrcpyA(path, ptr + 1);
}

static void * CDECL mem_alloc(ULONG cb)
{
    return HeapAlloc(GetProcessHeap(), 0, cb);
}

static void CDECL mem_free(void *memory)
{
    HeapFree(GetProcessHeap(), 0, memory);
}

static BOOL CDECL get_next_cabinet(PCCAB pccab, ULONG  cbPrevCab, void *ctx)
{
    sprintf(pccab->szCab, ctx, pccab->iCab);
    return TRUE;
}

static LONG CDECL progress(UINT typeStatus, ULONG cb1, ULONG cb2, void *ctx)
{
    return 0;
}

static int CDECL file_placed(PCCAB pccab, char *pszFile, LONG cbFile,
                             BOOL fContinuation, void *ctx)
{
    return 0;
}

static INT_PTR CDECL fci_open(char *pszFile, int oflag, int pmode, int *err, void *ctx)
{
    HANDLE handle;
    DWORD dwAccess = 0;
    DWORD dwShareMode = 0;
    DWORD dwCreateDisposition = OPEN_EXISTING;

    dwAccess = GENERIC_READ | GENERIC_WRITE;
    dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;

    if (GetFileAttributesA(pszFile) != INVALID_FILE_ATTRIBUTES)
        dwCreateDisposition = OPEN_EXISTING;
    else
        dwCreateDisposition = CREATE_NEW;

    handle = CreateFileA(pszFile, dwAccess, dwShareMode, NULL,
                         dwCreateDisposition, 0, NULL);

    ok(handle != INVALID_HANDLE_VALUE, "Failed to CreateFile %s\n", pszFile);

    return (INT_PTR)handle;
}

static UINT CDECL fci_read(INT_PTR hf, void *memory, UINT cb, int *err, void *ctx)
{
    HANDLE handle = (HANDLE)hf;
    DWORD dwRead;
    BOOL res;

    res = ReadFile(handle, memory, cb, &dwRead, NULL);
    ok(res, "Failed to ReadFile\n");

    return dwRead;
}

static UINT CDECL fci_write(INT_PTR hf, void *memory, UINT cb, int *err, void *ctx)
{
    HANDLE handle = (HANDLE)hf;
    DWORD dwWritten;
    BOOL res;

    res = WriteFile(handle, memory, cb, &dwWritten, NULL);
    ok(res, "Failed to WriteFile\n");

    return dwWritten;
}

static int CDECL fci_close(INT_PTR hf, int *err, void *ctx)
{
    HANDLE handle = (HANDLE)hf;
    ok(CloseHandle(handle), "Failed to CloseHandle\n");

    return 0;
}

static LONG CDECL fci_seek(INT_PTR hf, LONG dist, int seektype, int *err, void *ctx)
{
    HANDLE handle = (HANDLE)hf;
    DWORD ret;

    ret = SetFilePointer(handle, dist, NULL, seektype);
    ok(ret != INVALID_SET_FILE_POINTER, "Failed to SetFilePointer\n");

    return ret;
}

static int CDECL fci_delete(char *pszFile, int *err, void *ctx)
{
    BOOL ret = DeleteFileA(pszFile);
    ok(ret, "Failed to DeleteFile %s\n", pszFile);

    return 0;
}

static BOOL CDECL get_temp_file(char *pszTempName, int cbTempName, void *ctx)
{
    LPSTR tempname;

    tempname = malloc(MAX_PATH);
    GetTempFileNameA(".", "xx", 0, tempname);

    if (tempname && (strlen(tempname) < (unsigned)cbTempName))
    {
        lstrcpyA(pszTempName, tempname);
        free(tempname);
        return TRUE;
    }

    free(tempname);

    return FALSE;
}

static INT_PTR CDECL get_open_info(char *name, USHORT *date, USHORT *time, USHORT *attribs, int *err, void *ctx)
{
    BY_HANDLE_FILE_INFORMATION finfo;
    FILETIME filetime;
    HANDLE handle;
    DWORD attrs;
    BOOL res;

    handle = CreateFileA(name, GENERIC_READ, FILE_SHARE_READ, NULL,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    ok(handle != INVALID_HANDLE_VALUE, "Failed to create %s, error %lu.\n", debugstr_a(name), GetLastError());

    res = GetFileInformationByHandle(handle, &finfo);
    ok(res, "Expected GetFileInformationByHandle to succeed\n");

    FileTimeToLocalFileTime(&finfo.ftLastWriteTime, &filetime);
    FileTimeToDosDateTime(&filetime, date, time);

    attrs = GetFileAttributesA(name);
    ok(attrs != INVALID_FILE_ATTRIBUTES, "Failed to GetFileAttributes\n");

    return (INT_PTR)handle;
}

static BOOL add_file(HFCI hfci, const char *file, TCOMP compress)
{
    char path[MAX_PATH], filename[MAX_PATH];

    GetCurrentDirectoryA(sizeof(path), path);
    strcat(path, "\\");
    strcat(path, file);

    strcpy(filename, file);

    return FCIAddFile(hfci, path, filename, FALSE, get_next_cabinet, progress, get_open_info, compress);
}

static void create_cab_file(const char *name, const char *source)
{
    CCAB cab_params = {0};
    HFCI hfci;
    ERF erf;
    BOOL res;

    cab_params.cb = INT_MAX;
    cab_params.cbFolderThresh = 900000;
    cab_params.setID = 0xbeef;
    cab_params.iCab = 1;
    GetCurrentDirectoryA(sizeof(cab_params.szCabPath), cab_params.szCabPath);
    strcat(cab_params.szCabPath, "\\");
    strcpy(cab_params.szCab, name);

    hfci = FCICreate(&erf, file_placed, mem_alloc, mem_free, fci_open, fci_read,
            fci_write, fci_close, fci_seek, fci_delete, get_temp_file, &cab_params, NULL);

    ok(hfci != NULL, "Failed to create an FCI context\n");

    res = add_file(hfci, source, tcompTYPE_MSZIP);
    ok(res, "Failed to add %s\n", source);

    res = FCIFlushCabinet(hfci, FALSE, get_next_cabinet, progress);
    ok(res, "Failed to flush the cabinet\n");
    res = FCIDestroy(hfci);
    ok(res, "Failed to destroy the cabinet\n");
}

static void test_install_class(void)
{
    static const WCHAR classKey[] = {'S','y','s','t','e','m','\\',
     'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
     'C','o','n','t','r','o','l','\\','C','l','a','s','s','\\',
     '{','6','a','5','5','b','5','a','4','-','3','f','6','5','-',
     '1','1','d','b','-','b','7','0','4','-',
     '0','0','1','1','9','5','5','c','2','b','d','b','}',0};
    char tmpfile[MAX_PATH];
    char buf[32];
    DWORD size;
    BOOL ret;

    static const char inf_data[] =
            "[Version]\n"
            "Signature=\"$Chicago$\"\n"
            "Class=Bogus\n"
            "ClassGUID={6a55b5a4-3f65-11db-b704-0011955c2bdb}\n"
            "[ClassInstall32]\n"
            "AddReg=BogusClass.NT.AddReg\n"
            "[BogusClass.NT.AddReg]\n"
            "HKR,,,,\"Wine test devices\"\n";

    tmpfile[0] = '.';
    tmpfile[1] = '\\';
    get_temp_filename(tmpfile + 2);
    create_file(tmpfile + 2, inf_data);

    ret = SetupDiInstallClassA(NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    ret = SetupDiInstallClassA(NULL, NULL, DI_NOVCP, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    ret = SetupDiInstallClassA(NULL, tmpfile + 2, DI_NOVCP, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    ret = SetupDiInstallClassA(NULL, tmpfile + 2, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "Got unexpected error %#lx.\n", GetLastError());

    size = 123;
    SetLastError(0xdeadbeef);
    ret = SetupDiGetClassDescriptionA(&guid, buf, sizeof(buf), &size);
    ok(!ret, "Expected failure.\n");
    ok(size == 123, "Expected 123, got %ld.\n", size);
    todo_wine
    ok(GetLastError() == ERROR_NOT_FOUND /* win7 */ || GetLastError() == ERROR_INVALID_CLASS /* win10 */,
       "Got unexpected error %#lx.\n", GetLastError());

    /* The next call will succeed. Information is put into the registry but the
     * location(s) is/are depending on the Windows version.
     */
    ret = SetupDiInstallClassA(NULL, tmpfile, 0, NULL);
    ok(ret, "Failed to install class, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetClassDescriptionA(&guid, buf, sizeof(buf), &size);
    ok(ret == TRUE, "Failed to get class description.\n");
    ok(size == sizeof("Wine test devices"), "Expected %Iu, got %lu.\n", sizeof("Wine test devices"), size);
    ok(!strcmp(buf, "Wine test devices"), "Got unexpected class description %s.\n", debugstr_a(buf));
    todo_wine ok(!GetLastError(), "Got unexpected error %#lx.\n", GetLastError());

    ret = RegDeleteKeyW(HKEY_LOCAL_MACHINE, classKey);
    ok(!ret, "Failed to delete class key, error %lu.\n", GetLastError());
    DeleteFileA(tmpfile);
}

static void check_device_info_(int line, HDEVINFO set, int index, const GUID *class, const char *expect_id)
{
    SP_DEVINFO_DATA device = {sizeof(device)};
    char id[50];
    BOOL ret;

    SetLastError(0xdeadbeef);
    ret = SetupDiEnumDeviceInfo(set, index, &device);
    if (expect_id)
    {
        ok_(__FILE__, line)(ret, "Got unexpected error %#lx.\n", GetLastError());
        ok_(__FILE__, line)(IsEqualGUID(&device.ClassGuid, class),
                "Got unexpected class %s.\n", wine_dbgstr_guid(&device.ClassGuid));
        ret = SetupDiGetDeviceInstanceIdA(set, &device, id, sizeof(id), NULL);
        ok_(__FILE__, line)(ret, "Got unexpected error %#lx.\n", GetLastError());
        ok_(__FILE__, line)(!strcasecmp(id, expect_id), "Got unexpected id %s.\n", id);
    }
    else
    {
        ok_(__FILE__, line)(!ret, "Expected failure.\n");
        ok_(__FILE__, line)(GetLastError() == ERROR_NO_MORE_ITEMS,
                "Got unexpected error %#lx.\n", GetLastError());
    }
}
#define check_device_info(a,b,c,d) check_device_info_(__LINE__,a,b,c,d)

static void test_device_info(void)
{
    static const GUID deadbeef = {0xdeadbeef,0xdead,0xbeef,{0xde,0xad,0xbe,0xef,0xde,0xad,0xbe,0xef}};
    SP_DEVINFO_DATA device = {0}, ret_device = {sizeof(ret_device)};
    char id[MAX_DEVICE_ID_LEN + 2];
    HDEVINFO set;
    BOOL ret;
    INT i = 0;

    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInfoA(NULL, NULL, NULL, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DEVINST_NAME, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInfoA(NULL, "Root\\LEGACY_BOGUS\\0000", NULL, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected error %#lx.\n", GetLastError());

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device info, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", NULL, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &deadbeef, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_CLASS_MISMATCH, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &GUID_NULL, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_CLASS_MISMATCH, "Got unexpected error %#lx.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, NULL);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    check_device_info(set, 0, &guid, "ROOT\\LEGACY_BOGUS\\0000");
    check_device_info(set, 1, &guid, NULL);

    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_DEVINST_ALREADY_EXISTS, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0001", &guid, NULL, NULL, 0, &device);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "Got unexpected error %#lx.\n", GetLastError());

    check_device_info(set, 0, &guid, "ROOT\\LEGACY_BOGUS\\0000");
    check_device_info(set, 1, &guid, "ROOT\\LEGACY_BOGUS\\0001");
    check_device_info(set, 2, &guid, NULL);

    device.cbSize = sizeof(device);
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0002", &guid, NULL, NULL, 0, &device);
    ok(ret, "Got unexpected error %#lx.\n", GetLastError());
    ok(IsEqualGUID(&device.ClassGuid, &guid), "Got unexpected class %s.\n",
            wine_dbgstr_guid(&device.ClassGuid));
    ret = SetupDiGetDeviceInstanceIdA(set, &device, id, sizeof(id), NULL);
    ok(ret, "Got unexpected error %#lx.\n", GetLastError());
    ok(!strcmp(id, "ROOT\\LEGACY_BOGUS\\0002"), "Got unexpected id %s.\n", id);

    check_device_info(set, 0, &guid, "ROOT\\LEGACY_BOGUS\\0000");
    check_device_info(set, 1, &guid, "ROOT\\LEGACY_BOGUS\\0001");
    check_device_info(set, 2, &guid, "ROOT\\LEGACY_BOGUS\\0002");
    check_device_info(set, 3, &guid, NULL);

    ret = SetupDiEnumDeviceInfo(set, 0, &ret_device);
    ok(ret, "Failed to enumerate devices, error %#lx.\n", GetLastError());
    ret = SetupDiDeleteDeviceInfo(set, &ret_device);
    ok(ret, "Failed to delete device, error %#lx.\n", GetLastError());

    check_device_info(set, 0, &guid, "ROOT\\LEGACY_BOGUS\\0001");
    check_device_info(set, 1, &guid, "ROOT\\LEGACY_BOGUS\\0002");
    check_device_info(set, 2, &guid, NULL);

    ret = SetupDiRemoveDevice(set, &device);
    ok(ret, "Got unexpected error %#lx.\n", GetLastError());

    check_device_info(set, 0, &guid, "ROOT\\LEGACY_BOGUS\\0001");

    ret = SetupDiEnumDeviceInfo(set, 1, &ret_device);
    ok(ret, "Got unexpected error %#lx.\n", GetLastError());
    ok(IsEqualGUID(&ret_device.ClassGuid, &guid), "Got unexpected class %s.\n",
            wine_dbgstr_guid(&ret_device.ClassGuid));
    ret = SetupDiGetDeviceInstanceIdA(set, &ret_device, id, sizeof(id), NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_NO_SUCH_DEVINST, "Got unexpected error %#lx.\n", GetLastError());
    ok(ret_device.DevInst == device.DevInst, "Expected device node %#lx, got %#lx.\n",
            device.DevInst, ret_device.DevInst);

    check_device_info(set, 2, &guid, NULL);

    SetupDiDestroyDeviceInfoList(set);

    set = SetupDiCreateDeviceInfoList(NULL, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device info, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", NULL, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\deadbeef", &deadbeef, NULL, NULL, 0, NULL);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\null", &GUID_NULL, NULL, NULL, 0, NULL);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\testguid", &guid, NULL, NULL, 0, NULL);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    check_device_info(set, 0, &deadbeef, "ROOT\\LEGACY_BOGUS\\deadbeef");
    check_device_info(set, 1, &GUID_NULL, "ROOT\\LEGACY_BOGUS\\null");
    check_device_info(set, 2, &guid, "ROOT\\LEGACY_BOGUS\\testguid");
    check_device_info(set, 3, NULL, NULL);

    memset(id, 'x', sizeof(id));
    memcpy(id, "Root\\LEGACY_BOGUS\\", strlen("Root\\LEGACY_BOGUS\\"));
    id[MAX_DEVICE_ID_LEN + 1] = 0;
    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInfoA(set, id, &guid, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DEVINST_NAME, "Got unexpected error %#lx.\n", GetLastError());

    id[MAX_DEVICE_ID_LEN] = 0;
    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInfoA(set, id, &guid, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DEVINST_NAME, "Got unexpected error %#lx.\n", GetLastError());

    id[MAX_DEVICE_ID_LEN - 1] = 0;
    ret = SetupDiCreateDeviceInfoA(set, id, &guid, NULL, NULL, 0, NULL);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    SetupDiDestroyDeviceInfoList(set);

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device info list, error %#lx.\n", GetLastError());
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());
    ret = SetupDiRegisterDeviceInfo(set , &device, 0, NULL, NULL, NULL);
    ok(ret, "Failed to register device, error %#lx.\n", GetLastError());
    check_device_info(set, 0, &guid, "ROOT\\LEGACY_BOGUS\\0000");
    check_device_info(set, 1, NULL, NULL);
    SetupDiDestroyDeviceInfoList(set);

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(!ret, "Expect failure\n");
    ok(GetLastError() == ERROR_DEVINST_ALREADY_EXISTS, "Got error %#lx\n", GetLastError());
    check_device_info(set, 0, NULL, NULL);
    SetupDiDestroyDeviceInfoList(set);

    set = SetupDiGetClassDevsA(&guid, NULL, NULL, 0);
    while (SetupDiEnumDeviceInfo(set, i++, &device))
    {
        ret = SetupDiRemoveDevice(set, &device);
        ok(ret, "Failed to remove device, error %#lx.\n", GetLastError());
    }
    SetupDiDestroyDeviceInfoList(set);
}

static void test_device_property(void)
{
    static const WCHAR valueW[] = {'d', 'e', 'a', 'd', 'b', 'e', 'e', 'f', 0};
    SP_DEVINFO_DATA device_data = {sizeof(device_data)};
    HMODULE hmod;
    HDEVINFO set;
    DEVPROPTYPE type;
    DWORD size;
    BYTE buffer[256];
    DWORD err;
    BOOL ret;

    hmod = LoadLibraryA("setupapi.dll");
    pSetupDiSetDevicePropertyW = (void *)GetProcAddress(hmod, "SetupDiSetDevicePropertyW");
    pSetupDiGetDevicePropertyW = (void *)GetProcAddress(hmod, "SetupDiGetDevicePropertyW");
    pSetupDiGetDevicePropertyKeys = (void *)GetProcAddress(hmod, "SetupDiGetDevicePropertyKeys");

    if (!pSetupDiSetDevicePropertyW || !pSetupDiGetDevicePropertyW)
    {
        win_skip("SetupDi{Set,Get}DevicePropertyW() are only available on vista+, skipping tests.\n");
        FreeLibrary(hmod);
        return;
    }

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device_data);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    /* SetupDiSetDevicePropertyW */
    /* #1 Null device info list */
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDevicePropertyW(NULL, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_STRING, (const BYTE *)valueW, sizeof(valueW), 0);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_INVALID_HANDLE, "Expect last error %#x, got %#lx\n", ERROR_INVALID_HANDLE, err);

    /* #2 Null device */
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDevicePropertyW(set, NULL, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_STRING, (const BYTE *)valueW, sizeof(valueW), 0);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_INVALID_PARAMETER, "Expect last error %#x, got %#lx\n", ERROR_INVALID_PARAMETER, err);

    /* #3 Null property key pointer */
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDevicePropertyW(set, &device_data, NULL, DEVPROP_TYPE_STRING, (const BYTE *)valueW, sizeof(valueW), 0);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_INVALID_DATA, "Expect last error %#x, got %#lx\n", ERROR_INVALID_DATA, err);

    /* #4 Invalid property key type */
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, 0xffff, (const BYTE *)valueW, sizeof(valueW), 0);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_INVALID_DATA, "Expect last error %#x, got %#lx\n", ERROR_INVALID_DATA, err);

    /* #5 Null buffer pointer */
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_STRING, NULL, sizeof(valueW), 0);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_INVALID_USER_BUFFER, "Expect last error %#x, got %#lx\n", ERROR_INVALID_USER_BUFFER, err);

    /* #6 Zero buffer size */
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_STRING, (const BYTE *)valueW, 0, 0);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_INVALID_DATA, "Expect last error %#x, got %#lx\n", ERROR_INVALID_DATA, err);

    /* #7 Flags not zero */
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_STRING, (const BYTE *)valueW, sizeof(valueW), 1);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_INVALID_FLAGS, "Expect last error %#x, got %#lx\n", ERROR_INVALID_FLAGS, err);

    /* #8 Normal */
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_STRING, (const BYTE *)valueW, sizeof(valueW), 0);
    err = GetLastError();
    ok(ret, "Expect success\n");
    ok(err == NO_ERROR, "Expect last error %#x, got %#lx\n", NO_ERROR, err);

    /* #9 Delete property with buffer not null */
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_STRING, (const BYTE *)valueW, sizeof(valueW), 0);
    ok(ret, "Expect success\n");
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_EMPTY, (const BYTE *)valueW, 0, 0);
    err = GetLastError();
    ok(ret, "Expect success\n");
    ok(err == NO_ERROR, "Expect last error %#x, got %#lx\n", NO_ERROR, err);

    /* #10 Delete property with size not zero */
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_STRING, (const BYTE *)valueW, sizeof(valueW), 0);
    ok(ret, "Expect success\n");
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_EMPTY, NULL, sizeof(valueW), 0);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_INVALID_USER_BUFFER, "Expect last error %#x, got %#lx\n", ERROR_INVALID_USER_BUFFER, err);

    /* #11 Delete property */
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_STRING, (const BYTE *)valueW, sizeof(valueW), 0);
    ok(ret, "Expect success\n");
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_EMPTY, NULL, 0, 0);
    err = GetLastError();
    ok(ret, "Expect success\n");
    ok(err == NO_ERROR, "Expect last error %#x, got %#lx\n", NO_ERROR, err);

    /* #12 Delete non-existent property */
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_EMPTY, NULL, 0, 0);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_NOT_FOUND, "Expect last error %#x, got %#lx\n", ERROR_NOT_FOUND, err);

    /* #13 Delete property value with buffer not null */
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_STRING, (const BYTE *)valueW, sizeof(valueW), 0);
    ok(ret, "Expect success\n");
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_NULL, (const BYTE *)valueW, 0, 0);
    err = GetLastError();
    ok(ret, "Expect success\n");
    ok(err == NO_ERROR, "Expect last error %#x, got %#lx\n", NO_ERROR, err);

    /* #14 Delete property value with size not zero */
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_STRING, (const BYTE *)valueW, sizeof(valueW), 0);
    ok(ret, "Expect success\n");
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_NULL, NULL, sizeof(valueW), 0);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_INVALID_USER_BUFFER, "Expect last error %#x, got %#lx\n", ERROR_INVALID_USER_BUFFER, err);

    /* #15 Delete property value */
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_STRING, (const BYTE *)valueW, sizeof(valueW), 0);
    ok(ret, "Expect success\n");
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_NULL, NULL, 0, 0);
    err = GetLastError();
    ok(ret, "Expect success\n");
    ok(err == NO_ERROR, "Expect last error %#x, got %#lx\n", NO_ERROR, err);

    /* #16 Delete non-existent property value */
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_NULL, NULL, 0, 0);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_NOT_FOUND, "Expect last error %#x, got %#lx\n", ERROR_NOT_FOUND, err);


    /* SetupDiGetDevicePropertyW */
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_STRING, (const BYTE *)valueW, sizeof(valueW), 0);
    ok(ret, "Expect success\n");

    /* #1 Null device info list */
    SetLastError(0xdeadbeef);
    type = DEVPROP_TYPE_STRING;
    size = 0;
    ret = pSetupDiGetDevicePropertyW(NULL, &device_data, &DEVPKEY_Device_FriendlyName, &type, buffer, sizeof(buffer), &size, 0);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_INVALID_HANDLE, "Expect last error %#x, got %#lx\n", ERROR_INVALID_HANDLE, err);
    ok(type == DEVPROP_TYPE_STRING, "Expect type %#x, got %#lx\n", DEVPROP_TYPE_STRING, type);
    ok(size == 0, "Expect size %d, got %ld\n", 0, size);

    /* #2 Null device */
    SetLastError(0xdeadbeef);
    type = DEVPROP_TYPE_STRING;
    size = 0;
    ret = pSetupDiGetDevicePropertyW(set, NULL, &DEVPKEY_Device_FriendlyName, &type, buffer, sizeof(buffer), &size, 0);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_INVALID_PARAMETER, "Expect last error %#x, got %#lx\n", ERROR_INVALID_PARAMETER, err);
    ok(type == DEVPROP_TYPE_STRING, "Expect type %#x, got %#lx\n", DEVPROP_TYPE_STRING, type);
    ok(size == 0, "Expect size %d, got %ld\n", 0, size);

    /* #3 Null property key */
    SetLastError(0xdeadbeef);
    type = DEVPROP_TYPE_STRING;
    size = 0;
    ret = pSetupDiGetDevicePropertyW(set, &device_data, NULL, &type, buffer, sizeof(buffer), &size, 0);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_INVALID_DATA, "Expect last error %#x, got %#lx\n", ERROR_INVALID_DATA, err);
    ok(size == 0, "Expect size %d, got %ld\n", 0, size);

    /* #4 Null property type */
    SetLastError(0xdeadbeef);
    type = DEVPROP_TYPE_STRING;
    size = 0;
    ret = pSetupDiGetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, NULL, buffer, sizeof(buffer), &size, 0);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_INVALID_USER_BUFFER, "Expect last error %#x, got %#lx\n", ERROR_INVALID_USER_BUFFER, err);
    ok(type == DEVPROP_TYPE_STRING, "Expect type %#x, got %#lx\n", DEVPROP_TYPE_STRING, type);
    ok(size == 0, "Expect size %d, got %ld\n", 0, size);

    /* #5 Null buffer */
    SetLastError(0xdeadbeef);
    type = DEVPROP_TYPE_STRING;
    size = 0;
    ret = pSetupDiGetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, &type, NULL, sizeof(buffer), &size, 0);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_INVALID_USER_BUFFER, "Expect last error %#x, got %#lx\n", ERROR_INVALID_USER_BUFFER, err);
    ok(type == DEVPROP_TYPE_STRING, "Expect type %#x, got %#lx\n", DEVPROP_TYPE_STRING, type);
    ok(size == 0, "Expect size %d, got %ld\n", 0, size);

    /* #6 Null buffer with zero size and wrong type */
    SetLastError(0xdeadbeef);
    type = DEVPROP_TYPE_UINT64;
    size = 0;
    ret = pSetupDiGetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, &type, NULL, 0, &size, 0);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_INSUFFICIENT_BUFFER, "Expect last error %#x, got %#lx\n", ERROR_INSUFFICIENT_BUFFER, err);
    ok(type == DEVPROP_TYPE_STRING, "Expect type %#x, got %#lx\n", DEVPROP_TYPE_STRING, type);
    ok(size == sizeof(valueW), "Got size %ld\n", size);

    /* #7 Zero buffer size */
    SetLastError(0xdeadbeef);
    type = DEVPROP_TYPE_STRING;
    size = 0;
    ret = pSetupDiGetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, &type, buffer, 0, &size, 0);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_INSUFFICIENT_BUFFER, "Expect last error %#x, got %#lx\n", ERROR_INSUFFICIENT_BUFFER, err);
    ok(type == DEVPROP_TYPE_STRING, "Expect type %#x, got %#lx\n", DEVPROP_TYPE_STRING, type);
    ok(size == sizeof(valueW), "Got size %ld\n", size);

    /* #8 Null required size */
    SetLastError(0xdeadbeef);
    type = DEVPROP_TYPE_STRING;
    ret = pSetupDiGetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, &type, buffer, sizeof(buffer), NULL, 0);
    err = GetLastError();
    ok(ret, "Expect success\n");
    ok(err == NO_ERROR, "Expect last error %#x, got %#lx\n", NO_ERROR, err);
    ok(type == DEVPROP_TYPE_STRING, "Expect type %#x, got %#lx\n", DEVPROP_TYPE_STRING, type);

    /* #9 Flags not zero */
    SetLastError(0xdeadbeef);
    type = DEVPROP_TYPE_STRING;
    size = 0;
    ret = pSetupDiGetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, &type, buffer, sizeof(buffer), &size, 1);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_INVALID_FLAGS, "Expect last error %#x, got %#lx\n", ERROR_INVALID_FLAGS, err);
    ok(type == DEVPROP_TYPE_STRING, "Expect type %#x, got %#lx\n", DEVPROP_TYPE_STRING, type);
    ok(size == 0, "Expect size %d, got %ld\n", 0, size);

    /* #10 Non-existent property key */
    SetLastError(0xdeadbeef);
    type = DEVPROP_TYPE_STRING;
    size = 0;
    ret = pSetupDiGetDevicePropertyW(set, &device_data, &DEVPKEY_Device_HardwareIds, &type, buffer, sizeof(buffer), &size, 0);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_NOT_FOUND, "Expect last error %#x, got %#lx\n", ERROR_NOT_FOUND, err);
    ok(size == 0, "Expect size %d, got %ld\n", 0, size);

    /* #11 Wrong property key type */
    SetLastError(0xdeadbeef);
    type = DEVPROP_TYPE_UINT64;
    size = 0;
    memset(buffer, 0, sizeof(buffer));
    ret = pSetupDiGetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, &type, buffer, sizeof(buffer), &size, 0);
    err = GetLastError();
    ok(ret, "Expect success\n");
    ok(err == NO_ERROR, "Expect last error %#x, got %#lx\n", NO_ERROR, err);
    ok(type == DEVPROP_TYPE_STRING, "Expect type %#x, got %#lx\n", DEVPROP_TYPE_STRING, type);
    ok(size == sizeof(valueW), "Got size %ld\n", size);
    ok(!lstrcmpW((WCHAR *)buffer, valueW), "Expect buffer %s, got %s\n", wine_dbgstr_w(valueW), wine_dbgstr_w((WCHAR *)buffer));

    /* #12 Get null property value */
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_STRING, (const BYTE *)valueW, sizeof(valueW), 0);
    ok(ret, "Expect success\n");
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_NULL, NULL, 0, 0);
    ok(ret, "Expect success\n");
    SetLastError(0xdeadbeef);
    type = DEVPROP_TYPE_STRING;
    size = 0;
    ret = pSetupDiGetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, &type, buffer, sizeof(buffer), &size, 0);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_NOT_FOUND, "Expect last error %#x, got %#lx\n", ERROR_NOT_FOUND, err);
    ok(size == 0, "Expect size %d, got %ld\n", 0, size);

    /* #13 Insufficient buffer size */
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_STRING, (const BYTE *)valueW, sizeof(valueW), 0);
    ok(ret, "Expect success\n");
    SetLastError(0xdeadbeef);
    type = DEVPROP_TYPE_STRING;
    size = 0;
    ret = pSetupDiGetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, &type, buffer, sizeof(valueW) - 1, &size, 0);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_INSUFFICIENT_BUFFER, "Expect last error %#x, got %#lx\n", ERROR_INSUFFICIENT_BUFFER, err);
    ok(type == DEVPROP_TYPE_STRING, "Expect type %#x, got %#lx\n", DEVPROP_TYPE_STRING, type);
    ok(size == sizeof(valueW), "Got size %ld\n", size);

    /* #14 Normal */
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_STRING, (const BYTE *)valueW, sizeof(valueW), 0);
    ok(ret, "Expect success\n");
    SetLastError(0xdeadbeef);
    type = DEVPROP_TYPE_STRING;
    size = 0;
    memset(buffer, 0, sizeof(buffer));
    ret = pSetupDiGetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, &type, buffer, sizeof(buffer), &size, 0);
    err = GetLastError();
    ok(ret, "Expect success\n");
    ok(err == NO_ERROR, "Expect last error %#x, got %#lx\n", NO_ERROR, err);
    ok(type == DEVPROP_TYPE_STRING, "Expect type %#x, got %#lx\n", DEVPROP_TYPE_STRING, type);
    ok(size == sizeof(valueW), "Got size %ld\n", size);
    ok(!lstrcmpW((WCHAR *)buffer, valueW), "Expect buffer %s, got %s\n", wine_dbgstr_w(valueW), wine_dbgstr_w((WCHAR *)buffer));

    if (pSetupDiGetDevicePropertyKeys)
    {
        DWORD keys_len = 0, n, required_len, expected_keys = 1;
        DEVPROPKEY *keys;

        ret = pSetupDiGetDevicePropertyKeys(NULL, NULL, NULL, 0, NULL, 0);
        ok(!ret, "Expect failure\n");
        err = GetLastError();
        ok(err == ERROR_INVALID_HANDLE, "Expect last error %#x, got %#lx\n",
           ERROR_INVALID_HANDLE, err);

        SetLastError(0xdeadbeef);
        ret = pSetupDiGetDevicePropertyKeys(set, NULL, NULL, 0, NULL, 0);
        ok(!ret, "Expect failure\n");
        err = GetLastError();
        ok(err == ERROR_INVALID_PARAMETER, "Expect last error %#x, got %#lx\n",
           ERROR_INVALID_PARAMETER, err);

        SetLastError(0xdeadbeef);
        ret = pSetupDiGetDevicePropertyKeys(set, &device_data, NULL, 10, NULL, 0);
        ok(!ret, "Expect failure\n");
        err = GetLastError();
        ok(err == ERROR_INVALID_USER_BUFFER, "Expect last error %#x, got %#lx\n",
           ERROR_INVALID_USER_BUFFER, err);

        SetLastError(0xdeadbeef);
        ret = pSetupDiGetDevicePropertyKeys(set, &device_data, NULL, 0, NULL, 0);
        ok(!ret, "Expect failure\n");
        err = GetLastError();
        ok(err == ERROR_INSUFFICIENT_BUFFER, "Expect last error %#x, got %#lx\n",
           ERROR_INSUFFICIENT_BUFFER, err);

        SetLastError(0xdeadbeef);
        ret = pSetupDiGetDevicePropertyKeys(set, &device_data, NULL, 0, &keys_len, 0xdeadbeef);
        ok(!ret, "Expect failure\n");
        err = GetLastError();
        ok(err == ERROR_INVALID_FLAGS, "Expect last error %#x, got %#lx\n",
           ERROR_INVALID_FLAGS, err);

        SetLastError(0xdeadbeef);
        ret = pSetupDiGetDevicePropertyKeys(set, &device_data, NULL, 0, &keys_len, 0);
        ok(!ret, "Expect failure\n");
        err = GetLastError();
        ok(err == ERROR_INSUFFICIENT_BUFFER, "Expect last error %#x, got %#lx\n",
           ERROR_INSUFFICIENT_BUFFER, err);

        keys = calloc(keys_len, sizeof(*keys));
        ok(keys_len && !!keys, "Failed to allocate buffer\n");
        SetLastError(0xdeadbeef);

        ret = pSetupDiGetDevicePropertyKeys(set, &device_data, keys, keys_len, &keys_len, 0xdeadbeef);
        ok(!ret, "Expect failure\n");
        err = GetLastError();
        ok(err == ERROR_INVALID_FLAGS, "Expect last error %#x, got %#lx\n",
           ERROR_INVALID_FLAGS, err);

        required_len = 0xdeadbeef;
#ifdef __REACTOS__
        ret = pSetupDiGetDevicePropertyKeys(set, &device_data, keys, keys_len, &required_len, 0);
#else
        ret = SetupDiGetDevicePropertyKeys(set, &device_data, keys, keys_len, &required_len, 0);
#endif
        ok(ret, "Expect success\n");
        err = GetLastError();
        ok(!err, "Expect last error %#x, got %#lx\n", ERROR_SUCCESS, err);
        ok(keys_len == required_len, "%lu != %lu\n", keys_len, required_len);
        ok(keys_len >= expected_keys, "Expected %lu >= %lu\n", keys_len, expected_keys);

        keys_len = 0;
        if (keys)
        {
            for (n = 0; n < required_len; n++)
            {
                if (!memcmp(&keys[n], &DEVPKEY_Device_FriendlyName, sizeof(keys[n])))
                    keys_len++;
            }

        }
        ok(keys_len == expected_keys, "%lu != %lu\n", keys_len, expected_keys);
        free(keys);
    }
    else
        win_skip("SetupDiGetDevicePropertyKeys not available\n");

    ret = SetupDiRemoveDevice(set, &device_data);
    ok(ret, "Got unexpected error %#lx.\n", GetLastError());

    SetupDiDestroyDeviceInfoList(set);
    FreeLibrary(hmod);
}

static void test_get_device_instance_id(void)
{
    BOOL ret;
    HDEVINFO set;
    SP_DEVINFO_DATA device = {0};
    char id[200];
    DWORD size;

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInstanceIdA(NULL, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInstanceIdA(NULL, &device, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected error %#lx.\n", GetLastError());

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInstanceIdA(set, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInstanceIdA(set, &device, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInstanceIdA(set, &device, NULL, 0, &size);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    device.cbSize = sizeof(device);
    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInstanceIdA(set, &device, NULL, 0, &size);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInstanceIdA(set, &device, NULL, 0, &size);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Got unexpected error %#lx.\n", GetLastError());

    ret = SetupDiGetDeviceInstanceIdA(set, &device, id, sizeof(id), NULL);
    ok(ret, "Failed to get device id, error %#lx.\n", GetLastError());
    ok(!strcmp(id, "ROOT\\LEGACY_BOGUS\\0000"), "Got unexpected id %s.\n", id);

    ret = SetupDiCreateDeviceInfoA(set, "LEGACY_BOGUS", &guid, NULL, NULL, DICD_GENERATE_ID, &device);
    ok(ret, "SetupDiCreateDeviceInfoA failed: %08lx\n", GetLastError());

    ret = SetupDiGetDeviceInstanceIdA(set, &device, id, sizeof(id), NULL);
    ok(ret, "Failed to get device id, error %#lx.\n", GetLastError());
    ok(!strcmp(id, "ROOT\\LEGACY_BOGUS\\0001"), "Got unexpected id %s.\n", id);

    SetupDiDestroyDeviceInfoList(set);
}

static void test_open_device_info(void)
{
    SP_DEVINFO_DATA device = {sizeof(device)};
    CHAR id[MAX_DEVICE_ID_LEN + 2];
    HDEVINFO set;
    DWORD i = 0;
    BOOL ret;

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device info, error %#lx.\n", GetLastError());

    /* Open non-existent device */
    SetLastError(0xdeadbeef);
    ret = SetupDiOpenDeviceInfoA(set, "Root\\LEGACY_BOGUS\\FFFF", NULL, 0, &device);
    ok(GetLastError() == ERROR_NO_SUCH_DEVINST, "Got unexpected error %#lx.\n", GetLastError());
    ok(!ret, "Expected failure.\n");
    check_device_info(set, 0, NULL, NULL);

    /* Open unregistered device */
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\1000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = SetupDiOpenDeviceInfoA(set, "Root\\LEGACY_BOGUS\\1000", NULL, 0, &device);
    ok(GetLastError() == ERROR_NO_SUCH_DEVINST, "Got unexpected error %#lx.\n", GetLastError());
    ok(!ret, "Expected failure.\n");
    check_device_info(set, 0, &guid, "Root\\LEGACY_BOGUS\\1000");
    check_device_info(set, 1, NULL, NULL);

    /* Open registered device */
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\1001", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());
    ret = SetupDiRegisterDeviceInfo(set, &device, 0, NULL, NULL, NULL);
    ok(ret, "Failed to register device, error %#lx.\n", GetLastError());
    check_device_info(set, 0, &guid, "Root\\LEGACY_BOGUS\\1000");
    check_device_info(set, 1, &guid, "Root\\LEGACY_BOGUS\\1001");
    check_device_info(set, 2, NULL, NULL);

    SetLastError(0xdeadbeef);
    ret = SetupDiOpenDeviceInfoA(set, "Root\\LEGACY_BOGUS\\1001", NULL, 0, &device);
    ok(GetLastError() == NO_ERROR, "Got unexpected error %#lx.\n", GetLastError());
    ok(ret, "Failed to open device info\n");
    ret = SetupDiGetDeviceInstanceIdA(set, &device, id, sizeof(id), NULL);
    ok(ret, "Got unexpected error %#lx.\n", GetLastError());
    ok(!strcasecmp(id, "Root\\LEGACY_BOGUS\\1001"), "Got unexpected id %s.\n", id);
    check_device_info(set, 0, &guid, "Root\\LEGACY_BOGUS\\1000");
    check_device_info(set, 1, &guid, "Root\\LEGACY_BOGUS\\1001");
    check_device_info(set, 2, NULL, NULL);

    /* Open registered device in an empty device info set */
    SetupDiDestroyDeviceInfoList(set);
    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device info list, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiOpenDeviceInfoA(set, "Root\\LEGACY_BOGUS\\1001", NULL, 0, &device);
    ok(GetLastError() == NO_ERROR, "Got unexpected error %#lx.\n", GetLastError());
    ok(ret, "Failed to open device info\n");
    ret = SetupDiGetDeviceInstanceIdA(set, &device, id, sizeof(id), NULL);
    ok(ret, "Got unexpected error %#lx.\n", GetLastError());
    ok(!strcasecmp(id, "Root\\LEGACY_BOGUS\\1001"), "Got unexpected id %s.\n", id);
    check_device_info(set, 0, &guid, "Root\\LEGACY_BOGUS\\1001");
    check_device_info(set, 1, NULL, NULL);

    /* Open registered device again */
    SetLastError(0xdeadbeef);
    ret = SetupDiOpenDeviceInfoA(set, "Root\\LEGACY_BOGUS\\1001", NULL, 0, &device);
    ok(GetLastError() == NO_ERROR, "Got unexpected error %#lx.\n", GetLastError());
    ok(ret, "Failed to open device info\n");
    ret = SetupDiGetDeviceInstanceIdA(set, &device, id, sizeof(id), NULL);
    ok(ret, "Got unexpected error %#lx.\n", GetLastError());
    ok(!strcasecmp(id, "Root\\LEGACY_BOGUS\\1001"), "Got unexpected id %s.\n", id);
    check_device_info(set, 0, &guid, "Root\\LEGACY_BOGUS\\1001");
    check_device_info(set, 1, NULL, NULL);

    /* Open registered device in a new device info set with wrong GUID */
    SetupDiDestroyDeviceInfoList(set);
    set = SetupDiCreateDeviceInfoList(&guid2, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device info list, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiOpenDeviceInfoA(set, "Root\\LEGACY_BOGUS\\1001", NULL, 0, &device);
    ok(GetLastError() == ERROR_CLASS_MISMATCH, "Got unexpected error %#lx.\n", GetLastError());
    ok(!ret, "Expected failure.\n");
    check_device_info(set, 0, NULL, NULL);

    /* Open registered device in a new device info set with NULL GUID */
    SetupDiDestroyDeviceInfoList(set);
    set = SetupDiCreateDeviceInfoList(NULL, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device info list, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiOpenDeviceInfoA(set, "Root\\LEGACY_BOGUS\\1001", NULL, 0, &device);
    ok(GetLastError() == NO_ERROR, "Got unexpected error %#lx.\n", GetLastError());
    ok(ret, "Failed to open device info\n");
    check_device_info(set, 0, &guid, "Root\\LEGACY_BOGUS\\1001");
    check_device_info(set, 1, NULL, NULL);

    SetupDiDestroyDeviceInfoList(set);
    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device info list, error %#lx.\n", GetLastError());

    /* NULL set */
    SetLastError(0xdeadbeef);
    ret = SetupDiOpenDeviceInfoA(NULL, "Root\\LEGACY_BOGUS\\1001", NULL, 0, &device);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected error %#lx.\n", GetLastError());
    ok(!ret, "Expected failure.\n");

    /* NULL instance id */
    SetLastError(0xdeadbeef);
    ret = SetupDiOpenDeviceInfoA(set, NULL, NULL, 0, &device);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());
    ok(!ret, "Expected failure.\n");
    check_device_info(set, 0, NULL, NULL);

    /* Invalid SP_DEVINFO_DATA cbSize, device will be added despite failure */
    SetLastError(0xdeadbeef);
    device.cbSize = 0;
    ret = SetupDiOpenDeviceInfoA(set, "Root\\LEGACY_BOGUS\\1001", NULL, 0, &device);
    device.cbSize = sizeof(device);
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "Got unexpected error %#lx.\n", GetLastError());
    ok(!ret, "Expected failure.\n");
    check_device_info(set, 0, &guid, "Root\\LEGACY_BOGUS\\1001");
    check_device_info(set, 1, NULL, NULL);

    SetupDiDestroyDeviceInfoList(set);
    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device info list, error %#lx.\n", GetLastError());

    /* NULL device */
    SetLastError(0xdeadbeef);
    ret = SetupDiOpenDeviceInfoA(set, "Root\\LEGACY_BOGUS\\1001", NULL, 0, NULL);
    ok(GetLastError() == NO_ERROR, "Got unexpected error %#lx.\n", GetLastError());
    ok(ret, "Failed to open device info\n");
    check_device_info(set, 0, &guid, "Root\\LEGACY_BOGUS\\1001");
    check_device_info(set, 1, NULL, NULL);

    /* Clean up */
    SetupDiDestroyDeviceInfoList(set);
    set = SetupDiGetClassDevsA(&guid, NULL, NULL, 0);
    while (SetupDiEnumDeviceInfo(set, i++, &device))
    {
        ret = SetupDiRemoveDevice(set, &device);
        ok(ret, "Failed to remove device, error %#lx.\n", GetLastError());
    }
    SetupDiDestroyDeviceInfoList(set);
}

static void test_register_device_info(void)
{
    SP_DEVINFO_DATA device = {0};
    BOOL ret;
    HDEVINFO set;
    HKEY hkey;
    LSTATUS ls;
    DWORD type = 0;
    DWORD phantom = 0;
    DWORD size;
    int i = 0;

    SetLastError(0xdeadbeef);
    ret = SetupDiRegisterDeviceInfo(NULL, NULL, 0, NULL, NULL, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected error %#lx.\n", GetLastError());

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiRegisterDeviceInfo(set, NULL, 0, NULL, NULL, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiRegisterDeviceInfo(set, &device, 0, NULL, NULL, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    device.cbSize = sizeof(device);
    SetLastError(0xdeadbeef);
    ret = SetupDiRegisterDeviceInfo(set, &device, 0, NULL, NULL, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());
    RegOpenKeyA(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Enum\\ROOT\\LEGACY_BOGUS\\0000", &hkey);
    size = sizeof(phantom);
    ls = RegQueryValueExA(hkey, "Phantom", NULL, &type, (BYTE *)&phantom, &size);
    ok(ls == ERROR_SUCCESS, "Got wrong error code %#lx\n", ls);
    ok(phantom == 1, "Got wrong phantom value %ld\n", phantom);
    ok(type == REG_DWORD, "Got wrong phantom type %#lx\n", type);
    ok(size == sizeof(phantom), "Got wrong phantom size %ld\n", size);
    ret = SetupDiRegisterDeviceInfo(set, &device, 0, NULL, NULL, NULL);
    ok(ret, "Failed to register device, error %#lx.\n", GetLastError());
    size = sizeof(phantom);
    ls = RegQueryValueExA(hkey, "Phantom", NULL, NULL, (BYTE *)&phantom, &size);
    ok(ls == ERROR_FILE_NOT_FOUND, "Got wrong error code %#lx\n", ls);
    RegCloseKey(hkey);

    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0001", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());
    ret = SetupDiRegisterDeviceInfo(set, &device, 0, NULL, NULL, NULL);
    ok(ret, "Failed to register device, error %#lx.\n", GetLastError());
    ret = SetupDiRemoveDevice(set, &device);
    ok(ret, "Failed to remove device, error %#lx.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0002", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());
    ret = SetupDiRegisterDeviceInfo(set, &device, 0, NULL, NULL, NULL);
    ok(ret, "Failed to register device, error %#lx.\n", GetLastError());
    ret = SetupDiDeleteDeviceInfo(set, &device);
    ok(ret, "Failed to remove device, error %#lx.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0003", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    SetupDiDestroyDeviceInfoList(set);

    set = SetupDiGetClassDevsA(&guid, NULL, NULL, 0);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());

    check_device_info(set, 0, &guid, "Root\\LEGACY_BOGUS\\0000");
    check_device_info(set, 1, &guid, "Root\\LEGACY_BOGUS\\0002");
    check_device_info(set, 2, &guid, NULL);

    while (SetupDiEnumDeviceInfo(set, i++, &device))
    {
        ret = SetupDiRemoveDevice(set, &device);
        ok(ret, "Failed to remove device, error %#lx.\n", GetLastError());
    }

    SetupDiDestroyDeviceInfoList(set);
}

static void check_all_lower_case(int line, const char* str)
{
    const char *cur;

    for (cur = str; *cur; cur++)
    {
        BOOL is_lower = (tolower(*cur) == *cur);
        ok_(__FILE__, line)(is_lower, "Expected device path to be all lowercase but got %s.\n", str);
        if (!is_lower) break;
    }
}

static void check_device_iface_(int line, HDEVINFO set, SP_DEVINFO_DATA *device,
        const GUID *class, int index, DWORD flags, const char *path)
{
    char buffer[200];
    SP_DEVICE_INTERFACE_DETAIL_DATA_A *detail = (SP_DEVICE_INTERFACE_DETAIL_DATA_A *)buffer;
    SP_DEVICE_INTERFACE_DATA iface = {sizeof(iface)};
    BOOL ret;

    detail->cbSize = sizeof(*detail);

    SetLastError(0xdeadbeef);
    ret = SetupDiEnumDeviceInterfaces(set, device, class, index, &iface);
    if (path)
    {
        ok_(__FILE__, line)(ret, "Failed to enumerate interfaces, error %#lx.\n", GetLastError());
        ok_(__FILE__, line)(IsEqualGUID(&iface.InterfaceClassGuid, class),
                "Got unexpected class %s.\n", wine_dbgstr_guid(&iface.InterfaceClassGuid));
        ok_(__FILE__, line)(iface.Flags == flags, "Got unexpected flags %#lx.\n", iface.Flags);
        ret = SetupDiGetDeviceInterfaceDetailA(set, &iface, detail, sizeof(buffer), NULL, NULL);
        ok_(__FILE__, line)(ret, "Failed to get interface detail, error %#lx.\n", GetLastError());
        ok_(__FILE__, line)(!strcasecmp(detail->DevicePath, path), "Got unexpected path %s.\n", detail->DevicePath);
        check_all_lower_case(line, detail->DevicePath);
    }
    else
    {
        ok_(__FILE__, line)(!ret, "Expected failure.\n");
        ok_(__FILE__, line)(GetLastError() == ERROR_NO_MORE_ITEMS,
                "Got unexpected error %#lx.\n", GetLastError());
    }
}
#define check_device_iface(a,b,c,d,e,f) check_device_iface_(__LINE__,a,b,c,d,e,f)

static void test_device_iface(void)
{
    char buffer[200];
    SP_DEVICE_INTERFACE_DETAIL_DATA_A *detail = (SP_DEVICE_INTERFACE_DETAIL_DATA_A *)buffer;
    SP_DEVINFO_DATA device = {0}, device2 = {sizeof(device2)};
    SP_DEVICE_INTERFACE_DATA iface = {sizeof(iface)};
    BOOL ret;
    HDEVINFO set;

    detail->cbSize = sizeof(*detail);

    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInterfaceA(NULL, NULL, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInterfaceA(NULL, NULL, &guid, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected error %#lx.\n", GetLastError());

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInterfaceA(set, NULL, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInterfaceA(set, &device, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    device.cbSize = sizeof(device);
    ret = SetupDiCreateDeviceInfoA(set, "ROOT\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    check_device_iface(set, &device, &guid, 0, 0, NULL);

    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInterfaceA(set, &device, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "Got unexpected error %#lx.\n", GetLastError());

    ret = SetupDiCreateDeviceInterfaceA(set, &device, &guid, NULL, 0, NULL);
    ok(ret, "Failed to create interface, error %#lx.\n", GetLastError());

    check_device_iface(set, &device, &guid, 0, 0, "\\\\?\\ROOT#LEGACY_BOGUS#0000#{6A55B5A4-3F65-11DB-B704-0011955C2BDB}");
    check_device_iface(set, &device, &guid, 1, 0, NULL);

    /* Creating the same interface a second time succeeds */
    ret = SetupDiCreateDeviceInterfaceA(set, &device, &guid, NULL, 0, NULL);
    ok(ret, "Failed to create interface, error %#lx.\n", GetLastError());

    check_device_iface(set, &device, &guid, 0, 0, "\\\\?\\ROOT#LEGACY_BOGUS#0000#{6A55B5A4-3F65-11DB-B704-0011955C2BDB}");
    check_device_iface(set, &device, &guid, 1, 0, NULL);

    ret = SetupDiCreateDeviceInterfaceA(set, &device, &guid, "Oogah", 0, NULL);
    ok(ret, "Failed to create interface, error %#lx.\n", GetLastError());

    check_device_iface(set, &device, &guid, 0, 0, "\\\\?\\ROOT#LEGACY_BOGUS#0000#{6A55B5A4-3F65-11DB-B704-0011955C2BDB}");
    check_device_iface(set, &device, &guid, 1, 0, "\\\\?\\ROOT#LEGACY_BOGUS#0000#{6A55B5A4-3F65-11DB-B704-0011955C2BDB}\\Oogah");
    check_device_iface(set, &device, &guid, 2, 0, NULL);

    ret = SetupDiCreateDeviceInterfaceA(set, &device, &guid, "test", 0, &iface);
    ok(ret, "Failed to create interface, error %#lx.\n", GetLastError());
    ok(IsEqualGUID(&iface.InterfaceClassGuid, &guid), "Got unexpected class %s.\n",
            wine_dbgstr_guid(&iface.InterfaceClassGuid));
    ok(iface.Flags == 0, "Got unexpected flags %#lx.\n", iface.Flags);
    ret = SetupDiGetDeviceInterfaceDetailA(set, &iface, detail, sizeof(buffer), NULL, NULL);
    ok(ret, "Failed to get interface detail, error %#lx.\n", GetLastError());
    ok(!strcasecmp(detail->DevicePath, "\\\\?\\ROOT#LEGACY_BOGUS#0000#{6A55B5A4-3F65-11DB-B704-0011955C2BDB}\\test"),
            "Got unexpected path %s.\n", detail->DevicePath);

    check_device_iface(set, &device, &guid, 0, 0, "\\\\?\\ROOT#LEGACY_BOGUS#0000#{6A55B5A4-3F65-11DB-B704-0011955C2BDB}");
    check_device_iface(set, &device, &guid, 1, 0, "\\\\?\\ROOT#LEGACY_BOGUS#0000#{6A55B5A4-3F65-11DB-B704-0011955C2BDB}\\Oogah");
    check_device_iface(set, &device, &guid, 2, 0, "\\\\?\\ROOT#LEGACY_BOGUS#0000#{6A55B5A4-3F65-11DB-B704-0011955C2BDB}\\test");
    check_device_iface(set, &device, &guid, 3, 0, NULL);

    ret = SetupDiCreateDeviceInterfaceA(set, &device, &guid2, NULL, 0, NULL);
    ok(ret, "Failed to create interface, error %#lx.\n", GetLastError());

    check_device_iface(set, &device, &guid2, 0, 0, "\\\\?\\ROOT#LEGACY_BOGUS#0000#{6A55B5A5-3F65-11DB-B704-0011955C2BDB}");
    check_device_iface(set, &device, &guid2, 1, 0, NULL);

    ret = SetupDiEnumDeviceInterfaces(set, &device, &guid2, 0, &iface);
    ok(ret, "Failed to enumerate interfaces, error %#lx.\n", GetLastError());
    ret = SetupDiRemoveDeviceInterface(set, &iface);
    ok(ret, "Failed to remove interface, error %#lx.\n", GetLastError());

    check_device_iface(set, &device, &guid2, 0, SPINT_REMOVED, "\\\\?\\ROOT#LEGACY_BOGUS#0000#{6A55B5A5-3F65-11DB-B704-0011955C2BDB}");
    check_device_iface(set, &device, &guid2, 1, 0, NULL);

    ret = SetupDiEnumDeviceInterfaces(set, &device, &guid, 0, &iface);
    ok(ret, "Failed to enumerate interfaces, error %#lx.\n", GetLastError());
    ret = SetupDiDeleteDeviceInterfaceData(set, &iface);
    ok(ret, "Failed to delete interface, error %#lx.\n", GetLastError());

    check_device_iface(set, &device, &guid, 0, 0, "\\\\?\\ROOT#LEGACY_BOGUS#0000#{6A55B5A4-3F65-11DB-B704-0011955C2BDB}\\Oogah");
    check_device_iface(set, &device, &guid, 1, 0, "\\\\?\\ROOT#LEGACY_BOGUS#0000#{6A55B5A4-3F65-11DB-B704-0011955C2BDB}\\test");
    check_device_iface(set, &device, &guid, 2, 0, NULL);

    ret = SetupDiCreateDeviceInfoA(set, "ROOT\\LEGACY_BOGUS\\0001", &guid, NULL, NULL, 0, &device2);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());
    ret = SetupDiCreateDeviceInterfaceA(set, &device2, &guid, NULL, 0, NULL);
    ok(ret, "Failed to create interface, error %#lx.\n", GetLastError());

    check_device_iface(set, NULL, &guid, 0, 0, "\\\\?\\ROOT#LEGACY_BOGUS#0000#{6A55B5A4-3F65-11DB-B704-0011955C2BDB}\\Oogah");
    check_device_iface(set, NULL, &guid, 1, 0, "\\\\?\\ROOT#LEGACY_BOGUS#0000#{6A55B5A4-3F65-11DB-B704-0011955C2BDB}\\test");
    check_device_iface(set, NULL, &guid, 2, 0, "\\\\?\\ROOT#LEGACY_BOGUS#0001#{6A55B5A4-3F65-11DB-B704-0011955C2BDB}");
    check_device_iface(set, NULL, &guid, 3, 0, NULL);

    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list, error %#lx.\n", GetLastError());
}

static void test_device_iface_detail(void)
{
    static const char path[] = "\\\\?\\root#legacy_bogus#0000#{6a55b5a4-3f65-11db-b704-0011955c2bdb}";
    SP_DEVICE_INTERFACE_DETAIL_DATA_A *detail;
    SP_DEVICE_INTERFACE_DATA iface = {sizeof(iface)};
    SP_DEVINFO_DATA device = {sizeof(device)};
    DWORD size = 0, expected_size;
    HDEVINFO set;
    BOOL ret;

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInterfaceDetailA(NULL, NULL, NULL, 0, NULL, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected error %#lx.\n", GetLastError());

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInterfaceDetailA(set, NULL, NULL, 0, NULL, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "ROOT\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInterfaceA(set, &device, &guid, NULL, 0, &iface);
    ok(ret, "Failed to create interface, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInterfaceDetailA(set, &iface, NULL, 0, NULL, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInterfaceDetailA(set, &iface, NULL, 100, NULL, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInterfaceDetailA(set, &iface, NULL, 0, &size, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Got unexpected error %#lx.\n", GetLastError());

    expected_size = FIELD_OFFSET(SP_DEVICE_INTERFACE_DETAIL_DATA_A, DevicePath[strlen(path) + 1]);
    ok(size == expected_size, "Expected size %lu, got %lu.\n", expected_size, size);
    detail = malloc(size * 2);

    detail->cbSize = 0;
    SetLastError(0xdeadbeef);
    size = 0xdeadbeef;
    ret = SetupDiGetDeviceInterfaceDetailA(set, &iface, detail, expected_size * 2, &size, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "Got unexpected error %#lx.\n", GetLastError());
    ok(size == 0xdeadbeef, "Expected size %lu, got %lu.\n", expected_size, size);

    detail->cbSize = size;
    SetLastError(0xdeadbeef);
    size = 0xdeadbeef;
    ret = SetupDiGetDeviceInterfaceDetailA(set, &iface, detail, expected_size * 2, &size, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "Got unexpected error %#lx.\n", GetLastError());
    ok(size == 0xdeadbeef, "Expected size %lu, got %lu.\n", expected_size, size);

    detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);
    SetLastError(0xdeadbeef);
    size = 0xdeadbeef;
    ret = SetupDiGetDeviceInterfaceDetailA(set, &iface, detail, expected_size, &size, NULL);
    ok(ret, "Failed to get interface detail, error %#lx.\n", GetLastError());
    ok(!strcasecmp(path, detail->DevicePath), "Got unexpected path %s.\n", detail->DevicePath);
    ok(size == expected_size, "Expected size %lu, got %lu.\n", expected_size, size);

    SetLastError(0xdeadbeef);
    size = 0xdeadbeef;
    ret = SetupDiGetDeviceInterfaceDetailA(set, &iface, detail, expected_size * 2, &size, NULL);
    ok(ret, "Failed to get interface detail, error %#lx.\n", GetLastError());
    ok(!strcasecmp(path, detail->DevicePath), "Got unexpected path %s.\n", detail->DevicePath);
    ok(size == expected_size, "Expected size %lu, got %lu.\n", expected_size, size);

    expected_size = FIELD_OFFSET(SP_DEVICE_INTERFACE_DETAIL_DATA_W, DevicePath[strlen(path) + 1]);

    size = 0xdeadbeef;
    ret = SetupDiGetDeviceInterfaceDetailW(set, &iface, NULL, 0, &size, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Got unexpected error %#lx.\n", GetLastError());
    ok(size == expected_size, "Expected size %lu, got %lu.\n", expected_size, size);

    memset(&device, 0, sizeof(device));
    device.cbSize = sizeof(device);
    size = 0xdeadbeef;
    ret = SetupDiGetDeviceInterfaceDetailW(set, &iface, NULL, 0, &size, &device);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Got unexpected error %#lx.\n", GetLastError());
    ok(size == expected_size, "Expected size %lu, got %lu.\n", expected_size, size);
    ok(IsEqualGUID(&device.ClassGuid, &guid), "Got unexpected class %s.\n", wine_dbgstr_guid(&device.ClassGuid));

    free(detail);
    SetupDiDestroyDeviceInfoList(set);
}

static void test_device_key(void)
{
    static const char params_key_path[] = "System\\CurrentControlSet\\Enum\\Root"
            "\\LEGACY_BOGUS\\0000\\Device Parameters";
    static const char class_key_path[] = "System\\CurrentControlSet\\Control\\Class"
            "\\{6a55b5a4-3f65-11db-b704-0011955c2bdb}";
    static const WCHAR bogus[] = {'S','y','s','t','e','m','\\',
     'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
     'E','n','u','m','\\','R','o','o','t','\\',
     'L','E','G','A','C','Y','_','B','O','G','U','S',0};
    SP_DEVINFO_DATA device = {sizeof(device)};
    char driver_path[50], data[4];
    HKEY class_key, key;
    DWORD size;
    BOOL ret;
    HDEVINFO set;
    LONG res;

    SetLastError(0xdeadbeef);
    key = SetupDiCreateDevRegKeyW(NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(key == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected error %#lx.\n", GetLastError());

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());

    res = RegOpenKeyW(HKEY_LOCAL_MACHINE, bogus, &key);
    ok(res != ERROR_SUCCESS, "Key should not exist.\n");
    RegCloseKey(key);

    ret = SetupDiCreateDeviceInfoA(set, "ROOT\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());
    ok(!RegOpenKeyW(HKEY_LOCAL_MACHINE, bogus, &key), "Key should exist.\n");
    RegCloseKey(key);

    SetLastError(0xdeadbeef);
    key = SetupDiOpenDevRegKey(NULL, NULL, 0, 0, 0, 0);
    ok(key == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    key = SetupDiOpenDevRegKey(set, NULL, 0, 0, 0, 0);
    ok(key == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    key = SetupDiOpenDevRegKey(set, &device, 0, 0, 0, 0);
    ok(key == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_FLAGS, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    key = SetupDiOpenDevRegKey(set, &device, DICS_FLAG_GLOBAL, 0, 0, 0);
    ok(key == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_FLAGS, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    key = SetupDiOpenDevRegKey(set, &device, DICS_FLAG_GLOBAL, 0, DIREG_BOTH, 0);
    ok(key == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_FLAGS, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    key = SetupDiOpenDevRegKey(set, &device, DICS_FLAG_GLOBAL, 0, DIREG_DRV, 0);
    ok(key == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_DEVINFO_NOT_REGISTERED, "Got unexpected error %#lx.\n", GetLastError());

    ret = SetupDiRegisterDeviceInfo(set, &device, 0, NULL, NULL, NULL);
    ok(ret, "Failed to register device, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    key = SetupDiOpenDevRegKey(set, &device, DICS_FLAG_GLOBAL, 0, DIREG_DRV, 0);
    ok(key == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_KEY_DOES_NOT_EXIST, "Got unexpected error %#lx.\n", GetLastError());

    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_DRIVER, NULL,
            (BYTE *)driver_path, sizeof(driver_path), NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, class_key_path, &key);
    ok(res == ERROR_FILE_NOT_FOUND, "Key should not exist.\n");
    RegCloseKey(key);

    /* Vista+ will fail the following call to SetupDiCreateDevKeyW() if the
     * class key doesn't exist. */
    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, class_key_path, &key);
    ok(!res, "Failed to create class key, error %lu.\n", res);
    RegCloseKey(key);

    key = SetupDiCreateDevRegKeyW(set, &device, DICS_FLAG_GLOBAL, 0, DIREG_DRV, NULL, NULL);
    ok(key != INVALID_HANDLE_VALUE, "Failed to create device key, error %#lx.\n", GetLastError());
    RegCloseKey(key);

    ok(!RegOpenKeyA(HKEY_LOCAL_MACHINE, class_key_path, &key), "Key should exist.\n");
    RegCloseKey(key);

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Control\\Class", &class_key);
    ok(!res, "Failed to open class key, error %lu.\n", res);

    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_DRIVER, NULL,
            (BYTE *)driver_path, sizeof(driver_path), NULL);
    ok(ret, "Failed to get driver property, error %#lx.\n", GetLastError());
    res = RegOpenKeyA(class_key, driver_path, &key);
    ok(!res, "Failed to open driver key, error %lu.\n", res);
    RegSetValueExA(key, "foo", 0, REG_SZ, (BYTE *)"bar", sizeof("bar"));
    RegCloseKey(key);

    SetLastError(0xdeadbeef);
    key = SetupDiOpenDevRegKey(set, &device, DICS_FLAG_GLOBAL, 0, DIREG_DRV, 0);
todo_wine {
    ok(key == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DATA || GetLastError() == ERROR_ACCESS_DENIED, /* win2k3 */
            "Got unexpected error %#lx.\n", GetLastError());
}

    key = SetupDiOpenDevRegKey(set, &device, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_READ);
    ok(key != INVALID_HANDLE_VALUE, "Failed to open device key, error %#lx.\n", GetLastError());
    size = sizeof(data);
    res = RegQueryValueExA(key, "foo", NULL, NULL, (BYTE *)data, &size);
    ok(!res, "Failed to get value, error %lu.\n", res);
    ok(!strcmp(data, "bar"), "Got wrong data %s.\n", data);
    RegCloseKey(key);

    ret = SetupDiDeleteDevRegKey(set, &device, DICS_FLAG_GLOBAL, 0, DIREG_DRV);
    ok(ret, "Failed to delete device key, error %#lx.\n", GetLastError());

    res = RegOpenKeyA(class_key, driver_path, &key);
    ok(res == ERROR_FILE_NOT_FOUND, "Key should not exist.\n");

    key = SetupDiOpenDevRegKey(set, &device, DICS_FLAG_GLOBAL, 0, DIREG_DRV, 0);
    ok(key == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_KEY_DOES_NOT_EXIST, "Got unexpected error %#lx.\n", GetLastError());

    key = SetupDiOpenDevRegKey(set, &device, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
    ok(key == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_KEY_DOES_NOT_EXIST, "Got unexpected error %#lx.\n", GetLastError());

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, params_key_path, &key);
    ok(res == ERROR_FILE_NOT_FOUND, "Key should not exist.\n");

    key = SetupDiCreateDevRegKeyA(set, &device, DICS_FLAG_GLOBAL, 0, DIREG_DEV, NULL, NULL);
    ok(key != INVALID_HANDLE_VALUE, "Got unexpected error %#lx.\n", GetLastError());
    RegCloseKey(key);

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, params_key_path, &key);
    ok(!res, "Failed to open device key, error %lu.\n", res);
    res = RegSetValueExA(key, "foo", 0, REG_SZ, (BYTE *)"bar", sizeof("bar"));
    ok(!res, "Failed to set value, error %lu.\n", res);
    RegCloseKey(key);

    key = SetupDiOpenDevRegKey(set, &device, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
    ok(key != INVALID_HANDLE_VALUE, "Got unexpected error %#lx.\n", GetLastError());
    size = sizeof(data);
    res = RegQueryValueExA(key, "foo", NULL, NULL, (BYTE *)data, &size);
    ok(!res, "Failed to get value, error %lu.\n", res);
    ok(!strcmp(data, "bar"), "Got wrong data %s.\n", data);
    RegCloseKey(key);

    ret = SetupDiDeleteDevRegKey(set, &device, DICS_FLAG_GLOBAL, 0, DIREG_DEV);
    ok(ret, "Got unexpected error %#lx.\n", GetLastError());

    key = SetupDiOpenDevRegKey(set, &device, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
    ok(key == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_KEY_DOES_NOT_EXIST, "Got unexpected error %#lx.\n", GetLastError());

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, params_key_path, &key);
    ok(res == ERROR_FILE_NOT_FOUND, "Key should not exist.\n");

    key = SetupDiCreateDevRegKeyW(set, &device, DICS_FLAG_GLOBAL, 0, DIREG_DRV, NULL, NULL);
    ok(key != INVALID_HANDLE_VALUE, "Failed to create device key, error %#lx.\n", GetLastError());
    RegCloseKey(key);

    ret = SetupDiRemoveDevice(set, &device);
    ok(ret, "Failed to remove device, error %#lx.\n", GetLastError());
    SetupDiDestroyDeviceInfoList(set);

    res = RegOpenKeyA(class_key, driver_path, &key);
    ok(res == ERROR_FILE_NOT_FOUND, "Key should not exist.\n");

    /* Vista+ deletes the key automatically. */
    res = RegDeleteKeyA(HKEY_LOCAL_MACHINE, class_key_path);
    ok(!res || res == ERROR_FILE_NOT_FOUND, "Failed to delete class key, error %lu.\n", res);

    RegCloseKey(class_key);
}

static void test_register_device_iface(void)
{
    static const WCHAR bogus[] = {'S','y','s','t','e','m','\\',
     'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
     'E','n','u','m','\\','R','o','o','t','\\',
     'L','E','G','A','C','Y','_','B','O','G','U','S',0};
    SP_DEVICE_INTERFACE_DATA iface = {sizeof(iface)};
    SP_DEVINFO_DATA device2 = {sizeof(device2)};
    SP_DEVINFO_DATA device = {sizeof(device)};
    HDEVINFO set, set2;
    BOOL ret;
    HKEY key;
    LONG res;

    set = SetupDiGetClassDevsA(&guid, NULL, 0, DIGCF_ALLCLASSES);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());

    res = RegOpenKeyW(HKEY_LOCAL_MACHINE, bogus, &key);
    ok(res == ERROR_FILE_NOT_FOUND, "Key should not exist.\n");

    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());
    ret = SetupDiCreateDeviceInterfaceA(set, &device, &guid, NULL, 0, &iface);
    ok(ret, "Failed to create interface, error %#lx.\n", GetLastError());
    ret = SetupDiCreateDeviceInterfaceA(set, &device, &guid, "removed", 0, &iface);
    ok(ret, "Failed to create interface, error %#lx.\n", GetLastError());
    ret = SetupDiCreateDeviceInterfaceA(set, &device, &guid, "deleted", 0, &iface);
    ok(ret, "Failed to create interface, error %#lx.\n", GetLastError());
    ret = SetupDiRegisterDeviceInfo(set, &device, 0, NULL, NULL, NULL);
    ok(ret, "Failed to register device, error %#lx.\n", GetLastError());

    ret = SetupDiEnumDeviceInterfaces(set, &device, &guid, 1, &iface);
    ok(ret, "Failed to enumerate interfaces, error %#lx.\n", GetLastError());
    ret = SetupDiRemoveDeviceInterface(set, &iface);
    ok(ret, "Failed to delete interface, error %#lx.\n", GetLastError());
    ret = SetupDiEnumDeviceInterfaces(set, &device, &guid, 2, &iface);
    ok(ret, "Failed to enumerate interfaces, error %#lx.\n", GetLastError());
    ret = SetupDiDeleteDeviceInterfaceData(set, &iface);
    ok(ret, "Failed to delete interface, error %#lx.\n", GetLastError());

    set2 = SetupDiGetClassDevsA(&guid, NULL, 0, DIGCF_DEVICEINTERFACE);
    ok(set2 != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());

    check_device_iface(set2, NULL, &guid, 0, 0, "\\\\?\\root#legacy_bogus#0000#{6a55b5a4-3f65-11db-b704-0011955c2bdb}");
    check_device_iface(set2, NULL, &guid, 1, 0, "\\\\?\\root#legacy_bogus#0000#{6a55b5a4-3f65-11db-b704-0011955c2bdb}\\deleted");
    check_device_iface(set2, NULL, &guid, 2, 0, NULL);

    ret = SetupDiEnumDeviceInfo(set2, 0, &device2);
    ok(ret, "Failed to enumerate devices, error %#lx.\n", GetLastError());
    ret = SetupDiCreateDeviceInterfaceA(set2, &device2, &guid, "second", 0, NULL);
    ok(ret, "Failed to create interface, error %#lx.\n", GetLastError());

    ret = SetupDiRemoveDevice(set, &device);
    ok(ret, "Failed to remove device, error %#lx.\n", GetLastError());

    check_device_iface(set, NULL, &guid, 0, SPINT_REMOVED, "\\\\?\\root#legacy_bogus#0000#{6a55b5a4-3f65-11db-b704-0011955c2bdb}");
    check_device_iface(set, NULL, &guid, 1, SPINT_REMOVED, "\\\\?\\root#legacy_bogus#0000#{6a55b5a4-3f65-11db-b704-0011955c2bdb}\\removed");
    check_device_iface(set, NULL, &guid, 2, 0, NULL);

    check_device_iface(set2, NULL, &guid, 0, 0, "\\\\?\\root#legacy_bogus#0000#{6a55b5a4-3f65-11db-b704-0011955c2bdb}");
    check_device_iface(set2, NULL, &guid, 1, 0, "\\\\?\\root#legacy_bogus#0000#{6a55b5a4-3f65-11db-b704-0011955c2bdb}\\deleted");
    check_device_iface(set2, NULL, &guid, 2, 0, "\\\\?\\root#legacy_bogus#0000#{6a55b5a4-3f65-11db-b704-0011955c2bdb}\\second");
    check_device_iface(set2, NULL, &guid, 3, 0, NULL);

    SetupDiDestroyDeviceInfoList(set);
    SetupDiDestroyDeviceInfoList(set2);

    /* make sure all interface keys are deleted when a device is removed */

    set = SetupDiGetClassDevsA(&guid, NULL, 0, DIGCF_DEVICEINTERFACE);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    set2 = SetupDiGetClassDevsA(&guid, NULL, 0, DIGCF_DEVICEINTERFACE);
    ok(set2 != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    check_device_iface(set2, NULL, &guid, 0, 0, NULL);
    SetupDiDestroyDeviceInfoList(set2);

    SetupDiDestroyDeviceInfoList(set);
}

static void test_registry_property_a(void)
{
    static const CHAR bogus[] = "System\\CurrentControlSet\\Enum\\Root\\LEGACY_BOGUS";
    SP_DEVINFO_DATA device = {sizeof(device)};
    CHAR buf[64] = "";
    DWORD size, type;
    HDEVINFO set;
    BOOL ret;
    LONG res;
    HKEY key;

    set = SetupDiGetClassDevsA(&guid, NULL, 0, DIGCF_DEVICEINTERFACE);
    ok(set != INVALID_HANDLE_VALUE, "Failed to get device list, error %#lx.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "LEGACY_BOGUS", &guid, NULL, NULL, DICD_GENERATE_ID, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiSetDeviceRegistryPropertyA(NULL, NULL, -1, NULL, 0);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiSetDeviceRegistryPropertyA(set, NULL, -1, NULL, 0);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiSetDeviceRegistryPropertyA(set, &device, -1, NULL, 0);
    ok(!ret, "Expected failure.\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_REG_PROPERTY, "Got unexpected error %#lx.\n", GetLastError());

    ret = SetupDiSetDeviceRegistryPropertyA(set, &device, SPDRP_FRIENDLYNAME, NULL, 0);
    todo_wine
    ok(!ret, "Expected failure.\n");
    /* GetLastError() returns nonsense in win2k3 */

    SetLastError(0xdeadbeef);
    ret = SetupDiSetDeviceRegistryPropertyA(set, &device, SPDRP_FRIENDLYNAME, (BYTE *)"Bogus", sizeof("Bogus"));
    ok(ret, "Failed to set property, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyA(NULL, NULL, -1, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyA(set, NULL, -1, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, -1, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_REG_PROPERTY, "Got unexpected error %#lx.\n", GetLastError());

    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_FRIENDLYNAME, NULL, NULL, sizeof("Bogus"), NULL);
    ok(!ret, "Expected failure, got %d\n", ret);
    /* GetLastError() returns nonsense in win2k3 */

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_FRIENDLYNAME, NULL, NULL, 0, &size);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Got unexpected error %#lx.\n", GetLastError());
    ok(size == sizeof("Bogus"), "Got unexpected size %ld.\n", size);

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_FRIENDLYNAME, NULL, (BYTE *)buf, sizeof(buf), NULL);
    ok(ret, "Failed to get property, error %#lx.\n", GetLastError());
    ok(!strcmp(buf, "Bogus"), "Got unexpected property %s.\n", buf);

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_FRIENDLYNAME, &type, (BYTE *)buf, sizeof(buf), NULL);
    ok(ret, "Failed to get property, error %#lx.\n", GetLastError());
    ok(!strcmp(buf, "Bogus"), "Got unexpected property %s.\n", buf);
    ok(type == REG_SZ, "Got unexpected type %ld.\n", type);

    SetLastError(0xdeadbeef);
    ret = SetupDiSetDeviceRegistryPropertyA(set, &device, SPDRP_FRIENDLYNAME, NULL, 0);
    ok(ret, "Failed to set property, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_FRIENDLYNAME, NULL, (BYTE *)buf, sizeof(buf), &size);
todo_wine {
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "Got unexpected error %#lx.\n", GetLastError());
}

    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_HARDWAREID, NULL, NULL, 0, &size);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "Got unexpected error %#lx.\n", GetLastError());

    SetupDiDestroyDeviceInfoList(set);

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, bogus, &key);
    ok(res == ERROR_FILE_NOT_FOUND, "Key should not exist.\n");

    /* Test existing registry properties right after device creation */
    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());

    /* Create device from a not registered class without device name */
    ret = SetupDiCreateDeviceInfoA(set, "LEGACY_BOGUS", &guid, NULL, NULL, DICD_GENERATE_ID, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    /* No SPDRP_DEVICEDESC property */
    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_DEVICEDESC, NULL, (BYTE *)buf, sizeof(buf), NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "Got unexpected error %#lx.\n", GetLastError());

    /* No SPDRP_CLASS property */
    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_CLASS, NULL, (BYTE *)buf, sizeof(buf), NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "Got unexpected error %#lx.\n", GetLastError());

    /* Have SPDRP_CLASSGUID property */
    memset(buf, 0, sizeof(buf));
    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_CLASSGUID, NULL, (BYTE *)buf, sizeof(buf), NULL);
    ok(ret, "Failed to get property, error %#lx.\n", GetLastError());
    ok(!lstrcmpiA(buf, "{6a55b5a4-3f65-11db-b704-0011955c2bdb}"), "Got unexpected value %s.\n", buf);

    ret = SetupDiDeleteDeviceInfo(set, &device);
    ok(ret, "Failed to delete device, error %#lx.\n", GetLastError());

    /* Create device from a not registered class with a device name */
    ret = SetupDiCreateDeviceInfoA(set, "LEGACY_BOGUS", &guid, "device_name", NULL, DICD_GENERATE_ID, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    /* Have SPDRP_DEVICEDESC property */
    memset(buf, 0, sizeof(buf));
    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_DEVICEDESC, NULL, (BYTE *)buf, sizeof(buf), NULL);
    ok(ret, "Failed to get property, error %#lx.\n", GetLastError());
    ok(!lstrcmpA(buf, "device_name"), "Got unexpected value %s.\n", buf);

    SetupDiDestroyDeviceInfoList(set);

    /* Create device from a registered class */
    set = SetupDiCreateDeviceInfoList(&GUID_DEVCLASS_DISPLAY, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "display", &GUID_DEVCLASS_DISPLAY, NULL, NULL, DICD_GENERATE_ID, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    /* Have SPDRP_CLASS property */
    memset(buf, 0, sizeof(buf));
    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_CLASS, NULL, (BYTE *)buf, sizeof(buf), NULL);
    ok(ret, "Failed to get property, error %#lx.\n", GetLastError());
    ok(!lstrcmpA(buf, "Display"), "Got unexpected value %s.\n", buf);

    SetupDiDestroyDeviceInfoList(set);
}

static void test_registry_property_w(void)
{
    WCHAR friendly_name[] = {'B','o','g','u','s',0};
    SP_DEVINFO_DATA device = {sizeof(device)};
    WCHAR buf[64] = {0};
    DWORD size, type;
    HDEVINFO set;
    BOOL ret;
    LONG res;
    HKEY key;
    static const WCHAR bogus[] = {'S','y','s','t','e','m','\\',
     'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
     'E','n','u','m','\\','R','o','o','t','\\',
     'L','E','G','A','C','Y','_','B','O','G','U','S',0};

    set = SetupDiGetClassDevsA(&guid, NULL, 0, DIGCF_DEVICEINTERFACE);
    ok(set != INVALID_HANDLE_VALUE, "Failed to get device list, error %#lx.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "LEGACY_BOGUS", &guid, NULL, NULL, DICD_GENERATE_ID, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiSetDeviceRegistryPropertyW(NULL, NULL, -1, NULL, 0);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiSetDeviceRegistryPropertyW(set, NULL, -1, NULL, 0);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiSetDeviceRegistryPropertyW(set, &device, -1, NULL, 0);
    ok(!ret, "Expected failure.\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_REG_PROPERTY, "Got unexpected error %#lx.\n", GetLastError());

    ret = SetupDiSetDeviceRegistryPropertyW(set, &device, SPDRP_FRIENDLYNAME, NULL, 0);
    todo_wine
    ok(!ret, "Expected failure.\n");
    /* GetLastError() returns nonsense in win2k3 */

    SetLastError(0xdeadbeef);
    ret = SetupDiSetDeviceRegistryPropertyW(set, &device, SPDRP_FRIENDLYNAME, (BYTE *)friendly_name, sizeof(friendly_name));
    ok(ret, "Failed to set property, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyW(NULL, NULL, -1, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyW(set, NULL, -1, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyW(set, &device, -1, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_REG_PROPERTY, "Got unexpected error %#lx.\n", GetLastError());

    ret = SetupDiGetDeviceRegistryPropertyW(set, &device, SPDRP_FRIENDLYNAME, NULL, NULL, sizeof(buf), NULL);
    ok(!ret, "Expected failure.\n");
    /* GetLastError() returns nonsense in win2k3 */

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyW(set, &device, SPDRP_FRIENDLYNAME, NULL, NULL, 0, &size);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Got unexpected error %#lx.\n", GetLastError());
    ok(size == sizeof(friendly_name), "Got unexpected size %ld.\n", size);

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyW(set, &device, SPDRP_FRIENDLYNAME, NULL, (BYTE *)buf, sizeof(buf), NULL);
    ok(ret, "Failed to get property, error %#lx.\n", GetLastError());
    ok(!lstrcmpW(buf, friendly_name), "Got unexpected property %s.\n", wine_dbgstr_w(buf));

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyW(set, &device, SPDRP_FRIENDLYNAME, &type, (BYTE *)buf, sizeof(buf), NULL);
    ok(ret, "Failed to get property, error %#lx.\n", GetLastError());
    ok(!lstrcmpW(buf, friendly_name), "Got unexpected property %s.\n", wine_dbgstr_w(buf));
    ok(type == REG_SZ, "Got unexpected type %ld.\n", type);

    SetLastError(0xdeadbeef);
    ret = SetupDiSetDeviceRegistryPropertyW(set, &device, SPDRP_FRIENDLYNAME, NULL, 0);
    ok(ret, "Failed to set property, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyW(set, &device, SPDRP_FRIENDLYNAME, NULL, (BYTE *)buf, sizeof(buf), &size);
todo_wine {
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "Got unexpected error %#lx.\n", GetLastError());
}

    ret = SetupDiGetDeviceRegistryPropertyW(set, &device, SPDRP_HARDWAREID, NULL, NULL, 0, &size);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "Got unexpected error %#lx.\n", GetLastError());

    SetupDiDestroyDeviceInfoList(set);

    res = RegOpenKeyW(HKEY_LOCAL_MACHINE, bogus, &key);
    ok(res == ERROR_FILE_NOT_FOUND, "Key should not exist.\n");

    /* Test existing registry properties right after device creation */
    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());

    /* Create device from a not registered class without device name */
    ret = SetupDiCreateDeviceInfoW(set, L"LEGACY_BOGUS", &guid, NULL, NULL, DICD_GENERATE_ID, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    /* No SPDRP_DEVICEDESC property */
    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyW(set, &device, SPDRP_DEVICEDESC, NULL, (BYTE *)buf, sizeof(buf), NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "Got unexpected error %#lx.\n", GetLastError());

    /* No SPDRP_CLASS property */
    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyW(set, &device, SPDRP_CLASS, NULL, (BYTE *)buf, sizeof(buf), NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "Got unexpected error %#lx.\n", GetLastError());

    /* Have SPDRP_CLASSGUID property */
    memset(buf, 0, sizeof(buf));
    ret = SetupDiGetDeviceRegistryPropertyW(set, &device, SPDRP_CLASSGUID, NULL, (BYTE *)buf, sizeof(buf), NULL);
    ok(ret, "Failed to get property, error %#lx.\n", GetLastError());
    ok(!lstrcmpiW(buf, L"{6a55b5a4-3f65-11db-b704-0011955c2bdb}"), "Got unexpected value %s.\n", wine_dbgstr_w(buf));

    ret = SetupDiDeleteDeviceInfo(set, &device);
    ok(ret, "Failed to delete device, error %#lx.\n", GetLastError());

    /* Create device from a not registered class with a device name */
    ret = SetupDiCreateDeviceInfoW(set, L"LEGACY_BOGUS", &guid, L"device_name", NULL, DICD_GENERATE_ID, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    /* Have SPDRP_DEVICEDESC property */
    memset(buf, 0, sizeof(buf));
    ret = SetupDiGetDeviceRegistryPropertyW(set, &device, SPDRP_DEVICEDESC, NULL, (BYTE *)buf, sizeof(buf), NULL);
    ok(ret, "Failed to get property, error %#lx.\n", GetLastError());
    ok(!lstrcmpW(buf, L"device_name"), "Got unexpected value %s.\n", wine_dbgstr_w(buf));

    SetupDiDestroyDeviceInfoList(set);

    /* Create device from a registered class */
    set = SetupDiCreateDeviceInfoList(&GUID_DEVCLASS_DISPLAY, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoW(set, L"display", &GUID_DEVCLASS_DISPLAY, NULL, NULL, DICD_GENERATE_ID, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    /* Have SPDRP_CLASS property */
    memset(buf, 0, sizeof(buf));
    ret = SetupDiGetDeviceRegistryPropertyW(set, &device, SPDRP_CLASS, NULL, (BYTE *)buf, sizeof(buf), NULL);
    ok(ret, "Failed to get property, error %#lx.\n", GetLastError());
    ok(!lstrcmpW(buf, L"Display"), "Got unexpected value %s.\n", wine_dbgstr_w(buf));

    SetupDiDestroyDeviceInfoList(set);
}

static void test_get_inf_class(void)
{
    static const char inffile[] = "winetest.inf";
    static const char content[] = "[Version]\r\n\r\n";
    static const char* signatures[] = {"\"$CHICAGO$\"", "\"$Windows NT$\""};
    static const GUID deadbeef_class_guid = {0xdeadbeef,0xdead,0xbeef,{0xde,0xad,0xbe,0xef,0xde,0xad,0xbe,0xef}};
    static const char deadbeef_class_name[] = "DeadBeef";

    char cn[MAX_PATH];
    char filename[MAX_PATH];
    DWORD count;
    BOOL retval;
    GUID guid;
    HANDLE h;
    int i;

    GetTempPathA(MAX_PATH, filename);
    strcat(filename, inffile);
    DeleteFileA(filename);

    /* not existing file */
    SetLastError(0xdeadbeef);
    retval = SetupDiGetINFClassA(filename, &guid, cn, MAX_PATH, &count);
    ok(!retval, "expected SetupDiGetINFClassA to fail!\n");
    ok(ERROR_FILE_NOT_FOUND == GetLastError(),
        "expected error ERROR_FILE_NOT_FOUND, got %lu\n", GetLastError());

    /* missing file wins against other invalid parameter */
    SetLastError(0xdeadbeef);
    retval = SetupDiGetINFClassA(filename, NULL, cn, MAX_PATH, &count);
    ok(!retval, "expected SetupDiGetINFClassA to fail!\n");
    ok(ERROR_FILE_NOT_FOUND == GetLastError(),
        "expected error ERROR_FILE_NOT_FOUND, got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    retval = SetupDiGetINFClassA(filename, &guid, NULL, MAX_PATH, &count);
    ok(!retval, "expected SetupDiGetINFClassA to fail!\n");
    ok(ERROR_FILE_NOT_FOUND == GetLastError(),
        "expected error ERROR_FILE_NOT_FOUND, got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    retval = SetupDiGetINFClassA(filename, &guid, cn, 0, &count);
    ok(!retval, "expected SetupDiGetINFClassA to fail!\n");
    ok(ERROR_FILE_NOT_FOUND == GetLastError(),
        "expected error ERROR_FILE_NOT_FOUND, got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    retval = SetupDiGetINFClassA(filename, &guid, cn, MAX_PATH, NULL);
    ok(!retval, "expected SetupDiGetINFClassA to fail!\n");
    ok(ERROR_FILE_NOT_FOUND == GetLastError(),
        "expected error ERROR_FILE_NOT_FOUND, got %lu\n", GetLastError());

    /* test file content */
    h = CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL, NULL);
    ok(h != INVALID_HANDLE_VALUE, "Failed to create file, error %#lx.\n", GetLastError());
    CloseHandle(h);

    retval = SetupDiGetINFClassA(filename, &guid, cn, MAX_PATH, &count);
    ok(!retval, "expected SetupDiGetINFClassA to fail!\n");

    for(i=0; i < ARRAY_SIZE(signatures); i++)
    {
        trace("testing signature %s\n", signatures[i]);
        h = CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL, NULL);
        ok(h != INVALID_HANDLE_VALUE, "Failed to create file, error %#lx.\n", GetLastError());
        WriteFile(h, content, sizeof(content), &count, NULL);
        CloseHandle(h);

        retval = SetupDiGetINFClassA(filename, &guid, cn, MAX_PATH, &count);
        ok(!retval, "expected SetupDiGetINFClassA to fail!\n");

        WritePrivateProfileStringA("Version", "Signature", signatures[i], filename);

        retval = SetupDiGetINFClassA(filename, &guid, cn, MAX_PATH, &count);
        ok(!retval, "expected SetupDiGetINFClassA to fail!\n");

        WritePrivateProfileStringA("Version", "Class", "WINE", filename);

        count = 0xdeadbeef;
        retval = SetupDiGetINFClassA(filename, &guid, cn, MAX_PATH, &count);
        ok(retval, "expected SetupDiGetINFClassA to succeed! error %lu\n", GetLastError());
        ok(count == 5, "expected count==5, got %lu\n", count);

        count = 0xdeadbeef;
        retval = SetupDiGetINFClassA(filename, &guid, cn, 5, &count);
        ok(retval, "expected SetupDiGetINFClassA to succeed! error %lu\n", GetLastError());
        ok(count == 5, "expected count==5, got %lu\n", count);

        count = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        retval = SetupDiGetINFClassA(filename, &guid, cn, 4, &count);
        ok(!retval, "expected SetupDiGetINFClassA to fail!\n");
        ok(ERROR_INSUFFICIENT_BUFFER == GetLastError(),
            "expected error ERROR_INSUFFICIENT_BUFFER, got %lu\n", GetLastError());
        ok(count == 5, "expected count==5, got %lu\n", count);

        /* invalid parameter */
        SetLastError(0xdeadbeef);
        retval = SetupDiGetINFClassA(NULL, &guid, cn, MAX_PATH, &count);
        ok(!retval, "expected SetupDiGetINFClassA to fail!\n");
        ok(ERROR_INVALID_PARAMETER == GetLastError(),
            "expected error ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

        SetLastError(0xdeadbeef);
        retval = SetupDiGetINFClassA(filename, NULL, cn, MAX_PATH, &count);
        ok(!retval, "expected SetupDiGetINFClassA to fail!\n");
        ok(ERROR_INVALID_PARAMETER == GetLastError(),
            "expected error ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

        SetLastError(0xdeadbeef);
        retval = SetupDiGetINFClassA(filename, &guid, NULL, MAX_PATH, &count);
        ok(!retval, "expected SetupDiGetINFClassA to fail!\n");
        ok(ERROR_INVALID_PARAMETER == GetLastError(),
            "expected error ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

        SetLastError(0xdeadbeef);
        retval = SetupDiGetINFClassA(filename, &guid, cn, 0, &count);
        ok(!retval, "expected SetupDiGetINFClassA to fail!\n");
        ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER || GetLastError() == ERROR_INVALID_PARAMETER /* 2k3+ */,
                "Got unexpected error %#lx.\n", GetLastError());

        DeleteFileA(filename);

        WritePrivateProfileStringA("Version", "Signature", signatures[i], filename);
        WritePrivateProfileStringA("Version", "ClassGUID", "WINE", filename);

        SetLastError(0xdeadbeef);
        retval = SetupDiGetINFClassA(filename, &guid, cn, MAX_PATH, &count);
        ok(!retval, "expected SetupDiGetINFClassA to fail!\n");
        ok(GetLastError() == RPC_S_INVALID_STRING_UUID || GetLastError() == ERROR_INVALID_PARAMETER /* 7+ */,
                "Got unexpected error %#lx.\n", GetLastError());

        /* network adapter guid */
        WritePrivateProfileStringA("Version", "ClassGUID",
                                   "{4d36e972-e325-11ce-bfc1-08002be10318}", filename);

        /* this test succeeds only if the guid is known to the system */
        count = 0xdeadbeef;
        retval = SetupDiGetINFClassA(filename, &guid, cn, MAX_PATH, &count);
        ok(retval, "expected SetupDiGetINFClassA to succeed! error %lu\n", GetLastError());
        todo_wine
        ok(count == 4, "expected count==4, got %lu(%s)\n", count, cn);

        /* Test Strings substitution */
        WritePrivateProfileStringA("Version", "Class", "%ClassName%", filename);
        WritePrivateProfileStringA("Version", "ClassGUID", "%ClassGuid%", filename);

        /* Without Strings section the ClassGUID is invalid (has non-substituted strkey token) */
        retval = SetupDiGetINFClassA(filename, &guid, cn, MAX_PATH, NULL);
        ok(!retval, "expected SetupDiGetINFClassA to fail\n");
        ok(GetLastError() == ERROR_INVALID_PARAMETER,
           "expected error ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

        /* With Strings section the ClassGUID and Class should be substituted */
        WritePrivateProfileStringA("Strings", "ClassName", deadbeef_class_name, filename);
        WritePrivateProfileStringA("Strings", "ClassGuid", "{deadbeef-dead-beef-dead-beefdeadbeef}", filename);
        count = 0xdeadbeef;
        retval = SetupDiGetINFClassA(filename, &guid, cn, MAX_PATH, &count);
        ok(retval, "expected SetupDiGetINFClassA to succeed! error %lu\n", GetLastError());
        ok(count == lstrlenA(deadbeef_class_name) + 1, "expected count=%d, got %lu\n", lstrlenA(deadbeef_class_name) + 1, count);
        ok(!lstrcmpA(deadbeef_class_name, cn), "expected class_name='%s', got '%s'\n", deadbeef_class_name, cn);
        ok(IsEqualGUID(&deadbeef_class_guid, &guid), "expected ClassGUID to be deadbeef-dead-beef-dead-beefdeadbeef\n");

        DeleteFileA(filename);
    }
}

static void test_devnode(void)
{
    HDEVINFO set;
    SP_DEVINFO_DATA device = { sizeof(SP_DEVINFO_DATA) };
    char buffer[50];
    DWORD ret;

    set = SetupDiGetClassDevsA(&guid, NULL, NULL, DIGCF_DEVICEINTERFACE);
    ok(set != INVALID_HANDLE_VALUE, "SetupDiGetClassDevs failed: %#lx\n", GetLastError());
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL,
        NULL, 0, &device);
    ok(ret, "SetupDiCreateDeviceInfo failed: %#lx\n", GetLastError());

    ret = CM_Get_Device_IDA(device.DevInst, buffer, sizeof(buffer), 0);
    ok(!ret, "got %#lx\n", ret);
    ok(!strcmp(buffer, "ROOT\\LEGACY_BOGUS\\0000"), "got %s\n", buffer);

    SetupDiDestroyDeviceInfoList(set);
}

static void test_device_interface_key(void)
{
    const char keypath[] = "System\\CurrentControlSet\\Control\\DeviceClasses\\"
        "{6a55b5a4-3f65-11db-b704-0011955c2bdb}\\"
        "##?#ROOT#LEGACY_BOGUS#0001#{6a55b5a4-3f65-11db-b704-0011955c2bdb}";
    SP_DEVICE_INTERFACE_DATA iface = { sizeof(iface) };
    SP_DEVINFO_DATA devinfo = { sizeof(devinfo) };
    HKEY parent, key, dikey;
    char buffer[5];
    HDEVINFO set;
    LONG sz, ret;

    set = SetupDiGetClassDevsA(NULL, NULL, 0, DIGCF_ALLCLASSES);
    ok(set != INVALID_HANDLE_VALUE, "SetupDiGetClassDevs failed: %#lx\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "ROOT\\LEGACY_BOGUS\\0001", &guid, NULL, NULL, 0, &devinfo);
    ok(ret, "SetupDiCreateDeviceInfo failed: %#lx\n", GetLastError());

    ret = SetupDiCreateDeviceInterfaceA(set, &devinfo, &guid, NULL, 0, &iface);
    ok(ret, "SetupDiCreateDeviceInterface failed: %#lx\n", GetLastError());

    ret = RegOpenKeyA(HKEY_LOCAL_MACHINE, keypath, &parent);
    ok(!ret, "failed to open device parent key: %lu\n", ret);

    ret = RegOpenKeyA(parent, "#\\Device Parameters", &key);
    ok(ret == ERROR_FILE_NOT_FOUND, "key shouldn't exist\n");

    dikey = SetupDiCreateDeviceInterfaceRegKeyA(set, &iface, 0, KEY_ALL_ACCESS, NULL, NULL);
    ok(dikey != INVALID_HANDLE_VALUE, "Got error %#lx\n", GetLastError());

    ret = RegOpenKeyA(parent, "#\\Device Parameters", &key);
    ok(!ret, "key should exist: %lu\n", ret);

    ret = RegSetValueA(key, NULL, REG_SZ, "test", 5);
    ok(!ret, "RegSetValue failed: %lu\n", ret);
    sz = sizeof(buffer);
    ret = RegQueryValueA(dikey, NULL, buffer, &sz);
    ok(!ret, "RegQueryValue failed: %lu\n", ret);
    ok(!strcmp(buffer, "test"), "got wrong data %s\n", buffer);

    RegCloseKey(dikey);
    RegCloseKey(key);

    ret = SetupDiDeleteDeviceInterfaceRegKey(set, &iface, 0);
    ok(ret, "Got error %#lx\n", GetLastError());

    ret = RegOpenKeyA(parent, "#\\Device Parameters", &key);
    ok(ret == ERROR_FILE_NOT_FOUND, "key shouldn't exist\n");

    RegCloseKey(parent);
    SetupDiRemoveDeviceInterface(set, &iface);
    SetupDiRemoveDevice(set, &devinfo);
    SetupDiDestroyDeviceInfoList(set);
}

static void test_open_device_interface_key(void)
{
    SP_DEVICE_INTERFACE_DATA iface;
    SP_DEVINFO_DATA device;
    CHAR buffer[5];
    HDEVINFO set;
    LSTATUS lr;
    LONG size;
    HKEY key;
    BOOL ret;

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx\n", GetLastError());

    device.cbSize = sizeof(device);
    ret = SetupDiCreateDeviceInfoA(set, "ROOT\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    iface.cbSize = sizeof(iface);
    ret = SetupDiCreateDeviceInterfaceA(set, &device, &guid, NULL, 0, &iface);
    ok(ret, "Failed to create interface, error %#lx.\n", GetLastError());

    /* Test open before creation */
    key = SetupDiOpenDeviceInterfaceRegKey(set, &iface, 0, KEY_ALL_ACCESS);
    ok(key == INVALID_HANDLE_VALUE, "Expect open interface registry key failure\n");

    /* Test opened key is from SetupDiCreateDeviceInterfaceRegKey */
    key = SetupDiCreateDeviceInterfaceRegKeyW(set, &iface, 0, KEY_ALL_ACCESS, NULL, NULL);
    ok(key != INVALID_HANDLE_VALUE, "Failed to create interface registry key, error %#lx\n", GetLastError());

    lr = RegSetValueA(key, NULL, REG_SZ, "test", 5);
    ok(!lr, "RegSetValue failed, error %#lx\n", lr);

    RegCloseKey(key);

    key = SetupDiOpenDeviceInterfaceRegKey(set, &iface, 0, KEY_ALL_ACCESS);
    ok(key != INVALID_HANDLE_VALUE, "Failed to open interface registry key, error %#lx\n", GetLastError());

    size = sizeof(buffer);
    lr = RegQueryValueA(key, NULL, buffer, &size);
    ok(!lr, "RegQueryValue failed, error %#lx\n", lr);
    ok(!strcmp(buffer, "test"), "got wrong data %s\n", buffer);

    RegCloseKey(key);

    /* Test open after removal */
    ret = SetupDiRemoveDeviceInterface(set, &iface);
    ok(ret, "Failed to remove device interface, error %#lx.\n", GetLastError());

    key = SetupDiOpenDeviceInterfaceRegKey(set, &iface, 0, KEY_ALL_ACCESS);
    ok(key == INVALID_HANDLE_VALUE, "Expect open interface registry key failure\n");

    ret = SetupDiRemoveDevice(set, &device);
    ok(ret, "Failed to remove device, error %#lx.\n", GetLastError());
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list, error %#lx.\n", GetLastError());
}

static void test_device_install_params(void)
{
    SP_DEVINFO_DATA device = {sizeof(device)};
    SP_DEVINSTALL_PARAMS_A params;
    HDEVINFO set;
    BOOL ret;

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    params.cbSize = sizeof(params) - 1;
    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInstallParamsA(set, &device, &params);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "Got unexpected error %#lx.\n", GetLastError());

    params.cbSize = sizeof(params) + 1;
    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInstallParamsA(set, &device, &params);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "Got unexpected error %#lx.\n", GetLastError());

    params.cbSize = sizeof(params) - 1;
    SetLastError(0xdeadbeef);
    ret = SetupDiSetDeviceInstallParamsA(set, &device, &params);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "Got unexpected error %#lx.\n", GetLastError());

    params.cbSize = sizeof(params) + 1;
    SetLastError(0xdeadbeef);
    ret = SetupDiSetDeviceInstallParamsA(set, &device, &params);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "Got unexpected error %#lx.\n", GetLastError());

    memset(&params, 0xcc, sizeof(params));
    params.cbSize = sizeof(params);
    ret = SetupDiGetDeviceInstallParamsA(set, &device, &params);
    ok(ret, "Failed to get device install params, error %#lx.\n", GetLastError());
    ok(!params.Flags, "Got flags %#lx.\n", params.Flags);
    ok(!params.FlagsEx, "Got extended flags %#lx.\n", params.FlagsEx);
    ok(!params.hwndParent, "Got parent %p.\n", params.hwndParent);
    ok(!params.InstallMsgHandler, "Got callback %p.\n", params.InstallMsgHandler);
    ok(!params.InstallMsgHandlerContext, "Got callback context %p.\n", params.InstallMsgHandlerContext);
    ok(!params.FileQueue, "Got queue %p.\n", params.FileQueue);
    ok(!params.ClassInstallReserved, "Got class installer data %#Ix.\n", params.ClassInstallReserved);
    ok(!params.DriverPath[0], "Got driver path %s.\n", params.DriverPath);

    params.Flags = DI_INF_IS_SORTED;
    params.FlagsEx = DI_FLAGSEX_ALLOWEXCLUDEDDRVS;
    strcpy(params.DriverPath, "C:\\windows");
    ret = SetupDiSetDeviceInstallParamsA(set, &device, &params);
    ok(ret, "Failed to set device install params, error %#lx.\n", GetLastError());

    memset(&params, 0xcc, sizeof(params));
    params.cbSize = sizeof(params);
    ret = SetupDiGetDeviceInstallParamsA(set, &device, &params);
    ok(ret, "Failed to get device install params, error %#lx.\n", GetLastError());
    ok(params.Flags == DI_INF_IS_SORTED, "Got flags %#lx.\n", params.Flags);
    ok(params.FlagsEx == DI_FLAGSEX_ALLOWEXCLUDEDDRVS, "Got extended flags %#lx.\n", params.FlagsEx);
    ok(!params.hwndParent, "Got parent %p.\n", params.hwndParent);
    ok(!params.InstallMsgHandler, "Got callback %p.\n", params.InstallMsgHandler);
    ok(!params.InstallMsgHandlerContext, "Got callback context %p.\n", params.InstallMsgHandlerContext);
    ok(!params.FileQueue, "Got queue %p.\n", params.FileQueue);
    ok(!params.ClassInstallReserved, "Got class installer data %#Ix.\n", params.ClassInstallReserved);
    ok(!strcasecmp(params.DriverPath, "C:\\windows"), "Got driver path %s.\n", params.DriverPath);

    SetupDiDestroyDeviceInfoList(set);
}

#ifdef __i386__
#define MYEXT "x86"
#define WOWEXT "AMD64"
#define WRONGEXT "ARM"
#elif defined(__x86_64__)
#define MYEXT "AMD64"
#define WOWEXT "x86"
#define WRONGEXT "ARM64"
#elif defined(__arm__)
#define MYEXT "ARM"
#define WOWEXT "ARM64"
#define WRONGEXT "x86"
#elif defined(__aarch64__)
#define MYEXT "ARM64"
#define WOWEXT "ARM"
#define WRONGEXT "AMD64"
#else
#define MYEXT
#define WOWEXT
#define WRONGEXT
#endif

static void test_get_actual_section(void)
{
    static const char inf_data[] = "[Version]\n"
            "Signature=\"$Chicago$\"\n"
            "[section1]\n"
            "[section2]\n"
            "[section2.nt]\n"
            "[section3]\n"
            "[section3.nt" MYEXT "]\n"
            "[section4]\n"
            "[section4.nt]\n"
            "[section4.nt" MYEXT "]\n"
            "[section5.nt]\n"
            "[section6.nt" MYEXT "]\n"
            "[section7]\n"
            "[section7.nt" WRONGEXT "]\n"
            "[section8.nt" WRONGEXT "]\n"
            "[section9.nt" MYEXT "]\n"
            "[section9.nt" WOWEXT "]\n"
            "[section10.nt" WOWEXT "]\n";

    char inf_path[MAX_PATH], section[LINE_LEN], *extptr;
    DWORD size;
    HINF hinf;
    BOOL ret;

    GetTempPathA(sizeof(inf_path), inf_path);
    strcat(inf_path, "setupapi_test.inf");
    create_file(inf_path, inf_data);

    hinf = SetupOpenInfFileA(inf_path, NULL, INF_STYLE_WIN4, NULL);
    ok(hinf != INVALID_HANDLE_VALUE, "Failed to open INF file, error %#lx.\n", GetLastError());

    ret = SetupDiGetActualSectionToInstallA(hinf, "section1", section, ARRAY_SIZE(section), NULL, NULL);
    ok(ret, "Failed to get section, error %#lx.\n", GetLastError());
    ok(!strcmp(section, "section1"), "Got unexpected section %s.\n", section);

    size = 0xdeadbeef;
    ret = SetupDiGetActualSectionToInstallA(hinf, "section1", NULL, 5, &size, NULL);
    ok(ret, "Failed to get section, error %#lx.\n", GetLastError());
    ok(size == 9, "Got size %lu.\n", size);

    SetLastError(0xdeadbeef);
    size = 0xdeadbeef;
    ret = SetupDiGetActualSectionToInstallA(hinf, "section1", section, 5, &size, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Got unexpected error %#lx.\n", GetLastError());
    ok(size == 9, "Got size %lu.\n", size);

    SetLastError(0xdeadbeef);
    ret = SetupDiGetActualSectionToInstallA(hinf, "section1", section, ARRAY_SIZE(section), &size, NULL);
    ok(ret, "Failed to get section, error %#lx.\n", GetLastError());
    ok(!strcasecmp(section, "section1"), "Got unexpected section %s.\n", section);
    ok(size == 9, "Got size %lu.\n", size);

    extptr = section;
    ret = SetupDiGetActualSectionToInstallA(hinf, "section2", section, ARRAY_SIZE(section), NULL, &extptr);
    ok(ret, "Failed to get section, error %#lx.\n", GetLastError());
    ok(!strcasecmp(section, "section2.NT"), "Got unexpected section %s.\n", section);
    ok(extptr == section + 8, "Got extension %s.\n", extptr);

    extptr = section;
    ret = SetupDiGetActualSectionToInstallA(hinf, "section3", section, ARRAY_SIZE(section), NULL, &extptr);
    ok(ret, "Failed to get section, error %#lx.\n", GetLastError());
    ok(!strcasecmp(section, "section3.NT" MYEXT), "Got unexpected section %s.\n", section);
    ok(extptr == section + 8, "Got extension %s.\n", extptr);

    extptr = section;
    ret = SetupDiGetActualSectionToInstallA(hinf, "section4", section, ARRAY_SIZE(section), NULL, &extptr);
    ok(ret, "Failed to get section, error %#lx.\n", GetLastError());
    ok(!strcasecmp(section, "section4.NT" MYEXT), "Got unexpected section %s.\n", section);
    ok(extptr == section + 8, "Got extension %s.\n", extptr);

    extptr = section;
    ret = SetupDiGetActualSectionToInstallA(hinf, "section5", section, ARRAY_SIZE(section), NULL, &extptr);
    ok(ret, "Failed to get section, error %#lx.\n", GetLastError());
    ok(!strcasecmp(section, "section5.NT"), "Got unexpected section %s.\n", section);
    ok(extptr == section + 8, "Got extension %s.\n", extptr);

    extptr = section;
    ret = SetupDiGetActualSectionToInstallA(hinf, "section6", section, ARRAY_SIZE(section), NULL, &extptr);
    ok(ret, "Failed to get section, error %#lx.\n", GetLastError());
    ok(!strcasecmp(section, "section6.NT" MYEXT), "Got unexpected section %s.\n", section);
    ok(extptr == section + 8, "Got extension %s.\n", extptr);

    extptr = section;
    ret = SetupDiGetActualSectionToInstallA(hinf, "section9", section, ARRAY_SIZE(section), NULL, &extptr);
    ok(ret, "Failed to get section, error %#lx.\n", GetLastError());
    ok(!strcasecmp(section, "section9.NT" MYEXT), "Got unexpected section %s.\n", section);
    ok(extptr == section + 8, "Got extension %s.\n", extptr);

    if (0)
    {
        /* For some reason, these calls hang on Windows 10 1809+. */
        extptr = section;
        ret = SetupDiGetActualSectionToInstallA(hinf, "section1", section, ARRAY_SIZE(section), NULL, &extptr);
        ok(ret, "Failed to get section, error %#lx.\n", GetLastError());
        ok(!strcasecmp(section, "section1"), "Got unexpected section %s.\n", section);
        ok(!extptr || !*extptr /* Windows 10 1809 */, "Got extension %s.\n", extptr);

        extptr = section;
        ret = SetupDiGetActualSectionToInstallA(hinf, "section7", section, ARRAY_SIZE(section), NULL, &extptr);
        ok(ret, "Failed to get section, error %#lx.\n", GetLastError());
        ok(!strcasecmp(section, "section7"), "Got unexpected section %s.\n", section);
        ok(!extptr || !*extptr /* Windows 10 1809 */, "Got extension %s.\n", extptr);

        extptr = section;
        ret = SetupDiGetActualSectionToInstallA(hinf, "section8", section, ARRAY_SIZE(section), NULL, &extptr);
        ok(ret, "Failed to get section, error %#lx.\n", GetLastError());
        ok(!strcasecmp(section, "section8"), "Got unexpected section %s.\n", section);
        ok(!extptr || !*extptr /* Windows 10 1809 */, "Got extension %s.\n", extptr);

        extptr = section;
        ret = SetupDiGetActualSectionToInstallA(hinf, "nonexistent", section, ARRAY_SIZE(section), NULL, &extptr);
        ok(ret, "Failed to get section, error %#lx.\n", GetLastError());
        ok(!strcasecmp(section, "nonexistent"), "Got unexpected section %s.\n", section);
        ok(!extptr || !*extptr /* Windows 10 1809 */, "Got extension %s.\n", extptr);

        extptr = section;
        ret = SetupDiGetActualSectionToInstallA(hinf, "section10", section, ARRAY_SIZE(section), NULL, &extptr);
        ok(ret, "Failed to get section, error %#lx.\n", GetLastError());
        ok(!strcasecmp(section, "section10"), "Got unexpected section %s.\n", section);
        ok(!extptr, "Got extension %s.\n", extptr);
    }

    SetupCloseInfFile(hinf);
    ret = DeleteFileA(inf_path);
    ok(ret, "Failed to delete %s, error %lu.\n", inf_path, GetLastError());
}

static void test_driver_list(void)
{
    char detail_buffer[1000];
    SP_DRVINFO_DETAIL_DATA_A *detail = (SP_DRVINFO_DETAIL_DATA_A *)detail_buffer;
    char short_path[MAX_PATH], inf_dir[MAX_PATH], inf_path[MAX_PATH + 10], inf_path2[MAX_PATH + 10];
    static const char hardware_id[] = "bogus_hardware_id\0other_hardware_id\0";
    static const char compat_id[] = "bogus_compat_id\0";
    SP_DEVINSTALL_PARAMS_A params = {sizeof(params)};
    SP_DRVINFO_DATA_A driver = {sizeof(driver)};
    SP_DEVINFO_DATA device = {sizeof(device)};
    DWORD size, expect_size;
    FILETIME filetime;
    HDEVINFO set;
    HANDLE file;
    DWORD idx;
    BOOL ret;

    static const char inf_data[] = "[Version]\n"
            "Signature=\"$Chicago$\"\n"
            "ClassGuid={6a55b5a4-3f65-11db-b704-0011955c2bdb}\n"
            "[Manufacturer]\n"
            "mfg1=mfg1_key,NT" MYEXT "\n"
            "mfg2=mfg2_key,NT" MYEXT "\n"
            "mfg1_wow=mfg1_key,NT" WOWEXT "\n"
            "mfg2_wow=mfg2_key,NT" WOWEXT "\n"
            "mfg3=mfg3_key,NT" WRONGEXT "\n"
            "[mfg1_key.nt" MYEXT "]\n"
            "desc0=,other_hardware_id,bogus_compat_id\n"
            "desc1=install1,bogus_hardware_id\n"
            "desc2=,bogus_hardware_id\n"
            "desc3=,wrong_hardware_id\n"
            "desc4=,wrong_hardware_id,bogus_compat_id\n"
            "[mfg1_key.nt" WOWEXT "]\n"
            "desc0=,other_hardware_id,bogus_compat_id\n"
            "desc1=install1,bogus_hardware_id\n"
            "desc2=,bogus_hardware_id\n"
            "desc3=,wrong_hardware_id\n"
            "desc4=,wrong_hardware_id,bogus_compat_id\n"
            "[mfg2_key.nt" MYEXT "]\n"
            "desc5=,bogus_hardware_id\n"
            "[mfg2_key.nt" WOWEXT "]\n"
            "desc5=,bogus_hardware_id\n"
            "[mfg3_key.nt" WRONGEXT "]\n"
            "desc6=,bogus_hardware_id\n"
            "[install1.nt" MYEXT "]\n"
            "[install1.nt" WRONGEXT "]\n";

    static const char inf_data_file1[] = "[Version]\n"
            "Signature=\"$Chicago$\"\n"
            "ClassGuid={6a55b5a4-3f65-11db-b704-0011955c2bdb}\n"
            "[Manufacturer]\n"
            "mfg1=mfg1_key,NT" MYEXT ",NT" WOWEXT "\n"
            "[mfg1_key.nt" MYEXT "]\n"
            "desc1=,bogus_hardware_id\n"
            "[mfg1_key.nt" WOWEXT "]\n"
            "desc1=,bogus_hardware_id\n";

    static const char inf_data_file2[] = "[Version]\n"
            "Signature=\"$Chicago$\"\n"
            "ClassGuid={6a55b5a5-3f65-11db-b704-0011955c2bdb}\n"
            "[Manufacturer]\n"
            "mfg1=mfg1_key,NT" MYEXT ",NT" WOWEXT "\n"
            "[mfg1_key.nt" MYEXT "]\n"
            "desc2=,bogus_hardware_id\n"
            "[mfg1_key.nt" WOWEXT "]\n"
            "desc2=,bogus_hardware_id\n";

    GetTempPathA(sizeof(inf_path), inf_path);
    GetShortPathNameA(inf_path, short_path, sizeof(short_path));
    strcat(inf_path, "setupapi_test.inf");
    strcat(short_path, "setupapi_test.inf");
    create_file(inf_path, inf_data);
    file = CreateFileA(inf_path, 0, 0, NULL, OPEN_EXISTING, 0, 0);
    GetFileTime(file, NULL, NULL, &filetime);
    CloseHandle(file);

    set = SetupDiCreateDeviceInfoList(NULL, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    ret = SetupDiCreateDeviceInfoA(set, "Root\\BOGUS\\0000", &GUID_NULL, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    ret = SetupDiSetDeviceRegistryPropertyA(set, &device, SPDRP_HARDWAREID,
            (const BYTE *)hardware_id, sizeof(hardware_id));
    ok(ret, "Failed to set hardware ID, error %#lx.\n", GetLastError());

    ret = SetupDiSetDeviceRegistryPropertyA(set, &device, SPDRP_COMPATIBLEIDS,
            (const BYTE *)compat_id, sizeof(compat_id));
    ok(ret, "Failed to set hardware ID, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiEnumDriverInfoA(set, &device, SPDIT_COMPATDRIVER, 0, &driver);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_NO_MORE_ITEMS, "Got unexpected error %#lx.\n", GetLastError());

    ret = SetupDiGetDeviceInstallParamsA(set, &device, &params);
    ok(ret, "Failed to get device install params, error %#lx.\n", GetLastError());
    strcpy(params.DriverPath, inf_path);
    params.Flags = DI_ENUMSINGLEINF;
    ret = SetupDiSetDeviceInstallParamsA(set, &device, &params);
    ok(ret, "Failed to set device install params, error %#lx.\n", GetLastError());

    ret = SetupDiBuildDriverInfoList(set, &device, SPDIT_COMPATDRIVER);
    ok(ret, "Failed to build driver list, error %#lx.\n", GetLastError());

    idx = 0;
    ret = SetupDiEnumDriverInfoA(set, &device, SPDIT_COMPATDRIVER, idx++, &driver);
    ok(ret, "Failed to enumerate drivers, error %#lx.\n", GetLastError());
    ok(driver.DriverType == SPDIT_COMPATDRIVER, "Got wrong type %#lx.\n", driver.DriverType);
    ok(!strcmp(driver.Description, "desc0"), "Got wrong description '%s'.\n", driver.Description);
    ok(!strcmp(driver.MfgName, wow64 ? "mfg1_wow" : "mfg1"), "Got wrong manufacturer '%s'.\n", driver.MfgName);
    ok(!strcmp(driver.ProviderName, ""), "Got wrong provider '%s'.\n", driver.ProviderName);

    ret = SetupDiEnumDriverInfoA(set, &device, SPDIT_COMPATDRIVER, idx++, &driver);
    ok(ret, "Failed to enumerate drivers, error %#lx.\n", GetLastError());
    ok(driver.DriverType == SPDIT_COMPATDRIVER, "Got wrong type %#lx.\n", driver.DriverType);
    ok(!strcmp(driver.Description, "desc1"), "Got wrong description '%s'.\n", driver.Description);
    ok(!strcmp(driver.MfgName, wow64 ? "mfg1_wow" : "mfg1"), "Got wrong manufacturer '%s'.\n", driver.MfgName);
    ok(!strcmp(driver.ProviderName, ""), "Got wrong provider '%s'.\n", driver.ProviderName);

    expect_size = FIELD_OFFSET(SP_DRVINFO_DETAIL_DATA_A, HardwareID[sizeof("bogus_hardware_id\0")]);

    ret = SetupDiGetDriverInfoDetailA(set, &device, &driver, NULL, 0, &size);
    ok(ret || GetLastError() == ERROR_INVALID_USER_BUFFER /* Win10 1809 */,
            "Failed to get driver details, error %#lx.\n", GetLastError());
    ok(size == expect_size, "Got size %lu.\n", size);

    ret = SetupDiGetDriverInfoDetailA(set, &device, &driver, NULL, sizeof(SP_DRVINFO_DETAIL_DATA_A) - 1, &size);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "Got unexpected error %#lx.\n", GetLastError());
    ok(size == expect_size, "Got size %lu.\n", size);

    size = 0xdeadbeef;
    ret = SetupDiGetDriverInfoDetailA(set, &device, &driver, detail, 0, &size);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "Got unexpected error %#lx.\n", GetLastError());
    ok(size == 0xdeadbeef, "Got size %lu.\n", size);

    size = 0xdeadbeef;
    detail->CompatIDsLength = 0xdeadbeef;
    ret = SetupDiGetDriverInfoDetailA(set, &device, &driver, detail, sizeof(SP_DRVINFO_DETAIL_DATA_A) - 1, &size);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "Got unexpected error %#lx.\n", GetLastError());
    ok(size == 0xdeadbeef, "Got size %lu.\n", size);
    ok(detail->CompatIDsLength == 0xdeadbeef, "Got wrong compat IDs length %lu.\n", detail->CompatIDsLength);

    memset(detail_buffer, 0xcc, sizeof(detail_buffer));
    detail->cbSize = sizeof(*detail);
    ret = SetupDiGetDriverInfoDetailA(set, &device, &driver, detail, sizeof(SP_DRVINFO_DETAIL_DATA_A), NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Got unexpected error %#lx.\n", GetLastError());
    ok(detail->InfDate.dwHighDateTime == filetime.dwHighDateTime
            && detail->InfDate.dwLowDateTime == filetime.dwLowDateTime,
            "Expected %#lx%08lx, got %#lx%08lx.\n", filetime.dwHighDateTime, filetime.dwLowDateTime,
            detail->InfDate.dwHighDateTime, detail->InfDate.dwLowDateTime);
    ok(!strcmp(detail->SectionName, "install1"), "Got section name %s.\n", debugstr_a(detail->SectionName));
    ok(!stricmp(detail->InfFileName, short_path), "Got INF file name %s.\n", debugstr_a(detail->InfFileName));
    ok(!strcmp(detail->DrvDescription, "desc1"), "Got description %s.\n", debugstr_a(detail->DrvDescription));
    ok(!detail->CompatIDsOffset || detail->CompatIDsOffset == sizeof("bogus_hardware_id") /* Win10 1809 */,
            "Got wrong compat IDs offset %lu.\n", detail->CompatIDsOffset);
    ok(!detail->CompatIDsLength, "Got wrong compat IDs length %lu.\n", detail->CompatIDsLength);
    ok(!detail->HardwareID[0], "Got wrong ID list.\n");

    size = 0xdeadbeef;
    ret = SetupDiGetDriverInfoDetailA(set, &device, &driver, detail, sizeof(SP_DRVINFO_DETAIL_DATA_A), &size);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Got unexpected error %#lx.\n", GetLastError());
    ok(size == expect_size, "Got size %lu.\n", size);

    size = 0xdeadbeef;
    ret = SetupDiGetDriverInfoDetailA(set, &device, &driver, detail, sizeof(detail_buffer), &size);
    ok(ret, "Failed to get driver details, error %#lx.\n", GetLastError());
    ok(size == expect_size, "Got size %lu.\n", size);
    ok(detail->InfDate.dwHighDateTime == filetime.dwHighDateTime
            && detail->InfDate.dwLowDateTime == filetime.dwLowDateTime,
            "Expected %#lx%08lx, got %#lx%08lx.\n", filetime.dwHighDateTime, filetime.dwLowDateTime,
            detail->InfDate.dwHighDateTime, detail->InfDate.dwLowDateTime);
    ok(!strcmp(detail->SectionName, "install1"), "Got section name %s.\n", debugstr_a(detail->SectionName));
    ok(!stricmp(detail->InfFileName, short_path), "Got INF file name %s.\n", debugstr_a(detail->InfFileName));
    ok(!strcmp(detail->DrvDescription, "desc1"), "Got description %s.\n", debugstr_a(detail->DrvDescription));
    ok(!detail->CompatIDsOffset, "Got wrong compat IDs offset %lu.\n", detail->CompatIDsOffset);
    ok(!detail->CompatIDsLength, "Got wrong compat IDs length %lu.\n", detail->CompatIDsLength);
    ok(!memcmp(detail->HardwareID, "bogus_hardware_id\0", sizeof("bogus_hardware_id\0")), "Got wrong ID list.\n");

    ret = SetupDiEnumDriverInfoA(set, &device, SPDIT_COMPATDRIVER, idx++, &driver);
    ok(ret, "Failed to enumerate drivers, error %#lx.\n", GetLastError());
    ok(driver.DriverType == SPDIT_COMPATDRIVER, "Got wrong type %#lx.\n", driver.DriverType);
    ok(!strcmp(driver.Description, "desc2"), "Got wrong description '%s'.\n", driver.Description);
    ok(!strcmp(driver.MfgName, wow64 ? "mfg1_wow" : "mfg1"), "Got wrong manufacturer '%s'.\n", driver.MfgName);
    ok(!strcmp(driver.ProviderName, ""), "Got wrong provider '%s'.\n", driver.ProviderName);

    ret = SetupDiEnumDriverInfoA(set, &device, SPDIT_COMPATDRIVER, idx++, &driver);
    ok(ret, "Failed to enumerate drivers, error %#lx.\n", GetLastError());
    ok(driver.DriverType == SPDIT_COMPATDRIVER, "Got wrong type %#lx.\n", driver.DriverType);
    ok(!strcmp(driver.Description, "desc4"), "Got wrong description '%s'.\n", driver.Description);
    ok(!strcmp(driver.MfgName, wow64 ? "mfg1_wow" : "mfg1"), "Got wrong manufacturer '%s'.\n", driver.MfgName);
    ok(!strcmp(driver.ProviderName, ""), "Got wrong provider '%s'.\n", driver.ProviderName);
    ret = SetupDiGetDriverInfoDetailA(set, &device, &driver, detail, sizeof(detail_buffer), NULL);
    ok(ret, "Failed to get driver details, error %#lx.\n", GetLastError());
    ok(detail->InfDate.dwHighDateTime == filetime.dwHighDateTime
            && detail->InfDate.dwLowDateTime == filetime.dwLowDateTime,
            "Expected %#lx%08lx, got %#lx%08lx.\n", filetime.dwHighDateTime, filetime.dwLowDateTime,
            detail->InfDate.dwHighDateTime, detail->InfDate.dwLowDateTime);
    ok(!detail->SectionName[0], "Got section name %s.\n", debugstr_a(detail->SectionName));
    ok(!stricmp(detail->InfFileName, short_path), "Got INF file name %s.\n", debugstr_a(detail->InfFileName));
    ok(!strcmp(detail->DrvDescription, "desc4"), "Got description %s.\n", debugstr_a(detail->DrvDescription));
    ok(detail->CompatIDsOffset == sizeof("wrong_hardware_id"), "Got wrong compat IDs offset %lu.\n", detail->CompatIDsOffset);
    ok(detail->CompatIDsLength == sizeof("bogus_compat_id\0"), "Got wrong compat IDs length %lu.\n", detail->CompatIDsLength);
    ok(!memcmp(detail->HardwareID, "wrong_hardware_id\0bogus_compat_id\0",
            sizeof("wrong_hardware_id\0bogus_compat_id\0")), "Got wrong ID list.\n");

    ret = SetupDiEnumDriverInfoA(set, &device, SPDIT_COMPATDRIVER, idx++, &driver);
    ok(ret, "Failed to enumerate drivers, error %#lx.\n", GetLastError());
    ok(driver.DriverType == SPDIT_COMPATDRIVER, "Got wrong type %#lx.\n", driver.DriverType);
    ok(!strcmp(driver.Description, "desc5"), "Got wrong description '%s'.\n", driver.Description);
    ok(!strcmp(driver.MfgName, wow64 ? "mfg2_wow" : "mfg2"), "Got wrong manufacturer '%s'.\n", driver.MfgName);
    ok(!strcmp(driver.ProviderName, ""), "Got wrong provider '%s'.\n", driver.ProviderName);

    SetLastError(0xdeadbeef);
    ret = SetupDiEnumDriverInfoA(set, &device, SPDIT_COMPATDRIVER, idx++, &driver);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_NO_MORE_ITEMS, "Got unexpected error %#lx.\n", GetLastError());

    ret = SetupDiGetSelectedDriverA(set, &device, &driver);
    ok(ret /* Win10 1809 */ || GetLastError() == ERROR_NO_DRIVER_SELECTED,
            "Got unexpected error %#lx.\n", GetLastError());

    ret = SetupDiSelectBestCompatDrv(set, &device);
    ok(ret, "Failed to select driver, error %#lx.\n", GetLastError());

    ret = SetupDiGetSelectedDriverA(set, &device, &driver);
    ok(ret, "Failed to get selected driver, error %#lx.\n", GetLastError());
    ok(driver.DriverType == SPDIT_COMPATDRIVER, "Got wrong type %#lx.\n", driver.DriverType);
    ok(!strcmp(driver.Description, "desc1"), "Got wrong description '%s'.\n", driver.Description);
    ok(!strcmp(driver.MfgName, wow64 ? "mfg1_wow" : "mfg1"), "Got wrong manufacturer '%s'.\n", driver.MfgName);
    ok(!strcmp(driver.ProviderName, ""), "Got wrong provider '%s'.\n", driver.ProviderName);

    SetupDiDestroyDeviceInfoList(set);
    ret = DeleteFileA(inf_path);
    ok(ret, "Failed to delete %s, error %lu.\n", inf_path, GetLastError());

    /* Test building from a path. */

    GetTempPathA(sizeof(inf_dir), inf_dir);
    strcat(inf_dir, "setupapi_test");
    ret = CreateDirectoryA(inf_dir, NULL);
    ok(ret, "Failed to create directory, error %lu.\n", GetLastError());
    sprintf(inf_path, "%s/test1.inf", inf_dir);
    create_file(inf_path, inf_data_file1);
    sprintf(inf_path2, "%s/test2.inf", inf_dir);
    create_file(inf_path2, inf_data_file2);

    set = SetupDiCreateDeviceInfoList(NULL, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    ret = SetupDiCreateDeviceInfoA(set, "Root\\BOGUS\\0000", &GUID_NULL, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    ret = SetupDiSetDeviceRegistryPropertyA(set, &device, SPDRP_HARDWAREID,
            (const BYTE *)hardware_id, sizeof(hardware_id));
    ok(ret, "Failed to set hardware ID, error %#lx.\n", GetLastError());

    ret = SetupDiGetDeviceInstallParamsA(set, &device, &params);
    ok(ret, "Failed to get device install params, error %#lx.\n", GetLastError());
    strcpy(params.DriverPath, inf_dir);
    ret = SetupDiSetDeviceInstallParamsA(set, &device, &params);
    ok(ret, "Failed to set device install params, error %#lx.\n", GetLastError());

    ret = SetupDiBuildDriverInfoList(set, &device, SPDIT_COMPATDRIVER);
    ok(ret, "Failed to build driver list, error %#lx.\n", GetLastError());

    ret = SetupDiEnumDriverInfoA(set, &device, SPDIT_COMPATDRIVER, 0, &driver);
    ok(ret, "Failed to enumerate drivers, error %#lx.\n", GetLastError());
    ok(driver.DriverType == SPDIT_COMPATDRIVER, "Got wrong type %#lx.\n", driver.DriverType);
    ok(!strcmp(driver.Description, "desc1"), "Got wrong description '%s'.\n", driver.Description);
    ok(!strcmp(driver.MfgName, "mfg1"), "Got wrong manufacturer '%s'.\n", driver.MfgName);
    ok(!strcmp(driver.ProviderName, ""), "Got wrong provider '%s'.\n", driver.ProviderName);

    ret = SetupDiEnumDriverInfoA(set, &device, SPDIT_COMPATDRIVER, 1, &driver);
    ok(ret, "Failed to enumerate drivers, error %#lx.\n", GetLastError());
    ok(driver.DriverType == SPDIT_COMPATDRIVER, "Got wrong type %#lx.\n", driver.DriverType);
    ok(!strcmp(driver.Description, "desc2"), "Got wrong description '%s'.\n", driver.Description);
    ok(!strcmp(driver.MfgName, "mfg1"), "Got wrong manufacturer '%s'.\n", driver.MfgName);
    ok(!strcmp(driver.ProviderName, ""), "Got wrong provider '%s'.\n", driver.ProviderName);

    SetLastError(0xdeadbeef);
    ret = SetupDiEnumDriverInfoA(set, &device, SPDIT_COMPATDRIVER, 2, &driver);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_NO_MORE_ITEMS, "Got unexpected error %#lx.\n", GetLastError());

    SetupDiDestroyDeviceInfoList(set);
    ret = DeleteFileA(inf_path);
    ok(ret, "Failed to delete %s, error %lu.\n", inf_path, GetLastError());
    ret = DeleteFileA(inf_path2);
    ok(ret, "Failed to delete %s, error %lu.\n", inf_path2, GetLastError());
    ret = RemoveDirectoryA(inf_dir);
    ok(ret, "Failed to delete %s, error %lu.\n", inf_dir, GetLastError());

    /* Test the default path. */

    create_file("C:/windows/inf/wine_test1.inf", inf_data_file1);
    create_file("C:/windows/inf/wine_test2.inf", inf_data_file2);

    set = SetupDiCreateDeviceInfoList(NULL, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    ret = SetupDiCreateDeviceInfoA(set, "Root\\BOGUS\\0000", &GUID_NULL, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    ret = SetupDiSetDeviceRegistryPropertyA(set, &device, SPDRP_HARDWAREID,
            (const BYTE *)hardware_id, sizeof(hardware_id));
    ok(ret, "Failed to set hardware ID, error %#lx.\n", GetLastError());

    ret = SetupDiBuildDriverInfoList(set, &device, SPDIT_COMPATDRIVER);
    ok(ret, "Failed to build driver list, error %#lx.\n", GetLastError());

    ret = SetupDiEnumDriverInfoA(set, &device, SPDIT_COMPATDRIVER, 0, &driver);
    ok(ret, "Failed to enumerate drivers, error %#lx.\n", GetLastError());
    ok(driver.DriverType == SPDIT_COMPATDRIVER, "Got wrong type %#lx.\n", driver.DriverType);
    ok(!strcmp(driver.Description, "desc1"), "Got wrong description '%s'.\n", driver.Description);
    ok(!strcmp(driver.MfgName, "mfg1"), "Got wrong manufacturer '%s'.\n", driver.MfgName);
    ok(!strcmp(driver.ProviderName, ""), "Got wrong provider '%s'.\n", driver.ProviderName);

    ret = SetupDiEnumDriverInfoA(set, &device, SPDIT_COMPATDRIVER, 1, &driver);
    ok(ret, "Failed to enumerate drivers, error %#lx.\n", GetLastError());
    ok(driver.DriverType == SPDIT_COMPATDRIVER, "Got wrong type %#lx.\n", driver.DriverType);
    ok(!strcmp(driver.Description, "desc2"), "Got wrong description '%s'.\n", driver.Description);
    ok(!strcmp(driver.MfgName, "mfg1"), "Got wrong manufacturer '%s'.\n", driver.MfgName);
    ok(!strcmp(driver.ProviderName, ""), "Got wrong provider '%s'.\n", driver.ProviderName);

    SetLastError(0xdeadbeef);
    ret = SetupDiEnumDriverInfoA(set, &device, SPDIT_COMPATDRIVER, 2, &driver);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_NO_MORE_ITEMS, "Got unexpected error %#lx.\n", GetLastError());

    SetupDiDestroyDeviceInfoList(set);
    ret = DeleteFileA("C:/windows/inf/wine_test1.inf");
    ok(ret, "Failed to delete %s, error %lu.\n", inf_path, GetLastError());
    ret = DeleteFileA("C:/windows/inf/wine_test2.inf");
    ok(ret, "Failed to delete %s, error %lu.\n", inf_path2, GetLastError());
    /* Windows "precompiles" INF files in this dir; try to avoid leaving them behind. */
    DeleteFileA("C:/windows/inf/wine_test1.pnf");
    DeleteFileA("C:/windows/inf/wine_test2.pnf");
}

static BOOL device_is_registered(HDEVINFO set, SP_DEVINFO_DATA *device)
{
    HKEY key = SetupDiOpenDevRegKey(set, device, DICS_FLAG_GLOBAL, 0, DIREG_DRV, 0);
    ok(key == INVALID_HANDLE_VALUE, "Expected failure.\n");
    RegCloseKey(key);
    return GetLastError() == ERROR_KEY_DOES_NOT_EXIST;
}

static unsigned int *coinst_callback_count;
static DI_FUNCTION *coinst_last_message;

static void test_class_installer(void)
{
    SP_DEVINFO_DATA device = {sizeof(device)};
    char regdata[200];
    HKEY class_key;
    HDEVINFO set;
    BOOL ret;
    LONG res;

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Control\\Class"
            "\\{6a55b5a4-3f65-11db-b704-0011955c2bdb}", &class_key);
    ok(!res, "Failed to create class key, error %lu.\n", res);

    strcpy(regdata, "winetest_coinst.dll,class_success");
    res = RegSetValueExA(class_key, "Installer32", 0, REG_SZ, (BYTE *)regdata, strlen(regdata)+1);
    ok(!res, "Failed to set registry value, error %lu.\n", res);

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    ret = SetupDiCallClassInstaller(DIF_ALLOW_INSTALL, set, &device);
    ok(ret, "Failed to call class installer, error %#lx.\n", GetLastError());

    ok(*coinst_callback_count == 1, "Got %d callbacks.\n", *coinst_callback_count);
    ok(*coinst_last_message == DIF_ALLOW_INSTALL, "Got unexpected message %#x.\n", *coinst_last_message);
    *coinst_callback_count = 0;

    ret = SetupDiCallClassInstaller(0xdeadbeef, set, &device);
    ok(ret, "Failed to call class installer, error %#lx.\n", GetLastError());

    ok(*coinst_callback_count == 1, "Got %d callbacks.\n", *coinst_callback_count);
    ok(*coinst_last_message == 0xdeadbeef, "Got unexpected message %#x.\n", *coinst_last_message);
    *coinst_callback_count = 0;

    ok(!device_is_registered(set, &device), "Expected device not to be registered.\n");
    ret = SetupDiCallClassInstaller(DIF_REGISTERDEVICE, set, &device);
    ok(ret, "Failed to call class installer, error %#lx.\n", GetLastError());
    ok(!device_is_registered(set, &device), "Expected device not to be registered.\n");

    ok(*coinst_callback_count == 1, "Got %d callbacks.\n", *coinst_callback_count);
    ok(*coinst_last_message == DIF_REGISTERDEVICE, "Got unexpected message %#x.\n", *coinst_last_message);
    *coinst_callback_count = 0;

    ret = SetupDiCallClassInstaller(DIF_REMOVE, set, &device);
    ok(ret, "Failed to call class installer, error %#lx.\n", GetLastError());
    ok(!device_is_registered(set, &device), "Expected device not to be registered.\n");

    ok(*coinst_callback_count == 1, "Got %d callbacks.\n", *coinst_callback_count);
    ok(*coinst_last_message == DIF_REMOVE, "Got unexpected message %#x.\n", *coinst_last_message);
    *coinst_callback_count = 0;

    SetLastError(0xdeadbeef);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list.\n");
    ok(!GetLastError(), "Got unexpected error %#lx.\n", GetLastError());

    ok(*coinst_callback_count == 1, "Got %d callbacks.\n", *coinst_callback_count);
    ok(*coinst_last_message == DIF_DESTROYPRIVATEDATA, "Got unexpected message %#x.\n", *coinst_last_message);
    *coinst_callback_count = 0;

    /* Test returning an error. */

    strcpy(regdata, "winetest_coinst.dll,class_error");
    res = RegSetValueExA(class_key, "Installer32", 0, REG_SZ, (BYTE *)regdata, strlen(regdata)+1);
    ok(!res, "Failed to set registry value, error %lu.\n", res);

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    ret = SetupDiCallClassInstaller(DIF_ALLOW_INSTALL, set, &device);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == 0xdeadc0de, "Got unexpected error %#lx.\n", GetLastError());

    ok(!device_is_registered(set, &device), "Expected device not to be registered.\n");
    ret = SetupDiCallClassInstaller(DIF_REGISTERDEVICE, set, &device);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == 0xdeadc0de, "Got unexpected error %#lx.\n", GetLastError());
    ok(!device_is_registered(set, &device), "Expected device not to be registered.\n");

    ret = SetupDiCallClassInstaller(DIF_REMOVE, set, &device);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == 0xdeadc0de, "Got unexpected error %#lx.\n", GetLastError());
    ok(!device_is_registered(set, &device), "Expected device not to be registered.\n");

    SetLastError(0xdeadbeef);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list.\n");
    ok(!GetLastError(), "Got unexpected error %#lx.\n", GetLastError());

    /* Test returning ERROR_DI_DO_DEFAULT. */

    strcpy(regdata, "winetest_coinst.dll,class_default");
    res = RegSetValueExA(class_key, "Installer32", 0, REG_SZ, (BYTE *)regdata, strlen(regdata)+1);
    ok(!res, "Failed to set registry value, error %lu.\n", res);

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    ret = SetupDiCallClassInstaller(DIF_ALLOW_INSTALL, set, &device);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_DI_DO_DEFAULT, "Got unexpected error %#lx.\n", GetLastError());

    ok(!device_is_registered(set, &device), "Expected device not to be registered.\n");
    ret = SetupDiCallClassInstaller(DIF_REGISTERDEVICE, set, &device);
    ok(ret, "Failed to call class installer, error %#lx.\n", GetLastError());
    ok(device_is_registered(set, &device), "Expected device to be registered.\n");

    ret = SetupDiCallClassInstaller(DIF_REMOVE, set, &device);
    ok(ret, "Failed to call class installer, error %#lx.\n", GetLastError());
    ok(!device_is_registered(set, &device), "Expected device not to be registered.\n");

    SetLastError(0xdeadbeef);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list.\n");
    ok(!GetLastError(), "Got unexpected error %#lx.\n", GetLastError());

    /* The default entry point is ClassInstall(). */

    strcpy(regdata, "winetest_coinst.dll");
    res = RegSetValueExA(class_key, "Installer32", 0, REG_SZ, (BYTE *)regdata, strlen(regdata)+1);
    ok(!res, "Failed to set registry value, error %lu.\n", res);

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    ret = SetupDiCallClassInstaller(DIF_ALLOW_INSTALL, set, &device);
    ok(ret, "Failed to call class installer, error %#lx.\n", GetLastError());

    ok(*coinst_callback_count == 1, "Got %d callbacks.\n", *coinst_callback_count);
    ok(*coinst_last_message == DIF_ALLOW_INSTALL, "Got unexpected message %#x.\n", *coinst_last_message);
    *coinst_callback_count = 0;

    SetLastError(0xdeadbeef);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list.\n");
    ok(!GetLastError(), "Got unexpected error %#lx.\n", GetLastError());

    ok(*coinst_callback_count == 1, "Got %d callbacks.\n", *coinst_callback_count);
    ok(*coinst_last_message == DIF_DESTROYPRIVATEDATA, "Got unexpected message %#x.\n", *coinst_last_message);
    *coinst_callback_count = 0;

    res = RegDeleteKeyA(class_key, "");
    ok(!res, "Failed to delete class key, error %lu.\n", res);
    RegCloseKey(class_key);
}

static void test_class_coinstaller(void)
{
    SP_DEVINFO_DATA device = {sizeof(device)};
    char regdata[200];
    HKEY coinst_key;
    HDEVINFO set;
    BOOL ret;
    LONG res;

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Control\\CoDeviceInstallers", &coinst_key);
    ok(!res, "Failed to open CoDeviceInstallers key, error %lu.\n", res);
    strcpy(regdata, "winetest_coinst.dll,co_success");
    regdata[strlen(regdata) + 1] = 0;
    res = RegSetValueExA(coinst_key, "{6a55b5a4-3f65-11db-b704-0011955c2bdb}", 0,
            REG_MULTI_SZ, (BYTE *)regdata, strlen(regdata) + 2);
    ok(!res, "Failed to set registry value, error %lu.\n", res);

    /* We must recreate the device list, or Windows will not recognize that the
     * class co-installer exists. */
    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    ret = SetupDiCallClassInstaller(DIF_ALLOW_INSTALL, set, &device);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_DI_DO_DEFAULT, "Got unexpected error %#lx.\n", GetLastError());

    ok(*coinst_callback_count == 1, "Got %d callbacks.\n", *coinst_callback_count);
    ok(*coinst_last_message == DIF_ALLOW_INSTALL, "Got unexpected message %#x.\n", *coinst_last_message);
    *coinst_callback_count = 0;

    ret = SetupDiCallClassInstaller(0xdeadbeef, set, &device);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_DI_DO_DEFAULT, "Got unexpected error %#lx.\n", GetLastError());

    ok(*coinst_callback_count == 1, "Got %d callbacks.\n", *coinst_callback_count);
    ok(*coinst_last_message == 0xdeadbeef, "Got unexpected message %#x.\n", *coinst_last_message);
    *coinst_callback_count = 0;

    ok(!device_is_registered(set, &device), "Expected device not to be registered.\n");
    ret = SetupDiCallClassInstaller(DIF_REGISTERDEVICE, set, &device);
    ok(ret, "Failed to call class installer, error %#lx.\n", GetLastError());
    ok(device_is_registered(set, &device), "Expected device to be registered.\n");

    ok(*coinst_callback_count == 1, "Got %d callbacks.\n", *coinst_callback_count);
    ok(*coinst_last_message == DIF_REGISTERDEVICE, "Got unexpected message %#x.\n", *coinst_last_message);
    *coinst_callback_count = 0;

    ret = SetupDiCallClassInstaller(DIF_REMOVE, set, &device);
    ok(ret, "Failed to call class installer, error %#lx.\n", GetLastError());
    ok(!device_is_registered(set, &device), "Expected device not to be registered.\n");

    ok(*coinst_callback_count == 1, "Got %d callbacks.\n", *coinst_callback_count);
    ok(*coinst_last_message == DIF_REMOVE, "Got unexpected message %#x.\n", *coinst_last_message);
    *coinst_callback_count = 0;

    SetLastError(0xdeadbeef);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list.\n");
    ok(!GetLastError(), "Got unexpected error %#lx.\n", GetLastError());

    todo_wine ok(*coinst_callback_count == 1, "Got %d callbacks.\n", *coinst_callback_count);
    todo_wine ok(*coinst_last_message == DIF_DESTROYPRIVATEDATA, "Got unexpected message %#x.\n", *coinst_last_message);
    *coinst_callback_count = 0;

    /* Test returning an error from the co-installer. */

    strcpy(regdata, "winetest_coinst.dll,co_error");
    regdata[strlen(regdata) + 1] = 0;
    res = RegSetValueExA(coinst_key, "{6a55b5a4-3f65-11db-b704-0011955c2bdb}", 0,
            REG_MULTI_SZ, (BYTE *)regdata, strlen(regdata) + 2);
    ok(!res, "Failed to set registry value, error %lu.\n", res);

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    ret = SetupDiCallClassInstaller(DIF_ALLOW_INSTALL, set, &device);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == 0xdeadc0de, "Got unexpected error %#lx.\n", GetLastError());

    ok(!device_is_registered(set, &device), "Expected device not to be registered.\n");
    ret = SetupDiCallClassInstaller(DIF_REGISTERDEVICE, set, &device);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == 0xdeadc0de, "Got unexpected error %#lx.\n", GetLastError());
    ok(!device_is_registered(set, &device), "Expected device not to be registered.\n");

    SetLastError(0xdeadbeef);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list.\n");
    ok(!GetLastError(), "Got unexpected error %#lx.\n", GetLastError());

    /* The default entry point is CoDeviceInstall(). */

    strcpy(regdata, "winetest_coinst.dll");
    regdata[strlen(regdata) + 1] = 0;
    res = RegSetValueExA(coinst_key, "{6a55b5a4-3f65-11db-b704-0011955c2bdb}", 0,
            REG_MULTI_SZ, (BYTE *)regdata, strlen(regdata) + 2);
    ok(!res, "Failed to set registry value, error %lu.\n", res);

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    ret = SetupDiCallClassInstaller(DIF_ALLOW_INSTALL, set, &device);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_DI_DO_DEFAULT, "Got unexpected error %#lx.\n", GetLastError());

    ok(*coinst_callback_count == 1, "Got %d callbacks.\n", *coinst_callback_count);
    ok(*coinst_last_message == DIF_ALLOW_INSTALL, "Got unexpected message %#x.\n", *coinst_last_message);
    *coinst_callback_count = 0;

    SetLastError(0xdeadbeef);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list.\n");
    ok(!GetLastError(), "Got unexpected error %#lx.\n", GetLastError());

    ok(*coinst_callback_count == 1, "Got %d callbacks.\n", *coinst_callback_count);
    ok(*coinst_last_message == DIF_DESTROYPRIVATEDATA, "Got unexpected message %#x.\n", *coinst_last_message);
    *coinst_callback_count = 0;

    res = RegDeleteValueA(coinst_key, "{6a55b5a4-3f65-11db-b704-0011955c2bdb}");
    ok(!res, "Failed to delete value, error %lu.\n", res);
    RegCloseKey(coinst_key);
}

static void test_call_class_installer(void)
{
    SP_DEVINFO_DATA device = {sizeof(device)};
    HMODULE coinst;
    HDEVINFO set;
    BOOL ret;

    if (wow64)
    {
        skip("SetupDiCallClassInstaller() does not work on WoW64.\n");
        return;
    }
#ifdef __REACTOS__
    if (GetNTVersion() <= _WIN32_WINNT_WS03) {
        skip("Skipping test_call_class_installer on WS03.\n");
        return;
    }
#endif

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    ok(!device_is_registered(set, &device), "Expected device not to be registered.\n");
    ret = SetupDiCallClassInstaller(DIF_REGISTERDEVICE, set, &device);
    ok(ret, "Failed to call class installer, error %#lx.\n", GetLastError());
    ok(device_is_registered(set, &device), "Expected device to be registered.\n");

    /* This is probably not failure per se, but rather an indication that no
     * class installer was called and no default handler exists. */
    ret = SetupDiCallClassInstaller(DIF_ALLOW_INSTALL, set, &device);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_DI_DO_DEFAULT, "Got unexpected error %#lx.\n", GetLastError());

    ret = SetupDiCallClassInstaller(0xdeadbeef, set, &device);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_DI_DO_DEFAULT, "Got unexpected error %#lx.\n", GetLastError());

    ret = SetupDiCallClassInstaller(DIF_REMOVE, set, &device);
    ok(ret, "Failed to call class installer, error %#lx.\n", GetLastError());
    ok(!device_is_registered(set, &device), "Expected device not to be registered.\n");

    SetLastError(0xdeadbeef);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list.\n");
    ok(!GetLastError(), "Got unexpected error %#lx.\n", GetLastError());

    load_resource("coinst.dll", "C:\\windows\\system32\\winetest_coinst.dll");

    coinst = LoadLibraryA("winetest_coinst.dll");
    coinst_callback_count = (void *)GetProcAddress(coinst, "callback_count");
    coinst_last_message = (void *)GetProcAddress(coinst, "last_message");

    test_class_installer();
    test_class_coinstaller();

    FreeLibrary(coinst);

    ret = DeleteFileA("C:\\windows\\system32\\winetest_coinst.dll");
    ok(ret, "Failed to delete file, error %lu.\n", GetLastError());
}

static void check_all_devices_enumerated_(int line, HDEVINFO set, BOOL expect_dev3)
{
    SP_DEVINFO_DATA device = {sizeof(device)};
    BOOL ret, found_dev1 = 0, found_dev2 = 0, found_dev3 = 0;
    char id[50];
    DWORD i;

    for (i = 0; SetupDiEnumDeviceInfo(set, i, &device); ++i)
    {
        ret = SetupDiGetDeviceInstanceIdA(set, &device, id, sizeof(id), NULL);
        if (!ret) continue;

        if (!strcasecmp(id, "Root\\LEGACY_BOGUS\\foo"))
        {
            found_dev1 = 1;
            ok_(__FILE__, line)(IsEqualGUID(&device.ClassGuid, &guid),
                    "Got unexpected class %s.\n", wine_dbgstr_guid(&device.ClassGuid));
        }
        else if (!strcasecmp(id, "Root\\LEGACY_BOGUS\\qux"))
        {
            found_dev2 = 1;
            ok_(__FILE__, line)(IsEqualGUID(&device.ClassGuid, &guid),
                    "Got unexpected class %s.\n", wine_dbgstr_guid(&device.ClassGuid));
        }
        else if (!strcasecmp(id, "Root\\LEGACY_BOGUS\\bar"))
        {
            found_dev3 = 1;
            ok_(__FILE__, line)(IsEqualGUID(&device.ClassGuid, &guid2),
                    "Got unexpected class %s.\n", wine_dbgstr_guid(&device.ClassGuid));
        }
    }
    ok_(__FILE__, line)(found_dev1, "Expected device 1 to be enumerated.\n");
    ok_(__FILE__, line)(found_dev2, "Expected device 2 to be enumerated.\n");
    ok_(__FILE__, line)(found_dev3 == expect_dev3, "Expected device 2 %sto be enumerated.\n",
            expect_dev3 ? "" : "not ");
}
#define check_all_devices_enumerated(a,b) check_all_devices_enumerated_(__LINE__,a,b)

static void check_device_list_(int line, HDEVINFO set, const GUID *expect)
{
    SP_DEVINFO_LIST_DETAIL_DATA_A detail = {sizeof(detail)};
    BOOL ret = SetupDiGetDeviceInfoListDetailA(set, &detail);
    ok_(__FILE__, line)(ret, "Failed to get list detail, error %#lx.\n", GetLastError());
    ok_(__FILE__, line)(IsEqualGUID(&detail.ClassGuid, expect), "Expected class %s, got %s\n",
            wine_dbgstr_guid(expect), wine_dbgstr_guid(&detail.ClassGuid));
}
#define check_device_list(a,b) check_device_list_(__LINE__,a,b)

static void test_get_class_devs(void)
{
    SP_DEVICE_INTERFACE_DATA iface = {sizeof(iface)};
    SP_DEVINFO_DATA device = {sizeof(device)};
    HDEVINFO set;
    BOOL ret;

    set = SetupDiCreateDeviceInfoList(NULL, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\foo", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());
    ret = SetupDiCreateDeviceInterfaceA(set, &device, &iface_guid, NULL, 0, &iface);
    ok(ret, "Failed to create interface, error %#lx.\n", GetLastError());
    ret = SetupDiCreateDeviceInterfaceA(set, &device, &iface_guid2, NULL, 0, &iface);
    ok(ret, "Failed to create interface, error %#lx.\n", GetLastError());
    ret = SetupDiRegisterDeviceInfo(set, &device, 0, NULL, NULL, NULL);
    ok(ret, "Failed to register device, error %#lx.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\qux", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());
    ret = SetupDiCreateDeviceInterfaceA(set, &device, &iface_guid, NULL, 0, &iface);
    ok(ret, "Failed to create interface, error %#lx.\n", GetLastError());
    ret = SetupDiRegisterDeviceInfo(set, &device, 0, NULL, NULL, NULL);
    ok(ret, "Failed to register device, error %#lx.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\bar", &guid2, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());
    ret = SetupDiRegisterDeviceInfo(set, &device, 0, NULL, NULL, NULL);
    ok(ret, "Failed to register device, error %#lx.\n", GetLastError());

    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    set = SetupDiGetClassDevsA(NULL, NULL, NULL, 0);
    ok(set == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    set = SetupDiGetClassDevsA(NULL, NULL, NULL, DIGCF_ALLCLASSES);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    check_device_list(set, &GUID_NULL);
    check_all_devices_enumerated(set, TRUE);
    check_device_iface(set, NULL, &iface_guid, 0, 0, NULL);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list, error %#lx.\n", GetLastError());

    set = SetupDiGetClassDevsA(&guid, NULL, NULL, 0);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    check_device_list(set, &guid);
    check_device_info(set, 0, &guid, "ROOT\\LEGACY_BOGUS\\FOO");
    check_device_info(set, 1, &guid, "ROOT\\LEGACY_BOGUS\\QUX");
    check_device_info(set, 2, NULL, NULL);
    check_device_iface(set, NULL, &iface_guid, 0, 0, NULL);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list, error %#lx.\n", GetLastError());

    set = SetupDiGetClassDevsA(&guid, NULL, NULL, DIGCF_ALLCLASSES);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    check_device_list(set, &GUID_NULL);
    check_all_devices_enumerated(set, TRUE);
    check_device_iface(set, NULL, &iface_guid, 0, 0, NULL);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    set = SetupDiGetClassDevsA(NULL, "ROOT", NULL, 0);
    ok(set == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    set = SetupDiGetClassDevsA(NULL, "ROOT", NULL, DIGCF_ALLCLASSES);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    check_device_list(set, &GUID_NULL);
    check_all_devices_enumerated(set, TRUE);
    check_device_iface(set, NULL, &iface_guid, 0, 0, NULL);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list, error %#lx.\n", GetLastError());

    set = SetupDiGetClassDevsA(NULL, "ROOT\\LEGACY_BOGUS", NULL, DIGCF_ALLCLASSES);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    check_device_list(set, &GUID_NULL);
    check_device_info(set, 0, &guid2, "ROOT\\LEGACY_BOGUS\\BAR");
    check_device_info(set, 1, &guid, "ROOT\\LEGACY_BOGUS\\FOO");
    check_device_info(set, 2, &guid, "ROOT\\LEGACY_BOGUS\\QUX");
    check_device_info(set, 3, NULL, NULL);
    check_device_iface(set, NULL, &iface_guid, 0, 0, NULL);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list, error %#lx.\n", GetLastError());

    set = SetupDiGetClassDevsA(&guid, "ROOT\\LEGACY_BOGUS", NULL, 0);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    check_device_list(set, &guid);
    check_device_info(set, 0, &guid, "ROOT\\LEGACY_BOGUS\\FOO");
    check_device_info(set, 1, &guid, "ROOT\\LEGACY_BOGUS\\QUX");
    check_device_info(set, 2, NULL, NULL);
    check_device_iface(set, NULL, &iface_guid, 0, 0, NULL);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list, error %#lx.\n", GetLastError());

    set = SetupDiGetClassDevsA(&guid, "ROOT\\LEGACY_BOGUS", NULL, DIGCF_ALLCLASSES);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    check_device_list(set, &GUID_NULL);
    check_device_info(set, 0, &guid2, "ROOT\\LEGACY_BOGUS\\BAR");
    check_device_info(set, 1, &guid, "ROOT\\LEGACY_BOGUS\\FOO");
    check_device_info(set, 2, &guid, "ROOT\\LEGACY_BOGUS\\QUX");
    check_device_info(set, 3, NULL, NULL);
    check_device_iface(set, NULL, &iface_guid, 0, 0, NULL);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list, error %#lx.\n", GetLastError());

    /* test DIGCF_DEVICE_INTERFACE */

    SetLastError(0xdeadbeef);
    set = SetupDiGetClassDevsA(NULL, NULL, NULL, DIGCF_DEVICEINTERFACE);
    ok(set == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    set = SetupDiGetClassDevsA(NULL, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_ALLCLASSES);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    check_device_list(set, &GUID_NULL);
    check_all_devices_enumerated(set, FALSE);
    check_device_iface(set, NULL, &iface_guid, 0, 0, "\\\\?\\root#legacy_bogus#foo#{deadbeef-3f65-11db-b704-0011955c2bdb}");
    check_device_iface(set, NULL, &iface_guid, 1, 0, "\\\\?\\root#legacy_bogus#qux#{deadbeef-3f65-11db-b704-0011955c2bdb}");
    check_device_iface(set, NULL, &iface_guid, 2, 0, NULL);
    check_device_iface(set, NULL, &iface_guid2, 0, 0, "\\\\?\\root#legacy_bogus#foo#{deadf00d-3f65-11db-b704-0011955c2bdb}");
    check_device_iface(set, NULL, &iface_guid2, 1, 0, NULL);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list, error %#lx.\n", GetLastError());

    set = SetupDiGetClassDevsA(&guid, NULL, NULL, DIGCF_DEVICEINTERFACE);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    check_device_list(set, &GUID_NULL);
    check_device_info(set, 0, NULL, NULL);
    check_device_iface(set, NULL, &iface_guid, 0, 0, NULL);
    check_device_iface(set, NULL, &iface_guid2, 0, 0, NULL);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list, error %#lx.\n", GetLastError());

    set = SetupDiGetClassDevsA(&iface_guid, NULL, NULL, DIGCF_DEVICEINTERFACE);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    check_device_list(set, &GUID_NULL);
    check_device_info(set, 0, &guid, "ROOT\\LEGACY_BOGUS\\FOO");
    check_device_info(set, 1, &guid, "ROOT\\LEGACY_BOGUS\\QUX");
    check_device_info(set, 2, &guid, NULL);
    check_device_iface(set, NULL, &iface_guid, 0, 0, "\\\\?\\root#legacy_bogus#foo#{deadbeef-3f65-11db-b704-0011955c2bdb}");
    check_device_iface(set, NULL, &iface_guid, 1, 0, "\\\\?\\root#legacy_bogus#qux#{deadbeef-3f65-11db-b704-0011955c2bdb}");
    check_device_iface(set, NULL, &iface_guid, 2, 0, NULL);
    check_device_iface(set, NULL, &iface_guid2, 0, 0, NULL);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list, error %#lx.\n", GetLastError());

    set = SetupDiGetClassDevsA(&iface_guid, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_ALLCLASSES);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    check_device_list(set, &GUID_NULL);
    check_all_devices_enumerated(set, FALSE);
    check_device_iface(set, NULL, &iface_guid, 0, 0, "\\\\?\\root#legacy_bogus#foo#{deadbeef-3f65-11db-b704-0011955c2bdb}");
    check_device_iface(set, NULL, &iface_guid, 1, 0, "\\\\?\\root#legacy_bogus#qux#{deadbeef-3f65-11db-b704-0011955c2bdb}");
    check_device_iface(set, NULL, &iface_guid, 2, 0, NULL);
    check_device_iface(set, NULL, &iface_guid2, 0, 0, "\\\\?\\root#legacy_bogus#foo#{deadf00d-3f65-11db-b704-0011955c2bdb}");
    check_device_iface(set, NULL, &iface_guid2, 1, 0, NULL);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    set = SetupDiGetClassDevsA(NULL, "ROOT", NULL, DIGCF_DEVICEINTERFACE);
    ok(set == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    set = SetupDiGetClassDevsA(NULL, "ROOT", NULL, DIGCF_DEVICEINTERFACE | DIGCF_ALLCLASSES);
todo_wine {
    ok(set == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "Got unexpected error %#lx.\n", GetLastError());
}

    SetLastError(0xdeadbeef);
    set = SetupDiGetClassDevsA(NULL, "ROOT\\LEGACY_BOGUS", NULL, DIGCF_DEVICEINTERFACE | DIGCF_ALLCLASSES);
todo_wine {
    ok(set == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "Got unexpected error %#lx.\n", GetLastError());
}

    set = SetupDiGetClassDevsA(NULL, "ROOT\\LEGACY_BOGUS\\foo", NULL, DIGCF_DEVICEINTERFACE | DIGCF_ALLCLASSES);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    check_device_list(set, &GUID_NULL);
    check_device_info(set, 0, &guid, "ROOT\\LEGACY_BOGUS\\FOO");
    check_device_info(set, 1, NULL, NULL);
    check_device_iface(set, NULL, &iface_guid, 0, 0, "\\\\?\\root#legacy_bogus#foo#{deadbeef-3f65-11db-b704-0011955c2bdb}");
    check_device_iface(set, NULL, &iface_guid, 1, 0, NULL);
    check_device_iface(set, NULL, &iface_guid2, 0, 0, "\\\\?\\root#legacy_bogus#foo#{deadf00d-3f65-11db-b704-0011955c2bdb}");
    check_device_iface(set, NULL, &iface_guid2, 1, 0, NULL);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list, error %#lx.\n", GetLastError());

    set = SetupDiGetClassDevsA(NULL, "ROOT\\LEGACY_BOGUS\\bar", NULL, DIGCF_DEVICEINTERFACE | DIGCF_ALLCLASSES);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    check_device_list(set, &GUID_NULL);
    check_device_info(set, 0, NULL, NULL);
    check_device_iface(set, NULL, &iface_guid, 0, 0, NULL);
    check_device_iface(set, NULL, &iface_guid2, 0, 0, NULL);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    set = SetupDiGetClassDevsA(&iface_guid, "ROOT\\LEGACY_BOGUS", NULL, DIGCF_DEVICEINTERFACE);
todo_wine {
    ok(set == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "Got unexpected error %#lx.\n", GetLastError());
}

    SetLastError(0xdeadbeef);
    set = SetupDiGetClassDevsA(&iface_guid, "ROOT\\LEGACY_BOGUS", NULL, DIGCF_DEVICEINTERFACE | DIGCF_ALLCLASSES);
todo_wine {
    ok(set == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "Got unexpected error %#lx.\n", GetLastError());
}

    set = SetupDiGetClassDevsA(&iface_guid, "ROOT\\LEGACY_BOGUS\\foo", NULL, DIGCF_DEVICEINTERFACE);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    check_device_list(set, &GUID_NULL);
    check_device_info(set, 0, &guid, "ROOT\\LEGACY_BOGUS\\FOO");
    check_device_info(set, 1, NULL, NULL);
    check_device_iface(set, NULL, &iface_guid, 0, 0, "\\\\?\\root#legacy_bogus#foo#{deadbeef-3f65-11db-b704-0011955c2bdb}");
    check_device_iface(set, NULL, &iface_guid, 1, 0, NULL);
    check_device_iface(set, NULL, &iface_guid2, 0, 0, NULL);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list, error %#lx.\n", GetLastError());

    set = SetupDiGetClassDevsA(&iface_guid, "ROOT\\LEGACY_BOGUS\\foo", NULL, DIGCF_DEVICEINTERFACE | DIGCF_ALLCLASSES);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    check_device_list(set, &GUID_NULL);
    check_device_info(set, 0, &guid, "ROOT\\LEGACY_BOGUS\\FOO");
    check_device_info(set, 1, NULL, NULL);
    check_device_iface(set, NULL, &iface_guid, 0, 0, "\\\\?\\root#legacy_bogus#foo#{deadbeef-3f65-11db-b704-0011955c2bdb}");
    check_device_iface(set, NULL, &iface_guid, 1, 0, NULL);
    check_device_iface(set, NULL, &iface_guid2, 0, 0, "\\\\?\\root#legacy_bogus#foo#{deadf00d-3f65-11db-b704-0011955c2bdb}");
    check_device_iface(set, NULL, &iface_guid2, 1, 0, NULL);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list, error %#lx.\n", GetLastError());

    set = SetupDiGetClassDevsA(&guid, NULL, NULL, 0);
    SetupDiEnumDeviceInfo(set, 0, &device);
    SetupDiRemoveDevice(set, &device);
    SetupDiEnumDeviceInfo(set, 1, &device);
    SetupDiRemoveDevice(set, &device);
    SetupDiDestroyDeviceInfoList(set);
    set = SetupDiGetClassDevsA(&guid2, NULL, NULL, 0);
    SetupDiEnumDeviceInfo(set, 0, &device);
    SetupDiRemoveDevice(set, &device);
    SetupDiDestroyDeviceInfoList(set);
}

static BOOL file_exists(const char *path)
{
    return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
}

static BOOL is_in_inf_dir(const char *path)
{
    char expect[MAX_PATH];

    GetWindowsDirectoryA(expect, sizeof(expect));
    strcat(expect, "\\inf\\");
    return !strncasecmp(path, expect, strrchr(path, '\\') - path);
}

static void check_original_file_name(const char *dest_inf, const char *src_inf, const char *src_catalog)
{
    SP_ORIGINAL_FILE_INFO_A orig_info;
    SP_INF_INFORMATION *inf_info;
    DWORD size;
    HINF hinf;
    BOOL res;

    if (!pSetupQueryInfOriginalFileInformationA)
    {
        win_skip("SetupQueryInfOriginalFileInformationA is not available\n");
        return;
    }

    hinf = SetupOpenInfFileA(dest_inf, NULL, INF_STYLE_WIN4, NULL);
    ok(hinf != NULL, "Failed to open INF file, error %lu.\n", GetLastError());

    res = SetupGetInfInformationA(hinf, INFINFO_INF_SPEC_IS_HINF, NULL, 0, &size);
    ok(res, "Failed to get INF information, error %lu.\n", GetLastError());

    inf_info = malloc(size);

    res = SetupGetInfInformationA(hinf, INFINFO_INF_SPEC_IS_HINF, inf_info, size, NULL);
    ok(res, "Failed to get INF information, error %lu.\n", GetLastError());

    orig_info.cbSize = 0;
    SetLastError(0xdeadbeef);
    res = pSetupQueryInfOriginalFileInformationA(inf_info, 0, NULL, &orig_info);
    ok(!res, "Got %d.\n", res);
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "Got error %#lx.\n", GetLastError());

    orig_info.cbSize = sizeof(orig_info);
    SetLastError(0xdeadbeef);
    res = pSetupQueryInfOriginalFileInformationA(inf_info, 0, NULL, &orig_info);
    ok(res == TRUE, "Got %d.\n", res);
    ok(!GetLastError(), "Got error %#lx.\n", GetLastError());
    ok(!strcmp(orig_info.OriginalCatalogName, src_catalog), "Expected original catalog name %s, got %s.\n",
            debugstr_a(src_catalog), debugstr_a(orig_info.OriginalCatalogName));
    ok(!strcmp(orig_info.OriginalInfName, src_inf), "Expected orignal inf name %s, got %s.\n",
            debugstr_a(src_inf), debugstr_a(orig_info.OriginalInfName));

    free(inf_info);

    SetupCloseInfFile(hinf);
}

static void test_copy_oem_inf(struct testsign_context *ctx)
{
    char path[MAX_PATH * 2], dest[MAX_PATH], orig_dest[MAX_PATH], orig_store[MAX_PATH];
    char orig_cwd[MAX_PATH], *cwd, *filepart, pnf[MAX_PATH];
    SYSTEM_INFO system_info;
    HANDLE catalog;
    DWORD size;
    BOOL ret;

    static const char inf_data1[] =
        "[Version]\n"
        "Signature=\"$Chicago$\"\n"
        "CatalogFile=winetest.cat\n"
        /* Windows 10 needs a non-empty Manufacturer section, otherwise
         * SetupUninstallOEMInf() fails with ERROR_INVALID_PARAMETER. */
        "[Manufacturer]\n"
        "mfg1=mfg_section,NT" MYEXT "\n"
        "; This is a WINE test INF file\n";

    static const char inf_data2[] =
        "[Version]\n"
        "Signature=\"$Chicago$\"\n"
        "CatalogFile=winetest2.cat\n"
        "[Manufacturer]\n"
        "mfg1=mfg_section,NT" MYEXT "\n"
        "; This is another WINE test INF file\n";

    if (wow64)
        return;

    GetSystemInfo(&system_info);

    GetCurrentDirectoryA(sizeof(orig_cwd), orig_cwd);
    cwd = tempnam(NULL, "wine");
    ret = CreateDirectoryA(cwd, NULL);
    ok(ret, "Failed to create %s, error %#lx.\n", debugstr_a(cwd), GetLastError());
    ret = SetCurrentDirectoryA(cwd);
    ok(ret, "Failed to cd to %s, error %#lx.\n", debugstr_a(cwd), GetLastError());

    /* try NULL SourceInfFileName */
    SetLastError(0xdeadbeef);
    ret = SetupCopyOEMInfA(NULL, NULL, 0, SP_COPY_NOOVERWRITE, NULL, 0, NULL, NULL);
    ok(!ret, "Got %d.\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got error %#lx.\n", GetLastError());

    /* try empty SourceInfFileName */
    SetLastError(0xdeadbeef);
    ret = SetupCopyOEMInfA("", NULL, 0, SP_COPY_NOOVERWRITE, NULL, 0, NULL, NULL);
    ok(!ret, "Got %d.\n", ret);
    ok(GetLastError() == ERROR_FILE_NOT_FOUND
            || GetLastError() == ERROR_INVALID_PARAMETER /* vista, 2k8 */, "Got error %#lx.\n", GetLastError());

    /* try a relative nonexistent SourceInfFileName */
    SetLastError(0xdeadbeef);
    ret = SetupCopyOEMInfA("nonexistent", NULL, 0, SP_COPY_NOOVERWRITE, NULL, 0, NULL, NULL);
    ok(!ret, "Got %d.\n", ret);
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "Got error %#lx.\n", GetLastError());

    /* try an absolute nonexistent SourceInfFileName */
    GetCurrentDirectoryA(sizeof(path), path);
    strcat(path, "\\nonexistent");
    SetLastError(0xdeadbeef);
    ret = SetupCopyOEMInfA(path, NULL, 0, SP_COPY_NOOVERWRITE, NULL, 0, NULL, NULL);
    ok(!ret, "Got %d.\n", ret);
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "Got error %#lx.\n", GetLastError());

    create_file("winetest.inf", inf_data1);

    catalog = CryptCATOpen((WCHAR *)L"winetest.cat", CRYPTCAT_OPEN_CREATENEW, 0, CRYPTCAT_VERSION_1, 0);
    ok(catalog != INVALID_HANDLE_VALUE, "Failed to create catalog, error %#lx\n", GetLastError());

    add_file_to_catalog(catalog, L"winetest.inf");

    ret = CryptCATPersistStore(catalog);
    todo_wine ok(ret, "Failed to write catalog, error %#lx\n", GetLastError());

    ret = CryptCATClose(catalog);
    ok(ret, "Failed to close catalog, error %#lx\n", GetLastError());

    testsign_sign(ctx, L"winetest.cat");

    size = ARRAY_SIZE(dest);
    memset(dest, 0xcc, sizeof(dest));
    ret = pDriverStoreFindDriverPackageA("winetest.inf", 0, 0, system_info.wProcessorArchitecture, 0, dest, &size);
    ok(ret == HRESULT_FROM_WIN32(ERROR_NOT_FOUND), "Got %#x.\n", ret);
    ok(!dest[0], "Got %s.\n", debugstr_a(dest));
    todo_wine ok(!size, "Got size %lu.\n", size);

    /* Test with a relative path. */
    SetLastError(0xdeadbeef);
    memset(dest, 0xcc, sizeof(dest));
    ret = SetupCopyOEMInfA("winetest.inf", NULL, 0, SP_COPY_NOOVERWRITE, dest, sizeof(dest), NULL, &filepart);
    ok(ret == TRUE, "Got %d.\n", ret);
    ok(!GetLastError(), "Got error %#lx.\n", GetLastError());
    ok(file_exists("winetest.inf"), "Expected source inf to exist.\n");
    ok(file_exists(dest), "Expected dest file to exist.\n");
    ok(is_in_inf_dir(dest), "Got unexpected path '%s'.\n", dest);
    ok(filepart == strrchr(dest, '\\') + 1, "Got unexpected file part %s.\n", filepart);

    ret = SetupUninstallOEMInfA("bogus.inf", 0, NULL);
    ok(!ret, "Got %d.\n", ret);
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "Got error %#lx.\n", GetLastError());

    strcpy(pnf, dest);
    *(strrchr(pnf, '.') + 1) = 'p';
    SetLastError(0xdeadbeef);
    ret = SetupUninstallOEMInfA(filepart, 0, NULL);
    ok(ret == TRUE, "Got %d.\n", ret);
    ok(!GetLastError(), "Got error %#lx.\n", GetLastError());
    ok(!file_exists(dest), "Expected inf '%s' not to exist.\n", dest);
    DeleteFileA(dest);
    ok(!file_exists(pnf), "Expected pnf '%s' not to exist.\n", pnf);

    /* try SP_COPY_REPLACEONLY, dest does not exist */
    SetLastError(0xdeadbeef);
    ret = SetupCopyOEMInfA(path, NULL, SPOST_NONE, SP_COPY_REPLACEONLY, NULL, 0, NULL, NULL);
    ok(!ret, "Got %d.\n", ret);
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "Got error %#lx.\n", GetLastError());
    ok(file_exists("winetest.inf"), "Expected source inf to exist.\n");

    /* Test a successful call. */
    GetCurrentDirectoryA(sizeof(path), path);
    strcat(path, "\\winetest.inf");

    size = ARRAY_SIZE(dest);
    memset(dest, 0xcc, sizeof(dest));
    ret = pDriverStoreFindDriverPackageA(path, 0, 0, system_info.wProcessorArchitecture, 0, dest, &size);
    ok(ret == HRESULT_FROM_WIN32(ERROR_NOT_FOUND), "Got %#x.\n", ret);
    ok(!dest[0], "Got %s.\n", debugstr_a(dest));
    todo_wine ok(!size, "Got size %lu.\n", size);

    SetLastError(0xdeadbeef);
    ret = SetupCopyOEMInfA(path, NULL, SPOST_NONE, 0, dest, sizeof(dest), NULL, NULL);
    ok(ret == TRUE, "Got %d.\n", ret);
    ok(!GetLastError(), "Got error %#lx.\n", GetLastError());
    ok(file_exists(path), "Expected source inf to exist.\n");
    ok(file_exists(dest), "Expected dest file to exist.\n");
    ok(is_in_inf_dir(dest), "Got unexpected path '%s'.\n", dest);
    strcpy(orig_dest, dest);

    check_original_file_name(dest, "winetest.inf", "winetest.cat");

    size = ARRAY_SIZE(dest);
    memset(dest, 0xcc, sizeof(dest));
    ret = pDriverStoreFindDriverPackageA(path, 0, 0, system_info.wProcessorArchitecture, 0, dest, &size);
    ok(!ret, "Got %#x.\n", ret);
    strcpy(orig_store, dest);

    SetLastError(0xdeadbeef);
    ret = SetupCopyOEMInfA(path, NULL, SPOST_NONE, 0, dest, sizeof(dest), NULL, NULL);
    ok(ret == TRUE, "Got %d.\n", ret);
    ok(!GetLastError(), "Got error %#lx.\n", GetLastError());
    ok(file_exists(path), "Expected source inf to exist.\n");
    ok(file_exists(dest), "Expected dest file to exist.\n");
    ok(!strcmp(orig_dest, dest), "Expected '%s', got '%s'.\n", orig_dest, dest);

    /* On Windows 7 and earlier, trying to install the same file with a
     * different base name does nothing and returns the existing driver store
     * location and INF directory file.
     * On Windows 8 and later, it's installed to a new location. */

    /* try SP_COPY_REPLACEONLY, dest exists */
    GetCurrentDirectoryA(sizeof(path), path);
    strcat(path, "\\winetest.inf");
    SetLastError(0xdeadbeef);
    ret = SetupCopyOEMInfA(path, NULL, SPOST_NONE, SP_COPY_REPLACEONLY, dest, sizeof(dest), NULL, NULL);
    ok(ret == TRUE, "Got %d.\n", ret);
    ok(!GetLastError(), "Got error %#lx.\n", GetLastError());
    ok(file_exists(path), "Expected source inf to exist.\n");
    ok(file_exists(dest), "Expected dest file to exist.\n");
    ok(!strcmp(orig_dest, dest), "Expected '%s', got '%s'.\n", orig_dest, dest);

    strcpy(dest, "aaa");
    SetLastError(0xdeadbeef);
    ret = SetupCopyOEMInfA(path, NULL, SPOST_NONE, SP_COPY_NOOVERWRITE, dest, sizeof(dest), NULL, NULL);
    ok(!ret, "Got %d.\n", ret);
    ok(GetLastError() == ERROR_FILE_EXISTS, "Got error %#lx.\n", GetLastError());
    ok(!strcmp(orig_dest, dest), "Expected '%s', got '%s'.\n", orig_dest, dest);

    SetLastError(0xdeadbeef);
    ret = SetupCopyOEMInfA(path, NULL, SPOST_NONE, 0, NULL, 0, NULL, NULL);
    ok(ret == TRUE, "Got %d.\n", ret);
    ok(!GetLastError(), "Got error %#lx.\n", GetLastError());
    ok(file_exists(path), "Expected source inf to exist.\n");
    ok(file_exists(orig_dest), "Expected dest file to exist.\n");

    strcpy(dest, "aaa");
    size = 0;
    SetLastError(0xdeadbeef);
    ret = SetupCopyOEMInfA(path, NULL, SPOST_NONE, 0, dest, 5, &size, NULL);
    ok(!ret, "Got %d.\n", ret);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Got error %#lx.\n", GetLastError());
    ok(file_exists(path), "Expected source inf to exist.\n");
    ok(file_exists(orig_dest), "Expected dest inf to exist.\n");
    ok(!strcmp(dest, "aaa"), "Expected dest to be unchanged\n");
    ok(size == strlen(orig_dest) + 1, "Got %ld.\n", size);

    SetLastError(0xdeadbeef);
    ret = SetupCopyOEMInfA(path, NULL, SPOST_NONE, 0, dest, sizeof(dest), &size, NULL);
    ok(ret == TRUE, "Got %d.\n", ret);
    ok(!GetLastError(), "Got error %#lx.\n", GetLastError());
    ok(!strcmp(orig_dest, dest), "Expected '%s', got '%s'.\n", orig_dest, dest);
    ok(size == strlen(dest) + 1, "Got %ld.\n", size);

    SetLastError(0xdeadbeef);
    ret = SetupCopyOEMInfA(path, NULL, SPOST_NONE, 0, dest, sizeof(dest), NULL, &filepart);
    ok(ret == TRUE, "Got %d.\n", ret);
    ok(!GetLastError(), "Got error %#lx.\n", GetLastError());
    ok(!strcmp(orig_dest, dest), "Expected '%s', got '%s'.\n", orig_dest, dest);
    ok(filepart == strrchr(dest, '\\') + 1, "Got unexpected file part %s.\n", filepart);

    SetLastError(0xdeadbeef);
    ret = SetupCopyOEMInfA(path, NULL, SPOST_NONE, SP_COPY_DELETESOURCE, NULL, 0, NULL, NULL);
    ok(ret == TRUE, "Got %d.\n", ret);
    ok(!GetLastError(), "Got error %#lx.\n", GetLastError());
    ok(!file_exists(path), "Expected source inf not to exist.\n");

    strcpy(pnf, dest);
    *(strrchr(pnf, '.') + 1) = 'p';

    ret = SetupUninstallOEMInfA(strrchr(dest, '\\') + 1, 0, NULL);
    ok(ret, "Failed to uninstall '%s', error %#lx.\n", dest, GetLastError());
    ok(!file_exists(dest), "Expected inf '%s' not to exist.\n", dest);
    DeleteFileA(dest);
    ok(!file_exists(pnf), "Expected pnf '%s' not to exist.\n", pnf);

    size = ARRAY_SIZE(dest);
    memset(dest, 0xcc, sizeof(dest));
    ret = pDriverStoreFindDriverPackageA(path, 0, 0, system_info.wProcessorArchitecture, 0, dest, &size);
    ok(ret == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "Got %#x.\n", ret);
    ok(!dest[0], "Got %s.\n", debugstr_a(dest));
    todo_wine ok(!size, "Got size %lu.\n", size);

    create_file("winetest.inf", inf_data1);
    SetLastError(0xdeadbeef);
    ret = SetupCopyOEMInfA(path, NULL, SPOST_NONE, 0, dest, sizeof(dest), NULL, NULL);
    ok(ret == TRUE, "Got %d.\n", ret);
    ok(!GetLastError(), "Got error %#lx.\n", GetLastError());
    ok(is_in_inf_dir(dest), "Got unexpected path '%s'.\n", dest);
    strcpy(orig_dest, dest);

    create_file("winetest2.inf", inf_data2);

    catalog = CryptCATOpen((WCHAR *)L"winetest2.cat", CRYPTCAT_OPEN_CREATENEW, 0, CRYPTCAT_VERSION_1, 0);
    ok(catalog != INVALID_HANDLE_VALUE, "Failed to create catalog, error %#lx\n", GetLastError());

    add_file_to_catalog(catalog, L"winetest2.inf");

    ret = CryptCATPersistStore(catalog);
    todo_wine ok(ret, "Failed to write catalog, error %#lx\n", GetLastError());

    ret = CryptCATClose(catalog);
    ok(ret, "Failed to close catalog, error %#lx\n", GetLastError());

    testsign_sign(ctx, L"winetest2.cat");

    SetLastError(0xdeadbeef);
    GetCurrentDirectoryA(sizeof(path), path);
    strcat(path, "\\winetest2.inf");
    ret = SetupCopyOEMInfA(path, NULL, SPOST_NONE, 0, dest, sizeof(dest), NULL, NULL);
    ok(ret == TRUE, "Got %d.\n", ret);
    ok(!GetLastError(), "Got error %#lx.\n", GetLastError());
    ok(is_in_inf_dir(dest), "Got unexpected path '%s'.\n", dest);
    ok(strcmp(dest, orig_dest), "Expected INF files to be copied to different paths.\n");

    ret = SetupUninstallOEMInfA(strrchr(dest, '\\') + 1, 0, NULL);
    ok(ret, "Failed to uninstall '%s', error %#lx.\n", dest, GetLastError());
    ok(!file_exists(dest), "Expected inf '%s' not to exist.\n", dest);
    DeleteFileA(dest);
    strcpy(pnf, dest);
    *(strrchr(pnf, '.') + 1) = 'p';
    ok(!file_exists(pnf), "Expected pnf '%s' not to exist.\n", pnf);

    ret = SetupUninstallOEMInfA(strrchr(orig_dest, '\\') + 1, 0, NULL);
    ok(ret, "Failed to uninstall '%s', error %#lx.\n", orig_dest, GetLastError());
    ok(!file_exists(orig_dest), "Expected inf '%s' not to exist.\n", dest);
    DeleteFileA(orig_dest);
    strcpy(pnf, dest);
    *(strrchr(pnf, '.') + 1) = 'p';
    ok(!file_exists(pnf), "Expected pnf '%s' not to exist.\n", pnf);

    ret = DeleteFileA("winetest2.cat");
    ok(ret, "Failed to delete file, error %#lx.\n", GetLastError());

    ret = DeleteFileA("winetest2.inf");
    ok(ret, "Failed to delete file, error %#lx.\n", GetLastError());

    ret = DeleteFileA("winetest.cat");
    ok(ret, "Failed to delete file, error %#lx.\n", GetLastError());

    ret = DeleteFileA("winetest.inf");
    ok(ret, "Failed to delete file, error %#lx.\n", GetLastError());

    SetCurrentDirectoryA(orig_cwd);
    ret = RemoveDirectoryA(cwd);
    ok(ret, "Failed to delete %s, error %#lx.\n", cwd, GetLastError());

}

static void check_driver_store_file_exists(const char *driver_store, const char *file, BOOL exists)
{
    char path[MAX_PATH];

    sprintf(path, "%s\\%s", driver_store, file);
    ok(file_exists(path) == exists, "Expected %s to %s.\n", debugstr_a(path), exists ? "exist" : "not exist");
}

static const char driver_store_hardware_id[] = "winetest_store_hardware_id\0";

static void create_driver_store_test_device(HDEVINFO set, const char *name, SP_DEVINFO_DATA *device)
{
    static const GUID guid = {0x77777777};
    BOOL ret;

    if (SetupDiOpenDeviceInfoA(set, name, NULL, 0, device))
    {
        ret = SetupDiCallClassInstaller(DIF_REMOVE, set, device);
        ok(ret, "Failed to remove device, error %#lx.\n", GetLastError());
    }

    ret = SetupDiCreateDeviceInfoA(set, name, &guid, NULL, NULL, 0, device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());
    ret = SetupDiSetDeviceRegistryPropertyA(set, device, SPDRP_HARDWAREID,
            (const BYTE *)driver_store_hardware_id, sizeof(driver_store_hardware_id));
    ok(ret, "Failed to set hardware ID, error %#lx.\n", GetLastError());
    ret = SetupDiCallClassInstaller(DIF_REGISTERDEVICE, set, device);
    ok(ret, "Failed to call class installer, error %#lx.\n", GetLastError());
}

static void test_driver_store(struct testsign_context *ctx)
{
    static const char repository_dir[] = "C:\\windows\\system32\\DriverStore\\FileRepository\\";
    SP_DEVINFO_DATA device = {sizeof(device)}, device2 = {sizeof(device2)};
    char dest[MAX_PATH], orig_dest[MAX_PATH], inf_path[MAX_PATH];
    SP_DRVINFO_DATA_A driver = {sizeof(driver)};
    char orig_cwd[MAX_PATH], *cwd;
    char driver_path[MAX_PATH];
    SYSTEM_INFO system_info;
    HANDLE catalog;
    HDEVINFO set;
    DWORD size;
    BOOL ret;

    static const char inf_data1[] =
        "[Version]\n"
        "Signature=\"$Chicago$\"\n"
        "CatalogFile=winetest.cat\n"
        "Class=Bogus\n"
        "ClassGUID={6a55b5a4-3f65-11db-b704-0011955c2bdb}\n"

        "[Manufacturer]\n"
        "mfg1=mfg_section,NT" MYEXT "\n"
        "mfg2=mfg_section_wrongarch,NT" WRONGEXT "\n"

        "[mfg_section.nt" MYEXT "]\n"
        "desc1=device_section,winetest_store_hardware_id\n"

        "[mfg_section_wrongarch.nt" WRONGEXT "]\n"
        "desc1=device_section2,winetest_store_hardware_id\n"

        "[device_section.nt" MYEXT "]\n"
        "CopyFiles=file_section\n"

        "[device_section_wrongarch.nt" WRONGEXT "]\n"
        "CopyFiles=file_section_wrongarch\n"

        "[device_section.nt" MYEXT ".CoInstallers]\n"
        "CopyFiles=coinst_file_section\n"

        "[file_section]\n"
        "winetest_dst.txt,winetest_src.txt\n"
        "winetest_child.txt\n"
        "winetest_niece.txt\n"
        "winetest_cab.txt\n"

        "[file_section_wrongarch]\n"
        "winetest_wrongarch.txt\n"

        "[coinst_file_section]\n"
        "winetest_coinst.txt\n"

        "[SourceDisksFiles]\n"
        "winetest_src.txt=1\n"
        "winetest_child.txt=1,subdir\n"
        "winetest_niece.txt=2\n"
        "winetest_ignored.txt=1\n"
        "winetest_ignored2.txt=1\n"
        "winetest_wrongarch.txt=1\n"
        "winetest_cab.txt=3\n"
        "winetest_coinst.txt=1\n"

        "[SourceDisksNames]\n"
        "1=,winetest_src.txt\n"
        "2=,winetest_niece.txt,,sister\n"
        "3=,winetest_cab.cab\n"

        "[DestinationDirs]\n"
        "DefaultDestDir=11\n"
        ;

    if (wow64)
        return;

    GetSystemInfo(&system_info);

    set = SetupDiCreateDeviceInfoList(NULL, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());

    create_driver_store_test_device(set, "Root\\winetest_store\\1", &device);

    GetCurrentDirectoryA(sizeof(orig_cwd), orig_cwd);
    cwd = tempnam(NULL, "wine");
    ret = CreateDirectoryA(cwd, NULL);
    ok(ret, "Failed to create %s, error %lu.\n", debugstr_a(cwd), GetLastError());
    ret = SetCurrentDirectoryA(cwd);
    ok(ret, "Failed to cd to %s, error %lu.\n", debugstr_a(cwd), GetLastError());

    create_file("winetest.inf", inf_data1);
    create_file("winetest_src.txt", "data1");
    create_file("winetest_unused.txt", "unused");
    create_file("winetest_ignored2.txt", "ignored2");
    create_directory("subdir");
    create_directory("sister");
    create_file("subdir\\winetest_child.txt", "child");
    create_file("sister\\winetest_niece.txt", "niece");
    create_file("winetest_cab.txt", "cab");
    create_cab_file("winetest_cab.cab", "winetest_cab.txt");
    create_file("winetest_coinst.txt", "coinst");

    /* If the catalog doesn't exist, or any files are missing from it,
     * validation fails and we get a UI dialog. */

    catalog = CryptCATOpen((WCHAR *)L"winetest.cat", CRYPTCAT_OPEN_CREATENEW, 0, CRYPTCAT_VERSION_1, 0);
    ok(catalog != INVALID_HANDLE_VALUE, "Failed to create catalog, error %#lx\n", GetLastError());

    add_file_to_catalog(catalog, L"winetest.inf");
    add_file_to_catalog(catalog, L"winetest_src.txt");
    add_file_to_catalog(catalog, L"subdir\\winetest_child.txt");
    add_file_to_catalog(catalog, L"sister\\winetest_niece.txt");
    add_file_to_catalog(catalog, L"winetest_cab.txt");
    add_file_to_catalog(catalog, L"winetest_coinst.txt");
    add_file_to_catalog(catalog, L"winetest_unused.txt");
    add_file_to_catalog(catalog, L"winetest_ignored2.txt");

    ret = CryptCATPersistStore(catalog);
    todo_wine ok(ret, "Failed to write catalog, error %lu\n", GetLastError());

    ret = CryptCATClose(catalog);
    ok(ret, "Failed to close catalog, error %lu\n", GetLastError());

    testsign_sign(ctx, L"winetest.cat");

    /* Delete one of the files referenced in the catalog, to show that files
     * present in the catalog and not used in the INF don't need to be present. */
    delete_file("winetest_unused.txt");
    /* Also delete the cab source file. */
    delete_file("winetest_cab.txt");

    size = ARRAY_SIZE(dest);
    memset(dest, 0xcc, sizeof(dest));
    ret = pDriverStoreFindDriverPackageA("winetest.inf", 0, 0, system_info.wProcessorArchitecture, 0, dest, &size);
    ok(ret == HRESULT_FROM_WIN32(ERROR_NOT_FOUND), "Got %#x.\n", ret);
    ok(!dest[0], "Got %s.\n", debugstr_a(dest));
    todo_wine ok(!size, "Got size %lu.\n", size);

    /* Windows 7 allows relative paths. Windows 8+ do not.
     * However, all versions seem to accept relative paths in
     * DriverStoreFindDriverPackage(). */

    sprintf(inf_path, "%s\\winetest.inf", cwd);

    size = ARRAY_SIZE(dest);
    memset(dest, 0xcc, sizeof(dest));
    ret = pDriverStoreAddDriverPackageA(inf_path, 0, 0, system_info.wProcessorArchitecture, dest, &size);
    ok(!ret, "Got %#x.\n", ret);
    ok(size > ARRAY_SIZE(repository_dir), "Got size %lu.\n", size);
    ok(size == strlen(dest) + 1, "Expected size %Iu, got %lu.\n", strlen(dest) + 1, size);
    ok(!memicmp(dest, repository_dir, strlen(repository_dir)), "Got path %s.\n", debugstr_a(dest));
    ok(!strcmp(dest + strlen(dest) - 13, "\\winetest.inf"), "Got path %s.\n", debugstr_a(dest));

    strcpy(orig_dest, dest);

    /* Add again. */
    size = ARRAY_SIZE(dest);
    memset(dest, 0xcc, sizeof(dest));
    ret = pDriverStoreAddDriverPackageA(inf_path, 0, 0, system_info.wProcessorArchitecture, dest, &size);
    ok(!ret, "Got %#x.\n", ret);
    ok(!strcmp(dest, orig_dest), "Expected %s, got %s.\n", debugstr_a(orig_dest), debugstr_a(dest));
    ok(size == strlen(dest) + 1, "Expected size %Iu, got %lu.\n", strlen(dest) + 1, size);

    size = ARRAY_SIZE(dest);
    memset(dest, 0xcc, sizeof(dest));
    ret = pDriverStoreFindDriverPackageA("winetest.inf", 0, 0, system_info.wProcessorArchitecture, 0, dest, &size);
    ok(!ret, "Got %#x.\n", ret);
    ok(!strcmp(dest, orig_dest), "Expected %s, got %s.\n", debugstr_a(orig_dest), debugstr_a(dest));
    ok(size == strlen(dest) + 1, "Expected size %Iu, got %lu.\n", strlen(dest) + 1, size);

    /* Test the length parameter.
     * Anything less than MAX_PATH returns E_INVALIDARG.
     * It's not clear what happens if the returned path is longer than MAX_PATH. */

    size = MAX_PATH - 1;
    memset(dest, 0xcc, sizeof(dest));
    ret = pDriverStoreFindDriverPackageA("winetest.inf", 0, 0, system_info.wProcessorArchitecture, 0, dest, &size);
    ok(ret == E_INVALIDARG, "Got %#x.\n", ret);
    ok(dest[0] == (char)0xcc, "Got %s.\n", debugstr_a(dest));
    ok(size == MAX_PATH - 1, "Expected size %Iu, got %lu.\n", strlen(orig_dest) + 1, size);

    size = 0;
    ret = pDriverStoreFindDriverPackageA("winetest.inf", 0, 0, system_info.wProcessorArchitecture, 0, dest, &size);
    ok(ret == E_INVALIDARG, "Got %#x.\n", ret);
    ok(!size, "Got size %lu.\n", size);

    /* Adding to the store also copies to the C:\windows\inf dir. */
    ret = SetupCopyOEMInfA(orig_dest, NULL, 0, SP_COPY_REPLACEONLY, NULL, 0, NULL, NULL);
    ok(ret == TRUE, "Got %#x.\n", ret);

    /* The catalog, and files referenced through a CopyFiles section,
     * are also present. */
    strcpy(dest, orig_dest);
    *strrchr(dest, '\\') = 0;
    check_driver_store_file_exists(dest, "winetest.cat", TRUE);
    check_driver_store_file_exists(dest, "winetest_src.txt", TRUE);
    check_driver_store_file_exists(dest, "subdir/winetest_child.txt", TRUE);
    check_driver_store_file_exists(dest, "sister/winetest_niece.txt", TRUE);
    check_driver_store_file_exists(dest, "winetest_cab.txt", TRUE);
    check_driver_store_file_exists(dest, "winetest_coinst.txt", TRUE);
    check_driver_store_file_exists(dest, "winetest_dst.txt", FALSE);
    check_driver_store_file_exists(dest, "winetest_ignored2.txt", FALSE);
    check_driver_store_file_exists(dest, "winetest_cab.cab", FALSE);

    /* The inf is installed to C:\windows\inf, but the driver isn't installed
     * for existing devices that match, and hence files aren't installed to
     * their final destination. */

    ret = SetupDiBuildDriverInfoList(set, &device, SPDIT_COMPATDRIVER);
    ok(ret, "Got error %#lx.\n", GetLastError());

    ret = SetupDiSelectBestCompatDrv(set, &device);
    ok(ret, "Got error %#lx.\n", GetLastError());

    ret = SetupDiGetSelectedDriverA(set, &device, &driver);
    ok(driver.DriverType == SPDIT_COMPATDRIVER, "Got wrong type %#lx.\n", driver.DriverType);
    ok(!strcmp(driver.Description, "desc1"), "Got wrong description '%s'.\n", driver.Description);
    ok(!strcmp(driver.MfgName, "mfg1"), "Got wrong manufacturer '%s'.\n", driver.MfgName);
    ok(!strcmp(driver.ProviderName, ""), "Got wrong provider '%s'.\n", driver.ProviderName);

    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_DRIVER, NULL,
            (BYTE *)driver_path, sizeof(driver_path), NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "Got unexpected error %#lx.\n", GetLastError());

    ok(!file_exists("C:\\windows\\system32\\winetest_dst.txt"), "Expected dst to not exist.\n");

    /* The apparent point of the driver store is to provide a source directory
     * for INF files that doesn't require the original install medium.
     * Hence, test that we can move the original files out of place and then
     * trigger device installation.
     *
     * (Of course, it would have been easier just to install the driver files
     * directly, even if the corresponding device doesn't exist yet, but that's
     * not the model that Microsoft chose, for some reason. */

    ret = MoveFileExA("winetest.cat", "not_winetest.cat", 0);
    ok(ret, "Got error %#lx.\n", GetLastError());
    ret = MoveFileExA("winetest_src.txt", "not_winetest_src.txt", 0);
    ok(ret, "Got error %#lx.\n", GetLastError());
    ret = MoveFileExA("winetest_cab.cab", "not_winetest_cab.cab", 0);
    ok(ret, "Got error %#lx.\n", GetLastError());
    ret = MoveFileExA("sister", "not_sister", 0);
    ok(ret, "Got error %#lx.\n", GetLastError());

    /* Also call DriverStoreFindDriverPackageA() again here, to prove that it
     * only needs to compare the inf, not any of the other files. */
    size = ARRAY_SIZE(dest);
    memset(dest, 0xcc, sizeof(dest));
    ret = pDriverStoreFindDriverPackageA("winetest.inf", 0, 0, system_info.wProcessorArchitecture, 0, dest, &size);
    ok(!ret || ret == HRESULT_FROM_WIN32(ERROR_NOT_FOUND) /* Win < 8 */, "Got %#x.\n", ret);
    if (!ret)
    {
        ok(!strcmp(dest, orig_dest), "Expected %s, got %s.\n", debugstr_a(orig_dest), debugstr_a(dest));
        ok(size == strlen(dest) + 1, "Expected size %Iu, got %lu.\n", strlen(dest) + 1, size);
    }

    ret = MoveFileExA("winetest.inf", "not_winetest.inf", 0);
    ok(ret, "Got error %#lx.\n", GetLastError());

    /* However, the inf name does need to match. */
    size = ARRAY_SIZE(dest);
    memset(dest, 0xcc, sizeof(dest));
    ret = pDriverStoreFindDriverPackageA("not_winetest.inf", 0, 0, system_info.wProcessorArchitecture, 0, dest, &size);
    ok(ret == HRESULT_FROM_WIN32(ERROR_NOT_FOUND), "Got %#x.\n", ret);
    ok(!dest[0], "Got %s.\n", debugstr_a(dest));
    todo_wine ok(!size, "Got size %lu.\n", size);

    ret = SetupDiCallClassInstaller(DIF_INSTALLDEVICEFILES, set, &device);
    ok(ret, "Got error %#lx.\n", GetLastError());

    delete_file("C:\\windows\\system32\\winetest_dst.txt");
    delete_file("C:\\windows\\system32\\winetest_child.txt");
    delete_file("C:\\windows\\system32\\winetest_niece.txt");
    delete_file("C:\\windows\\system32\\winetest_cab.txt");
    todo_wine delete_file("C:\\windows\\system32\\winetest_coinst.txt");

    ret = MoveFileExA("not_winetest.inf", "winetest.inf", 0);
    ok(ret, "Got error %#lx.\n", GetLastError());
    ret = MoveFileExA("not_winetest.cat", "winetest.cat", 0);
    ok(ret, "Got error %#lx.\n", GetLastError());
    ret = MoveFileExA("not_winetest_src.txt", "winetest_src.txt", 0);
    ok(ret, "Got error %#lx.\n", GetLastError());
    ret = MoveFileExA("not_winetest_cab.cab", "winetest_cab.cab", 0);
    ok(ret, "Got error %#lx.\n", GetLastError());
    ret = MoveFileExA("not_sister", "sister", 0);
    ok(ret, "Got error %#lx.\n", GetLastError());

    ret = SetupDiCallClassInstaller(DIF_REMOVE, set, &device);
    ok(ret, "Got error %#lx.\n", GetLastError());

    ret = pDriverStoreDeleteDriverPackageA(orig_dest, 0, 0);
    ok(!ret, "Got %#x.\n", ret);

    ret = SetupCopyOEMInfA(orig_dest, NULL, 0, SP_COPY_REPLACEONLY, NULL, 0, NULL, NULL);
    ok(!ret, "Got %#x.\n", ret);
    todo_wine ok(GetLastError() == ERROR_FILE_NOT_FOUND, "Got error %lu.\n", GetLastError());

    /* All files reachable through CopyFiles have to be present. If any are
     * missing, DriverStoreAddDriverPackage() fails with
     * HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND).
     *
     * On Windows 8, this also seems to result in an internal leak: attempting
     * to delete the source directory afterward ("cwd") fails with
     * ERROR_SHARING_VIOLATION, even though FindFirstFile() confirms that it's
     * empty. */

    delete_file("subdir\\winetest_child.txt");
    delete_file("sister\\winetest_niece.txt");
    delete_file("winetest_cab.cab");
    delete_file("winetest_coinst.txt");
    delete_file("winetest_ignored2.txt");
    delete_file("winetest_src.txt");
    delete_file("winetest.cat");
    delete_file("winetest.inf");
    delete_directory("subdir");
    delete_directory("sister");

    SetCurrentDirectoryA(orig_cwd);
    ret = RemoveDirectoryA(cwd);
    ok(ret, "Failed to delete %s, error %lu.\n", cwd, GetLastError());

    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list.\n");
}

START_TEST(devinst)
{
    static BOOL (WINAPI *pIsWow64Process)(HANDLE, BOOL *);
    HMODULE module = GetModuleHandleA("setupapi.dll");
    struct testsign_context ctx;
    HKEY hkey;

    pDriverStoreAddDriverPackageA = (void *)GetProcAddress(module, "DriverStoreAddDriverPackageA");
    pDriverStoreFindDriverPackageA = (void *)GetProcAddress(module, "DriverStoreFindDriverPackageA");
    pDriverStoreDeleteDriverPackageA = (void *)GetProcAddress(module, "DriverStoreDeleteDriverPackageA");
    pSetupQueryInfOriginalFileInformationA = (void *)GetProcAddress(module, "SetupQueryInfOriginalFileInformationA");

    test_get_actual_section();

    if ((hkey = SetupDiOpenClassRegKey(NULL, KEY_ALL_ACCESS)) == INVALID_HANDLE_VALUE)
    {
        skip("needs admin rights\n");
        return;
    }
    RegCloseKey(hkey);

    pIsWow64Process = (void *)GetProcAddress(GetModuleHandleA("kernel32.dll"), "IsWow64Process");
    if (pIsWow64Process) pIsWow64Process(GetCurrentProcess(), &wow64);

    test_create_device_list_ex();
    test_open_class_key();
    test_install_class();
    test_device_info();
    test_device_property();
    test_get_device_instance_id();
    test_open_device_info();
    test_register_device_info();
    test_device_iface();
    test_device_iface_detail();
    test_device_key();
    test_register_device_iface();
    test_registry_property_a();
    test_registry_property_w();
    test_get_inf_class();
    test_devnode();
    test_device_interface_key();
    test_open_device_interface_key();
    test_device_install_params();
    test_driver_list();
    test_call_class_installer();
    test_get_class_devs();

#ifdef __REACTOS__
    if (GetNTVersion() <= _WIN32_WINNT_WS03 || !testsign_create_cert(&ctx))
#else
    if (!testsign_create_cert(&ctx))
#endif
        return;

    test_copy_oem_inf(&ctx);
    test_driver_store(&ctx);

    testsign_cleanup(&ctx);
}
