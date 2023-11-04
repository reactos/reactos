/** @file
  UGA Draw protocol from the EFI 1.10 specification.

  Abstraction of a very simple graphics device.

  Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __UGA_DRAW_H__
#define __UGA_DRAW_H__


#define EFI_UGA_DRAW_PROTOCOL_GUID \
  { \
    0x982c298b, 0xf4fa, 0x41cb, {0xb8, 0x38, 0x77, 0xaa, 0x68, 0x8f, 0xb8, 0x39 } \
  }

typedef struct _EFI_UGA_DRAW_PROTOCOL EFI_UGA_DRAW_PROTOCOL;

/**
  Return the current video mode information.

  @param  This                  The EFI_UGA_DRAW_PROTOCOL instance.
  @param  HorizontalResolution  The size of video screen in pixels in the X dimension.
  @param  VerticalResolution    The size of video screen in pixels in the Y dimension.
  @param  ColorDepth            Number of bits per pixel, currently defined to be 32.
  @param  RefreshRate           The refresh rate of the monitor in Hertz.

  @retval EFI_SUCCESS           Mode information returned.
  @retval EFI_NOT_STARTED       Video display is not initialized. Call SetMode ()
  @retval EFI_INVALID_PARAMETER One of the input args was NULL.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_UGA_DRAW_PROTOCOL_GET_MODE)(
  IN  EFI_UGA_DRAW_PROTOCOL *This,
  OUT UINT32                *HorizontalResolution,
  OUT UINT32                *VerticalResolution,
  OUT UINT32                *ColorDepth,
  OUT UINT32                *RefreshRate
  );

/**
  Set the current video mode information.

  @param  This                 The EFI_UGA_DRAW_PROTOCOL instance.
  @param  HorizontalResolution The size of video screen in pixels in the X dimension.
  @param  VerticalResolution   The size of video screen in pixels in the Y dimension.
  @param  ColorDepth           Number of bits per pixel, currently defined to be 32.
  @param  RefreshRate          The refresh rate of the monitor in Hertz.

  @retval EFI_SUCCESS          Mode information returned.
  @retval EFI_NOT_STARTED      Video display is not initialized. Call SetMode ()

**/
typedef
EFI_STATUS
(EFIAPI *EFI_UGA_DRAW_PROTOCOL_SET_MODE)(
  IN  EFI_UGA_DRAW_PROTOCOL *This,
  IN  UINT32                HorizontalResolution,
  IN  UINT32                VerticalResolution,
  IN  UINT32                ColorDepth,
  IN  UINT32                RefreshRate
  );

typedef struct {
  UINT8 Blue;
  UINT8 Green;
  UINT8 Red;
  UINT8 Reserved;
} EFI_UGA_PIXEL;

typedef union {
  EFI_UGA_PIXEL Pixel;
  UINT32        Raw;
} EFI_UGA_PIXEL_UNION;

///
/// Enumration value for actions of Blt operations.
///
typedef enum {
  EfiUgaVideoFill,          ///< Write data from the  BltBuffer pixel (SourceX, SourceY)
                            ///< directly to every pixel of the video display rectangle
                            ///< (DestinationX, DestinationY) (DestinationX + Width, DestinationY + Height).
                            ///< Only one pixel will be used from the BltBuffer. Delta is NOT used.
                            
  EfiUgaVideoToBltBuffer,   ///< Read data from the video display rectangle
                            ///< (SourceX, SourceY) (SourceX + Width, SourceY + Height) and place it in
                            ///< the BltBuffer rectangle (DestinationX, DestinationY )
                            ///< (DestinationX + Width, DestinationY + Height). If DestinationX or
                            ///< DestinationY is not zero then Delta must be set to the length in bytes
                            ///< of a row in the BltBuffer.
                            
  EfiUgaBltBufferToVideo,   ///< Write data from the  BltBuffer rectangle
                            ///< (SourceX, SourceY) (SourceX + Width, SourceY + Height) directly to the
                            ///< video display rectangle (DestinationX, DestinationY)
                            ///< (DestinationX + Width, DestinationY + Height). If SourceX or SourceY is
                            ///< not zero then Delta must be set to the length in bytes of a row in the
                            ///< BltBuffer.
  
  EfiUgaVideoToVideo,       ///< Copy from the video display rectangle (SourceX, SourceY)
                            ///< (SourceX + Width, SourceY + Height) .to the video display rectangle
                            ///< (DestinationX, DestinationY) (DestinationX + Width, DestinationY + Height).
                            ///< The BltBuffer and Delta  are not used in this mode.
                            
  EfiUgaBltMax              ///< Maxmimum value for enumration value of Blt operation. If a Blt operation
                            ///< larger or equal to this enumration value, it is invalid.
} EFI_UGA_BLT_OPERATION;

/**
    Blt a rectangle of pixels on the graphics screen.

    @param[in] This          - Protocol instance pointer.
    @param[in] BltBuffer     - Buffer containing data to blit into video buffer. This
                               buffer has a size of Width*Height*sizeof(EFI_UGA_PIXEL)
    @param[in] BltOperation  - Operation to perform on BlitBuffer and video memory
    @param[in] SourceX       - X coordinate of source for the BltBuffer.
    @param[in] SourceY       - Y coordinate of source for the BltBuffer.
    @param[in] DestinationX  - X coordinate of destination for the BltBuffer.
    @param[in] DestinationY  - Y coordinate of destination for the BltBuffer.
    @param[in] Width         - Width of rectangle in BltBuffer in pixels.
    @param[in] Height        - Hight of rectangle in BltBuffer in pixels.
    @param[in] Delta         - OPTIONAL

    @retval EFI_SUCCESS           - The Blt operation completed.
    @retval EFI_INVALID_PARAMETER - BltOperation is not valid.
    @retval EFI_DEVICE_ERROR      - A hardware error occured writting to the video buffer.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_UGA_DRAW_PROTOCOL_BLT)(
  IN  EFI_UGA_DRAW_PROTOCOL                   * This,
  IN  EFI_UGA_PIXEL                           * BltBuffer, OPTIONAL
  IN  EFI_UGA_BLT_OPERATION                   BltOperation,
  IN  UINTN                                   SourceX,
  IN  UINTN                                   SourceY,
  IN  UINTN                                   DestinationX,
  IN  UINTN                                   DestinationY,
  IN  UINTN                                   Width,
  IN  UINTN                                   Height,
  IN  UINTN                                   Delta         OPTIONAL
  );

///
/// This protocol provides a basic abstraction to set video modes and 
/// copy pixels to and from the graphics controller's frame buffer. 
///
struct _EFI_UGA_DRAW_PROTOCOL {
  EFI_UGA_DRAW_PROTOCOL_GET_MODE  GetMode;
  EFI_UGA_DRAW_PROTOCOL_SET_MODE  SetMode;
  EFI_UGA_DRAW_PROTOCOL_BLT       Blt;
};

extern EFI_GUID gEfiUgaDrawProtocolGuid;

#endif
