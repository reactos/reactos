<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version='1.0'>

<!-- ********************************************************************
     $Id: toc.xsl,v 1.1 2002/06/13 20:32:42 chorns Exp $
     ********************************************************************

     This file is part of the XSL DocBook Stylesheet distribution.
     See ../README or http://nwalsh.com/docbook/xsl/ for copyright
     and other information.

     ******************************************************************** -->

<!-- ==================================================================== -->

<xsl:template match="toc">
  <xsl:choose>
    <xsl:when test="*">
      <xsl:if test="$process.source.toc != 0">
        <!-- if the toc isn't empty, process it -->
        <xsl:element name="{$toc.list.type}">
          <xsl:apply-templates/>
        </xsl:element>
      </xsl:if>
    </xsl:when>
    <xsl:otherwise>
      <xsl:if test="$process.empty.source.toc != 0">
        <xsl:choose>
          <xsl:when test="parent::section
                          or parent::sect1
                          or parent::sect2
                          or parent::sect3
                          or parent::sect4
                          or parent::sect5">
            <xsl:apply-templates select="parent::*"
                                 mode="toc.for.section"/>
          </xsl:when>
          <xsl:when test="parent::article">
            <xsl:apply-templates select="parent::*"
                                 mode="toc.for.component"/>
          </xsl:when>
          <xsl:when test="parent::book
                          or parent::part">
            <xsl:apply-templates select="parent::*"
                                 mode="toc.for.division"/>
          </xsl:when>
          <xsl:when test="parent::set">
            <xsl:apply-templates select="parent::*"
                                 mode="toc.for.set"/>
          </xsl:when>
          <!-- there aren't any other contexts that allow toc -->
          <xsl:otherwise>
            <xsl:message>
              <xsl:text>I don't know how to make a TOC in this context!</xsl:text>
            </xsl:message>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:if>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="tocpart|tocchap
                     |toclevel1|toclevel2|toclevel3|toclevel4|toclevel5">
  <xsl:variable name="sub-toc">
    <xsl:if test="tocchap|toclevel1|toclevel2|toclevel3|toclevel4|toclevel5">
      <xsl:choose>
        <xsl:when test="$toc.list.type = 'dl'">
          <dd>
            <xsl:element name="{$toc.list.type}">
              <xsl:apply-templates select="tocchap|toclevel1|toclevel2|toclevel3|toclevel4|toclevel5"/>
            </xsl:element>
          </dd>
        </xsl:when>
        <xsl:otherwise>
          <xsl:element name="{$toc.list.type}">
            <xsl:apply-templates select="tocchap|toclevel1|toclevel2|toclevel3|toclevel4|toclevel5"/>
          </xsl:element>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:if>
  </xsl:variable>

  <xsl:apply-templates select="tocentry[position() != last()]"/>

  <xsl:choose>
    <xsl:when test="$toc.list.type = 'dl'">
      <dt>
        <xsl:apply-templates select="tocentry[position() = last()]"/>
      </dt>
      <xsl:copy-of select="$sub-toc"/>
    </xsl:when>
    <xsl:otherwise>
      <li>
        <xsl:apply-templates select="tocentry[position() = last()]"/>
        <xsl:copy-of select="$sub-toc"/>
      </li>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="tocentry|tocfront|tocback">
  <xsl:choose>
    <xsl:when test="$toc.list.type = 'dl'">
      <dt>
        <xsl:call-template name="tocentry-content"/>
      </dt>
    </xsl:when>
    <xsl:otherwise>
      <li>
        <xsl:call-template name="tocentry-content"/>
      </li>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="tocentry[position() = last()]" priority="2">
  <xsl:call-template name="tocentry-content"/>
</xsl:template>

<xsl:template name="tocentry-content">
  <xsl:variable name="targets" select="key('id',@linkend)"/>
  <xsl:variable name="target" select="$targets[1]"/>

  <xsl:choose>
    <xsl:when test="@linkend">
      <xsl:call-template name="check.id.unique">
        <xsl:with-param name="linkend" select="@linkend"/>
      </xsl:call-template>
      <a>
        <xsl:attribute name="href">
          <xsl:call-template name="href.target">
            <xsl:with-param name="object" select="$target"/>
          </xsl:call-template>
        </xsl:attribute>
        <xsl:apply-templates/>
      </a>
    </xsl:when>
    <xsl:otherwise>
      <xsl:apply-templates/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="*" mode="toc.for.section">
  <xsl:call-template name="section.toc"/>
</xsl:template>

<xsl:template match="*" mode="toc.for.component">
  <xsl:call-template name="component.toc"/>
</xsl:template>

<xsl:template match="*" mode="toc.for.section">
  <xsl:call-template name="section.toc"/>
</xsl:template>

<xsl:template match="*" mode="toc.for.division">
  <xsl:call-template name="division.toc"/>
</xsl:template>

<xsl:template match="*" mode="toc.for.set">
  <xsl:call-template name="set.toc"/>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="lot|lotentry">
</xsl:template>

</xsl:stylesheet>
