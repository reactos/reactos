<!-- THIS IS BROKEN -->
<!-- ==================================================================== -->

<xsl:template match="t:biblioentry">
  <xsl:text>&#xA;&#xA;</xsl:text>
  <xsl:element name="xsl:template">
    <xsl:attribute name="match">biblioentry</xsl:attribute>
    <xsl:text>&#xA;</xsl:text>
    <xsl:element name="xsl:variable">
      <xsl:attribute name="name">id</xsl:attribute>
      <xsl:element name="xsl:call-template">
        <xsl:attribute name="name">object.id</xsl:attribute>
      </xsl:element>
    </xsl:element>
    <xsl:text>&#xA;</xsl:text>
    <xsl:element name="{@wrapper}">
      <xsl:attribute name="id">{$id}</xsl:attribute>
      <xsl:attribute name="class">{name(.)}</xsl:attribute>
      <xsl:text>&#xA;  </xsl:text>
      <xsl:element name="a">
        <xsl:attribute name="name">{$id}</xsl:attribute>
      </xsl:element>
      <xsl:apply-templates mode="biblioentry"/>
      <xsl:text>&#xA;</xsl:text>
    </xsl:element>
    <xsl:text>&#xA;</xsl:text>
  </xsl:element>

<!--
  <xsl:text>&#xA;&#xA;</xsl:text>
  <xsl:element name="xsl:template">
    <xsl:attribute name="match">biblioentry/biblioset</xsl:attribute>
    <xsl:apply-templates mode="biblioentry"/>
  </xsl:element>
-->
</xsl:template>

<xsl:template match="t:if" mode="biblioentry">
  <xsl:element name="xsl:if">
    <xsl:attribute name="test">
      <xsl:value-of select="@test"/>
    </xsl:attribute>
    <xsl:apply-templates mode="biblioentry"/>
  </xsl:element>
</xsl:template>

<xsl:template match="t:text" mode="biblioentry">
  <xsl:element name="xsl:text">
    <xsl:apply-templates/>
  </xsl:element>
</xsl:template>

<xsl:template match="*" mode="biblioentry">
  <xsl:text>&#xA;  </xsl:text>
  <xsl:element name="xsl:apply-templates">
    <xsl:attribute name="select">
      <xsl:value-of select="name(.)"/>
    </xsl:attribute>
    <xsl:attribute name="mode">bibliography.mode</xsl:attribute>
  </xsl:element>
</xsl:template>

<xsl:template match="t:or" mode="biblioentry">
  <xsl:text>&#xA;  </xsl:text>
  <xsl:element name="xsl:apply-templates">
    <xsl:attribute name="select">
      <xsl:call-template name="element-or-list"/>
    </xsl:attribute>
    <xsl:attribute name="mode">bibliography.mode</xsl:attribute>
  </xsl:element>
</xsl:template>

