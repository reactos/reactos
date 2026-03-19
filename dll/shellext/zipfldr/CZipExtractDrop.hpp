/*
 * PROJECT:     ReactOS Zip Shell Extension
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     IDataObject for dragging/copying files out of a ZIP folder
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#pragma once

class CZipFolder; // Forward declaration

HRESULT CZipExtractDrop_CreateInstance(
    CZipFolder*             pFolder,
    UINT                    cidl,
    PCUITEMID_CHILD_ARRAY   apidl,
    IDataObject**           ppDataObject);
