<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:fo="http://www.w3.org/1999/XSL/Format"
                xmlns:fox="http://xml.apache.org/fop/extensions"
                version="1.0">

<xsl:output method="xml"/>

<xsl:template match="*">
  <xsl:element name="{name(.)}">
    <xsl:copy-of select="@*"/>
    <xsl:apply-templates/>
  </xsl:element>
</xsl:template>

<xsl:template match="fo:page-sequence
                     |fo:single-page-master-reference
                     |fo:repeatable-page-master-reference
                     |fo:conditional-page-master-reference">
  <xsl:element name="{name(.)}">
    <xsl:for-each select="@*">
      <xsl:choose>
        <xsl:when test="name(.) = 'master-reference'">
          <xsl:attribute name="master-name">
            <xsl:value-of select="."/>
          </xsl:attribute>
        </xsl:when>
        <xsl:otherwise>
          <xsl:attribute name="{name(.)}">
            <xsl:value-of select="."/>
          </xsl:attribute>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
    <xsl:apply-templates/>
  </xsl:element>
</xsl:template>

<!-- a clever idea that doesn't quite work. fop 0.20.1 doesn't understand % -->
<!-- and fop 0.20.2 doesn't work for me at all... -->
<xsl:template match="fo:table-column">
  <xsl:element name="{name(.)}">
    <xsl:if test="not(@column-width)">
      <xsl:attribute name="column-width">
        <xsl:value-of select="100 div count(../fo:table-column)"/>
        <xsl:text>%</xsl:text>
      </xsl:attribute>
    </xsl:if>
    <xsl:copy-of select="@*"/>
    <xsl:apply-templates/>
  </xsl:element>
</xsl:template>

</xsl:stylesheet>
