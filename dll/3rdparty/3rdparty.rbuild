<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../tools/rbuild/project.dtd">
<group xmlns:xi="http://www.w3.org/2001/XInclude">
	<directory name="isc">
		<xi:include href="isc/isc.rbuild" />
	</directory>
	<directory name="dns">
		<xi:include href="dns/dns.rbuild" />
	</directory>
	<directory name="isccfg">
		<xi:include href="isccfg/isccfg.rbuild" />
	</directory>
	<directory name="lwres">
		<xi:include href="lwres/lwres.rbuild" />
	</directory>
	<directory name="bind9">
		<xi:include href="bind9/bind9.rbuild" />
	</directory>
	<directory name="mesa32">
		<xi:include href="mesa32/mesa32.rbuild" />
	</directory>
	<directory name="libjpeg">
		<xi:include href="libjpeg/libjpeg.rbuild" />
	</directory>
	<directory name="libxslt">
		<xi:include href="libxslt/libxslt.rbuild" />
	</directory>
	<if property="NSWPAT" value="1">
		<directory name="dxtn">
			<xi:include href="dxtn/dxtn.rbuild" />
		</directory>
	</if>
</group>
	
