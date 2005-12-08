<?xml version="1.0"?>
<xsd:schema 
	xmlns:xsd="http://www.w3.org/2001/XMLSchema"
	elementFormDefault="qualified" attributeFormDefault="qualified"
	blockDefault="extension" finalDefault="extension">

	<xsd:element name="bar">
		<xsd:complexType>
			<xsd:sequence>
				<xsd:element name="bar.B" type="xsd:anyType"/>
			</xsd:sequence>
			<xsd:attribute name="bar"/>
		</xsd:complexType>
		
	</xsd:element>

</xsd:schema>