<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version='1.0'
                xmlns:doc="http://nwalsh.com/xsl/documentation/1.0"
                xmlns:lxslt="http://xml.apache.org/xslt"
                exclude-result-prefixes="doc xsl lxslt">

<xsl:include href="../html/param.xsl"/>
<xsl:include href="../html/chunker.xsl"/>

<xsl:output
     method="xml"
     doctype-public="-//Norman Walsh//DTD JRefEntry V1.1//EN"
     doctype-system="http://docbook.sourceforge.net/release/jrefentry/1.1/jrefentry.dtd"
/>

<xsl:preserve-space elements="xsl:variable"/>
<xsl:strip-space elements="xsl:stylesheet"/>

<!-- ********************************************************************
     $Id: xsl2jref.xsl,v 1.1 2002/06/13 20:31:57 chorns Exp $
     ********************************************************************

     This file is part of the XSL DocBook Stylesheet distribution.
     See ../README or http://nwalsh.com/docbook/xsl/ for copyright
     and other information.

     ******************************************************************** -->

<!-- ==================================================================== -->

<xsl:param name="output-file" select="''"/>

<!-- ==================================================================== -->

<xsl:template match="lxslt:*">
  <!-- nop -->
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="/">
  <xsl:choose>
    <xsl:when test="$output-file = ''">
      <xsl:message terminate='yes'>
        <xsl:text>You must set the output-file parameter!</xsl:text>
      </xsl:message>
    </xsl:when>
    <xsl:when test="/xsl:stylesheet/doc:*">
      <xsl:call-template name="write.chunk">
        <xsl:with-param name="filename" select="$output-file"/>
        <xsl:with-param name="method" select="'xml'"/>
        <xsl:with-param name="encoding" select="'utf-8'"/>
        <xsl:with-param name="content">
          <xsl:apply-templates/>
        </xsl:with-param>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <!-- nop -->
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="*">
  <xsl:variable name="block-element" select="name(.) = 'para'
                                             or name(.) = 'sidebar'
                                             or name(.) = 'variablelist'
                                             or name(.) = 'itemizedlist'"/>
  <xsl:if test="$block-element"><xsl:text>&#10;</xsl:text></xsl:if>
  <xsl:element namespace="" name="{name(.)}">
    <xsl:apply-templates select="@*" mode="copy-attr"/>
    <xsl:apply-templates/>
  </xsl:element>
  <xsl:if test="$block-element"><xsl:text>&#10;</xsl:text></xsl:if>
</xsl:template>

<xsl:template match="@*" mode="copy-attr">
  <xsl:attribute name="{name(.)}">
    <xsl:value-of select="."/>
  </xsl:attribute>
</xsl:template>

<xsl:template match="xsl:*"></xsl:template>

<xsl:template match="xsl:stylesheet">
  <xsl:choose>
    <xsl:when test="doc:reference">
      <reference>
        <xsl:apply-templates/>
      </reference>
      <xsl:text>&#10;</xsl:text>
    </xsl:when>
    <xsl:otherwise>
      <xsl:apply-templates/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="xsl:include">
  <!-- nop -->
<!--
  <xsl:apply-templates select="document(@href)/*"/>
-->
</xsl:template>

<xsl:template match="@*" mode="copy">
  <xsl:text> </xsl:text>
  <xsl:value-of select="name(.)"/>
  <xsl:text>="</xsl:text>
  <xsl:value-of select="."/>
  <xsl:text>"</xsl:text>
</xsl:template>

<xsl:template match="*" mode="copy">
  <xsl:variable name="content">
    <xsl:apply-templates mode="copy"/>
  </xsl:variable>

  <xsl:text>&lt;</xsl:text>
  <xsl:value-of select="name(.)"/>
  <xsl:apply-templates select="@*" mode="copy"/>
  <xsl:choose>
    <xsl:when test="$content = ''">
      <xsl:text>/</xsl:text>
      <xsl:text>&gt;</xsl:text>
    </xsl:when>
    <xsl:otherwise>
      <xsl:text>&gt;</xsl:text>
      <xsl:copy-of select="$content"/>
      <xsl:text>&lt;/</xsl:text>
      <xsl:value-of select="name(.)"/>
      <xsl:text>&gt;</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="comment()" mode="copy">
  <xsl:text>&lt;!--</xsl:text>
  <xsl:value-of select="."/>
  <xsl:text>--&gt;</xsl:text>
