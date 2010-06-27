/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            include/reactos/drivers/pci/pci.h
 * PURPOSE:         Internal, Shared, PCI Definitions
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

//
// PCI Hack Flags
//
#define PCI_HACK_LOCK_RESOURCES                             0x0000000000000004LL
#define PCI_HACK_NO_ENUM_AT_ALL                             0x0000000000000008LL
#define PCI_HACK_ENUM_NO_RESOURCE                           0x0000000000000010LL
#define PCI_HACK_AVOID_D1D2_FOR_SLD                         0x0000000000000020LL
#define PCI_HACK_NEVER_DISCONNECT                           0x0000000000000040LL
#define PCI_HACK_DONT_DISABLE                               0x0000000000000080LL
#define PCI_HACK_MULTIFUNCTION                              0x0000000000000100LL
#define PCI_HACK_UNUSED_200                                 0x0000000000000200LL
#define PCI_HACK_IGNORE_NON_STICKY_ISA                      0x0000000000000400LL
#define PCI_HACK_UNUSED_800                                 0x0000000000000800LL
#define PCI_HACK_DOUBLE_DECKER                              0x0000000000001000LL
#define PCI_HACK_ONE_CHILD                                  0x0000000000002000LL
#define PCI_HACK_PRESERVE_COMMAND                           0x0000000000004000LL
#define PCI_HACK_DEFAULT_CARDBUS_WINDOWS                    0x0000000000008000LL
#define PCI_HACK_CB_SHARE_CMD_BITS                          0x0000000000010000LL
#define PCI_HACK_IGNORE_ROOT_TOPOLOGY                       0x0000000000020000LL
#define PCI_HACK_SUBTRACTIVE_DECODE                         0x0000000000040000LL
#define PCI_HACK_NO_EXPRESS_CAP                             0x0000000000080000LL
#define PCI_HACK_NO_ASPM_FOR_EXPRESS_LINK                   0x0000000000100000LL
#define PCI_HACK_CLEAR_INT_DISABLE_FOR_MSI                  0x0000000000200000LL
#define PCI_HACK_NO_SUBSYSTEM                               0x0000000000400000LL
#define PCI_HACK_COMMAND_REWRITE                            0x0000000000800000LL
#define PCI_HACK_AVOID_HARDWARE_ISA_BIT                     0x0000000001000000LL
#define PCI_HACK_FORCE_BRIDGE_WINDOW_ALIGNMENT              0x0000000002000000LL
#define PCI_HACK_NOT_MSI_HT_CONVERTER                       0x0000000004000000LL
#define PCI_HACK_PCI_HACK_SBR_ON_LINK_STATE_CHANGE          0x0000000008000000LL
#define PCI_HACK_PCI_HACK_LINK_DISABLE_ON_SLOT_PWRDN        0x0000000010000000LL
#define PCI_HACK_NO_PM_CAPS                                 0x0000000020000000LL
#define PCI_HACK_DONT_DISABLE_DECODES                       0x0000000040000000LL
#define PCI_HACK_NO_SUBSYSTEM_AFTER_D3                      0x0000000080000000LL
#define PCI_HACK_VIDEO_LEGACY_DECODE                        0x0000000100000000LL
#define PCI_HACK_FAKE_CLASS_CODE                            0x0000000200000000LL
#define PCI_HACK_UNUSED_40000000                            0x0000000400000000LL
#define PCI_HACK_UNUSED_80000000                            0x0000000800000000LL
#define PCI_HACK_FAIL_QUERY_REMOVE                          0x0000001000000000LL
#define PCI_HACK_CRITICAL_DEVICE                            0x0000002000000000LL
#define PCI_HACK_UNUSED_4000000000                          0x0000004000000000LL
#define PCI_HACK_BROKEN_SUBTRACTIVE_DECODE                  0x0000008000000000LL
#define PCI_HACK_NO_REVISION_AFTER_D3                       0x0000010000000000LL
#define PCI_HACK_ENABLE_MSI_MAPPING                         0x0000020000000000LL
#define PCI_HACK_DISABLE_PM_DOWNSTREAM_PCI_BRIDGE           0x0000040000000000LL

/* EOF */
