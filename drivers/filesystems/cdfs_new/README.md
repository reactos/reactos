<!---
    name: CDFS File System Driver
    platform: WDM
    language: cpp
    category: FileSystem
    description: The CD-ROM file system driver (cdfs) sample is a file system driver for removable media.
    samplefwlink: http://go.microsoft.com/fwlink/p/?LinkId=617642
--->


CDFS File System Driver
=======================

The CD-ROM file system driver (cdfs) sample is a sample file system driver that you can use to write new file systems.

Cdfs is a read-only file system that addresses various issues such as accessing data on disk, interacting with the cache manager, and handling various I/O operations such as opening files, performing reads on a file, retrieving information on a file, and performing various control operations on the file system. The Cdfs file system is included with the Microsoft Windows operating system.

## Universal Windows Driver Compliant
This sample builds a Universal Windows Driver. It uses only APIs and DDIs that are included in OneCoreUAP.

