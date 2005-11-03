<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:fo="http://www.w3.org/1999/XSL/Format"
                version='1.0'>

<!-- ********************************************************************
     $Id: block.xsl,v 1.1 2002/06/13 20:32:19 chorns Exp $
     ********************************************************************

     This file is part of the XSL DocBook Stylesheet distribution.
     See ../README or http://nwalsh.com/docbook/xsl/ for copyright
     and other information.

     ******************************************************************** -->

<!-- ==================================================================== -->

<xsl:template name="block.object">
  <fo:block>
    <xsl:apply-templates/>
  </fo:block>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="para">
  <fo:block xsl:use-attribute-sets="normal.para.spacing">
    <xsl:apply-templates/>
  </fo:block>
</xsl:template>

<xsl:template match="simpara">
  <fo:block xsl:use-attribute-sets="normal.para.spacing">
    <xsl:apply-templates/>
  </fo:block>
</xsl:template>

<xsl:template match="formalpara">
  <fo:block xsl:use-attribute-sets="normal.para.spacing">
    <xsl:apply-templates/>
  </fo:block>
</xsl:template>

<xsl:template match="formalpara/title">
  <xsl:variable name="titleStr" select="."/>
  <xsl:variable name="lastChar">
    <xsl:if test="$titleStr != ''">
      <xsl:value-of select="substring($titleStr,string-length($titleStr),1)"/>
    </xsl:if>
  </xsl:variable>

  <fo:inline font-weight="bold"
             keep-with-next.within-line="always"
             padding-end="1em">
    <xsl:apply-templates/>
    <xsl:if test="$lastChar != ''
                  and not(contains($runinhead.title.end.punct, $lastChar))">
      <xsl:value-of select="$runinhead.default.title.end.punct"/>
    </xsl:if>
    <xsl:text>&#160;</xsl:text>
  </fo:inline>
</xsl:template>

<xsl:template match="formalpara/para">
  <xsl:apply-templates/>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="blockquote">
  <fo:block xsl:use-attribute-sets="blockquote.properties">
    <xsl:call-template name="anchor"/>
    <fo:block>
      <xsl:if test="title">
        <fo:block xsl:use-attribute-sets="formal.title.properties">
          <xsl:apply-templates select="." mode="object.title.markup"/>
        </fo:block>
      </xsl:if>
      <xsl:apply-templates select="*[local-name(.) != 'title'
                                   and local-name(.) != 'attribution']"/>
    </fo:block>
    <xsl:if test="attribution">
      <fo:block text-align="right">
        <!-- mdash -->
        <xsl:text>&#x2014;</xsl:text>
        <xsl:apply-templates select="attribution"/>
      </fo:block>
    </xsl:if>
  </fo:block>
</xsl:template>

<xsl:template match="epigraph">
  <fo:block>
    <xsl:call-template name="anchor"/>
    <xsl:apply-templates select="para"/>
    <fo:inline>
      <xsl:text>--</xsl:text>
      <xsl:apply-templates select="attribution"/>
    </fo:inline>
  </fo:block>
</xsl:template>

<xsl:template match="attribution">
  <fo:inline><xsl:apply-templates/></fo:inline>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="sidebar">
  <fo:block>
    <xsl:if test="./title">
      <fo:block font-weight="bold"
                keep-with-next.within-column="always"
                hyphenate="false">
        <xsl:apply-templates select="./title" mode="sidebar.title.mode"/>
      </fo:block>
    </xsl:if>

    <xsl:apply-templates/>
  </fo:block>
</xsl:template>

<xsl:template match="sidebar/title">
</xsl:template>

<xsl:template match="sidebar/title" mode="sidebar.title.mode">
  <xsl:apply-templates/>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="abstract">
  <fo:block>
    <xsl:if test="@id">
      <xsl:attribute name="id"><xsl:value-of select="@id"/></xsl:attribute>
    </xsl:if>
    <xsl:call-template name="formal.object.heading">
      <xsl:with-param name="title">
        <xsl:apply-templates select="." mode="title.markup"/>
      </xsl:with-param>
    </xsl:call-template>
    <xsl:apply-templates/>
  </fo:block>
</xsl:template>

<xsl:template match="abstract/title">
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="msgset">
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="msgentry">
  <xsl:call-template name="block.object"/>
</xsl:template>

<xsl:template match="msg">
  <xsl:call-template name="block.object"/>
</xsl:template>

