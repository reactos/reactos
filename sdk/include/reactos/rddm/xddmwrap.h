#pragma once

NTSTATUS
NTAPI
XDDMWrapSetVidPnSourceOwner(_In_ const D3DKMT_SETVIDPNSOURCEOWNER* unnamedParam1);

NTSTATUS
NTAPI
XDDMWrapCloseAdapter(_In_ const D3DKMT_CLOSEADAPTER *desc );

NTSTATUS
NTAPI
XDDMWrapCreateDevice(_Inout_ D3DKMT_CREATEDEVICE* unnamedParam1);

NTSTATUS
NTAPI
XDDMWrapDestroyDevice(_In_ const D3DKMT_DESTROYDEVICE* unnamedParam1);
