

#pragma once

// 0: 0x398000 - called from LSASRV!LsapInitLsa
#define IOCTL_KSEC_REGISTER_LSA_PROCESS \
    CTL_CODE(FILE_DEVICE_KSEC, 0x00, METHOD_BUFFERED, FILE_WRITE_ACCESS)

// 1: 0x390004 - called from tcpip!InitIsnGenerator
#define IOCTL_KSEC_1 \
    CTL_CODE(FILE_DEVICE_KSEC, 0x01, METHOD_BUFFERED, FILE_ANY_ACCESS)

// 2: 0x390008 - called from SystemFunction036 aka RtlGenRandom via RandomFillBuffer
#define IOCTL_KSEC_RANDOM_FILL_BUFFER \
    CTL_CODE(FILE_DEVICE_KSEC, 0x02, METHOD_BUFFERED, FILE_ANY_ACCESS)

// 3: 0x39000E - called from SystemFunction040 aka RtlEncryptMemory with OptionFlags == 0
#define IOCTL_KSEC_ENCRYPT_SAME_PROCESS \
    CTL_CODE(FILE_DEVICE_KSEC, 0x03, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)

// 4: 0x390012 - called from SystemFunction041 aka RtlDecryptMemory with OptionFlags == 0
#define IOCTL_KSEC_DECRYPT_SAME_PROCESS \
    CTL_CODE(FILE_DEVICE_KSEC, 0x04, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)

// 5: 0x390016 - called from SystemFunction040 aka RtlEncryptMemory with OptionFlags == 1
#define IOCTL_KSEC_ENCRYPT_CROSS_PROCESS \
    CTL_CODE(FILE_DEVICE_KSEC, 0x05, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)

// 6: 0x39001A - called from SystemFunction041 aka RtlDecryptMemory with OptionFlags == 1
#define IOCTL_KSEC_DECRYPT_CROSS_PROCESS \
    CTL_CODE(FILE_DEVICE_KSEC, 0x06, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)

// 7: 0x39001E - called from SystemFunction040 aka RtlEncryptMemory with OptionFlags == 2
#define IOCTL_KSEC_ENCRYPT_SAME_LOGON \
    CTL_CODE(FILE_DEVICE_KSEC, 0x07, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)

// 8: 0x390022 - called from SystemFunction041 aka RtlDecryptMemory with OptionFlags == 2
#define IOCTL_KSEC_DECRYPT_SAME_LOGON \
    CTL_CODE(FILE_DEVICE_KSEC, 0x08, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)

// e: 0x390038 - see http://wiki.mysmartlogon.com/Kernel_mode_SSP
#define IOCTL_KSEC_REGISTER_EXTENSION \
    CTL_CODE(FILE_DEVICE_KSEC, 0x0e, METHOD_BUFFERED, FILE_ANY_ACCESS)
