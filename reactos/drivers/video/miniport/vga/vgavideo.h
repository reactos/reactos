#include <ddk/ntddk.h>
#include <ddk/ntddvid.h>

#define VGA_NORMAL 0
#define VGA_AND    8
#define VGA_OR     16
#define VGA_XOR    24

#define MISC         (PUCHAR)0x3c2
#define SEQ          (PUCHAR)0x3c4
#define SEQDATA      (PUCHAR)0x3c5
#define CRTC         (PUCHAR)0x3d4
#define CRTCDATA     (PUCHAR)0x3d5
#define GRAPHICS     (PUCHAR)0x3ce
#define GRAPHICSDATA (PUCHAR)0x3cf
#define ATTRIB       (PUCHAR)0x3c0
#define ATTRIBREAD   (PUCHAR)0x3c1
#define STATUS       (PUCHAR)0x3da
#define PELMASK      (PUCHAR)0x3c6
#define PELINDEX     (PUCHAR)0x3c8
#define PELDATA      (PUCHAR)0x3c9
#define FEATURE      (PUCHAR)0x3da

typedef struct _VGA_REGISTERS
{
   UCHAR CRT[24];
   UCHAR Attribute[21];
   UCHAR Graphics[9];
   UCHAR Sequencer[5];
   UCHAR Misc;
} VGA_REGISTERS, *PVGA_REGISTERS;
