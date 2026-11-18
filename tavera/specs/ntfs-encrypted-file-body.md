<!--
PROJECT:     Tavera OS
LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
FILE:        tavera/specs/ntfs-encrypted-file-body.md
PURPOSE:     Record the documented encrypted-file reads and the missing body layout
COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@taveraos.com>
PROGRAMMERS: Ryan Wilson <ryan@taveraos.com>
-->

# NTFS encrypted file body

## 1. Header and provenance

- **Component:** NTFS encrypted file body
- **Version:** not assigned
- **Author:** Ryan Wilson <ryan@taveraos.com>
- **Date:** not recorded on the previous text. Reorganized to this template on 2026-11-18.
- **Provenance:** Only allowed sources were used. They are the public documents and in-tree files in section 2. No Windows guest observation is recorded. This file does not add the on-disk framing of encrypted file data.

## 2. Sources

Every claim below names one of these entries. The date each page was read was not recorded.

1. [Handling Encrypted Files and Directories](https://learn.microsoft.com/en-us/windows/win32/fileio/handling-encrypted-files-and-directories). Date read: not recorded.
2. [Backup and Restore of Encrypted Files](https://learn.microsoft.com/en-us/windows/win32/fileio/backup-and-restore-of-encrypted-files). Date read: not recorded.
3. [MS-FSCC: NTFS Attribute Types](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-fscc/a82e9105-2405-4e37-b2c3-28c773902d85). Date read: not recorded.
4. [MS-EFSR: EFSRPC Raw Data Format](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-efsr/e67520ea-0cb6-4349-bbe5-427a644d9492). Date read: not recorded.
5. [MS-GPEF: Overview](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-gpef/4d4687f9-132c-4501-9996-684b40b14735). Date read: not recorded.
6. `drivers/filesystems/ntfs/ntfs3x.c` in this tree. `NtfsReadEncrypted`, `NtfsWriteEncrypted`, and `NtfsSetEncryption`.
7. `drivers/filesystems/ntfs/fsctl.c` in this tree. `FSCTL_SET_ENCRYPTION`.

## 3. Purpose and scope

This component is the body of a file that NTFS stores encrypted, and the reads that return either plaintext or the still-encrypted bytes.

In scope: what `EncryptFile`, a normal read, and `ReadEncryptedFileRaw` return, the attribute type named for the encrypting file system, and the in-tree entry points.

Left out on purpose: the on-disk framing of the encrypted data. Source 5 does not give it. Source 4 describes a backup format and does not say those bytes are the on-disk file body.

## 4. Interfaces

- **`EncryptFile`.** Encrypts every data stream in the file (source 1). Parameters, types, and calling convention are not listed in the previous text.
- **Normal file read.** A later read through the normal file APIs returns plaintext for a caller who can decrypt the file (source 1).
- **`ReadEncryptedFileRaw`.** Returns the file's data still encrypted (source 2). Parameters and calling convention are not listed in the previous text.
- **`$LOGGED_UTILITY_STREAM`.** Attribute type used by the encrypting file system (source 3).
- **`NtfsReadEncrypted`, `NtfsWriteEncrypted`, `NtfsSetEncryption`.** Defined in `drivers/filesystems/ntfs/ntfs3x.c` (source 6). Parameters and calling convention are not listed in the previous text. Each returns `STATUS_NOT_IMPLEMENTED` in this tree.
- **`FSCTL_SET_ENCRYPTION`.** Handled in `drivers/filesystems/ntfs/fsctl.c`, which calls `NtfsSetEncryption` (source 7).

## 5. Behavior

`EncryptFile` encrypts every data stream in the file (source 1). A caller who can decrypt the file then reads plaintext through the normal file APIs (source 1). `ReadEncryptedFileRaw` returns those bytes still encrypted (source 2).

A symmetric key encrypts the file data (source 5). The framing of that data on disk is not in source 5.

Source 4 says the details of the backup format are implementation-dependent and that a client should not parse it. The same page does not say those bytes are the on-disk file body.

In this tree, `NtfsReadEncrypted`, `NtfsWriteEncrypted`, and `NtfsSetEncryption` return `STATUS_NOT_IMPLEMENTED` (source 6). `FSCTL_SET_ENCRYPTION` calls `NtfsSetEncryption` (source 7). Those returns stay.

Threading, IRQL, and memory ownership are not stated by sources 1 through 7.

## 6. Observed behavior

No black-box test on Windows is recorded. No Windows build is named.

## 7. Compatibility notes

No comparison between Windows 10 22H2 and an older version is recorded.

This tree rejects encrypted read, encrypted write, and set-encryption with `STATUS_NOT_IMPLEMENTED` (sources 6 and 7).

## 8. Test plan

These checks compare results. They do not say how the file body is stored.

Until the on-disk framing is specified:

- On Tavera OS, encrypted read, encrypted write, and `FSCTL_SET_ENCRYPTION` each return `STATUS_NOT_IMPLEMENTED`.
- Do not compare on-disk encrypted bytes with Windows. This spec has no Windows result for those bytes.

After a later spec states the framing:

- Encrypt one file on Windows and the same file on Tavera OS. A normal read by a caller who can decrypt it returns the original plaintext on both. `ReadEncryptedFileRaw` returns still-encrypted bytes on both. Record the Windows build, the input, and both outputs.

## 9. Open questions

- The on-disk framing of data encrypted with the symmetric key.
- Whether the EFSRPC raw backup bytes match the on-disk file body. Source 4 does not say they do.
- Parameters, types, and calling conventions for `EncryptFile`, `ReadEncryptedFileRaw`, `NtfsReadEncrypted`, `NtfsWriteEncrypted`, and `NtfsSetEncryption`.
- Threading, IRQL, and memory ownership.
- Any difference between Windows 10 22H2 and older versions.
- Any black-box input, output, and Windows build.

## 10. Review log

No review is recorded. Reorganized to this template on 2026-11-18. A reviewer still has to confirm this file has no implementation detail from a forbidden source.
