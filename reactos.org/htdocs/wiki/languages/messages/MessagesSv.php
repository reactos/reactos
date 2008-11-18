<?php
/** Swedish (Svenska)
 *
 * @ingroup Language
 * @file
 *
 * @author Boivie
 * @author Grillo
 * @author Habj
 * @author Habjchen
 * @author Jon Harald Søby
 * @author Lejonel
 * @author Leo Johannes
 * @author Lokal Profil
 * @author M.M.S.
 * @author Max sonnelid
 * @author Micke
 * @author Najami
 * @author S.Örvarr.S
 * @author Sannab
 * @author Skalman
 * @author Steinninn
 * @author לערי ריינהארט
 */

$skinNames = array(
	'standard'    => 'Standard',
	'nostalgia'   => 'Nostalgi',
	'cologneblue' => 'Cologne blå',
	'monobook'    => 'Monobook',
	'myskin'      => 'Mitt utseende',
	'chick'       => 'Chick',
	'simple'      => 'Enkel',
	'modern'      => 'Modern',
);

$namespaceNames = array(
	NS_MEDIA          => 'Media',
	NS_SPECIAL        => 'Special',
	NS_MAIN           => '',
	NS_TALK           => 'Diskussion',
	NS_USER           => 'Användare',
	NS_USER_TALK      => 'Användardiskussion',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK   => '$1diskussion',
	NS_IMAGE          => 'Bild',
	NS_IMAGE_TALK     => 'Bilddiskussion',
	NS_MEDIAWIKI      => 'MediaWiki',
	NS_MEDIAWIKI_TALK => 'MediaWiki-diskussion',
	NS_TEMPLATE       => 'Mall',
	NS_TEMPLATE_TALK  => 'Malldiskussion',
	NS_HELP           => 'Hjälp',
	NS_HELP_TALK      => 'Hjälpdiskussion',
	NS_CATEGORY       => 'Kategori',
	NS_CATEGORY_TALK  => 'Kategoridiskussion',
);

$namespaceAliases = array(
	// For compatibility with 1.7 and older
	'MediaWiki_diskussion' => NS_MEDIAWIKI_TALK,
	'Hjälp_diskussion'     => NS_HELP_TALK
);

$specialPageAliases = array(
	'DoubleRedirects'           => array( 'Dubbla_omdirigeringar' ),
	'BrokenRedirects'           => array( 'Dåliga_omdirigeringar' ),
	'Disambiguations'           => array( 'Förgreningssidor' ),
	'Userlogin'                 => array( 'Inloggning' ),
	'Userlogout'                => array( 'Utloggning' ),
	'CreateAccount'             => array( 'Skapa_konto' ),
	'Preferences'               => array( 'Inställningar' ),
	'Watchlist'                 => array( 'Bevakningslista', 'Övervakningslista' ),
	'Recentchanges'             => array( 'Senaste_ändringar' ),
	'Upload'                    => array( 'Uppladdning' ),
	'Imagelist'                 => array( 'Bildlista' ),
	'Newimages'                 => array( 'Nya_bilder' ),
	'Listusers'                 => array( 'Användare', 'Användarlista' ),
	'Listgrouprights'           => array( 'Grupprättigheter' ),
	'Statistics'                => array( 'Statistik' ),
	'Randompage'                => array( 'Slumpsida' ),
	'Lonelypages'               => array( 'Övergivna_sidor', 'Sidor_utan_länkar_till' ),
	'Uncategorizedpages'        => array( 'Okategoriserade_sidor' ),
	'Uncategorizedcategories'   => array( 'Okategoriserade_kategorier' ),
	'Uncategorizedimages'       => array( 'Okategoriserade_bilder' ),
	'Uncategorizedtemplates'    => array( 'Okategoriserade_mallar' ),
	'Unusedcategories'          => array( 'Oanvända_kategorier' ),
	'Unusedimages'              => array( 'Oanvända_bilder' ),
	'Wantedpages'               => array( 'Önskade_sidor', 'Trasiga_länkar' ),
	'Wantedcategories'          => array( 'Önskade_kategorier' ),
	'Missingfiles'              => array( 'Saknade_filer', 'Saknade_bilder' ),
	'Mostlinked'                => array( 'Mest_länkade_sidor' ),
	'Mostlinkedcategories'      => array( 'Största_kategorier' ),
	'Mostlinkedtemplates'       => array( 'Mest_använda_mallar' ),
	'Mostcategories'            => array( 'Flest_kategorier' ),
	'Mostimages'                => array( 'Flest_bilder' ),
	'Mostrevisions'             => array( 'Flest_versioner' ),
	'Fewestrevisions'           => array( 'Minst_versioner' ),
	'Shortpages'                => array( 'Korta_sidor' ),
	'Longpages'                 => array( 'Långa_sidor' ),
	'Newpages'                  => array( 'Nya_sidor' ),
	'Ancientpages'              => array( 'Gamla_sidor' ),
	'Deadendpages'              => array( 'Sidor_utan_länkar', 'Sidor_utan_länkar_från' ),
	'Protectedpages'            => array( 'Skyddade_sidor' ),
	'Protectedtitles'           => array( 'Skyddade_titlar' ),
	'Allpages'                  => array( 'Alla_sidor' ),
	'Prefixindex'               => array( 'Prefixindex' ),
	'Ipblocklist'               => array( 'Blockeringslista' ),
	'Specialpages'              => array( 'Specialsidor' ),
	'Contributions'             => array( 'Bidrag' ),
	'Emailuser'                 => array( 'E-mail' ),
	'Confirmemail'              => array( 'Bekräfta_epost' ),
	'Whatlinkshere'             => array( 'Länkar_hit' ),
	'Recentchangeslinked'       => array( 'Senaste_relaterade_ändringar' ),
	'Movepage'                  => array( 'Flytta' ),
	'Blockme'                   => array( 'Blockera_mig' ),
	'Booksources'               => array( 'Bokkällor' ),
	'Categories'                => array( 'Kategorier' ),
	'Export'                    => array( 'Exportera' ),
	'Version'                   => array( 'Version' ),
	'Allmessages'               => array( 'Systemmeddelanden' ),
	'Log'                       => array( 'Logg' ),
	'Blockip'                   => array( 'Blockera' ),
	'Undelete'                  => array( 'Återställ' ),
	'Import'                    => array( 'Importera' ),
	'Lockdb'                    => array( 'Lås_databasen' ),
	'Unlockdb'                  => array( 'Lås_upp_databasen' ),
	'Userrights'                => array( 'Rättigheter' ),
	'MIMEsearch'                => array( 'MIME-sökning' ),
	'FileDuplicateSearch'       => array( 'Dublettfilsökning' ),
	'Unwatchedpages'            => array( 'Obevakade_sidor' ),
	'Listredirects'             => array( 'Omdirigeringar' ),
	'Revisiondelete'            => array( 'Radera_version' ),
	'Unusedtemplates'           => array( 'Oanvända_mallar' ),
	'Randomredirect'            => array( 'Slumpomdirigering' ),
	'Mypage'                    => array( 'Min_sida' ),
	'Mytalk'                    => array( 'Min_diskussion' ),
	'Mycontributions'           => array( 'Mina_bidrag' ),
	'Listadmins'                => array( 'Administratörer' ),
	'Listbots'                  => array( 'Robotlista' ),
	'Popularpages'              => array( 'Populära_sidor' ),
	'Search'                    => array( 'Sök' ),
	'Resetpass'                 => array( 'Återställ_lösenord' ),
	'Withoutinterwiki'          => array( 'Utan_interwikilänkar' ),
	'MergeHistory'              => array( 'Slå_ihop_historik' ),
	'Filepath'                  => array( 'Filsökväg' ),
	'Invalidateemail'           => array( 'Ogiltigförklara_epost' ),
);

$magicWords = array(
	'redirect'            => array( '0', '#REDIRECT', '#OMDIRIGERING' ),
	'notoc'               => array( '0', '__NOTOC__', '__INGENINNEHÅLLSFÖRTECKNING__' ),
	'nogallery'           => array( '0', '__NOGALLERY__', '__INGETGALLERI__' ),
	'forcetoc'            => array( '0', '__FORCETOC__', '__ALLTIDINNEHÅLLSFÖRTECKNING__' ),
	'toc'                 => array( '0', '__TOC__', '__INNEHÅLLSFÖRTECKNING__' ),
	'noeditsection'       => array( '0', '__NOEDITSECTION__', '__INTEREDIGERASEKTION__' ),
	'currentmonth'        => array( '1', 'CURRENTMONTH', 'NUVARANDEMÅNAD', 'NUMÅNAD' ),
	'currentmonthname'    => array( '1', 'CURRENTMONTHNAME', 'NUVARANDEMÅNADSNAMN', 'NUMÅNADSNAMN' ),
	'currentmonthabbrev'  => array( '1', 'CURRENTMONTHABBREV', 'NUVARANDEMÅNADKORT', 'NUMÅNADKORT' ),
	'currentday'          => array( '1', 'CURRENTDAY', 'NUVARANDEDAG', 'NUDAG' ),
	'currentday2'         => array( '1', 'CURRENTDAY2', 'NUVARANDEDAG2', 'NUDAG2' ),
	'currentdayname'      => array( '1', 'CURRENTDAYNAME', 'NUVARANDEDAGSNAMN', 'NUDAGSNAMN' ),
	'currentyear'         => array( '1', 'CURRENTYEAR', 'NUVARANDEÅR', 'NUÅR' ),
	'currenttime'         => array( '1', 'CURRENTTIME', 'NUVARANDETID', 'NUTID' ),
	'currenthour'         => array( '1', 'CURRENTHOUR', 'NUVARANDETIMME', 'NUTIMME' ),
	'localmonth'          => array( '1', 'LOCALMONTH', 'LOKALMÅNAD' ),
	'localmonthname'      => array( '1', 'LOCALMONTHNAME', 'LOKALMÅNADSNAMN' ),
	'localmonthabbrev'    => array( '1', 'LOCALMONTHABBREV', 'LOKALMÅNADKORT' ),
	'localday'            => array( '1', 'LOCALDAY', 'LOKALDAG' ),
	'localday2'           => array( '1', 'LOCALDAY2', 'LOKALDAG2' ),
	'localdayname'        => array( '1', 'LOCALDAYNAME', 'LOKALDAGSNAMN' ),
	'localyear'           => array( '1', 'LOCALYEAR', 'LOKALTÅR' ),
	'localtime'           => array( '1', 'LOCALTIME', 'LOKALTID' ),
	'localhour'           => array( '1', 'LOCALHOUR', 'LOKALTIMME' ),
	'numberofpages'       => array( '1', 'NUMBEROFPAGES', 'ANTALSIDOR' ),
	'numberofarticles'    => array( '1', 'NUMBEROFARTICLES', 'ANTALARTIKLAR' ),
	'numberoffiles'       => array( '1', 'NUMBEROFFILES', 'ANTALFILER' ),
	'numberofusers'       => array( '1', 'NUMBEROFUSERS', 'ANTALANVÄNDARE' ),
	'numberofedits'       => array( '1', 'NUMBEROFEDITS', 'ANTALREDIGERINGAR' ),
	'pagename'            => array( '1', 'PAGENAME', 'SIDNAMN' ),
	'pagenamee'           => array( '1', 'PAGENAMEE', 'SIDNAMNE' ),
	'namespace'           => array( '1', 'NAMESPACE', 'NAMNRYMD' ),
	'namespacee'          => array( '1', 'NAMESPACEE', 'NAMNRYMDE' ),
	'talkspace'           => array( '1', 'TALKSPACE', 'DISKUSSIONSRYMD' ),
	'talkspacee'          => array( '1', 'TALKSPACEE', 'DISKUSSIONSRYMDE' ),
	'subjectspace'        => array( '1', 'SUBJECTSPACE', 'ARTICLESPACE', 'ARTIKELRYMD' ),
	'subjectspacee'       => array( '1', 'SUBJECTSPACEE', 'ARTICLESPACEE', 'ARTIKELRYMDE' ),
	'fullpagename'        => array( '1', 'FULLPAGENAME', 'HELASIDNAMNET' ),
	'fullpagenamee'       => array( '1', 'FULLPAGENAMEE', 'HELASIDNAMNETE' ),
	'subpagename'         => array( '1', 'SUBPAGENAME', 'UNDERSIDNAMN' ),
	'subpagenamee'        => array( '1', 'SUBPAGENAMEE', 'UNDERSIDNAMNE' ),
	'talkpagename'        => array( '1', 'TALKPAGENAME', 'DISKUSSIONSSIDNAMN' ),
	'talkpagenamee'       => array( '1', 'TALKPAGENAMEE', 'DISKUSSIONSSIDNAMNE' ),
	'subst'               => array( '0', 'SUBST:', 'BYT:' ),
	'img_thumbnail'       => array( '1', 'thumbnail', 'thumb', 'miniatyr', 'mini' ),
	'img_manualthumb'     => array( '1', 'thumbnail=$1', 'thumb=$1', 'miniatyr=$1', 'mini=$1' ),
	'img_right'           => array( '1', 'right', 'höger' ),
	'img_left'            => array( '1', 'left', 'vänster' ),
	'img_none'            => array( '1', 'none', 'ingen' ),
	'img_width'           => array( '1', '$1px' ),
	'img_framed'          => array( '1', 'framed', 'enframed', 'frame', 'inramad', 'ram' ),
	'img_frameless'       => array( '1', 'frameless', 'ramlös' ),
	'img_page'            => array( '1', 'page=$1', 'page $1', 'sida=$1', 'sida $1' ),
	'img_border'          => array( '1', 'border', 'ram' ),
	'img_sub'             => array( '1', 'sub', 'ned' ),
	'img_super'           => array( '1', 'super', 'sup', 'upp' ),
	'img_top'             => array( '1', 'top', 'topp' ),
	'img_text_top'        => array( '1', 'text-top', 'text-topp' ),
	'img_middle'          => array( '1', 'middle', 'mitten' ),
	'img_bottom'          => array( '1', 'bottom', 'botten' ),
	'img_text_bottom'     => array( '1', 'text-bottom', 'text-botten' ),
	'sitename'            => array( '1', 'SITENAME', 'SAJTNAMN', 'SITENAMN' ),
	'ns'                  => array( '0', 'NS:', 'NR:' ),
	'localurl'            => array( '0', 'LOCALURL:', 'LOKALURL:' ),
	'localurle'           => array( '0', 'LOCALURLE:', 'LOKALURLE:' ),
	'server'              => array( '0', 'SERVER' ),
	'servername'          => array( '0', 'SERVERNAME', 'SERVERNAMN' ),
	'grammar'             => array( '0', 'GRAMMAR:', 'GRAMMATIK:' ),
	'currentweek'         => array( '1', 'CURRENTWEEK', 'NUVARANDEVECKA', 'NUVECKA' ),
	'localweek'           => array( '1', 'LOCALWEEK', 'LOKALVECKA' ),
	'revisionid'          => array( '1', 'REVISIONID', 'REVISIONSID' ),
	'revisionday'         => array( '1', 'REVISIONDAY', 'REVISIONSDAG' ),
	'revisionday2'        => array( '1', 'REVISIONDAY2', 'REVISIONSDAG2' ),
	'revisionmonth'       => array( '1', 'REVISIONMONTH', 'REVISIONSMÅNAD' ),
	'revisionyear'        => array( '1', 'REVISIONYEAR', 'REVISIONSÅR' ),
	'revisiontimestamp'   => array( '1', 'REVISIONTIMESTAMP', 'REVISIONSTIDSSTÄMPEL' ),
	'plural'              => array( '0', 'PLURAL:' ),
	'lcfirst'             => array( '0', 'LCFIRST:', 'LBFÖRST:' ),
	'ucfirst'             => array( '0', 'UCFIRST', 'SBFÖRST:' ),
	'lc'                  => array( '0', 'LC:', 'LB:' ),
	'uc'                  => array( '0', 'UC:', 'SB:' ),
	'raw'                 => array( '0', 'RAW:', 'RÅ:' ),
	'displaytitle'        => array( '1', 'DISPLAYTITLE', 'VISATITEL' ),
	'newsectionlink'      => array( '1', '__NEWSECTIONLINK__', '__NYTTAVSNITTLÄNK__' ),
	'currentversion'      => array( '1', 'CURRENTVERSION', 'NUVARANDEVERSION', 'NUVERSION' ),
	'language'            => array( '0', '#LANGUAGE:', '#SPRÅK:' ),
	'contentlanguage'     => array( '1', 'CONTENTLANGUAGE', 'CONTENTLANG', 'INNEHÅLLSSPRÅK' ),
	'numberofadmins'      => array( '1', 'NUMBEROFADMINS', 'ANTALADMINS', 'ANTALADMINISTRATÖRER' ),
	'special'             => array( '0', 'special' ),
	'defaultsort'         => array( '1', 'DEFAULTSORT:', 'DEFAULTSORTKEY:', 'DEFAULTCATEGORYSORT:', 'STANDARDSORTERING:' ),
	'filepath'            => array( '0', 'FILEPATH:', 'FILSÖKVÄG:' ),
	'tag'                 => array( '0', 'tag', 'tagg' ),
	'hiddencat'           => array( '1', '__HIDDENCAT__', '__DOLDKAT__' ),
	'pagesincategory'     => array( '1', 'PAGESINCATEGORY', 'PAGESINCAT', 'SIDORIKATEGORI' ),
	'pagesize'            => array( '1', 'PAGESIZE', 'SIDSTORLEK' ),
);

$linkTrail = '/^([a-zåäöéÅÄÖÉ]+)(.*)$/sDu';
$separatorTransformTable =  array(
	',' => "\xc2\xa0", // @bug 2749
	'.' => ','
);

$dateFormats = array(
	'mdy time' => 'H.i',
	'mdy date' => 'F j, Y',
	'mdy both' => 'F j, Y "kl." H.i',

	'dmy time' => 'H.i',
	'dmy date' => 'j F Y',
	'dmy both' => 'j F Y "kl." H.i',

	'ymd time' => 'H.i',
	'ymd date' => 'Y F j',
	'ymd both' => 'Y F j "kl." H.i',
);

$messages = array(
# User preference toggles
'tog-underline'               => 'Stryk under länkar',
'tog-highlightbroken'         => 'Formatera trasiga länkar <a href="" class="new">så här</a> (alternativt: <a href="" class="internal">?</a>).',
'tog-justify'                 => 'Justera indrag',
'tog-hideminor'               => 'Visa inte mindre redigeringar i Senaste ändringar',
'tog-extendwatchlist'         => 'Utöka bevakningslistan till att visa alla ändringar',
'tog-usenewrc'                => 'Avancerad Senaste ändringar (Javascript)',
'tog-numberheadings'          => 'Numrerade rubriker',
'tog-showtoolbar'             => 'Visa verktygsrad (Javascript)',
'tog-editondblclick'          => 'Redigera sidor med dubbelklick (Javascript)',
'tog-editsection'             => 'Möjliggör styckesvis redigering, genom [redigera]-länkar',
'tog-editsectiononrightclick' => 'Styckesvis redigering via högerklick på underrubriker (Javascript)',
'tog-showtoc'                 => 'Visa innehållsförteckning (för sidor som har minst fyra rubriker)',
'tog-rememberpassword'        => 'Kom ihåg lösenordet till nästa besök',
'tog-editwidth'               => 'Full bredd på redigeringsrutan',
'tog-watchcreations'          => 'Lägg till sidor jag skapar i min bevakningslista',
'tog-watchdefault'            => 'Lägg till sidor jag redigerar i min bevakningslista',
'tog-watchmoves'              => 'Lägg till sidor jag flyttar i min bevakningslista',
'tog-watchdeletion'           => 'Lägg till sidor jag raderar i min bevakningslista',
'tog-minordefault'            => 'Markera automatiskt ändringar som mindre',
'tog-previewontop'            => 'Visa förhandsgranskningen ovanför redigeringsrutan',
'tog-previewonfirst'          => 'Visa förhandsgranskning när redigering påbörjas',
'tog-nocache'                 => 'Stäng av cachning av sidor',
'tog-enotifwatchlistpages'    => 'Skicka e-post till mig när en sida på min bevakningslista ändras',
'tog-enotifusertalkpages'     => 'Skicka e-post till mig när något händer på min diskussionssida',
'tog-enotifminoredits'        => 'Skicka mig e-post även för små redigeringar',
'tog-enotifrevealaddr'        => 'Visa min e-postadress i e-post från systemet',
'tog-shownumberswatching'     => 'Visa antalet användare som bevakar',
'tog-fancysig'                => 'Rå signatur, utan automatisk länk',
'tog-externaleditor'          => 'Använd extern texteditor som standard (avancerat, kräver speciella inställningar i din dator)',
'tog-externaldiff'            => 'Använd externt diff-verktyg (avancerat, kräver speciella inställningar i din dator)',
'tog-showjumplinks'           => 'Aktivera "hoppa till"-tillgänglighetslänkar',
'tog-uselivepreview'          => 'Använd direktuppdaterad förhandsgranskning (Javascript, på försöksstadiet)',
'tog-forceeditsummary'        => 'Påminn mig om jag inte fyller i en redigeringskommentar',
'tog-watchlisthideown'        => 'Visa inte mina redigeringar i bevakningslistan',
'tog-watchlisthidebots'       => 'Visa inte robotredigeringar i bevakningslistan',
'tog-watchlisthideminor'      => 'Visa inte mindre ändringar i bevakningslistan',
'tog-nolangconversion'        => 'Konvertera inte mellan språkvarianter',
'tog-ccmeonemails'            => 'Skicka mig kopior av epost jag skickar till andra användare',
'tog-diffonly'                => 'Visa inte sidinnehåll under diffar',
'tog-showhiddencats'          => 'Visa dolda kategorier',

'underline-always'  => 'Alltid',
'underline-never'   => 'Aldrig',
'underline-default' => 'Webbläsarens standardinställning',

'skinpreview' => '(Förhandsvisning)',

# Dates
'sunday'        => 'söndag',
'monday'        => 'måndag',
'tuesday'       => 'tisdag',
'wednesday'     => 'onsdag',
'thursday'      => 'torsdag',
'friday'        => 'fredag',
'saturday'      => 'lördag',
'sun'           => 'sön',
'mon'           => 'mån',
'tue'           => 'tis',
'wed'           => 'ons',
'thu'           => 'tor',
'fri'           => 'fre',
'sat'           => 'lör',
'january'       => 'januari',
'february'      => 'februari',
'march'         => 'mars',
'april'         => 'april',
'may_long'      => 'maj',
'june'          => 'juni',
'july'          => 'juli',
'august'        => 'augusti',
'september'     => 'september',
'october'       => 'oktober',
'november'      => 'november',
'december'      => 'december',
'january-gen'   => 'januaris',
'february-gen'  => 'februaris',
'march-gen'     => 'mars',
'april-gen'     => 'aprils',
'may-gen'       => 'majs',
'june-gen'      => 'junis',
'july-gen'      => 'julis',
'august-gen'    => 'augustis',
'september-gen' => 'septembers',
'october-gen'   => 'oktobers',
'november-gen'  => 'novembers',
'december-gen'  => 'decembers',
'jan'           => 'jan',
'feb'           => 'feb',
'mar'           => 'mar',
'apr'           => 'apr',
'may'           => 'maj',
'jun'           => 'jun',
'jul'           => 'jul',
'aug'           => 'aug',
'sep'           => 'sep',
'oct'           => 'okt',
'nov'           => 'nov',
'dec'           => 'dec',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|Kategori|Kategorier}}',
'category_header'                => 'Sidor i kategorin "$1"',
'subcategories'                  => 'Underkategorier',
'category-media-header'          => 'Media i kategorin "$1"',
'category-empty'                 => "''Den här kategorin innehåller just nu inga sidor eller filer.''",
'hidden-categories'              => '{{PLURAL:$1|Dold kategori|Dolda kategorier}}',
'hidden-category-category'       => 'Dolda kategorier', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|Denna kategori har endast följande underkategori.|Denna kategori har följande {{PLURAL:$1|underkategori|$1 underkategorier}} (av totalt $2).}}',
'category-subcat-count-limited'  => 'Denna kategori har följande {{PLURAL:$1|underkategori|$1 underkategorier}}.',
'category-article-count'         => '{{PLURAL:$2|Denna kategori innehåller endast följande sida.|Följande {{PLURAL:$1|sida|$1 sidor}} (av totalt $2) finns i denna kategori.}}',
'category-article-count-limited' => 'Följande {{PLURAL:$1|sida|$1 sidor}} finns i den här kategorin.',
'category-file-count'            => '{{PLURAL:$2|Denna kategori innehåller endast följande fil.|Följande {{PLURAL:$1|fil|$1 filer}} (av totalt $2) finns i denna kategori.}}',
'category-file-count-limited'    => 'Följande {{PLURAL:$1|fil |$1 filer}} finns i den här kategorin.',
'listingcontinuesabbrev'         => 'forts.',

