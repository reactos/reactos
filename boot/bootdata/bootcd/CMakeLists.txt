set(UNATTENDED_BOOTCD "no" CACHE STRING "Whether to build a self-installing bootcd (yes/no)")

function(add_unattended_bootcd_option _var _default _doc)
    if (UNATTENDED_BOOTCD STREQUAL "yes")
        set(${_var} "${_default}" CACHE STRING ${_doc})
    endif()
endfunction()

add_unattended_bootcd_option(UNATTENDED_BOOTCD_DestinationDiskNumber "0"
    "The Disk Number ReactOS will be installed on during bootcd unattended setup")
add_unattended_bootcd_option(UNATTENDED_BOOTCD_DestinationPartitionNumber "1"
    "The Partition Number ReactOS will be installed on during bootcd unattended setup")
add_unattended_bootcd_option(UNATTENDED_BOOTCD_InstallationDirectory "ReactOS"
    "The directory ReactOS will be installed in during bootcd unattended setup")
add_unattended_bootcd_option(UNATTENDED_BOOTCD_MBRInstallType "2"
    "MBR installation type during bootcd unattended setup: 0: skip, 1: On floppy, 2: On HDD")
add_unattended_bootcd_option(UNATTENDED_BOOTCD_FullName "MyName"
    "The user name used for unattended bootcd installation")
add_unattended_bootcd_option(UNATTENDED_BOOTCD_OrgName ""
    "The organisation name used for unattended bootcd installation (None if empty)")
add_unattended_bootcd_option(UNATTENDED_BOOTCD_ComputerName "MYCOMPUTERNAME"
    "The computer name used for unattended bootcd installation")
add_unattended_bootcd_option(UNATTENDED_BOOTCD_AdminPassword ""
    "The administrator password used for unattended bootcd installation (None if empty)")
add_unattended_bootcd_option(UNATTENDED_BOOTCD_FormatPartition "1"
    "Whether to format the partition during bootcd unattended setup. 0: Don't format, 1: Format)")
add_unattended_bootcd_option(UNATTENDED_BOOTCD_AutoPartition "1"
    "Whether to automatically create the partition during bootcd unattended setup. 0: Don't create partition, 1: Create partition")
add_unattended_bootcd_option(UNATTENDED_BOOTCD_FsType "0"
    "The filesystem used during bootcd unattended setup. 0: FAT, 1: BTRFS")
add_unattended_bootcd_option(UNATTENDED_BOOTCD_DisableGeckoInst "no"
    "Whether to disable the Gecko module installation. (yes/no)")
add_unattended_bootcd_option(UNATTENDED_BOOTCD_LocaleID "409"
    "The locale used during bootcd unattended setup. See hivesys.inf for values")
add_unattended_bootcd_option(UNATTENDED_BOOTCD_ProductOption "0"
    "The product installed during bootcd unattended setup. 0: Server, 1: WorkStation")

if (UNATTENDED_BOOTCD STREQUAL "yes")
    configure_file(unattend.inf.in unattend.inf @ONLY)
    add_cd_file(FILE ${CMAKE_CURRENT_BINARY_DIR}/unattend.inf DESTINATION reactos NO_CAB FOR bootcd)
endif()

