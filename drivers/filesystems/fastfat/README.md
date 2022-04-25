---
page_type: sample
description: "A file system driver based on the Windows inbox FastFAT file system used as a model for new file systems."
languages:
- cpp
products:
- windows
- windows-wdk
---

# fastfat File System Driver

The *fastfat* sample is file system driver that you can use as a model to write new file systems.

*fastfat* is a complete file system that addresses various issues such as storing data on disk, interacting with the cache manager, and handling various I/O operations such as file creation, performing read/writes on a file, setting information on a file, and performing control operations on the file system.

## Universal Windows Driver Compliant

This sample builds a Universal Windows Driver. It uses only APIs and DDIs that are included in OneCoreUAP.

## Build the sample

You can build the sample in two ways: using Microsoft Visual Studio or the command line (*MSBuild*).

### Building a Driver Using Visual Studio

You build a driver the same way you build any project or solution in Visual Studio. When you create a new driver project using a Windows driver template, the template defines a default (active) project configuration and a default (active) solution build configuration. When you create a project from existing driver sources or convert existing driver code that was built with previous versions of the WDK, the conversion process preserves the target version information (operating systems and platform).

The default Solution build configuration is **Debug** and **Win32**.

#### To select a configuration and build a driver

1. Open the driver project or solution in Visual Studio (find fastfat.sln or fastfat.vcxproj).

1. Right-click the solution in the **Solutions Explorer** and select **Configuration Manager**.

1. From the **Configuration Manager**, select the **Active Solution Configuration** (for example, Debug or Release) and the **Active Solution Platform** (for example, Win32) that correspond to the type of build you are interested in.

1. From the Build menu, click **Build Solution** (Ctrl+Shift+B).

### Building a Driver Using the Command Line (MSBuild)

You can build a driver from the command line using the Visual Studio Command Prompt window and the Microsoft Build Engine (MSBuild.exe) Previous versions of the WDK used the Windows Build utility (Build.exe) and provided separate build environment windows for each of the supported build configurations. You can now use the Visual Studio Command Prompt window for all build configurations.

#### To select a configuration and build a driver or an application

1. Open a Visual Studio Command Prompt window at the **Start** screen. From this window you can use MsBuild.exe to build any Visual Studio project by specifying the project (.VcxProj) or solutions (.Sln) file.

1. Navigate to the project directory and enter the **MSbuild** command for your target. For example, to perform a clean build of a Visual Studio driver project called *filtername*.vcxproj, navigate to the project directory and enter the following MSBuild command:

`msbuild /t:clean /t:build .\\fastfat.vcxproj`

## Installation

No INF file is provided with this sample because the *fastfat* file system driver (fastfat.sys) is already part of the Windows operating system. You can build a private version of this file system and use it as a replacement for the native driver.
