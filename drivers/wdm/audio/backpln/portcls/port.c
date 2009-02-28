#include "private.h"

NTSTATUS StringFromCLSID(
	const CLSID *id,	/* [in] GUID to be converted */
	LPWSTR idstr		/* [out] pointer to buffer to contain converted guid */
) {
  static const char hex[] = "0123456789ABCDEF";
  WCHAR *s;
  int	i;

  swprintf(idstr, L"{%08X-%04X-%04X-%02X%02X-",
	  id->Data1, id->Data2, id->Data3,
	  id->Data4[0], id->Data4[1]);
  s = &idstr[25];

  /* 6 hex bytes */
  for (i = 2; i < 8; i++) {
    *s++ = hex[id->Data4[i]>>4];
    *s++ = hex[id->Data4[i] & 0xf];
  }

  *s++ = '}';
  *s++ = '\0';

  return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
PcNewPort(
    OUT PPORT* OutPort,
    IN  REFCLSID ClassId)
{
    NTSTATUS Status;
    WCHAR Buffer[100];

    DPRINT1("PcNewPort entered\n");

    if (!OutPort)
    {
        DPRINT("PcNewPort was supplied a NULL OutPort parameter\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (IsEqualGUIDAligned(ClassId, &CLSID_PortMidi))
        Status = NewPortMidi(OutPort);
    else if (IsEqualGUIDAligned(ClassId, &CLSID_PortDMus))
        Status = NewPortDMus(OutPort);
    else if (IsEqualGUIDAligned(ClassId, &CLSID_PortTopology))
        Status = NewPortTopology(OutPort);
    else if (IsEqualGUIDAligned(ClassId, &CLSID_PortWaveCyclic))
        Status = NewPortWaveCyclic(OutPort);
    else if (IsEqualGUIDAligned(ClassId, &CLSID_PortWavePci))
        Status = NewPortWavePci(OutPort);
    else
    {

        StringFromCLSID(ClassId, Buffer);
        DPRINT1("unknown interface %S\n", Buffer);

        Status = STATUS_NOT_SUPPORTED;
        return Status;
     }
    DPRINT("PcNewPort Status %lx\n", Status);

    return Status;
}
