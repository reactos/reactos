<?php
/** Nedersaksisch (Nedersaksisch)
 *
 * @ingroup Language
 * @file
 *
 * @author Erwin85
 * @author Jens Frank
 * @author Servien
 * @author Slomox
 * @author Ævar Arnfjörð Bjarmason <avarab@gmail.com>
 * @author לערי ריינהארט
 */

$fallback = 'nl';

$skinNames = array(
	'standard'      => 'Klassiek',
	'nostalgia'     => 'Nostalgie',
	'cologneblue'   => 'Keuls blauw',
	'chick'         => 'Sjiek',
	'myskin'        => 'MienSkin',
);

$namespaceNames = array(
	NS_MEDIA            => 'Media',
	NS_SPECIAL          => 'Speciaal',
	NS_MAIN             => '',
	NS_TALK             => 'Overleg',
	NS_USER             => 'Gebruker',
	NS_USER_TALK        => 'Overleg_gebruker',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK     => 'Overleg_$1',
	NS_IMAGE            => 'Ofbeelding',
	NS_IMAGE_TALK       => 'Overleg_ofbeelding',
	NS_MEDIAWIKI        => 'MediaWiki',
	NS_MEDIAWIKI_TALK   => 'Overleg_MediaWiki',
	NS_TEMPLATE         => 'Sjabloon',
	NS_TEMPLATE_TALK    => 'Overleg_sjabloon',
	NS_HELP             => 'Hulpe',
	NS_HELP_TALK        => 'Overleg_hulpe',
	NS_CATEGORY         => 'Kattegerie',
	NS_CATEGORY_TALK    => 'Overleg_kattegerie'
);

$namespaceAliases = array(
	'Speciaol'          => NS_SPECIAL,
	'Categorie'         => NS_CATEGORY,
	'Overleg_categorie' => NS_CATEGORY_TALK,
	'Overleg_help'      => NS_HELP_TALK,
);

$dateFormats = array(
	'mdy time' => 'H:i',
	'mdy date' => 'M j, Y',
	'mdy both' => 'H:i, M j, Y',

	'dmy time' => 'H:i',
	'dmy date' => 'j M Y',
	'dmy both' => 'H:i, j M Y',

	'ymd time' => 'H:i',
	'ymd date' => 'Y M j',
	'ymd both' => 'H:i, Y M j',
);

$bookstoreList = array(
        'Koninklijke Bibliotheek' => 'http://opc4.kb.nl/DB=1/SET=5/TTL=1/CMD?ACT=SRCH&IKT=1007&SRT=RLV&TRM=$1'
);

$magicWords = array(
#   ID                                 CASE  SYNONYMS
        'redirect'               => array( 0,    '#REDIRECT', '#DEURVERWIEZING' ),
        'notoc'                  => array( 0,    '__NOTOC__', '__GIENONDERWARPEN__' ),
        'nogallery'              => array( 0,    '__NOGALLERY__', '__GIENGALLERIEJE__' ),
        'forcetoc'               => array( 0,    '__FORCETOC__', '__FORCEERONDERWARPEN__' ),
        'toc'                    => array( 0,    '__TOC__', '__ONDERWARPEN__' ),
        'noeditsection'          => array( 0,    '__NOEDITSECTION__', '__GIENBEWARKSECTIE__' ),
        'currentmonth'           => array( 1,    'CURRENTMONTH', 'DISSEMAOND' ),
        'currentmonthname'       => array( 1,    'CURRENTMONTHNAME', 'DISSEMAONDNAAM' ),
        'currentmonthnamegen'    => array( 1,    'CURRENTMONTHNAMEGEN', 'DISSEMAONDGEN' ),
        'currentmonthabbrev'     => array( 1,    'CURRENTMONTHABBREV', 'DISSEMAONDOFK' ),
        'currentday'             => array( 1,    'CURRENTDAY', 'DISSEDAG' ),
        'currentday2'            => array( 1,    'CURRENTDAY2', 'DISSEDAG2' ),
        'currentdayname'         => array( 1,    'CURRENTDAYNAME', 'DISSEDAGNAAM' ),
        'currentyear'            => array( 1,    'CURRENTYEAR', 'DITJAOR' ),
        'currenttime'            => array( 1,    'CURRENTTIME', 'DISSETIED' ),
        'currenthour'            => array( 1,    'CURRENTHOUR', 'DITURE' ),
        'localmonth'             => array( 1,    'LOCALMONTH', 'LOKALEMAOND' ),
        'localmonthname'         => array( 1,    'LOCALMONTHNAME', 'LOKALEMAONDNAAM' ),
        'localmonthnamegen'      => array( 1,    'LOCALMONTHNAMEGEN', 'LOKALEMAONDNAAMGEN' ),
        'localmonthabbrev'       => array( 1,    'LOCALMONTHABBREV', 'LOKALEMAONDOFK' ),
        'localday'               => array( 1,    'LOCALDAY', 'LOKALEDAG' ),
        'localday2'              => array( 1,    'LOCALDAY2', 'LOKALEDAG2' ),
        'localdayname'           => array( 1,    'LOCALDAYNAME', 'LOKALEDAGNAAM' ),
        'localyear'              => array( 1,    'LOCALYEAR', 'LOKAALJAOR' ),
        'localtime'              => array( 1,    'LOCALTIME', 'LOKALETIED' ),
        'localhour'              => array( 1,    'LOCALHOUR', 'LOKAALURE' ),
        'numberofpages'          => array( 1,    'NUMBEROFPAGES', 'ANTALPAGINAS', 'ANTALPAGINA\'S', 'ANTALPAGINA’S' ),
        'numberofarticles'       => array( 1,    'NUMBEROFARTICLES', 'ANTALARTIKELS' ),
        'numberoffiles'          => array( 1,    'NUMBEROFFILES', 'ANTALBESTANDEN' ),
        'numberofusers'          => array( 1,    'NUMBEROFUSERS', 'ANTALGEBRUKERS' ),
        'pagename'               => array( 1,    'PAGENAME', 'PAGINANAAM' ),
        'pagenamee'              => array( 1,    'PAGENAMEE', 'PAGINANAAME' ),
        'namespace'              => array( 1,    'NAMESPACE', 'NAAMRUUMTE' ),
        'namespacee'             => array( 1,    'NAMESPACEE', 'NAAMRUUMTEE' ),
        'talkspace'              => array( 1,    'TALKSPACE', 'OVERLEGRUUMTE' ),
        'talkspacee'             => array( 1,    'TALKSPACEE', 'OVERLEGRUUMTEE' ),
        'subjectspace'           => array( 1,    'SUBJECTSPACE', 'ARTICLESPACE', 'ONDERWARPRUUMTE', 'ARTIKELRUUMTE' ),
        'subjectspacee'          => array( 1,    'SUBJECTSPACEE', 'ARTICLESPACEE', 'ONDERWARPRUUMTEE', 'ARTIKELRUUMTEE' ),
        'fullpagename'           => array( 1,    'FULLPAGENAME', 'HELEPAGINANAAM' ),
        'fullpagenamee'          => array( 1,    'FULLPAGENAMEE', 'HELEPAGINANAAME' ),
        'subpagename'            => array( 1,    'SUBPAGENAME', 'DEELPAGINANAAM' ),
        'subpagenamee'           => array( 1,    'SUBPAGENAMEE', 'DEELPAGINANAAME' ),
        'basepagename'           => array( 1,    'BASEPAGENAME', 'BAOSISPAGINANAAM' ),
        'basepagenamee'          => array( 1,    'BASEPAGENAMEE', 'BAOSISPAGINANAAME' ),
        'talkpagename'           => array( 1,    'TALKPAGENAME', 'OVERLEGPAGINANAAM' ),
        'talkpagenamee'          => array( 1,    'TALKPAGENAMEE', 'OVERLEGPAGINANAAME' ),
        'subjectpagename'        => array( 1,    'SUBJECTPAGENAME', 'ARTICLEPAGENAME', 'ONDERWARPPAGINANAAM', 'ARTIKELPAGINANAAM' ),
        'subjectpagenamee'       => array( 1,    'SUBJECTPAGENAMEE', 'ARTICLEPAGENAMEE', 'ONDERWARPPAGINANAAME', 'ARTIKELPAGINANAAME' ),
        'msg'                    => array( 0,    'MSG:', 'BERICH:' ),
        'msgnw'                  => array( 0,    'MSGNW:', 'BERICHNW' ),
        'img_right'              => array( 1,    'right', 'rechs' ),
        'img_left'               => array( 1,    'left', 'links' ),
        'img_none'               => array( 1,    'none', 'gien' ),
        'img_center'             => array( 1,    'center', 'centre', 'ecentreerd' ),
        'img_framed'             => array( 1,    'framed', 'enframed', 'frame', 'umraand' ),
        'img_page'               => array( 1,    'page=$1', 'page $1', 'pagina=$1', 'pagina $1' ),
        'img_baseline'           => array( 1,    'baseline', 'grondliende' ),
        'img_top'                => array( 1,    'top', 'boven' ),
        'img_text_top'           => array( 1,    'text-top', 'tekse-boven' ),
        'img_middle'             => array( 1,    'middle', 'midden' ),
        'img_bottom'             => array( 1,    'bottom', 'ummeneer' ),
        'img_text_bottom'        => array( 1,    'text-bottom', 'tekse-ummeneer' ),
        'sitename'               => array( 1,    'SITENAME', 'WEBSTEENAAM' ),
        'ns'                     => array( 0,    'NS:', 'NR:' ),
        'localurl'               => array( 0,    'LOCALURL:', 'LOKALEURL' ),
        'localurle'              => array( 0,    'LOCALURLE:', 'LOKALEURLE' ),
        'servername'             => array( 0,    'SERVERNAME', 'SERVERNAAM' ),
        'scriptpath'             => array( 0,    'SCRIPTPATH', 'SCRIPTPAD' ),
        'grammar'                => array( 0,    'GRAMMAR:', 'GRAMMATICA:' ),
        'notitleconvert'         => array( 0,    '__NOTITLECONVERT__', '__NOTC__', '__GIENTITELCONVERSIE__', '__GIENTC__' ),
        'nocontentconvert'       => array( 0,    '__NOCONTENTCONVERT__', '__NOCC__', '__GIENINHOUDCONVERSIE__', '__GIENIC__' ),
        'currentweek'            => array( 1,    'CURRENTWEEK', 'DISSEWEKE' ),
        'currentdow'             => array( 1,    'CURRENTDOW', 'DISSEDVDW' ),
        'localweek'              => array( 1,    'LOCALWEEK', 'LOKALEWEKE' ),
        'localdow'               => array( 1,    'LOCALDOW', 'LOKALEDVDW' ),
        'revisionid'             => array( 1,    'REVISIONID', 'REVISIEID', 'REVISIE-ID' ),
        'revisionday'            => array( 1,    'REVISIONDAY', 'REVISIEDAG' ),
        'revisionday2'           => array( 1,    'REVISIONDAY2', 'REVISIEDAG2' ),
        'revisionmonth'          => array( 1,    'REVISIONMONTH', 'REVISIEMAOND' ),
        'revisionyear'           => array( 1,    'REVISIONYEAR', 'REVISIEJAOR' ),
        'revisiontimestamp'      => array( 1,    'REVISIONTIMESTAMP', 'REVISIETIEDSTEMPEL' ),
        'plural'                 => array( 0,    'PLURAL:', 'MEERVOUD:' ),
        'fullurl'                => array( 0,    'FULLURL:', 'HELEURL' ),
        'fullurle'               => array( 0,    'FULLURLE:', 'HELEURLE' ),
        'lcfirst'                => array( 0,    'LCFIRST:', 'HLEERSTE:' ),
        'ucfirst'                => array( 0,    'UCFIRST:', 'KLEERSTE:' ),
        'lc'                     => array( 0,    'LC:', 'KL:' ),
        'uc'                     => array( 0,    'UC:', 'HL:' ),
        'raw'                    => array( 0,    'RAW:', 'RAUW:' ),
        'displaytitle'           => array( 1,    'DISPLAYTITLE', 'TEUNTITEL' ),
        'newsectionlink'         => array( 1,    '__NEWSECTIONLINK__', '__NIEJESECTIEVERWIEZING__' ),
        'currentversion'         => array( 1,    'CURRENTVERSION', 'DISSEVERSIE' ),
        'urlencode'              => array( 0,    'URLENCODE:', 'CODEERURL' ),
        'anchorencode'           => array( 0,    'ANCHORENCODE', 'CODEERANKER' ),
        'currenttimestamp'       => array( 1,    'CURRENTTIMESTAMP', 'DISSETIEDSTEMPEL' ),
        'localtimestamp'         => array( 1,    'LOCALTIMESTAMP', 'LOKALETIEDSTEMPEL' ),
        'directionmark'          => array( 1,    'DIRECTIONMARK', 'DIRMARK', 'RICHTINGMARKERING', 'RICHTINGSMARKERING' ),
        'language'               => array( 0,    '#LANGUAGE:', '#TAAL:' ),
        'contentlanguage'        => array( 1,    'CONTENTLANGUAGE', 'CONTENTLANG', 'INHOUDSTAAL' ),
        'pagesinnamespace'       => array( 1,    'PAGESINNAMESPACE:', 'PAGESINNS:', 'PAGINASINNAAMRUUMTE', 'PAGINA’SINNAAMRUUMTE', 'PAGINA\'SINNAAMRUUMTE' ),
        'numberofadmins'         => array( 1,    'NUMBEROFADMINS', 'ANTALBEHEERDERS' ),
        'formatnum'              => array( 0,    'FORMATNUM', 'FORMATTEERNUM' ),
        'padleft'                => array( 0,    'PADLEFT', 'LINKSOPVULLEN' ),
        'padright'               => array( 0,    'PADRIGHT', 'RECHSOPVULLEN' ),
        'special'                => array( 0,    'special', 'speciaal' ),
        'defaultsort'            => array( 1,    'DEFAULTSORT:', 'STANDARDSORTERING:' )
);

$specialPageAliases = array(
	'DoubleRedirects'           => array( 'Dubbele_deurverwiezingen' ),
	'BrokenRedirects'           => array( 'Ebreuken_deurverwiezingen' ),
	'Disambiguations'           => array( 'Deurverwiespagina\'s' ),
	'Userlogin'                 => array( 'Anmelden' ),
	'Userlogout'                => array( 'Ofmelden' ),
	'Preferences'               => array( 'Veurkeuren' ),
	'Watchlist'                 => array( 'Volglieste' ),
	'Recentchanges'             => array( 'Leste_wiezigingen' ),
	'Upload'                    => array( 'Bestanden_toevoegen' ),
	'Imagelist'                 => array( 'Ofbeeldingenlieste' ),
	'Newimages'                 => array( 'Nieje_ofbeeldingen' ),
	'Listusers'                 => array( 'Gebrukerslieste' ),
	'Statistics'                => array( 'Staotestieken' ),
	'Randompage'                => array( 'Willekeurige_pagina' ),
	'Lonelypages'               => array( 'Weespagina\'s' ),
	'Uncategorizedpages'        => array( 'Pagina\'s_zonder_kattegerie' ),
	'Uncategorizedcategories'   => array( 'Kattergieën_zonder_kattegerie' ),
	'Uncategorizedimages'       => array( 'Ofbeeldingen_zonder_kattegerie' ),
	'Unusedcategories'          => array( 'Ongebruken_kattegerieën' ),
	'Unusedimages'              => array( 'Ongebruken_ofbeeldingen' ),
	'Wantedpages'               => array( 'Gewunste_pagina\'s' ),
	'Wantedcategories'          => array( 'Gewunste_kattegerieën' ),
	'Mostlinked'                => array( 'Meest_naor_verwezen_pagina\'s' ),
	'Mostlinkedcategories'      => array( 'Meestgebruken_kattegerieën' ),
	'Mostcategories'            => array( 'Meeste_kattegerieën' ),
	'Mostimages'                => array( 'Meeste_ofbeeldingen' ),
	'Mostrevisions'             => array( 'Meeste_bewarkingen' ),
	'Fewestrevisions'           => array( 'Minste_bewarkingen' ),
	'Shortpages'                => array( 'Korte_artikels' ),
	'Longpages'                 => array( 'Lange_artikels' ),
	'Newpages'                  => array( 'Nieje_pagina\'s' ),
	'Ancientpages'              => array( 'Oudste_pagina\'s' ),
	'Deadendpages'              => array( 'Doodlopende_deurverwiezingen' ),
	'Protectedpages'            => array( 'Beveiligen_pagina\'s' ),
	'Allpages'                  => array( 'Alle_pagina\'s' ),
	'Prefixindex'               => array( 'Prefixindex' ),
	'Ipblocklist'               => array( 'IP-blokkeerlieste' ),
	'Specialpages'              => array( 'Speciale_pagina\'s' ),
	'Contributions'             => array( 'Biedragen' ),
	'Emailuser'                 => array( 'Berich_sturen' ),
	'Whatlinkshere'             => array( 'Verwiezingen_naor_disse_pagina' ),
	'Recentchangeslinked'       => array( 'Volg_verwiezingen' ),
	'Movepage'                  => array( 'Herneum_pagina' ),
	'Blockme'                   => array( 'Blokkeer_mien' ),
	'Booksources'               => array( 'Boekinfermasie' ),
	'Categories'                => array( 'Kattegerieën' ),
	'Export'                    => array( 'Uutvoeren' ),
	'Version'                   => array( 'Versie' ),
	'Allmessages'               => array( 'Alle_systeemteksen' ),
	'Log'                       => array( 'Log', 'Logs' ),
	'Blockip'                   => array( 'Blokkeer_IP' ),
	'Undelete'                  => array( 'Weerummeplaosen' ),
	'Import'                    => array( 'Invoeren' ),
	'Lockdb'                    => array( 'Databanke_blokkeren' ),
	'Unlockdb'                  => array( 'Databanke_vriegeven' ),
	'Userrights'                => array( 'Gebrukersrechen' ),
	'MIMEsearch'                => array( 'MIME-zeuken' ),
	'Unwatchedpages'            => array( 'Neet-evolgen_pagina\'s' ),
	'Listredirects'             => array( 'Deurverwiezingslieste' ),
	'Revisiondelete'            => array( 'Versie_vortdoon' ),
	'Unusedtemplates'           => array( 'Ongebruken_sjablonen' ),
	'Randomredirect'            => array( 'Willekeurige_deurverwiezing' ),
	'Mypage'                    => array( 'Mien_gebrukerspagina' ),
	'Mytalk'                    => array( 'Mien_overleg' ),
	'Mycontributions'           => array( 'Mien_biedragen' ),
	'Listadmins'                => array( 'Beheerderslieste' ),
	'Popularpages'              => array( 'Populaire_artikels' ),
	'Search'                    => array( 'Zeuken' ),
	'Resetpass'                 => array( 'Wachwoord_opniej_instellen' ),
	'Withoutinterwiki'          => array( 'Gien_interwiki' ),
	);

$linkTrail = '/^([a-zäöüïëéèà]+)(.*)$/sDu';

$messages = array(
# User preference toggles
'tog-underline'               => 'Verwiezingen onderstrepen',
'tog-highlightbroken'         => "Verwiezingen naor lege pagina's op laoten lochen",
'tog-justify'                 => "Alinea's uutvullen",
'tog-hideminor'               => 'Kleine wiezigingen verbargen in leste wiezigingen',
'tog-extendwatchlist'         => 'Uut-ebreien volglieste',
'tog-usenewrc'                => 'Gebruuk de uut-ebreien pagina "leste wiezigingen" (hierveur he-j JavaScript neudig)',
'tog-numberheadings'          => 'Koppen vanzelf nummeren',
'tog-showtoolbar'             => 'Warkbalke weergeven',
'tog-editondblclick'          => 'Mit dubbelklik bewarken (JavaScript)',
'tog-editsection'             => 'Mit bewarkgedeeltes',
'tog-editsectiononrightclick' => 'Bewarkgedeelte mit rechtermuusknoppe bewarken (JavaScript)',
'tog-showtoc'                 => 'Samenvatting van de onderwarpen laoten zien (mit meer as dree onderwarpen)',
'tog-rememberpassword'        => 'Vanzelf anmelden',
'tog-editwidth'               => 'Bewarkingsveld over volle breedte',
'tog-watchcreations'          => 'Artikels dee-j anmaken an volglieste toevoegen',
'tog-watchdefault'            => 'Artikels dee-j wiezigen an volglieste toevoegen',
'tog-watchmoves'              => "Pagina's dee-k herneume op mien volglieste zetten",
'tog-watchdeletion'           => 'Voeg pagina dee-k vortdo an mien volglieste toe',
'tog-minordefault'            => "Markeer alle veraanderingen as 'kleine wieziging'",
'tog-previewontop'            => 'Naokiekpagina boven bewarkingsveld weergeven',
'tog-previewonfirst'          => 'Naokieken bie eerste wieziging',
'tog-nocache'                 => 'De kas uutschakelen',
'tog-enotifwatchlistpages'    => 'Stuur mien een berichjen over paginawiezigingen.',
'tog-enotifusertalkpages'     => 'Stuur mien een berichjen as mien overlegpagina ewiezig is.',
'tog-enotifminoredits'        => 'Stuur mien oek een berichjen bie kleine bewarkingen',
'tog-enotifrevealaddr'        => 'Mien e-mailadres weergeven in e-mailmededelingen',
'tog-shownumberswatching'     => 'Antal volgende gebrukers weergeven',
'tog-fancysig'                => 'ondertekening zonder verwiezing naor gebrukerspagina',
'tog-externaleditor'          => 'Gebruuk standard een externe teksbewarker',
'tog-externaldiff'            => 'Gebruuk standard een extern vergeliekingspregramma',
'tog-showjumplinks'           => 'Verwiezingen naor "navigasie" en "zeuken" weergeven bovenan pagina\'s in partie uterlijken (zoas Myskin)',
'tog-uselivepreview'          => 'Gebruuk "rechstreekse veurbeschouwing" (mu-j JavaScript veur hemmen - experimenteel)',
'tog-forceeditsummary'        => 'Geef een melding bie een lege samenvatting',
'tog-watchlisthideown'        => 'Verbarg mien eigen bewarkingen',
'tog-watchlisthidebots'       => 'Verbarg botgebrukers',
'tog-watchlisthideminor'      => 'Verbarg kleine wiezigingen in mien volglieste',
'tog-nolangconversion'        => 'Ummezetten van varianten uutschakelen',
'tog-ccmeonemails'            => 'Stuur mien kopieën van berichen an aandere gebrukers',
'tog-diffonly'                => 'Pagina-inhoud neet onder de an-egeven wiezigingen weergeven.',
'tog-showhiddencats'          => 'Verbörgen kattegerieën weergeven',

'underline-always'  => 'Altied',
'underline-never'   => 'Nooit',
'underline-default' => 'Standardinstelling',

'skinpreview' => '(bekieken)',

# Dates
'sunday'        => 'zundag',
'monday'        => 'maondag',
'tuesday'       => 'diensdag',
'wednesday'     => 'woonsdag',
'thursday'      => 'donderdag',
'friday'        => 'vriedag',
'saturday'      => 'zaoterdag',
'sun'           => 'zun',
'mon'           => 'mao',
'tue'           => 'die',
'wed'           => 'woo',
'thu'           => 'don',
'fri'           => 'vrie',
'sat'           => 'zao',
'january'       => 'jannewaori',
'february'      => 'febrewaori',
'march'         => 'meert',
'april'         => 'april',
'may_long'      => 'mei',
'june'          => 'juni',
'july'          => 'juli',
'august'        => 'augustus',
'september'     => 'september',
'october'       => 'oktober',
'november'      => 'november',
'december'      => 'december',
'january-gen'   => 'jannewaori',
'february-gen'  => 'febrewaori',
'march-gen'     => 'meert',
'april-gen'     => 'april',
'may-gen'       => 'mei',
'june-gen'      => 'juni',
'july-gen'      => 'juli',
'august-gen'    => 'augustus',
'september-gen' => 'september',
'october-gen'   => 'oktober',
'november-gen'  => 'november',
'december-gen'  => 'december',
'jan'           => 'jan',
'feb'           => 'feb',
'mar'           => 'mrt',
'apr'           => 'apr',
'may'           => 'mei',
'jun'           => 'jun',
'jul'           => 'juli',
'aug'           => 'aug',
'sep'           => 'sep',
'oct'           => 'okt',
'nov'           => 'nov',
'dec'           => 'dec',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|Kattegerie|Kattegerieën}}',
'category_header'                => 'Artikels in kattegerie $1',
'subcategories'                  => 'Subkattegerieën',
'category-media-header'          => 'Media in kattegerie "$1"',
'category-empty'                 => "''Disse kattegerie bevat op 't mement nog gien artikels of media.''",
'hidden-categories'              => 'Verbörgen {{PLURAL:$1|kattegerie|kattegerieën}}',
'hidden-category-category'       => 'Verbörgen kattegerieën', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|Disse kattegerie hef de volgende subkattegerie.|Disse kattegerie hef de volgende {{PLURAL:$1|subkattegerie|$1 subkattegerieën}}, van een totaal van $2.}}',
'category-subcat-count-limited'  => 'Disse kattegerie hef de volgende {{PLURAL:$1|subkattegerie|$1 subkattegerieën}}.',
'category-article-count'         => "{{PLURAL:$2|Disse kattegerie bevat de volgende pagina.|Disse kattegerie bevat de volgende {{PLURAL:$1|pagina|$1 pagina's}}, van in totaal $2.}}",
'category-article-count-limited' => "Disse kattegerie bevat de volgende {{PLURAL:$1|pagina|$1 pagina's}}.",
'category-file-count'            => "{{PLURAL:$2|Disse kattegerie bevat 't volgende bestand.|Disse kattegerie bevat {{PLURAL:$1|'t volgende bestand|de volgende $1 bestanden}}, van in totaal $2.}}",
'category-file-count-limited'    => "Disse kattegerie bevat {{PLURAL:$1|'t volgende bestand|de volgende $1 bestanden}}.",
'listingcontinuesabbrev'         => '(vervolg)',

