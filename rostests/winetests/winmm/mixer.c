/*
 * Test mixer
 *
 * Copyright (c) 2004 Robert Reif
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/*
 * To Do:
 * add interactive tests
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "mmsystem.h"

#include "winmm_test.h"

static const char * line_flags(DWORD fdwLine)
{
    static char flags[100];
    BOOL first=TRUE;
    flags[0]=0;
    if (fdwLine&MIXERLINE_LINEF_ACTIVE) {
        strcat(flags,"MIXERLINE_LINEF_ACTIVE");
        first=FALSE;
    }
    if (fdwLine&MIXERLINE_LINEF_DISCONNECTED) {
        if (!first)
            strcat(flags, "|");

        strcat(flags,"MIXERLINE_LINEF_DISCONNECTED");
        first=FALSE;
    }

    if (fdwLine&MIXERLINE_LINEF_SOURCE) {
        if (!first)
            strcat(flags, "|");

        strcat(flags,"MIXERLINE_LINEF_SOURCE");
    }

    return flags;
}

static const char * component_type(DWORD dwComponentType)
{
#define TYPE_TO_STR(x) case x: return #x
    switch (dwComponentType) {
    TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_DST_UNDEFINED);
    TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_DST_DIGITAL);
    TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_DST_LINE);
    TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_DST_MONITOR);
    TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_DST_SPEAKERS);
    TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_DST_HEADPHONES);
    TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_DST_TELEPHONE);
    TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_DST_WAVEIN);
    TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_DST_VOICEIN);
    TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_SRC_UNDEFINED);
    TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_SRC_DIGITAL);
    TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_SRC_LINE);
    TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE);
    TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_SRC_SYNTHESIZER);
    TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC);
    TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_SRC_TELEPHONE);
    TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_SRC_PCSPEAKER);
    TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT);
    TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_SRC_AUXILIARY);
    TYPE_TO_STR(MIXERLINE_COMPONENTTYPE_SRC_ANALOG);
    }
#undef TYPE_TO_STR
    return "UNKNOWN";
}

static const char * target_type(DWORD dwType)
{
#define TYPE_TO_STR(x) case x: return #x
    switch (dwType) {
    TYPE_TO_STR(MIXERLINE_TARGETTYPE_UNDEFINED);
    TYPE_TO_STR(MIXERLINE_TARGETTYPE_WAVEOUT);
    TYPE_TO_STR(MIXERLINE_TARGETTYPE_WAVEIN);
    TYPE_TO_STR(MIXERLINE_TARGETTYPE_MIDIOUT);
    TYPE_TO_STR(MIXERLINE_TARGETTYPE_MIDIIN);
    TYPE_TO_STR(MIXERLINE_TARGETTYPE_AUX);
    }
#undef TYPE_TO_STR
    return "UNKNOWN";
}

static const char * control_type(DWORD dwControlType)
{
#define TYPE_TO_STR(x) case x: return #x
    switch (dwControlType) {
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_CUSTOM);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_BOOLEANMETER);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_SIGNEDMETER);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_PEAKMETER);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_UNSIGNEDMETER);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_BOOLEAN);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_ONOFF);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_MUTE);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_MONO);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_LOUDNESS);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_STEREOENH);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_BASS_BOOST);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_BUTTON);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_DECIBELS);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_SIGNED);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_UNSIGNED);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_PERCENT);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_SLIDER);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_PAN);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_QSOUNDPAN);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_FADER);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_VOLUME);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_BASS);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_TREBLE);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_EQUALIZER);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_SINGLESELECT);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_MUX);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_MULTIPLESELECT);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_MIXER);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_MICROTIME);
    TYPE_TO_STR(MIXERCONTROL_CONTROLTYPE_MILLITIME);
    }
#undef TYPE_TO_STR
    return "UNKNOWN";
}

static const char * control_flags(DWORD fdwControl)
{
    static char flags[100];
    BOOL first=TRUE;
    flags[0]=0;
    if (fdwControl&MIXERCONTROL_CONTROLF_UNIFORM) {
        strcat(flags,"MIXERCONTROL_CONTROLF_UNIFORM");
        first=FALSE;
    }
    if (fdwControl&MIXERCONTROL_CONTROLF_MULTIPLE) {
        if (!first)
            strcat(flags, "|");

        strcat(flags,"MIXERCONTROL_CONTROLF_MULTIPLE");
        first=FALSE;
    }

    if (fdwControl&MIXERCONTROL_CONTROLF_DISABLED) {
        if (!first)
            strcat(flags, "|");

        strcat(flags,"MIXERCONTROL_CONTROLF_DISABLED");
    }

    return flags;
}

static void test_mixerClose(HMIXER mix)
{
    MMRESULT rc;

    rc = mixerClose(mix);
    ok(rc == MMSYSERR_NOERROR || rc == MMSYSERR_INVALHANDLE,
       "mixerClose: MMSYSERR_NOERROR or MMSYSERR_INVALHANDLE expected, got %s\n",
       mmsys_error(rc));
}

static void mixer_test_controlA(HMIXEROBJ mix, MIXERCONTROLA *control)
{
    MMRESULT rc;

    if ((control->dwControlType == MIXERCONTROL_CONTROLTYPE_VOLUME) ||
        (control->dwControlType == MIXERCONTROL_CONTROLTYPE_UNSIGNED)) {
        MIXERCONTROLDETAILS details;
        MIXERCONTROLDETAILS_UNSIGNED value;

        details.cbStruct = sizeof(MIXERCONTROLDETAILS);
        details.dwControlID = control->dwControlID;
        details.cChannels = 1;
        U(details).cMultipleItems = 0;
        details.cbDetails = sizeof(value);

        /* test NULL paDetails */
        details.paDetails = NULL;
        rc = mixerGetControlDetailsA(mix, &details, MIXER_GETCONTROLDETAILSF_VALUE);
        ok(rc==MMSYSERR_INVALPARAM,
           "mixerGetDevCapsA: MMSYSERR_INVALPARAM expected, got %s\n",
           mmsys_error(rc));

        /* read the current control value */
        details.paDetails = &value;
        rc = mixerGetControlDetailsA(mix, &details, MIXER_GETCONTROLDETAILSF_VALUE);
        ok(rc==MMSYSERR_NOERROR,"mixerGetControlDetails(MIXER_GETCONTROLDETAILSF_VALUE): "
           "MMSYSERR_NOERROR expected, got %s\n",
           mmsys_error(rc));
        if (rc==MMSYSERR_NOERROR && winetest_interactive) {
            MIXERCONTROLDETAILS new_details;
            MIXERCONTROLDETAILS_UNSIGNED new_value;

            trace("            Value=%d\n",value.dwValue);

            if (value.dwValue + control->Metrics.cSteps < S1(control->Bounds).dwMaximum)
                new_value.dwValue = value.dwValue + control->Metrics.cSteps;
            else
                new_value.dwValue = value.dwValue - control->Metrics.cSteps;

            new_details.cbStruct = sizeof(MIXERCONTROLDETAILS);
            new_details.dwControlID = control->dwControlID;
            new_details.cChannels = 1;
            U(new_details).cMultipleItems = 0;
            new_details.paDetails = &new_value;
            new_details.cbDetails = sizeof(new_value);

            /* change the control value by one step */
            rc = mixerSetControlDetails(mix, &new_details, MIXER_SETCONTROLDETAILSF_VALUE);
            ok(rc==MMSYSERR_NOERROR,"mixerSetControlDetails(MIXER_SETCONTROLDETAILSF_VALUE): "
               "MMSYSERR_NOERROR expected, got %s\n",
               mmsys_error(rc));
            if (rc==MMSYSERR_NOERROR) {
                MIXERCONTROLDETAILS ret_details;
                MIXERCONTROLDETAILS_UNSIGNED ret_value;

                ret_details.cbStruct = sizeof(MIXERCONTROLDETAILS);
                ret_details.dwControlID = control->dwControlID;
                ret_details.cChannels = 1;
                U(ret_details).cMultipleItems = 0;
                ret_details.paDetails = &ret_value;
                ret_details.cbDetails = sizeof(ret_value);

                /* read back the new control value */
                rc = mixerGetControlDetailsA(mix, &ret_details, MIXER_GETCONTROLDETAILSF_VALUE);
                ok(rc==MMSYSERR_NOERROR,"mixerGetControlDetails(MIXER_GETCONTROLDETAILSF_VALUE): "
                   "MMSYSERR_NOERROR expected, got %s\n",
                   mmsys_error(rc));
                if (rc==MMSYSERR_NOERROR) {
                    /* result may not match exactly because of rounding */
                    ok(abs(ret_value.dwValue-new_value.dwValue)<=1,
                       "Couldn't change value from %d to %d, returned %d\n",
                       value.dwValue,new_value.dwValue,ret_value.dwValue);

                    if (abs(ret_value.dwValue-new_value.dwValue)<=1) {
                        details.cbStruct = sizeof(MIXERCONTROLDETAILS);
                        details.dwControlID = control->dwControlID;
                        details.cChannels = 1;
                        U(details).cMultipleItems = 0;
                        details.paDetails = &value;
                        details.cbDetails = sizeof(value);

                        /* restore original value */
                        rc = mixerSetControlDetails(mix, &details, MIXER_SETCONTROLDETAILSF_VALUE);
                        ok(rc==MMSYSERR_NOERROR,"mixerSetControlDetails(MIXER_SETCONTROLDETAILSF_VALUE): "
                           "MMSYSERR_NOERROR expected, got %s\n",
                           mmsys_error(rc));
                    }
                }
            }
        }
    } else if ((control->dwControlType == MIXERCONTROL_CONTROLTYPE_MUTE) ||
        (control->dwControlType == MIXERCONTROL_CONTROLTYPE_BOOLEAN) ||
        (control->dwControlType == MIXERCONTROL_CONTROLTYPE_BUTTON)) {
        MIXERCONTROLDETAILS details;
        MIXERCONTROLDETAILS_BOOLEAN value;

        details.cbStruct = sizeof(MIXERCONTROLDETAILS);
        details.dwControlID = control->dwControlID;
        details.cChannels = 1;
        U(details).cMultipleItems = 0;
        details.paDetails = &value;
        details.cbDetails = sizeof(value);

        rc = mixerGetControlDetailsA(mix, &details, MIXER_GETCONTROLDETAILSF_VALUE);
        ok(rc==MMSYSERR_NOERROR,"mixerGetControlDetails(MIXER_GETCONTROLDETAILSF_VALUE): "
           "MMSYSERR_NOERROR expected, got %s\n",
           mmsys_error(rc));
        if (rc==MMSYSERR_NOERROR && winetest_interactive) {
            MIXERCONTROLDETAILS new_details;
            MIXERCONTROLDETAILS_BOOLEAN new_value;

            trace("            Value=%d\n",value.fValue);

            if (value.fValue == FALSE)
                new_value.fValue = TRUE;
            else
                new_value.fValue = FALSE;

            new_details.cbStruct = sizeof(MIXERCONTROLDETAILS);
            new_details.dwControlID = control->dwControlID;
            new_details.cChannels = 1;
            U(new_details).cMultipleItems = 0;
            new_details.paDetails = &new_value;
            new_details.cbDetails = sizeof(new_value);

            /* change the control value by one step */
            rc = mixerSetControlDetails(mix, &new_details, MIXER_SETCONTROLDETAILSF_VALUE);
            ok(rc==MMSYSERR_NOERROR,"mixerSetControlDetails(MIXER_SETCONTROLDETAILSF_VALUE): "
               "MMSYSERR_NOERROR expected, got %s\n",
               mmsys_error(rc));
            if (rc==MMSYSERR_NOERROR) {
                MIXERCONTROLDETAILS ret_details;
                MIXERCONTROLDETAILS_BOOLEAN ret_value;

                ret_details.cbStruct = sizeof(MIXERCONTROLDETAILS);
                ret_details.dwControlID = control->dwControlID;
                ret_details.cChannels = 1;
                U(ret_details).cMultipleItems = 0;
                ret_details.paDetails = &ret_value;
                ret_details.cbDetails = sizeof(ret_value);

                /* read back the new control value */
                rc = mixerGetControlDetailsA(mix, &ret_details, MIXER_GETCONTROLDETAILSF_VALUE);
                ok(rc==MMSYSERR_NOERROR,"mixerGetControlDetails(MIXER_GETCONTROLDETAILSF_VALUE): "
                   "MMSYSERR_NOERROR expected, got %s\n",
                   mmsys_error(rc));
                if (rc==MMSYSERR_NOERROR) {
                    /* result may not match exactly because of rounding */
                    ok(ret_value.fValue==new_value.fValue,
                       "Couldn't change value from %d to %d, returned %d\n",
                       value.fValue,new_value.fValue,ret_value.fValue);

                    if (ret_value.fValue==new_value.fValue) {
                        details.cbStruct = sizeof(MIXERCONTROLDETAILS);
                        details.dwControlID = control->dwControlID;
                        details.cChannels = 1;
                        U(details).cMultipleItems = 0;
                        details.paDetails = &value;
                        details.cbDetails = sizeof(value);

                        /* restore original value */
                        rc = mixerSetControlDetails(mix, &details, MIXER_SETCONTROLDETAILSF_VALUE);
                        ok(rc==MMSYSERR_NOERROR,"mixerSetControlDetails(MIXER_SETCONTROLDETAILSF_VALUE): "
                           "MMSYSERR_NOERROR expected, got %s\n",
                           mmsys_error(rc));
                    }
                }
            }
        }
    } else {
        /* FIXME */
    }
}

