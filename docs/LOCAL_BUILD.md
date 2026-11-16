# Local build

This fork is built and booted on Windows. `ARCH=amd64` is the only supported OS build. 32-bit applications run on that boot. The daily configuration is Clang 18.1.8, amd64, Debug, target `bootcd`.

## Tools

- ReactOS Build Environment 2.2.1. The silent installer returned exit code 2, so the same files were extracted to `D:\RosBE`. The scripts also look for `C:\RosBE`. `Build-Bootcd.ps1` looks first at `tools\rosbe` in the parent of this checkout, which is the layout written by `Setup-BuildEnvironment.ps1`. It loads that checkout's `env.cmd`, which puts `tools\clang\bin`, `tools\rosbe\amd64\bin`, and `tools\rosbe\bin` on `PATH` and sets `_ROSBE_ROSSCRIPTDIR`. `rosbe-gcc-env.cmd` only adds the MinGW bins. The package is the unmodified 2.2.1 release.
- Clang 18.1.8, the Windows package `clang+llvm-18.1.8-x86_64-pc-windows-msvc`. `Build-Bootcd.ps1` uses `tools\clang\bin\clang.exe` in that same parent folder, then `D:\LLVM-18\clang+llvm-18.1.8-x86_64-pc-windows-msvc\bin\clang.exe`, then `C:\Program Files\LLVM\bin\clang.exe`. RosBE's amd64 GCC was built without plugin support, so the amd64 build uses this Clang.
- QEMU 11.1.0. `Test-Boot.ps1` uses `tools\qemu\qemu-system-x86_64.exe` in the parent of this checkout when `Setup-BuildEnvironment.ps1` extracted it there, otherwise `C:\Program Files\qemu\qemu-system-x86_64.exe`. It is not on `PATH`. Boot tests pass `-accel tcg` and record `accelerator=tcg`. Do not pass `-accel whpx`. Windows 10 22H2 and VerOS NT boots pass `-m 8192 -smp 4`.

If a real boot never prints the ReactOS version banner within 20 minutes, switch to a QEMU build that includes WHPX. `WinHvPlatform.dll` is already on this machine.

## Commands

From the source root:

```powershell
.\tools\qemu-harness\Test-Plugin.ps1
.\tools\pipeline\Build-Bootcd.ps1
.\tools\pipeline\Test-Boot.ps1
.\tools\pipeline\Invoke-Pipeline.ps1
```

`Build-Bootcd.ps1` configures an out-of-tree `build-amd64/` directory and builds `bootcd` with Ninja at `-j 8`. `Test-Boot.ps1` boots `build-amd64\bootcd.iso`. It sets `BISON_PKGDATADIR` and `M4` before the toolchain script, because the extracted RosBE bison otherwise looks for data under its original build path. A parallel MinGW link can fail with `Cannot open output file: Invalid argument`, and `pefixup` can fail with `Can't open` when the image is still being written. The script retries the build up to 8 times, and Ninja resumes at the failed target. `Test-Boot.ps1` boots that ISO for 20 minutes, writes a serial log, and loads `tools/qemu-harness/abi.dll` when the DLL is present. `Invoke-Pipeline.ps1` builds, boots, writes a crash report, and refreshes `dashboard/data/snapshot.json`. When that snapshot records a plugin return or bugcheck for a kernel export or a driver ABI export, the same run copies those category percents into `veros/dashboard/latest.json`. `differential-tests/syscall-diff/Publish-Dashboard.ps1` in the parent checkout copies them again after it refreshes the syscall scores.

`build-amd64/` and `output/` are gitignored. Open `dashboard/index.html` after a snapshot refresh. The page reads `dashboard/data/snapshot.js`, so it works from the filesystem.

## Harness

`abi.dll` logs `SYSENTER`, `SYSCALL`, and `INT 2E`, plus `CALL`/`JMP` targets listed in a map file (`module!Export 0xaddress`). A single instruction that fault-loops is written at most four times so a triple fault cannot fill the disk. Pass the map with `Test-Boot.ps1 -Map path`. Export addresses come from the ReactOS build map for that ISO. The plugin does not contain Windows source.

`Test-Plugin.ps1` builds the DLL and runs a 512-byte guest that executes `INT 2E`, `SYSENTER`, and a `CALL` to `ntoskrnl.exe!IoCallDriver`. It does not need a ReactOS ISO.
