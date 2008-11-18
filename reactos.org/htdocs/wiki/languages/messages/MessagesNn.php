<?php
/** Norwegian Nynorsk (‪Norsk (nynorsk)‬)
 *
 * @ingroup Language
 * @file
 *
 * @author Boivie
 * @author Dittaeva
 * @author Eirik
 * @author Finnrind
 * @author Frokor
 * @author Guttorm Flatabø
 * @author H92
 * @author Jon Harald Søby
 * @author Jorunn
 * @author Max sonnelid
 * @author Olve Utne
 * @author Ranveig
 * @author Shauni
 * @author לערי ריינהארט
 */

/**
  * @license http://www.gnu.org/copyleft/fdl.html GNU Free Documentation License
  * @license http://www.gnu.org/copyleft/gpl.html GNU General Public License
  *
  * @see http://meta.wikimedia.org/w/index.php?title=LanguageNn.php&action=history
  * @see http://nn.wikipedia.org/w/index.php?title=Brukar:Dittaeva/LanguageNn.php&action=history
  */

$skinNames = array(
	'standard'    => 'Klassisk',
	'nostalgia'   => 'Nostalgi',
	'cologneblue' => 'Kölnerblå',
	'myskin'      => 'MiDrakt',
	'simple'      => 'Enkel',
	'modern'      => 'Moderne',
);

$datePreferences = array(
	'default',
	'dmyt',
	'short dmyt',
	'tdmy',
	'short dmyt',
	'ISO 8601',
);

$datePreferenceMigrationMap = array(
	'default',
	'dmyt',
	'short dmyt',
	'tdmy',
	'short tdmy',
);

$defaultDateFormat = 'dmyt';

$dateFormats = array(
	/*
	'Standard',
	'15. januar 2001 kl. 16:12',
	'15. jan. 2001 kl. 16:12',
	'16:12, 15. januar 2001',
	'16:12, 15. jan. 2001',
	'ISO 8601' => '2001-01-15 16:12:34'
 */
	'dmyt time' => 'H:i',
	'dmyt date' => 'j. F Y',
	'dmyt both' => 'j. F Y "kl." H:i',

	'short dmyt time' => 'H:i',
	'short dmyt date' => 'j. M. Y',
	'short dmyt both' => 'j. M. Y "kl." H:i',

	'tdmy time' => 'H:i',
	'tdmy date' => 'j. F Y',
	'tdmy both' => 'H:i, j. F Y',

	'short tdmy time' => 'H:i',
	'short tdmy date' => 'j. M. Y',
	'short tdmy both' => 'H:i, j. M. Y',
);

$bookstoreList = array(
	'Bibsys'       => 'http://ask.bibsys.no/ask/action/result?kilde=biblio&fid=isbn&lang=nn&term=$1',
	'BokBerit'     => 'http://www.bokberit.no/annet_sted/bocker/$1.html',
	'Bokkilden'    => 'http://www.bokkilden.no/ProductDetails.aspx?ProductId=$1',
	'Haugenbok'    => 'http://www.haugenbok.no/resultat.cfm?st=hurtig&isbn=$1',
	'Akademika'    => 'http://www.akademika.no/sok.php?isbn=$1',
	'Gnist'        => 'http://www.gnist.no/sok.php?isbn=$1',
	'Amazon.co.uk' => 'http://www.amazon.co.uk/exec/obidos/ISBN=$1',
	'Amazon.de'    => 'http://www.amazon.de/exec/obidos/ISBN=$1',
	'Amazon.com'   => 'http://www.amazon.com/exec/obidos/ISBN=$1'
);

# Note to translators:
#   Please include the English words as synonyms.  This allows people
#   from other wikis to contribute more easily.
#
$magicWords = array(
	#   ID                                 CASE  SYNONYMS
	'redirect'               => array( 0,    '#redirect', '#omdiriger'                                              ),
	'notoc'                  => array( 0,    '__NOTOC__', '__INGAINNHALDSLISTE__', '__INGENINNHOLDSLISTE__'         ),
	'forcetoc'               => array( 0,    '__FORCETOC__', '__ALLTIDINNHALDSLISTE__', '__ALLTIDINNHOLDSLISTE__'   ),
	'toc'                    => array( 0,    '__TOC__', '__INNHALDSLISTE__', '__INNHOLDSLISTE__'                    ),
	'noeditsection'          => array( 0,    '__NOEDITSECTION__', '__INGABOLKENDRING__', '__INGABOLKREDIGERING__', '__INGENDELENDRING__'),
	'currentmonth'           => array( 1,    'CURRENTMONTH', 'MÅNADNO', 'MÅNEDNÅ'                                   ),
	'currentmonthname'       => array( 1,    'CURRENTMONTHNAME', 'MÅNADNONAMN', 'MÅNEDNÅNAVN'                       ),
	'currentmonthabbrev'     => array( 1,    'CURRENTMONTHABBREV', 'MÅNADNOKORT', 'MÅNEDNÅKORT'                     ),
	'currentday'             => array( 1,    'CURRENTDAY', 'DAGNO', 'DAGNÅ'                                         ),
	'currentdayname'         => array( 1,    'CURRENTDAYNAME', 'DAGNONAMN', 'DAGNÅNAVN'                             ),
	'currentyear'            => array( 1,    'CURRENTYEAR', 'ÅRNO', 'ÅRNÅ'                                          ),
	'currenttime'            => array( 1,    'CURRENTTIME', 'TIDNO', 'TIDNÅ'                                        ),
	'numberofarticles'       => array( 1,    'NUMBEROFARTICLES', 'INNHALDSSIDETAL', 'INNHOLDSSIDETALL'              ),
	'numberoffiles'          => array( 1,    'NUMBEROFFILES', 'FILTAL'                                              ),
	'pagename'               => array( 1,    'PAGENAME', 'SIDENAMN', 'SIDENAVN'                                     ),
	'pagenamee'              => array( 1,    'PAGENAMEE', 'SIDENAMNE', 'SIDENAVNE'                                  ),
	'namespace'              => array( 1,    'NAMESPACE', 'NAMNEROM', 'NAVNEROM'                                    ),
	'subst'                  => array( 0,    'SUBST:', 'LIMINN:'                                                    ),
	'msgnw'                  => array( 0,    'MSGNW:', 'IKWIKMELD:'                                                 ),
	'img_thumbnail'          => array( 1,    'thumbnail', 'thumb', 'mini', 'miniatyr'                               ),
	'img_manualthumb'        => array( 1,    'thumbnail=$1', 'thumb=$1', 'mini=$1', 'miniatyr=$1'                   ),
	'img_right'              => array( 1,    'right', 'høgre', 'høyre'                                              ),
	'img_left'               => array( 1,    'left', 'venstre'                                                      ),
	'img_none'               => array( 1,    'none', 'ingen'                                                        ),
	'img_width'              => array( 1,    '$1px', '$1pk'                                                         ),
	'img_center'             => array( 1,    'center', 'centre', 'sentrum'                                          ),
	'img_framed'             => array( 1,    'framed', 'enframed', 'frame', 'ramme'                                 ),
	'sitename'               => array( 1,    'SITENAME', 'NETTSTADNAMN'                                             ),
	'ns'                     => array( 0,    'NS:', 'NR:'                                                           ),
	'localurl'               => array( 0,    'LOCALURL:', 'LOKALLENKJE:', 'LOKALLENKE:'                             ),
	'localurle'              => array( 0,    'LOCALURLE:', 'LOKALLENKJEE:', 'LOKALLENKEE:'                          ),
	'server'                 => array( 0,    'SERVER', 'TENAR', 'TJENER'                                            ),
	'servername'             => array( 0,    'SERVERNAME', 'TENARNAMN', 'TJENERNAVN'                                ),
	'scriptpath'             => array( 0,    'SCRIPTPATH', 'SKRIPTSTI'                                              ),
	'grammar'                => array( 0,    'GRAMMAR:', 'GRAMMATIKK:'                                              ),
	'notitleconvert'         => array( 0,    '__NOTITLECONVERT__', '__NOTC__'                                       ),
	'nocontentconvert'       => array( 0,    '__NOCONTENTCONVERT__', '__NOCC__'                                     ),
	'currentweek'            => array( 1,    'CURRENTWEEK', 'VEKENRNO', 'UKENRNÅ'                                   ),
	'currentdow'             => array( 1,    'CURRENTDOW', 'VEKEDAGNRNO', 'UKEDAGNRNÅ'                              ),
	'revisionid'             => array( 1,    'REVISIONID', 'VERSJONSID'                                             )
);

$namespaceNames = array(
	NS_MEDIA          => 'Filpeikar',
	NS_SPECIAL        => 'Spesial',
	NS_MAIN           => '',
	NS_TALK           => 'Diskusjon',
	NS_USER           => 'Brukar',
	NS_USER_TALK      => 'Brukardiskusjon',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK   => '$1-diskusjon',
	NS_IMAGE          => 'Fil',
	NS_IMAGE_TALK     => 'Fildiskusjon',
	NS_MEDIAWIKI      => 'MediaWiki',
	NS_MEDIAWIKI_TALK => 'MediaWiki-diskusjon',
	NS_TEMPLATE       => 'Mal',
	NS_TEMPLATE_TALK  => 'Maldiskusjon',
	NS_HELP           => 'Hjelp',
	NS_HELP_TALK      => 'Hjelpdiskusjon',
	NS_CATEGORY       => 'Kategori',
	NS_CATEGORY_TALK  => 'Kategoridiskusjon'
);

$specialPageAliases = array(
	'DoubleRedirects'           => array( 'Doble_omdirigeringar' ),
	'BrokenRedirects'           => array( 'Blindvegsomdirigeringar' ),
	'Disambiguations'           => array( 'Fleirtydingssider' ),
	'Userlogin'                 => array( 'Logg_inn' ),
	'Userlogout'                => array( 'Logg_ut' ),
	'CreateAccount'             => array( 'Opprett_konto' ),
	'Preferences'               => array( 'Innstillingar' ),
	'Watchlist'                 => array( 'Overvakingsliste' ),
	'Recentchanges'             => array( 'Siste_endringar' ),
	'Upload'                    => array( 'Last_opp' ),
	'Imagelist'                 => array( 'Filliste' ),
	'Newimages'                 => array( 'Nye_filer' ),
	'Listusers'                 => array( 'Brukarliste' ),
	'Statistics'                => array( 'Statistikk' ),
	'Randompage'                => array( 'Tilfeldig_side' ),
	'Lonelypages'               => array( 'Foreldrelause_sider' ),
	'Uncategorizedpages'        => array( 'Ukategoriserte_sider' ),
	'Uncategorizedcategories'   => array( 'Ukategoriserte_kategoriar' ),
	'Uncategorizedimages'       => array( 'Ukategoriserte_filer' ),
	'Uncategorizedtemplates'    => array( 'Ukategoriserte_malar' ),
	'Unusedcategories'          => array( 'Ubrukte_kategoriar' ),
	'Unusedimages'              => array( 'Ubrukte_filer' ),
	'Wantedpages'               => array( 'Etterspurde_sider' ),
	'Wantedcategories'          => array( 'Etterspurde_kategoriar' ),
	'Mostlinked'                => array( 'Mest_lenka_sider' ),
	'Mostlinkedcategories'      => array( 'Mest_brukte_kategoriar' ),
	'Mostlinkedtemplates'       => array( 'Mest_brukte_malar' ),
	'Mostcategories'            => array( 'Flest_kategoriar' ),
	'Mostimages'                => array( 'Mest_brukte_filer' ),
	'Mostrevisions'             => array( 'Flest_endringar' ),
	'Fewestrevisions'           => array( 'Færrast_endringar' ),
	'Shortpages'                => array( 'Korte_sider' ),
	'Longpages'                 => array( 'Lange_sider' ),
	'Newpages'                  => array( 'Nye_sider' ),
	'Ancientpages'              => array( 'Gamle_sider' ),
	'Deadendpages'              => array( 'Blindvegsider' ),
	'Protectedpages'            => array( 'Verna_sider' ),
	'Protectedtitles'           => array( 'Verna_sidenamn' ),
	'Allpages'                  => array( 'Alle_sider' ),
	'Prefixindex'               => array( 'Prefiksindeks' ),
	'Ipblocklist'               => array( 'Blokkeringsliste' ),
	'Specialpages'              => array( 'Spesialsider' ),
	'Contributions'             => array( 'Bidrag' ),
	'Emailuser'                 => array( 'E-post' ),
	'Confirmemail'              => array( 'Stadfest_e-postadresse' ),
	'Whatlinkshere'             => array( 'Lenkjer_hit' ),
	'Recentchangeslinked'       => array( 'Relaterte_endringar' ),
	'Movepage'                  => array( 'Flytt_side' ),
	'Blockme'                   => array( 'Blokker_meg' ),
	'Booksources'               => array( 'Bokkjelder' ),
	'Categories'                => array( 'Kategoriar' ),
	'Export'                    => array( 'Eksport' ),
	'Version'                   => array( 'Versjon' ),
	'Allmessages'               => array( 'Alle_systemmeldingar' ),
	'Log'                       => array( 'Logg', 'Loggar' ),
	'Blockip'                   => array( 'Blokker' ),
	'Undelete'                  => array( 'Attopprett' ),
	'Import'                    => array( 'Import' ),
	'Lockdb'                    => array( 'Lås_database' ),
	'Unlockdb'                  => array( 'Opne_database' ),
	'Userrights'                => array( 'Brukarrettar' ),
	'MIMEsearch'                => array( 'MIME-søk' ),
	'Unwatchedpages'            => array( 'Uovervaka_sider' ),
	'Listredirects'             => array( 'Omdirigeringsliste' ),
	'Revisiondelete'            => array( 'Versjonssletting' ),
	'Unusedtemplates'           => array( 'Ubrukte_malar' ),
	'Randomredirect'            => array( 'Tilfeldig_omdirigering' ),
	'Mypage'                    => array( 'Sida_mi' ),
	'Mytalk'                    => array( 'Diskusjonssida_mi' ),
	'Mycontributions'           => array( 'Bidraga_mine' ),
	'Listadmins'                => array( 'Administratorliste', 'Administratorar' ),
	'Listbots'                  => array( 'Bottliste', 'Bottar' ),
	'Popularpages'              => array( 'Populære_sider' ),
	'Search'                    => array( 'Søk' ),
	'Resetpass'                 => array( 'Nullstill_passord' ),
	'Withoutinterwiki'          => array( 'Utan_interwiki' ),
	'MergeHistory'              => array( 'Flettehistorie' ),
);

$separatorTransformTable = array(
	',' => "\xc2\xa0",
	'.' => ','
);
$linkTrail = '/^([æøåa-z]+)(.*)$/sDu';


#-------------------------------------------------------------------
# Default messages
#-------------------------------------------------------------------

$messages = array(
# User preference toggles
'tog-underline'               => 'Strek under lenkjer:',
'tog-highlightbroken'         => 'Vis lenkjer til tomme sider <a href="" class="new">slik</a> (alternativt slik<a href="" class="internal">?</a>)',
'tog-justify'                 => 'Blokkjusterte avsnitt',
'tog-hideminor'               => 'Gøym småplukk på «siste endringar»',
'tog-extendwatchlist'         => 'Utvid overvakingslista til å vise alle endringane i det valde tidsrommet',
'tog-usenewrc'                => 'Utvida funksjonalitet på «siste endringar» (JavaScript)',
'tog-numberheadings'          => 'Vis nummererte overskrifter',
'tog-showtoolbar'             => 'Vis endringsknappar (JavaScript)',
'tog-editondblclick'          => 'Endre sider med dobbelklikk (JavaScript)',
'tog-editsection'             => 'Endre avsnitt med hjelp av [endre]-lenkje',
'tog-editsectiononrightclick' => 'Endre avsnitt med å høgreklikke på avsnittsoverskrift (JavaScript)',
'tog-showtoc'                 => 'Vis innhaldsliste (for sider med meir enn tre bolkar)',
'tog-rememberpassword'        => 'Hugs passordet til neste gong',
'tog-editwidth'               => 'Gjev endringsboksen full breidd',
'tog-watchcreations'          => 'Legg til sidene eg opprettar på overvakingslista mi',
'tog-watchdefault'            => 'Legg til sidene eg endrar på overvakingslista mi',
'tog-watchmoves'              => 'Legg til sidene eg flyttar på overvakingslista mi',
'tog-watchdeletion'           => 'Legg til sidene eg slettar på overvakingslista mi',
'tog-minordefault'            => 'Merk endringar som «småplukk» som standard',
'tog-previewontop'            => 'Vis førehandsvisinga føre endringsboksen',
'tog-previewonfirst'          => 'Førehandsvis første endring',
'tog-nocache'                 => 'Ikkje bruk nettlesaren sitt mellomlager (cache)',
'tog-enotifwatchlistpages'    => 'Send e-post når dei overvaka sidene mine blir endra',
'tog-enotifusertalkpages'     => 'Send e-post når brukarsida mi blir endra',
'tog-enotifminoredits'        => 'Send e-post også for småplukk',
'tog-enotifrevealaddr'        => 'Vis e-postadressa mi i endrings-e-post',
'tog-shownumberswatching'     => 'Vis kor mange som overvakar sida',
'tog-fancysig'                => 'Signatur utan automatisk lenkje',
'tog-externaleditor'          => 'Eksternt handsamingsprogram som standard',
'tog-externaldiff'            => 'Eksternt skilnadprogram som standard',
'tog-showjumplinks'           => 'Slå på «gå til»-lenkjer',
'tog-uselivepreview'          => 'Bruk levande førehandsvising (eksperimentelt JavaScript)',
'tog-forceeditsummary'        => 'Spør meg når eg ikkje har skrive noko i endringssamandraget',
'tog-watchlisthideown'        => 'Gøym endringane mine frå overvakingslista',
'tog-watchlisthidebots'       => 'Gøym endringar gjort av robotar frå overvakingslista',
'tog-watchlisthideminor'      => 'Gøym småplukk frå overvakingslista',
'tog-nolangconversion'        => 'Slå av variantkonvertering',
'tog-ccmeonemails'            => 'Send meg kopi av e-postane eg sender til andre brukarar',
'tog-diffonly'                => 'Ikkje vis sideinnhaldet under skilnadene mellom versjonane',
'tog-showhiddencats'          => 'Vis gøymde kategoriar',

'underline-always'  => 'Alltid',
'underline-never'   => 'Aldri',
'underline-default' => 'Nettlesarstandard',

'skinpreview' => '(førehandsvis)',

# Dates
'sunday'        => 'søndag',
'monday'        => 'måndag',
'tuesday'       => 'tysdag',
'wednesday'     => 'onsdag',
'thursday'      => 'torsdag',
'friday'        => 'fredag',
'saturday'      => 'laurdag',
'sun'           => 'søn',
'mon'           => 'mån',
'tue'           => 'tys',
'wed'           => 'ons',
'thu'           => 'tor',
'fri'           => 'fre',
'sat'           => 'lau',
'january'       => 'januar',
'february'      => 'februar',
'march'         => 'mars',
'april'         => 'april',
'may_long'      => 'mai',
'june'          => 'juni',
'july'          => 'juli',
'august'        => 'august',
'september'     => 'september',
'october'       => 'oktober',
'november'      => 'november',
'december'      => 'desember',
'january-gen'   => 'januar',
'february-gen'  => 'februar',
'march-gen'     => 'mars',
'april-gen'     => 'april',
'may-gen'       => 'mai',
'june-gen'      => 'juni',
'july-gen'      => 'juli',
'august-gen'    => 'august',
'september-gen' => 'september',
'october-gen'   => 'oktober',
'november-gen'  => 'november',
'december-gen'  => 'Desember',
'jan'           => 'jan',
'feb'           => 'feb',
'mar'           => 'mar',
'apr'           => 'apr',
'may'           => 'mai',
'jun'           => 'jun',
'jul'           => 'jul',
'aug'           => 'aug',
'sep'           => 'sep',
'oct'           => 'okt',
'nov'           => 'nov',
'dec'           => 'des',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|Kategori|Kategoriar}}',
'category_header'                => 'Artiklar i kategorien «$1»',
'subcategories'                  => 'Underkategoriar',
'category-media-header'          => 'Media i kategorien «$1»',
'category-empty'                 => "''Denne kategorien inneheld for tida ingen sider eller anna media.''",
'hidden-categories'              => '{{PLURAL:$1|Gøymd kategori|Gøymde kategoriar}}',
'hidden-category-category'       => 'Gøymde kategoriar', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|Denne kategorien har berre den følgjande underkategorien.|Denne kategorien har {{PLURAL:$1|den følgjande underkategorien|dei følgjande $1 underkategoriane}}, av totalt $2.}}',
'category-subcat-count-limited'  => 'Denne kategorien har {{PLURAL:$1|den følgjande underkategorien|dei følgjande $1 underkategoriane}}.',
'category-article-count'         => '{{PLURAL:$2|Denne kategorien inneheld berre den følgjande sida.|Følgjande {{PLURAL:$1|side|$1 sider}} er i denne kategorien, av totalt $2.}}',
'category-article-count-limited' => 'Følgjande {{PLURAL:$1|side|$1 sider}} er i denne kategorien.',
'category-file-count'            => '{{PLURAL:$2|Denne kategorien inneheld berre den følgjande fila.|Følgjande {{PLURAL:$1|fil|$1 filer}} er i denne kategorien, av totalt $2.}}',
'category-file-count-limited'    => 'Følgjande {{PLURAL:$1|fil|$1 filer}} er i denne kategorien.',
'listingcontinuesabbrev'         => 'vidare',

'mainpagetext'      => 'MediaWiki er no installert.',
'mainpagedocfooter' => 'Sjå [http://meta.wikimedia.org/wiki/Help:Contents brukarmanualen] for informasjon om bruk og konfigurasjonshjelp for wikiprogramvaren.