static void mixer_test_deviceA(int device)
{
    MIXERCAPSA capsA;
    HMIXEROBJ mix;
    MMRESULT rc;
    DWORD d,s,ns,nc;

    rc=mixerGetDevCapsA(device,0,sizeof(capsA));
    ok(rc==MMSYSERR_INVALPARAM,
       "mixerGetDevCapsA: MMSYSERR_INVALPARAM expected, got %s\n",
       mmsys_error(rc));

    rc=mixerGetDevCapsA(device,&capsA,4);
    ok(rc==MMSYSERR_NOERROR,
       "mixerGetDevCapsA: MMSYSERR_NOERROR expected, got %s\n",
       mmsys_error(rc));

    rc=mixerGetDevCapsA(device,&capsA,sizeof(capsA));
    ok(rc==MMSYSERR_NOERROR,
       "mixerGetDevCapsA: MMSYSERR_NOERROR expected, got %s\n",
       mmsys_error(rc));

    if (winetest_interactive) {
        trace("  %d: \"%s\" %d.%d (%d:%d) destinations=%d\n", device,
              capsA.szPname, capsA.vDriverVersion >> 8,
              capsA.vDriverVersion & 0xff,capsA.wMid,capsA.wPid,
              capsA.cDestinations);
    } else {
        trace("  %d: \"%s\" %d.%d (%d:%d)\n", device,
              capsA.szPname, capsA.vDriverVersion >> 8,
              capsA.vDriverVersion & 0xff,capsA.wMid,capsA.wPid);
    }

    rc = mixerOpen((HMIXER*)&mix, device, 0, 0, 0);
    ok(rc==MMSYSERR_NOERROR,
       "mixerOpen: MMSYSERR_NOERROR expected, got %s\n",mmsys_error(rc));
    if (rc==MMSYSERR_NOERROR) {
        MIXERCAPSA capsA2;

        rc=mixerGetDevCapsA((UINT_PTR)mix,&capsA2,sizeof(capsA2));
        ok(rc==MMSYSERR_NOERROR,
           "mixerGetDevCapsA: MMSYSERR_NOERROR expected, got %s\n",
           mmsys_error(rc));
        ok(!strcmp(capsA2.szPname, capsA.szPname), "Got wrong device caps\n");

        for (d=0;d<capsA.cDestinations;d++) {
            MIXERLINEA mixerlineA;
            mixerlineA.cbStruct = 0;
            mixerlineA.dwDestination=d;
            rc = mixerGetLineInfoA(mix, &mixerlineA, MIXER_GETLINEINFOF_DESTINATION);
            ok(rc==MMSYSERR_INVALPARAM,
               "mixerGetLineInfoA(MIXER_GETLINEINFOF_DESTINATION): "
               "MMSYSERR_INVALPARAM expected, got %s\n",
               mmsys_error(rc));

            mixerlineA.cbStruct = sizeof(mixerlineA);
            mixerlineA.dwDestination=capsA.cDestinations;
            rc = mixerGetLineInfoA(mix, &mixerlineA, MIXER_GETLINEINFOF_DESTINATION);
            ok(rc==MMSYSERR_INVALPARAM||rc==MIXERR_INVALLINE,
               "mixerGetLineInfoA(MIXER_GETLINEINFOF_DESTINATION): "
               "MMSYSERR_INVALPARAM or MIXERR_INVALLINE expected, got %s\n",
               mmsys_error(rc));

            mixerlineA.cbStruct = sizeof(mixerlineA);
            mixerlineA.dwDestination=d;
            rc = mixerGetLineInfoA(mix, 0, MIXER_GETLINEINFOF_DESTINATION);
            ok(rc==MMSYSERR_INVALPARAM,
               "mixerGetLineInfoA(MIXER_GETLINEINFOF_DESTINATION): "
               "MMSYSERR_INVALPARAM expected, got %s\n",
               mmsys_error(rc));

            mixerlineA.cbStruct = sizeof(mixerlineA);
            mixerlineA.dwDestination=d;
            rc = mixerGetLineInfoA(mix, &mixerlineA, -1);
            ok(rc==MMSYSERR_INVALFLAG,
               "mixerGetLineInfoA(-1): MMSYSERR_INVALFLAG expected, got %s\n",
               mmsys_error(rc));

            mixerlineA.cbStruct = sizeof(mixerlineA);
            mixerlineA.dwDestination=d;
            rc = mixerGetLineInfoA(mix, &mixerlineA, MIXER_GETLINEINFOF_DESTINATION);
            ok(rc==MMSYSERR_NOERROR||rc==MMSYSERR_NODRIVER,
               "mixerGetLineInfoA(MIXER_GETLINEINFOF_DESTINATION): "
               "MMSYSERR_NOERROR expected, got %s\n",
               mmsys_error(rc));
            if (rc==MMSYSERR_NODRIVER)
                trace("  No Driver\n");
            else if (rc==MMSYSERR_NOERROR) {
	      if (winetest_interactive) {
                trace("    %d: \"%s\" (%s) Destination=%d Source=%d\n",
                      d,mixerlineA.szShortName, mixerlineA.szName,
                      mixerlineA.dwDestination,mixerlineA.dwSource);
                trace("        LineID=%08x Channels=%d "
                      "Connections=%d Controls=%d\n",
                      mixerlineA.dwLineID,mixerlineA.cChannels,
                      mixerlineA.cConnections,mixerlineA.cControls);
                trace("        State=0x%08x(%s)\n",
                      mixerlineA.fdwLine,line_flags(mixerlineA.fdwLine));
                trace("        ComponentType=%s\n",
                      component_type(mixerlineA.dwComponentType));
                trace("        Type=%s\n",
                      target_type(mixerlineA.Target.dwType));
                trace("        Device=%d (%s) %d.%d (%d:%d)\n",
                      mixerlineA.Target.dwDeviceID,
                      mixerlineA.Target.szPname,
                      mixerlineA.Target.vDriverVersion >> 8,
                      mixerlineA.Target.vDriverVersion & 0xff,
                      mixerlineA.Target.wMid, mixerlineA.Target.wPid);
	      }
              ns=mixerlineA.cConnections;
              for(s=0;s<ns;s++) {
                mixerlineA.cbStruct = sizeof(mixerlineA);
                mixerlineA.dwDestination=d;
                mixerlineA.dwSource=s;
                rc = mixerGetLineInfoA(mix, &mixerlineA, MIXER_GETLINEINFOF_SOURCE);
                ok(rc==MMSYSERR_NOERROR||rc==MMSYSERR_NODRIVER,
                   "mixerGetLineInfoA(MIXER_GETLINEINFOF_SOURCE): "
                   "MMSYSERR_NOERROR expected, got %s\n",
                   mmsys_error(rc));
                if (rc==MMSYSERR_NODRIVER)
                    trace("  No Driver\n");
                else if (rc==MMSYSERR_NOERROR) {
                    LPMIXERCONTROLA    array;
                    MIXERLINECONTROLSA controls;
                    if (winetest_interactive) {
                        trace("      %d: \"%s\" (%s) Destination=%d Source=%d\n",
                              s,mixerlineA.szShortName, mixerlineA.szName,
                              mixerlineA.dwDestination,mixerlineA.dwSource);
                        trace("          LineID=%08x Channels=%d "
                              "Connections=%d Controls=%d\n",
                              mixerlineA.dwLineID,mixerlineA.cChannels,
                              mixerlineA.cConnections,mixerlineA.cControls);
                        trace("          State=0x%08x(%s)\n",
                              mixerlineA.fdwLine,line_flags(mixerlineA.fdwLine));
                        trace("          ComponentType=%s\n",
                              component_type(mixerlineA.dwComponentType));
                        trace("          Type=%s\n",
                              target_type(mixerlineA.Target.dwType));
                        trace("          Device=%d (%s) %d.%d (%d:%d)\n",
                              mixerlineA.Target.dwDeviceID,
                              mixerlineA.Target.szPname,
                              mixerlineA.Target.vDriverVersion >> 8,
                              mixerlineA.Target.vDriverVersion & 0xff,
                              mixerlineA.Target.wMid, mixerlineA.Target.wPid);
                    }
                    if (mixerlineA.cControls) {
                        array=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,
                            mixerlineA.cControls*sizeof(MIXERCONTROLA));
                        if (array) {
                            memset(&controls, 0, sizeof(controls));

                            rc = mixerGetLineControlsA(mix, 0, MIXER_GETLINECONTROLSF_ALL);
                            ok(rc==MMSYSERR_INVALPARAM,
                               "mixerGetLineControlsA(MIXER_GETLINECONTROLSF_ALL): "
                               "MMSYSERR_INVALPARAM expected, got %s\n",
                               mmsys_error(rc));

                            rc = mixerGetLineControlsA(mix, &controls, -1);
                            ok(rc==MMSYSERR_INVALFLAG||rc==MMSYSERR_INVALPARAM,
                               "mixerGetLineControlsA(-1): "
                               "MMSYSERR_INVALFLAG or MMSYSERR_INVALPARAM expected, got %s\n",
                               mmsys_error(rc));

                            controls.cbStruct = sizeof(MIXERLINECONTROLSA);
                            controls.cControls = mixerlineA.cControls;
                            controls.dwLineID = mixerlineA.dwLineID;
                            controls.pamxctrl = array;
                            controls.cbmxctrl = sizeof(MIXERCONTROLA);

                            /* FIXME: do MIXER_GETLINECONTROLSF_ONEBYID
                             * and MIXER_GETLINECONTROLSF_ONEBYTYPE
                             */
                            rc = mixerGetLineControlsA(mix, &controls, MIXER_GETLINECONTROLSF_ALL);
                            ok(rc==MMSYSERR_NOERROR,
                               "mixerGetLineControlsA(MIXER_GETLINECONTROLSF_ALL): "
                               "MMSYSERR_NOERROR expected, got %s\n",
                               mmsys_error(rc));
                            if (rc==MMSYSERR_NOERROR) {
                                for(nc=0;nc<mixerlineA.cControls;nc++) {
                                    if (winetest_interactive) {
                                        trace("        %d: \"%s\" (%s) ControlID=%d\n", nc,
                                              array[nc].szShortName,
                                              array[nc].szName, array[nc].dwControlID);
                                        trace("            ControlType=%s\n",
                                               control_type(array[nc].dwControlType));
                                        trace("            Control=0x%08x(%s)\n",
                                              array[nc].fdwControl,
                                              control_flags(array[nc].fdwControl));
                                        trace("            Items=%d Min=%d Max=%d Step=%d\n",
                                              array[nc].cMultipleItems,
                                              S1(array[nc].Bounds).dwMinimum,
                                              S1(array[nc].Bounds).dwMaximum,
                                              array[nc].Metrics.cSteps);
                                    }

                                    mixer_test_controlA(mix, &array[nc]);
                                }
                            }

                            HeapFree(GetProcessHeap(),0,array);
                        }
                    }
                }
              }
            }
        }
        test_mixerClose((HMIXER)mix);
    }
}

