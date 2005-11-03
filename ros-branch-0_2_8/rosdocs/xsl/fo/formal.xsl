<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:fo="http://www.w3.org/1999/XSL/Format"
                version='1.0'>

<!-- ********************************************************************
     $Id: formal.xsl,v 1.1 2002/06/13 20:32:20 chorns Exp $
     ********************************************************************

     This file is part of the XSL DocBook Stylesheet distribution.
     See ../README or http://nwalsh.com/docbook/xsl/ for copyright
     and other information.

     ******************************************************************** -->

<xsl:template name="formal.object">
  <xsl:param name="placement" select="'before'"/>

  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>

  <fo:block id="{$id}"
            xsl:use-attribute-sets="formal.object.properties">
    <xsl:if test="$placement = 'before'">
      <xsl:call-template name="formal.object.heading"/>
    </xsl:if>
    <xsl:apply-templates/>
    <xsl:if test="$placement != 'before'">
      <xsl:call-template name="formal.object.heading"/>
    </xsl:if>
  </fo:block>
</xsl:template>

<xsl:template name="formal.object.heading">
  <xsl:param name="title"></xsl:param>
  <fo:block xsl:use-attribute-sets="formal.title.properties">
    <xsl:apply-templates select="." mode="object.title.markup"/>
  </fo:block>
</xsl:template>

<xsl:template name="informal.object">
  <fo:block>
    <xsl:apply-templates/>
  </fo:block>
</xsl:template>

<xsl:template name="semiformal.object">
  <xsl:param name="placement" select="'before'"/>
  <xsl:choose>
    <xsl:when test="./title">
      <xsl:call-template name="formal.object">
        <xsl:with-param name="placement" select="$placement"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="informal.object"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="figure">
  <xsl:variable name="param.placement"
                select="substring-after(normalize-space($formal.title.placement),
                                        concat(local-name(.), ' '))"/>

  <xsl:variable name="placement">
    <xsl:choose>
      <xsl:when test="contains($param.placement, ' ')">
        <xsl:value-of select="substring-before($param.placement, ' ')"/>
      </xsl:when>
      <xsl:when test="$param.placement = ''">before</xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$param.placement"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <!-- FIXME: is this too careless? -->
  <xsl:choose>
    <xsl:when test=".//imagedata[@align][1]">
      <fo:block text-align="{.//imagedata[@align][1]/@align}">
        <xsl:call-template name="formal.object">
          <xsl:with-param name="placement" select="$placement"/>
        </xsl:call-template>
      </fo:block>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="formal.object">
        <xsl:with-param name="placement" select="$placement"/>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="example">
  <xsl:variable name="param.placement"
                select="substring-after(normalize-space($formal.title.placement),
                                        concat(local-name(.), ' '))"/>

  <xsl:variable name="placement">
    <xsl:choose>
      <xsl:when test="contains($param.placement, ' ')">
        <xsl:value-of select="substring-before($param.placement, ' ')"/>
      </xsl:when>
      <xsl:when test="$param.placement = ''">before</xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$param.placement"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:call-template name="formal.object">
    <xsl:with-param name="placement" select="$placement"/>
  </xsl:call-template>
</xsl:template>

