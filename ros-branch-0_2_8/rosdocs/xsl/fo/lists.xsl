<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:fo="http://www.w3.org/1999/XSL/Format"
                version='1.0'>

<!-- ********************************************************************
     $Id: lists.xsl,v 1.1 2002/06/13 20:32:22 chorns Exp $
     ********************************************************************

     This file is part of the XSL DocBook Stylesheet distribution.
     See ../README or http://nwalsh.com/docbook/xsl/ for copyright
     and other information.

     ******************************************************************** -->

<!-- ==================================================================== -->

<xsl:template match="itemizedlist">
  <xsl:variable name="id"><xsl:call-template name="object.id"/></xsl:variable>

  <xsl:if test="title">
    <xsl:apply-templates select="title" mode="list.title.mode"/>
  </xsl:if>

  <fo:list-block id="{$id}" xsl:use-attribute-sets="list.block.spacing"
                 provisional-distance-between-starts="1.5em"
                 provisional-label-separation="0.2em">
    <xsl:apply-templates/>
  </fo:list-block>
</xsl:template>

<xsl:template match="itemizedlist/title|orderedlist/title">
  <!--nop-->
</xsl:template>

<xsl:template match="variablelist/title" mode="vl.as.list">
  <!--nop-->
</xsl:template>

<xsl:template match="variablelist/title" mode="vl.as.blocks">
  <!--nop-->
</xsl:template>

<xsl:template match="itemizedlist/listitem">
  <xsl:variable name="id"><xsl:call-template name="object.id"/></xsl:variable>

  <xsl:variable name="itemsymbol">
    <xsl:call-template name="list.itemsymbol">
      <xsl:with-param name="node" select="parent::itemizedlist"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="item.contents">
    <fo:list-item-label end-indent="label-end()">
      <fo:block>
        <xsl:choose>
          <xsl:when test="$itemsymbol='disc'">&#x2022;</xsl:when>
          <xsl:when test="$itemsymbol='bullet'">&#x2022;</xsl:when>
          <!-- why do these symbols not work? -->
          <!--
          <xsl:when test="$itemsymbol='circle'">&#x2218;</xsl:when>
          <xsl:when test="$itemsymbol='round'">&#x2218;</xsl:when>
          <xsl:when test="$itemsymbol='square'">&#x2610;</xsl:when>
          <xsl:when test="$itemsymbol='box'">&#x2610;</xsl:when>
          -->
          <xsl:otherwise>&#x2022;</xsl:otherwise>
        </xsl:choose>
      </fo:block>
    </fo:list-item-label>
    <fo:list-item-body start-indent="body-start()">
      <xsl:apply-templates/>
    </fo:list-item-body>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="parent::*/@spacing = 'compact'">
      <fo:list-item id="{$id}" xsl:use-attribute-sets="compact.list.item.spacing">
        <xsl:copy-of select="$item.contents"/>
      </fo:list-item>
    </xsl:when>
    <xsl:otherwise>
      <fo:list-item id="{$id}" xsl:use-attribute-sets="list.item.spacing">
        <xsl:copy-of select="$item.contents"/>
      </fo:list-item>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="orderedlist">
  <xsl:variable name="id"><xsl:call-template name="object.id"/></xsl:variable>

  <xsl:if test="title">
    <xsl:apply-templates select="title" mode="list.title.mode"/>
  </xsl:if>

  <fo:list-block id="{$id}" xsl:use-attribute-sets="list.block.spacing"
                 provisional-distance-between-starts="2em"
                 provisional-label-separation="0.2em">
    <xsl:apply-templates/>
  </fo:list-block>
</xsl:template>

