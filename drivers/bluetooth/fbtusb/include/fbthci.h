#ifndef _HCI_H_
#define _HCI_H_

#include <windows.h>

#include "fbthw.h"
#include "fbtHciDefs.h"

// Number of overlapped requests to have pending in the driver
#define HCI_NUMBER_OF_OVERLAPPED_LISTENS	MAXIMUM_WAIT_OBJECTS-1

// HCI Abstraction layer
class CHci;
typedef struct
{
	PFBT_HCI_EVENT_HEADER		pEvent;
	DWORD						dwLength;
	CHci						*pThis;

} HCI_EVENT, *PHCI_EVENT;

class CHci : public CBTHW
{
public:
	CHci(void);
	virtual ~CHci(void);

	virtual DWORD StartEventListener(void);
	virtual DWORD StopEventListener(void);
    virtual DWORD OnEvent(PFBT_HCI_EVENT_HEADER pEvent, DWORD Length);

	static LPCTSTR GetEventText(BYTE Event);
	static LPCTSTR GetStatusText(BYTE Status);
	static LPCTSTR GetManufacturerName(USHORT Company);

    virtual DWORD OnCommandComplete(BYTE NumHCICommandPackets, USHORT CommandOpcode, BYTE *Parameters, DWORD ParameterLength);
    virtual DWORD OnCommandStatus(BYTE Status, BYTE NumHCICommandPackets, USHORT CommandOpcode);

    virtual DWORD OnConnectionRequest(BYTE BD_ADDR[FBT_HCI_BDADDR_SIZE], ULONG ClassOfDevice[FBT_HCI_DEVICE_CLASS_SIZE], BYTE LinkType);
    virtual DWORD OnConnectionComplete(BYTE Status, USHORT ConnectionHandle, BYTE BD_ADDR[FBT_HCI_BDADDR_SIZE], BYTE LinkType, BYTE EncryptionMode);
    virtual DWORD OnDisconnectionComplete(BYTE Status, USHORT ConnectionHandle, BYTE Reason);

    virtual DWORD OnInquiryComplete(BYTE Status, BYTE NumResponses);
    virtual DWORD OnInquiryResult(BYTE NumResponses, BYTE BD_ADDR[FBT_HCI_VARIABLE_SIZE][FBT_HCI_BDADDR_SIZE], BYTE PageScanRepetitionMode[FBT_HCI_VARIABLE_SIZE], BYTE PageScanPeriodMode[FBT_HCI_VARIABLE_SIZE], BYTE PageScanMode[FBT_HCI_VARIABLE_SIZE], BYTE ClassOfDevice[FBT_HCI_VARIABLE_SIZE][FBT_HCI_DEVICE_CLASS_SIZE], USHORT ClockOffset[FBT_HCI_VARIABLE_SIZE]);

    virtual DWORD OnRemoteNameRequestComplete(BYTE Status, BYTE BD_ADDR[FBT_HCI_BDADDR_SIZE], BYTE RemoteName[FBT_HCI_NAME_SIZE]);

    virtual DWORD OnRoleChange(BYTE Status, BYTE BD_ADDR[FBT_HCI_BDADDR_SIZE], BYTE Role);

    virtual DWORD OnPINCodeRequest(BYTE BD_ADDR[FBT_HCI_BDADDR_SIZE]);

	virtual DWORD OnLinkKeyNotification(BYTE BD_ADDR[FBT_HCI_BDADDR_SIZE], BYTE LinkKey[FBT_HCI_LINK_KEY_SIZE]);
    virtual DWORD OnLinkKeyRequest(BYTE BD_ADDR[FBT_HCI_BDADDR_SIZE]);

	virtual DWORD OnAuthenticationComplete(BYTE Status, USHORT ConnectionHandle);

	virtual DWORD OnReadLocalNameComplete(BYTE Status, BYTE Name[FBT_HCI_NAME_SIZE]);

    virtual DWORD OnUnknown(PFBT_HCI_EVENT_HEADER pEvent, DWORD Length);

    virtual DWORD SendReset(void);

