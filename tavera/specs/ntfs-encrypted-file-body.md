<!--
PROJECT:     VerOS NT
LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
FILE:        tavera/specs/ntfs-encrypted-file-body.md
PURPOSE:     Record the documented encrypted-file reads and the missing body layout
COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@taveraos.com>
PROGRAMMERS: Ryan Wilson <ryan@taveraos.com>
-->

# NTFS encrypted file body

[Handling Encrypted Files and Directories](https://learn.microsoft.com/en-us/windows/win32/fileio/handling-encrypted-files-and-directories) says `EncryptFile` encrypts every data stream in the file, and that a later read through the normal file APIs returns plaintext for a caller who can decrypt it.

[Backup and Restore of Encrypted Files](https://learn.microsoft.com/en-us/windows/win32/fileio/backup-and-restore-of-encrypted-files) says `ReadEncryptedFileRaw` returns the file's data still encrypted.

[MS-FSCC: NTFS Attribute Types](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-fscc/a82e9105-2405-4e37-b2c3-28c773902d85) says `$LOGGED_UTILITY_STREAM` is used by the encrypting file system.

[MS-EFSR: EFSRPC Raw Data Format](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-efsr/e67520ea-0cb6-4349-bbe5-427a644d9492) says the details of that backup format are implementation-dependent and that a client should not parse it. The page does not say those bytes are the on-disk file body.

[MS-GPEF: Overview](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-gpef/4d4687f9-132c-4501-9996-684b40b14735) says a symmetric key encrypts the file data. It does not give the framing of that data on disk.

`NtfsReadEncrypted`, `NtfsWriteEncrypted`, and `NtfsSetEncryption` in `drivers/filesystems/ntfs/ntfs3x.c` return `STATUS_NOT_IMPLEMENTED`. `FSCTL_SET_ENCRYPTION` in `drivers/filesystems/ntfs/fsctl.c` calls `NtfsSetEncryption`. Those returns stay.
