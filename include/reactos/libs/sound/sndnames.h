/*
    ReactOS Sound System
    NT4 audio device and registry key names

    Author:
        Andrew Greenwood (silverblade@reactos.org)

    History:
        14 Feb 2009 - Split from ntddsnd.h

    These were enhancements to the original NT4 DDK audio device header
    files.
*/

#ifndef SNDNAMES_H
#define SNDNAMES_H

/*
    Base device names (NT4)

    Each device name should be given a numerical suffix identifying that
    unique device, eg:

    \Device\WaveOut0    - First wave output device
    \Device\WaveOut1    - Second wave output device
*/

#define DD_WAVE_IN_DEVICE_NAME              "\\Device\\WaveIn"
#define DD_WAVE_IN_DEVICE_NAME_U           L"\\Device\\WaveIn"
#define DD_WAVE_IN_DOS_DEVICE_NAME          "\\DosDevices\\WaveIn"
#define DD_WAVE_IN_DOS_DEVICE_NAME_U       L"\\DosDevices\\WaveIn"
#define DD_WAVE_IN_WIN32_DEVICE_NAME        "\\\\.\\WaveIn"
#define DD_WAVE_IN_WIN32_DEVICE_NAME_U     L"\\\\.\\WaveIn"

#define DD_WAVE_OUT_DEVICE_NAME             "\\Device\\WaveOut"
#define DD_WAVE_OUT_DEVICE_NAME_U          L"\\Device\\WaveOut"
#define DD_WAVE_OUT_DOS_DEVICE_NAME         "\\DosDevices\\WaveOut"
#define DD_WAVE_OUT_DOS_DEVICE_NAME_U      L"\\DosDevices\\WaveOut"
#define DD_WAVE_OUT_WIN32_DEVICE_NAME       "\\\\.\\WaveOut"
#define DD_WAVE_OUT_WIN32_DEVICE_NAME_U    L"\\\\.\\WaveOut"

#define DD_MIDI_IN_DEVICE_NAME              "\\Device\\MidiIn"
#define DD_MIDI_IN_DEVICE_NAME_U           L"\\Device\\MidiIn"
#define DD_MIDI_IN_DOS_DEVICE_NAME          "\\DosDevices\\MidiIn"
#define DD_MIDI_IN_DOS_DEVICE_NAME_U       L"\\DosDevices\\MidiIn"
#define DD_MIDI_IN_WIN32_DEVICE_NAME        "\\\\.\\MidiIn"
#define DD_MIDI_IN_WIN32_DEVICE_NAME_U     L"\\\\.\\MidiIn"

#define DD_MIDI_OUT_DEVICE_NAME             "\\Device\\MidiOut"
#define DD_MIDI_OUT_DEVICE_NAME_U          L"\\Device\\MidiOut"
#define DD_MIDI_OUT_DOS_DEVICE_NAME         "\\DosDevices\\MidiOut"
#define DD_MIDI_OUT_DOS_DEVICE_NAME_U      L"\\DosDevices\\MidiOut"
#define DD_MIDI_OUT_WIN32_DEVICE_NAME       "\\\\.\\MidiOut"
#define DD_MIDI_OUT_WIN32_DEVICE_NAME_U    L"\\\\.\\MidiOut"

#define DD_MIX_DEVICE_NAME                  "\\Device\\MMMix"
#define DD_MIX_DEVICE_NAME_U               L"\\Device\\MMMix"
#define DD_MIX_DOS_DEVICE_NAME              "\\DosDevices\\MMMix"
#define DD_MIX_DOS_DEVICE_NAME_U           L"\\DosDevices\\MMMix"
#define DD_MIX_WIN32_DEVICE_NAME            "\\\\.\\MMMix"
#define DD_MIX_WIN32_DEVICE_NAME_U         L"\\\\.\\MMMix"

#define DD_AUX_DEVICE_NAME                  "\\Device\\MMAux"
#define DD_AUX_DEVICE_NAME_U               L"\\Device\\MMAux"
#define DD_AUX_DOS_DEVICE_NAME              "\\DosDevices\\MMAux"
#define DD_AUX_DOS_DEVICE_NAME_U           L"\\DosDevices\\MMAux"
#define DD_AUX_WIN32_DEVICE_NAME            "\\\\.\\MMAux"
#define DD_AUX_WIN32_DEVICE_NAME_U         L"\\\\.\\MMAux"


/*
    Registry keys (NT4)
*/

#define REG_SERVICES_KEY_NAME_U            L"System\\CurrentControlSet\\Services"
#define REG_PARAMETERS_KEY_NAME_U          L"Parameters"
#define REG_DEVICE_KEY_NAME_U              L"Device"
#define REG_DEVICES_KEY_NAME_U             L"Devices"

#endif
