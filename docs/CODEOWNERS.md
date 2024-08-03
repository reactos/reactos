# List of Maintainers for the ReactOS Project
This file's purpose is to give newcomers to the project the responsible developers when submitting a pull request on GitHub, or opening a bug report on Jira.

This file will notably establish who is responsible for a specific area of ReactOS. Being a maintainer means the following:
- that person has good knowledge in the area
- that person is able to enforce consistency in the area
- that person may be available for giving help in the area
- that person has push access on the repository

Being a maintainer does not mean the following:
- that person is dedicated to the area
- that person is working full-time on the area/on ReactOS
- that person is paid
- that person is always available

**We have no supported (paid) areas in ReactOS.**

When submitting a pull request on GitHub and looking for reviewers, look at that file and ask for a review from some of the people listed in the matching area, also, assign the pull request to the M person. Don't ask for a review from all the listed reviewers. Also, when submitted a pull request on GitHub, rules defined in [CONTRIBUTING.md](CONTRIBUTING.md) apply. And if the maintainer is not available and reviewers approved the pull request, developers feeling confident can merge the pull request. Note that reviewers do not necessarily have push access to the repository. When submitting a bug report on Jira, if you want to be sure to have a developer with skills in that area, write @nick from M people.

There should be one and only one primary maintainer per area.

In case of 3rd party code (also referred as upstream), the maintainer is responsible of updating periodically the source code and of managing local patches. He is not here to upstream code on your behalf. As responsible, he may refuse a local patch if you did not try to upstream your changes.

If you want to get listed in this file, either put yourself in the file and push it, or open a pull request. You can also ask a person who has push access to add yourself.

This file uses GitHub's format for specifying code owners.
- Lines starting with # are comment lines.
- All other lines specify a path / file (wildcards allowed) followed by the GitHub user name(s) of the code owners. See https://help.github.com/en/articles/about-code-owners

## ReactOS Components

### 3rd Party File Format Libraries
Maintainer: @ThFabba
Reviewers: None
Status: Upstream
Comments: See media/doc/3rd Party Files.txt

|Component                               |Code Owner(s)|
|----------------------------------------|-------------|
|/dll/3rdparty/libjpeg/                  |@ThFabba     |
|/dll/3rdparty/libpng/                   |@ThFabba     |
|/dll/3rdparty/libtiff/                  |@ThFabba     |
|/dll/3rdparty/libxslt/                  |@ThFabba     |
|/sdk/include/reactos/libs/libjpeg/      |@ThFabba     |
|/sdk/include/reactos/libs/libmpg123/    |@ThFabba     |
|/sdk/include/reactos/libs/libpng/       |@ThFabba     |
|/sdk/include/reactos/libs/libtiff/      |@ThFabba     |
|/sdk/include/reactos/libs/libxml/       |@ThFabba     |
|/sdk/include/reactos/libs/libxslt/      |@ThFabba     |
|/sdk/lib/3rdparty/libmpg123/            |@ThFabba     |
|/sdk/lib/3rdparty/libsamplerate/        |@ThFabba     |
|/sdk/lib/3rdparty/libxml2/              |@ThFabba     |

### ACPI
Maintainer: None
Reviewers: @ThFabba
Status: Maintained
Comments: None

|Component           |Code Owner(s)|
|--------------------|-------------|
|/drivers/bus/acpi/  |@ThFabba     |
|/hal/halx86/acpi/   |@ThFabba     |

### ACPICA Library
Maintainer: @ThFabba
Reviewers: None
Status: Upstream
Comments: None

|Component                   |Code Owner(s)|
|----------------------------|-------------|
|/drivers/bus/acpi/acpica/   |@ThFabba     |

### Apisets
Maintainer: @learn-more
Reviewers: None
Status: Maintained
Comments: None

|Component          |Code Owner(s)|
|-------------------|-------------|
|/sdk/lib/apisets/  |@learn-more  |

### Application Compatibility Subsystem
Maintainer: @learn-more
Reviewers: None
Status: Maintained
Comments: None

|Component               |Code Owner(s)|
|------------------------|-------------|
|/dll/appcompat/         |@learn-more  |
|/dll/shellext/acppage/  |@learn-more  |
|/ntoskrnl/ps/apphelp.c  |@learn-more  |
|/sdk/tools/xml2sdb/     |@learn-more  |

