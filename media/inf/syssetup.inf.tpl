;
; For Windows, this file controls various aspects of the Installation and
; Upgrade process. If you know a good documentation about this file,
; please add a link here.
;
;
[Version]
Signature = "$Windows NT$"
;Signature = "$ReactOS$"
ClassGUID={00000000-0000-0000-0000-000000000000}


; These .INFs install the device classes
[DeviceInfsToInstall]
; MS uses netnovel.inf as class-installer INF for NICs
; we use a separate one to keep things clean
battery.inf
bth.inf
cdrom.inf
cpu.inf
disk.inf
display.inf
fdc.inf
hal.inf
hdc.inf
input.inf
keyboard.inf
legcydrv.inf
machine.inf
monitor.inf
msmouse.inf
NET_NIC.inf
ports.inf
scsi.inf
unknown.inf
usbport.inf
audio.inf
ks.inf

[RegistrationPhase2]
RegisterDlls=OleControlDlls
