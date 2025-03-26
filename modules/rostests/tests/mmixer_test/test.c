#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <setupapi.h>
#include <ks.h>
#include <ksmedia.h>
#include <mmsystem.h>
#include <mmreg.h>

HMIXER hMixer;
HANDLE hThread;
HWND hwnd;

DWORD
WINAPI
MixerThread(LPVOID Parameter)
{

    MSG msg;

	printf("hMixer %p\n", hMixer);

	while(GetMessage(&msg, NULL, 0, 0))
	{
			if (msg.message == MM_MIXM_CONTROL_CHANGE)
				printf("got MM_MIXM_CONTROL_CHANGE wParam %Ix lParam %Ix\n", msg.wParam, msg.lParam);
			else if (msg.message == MM_MIXM_LINE_CHANGE)
				printf("got MM_MIXM_LINE_CHANGE wParam %Ix lParam %Ix\n", msg.wParam, msg.lParam);
	}
	return 1;
}

VOID
test()
{


    hwnd = CreateWindowExA(0, "static", "winmm test", WS_POPUP, 0,0,100,100,
                           0, 0, 0, NULL);


	if (!hwnd) {
		printf("failed to create window\n");
		exit(-1);
	}

	printf("window created\n");

	if (mixerOpen(&hMixer, 0, (DWORD_PTR)hwnd, 0, CALLBACK_WINDOW | MIXER_OBJECTF_MIXER) != MMSYSERR_NOERROR)
	{
		printf("failed to create mixer\n");
		exit(-2);
	}

	hThread = CreateThread(NULL, 0, MixerThread, NULL, 0, NULL);

	if (hThread == NULL)
	{
		printf("failed to create thread\n");
		exit(-3);
	}
}






void
printMixerLine(LPMIXERLINEW MixerLine, IN ULONG MixerIndex)
{
    MIXERLINECONTROLSW MixerLineControls;
    LPMIXERCONTROLDETAILS_LISTTEXTW ListText;
    MIXERCONTROLDETAILS MixerControlDetails;
    ULONG Index, SubIndex;
    MMRESULT Result;

    printf("\n");
    printf("cChannels %lu\n", MixerLine->cChannels);
    printf("cConnections %lu\n", MixerLine->cConnections);
    printf("cControls %lu\n", MixerLine->cControls);
    printf("dwComponentType %lx\n", MixerLine->dwComponentType);
    printf("dwDestination %lu\n", MixerLine->dwDestination);
    printf("dwLineID %lx\n", MixerLine->dwLineID);
    printf("dwSource %lx\n", MixerLine->dwSource);
    printf("dwUser %Iu\n", MixerLine->dwUser);
    printf("fdwLine %lu\n", MixerLine->fdwLine);
    printf("szName %S\n", MixerLine->szName);
    printf("szShortName %S\n", MixerLine->szShortName);
    printf("Target.dwDeviceId %lu\n", MixerLine->Target.dwDeviceID);
    printf("Target.dwType %lu\n", MixerLine->Target.dwType);
    printf("Target.szName %S\n", MixerLine->Target.szPname);
    printf("Target.vDriverVersion %x\n", MixerLine->Target.vDriverVersion);
    printf("Target.wMid %x\n", MixerLine->Target.wMid );
    printf("Target.wPid %x\n", MixerLine->Target.wPid);

    MixerLineControls.cbStruct = sizeof(MixerLineControls);
    MixerLineControls.dwLineID = MixerLine->dwLineID;
    MixerLineControls.cControls = MixerLine->cControls;
    MixerLineControls.cbmxctrl= sizeof(MIXERCONTROLW);
    MixerLineControls.pamxctrl = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MixerLineControls.cControls * sizeof(MIXERCONTROLW));

    Result = mixerGetLineControlsW((HMIXEROBJ)UlongToHandle(MixerIndex), &MixerLineControls, MIXER_GETLINECONTROLSF_ALL | MIXER_OBJECTF_MIXER);

    printf("Result %u\n", Result);

    for(Index = 0; Index < MixerLine->cControls; Index++)
    {
        printf("\n");
        printf("Control Index: %lu\n", Index);
        printf("\n");
        printf("cbStruct %lu\n", MixerLineControls.pamxctrl[Index].cbStruct);
        printf("dwControlID %lu\n", MixerLineControls.pamxctrl[Index].dwControlID);
        printf("dwControlType %lx\n", MixerLineControls.pamxctrl[Index].dwControlType);
        printf("fdwControl %lu\n", MixerLineControls.pamxctrl[Index].fdwControl);
        printf("cMultipleItems %lu\n", MixerLineControls.pamxctrl[Index].cMultipleItems);
        printf("szShortName %S\n", MixerLineControls.pamxctrl[Index].szShortName);
        printf("szName %S\n", MixerLineControls.pamxctrl[Index].szName);
        printf("Bounds.dwMinimum %lu\n", MixerLineControls.pamxctrl[Index].Bounds.dwMinimum);
        printf("Bounds.dwMaximum %lu\n", MixerLineControls.pamxctrl[Index].Bounds.dwMaximum);

        printf("Metrics.Reserved[0] %lu\n", MixerLineControls.pamxctrl[Index].Metrics.dwReserved[0]);
        printf("Metrics.Reserved[1] %lu\n", MixerLineControls.pamxctrl[Index].Metrics.dwReserved[1]);
        printf("Metrics.Reserved[2] %lu\n", MixerLineControls.pamxctrl[Index].Metrics.dwReserved[2]);
        printf("Metrics.Reserved[3] %lu\n", MixerLineControls.pamxctrl[Index].Metrics.dwReserved[3]);
        printf("Metrics.Reserved[4] %lu\n", MixerLineControls.pamxctrl[Index].Metrics.dwReserved[4]);
        printf("Metrics.Reserved[5] %lu\n", MixerLineControls.pamxctrl[Index].Metrics.dwReserved[5]);

        if (MixerLineControls.pamxctrl[Index].dwControlType == MIXERCONTROL_CONTROLTYPE_MUX)
        {
            ZeroMemory(&MixerControlDetails, sizeof(MixerControlDetails));
            MixerControlDetails.cbStruct = sizeof(MIXERCONTROLDETAILS);
            MixerControlDetails.cbDetails = sizeof(MIXERCONTROLDETAILS_LISTTEXTW);
            MixerControlDetails.cChannels = 1;
            MixerControlDetails.cMultipleItems = MixerLineControls.pamxctrl[Index].Metrics.dwReserved[0];
            MixerControlDetails.dwControlID = MixerLineControls.pamxctrl[Index].dwControlID;
            MixerControlDetails.paDetails = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MixerControlDetails.cbDetails * MixerControlDetails.cChannels * MixerControlDetails.cMultipleItems);

            Result = mixerGetControlDetailsW((HMIXEROBJ)UlongToHandle(MixerIndex), &MixerControlDetails, MIXER_GETCONTROLDETAILSF_LISTTEXT | MIXER_OBJECTF_MIXER);

            printf("Result %x\n", Result);
            ListText = (LPMIXERCONTROLDETAILS_LISTTEXTW)MixerControlDetails.paDetails;
            for(SubIndex = 0; SubIndex < MixerControlDetails.cMultipleItems; SubIndex++)
            {
                printf("dwParam1 %lx\n", ListText[SubIndex].dwParam1);
                printf("dwParam1 %lx\n", ListText[SubIndex].dwParam2);
                printf("szName %S\n", ListText[SubIndex].szName);
            }

			MixerControlDetails.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
			MixerControlDetails.paDetails = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MIXERCONTROLDETAILS_BOOLEAN) * MixerControlDetails.cMultipleItems);
             ((LPMIXERCONTROLDETAILS_BOOLEAN)MixerControlDetails.paDetails)->fValue = TRUE;

			Result = mixerSetControlDetails((HMIXEROBJ)hMixer, &MixerControlDetails, MIXER_SETCONTROLDETAILSF_VALUE | MIXER_OBJECTF_HANDLE);
			printf("Result %x hMixer %p\n", Result, hMixer);
        }
    }
}

