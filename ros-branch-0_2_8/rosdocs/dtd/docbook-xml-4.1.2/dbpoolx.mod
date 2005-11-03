<!-- ...................................................................... -->
<!-- DocBook XML information pool module V4.1.2 ............................. -->
<!-- File dbpoolx.mod ..................................................... -->

<!-- Copyright 1992-2000 HaL Computer Systems, Inc.,
     O'Reilly & Associates, Inc., ArborText, Inc., Fujitsu Software
     Corporation, Norman Walsh and the Organization for the Advancement
     of Structured Information Standards (OASIS).

     $Id: dbpoolx.mod,v 1.1 2001/10/09 17:54:26 jfilby Exp $

     Permission to use, copy, modify and distribute the DocBook XML DTD
     and its accompanying documentation for any purpose and without fee
     is hereby granted in perpetuity, provided that the above copyright
     notice and this paragraph appear in all copies.  The copyright
     holders make no representation about the suitability of the DTD for
     any purpose.  It is provided "as is" without expressed or implied
     warranty.

     If you modify the DocBook XML DTD in any way, except for declaring and
     referencing additional sets of general entities and declaring
     additional notations, label your DTD as a variant of DocBook.  See
     the maintenance documentation for more information.

     Please direct all questions, bug reports, or suggestions for
     changes to the docbook@lists.oasis-open.org mailing list. For more
     information, see http://www.oasis-open.org/docbook/.
-->

<!-- ...................................................................... -->

<!-- This module contains the definitions for the objects, inline
     elements, and so on that are available to be used as the main
     content of DocBook documents.  Some elements are useful for general
     publishing, and others are useful specifically for computer
     documentation.

     This module has the following dependencies on other modules:

     o It assumes that a %notation.class; entity is defined by the
       driver file or other high-level module.  This entity is
       referenced in the NOTATION attributes for the graphic-related and
       ModeSpec elements.

     o It assumes that an appropriately parameterized table module is
       available for use with the table-related elements.

     In DTD driver files referring to this module, please use an entity
     declaration that uses the public identifier shown below:

     <!ENTITY % dbpool PUBLIC
     "-//OASIS//ELEMENTS DocBook XML Information Pool V4.1.2//EN"
     "dbpoolx.mod">
     %dbpool;

     See the documentation for detailed information on the parameter
     entity and module scheme used in DocBook, customizing DocBook and
     planning for interchange, and changes made since the last release
     of DocBook.
-->

<!-- ...................................................................... -->
<!-- General-purpose semantics entities ................................... -->

<!ENTITY % yesorno.attvals	"CDATA">

<!-- ...................................................................... -->
<!-- Entities for module inclusions ....................................... -->

<!ENTITY % dbpool.redecl.module "IGNORE">

<!-- ...................................................................... -->
<!-- Entities for element classes and mixtures ............................ -->

<!-- "Ubiquitous" classes: ndxterm.class and beginpage -->

<!ENTITY % local.ndxterm.class "">
<!ENTITY % ndxterm.class
		"indexterm %local.ndxterm.class;">

<!-- Object-level classes ................................................. -->

<!ENTITY % local.list.class "">
<!ENTITY % list.class
		"calloutlist|glosslist|itemizedlist|orderedlist|segmentedlist
		|simplelist|variablelist %local.list.class;">

<!ENTITY % local.admon.class "">
<!ENTITY % admon.class
		"caution|important|note|tip|warning %local.admon.class;">

<!ENTITY % local.linespecific.class "">
<!ENTITY % linespecific.class
		"literallayout|programlisting|programlistingco|screen
		|screenco|screenshot %local.linespecific.class;">

<!ENTITY % local.method.synop.class "">
<!ENTITY % method.synop.class
		"constructorsynopsis
                 |destructorsynopsis
                 |methodsynopsis %local.method.synop.class;">

<!ENTITY % local.synop.class "">
<!ENTITY % synop.class
		"synopsis|cmdsynopsis|funcsynopsis
                 |classsynopsis|fieldsynopsis
                 |%method.synop.class; %local.synop.class;">

<!ENTITY % local.para.class "">
<!ENTITY % para.class
		"formalpara|para|simpara %local.para.class;">

<!ENTITY % local.informal.class "">
<!ENTITY % informal.class
		"address|blockquote
                |graphic|graphicco|mediaobject|mediaobjectco
                |informalequation
		|informalexample
                |informalfigure
                |informaltable %local.informal.class;">

<!ENTITY % local.formal.class "">
<!ENTITY % formal.class
		"equation|example|figure|table %local.formal.class;">

<!-- The DocBook TC may produce an official EBNF module for DocBook. -->
<!-- This PE provides the hook by which it can be inserted into the DTD. -->
<!ENTITY % ebnf.block.hook "">

<!ENTITY % local.compound.class "">
<!ENTITY % compound.class
		"msgset|procedure|sidebar|qandaset
                 %ebnf.block.hook;
                 %local.compound.class;">

<!ENTITY % local.genobj.class "">
<!ENTITY % genobj.class
		"anchor|bridgehead|remark|highlights
		%local.genobj.class;">

<!ENTITY % local.descobj.class "">
<!ENTITY % descobj.class
		"abstract|authorblurb|epigraph
		%local.descobj.class;">

<!-- Character-level classes .............................................. -->

<!ENTITY % local.xref.char.class "">
<!ENTITY % xref.char.class
		"footnoteref|xref %local.xref.char.class;">

<!ENTITY % local.gen.char.class "">
<!ENTITY % gen.char.class
		"abbrev|acronym|citation|citerefentry|citetitle|emphasis
		|firstterm|foreignphrase|glossterm|footnote|phrase
		|quote|trademark|wordasword %local.gen.char.class;">

<!ENTITY % local.link.char.class "">
<!ENTITY % link.char.class
		"link|olink|ulink %local.link.char.class;">

<!-- The DocBook TC may produce an official EBNF module for DocBook. -->
<!-- This PE provides the hook by which it can be inserted into the DTD. -->
<!ENTITY % ebnf.inline.hook "">

<!ENTITY % local.tech.char.class "">
<!ENTITY % tech.char.class
		"action|application
                |classname|methodname|interfacename|exceptionname
                |ooclass|oointerface|ooexception
                |command|computeroutput
		|database|email|envar|errorcode|errorname|errortype|filename
		|function|guibutton|guiicon|guilabel|guimenu|guimenuitem
		|guisubmenu|hardware|interface|keycap
		|keycode|keycombo|keysym|literal|constant|markup|medialabel
		|menuchoice|mousebutton|option|optional|parameter
		|prompt|property|replaceable|returnvalue|sgmltag|structfield
		|structname|symbol|systemitem|token|type|userinput|varname
                %ebnf.inline.hook;
		%local.tech.char.class;">

<!ENTITY % local.base.char.class "">
<!ENTITY % base.char.class
		"anchor %local.base.char.class;">

<!ENTITY % local.docinfo.char.class "">
<!ENTITY % docinfo.char.class
		"author|authorinitials|corpauthor|modespec|othercredit
		|productname|productnumber|revhistory
		%local.docinfo.char.class;">

<!ENTITY % local.other.char.class "">
<!ENTITY % other.char.class
		"remark|subscript|superscript %local.other.char.class;">

<!ENTITY % local.inlineobj.char.class "">
<!ENTITY % inlineobj.char.class
		"inlinegraphic|inlinemediaobject|inlineequation %local.inlineobj.char.class;">

<!-- Redeclaration placeholder ............................................ -->

<!-- For redeclaring entities that are declared after this point while
     retaining their references to the entities that are declared before
     this point -->

<![%dbpool.redecl.module;[
<!-- Defining rdbpool here makes some buggy XML parsers happy. -->
<!ENTITY % rdbpool "">
%rdbpool;
<!--end of dbpool.redecl.module-->]]>

<!-- Object-level mixtures ................................................ -->

<!--
                      list admn line synp para infm form cmpd gen  desc
Component mixture       X    X    X    X    X    X    X    X    X    X
Sidebar mixture         X    X    X    X    X    X    X    a    X
Footnote mixture        X         X    X    X    X
Example mixture         X         X    X    X    X
Highlights mixture      X    X              X
Paragraph mixture       X         X    X         X
Admonition mixture      X         X    X    X    X    X    b    c
Figure mixture                    X    X         X
Table entry mixture     X    X    X         X    d
Glossary def mixture    X         X    X    X    X         e
Legal notice mixture    X    X    X         X    f

a. Just Procedure; not Sidebar itself or MsgSet.
b. No MsgSet.
c. No Highlights.
d. Just Graphic; no other informal objects.
e. No Anchor, BridgeHead, or Highlights.
f. Just BlockQuote; no other informal objects.
-->

<!ENTITY % local.component.mix "">
<!ENTITY % component.mix
		"%list.class;		|%admon.class;
		|%linespecific.class;	|%synop.class;
		|%para.class;		|%informal.class;
		|%formal.class;		|%compound.class;
		|%genobj.class;		|%descobj.class;
		|%ndxterm.class;        |beginpage
		%local.component.mix;">

<!ENTITY % local.sidebar.mix "">
<!ENTITY % sidebar.mix
		"%list.class;		|%admon.class;
		|%linespecific.class;	|%synop.class;
		|%para.class;		|%informal.class;
		|%formal.class;		|procedure
		|%genobj.class;
		|%ndxterm.class;        |beginpage
		%local.sidebar.mix;">

<!ENTITY % local.qandaset.mix "">
<!ENTITY % qandaset.mix
		"%list.class;           |%admon.class;
		|%linespecific.class;	|%synop.class;
		|%para.class;		|%informal.class;
		|%formal.class;		|procedure
		|%genobj.class;
		|%ndxterm.class;
		%local.qandaset.mix;">

<!ENTITY % local.revdescription.mix "">
<!ENTITY % revdescription.mix
		"%list.class;		|%admon.class;
		|%linespecific.class;	|%synop.class;
		|%para.class;		|%informal.class;
		|%formal.class;		|procedure
		|%genobj.class;
		|%ndxterm.class;
		%local.revdescription.mix;">

<!ENTITY % local.footnote.mix "">
<!ENTITY % footnote.mix
		"%list.class;
		|%linespecific.class;	|%synop.class;
		|%para.class;		|%informal.class;
		%local.footnote.mix;">

<!ENTITY % local.example.mix "">
<!ENTITY % example.mix
		"%list.class;
		|%linespecific.class;	|%synop.class;
		|%para.class;		|%informal.class;
		|%ndxterm.class;        |beginpage
		%local.example.mix;">

<!ENTITY % local.highlights.mix "">
<!ENTITY % highlights.mix
		"%list.class;		|%admon.class;
		|%para.class;
		|%ndxterm.class;
		%local.highlights.mix;">

<!-- %formal.class; is explicitly excluded from many contexts in which
     paragraphs are used -->
<!ENTITY % local.para.mix "">
<!ENTITY % para.mix
		"%list.class;           |%admon.class;
		|%linespecific.class;
					|%informal.class;
		|%formal.class;
		%local.para.mix;">

<!ENTITY % local.admon.mix "">
<!ENTITY % admon.mix
		"%list.class;
		|%linespecific.class;	|%synop.class;
		|%para.class;		|%informal.class;
		|%formal.class;		|procedure|sidebar
		|anchor|bridgehead|remark
		|%ndxterm.class;        |beginpage
		%local.admon.mix;">

<!ENTITY % local.figure.mix "">
<!ENTITY % figure.mix
		"%linespecific.class;	|%synop.class;
					|%informal.class;
		|%ndxterm.class;        |beginpage
		%local.figure.mix;">

<!ENTITY % local.tabentry.mix "">
<!ENTITY % tabentry.mix
		"%list.class;		|%admon.class;
		|%linespecific.class;
		|%para.class;		|graphic|mediaobject
		%local.tabentry.mix;">

<!ENTITY % local.glossdef.mix "">
<!ENTITY % glossdef.mix
		"%list.class;
		|%linespecific.class;	|%synop.class;
		|%para.class;		|%informal.class;
		|%formal.class;
		|remark
		|%ndxterm.class;        |beginpage
		%local.glossdef.mix;">

<!ENTITY % local.legalnotice.mix "">
<!ENTITY % legalnotice.mix
		"%list.class;		|%admon.class;
		|%linespecific.class;
		|%para.class;		|blockquote
		|%ndxterm.class;        |beginpage
		%local.legalnotice.mix;">

<!ENTITY % local.textobject.mix "">
<!ENTITY % textobject.mix
		"%list.class;		|%admon.class;
		|%linespecific.class;
		|%para.class;		|blockquote
		%local.textobject.mix;">

<!ENTITY % local.mediaobject.mix "">
<!ENTITY % mediaobject.mix 
		"videoobject|audioobject|imageobject %local.mediaobject.mix;">

<!-- Character-level mixtures ............................................. -->

<!--
                    #PCD xref word link cptr base dnfo othr inob (synop)
para.char.mix         X    X    X    X    X    X    X    X    X
title.char.mix        X    X    X    X    X    X    X    X    X
ndxterm.char.mix      X    X    X    X    X    X    X    X    a
cptr.char.mix         X              X    X    X         X    a
smallcptr.char.mix    X                   b                   a
word.char.mix         X         c    X         X         X    a
docinfo.char.mix      X         d    X    b              X    a

a. Just InlineGraphic; no InlineEquation.
b. Just Replaceable; no other computer terms.
c. Just Emphasis and Trademark; no other word elements.
d. Just Acronym, Emphasis, and Trademark; no other word elements.
-->

<!-- The DocBook TC may produce an official forms module for DocBook. -->
<!-- This PE provides the hook by which it can be inserted into the DTD. -->
<!ENTITY % forminlines.hook "">

<!ENTITY % local.para.char.mix "">
<!ENTITY % para.char.mix
		"#PCDATA
		|%xref.char.class;	|%gen.char.class;
		|%link.char.class;	|%tech.char.class;
		|%base.char.class;	|%docinfo.char.class;
		|%other.char.class;	|%inlineobj.char.class;
		|%synop.class;
		|%ndxterm.class;        |beginpage
                %forminlines.hook;
		%local.para.char.mix;">

<!ENTITY % local.title.char.mix "">
<!ENTITY % title.char.mix
		"#PCDATA
		|%xref.char.class;	|%gen.char.class;
		|%link.char.class;	|%tech.char.class;
		|%base.char.class;	|%docinfo.char.class;
		|%other.char.class;	|%inlineobj.char.class;
		|%ndxterm.class;
		%local.title.char.mix;">

<!ENTITY % local.ndxterm.char.mix "">
<!ENTITY % ndxterm.char.mix
		"#PCDATA
		|%xref.char.class;	|%gen.char.class;
		|%link.char.class;	|%tech.char.class;
		|%base.char.class;	|%docinfo.char.class;
		|%other.char.class;	|inlinegraphic|inlinemediaobject
		%local.ndxterm.char.mix;">

<!ENTITY % local.cptr.char.mix "">
<!ENTITY % cptr.char.mix
		"#PCDATA
		|%link.char.class;	|%tech.char.class;
		|%base.char.class;
		|%other.char.class;	|inlinegraphic|inlinemediaobject
		|%ndxterm.class;        |beginpage
		%local.cptr.char.mix;">

<!ENTITY % local.smallcptr.char.mix "">
<!ENTITY % smallcptr.char.mix
		"#PCDATA
					|replaceable
					|inlinegraphic|inlinemediaobject
		|%ndxterm.class;        |beginpage
		%local.smallcptr.char.mix;">

<!ENTITY % local.word.char.mix "">
<!ENTITY % word.char.mix
		"#PCDATA
					|acronym|emphasis|trademark
		|%link.char.class;
		|%base.char.class;
		|%other.char.class;	|inlinegraphic|inlinemediaobject
		|%ndxterm.class;        |beginpage
		%local.word.char.mix;">

<!ENTITY % local.docinfo.char.mix "">
<!ENTITY % docinfo.char.mix
		"#PCDATA
		|%link.char.class;
					|emphasis|trademark
					|replaceable
		|%other.char.class;	|inlinegraphic|inlinemediaobject
		|%ndxterm.class;
		%local.docinfo.char.mix;">
<!--ENTITY % bibliocomponent.mix (see Bibliographic section, below)-->
<!--ENTITY % person.ident.mix (see Bibliographic section, below)-->

<!-- ...................................................................... -->
<!-- Entities for content models .......................................... -->

<!ENTITY % formalobject.title.content "title, titleabbrev?">

<!-- ...................................................................... -->
<!-- Entities for attributes and attribute components ..................... -->

<!-- Effectivity attributes ............................................... -->


<!-- Arch: Computer or chip architecture to which element applies; no 
	default -->

<!ENTITY % arch.attrib
	"arch		CDATA		#IMPLIED">

<!-- Condition: General-purpose effectivity attribute -->

<!ENTITY % condition.attrib
	"condition	CDATA		#IMPLIED">

<!-- Conformance: Standards conformance characteristics -->

<!ENTITY % conformance.attrib
	"conformance	NMTOKENS	#IMPLIED">


<!-- OS: Operating system to which element applies; no default -->

<!ENTITY % os.attrib
	"os		CDATA		#IMPLIED">


<!-- Revision: Editorial revision to which element belongs; no default -->

<!ENTITY % revision.attrib
	"revision	CDATA		#IMPLIED">

<!-- Security: Security classification; no default -->

<!ENTITY % security.attrib
	"security	CDATA		#IMPLIED">

<!-- UserLevel: Level of user experience to which element applies; no 
	default -->

<!ENTITY % userlevel.attrib
	"userlevel	CDATA		#IMPLIED">


<!-- Vendor: Computer vendor to which element applies; no default -->

<!ENTITY % vendor.attrib
	"vendor		CDATA		#IMPLIED">

<!ENTITY % local.effectivity.attrib "">
<!ENTITY % effectivity.attrib
	"%arch.attrib;
        %condition.attrib;
	%conformance.attrib;
	%os.attrib;
	%revision.attrib;
        %security.attrib;
	%userlevel.attrib;
	%vendor.attrib;
	%local.effectivity.attrib;"
>

<!-- Common attributes .................................................... -->


<!-- Id: Unique identifier of element; no default -->

<!ENTITY % id.attrib
	"id		ID		#IMPLIED">


<!-- Id: Unique identifier of element; a value must be supplied; no 
	default -->

<!ENTITY % idreq.attrib
	"id		ID		#REQUIRED">


<!-- Lang: Indicator of language in which element is written, for
	translation, character set management, etc.; no default -->

<!ENTITY % lang.attrib
	"lang		CDATA		#IMPLIED">


<!-- Remap: Previous role of element before conversion; no default -->

<!ENTITY % remap.attrib
	"remap		CDATA		#IMPLIED">


<!-- Role: New role of element in local environment; no default -->

<!ENTITY % role.attrib
	"role		CDATA		#IMPLIED">


<!-- XRefLabel: Alternate labeling string for XRef text generation;
	default is usually title or other appropriate label text already
	contained in element -->

<!ENTITY % xreflabel.attrib
	"xreflabel	CDATA		#IMPLIED">


<!-- RevisionFlag: Revision status of element; default is that element
	wasn't revised -->

<!ENTITY % revisionflag.attrib
	"revisionflag	(changed
			|added
			|deleted
			|off)		#IMPLIED">

<!ENTITY % local.common.attrib "">

<!-- Role is included explicitly on each element -->

<!ENTITY % common.attrib
	"%id.attrib;
	%lang.attrib;
	%remap.attrib;
	%xreflabel.attrib;
	%revisionflag.attrib;
	%effectivity.attrib;
	%local.common.attrib;"
>


<!-- Role is included explicitly on each element -->

<!ENTITY % idreq.common.attrib
	"%idreq.attrib;
	%lang.attrib;
	%remap.attrib;
	%xreflabel.attrib;
	%revisionflag.attrib;
	%effectivity.attrib;
	%local.common.attrib;"
>

<!-- Semi-common attributes and other attribute entities .................. -->

<!ENTITY % local.graphics.attrib "">

<!-- EntityRef: Name of an external entity containing the content
	of the graphic -->
<!-- FileRef: Filename, qualified by a pathname if desired, 
	designating the file containing the content of the graphic -->
<!-- Format: Notation of the element content, if any -->
<!-- SrcCredit: Information about the source of the Graphic -->
<!-- Width: Same as CALS reprowid (desired width) -->
<!-- Depth: Same as CALS reprodep (desired depth) -->
<!-- Align: Same as CALS hplace with 'none' removed; #IMPLIED means 
	application-specific -->
<!-- Scale: Conflation of CALS hscale and vscale -->
<!-- Scalefit: Same as CALS scalefit -->

<!ENTITY % graphics.attrib
	"
	entityref	ENTITY		#IMPLIED
	fileref 	CDATA		#IMPLIED
	format		(%notation.class;) #IMPLIED
	srccredit	CDATA		#IMPLIED
	width		CDATA		#IMPLIED
	depth		CDATA		#IMPLIED
	align		(left
			|right 
			|center)	#IMPLIED
	scale		CDATA		#IMPLIED
	scalefit	%yesorno.attvals;
					#IMPLIED
	%local.graphics.attrib;"
>

<!ENTITY % local.keyaction.attrib "">

<!-- Action: Key combination type; default is unspecified if one 
	child element, Simul if there is more than one; if value is 
	Other, the OtherAction attribute must have a nonempty value -->
<!-- OtherAction: User-defined key combination type -->

<!ENTITY % keyaction.attrib
	"
	action		(click
			|double-click
			|press
			|seq
			|simul
			|other)		#IMPLIED
	otheraction	CDATA		#IMPLIED
	%local.keyaction.attrib;"
>


<!-- Label: Identifying number or string; default is usually the
	appropriate number or string autogenerated by a formatter -->

<!ENTITY % label.attrib
	"label		CDATA		#IMPLIED">


<!-- Format: whether element is assumed to contain significant white
	space -->

<!ENTITY % linespecific.attrib
	"format		NOTATION
			(linespecific)	'linespecific'
         linenumbering	(numbered|unnumbered) 	#IMPLIED">


<!-- Linkend: link to related information; no default -->

<!ENTITY % linkend.attrib
	"linkend	IDREF		#IMPLIED">


<!-- Linkend: required link to related information -->

<!ENTITY % linkendreq.attrib
	"linkend	IDREF		#REQUIRED">


<!-- Linkends: link to one or more sets of related information; no 
	default -->

<!ENTITY % linkends.attrib
	"linkends	IDREFS		#IMPLIED">


<!ENTITY % local.mark.attrib "">
<!ENTITY % mark.attrib
	"mark		CDATA		#IMPLIED
	%local.mark.attrib;"
>


<!-- MoreInfo: whether element's content has an associated RefEntry -->

<!ENTITY % moreinfo.attrib
	"moreinfo	(refentry|none)	'none'">


<!-- Pagenum: number of page on which element appears; no default -->

<!ENTITY % pagenum.attrib
	"pagenum	CDATA		#IMPLIED">

<!ENTITY % local.status.attrib "">

<!-- Status: Editorial or publication status of the element
	it applies to, such as "in review" or "approved for distribution" -->

<!ENTITY % status.attrib
	"status		CDATA		#IMPLIED
	%local.status.attrib;"
>


<!-- Width: width of the longest line in the element to which it
	pertains, in number of characters -->

<!ENTITY % width.attrib
	"width		CDATA		#IMPLIED">

<!-- ...................................................................... -->
<!-- Title elements ....................................................... -->

<!ENTITY % title.module "INCLUDE">
<![%title.module;[
<!ENTITY % local.title.attrib "">
<!ENTITY % title.role.attrib "%role.attrib;">

<!ENTITY % title.element "INCLUDE">
<![%title.element;[
<!ELEMENT title (%title.char.mix;)*>
<!--end of title.element-->]]>

<!ENTITY % title.attlist "INCLUDE">
<![%title.attlist;[
<!ATTLIST title
		%pagenum.attrib;
		%common.attrib;
		%title.role.attrib;
		%local.title.attrib;
>
<!--end of title.attlist-->]]>
<!--end of title.module-->]]>

<!ENTITY % titleabbrev.module "INCLUDE">
<![%titleabbrev.module;[
<!ENTITY % local.titleabbrev.attrib "">
<!ENTITY % titleabbrev.role.attrib "%role.attrib;">

<!ENTITY % titleabbrev.element "INCLUDE">
<![%titleabbrev.element;[
<!ELEMENT titleabbrev (%title.char.mix;)*>
<!--end of titleabbrev.element-->]]>

<!ENTITY % titleabbrev.attlist "INCLUDE">
<![%titleabbrev.attlist;[
<!ATTLIST titleabbrev
		%common.attrib;
		%titleabbrev.role.attrib;
		%local.titleabbrev.attrib;
>
<!--end of titleabbrev.attlist-->]]>
<!--end of titleabbrev.module-->]]>

<!ENTITY % subtitle.module "INCLUDE">
<![%subtitle.module;[
<!ENTITY % local.subtitle.attrib "">
<!ENTITY % subtitle.role.attrib "%role.attrib;">

<!ENTITY % subtitle.element "INCLUDE">
<![%subtitle.element;[
<!ELEMENT subtitle (%title.char.mix;)*>
<!--end of subtitle.element-->]]>

<!ENTITY % subtitle.attlist "INCLUDE">
<![%subtitle.attlist;[
<!ATTLIST subtitle
		%common.attrib;
		%subtitle.role.attrib;
		%local.subtitle.attrib;
>
<!--end of subtitle.attlist-->]]>
<!--end of subtitle.module-->]]>

<!-- ...................................................................... -->
<!-- Bibliographic entities and elements .................................. -->

<!-- The bibliographic elements are typically used in the document
     hierarchy. They do not appear in content models of information
     pool elements.  See also the document information elements,
     below. -->

<!ENTITY % local.person.ident.mix "">
<!ENTITY % person.ident.mix
		"honorific|firstname|surname|lineage|othername|affiliation
		|authorblurb|contrib %local.person.ident.mix;">

<!ENTITY % local.bibliocomponent.mix "">
<!ENTITY % bibliocomponent.mix
		"abbrev|abstract|address|artpagenums|author
		|authorgroup|authorinitials|bibliomisc|biblioset
		|collab|confgroup|contractnum|contractsponsor
		|copyright|corpauthor|corpname|date|edition
		|editor|invpartnumber|isbn|issn|issuenum|orgname
		|othercredit|pagenums|printhistory|productname
		|productnumber|pubdate|publisher|publishername
		|pubsnumber|releaseinfo|revhistory|seriesvolnums
		|subtitle|title|titleabbrev|volumenum|citetitle
		|%person.ident.mix;
		|%ndxterm.class;
		%local.bibliocomponent.mix;">

<!ENTITY % biblioentry.module "INCLUDE">
<![%biblioentry.module;[
<!ENTITY % local.biblioentry.attrib "">
<!ENTITY % biblioentry.role.attrib "%role.attrib;">

<!ENTITY % biblioentry.element "INCLUDE">
<![%biblioentry.element;[
<!ELEMENT biblioentry ((articleinfo | (%bibliocomponent.mix;))+)>
<!--end of biblioentry.element-->]]>

<!ENTITY % biblioentry.attlist "INCLUDE">
<![%biblioentry.attlist;[
<!ATTLIST biblioentry
		%common.attrib;
		%biblioentry.role.attrib;
		%local.biblioentry.attrib;
>
<!--end of biblioentry.attlist-->]]>
<!--end of biblioentry.module-->]]>