### Cache Manager
Maintainer: @HeisSpiter
Reviewers: None
Status: Maintained
Comments: None

|Component                           |Code Owner(s)|
|------------------------------------|-------------|
|/modules/rostests/kmtests/ntos_cc/  |@HeisSpiter  |
|/ntoskrnl/cc/                       |@HeisSpiter  |

### Cache Manager Rewrite
Maintainer: None
Reviewers: None
Status: Abandoned
Comments: None

|Component         |Code Owner(s)|
|------------------|-------------|
|/ntoskrnl/cache/  |             |

### CMake Build Scripts
Maintainer: None
Reviewers: @learn-more
Status: Maintained
Comments: None

|Component    |Code Owner(s)|
|-------------|-------------|
|/sdk/cmake/  |             |
|*.cmake      |             |

### File Patch API
Maintainer: @learn-more
Reviewers: None
Status: Maintained
Comments: None

|Component             |Code Owner(s)|
|----------------------|-------------|
|/dll/win32/mspatcha/  |@learn-more  |

### File Systems
Maintainer: @HeisSpiter
Reviewers: None
Status: Maintained
Comments: Also see "Upstream File Systems"

|Component               |Code Owner(s)|
|------------------------|-------------|
|/drivers/filesystems/   |@HeisSpiter  |
|/sdk/lib/fslib/         |@HeisSpiter  |

### Filesystem Filter Manager
Maintainer: @gedmurphy
Reviewers: None
Status: Maintained
Comments: None

|Component                 |Code Owner(s)|
|--------------------------|-------------|
|/drivers/filters/fltmgr/  |@gedmurphy   |

### File Systems Run Time Library
Maintainer: @HeisSpiter
Reviewers: None
Status: Maintained
Comments: None

|Component                                |Code Owner(s)|
|-----------------------------------------|-------------|
|/modules/rostests/kmtests/ntos_fsrtl/    |@HeisSpiter  |
|/modules/rostests/kmtests/novp_fsrtl/    |@HeisSpiter  |
|/ntoskrnl/fsrtl/                         |@HeisSpiter  |
|/sdk/lib/drivers/ntoskrnl_vista/fsrtl.c  |@HeisSpiter  |

### Freeloader
Maintainer: None
Reviewers: @tkreuzer
Status: Maintained
Comments: None

|Component               |Code Owner(s)           |
|------------------------|------------------------|
|/boot/freeldr/freeldr/  |@tkreuzer, @Extravert-ir|

### HAL / APIC
Maintainer: @tkreuzer
Reviewers: None
Status: Maintained
Comments: None

|Component         |Code Owner(s)|
|------------------|-------------|
/hal/halx86/apic/  |@tkreuzer    |

### HID Drivers
Maintainer: None
Reviewers: @ThFabba
Status: Maintained
Comments: None

|Component      |Code Owner(s)|
|---------------|-------------|
|/drivers/hid/  |@ThFabba     |

### Kernel
Maintainer: None
Reviewers: @HeisSpiter, @ThFabba, @tkreuzer
Status: Maintained
Comments: None

|Component   |Code Owner(s)                   |
|------------|--------------------------------|
|/ntoskrnl/  |@HeisSpiter, @ThFabba, @tkreuzer|

### Mbed TLS
Maintainer: @ThFabba
Reviewers: None
Status: Upstream
Comments: See media/doc/3rd Party Files.txt

|Component                           |Code Owner(s)|
|------------------------------------|-------------|
|/dll/3rdparty/mbedtls/              |@ThFabba     |
|/sdk/include/reactos/libs/mbedtls/  |@ThFabba     |

### Mount Point Manager
Maintainer: @HeisSpiter
Reviewers: None
Status: Maintained
Comments: None

|Component                   |Code Owner(s)|
|----------------------------|-------------|
|/drivers/storage/mountmgr/  |@HeisSpiter  |

### Network Drivers
Maintainer: None
Reviewers: @ThFabba
Status: Maintained
Comments: None

|Component          |Code Owner(s)|
|-------------------|-------------|
|/drivers/network/  |@ThFabba     |

### Intel PRO/1000 NIC Family Driver
Maintainer: None
Reviewers: @ThFabba, @Extravert-ir
Status: Maintained
Comments: None

