<!--
PROJECT:     VerOS NT
LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
FILE:        tavera/specs/ntfs-quota-on-disk.md
PURPOSE:     Record the documented NTFS quota query and the missing index value
COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@taveraos.com>
PROGRAMMERS: Ryan Wilson <ryan@taveraos.com>
-->

# NTFS quota on-disk value

[MS-FSCC: NTFS Reserved File Names](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-fscc/b04c3bd0-79dc-4e58-b8ed-74f19fc2ea0a) lists `$Extend\$Quota` as quota information.

[MS-FSCC: NTFS Stream Names](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-fscc/6b5e7e75-c2ad-4769-ad96-37077ee69702) names `$Q` as the quota index stream.

[MS-FSCC 2.4.41](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-fscc/acdc0738-ba3c-47a1-b11a-72e22d831c57) defines `FILE_QUOTA_INFORMATION`. On set, `ChangeTime` and `QuotaUsed` are ignored. A threshold or limit of `-1` means none is set. A limit of `-2` deletes the entry.

[MS-FSA: Server Requests Querying Quota Information](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-fsa/b81634b1-ce07-4770-ab21-af87f9f53515) gives the status rules for that query. [MS-FSA Appendix A](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-fsa/4e3695bd-7574-4f24-a223-b4679c065b63) notes 198 and 200 name the stream `$Extend\$Quota:$Q:$INDEX_ALLOCATION`.

`FILE_QUOTA_INFORMATION` is already declared in `sdk/include/ndk/iotypes.h`. That is the query and set buffer. None of the pages above define the bytes stored in the `$Q` index entry.

`NtfsSetQuota` in `drivers/filesystems/ntfs/ntfs3x.c` returns `STATUS_NOT_IMPLEMENTED`. `FSCTL_SET_QUOTA` in `drivers/filesystems/ntfs/fsctl.c` calls it. That return stays.