int main(int argc, char**argv)
{
    DWORD MixerCount, MixerIndex, DestinationIndex, SrcIndex;
    MIXERCAPSW MixerCaps;
    MIXERLINEW DstMixerLine, SrcLine;
    MMRESULT Result;


test();

    MixerCount = mixerGetNumDevs();

    printf("MixerCount %lu\n", MixerCount);

    for(MixerIndex = 0; MixerIndex < MixerCount; MixerIndex++)
    {
        Result = mixerGetDevCapsW((UINT_PTR)MixerIndex, &MixerCaps, sizeof(MixerCaps));

        printf("\n");
        printf("Index %lu Result %x\n", MixerIndex, Result);
        printf("Name :%S\n", MixerCaps.szPname);
        printf("cDestinations: %lu\n", MixerCaps.cDestinations);
        printf("fdwSupport %lx\n", MixerCaps.fdwSupport);
        printf("vDriverVersion %x\n", MixerCaps.vDriverVersion);
        printf("wMid %x\n", MixerCaps.wMid);
        printf("wPid %x\n", MixerCaps.wPid);

        for(DestinationIndex = 0; DestinationIndex < MixerCaps.cDestinations; DestinationIndex++)
        {
            ZeroMemory(&DstMixerLine, sizeof(DstMixerLine));
            DstMixerLine.dwDestination = DestinationIndex;
            DstMixerLine.cbStruct = sizeof(DstMixerLine);

            Result = mixerGetLineInfoW((HMIXEROBJ)UlongToHandle(MixerIndex), &DstMixerLine, MIXER_GETLINEINFOF_DESTINATION | MIXER_OBJECTF_MIXER);
            printf("\n");
            printf("Destination Index %lu\n", DestinationIndex);
            printMixerLine(&DstMixerLine, MixerIndex);
            for(SrcIndex = 0; SrcIndex < DstMixerLine.cConnections; SrcIndex++)
            {
                ZeroMemory(&SrcLine, sizeof(SrcLine));
                SrcLine.dwDestination = DestinationIndex;
                SrcLine.dwSource = SrcIndex;
                SrcLine.cbStruct = sizeof(SrcLine);

                Result = mixerGetLineInfoW((HMIXEROBJ)UlongToHandle(MixerIndex), &SrcLine, MIXER_GETLINEINFOF_SOURCE  | MIXER_OBJECTF_MIXER);

                if (Result == MMSYSERR_NOERROR)
                {
                    printf("\n");
                    printf("SrcLineIndex %lu\n", SrcIndex);
                    printMixerLine(&SrcLine, MixerIndex);
                }
            }
        }
    }
WaitForSingleObject(hThread, INFINITE);
CloseHandle(hThread);

    return 0;
}