</xsl:template>

<xsl:template match="processing-instruction()" mode="copy">
  <xsl:text>&lt;?</xsl:text>
  <xsl:value-of select="name(.)"/>
  <xsl:text> </xsl:text>
  <xsl:value-of select="."/>
  <xsl:text>?&gt;</xsl:text>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="doc:reference">
  <!-- only process the children; doc:reference logically wraps the entire
       stylesheet, even if it can't syntactically do so. -->
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="doc:variable">
  <xsl:variable name="name" select="@name"/>

  <xsl:text>&#10;</xsl:text>
  <refentry id="var.{$name}">
    <xsl:text>&#10;</xsl:text>
    <refnamediv>
      <xsl:text>&#10;</xsl:text>
      <refname><xsl:value-of select="$name"/></refname>
      <xsl:text>&#10;</xsl:text>
      <xsl:apply-templates select="refpurpose"/>
      <xsl:text>&#10;</xsl:text>
    </refnamediv>
    <xsl:text>&#10;</xsl:text>
    <refsynopsisdiv>
      <xsl:text>&#10;</xsl:text>
      <synopsis>
        <xsl:apply-templates select="../xsl:variable[@name=$name]"
                             mode="copy-template"/>
      </synopsis>
      <xsl:text>&#10;</xsl:text>
    </refsynopsisdiv>
    <xsl:text>&#10;</xsl:text>
    <xsl:apply-templates select="*[name(.)!='refpurpose']"/>
  </refentry>
  <xsl:text>&#10;</xsl:text>
</xsl:template>

<xsl:template match="doc:param">
  <xsl:variable name="name" select="@name"/>

  <xsl:text>&#10;</xsl:text>
  <refentry id="param.{$name}">
    <xsl:text>&#10;</xsl:text>
    <refnamediv>
      <xsl:text>&#10;</xsl:text>
      <refname><xsl:value-of select="$name"/></refname>
      <xsl:text>&#10;</xsl:text>
      <xsl:apply-templates select="refpurpose"/>
      <xsl:text>&#10;</xsl:text>
    </refnamediv>
    <xsl:text>&#10;</xsl:text>
    <refsynopsisdiv>
      <xsl:text>&#10;</xsl:text>
      <synopsis>
        <xsl:apply-templates select="../xsl:param[@name=$name]"
                             mode="copy-template"/>
      </synopsis>
      <xsl:text>&#10;</xsl:text>
    </refsynopsisdiv>
    <xsl:text>&#10;</xsl:text>
    <xsl:apply-templates select="*[name(.)!='refpurpose']"/>
  </refentry>
  <xsl:text>&#10;</xsl:text>
</xsl:template>

<xsl:template match="doc:template[@name]">
  <xsl:variable name="name" select="@name"/>

  <xsl:text>&#10;</xsl:text>
  <refentry id="template.{$name}">
    <xsl:text>&#10;</xsl:text>
    <refnamediv>
      <xsl:text>&#10;</xsl:text>
      <refname><xsl:value-of select="$name"/></refname>
      <xsl:text>&#10;</xsl:text>
      <xsl:apply-templates select="refpurpose"/>
      <xsl:text>&#10;</xsl:text>
    </refnamediv>
    <xsl:text>&#10;</xsl:text>
    <refsynopsisdiv>
      <xsl:text>&#10;</xsl:text>
      <synopsis>
        <xsl:apply-templates select="../xsl:template[@name=$name]"
                             mode="copy-template"/>
      </synopsis>
      <xsl:text>&#10;</xsl:text>
    </refsynopsisdiv>
    <xsl:text>&#10;</xsl:text>
    <xsl:apply-templates select="*[name(.)!='refpurpose']"/>
  </refentry>
  <xsl:text>&#10;</xsl:text>