<!ENTITY % bibliomixed.module "INCLUDE">
<![%bibliomixed.module;[
<!ENTITY % local.bibliomixed.attrib "">
<!ENTITY % bibliomixed.role.attrib "%role.attrib;">

<!ENTITY % bibliomixed.element "INCLUDE">
<![%bibliomixed.element;[
<!ELEMENT bibliomixed (#PCDATA | %bibliocomponent.mix; | bibliomset)*>
<!--end of bibliomixed.element-->]]>

<!ENTITY % bibliomixed.attlist "INCLUDE">
<![%bibliomixed.attlist;[
<!ATTLIST bibliomixed
		%common.attrib;
		%bibliomixed.role.attrib;
		%local.bibliomixed.attrib;
>
<!--end of bibliomixed.attlist-->]]>
<!--end of bibliomixed.module-->]]>

<!ENTITY % articleinfo.module "INCLUDE">
<![%articleinfo.module;[
<!ENTITY % local.articleinfo.attrib "">
<!ENTITY % articleinfo.role.attrib "%role.attrib;">

<!ENTITY % articleinfo.element "INCLUDE">
<![%articleinfo.element;[
<!ELEMENT articleinfo ((graphic | mediaobject | legalnotice | modespec 
	| subjectset | keywordset | itermset | %bibliocomponent.mix;)+)>
<!--end of articleinfo.element-->]]>

<!ENTITY % articleinfo.attlist "INCLUDE">
<![%articleinfo.attlist;[
<!ATTLIST articleinfo
		%common.attrib;
		%articleinfo.role.attrib;
		%local.articleinfo.attrib;
>
<!--end of articleinfo.attlist-->]]>
<!--end of articleinfo.module-->]]>

<!ENTITY % biblioset.module "INCLUDE">
<![%biblioset.module;[
<!ENTITY % local.biblioset.attrib "">
<!ENTITY % biblioset.role.attrib "%role.attrib;">

<!ENTITY % biblioset.element "INCLUDE">
<![%biblioset.element;[
<!ELEMENT biblioset ((%bibliocomponent.mix;)+)>
<!--end of biblioset.element-->]]>

<!-- Relation: Relationship of elements contained within BiblioSet -->


<!ENTITY % biblioset.attlist "INCLUDE">
<![%biblioset.attlist;[
<!ATTLIST biblioset
		relation	CDATA		#IMPLIED
		%common.attrib;
		%biblioset.role.attrib;
		%local.biblioset.attrib;
>
<!--end of biblioset.attlist-->]]>
<!--end of biblioset.module-->]]>

<!ENTITY % bibliomset.module "INCLUDE">
<![%bibliomset.module;[
<!ENTITY % bibliomset.role.attrib "%role.attrib;">
<!ENTITY % local.bibliomset.attrib "">

<!ENTITY % bibliomset.element "INCLUDE">
<![%bibliomset.element;[
<!ELEMENT bibliomset (#PCDATA | %bibliocomponent.mix; | bibliomset)*>
<!--end of bibliomset.element-->]]>

<!-- Relation: Relationship of elements contained within BiblioMSet -->


<!ENTITY % bibliomset.attlist "INCLUDE">
<![%bibliomset.attlist;[
<!ATTLIST bibliomset
		relation	CDATA		#IMPLIED
		%bibliomset.role.attrib;
		%common.attrib;
		%local.bibliomset.attrib;
>
<!--end of bibliomset.attlist-->]]>
<!--end of bibliomset.module-->]]>

<!ENTITY % bibliomisc.module "INCLUDE">
<![%bibliomisc.module;[
<!ENTITY % local.bibliomisc.attrib "">
<!ENTITY % bibliomisc.role.attrib "%role.attrib;">

<!ENTITY % bibliomisc.element "INCLUDE">
<![%bibliomisc.element;[
<!ELEMENT bibliomisc (%para.char.mix;)*>
<!--end of bibliomisc.element-->]]>

<!ENTITY % bibliomisc.attlist "INCLUDE">
<![%bibliomisc.attlist;[
<!ATTLIST bibliomisc
		%common.attrib;
		%bibliomisc.role.attrib;
		%local.bibliomisc.attrib;
>
<!--end of bibliomisc.attlist-->]]>
<!--end of bibliomisc.module-->]]>

<!-- ...................................................................... -->
<!-- Subject, Keyword, and ITermSet elements .............................. -->

<!ENTITY % subjectset.content.module "INCLUDE">
<![%subjectset.content.module;[
<!ENTITY % subjectset.module "INCLUDE">
<![%subjectset.module;[
<!ENTITY % local.subjectset.attrib "">
<!ENTITY % subjectset.role.attrib "%role.attrib;">

<!ENTITY % subjectset.element "INCLUDE">
<![%subjectset.element;[
<!ELEMENT subjectset (subject+)>
<!--end of subjectset.element-->]]>

<!-- Scheme: Controlled vocabulary employed in SubjectTerms -->


<!ENTITY % subjectset.attlist "INCLUDE">
<![%subjectset.attlist;[
<!ATTLIST subjectset
		scheme		NMTOKEN		#IMPLIED
		%common.attrib;
		%subjectset.role.attrib;
		%local.subjectset.attrib;
>
<!--end of subjectset.attlist-->]]>
<!--end of subjectset.module-->]]>

<!ENTITY % subject.module "INCLUDE">
<![%subject.module;[
<!ENTITY % local.subject.attrib "">
<!ENTITY % subject.role.attrib "%role.attrib;">

<!ENTITY % subject.element "INCLUDE">
<![%subject.element;[
<!ELEMENT subject (subjectterm+)>
<!--end of subject.element-->]]>

<!-- Weight: Ranking of this group of SubjectTerms relative 
		to others, 0 is low, no highest value specified -->


<!ENTITY % subject.attlist "INCLUDE">
<![%subject.attlist;[
<!ATTLIST subject
		weight		CDATA		#IMPLIED
		%common.attrib;
		%subject.role.attrib;
		%local.subject.attrib;
>
<!--end of subject.attlist-->]]>
<!--end of subject.module-->]]>

<!ENTITY % subjectterm.module "INCLUDE">
<![%subjectterm.module;[
<!ENTITY % local.subjectterm.attrib "">
<!ENTITY % subjectterm.role.attrib "%role.attrib;">

<!ENTITY % subjectterm.element "INCLUDE">
<![%subjectterm.element;[
<!ELEMENT subjectterm (#PCDATA)>
<!--end of subjectterm.element-->]]>

<!ENTITY % subjectterm.attlist "INCLUDE">
<![%subjectterm.attlist;[
<!ATTLIST subjectterm
		%common.attrib;
		%subjectterm.role.attrib;
		%local.subjectterm.attrib;
>
<!--end of subjectterm.attlist-->]]>
<!--end of subjectterm.module-->]]>
<!--end of subjectset.content.module-->]]>

<!ENTITY % keywordset.content.module "INCLUDE">
<![%keywordset.content.module;[
<!ENTITY % keywordset.module "INCLUDE">
<![%keywordset.module;[
<!ENTITY % local.keywordset.attrib "">
<!ENTITY % keywordset.role.attrib "%role.attrib;">

<!ENTITY % keywordset.element "INCLUDE">
<![%keywordset.element;[
<!ELEMENT keywordset (keyword+)>
<!--end of keywordset.element-->]]>

<!ENTITY % keywordset.attlist "INCLUDE">
<![%keywordset.attlist;[
<!ATTLIST keywordset
		%common.attrib;
		%keywordset.role.attrib;
		%local.keywordset.attrib;
>
<!--end of keywordset.attlist-->]]>
<!--end of keywordset.module-->]]>

<!ENTITY % keyword.module "INCLUDE">
<![%keyword.module;[
<!ENTITY % local.keyword.attrib "">
<!ENTITY % keyword.role.attrib "%role.attrib;">

<!ENTITY % keyword.element "INCLUDE">
<![%keyword.element;[
<!ELEMENT keyword (#PCDATA)>
<!--end of keyword.element-->]]>

<!ENTITY % keyword.attlist "INCLUDE">
<![%keyword.attlist;[
<!ATTLIST keyword
		%common.attrib;
		%keyword.role.attrib;
		%local.keyword.attrib;
>
<!--end of keyword.attlist-->]]>
<!--end of keyword.module-->]]>
<!--end of keywordset.content.module-->]]>

<!ENTITY % itermset.module "INCLUDE">
<![%itermset.module;[
<!ENTITY % local.itermset.attrib "">
<!ENTITY % itermset.role.attrib "%role.attrib;">

<!ENTITY % itermset.element "INCLUDE">
<![%itermset.element;[
<!ELEMENT itermset (indexterm+)>
<!--end of itermset.element-->]]>

<!ENTITY % itermset.attlist "INCLUDE">
<![%itermset.attlist;[
<!ATTLIST itermset
		%common.attrib;
		%itermset.role.attrib;
		%local.itermset.attrib;
>
<!--end of itermset.attlist-->]]>
<!--end of itermset.module-->]]>

<!-- ...................................................................... -->
<!-- Compound (section-ish) elements ...................................... -->

<!-- Message set ...................... -->

<!ENTITY % msgset.content.module "INCLUDE">
<![%msgset.content.module;[
<!ENTITY % msgset.module "INCLUDE">
<![%msgset.module;[
<!ENTITY % local.msgset.attrib "">
<!ENTITY % msgset.role.attrib "%role.attrib;">

<!ENTITY % msgset.element "INCLUDE">
<![%msgset.element;[
<!ELEMENT msgset ((%formalobject.title.content;)?, (msgentry+|simplemsgentry+))>
<!--end of msgset.element-->]]>

<!ENTITY % msgset.attlist "INCLUDE">
<![%msgset.attlist;[
<!ATTLIST msgset
		%common.attrib;
		%msgset.role.attrib;
		%local.msgset.attrib;
>
<!--end of msgset.attlist-->]]>
<!--end of msgset.module-->]]>

<!ENTITY % msgentry.module "INCLUDE">
<![%msgentry.module;[
<!ENTITY % local.msgentry.attrib "">
<!ENTITY % msgentry.role.attrib "%role.attrib;">

<!ENTITY % msgentry.element "INCLUDE">
<![%msgentry.element;[
<!ELEMENT msgentry (msg+, msginfo?, msgexplan*)>
<!--end of msgentry.element-->]]>

<!ENTITY % msgentry.attlist "INCLUDE">
<![%msgentry.attlist;[
<!ATTLIST msgentry
		%common.attrib;
		%msgentry.role.attrib;
		%local.msgentry.attrib;
>
<!--end of msgentry.attlist-->]]>
<!--end of msgentry.module-->]]>

<!ENTITY % simplemsgentry.module "INCLUDE">
<![ %simplemsgentry.module; [
<!ENTITY % local.simplemsgentry.attrib "">
<!ENTITY % simplemsgentry.role.attrib "%role.attrib;">

<!ENTITY % simplemsgentry.element "INCLUDE">
<![ %simplemsgentry.element; [
<!ELEMENT simplemsgentry (msgtext, msgexplan)>
<!--end of simplemsgentry.element-->]]>

<!ENTITY % simplemsgentry.attlist "INCLUDE">
<![ %simplemsgentry.attlist; [
<!ATTLIST simplemsgentry
		%common.attrib;
		%simplemsgentry.role.attrib;
		%local.simplemsgentry.attrib;
		audience	CDATA	#IMPLIED
		level		CDATA	#IMPLIED
		origin		CDATA	#IMPLIED
>
<!--end of simplemsgentry.attlist-->]]>
<!--end of simplemsgentry.module-->]]>

<!ENTITY % msg.module "INCLUDE">
<![%msg.module;[
<!ENTITY % local.msg.attrib "">
<!ENTITY % msg.role.attrib "%role.attrib;">

<!ENTITY % msg.element "INCLUDE">
<![%msg.element;[
<!ELEMENT msg (title?, msgmain, (msgsub | msgrel)*)>
<!--end of msg.element-->]]>

<!ENTITY % msg.attlist "INCLUDE">
<![%msg.attlist;[
<!ATTLIST msg
		%common.attrib;
		%msg.role.attrib;
		%local.msg.attrib;
>
<!--end of msg.attlist-->]]>
<!--end of msg.module-->]]>

<!ENTITY % msgmain.module "INCLUDE">
<![%msgmain.module;[
<!ENTITY % local.msgmain.attrib "">
<!ENTITY % msgmain.role.attrib "%role.attrib;">

<!ENTITY % msgmain.element "INCLUDE">
<![%msgmain.element;[
<!ELEMENT msgmain (title?, msgtext)>
<!--end of msgmain.element-->]]>

<!ENTITY % msgmain.attlist "INCLUDE">
<![%msgmain.attlist;[
<!ATTLIST msgmain
		%common.attrib;
		%msgmain.role.attrib;
		%local.msgmain.attrib;
>
<!--end of msgmain.attlist-->]]>
<!--end of msgmain.module-->]]>

<!ENTITY % msgsub.module "INCLUDE">
<![%msgsub.module;[
<!ENTITY % local.msgsub.attrib "">
<!ENTITY % msgsub.role.attrib "%role.attrib;">

<!ENTITY % msgsub.element "INCLUDE">
<![%msgsub.element;[
<!ELEMENT msgsub (title?, msgtext)>
<!--end of msgsub.element-->]]>

<!ENTITY % msgsub.attlist "INCLUDE">
<![%msgsub.attlist;[
<!ATTLIST msgsub
		%common.attrib;
		%msgsub.role.attrib;
		%local.msgsub.attrib;
>
<!--end of msgsub.attlist-->]]>
<!--end of msgsub.module-->]]>

<!ENTITY % msgrel.module "INCLUDE">
<![%msgrel.module;[
<!ENTITY % local.msgrel.attrib "">
<!ENTITY % msgrel.role.attrib "%role.attrib;">

<!ENTITY % msgrel.element "INCLUDE">
<![%msgrel.element;[
<!ELEMENT msgrel (title?, msgtext)>
<!--end of msgrel.element-->]]>

<!ENTITY % msgrel.attlist "INCLUDE">
<![%msgrel.attlist;[
<!ATTLIST msgrel
		%common.attrib;
		%msgrel.role.attrib;
		%local.msgrel.attrib;
>
<!--end of msgrel.attlist-->]]>
<!--end of msgrel.module-->]]>

<!-- MsgText (defined in the Inlines section, below)-->

<!ENTITY % msginfo.module "INCLUDE">
<![%msginfo.module;[
<!ENTITY % local.msginfo.attrib "">
<!ENTITY % msginfo.role.attrib "%role.attrib;">

<!ENTITY % msginfo.element "INCLUDE">
<![%msginfo.element;[
<!ELEMENT msginfo ((msglevel | msgorig | msgaud)*)>
<!--end of msginfo.element-->]]>

<!ENTITY % msginfo.attlist "INCLUDE">
<![%msginfo.attlist;[
<!ATTLIST msginfo
		%common.attrib;
		%msginfo.role.attrib;
		%local.msginfo.attrib;
>
<!--end of msginfo.attlist-->]]>
<!--end of msginfo.module-->]]>

<!ENTITY % msglevel.module "INCLUDE">
<![%msglevel.module;[
<!ENTITY % local.msglevel.attrib "">
<!ENTITY % msglevel.role.attrib "%role.attrib;">

<!ENTITY % msglevel.element "INCLUDE">
<![%msglevel.element;[
<!ELEMENT msglevel (%smallcptr.char.mix;)*>
<!--end of msglevel.element-->]]>

<!ENTITY % msglevel.attlist "INCLUDE">
<![%msglevel.attlist;[
<!ATTLIST msglevel
		%common.attrib;
		%msglevel.role.attrib;
		%local.msglevel.attrib;
>
<!--end of msglevel.attlist-->]]>
<!--end of msglevel.module-->]]>

<!ENTITY % msgorig.module "INCLUDE">
<![%msgorig.module;[
<!ENTITY % local.msgorig.attrib "">
<!ENTITY % msgorig.role.attrib "%role.attrib;">

<!ENTITY % msgorig.element "INCLUDE">
<![%msgorig.element;[
<!ELEMENT msgorig (%smallcptr.char.mix;)*>
<!--end of msgorig.element-->]]>

<!ENTITY % msgorig.attlist "INCLUDE">
<![%msgorig.attlist;[
<!ATTLIST msgorig
		%common.attrib;
		%msgorig.role.attrib;
		%local.msgorig.attrib;
>
<!--end of msgorig.attlist-->]]>
<!--end of msgorig.module-->]]>

<!ENTITY % msgaud.module "INCLUDE">
<![%msgaud.module;[
<!ENTITY % local.msgaud.attrib "">
<!ENTITY % msgaud.role.attrib "%role.attrib;">

<!ENTITY % msgaud.element "INCLUDE">
<![%msgaud.element;[
<!ELEMENT msgaud (%para.char.mix;)*>
<!--end of msgaud.element-->]]>

<!ENTITY % msgaud.attlist "INCLUDE">
<![%msgaud.attlist;[
<!ATTLIST msgaud
		%common.attrib;
		%msgaud.role.attrib;
		%local.msgaud.attrib;
>
<!--end of msgaud.attlist-->]]>
<!--end of msgaud.module-->]]>

<!ENTITY % msgexplan.module "INCLUDE">
<![%msgexplan.module;[
<!ENTITY % local.msgexplan.attrib "">
<!ENTITY % msgexplan.role.attrib "%role.attrib;">

<!ENTITY % msgexplan.element "INCLUDE">
<![%msgexplan.element;[
<!ELEMENT msgexplan (title?, (%component.mix;)+)>
<!--end of msgexplan.element-->]]>

<!ENTITY % msgexplan.attlist "INCLUDE">
<![%msgexplan.attlist;[
<!ATTLIST msgexplan
		%common.attrib;
		%msgexplan.role.attrib;
		%local.msgexplan.attrib;
>
<!--end of msgexplan.attlist-->]]>
<!--end of msgexplan.module-->]]>
<!--end of msgset.content.module-->]]>

<!-- QandASet ........................ -->
<!ENTITY % qandset.content.module "INCLUDE">
<![ %qandset.content.module; [
<!ENTITY % qandset.module "INCLUDE">
<![ %qandset.module; [
<!ENTITY % local.qandset.attrib "">
<!ENTITY % qandset.role.attrib "%role.attrib;">

<!ENTITY % qandset.element "INCLUDE">
<![ %qandset.element; [
<!ELEMENT qandaset ((%formalobject.title.content;)?,
			(%qandaset.mix;)*,
                        (qandadiv+|qandaentry+))>
<!--end of qandset.element-->]]>

<!ENTITY % qandset.attlist "INCLUDE">
<![ %qandset.attlist; [
<!ATTLIST qandaset
		defaultlabel	(qanda|number|none)       #IMPLIED
		%common.attrib;
		%qandset.role.attrib;
		%local.qandset.attrib;>
<!--end of qandset.attlist-->]]>
<!--end of qandset.module-->]]>

<!ENTITY % qandadiv.module "INCLUDE">
<![ %qandadiv.module; [
<!ENTITY % local.qandadiv.attrib "">
<!ENTITY % qandadiv.role.attrib "%role.attrib;">

<!ENTITY % qandadiv.element "INCLUDE">
<![ %qandadiv.element; [
<!ELEMENT qandadiv ((%formalobject.title.content;)?, 
			(%qandaset.mix;)*,
			(qandadiv+|qandaentry+))>
<!--end of qandadiv.element-->]]>

<!ENTITY % qandadiv.attlist "INCLUDE">
<![ %qandadiv.attlist; [
<!ATTLIST qandadiv
		%common.attrib;
		%qandadiv.role.attrib;
		%local.qandadiv.attrib;>
<!--end of qandadiv.attlist-->]]>
<!--end of qandadiv.module-->]]>

<!ENTITY % qandaentry.module "INCLUDE">
<![ %qandaentry.module; [
<!ENTITY % local.qandaentry.attrib "">
<!ENTITY % qandaentry.role.attrib "%role.attrib;">

<!ENTITY % qandaentry.element "INCLUDE">
<![ %qandaentry.element; [
<!ELEMENT qandaentry (revhistory?, question, answer*)>
<!--end of qandaentry.element-->]]>

<!ENTITY % qandaentry.attlist "INCLUDE">
<![ %qandaentry.attlist; [
<!ATTLIST qandaentry
		%common.attrib;
		%qandaentry.role.attrib;
		%local.qandaentry.attrib;>
<!--end of qandaentry.attlist-->]]>
<!--end of qandaentry.module-->]]>

<!ENTITY % question.module "INCLUDE">
<![ %question.module; [
<!ENTITY % local.question.attrib "">
<!ENTITY % question.role.attrib "%role.attrib;">

<!ENTITY % question.element "INCLUDE">
<![ %question.element; [
<!ELEMENT question (label?, (%qandaset.mix;)+)>
<!--end of question.element-->]]>

<!ENTITY % question.attlist "INCLUDE">
<![ %question.attlist; [
<!ATTLIST question
		%common.attrib;
		%question.role.attrib;
		%local.question.attrib;
>
<!--end of question.attlist-->]]>
<!--end of question.module-->]]>

<!ENTITY % answer.module "INCLUDE">
<![ %answer.module; [
<!ENTITY % local.answer.attrib "">
<!ENTITY % answer.role.attrib "%role.attrib;">

<!ENTITY % answer.element "INCLUDE">
<![ %answer.element; [
<!ELEMENT answer (label?, (%qandaset.mix;)*, qandaentry*)>
<!--end of answer.element-->]]>

<!ENTITY % answer.attlist "INCLUDE">
<![ %answer.attlist; [
<!ATTLIST answer
		%common.attrib;
		%answer.role.attrib;
		%local.answer.attrib;
>
<!--end of answer.attlist-->]]>
<!--end of answer.module-->]]>

<!ENTITY % label.module "INCLUDE">
<![ %label.module; [
<!ENTITY % local.label.attrib "">
<!ENTITY % label.role.attrib "%role.attrib;">

<!ENTITY % label.element "INCLUDE">
<![ %label.element; [
<!ELEMENT label (%word.char.mix;)*>
<!--end of label.element-->]]>

<!ENTITY % label.attlist "INCLUDE">
<![ %label.attlist; [
<!ATTLIST label
		%common.attrib;
		%label.role.attrib;
		%local.label.attrib;
>
<!--end of label.attlist-->]]>
<!--end of label.module-->]]>
<!--end of qandset.content.module-->]]>

<!-- Procedure ........................ -->

<!ENTITY % procedure.content.module "INCLUDE">
<![%procedure.content.module;[
<!ENTITY % procedure.module "INCLUDE">
<![%procedure.module;[
<!ENTITY % local.procedure.attrib "">
<!ENTITY % procedure.role.attrib "%role.attrib;">

<!ENTITY % procedure.element "INCLUDE">
<![%procedure.element;[
<!ELEMENT procedure ((%formalobject.title.content;)?,
	(%component.mix;)*, step+)>
<!--end of procedure.element-->]]>

<!ENTITY % procedure.attlist "INCLUDE">
<![%procedure.attlist;[
<!ATTLIST procedure
		%common.attrib;
		%procedure.role.attrib;
		%local.procedure.attrib;
>
<!--end of procedure.attlist-->]]>
<!--end of procedure.module-->]]>

<!ENTITY % step.module "INCLUDE">
<![%step.module;[
<!ENTITY % local.step.attrib "">
<!ENTITY % step.role.attrib "%role.attrib;">

<!ENTITY % step.element "INCLUDE">
<![%step.element;[
<!ELEMENT step (title?, (((%component.mix;)+, (substeps,
		(%component.mix;)*)?) | (substeps, (%component.mix;)*)))>
<!--end of step.element-->]]>

<!-- Performance: Whether the Step must be performed -->
<!-- not #REQUIRED! -->


<!ENTITY % step.attlist "INCLUDE">
<![%step.attlist;[
<!ATTLIST step
		performance	(optional
				|required)	"required"
		%common.attrib;
		%step.role.attrib;
		%local.step.attrib;
>
<!--end of step.attlist-->]]>
<!--end of step.module-->]]>

<!ENTITY % substeps.module "INCLUDE">
<![%substeps.module;[
<!ENTITY % local.substeps.attrib "">
<!ENTITY % substeps.role.attrib "%role.attrib;">

<!ENTITY % substeps.element "INCLUDE">
<![%substeps.element;[
<!ELEMENT substeps (step+)>
<!--end of substeps.element-->]]>

<!-- Performance: whether entire set of substeps must be performed -->
<!-- not #REQUIRED! -->


<!ENTITY % substeps.attlist "INCLUDE">
<![%substeps.attlist;[
<!ATTLIST substeps
		performance	(optional
				|required)	"required"
		%common.attrib;
		%substeps.role.attrib;
		%local.substeps.attrib;
>
<!--end of substeps.attlist-->]]>
<!--end of substeps.module-->]]>
<!--end of procedure.content.module-->]]>

<!-- Sidebar .......................... -->

<!ENTITY % sidebar.content.model "INCLUDE">
<![ %sidebar.content.model; [

<!ENTITY % sidebarinfo.module "INCLUDE">
<![ %sidebarinfo.module; [
<!ENTITY % local.sidebarinfo.attrib "">
<!ENTITY % sidebarinfo.role.attrib "%role.attrib;">

<!ENTITY % sidebarinfo.element "INCLUDE">
<![ %sidebarinfo.element; [
<!ELEMENT sidebarinfo ((graphic | mediaobject | legalnotice | modespec 
	| subjectset | keywordset | itermset | %bibliocomponent.mix;)+)>
<!--end of sidebarinfo.element-->]]>

<!ENTITY % sidebarinfo.attlist "INCLUDE">
<![ %sidebarinfo.attlist; [
<!ATTLIST sidebarinfo
		%common.attrib;
		%sidebarinfo.role.attrib;
		%local.sidebarinfo.attrib;
>
<!--end of sidebarinfo.attlist-->]]>
<!--end of sidebarinfo.module-->]]>

<!ENTITY % sidebar.module "INCLUDE">
<![%sidebar.module;[
<!ENTITY % local.sidebar.attrib "">
<!ENTITY % sidebar.role.attrib "%role.attrib;">

<!ENTITY % sidebar.element "INCLUDE">
<![%sidebar.element;[
<!ELEMENT sidebar (sidebarinfo?, 
                   (%formalobject.title.content;)?,
                   (%sidebar.mix;)+)>
<!--end of sidebar.element-->]]>

<!ENTITY % sidebar.attlist "INCLUDE">
<![%sidebar.attlist;[
<!ATTLIST sidebar
		%common.attrib;
		%sidebar.role.attrib;
		%local.sidebar.attrib;
>
<!--end of sidebar.attlist-->]]>
<!--end of sidebar.module-->]]>
<!--end of sidebar.content.model-->]]>

<!-- ...................................................................... -->
<!-- Paragraph-related elements ........................................... -->

<!ENTITY % abstract.module "INCLUDE">
<![%abstract.module;[
<!ENTITY % local.abstract.attrib "">
<!ENTITY % abstract.role.attrib "%role.attrib;">

<!ENTITY % abstract.element "INCLUDE">
<![%abstract.element;[
<!ELEMENT abstract (title?, (%para.class;)+)>
<!--end of abstract.element-->]]>

<!ENTITY % abstract.attlist "INCLUDE">
<![%abstract.attlist;[
<!ATTLIST abstract
		%common.attrib;
		%abstract.role.attrib;
		%local.abstract.attrib;
>
<!--end of abstract.attlist-->]]>
<!--end of abstract.module-->]]>

<!ENTITY % authorblurb.module "INCLUDE">
<![%authorblurb.module;[
<!ENTITY % local.authorblurb.attrib "">
<!ENTITY % authorblurb.role.attrib "%role.attrib;">

<!ENTITY % authorblurb.element "INCLUDE">
<![%authorblurb.element;[
<!ELEMENT authorblurb (title?, (%para.class;)+)>
<!--end of authorblurb.element-->]]>

<!ENTITY % authorblurb.attlist "INCLUDE">
<![%authorblurb.attlist;[
<!ATTLIST authorblurb
		%common.attrib;
		%authorblurb.role.attrib;
		%local.authorblurb.attrib;
>
<!--end of authorblurb.attlist-->]]>
<!--end of authorblurb.module-->]]>

<!ENTITY % blockquote.module "INCLUDE">
<![%blockquote.module;[

<!ENTITY % local.blockquote.attrib "">
<!ENTITY % blockquote.role.attrib "%role.attrib;">

<!ENTITY % blockquote.element "INCLUDE">
<![%blockquote.element;[
<!ELEMENT blockquote (title?, attribution?, (%component.mix;)+)>
<!--end of blockquote.element-->]]>

<!ENTITY % blockquote.attlist "INCLUDE">
<![%blockquote.attlist;[
<!ATTLIST blockquote
		%common.attrib;
		%blockquote.role.attrib;
		%local.blockquote.attrib;
>
<!--end of blockquote.attlist-->]]>
<!--end of blockquote.module-->]]>

<!ENTITY % attribution.module "INCLUDE">
<![%attribution.module;[
<!ENTITY % local.attribution.attrib "">
<!ENTITY % attribution.role.attrib "%role.attrib;">

<!ENTITY % attribution.element "INCLUDE">
<![%attribution.element;[
<!ELEMENT attribution (%para.char.mix;)*>
<!--end of attribution.element-->]]>

<!ENTITY % attribution.attlist "INCLUDE">
<![%attribution.attlist;[
<!ATTLIST attribution
		%common.attrib;
		%attribution.role.attrib;
		%local.attribution.attrib;
>
<!--end of attribution.attlist-->]]>
<!--end of attribution.module-->]]>

<!ENTITY % bridgehead.module "INCLUDE">
<![%bridgehead.module;[
<!ENTITY % local.bridgehead.attrib "">
<!ENTITY % bridgehead.role.attrib "%role.attrib;">

<!ENTITY % bridgehead.element "INCLUDE">
<![%bridgehead.element;[
<!ELEMENT bridgehead (%title.char.mix;)*>
<!--end of bridgehead.element-->]]>

<!-- Renderas: Indicates the format in which the BridgeHead
		should appear -->


<!ENTITY % bridgehead.attlist "INCLUDE">
<![%bridgehead.attlist;[
<!ATTLIST bridgehead
		renderas	(other
				|sect1
				|sect2
				|sect3
				|sect4
				|sect5)		#IMPLIED
		%common.attrib;
		%bridgehead.role.attrib;
		%local.bridgehead.attrib;
>
<!--end of bridgehead.attlist-->]]>
<!--end of bridgehead.module-->]]>

<!ENTITY % remark.module "INCLUDE">
<![%remark.module;[
<!ENTITY % local.remark.attrib "">
<!ENTITY % remark.role.attrib "%role.attrib;">

<!ENTITY % remark.element "INCLUDE">
<![%remark.element;[
<!ELEMENT remark (%para.char.mix;)*>
<!--end of remark.element-->]]>

<!ENTITY % remark.attlist "INCLUDE">
<![%remark.attlist;[
<!ATTLIST remark
		%common.attrib;
		%remark.role.attrib;
		%local.remark.attrib;
>
<!--end of remark.attlist-->]]>
<!--end of remark.module-->]]>

<!ENTITY % epigraph.module "INCLUDE">
<![%epigraph.module;[
<!ENTITY % local.epigraph.attrib "">
<!ENTITY % epigraph.role.attrib "%role.attrib;">

<!ENTITY % epigraph.element "INCLUDE">
<![%epigraph.element;[
<!ELEMENT epigraph (attribution?, (%para.class;)+)>
<!--end of epigraph.element-->]]>

<!ENTITY % epigraph.attlist "INCLUDE">
<![%epigraph.attlist;[
<!ATTLIST epigraph
		%common.attrib;
		%epigraph.role.attrib;
		%local.epigraph.attrib;
>
<!--end of epigraph.attlist-->]]>
<!-- Attribution (defined above)-->
<!--end of epigraph.module-->]]>

<!ENTITY % footnote.module "INCLUDE">
<![%footnote.module;[
<!ENTITY % local.footnote.attrib "">
<!ENTITY % footnote.role.attrib "%role.attrib;">

<!ENTITY % footnote.element "INCLUDE">
<![%footnote.element;[
<!ELEMENT footnote ((%footnote.mix;)+)>
<!--end of footnote.element-->]]>

<!ENTITY % footnote.attlist "INCLUDE">
<![%footnote.attlist;[
<!ATTLIST footnote
		%label.attrib;
		%common.attrib;
		%footnote.role.attrib;
		%local.footnote.attrib;
>
<!--end of footnote.attlist-->]]>
<!--end of footnote.module-->]]>

<!ENTITY % highlights.module "INCLUDE">
<![%highlights.module;[
<!ENTITY % local.highlights.attrib "">
<!ENTITY % highlights.role.attrib "%role.attrib;">

<!ENTITY % highlights.element "INCLUDE">
<![%highlights.element;[
<!ELEMENT highlights ((%highlights.mix;)+)>
<!--end of highlights.element-->]]>

<!ENTITY % highlights.attlist "INCLUDE">
<![%highlights.attlist;[
<!ATTLIST highlights
		%common.attrib;
		%highlights.role.attrib;
		%local.highlights.attrib;
>
<!--end of highlights.attlist-->]]>
<!--end of highlights.module-->]]>

<!ENTITY % formalpara.module "INCLUDE">
<![%formalpara.module;[
<!ENTITY % local.formalpara.attrib "">
<!ENTITY % formalpara.role.attrib "%role.attrib;">

<!ENTITY % formalpara.element "INCLUDE">
<![%formalpara.element;[
<!ELEMENT formalpara (title, (%ndxterm.class;)*, para)>
<!--end of formalpara.element-->]]>

<!ENTITY % formalpara.attlist "INCLUDE">
<![%formalpara.attlist;[
<!ATTLIST formalpara
		%common.attrib;
		%formalpara.role.attrib;
		%local.formalpara.attrib;
>
<!--end of formalpara.attlist-->]]>
<!--end of formalpara.module-->]]>

<!ENTITY % para.module "INCLUDE">
<![%para.module;[
<!ENTITY % local.para.attrib "">
<!ENTITY % para.role.attrib "%role.attrib;">

<!ENTITY % para.element "INCLUDE">
<![%para.element;[
<!ELEMENT para (%para.char.mix; | %para.mix;)*>
<!--end of para.element-->]]>

<!ENTITY % para.attlist "INCLUDE">
<![%para.attlist;[
<!ATTLIST para
		%common.attrib;
		%para.role.attrib;
		%local.para.attrib;
>
<!--end of para.attlist-->]]>
<!--end of para.module-->]]>

<!ENTITY % simpara.module "INCLUDE">
<![%simpara.module;[
<!ENTITY % local.simpara.attrib "">
<!ENTITY % simpara.role.attrib "%role.attrib;">

<!ENTITY % simpara.element "INCLUDE">
<![%simpara.element;[
<!ELEMENT simpara (%para.char.mix;)*>
<!--end of simpara.element-->]]>

<!ENTITY % simpara.attlist "INCLUDE">
<![%simpara.attlist;[
<!ATTLIST simpara
		%common.attrib;
		%simpara.role.attrib;
		%local.simpara.attrib;
>
<!--end of simpara.attlist-->]]>
<!--end of simpara.module-->]]>

<!ENTITY % admon.module "INCLUDE">
<![%admon.module;[
<!ENTITY % local.admon.attrib "">
<!ENTITY % admon.role.attrib "%role.attrib;">


<!ENTITY % caution.element "INCLUDE">
<![%caution.element;[
<!ELEMENT caution (title?, (%admon.mix;)+)>
<!--end of caution.element-->]]>

<!ENTITY % caution.attlist "INCLUDE">
<![%caution.attlist;[
<!ATTLIST caution
		%common.attrib;
		%admon.role.attrib;
		%local.admon.attrib;
>
<!--end of caution.attlist-->]]>


<!ENTITY % important.element "INCLUDE">
<![%important.element;[
<!ELEMENT important (title?, (%admon.mix;)+)>
<!--end of important.element-->]]>

<!ENTITY % important.attlist "INCLUDE">
<![%important.attlist;[
<!ATTLIST important
		%common.attrib;
		%admon.role.attrib;
		%local.admon.attrib;
>
<!--end of important.attlist-->]]>


<!ENTITY % note.element "INCLUDE">
<![%note.element;[
<!ELEMENT note (title?, (%admon.mix;)+)>
<!--end of note.element-->]]>

<!ENTITY % note.attlist "INCLUDE">
<![%note.attlist;[
<!ATTLIST note
		%common.attrib;
		%admon.role.attrib;
		%local.admon.attrib;
>
<!--end of note.attlist-->]]>


<!ENTITY % tip.element "INCLUDE">
<![%tip.element;[
<!ELEMENT tip (title?, (%admon.mix;)+)>
<!--end of tip.element-->]]>

<!ENTITY % tip.attlist "INCLUDE">
<![%tip.attlist;[
<!ATTLIST tip
		%common.attrib;
		%admon.role.attrib;
		%local.admon.attrib;
>
<!--end of tip.attlist-->]]>


<!ENTITY % warning.element "INCLUDE">
<![%warning.element;[
<!ELEMENT warning (title?, (%admon.mix;)+)>
<!--end of warning.element-->]]>

<!ENTITY % warning.attlist "INCLUDE">
<![%warning.attlist;[
<!ATTLIST warning
		%common.attrib;
		%admon.role.attrib;
		%local.admon.attrib;
>
<!--end of warning.attlist-->]]>

<!--end of admon.module-->]]>

<!-- ...................................................................... -->
<!-- Lists ................................................................ -->

<!-- GlossList ........................ -->

<!ENTITY % glosslist.module "INCLUDE">
<![%glosslist.module;[
<!ENTITY % local.glosslist.attrib "">
<!ENTITY % glosslist.role.attrib "%role.attrib;">

<!ENTITY % glosslist.element "INCLUDE">
<![%glosslist.element;[
<!ELEMENT glosslist (glossentry+)>
<!--end of glosslist.element-->]]>

<!ENTITY % glosslist.attlist "INCLUDE">
<![%glosslist.attlist;[
<!ATTLIST glosslist
		%common.attrib;
		%glosslist.role.attrib;
		%local.glosslist.attrib;
>
<!--end of glosslist.attlist-->]]>
<!--end of glosslist.module-->]]>

<!ENTITY % glossentry.content.module "INCLUDE">
<![%glossentry.content.module;[
<!ENTITY % glossentry.module "INCLUDE">
<![%glossentry.module;[
<!ENTITY % local.glossentry.attrib "">
<!ENTITY % glossentry.role.attrib "%role.attrib;">

<!ENTITY % glossentry.element "INCLUDE">
<![%glossentry.element;[
<!ELEMENT glossentry (glossterm, acronym?, abbrev?,
                      (%ndxterm.class;)*,
                      revhistory?, (glosssee|glossdef+))>
<!--end of glossentry.element-->]]>

<!-- SortAs: String by which the GlossEntry is to be sorted
		(alphabetized) in lieu of its proper content -->


<!ENTITY % glossentry.attlist "INCLUDE">
<![%glossentry.attlist;[
<!ATTLIST glossentry
		sortas		CDATA		#IMPLIED
		%common.attrib;
		%glossentry.role.attrib;
		%local.glossentry.attrib;
>
<!--end of glossentry.attlist-->]]>
<!--end of glossentry.module-->]]>

<!-- GlossTerm (defined in the Inlines section, below)-->
<!ENTITY % glossdef.module "INCLUDE">
<![%glossdef.module;[
<!ENTITY % local.glossdef.attrib "">
<!ENTITY % glossdef.role.attrib "%role.attrib;">

<!ENTITY % glossdef.element "INCLUDE">
<![%glossdef.element;[
<!ELEMENT glossdef ((%glossdef.mix;)+, glossseealso*)>
<!--end of glossdef.element-->]]>

<!-- Subject: List of subjects; keywords for the definition -->


<!ENTITY % glossdef.attlist "INCLUDE">
<![%glossdef.attlist;[
<!ATTLIST glossdef
		subject		CDATA		#IMPLIED
		%common.attrib;
		%glossdef.role.attrib;
		%local.glossdef.attrib;
>
<!--end of glossdef.attlist-->]]>
<!--end of glossdef.module-->]]>

<!ENTITY % glosssee.module "INCLUDE">
<![%glosssee.module;[
<!ENTITY % local.glosssee.attrib "">
<!ENTITY % glosssee.role.attrib "%role.attrib;">

<!ENTITY % glosssee.element "INCLUDE">
<![%glosssee.element;[
<!ELEMENT glosssee (%para.char.mix;)*>
<!--end of glosssee.element-->]]>

<!-- OtherTerm: Reference to the GlossEntry whose GlossTerm
		should be displayed at the point of the GlossSee -->


<!ENTITY % glosssee.attlist "INCLUDE">
<![%glosssee.attlist;[
<!ATTLIST glosssee
		otherterm	IDREF		#IMPLIED
		%common.attrib;
		%glosssee.role.attrib;
		%local.glosssee.attrib;
>
<!--end of glosssee.attlist-->]]>
<!--end of glosssee.module-->]]>

<!ENTITY % glossseealso.module "INCLUDE">
<![%glossseealso.module;[
<!ENTITY % local.glossseealso.attrib "">
<!ENTITY % glossseealso.role.attrib "%role.attrib;">

<!ENTITY % glossseealso.element "INCLUDE">
<![%glossseealso.element;[
<!ELEMENT glossseealso (%para.char.mix;)*>
<!--end of glossseealso.element-->]]>

<!-- OtherTerm: Reference to the GlossEntry whose GlossTerm
		should be displayed at the point of the GlossSeeAlso -->


<!ENTITY % glossseealso.attlist "INCLUDE">
<![%glossseealso.attlist;[
<!ATTLIST glossseealso
		otherterm	IDREF		#IMPLIED
		%common.attrib;
		%glossseealso.role.attrib;
		%local.glossseealso.attrib;
>
<!--end of glossseealso.attlist-->]]>
<!--end of glossseealso.module-->]]>
<!--end of glossentry.content.module-->]]>

<!-- ItemizedList and OrderedList ..... -->

<!ENTITY % itemizedlist.module "INCLUDE">
<![%itemizedlist.module;[
<!ENTITY % local.itemizedlist.attrib "">
<!ENTITY % itemizedlist.role.attrib "%role.attrib;">

<!ENTITY % itemizedlist.element "INCLUDE">
<![%itemizedlist.element;[
<!ELEMENT itemizedlist ((%formalobject.title.content;)?, listitem+)>
<!--end of itemizedlist.element-->]]>

<!-- Spacing: Whether the vertical space in the list should be
		compressed -->
<!-- Mark: Keyword, e.g., bullet, dash, checkbox, none;
		list of keywords and defaults are implementation specific -->


<!ENTITY % itemizedlist.attlist "INCLUDE">
<![%itemizedlist.attlist;[
<!ATTLIST itemizedlist		spacing		(normal
				|compact)	#IMPLIED
		%mark.attrib;
		%common.attrib;
		%itemizedlist.role.attrib;
		%local.itemizedlist.attrib;
>
<!--end of itemizedlist.attlist-->]]>
<!--end of itemizedlist.module-->]]>

<!ENTITY % orderedlist.module "INCLUDE">
<![%orderedlist.module;[
<!ENTITY % local.orderedlist.attrib "">
<!ENTITY % orderedlist.role.attrib "%role.attrib;">

<!ENTITY % orderedlist.element "INCLUDE">
<![%orderedlist.element;[
<!ELEMENT orderedlist ((%formalobject.title.content;)?, listitem+)>
<!--end of orderedlist.element-->]]>

<!-- Numeration: Style of ListItem numbered; default is expected
		to be Arabic -->
<!-- InheritNum: Specifies for a nested list that the numbering
		of ListItems should include the number of the item
		within which they are nested (e.g., 1a and 1b within 1,
		rather than a and b) -->
<!-- Continuation: Where list numbering begins afresh (Restarts,
		the default) or continues that of the immediately preceding 
		list (Continues) -->
<!-- Spacing: Whether the vertical space in the list should be
		compressed -->


<!ENTITY % orderedlist.attlist "INCLUDE">
<![%orderedlist.attlist;[
<!ATTLIST orderedlist
		numeration	(arabic
				|upperalpha
				|loweralpha
				|upperroman
				|lowerroman)	#IMPLIED
		inheritnum	(inherit
				|ignore)	"ignore"
		continuation	(continues
				|restarts)	"restarts"
		spacing		(normal
				|compact)	#IMPLIED
		%common.attrib;
		%orderedlist.role.attrib;
		%local.orderedlist.attrib;
>
<!--end of orderedlist.attlist-->]]>
<!--end of orderedlist.module-->]]>

<!ENTITY % listitem.module "INCLUDE">
<![%listitem.module;[
<!ENTITY % local.listitem.attrib "">
<!ENTITY % listitem.role.attrib "%role.attrib;">

<!ENTITY % listitem.element "INCLUDE">
<![%listitem.element;[
<!ELEMENT listitem ((%component.mix;)+)>
<!--end of listitem.element-->]]>

<!-- Override: Indicates the mark to be used for this ListItem
		instead of the default mark or the mark specified by
		the Mark attribute on the enclosing ItemizedList -->


<!ENTITY % listitem.attlist "INCLUDE">
<![%listitem.attlist;[
<!ATTLIST listitem
		override	CDATA		#IMPLIED
		%common.attrib;
		%listitem.role.attrib;
		%local.listitem.attrib;
>
<!--end of listitem.attlist-->]]>
<!--end of listitem.module-->]]>

<!-- SegmentedList .................... -->
<!ENTITY % segmentedlist.content.module "INCLUDE">
<![%segmentedlist.content.module;[
<!ENTITY % segmentedlist.module "INCLUDE">
<![%segmentedlist.module;[
<!ENTITY % local.segmentedlist.attrib "">
<!ENTITY % segmentedlist.role.attrib "%role.attrib;">

<!ENTITY % segmentedlist.element "INCLUDE">
<![%segmentedlist.element;[
<!ELEMENT segmentedlist ((%formalobject.title.content;)?,
                         segtitle, segtitle+,
                         seglistitem+)>
<!--end of segmentedlist.element-->]]>

<!ENTITY % segmentedlist.attlist "INCLUDE">
<![%segmentedlist.attlist;[
<!ATTLIST segmentedlist
		%common.attrib;
		%segmentedlist.role.attrib;
		%local.segmentedlist.attrib;
>
<!--end of segmentedlist.attlist-->]]>
<!--end of segmentedlist.module-->]]>

<!ENTITY % segtitle.module "INCLUDE">
<![%segtitle.module;[
<!ENTITY % local.segtitle.attrib "">
<!ENTITY % segtitle.role.attrib "%role.attrib;">

<!ENTITY % segtitle.element "INCLUDE">
<![%segtitle.element;[
<!ELEMENT segtitle (%title.char.mix;)*>
<!--end of segtitle.element-->]]>

<!ENTITY % segtitle.attlist "INCLUDE">
<![%segtitle.attlist;[
<!ATTLIST segtitle
		%common.attrib;
		%segtitle.role.attrib;
		%local.segtitle.attrib;
>
<!--end of segtitle.attlist-->]]>
<!--end of segtitle.module-->]]>

<!ENTITY % seglistitem.module "INCLUDE">
<![%seglistitem.module;[
<!ENTITY % local.seglistitem.attrib "">
<!ENTITY % seglistitem.role.attrib "%role.attrib;">

<!ENTITY % seglistitem.element "INCLUDE">
<![%seglistitem.element;[
<!ELEMENT seglistitem (seg, seg+)>
<!--end of seglistitem.element-->]]>

<!ENTITY % seglistitem.attlist "INCLUDE">
<![%seglistitem.attlist;[
<!ATTLIST seglistitem
		%common.attrib;
		%seglistitem.role.attrib;
		%local.seglistitem.attrib;
>
<!--end of seglistitem.attlist-->]]>
<!--end of seglistitem.module-->]]>

<!ENTITY % seg.module "INCLUDE">
<![%seg.module;[
<!ENTITY % local.seg.attrib "">
<!ENTITY % seg.role.attrib "%role.attrib;">

<!ENTITY % seg.element "INCLUDE">
<![%seg.element;[
<!ELEMENT seg (%para.char.mix;)*>
<!--end of seg.element-->]]>

<!ENTITY % seg.attlist "INCLUDE">
<![%seg.attlist;[
<!ATTLIST seg
		%common.attrib;
		%seg.role.attrib;
		%local.seg.attrib;
>
<!--end of seg.attlist-->]]>
<!--end of seg.module-->]]>
<!--end of segmentedlist.content.module-->]]>

<!-- SimpleList ....................... -->

<!ENTITY % simplelist.content.module "INCLUDE">
<![%simplelist.content.module;[
<!ENTITY % simplelist.module "INCLUDE">
<![%simplelist.module;[
<!ENTITY % local.simplelist.attrib "">
<!ENTITY % simplelist.role.attrib "%role.attrib;">

<!ENTITY % simplelist.element "INCLUDE">
<![%simplelist.element;[
<!ELEMENT simplelist (member+)>
<!--end of simplelist.element-->]]>

<!-- Columns: The number of columns the array should contain -->
<!-- Type: How the Members of the SimpleList should be
		formatted: Inline (members separated with commas etc.
		inline), Vert (top to bottom in n Columns), or Horiz (in
		the direction of text flow) in n Columns.  If Column
		is 1 or implied, Type=Vert and Type=Horiz give the same
		results. -->


<!ENTITY % simplelist.attlist "INCLUDE">
<![%simplelist.attlist;[
<!ATTLIST simplelist
		columns		CDATA		#IMPLIED
		type		(inline
				|vert
				|horiz)		"vert"
		%common.attrib;
		%simplelist.role.attrib;
		%local.simplelist.attrib;
>
<!--end of simplelist.attlist-->]]>
<!--end of simplelist.module-->]]>

<!ENTITY % member.module "INCLUDE">
<![%member.module;[
<!ENTITY % local.member.attrib "">
<!ENTITY % member.role.attrib "%role.attrib;">

<!ENTITY % member.element "INCLUDE">
<![%member.element;[
<!ELEMENT member (%para.char.mix;)*>
<!--end of member.element-->]]>

<!ENTITY % member.attlist "INCLUDE">
<![%member.attlist;[
<!ATTLIST member
		%common.attrib;
		%member.role.attrib;
		%local.member.attrib;
>
<!--end of member.attlist-->]]>
<!--end of member.module-->]]>
<!--end of simplelist.content.module-->]]>

<!-- VariableList ..................... -->

<!ENTITY % variablelist.content.module "INCLUDE">
<![%variablelist.content.module;[
<!ENTITY % variablelist.module "INCLUDE">
<![%variablelist.module;[
<!ENTITY % local.variablelist.attrib "">
<!ENTITY % variablelist.role.attrib "%role.attrib;">

<!ENTITY % variablelist.element "INCLUDE">
<![%variablelist.element;[
<!ELEMENT variablelist ((%formalobject.title.content;)?, varlistentry+)>
<!--end of variablelist.element-->]]>

<!-- TermLength: Length beyond which the presentation engine
		may consider the Term too long and select an alternate
		presentation of the Term and, or, its associated ListItem. -->


<!ENTITY % variablelist.attlist "INCLUDE">
<![%variablelist.attlist;[
<!ATTLIST variablelist
		termlength	CDATA		#IMPLIED
		%common.attrib;
		%variablelist.role.attrib;
		%local.variablelist.attrib;
>
<!--end of variablelist.attlist-->]]>
<!--end of variablelist.module-->]]>

<!ENTITY % varlistentry.module "INCLUDE">
<![%varlistentry.module;[
<!ENTITY % local.varlistentry.attrib "">
<!ENTITY % varlistentry.role.attrib "%role.attrib;">

<!ENTITY % varlistentry.element "INCLUDE">
<![%varlistentry.element;[
<!ELEMENT varlistentry (term+, listitem)>
<!--end of varlistentry.element-->]]>

<!ENTITY % varlistentry.attlist "INCLUDE">
<![%varlistentry.attlist;[
<!ATTLIST varlistentry
		%common.attrib;
		%varlistentry.role.attrib;
		%local.varlistentry.attrib;
>
<!--end of varlistentry.attlist-->]]>
<!--end of varlistentry.module-->]]>

<!ENTITY % term.module "INCLUDE">
<![%term.module;[
<!ENTITY % local.term.attrib "">
<!ENTITY % term.role.attrib "%role.attrib;">

<!ENTITY % term.element "INCLUDE">
<![%term.element;[
<!ELEMENT term (%para.char.mix;)*>
<!--end of term.element-->]]>

<!ENTITY % term.attlist "INCLUDE">
<![%term.attlist;[
<!ATTLIST term
		%common.attrib;
		%term.role.attrib;
		%local.term.attrib;
>
<!--end of term.attlist-->]]>
<!--end of term.module-->]]>

<!-- ListItem (defined above)-->
<!--end of variablelist.content.module-->]]>

<!-- CalloutList ...................... -->

<!ENTITY % calloutlist.content.module "INCLUDE">
<![%calloutlist.content.module;[
<!ENTITY % calloutlist.module "INCLUDE">
<![%calloutlist.module;[
<!ENTITY % local.calloutlist.attrib "">
<!ENTITY % calloutlist.role.attrib "%role.attrib;">

<!ENTITY % calloutlist.element "INCLUDE">
<![%calloutlist.element;[
<!ELEMENT calloutlist ((%formalobject.title.content;)?, callout+)>
<!--end of calloutlist.element-->]]>

<!ENTITY % calloutlist.attlist "INCLUDE">
<![%calloutlist.attlist;[
<!ATTLIST calloutlist
		%common.attrib;
		%calloutlist.role.attrib;
		%local.calloutlist.attrib;
>
<!--end of calloutlist.attlist-->]]>
<!--end of calloutlist.module-->]]>

<!ENTITY % callout.module "INCLUDE">
<![%callout.module;[
<!ENTITY % local.callout.attrib "">
<!ENTITY % callout.role.attrib "%role.attrib;">

<!ENTITY % callout.element "INCLUDE">
<![%callout.element;[
<!ELEMENT callout ((%component.mix;)+)>
<!--end of callout.element-->]]>

<!-- AreaRefs: IDs of one or more Areas or AreaSets described
		by this Callout -->


<!ENTITY % callout.attlist "INCLUDE">
<![%callout.attlist;[
<!ATTLIST callout
		arearefs	IDREFS		#REQUIRED
		%common.attrib;
		%callout.role.attrib;
		%local.callout.attrib;
>
<!--end of callout.attlist-->]]>
<!--end of callout.module-->]]>
<!--end of calloutlist.content.module-->]]>

<!-- ...................................................................... -->
<!-- Objects .............................................................. -->

<!-- Examples etc. .................... -->

<!ENTITY % example.module "INCLUDE">
<![%example.module;[
<!ENTITY % local.example.attrib "">
<!ENTITY % example.role.attrib "%role.attrib;">

<!ENTITY % example.element "INCLUDE">
<![%example.element;[
<!ELEMENT example ((%formalobject.title.content;), (%example.mix;)+)>
<!--end of example.element-->]]>

<!ENTITY % example.attlist "INCLUDE">
<![%example.attlist;[
<!ATTLIST example
		%label.attrib;
		%width.attrib;
		%common.attrib;
		%example.role.attrib;
		%local.example.attrib;
>
<!--end of example.attlist-->]]>
<!--end of example.module-->]]>

<!ENTITY % informalexample.module "INCLUDE">
<![%informalexample.module;[
<!ENTITY % local.informalexample.attrib "">
<!ENTITY % informalexample.role.attrib "%role.attrib;">

<!ENTITY % informalexample.element "INCLUDE">
<![%informalexample.element;[
<!ELEMENT informalexample ((%example.mix;)+)>
<!--end of informalexample.element-->]]>

<!ENTITY % informalexample.attlist "INCLUDE">
<![%informalexample.attlist;[
<!ATTLIST informalexample
		%width.attrib;
		%common.attrib;
		%informalexample.role.attrib;
		%local.informalexample.attrib;
>
<!--end of informalexample.attlist-->]]>
<!--end of informalexample.module-->]]>

<!ENTITY % programlistingco.module "INCLUDE">
<![%programlistingco.module;[
<!ENTITY % local.programlistingco.attrib "">
<!ENTITY % programlistingco.role.attrib "%role.attrib;">

<!ENTITY % programlistingco.element "INCLUDE">
<![%programlistingco.element;[
<!ELEMENT programlistingco (areaspec, programlisting, calloutlist*)>
<!--end of programlistingco.element-->]]>

<!ENTITY % programlistingco.attlist "INCLUDE">
<![%programlistingco.attlist;[
<!ATTLIST programlistingco
		%common.attrib;
		%programlistingco.role.attrib;
		%local.programlistingco.attrib;
>
<!--end of programlistingco.attlist-->]]>
<!-- CalloutList (defined above in Lists)-->
<!--end of informalexample.module-->]]>

<!ENTITY % areaspec.content.module "INCLUDE">
<![%areaspec.content.module;[
<!ENTITY % areaspec.module "INCLUDE">
<![%areaspec.module;[
<!ENTITY % local.areaspec.attrib "">
<!ENTITY % areaspec.role.attrib "%role.attrib;">

<!ENTITY % areaspec.element "INCLUDE">
<![%areaspec.element;[
<!ELEMENT areaspec ((area|areaset)+)>
<!--end of areaspec.element-->]]>

<!-- Units: global unit of measure in which coordinates in
		this spec are expressed:

		- CALSPair "x1,y1 x2,y2": lower-left and upper-right 
		coordinates in a rectangle describing repro area in which 
		graphic is placed, where X and Y dimensions are each some 
		number 0..10000 (taken from CALS graphic attributes)

		- LineColumn "line column": line number and column number
		at which to start callout text in "linespecific" content

		- LineRange "startline endline": whole lines from startline
		to endline in "linespecific" content

		- LineColumnPair "line1 col1 line2 col2": starting and ending
		points of area in "linespecific" content that starts at
		first position and ends at second position (including the
		beginnings of any intervening lines)

		- Other: directive to look at value of OtherUnits attribute
		to get implementation-specific keyword

		The default is implementation-specific; usually dependent on 
		the parent element (GraphicCO gets CALSPair, ProgramListingCO
		and ScreenCO get LineColumn) -->
<!-- OtherUnits: User-defined units -->


<!ENTITY % areaspec.attlist "INCLUDE">
<![%areaspec.attlist;[
<!ATTLIST areaspec
		units		(calspair
				|linecolumn
				|linerange
				|linecolumnpair
				|other)		#IMPLIED
		otherunits	NMTOKEN		#IMPLIED
		%common.attrib;
		%areaspec.role.attrib;
		%local.areaspec.attrib;
>
<!--end of areaspec.attlist-->]]>
<!--end of areaspec.module-->]]>

<!ENTITY % area.module "INCLUDE">
<![%area.module;[
<!ENTITY % local.area.attrib "">
<!ENTITY % area.role.attrib "%role.attrib;">

<!ENTITY % area.element "INCLUDE">
<![%area.element;[
<!ELEMENT area EMPTY>
<!--end of area.element-->]]>

<!-- bug number/symbol override or initialization -->
<!-- to any related information -->
<!-- Units: unit of measure in which coordinates in this
		area are expressed; inherits from AreaSet and AreaSpec -->
<!-- OtherUnits: User-defined units -->


<!ENTITY % area.attlist "INCLUDE">
<![%area.attlist;[
<!ATTLIST area
		%label.attrib;		
		%linkends.attrib;
		units		(calspair
				|linecolumn
				|linerange
				|linecolumnpair
				|other)		#IMPLIED
		otherunits	NMTOKEN		#IMPLIED
		coords		CDATA		#REQUIRED
		%idreq.common.attrib;
		%area.role.attrib;
		%local.area.attrib;
>
<!--end of area.attlist-->]]>
<!--end of area.module-->]]>

<!ENTITY % areaset.module "INCLUDE">
<![%areaset.module;[
<!ENTITY % local.areaset.attrib "">
<!ENTITY % areaset.role.attrib "%role.attrib;">

<!ENTITY % areaset.element "INCLUDE">
<![%areaset.element;[
<!ELEMENT areaset (area+)>
<!--end of areaset.element-->]]>

<!-- bug number/symbol override or initialization -->
<!-- Units: unit of measure in which coordinates in this
		area are expressed; inherits from AreaSpec -->


<!ENTITY % areaset.attlist "INCLUDE">
<![%areaset.attlist;[
<!ATTLIST areaset
		%label.attrib;
		units		(calspair
				|linecolumn
				|linerange
				|linecolumnpair
				|other)		#IMPLIED
		otherunits	NMTOKEN		#IMPLIED
		coords		CDATA		#REQUIRED
		%idreq.common.attrib;
		%areaset.role.attrib;
		%local.areaset.attrib;
>
<!--end of areaset.attlist-->]]>
<!--end of areaset.module-->]]>
<!--end of areaspec.content.module-->]]>

<!ENTITY % programlisting.module "INCLUDE">
<![%programlisting.module;[
<!ENTITY % local.programlisting.attrib "">
<!ENTITY % programlisting.role.attrib "%role.attrib;">

<!ENTITY % programlisting.element "INCLUDE">
<![%programlisting.element;[
<!ELEMENT programlisting (%para.char.mix; | co | lineannotation)*>
<!--end of programlisting.element-->]]>

<!ENTITY % programlisting.attlist "INCLUDE">
<![%programlisting.attlist;[
<!ATTLIST programlisting
		%width.attrib;
		%linespecific.attrib;
		%common.attrib;
		%programlisting.role.attrib;
		%local.programlisting.attrib;
>
<!--end of programlisting.attlist-->]]>
<!--end of programlisting.module-->]]>

<!ENTITY % literallayout.module "INCLUDE">
<![%literallayout.module;[
<!ENTITY % local.literallayout.attrib "">
<!ENTITY % literallayout.role.attrib "%role.attrib;">

<!ENTITY % literallayout.element "INCLUDE">
<![%literallayout.element;[
<!ELEMENT literallayout (%para.char.mix; | lineannotation | co)*>
<!--end of literallayout.element-->]]>

<!ENTITY % literallayout.attlist "INCLUDE">
<![%literallayout.attlist;[
<!ATTLIST literallayout
		%width.attrib;
		%linespecific.attrib;
		class	(monospaced|normal)	"normal"
		%common.attrib;
		%literallayout.role.attrib;
		%local.literallayout.attrib;
>
<!--end of literallayout.attlist-->]]>
<!-- LineAnnotation (defined in the Inlines section, below)-->
<!--end of literallayout.module-->]]>

<!ENTITY % screenco.module "INCLUDE">
<![%screenco.module;[
<!ENTITY % local.screenco.attrib "">
<!ENTITY % screenco.role.attrib "%role.attrib;">

<!ENTITY % screenco.element "INCLUDE">
<![%screenco.element;[
<!ELEMENT screenco (areaspec, screen, calloutlist*)>
<!--end of screenco.element-->]]>

<!ENTITY % screenco.attlist "INCLUDE">
<![%screenco.attlist;[
<!ATTLIST screenco
		%common.attrib;
		%screenco.role.attrib;
		%local.screenco.attrib;
>
<!--end of screenco.attlist-->]]>
<!-- AreaSpec (defined above)-->
<!-- CalloutList (defined above in Lists)-->
<!--end of screenco.module-->]]>

<!ENTITY % screen.module "INCLUDE">
<![%screen.module;[
<!ENTITY % local.screen.attrib "">
<!ENTITY % screen.role.attrib "%role.attrib;">

<!ENTITY % screen.element "INCLUDE">
<![%screen.element;[
<!ELEMENT screen (%para.char.mix; | co | lineannotation)*>
<!--end of screen.element-->]]>

<!ENTITY % screen.attlist "INCLUDE">
<![%screen.attlist;[
<!ATTLIST screen
		%width.attrib;
		%linespecific.attrib;
		%common.attrib;
		%screen.role.attrib;
		%local.screen.attrib;
>
<!--end of screen.attlist-->]]>
<!--end of screen.module-->]]>

<!ENTITY % screenshot.content.module "INCLUDE">
<![%screenshot.content.module;[
<!ENTITY % screenshot.module "INCLUDE">
<![%screenshot.module;[
<!ENTITY % local.screenshot.attrib "">
<!ENTITY % screenshot.role.attrib "%role.attrib;">

<!ENTITY % screenshot.element "INCLUDE">
<![%screenshot.element;[
<!ELEMENT screenshot (screeninfo?,
                      (graphic|graphicco
                      |mediaobject|mediaobjectco))>
<!--end of screenshot.element-->]]>

<!ENTITY % screenshot.attlist "INCLUDE">
<![%screenshot.attlist;[
<!ATTLIST screenshot
		%common.attrib;
		%screenshot.role.attrib;
		%local.screenshot.attrib;
>
<!--end of screenshot.attlist-->]]>
<!--end of screenshot.module-->]]>

<!ENTITY % screeninfo.module "INCLUDE">
<![%screeninfo.module;[
<!ENTITY % local.screeninfo.attrib "">
<!ENTITY % screeninfo.role.attrib "%role.attrib;">

<!ENTITY % screeninfo.element "INCLUDE">
<![%screeninfo.element;[
<!ELEMENT screeninfo (%para.char.mix;)*>
<!--end of screeninfo.element-->]]>

<!ENTITY % screeninfo.attlist "INCLUDE">
<![%screeninfo.attlist;[
<!ATTLIST screeninfo
		%common.attrib;
		%screeninfo.role.attrib;
		%local.screeninfo.attrib;
>
<!--end of screeninfo.attlist-->]]>
<!--end of screeninfo.module-->]]>
<!--end of screenshot.content.module-->]]>

<!-- Figures etc. ..................... -->

<!ENTITY % figure.module "INCLUDE">
<![%figure.module;[
<!ENTITY % local.figure.attrib "">
<!ENTITY % figure.role.attrib "%role.attrib;">

<!ENTITY % figure.element "INCLUDE">
<![%figure.element;[
<!ELEMENT figure ((%formalobject.title.content;), (%figure.mix; |
		%link.char.class;)+)>
<!--end of figure.element-->]]>

<!-- Float: Whether the Figure is supposed to be rendered
		where convenient (yes (1) value) or at the place it occurs
		in the text (no (0) value, the default) -->


<!ENTITY % figure.attlist "INCLUDE">
<![%figure.attlist;[
<!ATTLIST figure
		float		%yesorno.attvals;	'0'
		pgwide      	%yesorno.attvals;       #IMPLIED
		%label.attrib;
		%common.attrib;
		%figure.role.attrib;
		%local.figure.attrib;
>
<!--end of figure.attlist-->]]>
<!--end of figure.module-->]]>

<!ENTITY % informalfigure.module "INCLUDE">
<![ %informalfigure.module; [
<!ENTITY % local.informalfigure.attrib "">
<!ENTITY % informalfigure.role.attrib "%role.attrib;">

<!ENTITY % informalfigure.element "INCLUDE">
<![ %informalfigure.element; [
<!ELEMENT informalfigure ((%figure.mix; | %link.char.class;)+)>
<!--end of informalfigure.element-->]]>

<!ENTITY % informalfigure.attlist "INCLUDE">
<![ %informalfigure.attlist; [
<!--
Float: Whether the Figure is supposed to be rendered
where convenient (yes (1) value) or at the place it occurs
in the text (no (0) value, the default)
-->
<!ATTLIST informalfigure
		float		%yesorno.attvals;	"0"
		pgwide      	%yesorno.attvals;       #IMPLIED
		%label.attrib;
		%common.attrib;
		%informalfigure.role.attrib;
		%local.informalfigure.attrib;
>
<!--end of informalfigure.attlist-->]]>
<!--end of informalfigure.module-->]]>

<!ENTITY % graphicco.module "INCLUDE">
<![%graphicco.module;[
<!ENTITY % local.graphicco.attrib "">
<!ENTITY % graphicco.role.attrib "%role.attrib;">

<!ENTITY % graphicco.element "INCLUDE">
<![%graphicco.element;[
<!ELEMENT graphicco (areaspec, graphic, calloutlist*)>
<!--end of graphicco.element-->]]>

<!ENTITY % graphicco.attlist "INCLUDE">
<![%graphicco.attlist;[
<!ATTLIST graphicco
		%common.attrib;
		%graphicco.role.attrib;
		%local.graphicco.attrib;
>
<!--end of graphicco.attlist-->]]>
<!-- AreaSpec (defined above in Examples)-->
<!-- CalloutList (defined above in Lists)-->
<!--end of graphicco.module-->]]>

<!-- Graphical data can be the content of Graphic, or you can reference
     an external file either as an entity (Entitref) or a filename
     (Fileref). -->

<!ENTITY % graphic.module "INCLUDE">
<![%graphic.module;[
<!ENTITY % local.graphic.attrib "">
<!ENTITY % graphic.role.attrib "%role.attrib;">

<!ENTITY % graphic.element "INCLUDE">
<![%graphic.element;[
<!ELEMENT graphic EMPTY>
<!--end of graphic.element-->]]>

<!ENTITY % graphic.attlist "INCLUDE">
<![%graphic.attlist;[
<!ATTLIST graphic
		%graphics.attrib;
		%common.attrib;
		%graphic.role.attrib;
		%local.graphic.attrib;
>
<!--end of graphic.attlist-->]]>
<!--end of graphic.module-->]]>

<!ENTITY % inlinegraphic.module "INCLUDE">
<![%inlinegraphic.module;[
<!ENTITY % local.inlinegraphic.attrib "">
<!ENTITY % inlinegraphic.role.attrib "%role.attrib;">

<!ENTITY % inlinegraphic.element "INCLUDE">
<![%inlinegraphic.element;[
<!ELEMENT inlinegraphic EMPTY>
<!--end of inlinegraphic.element-->]]>

<!ENTITY % inlinegraphic.attlist "INCLUDE">
<![%inlinegraphic.attlist;[
<!ATTLIST inlinegraphic
		%graphics.attrib;
		%common.attrib;
		%inlinegraphic.role.attrib;
		%local.inlinegraphic.attrib;
>
<!--end of inlinegraphic.attlist-->]]>
<!--end of inlinegraphic.module-->]]>

<!ENTITY % mediaobject.content.module "INCLUDE">
<![ %mediaobject.content.module; [

<!ENTITY % mediaobject.module "INCLUDE">
<![ %mediaobject.module; [
<!ENTITY % local.mediaobject.attrib "">
<!ENTITY % mediaobject.role.attrib "%role.attrib;">

<!ENTITY % mediaobject.element "INCLUDE">
<![ %mediaobject.element; [
<!ELEMENT mediaobject (objectinfo?,
                           (%mediaobject.mix;),
			   (%mediaobject.mix;|textobject)*,
			   caption?)>
<!--end of mediaobject.element-->]]>

<!ENTITY % mediaobject.attlist "INCLUDE">
<![ %mediaobject.attlist; [
<!ATTLIST mediaobject
		%common.attrib;
		%mediaobject.role.attrib;
		%local.mediaobject.attrib;
>
<!--end of mediaobject.attlist-->]]>
<!--end of mediaobject.module-->]]>

<!ENTITY % inlinemediaobject.module "INCLUDE">
<![ %inlinemediaobject.module; [
<!ENTITY % local.inlinemediaobject.attrib "">
<!ENTITY % inlinemediaobject.role.attrib "%role.attrib;">

<!ENTITY % inlinemediaobject.element "INCLUDE">
<![ %inlinemediaobject.element; [
<!ELEMENT inlinemediaobject (objectinfo?,
                	         (%mediaobject.mix;),
				 (%mediaobject.mix;|textobject)*)>
<!--end of inlinemediaobject.element-->]]>

<!ENTITY % inlinemediaobject.attlist "INCLUDE">
<![ %inlinemediaobject.attlist; [
<!ATTLIST inlinemediaobject
		%common.attrib;
		%inlinemediaobject.role.attrib;
		%local.inlinemediaobject.attrib;
>
<!--end of inlinemediaobject.attlist-->]]>
<!--end of inlinemediaobject.module-->]]>

<!ENTITY % videoobject.module "INCLUDE">
<![ %videoobject.module; [
<!ENTITY % local.videoobject.attrib "">
<!ENTITY % videoobject.role.attrib "%role.attrib;">

<!ENTITY % videoobject.element "INCLUDE">
<![ %videoobject.element; [
<!ELEMENT videoobject (objectinfo?, videodata)>
<!--end of videoobject.element-->]]>

<!ENTITY % videoobject.attlist "INCLUDE">
<![ %videoobject.attlist; [
<!ATTLIST videoobject
		%common.attrib;
		%videoobject.role.attrib;
		%local.videoobject.attrib;
>
<!--end of videoobject.attlist-->]]>
<!--end of videoobject.module-->]]>

<!ENTITY % audioobject.module "INCLUDE">
<![ %audioobject.module; [
<!ENTITY % local.audioobject.attrib "">
<!ENTITY % audioobject.role.attrib "%role.attrib;">

<!ENTITY % audioobject.element "INCLUDE">
<![ %audioobject.element; [
<!ELEMENT audioobject (objectinfo?, audiodata)>
<!--end of audioobject.element-->]]>

<!ENTITY % audioobject.attlist "INCLUDE">
<![ %audioobject.attlist; [
<!ATTLIST audioobject
		%common.attrib;
		%audioobject.role.attrib;
		%local.audioobject.attrib;
>
<!--end of audioobject.attlist-->]]>
<!--end of audioobject.module-->]]>

<!ENTITY % imageobject.module "INCLUDE">
<![ %imageobject.module; [
<!ENTITY % local.imageobject.attrib "">
<!ENTITY % imageobject.role.attrib "%role.attrib;">

<!ENTITY % imageobject.element "INCLUDE">
<![ %imageobject.element; [
<!ELEMENT imageobject (objectinfo?, imagedata)>
<!--end of imageobject.element-->]]>

<!ENTITY % imageobject.attlist "INCLUDE">
<![ %imageobject.attlist; [
<!ATTLIST imageobject
		%common.attrib;
		%imageobject.role.attrib;
		%local.imageobject.attrib;
>
<!--end of imageobject.attlist-->]]>
<!--end of imageobject.module-->]]>

<!ENTITY % textobject.module "INCLUDE">
<![ %textobject.module; [
<!ENTITY % local.textobject.attrib "">
<!ENTITY % textobject.role.attrib "%role.attrib;">

<!ENTITY % textobject.element "INCLUDE">
<![ %textobject.element; [
<!ELEMENT textobject (objectinfo?, (phrase|(%textobject.mix;)+))>
<!--end of textobject.element-->]]>

<!ENTITY % textobject.attlist "INCLUDE">
<![ %textobject.attlist; [
<!ATTLIST textobject
		%common.attrib;
		%textobject.role.attrib;
		%local.textobject.attrib;
>
<!--end of textobject.attlist-->]]>
<!--end of textobject.module-->]]>

<!ENTITY % objectinfo.module "INCLUDE">
<![ %objectinfo.module; [
<!ENTITY % local.objectinfo.attrib "">
<!ENTITY % objectinfo.role.attrib "%role.attrib;">

<!ENTITY % objectinfo.element "INCLUDE">
<![ %objectinfo.element; [
<!ELEMENT objectinfo ((graphic | mediaobject | legalnotice | modespec 
	| subjectset | keywordset | itermset | %bibliocomponent.mix;)+)>
<!--end of objectinfo.element-->]]>

<!ENTITY % objectinfo.attlist "INCLUDE">
<![ %objectinfo.attlist; [
<!ATTLIST objectinfo
		%common.attrib;
		%objectinfo.role.attrib;
		%local.objectinfo.attrib;
>
<!--end of objectinfo.attlist-->]]>
<!--end of objectinfo.module-->]]>

<!--EntityRef: Name of an external entity containing the content
	of the object data-->
<!--FileRef: Filename, qualified by a pathname if desired, 
	designating the file containing the content of the object data-->
<!--Format: Notation of the element content, if any-->
<!--SrcCredit: Information about the source of the image-->
<!ENTITY % local.objectdata.attrib "">
<!ENTITY % objectdata.attrib
	"
	entityref	ENTITY		#IMPLIED
	fileref 	CDATA		#IMPLIED
	format		(%notation.class;)
					#IMPLIED
	srccredit	CDATA		#IMPLIED
	%local.objectdata.attrib;"
>

<!ENTITY % videodata.module "INCLUDE">
<![ %videodata.module; [
<!ENTITY % local.videodata.attrib "">
<!ENTITY % videodata.role.attrib "%role.attrib;">

<!ENTITY % videodata.element "INCLUDE">
<![ %videodata.element; [
<!ELEMENT videodata EMPTY>
<!--end of videodata.element-->]]>

<!ENTITY % videodata.attlist "INCLUDE">
<![ %videodata.attlist; [

<!--Width: Same as CALS reprowid (desired width)-->
<!--Depth: Same as CALS reprodep (desired depth)-->
<!--Align: Same as CALS hplace with 'none' removed; #IMPLIED means 
	application-specific-->
<!--Scale: Conflation of CALS hscale and vscale-->
<!--Scalefit: Same as CALS scalefit-->
<!ATTLIST videodata
		%common.attrib;
		%objectdata.attrib;
	width		CDATA		#IMPLIED
	depth		CDATA		#IMPLIED
	align		(left
			|right 
			|center)	#IMPLIED
	scale		CDATA		#IMPLIED
	scalefit	%yesorno.attvals;
					#IMPLIED
		%videodata.role.attrib;
		%local.videodata.attrib;
>
<!--end of videodata.attlist-->]]>
<!--end of videodata.module-->]]>

<!ENTITY % audiodata.module "INCLUDE">
<![ %audiodata.module; [
<!ENTITY % local.audiodata.attrib "">
<!ENTITY % audiodata.role.attrib "%role.attrib;">

<!ENTITY % audiodata.element "INCLUDE">
<![ %audiodata.element; [
<!ELEMENT audiodata EMPTY>
<!--end of audiodata.element-->]]>

<!ENTITY % audiodata.attlist "INCLUDE">
<![ %audiodata.attlist; [
<!ATTLIST audiodata
		%common.attrib;
		%objectdata.attrib;
		%local.audiodata.attrib;
		%audiodata.role.attrib;
>
<!--end of audiodata.attlist-->]]>
<!--end of audiodata.module-->]]>

<!ENTITY % imagedata.module "INCLUDE">
<![ %imagedata.module; [
<!ENTITY % local.imagedata.attrib "">
<!ENTITY % imagedata.role.attrib "%role.attrib;">

<!ENTITY % imagedata.element "INCLUDE">
<![ %imagedata.element; [
<!ELEMENT imagedata EMPTY>
<!--end of imagedata.element-->]]>

<!ENTITY % imagedata.attlist "INCLUDE">
<![ %imagedata.attlist; [

<!--Width: Same as CALS reprowid (desired width)-->
<!--Depth: Same as CALS reprodep (desired depth)-->
<!--Align: Same as CALS hplace with 'none' removed; #IMPLIED means 
	application-specific-->
<!--Scale: Conflation of CALS hscale and vscale-->
<!--Scalefit: Same as CALS scalefit-->
<!ATTLIST imagedata
		%common.attrib;
		%objectdata.attrib;
	width		CDATA		#IMPLIED
	depth		CDATA		#IMPLIED
	align		(left
			|right 
			|center)	#IMPLIED
	scale		CDATA		#IMPLIED
	scalefit	%yesorno.attvals;
					#IMPLIED
		%local.imagedata.attrib;
		%imagedata.role.attrib;
>
<!--end of imagedata.attlist-->]]>
<!--end of imagedata.module-->]]>

<!ENTITY % caption.module "INCLUDE">
<![ %caption.module; [
<!ENTITY % local.caption.attrib "">
<!ENTITY % caption.role.attrib "%role.attrib;">

<!ENTITY % caption.element "INCLUDE">
<![ %caption.element; [
<!ELEMENT caption (%textobject.mix;)*>
<!--end of caption.element-->]]>

<!ENTITY % caption.attlist "INCLUDE">
<![ %caption.attlist; [
<!ATTLIST caption
		%common.attrib;
		%local.caption.attrib;
		%caption.role.attrib;
>
<!--end of caption.attlist-->]]>
<!--end of caption.module-->]]>

<!ENTITY % mediaobjectco.module "INCLUDE">
<![ %mediaobjectco.module; [
<!ENTITY % local.mediaobjectco.attrib "">
<!ENTITY % mediaobjectco.role.attrib "%role.attrib;">

<!ENTITY % mediaobjectco.element "INCLUDE">
<![ %mediaobjectco.element; [
<!ELEMENT mediaobjectco (objectinfo?, imageobjectco,
			   (imageobjectco|textobject)*)>
<!--end of mediaobjectco.element-->]]>

<!ENTITY % mediaobjectco.attlist "INCLUDE">
<![ %mediaobjectco.attlist; [
<!ATTLIST mediaobjectco
		%common.attrib;
		%mediaobjectco.role.attrib;
		%local.mediaobjectco.attrib;
>
<!--end of mediaobjectco.attlist-->]]>
<!--end of mediaobjectco.module-->]]>

<!ENTITY % imageobjectco.module "INCLUDE">
<![ %imageobjectco.module; [
<!ENTITY % local.imageobjectco.attrib "">
<!ENTITY % imageobjectco.role.attrib "%role.attrib;">

<!ENTITY % imageobjectco.element "INCLUDE">
<![ %imageobjectco.element; [
<!ELEMENT imageobjectco (areaspec, imageobject, calloutlist*)>
<!--end of imageobjectco.element-->]]>

<!ENTITY % imageobjectco.attlist "INCLUDE">
<![ %imageobjectco.attlist; [
<!ATTLIST imageobjectco
		%common.attrib;
		%imageobjectco.role.attrib;
		%local.imageobjectco.attrib;
>
<!--end of imageobjectco.attlist-->]]>
<!--end of imageobjectco.module-->]]>
<!--end of mediaobject.content.module-->]]>

<!-- Equations ........................ -->

<!-- This PE provides a mechanism for replacing equation content, -->
<!-- perhaps adding a new or different model (e.g., MathML) -->
<!ENTITY % equation.content "(alt?, (graphic+|mediaobject+))">
<!ENTITY % inlineequation.content "(alt?, (graphic+|inlinemediaobject+))">

<!ENTITY % equation.module "INCLUDE">
<![%equation.module;[
<!ENTITY % local.equation.attrib "">
<!ENTITY % equation.role.attrib "%role.attrib;">

<!ENTITY % equation.element "INCLUDE">
<![%equation.element;[
<!ELEMENT equation ((%formalobject.title.content;)?, (informalequation |
		%equation.content;))>
<!--end of equation.element-->]]>

<!ENTITY % equation.attlist "INCLUDE">
<![%equation.attlist;[
<!ATTLIST equation
		%label.attrib;
	 	%common.attrib;
		%equation.role.attrib;
		%local.equation.attrib;
>
<!--end of equation.attlist-->]]>
<!--end of equation.module-->]]>

<!ENTITY % informalequation.module "INCLUDE">
<![%informalequation.module;[
<!ENTITY % local.informalequation.attrib "">
<!ENTITY % informalequation.role.attrib "%role.attrib;">

<!ENTITY % informalequation.element "INCLUDE">
<![%informalequation.element;[
<!ELEMENT informalequation (%equation.content;) >
<!--end of informalequation.element-->]]>

<!ENTITY % informalequation.attlist "INCLUDE">
<![%informalequation.attlist;[
<!ATTLIST informalequation
		%common.attrib;
		%informalequation.role.attrib;
		%local.informalequation.attrib;
>
<!--end of informalequation.attlist-->]]>
<!--end of informalequation.module-->]]>

<!ENTITY % inlineequation.module "INCLUDE">
<![%inlineequation.module;[
<!ENTITY % local.inlineequation.attrib "">
<!ENTITY % inlineequation.role.attrib "%role.attrib;">

<!ENTITY % inlineequation.element "INCLUDE">
<![%inlineequation.element;[
<!ELEMENT inlineequation (%inlineequation.content;)>
<!--end of inlineequation.element-->]]>

<!ENTITY % inlineequation.attlist "INCLUDE">
<![%inlineequation.attlist;[
<!ATTLIST inlineequation
		%common.attrib;
		%inlineequation.role.attrib;
		%local.inlineequation.attrib;
>
<!--end of inlineequation.attlist-->]]>
<!--end of inlineequation.module-->]]>

<!ENTITY % alt.module "INCLUDE">
<![%alt.module;[
<!ENTITY % local.alt.attrib "">
<!ENTITY % alt.role.attrib "%role.attrib;">

<!ENTITY % alt.element "INCLUDE">
<![%alt.element;[
<!ELEMENT alt (#PCDATA)>
<!--end of alt.element-->]]>

<!ENTITY % alt.attlist "INCLUDE">
<![%alt.attlist;[
<!ATTLIST alt 
		%common.attrib;
		%alt.role.attrib;
		%local.alt.attrib;
>
<!--end of alt.attlist-->]]>
<!--end of alt.module-->]]>

<!-- Tables ........................... -->

<!ENTITY % table.module "INCLUDE">
<![%table.module;[

<!-- Choose a table model. CALS or OASIS XML Exchange -->

<!ENTITY % cals.table.module "INCLUDE">
<![%cals.table.module;[
<!ENTITY % exchange.table.module "IGNORE">
]]>
<!ENTITY % exchange.table.module "INCLUDE">

<!ENTITY % tables.role.attrib "%role.attrib;">

<![%cals.table.module;[
<!-- Add label and role attributes to table and informaltable -->
<!ENTITY % bodyatt "%label.attrib;">

<!-- Add common attributes to Table, TGroup, TBody, THead, TFoot, Row, 
     EntryTbl, and Entry (and InformalTable element). -->
<!ENTITY % secur
	"%common.attrib;
	%tables.role.attrib;">

<!ENTITY % common.table.attribs
	"%bodyatt;
	%secur;">

<!-- Content model for Table. -->
<!ENTITY % tbl.table.mdl
	"((%formalobject.title.content;), (%ndxterm.class;)*,
          (graphic+|mediaobject+|tgroup+))">

<!-- Allow either objects or inlines; beware of REs between elements. -->
<!ENTITY % tbl.entry.mdl "%para.char.mix; | %tabentry.mix;">

<!-- Reference CALS Table Model -->
<!ENTITY % tablemodel 
  PUBLIC "-//OASIS//DTD DocBook XML CALS Table Model V4.1.2//EN"
  "calstblx.dtd">
]]>

<![%exchange.table.module;[
<!-- Add common attributes and the Label attribute to Table and -->
<!-- InformalTable.                                             -->
<!ENTITY % bodyatt 
	"%common.attrib;
	%label.attrib;
	%tables.role.attrib;">

<!ENTITY % common.table.attribs
	"%bodyatt;">

<!-- Add common attributes to TGroup, ColSpec, TBody, THead, Row, Entry -->

<!ENTITY % tbl.tgroup.att       "%common.attrib;">
<!ENTITY % tbl.colspec.att      "%common.attrib;">
<!ENTITY % tbl.tbody.att        "%common.attrib;">
<!ENTITY % tbl.thead.att        "%common.attrib;">
<!ENTITY % tbl.row.att          "%common.attrib;">
<!ENTITY % tbl.entry.att        "%common.attrib;">

<!-- Content model for Table. -->
<!ENTITY % tbl.table.mdl
	"((%formalobject.title.content;),
          (%ndxterm.class;)*,
          (graphic+|tgroup+))">

<!-- Allow either objects or inlines; beware of REs between elements. -->
<!ENTITY % tbl.entry.mdl "(%para.char.mix; | %tabentry.mix;)*">

<!-- Reference OASIS Exchange Table Model -->
<!ENTITY % tablemodel 
  PUBLIC "-//OASIS//DTD XML Exchange Table Model 19990315//EN"
  "soextblx.dtd">
]]>

%tablemodel;

<!--end of table.module-->]]>

<!ENTITY % informaltable.module "INCLUDE">
<![%informaltable.module;[

<!-- Note that InformalTable is dependent on some of the entity
     declarations that customize Table. -->

<!ENTITY % local.informaltable.attrib "">

<!ENTITY % informaltable.element "INCLUDE">
<![%informaltable.element;[
<!ELEMENT informaltable (graphic+|mediaobject+|tgroup+)>
<!--end of informaltable.element-->]]>

<!-- Frame, Colsep, and Rowsep must be repeated because
		they are not in entities in the table module. -->
<!-- includes TabStyle, ToCentry, ShortEntry, 
				Orient, PgWide -->
<!-- includes Label -->
<!-- includes common attributes -->


<!ENTITY % informaltable.attlist "INCLUDE">
<![%informaltable.attlist;[
<!ATTLIST informaltable
		frame		(top
				|bottom
				|topbot
				|all
				|sides
				|none)			#IMPLIED
		colsep		%yesorno.attvals;	#IMPLIED
		rowsep		%yesorno.attvals;	#IMPLIED
		%common.table.attribs;
		%tbl.table.att;
		%local.informaltable.attrib;
>
<!--end of informaltable.attlist-->]]>
<!--end of informaltable.module-->]]>

<!-- ...................................................................... -->
<!-- Synopses ............................................................. -->

<!-- Synopsis ......................... -->

<!ENTITY % synopsis.module "INCLUDE">
<![%synopsis.module;[
<!ENTITY % local.synopsis.attrib "">
<!ENTITY % synopsis.role.attrib "%role.attrib;">

<!ENTITY % synopsis.element "INCLUDE">
<![%synopsis.element;[
<!ELEMENT synopsis (%para.char.mix; | graphic | mediaobject | lineannotation | co)*>
<!--end of synopsis.element-->]]>

<!ENTITY % synopsis.attlist "INCLUDE">
<![%synopsis.attlist;[
<!ATTLIST synopsis
		%label.attrib;
		%linespecific.attrib;
		%common.attrib;
		%synopsis.role.attrib;
		%local.synopsis.attrib;
>
<!--end of synopsis.attlist-->]]>

<!-- LineAnnotation (defined in the Inlines section, below)-->
<!--end of synopsis.module-->]]>

<!-- CmdSynopsis ...................... -->

<!ENTITY % cmdsynopsis.content.module "INCLUDE">
<![%cmdsynopsis.content.module;[
<!ENTITY % cmdsynopsis.module "INCLUDE">
<![%cmdsynopsis.module;[
<!ENTITY % local.cmdsynopsis.attrib "">
<!ENTITY % cmdsynopsis.role.attrib "%role.attrib;">

<!ENTITY % cmdsynopsis.element "INCLUDE">
<![%cmdsynopsis.element;[
<!ELEMENT cmdsynopsis ((command | arg | group | sbr)+, synopfragment*)>
<!--end of cmdsynopsis.element-->]]>

<!-- Sepchar: Character that should separate command and all 
		top-level arguments; alternate value might be e.g., &Delta; -->


<!ENTITY % cmdsynopsis.attlist "INCLUDE">
<![%cmdsynopsis.attlist;[
<!ATTLIST cmdsynopsis
		%label.attrib;
		sepchar		CDATA		" "
		cmdlength	CDATA		#IMPLIED
		%common.attrib;
		%cmdsynopsis.role.attrib;
		%local.cmdsynopsis.attrib;
>
<!--end of cmdsynopsis.attlist-->]]>
<!--end of cmdsynopsis.module-->]]>

<!ENTITY % arg.module "INCLUDE">
<![%arg.module;[
<!ENTITY % local.arg.attrib "">
<!ENTITY % arg.role.attrib "%role.attrib;">

<!ENTITY % arg.element "INCLUDE">
<![%arg.element;[
<!ELEMENT arg (#PCDATA 
		| arg 
		| group 
		| option 
		| synopfragmentref 
		| replaceable
		| sbr)*>
<!--end of arg.element-->]]>

<!-- Choice: Whether Arg must be supplied: Opt (optional to 
		supply, e.g. [arg]; the default), Req (required to supply, 
		e.g. {arg}), or Plain (required to supply, e.g. arg) -->
<!-- Rep: whether Arg is repeatable: Norepeat (e.g. arg without 
		ellipsis; the default), or Repeat (e.g. arg...) -->


<!ENTITY % arg.attlist "INCLUDE">
<![%arg.attlist;[
<!ATTLIST arg
		choice		(opt
				|req
				|plain)		'opt'
		rep		(norepeat
				|repeat)	'norepeat'
		%common.attrib;
		%arg.role.attrib;
		%local.arg.attrib;
>
<!--end of arg.attlist-->]]>
<!--end of arg.module-->]]>

<!ENTITY % group.module "INCLUDE">
<![%group.module;[

<!ENTITY % local.group.attrib "">
<!ENTITY % group.role.attrib "%role.attrib;">

<!ENTITY % group.element "INCLUDE">
<![%group.element;[
<!ELEMENT group ((arg | group | option | synopfragmentref 
		| replaceable | sbr)+)>
<!--end of group.element-->]]>

<!-- Choice: Whether Group must be supplied: Opt (optional to
		supply, e.g.  [g1|g2|g3]; the default), Req (required to
		supply, e.g.  {g1|g2|g3}), Plain (required to supply,
		e.g.  g1|g2|g3), OptMult (can supply zero or more, e.g.
		[[g1|g2|g3]]), or ReqMult (must supply one or more, e.g.
		{{g1|g2|g3}}) -->
<!-- Rep: whether Group is repeatable: Norepeat (e.g. group 
		without ellipsis; the default), or Repeat (e.g. group...) -->


<!ENTITY % group.attlist "INCLUDE">
<![%group.attlist;[
<!ATTLIST group
		choice		(opt
				|req
				|plain)         'opt'
		rep		(norepeat
				|repeat)	'norepeat'
		%common.attrib;
		%group.role.attrib;
		%local.group.attrib;
>
<!--end of group.attlist-->]]>
<!--end of group.module-->]]>

<!ENTITY % sbr.module "INCLUDE">
<![%sbr.module;[
<!ENTITY % local.sbr.attrib "">
<!-- Synopsis break -->
<!ENTITY % sbr.role.attrib "%role.attrib;">

<!ENTITY % sbr.element "INCLUDE">
<![%sbr.element;[
<!ELEMENT sbr EMPTY>
<!--end of sbr.element-->]]>

<!ENTITY % sbr.attlist "INCLUDE">
<![%sbr.attlist;[
<!ATTLIST sbr
		%common.attrib;
		%sbr.role.attrib;
		%local.sbr.attrib;
>
<!--end of sbr.attlist-->]]>
<!--end of sbr.module-->]]>

<!ENTITY % synopfragmentref.module "INCLUDE">
<![%synopfragmentref.module;[
<!ENTITY % local.synopfragmentref.attrib "">
<!ENTITY % synopfragmentref.role.attrib "%role.attrib;">

<!ENTITY % synopfragmentref.element "INCLUDE">
<![%synopfragmentref.element;[
<!ELEMENT synopfragmentref (#PCDATA)>
<!--end of synopfragmentref.element-->]]>

<!-- to SynopFragment of complex synopsis
			material for separate referencing -->


<!ENTITY % synopfragmentref.attlist "INCLUDE">
<![%synopfragmentref.attlist;[
<!ATTLIST synopfragmentref
		%linkendreq.attrib;		%common.attrib;
		%synopfragmentref.role.attrib;
		%local.synopfragmentref.attrib;
>
<!--end of synopfragmentref.attlist-->]]>
<!--end of synopfragmentref.module-->]]>

<!ENTITY % synopfragment.module "INCLUDE">
<![%synopfragment.module;[
<!ENTITY % local.synopfragment.attrib "">
<!ENTITY % synopfragment.role.attrib "%role.attrib;">

<!ENTITY % synopfragment.element "INCLUDE">
<![%synopfragment.element;[
<!ELEMENT synopfragment ((arg | group)+)>
<!--end of synopfragment.element-->]]>

<!ENTITY % synopfragment.attlist "INCLUDE">
<![%synopfragment.attlist;[
<!ATTLIST synopfragment
		%idreq.common.attrib;
		%synopfragment.role.attrib;
		%local.synopfragment.attrib;
>
<!--end of synopfragment.attlist-->]]>
<!--end of synopfragment.module-->]]>

<!-- Command (defined in the Inlines section, below)-->
<!-- Option (defined in the Inlines section, below)-->
<!-- Replaceable (defined in the Inlines section, below)-->
<!--end of cmdsynopsis.content.module-->]]>

<!-- FuncSynopsis ..................... -->

<!ENTITY % funcsynopsis.content.module "INCLUDE">
<![%funcsynopsis.content.module;[
<!ENTITY % funcsynopsis.module "INCLUDE">
<![%funcsynopsis.module;[

<!ENTITY % local.funcsynopsis.attrib "">
<!ENTITY % funcsynopsis.role.attrib "%role.attrib;">

<!ENTITY % funcsynopsis.element "INCLUDE">
<![%funcsynopsis.element;[
<!ELEMENT funcsynopsis ((funcsynopsisinfo | funcprototype)+)>
<!--end of funcsynopsis.element-->]]>

<!ENTITY % funcsynopsis.attlist "INCLUDE">
<![%funcsynopsis.attlist;[
<!ATTLIST funcsynopsis
		%label.attrib;
		%common.attrib;
		%funcsynopsis.role.attrib;
		%local.funcsynopsis.attrib;
>
<!--end of funcsynopsis.attlist-->]]>
<!--end of funcsynopsis.module-->]]>

<!ENTITY % funcsynopsisinfo.module "INCLUDE">
<![%funcsynopsisinfo.module;[
<!ENTITY % local.funcsynopsisinfo.attrib "">
<!ENTITY % funcsynopsisinfo.role.attrib "%role.attrib;">

<!ENTITY % funcsynopsisinfo.element "INCLUDE">
<![%funcsynopsisinfo.element;[
<!ELEMENT funcsynopsisinfo (%cptr.char.mix; | lineannotation)*>
<!--end of funcsynopsisinfo.element-->]]>

<!ENTITY % funcsynopsisinfo.attlist "INCLUDE">
<![%funcsynopsisinfo.attlist;[
<!ATTLIST funcsynopsisinfo
		%linespecific.attrib;
		%common.attrib;
		%funcsynopsisinfo.role.attrib;
		%local.funcsynopsisinfo.attrib;
>
<!--end of funcsynopsisinfo.attlist-->]]>
<!--end of funcsynopsisinfo.module-->]]>

<!ENTITY % funcprototype.module "INCLUDE">
<![%funcprototype.module;[
<!ENTITY % local.funcprototype.attrib "">
<!ENTITY % funcprototype.role.attrib "%role.attrib;">

<!ENTITY % funcprototype.element "INCLUDE">
<![%funcprototype.element;[
<!ELEMENT funcprototype (funcdef, (void | varargs | paramdef+))>
<!--end of funcprototype.element-->]]>

<!ENTITY % funcprototype.attlist "INCLUDE">
<![%funcprototype.attlist;[
<!ATTLIST funcprototype
		%common.attrib;
		%funcprototype.role.attrib;
		%local.funcprototype.attrib;
>
<!--end of funcprototype.attlist-->]]>
<!--end of funcprototype.module-->]]>

<!ENTITY % funcdef.module "INCLUDE">
<![%funcdef.module;[
<!ENTITY % local.funcdef.attrib "">
<!ENTITY % funcdef.role.attrib "%role.attrib;">

<!ENTITY % funcdef.element "INCLUDE">
<![%funcdef.element;[
<!ELEMENT funcdef (#PCDATA 
		| replaceable 
		| function)*>
<!--end of funcdef.element-->]]>

<!ENTITY % funcdef.attlist "INCLUDE">
<![%funcdef.attlist;[
<!ATTLIST funcdef
		%common.attrib;
		%funcdef.role.attrib;
		%local.funcdef.attrib;
>
<!--end of funcdef.attlist-->]]>
<!--end of funcdef.module-->]]>

<!ENTITY % void.module "INCLUDE">
<![%void.module;[
<!ENTITY % local.void.attrib "">
<!ENTITY % void.role.attrib "%role.attrib;">

<!ENTITY % void.element "INCLUDE">
<![%void.element;[
<!ELEMENT void EMPTY>
<!--end of void.element-->]]>

<!ENTITY % void.attlist "INCLUDE">
<![%void.attlist;[
<!ATTLIST void
		%common.attrib;
		%void.role.attrib;
		%local.void.attrib;
>
<!--end of void.attlist-->]]>
<!--end of void.module-->]]>

<!ENTITY % varargs.module "INCLUDE">
<![%varargs.module;[
<!ENTITY % local.varargs.attrib "">
<!ENTITY % varargs.role.attrib "%role.attrib;">

<!ENTITY % varargs.element "INCLUDE">
<![%varargs.element;[
<!ELEMENT varargs EMPTY>
<!--end of varargs.element-->]]>

<!ENTITY % varargs.attlist "INCLUDE">
<![%varargs.attlist;[
<!ATTLIST varargs
		%common.attrib;
		%varargs.role.attrib;
		%local.varargs.attrib;
>
<!--end of varargs.attlist-->]]>
<!--end of varargs.module-->]]>

<!-- Processing assumes that only one Parameter will appear in a
     ParamDef, and that FuncParams will be used at most once, for
     providing information on the "inner parameters" for parameters that
     are pointers to functions. -->

<!ENTITY % paramdef.module "INCLUDE">
<![%paramdef.module;[
<!ENTITY % local.paramdef.attrib "">
<!ENTITY % paramdef.role.attrib "%role.attrib;">

<!ENTITY % paramdef.element "INCLUDE">
<![%paramdef.element;[
<!ELEMENT paramdef (#PCDATA 
		| replaceable 
		| parameter 
		| funcparams)*>
<!--end of paramdef.element-->]]>

<!ENTITY % paramdef.attlist "INCLUDE">
<![%paramdef.attlist;[
<!ATTLIST paramdef
		%common.attrib;
		%paramdef.role.attrib;
		%local.paramdef.attrib;
>
<!--end of paramdef.attlist-->]]>
<!--end of paramdef.module-->]]>

<!ENTITY % funcparams.module "INCLUDE">
<![%funcparams.module;[
<!ENTITY % local.funcparams.attrib "">
<!ENTITY % funcparams.role.attrib "%role.attrib;">

<!ENTITY % funcparams.element "INCLUDE">
<![%funcparams.element;[
<!ELEMENT funcparams (%cptr.char.mix;)*>
<!--end of funcparams.element-->]]>

<!ENTITY % funcparams.attlist "INCLUDE">
<![%funcparams.attlist;[
<!ATTLIST funcparams
		%common.attrib;
		%funcparams.role.attrib;
		%local.funcparams.attrib;
>
<!--end of funcparams.attlist-->]]>
<!--end of funcparams.module-->]]>

<!-- LineAnnotation (defined in the Inlines section, below)-->
<!-- Replaceable (defined in the Inlines section, below)-->
<!-- Function (defined in the Inlines section, below)-->
<!-- Parameter (defined in the Inlines section, below)-->
<!--end of funcsynopsis.content.module-->]]>

<!-- ClassSynopsis ..................... -->

<!ENTITY % classsynopsis.content.module "INCLUDE">
<![%classsynopsis.content.module;[

<!ENTITY % classsynopsis.module "INCLUDE">
<![%classsynopsis.module;[
<!ENTITY % local.classsynopsis.attrib "">
<!ENTITY % classsynopsis.role.attrib "%role.attrib;">

<!ENTITY % classsynopsis.element "INCLUDE">
<![%classsynopsis.element;[
<!ELEMENT classsynopsis ((ooclass|oointerface|ooexception)+,
                         (classsynopsisinfo
                          |fieldsynopsis|%method.synop.class;)*)>
<!--end of classsynopsis.element-->]]>

<!ENTITY % classsynopsis.attlist "INCLUDE">
<![%classsynopsis.attlist;[
<!ATTLIST classsynopsis
	%common.attrib;
	%classsynopsis.role.attrib;
	%local.classsynopsis.attrib;
	language	CDATA	#IMPLIED
	class	(class|interface)	"class"
>
<!--end of classsynopsis.attlist-->]]>
<!--end of classsynopsis.module-->]]>

<!ENTITY % classsynopsisinfo.module "INCLUDE">
<![ %classsynopsisinfo.module; [
<!ENTITY % local.classsynopsisinfo.attrib "">
<!ENTITY % classsynopsisinfo.role.attrib "%role.attrib;">

<!ENTITY % classsynopsisinfo.element "INCLUDE">
<![ %classsynopsisinfo.element; [
<!ELEMENT classsynopsisinfo (%cptr.char.mix; | lineannotation)*>
<!--end of classsynopsisinfo.element-->]]>

<!ENTITY % classsynopsisinfo.attlist "INCLUDE">
<![ %classsynopsisinfo.attlist; [
<!ATTLIST classsynopsisinfo
		%linespecific.attrib;
		%common.attrib;
		%classsynopsisinfo.role.attrib;
		%local.classsynopsisinfo.attrib;
>
<!--end of classsynopsisinfo.attlist-->]]>
<!--end of classsynopsisinfo.module-->]]>

<!ENTITY % ooclass.module "INCLUDE">
<![%ooclass.module;[
<!ENTITY % local.ooclass.attrib "">
<!ENTITY % ooclass.role.attrib "%role.attrib;">

<!ENTITY % ooclass.element "INCLUDE">
<![%ooclass.element;[
<!ELEMENT ooclass (modifier*, classname)>
<!--end of ooclass.element-->]]>

<!ENTITY % ooclass.attlist "INCLUDE">
<![%ooclass.attlist;[
<!ATTLIST ooclass
	%common.attrib;
	%ooclass.role.attrib;
	%local.ooclass.attrib;
>
<!--end of ooclass.attlist-->]]>
<!--end of ooclass.module-->]]>

<!ENTITY % oointerface.module "INCLUDE">
<![%oointerface.module;[
<!ENTITY % local.oointerface.attrib "">
<!ENTITY % oointerface.role.attrib "%role.attrib;">

<!ENTITY % oointerface.element "INCLUDE">
<![%oointerface.element;[
<!ELEMENT oointerface (modifier*, interfacename)>
<!--end of oointerface.element-->]]>

<!ENTITY % oointerface.attlist "INCLUDE">
<![%oointerface.attlist;[
<!ATTLIST oointerface
	%common.attrib;
	%oointerface.role.attrib;
	%local.oointerface.attrib;
>
<!--end of oointerface.attlist-->]]>
<!--end of oointerface.module-->]]>

<!ENTITY % ooexception.module "INCLUDE">
<![%ooexception.module;[
<!ENTITY % local.ooexception.attrib "">
<!ENTITY % ooexception.role.attrib "%role.attrib;">

<!ENTITY % ooexception.element "INCLUDE">
<![%ooexception.element;[
<!ELEMENT ooexception (modifier*, exceptionname)>
<!--end of ooexception.element-->]]>

<!ENTITY % ooexception.attlist "INCLUDE">
<![%ooexception.attlist;[
<!ATTLIST ooexception
	%common.attrib;
	%ooexception.role.attrib;
	%local.ooexception.attrib;
>
<!--end of ooexception.attlist-->]]>
<!--end of ooexception.module-->]]>

<!ENTITY % modifier.module "INCLUDE">
<![%modifier.module;[
<!ENTITY % local.modifier.attrib "">
<!ENTITY % modifier.role.attrib "%role.attrib;">

<!ENTITY % modifier.element "INCLUDE">
<![%modifier.element;[
<!ELEMENT modifier (%smallcptr.char.mix;)*>
<!--end of modifier.element-->]]>

<!ENTITY % modifier.attlist "INCLUDE">
<![%modifier.attlist;[
<!ATTLIST modifier
	%common.attrib;
	%modifier.role.attrib;
	%local.modifier.attrib;
>
<!--end of modifier.attlist-->]]>
<!--end of modifier.module-->]]>

<!ENTITY % interfacename.module "INCLUDE">
<![%interfacename.module;[
<!ENTITY % local.interfacename.attrib "">
<!ENTITY % interfacename.role.attrib "%role.attrib;">

<!ENTITY % interfacename.element "INCLUDE">
<![%interfacename.element;[
<!ELEMENT interfacename (%smallcptr.char.mix;)*>
<!--end of interfacename.element-->]]>

<!ENTITY % interfacename.attlist "INCLUDE">
<![%interfacename.attlist;[
<!ATTLIST interfacename
	%common.attrib;
	%interfacename.role.attrib;
	%local.interfacename.attrib;
>
<!--end of interfacename.attlist-->]]>
<!--end of interfacename.module-->]]>

<!ENTITY % exceptionname.module "INCLUDE">
<![%exceptionname.module;[
<!ENTITY % local.exceptionname.attrib "">
<!ENTITY % exceptionname.role.attrib "%role.attrib;">

<!ENTITY % exceptionname.element "INCLUDE">
<![%exceptionname.element;[
<!ELEMENT exceptionname (%smallcptr.char.mix;)*>
<!--end of exceptionname.element-->]]>

<!ENTITY % exceptionname.attlist "INCLUDE">
<![%exceptionname.attlist;[
<!ATTLIST exceptionname
	%common.attrib;
	%exceptionname.role.attrib;
	%local.exceptionname.attrib;
>
<!--end of exceptionname.attlist-->]]>
<!--end of exceptionname.module-->]]>

<!ENTITY % fieldsynopsis.module "INCLUDE">
<![%fieldsynopsis.module;[
<!ENTITY % local.fieldsynopsis.attrib "">
<!ENTITY % fieldsynopsis.role.attrib "%role.attrib;">

<!ENTITY % fieldsynopsis.element "INCLUDE">
<![%fieldsynopsis.element;[
<!ELEMENT fieldsynopsis (modifier*, type?, varname, initializer?)>
<!--end of fieldsynopsis.element-->]]>

<!ENTITY % fieldsynopsis.attlist "INCLUDE">
<![%fieldsynopsis.attlist;[
<!ATTLIST fieldsynopsis
	%common.attrib;
	%fieldsynopsis.role.attrib;
	%local.fieldsynopsis.attrib;
>
<!--end of fieldsynopsis.attlist-->]]>
<!--end of fieldsynopsis.module-->]]>

<!ENTITY % initializer.module "INCLUDE">
<![%initializer.module;[
<!ENTITY % local.initializer.attrib "">
<!ENTITY % initializer.role.attrib "%role.attrib;">

<!ENTITY % initializer.element "INCLUDE">
<![%initializer.element;[
<!ELEMENT initializer (%smallcptr.char.mix;)*>
<!--end of initializer.element-->]]>

<!ENTITY % initializer.attlist "INCLUDE">
<![%initializer.attlist;[
<!ATTLIST initializer
	%common.attrib;
	%initializer.role.attrib;
	%local.initializer.attrib;
>
<!--end of initializer.attlist-->]]>
<!--end of initializer.module-->]]>

<!ENTITY % constructorsynopsis.module "INCLUDE">
<![%constructorsynopsis.module;[
<!ENTITY % local.constructorsynopsis.attrib "">
<!ENTITY % constructorsynopsis.role.attrib "%role.attrib;">

<!ENTITY % constructorsynopsis.element "INCLUDE">
<![%constructorsynopsis.element;[
<!ELEMENT constructorsynopsis (modifier*,
                               methodname?,
                               (methodparam+|void),
                               exceptionname*)>
<!--end of constructorsynopsis.element-->]]>

<!ENTITY % constructorsynopsis.attlist "INCLUDE">
<![%constructorsynopsis.attlist;[
<!ATTLIST constructorsynopsis
	%common.attrib;
	%constructorsynopsis.role.attrib;
	%local.constructorsynopsis.attrib;
>
<!--end of constructorsynopsis.attlist-->]]>
<!--end of constructorsynopsis.module-->]]>

<!ENTITY % destructorsynopsis.module "INCLUDE">
<![%destructorsynopsis.module;[
<!ENTITY % local.destructorsynopsis.attrib "">
<!ENTITY % destructorsynopsis.role.attrib "%role.attrib;">

<!ENTITY % destructorsynopsis.element "INCLUDE">
<![%destructorsynopsis.element;[
<!ELEMENT destructorsynopsis (modifier*,
                              methodname?,
                              (methodparam+|void),
                              exceptionname*)>
<!--end of destructorsynopsis.element-->]]>

<!ENTITY % destructorsynopsis.attlist "INCLUDE">
<![%destructorsynopsis.attlist;[
<!ATTLIST destructorsynopsis
	%common.attrib;
	%destructorsynopsis.role.attrib;
	%local.destructorsynopsis.attrib;
>
<!--end of destructorsynopsis.attlist-->]]>
<!--end of destructorsynopsis.module-->]]>

<!ENTITY % methodsynopsis.module "INCLUDE">
<![%methodsynopsis.module;[
<!ENTITY % local.methodsynopsis.attrib "">
<!ENTITY % methodsynopsis.role.attrib "%role.attrib;">

<!ENTITY % methodsynopsis.element "INCLUDE">
<![%methodsynopsis.element;[
<!ELEMENT methodsynopsis (modifier*,
                          (type|void)?,
                          methodname,
                          (methodparam+|void),
                          exceptionname*,
                          modifier*)>
<!--end of methodsynopsis.element-->]]>

<!ENTITY % methodsynopsis.attlist "INCLUDE">
<![%methodsynopsis.attlist;[
<!ATTLIST methodsynopsis
	%common.attrib;
	%methodsynopsis.role.attrib;
	%local.methodsynopsis.attrib;
>
<!--end of methodsynopsis.attlist-->]]>
<!--end of methodsynopsis.module-->]]>

<!ENTITY % methodname.module "INCLUDE">
<![%methodname.module;[
<!ENTITY % local.methodname.attrib "">
<!ENTITY % methodname.role.attrib "%role.attrib;">

<!ENTITY % methodname.element "INCLUDE">
<![%methodname.element;[
<!ELEMENT methodname (%smallcptr.char.mix;)*>
<!--end of methodname.element-->]]>

<!ENTITY % methodname.attlist "INCLUDE">
<![%methodname.attlist;[
<!ATTLIST methodname
	%common.attrib;
	%methodname.role.attrib;
	%local.methodname.attrib;
>
<!--end of methodname.attlist-->]]>
<!--end of methodname.module-->]]>

<!ENTITY % methodparam.module "INCLUDE">
<![%methodparam.module;[
<!ENTITY % local.methodparam.attrib "">
<!ENTITY % methodparam.role.attrib "%role.attrib;">

<!ENTITY % methodparam.element "INCLUDE">
<![%methodparam.element;[
<!ELEMENT methodparam (modifier*,
                       type?,
                       ((parameter,initializer?)|funcparams),
                       modifier*)>
<!--end of methodparam.element-->]]>

<!ENTITY % methodparam.attlist "INCLUDE">
<![%methodparam.attlist;[
<!ATTLIST methodparam
	%common.attrib;
	%methodparam.role.attrib;
	%local.methodparam.attrib;
	choice		(opt
			|req
			|plain)		"req"
	rep		(norepeat
			|repeat)	"norepeat"
>
<!--end of methodparam.attlist-->]]>
<!--end of methodparam.module-->]]>
<!--end of classsynopsis.content.module-->]]>

<!-- ...................................................................... -->
<!-- Document information entities and elements ........................... -->

<!-- The document information elements include some elements that are
     currently used only in the document hierarchy module. They are
     defined here so that they will be available for use in customized
     document hierarchies. -->

<!-- .................................. -->

<!ENTITY % docinfo.content.module "INCLUDE">
<![%docinfo.content.module;[

<!-- Ackno ............................ -->

<!ENTITY % ackno.module "INCLUDE">
<![%ackno.module;[
<!ENTITY % local.ackno.attrib "">
<!ENTITY % ackno.role.attrib "%role.attrib;">

<!ENTITY % ackno.element "INCLUDE">
<![%ackno.element;[
<!ELEMENT ackno (%docinfo.char.mix;)*>
<!--end of ackno.element-->]]>

<!ENTITY % ackno.attlist "INCLUDE">
<![%ackno.attlist;[
<!ATTLIST ackno
		%common.attrib;
		%ackno.role.attrib;
		%local.ackno.attrib;
>
<!--end of ackno.attlist-->]]>
<!--end of ackno.module-->]]>

<!-- Address .......................... -->

<!ENTITY % address.content.module "INCLUDE">
<![%address.content.module;[
<!ENTITY % address.module "INCLUDE">
<![%address.module;[
<!ENTITY % local.address.attrib "">
<!ENTITY % address.role.attrib "%role.attrib;">

<!ENTITY % address.element "INCLUDE">
<![%address.element;[
<!ELEMENT address (#PCDATA|%person.ident.mix;
		|street|pob|postcode|city|state|country|phone
		|fax|email|otheraddr)*>
<!--end of address.element-->]]>

<!ENTITY % address.attlist "INCLUDE">
<![%address.attlist;[
<!ATTLIST address
		%linespecific.attrib;
		%common.attrib;
		%address.role.attrib;
		%local.address.attrib;
>
<!--end of address.attlist-->]]>
<!--end of address.module-->]]>

  <!ENTITY % street.module "INCLUDE">
  <![%street.module;[
 <!ENTITY % local.street.attrib "">
  <!ENTITY % street.role.attrib "%role.attrib;">
  
<!ENTITY % street.element "INCLUDE">
<![%street.element;[
<!ELEMENT street (%docinfo.char.mix;)*>
<!--end of street.element-->]]>
  
<!ENTITY % street.attlist "INCLUDE">
<![%street.attlist;[
<!ATTLIST street
		%common.attrib;
		%street.role.attrib;
		%local.street.attrib;
>
<!--end of street.attlist-->]]>
  <!--end of street.module-->]]>

  <!ENTITY % pob.module "INCLUDE">
  <![%pob.module;[
  <!ENTITY % local.pob.attrib "">
  <!ENTITY % pob.role.attrib "%role.attrib;">
  
<!ENTITY % pob.element "INCLUDE">
<![%pob.element;[
<!ELEMENT pob (%docinfo.char.mix;)*>
<!--end of pob.element-->]]>
  
<!ENTITY % pob.attlist "INCLUDE">
<![%pob.attlist;[
<!ATTLIST pob
		%common.attrib;
		%pob.role.attrib;
		%local.pob.attrib;
>
<!--end of pob.attlist-->]]>
  <!--end of pob.module-->]]>

  <!ENTITY % postcode.module "INCLUDE">
  <![%postcode.module;[
  <!ENTITY % local.postcode.attrib "">
  <!ENTITY % postcode.role.attrib "%role.attrib;">
  
<!ENTITY % postcode.element "INCLUDE">
<![%postcode.element;[
<!ELEMENT postcode (%docinfo.char.mix;)*>
<!--end of postcode.element-->]]>
  
<!ENTITY % postcode.attlist "INCLUDE">
<![%postcode.attlist;[
<!ATTLIST postcode
		%common.attrib;
		%postcode.role.attrib;
		%local.postcode.attrib;
>
<!--end of postcode.attlist-->]]>
  <!--end of postcode.module-->]]>

  <!ENTITY % city.module "INCLUDE">
  <![%city.module;[
  <!ENTITY % local.city.attrib "">
  <!ENTITY % city.role.attrib "%role.attrib;">
  
<!ENTITY % city.element "INCLUDE">
<![%city.element;[
<!ELEMENT city (%docinfo.char.mix;)*>
<!--end of city.element-->]]>
  
<!ENTITY % city.attlist "INCLUDE">
<![%city.attlist;[
<!ATTLIST city
		%common.attrib;
		%city.role.attrib;
		%local.city.attrib;
>
<!--end of city.attlist-->]]>
  <!--end of city.module-->]]>

  <!ENTITY % state.module "INCLUDE">
  <![%state.module;[
  <!ENTITY % local.state.attrib "">
  <!ENTITY % state.role.attrib "%role.attrib;">
  
<!ENTITY % state.element "INCLUDE">
<![%state.element;[
<!ELEMENT state (%docinfo.char.mix;)*>
<!--end of state.element-->]]>
  
<!ENTITY % state.attlist "INCLUDE">
<![%state.attlist;[
<!ATTLIST state
		%common.attrib;
		%state.role.attrib;
		%local.state.attrib;
>
<!--end of state.attlist-->]]>
  <!--end of state.module-->]]>

  <!ENTITY % country.module "INCLUDE">
  <![%country.module;[
  <!ENTITY % local.country.attrib "">
  <!ENTITY % country.role.attrib "%role.attrib;">
  
<!ENTITY % country.element "INCLUDE">
<![%country.element;[
<!ELEMENT country (%docinfo.char.mix;)*>
<!--end of country.element-->]]>
  
<!ENTITY % country.attlist "INCLUDE">
<![%country.attlist;[
<!ATTLIST country
		%common.attrib;
		%country.role.attrib;
		%local.country.attrib;
>
<!--end of country.attlist-->]]>
  <!--end of country.module-->]]>

  <!ENTITY % phone.module "INCLUDE">
  <![%phone.module;[
  <!ENTITY % local.phone.attrib "">
  <!ENTITY % phone.role.attrib "%role.attrib;">
  
<!ENTITY % phone.element "INCLUDE">
<![%phone.element;[
<!ELEMENT phone (%docinfo.char.mix;)*>
<!--end of phone.element-->]]>
  
<!ENTITY % phone.attlist "INCLUDE">
<![%phone.attlist;[
<!ATTLIST phone
		%common.attrib;
		%phone.role.attrib;
		%local.phone.attrib;
>
<!--end of phone.attlist-->]]>
  <!--end of phone.module-->]]>

  <!ENTITY % fax.module "INCLUDE">
  <![%fax.module;[
  <!ENTITY % local.fax.attrib "">
  <!ENTITY % fax.role.attrib "%role.attrib;">
  
<!ENTITY % fax.element "INCLUDE">
<![%fax.element;[
<!ELEMENT fax (%docinfo.char.mix;)*>
<!--end of fax.element-->]]>
  
<!ENTITY % fax.attlist "INCLUDE">
<![%fax.attlist;[
<!ATTLIST fax
		%common.attrib;
		%fax.role.attrib;
		%local.fax.attrib;
>
<!--end of fax.attlist-->]]>
  <!--end of fax.module-->]]>

  <!-- Email (defined in the Inlines section, below)-->

  <!ENTITY % otheraddr.module "INCLUDE">
  <![%otheraddr.module;[
  <!ENTITY % local.otheraddr.attrib "">
  <!ENTITY % otheraddr.role.attrib "%role.attrib;">
  
<!ENTITY % otheraddr.element "INCLUDE">
<![%otheraddr.element;[
<!ELEMENT otheraddr (%docinfo.char.mix;)*>
<!--end of otheraddr.element-->]]>
  
<!ENTITY % otheraddr.attlist "INCLUDE">
<![%otheraddr.attlist;[
<!ATTLIST otheraddr
		%common.attrib;
		%otheraddr.role.attrib;
		%local.otheraddr.attrib;
>
<!--end of otheraddr.attlist-->]]>
  <!--end of otheraddr.module-->]]>
<!--end of address.content.module-->]]>

<!-- Affiliation ...................... -->

<!ENTITY % affiliation.content.module "INCLUDE">
<![%affiliation.content.module;[
<!ENTITY % affiliation.module "INCLUDE">
<![%affiliation.module;[
<!ENTITY % local.affiliation.attrib "">
<!ENTITY % affiliation.role.attrib "%role.attrib;">

<!ENTITY % affiliation.element "INCLUDE">
<![%affiliation.element;[
<!ELEMENT affiliation (shortaffil?, jobtitle*, orgname?, orgdiv*,
		address*)>
<!--end of affiliation.element-->]]>

<!ENTITY % affiliation.attlist "INCLUDE">
<![%affiliation.attlist;[
<!ATTLIST affiliation
		%common.attrib;
		%affiliation.role.attrib;
		%local.affiliation.attrib;
>
<!--end of affiliation.attlist-->]]>
<!--end of affiliation.module-->]]>

  <!ENTITY % shortaffil.module "INCLUDE">
  <![%shortaffil.module;[
  <!ENTITY % local.shortaffil.attrib "">
  <!ENTITY % shortaffil.role.attrib "%role.attrib;">
  
<!ENTITY % shortaffil.element "INCLUDE">
<![%shortaffil.element;[
<!ELEMENT shortaffil (%docinfo.char.mix;)*>
<!--end of shortaffil.element-->]]>
  
<!ENTITY % shortaffil.attlist "INCLUDE">
<![%shortaffil.attlist;[
<!ATTLIST shortaffil
		%common.attrib;
		%shortaffil.role.attrib;
		%local.shortaffil.attrib;
>
<!--end of shortaffil.attlist-->]]>
  <!--end of shortaffil.module-->]]>

  <!ENTITY % jobtitle.module "INCLUDE">
  <![%jobtitle.module;[
  <!ENTITY % local.jobtitle.attrib "">
  <!ENTITY % jobtitle.role.attrib "%role.attrib;">
  
<!ENTITY % jobtitle.element "INCLUDE">
<![%jobtitle.element;[
<!ELEMENT jobtitle (%docinfo.char.mix;)*>
<!--end of jobtitle.element-->]]>
  
<!ENTITY % jobtitle.attlist "INCLUDE">
<![%jobtitle.attlist;[
<!ATTLIST jobtitle
		%common.attrib;
		%jobtitle.role.attrib;
		%local.jobtitle.attrib;
>
<!--end of jobtitle.attlist-->]]>
  <!--end of jobtitle.module-->]]>

  <!-- OrgName (defined elsewhere in this section)-->

  <!ENTITY % orgdiv.module "INCLUDE">
  <![%orgdiv.module;[
  <!ENTITY % local.orgdiv.attrib "">
  <!ENTITY % orgdiv.role.attrib "%role.attrib;">
  
<!ENTITY % orgdiv.element "INCLUDE">
<![%orgdiv.element;[
<!ELEMENT orgdiv (%docinfo.char.mix;)*>
<!--end of orgdiv.element-->]]>
  
<!ENTITY % orgdiv.attlist "INCLUDE">
<![%orgdiv.attlist;[
<!ATTLIST orgdiv
		%common.attrib;
		%orgdiv.role.attrib;
		%local.orgdiv.attrib;
>
<!--end of orgdiv.attlist-->]]>
  <!--end of orgdiv.module-->]]>

  <!-- Address (defined elsewhere in this section)-->
<!--end of affiliation.content.module-->]]>

<!-- ArtPageNums ...................... -->

<!ENTITY % artpagenums.module "INCLUDE">
<![%artpagenums.module;[
<!ENTITY % local.artpagenums.attrib "">
<!ENTITY % artpagenums.role.attrib "%role.attrib;">

<!ENTITY % artpagenums.element "INCLUDE">
<![%artpagenums.element;[
<!ELEMENT artpagenums (%docinfo.char.mix;)*>
<!--end of artpagenums.element-->]]>

<!ENTITY % artpagenums.attlist "INCLUDE">
<![%artpagenums.attlist;[
<!ATTLIST artpagenums
		%common.attrib;
		%artpagenums.role.attrib;
		%local.artpagenums.attrib;
>
<!--end of artpagenums.attlist-->]]>
<!--end of artpagenums.module-->]]>

<!-- Author ........................... -->

<!ENTITY % author.module "INCLUDE">
<![%author.module;[
<!ENTITY % local.author.attrib "">
<!ENTITY % author.role.attrib "%role.attrib;">

<!ENTITY % author.element "INCLUDE">
<![%author.element;[
<!ELEMENT author ((%person.ident.mix;)+)>
<!--end of author.element-->]]>

<!ENTITY % author.attlist "INCLUDE">
<![%author.attlist;[
<!ATTLIST author
		%common.attrib;
		%author.role.attrib;
		%local.author.attrib;
>
<!--end of author.attlist-->]]>
<!--(see "Personal identity elements" for %person.ident.mix;)-->
<!--end of author.module-->]]>

<!-- AuthorGroup ...................... -->

<!ENTITY % authorgroup.content.module "INCLUDE">
<![%authorgroup.content.module;[
<!ENTITY % authorgroup.module "INCLUDE">
<![%authorgroup.module;[
<!ENTITY % local.authorgroup.attrib "">
<!ENTITY % authorgroup.role.attrib "%role.attrib;">

<!ENTITY % authorgroup.element "INCLUDE">
<![%authorgroup.element;[
<!ELEMENT authorgroup ((author|editor|collab|corpauthor|othercredit)+)>
<!--end of authorgroup.element-->]]>

<!ENTITY % authorgroup.attlist "INCLUDE">
<![%authorgroup.attlist;[
<!ATTLIST authorgroup
		%common.attrib;
		%authorgroup.role.attrib;
		%local.authorgroup.attrib;
>
<!--end of authorgroup.attlist-->]]>
<!--end of authorgroup.module-->]]>

  <!-- Author (defined elsewhere in this section)-->
  <!-- Editor (defined elsewhere in this section)-->

  <!ENTITY % collab.content.module "INCLUDE">
  <![%collab.content.module;[
  <!ENTITY % collab.module "INCLUDE">
  <![%collab.module;[
  <!ENTITY % local.collab.attrib "">
  <!ENTITY % collab.role.attrib "%role.attrib;">
  
<!ENTITY % collab.element "INCLUDE">
<![%collab.element;[
<!ELEMENT collab (collabname, affiliation*)>
<!--end of collab.element-->]]>
  
<!ENTITY % collab.attlist "INCLUDE">
<![%collab.attlist;[
<!ATTLIST collab
		%common.attrib;
		%collab.role.attrib;
		%local.collab.attrib;
>
<!--end of collab.attlist-->]]>
  <!--end of collab.module-->]]>

    <!ENTITY % collabname.module "INCLUDE">
  <![%collabname.module;[
  <!ENTITY % local.collabname.attrib "">
  <!ENTITY % collabname.role.attrib "%role.attrib;">
    
<!ENTITY % collabname.element "INCLUDE">
<![%collabname.element;[
<!ELEMENT collabname (%docinfo.char.mix;)*>
<!--end of collabname.element-->]]>
    
<!ENTITY % collabname.attlist "INCLUDE">
<![%collabname.attlist;[
<!ATTLIST collabname
		%common.attrib;
		%collabname.role.attrib;
		%local.collabname.attrib;
>
<!--end of collabname.attlist-->]]>
    <!--end of collabname.module-->]]>

    <!-- Affiliation (defined elsewhere in this section)-->
  <!--end of collab.content.module-->]]>

  <!-- CorpAuthor (defined elsewhere in this section)-->
  <!-- OtherCredit (defined elsewhere in this section)-->

<!--end of authorgroup.content.module-->]]>

<!-- AuthorInitials ................... -->

<!ENTITY % authorinitials.module "INCLUDE">
<![%authorinitials.module;[
<!ENTITY % local.authorinitials.attrib "">
<!ENTITY % authorinitials.role.attrib "%role.attrib;">

<!ENTITY % authorinitials.element "INCLUDE">
<![%authorinitials.element;[
<!ELEMENT authorinitials (%docinfo.char.mix;)*>
<!--end of authorinitials.element-->]]>

<!ENTITY % authorinitials.attlist "INCLUDE">
<![%authorinitials.attlist;[
<!ATTLIST authorinitials
		%common.attrib;
		%authorinitials.role.attrib;
		%local.authorinitials.attrib;
>
<!--end of authorinitials.attlist-->]]>
<!--end of authorinitials.module-->]]>

<!-- ConfGroup ........................ -->

<!ENTITY % confgroup.content.module "INCLUDE">
<![%confgroup.content.module;[
<!ENTITY % confgroup.module "INCLUDE">
<![%confgroup.module;[
<!ENTITY % local.confgroup.attrib "">
<!ENTITY % confgroup.role.attrib "%role.attrib;">

<!ENTITY % confgroup.element "INCLUDE">
<![%confgroup.element;[
<!ELEMENT confgroup ((confdates|conftitle|confnum|address|confsponsor)*)>
<!--end of confgroup.element-->]]>

<!ENTITY % confgroup.attlist "INCLUDE">
<![%confgroup.attlist;[
<!ATTLIST confgroup
		%common.attrib;
		%confgroup.role.attrib;
		%local.confgroup.attrib;
>
<!--end of confgroup.attlist-->]]>
<!--end of confgroup.module-->]]>

  <!ENTITY % confdates.module "INCLUDE">
  <![%confdates.module;[
  <!ENTITY % local.confdates.attrib "">
  <!ENTITY % confdates.role.attrib "%role.attrib;">
  
<!ENTITY % confdates.element "INCLUDE">
<![%confdates.element;[
<!ELEMENT confdates (%docinfo.char.mix;)*>
<!--end of confdates.element-->]]>
  
<!ENTITY % confdates.attlist "INCLUDE">
<![%confdates.attlist;[
<!ATTLIST confdates
		%common.attrib;
		%confdates.role.attrib;
		%local.confdates.attrib;
>
<!--end of confdates.attlist-->]]>
  <!--end of confdates.module-->]]>

  <!ENTITY % conftitle.module "INCLUDE">
  <![%conftitle.module;[
  <!ENTITY % local.conftitle.attrib "">
  <!ENTITY % conftitle.role.attrib "%role.attrib;">
  
<!ENTITY % conftitle.element "INCLUDE">
<![%conftitle.element;[
<!ELEMENT conftitle (%docinfo.char.mix;)*>
<!--end of conftitle.element-->]]>
  
<!ENTITY % conftitle.attlist "INCLUDE">
<![%conftitle.attlist;[
<!ATTLIST conftitle
		%common.attrib;
		%conftitle.role.attrib;
		%local.conftitle.attrib;
>
<!--end of conftitle.attlist-->]]>
  <!--end of conftitle.module-->]]>

  <!ENTITY % confnum.module "INCLUDE">
  <![%confnum.module;[
  <!ENTITY % local.confnum.attrib "">
  <!ENTITY % confnum.role.attrib "%role.attrib;">
  
<!ENTITY % confnum.element "INCLUDE">
<![%confnum.element;[
<!ELEMENT confnum (%docinfo.char.mix;)*>
<!--end of confnum.element-->]]>
  
<!ENTITY % confnum.attlist "INCLUDE">
<![%confnum.attlist;[
<!ATTLIST confnum
		%common.attrib;
		%confnum.role.attrib;
		%local.confnum.attrib;
>
<!--end of confnum.attlist-->]]>
  <!--end of confnum.module-->]]>

  <!-- Address (defined elsewhere in this section)-->

  <!ENTITY % confsponsor.module "INCLUDE">
  <![%confsponsor.module;[
  <!ENTITY % local.confsponsor.attrib "">
  <!ENTITY % confsponsor.role.attrib "%role.attrib;">
  
<!ENTITY % confsponsor.element "INCLUDE">
<![%confsponsor.element;[
<!ELEMENT confsponsor (%docinfo.char.mix;)*>
<!--end of confsponsor.element-->]]>
  
<!ENTITY % confsponsor.attlist "INCLUDE">
<![%confsponsor.attlist;[
<!ATTLIST confsponsor
		%common.attrib;
		%confsponsor.role.attrib;
		%local.confsponsor.attrib;
>
<!--end of confsponsor.attlist-->]]>
  <!--end of confsponsor.module-->]]>
<!--end of confgroup.content.module-->]]>

<!-- ContractNum ...................... -->

<!ENTITY % contractnum.module "INCLUDE">
<![%contractnum.module;[
<!ENTITY % local.contractnum.attrib "">
<!ENTITY % contractnum.role.attrib "%role.attrib;">

<!ENTITY % contractnum.element "INCLUDE">
<![%contractnum.element;[
<!ELEMENT contractnum (%docinfo.char.mix;)*>
<!--end of contractnum.element-->]]>

<!ENTITY % contractnum.attlist "INCLUDE">
<![%contractnum.attlist;[
<!ATTLIST contractnum
		%common.attrib;
		%contractnum.role.attrib;
		%local.contractnum.attrib;
>
<!--end of contractnum.attlist-->]]>
<!--end of contractnum.module-->]]>

<!-- ContractSponsor .................. -->

<!ENTITY % contractsponsor.module "INCLUDE">
<![%contractsponsor.module;[
<!ENTITY % local.contractsponsor.attrib "">
<!ENTITY % contractsponsor.role.attrib "%role.attrib;">

<!ENTITY % contractsponsor.element "INCLUDE">
<![%contractsponsor.element;[
<!ELEMENT contractsponsor (%docinfo.char.mix;)*>
<!--end of contractsponsor.element-->]]>

<!ENTITY % contractsponsor.attlist "INCLUDE">
<![%contractsponsor.attlist;[
<!ATTLIST contractsponsor
		%common.attrib;
		%contractsponsor.role.attrib;
		%local.contractsponsor.attrib;
>
<!--end of contractsponsor.attlist-->]]>
<!--end of contractsponsor.module-->]]>

<!-- Copyright ........................ -->

<!ENTITY % copyright.content.module "INCLUDE">
<![%copyright.content.module;[
<!ENTITY % copyright.module "INCLUDE">
<![%copyright.module;[
<!ENTITY % local.copyright.attrib "">
<!ENTITY % copyright.role.attrib "%role.attrib;">

<!ENTITY % copyright.element "INCLUDE">
<![%copyright.element;[
<!ELEMENT copyright (year+, holder*)>
<!--end of copyright.element-->]]>

<!ENTITY % copyright.attlist "INCLUDE">
<![%copyright.attlist;[
<!ATTLIST copyright
		%common.attrib;
		%copyright.role.attrib;
		%local.copyright.attrib;
>
<!--end of copyright.attlist-->]]>
<!--end of copyright.module-->]]>

  <!ENTITY % year.module "INCLUDE">
  <![%year.module;[
  <!ENTITY % local.year.attrib "">
  <!ENTITY % year.role.attrib "%role.attrib;">
  
<!ENTITY % year.element "INCLUDE">
<![%year.element;[
<!ELEMENT year (%docinfo.char.mix;)*>
<!--end of year.element-->]]>
  
<!ENTITY % year.attlist "INCLUDE">
<![%year.attlist;[
<!ATTLIST year
		%common.attrib;
		%year.role.attrib;
		%local.year.attrib;
>
<!--end of year.attlist-->]]>
  <!--end of year.module-->]]>

  <!ENTITY % holder.module "INCLUDE">
  <![%holder.module;[
  <!ENTITY % local.holder.attrib "">
  <!ENTITY % holder.role.attrib "%role.attrib;">
  
<!ENTITY % holder.element "INCLUDE">
<![%holder.element;[
<!ELEMENT holder (%docinfo.char.mix;)*>
<!--end of holder.element-->]]>
  
<!ENTITY % holder.attlist "INCLUDE">
<![%holder.attlist;[
<!ATTLIST holder
		%common.attrib;
		%holder.role.attrib;
		%local.holder.attrib;
>
<!--end of holder.attlist-->]]>
  <!--end of holder.module-->]]>
<!--end of copyright.content.module-->]]>

<!-- CorpAuthor ....................... -->

<!ENTITY % corpauthor.module "INCLUDE">
<![%corpauthor.module;[
<!ENTITY % local.corpauthor.attrib "">
<!ENTITY % corpauthor.role.attrib "%role.attrib;">

<!ENTITY % corpauthor.element "INCLUDE">
<![%corpauthor.element;[
<!ELEMENT corpauthor (%docinfo.char.mix;)*>
<!--end of corpauthor.element-->]]>

<!ENTITY % corpauthor.attlist "INCLUDE">
<![%corpauthor.attlist;[
<!ATTLIST corpauthor
		%common.attrib;
		%corpauthor.role.attrib;
		%local.corpauthor.attrib;
>
<!--end of corpauthor.attlist-->]]>
<!--end of corpauthor.module-->]]>

<!-- CorpName ......................... -->

<!ENTITY % corpname.module "INCLUDE">
<![%corpname.module;[
<!ENTITY % local.corpname.attrib "">

<!ENTITY % corpname.element "INCLUDE">
<![%corpname.element;[
<!ELEMENT corpname (%docinfo.char.mix;)*>
<!--end of corpname.element-->]]>
<!ENTITY % corpname.role.attrib "%role.attrib;">

<!ENTITY % corpname.attlist "INCLUDE">
<![%corpname.attlist;[
<!ATTLIST corpname
		%common.attrib;
		%corpname.role.attrib;
		%local.corpname.attrib;
>
<!--end of corpname.attlist-->]]>
<!--end of corpname.module-->]]>

<!-- Date ............................. -->

<!ENTITY % date.module "INCLUDE">
<![%date.module;[
<!ENTITY % local.date.attrib "">
<!ENTITY % date.role.attrib "%role.attrib;">

<!ENTITY % date.element "INCLUDE">
<![%date.element;[
<!ELEMENT date (%docinfo.char.mix;)*>
<!--end of date.element-->]]>

<!ENTITY % date.attlist "INCLUDE">
<![%date.attlist;[
<!ATTLIST date
		%common.attrib;
		%date.role.attrib;
		%local.date.attrib;
>
<!--end of date.attlist-->]]>
<!--end of date.module-->]]>

<!-- Edition .......................... -->

<!ENTITY % edition.module "INCLUDE">
<![%edition.module;[
<!ENTITY % local.edition.attrib "">
<!ENTITY % edition.role.attrib "%role.attrib;">

<!ENTITY % edition.element "INCLUDE">
<![%edition.element;[
<!ELEMENT edition (%docinfo.char.mix;)*>
<!--end of edition.element-->]]>

<!ENTITY % edition.attlist "INCLUDE">
<![%edition.attlist;[
<!ATTLIST edition
		%common.attrib;
		%edition.role.attrib;
		%local.edition.attrib;
>
<!--end of edition.attlist-->]]>
<!--end of edition.module-->]]>

<!-- Editor ........................... -->

<!ENTITY % editor.module "INCLUDE">
<![%editor.module;[
<!ENTITY % local.editor.attrib "">
<!ENTITY % editor.role.attrib "%role.attrib;">

<!ENTITY % editor.element "INCLUDE">
<![%editor.element;[
<!ELEMENT editor ((%person.ident.mix;)+)>
<!--end of editor.element-->]]>

<!ENTITY % editor.attlist "INCLUDE">
<![%editor.attlist;[
<!ATTLIST editor
		%common.attrib;
		%editor.role.attrib;
		%local.editor.attrib;
>
<!--end of editor.attlist-->]]>
  <!--(see "Personal identity elements" for %person.ident.mix;)-->
<!--end of editor.module-->]]>

<!-- ISBN ............................. -->

<!ENTITY % isbn.module "INCLUDE">
<![%isbn.module;[
<!ENTITY % local.isbn.attrib "">
<!ENTITY % isbn.role.attrib "%role.attrib;">

<!ENTITY % isbn.element "INCLUDE">
<![%isbn.element;[
<!ELEMENT isbn (%docinfo.char.mix;)*>
<!--end of isbn.element-->]]>

<!ENTITY % isbn.attlist "INCLUDE">
<![%isbn.attlist;[
<!ATTLIST isbn
		%common.attrib;
		%isbn.role.attrib;
		%local.isbn.attrib;
>
<!--end of isbn.attlist-->]]>
<!--end of isbn.module-->]]>

<!-- ISSN ............................. -->

<!ENTITY % issn.module "INCLUDE">
<![%issn.module;[
<!ENTITY % local.issn.attrib "">
<!ENTITY % issn.role.attrib "%role.attrib;">

<!ENTITY % issn.element "INCLUDE">
<![%issn.element;[
<!ELEMENT issn (%docinfo.char.mix;)*>
<!--end of issn.element-->]]>

<!ENTITY % issn.attlist "INCLUDE">
<![%issn.attlist;[
<!ATTLIST issn
		%common.attrib;
		%issn.role.attrib;
		%local.issn.attrib;
>
<!--end of issn.attlist-->]]>
<!--end of issn.module-->]]>

<!-- InvPartNumber .................... -->

<!ENTITY % invpartnumber.module "INCLUDE">
<![%invpartnumber.module;[
<!ENTITY % local.invpartnumber.attrib "">
<!ENTITY % invpartnumber.role.attrib "%role.attrib;">

<!ENTITY % invpartnumber.element "INCLUDE">
<![%invpartnumber.element;[
<!ELEMENT invpartnumber (%docinfo.char.mix;)*>
<!--end of invpartnumber.element-->]]>

<!ENTITY % invpartnumber.attlist "INCLUDE">
<![%invpartnumber.attlist;[
<!ATTLIST invpartnumber
		%common.attrib;
		%invpartnumber.role.attrib;
		%local.invpartnumber.attrib;
>
<!--end of invpartnumber.attlist-->]]>
<!--end of invpartnumber.module-->]]>

<!-- IssueNum ......................... -->

<!ENTITY % issuenum.module "INCLUDE">
<![%issuenum.module;[
<!ENTITY % local.issuenum.attrib "">
<!ENTITY % issuenum.role.attrib "%role.attrib;">

<!ENTITY % issuenum.element "INCLUDE">
<![%issuenum.element;[
<!ELEMENT issuenum (%docinfo.char.mix;)*>
<!--end of issuenum.element-->]]>

<!ENTITY % issuenum.attlist "INCLUDE">
<![%issuenum.attlist;[
<!ATTLIST issuenum
		%common.attrib;
		%issuenum.role.attrib;
		%local.issuenum.attrib;
>
<!--end of issuenum.attlist-->]]>
<!--end of issuenum.module-->]]>

<!-- LegalNotice ...................... -->

<!ENTITY % legalnotice.module "INCLUDE">
<![%legalnotice.module;[
<!ENTITY % local.legalnotice.attrib "">
<!ENTITY % legalnotice.role.attrib "%role.attrib;">

<!ENTITY % legalnotice.element "INCLUDE">
<![%legalnotice.element;[
<!ELEMENT legalnotice (title?, (%legalnotice.mix;)+)>
<!--end of legalnotice.element-->]]>

<!ENTITY % legalnotice.attlist "INCLUDE">
<![%legalnotice.attlist;[
<!ATTLIST legalnotice
		%common.attrib;
		%legalnotice.role.attrib;
		%local.legalnotice.attrib;
>
<!--end of legalnotice.attlist-->]]>
<!--end of legalnotice.module-->]]>

<!-- ModeSpec ......................... -->

<!ENTITY % modespec.module "INCLUDE">
<![%modespec.module;[
<!ENTITY % local.modespec.attrib "">
<!ENTITY % modespec.role.attrib "%role.attrib;">

<!ENTITY % modespec.element "INCLUDE">
<![%modespec.element;[
<!ELEMENT modespec (%docinfo.char.mix;)*>
<!--end of modespec.element-->]]>

<!-- Application: Type of action required for completion
		of the links to which the ModeSpec is relevant (e.g.,
		retrieval query) -->


<!ENTITY % modespec.attlist "INCLUDE">
<![%modespec.attlist;[
<!ATTLIST modespec
		application	NOTATION
				(%notation.class;)	#IMPLIED
		%common.attrib;
		%modespec.role.attrib;
		%local.modespec.attrib;
>
<!--end of modespec.attlist-->]]>
<!--end of modespec.module-->]]>

<!-- OrgName .......................... -->

<!ENTITY % orgname.module "INCLUDE">
<![%orgname.module;[
<!ENTITY % local.orgname.attrib "">
<!ENTITY % orgname.role.attrib "%role.attrib;">

<!ENTITY % orgname.element "INCLUDE">
<![%orgname.element;[
<!ELEMENT orgname (%docinfo.char.mix;)*>
<!--end of orgname.element-->]]>

<!ENTITY % orgname.attlist "INCLUDE">
<![%orgname.attlist;[
<!ATTLIST orgname
		%common.attrib;
		%orgname.role.attrib;
		%local.orgname.attrib;
>
<!--end of orgname.attlist-->]]>
<!--end of orgname.module-->]]>

<!-- OtherCredit ...................... -->

<!ENTITY % othercredit.module "INCLUDE">
<![%othercredit.module;[
<!ENTITY % local.othercredit.attrib "">
<!ENTITY % othercredit.role.attrib "%role.attrib;">

<!ENTITY % othercredit.element "INCLUDE">
<![%othercredit.element;[
<!ELEMENT othercredit ((%person.ident.mix;)+)>
<!--end of othercredit.element-->]]>

<!ENTITY % othercredit.attlist "INCLUDE">
<![%othercredit.attlist;[
<!ATTLIST othercredit
		%common.attrib;
		%othercredit.role.attrib;
		%local.othercredit.attrib;
>
<!--end of othercredit.attlist-->]]>
  <!--(see "Personal identity elements" for %person.ident.mix;)-->
<!--end of othercredit.module-->]]>

<!-- PageNums ......................... -->

<!ENTITY % pagenums.module "INCLUDE">
<![%pagenums.module;[
<!ENTITY % local.pagenums.attrib "">
<!ENTITY % pagenums.role.attrib "%role.attrib;">

<!ENTITY % pagenums.element "INCLUDE">
<![%pagenums.element;[
<!ELEMENT pagenums (%docinfo.char.mix;)*>
<!--end of pagenums.element-->]]>

<!ENTITY % pagenums.attlist "INCLUDE">
<![%pagenums.attlist;[
<!ATTLIST pagenums
		%common.attrib;
		%pagenums.role.attrib;
		%local.pagenums.attrib;
>
<!--end of pagenums.attlist-->]]>
<!--end of pagenums.module-->]]>

<!-- Personal identity elements ....... -->

<!-- These elements are used only within Author, Editor, and 
OtherCredit. -->

<!ENTITY % person.ident.module "INCLUDE">
<![%person.ident.module;[
  <!ENTITY % contrib.module "INCLUDE">
  <![%contrib.module;[
  <!ENTITY % local.contrib.attrib "">
  <!ENTITY % contrib.role.attrib "%role.attrib;">
  
<!ENTITY % contrib.element "INCLUDE">
<![%contrib.element;[
<!ELEMENT contrib (%docinfo.char.mix;)*>
<!--end of contrib.element-->]]>
  
<!ENTITY % contrib.attlist "INCLUDE">
<![%contrib.attlist;[
<!ATTLIST contrib
		%common.attrib;
		%contrib.role.attrib;
		%local.contrib.attrib;
>
<!--end of contrib.attlist-->]]>
  <!--end of contrib.module-->]]>

  <!ENTITY % firstname.module "INCLUDE">
  <![%firstname.module;[
  <!ENTITY % local.firstname.attrib "">
  <!ENTITY % firstname.role.attrib "%role.attrib;">
  
<!ENTITY % firstname.element "INCLUDE">
<![%firstname.element;[
<!ELEMENT firstname (%docinfo.char.mix;)*>
<!--end of firstname.element-->]]>
  
<!ENTITY % firstname.attlist "INCLUDE">
<![%firstname.attlist;[
<!ATTLIST firstname
		%common.attrib;
		%firstname.role.attrib;
		%local.firstname.attrib;
>
<!--end of firstname.attlist-->]]>
  <!--end of firstname.module-->]]>

  <!ENTITY % honorific.module "INCLUDE">
  <![%honorific.module;[
  <!ENTITY % local.honorific.attrib "">
  <!ENTITY % honorific.role.attrib "%role.attrib;">
  
<!ENTITY % honorific.element "INCLUDE">
<![%honorific.element;[
<!ELEMENT honorific (%docinfo.char.mix;)*>
<!--end of honorific.element-->]]>
  
<!ENTITY % honorific.attlist "INCLUDE">
<![%honorific.attlist;[
<!ATTLIST honorific
		%common.attrib;
		%honorific.role.attrib;
		%local.honorific.attrib;
>
<!--end of honorific.attlist-->]]>
  <!--end of honorific.module-->]]>

  <!ENTITY % lineage.module "INCLUDE">
  <![%lineage.module;[
  <!ENTITY % local.lineage.attrib "">
  <!ENTITY % lineage.role.attrib "%role.attrib;">
  
<!ENTITY % lineage.element "INCLUDE">
<![%lineage.element;[
<!ELEMENT lineage (%docinfo.char.mix;)*>
<!--end of lineage.element-->]]>
  
<!ENTITY % lineage.attlist "INCLUDE">
<![%lineage.attlist;[
<!ATTLIST lineage
		%common.attrib;
		%lineage.role.attrib;
		%local.lineage.attrib;
>
<!--end of lineage.attlist-->]]>
  <!--end of lineage.module-->]]>

  <!ENTITY % othername.module "INCLUDE">
  <![%othername.module;[
  <!ENTITY % local.othername.attrib "">
  <!ENTITY % othername.role.attrib "%role.attrib;">
  
<!ENTITY % othername.element "INCLUDE">
<![%othername.element;[
<!ELEMENT othername (%docinfo.char.mix;)*>
<!--end of othername.element-->]]>
  
<!ENTITY % othername.attlist "INCLUDE">
<![%othername.attlist;[
<!ATTLIST othername
		%common.attrib;
		%othername.role.attrib;
		%local.othername.attrib;
>
<!--end of othername.attlist-->]]>
  <!--end of othername.module-->]]>

  <!ENTITY % surname.module "INCLUDE">
  <![%surname.module;[
  <!ENTITY % local.surname.attrib "">
  <!ENTITY % surname.role.attrib "%role.attrib;">
  
<!ENTITY % surname.element "INCLUDE">
<![%surname.element;[
<!ELEMENT surname (%docinfo.char.mix;)*>
<!--end of surname.element-->]]>
  
<!ENTITY % surname.attlist "INCLUDE">
<![%surname.attlist;[
<!ATTLIST surname
		%common.attrib;
		%surname.role.attrib;
		%local.surname.attrib;
>
<!--end of surname.attlist-->]]>
  <!--end of surname.module-->]]>
<!--end of person.ident.module-->]]>

<!-- PrintHistory ..................... -->

<!ENTITY % printhistory.module "INCLUDE">
<![%printhistory.module;[
<!ENTITY % local.printhistory.attrib "">
<!ENTITY % printhistory.role.attrib "%role.attrib;">

<!ENTITY % printhistory.element "INCLUDE">
<![%printhistory.element;[
<!ELEMENT printhistory ((%para.class;)+)>
<!--end of printhistory.element-->]]>

<!ENTITY % printhistory.attlist "INCLUDE">
<![%printhistory.attlist;[
<!ATTLIST printhistory
		%common.attrib;
		%printhistory.role.attrib;
		%local.printhistory.attrib;
>
<!--end of printhistory.attlist-->]]>
<!--end of printhistory.module-->]]>

<!-- ProductName ...................... -->

<!ENTITY % productname.module "INCLUDE">
<![%productname.module;[
<!ENTITY % local.productname.attrib "">
<!ENTITY % productname.role.attrib "%role.attrib;">

<!ENTITY % productname.element "INCLUDE">
<![%productname.element;[
<!ELEMENT productname (%para.char.mix;)*>
<!--end of productname.element-->]]>

<!-- Class: More precisely identifies the item the element names -->


<!ENTITY % productname.attlist "INCLUDE">
<![%productname.attlist;[
<!ATTLIST productname
		class		(service
				|trade
				|registered
				|copyright)	'trade'
		%common.attrib;
		%productname.role.attrib;
		%local.productname.attrib;
>
<!--end of productname.attlist-->]]>
<!--end of productname.module-->]]>

<!-- ProductNumber .................... -->

<!ENTITY % productnumber.module "INCLUDE">
<![%productnumber.module;[
<!ENTITY % local.productnumber.attrib "">
<!ENTITY % productnumber.role.attrib "%role.attrib;">

<!ENTITY % productnumber.element "INCLUDE">
<![%productnumber.element;[
<!ELEMENT productnumber (%docinfo.char.mix;)*>
<!--end of productnumber.element-->]]>

<!ENTITY % productnumber.attlist "INCLUDE">
<![%productnumber.attlist;[
<!ATTLIST productnumber
		%common.attrib;
		%productnumber.role.attrib;
		%local.productnumber.attrib;
>
<!--end of productnumber.attlist-->]]>
<!--end of productnumber.module-->]]>

<!-- PubDate .......................... -->

<!ENTITY % pubdate.module "INCLUDE">
<![%pubdate.module;[
<!ENTITY % local.pubdate.attrib "">
<!ENTITY % pubdate.role.attrib "%role.attrib;">

<!ENTITY % pubdate.element "INCLUDE">
<![%pubdate.element;[
<!ELEMENT pubdate (%docinfo.char.mix;)*>
<!--end of pubdate.element-->]]>

<!ENTITY % pubdate.attlist "INCLUDE">
<![%pubdate.attlist;[
<!ATTLIST pubdate
		%common.attrib;
		%pubdate.role.attrib;
		%local.pubdate.attrib;
>
<!--end of pubdate.attlist-->]]>
<!--end of pubdate.module-->]]>

<!-- Publisher ........................ -->

<!ENTITY % publisher.content.module "INCLUDE">
<![%publisher.content.module;[
<!ENTITY % publisher.module "INCLUDE">
<![%publisher.module;[
<!ENTITY % local.publisher.attrib "">
<!ENTITY % publisher.role.attrib "%role.attrib;">

<!ENTITY % publisher.element "INCLUDE">
<![%publisher.element;[
<!ELEMENT publisher (publishername, address*)>
<!--end of publisher.element-->]]>

<!ENTITY % publisher.attlist "INCLUDE">
<![%publisher.attlist;[
<!ATTLIST publisher
		%common.attrib;
		%publisher.role.attrib;
		%local.publisher.attrib;
>
<!--end of publisher.attlist-->]]>
<!--end of publisher.module-->]]>

  <!ENTITY % publishername.module "INCLUDE">
  <![%publishername.module;[
  <!ENTITY % local.publishername.attrib "">
  <!ENTITY % publishername.role.attrib "%role.attrib;">
  
<!ENTITY % publishername.element "INCLUDE">
<![%publishername.element;[
<!ELEMENT publishername (%docinfo.char.mix;)*>
<!--end of publishername.element-->]]>
  
<!ENTITY % publishername.attlist "INCLUDE">
<![%publishername.attlist;[
<!ATTLIST publishername
		%common.attrib;
		%publishername.role.attrib;
		%local.publishername.attrib;
>
<!--end of publishername.attlist-->]]>
  <!--end of publishername.module-->]]>

  <!-- Address (defined elsewhere in this section)-->
<!--end of publisher.content.module-->]]>

<!-- PubsNumber ....................... -->

<!ENTITY % pubsnumber.module "INCLUDE">
<![%pubsnumber.module;[
<!ENTITY % local.pubsnumber.attrib "">
<!ENTITY % pubsnumber.role.attrib "%role.attrib;">

<!ENTITY % pubsnumber.element "INCLUDE">
<![%pubsnumber.element;[
<!ELEMENT pubsnumber (%docinfo.char.mix;)*>
<!--end of pubsnumber.element-->]]>

<!ENTITY % pubsnumber.attlist "INCLUDE">
<![%pubsnumber.attlist;[
<!ATTLIST pubsnumber
		%common.attrib;
		%pubsnumber.role.attrib;
		%local.pubsnumber.attrib;
>
<!--end of pubsnumber.attlist-->]]>
<!--end of pubsnumber.module-->]]>

<!-- ReleaseInfo ...................... -->

<!ENTITY % releaseinfo.module "INCLUDE">
<![%releaseinfo.module;[
<!ENTITY % local.releaseinfo.attrib "">
<!ENTITY % releaseinfo.role.attrib "%role.attrib;">

<!ENTITY % releaseinfo.element "INCLUDE">
<![%releaseinfo.element;[
<!ELEMENT releaseinfo (%docinfo.char.mix;)*>
<!--end of releaseinfo.element-->]]>

<!ENTITY % releaseinfo.attlist "INCLUDE">
<![%releaseinfo.attlist;[
<!ATTLIST releaseinfo
		%common.attrib;
		%releaseinfo.role.attrib;
		%local.releaseinfo.attrib;
>
<!--end of releaseinfo.attlist-->]]>
<!--end of releaseinfo.module-->]]>

<!-- RevHistory ....................... -->

<!ENTITY % revhistory.content.module "INCLUDE">
<![%revhistory.content.module;[
<!ENTITY % revhistory.module "INCLUDE">
<![%revhistory.module;[
<!ENTITY % local.revhistory.attrib "">
<!ENTITY % revhistory.role.attrib "%role.attrib;">

<!ENTITY % revhistory.element "INCLUDE">
<![%revhistory.element;[
<!ELEMENT revhistory (revision+)>
<!--end of revhistory.element-->]]>

<!ENTITY % revhistory.attlist "INCLUDE">
<![%revhistory.attlist;[
<!ATTLIST revhistory
		%common.attrib;
		%revhistory.role.attrib;
		%local.revhistory.attrib;
>
<!--end of revhistory.attlist-->]]>
<!--end of revhistory.module-->]]>

<!ENTITY % revision.module "INCLUDE">
<![%revision.module;[
<!ENTITY % local.revision.attrib "">
<!ENTITY % revision.role.attrib "%role.attrib;">

<!ENTITY % revision.element "INCLUDE">
<![%revision.element;[
<!ELEMENT revision (revnumber, date, authorinitials*, 
                    (revremark|revdescription)?)>
<!--end of revision.element-->]]>

<!ENTITY % revision.attlist "INCLUDE">
<![%revision.attlist;[
<!ATTLIST revision
		%common.attrib;
		%revision.role.attrib;
		%local.revision.attrib;
>
<!--end of revision.attlist-->]]>
<!--end of revision.module-->]]>

<!ENTITY % revnumber.module "INCLUDE">
<![%revnumber.module;[
<!ENTITY % local.revnumber.attrib "">
<!ENTITY % revnumber.role.attrib "%role.attrib;">

<!ENTITY % revnumber.element "INCLUDE">
<![%revnumber.element;[
<!ELEMENT revnumber (%docinfo.char.mix;)*>
<!--end of revnumber.element-->]]>

<!ENTITY % revnumber.attlist "INCLUDE">
<![%revnumber.attlist;[
<!ATTLIST revnumber
		%common.attrib;
		%revnumber.role.attrib;
		%local.revnumber.attrib;
>
<!--end of revnumber.attlist-->]]>
<!--end of revnumber.module-->]]>

<!-- Date (defined elsewhere in this section)-->
<!-- AuthorInitials (defined elsewhere in this section)-->

<!ENTITY % revremark.module "INCLUDE">
<![%revremark.module;[
<!ENTITY % local.revremark.attrib "">
<!ENTITY % revremark.role.attrib "%role.attrib;">

<!ENTITY % revremark.element "INCLUDE">
<![%revremark.element;[
<!ELEMENT revremark (%docinfo.char.mix;)*>
<!--end of revremark.element-->]]>

<!ENTITY % revremark.attlist "INCLUDE">
<![%revremark.attlist;[
<!ATTLIST revremark
		%common.attrib;
		%revremark.role.attrib;
		%local.revremark.attrib;
>
<!--end of revremark.attlist-->]]>
<!--end of revremark.module-->]]>

<!ENTITY % revdescription.module "INCLUDE">
<![ %revdescription.module; [
<!ENTITY % local.revdescription.attrib "">
<!ENTITY % revdescription.role.attrib "%role.attrib;">

<!ENTITY % revdescription.element "INCLUDE">
<![ %revdescription.element; [
<!ELEMENT revdescription ((%revdescription.mix;)+)>
<!--end of revdescription.element-->]]>

<!ENTITY % revdescription.attlist "INCLUDE">
<![ %revdescription.attlist; [
<!ATTLIST revdescription
		%common.attrib;
		%revdescription.role.attrib;
		%local.revdescription.attrib;
>
<!--end of revdescription.attlist-->]]>
<!--end of revdescription.module-->]]>
<!--end of revhistory.content.module-->]]>

<!-- SeriesVolNums .................... -->

<!ENTITY % seriesvolnums.module "INCLUDE">
<![%seriesvolnums.module;[
<!ENTITY % local.seriesvolnums.attrib "">
<!ENTITY % seriesvolnums.role.attrib "%role.attrib;">

<!ENTITY % seriesvolnums.element "INCLUDE">
<![%seriesvolnums.element;[
<!ELEMENT seriesvolnums (%docinfo.char.mix;)*>
<!--end of seriesvolnums.element-->]]>

<!ENTITY % seriesvolnums.attlist "INCLUDE">
<![%seriesvolnums.attlist;[
<!ATTLIST seriesvolnums
		%common.attrib;
		%seriesvolnums.role.attrib;
		%local.seriesvolnums.attrib;
>
<!--end of seriesvolnums.attlist-->]]>
<!--end of seriesvolnums.module-->]]>

<!-- VolumeNum ........................ -->

<!ENTITY % volumenum.module "INCLUDE">
<![%volumenum.module;[
<!ENTITY % local.volumenum.attrib "">
<!ENTITY % volumenum.role.attrib "%role.attrib;">

<!ENTITY % volumenum.element "INCLUDE">
<![%volumenum.element;[
<!ELEMENT volumenum (%docinfo.char.mix;)*>
<!--end of volumenum.element-->]]>

<!ENTITY % volumenum.attlist "INCLUDE">
<![%volumenum.attlist;[
<!ATTLIST volumenum
		%common.attrib;
		%volumenum.role.attrib;
		%local.volumenum.attrib;
>
<!--end of volumenum.attlist-->]]>
<!--end of volumenum.module-->]]>

<!-- .................................. -->

<!--end of docinfo.content.module-->]]>

<!-- ...................................................................... -->
<!-- Inline, link, and ubiquitous elements ................................ -->

<!-- Technical and computer terms ......................................... -->

<!ENTITY % accel.module "INCLUDE">
<![%accel.module;[
<!ENTITY % local.accel.attrib "">
<!ENTITY % accel.role.attrib "%role.attrib;">

<!ENTITY % accel.element "INCLUDE">
<![%accel.element;[
<!ELEMENT accel (%smallcptr.char.mix;)*>
<!--end of accel.element-->]]>

<!ENTITY % accel.attlist "INCLUDE">
<![%accel.attlist;[
<!ATTLIST accel
		%common.attrib;
		%accel.role.attrib;
		%local.accel.attrib;
>
<!--end of accel.attlist-->]]>
<!--end of accel.module-->]]>

<!ENTITY % action.module "INCLUDE">
<![%action.module;[
<!ENTITY % local.action.attrib "">
<!ENTITY % action.role.attrib "%role.attrib;">

<!ENTITY % action.element "INCLUDE">
<![%action.element;[
<!ELEMENT action (%smallcptr.char.mix;)*>
<!--end of action.element-->]]>

<!ENTITY % action.attlist "INCLUDE">
<![%action.attlist;[
<!ATTLIST action
		%moreinfo.attrib;
		%common.attrib;
		%action.role.attrib;
		%local.action.attrib;
>
<!--end of action.attlist-->]]>
<!--end of action.module-->]]>

<!ENTITY % application.module "INCLUDE">
<![%application.module;[
<!ENTITY % local.application.attrib "">
<!ENTITY % application.role.attrib "%role.attrib;">

<!ENTITY % application.element "INCLUDE">
<![%application.element;[
<!ELEMENT application (%para.char.mix;)*>
<!--end of application.element-->]]>

<!ENTITY % application.attlist "INCLUDE">
<![%application.attlist;[
<!ATTLIST application
		class 		(hardware
				|software)	#IMPLIED
		%moreinfo.attrib;
		%common.attrib;
		%application.role.attrib;
		%local.application.attrib;
>
<!--end of application.attlist-->]]>
<!--end of application.module-->]]>

<!ENTITY % classname.module "INCLUDE">
<![%classname.module;[
<!ENTITY % local.classname.attrib "">
<!ENTITY % classname.role.attrib "%role.attrib;">

<!ENTITY % classname.element "INCLUDE">
<![%classname.element;[
<!ELEMENT classname (%smallcptr.char.mix;)*>
<!--end of classname.element-->]]>

<!ENTITY % classname.attlist "INCLUDE">
<![%classname.attlist;[
<!ATTLIST classname
		%common.attrib;
		%classname.role.attrib;
		%local.classname.attrib;
>
<!--end of classname.attlist-->]]>
<!--end of classname.module-->]]>

<!ENTITY % co.module "INCLUDE">
<![%co.module;[
<!ENTITY % local.co.attrib "">
<!-- CO is a callout area of the LineColumn unit type (a single character 
     position); the position is directly indicated by the location of CO. -->
<!ENTITY % co.role.attrib "%role.attrib;">

<!ENTITY % co.element "INCLUDE">
<![%co.element;[
<!ELEMENT co EMPTY>
<!--end of co.element-->]]>

<!-- bug number/symbol override or initialization -->
<!-- to any related information -->


<!ENTITY % co.attlist "INCLUDE">
<![%co.attlist;[
<!ATTLIST co
		%label.attrib;
		%linkends.attrib;
		%idreq.common.attrib;
		%co.role.attrib;
		%local.co.attrib;
>
<!--end of co.attlist-->]]>
<!--end of co.module-->]]>

<!ENTITY % command.module "INCLUDE">
<![%command.module;[
<!ENTITY % local.command.attrib "">
<!ENTITY % command.role.attrib "%role.attrib;">

<!ENTITY % command.element "INCLUDE">
<![%command.element;[
<!ELEMENT command (%cptr.char.mix;)*>
<!--end of command.element-->]]>

<!ENTITY % command.attlist "INCLUDE">
<![%command.attlist;[
<!ATTLIST command
		%moreinfo.attrib;
		%common.attrib;
		%command.role.attrib;
		%local.command.attrib;
>
<!--end of command.attlist-->]]>
<!--end of command.module-->]]>

<!ENTITY % computeroutput.module "INCLUDE">
<![%computeroutput.module;[
<!ENTITY % local.computeroutput.attrib "">
<!ENTITY % computeroutput.role.attrib "%role.attrib;">

<!ENTITY % computeroutput.element "INCLUDE">
<![%computeroutput.element;[
<!ELEMENT computeroutput (%cptr.char.mix;)*>
<!--end of computeroutput.element-->]]>

<!ENTITY % computeroutput.attlist "INCLUDE">
<![%computeroutput.attlist;[
<!ATTLIST computeroutput
		%moreinfo.attrib;
		%common.attrib;
		%computeroutput.role.attrib;
		%local.computeroutput.attrib;
>
<!--end of computeroutput.attlist-->]]>
<!--end of computeroutput.module-->]]>

<!ENTITY % database.module "INCLUDE">
<![%database.module;[
<!ENTITY % local.database.attrib "">
<!ENTITY % database.role.attrib "%role.attrib;">

<!ENTITY % database.element "INCLUDE">
<![%database.element;[
<!ELEMENT database (%smallcptr.char.mix;)*>
<!--end of database.element-->]]>

<!-- Class: Type of database the element names; no default -->


<!ENTITY % database.attlist "INCLUDE">
<![%database.attlist;[
<!ATTLIST database
		class 		(name
				|table
				|field
				|key1
				|key2
				|record)	#IMPLIED
		%moreinfo.attrib;
		%common.attrib;
		%database.role.attrib;
		%local.database.attrib;
>
<!--end of database.attlist-->]]>
<!--end of database.module-->]]>

<!ENTITY % email.module "INCLUDE">
<![%email.module;[
<!ENTITY % local.email.attrib "">
<!ENTITY % email.role.attrib "%role.attrib;">

<!ENTITY % email.element "INCLUDE">
<![%email.element;[
<!ELEMENT email (%docinfo.char.mix;)*>
<!--end of email.element-->]]>

<!ENTITY % email.attlist "INCLUDE">
<![%email.attlist;[
<!ATTLIST email
		%common.attrib;
		%email.role.attrib;
		%local.email.attrib;
>
<!--end of email.attlist-->]]>
<!--end of email.module-->]]>

<!ENTITY % envar.module "INCLUDE">
<![%envar.module;[
<!ENTITY % local.envar.attrib "">
<!ENTITY % envar.role.attrib "%role.attrib;">

<!ENTITY % envar.element "INCLUDE">
<![%envar.element;[
<!ELEMENT envar (%smallcptr.char.mix;)*>
<!--end of envar.element-->]]>

<!ENTITY % envar.attlist "INCLUDE">
<![%envar.attlist;[
<!ATTLIST envar
		%common.attrib;
		%envar.role.attrib;
		%local.envar.attrib;
>
<!--end of envar.attlist-->]]>
<!--end of envar.module-->]]>


<!ENTITY % errorcode.module "INCLUDE">
<![%errorcode.module;[
<!ENTITY % local.errorcode.attrib "">
<!ENTITY % errorcode.role.attrib "%role.attrib;">

<!ENTITY % errorcode.element "INCLUDE">
<![%errorcode.element;[
<!ELEMENT errorcode (%smallcptr.char.mix;)*>
<!--end of errorcode.element-->]]>

<!ENTITY % errorcode.attlist "INCLUDE">
<![%errorcode.attlist;[
<!ATTLIST errorcode
		%moreinfo.attrib;
		%common.attrib;
		%errorcode.role.attrib;
		%local.errorcode.attrib;
>
<!--end of errorcode.attlist-->]]>
<!--end of errorcode.module-->]]>

<!ENTITY % errorname.module "INCLUDE">
<![%errorname.module;[
<!ENTITY % local.errorname.attrib "">
<!ENTITY % errorname.role.attrib "%role.attrib;">

<!ENTITY % errorname.element "INCLUDE">
<![%errorname.element;[
<!ELEMENT errorname (%smallcptr.char.mix;)*>
<!--end of errorname.element-->]]>

<!ENTITY % errorname.attlist "INCLUDE">
<![%errorname.attlist;[
<!ATTLIST errorname
		%common.attrib;
		%errorname.role.attrib;
		%local.errorname.attrib;
>
<!--end of errorname.attlist-->]]>
<!--end of errorname.module-->]]>

<!ENTITY % errortype.module "INCLUDE">
<![%errortype.module;[
<!ENTITY % local.errortype.attrib "">
<!ENTITY % errortype.role.attrib "%role.attrib;">

<!ENTITY % errortype.element "INCLUDE">
<![%errortype.element;[
<!ELEMENT errortype (%smallcptr.char.mix;)*>
<!--end of errortype.element-->]]>

<!ENTITY % errortype.attlist "INCLUDE">
<![%errortype.attlist;[
<!ATTLIST errortype
		%common.attrib;
		%errortype.role.attrib;
		%local.errortype.attrib;
>
<!--end of errortype.attlist-->]]>
<!--end of errortype.module-->]]>

<!ENTITY % filename.module "INCLUDE">
<![%filename.module;[
<!ENTITY % local.filename.attrib "">
<!ENTITY % filename.role.attrib "%role.attrib;">

<!ENTITY % filename.element "INCLUDE">
<![%filename.element;[
<!ELEMENT filename (%smallcptr.char.mix;)*>
<!--end of filename.element-->]]>

<!-- Class: Type of filename the element names; no default -->
<!-- Path: Search path (possibly system-specific) in which 
		file can be found -->


<!ENTITY % filename.attlist "INCLUDE">
<![%filename.attlist;[
<!ATTLIST filename
		class		(headerfile
                                |devicefile
                                |libraryfile
                                |directory
				|symlink)       #IMPLIED
		path		CDATA		#IMPLIED
		%moreinfo.attrib;
		%common.attrib;
		%filename.role.attrib;
		%local.filename.attrib;
>
<!--end of filename.attlist-->]]>
<!--end of filename.module-->]]>

<!ENTITY % function.module "INCLUDE">
<![%function.module;[
<!ENTITY % local.function.attrib "">
<!ENTITY % function.role.attrib "%role.attrib;">

<!ENTITY % function.element "INCLUDE">
<![%function.element;[
<!ELEMENT function (%cptr.char.mix;)*>
<!--end of function.element-->]]>

<!ENTITY % function.attlist "INCLUDE">
<![%function.attlist;[
<!ATTLIST function
		%moreinfo.attrib;
		%common.attrib;
		%function.role.attrib;
		%local.function.attrib;
>
<!--end of function.attlist-->]]>
<!--end of function.module-->]]>

<!ENTITY % guibutton.module "INCLUDE">
<![%guibutton.module;[
<!ENTITY % local.guibutton.attrib "">
<!ENTITY % guibutton.role.attrib "%role.attrib;">

<!ENTITY % guibutton.element "INCLUDE">
<![%guibutton.element;[
<!ELEMENT guibutton (%smallcptr.char.mix;|accel)*>
<!--end of guibutton.element-->]]>

<!ENTITY % guibutton.attlist "INCLUDE">
<![%guibutton.attlist;[
<!ATTLIST guibutton
		%moreinfo.attrib;
		%common.attrib;
		%guibutton.role.attrib;
		%local.guibutton.attrib;
>
<!--end of guibutton.attlist-->]]>
<!--end of guibutton.module-->]]>

<!ENTITY % guiicon.module "INCLUDE">
<![%guiicon.module;[
<!ENTITY % local.guiicon.attrib "">
<!ENTITY % guiicon.role.attrib "%role.attrib;">

<!ENTITY % guiicon.element "INCLUDE">
<![%guiicon.element;[
<!ELEMENT guiicon (%smallcptr.char.mix;|accel)*>
<!--end of guiicon.element-->]]>

<!ENTITY % guiicon.attlist "INCLUDE">
<![%guiicon.attlist;[
<!ATTLIST guiicon
		%moreinfo.attrib;
		%common.attrib;
		%guiicon.role.attrib;
		%local.guiicon.attrib;
>
<!--end of guiicon.attlist-->]]>
<!--end of guiicon.module-->]]>

<!ENTITY % guilabel.module "INCLUDE">
<![%guilabel.module;[
<!ENTITY % local.guilabel.attrib "">
<!ENTITY % guilabel.role.attrib "%role.attrib;">

<!ENTITY % guilabel.element "INCLUDE">
<![%guilabel.element;[
<!ELEMENT guilabel (%smallcptr.char.mix;|accel)*>
<!--end of guilabel.element-->]]>

<!ENTITY % guilabel.attlist "INCLUDE">
<![%guilabel.attlist;[
<!ATTLIST guilabel
		%moreinfo.attrib;
		%common.attrib;
		%guilabel.role.attrib;
		%local.guilabel.attrib;
>
<!--end of guilabel.attlist-->]]>
<!--end of guilabel.module-->]]>

<!ENTITY % guimenu.module "INCLUDE">
<![%guimenu.module;[
<!ENTITY % local.guimenu.attrib "">
<!ENTITY % guimenu.role.attrib "%role.attrib;">

<!ENTITY % guimenu.element "INCLUDE">
<![%guimenu.element;[
<!ELEMENT guimenu (%smallcptr.char.mix;|accel)*>
<!--end of guimenu.element-->]]>

<!ENTITY % guimenu.attlist "INCLUDE">
<![%guimenu.attlist;[
<!ATTLIST guimenu
		%moreinfo.attrib;
		%common.attrib;
		%guimenu.role.attrib;
		%local.guimenu.attrib;
>
<!--end of guimenu.attlist-->]]>
<!--end of guimenu.module-->]]>

<!ENTITY % guimenuitem.module "INCLUDE">
<![%guimenuitem.module;[
<!ENTITY % local.guimenuitem.attrib "">
<!ENTITY % guimenuitem.role.attrib "%role.attrib;">

<!ENTITY % guimenuitem.element "INCLUDE">
<![%guimenuitem.element;[
<!ELEMENT guimenuitem (%smallcptr.char.mix;|accel)*>
<!--end of guimenuitem.element-->]]>

<!ENTITY % guimenuitem.attlist "INCLUDE">
<![%guimenuitem.attlist;[
<!ATTLIST guimenuitem
		%moreinfo.attrib;
		%common.attrib;
		%guimenuitem.role.attrib;
		%local.guimenuitem.attrib;
>
<!--end of guimenuitem.attlist-->]]>
<!--end of guimenuitem.module-->]]>

<!ENTITY % guisubmenu.module "INCLUDE">
<![%guisubmenu.module;[
<!ENTITY % local.guisubmenu.attrib "">
<!ENTITY % guisubmenu.role.attrib "%role.attrib;">

<!ENTITY % guisubmenu.element "INCLUDE">
<![%guisubmenu.element;[
<!ELEMENT guisubmenu (%smallcptr.char.mix;|accel)*>
<!--end of guisubmenu.element-->]]>

<!ENTITY % guisubmenu.attlist "INCLUDE">
<![%guisubmenu.attlist;[
<!ATTLIST guisubmenu
		%moreinfo.attrib;
		%common.attrib;
		%guisubmenu.role.attrib;
		%local.guisubmenu.attrib;
>
<!--end of guisubmenu.attlist-->]]>
<!--end of guisubmenu.module-->]]>

<!ENTITY % hardware.module "INCLUDE">
<![%hardware.module;[
<!ENTITY % local.hardware.attrib "">
<!ENTITY % hardware.role.attrib "%role.attrib;">

<!ENTITY % hardware.element "INCLUDE">
<![%hardware.element;[
<!ELEMENT hardware (%smallcptr.char.mix;)*>
<!--end of hardware.element-->]]>

<!ENTITY % hardware.attlist "INCLUDE">
<![%hardware.attlist;[
<!ATTLIST hardware
		%moreinfo.attrib;
		%common.attrib;
		%hardware.role.attrib;
		%local.hardware.attrib;
>
<!--end of hardware.attlist-->]]>
<!--end of hardware.module-->]]>

<!ENTITY % interface.module "INCLUDE">
<![%interface.module;[
<!ENTITY % local.interface.attrib "">
<!ENTITY % interface.role.attrib "%role.attrib;">

<!ENTITY % interface.element "INCLUDE">
<![%interface.element;[
<!ELEMENT interface (%smallcptr.char.mix;|accel)*>
<!--end of interface.element-->]]>

<!-- Class: Type of the Interface item; no default -->


<!ENTITY % interface.attlist "INCLUDE">
<![%interface.attlist;[
<!ATTLIST interface
		%moreinfo.attrib;
		%common.attrib;
		%interface.role.attrib;
		%local.interface.attrib;
>
<!--end of interface.attlist-->]]>
<!--end of interface.module-->]]>

<!ENTITY % keycap.module "INCLUDE">
<![%keycap.module;[
<!ENTITY % local.keycap.attrib "">
<!ENTITY % keycap.role.attrib "%role.attrib;">

<!ENTITY % keycap.element "INCLUDE">
<![%keycap.element;[
<!ELEMENT keycap (%smallcptr.char.mix;)*>
<!--end of keycap.element-->]]>

<!ENTITY % keycap.attlist "INCLUDE">
<![%keycap.attlist;[
<!ATTLIST keycap
		%moreinfo.attrib;
		%common.attrib;
		%keycap.role.attrib;
		%local.keycap.attrib;
>
<!--end of keycap.attlist-->]]>
<!--end of keycap.module-->]]>

<!ENTITY % keycode.module "INCLUDE">
<![%keycode.module;[
<!ENTITY % local.keycode.attrib "">
<!ENTITY % keycode.role.attrib "%role.attrib;">

<!ENTITY % keycode.element "INCLUDE">
<![%keycode.element;[
<!ELEMENT keycode (%smallcptr.char.mix;)*>
<!--end of keycode.element-->]]>

<!ENTITY % keycode.attlist "INCLUDE">
<![%keycode.attlist;[
<!ATTLIST keycode
		%common.attrib;
		%keycode.role.attrib;
		%local.keycode.attrib;
>
<!--end of keycode.attlist-->]]>
<!--end of keycode.module-->]]>

<!ENTITY % keycombo.module "INCLUDE">
<![%keycombo.module;[
<!ENTITY % local.keycombo.attrib "">
<!ENTITY % keycombo.role.attrib "%role.attrib;">

<!ENTITY % keycombo.element "INCLUDE">
<![%keycombo.element;[
<!ELEMENT keycombo ((keycap|keycombo|keysym|mousebutton)+)>
<!--end of keycombo.element-->]]>

<!ENTITY % keycombo.attlist "INCLUDE">
<![%keycombo.attlist;[
<!ATTLIST keycombo
		%keyaction.attrib;
		%moreinfo.attrib;
		%common.attrib;
		%keycombo.role.attrib;
		%local.keycombo.attrib;
>
<!--end of keycombo.attlist-->]]>
<!--end of keycombo.module-->]]>

<!ENTITY % keysym.module "INCLUDE">
<![%keysym.module;[
<!ENTITY % local.keysym.attrib "">
<!ENTITY % keysysm.role.attrib "%role.attrib;">

<!ENTITY % keysym.element "INCLUDE">
<![%keysym.element;[
<!ELEMENT keysym (%smallcptr.char.mix;)*>
<!--end of keysym.element-->]]>

<!ENTITY % keysym.attlist "INCLUDE">
<![%keysym.attlist;[
<!ATTLIST keysym
		%common.attrib;
		%keysysm.role.attrib;
		%local.keysym.attrib;
>
<!--end of keysym.attlist-->]]>
<!--end of keysym.module-->]]>

<!ENTITY % lineannotation.module "INCLUDE">
<![%lineannotation.module;[
<!ENTITY % local.lineannotation.attrib "">
<!ENTITY % lineannotation.role.attrib "%role.attrib;">

<!ENTITY % lineannotation.element "INCLUDE">
<![%lineannotation.element;[
<!ELEMENT lineannotation (%para.char.mix;)*>
<!--end of lineannotation.element-->]]>

<!ENTITY % lineannotation.attlist "INCLUDE">
<![%lineannotation.attlist;[
<!ATTLIST lineannotation
		%common.attrib;
		%lineannotation.role.attrib;
		%local.lineannotation.attrib;
>
<!--end of lineannotation.attlist-->]]>
<!--end of lineannotation.module-->]]>

<!ENTITY % literal.module "INCLUDE">
<![%literal.module;[
<!ENTITY % local.literal.attrib "">
<!ENTITY % literal.role.attrib "%role.attrib;">

<!ENTITY % literal.element "INCLUDE">
<![%literal.element;[
<!ELEMENT literal (%cptr.char.mix;)*>
<!--end of literal.element-->]]>

<!ENTITY % literal.attlist "INCLUDE">
<![%literal.attlist;[
<!ATTLIST literal
		%moreinfo.attrib;
		%common.attrib;
		%literal.role.attrib;
		%local.literal.attrib;
>
<!--end of literal.attlist-->]]>
<!--end of literal.module-->]]>

<!ENTITY % constant.module "INCLUDE">
<![ %constant.module; [
<!ENTITY % local.constant.attrib "">
<!ENTITY % constant.role.attrib "%role.attrib;">

<!ENTITY % constant.element "INCLUDE">
<![ %constant.element; [
<!ELEMENT constant (%smallcptr.char.mix;)*>
<!--end of constant.element-->]]>

<!ENTITY % constant.attlist "INCLUDE">
<![ %constant.attlist; [
<!ATTLIST constant
		%common.attrib;
		%constant.role.attrib;
		%local.constant.attrib;
		class	(limit)		#IMPLIED
>
<!--end of constant.attlist-->]]>
<!--end of constant.module-->]]>

<!ENTITY % varname.module "INCLUDE">
<![ %varname.module; [
<!ENTITY % local.varname.attrib "">
<!ENTITY % varname.role.attrib "%role.attrib;">

<!ENTITY % varname.element "INCLUDE">
<![ %varname.element; [
<!ELEMENT varname (%smallcptr.char.mix;)*>
<!--end of varname.element-->]]>

<!ENTITY % varname.attlist "INCLUDE">
<![ %varname.attlist; [
<!ATTLIST varname
		%common.attrib;
		%varname.role.attrib;
		%local.varname.attrib;
>
<!--end of varname.attlist-->]]>
<!--end of varname.module-->]]>

<!ENTITY % markup.module "INCLUDE">
<![%markup.module;[
<!ENTITY % local.markup.attrib "">
<!ENTITY % markup.role.attrib "%role.attrib;">

<!ENTITY % markup.element "INCLUDE">
<![%markup.element;[
<!ELEMENT markup (%smallcptr.char.mix;)*>
<!--end of markup.element-->]]>

<!ENTITY % markup.attlist "INCLUDE">
<![%markup.attlist;[
<!ATTLIST markup
		%common.attrib;
		%markup.role.attrib;
		%local.markup.attrib;
>
<!--end of markup.attlist-->]]>
<!--end of markup.module-->]]>

<!ENTITY % medialabel.module "INCLUDE">
<![%medialabel.module;[
<!ENTITY % local.medialabel.attrib "">
<!ENTITY % medialabel.role.attrib "%role.attrib;">

<!ENTITY % medialabel.element "INCLUDE">
<![%medialabel.element;[
<!ELEMENT medialabel (%smallcptr.char.mix;)*>
<!--end of medialabel.element-->]]>

<!-- Class: Type of medium named by the element; no default -->


<!ENTITY % medialabel.attlist "INCLUDE">
<![%medialabel.attlist;[
<!ATTLIST medialabel
		class 		(cartridge
				|cdrom
				|disk
				|tape)		#IMPLIED
		%common.attrib;
		%medialabel.role.attrib;
		%local.medialabel.attrib;
>
<!--end of medialabel.attlist-->]]>
<!--end of medialabel.module-->]]>

<!ENTITY % menuchoice.content.module "INCLUDE">
<![%menuchoice.content.module;[
<!ENTITY % menuchoice.module "INCLUDE">
<![%menuchoice.module;[
<!ENTITY % local.menuchoice.attrib "">
<!ENTITY % menuchoice.role.attrib "%role.attrib;">

<!ENTITY % menuchoice.element "INCLUDE">
<![%menuchoice.element;[
<!ELEMENT menuchoice (shortcut?, (guibutton|guiicon|guilabel
		|guimenu|guimenuitem|guisubmenu|interface)+)>
<!--end of menuchoice.element-->]]>

<!ENTITY % menuchoice.attlist "INCLUDE">
<![%menuchoice.attlist;[
<!ATTLIST menuchoice
		%moreinfo.attrib;
		%common.attrib;
		%menuchoice.role.attrib;
		%local.menuchoice.attrib;
>
<!--end of menuchoice.attlist-->]]>
<!--end of menuchoice.module-->]]>

<!ENTITY % shortcut.module "INCLUDE">
<![%shortcut.module;[
<!-- See also KeyCombo -->
<!ENTITY % local.shortcut.attrib "">
<!ENTITY % shortcut.role.attrib "%role.attrib;">

<!ENTITY % shortcut.element "INCLUDE">
<![%shortcut.element;[
<!ELEMENT shortcut ((keycap|keycombo|keysym|mousebutton)+)>
<!--end of shortcut.element-->]]>

<!ENTITY % shortcut.attlist "INCLUDE">
<![%shortcut.attlist;[
<!ATTLIST shortcut
		%keyaction.attrib;
		%moreinfo.attrib;
		%common.attrib;
		%shortcut.role.attrib;
		%local.shortcut.attrib;
>
<!--end of shortcut.attlist-->]]>
<!--end of shortcut.module-->]]>
<!--end of menuchoice.content.module-->]]>

<!ENTITY % mousebutton.module "INCLUDE">
<![%mousebutton.module;[
<!ENTITY % local.mousebutton.attrib "">
<!ENTITY % mousebutton.role.attrib "%role.attrib;">

<!ENTITY % mousebutton.element "INCLUDE">
<![%mousebutton.element;[
<!ELEMENT mousebutton (%smallcptr.char.mix;)*>
<!--end of mousebutton.element-->]]>

<!ENTITY % mousebutton.attlist "INCLUDE">
<![%mousebutton.attlist;[
<!ATTLIST mousebutton
		%moreinfo.attrib;
		%common.attrib;
		%mousebutton.role.attrib;
		%local.mousebutton.attrib;
>
<!--end of mousebutton.attlist-->]]>
<!--end of mousebutton.module-->]]>

<!ENTITY % msgtext.module "INCLUDE">
<![%msgtext.module;[
<!ENTITY % local.msgtext.attrib "">
<!ENTITY % msgtext.role.attrib "%role.attrib;">

<!ENTITY % msgtext.element "INCLUDE">
<![%msgtext.element;[
<!ELEMENT msgtext ((%component.mix;)+)>
<!--end of msgtext.element-->]]>

<!ENTITY % msgtext.attlist "INCLUDE">
<![%msgtext.attlist;[
<!ATTLIST msgtext
		%common.attrib;
		%msgtext.role.attrib;
		%local.msgtext.attrib;
>
<!--end of msgtext.attlist-->]]>
<!--end of msgtext.module-->]]>

<!ENTITY % option.module "INCLUDE">
<![%option.module;[
<!ENTITY % local.option.attrib "">
<!ENTITY % option.role.attrib "%role.attrib;">

<!ENTITY % option.element "INCLUDE">
<![%option.element;[
<!ELEMENT option (%smallcptr.char.mix;)*>
<!--end of option.element-->]]>

<!ENTITY % option.attlist "INCLUDE">
<![%option.attlist;[
<!ATTLIST option
		%common.attrib;
		%option.role.attrib;
		%local.option.attrib;
>
<!--end of option.attlist-->]]>
<!--end of option.module-->]]>

<!ENTITY % optional.module "INCLUDE">
<![%optional.module;[
<!ENTITY % local.optional.attrib "">
<!ENTITY % optional.role.attrib "%role.attrib;">

<!ENTITY % optional.element "INCLUDE">
<![%optional.element;[
<!ELEMENT optional (%cptr.char.mix;)*>
<!--end of optional.element-->]]>

<!ENTITY % optional.attlist "INCLUDE">
<![%optional.attlist;[
<!ATTLIST optional
		%common.attrib;
		%optional.role.attrib;
		%local.optional.attrib;
>
<!--end of optional.attlist-->]]>
<!--end of optional.module-->]]>

<!ENTITY % parameter.module "INCLUDE">
<![%parameter.module;[
<!ENTITY % local.parameter.attrib "">
<!ENTITY % parameter.role.attrib "%role.attrib;">

<!ENTITY % parameter.element "INCLUDE">
<![%parameter.element;[
<!ELEMENT parameter (%smallcptr.char.mix;)*>
<!--end of parameter.element-->]]>

<!-- Class: Type of the Parameter; no default -->


<!ENTITY % parameter.attlist "INCLUDE">
<![%parameter.attlist;[
<!ATTLIST parameter
		class 		(command
				|function
				|option)	#IMPLIED
		%moreinfo.attrib;
		%common.attrib;
		%parameter.role.attrib;
		%local.parameter.attrib;
>
<!--end of parameter.attlist-->]]>
<!--end of parameter.module-->]]>

<!ENTITY % prompt.module "INCLUDE">
<![%prompt.module;[
<!ENTITY % local.prompt.attrib "">
<!ENTITY % prompt.role.attrib "%role.attrib;">

<!ENTITY % prompt.element "INCLUDE">
<![%prompt.element;[
<!ELEMENT prompt (%smallcptr.char.mix;)*>
<!--end of prompt.element-->]]>

<!ENTITY % prompt.attlist "INCLUDE">
<![%prompt.attlist;[
<!ATTLIST prompt
		%moreinfo.attrib;
		%common.attrib;
		%prompt.role.attrib;
		%local.prompt.attrib;
>
<!--end of prompt.attlist-->]]>
<!--end of prompt.module-->]]>

<!ENTITY % property.module "INCLUDE">
<![%property.module;[
<!ENTITY % local.property.attrib "">
<!ENTITY % property.role.attrib "%role.attrib;">

<!ENTITY % property.element "INCLUDE">
<![%property.element;[
<!ELEMENT property (%smallcptr.char.mix;)*>
<!--end of property.element-->]]>

<!ENTITY % property.attlist "INCLUDE">
<![%property.attlist;[
<!ATTLIST property
		%moreinfo.attrib;
		%common.attrib;
		%property.role.attrib;
		%local.property.attrib;
>
<!--end of property.attlist-->]]>
<!--end of property.module-->]]>

<!ENTITY % replaceable.module "INCLUDE">
<![%replaceable.module;[
<!ENTITY % local.replaceable.attrib "">
<!ENTITY % replaceable.role.attrib "%role.attrib;">

<!ENTITY % replaceable.element "INCLUDE">
<![%replaceable.element;[
<!ELEMENT replaceable (#PCDATA 
		| %link.char.class; 
		| optional
		| %base.char.class; 
		| %other.char.class; 
		| inlinegraphic
                | inlinemediaobject)*>
<!--end of replaceable.element-->]]>

<!-- Class: Type of information the element represents; no
		default -->


<!ENTITY % replaceable.attlist "INCLUDE">
<![%replaceable.attlist;[
<!ATTLIST replaceable
		class		(command
				|function
				|option
				|parameter)	#IMPLIED
		%common.attrib;
		%replaceable.role.attrib;
		%local.replaceable.attrib;
>
<!--end of replaceable.attlist-->]]>
<!--end of replaceable.module-->]]>

<!ENTITY % returnvalue.module "INCLUDE">
<![%returnvalue.module;[
<!ENTITY % local.returnvalue.attrib "">
<!ENTITY % returnvalue.role.attrib "%role.attrib;">

<!ENTITY % returnvalue.element "INCLUDE">
<![%returnvalue.element;[
<!ELEMENT returnvalue (%smallcptr.char.mix;)*>
<!--end of returnvalue.element-->]]>

<!ENTITY % returnvalue.attlist "INCLUDE">
<![%returnvalue.attlist;[
<!ATTLIST returnvalue
		%common.attrib;
		%returnvalue.role.attrib;
		%local.returnvalue.attrib;
>
<!--end of returnvalue.attlist-->]]>
<!--end of returnvalue.module-->]]>

<!ENTITY % sgmltag.module "INCLUDE">
<![%sgmltag.module;[
<!ENTITY % local.sgmltag.attrib "">
<!ENTITY % sgmltag.role.attrib "%role.attrib;">

<!ENTITY % sgmltag.element "INCLUDE">
<![%sgmltag.element;[
<!ELEMENT sgmltag (%smallcptr.char.mix;)*>
<!--end of sgmltag.element-->]]>

<!-- Class: Type of SGML construct the element names; no default -->


<!ENTITY % sgmltag.attlist "INCLUDE">
<![%sgmltag.attlist;[
<!ATTLIST sgmltag
		class 		(attribute
				|attvalue
				|element
				|endtag
                                |emptytag
				|genentity
				|numcharref
				|paramentity
				|pi
                                |xmlpi
				|starttag
				|sgmlcomment)	#IMPLIED
		%common.attrib;
		%sgmltag.role.attrib;
		%local.sgmltag.attrib;
>
<!--end of sgmltag.attlist-->]]>
<!--end of sgmltag.module-->]]>

<!ENTITY % structfield.module "INCLUDE">
<![%structfield.module;[
<!ENTITY % local.structfield.attrib "">
<!ENTITY % structfield.role.attrib "%role.attrib;">

<!ENTITY % structfield.element "INCLUDE">
<![%structfield.element;[
<!ELEMENT structfield (%smallcptr.char.mix;)*>
<!--end of structfield.element-->]]>

<!ENTITY % structfield.attlist "INCLUDE">
<![%structfield.attlist;[
<!ATTLIST structfield
		%common.attrib;
		%structfield.role.attrib;
		%local.structfield.attrib;
>
<!--end of structfield.attlist-->]]>
<!--end of structfield.module-->]]>

<!ENTITY % structname.module "INCLUDE">
<![%structname.module;[
<!ENTITY % local.structname.attrib "">
<!ENTITY % structname.role.attrib "%role.attrib;">

<!ENTITY % structname.element "INCLUDE">
<![%structname.element;[
<!ELEMENT structname (%smallcptr.char.mix;)*>
<!--end of structname.element-->]]>

<!ENTITY % structname.attlist "INCLUDE">
<![%structname.attlist;[
<!ATTLIST structname
		%common.attrib;
		%structname.role.attrib;
		%local.structname.attrib;
>
<!--end of structname.attlist-->]]>
<!--end of structname.module-->]]>

<!ENTITY % symbol.module "INCLUDE">
<![%symbol.module;[
<!ENTITY % local.symbol.attrib "">
<!ENTITY % symbol.role.attrib "%role.attrib;">

<!ENTITY % symbol.element "INCLUDE">
<![%symbol.element;[
<!ELEMENT symbol (%smallcptr.char.mix;)*>
<!--end of symbol.element-->]]>

<!-- Class: Type of symbol; no default -->


<!ENTITY % symbol.attlist "INCLUDE">
<![%symbol.attlist;[
<!ATTLIST symbol
		class		(limit)		#IMPLIED
		%common.attrib;
		%symbol.role.attrib;
		%local.symbol.attrib;
>
<!--end of symbol.attlist-->]]>
<!--end of symbol.module-->]]>

<!ENTITY % systemitem.module "INCLUDE">
<![%systemitem.module;[
<!ENTITY % local.systemitem.attrib "">
<!ENTITY % systemitem.role.attrib "%role.attrib;">

<!ENTITY % systemitem.element "INCLUDE">
<![%systemitem.element;[
<!ELEMENT systemitem (%smallcptr.char.mix; | acronym)*>
<!--end of systemitem.element-->]]>

<!-- Class: Type of system item the element names; no default -->

<!ENTITY % systemitem.attlist "INCLUDE">
<![%systemitem.attlist;[
<!ATTLIST systemitem
		class	(constant
			|groupname
                        |library
			|macro
			|osname
			|resource
			|systemname
                        |username)	#IMPLIED
		%moreinfo.attrib;
		%common.attrib;
		%systemitem.role.attrib;
		%local.systemitem.attrib;
>
<!--end of systemitem.attlist-->]]>
<!--end of systemitem.module-->]]>


<!ENTITY % token.module "INCLUDE">
<![%token.module;[
<!ENTITY % local.token.attrib "">
<!ENTITY % token.role.attrib "%role.attrib;">

<!ENTITY % token.element "INCLUDE">
<![%token.element;[
<!ELEMENT token (%smallcptr.char.mix;)*>
<!--end of token.element-->]]>

<!ENTITY % token.attlist "INCLUDE">
<![%token.attlist;[
<!ATTLIST token
		%common.attrib;
		%token.role.attrib;
		%local.token.attrib;
>
<!--end of token.attlist-->]]>
<!--end of token.module-->]]>

<!ENTITY % type.module "INCLUDE">
<![%type.module;[
<!ENTITY % local.type.attrib "">
<!ENTITY % type.role.attrib "%role.attrib;">

<!ENTITY % type.element "INCLUDE">
<![%type.element;[
<!ELEMENT type (%smallcptr.char.mix;)*>
<!--end of type.element-->]]>

<!ENTITY % type.attlist "INCLUDE">
<![%type.attlist;[
<!ATTLIST type
		%common.attrib;
		%type.role.attrib;
		%local.type.attrib;
>
<!--end of type.attlist-->]]>
<!--end of type.module-->]]>

<!ENTITY % userinput.module "INCLUDE">
<![%userinput.module;[
<!ENTITY % local.userinput.attrib "">
<!ENTITY % userinput.role.attrib "%role.attrib;">

<!ENTITY % userinput.element "INCLUDE">
<![%userinput.element;[
<!ELEMENT userinput (%cptr.char.mix;)*>
<!--end of userinput.element-->]]>

<!ENTITY % userinput.attlist "INCLUDE">
<![%userinput.attlist;[
<!ATTLIST userinput
		%moreinfo.attrib;
		%common.attrib;
		%userinput.role.attrib;
		%local.userinput.attrib;
>
<!--end of userinput.attlist-->]]>
<!--end of userinput.module-->]]>

<!-- General words and phrases ............................................ -->

<!ENTITY % abbrev.module "INCLUDE">
<![%abbrev.module;[
<!ENTITY % local.abbrev.attrib "">
<!ENTITY % abbrev.role.attrib "%role.attrib;">

<!ENTITY % abbrev.element "INCLUDE">
<![%abbrev.element;[
<!ELEMENT abbrev (%word.char.mix;)*>
<!--end of abbrev.element-->]]>

<!ENTITY % abbrev.attlist "INCLUDE">
<![%abbrev.attlist;[
<!ATTLIST abbrev
		%common.attrib;
		%abbrev.role.attrib;
		%local.abbrev.attrib;
>
<!--end of abbrev.attlist-->]]>
<!--end of abbrev.module-->]]>

<!ENTITY % acronym.module "INCLUDE">
<![%acronym.module;[
<!ENTITY % local.acronym.attrib "">
<!ENTITY % acronym.role.attrib "%role.attrib;">

<!ENTITY % acronym.element "INCLUDE">
<![%acronym.element;[
<!ELEMENT acronym (%word.char.mix;)*>
<!--end of acronym.element-->]]>

<!ENTITY % acronym.attlist "INCLUDE">
<![%acronym.attlist;[
<!ATTLIST acronym
		%common.attrib;
		%acronym.role.attrib;
		%local.acronym.attrib;
>
<!--end of acronym.attlist-->]]>
<!--end of acronym.module-->]]>

<!ENTITY % citation.module "INCLUDE">
<![%citation.module;[
<!ENTITY % local.citation.attrib "">
<!ENTITY % citation.role.attrib "%role.attrib;">

<!ENTITY % citation.element "INCLUDE">
<![%citation.element;[
<!ELEMENT citation (%para.char.mix;)*>
<!--end of citation.element-->]]>

<!ENTITY % citation.attlist "INCLUDE">
<![%citation.attlist;[
<!ATTLIST citation
		%common.attrib;
		%citation.role.attrib;
		%local.citation.attrib;
>
<!--end of citation.attlist-->]]>
<!--end of citation.module-->]]>

<!ENTITY % citerefentry.module "INCLUDE">
<![%citerefentry.module;[
<!ENTITY % local.citerefentry.attrib "">
<!ENTITY % citerefentry.role.attrib "%role.attrib;">

<!ENTITY % citerefentry.element "INCLUDE">
<![%citerefentry.element;[
<!ELEMENT citerefentry (refentrytitle, manvolnum?)>
<!--end of citerefentry.element-->]]>

<!ENTITY % citerefentry.attlist "INCLUDE">
<![%citerefentry.attlist;[
<!ATTLIST citerefentry
		%common.attrib;
		%citerefentry.role.attrib;
		%local.citerefentry.attrib;
>
<!--end of citerefentry.attlist-->]]>
<!--end of citerefentry.module-->]]>

<!ENTITY % refentrytitle.module "INCLUDE">
<![%refentrytitle.module;[
<!ENTITY % local.refentrytitle.attrib "">
<!ENTITY % refentrytitle.role.attrib "%role.attrib;">

<!ENTITY % refentrytitle.element "INCLUDE">
<![%refentrytitle.element;[
<!ELEMENT refentrytitle (%para.char.mix;)*>
<!--end of refentrytitle.element-->]]>

<!ENTITY % refentrytitle.attlist "INCLUDE">
<![%refentrytitle.attlist;[
<!ATTLIST refentrytitle
		%common.attrib;
		%refentrytitle.role.attrib;
		%local.refentrytitle.attrib;
>
<!--end of refentrytitle.attlist-->]]>
<!--end of refentrytitle.module-->]]>

<!ENTITY % manvolnum.module "INCLUDE">
<![%manvolnum.module;[
<!ENTITY % local.manvolnum.attrib "">
<!ENTITY % namvolnum.role.attrib "%role.attrib;">

<!ENTITY % manvolnum.element "INCLUDE">
<![%manvolnum.element;[
<!ELEMENT manvolnum (%word.char.mix;)*>
<!--end of manvolnum.element-->]]>

<!ENTITY % manvolnum.attlist "INCLUDE">
<![%manvolnum.attlist;[
<!ATTLIST manvolnum
		%common.attrib;
		%namvolnum.role.attrib;
		%local.manvolnum.attrib;
>
<!--end of manvolnum.attlist-->]]>
<!--end of manvolnum.module-->]]>

<!ENTITY % citetitle.module "INCLUDE">
<![%citetitle.module;[
<!ENTITY % local.citetitle.attrib "">
<!ENTITY % citetitle.role.attrib "%role.attrib;">

<!ENTITY % citetitle.element "INCLUDE">
<![%citetitle.element;[
<!ELEMENT citetitle (%para.char.mix;)*>
<!--end of citetitle.element-->]]>

<!-- Pubwork: Genre of published work cited; no default -->


<!ENTITY % citetitle.attlist "INCLUDE">
<![%citetitle.attlist;[
<!ATTLIST citetitle
		pubwork		(article
				|book
				|chapter
				|part
				|refentry
				|section
				|journal
				|series
				|set
				|manuscript)	#IMPLIED
		%common.attrib;
		%citetitle.role.attrib;
		%local.citetitle.attrib;
>
<!--end of citetitle.attlist-->]]>
<!--end of citetitle.module-->]]>

<!ENTITY % emphasis.module "INCLUDE">
<![%emphasis.module;[
<!ENTITY % local.emphasis.attrib "">
<!ENTITY % emphasis.role.attrib "%role.attrib;">

<!ENTITY % emphasis.element "INCLUDE">
<![%emphasis.element;[
<!ELEMENT emphasis (%para.char.mix;)*>
<!--end of emphasis.element-->]]>

<!ENTITY % emphasis.attlist "INCLUDE">
<![%emphasis.attlist;[
<!ATTLIST emphasis
		%common.attrib;
		%emphasis.role.attrib;
		%local.emphasis.attrib;
>
<!--end of emphasis.attlist-->]]>
<!--end of emphasis.module-->]]>

<!ENTITY % firstterm.module "INCLUDE">
<![%firstterm.module;[
<!ENTITY % local.firstterm.attrib "">
<!ENTITY % firstterm.role.attrib "%role.attrib;">

<!ENTITY % firstterm.element "INCLUDE">
<![%firstterm.element;[
<!ELEMENT firstterm (%word.char.mix;)*>
<!--end of firstterm.element-->]]>

<!-- to GlossEntry or other explanation -->


<!ENTITY % firstterm.attlist "INCLUDE">
<![%firstterm.attlist;[
<!ATTLIST firstterm
		%linkend.attrib;		%common.attrib;
		%firstterm.role.attrib;
		%local.firstterm.attrib;
>
<!--end of firstterm.attlist-->]]>
<!--end of firstterm.module-->]]>

<!ENTITY % foreignphrase.module "INCLUDE">
<![%foreignphrase.module;[
<!ENTITY % local.foreignphrase.attrib "">
<!ENTITY % foreignphrase.role.attrib "%role.attrib;">

<!ENTITY % foreignphrase.element "INCLUDE">
<![%foreignphrase.element;[
<!ELEMENT foreignphrase (%para.char.mix;)*>
<!--end of foreignphrase.element-->]]>

<!ENTITY % foreignphrase.attlist "INCLUDE">
<![%foreignphrase.attlist;[
<!ATTLIST foreignphrase
		%common.attrib;
		%foreignphrase.role.attrib;
		%local.foreignphrase.attrib;
>
<!--end of foreignphrase.attlist-->]]>
<!--end of foreignphrase.module-->]]>

<!ENTITY % glossterm.module "INCLUDE">
<![%glossterm.module;[
<!ENTITY % local.glossterm.attrib "">
<!ENTITY % glossterm.role.attrib "%role.attrib;">

<!ENTITY % glossterm.element "INCLUDE">
<![%glossterm.element;[
<!ELEMENT glossterm (%para.char.mix;)*>
<!--end of glossterm.element-->]]>

<!-- to GlossEntry if Glossterm used in text -->
<!-- BaseForm: Provides the form of GlossTerm to be used
		for indexing -->


<!ENTITY % glossterm.attlist "INCLUDE">
<![%glossterm.attlist;[
<!ATTLIST glossterm
		%linkend.attrib;		baseform	CDATA		#IMPLIED
		%common.attrib;
		%glossterm.role.attrib;
		%local.glossterm.attrib;
>
<!--end of glossterm.attlist-->]]>
<!--end of glossterm.module-->]]>

<!ENTITY % phrase.module "INCLUDE">
<![%phrase.module;[
<!ENTITY % local.phrase.attrib "">
<!ENTITY % phrase.role.attrib "%role.attrib;">

<!ENTITY % phrase.element "INCLUDE">
<![%phrase.element;[
<!ELEMENT phrase (%para.char.mix;)*>
<!--end of phrase.element-->]]>

<!ENTITY % phrase.attlist "INCLUDE">
<![%phrase.attlist;[
<!ATTLIST phrase
		%common.attrib;
		%phrase.role.attrib;
		%local.phrase.attrib;
>
<!--end of phrase.attlist-->]]>
<!--end of phrase.module-->]]>

<!ENTITY % quote.module "INCLUDE">
<![%quote.module;[
<!ENTITY % local.quote.attrib "">
<!ENTITY % quote.role.attrib "%role.attrib;">

<!ENTITY % quote.element "INCLUDE">
<![%quote.element;[
<!ELEMENT quote (%para.char.mix;)*>
<!--end of quote.element-->]]>

<!ENTITY % quote.attlist "INCLUDE">
<![%quote.attlist;[
<!ATTLIST quote
		%common.attrib;
		%quote.role.attrib;
		%local.quote.attrib;
>
<!--end of quote.attlist-->]]>
<!--end of quote.module-->]]>

<!ENTITY % ssscript.module "INCLUDE">
<![%ssscript.module;[
<!ENTITY % local.ssscript.attrib "">
<!ENTITY % ssscript.role.attrib "%role.attrib;">

<!ENTITY % subscript.element "INCLUDE">
<![%subscript.element;[
<!ELEMENT subscript (#PCDATA 
		| %link.char.class;
		| emphasis
		| replaceable 
		| symbol 
		| inlinegraphic
                | inlinemediaobject
		| %base.char.class; 
		| %other.char.class;)*>
<!--end of subscript.element-->]]>

<!ENTITY % subscript.attlist "INCLUDE">
<![%subscript.attlist;[
<!ATTLIST subscript
		%common.attrib;
		%ssscript.role.attrib;
		%local.ssscript.attrib;
>
<!--end of subscript.attlist-->]]>

<!ENTITY % superscript.element "INCLUDE">
<![%superscript.element;[
<!ELEMENT superscript (#PCDATA 
		| %link.char.class;
		| emphasis
		| replaceable 
		| symbol 
		| inlinegraphic
                | inlinemediaobject 
		| %base.char.class; 
		| %other.char.class;)*>
<!--end of superscript.element-->]]>

<!ENTITY % superscript.attlist "INCLUDE">
<![%superscript.attlist;[
<!ATTLIST superscript
		%common.attrib;
		%ssscript.role.attrib;
		%local.ssscript.attrib;
>
<!--end of superscript.attlist-->]]>
<!--end of ssscript.module-->]]>

<!ENTITY % trademark.module "INCLUDE">
<![%trademark.module;[
<!ENTITY % local.trademark.attrib "">
<!ENTITY % trademark.role.attrib "%role.attrib;">

<!ENTITY % trademark.element "INCLUDE">
<![%trademark.element;[
<!ELEMENT trademark (#PCDATA 
		| %link.char.class; 
		| %tech.char.class;
		| %base.char.class; 
		| %other.char.class; 
		| inlinegraphic
                | inlinemediaobject
		| emphasis)*>
<!--end of trademark.element-->]]>

<!-- Class: More precisely identifies the item the element names -->


<!ENTITY % trademark.attlist "INCLUDE">
<![%trademark.attlist;[
<!ATTLIST trademark
		class		(service
				|trade
				|registered
				|copyright)	'trade'
		%common.attrib;
		%trademark.role.attrib;
		%local.trademark.attrib;
>
<!--end of trademark.attlist-->]]>
<!--end of trademark.module-->]]>

<!ENTITY % wordasword.module "INCLUDE">
<![%wordasword.module;[
<!ENTITY % local.wordasword.attrib "">
<!ENTITY % wordasword.role.attrib "%role.attrib;">

<!ENTITY % wordasword.element "INCLUDE">
<![%wordasword.element;[
<!ELEMENT wordasword (%word.char.mix;)*>
<!--end of wordasword.element-->]]>

<!ENTITY % wordasword.attlist "INCLUDE">
<![%wordasword.attlist;[
<!ATTLIST wordasword
		%common.attrib;
		%wordasword.role.attrib;
		%local.wordasword.attrib;
>
<!--end of wordasword.attlist-->]]>
<!--end of wordasword.module-->]]>

<!-- Links and cross-references ........................................... -->

<!ENTITY % link.module "INCLUDE">
<![%link.module;[
<!ENTITY % local.link.attrib "">
<!ENTITY % link.role.attrib "%role.attrib;">

<!ENTITY % link.element "INCLUDE">
<![%link.element;[
<!ELEMENT link (%para.char.mix;)*>
<!--end of link.element-->]]>

<!-- Endterm: ID of element containing text that is to be
		fetched from elsewhere in the document to appear as
		the content of this element -->
<!-- to linked-to object -->
<!-- Type: Freely assignable parameter -->


<!ENTITY % link.attlist "INCLUDE">
<![%link.attlist;[
<!ATTLIST link
		endterm		IDREF		#IMPLIED
		%linkendreq.attrib;		type		CDATA		#IMPLIED
		%common.attrib;
		%link.role.attrib;
		%local.link.attrib;
>
<!--end of link.attlist-->]]>
<!--end of link.module-->]]>

<!ENTITY % olink.module "INCLUDE">
<![%olink.module;[
<!ENTITY % local.olink.attrib "">
<!ENTITY % olink.role.attrib "%role.attrib;">

<!ENTITY % olink.element "INCLUDE">
<![%olink.element;[
<!ELEMENT olink (%para.char.mix;)*>
<!--end of olink.element-->]]>

<!-- TargetDocEnt: Name of an entity to be the target of the link -->
<!-- LinkMode: ID of a ModeSpec containing instructions for
		operating on the entity named by TargetDocEnt -->
<!-- LocalInfo: Information that may be passed to ModeSpec -->
<!-- Type: Freely assignable parameter -->


<!ENTITY % olink.attlist "INCLUDE">
<![%olink.attlist;[
<!ATTLIST olink
		targetdocent	ENTITY 		#IMPLIED
		linkmode	IDREF		#IMPLIED
		localinfo 	CDATA		#IMPLIED
		type		CDATA		#IMPLIED
		%common.attrib;
		%olink.role.attrib;
		%local.olink.attrib;
>
<!--end of olink.attlist-->]]>
<!--end of olink.module-->]]>

<!ENTITY % ulink.module "INCLUDE">
<![%ulink.module;[
<!ENTITY % local.ulink.attrib "">
<!ENTITY % ulink.role.attrib "%role.attrib;">

<!ENTITY % ulink.element "INCLUDE">
<![%ulink.element;[
<!ELEMENT ulink (%para.char.mix;)*>
<!--end of ulink.element-->]]>

<!-- URL: uniform resource locator; the target of the ULink -->
<!-- Type: Freely assignable parameter -->


<!ENTITY % ulink.attlist "INCLUDE">
<![%ulink.attlist;[
<!ATTLIST ulink
		url		CDATA		#REQUIRED
		type		CDATA		#IMPLIED
		%common.attrib;
		%ulink.role.attrib;
		%local.ulink.attrib;
>
<!--end of ulink.attlist-->]]>
<!--end of ulink.module-->]]>

<!ENTITY % footnoteref.module "INCLUDE">
<![%footnoteref.module;[
<!ENTITY % local.footnoteref.attrib "">
<!ENTITY % footnoteref.role.attrib "%role.attrib;">

<!ENTITY % footnoteref.element "INCLUDE">
<![%footnoteref.element;[
<!ELEMENT footnoteref EMPTY>
<!--end of footnoteref.element-->]]>

<!-- to footnote content supplied elsewhere -->


<!ENTITY % footnoteref.attlist "INCLUDE">
<![%footnoteref.attlist;[
<!ATTLIST footnoteref
		%linkendreq.attrib;		%label.attrib;
		%common.attrib;
		%footnoteref.role.attrib;
		%local.footnoteref.attrib;
>
<!--end of footnoteref.attlist-->]]>
<!--end of footnoteref.module-->]]>

<!ENTITY % xref.module "INCLUDE">
<![%xref.module;[
<!ENTITY % local.xref.attrib "">
<!ENTITY % xref.role.attrib "%role.attrib;">

<!ENTITY % xref.element "INCLUDE">
<![%xref.element;[
<!ELEMENT xref EMPTY>
<!--end of xref.element-->]]>

<!-- Endterm: ID of element containing text that is to be
		fetched from elsewhere in the document to appear as
		the content of this element -->
<!-- to linked-to object -->


<!ENTITY % xref.attlist "INCLUDE">
<![%xref.attlist;[
<!ATTLIST xref
		endterm		IDREF		#IMPLIED
		%linkendreq.attrib;		%common.attrib;
		%xref.role.attrib;
		%local.xref.attrib;
>
<!--end of xref.attlist-->]]>
<!--end of xref.module-->]]>

<!-- Ubiquitous elements .................................................. -->

<!ENTITY % anchor.module "INCLUDE">
<![%anchor.module;[
<!ENTITY % local.anchor.attrib "">
<!ENTITY % anchor.role.attrib "%role.attrib;">

<!ENTITY % anchor.element "INCLUDE">
<![%anchor.element;[
<!ELEMENT anchor EMPTY>
<!--end of anchor.element-->]]>

<!-- required -->
<!-- replaces Lang -->


<!ENTITY % anchor.attlist "INCLUDE">
<![%anchor.attlist;[
<!ATTLIST anchor
		%idreq.attrib;		%pagenum.attrib;		%remap.attrib;
		%xreflabel.attrib;
		%revisionflag.attrib;
		%effectivity.attrib;
		%anchor.role.attrib;
		%local.anchor.attrib;
>
<!--end of anchor.attlist-->]]>
<!--end of anchor.module-->]]>

<!ENTITY % beginpage.module "INCLUDE">
<![%beginpage.module;[
<!ENTITY % local.beginpage.attrib "">
<!ENTITY % beginpage.role.attrib "%role.attrib;">

<!ENTITY % beginpage.element "INCLUDE">
<![%beginpage.element;[
<!ELEMENT beginpage EMPTY>
<!--end of beginpage.element-->]]>

<!-- PageNum: Number of page that begins at this point -->


<!ENTITY % beginpage.attlist "INCLUDE">
<![%beginpage.attlist;[
<!ATTLIST beginpage
		%pagenum.attrib;
		%common.attrib;
		%beginpage.role.attrib;
		%local.beginpage.attrib;
>
<!--end of beginpage.attlist-->]]>
<!--end of beginpage.module-->]]>

<!-- IndexTerms appear in the text flow for generating or linking an
     index. -->

<!ENTITY % indexterm.content.module "INCLUDE">
<![%indexterm.content.module;[
<!ENTITY % indexterm.module "INCLUDE">
<![%indexterm.module;[
<!ENTITY % local.indexterm.attrib "">
<!ENTITY % indexterm.role.attrib "%role.attrib;">

<!ENTITY % indexterm.element "INCLUDE">
<![%indexterm.element;[
<!ELEMENT indexterm (primary?, ((secondary, ((tertiary, (see|seealso+)?)
		| see | seealso+)?) | see | seealso+)?)>
<!--end of indexterm.element-->]]>

<!-- Scope: Indicates which generated indices the IndexTerm
		should appear in: Global (whole document set), Local (this
		document only), or All (both) -->
<!-- Significance: Whether this IndexTerm is the most pertinent
		of its series (Preferred) or not (Normal, the default) -->
<!-- Class: Indicates type of IndexTerm; default is Singular, 
		or EndOfRange if StartRef is supplied; StartOfRange value 
		must be supplied explicitly on starts of ranges -->
<!-- StartRef: ID of the IndexTerm that starts the indexing 
		range ended by this IndexTerm -->
<!-- Zone: IDs of the elements to which the IndexTerm applies,
		and indicates that the IndexTerm applies to those entire
		elements rather than the point at which the IndexTerm
		occurs -->


<!ENTITY % indexterm.attlist "INCLUDE">
<![%indexterm.attlist;[
<!ATTLIST indexterm
		%pagenum.attrib;
		scope		(all
				|global
				|local)		#IMPLIED
		significance	(preferred
				|normal)	"normal"
		class		(singular
				|startofrange
				|endofrange)	#IMPLIED
		startref		IDREF		#IMPLIED
		zone			IDREFS		#IMPLIED
		%common.attrib;
		%indexterm.role.attrib;
		%local.indexterm.attrib;
>
<!--end of indexterm.attlist-->]]>
<!--end of indexterm.module-->]]>

<!ENTITY % primsecter.module "INCLUDE">
<![%primsecter.module;[
<!ENTITY % local.primsecter.attrib "">
<!ENTITY % primsecter.role.attrib "%role.attrib;">


<!ENTITY % primary.element "INCLUDE">
<![%primary.element;[
<!ELEMENT primary   (%ndxterm.char.mix;)*>
<!--end of primary.element-->]]>
<!-- SortAs: Alternate sort string for index sorting, e.g.,
		"fourteen" for an element containing "14" -->

<!ENTITY % primary.attlist "INCLUDE">
<![%primary.attlist;[
<!ATTLIST primary
		sortas		CDATA		#IMPLIED
		%common.attrib;
		%primsecter.role.attrib;
		%local.primsecter.attrib;
>
<!--end of primary.attlist-->]]>


<!ENTITY % secondary.element "INCLUDE">
<![%secondary.element;[
<!ELEMENT secondary (%ndxterm.char.mix;)*>
<!--end of secondary.element-->]]>
<!-- SortAs: Alternate sort string for index sorting, e.g.,
		"fourteen" for an element containing "14" -->

<!ENTITY % secondary.attlist "INCLUDE">
<![%secondary.attlist;[
<!ATTLIST secondary
		sortas		CDATA		#IMPLIED
		%common.attrib;
		%primsecter.role.attrib;
		%local.primsecter.attrib;
>
<!--end of secondary.attlist-->]]>


<!ENTITY % tertiary.element "INCLUDE">
<![%tertiary.element;[
<!ELEMENT tertiary  (%ndxterm.char.mix;)*>
<!--end of tertiary.element-->]]>
<!-- SortAs: Alternate sort string for index sorting, e.g.,
		"fourteen" for an element containing "14" -->

<!ENTITY % tertiary.attlist "INCLUDE">
<![%tertiary.attlist;[
<!ATTLIST tertiary
		sortas		CDATA		#IMPLIED
		%common.attrib;
		%primsecter.role.attrib;
		%local.primsecter.attrib;
>
<!--end of tertiary.attlist-->]]>

<!--end of primsecter.module-->]]>

<!ENTITY % seeseealso.module "INCLUDE">
<![%seeseealso.module;[
<!ENTITY % local.seeseealso.attrib "">
<!ENTITY % seeseealso.role.attrib "%role.attrib;">

<!ENTITY % see.element "INCLUDE">
<![%see.element;[
<!ELEMENT see (%ndxterm.char.mix;)*>
<!--end of see.element-->]]>

<!ENTITY % see.attlist "INCLUDE">
<![%see.attlist;[
<!ATTLIST see
		%common.attrib;
		%seeseealso.role.attrib;
		%local.seeseealso.attrib;
>
<!--end of see.attlist-->]]>

<!ENTITY % seealso.element "INCLUDE">
<![%seealso.element;[
<!ELEMENT seealso (%ndxterm.char.mix;)*>
<!--end of seealso.element-->]]>

<!ENTITY % seealso.attlist "INCLUDE">
<![%seealso.attlist;[
<!ATTLIST seealso
		%common.attrib;
		%seeseealso.role.attrib;
		%local.seeseealso.attrib;
>
<!--end of seealso.attlist-->]]>
<!--end of seeseealso.module-->]]>
<!--end of indexterm.content.module-->]]>

<!-- End of DocBook XML information pool module V4.1.2 ...................... -->
<!-- ...................................................................... -->