==Kome i gang==
* [http://www.mediawiki.org/wiki/Manual:Configuration_settings Liste over konfigurasjonsinnstillingar]
* [http://www.mediawiki.org/wiki/Manual:FAQ Spørsmål og svar om MediaWiki]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce E-postliste med informasjon om nye MediaWiki-versjonar]',

'about'          => 'Om',
'article'        => 'Innhaldsside',
'newwindow'      => '(blir opna i eit nytt vindauge)',
'cancel'         => 'Avbryt',
'qbfind'         => 'Finn',
'qbbrowse'       => 'Bla gjennom',
'qbedit'         => 'Endre',
'qbpageoptions'  => 'Denne sida',
'qbpageinfo'     => 'Samanheng',
'qbmyoptions'    => 'Sidene mine',
'qbspecialpages' => 'Spesialsider',
'moredotdotdot'  => 'Meir...',
'mypage'         => 'Sida mi',
'mytalk'         => 'Diskusjonssida mi',
'anontalk'       => 'Diskusjonside for denne IP-adressa',
'navigation'     => 'Navigering',
'and'            => 'og',

# Metadata in edit box
'metadata_help' => 'Utvida informasjon:',

'errorpagetitle'    => 'Feil',
'returnto'          => 'Attende til $1.',
'tagline'           => 'Frå {{SITENAME}}',
'help'              => 'Hjelp',
'search'            => 'Søk',
'searchbutton'      => 'Søk',
'go'                => 'Vis',
'searcharticle'     => 'Vis',
'history'           => 'Sidehistorikk',
'history_short'     => 'Historikk',
'updatedmarker'     => 'oppdatert etter mitt siste besøk',
'info_short'        => 'Informasjon',
'printableversion'  => 'Utskriftsversjon',
'permalink'         => 'Fast lenkje',
'print'             => 'Skriv ut',
'edit'              => 'Endre',
'create'            => 'opprett',
'editthispage'      => 'Endre sida',
'create-this-page'  => 'Lag denne sida',
'delete'            => 'Slett',
'deletethispage'    => 'Slett denne sida',
'undelete_short'    => 'Attopprett {{PLURAL:$1|éin versjon|$1 versjonar}}',
'protect'           => 'Vern',
'protect_change'    => 'endre',
'protectthispage'   => 'Vern denne sida',
'unprotect'         => 'Fjern vern',
'unprotectthispage' => 'Fjern vern av denne sida',
'newpage'           => 'Ny side',
'talkpage'          => 'Drøft sida',
'talkpagelinktext'  => 'Diskusjon',
'specialpage'       => 'Spesialside',
'personaltools'     => 'Personlege verktøy',
'postcomment'       => 'Legg til kommentar',
'articlepage'       => 'Vis innhaldsside',
'talk'              => 'Diskusjon',
'views'             => 'Visningar',
'toolbox'           => 'Verktøy',
'userpage'          => 'Vis brukarside',
'projectpage'       => 'Sjå prosjektsida',
'imagepage'         => 'Vis filside',
'mediawikipage'     => 'Vis systemmeldingsside',
'templatepage'      => 'Vis malside',
'viewhelppage'      => 'Vis hjelpeside',
'categorypage'      => 'Vis kategoriside',
'viewtalkpage'      => 'Vis diskusjon',
'otherlanguages'    => 'På andre språk',
'redirectedfrom'    => '(Omdirigert frå $1)',
'redirectpagesub'   => 'Omdirigeringsside',
'lastmodifiedat'    => 'Sist endra $2, $1.', # $1 date, $2 time
'viewcount'         => 'Sida er vist {{PLURAL:$1|éin gong|$1 gonger}}.',
'protectedpage'     => 'Verna side',
'jumpto'            => 'Gå til:',
'jumptonavigation'  => 'navigering',
'jumptosearch'      => 'søk',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'Om {{SITENAME}}',
'aboutpage'            => 'Project:Om',
'bugreports'           => 'Feilmeldingar',
'bugreportspage'       => 'Project:Feilmeldingar',
'copyright'            => 'Innhaldet er utgjeve under $1.',
'copyrightpagename'    => '{{SITENAME}} opphavsrett',
'copyrightpage'        => '{{ns:project}}:Opphavsrett',
'currentevents'        => 'Aktuelt',
'currentevents-url'    => 'Project:Aktuelt',
'disclaimers'          => 'Vilkår',
'disclaimerpage'       => 'Project:Vilkår',
'edithelp'             => 'Hjelp til endring',
'edithelppage'         => 'Help:Endring',
'faq'                  => 'OSS',
'faqpage'              => 'Project:OSS',
'helppage'             => 'Help:Innhald',
'mainpage'             => 'Hovudside',
'mainpage-description' => 'Hovudside',
'policy-url'           => 'Project:Retningsliner',
'portal'               => 'Brukarportal',
'portal-url'           => 'Project:Brukarportal',
'privacy'              => 'Personvern',
'privacypage'          => 'Project:Personvern',

'badaccess'        => 'Tilgangsfeil',
'badaccess-group0' => 'Du har ikkje lov til å utføre handlinga du ba om.',
'badaccess-group1' => 'Handlinga du ba om kan berre utførast av brukarar i gruppa $1.',
'badaccess-group2' => 'Handlinga du ba om kan berre utførast av brukarar i gruppene $1.',
'badaccess-groups' => 'Handlinga du ba om kan berre utførast av brukarar i gruppene $1.',

'versionrequired'     => 'MediaWiki versjon $1 trengst',
'versionrequiredtext' => 'Ein må ha MediaWiki versjon $1 for å bruke denne sida. Sjå [[Special:Version|versjonssida]].',

'ok'                      => 'OK',
'retrievedfrom'           => 'Henta frå «$1»',
'youhavenewmessages'      => 'Du har $1 ($2).',
'newmessageslink'         => 'nye meldingar',
'newmessagesdifflink'     => 'sjå skilnad',
'youhavenewmessagesmulti' => 'Du har nye meldingar på $1',
'editsection'             => 'endre',
'editold'                 => 'endre',
'viewsourceold'           => 'vis kjeldetekst',
'editsectionhint'         => 'Endre bolk: $1',
'toc'                     => 'Innhaldsliste',
'showtoc'                 => 'vis',
'hidetoc'                 => 'gøym',
'thisisdeleted'           => 'Sjå eller attopprett $1?',
'viewdeleted'             => 'Sjå historikk for $1?',
'restorelink'             => '{{PLURAL:$1|Éin sletta versjon|$1 sletta versjonar}}',
'feedlinks'               => 'Mating:',
'feed-invalid'            => 'Ugyldig abonnementstype.',
'feed-unavailable'        => 'Det er ingen kjelder til abonnement på {{SITENAME}}',
'site-rss-feed'           => '$1 RSS-abonnement',
'site-atom-feed'          => '$1 Atom-abonnement',
'page-rss-feed'           => '«$1» RSS-abonnement',
'page-atom-feed'          => '«$1» Atom-abonnement',
'red-link-title'          => '$1 (ikkje oppretta endå)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Side',
'nstab-user'      => 'Brukarside',
'nstab-media'     => 'Filside',
'nstab-special'   => 'Spesial',
'nstab-project'   => 'Prosjektside',
'nstab-image'     => 'Fil',
'nstab-mediawiki' => 'Systemmelding',
'nstab-template'  => 'Mal',
'nstab-help'      => 'Hjelp',
'nstab-category'  => 'Kategori',

# Main script and global functions
'nosuchaction'      => 'Funksjonen finst ikkje',
'nosuchactiontext'  => 'Wikiprogramvaren kjenner ikkje att funksjonen som er spesifisert i nettadressa',
'nosuchspecialpage' => 'Ei slik spesialside finst ikkje',
'nospecialpagetext' => 'Du har bede om ei spesialside som ikkje finst, liste over spesialsider er [[Special:SpecialPages|her]].',

# General errors
'error'                => 'Feil',
'databaseerror'        => 'Databasefeil',
'dberrortext'          => 'Det oppstod ein syntaksfeil i databaseførespurnaden. Dette kan tyde på ein feil i programvaren. Den sist prøvde førespurnaden var: <blockquote><tt>$1</tt></blockquote> frå funksjonen «<tt>$2</tt>». MySQL returnerte feilen «<tt>$3: $4</tt>».',
'dberrortextcl'        => 'Det oppstod ein syntaksfeil i databaseførespurnaden. Den sist prøvde førespurnaden var: «$1» frå funksjonen "$2".
MySQL returnerte feilen «$3: $4».',
'noconnect'            => 'Årsak! Wikien har tekniske problem og kunne ikkje kople til databasen.<br />
$1',
'nodb'                 => 'Kunne ikkje velja databasen $1',
'cachederror'          => 'Det følgjande er ein lagra kopi av den ønska sida, og er ikkje nødvendigvis oppdatert.',
'laggedslavemode'      => 'Åtvaring: Det er mogleg at sida ikkje er heilt oppdatert.',
'readonly'             => 'Databasen er skriveverna',
'enterlockreason'      => 'Skriv ein grunn for vernet, inkludert eit overslag for kva tid det vil bli oppheva',
'readonlytext'         => 'Databasen er akkurat no skriveverna, truleg for rutinemessig vedlikehald. Administratoren som verna han har gjeve denne forklaringa:

$1',
'missing-article'      => 'Databasen burde ha funne sida «$1» $2, men det gjorde han ikkje.

Dei vanlegaste årsakene til denne feilen er ei lenkje til ein skilnad mellom forskjellige versjonar eller lenkjer til ein gammal versjon av ei side som har vorte sletta. 

Om det ikkje er tilfellet kan du ha funne ein feil i programvara. 
Meld gjerne problemet til ein [[Special:ListUsers/sysop|administrator]] og oppgje då adressa til sida.',
'missingarticle-rev'   => '(versjon $1)',
'missingarticle-diff'  => '(jamføring av versjon $1 og $2)',
'readonly_lag'         => 'Databasen er mellombels skriveverna for at databasetenarane skal kunna synkronisere seg mot kvarandre',
'internalerror'        => 'Intern feil',
'internalerror_info'   => 'Intern feil: $1',
'filecopyerror'        => 'Kunne ikkje kopiere fila frå «$1» til «$2».',
'filerenameerror'      => 'Kunne ikkje døype om fila frå «$1» til «$2».',
'filedeleteerror'      => 'Kunne ikkje slette fila «$1».',
'directorycreateerror' => 'Kunne ikkje opprette mappa «$1».',
'filenotfound'         => 'Kunne ikkje finne fila «$1».',
'fileexistserror'      => 'Kunne ikkje skrive til fila «$1», ho eksisterer allereie',
'unexpected'           => 'Uventa verdi: «$1»=«$2».',
'formerror'            => 'Feil: Kunne ikkje sende skjema',
'badarticleerror'      => 'Handlinga kan ikkje utførast på denne sida.',
'cannotdelete'         => 'Kunne ikkje slette fila. (Ho kan vera sletta av andre.)',
'badtitle'             => 'Feil i tittelen',
'badtitletext'         => 'Den ønska tittelen var ulovleg, tom eller feil lenka frå ei anna wiki.',
'perfdisabled'         => 'Beklagar! Denne funksjonen er mellombels deaktivert for å spara tenarkapasitet.',
'perfcached'           => 'Det følgjande er frå mellomlageret åt tenaren og er ikkje nødvendigvis oppdatert.',
'perfcachedts'         => 'Desse data er mellomlagra, og vart sist oppdaterte $1.',
'querypage-no-updates' => 'Oppdatering av denne sida er slått av, og data her vil ikkje verte fornya.',
'wrong_wfQuery_params' => 'Feil parameter gjevne til wfQuery()<br />Funksjon: $1<br />Førespurnad: $2',
'viewsource'           => 'Vis kjeldetekst',
'viewsourcefor'        => 'for $1',
'actionthrottled'      => 'Handlinga vart stoppa',
'actionthrottledtext'  => 'For å hindre spamming, kan du ikkje utføre denne handlinga for mange gonger på kort tid. Ver venleg og prøv igjen litt seinare.',
'protectedpagetext'    => 'Denne sida er verna for å hindre endring.',
'viewsourcetext'       => 'Du kan sjå og kopiere kjeldekoden til denne sida:',
'protectedinterface'   => 'Denne sida inneheld tekst som er brukt av brukargrensesnittet for programvaren, og er difor låst for å hindre hærverk.',
'editinginterface'     => "'''Åtvaring:''' Du endrar på ei side som inneheld tekst som er brukt av brukargrensesnittet for programvaren. Endringar på denne sida påverkar utsjånaden til sida for dei andre brukarane. Dersom du ynskjer å omsetje, ver venleg og vurder å bruke [http://translatewiki.net/wiki/Main_Page?setlang=nn Betawiki], prosjektet for omsetjing av MediaWiki.",
'sqlhidden'            => '(SQL-førespurnaden er gøymd)',
'cascadeprotected'     => 'Denne sida er verna mot endring fordi ho er inkludert i {{PLURAL:$1|den opplista sida|dei opplista sidene}} som har djupvern slått på:
$2',
'namespaceprotected'   => "Du har ikkje tilgang til å endre sidene i '''$1'''-namnerommet.",
'customcssjsprotected' => 'Du har ikkje tilgang til å endre denne sida, fordi ho inneheld ein annan brukar sine personlege innstillingar.',
'ns-specialprotected'  => 'Sider i {{ns:special}}-namnerommet kan ikkje endrast.',
'titleprotected'       => "Denne sidetittelen er verna mot oppretting av [[User:$1|$1]].
Grunnen som er gjeven er: ''$2''.",

# Virus scanner
'virus-badscanner'     => 'Dårleg konfigurasjon: ukjend virusskanner: <i>$1</i>',
'virus-scanfailed'     => 'skanning mislukkast (kode $1)',
'virus-unknownscanner' => 'ukjend antivirusprogram:',

# Login and logout pages
'logouttitle'                => 'Logg ut',
'logouttext'                 => '<strong>Du er no utlogga.</strong>

Du kan no halde fram og bruke {{SITENAME}} anonymt, eller du kan [[Special:UserLogin|logge inn att]]  med same kontoen eller ein annan brukar kan logge inn. 
Ver merksam på at nokre sider kan halde fram med å verte viste som om du er innlogga fram til du slettar mellomlageret til nettlesaren din.',
'welcomecreation'            => '== Hjarteleg velkommen til {{SITENAME}}, $1! ==
Brukarkontoen din har vorte oppretta.
Ikkje gløym å endre på [[Special:Preferences|innstillingane]] dine.',
'loginpagetitle'             => 'Logg inn',
'yourname'                   => 'Brukarnamn:',
'yourpassword'               => 'Passord:',
'yourpasswordagain'          => 'Skriv opp att passordet',
'remembermypassword'         => 'Hugs passordet.',
'yourdomainname'             => 'Domenet ditt',
'externaldberror'            => 'Det var anten ein ekstern databasefeil i tilgjengekontrollen, eller du har ikkje løyve til å oppdatere den eksterne kontoen din.',
'loginproblem'               => '<b>Du vart ikkje innlogga.</b><br />Prøv om att!',
'login'                      => 'Logg inn',
'nav-login-createaccount'    => 'Lag brukarkonto / logg inn',
'loginprompt'                => 'Nettlesaren din må godta informasjonskapslar for at du skal kunna logge inn.',
'userlogin'                  => 'Lag brukarkonto / logg inn',
'logout'                     => 'Logg ut',
'userlogout'                 => 'Logg ut',
'notloggedin'                => 'Ikkje innlogga',
'nologin'                    => 'Er du allereie registrert? $1.',
'nologinlink'                => 'Registrer deg',
'createaccount'              => 'Opprett ny konto',
'gotaccount'                 => 'Er du allereie registrert? $1.',
'gotaccountlink'             => 'Logg inn',
'createaccountmail'          => 'over e-post',
'badretype'                  => 'Passorda du skreiv inn er ikkje like.',
'userexists'                 => 'Brukarnamnet er alt i bruk. Vel eit anna.',
'youremail'                  => 'E-post:',
'username'                   => 'Brukarnamn:',
'uid'                        => 'Brukar-ID:',
'prefs-memberingroups'       => 'Medlem av {{PLURAL:$1|denne gruppa|desse gruppene}}:',
'yourrealname'               => 'Verkeleg namn:',
'yourlanguage'               => 'Språk:',
'yourvariant'                => 'Språkvariant',
'yournick'                   => 'Signatur:',
'badsig'                     => 'Ugyldig råsignatur, sjekk HTML-kodinga.',
'badsiglength'               => 'Signaturen er for lang, han må vere under $1 {{PLURAL:$1|teikn|teikn}}.',
'email'                      => 'E-post',
'prefs-help-realname'        => '* Namn (valfritt): Om du vel å fylle ut dette feltet, vil informasjonen bli brukt til å godskrive arbeid du har gjort.',
'loginerror'                 => 'Innloggingsfeil',
'prefs-help-email'           => 'Å oppgje e-postadresse er valfritt, men lar deg ta i mot nytt passord om du gløymer det gamle. 
Du kan òg velje å la andre brukarar kontakte deg på e-post via brukarsida di utan å røpe identiteten din.',
'prefs-help-email-required'  => 'E-postadresse må oppgjevast.',
'nocookiesnew'               => 'Brukarkontoen vart oppretta, men du er ikkje innlogga. {{SITENAME}} bruker informasjonskapslar for å logge inn brukarar,
nettlesaren din er innstilt for ikkje å godta desse. Etter at du har endra innstillingane slik at nettlesaren godtek informasjonskapslar, kan du logge inn med det nye brukarnamnet og passordet ditt.',
'nocookieslogin'             => '{{SITENAME}} bruker informasjonskapslar for å logge inn brukarar, nettlesaren din er innstilt for ikkje å godta desse.
Etter at du har endra innstillingane slik at nettlesaren godtek informasjonskapslar kan du prøve å logge inn på nytt.',
'noname'                     => 'Du har ikkje oppgjeve gyldig brukarnamn.',
'loginsuccesstitle'          => 'Du er no innlogga',
'loginsuccess'               => 'Du er no innlogga som «$1».',
'nosuchuser'                 => 'Det finst ingen med brukarnamnet «$1». Sjekk at du har skrive det rett eller 
Sjekk stavemåten eller [[Special:Userlogin/signup|opprett ein ny konto]].',
'nosuchusershort'            => 'Det finst ikkje nokon brukar med brukarnamnet «<nowiki>$1</nowiki>». Sjekk at du har skrive rett.',
'nouserspecified'            => 'Du må oppgje eit brukarnamn.',
'wrongpassword'              => 'Du har oppgjeve eit ugyldig passord. Prøv om att.',
'wrongpasswordempty'         => 'Du oppgav ikkje noko passord. Ver venleg og prøv igjen.',
'passwordtooshort'           => 'Passordet er ugyldig eller for kort.
Det må vera minst {{PLURAL:$1|éitt teikn|$1 teikn}} langt og noko anna enn brukarnamnet ditt.',
'mailmypassword'             => 'Send nytt passord',
'passwordremindertitle'      => 'Nytt passord til {{SITENAME}}',
'passwordremindertext'       => 'Nokon (truleg du, frå IP-adressa $1) bad oss sende deg eit nytt passord til {{SITENAME}} ($4).
Passordet for brukaren «$2» er no «$3».
Du bør logge inn og endre passordet så snart som råd.

Logg inn her: {{SERVER}}

Dersom du ikkje ynskjer å endre passordet ditt kan du sjå bort frå denne meldinga og halde fram med å bruka det gamle passordet ditt.',
'noemail'                    => 'Det er ikkje registrert noka e-postadresse åt brukaren «$1».',
'passwordsent'               => 'Eit nytt passord er sendt åt e-postadressa registrert på brukaren «$1».',
'blocked-mailpassword'       => 'IP-adressa di er blokkert frå å endre sider, og du kan difor heller ikkje få nytt passord. Dette er for å hindre misbruk.',
'eauthentsent'               => 'Ein stadfestings-e-post er sendt til den oppgjevne e-postadressa. For at adressa skal kunna brukast, må du følgje instruksjonane i e-posten for å stadfeste at ho faktisk tilhøyrer deg.',
'throttled-mailpassword'     => 'Ei passordpåminning er allereie sendt {{PLURAL:$1|den siste timen|dei siste $1 timane}}. For å hindre misbruk vert det berre sendt ut nytt passord ein gong kvar {{PLURAL:$1|time|$1. time}}.',
'mailerror'                  => 'Ein feil oppstod ved sending av e-post: $1',
'acct_creation_throttle_hit' => 'Beklagar, du har allereie laga $1 brukarkontoar. Du har ikkje høve til å laga fleire.',
'emailauthenticated'         => 'E-postadressa di vart stadfest $1.',
'emailnotauthenticated'      => 'E-postadressa di er enno ikkje stadfest. Dei følgjande funksjonane kan ikkje bruke ho.',
'noemailprefs'               => '<strong>Du har ikkje oppgjeve noko e-postadresse</strong>, dei følgjande funksjonane vil ikkje verke.',
'emailconfirmlink'           => 'Stadfest e-post-adressa di',
'invalidemailaddress'        => 'E-postadressa kan ikkje nyttast sidan formatet truleg er feil. Skriv ei fungerande adresse eller tøm feltet.',
'accountcreated'             => 'Brukarkonto oppretta',
'accountcreatedtext'         => 'Brukarkontoen til $1 er oppretta.',
'createaccount-title'        => 'Oppretting av brukarkonto på {{SITENAME}}',
'createaccount-text'         => 'Nokon oppretta ein brukarkonto for $2 på {{SITENAME}} ($4). Passordet til «$2» er «$3». Du bør logge inn og endre passordet ditt med ein gong.

Du kan sjå bort frå denne meldinga dersom kontoen vart oppretta med eit uhell.',
'loginlanguagelabel'         => 'Språk: $1',

# Password reset dialog
'resetpass'               => 'Nullstill passordet til brukarkontoen',
'resetpass_announce'      => 'Du logga inn med eit mellombels passord du fekk på e-post. For å fullføre innlogginga må du lage eit nytt passord her:',
'resetpass_text'          => '<!-- Legg til tekst her -->',
'resetpass_header'        => 'Nullstill passord',
'resetpass_submit'        => 'Oppgje passord og logg inn',
'resetpass_success'       => 'Passordet ditt er no nullstilt! Loggar inn...',
'resetpass_bad_temporary' => 'Ugyldig mellombels passord. Du kan allereie ha endra det, eller bede om eit nytt.',
'resetpass_forbidden'     => 'Passord kan ikkje endrast på {{SITENAME}}',
'resetpass_missing'       => 'Skjemaet er tomt.',

# Edit page toolbar
'bold_sample'     => 'Halvfeit skrift',
'bold_tip'        => 'Halvfeit skrift',
'italic_sample'   => 'Kursivskrift',
'italic_tip'      => 'Kursivskrift',
'link_sample'     => 'Lenkjetittel',
'link_tip'        => 'Intern lenkje',
'extlink_sample'  => 'http://www.example.com lenkjetittel',
'extlink_tip'     => 'Ekstern lenkje (hugs http:// prefiks)',
'headline_sample' => 'Overskriftstekst',
'headline_tip'    => '2. nivå-overskrift',
'math_sample'     => 'Skriv formel her',
'math_tip'        => 'Matematisk formel (LaTeX)',
'nowiki_sample'   => 'Skriv uformatert tekst her',
'nowiki_tip'      => 'Sjå bort frå wikiformatering',
'image_sample'    => 'Eksempel.jpg',
'image_tip'       => 'Bilete eller lenkje til filomtale',
'media_sample'    => 'Eksempel.ogg',
'media_tip'       => 'Filpeikar',
'sig_tip'         => 'Signaturen din med tidsstempel',
'hr_tip'          => 'Vassrett line',

# Edit pages
'summary'                          => 'Samandrag',
'subject'                          => 'Emne/overskrift',
'minoredit'                        => 'Småplukk',
'watchthis'                        => 'Overvak denne sida',
'savearticle'                      => 'Lagre',
'preview'                          => 'Førehandsvising',
'showpreview'                      => 'Førehandsvis',
'showlivepreview'                  => 'Levande førehandsvising',
'showdiff'                         => 'Vis skilnad',
'anoneditwarning'                  => "'''Åtvaring:''' Du er ikkje innlogga. IP-adressa di vert lagra i historikken for denne sida.",
'missingsummary'                   => "'''Påminning:''' Du har ikkje skrive noko endringssamandrag. Dersom du trykkjer «Lagre» ein gong til, vert endringa di lagra utan.",
'missingcommenttext'               => 'Ver venleg og skriv ein kommentar nedanfor.',
'missingcommentheader'             => "'''Påminning:''' Du har ikkje oppgjeve noko emne/overskrift for denne kommentaren. Dersom du trykkjer «Lagre» ein gong til, vert endringa di lagra utan.",
'summary-preview'                  => 'Førehandsvising av endringssamandraget',
'subject-preview'                  => 'Førehandsvising av emne/overskrift',
'blockedtitle'                     => 'Brukaren er blokkert',
'blockedtext'                      => "<big>'''Brukarnamnet ditt eller IP-adressa di er blokkert frå endring'''</big>

Blokkeringa vart gjort av $1. 
Denne grunnen vart gjeven: ''$2''.

* Blokkeringa byrja: $8
* Blokkeringa utgår: $6
* Blokkeringa var meint på: $7

Du kan kontakte $1 eller ein annan [[{{MediaWiki:Grouppage-sysop}}|administrator]] for å diskutere blokkeringa. 
Ver merksam på at du ikkje kan bruke «send e-post til brukar»-funksjonen så lenge du ikkje har ei gyldig e-postadresse registrert i [[Special:Preferences|innstillingane dine]]. Du kan heller ikkje bruke funksjonen dersom du er blokkert frå å sende e-post. 
IP-adressa di er $3, og blokkeringsnummeret er $5. 
Ver venleg og opplys om dette ved eventuelle førespurnader.",
'autoblockedtext'                  => "IP-adressa di er automatisk blokkert fordi ho vart brukt av ein annan brukar som vart blokkert av $1. Grunne til dette vart gjeve som: ''$2''.

* Blokkeringa byrja: $8
* Blokkeringa går ut: $6
* Blokkeringa er meint for: $7

Du kan kontakte $1 eller ein annan [[{{MediaWiki:Grouppage-sysop}}|administrator]] for å diskutere blokkeringa. Ver merksam på at du ikkje kan bruke «send e-post til brukar»-funksjonen så lenge du ikkje har ei gyldig e-postadresse registrert i [[Special:Preferences|innstillingane dine]]. 

IP-adressa di er $3, og blokkeringnummeret ditt er #$5.
Ver venleg og opplyse dette ved eventuelle førespurnader.",
'blockednoreason'                  => 'inga grunngjeving',
'blockedoriginalsource'            => "Kjeldekoden til '''$1''' er vist nedanfor:",
'blockededitsource'                => "Teksten i '''endringane dine''' på '''$1''' er vist nedanfor:",
'whitelistedittitle'               => 'Du lyt logge inn for å gjera endringar',
'whitelistedittext'                => 'Du lyt $1 for å endre sider.',
'confirmedittitle'                 => 'Du må stadfeste e-postadressa di før du kan endre noko',
'confirmedittext'                  => 'Du må stadfeste e-postadressa di før du kan endre sidene. Ver venleg og legg inn og stadfest e-postadressa di i [[Special:Preferences|innstillingane dine]].',
'nosuchsectiontitle'               => 'Ingen slik bolk',
'nosuchsectiontext'                => 'Du prøvde å endre ein bolk som ikkje finst. Sidan det ikkje er {{PLURAL:bolkar|$1 bolkar}} i teksten, er det ingen stad å lagre endringa di.',
'loginreqtitle'                    => 'Innlogging trengst',
'loginreqlink'                     => 'logg inn',
'loginreqpagetext'                 => 'Du lyt $1 for å lesa andre sider.',
'accmailtitle'                     => 'Passord er sendt.',
'accmailtext'                      => 'Passordet for «$1» er vorte sendt til $2.',
'newarticle'                       => '(Ny)',
'newarticletext'                   => "'''{{SITENAME}} har ikkje noka side med namnet {{PAGENAME}} enno.'''
* For å opprette ei slik side kan du skrive i boksen under og klikke på «Lagre». Endringane vil vere synlege med det same.
* Om du er ny her er det tilrådd å sjå på [[{{MediaWiki:Helppage}}|hjelpesida]] først.
* Om du lagrar ei testside, vil du ikkje kunne slette henne sjølv.
* Dersom du ikkje ønskjer å endre sida, kan du utan risiko klikke på '''attende'''-knappen i nettlesaren din.",
'anontalkpagetext'                 => "----''Dette er ei diskusjonsside for ein anonym brukar som ikkje har oppretta konto eller ikkje har logga inn.
Vi er difor nøydde til å bruke den numeriske IP-adressa til å identifisere brukaren. Same IP-adresse kan vere knytt til fleire brukarar. Om du er ein anonym brukar og meiner at du har fått irrelevante kommentarar på ei slik side, [[Special:UserLogin|logg inn]] slik at vi unngår framtidige forvekslingar med andre anonyme brukarar.''",
'noarticletext'                    => "Det er ikkje noko tekst på denne sida. Du kan [[Special:Search/{{PAGENAME}}|søke etter henne]] i andre sider, eller '''[{{fullurl:{{FULLPAGENAME}}|action=edit}} opprette sida]'''.",
'userpage-userdoesnotexist'        => 'Brukarkontoen «$1» finst ikkje. Vil du verkeleg opprette/endre denne sida?',
'clearyourcache'                   => "'''Merk: Etter lagring vil det kanskje vera naudsynt at nettlesaren slettar mellomlageret sitt for at endringane skal tre i kraft.''' '''Firefox og Safari:''' Hald ''Shift'' nede medan du trykkjer anten ''Ctrl-F5'' eller ''Ctrl-R'' (''Command-R'' på Mac). '''Konqueror:''' Trykk ''Oppdater'' eller på ''F5''. '''Opera:''' Tøm mellomlageret i ''Verktøy → Innstillingar''. '''Internet Explorer:''' Hald nede ''Ctrl'' medan du trykkjer ''Oppdater'', eler trykk ''Ctrl-F5.''",
'usercssjsyoucanpreview'           => '<strong>Tips:</strong> Bruk «Førehandsvis»-knappen for å teste den nye CSS- eller JS-koden din før du lagrar.',
'usercsspreview'                   => "'''Hugs at dette berre er ei førehandsvising av din eigen CSS og at han ikkje er lagra enno!'''",
'userjspreview'                    => "'''Hugs at du berre testar ditt eige JavaScript, det har ikkje vorte lagra enno!!'''",
'userinvalidcssjstitle'            => "'''Åtvaring:''' Det finst ikkje noka sidedrakt som heiter «$1». Hugs på at vanlege .css- og .js-sider brukar titlar med små bokstavar, til dømes {{ns:user}}:Døme/monobook.css, og ikkje {{ns:user}}:Døme/Monobook.css.",
'updated'                          => '(Oppdatert)',
'note'                             => '<strong>Merk:</strong>',
'previewnote'                      => '<strong>Hugs at dette berre er ei førehandsvising og at teksten ikkje er lagra!</strong>',
'previewconflict'                  => 'Dette er ei førehandsvising av teksten i endringsboksen over, slik han vil sjå ut om du lagrar han',
'session_fail_preview'             => '<strong>Beklagar! Endringa di kunne ikkje lagrast. Ver venleg og prøv ein gong til. Dersom det framleis ikkje går, prøv å logge deg ut og inn att.</strong>',
'session_fail_preview_html'        => "<strong>Beklagar! Endringa di kunne ikkje lagrast.</strong>

''Fordi {{SITENAME}} har rå HTML-kode slått på, er førehandsvisinga gøymd grunna fare for JavaScript-angrep.''

<strong>Dersom dette er eit heilt vanleg forsøk på endring, prøv ein gong til. Dersom det framleis ikkje går, prøv å logge deg ut og inn att.</strong>",
'token_suffix_mismatch'            => '<strong>Endringa di vart avvist fordi klienten/nettlesaren din lagar teiknfeil i teksten. Dette vart gjort for å hindre øydelegging av teksten på sida. Slikt kan av og til hende når ein brukar feilprogrammerte og vevbaserte anonyme proxytenester.</strong>',
'editing'                          => 'Endrar $1',
'editingsection'                   => 'Endrar $1 (bolk)',
'editingcomment'                   => 'Endrar $1 (kommentar)',
'editconflict'                     => 'Endringskonflikt: $1',
'explainconflict'                  => 'Nokon annan har endra teksten sidan du byrja å skrive. Den øvste boksen inneheld den noverande teksten. Skilnaden mellom den lagra versjonen og din endra versjon er viste under. Versjonen som du har endra er i den nedste boksen. Du lyt flette endringane dine saman med den noverande teksten. <strong>Berre</strong> teksten i den øvste tekstboksen vil bli lagra når du klikkar på «Lagre».<br />',
'yourtext'                         => 'Teksten din',
'storedversion'                    => 'Den lagra versjonen',
'nonunicodebrowser'                => '<strong>ÅTVARING: Nettlesaren din støttar ikkje «Unicode».
For å omgå problemet blir teikn utanfor ASCII-standarden viste som heksadesimale kodar.</strong><br />',
'editingold'                       => '<strong>ÅTVARING: Du endrar ein gammal versjon av denne sida. Om du lagrar ho, vil alle endringar gjorde etter denne versjonen bli overskrivne.</strong> (Men dei kan hentast fram att frå historikken.)<br />',
'yourdiff'                         => 'Skilnader',
'copyrightwarning'                 => 'Merk deg at alle bidrag til {{SITENAME}} er å rekne som utgjevne under $2 (sjå $1 for detaljar). Om du ikkje vil ha teksten endra og kopiert under desse vilkåra, kan du ikkje leggje han her.<br />
Teksten må du ha skrive sjølv, eller kopiert frå ein ressurs som er kompatibel med vilkåra eller ikkje verna av opphavsrett.

<strong>LEGG ALDRI INN MATERIALE SOM ANDRE HAR OPPHAVSRETT TIL UTAN LØYVE FRÅ DEI!</strong>',
'copyrightwarning2'                => 'Merk deg at alle bidrag til {{SITENAME}} kan bli endra, omskrive og fjerna av andre bidragsytarar. Om du ikkje vil ha teksten endra under desse vilkåra, kan du ikkje leggje han her.<br />
Teksten må du ha skrive sjølv eller ha kopiert frå ein ressurs som er kompatibel med vilkåra eller ikkje verna av opphavsrett (sjå $1 for detaljar).

<strong>LEGG ALDRI INN MATERIALE SOM ANDRE HAR OPPHAVSRETT TIL UTAN LØYVE FRÅ DEI!</strong>',
'longpagewarning'                  => '<strong>ÅTVARING: Denne sida er $1 KB lang; nokre nettlesarar kan ha problem med å handsama endringar av sider som nærmar seg eller er lengre enn 32 KB. Du bør vurdere å dele opp sida i mindre bolkar.</strong><br />',
'longpageerror'                    => '<strong>Feil: Teksten du har prøvd å lagre er $1 kilobyte
lang, altså lenger enn $2 kilobyte som er maksimum. Han kan difor ikkje lagrast.</strong>',
'readonlywarning'                  => '<strong>ÅTVARING: Databasen er skriveverna på grunn av vedlikehald, difor kan du ikkje lagre endringane dine akkurat no. Det kan vera lurt å  kopiere teksten din åt ei tekstfil, så du kan lagre han her seinare.</strong><br />',
'protectedpagewarning'             => '<strong>ÅTVARING: Denne sida er verna, slik at berre administratorar kan endre ho.</strong><br />',
'semiprotectedpagewarning'         => "'''NB:''' Denne sida er verna slik at berre registrerte brukarar kan endre henne.",
'cascadeprotectedwarning'          => "'''Åtvaring:''' Denne sida er verna så berre brukarar med administratortilgang kan endre henne. Dette er fordi ho er inkludert i {{PLURAL:$1|denne djupverna sida|desse djupverna sidene}}:",
'titleprotectedwarning'            => '<strong>Åtvaring: Denne sida er verna, så berre nokre brukarar kan opprette henne.</strong>',
'templatesused'                    => 'Malar som er brukte på denne sida:',
'templatesusedpreview'             => 'Malar som er brukte i denne førehandsvisinga:',
'templatesusedsection'             => 'Malar som er brukte i denne bolken:',
'template-protected'               => '(verna)',
'template-semiprotected'           => '(delvis verna)',
'hiddencategories'                 => 'Denne sida er med i {{PLURAL:$1|éin gøymd kategori|$1 gøymde kategoriar}}:',
'edittools'                        => '<!-- Teksten her vert synt mellom tekstboksen og «Lagre»-knappen når ein endrar ei side. -->',
'nocreatetitle'                    => 'Avgrensa sideoppretting',
'nocreatetext'                     => '{{SITENAME}} har avgrensa tilgang til å opprette nye sider.
Du kan gå attende og endre ei eksisterande side, [[Special:UserLogin|logge inn eller opprette ein brukarkonto]].',
'nocreate-loggedin'                => 'Du har ikkje tilgang til å opprette nye sider på {{SITENAME}}.',
'permissionserrors'                => 'Tilgangsfeil',
'permissionserrorstext'            => 'Du har ikkje tilgang til å gjere dette, {{PLURAL:$1|grunnen|grunnane}} til det finn du her:',
'permissionserrorstext-withaction' => 'Du har ikke løyve til å $2 {{PLURAL:$1|på grunn av|av desse grunnane}}:',
'recreate-deleted-warn'            => "'''Åtvaring: Du nyopprettar ei side som tidlegare har vorte sletta.'''

Du bør tenkje over om det er lurt å halde fram med å endre denne sida.
Sletteloggen for sida finn du her:",

# Parser/template warnings
'expensive-parserfunction-warning'        => 'Åtvaring: Denne sida inneheld for mange prosesskrevande parsarfunksjonar.

Det burde vere færre enn $2, men er no $1.',
'expensive-parserfunction-category'       => 'Sider med for mange prosesskrevande parserfunksjonar',
'post-expand-template-inclusion-warning'  => 'Åtvaring: Storleiken på malar som er inkluderte er for stor.
Nokre malar vert ikkje inkluderte.',
'post-expand-template-inclusion-category' => 'Sider som inneheld for store malar',
'post-expand-template-argument-warning'   => 'Åtvaring: Sida inneheld ein eller fleire malparameterar som vert for lange når dei utvidast. 
Desse parameterane har vorte utelatne.',
'post-expand-template-argument-category'  => 'Sider med utelatne malparameterar',

# "Undo" feature
'undo-success' => 'Endringa kan attenderullast. Ver venleg og sjå over skilnadene nedanfor for å vere sikker på at du vil attenderulle. Deretter kan du lagre attenderullinga.',
'undo-failure' => 'Endringa kunne ikkje attenderullast grunna konflikt med endringar som er gjort i mellomtida.',
'undo-norev'   => 'Redigeringa kunne ikkje fjernast fordi den ikkje finnast eller vart sletta',
'undo-summary' => 'Rullar attende versjon $1 av [[Special:Contributions/$2|$2]] ([[User talk:$2|diskusjon]])',

# Account creation failure
'cantcreateaccounttitle' => 'Kan ikkje opprette brukarkonto',
'cantcreateaccount-text' => "Kontooppretting frå denne IP-adressa ('''$1''') er blokkert av [[User:$3|$3]].

Grunnen som vart gjeven av $3 er ''$2''",

# History pages
'viewpagelogs'        => 'Vis loggane for denne sida',
'nohistory'           => 'Det finst ikkje nokon historikk for denne sida.',
'revnotfound'         => 'Fann ikkje versjonen',
'revnotfoundtext'     => 'Den gamle versjonen av sida du spurde etter finst ikkje. Sjekk nettadressa du brukte for å komma deg åt denne sida.',
'currentrev'          => 'Noverande versjon',
'revisionasof'        => 'Versjonen frå $1',
'revision-info'       => 'Versjonen frå $1 av $2',
'previousrevision'    => '←Eldre versjon',
'nextrevision'        => 'Nyare versjon→',
'currentrevisionlink' => 'Noverande versjon',
'cur'                 => 'no',
'next'                => 'neste',
'last'                => 'førre',
'page_first'          => 'fyrste',
'page_last'           => 'siste',
'histlegend'          => 'Merk av for dei versjonane du vil samanlikne og trykk [Enter] eller klikk på knappen nedst på sida.<br />Forklaring: (no) = skilnad frå den noverande versjonen, (førre) = skilnad frå den førre versjonen, <b>s</b> = småplukk',
'deletedrev'          => '[sletta]',
'histfirst'           => 'Første',
'histlast'            => 'Siste',
'historysize'         => '({{PLURAL:$1|1 byte|$1 byte}})',
'historyempty'        => '(tom)',

# Revision feed
'history-feed-title'          => 'Endringshistorikk',
'history-feed-description'    => 'Endringshistorikk for denne sida på wikien',
'history-feed-item-nocomment' => '$1 på $2', # user at time
'history-feed-empty'          => 'Den etterspurde sida finst ikkje. Ho kan vere sletta frå wikien, eller vere flytta. Prøv å [[Special:Search|søke på wikien]] for relevante nye sider.',

# Revision deletion
'rev-deleted-comment'         => '(samandraget er fjerna)',
'rev-deleted-user'            => '(brukarnamnet er fjerna)',
'rev-deleted-event'           => '(fjerna loggoppføring)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">Denne versjonen av sida er fjerna frå den offentlege historikken. Det kan ligge detaljar om dette i [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} sletteloggen].</div>',
'rev-deleted-text-view'       => '<div class="mw-warning plainlinks">Denne versjonen av sida er fjerna frå den offentlege historikken, men du som administrator på {{SITENAME}} kan sjå han. Det kan ligge detaljar om fjerninga i [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} sletteloggen].</div>',
'rev-delundel'                => 'vis/gøym',
'revisiondelete'              => 'Slett/attopprett versjonar',
'revdelete-nooldid-title'     => 'Ugyldig målversjon',
'revdelete-nooldid-text'      => 'Du har ikkje oppgjeve kva for versjon(ar) du vil utføre denne handlinga på, versjonen eksisterer ikkje, eller du prøver å gøyme den noverande versjonen.',
'revdelete-selected'          => '{{PLURAL:$2|Vald versjon|Valde versjonar}} av [[:$1]]:',
'logdelete-selected'          => '{{PLURAL:$1|Vald loggoppføring|Valde loggoppføringar}} for [[:$1]]:',
'revdelete-text'              => 'Sletta versjonar og oppføringar vert framleis synlege i sidehistorikken og loggane, men delar av innhaldet deira vert ikkje lenger offentleggjort.

Andre administratorar på {{SITENAME}} kan framleis sjå det gøymde innhaldet og attopprette det, med mindre fleire avgrensingar vert lagde inn av sideoperatørane.',
'revdelete-legend'            => 'Vel avgrensing for synlegdom',
'revdelete-hide-text'         => 'Gøym endringssamandraga',
'revdelete-hide-name'         => 'Gøym handling og sidenamn',
'revdelete-hide-comment'      => 'Gøym endringssamandraga',
'revdelete-hide-user'         => 'Gøym brukarnamn/IP-adresse',
'revdelete-hide-restricted'   => 'La desse avgrensingane gjelde for administratorar også, og steng dette grensesnittet',
'revdelete-suppress'          => 'Fjern informasjon frå administratorar også',
'revdelete-hide-image'        => 'Skjul filinnhald',
'revdelete-unsuppress'        => 'Fjern avgrensingane på dei attoppretta versjonane',
'revdelete-log'               => 'Kommentar:',
'revdelete-submit'            => 'Utfør på vald versjon',
'revdelete-logentry'          => 'endra versjonsvisinga til [[$1]]',
'logdelete-logentry'          => 'endra visinga av loggoppføringane til [[$1]]',
'revdelete-success'           => "'''Versjonsvisinga er endra.'''",
'logdelete-success'           => "'''Visinga av loggoppføringar er endra.'''",
'revdel-restore'              => 'Endr synlegheita',
'pagehist'                    => 'Sidehistorikk',
'deletedhist'                 => 'Sletta historikk',
'revdelete-content'           => 'innhald',
'revdelete-summary'           => 'Samandrag',
'revdelete-uname'             => 'brukarnamn',
'revdelete-restricted'        => 'la til avgrensingar for administratorar',
'revdelete-unrestricted'      => 'fjerna avgrensingar for administratorar',
'revdelete-hid'               => 'løynde $1',
'revdelete-unhid'             => 'gjorde $1 synleg',
'revdelete-log-message'       => '$1 for $2 {{PLURAL:$2|revisjon|revisjonar}}',
'logdelete-log-message'       => '$1 for {{PLURAL:$2|éitt element|$2 element}}',

# Suppression log
'suppressionlog'     => 'Logg over historikkfjerningar',
'suppressionlogtext' => 'Under er ei liste over slettingar og blokkeringar som er gøymde frå administratorane.
Sjå [[Special:IPBlockList|blokkeringslista]] for oversikta over gjeldande blokkeringar.',

# History merging
'mergehistory'                     => 'Flett sidehistorikkar',
'mergehistory-header'              => 'Denne sida lar deg flette historikken til to sider.
Pass på at den nye sida også har innhald frå den innfletta sida.',
'mergehistory-box'                 => 'Flett historikkane til to sider:',
'mergehistory-from'                => 'Kjeldeside',
'mergehistory-into'                => 'Målside:',
'mergehistory-list'                => 'Flettbar endringshistorikk',
'mergehistory-merge'               => 'Versjonane nedanfor frå [[:$1]] kan flettast med [[:$2]]. Du kan velje å berre flette dei versjonane som kom før tidspunktet som er oppgjeve i tabellen. Merk at bruk av lenkjene nullstiller denne kolonnen.',
'mergehistory-go'                  => 'Vis flettbare endringar',
'mergehistory-submit'              => 'Flett versjonane',
'mergehistory-empty'               => 'Ingen endringar kan flettast.',
'mergehistory-success'             => '{{PLURAL:$3|Éin versjon|$3 versjonar}} av [[:$1]] er fletta til [[:$2]].',
'mergehistory-fail'                => 'Kunne ikkje utføre fletting av historikkane, ver venleg og dobbelsjekk sidene og versjonane du har valt.',
'mergehistory-no-source'           => 'Kjeldesida $1 finst ikkje.',
'mergehistory-no-destination'      => 'Målsida $1 finst ikkje.',
'mergehistory-invalid-source'      => 'Kjeldesida må ha ein gyldig tittel.',
'mergehistory-invalid-destination' => 'Målsida må ha ein gyldig tittel.',
'mergehistory-autocomment'         => 'Fletta «[[:$1]]» inn i «[[:$2]]»',
'mergehistory-comment'             => 'Fletta «[[:$1]]» inn i «[[:$2]]»: $3',

# Merge log
'mergelog'           => 'Flettingslogg',
'pagemerge-logentry' => 'fletta [[$1]] til [[$2]] (versjonar fram til $3)',
'revertmerge'        => 'Fjern fletting',
'mergelogpagetext'   => 'Nedanfor finn du ei liste over dei siste flettingane av ein sidehistorikk til ein annan.',

# Diffs
'history-title'           => 'Historikken til «$1»',
'difference'              => '(Skilnad mellom versjonar)',
'lineno'                  => 'Line $1:',
'compareselectedversions' => 'Samanlikn valde versjonar',
'editundo'                => 'angre',
'diff-multi'              => '({{PLURAL:$1|Éin versjon mellom desse er ikkje vist|$1 versjonar mellom desse er ikkje viste}}.)',

# Search results
'searchresults'             => 'Søkjeresultat',
'searchresulttext'          => 'For meir info om søkjefunksjonen i {{SITENAME}}, sjå [[{{MediaWiki:Helppage}}|Hjelp]].',
'searchsubtitle'            => 'Du søkte etter «[[:$1]]» ([[Special:Prefixindex/$1|alle sider som byrjar med «$1»]] | [[Special:WhatLinksHere/$1|alle sider som lenkjer til «$1»]])',
'searchsubtitleinvalid'     => 'Du søkte etter «$1»',
'noexactmatch'              => "* '''{{SITENAME}} har ikkje noka side med [[:$1|dette namnet]].'''
* <big>'''Du kan [[:$1|opprette ho no]]'''</big>.<br />
(Men du bør søkje etter andre namnevariasjonar først, slik at du ikkje lagar ei side som allereie finst under eit anna namn!)",
'noexactmatch-nocreate'     => "'''Inga side har tittelen «$1».'''",
'toomanymatches'            => 'Søket gav for mange treff, prøv ei anna spørjing',
'titlematches'              => 'Sidetitlar med treff på førespurnaden',
'notitlematches'            => 'Ingen sidetitlar hadde treff på førespurnaden',
'textmatches'               => 'Sider med treff på førespurnaden',
'notextmatches'             => 'Ingen sider hadde treff på førespurnaden',
'prevn'                     => 'førre $1',
'nextn'                     => 'neste $1',
'viewprevnext'              => 'Vis ($1) ($2) ($3).',
'search-result-size'        => '$1 ({{PLURAL:$2|eitt|$2}} ord)',
'search-result-score'       => 'Relevans: $1&nbsp;%',
'search-redirect'           => '(omdirigering $1)',
'search-section'            => '(bolk $1)',
'search-suggest'            => 'Meinte du: «$1»',
'search-interwiki-caption'  => 'Systerprosjekt',
'search-interwiki-default'  => '$1 resultat:',
'search-interwiki-more'     => '(meir)',
'search-mwsuggest-enabled'  => 'med forslag',
'search-mwsuggest-disabled' => 'ingen forslag',
'search-relatedarticle'     => 'Relaterte',
'mwsuggest-disable'         => 'Slå av AJAX-forslag',
'searchrelated'             => 'relaterte',
'searchall'                 => 'alle',
'showingresults'            => "Nedanfor er opp til {{PLURAL:$1|'''éitt''' resultat|'''$1''' resultat}} som byrjar med nummer '''$2''' vist.",
'showingresultsnum'         => "Nedanfor er {{PLURAL:$3|'''éitt''' resultat|'''$3''' resultat}} som byrjar med nummer '''$2''' vist.",
'showingresultstotal'       => "Under er {{PLURAL:$3|resultat '''$1''' av '''$3'''|resultata '''$1 - $2''' av '''$3'''}} viste",
'nonefound'                 => "'''Merk''': søk utan resultat kan komma av at du leitar etter alminnelege engelske ord som ikkje blir indekserte, eller det kan komma av at du har gjeve meir enn eitt søkjeord (berre sider som inneheld alle søkjeorda vil bli funne).",
'powersearch'               => 'Søk',
'powersearch-legend'        => 'Avansert søk',
'powersearch-ns'            => 'Søk i namnerom:',
'powersearch-redir'         => 'Vis omdirigeringar',
'powersearch-field'         => 'Søk etter',
'search-external'           => 'Eksternt søk',
'searchdisabled'            => 'Søkjefunksjonen på {{SITENAME}} er slått av akkurat no.
I mellomtida kan du søkje gjennom Google.
Ver merksam på at registra deira kan vera utdaterte.',

# Preferences page
'preferences'              => 'Innstillingar',
'mypreferences'            => 'Innstillingane mine',
'prefs-edits'              => 'Tal på endringar:',
'prefsnologin'             => 'Ikkje innlogga',
'prefsnologintext'         => 'Du må vere <span class="plainlinks">[{{fullurl:Special:Userlogin|returnto=$1}} logga inn]</span> for å endre brukarinnstillingane.',
'prefsreset'               => 'Innstillingane er tilbakestilte til siste lagra versjon.',
'qbsettings'               => 'Snøggmeny',
'qbsettings-none'          => 'Ingen',
'qbsettings-fixedleft'     => 'Venstre',
'qbsettings-fixedright'    => 'Høgre',
'qbsettings-floatingleft'  => 'Flytande venstre',
'qbsettings-floatingright' => 'Flytande høgre',
'changepassword'           => 'Skift passord',
'skin'                     => 'Drakt',
'math'                     => 'Matematiske formlar',
'dateformat'               => 'Datoformat',
'datedefault'              => 'Standard',
'datetime'                 => 'Dato og klokkeslett',
'math_failure'             => 'Klarte ikkje å tolke formelen',
'math_unknown_error'       => 'ukjend feil',
'math_unknown_function'    => 'ukjend funksjon',
'math_lexing_error'        => 'lexerfeil',
'math_syntax_error'        => 'syntaksfeil',
'math_image_error'         => 'PNG-konverteringa var mislukka; sjekk at latex, dvips, gs, og convert er rett installerte',
'math_bad_tmpdir'          => 'Kan ikkje skrive til eller laga mellombels mattemappe',
'math_bad_output'          => 'Kan ikkje skrive til eller laga mattemappe',
'math_notexvc'             => 'Manglar texvc-program; sjå math/README for konfigurasjon.',
'prefs-personal'           => 'Brukaropplysningar',
'prefs-rc'                 => 'Siste endringar',
'prefs-watchlist'          => 'Overvakingsliste',
'prefs-watchlist-days'     => 'Tal på dagar som skal visast i overvakingslista:',
'prefs-watchlist-edits'    => 'Talet på endringar som vert viste i den utvida overvakingslista:',
'prefs-misc'               => 'Andre',
'saveprefs'                => 'Lagre',
'resetprefs'               => 'Rull attende',
'oldpassword'              => 'Gammalt passord',
'newpassword'              => 'Nytt passord',
'retypenew'                => 'Nytt passord om att',
'textboxsize'              => 'Endring',
'rows'                     => 'Rekkjer',
'columns'                  => 'Kolonnar',
'searchresultshead'        => 'Søk',
'resultsperpage'           => 'Resultat per side',
'contextlines'             => 'Liner per resultat',
'contextchars'             => 'Teikn per line i resultatet',
'stub-threshold'           => 'Grense (i byte) for at frø/spirer skal formaterast <a href="#" class="stub">slik</a>:',
'recentchangesdays'        => 'Tal dagar som skal visast på siste endringar:',
'recentchangescount'       => 'Tal titlar på «siste endringar»',
'savedprefs'               => 'Brukarinnstillingane er lagra.',
'timezonelegend'           => 'Tidssone',
'timezonetext'             => 'Tal timar lokal tid skil seg frå tenaren si tid.',
'localtime'                => 'Lokaltid',
'timezoneoffset'           => 'Skilnad',
'servertime'               => 'Tenartid',
'guesstimezone'            => 'Hent tidssone frå nettlesaren',
'allowemail'               => 'Tillat e-post frå andre brukarar',
'prefs-searchoptions'      => 'Søkjealternativ',
'prefs-namespaces'         => 'Namnerom',
'defaultns'                => 'Søk som standard i desse namneromma:',
'default'                  => 'standard',
'files'                    => 'Filer',

# User rights
'userrights'                  => 'Administrering av brukartilgang', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => 'Administrer brukargrupper',
'userrights-user-editname'    => 'Skriv inn brukarnamn:',
'editusergroup'               => 'Endre brukargrupper',
'editinguser'                 => "Endrar brukartilgangen til '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup'    => 'Endre brukargrupper',
'saveusergroups'              => 'Lagre brukargrupper',
'userrights-groupsmember'     => 'Medlem av:',
'userrights-groups-help'      => 'Du kan endre kva for grupper denne brukaren er medlem av.
* Ein krossa boks tyder at brukaren er medlem av denne gruppa.
* Ein ikkjekrossa boks tyder at brukaren ikkje er medlem av denne gruppa.
* Ein * tyder at du ikkje kan fjerna gruppemedlemskapen etter at du har lagt den til, eller omvendt.',
'userrights-reason'           => 'Grunn til endring:',
'userrights-no-interwiki'     => 'Du har ikkje tilgang til å endre brukartilgangar på andre wikiar.',
'userrights-nodatabase'       => 'Databasen $1 finst ikkje eller er ikkje lokal.',
'userrights-nologin'          => 'Du må [[Special:UserLogin|logge inn]] med ein administrator- og/eller byråkratkonto for å endre brukartilgangar.',
'userrights-notallowed'       => 'Kontoen din har ikkje tilgang til å endre brukartilgangar.',
'userrights-changeable-col'   => 'Grupper du kan endre',
'userrights-unchangeable-col' => 'Grupper du ikkje kan endre',

# Groups
'group'               => 'Gruppe:',
'group-user'          => 'Brukarar',
'group-autoconfirmed' => 'Automatisk godkjende brukarar',
'group-bot'           => 'Robotar',
'group-sysop'         => 'Administratorar',
'group-bureaucrat'    => 'Byråkratar',
'group-suppress'      => 'Historikkfjernarar',
'group-all'           => '(alle)',

'group-user-member'          => 'Brukar',
'group-autoconfirmed-member' => 'Automatisk godkjend brukar',
'group-bot-member'           => 'Robot',
'group-sysop-member'         => 'Administrator',
'group-bureaucrat-member'    => 'Byråkrat',
'group-suppress-member'      => 'Historikkfjernar',

'grouppage-user'          => '{{ns:project}}:Brukarar',
'grouppage-autoconfirmed' => '{{ns:project}}:Automatisk godkjende brukarar',
'grouppage-bot'           => '{{ns:project}}:Robotar',
'grouppage-sysop'         => '{{ns:project}}:Administratorar',
'grouppage-bureaucrat'    => '{{ns:project}}:Byråkratar',
'grouppage-suppress'      => '{{ns:project}}:Historikkfjerning',

# Rights
'right-read'                 => 'Sjå sider',
'right-edit'                 => 'Endre sider',
'right-createpage'           => 'Opprette sider (som ikkje er diskusjonssider)',
'right-createtalk'           => 'Opprette diskusjonssider',
'right-createaccount'        => 'Opprette nye brukarkontoar',
'right-minoredit'            => 'Merke endringar som småplukk',
'right-move'                 => 'Flytte sider',
'right-move-subpages'        => 'Flytte sider med undersider',
'right-suppressredirect'     => 'Treng ikkje lage omdirigering frå det gamle namnet når sida vert flytta',
'right-upload'               => 'Laste opp filer',
'right-reupload'             => 'Skrive over ei eksisterande fil',
'right-reupload-own'         => 'Skrive over eigne filer',
'right-reupload-shared'      => 'Skrive over delte filer lokalt',
'right-upload_by_url'        => 'Laste opp ei fil frå ei nettadresse',
'right-purge'                => 'Reinse mellomlageret for sider',
'right-autoconfirmed'        => 'Endre halvlåste sider',
'right-bot'                  => 'Bli handsama som ein automatisk prosess.',
'right-nominornewtalk'       => 'Mindre endringar på diskujsonssida gjev ikkje beskjed om at du har nye meldingar.',
'right-apihighlimits'        => 'Bruke API med høgare grenser',
'right-writeapi'             => 'Redigere via API',
'right-delete'               => 'Slette sider',
'right-bigdelete'            => 'Slette sider med lange historikkar',
'right-deleterevision'       => 'Slette og gjenopprette enkeltendringar av sider',
'right-deletedhistory'       => 'Sjå sletta sidehistorikk utan tilhøyrande sidetekst',
'right-browsearchive'        => 'Søk i sletta sider',
'right-undelete'             => 'Attopprett sider',
'right-suppressrevision'     => 'Sjå og gjenopprett skjulte siderevisjonar',
'right-suppressionlog'       => 'Sjå private loggar',
'right-block'                => 'Blokkere andre brukarar frå å redigere',
'right-blockemail'           => 'Blokkere brukarar frå å sende e-post',
'right-hideuser'             => 'Blokkere eit brukarnamn og skjule det frå ålmenta.',
'right-ipblock-exempt'       => 'Kan gjere endringar frå blokkerte IP-adresser',
'right-proxyunbannable'      => 'Kan gjere endringar frå blokkerte proxyar',
'right-protect'              => 'Endre vernenivå',
'right-editprotected'        => 'Endre verna sider',
'right-editinterface'        => 'Redigere brukargrensesnittet',
'right-editusercssjs'        => 'Endre andre brukarar sine CSS- og JS-filer',
'right-rollback'             => 'Raskt tilbakestille den siste brukaren som har endra ei viss side',
'right-markbotedits'         => 'Markere tilbakerullingar som robotendringar',
'right-noratelimit'          => 'Vert ikkje påverka av snøggleiksgrenser',
'right-import'               => 'Importere sider frå andre wikiar',
'right-importupload'         => 'Importere sider via opplasting',
'right-patrol'               => 'Markere endringar som godkjende',
'right-autopatrol'           => 'Får sine eigne endringar merka som godkjende',
'right-patrolmarks'          => 'Vis godkjende endringar i siste endringar',
'right-unwatchedpages'       => 'Sjå lista over sider som ikkje er overvaka',
'right-trackback'            => 'Gje tilbakemelding',
'right-mergehistory'         => 'Flette sidehistorikkar',
'right-userrights'           => 'Endre alle brukarrettar',
'right-userrights-interwiki' => 'Endre rettar for brukarar på andre wikiar',
'right-siteadmin'            => 'Låse og låse opp databasen',

# User rights log
'rightslog'      => 'Brukartilgangslogg',
'rightslogtext'  => 'Dette er ein logg over endringar av brukartilgang.',
'rightslogentry' => 'endra brukartilgangen til $1 frå $2 til $3',
'rightsnone'     => '(ingen)',

# Recent changes
'nchanges'                          => '{{PLURAL:$1|Éi endring|$1 endringar}}',
'recentchanges'                     => 'Siste endringar',
'recentchangestext'                 => 'På denne sida ser du dei sist endra sidene i {{SITENAME}}.',
'recentchanges-feed-description'    => 'Fylg med på dei siste endringane på denne wikien med dette abonnementet.',
'rcnote'                            => "Nedanfor er {{PLURAL:$1|den siste endringa|dei siste '''$1''' endringane}} gjort {{PLURAL:$2|den siste dagen|dei siste '''$2''' dagane}}, sidan $4, kl. $5.",
'rcnotefrom'                        => 'Nedanfor er endringane frå <b>$2</b> inntil <b>$1</b> viste.',
'rclistfrom'                        => 'Vis nye endringar frå $1',
'rcshowhideminor'                   => '$1 småplukk',
'rcshowhidebots'                    => '$1 robotar',
'rcshowhideliu'                     => '$1 innlogga brukarar',
'rcshowhideanons'                   => '$1 anonyme brukarar',
'rcshowhidepatr'                    => '$1 godkjende endringar',
'rcshowhidemine'                    => '$1 endringane mine',
'rclinks'                           => 'Vis siste $1 endringar dei siste $2 dagane<br />$3',
'diff'                              => 'skil',
'hist'                              => 'hist',
'hide'                              => 'gøym',
'show'                              => 'vis',
'minoreditletter'                   => 's',
'newpageletter'                     => 'n',
'boteditletter'                     => 'b',
'number_of_watching_users_pageview' => '[{{PLURAL:$1|Éin brukar|$1 brukarar}} overvakar]',
'rc_categories'                     => 'Avgrens til kategoriar (skilde med «|»)',
'rc_categories_any'                 => 'Alle',
'newsectionsummary'                 => '/* $1 */ ny seksjon',

# Recent changes linked
'recentchangeslinked'          => 'Relaterte endringar',
'recentchangeslinked-title'    => 'Endringar relaterte til "$1"',
'recentchangeslinked-noresult' => 'Det er ikkje gjort endringar på sidene som var lenkja hit i den oppgjevne perioden.',
'recentchangeslinked-summary'  => "Denne spesialsida inneheld alle endringane som er gjort på sider som vert ''lenkja til'' frå denne (eller på sider i ein viss kategori). Dei av sidene du har på [[Special:Watchlist|overvakingslista]] di er '''utheva'''.",
'recentchangeslinked-page'     => 'Sidnamn:',
'recentchangeslinked-to'       => 'Vis endringar på sider som lenkjer til den gitte sida i staden',

# Upload
'upload'                      => 'Last opp fil',
'uploadbtn'                   => 'Last opp fil',
'reupload'                    => 'Nytt forsøk',
'reuploaddesc'                => 'Attende til opplastingsskjemaet.',
'uploadnologin'               => 'Ikkje innlogga',
'uploadnologintext'           => 'Du lyt vera [[Special:UserLogin|innlogga]] for å kunna laste opp filer.',
'upload_directory_missing'    => 'Opplastingsmappa ($1) manglar og kunne ikkje opprettast av tenaren.',
'upload_directory_read_only'  => 'Opplastingsmappa ($1) er skriveverna.',
'uploaderror'                 => 'Feil under opplasting av fil',
'uploadtext'                  => "Bruk skjemaet under for å laste opp filer.
For å sjå eller søkje i eksisterande filer, gå til [[Special:ImageList|fillista]]. Opplastingar vert òg lagra i [[Special:Log/upload|opplastingsloggen]].

For å bruke ei fil på ei side, bruk ei slik lenkje:
*'''<tt><nowiki>[[</nowiki>{{ns:image}}:Filnavn.jpg<nowiki>]]</nowiki></tt>''' for å bruke biletet i opphavleg form
*'''<tt><nowiki>[[</nowiki>{{ns:image}}:Filnavn.png|200px|mini|venstre|Alternativ tekst<nowiki>]]</nowiki></tt>''' for å bruke bilete med ei breidde på 200&nbsp;pikslar, venstrestilt og med «Alternativ tekst» som bilettekst
*'''<tt><nowiki>[[</nowiki>{{ns:media}}:Filnavn.ogg<nowiki>]]</nowiki></tt>''' for å lenkje direkte til fila utan å vise ho",
'upload-permitted'            => 'Godtekne filtypar: $1.',
'upload-preferred'            => 'Føretrekte filtypar: $1.',
'upload-prohibited'           => 'Ikkje godtekne filtypar: $1.',
'uploadlog'                   => 'opplastingslogg',
'uploadlogpage'               => 'Opplastingslogg',
'uploadlogpagetext'           => 'Dette er ei liste over filer som nyleg er lasta opp.',
'filename'                    => 'Filnamn',
'filedesc'                    => 'Skildring',
'fileuploadsummary'           => 'Skildring:',
'filestatus'                  => 'Opphavsrettsstatus:',
'filesource'                  => 'Kjelde:',
'uploadedfiles'               => 'Filer som er opplasta',
'ignorewarning'               => 'Oversjå åtvaringa og lagre fila',
'ignorewarnings'              => 'Oversjå åtvaringar',
'minlength1'                  => 'Filnamn må ha minst éitt teikn.',
'illegalfilename'             => 'Filnamnet «$1» inneheld teikn som ikkje er tillatne i sidetitlar. Skift namn på fila og prøv på nytt.',
'badfilename'                 => 'Namnet på fila har vorte endra til «$1».',
'filetype-badmime'            => 'Filer av MIME-typen «$1» kan ikkje lastast opp.',
'filetype-unwanted-type'      => "«'''.$1'''» er ein uynskt filtype.
{{PLURAL:$3|Føretrekt filtype er|Føretrekte filtypar er}} $2.",
'filetype-banned-type'        => "«'''.$1'''» er ikkje ein tillaten filtype.
{{PLURAL:$3|Tillaten filtype er|Tillatne filtypar er}} $2.",
'filetype-missing'            => 'Fila har inga ending (som t.d. «.jpg»).',
'large-file'                  => 'Det er tilrådd at filene ikkje er større enn $1, denne fila er $2.',
'largefileserver'             => 'Denne fila er større enn det tenaren tillèt.',
'emptyfile'                   => 'Det ser ut til at fila du lasta opp er tom. Dette kan komma av ein skrivefeil i filnamnet. Sjekk og tenk etter om du verkeleg vil laste opp fila.',
'fileexists'                  => 'Ei fil med dette namnet finst allereie, sjekk <strong><tt>$1</tt></strong> om du ikkje er sikker på om du vil endre namnet.',
'filepageexists'              => 'Skildringssida for denne fila finst allereie på <strong><tt>$1</tt></strong>, men det finst ikkje noka fil med dette namnet. Endringssamandraget du skriv inn vert ikkje vist på skildringssida. For at det skal dukke opp der, må du skrive det inn på skildringssida manuelt etter å ha lasta opp fila.',
'fileexists-extension'        => 'Ei fil med eit liknande namn finst allereie:<br />
Namnet på fila du lastar opp: <strong><tt>$1</tt></strong><br />
Namnet på den eksisterande fila: <strong><tt>$2</tt></strong><br />
Ver venleg og vel eit anna namn.',
'fileexists-thumb'            => "<center>'''Den eksisterande fila'''</center>",
'fileexists-thumbnail-yes'    => 'Fila ser ut til å vere eit bilete med redusert storleik. Ver venleg og sjekk fila <strong><tt>$1</tt></strong>.<br />
Dersom denne er det same biletet i original storleik, er det ikkje nødvendig å laste opp ein mindre versjon.',
'file-thumbnail-no'           => 'Filnamnet byrjar med <strong><tt>$1</tt></strong>.
Det ser ut til å vere eit bilte med redusert storleik<i>(miniatyrbilete)</i>.
Om du har dette bilete i stor utgåve, så last det opp eller endre filnamnet på denne fila.',
'fileexists-forbidden'        => 'Ei fil med dette namnet finst allereie, ver venleg og gå tilbake og last opp denne fila med eit anna namn. [[Image:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => 'Ei fil med dette namnet finst frå før i det delte fillageret.
Om du framleis ønskjer å laste opp fila, gå tilbake og last ho opp med eit anna namn. [[Image:$1|thumb|center|$1]]',
'file-exists-duplicate'       => 'Denne fila er ein duplikat av følgjande {{PLURAL:$1|fil|filer}}:',
'successfulupload'            => 'Opplastinga er ferdig',
'uploadwarning'               => 'Opplastingsåtvaring',
'savefile'                    => 'Lagre fil',
'uploadedimage'               => 'Lasta opp «[[$1]]»',
'overwroteimage'              => 'lasta opp ein ny versjon av «[[$1]]»',
'uploaddisabled'              => 'Beklagar, funksjonen for opplasting er deaktivert på denne nettenaren.',
'uploaddisabledtext'          => 'Filopplasting er slått av på {{SITENAME}}.',
'uploadscripted'              => 'Fila inneheld HTML- eller skriptkode som feilaktig kan bli tolka og køyrd av nettlesarar.',
'uploadcorrupt'               => 'Fila er øydelagd eller har feil etternamn. Sjekk fila og prøv på nytt.',
'uploadvirus'                 => 'Fila innheld virus! Detaljar: $1',
'sourcefilename'              => 'Filsti:',
'destfilename'                => 'Målfilnamn:',
'upload-maxfilesize'          => 'Maksimal filstorleik: $1',
'watchthisupload'             => 'Overvak denne sida',
'filewasdeleted'              => 'Ei fil med dette namnet har tidlegare vore lasta opp og sletta. Du bør sjekke $1 før du prøvar å laste henne opp att.',
'upload-wasdeleted'           => "'''Åtvaring: Du nyopplastar ei fil som tidlegare har vorte sletta.'''

Du bør tenkje over om det er lurt å halde fram med å laste opp denne fila.
Sletteloggen for fila finn du her:",
'filename-bad-prefix'         => 'Namnet på fila du lastar opp byrjar med <strong>«$1»</strong>, som er eit inkjeseiande namn som vanlegvis vert gjeve til bilete automatisk av digitale kamera. Ver venleg og vel eit meir skildrande namn på fila di.',

'upload-proto-error'      => 'Feil protokoll',
'upload-proto-error-text' => 'Fjernopplasting krev nettadresser som byrjar med <code>http://</code> eller <code>ftp://</code>.',
'upload-file-error'       => 'Intern feil',
'upload-file-error-text'  => 'Ein intern feil oppstod under forsøk på å lage ei mellombels fil på tenaren. Ver venleg og ta kontakt med ein [[Special:ListUsers/sysop|administrator]].',
'upload-misc-error'       => 'Ukjend feil ved opplastinga',
'upload-misc-error-text'  => 'Ein ukjend feil oppstod under opplastinga. Ver venleg og stadfest at nettadressa er gyldig og tilgjengeleg, og prøv ein gong til. Dersom problemet held fram, ta kontakt med ein [[Special:ListUsers/sysop|administrator]].',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => 'Kunne ikkje nå nettadressa',
'upload-curl-error6-text'  => 'Nettadressa som er oppgjeve kunne ikkje nåast. Ver venleg og dobbelsjekk at nettadressa er rett og at sida fungerer.',
'upload-curl-error28'      => 'Opplastinga fekk tidsavbrot',
'upload-curl-error28-text' => 'Sida brukte for lang tid på å svare. Ver venleg og sjekk om sida fungerer, vent litt og prøv ein gong til. Det kan også vere lurt å prøve på ei tid med mindre nettrafikk.',

'license'            => 'Lisensiering:',
'nolicense'          => 'Ingen lisens er vald',
'license-nopreview'  => '(Førehandsvising er ikkje tilgjengeleg)',
'upload_source_url'  => ' (ei gyldig, offentleg tilgjengeleg nettadresse)',
'upload_source_file' => ' (ei fil på datamaskina di)',

# Special:ImageList
'imagelist-summary'     => 'Denne spesialsida viser alle opplasta filer. Dei sist opplasta filene vert viste på toppen som standard. Klikk på ei kolonneoverskrift for å byte sorteringsmetode.',
'imagelist_search_for'  => 'Søk etter filnamn:',
'imgfile'               => 'fil',
'imagelist'             => 'Filliste',
'imagelist_date'        => 'Dato',
'imagelist_name'        => 'Namn',
'imagelist_user'        => 'Brukar',
'imagelist_size'        => 'Storleik',
'imagelist_description' => 'Beskriving',

# Image description page
'filehist'                       => 'Filhistorikk',
'filehist-help'                  => 'Klikk på dato/klokkeslett for å sjå fila slik ho var på det tidspunktet.',
'filehist-deleteall'             => 'slett alle',
'filehist-deleteone'             => 'slett',
'filehist-revert'                => 'rull attende',
'filehist-current'               => 'noverande',
'filehist-datetime'              => 'Dato/klokkeslett',
'filehist-user'                  => 'Brukar',
'filehist-dimensions'            => 'Oppløysing',
'filehist-filesize'              => 'Filstorleik',
'filehist-comment'               => 'Kommentar',
'imagelinks'                     => 'Fillenkjer',
'linkstoimage'                   => '{{PLURAL:$1|Den følgjande sida|Dei følgjande $1 sidene}} har lenkjer til denne fila:',
'nolinkstoimage'                 => 'Det finst ikkje noka side med lenkje til denne fila.',
'morelinkstoimage'               => 'Vis [[Special:WhatLinksHere/$1|fleire lenkjer]] til denne fila.',
'redirectstofile'                => 'Følgjande {{PLURAL:$1|fil er ei omdirigering|filer er omdirigeringar}} til denne fila:',
'duplicatesoffile'               => 'Følgjande {{PLURAL:$1|fil er ein dublett|filer er dublettar}} av denne fila:',
'sharedupload'                   => 'Denne fila er ei delt opplasting og kan brukast av andre prosjekt.',
'shareduploadwiki'               => 'Sjå $1 for meir informasjon.',
'shareduploadwiki-desc'          => 'Skildringa til $1 i det delte lageret er vist nedanfor.',
'shareduploadwiki-linktext'      => 'filskildringssida',
'shareduploadduplicate'          => 'Denne fila er ein kopi av $1 frå det delte fillageret.',
'shareduploadduplicate-linktext' => 'ei anna fil',
'shareduploadconflict'           => 'Denne fila har same namn som $1 frå det delte lagringsområdet.',
'shareduploadconflict-linktext'  => 'ei anna fil',
'noimage'                        => 'Det finst inga fil med dette namnet, men du kan $1.',
'noimage-linktext'               => 'laste opp eitt',
'uploadnewversion-linktext'      => 'Last opp ny versjon av denne fila',
'imagepage-searchdupe'           => 'Søk etter filer som ligg dobbelt',

# File reversion
'filerevert'                => 'Rull attende $1',
'filerevert-legend'         => 'Rull attende fila',
'filerevert-intro'          => "Du rullar attende '''[[Media:$1|$1]]''' til [$4 versjonen frå $3, $2].",
'filerevert-comment'        => 'Kommentar:',
'filerevert-defaultcomment' => 'Rulla attende til versjonen frå $2, $1',
'filerevert-submit'         => 'Rull attende',
'filerevert-success'        => "'''[[Media:$1|$1]]''' er rulla attende til [$4 versjonen frå $3, $2].",
'filerevert-badversion'     => 'Det finst ingen tidlegare lokal versjon av denne fila frå det oppgjevne tidspunktet.',

# File deletion
'filedelete'                  => 'Slett $1',
'filedelete-legend'           => 'Slett fil',
'filedelete-intro'            => "Du slettar '''[[Media:$1|$1]]'''.",
'filedelete-intro-old'        => "Du slettar versjonen av '''[[Media:$1|$1]]''' frå [$4 $3, $2].",
'filedelete-comment'          => 'Slettingsårsak:',
'filedelete-submit'           => 'Slett',
'filedelete-success'          => "'''$1''' er sletta.",
'filedelete-success-old'      => "Versjonen av '''[[Media:$1|$1]]''' frå $3, $2 er sletta.",
'filedelete-nofile'           => "'''$1''' finst ikkje.",
'filedelete-nofile-old'       => "Det finst ingen arkivert versjon av '''$1''' med dei oppgjevne attributta.",
'filedelete-iscurrent'        => 'Du prøver å slette den nyaste versjonen av denne fila. Ver venleg og rull attende til ein eldre versjon fyrst.',
'filedelete-otherreason'      => 'Annan grunn/tilleggsgrunn:',
'filedelete-reason-otherlist' => 'Annan grunn',
'filedelete-reason-dropdown'  => '*Vanlege grunnar for sletting
** Brot på opphavsretten
** Ligg dobbelt',
'filedelete-edit-reasonlist'  => 'Endre grunnar til sletting',

# MIME search
'mimesearch'         => 'MIME-søk',
'mimesearch-summary' => 'Denne sida gjer filtrering av filer etter MIME-type mogleg. Skriv inn: innhaldstype/undertype, t.d. <tt>image/jpeg</tt>.',
'mimetype'           => 'MIME-type:',
'download'           => 'last ned',

# Unwatched pages
'unwatchedpages' => 'Uovervaka sider',

# List redirects
'listredirects' => 'Omdirigeringsliste',

# Unused templates
'unusedtemplates'     => 'Ubrukte malar',
'unusedtemplatestext' => 'Denne sida viser alle sidene i mal-namnerommet som ikkje er brukte på andre sider. Hugs også å sjå etter andre lenkjer til malane før du slettar dei.',
'unusedtemplateswlh'  => 'andre lenkjer',

# Random page
'randompage'         => 'Tilfeldig side',
'randompage-nopages' => 'Det er ingen sider i dette namnerommet.',

# Random redirect
'randomredirect'         => 'Tilfeldig omdirigering',
'randomredirect-nopages' => 'Det er ingen omdirigeringar i dette namnerommet.',

# Statistics
'statistics'             => 'Statistikk',
'sitestats'              => '{{SITENAME}}-statistikk',
'userstats'              => 'Brukarstatistikk',
'sitestatstext'          => "Det er i alt {{PLURAL:$1|'''éi''' side|'''$1''' sider}} i databasen. Dette inkluderer diskusjonssider, sider om {{SITENAME}}, småsider,
omdirigeringssider, og andre som truleg ikkje kan kallast innhaldssider. Om ein ser bort frå desse sidene, er det {{PLURAL:$2|'''éi''' side|'''$2''' sider}} som truleg er innhaldssider.

'''$8''' {{PLURAL:$8|fil|filer}} har vorte lasta opp.

Alle sidene er vortne viste {{PLURAL:$3|'''éin''' gong|'''$3''' gonger}} og endra {{PLURAL:$4|'''éin''' gong|'''$4''' gonger}} sidan programvaren vart installert. Det vil seie at kvar side gjennomsnittleg har vorte endra {{PLURAL:$5|'''éin''' gong|'''$5''' gonger}}, og vist {{PLURAL:$6|'''éin''' gong|'''$6''' gonger}} per endring.

[http://www.mediawiki.org/wiki/Manual:Job_queue Jobbkøen] er '''$7'''.",
'userstatstext'          => "{{SITENAME}} har {{PLURAL:$1|'''éin''' registrert brukar|'''$1''' registrerte brukarar}}. '''$2''' (eller '''$4%''') av desse har $5rettar.",
'statistics-mostpopular' => 'Mest viste sider',

'disambiguations'      => 'Fleirtydingssider',
'disambiguationspage'  => 'Template:Fleirtyding',
'disambiguations-text' => "Sidene nedanfor har lenkje til ei '''fleirtydingsside'''. Dei bør ha lenkje til det rette oppslagsordet i staden for.<br />Sider vert handsama som fleirtydingssider dersom dei inneheld ein mal som har lenkje på [[MediaWiki:Disambiguationspage]].",

'doubleredirects'            => 'Doble omdirigeringar',
'doubleredirectstext'        => 'Kvar line inneheld lenkjer til den første og den andre omdirigeringa, og den første lina frå den andre omdirigeringsteksten. Det gjev som regel den «rette» målartikkelen, som den første omdirigeringa skulle ha peikt på.',
'double-redirect-fixed-move' => '[[$1]] har blitt flytta, og er no ei omdirigering til [[$2]]',
'double-redirect-fixer'      => 'Omdirigeringsfiksar',

'brokenredirects'        => 'Blindvegsomdirigeringar',
'brokenredirectstext'    => 'Dei følgjande omdirigeringane viser til ei side som ikkje finst.',
'brokenredirects-edit'   => '(endre)',
'brokenredirects-delete' => '(slett)',

'withoutinterwiki'         => 'Sider utan lenkjer til andre språk',
'withoutinterwiki-summary' => 'Desse sidene manglar lenkjer til sider på andre språk:',
'withoutinterwiki-legend'  => 'Prefiks',
'withoutinterwiki-submit'  => 'Vis',

'fewestrevisions' => 'Sidene med færrast endringar',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|byte|byte}}',
'ncategories'             => '$1 {{PLURAL:$1|kategori|kategoriar}}',
'nlinks'                  => '{{PLURAL:$1|Éi lenkje|$1 lenkjer}}',
'nmembers'                => '$1 {{PLURAL:$1|medlem|medlemmer}}',
'nrevisions'              => '{{PLURAL:$1|Éin versjon|$1 versjonar}}',
'nviews'                  => '{{PLURAL:$1|Éi vising|$1 visingar}}',
'specialpage-empty'       => 'Denne sida er tom.',
'lonelypages'             => 'Foreldrelause sider',
'lonelypagestext'         => 'Desse sidene er ikkje lenkja til på andre sider på {{SITENAME}}.',
'uncategorizedpages'      => 'Ikkje kategoriserte sider',
'uncategorizedcategories' => 'Ikkje kategoriserte kategoriar',
'uncategorizedimages'     => 'Ukategoriserte filer',
'uncategorizedtemplates'  => 'Ukategoriserte malar',
'unusedcategories'        => 'Ubrukte kategoriar',
'unusedimages'            => 'Ubrukte filer',
'popularpages'            => 'Populære sider',
'wantedcategories'        => 'Etterspurde kategoriar',
'wantedpages'             => 'Etterspurde sider',
'missingfiles'            => 'Manglande filer',
'mostlinked'              => 'Sidene med flest lenkjer til seg',
'mostlinkedcategories'    => 'Mest brukte kategoriar',
'mostlinkedtemplates'     => 'Mest brukte malar',
'mostcategories'          => 'Sidene med flest kategoriar',
'mostimages'              => 'Mest brukte filer',
'mostrevisions'           => 'Sidene med flest endringar',
'prefixindex'             => 'Prefiksindeks',
'shortpages'              => 'Korte sider',
'longpages'               => 'Lange sider',
'deadendpages'            => 'Blindvegsider',
'deadendpagestext'        => 'Desse sidene har ikkje lenkjer til andre sider på {{SITENAME}}.',
'protectedpages'          => 'Verna sider',
'protectedpages-indef'    => 'Berre vern på ubestemt tid',
'protectedpagestext'      => 'Desse sidene er verna mot flytting og endring',
'protectedpagesempty'     => 'Ingen sider er verna på den valde måten akkurat no.',
'protectedtitles'         => 'Verna sidenamn',
'protectedtitlestext'     => 'Desse sidene er verna mot oppretting',
'protectedtitlesempty'    => 'Ingen sider er verna på den valde måten akkurat no.',
'listusers'               => 'Brukarliste',
'newpages'                => 'Nye sider',
'newpages-username'       => 'Brukarnamn:',
'ancientpages'            => 'Eldste sider',
'move'                    => 'Flytt',
'movethispage'            => 'Flytt denne sida',
'unusedimagestext'        => '<p>Merk deg at andre internettsider kan ha lenkjer til filer som er lista her. Dei kan difor vera i aktiv bruk.</p>',
'unusedcategoriestext'    => 'Dei følgjande kategorisidene er oppretta, sjølv om ingen artikkel eller kategori brukar dei.',
'notargettitle'           => 'Inkje mål',
'notargettext'            => 'Du har ikkje spesifisert noka målside eller nokon brukar å bruke denne funksjonen på.',
'nopagetitle'             => 'Målsida finst ikkje',
'nopagetext'              => 'Sida du ville flytte finst ikkje.',
'pager-newer-n'           => '{{PLURAL:$1|nyare|nyare $1}}',
'pager-older-n'           => '{{PLURAL:$1|eldre|eldre $1}}',
'suppress'                => 'Historikkfjerning',

# Book sources
'booksources'               => 'Bokkjelder',
'booksources-search-legend' => 'Søk etter bokkjelder',
'booksources-go'            => 'Gå',
'booksources-text'          => 'Nedanfor finn du ei liste over lenkjer til andre nettstader som sel nye og brukte bøker, og desse kan ha meir informasjon om bøker du leitar etter:',

# Special:Log
'specialloguserlabel'  => 'Brukar:',
'speciallogtitlelabel' => 'Tittel:',
'log'                  => 'Loggar',
'all-logs-page'        => 'Alle loggane',
'log-search-legend'    => 'Søk i loggane',
'log-search-submit'    => 'Gå',
'alllogstext'          => 'Kombinert vising av alle loggane på {{SITENAME}}. Du kan avgrense resultatet ved å velje loggtype, brukarnamn eller den sida som er påverka (hugs å skilje mellom store og små bokstavar)',
'logempty'             => 'Ingen treff i loggane.',
'log-title-wildcard'   => 'Søk i titlar som byrjar med denne teksten',

# Special:AllPages
'allpages'          => 'Alle sider',
'alphaindexline'    => '$1 til $2',
'nextpage'          => 'Neste side ($1)',
'prevpage'          => 'Førre side ($1)',
'allpagesfrom'      => 'Vis sider frå:',
'allarticles'       => 'Alle sider',
'allinnamespace'    => 'Alle sider ($1 namnerom)',
'allnotinnamespace' => 'Alle sider (ikkje i $1-namnerommet)',
'allpagesprev'      => 'Førre',
'allpagesnext'      => 'Neste',
'allpagessubmit'    => 'Vis',
'allpagesprefix'    => 'Vis sider med prefikset:',
'allpagesbadtitle'  => 'Det oppgjevne sidenamnet var ugyldig eller hadde eit interwiki-prefiks. Det kan også ha hatt eitt eller fleire teikn som ikkje kan brukast i sidenamn.',
'allpages-bad-ns'   => '{{SITENAME}} har ikkje namnerommet «$1».',

# Special:Categories
'categories'                    => 'Kategoriar',
'categoriespagetext'            => 'Følgjande kategoriar inneheld sider eller media.
[[Special:UnusedCategories|Unytta kategoriar]] vert ikkje vist her. 
Sjå òg [[Special:WantedCategories|ønska kategoriar]].',
'categoriesfrom'                => 'Vis kategoriar frå og med:',
'special-categories-sort-count' => 'sorter etter storleik',
'special-categories-sort-abc'   => 'sorter alfabetisk',

# Special:ListUsers
'listusersfrom'      => 'Vis brukarnamna frå og med:',
'listusers-submit'   => 'Vis',
'listusers-noresult' => 'Ingen brukarnamn vart funne.',

# Special:ListGroupRights
'listgrouprights'          => 'Tilgangar for brukargrupper',
'listgrouprights-summary'  => 'Følgjande liste viser brukargruppene som er definert på denne wikien, og kvar rettar dei har. Meir informasjon om dei ulike rettane ein kan ha finn ein [[{{MediaWiki:Listgrouprights-helppage}}|her]].',
'listgrouprights-group'    => 'Gruppe',
'listgrouprights-rights'   => 'Tilgangar',
'listgrouprights-helppage' => 'Help:Gruppetilgangar',
'listgrouprights-members'  => '(liste over medlemmer)',

# E-mail user
'mailnologin'     => 'Inga avsendaradresse',
'mailnologintext' => 'Du lyt vera [[Special:UserLogin|innlogga]] og ha ei gyldig e-postadresse sett i [[Special:Preferences|brukarinnstillingane]] for å sende e-post åt andre brukarar.',
'emailuser'       => 'Send e-post åt denne brukaren',
'emailpage'       => 'Send e-post åt brukar',
'emailpagetext'   => 'Om denne brukaren har gjeve ei gyldig e-postadresse i brukarinnstillingane sine, vil dette skjemaet sende ei melding. E-postadressa di frå [[Special:Preferences|innstillingane dine]] vil vere synlege i «frå»-feltet i denne e-posten, slik at mottakaren kan svare deg.',
'usermailererror' => 'E-post systemet gav feilmelding:',
'defemailsubject' => '{{SITENAME}} e-post',
'noemailtitle'    => 'Inga e-postadresse',
'noemailtext'     => 'Denne brukaren har ikkje oppgjeve ei gyldig e-postadresse, eller har valt å ikkje opne for e-post frå andre brukarar.',
'emailfrom'       => 'Frå:',
'emailto'         => 'Åt:',
'emailsubject'    => 'Emne:',
'emailmessage'    => 'Melding:',
'emailsend'       => 'Send',
'emailccme'       => 'Send meg ein kopi av meldinga mi.',
'emailccsubject'  => 'Kopi av meldinga di til $1: $2',
'emailsent'       => 'E-posten er sendt',
'emailsenttext'   => 'E-posten er sendt.',
'emailuserfooter' => 'E-posten vart sendt av $1 til $2 via «Send e-post»-funksjonen på {{SITENAME}}.',

# Watchlist
'watchlist'            => 'Overvakingsliste',
'mywatchlist'          => 'Overvakingslista mi',
'watchlistfor'         => "(for '''$1''')",
'nowatchlist'          => 'Du har ikkje noko i overvakingslista di.',
'watchlistanontext'    => 'Ver venleg og $1 for å vise eller endre sider på overvakingslista di.',
'watchnologin'         => 'Ikkje innlogga',
'watchnologintext'     => 'Du lyt vera [[Special:UserLogin|innlogga]] for å kunna endre overvakingslista.',
'addedwatch'           => 'Lagt til overvakingslista',
'addedwatchtext'       => "Sida «[[:$1]]» er lagt til [[Special:Watchlist|overvakingslista]] di. Framtidige endringar av denne sida og den tilhøyrande diskusjonssida vil bli oppførde her, og sida vil vera '''utheva''' på «[[Special:RecentChanges|siste endringar]]» for å gjera deg merksam på henne.

Om du seinere vil fjerne sida frå overvakingslista, klikk på «Fjern overvaking» på den aktuelle sida.",
'removedwatch'         => 'Fjerna frå overvakingslista',
'removedwatchtext'     => 'Sida «<nowiki>$1</nowiki>» er fjerna frå overvakingslista.',
'watch'                => 'Overvak',
'watchthispage'        => 'Overvak denne sida',
'unwatch'              => 'Fjern overvaking',
'unwatchthispage'      => 'Fjern overvaking',
'notanarticle'         => 'Ikkje innhaldsside',
'notvisiblerev'        => 'Sideversjonen er sletta',
'watchnochange'        => 'Ingen av sidene i overvakingslista er endra i den valde perioden.',
'watchlist-details'    => '{{PLURAL:$1|Éi side|$1 sider}} er overvaka, utanom diskusjonssider.',
'wlheader-enotif'      => '* Funksjonen for endringsmeldingar per e-post er på.',
'wlheader-showupdated' => "* Sider som har vorte endra sidan du sist såg på dei er '''utheva'''",
'watchmethod-recent'   => 'sjekkar siste endringar for dei overvaka sidene',
'watchmethod-list'     => 'sjekkar om dei overvaka sidene er vortne endra i det siste',
'watchlistcontains'    => 'Overvakingslista di inneheld {{PLURAL:$1|éi side|$1 sider}}.',
'iteminvalidname'      => 'Problem med «$1», ugyldig namn...',
'wlnote'               => 'Nedanfor er {{PLURAL:$1|den siste endringa|dei siste $1 endringane}} {{PLURAL:$2|den siste timen|dei siste $2 timane}}.',
'wlshowlast'           => 'Vis siste $1 timar $2 dagar $3',
'watchlist-show-bots'  => 'Vis robotar',
'watchlist-hide-bots'  => 'Gøym robotar',
'watchlist-show-own'   => 'Vis endringane mine',
'watchlist-hide-own'   => 'Gøym endringane mine',
'watchlist-show-minor' => 'Vis småplukk',
'watchlist-hide-minor' => 'Gøym småplukk',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Overvakar...',
'unwatching' => 'Fjernar frå overvakinglista...',

'enotif_mailer'                => '{{SITENAME}}-endringsmeldingssendar',
'enotif_reset'                 => 'Merk alle sider som vitja',
'enotif_newpagetext'           => 'Dette er ei ny side.',
'enotif_impersonal_salutation' => '{{SITENAME}}-brukar',
'changed'                      => 'endra',
'created'                      => 'oppretta',
'enotif_subject'               => '{{SITENAME}}-sida $PAGETITLE har vorte $CHANGEDORCREATED av $PAGEEDITOR',
'enotif_lastvisited'           => 'Sjå $1 for alle endringane sidan siste vitjing.',
'enotif_lastdiff'              => 'Sjå $1 for å sjå denne endringa.',
'enotif_anon_editor'           => 'anonym brukar $1',
'enotif_body'                  => 'Hei $WATCHINGUSERNAME,

{{SITENAME}}-sida $PAGETITLE har vorte $CHANGEDORCREATED $PAGEEDITDATE av $PAGEEDITOR, sjå $PAGETITLE_URL for den gjeldande versjonen.

$NEWPAGE

Bidragytaren sitt endringssamandrag: $PAGESUMMARY $PAGEMINOREDIT

Du kan kontakte bidragsytaren gjennom:
e-post: $PAGEEDITOR_EMAIL , eller
wiki: $PAGEEDITOR_WIKI

Du får ikkje fleire endringsmeldingar om denne sida før du har vitja henne på nytt. Du kan også tilbakestille endringsmeldingsstatus for alle sidene på overvakingslista di.

             Helsing din overvakande {{SITENAME}}-endringsmeldingssystemven

--
For å endre innstillingane for overvakingslista di, gå til
{{fullurl:Special:Watchlist/edit}}

For hjelp og meir informasjon:
{{fullurl:Hjelp:Overvaking}}',

# Delete/protect/revert
'deletepage'                  => 'Slett side',
'confirm'                     => 'Stadfest',
'excontent'                   => 'innhaldet var: «$1»',
'excontentauthor'             => 'innhaldet var: «$1» (og den einaste bidragsytaren var «$2»)',
'exbeforeblank'               => 'innhaldet før sida vart tømd var: «$1»',
'exblank'                     => 'sida var tom',
'delete-confirm'              => 'Slett «$1»',
'delete-legend'               => 'Slett',
'historywarning'              => 'Åtvaring: Sida du held på å slette har ein historikk:',
'confirmdeletetext'           => 'Du held på å varig slette ei side eller eit bilete saman med heile den tilhøyrande historikken frå databasen. Stadfest at du verkeleg vil gjere dette, at du skjønar konsekvensane, og at du gjer dette i tråd med [[{{MediaWiki:Policy-url}}|retningslinene]].',
'actioncomplete'              => 'Ferdig',
'deletedtext'                 => '«<nowiki>$1</nowiki>» er sletta. Sjå $2 for eit oversyn over dei siste slettingane.',
'deletedarticle'              => 'sletta «[[$1]]»',
'suppressedarticle'           => 'gøymde «[[$1]]»',
'dellogpage'                  => 'Slettelogg',
'dellogpagetext'              => 'Her er ei liste over dei siste slettingane.',
'deletionlog'                 => 'slettelogg',
'reverted'                    => 'Attenderulla til ein tidlegare versjon',
'deletecomment'               => 'Slettingsårsak:',
'deleteotherreason'           => 'Anna grunn:',
'deletereasonotherlist'       => 'Anna grunn',
'deletereason-dropdown'       => '*Vanlege grunnar for sletting
** På førespurnad frå forfattren
** Brot på opphavsretten
** Hærverk',
'delete-edit-reasonlist'      => 'Endre grunnar til sletting',
'delete-toobig'               => 'Denne sida har ein stor endringsshistorikk, med over {{PLURAL:$1|$1&nbsp;endring|$1&nbsp;endringar}}. Sletting av slike sider er avgrensa for å unngå utilsikta forstyrring av {{SITENAME}}.',
'delete-warning-toobig'       => 'Denne sida har ein lang endringshistorikk, med meir enn {{PLURAL:$1|$1&nbsp;endring|$1&nbsp;endringar}}. Dersom du slettar henne kan det forstyrre handlingar i databasen til {{SITENAME}}, ver varsam.',
'rollback'                    => 'Rull attende endringar',
'rollback_short'              => 'Rull attende',
'rollbacklink'                => 'rull attende',
'rollbackfailed'              => 'Kunne ikkje rulle attende',
'cantrollback'                => 'Kan ikkje rulle attende fordi den siste brukaren er den einaste forfattaren.',
'alreadyrolled'               => 'Kan ikkje rulle attende den siste endringa av [[$1]] gjort av [[User:$2|$2]] ([[User talk:$2|diskusjon]] | [[Special:Contributions/$2|{{int:contribslink}}]]) fordi nokon andre alt har endra sida att eller fjerna endringa.

Den siste endringa vart gjort av [[User:$3|$3]] ([[User talk:$3|brukardiskusjon]] | [[Special:Contributions/$3|{{int:contribslink}}]]).',
'editcomment'                 => 'Samandraget for endringa var: "<i>$1</i>".', # only shown if there is an edit comment
'revertpage'                  => 'Attenderulla endring gjort av [[Special:Contributions/$2|$2]] til tidlegare versjon endra av [[User:$1|$1]]', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => 'Rulla attende endringane av $1, tilbake til siste versjon av $2.',
'sessionfailure'              => 'Det ser ut til å vera eit problem med innloggingsøkta di. Handlinga er vorten avbroten for å vera føre var mot kidnapping av økta. Bruk attendeknappen i nettlesaren din og prøv om att.',
'protectlogpage'              => 'Vernelogg',
'protectlogtext'              => 'Dette er ei liste over sider som er vortne verna eller har fått fjerna vern. [[Special:ProtectedPages|Verna side]] for meir info.',
'protectedarticle'            => 'verna «[[$1]]»',
'modifiedarticleprotection'   => 'endra nivået på vernet av «[[$1]]»',
'unprotectedarticle'          => 'fjerna vern av «[[$1]]»',
'protect-title'               => 'Vernar «$1»',
'protect-legend'              => 'Stadfest vern',
'protectcomment'              => 'Grunn til verning',
'protectexpiry'               => 'Utgår:',
'protect_expiry_invalid'      => 'Utløpstida er ugyldig.',
'protect_expiry_old'          => 'Utløpstida har allereie vore.',
'protect-unchain'             => 'Tillat flytting',
'protect-text'                => 'Her kan du kan sjå og endre på graden av vern for sida <strong><nowiki>$1</nowiki></strong>.',
'protect-locked-blocked'      => 'Du kan ikkje endre nivå på vern medan du er blokkert. Dette er dei noverande innstillingane for sida <strong>$1</strong>:',
'protect-locked-dblock'       => 'Du kan ikkje endre nivå på vern fordi databasen er låst akkurat no. Dette er dei noverande innstillingane for sida <strong>$1</strong>:',
'protect-locked-access'       => 'Brukarkontoen din har ikkje tilgang til endring av vern.
Her er dei noverande innstillingane for sida <strong>$1</strong>:',
'protect-cascadeon'           => 'Denne sida er verna fordi ho er inkludert på {{PLURAL:$1|den opplista sida|dei opplista sidene}} som har djupvern slått på. Du kan endre på nivået til vernet av denne sida, men det vil ikkje ha innverknad på djupvernet.',
'protect-default'             => '(standard)',
'protect-fallback'            => 'Må ha «$1»-tilgang',
'protect-level-autoconfirmed' => 'Blokker uregistrerte brukarar',
'protect-level-sysop'         => 'Berre administratorar',
'protect-summary-cascade'     => 'djupvern',
'protect-expiring'            => 'utgår $1 (UTC)',
'protect-cascade'             => 'Vern alle sidene som er inkludert på denne sida (djupvern)',
'protect-cantedit'            => 'Du kan ikkje endre på nivået på vernet av denne sida, fordi du ikkje har tilgang til å endre henne.',
'restriction-type'            => 'Tilgang:',
'restriction-level'           => 'Avgrensingsnivå:',
'minimum-size'                => 'Minimumstorleik',
'maximum-size'                => 'Maksimumstorleik:',
'pagesize'                    => '(byte)',

# Restrictions (nouns)
'restriction-edit'   => 'Endring',
'restriction-move'   => 'Flytting',
'restriction-create' => 'Opprett',
'restriction-upload' => 'Last opp',

# Restriction levels
'restriction-level-sysop'         => 'heilt verna',
'restriction-level-autoconfirmed' => 'delvis verna',
'restriction-level-all'           => 'alle nivå',

# Undelete
'undelete'                     => 'Sletta sider',
'undeletepage'                 => 'Sletta sider',
'undeletepagetitle'            => "'''Følgjande innhald er sletta versjonar av [[:$1]]'''.",
'viewdeletedpage'              => 'Sjå sletta sider',
'undeletepagetext'             => 'Dei følgjande sidene er sletta, men dei finst enno i arkivet og kan attopprettast. Arkivet blir periodevis sletta.',
'undelete-fieldset-title'      => 'Attenderull endringar',
'undeleteextrahelp'            => "For å attenderulle heile sida, la alle boksane vere som dei er, og klikk '''''Rull attende'''''.
For å berre attenderulle delar, kryss av boksane til endringane, og klikk '''''Rull attende'''''.
Å klikke '''''Nullstill''''' vil føre til at alle tekstfelt og boksar vert blanke.",
'undeleterevisions'            => '{{PLURAL:$1|Éin versjon arkivert|$1 versjonar arkiverte}}',
'undeletehistory'              => 'Om du gjenopprettar sida vil alle endringar i historikken også bli gjenoppretta. Dersom ei ny side med same namn er oppretta etter slettinga, vil dei gjenoppretta endringane dukke opp før denne i endringshistorikken.',
'undeleterevdel'               => 'Gjenoppretting kan ikkje utførast om det resulterer i at den øvste endringa delvis vert sletta. I slike tilfelle må du fjerne merkinga av den siste sletta endringa.',
'undeletehistorynoadmin'       => 'Ein eller fleire versjonar av denne sida har blitt sletta.
Grunnlaget for sletting er oppgjeve under, saman med informasjon om kven som sletta og når versjonane vart sletta.
Innhaldet i dei sletta versjonane er berre tilgjengeleg for administratorar.',
'undelete-revision'            => 'Sletta versjon av $1 av $3 (frå $2):',
'undeleterevision-missing'     => 'Ugyldig eller manglande versjon. Lenkja kan vere feil, eller han kan vere fjerna frå arkivet.',
'undelete-nodiff'              => 'Fann ingen eldre versjonar.',
'undeletebtn'                  => 'Attopprett',
'undeletelink'                 => 'attopprett',
'undeletereset'                => 'Nullstill',
'undeletecomment'              => 'Kommentar:',
'undeletedarticle'             => 'attoppretta «[[$1]]»',
'undeletedrevisions'           => '{{PLURAL:$1|Éin versjon|$1 versjonar}} attoppretta.',
'undeletedrevisions-files'     => '{{PLURAL:$1|Éin versjon|$1 versjonar}} og {{PLURAL:$2|éi fil|$2 filer}} er attoppretta',
'undeletedfiles'               => '{{PLURAL:$1|Éi fil|$1 filer}} er attoppretta',
'cannotundelete'               => 'Feil ved attoppretting, andre kan allereie ha attoppretta sida.',
'undeletedpage'                => "<big>'''$1 er attoppretta'''</big>

Sjå [[Special:Log/delete|sletteloggen]] for eit oversyn over sider som nyleg er sletta eller attoppretta.",
'undelete-header'              => 'Sjå [[Special:Log/delete|sletteloggen]] for dei sist sletta sidene.',
'undelete-search-box'          => 'Søk i sletta sider',
'undelete-search-prefix'       => 'Vis sider frå og med:',
'undelete-search-submit'       => 'Søk',
'undelete-no-results'          => 'Fann ingen treff i arkivet over sletta sider.',
'undelete-filename-mismatch'   => 'Filversjonen med tidstrykk $1 kan ikkje attopprettast: filnamnet samsvarer ikkje.',
'undelete-bad-store-key'       => 'Kan ikkje gjenopprette filutgåva med tidstrykk $1: fil mangla før sletting',
'undelete-cleanup-error'       => 'Feil ved sletting av den ubrukte arkivfila «$1».',
'undelete-missing-filearchive' => 'Kunne ikkje attopprette filarkivet med nummer $1 fordi det ikkje ligg i databasen. Det kan allereie ver attoppretta.',
'undelete-error-short'         => 'Veil ved sletting av fila: $1',
'undelete-error-long'          => 'Feil ved attoppretting av fila:

$1',

# Namespace form on various pages
'namespace'      => 'Namnerom:',
'invert'         => 'Vreng val',
'blanknamespace' => '(Hovud)',

# Contributions
'contributions' => 'Brukarbidrag',
'mycontris'     => 'Eigne bidrag',
'contribsub2'   => 'For $1 ($2)',
'nocontribs'    => 'Det vart ikkje funne nokon endringar gjorde av denne brukaren.',
'uctop'         => ' (øvst)',
'month'         => 'Månad:',
'year'          => 'År:',

'sp-contributions-newbies'     => 'Vis berre bidrag frå nye brukarar',
'sp-contributions-newbies-sub' => 'For nybyrjarar',
'sp-contributions-blocklog'    => 'Blokkeringslogg',
'sp-contributions-search'      => 'Søk etter bidrag',
'sp-contributions-username'    => 'IP-adresse eller brukarnamn:',
'sp-contributions-submit'      => 'Søk',

# What links here
'whatlinkshere'            => 'Lenkjer hit',
'whatlinkshere-title'      => 'Sider som har lenkje til «$1»',
'whatlinkshere-page'       => 'Side:',
'linklistsub'              => '(Liste over lenkjer)',
'linkshere'                => "Desse sidene har lenkjer til '''[[:$1]]''':",
'nolinkshere'              => "Inga side har lenkje '''[[:$1]]'''.",
'nolinkshere-ns'           => "Ingen sider har lenkje til '''[[:$1]]''' i det valde namnerommet.",
'isredirect'               => 'omdirigeringsside',
'istemplate'               => 'inkludert som mal',
'isimage'                  => 'fillenkje',
'whatlinkshere-prev'       => '{{PLURAL:$1|førre|førre $1}}',
'whatlinkshere-next'       => '{{PLURAL:$1|neste|neste $1}}',
'whatlinkshere-links'      => '← lenkjer',
'whatlinkshere-hideredirs' => '$1 omdirigeringer',
'whatlinkshere-hidetrans'  => '$1 inkluderinger',
'whatlinkshere-hidelinks'  => '$1 lenkjer',
'whatlinkshere-hideimages' => '$1 fillenkjer',
'whatlinkshere-filters'    => 'Filter',

# Block/unblock
'blockip'                         => 'Blokker brukar',
'blockip-legend'                  => 'Blokker brukar',
'blockiptext'                     => 'Bruk skjemaet nedanfor for å blokkere skrivetilgangen frå ei spesifikk IP-adresse eller brukarnamn. Dette bør berre gjerast for å hindre hærverk, og i samsvar med [[{{MediaWiki:Policy-url}}|retningslinene]].',
'ipaddress'                       => 'IP-adresse',
'ipadressorusername'              => 'IP-adresse eller brukarnamn',
'ipbexpiry'                       => 'Opphøyrstid:',
'ipbreason'                       => 'Årsak:',
'ipbreasonotherlist'              => 'Anna grunn',
'ipbreason-dropdown'              => '*Vanlege grunnar for blokkering
** Legg inn usann tekst/tull
** Fjernar innhald frå sider
** Legg inn reklamelenkjer til eksterne nettstader
** Sjikane/plaging av andre brukarar
** Misbruk ved hjelp av fleire brukarkontoar
** Uansvarleg brukarnamn',
'ipbanononly'                     => 'Blokker berre anonyme brukarar',
'ipbcreateaccount'                => 'Hindre kontooppretting',
'ipbemailban'                     => 'Hindre sending av e-post til andre brukarar',
'ipbenableautoblock'              => 'Blokker den førre IP-adressa som vart brukt av denne brukaren automatisk, og alle andre IP-adresser brukaren prøvar å endre sider med i framtida',
'ipbsubmit'                       => 'Blokker denne brukaren',
'ipbother'                        => 'Anna tid',
'ipboptions'                      => '2 timar:2 hours,1 dag:1 day,3 dagar:3 days,1 veke:1 week,2 veker:2 weeks,1 månad:1 month,3 månader:3 months,6 månader:6 months,1 år:1 year,endelaus:infinite', # display1:time1,display2:time2,...
'ipbotheroption'                  => 'anna tid',
'ipbotherreason'                  => 'Anna grunn/tilleggsgrunn:',
'ipbhidename'                     => 'Gøym brukarnamnet frå blokkeringsloggen, lista over aktive blokkeringar og brukarlista',
'ipbwatchuser'                    => 'Overvak brukarsida og diskusjonssida til brukaren',
'badipaddress'                    => 'IP-adressa er ugyldig eller blokkering av brukarar er slått av på tenaren.',
'blockipsuccesssub'               => 'Blokkering utført',
'blockipsuccesstext'              => '«[[Special:Contributions/$1|$1]]» er blokkert.<br />
Sjå [[Special:IPBlockList|blokkeringslista]] for alle blokkeringar.',
'ipb-edit-dropdown'               => 'Endre grunnane for blokkering',
'ipb-unblock-addr'                => 'Opphev blokkeringa av $1',
'ipb-unblock'                     => 'Opphev blokkeringa av eit brukarnamn eller ei IP-adresse',
'ipb-blocklist-addr'              => 'Vis gjeldande blokkeringar av $1',
'ipb-blocklist'                   => 'Vis gjeldande blokkeringar',
'unblockip'                       => 'Opphev blokkering',
'unblockiptext'                   => 'Bruk skjemaet nedanfor for å oppheve blokkeringa av ein tidlegare blokkert brukar.',
'ipusubmit'                       => 'Opphev blokkering',
'unblocked'                       => 'Blokkeringa av [[User:$1|$1]] er oppheva',
'unblocked-id'                    => 'Blokkering $1 er oppheva',
'ipblocklist'                     => 'Blokkerte IP-adresser og brukarnamn',
'ipblocklist-legend'              => 'Finn ein blokkert brukar',
'ipblocklist-username'            => 'Brukarnamn eller IP-adresse:',
'ipblocklist-submit'              => 'Søk',
'blocklistline'                   => '$1, $2 blokkerte $3 ($4)',
'infiniteblock'                   => 'uendeleg opphøyrstid',
'expiringblock'                   => '$1 opphøyrstid',
'anononlyblock'                   => 'berre anonyme',
'noautoblockblock'                => 'automatisk blokkering slått av',
'createaccountblock'              => 'kontooppretting blokkert',
'emailblock'                      => 'sending av e-post blokkert',
'ipblocklist-empty'               => 'Lista over blokkeringar er tom.',
'ipblocklist-no-results'          => 'Det etterspurde brukarnamnet eller IP-adressa er ikkje blokkert.',
'blocklink'                       => 'blokker',
'unblocklink'                     => 'opphev blokkering',
'contribslink'                    => 'bidrag',
'autoblocker'                     => 'Automatisk blokkert fordi du deler IP-adresse med [[User:$1|$1]]. Grunngjeving gjeve for blokkeringa av $1 var: «$2».',
'blocklogpage'                    => 'Blokkeringslogg',
'blocklogentry'                   => 'Blokkerte «[[$1]]» med opphøyrstid $2 $3',
'blocklogtext'                    => 'Dette er ein logg over blokkeringar og oppheving av blokkeringar gjorde.
IP-adresser som blir automatisk blokkerte er ikkje lista her. Sjå [[Special:IPBlockList|blokkeringslista]] for alle aktive blokkeringar.',
'unblocklogentry'                 => 'oppheva blokkering av «$1»',
'block-log-flags-anononly'        => 'berre anonyme brukarar',
'block-log-flags-nocreate'        => 'kontooppretting slått av',
'block-log-flags-noautoblock'     => 'automatisk blokkering slått av',
'block-log-flags-noemail'         => 'sending av e-post blokkert',
'block-log-flags-angry-autoblock' => 'utvida autoblokkering aktivert',
'range_block_disabled'            => 'Funksjonen for blokkering av IP-adresse-seriar er deaktivert på tenaren.',
'ipb_expiry_invalid'              => 'Ugyldig opphørstid.',
'ipb_expiry_temp'                 => 'For å skjule brukarnamnet må blokkeringa vere permanent.',
'ipb_already_blocked'             => '«$1» er allereie blokkert',
'ipb_cant_unblock'                => 'Feil: Fann ikkje blokkeringsnummeret $1. Blokkeringa kan vere oppheva allereie.',
'ipb_blocked_as_range'            => 'Feil: IP-en $1 er ikkje direkte blokkert og kan ikkje opphevast. Adressa er blokkert som ein del av blokkeringa av IP-intervallet $2. Denne blokkeringa kan opphevast.',
'ip_range_invalid'                => 'Ugyldig IP-adresseserie.',
'blockme'                         => 'Blokker meg',
'proxyblocker'                    => 'Proxy-blokkerar',
'proxyblocker-disabled'           => 'Denne funksjonen er slått av.',
'proxyblockreason'                => 'Du er blokkert frå å endre fordi IP-adressa di tilhøyrer ein open mellomtenar (proxy). Du bør kontakte internettleverandøren din eller kundesørvis og gje dei beskjed, ettersom dette er eit alvorleg sikkerheitsproblem.',
'proxyblocksuccess'               => 'Utført.',
'sorbsreason'                     => 'IP-adressa di er lista som ein open mellomtenar i DNSBL.',
'sorbs_create_account_reason'     => 'IP-adressa di er lista som ein open mellomtenar i DNSBL, og difor får du ikkje registrert deg.',

# Developer tools
'lockdb'              => 'Skrivevern (lock) database',
'unlockdb'            => 'Opphev skrivevern (unlock) av databasen',
'lockdbtext'          => 'Å skriveverne databasen vil gjere det umogleg for alle brukarar å endre sider, brukarinnstillingar, overvakingslister og andre ting som krev endringar i databasen. Stadfest at du ønskjer å gjera dette, og at du vil låse opp databasen att når vedlikehaldet er ferdig.',
'unlockdbtext'        => 'Å oppheva skrivevernet på databasen fører til at alle brukarar kan endre sider, brukarinnstillingar, overvakingslister og andre ting som krev endringar i databasen att. Stadfest at du ønskjer å gjera dette.',
'lockconfirm'         => 'Ja, eg vil verkeleg skriveverne databasen.',
'unlockconfirm'       => 'Ja, eg vil verkeleg oppheva skrivevernet på databasen.',
'lockbtn'             => 'Skrivevern databasen',
'unlockbtn'           => 'Opphev skrivevern på databasen',
'locknoconfirm'       => 'Du har ikkje stadfest handlinga.',
'lockdbsuccesssub'    => 'Databasen er no skriveverna',
'unlockdbsuccesssub'  => 'Skrivevernet på databasen er no oppheva',
'lockdbsuccesstext'   => 'Databasen er no skriveverna. <br />Hugs å [[Special:UnlockDB|oppheve skrivevernet]] når du er ferdig med vedlikehaldet.',
'unlockdbsuccesstext' => 'Skrivevernet er oppheva.',
'lockfilenotwritable' => 'Kan ikkje skrive til databasen si låsefil. For å låse eller opne databasen, må tenaren kunne skrive til denne fila.',
'databasenotlocked'   => 'Databasen er ikkje låst.',

# Move page
'move-page'               => 'Flytt $1',
'move-page-legend'        => 'Flytt side',
'movepagetext'            => "Ved å bruke skjemaet nedanfor kan du få omdøypt ei side og flytt heile historikken til det nye namnet. Den gamle tittelen vil bli ei omdirigeringsside til den nye tittelen. Lenkjer til den gamle tittelen vil ikkje bli endra. Pass på å sjekke for doble eller dårlege omdirigeringar. Du er ansvarleg for at alle lenkjene stadig peiker dit det er meininga at dei skal peike.

Merk at sida '''ikkje''' kan flyttast dersom det allereie finst ei side med den nye tittelen. Du kan likevel flytte ei side attende dit ho vart flytt frå dersom du gjer ein feil, så lenge den sida du flytter attende til ikkje er vorten endra sidan flyttinga.

<b>ÅTVARING!</b> Dette kan vera ei drastisk og uventa endring for ei populær side; ver sikker på at du skjønner konsekvensane av dette før du fortset.",
'movepagetalktext'        => "Den tilhøyrande diskusjonssida, om ho finst, vil automatisk bli flytt med sida '''med mindre:'''
*Du flytter sida til eit anna namnerom, eller
*Du fjernar merkinga i boksen nedanfor.

I desse falla lyt du flytte eller flette saman sida manuelt.",
'movearticle'             => 'Flytt side:',
'movenotallowed'          => 'Du har ikkje tilgang til å flytte sider.',
'newtitle'                => 'Til ny tittel',
'move-watch'              => 'Overvak denne sida',
'movepagebtn'             => 'Flytt side',
'pagemovedsub'            => 'Flyttinga er gjennomført',
'movepage-moved'          => "<big>'''«$1» er flytt til «$2»'''</big>", # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => 'Ei side med det namnet finst allereie, eller det namnet du har valt er ikkje gyldig. Vel eit anna namn.',
'cantmove-titleprotected' => 'Du kan ikkje flytte sida hit, fordi det nye sidenamnet er verna mot oppretting.',
'talkexists'              => "'''Innhaldssida vart flytt, men diskusjonssida som høyrer til kunne ikkje flyttast fordi det allereie finst ei side med den nye tittelen. Du lyt difor flette dei saman manuelt.'''",
'movedto'                 => 'er flytt til',
'movetalk'                => 'Flytt diskusjonssida òg om ho finst.',
'move-subpages'           => 'Flytt alle undersider om det finst nokre',
'move-talk-subpages'      => 'Flytt alle undersider av diskusjonssida, om det finst nokre',
'movepage-page-exists'    => 'Sida $1 finst alt og kan ikkje skrivast over automatisk.',
'movepage-page-moved'     => 'Sida $1 har blitt flytta til $2.',
'movepage-page-unmoved'   => 'Sida $1 kunne ikkje flyttast til $2.',
'movepage-max-pages'      => 'Grensa på {{PLURAL:$1|éi side|$1 sider}} er nådd; ingen fleire sider kjem til å verte flytta automatisk.',
'1movedto2'               => '«[[$1]]» flytt til «[[$2]]»',
'1movedto2_redir'         => '«[[$1]]» flytt over omdirigering til «[[$2]]»',
'movelogpage'             => 'Flyttelogg',
'movelogpagetext'         => 'Under er ei liste over sider som er flytte.',
'movereason'              => 'Årsak:',
'revertmove'              => 'attende',
'delete_and_move'         => 'Slett og flytt',
'delete_and_move_text'    => '== Sletting påkrevd ==

Målsida «[[:$1]]» finst allereie. Vil du slette ho for å gje rom for flytting?',
'delete_and_move_confirm' => 'Ja, slett sida',
'delete_and_move_reason'  => 'Sletta for å gje rom for flytting',
'selfmove'                => 'Kjelde- og måltitlane er like; kan ikkje flytte sida over seg sjølv.',
'immobile_namespace'      => 'Måltittelen høyrer til eit namnerom som gjer at sida ikkje kan flyttast dit.',
'imagenocrossnamespace'   => 'Kan ikkje flytte bilete til andre namnerom enn biletnamnerommet',
'imagetypemismatch'       => 'Den nye filendinga høver ikkje til filtypen',
'imageinvalidfilename'    => 'Målnamnet er ugyldig',
'fix-double-redirects'    => 'Oppdater omdirigeringar som viser til den gamle tittelen',

# Export
'export'            => 'Eksporter sider',
'exporttext'        => 'Du kan eksportere teksten og endringshistorikken til ei bestemt side eller ei gruppe sider, pakka inn i litt XML.
Dette kan så importerast til ein annan wiki som brukar MediaWiki-programvaren gjennom [[Special:Import|import-sida]].

For å eksportere sider, skriv inn titlar i tekstboksen under, ein tittel per linje, og velg om du vil ha berre noverande versjon, eller alle versjonar i historikken.

Dersom du berre vil ha noverande versjon, kan du også bruke ei lenkje, til dømes [[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]] for sida «[[{{MediaWiki:Mainpage}}]]».',
'exportcuronly'     => 'Berre eksporter siste versjonen, ikkje med heile historikken.',
'exportnohistory'   => "----
'''Merk:''' Å eksportere heile sidehistorikkar gjennom dette skjemaet er slått av grunna problem med ytinga.",
'export-submit'     => 'Eksporter',
'export-addcattext' => 'Legg til sider frå kategori:',
'export-addcat'     => 'Legg til',
'export-download'   => 'Lagre som fil',
'export-templates'  => 'Inkluder malane',

# Namespace 8 related
'allmessages'               => 'Systemmeldingar',
'allmessagesname'           => 'Namn',
'allmessagesdefault'        => 'Standardtekst',
'allmessagescurrent'        => 'Noverande tekst',
'allmessagestext'           => 'Dette er ei liste over systemmeldingar i MediaWiki-namnerommet.',
'allmessagesnotsupportedDB' => "Denne sida er ikkje brukande fordi \"'''\$wgUseDatabaseMessages'''\" er slått av.",
'allmessagesfilter'         => 'Meldingsfilter:',
'allmessagesmodified'       => 'Vis berre endra',

# Thumbnails
'thumbnail-more'           => 'Forstørr',
'filemissing'              => 'Fila manglar',
'thumbnail_error'          => 'Feil ved oppretting av miniatyrbilete: $1',
'djvu_page_error'          => 'DjVu-sida er utanfor rekkjevidd',
'djvu_no_xml'              => 'Klarte ikkje hente inn XML for DjVu-fila',
'thumbnail_invalid_params' => 'Ugyldige miniatyrparameterar',
'thumbnail_dest_directory' => 'Klarte ikkje å opprette målmappe',

# Special:Import
'import'                     => 'Importer sider',
'importinterwiki'            => 'Transwikiimport',
'import-interwiki-text'      => 'Vel ei wiki og ei side å importere. Endringssdatoer og brukarar som har medverka vert bevart. Alle transwiki-importeringar vert vist i [[Special:Log/import|importloggen]].',
'import-interwiki-history'   => 'Kopier all historikken for denne sida',
'import-interwiki-submit'    => 'Importer',
'import-interwiki-namespace' => 'Flytt sidene til namnerommet:',
'importtext'                 => 'Eksporter fila frå kjeldewikien med [[Special:Export|eksporteringsverktøyet]], lagre ho på di eiga datamaskin, og last henne opp her.',
'importstart'                => 'Importerer sidene…',
'import-revision-count'      => '$1 {{PLURAL:$1|versjon|versjonar}}',
'importnopages'              => 'Ingen sider å importere.',
'importfailed'               => 'Importeringa var mislukka: $1',
'importunknownsource'        => 'Ukjend importkjeldetype',
'importcantopen'             => 'Kunne ikkje opne importfil',
'importbadinterwiki'         => 'Ugyldig interwikilenkje',
'importnotext'               => 'Tom eller ingen tekst',
'importsuccess'              => 'Importeringa er ferdig!',
'importhistoryconflict'      => 'Det kan vera at det er konflikt i historikken (kanskje sida vart importert før)',
'importnosources'            => 'Ingen kjelder for transwikiimport er oppgjevne og funksjonen for opplasting av historikk er deaktivert.',
'importnofile'               => 'Inga importfil er lasta opp.',
'importuploaderrorsize'      => 'Opplastinga av importfila var mislukka. Fila er større enn det som er lov å laste opp.',
'importuploaderrorpartial'   => 'Opplastinga av importfila var mislukka. Fila vart berre delvis lasta opp.',
'importuploaderrortemp'      => 'Opplastinga av importfila var mislukka. Ei mellombels mappe manglar.',
'import-parse-failure'       => 'Feil i tolking av XML-import',
'import-noarticle'           => 'Ingen sider å importere!',
'import-nonewrevisions'      => 'Alle versjonar var importert frå før.',
'xml-error-string'           => '$1 på rad $2, kolonne $3 (byte: $4): $5',
'import-upload'              => 'Last opp XML-data',

# Import log
'importlogpage'                    => 'Importeringslogg',
'importlogpagetext'                => 'Administrativ import av sider med endringshistorikk frå andre wikiar.',
'import-logentry-upload'           => 'importerte [[$1]] frå opplasta fil',
'import-logentry-upload-detail'    => '{{PLURAL:$1|Éin versjon|$1 versjonar}}',
'import-logentry-interwiki'        => 'overførte $1 mellom wikiar',
'import-logentry-interwiki-detail' => '{{PLURAL:$1|Éin versjon|$1 versjonar}} frå $2',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'Brukarsida mi',
'tooltip-pt-anonuserpage'         => 'Brukarsida for ip-adressa du endrar under',
'tooltip-pt-mytalk'               => 'Diskusjonssida mi',
'tooltip-pt-anontalk'             => 'Diskusjon om endringar gjorde av denne ip-adressa',
'tooltip-pt-preferences'          => 'Innstillingane mine',
'tooltip-pt-watchlist'            => 'Liste over sidene du overvakar.',
'tooltip-pt-mycontris'            => 'Liste over bidraga mine',
'tooltip-pt-login'                => 'Det er ikkje obligatorisk å logga inn, men medfører mange fordelar.',
'tooltip-pt-anonlogin'            => 'Det er ikkje obligatorisk å logga inn, men medfører mange fordelar.',
'tooltip-pt-logout'               => 'Logg ut',
'tooltip-ca-talk'                 => 'Diskusjon om innhaldssida',
'tooltip-ca-edit'                 => 'Du kan endre denne sida. Bruk førehandsvisings-knappen før du lagrar.',
'tooltip-ca-addsection'           => 'Legg til ein bolk på denne diskusjonssida.',
'tooltip-ca-viewsource'           => 'Denne sida er verna, men du kan sjå kjeldeteksten.',
'tooltip-ca-history'              => 'Eldre versjonar av denne sida.',
'tooltip-ca-protect'              => 'Vern denne sida',
'tooltip-ca-delete'               => 'Slett denne sida',
'tooltip-ca-undelete'             => 'Attopprett denne sida',
'tooltip-ca-move'                 => 'Flytt denne sida',
'tooltip-ca-watch'                => 'Legg denne sida til i overvakingslista di',
'tooltip-ca-unwatch'              => 'Fjern denne sida frå overvakingslista di',
'tooltip-search'                  => 'Søk gjennom denne wikien',
'tooltip-search-go'               => 'Gå til ei side med dette namnet om ho finst',
'tooltip-search-fulltext'         => 'Søk etter sider som inneheld denne teksten',
'tooltip-p-logo'                  => 'Hovudside',
'tooltip-n-mainpage'              => 'Gå til hovudsida',
'tooltip-n-portal'                => 'Om prosjektet, kva du kan gjera, kvar du finn saker og ting',
'tooltip-n-currentevents'         => 'Aktuelt',
'tooltip-n-recentchanges'         => 'Liste over dei siste endringane som er gjort på wikien.',
'tooltip-n-randompage'            => 'Vis ei tilfeldig side',
'tooltip-n-help'                  => 'Hjelp til å bruke alle funksjonane.',
'tooltip-t-whatlinkshere'         => 'Liste over alle wikisidene som har lenkjer hit',
'tooltip-t-recentchangeslinked'   => 'Siste endringar på sider denne sida lenkjer til',
'tooltip-feed-rss'                => 'RSS-mating for denne sida',
'tooltip-feed-atom'               => 'Atom-mating for denne sida',
'tooltip-t-contributions'         => 'Sjå liste over bidrag frå denne brukaren',
'tooltip-t-emailuser'             => 'Send ein e-post til denne brukaren',
'tooltip-t-upload'                => 'Last opp filer',
'tooltip-t-specialpages'          => 'Liste over spesialsider',
'tooltip-t-print'                 => 'Utskriftsversjon av sida',
'tooltip-t-permalink'             => 'Fast lenkje til denne versjonen av sida',
'tooltip-ca-nstab-main'           => 'Vis innhaldssida',
'tooltip-ca-nstab-user'           => 'Vis brukarsida',
'tooltip-ca-nstab-media'          => 'Direktelenkje (filpeikar) til fil',
'tooltip-ca-nstab-special'        => 'Dette er ei spesialside, du kan ikkje endre ho.',
'tooltip-ca-nstab-project'        => 'Vis prosjektside',
'tooltip-ca-nstab-image'          => 'Vis filside',
'tooltip-ca-nstab-mediawiki'      => 'Vis systemmelding',
'tooltip-ca-nstab-template'       => 'Vis mal',
'tooltip-ca-nstab-help'           => 'Vis hjelpeside',
'tooltip-ca-nstab-category'       => 'Vis kategoriside',
'tooltip-minoredit'               => 'Merk dette som småplukk',
'tooltip-save'                    => 'Lagre endringane dine',
'tooltip-preview'                 => 'Førehandsvis endringane dine, bruk denne funksjonen før du lagrar!',
'tooltip-diff'                    => 'Vis skilnaden mellom din versjon og lagra versjon, utan å lagre.',
'tooltip-compareselectedversions' => 'Sjå endringane mellom dei valde versjonane av denne sida.',
'tooltip-watch'                   => 'Legg denne sida til i overvakingslista di [alt-w]',
'tooltip-recreate'                => 'Ved å trykkje på «Nyopprett» vert sida oppretta på nytt.',
'tooltip-upload'                  => 'Start opplastinga',

# Stylesheets
'monobook.css' => '/* CSS-tekst som vert plassert her, endrar utsjånaden til sidedrakta Monobook */',

# Scripts
'monobook.js' => '/* Ute av bruk, bruk [[MediaWiki:common.js]] i staden */',

# Metadata
'nodublincore'      => 'Funksjonen for Dublin Core RDF metadata er deaktivert på denne tenaren.',
'nocreativecommons' => 'Funksjonen for Creative Commons RDF er deaktivert på denne tenaren.',
'notacceptable'     => 'Wikitenaren kan ikkje gje data i noko format som programmet ditt kan lesa.',

# Attribution
'anonymous'        => 'Anonym(e) brukar(ar) av {{SITENAME}}',
'siteuser'         => '{{SITENAME}} brukar $1',
'lastmodifiedatby' => 'Denne sida vart sist endra $2, $1 av $3.', # $1 date, $2 time, $3 user
'othercontribs'    => 'Basert på arbeid av $1.',
'others'           => 'andre',
'siteusers'        => '{{SITENAME}} brukar(ar) $1',
'creditspage'      => 'Sidegodskriving',
'nocredits'        => 'Det finst ikkje ikkje nokon godskrivingsinformasjon for denne sida.',

# Spam protection
'spamprotectiontitle' => 'Filter for vern mot reklame',
'spamprotectiontext'  => 'Sida du prøvde å lagre vart blokkert av spamfilteret. Dette kjem truleg av at ei ekstern lenkje på sida er svartelista.',
'spamprotectionmatch' => 'Den følgjande teksten utløyste reklamefilteret: $1',
'spambot_username'    => 'MediaWiki si spamopprydding',
'spam_reverting'      => 'Attenderullar til siste versjon utan lenkje til $1',
'spam_blanking'       => 'Alle versjonar inneheldt lenkje til $1, tømmer sida',

# Info page
'infosubtitle'   => 'Informasjon om side',
'numedits'       => 'Tal endringar (innhaldsside): $1',
'numtalkedits'   => 'Tal endringar (diskusjonsside): $1',
'numwatchers'    => 'Tal brukarar som overvakar: $1',
'numauthors'     => 'Tal ulike bidragsytarar (innhaldsside): $1',
'numtalkauthors' => 'Tal ulike bidragsytarar (diskusjonsside): $1',

# Math options
'mw_math_png'    => 'Vis alltid som PNG',
'mw_math_simple' => 'HTML om svært enkel, elles PNG',
'mw_math_html'   => 'HTML om mogleg, elles PNG',
'mw_math_source' => 'Behald som TeX (for tekst-nettlesarar)',
'mw_math_modern' => 'Tilrådd for moderne nettlesarar',
'mw_math_mathml' => 'MathML dersom mogleg (eksperimentell)',

# Patrolling
'markaspatrolleddiff'                 => 'Merk som patruljert',
'markaspatrolledtext'                 => 'Merk denne innhaldssida som patruljert',
'markedaspatrolled'                   => 'Merk som patruljert',
'markedaspatrolledtext'               => 'Den valde versjonen er vorten merkt som patruljert.',
'rcpatroldisabled'                    => 'Siste-endringar-patruljering er deaktivert',
'rcpatroldisabledtext'                => 'Patruljeringsfunksjonen er deaktivert.',
'markedaspatrollederror'              => 'Kan ikkje merke sida som patruljert',
'markedaspatrollederrortext'          => 'Du må markere ein versjon for å kunne godkjenne.',
'markedaspatrollederror-noautopatrol' => 'Ein har ikkje høve til å merkje sine eigne endringar som godkjende.',

# Patrol log
'patrol-log-page'   => 'Patruljeringslogg',
'patrol-log-header' => 'Dette er ein logg over patruljerte sideversjonar.',
'patrol-log-line'   => 'merka $1 av $2 godkjend $3',
'patrol-log-auto'   => '(automatisk)',

# Image deletion
'deletedrevision'                 => 'Slett gammal versjon $1',
'filedeleteerror-short'           => 'Feil ved sletting av fila: $1',
'filedeleteerror-long'            => 'Det vart ein feil under filslettinga av:

$1',
'filedelete-missing'              => 'Det finst ikkje noko fil som heiter «$1», og difor går det heller ikkje å slette noko slik fil.',
'filedelete-old-unregistered'     => 'Filversjonen «$1» finst ikkje i databasen.',
'filedelete-current-unregistered' => 'Fila «$1» finst ikkje i databasen.',
'filedelete-archive-read-only'    => 'Webserveraren har ikkje skrivetilgang til arkivkatalogen "$1".',

# Browsing diffs
'previousdiff' => '← Eldre endring',
'nextdiff'     => 'Nyare endring →',

# Media information
'mediawarning'         => "'''Åtvaring''': Denne fila kan innehalda skadelege program, ved å opna ho kan systemet ditt ta skade.<hr />",
'imagemaxsize'         => 'Avgrens bilete på filsider til (pikslar):',
'thumbsize'            => 'Miniatyrstørrelse:',
'widthheightpage'      => '$1×$2, $3 {{PLURAL:$3|side|sider}}',
'file-info'            => '(filstorleik: $1, MIME-type: $2)',
'file-info-size'       => '($1 × $2 pikslar, filstorleik: $3, MIME-type: $4)',
'file-nohires'         => '<small>Høgare oppløysing er ikkje tilgjengeleg.</small>',
'svg-long-desc'        => '(SVG-fil, standardoppløysing: $1 × $2 pikslar, filstorleik: $3)',
'show-big-image'       => 'Full oppløysing',
'show-big-image-thumb' => '<small>Storleiken på denne førehandsvisinga: $1 × $2 pikslar</small>',

# Special:NewImages
'newimages'             => 'Filgalleri',
'imagelisttext'         => 'Her er ei liste med {{PLURAL:$1|éi fil sortert|$1 filer sorterte}} $2.',
'newimages-summary'     => 'Denne spesialsida syner dei sist opplasta filene.',
'showhidebots'          => '($1 robotar)',
'noimages'              => 'Her er ingen filer som kan visast.',
'ilsubmit'              => 'Søk',
'bydate'                => 'etter dato',
'sp-newimages-showfrom' => 'Vis nye filer frå og med $2 $1',

# Bad image list
'bad_image_list' => 'Formatet er slik:

Berre liner som startar med asterisk (*) vert tekne med.
Den fyrste lenkja på ei line må gå til ei uønskt fil.
Alle andre lenkjer på same line vert sett på som unnatak, med andre ord sider der fila kan brukast.',

# Metadata
'metadata'          => 'Utvida informasjon',
'metadata-help'     => 'Denne fila inneheld tilleggsopplysningar, mest sannsynleg frå digitalkameraet eller skannaren som vart brukt til å lage eller digitalisere henne.
Dersom fila har vore endra sidan ho vart oppretta, kan nokre av opplysningane vere feil.',
'metadata-expand'   => 'Vis utvida opplysningar',
'metadata-collapse' => 'Gøym utvida opplysningar',
'metadata-fields'   => 'EXIF-metadatafelta denne meldinga inneheld vert med på filskildringssida når dei utvida opplysningane er slått av.
Dei andre felta er gøymde som standard.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => 'Breidd',
'exif-imagelength'                 => 'Høgd',
'exif-bitspersample'               => 'Bitar per komponent',
'exif-compression'                 => 'Komprimeringsteknikk',
'exif-photometricinterpretation'   => 'Pikselsamansetjing',
'exif-orientation'                 => 'Retning',
'exif-samplesperpixel'             => 'Tal komponentar',
'exif-planarconfiguration'         => 'Dataarrangement',
'exif-ycbcrsubsampling'            => 'Subsamplingstilhøve mellom Y og C',
'exif-ycbcrpositioning'            => 'Y- og C-posisjon',
'exif-xresolution'                 => 'Oppløysing i breidda',
'exif-yresolution'                 => 'Oppløysing i høgda',
'exif-resolutionunit'              => 'Eining for X- og Y-oppløysing',
'exif-stripoffsets'                => 'Plassering for biletdata',
'exif-rowsperstrip'                => 'Tal rader per stripe',
'exif-stripbytecounts'             => 'Tal byte per kompimerte stripe',
'exif-jpeginterchangeformat'       => 'Offset til JPEG SOI',
'exif-jpeginterchangeformatlength' => 'Byte JPEG-data',
'exif-transferfunction'            => 'Overføringsfunksjon',
'exif-whitepoint'                  => 'Kvitpunktsreinleik',
'exif-primarychromaticities'       => 'Reinheita til primærfargane',
'exif-ycbcrcoefficients'           => 'Koeffisientar for fargeromstransformasjonsmatrise',
'exif-referenceblackwhite'         => 'Svart og kvitt referanseverdipar',
'exif-datetime'                    => 'Dato og tid endra',
'exif-imagedescription'            => 'Tittel',
'exif-make'                        => 'Kameraprodusent',
'exif-model'                       => 'Kameramodell',
'exif-software'                    => 'Programvare brukt',
'exif-artist'                      => 'Skapar',
'exif-copyright'                   => 'Opphavsrettsleg eigar',
'exif-exifversion'                 => 'Exif-versjon',
'exif-flashpixversion'             => 'Støtta Flashpix versjon',
'exif-colorspace'                  => 'Fargerom',
'exif-componentsconfiguration'     => 'Komponentanalyse',
'exif-compressedbitsperpixel'      => 'Komprimerte bits pr. pixel',
'exif-pixelydimension'             => 'Gyldig biletbreidd',
'exif-pixelxdimension'             => 'Gyldig bilethøgd',
'exif-makernote'                   => 'Produsentnotat',
'exif-usercomment'                 => 'Brukarkommentarar',
'exif-relatedsoundfile'            => 'Tilknytt lydfil',
'exif-datetimeoriginal'            => 'Dato og tid laga',
'exif-datetimedigitized'           => 'Dato og tid digitalisert',
'exif-subsectime'                  => 'Dato og tid subsekund',
'exif-subsectimeoriginal'          => 'Dato og tid laga subsekund',
'exif-subsectimedigitized'         => 'Dato og tid digitalisert subsekund',
'exif-exposuretime'                => 'Eksponeringstid',
'exif-exposuretime-format'         => '$1 sekund ($2)',
'exif-fnumber'                     => 'F-nummer',
'exif-exposureprogram'             => 'Eksponeringsprogram',
'exif-spectralsensitivity'         => 'Spektralsensitivitet',
'exif-isospeedratings'             => 'Lyskjensle (ISO)',
'exif-oecf'                        => 'Optoelektronisk omregningsfaktor',
'exif-shutterspeedvalue'           => 'Lukkarfart',
'exif-aperturevalue'               => 'Blendartal',
'exif-brightnessvalue'             => 'Lysstyrke',
'exif-exposurebiasvalue'           => 'Eksponeringsinnstilling',
'exif-maxaperturevalue'            => 'Maksimal blendar',
'exif-subjectdistance'             => 'Motivavstand',
'exif-meteringmode'                => 'Lysmålarmodus',
'exif-lightsource'                 => 'Lyskjelde',
'exif-flash'                       => 'Blits',
'exif-focallength'                 => 'Linsefokallengd',
'exif-subjectarea'                 => 'Motivområde',
'exif-flashenergy'                 => 'Blitsstyrke',
'exif-spatialfrequencyresponse'    => 'Romleg frekvensrespons',
'exif-focalplanexresolution'       => 'Oppløysing i fokalplan X',
'exif-focalplaneyresolution'       => 'Oppløysing i fokalplan Y',
'exif-focalplaneresolutionunit'    => 'Oppløysingseining for fokalplanet',
'exif-subjectlocation'             => 'Motivplassering',
'exif-exposureindex'               => 'Eksponeringsindeks',
'exif-sensingmethod'               => 'Sensor',
'exif-filesource'                  => 'Filkjelde',
'exif-scenetype'                   => 'Scenetype',
'exif-cfapattern'                  => 'CFA-mønster',
'exif-customrendered'              => 'Tilpassa biletehandsaming',
'exif-exposuremode'                => 'Eksponeringsmodus',
'exif-whitebalance'                => 'Kvitbalanse',
'exif-digitalzoomratio'            => 'Digital zoom-rate',
'exif-focallengthin35mmfilm'       => '(Tilsvarande) brennvidd ved 35 mm film',
'exif-scenecapturetype'            => 'Motivtype',
'exif-gaincontrol'                 => 'Scenekontroll',
'exif-contrast'                    => 'Kontrast',
'exif-saturation'                  => 'Metting',
'exif-sharpness'                   => 'Skarpleik',
'exif-devicesettingdescription'    => 'Apparatinnstilling',
'exif-subjectdistancerange'        => 'Motivavstandsområde',
'exif-imageuniqueid'               => 'Unik bilete-ID',
'exif-gpsversionid'                => 'GPS-merke-versjon',
'exif-gpslatituderef'              => 'Nordleg eller sørleg breiddegrad',
'exif-gpslatitude'                 => 'Breiddegrad',
'exif-gpslongituderef'             => 'Austleg eller vestleg lengdegrad',
'exif-gpslongitude'                => 'Lengdegrad',
'exif-gpsaltituderef'              => 'Høgdereferanse',
'exif-gpsaltitude'                 => 'Høgd over havet',
'exif-gpstimestamp'                => 'GPS-tid (atomklokke)',
'exif-gpssatellites'               => 'Satellittar brukt for å måle',
'exif-gpsstatus'                   => 'GPS-Mottakarstatus',
'exif-gpsmeasuremode'              => 'Målemodus',
'exif-gpsdop'                      => 'Målepresisjon',
'exif-gpsspeedref'                 => 'Fartsmåleining',
'exif-gpsspeed'                    => 'Fart på GPS-mottakar',
'exif-gpstrackref'                 => 'Referanse for rørsleretning',
'exif-gpstrack'                    => 'Rørsleretning',
'exif-gpsimgdirectionref'          => 'Referanse for retning åt biletet',
'exif-gpsimgdirection'             => 'Retninga åt biletet',
'exif-gpsmapdatum'                 => 'Geodetisk kartleggingsdata brukt',
'exif-gpsdestlatituderef'          => 'Referanse for målbreiddegrad',
'exif-gpsdestlatitude'             => 'Målbreiddegrad',
'exif-gpsdestlongituderef'         => 'Referanse for mållengdegrad',
'exif-gpsdestlongitude'            => 'Mållengdegrad',
'exif-gpsdestbearingref'           => 'Referanse for retning mot målet',
'exif-gpsdestbearing'              => 'Retning mot målet',
'exif-gpsdestdistanceref'          => 'Referanse for avstand til mål',
'exif-gpsdestdistance'             => 'Avstand til mål',
'exif-gpsprocessingmethod'         => 'Namn på GPS-handsamingsmetode',
'exif-gpsareainformation'          => 'Namn på GPS-område',
'exif-gpsdatestamp'                => 'GPS-dato',
'exif-gpsdifferential'             => 'Differensiell GPS-retting',

# EXIF attributes
'exif-compression-1' => 'Ukomprimert',

'exif-unknowndate' => 'Ukjend dato',

'exif-orientation-1' => 'Normal', # 0th row: top; 0th column: left
'exif-orientation-2' => 'Spegla vassrett', # 0th row: top; 0th column: right
'exif-orientation-3' => 'Rotert 180°', # 0th row: bottom; 0th column: right
'exif-orientation-4' => 'Spegla loddrett', # 0th row: bottom; 0th column: left
'exif-orientation-5' => 'Rotert 90° motsols og spegla vassrett', # 0th row: left; 0th column: top
'exif-orientation-6' => 'Rotert 90° medsols', # 0th row: right; 0th column: top
'exif-orientation-7' => 'Rotert 90° medsols og spegla loddrett', # 0th row: right; 0th column: bottom
'exif-orientation-8' => 'Rotert 90° motsols', # 0th row: left; 0th column: bottom

'exif-planarconfiguration-1' => 'grovformat',
'exif-planarconfiguration-2' => 'planærformat',

'exif-componentsconfiguration-0' => 'finst ikkje',

'exif-exposureprogram-0' => 'Ikkje bestemt',
'exif-exposureprogram-1' => 'Manuelt',
'exif-exposureprogram-2' => 'Normalt program',
'exif-exposureprogram-3' => 'Blendarprioritet',
'exif-exposureprogram-4' => 'Lukkarprioritet',
'exif-exposureprogram-5' => 'Kreativt program (mest mogleg skarpt)',
'exif-exposureprogram-6' => 'Handlingsprogram (med vekt på snøgg lukkar)',
'exif-exposureprogram-7' => 'Portrettmodus (for nærbilete med uskarp bakgrunn)',
'exif-exposureprogram-8' => 'Landskapsmodus (for landskapsbilete med skarp bakgrunn)',

'exif-subjectdistance-value' => '$1 meter',

'exif-meteringmode-0'   => 'Ukjent',
'exif-meteringmode-1'   => 'Snittmåling',
'exif-meteringmode-2'   => 'Snittmåling med vekt på midten',
'exif-meteringmode-3'   => 'Punktmåling',
'exif-meteringmode-4'   => 'Fleirpunktsmåling',
'exif-meteringmode-5'   => 'Mønster',
'exif-meteringmode-6'   => 'Delvis',
'exif-meteringmode-255' => 'Annan',

'exif-lightsource-0'   => 'Ukjent',
'exif-lightsource-1'   => 'Dagslys',
'exif-lightsource-2'   => 'Fluorescerande',
'exif-lightsource-3'   => 'Glødelampe',
'exif-lightsource-4'   => 'Blits',
'exif-lightsource-9'   => 'Fint vêr',
'exif-lightsource-10'  => 'Overskya vêr',
'exif-lightsource-11'  => 'Skugge',
'exif-lightsource-12'  => 'Fluorescerande dagslys (D 5700 – 7100K)',
'exif-lightsource-13'  => 'Dag, kvitt, fluorescerande (N 4600 – 5400K)',
'exif-lightsource-14'  => 'Kjølig, kvitt, fluorescerande (W 3900 – 4500K)',
'exif-lightsource-15'  => 'Kvitt fluorescerande (WW 3200 – 3700K)',
'exif-lightsource-17'  => 'Standardlys A',
'exif-lightsource-18'  => 'Standardlys B',
'exif-lightsource-19'  => 'Standardlys C',
'exif-lightsource-24'  => 'ISO studio kunstljos',
'exif-lightsource-255' => 'Anna lyskjelde',

'exif-focalplaneresolutionunit-2' => 'tommar',

'exif-sensingmethod-1' => 'Ikkje bestemt',
'exif-sensingmethod-2' => 'Einbrikka fargeområdesensor',
'exif-sensingmethod-3' => 'Tobrikka fargeområdesensor',
'exif-sensingmethod-4' => 'Trebrikka fargeområdesensor',
'exif-sensingmethod-5' => 'Fargesekvensiell områdesensor',
'exif-sensingmethod-7' => 'Trilinær sensor',
'exif-sensingmethod-8' => 'Fargesekvensiell lineærsensor',

'exif-scenetype-1' => 'Direkte fotografert bilete',

'exif-customrendered-0' => 'Normal prosess',
'exif-customrendered-1' => 'Tilpassa prosess',

'exif-exposuremode-0' => 'Autoeksponert',
'exif-exposuremode-1' => 'Manuelt eksponert',
'exif-exposuremode-2' => 'Automatisk alternativeksponering',

'exif-whitebalance-0' => 'Automatisk kvitbalanse',
'exif-whitebalance-1' => 'Manuell kvitbalanse',

'exif-scenecapturetype-0' => 'Standard',
'exif-scenecapturetype-1' => 'Landskap',
'exif-scenecapturetype-2' => 'Portrett',
'exif-scenecapturetype-3' => 'Nattscene',

'exif-gaincontrol-0' => 'Ingen',
'exif-gaincontrol-1' => 'Auke av lågnivåforsterking',
'exif-gaincontrol-2' => 'Auke av høgnivåforsterking',
'exif-gaincontrol-3' => 'Minking av lågnivåforsterking',
'exif-gaincontrol-4' => 'Minking av høgnivåforsterking',

'exif-contrast-0' => 'Normal',
'exif-contrast-1' => 'Mjuk',
'exif-contrast-2' => 'Hard',

'exif-saturation-0' => 'Normal',
'exif-saturation-1' => 'Låg metting',
'exif-saturation-2' => 'Høg metting',

'exif-sharpness-0' => 'Normal',
'exif-sharpness-1' => 'Mjuk',
'exif-sharpness-2' => 'Hard',

'exif-subjectdistancerange-0' => 'Ukjent',
'exif-subjectdistancerange-1' => 'Makro',
'exif-subjectdistancerange-2' => 'Nært',
'exif-subjectdistancerange-3' => 'Fjernt',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'Nordleg breiddegrad',
'exif-gpslatitude-s' => 'Sørleg breiddegrad',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'Austleg lengdegrad',
'exif-gpslongitude-w' => 'Vestleg lengdegrad',

'exif-gpsstatus-a' => 'Måling pågår',
'exif-gpsstatus-v' => 'Målingsinteroperabilitet',

'exif-gpsmeasuremode-2' => 'todimensjonalt målt',
'exif-gpsmeasuremode-3' => 'tredimensjonalt målt',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'Kilometer per time',
'exif-gpsspeed-m' => 'Engelsk mil per time',
'exif-gpsspeed-n' => 'Knop',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => 'Verkeleg retning',
'exif-gpsdirection-m' => 'Magnetisk retning',

# External editor support
'edit-externally'      => 'Endre denne fila med eit eksternt program',
'edit-externally-help' => 'Sjå [http://www.mediawiki.org/wiki/Manual:External_editors eksterne program instruksjonane] for meir informasjon.',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'alle',
'imagelistall'     => 'alle',
'watchlistall2'    => 'alle',
'namespacesall'    => 'alle',
'monthsall'        => 'alle',

# E-mail address confirmation
'confirmemail'             => 'Stadfest e-postadresse',
'confirmemail_noemail'     => 'Du har ikkje gjeve ei gyldig e-postadresse i [[Special:Preferences|innstillingane dine]].',
'confirmemail_text'        => '{{SITENAME}} krev at du stadfester e-postadressa di
før du får brukt funksjonar knytt til e-post. Klikk på knappen under for å sende ei stadfestingsmelding
til adressa di. E-posten kjem med ei lenkje som har ein kode; opne
lenkja i nettlesaren din for å stadfeste at e-postadressa di er gyldig.',
'confirmemail_pending'     => '<div class="error">Ein stadfestingskode har alt vorte send til deg på e-post;
gjer vel å vente nokre minutt før du ber om ny kode om du nett har oppretta kontoen din.</div>',
'confirmemail_send'        => 'Send stadfestingsmelding',
'confirmemail_sent'        => 'Stadfestingsmelding er sendt.',
'confirmemail_oncreate'    => 'Ein stadfestingskode er no send til e-postadressa di.
Koden trengst ikkje for å få logga seg inn, men er naudsynd om ein skal aktivere e-postbaserte tenester på denne wikien.',
'confirmemail_sendfailed'  => '{{SITENAME}} klarte ikkje å sende stadfestingsmelding. 
Sjekk e-postadressa for ugyldige teikn.

E-postsendaren gav denne meldinga: $1',
'confirmemail_invalid'     => 'Feil stadfestingskode. Koden er kanskje for forelda.',
'confirmemail_needlogin'   => 'Du må $1 for å stadfeste e-postadressa di.',
'confirmemail_success'     => 'E-postadressa di er stadfest. Du kan no logge inn og kose deg med {{SITENAME}}.',
'confirmemail_loggedin'    => 'E-postadressa di er stadfest.',
'confirmemail_error'       => 'Noko gjekk gale når stadfestinga di skulle lagrast.',
'confirmemail_subject'     => 'Stadfesting av e-postadresse frå {{SITENAME}}',
'confirmemail_body'        => 'Nokon, truleg du, frå IP-adressa $1, har registrert kontoen «$2» med di e-postadresse på {{SITENAME}}.

For å stadfeste at denne kontoen faktisk høyrer til deg og for å slå på
funksjonar tilknytt e-post på {{SITENAME}} må du opne denne lenkja i nettlesaren din:

$3

Dersom dette *ikkje* er deg, følg denne lenkja for avbryte stadfestinga av e-postadressa:

$5

Denne stadfestingskoden vert forelda $4.',
'confirmemail_invalidated' => 'Stadfestinga av e-postadresse er avbrote',
'invalidateemail'          => 'Avbryt stadfestinga av e-postadressa',

# Scary transclusion
'scarytranscludedisabled' => '[Interwiki-tilkopling er slått av]',
'scarytranscludefailed'   => '[Henting av mal for $1 gjekk ikkje]',
'scarytranscludetoolong'  => '[URL-en er for lang]',

# Trackbacks
'trackbackbox'      => "<div id='mw_trackbacks'>
Attendelenkjer for denne sida:<br />
$1
</div>",
'trackbackremove'   => ' ([$1 Slett])',
'trackbacklink'     => 'Attendelenkje',
'trackbackdeleteok' => 'Attendelenkja vart sletta.',

# Delete conflict
'deletedwhileediting' => "'''Åtvaring:''' Denne sida har vorte sletta etter du starta å endre henne!",
'confirmrecreate'     => "Brukaren «[[User:$1|$1]]» ([[User talk:$1|brukardiskusjon]]) sletta denne sida medan du endra henne, og gav denne grunnen: ''$2''

Du må stadfeste at du verkeleg vil nyopprette denne sida.",
'recreate'            => 'Nyopprett',

# HTML dump
'redirectingto' => 'Omdirigerer til [[:$1]]...',

# action=purge
'confirm_purge'        => 'Vil du slette tenarane sin mellomlagra versjon av denne sida?

$1',
'confirm_purge_button' => 'Ja',

# AJAX search
'searchcontaining' => "Søk etter sider som inneheld ''$1''.",
'searchnamed'      => "Søk etter sider som heiter ''$1''.",
'articletitles'    => "Sider som byrjar med ''$1''",
'hideresults'      => 'Gøym resultatet',
'useajaxsearch'    => 'Bruk AJAX-søk',

# Multipage image navigation
'imgmultipageprev' => '← førre side',
'imgmultipagenext' => 'neste side →',
'imgmultigo'       => 'Gå!',
'imgmultigoto'     => 'Gå til side $1',

# Table pager
'ascending_abbrev'         => 'stigande',
'descending_abbrev'        => 'synkande',
'table_pager_next'         => 'Neste side',
'table_pager_prev'         => 'Førre side',
'table_pager_first'        => 'Fyrste side',
'table_pager_last'         => 'Siste side',
'table_pager_limit'        => 'Vis $1 element per side',
'table_pager_limit_submit' => 'Gå',
'table_pager_empty'        => 'Ingen resultat',

# Auto-summaries
'autosumm-blank'   => 'Fjernar alt innhald på sida',
'autosumm-replace' => 'Erstattar innhaldet på sida med «$1»',
'autoredircomment' => 'Omdirigerer til [[$1]]',
'autosumm-new'     => 'Ny side: $1',

# Live preview
'livepreview-loading' => 'Lastar inn&nbsp;…',
'livepreview-ready'   => 'Lastar inn… Ferdig!',
'livepreview-failed'  => 'Levande førehandsvising var mislykka. Prøv vanleg førehandsvising.',
'livepreview-error'   => 'Tilkoplinga var mislykka: $1 «$2». Prøv vanleg førehandsvising.',

# Friendlier slave lag warnings
'lag-warn-normal' => 'Endringar som er nyare enn {{PLURAL:$1|sekund|sekund}} er ikkje viste på denne lista.',
'lag-warn-high'   => 'På grunn av stor databaseforseinking, er ikkje endringar som er nyare enn {{PLURAL:$1|sekund|sekund}} viste på denne lista.',

# Watchlist editor
'watchlistedit-numitems'       => 'Overvakingslista di inneheld {{PLURAL:$1|éi side|$1 sider}} (diskusjonssider ikkje medrekna).',
'watchlistedit-noitems'        => 'Overvakingslista di er tom.',
'watchlistedit-normal-title'   => 'Endre overvakingslista',
'watchlistedit-normal-legend'  => 'Fjern sider frå overvakingslista',
'watchlistedit-normal-explain' => 'Sidene på overvakingslista di er viste nedanfor.
For å fjerne ei side, kryss av boksen ved sidan av sida du vil fjerne og klikk på «Fjern side».
Du kan òg [[Special:Watchlist/raw|endre overvakingslista i råformat]].',
'watchlistedit-normal-submit'  => 'Fjern sider',
'watchlistedit-normal-done'    => '{{PLURAL:$1|Éi side|$1 sider}} vart fjerna frå overvakingslista di:',
'watchlistedit-raw-title'      => 'Endre på overvakingslista i råformat',
'watchlistedit-raw-legend'     => 'Endre på overvakingslista i råformat',
'watchlistedit-raw-explain'    => 'Sidene på overvakingslista di er viste nedanfor, og lista kan endrast ved å legge til eller fjerne sider frå lista; ei side per line. Når du er ferdig, klikk «Oppdater overvakingsliste». Du kan òg [[Special:Watchlist/edit|nytte standardverktøyet]].',
'watchlistedit-raw-titles'     => 'Sider:',
'watchlistedit-raw-submit'     => 'Oppdater overvakingslista',
'watchlistedit-raw-done'       => 'Overvakingslista er oppdatert.',
'watchlistedit-raw-added'      => '{{PLURAL:$1|Éi side vart lagt til|$1 sider vart lagde til}}:',
'watchlistedit-raw-removed'    => '{{PLURAL:$1|Éi side|$1 sider}} vart fjerna:',

# Watchlist editing tools
'watchlisttools-view' => 'Vis relevante endringar',
'watchlisttools-edit' => 'Vis og endre overvakingslista',
'watchlisttools-raw'  => 'Endre på overvakingslista i råformat',

# Core parser functions
'unknown_extension_tag' => 'Ukjend tilleggsmerking «$1»',

# Special:Version
'version'                          => 'Versjon', # Not used as normal message but as header for the special page itself
'version-extensions'               => 'Installerte utvidingar',
'version-specialpages'             => 'Spesialsider',
'version-parserhooks'              => 'Parsertillegg',
'version-variables'                => 'Variablar',
'version-other'                    => 'Anna',
'version-mediahandlers'            => 'Mediahandsamarar',
'version-hooks'                    => 'Tilleggsuttrykk',
'version-extension-functions'      => 'Utvidingsfunksjonar',
'version-parser-extensiontags'     => 'Parserutvidingstaggar',
'version-parser-function-hooks'    => 'Parserfunksjonstillegg',
'version-skin-extension-functions' => 'Draktutvidingsfunksjonar',
'version-hook-name'                => 'Namn på tillegg',
'version-hook-subscribedby'        => 'Brukt av',
'version-version'                  => 'versjon',
'version-license'                  => 'Lisens',
'version-software'                 => 'Installert programvare',
'version-software-product'         => 'Produkt',
'version-software-version'         => 'Versjon',

# Special:FilePath
'filepath'         => 'Filsti',
'filepath-page'    => 'Fil:',
'filepath-submit'  => 'Sti',
'filepath-summary' => 'Denne spesialsida gjev den fullstendige stien for ei fil. Bilete vert vist i oppløysing; andre filtypar vert starta direkte i dei tilknytte programma sine.

Skriv inn filnamnet utan «{{ns:image}}:»-prefikset.',

# Special:FileDuplicateSearch
'fileduplicatesearch'          => 'Søk etter duplikatfiler',
'fileduplicatesearch-summary'  => 'Søk etter duplikatfiler basert på hash-verdiane deira.

Skriv inn filnamn utan «{{ns:image}}:»-prefikset.',
'fileduplicatesearch-legend'   => 'Søk etter ei duplikatfil',
'fileduplicatesearch-filename' => 'Filnamn:',
'fileduplicatesearch-submit'   => 'Søk',
'fileduplicatesearch-info'     => '$1 × $2 piksler<br />Filstorleik: $3<br />MIME-type: $4',
'fileduplicatesearch-result-1' => 'Det er ingen kopiar av fila «$1».',
'fileduplicatesearch-result-n' => 'Det er {{PLURAL:$2|éin kopi|$2 kopiar}} av fila «$1».',

# Special:SpecialPages
'specialpages'                   => 'Spesialsider',
'specialpages-note'              => '----
* Vanlege spesialsider.
* <span class="mw-specialpagerestricted">Spesialsider med avgrensa tilgang.</span>',
'specialpages-group-maintenance' => 'Vedlikehaldsrapportar',
'specialpages-group-other'       => 'Andre spesialsider',
'specialpages-group-login'       => 'Innlogging / registrering',
'specialpages-group-changes'     => 'Siste endringar og loggar',
'specialpages-group-media'       => 'Medierapportar og opplastingar',
'specialpages-group-users'       => 'Brukarar og brukartilgangar',
'specialpages-group-highuse'     => 'Mykje brukte sider',
'specialpages-group-pages'       => 'Sidelister',
'specialpages-group-pagetools'   => 'Sideverktøy',
'specialpages-group-wiki'        => 'Informasjon og verktøy for wikien',
'specialpages-group-redirects'   => 'Omdirigerande spesialsider',
'specialpages-group-spam'        => 'Spamverktøy',

# Special:BlankPage
'blankpage'              => 'Tom side',
'intentionallyblankpage' => 'Denne sida er tom med vilje',

);
