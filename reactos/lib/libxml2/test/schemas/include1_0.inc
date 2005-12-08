<?xml version="1.0"?>
<xsd:schema xmlns:xsd="http://www.w3.org/2001/XMLSchema" >

	<xsd:simpleType name="barType">
		<xsd:restriction base="xsd:string">
			<xsd:maxLength value="10" />
		</xsd:restriction>
	</xsd:simpleType>

	<xsd:attribute name="bar" type="barType" fixed="o"/>

	<xsd:element name="bar.B" type="xsd:anyURI" block="#all" nillable="true" final="restriction extension"/>
	
	<xsd:element name="bar">
		<xsd:complexType>
			<xsd:sequence>
				<xsd:element name="bar.A" block="restriction" form="qualified" nillable="true" />
				<xsd:element ref="bar.B"/>
			</xsd:sequence>
			
			<xsd:attribute ref="bar" xml:lang="DE">
				<xsd:annotation/>
			</xsd:attribute>
			<xsd:attribute name="uri" type="xsd:token" />
		</xsd:complexType>
	</xsd:element>

</xsd:schema>