<?xml version="1.0" encoding="ISO-8859-1"?>
<!DOCTYPE group SYSTEM "tools/rbuild/project.dtd">
<group xmlns:xi="http://www.w3.org/2001/XInclude">

	<!-- Core API and Components -->
	<buildfamily name="core" description="Core aplications , drivers and dlls" />
	<buildfamily name="kernel" description="OS Kernel" />

	<!-- User mode applications -->
	<buildfamily name="applications" />
	<buildfamily name="guiapplications" description="Win32 GUI applications" />
	<buildfamily name="cuiapplications" description="Win32 console applications" />
	<buildfamily name="nativeapplications"  description="Native console applications"/>

	<!-- By functionality -->
	<buildfamily name="games" />
	<buildfamily name="screensavers" />
	<buildfamily name="services" />
	<buildfamily name="shells" />
	<buildfamily name="cpapplets" />

	<!-- Drivers -->
	<buildfamily name="drivers" />
	<buildfamily name="fsdrivers"  description="File system drivers" />
	<buildfamily name="hardwaredrivers" description="Hardware drivers" />
	<buildfamily name="displaydrivers" description="Hardware display drivers" />
	<buildfamily name="inputdrivers" description="I/O device drivers" />

</group>
