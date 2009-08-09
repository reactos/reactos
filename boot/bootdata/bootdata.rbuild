<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../tools/rbuild/project.dtd">
<group xmlns:xi="http://www.w3.org/2001/XInclude">
	<cdfile>autorun.inf</cdfile>
	<cdfile>icon.ico</cdfile>
	<cdfile>readme.txt</cdfile>
	<cdfile nameoncd="freeldr.ini">bootcd.ini</cdfile>

	<cdfile installbase="$(CDOUTPUT)" nameoncd="hivecls.inf">hivecls_$(ARCH).inf</cdfile>
	<cdfile installbase="$(CDOUTPUT)" nameoncd="hivedef.inf">hivedef_$(ARCH).inf</cdfile>
	<cdfile installbase="$(CDOUTPUT)" nameoncd="hivesft.inf">hivesft_$(ARCH).inf</cdfile>
	<cdfile installbase="$(CDOUTPUT)" nameoncd="hivesys.inf">hivesys_$(ARCH).inf</cdfile>
	<cdfile installbase="$(CDOUTPUT)">txtsetup.sif</cdfile>
	<cdfile installbase="$(CDOUTPUT)">unattend.inf</cdfile>

	<directory name="bootcd">
		<xi:include href="bootcd/bootcd.rbuild" />
	</directory>
	<directory name="livecd">
		<xi:include href="livecd/livecd.rbuild" />
	</directory>
	<directory name="bootcdregtest">
		<xi:include href="bootcdregtest/bootcdregtest.rbuild" />
	</directory>
	<directory name="livecdregtest">
		<xi:include href="livecdregtest/livecdregtest.rbuild" />
	</directory>
</group>
