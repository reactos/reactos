<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="setupldr" type="bootloader">
	<bootstrap base="loader" />
	<library>freeldr_startup</library>
	<library>freeldr_base64k</library>
	<library>freeldr_base</library>
	<library>freeldr_arch</library>
	<library>setupldr_main</library>
	<library>rossym</library>
	<library>string</library>
	<library>rtl</library>
</module>
</rbuild>