    virtual DWORD SendInquiry(ULONG LAP, BYTE InquiryLength, BYTE NumResponses);
    virtual DWORD SendInquiryCancel(void);

    virtual DWORD SendReadBDADDR(void);

    virtual DWORD SendWriteScanEnable(BYTE ScanEnable);

    virtual DWORD SendWriteAuthenticationEnable(BYTE ScanEnable);

    virtual DWORD SendSetEventFilter(BYTE FilterType,
                                     BYTE FilterConditionType,
                                     BYTE Condition[FBT_HCI_MAX_CONDITION_SIZE],
									 BYTE ConditionBytes);

    virtual DWORD SendReadClassOfDevice(void);

    virtual DWORD SendWriteClassOfDevice(BYTE ClassOfDevice[FBT_HCI_DEVICE_CLASS_SIZE]);

    virtual DWORD SendCreateConnection(BYTE		BD_ADDR[FBT_HCI_BDADDR_SIZE],
                                       USHORT	PacketType,
                                       BYTE		PageScanRepetitionMode,
                                       BYTE		PageScanMode,
                                       USHORT	ClockOffset,
                                       BYTE		AllowRoleSwitch);

    virtual DWORD SendAcceptConnectionRequest(BYTE BD_ADDR[FBT_HCI_BDADDR_SIZE], BYTE RoleSwitch);

    virtual DWORD SendDisconnect(USHORT ConnectionHandle, BYTE  Reason);

    virtual DWORD SendWriteLinkSupervisionTimeout(USHORT ConnectionHandle, USHORT LinkSupervisionTimeout);

    virtual DWORD SendWritePageTimeout(USHORT PageTimeout);

    virtual DWORD SendRemoteNameRequest(BYTE BD_ADDR[FBT_HCI_BDADDR_SIZE], BYTE PageScanRepetitionMode, BYTE PageScanMode, USHORT ClockOffset);

    virtual DWORD SendReadLocalName(void);

    virtual DWORD SendChangeLocalName(BYTE Name[FBT_HCI_NAME_SIZE]);

	virtual DWORD SendSwitchRole(BYTE BD_ADDR[FBT_HCI_BDADDR_SIZE], BYTE Role);

	virtual DWORD SendPINCodeRequestReply(BYTE BD_ADDR[FBT_HCI_BDADDR_SIZE], BYTE PINCodeLength, BYTE PINCode[FBT_HCI_PIN_CODE_SIZE]);
	virtual DWORD SendPINCodeRequestNegativeReply(BYTE BD_ADDR[FBT_HCI_BDADDR_SIZE]);

	virtual DWORD SendLinkKeyRequestReply(BYTE BD_ADDR[FBT_HCI_BDADDR_SIZE], BYTE LinkKey[FBT_HCI_LINK_KEY_SIZE]);
	virtual DWORD SendLinkKeyRequestNegativeReply(BYTE BD_ADDR[FBT_HCI_BDADDR_SIZE]);

	virtual DWORD SendReadLocalVersionInformation(void);

	DWORD CompareBDADDRs(BYTE BD_ADDR1[FBT_HCI_BDADDR_SIZE], BYTE BD_ADDR2[FBT_HCI_BDADDR_SIZE]);

protected:
	friend static DWORD CALLBACK Listener(LPVOID pContext);
	friend static DWORD EventHandler(PFBT_HCI_EVENT_HEADER pEvent, DWORD Length);

	virtual DWORD SendHciCommand(PFBT_HCI_CMD_HEADER lpCommand, DWORD dwBufferSize);

    DWORD SendListenForEvent(OVERLAPPED *pOverlapped, BYTE *pEventBuffer);

    HANDLE	m_hStopListeningEvent;
    HANDLE	m_hListenerReadyEvent;
    HANDLE	m_hListenerThread;

    DWORD	m_dwListenerThreadId;

    OVERLAPPED  m_Overlappeds[HCI_NUMBER_OF_OVERLAPPED_LISTENS];
    BYTE		m_pEventBuffers[HCI_NUMBER_OF_OVERLAPPED_LISTENS][FBT_HCI_EVENT_MAX_SIZE];

};


#endif // _HCI_H_
