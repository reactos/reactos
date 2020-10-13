#ifndef _STREAM_H_
#define _STREAM_H_

class CMiniportStream : public IDrmAudioStream
{
public:
    CMiniport*                  Miniport;
    PSERVICEGROUP               ServiceGroup;   // service group helps with DPCs
    ULONG                       CurrentRate;    // Current Sample Rate

public:

    /*************************************************************************
     * Include IDrmAudioStream (public/exported) methods.
     *************************************************************************
     */
    IMP_IDrmAudioStream;

    //
    // This method is called when the device changes power states.
    //
    virtual NTSTATUS PowerChangeNotify
    (
        IN  POWER_STATE NewState
    ) = 0;

    //
    // Return the current sample rate.
    //
    ULONG GetCurrentSampleRate (void)
    {
        return CurrentRate;
    }
};

#endif
