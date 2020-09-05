/********************************************************************************
**    Copyright (c) 1998-2000 Microsoft Corporation. All Rights Reserved.
**
**       Portions Copyright (c) 1998-1999 Intel Corporation
**
********************************************************************************/

/* The file prophnd.cpp was reviewed by LCA in June 2011 and is acceptable for use by Microsoft. */

// Every debug output has "Modulname text".
static char STR_MODULENAME[] = "AC97 Property handler: ";

#include "mintopo.h"

// These are the values passed to the property handler in the instance
// parameter that normally represents the channel.
const LONG CHAN_LEFT   = 0;
const LONG CHAN_RIGHT  = 1;
const LONG CHAN_MASTER = -1;


// paged code goes here.
#pragma code_seg("PAGE")


/*****************************************************************************
 * CAC97MiniportTopology::SetMultichannelMute
 *****************************************************************************
 * This function is used to set one of the multichannel mutes.
 * It takes the master mono into account when calculating the mute.
 * Make sure that you updated the stNodeCache before calling this function.
 */
NTSTATUS CAC97MiniportTopology::SetMultichannelMute
(
    IN CAC97MiniportTopology *that,
    IN TopoNodes             Mute
)
{
    PAGED_CODE();

    NTSTATUS    ntStatus = STATUS_SUCCESS;
    BOOL        bMute;

    // The first calls to SetMultichannelMute could be without valid
    // cache information because WDMAUD might currently query the nodes
    // (this is at system startup). When WDMAUD queried all nodes then
    // all cache information will be valid.
    if (that->stNodeCache[NODE_VIRT_MASTERMONO_MUTE].bLeftValid &&
        that->stNodeCache[Mute].bLeftValid)
    {
        // We get the master mono mute and the mute that is to change.
        // Then we "or" them and write the value to the register.
        bMute = that->stNodeCache[NODE_VIRT_MASTERMONO_MUTE].lLeft ||
                that->stNodeCache[Mute].lLeft;

        ntStatus = that->AdapterCommon->WriteCodecRegister (
                that->AdapterCommon->GetNodeReg (Mute),
                bMute ? -1 : 0,
                that->AdapterCommon->GetNodeMask (Mute));

        DOUT (DBG_PROPERTY, ("SET: %s -> 0x%x", NodeStrings[Mute], (int)bMute));
    }

    return ntStatus;
}

/*****************************************************************************
 * CAC97MiniportTopology::SetMultichannelVolume
 *****************************************************************************
 * This function is used to set one of the multichannel volumes.
 * It takes the master mono into account when calculating the volume.
 * Make sure that you updated the stNodeCache before calling this function.
 */
NTSTATUS CAC97MiniportTopology::SetMultichannelVolume
(
    IN CAC97MiniportTopology *that,
    IN TopoNodes             Volume
)
{
    PAGED_CODE();

    NTSTATUS        ntStatus = STATUS_SUCCESS;
    LONG            lMinimum, lMaximum;
    ULONG           uStep;
    LONG            lLevel;
    WORD            wRegister;
    
    // The first calls to SetMultichannelMute could be without valid
    // cache information because WDMAUD might currently query the nodes
    // (this is at system startup). When WDMAUD queried all nodes then
    // all cache information will be valid.
    if (that->stNodeCache[NODE_VIRT_MASTERMONO_VOLUME].bLeftValid &&
        that->stNodeCache[NODE_VIRT_MASTERMONO_VOLUME].bRightValid &&
        that->stNodeCache[Volume].bLeftValid &&
        that->stNodeCache[Volume].bRightValid)
    {
        // We get the master mono volume and the volume that is to change.
        // Then we substract master mono from it and write the value to the
        // register.
        lLevel = that->stNodeCache[Volume].lLeft +
                 that->stNodeCache[NODE_VIRT_MASTERMONO_VOLUME].lLeft;

        // Translate the dB value into a register value.

        // Get the registered DB values
        ntStatus = GetDBValues (that->AdapterCommon, Volume,
                                &lMinimum, &lMaximum, &uStep);
        if (!NT_SUCCESS(ntStatus))
            return ntStatus;

        // Check borders.
        if (lLevel < lMinimum) lLevel = lMinimum;
        if (lLevel > lMaximum) lLevel = lMaximum;

        // Calculate the register value
        wRegister = (WORD)(((lMaximum + uStep / 2) - lLevel) / uStep) << 8;

        // Get the right value too.
        lLevel = that->stNodeCache[Volume].lRight +
                 that->stNodeCache[NODE_VIRT_MASTERMONO_VOLUME].lRight;

        // Check borders.
        if (lLevel < lMinimum) lLevel = lMinimum;
        if (lLevel > lMaximum) lLevel = lMaximum;

        // Add it to the register value.
        wRegister += (WORD)(((lMaximum + uStep / 2) - lLevel) / uStep);

        // Write it.
        ntStatus = that->AdapterCommon->WriteCodecRegister (
                that->AdapterCommon->GetNodeReg (Volume),
                wRegister,
                that->AdapterCommon->GetNodeMask (Volume));

        DOUT (DBG_PROPERTY, ("SET: %s -> 0x%x/0x%x", NodeStrings[Volume],
                             that->stNodeCache[Volume].lLeft +
                             that->stNodeCache[NODE_VIRT_MASTERMONO_VOLUME].lLeft,
                             lLevel));
    }

    return ntStatus;
}

/*****************************************************************************
 * CAC97MiniportTopology::GetDBValues
 *****************************************************************************
 * This function is used internally and does no parameter checking. The only
 * parameter that could be invalid is the node.
 * It returns the dB values (means minimum, maximum, step) of the node control,
 * mainly for the property call "basic support". Sure, the node must be a
 * volume or tone control node, not a mute or mux node.
 */
