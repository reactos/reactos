<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="bootcdregtest" type="iso" output="ReactOS-RegTest.iso">
	<bootsector>isobtrt</bootsector>
	<cdfile installbase="$(CDOUTPUT)">unattend.inf</cdfile>
</module>