static void mixer_test_controlW(HMIXEROBJ mix, MIXERCONTROLW *control)
{
    MMRESULT rc;

    if ((control->dwControlType == MIXERCONTROL_CONTROLTYPE_VOLUME) ||
        (control->dwControlType == MIXERCONTROL_CONTROLTYPE_UNSIGNED)) {
        MIXERCONTROLDETAILS details;
        MIXERCONTROLDETAILS_UNSIGNED value;

        details.cbStruct = sizeof(MIXERCONTROLDETAILS);
        details.dwControlID = control->dwControlID;
        details.cChannels = 1;
        U(details).cMultipleItems = 0;
        details.paDetails = &value;
        details.cbDetails = sizeof(value);

        /* read the current control value */
        rc = mixerGetControlDetailsW(mix, &details, MIXER_GETCONTROLDETAILSF_VALUE);
        ok(rc==MMSYSERR_NOERROR,"mixerGetControlDetails(MIXER_GETCONTROLDETAILSF_VALUE): "
           "MMSYSERR_NOERROR expected, got %s\n",
           mmsys_error(rc));
        if (rc==MMSYSERR_NOERROR && winetest_interactive) {
            MIXERCONTROLDETAILS new_details;
            MIXERCONTROLDETAILS_UNSIGNED new_value;

            trace("            Value=%d\n",value.dwValue);

            if (value.dwValue + control->Metrics.cSteps < S1(control->Bounds).dwMaximum)
                new_value.dwValue = value.dwValue + control->Metrics.cSteps;
            else
                new_value.dwValue = value.dwValue - control->Metrics.cSteps;

            new_details.cbStruct = sizeof(MIXERCONTROLDETAILS);
            new_details.dwControlID = control->dwControlID;
            new_details.cChannels = 1;
            U(new_details).cMultipleItems = 0;
            new_details.paDetails = &new_value;
            new_details.cbDetails = sizeof(new_value);

            /* change the control value by one step */
            rc = mixerSetControlDetails(mix, &new_details, MIXER_SETCONTROLDETAILSF_VALUE);
            ok(rc==MMSYSERR_NOERROR,"mixerSetControlDetails(MIXER_SETCONTROLDETAILSF_VALUE): "
               "MMSYSERR_NOERROR expected, got %s\n",
               mmsys_error(rc));
            if (rc==MMSYSERR_NOERROR) {
                MIXERCONTROLDETAILS ret_details;
                MIXERCONTROLDETAILS_UNSIGNED ret_value;

                ret_details.cbStruct = sizeof(MIXERCONTROLDETAILS);
                ret_details.dwControlID = control->dwControlID;
                ret_details.cChannels = 1;
                U(ret_details).cMultipleItems = 0;
                ret_details.paDetails = &ret_value;
                ret_details.cbDetails = sizeof(ret_value);

                /* read back the new control value */
                rc = mixerGetControlDetailsW(mix, &ret_details, MIXER_GETCONTROLDETAILSF_VALUE);
                ok(rc==MMSYSERR_NOERROR,"mixerGetControlDetails(MIXER_GETCONTROLDETAILSF_VALUE): "
                   "MMSYSERR_NOERROR expected, got %s\n",
                   mmsys_error(rc));
                if (rc==MMSYSERR_NOERROR) {
                    /* result may not match exactly because of rounding */
                    ok(abs(ret_value.dwValue-new_value.dwValue)<=1,
                       "Couldn't change value from %d to %d, returned %d\n",
                       value.dwValue,new_value.dwValue,ret_value.dwValue);

                    if (abs(ret_value.dwValue-new_value.dwValue)<=1) {
                        details.cbStruct = sizeof(MIXERCONTROLDETAILS);
                        details.dwControlID = control->dwControlID;
                        details.cChannels = 1;
                        U(details).cMultipleItems = 0;
                        details.paDetails = &value;
                        details.cbDetails = sizeof(value);

                        /* restore original value */
                        rc = mixerSetControlDetails(mix, &details, MIXER_SETCONTROLDETAILSF_VALUE);
                        ok(rc==MMSYSERR_NOERROR,"mixerSetControlDetails(MIXER_SETCONTROLDETAILSF_VALUE): "
                           "MMSYSERR_NOERROR expected, got %s\n",
                           mmsys_error(rc));
                    }
                }
            }
        }
    } else if ((control->dwControlType == MIXERCONTROL_CONTROLTYPE_MUTE) ||
        (control->dwControlType == MIXERCONTROL_CONTROLTYPE_BOOLEAN) ||
        (control->dwControlType == MIXERCONTROL_CONTROLTYPE_BUTTON)) {
        MIXERCONTROLDETAILS details;
        MIXERCONTROLDETAILS_BOOLEAN value;

        details.cbStruct = sizeof(MIXERCONTROLDETAILS);
        details.dwControlID = control->dwControlID;
        details.cChannels = 1;
        U(details).cMultipleItems = 0;
        details.paDetails = &value;
        details.cbDetails = sizeof(value);

        rc = mixerGetControlDetailsW(mix, &details, MIXER_GETCONTROLDETAILSF_VALUE);
        ok(rc==MMSYSERR_NOERROR,"mixerGetControlDetails(MIXER_GETCONTROLDETAILSF_VALUE): "
           "MMSYSERR_NOERROR expected, got %s\n",
           mmsys_error(rc));
        if (rc==MMSYSERR_NOERROR && winetest_interactive) {
            MIXERCONTROLDETAILS new_details;
            MIXERCONTROLDETAILS_BOOLEAN new_value;

            trace("            Value=%d\n",value.fValue);

            if (value.fValue == FALSE)
                new_value.fValue = TRUE;
            else
                new_value.fValue = FALSE;

            new_details.cbStruct = sizeof(MIXERCONTROLDETAILS);
            new_details.dwControlID = control->dwControlID;
            new_details.cChannels = 1;
            U(new_details).cMultipleItems = 0;
            new_details.paDetails = &new_value;
            new_details.cbDetails = sizeof(new_value);

            /* change the control value by one step */
            rc = mixerSetControlDetails(mix, &new_details, MIXER_SETCONTROLDETAILSF_VALUE);
            ok(rc==MMSYSERR_NOERROR,"mixerSetControlDetails(MIXER_SETCONTROLDETAILSF_VALUE): "
               "MMSYSERR_NOERROR expected, got %s\n",
               mmsys_error(rc));
            if (rc==MMSYSERR_NOERROR) {
                MIXERCONTROLDETAILS ret_details;
                MIXERCONTROLDETAILS_BOOLEAN ret_value;

                ret_details.cbStruct = sizeof(MIXERCONTROLDETAILS);
                ret_details.dwControlID = control->dwControlID;
                ret_details.cChannels = 1;
                U(ret_details).cMultipleItems = 0;
                ret_details.paDetails = &ret_value;
                ret_details.cbDetails = sizeof(ret_value);

                /* read back the new control value */
                rc = mixerGetControlDetailsW(mix, &ret_details, MIXER_GETCONTROLDETAILSF_VALUE);
                ok(rc==MMSYSERR_NOERROR,"mixerGetControlDetails(MIXER_GETCONTROLDETAILSF_VALUE): "
                   "MMSYSERR_NOERROR expected, got %s\n",
                   mmsys_error(rc));
                if (rc==MMSYSERR_NOERROR) {
                    /* result may not match exactly because of rounding */
                    ok(ret_value.fValue==new_value.fValue,
                       "Couldn't change value from %d to %d, returned %d\n",
                       value.fValue,new_value.fValue,ret_value.fValue);

                    if (ret_value.fValue==new_value.fValue) {
                        details.cbStruct = sizeof(MIXERCONTROLDETAILS);
                        details.dwControlID = control->dwControlID;
                        details.cChannels = 1;
                        U(details).cMultipleItems = 0;
                        details.paDetails = &value;
                        details.cbDetails = sizeof(value);

                        /* restore original value */
                        rc = mixerSetControlDetails(mix, &details, MIXER_SETCONTROLDETAILSF_VALUE);
                        ok(rc==MMSYSERR_NOERROR,"mixerSetControlDetails(MIXER_SETCONTROLDETAILSF_VALUE): "
                           "MMSYSERR_NOERROR expected, got %s\n",
                           mmsys_error(rc));
                    }
                }
            }
        }
    } else {
        /* FIXME */
    }
}