|Component                   |Code Owner(s)          |
|----------------------------|-----------------------|
|/drivers/network/dd/e1000/  |@ThFabba, @Extravert-ir|

### Network File Systems Kernel Libraries
Maintainer: @HeisSpiter
Reviewers: None
Status: Maintained
Comments: None

|Component                   |Code Owner(s)|
|----------------------------|-------------|
|/sdk/lib/drivers/rdbsslib/  |@HeisSpiter  |
|/sdk/lib/drivers/rxce/      |@HeisSpiter  |

### NTDLL
Maintainer: @HeisSpiter
Reviewers: @HeisSpiter, @learn-more, @ThFabba, @tkreuzer
Status: Maintained
Comments: None

|Component    |Code Owner(s)                                |
|-------------|---------------------------------------------|
|/dll/ntdll/  |@HeisSpiter, @learn-more, @ThFabba, @tkreuzer|

### Printing
Maintainer: @ColinFinck
Reviewers: None
Status: Maintained
Comments: None

|Component           |Code Owner(s)|
|--------------------|-------------|
|/win32ss/printing/  |@ColinFinck  |

### ReactOS API Tests
Maintainer: None
Reviewers: @learn-more, @ThFabba
Status: Maintained
Comments: None

|Component                    |Code Owner(s)|
|-----------------------------|-------------|
|/modules/rostests/apitests/  |             |

### ReactOS Kernel-Mode Tests
Maintainer: @ThFabba
Reviewers: None
Status: Maintained
Comments: None

|Component                   |Code Owner(s)|
|----------------------------|-------------|
|/modules/rostests/kmtests/  |@ThFabba     |

### ROS Internals Tools
Maintainer: @HeisSpiter
Reviewers: None
Status: Maintained
Comments: None

|Component                                    |Code Owner(s)|
|---------------------------------------------|-------------|
|/modules/rosapps/applications/rosinternals/  |@HeisSpiter  |

### Run-Time Library (RTL)
Maintainer: None
Reviewers: @HeisSpiter, @learn-more, @ThFabba, @tkreuzer
Status: Maintained
Comments: None

|Component      |Code Owner(s)                                |
|---------------|---------------------------------------------|
|/sdk/lib/rtl/  |@HeisSpiter, @learn-more, @ThFabba, @tkreuzer|

### Security Manager
Maintainer: @GeoB99
Reviewers: None
Status: Maintained
Comments: None

|Component                           |Code Owner(s)|
|------------------------------------|-------------|
|/modules/rostests/kmtests/ntos_se/  |@GeoB99      |
|/ntoskrnl/se/                       |@GeoB99      |

### Shell
Maintainer: None
Reviewers: @learn-more, @yagoulas
Status: Maintained
Comments: None

|Component              |Code Owner(s)         |
|-----------------------|----------------------|
|/base/shell/explorer/  |@learn-more, @yagoulas|
|/base/shell/rshell/    |@learn-more, @yagoulas|
|/dll/win32/browseui/   |@learn-more, @yagoulas|
|/dll/win32/shell32/    |@learn-more, @yagoulas|

### Shell Extensions
Maintainer: None
Reviewers: @learn-more
Status: Maintained
Comments: None

|Component       |Code Owner(s)|
|----------------|-------------|
|/dll/shellext/  |@learn-more  |

### UniATA
Maintainer: @ThFabba
Reviewers: None
Status: Upstream
Comments: None

|Component                     |Code Owner(s)|
|------------------------------|-------------|
|/drivers/storage/ide/uniata/  |@ThFabba     |

### Upstream File Systems
Maintainer: @HeisSpiter
Reviewers: None
Status: Upstream
Comments: None

|Component                      |Code Owner(s)|
|-------------------------------|-------------|
|/base/services/nfsd/           |@HeisSpiter  |
|/dll/np/nfs/                   |@HeisSpiter  |
|/dll/shellext/shellbtrfs/      |@HeisSpiter  |
|/drivers/filesystems/btrfs/    |@HeisSpiter  |
|/drivers/filesystems/cdfs/     |@HeisSpiter  |
|/drivers/filesystems/ext2/     |@HeisSpiter  |
|/drivers/filesystems/fastfat/  |@HeisSpiter  |
|/drivers/filesystems/nfs/      |@HeisSpiter  |
|/media/doc/README.FSD          |@HeisSpiter  |
|/sdk/lib/fslib/btrfslib/       |@HeisSpiter  |
|/sdk/lib/fslib/ext2lib/        |@HeisSpiter  |
|/sdk/lib/fslib/vfatlib/check/  |@HeisSpiter  |