<xsl:template match="orderedlist/listitem">
  <xsl:variable name="id"><xsl:call-template name="object.id"/></xsl:variable>

  <xsl:variable name="numeration">
    <xsl:call-template name="list.numeration">
      <xsl:with-param name="node" select="parent::orderedlist"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="type">
    <xsl:choose>
      <xsl:when test="$numeration='arabic'">1.</xsl:when>
      <xsl:when test="$numeration='loweralpha'">a.</xsl:when>
      <xsl:when test="$numeration='lowerroman'">i.</xsl:when>
      <xsl:when test="$numeration='upperalpha'">A.</xsl:when>
      <xsl:when test="$numeration='upperroman'">I.</xsl:when>
      <!-- What!? This should never happen -->
      <xsl:otherwise>
        <xsl:message>
          <xsl:text>Unexpected numeration: </xsl:text>
          <xsl:value-of select="$numeration"/>
        </xsl:message>
        <xsl:value-of select="1."/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:variable name="item.contents">
    <fo:list-item-label end-indent="label-end()">
      <fo:block>
        <xsl:choose>
          <xsl:when test="@override">
            <xsl:number value="@override" format="{$type}"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:number count="listitem" format="{$type}"/>
          </xsl:otherwise>
        </xsl:choose>
      </fo:block>
    </fo:list-item-label>
    <fo:list-item-body start-indent="body-start()">
      <xsl:apply-templates/>
    </fo:list-item-body>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="parent::*/@spacing = 'compact'">
      <fo:list-item id="{$id}" xsl:use-attribute-sets="compact.list.item.spacing">
        <xsl:copy-of select="$item.contents"/>
      </fo:list-item>
    </xsl:when>
    <xsl:otherwise>
      <fo:list-item id="{$id}" xsl:use-attribute-sets="list.item.spacing">
        <xsl:copy-of select="$item.contents"/>
      </fo:list-item>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="listitem/para[1]
                     |listitem/simpara[1]
                     |listitem/formalpara[1]
                     |callout/para[1]
                     |callout/simpara[1]
                     |callout/formalpara[1]"
              priority="2">
  <fo:block>
    <xsl:apply-templates/>
  </fo:block>
</xsl:template>

<xsl:template match="variablelist">
  <xsl:variable name="presentation">
    <xsl:call-template name="dbfo-attribute">
      <xsl:with-param name="pis"
                      select="processing-instruction('dbfo')"/>
      <xsl:with-param name="attribute" select="'list-presentation'"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="$presentation = 'list'">
      <xsl:apply-templates select="." mode="vl.as.list"/>
    </xsl:when>
    <xsl:when test="$presentation = 'blocks'">
      <xsl:apply-templates select="." mode="vl.as.blocks"/>
    </xsl:when>
    <xsl:when test="$variablelist.as.blocks != 0">
      <xsl:apply-templates select="." mode="vl.as.blocks"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:apply-templates select="." mode="vl.as.list"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="variablelist" mode="vl.as.list">
  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>

  <xsl:variable name="term-width">
    <xsl:call-template name="dbfo-attribute">
      <xsl:with-param name="pis"
                      select="processing-instruction('dbfo')"/>
      <xsl:with-param name="attribute" select="'term-width'"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="termlength">
    <xsl:choose>
      <xsl:when test="$term-width != ''">
        <xsl:value-of select="$term-width"/>
      </xsl:when>
      <xsl:when test="@termlength">
        <xsl:variable name="termlength.is.number">
          <xsl:value-of select="@termlength + 0"/>
        </xsl:variable>
        <xsl:choose>
          <xsl:when test="$termlength.is.number = 'NaN'">
            <!-- if the term length isn't just a number, assume it's a measurement -->
            <xsl:value-of select="@termlength"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="@termlength"/>
            <xsl:text>em</xsl:text>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:otherwise>
        <!-- FIXME: this should be a parameter! -->
        <xsl:call-template name="longest.term">
          <xsl:with-param name="terms" select="varlistentry/term"/>
          <xsl:with-param name="maxlength" select="12"/>
        </xsl:call-template>
        <xsl:text>em</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

<!--
  <xsl:message>
    <xsl:text>term width: </xsl:text>
    <xsl:value-of select="$termlength"/>
  </xsl:message>
-->

  <xsl:if test="title">
    <xsl:apply-templates select="title" mode="list.title.mode"/>
  </xsl:if>

  <fo:list-block id="{$id}"
                 provisional-distance-between-starts="{$termlength}"
                 provisional-label-separation="0.25in"
                 xsl:use-attribute-sets="list.block.spacing">
    <xsl:apply-templates mode="vl.as.list"/>
  </fo:list-block>
</xsl:template>