'mainpagetext'      => "<big>'''MediaWiki har installerats utan problem.'''</big>",
'mainpagedocfooter' => 'Information om hur wiki-programvaran används finns i [http://meta.wikimedia.org/wiki/Help:Contents användarguiden].

== Att komma igång ==

* [http://www.mediawiki.org/wiki/Manual:Configuration_settings Lista över konfigurationsinställningar]
* [http://www.mediawiki.org/wiki/Manual:FAQ MediaWiki FAQ]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce MediaWiki release mail list]',

'about'          => 'Om',
'article'        => 'Innehållssida',
'newwindow'      => '(öppnas i ett nytt fönster)',
'cancel'         => 'Avbryt',
'qbfind'         => 'Hitta',
'qbbrowse'       => 'Bläddra igenom',
'qbedit'         => 'Redigera',
'qbpageoptions'  => 'Denna sida',
'qbpageinfo'     => 'Sidinformation',
'qbmyoptions'    => 'Mina inställningar',
'qbspecialpages' => 'Specialsidor',
'moredotdotdot'  => 'Mer...',
'mypage'         => 'Min sida',
'mytalk'         => 'Min diskussionssida',
'anontalk'       => 'Diskussionssida för denna IP-adress',
'navigation'     => 'Navigering',
'and'            => 'och',

# Metadata in edit box
'metadata_help' => 'Metadata:',

'errorpagetitle'    => 'Fel',
'returnto'          => 'Tillbaka till $1.',
'tagline'           => 'Från {{SITENAME}}',
'help'              => 'Hjälp',
'search'            => 'Sök',
'searchbutton'      => 'Sök',
'go'                => 'Gå till',
'searcharticle'     => 'Gå till',
'history'           => 'Versionshistorik',
'history_short'     => 'Historik',
'updatedmarker'     => 'uppdaterad sedan senaste besöket',
'info_short'        => 'Information',
'printableversion'  => 'Utskriftsvänlig version',
'permalink'         => 'Permanent länk',
'print'             => 'Skriv ut',
'edit'              => 'Redigera',
'create'            => 'Skapa',
'editthispage'      => 'Redigera denna sida',
'create-this-page'  => 'Skapa denna sida',
'delete'            => 'radera',
'deletethispage'    => 'Radera denna sida',
'undelete_short'    => 'Återställ {{PLURAL:$1|en version|$1 versioner}}',
'protect'           => 'Skrivskydda',
'protect_change'    => 'ändra',
'protectthispage'   => 'Skrivskydda denna sida',
'unprotect'         => 'Ta bort skrivskydd',
'unprotectthispage' => 'Ta bort skrivskyddet från den här sidan',
'newpage'           => 'Ny sida',
'talkpage'          => 'Diskutera denna sida',
'talkpagelinktext'  => 'Diskussion',
'specialpage'       => 'Specialsida',
'personaltools'     => 'Personliga verktyg',
'postcomment'       => 'Skicka en kommentar',
'articlepage'       => 'Visa innehållssida',
'talk'              => 'Diskussion',
'views'             => 'Visningar',
'toolbox'           => 'Verktygslåda',
'userpage'          => 'Visa användarsida',
'projectpage'       => 'Visa projektsida',
'imagepage'         => 'Visa mediasida',
'mediawikipage'     => 'Visa meddelandesida',
'templatepage'      => 'Visa mallsida',
'viewhelppage'      => 'Visa hjälpsida',
'categorypage'      => 'Visa kategorisida',
'viewtalkpage'      => 'Visa diskussionssida',
'otherlanguages'    => 'Andra språk',
'redirectedfrom'    => '(Omdirigerad från $1)',
'redirectpagesub'   => 'Omdirigeringssida',
'lastmodifiedat'    => 'Sidan ändrades senast den $1 kl. $2.', # $1 date, $2 time
'viewcount'         => 'Sidan har visats {{PLURAL:$1|en gång|$1 gånger}}.',
'protectedpage'     => 'Skrivskyddad sida',
'jumpto'            => 'Hoppa till:',
'jumptonavigation'  => 'navigering',
'jumptosearch'      => 'sök',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'Om {{SITENAME}}',
'aboutpage'            => 'Project:Om',
'bugreports'           => 'Felrapporter',
'bugreportspage'       => 'Project:Felrapporter',
'copyright'            => 'Innehållet är tillgängligt under $1.',
'copyrightpagename'    => '{{SITENAME}} upphovsrätt',
'copyrightpage'        => '{{ns:project}}:Upphovsrätt',
'currentevents'        => 'Aktuella händelser',
'currentevents-url'    => 'Project:Aktuella händelser',
'disclaimers'          => 'Förbehåll',
'disclaimerpage'       => 'Project:Allmänt förbehåll',
'edithelp'             => 'Redigeringshjälp',
'edithelppage'         => 'Help:Hur man redigerar en sida',
'faq'                  => 'FAQ',
'faqpage'              => 'Project:FAQ',
'helppage'             => 'Help:Innehåll',
'mainpage'             => 'Huvudsida',
'mainpage-description' => 'Huvudsida',
'policy-url'           => 'Project:Riktlinjer',
'portal'               => 'Deltagarportalen',
'portal-url'           => 'Project:Deltagarportalen',
'privacy'              => 'Integritetspolicy',
'privacypage'          => 'Project:Integritetspolicy',

'badaccess'        => 'Behörighetsfel',
'badaccess-group0' => 'Du har inte behörighet att utföra den handling du begärt.',
'badaccess-group1' => 'Den handling du har begärt kan enbart utföras av användare i gruppen $1.',
'badaccess-group2' => 'Den handling du har begärt kan enbart utföras av användare i grupperna $1.',
'badaccess-groups' => 'Den handling du har begärt kan enbart utföras av användare i grupperna $1.',

'versionrequired'     => 'Version $1 av MediaWiki krävs',
'versionrequiredtext' => 'Version $1 av MediaWiki är nödvändig för att använda denna sida. Se [[Special:Version|versionssidan]].',

'ok'                      => 'OK',
'retrievedfrom'           => 'Hämtad från "$1"',
'youhavenewmessages'      => 'Du har $1 ($2).',
'newmessageslink'         => 'nya meddelanden',
'newmessagesdifflink'     => 'senaste ändring',
'youhavenewmessagesmulti' => 'Du har nya meddelanden på $1',
'editsection'             => 'redigera',
'editold'                 => 'redigera',
'viewsourceold'           => 'visa wikitext',
'editsectionhint'         => 'Redigera avsnitt: $1',
'toc'                     => 'Innehåll',
'showtoc'                 => 'visa',
'hidetoc'                 => 'göm',
'thisisdeleted'           => 'Visa eller återställ $1?',
'viewdeleted'             => 'Visa $1?',
'restorelink'             => '{{PLURAL:$1|en raderad version|$1 raderade versioner}}',
'feedlinks'               => 'Matning:',
'feed-invalid'            => 'Ogiltig matningstyp.',
'feed-unavailable'        => 'Webbmatningar (feeds) är inte tillgängliga',
'site-rss-feed'           => '$1 RSS-matning',
'site-atom-feed'          => '$1 Atom-matning',
'page-rss-feed'           => '"$1" RSS-matning',
'page-atom-feed'          => '"$1" Atom-matning',
'red-link-title'          => '$1 (inte skriven än)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Sida',
'nstab-user'      => 'Användarsida',
'nstab-media'     => 'Media',
'nstab-special'   => 'Special',
'nstab-project'   => 'Projektsida',
'nstab-image'     => 'Fil',
'nstab-mediawiki' => 'Systemmeddelande',
'nstab-template'  => 'Mall',
'nstab-help'      => 'Hjälpsida',
'nstab-category'  => 'Kategori',

# Main script and global functions
'nosuchaction'      => 'Funktionen finns inte',
'nosuchactiontext'  => 'Den funktion som angivits i URL:en kan inte
hittas av programvaran',
'nosuchspecialpage' => 'Någon sådan specialsida finns inte',
'nospecialpagetext' => "<big>'''Du har begärt en specialsida som inte finns.'''</big>

I [[Special:SpecialPages|listan över specialsidor]] kan du se vilka specialsidor som finns.",

# General errors
'error'                => 'Fel',
'databaseerror'        => 'Databasfel',
'dberrortext'          => 'Ett syntaxfel i databasfrågan har uppstått.
Den senaste utförda databasfrågan var:
<blockquote><tt>$1</tt></blockquote>
från funktionen "<tt>$2</tt>".
MySQL returnerade felen "$3<tt>: $4</tt>".',
'dberrortextcl'        => 'Ett felaktigt utformat sökbegrepp har påträffats. Senaste sökbegrepp var: "$1"  från funktionen "$2". MySQL svarade med felmeddelandet "$3: $4"',
'noconnect'            => 'Wikin har tekniska problem, och kan inte få kontakt med databasservern.<br />
$1',
'nodb'                 => 'Kunde inte välja databasen $1',
'cachederror'          => 'Detta är en cachad kopia av den efterfrågade sidan. Det är inte säkert att den är aktuell.',
'laggedslavemode'      => '<b>Observera: det kan dröja en stund innan de senaste redigeringarna blir synliga.</b>',
'readonly'             => 'Databasen är skrivskyddad',
'enterlockreason'      => 'Ange varför sidan skrivskyddats, och ge en uppskattning av hur länge skrivskyddet bör behållas.',
'readonlytext'         => 'Databasen är tillfälligt låst för ändringar, förmodligen på grund av rutinmässigt underhåll. Efter avslutat arbete kommer den att återgå till normalläge. Den utvecklare som skrivskyddade den har angivit följande anledning: <p>$1',
'missing-article'      => 'Databasen borde ha funnit texten till en sida med namnet "$1" $2, men det gjorde den inte.

Detta fel beror oftast på en länk till en jämförelse mellan versioner (diff) eller till en gammal version av en sida som raderats.

Om inte så är fallet, kan du ha hittat en bugg i mjukvaran.
Rapportera gärna problemet till någon [[Special:ListUsers/sysop|administratör]], ange då URL:en (webbadressen).',
'missingarticle-rev'   => '(version $1)',
'missingarticle-diff'  => '(jämförelse mellan version $1 och $2)',
'readonly_lag'         => 'Databasen har automatiskt skrivskyddats medan slavdatabasservrarna synkroniseras med huvudservern.',
'internalerror'        => 'Internt fel',
'internalerror_info'   => 'Internt fel: $1',
'filecopyerror'        => 'Kunde inte kopiera filen "$1" till "$2".',
'filerenameerror'      => 'Kunde inte byta namn på filen "$1" till "$2".',
'filedeleteerror'      => 'Kunde inte radera filen "$1".',
'directorycreateerror' => 'Kunde inte skapa katalogen "$1".',
'filenotfound'         => 'Kunde inte hitta filen "$1".',
'fileexistserror'      => 'Kan inte skriva till "$1": filen finns redan',
'unexpected'           => 'Oväntat värde: "$1"="$2".',
'formerror'            => 'Fel: Kunde inte sända formulär',
'badarticleerror'      => 'Den åtgärden kan inte utföras på den här sidan.',
'cannotdelete'         => 'Det gick inte att radera sidan eller bilden, kanske för att någon annan redan raderat den.',
'badtitle'             => 'Felaktig titel',
'badtitletext'         => 'Den begärda sidtiteln är antingen ogiltig eller tom, eller så är titeln felaktigt länkad från en annan wiki.
Den kan innehålla ett eller flera tecken som inte får användas i sidtitlar.',
'perfdisabled'         => 'Denna funktion har stängts av tillfälligt, eftersom den gör databasen så långsam att ingen kan använda wikin.',
'perfcached'           => 'Sidan är hämtad ur ett cacheminne; det är inte säkert att det är den senaste versionen.',
'perfcachedts'         => 'Sidan är hämtad ur ett cacheminne och uppdaterades senast $1.',
'querypage-no-updates' => 'Uppdatering av den här sidan är inte aktiverad. Datan kommer i nuläget inte att uppdateras.',
'wrong_wfQuery_params' => 'Felaktiga parametrar för wfQuery()<br /> Funktion: $1<br /> Förfrågan: $2',
'viewsource'           => 'Visa wikitext',
'viewsourcefor'        => 'för $1',
'actionthrottled'      => 'Åtgärden stoppades',
'actionthrottledtext'  => 'Som skydd mot spam, finns det en begränsning av hur många gånger du kan utföra den här åtgärden under en viss tid. Du har överskridit den gränsen. Försök igen om några minuter.',
'protectedpagetext'    => 'Den här sidan har skrivskyddats för att förhindra redigering.',
'viewsourcetext'       => 'Du kan se och kopiera sidans wikikod:',
'protectedinterface'   => 'Denna sida innehåller text för mjukvarans gränssnitt, och är skrivskyddad för att förebygga missbruk.',
'editinginterface'     => "'''Varning:''' Du redigerar en sida som används till texten i gränssnittet. Ändringar på denna sida kommer att påverka gränssnittets utseende för alla användare.
För översättningar, använd gärna [http://translatewiki.net/wiki/Main_Page?setlang=sv Betawiki], översättningsprojektet för MediaWiki.",
'sqlhidden'            => '(gömd SQL-förfrågan)',
'cascadeprotected'     => 'Den här sidan har skyddats från redigering eftersom den inkluderas på följande {{PLURAL:$1|sida|sidor}} som skrivskyddats med "kaskaderande skydd":
$2',
'namespaceprotected'   => "Du har inte behörighet att redigera sidor i namnrymden '''$1'''.",
'customcssjsprotected' => 'Du har inte behörighet att redigera den här sidan eftersom den innehåller en annan användares personliga inställningar.',
'ns-specialprotected'  => 'Specialsidor kan inte redigeras.',
'titleprotected'       => "Den här sidtiteln har skyddats från att skapas.
[[User:$1|$1]] skyddade sidan med motiveringen ''$2''.",

# Virus scanner
'virus-badscanner'     => 'Dålig konfigurering: okänd virusskanner: <i>$1</i>',
'virus-scanfailed'     => 'skanning misslyckades (kod $1)',
'virus-unknownscanner' => 'okänt antivirusprogram:',

# Login and logout pages
'logouttitle'                => 'Användarutloggning',
'logouttext'                 => '<strong>Du är nu utloggad.</strong>

Du kan fortsätta att använda {{SITENAME}} anonymt, eller så kan du [[Special:UserLogin|logga in igen]] som samma eller som en annan användare.
Observera att det, tills du tömmer din webbläsares cache, på vissa sidor kan se ut som att du fortfarande är inloggad.',
'welcomecreation'            => '== Välkommen, $1! ==
Ditt konto har skapats.
Glöm inte att justera dina [[Special:Preferences|{{SITENAME}}-inställningar]].',
'loginpagetitle'             => 'Användarinloggning',
'yourname'                   => 'Användarnamn:',
'yourpassword'               => 'Lösenord:',
'yourpasswordagain'          => 'Upprepa lösenord',
'remembermypassword'         => 'Automatisk inloggning i framtiden.',
'yourdomainname'             => 'Din domän',
'externaldberror'            => 'Antingen inträffade autentiseringsproblem med en extern databas, eller så får du inte uppdatera ditt externa konto.',
'loginproblem'               => '<b>Det uppstod problem vid inloggningen.</b><br />Pröva igen!',
'login'                      => 'Logga in',
'nav-login-createaccount'    => 'Logga in/skapa konto',
'loginprompt'                => 'Du måste tillåta cookies för att logga in på {{SITENAME}}.',
'userlogin'                  => 'Logga in / skapa konto',
'logout'                     => 'Logga ut',
'userlogout'                 => 'Logga ut',
'notloggedin'                => 'Inte inloggad',
'nologin'                    => 'Har du inget användarkonto? $1.',
'nologinlink'                => 'Skapa ett användarkonto',
'createaccount'              => 'Skapa ett konto',
'gotaccount'                 => 'Har du redan ett användarkonto? $1.',
'gotaccountlink'             => 'Logga in',
'createaccountmail'          => 'med e-post',
'badretype'                  => 'De lösenord du uppgett överenstämmer inte med varandra.',
'userexists'                 => 'Det valda användarnamnet används redan.
Välj ett annat namn.',
'youremail'                  => 'E-post:',
'username'                   => 'Användarnamn:',
'uid'                        => 'Användar-ID:',
'prefs-memberingroups'       => '{{PLURAL:$1|Användargrupp|Användargrupper}}:',
'yourrealname'               => 'Riktigt namn:',
'yourlanguage'               => 'Språk:',
'yournick'                   => 'Signatur:',
'badsig'                     => 'Det är något fel med råsignaturen, kontrollera HTML-koden.',
'badsiglength'               => 'Signaturen är för lång.
Den får innehålla högst $1 {{PLURAL:$1|tecken|tecken}}.',
'email'                      => 'E-post',
'prefs-help-realname'        => 'Riktigt namn behöver inte anges.
Om du väljer att ange ditt riktiga namn, kommer det att användas för att tillskriva dig ditt arbete.',
'loginerror'                 => 'Inloggningsproblem',
'prefs-help-email'           => 'Att ange e-postadress är valfritt, men gör det möjligt att få ditt lösenord mejlat till dig om du glömmer det.
Du kan också välja att låta andra användare kontakta dig genom din användar- eller användardiskussionssida utan att du behöver avslöja din identitet.',
'prefs-help-email-required'  => 'E-postadress måste anges.',
'nocookiesnew'               => 'Användarkontot skapades, men du är inte inloggad.
{{SITENAME}} använder cookies för att logga in användare.
Du har cookies avaktiverade.
Aktivera dem, och logga sen in med ditt nya användarnamn och lösenord.',
'nocookieslogin'             => '{{SITENAME}} använder cookies för att logga in användare. Du har stängt av cookies i din webbläsare. Försök igen med stöd för cookies aktiverat.',
'noname'                     => 'Du har angett ett ogiltigt användarnamn.',
'loginsuccesstitle'          => 'Inloggningen lyckades',
'loginsuccess'               => "'''Du är nu inloggad på {{SITENAME}} som \"\$1\".'''",
'nosuchuser'                 => 'Det finns ingen användare med namnet "$1".
Kontrollera stavningen, eller [[Special:Userlogin/signup|skapa ett nytt konto]].',
'nosuchusershort'            => 'Det finns ingen användare som heter "<nowiki>$1</nowiki>". Kontrollera att du stavat rätt.',
'nouserspecified'            => 'Du måste ange ett användarnamn.',
'wrongpassword'              => 'Lösenordet du angav är felaktigt. Försök igen',
'wrongpasswordempty'         => 'Lösenordet som angavs var blankt. Var god försök igen.',
'passwordtooshort'           => 'Ditt lösenord är ogiltigt eller för kort.
Det måste innehålla minst {{PLURAL:$1|$1 tecken}} och det får inte vara ditt användarnamn.',
'mailmypassword'             => 'Skicka nytt lösenord',
'passwordremindertitle'      => 'Nytt temporärt lösenord från {{SITENAME}}',
'passwordremindertext'       => 'Någon (förmodligen du, från IP-adressen $1) har begärt ett nytt lösenord till {{SITENAME}} ($4). Ett tillfälligt lösenordet för användaren "$2" har skapats och det blev "$3". Om detta var vad du önskade, så behöver du nu logga in och välja ett nytt lösenord.

Om denna begäran gjordes av någon annan, eller om du har kommit på ditt lösenord,
och inte längre önskar ändra det, så kan du ignorera detta meddelande och 
fortsätta använda ditt gamla lösenord.',
'noemail'                    => 'Användaren "$1" har inte registrerat någon e-postadress.',
'passwordsent'               => 'Ett nytt lösenord har skickats till den e-postadress som användaren "$1" har registrerat. När du får meddelandet, var god logga in igen.',
'blocked-mailpassword'       => 'Din IP-adress är blockerad, därför kan den inte användas för att få ett nytt lösenord.',
'eauthentsent'               => 'Ett e-brev för bekräftelse har skickats till den e-postadress som angivits.
Innan någon annan e-post kan skickas härifrån till kontot, måste du följa instruktionerna i e-brevet för att bekräfta att kontot verkligen är ditt.',
'throttled-mailpassword'     => 'Ett nytt lösenord har redan skickats för mindre än {{PLURAL:$1|en timme|$1 timmar}} sedan.
För att förhindra missbruk skickas bara ett nytt lösenord per {{PLURAL:$1|timme|$1-timmarsperiod}}.',
'mailerror'                  => 'Fel vid skickande av e-post: $1',
'acct_creation_throttle_hit' => 'Du har redan skapat $1 användare och kan inte göra fler.',
'emailauthenticated'         => 'Din e-postadress bekräftades den $1.',
'emailnotauthenticated'      => 'Din e-postadress är ännu inte bekräftad. Ingen e-post kommer att skickas vad gäller det följande:',
'noemailprefs'               => 'Uppge en e-postadress för att dessa funktioner skall gå att använda.',
'emailconfirmlink'           => 'Bekräfta din e-postadress',
'invalidemailaddress'        => 'E-postadressen kan inte godtas då formatet verkar vara felaktigt.
Skriv in en adress med korrekt format eller töm fältet.',
'accountcreated'             => 'Användarkontot har skapats',
'accountcreatedtext'         => 'Användarkontot $1 har skapats.',
'createaccount-title'        => 'Konto skapat på {{SITENAME}}',
'createaccount-text'         => 'Någon har skapat ett konto åt din e-postadress på {{SITENAME}} ($4) med namnet "$2" och lösenordet "$3". Du bör nu logga in och ändra ditt lösenord.

Du kan ignorera detta meddelande om kontot skapats av misstag.',
'loginlanguagelabel'         => 'Språk: $1',

# Password reset dialog
'resetpass'               => 'Välj nytt lösenord',
'resetpass_announce'      => 'Du loggade in med ett temporärt lösenord. För att slutföra inloggningen måste du välja ett nytt lösenord.',
'resetpass_text'          => '<!-- här kan text läggas till -->',
'resetpass_header'        => 'Välj nytt lösenord',
'resetpass_submit'        => 'Spara lösenord och logga in',
'resetpass_success'       => 'Ditt lösenord ändrades. Du är nu inloggad.',
'resetpass_bad_temporary' => 'Ditt temporära lösenord är felaktigt. Du kanske redan har loggat in med det eller begärt att få ett nytt tillfälligt lösenord.',
'resetpass_forbidden'     => 'Lösenord kan inte ändras',
'resetpass_missing'       => 'Formulärdata saknas.',

# Edit page toolbar
'bold_sample'     => 'Fet text',
'bold_tip'        => 'Fet stil',
'italic_sample'   => 'Kursiv text',
'italic_tip'      => 'Kursiv stil',
'link_sample'     => 'länkens namn',
'link_tip'        => 'Intern länk',
'extlink_sample'  => 'http://www.example.com länkens namn',
'extlink_tip'     => 'Extern länk (kom ihåg prefixet http://)',
'headline_sample' => 'Rubriktext',
'headline_tip'    => 'Rubrik i nivå 2',
'math_sample'     => 'Skriv formeln här',
'math_tip'        => 'Matematisk formel (LaTeX)',
'nowiki_sample'   => 'Skriv in oformaterad text här',
'nowiki_tip'      => 'Strunta i wikiformatering',
'image_sample'    => 'Exempel.jpg',
'image_tip'       => 'Inbäddad fil',
'media_sample'    => 'Exempel.mp3',
'media_tip'       => 'Länk till fil',
'sig_tip'         => 'Din signatur med tidsstämpel',
'hr_tip'          => 'Horisontell linje (använd sparsamt)',

# Edit pages
'summary'                          => 'Sammanfattning',
'subject'                          => 'Rubrik',
'minoredit'                        => 'Mindre ändring (m)',
'watchthis'                        => 'Bevaka denna sida',
'savearticle'                      => 'Spara',
'preview'                          => 'Förhandsgranska',
'showpreview'                      => 'Visa förhandsgranskning',
'showlivepreview'                  => 'Automatiskt uppdaterad förhandsvisning',
'showdiff'                         => 'Visa ändringar',
'anoneditwarning'                  => "'''Varning:''' Du är inte inloggad.
Din IP-adress kommer att sparas i historiken för den här sidan.",
'missingsummary'                   => "'''Påminnelse:''' Du har inte skrivit någon redigeringskommentar. 
Om du klickar på Spara igen, kommer din redigering att sparas utan en sådan.",
'missingcommenttext'               => 'Var god och skriv in en kommentar nedan.',
'missingcommentheader'             => "'''OBS:''' Du har inte skrivit någon rubrik till den här kommentaren. Om du trycker på \"Spara\" igen, så sparas kommentaren utan någon rubrik.",
'summary-preview'                  => 'Förhandsgranskning av sammanfattning',
'subject-preview'                  => 'Rubrikförhandsgranskning',
'blockedtitle'                     => 'Användaren är blockerad',
'blockedtext'                      => "<big>'''Din IP-adress eller ditt användarnamn är blockerat.'''</big>

Blockeringen utfördes av $1 med motiveringen: ''$2''.

* Blockeringen startade $8
* Blockeringen gäller till $6.
* Blockeringen var avsedd för $7.

Du kan kontakta $1 eller någon annan av [[{{MediaWiki:Grouppage-sysop}}|administratörerna]] för att diskutera blockeringen.
Om du är inloggad och har uppgivit en e-postadress i dina [[Special:Preferences|inställningar]] så kan du använda funktionen 'skicka e-post till den här användaren', såvida du inte blivit blockerad från funktionen.

Din IP-adress är $3 och blockerings-ID är #$5.
Vänligen ange informationen ovan i alla förfrågningar som du gör i ärendet.",
'autoblockedtext'                  => 'Din IP-adress har blockerats automatiskt eftersom den har använts av en annan användare som blockerats av $1.
Motiveringen av blockeringen var:

:\'\'$2\'\'

* Blockeringen startade $8
* Blockeringen gäller till $6
* Blockeringen är avsedd för $7

Du kan kontakta $1 eller någon annan [[{{MediaWiki:Grouppage-sysop}}|administratör]] för att diskutera blockeringen.

Observera att du inte kan använda dig av funktionen "skicka e-post till användare" om du inte har registrerat en giltig e-postadress i [[Special:Preferences|dina inställningar]] eller om du har blivit blockerad från att skicka e-post.

Din nuvarande IP-adress är $3, och blockerings-ID är #$5.
Vänligen ange informationen ovan i alla förfrågningar som du gör i ärendet.',
'blockednoreason'                  => 'ingen motivering angavs',
'blockedoriginalsource'            => "Källkoden för '''$1''' visas nedan:",
'blockededitsource'                => "Texten för '''dina ändringar''' av '''$1''' visas nedanför:",
'whitelistedittitle'               => 'Du måste logga in för att redigera',
'whitelistedittext'                => 'Du måste $1 för att kunna redigera sidor.',
'confirmedittitle'                 => 'E-postbekräftelse krävs för redigering',
'confirmedittext'                  => 'Du måste bekräfta din e-postadress innan du kan redigera sidor. Var vänlig ställ in och validera din e-postadress genom dina [[Special:Preferences|användarinställningar]].',
'nosuchsectiontitle'               => 'Avsnittet finns inte',
'nosuchsectiontext'                => 'Du försökte redigera ett avsnitt som inte finns. Eftersom avsnitt $1 inte finns, så kan inte din redigering sparas.',
'loginreqtitle'                    => 'Inloggning krävs',
'loginreqlink'                     => 'logga in',
'loginreqpagetext'                 => 'Du måste $1 för att visa andra sidor.',
'accmailtitle'                     => 'Lösenordet är skickat.',
'accmailtext'                      => "Lösenordet för '$1' har skickats till $2.",
'newarticle'                       => '(Ny)',
'newarticletext'                   => 'Du har klickat på en länk till en sida som inte finns ännu. Du kan själv skapa sidan genom att skriva i fältet nedan (du kan läsa mer på [[{{MediaWiki:Helppage}}|hjälpsidan]]). Om du inte vill skriva något kan du bara trycka på "tillbaka" i din webbläsare.',
'anontalkpagetext'                 => "---- ''Detta är en diskussionssida för en användare som inte har loggat in.
Därför måste personens numeriska IP-adress användas för att identifiera honom eller henne.
En sådan IP-adress kan ibland användas av flera olika personer.
Om du får meddelanden här som inte tycks vara riktade till dig, kan du gärna [[Special:UserLogin/signup|skapa ett konto]] eller [[Special:UserLogin|logga in]]. Då undviker du framtida förväxlingar.''",
'noarticletext'                    => 'Det finns just nu ingen text på denna sida. Du kan [[Special:Search/{{PAGENAME}}|söka efter denna sidtitel]] i andra sidor eller [{{fullurl:{{FULLPAGENAME}}|action=edit}} redigera denna sida].',
'userpage-userdoesnotexist'        => '"$1" är inte ett registrerat användarkonto. Tänk efter om du vill skapa/redigera den här sidan.',
'clearyourcache'                   => "'''Observera: Sedan du sparat sidan kan du behöva tömma din webbläsares cache för att se ändringarna.''' '''Mozilla/Firefox/Safari:''' håll ner ''Skift'' och klicka på ''Reload'' eller tryck antingen ''Ctrl-F5'' eller ''Ctrl-R'' (''Command-R'' på Macintosh); '''Konqueror:''': klicka ''Reload'' eller tryck ''F5;'' '''Opera:''' rensa cachen i ''Tools → Preferences;'' '''Internet Explorer:'''  håll ner ''Ctrl'' och klicka på ''Refresh'' eller tryck ''Ctrl-F5.''",
'usercssjsyoucanpreview'           => "<strong>Tips:</strong> Använd 'Visa förhandsgranskning' för att testa din nya css/js innan du sparar.",
'usercsspreview'                   => "'''Kom ihåg att du bara förhandsgranskar din användar-CSS.
Den har inte sparats än!'''",
'userjspreview'                    => "'''Kom ihåg att du bara testar/förhandsgranskar ditt JavaScript, det har inte sparats än!'''",
'userinvalidcssjstitle'            => "'''Varning:''' Skalet \"\$1\" finns inte. Kom ihåg att .css- och .js-sidor för enskilda användare börjar på liten bokstav. Exempel: {{ns:user}}:Foo/monobook.css i stället för {{ns:user}}:Foo/Monobook.css.",
'updated'                          => '(Uppdaterad)',
'note'                             => '<strong>Obs!</strong>',
'previewnote'                      => '<strong>Detta är bara en förhandsvisning;
ändringar har ännu inte sparats!</strong>',
'previewconflict'                  => 'Den här förhandsvisningen är resultatet av den
redigerbara texten ovanför,
så som det kommer att se ut om du väljer att spara.',
'session_fail_preview'             => '<strong>Vi kunde inte behandla din redigering eftersom sessionsdata gått förlorad.
Var god försök igen.
Om det fortfarande inte fungerar, pröva att [[Special:UserLogout|logga ut]] och logga in igen.</strong>',
'session_fail_preview_html'        => "<strong>Vi kunde inte behandla din redigering eftersom sessionsdata gått förlorad.</strong>

''Eftersom {{SITENAME}} har aktiverat rå HTML, så döljs förhandsvisningen som en förebyggande säkerhetsåtgärd mot JavaScript-attacker.''

<strong>Om detta är ett försök att göra en rättmätig redigering, så försök igen.
Om det fortfarande inte fungerar, pröva att [[Special:UserLogout|logga ut]] och logga in igen.</strong>",
'token_suffix_mismatch'            => '<strong>Din redigering har stoppats eftersom din klient har ändrat tecknen
i redigeringens "edit token". Redigeringen stoppades för att förhindra att sidtexten skadas.
Detta händer ibland om du använder buggiga webbaserade anonyma proxytjänster.</strong>',
'editing'                          => 'Redigerar $1',
'editingsection'                   => 'Redigerar $1 (avsnitt)',
'editingcomment'                   => 'Redigerar $1 (kommentar)',
'editconflict'                     => 'Redigeringskonflikt: $1',
'explainconflict'                  => "Någon annan har ändrat den här sidan efter att du började att redigera den.
Den översta textrutan innehåller den nuvarande sparade versionen av texten.
Din ändrade version visas i den nedre rutan.
Om du vill spara dina ändringar så måste du infoga dem i den övre texten.
'''Endast''' texten i den översta textrutan kommer att sparas när du trycker på \"Spara\".",
'yourtext'                         => 'Din text',
'storedversion'                    => 'Den sparade versionen',
'nonunicodebrowser'                => '<strong>VARNING: Din webbläsare saknar stöd för unicode. För att du ska kunna redigera sidor utan problem, så visas icke-ASCII-tecken som hexadecimala koder i redigeringsrutan.</strong>',
'editingold'                       => '<strong>VARNING: Du redigerar en gammal version av denna sida. Om du sparar den kommer alla ändringar som har gjorts sedan denna version att skrivas över.</strong>',
'yourdiff'                         => 'Skillnader',
'copyrightwarning'                 => 'Observera att alla bidrag till {{SITENAME}} är att betrakta som utgivna under $2 (se $1 för detaljer). Om du inte vill att din text ska redigeras eller kopieras efter andras gottfinnande skall du inte skriva något här.<br />
Du lovar oss också att du skrev texten själv, eller kopierade från kulturellt allmängods som inte skyddas av upphovsrätt, eller liknande källor. <strong>LÄGG INTE UT UPPHOVSRÄTTSSKYDDAT MATERIAL HÄR UTAN TILLÅTELSE!</strong>',
'copyrightwarning2'                => 'Observera att alla bidrag till {{SITENAME}} kan komma att redigeras, ändras, eller tas bort av andra deltagare. Om du inte vill se din text förändrad efter andras gottfinnade skall du inte skriva in någon text här.<br />
Du lovar oss också att du skrev texten själv, eller kopierade från kulturellt allmängods som inte skyddas av upphovsrätt, eller liknande källor - se $1 för detaljer.
<strong>LÄGG INTE UT UPPHOVSRÄTTSSKYDDAT MATERIAL HÄR UTAN TILLÅTELSE!</strong>',
'longpagewarning'                  => '<strong>VARNING: Den här sidan är $1 kilobyte lång;
vissa webbläsare kan ha problem att redigera sidor som närmar sig eller är större än 32 kB.
Överväg att bryta upp sidan i mindre delar.</strong>',
'longpageerror'                    => '<strong>FEL: Texten som du försöker spara är $1 kilobyte, vilket är mer än det maximalt tillåtna $2 kilobyte. Den kan inte sparas.</strong>',
'readonlywarning'                  => '<strong>VARNING: Databasen är tillfälligt låst för underhåll. Du kommer inte att kunna spara
dina ändringar just nu. Det kan vara klokt att kopiera över texten till din egen dator, tills databasen är upplåst igen.</strong>',
'protectedpagewarning'             => '<strong>VARNING: Den här sidan är låst så att bara administratörer kan redigera den.
Försäkra dig om att du följer riktlinjerna för redigering av skyddade sidor.</strong>',
'semiprotectedpagewarning'         => "'''Observera:''' Denna sida har delvis skrivskyddats, så att endast registrerade användare kan redigera den.",
'cascadeprotectedwarning'          => '<strong>VARNING:</strong> Den här sidan är låst så att bara administratörer kan redigera den. Det beror på att sidan inkluderas på följande {{PLURAL:$1|sida|sidor}} som skyddats med "kaskaderande skrivskydd":',
'titleprotectedwarning'            => '<strong>VARNING: Den här sidan har skyddats så att endast vissa användare kan skapa den.</strong>',
'templatesused'                    => 'Mallar som används på den här sidan:',
'templatesusedpreview'             => 'Mallar som används i förhandsgranskningen:',
'templatesusedsection'             => 'Mallar som används i det här avsnittet:',
'template-protected'               => '(skyddad)',
'template-semiprotected'           => '(delvis skyddad)',
'hiddencategories'                 => 'Denna sida är medlem i följande dolda {{PLURAL:$1|kategori|kategorier}}:',
'edittools'                        => '<!-- Denna text kommer att visas nedanför redigeringsrutor och uppladdningsformulär. -->',
'nocreatetitle'                    => 'Skapande av sidor begränsat',
'nocreatetext'                     => '{{SITENAME}} har begränsat möjligheterna att skapa nya sidor.
Du kan redigera existerande sidor, eller [[Special:UserLogin|logga in eller skapa ett användarkonto]].',
'nocreate-loggedin'                => 'Du har inte behörighet att skapa nya sidor.',
'permissionserrors'                => 'Behörighetsfel',
'permissionserrorstext'            => 'Du har inte behörighet att göra det du försöker göra, av följande {{PLURAL:$1|anledning|anledningar}}:',
'permissionserrorstext-withaction' => 'Du har inte behörighet att $2, av följande {{PLURAL:$1|anledning|anledningar}}:',
'recreate-deleted-warn'            => "'''Varning: Den sida du skapar har tidigare raderats.'''

Du bör överväga om det är lämpligt att fortsätta redigera sidan.
Raderingsloggen för sidan innehåller följande:",

# Parser/template warnings
'expensive-parserfunction-warning'        => 'Varning: Denna sida innehåller för många anrop av resurskrävande parserfunktioner.

Antalet anrop borde vara mindre än $2, det är nu $1.',
'expensive-parserfunction-category'       => 'Sidor med för många resurskrävande parserfunktioner',
'post-expand-template-inclusion-warning'  => 'Varning: Den här sidan innehåller för mycket mallinklusioner.
Några av mallarna kommer inte att inkluderas.',
'post-expand-template-inclusion-category' => 'Sidor som inkluderar för mycket mallar',
'post-expand-template-argument-warning'   => 'Varning: Sidan innehåller en eller flera mallparametrar som blir för långa när de expanderas.
Dessa parametrar har uteslutits.',
'post-expand-template-argument-category'  => 'Sidor med uteslutna mallparametrar',

# "Undo" feature
'undo-success' => 'Sidan kan återställas till tidigare version. Var god och kontrollera jämförelsen nedan för att bekräfta att detta är vad du avser att göra och slutför återställningen genom att spara.',
'undo-failure' => 'Ändringen kunde inte avlägsnas på grund av motstridande ändringar som gjorts sedan dess.',
'undo-norev'   => 'Ändringen kan inte avlägsnas eftersom den inte finns eller har raderats.',
'undo-summary' => 'Tar bort version $1 av [[Special:Contributions/$2|$2]] ([[User talk:$2|diskussion]])',

# Account creation failure
'cantcreateaccounttitle' => 'Kan inte skapa konto',
'cantcreateaccount-text' => '[[User:$3|$3]] har blockerat den här IP-adressen (\'\'\'$1\'\'\') från att registrera konton.

Anledningen till blockeringen var "$2".',

# History pages
'viewpagelogs'        => 'Visa loggar för denna sida',
'nohistory'           => 'Den här sidan har ingen versionshistorik.',
'revnotfound'         => 'Versionen hittades inte',
'revnotfoundtext'     => 'Den gamla versionen av den sida du frågade efter kan inte hittas. Kontrollera den URL du använde för att nå den här sidan.',
'currentrev'          => 'Nuvarande version',
'revisionasof'        => 'Versionen från $1',
'revision-info'       => 'Version från den $1 av $2',
'previousrevision'    => '← Äldre version',
'nextrevision'        => 'Nyare version →',
'currentrevisionlink' => 'Nuvarande version',
'cur'                 => 'nuvarande',
'next'                => 'nästa',
'last'                => 'föregående',
'page_first'          => 'första',
'page_last'           => 'sista',
'histlegend'          => "Val av diff: markera i klickrutorna för att jämföra versioner och tryck enter eller knappen längst ner.<br />
Förklaring: (nuvarande) = skillnad mot nuvarande version; (föregående) = skillnad mot föregående version; '''m''' = mindre ändring.",
'deletedrev'          => '[raderad]',
'histfirst'           => 'Första',
'histlast'            => 'Senaste',
'historysize'         => '($1 byte)',
'historyempty'        => '(tom)',

# Revision feed
'history-feed-title'          => 'Versionshistorik',
'history-feed-description'    => 'Versionshistorik för denna sida på wikin',
'history-feed-item-nocomment' => '$1 den $2', # user at time
'history-feed-empty'          => 'Den begärda sidan finns inte.
Den kan ha tagits bort från wikin eller bytt namn.
Prova att [[Special:Search|söka på wikin]] för relevanta nya sidor.',

# Revision deletion
'rev-deleted-comment'         => '(kommentar borttagen)',
'rev-deleted-user'            => '(användarnamn borttaget)',
'rev-deleted-event'           => '(loggåtgärd borttagen)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">
Denna version av sidan har avlägsnats från de öppna arkiven.
Det kan finnas mer information i [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} borttagningsloggen].
</div>',
'rev-deleted-text-view'       => '<div class="mw-warning plainlinks"> Denna version av sidan har avlägsnats från de öppna arkiven. Som administratör på denna wiki kan du se den. Det kan finnas mer information i [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} borttagningsloggen]. </div>',
'rev-delundel'                => 'visa/göm',
'revisiondelete'              => 'Ta bort/återställ versioner',
'revdelete-nooldid-title'     => 'Ogiltig målversion',
'revdelete-nooldid-text'      => 'Antingen har du inte angivit någon sidversion att utföra funktionen på,
eller så finns inte den version du angav,
eller så försöker du gömma den senaste versionen av sidan.',
'revdelete-selected'          => '{{PLURAL:$2|Vald version|Valda versioner}} av [[:$1]]:',
'logdelete-selected'          => '{{PLURAL:$1|Vald loggåtgärd|Valda loggåtgärder}}:',
'revdelete-text'              => 'Borttagna versioner och åtgärder kommer fortfarande att synas i historiken och i loggar, men deras innehåll kommer ej att vara tillgängligt för allmänheten.

Andra administratörer på {{SITENAME}} kommer fortfarande att kunna läsa det dolda innehållet och kan återställa sidan genom samma gränssnitt, om inte en ytterligare begränsningar finns.',
'revdelete-legend'            => 'Ändra synlighet',
'revdelete-hide-text'         => 'Dölj versionstext',
'revdelete-hide-name'         => 'Dölj åtgärd och sidnamn',
'revdelete-hide-comment'      => 'Dölj redigeringskommentar',
'revdelete-hide-user'         => 'Dölj redaktörens användarnamn/IP-adress',
'revdelete-hide-restricted'   => 'Låt dessa begränsningar gälla även för administratörer och lås det här gränssnittet',
'revdelete-suppress'          => 'Undanhåll data även från administratörer',
'revdelete-hide-image'        => 'Dölj filinnehåll',
'revdelete-unsuppress'        => 'Ta bort begränsningar på återställda versioner',
'revdelete-log'               => 'Kommentar:',
'revdelete-submit'            => 'Tillämpa på vald version',
'revdelete-logentry'          => 'ändrade synlighet för versioner av [[$1]]',
'logdelete-logentry'          => 'ändrade synlighet för åtgärder i [[$1]]',
'revdelete-success'           => "'''Versionens synlighet har ändrats.'''",
'logdelete-success'           => "'''Loggåtgärdens synlighet har ändrats.'''",
'revdel-restore'              => 'Ändra synlighet',
'pagehist'                    => 'Sidhistorik',
'deletedhist'                 => 'Raderad historik',
'revdelete-content'           => 'innehåll',
'revdelete-summary'           => 'sammanfattning',
'revdelete-uname'             => 'användarnamn',
'revdelete-restricted'        => 'satte begränsningar för administratörer',
'revdelete-unrestricted'      => 'tog bort begränsningar för administratörer',
'revdelete-hid'               => 'dolde $1',
'revdelete-unhid'             => 'synliggjorde $1',
'revdelete-log-message'       => '$1 för $2 {{PLURAL:$2|sidversion|sidversioner}}',
'logdelete-log-message'       => '$1 för $2 {{PLURAL:$2|åtgärd|åtgärder}}',

# Suppression log
'suppressionlog'     => 'Versionsraderingslogg',
'suppressionlogtext' => 'Nedan visas en lista över raderingar och blockeringar som berör innehåll dolt för administratörer.
Se [[Special:IPBlockList|blockeringslistan]] för listan över gällande blockeringar.',

# History merging
'mergehistory'                     => 'Sammanfoga sidhistoriker',
'mergehistory-header'              => 'Med den här specialsidan kan du infoga versioner av en sida i en nyare sidas historik.
Se till att sidhistorikens kontinuitet behålls när du sammanfogar historik.',
'mergehistory-box'                 => 'Sammanfoga versioner av följande två sidor:',
'mergehistory-from'                => 'Källsida:',
'mergehistory-into'                => 'Målsida:',
'mergehistory-list'                => 'Sidhistorik som kan sammanfogas',
'mergehistory-merge'               => 'Följande versioner av [[:$1]] kan infogas i [[:$2]]. Med hjälp av alternativknapparna för varje version kan du välja att endast infoga versioner fram till en viss tidpunkt. Notera att om du använder navigationslänkarna så avmarkeras alla alternativknappar.',
'mergehistory-go'                  => 'Visa versioner som kan infogas',
'mergehistory-submit'              => 'Sammanfoga',
'mergehistory-empty'               => 'Inga versioner av sidorna kan sammanfogas.',
'mergehistory-success'             => '$3 {{PLURAL:$3|version|versioner}} av [[:$1]] har infogats i [[:$2]].',
'mergehistory-fail'                => 'Historikerna kunde inte sammanfogas, kontrollera de sidor och den sidversion som du valt.',
'mergehistory-no-source'           => 'Källsidan $1 finns inte.',
'mergehistory-no-destination'      => 'Målsidan $1 finns inte.',
'mergehistory-invalid-source'      => 'Källsidan måste vara en giltig sidtitel.',
'mergehistory-invalid-destination' => 'Målsidan måste vara en giltig sidtitel.',
'mergehistory-autocomment'         => 'Infogade [[:$1]] i [[:$2]]',
'mergehistory-comment'             => 'Infogade [[:$1]] i [[:$2]]: $3',

# Merge log
'mergelog'           => 'Sammanfogningslogg',
'pagemerge-logentry' => 'infogade [[$1]] i [[$2]] (versioner t.o.m. $3)',
'revertmerge'        => 'Återställ infogning',
'mergelogpagetext'   => 'Detta är en lista över de senaste sammansfogningarna av sidhistoriker.',

# Diffs
'history-title'           => 'Versionshistorik för "$1"',
'difference'              => '(Skillnad mellan versioner)',
'lineno'                  => 'Rad $1:',
'compareselectedversions' => 'Jämför angivna versioner',
'editundo'                => 'avlägsna',
'diff-multi'              => '({{PLURAL:$1|En mellanliggande version|$1 mellanliggande versioner}} visas inte.)',

# Search results
'searchresults'             => 'Sökresultat',
'searchresulttext'          => 'Se [[{{MediaWiki:Helppage}}|hjälpsidan]] för mer information om sökning på {{SITENAME}}.',
'searchsubtitle'            => 'Du sökte efter \'\'\'[[:$1]]\'\'\' ([[Special:Prefixindex/$1|alla sidor som startar med "$1"]] | [[Special:WhatLinksHere/$1|alla sidor som länkar till "$1"]])',
'searchsubtitleinvalid'     => 'För sökbegreppet $1',
'noexactmatch'              => "'''Det finns ingen sida med titeln \"\$1\".''' Du kan  [[:\$1|skapa denna sida]].",
'noexactmatch-nocreate'     => "'''Det finns ingen sida med titeln \"\$1\".'''",
'toomanymatches'            => 'Sökningen gav för många resultat, försök med en annan fråga',
'titlematches'              => 'Träffar i sidtitlar',
'notitlematches'            => 'Det finns ingen sida vars titel överensstämmer med sökordet.',
'textmatches'               => 'Sidor som innehåller sökordet:',
'notextmatches'             => 'Det finns inga sidor som innehåller sökordet',
'prevn'                     => 'förra $1',
'nextn'                     => 'nästa $1',
'viewprevnext'              => 'Visa ($1) ($2) ($3)',
'search-result-size'        => '$1 ({{PLURAL:$2|1 ord|$2 ord}})',
'search-result-score'       => 'Relevans: $1%',
'search-redirect'           => '(omdirigering $1)',
'search-section'            => '(avsnitt $1)',
'search-suggest'            => 'Menade du: $1',
'search-interwiki-caption'  => 'Systerprojekt',
'search-interwiki-default'  => '$1 resultat:',
'search-interwiki-more'     => '(mer)',
'search-mwsuggest-enabled'  => 'med förslag',
'search-mwsuggest-disabled' => 'inga förslag',
'search-relatedarticle'     => 'Relaterad',
'mwsuggest-disable'         => 'Avaktivera AJAX-förslag',
'searchrelated'             => 'relaterad',
'searchall'                 => 'alla',
'showingresults'            => "Nedan visas upp till {{PLURAL:$1|'''1''' post|'''$1''' poster}} från och med nummer '''$2'''.",
'showingresultsnum'         => "Nedan visas {{PLURAL:$3|'''1''' post|'''$3''' poster}} från och med nummer '''$2'''.",
'showingresultstotal'       => "Härunder visas resultat {{PLURAL:$3|'''$1'''|'''$1 - $2'''}} av '''$3'''",
'nonefound'                 => "'''Observera:''' Som standard sker sökning endast i vissa namnrymder. Du kan pröva att skriva ''all:'' i början av din sökning om du vill söka i alla sidor (inklusive diskussionssidor, mallar, m.m.), eller så kan du att börja din sökning med namnet på den namnrymd du vill söka i.",
'powersearch'               => 'Sök',
'powersearch-legend'        => 'Avancerad sökning',
'powersearch-ns'            => 'Sök i namnrymderna:',
'powersearch-redir'         => 'Visa omdirigeringar',
'powersearch-field'         => 'Sök efter',
'search-external'           => 'Extern sökning',
'searchdisabled'            => 'Sökfunktionen på {{SITENAME}} är avstängd.
Du kan istället göra sökningar med hjälp av Google.
Notera dock att deras indexering av {{SITENAME}} kan vara något föråldrad.',

# Preferences page
'preferences'              => 'Inställningar',
'mypreferences'            => 'Mina inställningar',
'prefs-edits'              => 'Antal redigeringar:',
'prefsnologin'             => 'Inte inloggad',
'prefsnologintext'         => 'Du måste vara <span class="plainlinks">[{{fullurl:Special:Userlogin|returnto=$1}} inloggad]</span> för att kunna ändra dina inställningar.',
'prefsreset'               => 'Inställningarna har återställts till ursprungsvärdena.',
'qbsettings'               => 'Inställningar för snabbmeny',
'qbsettings-none'          => 'Ingen',
'qbsettings-fixedleft'     => 'Fast vänster',
'qbsettings-fixedright'    => 'Fast höger',
'qbsettings-floatingleft'  => 'Flytande vänster',
'qbsettings-floatingright' => 'Flytande höger',
'changepassword'           => 'Byt lösenord',
'skin'                     => 'Utseende',
'math'                     => 'Matematik',
'dateformat'               => 'Datumformat',
'datedefault'              => 'Ovidkommande',
'datetime'                 => 'Datum och tid',
'math_failure'             => 'Misslyckades med att tolka formel.',
'math_unknown_error'       => 'okänt fel',
'math_unknown_function'    => 'okänd funktion',
'math_lexing_error'        => 'regelfel',
'math_syntax_error'        => 'syntaxfel',
'math_image_error'         => 'Konvertering till PNG-format misslyckades; kontrollera om latex, dvips, gs och convert är korrekt installerade',
'math_bad_tmpdir'          => 'Kan inte skriva till eller skapa temporär mapp för matematikresultat',
'math_bad_output'          => 'Kan inte skriva till eller skapa mapp för matematikresultat',
'math_notexvc'             => 'Applicationen texvc saknas; läs math/README för konfigureringsanvisningar.',
'prefs-personal'           => 'Mitt konto',
'prefs-rc'                 => 'Senaste ändringar',
'prefs-watchlist'          => 'Bevakningslista',
'prefs-watchlist-days'     => 'Antal dagar som visas i bevakningslistan:',
'prefs-watchlist-edits'    => 'Maximalt antal redigeringar som visas i utökad bevakningslista:',
'prefs-misc'               => 'Diverse',
'saveprefs'                => 'Spara inställningar',
'resetprefs'               => 'Återställ osparade ändringar',
'oldpassword'              => 'Gammalt lösenord:',
'newpassword'              => 'Nytt lösenord:',
'retypenew'                => 'Upprepa det nya lösenordet:',
'textboxsize'              => 'Redigering',
'rows'                     => 'Rader:',
'columns'                  => 'Kolumner:',
'searchresultshead'        => 'Sökresultat',
'resultsperpage'           => 'Träffar per sida:',
'contextlines'             => 'Antal rader per träff:',
'contextchars'             => 'Tecken per rad:',
'stub-threshold'           => 'Formatera länkar <a href="#" class="stub">så här</a> till sidor som är kortare än:',
'recentchangesdays'        => 'Antal dagar i "senaste ändringarna":',
'recentchangescount'       => 'Antal ändringar som visas i "senaste ändringarna", sidhistoriker och loggsidor:',
'savedprefs'               => 'Dina inställningar har sparats',
'timezonelegend'           => 'Tidszon',
'timezonetext'             => 'Ange skillnaden i timmar mellan din lokala tid och serverns tid (UTC).',
'localtime'                => 'Lokal tid',
'timezoneoffset'           => 'Utjämna',
'servertime'               => 'Serverns klocka är',
'guesstimezone'            => 'Fyll i från webbläsare',
'allowemail'               => 'Tillåt e-post från andra användare',
'prefs-searchoptions'      => 'Sökalternativ',
'prefs-namespaces'         => 'Namnrymder',
'defaultns'                => 'Sök i följande namnrymder som förval:',
'default'                  => 'ursprungsinställning',
'files'                    => 'Filer',

# User rights
'userrights'                  => 'Användarrättigheter', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => 'Hantera användargrupper',
'userrights-user-editname'    => 'Skriv in ett användarnamn:',
'editusergroup'               => 'Ändra användargrupper',
'editinguser'                 => "Ändrar rättigheter för användaren '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup'    => 'Ändra användargrupper',
'saveusergroups'              => 'Spara användargrupper',
'userrights-groupsmember'     => 'Medlem i:',
'userrights-groups-help'      => 'Du kan ändra vilka grupper denna användare är medlem i.
* En ikryssad ruta betyder användaren är medlem i den gruppen.
* En okryssad ruta betyder att användaren inte är medlem i den gruppen.
* En asterisk (*) markerar att du inte kan ta bort gruppen när du har lagt till den, eller vice versa.',
'userrights-reason'           => 'Anledning till ändringen:',
'userrights-no-interwiki'     => 'Du har inte behörighet att ändra användarrättigheter på andra wikis.',
'userrights-nodatabase'       => 'Databasen $1 finns inte eller så är den inte lokal.',
'userrights-nologin'          => 'Du måste [[Special:UserLogin|logga in]] med ett administratörskonto för att ändra användarrättigheter.',
'userrights-notallowed'       => 'Ditt konto har inte behörighet till att ändra användarrättigheter.',
'userrights-changeable-col'   => 'Grupper du kan ändra',
'userrights-unchangeable-col' => 'Grupper du inte kan ändra',

# Groups
'group'               => 'Grupp:',
'group-user'          => 'Användare',
'group-autoconfirmed' => 'Bekräftade användare',
'group-bot'           => 'Robotar',
'group-sysop'         => 'Administratörer',
'group-bureaucrat'    => 'Byråkrater',
'group-suppress'      => 'Versionsraderare',
'group-all'           => '(alla)',

'group-user-member'          => 'användare',
'group-autoconfirmed-member' => 'bekräftad användare',
'group-bot-member'           => 'robot',
'group-sysop-member'         => 'administratör',
'group-bureaucrat-member'    => 'byråkrat',
'group-suppress-member'      => 'versionsraderare',

'grouppage-user'          => '{{ns:project}}:Användare',
'grouppage-autoconfirmed' => '{{ns:project}}:Bekräftade användare',
'grouppage-bot'           => '{{ns:project}}:Robotar',
'grouppage-sysop'         => '{{ns:project}}:Administratörer',
'grouppage-bureaucrat'    => '{{ns:project}}:Byråkrater',
'grouppage-suppress'      => '{{ns:project}}:Oversight',

# Rights
'right-read'                 => 'Se sidor',
'right-edit'                 => 'Redigera sidor',
'right-createpage'           => 'Skapa sidor (som inte är diskussionssidor)',
'right-createtalk'           => 'Skapa diskussionssidor',
'right-createaccount'        => 'Skapa nya användarkonton',
'right-minoredit'            => 'Markera mindre ändringar',
'right-move'                 => 'Flytta sidor',
'right-move-subpages'        => 'Flytta sidor med deras undersidor',
'right-suppressredirect'     => 'Behöver inte skapa omdirigeringar vid sidflyttning',
'right-upload'               => 'Ladda upp filer',
'right-reupload'             => 'Skriva över existerande filer',
'right-reupload-own'         => 'Skriva över egna filer',
'right-reupload-shared'      => 'Skriva över delade filer lokalt',
'right-upload_by_url'        => 'Ladda upp en fil genom en URL',
'right-purge'                => 'Rensa cachen för sidor utan att behöva bekräfta',
'right-autoconfirmed'        => 'Redigera halvlåsta sidor',
'right-bot'                  => 'Bli behandlad som en automatisk process',
'right-nominornewtalk'       => 'Får inte meddelanden om nya ändringar på diskussionssidan vid mindre ändringar.',
'right-apihighlimits'        => 'Använda högre gränser i API-frågor',
'right-writeapi'             => 'Redigera via API:t',
'right-delete'               => 'Radera sidor',
'right-bigdelete'            => 'Radera sidor med stor historik',
'right-deleterevision'       => 'Radera och återställa enskilda sidversioner',
'right-deletedhistory'       => 'Se raderad historik utan tillhörande sidtext',
'right-browsearchive'        => 'Söka efter raderade sidor',
'right-undelete'             => 'Återställa raderade sidor',
'right-suppressrevision'     => 'Se och återställa sidversioner som dolts för administratörer',
'right-suppressionlog'       => 'Se privata loggar',
'right-block'                => 'Blockera andra användare från att redigera',
'right-blockemail'           => 'Blockera användare från att skicka e-post',
'right-hideuser'             => 'Dölj ett användarnamn från det offentliga',
'right-ipblock-exempt'       => 'Kan redigera från blockerade IP-adresser',
'right-proxyunbannable'      => 'Kan redigera från blockerade proxyer',
'right-protect'              => 'Ändra skyddsnivåer och redigera skyddade sidor',
'right-editprotected'        => 'Redigera skyddade sidor',
'right-editinterface'        => 'Redigera användargränssnittet',
'right-editusercssjs'        => 'Redigera andra användares CSS- och JS-filer',
'right-rollback'             => 'Rulla tillbaka den användare som senast redigerat en sida',
'right-markbotedits'         => 'Markera tillbakarullningar som robotändringar',
'right-noratelimit'          => 'Påverkas inte av hastighetsgränser',
'right-import'               => 'Importera sidor från andra wikier',
'right-importupload'         => 'Importera sidor genom uppladdning',
'right-patrol'               => 'Markera ändringar som patrullerade',
'right-autopatrol'           => 'Får automatiskt sina ändringar markerade som patrullerade',
'right-patrolmarks'          => 'Se markeringar av opatrullerade ändringar i senaste ändringarna',
'right-unwatchedpages'       => 'Se listan över obevakade sidor',
'right-trackback'            => 'Ge respons',
'right-mergehistory'         => 'Sammanfoga sidhistoriker',
'right-userrights'           => 'Ändra alla användarrättigheter',
'right-userrights-interwiki' => 'Ändra rättigheter för användare på andra wikier',
'right-siteadmin'            => 'Låsa och låsa upp databasen',

# User rights log
'rightslog'      => 'Användarrättighetslogg',
'rightslogtext'  => 'Detta är en logg över ändringar av användares rättigheter.',
'rightslogentry' => 'ändrade grupptillhörighet för $1 från $2 till $3',
'rightsnone'     => '(inga)',

# Recent changes
'nchanges'                          => '$1 {{PLURAL:$1|ändring|ändringar}}',
'recentchanges'                     => 'Senaste ändringarna',
'recentchangestext'                 => 'Följ de senaste ändringarna i wikin på denna sida.',
'recentchanges-feed-description'    => 'Följ de senaste ändringarna i wikin genom den här matningen.',
'rcnote'                            => "Nedan visas {{PLURAL:$1|'''1''' ändring|de senaste '''$1''' ändringarna}} från {{PLURAL:$2|den senaste dagen|de senaste '''$2''' dagarna}}, per $4, kl. $5.",
'rcnotefrom'                        => "Nedan visas ändringar sedan '''$2''' (upp till '''$1''' visas).",
'rclistfrom'                        => 'Visa ändringar efter $1',
'rcshowhideminor'                   => '$1 mindre ändringar',
'rcshowhidebots'                    => '$1 robotar',
'rcshowhideliu'                     => '$1 inloggade användare',
'rcshowhideanons'                   => '$1 oinloggade användare',
'rcshowhidepatr'                    => '$1 kontrollerade redigeringar',
'rcshowhidemine'                    => '$1 mina ändringar',
'rclinks'                           => 'Visa de senaste $1 ändringarna under de senaste $2 dagarna<br />
$3',
'diff'                              => 'skillnad',
'hist'                              => 'historik',
'hide'                              => 'Göm',
'show'                              => 'Visa',
'minoreditletter'                   => 'm',
'newpageletter'                     => 'N',
'boteditletter'                     => 'b',
'number_of_watching_users_pageview' => '[$1 användare bevakar]',
'rc_categories'                     => 'Begränsa till följande kategorier (separera med "|")',
'rc_categories_any'                 => 'Vilken som helst',
'newsectionsummary'                 => '/* $1 */ nytt avsnitt',

# Recent changes linked
'recentchangeslinked'          => 'Ändringar på angränsande sidor',
'recentchangeslinked-title'    => 'Angränsande ändringar till $1',
'recentchangeslinked-noresult' => 'Inga ändringar på länkade sidor under den angivna tidsperioden.',
'recentchangeslinked-summary'  => "Detta är en lista över de senaste ändringarna på sidor som länkas till från en given sida (eller på sidor som hör till en viss kategori).
Sidor på [[Special:Watchlist|din bevakningslista]] är markerade med '''fetstil'''.",
'recentchangeslinked-page'     => 'Sidnamn:',
'recentchangeslinked-to'       => 'Visa ändringar på sidor med länkar till den givna sidan istället',

# Upload
'upload'                      => 'Ladda upp fil',
'uploadbtn'                   => 'Ladda upp fil',
'reupload'                    => 'Ladda upp på nytt',
'reuploaddesc'                => 'Avbryt uppladdningen och gå tillbaka till uppladdningsformuläret.',
'uploadnologin'               => 'Inte inloggad',
'uploadnologintext'           => 'Du måste vara [[Special:UserLogin|inloggad]] för att kunna ladda upp filer.',
'upload_directory_missing'    => 'Uppladdningskatalogen ($1) saknas och kunde inte skapas av webbservern.',
'upload_directory_read_only'  => 'Webbservern kan inte skriva till uppladdningskatalogen ($1).',
'uploaderror'                 => 'Fel vid uppladdningen',
'uploadtext'                  => "Använd formuläret nedan för att ladda upp filer.
För att titta på eller leta efter filer som redan har laddats upp, se [[Special:ImageList|listan över uppladdade filer]]. Uppladdningar loggförs även i [[Special:Log/upload|uppladdningsloggen]], och raderingar i [[Special:Log/delete|raderingsloggen]].

Använd en länk på något av följande format för att infoga en bild på en sida:
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:File.jpg]]</nowiki></tt>''' för att visa bilden i dess hela storlek
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:File.png|200px|thumb|left|alternativ text]]</nowiki></tt>''' för att visa en miniatyrbild med bredden 200 pixel i en ruta till vänster med bildtexten 'alternativ text'
* '''<tt><nowiki>[[</nowiki>{{ns:media}}<nowiki>:File.ogg]]</nowiki></tt>''' om du vill länka direkt till filen utan att visa den",
'upload-permitted'            => 'Tillåtna filtyper: $1.',
'upload-preferred'            => 'Föredragna filtyper: $1.',
'upload-prohibited'           => 'Förbjudna filtyper: $1.',
'uploadlog'                   => 'Uppladdningar',
'uploadlogpage'               => 'Uppladdningslogg',
'uploadlogpagetext'           => 'Det här är en logg över de senast uppladdade filerna.
Se [[Special:NewImages|galleriet över nya filer]] för en mer visuell översikt.',
'filename'                    => 'Filnamn',
'filedesc'                    => 'Beskrivning',
'fileuploadsummary'           => 'Beskrivning<br />och licens:',
'filestatus'                  => 'Upphovsrättslig status:',
'filesource'                  => 'Källa:',
'uploadedfiles'               => 'Uppladdade filer',
'ignorewarning'               => 'Ignorera varningen och spara filen ändå.',
'ignorewarnings'              => 'Ignorera eventuella varningar',
'minlength1'                  => 'Filens namn måste innehålla minst ett tecken.',
'illegalfilename'             => 'Filnamnet "$1" innehåller tecken som inte är tillåtna i sidtitlar. Byt namn på filen och försök ladda upp igen.',
'badfilename'                 => 'Filens namn har blivit ändrat till "$1".',
'filetype-badmime'            => 'Uppladdning av filer med MIME-typen "$1" är inte tillåten.',
'filetype-unwanted-type'      => "'''\".\$1\"''' är en oönskad filtyp.
{{PLURAL:\$3|Föredragen filtyp|Föredragna filtyper}} är \$2.",
'filetype-banned-type'        => "'''\".\$1\"''' är inte en tillåten filtyp.
{{PLURAL:\$3|Tillåtna filtyper|Tillåten filtyp}} är \$2.",
'filetype-missing'            => 'Filnamnet saknar ändelse (t ex ".jpg").',
'large-file'                  => 'Filer bör inte vara större än $1; denna fil är $2',
'largefileserver'             => 'Denna fil är större än vad servern ställts in att tillåta.',
'emptyfile'                   => 'Filen du laddade upp verkar vara tom; felet kan bero på ett stavfel i filnamnet. Kontrollera om du verkligen vill ladda upp denna fil.',
'fileexists'                  => 'Det finns redan en fil med detta namn. Titta på <strong><tt>$1</tt></strong>, såvida du inte är säker på att du vill ändra den.',
'filepageexists'              => 'Beskrivningssidan för denna fil har redan skapats på <strong><tt>$1</tt></strong>, men just nu finns ingen fil med detta namn. Den sammanfattning du skriver här kommer inte visas på beskrivningssidan. För att din sammanfattning ska visas där, så måste du redigera beskrivningssidan manuellt.',
'fileexists-extension'        => 'En fil med ett liknande namn finns redan:<br />
Namn på den fil du försöker ladda upp: <strong><tt>$1</tt></strong><br />
Namn på filen som redan finns: <strong><tt>$2</tt></strong><br />
Den enda skillnaden är versaliseringen av filnamnsändelsen. Var vänlig kontrollera om filerna är identiska.',
'fileexists-thumb'            => "<center>'''Den existerande filen'''</center>",
'fileexists-thumbnail-yes'    => 'Filen verkar vara en bild med förminskad storlek <i>(miniatyrbild)</i>. Var vänlig kontrollera filen <strong><tt>$1</tt></strong>.<br />
Om det är samma fil i originalstorlek så är det inte nödvändigt att ladda upp en extra miniatyrbild.',
'file-thumbnail-no'           => 'Filnamnet börjar med <strong><tt>$1</tt></strong>.
Det verkar vara en bild med förminskad storlek <i>(miniatyrbild)</i>.
Om du har denna bild i full storlek, ladda då hellre upp den, annars var vänlig och ändra filens namn.',
'fileexists-forbidden'        => 'Det finns redan en fil med detta namn.
Om du ändå vill ladda upp din fil, gå då tillbaka och välj ett annat namn. [[Image:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => 'En fil med detta namn finns redan bland de delade filerna.
Om du ändå vill ladda upp din fil, gå då tillbaka och använd ett annat namn. [[Image:$1|thumb|center|$1]]',
'file-exists-duplicate'       => 'Den här filen är en dubblett till följande {{PLURAL:$1|fil|filer}}:',
'successfulupload'            => 'Uppladdningen lyckades',
'uploadwarning'               => 'Uppladdningsvarning',
'savefile'                    => 'Spara fil',
'uploadedimage'               => 'laddade upp "[[$1]]"',
'overwroteimage'              => 'laddade upp ny version av "[[$1]]"',
'uploaddisabled'              => 'Uppladdningsfunktionen är avstängd',
'uploaddisabledtext'          => 'Uppladdning av filer är avstängd.',
'uploadscripted'              => 'Denna fil innehåller HTML eller script som felaktigt kan komma att tolkas av webbläsare.',
'uploadcorrupt'               => 'Antingen har det blivit något fel på filen, eller så har den en felaktig filändelse. Kontrollera din fil, och ladda upp på nytt.',
'uploadvirus'                 => 'Filen innehåller virus! Detaljer: $1',
'sourcefilename'              => 'Ursprungsfilens namn:',
'destfilename'                => 'Nytt filnamn:',
'upload-maxfilesize'          => 'Maximal filstorlek: $1',
'watchthisupload'             => 'Bevaka denna sida',
'filewasdeleted'              => 'En fil med detta namn har tidigare laddats upp och därefter tagits bort. Du bör kontrollera $1 innan du fortsätter att ladda upp den.',
'upload-wasdeleted'           => "'''Varning: Du håller på att ladda upp en fil som tidigare raderats.'''

Tänk över om det är lämpligt att fortsätta ladda upp denna fil.

Här finns raderingsloggen för denna fil:",
'filename-bad-prefix'         => 'Namnet på filen du vill ladda upp börjar med <strong>"$1"</strong>. Filnamnet kommer förmodligen direkt från en digitalkamera och beskriver inte filens innehåll. Välj ett annat filnamn som bättre beskriver filen.',
'filename-prefix-blacklist'   => ' #<!-- ändra inte den här raden --> <pre>
# Syntaxen är följande:
#   * All text från ett #-tecken till radens slut är en kommentar
#   * Icke-tomma rader anger typiska prefix för filnamn som används av olika digitalkameror
CIMG # Casio
DSC_ # Nikon
DSCF # Fuji
DSCN # Nikon
DUW # en del mobiltelefoner
IMG # allmänt bildprefix
JD # Jenoptik
MGP # Pentax
PICT # allmänt bildprefix
 #</pre> <!-- ändra inte den här raden -->',

'upload-proto-error'      => 'Felaktigt protokoll',
'upload-proto-error-text' => 'Fjärruppladdning kräver URL:ar som börjar med <code>http://</code> eller <code>ftp://</code>.',
'upload-file-error'       => 'Internt fel',
'upload-file-error-text'  => 'Ett internt fel inträffade när en temporär fil skulle skapas på servern. Kontakta en systemadministratör.',
'upload-misc-error'       => 'Okänt uppladdningsfel',
'upload-misc-error-text'  => 'Ett okänt fel inträffade under uppladdningen.
Kontrollera att URL:en giltig och försök igen.
Om problemet kvarstår, kontakta en [[Special:ListUsers/sysop|administratör]].',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => 'URL:en kunde inte nås',
'upload-curl-error6-text'  => 'Den angivna URL:en kunde inte nås. Kontrollera att den är korrekt och att webbplatsern fungerar.',
'upload-curl-error28'      => 'Timeout för uppladdningen',
'upload-curl-error28-text' => 'Webbplatsen tog för lång tid på sig att svara. Kontrollera att den är uppe och försök igen om en liten stund.',

'license'            => 'Licens:',
'nolicense'          => 'Ingen angiven',
'license-nopreview'  => '(Förhandsvisning är inte tillgänglig)',
'upload_source_url'  => ' (en giltig URL som är allmänt åtkomlig)',
'upload_source_file' => ' (en fil på din dator)',

# Special:ImageList
'imagelist-summary'     => 'Den här specialsidan visar alla filer som har laddats upp.
Som standard visas de senast upladdade filerna högst upp i listan.
Genom att klicka på rubrikerna för kolumnerna kan man ändra sorteringsordningen.',
'imagelist_search_for'  => 'Sök efter filnamn:',
'imgfile'               => 'fil',
'imagelist'             => 'Fillista',
'imagelist_date'        => 'Datum',
'imagelist_name'        => 'Namn',
'imagelist_user'        => 'Användare',
'imagelist_size'        => 'Storlek (byte)',
'imagelist_description' => 'Beskrivning',

# Image description page
'filehist'                       => 'Filhistorik',
'filehist-help'                  => 'Klicka på ett datum/klockslag för att se filen som den såg ut då.',
'filehist-deleteall'             => 'radera alla',
'filehist-deleteone'             => 'radera version',
'filehist-revert'                => 'återställ',
'filehist-current'               => 'nuvarande',
'filehist-datetime'              => 'Datum/Tid',
'filehist-user'                  => 'Användare',
'filehist-dimensions'            => 'Dimensioner',
'filehist-filesize'              => 'Filstorlek',
'filehist-comment'               => 'Kommentar',
'imagelinks'                     => 'Bildlänkar',
'linkstoimage'                   => 'Följande {{PLURAL:$1|sida|sidor}} länkar till den här filen:',
'nolinkstoimage'                 => 'Inga sidor länkar till den här filen.',
'morelinkstoimage'               => 'Visa [[Special:WhatLinksHere/$1|fler länkar]] till den här filen.',
'redirectstofile'                => 'Följande {{PLURAL:$1|fil är en omdirigering|filer är omdirigeringar}} till den här filen:',
'duplicatesoffile'               => 'Följande {{PLURAL:$1|fil är en dubblett|filer är dubbletter}} till den här filen:',
'sharedupload'                   => 'Denna fil är uppladdad som delad, och kan användas av andra projekt.',
'shareduploadwiki'               => 'Vänligen se $1 för mer information.',
'shareduploadwiki-desc'          => 'Innehållet på $1 i den gemensamma filförvaringen visas nedan.',
'shareduploadwiki-linktext'      => 'filens beskrivningssida',
'shareduploadduplicate'          => 'Den här filen är en dubblett till $1 i den delade filförvaringen.',
'shareduploadduplicate-linktext' => 'en fil',
'shareduploadconflict'           => 'Den här filen har samma namn som $1 i den delade filförvaringen.',
'shareduploadconflict-linktext'  => 'en fil',
'noimage'                        => 'Det finns ingen fil med detta namn, men du kan $1.',
'noimage-linktext'               => 'ladda upp en',
'uploadnewversion-linktext'      => 'Ladda upp en ny version av denna fil',
'imagepage-searchdupe'           => 'Sök efter dubbletter till denna fil',

# File reversion
'filerevert'                => 'Återställ $1',
'filerevert-legend'         => 'Återställ fil',
'filerevert-intro'          => "Du återställer '''[[Media:$1|$1]]''' till [$4 versionen från $2 kl. $3].",
'filerevert-comment'        => 'Kommentar:',
'filerevert-defaultcomment' => 'Återställer till versionen från $1 kl. $2.',
'filerevert-submit'         => 'Återställ',
'filerevert-success'        => "'''[[Media:$1|$1]]''' har återställts till [$4 versionen från $2 kl. $3].",
'filerevert-badversion'     => 'Det finns ingen tidigare version av filen från den angivna tidpunkten.',

# File deletion
'filedelete'                  => 'Radera $1',
'filedelete-legend'           => 'Radera fil',
'filedelete-intro'            => "Du håller på att radera '''[[Media:$1|$1]]'''.",
'filedelete-intro-old'        => "Du håller på att radera versionen av '''[[Media:$1|$1]]''' från [$4 $2 kl. $3].",
'filedelete-comment'          => 'Anledning:',
'filedelete-submit'           => 'Radera',
'filedelete-success'          => "'''$1''' har raderats.",
'filedelete-success-old'      => "Versionen av '''[[Media:$1|$1]]''' från $2 kl. $3 har raderats.",
'filedelete-nofile'           => "Filen '''$1''' finns inte.",
'filedelete-nofile-old'       => "Den versionen av '''$1''' kan inte raderas eftersom den inte finns.",
'filedelete-iscurrent'        => 'Du försöker radera den senaste versionen av en fil. För att göra det måste du först återställa till en äldre version av filen.',
'filedelete-otherreason'      => 'Annan/ytterligare anledning:',
'filedelete-reason-otherlist' => 'Annan anledning',
'filedelete-reason-dropdown'  => '*Vanliga anledningar till radering
** Upphovsrättsbrott
** Dubblettfil',
'filedelete-edit-reasonlist'  => 'Redigera anledningar för radering',

# MIME search
'mimesearch'         => 'MIME-sökning',
'mimesearch-summary' => 'På den här sidan kan du söka efter filer via dess MIME-typ. Input: contenttype/subtype, t.ex. <tt>image/jpeg</tt>.',
'mimetype'           => 'MIME-typ:',
'download'           => 'ladda ner',

# Unwatched pages
'unwatchedpages' => 'Obevakade sidor',

# List redirects
'listredirects' => 'Lista över omdirigeringar',

# Unused templates
'unusedtemplates'     => 'Oanvända mallar',
'unusedtemplatestext' => 'Denna sida listar alla de sidor i mallnamnrymden som inte inkluderas på någon annan sida. Innan mallarna raderas, kontrollera att det inte finns andra länkar till dem.',
'unusedtemplateswlh'  => 'andra länkar',

# Random page
'randompage'         => 'Slumpsida',
'randompage-nopages' => 'Det finns inte några sidor i denna namnrymd.',

# Random redirect
'randomredirect'         => 'Slumpvald omdirigering',
'randomredirect-nopages' => 'Det finns inte några omdirigeringar i denna namnrymd.',

# Statistics
'statistics'             => 'Statistik',
'sitestats'              => 'Statistiksida',
'userstats'              => 'Användarstatistik',
'sitestatstext'          => "I databasen finns just nu <b>$1</b> {{PLURAL:$1|sida|sidor}}, inklusive diskussionssidor, sidor om {{SITENAME}}, korta stumpartiklar, omdirigeringssidor, och andra sidor som inte kan räknas som artiklar. Om man tar bort ovanstående, återstår <b>$2</b> {{PLURAL:$2|riktig artikel|riktiga artiklar}}.

'''$8''' {{PLURAL:$8|fil|filer}} har laddats upp.

Sedan denna wiki startades har sidor visats totalt <b>$3</b> {{PLURAL:$3|gång|gånger}}, och <b>$4</b> {{PLURAL:$4|sida|sidor}} har ändrats. Detta är i genomsnitt <b>$5</b> ändringar per sida, och <b>$6</b> sidvisningar per ändring.

[http://www.mediawiki.org/wiki/Manual:Job_queue Jobbkön]s längd är för tillfället '''$7'''.",
'userstatstext'          => "Det finns '''$1''' {{PLURAL:$1|registrerad|registrerade}} [[Special:ListUsers|användare]]. Av dem är '''$2''' (eller '''$4%''') $5.",
'statistics-mostpopular' => 'Mest besökta sidor',

'disambiguations'      => 'Sidor som länkar till förgreningssidor',
'disambiguationspage'  => 'Template:Förgrening',
'disambiguations-text' => "Följande sidor länkar till ''förgreningssidor''.
Länkarna bör troligtvis ändras så att de länkar till en artikel istället.<br />
En sida anses vara en förgreningssida om den inkluderar en mall som länkas till från [[MediaWiki:Disambiguationspage]].",

'doubleredirects'            => 'Dubbla omdirigeringar',
'doubleredirectstext'        => 'Det här är en lista över sidor som omdirigerar till andra omdirigeringssidor. Varje rad innehåller länkar till den första och den andra omdirigeringsidan, samt till målet för den andra omdirigeringen. Målet för den andra omdirigeringen är ofta den "riktiga" sidan, som den första omdirigeringen egentligen ska leda till.',
'double-redirect-fixed-move' => '[[$1]] har flyttats, och är nu en omdirigering till [[$2]]',
'double-redirect-fixer'      => 'Omdirigeringsrättaren',

'brokenredirects'        => 'Trasiga omdirigeringar',
'brokenredirectstext'    => 'Följande länkar omdirigerar till sidor som inte existerar.',
'brokenredirects-edit'   => '(redigera)',
'brokenredirects-delete' => '(radera)',

'withoutinterwiki'         => 'Sidor utan språklänkar',
'withoutinterwiki-summary' => 'Följande sidor innehåller inte några länkar till andra språkversioner.',
'withoutinterwiki-legend'  => 'Prefix',
'withoutinterwiki-submit'  => 'Visa',

'fewestrevisions' => 'Sidor med minst antal ändringar',

# Miscellaneous special pages
'nbytes'                  => '$1 byte',
'ncategories'             => '$1 {{PLURAL:$1|kategori|kategorier}}',
'nlinks'                  => '$1 {{PLURAL:$1|länk|länkar}}',
'nmembers'                => '$1 {{PLURAL:$1|medlem|medlemmar}}',
'nrevisions'              => '$1 {{PLURAL:$1|ändring|ändringar}}',
'nviews'                  => '$1 {{PLURAL:$1|visning|visningar}}',
'specialpage-empty'       => 'Den här sidan är tom.',
'lonelypages'             => 'Föräldralösa sidor',
'lonelypagestext'         => 'Följande sidor länkas inte till från någon annan sida på den här wikin.',
'uncategorizedpages'      => 'Ej kategoriserade sidor',
'uncategorizedcategories' => 'Ej kategoriserade kategorier',
'uncategorizedimages'     => 'Filer utan kategori',
'uncategorizedtemplates'  => 'Ej kategoriserade mallar',
'unusedcategories'        => 'Tomma kategorier',
'unusedimages'            => 'Oanvända bilder',
'popularpages'            => 'Populära sidor',
'wantedcategories'        => 'Önskade kategorier',
'wantedpages'             => 'Önskade sidor',
'missingfiles'            => 'Saknade filer',
'mostlinked'              => 'Sidor med flest länkar till sig',
'mostlinkedcategories'    => 'Kategorier med flest länkar till sig',
'mostlinkedtemplates'     => 'Mest använda mallar',
'mostcategories'          => 'Sidor med flest kategorier',
'mostimages'              => 'Filer med flest länkar till sig',
'mostrevisions'           => 'Sidor med flest ändringar',
'prefixindex'             => 'Prefixindex',
'shortpages'              => 'Korta sidor',
'longpages'               => 'Långa sidor',
'deadendpages'            => 'Sidor utan länkar',
'deadendpagestext'        => 'Följande sidor saknar länkar till andra sidor på den här wikin.',
'protectedpages'          => 'Skyddade sidor',
'protectedpages-indef'    => 'Endast skydd på obestämd tid',
'protectedpagestext'      => 'Följande sidor är skyddade mot redigering eller flyttning.',
'protectedpagesempty'     => 'Inga sidor är skyddade under de villkoren.',
'protectedtitles'         => 'Skyddade titlar',
'protectedtitlestext'     => 'Följande sidtitlar är skyddade från att skapas',
'protectedtitlesempty'    => 'Just nu finns inga skyddade sidtitlar med de parametrarna.',
'listusers'               => 'Användarlista',
'newpages'                => 'Nya sidor',
'newpages-username'       => 'Användare:',
'ancientpages'            => 'Äldsta sidorna',
'move'                    => 'Flytta',
'movethispage'            => 'Flytta denna sida',
'unusedimagestext'        => 'Lägg märke till att andra webbplatser kan länka till filer med en direkt URL. Filer kan därför  användas trots att de finns i den här listan.',
'unusedcategoriestext'    => 'Följande kategorier finns men innehåller inga sidor eller underkategorier.',
'notargettitle'           => 'Inget mål',
'notargettext'            => 'Du har inte angivit någon sida eller användare att utföra denna funktion på.',
'nopagetitle'             => 'Målsidan finns inte',
'nopagetext'              => 'Sidan som du vill flytta finns inte.',
'pager-newer-n'           => '{{PLURAL:$1|1 nyare|$1 nyare}}',
'pager-older-n'           => '{{PLURAL:$1|1 äldre|$1 äldre}}',
'suppress'                => 'Oversight',

# Book sources
'booksources'               => 'Bokkällor',
'booksources-search-legend' => 'Sök efter bokkällor',
'booksources-go'            => 'Sök',
'booksources-text'          => 'Nedan följer en lista över länkar till webbplatser som säljer nya och begagnade böcker, och som kanske har ytterligare information om de böcker du söker.',

# Special:Log
'specialloguserlabel'  => 'Användare:',
'speciallogtitlelabel' => 'Titel:',
'log'                  => 'Loggar',
'all-logs-page'        => 'Alla loggar',
'log-search-legend'    => 'Sök efter loggar',
'log-search-submit'    => 'Sök',
'alllogstext'          => 'Kombinerad visning av alla tillgängliga loggar för {{SITENAME}}.
Du kan avgränsa sökningen och få färre träffar genom att ange typ av logg, användarnamn (skiftlägeskänsligt), eller berörd sida (också skiftlägeskänsligt).',
'logempty'             => 'Inga matchande träffar i loggen.',
'log-title-wildcard'   => 'Sök efter sidtitlar som börjar med texten',

# Special:AllPages
'allpages'          => 'Alla sidor',
'alphaindexline'    => '$1 till $2',
'nextpage'          => 'Nästa sida ($1)',
'prevpage'          => 'Föregående sida ($1)',
'allpagesfrom'      => 'Visa sidor från och med:',
'allarticles'       => 'Alla sidor',
'allinnamespace'    => 'Alla sidor (i namnrymden $1)',
'allnotinnamespace' => 'Alla sidor (inte i namnrymden $1)',
'allpagesprev'      => 'Föregående',
'allpagesnext'      => 'Nästa',
'allpagessubmit'    => 'Visa',
'allpagesprefix'    => 'Visa sidor med prefixet:',
'allpagesbadtitle'  => 'Den sökta sidtiteln var ogiltig eller så innehöll den ett prefix för annan språkversion eller interwiki-prefix. Titeln kan innehålla bokstäver som inte är tillåtna i sidtitlar.',
'allpages-bad-ns'   => 'Namnrymden "$1" finns inte på {{SITENAME}}.',

# Special:Categories
'categories'                    => 'Kategorier',
'categoriespagetext'            => 'Följande kategorier innehåller sidor eller media.
[[Special:UnusedCategories|Oanvända kategorier]] visas inte här; [[Special:WantedCategories|önskade kategorier]] listas även separat.',
'categoriesfrom'                => 'Visa kategorier från och med:',
'special-categories-sort-count' => 'sortera efter storlek',
'special-categories-sort-abc'   => 'sortera alfabetiskt',

# Special:ListUsers
'listusersfrom'      => 'Visa användare från och med:',
'listusers-submit'   => 'Visa',
'listusers-noresult' => 'Ingen användare hittades.',

# Special:ListGroupRights
'listgrouprights'          => 'Behörigheter för användargrupper',
'listgrouprights-summary'  => 'Följande lista visar vilka användargrupper som är definierade på den här wikin och vilka behörigheter grupperna har.
Det kan finnas [[{{MediaWiki:Listgrouprights-helppage}}|ytterligare information]] om de olika behörigheterna.',
'listgrouprights-group'    => 'Grupp',
'listgrouprights-rights'   => 'Behörigheter',
'listgrouprights-helppage' => 'Help:Gruppbehörigheter',
'listgrouprights-members'  => '(lista över medlemmar)',

# E-mail user
'mailnologin'     => 'Ingen adress att skicka till',
'mailnologintext' => 'För att kunna skicka e-post till andra användare, måste du vara [[Special:UserLogin|inloggad]] och ha angivit en korrekt e-postadress i dina [[Special:Preferences|användarinställningar]].',
'emailuser'       => 'Skicka e-post till den här användaren',
'emailpage'       => 'Skicka e-post till annan användare',
'emailpagetext'   => 'Om den här användaren har skrivit in en giltig e-postadress i sina användarinställningar, kommer formuläret nedan att skicka ett meddelande.
Den e-postadress du har angivit i [[Special:Preferences|dina användarinställningar]] kommer att visas som "Från"-adress i meddelandet, så att mottagaren har möjlighet att svara direkt till dig.',
'usermailererror' => 'Fel i hanteringen av mail:',
'defemailsubject' => '{{SITENAME}} e-post',
'noemailtitle'    => 'Ingen e-postadress',
'noemailtext'     => 'Den här användaren har inte angivet en giltig e-postadress eller har valt att inte ta emot mail från andra användare.',
'emailfrom'       => 'Från:',
'emailto'         => 'Till:',
'emailsubject'    => 'Ämne:',
'emailmessage'    => 'Meddelande:',
'emailsend'       => 'Skicka',
'emailccme'       => 'Skicka en kopia av meddelandet till mig.',
'emailccsubject'  => 'Kopia av ditt meddelande till $1: $2',
'emailsent'       => 'E-post har nu skickats',
'emailsenttext'   => 'Din e-post har skickats.',
'emailuserfooter' => 'Detta e-brev skickades av $1 till $2 genom "Skicka e-post"-funktionen på {{SITENAME}}.',

# Watchlist
'watchlist'            => 'Bevakningslista',
'mywatchlist'          => 'Min bevakningslista',
'watchlistfor'         => "(för '''$1''')",
'nowatchlist'          => 'Du har inga sidor i din bevakningslista.',
'watchlistanontext'    => 'Du måste $1 för att se eller redigera din bevakningslista.',
'watchnologin'         => 'Inte inloggad',
'watchnologintext'     => 'Du måste vara [[Special:UserLogin|inloggad]] för att kunna ändra din bevakningslista.',
'addedwatch'           => 'Tillagd på bevakningslistan',
'addedwatchtext'       => "Sidan \"[[:\$1]]\" har lagts till på din [[Special:Watchlist|bevakningslista]].
Framtida ändringar av den här sidan och dess diskussionssida kommer att listas där, och sidan kommer att markeras med '''fetstil''' i [[Special:RecentChanges|listan över de senaste ändringarna]] för att lättare kunna hittas.",
'removedwatch'         => 'Borttagen från bevakningslista',
'removedwatchtext'     => 'Sidan "[[:$1]]" har tagits bort från [[Special:Watchlist|din bevakningslista]].',
'watch'                => 'bevaka',
'watchthispage'        => 'Bevaka denna sida',
'unwatch'              => 'avbevaka',
'unwatchthispage'      => 'Sluta bevaka',
'notanarticle'         => 'Inte en artikel',
'notvisiblerev'        => 'Sidversionen har raderats',
'watchnochange'        => 'Inga av dina bevakade sidor har ändrats inom den visade tidsperioden.',
'watchlist-details'    => 'Du har $1 {{PLURAL:$1|sida|sidor}} på din bevakningslista (diskussionssidor är inte medräknade).',
'wlheader-enotif'      => '* Bekräftelse per e-post är aktiverad.',
'wlheader-showupdated' => "* Sidor som har ändrats sedan ditt senaste besök visas i '''fetstil.'''",
'watchmethod-recent'   => 'letar efter bevakade sidor bland senaste ändringar',
'watchmethod-list'     => 'letar efter nyligen gjorda ändringar bland bevakade sidor',
'watchlistcontains'    => 'Din bevakningslista innehåller $1 {{PLURAL:$1|sida|sidor}}.',
'iteminvalidname'      => "Problem med sidan '$1', ogiltigt namn...",
'wlnote'               => 'Nedan finns {{PLURAL:$1|den senaste ändringen|de senaste $1 ändringarna}} under {{PLURAL:$2|den senaste timmen|de senaste <b>$2</b> timmarna}}.',
'wlshowlast'           => 'Visa senaste $1 timmarna $2 dagarna $3',
'watchlist-show-bots'  => 'Visa robotredigeringar',
'watchlist-hide-bots'  => 'Göm robotredigeringar',
'watchlist-show-own'   => 'Visa mina redigeringar',
'watchlist-hide-own'   => 'Göm mina redigeringar',
'watchlist-show-minor' => 'Visa mindre ändringar',
'watchlist-hide-minor' => 'Göm mindre ändringar',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Bevakar...',
'unwatching' => 'Avbevakar...',

'enotif_mailer'                => '{{SITENAME}}s system för att få meddelanden om förändringar per e-post',
'enotif_reset'                 => 'Markera alla sidor som besökta',
'enotif_newpagetext'           => 'Detta är en ny sida.',
'enotif_impersonal_salutation' => '{{SITENAME}}användare',
'changed'                      => 'ändrad',
'created'                      => 'skapad',
'enotif_subject'               => '{{SITENAME}}-sidan $PAGETITLE har blivit $CHANGEDORCREATED av $PAGEEDITOR',
'enotif_lastvisited'           => 'På $1 återfinner du alla ändringar sedan ditt senaste besök.',
'enotif_lastdiff'              => 'Se denna ändring på $1',
'enotif_anon_editor'           => 'anonym användare $1',
'enotif_body'                  => '$WATCHINGUSERNAME,

{{SITENAME}}-sidan $PAGETITLE har blivit $CHANGEDORCREATED $PAGEEDITDATE av $PAGEEDITOR; den nuvarande versionen hittar du på $PAGETITLE_URL.

$NEWPAGE

Angiven sammanfattning av redigeringen: $PAGESUMMARY $PAGEMINOREDIT

Kontakta användaren:
e-post: $PAGEEDITOR_EMAIL
wiki: $PAGEEDITOR_WIKI

Såvida du inte besöker sidan, kommer du inte att få flera meddelanden om ändringar av sidan.
Du kan också ta bort flaggan för meddelanden om ändringar på alla sidor i din bevakningslista.

Hälsningar från {{SITENAME}}s meddelandesystem

--
För att ändra inställningarna i din bevakningslista, besök
{{fullurl:{{ns:special}}:Watchlist/edit}}

Feedback och hjälp:
{{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => 'Ta bort sida',
'confirm'                     => 'Bekräfta',
'excontent'                   => "Före radering: '$1'",
'excontentauthor'             => "sidan innehöll '$1' (den enda som skrivit var '$2')",
'exbeforeblank'               => "Före tömning: '$1'",
'exblank'                     => 'sidan var tom',
'delete-confirm'              => 'Radera "$1"',
'delete-legend'               => 'Radera',
'historywarning'              => 'Varning: Sidan du håller på att radera har en historik:',
'confirmdeletetext'           => 'Du håller på att ta bort en sida med hela dess historik.
Bekräfta att du förstår vad du håller på med och vilka konsekvenser detta leder till, och att du följer [[{{MediaWiki:Policy-url}}|riktlinjerna]].',
'actioncomplete'              => 'Genomfört',
'deletedtext'                 => '"<nowiki>$1</nowiki>" har tagits bort.
Se $2 för noteringar om de senaste raderingarna.',
'deletedarticle'              => 'raderade "[[$1]]"',
'suppressedarticle'           => 'upphävde "[[$1]]"',
'dellogpage'                  => 'Raderingslogg',
'dellogpagetext'              => 'Nedan listas de senaste raderingarna och återställningarna.',
'deletionlog'                 => 'raderingsloggen',
'reverted'                    => 'Återgått till tidigare version',
'deletecomment'               => 'Anledning till borttagning:',
'deleteotherreason'           => 'Annan/ytterligare anledning:',
'deletereasonotherlist'       => 'Annan anledning',
'deletereason-dropdown'       => '*Vanliga anledningar till radering
** Författarens begäran
** Upphovsrättsbrott
** Vandalism',
'delete-edit-reasonlist'      => 'Redigera anledningar för radering',
'delete-toobig'               => 'Denna sida har en lång redigeringshistorik med mer än $1 {{PLURAL:$1|sidversion|sidversioner}}. Borttagning av sådana sidor har begränsats för att förhindra oavsiktliga driftstörningar på {{SITENAME}}.',
'delete-warning-toobig'       => 'Denna sida har en lång redigeringshistorik med mer än $1 {{PLURAL:$1|sidversion|sidversioner}}. Att radera sidan kan skapa problem med hanteringen av databasen på {{SITENAME}}; var försiktig.',
'rollback'                    => 'Rulla tillbaka ändringar',
'rollback_short'              => 'Återställning',
'rollbacklink'                => 'rulla tillbaka',
'rollbackfailed'              => 'Tillbakarullning misslyckades',
'cantrollback'                => 'Det gick inte att rulla tillbaka, då sidan endast redigerats av en användare.',
'alreadyrolled'               => 'Det gick inte att rulla tillbaka den sista redigeringen av [[User:$2|$2]] ([[User talk:$2|diskussion]] | [[Special:Contributions/$2|{{int:contribslink}}]]) på sidan [[:$1|$1]]. Någon annan har redan rullat tillbaka eller redigerat sidan.

Sidan ändrades senast av [[User:$3|$3]] ([[User talk:$3|diskussion]] | [[Special:Contributions/$2|{{int:contribslink}}]]).',
'editcomment'                 => 'Redigeringskommentaren var: "<i>$1</i>".', # only shown if there is an edit comment
'revertpage'                  => 'Återställde redigeringar av  [[Special:Contributions/$2|$2]] ([[User talk:$2|användardiskussion]]) till senaste versionen av [[User:$1|$1]]', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => 'Återställde ändringar av $1 till senaste versionen av $2.',
'sessionfailure'              => 'Något med din session som inloggad är på tok. Din begärda åtgärd har avbrutits, för att förhindra att någon kapar din session. Klicka på "Tillbaka" i din webbläsare och ladda om den sida du kom ifrån. Försök sedan igen.',
'protectlogpage'              => 'Skrivskyddslogg',
'protectlogtext'              => 'Detta är en lista över applicerande och borttagande av skrivskydd.',
'protectedarticle'            => 'skyddade [[$1]]',
'modifiedarticleprotection'   => 'ändrade skyddsnivån för "[[$1]]"',
'unprotectedarticle'          => 'tog bort skydd av $1',
'protect-title'               => 'Skyddsinställningar för "$1"',
'protect-legend'              => 'Bekräfta skrivskydd av sida',
'protectcomment'              => 'Anledning:',
'protectexpiry'               => 'Varaktighet:',
'protect_expiry_invalid'      => 'Ogiltig varaktighetstid.',
'protect_expiry_old'          => 'Den angivna varaktighetentiden har redan passerats.',
'protect-unchain'             => 'Lås upp flyttillstånd',
'protect-text'                => 'Här kan du se och ändra skyddsnivån av sidan <strong><nowiki>$1</nowiki></strong>.',
'protect-locked-blocked'      => 'Du kan inte ändra sidors skydd medan du är blockerad.
Här kan du se gällande skyddsinställninger för sidan <strong>$1</strong>:',
'protect-locked-dblock'       => 'Skrivskydd kan inte ändras då databasen är låst.
Nuvarande skrivskyddsinställning för sidan <strong>$1</strong> är:',
'protect-locked-access'       => 'Ditt konto har inte behörighet att ändra skrivskydd på sidor.
Nuvarande skrivskyddsinställning för sidan <strong>$1</strong> är:',
'protect-cascadeon'           => 'Den här sidan är skrivskyddad eftersom den inkluderas på följande {{PLURAL:$1|sida|sidor}} som har ett kaskaderande skydd.
Du kan ändra skyddet av den här sidan, men det påverkar inte det kaskaderande skyddet.',
'protect-default'             => '(standard)',
'protect-fallback'            => 'Kräver "$1"-behörighet',
'protect-level-autoconfirmed' => 'Blockera oregistrerade användare',
'protect-level-sysop'         => 'Enbart administratörer',
'protect-summary-cascade'     => 'kaskaderande',
'protect-expiring'            => 'upphör den $1 (UTC)',
'protect-cascade'             => 'Skydda sidor som är inkluderade i den här sidan (kaskaderande skydd)',
'protect-cantedit'            => 'Du kan inte ändra skyddsnivån för den här sidan, eftersom du inte har behörighet att redigera den.',
'restriction-type'            => 'Typ av skydd:',
'restriction-level'           => 'Skyddsnivå:',
'minimum-size'                => 'Minsta storlek',
'maximum-size'                => 'Största storlek:',
'pagesize'                    => '(byte)',

# Restrictions (nouns)
'restriction-edit'   => 'Redigering',
'restriction-move'   => 'Flyttning',
'restriction-create' => 'Skapa sidan',
'restriction-upload' => 'Uppladdning',

# Restriction levels
'restriction-level-sysop'         => 'helt låst',
'restriction-level-autoconfirmed' => 'halvlåst',
'restriction-level-all'           => 'alla nivåer',

# Undelete
'undelete'                     => 'Visa raderade sidor',
'undeletepage'                 => 'Visa och återställ borttagna sidor',
'undeletepagetitle'            => "'''Härunder visas en lista över raderade versioner av [[:$1]]'''.",
'viewdeletedpage'              => 'Visa raderade sidor',
'undeletepagetext'             => 'Följande sidor har blivit borttagna, men finns fortfarande i ett arkiv och kan återställas. Arkivet kan ibland rensas på gamla versioner.',
'undelete-fieldset-title'      => 'Återställ sidversioner',
'undeleteextrahelp'            => "För att återställa sidans hela historik, lämna alla rutor oifyllda och klicka '''''Återställ'''''.
För att göra en selektiv återställning, kryssa i de rutor som hör till de versioner som ska återställas, och klicka '''''Återställ'''''.
Om du klickar '''''Rensa''''' så töms kommentarfältet och alla kryssrutorna.",
'undeleterevisions'            => '$1 {{PLURAL:$1|version|versioner}} arkiverade',
'undeletehistory'              => 'Om du återställer sidan kommer alla tidigare versioner att återfinnas i versionshistoriken.
Om en ny sida med samma namn har skapats sedan sidan raderades, kommer den återskapade historiken automatiskt att återfinnas i den äldre historiken.',
'undeleterevdel'               => 'Återställningen kan inte utföras om den resulterar i att den senaste versionen är delvis borttagen.
I sådana fall måste du se till att den senaste raderade versionen inte är ikryssad, eller att den inte är dold.',
'undeletehistorynoadmin'       => 'Den här sidan har blivit raderad. Anledningen till detta anges i sammanfattningen nedan, tillsammans med uppgifter om de användare som redigerat sidan innan den raderades. Enbart administratörerna har tillgång till den raderade texten.',
'undelete-revision'            => 'Raderad version av $1 från den $2 av $3.',
'undeleterevision-missing'     => 'Versionen finns inte eller är felaktig. Versionen kan ha återställts eller tagits bort från arkivet, du kan också ha följt en felaktig länk.',
'undelete-nodiff'              => 'Ingen tidigare version hittades.',
'undeletebtn'                  => 'Återställ',
'undeletelink'                 => 'återställ',
'undeletereset'                => 'Rensa',
'undeletecomment'              => 'Kommentar:',
'undeletedarticle'             => 'återställde "$1"',
'undeletedrevisions'           => '{{PLURAL:$1|en version återställd|$1 versioner återställda}}',
'undeletedrevisions-files'     => '$1 {{PLURAL:$1|version|versioner}} och $2 {{PLURAL:$2|fil|filer}} återställda',
'undeletedfiles'               => '$1 {{PLURAL:$1|fil återställd|filer återställda}}',
'cannotundelete'               => 'Återställning misslyckades; kanske någon redan har återställt sidan.',
'undeletedpage'                => "<big>'''$1 har återställts'''</big>

I  [[Special:Log/delete|borttagningsloggen]] kan du hitta information om nyligen borttagna och återställda sidor.",
'undelete-header'              => 'Se [[Special:Log/delete|raderingsloggen]] för nyligen raderade sidor.',
'undelete-search-box'          => 'Sök efter raderade sidor',
'undelete-search-prefix'       => 'Sidor som börjar med:',
'undelete-search-submit'       => 'Sök',
'undelete-no-results'          => 'Inga sidor med sådan titel hittades i arkivet över raderade sidor.',
'undelete-filename-mismatch'   => 'Filversionen med tidsstämpeln $1 kan inte återställas: filnamnet stämmer inte.',
'undelete-bad-store-key'       => 'Filversionen med tidsstämpeln $1 kan inte återställas: filen saknades före radering.',
'undelete-cleanup-error'       => 'Fel vid radering av den oanvända arkivfilen "$1".',
'undelete-missing-filearchive' => 'Filen med arkiv-ID $1 kunde inte återställas eftersom den inte finns i databasen. Filen kanske redan har återställts.',
'undelete-error-short'         => 'Fel vid filåterställning: $1',
'undelete-error-long'          => 'Fel inträffade när vid återställning av filen:

$1',

# Namespace form on various pages
'namespace'      => 'Namnrymd:',
'invert'         => 'Uteslut vald namnrymd',
'blanknamespace' => '(Huvudnamnrymden)',

# Contributions
'contributions' => 'Användarbidrag',
'mycontris'     => 'Mina bidrag',
'contribsub2'   => 'För $1 ($2)',
'nocontribs'    => 'Inga ändringar hittades, som motsvarar dessa kriterier',
'uctop'         => '(senaste)',
'month'         => 'Från månad (och tidigare):',
'year'          => 'Från år (och tidigare):',

'sp-contributions-newbies'     => 'Visa endast bidrag från nya konton',
'sp-contributions-newbies-sub' => 'Från nya konton',
'sp-contributions-blocklog'    => 'Blockeringslogg',
'sp-contributions-search'      => 'Sök efter användarbidrag',
'sp-contributions-username'    => 'IP-adress eller användarnamn:',
'sp-contributions-submit'      => 'Sök',

# What links here
'whatlinkshere'            => 'Sidor som länkar hit',
'whatlinkshere-title'      => 'Sidor som länkar till "$1"',
'whatlinkshere-page'       => 'Sida:',
'linklistsub'              => '(Länklista)',
'linkshere'                => "Följande sidor länkar till '''[[:$1]]''':",
'nolinkshere'              => "Inga sidor länkar till '''[[:$1]]'''.",
'nolinkshere-ns'           => "Inga sidor i den angivna namnrymden länkar till '''[[:$1]]'''.",
'isredirect'               => 'omdirigeringssida',
'istemplate'               => 'inkluderad som mall',
'isimage'                  => 'fillänk',
'whatlinkshere-prev'       => '{{PLURAL:$1|förra|förra $1}}',
'whatlinkshere-next'       => '{{PLURAL:$1|nästa|nästa $1}}',
'whatlinkshere-links'      => '← länkar',
'whatlinkshere-hideredirs' => '$1 omdirigeringar',
'whatlinkshere-hidetrans'  => '$1 mallinkluderingar',
'whatlinkshere-hidelinks'  => '$1 länkar',
'whatlinkshere-hideimages' => '$1 fillänkar',
'whatlinkshere-filters'    => 'Filter',

# Block/unblock
'blockip'                         => 'Blockera användare',
'blockip-legend'                  => 'Blockera användare',
'blockiptext'                     => 'Använd formuläret nedan för att blockera möjligheten att redigera sidor från en specifik IP-adress eller ett användarnamn.
Detta bör endast göras för att förhindra vandalisering, och i överensstämmelse med gällande [[{{MediaWiki:Policy-url}}|policy]].
Ange orsak nedan (exempelvis genom att nämna sidor som blivit vandaliserade).',
'ipaddress'                       => 'IP-adress',
'ipadressorusername'              => 'IP-adress eller användarnamn:',
'ipbexpiry'                       => 'Varaktighet:',
'ipbreason'                       => 'Anledning:',
'ipbreasonotherlist'              => 'Annan anledning',
'ipbreason-dropdown'              => '*Vanliga motiv till blockering
** Infogar falsk information
** Tar bort sidinnehåll
** Länkspam till externa sajter
** Lägger till nonsens på sidor
** Hotfullt beteende/trakasserier
** Missbruk av flera användarkonton
** Oacceptabelt användarnamn',
'ipbanononly'                     => 'Blockera bara oinloggade användare',
'ipbcreateaccount'                => 'Förhindra registrering av användarkonton',
'ipbemailban'                     => 'Hindra användaren från att skicka e-post',
'ipbenableautoblock'              => 'Blockera automatiskt den IP-adress som användaren använde senast, samt alla adresser som användaren försöker redigera ifrån',
'ipbsubmit'                       => 'Blockera användaren',
'ipbother'                        => 'Annan tidsperiod:',
'ipboptions'                      => '2 timmar:2 hours,1 dag:1 day,3 dagar:3 days,1 vecka:1 week,2 veckor:2 weeks,1 månad:1 month,3 månader:3 months,6 månader:6 months,1 år:1 year,oändlig:infinite', # display1:time1,display2:time2,...
'ipbotheroption'                  => 'annan tidsperiod',
'ipbotherreason'                  => 'Annan/ytterligare anledning:',
'ipbhidename'                     => 'Dölj användarnamnet från blockeringsloggen, blockeringslistan och användarlistan',
'ipbwatchuser'                    => 'Bevaka användarens användarsida och diskussionssida',
'badipaddress'                    => 'Du har inte skrivit IP-adressen korrekt.',
'blockipsuccesssub'               => 'Blockeringen är utförd',
'blockipsuccesstext'              => '[[Special:Contributions/$1|$1]] har blockerats.
<br />För att se alla aktuella blockeringar, gå till [[Special:IPBlockList|listan över blockeringar]].',
'ipb-edit-dropdown'               => 'Redigera blockeringsanledningar',
'ipb-unblock-addr'                => 'Ta bort blockering av $1',
'ipb-unblock'                     => 'Ta bort blockering av en användare eller IP-adress',
'ipb-blocklist-addr'              => 'Visa gällande blockeringar av $1',
'ipb-blocklist'                   => 'Visa gällande blockeringar',
'unblockip'                       => 'Ta bort blockering av användare/IP-adress',
'unblockiptext'                   => 'Använd formuläret nedan för att ta bort blockeringen av en IP-adress.',
'ipusubmit'                       => 'Ta bort blockeringen',
'unblocked'                       => 'Blockeringen av [[User:$1|$1]] har hävts',
'unblocked-id'                    => 'Blockeringen $1 har hävts',
'ipblocklist'                     => 'Blockerade IP-adresser och användarnamn',
'ipblocklist-legend'              => 'Sök efter en blockerad användare',
'ipblocklist-username'            => 'Användarnamn eller IP-adress',
'ipblocklist-submit'              => 'Sök',
'blocklistline'                   => '$1: $2 blockerar $3 $4',
'infiniteblock'                   => 'för evigt',
'expiringblock'                   => 'till $1',
'anononlyblock'                   => 'endast för oinloggade',
'noautoblockblock'                => 'utan automatisk blockering',
'createaccountblock'              => 'kontoregistrering blockerad',
'emailblock'                      => 'e-post blockerad',
'ipblocklist-empty'               => 'Listan över blockerade IP-adresser är tom.',
'ipblocklist-no-results'          => 'Den angivna IP-adressen eller användaren är inte blockerad.',
'blocklink'                       => 'blockera',
'unblocklink'                     => 'ta bort blockering',
'contribslink'                    => 'bidrag',
'autoblocker'                     => 'Automatisk blockering eftersom du har samma IP-adress som "$1". Motivering till blockeringen: "$2".',
'blocklogpage'                    => 'Blockeringslogg',
'blocklogentry'                   => 'blockerade [[$1]] med blockeringstid på $2 $3',
'blocklogtext'                    => 'Detta är en logg över blockeringar och avblockeringar.
Automatiskt blockerade IP-adresser listas ej.
I [[Special:IPBlockList|blockeringslistan]] listas alla IP-adresser och användare som är blockerade för närvarande.',
'unblocklogentry'                 => 'tog bort blockering av "$1"',
'block-log-flags-anononly'        => 'bara oinloggade',
'block-log-flags-nocreate'        => 'hindrar kontoregistrering',
'block-log-flags-noautoblock'     => 'utan automatblockering',
'block-log-flags-noemail'         => 'e-post blockerad',
'block-log-flags-angry-autoblock' => 'utökad automatblockering aktiverad',
'range_block_disabled'            => 'Möjligheten för administratörer att blockera intervall av IP-adresser har stängts av.',
'ipb_expiry_invalid'              => 'Ogiltig varaktighetstid.',
'ipb_expiry_temp'                 => 'För att dölja användarnamnet måste blockeringen vara permanent.',
'ipb_already_blocked'             => '"$1" är redan blockerad',
'ipb_cant_unblock'                => 'Fel: Hittade inte blockering $1. Det är möjligt att den redan har upphävts.',
'ipb_blocked_as_range'            => 'Fel: IP-adressen $1 är inte direkt blockerad, och kan därför inte avblockeras. Adressen är blockerad som en del av IP-intervallet $2, som kan avblockeras.',
'ip_range_invalid'                => 'Ogiltigt IP-intervall.',
'blockme'                         => 'Blockera mig',
'proxyblocker'                    => 'Proxy-block',
'proxyblocker-disabled'           => 'Den här funktionen är avaktiverad.',
'proxyblockreason'                => 'Din IP-adress har blivit blockerad eftersom den tillhör en öppen proxy. Kontakta din internetleverantör eller din organisations eller företags tekniska support, och informera dem om denna allvarliga säkerhetsrisk.',
'proxyblocksuccess'               => 'Gjort.',
'sorbsreason'                     => 'Din IP-adress finns med på DNSBL:s lista över öppna proxies.',
'sorbs_create_account_reason'     => 'Din IP-adress finns med på listan över öppna proxyn, DNSBL, som används av {{SITENAME}}. Du kan därför inte skapa något användarkonto.',

# Developer tools
'lockdb'              => 'Lås databas',
'unlockdb'            => 'Lås upp databas',
'lockdbtext'          => 'En låsning av databasen hindrar alla användare från att redigera sidor, ändra inställningar och andra saker som kräver ändringar i databasen.
Bekräfta att du verkligen vill göra detta, och att du kommer att låsa upp databasen när underhållet är utfört.',
'unlockdbtext'        => 'Om du låser upp databasen kommer alla användare att åter kunna redigera sidor, ändra sina inställningar och så vidare. Bekräfta att du vill göra detta.',
'lockconfirm'         => 'Ja, jag vill verkligen låsa databasen.',
'unlockconfirm'       => 'Ja, jag vill låsa upp databasen.',
'lockbtn'             => 'Lås databasen',
'unlockbtn'           => 'Lås upp databasen',
'locknoconfirm'       => 'Du har inte bekräftat låsningen.',
'lockdbsuccesssub'    => 'Databasen har låsts',
'unlockdbsuccesssub'  => 'Databasen har låsts upp',
'lockdbsuccesstext'   => 'Databasen är nu låst.
<br />Kom ihåg att [[Special:UnlockDB|ta bort låsningen]] när du är färdig med ditt underhåll.',
'unlockdbsuccesstext' => 'Databasen är upplåst.',
'lockfilenotwritable' => 'Det går inte att skriva till databasens låsfil. För att låsa eller låsa upp databasen, så måste webbservern kunna skriva till den filen.',
'databasenotlocked'   => 'Databasen är inte låst.',

# Move page
'move-page'               => 'Flytta $1',
'move-page-legend'        => 'Flytta sida',
'movepagetext'            => "Med hjälp av formuläret härunder kan du byta namn på en sida, och flytta hela dess historik till ett nytt namn.
Den gamla sidtiteln kommer att göras om till en omdirigering till den nya titeln.
Du kan välja att automatiskt uppdatera omdirigeringar som leder till den gamla titeln.
Om du väljer att inte göra det, kontrollera då att du inte skapar några [[Special:DoubleRedirects|dubbla]] eller [[Special:BrokenRedirects|trasiga omdirigeringar]].
Du bör också se till att länkar fortsätter att peka dit de ska.

Notera att sidan '''inte''' kan flyttas om det redan finns en sida under den nya sidtiteln, såvida inte den sidan är tom eller en omdirigering till den gamla titeln och saknar annan versionshistorik.
Det innebär att du kan flytta tillbaks en sida om du råkar göra fel, och att du inte kan skriva över existerande sidor.

'''VARNING!'''
Att flytta en populär sida kan vara en drastisk och oväntad ändring;
därför bör du vara säker på att du förstår konsekvenserna innan du fortsätter med flytten.",
'movepagetalktext'        => "Diskussionssidan kommer att även den automatiskt flyttas '''om inte''':
*Det redan finns en diskussionssida som inte är tom med det nya namnet, eller
*Du avmarkerar rutan nedan.

I de fallen måste du flytta eller sammanfoga sidan manuellt, om det önskas.",
'movearticle'             => 'Flytta sidan:',
'movenotallowed'          => 'Du har inte behörighet att flytta sidor på den här wikin.',
'newtitle'                => 'Till nya titeln:',
'move-watch'              => 'Bevaka denna sida',
'movepagebtn'             => 'Flytta sidan',
'pagemovedsub'            => 'Flyttningen lyckades',
'movepage-moved'          => '<big>\'\'\'"$1" har flyttats till "$2"\'\'\'</big>', # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => 'Antingen existerar redan en sida med det namnet, eller så har du valt ett namn som inte är tillåtet.
Välj något annat namn istället.',
'cantmove-titleprotected' => 'Du kan inte flytta sidan till den titeln, eftersom den nya titeln har skyddats från att skapas.',
'talkexists'              => "'''Sidan flyttades, men diskussionssidan kunde inte flyttas eftersom det redan fanns en diskussionssida med det nya namnet.
Försök att sammanfoga dem manuellt.'''",
'movedto'                 => 'flyttad till',
'movetalk'                => 'Flytta tillhörande diskussionssida',
'move-subpages'           => 'Flytta alla undersidor, om det finns sådana',
'move-talk-subpages'      => 'Flytta alla undersidor av diskussionssidan, om det finns sådana',
'movepage-page-exists'    => 'Sidan $1 finns redan och kan inte skrivas över automatiskt.',
'movepage-page-moved'     => 'Sidan $1 har flyttats till $2.',
'movepage-page-unmoved'   => 'Sidan $1 kunde inte flyttas till $2.',
'movepage-max-pages'      => 'Gränsen på $1 {{PLURAL:$1|flyttad sida|flyttade sidor}} har uppnåtts och inga fler sidor kommer att flyttas automatiskt.',
'1movedto2'               => 'flyttade [[$1]] till [[$2]]',
'1movedto2_redir'         => 'flyttade [[$1]] till [[$2]], som var en omdirigeringssida',
'movelogpage'             => 'Sidflyttslogg',
'movelogpagetext'         => 'Listan nedan visar sidor som flyttats.',
'movereason'              => 'Anledning:',
'revertmove'              => 'flytta tillbaka',
'delete_and_move'         => 'Radera och flytta',
'delete_and_move_text'    => '==Radering krävs==
Den titel du vill flytta sidan till, "[[:$1]]", finns redan. Vill du radera den för att möjliggöra flytt av denna sida dit?',
'delete_and_move_confirm' => 'Ja, radera sidan',
'delete_and_move_reason'  => 'Raderad för att flytta hit en annan sida.',
'selfmove'                => 'Ursprungstitel och destinationstitel är identiska. Sidan kan inte flyttas till sig själv.',
'immobile_namespace'      => 'Namnrymden du försöker flytta sidan till eller från är av en speciell typ. Det går inte att flytta sidor till eller från den namnrymden.',
'imagenocrossnamespace'   => 'Kan inte flytta bilder till andra namnrymder än bildnamnrymden',
'imagetypemismatch'       => 'Den nya filändelsen motsvarar inte filtypen',
'imageinvalidfilename'    => 'Önskat filnamn är ogiltigt',
'fix-double-redirects'    => 'Uppdatera omdirigeringar som leder till den gamla titeln',

# Export
'export'            => 'Exportera sidor',
'exporttext'        => 'Du kan exportera text och versionshistorik för en eller flera sidor i XML-format.
Filen kan sedan importeras till en annan MediaWiki-wiki med hjälp av sidan [[Special:Import|importera]].

Exportera sidor genom att skriva in sidtitlarna i rutan här nedan.
Skriv en titel per rad och välj om du du vill exportera alla versioner av texten med sidhistorik, eller om du enbart vill exportera den nuvarande versionen med information om den senaste redigeringen.

I det senare fallet kan du även använda en länk, exempel [[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]] för sidan "[[{{MediaWiki:Mainpage}}]]".',
'exportcuronly'     => 'Inkludera endast den nuvarande versionen, inte hela historiken',
'exportnohistory'   => "----
'''OBS:''' export av fullständig sidhistorik med hjälp av detta formulär har stängts av på grund av prestandaskäl.",
'export-submit'     => 'Exportera',
'export-addcattext' => 'Lägg till sidor från kategori:',
'export-addcat'     => 'Lägg till',
'export-download'   => 'Ladda ner som fil',
'export-templates'  => 'Inkludera mallar',

# Namespace 8 related
'allmessages'               => 'Systemmeddelanden',
'allmessagesname'           => 'Namn',
'allmessagesdefault'        => 'Standardtext',
'allmessagescurrent'        => 'Nuvarande text',
'allmessagestext'           => 'Detta är en lista över alla meddelanden i namnrymden MediaWiki.
Besök [http://www.mediawiki.org/wiki/Localisation MediaWiki Localisation] eller [http://translatewiki.net Betawiki] om du vill bidra till översättningen av MediaWiki.',
'allmessagesnotsupportedDB' => "Den här sidan kan inte användas eftersom '''\$wgUseDatabaseMessages''' är avstängd.",
'allmessagesfilter'         => 'Filter för meddelandenamn:',
'allmessagesmodified'       => 'Visa bara ändrade',

# Thumbnails
'thumbnail-more'           => 'Förstora',
'filemissing'              => 'Fil saknas',
'thumbnail_error'          => 'Ett fel uppstod när minibilden skulle skapas: $1',
'djvu_page_error'          => 'DjVu-sida utanför gränserna',
'djvu_no_xml'              => 'Kan inte hämta DjVu-filens XML',
'thumbnail_invalid_params' => 'Ogiltiga parametrar för miniatyrbilden',
'thumbnail_dest_directory' => 'Kan inte skapa målkatalogen',

# Special:Import
'import'                     => 'Importera sidor',
'importinterwiki'            => 'Transwiki-import',
'import-interwiki-text'      => 'Välj en wiki och sidtitel att importera.
Versionshistorik (datum och redaktörer) kommer att bevaras.
All överföring mellan wikier (transwiki) listas i  [[Special:Log/import|importloggen]].',
'import-interwiki-history'   => 'Kopiera hela versionshistoriken för denna sida',
'import-interwiki-submit'    => 'Importera',
'import-interwiki-namespace' => 'Överför sidorna till namnrymden:',
'importtext'                 => 'Exportera filen från ursprungs-wikin genom att använda [[Special:Export|exportverktyget]], spara den till din hårddisk och ladda upp den här.',
'importstart'                => 'Importerar sidor....',
'import-revision-count'      => '$1 {{PLURAL:$1|version|versioner}}',
'importnopages'              => 'Det finns inga sidor att importera.',
'importfailed'               => 'Importen misslyckades: $1',
'importunknownsource'        => 'Okänd typ av importkälla',
'importcantopen'             => 'Misslyckades med att öppna importfilen.',
'importbadinterwiki'         => 'Felaktig interwiki-länk',
'importnotext'               => 'Tom eller ingen text',
'importsuccess'              => 'Importen är genomförd!',
'importhistoryconflict'      => 'Det föreligger en konflikt i versionshistoriken (kanske har denna sida importerats tidigare)',
'importnosources'            => 'Inga källor för transwiki-import har angivits, och direkt uppladdning av historik har stängts av.',
'importnofile'               => 'Ingen fil att importera har laddats upp.',
'importuploaderrorsize'      => 'Uppladdningen av importfilen misslyckades. Filen är större än vad som är tillåtet att ladda upp.',
'importuploaderrorpartial'   => 'Uppladdningen av importfilen misslyckades. Bara en del av filen laddades upp.',
'importuploaderrortemp'      => 'Uppladdningen av importfilen misslyckades. En temporär katalog saknas.',
'import-parse-failure'       => 'Tolkningsfel vid XML-import',
'import-noarticle'           => 'Inga sidor att importera!',
'import-nonewrevisions'      => 'Alla sidversioner hade importerats tidigare.',
'xml-error-string'           => '$1 på rad $2, kolumn $3 (byte $4): $5',
'import-upload'              => 'Ladda upp XML-data',

# Import log
'importlogpage'                    => 'Importlogg',
'importlogpagetext'                => 'Administrativa sidimporter med versionshistorik från andra wikier.',
'import-logentry-upload'           => 'importerade [[$1]] genom filuppladdning',
'import-logentry-upload-detail'    => '$1 {{PLURAL:$1|version|versioner}}',
'import-logentry-interwiki'        => 'överförde $1 mellan wikier',
'import-logentry-interwiki-detail' => '$1 {{PLURAL:$1|version|versioner}} från $2',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'Min användarsida',
'tooltip-pt-anonuserpage'         => 'Användarsida för ip-numret du redigerar från',
'tooltip-pt-mytalk'               => 'Min diskussionssida',
'tooltip-pt-anontalk'             => 'Diskussion om redigeringar från det här ip-numret',
'tooltip-pt-preferences'          => 'Mina inställningar',
'tooltip-pt-watchlist'            => 'Listan över sidor du bevakar för ändringar',
'tooltip-pt-mycontris'            => 'Lista över mina bidrag',
'tooltip-pt-login'                => 'Du får gärna logga in, men det är inte nödvändigt',
'tooltip-pt-anonlogin'            => 'Du får gärna logga in, men det är inte nödvändigt',
'tooltip-pt-logout'               => 'Logga ut',
'tooltip-ca-talk'                 => 'Diskussion om sidans innehåll',
'tooltip-ca-edit'                 => 'Du kan redigera den här sidan. Var vänlig förhandsgranska innan du sparar.',
'tooltip-ca-addsection'           => 'Lägg till en kommentar i den här diskussionen',
'tooltip-ca-viewsource'           => 'Den här sidan är skrivskyddad. Du kan se källtexten.',
'tooltip-ca-history'              => 'Tidigare versioner av sidan',
'tooltip-ca-protect'              => 'Skydda den här sidan',
'tooltip-ca-delete'               => 'Radera denna sida',
'tooltip-ca-undelete'             => 'Återställ alla redigeringar som gjorts innan sidan raderades',
'tooltip-ca-move'                 => 'Flytta den här sidan',
'tooltip-ca-watch'                => 'Lägg till sidan på din bevakningslista',
'tooltip-ca-unwatch'              => 'Ta bort denna sida från din bevakningslista',
'tooltip-search'                  => 'Sök på {{SITENAME}}',
'tooltip-search-go'               => 'Gå till sidan med detta namn om den finns',
'tooltip-search-fulltext'         => 'Sök efter sidor som innehåller denna text',
'tooltip-p-logo'                  => 'Huvudsida',
'tooltip-n-mainpage'              => 'Gå till huvudsidan',
'tooltip-n-portal'                => 'Om projektet, vad du kan göra, var man kan hitta saker',
'tooltip-n-currentevents'         => 'Information om aktuella händelser',
'tooltip-n-recentchanges'         => 'Listan över senaste ändringar i wikin.',
'tooltip-n-randompage'            => 'Gå till en slumpmässigt vald sida',
'tooltip-n-help'                  => 'Hjälp och information.',
'tooltip-t-whatlinkshere'         => 'Lista över alla sidor på {{SITENAME}} som länkar hit',
'tooltip-t-recentchangeslinked'   => 'Visa senaste ändringarna av sidor som den här sidan länkar till',
'tooltip-feed-rss'                => 'RSS-matning för den här sidan',
'tooltip-feed-atom'               => 'Atom-matning för den här sidan',
'tooltip-t-contributions'         => 'Visa lista över bidrag från den här användaren',
'tooltip-t-emailuser'             => 'Skicka e-post till den här användaren',
'tooltip-t-upload'                => 'Ladda upp filer',
'tooltip-t-specialpages'          => 'Lista över alla specialsidor',
'tooltip-t-print'                 => 'Utskriftvänlig version av den här sidan',
'tooltip-t-permalink'             => 'Permanent länk till den här versionen av sidan',
'tooltip-ca-nstab-main'           => 'Visa sidan',
'tooltip-ca-nstab-user'           => 'Visa användarsidan',
'tooltip-ca-nstab-media'          => 'Visa mediesidan',
'tooltip-ca-nstab-special'        => 'Detta är en specialsida; specialsidor kan inte redigeras',
'tooltip-ca-nstab-project'        => 'Visa projektsidan',
'tooltip-ca-nstab-image'          => 'Se filsidan',
'tooltip-ca-nstab-mediawiki'      => 'Se systemmeddelandet',
'tooltip-ca-nstab-template'       => 'Se mallen',
'tooltip-ca-nstab-help'           => 'Se hjälpsidan',
'tooltip-ca-nstab-category'       => 'Se kategorisidan',
'tooltip-minoredit'               => 'Markera som mindre ändring',
'tooltip-save'                    => 'Spara dina ändringar',
'tooltip-preview'                 => 'Det är bra om du förhandsgranskar dina ändringar innan du sparar!',
'tooltip-diff'                    => 'Visa vilka förändringar du har gjort av texten.',
'tooltip-compareselectedversions' => 'Visa skillnaden mellan de två markerade versionerna av den här sidan.',
'tooltip-watch'                   => 'Lägg till den här sidan i din bevakningslista',
'tooltip-recreate'                => 'Återskapa sidan fast den har tagits bort',
'tooltip-upload'                  => 'Starta uppladdning',

# Stylesheets
'common.css'      => '/* CSS som skrivs här påverkar alla skal */',
'standard.css'    => '/* CSS som skrivs här kommer att påverka alla användare av skalet Standard */',
'nostalgia.css'   => '/* CSS som skrivs här kommer att påverka alla användare av skalet Nostalgi */',
'cologneblue.css' => '/* CSS som skrivs här kommer att påverka alla användare av skalet Cologne blå */',
'monobook.css'    => '/* CSS som skrivs här kommer att påverka alla användare av skalet Monobook */',
'myskin.css'      => '/* CSS som skrivs här kommer att påverka alla användare av skalet Mitt skal */',
'chick.css'       => '/* CSS som skrivs här kommer att påverka alla användare av skalet Chick */',
'simple.css'      => '/* CSS som skrivs här kommer att påverka alla användare av skalet Enkelt */',
'modern.css'      => '/* CSS som skrivs här kommer att påverka alla användare av skalet Modern */',

# Scripts
'common.js'   => '/* JavaScript som skrivs här körs varje gång en användare laddar en sida. */',
'monobook.js' => '/* Javascript härifrån laddas endast för användare som använder Monobook-utseendet */',

# Metadata
'nodublincore'      => 'Dublin Core RDF metadata avstängt på den här servern.',
'nocreativecommons' => 'Creative Commons RDF metadata avstängd på denna server.',
'notacceptable'     => 'Den här wiki-servern kan inte erbjuda data i ett format som din klient kan läsa.',

# Attribution
'anonymous'        => 'Anonym användare på {{SITENAME}}',
'siteuser'         => 'användaren $1 på {{SITENAME}}',
'lastmodifiedatby' => 'Den här sidan ändrades senast kl. $2 den $1 av $3.', # $1 date, $2 time, $3 user
'othercontribs'    => 'Baserad på arbete av $1.',
'others'           => 'andra',
'siteusers'        => '{{SITENAME}} användare $1',
'creditspage'      => 'Användare som bidragit till sidan',
'nocredits'        => 'Det finns ingen information tillgänglig om vem som bidragit till denna sida.',

# Spam protection
'spamprotectiontitle' => 'Spamfilter',
'spamprotectiontext'  => 'Sidan du ville spara blockerades av spamfiltret.
Detta orsakades troligen av en länk till en svartlistad webbplats.',
'spamprotectionmatch' => 'Följande text aktiverade vårt spamfilter: $1',
'spambot_username'    => 'MediaWikis spampatrull',
'spam_reverting'      => 'Återställer till den senaste versionen som inte innehåller länkar till $1',
'spam_blanking'       => 'Alla versioner innehöll en länk till $1, blankar',

# Info page
'infosubtitle'   => 'Information om sida',
'numedits'       => 'Antal redigeringar (sida): $1',
'numtalkedits'   => 'Antal redigeringar (diskussionssida): $1',
'numwatchers'    => 'Antal användare som bevakar sidan: $1',
'numauthors'     => 'Antal olika bidragsgivare (sida): $1',
'numtalkauthors' => 'Antal olika bidragsgivare (diskussionssida): $1',

# Math options
'mw_math_png'    => 'Rendera alltid PNG',
'mw_math_simple' => 'HTML om mycket enkel, annars PNG',
'mw_math_html'   => 'HTML om möjligt, annars PNG',
'mw_math_source' => 'Låt vara TeX (för textbaserade webbläsare)',
'mw_math_modern' => 'Rekommenderat för modern webbläsare',
'mw_math_mathml' => 'MathML om möjligt (experimentellt)',

# Patrolling
'markaspatrolleddiff'                 => 'Märk som patrullerad',
'markaspatrolledtext'                 => 'Märk den här sidan som patrullerad',
'markedaspatrolled'                   => 'Markerad som patrullerad',
'markedaspatrolledtext'               => 'Den valda versionen har märkts som patrullerad.',
'rcpatroldisabled'                    => 'Patrullering av Senaste ändringar är avstängd.',
'rcpatroldisabledtext'                => 'Funktionen "patrullering av Senaste ändringar" är tillfälligt avstängd.',
'markedaspatrollederror'              => 'Kan inte markera som patrullerad',
'markedaspatrollederrortext'          => 'Det går inte att markera som patrullerad utan att ange version.',
'markedaspatrollederror-noautopatrol' => 'Du har inte tillåtelse att markera dina egna redigeringar som patrullerade.',

# Patrol log
'patrol-log-page'   => 'Patrulleringslogg',
'patrol-log-header' => 'Detta är en logg över patrullerade sidversioner.',
'patrol-log-line'   => 'markerade $1 av $2 som patrullerad $3',
'patrol-log-auto'   => '(automatiskt)',
'patrol-log-diff'   => 'version $1',

# Image deletion
'deletedrevision'                 => 'Raderade gammal sidversion $1',
'filedeleteerror-short'           => 'Fel vid radering av fil: $1',
'filedeleteerror-long'            => 'Fel inträffade vid raderingen av filen:

$1',
'filedelete-missing'              => 'Filen "$1" kan inte raderas eftersom den inte finns.',
'filedelete-old-unregistered'     => 'Den angivna filversionen "$1" finns inte i databasen.',
'filedelete-current-unregistered' => 'Den angivna filen "$1" finns inte i databasen.',
'filedelete-archive-read-only'    => 'Webbservern kan inte skriva till arkivkatalogen "$1".',

# Browsing diffs
'previousdiff' => '← Äldre redigering',
'nextdiff'     => 'Nyare redigering →',

# Media information
'mediawarning'         => "'''Varning:''': Denna fil kan innehålla programkod som, om den körs, kan skada din dator.",
'imagemaxsize'         => 'Begränsa bilders storlek på filbeskrivningssidor till:',
'thumbsize'            => 'Storlek på minibild:',
'widthheightpage'      => '$1×$2, $3 {{PLURAL:$3|sida|sidor}}',
'file-info'            => '(filstorlek: $1, MIME-typ: $2)',
'file-info-size'       => '($1 × $2 pixel, filstorlek: $3, MIME-typ: $4)',
'file-nohires'         => '<small>Det finns ingen version med högre upplösning.</small>',
'svg-long-desc'        => '(SVG-fil, grundstorlek: $1 × $2 pixel, filstorlek: $3)',
'show-big-image'       => 'Högupplöst version',
'show-big-image-thumb' => '<small>Storlek på förhandsvisningen: $1 × $2 pixel</small>',

# Special:NewImages
'newimages'             => 'Galleri över nya filer',
'imagelisttext'         => 'Nedan finns en lista med <strong>$1</strong> {{PLURAL:$1|bild|bilder}} sorterad <strong>$2</strong>.',
'newimages-summary'     => 'Den här specialsidan visar de senast uppladdade filerna.',
'showhidebots'          => '($1 robotar)',
'noimages'              => 'Ingenting att se.',
'ilsubmit'              => 'Sök',
'bydate'                => 'efter datum',
'sp-newimages-showfrom' => 'Visa nya filer från och med kl. $2 den $1',

# Bad image list
'bad_image_list' => 'Listan fungerar enligt följande:

Listan tar enbart hänsyn till rader som börjar med asterisk (*). 
Den första länken på en rad måste vara en länk till en otillåten fil.
Övriga länkar på samma rad kommer att hanteras som undantag, det vill säga sidor där filen tillåts användas.',

# Metadata
'metadata'          => 'Metadata',
'metadata-help'     => 'Den här filen innehåller extrainformation som troligen lades till av en digitalkamera eller skanner när filen skapades. Om filen har modifierats kan det hända att vissa detaljer inte överensstämmer med den modifierade filen.',
'metadata-expand'   => 'Visa utökade detaljer',
'metadata-collapse' => 'Dölj utökade detaljer',
'metadata-fields'   => 'EXIF-fält som listas i det här meddelandet visas på bildsidan när metadatatabellen är minimerad.
Övriga fält är gömda som standard, men visas när tabellen expanderas.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => 'Bredd',
'exif-imagelength'                 => 'Höjd',
'exif-bitspersample'               => 'Bitar per komponent',
'exif-compression'                 => 'Komprimeringsalgoritm',
'exif-photometricinterpretation'   => 'Pixelsammansättning',
'exif-orientation'                 => 'Orientering',
'exif-samplesperpixel'             => 'Antal komponenter',
'exif-planarconfiguration'         => 'Dataarrangemang',
'exif-ycbcrsubsampling'            => 'Subsamplingsförhållande mellan Y och C',
'exif-ycbcrpositioning'            => 'Positionering av Y och C',
'exif-xresolution'                 => 'Upplösning i horisontalplan',
'exif-yresolution'                 => 'Upplösning i vertikalplan',
'exif-resolutionunit'              => 'Enhet för upplösning av X och Y',
'exif-stripoffsets'                => 'Offset till bilddata',
'exif-rowsperstrip'                => 'Antal rader per strip',
'exif-stripbytecounts'             => 'Byte per komprimerad strip',
'exif-jpeginterchangeformat'       => 'Offset till JPEG SOI',
'exif-jpeginterchangeformatlength' => 'Antal bytes JPEG-data',
'exif-transferfunction'            => 'Överföringsfunktion',
'exif-whitepoint'                  => 'Vitpunktens renhet',
'exif-primarychromaticities'       => 'Primärfärgernas renhet',
'exif-ycbcrcoefficients'           => 'Koefficienter för färgrymdstransformationsmatris',
'exif-referenceblackwhite'         => 'Referensvärden för svart och vitt',
'exif-datetime'                    => 'Ändringstidpunkt',
'exif-imagedescription'            => 'Bildtitel',
'exif-make'                        => 'Kameratillverkare',
'exif-model'                       => 'Kameramodell',
'exif-software'                    => 'Använd mjukvara',
'exif-artist'                      => 'Skapare',
'exif-copyright'                   => 'Upphovsrättsägare',
'exif-exifversion'                 => 'Exif-version',
'exif-flashpixversion'             => 'Flashpix-version som stöds',
'exif-colorspace'                  => 'Färgrymd',
'exif-componentsconfiguration'     => 'Komponentanalys',
'exif-compressedbitsperpixel'      => 'Bildkomprimeringsläge',
'exif-pixelydimension'             => 'Giltig bildbredd',
'exif-pixelxdimension'             => 'Giltig bildhöjd',
'exif-makernote'                   => 'Tillverkarkommentarer',
'exif-usercomment'                 => 'Kommentarer',
'exif-relatedsoundfile'            => 'Relaterad ljudfil',
'exif-datetimeoriginal'            => 'Exponeringstidpunkt',
'exif-datetimedigitized'           => 'Tidpunkt för digitalisering',
'exif-subsectime'                  => 'Ändringstidpunkt, sekunddelar',
'exif-subsectimeoriginal'          => 'Exponeringstidpunkt, sekunddelar',
'exif-subsectimedigitized'         => 'Digitaliseringstidpunkt, sekunddelar',
'exif-exposuretime'                => 'Exponeringstid',
'exif-exposuretime-format'         => '$1 sek ($2)',
'exif-fnumber'                     => 'F-nummer',
'exif-exposureprogram'             => 'Exponeringsprogram',
'exif-spectralsensitivity'         => 'Spektral känslighet',
'exif-isospeedratings'             => 'Filmhastighet (ISO)',
'exif-oecf'                        => 'Optoelektronisk konversionsfaktor',
'exif-shutterspeedvalue'           => 'Slutarhastighet',
'exif-aperturevalue'               => 'Bländare',
'exif-brightnessvalue'             => 'Ljusstyrka',
'exif-exposurebiasvalue'           => 'Exponeringsbias',
'exif-maxaperturevalue'            => 'Maximal bländare',
'exif-subjectdistance'             => 'Avstånd till motivet',
'exif-meteringmode'                => 'Mätmetod',
'exif-lightsource'                 => 'Ljuskälla',
'exif-flash'                       => 'Blixt',
'exif-focallength'                 => 'Linsens brännvidd',
'exif-subjectarea'                 => 'Motivområde',
'exif-flashenergy'                 => 'Blixteffekt',
'exif-spatialfrequencyresponse'    => 'Rumslig frekvensrespons',
'exif-focalplanexresolution'       => 'Upplösning i fokalplan x',
'exif-focalplaneyresolution'       => 'Upplösning i fokalplan y',
'exif-focalplaneresolutionunit'    => 'Enhet för upplösning i fokalplan',
'exif-subjectlocation'             => 'Motivets läge',
'exif-exposureindex'               => 'Exponeringsindex',
'exif-sensingmethod'               => 'Avkänningsmetod',
'exif-filesource'                  => 'Filkälla',
'exif-scenetype'                   => 'Scentyp',
'exif-cfapattern'                  => 'CFA-mönster',
'exif-customrendered'              => 'Anpassad bildbehandling',
'exif-exposuremode'                => 'Exponeringsläge',
'exif-whitebalance'                => 'Vitbalans',
'exif-digitalzoomratio'            => 'Digitalt zoomomfång',
'exif-focallengthin35mmfilm'       => 'Brännvidd på 35 mm film',
'exif-scenecapturetype'            => 'Motivprogram',
'exif-gaincontrol'                 => 'Bildförstärkning',
'exif-contrast'                    => 'Kontrast',
'exif-saturation'                  => 'Mättnad',
'exif-sharpness'                   => 'Skärpa',
'exif-devicesettingdescription'    => 'Beskrivning av apparatens inställning',
'exif-subjectdistancerange'        => 'Avståndsintervall till motiv',
'exif-imageuniqueid'               => 'Unikt bild-ID',
'exif-gpsversionid'                => 'Version för GPS-taggar',
'exif-gpslatituderef'              => 'Nordlig eller sydlig latitud',
'exif-gpslatitude'                 => 'Latitud',
'exif-gpslongituderef'             => 'Östlig eller västlig longitud',
'exif-gpslongitude'                => 'Longitud',
'exif-gpsaltituderef'              => 'Referenshöjd',
'exif-gpsaltitude'                 => 'Höjd',
'exif-gpstimestamp'                => 'GPS-tid (atomur)',
'exif-gpssatellites'               => 'Satelliter använda för mätning',
'exif-gpsstatus'                   => 'Mottagarstatus',
'exif-gpsmeasuremode'              => 'Mätmetod',
'exif-gpsdop'                      => 'Mätnoggrannhet',
'exif-gpsspeedref'                 => 'Hastighetsenhet',
'exif-gpsspeed'                    => 'GPS-mottagarens hastighet',
'exif-gpstrackref'                 => 'Referenspunkt för rörelsens riktning',
'exif-gpstrack'                    => 'Rörelsens riktning',
'exif-gpsimgdirectionref'          => 'Referens för bildens riktning',
'exif-gpsimgdirection'             => 'Bildens riktning',
'exif-gpsmapdatum'                 => 'Använd geodetisk data',
'exif-gpsdestlatituderef'          => 'Referenspunkt för målets latitud',
'exif-gpsdestlatitude'             => 'Målets latitud',
'exif-gpsdestlongituderef'         => 'Referenspunkt för målets longitud',
'exif-gpsdestlongitude'            => 'Målets longitud',
'exif-gpsdestbearingref'           => 'Referens för riktning mot målet',
'exif-gpsdestbearing'              => 'Riktning mot målet',
'exif-gpsdestdistanceref'          => 'Referenspunkt för avstånd till målet',
'exif-gpsdestdistance'             => 'Avstånd till målet',
'exif-gpsprocessingmethod'         => 'GPS-behandlingsmetodens namn',
'exif-gpsareainformation'          => 'GPS-områdets namn',
'exif-gpsdatestamp'                => 'GPS-datum',
'exif-gpsdifferential'             => 'Differentiell GPS-korrektion',

# EXIF attributes
'exif-compression-1' => 'Inte komprimerad',

'exif-unknowndate' => 'Okänt datum',

'exif-orientation-1' => 'Normal', # 0th row: top; 0th column: left
'exif-orientation-2' => 'Spegelvänd horisontellt', # 0th row: top; 0th column: right
'exif-orientation-3' => 'Roterad 180°', # 0th row: bottom; 0th column: right
'exif-orientation-4' => 'Spegelvänd vertikalt', # 0th row: bottom; 0th column: left
'exif-orientation-5' => 'Roterad 90° moturs och spegelvänd vertikalt', # 0th row: left; 0th column: top
'exif-orientation-6' => 'Roterad 90° medurs', # 0th row: right; 0th column: top
'exif-orientation-7' => 'Roterad 90° medurs och spegelvänd vertikalt', # 0th row: right; 0th column: bottom
'exif-orientation-8' => 'Roterad 90° moturs', # 0th row: left; 0th column: bottom

'exif-planarconfiguration-1' => 'chunkformat',
'exif-planarconfiguration-2' => 'planärformat',

'exif-componentsconfiguration-0' => 'saknas',

'exif-exposureprogram-0' => 'Inte definierad',
'exif-exposureprogram-1' => 'Manuell inställning',
'exif-exposureprogram-2' => 'Normalprogram',
'exif-exposureprogram-3' => 'Prioritet för bländare',
'exif-exposureprogram-4' => 'Prioritet för slutare',
'exif-exposureprogram-5' => 'Konstnärligt program (prioriterar skärpedjup)',
'exif-exposureprogram-6' => 'Rörelseprogram (prioriterar kortare slutartid)',
'exif-exposureprogram-7' => 'Porträttläge (för närbilder med bakgrunden ofokuserad)',
'exif-exposureprogram-8' => 'Landskapsläge (för foton av landskap med bakgrunden i fokus)',

'exif-subjectdistance-value' => '$1 meter',

'exif-meteringmode-0'   => 'Okänd',
'exif-meteringmode-1'   => 'Medelvärde',
'exif-meteringmode-2'   => 'Centrumviktat medelvärde',
'exif-meteringmode-3'   => 'Spotmätning',
'exif-meteringmode-4'   => 'Multispot',
'exif-meteringmode-5'   => 'Mönster',
'exif-meteringmode-6'   => 'Partiell',
'exif-meteringmode-255' => 'Annan',

'exif-lightsource-0'   => 'Okänd',
'exif-lightsource-1'   => 'Dagsljus',
'exif-lightsource-2'   => 'Lysrör',
'exif-lightsource-3'   => 'Glödlampa',
'exif-lightsource-4'   => 'Blixt',
'exif-lightsource-9'   => 'Klart väder',
'exif-lightsource-10'  => 'Molnigt',
'exif-lightsource-11'  => 'Skugga',
'exif-lightsource-12'  => 'Dagsljuslysrör (D 5700 – 7100K)',
'exif-lightsource-13'  => 'Dagsvitt lysrör (N 4600 – 5400K)',
'exif-lightsource-14'  => 'Kallvitt lysrör (W 3900 – 4500K)',
'exif-lightsource-15'  => 'Vitt lysrör (WW 3200 – 3700K)',
'exif-lightsource-17'  => 'Standardljus A',
'exif-lightsource-18'  => 'Standardljus B',
'exif-lightsource-19'  => 'Standardljus C',
'exif-lightsource-24'  => 'ISO studiobelysning',
'exif-lightsource-255' => 'Annan ljuskälla',

'exif-focalplaneresolutionunit-2' => 'tum',

'exif-sensingmethod-1' => 'Ej angivet',
'exif-sensingmethod-2' => 'Enchipsfärgsensor',
'exif-sensingmethod-3' => 'Tvåchipsfärgsensor',
'exif-sensingmethod-4' => 'Trechipsfärgsensor',
'exif-sensingmethod-5' => 'Färgsekventiell områdessensor',
'exif-sensingmethod-7' => 'Trilinjär sensor',
'exif-sensingmethod-8' => 'Färgsekventiell linjär sensor',

'exif-scenetype-1' => 'Direkt fotograferad bild',

'exif-customrendered-0' => 'Normal',
'exif-customrendered-1' => 'Anpassad',

'exif-exposuremode-0' => 'Automatisk exponering',
'exif-exposuremode-1' => 'Manuell exponering',
'exif-exposuremode-2' => 'Automatisk alternativexponering',

'exif-whitebalance-0' => 'Automatisk vitbalans',
'exif-whitebalance-1' => 'Manuell vitbalans',

'exif-scenecapturetype-0' => 'Standard',
'exif-scenecapturetype-1' => 'Landskap',
'exif-scenecapturetype-2' => 'Porträtt',
'exif-scenecapturetype-3' => 'Nattfotografering',

'exif-gaincontrol-0' => 'Ingen',
'exif-gaincontrol-1' => 'Ökning av lågnivåförstärkning',
'exif-gaincontrol-2' => 'Ökning av högnivåförstärkning',
'exif-gaincontrol-3' => 'Sänkning av lågnivåförstärkning',
'exif-gaincontrol-4' => 'Sänkning av högnivåförstärkning',

'exif-contrast-0' => 'Normal',
'exif-contrast-1' => 'Mjuk',
'exif-contrast-2' => 'Skarp',

'exif-saturation-0' => 'Normal',
'exif-saturation-1' => 'Låg mättnadsgrad',
'exif-saturation-2' => 'Hög mättnadsgrad',

'exif-sharpness-0' => 'Normal',
'exif-sharpness-1' => 'Mjuk',
'exif-sharpness-2' => 'Hård',

'exif-subjectdistancerange-0' => 'Okänd',
'exif-subjectdistancerange-1' => 'Makro',
'exif-subjectdistancerange-2' => 'Närbild',
'exif-subjectdistancerange-3' => 'Avståndsbild',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'Nordlig latitud',
'exif-gpslatitude-s' => 'Sydlig latitud',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'Östlig longitud',
'exif-gpslongitude-w' => 'Västlig longitud',

'exif-gpsstatus-a' => 'Mätning pågår',
'exif-gpsstatus-v' => 'Mätningsinteroperabilitet',

'exif-gpsmeasuremode-2' => 'Tvådimensionell mätning',
'exif-gpsmeasuremode-3' => 'Tredimensionell mätning',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'Kilometer i timmen',
'exif-gpsspeed-m' => 'Miles i timmen',
'exif-gpsspeed-n' => 'Knop',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => 'Sann bäring',
'exif-gpsdirection-m' => 'Magnetisk bäring',

# External editor support
'edit-externally'      => 'Redigera denna fil med hjälp av extern programvara',
'edit-externally-help' => 'Se [http://www.mediawiki.org/wiki/Manual:External_editors instruktioner] för mer information.',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'alla',
'imagelistall'     => 'alla',
'watchlistall2'    => 'alla',
'namespacesall'    => 'alla',
'monthsall'        => 'alla',

# E-mail address confirmation
'confirmemail'             => 'Bekräfta e-postadress',
'confirmemail_noemail'     => 'Du har inte angivit någon giltig e-postadress i dina [[Special:Preferences|inställningar]].',
'confirmemail_text'        => 'Innan du kan använda {{SITENAME}}s funktioner för e-post måste du bekräfta din e-postadress. Aktivera knappen nedan för att skicka en bekräftelsekod till din e-postadress. Mailet kommer att innehålla en länk, som innehåller en kod. Genom att klicka på den länken eller kopiera den till din webbläsares fönster för webbadresser, bekräftar du att din e-postadress fungerar.',
'confirmemail_pending'     => 'En bekräftelsekod har redan skickats till din epostadress. Om du skapade ditt konto nyligen, så kanske du vill vänta några minuter innan du begär en ny kod.',
'confirmemail_send'        => 'Skicka bekräftelsekod',
'confirmemail_sent'        => 'E-post med bekräftelse skickat.',
'confirmemail_oncreate'    => 'En bekräftelsekod skickades till din epostadress. Koden behövs inte för att logga in, men du behöver koden för att få tillgång till de epostbaserade funktionerna på wikin.',
'confirmemail_sendfailed'  => '{{SITENAME}} kunde inte skicka din e-postbekräftelse.
Kontrollera om e-postadressen innehåller ogiltiga tecken.

Mailservern svarade: $1',
'confirmemail_invalid'     => 'Ogiltig bekräftelsekod. Dess giltighetstid kan ha löpt ut.',
'confirmemail_needlogin'   => 'Du behöver $1 för att bekräfta din e-postadress',
'confirmemail_success'     => 'Din e-postadress har bekräftats. 
Du kan nu [[Special:UserLogin|logga in]] och använda wikin.',
'confirmemail_loggedin'    => 'Din e-postadress är nu bekräftad.',
'confirmemail_error'       => 'Någonting gick fel när din bekräftelse skulle sparas.',
'confirmemail_subject'     => 'Bekräftelse av e-postadress på {{SITENAME}}',
'confirmemail_body'        => 'Någon, troligen du, har från IP-adressen $1 registrerat användarkontot "$2" med denna e-postadress på {{SITENAME}}.

För att bekräfta att detta konto verkligen är ditt, och för att aktivera funktionerna för e-post på {{SITENAME}}, öppna denna länk i din webbläsare:

$3

Om det *inte* är du som registrerat kontot, följ denna länk för att avbryta bekräftelsen av e-postadressen:

$5

Denna bekräftelsekod kommer inte att fungera efter $4.',
'confirmemail_invalidated' => 'Bekräftelsen av e-postadressen har ogiltigförklarats',
'invalidateemail'          => 'Avbryt bekräftelse av e-postadress',

# Scary transclusion
'scarytranscludedisabled' => '[Interwiki-inklusion är inte aktiverad]',
'scarytranscludefailed'   => '[Hämtning av mall för $1 misslyckades]',
'scarytranscludetoolong'  => '[För lång URL]',

# Trackbacks
'trackbackbox'      => '<div id="mw_trackbacks"> Till denna sida finns följande trackback:<br /> $1 </div>',
'trackbackremove'   => '([$1 Ta bort])',
'trackbacklink'     => 'Trackback',
'trackbackdeleteok' => 'Trackback har tagits bort.',

# Delete conflict
'deletedwhileediting' => "'''Varning''': Denna sida raderades efter att du började redigera!",
'confirmrecreate'     => "Användaren [[User:$1|$1]] ([[User talk:$1|diskussion]]) raderade den här sidan efter att du började redigera den med motiveringen:
: ''$2''
Bekräfta att du verkligen vill återskapa sidan.",
'recreate'            => 'Återskapa',

# HTML dump
'redirectingto' => 'Omdirigerar till [[:$1]]...',

# action=purge
'confirm_purge'        => 'Rensa denna sidas cache?

$1',
'confirm_purge_button' => 'OK',

# AJAX search
'searchcontaining' => "Leta efter sidor som innehåller ''$1''.",
'searchnamed'      => "Leta efter sidor som heter ''$1''.",
'articletitles'    => "Sidor som börjar med ''$1''",
'hideresults'      => 'Göm resultat',
'useajaxsearch'    => 'Använd AJAX-sökning',

# Multipage image navigation
'imgmultipageprev' => '← föregående sida',
'imgmultipagenext' => 'nästa sida →',
'imgmultigo'       => 'Gå',
'imgmultigoto'     => 'Gå till sida $1',

# Table pager
'ascending_abbrev'         => 'stigande',
'descending_abbrev'        => 'fallande',
'table_pager_next'         => 'Nästa sida',
'table_pager_prev'         => 'Föregående sida',
'table_pager_first'        => 'Första sidan',
'table_pager_last'         => 'Sista sidan',
'table_pager_limit'        => 'Visa $1 poster per sida',
'table_pager_limit_submit' => 'Utför',
'table_pager_empty'        => 'Inga resultat',

# Auto-summaries
'autosumm-blank'   => 'Tar bort sidans innehåll',
'autosumm-replace' => "Ersätter sidans innehåll med '$1'",
'autoredircomment' => 'Omdirigerar till [[$1]]',
'autosumm-new'     => 'Ny sida: $1',

# Size units
'size-bytes'     => '$1 byte',
'size-kilobytes' => '$1 kbyte',
'size-megabytes' => '$1 Mbyte',
'size-gigabytes' => '$1 Gbyte',

# Live preview
'livepreview-loading' => 'Laddar…',
'livepreview-ready'   => 'Laddar… Färdig!',
'livepreview-failed'  => 'Live preview misslyckades!
Pröva vanlig förhandsgranskning istället.',
'livepreview-error'   => 'Lyckades inte ansluta: $1 "$2"
Pröva vanlig förhandsgranskning istället.',

# Friendlier slave lag warnings
'lag-warn-normal' => 'Ändringar under {{PLURAL:$1|den senaste sekunden|de $1 senaste sekunderna}} kanske inte visas i den här listan.',
'lag-warn-high'   => 'På grund av stor fördröjning i databasen, så visas kanske inte ändringar nyare än $1 {{PLURAL:$1|sekund|sekunder}} i den här listan.',

# Watchlist editor
'watchlistedit-numitems'       => 'Din bevakningslista innehåller {{PLURAL:$1|1 sida|$1 sidor}}, utöver diskussionsidor.',
'watchlistedit-noitems'        => 'Din bevakningslista innehåller inga sidor.',
'watchlistedit-normal-title'   => 'Redigera bevakningslista',
'watchlistedit-normal-legend'  => 'Ta bort sidor från bevakningslistan',
'watchlistedit-normal-explain' => 'Sidorna i din bevakningslista visas nedan.
För att ta bort en sida, kryssa i rutan intill den, och tryck på "Ta bort sidor".
Du kan även [[Special:Watchlist/raw|redigera listan i råformat]].',
'watchlistedit-normal-submit'  => 'Ta bort sidor',
'watchlistedit-normal-done'    => '{{PLURAL:$1|1 sida|$1 sidor}} togs bort från din bevakningslista:',
'watchlistedit-raw-title'      => 'Redigera bevakningslistan i råformat',
'watchlistedit-raw-legend'     => 'Redigera bevakningslistan i råformat',
'watchlistedit-raw-explain'    => 'Sidorna i din bevakningslista visas här nedanför, och kan redigeras genom att lägga till och ta bort från listan;
en sida per rad.
Tryck på "Uppdatera bevakningslista", när du är färdig.
Du kan också [[Special:Watchlist/edit|använda standardeditorn]].',
'watchlistedit-raw-titles'     => 'Sidor:',
'watchlistedit-raw-submit'     => 'Uppdatera bevakningslistan',
'watchlistedit-raw-done'       => 'Din bevakningslista har uppdaterats.',
'watchlistedit-raw-added'      => '{{PLURAL:$1|1 sida|$1 sidor}} lades till:',
'watchlistedit-raw-removed'    => '{{PLURAL:$1|1 sida|$1 sidor}} togs bort:',

# Watchlist editing tools
'watchlisttools-view' => 'Visa relevanta ändringar',
'watchlisttools-edit' => 'Visa och redigera bevakningslistan',
'watchlisttools-raw'  => 'Redigera bevakningslistan i råformat',

# Core parser functions
'unknown_extension_tag' => 'Okänd tagg "$1"',

# Special:Version
'version'                          => 'Version', # Not used as normal message but as header for the special page itself
'version-extensions'               => 'Installerade programtillägg',
'version-specialpages'             => 'Specialsidor',
'version-parserhooks'              => 'Parsertillägg',
'version-variables'                => 'Variabler',
'version-other'                    => 'Annat',
'version-mediahandlers'            => 'Mediahanterare',
'version-hooks'                    => 'Hakar',
'version-extension-functions'      => 'Tilläggsfunktioner',
'version-parser-extensiontags'     => 'Tilläggstaggar',
'version-parser-function-hooks'    => 'Parserfunktioner',
'version-skin-extension-functions' => 'Skaltilläggsfunktioner',
'version-hook-name'                => 'Namn',
'version-hook-subscribedby'        => 'Används av',
'version-version'                  => 'Version',
'version-license'                  => 'Licens',
'version-software'                 => 'Installerad programvara',
'version-software-product'         => 'Produkt',
'version-software-version'         => 'Version',

# Special:FilePath
'filepath'         => 'Sökväg till fil',
'filepath-page'    => 'Fil:',
'filepath-submit'  => 'Sökväg',
'filepath-summary' => 'Den här sidan ger den fullständiga sökvägen till en fil. Bilder visas i full upplösning i din webbläsare, andra filtyper öppnas direkt i de program som är associerade till dem.

Ange filens namn utan prefixet "{{ns:image}}:".',

# Special:FileDuplicateSearch
'fileduplicatesearch'          => 'Sök efter dubblettfiler',
'fileduplicatesearch-summary'  => 'Sök efter dubblettfiler baserat på filernas hash-värden.

Skriv filnamnet utan prefixet "{{ns:image}}:" .',
'fileduplicatesearch-legend'   => 'Sök efter en dubblettfil',
'fileduplicatesearch-filename' => 'Filnamn:',
'fileduplicatesearch-submit'   => 'Sök',
'fileduplicatesearch-info'     => '$1 × $2 pixel<br />Filstorlek: $3<br />MIME-typ: $4',
'fileduplicatesearch-result-1' => 'Filen "$1" har inga identiska dubbletter.',
'fileduplicatesearch-result-n' => 'Filen "$1" har {{PLURAL:$2|1 identisk dubblett|$2 identiska dubbletter}}.',

# Special:SpecialPages
'specialpages'                   => 'Specialsidor',
'specialpages-note'              => '----
* Normala specialsidor.
* <span class="mw-specialpagerestricted">Specialsidor med begränsad åtkomst.</span>',
'specialpages-group-maintenance' => 'Underhållsrapporter',
'specialpages-group-other'       => 'Övriga specialsidor',
'specialpages-group-login'       => 'Inloggning/registrering',
'specialpages-group-changes'     => 'Senaste ändringar och loggar',
'specialpages-group-media'       => 'Filer och uppladdning',
'specialpages-group-users'       => 'Användare och behörigheter',
'specialpages-group-highuse'     => 'Sidor som används mycket',
'specialpages-group-pages'       => 'Sidlistor',
'specialpages-group-pagetools'   => 'Sidverktyg',
'specialpages-group-wiki'        => 'Information och verktyg för wikin',
'specialpages-group-redirects'   => 'Omdirigerande specialsidor',
'specialpages-group-spam'        => 'Spamverktyg',

# Special:BlankPage
'blankpage'              => 'Tom sida',
'intentionallyblankpage' => 'Denna sida har avsiktligen lämnats tom.',

);
