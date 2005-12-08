<?xml version="1.0"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<xsl:strip-space elements="xsdtest datatype equiv class"/>

<xsl:output indent="yes" encoding="utf-8"/>

<xsl:template match="xsdtest">
  <testSuite>
    <xsl:apply-templates/>
  </testSuite>
</xsl:template>

<xsl:template match="datatype">
<testSuite>
<documentation>Datatype <xsl:value-of select="@name"/></documentation>
<testCase>
<requires datatypeLibrary="http://www.w3.org/2001/XMLSchema-datatypes"/>
<correct>
<element xmlns="http://relaxng.org/ns/structure/1.0" name="doc"
         datatypeLibrary="http://www.w3.org/2001/XMLSchema-datatypes">
  <data type="{@name}">
    <xsl:for-each select="param">
      <param name="{@name}"><xsl:value-of select="."/></param>
    </xsl:for-each>
  </data>
</element>
</correct>
<xsl:apply-templates select="valid|invalid"/>
</testCase>
<xsl:apply-templates select="equiv/class|length|lessThan|incomparable"/>
</testSuite>
</xsl:template>

<xsl:template match="valid">
 <xsl:call-template name="valid"/>
</xsl:template>

<xsl:template match="invalid">
 <xsl:call-template name="invalid"/>
</xsl:template>

<xsl:template name="valid">
  <valid>
    <xsl:apply-templates select="@internalSubset"/>
    <doc>
      <xsl:copy-of select="namespace::*"/>
      <xsl:value-of select="."/>
    </doc>
  </valid>
</xsl:template>

<xsl:template name="invalid">
  <invalid>
    <xsl:apply-templates select="@internalSubset"/>
    <doc>
      <xsl:copy-of select="namespace::*"/>
      <xsl:value-of select="."/>
    </doc>
  </invalid>
</xsl:template>

<xsl:template match="@internalSubset">
  <xsl:param name="doc" select="'doc'"/>
  <xsl:attribute name="dtd">
    <xsl:text>
&lt;!DOCTYPE </xsl:text>
    <xsl:value-of select="$doc"/>
    <xsl:text> [
</xsl:text>
    <xsl:value-of select="."/>
    <xsl:text>
]></xsl:text>
  </xsl:attribute>
</xsl:template>

<xsl:template match="class">
<testCase>
<correct>
  <xsl:for-each select="value[1]">
    <xsl:apply-templates select="@internalSubset">
      <xsl:with-param name="doc">element</xsl:with-param>
    </xsl:apply-templates>
    <element xmlns="http://relaxng.org/ns/structure/1.0" name="doc"
         datatypeLibrary="http://www.w3.org/2001/XMLSchema-datatypes">
      <value>
        <xsl:copy-of select="namespace::*"/>
        <xsl:attribute name="type"><xsl:value-of select="../../../@name"/></xsl:attribute>
        <xsl:value-of select="."/>
      </value>
    </element>
  </xsl:for-each>
</correct>
<xsl:for-each select="value[position() != 1]">
  <xsl:call-template name="valid"/>
</xsl:for-each>
<xsl:for-each select="preceding-sibling::class/value|following-sibling::class/value">
  <xsl:call-template name="invalid"/>
</xsl:for-each>
</testCase>
</xsl:template>

<xsl:template match="length">
<testCase>
<correct>
<element xmlns="http://relaxng.org/ns/structure/1.0" name="doc"
         datatypeLibrary="http://www.w3.org/2001/XMLSchema-datatypes">
  <data type="{../@name}">
    <param name="length"><xsl:value-of select="@value"/></param>
  </data>
</element>
</correct>
<xsl:call-template name="valid"/>
</testCase>

<testCase>
<correct>
<element xmlns="http://relaxng.org/ns/structure/1.0" name="doc"
         datatypeLibrary="http://www.w3.org/2001/XMLSchema-datatypes">
  <data type="{../@name}">
    <param name="length"><xsl:value-of select="@value + 1"/></param>
  </data>
</element>
</correct>
<xsl:call-template name="invalid"/>
</testCase>

<xsl:if test="@value != 0">
  <testCase>
  <correct>
  <element xmlns="http://relaxng.org/ns/structure/1.0" name="doc"
	   datatypeLibrary="http://www.w3.org/2001/XMLSchema-datatypes">
    <data type="{../@name}">
      <param name="length"><xsl:value-of select="@value - 1"/></param>
    </data>
  </element>
  </correct>
  <xsl:call-template name="invalid"/>
  </testCase>
</xsl:if>

</xsl:template>

<xsl:template match="lessThan">
<testCase>
<correct>
  <element xmlns="http://relaxng.org/ns/structure/1.0" name="doc"
	   datatypeLibrary="http://www.w3.org/2001/XMLSchema-datatypes">
    <data type="{../@name}">
      <param name="minExclusive">
        <xsl:value-of select="value[1]"/>
      </param>
    </data>
   </element>
</correct>
<valid>
<doc>
<xsl:value-of select="value[2]"/>
</doc>
</valid>
<invalid>
<doc>
<xsl:value-of select="value[1]"/>
</doc>
</invalid>
</testCase>
<testCase>
<correct>
  <element xmlns="http://relaxng.org/ns/structure/1.0" name="doc"
	   datatypeLibrary="http://www.w3.org/2001/XMLSchema-datatypes">
    <data type="{../@name}">
      <param name="minExclusive">
        <xsl:value-of select="value[2]"/>
      </param>
    </data>
   </element>
</correct>
<invalid>
<doc>
<xsl:value-of select="value[1]"/>
</doc>
</invalid>
<invalid>
<doc>
<xsl:value-of select="value[2]"/>
</doc>
</invalid>
</testCase>
</xsl:template>

<xsl:template match="incomparable">
<testCase>
<correct>
  <element xmlns="http://relaxng.org/ns/structure/1.0" name="doc"
	   datatypeLibrary="http://www.w3.org/2001/XMLSchema-datatypes">
    <data type="{../@name}">
      <param name="minExclusive">
        <xsl:value-of select="value[1]"/>
      </param>
    </data>
   </element>
</correct>
<invalid>
<doc>
<xsl:value-of select="value[2]"/>
</doc>
</invalid>
<invalid>
<doc>
<xsl:value-of select="value[1]"/>
</doc>
</invalid>
</testCase>
<testCase>
<correct>
  <element xmlns="http://relaxng.org/ns/structure/1.0" name="doc"
	   datatypeLibrary="http://www.w3.org/2001/XMLSchema-datatypes">
    <data type="{../@name}">
      <param name="minExclusive">
        <xsl:value-of select="value[2]"/>
      </param>
    </data>
   </element>
</correct>
<invalid>
<doc>
<xsl:value-of select="value[1]"/>
</doc>
</invalid>
<invalid>
<doc>
<xsl:value-of select="value[2]"/>
</doc>
</invalid>
</testCase>
</xsl:template>

</xsl:stylesheet>