NTSTATUS CAC97MiniportTopology::GetDBValues
(
    IN PADAPTERCOMMON AdapterCommon,
    IN TopoNodes Node,
    OUT LONG *plMinimum,
    OUT LONG *plMaximum,
    OUT ULONG *puStep
)
{
    PAGED_CODE();

    DOUT (DBG_PRINT, ("[CAC97MiniportTopology::GetDBValues]"));
    
    // This is going to be simple. Check the node and return the parameters.
    switch (Node)
    {
        // These nodes could have 5bit or 6bit controls, so we first
        // have to check this.
        case NODE_MASTEROUT_VOLUME:
        case NODE_FRONT_VOLUME:
        case NODE_HPOUT_VOLUME:
        case NODE_SURROUND_VOLUME:
        case NODE_CENTER_VOLUME:
        case NODE_LFE_VOLUME:
        case NODE_VIRT_MONOOUT_VOLUME1:
        case NODE_VIRT_MONOOUT_VOLUME2:
            // needed for the config query
            TopoNodeConfig  config;

            // which node to query?
            config = NODEC_6BIT_MONOOUT_VOLUME;
            if ((Node == NODE_MASTEROUT_VOLUME) || (Node == NODE_FRONT_VOLUME))
                config = NODEC_6BIT_MASTER_VOLUME;
            if (Node == NODE_HPOUT_VOLUME)
                config = NODEC_6BIT_HPOUT_VOLUME;
            if (Node == NODE_SURROUND_VOLUME)
                config = NODEC_6BIT_SURROUND_VOLUME;
            if ((Node == NODE_CENTER_VOLUME) || (Node == NODE_LFE_VOLUME))
                config = NODEC_6BIT_CENTER_LFE_VOLUME;

            // check if we have 6th bit support.
            if (AdapterCommon->GetNodeConfig (config))
            {
                // 6bit control
                *plMaximum = 0;            // 0 dB
                *plMinimum = 0xFFA18000;   // -94.5 dB
                *puStep    = 0x00018000;   // 1.5 dB
            }
            else
            {
                // 5bit control
                *plMaximum = 0;            // 0 dB
                *plMinimum = 0xFFD18000;   // -46.5 dB
                *puStep    = 0x00018000;   // 1.5 dB
            }
            break;

        case NODE_VIRT_MASTERMONO_VOLUME:
            // This virtual control gets added to the speaker volumes.
            // We assume 5-bit volumes.
            *plMaximum = 0;            // 0 dB
            *plMinimum = 0xFFD18000;   // -46.5 dB
            *puStep    = 0x00018000;   // 1.5 dB
            break;

        case NODE_PCBEEP_VOLUME:
            *plMaximum = 0;            // 0 dB
            *plMinimum = 0xFFD30000;   // -45 dB
            *puStep    = 0x00030000;   // 3 dB
            break;

        case NODE_PHONE_VOLUME:
        case NODE_MICIN_VOLUME:
        case NODE_LINEIN_VOLUME:
        case NODE_CD_VOLUME:
        case NODE_VIDEO_VOLUME:
        case NODE_AUX_VOLUME:
        case NODE_WAVEOUT_VOLUME:
            *plMaximum = 0x000C0000;   // 12 dB
            *plMinimum = 0xFFDD8000;   // -34.5 dB
            *puStep    = 0x00018000;   // 1.5 dB
            break;

    
        case NODE_VIRT_MASTER_INPUT_VOLUME1:
        case NODE_VIRT_MASTER_INPUT_VOLUME2:
        case NODE_VIRT_MASTER_INPUT_VOLUME3:
        case NODE_VIRT_MASTER_INPUT_VOLUME4:
        case NODE_VIRT_MASTER_INPUT_VOLUME5:
        case NODE_VIRT_MASTER_INPUT_VOLUME6:
        case NODE_VIRT_MASTER_INPUT_VOLUME7:
        case NODE_VIRT_MASTER_INPUT_VOLUME8:
        case NODE_MIC_VOLUME:
            *plMaximum = 0x00168000;   // 22.5 dB
            *plMinimum = 0;            // 0 dB
            *puStep    = 0x00018000;   // 1.5 dB
            break;

        case NODE_BASS:
        case NODE_TREBLE:
            *plMaximum = 0x000A8000;   // 10.5 dB
            *plMinimum = 0xFFF58000;   // -10.5 dB
            *puStep    = 0x00018000;   // 1.5 dB
            break;

        // These nodes can be fixed or variable.
        // Normally we won't display a fixed volume slider, but if 3D is
        // supported and both sliders are fixed, we have to display one fixed
        // slider for the advanced control panel.
        case NODE_VIRT_3D_CENTER:
        case NODE_VIRT_3D_DEPTH:
            if (AdapterCommon->GetNodeConfig (NODEC_3D_CENTER_ADJUSTABLE) &&
               (Node == NODE_VIRT_3D_CENTER))
            {
                *plMaximum = 0x000F0000;   // +15 dB
                *plMinimum = 0x00000000;   // 0 dB
                *puStep    = 0x00010000;   // 1 dB
            }
            else
            if (AdapterCommon->GetNodeConfig (NODEC_3D_DEPTH_ADJUSTABLE) &&
               (Node == NODE_VIRT_3D_DEPTH))
            {
                *plMaximum = 0x000F0000;   // +15 dB
                *plMinimum = 0x00000000;   // 0 dB
                *puStep    = 0x00010000;   // 1 dB
            }
            else
            {
                // In case it is fixed, read the value and return it.
                WORD wRegister;

                // read the register
                if (!NT_SUCCESS (AdapterCommon->ReadCodecRegister (
                            AdapterCommon->GetNodeReg (Node), &wRegister)))
                    wRegister = 0;      // in case we fail.

                // mask out the control
                wRegister &= AdapterCommon->GetNodeMask (Node);
                if (Node == NODE_VIRT_3D_CENTER)
                {
                    wRegister >>= 8;
                }
                // calculate the dB value.
                *plMaximum = (DWORD)(-wRegister) << 16;    // fixed value
                *plMinimum = (DWORD)(-wRegister) << 16;    // fixed value
                *puStep    = 0x00010000;   // 1 dB
            }
            break;

        case NODE_INVALID:
        default:
            // poeser pupe, tu.
            DOUT (DBG_ERROR, ("GetDBValues: Invalid node requested."));
            return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}
 
/*****************************************************************************
 * CAC97MiniportTopology::PropertyHandler_OnOff
 *****************************************************************************
 * Accesses a KSAUDIO_ONOFF value property.
 * This function (property handler) is called by portcls every time there is a
 * get or a set request for the node. The connection between the node type and
 * the property handler is made in the automation table which is referenced
 * when you register the node.
 * We use this property handler for all nodes that have a checkbox, means mute
 * controls and the special checkbox controls under advanced properties, which
 * are AGC and LOUDNESS.
 */
NTSTATUS CAC97MiniportTopology::PropertyHandler_OnOff
(
    IN      PPCPROPERTY_REQUEST   PropertyRequest
)
{
    PAGED_CODE ();

    ASSERT (PropertyRequest);

    NTSTATUS        ntStatus = STATUS_INVALID_PARAMETER;
    LONG            channel;
    TopoNodes       NodeDef;
    // The major target is the object pointer to the topology miniport.
    CAC97MiniportTopology *that =
        (CAC97MiniportTopology *) PropertyRequest->MajorTarget;

    ASSERT (that);

    DOUT (DBG_PRINT, ("[CAC97MiniportTopology::PropertyHandler_OnOff]"));

    // validate node
    if (PropertyRequest->Node == (ULONG)-1)
        return ntStatus;

    // do the appropriate action for the request.

    // we should do a get or a set?
    if ((PropertyRequest->Verb & KSPROPERTY_TYPE_GET) ||
        (PropertyRequest->Verb & KSPROPERTY_TYPE_SET))
    {
        // validate parameters
        if ((PropertyRequest->InstanceSize < sizeof(LONG)) ||
            (PropertyRequest->ValueSize < sizeof(BOOL)))
            return ntStatus;

        // get channel
        channel = *(PLONG)PropertyRequest->Instance;

        // check channel types, return when unknown
        // as you can see, we have no multichannel support.
        if ((channel != CHAN_LEFT) &&
            (channel != CHAN_RIGHT) &&
            (channel != CHAN_MASTER))
            return ntStatus;

        // We have only mono mutes or On/Off checkboxes although they might control
        // a stereo path. For example, we have a 1-bit mute for CD Volume. This
        // mute controls both CD Volume channels.
        if (channel == CHAN_RIGHT)
            return ntStatus;
        
        // get the buffer
        PBOOL OnOff = (PBOOL)PropertyRequest->Value;

        // Switch on the node id. This is just for parameter checking.
        // If something goes wrong, we will immediately return with
        // ntStatus, which is STATUS_INVALID_PARAMETER.
        switch (NodeDef = that->TransNodeNrToNodeDef (PropertyRequest->Node))
        {
            // These are mutes for mono volumes.
            case NODE_PCBEEP_MUTE:
            case NODE_PHONE_MUTE:
            case NODE_MIC_MUTE:
            case NODE_MICIN_MUTE:
            case NODE_CENTER_MUTE:
            case NODE_LFE_MUTE:
            case NODE_VIRT_MASTERMONO_MUTE:
                // check type
                if (PropertyRequest->PropertyItem->Id != KSPROPERTY_AUDIO_MUTE)
                    return ntStatus;
                break;

            // Well, this one is a AGC, although there is no _automatic_ gain
            // control, but we have a mic boost (which is some kind of manual
            // gain control).
            // The 3D Bypass is a real fake, but that's how you get check boxes
            // on the advanced control panel.
            // Both controls are in a mono path.
            case NODE_VIRT_WAVEOUT_3D_BYPASS:
            case NODE_MIC_BOOST:
                // check type
                if (PropertyRequest->PropertyItem->Id != KSPROPERTY_AUDIO_AGC)
                    return ntStatus;
                break;

            // Simulated Stereo is a AGC control in a stereo path.
            case NODE_SIMUL_STEREO:
                // check type
                if (PropertyRequest->PropertyItem->Id != KSPROPERTY_AUDIO_AGC)
                    return ntStatus;
                break;

            // This is a loudness control in a stereo path. We have to check the
            // type.
            case NODE_LOUDNESS:
                // check type
                if (PropertyRequest->PropertyItem->Id != KSPROPERTY_AUDIO_LOUDNESS)
                    return ntStatus;
                break;

            // For 3D Enable and Mic are exposed as loudness in a mono path.
            case NODE_VIRT_3D_ENABLE:
            case NODE_MIC_SELECT:
                // check type
                if (PropertyRequest->PropertyItem->Id != KSPROPERTY_AUDIO_LOUDNESS)
                    return ntStatus;
                break;

            // These are mutes in a stereo path.
            // Because the HW has only one mute-bit for the stereo channel, we
            // expose the mute as mono. this works in current OS and hopefully
            // will work in future OS.
            case NODE_WAVEOUT_MUTE:
            case NODE_LINEIN_MUTE:
            case NODE_CD_MUTE:
            case NODE_VIDEO_MUTE:
            case NODE_AUX_MUTE:
            case NODE_MASTEROUT_MUTE:
            case NODE_FRONT_MUTE:
            case NODE_SURROUND_MUTE:
            case NODE_HPOUT_MUTE:
                // just check the type.
                if (PropertyRequest->PropertyItem->Id != KSPROPERTY_AUDIO_MUTE)
                    return ntStatus;
                break;

            case NODE_INVALID:
            default:
                // Ooops.
                DOUT (DBG_ERROR, ("PropertyHandler_OnOff: Invalid node requested."));
                return ntStatus;
        }

        // Now, do some action!

        if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
        {
            WORD    wRegister;

            // Read the HW register for the node except NODE_VIRT_MASTERMONO_MUTE,
            // since this is pure virtual.
            if (NodeDef != NODE_VIRT_MASTERMONO_MUTE)
            {
                // get the register and read it.
                ntStatus = that->AdapterCommon->ReadCodecRegister (
                        that->AdapterCommon->GetNodeReg (NodeDef), &wRegister);
                if (!NT_SUCCESS (ntStatus))
                    return ntStatus;
                // Mask out every unused bit.
                wRegister &= that->AdapterCommon->GetNodeMask (NodeDef);
                // Store the value.
                *OnOff = wRegister ? TRUE : FALSE;
            }
            else
            {
                // Assume no mute for master mono.
                *OnOff = FALSE;
            }

            // When we have cache information then return this instead of the
            // calculated value. If we don't, store the calculated value.
            if (that->stNodeCache[NodeDef].bLeftValid)
                *OnOff = that->stNodeCache[NodeDef].lLeft;
            else
            {
                that->stNodeCache[NodeDef].lLeft = *OnOff;
                that->stNodeCache[NodeDef].bLeftValid = (BYTE)-1;
            }
            
            PropertyRequest->ValueSize = sizeof(BOOL);
            DOUT (DBG_PROPERTY, ("GET: %s = 0x%x", NodeStrings[NodeDef], *OnOff));
            
            // Set the return code here.
            ntStatus = STATUS_SUCCESS;
        }
        else    // this must be a set.
        {
            // First update the node cache.
            that->stNodeCache[NodeDef].bLeftValid = (BYTE)-1;
            that->stNodeCache[NodeDef].lLeft = (*OnOff) ? TRUE : FALSE;
            
            //
            // If we have a master mono, then we have to program the speaker
            // mutes a little different.
            // Check for master mono (surround or headphone present) and
            // if one of the speaker mutes is requested.
            //
            if ((that->AdapterCommon->GetPinConfig (PINC_SURROUND_PRESENT) ||
                 that->AdapterCommon->GetPinConfig (PINC_HPOUT_PRESENT)) &&
                ((NodeDef == NODE_VIRT_MASTERMONO_MUTE) || (NodeDef == NODE_LFE_MUTE) ||
                 (NodeDef == NODE_CENTER_MUTE) || (NodeDef == NODE_FRONT_MUTE) ||
                 (NodeDef == NODE_SURROUND_MUTE) || (NodeDef == NODE_HPOUT_MUTE)))
            {
                //
                // For master mono we have to update all speakers.
                //
                if (NodeDef == NODE_VIRT_MASTERMONO_MUTE)
                {
                    // Update all speaker mutes.
                    ntStatus = SetMultichannelMute (that, NODE_FRONT_MUTE);
                    if (that->AdapterCommon->GetPinConfig (PINC_HPOUT_PRESENT))
                        ntStatus = SetMultichannelMute (that, NODE_HPOUT_MUTE);
                    if (that->AdapterCommon->GetPinConfig (PINC_SURROUND_PRESENT))
                        ntStatus = SetMultichannelMute (that, NODE_SURROUND_MUTE);
                    if (that->AdapterCommon->GetPinConfig (PINC_CENTER_LFE_PRESENT))
                    {
                        ntStatus = SetMultichannelMute (that, NODE_CENTER_MUTE);
                        ntStatus = SetMultichannelMute (that, NODE_LFE_MUTE);
                    }
                }
                else    // Update the individual speaker mute.
                {
                    ntStatus = SetMultichannelMute (that, NodeDef);
                }
            }
            else
            {
                //
                // For all other mutes/checkboxes just write the value to the HW.
                //
                ntStatus = that->AdapterCommon->WriteCodecRegister (
                        that->AdapterCommon->GetNodeReg (NodeDef),
                        (*OnOff) ? -1 : 0,
                        that->AdapterCommon->GetNodeMask (NodeDef));
            }

            DOUT (DBG_PROPERTY, ("SET: %s -> 0x%x", NodeStrings[NodeDef], *OnOff));

            // ntStatus was set with the write call! whatever this is, return it.
        }
    }
    
    return ntStatus;
}

/*****************************************************************************
 * CAC97MiniportTopology::BasicSupportHandler
 *****************************************************************************
 * Assists in BASICSUPPORT accesses on level properties.
 * This function is called internally every time there is a "basic support"
 * request on a volume or tone control. The basic support is used to retrieve
 * some information about the range of the control (from - to dB, steps) and
 * which type of control (tone, volume).
 * Basically, this function just calls GetDBValues to get the range information
 * and fills the rest of the structure with some constants.
 */
NTSTATUS CAC97MiniportTopology::BasicSupportHandler
(
    IN      PPCPROPERTY_REQUEST   PropertyRequest
)
{
    PAGED_CODE ();

    ASSERT (PropertyRequest);

    DOUT (DBG_PRINT, ("[CAC97MiniportTopology::BasicSupportHandler]"));

    NTSTATUS ntStatus = STATUS_BUFFER_TOO_SMALL;
    // The major target is the object pointer to the topology miniport.
    CAC97MiniportTopology *that =
        (CAC97MiniportTopology *) PropertyRequest->MajorTarget;

    ASSERT (that);


    // if there is enough space for a KSPROPERTY_DESCRIPTION information
    if (PropertyRequest->ValueSize >= (sizeof(KSPROPERTY_DESCRIPTION)))
    {
        // we return a KSPROPERTY_DESCRIPTION structure.
        PKSPROPERTY_DESCRIPTION PropDesc = (PKSPROPERTY_DESCRIPTION)PropertyRequest->Value;

        PropDesc->AccessFlags = KSPROPERTY_TYPE_BASICSUPPORT |
                                KSPROPERTY_TYPE_GET |
                                KSPROPERTY_TYPE_SET;
        PropDesc->DescriptionSize   = sizeof(KSPROPERTY_DESCRIPTION) +
                                      sizeof(KSPROPERTY_MEMBERSHEADER) +
                                      sizeof(KSPROPERTY_STEPPING_LONG);
        PropDesc->PropTypeSet.Set   = KSPROPTYPESETID_General;
        PropDesc->PropTypeSet.Id    = VT_I4;
        PropDesc->PropTypeSet.Flags = 0;
        PropDesc->MembersListCount  = 1;
        PropDesc->Reserved          = 0;

        // if return buffer can also hold a range description, return it too
        if (PropertyRequest->ValueSize >= (sizeof(KSPROPERTY_DESCRIPTION) +
            sizeof(KSPROPERTY_MEMBERSHEADER) + sizeof(KSPROPERTY_STEPPING_LONG)))
        {
            // fill in the members header
            PKSPROPERTY_MEMBERSHEADER Members = (PKSPROPERTY_MEMBERSHEADER)(PropDesc + 1);

            Members->MembersFlags   = KSPROPERTY_MEMBER_STEPPEDRANGES;
            Members->MembersSize    = sizeof(KSPROPERTY_STEPPING_LONG);
            Members->MembersCount   = 1;
            Members->Flags          = 0;

            // fill in the stepped range
            PKSPROPERTY_STEPPING_LONG Range = (PKSPROPERTY_STEPPING_LONG)(Members + 1);

            ntStatus = GetDBValues (that->AdapterCommon,
                                    that->TransNodeNrToNodeDef (PropertyRequest->Node),
                                    &Range->Bounds.SignedMinimum,
                                    &Range->Bounds.SignedMaximum,
                                    &Range->SteppingDelta);

            Range->Reserved         = 0;

            // set the return value size
            PropertyRequest->ValueSize = sizeof(KSPROPERTY_DESCRIPTION) +
                                         sizeof(KSPROPERTY_MEMBERSHEADER) +
                                         sizeof(KSPROPERTY_STEPPING_LONG);

            DOUT (DBG_PROPERTY, ("BASIC_SUPPORT: %s max=0x%x min=0x%x step=0x%x",
                NodeStrings[that->TransNodeNrToNodeDef (PropertyRequest->Node)],
                Range->Bounds.SignedMaximum, Range->Bounds.SignedMinimum,
                Range->SteppingDelta));
        } else
        {
            // we hadn't enough space for the range information; 
            // set the return value size
            PropertyRequest->ValueSize = sizeof(KSPROPERTY_DESCRIPTION);
        }

        ntStatus = STATUS_SUCCESS;
    }
    else if (PropertyRequest->ValueSize >= sizeof(ULONG))
    {
        // if return buffer can hold a ULONG, return the access flags
        PULONG AccessFlags = (PULONG)PropertyRequest->Value;

        *AccessFlags = KSPROPERTY_TYPE_BASICSUPPORT |
                       KSPROPERTY_TYPE_GET |
                       KSPROPERTY_TYPE_SET;

        // set the return value size
        PropertyRequest->ValueSize = sizeof(ULONG);
        ntStatus = STATUS_SUCCESS;
    }

    // In case there was not even enough space for a ULONG in the return buffer,
    // we fail this request with STATUS_INVALID_DEVICE_REQUEST.
    // Any other case will return STATUS_SUCCESS.
    return ntStatus;
}

/*****************************************************************************
 * CAC97MiniportTopology::PropertyHandler_Level
 *****************************************************************************
 * Accesses a KSAUDIO_LEVEL property.
 * This function (property handler) is called by portcls every time there is a
 * get, set or basic support request for the node. The connection between the
 * node type and the property handler is made in the automation table which is
 * referenced when you register the node.
 * We use this property handler for all volume controls (and virtual volume
 * controls for recording).
 */
NTSTATUS CAC97MiniportTopology::PropertyHandler_Level
(
    IN      PPCPROPERTY_REQUEST   PropertyRequest
)
{
    PAGED_CODE ();

    ASSERT (PropertyRequest);

    DOUT (DBG_PRINT, ("[CAC97MiniportTopology::PropertyHandler_Level]"));

    NTSTATUS        ntStatus = STATUS_INVALID_PARAMETER;
    TopoNodes       NodeDef;
    LONG            channel;
    LONG            lMinimum, lMaximum;
    ULONG           uStep;
    // The major target is the object pointer to the topology miniport.
    CAC97MiniportTopology *that =
        (CAC97MiniportTopology *) PropertyRequest->MajorTarget;

    ASSERT (that);

    // validate node
    if (PropertyRequest->Node == (ULONG)-1)
        return ntStatus;

    // do the appropriate action for the request.

    // we should do a get or a set?
    if ((PropertyRequest->Verb & KSPROPERTY_TYPE_GET) ||
        (PropertyRequest->Verb & KSPROPERTY_TYPE_SET))
    {
        // validate parameters
        if ((PropertyRequest->InstanceSize < sizeof(LONG)) ||
            (PropertyRequest->ValueSize < sizeof(LONG)))
            return ntStatus;

        // get channel information
        channel = *((PLONG)PropertyRequest->Instance);

        // check channel types, return when unknown
        // as you can see, we have no multichannel support.
        if ((channel != CHAN_LEFT) &&
            (channel != CHAN_RIGHT) &&
            (channel != CHAN_MASTER))
            return ntStatus;

        // get the buffer
        PLONG Level = (PLONG)PropertyRequest->Value;

        // Switch on the node id. This is just for parameter checking.
        // If something goes wrong, we will immideately return with
        // ntStatus, which is STATUS_INVALID_PARAMETER.
        switch(NodeDef = that->TransNodeNrToNodeDef (PropertyRequest->Node))
        {
            // these are mono channels, don't respond to a right channel
            // request.
            case NODE_PCBEEP_VOLUME:
            case NODE_PHONE_VOLUME:
            case NODE_MIC_VOLUME:
            case NODE_VIRT_MONOOUT_VOLUME1:
            case NODE_VIRT_MONOOUT_VOLUME2:
            case NODE_VIRT_MASTER_INPUT_VOLUME1:
            case NODE_VIRT_MASTER_INPUT_VOLUME7:
            case NODE_VIRT_MASTER_INPUT_VOLUME8:
            case NODE_MICIN_VOLUME:
            case NODE_VIRT_MASTERMONO_VOLUME:
            case NODE_CENTER_VOLUME:
            case NODE_LFE_VOLUME:
                // check type
                if (PropertyRequest->PropertyItem->Id != KSPROPERTY_AUDIO_VOLUMELEVEL)
                    return ntStatus;
                // check channel
                if (channel == CHAN_RIGHT)
                    return ntStatus;
                // Well, this is a fake for the routine below that should work
                // for all nodes ... On AC97 the right channel are the LSBs and
                // mono channels have only LSBs used. Windows however thinks that
                // mono channels are left channels (only). So we could say here
                // we have a right channel request (to prg. the LSBs) instead of
                // a left channel request. But we have some controls that are HW-
                // stereo, but exposed to the system as mono. These are the virtual
                // volume controls in front of the wave-in muxer for the MIC, PHONE
                // and MONO MIX signals (see to the switch:
                // NODE_VIRT_MASTER_INPUT_VOLUME1, 7 and 8). Saying we have a MASTER
                // request makes sure the value is prg. for left and right channel,
                // but on HW-mono controls the right channel is prg. only, cause the
                // mask in ac97reg.h leads to a 0-mask for left channel prg. which
                // just does nothing ;)
                channel = CHAN_MASTER;
                break;
            
            // These are stereo channels.
            case NODE_MASTEROUT_VOLUME:
            case NODE_FRONT_VOLUME:
            case NODE_SURROUND_VOLUME:
            case NODE_HPOUT_VOLUME:
            case NODE_LINEIN_VOLUME:
            case NODE_CD_VOLUME:
            case NODE_VIDEO_VOLUME:
            case NODE_AUX_VOLUME:
            case NODE_WAVEOUT_VOLUME:
            case NODE_VIRT_MASTER_INPUT_VOLUME2:
            case NODE_VIRT_MASTER_INPUT_VOLUME3:
            case NODE_VIRT_MASTER_INPUT_VOLUME4:
            case NODE_VIRT_MASTER_INPUT_VOLUME5:
            case NODE_VIRT_MASTER_INPUT_VOLUME6:
                // check type
                if (PropertyRequest->PropertyItem->Id != KSPROPERTY_AUDIO_VOLUMELEVEL)
                    return ntStatus;
                // check channel; we don't support a get with master
                if ((channel == CHAN_MASTER) &&
                    (PropertyRequest->Verb & KSPROPERTY_TYPE_GET))
                    return ntStatus;
                break;

            case NODE_INVALID:
            default:
                // Ooops
                DOUT (DBG_ERROR, ("PropertyHandler_Level: Invalid node requested."));
                return ntStatus;
        }

        // Now, do some action!

        // get the registered dB values.
        ntStatus = GetDBValues (that->AdapterCommon, NodeDef, &lMinimum,
                                &lMaximum, &uStep);
        if (!NT_SUCCESS (ntStatus))
            return ntStatus;

        // do a get
        if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
        {
            WORD    wRegister;

            // Read the HW register for the node except NODE_VIRT_MASTERMONO_VOLUME
            // since this is pure virtual.
            if (NodeDef != NODE_VIRT_MASTERMONO_VOLUME)
            {
                // Get the register and read it.
                ntStatus = that->AdapterCommon->ReadCodecRegister (
                        that->AdapterCommon->GetNodeReg (NodeDef), &wRegister);
                if (!NT_SUCCESS (ntStatus))
                    return ntStatus;
                
                // mask out every unused bit and rotate.
                if (channel == CHAN_LEFT)
                {
                    wRegister = (wRegister & (that->AdapterCommon->GetNodeMask (NodeDef)
                                & AC97REG_MASK_LEFT)) >> 8;
                }
                else    // here goes mono or stereo-right
                {
                    wRegister &= (that->AdapterCommon->GetNodeMask (NodeDef) &
                                  AC97REG_MASK_RIGHT);
                }
            
                // Oops - NODE_PCBEEP_VOLUME doesn't use bit0. We have to adjust.
                if (NodeDef == NODE_PCBEEP_VOLUME)
                    wRegister >>= 1;

                // we have to translate the reg to dB.dB value.

                switch (NodeDef)
                {
                    // for record, we calculate it reverse.
                    case NODE_VIRT_MASTER_INPUT_VOLUME1:
                    case NODE_VIRT_MASTER_INPUT_VOLUME2:
                    case NODE_VIRT_MASTER_INPUT_VOLUME3:
                    case NODE_VIRT_MASTER_INPUT_VOLUME4:
                    case NODE_VIRT_MASTER_INPUT_VOLUME5:
                    case NODE_VIRT_MASTER_INPUT_VOLUME6:
                    case NODE_VIRT_MASTER_INPUT_VOLUME7:
                    case NODE_VIRT_MASTER_INPUT_VOLUME8:
                    case NODE_MICIN_VOLUME:
                        *Level = lMinimum + uStep * wRegister;
                        break;
                    default:
                        *Level = lMaximum - uStep * wRegister;
                        break;
                }
                
                // For the virtual controls, which are in front of a muxer, there
                // is no mute control displayed. But we have a HW mute control, so
                // what we do is enabling this mute when the user moves the slider
                // down to the bottom and disabling it on every other position.
                // We will return a PROP_MOST_NEGATIVE value in case the slider
                // is moved to the bottom.
                // We do this only for the "mono muxer" since the volume there ranges
                // from 0 to -46.5dB. The record volumes only have a range from
                // 0 to +22.5dB and we cannot mute them when the slider is down.
                if ((NodeDef == NODE_VIRT_MONOOUT_VOLUME1) ||
                    (NodeDef == NODE_VIRT_MONOOUT_VOLUME2))
                {
                    // read the register again.
                    ntStatus = that->AdapterCommon->ReadCodecRegister (
                               that->AdapterCommon->GetNodeReg (NodeDef), &wRegister);
                    if (!NT_SUCCESS (ntStatus))
                        return ntStatus;
                    // return most negative value in case it is checked.
                    if (wRegister & AC97REG_MASK_MUTE)
                        *Level = PROP_MOST_NEGATIVE;
                }
            }
            else    // This is master mono volume.
            {
                // Assume 0dB for master mono volume.
                *Level = 0;
            }

            // when we have cache information then return this instead
            // of the calculated value. if we don't, store the calculated
            // value.
            // We do that twice for master because in case we didn't set
            // the NodeCache yet it will be set then.
            if ((channel == CHAN_LEFT) || (channel == CHAN_MASTER))
            {
                if (that->stNodeCache[NodeDef].bLeftValid)
                    *Level = that->stNodeCache[NodeDef].lLeft;
                else
                {
                    that->stNodeCache[NodeDef].lLeft = *Level;
                    that->stNodeCache[NodeDef].bLeftValid = (BYTE)-1;
                }
            }

            if ((channel == CHAN_RIGHT) || (channel == CHAN_MASTER))
            {
                if (that->stNodeCache[NodeDef].bRightValid)
                    *Level = that->stNodeCache[NodeDef].lRight;
                else
                {
                    that->stNodeCache[NodeDef].lRight = *Level;
                    that->stNodeCache[NodeDef].bRightValid = (BYTE)-1;
                }
            }

            // thats all, good bye.
            PropertyRequest->ValueSize = sizeof(LONG);
            DOUT (DBG_PROPERTY, ("GET: %s(%s) = 0x%x",NodeStrings[NodeDef],
                    channel==CHAN_LEFT ? "L" : "R", *Level));
            
            // ntStatus was set with the read call! whatever this is, return it.
        }
        else        // this must be a set
        {
            WORD    wRegister;
            LONG    lLevel = *Level;

            //
            // Check borders.
            //
            // These 2 lines will have a special effect on sndvol32.
            // Whenever you move the balance slider on a volume, one channel
            // keeps the same and the other volume channel gets descreased.
            // With ac97 on recording controls, the default slider position
            // is at 0dB and the range of the volume is 0dB till +22.5dB.
            // That means that panning (moving the balance slider) is simply
            // impossible. If you would store the volume like sndvol gives it
            // to you and you return it on a get, then the balance slider
            // moves and stays at the position the user wanted it. However,
            // if you return the actual volume the balance slider will jump
            // back to the position that the HW can do (play with it to see
            // how it works).
            //
            if (lLevel > lMaximum) lLevel = lMaximum;
            if (lLevel < lMinimum) lLevel = lMinimum;
            
            // First update the node cache.
            if ((channel == CHAN_LEFT) || (channel == CHAN_MASTER))
            {
                that->stNodeCache[NodeDef].bLeftValid = (BYTE)-1;
                that->stNodeCache[NodeDef].lLeft = lLevel;
            }
            if ((channel == CHAN_RIGHT) || (channel == CHAN_MASTER))
            {
                that->stNodeCache[NodeDef].bRightValid = (BYTE)-1;
                that->stNodeCache[NodeDef].lRight = lLevel;
            }
            
            //
            // If we have a master mono, then we have to program the speaker
            // volumes a little different.
            // Check for master mono (surround or headphone present) and
            // if one of the speaker volumes is requested.
            //
            if ((that->AdapterCommon->GetPinConfig (PINC_SURROUND_PRESENT) ||
                 that->AdapterCommon->GetPinConfig (PINC_HPOUT_PRESENT)) &&
                ((NodeDef == NODE_VIRT_MASTERMONO_VOLUME) || (NodeDef == NODE_LFE_VOLUME) ||
                 (NodeDef == NODE_CENTER_VOLUME) || (NodeDef == NODE_FRONT_VOLUME) ||
                 (NodeDef == NODE_SURROUND_VOLUME) || (NodeDef == NODE_HPOUT_VOLUME)))
            {
                //
                // For master mono we have to update all speaker volumes.
                //
                if (NodeDef == NODE_VIRT_MASTERMONO_VOLUME)
                {
                    // Update all speaker volumes.
                    ntStatus = SetMultichannelVolume (that, NODE_FRONT_VOLUME);
                    if (that->AdapterCommon->GetPinConfig (PINC_HPOUT_PRESENT))
                        ntStatus = SetMultichannelVolume (that, NODE_HPOUT_VOLUME);
                    if (that->AdapterCommon->GetPinConfig (PINC_SURROUND_PRESENT))
                        ntStatus = SetMultichannelVolume (that, NODE_SURROUND_VOLUME);
                    if (that->AdapterCommon->GetPinConfig (PINC_CENTER_LFE_PRESENT))
                    {
                        ntStatus = SetMultichannelVolume (that, NODE_CENTER_VOLUME);
                        ntStatus = SetMultichannelVolume (that, NODE_LFE_VOLUME);
                    }
                }
                else    // update the individual speaker volume only.
                {
                    ntStatus = SetMultichannelVolume (that, NodeDef);
                }
            }
            else    // This is for all other volumes (or no master mono present).
            {
                // calculate the dB.dB value.

                // The nodes are calculated differently.
                switch (NodeDef)
                {
                    // for record controls we calculate it 'reverse'.
                    case NODE_VIRT_MASTER_INPUT_VOLUME1:
                    case NODE_VIRT_MASTER_INPUT_VOLUME2:
                    case NODE_VIRT_MASTER_INPUT_VOLUME3:
                    case NODE_VIRT_MASTER_INPUT_VOLUME4:
                    case NODE_VIRT_MASTER_INPUT_VOLUME5:
                    case NODE_VIRT_MASTER_INPUT_VOLUME6:
                    case NODE_VIRT_MASTER_INPUT_VOLUME7:
                    case NODE_VIRT_MASTER_INPUT_VOLUME8:
                        // read the wavein selector.
                        ntStatus = that->AdapterCommon->ReadCodecRegister (
                                that->AdapterCommon->GetNodeReg (NODE_WAVEIN_SELECT),
                                &wRegister);
                        if (!NT_SUCCESS (ntStatus))
                            return ntStatus;
    
                        // mask out every unused bit.
                        wRegister &= (that->AdapterCommon->GetNodeMask (
                                NODE_WAVEIN_SELECT) & AC97REG_MASK_RIGHT);
    
                        // check if the volume that we change belongs to the active
                        // (selected) virtual channel.
                        // Tricky: If the virtual nodes are not defined consecutively
                        // this comparision will fail.
                        if ((NodeDef - NODE_VIRT_MASTER_INPUT_VOLUME1) != wRegister)
                            return ntStatus;
                        
                        // fall through for calculation.

                    case NODE_MICIN_VOLUME:
                        wRegister = (WORD)(((lLevel + uStep / 2) - lMinimum) / uStep);
                        break;

                    case NODE_VIRT_MONOOUT_VOLUME1:
                    case NODE_VIRT_MONOOUT_VOLUME2:
                        // read the monoout selector.
                        ntStatus = that->AdapterCommon->ReadCodecRegister (
                                that->AdapterCommon->GetNodeReg (NODE_MONOOUT_SELECT),
                                &wRegister);
                        if (!NT_SUCCESS (ntStatus))
                            return ntStatus;
    
                        // mask out every unused bit.
                        wRegister &= that->AdapterCommon->GetNodeMask (NODE_MONOOUT_SELECT);
    
                        // check if the volume that we change belongs to the active
                        // (selected) virtual channel.
                        // Note: Monout select is set if we want to prg. MIC (Volume2).
                        if ((!wRegister && (NodeDef == NODE_VIRT_MONOOUT_VOLUME2)) ||
                            (wRegister && (NodeDef == NODE_VIRT_MONOOUT_VOLUME1)))
                            return ntStatus;
                    
                        // fall through for calculation.
                    default:
                        wRegister = (WORD)(((lMaximum + uStep / 2) - lLevel) / uStep);
                        break;
                }

                // Oops - NODE_PCBEEP_VOLUME doesn't use bit0. We have to adjust.
                if (NodeDef == NODE_PCBEEP_VOLUME)
                    wRegister <<= 1;
    
                // write the stuff (with mask!).
                // Note: mono channels are 'master' here (see fake above).
                // this makes sure that left and right channel is prg. for the virt.
                // controls. On controls that only have the right channel, the left
                // channel programming does nothing cause the mask will be zero.
                if ((channel == CHAN_LEFT) || (channel == CHAN_MASTER))
                {
                    // write only left.
                    ntStatus = that->AdapterCommon->WriteCodecRegister (
                        that->AdapterCommon->GetNodeReg (NodeDef),
                        wRegister << 8,
                        that->AdapterCommon->GetNodeMask (NodeDef) & AC97REG_MASK_LEFT);
                    // immediately return on error
                    if (!NT_SUCCESS (ntStatus))
                        return ntStatus;
                }
    
                if ((channel == CHAN_RIGHT) || (channel == CHAN_MASTER))
                {
                    // write only right.
                    ntStatus = that->AdapterCommon->WriteCodecRegister (
                        that->AdapterCommon->GetNodeReg (NodeDef),
                        wRegister,
                        that->AdapterCommon->GetNodeMask (NodeDef) & AC97REG_MASK_RIGHT);
                    // immediately return on error
                    if (!NT_SUCCESS (ntStatus))
                        return ntStatus;
                }

                // For the virtual controls, which are in front of a muxer, there
                // is no mute control displayed. But we have a HW mute control, so
                // what we do is enabling this mute when the user moves the slider
                // down to the bottom and disabling it on every other position.
                // We do this only for the "mono muxer", the recording mutes will
                // never be muted.
                // Tricky: Master input virtual controls must be defined consecutively.
                if ((NodeDef >= NODE_VIRT_MASTER_INPUT_VOLUME1) &&
                    (NodeDef <= NODE_VIRT_MASTER_INPUT_VOLUME8))
                {
                    // disable the mute; this only works because the mute and volume
                    // share the same register.
                    ntStatus = that->AdapterCommon->WriteCodecRegister (
                        that->AdapterCommon->GetNodeReg (NodeDef),
                        0, AC97REG_MASK_MUTE);
    
                    // Just in case.
                    that->UpdateRecordMute ();
                }

                if ((NodeDef == NODE_VIRT_MONOOUT_VOLUME1) ||
                    (NodeDef == NODE_VIRT_MONOOUT_VOLUME2))
                {
                    // these are only mono controls so checking one entry is enough.
                    if ( that->stNodeCache[NodeDef].bLeftValid &&
                        (that->stNodeCache[NodeDef].lLeft <= lMinimum))
                    {
                        // set the mute; this only works because the mute and volume
                        // share the same register.
                        ntStatus = that->AdapterCommon->WriteCodecRegister (
                            that->AdapterCommon->GetNodeReg (NodeDef),
                            AC97REG_MASK_MUTE, AC97REG_MASK_MUTE);
                    }
                    else
                    {
                        // clear the mute; this only works because the mute and volume
                        // share the same register.
                        ntStatus = that->AdapterCommon->WriteCodecRegister (
                            that->AdapterCommon->GetNodeReg (NodeDef),
                            0, AC97REG_MASK_MUTE);
                    }
                }
            }
            
            DOUT (DBG_PROPERTY, ("SET: %s(%s) -> 0x%x", NodeStrings[NodeDef],
                    channel==CHAN_LEFT ? "L" : channel==CHAN_RIGHT ? "R" : "M",
                    *Level));
            
            // ntStatus was set with the read call! whatever this is, return it.
        }
    }
    else
    {
        if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
        {
            ntStatus = BasicSupportHandler (PropertyRequest);
        }
    }

    return ntStatus;
}

/*****************************************************************************
 * CAC97MiniportTopology::PropertyHandler_Tone
 *****************************************************************************
 * Accesses a KSAUDIO_TONE property.
 * This function (property handler) is called by portcls every time there is a
 * get, set or basic support request for the node. The connection between the
 * node type and the property handler is made in the automation table which is
 * referenced when you register the node.
 * We use this property handler for all tone controls displayed at the advanced
 * property dialog in sndvol32 and the 3D controls displayed and exposed as
 * normal volume controls.
 */
NTSTATUS CAC97MiniportTopology::PropertyHandler_Tone
(
    IN      PPCPROPERTY_REQUEST   PropertyRequest
)
{
    PAGED_CODE ();

    ASSERT (PropertyRequest);

    DOUT (DBG_PRINT, ("[CAC97MiniportTopology::PropertyHandler_Tone]"));

    NTSTATUS        ntStatus = STATUS_INVALID_PARAMETER;
    TopoNodes       NodeDef;
    LONG            lMinimum, lMaximum;
    ULONG           uStep;
     // The major target is the object pointer to the topology miniport.
   CAC97MiniportTopology *that =
        (CAC97MiniportTopology *) PropertyRequest->MajorTarget;

    ASSERT (that);

    // validate node
    if (PropertyRequest->Node == (ULONG)-1)
        return ntStatus;

    // do the appropriate action for the request.

    // we should do a get or a set?
    if ((PropertyRequest->Verb & KSPROPERTY_TYPE_GET) ||
        (PropertyRequest->Verb & KSPROPERTY_TYPE_SET))
    {
        // validate parameters
        if ((PropertyRequest->InstanceSize < sizeof(LONG)) ||
            (PropertyRequest->ValueSize < sizeof(LONG)))
            return ntStatus;

        // get the buffer
        PLONG Level = (PLONG)PropertyRequest->Value;

        // Switch on the node id. This is just for parameter checking.
        // If something goes wrong, we will immideately return with
        // ntStatus, which is STATUS_INVALID_PARAMETER.
        switch(NodeDef = that->TransNodeNrToNodeDef (PropertyRequest->Node))
        {
            case NODE_BASS:
                // check type.
                if (PropertyRequest->PropertyItem->Id != KSPROPERTY_AUDIO_BASS)
                    return ntStatus;
                break;

            case NODE_TREBLE:
                // check type.
                if (PropertyRequest->PropertyItem->Id != KSPROPERTY_AUDIO_TREBLE)
                    return ntStatus;
                break;

            case NODE_VIRT_3D_CENTER:
            case NODE_VIRT_3D_DEPTH:
                // check 3D control
                if (!that->AdapterCommon->GetNodeConfig (NODEC_3D_CENTER_ADJUSTABLE)
                    && (NodeDef == NODE_VIRT_3D_CENTER))
                    return ntStatus;
                if (!that->AdapterCommon->GetNodeConfig (NODEC_3D_DEPTH_ADJUSTABLE)
                    && (NodeDef == NODE_VIRT_3D_DEPTH))
                    return ntStatus;
                // check type
                if (PropertyRequest->PropertyItem->Id != KSPROPERTY_AUDIO_VOLUMELEVEL)
                    return ntStatus;
                // check channel
                if (*(PLONG(PropertyRequest->Instance)) == CHAN_RIGHT)
                    return ntStatus;
                break;
            
            case NODE_INVALID:
            default:
                // Ooops
                DOUT (DBG_ERROR, ("PropertyHandler_Tone: Invalid node requested."));
                return ntStatus;
        }

        // Now, do some action!

        // get the registered DB values
        ntStatus = GetDBValues (that->AdapterCommon, NodeDef, &lMinimum,
                                &lMaximum, &uStep);
        if (!NT_SUCCESS (ntStatus))
            return ntStatus;

        // do a get
        if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
        {
            WORD    wRegister;

            // first get the stuff.
            ntStatus = that->AdapterCommon->ReadCodecRegister (
                    that->AdapterCommon->GetNodeReg (NodeDef), &wRegister);
            if (!NT_SUCCESS (ntStatus))
                return ntStatus;

            // mask out every unused bit.
            wRegister &= that->AdapterCommon->GetNodeMask (NodeDef);

            // rotate if bass tone control or 3D center control
            if ((NodeDef == NODE_BASS) || (NodeDef == NODE_VIRT_3D_CENTER))
                wRegister >>= 8;

            // convert from reg to dB.dB value.
            if ((NodeDef == NODE_VIRT_3D_CENTER) ||
                (NodeDef == NODE_VIRT_3D_DEPTH))
            {
                // That's for the 3D controls
                *Level = lMinimum + uStep * wRegister;
            }
            else
            {
                if (wRegister == 0x000F)
                    *Level = 0;     // bypass
                else
                    // And that's for the tone controls
                    *Level = lMaximum - uStep * wRegister;
            }
            
            // when we have cache information then return this instead
            // of the calculated value. if we don't, store the calculated
            // value.
            if (that->stNodeCache[NodeDef].bLeftValid)
                *Level = that->stNodeCache[NodeDef].lLeft;
            else
            {
                that->stNodeCache[NodeDef].lLeft = *Level;
                that->stNodeCache[NodeDef].bLeftValid = (BYTE)-1;
            }

            // we return a LONG
            PropertyRequest->ValueSize = sizeof(LONG);
            DOUT (DBG_PROPERTY, ("GET: %s = 0x%x", NodeStrings[NodeDef], *Level));
            // ntStatus was set with the read call! whatever this is, return it.
        }
        else        // that must be a set
        {
            WORD    wRegister;
            LONG    lLevel = *Level;

            // calculate the dB.dB value.
            // check borders.
            if (lLevel > lMaximum) lLevel = lMaximum;
            if (lLevel < lMinimum) lLevel = lMinimum;
            
            // write the value to the node cache.
            that->stNodeCache[NodeDef].lLeft = *Level;
            that->stNodeCache[NodeDef].bLeftValid = (BYTE)-1;

            // convert from dB.dB value to reg.
            if ((NodeDef == NODE_VIRT_3D_CENTER) ||
                (NodeDef == NODE_VIRT_3D_DEPTH))
            {
                // For 3D controls
                wRegister = (WORD)(((lLevel + uStep / 2) - lMinimum) / uStep);
            }
            else
            {
                // For tone controls
                wRegister = (WORD)(((lMaximum + uStep / 2) - lLevel) / uStep);
                // We don't prg. 0dB Bass or 0dB Treble, instead we smartly prg.
                // a bypass which is reg. value 0x0F.
                if (wRegister == 7)             // 0 dB
                    wRegister = 0x000F;         // bypass
            }

            // rotate if bass tone control or 3D center control
            if ((NodeDef == NODE_BASS) || (NodeDef == NODE_VIRT_3D_CENTER))
                wRegister <<= 8;

            // write the stuff.
            ntStatus = that->AdapterCommon->WriteCodecRegister (
                    that->AdapterCommon->GetNodeReg (NodeDef),
                    wRegister,
                    that->AdapterCommon->GetNodeMask (NodeDef));

            DOUT (DBG_PROPERTY,("SET: %s -> 0x%x", NodeStrings[NodeDef], *Level));
            // ntStatus was set with the write call! whatever this is, return in.
        }
    }
    else
    {
        if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
        {
            ntStatus = BasicSupportHandler (PropertyRequest);
        }
    }

    return ntStatus;
}
            
/*****************************************************************************
 * CAC97MiniportTopology::PropertyHandler_Ulong
 *****************************************************************************
 * Accesses a ULONG value property. For MUX and DEMUX.
 * This function (property handler) is called by portcls every time there is a
 * get, set or basic support request for the node. The connection between the
 * node type and the property handler is made in the automation table which is
 * referenced when you register the node.
 * We use this property handler for all muxer controls.
 */
NTSTATUS CAC97MiniportTopology::PropertyHandler_Ulong
(
    IN      PPCPROPERTY_REQUEST   PropertyRequest
)
{
    PAGED_CODE ();

    ASSERT (PropertyRequest);

    DOUT (DBG_PRINT, ("[CAC97MiniportTopology::PropertyHandler_Ulong]"));

    NTSTATUS        ntStatus = STATUS_INVALID_PARAMETER;
    TopoNodes       NodeDef;
    LONG            lMinimum, lMaximum;
    ULONG           uStep;
    // The major target is the object pointer to the topology miniport.
    CAC97MiniportTopology *that =
        (CAC97MiniportTopology *) PropertyRequest->MajorTarget;

    ASSERT (that);


    // validate node instance
    if (PropertyRequest->Node == (ULONG)-1)
        return ntStatus;

    // if we should do a get or set.
    if ((PropertyRequest->Verb & KSPROPERTY_TYPE_GET) ||
        (PropertyRequest->Verb & KSPROPERTY_TYPE_SET))
    {
        // validate buffer size.
        if (PropertyRequest->ValueSize < sizeof(ULONG))
            return ntStatus;

        // get the pointer to the buffer.
        PULONG PropValue = (PULONG)PropertyRequest->Value;

        // Switch on the node id. This is just for parameter checking.
        // If something goes wrong, we will immideately return with
        // ntStatus, which is STATUS_INVALID_PARAMETER.
        switch(NodeDef = that->TransNodeNrToNodeDef (PropertyRequest->Node))
        {
            case NODE_MONOOUT_SELECT:
            case NODE_WAVEIN_SELECT:
                // check the type
                if (PropertyRequest->PropertyItem->Id != KSPROPERTY_AUDIO_MUX_SOURCE)
                    return ntStatus;
                break;
                
            case NODE_INVALID:
            default:
                // Ooops
                DOUT (DBG_ERROR, ("PropertyHandler_Tone: Invalid node requested."));
                return ntStatus;
        }

        // Now do some action!

        // should we return the value?
        if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
        {
            WORD    wRegister;

            // first get the stuff.
            ntStatus = that->AdapterCommon->ReadCodecRegister (
                    that->AdapterCommon->GetNodeReg (NodeDef), &wRegister);
            if (!NT_SUCCESS (ntStatus))
                return ntStatus;

            // mask out every unused bit.
            wRegister &= that->AdapterCommon->GetNodeMask (NodeDef);

            // calculate the selected pin
            if (NodeDef == NODE_MONOOUT_SELECT)
            {
                // for mono out we have just one bit
                if (wRegister)
                    *PropValue = 2;
                else
                    *PropValue = 1;
            }
            else
            {
                // the wave in muxer is a stereo muxer, so just return the
                // right channel (gives values 0-7) and adjust it by adding 1.
                *PropValue = (wRegister & AC97REG_MASK_RIGHT) + 1;
            }

            // we return a LONG
            PropertyRequest->ValueSize = sizeof(LONG);
            DOUT (DBG_PROPERTY, ("GET: %s = 0x%x", NodeStrings[NodeDef],
                    *PropValue));
            // ntStatus was set with the read call! whatever this is, return it.
        }
        else        // that must be a set
        {
            TopoNodes   VirtNode;
            WORD        wRegister;
            ULONG       ulSelect = *PropValue;
            LONG        lLevel;

            // Check the selection first.
            if (NodeDef == NODE_MONOOUT_SELECT)
            {
                if ((ulSelect < 1) || (ulSelect > 2))
                    return ntStatus;    // STATUS_INVALID_PARAMETER
            }
            else
            {
                if ((ulSelect < 1) || (ulSelect > 8))
                    return ntStatus;    // STATUS_INVALID_PARAMETER
            }

            // calculate the register value for programming.
            if (NodeDef == NODE_MONOOUT_SELECT)
            {
                // for mono out we have just one bit
                if (ulSelect == 2)
                    // the mask will make sure we only prg. one bit.
                    wRegister = 0xFFFF;
                else
                    // ulSelect == 1
                    wRegister = 0;
            }
            else
            {
                // *257 is the same as: (ulSelect << 8) + ulSelect
                wRegister = (WORD)(ulSelect - 1) * 257;
            }

            // write the stuff.
            ntStatus = that->AdapterCommon->WriteCodecRegister (
                    that->AdapterCommon->GetNodeReg (NodeDef),
                    wRegister,
                    that->AdapterCommon->GetNodeMask (NodeDef));

            // Store the virt. node for later use.
            // Tricky: Master input virtual controls must be defined consecutively.
            if (NodeDef == NODE_MONOOUT_SELECT)
                VirtNode = (TopoNodes)(NODE_VIRT_MONOOUT_VOLUME1 + (ulSelect - 1));
            else
                VirtNode = (TopoNodes)(NODE_VIRT_MASTER_INPUT_VOLUME1 + (ulSelect - 1));

            // Virtual controls make our life more complicated. When the user
            // changes the input source say from CD to LiniIn, then the system just
            // sends a message to the input muxer that the selection changed.
            // Cause we have only one HW register for the input muxer, all volumes
            // displayed for the user are "virtualized", means they are not there,
            // and when the selection changes, we have to prg. the volume of the
            // selected input to the HW register. That's what we do now.
            
            // get the registered DB values
            ntStatus = GetDBValues (that->AdapterCommon, VirtNode,
                                    &lMinimum, &lMaximum, &uStep);
            if (!NT_SUCCESS (ntStatus))
                return ntStatus;

            // We can be lazy here and don't check for mono controls. Reason
            // is that the level handler writes the volume value for mono
            // controls into both the left and right node cache ;))
            
            if (that->stNodeCache[VirtNode].bLeftValid &&
                that->stNodeCache[VirtNode].bRightValid)
            {
                // prg. left channel
                lLevel = that->stNodeCache[VirtNode].lLeft;

                // calculate the dB.dB value.
                if (NodeDef == NODE_MONOOUT_SELECT)
                    wRegister = (WORD)(((lMaximum + uStep / 2) - lLevel) / uStep);
                else
                    wRegister = (WORD)(((lLevel + uStep / 2) - lMinimum) / uStep);

                // write left channel.
                ntStatus = that->AdapterCommon->WriteCodecRegister (
                    that->AdapterCommon->GetNodeReg (VirtNode),
                    wRegister << 8,
                    that->AdapterCommon->GetNodeMask (VirtNode) & AC97REG_MASK_LEFT);

                // prg. right channel
                lLevel = that->stNodeCache[VirtNode].lRight;

                // calculate the dB.dB value.
                if (NodeDef == NODE_MONOOUT_SELECT)
                    wRegister = (WORD)(((lMaximum + uStep / 2) - lLevel) / uStep);
                else
                    wRegister = (WORD)(((lLevel + uStep / 2) - lMinimum) / uStep);

                // write right channel.
                ntStatus = that->AdapterCommon->WriteCodecRegister (
                    that->AdapterCommon->GetNodeReg (VirtNode),
                    wRegister,
                    that->AdapterCommon->GetNodeMask (VirtNode) & AC97REG_MASK_RIGHT);
                
                // For the virtual controls, which are in front of a muxer, there
                // is no mute control displayed. But we have a HW mute control, so
                // what we do is enabling this mute when the user moves the slider
                // down to the bottom and disabling it on every other position.
                // We do this only for the "mono muxer", the recording mutes will
                // never be muted.
                if (NodeDef == NODE_WAVEIN_SELECT)
                {
                    // disable the mute; this only works because the mute and volume
                    // share the same register.
                    ntStatus = that->AdapterCommon->WriteCodecRegister (
                        that->AdapterCommon->GetNodeReg (VirtNode),
                        0, AC97REG_MASK_MUTE);
            
                    that->UpdateRecordMute ();
                }

                if (NodeDef == NODE_MONOOUT_SELECT)
                {
                    // these are only mono controls so checking one entry is enough.
                    if ( that->stNodeCache[VirtNode].bLeftValid &&
                        (that->stNodeCache[VirtNode].lLeft <= lMinimum))
                    {
                        // set the mute; this only works because the mute and volume
                        // share the same register.
                        ntStatus = that->AdapterCommon->WriteCodecRegister (
                            that->AdapterCommon->GetNodeReg (VirtNode),
                            AC97REG_MASK_MUTE, AC97REG_MASK_MUTE);
                    }
                    else
                    {
                        // clear the mute; this only works because the mute and volume
                        // share the same register.
                        ntStatus = that->AdapterCommon->WriteCodecRegister (
                            that->AdapterCommon->GetNodeReg (VirtNode),
                            0, AC97REG_MASK_MUTE);
                    }
                }
            }                
            
            DOUT (DBG_PROPERTY, ("SET: %s -> 0x%x", NodeStrings[NodeDef],
                    *PropValue));
            // ntStatus was set with the write call! whatever this is, return it.
        }
    }

    return ntStatus;
}

/*****************************************************************************
 * CAC97MiniportTopology::PropertyHandler_CpuResources
 *****************************************************************************
 * Propcesses a KSPROPERTY_AUDIO_CPU_RESOURCES request
 * This property handler is called by the system for every node and every node
 * must support this property. Basically, this property is for performance
 * monitoring and we just say here that every function we claim to have has HW
 * support (which by the way is true).
 */
NTSTATUS CAC97MiniportTopology::PropertyHandler_CpuResources
(
    IN      PPCPROPERTY_REQUEST   PropertyRequest
)
{
    PAGED_CODE ();

    ASSERT (PropertyRequest);

    DOUT (DBG_PRINT, ("[CAC97MiniportTopology::PropertyHandler_CpuResources]"));

    CAC97MiniportTopology *that =
        (CAC97MiniportTopology *) PropertyRequest->MajorTarget;
    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;

    ASSERT (that);

    // validate node
    if (PropertyRequest->Node == (ULONG)-1)
        return ntStatus;

    // validate the node def.
    if (that->TransNodeNrToNodeDef (PropertyRequest->Node) == NODE_INVALID)
        return ntStatus;
    
    // we should do a get
    if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
    {
        // just return the flag.
        if (PropertyRequest->ValueSize >= sizeof(LONG))
        {
            *((PLONG)PropertyRequest->Value) = KSAUDIO_CPU_RESOURCES_NOT_HOST_CPU;
            PropertyRequest->ValueSize = sizeof(LONG);
            ntStatus = STATUS_SUCCESS;
        }
        else    // not enough buffer.
        {
            ntStatus = STATUS_BUFFER_TOO_SMALL;
        }
    }

    return ntStatus;
}

#ifdef INCLUDE_PRIVATE_PROPERTY
/*****************************************************************************
 * CAC97MiniportTopology::PropertyHandler_Private
 *****************************************************************************
 * This is a private property that returns some AC97 codec features.
 * This routine gets called whenever the topology filter gets a property
 * request with KSPROSETPID_Private and KSPROPERTY_AC97_FEATURES set. It is not
 * a node property but a filter property (you don't have to specify a node).
 */
NTSTATUS CAC97MiniportTopology::PropertyHandler_Private
(
    IN      PPCPROPERTY_REQUEST   PropertyRequest
)
{
    PAGED_CODE ();

    ASSERT (PropertyRequest);

    DOUT (DBG_PRINT, ("[CAC97MiniportTopology::PropertyHandler_Private]"));

    NTSTATUS        ntStatus = STATUS_INVALID_PARAMETER;
    // The major target is the object pointer to the topology miniport.
    CAC97MiniportTopology *that =
        (CAC97MiniportTopology *) PropertyRequest->MajorTarget;


    ASSERT (that);


    // We only have a get defined.
    if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
    {
        // Check the ID ("function" in "group").
        if (PropertyRequest->PropertyItem->Id != KSPROPERTY_AC97_FEATURES)
            return ntStatus;

        // validate buffer size.
        if (PropertyRequest->ValueSize < sizeof (tAC97Features))
            return ntStatus;

        // The "Value" is the out buffer that you pass in DeviceIoControl call.
        tAC97Features *pAC97Features = (tAC97Features *) PropertyRequest->Value;
        
        // Check the buffer.
        if (!pAC97Features)
            return ntStatus;

        //
        // Fill the AC97Features structure.
        //

        // Set the volumes.
        pAC97Features->MasterVolume = Volume5bit;
        if (that->AdapterCommon->GetNodeConfig (NODEC_6BIT_MASTER_VOLUME))
            pAC97Features->MasterVolume = Volume6bit;
        
        pAC97Features->HeadphoneVolume = Volume5bit;
        if (!that->AdapterCommon->GetPinConfig (PINC_HPOUT_PRESENT))
            pAC97Features->HeadphoneVolume = VolumeDisabled;
        else if (that->AdapterCommon->GetNodeConfig (NODEC_6BIT_HPOUT_VOLUME))
            pAC97Features->HeadphoneVolume = Volume6bit;
        
        pAC97Features->MonoOutVolume = Volume5bit;
        if (!that->AdapterCommon->GetPinConfig (PINC_MONOOUT_PRESENT))
            pAC97Features->MonoOutVolume = VolumeDisabled;
        else if (that->AdapterCommon->GetNodeConfig (NODEC_6BIT_MONOOUT_VOLUME))
            pAC97Features->MonoOutVolume = Volume6bit;

        // The 18/20bit Resolution information.
        WORD wCodecID;

        // Read the reset register.
        ntStatus = that->AdapterCommon->ReadCodecRegister (AC97REG_RESET, &wCodecID);
        if (!NT_SUCCESS (ntStatus))
            return ntStatus;

        //
        // Now check the DAC and ADC resolution.
        //

        // First the DAC.
        pAC97Features->DAC = Resolution16bit;
        if (wCodecID & 0x0040)
            pAC97Features->DAC = Resolution18bit;
        if (wCodecID & 0x0080)
            pAC97Features->DAC = Resolution20bit;

        // Then the ADC.
        pAC97Features->ADC = Resolution16bit;
        if (wCodecID & 0x0100)
            pAC97Features->ADC = Resolution18bit;
        if (wCodecID & 0x0200)
            pAC97Features->ADC = Resolution20bit;

        // 3D technique
        pAC97Features->n3DTechnique = ((wCodecID & 0x7C00) >> 10);

        // Set the flag for MicIn.
        pAC97Features->bMicInPresent = that->AdapterCommon->
            GetPinConfig (PINC_MICIN_PRESENT) ? TRUE : FALSE;

        // Variable sample rate info.
        pAC97Features->bVSRPCM = that->AdapterCommon->
            GetNodeConfig (NODEC_PCM_VARIABLERATE_SUPPORTED) ? TRUE : FALSE;
        pAC97Features->bDSRPCM = that->AdapterCommon->
            GetNodeConfig (NODEC_PCM_DOUBLERATE_SUPPORTED) ? TRUE : FALSE;
        pAC97Features->bVSRMIC = that->AdapterCommon->
            GetNodeConfig (NODEC_MIC_VARIABLERATE_SUPPORTED) ? TRUE : FALSE;

        // Additional DAC's
        pAC97Features->bCenterDAC = that->AdapterCommon->
            GetNodeConfig (NODEC_CENTER_DAC_PRESENT) ? TRUE : FALSE;
        pAC97Features->bSurroundDAC = that->AdapterCommon->
            GetNodeConfig (NODEC_SURROUND_DAC_PRESENT) ? TRUE : FALSE;
        pAC97Features->bLFEDAC = that->AdapterCommon->
            GetNodeConfig (NODEC_LFE_DAC_PRESENT) ? TRUE : FALSE;


        // We filled out the structure.
        PropertyRequest->ValueSize = sizeof (tAC97Features);
        DOUT (DBG_PROPERTY, ("Get AC97Features succeeded."));

        // ntStatus was set with the read call! whatever this is, return it.
    }
#ifdef PROPERTY_SHOW_SET
    else
    {
        // Just to show, we have a SET also.
        if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
        {
            // This is the only property for a SET.
            if (PropertyRequest->PropertyItem->Id != KSPROPERTY_AC97_SAMPLE_SET)
                return ntStatus;

            // validate buffer size.
            if (PropertyRequest->ValueSize < sizeof (DWORD))
                return ntStatus;

            // Get the pointer to the DWORD.
            DWORD   *pTimerTick = (DWORD *)PropertyRequest->Value;

            // Check the buffer.
            if (!pTimerTick)
                return ntStatus;

            // Print the message.
            DOUT (DBG_ALL, ("This computer is already %d ms running Windows!", *pTimerTick));
            ntStatus = STATUS_SUCCESS;
        }
    }
#endif

    return ntStatus;
}
#endif


