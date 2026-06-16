/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Test for RtlGetProductInfo
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "precomp.h"

typedef
BOOLEAN
WINAPI
FN_RtlGetProductInfo(
    _In_ ULONG OSMajorVersion,
    _In_ ULONG OSMinorVersion,
    _In_ ULONG SpMajorVersion,
    _In_ ULONG SpMinorVersion,
    _Out_ PULONG ReturnedProductType);

FN_RtlGetProductInfo* pRtlGetProductInfo;

void Test_Product_(ULONG Line, ULONG OSMajorVersion, ULONG OSMinorVersion, ULONG SpMajorVersion, ULONG SpMinorVersion, BOOLEAN ExpectedResult, ULONG ExpectedProductType)
{
    ULONG ProductType = 0xDEADBEEF;
    BOOLEAN Result;
    Result = pRtlGetProductInfo(OSMajorVersion, OSMinorVersion, SpMajorVersion, SpMinorVersion, &ProductType);
    ok_(__FILE__, Line)(Result == ExpectedResult, "RtlGetProductInfo wrong return value. Got %u, expected %u\n", Result, ExpectedResult);
    ok_(__FILE__, Line)(ProductType == ExpectedProductType, "RtlGetProductInfo wrong product type. Got 0x%x, expected 0x%x\n", ProductType, ExpectedProductType);
}
#define Test_Product(OSMajorVersion, OSMinorVersion, SpMajorVersion, SpMinorVersion, ExpectedResult, ExpectedProductType) \
    Test_Product_(__LINE__, OSMajorVersion, OSMinorVersion, SpMajorVersion, SpMinorVersion, ExpectedResult, ExpectedProductType)