</xsl:template>

<xsl:template match="doc:template[@match]">
  <xsl:variable name="match" select="@match"/>
  <xsl:variable name="mode" select="@mode"/>

  <xsl:text>&#10;</xsl:text>
  <refentry>
    <xsl:text>&#10;</xsl:text>
    <refnamediv>
      <xsl:text>&#10;</xsl:text>
      <refname>
        <xsl:value-of select="$match"/>
        <xsl:if test="@mode">
          <xsl:text> (in </xsl:text>
          <xsl:value-of select="$mode"/>
          <xsl:text> mode)</xsl:text>
        </xsl:if>
      </refname>
      <xsl:text>&#10;</xsl:text>
      <xsl:apply-templates select="refpurpose"/>
      <xsl:text>&#10;</xsl:text>
    </refnamediv>
    <xsl:text>&#10;</xsl:text>
    <refsynopsisdiv>
      <xsl:text>&#10;</xsl:text>
      <synopsis>
        <xsl:choose>
          <xsl:when test="@mode">
            <xsl:apply-templates select="../xsl:template[@match=$match and @mode=$mode]"
                                 mode="copy-template"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:apply-templates select="../xsl:template[@match=$match]"
                                 mode="copy-template"/>
          </xsl:otherwise>
        </xsl:choose>
      </synopsis>
      <xsl:text>&#10;</xsl:text>
    </refsynopsisdiv>
    <xsl:text>&#10;</xsl:text>
    <xsl:apply-templates select="*[name(.)!='refpurpose']"/>
  </refentry>
  <xsl:text>&#10;</xsl:text>
</xsl:template>

<xsl:template match="doc:mode">
  <xsl:variable name="name" select="@mode"/>

  <refentry id="mode.{$name}">
    <refnamediv>
      <refname><xsl:value-of select="$name"/> mode</refname>
      <xsl:apply-templates select="refpurpose"/>
    </refnamediv>
    <xsl:apply-templates select="*[name(.)!='refpurpose']"/>
  </refentry>
</xsl:template>

<xsl:template match="doc:attribute-set">
  <xsl:variable name="name" select="@name"/>

  <refentry id="attrset.{$name}">
    <refnamediv>
      <refname><xsl:value-of select="$name"/> mode</refname>
      <xsl:apply-templates select="refpurpose"/>
    </refnamediv>
    <xsl:apply-templates select="*[name(.)!='refpurpose']"/>
  </refentry>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="*|text()|comment()|processing-instruction()"
              mode="copy-template">
  <!-- suppress -->
</xsl:template>

<xsl:template match="xsl:param" mode="copy-template">
  <xsl:text>&#10;</xsl:text>
  <xsl:apply-templates select="." mode="copy"/>
</xsl:template>

<xsl:template match="xsl:template" mode="copy-template">
  <xsl:variable name="content">
    <xsl:apply-templates mode="copy-template"/>
  </xsl:variable>

  <xsl:text>&lt;</xsl:text>
  <xsl:value-of select="name(.)"/>
  <xsl:apply-templates select="@*" mode="copy"/>
  <xsl:choose>
    <xsl:when test="$content = ''">
      <xsl:text>/</xsl:text>
      <xsl:text>&gt;</xsl:text>
    </xsl:when>
    <xsl:otherwise>
      <xsl:text>&gt;</xsl:text>
      <xsl:copy-of select="$content"/>
      <xsl:text>&#10;</xsl:text>
      <xsl:text>  ...</xsl:text>
      <xsl:text>&#10;</xsl:text>
      <xsl:text>&lt;/</xsl:text>
      <xsl:value-of select="name(.)"/>
      <xsl:text>&gt;</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- ==================================================================== -->

</xsl:stylesheet>