<xsl:template name="longest.term">
  <xsl:param name="longest" select="0"/>
  <xsl:param name="terms" select="."/>
  <xsl:param name="maxlength" select="-1"/>

  <xsl:choose>
    <xsl:when test="$longest &gt; $maxlength and $maxlength &gt; 0">
      <xsl:value-of select="$maxlength"/>
    </xsl:when>
    <xsl:when test="not($terms)">
      <xsl:value-of select="$longest"/>
    </xsl:when>
    <xsl:when test="string-length($terms[1]) &gt; $longest">
      <xsl:call-template name="longest.term">
        <xsl:with-param name="longest" select="string-length($terms[1])"/>
        <xsl:with-param name="maxlength" select="$maxlength"/>
        <xsl:with-param name="terms" select="terms[position() &gt; 1]"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="longest.term">
        <xsl:with-param name="longest" select="$longest"/>
        <xsl:with-param name="maxlength" select="$maxlength"/>
        <xsl:with-param name="terms" select="terms[position() &gt; 1]"/>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="varlistentry" mode="vl.as.list">
  <xsl:variable name="id"><xsl:call-template name="object.id"/></xsl:variable>
  <fo:list-item id="{$id}" xsl:use-attribute-sets="list.item.spacing">
    <fo:list-item-label end-indent="label-end()" text-align="start">
      <fo:block>
        <xsl:apply-templates select="term"/>
      </fo:block>
    </fo:list-item-label>
    <fo:list-item-body start-indent="body-start()">
      <xsl:apply-templates select="listitem"/>
    </fo:list-item-body>
  </fo:list-item>
</xsl:template>

<xsl:template match="variablelist" mode="vl.as.blocks">
  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>

  <!-- termlength is irrelevant -->

  <xsl:if test="title">
    <xsl:apply-templates select="title" mode="list.title.mode"/>
  </xsl:if>

  <fo:block id="{$id}" xsl:use-attribute-sets="list.block.spacing">
    <xsl:apply-templates mode="vl.as.blocks"/>
  </fo:block>
</xsl:template>

<xsl:template match="varlistentry" mode="vl.as.blocks">
  <xsl:variable name="id"><xsl:call-template name="object.id"/></xsl:variable>

  <fo:block id="{$id}" xsl:use-attribute-sets="list.item.spacing">
    <xsl:apply-templates select="term"/>
  </fo:block>

  <fo:block margin-left="0.25in">
    <xsl:apply-templates select="listitem"/>
  </fo:block>
</xsl:template>

<xsl:template match="varlistentry/term">
  <fo:inline><xsl:apply-templates/>, </fo:inline>
</xsl:template>

<xsl:template match="varlistentry/term[position()=last()]" priority="2">
  <fo:inline><xsl:apply-templates/></fo:inline>
</xsl:template>

<xsl:template match="varlistentry/listitem">
  <xsl:apply-templates/>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="title" mode="list.title.mode">
  <fo:block font-size="12pt" font-weight="bold"
            xsl:use-attribute-sets="list.block.spacing">
    <xsl:apply-templates/>
  </fo:block>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="simplelist">
  <!-- with no type specified, the default is 'vert' -->
  <fo:table>
    <fo:table-body>
      <xsl:call-template name="simplelist.vert">
	<xsl:with-param name="cols">
	  <xsl:choose>
	    <xsl:when test="@columns">
	      <xsl:value-of select="@columns"/>
	    </xsl:when>
	    <xsl:otherwise>1</xsl:otherwise>
	  </xsl:choose>
	</xsl:with-param>
      </xsl:call-template>
    </fo:table-body>
  </fo:table>
</xsl:template>

<xsl:template match="simplelist[@type='inline']">
  <fo:inline><xsl:apply-templates/></fo:inline>
</xsl:template>

<xsl:template match="simplelist[@type='horiz']">
  <fo:table>
    <fo:table-body>
      <xsl:call-template name="simplelist.horiz">
	<xsl:with-param name="cols">
	  <xsl:choose>
	    <xsl:when test="@columns">
	      <xsl:value-of select="@columns"/>
	    </xsl:when>
	    <xsl:otherwise>1</xsl:otherwise>
	  </xsl:choose>
	</xsl:with-param>
      </xsl:call-template>
    </fo:table-body>
  </fo:table>
</xsl:template>

<xsl:template match="simplelist[@type='vert']">
  <fo:table>
    <fo:table-body>
      <xsl:call-template name="simplelist.vert">
	<xsl:with-param name="cols">
	  <xsl:choose>
	    <xsl:when test="@columns">
	      <xsl:value-of select="@columns"/>
	    </xsl:when>
	    <xsl:otherwise>1</xsl:otherwise>
	  </xsl:choose>
	</xsl:with-param>
      </xsl:call-template>
    </fo:table-body>
  </fo:table>
</xsl:template>