<xsl:template match="msgmain">
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="msgsub">
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="msgrel">
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="msgtext">
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="msginfo">
  <xsl:call-template name="block.object"/>
</xsl:template>

<xsl:template match="msglevel">
  <fo:block>
    <fo:inline font-weight="bold"
               keep-with-next.within-line="always">
      <xsl:call-template name="gentext.template">
        <xsl:with-param name="context" select="'msgset'"/>
        <xsl:with-param name="name" select="'MsgLevel'"/>
      </xsl:call-template>
    </fo:inline>
    <xsl:apply-templates/>
  </fo:block>
</xsl:template>

<xsl:template match="msgorig">
  <fo:block>
    <fo:inline font-weight="bold"
               keep-with-next.within-line="always">
      <xsl:call-template name="gentext.template">
        <xsl:with-param name="context" select="'msgset'"/>
        <xsl:with-param name="name" select="'MsgOrig'"/>
      </xsl:call-template>
    </fo:inline>
    <xsl:apply-templates/>
  </fo:block>
</xsl:template>

<xsl:template match="msgaud">
  <fo:block>
    <fo:inline font-weight="bold"
               keep-with-next.within-line="always">
      <xsl:call-template name="gentext.template">
        <xsl:with-param name="context" select="'msgset'"/>
        <xsl:with-param name="name" select="'MsgAud'"/>
      </xsl:call-template>
    </fo:inline>
    <xsl:apply-templates/>
  </fo:block>
</xsl:template>

<xsl:template match="msgexplan">
  <xsl:call-template name="block.object"/>
</xsl:template>

<xsl:template match="msgexplan/title">
  <fo:block font-weight="bold"
            keep-with-next.within-column="always"
            hyphenate="false">
    <xsl:apply-templates/>
  </fo:block>
</xsl:template>

<!-- ==================================================================== -->
<!-- For better or worse, revhistory is allowed in content... -->

<xsl:template match="revhistory">
  <fo:table table-layout="fixed">
    <fo:table-column column-number="1" column-width="33%"/>
    <fo:table-column column-number="2" column-width="33%"/>
    <fo:table-column column-number="3" column-width="33%"/>
    <fo:table-body>
      <fo:table-row>
        <fo:table-cell number-columns-spanned="3">
          <fo:block>
            <xsl:call-template name="gentext">
              <xsl:with-param name="key" select="'RevHistory'"/>
            </xsl:call-template>
          </fo:block>
        </fo:table-cell>
      </fo:table-row>
      <xsl:apply-templates/>
    </fo:table-body>
  </fo:table>
</xsl:template>

<xsl:template match="revhistory/revision">
  <xsl:variable name="revnumber" select=".//revnumber"/>
  <xsl:variable name="revdate"   select=".//date"/>
  <xsl:variable name="revauthor" select=".//authorinitials"/>
  <xsl:variable name="revremark" select=".//revremark"/>
  <fo:table-row>
    <fo:table-cell>
      <fo:block>
        <xsl:if test="$revnumber">
          <xsl:call-template name="gentext">
            <xsl:with-param name="key" select="'Revision'"/>
          </xsl:call-template>
          <xsl:call-template name="gentext.space"/>
          <xsl:apply-templates select="$revnumber[1]"/>
        </xsl:if>
      </fo:block>
    </fo:table-cell>
    <fo:table-cell>
      <fo:block>
        <xsl:apply-templates select="$revdate[1]"/>
      </fo:block>
    </fo:table-cell>
    <fo:table-cell>
      <fo:block>
        <xsl:apply-templates select="$revauthor[1]"/>
      </fo:block>
    </fo:table-cell>
  </fo:table-row>
  <xsl:if test="$revremark">
    <fo:table-row>
      <fo:table-cell number-columns-spanned="3">
        <fo:block>
          <xsl:apply-templates select="$revremark[1]"/>
        </fo:block>
      </fo:table-cell>
    </fo:table-row>
  </xsl:if>
</xsl:template>

<xsl:template match="revision/revnumber">
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="revision/date">
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="revision/authorinitials">
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="revision/revremark">
  <xsl:apply-templates/>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="ackno">
  <fo:block xsl:use-attribute-sets="normal.para.spacing">
    <xsl:apply-templates/>
  </fo:block>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="highlights">
  <xsl:call-template name="block.object"/>
</xsl:template>

<!-- ==================================================================== -->

</xsl:stylesheet>
