<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:fo="http://www.w3.org/1999/XSL/Format"
                version='1.0'>

<!-- ********************************************************************
     $Id: qandaset.xsl,v 1.1 2002/06/13 20:32:23 chorns Exp $
     ********************************************************************

     This file is part of the XSL DocBook Stylesheet distribution.
     See ../README or http://nwalsh.com/docbook/xsl/ for copyright
     and other information.

     ******************************************************************** -->

<xsl:variable name="qanda.defaultlabel">number</xsl:variable>
<xsl:variable name="generate.qandaset.toc" select="true()"/>
<xsl:variable name="generate.qandadiv.toc" select="false()"/>

<!-- ==================================================================== -->

<xsl:template match="qandaset">
  <xsl:variable name="id"><xsl:call-template name="object.id"/></xsl:variable>

  <fo:block id="{$id}">
    <xsl:if test="title">
      <xsl:apply-templates select="title"/>
    </xsl:if>

    <xsl:apply-templates select="*[name(.) != 'title'
                                 and name(.) != 'qandadiv'
                                 and name(.) != 'qandaentry']"/>
    <xsl:apply-templates select="qandadiv"/>

    <xsl:if test="qandaentry">
      <fo:list-block xsl:use-attribute-sets="list.block.spacing"
                     provisional-distance-between-starts="2.5em"
                     provisional-label-separation="0.2em">
        <xsl:apply-templates select="qandaentry"/>
      </fo:list-block>
    </xsl:if>
  </fo:block>
</xsl:template>

<xsl:template match="qandaset/title">
  <xsl:variable name="enclsect" select="(ancestor::section
                                        | ancestor::simplesect
                                        | ancestor::sect5
                                        | ancestor::sect4
                                        | ancestor::sect3
                                        | ancestor::sect2
                                        | ancestor::sect1
                                        | ancestor::refsect3
                                        | ancestor::refsect2
                                        | ancestor::refsect1)[last()]"/>
  <xsl:variable name="sectlvl">
    <xsl:call-template name="section.level">
      <xsl:with-param name="node" select="$enclsect"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:call-template name="section.heading">
    <xsl:with-param name="level" select="$sectlvl"/>
    <xsl:with-param name="title">
      <xsl:apply-templates/>
    </xsl:with-param>
  </xsl:call-template>
</xsl:template>

<xsl:template match="qandadiv">
  <xsl:variable name="id"><xsl:call-template name="object.id"/></xsl:variable>

  <fo:block id="{$id}">
    <xsl:apply-templates select="title"/>
    <xsl:apply-templates select="*[name(.) != 'title'
                                 and name(.) != 'qandadiv'
                                 and name(.) != 'qandaentry']"/>
    <fo:block start-indent="{count(ancestor::qandadiv)*2}pc">
      <xsl:apply-templates select="qandadiv"/>

      <xsl:if test="qandaentry">
        <fo:list-block xsl:use-attribute-sets="list.block.spacing"
                       provisional-distance-between-starts="2.5em"
                       provisional-label-separation="0.2em">
          <xsl:apply-templates select="qandaentry"/>
        </fo:list-block>
      </xsl:if>
    </fo:block>
  </fo:block>
</xsl:template>

<xsl:template match="qandadiv/title">
  <xsl:variable name="enclsect" select="(ancestor::section
                                        | ancestor::simplesect
                                        | ancestor::sect5
                                        | ancestor::sect4
                                        | ancestor::sect3
                                        | ancestor::sect2
                                        | ancestor::sect1
                                        | ancestor::refsect3
                                        | ancestor::refsect2
                                        | ancestor::refsect1)[last()]"/>
  <xsl:variable name="sectlvl">
    <xsl:call-template name="section.level">
      <xsl:with-param name="node" select="$enclsect"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:call-template name="section.heading">
    <xsl:with-param name="level"
                    select="$sectlvl + count(ancestor::qandadiv)"/>
    <xsl:with-param name="title">
      <xsl:apply-templates/>
    </xsl:with-param>
  </xsl:call-template>
</xsl:template>

<xsl:template match="qandaentry">
  <xsl:apply-templates/>
<!--
  <fo:block>
    <xsl:if test="@id">
      <xsl:attribute name="id"><xsl:value-of select="@id"/></xsl:attribute>
    </xsl:if>
    <xsl:apply-templates/>
  </fo:block>
-->
</xsl:template>

<xsl:template match="question">
  <xsl:variable name="id"><xsl:call-template name="object.id"/></xsl:variable>

  <xsl:variable name="entry.id">
    <xsl:call-template name="object.id">
      <xsl:with-param name="object" select="parent::*"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="deflabel">
    <xsl:choose>
      <xsl:when test="ancestor-or-self::*[@defaultlabel]">
        <xsl:value-of select="(ancestor-or-self::*[@defaultlabel])[last()]
                              /@defaultlabel"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="qanda.defaultlabel"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <fo:list-item id="{$entry.id}" xsl:use-attribute-sets="list.item.spacing">
    <fo:list-item-label id="{$id}" end-indent="label-end()">
      <xsl:choose>
        <xsl:when test="$deflabel = 'none'">
          <fo:block/>
        </xsl:when>
        <xsl:otherwise>
          <fo:block>
            <xsl:apply-templates select="." mode="label.markup"/>
            <xsl:text>.</xsl:text> <!-- FIXME: Hack!!! This should be in the locale! -->
          </fo:block>
        </xsl:otherwise>
      </xsl:choose>
    </fo:list-item-label>
    <fo:list-item-body start-indent="body-start()">
      <xsl:choose>
        <xsl:when test="$deflabel = 'none'">
          <fo:block font-weight="bold">
            <xsl:apply-templates select="*[local-name(.)!='label']"/>
          </fo:block>
        </xsl:when>
        <xsl:otherwise>
          <xsl:apply-templates select="*[local-name(.)!='label']"/>
        </xsl:otherwise>
      </xsl:choose>
    </fo:list-item-body>
  </fo:list-item>
</xsl:template>

<xsl:template match="answer">
  <xsl:variable name="id"><xsl:call-template name="object.id"/></xsl:variable>
  <xsl:variable name="entry.id">
    <xsl:call-template name="object.id">
      <xsl:with-param name="object" select="parent::*"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="deflabel">
    <xsl:choose>
      <xsl:when test="ancestor-or-self::*[@defaultlabel]">
        <xsl:value-of select="(ancestor-or-self::*[@defaultlabel])[last()]
                              /@defaultlabel"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="qanda.defaultlabel"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <fo:list-item xsl:use-attribute-sets="list.item.spacing">
    <fo:list-item-label id="{$id}" end-indent="label-end()">
      <xsl:choose>
        <xsl:when test="$deflabel = 'none'">
          <fo:block/>
        </xsl:when>
        <xsl:otherwise>
          <fo:block>
            <!-- FIXME: Hack!!! This should be in the locale! -->
            <xsl:variable name="answer.label">
              <xsl:apply-templates select="." mode="label.markup"/>
            </xsl:variable>
            <xsl:copy-of select="$answer.label"/>
            <xsl:if test="string($answer.label) != ''">
              <xsl:text>.</xsl:text>
            </xsl:if>
          </fo:block>
        </xsl:otherwise>
      </xsl:choose>
    </fo:list-item-label>
    <fo:list-item-body start-indent="body-start()">
      <xsl:apply-templates select="*[local-name(.)!='label']"/>
    </fo:list-item-body>
  </fo:list-item>
</xsl:template>

<xsl:template match="label">
  <xsl:apply-templates/>
</xsl:template>

</xsl:stylesheet>