'mainpagetext'      => "'t Installeren van de wikipregrammetuur is succesvol.",
'mainpagedocfooter' => "Raodpleeg de [http://meta.wikimedia.org/wiki/Help:Contents haandleiding] veur infermasie over 't gebruuk van de wikipregrammetuur.

== Meer hulpe ==
* [http://www.mediawiki.org/wiki/Help:Configuration_settings Lieste mit instellingen]
* [http://www.mediawiki.org/wiki/Help:FAQ MediaWiki-vragen dee vake esteld wonnen]
* [http://mail.wikimedia.org/mailman/listinfo/mediawiki-announce MediaWiki-poslieste veur nieje versies]",

'about'          => 'Infermasie',
'article'        => 'artikel',
'newwindow'      => '(niej vienster)',
'cancel'         => 'Annuleren',
'qbfind'         => 'Zeuken',
'qbbrowse'       => 'Blaojen',
'qbedit'         => 'Bewark',
'qbpageoptions'  => 'Pagina-opties',
'qbpageinfo'     => 'Pagina-infermasie',
'qbmyoptions'    => 'Veurkeuren',
'qbspecialpages' => "Speciale pagina's",
'moredotdotdot'  => 'Meer...',
'mypage'         => 'Mien gebrukerspagina',
'mytalk'         => 'Mien overleg',
'anontalk'       => 'Overlegpagina veur dit IP-adres',
'navigation'     => 'Navigasie',
'and'            => 'en',

# Metadata in edit box
'metadata_help' => 'Metadata:',

'errorpagetitle'    => 'Foutmelding',
'returnto'          => 'Weerumme naor $1.',
'tagline'           => 'Van {{SITENAME}}',
'help'              => 'Hulp en kontak',
'search'            => 'Zeuken',
'searchbutton'      => 'Zeuken',
'go'                => 'artikel',
'searcharticle'     => 'artikel',
'history'           => 'Geschiedenisse',
'history_short'     => 'Geschiedenisse',
'updatedmarker'     => 'bie-ewark sins mien leste bezeuk',
'info_short'        => 'Infermasie',
'printableversion'  => 'Ofdrokbaore versie',
'permalink'         => 'Vaste verwiezing',
'print'             => 'Ofdrokken',
'edit'              => 'Bewark',
'create'            => 'Anmaken',
'editthispage'      => 'Pagina bewarken',
'create-this-page'  => 'Disse pagina anmaken',
'delete'            => 'vortdoon',
'deletethispage'    => 'Pagina vortdoon',
'undelete_short'    => '$1 {{PLURAL:$1|versie|versies}} weerummeplaosen',
'protect'           => 'Beveiligen',
'protect_change'    => 'wiezigen',
'protectthispage'   => 'Beveiligen',
'unprotect'         => 'ontgrendelen',
'unprotectthispage' => 'Beveiliging opheffen',
'newpage'           => 'Nieje pagina',
'talkpage'          => 'Overlegpagina',
'talkpagelinktext'  => 'Overleeg',
'specialpage'       => 'speciale pagina',
'personaltools'     => 'Persoonlijke instellingen',
'postcomment'       => 'Opmarking plaosen',
'articlepage'       => 'Artikel',
'talk'              => 'Overleeg',
'views'             => 'Aspekken/acties',
'toolbox'           => 'Hulpmiddels',
'userpage'          => 'gebrukerspagina',
'projectpage'       => 'Bekiek prejekpagina',
'imagepage'         => 'Beschrievingspagina',
'mediawikipage'     => 'Berichpagina bekieken',
'templatepage'      => 'Sjabloonpagina bekieken',
'viewhelppage'      => 'Hulppagina bekieken',
'categorypage'      => 'Kattegeriepagina bekieken',
'viewtalkpage'      => 'Teun overlegpagina',
'otherlanguages'    => "Interwiki's",
'redirectedfrom'    => '(deur-estuurd vanof "$1")',
'redirectpagesub'   => 'Deurstuurpagina',
'lastmodifiedat'    => "Disse pagina is 't les ewiezig op $1 um $2.", # $1 date, $2 time
'viewcount'         => 'Disse pagina is $1 {{PLURAL:$1|keer|keer}} bekeken.',
'protectedpage'     => 'Beveiligen pagina',
'jumpto'            => 'Gao naor:',
'jumptonavigation'  => 'navigasie',
'jumptosearch'      => 'zeuk',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'Over {{SITENAME}}',
'aboutpage'            => 'Project:Info',
'bugreports'           => 'Kontak',
'bugreportspage'       => 'Project:Foutrepporten',
'copyright'            => 'De inhold is beschikbaor onder de $1.',
'copyrightpagename'    => '{{SITENAME}}-auteursrechen',
'copyrightpage'        => '{{ns:project}}:Auteursrechen',
'currentevents'        => "In 't niejs",
'currentevents-url'    => "Project:In 't niejs",
'disclaimers'          => 'Veurbehold',
'disclaimerpage'       => 'Project:Veurbehold',
'edithelp'             => "Hulpe bie 't bewarken",
'edithelppage'         => 'Help:Uutleg',
'faq'                  => 'Vragen dee vake esteld wonnen',
'faqpage'              => 'Project:Vragen dee vake esteld wonnen',
'helppage'             => 'Help:Inhold',
'mainpage'             => 'Veurpagina',
'mainpage-description' => 'Veurpagina',
'policy-url'           => 'Project:Beleid',
'portal'               => 'Gebrukerspertaol',
'portal-url'           => 'Project:Gebrukerspertaol',
'privacy'              => 'Gegevensbeleid',
'privacypage'          => 'Project:Gegevensbeleid',

'badaccess'        => 'Gien toestemming',
'badaccess-group0' => 'Je hemmen gien toestemming um disse actie uut te voeren.',
'badaccess-group1' => 'Disse actie kan allinnig uut-evoerd wonnen deur gebrukers dee tot de groep $1 beheuren.',
'badaccess-group2' => 'Disse actie kan allinnig uut-evoerd wonnen deur gebrukers dee tot een van groepen $1 beheuren.',
'badaccess-groups' => 'Disse actie kan allinnig uut-evoerd wonnen deur gebrukers dee tot een van de groepen $1 beheuren.',

'versionrequired'     => 'Versie $1 van MediaWiki is neudig',
'versionrequiredtext' => 'Versie $1 van MediaWiki is neudig um disse pagina te gebruken. Zie [[Special:Version|Versie]].',

'ok'                      => 'Oké',
'retrievedfrom'           => 'Van "$1"',
'youhavenewmessages'      => 'Je hemmen $1 ($2).',
'newmessageslink'         => 'een niej berich',
'newmessagesdifflink'     => 'wieziging weergeven',
'youhavenewmessagesmulti' => 'Je hemmen een niej berich op $1',
'editsection'             => 'bewark',
'editold'                 => 'bewark',
'viewsourceold'           => 'brontekse bekieken',
'editsectionhint'         => 'Bewarkingsveld: $1',
'toc'                     => 'Onderwarpen',
'showtoc'                 => 'Teun',
'hidetoc'                 => 'Verbarg',
'thisisdeleted'           => 'Bekieken of herstellen van $1?',
'viewdeleted'             => 'Bekiek $1?',
'restorelink'             => '{{PLURAL:$1|versie dee vort-edaon is|versies dee vort-edaon bin}}',
'feedlinks'               => 'Kenaal:',
'feed-invalid'            => 'Ongeldig abonnementstype.',
'feed-unavailable'        => 'Syndicasiefeeds bin neet beschikbaor',
'site-rss-feed'           => '$1 RSS-feed',
'site-atom-feed'          => '$1 Atom-feed',
'page-rss-feed'           => '"$1" RSS-feed',
'page-atom-feed'          => '"$1" Atom-feed',
'red-link-title'          => '$1 (besteet nog neet)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Artikel',
'nstab-user'      => 'Gebruker',
'nstab-media'     => 'Media',
'nstab-special'   => 'Speciaal',
'nstab-project'   => 'Prejekpagina',
'nstab-image'     => 'Ofbeelding',
'nstab-mediawiki' => 'Berich',
'nstab-template'  => 'Sjabloon',
'nstab-help'      => 'Hulpe',
'nstab-category'  => 'Kattegerie',

# Main script and global functions
'nosuchaction'      => 'De op-egeven haandeling besteet neet',
'nosuchactiontext'  => 'De op-egeven haandeling wonnen neet herkend deur de MediaWiki-pregrammetuur.',
'nosuchspecialpage' => 'Der besteet gien speciale pagina mit disse naam',
'nospecialpagetext' => "<big>'''Disse speciale pagina wonnen neet herkend deur de pregrammetuur.'''</big>

Een lieste mit bestaonde speciale pagina ku-j vienen op [[Special:SpecialPages|{{int:specialpages}}]].",

# General errors
'error'                => 'Foutmelding',
'databaseerror'        => 'Fout in de databanke',
'dberrortext'          => 'Bie \'t zeuken is een syntaxfout in de databanke op-etrejen.
De oorzake hiervan kan dujen op een fout in de pregrammetuur.

De leste zeukpoging in de databanke was:
<blockquote><tt>$1</tt></blockquote>
vanuut de functie "<tt>$2</tt>".
MySQL gaf de foutmelding "<tt>$3: $4</tt>".',
'dberrortextcl'        => 'Bie \'t opvragen van de databanke is een syntaxfout op-etrejen. De leste opdrach was:
"$1"
Vanuut de functie "$2"
MySQL gaf de volgende foutmelding: "$3: $4".',
'noconnect'            => 'De wiki hef technische preblemen en kan de databanke neet bereiken.<br />
$1',
'nodb'                 => 'Selectie van databanke $1 is neet meugelijk.',
'cachederror'          => 'Hieronder wonnen een versie uut de kas weer-egeven. Dit is meschien neet de leste versie.',
'laggedslavemode'      => "<strong>Waorschuwing:</strong> 't is meugelijk dat leste wiezigingen in de tekse van dit artikel nog neet verwark bin.",
'readonly'             => 'De databanke is beveilig',
'enterlockreason'      => "Geef een rejen veur de blokkering op en hoelange 't geet duren. De op-egeven rejen zal an de gebrukers eteund wonnen.",
'readonlytext'         => "De databanke van {{SITENAME}} is noen esleuten veur nieje bewarkingen en wiezigingen, werschienlijk veur bestansonderhoud. De verantwoordelijke systeembeheerder gaf hierveur de volgende rejen op: '''$1'''",
'missing-article'      => 'In de databanke steet gien tekse veur de pagina "$1" dee der wel in zol mutten staon ($2).

Dit kan koemen deurda-j een ouwe verwiezing naor \'t verschil tussen twee versies van een pagina volgen of een versie opvragen dee vort-edaon is.

As dat neet zo is, dan he-j meschien een fout in de pregremmetuur evunnen.
Meld \'t dan effen bie een [[Special:ListUsers/sysop|systeembeheerder]] van {{SITENAME}} en vermeld derbie de internetverwiezing van disse pagina.',
'missingarticle-rev'   => '(versienummer: $1)',
'missingarticle-diff'  => '(Wieziging: $1, $2)',
'readonly_lag'         => 'De databanke is autematisch beveilig, zodat de onder-eschikken servers zich kunnen synchroniseren mit de centrale server.',
'internalerror'        => 'Interne fout',
'internalerror_info'   => 'Interne fout: $1',
'filecopyerror'        => 'Kon bestand "$1" neet naor "$2" kopiëren.',
'filerenameerror'      => 'Bestandnaamwieziging "$1" naor "$2" neet meugelijk.',
'filedeleteerror'      => 'Kon bestand "$1" neet vortdoon.',
'directorycreateerror' => 'Map "$1" kon neet an-emaak wonnen.',
'filenotfound'         => 'Kon bestand "$1" neet vienen.',
'fileexistserror'      => 'Schrieven naor bestand "$1" was neet meugelijk: \'t bestand besteet al',
'unexpected'           => 'Onverwachen weerde: "$1"="$2".',
'formerror'            => 'Fout: kon formelier neet versturen',
'badarticleerror'      => 'Disse haandeling kan op disse pagina neet uut-evoerd wonnen.',
'cannotdelete'         => 'Kon de pagina of ofbeelding neet vort-edaon wonnen.',
'badtitle'             => 'Ongeldige naam',
'badtitletext'         => 'De naam van de op-evreugen pagina is neet geldig, leeg, of een interwiki-verwiezing naor een onbekende of ongeldige wiki.',
'perfdisabled'         => "Um overbelasting van 't systeem te veurkoemen, ku-j disse optie noen neet gebruken.",
'perfcached'           => 'Disse gegevens kwammen uut de kas en bin werschienlijk neet akteweel:',
'perfcachedts'         => 'De infermasie dee hieronder steet, is op-esleugen, en is van $1.',
'querypage-no-updates' => "Opwerderingen veur disse pagina bin op 't mement uut-eschakeld. Data zal noen neet verniejd wonnen.",
'wrong_wfQuery_params' => 'Parremeters veur wfQuery() wanen verkeerd<br />
Functie: $1<br />
Query: $2',
'viewsource'           => 'Brontekse bekieken',
'viewsourcefor'        => 'veur "$1"',
'actionthrottled'      => 'Haandeling tegen-ehuilen',
'actionthrottledtext'  => "As maotregel tegen 't ongewunst plaosen van verwiezingen naor aandere websteeën is 't antal keren da-j disse haandeling in een korte tied uutvoeren kunnen beteund. Je hemmen de limiet overschrejen. Prebeer 't over een antal menuten weer.",
'protectedpagetext'    => 'Disse pagina is beveilig um bewarkingen te veurkoemen.',
'viewsourcetext'       => 'Je kunnen de brontekse van disse pagina bewarken en bekieken:',
'protectedinterface'   => 'Disse pagina bevat een tekse dee gebruuk wonnen veur systeemteksen van de wiki. Allinnig beheerders kunnen disse pagina bewarken.',
'editinginterface'     => "'''Waorschuwing:''' je bewarken een pagina dee gebruuk wonnen deur de pregrammetuur. Wiezigingen dee an-ebröch wonnen op disse pagina zullen 't uterlijk veur iederene beïnvleujen. Overweeg veur vertalingen um [http://translatewiki.net/wiki/Main_Page?setlang=nds-nl Betawiki] te gebruken, 't vertalingsprejek veur MediaWiki.",
'sqlhidden'            => '(SQL-zeukopdrachte verbörgen)',
'cascadeprotected'     => 'Disse pagina is beveilig umdat \'t veurkump in de volgende {{PLURAL:$1|pagina|pagina\'s}}, dee beveilig {{PLURAL:$1|is|bin}} mit de "cascade"-optie:
$2',
'namespaceprotected'   => "Je bin neet bevoeg um pagina is de '''$1'''-naamruumte te bewarken.",
'customcssjsprotected' => 'Je kunnen disse pagina neet bewarken umdat der persoonlijke instellingen van een aandere gebruker in staon.',
'ns-specialprotected'  => "Speciale pagina's kunnen neet bewörk wonnen.",
'titleprotected'       => "'t Anmaken van disse pagina is beveilig deur [[User:$1|$1]].
De op-egeven rejen is ''$2''.",

# Virus scanner
'virus-badscanner'     => 'Slichte configurasie: onbekende virusscanner: <i>$1</i>',
'virus-scanfailed'     => 'scannen is mislok (code $1)',
'virus-unknownscanner' => 'onbekende virusscanner:',

# Login and logout pages
'logouttitle'                => 'Ofmelden gebruker',
'logouttext'                 => "<strong>Je bin noen of-emeld.</strong>

Je kunnen {{SITENAME}} noen anneniem gebruken of onder disse of een aandere gebrukersnaam je eigen weer anmelden.
't Kan ween dat der een antal pagina's weer-egeven wonnen asof je an-emeld bin totda-j de kas van joew webblaojeraar leegmaken.",
'welcomecreation'            => '<h2>Welkom, $1!</h2><p>Joew gebrukersprefiel is an-emaak. Je kunnen noen joew persoonlijke veurkeuren instellen.</p>',
'loginpagetitle'             => 'Gebrukersnaam',
'yourname'                   => 'Gebrukersnaam',
'yourpassword'               => 'Wachwoord',
'yourpasswordagain'          => 'Opniej invoeren',
'remembermypassword'         => 'vanzelf anmelden',
'yourdomainname'             => 'Joew domein',
'externaldberror'            => 'Der gung iets fout bie de externe authenticering, of je maggen je gebrukersprefiel neet bewarken.',
'loginproblem'               => "<b>Der was een prebleem mit 't anmelden.</b><br />Prebleem 't opniej!",
'login'                      => 'Anmelden',
'nav-login-createaccount'    => 'Anmelden',
'loginprompt'                => 'Je mutten cookies an hemmen staon um an te kunnen melden bie {{SITENAME}}.',
'userlogin'                  => 'Anmelden',
'logout'                     => 'Ofmelden',
'userlogout'                 => 'Ofmelden',
'notloggedin'                => 'Neet an-emeld',
'nologin'                    => 'He-j nog gien gebrukersnaam? $1.',
'nologinlink'                => 'Maak een gebrukersprefiel an',
'createaccount'              => 'Niej gebrukersprefiel anmaken',
'gotaccount'                 => 'Bi-j al eregistreerd? $1.',
'gotaccountlink'             => 'Anmelden',
'createaccountmail'          => 'per e-mail',
'badretype'                  => 'De wachwoorden dee-j in-etik hemmen bin neet liekeleens.',
'userexists'                 => 'Disse gebrukersnaam is al gebruuk. 
Kies een aandere naam.',
'youremail'                  => 'E-mailadres (neet verplich) *',
'username'                   => 'Gebrukersnaam:',
'uid'                        => 'Gebrukersnummer:',
'prefs-memberingroups'       => 'Lid van {{PLURAL:$1|groep|groepen}}:',
'yourrealname'               => 'Echte naam (neet verplich)',
'yourlanguage'               => 'Taal veur systeemteksen',
'yourvariant'                => 'Gewunste taal:',
'yournick'                   => 'Alias veur ondertekeningen',
'badsig'                     => 'Ongeldige haandtekening; controleer HTML.',
'badsiglength'               => "De naam is te lange; 't mut minder as {{PLURAL:$1|$1 letter|$1 letters}} hemmen.",
'email'                      => 'Privéberichen',
'prefs-help-realname'        => '* Echte naam (optioneel): a-j disse optie invullen zal joew echte naam gebruuk wonnen veur toekenningen veur joew warkzaamheen.',
'loginerror'                 => 'Anmeldingsfout',
'prefs-help-email'           => "E-mailadres is neet verplich, mar maak 't meugelijk um joew wachtwoord te e-mailen a-j 't vergeten bin.
Je kunnen oek aanderen in staot stellen per e-mail kontak mit joe op te nemen via een verwiezing op joew gebrukers- en overlegpagina zonder da-j joew identiteit priesgeven.",
'prefs-help-email-required'  => 'Hier he-w een e-mailadres veur neudig.',
'nocookiesnew'               => "Je gebrukersnaam is an-emaak, mar 't anmelden is mislok. Dit kump deurdat je webblaojeraar gien cookies an hef staon. Je kunnen de instelling van je webblaojeraar wiezigen, en daornao mit je nieje gebrukersnaam en wachwoord anmelden.",
'nocookieslogin'             => "'t Anmelden is mislok umdat je webblaojeraar gien cookies an hef staon. Prebeer 't accepteren van cookies an te zetten en daornao opniej an te melden.",
'noname'                     => 'Je mutten een gebrukersnaam opgeven.',
'loginsuccesstitle'          => 'Succesvol an-emeld',
'loginsuccess'               => 'Je bin noen an-emeld bie {{SITENAME}} as "$1".',
'nosuchuser'                 => 'De gebruker "$1" besteet neet.
Contreleer de spelling of [[Special:Userlogin/signup|maak een nieje gebruker an]].',
'nosuchusershort'            => 'Der is gien gebruker mit de naam "$1". Controleer de schriefwieze.',
'nouserspecified'            => 'Vul asjeblief een naam in',
'wrongpassword'              => "verkeerd wachwoord, prebeer 't opniej.",
'wrongpasswordempty'         => "Gien wachwoord in-evoerd. Prebeer 't opniej.",
'passwordtooshort'           => "Wachwoord is te kort.
't Mut uut minstens $1 {{PLURAL:$1|teken|tekens}} bestaon.",
'mailmypassword'             => 'Niej wachwoord opsturen',
'passwordremindertitle'      => 'niej tiedelik wachwoord veur {{SITENAME}}',
'passwordremindertext'       => 'Iemand vanof \'t IP-adres $1 (werschienlijk jiezelf) hef evreugen um een niej wachwoord veur {{SITENAME}} ($4) toe te sturen. \'t Nieje wachwoord veur gebruker "$2" is "$3". Advies: noen anmelden en \'t wachwoord wiezigigen.',
'noemail'                    => 'Gien e-mailadres eregistreerd veur "$1".',
'passwordsent'               => 'Der is een niej wachwoord verstuurd naor \'t e-mailadres van gebruker "$1". Meld an, a-j \'t wachwoord ontvangen.',
'blocked-mailpassword'       => 'Dit IP-adres is eblokkeerd. Dit betekent da-j neet bewarken kunnen en dat {{SITENAME}} joew wachwoord neet weerummehaolen kan, dit wonnen edaon um misbruuk tegen te gaon.',
'eauthentsent'               => "Der is een bevestigingsberich naor 't op-egeven e-mailadres verstuurd. Veurdat der veerdere berichen naor dit e-mailadres verstuurd kunnen wonnen, mu-j de instructies volgen in 't toe-esturen berich, um te bevestigen da-j joe eigen daodwarkelijk an-emeld hemmen.",
'throttled-mailpassword'     => 'In de leste {{PLURAL:$1|uur|$1 ure}} is der al een wachwoordherinnering estuurd.
Um misbruuk te veurkoemen wonnen der mar één wachwoordherinnering per {{PLURAL:$1|uur|$1 ure}} verzunnen.',
'mailerror'                  => "Fout bie 't versturen van berich: $1",
'acct_creation_throttle_hit' => 'Je hemmen al $1 gebrukersnamen an-emaak. Je kunnen der neet nog meer anmaken.',
'emailauthenticated'         => 'Joew e-mailadres is bevestig op $1.',
'emailnotauthenticated'      => 'E-mailadres is <strong>nog neet bevestig</strong>. Je ontvangen gien berichen veur de onstaonde opties.',
'noemailprefs'               => '<strong>Gien e-mailadres in-evoerd</strong>, waordeur de onderstaonde functies neet warken.',
'emailconfirmlink'           => 'Bevestig e-mailadres',
'invalidemailaddress'        => "'t E-mailadres kon neet eaccepteerd wonnen umdat de opmaak ongeldig is. 
Voer de juuste opmaak van 't adres in of laot 't veld leeg.",
'accountcreated'             => 'Gebrukersprefiel is an-emaak',
'accountcreatedtext'         => 'De gebrukersnaam veur $1 is an-emaak.',
'createaccount-title'        => 'Gebrukers anmaken veur {{SITENAME}}',
'createaccount-text'         => 'Der hef der ene een gebruker veur $2 an-emaak op {{SITENAME}} ($4). \'t Wachwoord veur "$2" is "$3".
Meld je noen an en wiezig \'t wachwoord.

Negeer dit berich as disse gebruker zonder joew toestemming an-emaak is.',
'loginlanguagelabel'         => 'Taal: $1',

# Password reset dialog
'resetpass'               => 'Wachwoord opniej instellen',
'resetpass_announce'      => "Je bin an-emeld mit een veurlopige code dee per e-mail toe-estuurd wonnen. Um 't anmelden te voltooien, mu-j een niej wachwoord invoeren:",
'resetpass_text'          => '<!-- Tekse hier invoegen -->',
'resetpass_header'        => 'Wachwoord opniej instellen',
'resetpass_submit'        => "Voer 't wachwoord in en meld je an",
'resetpass_success'       => 'Joew wachwoord is succesvol ewiezig. Je wonnen noen an-emeld...',
'resetpass_bad_temporary' => 'Ongeldig tiejelijk wachwoord. Je hemmen joew wachwoord al ewiezig of een niej tiejelijk wachwoord an-evreugen.',
'resetpass_forbidden'     => 'Wachwoorden kunnen neet ewiezig wonnen',
'resetpass_missing'       => 'Je hemmen gien wachwoord op-egeven.',

# Edit page toolbar
'bold_sample'     => 'Vet-edrokken tekse',
'bold_tip'        => 'Vet-edrokken tekse',
'italic_sample'   => 'Schunedrokken tekse',
'italic_tip'      => 'Schunedrok',
'link_sample'     => 'Onderwarp',
'link_tip'        => 'Interne verwiezing',
'extlink_sample'  => 'http://www.example.com linktekst',
'extlink_tip'     => 'Uutgaonde verwiezing',
'headline_sample' => 'Deelonderwarp',
'headline_tip'    => 'Deelonderwarp',
'math_sample'     => 'a^2 + b^2 = c^2',
'math_tip'        => 'Wiskundige formule (in LaTeX)',
'nowiki_sample'   => 'Tekse zonder wiki-opmaak.',
'nowiki_tip'      => 'Gien wiki-opmaak toepassen',
'image_sample'    => 'Veurbeeld.jpg',
'image_tip'       => 'Ofbeelding',
'media_sample'    => 'Veurbeeld.ogg',
'media_tip'       => 'Verwiezing naor bestand',
'sig_tip'         => 'Joew ondertekening (mit daotum en tied)',
'hr_tip'          => 'Horizontale liende',

# Edit pages
'summary'                          => 'Samenvatting',
'subject'                          => 'Onderwarp',
'minoredit'                        => 'kleine wieziging / spelling',
'watchthis'                        => 'volg disse pagina',
'savearticle'                      => 'Pagina opslaon',
'preview'                          => 'Naokieken',
'showpreview'                      => 'Pagina naokieken',
'showlivepreview'                  => 'Drekte weergave',
'showdiff'                         => 'Teun wiezigingen',
'anoneditwarning'                  => "'''Waorschuwing:''' Je bin neet an-emeld.
As annenieme gebruker zal joew IP-adres bie elke bewarking veur iederene zichbaor ween.",
'missingsummary'                   => "'''Herinnering:''' je hemmen gien samenvatting op-egeven veur de bewarking. A-j noen weer op ''Pagina opslaon'' klikken wonnen de bewarking zonder samenvatting op-esleugen.",
'missingcommenttext'               => 'Plaos joew opmarking hieronder.',
'missingcommentheader'             => "'''Let wel:''' je hemmen gien onderwarptitel toe-evoeg. A-j opniej op Pagina opslaon klikken wonnen de bewarking op-esleugen zonder onderwarptitel.",
'summary-preview'                  => 'Samenvatting naokieken',
'subject-preview'                  => 'Onderwarp/kop naokieken',
'blockedtitle'                     => 'Gebruker is eblokkeerd',
'blockedtext'                      => "<big>'''Joew gebrukersnaam of IP-adres is eblokkeerd.'''</big>

Je bin eblokkeerd deur: $1.
De op-egeven rejen is: ''$2''.

* Eblokkeerd vanof: $8
* Eblokkeerd tot: $6
* Bedoeld um te blokkeren: $7

Je kunnen kontak opnemen mit $1 of een aandere [[{{MediaWiki:Grouppage-sysop}}|beheerder]] um de blokkering te bepraoten.
Je kunnen gien gebruukmaken van de functie 'een berich sturen', behalven a-j een geldig e-mailadres op-egeven hemmen in joew [[Special:Preferences|veurkeuren]] en 't gebruuk van disse functie neet eblokkeerd is.
't IP-adres da-j noen gebruken is $3 en 't blokkeringsnummer is #$5. 
Vermeld 't allebeie a-j argens op disse blokkering reageren.",
'autoblockedtext'                  => 'Joew IP-adres is autematisch eblokkeerd umdat \'t gebruuk wönnen deur een aandere gebruker, dee eblokkeerd wönnen deur $1.
De rejen hierveur was:

:\'\'$2\'\'

* Begint: $8
* Verloop nao: $6
* Wee eblokkeerd wonnen: $7

Je kunnen kontak opnemen mit $1 of een van de aandere
[[{{MediaWiki:Grouppage-sysop}}|beheerders]] um de blokkering te bepraoten.

NB: je kunnen de optie "een berich sturen" neet gebruken, behalven a-j een geldig e-mailadres op-egeven hemmen in de [[Special:Preferences|gebrukersveurkeuren]] en je neet eblokkeerd bin.

Joew IP-adres is $3 en joew blokkeernummer is $5. 
Geef disse nummers deur a-j kontak mit ene opnemen over de blokkering.',
'blockednoreason'                  => 'gien rejen op-egeven',
'blockedoriginalsource'            => "De brontekse van '''$1''' wonnen hieronder weer-egeven:",
'blockededitsource'                => "De tekse van '''joew eigen bewarkingen''' an '''$1''' wonnen hieronder weer-egeven:",
'whitelistedittitle'               => 'Um disse pagina te bewarken, mu-j je anmelden',
'whitelistedittext'                => "Um pagina's te kunnen wiezigen, mu-j $1 ween",
'confirmedittitle'                 => 'Berichbevestiging is neudig um te bewarken.',
'confirmedittext'                  => "Je mutten je e-mailadres bevestigen veurda-j bewarken kunnen. Vul je adres in en bevestig 't via [[Special:Preferences|mien veurkeuren]].",
'nosuchsectiontitle'               => 'Disse sectie besteet neet',
'nosuchsectiontext'                => 'Je preberen een sectie te bewarken dat neet besteet. Umdat der gien sectie $1 is, is der gien plaos um joew bewarking op te slaon.',
'loginreqtitle'                    => 'Anmelden verplich',
'loginreqlink'                     => 'Anmelden',
'loginreqpagetext'                 => 'Je mutten $1 um disse pagina te bekieken.',
'accmailtitle'                     => 'Wachwoord is verzunnen.',
'accmailtext'                      => "Wachwoord veur '$1' is verzunnen naor $2.",
'newarticle'                       => '(Niej)',
'newarticletext'                   => "Disse pagina besteet nog neet. Hieronder ku-j wat schrieven en naokieken of opslaon. A-j hier per ongelok terechtekeumen bin gebruuk dan de knoppe ''veurige'' um weerumme te gaon.",
'anontalkpagetext'                 => "---- ''Disse overlegpagina heurt bie een annenieme gebruker dee: óf gien gebrukersnaam hef, óf 't neet gebruuk. We gebruken daorumme 't IP-adres ter herkenning, mar 't kan oek ween dat meerdere personen 'tzelfde IP-adres gebruken, en da-j hiermee berichen ontvangen dee neet veur joe bedoeld bin. A-j dit veurkoemen willen, dan ku-j 't bes [[Special:UserLogin|een gebrukersnaam anmaken of anmelden]].''",
'noarticletext'                    => 'Disse pagina besteet nog neet.
Je kunnen \'t woord [[Special:Search/{{PAGENAME}}|opzeuken]] in aandere pagina\'s of <span class="plainlinks">[{{fullurl:{{FULLPAGENAME}}|action=edit}} disse pagina bewarken]</span>.',
'userpage-userdoesnotexist'        => 'Je bewarken een gebrukerspagina van een gebruker dee neet besteet (gebruker "$1"). Kiek effen nao o-j disse pagina wel anmaken/bewarken willen.',
'clearyourcache'                   => "'''NB:''' naodat de wiezigingen op-esleugen bin, mut de kas van de webblaojeraar nog leeg-emaak wonnen um 't te kunnen zien. '''Mozilla / Firefox / Safari:''' drok op ''Shift'' + ''Pagina verniejen,'' of ''Ctrl-F5'' of ''Ctrl-R'' (''Command-R'' op een Macintosh-computer); '''Konqueror: '''klik op ''verniejen'' of drok op ''F5;'' '''Opera:''' leeg de kas in ''Extra → Voorkeuren;'' '''Internet Explorer:''' huil ''Ctrl'' in-edrok terwiel je op ''Pagina verniejen'' klikken of ''Ctrl-F5'' gebruken.",
'usercssjsyoucanpreview'           => "<strong>Tip:</strong> gebruuk de knoppe 'Pagina naokieken' um joew nieje css/js nao te kieken veurda-j 't opslaon.",
'usercsspreview'                   => "'''Dit is allinnig een controle van joew persoonlijke CSS.'''
''''t Is nog neet op-esleugen!'''",
'userjspreview'                    => "'''Denk deran da-j joew nieje gebrukersspecifieke JavaScript allinnig an 't tessen bin, 't is nog neet op-esleugen!'''",
'userinvalidcssjstitle'            => "'''Waorschuwing:''' der is gien uutvoering mit de naam \"\$1\". Vergeet neet dat joew eigen .css- en .js-pagina's beginnen mit een kleine letter, bv. \"{{ns:user}}:Naam/'''m'''onobook.css\" in plaose van \"{{ns:user}}:Naam/'''M'''onobook.css\".",
'updated'                          => '(Bewark)',
'note'                             => '<strong>Opmarking:</strong>',
'previewnote'                      => "<strong>NB: je bin de pagina allinnig nog mar an 't naokieken; de tekse is nog neet op-esleugen!</strong>",
'previewconflict'                  => "Disse versie laot zien ho de tekse in 't bovenste veld deruut kump te zien a-j de tekse opslaon.",
'session_fail_preview'             => "<strong>De bewarking kan neet verwark wonnen wegens een verlies an data. Prebeer 't laoter weer, as 't prebleem dan nog steeds veurkump, prebeer dan opniej an te melden.</strong>",
'session_fail_preview_html'        => "<strong>Joew wieziging kon neet verwark wonnen umdat sessiegegevens verleuren egaon bin.</strong>

''Umdat in {{SITENAME}} roewe HTML in-eschakeld is, is de weergave dervan verbörgen um te veurkoemen dat 't JavaScript an-evuilen wonnen.''

<strong>As dit een legetieme wieziging is, prebeer 't dan opniej. 
As 't dan nog preblemen geef, prebeer dan um [[Special:UserLogout|opniej an te melden]].</strong>",
'token_suffix_mismatch'            => "<strong>De bewarking is eweigerd umdat joew webblaojeraar de leestekens in 't bewarkingstoken verkeerd behaandeld hef. De bewarking is eweigerd um verminking van de paginatekse te veurkoemen. Dit gebeurt soms as der een web-ebaseren proxydiens gebruuk wonnen dee fouten bevat.</strong>",
'editing'                          => 'Bewark: $1',
'editingsection'                   => 'Bewark: $1 (deelpagina)',
'editingcomment'                   => 'Bewark: $1 (niej berich)',
'editconflict'                     => 'Bewarkingskonflik: $1',
'explainconflict'                  => "'''NB:''' een aander hef disse pagina ewiezig naoda-j an disse bewarking begunnen bin.

't Bovenste bewarkingsveld laot de pagina zien zoas 't noen is. Daoronder (bie \"Wiezigingen\") staon de verschillen tussen joew versie en de op-esleugen pagina. Helemaole onderan (bie \"Joew tekse\") steet nog een bewarkingsveld mit joew versie.

Je zullen je eigen wiezigingen in de nieje tekse in mutten passen. Allinnig de tekse in 't bovenste veld wonnen beweerd a-j noen kiezen veur \"Pagina opslaon\".",
'yourtext'                         => 'Joew tekse',
'storedversion'                    => 'Op-esleugen versie',
'nonunicodebrowser'                => '<strong>Waorschuwing: joew webblaojeraar kan neet goed overweg mit unicode, schakel over op een aandere webblaojeraar um de wiezigingen an te brengen!</strong>',
'editingold'                       => "<strong>Waorschuwing: je bin een ouwere versie van disse pagina an 't bewarken. A-j de veraandering opslaon, wonnen alle niejere versies over-eschreven.</strong>",
'yourdiff'                         => 'Wiezigingen',
'copyrightwarning'                 => "NB: Alle biedragen an {{SITENAME}} mutten vrie-egeven wonnen onder de $2 (zie $1 veur infermasie).
A-j neet willen dat joew tekse deur aandere gebrukers an-epas en verspreid kan wonnen, kies dan neet veur 'Pagina opslaon'.<br />
Deur op 'Pagina opslaon' te klikken beleuf je da-j disse tekse zelf eschreven hemmen, of over-eneumen hemmen uut een vrieje, openbaore bron.<br />
<strong>GEBRUUK GIEN MATERIAAL DAT BESCHARMP WONNEN DEUR AUTEURSRECHEN, BEHALVEN A-J DAOR TOESTEMMING VEUR HEMMEN!</strong>",
'copyrightwarning2'                => "Let wel dat alle biedragen an {{SITENAME}} deur aandere gebrukers ewiezig of vort-edaon kunnen wonnen. A-j neet willen dat joew tekse veraanderd wonnen, plaos 't hier dan neet.<br />
De tekse mut auteursrechvrie ween (zie $1 veur details).
<strong>GIEN WARK VAN AANDERE LUUI TOEVOEGEN ZONDER TOESTEMMING VAN DE AUTEUR!</strong>",
'longpagewarning'                  => "Disse pagina is $1 KB groot. 't Bewarken van grote pagina's kan veur preblemen zörgen bie iezelig ouwe webblaojeraars.",
'longpageerror'                    => "<strong>Foutmelding: de tekse dee-j opslaon willen is $1 kilobytes. Dit is groter as 't toe-estaone maximum van $2 kilobytes. Joew tekse kan neet op-esleugen wonnen.</strong>",
'readonlywarning'                  => "<strong>Waorschuwing! De databanke is op dit mement in onderhoud; 't is daorumme neet meugelijk um pagina's te wiezigen. Je kunnen de tekse 't beste op de computer opslaon en laoter opniej preberen de pagina te bewarken.</strong>",
'protectedpagewarning'             => "<strong>Waorschuwing! Disse pagina is beveilig zodat allinnig beheerders 't kunnen wiezigen.</strong>",
'semiprotectedpagewarning'         => "'''Let op:''' disse pagina is allinnig te bewarken deur gebrukers dee tenminsen 4 dagen in-eschreven bin.",
'cascadeprotectedwarning'          => "'''Waorschuwing:''' disse pagina is beveilig zodat allinnig beheerders disse pagina kunnen bewarken, dit wonnen edaon umdat disse pagina veurkump in de volgende {{PLURAL:$1|cascade-beveilige pagina|cascade-beveiligen pagina's}}:",
'titleprotectedwarning'            => "<strong>Waorschuwing: disse pagina is beveilig zodat allinnig bepaolde gebrukers 't an kunnen maken.</strong>",
'templatesused'                    => 'Gebruken sjablonen op disse pagina:',
'templatesusedpreview'             => 'Gebruken sjablonen in disse sectie:',
'templatesusedsection'             => 'Gebruken sjablonen in disse sectie:',
'template-protected'               => '(beveilig)',
'template-semiprotected'           => '(semibeveilig)',
'hiddencategories'                 => 'Disse pagina vuilt in de volgende verbörgen {{PLURAL:$1|kattegerie|kattegerieën}}:',
'nocreatetitle'                    => "'t Anmaken van pagina's is beteund",
'nocreatetext'                     => "Disse webstee hef de meugelijkheid um nieje pagina's an te maken beteund. Je kunnen pagina's dee al bestaon wiezigen of je kunnen je [[Special:UserLogin|anmelden of een gebrukerspagina anmaken]].",
'nocreate-loggedin'                => "Je maggen gien nieje pagina's anmaken op disse wiki.",
'permissionserrors'                => 'Fouten mit de rechen',
'permissionserrorstext'            => 'Je maggen of kunnen dit neet doon. De {{PLURAL:$1|rejen|rejens}} daoveur {{PLURAL:$1|is|bin}}:',
'permissionserrorstext-withaction' => 'Je hemmen gien rech um $2, mit de volgende {{PLURAL:$1|rejen|rejens}}:',
'recreate-deleted-warn'            => "'''Waorschuwing: je maken een pagina an dee eerder al vort-edaon is.'''

Bedenk eers of 't neudig is um disse pagina veerder te bewarken.
't Logboek mit de rejen(s) waorumme as disse pagina vort-edaon is, wonnen veur de dudelijkheid eteund:",

# Parser/template warnings
'expensive-parserfunction-warning'        => "Waorschuwing: disse pagina gebruuk te veule kosbaore parserfuncties.

Noen bin 't der $1, terwiel 't der minder as $2 mutten ween.",
'expensive-parserfunction-category'       => "Pagina's dee te veule kosbaore parserfuncties gebruken",
'post-expand-template-inclusion-warning'  => "Waorschuwing: de grootte van 't in-evoegen sjabloon is te groot.
Sommigen sjablonen wonnen neet in-evoeg.",
'post-expand-template-inclusion-category' => "Pagina's waoveur de grootte van de in-evoegen sjabloon overschrejen wonnen",
'post-expand-template-argument-warning'   => "Waorschuwing: op disse pagina steet tenminsen één sjabloonparremeter dee te lange zol wonnen as 't uut-eklap wonnen.
Disse parremeters bin vort-eleuten.",
'post-expand-template-argument-category'  => "Pagina's mit ontbrekende sjabloonelementen",

# "Undo" feature
'undo-success' => 'De bewarking kan ongedaon-emaak wonnen. Controleer de vergelieking hieronder um vaste te stellen da-j disse haandeling uutvoeren willen, en slao vervolgens de pagina op um de bewarking ongedaon te maken.',
'undo-failure' => 'De wieziging kon neet ongedaon emaak wonnen vanwegen aandere striejige wiezigingen.',
'undo-norev'   => "De bewarking kon neet ongedaon-emaak wonnen, umdat 't neet besteet of vort-edaon is.",
'undo-summary' => 'Versie $1 van [[Special:Contributions/$2|$2]] ([[User talk:$2|overleeg]]) ongedaon-emaak.',

# Account creation failure
'cantcreateaccounttitle' => 'Anmaken van een gebrukersprefiel is neet meugelijk',
'cantcreateaccount-text' => "'t Anmaken van gebrukers van dit IP-adres (<b>$1</b>) is eblokkeerd deur [[User:$3|$3]].

De deur $3 op-egeven rejen is ''$2''",

# History pages
'viewpagelogs'        => 'Bekiek logboeken veur disse pagina',
'nohistory'           => 'Der bin gien eerdere versies van disse pagina.',
'revnotfound'         => 'Wieziging neet evunnen',
'revnotfoundtext'     => 'De op-evreugen ouwe versie van disse pagina is onvientbaor. Kiek de URL dee-j gebruken nao um naor disse pagina te gaon.',
'currentrev'          => 'Leste versie',
'revisionasof'        => 'Versie op $1',
'revision-info'       => 'Versie op $1 van $2',
'previousrevision'    => '&larr; eerdere versie',
'nextrevision'        => 'niejere versie &rarr;',
'currentrevisionlink' => "versie zoas 't noen is",
'cur'                 => 'noen',
'next'                => 'Volgende',
'last'                => 'leste',
'page_first'          => 'eerste',
'page_last'           => 'leste',
'histlegend'          => 'Verklaoring ofkortingen: (noen) = verschil mit de op-esleugen versie, (veurige) = verschil mit de veurige versie, K = kleine wieziging',
'deletedrev'          => '[vort-edaon]',
'histfirst'           => 'Eerste',
'histlast'            => 'Leste',
'historysize'         => '({{PLURAL:$1|1 byte|$1 bytes}})',
'historyempty'        => '(leeg)',

# Revision feed
'history-feed-title'          => 'Wiezigingsoverzichte',
'history-feed-description'    => 'Wiezigingsoverzichte veur disse pagina op de wiki',
'history-feed-item-nocomment' => '$1 op $2', # user at time
'history-feed-empty'          => "De op-evreugen pagina besteet neet. 't Is meugelijk dat disse pagina vort-edaon is of dat 't herneumd is. Prebeer te [[Special:Search|zeuken]] veur relevante nieje pagina's.",

# Revision deletion
'rev-deleted-comment'         => '(commentaar vort-ehaold)',
'rev-deleted-user'            => '(gebrukersnaam vort-edaon)',
'rev-deleted-event'           => '(antekening vort-edaon)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">
De geschiedenisse van disse pagina is uut de peblieke archieven ewis.
Der kan veerdere infermasie staon in \'t [{{fullurl:Special:Log/delete|page={{PAGENAMEE}}}} logboek vort-edaone pagina\'s].
</div>',
'rev-deleted-text-view'       => "<div class=\"mw-warning plainlinks\">
De geschiedenisse van disse pagina is uut de peblieke archieven ewis.
As beheerder van disse wiki ku-j 't wel zien;
der kan veerdere infermasie staon in 't [{{fullurl:Special:Log/delete|page={{PAGENAMEE}}}} logboek vort-edaone pagina's].
</div>",
'rev-delundel'                => 'teun/verbarg',
'revisiondelete'              => 'Wiezigingen vortdoon/herstellen',
'revdelete-nooldid-title'     => 'Gien doelversie',
'revdelete-nooldid-text'      => 'Je hemmen gien versie an-egeven waor disse actie op uut-evoerd mut wonnen.',
'revdelete-selected'          => "{{PLURAL:$2|Esillekteren bewarking|Esillekteren bewarkingen}} van '''[[:$1]]''':",
'logdelete-selected'          => '{{PLURAL:$1|Esillecteren logboekboekactie|Esillecteren logboekacties}}:',
'revdelete-text'              => "Bewarkingen dee vort-ehaold bin, ku-j wel zien in de geschiedenisse, mar de inhoud is neet langer pebliekelijk toegankelijk.

Aandere beheerders van {{SITENAME}} kunnen de verbörgen inhoud bekieken en 't weerummeplaosen mit behulpe van dit scharm, behalven as der aandere beparkingen gelden dee in-esteld bin deur de systeembeheerder.",
'revdelete-legend'            => 'Stel versiebeparkingen in:',
'revdelete-hide-text'         => 'Verbarg de bewarken tekse',
'revdelete-hide-name'         => 'Verbarg logboekactie',
'revdelete-hide-comment'      => 'Verbarg bewarkingssamenvatting',
'revdelete-hide-user'         => 'Verbarg gebrukersnamen en IP-adressen van aandere luui.',
'revdelete-hide-restricted'   => 'Pas disse beparkingen toe op beheerders en aandere gebrukers',
'revdelete-suppress'          => 'Gegevens veur beheerders en aandere gebrukers onderdrokken',
'revdelete-hide-image'        => 'Verbarg bestansinhoud',
'revdelete-unsuppress'        => 'Beparkingen op weerummezetten wiezigingen vortdoon',
'revdelete-log'               => 'Logopmarkingen:',
'revdelete-submit'            => 'De esillecteren versie toepassen',
'revdelete-logentry'          => 'zichbaorheid van bewarkingen is ewiezig veur [[$1]]',
'logdelete-logentry'          => 'wiezigen zichbaorheid van gebeurtenisse [[$1]]',
'revdelete-success'           => 'Zichbaorheid van de wieziging succesvol in-esteld.',
'logdelete-success'           => "'''Zichbaorheid van de gebeurtenisse is succesvol in-esteld.'''",
'revdel-restore'              => 'Zichbaorheid wiezigen',
'pagehist'                    => 'Paginageschiedenisse',
'deletedhist'                 => 'Geschiedenisse dee vort-ehaold is',
'revdelete-content'           => 'inhoud',
'revdelete-summary'           => 'samenvatting bewarken',
'revdelete-uname'             => 'gebrukersnaam',
'revdelete-restricted'        => 'hef beparkingen an beheerders op-eleg',
'revdelete-unrestricted'      => 'hef beparkingen veur beheerders derof ehaold',
'revdelete-hid'               => 'hef $1 verbörgen',
'revdelete-unhid'             => 'hef $1 zichbaor emaak',
'revdelete-log-message'       => '$1 veur $2 {{PLURAL:$2|versie|versies}}',
'logdelete-log-message'       => '$1 veur $2 {{PLURAL:$2|logboekregel|logboekregels}}',

# Suppression log
'suppressionlog'     => 'Verbargingslogboek',
'suppressionlogtext' => 'De onderstaande lieste bevat de pagina dee vort-edaon bin en blokkeringen dee veur beheerders verbörgen bin. In de [[Special:IPBlockList|IP-blokkeerlieste]] bin de blokkeringen dee noen van toepassing bin te bekieken.',

# History merging
'mergehistory'                     => "Geschiedenisse van pagina's bie mekaar doon",
'mergehistory-header'              => 'Via disse pagina ku-j versies uut de geschiedenisse van een bronpagina mit een niejere pagina samenvoegen. Zörg derveur dat disse versies uut de geschiedenisse historisch juustement bin.',
'mergehistory-box'                 => "Versies van twee pagina's samenvoegen:",
'mergehistory-from'                => 'Bronpagina:',
'mergehistory-into'                => 'Bestemmingspagina:',
'mergehistory-list'                => 'Samenvoegbaore bewarkingsgeschiedenisse',
'mergehistory-merge'               => "De volgende versies van [[:$1]] kunnen samen-evoeg wonnen naor [[:$2]]. Gebruuk de kelom mit keuzerondjes um allinnig de versies emaak op en veur de an-egeven tied samen te voegen. Let op dat 't gebruken van de navigasieverwiezingen disse kelom zal herinstellen.",
'mergehistory-go'                  => 'Samenvoegbaore bewarkingen bekieken',
'mergehistory-submit'              => 'Versies bie mekaar doon',
'mergehistory-empty'               => 'Der bin gien versies dee samen-evoeg kunnen wonnen.',
'mergehistory-success'             => '$3 {{PLURAL:$3|versie|versies}} van [[:$1]] bin succesvol samen-evoeg naor [[:$2]].',
'mergehistory-fail'                => 'Kan gien geschiedenisse samenvoegen, controleer opniej de pagina- en tiedparremeters.',
'mergehistory-no-source'           => 'Bronpagina $1 besteet neet.',
'mergehistory-no-destination'      => 'Bestemmingspagina $1 besteet neet.',
'mergehistory-invalid-source'      => 'De bronpagina mut een geldige titel ween.',
'mergehistory-invalid-destination' => 'De bestemmingspagina mut een geldige titel ween.',
'mergehistory-autocomment'         => '[[:$1]] samen-evoeg naor [[:$2]]',
'mergehistory-comment'             => '[[:$1]] samen-evoeg naor [[:$2]]: $3',

# Merge log
'mergelog'           => 'Samenvoegingslogboek',
'pagemerge-logentry' => 'voegen [[$1]] naor [[$2]] samen (versies tot en mit $3)',
'revertmerge'        => 'Samenvoeging ongedaonmaken',
'mergelogpagetext'   => 'Hieronder zie-j een lieste van de leste samenvoegingen van een paginageschiedenisse naor een aandere.',

# Diffs
'history-title'           => 'Geschiedenisse van "$1"',
'difference'              => '(Verschil tussen bewarkingen)',
'lineno'                  => 'Regel $1:',
'compareselectedversions' => 'Vergeliek de ekeuzen versies',
'editundo'                => 'ongedaonmaken',
'diff-multi'              => '({{PLURAL:$1|1 tussenliggende versie|$1 tussenliggende versies}} wonnen neet weer-egeven.)',

# Search results
'searchresults'             => 'Zeukrisseltaoten',
'searchresulttext'          => "'''Opmarking:''' een pagina dee kortens an-emaak is ku-j meschien neet vienen via de zeukfunctie. 't Zeuken geet via een speciale zeukdatabanke dee ongeveer um de 30 tot 48 uur bie-ewörk wonnen.",
'searchsubtitle'            => "Je zochen naor '''[[:$1]]'''",
'searchsubtitleinvalid'     => 'Veur zeukopdrachte "$1"',
'noexactmatch'              => "'''Der besteet gien artikel mit de naam $1.''' Je kunnen disse pagina [[:$1|anmaken]].",
'noexactmatch-nocreate'     => "'''Der besteet gien pagina mit de naam \"\$1\".'''",
'toomanymatches'            => 'Der wanen te veule risseltaoten. Prebeer asjeblief een aandere zeukopdrachte.',
'titlematches'              => 'Overeenkoms mit volgende namen',
'notitlematches'            => 'Gien overeenstemming',
'textmatches'               => 'Overeenkoms mit teksen',
'notextmatches'             => 'Gien overeenstemming',
'prevn'                     => 'veurige $1',
'nextn'                     => 'volgende $1',
'viewprevnext'              => '($1) ($2) ($3)',
'search-result-size'        => '$1 ({{PLURAL:$2|1 woord|$2 woorden}})',
'search-result-score'       => 'Relevantie: $1%',
'search-redirect'           => '(deurverwiezing $1)',
'search-section'            => '(onderwarp $1)',
'search-suggest'            => 'Bedoelen je: $1',
'search-interwiki-caption'  => 'Zusterprejekken',
'search-interwiki-default'  => '$1 risseltaoten:',
'search-interwiki-more'     => '(meer)',
'search-mwsuggest-enabled'  => 'mit anbevelingen',
'search-mwsuggest-disabled' => 'gien anbevelingen',
'search-relatedarticle'     => 'Verwant',
'mwsuggest-disable'         => 'Anbevelingen via AJAX uutschakelen',
'searchrelated'             => 'verwant',
'searchall'                 => 'alles',
'showingresults'            => "Hieronder {{PLURAL:$1|steet '''1''' risseltaot|staon '''$1''' risseltaoten}}  <b>$1</b> vanof nummer <b>$2</b>.",
'showingresultsnum'         => "Hieronder {{PLURAL:$3|steet '''1''' risseltaot|staon '''$3''' risseltaoten}} vanof nummer '''$2'''.",
'showingresultstotal'       => "Hieronder {{PLURAL:$3|wordt et risseltaot '''$1''' van '''$3''' weer-egeven|wonnen de risseltaoten '''$1 tot $2''' van '''$3''' weer-egeven}}",
'nonefound'                 => '<strong>Let wel:</strong> as een zeukopdrachte mislok kump dat vake deur gebruuk van veulveurkoemmende woorden as "de" en "het", dee neet eïndexeerd bin.',
'powersearch'               => 'Zeuk',
'powersearch-legend'        => 'Uut-ebreid zeuken',
'powersearch-ns'            => 'Zeuken in naamruumten:',
'powersearch-redir'         => 'Deurverwiezingen weergeven',
'powersearch-field'         => 'Zeuken naor',
'search-external'           => 'Extern zeuken',
'searchdisabled'            => 'Zeuken in {{SITENAME}} is neet meugelijk. Je kunnen gebruukmaken van Google. De gegevens over {{SITENAME}} bin meugelijk neet bie-ewörk.',

# Preferences page
'preferences'              => 'Veurkeuren',
'mypreferences'            => 'Mien veurkeuren',
'prefs-edits'              => 'Antal bewarkingen:',
'prefsnologin'             => 'Neet an-meld',
'prefsnologintext'         => 'Je mutten <span class="plainlinks">[{{fullurl:Special:Userlogin|returnto=$1}} an-emeld]</span> ween um joew veurkeuren in te kunnen stellen.',
'prefsreset'               => 'Standardveurkeuren hersteld.',
'qbsettings'               => 'Paginalieste',
'qbsettings-none'          => 'Gien',
'qbsettings-fixedleft'     => 'Links, vaste',
'qbsettings-fixedright'    => 'Rechs, vaste',
'qbsettings-floatingleft'  => 'Links, zweven',
'qbsettings-floatingright' => 'Rechs, zweven',
'changepassword'           => 'Wachwoord wiezigen',
'skin'                     => '{{SITENAME}}-uterlijk',
'math'                     => 'Wiskundige formules',
'dateformat'               => 'Daotumweergave',
'datedefault'              => 'Gien veurkeur',
'datetime'                 => 'Daotum en tied',
'math_failure'             => 'Wiskundige formule neet begriepelijk',
'math_unknown_error'       => 'Onbekende fout in formule',
'math_unknown_function'    => 'Onbekende functie in formule',
'math_lexing_error'        => 'Lexicografische fout in formule',
'math_syntax_error'        => 'Syntactische fout in formule',
'math_image_error'         => "'t Overzetten naor PNG is mislok.",
'math_bad_tmpdir'          => 'Map veur tiedelijke bestanden veur wiskundige formules besteet neet of is neet creëerbaar.',
'math_bad_output'          => 'De map veur wiskundebestanden besteet neet of is neet te creëren.',
'math_notexvc'             => "Kan 't pregramma texvc neet vienen; configureer volgens de beschrieving in math/README.",
'prefs-personal'           => 'Gebrukersgegevens',
'prefs-rc'                 => 'Leste wiezigingen',
'prefs-watchlist'          => 'Volglieste',
'prefs-watchlist-days'     => 'Antal dagen weergeven:',
'prefs-watchlist-edits'    => 'Antal wiezigingen in de uut-ebreien volglieste:',
'prefs-misc'               => 'Overig',
'saveprefs'                => 'Veurkeuren opslaon',
'resetprefs'               => 'Standardveurkeuren herstellen',
'oldpassword'              => 'Wachwoord da-j noen hemmen',
'newpassword'              => 'Niej wachwoord',
'retypenew'                => 'Niej wachwoord (opniej)',
'textboxsize'              => 'Bewarkingsveld',
'rows'                     => 'Regels',
'columns'                  => 'Kolommen',
'searchresultshead'        => 'Zeukrisseltaoten',
'resultsperpage'           => 'Antal zeukrisseltaoten per pagina',
'contextlines'             => 'Antal regels per evunnen pagina',
'contextchars'             => 'Antal tekens per pagina',
'stub-threshold'           => 'Verwiezingsformettering van <a href="#" class="stub">beginnetjes</a>:',
'recentchangesdays'        => 'Antal dagen dee eteund mutten wonnen in "leste wiezigingen":',
'recentchangescount'       => 'Antal wiezigingen in de lieste "leste wiezigingen"',
'savedprefs'               => 'Veurkeuren bin op-esleugen.',
'timezonelegend'           => 'Tiedzone',
'timezonetext'             => "Geef 't antal uren an, dee tussen joew tiedgebied en UTC liggen.",
'localtime'                => 'Plaoselijke tied',
'timezoneoffset'           => 'Tiedverschil',
'servertime'               => 'Tied op de server',
'guesstimezone'            => 'Vanuut webblaojeraar toevoegen',
'allowemail'               => 'Berichen van aandere gebrukers toelaoten',
'prefs-searchoptions'      => 'Zeukinstellingen',
'prefs-namespaces'         => 'Naamruumtes',
'defaultns'                => 'Naamruumtes um in te zeuken:',
'default'                  => 'standard',
'files'                    => 'Bestanden',

# User rights
'userrights'                  => 'Gebrukersrechenbeheer', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => 'Beheer gebrukersgroepen',
'userrights-user-editname'    => 'Vul een gebrukersnaam in:',
'editusergroup'               => 'Bewark gebrukersgroepen',
'editinguser'                 => "Doonde mit 't wiezigen van de gebrukersrechen van '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup'    => 'Bewark gebrukersgroep',
'saveusergroups'              => 'Gebrukergroepen opslaon',
'userrights-groupsmember'     => 'Lid van:',
'userrights-groups-help'      => 'Je kunnen de groepen wiezigen waor as de gebruker lid van is.
* Een an-evink vakjen betekent dat de gebruker lid is van de groep.
* Een neet an-evink vakjen betekent dat de gebruker gien lid is van de groep.
* Een "*" betekent da-j een gebruker neet uut een groep vort kunnen haolen naoda-j-m deran toe-evoeg hemmen, of aandersumme.',
'userrights-reason'           => 'Rejen:',
'userrights-no-interwiki'     => "Je hemmen gien rechen um gebrukersrechen op aandere wiki's te wiezigen.",
'userrights-nodatabase'       => 'Databanke $1 besteet neet of is gien plaoselijke databanke.',
'userrights-nologin'          => 'Je mutten [[Special:UserLogin|an-emeld]] ween en as gebruker de juuste rechen hemmen um gebrukersrechen toe te kunnen wiezen.',
'userrights-notallowed'       => 'Je hemmen gien rechen um gebrukersrechen toe te kunnen wiezen.',
'userrights-changeable-col'   => 'Groepen dee-j beheren kunnen',
'userrights-unchangeable-col' => 'Groepen dee-j neet beheren kunnen',

# Groups
'group'               => 'Groep:',
'group-user'          => 'Gebrukers',
'group-autoconfirmed' => 'An-emelde gebrukers',
'group-bot'           => 'bots',
'group-sysop'         => 'beheerders',
'group-bureaucrat'    => 'burocraoten',
'group-suppress'      => 'Toezichhouwers',
'group-all'           => '(alles)',

'group-user-member'          => 'Gebruker',
'group-autoconfirmed-member' => 'An-emelde gebruker',
'group-bot-member'           => 'bot',
'group-sysop-member'         => 'beheerder',
'group-bureaucrat-member'    => 'burocraot',
'group-suppress-member'      => 'Toezichhouwer',

'grouppage-user'          => '{{ns:project}}:Gebrukers',
'grouppage-autoconfirmed' => '{{ns:project}}:An-emelde gebrukers',
'grouppage-bot'           => '{{ns:project}}:Bots',
'grouppage-sysop'         => '{{ns:project}}:Beheerder',
'grouppage-bureaucrat'    => '{{ns:project}}:Beheerder',
'grouppage-suppress'      => '{{ns:project}}:Toezichte',

# Rights
'right-read'                 => "Pagina's bekieken",
'right-edit'                 => "Pagina's bewarken",
'right-createpage'           => "Pagina's anmaken",
'right-createtalk'           => "Overlegpagina's anmaken",
'right-createaccount'        => 'Nieje gebrukers anmaken',
'right-minoredit'            => 'Bewarkingen markeren as klein',
'right-move'                 => "Pagina's herneumen",
'right-move-subpages'        => "Pagina's samen mit subpagina's verplaosen",
'right-suppressredirect'     => 'Gien deurverwiezing anmaken op de ouwe naam as een pagina herneumd wonnen',
'right-upload'               => 'Bestanden toevoegen',
'right-reupload'             => 'Een bestaond bestand overschrieven',
'right-reupload-own'         => 'Eigen toe-evoegen bestanden overschrieven',
'right-reupload-shared'      => 'Media uut de edelen mediadatabanke plaoselijk overschrieven',
'right-upload_by_url'        => 'Bestanden toevoegen via een verwiezing',
'right-purge'                => 'De kas van een pagina legen',
'right-autoconfirmed'        => 'Behaandeld wonnen as een an-emelde gebruker',
'right-bot'                  => 'Behaandeld wonnen as een eautomatiseerd preces',
'right-nominornewtalk'       => "Kleine bewarkingen an een overlegpagina leien neet tot een melding 'nieje berichen'",
'right-apihighlimits'        => 'Hoge API-limieten gebruken',
'right-writeapi'             => 'Bewarken via de API',
'right-delete'               => "Pagina's vortdoon",
'right-bigdelete'            => "Pagina's mit een grote geschiedenisse vortdoon",
'right-deleterevision'       => "Versies van pagina's verbargen",
'right-deletedhistory'       => 'Vort-edaone versies bekieken, zonder te kunnen zien wat der vort-edaon is',
'right-browsearchive'        => "Vort-edaone pagina's bekieken",
'right-undelete'             => "Vort-edaone pagina's weerummeplaosen",
'right-suppressrevision'     => 'Verbörgen versies bekieken en weerummeplaosen',
'right-suppressionlog'       => 'Neet-peblieke logboeken bekieken',
'right-block'                => 'Aandere gebrukers de meugelijkheid ontnemen um te bewarken',
'right-blockemail'           => "Een gebruker 't rech ontnemen um liendepos te versturen",
'right-hideuser'             => 'Een gebruker veur de overige gebrukers verbargen',
'right-ipblock-exempt'       => 'IP-blokkeringen ummezeilen',
'right-proxyunbannable'      => "Blokkeringen veur proxy's gellen neet",
'right-protect'              => "Beveiligingsnivo's wiezigen",
'right-editprotected'        => "Beveiligen pagina's bewarken",
'right-editinterface'        => "'t {{SITENAME}}-uterlijk bewarken",
'right-editusercssjs'        => 'De CSS- en JS-bestanden van aandere gebrukers bewarken',
'right-rollback'             => 'Gauw de leste bewarking(en) van een gebruker an een pagina weerummedreien',
'right-markbotedits'         => 'Weerummedreien bewarkingen markeren as botbewarkingen',
'right-noratelimit'          => 'Hef gien tiedsofhankelijke beparkingen',
'right-import'               => "Pagina's uut aandere wiki's invoeren",
'right-importupload'         => "Pagina's vanuut een bestand invoeren",
'right-patrol'               => 'Bewarkingen as econtroleerd markeren',
'right-autopatrol'           => 'Bewarkingen wonnen autematisch as econtroleerd emarkeerd',
'right-patrolmarks'          => 'Controletekens in leste wiezigingen bekieken',
'right-unwatchedpages'       => "Bekiek een lieste mit pagina's dee neet op een volglieste staon",
'right-trackback'            => 'Een weerummespoor opgeven',
'right-mergehistory'         => "De geschiedenisse van pagina's bie mekaar doon",
'right-userrights'           => 'Alle gebrukersrechen bewarken',
'right-userrights-interwiki' => "Gebrukersrechen van gebrukers in aandere wiki's wiezigen",
'right-siteadmin'            => 'De databanke blokkeren en weer vriegeven',

# User rights log
'rightslog'      => 'Gebrukersrechenlogboek',
'rightslogtext'  => 'Dit is een logboek mit veraanderingen van gebrukersrechen',
'rightslogentry' => 'Gebrukersrechen veur $1 ewiezig van $2 naor $3',
'rightsnone'     => '(gien)',

# Recent changes
'nchanges'                          => '$1 {{PLURAL:$1|wieziging|wiezigingen}}',
'recentchanges'                     => 'Leste wiezigingen',
'recentchangestext'                 => 'Op disse pagina ku-j de leste wiezigingen van disse wiki bekieken.',
'recentchanges-feed-description'    => 'Zeuk naor de alderleste wiezingen op disse wiki in disse feed.',
'rcnote'                            => "Hieronder {{PLURAL:$1|steet de leste bewarking|staon de leste '''$1''' bewarkingen}} van de of-eleupen {{PLURAL:$2|dag|'''$2''' dagen}} (stand: $5, $4).",
'rcnotefrom'                        => 'Dit bin de wiezigingen sins <b>$2</b> (maximum van <b>$1</b> wiezigingen).',
'rclistfrom'                        => 'Teun wiezigingen vanof $1',
'rcshowhideminor'                   => '$1 kleine wiezigingen',
'rcshowhidebots'                    => '$1 botgebrukers',
'rcshowhideliu'                     => '$1 an-emelde gebrukers',
'rcshowhideanons'                   => '$1 annenieme gebrukers',
'rcshowhidepatr'                    => '$1 nao-ekeken bewarkingen',
'rcshowhidemine'                    => '$1 mien bewarkingen',
'rclinks'                           => 'Bekiek de leste $1 wiezigingen van de of-eleupen $2 dagen<br />$3',
'diff'                              => 'wiezig',
'hist'                              => 'gesch',
'hide'                              => 'verbarg',
'show'                              => 'teun',
'minoreditletter'                   => 'K',
'newpageletter'                     => 'N',
'boteditletter'                     => ' (bot)',
'number_of_watching_users_pageview' => '[$1 {{PLURAL:$1|keer|keer}} op een volglieste]',
'rc_categories'                     => 'Kattegeriebeparking (scheiden mit "|")',
'rc_categories_any'                 => 'alles',
'newsectionsummary'                 => 'Niej onderwarp: /* $1 */',

# Recent changes linked
'recentchangeslinked'          => 'Volg verwiezigingen',
'recentchangeslinked-title'    => 'Wiezigingen verwant an $1',
'recentchangeslinked-noresult' => 'Gien wiezigingen of pagina waornaor verwezen wonnen in disse periode.',
'recentchangeslinked-summary'  => "Op disse speciale pagina steet een lieste mit de leste wieziginen op pagina's waornaor verwezen wonnen. Pagina's op joew volglieste staon '''vet-edrok'''.",
'recentchangeslinked-page'     => 'Paginanaam:',
'recentchangeslinked-to'       => "Bekiek wiezigingen op pagina's mit verwiezingen naor disse pagina",

# Upload
'upload'                      => 'Bestand toevoegen',
'uploadbtn'                   => 'Bestand toevoegen',
'reupload'                    => 'Opniej toevoegen',
'reuploaddesc'                => 'Weerumme naor bestandtoevoegingsformelier.',
'uploadnologin'               => 'Neet an-emeld',
'uploadnologintext'           => 'Je mutten [[Special:UserLogin|an-emeld]] ween um bestanden toe te kunnen voegen.',
'upload_directory_missing'    => 'De bestanstoevoegingsmap ($1) ontbreek en kon neet an-emaak wonnen deur de webserver.',
'upload_directory_read_only'  => "Op 't mement ku-j gien bestanden toevoegen wegens technische rejens ($1).",
'uploaderror'                 => "Fout bie 't toevoegen van 't bestand",
'uploadtext'                  => "Gebruuk 't onderstaonde formelier um bestanden toe te voegen.
Um eerder toe-evoegen bestanden te bekieken of te zeuken ku-j naor de [[Special:ImageList|bestanslieste]] gaon.
Toe-evoegen bestanden en media dee vort-edaon bin wonnen bie-ehuilen in 't [[Special:Log/upload|logboek mit toe-evoegen bestanden]] en 't [[Special:Log/delete|logboek mit vort-edaon bestanden]].

Um 't bestand in te voegen in een pagina ku-j een van de volgende codes gebruken:
* '''<nowiki>[[</nowiki>{{ns:image}}<nowiki>:Bestand.jpg]]</nowiki>'''
* '''<nowiki>[[</nowiki>{{ns:image}}<nowiki>:Bestand.png|alternatieve tekst]]</nowiki>'''
* '''<nowiki>[[</nowiki>{{ns:media}}<nowiki>:Bestand.ogg]]</nowiki>''' drekte verwiezing naor een bestand.",
'upload-permitted'            => 'Toe-estaone bestanstypes: $1.',
'upload-preferred'            => 'An-ewezen bestanstypes: $1.',
'upload-prohibited'           => 'Verbeujen bestanstypes: $1.',
'uploadlog'                   => 'Toe-evoegen bestanden',
'uploadlogpage'               => 'Toe-evoegen bestanden',
'uploadlogpagetext'           => 'Hieronder steet een lieste mit bestanden dee net niej bin.
Zie de [[Special:NewImages|gallerieje mit media]] veur een overzichte.',
'filename'                    => 'Bestansnaam',
'filedesc'                    => 'Beschrieving',
'fileuploadsummary'           => 'Beschrieving:',
'filestatus'                  => 'Auteursrechstaotus',
'filesource'                  => 'Bron',
'uploadedfiles'               => 'Toe-evoegen bestanden',
'ignorewarning'               => 'Negeer alle waorschuwingen',
'ignorewarnings'              => 'negeer waorschuwingen',
'minlength1'                  => 'Bestansnamen mutten tenminsen één letter hemmen.',
'illegalfilename'             => 'De bestansnaam "$1" bevat karakters dee neet in namen van artikels veur maggen koemen. Geef \'t bestand een aandere naam, en prebeer \'t dan opniej toe te voegen.',
'badfilename'                 => 'De naam van \'t bestand is ewiezig naor "$1".',
'filetype-badmime'            => 'Bestanden mit \'t MIME-type "$1" maggen hier neet toe-evoeg wonnen.',
'filetype-unwanted-type'      => "'''\".\$1\"''' is een ongewunst bestanstype. An-ewezen {{PLURAL:\$3|bestanstype is|bestanstypes bin}} \$2.",
'filetype-banned-type'        => "'''\".\$1\"''' is gien toe-eleuten bestanstype.
Toe-eleuten {{PLURAL:\$3|bestanstype is|bestanstypes bin}} \$2.",
'filetype-missing'            => 'Dit bestand hef gien extensie (bv. ".jpg").',
'large-file'                  => "'t Wonnen an-raojen dat bestanden neet groter bin as $1, dit bestand is $2.",
'largefileserver'             => "'t Bestand is groter as dat de server toesteet.",
'emptyfile'                   => "'t Bestand da-j toe-evoeg hemmen is leeg. Dit kan koemen deur een tikfout in de bestansnaam. Kiek effen nao of je dit bestand wel bedoelen.",
'fileexists'                  => "Een ofbeelding mit disse naam besteet al; je wonnen verzoch 't bestand onder een aandere naam toe te voegen. <strong><tt>$1</tt></strong>",
'filepageexists'              => 'De beschrievingspagina veur dit bestand bestung al op <strong><tt>$1</tt></strong>, mar der besteet nog gien bestand mit disse naam.
De samenvatting dee-j op-egeven hemmen zal neet op de beschrievingspagina koemen.
Bewark de pagina haandmaotig um joew beschrieving daor weer te geven.',
'fileexists-extension'        => "Een bestand mit een soortgelieke naam besteet al:<br />
Naam van 't bestand da-j toevoegen wollen: <strong><tt>$1</tt></strong><br />
Naam van 't bestaonde bestand: <strong><tt>$2</tt></strong><br />
't Enigste verschil is de heufletters/kleine letters van de extensie. Kiek effen nao of de bestanden neet liekelleens bin.",
'fileexists-thumb'            => "'''<center>Bestaonde ofbeelding</center>'''",
'fileexists-thumbnail-yes'    => "Dit bestand is een ofbeelding waorvan de grootte verkleind is <i>(ofbeeldingsoverzichte)</i>. Controleer 't bestand <strong><tt>$1</tt></strong>.<br />
As de ofbeelding dee-j krek nao-ekeken hemmen dezelfde grootte hef, dan is 't neet neudig um 't opniej toe te voegen.",
'file-thumbnail-no'           => "De bestansnaam begint mit <strong><tt>$1</tt></strong>. 
Dit is werschienlijk een verkleinde ofbeelding <i>(overzichsofbeelding)</i>.
A-j disse ofbeelding in volle grootte hemmen voeg 't dan toe, wiezig aanders de bestansnaam.",
'fileexists-forbidden'        => "Een ofbeelding mit disse naam besteet al;
je wonnen verzoch 't toe te voegen onder een aandere naam. [[Image:$1|thumb|center|$1]]",
'fileexists-shared-forbidden' => "Der besteet al een bestand mit disse naam in de gezamelijke bestanslokasie.
A-j 't bestand asnog toevoegen willen, gao dan weerumme en kies een aandere naam.
[[Image:$1|thumb|center|$1]]",
'file-exists-duplicate'       => "Dit bestand is liekeleens as {{PLURAL:$1|'t volgende bestand|de volgende bestanden}}:",
'successfulupload'            => 'Bestanstoevoeging was succesvol',
'uploadwarning'               => 'Waorschuwing',
'savefile'                    => 'Bestand opslaon',
'uploadedimage'               => 'Toe-evoeg: [[$1]]',
'overwroteimage'              => 'Nieje versie van "[[$1]]" toe-evoeg',
'uploaddisabled'              => 'Bestanden toevoegen is neet meugelijk.',
'uploaddisabledtext'          => 'Bestandtoevoegingen bin uut-eschakeld.',
'uploadscripted'              => 'Dit bestand bevat een HTML- of skripcode dat verkeerd deur je webblaojeraar weer-egeven kan wonnen.',
'uploadcorrupt'               => "De inhoud van 't bestand kan neet in overeenstemming ebröch wonnen mit de extentie. Controleer 't bestand, en prebeer daornao 't bestand opniej toe te voegen.",
'uploadvirus'                 => "'t Bestand bevat een virus! Details: $1",
'sourcefilename'              => 'Oorspronkelijk bestansnaam',
'destfilename'                => 'Opslaon as (optioneel)',
'upload-maxfilesize'          => 'Maximale bestansgrootte: $1',
'watchthisupload'             => 'Volg ofbeeldingspagina',
'filewasdeleted'              => "Een bestand mit disse naam is al eerder vort-edaon. Kiek 't $1 nao veurda-j 't opniej toevoegen.",
'upload-wasdeleted'           => "'''Waorschuwing: je bin een bestand an 't toevoegen dee eerder al vort-edaon is.'''

Bedenk eers of 't inderdaod de bedoeling is dat dit bestand toe-evoeg wonnen.
't Logboek mit vort-edaone pagina's ku-j hier vienen:",
'filename-bad-prefix'         => 'De naam van \'t bestand da-j an \'t toevoegen bin begint mit <strong>"$1"</strong>, wat een neet-beschrievende naam is dee meestentieds autematisch deur een digitale camera egeven wonnen. Kies een dudelijke naam veur \'t bestand.',

'upload-proto-error'      => 'Verkeerde protocol',
'upload-proto-error-text' => 'Um op disse meniere bestanden toe te voegen mutten webadressen beginnen mit <code>http://</code> of <code>ftp://</code>.',
'upload-file-error'       => 'Interne fout',
'upload-file-error-text'  => 'Bie ons gung der effen wat fout to een tiejelijk bestand op de server an-emaak wönnen. Neem kontak op mit een [[Special:ListUsers/sysop|systeembeheerder]].',
'upload-misc-error'       => "Onbekende fout bie 't toevoegen van joew bestand",
'upload-misc-error-text'  => "Der is bie toevoegen van 't bestand een onbekende fout op-etrejen. Kiek effen nao of de verwiezing 't wel dut en prebeer 't opniej. As 't prebleem anhuilt, neem dan kontak op mit één van de systeembeheerders.",

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => 'Kon webadres neet bereiken',
'upload-curl-error6-text'  => "'t Webadres kon neet bereik wonnen. Kiek effen nao of je 't goeie adres in-evoerd hemmen en of de webstee bereikbaor is.",
'upload-curl-error28'      => "Tiedoverschriejing bie 't toevoegen van 't bestand",
'upload-curl-error28-text' => "'t Duren te lange veurdat de webstee reageren. Kiek effen nao of de webstee bereikbaor is, wach effen en prebeer 't daornao weer. Prebeer 't aanders as 't wat rustiger is.",

'license'            => 'Licentie',
'nolicense'          => 'Gien licentie ekeuzen',
'license-nopreview'  => '(Naokieken is neet meugelijk)',
'upload_source_url'  => ' (een geldig, pebliek toegankelijk webadres)',
'upload_source_file' => ' (een bestand op joew computer)',

# Special:ImageList
'imagelist-summary'     => 'Op disse speciale pagina ku-j alle toe-evoegen bestanden bekieken.
Standard wonnen de les toe-evoegen bestanden bovenan de lieste ezet.
Klikken op een kelomkop veraandert de sortering.',
'imagelist_search_for'  => 'Zeuk op ofbeeldingnaam:',
'imgfile'               => 'bestand',
'imagelist'             => 'Ofbeeldingenlieste',
'imagelist_date'        => 'Daotum',
'imagelist_name'        => 'Naam',
'imagelist_user'        => 'Gebruker',
'imagelist_size'        => 'Grootte (bytes)',
'imagelist_description' => 'Beschrieving',

# Image description page
'filehist'                       => 'Bestansgeschiedenisse',
'filehist-help'                  => "Klik op een daotum/tied um 't bestand te zien zoas 't to was.",
'filehist-deleteall'             => 'alles vortdoon',
'filehist-deleteone'             => 'disse vortdoon',
'filehist-revert'                => 'weerummedreien',
'filehist-current'               => "zoas 't noen is",
'filehist-datetime'              => 'Daotum/tied',
'filehist-user'                  => 'Gebruker',
'filehist-dimensions'            => 'Ofmetingen',
'filehist-filesize'              => 'Bestansgrootte',
'filehist-comment'               => 'Opmarkingen',
'imagelinks'                     => 'Gebruuk van dit bestand',
'linkstoimage'                   => "Disse ofbeelding wonnen gebruuk op de volgende {{PLURAL:$1|pagina|$1 pagina's}}:",
'nolinkstoimage'                 => 'Ofbeelding is neet in gebruuk.',
'morelinkstoimage'               => '[[Special:WhatLinksHere/$1|Meer verwiezingen]] naor dit bestand bekieken.',
'redirectstofile'                => "{{PLURAL:$1|'t Volgende bestand verwies|De volgende $1 bestanden verwiezen}} deur naor dit bestand:",
'duplicatesoffile'               => "{{PLURAL:$1|'t Volgende bestand is|De volgende $1 bestanden bin}} liekeleens as dit bestand:",
'sharedupload'                   => 'Dit bestand is een gedeelde upload en kan ook deur andere prejekken ebruukt worden.',
'shareduploadwiki'               => 'Zie $1 veur veerdere infermasie.',
'shareduploadwiki-desc'          => 'De $1 in de edelen bestansbanke zie-j hieronder.',
'shareduploadwiki-linktext'      => 'bestandbeschrievingspagina',
'shareduploadduplicate'          => 'Dit bestand is een kopie van $1 in de edelen mediabanke.',
'shareduploadduplicate-linktext' => 'een aander bestand',
'shareduploadconflict'           => 'Dit bestand hef dezelfde naam as $1 in de edelen mediabanke.',
'shareduploadconflict-linktext'  => 'een aander bestand',
'noimage'                        => "Der besteet gien bestand mit disse naam, je kunnen 't $1.",
'noimage-linktext'               => 'toevoegen',
'uploadnewversion-linktext'      => 'Een niejere versie van dit bestand toevoegen.',
'imagepage-searchdupe'           => 'Kopiebestanden zeuken',

# File reversion
'filerevert'                => '$1 weerummedreien',
'filerevert-legend'         => 'Bestand weerummezetten',
'filerevert-intro'          => "Je bin '''[[Media:$1|$1]]''' an 't weerummedreien tot de [$4 versie van $2, $3]",
'filerevert-comment'        => 'Opmarkingen:',
'filerevert-defaultcomment' => 'Weerummedreid tot de versie van $1, $2',
'filerevert-submit'         => 'Weerummedreien',
'filerevert-success'        => '<span class="plainlinks">\'\'\'[[Media:$1|$1]]\'\'\' is weerummedreid naor de [$4 versie op $2, $3]</span>.',
'filerevert-badversion'     => 'Der is gien veurige lokale versie van dit bestand mit de op-egeven tied.',

# File deletion
'filedelete'                  => '$1 vortdoon',
'filedelete-legend'           => 'Bestand vortdoon',
'filedelete-intro'            => "Je doon '''[[Media:$1|$1]]''' noen vort.",
'filedelete-intro-old'        => "Je bin de versie van '''[[Media:$1|$1]]''' van [$4 $3, $2] vort an 't doon.",
'filedelete-comment'          => 'Opmarking:',
'filedelete-submit'           => 'Vortdoon',
'filedelete-success'          => "'''$1''' is vort-edaon.",
'filedelete-success-old'      => "De versie van '''[[Media:$1|$1]]''' van $3, $2 is vort-edaon.",
'filedelete-nofile'           => "'''$1''' besteet neet.",
'filedelete-nofile-old'       => "Der is gien versie van '''$1''' in 't archief mit de an-egeven eigenschappen.",
'filedelete-iscurrent'        => 'Je preberen de niejste versie van dit bestand vort te doon. Zet eers een ouwere versie weerumme.',
'filedelete-otherreason'      => 'Aandere rejen:',
'filedelete-reason-otherlist' => 'Aandere rejen',
'filedelete-reason-dropdown'  => '*Veulveurkoemende rejens
** Auteursrechenschending
** Dit bestand he-w dubbel',
'filedelete-edit-reasonlist'  => "Rejen veur 't vortdoon van de bewarken",

# MIME search
'mimesearch'         => 'Zeuken op MIME-type',
'mimesearch-summary' => "Op disse speciale pagina kunnen de bestanden naor 't MIME-type efiltreerd wonnen. De invoer mut altied 't media- en subtype bevatten, bieveurbeeld: <tt>ofbeelding/jpeg</tt>.",
'mimetype'           => 'MIME-type:',
'download'           => 'oflaojen',

# Unwatched pages
'unwatchedpages' => "Pagina's dee neet evolg wonnen",

# List redirects
'listredirects' => 'Lieste van deurverwiezingen',

# Unused templates
'unusedtemplates'     => 'Ongebruken sjablonen',
'unusedtemplatestext' => "Hieronder steet een lieste van ongebruken sjablonen. Vergeet neet de verwiezingen te controleren veurda-j 't sjabloon vortdoon.",
'unusedtemplateswlh'  => 'aandere verwiezingen',

# Random page
'randompage'         => 'Willekeurig artikel',
'randompage-nopages' => "Der staon gien pagina's in disse naamruumte.",

# Random redirect
'randomredirect'         => 'Willekeurige deurverwiezing',
'randomredirect-nopages' => 'Der staon gien deurverwiezingen in disse naamruumte.',

# Statistics
'statistics'             => 'Staotestieken',
'sitestats'              => 'Staotestieken van {{SITENAME}}',
'userstats'              => 'Gebrukerstaotestieken',
'sitestatstext'          => "In totaal {{PLURAL:$1|steet der '''1''' pagina|staon der '''$1''' pagina's}} in de databanke van disse {{SITENAME}}. Hier zitten de overlegpagina's, pagina's over Wikipedie, iezelig korte artikels, deurstuurpagina's en een antal aandere pagina's dee neet as artikel mee-eteld wonnen bie in. Zonder disse pagina's {{PLURAL:$2|is der ongeveer '''1''' artikel|bin der ongeveer '''$2''' artikels}}.

Der {{PLURAL:$8|is '''1''' bestand|bin '''$8''' bestanden}} toe-evoeg.

Der {{PLURAL:$3|is '''1''' pagina|bin '''$3''' pagina's}} weer-egeven en '''$4''' {{PLURAL:$4|bewarking|bewarkingen}} edaon sins {{SITENAME}} op-ezet is. Dit geef een gemiddelde van '''$5''' bewarkingen per pagina en '''$6''' weer-egeven pagina's per bewarking.

De lengte van de [http://www.mediawiki.org/wiki/Manual:Job_queue taakwachrie] is '''$7'''.",
'userstatstext'          => "Der {{PLURAL:$1|is '''1''' an-emelde gebruker|bin '''$1''' an-emelde gebrukers}}. Daovan  {{PLURAL:$2|hef|hemmen}} der '''$2''' (van '''$4%''') $5rechen.",
'statistics-mostpopular' => "Meestbekeken pagina's",

'disambiguations'      => "Deurverwiespagina's",
'disambiguationspage'  => 'Template:Dv',
'disambiguations-text' => "De onderstaonde pagina's verwiezen naor een '''deurverwiespagina'''. Disse verwiezingen mutten eigenlijks rechstreeks verwiezen naor 't juuste onderwarp.

Pagina's wonnen ezien as een deurverwiespagina, as 't sjabloon gebruuk wonnen dat vermeld steet op [[MediaWiki:Disambiguationspage]]",

'doubleredirects'            => 'Dubbele deurverwiezingen',
'doubleredirectstext'        => "Op elke regel steet de eerste deurstuurpagina, de tweede deurstuurpagina en de eerste regel van de tweede deurverwiezing. Meestentieds is leste pagina 't eigenlijke doel.",
'double-redirect-fixed-move' => '[[$1]] is herneumd en is noen een deurverwiezing naor [[$2]]',
'double-redirect-fixer'      => 'Deurverwiezingsverbeteraar',

'brokenredirects'        => 'Doodlopende deurverwiezingen',
'brokenredirectstext'    => "De onderstaonde pagina's bevatten een deurverwiezingen naor een neet-bestaonde pagina.",
'brokenredirects-edit'   => '(bewark)',
'brokenredirects-delete' => '(vortdoon)',

'withoutinterwiki'         => "Pagina's zonder verwiezingen naor aandere talen",
'withoutinterwiki-summary' => "De volgende pagina's verwiezen neet naor versies in een aandere taal.",
'withoutinterwiki-legend'  => 'Veurvoegsel',
'withoutinterwiki-submit'  => 'Bekieken',

'fewestrevisions' => 'Artikels mit de minste bewarkingen',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|byte|bytes}}',
'ncategories'             => '$1 {{PLURAL:$1|kattegerie|kattegerieën}}',
'nlinks'                  => '$1 {{PLURAL:$1|verwiezing|verwiezingen}}',
'nmembers'                => '$1 {{PLURAL:$1|onderwarp|onderwarpen}}',
'nrevisions'              => '$1 {{PLURAL:$1|versie|versies}}',
'nviews'                  => '{{PLURAL:$1|1 keer|$1 keer}} bekeken',
'specialpage-empty'       => 'Disse pagina is leeg.',
'lonelypages'             => "Weespagina's",
'lonelypagestext'         => "Naor de onderstaonde pagina's wonnen vanuut disse wiki neet verwezen.",
'uncategorizedpages'      => "Pagina's zonder kattegerie",
'uncategorizedcategories' => 'Kattegerieën zonder kattegerie',
'uncategorizedimages'     => 'Ofbeeldingen zonder kattegerie',
'uncategorizedtemplates'  => 'Sjablonen zonder kattegerie',
'unusedcategories'        => 'Ongebruken kattegerieën',
'unusedimages'            => 'Ongebruken ofbeeldingen',
'popularpages'            => 'Populaire artikels',
'wantedcategories'        => 'Gewunste kattegerieën',
'wantedpages'             => "Gewunste pagina's",
'missingfiles'            => 'Ontbrekende bestanden',
'mostlinked'              => "Pagina's waor 't meest naor verwezen wonnen",
'mostlinkedcategories'    => 'Meestgebruken kattegerieën',
'mostlinkedtemplates'     => "Sjablonen dee 't meest gebruuk wonnen",
'mostcategories'          => 'Artikels mit de meeste kattegerieën',
'mostimages'              => 'Meestgebruken ofbeeldingen',
'mostrevisions'           => 'Artikels mit de meeste bewarkingen',
'prefixindex'             => 'Veurvoegselindex',
'shortpages'              => 'Korte artikels',
'longpages'               => 'Lange artikels',
'deadendpages'            => "Pagina's zonder verwiezingen",
'deadendpagestext'        => "De onderstaonde pagina's verwiezen neet naor aandere pagina's in disse wiki.",
'protectedpages'          => "Pagina's dee beveilig bin",
'protectedpages-indef'    => 'Allinnig blokkeringen zonder verloopdaotum',
'protectedpagestext'      => "De volgende pagina's bin beveilig en kunnen neet herneumd of bewark wonnen.",
'protectedpagesempty'     => "Der bin op 't mement gien beveiligen pagina's",
'protectedtitles'         => 'Beveiligen titels',
'protectedtitlestext'     => "De volgende pagina's bin beveilig zoda-ze neet opniej an-emaak kunnen wonnen",
'protectedtitlesempty'    => 'Der bin noen gien titels beveilig dee an disse parremeters voldoon.',
'listusers'               => 'Gebrukerslieste',
'newpages'                => 'Nieje artikels',
'newpages-username'       => 'Gebrukersnaam:',
'ancientpages'            => 'Artikels dee lange neet bewörk bin',
'move'                    => 'Herneum',
'movethispage'            => 'Herneum',
'unusedimagestext'        => "Vergeet neet dat aandere wiki's meschien oek enkele van disse ofbeeldingen gebruken.",
'unusedcategoriestext'    => 'De onderstaonde kattegerieën bin an-emaak mar wonnen neet gebruuk.',
'notargettitle'           => 'Gien pagina op-egeven',
'notargettext'            => 'Je hemmen neet op-egeven veur welke pagina je disse functie bekieken willen.',
'nopagetitle'             => 'Doelpagina besteet neet',
'nopagetext'              => 'De pagina dee-j herneumen willen besteet neet.',
'pager-newer-n'           => '{{PLURAL:$1|1 niejere|$1 niejere}}',
'pager-older-n'           => '{{PLURAL:$1|1 ouwere|$1 ouwere}}',
'suppress'                => 'Toezichte',

# Book sources
'booksources'               => 'Boekinfermasie',
'booksources-search-legend' => 'Zeuk infermasie over een boek',
'booksources-go'            => 'Zeuk',
'booksources-text'          => "Hieronder steet een lieste mit verwiezingen naor aandere websteeën dee nieje of gebruken boeken verkopen, en hemmen meschien meer infermasie over 't boek da-j zeuken:",

# Special:Log
'specialloguserlabel'  => 'Gebruker:',
'speciallogtitlelabel' => 'Naam:',
'log'                  => 'Logboeken',
'all-logs-page'        => 'Alle logboeken',
'log-search-legend'    => 'Logboeken deurzeuken',
'log-search-submit'    => 'Zeuk',
'alllogstext'          => "Dit is 't combinasielogboek van {{SITENAME}}. 
Je kunnen oek kiezen veur bepaolde logboeken en filteren op gebruker (heuflettergeveulig) en titel (heuflettergeveulig).",
'logempty'             => "Der steet gien infermasie in 't logboek dee voldut an disse criteria.",
'log-title-wildcard'   => 'Zeuk naor titels dee beginnen mit disse tekse:',

# Special:AllPages
'allpages'          => "Alle pagina's",
'alphaindexline'    => '$1 tot $2',
'nextpage'          => 'Volgende pagina ($1)',
'prevpage'          => 'Veurige pagina ($1)',
'allpagesfrom'      => "Teun pagina's vanof:",
'allarticles'       => 'Alle artikels',
'allinnamespace'    => "Alle pagina's (naamruumte $1)",
'allnotinnamespace' => "Alle pagina's (neet in naamruumte $1)",
'allpagesprev'      => 'veurige',
'allpagesnext'      => 'volgende',
'allpagessubmit'    => 'Zeuk',
'allpagesprefix'    => "Teun pagina's dee beginnen mit:",
'allpagesbadtitle'  => 'De op-egeven paginanaam is ongeldig of bevatten een interwikiveurvoegsel. Meugelijkerwieze bevatten de naam kerakters dee neet gebruuk maggen wonnen in paginanamen.',
'allpages-bad-ns'   => '{{SITENAME}} hef gien "$1"-naamruumte.',

# Special:Categories
'categories'                    => 'Kattegerieën',
'categoriespagetext'            => "De volgende kattegerieën bin anwezig in {{SITENAME}}.

De volgende kattegerieën bevatten pagina's of media.
[[Special:UnusedCategories|ongebruken kattegerieën]] zie-j hier neet.
Zie oek [[Special:WantedCategories|gewunste kattegerieën]].",
'categoriesfrom'                => 'Kattegerieën weergeven vanof:',
'special-categories-sort-count' => 'op antal sorteren',
'special-categories-sort-abc'   => 'alfebetisch sorteren',

# Special:ListUsers
'listusersfrom'      => 'Teun vanof:',
'listusers-submit'   => 'Teun',
'listusers-noresult' => 'Gien gebrukers evunnen. Zeuk oek naor varianten mit kleine letters of heufletters.',

# Special:ListGroupRights
'listgrouprights'          => 'Rechen van gebrukersgroepen',
'listgrouprights-summary'  => 'Op disse pagina staon de gebrukersgroepen van disse wiki beschreven, mit de biebeheurende rechen.
Meer infermasie over de rechen ku-j [[{{MediaWiki:Listgrouprights-helppage}}|hier vienen]].',
'listgrouprights-group'    => 'Groep',
'listgrouprights-rights'   => 'Rechen',
'listgrouprights-helppage' => 'Help:Gebrukersrechen',
'listgrouprights-members'  => '(lejenlieste)',

# E-mail user
'mailnologin'     => 'Neet an-emeld.',
'mailnologintext' => 'Je mutten [[Special:UserLogin|an-emeld]] ween en een geldig e-mailadres in "[[Special:Preferences|mien veurkeuren]]" invoeren um disse functie te kunnen gebruken.',
'emailuser'       => 'Een berich sturen',
'emailpage'       => 'Gebruker een berich sturen',
'emailpagetext'   => "As disse gebruker een geldig e-mailadres op-egeven hef, dan ku-j via dit formelier een berich versturen. 
't Adres da-j op-egeven hemmen bie [[Special:Preferences|joew veurkeuren]] zal as ofzender gebruuk wonnen.
De ontvanger kan dus drek beantwoorden.",
'usermailererror' => "Foutmelding bie 't versturen:",
'defemailsubject' => 'Berich van {{SITENAME}}',
'noemailtitle'    => 'Gebruker hef gien e-mailadres op-egeven',
'noemailtext'     => 'Disse gebruker hef gien geldig e-mailadres in-evoerd, of wil gien berichen van aandere gebrukers ontvangen.',
'emailfrom'       => 'Van:',
'emailto'         => 'An:',
'emailsubject'    => 'Onderwarp:',
'emailmessage'    => 'Berich:',
'emailsend'       => 'Versturen',
'emailccme'       => 'Stuur mien een kopie van dit berich.',
'emailccsubject'  => 'Kopie van joew berich an $1: $2',
'emailsent'       => 'Berich verstuurd',
'emailsenttext'   => 'Berich is verzunnen.',
'emailuserfooter' => 'Dit berich is verstuurd deur $1 an $2 deur de functie "Een berich sturen" van {{SITENAME}} te gebruken.',

# Watchlist
'watchlist'            => 'Volglieste',
'mywatchlist'          => 'Mien volglieste',
'watchlistfor'         => "(veur '''$1''')",
'nowatchlist'          => 'Gien artikels in volglieste.',
'watchlistanontext'    => '$1 is verplich um joew volglieste te bekieken of te wiezigen.',
'watchnologin'         => 'Neet an-emeld',
'watchnologintext'     => 'Um je volglieste an te passen mu-j eers [[Special:UserLogin|an-emeld]] ween.',
'addedwatch'           => 'Disse pagina steet noen op joew volglieste',
'addedwatchtext'       => "De pagina \"[[:\$1]]\" steet noen op joew [[Special:Watchlist|volglieste]].
Toekomstige wiezigingen op disse pagina en de overlegpagina zullen hier vermeld wonnen, oek zullen disse pagina's '''vet-edrok''' ween in de lieste mit de [[Special:RecentChanges|leste wiezigingen]] zoda-j 't makkelijker zien kunnen.",
'removedwatch'         => 'Van volglieste ofhaolen',
'removedwatchtext'     => 'De pagina "$1" is van joew volglieste op-ehaold.',
'watch'                => 'Volg',
'watchthispage'        => 'Volg disse pagina',
'unwatch'              => 'Neet volgen',
'unwatchthispage'      => 'Neet volgen',
'notanarticle'         => 'Gien artikel',
'notvisiblerev'        => 'Bewarking is vort-edaon',
'watchnochange'        => "Gien van de pagina's op joew volglieste is in disse periode ewiezig.",
'watchlist-details'    => "Der {{PLURAL:$1|steet één pagina|staon $1 pagina's}} op joew volglieste, zonder de overlegpagina's mee-erekend.",
'wlheader-enotif'      => 'Je kriegen berich per e-mail',
'wlheader-showupdated' => "* Pagina's dee ewiezig sinds je ze 't veur 't les bie-ewark hemmen, wonnen '''vet''' weer-egeven.",
'watchmethod-recent'   => "Bie de pagina's dee kortens ewiezig bin, ezoch naor pagina's dee evolg wonnen",
'watchmethod-list'     => 'Kik joew nao volglieste veur de leste wiezigingen',
'iteminvalidname'      => "Verkeerde naam '$1'",
'wlshowlast'           => 'Teun de leste $1 ure $2 dagen $3',
'watchlist-show-bots'  => 'Teun botgebrukers',
'watchlist-hide-bots'  => 'Verbarg botgebrukers',
'watchlist-show-own'   => 'Teun mien bewarkingen',
'watchlist-hide-own'   => 'Verbarg mien bewarkingen',
'watchlist-show-minor' => 'Teun kleine wiezigingen',
'watchlist-hide-minor' => 'Verbarg kleine wiezigingen',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Volg...',
'unwatching' => 'Neet volgen...',

'enotif_mailer'                => '{{SITENAME}}-berichgevingssysteem',
'enotif_reset'                 => "Markeer alle pagina's as bezoch.",
'enotif_newpagetext'           => 'Dit is een nieje pagina.',
'enotif_impersonal_salutation' => '{{SITENAME}}-gebruker',
'changed'                      => 'ewiezig',
'created'                      => 'an-emaak',
'enotif_subject'               => '{{SITENAME}}-pagina $PAGETITLE is $CHANGEDORCREATED deur $PAGEEDITOR',
'enotif_lastvisited'           => 'Zie $1 veur alle wiezigingen sinds joew leste bezeuk.',
'enotif_lastdiff'              => 'Zie $1 um disse wieziging te bekieken.',
'enotif_anon_editor'           => 'annenieme gebruker $1',

# Delete/protect/revert
'deletepage'                  => 'Vortdoon',
'confirm'                     => 'Bevestigen',
'excontent'                   => "De tekse was: '$1'",
'excontentauthor'             => "De tekse was: '$1' (pagina an-emaak deur: [[Special:Contributions/$2|$2]])",
'exbeforeblank'               => "veurdat disse pagina leeg-emaak wönnen stung hier: '$1'",
'exblank'                     => 'Pagina was leeg',
'delete-confirm'              => '"$1" vortdoon',
'delete-legend'               => 'Vortdoon',
'historywarning'              => 'Waorschuwing: disse pagina hef een veurgeschiedenisse. Kiek effen nao of je neet een ouwere versie van disse pagina herstellen kunnen.',
'confirmdeletetext'           => 'Disse actie wis alle inhoud en geschiedenisse uut de databanke. Bevestig hieronder dat dit de bedoeling is en da-j de gevolgen dervan begriepen.',
'actioncomplete'              => 'Uut-evoerd',
'deletedtext'                 => '\'t Artikel "$1" is vort-edaon. Zie de "$2" veur een lieste van pagina\'s dee as les vort-edaon bin.',
'deletedarticle'              => '"$1" vort-edaon',
'dellogpage'                  => "Vort-edaone pagina's",
'dellogpagetext'              => "Hieronder een lieste van pagina's en ofbeeldingen dee 't les vort-edaon bin.",
'deletionlog'                 => "Vort-edaone pagina's",
'deletecomment'               => 'Rejen',
'deleteotherreason'           => 'Aandere/extra rejen:',
'deletereasonotherlist'       => 'Aandere rejen',
'deletereason-dropdown'       => '*Veulveurkoemende rejens
** Op anvrage van de auteur
** Schending van de auteursrechen
** Vandelisme',
'rollback'                    => 'Wiezigingen herstellen',
'rollback_short'              => 'Weerummedreien',
'rollbacklink'                => 'Weerummedreien',
'rollbackfailed'              => 'Wieziging herstellen is mislok',
'cantrollback'                => 'De wiezigingen konnen neet hersteld wonnen; der is mar 1 auteur.',
'alreadyrolled'               => "'t Is neet meugelijk um de wieziging van de pagina [[$1]]
deur [[User:$2|$2]] ([[User talk:$2|Overleeg]]) te herstellen.

Een aander hef disse wieziging al hersteld tot een veurige versie van disse pagina of hef een aandere bewarking edaon.

De leste bewarking is edaon deur [[User:$3|$3]] ([[User talk:$3|Overleeg]]).",
'editcomment'                 => 'De samenvatting was: <i>$1</i>', # only shown if there is an edit comment
'revertpage'                  => 'Wiezigingen deur [[Special:Contributions/$2|$2]] hersteld tot de versie nao de leste wieziging deur $1', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => 'Wiezigingen van $1; weerummedreid naor de leste versie van $2.',
'sessionfailure'              => 'Der is een prebleem mit joew anmeldsessie. De actie is stop-ezet uut veurzörg tegen een beveiligingsrisico (dat besteet uut \'t meugelijke "kraken" van disse sessie). Gao een pagina weerumme, laot disse pagina opniej en prebeer \'t nog es.',
'protectlogpage'              => 'Beveiligingslogboek',
'protectlogtext'              => "Hieronder steet een lieste mit pagina's dee beveilig bin.",
'protectedarticle'            => '[[$1]] is beveilig',
'modifiedarticleprotection'   => 'beveiligingsnivo van "[[$1]]"  ewiezig',
'unprotectedarticle'          => '[[$1]] vrie-egeven',
'protectcomment'              => 'Rejen',
'protectexpiry'               => 'Duur',
'protect_expiry_invalid'      => 'Verlooptied is ongeldig.',
'protect_expiry_old'          => 'De verlooptied is al veurbie.',
'protect-unchain'             => 'Ontkoppel de naamwiezigingsrechen',
'protect-text'                => "Hier ku-j 't beveiligingsnivo veur de pagina <strong>$1</strong> instellen.",
'protect-locked-blocked'      => "Je kunnen beveiligingsnivo's neet wiezigen terwiel je eblokkeerd bin. Hier bin de instellingen zoas ze noen bin veur de pagina <strong>$1</strong>:",
'protect-locked-dblock'       => "Beveiligingsnivo's kunnen noen effen neet ewiezig wonnen umdat de databanke noen beveilig is.
Hier staon de instellingen zoas ze noen bin veur de pagina <strong>$1</strong>:",
'protect-locked-access'       => "Je hemmen gien rechen um 't beveilingsnivo van pagina's te wiezigen.
Hier staon de instellingen zoas ze noen bin veur de pagina <strong>$1</strong>:",
'protect-cascadeon'           => "Disse pagina wonnen beveilig umdat 't op-eneumen is in de volgende {{PLURAL:$1|pagina|pagina's}} dee beveilig {{PLURAL:$1|is|bin}} mit de cascade-optie. Je kunnen 't beveiligingsnivo van disse pagina anpassen, mar dat hef gien invleud op de cascadebeveiliging.",
'protect-default'             => '(standard)',
'protect-fallback'            => 'Hierveur is \'t rech "$1" neudig',
'protect-level-autoconfirmed' => 'Allinnig an-emelde gebrukers',
'protect-level-sysop'         => 'Allinnig beheerders',
'protect-summary-cascade'     => 'cascade',
'protect-expiring'            => 'verloop op $1 (UTC)',
'protect-cascade'             => "Cascadebeveiliging (beveilig alle pagina's en sjablonen dee in disse pagina op-eneumen bin)",
'protect-cantedit'            => "Je kunnen 't beveiligingsnivo van disse pagina neet wiezigen, umda-j gien rechen hemmen um 't te bewarken.",
'restriction-type'            => 'Toegang',
'restriction-level'           => 'Beveiligingsnivo',
'minimum-size'                => 'Minimumgrootte (bytes)',
'maximum-size'                => 'Maximumgrootte',
'pagesize'                    => '(byte)',

# Restrictions (nouns)
'restriction-edit'   => 'Bewark',
'restriction-move'   => 'Herneum',
'restriction-create' => 'Anmaken',

# Restriction levels
'restriction-level-sysop'         => 'volledige beveiliging',
'restriction-level-autoconfirmed' => 'semibeveilig',
'restriction-level-all'           => 'alles',

# Undelete
'undelete'                 => "Vort-edaone pagina's bekieken",
'undeletepage'             => "Vort-edaone pagina's bekieken en weerummeplaosen",
'viewdeletedpage'          => "Bekiek vort-edaone pagina's",
'undeletepagetext'         => 'Disse pagina is vort-edaon, mar steet in de kas en kan nog weerummeplaos wonnen.',
'undeleteextrahelp'        => "Um de pagina mit alle eerdere versies weerumme te plaosen lao-j alle hokjes leeg en klik op '''''Weerummeplaosen!'''''.
Um een bepaolde versies weerumme te plaosen mu-j de versies dee-j weerummeplaosen willen anvinken en klik op '''''Weerummeplaosen!'''''.
Um een bulte achter mekaarstaonde versies te kiezen mu-j de eerste in de reeks anvinken en vervolgens mit de schuufknoppe in-edrok de leste anvinken. Hierdeur wonnen oek alle tussenliggende versies mee-eneumen.
A-j op '''''Herstel''''' klikken wonnen 't infermasieveld en alle hokjes leeg-emaak.",
'undeletehistory'          => 'A-j een pagina weerummeplaosen, wonnen alle versies as ouwe versies weerummeplaos. 
As der al een nieje pagina mit dezelfde naam an-emaak is, zullen disse versies as ouwe versies weerummeplaos wonnen, mar de op-esleugen versie zal neet ewiezig wonnen.',
'undeletehistorynoadmin'   => "Disse pagina is vort-edaon. De rejen hierveur steet hieronder, samen mit de infermasie van de gebrukers dee dit artikel ewiezig hemmen veurdat 't vort-edaon is. De tekse van 't artikel is allinnig zichbaor veur beheerders.",
'undeleterevision-missing' => "Ongeldige of ontbrekende versie. 't Is meugelijk da-j een verkeerde verwiezing gebruken of dat disse pagina weerummeplaos is of dat 't uut archief ewis is.",
'undeletebtn'              => 'Weerummeplaosen',
'undeletelink'             => 'weerummeplaosen',
'undeletereset'            => 'Herstel',
'undeletecomment'          => 'Opmarking:',
'undeletedarticle'         => '"$1" is weerummeplaos',
'cannotundelete'           => "Weerummeplaosen van 't bestand is mislok; iemand aanders hef disse pagina meschien al weerummeplaos.",
'undeletedpage'            => "<big>'''$1 is weerummeplaos'''</big>

Bekiek 't [[Special:Log/delete|logboek vort-edaone pagina's]] veur een overzichte van pagina's dee kortens vort-edaon en weerummeplaos bin.",
'undelete-header'          => "Zie [[Special:Log/delete|'t logboek vort-edaone pagina's]] veur pagina's dee 't les vort-edaon bin.",
'undelete-search-box'      => "Deurzeuk vort-edaone pagina's",
'undelete-search-prefix'   => "Teun pagina's vanof:",
'undelete-search-submit'   => 'Zeuk',
'undelete-no-results'      => "Gien pagina's evunnen in 't archief mit vort-edaone pagina's.",

# Namespace form on various pages
'namespace'      => 'Naamruumte:',
'invert'         => 'selectie ummekeren',
'blanknamespace' => '(encyclopedie)',

# Contributions
'contributions' => 'Biedragen van disse gebruker',
'mycontris'     => 'Mien biedragen',
'contribsub2'   => 'Veur $1 ($2)',
'nocontribs'    => 'Gien wiezigingen evunnen dee an de estelde criteria voldoon.',
'uctop'         => ' (leste wieziging)',
'month'         => 'Maond:',
'year'          => 'Jaor:',

'sp-contributions-newbies'     => 'Teun allinnig de biedragen van nieje gebrukers',
'sp-contributions-newbies-sub' => 'Veur niejelingen',
'sp-contributions-blocklog'    => 'Blokkeerlogboek',
'sp-contributions-search'      => 'Zeuken naor biedragen',
'sp-contributions-username'    => 'IP-adres of gebrukersnaam:',
'sp-contributions-submit'      => 'Zeuk',

# What links here
'whatlinkshere'            => 'Verwiezingen naor disse pagina',
'whatlinkshere-title'      => 'Pagina\'s dee verwiezen naor "$1"',
'whatlinkshere-page'       => 'Pagina:',
'linklistsub'              => '(lieste van verwiezingen)',
'linkshere'                => "Disse pagina's verwiezen naor '''[[:$1]]''':",
'nolinkshere'              => "Gien enkele pagina verwies naor '''[[:$1]]'''.",
'nolinkshere-ns'           => "Gien enkele pagina verwiest naor '''[[:$1]]''' in de ekeuzen naamruumte.",
'isredirect'               => 'deurverwiezing',
'istemplate'               => 'in-evoeg as sjabloon',
'whatlinkshere-prev'       => '{{PLURAL:$1|veurige|veurige $1}}',
'whatlinkshere-next'       => '{{PLURAL:$1|volgende|volgende $1}}',
'whatlinkshere-links'      => '← verwiezingen',
'whatlinkshere-hideredirs' => '$1 deurverwiezingen',
'whatlinkshere-hidetrans'  => '$1 in-evoegen sjablonen',
'whatlinkshere-hidelinks'  => '$1 verwiezingen',
'whatlinkshere-hideimages' => '$1 bestansverwiezingen',
'whatlinkshere-filters'    => 'Filters',

# Block/unblock
'blockip'                     => 'Gebruker blokkeren',
'blockiptext'                 => "Gebruuk dit formelier um een IP-adres te blokkeren. 't Is bedoeld um vandelisme te veurkoemen. Misbruuk van disse functie zal tot gevolg hemmen dat de staotus van beheerder of-eneumen zal wonnen.",
'ipaddress'                   => 'IP-adres:',
'ipadressorusername'          => 'IP-adres of gebrukersnaam',
'ipbexpiry'                   => 'Verloop nao',
'ipbreason'                   => 'Rejen',
'ipbreasonotherlist'          => 'aandere rejen',
'ipbreason-dropdown'          => "*Algemene rejens veur 't blokkeren
** pagina's leegmaken
** ongewunste verwiezingen toevoegen
** anmaken van onzinpagina's
** targerieje en/of intimiderend gedrag
** sokpopperieje
** onacceptabele gebrukersnaam
** vandelisme",
'ipbanononly'                 => 'Blokkeer allinnig annenieme gebrukers',
'ipbcreateaccount'            => "Veurkom 't anmaken van gebrukersprefielen",
'ipbemailban'                 => 'Veurkom dat bepaolde gebrukers berichen versturen',
'ipbenableautoblock'          => 'De IP-adressen van disse gebruker vanzelf blokkeren',
'ipbsubmit'                   => 'adres blokkeren',
'ipbother'                    => 'Aandere tied',
'ipboptions'                  => '2 uren:2 hours,1 dag:1 day,3 dagen:3 days,1 weke:1 week,2 weken:2 weeks,1 maond:1 month,3 maonden:3 months,6 maonden:6 months,1 jaor:1 year,onbepark:infinite', # display1:time1,display2:time2,...
'ipbotheroption'              => 'aanders',
'ipbotherreason'              => 'Aandere/extra rejen:',
'ipbhidename'                 => "Verbarg de gebrukersnaam of 't IP-adres van 't blokkeerlogboek, de actieve blokkeerlieste en de gebrukerslieste",
'badipaddress'                => 'ongeldig IP-adres of onbestaonde gebrukersnaam',
'blockipsuccesssub'           => 'Succesvol eblokkeerd',
'blockipsuccesstext'          => 'IP-adres "$1" is noen eblokkeerd.

Op de [[Special:IPBlockList|IP-blokkeerlieste]] steet een lieste mit alle blokkeringen.',
'ipb-edit-dropdown'           => 'Blokkeerrejens bewarken',
'ipb-unblock-addr'            => 'Deblokkeer $1',
'ipb-unblock'                 => 'Deblokkeer een gebruker of IP-adres',
'ipb-blocklist-addr'          => 'Bekiek bestaonde blokkeringen veur $1',
'ipb-blocklist'               => 'Bekiek bestaonde blokkeringen',
'unblockip'                   => 'Deblokkeer gebruker',
'unblockiptext'               => "Gebruuk 't onderstaonde formelier um weerumme schrieftoegang te geven an een eblokkeren gebruker of IP-adres.",
'ipusubmit'                   => 'Dit adres deblokkeren',
'unblocked'                   => '[[User:$1|$1]] is edeblokeerd',
'unblocked-id'                => 'Blokkade $1 is derof ehaold',
'ipblocklist'                 => 'Lieste van IP-adressen en gebrukers dee eblokkeerd bin',
'ipblocklist-legend'          => 'Een eblokkeren gebruker zeuken',
'ipblocklist-username'        => 'Gebrukersnaam of IP-adres:',
'ipblocklist-submit'          => 'Zeuk',
'blocklistline'               => 'Op $1 (vervuilt op $4) blokkeren $2: $3',
'infiniteblock'               => 'onbepark',
'expiringblock'               => '$1',
'anononlyblock'               => 'allinnig anneniemen',
'noautoblockblock'            => 'autoblok neet actief',
'createaccountblock'          => 'anmaken van een gebrukersprefiel is eblokkeerd',
'emailblock'                  => "'t versturen van berichen is eblokkeerd",
'ipblocklist-empty'           => 'De blokkeerlieste is leeg.',
'ipblocklist-no-results'      => "'t Op-evreugen IP-adres of de gebrukersnaam is neet eblokkeerd.",
'blocklink'                   => 'Blokkeer',
'unblocklink'                 => 'deblokkeer',
'contribslink'                => 'Biedragen',
'autoblocker'                 => 'Vanzelf eblokkeerd umdat \'t IP-adres overenekump mit \'t IP-adres van [[User:$1|$1]], dee eblokkeerd is mit as rejen: "$2"',
'blocklogpage'                => 'Blokkeerlogboek',
'blocklogentry'               => 'blokkeren "[[$1]]" veur $2 $3',
'blocklogtext'                => "Hier zie-j een lieste van de leste blokkeringen en deblokkeringen. Autematische blokkeringen en deblokkeringen koemen neet in 't logboek te staon. Zie de [[Special:IPBlockList|IP-blokkeerlieste]] veur de lieste van adressen dee noen eblokkeerd bin.",
'unblocklogentry'             => 'Blokkering van [[$1]] op-eheven',
'block-log-flags-anononly'    => 'allinnig anneniemen',
'block-log-flags-nocreate'    => 'anmaken van gebrukersprefielen uut-eschakeld',
'block-log-flags-noautoblock' => 'autoblokkeren uut-eschakeld',
'block-log-flags-noemail'     => "'t versturen van berichen is eblokkeerd",
'range_block_disabled'        => 'De meugelijkheid veur beheerders um een groep adressen te blokkeren is uut-eschakeld.',
'ipb_expiry_invalid'          => 'De op-egeven verlooptied is ongeldig.',
'ipb_already_blocked'         => '"$1" is al eblokkeerd',
'ipb_cant_unblock'            => "Foutmelding: blokkade ID $1 neet evunnen, 't is meschien al edeblokkeerd.",
'blockme'                     => 'Mien blokkeren',
'proxyblocker'                => 'Proxyblokker',
'proxyblockreason'            => 'Dit is een autematische preventieve blokkering umda-j gebruuk maken van een open proxyserver.',
'proxyblocksuccess'           => 'Succesvol.',
'sorbsreason'                 => 'Joew IP-adres is op-eneumen as open proxyserver in de DNS-blacklist de {{SITENAME}} ebruukt.',
'sorbs_create_account_reason' => 'Joew IP-adres is op-eneumen as open proxyserver in de DNS-blacklist de {{SITENAME}} ebruukt.
Je kunnen gien gebrukerspagina anmaken.',

# Developer tools
'lockdb'              => 'Databanke blokkeren',
'unlockdb'            => 'Databanke vriegeven',
'lockdbtext'          => "Waorschuwing: de databanke blokkeren hef tot gevolg dat gien enkele gebruker meer in staot is um pagina's te bewarking, d'r veurkeuren te wiezing of iets aanders te doon waorveur der wiezigingen in de databanke neudig bin.",
'unlockdbtext'        => 'Vriegeven van de databanke maak alle bewarkingen weer meugelijk.
Mut de databanke vrie-egeven wonnen?',
'lockconfirm'         => 'Ja, ik wille de databanke blokkeren.',
'unlockconfirm'       => 'Ja, ik wille de databanke vriegeven.',
'lockbtn'             => 'Databanke blokkeren',
'unlockbtn'           => 'Databanke vriegeven',
'locknoconfirm'       => "Je hemmen 't vakjen neet esillekteerd um joew keuze te bevestigen.",
'lockdbsuccesssub'    => 'Databanke succesvol eblokkeerd',
'unlockdbsuccesssub'  => 'Blokkering van de databanke is op-eheven.',
'lockfilenotwritable' => "Gien schriefrechen op 't beveiligingsbestand van de databanke. Um de databanke te blokkeren of de blokkade op te heffen, mut der eschreven kunnen wonnen deur de webserver.",
'databasenotlocked'   => 'De databanke is neet eblokkeerd.',

# Move page
'movepagetext'            => "Deur 't formelier da-j hieronder zien in te vullen ku-j de naam wiezigen, zo geet de veurgeschiedenisse neet verleuren. De ouwe paginanaam zal autematisch een deurverwiezing wonnen naor de nieje pagina (disse pagina kan, zoas op alle artikels mit een deurverwiezing, an-epas wonnen). Deurverwiezingen wonnen neet meeveraanderd en mutten mit de haand ewiezig wonnen.",
'movepagetalktext'        => "De biebeheurende overlegpagina krieg oek een nieje titel, mar '''neet''' in de volgende gevallen:
* As de pagina in een aandere naamruumte eplaos wonnen
* As der al een neet-lege overlegpagina besteet onder de aandere naam
* A-j 't onderstaonde vinkjen vorthaolen",
'movearticle'             => 'Herneum',
'newtitle'                => 'Nieje naam',
'move-watch'              => 'volg disse pagina',
'movepagebtn'             => 'Herneum',
'pagemovedsub'            => 'Naamwieziging succesvol',
'movepage-moved'          => '<big>\'\'\'"$1" is ewiezig naor "$2"\'\'\'</big>', # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => 'Onder disse naam besteet al een pagina. Kies een aandere naam.',
'cantmove-titleprotected' => "Je kunnen gien pagina naor disse titel herneumen, umdat de nieje titel beveilig is tegen 't anmaken dervan.",
'talkexists'              => "De pagina zelf is verplaos, mar de overlegpagina kon neet verplaos wonnen, umdat de doelnaam al een neet-lege overlegpagina had. Combineer de overlegpagina's haandmaotig.",
'movedto'                 => 'wiezigen naor',
'movetalk'                => "De overlegpagina oek wiezigen, as 't meuglijk is.",
'1movedto2'               => '[[$1]] is ewiezig naor [[$2]]',
'1movedto2_redir'         => '[[$1]] is ewiezig over de deurverwiezing [[$2]] hinne',
'movelogpage'             => 'Titelwiezigingen',
'movelogpagetext'         => "Hieronder steet een lieste mit pagina's dee herneumd bin.",
'movereason'              => 'Rejen:',
'revertmove'              => 'Weerummedreien',
'delete_and_move'         => 'Vortdoon en herneumen',
'delete_and_move_text'    => '==Mut vort-edaon wonnen==
<div style="color: red"> Onder de nieje naam "[[:$1]]" besteet al een artikel. Wi-j \'t vortdoon um plaose te maken veur \'t herneumen?</div>',
'delete_and_move_confirm' => 'Ja, disse pagina vortdoon',
'delete_and_move_reason'  => 'Vort-edaon vanwegen naamwieziging',
'selfmove'                => "De naam kan neet ewiezig wonnen naor de naam dee 't al hef.",
'immobile_namespace'      => "De nieje naam is een speciaal type; der kunnen gien pagina's in disse naamruumte eplaos wonnen.",

# Export
'export'            => "Pagina's uutvoeren",
'exporttext'        => "De tekse en geschiedenis van een bepaolen pagina of een antal pagina's kunnen in XML-formaot uut-evoerd wonnen. 't Kan daornao naor een aandere MediaWiki-wiki in-evoerd, verwark of gewoon op-esleugen wonnen.",
'exportcuronly'     => 'Allinnig de actuele versie, neet de veurgeschiedenisse',
'exportnohistory'   => "----
'''NB:''' 't uutvoeren van de hele geschiedenisse is uut-eschakeld vanwegen prestasierejens.",
'export-submit'     => 'Uutvoeren',
'export-addcattext' => "Pagina's toevoegen uut kattegerie:",

# Namespace 8 related
'allmessages'               => 'Alle systeemteksten',
'allmessagesdefault'        => 'Standardtekse',
'allmessagescurrent'        => 'De leste versie',
'allmessagestext'           => 'Hier vie-j alle berichen in de MediaWiki-naamruumte:',
'allmessagesnotsupportedDB' => "Disse pagina kan neet gebruuk wonnen umdat '''\$wgUseDatabaseMessages''' uut-eschakeld is.",
'allmessagesfilter'         => 'Berichnaamfilter:',
'allmessagesmodified'       => 'Allinnig teksen dee ewiezig bin',

# Thumbnails
'thumbnail-more'           => 'vergroten',
'filemissing'              => 'Bestand ontbreek',
'thumbnail_error'          => "Fout bie 't laojen van 't ofbeeldingsoverzichte: $1",
'djvu_page_error'          => 'DjVu-pagina buten bereik',
'djvu_no_xml'              => "Kon de XML-gevens veur 't DjVu-bestand neet opreupen",
'thumbnail_invalid_params' => 'Ongeldige ofbeeldingsoverzichparremeters',
'thumbnail_dest_directory' => 'De bestemmingsmap kon neet an-emaak wonnen.',

# Special:Import
'import'                     => "Pagina's invoeren",
'importinterwiki'            => 'Transwiki-invoer',
'import-interwiki-text'      => "Kies een wiki en paginanaam um in te voeren.
Versie- en auteursgegevens blieven hierbie beweerd.
Alle transwiki-invoerhaandelingen wonnen op-esleugen in 't [[Special:Log/import|invoerlogboek]].",
'import-interwiki-history'   => 'Kopieer de hele geschiedenisse veur disse pagina',
'import-interwiki-submit'    => 'Invoeren',
'import-interwiki-namespace' => "Plaos pagina's in de volgende naamruumte:",
'importtext'                 => "Gebruuk de Special:Export-optie in de wiki waor de infermasie vandaonkump, slao 't op joew eigen systeem op, en stuur 't daornao hier op.",
'importstart'                => "Pagina's an 't invoeren...",
'importnopages'              => "Der bin gien pagina's um in te voeren.",
'importfailed'               => 'Invoeren is mislok: $1',
'importunknownsource'        => 'Onbekend invoerbrontype',
'importcantopen'             => "Kon 't invoerbestand neet los doon",
'importbadinterwiki'         => 'Foute interwikiverwiezing',
'importnotext'               => 'Leeg of gien tekse',
'importsuccess'              => 'Invoeren succesvol!',
'importhistoryconflict'      => 'Der bin konflikken in de geschiedenisse van de pagina (is meschien eerder al in-evoerd)',
'importnosources'            => 'Gien transwiki-invoerbronnen edefinieerd en drekte geschiedenistoevoegingen bin eblokkeerd.',
'importnofile'               => 'Der is gien invoerbestand toe-evoeg.',

# Import log
'importlogpage'             => 'Invoerlogboek',
'importlogpagetext'         => "Administratieve invoer van pagina's mit geschiedenisse van aandere wiki's.",
'import-logentry-upload'    => '[[$1]] in-evoerd via een bestanstoevoeging',
'import-logentry-interwiki' => 'transwiki $1',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'Miene gebroekersbladziede',
'tooltip-pt-anonuserpage'         => "Gebroekersbladziede vuur t IP-adres da'j broekt",
'tooltip-pt-mytalk'               => 'Miene oaverlegbladziede',
'tooltip-pt-anontalk'             => 'Oaverlegbladziede van n naamloosn gebroeker van dit IP-adres',
'tooltip-pt-preferences'          => 'Miene vuurkeurn',
'tooltip-pt-watchlist'            => 'Lieste van bladziedn dee op miene voalglieste stoan',
'tooltip-pt-mycontris'            => 'Miene biejdreagn',
'tooltip-pt-login'                => 'Iej wördt van harte oetneugd um oe an te mealdn as gebroeker, mer t is nich verplicht',
'tooltip-pt-anonlogin'            => 'Iej wördt van harte oetneugd um oe an te mealdn as gebroeker, mer t is nich verplicht',
'tooltip-pt-logout'               => 'Ofmealdn',
'tooltip-ca-talk'                 => 'Loat n oaverlegtekst oaver disse bladziede zeen',
'tooltip-ca-edit'                 => 'Beweark disse bladziede',
'tooltip-ca-addsection'           => 'Voog oew kommentoar too an de oaverlegbladziede',
'tooltip-ca-viewsource'           => 'Disse bladziede is beveiligd teagn veraandern. Iej könt wal kiekn noar de bladziede',
'tooltip-ca-history'              => 'Oaldere versies van disse bladziede',
'tooltip-ca-protect'              => 'Beveilig disse bladziede teagn veraandern',
'tooltip-ca-delete'               => 'Smiet disse bladziede vort',
'tooltip-ca-undelete'             => 'Haal n inhoald van disse bladziede oet n emmer',
'tooltip-ca-move'                 => 'Gef disse bladziede nen aandern titel',
'tooltip-ca-watch'                => 'Voog disse bladziede too an oewe voalglieste',
'tooltip-ca-unwatch'              => 'Smiet disse bladziede van oewe voalglieste',
'tooltip-search'                  => '{{SITENAME}} deurzeuken',
'tooltip-search-fulltext'         => "De pagina's vuur disse tekst zeukn",
'tooltip-p-logo'                  => 'Vuurziede',
'tooltip-n-mainpage'              => 'Goa noar de vuurziede',
'tooltip-n-portal'                => 'Informoasie oaver t projekt: wel, wat, hoo en woarum',
'tooltip-n-currentevents'         => 'Achtergroondinformoasie oaver dinge in t niejs',
'tooltip-n-recentchanges'         => 'Lieste van pas verrichte veraanderingn',
'tooltip-n-randompage'            => 'Loat ne willekeurige bladziede zeen',
'tooltip-n-help'                  => 'Hölpinformoasie oaver {{SITENAME}}',
'tooltip-t-whatlinkshere'         => 'Lieste van alle bladziedn dee hiernoar verwiezn',
'tooltip-t-recentchangeslinked'   => 'Pas verrichte veraanderingn dee noar disse bladziede verwiezn',
'tooltip-feed-rss'                => 'Rss-feed vuur disse bladziede',
'tooltip-feed-atom'               => 'Atom-feed vuur disse bladziede',
'tooltip-t-contributions'         => 'Lieste met biejdreagn van disse gebroeker',
'tooltip-t-emailuser'             => 'Stuur disse gebroeker n iejmeel',
'tooltip-t-upload'                => 'Laad ofbeeldingn en/of geluudsmateriaal',
'tooltip-t-specialpages'          => 'Lieste van alle biejzeundere bladziedn',
'tooltip-t-print'                 => 'De ofdrukboare versie van disse bladziede',
'tooltip-t-permalink'             => 'Verbeending vuur altied noar de versie van disse bladziede van vandaag-an-n-dag',
'tooltip-ca-nstab-main'           => 'Loat n tekst van t artikel zeen',
'tooltip-ca-nstab-user'           => 'Loat de gebroekersbladziede zeen',
'tooltip-ca-nstab-media'          => 'Loat n mediatekst zeen',
'tooltip-ca-nstab-special'        => "Dit is ne biejzeundere bladziede dee'j nich könt veraandern",
'tooltip-ca-nstab-project'        => 'Loat de projektbladziede zeen',
'tooltip-ca-nstab-image'          => 'Loat de ofbeeldingnbladziede zeen',
'tooltip-ca-nstab-mediawiki'      => 'Loat de systeemtekstbladziede zeen',
'tooltip-ca-nstab-template'       => 'Loat de sjabloonbladziede zeen',
'tooltip-ca-nstab-help'           => 'Loat de hölpbladziede zeen',
'tooltip-ca-nstab-category'       => 'Loat de rubriekbladziede zeen',
'tooltip-minoredit'               => 'Markeer as een kleine wieziging',
'tooltip-save'                    => 'Wiezigingen opslaon',
'tooltip-preview'                 => "Bekiek joew versie veurda-j 't opslaon (anbeveulen)!",
'tooltip-diff'                    => 'Teun de deur joe an-ebröchen wiezigingen.',
'tooltip-compareselectedversions' => 'Teun de verschillen tussen de ekeuzen versies.',
'tooltip-watch'                   => 'Voeg disse pagina toe an joew volglieste',
'tooltip-recreate'                => "Disse pagina opniej anmaken, ondanks 't feit dat 't vort-edaon is.",
'tooltip-upload'                  => 'Bestaandn toovoogn',

# Metadata
'nodublincore'      => 'Dublin Core RDF-metadata is uut-eschakeld op disse server.',
'nocreativecommons' => 'Creative Commons RDF-metadata is uut-eschakeld op disse server.',
'notacceptable'     => 'De wikiserver kan de gegevens neet leveren in een vorm dee joew cliënt kan lezen.',

# Attribution
'anonymous'        => 'Annenieme gebruker(s) van {{SITENAME}}',
'siteuser'         => '{{SITENAME}}-gebruker $1',
'lastmodifiedatby' => "Disse pagina is 't les ewiezig op $2, $1 deur $3.", # $1 date, $2 time, $3 user
'othercontribs'    => 'Ebaseerd op wark van $1.',
'others'           => 'aandere',
'siteusers'        => '{{SITENAME}}-gebruker(s) $1',
'creditspage'      => 'Pagina-auteurs',
'nocredits'        => 'Der is gien auteursinfermasie beschikbaor veur disse pagina.',

# Spam protection
'spamprotectiontext'  => 'De pagina dee-j opslaon wollen is eblokkeerd deur de ongewunsteverwiezingsfilter. 
Meestentieds wonnen dit veroorzaak deur een uutgaonde verwiezing dee op de zwarte lieste steet.',
'spamprotectionmatch' => 'Disse tekse zörgen derveur dat onze spamfilter alarmsleug: $1',
'spambot_username'    => 'MediaWiki keren van ongewunste toevoegingen',
'spam_reverting'      => "Bezig mit 't weerummezetten naor de leste versie dee gien verwiezing hef naor $1",
'spam_blanking'       => 'Alle wiezigingen mit een verwiezing naor $1 wonnen vort-ehaold',

# Info page
'infosubtitle'   => 'Infermasie veur disse pagina',
'numedits'       => 'Antal bewarkingen (artikel): $1',
'numtalkedits'   => 'Antal bewarkingen (overlegpagina): $1',
'numwatchers'    => 'Antal volgers: $1',
'numauthors'     => 'Antal verschillende auteurs (artikel): $1',
'numtalkauthors' => 'Antal verschillende auteurs (overlegpagina): $1',

# Math options
'mw_math_png'    => 'Altied as PNG weergeven',
'mw_math_simple' => 'HTML veur eenvoudige formules, aanders PNG',
'mw_math_html'   => "HTML as 't meugelijk is, aanders PNG",
'mw_math_source' => 'Laot TeX-broncode staon (veur teksblaojeraars)',
'mw_math_modern' => 'Anbeveulen methode veur niejere webblaojeraars',
'mw_math_mathml' => 'MathML',

# Patrolling
'markaspatrolleddiff'                 => 'Markeer as econtroleerd',
'markaspatrolledtext'                 => 'Disse pagina is emarkeerd as econtroleerd',
'markedaspatrolled'                   => 'As econtroleerd emarkeerd',
'markedaspatrolledtext'               => 'De ekeuzen versie is emarkeerd as econtroleerd.',
'rcpatroldisabled'                    => 'De controlemeugelijkheid op leste wiezigingen is uut-eschakeld.',
'rcpatroldisabledtext'                => 'De meugelijkheid um recente wiezigingen as econtroleerd an te marken is op dit ogenblik uut-eschakeld.',
'markedaspatrollederror'              => 'De bewarking kon neet of-evink wonnen.',
'markedaspatrollederrortext'          => "Je mutten een wieziging sillecteren um 't as nao-ekeken te markeren.",
'markedaspatrollederror-noautopatrol' => 'Je maggen joew eigen bewarkingen neet as econtroleerd markeren.',

# Patrol log
'patrol-log-line' => '$1 van $2 emarkeerd as econtroleerd $3',
'patrol-log-auto' => '(autematisch)',

# Image deletion
'deletedrevision'              => 'Vort-edaone ouwe versie $1.',
'filedelete-archive-read-only' => 'De webserver kan neet in de archiefmap "$1" schrieven.',

# Browsing diffs
'previousdiff' => '← veurige wieziging',
'nextdiff'     => 'volgende wieziging →',

# Media information
'mediawarning'         => "'''Waorschuwing:''' dit bestand bevat meschien codering dee slich is veur 't systeem. <hr />",
'imagemaxsize'         => 'Maximumgrootte van ofbeeldingen op de beschrievingspagina:',
'thumbsize'            => "Grootte van 't ofbeeldingsoverzichte (thumbnail):",
'file-info'            => 'Bestansgrootte: $1, MIME-type: $2',
'file-info-size'       => '($1 × $2 beeldpunten, bestansgrootte: $3, MIME-type: $4)',
'file-nohires'         => '<small>Gien hogere resolusie beschikbaor.</small>',
'svg-long-desc'        => '(SVG-bestand, uutgangsgrootte $1 × $2 pixels, bestansgrootte: $3)',
'show-big-image'       => 'Ofbeelding in hogere resolusie',
'show-big-image-thumb' => '<small>Grootte van disse weergave: $1 × $2 beeldpunten</small>',

# Special:NewImages
'newimages'             => 'Nieje ofbeeldingen',
'noimages'              => 'Niks te zien.',
'ilsubmit'              => 'Zeuk',
'bydate'                => 'op daotum',
'sp-newimages-showfrom' => 'Teun nieje ofbeeldingen vanof $1, $2',

# Bad image list
'bad_image_list' => "De opmaak is as volg:

Allinnig regels in een lieste (regels dee beginnen mit *) wonnen verwark. De eerste verwiezing op een regel mut een verwiezing ween naor een ongewunste ofbeelding.
Alle volgende verwiezingen dee op dezelfde regel staon, wonnen behaandeld as uutzundering, zoas pagina's waorop de ofbeelding in te tekse op-eneumen is.",

# Metadata
'metadata'          => 'Metadata',
'metadata-help'     => 'Dit bestand bevat metadata mit EXIF-infermasie, dee deur een fotocamera, scanner of fotobewarkingspregramma toe-evoeg kan ween.',
'metadata-expand'   => 'Teun uut-ebreien gegevens',
'metadata-collapse' => 'Verbarg uut-ebreien gegevens',
'metadata-fields'   => 'EXIF-gegevens dee zichbaor bin as de tebel in-eklap is. De overige gegevens bin zichbaor as de tebel uut-eklap is, nao \'t klikken op "Teun uut-ebreien gegevens".
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => 'Wiedte',
'exif-imagelength'                 => 'Heugte',
'exif-bitspersample'               => 'Bits per compenent',
'exif-compression'                 => 'Compressiemethode',
'exif-photometricinterpretation'   => 'Beeldpuntsamenstelling',
'exif-orientation'                 => 'Oriëntasie',
'exif-ycbcrpositioning'            => 'Y- en C-posisionering',
'exif-stripoffsets'                => 'Lokasie ofbeeldingsgegevens',
'exif-jpeginterchangeformat'       => 'Ofstand tot JPEG SOI',
'exif-jpeginterchangeformatlength' => 'Bytes van JPEG-gegevens',
'exif-transferfunction'            => 'Overdrachsfunctie',
'exif-datetime'                    => 'Tiedstip van digitalisasie',
'exif-imagedescription'            => 'Ofbeeldingnaam',
'exif-make'                        => 'Camera-mark',
'exif-model'                       => 'Camera-medel',
'exif-software'                    => 'Pregrammetuur dee gebruuk wönnen',
'exif-artist'                      => 'Eschreven deur',
'exif-copyright'                   => 'Auteursrechenhouwer',
'exif-exifversion'                 => 'Exif-versie',
'exif-flashpixversion'             => 'Ondersteunen Flashpix-versie',
'exif-colorspace'                  => 'Kleurruumte',
'exif-componentsconfiguration'     => 'Betekenisse van elk compenent',
'exif-compressedbitsperpixel'      => 'Beeldcompressiemethode',
'exif-pixelydimension'             => 'Bruukbaore ofbeeldingsbreedte',
'exif-pixelxdimension'             => 'Bruukbaore ofbeeldingsheugte',
'exif-makernote'                   => 'Notities van de fabrikant',
'exif-usercomment'                 => 'Opmarkingen',
'exif-relatedsoundfile'            => 'Biebeheurend geluudsbestand',
'exif-datetimeoriginal'            => 'Tiedstip van datagenerasie',
'exif-datetimedigitized'           => 'Tiedstip van digitalisasie',
'exif-subsectime'                  => 'Subseconden tiedstip bestanswieziging',
'exif-subsectimeoriginal'          => 'Subseconden tiedstip dataginnerasie',
'exif-subsectimedigitized'         => 'Subseconden tiedstip digitalisasie',
'exif-exposuretime'                => 'Belochtingstied',
'exif-exposuretime-format'         => '$1 sec ($2)',
'exif-fnumber'                     => 'F-getal',
'exif-exposureprogram'             => 'Belochtingspregramma',
'exif-spectralsensitivity'         => 'Spectrale geveuligheid',
'exif-isospeedratings'             => 'ISO-weerde.',
'exif-oecf'                        => 'Opto-elektronische conversiefactor',
'exif-shutterspeedvalue'           => 'Slutersnelheid',
'exif-aperturevalue'               => 'Diafragma',
'exif-brightnessvalue'             => 'Helderheid',
'exif-exposurebiasvalue'           => 'Belochtingscompensasie',
'exif-maxaperturevalue'            => 'Maximale diafragmaweerde van de lenze',
'exif-subjectdistance'             => 'Ofstand tot onderwarp',
'exif-meteringmode'                => 'Methode lochmeting',
'exif-lightsource'                 => 'Lochbron',
'exif-flash'                       => 'Flitser',
'exif-focallength'                 => 'Braandpuntofstand',
'exif-subjectarea'                 => 'Objekruumte',
'exif-flashenergy'                 => 'Flitserstarkte',
'exif-spatialfrequencyresponse'    => 'Ruumtelijke frequentiereactie',
'exif-focalplanexresolution'       => 'X-resolutie van CDD',
'exif-focalplaneyresolution'       => 'Y-resolutie van CCD',
'exif-focalplaneresolutionunit'    => 'Eenheid CCD-resolusie',
'exif-subjectlocation'             => 'Objeklokasie',
'exif-exposureindex'               => 'Belochtingindex',
'exif-sensingmethod'               => 'Meetmethode',
'exif-filesource'                  => 'Oorspronkelijk bestansnaam',
'exif-scenetype'                   => 'Scènetype',
'exif-cfapattern'                  => 'CFA-petroon',
'exif-customrendered'              => 'An-epaste beeldbewarking',
'exif-exposuremode'                => 'Belochtingsinstelling',
'exif-whitebalance'                => 'Witbelans',
'exif-digitalzoomratio'            => 'Digitale zoomfactor',
'exif-focallengthin35mmfilm'       => 'Braandpuntofstand (35mm-equivalent)',
'exif-scenecapturetype'            => 'Soort opname',
'exif-gaincontrol'                 => 'Piekbeheersing',
'exif-contrast'                    => 'Kontras',
'exif-saturation'                  => 'Verzaojiging',
'exif-sharpness'                   => 'Scharpte',
'exif-devicesettingdescription'    => 'Umschrieving apperaotinstellingen',
'exif-subjectdistancerange'        => 'Ofstanskattegerie',
'exif-imageuniqueid'               => 'Unieke ID-ofbeelding',
'exif-gpsversionid'                => 'GPS-versienummer',
'exif-gpslatituderef'              => 'Noorder- of zujerbreedte',
'exif-gpslatitude'                 => 'Breedte',
'exif-gpslongituderef'             => 'Ooster- of westerlengte',
'exif-gpslongitude'                => 'Lengtegraod',
'exif-gpsaltituderef'              => 'Heugterifferentie',
'exif-gpsaltitude'                 => 'Heugte',
'exif-gpstimestamp'                => 'GPS-tied (atoomklokke)',
'exif-gpssatellites'               => 'Satellieten dee gebruuk bin veur de meting',
'exif-gpsstatus'                   => 'Ontvangerstaotus',
'exif-gpsmeasuremode'              => 'Meetmodus',
'exif-gpsdop'                      => 'Meetprecisie',
'exif-gpsspeedref'                 => 'Snelheidseenheid',
'exif-gpsspeed'                    => 'Snelheid van GPS-ontvanger',
'exif-gpstrackref'                 => 'Referentie veur bewegingsrichting',
'exif-gpstrack'                    => 'Bewegingsrichting',
'exif-gpsimgdirectionref'          => 'Referentie veur ofbeeldingsrichting',
'exif-gpsimgdirection'             => 'Ofbeeldingsrichting',
'exif-gpsmapdatum'                 => 'Geodetische onderzeuksgegevens dee gebruuk bin',
'exif-gpsdestlatituderef'          => 'Rifferentie veur breedtegraod tot bestemming',
'exif-gpsdestlatitude'             => 'Breedtegraod bestemming',
'exif-gpsdestlongituderef'         => 'Rifferentie veur lengtegraod bestemming',
'exif-gpsdestlongitude'            => 'Lengtegraod bestemming',
'exif-gpsdestbearingref'           => 'Rifferentie veur richting naor bestemming',
'exif-gpsdestbearing'              => 'Richting naor bestemming',
'exif-gpsdestdistanceref'          => 'Rifferentie veur ofstand tot bestemming',
'exif-gpsdestdistance'             => 'Ofstand tot bestemming',
'exif-gpsprocessingmethod'         => 'Naam van de GPS-verwarkingsmethode',
'exif-gpsareainformation'          => "Naam van 't GPS-gebied",
'exif-gpsdatestamp'                => 'GPS-daotum',
'exif-gpsdifferential'             => 'Differentiële GPS-correctie',

# EXIF attributes
'exif-compression-1' => 'Neet ecomprimeerd',

'exif-unknowndate' => 'Onbekende daotum',

'exif-orientation-1' => 'Normaal', # 0th row: top; 0th column: left
'exif-orientation-2' => 'horizontaal espegeld', # 0th row: top; 0th column: right
'exif-orientation-3' => '180° edreid', # 0th row: bottom; 0th column: right
'exif-orientation-4' => 'verticaal edreid', # 0th row: bottom; 0th column: left
'exif-orientation-5' => 'espegeld um as linksboven-rechsonder', # 0th row: left; 0th column: top
'exif-orientation-6' => '90° rechsummedreid', # 0th row: right; 0th column: top
'exif-orientation-7' => '90° linksummedreid', # 0th row: right; 0th column: bottom
'exif-orientation-8' => '90° linksummedreid', # 0th row: left; 0th column: bottom

'exif-planarconfiguration-1' => 'Grof gegevensformaot',
'exif-planarconfiguration-2' => 'planar gegevensformaot',

'exif-componentsconfiguration-0' => 'besteet neet',

'exif-exposureprogram-0' => 'Neet umschreven',
'exif-exposureprogram-1' => 'Haandmaotig',
'exif-exposureprogram-2' => 'Normaal',
'exif-exposureprogram-3' => 'Diafragmaprioriteit',
'exif-exposureprogram-4' => 'Sluterprioriteit',
'exif-exposureprogram-5' => 'Creatief (veurkeur veur grote scharptediepte)',
'exif-exposureprogram-6' => 'Actie (veurkeur veur hoge slutersnelheid)',
'exif-exposureprogram-7' => 'pertret (detailopname mit onscharpe achtergrond)',
'exif-exposureprogram-8' => 'laandschap (scharpe achtergrond)',

'exif-subjectdistance-value' => '$1 m',

'exif-meteringmode-0'   => 'Onbekend',
'exif-meteringmode-1'   => 'Gemiddeld',
'exif-meteringmode-2'   => 'Gemiddeld, naodrok op midden',
'exif-meteringmode-3'   => 'Spot',
'exif-meteringmode-4'   => 'MultiSpot',
'exif-meteringmode-5'   => 'Multi-segment (petroon)',
'exif-meteringmode-6'   => 'Deelmeting',
'exif-meteringmode-255' => 'Aanders',

'exif-lightsource-0'   => 'Onbekend',
'exif-lightsource-1'   => 'Dagloch',
'exif-lightsource-2'   => 'Tl-loch',
'exif-lightsource-3'   => 'Tungsten (lamploch)',
'exif-lightsource-4'   => 'Flitser',
'exif-lightsource-9'   => 'Mooi weer',
'exif-lightsource-10'  => 'Bewolk',
'exif-lightsource-11'  => 'Schaoduw',
'exif-lightsource-12'  => 'Fluorescerend dagloch (D 5700 – 7100K)',
'exif-lightsource-13'  => 'Witfluorescerend dagloch (N 4600 – 5400K)',
'exif-lightsource-14'  => 'Koel witfluorescerend (W 3900 – 4500K)',
'exif-lightsource-15'  => 'Witfluorescerend (WW 3200 – 3700K)',
'exif-lightsource-17'  => 'Standardloch A',
'exif-lightsource-18'  => 'Standardloch B',
'exif-lightsource-19'  => 'Standardloch C',
'exif-lightsource-24'  => 'ISO-studiokunsloch',
'exif-lightsource-255' => 'Aanders',

'exif-focalplaneresolutionunit-2' => 'duum',

'exif-sensingmethod-1' => 'Neet vastesteld',
'exif-sensingmethod-2' => 'Eén-chip-kleursensor',
'exif-sensingmethod-3' => 'Twee-chips-kleursensor',
'exif-sensingmethod-4' => 'Dree-chips-kleurensensor',
'exif-sensingmethod-5' => 'Kleurvolgende gebiedssensor',
'exif-sensingmethod-7' => 'Dreeliendige sensor',
'exif-sensingmethod-8' => 'Kleurvolgende liendesensor',

'exif-scenetype-1' => 'Een drek efotograferen ofbeelding',

'exif-customrendered-0' => 'Normaal',
'exif-customrendered-1' => 'An-epas',

'exif-exposuremode-0' => 'Autematisch',
'exif-exposuremode-1' => 'Haandmaotig',
'exif-exposuremode-2' => 'Autobracket',

'exif-whitebalance-0' => 'Autematisch',
'exif-whitebalance-1' => 'Haandmaotig',

'exif-scenecapturetype-0' => 'standard',
'exif-scenecapturetype-1' => 'laandschap',
'exif-scenecapturetype-2' => 'pertret',
'exif-scenecapturetype-3' => 'nachscène',

'exif-gaincontrol-0' => 'Gien',
'exif-gaincontrol-1' => 'Lege pieken umhoge',
'exif-gaincontrol-2' => 'Hoge pieken umhoge',
'exif-gaincontrol-3' => 'Lege pieken ummeneer',
'exif-gaincontrol-4' => 'Hoge pieken ummeneer',

'exif-contrast-0' => 'Normaal',
'exif-contrast-1' => 'Zachte',
'exif-contrast-2' => 'Hard',

'exif-saturation-0' => 'Normaal',
'exif-saturation-1' => 'Leeg',
'exif-saturation-2' => 'Hoge',

'exif-sharpness-0' => 'Normaal',
'exif-sharpness-1' => 'Zach',
'exif-sharpness-2' => 'Hard',

'exif-subjectdistancerange-0' => 'Onbekend',
'exif-subjectdistancerange-1' => 'Macro',
'exif-subjectdistancerange-2' => 'Dichtebie',
'exif-subjectdistancerange-3' => 'Veerof',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'Noorderbreedte',
'exif-gpslatitude-s' => 'Zujerbreedte',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'Oosterlengte',
'exif-gpslongitude-w' => 'Westerlengte',

'exif-gpsstatus-a' => 'Bezig mit meten',
'exif-gpsstatus-v' => 'Meetinteroperebiliteit',

'exif-gpsmeasuremode-2' => '2-dimensionale meting',
'exif-gpsmeasuremode-3' => '3-dimensionale meting',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'Kilemeter per uur',
'exif-gpsspeed-m' => 'Miel per ure',
'exif-gpsspeed-n' => 'Knopen',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => 'Waore richting',
'exif-gpsdirection-m' => 'Magnetische richting',

# External editor support
'edit-externally'      => 'Wiezig dit bestand mit een extern pregramma',
'edit-externally-help' => 'Zie de [http://www.mediawiki.org/wiki/Manual:External_editors installasie-instructies] veur meer infermasie.',

# 'all' in various places, this might be different for inflected languages
'imagelistall'  => 'alles',
'watchlistall2' => 'alles',
'namespacesall' => 'alles',
'monthsall'     => 'alles',

# E-mail address confirmation
'confirmemail'           => 'Bevestig e-mailadres',
'confirmemail_noemail'   => 'Je hemmen gien geldig e-mailadres in-evoerd in joew [[Special:Preferences|veurkeuren]].',
'confirmemail_text'      => 'Bie disse wiki mu-j je e-mailadres bevestigen veurda-j de berichopties gebruken kunnen. Klik op de onderstaonde knoppe um een bevestigingsberich te ontvangen. Dit berich bevat een code mit een verwiezing; um je e-mailadres te bevestigen mu-j disse verwiezing los doon.',
'confirmemail_pending'   => '<div class="error">
Der is al een bevestigingscode op-estuurd; a-j net een gebrukersnaam an-emaak hemmen, wach dan eers een paor menuten tot da-j dit berich ontvungen hemmen veurda-j een nieje code anvragen.
</div>',
'confirmemail_send'      => 'Stuur een bevestigingscode',
'confirmemail_sent'      => 'Bevestigingsberich verstuurd.',
'confirmemail_oncreate'  => "Een bevestigingscode is naor joew e-mailadres verstuurd. Disse code is neet neudig um an te melden, mar je mutten 't wel bevestigen veurda-j de e-mailmeugelijkheen van disse wiki gebruken kunnen.",
'confirmemail_invalid'   => 'Ongeldige bevestigingscode. De code kan verlopen ween.',
'confirmemail_needlogin' => 'Je muttnen $1 um joew e-mailadres te bevestigen.',
'confirmemail_success'   => 'Joew e-mailadres is bevestig. Je kunnen noen anmelden en {{SITENAME}} gebruken.',
'confirmemail_loggedin'  => 'Joew e-mailadres is noen bevestig.',
'confirmemail_error'     => "Der is iets fout egaon bie 't opslaon van joew bevestiging.",

# Scary transclusion
'scarytranscludedisabled' => '[Interwiki-intergrasie is edeactiveerd]',
'scarytranscludefailed'   => "['t Sjabloon $1 kon neet op-ehaold wonnen]",
'scarytranscludetoolong'  => '[URL is te lang]',

# Trackbacks
'trackbackbox'      => "<div id='mw_trackbacks'>
Trackbacks veur disse pagina:<br />
$1
</div>",
'trackbackremove'   => ' ([$1 vortdoon])',
'trackbackdeleteok' => 'De trackback is vort-edaon.',

# Delete conflict
'deletedwhileediting' => "'''Waorschuwing''': disse pagina is vort-edaon terwiel jie 't an 't bewarken wanen!",
'confirmrecreate'     => "Gebruker [[User:$1|$1]] ([[User talk:$1|Overleeg]]) hef disse pagina vort-edaon naoda-j  begunnen bin mit joew wieziging, mit opgave van de volgende rejen: ''$2''. Bevestig da-j 't artikel herschrieven willen.",
'recreate'            => 'Herschrieven',

# HTML dump
'redirectingto' => 'Bezig mit deursturen naor [[:$1]]...',

# action=purge
'confirm_purge'        => "Klik op 'bevestig' um de kas van disse pagina te legen.

$1",
'confirm_purge_button' => 'Bevestig',

# AJAX search
'searchcontaining' => "Zeuk naor artikels dee ''$1'' bevatten.",
'searchnamed'      => "Zeuk naor artikels mit de naam ''$1''.",
'articletitles'    => "Artikels dee beginnen mit ''$1''",
'hideresults'      => 'Verbarg risseltaoten',

# Multipage image navigation
'imgmultipageprev' => '&larr; veurige',
'imgmultipagenext' => 'volgende &rarr;',
'imgmultigo'       => 'Oké',

# Table pager
'ascending_abbrev'         => 'daol',
'descending_abbrev'        => 'stieg',
'table_pager_next'         => 'Volgende',
'table_pager_prev'         => 'Veurige',
'table_pager_last'         => 'Leste pagina',
'table_pager_limit'        => 'Teun $1 onderwarpen per pagina',
'table_pager_limit_submit' => 'Zeuk',
'table_pager_empty'        => 'Gien risseltaoten',

# Auto-summaries
'autosumm-blank'   => 'Pagina leeg-emaak',
'autosumm-replace' => "Tekse vervungen deur '$1'",
'autoredircomment' => 'deurverwiezing naor [[$1]]',
'autosumm-new'     => 'Nieje pagina: $1',

# Live preview
'livepreview-loading' => "An 't laojen…",
'livepreview-ready'   => "An 't laojen… ree!",
'livepreview-failed'  => 'Rechstreeks naokieken is neet meugelijk!
Kiek de pagina op de normale meniere nao.',
'livepreview-error'   => 'Verbiending neet meugelijk: $1 "$2"
Kiek de pagina op de normale meniere nao.',

# Friendlier slave lag warnings
'lag-warn-normal' => 'Wiezigingen dee niejer bin as $1 {{PLURAL:$1|seconde|seconden}} staon meschien nog neet in de lieste.',
'lag-warn-high'   => 'De databanke is aorig zwaor belas. Wiezigingen dee niejer bin as $1 {{PLURAL:$1|seconde|seconden}} staon daorumme meschien nog neet in de lieste.',

# Watchlist editor
'watchlistedit-noitems'        => 'Joew volglieste is leeg.',
'watchlistedit-normal-title'   => 'Volglieste bewarken',
'watchlistedit-normal-legend'  => "Disse pagina's van mien volglieste ofhaolen.",
'watchlistedit-normal-explain' => "Pagina's op joew volglieste wonnen hieronder weer-egeven.
Um een pagina van joew volglieste of te haolen mu-j 't vakjen dernaos sillekteren, en klik dan op 'Pagina's derof haolen'.
Je kunnen oek [[Special:Watchlist/raw|de roege lieste bewarken]].",
'watchlistedit-normal-submit'  => "Pagina's derof haolen",
'watchlistedit-normal-done'    => "Der {{PLURAL:$1|is 1 pagina|bin $1 pagina's}} vort-edaon uut joew volglieste:",
'watchlistedit-raw-title'      => 'Roewe volglieste bewarken',
'watchlistedit-raw-legend'     => 'Roewe volglieste bewarken',
'watchlistedit-raw-explain'    => "Hieronder staon pagina’s op joew volglieste. Je kunnen de lieste bewarken deur pagina’s deruut vort te haolen en derbie te te doon. 
Eén pagina per regel.
A-j ree bin, klik dan op ‘Volglieste biewarken’.
Je kunnen oek [[Special:Watchlist/edit|'t standard bewarkingsscharm gebruken]].",
'watchlistedit-raw-titles'     => 'Titels:',
'watchlistedit-raw-submit'     => 'Volglieste biewarken',
'watchlistedit-raw-done'       => 'Joew volglieste is bie-ewörk.',
'watchlistedit-raw-added'      => "Der {{PLURAL:$1|is 1 pagina|bin $1 pagina's}} bie edaon:",
'watchlistedit-raw-removed'    => "Der {{PLURAL:$1|is 1 pagina|bin $1 pagina's}} vort-edaon:",

# Watchlist editing tools
'watchlisttools-view' => 'Wiezigingen bekieken',
'watchlisttools-edit' => 'Volglieste bekieken en bewarken',
'watchlisttools-raw'  => 'Roewe volglieste bewarken',

# Core parser functions
'unknown_extension_tag' => 'Onbekende tag "$1"',

# Special:Version
'version'                          => 'Versie', # Not used as normal message but as header for the special page itself
'version-extensions'               => 'Eïnstalleren uutbreidingen',
'version-specialpages'             => "Speciale pagina's",
'version-parserhooks'              => 'Parserhooks',
'version-variables'                => 'Variabels',
'version-other'                    => 'Overige',
'version-mediahandlers'            => 'Mediaverwarkers',
'version-hooks'                    => 'Hooks',
'version-extension-functions'      => 'Uutbreidingsfuncties',
'version-parser-extensiontags'     => 'Parseruutbreidingsplaotjes',
'version-parser-function-hooks'    => 'Parserfunctiehooks',
'version-skin-extension-functions' => 'Vormgevingsuutbreidingsfuncties',
'version-hook-name'                => 'Hooknaam',
'version-hook-subscribedby'        => 'Eabonneerd deur',
'version-version'                  => 'Versie',
'version-license'                  => 'Licentie',
'version-software'                 => 'Eïnstalleren pregrammetuur',
'version-software-product'         => 'Preduk',
'version-software-version'         => 'Versie',

# Special:FilePath
'filepath'         => 'Bestanslokasie',
'filepath-page'    => 'Bestand:',
'filepath-submit'  => 'Zeuken',
'filepath-summary' => "Disse speciale pagina geef 't hele pad veur een bestand. Ofbeeldingen wonnen in resolusie helemaole weer-egeven. Aandere bestanstypen wonnen gelieke in 't mit 't MIME-type verbunnen pregramma los edaon.

Voer de bestansnaam in zonder 't veurvoegsel \"{{ns:image}}:\".",

# Special:FileDuplicateSearch
'fileduplicatesearch'          => 'Dubbele bestanden zeuken',
'fileduplicatesearch-summary'  => 'Dubbele bestanden zeuken op baosis van de hashweerde.

Voer de bestansnaam in zonder \'t veurvoegsel "{{ns:image}}:".',
'fileduplicatesearch-legend'   => 'Dubbele bestanden zeuken',
'fileduplicatesearch-filename' => 'Bestansnaam:',
'fileduplicatesearch-submit'   => 'Zeuken',
'fileduplicatesearch-info'     => '$1 × $2 beeldpunten<br />Bestansgrootte: $3<br />MIME-type: $4',
'fileduplicatesearch-result-1' => 'Der bin gien bestanden dee liekeleens bin as "$1".',
'fileduplicatesearch-result-n' => 'Der {{PLURAL:$2|is één bestand|bin $2 bestanden}} dee liekeleens bin as "$1".',

# Special:SpecialPages
'specialpages'                   => "Speciale pagina's",
'specialpages-note'              => '----
* Normale speciale pagina\'s
* <span class="mw-specialpagerestricted">Beteund toegankelijke speciale pagina\'s</span>',
'specialpages-group-maintenance' => 'Onderhoudsliesten',
'specialpages-group-other'       => "Overige speciale pagina's",
'specialpages-group-login'       => 'Anmelden / inschrieven',
'specialpages-group-changes'     => 'Leste wiezigingen en logboeken',
'specialpages-group-media'       => 'Media-overzichen en toe-evoegen bestanden',
'specialpages-group-users'       => 'Gebrukers en rechen',
'specialpages-group-highuse'     => "Veulgebruken pagina's",
'specialpages-group-pages'       => 'Paginaliesten',
'specialpages-group-pagetools'   => 'Paginahulpmiddels',
'specialpages-group-wiki'        => 'Wikigegevens en -hulpmiddels',
'specialpages-group-redirects'   => "Deurverwiezende speciale pagina's",
'specialpages-group-spam'        => "Hulpmiddels tegen 't plaosen van moek",

# Special:BlankPage
'blankpage'              => 'Lege pagina',
'intentionallyblankpage' => 'Disse pagina is bewus leeg eleuten.',

);
