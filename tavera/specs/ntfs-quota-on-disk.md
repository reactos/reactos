<!--
PROJECT:     Tavera OS
LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
FILE:        tavera/specs/ntfs-quota-on-disk.md
PURPOSE:     Record the documented NTFS quota query and the missing index value
COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@taveraos.com>
PROGRAMMERS: Ryan Wilson <ryan@taveraos.com>
-->

# NTFS quota on-disk value

## 1. Header and provenance

- **Component:** NTFS quota on-disk value
- **Version:** not assigned
- **Author:** Ryan Wilson <ryan@taveraos.com>
- **Date:** not recorded on the previous text. Reorganized to this template on 2026-11-18.
- **Provenance:** Only allowed sources were used. They are the public documents and in-tree files in section 2. No Windows guest observation is recorded. This file does not add the bytes stored in a `$Q` index entry.

## 2. Sources

Every claim below names one of these entries. The date each page was read was not recorded.

1. [MS-FSCC: NTFS Reserved File Names](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-fscc/b04c3bd0-79dc-4e58-b8ed-74f19fc2ea0a). Date read: not recorded.
2. [MS-FSCC: NTFS Stream Names](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-fscc/6b5e7e75-c2ad-4769-ad96-37077ee69702). Date read: not recorded.
3. [MS-FSCC 2.4.41](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-fscc/acdc0738-ba3c-47a1-b11a-72e22d831c57). Date read: not recorded. Defines `FILE_QUOTA_INFORMATION`.
4. [MS-FSA: Server Requests Querying Quota Information](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-fsa/b81634b1-ce07-4770-ab21-af87f9f53515). Date read: not recorded.
5. [MS-FSA Appendix A](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-fsa/4e3695bd-7574-4f24-a223-b4679c065b63). Date read: not recorded. Notes 198 and 200.
6. `sdk/include/ndk/iotypes.h` in this tree. Declares `FILE_QUOTA_INFORMATION`.
7. `drivers/filesystems/ntfs/ntfs3x.c` in this tree. `NtfsSetQuota`.
8. `drivers/filesystems/ntfs/fsctl.c` in this tree. `FSCTL_SET_QUOTA`.

## 3. Purpose and scope

This component is the quota information NTFS stores for a volume, and the query and set buffer callers use to read and change it.

In scope: the reserved names, the query and set buffer already declared in this tree, the set-buffer rules restated in section 5, and the in-tree set path.

Left out on purpose: the bytes stored in a `$Q` index entry. Sources 1 through 6 do not define them. The status rules for the query are named by source 4 and are not restated here.

## 4. Interfaces

- **`$Extend\$Quota`.** Reserved name for quota information (source 1).
- **`$Q`.** Quota index stream (source 2).
- **`$Extend\$Quota:$Q:$INDEX_ALLOCATION`.** Stream name given by notes 198 and 200 (source 5).
- **`FILE_QUOTA_INFORMATION`.** Query and set buffer. Already declared in `sdk/include/ndk/iotypes.h` (source 6). Source 3 is the public definition. The previous text names the fields `ChangeTime`, `QuotaUsed`, the threshold, and the limit. It does not restate the rest of the buffer, its size, or a calling convention.
- **`NtfsSetQuota`.** Defined in `drivers/filesystems/ntfs/ntfs3x.c` (source 7). Parameters and calling convention are not listed in the previous text. Its return in this tree is `STATUS_NOT_IMPLEMENTED`.
- **`FSCTL_SET_QUOTA`.** Handled in `drivers/filesystems/ntfs/fsctl.c`, which calls `NtfsSetQuota` (source 8).

## 5. Behavior

On set, `ChangeTime` and `QuotaUsed` are ignored (source 3). A threshold or limit of `-1` means none is set (source 3). A limit of `-2` deletes the entry (source 3).

Source 4 gives the status rules for the query. Those status values are not copied into this file.

None of sources 1 through 6 define the bytes stored in the `$Q` index entry.

In this tree, `NtfsSetQuota` returns `STATUS_NOT_IMPLEMENTED`, and `FSCTL_SET_QUOTA` calls it (sources 7 and 8). That return stays.

Threading, IRQL, and memory ownership are not stated by sources 1 through 8.

## 6. Observed behavior

No black-box test on Windows is recorded. No Windows build is named.

## 7. Compatibility notes

No comparison between Windows 10 22H2 and an older version is recorded.

This tree rejects a quota set with `STATUS_NOT_IMPLEMENTED` (sources 7 and 8).

## 8. Test plan

These checks compare results. They do not say how the index entry is stored.

Until the `$Q` bytes are specified:

- On Tavera OS, a quota set through `FSCTL_SET_QUOTA` returns `STATUS_NOT_IMPLEMENTED`.
- Do not compare on-disk quota bytes with Windows. This spec has no Windows result for those bytes.

After a later spec states the index entry, and after the query status rules are restated here:

- Set a threshold and a limit on Windows and on Tavera OS with the same `FILE_QUOTA_INFORMATION` input. A following query returns that threshold and that limit. `ChangeTime` and `QuotaUsed` in the set buffer do not change the stored values. A threshold or limit of `-1` leaves that bound unset. A limit of `-2` removes the entry. Record the Windows build, the input, and the output beside each result.

## 9. Open questions

- The bytes stored in a `$Q` index entry.
- The query status codes from source 4, which are not restated in this file.
- The full field list, size, and calling convention of `FILE_QUOTA_INFORMATION` beyond the names in section 4.
- The parameters and calling convention of `NtfsSetQuota`.
- Threading, IRQL, and memory ownership.
- Any difference between Windows 10 22H2 and older versions.
- Any black-box input, output, and Windows build.

## 10. Review log

No review is recorded. Reorganized to this template on 2026-11-18. A reviewer still has to confirm this file has no implementation detail from a forbidden source.