static void mixer_test_deviceW(int device)
{
    MIXERCAPSW capsW;
    HMIXEROBJ mix;
    MMRESULT rc;
    DWORD d,s,ns,nc;
    char szShortName[MIXER_SHORT_NAME_CHARS];
    char szName[MIXER_LONG_NAME_CHARS];
    char szPname[MAXPNAMELEN];

    rc=mixerGetDevCapsW(device,0,sizeof(capsW));
    ok(rc==MMSYSERR_INVALPARAM,
       "mixerGetDevCapsW: MMSYSERR_INVALPARAM expected, got %s\n",
       mmsys_error(rc));

    rc=mixerGetDevCapsW(device,&capsW,4);
    ok(rc==MMSYSERR_NOERROR ||
       rc==MMSYSERR_INVALPARAM, /* Vista and W2K8 */
       "mixerGetDevCapsW: MMSYSERR_NOERROR or MMSYSERR_INVALPARAM expected, got %s\n",
       mmsys_error(rc));

    rc=mixerGetDevCapsW(device,&capsW,sizeof(capsW));
    ok(rc==MMSYSERR_NOERROR,
       "mixerGetDevCapsW: MMSYSERR_NOERROR expected, got %s\n",
       mmsys_error(rc));

    WideCharToMultiByte(CP_ACP,0,capsW.szPname, MAXPNAMELEN,szPname,
                        MAXPNAMELEN,NULL,NULL);
    if (winetest_interactive) {
        trace("  %d: \"%s\" %d.%d (%d:%d) destinations=%d\n", device,
              szPname, capsW.vDriverVersion >> 8,
              capsW.vDriverVersion & 0xff,capsW.wMid,capsW.wPid,
              capsW.cDestinations);
    } else {
        trace("  %d: \"%s\" %d.%d (%d:%d)\n", device,
              szPname, capsW.vDriverVersion >> 8,
              capsW.vDriverVersion & 0xff,capsW.wMid,capsW.wPid);
    }


    rc = mixerOpen((HMIXER*)&mix, device, 0, 0, 0);
    ok(rc==MMSYSERR_NOERROR,
       "mixerOpen: MMSYSERR_NOERROR expected, got %s\n",mmsys_error(rc));
    if (rc==MMSYSERR_NOERROR) {
        MIXERCAPSW capsW2;

        rc=mixerGetDevCapsW((UINT_PTR)mix,&capsW2,sizeof(capsW2));
        ok(rc==MMSYSERR_NOERROR,
           "mixerGetDevCapsW: MMSYSERR_NOERROR expected, got %s\n",
           mmsys_error(rc));
        ok(!lstrcmpW(capsW2.szPname, capsW.szPname), "Got wrong device caps\n");

        for (d=0;d<capsW.cDestinations;d++) {
            MIXERLINEW mixerlineW;
            mixerlineW.cbStruct = 0;
            mixerlineW.dwDestination=d;
            rc = mixerGetLineInfoW(mix, &mixerlineW, MIXER_GETLINEINFOF_DESTINATION);
            ok(rc==MMSYSERR_INVALPARAM,
               "mixerGetLineInfoW(MIXER_GETLINEINFOF_DESTINATION): "
               "MMSYSERR_INVALPARAM expected, got %s\n",
               mmsys_error(rc));

            mixerlineW.cbStruct = sizeof(mixerlineW);
            mixerlineW.dwDestination=capsW.cDestinations;
            rc = mixerGetLineInfoW(mix, &mixerlineW, MIXER_GETLINEINFOF_DESTINATION);
            ok(rc==MMSYSERR_INVALPARAM||rc==MIXERR_INVALLINE,
               "mixerGetLineInfoW(MIXER_GETLINEINFOF_DESTINATION): "
               "MMSYSERR_INVALPARAM or MIXERR_INVALLINE expected, got %s\n",
               mmsys_error(rc));

            mixerlineW.cbStruct = sizeof(mixerlineW);
            mixerlineW.dwDestination=d;
            rc = mixerGetLineInfoW(mix, 0, MIXER_GETLINEINFOF_DESTINATION);
            ok(rc==MMSYSERR_INVALPARAM,
               "mixerGetLineInfoW(MIXER_GETLINEINFOF_DESTINATION): "
               "MMSYSERR_INVALPARAM expected, got %s\n",
               mmsys_error(rc));

            mixerlineW.cbStruct = sizeof(mixerlineW);
            mixerlineW.dwDestination=d;
            rc = mixerGetLineInfoW(mix, &mixerlineW, -1);
            ok(rc==MMSYSERR_INVALFLAG,
               "mixerGetLineInfoW(-1): MMSYSERR_INVALFLAG expected, got %s\n",
               mmsys_error(rc));

            mixerlineW.cbStruct = sizeof(mixerlineW);
            mixerlineW.dwDestination=d;
            rc = mixerGetLineInfoW(mix, &mixerlineW, MIXER_GETLINEINFOF_DESTINATION);
            ok(rc==MMSYSERR_NOERROR||rc==MMSYSERR_NODRIVER,
               "mixerGetLineInfoW(MIXER_GETLINEINFOF_DESTINATION): "
               "MMSYSERR_NOERROR expected, got %s\n",
               mmsys_error(rc));
            if (rc==MMSYSERR_NODRIVER)
                trace("  No Driver\n");
            else if (rc==MMSYSERR_NOERROR && winetest_interactive) {
                WideCharToMultiByte(CP_ACP,0,mixerlineW.szShortName,
                    MIXER_SHORT_NAME_CHARS,szShortName,
                    MIXER_SHORT_NAME_CHARS,NULL,NULL);
                WideCharToMultiByte(CP_ACP,0,mixerlineW.szName,
                    MIXER_LONG_NAME_CHARS,szName,
                    MIXER_LONG_NAME_CHARS,NULL,NULL);
                WideCharToMultiByte(CP_ACP,0,mixerlineW.Target.szPname,
                    MAXPNAMELEN,szPname,
                    MAXPNAMELEN,NULL, NULL);
                trace("    %d: \"%s\" (%s) Destination=%d Source=%d\n",
                      d,szShortName,szName,
                      mixerlineW.dwDestination,mixerlineW.dwSource);
                trace("        LineID=%08x Channels=%d "
                      "Connections=%d Controls=%d\n",
                      mixerlineW.dwLineID,mixerlineW.cChannels,
                      mixerlineW.cConnections,mixerlineW.cControls);
                trace("        State=0x%08x(%s)\n",
                      mixerlineW.fdwLine,line_flags(mixerlineW.fdwLine));
                trace("        ComponentType=%s\n",
                      component_type(mixerlineW.dwComponentType));
                trace("        Type=%s\n",
                      target_type(mixerlineW.Target.dwType));
                trace("        Device=%d (%s) %d.%d (%d:%d)\n",
                      mixerlineW.Target.dwDeviceID,szPname,
                      mixerlineW.Target.vDriverVersion >> 8,
                      mixerlineW.Target.vDriverVersion & 0xff,
                      mixerlineW.Target.wMid, mixerlineW.Target.wPid);
            }
            ns=mixerlineW.cConnections;
            for(s=0;s<ns;s++) {
                mixerlineW.cbStruct = sizeof(mixerlineW);
                mixerlineW.dwDestination=d;
                mixerlineW.dwSource=s;
                rc = mixerGetLineInfoW(mix, &mixerlineW, MIXER_GETLINEINFOF_SOURCE);
                ok(rc==MMSYSERR_NOERROR||rc==MMSYSERR_NODRIVER,
                   "mixerGetLineInfoW(MIXER_GETLINEINFOF_SOURCE): "
                   "MMSYSERR_NOERROR expected, got %s\n",
                   mmsys_error(rc));
                if (rc==MMSYSERR_NODRIVER)
                    trace("  No Driver\n");
                else if (rc==MMSYSERR_NOERROR) {
                    LPMIXERCONTROLW    array;
                    MIXERLINECONTROLSW controls;
                    if (winetest_interactive) {
                        WideCharToMultiByte(CP_ACP,0,mixerlineW.szShortName,
                            MIXER_SHORT_NAME_CHARS,szShortName,
                            MIXER_SHORT_NAME_CHARS,NULL,NULL);
                        WideCharToMultiByte(CP_ACP,0,mixerlineW.szName,
                            MIXER_LONG_NAME_CHARS,szName,
                            MIXER_LONG_NAME_CHARS,NULL,NULL);
                        WideCharToMultiByte(CP_ACP,0,mixerlineW.Target.szPname,
                            MAXPNAMELEN,szPname,
                            MAXPNAMELEN,NULL, NULL);
                        trace("      %d: \"%s\" (%s) Destination=%d Source=%d\n",
                              s,szShortName,szName,
                              mixerlineW.dwDestination,mixerlineW.dwSource);
                        trace("          LineID=%08x Channels=%d "
                              "Connections=%d Controls=%d\n",
                              mixerlineW.dwLineID,mixerlineW.cChannels,
                              mixerlineW.cConnections,mixerlineW.cControls);
                        trace("          State=0x%08x(%s)\n",
                              mixerlineW.fdwLine,line_flags(mixerlineW.fdwLine));
                        trace("          ComponentType=%s\n",
                              component_type(mixerlineW.dwComponentType));
                        trace("          Type=%s\n",
                              target_type(mixerlineW.Target.dwType));
                        trace("          Device=%d (%s) %d.%d (%d:%d)\n",
                              mixerlineW.Target.dwDeviceID,szPname,
                              mixerlineW.Target.vDriverVersion >> 8,
                              mixerlineW.Target.vDriverVersion & 0xff,
                              mixerlineW.Target.wMid, mixerlineW.Target.wPid);
                    }
                    if (mixerlineW.cControls) {
                        array=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,
                            mixerlineW.cControls*sizeof(MIXERCONTROLW));
                        if (array) {
                            rc = mixerGetLineControlsW(mix, 0, MIXER_GETLINECONTROLSF_ALL);
                            ok(rc==MMSYSERR_INVALPARAM,
                               "mixerGetLineControlsW(MIXER_GETLINECONTROLSF_ALL): "
                               "MMSYSERR_INVALPARAM expected, got %s\n",
                               mmsys_error(rc));
                            rc = mixerGetLineControlsW(mix, &controls, -1);
                            ok(rc==MMSYSERR_INVALFLAG||rc==MMSYSERR_INVALPARAM,
                               "mixerGetLineControlsW(-1): "
                               "MMSYSERR_INVALFLAG or MMSYSERR_INVALPARAM expected, got %s\n",
                               mmsys_error(rc));

                            controls.cbStruct = sizeof(MIXERLINECONTROLSW);
                            controls.cControls = mixerlineW.cControls;
                            controls.dwLineID = mixerlineW.dwLineID;
                            controls.pamxctrl = array;
                            controls.cbmxctrl = sizeof(MIXERCONTROLW);

                            /* FIXME: do MIXER_GETLINECONTROLSF_ONEBYID
                             * and MIXER_GETLINECONTROLSF_ONEBYTYPE
                             */
                            rc = mixerGetLineControlsW(mix, &controls, MIXER_GETLINECONTROLSF_ALL);
                            ok(rc==MMSYSERR_NOERROR,
                               "mixerGetLineControlsW(MIXER_GETLINECONTROLSF_ALL): "
                               "MMSYSERR_NOERROR expected, got %s\n",
                               mmsys_error(rc));
                            if (rc==MMSYSERR_NOERROR) {
                                for(nc=0;nc<mixerlineW.cControls;nc++) {
                                    if (winetest_interactive) {
                                        WideCharToMultiByte(CP_ACP,0,array[nc].szShortName,
                                            MIXER_SHORT_NAME_CHARS,szShortName,
                                            MIXER_SHORT_NAME_CHARS,NULL,NULL);
                                        WideCharToMultiByte(CP_ACP,0,array[nc].szName,
                                            MIXER_LONG_NAME_CHARS,szName,
                                            MIXER_LONG_NAME_CHARS,NULL,NULL);
                                        trace("        %d: \"%s\" (%s) ControlID=%d\n", nc,
                                              szShortName, szName, array[nc].dwControlID);
                                        trace("            ControlType=%s\n",
                                               control_type(array[nc].dwControlType));
                                        trace("            Control=0x%08x(%s)\n",
                                              array[nc].fdwControl,
                                              control_flags(array[nc].fdwControl));
                                        trace("            Items=%d Min=%d Max=%d Step=%d\n",
                                              array[nc].cMultipleItems,
                                              S1(array[nc].Bounds).dwMinimum,
                                              S1(array[nc].Bounds).dwMaximum,
                                              array[nc].Metrics.cSteps);
                                    }
                                    mixer_test_controlW(mix, &array[nc]);
                                }
                            }

                            HeapFree(GetProcessHeap(),0,array);
                        }
                    }
                }
            }
        }
        test_mixerClose((HMIXER)mix);
    }
}

