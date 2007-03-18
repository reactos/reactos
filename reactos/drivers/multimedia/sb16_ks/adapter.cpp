#include "sb16.h"

class CAdapterSB16 :
    public IAdapterSB16,
    public IAdapterPowerManagement,
    public CUnknown
{
    public :
        DECLARE_STD_UNKNOWN();
        DEFINE_STD_CONSTRUCTOR(CAdapterSB16);
        ~CAdapterSB16();
    
        STDMETHODIMP_(NTSTATUS) Init(
            IN  PRESOURCELIST ResourceList,
            IN  PDEVICE_OBJECT DeviceObject);
    
        STDMETHODIMP_(PINTERRUPTSYNC) GetInterruptSync(void);
    
        STDMETHODIMP_(void) SetWaveMiniport(
            IN  PWAVEMINIPORTSB16 Miniport);
    
        STDMETHODIMP_(BYTE) Read(void);
    
        STDMETHODIMP_(BOOLEAN) Write(
            IN  BYTE Value);
    
        STDMETHODIMP_(NTSTATUS) Reset(void);
    
        STDMETHODIMP_(void) SetMixerValue(
            IN  BYTE Index,
            IN  BYTE Value);
    
        STDMETHODIMP_(BYTE) GetMixerValue(
            IN  BYTE Index);
    
        STDMETHODIMP_(void) ResetMixer(void);
    
        //IMP_IAdapterPowerManagement;
};


NTSTATUS
NewAdapter(
    OUT PUNKNOWN* Unknown,
    IN  REFCLSID,
    IN  PUNKNOWN UnknownOuter OPTIONAL,
    IN  POOL_TYPE PoolType)
{
    STD_CREATE_BODY_( CAdapterSB16, Unknown, UnknownOuter, PoolType, PADAPTERSB16 );
}


NTSTATUS CAdapterSB16::Init(
    IN  PRESOURCELIST ResourceList,
    IN  PDEVICE_OBJECT DeviceObject)
{
    return STATUS_UNSUCCESSFUL;
}

CAdapterSB16::~CAdapterSB16()
{
}
/*
STDMETHODIMP
CAdapterSB16::NonDelegatingQueryInterface(
    REFIID Interface,
    PVOID* Object)
{
    return STATUS_UNSUCCESSFUL;
}
*/
STDMETHODIMP_(PINTERRUPTSYNC)
CAdapterSB16::GetInterruptSync()
{
    return NULL;
}

STDMETHODIMP_(BYTE)
CAdapterSB16::Read()
{
    return 0x00;
}

STDMETHODIMP_(BOOLEAN)
CAdapterSB16::Write(
    IN  BYTE Value)
{
    return FALSE;
}

STDMETHODIMP_(NTSTATUS)
CAdapterSB16::Reset()
{
    return STATUS_UNSUCCESSFUL;
}

STDMETHODIMP_(void)
CAdapterSB16::SetMixerValue(
    IN  BYTE Index,
    IN  BYTE Value)
{
}

STDMETHODIMP_(BYTE)
CAdapterSB16::GetMixerValue(
    IN  BYTE Index)
{
    return 0x00;
}

STDMETHODIMP_(void)
CAdapterSB16::ResetMixer()
{
}


STDMETHODIMP_(void)
SetWaveMiniport(
     IN  PWAVEMINIPORTSB16 Miniport)
{
}

/*
STDMETHODIMP_(void)
CAdapterSB16::PowerChangeState(
    IN  POWER_STATE NewState)
{
}

STDMETHODIMP_(NTSTATUS)
CAdapterSB16::QueryPowerChangeState(
    IN  POWER_STATE NewStateQuery)
{
    return STATUS_UNSUCCESSFUL;
}

STDMETHODIMP_(NTSTATUS)
CAdapterSB16::QueryDeviceCapabilities(
    IN  PDEVICE_CAPABILITIES)
{
    return STATUS_UNSUCCESSFUL;
}
*/
