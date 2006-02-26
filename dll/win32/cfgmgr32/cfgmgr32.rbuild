<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="cfgmgr32" type="win32dll" baseaddress="${BASEADDRESS_CFGMGR32}" entrypoint="0" installbase="system32" installname="cfgmgr32.dll">
	<linkerflag>-nostartfiles</linkerflag>
	<linkerflag>-nostdlib</linkerflag>
	<linkerflag>-lgcc</linkerflag>
	<importlibrary definition="cfgmgr32.def" />
	<file>cfgmgr32.rc</file>
</module>
</rbuild>