### USB Drivers
Maintainer: @ThFabba
Reviewers: @Extravert-ir
Status: Maintained
Comments: None

|Component                             |Code Owner(s)          |
|--------------------------------------|-----------------------|
/drivers/usb/                          |@ThFabba, @Extravert-ir|
/sdk/include/reactos/drivers/usbport/  |@ThFabba, @Extravert-ir|

### Virtual CD-ROM
Maintainer: @HeisSpiter
Reviewers: None
Status: Maintained
Comments: None

|Component                                       |Code Owner(s)|
|------------------------------------------------|-------------|
|/modules/rosapps/applications/cmdutils/vcdcli/  |@HeisSpiter  |
|/modules/rosapps/applications/vcdcontroltool/   |@HeisSpiter  |
|/modules/rosapps/drivers/vcdrom/                |@HeisSpiter  |

### Virtual Floppy Disk
Maintainer: @HeisSpiter
Reviewers: None
Status: Upstream
Comments: None

|Component                                       |Code Owner(s)|
|------------------------------------------------|-------------|
|/modules/rosapps/applications/cmdutils/vfdcmd/  |@HeisSpiter  |
|/modules/rosapps/drivers/vfd/                   |@HeisSpiter  |

### Win32 file functions
Maintainer: @HeisSpiter
Reviewers: None
Status: Maintained
Comments: None

|Component                         |Code Owner(s)|
|----------------------------------|-------------|
|/dll/win32/kernel32/client/file/  |@HeisSpiter  |

### Windows Network File Systems functions
Maintainer: @HeisSpiter
Reviewers: None
Status: Upstream
Comments: None

|Component              |Code Owner(s)|
|-----------------------|-------------|
|/dll/win32/mpr/wnet.c  |@HeisSpiter  |

### Wine Tests
Maintainer: None
Reviewers: @ThFabba
Status: Upstream
Comments: None

|Component                     |Code Owner(s)|
|------------------------------|-------------|
|/modules/rostests/winetests/  |@ThFabba     |

### Zlib
Maintainer: @ThFabba
Reviewers: None
Status: Upstream
Comments: See media/doc/3rd Party Files.txt

|Component                        |Code Owner(s)|
|---------------------------------|-------------|
|/sdk/include/reactos/libs/zlib/  |@ThFabba     |
|/sdk/lib/3rdparty/zlib/          |@ThFabba     |

### x64 Port <!-- Keep this at the bottom -->
Maintainer: @tkreuzer
Reviewers: None
Status: Maintained
Comments: None

|Component                                    |Code Owner(s)|
|---------------------------------------------|-------------|
|amd64/                                       |@tkreuzer    |
|/boot/freeldr/freeldr/arch/realmode/amd64.S  |@tkreuzer    |

### Translations
This is the list of translation teams in ReactOS GitHub organization.
If you want to be part of one - hit us at https://chat.reactos.org/

|Locale |Translation Team        |
|-------|------------------------|
|de-DE  |@reactos/lang-german    |
|es-ES  |@reactos/lang-spanish   |
|et-EE  |@reactos/lang-estonian  |
|fr-FR  |@reactos/lang-french    |
|he-IL  |@reactos/lang-hebrew    |
|hi-IN  |@reactos/lang-hindi     |
|hu-HU  |@reactos/lang-hungarian |
|id-ID  |@reactos/lang-indonesian|
|it-IT  |@reactos/lang-italian   |
|nl-NL  |@reactos/lang-dutch     |
|pl-PL  |@reactos/lang-polish    |
|pt-BR  |@reactos/lang-portuguese|
|pt-PT  |@reactos/lang-portuguese|
|ro-RO  |@reactos/lang-romanian  |
|ru-RU  |@reactos/lang-russian   |
|tr-TR  |@reactos/lang-turkish   |
|uk-UA  |@reactos/lang-ukrainian |
|zh-CN  |@reactos/lang-chinese   |
|zh-HK  |@reactos/lang-chinese   |
|zh-TW  |@reactos/lang-chinese   |
