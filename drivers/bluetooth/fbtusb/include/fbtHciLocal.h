#ifndef _LOCAL_HCI_H_
#define _LOCAL_HCI_H_

#include "fbtHci.h"

#define MAX_QUEUED_COMMANDS 100

typedef struct _QueuedCommand
{
	USHORT	nCommand;
	BYTE	*pResultBuffer;
	DWORD	dwBufferSize;
	HANDLE	hEvent;

} QueuedCommand, *PQueuedCommand;

// Local HCI command abstraction
// 1. Send Command
// 2. Wait for Command status / Command complete
class CHciLocal : public CHci
{
public:
	CHciLocal(void);
	virtual ~CHciLocal(void);

	virtual int				QueueCommand(USHORT nCommand, BYTE *pResultBuffer=NULL, DWORD dwBufferSize=0);
	virtual int				QueueCommandStatus(USHORT nCommand);
	virtual void			DeQueueCommand(int nSlot);
	virtual DWORD			ClearQueue(void);
	virtual PQueuedCommand	GetQueuedCommand(int nSlot);

	virtual DWORD WaitForCommandComplete(int nSlot);
	virtual DWORD WaitForCommandStatus(int nSlot, BYTE &nStatus);

	virtual DWORD SendReset(void);
	virtual DWORD SendWriteClassOfDevice(BYTE ClassOfDevice[FBT_HCI_DEVICE_CLASS_SIZE]);
	virtual DWORD SendSetEventFilter(
					BYTE nFilterType,
					BYTE nFilterConditionType,
					BYTE nCondition[FBT_HCI_MAX_CONDITION_SIZE],
					BYTE nConditionBytes);

	virtual DWORD SendInquiry(ULONG nLAP, BYTE nInquiryLength, BYTE nNumResponses);
	virtual DWORD SendInquiryCancel(void);
    virtual DWORD SendCreateConnection(BYTE		BD_ADDR[FBT_HCI_BDADDR_SIZE],
                                       USHORT	nPacketType,
                                       BYTE		nPageScanRepetitionMode,
                                       BYTE		nPageScanMode,
                                       USHORT	nClockOffset,
                                       BYTE		nAllowRoleSwitch);

	virtual DWORD SendDisconnect(USHORT nConnectionHandle, BYTE nReason);
	virtual DWORD SendSwitchRole(BYTE BD_ADDR[FBT_HCI_BDADDR_SIZE], BYTE nRole);
	virtual DWORD SendRemoteNameRequest(BYTE BD_ADDR[FBT_HCI_BDADDR_SIZE], BYTE nPageScanRepetitionMode, BYTE nPageScanMode, USHORT nClockOffset);
	virtual DWORD SendReadLocalVersionInformation(FBT_HCI_READ_LOCAL_VERSION_INFORMATION_COMPLETE &CommandComplete);

    virtual DWORD OnEvent(PFBT_HCI_EVENT_HEADER pEvent, DWORD dwLength);

    virtual DWORD CommandCompleteHandler(USHORT nCommand, BYTE *pParameters, DWORD dwParameterLength);
    virtual DWORD CommandStatusHandler(BYTE nStatus, USHORT nCommand);

protected:
	virtual int		FindCommandSlot(USHORT nCommand);

	QueuedCommand		m_QueuedCommands[MAX_QUEUED_COMMANDS];
	CRITICAL_SECTION	m_QueueCriticalSection;

};


#endif // _LOCAL_HCI_H_
