<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">

<xsl:template match="/">
  <xsl:apply-templates mode="olink.mode"/>
</xsl:template>

<xsl:template name="attrs">
  <xsl:param name="nd" select="."/>

  <xsl:attribute name="type">
    <xsl:value-of select="local-name(.)"/>
  </xsl:attribute>

  <xsl:attribute name="href">
    <xsl:call-template name="olink.href.target">
      <xsl:with-param name="object" select="$nd"/>
    </xsl:call-template>
  </xsl:attribute>

  <xsl:attribute name="label">
    <xsl:apply-templates select="$nd" mode="label.markup"/>
  </xsl:attribute>

  <xsl:if test="$nd/@id">
    <xsl:attribute name="id">
      <xsl:value-of select="$nd/@id"/>
    </xsl:attribute>
  </xsl:if>

  <xsl:if test="$nd/@lang">
    <xsl:attribute name="lang">
      <xsl:value-of select="$nd/@lang"/>
    </xsl:attribute>
  </xsl:if>

  <xsl:if test="$nd/@arch">
    <xsl:attribute name="arch">
      <xsl:value-of select="$nd/@arch"/>
    </xsl:attribute>
  </xsl:if>

  <xsl:if test="$nd/@conformance">
    <xsl:attribute name="conformance">
      <xsl:value-of select="$nd/@conformance"/>
    </xsl:attribute>
  </xsl:if>

  <xsl:if test="$nd/@os">
    <xsl:attribute name="os">
      <xsl:value-of select="$nd/@os"/>
    </xsl:attribute>
  </xsl:if>

  <xsl:if test="$nd/@revision">
    <xsl:attribute name="revision">
      <xsl:value-of select="$nd/@revision"/>
    </xsl:attribute>
  </xsl:if>

  <xsl:if test="$nd/@userlevel">
    <xsl:attribute name="userlevel">
      <xsl:value-of select="$nd/@userlevel"/>
    </xsl:attribute>
  </xsl:if>

  <xsl:if test="$nd/@vendor">
    <xsl:attribute name="vendor">
      <xsl:value-of select="$nd/@vendor"/>
    </xsl:attribute>
  </xsl:if>

  <xsl:if test="$nd/@condition">
    <xsl:attribute name="condition">
      <xsl:value-of select="$nd/@condition"/>
    </xsl:attribute>
  </xsl:if>

  <xsl:if test="$nd/@security">
    <xsl:attribute name="security">
      <xsl:value-of select="$nd/@security"/>
    </xsl:attribute>
  </xsl:if>
</xsl:template>

<xsl:template name="div">
  <xsl:param name="nd" select="."/>

  <div>
    <xsl:call-template name="attrs">
      <xsl:with-param name="nd" select="$nd"/>
    </xsl:call-template>
    <ttl>
      <xsl:apply-templates select="$nd" mode="title.markup"/>
    </ttl>
    <objttl>
      <xsl:apply-templates select="$nd" mode="object.title.markup"/>
    </objttl>
    <xref>
      <xsl:choose>
        <xsl:when test="$nd/@xreflabel">
          <xsl:call-template name="xref.xreflabel">
            <xsl:with-param name="target" select="$nd"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:apply-templates select="$nd" mode="xref-to"/>
        </xsl:otherwise>
      </xsl:choose>
    </xref>
    <xsl:apply-templates mode="olink.mode"/>
  </div>
</xsl:template>

<xsl:template name="obj">
  <xsl:param name="nd" select="."/>

  <obj>
    <xsl:call-template name="attrs">
      <xsl:with-param name="nd" select="$nd"/>
    </xsl:call-template>
    <ttl>
      <xsl:apply-templates select="$nd" mode="title.markup"/>
    </ttl>
    <objttl>
      <xsl:apply-templates select="$nd" mode="object.title.markup"/>
    </objttl>
    <xref>
      <xsl:choose>
        <xsl:when test="$nd/@xreflabel">
          <xsl:call-template name="xref.xreflabel">
            <xsl:with-param name="target" select="$nd"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:apply-templates select="$nd" mode="xref-to"/>
        </xsl:otherwise>
      </xsl:choose>
    </xref>
  </obj>
</xsl:template>

<xsl:template match="text()|processing-instruction()|comment()"
              mode="olink.mode">
  <!-- nop -->
</xsl:template>

<xsl:template match="*" mode="olink.mode">
  <!-- nop -->
</xsl:template>

<xsl:template match="set" mode="olink.mode">
  <xsl:call-template name="div"/>
</xsl:template>

<xsl:template match="book" mode="olink.mode">
  <xsl:call-template name="div"/>
</xsl:template>

<xsl:template match="preface|chapter|appendix" mode="olink.mode">
  <xsl:call-template name="div"/>
</xsl:template>

<xsl:template match="part|reference" mode="olink.mode">
  <xsl:call-template name="div"/>
</xsl:template>

<xsl:template match="article" mode="olink.mode">
  <xsl:call-template name="div"/>
</xsl:template>

<xsl:template match="refentry" mode="olink.mode">
  <xsl:call-template name="div"/>
</xsl:template>

<xsl:template match="section|sect1|sect2|sect3|sect4|sect5" mode="olink.mode">
  <xsl:call-template name="div"/>
</xsl:template>

<xsl:template match="refsection|refsect1|refsect2|refsect3" mode="olink.mode">
  <xsl:call-template name="div"/>
</xsl:template>

<xsl:template match="figure|example|table" mode="olink.mode">
  <xsl:call-template name="obj"/>
</xsl:template>

<xsl:template match="equation[title]" mode="olink.mode">
  <xsl:call-template name="div"/>
</xsl:template>

</xsl:stylesheet>