<xsl:template name="simplelist.horiz">
  <xsl:param name="cols">1</xsl:param>
  <xsl:param name="cell">1</xsl:param>
  <xsl:param name="members" select="./member"/>

  <xsl:if test="$cell &lt;= count($members)">
    <fo:table-row>
      <xsl:call-template name="simplelist.horiz.row">
        <xsl:with-param name="cols" select="$cols"/>
        <xsl:with-param name="cell" select="$cell"/>
        <xsl:with-param name="members" select="$members"/>
      </xsl:call-template>
   </fo:table-row>
    <xsl:call-template name="simplelist.horiz">
      <xsl:with-param name="cols" select="$cols"/>
      <xsl:with-param name="cell" select="$cell + $cols"/>
      <xsl:with-param name="members" select="$members"/>
    </xsl:call-template>
  </xsl:if>
</xsl:template>

<xsl:template name="simplelist.horiz.row">
  <xsl:param name="cols">1</xsl:param>
  <xsl:param name="cell">1</xsl:param>
  <xsl:param name="members" select="./member"/>
  <xsl:param name="curcol">1</xsl:param>

  <xsl:if test="$curcol &lt;= $cols">
    <fo:table-cell>
      <fo:block>
        <xsl:if test="$members[position()=$cell]">
          <xsl:apply-templates select="$members[position()=$cell]"/>
        </xsl:if>
      </fo:block>
    </fo:table-cell>
    <xsl:call-template name="simplelist.horiz.row">
      <xsl:with-param name="cols" select="$cols"/>
      <xsl:with-param name="cell" select="$cell+1"/>
      <xsl:with-param name="members" select="$members"/>
      <xsl:with-param name="curcol" select="$curcol+1"/>
    </xsl:call-template>
  </xsl:if>
</xsl:template>

<xsl:template name="simplelist.vert">
  <xsl:param name="cols">1</xsl:param>
  <xsl:param name="cell">1</xsl:param>
  <xsl:param name="members" select="./member"/>
  <xsl:param name="rows"
             select="floor((count($members)+$cols - 1) div $cols)"/>

  <xsl:if test="$cell &lt;= $rows">
    <fo:table-row>
      <xsl:call-template name="simplelist.vert.row">
        <xsl:with-param name="cols" select="$cols"/>
        <xsl:with-param name="rows" select="$rows"/>
        <xsl:with-param name="cell" select="$cell"/>
        <xsl:with-param name="members" select="$members"/>
      </xsl:call-template>
   </fo:table-row>
    <xsl:call-template name="simplelist.vert">
      <xsl:with-param name="cols" select="$cols"/>
      <xsl:with-param name="cell" select="$cell+1"/>
      <xsl:with-param name="members" select="$members"/>
      <xsl:with-param name="rows" select="$rows"/>
    </xsl:call-template>
  </xsl:if>
</xsl:template>

<xsl:template name="simplelist.vert.row">
  <xsl:param name="cols">1</xsl:param>
  <xsl:param name="rows">1</xsl:param>
  <xsl:param name="cell">1</xsl:param>
  <xsl:param name="members" select="./member"/>
  <xsl:param name="curcol">1</xsl:param>

  <xsl:if test="$curcol &lt;= $cols">
    <fo:table-cell>
      <fo:block>
        <xsl:if test="$members[position()=$cell]">
          <xsl:apply-templates select="$members[position()=$cell]"/>
        </xsl:if>
      </fo:block>
    </fo:table-cell>
    <xsl:call-template name="simplelist.vert.row">
      <xsl:with-param name="cols" select="$cols"/>
      <xsl:with-param name="rows" select="$rows"/>
      <xsl:with-param name="cell" select="$cell+$rows"/>
      <xsl:with-param name="members" select="$members"/>
      <xsl:with-param name="curcol" select="$curcol+1"/>
    </xsl:call-template>
  </xsl:if>
</xsl:template>

<xsl:template match="member">
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="simplelist[@type='inline']/member">
  <xsl:apply-templates/>
  <xsl:text>, </xsl:text>
</xsl:template>

