#pragma once

#include "hardware.h"
#include "hwiface.h"
#include "physmem.h"
#include <usb.h>
#include <ntddk.h>

BOOLEAN
SubmitControlTransfer(PEHCI_HOST_CONTROLLER hcd,
                      PUSB_DEFAULT_PIPE_SETUP_PACKET CtrlSetup,
                      PVOID TransferBuffer,
                      ULONG TransferBufferLength,
                      PIRP IrpToComplete);

VOID
BuildSetupPacketFromURB(PEHCI_HOST_CONTROLLER hcd,
                        PURB Urb,
                        PUSB_DEFAULT_PIPE_SETUP_PACKET CtrlSetup);

