<!--
PROJECT:     VerOS NT
LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
FILE:        tavera/specs/ntfs-log-replay.md
PURPOSE:     Record the documented NTFS log recovery behavior and the missing record layout
COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@taveraos.com>
PROGRAMMERS: Ryan Wilson <ryan@taveraos.com>
-->

# NTFS log replay

[MS-FSCC: NTFS Reserved File Names](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-fscc/b04c3bd0-79dc-4e58-b8ed-74f19fc2ea0a) lists `$LogFile` as the transactional logging file.

[How NTFS Works](https://learn.microsoft.com/en-us/previous-versions/windows/it-pro/windows-server-2003/cc781134(v=ws.10)) says that during recovery NTFS redoes each committed transaction found in the log, then rolls back transactions that were not committed. The same page says the log records for a transaction are written to disk before the metadata that transaction changes.

[NTFS overview](https://learn.microsoft.com/en-us/windows-server/storage/file-server/ntfs-overview) says NTFS uses its transaction log to restore file system consistency after a system failure.

None of those pages, and no declaration in `drivers/filesystems/ntfs/ntfs.h`, gives the byte layout of a restart page or a log record.

`NtfsReplayLog` in `drivers/filesystems/ntfs/ntfs3x.c` returns `STATUS_SUCCESS` and ignores the volume. Mount calls it from `drivers/filesystems/ntfs/fsctl.c` when `VCB_VOLUME_READ_ONLY` is set. `NtfsLogFileRecord` in the same file is empty. Replaying the log needs the missing layout, so this stays unimplemented.
