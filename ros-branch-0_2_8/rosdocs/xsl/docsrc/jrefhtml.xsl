<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version='1.0'>

<xsl:import href="../html/docbook.xsl"/>

<xsl:output
  method="html"
  indent="yes"
  encoding="ISO-8859-1"/>

<!-- ********************************************************************
     $Id: jrefhtml.xsl,v 1.1 2002/06/13 20:31:57 chorns Exp $
     ********************************************************************

     This file is part of the XSL DocBook Stylesheet distribution.
     See ../README or http://nwalsh.com/docbook/xsl/ for copyright
     and other information.

     ******************************************************************** -->

<!-- ==================================================================== -->

<xsl:param name="part.autolabel" select="0"/>

<xsl:template match="refentry">
  <xsl:apply-imports/>
</xsl:template>

<xsl:template match="refdescription">
  <div class="{name(.)}">
    <a>
      <xsl:attribute name="name">
        <xsl:call-template name="object.id"/>
      </xsl:attribute>
    </a>
    <xsl:apply-templates/>
  </div>
</xsl:template>

<xsl:template match="refauthor">
  <div class="{name(.)}">
    <b>
      <a>
        <xsl:attribute name="name">
          <xsl:call-template name="object.id"/>
        </xsl:attribute>
      </a>
      <xsl:text>Author</xsl:text>
    </b>
    <xsl:apply-templates/>
  </div>
</xsl:template>

<xsl:template match="refversion">
  <div class="{name(.)}">
    <b>
      <a>
        <xsl:attribute name="name">
          <xsl:call-template name="object.id"/>
        </xsl:attribute>
      </a>
      <xsl:text>Version</xsl:text>
    </b>
    <xsl:apply-templates/>
  </div>
</xsl:template>

<xsl:template match="refparameter">
  <div class="{name(.)}">
    <b>
      <a>
        <xsl:attribute name="name">
          <xsl:call-template name="object.id"/>
        </xsl:attribute>
      </a>
      <xsl:text>Parameters</xsl:text>
    </b>
    <xsl:apply-templates/>
  </div>
</xsl:template>

<xsl:template match="refreturn">
  <div class="{name(.)}">
    <b>
      <a>
        <xsl:attribute name="name">
          <xsl:call-template name="object.id"/>
        </xsl:attribute>
      </a>
      <xsl:text>Returns</xsl:text>
    </b>
    <xsl:apply-templates/>
  </div>
</xsl:template>

<xsl:template match="refexception|refthrows">
  <div class="{name(.)}">
    <b>
      <a>
        <xsl:attribute name="name">
          <xsl:call-template name="object.id"/>
        </xsl:attribute>
      </a>
      <xsl:text>Exceptions</xsl:text>
    </b>
    <xsl:apply-templates/>
  </div>
</xsl:template>

<xsl:template match="refsee">
  <div class="{name(.)}">
    <b>
      <a>
        <xsl:attribute name="name">
          <xsl:call-template name="object.id"/>
        </xsl:attribute>
      </a>
      <xsl:text>See</xsl:text>
    </b>
    <xsl:apply-templates/>
  </div>
</xsl:template>

<xsl:template match="refsince">
  <div class="{name(.)}">
    <b>
      <a>
        <xsl:attribute name="name">
          <xsl:call-template name="object.id"/>
        </xsl:attribute>
      </a>
      <xsl:text>Since</xsl:text>
    </b>
    <xsl:apply-templates/>
  </div>
</xsl:template>

<xsl:template match="refserial">
  <div class="{name(.)}">
    <b>
      <a>
        <xsl:attribute name="name">
          <xsl:call-template name="object.id"/>
        </xsl:attribute>
      </a>
      <xsl:text>Serial</xsl:text>
    </b>
    <xsl:apply-templates/>
  </div>
</xsl:template>

<xsl:template match="refdeprecated">
  <div class="{name(.)}">
    <b>
      <a>
        <xsl:attribute name="name">
          <xsl:call-template name="object.id"/>
        </xsl:attribute>
      </a>
      <xsl:text>Deprecated</xsl:text>
    </b>
    <xsl:apply-templates/>
  </div>
</xsl:template>

<!-- ==================================================================== -->

</xsl:stylesheet>
