
#pragma once

/* Ioctl called by GetInterfaceInfo. Returns IP_INTERFACE_INFO structure. */
#define IOCTL_IP_INTERFACE_INFO _TCP_CTL_CODE(0x10, METHOD_BUFFERED, FILE_ANY_ACCESS)