static void mixer_testsA(void)
{
    MIXERCAPSA capsA;
    MMRESULT rc;
    UINT ndev, d;

    trace("--- Testing ASCII functions ---\n");

    ndev=mixerGetNumDevs();
    trace("found %d Mixer devices\n",ndev);

    rc=mixerGetDevCapsA(ndev+1,&capsA,sizeof(capsA));
    ok(rc==MMSYSERR_BADDEVICEID,
       "mixerGetDevCapsA: MMSYSERR_BADDEVICEID expected, got %s\n",
       mmsys_error(rc));

    for (d=0;d<ndev;d++)
        mixer_test_deviceA(d);
}

static void mixer_testsW(void)
{
    MIXERCAPSW capsW;
    MMRESULT rc;
    UINT ndev, d;

    trace("--- Testing WCHAR functions ---\n");

    ndev=mixerGetNumDevs();
    trace("found %d Mixer devices\n",ndev);

    rc=mixerGetDevCapsW(ndev+1,&capsW,sizeof(capsW));
    ok(rc==MMSYSERR_BADDEVICEID||rc==MMSYSERR_NOTSUPPORTED,
       "mixerGetDevCapsW: MMSYSERR_BADDEVICEID or MMSYSERR_NOTSUPPORTED "
       "expected, got %s\n", mmsys_error(rc));
    if (rc==MMSYSERR_NOTSUPPORTED)
        return;

    for (d=0;d<ndev;d++)
        mixer_test_deviceW(d);
}

