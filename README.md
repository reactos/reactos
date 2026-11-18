# VerOS NT™

VerOS NT is my hobby. It's my fork of [ReactOS](https://github.com/reactos/reactos), and I'm the only person on it. I want a general-purpose operating system that can install and run what Windows 10 22H2 can, x64 first. 32-bit programs on that same boot come later. Cursor and Grok write most of the patches. Before one is committed, the diff is read again, and a hunk that looks copied is shown to me. I check its sources before it is committed.

These builds are alpha. I run them in a virtual machine so I can see what breaks. Don't install this on a machine you use.

Right now the work is boot, NTFS, drivers, and the calls an application makes. The plan is the [milestones on GitHub](https://github.com/taveraos/taveraos/milestones). The number in the title is the order, and I close a milestone there when that work is done.

## How I record Windows

I boot Windows 10 22H2 and run my own probes in it. Each probe calls something the OS already offers, then prints what came back. The log is that printout. I compare it with the same probe on VerOS. What I keep from a Windows run is the name I asked about and the status code. I don't copy a program, a header, or a disassembly out of the guest.

Windows 10 22H2 boots in QEMU with 8 GB of RAM and 4 CPUs, the same as VerOS. The boot does not write the Windows disk. A capture uses the plain emulator, not WHPX. A syscall probe calls `GetProcAddress` for a name, calls that function, and prints the status code. A kernel probe and a graphics probe do that for their calls. I use a Windows log when it ends with `DIFF END OK`. If I change a probe, I boot Windows again. If I don't, I reuse the log I have.

A separate trace records only a call number, a status, and four arguments, with no names. That trace stays out of the kernel. I don't take the guest apart to make up more syscall numbers.

The logs stay out of this repository. After a run, the result files are copied into a private archive, one new folder each time, and that archive is its own git repository. I don't paste a log into the kernel tree. A note may include the SHA-256 of a log only when that file is still on disk and still contains the line the note quotes. A later capture is not hashed as if it were the earlier one.

## What a patch is allowed to come from

If the change is about how Windows behaves, it can only come from public docs, public specs, code already in this tree, or a log from booting this tree. A size, an offset, or a return code comes from one of those. I don't paste a header, and I don't rebuild a function from one.

If any of this shows up, that piece of work stops: Windows source, leaked NT source, or the Windows Research Kernel; a decompiler listing, or a disassembly of a Microsoft binary; WDK or SDK headers, a `.lib`, a `.sys`, or other object code. That stays out of the repo and out of the patch. Once it has shown up, the work stops there.

Before a commit, the diff is read again. If a hunk has a whole function I didn't ask for, a comment about Windows internals nobody mentioned, header text, or code that looks decompiled, the commit stops and that hunk is shown to me. I look up where it came from. I commit it when I can cite a public document, a public spec, code already in this tree, or a log from booting this tree, or when Cursor, Grok, and I got it without using the list above. If I can't cite a source, we rewrite that hunk from one of those allowed sources. I'm not aware of Cursor or Grok using any unauthorized code.

## Source notes

If Cursor commits a change to boot, the kernel, the HAL, a driver, NTFS, or a Windows DLL, that commit also adds a note under [`veros/transparency/`](veros/transparency/). Docs, branding, GitHub workflows, and the editor rules don't get one. The note is in the same commit. The name is picked first, with the date and a short title. It can't be that commit's id, because the id includes the file. If I miss one, a later commit adds a note named with the old commit's full id. One catch-up commit recorded the commits that were still missing a note. After that, a missed note goes in the next commit that changes the code. I don't make another commit whose only change is a note.

The note says what question I was answering, what I was allowed to use, and that the license is GPL-2.0-or-later. Each input is something that was actually used, and it says where to open it. A public document or another project's file includes an https link. A file in this tree includes its path. A guest log includes its path and the line that shows it finished, such as `DIFF END OK`. Something I said starts with `Reported:`. There's a harness section only if a guest ran. There's a before and after only if both numbers come from a log I committed or a dashboard snapshot. The prompt stays out. So do the old yes-or-no checkboxes, a copied trace, a header, a disassembly, and a Windows call number.

GitHub runs [tools/transparency/check_records.py](tools/transparency/check_records.py) on a push or a pull request to master. The check fails a Cursor commit in those directories when no note that commit added is still in the tree and the tree has no note named with that commit's full id. It fails a note that includes the prompt or the old yes-or-no checkboxes. When a log hash is present, it has to be 64 lowercase hex characters.

## What the record is

The notes and the check are the record of what a patch used. I'm the person who reads the logs and the person who commits the code. Code that has to return a particular status, or write a field at a known offset, will look like Windows. The notes don't promise that a function is unlike Microsoft's. They cover the commits I make. They don't review the ReactOS code this fork started from.

## Building

The daily build is Windows, amd64, Debug, target `bootcd`. A native 32-bit boot isn't supported. `Setup-BuildEnvironment.ps1` in the parent of this checkout downloads the tools into `tools\`. Details are in [docs/LOCAL_BUILD.md](docs/LOCAL_BUILD.md).

You need:

- Windows, with PowerShell and `curl.exe`
- Git
- 7-Zip 26.06, so the setup script can unpack the installers
- ReactOS Build Environment 2.2.1. That package supplies CMake 3.17, Ninja, Bison, m4, Flex, and the amd64 MinGW GCC the environment script uses
- Clang 18.1.8, the Windows package `clang+llvm-18.1.8-x86_64-pc-windows-msvc`. This is the compiler. RosBE's amd64 GCC was built without plugin support, so the amd64 build uses Clang
- QEMU 11.1.0 (`qemu-w64-setup-20260811.exe`), to boot the ISO. The boot test uses the plain emulator, 8 GB of RAM, and 4 CPUs
- Rust 1.99.1, target `x86_64-pc-windows-gnu`. A new file is Rust unless the change has to stay in an existing C or C++ file, or C is required for a binary interface, a build that only compiles C, or code that runs before a Rust runtime exists. The boot CD build does not call `rustc` yet

From the source tree:

```powershell
.\tools\pipeline\Build-Bootcd.ps1
.\tools\pipeline\Test-Boot.ps1
```

`Build-Bootcd.ps1` writes `build-amd64\bootcd.iso`. `tools/pipeline/Resolve-VerosPaths.ps1` finds the buildenv folder (parent of this checkout, or a sibling `buildenv/`) on whatever drive the tree sits on, then uses that folder's `tools\rosbe`, `tools\clang`, and `env.cmd`. `Test-Boot.ps1` uses the same discovery for `tools\qemu`, then `%ProgramFiles%\qemu` if needed. `build-amd64/` and `output/` stay untracked.

## License

The license is [GPL-2.0](COPYING). There is no warranty. If a file header names a different license, that header wins. A new userland DLL is LGPL-2.1-or-later. Any other new file is GPL-2.0-or-later. Code that came from other projects keeps the license it came with. The name, path, version, license, and upstream site for each one are in [media/doc/3rd Party Files.txt](media/doc/3rd%20Party%20Files.txt). Wine code is LGPL-2.1 or later unless a file header says otherwise. The modules are listed in [media/doc/WINESYNC.txt](media/doc/WINESYNC.txt).

Upstream is [reactos/reactos](https://github.com/reactos/reactos). This fork is [taveraos/taveraos](https://github.com/taveraos/taveraos).

VerOS NT is my trademark. Windows is a registered trademark of Microsoft Corporation. ReactOS is a registered trademark of ReactOS Deutschland e.V. I use those names to say what this is compared with and where the tree came from. I don't use their logos. Microsoft and the ReactOS Project have not licensed or endorsed this fork.