<xsl:template match="simplelist[@type='inline']/member[position()=last()]"
              priority="2">
  <xsl:apply-templates/>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="procedure">
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

  <xsl:variable name="preamble"
                select="*[not(self::step or self::title)]"/>
  <xsl:variable name="steps" select="step"/>

  <fo:block id="{$id}" xsl:use-attribute-sets="list.block.spacing">
    <xsl:if test="./title and $placement = 'before'">
      <xsl:choose>
        <xsl:when test="$formal.procedures != 0">
          <xsl:call-template name="formal.object.heading"/>
        </xsl:when>
        <xsl:otherwise>
          <fo:block font-weight="bold">
            <xsl:apply-templates select="./title" mode="procedure.title.mode"/>
          </fo:block>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:if>

    <xsl:apply-templates select="$preamble"/>

    <fo:list-block xsl:use-attribute-sets="list.block.spacing"
                   provisional-distance-between-starts="2em"
                   provisional-label-separation="0.2em">
      <xsl:apply-templates select="$steps"/>
    </fo:list-block>

    <xsl:if test="./title and $placement != 'before'">
      <xsl:choose>
        <xsl:when test="$formal.procedures != 0">
          <xsl:call-template name="formal.object.heading"/>
        </xsl:when>
        <xsl:otherwise>
          <fo:block font-weight="bold">
            <xsl:apply-templates select="./title" mode="procedure.title.mode"/>
          </fo:block>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:if>
  </fo:block>
</xsl:template>

<xsl:template match="procedure/title">
</xsl:template>

<xsl:template match="procedure/title" mode="procedure.title.mode">
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="substeps">
  <fo:list-block xsl:use-attribute-sets="list.block.spacing"
                 provisional-distance-between-starts="2em"
                 provisional-label-separation="0.2em">
    <xsl:apply-templates/>
  </fo:list-block>
</xsl:template>

<xsl:template match="step">
  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>

  <fo:list-item>
    <fo:list-item-label end-indent="label-end()">
      <fo:block id="{$id}" xsl:use-attribute-sets="list.item.spacing">
        <!-- dwc: fix for one step procedures. Use a bullet if there's no step 2 -->
        <xsl:choose>
          <xsl:when test="count(../step) = 1">
            <xsl:text>&#x2022;</xsl:text>
          </xsl:when>
          <xsl:otherwise>
            <xsl:apply-templates select="." mode="number">
              <xsl:with-param name="recursive" select="0"/>
            </xsl:apply-templates>.
          </xsl:otherwise>
        </xsl:choose>
      </fo:block>
    </fo:list-item-label>
    <fo:list-item-body start-indent="body-start()">
      <xsl:apply-templates/>
    </fo:list-item-body>
  </fo:list-item>
</xsl:template>

<xsl:template match="step/title">
  <fo:block font-weight="bold" xsl:use-attribute-sets="list.item.spacing">
    <xsl:apply-templates/>
  </fo:block>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="segmentedlist">
  <xsl:variable name="presentation">
    <xsl:call-template name="pi-attribute">
      <xsl:with-param name="pis"
                      select="processing-instruction('dbfo')"/>
      <xsl:with-param name="attribute" select="'list-presentation'"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="$presentation = 'table'">
      <xsl:apply-templates select="." mode="seglist-table"/>
    </xsl:when>
    <xsl:when test="$presentation = 'list'">
      <xsl:apply-templates/>
    </xsl:when>
    <xsl:when test="$segmentedlist.as.table != 0">
      <xsl:apply-templates select="." mode="seglist-table"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:apply-templates/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="segmentedlist/title">
  <fo:block font-weight="bold">
    <xsl:apply-templates/>
  </fo:block>
</xsl:template>

<xsl:template match="segtitle">
</xsl:template>

<xsl:template match="segtitle" mode="segtitle-in-seg">
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="seglistitem">
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="seg">
  <xsl:variable name="segnum" select="position()"/>
  <xsl:variable name="seglist" select="ancestor::segmentedlist"/>
  <xsl:variable name="segtitles" select="$seglist/segtitle"/>

  <!--
     Note: segtitle is only going to be the right thing in a well formed
     SegmentedList.  If there are too many Segs or too few SegTitles,
     you'll get something odd...maybe an error
  -->

  <fo:block>
    <fo:inline font-weight="bold">
      <xsl:apply-templates select="$segtitles[$segnum=position()]"
                           mode="segtitle-in-seg"/>
      <xsl:text>: </xsl:text>
    </fo:inline>
    <xsl:apply-templates/>
  </fo:block>
</xsl:template>