<xsl:template name="table.frame">
  <xsl:variable name="frame">
    <xsl:choose>
      <xsl:when test="@frame">
        <xsl:value-of select="@frame"/>
      </xsl:when>
      <xsl:otherwise>all</xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="$frame='all'">
      <xsl:attribute name="border-left-style">solid</xsl:attribute>
      <xsl:attribute name="border-right-style">solid</xsl:attribute>
      <xsl:attribute name="border-top-style">solid</xsl:attribute>
      <xsl:attribute name="border-bottom-style">solid</xsl:attribute>
      <xsl:attribute name="border-left-width">
        <xsl:value-of select="$table.border.thickness"/>
      </xsl:attribute>
      <xsl:attribute name="border-right-width">
        <xsl:value-of select="$table.border.thickness"/>
      </xsl:attribute>
      <xsl:attribute name="border-top-width">
        <xsl:value-of select="$table.border.thickness"/>
      </xsl:attribute>
      <xsl:attribute name="border-bottom-width">
        <xsl:value-of select="$table.border.thickness"/>
      </xsl:attribute>
    </xsl:when>
    <xsl:when test="$frame='bottom'">
      <xsl:attribute name="border-left-style">none</xsl:attribute>
      <xsl:attribute name="border-right-style">none</xsl:attribute>
      <xsl:attribute name="border-top-style">none</xsl:attribute>
      <xsl:attribute name="border-bottom-style">solid</xsl:attribute>
      <xsl:attribute name="border-bottom-width">
        <xsl:value-of select="$table.border.thickness"/>
      </xsl:attribute>
    </xsl:when>
    <xsl:when test="$frame='sides'">
      <xsl:attribute name="border-left-style">solid</xsl:attribute>
      <xsl:attribute name="border-right-style">solid</xsl:attribute>
      <xsl:attribute name="border-top-style">none</xsl:attribute>
      <xsl:attribute name="border-bottom-style">none</xsl:attribute>
      <xsl:attribute name="border-left-width">
        <xsl:value-of select="$table.border.thickness"/>
      </xsl:attribute>
      <xsl:attribute name="border-right-width">
        <xsl:value-of select="$table.border.thickness"/>
      </xsl:attribute>
    </xsl:when>
    <xsl:when test="$frame='top'">
      <xsl:attribute name="border-left-style">none</xsl:attribute>
      <xsl:attribute name="border-right-style">none</xsl:attribute>
      <xsl:attribute name="border-top-style">solid</xsl:attribute>
      <xsl:attribute name="border-bottom-style">none</xsl:attribute>
      <xsl:attribute name="border-top-width">
        <xsl:value-of select="$table.border.thickness"/>
      </xsl:attribute>
    </xsl:when>
    <xsl:when test="$frame='topbot'">
      <xsl:attribute name="border-left-style">none</xsl:attribute>
      <xsl:attribute name="border-right-style">none</xsl:attribute>
      <xsl:attribute name="border-top-style">solid</xsl:attribute>
      <xsl:attribute name="border-bottom-style">solid</xsl:attribute>
      <xsl:attribute name="border-top-width">
        <xsl:value-of select="$table.border.thickness"/>
      </xsl:attribute>
      <xsl:attribute name="border-bottom-width">
        <xsl:value-of select="$table.border.thickness"/>
      </xsl:attribute>
    </xsl:when>
    <xsl:when test="$frame='none'">
      <xsl:attribute name="border-left-style">none</xsl:attribute>
      <xsl:attribute name="border-right-style">none</xsl:attribute>
      <xsl:attribute name="border-top-style">none</xsl:attribute>
      <xsl:attribute name="border-bottom-style">none</xsl:attribute>
    </xsl:when>
    <xsl:otherwise>
      <xsl:message>
        <xsl:text>Impossible frame on table: </xsl:text>
        <xsl:value-of select="$frame"/>
      </xsl:message>
      <xsl:attribute name="border-left-style">none</xsl:attribute>
      <xsl:attribute name="border-right-style">none</xsl:attribute>
      <xsl:attribute name="border-top-style">none</xsl:attribute>
      <xsl:attribute name="border-bottom-style">none</xsl:attribute>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="table">
  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>

  <xsl:variable name="param.placement"
                select="substring-after(normalize-space($formal.title.placement),
                                        concat(local-name(.), ' '))"/>

  <xsl:variable name="placement">
    <xsl:choose>
      <xsl:when test="contains($param.placement, ' ')">
        <xsl:value-of select="substring-before($param.placement, ' ')"/>
      </xsl:when>
      <xsl:when test="$param.placement = ''">before</xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$param.placement"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:variable name="prop-columns"
    select=".//colspec[contains(@colwidth, '*')]"/>

  <xsl:variable name="table.content">
    <fo:block id="{$id}"
              xsl:use-attribute-sets="formal.object.properties"
              keep-together.within-column="1">

      <xsl:if test="$placement = 'before'">
        <xsl:call-template name="formal.object.heading"/>
      </xsl:if>

      <fo:table border-collapse="collapse">
        <xsl:call-template name="table.frame"/>
        <xsl:if test="count($prop-columns) != 0">
          <xsl:attribute name="table-layout">fixed</xsl:attribute>
        </xsl:if>
        <xsl:apply-templates select="tgroup"/>
      </fo:table>

      <xsl:if test="$placement != 'before'">
        <xsl:call-template name="formal.object.heading"/>
      </xsl:if>
    </fo:block>
  </xsl:variable>

  <xsl:variable name="footnotes">
    <xsl:if test=".//footnote">
      <fo:block font-family="{$body.font.family}"
                font-size="{$footnote.font.size}"
                keep-together.within-column="1"
                keep-with-previous="always">
        <xsl:apply-templates select=".//footnote" mode="table.footnote.mode"/>
      </fo:block>
    </xsl:if>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="@orient='land'">
      <fo:block-container reference-orientation="90">
        <fo:block>
          <xsl:attribute name="span">
            <xsl:choose>
              <xsl:when test="@pgwide=1">all</xsl:when>
              <xsl:otherwise>none</xsl:otherwise>
            </xsl:choose>
          </xsl:attribute>
          <xsl:copy-of select="$table.content"/>
          <xsl:copy-of select="$footnotes"/>
        </fo:block>
      </fo:block-container>
    </xsl:when>
    <xsl:otherwise>
      <fo:block>
        <xsl:attribute name="span">
          <xsl:choose>
            <xsl:when test="@pgwide=1">all</xsl:when>
            <xsl:otherwise>none</xsl:otherwise>
          </xsl:choose>
        </xsl:attribute>
        <xsl:copy-of select="$table.content"/>
        <xsl:copy-of select="$footnotes"/>
      </fo:block>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="equation">
  <xsl:variable name="param.placement"
                select="substring-after(normalize-space($formal.title.placement),
                                        concat(local-name(.), ' '))"/>

  <xsl:variable name="placement">
    <xsl:choose>
      <xsl:when test="contains($param.placement, ' ')">
        <xsl:value-of select="substring-before($param.placement, ' ')"/>
      </xsl:when>
      <xsl:when test="$param.placement = ''">before</xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$param.placement"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:call-template name="semiformal.object">
    <xsl:with-param name="placement" select="$placement"/>
  </xsl:call-template>
