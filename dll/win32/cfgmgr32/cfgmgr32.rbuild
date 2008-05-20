<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="cfgmgr32" type="win32dll" baseaddress="${BASEADDRESS_CFGMGR32}" entrypoint="0" installbase="system32" installname="cfgmgr32.dll">
	<linkerflag>-nostartfiles</linkerflag>
	<linkerflag>-nostdlib</linkerflag>
	<linkerflag>-lgcc</linkerflag>
	<importlibrary definition="cfgmgr32.def" />
	<file>cfgmgr32.rc</file>
</module>