<xsl:template match="segmentedlist" mode="seglist-table">
  <xsl:apply-templates select="title" mode="seglist-table"/>
  <fo:table>
    <fo:table-header>
      <fo:table-row>
        <xsl:apply-templates select="segtitle" mode="seglist-table"/>
      </fo:table-row>
    </fo:table-header>
    <fo:table-body>
      <xsl:apply-templates select="seglistitem" mode="seglist-table"/>
    </fo:table-body>
  </fo:table>
</xsl:template>

<xsl:template match="segtitle" mode="seglist-table">
  <fo:table-cell>
    <fo:block>
      <xsl:apply-templates/>
    </fo:block>
  </fo:table-cell>
</xsl:template>

<xsl:template match="seglistitem" mode="seglist-table">
  <fo:table-row>
    <xsl:apply-templates mode="seglist-table"/>
  </fo:table-row>
</xsl:template>

<xsl:template match="seg" mode="seglist-table">
  <fo:table-cell>
    <fo:block>
      <xsl:apply-templates/>
    </fo:block>
  </fo:table-cell>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="calloutlist">
  <xsl:variable name="id"><xsl:call-template name="object.id"/></xsl:variable>

  <fo:block id="{$id}">
    <xsl:if test="./title">
      <fo:block xsl:use-attribute-sets="formal.title.properties">
        <xsl:apply-templates select="./title" mode="calloutlist.title.mode"/>
      </fo:block>
    </xsl:if>

    <fo:list-block space-before.optimum="1em"
                   space-before.minimum="0.8em"
                   space-before.maximum="1.2em"
                   provisional-distance-between-starts="2.2em"
                   provisional-label-separation="0.2em">
      <xsl:apply-templates/>
    </fo:list-block>
  </fo:block>
</xsl:template>

<xsl:template match="calloutlist/title">
</xsl:template>

<xsl:template match="calloutlist/title" mode="calloutlist.title.mode">
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="callout">
  <xsl:variable name="id"><xsl:call-template name="object.id"/></xsl:variable>
  <fo:list-item id="{$id}">
    <fo:list-item-label end-indent="label-end()">
      <fo:block>
        <xsl:call-template name="callout.arearefs">
          <xsl:with-param name="arearefs" select="@arearefs"/>
        </xsl:call-template>
      </fo:block>
    </fo:list-item-label>
    <fo:list-item-body start-indent="body-start()">
      <xsl:apply-templates/>
    </fo:list-item-body>
  </fo:list-item>
</xsl:template>

<xsl:template name="callout.arearefs">
  <xsl:param name="arearefs"></xsl:param>
  <xsl:if test="$arearefs!=''">
    <xsl:choose>
      <xsl:when test="substring-before($arearefs,' ')=''">
        <xsl:call-template name="callout.arearef">
          <xsl:with-param name="arearef" select="$arearefs"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="callout.arearef">
          <xsl:with-param name="arearef"
                          select="substring-before($arearefs,' ')"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:call-template name="callout.arearefs">
      <xsl:with-param name="arearefs"
                      select="substring-after($arearefs,' ')"/>
    </xsl:call-template>
  </xsl:if>
</xsl:template>

<xsl:template name="callout.arearef">
  <xsl:param name="arearef"></xsl:param>
  <xsl:variable name="targets" select="key('id',$arearef)"/>
  <xsl:variable name="target" select="$targets[1]"/>

  <xsl:choose>
    <xsl:when test="count($target)=0">
      <xsl:value-of select="$arearef"/>
      <xsl:text>: ???</xsl:text>
    </xsl:when>
    <xsl:when test="local-name($target)='co'">
      <xsl:apply-templates select="$target" mode="callout-bug"/>
    </xsl:when>
    <xsl:when test="local-name($target)='areaset'">
      <xsl:call-template name="callout-bug">
        <xsl:with-param name="conum">
          <xsl:apply-templates select="$target" mode="conumber"/>
        </xsl:with-param>
      </xsl:call-template>
    </xsl:when>
    <xsl:when test="local-name($target)='area'">
      <xsl:choose>
        <xsl:when test="$target/parent::areaset">
          <xsl:call-template name="callout-bug">
            <xsl:with-param name="conum">
              <xsl:apply-templates select="$target/parent::areaset"
                                   mode="conumber"/>
            </xsl:with-param>
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="callout-bug">
            <xsl:with-param name="conum">
              <xsl:apply-templates select="$target" mode="conumber"/>
            </xsl:with-param>
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <xsl:otherwise>
      <xsl:text>???</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- ==================================================================== -->

</xsl:stylesheet>