</xsl:template>

<xsl:template match="figure/title"></xsl:template>
<xsl:template match="table/title"></xsl:template>
<xsl:template match="table/textobject"></xsl:template>
<xsl:template match="example/title"></xsl:template>
<xsl:template match="equation/title"></xsl:template>

<xsl:template match="informalfigure">
  <xsl:call-template name="informal.object"/>
</xsl:template>

<xsl:template match="informalexample">
  <xsl:call-template name="informal.object"/>
</xsl:template>

<xsl:template match="informaltable">
  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>
  <xsl:variable name="prop-columns"
    select=".//colspec[contains(@colwidth, '*')]"/>

  <xsl:variable name="table.content">
    <fo:table id="{$id}"
              border-collapse="collapse"
              xsl:use-attribute-sets="informal.object.properties">
      <xsl:call-template name="table.frame"/>
      <xsl:if test="count($prop-columns) != 0">
        <xsl:attribute name="table-layout">fixed</xsl:attribute>
      </xsl:if>
      <xsl:apply-templates select="tgroup"/>
    </fo:table>
  </xsl:variable>

  <xsl:variable name="footnotes">
    <xsl:if test=".//footnote">
      <fo:block font-family="{$body.font.family}"
                font-size="{$footnote.font.size}"
                keep-together.within-column="1"
                keep-with-previous="always">
        <xsl:apply-templates select=".//footnote" mode="table.footnote.mode"/>
      </fo:block>
    </xsl:if>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="@orient='land'">
      <fo:block-container reference-orientation="90">
        <fo:block>
          <xsl:attribute name="span">
            <xsl:choose>
              <xsl:when test="@pgwide=1">all</xsl:when>
              <xsl:otherwise>none</xsl:otherwise>
            </xsl:choose>
          </xsl:attribute>
          <xsl:copy-of select="$table.content"/>
          <xsl:copy-of select="$footnotes"/>
        </fo:block>
      </fo:block-container>
    </xsl:when>
    <xsl:otherwise>
      <fo:block>
        <xsl:attribute name="span">
          <xsl:choose>
            <xsl:when test="@pgwide=1">all</xsl:when>
            <xsl:otherwise>none</xsl:otherwise>
          </xsl:choose>
        </xsl:attribute>
        <xsl:copy-of select="$table.content"/>
        <xsl:copy-of select="$footnotes"/>
      </fo:block>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="informaltable/textobject"></xsl:template>

<xsl:template match="informalequation">
  <xsl:call-template name="informal.object"/>
</xsl:template>

</xsl:stylesheet>
