#ifndef _ROUND_TRIP_HCI_H_
#define _ROUND_TRIP_HCI_H_

#include "fbtHciLocal.h"

// Complete round trip HCI abstraction
// 1. Send Command
// 2. Wait for Command status / Command complete
// 3. Wait fo event
class CHciRoundTrip : public CHciLocal
{
public:

	CHciRoundTrip();
	virtual ~CHciRoundTrip();

	virtual DWORD QueueEvent(BYTE EventCode, LPVOID pParameters, DWORD dwParameterLength);
	virtual DWORD WaitForEvent();

    virtual DWORD OnEvent(PFBT_HCI_EVENT_HEADER pEvent, DWORD Length);

	virtual DWORD ReadBDADDR(BYTE *BDADDR);
	virtual DWORD ReadClassOfDevice(BYTE *ClassOfDevice);
    virtual DWORD ReadLocalName(BYTE *Name);
    virtual DWORD CreateConnection(BYTE  BD_ADDR[FBT_HCI_BDADDR_SIZE],
								   USHORT PacketType,
								   BYTE  PageScanRepetitionMode,
								   BYTE  PageScanMode,
								   USHORT ClockOffset,
								   BYTE  AllowRoleSwitch,
								   USHORT &ConnectionHandle);

	virtual DWORD Disconnect(USHORT ConnectionHandler, BYTE Reason);
	virtual DWORD SwitchRole(BYTE BD_ADDR[FBT_HCI_BDADDR_SIZE], BYTE Role);
    virtual DWORD RemoteNameRequest(BYTE BD_ADDR[FBT_HCI_BDADDR_SIZE],
    								BYTE PageScanRepetitionMode,
    								BYTE PageScanMode,
    								USHORT ClockOffset,
    								BYTE Name[FBT_HCI_NAME_SIZE]);

protected:
	BYTE	m_PendingEvent;
	LPVOID	m_pEventParameters;
	DWORD	m_dwEventParameterLength;

	HANDLE	m_hEventSignal;

};


#endif // _ROUND_TRIP_HCI_H_
