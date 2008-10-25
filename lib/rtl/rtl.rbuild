<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../tools/rbuild/project.dtd">

<group xmlns:xi="http://www.w3.org/2001/XInclude">
	<module name="rtl_umode" type="staticlibrary">
		<xi:include href="rtl-common.rbuild" />
	</module>

	<module name="rtl_kmode" type="staticlibrary">
		<xi:include href="rtl-common.rbuild" />
	</module>

	<!-- dummy module to enable <include base="rtl"> -->
	<module name="rtl" type="staticlibrary" />
</group>
