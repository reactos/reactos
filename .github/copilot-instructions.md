# ReactOS Development Guide for AI Assistants

This guide provides essential information for contributing to ReactOS, an open-source operating system compatible with Windows applications and drivers.

## Architecture Overview

ReactOS follows a modular architecture similar to Windows NT. Understanding the roles of these key components is crucial.

- **Kernel (`ntoskrnl/`)**: The core of the operating system. It manages the CPU, memory (`mm/`), processes and threads (`ps/`), I/O (`io/`), and other low-level system resources. The code here is highly sensitive and follows strict patterns.
- **Hardware Abstraction Layer (`hal/`)**: Isolates the kernel and drivers from hardware-specific details, allowing ReactOS to run on different hardware platforms (e.g., `halx86/`, `halarm/`).
- **Win32 Subsystem (`win32ss/`)**: Implements the Windows user-mode API, including the graphics device interface (`gdi/`) and user interface components (`user/`). It communicates with the kernel to perform privileged operations.
- **NTDLL (`dll/ntdll/`)**: The lowest-level user-mode library. It provides the system call interface that user-mode applications and subsystems use to communicate with the kernel.
- **Drivers (`drivers/`)**: Contains drivers for filesystems, network, storage, USB, and more.
- **User-Mode Applications and DLLs**: Located in `base/applications/` and `dll/win32/`, these are the standard utilities and libraries that come with the OS.

## Build System

The project uses CMake and Ninja for building. The ReactOS Build Environment (RosBE) is the recommended toolchain.

- **Configuration**: Run `configure.cmd` in the root directory to set up a build environment (e.g., in `output-VS-amd64/`).
- **Building the entire project**: Navigate to the build directory and run `ninja`.
- **Building a specific module**: Run `ninja <modulename>`. For example, `ninja ntoskrnl`.
- **Creating a bootable ISO**: Run `ninja bootcd` to generate `bootcd.iso`.

For more details, refer to the [Building ReactOS](https://reactos.org/wiki/Building_ReactOS) wiki page.

## Coding Conventions (C/C++)

ReactOS has a strict coding style. Adhere to these rules for any new or modified code. Refer to `CODING_STYLE.md` for the complete guide.

- **Indentation & Formatting**:
    - Use 4 spaces for indentation, not tabs.
    - Line width must be at most 100 characters.
    - Braces (`{` and `}`) for blocks must be on their own lines (Allman style).
- **Naming Conventions**:
    - Use `PascalCase` for functions and variables (e.g., `MyFunction`, `LocalVariable`).
    - Do not use `camelCase` or `snake_case`.
    - Hungarian notation is optional but may be present in older code.
    - Boolean variables should be prefixed with verbs like `Is` or `Did` (e.g., `IsValid`, `DidComplete`).
- **Constants and Macros**:
    - Use `NULL` for null pointers.
    - Use `TRUE` and `FALSE` for boolean values.
- **Headers**:
    - Use `#pragma once` for header guards.
- **File Header**:
    - All source files must begin with a standard header block specifying the `PROJECT`, `LICENSE`, `PURPOSE`, and `COPYRIGHT`.
    ```c
    /*
     * PROJECT:     ReactOS Kernel
     * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
     * PURPOSE:     Brief description of the file's purpose
     * COPYRIGHT:   Copyright 2023 Your Name <you@example.com>
     */
    ```
- **Commits**:
    - Changes that only reformat code should be in separate commits from logic changes. Prefix the commit message with `[FORMATTING]`.
