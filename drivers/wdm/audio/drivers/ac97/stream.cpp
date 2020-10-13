// Every debug output has "Modulname text"
#define STR_MODULENAME "AC97 Stream: "

#include "shared.h"
#include "miniport.h"

/*****************************************************************************
 * CMiniportStream::SetContentId
 *****************************************************************************
 * This routine gets called by drmk.sys to pass the content to the driver.
 * The driver has to enforce the rights passed.
 */
STDMETHODIMP_(NTSTATUS) CMiniportStream::SetContentId
(
    _In_  ULONG       contentId,
    _In_  PCDRMRIGHTS drmRights
)
{
    PAGED_CODE ();

    DOUT (DBG_PRINT, ("[CMiniportStream::SetContentId]"));

    UNREFERENCED_PARAMETER(contentId);

    //
    // If "drmRights->DigitalOutputDisable" is set, we need to disable S/P-DIF.
    // Currently, we don't have knowledge about the S/P-DIF interface. However,
    // in case you expanded the driver with S/P-DIF features you need to disable
    // S/P-DIF or fail SetContentId. If you have HW that has S/P-DIF turned on
    // by default and you don't know how to turn off (or you cannot do that)
    // then you must fail SetContentId.
    //
    // In our case, we assume the codec has no S/P-DIF or disabled S/P-DIF by
    // default, so we can ignore the flag.
    //
    // Store the copyright flag. We have to disable PCM recording if it's set.
    //
    if (!Miniport->AdapterCommon->GetMiniportTopology ())
    {
        DOUT (DBG_ERROR, ("Topology pointer not set!"));
        return STATUS_UNSUCCESSFUL;
    }
    else
    {
        Miniport->AdapterCommon->GetMiniportTopology ()->
            SetCopyProtectFlag (drmRights->CopyProtect);
    }

    //
    // We assume that if we can enforce the rights, that the old content
    // will be destroyed. We don't need to store the content id since we
    // have only one playback channel, so we are finished here.
    //

    return STATUS_SUCCESS;
}
