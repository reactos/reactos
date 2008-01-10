// Simple header for defining architecture-dependent settings

#ifndef _REACTOS_ARCHITECTURE_H
#define _REACTOS_ARCHITECTURE_H

#if defined(_M_IX86)
#   define ARCH_CD_ROOT             "I386"
#elif defined(_M_PPC)
#   define ARCH_CD_ROOT             "PPC"
#else
#   error Unsupported architecture
#endif

#endif
