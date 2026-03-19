/*
 * PROJECT:     ReactOS Zip Shell Extension
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     IDataObject for dragging/copying files out of a ZIP folder
 * COPYRIGHT:   Copyright 2024-2026 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#pragma once

// Forward declaration
class CZipFolder;

// Creates an IDataObject that, when CF_HDROP is requested, extracts the
// selected entries from the ZIP into a temporary directory and returns
// their paths via HDROP.  The temporary directory is cleaned up when the
// object's last reference is released.
HRESULT CZipExtractDrop_CreateInstance(
    CZipFolder*             pFolder,
    UINT                    cidl,
    PCUITEMID_CHILD_ARRAY   apidl,
    IDataObject**           ppDataObject);