START_TEST(RtlGetProductInfo)
{
    HINSTANCE hinst = GetModuleHandleW(L"ntdll.dll");
    pRtlGetProductInfo = (FN_RtlGetProductInfo*)GetProcAddress(hinst, "RtlGetProductInfo");
    if (pRtlGetProductInfo == NULL)
    {
        ok(GetNTVersion() < _WIN32_WINNT_VISTA, "RtlGetProductInfo missing on NT version 0x%lx\n", GetNTVersion());
        skip("RtlGetProductInfo not found\n");
        return;
    }

    /* Test invalid parameters */
    ok_eq_bool(pRtlGetProductInfo(0, 0, 0, 0, NULL), FALSE);
    ok_eq_bool(pRtlGetProductInfo(6, 0, 0, 0, NULL), FALSE);

    /* Test invalid versions */
    Test_Product(0, 0, 0, 0, FALSE, PRODUCT_UNDEFINED);
    Test_Product(5, 0, 0, 0, FALSE, PRODUCT_UNDEFINED);
    Test_Product(5, 1, 0, 0, FALSE, PRODUCT_UNDEFINED);
    Test_Product(5, 2, 0, 0, FALSE, PRODUCT_UNDEFINED);
    Test_Product(5, 6, 0, 0, FALSE, PRODUCT_UNDEFINED);

    /* Get version information of the current OS */
    OSVERSIONINFOEXA OsVersionInfo = { .dwOSVersionInfoSize = sizeof(OsVersionInfo) };
    GetVersionExA((LPOSVERSIONINFOA)&OsVersionInfo);

    /* Get the product type for the current OS version */
    BOOLEAN Result;
    ULONG CurrentProductType = 0xDEADBEEF;
    Result = pRtlGetProductInfo(OsVersionInfo.dwMajorVersion,
                                OsVersionInfo.dwMinorVersion,
                                OsVersionInfo.wServicePackMajor,
                                OsVersionInfo.wServicePackMinor,
                                &CurrentProductType);
    ok_eq_bool(Result, TRUE);
    ok(CurrentProductType != PRODUCT_UNDEFINED, "CurrentProductType is PRODUCT_UNDEFINED\n");

    /* Check if the result is reasonable */
    switch (GetNTVersion())
    {
        case _WIN32_WINNT_VISTA: // Vista or Server 2008
            ok((CurrentProductType == PRODUCT_STARTER) ||
               (CurrentProductType == PRODUCT_HOME_BASIC) ||
               (CurrentProductType == PRODUCT_HOME_PREMIUM) ||
               (CurrentProductType == PRODUCT_BUSINESS) ||
               (CurrentProductType == PRODUCT_ENTERPRISE) ||
               (CurrentProductType == PRODUCT_ULTIMATE) ||
               (CurrentProductType == PRODUCT_SERVER_FOUNDATION) ||
               (CurrentProductType == PRODUCT_STANDARD_SERVER) ||
               (CurrentProductType == PRODUCT_ENTERPRISE_SERVER),
               "Unexpected product type for Vista: 0x%x\n", CurrentProductType);
            break;

        case _WIN32_WINNT_WIN7: // Windows 7 or Server 2008 R2
            ok((CurrentProductType == PRODUCT_STARTER) ||
               (CurrentProductType == PRODUCT_HOME_BASIC) ||
               (CurrentProductType == PRODUCT_HOME_PREMIUM) ||
               (CurrentProductType == PRODUCT_PROFESSIONAL) ||
               (CurrentProductType == PRODUCT_ENTERPRISE) ||
               (CurrentProductType == PRODUCT_ULTIMATE) ||
               (CurrentProductType == PRODUCT_SERVER_FOUNDATION) ||
               (CurrentProductType == PRODUCT_STANDARD_SERVER) ||
               (CurrentProductType == PRODUCT_ENTERPRISE_SERVER),
               "Unexpected product type for Windows 7: 0x%x\n", CurrentProductType);
            break;

        case _WIN32_WINNT_WIN8: // Windows 8 or Server 2012
            ok((CurrentProductType == PRODUCT_CORE) ||
               (CurrentProductType == PRODUCT_PROFESSIONAL) ||
               (CurrentProductType == PRODUCT_PROFESSIONAL_WMC) ||
               (CurrentProductType == PRODUCT_ENTERPRISE) ||
               (CurrentProductType == PRODUCT_SERVER_FOUNDATION) ||
               (CurrentProductType == PRODUCT_HOME_SERVER) || // Essentials
               (CurrentProductType == PRODUCT_STANDARD_SERVER) ||
               (CurrentProductType == PRODUCT_DATACENTER_SERVER),
               "Unexpected product type for Windows 8: 0x%x\n", CurrentProductType);
            break;

        case _WIN32_WINNT_WINBLUE: // Windows 8.1 or Server 2012 R2
            ok((CurrentProductType == PRODUCT_CORE) ||
               (CurrentProductType == PRODUCT_PROFESSIONAL) ||
               (CurrentProductType == PRODUCT_ENTERPRISE) ||
               (CurrentProductType == PRODUCT_SERVER_FOUNDATION) ||
               (CurrentProductType == PRODUCT_HOME_SERVER) || // Essentials
               (CurrentProductType == PRODUCT_STANDARD_SERVER) ||
               (CurrentProductType == PRODUCT_DATACENTER_SERVER),
               "Unexpected product type for Windows 8.1: 0x%x\n", CurrentProductType);
            break;

        case _WIN32_WINNT_WIN10: // Windows 10 or Server 2016
        default:
            ok((CurrentProductType == PRODUCT_CORE) || // Home
               (CurrentProductType == PRODUCT_PROFESSIONAL) ||
               (CurrentProductType == PRODUCT_PRO_WORKSTATION) ||
               (CurrentProductType == PRODUCT_EDUCATION) ||
               (CurrentProductType == PRODUCT_PRO_FOR_EDUCATION) ||
               (CurrentProductType == PRODUCT_ENTERPRISE) ||
               (CurrentProductType == PRODUCT_IOTENTERPRISES) ||
               (CurrentProductType == PRODUCT_ENTERPRISE_SERVER) ||
               (CurrentProductType == PRODUCT_DATACENTER_SERVER),
               "Unexpected product type for Windows 10: 0x%x\n", CurrentProductType);
            break;
    }

    /* TODO: Fix this up for more versions as needed */
    ULONG VistaProductType = CurrentProductType;
    if (CurrentProductType == PRODUCT_PROFESSIONAL)
    {
        VistaProductType = PRODUCT_BUSINESS;
    }

    /* Test a few version values */
    Test_Product(6, 0, 0, 0, TRUE, VistaProductType);
    Test_Product(6, 1, 0, 0, TRUE, CurrentProductType);
    Test_Product(6, 2, 0, 0, TRUE, CurrentProductType);
    Test_Product(6, 3, 0, 0, TRUE, CurrentProductType);
    Test_Product(6, 4, 0, 0, TRUE, CurrentProductType);
    Test_Product(7, 0, 0, 0, TRUE, CurrentProductType);
    Test_Product(10, 0, 0, 0, TRUE, CurrentProductType);
    Test_Product(23, 212, 0, 0, TRUE, CurrentProductType);
}