static void test_mixerOpen(void)
{
    HMIXER mix;
    HANDLE event;
    MMRESULT rc;
    UINT ndev, d;

    ndev = mixerGetNumDevs();

    /* Test mixerOpen with invalid device ID values. */
    rc = mixerOpen(&mix, ndev + 1, 0, 0, 0);
    ok(rc == MMSYSERR_BADDEVICEID,
       "mixerOpen: MMSYSERR_BADDEVICEID expected, got %s\n",
       mmsys_error(rc));

    rc = mixerOpen(&mix, -1, 0, 0, 0);
    ok(rc == MMSYSERR_BADDEVICEID ||
       rc == MMSYSERR_INVALHANDLE, /* NT4/W2K */
       "mixerOpen: MMSYSERR_BADDEVICEID or MMSYSERR_INVALHANDLE expected, got %s\n",
       mmsys_error(rc));

    for (d = 0; d < ndev; d++) {
        /* Test mixerOpen with valid device ID values and invalid parameters. */
        rc = mixerOpen(&mix, d, 0, 0, CALLBACK_FUNCTION);
        ok(rc == MMSYSERR_INVALFLAG
           || rc == MMSYSERR_NOTSUPPORTED, /* 98/ME */
           "mixerOpen: MMSYSERR_INVALFLAG expected, got %s\n",
           mmsys_error(rc));

        rc = mixerOpen(&mix, d, 0xdeadbeef, 0, CALLBACK_WINDOW);
        ok(rc == MMSYSERR_INVALPARAM ||
           broken(rc == MMSYSERR_NOERROR /* 98 */),
           "mixerOpen: MMSYSERR_INVALPARAM expected, got %s\n",
           mmsys_error(rc));
        if (rc == MMSYSERR_NOERROR)
            test_mixerClose(mix);

        /* Test mixerOpen with a NULL dwCallback and CALLBACK_WINDOW flag. */
        rc = mixerOpen(&mix, d, 0, 0, CALLBACK_WINDOW);
        ok(rc == MMSYSERR_NOERROR,
           "mixerOpen: MMSYSERR_NOERROR expected, got %s\n",
           mmsys_error(rc));
        if (rc == MMSYSERR_NOERROR)
            test_mixerClose(mix);

        rc = mixerOpen(&mix, d, 0, 0, CALLBACK_THREAD);
        ok(rc == MMSYSERR_NOERROR /* since w2k */ ||
           rc == MMSYSERR_NOTSUPPORTED, /* 98 */
           "mixerOpen: MMSYSERR_NOERROR expected, got %s\n",
           mmsys_error(rc));
        if (rc == MMSYSERR_NOERROR)
            test_mixerClose(mix);

        rc = mixerOpen(&mix, d, 0, 0, CALLBACK_EVENT);
        ok(rc == MMSYSERR_NOERROR /* since w2k */ ||
           rc == MMSYSERR_NOTSUPPORTED, /* 98 */
           "mixerOpen: MMSYSERR_NOERROR expected, got %s\n",
           mmsys_error(rc));
        if (rc == MMSYSERR_NOERROR)
            test_mixerClose(mix);

        event = CreateEventW(NULL, FALSE, FALSE, NULL);

        /* NOTSUPPORTED is not broken, but it enables the todo_wine marker. */
        rc = mixerOpen(&mix, d, (DWORD_PTR)event, 0, CALLBACK_EVENT);
        todo_wine
        ok(rc == MMSYSERR_NOERROR /* since w2k */ ||
           broken(rc == MMSYSERR_NOTSUPPORTED), /* 98 */
           "mixerOpen: MMSYSERR_NOERROR expected, got %s\n",
           mmsys_error(rc));
        if (rc == MMSYSERR_NOERROR)
            test_mixerClose(mix);

        /* Test mixerOpen with normal parameters. */
        rc = mixerOpen(&mix, d, 0, 0, 0);
        ok(rc == MMSYSERR_NOERROR,
           "mixerOpen: MMSYSERR_NOERROR expected, got %s\n",
           mmsys_error(rc));

        if (rc == MMSYSERR_NOERROR)
            test_mixerClose(mix);

        rc = WaitForSingleObject(event, 0);
        ok(rc == WAIT_TIMEOUT, "WaitEvent %d\n", rc);
        CloseHandle(event);
    }
}

START_TEST(mixer)
{
    test_mixerOpen();
    mixer_testsA();
    mixer_testsW();
}
