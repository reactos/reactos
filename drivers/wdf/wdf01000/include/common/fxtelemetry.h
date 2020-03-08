#ifndef _FXTELEMETRY_H_
#define _FXTELEMETRY_H_

//
// bit-flags for tracking hardware info for device start telemetry event
//
enum FxDeviceInfoFlags : USHORT {
    DeviceInfoLineBasedLevelTriggeredInterrupt =   0x1,
    DeviceInfoLineBasedEdgeTriggeredInterrupt  =   0x2,
    DeviceInfoMsiXOrSingleMsi22Interrupt       =   0x4,
    DeviceInfoMsi22MultiMessageInterrupt       =   0x8,
    DeviceInfoPassiveLevelInterrupt            =  0x10,
    DeviceInfoDmaBusMaster                     =  0x20,
    DeviceInfoDmaSystem                        =  0x40,
    DeviceInfoDmaSystemDuplex                  =  0x80,
    DeviceInfoHasStaticChildren                = 0x100,
    DeviceInfoHasDynamicChildren               = 0x200,
    DeviceInfoIsUsingDriverWppRecorder         = 0x400
};

#endif // _FXTELEMETRY_H_