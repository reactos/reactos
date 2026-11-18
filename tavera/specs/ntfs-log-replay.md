<!--
PROJECT:     Tavera OS
LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
FILE:        tavera/specs/ntfs-log-replay.md
PURPOSE:     Record the documented NTFS log recovery behavior and the missing record layout
COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@taveraos.com>
PROGRAMMERS: Ryan Wilson <ryan@taveraos.com>
-->

# NTFS log replay

## 1. Header and provenance

- **Component:** NTFS log replay
- **Version:** not assigned
- **Author:** Ryan Wilson <ryan@taveraos.com>
- **Date:** not recorded on the previous text. Reorganized to this template on 2026-11-18.
- **Provenance:** Only allowed sources were used. They are the public documents and in-tree files in section 2. No Windows guest observation is recorded. This file does not add a restart-page layout or a log-record layout.

## 2. Sources

Every claim below names one of these entries. The date each page was read was not recorded.

1. [MS-FSCC: NTFS Reserved File Names](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-fscc/b04c3bd0-79dc-4e58-b8ed-74f19fc2ea0a). Date read: not recorded.
2. [How NTFS Works](https://learn.microsoft.com/en-us/previous-versions/windows/it-pro/windows-server-2003/cc781134(v=ws.10)). Date read: not recorded.
3. [NTFS overview](https://learn.microsoft.com/en-us/windows-server/storage/file-server/ntfs-overview). Date read: not recorded.
4. `drivers/filesystems/ntfs/ntfs.h` in this tree. It does not declare a restart page or a log record.
5. `drivers/filesystems/ntfs/ntfs3x.c` in this tree. `NtfsReplayLog` and `NtfsLogFileRecord`.
6. `drivers/filesystems/ntfs/fsctl.c` in this tree. Mount calls `NtfsReplayLog`.

## 3. Purpose and scope

NTFS uses a transaction log to restore file-system consistency after a system failure (source 3). Recovery redoes each committed transaction found in the log, then rolls back transactions that were not committed (source 2).

In scope: that recovery order, the reserved log-file name, and the in-tree entry points that would perform it.

Left out on purpose: the byte layout of a restart page and the byte layout of a log record. Sources 1 through 4 do not give those layouts. Replay stays unimplemented until a later spec states them.

## 4. Interfaces

- **`$LogFile`.** Reserved file name for the transactional logging file (source 1).
- **`NtfsReplayLog`.** Defined in `drivers/filesystems/ntfs/ntfs3x.c` (source 5). The previous text does not list its parameters, types, or calling convention. Its return in this tree is `STATUS_SUCCESS`. It ignores the volume.
- **Mount.** `drivers/filesystems/ntfs/fsctl.c` calls `NtfsReplayLog` when `VCB_VOLUME_READ_ONLY` is set (source 6).
- **`NtfsLogFileRecord`.** Defined in `drivers/filesystems/ntfs/ntfs3x.c` (source 5). The function body is empty. Parameters and calling convention are not listed in the previous text.

## 5. Behavior

The log records for a transaction are written to disk before the metadata that transaction changes (source 2).

During recovery, NTFS redoes each committed transaction found in the log, then rolls back transactions that were not committed (source 2). The transaction log is the record used to restore consistency after a system failure (source 3).

In this tree, `NtfsReplayLog` returns `STATUS_SUCCESS` and does not read the volume (source 5). `NtfsLogFileRecord` does nothing (source 5). Those stay as they are, because replay needs the missing layout (source 4).

Threading, IRQL, and memory ownership are not stated by sources 1 through 6.

## 6. Observed behavior

No black-box test on Windows is recorded. No Windows build is named.

## 7. Compatibility notes

Source 2 is a Windows Server 2003 document. Nothing in this file compares that description with Windows 10 22H2, or with any other build.

This tree's mount path returns success from `NtfsReplayLog` and does not apply the log (sources 5 and 6).

## 8. Test plan

These checks compare results. They do not say how replay is coded.

Until a layout is specified:

- Mount a read-only NTFS volume on Tavera OS. The replay entry point returns `STATUS_SUCCESS`. The volume bytes are unchanged.
- The same mount on Windows is not a comparison for replay bytes, because this spec has no Windows result to match.

After a later spec states the record layout:

- Prepare one volume whose log contains a committed transaction, and one whose log contains a transaction that was not committed. Interrupt the update the same way on Windows and on Tavera OS. After mount, the committed transaction's metadata is present and the uncommitted transaction's metadata is absent. Record the Windows build beside the result.

## 9. Open questions

- The byte layout of a restart page.
- The byte layout of a log record.
- The parameters, types, and calling convention of `NtfsReplayLog` and `NtfsLogFileRecord`.
- Threading, IRQL, and memory ownership.
- Whether Windows 10 22H2 follows the recovery order in source 2.
- Any black-box input, output, and Windows build.

## 10. Review log

No review is recorded. Reorganized to this template on 2026-11-18. A reviewer still has to confirm this file has no implementation detail from a forbidden source.
