
//
//   PDEV
//  DESCRIPTION
//    This structure will contain all state information
//    required to maintain the video device
//  ACCESS
//    Allocated from non-paged pool by the GDI

typedef struct _PDEV
{
  HANDLE  KMDriver;
  HDEV  GDIDevHandle;
} PDEV, *PPDEV;


