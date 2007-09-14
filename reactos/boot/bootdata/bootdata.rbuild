<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../tools/rbuild/project.dtd">
<group xmlns:xi="http://www.w3.org/2001/XInclude">
	<cdfile>autorun.inf</cdfile>
	<cdfile>icon.ico</cdfile>
	<cdfile>readme.txt</cdfile>
	<cdfile base="$(CDOUTPUT)">hivecls.inf</cdfile>
	<cdfile base="$(CDOUTPUT)">hivedef.inf</cdfile>
	<cdfile base="$(CDOUTPUT)">hivesft.inf</cdfile>
	<cdfile base="$(CDOUTPUT)">hivesys.inf</cdfile>
	<cdfile base="$(CDOUTPUT)">txtsetup.sif</cdfile>
	<cdfile base="$(CDOUTPUT)">unattend.inf</cdfile>
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
