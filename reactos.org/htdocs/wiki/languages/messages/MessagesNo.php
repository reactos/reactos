<?php
/** Norwegian (bokmål)‬ (‪Norsk (bokmål)‬)
 *
 * @ingroup Language
 * @file
 *
 * @author Boivie
 * @author Eirik
 * @author EivindJ
 * @author Finnrind
 * @author H92
 * @author Jon Harald Søby
 * @author Jóna Þórunn
 * @author Kph
 * @author Kph-no
 * @author Max sonnelid
 * @author Samuelsen
 * @author Stigmj
 * @author Teak
 * @author לערי ריינהארט
 */

$skinNames = array(
	'standard'    => 'Standard',
	'nostalgia'   => 'Nostalgi',
	'cologneblue' => 'Kølnerblå',
	'monobook'    => 'Monobook',
	'myskin'      => 'Eget utseende',
	'simple'      => 'Enkel',
	'modern'      => 'Moderne',
);

$bookstoreList = array(
	'Antikvariat.net' => 'http://www.antikvariat.net/',
	'Frida' => 'http://wo.uio.no/as/WebObjects/frida.woa/wa/fres?action=sok&isbn=$1&visParametre=1&sort=alfabetisk&bs=50',
	'Bibsys' => 'http://ask.bibsys.no/ask/action/result?cmd=&kilde=biblio&fid=isbn&term=$1&op=and&fid=bd&term=&arstall=&sortering=sortdate-&treffPrSide=50',
	'Akademika' => 'http://www.akademika.no/sok.php?ts=4&sok=$1',
	'Haugenbok' => 'http://www.haugenbok.no/resultat.cfm?st=extended&isbn=$1',
	'Amazon.com' => 'http://www.amazon.com/exec/obidos/ISBN=$1'
);

$namespaceNames = array(
	NS_MEDIA          => 'Medium',
	NS_SPECIAL        => 'Spesial',
	NS_MAIN           => '',
	NS_TALK           => 'Diskusjon',
	NS_USER           => 'Bruker',
	NS_USER_TALK      => 'Brukerdiskusjon',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK   => '$1-diskusjon',
	NS_IMAGE          => 'Bilde',
	NS_IMAGE_TALK     => 'Bildediskusjon',
	NS_MEDIAWIKI      => 'MediaWiki',
	NS_MEDIAWIKI_TALK => 'MediaWiki-diskusjon',
	NS_TEMPLATE       => 'Mal',
	NS_TEMPLATE_TALK  => 'Maldiskusjon',
	NS_HELP           => 'Hjelp',
	NS_HELP_TALK      => 'Hjelpdiskusjon',
	NS_CATEGORY       => 'Kategori',
	NS_CATEGORY_TALK  => 'Kategoridiskusjon',
);

$separatorTransformTable = array(',' => "\xc2\xa0", '.' => ',' );
$linkTrail = '/^([æøåa-z]+)(.*)$/sDu';

$dateFormats = array(
	'mdy time' => 'H:i',
	'mdy date' => 'M j., Y',
	'mdy both' => 'M j., Y "kl." H:i',

	'dmy time' => 'H:i',
	'dmy date' => 'j. M Y',
	'dmy both' => 'j. M Y "kl." H:i',

	'ymd time' => 'H:i',
	'ymd date' => 'Y M j.',
	'ymd both' => 'Y M j. "kl." H:i',
);

$specialPageAliases = array(
	'DoubleRedirects'           => array( 'Doble_omdirigeringer' ),
	'BrokenRedirects'           => array( 'Ødelagte_omdirigeringer' ),
	'Disambiguations'           => array( 'Pekere' ),
	'Userlogin'                 => array( 'Logg_inn' ),
	'Userlogout'                => array( 'Logg_ut' ),
	'CreateAccount'             => array( 'Opprett_konto' ),
	'Preferences'               => array( 'Innstillinger' ),
	'Watchlist'                 => array( 'Overvåkningsliste', 'Overvåkingsliste' ),
	'Recentchanges'             => array( 'Siste_endringer' ),
	'Upload'                    => array( 'Last_opp' ),
	'Imagelist'                 => array( 'Filliste', 'Bildeliste', 'Billedliste' ),
	'Newimages'                 => array( 'Nye_bilder' ),
	'Listusers'                 => array( 'Brukerliste' ),
	'Listgrouprights'           => array( 'Grupperettigheter' ),
	'Statistics'                => array( 'Statistikk' ),
	'Randompage'                => array( 'Tilfeldig_side', 'Tilfeldig' ),
	'Lonelypages'               => array( 'Foreldreløse_sider' ),
	'Uncategorizedpages'        => array( 'Ukategoriserte_sider' ),
	'Uncategorizedcategories'   => array( 'Ukategoriserte_kategorier' ),
	'Uncategorizedimages'       => array( 'Ukategoriserte_filer', 'Ukategoriserte_bilder' ),
	'Uncategorizedtemplates'    => array( 'Ukategoriserte_maler' ),
	'Unusedcategories'          => array( 'Ubrukte_kategorier' ),
	'Unusedimages'              => array( 'Ubrukte_filer', 'Ubrukte_bilder' ),
	'Wantedpages'               => array( 'Ønskede_sider' ),
	'Wantedcategories'          => array( 'Ønskede_kategorier' ),
	'Missingfiles'              => array( 'Manglende_filer', 'Manglende_bilder' ),
	'Mostlinked'                => array( 'Mest_lenkede_sider', 'Mest_lenka_sider' ),
	'Mostlinkedcategories'      => array( 'Største_kategorier' ),
	'Mostlinkedtemplates'       => array( 'Mest_brukte_maler' ),
	'Mostcategories'            => array( 'Flest_kategorier' ),
	'Mostimages'                => array( 'Mest_brukte_bilder', 'Mest_brukte_filer' ),
	'Mostrevisions'             => array( 'Flest_revisjoner' ),
	'Fewestrevisions'           => array( 'Færrest_revisjoner' ),
	'Shortpages'                => array( 'Korte_sider' ),
	'Longpages'                 => array( 'Lange_sider' ),
	'Newpages'                  => array( 'Nye_sider' ),
	'Ancientpages'              => array( 'Gamle_sider' ),
	'Deadendpages'              => array( 'Blindveisider' ),
	'Protectedpages'            => array( 'Beskyttede_sider' ),
	'Protectedtitles'           => array( 'Beskyttede_titler' ),
	'Allpages'                  => array( 'Alle_sider' ),
	'Prefixindex'               => array( 'Prefiksindeks' ),
	'Ipblocklist'               => array( 'Blokkeringsliste' ),
	'Specialpages'              => array( 'Spesialsider' ),
	'Contributions'             => array( 'Bidrag' ),
	'Emailuser'                 => array( 'E-post' ),
	'Confirmemail'              => array( 'Bekreft_e-postadresse' ),
	'Whatlinkshere'             => array( 'Lenker_hit' ),
	'Recentchangeslinked'       => array( 'Relaterte_endringer' ),
	'Movepage'                  => array( 'Flytt_side' ),
	'Blockme'                   => array( 'Blokker_meg' ),
	'Booksources'               => array( 'Bokkilder' ),
	'Categories'                => array( 'Kategorier' ),
	'Export'                    => array( 'Eksporter' ),
	'Version'                   => array( 'Versjon' ),
	'Allmessages'               => array( 'Alle_systembeskjeder' ),
	'Log'                       => array( 'Logg', 'Logger' ),
	'Blockip'                   => array( 'Blokker' ),
	'Undelete'                  => array( 'Gjenopprett' ),
	'Import'                    => array( 'Importer' ),
	'Lockdb'                    => array( 'Lås_database' ),
	'Unlockdb'                  => array( 'Åpne_database' ),
	'Userrights'                => array( 'Brukerrettigheter' ),
	'MIMEsearch'                => array( 'MIME-søk' ),
	'FileDuplicateSearch'       => array( 'Filduplikatsøk' ),
	'Unwatchedpages'            => array( 'Uovervåkede_sider' ),
	'Listredirects'             => array( 'Omdirigeringsliste' ),
	'Revisiondelete'            => array( 'Revisjonssletting' ),
	'Unusedtemplates'           => array( 'Ubrukte_maler' ),
	'Randomredirect'            => array( 'Tilfeldig_omdirigering' ),
	'Mypage'                    => array( 'Min_side' ),
	'Mytalk'                    => array( 'Min_diskusjon' ),
	'Mycontributions'           => array( 'Mine_bidrag' ),
	'Listadmins'                => array( 'Administratorliste', 'Administratorer' ),
	'Listbots'                  => array( 'Robotliste', 'Liste_over_roboter' ),
	'Popularpages'              => array( 'Populære_sider' ),
	'Search'                    => array( 'Søk' ),
	'Resetpass'                 => array( 'Resett_passord' ),
	'Withoutinterwiki'          => array( 'Uten_interwiki' ),
	'MergeHistory'              => array( 'Flett_historikk' ),
	'Filepath'                  => array( 'Filsti' ),
	'Invalidateemail'           => array( 'Ugyldiggjøre_e-post' ),
);

$messages = array(
# User preference toggles
'tog-underline'               => 'Strek under lenker:',
'tog-highlightbroken'         => 'Formater lenker til ikke-eksisterende sider <a href="" class="new">slik</a> (alternativt: slik<a href="" class="internal">?</a>).',
'tog-justify'                 => 'Blokkjusterte avsnitt',
'tog-hideminor'               => 'Skjul mindre endringer i siste endringer',
'tog-extendwatchlist'         => 'Utvid overvåkningslisten til å vise alle endringer i valgt tidsrom',
'tog-usenewrc'                => 'Forbedret siste endringer (ikke for alle nettlesere)',
'tog-numberheadings'          => 'Nummerer overskrifter',
'tog-showtoolbar'             => 'Vis verktøylinje (JavaScript)',
'tog-editondblclick'          => 'Rediger sider ved å dobbeltklikke (JavaScript)',
'tog-editsection'             => 'Rediger avsnitt ved hjelp av [rediger]-lenke',
'tog-editsectiononrightclick' => 'Rediger avsnitt ved å høyreklikke på avsnittsoverskrift (JavaScript)',
'tog-showtoc'                 => 'Vis innholdsfortegnelse (for sider med mer enn tre seksjoner)',
'tog-rememberpassword'        => 'Husk passordet',
'tog-editwidth'               => 'Full bredde på redigeringsboksen',
'tog-watchcreations'          => 'Overvåk sider jeg oppretter',
'tog-watchdefault'            => 'Overvåk alle sider jeg redigerer',
'tog-watchmoves'              => 'Overvåk sider jeg flytter',
'tog-watchdeletion'           => 'Overvåk sider jeg sletter.',
'tog-minordefault'            => 'Merk i utgangspunktet alle redigeringer som mindre',
'tog-previewontop'            => 'Flytt forhåndsvisningen foran redigeringsboksen',
'tog-previewonfirst'          => 'Vis forhåndsvisning ved første redigering av en side',
'tog-nocache'                 => 'Skru av mellomlagring av sider («caching»)',
'tog-enotifwatchlistpages'    => 'Send meg en e-post når sider på overvåkningslisten blir endret',
'tog-enotifusertalkpages'     => 'Send meg en e-post ved endringer av brukerdiskusjonssiden min',
'tog-enotifminoredits'        => 'Send meg en e-post også ved mindre sideendringer',
'tog-enotifrevealaddr'        => 'Vis min e-postadresse i utgående meldinger',
'tog-shownumberswatching'     => 'Vis antall overvåkende brukere',
'tog-fancysig'                => 'Råsignatur (uten automatisk lenke)',
'tog-externaleditor'          => 'Bruk ekstern behandler som standard (kun for viderekomne, krever spesielle innstillinger på din datamaskin)',
'tog-externaldiff'            => 'Bruk ekstern differanse som standard (kun for viderekomne, krever spesielle innstillinger på din datamaskin)',
'tog-showjumplinks'           => 'Slå på «gå til»-lenker',
'tog-uselivepreview'          => 'Bruk levende forhåndsvisning (eksperimentell JavaScript)',
'tog-forceeditsummary'        => 'Advar meg når jeg ikke gir noen redigeringsforklaring',
'tog-watchlisthideown'        => 'Skjul egne endringer fra overvåkningslisten',
'tog-watchlisthidebots'       => 'Skjul robotendringer fra overvåkningslisten',
'tog-watchlisthideminor'      => 'Skjul mindre endringer fra overvåkningslisten',
'tog-nolangconversion'        => 'Slå av variantkonvertering',
'tog-ccmeonemails'            => 'Send meg kopier av e-poster jeg sender til andre brukere',
'tog-diffonly'                => 'Ikke vis sideinnhold under differ',
'tog-showhiddencats'          => 'Vis skjulte kategorier',

'underline-always'  => 'Alltid',
'underline-never'   => 'Aldri',
'underline-default' => 'Bruk nettleserstandard',

'skinpreview' => '(forhåndsvisning)',

# Dates
'sunday'        => 'søndag',
'monday'        => 'mandag',
'tuesday'       => 'tirsdag',
'wednesday'     => 'onsdag',
'thursday'      => 'torsdag',
'friday'        => 'fredag',
'saturday'      => 'lørdag',
'sun'           => 'søn',
'mon'           => 'man',
'tue'           => 'tir',
'wed'           => 'ons',
'thu'           => 'tor',
'fri'           => 'fre',
'sat'           => 'lør',
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
'december-gen'  => 'desember',
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
'pagecategories'                 => '{{PLURAL:$1|Kategori|Kategorier}}',
'category_header'                => 'Artikler i kategorien «$1»',
'subcategories'                  => 'Underkategorier',
'category-media-header'          => 'Filer i kategorien «$1»',
'category-empty'                 => "''Denne kategorien inneholder for tiden ingen artikler eller filer.''",
'hidden-categories'              => '{{PLURAL:$1|Skjult kategori|Skjulte kategorier}}',
'hidden-category-category'       => 'Skjulte kategorier', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|Denne kategorien har kun den følgende underkategorien.|Denne kategorien har følgende {{PLURAL:$1|underkategori|$1 underkategorier}}, av totalt $2.}}',
'category-subcat-count-limited'  => 'Kategorien har følgende {{PLURAL:$1|underkategori|$1 underkategorier}}.',
'category-article-count'         => '{{PLURAL:$2|Denne kategorien inneholder kun den følgende siden.|Følgende {{PLURAL:$1|side|$1 sider}} er i denne kategorien, av totalt $2.}}',
'category-article-count-limited' => 'Følgende {{PLURAL:$1|side|$1 sider}} er i denne kategorien.',
'category-file-count'            => '{{PLURAL:$2|Denne kategorien inneholder kun den følgende filen.|Følgende {{PLURAL:$1|fil|$1 filer}} er i denne kategorien, av totalt $2.}}',
'category-file-count-limited'    => 'Følgende {{PLURAL:$1|fil|$1 filer}} er i denne kategorien.',
'listingcontinuesabbrev'         => ' forts.',

'mainpagetext'      => "<big>'''MediaWiki-programvaren er nå installert.'''</big>",
'mainpagedocfooter' => 'Se [http://meta.wikimedia.org/wiki/Help:Contents brukerveiledningen] for informasjon om hvordan du bruker wiki-programvaren.

==Å starte==
*[http://www.mediawiki.org/wiki/Manual:Configuration_settings Oppsettsliste]
*[http://www.mediawiki.org/wiki/Manual:FAQ Ofte stilte spørsmål]
*[http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce MediaWiki e-postliste]',

'about'          => 'Om',
'article'        => 'Innholdsside',
'newwindow'      => '(åpner i nytt vindu)',
'cancel'         => 'Avbryt',
'qbfind'         => 'Finn',
'qbbrowse'       => 'Bla gjennom',
'qbedit'         => 'Rediger',
'qbpageoptions'  => 'Sideinnstillinger',
'qbpageinfo'     => 'Sideinformasjon',
'qbmyoptions'    => 'Egne innstillinger',
'qbspecialpages' => 'Spesialsider',
'moredotdotdot'  => 'Mer …',
'mypage'         => 'Min side',
'mytalk'         => 'Min diskusjonsside',
'anontalk'       => 'Brukerdiskusjon for denne IP-adressen',
'navigation'     => 'Navigasjon',
'and'            => 'og',

# Metadata in edit box
'metadata_help' => 'Metadata:',

'errorpagetitle'    => 'Feil',
'returnto'          => 'Tilbake til $1.',
'tagline'           => 'Fra {{SITENAME}}',
'help'              => 'Hjelp',
'search'            => 'Søk',
'searchbutton'      => 'Søk',
'go'                => 'Gå',
'searcharticle'     => 'Gå',
'history'           => 'Historikk',
'history_short'     => 'Historikk',
'updatedmarker'     => 'oppdatert siden mitt forrige besøk',
'info_short'        => 'Informasjon',
'printableversion'  => 'Utskriftsvennlig versjon',
'permalink'         => 'Permanent lenke',
'print'             => 'Skriv ut',
'edit'              => 'Rediger',
'create'            => 'Opprett',
'editthispage'      => 'Rediger siden',
'create-this-page'  => 'Opprett denne siden',
'delete'            => 'Slett',
'deletethispage'    => 'Slett denne siden',
'undelete_short'    => 'Gjenopprett {{PLURAL:$1|én revisjon|$1 revisjoner}}',
'protect'           => 'Lås',
'protect_change'    => 'endre',
'protectthispage'   => 'Lås siden',
'unprotect'         => 'Åpne',
'unprotectthispage' => 'Åpne siden',
'newpage'           => 'Ny side',
'talkpage'          => 'Diskuter siden',
'talkpagelinktext'  => 'Diskusjon',
'specialpage'       => 'Spesialside',
'personaltools'     => 'Personlige verktøy',
'postcomment'       => 'Legg til en kommentar',
'articlepage'       => 'Vis innholdsside',
'talk'              => 'Diskusjon',
'views'             => 'Visninger',
'toolbox'           => 'Verktøy',
'userpage'          => 'Vis brukerside',
'projectpage'       => 'Vis prosjektside',
'imagepage'         => 'Vis medieside',
'mediawikipage'     => 'Vis beskjedside',
'templatepage'      => 'Vis mal',
'viewhelppage'      => 'Vis hjelpeside',
'categorypage'      => 'Vis kategoriside',
'viewtalkpage'      => 'Vis diskusjon',
'otherlanguages'    => 'Andre språk',
'redirectedfrom'    => '(Omdirigert fra $1)',
'redirectpagesub'   => 'Omdirigeringsside',
'lastmodifiedat'    => 'Denne siden ble sist endret $1 kl. $2.', # $1 date, $2 time
'viewcount'         => 'Denne siden er vist $1 {{PLURAL:$1|gang|ganger}}.',
'protectedpage'     => 'Låst side',
'jumpto'            => 'Gå til:',
'jumptonavigation'  => 'navigasjon',
'jumptosearch'      => 'søk',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'Om {{SITENAME}}',
'aboutpage'            => 'Project:Om',
'bugreports'           => 'Feilmeldinger',
'bugreportspage'       => 'Project:Feilmeldinger',
'copyright'            => 'Innholdet er tilgjengelig under $1.',
'copyrightpagename'    => 'Opphavsrett',
'copyrightpage'        => '{{ns:project}}:Opphavsrett',
'currentevents'        => 'Aktuelt',
'currentevents-url'    => 'Project:Aktuelt',
'disclaimers'          => 'Opphavsrett',
'disclaimerpage'       => 'Project:Opphavsrett',
'edithelp'             => 'Redigeringshjelp',
'edithelppage'         => 'Help:Hvordan redigere',
'faq'                  => 'Ofte stilte spørsmål',
'faqpage'              => 'Project:Ofte stilte spørsmål',
'helppage'             => 'Help:Hjelp',
'mainpage'             => 'Hovedside',
'mainpage-description' => 'Hovedside',
'policy-url'           => 'Project:Retningslinjer',
'portal'               => 'Prosjektportal',
'portal-url'           => 'Project:Prosjektportal',
'privacy'              => 'Personvern',
'privacypage'          => 'Project:Personvern',

'badaccess'        => 'Rettighetsfeil',
'badaccess-group0' => 'Du har ikke tilgang til å utføre handlingen du prøvde på.',
'badaccess-group1' => 'Handlingen du prøvde å utføre er begrenset til $1.',
'badaccess-group2' => 'Handlingen du prøvde å utføre kan kun utføres av $1.',
'badaccess-groups' => 'Handlingen du prøvde å utføre kan kun utføres av $1.',

'versionrequired'     => 'Versjon $1 av MediaWiki påtrengt',
'versionrequiredtext' => 'Versjon $1 av MediaWiki er nødvendig for å bruke denne siden. Se [[Special:Version|versjonsiden]]',

'ok'                      => 'OK',
'retrievedfrom'           => 'Hentet fra «$1»',
'youhavenewmessages'      => 'Du har $1 ($2).',
'newmessageslink'         => 'nye meldinger',
'newmessagesdifflink'     => 'forskjell fra forrige beskjed',
'youhavenewmessagesmulti' => 'Du har nye beskjeder på $1',
'editsection'             => 'rediger',
'editold'                 => 'rediger',
'viewsourceold'           => 'vis kilde',
'editsectionhint'         => 'Rediger seksjon: $1',
'toc'                     => 'Innhold',
'showtoc'                 => 'vis',
'hidetoc'                 => 'skjul',
'thisisdeleted'           => 'Se eller gjenopprett $1?',
'viewdeleted'             => 'Vis $1?',
'restorelink'             => '{{PLURAL:$1|én slettet revisjon|$1 slettede revisjoner}}',
'feedlinks'               => 'Mating:',
'feed-invalid'            => 'Ugyldig matingstype.',
'feed-unavailable'        => 'Abonnementskilder er ikke tilgjengelig på {{SITENAME}}',
'site-rss-feed'           => '$1 RSS-kilde',
'site-atom-feed'          => '$1 Atom-kilde',
'page-rss-feed'           => '«$1» RSS-kilde',
'page-atom-feed'          => '«$1» Atom-kilde',
'red-link-title'          => '$1 (finnes ikke ennå)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Side',
'nstab-user'      => 'Brukerside',
'nstab-media'     => 'Mediaside',
'nstab-special'   => 'Spesial',
'nstab-project'   => 'Prosjektside',
'nstab-image'     => 'Fil',
'nstab-mediawiki' => 'Beskjed',
'nstab-template'  => 'Mal',
'nstab-help'      => 'Hjelp',
'nstab-category'  => 'Kategori',

# Main script and global functions
'nosuchaction'      => 'Funksjonen finnes ikke',
'nosuchactiontext'  => 'MediaWiki-programvaren kjenner ikke igjen funksjonen som er spesifisert i URL-en.',
'nosuchspecialpage' => 'En slik spesialside finnes ikke',
'nospecialpagetext' => 'Du ba om en ugyldig spesialside; en liste over gyldige spesialsider finnes på [[Special:SpecialPages|{{int:specialpages}}]].',

# General errors
'error'                => 'Feil',
'databaseerror'        => 'databasefeil',
'dberrortext'          => 'Det skjedde en syntaksfeil i databasen. Den sist forsøkte forespørselen var: <blockquote><tt>$1</tt></blockquote> fra funksjonen «<tt>$2</tt>». MySQL returnerte feilen «<tt>$3: $4</tt>».',
'dberrortextcl'        => 'Det skjedde en syntaksfeil i databasen. Den sist forsøkte forespørselen var: «$1» fra funksjonen «$2». MySQL returnerte feilen «$3: $4».',
'noconnect'            => 'Beklager! Wikien har tekniske problemer, og kan ikke kontakte databasetjeneren.
<br />$1',
'nodb'                 => 'Klarte ikke å velge databasen $1',
'cachederror'          => 'Det følgende er en lagret kopi av den ønskede siden, og er ikke nødvendigvis oppdatert.',
'laggedslavemode'      => 'Advarsel: Dette kan være en eldre versjon av siden.',
'readonly'             => 'Databasen er skrivebeskyttet',
'enterlockreason'      => 'Skriv en begrunnelse for skrivebeskyttelsen, inkludert et estimat for når den blir opphevet',
'readonlytext'         => 'Databasen er for øyeblikket skrivebeskyttet, sannsynligvis på grunn av rutinemessig vedlikehold.

Administratoren som låste databasen ga forklaringen: $1',
'missing-article'      => 'Databasen burde ha funnet siden «$1» $2, men det gjorde den ikke.

De vanligste grunnene til denne feilen er en lenke til en diff mellom forskjellige versjoner eller lenker til en gammel versjon av en side som har blitt slettet.

Om det ikke er tilfellet kan du ha funnet en feil i programvaren.
Rapporter gjerne problemet til en [[Special:ListUsers/sysop|administrator]], angi da adressen til siden.',
'missingarticle-rev'   => '(versjon $1)',
'missingarticle-diff'  => '(diff: $1, $2)',
'readonly_lag'         => 'Databasen er automatisk skrivebeskyttet så slavetjenerne kan ta igjen mestertjeneren',
'internalerror'        => 'Intern feil',
'internalerror_info'   => 'Intern feil: $1',
'filecopyerror'        => 'Klarte ikke å kopiere filen «$1» til «$2».',
'filerenameerror'      => 'Klarte ikke å døpe om filen «$1» til «$2».',
'filedeleteerror'      => 'Klarte ikke å slette filen «$1».',
'directorycreateerror' => 'Klarte ikke å opprette mappe «$1».',
'filenotfound'         => 'Klarte ikke å finne filen «$1».',
'fileexistserror'      => 'Klarte ikke å skrive til filen «$1»: filen finnes fra før',
'unexpected'           => 'Uventet verdi: «$1»=«$2».',
'formerror'            => 'Feil: klarte ikke å sende skjema',
'badarticleerror'      => 'Handlingen kan ikke utføres på denne siden.',
'cannotdelete'         => 'Kunne ikke slette filen (den kan være slettet av noen andre).',
'badtitle'             => 'Ugyldig tittel',
'badtitletext'         => 'Den ønskede tittelen var ugyldig, tom eller galt lenket fra et annet språk.',
'perfdisabled'         => 'Denne funksjonen er midlertidig utilgjengelig av vedlikeholdsgrunner.',
'perfcached'           => 'Følgende data er en lagret kopi, og ikke nødvendigvis den siste versjonen i databasen.',
'perfcachedts'         => 'Følgende data er en lagret kopi, og ble sist oppdatert $1.',
'querypage-no-updates' => 'Oppdateringer for denne siden er slått av. Data her blir ikke gjenoppfrisket.',
'wrong_wfQuery_params' => 'Gale paramtere til wfQuery()<br />
Funksjon: $1<br />
Spørring: $2',
'viewsource'           => 'Vis kildekode',
'viewsourcefor'        => 'for $1',
'actionthrottled'      => 'Handlingsgrense overskredet',
'actionthrottledtext'  => 'For å beskytte mot spam, kan du ikke utføre denne handlingen for mange ganger i løpet av et kort tidssrom, og du har overskredet denne grensen. Prøv igjen om noen minutter.',
'protectedpagetext'    => 'Denne siden har blitt låst for redigeringer.',
'viewsourcetext'       => 'Du kan se og kopiere kilden til denne siden:',
'protectedinterface'   => 'Denne siden viser brukergrensesnittet for programvaren, og er låst for å hindre misbruk.',
'editinginterface'     => "'''Advarsel:''' Du redigerer en side som brukes i grensesnittet for programvaren. Endringer på denne siden vil påvirke hvordan grensesnittet vil se ut. For oversettelser er det best om du bruker [http://translatewiki.net/wiki/Main_Page?setlang=no Betawiki], prosjektet for oversettelse av MediaWiki.",
'sqlhidden'            => '(SQL-spørring skjult)',
'cascadeprotected'     => 'Denne siden er låst for redigering fordi den inkluderes på følgende sider som har dypbeskyttelse slått på:<!--{{PLURAL:$1}}-->
$2',
'namespaceprotected'   => "Du har ikke tillatelse til å redigere sider i navnerommet '''$1'''.",
'customcssjsprotected' => 'Du har ikke tillatelse til å redigere denne siden, fordi den inneholder en annen brukers personlige innstillinger.',
'ns-specialprotected'  => 'Sier i navnerommet {{ns:special}} kan ikke redigeres.',
'titleprotected'       => "Denne tittelen har blitt låst for oppretting av [[User:$1|$1]].
Den angitte grunnen er ''$2''.",

# Virus scanner
'virus-badscanner'     => 'Dårlig konfigurasjon: ukjent virusskanner: <i>$1</i>',
'virus-scanfailed'     => 'skanning mislyktes (kode $1)',
'virus-unknownscanner' => 'ukjent antivirusprogram:',

# Login and logout pages
'logouttitle'                => 'Logg ut',
'logouttext'                 => '<strong>Du er nå logget ut.</strong>

Du kan fortsette å bruke {{SITENAME}} anonymt, eller [[Special:UserLogin|logge inn igjen]] som samme eller annen bruker.
Merk at noen sider kan vise at du fortsatt er logget inn fram til du tømmer mellomlageret i nettleseren.',
'welcomecreation'            => '==Velkommen, $1!==
Brukerkontoen din har blitt opprettet.
Ikke glem å endre [[Special:Preferences|innstillingene]] dine.',
'loginpagetitle'             => 'Logg inn',
'yourname'                   => 'Brukernavn:',
'yourpassword'               => 'Passord:',
'yourpasswordagain'          => 'Gjenta passord',
'remembermypassword'         => 'Husk passordet',
'yourdomainname'             => 'Ditt domene',
'externaldberror'            => 'Det var en ekstern autentifiseringsfeil, eller du kan ikke oppdatere din eksterne konto.',
'loginproblem'               => '<strong>Du ble ikke logget inn.</strong><br />Prøv igjen!',
'login'                      => 'Logg inn',
'nav-login-createaccount'    => 'Logg inn eller registrer deg',
'loginprompt'                => 'Du må ha slått på informasjonskapsler for å logge in på {{SITENAME}}.',
'userlogin'                  => 'Logg inn eller registrer deg',
'logout'                     => 'Logg ut',
'userlogout'                 => 'Logg ut',
'notloggedin'                => 'Ikke logget inn',
'nologin'                    => 'Er du ikke registrert? $1.',
'nologinlink'                => 'Registrer deg',
'createaccount'              => 'Opprett ny konto',
'gotaccount'                 => 'Har du allerede et brukernavn? $1.',
'gotaccountlink'             => 'Logg inn',
'createaccountmail'          => 'per e-post',
'badretype'                  => 'Passordene samsvarte ikke.',
'userexists'                 => 'Brukernavnet er allerede i bruk. Velg et nytt.',
'youremail'                  => 'E-post:',
'username'                   => 'Brukernavn:',
'uid'                        => 'Bruker-ID:',
'prefs-memberingroups'       => 'Medlem i følgende {{PLURAL:$1|gruppe|grupper}}:',
'yourrealname'               => 'Virkelig navn:',
'yourlanguage'               => 'Språk:',
'yournick'                   => 'Signatur:',
'badsig'                     => 'Ugyldig råsignatur; sjekk HTML-elementer.',
'badsiglength'               => 'Signaturen er for lang.
Den kan maks inneholde $1 {{PLURAL:$1|tegn|tegn}}.',
'email'                      => 'E-post',
'prefs-help-realname'        => '* Virkelig navn (valgfritt): dersom du velger å oppgi navnet, vil det bli brukt til å kreditere deg for ditt arbeid.',
'loginerror'                 => 'Innloggingsfeil',
'prefs-help-email'           => 'Å angi e-postadresse er valgfritt, men lar det motta nytt passord om du glemmer det gamle.
Du kan også la velge å la andre brukere kontakte deg via brukersiden din uten å røpe identiteten din.',
'prefs-help-email-required'  => 'E-postadresse er påkrevd.',
'nocookiesnew'               => 'Din brukerkonto er nå opprettet, men du har ikke logget på. {{SITENAME}} bruker informasjonskapsler («cookies») for å logge brukere på. Du har slått dem av. Slå dem p åfor å kunne logge på med ditt nye brukernavn og passord.',
'nocookieslogin'             => '{{SITENAME}} bruker informasjonskapsler («cookies») for å logge brukere på. Du har slått dem av. Slå dem på og prøv igjen.',
'noname'                     => 'Du har ikke oppgitt et gyldig brukernavn.',
'loginsuccesstitle'          => 'Du er nå logget inn',
'loginsuccess'               => 'Du er nå logget inn på {{SITENAME}} som «$1».',
'nosuchuser'                 => '!Det eksisterer ingen bruker ved navn «$1».
Sjekk stavemåten eller [[Special:Userlogin/signup|opprett en ny konto]].',
'nosuchusershort'            => 'Det finnes ingen bruker ved navn «<nowiki>$1</nowiki>». Kontroller stavemåten.',
'nouserspecified'            => 'Du må oppgi et brukernavn.',
'wrongpassword'              => 'Du har oppgitt et ugyldig passord. Prøv igjen.',
'wrongpasswordempty'         => 'Du oppga ikke noe passord. Prøv igjen.',
'passwordtooshort'           => 'Passordet ditt er ugyldig eller for kort.
Det må ha minst {{PLURAL:$1|ett tegn|$1 tegn}} og kan ikke være det samme som brukernavnet ditt.',
'mailmypassword'             => 'Send nytt passord',
'passwordremindertitle'      => 'Nytt midlertidig passord fra {{SITENAME}}',
'passwordremindertext'       => 'Noen (antagelig deg, fra IP-adressen $1) ba oss sende deg et nytt
passord til {{SITENAME}} ($4). Et midlertidig passord for «$2» har
blitt laget og sendt til «$3». Om det var det du ville, må du logge inn
og velge et nytt passord nå.

Dersom denne forespørselen ble utført av noen andre, eller om du kom på passordet
og ikke lenger ønsker å endre det, kan du ignorere denne beskjeden
og fortsette å bruke det gamle passordet.',
'noemail'                    => 'Det er ikke registrert noen e-postadresse for brukeren «$1».',
'passwordsent'               => 'Et nytt passord har blitt sendt til e-postadressen registrert på bruker «$1». Logg inn når du har mottatt det nye passordet.',
'blocked-mailpassword'       => 'IP-adressen din er blokkert fra å redigere, og for å forhindre misbruk kan du heller ikke bruke funksjonen som gir deg nytt passord.',
'eauthentsent'               => 'En bekreftelsesmelding ble sendt til gitte e-postadresse. Før andre e-poster kan sendes til kontoen må du følge instruksjonene i e-posten for å bekrefte at kontoen faktisk er din.',
'throttled-mailpassword'     => 'En passordpåminnelse ble sendt for mindre enn {{PLURAL:$1|en time|$1 timer}} siden.
For å forhindre misbruk kan kun én passordpåminnelse sendes per {{PLURAL:$1|time|$1 timer}}.',
'mailerror'                  => 'Feil under sending av e-post: $1',
'acct_creation_throttle_hit' => 'Beklager, du har allerede opprettet {{PLURAL:$1|én konto|$1 kontoer}}. Du kan ikke opprette flere.',
'emailauthenticated'         => 'Din e-postadresse ble bekreftet $1.',
'emailnotauthenticated'      => 'Din e-postadresse er ikke bekreftet. Du vil ikke kunne motta e-post for noen av følgende egenskaper.',
'noemailprefs'               => 'Oppgi en e-postadresse for at disse funksjonene skal fungere.',
'emailconfirmlink'           => 'Bekreft e-postadressen din.',
'invalidemailaddress'        => 'Din e-postadresse kan ikke aksepteres, fordi den er ugyldig formatert.
Skriv inn en fungerende e-postadresse eller tøm feltet.',
'accountcreated'             => 'Konto opprettet',
'accountcreatedtext'         => 'Brukerkonto for $1 har blitt opprettet.',
'createaccount-title'        => 'Kontooppretting på {{SITENAME}}',
'createaccount-text'         => 'Noen opprettet en konto for din e-postadresse på {{SITENAME}} ($4) med navnet «$2», med «$3» som passord. Du burde logge inn og endre passordet nå.

Du kan ignorere denne beskjeden dersom kontoen ble opprettet ved en feil.',
'loginlanguagelabel'         => 'Språk: $1',

# Password reset dialog
'resetpass'               => 'Resett kontopassord',
'resetpass_announce'      => 'Du logget inn med en midlertidig e-postkode. For å fullføre innloggingen må du oppgi et nytt passord her:',
'resetpass_text'          => '<!-- Legg til tekst her -->',
'resetpass_header'        => 'Nullstill passord',
'resetpass_submit'        => 'Angi passord og logg inn',
'resetpass_success'       => 'Passordet ditt ble endret! Logger inn&nbsp;…',
'resetpass_bad_temporary' => 'Ugyldig midlertidig passord. Du kan allerede ha endret passordet, eller bedt om et nytt midlertidig passord.',
'resetpass_forbidden'     => 'Passord kan ikke endres på {{SITENAME}}',
'resetpass_missing'       => 'Ingen skjemadata.',

# Edit page toolbar
'bold_sample'     => 'Fet tekst',
'bold_tip'        => 'Fet tekst',
'italic_sample'   => 'Kursiv tekst',
'italic_tip'      => 'Kursiv tekst',
'link_sample'     => 'Lenketittel',
'link_tip'        => 'Intern lenke',
'extlink_sample'  => 'http://www.example.com lenketittel',
'extlink_tip'     => 'Ekstern lenke (husk prefikset http://)',
'headline_sample' => 'Overskrift',
'headline_tip'    => 'Overskrift',
'math_sample'     => 'Sett inn formel her',
'math_tip'        => 'Matematisk formel (LaTeX)',
'nowiki_sample'   => 'Sett inn uformatert tekst her',
'nowiki_tip'      => 'Ignorer wikiformatering',
'image_sample'    => 'Eksempel.jpg',
'image_tip'       => 'Fil',
'media_sample'    => 'Eksempel.ogg',
'media_tip'       => 'Fillenke',
'sig_tip'         => 'Din signatur med dato',
'hr_tip'          => 'Horisontal linje',

# Edit pages
'summary'                          => 'Redigeringsforklaring',
'subject'                          => 'Overskrift',
'minoredit'                        => 'Mindre endring',
'watchthis'                        => 'Overvåk denne siden',
'savearticle'                      => 'Lagre siden',
'preview'                          => 'Forhåndsvisning',
'showpreview'                      => 'Forhåndsvisning',
'showlivepreview'                  => 'Levende forhåndsvisning',
'showdiff'                         => 'Vis endringer',
'anoneditwarning'                  => "'''Advarsel:''' Du er ikke logget inn. IP-adressen din blir bevart i sidens redigeringshistorikk.",
'missingsummary'                   => "'''Påminnelse:''' Du har ikke lagt inn en redigeringsforklaring.
Velger du ''Lagre siden'' en gang til blir endringene lagret uten forklaring.",
'missingcommenttext'               => 'Vennligst legg inn en kommentar under.',
'missingcommentheader'             => "'''Merk:''' Du har ikke angitt et emne/overskrift for denne kommentaren. Om du trykker Lagre igjen, vil redigeringen din bli lagret uten en.",
'summary-preview'                  => 'Forhåndsvisning av sammendrag',
'subject-preview'                  => 'Forhåndsvisning av emne/overskrift',
'blockedtitle'                     => 'Brukeren er blokkert',
'blockedtext'                      => "<big>'''Ditt brukernavn eller din IP-adresse har blitt blokkert.'''</big>

Blokkeringen ble utført av $1. Grunnen som ble oppgitt var ''$2''.

* Blokkeringen begynte: $8
* Blokkeringen utgår: $6
* Blokkering ment på: $7

Du kan kontakte $1 eller en annen [[{{MediaWiki:Grouppage-sysop}}|administrator]] for å diskutere blokkeringen.
Du kan ikke bruke «E-post til denne brukeren»-funksjonen med mindre du har oppgitt en gyldig e-postadresse i [[Special:Preferences|innstillingene dine]] og du ikke er blokkert fra å sende e-post.
Din nåværende IP-adresse er $3, og blokkerings-ID-en er #$5.
Vennligst ta all denne informasjonen ved henvendelser.",
'autoblockedtext'                  => "Din IP-adresse har blitt automatisk blokkert fordi den ble brukt av en annen bruker som ble blokkert av $1.
Den oppgitte grunnen var:

:'''$2'''

* Blokkeringen begynte: $8
* Blokkeringen utgår: $6
* Blokkeringen er ment for: $7

Du kan kontakte $1 eller en av de andre [[{{MediaWiki:Grouppage-sysop}}|administratorene]] for å diskutere blokkeringen.

Merk at du ikke kan bruke «E-post til denne brukeren»-funksjonen med mindre du har registrert en gyldig e-postadresse i [[Special:Preferences|innstillingene dine]].

Din IP-adresse er $3, og blokkerings-ID-en er #$5.
Vennligst ta med all denne informasjonen ved henvendelser.",
'blockednoreason'                  => 'ingen grunn gitt',
'blockedoriginalsource'            => "Kildekoden til '''$1''' vises nedenfor:",
'blockededitsource'                => "Kildekoden '''dine endringer''' på '''$1''' vises nedenfor:",
'whitelistedittitle'               => 'Du må logge inn for å redigere',
'whitelistedittext'                => 'Du må $1 for å redigere artikler.',
'confirmedittitle'                 => 'E-postbekreftelse nødvendig før du kan redigere',
'confirmedittext'                  => 'Du må bekrefte e-postadressen din før du kan redigere sider. Vennligst oppgi og bekreft e-postadressen din via [[Special:Preferences|innstillingene dine]].',
'nosuchsectiontitle'               => 'Ingen slik seksjon',
'nosuchsectiontext'                => 'Du prøvde å redigere en seksjon som ikke eksisterer. Siden det ikke finnes noen seksjon «$1», er det ikke mulig å lagre endringen din.',
'loginreqtitle'                    => 'Innlogging kreves',
'loginreqlink'                     => 'logg inn',
'loginreqpagetext'                 => 'Du må $1 for å se andre sider.',
'accmailtitle'                     => 'Passord sendt.',
'accmailtext'                      => 'Passordet for «$1» ble sendt til $2.',
'newarticle'                       => '(Ny)',
'newarticletext'                   => "Du fulgte en lenke til en side som ikke finnes ennå. For å opprette siden, start å skrive i boksen nedenfor (se [[{{MediaWiki:Helppage}}|hjelpesiden]] for mer informasjon). Om du kom hit ved en feil, bare trykk på nettleserens '''tilbake'''-knapp.",
'anontalkpagetext'                 => "----
''Dette er en diskusjonsside for en uregistrert bruker som ikke har opprettet konto eller ikke er logget inn.
Vi er derfor nødt til å bruke den numeriske IP-adressen til å identifisere ham eller henne.
En IP-adresse kan være delt mellom flere brukere.
Hvis du er en uregistrert bruker og synes at du har fått irrelevante kommentarer på en slik side, [[Special:UserLogin/signup|opprett en konto]] eller [[Special:UserLogin|logg inn]] så vi unngår framtidige forvekslinger med andre uregistrerte brukere.''",
'noarticletext'                    => 'Det er ikke noe tekst på denne siden. Du kan [[Special:Search/{{PAGENAME}}|søke etter siden]] i andre sider, eller [{{fullurl:{{FULLPAGENAME}}|action=edit}} opprette den].',
'userpage-userdoesnotexist'        => 'Brukerkontoen «$1» er ikke registrert. Sjekk om du ønsker å opprette/redigere denne siden.',
'clearyourcache'                   => "'''Merk:''' Etter lagring vil det kanskje være nødvendig at nettleseren sletter mellomlageret sitt for at endringene skal tre i kraft. '''Mozilla og Firefox:''' trykk ''Ctrl-Shift-R'', '''Internet Explorer:''' ''Ctrl-F5'', '''Safari:''' ''Cmd-Shift-R'' i engelskspråklig versjon, ''Cmd-Alt-E'' i norskspråklig versjon, '''Konqueror og Opera:''' ''F5''.",
'usercssjsyoucanpreview'           => '<strong>Tips:</strong> Bruk «Forhåndsvisning»-knappen for å teste din nye CSS/JS før du lagrer.',
'usercsspreview'                   => "'''Husk at dette bare er en forhåndsvisning av din bruker-CSS og at den ikke er lagret!'''",
'userjspreview'                    => "'''Husk at dette bare er en test eller forhåndsvisning av ditt bruker-JavaScript, og det ikke er lagret!'''",
'userinvalidcssjstitle'            => "'''Advarsel:''' Det finnes ikke noe utseende ved navn «$1». Husk at .css- og .js-sider bruker titler i små bokstaver, for eksempel {{ns:user}}:Eksempel/monobook.css, ikke {{ns:user}}:Eksempel/Monobook.css",
'updated'                          => '(Oppdatert)',
'note'                             => '<strong>Merk:</strong>',
'previewnote'                      => '<strong>Dette er bare en forhåndsvisning; endringer har ikke blitt lagret!</strong>',
'previewconflict'                  => 'Slik vil teksten i redigeringsvinduet se ut dersom du lagrer den.',
'session_fail_preview'             => '<strong>Beklager! Klarte ikke å lagre redigeringen din. Prøv igjen. Om det fortsetter å gå galt, prøv å [[Special:UserLogout|logge ut]] og så inn igjen.</strong>',
'session_fail_preview_html'        => "<strong>Beklager! Klarte ikke å lagre redigeringen din på grunn av tap av øktdata.</strong>

''Fordi {{SITENAME}} har rå HTML slått på, er forhåndsvisningen skjult for å forhindre JavaScript-angrep.''

<strong>Om dette er et legitimt redigeringsforsøk, prøv igjen. Om det da ikke fungerer, prøv å [[Special:UserLogout|logge ut]] og logge inn igjen.</strong>",
'token_suffix_mismatch'            => '<strong>Redigeringen din har blitt avvist fordi klienten din ikke hadde punktasjonstegn i redigeringsteksten. Redigeringen har blitt avvist for å hindre ødeleggelse av artikkelteksten. Dette forekommer av og til når man bruker vevbaserte anonyme proxytjenester.</strong>',
'editing'                          => 'Redigerer $1',
'editingsection'                   => 'Redigerer $1 (seksjon)',
'editingcomment'                   => 'Redigerer $1 (kommentar)',
'editconflict'                     => 'Redigeringskonflikt: $1',
'explainconflict'                  => "Noen andre har endret teksten siden du begynte å redigere.
Den øverste boksen inneholder den nåværende tekst.
Dine endringer vises i den nederste boksen.
Du er nødt til å flette dine endringer sammen med den nåværende teksten.
'''Kun''' teksten i den øverste tekstboksen blir lagret når du trykker «Lagre siden».",
'yourtext'                         => 'Din tekst',
'storedversion'                    => 'Den lagrede versjonen',
'nonunicodebrowser'                => '<strong>ADVARSEL: Nettleseren din har ikke støtte for Unicode. Skru det på før du begynner å redigere artikler.</strong>',
'editingold'                       => '<strong>ADVARSEL:
Du redigerer en gammel versjon av denne siden.
Hvis du lagrer den, vil alle endringer foretatt siden denne versjonen bli overskrevet.</strong>',
'yourdiff'                         => 'Forskjeller',
'copyrightwarning'                 => 'Vennligst merk at alle bidrag til {{SITENAME}} anses som utgitt under $2 (se $1 for detaljer). Om du ikke vil at dine bidrag skal kunne redigeres og distribuert fritt, ikke legg det til her.<br />
Du lover også at du har skrevet dette selv, eller kopiert det fra en ressurs som er i public domain eller lignende. <strong>IKKE LEGG TIL OPPHAVSBESKYTTET MATERIALE UTEN TILLATELSE!</strong>',
'copyrightwarning2'                => 'Vennligst merk at alle bidrag til {{SITENAME}} kan bli redigert, endret eller fjernet av andre bidragsytere. Om du ikke vil at dine bidrag skal kunne redigeres fritt, ikke legg det til her.<br />
Du lover også at du har skrevet dette selv, eller kopiert det fra en ressurs som er i public domain eller lignende (se $1 for detaljer). <strong>IKKE LEGG TIL OPPHAVSBESKYTTET MATERIALE UTEN TILLATELSE!</strong>',
'longpagewarning'                  => '<strong>ADVARSEL: Denne siden er $1&nbsp;kB lang; noen eldre nettlesere kan ha problemer med å redigere sider som nærmer seg eller er lengre enn 32&nbsp;kB. Overvei om ikke siden kan deles opp i mindre deler.</strong>',
'longpageerror'                    => '<strong>FEIL: Teksten du prøvde å lagre er $1&nbsp;kB lang, dvs. lenger enn det maksimale $2&nbsp;kB. Den kan ikke lagres.</strong>',
'readonlywarning'                  => '<strong>ADVARSEL: Databasen er låst på grunn av vedlikehold,
så du kan ikke lagre dine endringer akkurat nå. Det kan være en god idé å
kopiere teksten din til en tekstfil, så du kan lagre den til senere.</strong>',
'protectedpagewarning'             => '<strong>ADVARSEL: Denne siden er låst, slik at kun brukere med administratorrettigheter kan redigere den.</strong>',
'semiprotectedpagewarning'         => "'''Merk:''' Denne siden har blitt låst slik at kun registrerte brukere kan endre den. Nyopprettede og uregistrerte brukere kan ikke redigere.",
'cascadeprotectedwarning'          => "'''Advarsel:''' Denne siden har blitt låst slik at kun brukere med administratorrettigheter kan redigere den, fordi den inkluderes på følgende dypbeskyttede sider:<!--{{PLURAL:$1}}-->",
'titleprotectedwarning'            => '<strong>ADVARSEL: Denne siden har blitt låst slik at kun visse brukere kan opprette den.</strong>',
'templatesused'                    => 'Maler i bruk på denne siden:',
'templatesusedpreview'             => 'Maler som brukes i denne forhåndsvisningen:',
'templatesusedsection'             => 'Maler brukt i denne seksjonen:',
'template-protected'               => '(beskyttet)',
'template-semiprotected'           => '(halvbeskyttet)',
'hiddencategories'                 => 'Skjulte kategorier denne siden er medlem av{{PLURAL:$1|:|:}}',
'edittools'                        => '<!-- Teksten her vil vises under redigerings- og opplastingsboksene. -->',
'nocreatetitle'                    => 'Sideoppretting er begrenset',
'nocreatetext'                     => '{{SITENAME}} har begrensede muligheter for oppretting av nye sider. Du kan gå tilbake og redigere en eksisterende side, eller [[Special:UserLogin|logge inn eller opprette en ny konto]].',
'nocreate-loggedin'                => 'Du har ikke tillatelse til å opprette sider på {{SITENAME}}.',
'permissionserrors'                => 'Tilgangsfeil',
'permissionserrorstext'            => 'Du har ikke tillatelse til å utføre dette, av følgende {{PLURAL:$1|grunn|grunner}}:',
'permissionserrorstext-withaction' => 'Du har ikke tillatelse til å $2 {{PLURAL:$1|på grunn av|av følgende grunner}}:',
'recreate-deleted-warn'            => "'''Advarsel: Du gjenskaper en side som tidligere har blitt slettet.'''

Du burde vurdere hvorvidt det er passende å fortsette å redigere denne siden. Slettingsloggen for denne siden gjengis her:",

# Parser/template warnings
'expensive-parserfunction-warning'        => 'Advarsel: Denne siden inneholder for mange prosesskrevende parserfunksjoner.

Det burde være mindre enn $2, men er nå $1.',
'expensive-parserfunction-category'       => 'Sider med for mange prosesskrevende parserfunksjoner',
'post-expand-template-inclusion-warning'  => 'Advarsel: Størrelsen på inkluderte maler er for stor.
Noen maler vil ikke bli inkludert.',
'post-expand-template-inclusion-category' => 'Sider som inneholder for store maler',
'post-expand-template-argument-warning'   => 'Advarsel: Siden inneholder ett eller flere malparametere som blir for lange når de utvides.
Disse parameterne har blitt utelatt.',
'post-expand-template-argument-category'  => 'Sider med utelatte malparametere',

# "Undo" feature
'undo-success' => 'Redigeringen kan omgjøres. Sjekk sammenligningen under for å bekrefte at du vil gjøre dette, og lagre endringene for å fullføre omgjøringen.',
'undo-failure' => 'Redigeringen kunne ikke omgjøres på grunn av konflikterende etterfølgende redigeringer.',
'undo-norev'   => 'Redigeringen kunne ikke fjernes fordi den ikke eksisterer eller ble slettet',
'undo-summary' => 'Fjerner revisjon $1 av [[Special:Contributions/$2]] ([[User talk:$2|diskusjon]] | [[Special:Contributions/$2|{{int:contribsilnk}}]])',

# Account creation failure
'cantcreateaccounttitle' => 'Kan ikke opprette konto',
'cantcreateaccount-text' => "Kontooppretting fra denne IP-adressen ('''$1''') har blitt blokkert av [[User:$3|$3]].

Grunnen som ble oppgitt av $3 er ''$2''",

# History pages
'viewpagelogs'        => 'Vis logger for denne siden',
'nohistory'           => 'Denne siden har ingen historikk.',
'revnotfound'         => 'Versjonen er ikke funnet',
'revnotfoundtext'     => 'Den gamle versjon av siden du etterspurte finnes ikke. Kontroller adressen du brukte for å få adgang til denne siden.',
'currentrev'          => 'Nåværende versjon',
'revisionasof'        => 'Versjonen fra $1',
'revision-info'       => 'Revisjon per $1 av $2',
'previousrevision'    => '← Eldre versjon',
'nextrevision'        => 'Nyere versjon →',
'currentrevisionlink' => 'Nåværende versjon',
'cur'                 => 'nå',
'next'                => 'neste',
'last'                => 'forrige',
'page_first'          => 'første',
'page_last'           => 'siste',
'histlegend'          => "Forklaring: (nå) = forskjell fra nåværende versjon, (forrige) = forskjell fra forrige versjon, '''m''' = mindre endring.",
'deletedrev'          => '[slettet]',
'histfirst'           => 'Første',
'histlast'            => 'Siste',
'historysize'         => '({{PLURAL:$1|1 byte|$1 byte}})',
'historyempty'        => '(tom)',

# Revision feed
'history-feed-title'          => 'Revisjonshistorikk',
'history-feed-description'    => 'Revisjonshistorikk for denne siden',
'history-feed-item-nocomment' => '$1 på $2', # user at time
'history-feed-empty'          => 'Den etterspurte siden finnes ikke. Den kan ha blitt slettet fra wikien, eller fått et nytt navn. Prøv å [[Special:Search|søke]] etter beslektede sider.',

# Revision deletion
'rev-deleted-comment'         => '(kommentar fjernet)',
'rev-deleted-user'            => '(brukernavn fjernet)',
'rev-deleted-event'           => '(fjernet loggoppføring)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">
Denne revisjonen har blitt fjernet fra de offentlige arkivene. Det kan være detaljer i [{{fullurl:Special:Log/delete|page={{PAGENAMEE}}}} slettingsloggen].
</div>',
'rev-deleted-text-view'       => '<div class="mw-warning plainlinks">
Denne revisjonen har blitt fjernet fra det offentlige arkivet. Som administrator har du mulighet til å se den; det kan være detaljer i [{{fullurl:Special:Log/delete|page={{PAGENAMEE}}}} slettingsloggen].
</div>',
'rev-delundel'                => 'vis/skjul',
'revisiondelete'              => 'Slett/gjenopprett revisjoner',
'revdelete-nooldid-title'     => 'Ugyldig målversjon',
'revdelete-nooldid-text'      => 'Du har ikke angitt en målversjon for denne funksjonen, den angitte versjonen finnes ikke, eller du forsøker å skjule den nåværende versjonen.',
'revdelete-selected'          => '{{PLURAL:$2|Valgt revisjon|Valgte revisjoner}} av [[:$1]]:',
'logdelete-selected'          => '{{PLURAL:$1|Valgt loggoppføring|Valgte loggoppføringer}}:',
'revdelete-text'              => 'Slettede revisjoner vil fortsatt vises i sidehistorikken, men innholdet vil ikke være tilgjengelig for offentligheten.

Andre administratorer på {{SITENAME}} vil fortsatt kunne se det skjulte innholdet, og kan gjenopprette det, med mindre videre begrensninger blir gitt av sideoperatørene.',
'revdelete-legend'            => 'Fastsett synlighetsbegrensninger',
'revdelete-hide-text'         => 'Skjul revisjonstekst',
'revdelete-hide-name'         => 'Skjul handling og mål',
'revdelete-hide-comment'      => 'Skjul redigeringsforklaring',
'revdelete-hide-user'         => 'Skjul bidragsyters brukernavn eller IP',
'revdelete-hide-restricted'   => 'La disse begrensningene gjelde for administratorer også, og steng dette grensesnittet',
'revdelete-suppress'          => 'Fjern informasjon også fra administratorer',
'revdelete-hide-image'        => 'Skjul filinnhold',
'revdelete-unsuppress'        => 'Fjern betingelser på gjenopprettede revisjoner',
'revdelete-log'               => 'Kommentar:',
'revdelete-submit'            => 'Utfør for valgte revisjoner',
'revdelete-logentry'          => 'endre revisjonssynlighet for [[$1]]',
'logdelete-logentry'          => 'endre hendelsessynlighet for [[$1]]',
'revdelete-success'           => "'''Revisjonssynlighet satt.'''",
'logdelete-success'           => "'''Hendelsessynlighet satt.'''",
'revdel-restore'              => 'Ender synlighet',
'pagehist'                    => 'Sidehistorikk',
'deletedhist'                 => 'Slettet historikk',
'revdelete-content'           => 'innhold',
'revdelete-summary'           => 'redigeringssammendrag',
'revdelete-uname'             => 'brukernavn',
'revdelete-restricted'        => 'begrensninger gjelder også administratorer',
'revdelete-unrestricted'      => 'fjernet begrensninger for administratorer',
'revdelete-hid'               => 'skjulte $1',
'revdelete-unhid'             => 'synliggjorde $1',
'revdelete-log-message'       => '$1 for $2 {{PLURAL:$2|revisjon|revisjoner}}',
'logdelete-log-message'       => '$1 for $2 {{PLURAL:$2|element|elementer}}',

# Suppression log
'suppressionlog'     => 'Sidefjerningslogg',
'suppressionlogtext' => 'Nedenfor er en liste over sider og blokkeringer med innhold skjult fra administratorer.
Se [[Special:IPBlockList|blokkeringslisten]] for oversikten over nåværende blokkeringer.',

# History merging
'mergehistory'                     => 'Flett sidehistorikker',
'mergehistory-header'              => 'Denne siden lar deg flette historikken til to sider.
Forsikre deg om at denne endringen vil opprettholde historisk sidekontinuitet.',
'mergehistory-box'                 => 'Flett historikken til to sider:',
'mergehistory-from'                => 'Kildeside:',
'mergehistory-into'                => 'Målside:',
'mergehistory-list'                => 'Flettbar redigeringshistorikk',
'mergehistory-merge'               => 'Følgende revisjoner av [[:$1]] kan flettes til [[:$2]]. Du kan velge å flette kun de revisjonene som kom før tidspunktet gitt i tabellen. Merk at bruk av navigasjonslenkene vil resette denne kolonnen.',
'mergehistory-go'                  => 'Vis flettbare redigeringer',
'mergehistory-submit'              => 'Flett revisjoner',
'mergehistory-empty'               => 'Ingen revisjoner kan flettes.',
'mergehistory-success'             => '{{PLURAL:$3|Én revisjon|$3 revisjoner}} av [[:$1]] ble flettet til [[:$2]].',
'mergehistory-fail'                => 'Klarte ikke å utføre historikkfletting; sjekk siden og tidsparameterne igjen.',
'mergehistory-no-source'           => 'Kildesiden $1 finnes ikke.',
'mergehistory-no-destination'      => 'Målsiden $1 finnes ikke.',
'mergehistory-invalid-source'      => 'Kildesiden må ha en gyldig tittel.',
'mergehistory-invalid-destination' => 'Målsiden må ha en gyldig tittel.',
'mergehistory-autocomment'         => 'Flettet [[:$1]] inn i [[:$2]]',
'mergehistory-comment'             => 'Flettet [[:$1]] inn i [[:$2]]: $3',

# Merge log
'mergelog'           => 'Flettingslogg',
'pagemerge-logentry' => 'flettet [[$1]] til [[$2]] (revisjoner fram til $3)',
'revertmerge'        => 'Omgjør fletting',
'mergelogpagetext'   => 'Nedenfor er en liste over de nyligste flettingene av sidehistorikker.',

# Diffs
'history-title'           => 'Revisjonshistorikk for «$1»',
'difference'              => '(Forskjeller mellom versjoner)',
'lineno'                  => 'Linje $1:',
'compareselectedversions' => 'Sammenlign valgte versjoner',
'editundo'                => 'omgjør',
'diff-multi'              => '({{PLURAL:$1|Én mellomrevisjon|$1 mellomrevisjoner}} ikke vist.)',

# Search results
'searchresults'             => 'Søkeresultater',
'searchresulttext'          => 'For mer informasjon om søking i {{SITENAME}}, se [[{{MediaWiki:Helppage}}|{{int:help}}]].',
'searchsubtitle'            => "Du søkte på '''[[:$1]]''' ([[Special:Prefixindex/$1|alle sider som begynner med «$1»]] | [[Special:WhatLinksHere/$1|alle sider som lenker til «$1»]])",
'searchsubtitleinvalid'     => 'For forespørsel "$1"',
'noexactmatch'              => "'''Det er ingen side med tittelen «$1».''' Du kan [[:$1|opprette siden]].",
'noexactmatch-nocreate'     => "'''Det er ingen side med tittelen «$1».'''",
'toomanymatches'            => 'For mange mulige svar, prøv med en annen spørring',
'titlematches'              => 'Artikkeltitler med treff på forespørselen',
'notitlematches'            => 'Ingen artikkeltitler hadde treff på forespørselen',
'textmatches'               => 'Artikkeltekster med treff på forespørselen',
'notextmatches'             => 'Ingen artikkeltekster hadde treff på forespørselen',
'prevn'                     => 'forrige $1',
'nextn'                     => 'neste $1',
'viewprevnext'              => 'Vis ($1) ($2) ($3).',
'search-result-size'        => '$1 ({{PLURAL:$2|ett|$2}} ord)',
'search-result-score'       => 'Relevans: $1&nbsp;%',
'search-redirect'           => '(omdirigering $1)',
'search-section'            => '(seksjon $1)',
'search-suggest'            => 'Mente du: $1',
'search-interwiki-caption'  => 'Søsterprosjekter',
'search-interwiki-default'  => '$1 resultater:',
'search-interwiki-more'     => '(mer)',
'search-mwsuggest-enabled'  => 'med forslag',
'search-mwsuggest-disabled' => 'ingen forslag',
'search-relatedarticle'     => 'Relatert',
'mwsuggest-disable'         => 'Slå av AJAX-forslag',
'searchrelated'             => 'relatert',
'searchall'                 => 'alle',
'showingresults'            => "Nedenfor vises opptil {{PLURAL:$1|'''ett''' resultat|'''$1''' resultater}} fra og med nummer <b>$2</b>.",
'showingresultsnum'         => "Nedenfor vises {{PLURAL:$3|'''ett''' resultat|'''$3''' resultater}} fra og med nummer '''$2'''.",
'showingresultstotal'       => "Viser resultat '''{{PLURAL:$3|$1|$1–$2}}''' av '''$3''' nedenfor",
'nonefound'                 => "'''Merk:''' Som standard søkes det kun i enkelte navnerom. For å søke i alle, bruk prefikset ''all:'' (inkluderer diskusjonssider, maler etc.), eller bruk det ønskede navnerommet som prefiks.",
'powersearch'               => 'Avansert søk',
'powersearch-legend'        => 'Avansert søk',
'powersearch-ns'            => 'Søk i navnerom:',
'powersearch-redir'         => 'Vis omdirigeringer',
'powersearch-field'         => 'Søk etter',
'search-external'           => 'Eksternt søk',
'searchdisabled'            => 'Søkefunksjonen er slått av. Du kan søke via Google i mellomtiden. Merk at Googles indeksering av {{SITENAME}} muligens er utdatert.',

# Preferences page
'preferences'              => 'Innstillinger',
'mypreferences'            => 'Innstillinger',
'prefs-edits'              => 'Antall redigeringer:',
'prefsnologin'             => 'Ikke logget inn',
'prefsnologintext'         => 'Du må være <span class="plainlinks">[{{fullurl:Special:Userlogin|returnto=$1}} logget inn]</span> for å endre brukerinnstillingene.',
'prefsreset'               => 'Brukerinnstillingene er tilbakestilt.',
'qbsettings'               => 'Brukerinnstillinger for hurtigmeny.',
'qbsettings-none'          => 'Ingen',
'qbsettings-fixedleft'     => 'Fast venstre',
'qbsettings-fixedright'    => 'Fast høyre',
'qbsettings-floatingleft'  => 'Flytende venstre',
'qbsettings-floatingright' => 'Flytende til høyre',
'changepassword'           => 'Endre passord',
'skin'                     => 'Utseende',
'math'                     => 'Matteformler',
'dateformat'               => 'Datoformat',
'datedefault'              => 'Ingen foretrukket',
'datetime'                 => 'Dato og tid',
'math_failure'             => 'Feil i matematikken',
'math_unknown_error'       => 'ukjent feil',
'math_unknown_function'    => 'ukjent funksjon',
'math_lexing_error'        => 'lexerfeil',
'math_syntax_error'        => 'syntaksfeil',
'math_image_error'         => 'PNG-konversjon mislyktes',
'math_bad_tmpdir'          => 'Kan ikke skrive til eller opprette midlertidig mappe',
'math_bad_output'          => 'Kan ikke skrive til eller opprette resultatmappe',
'math_notexvc'             => 'Mangler kjørbar texvc;
se math/README for oppsett.',
'prefs-personal'           => 'Brukerdata',
'prefs-rc'                 => 'Siste endringer',
'prefs-watchlist'          => 'Overvåkningsliste',
'prefs-watchlist-days'     => 'Dager som skal vises i overvåkningslisten:',
'prefs-watchlist-edits'    => 'Antall redigeringer som skal vises i utvidet overvåkningsliste:',
'prefs-misc'               => 'Diverse',
'saveprefs'                => 'Lagre',
'resetprefs'               => 'Tilbakestill ulagrede endringer',
'oldpassword'              => 'Gammelt passord:',
'newpassword'              => 'Nytt passord:',
'retypenew'                => 'Gjenta nytt passord:',
'textboxsize'              => 'Redigering',
'rows'                     => 'Rader:',
'columns'                  => 'Kolonner',
'searchresultshead'        => 'Søking',
'resultsperpage'           => 'Resultater per side:',
'contextlines'             => 'Linjer per resultat',
'contextchars'             => 'Tegn per linje i resultatet',
'stub-threshold'           => 'Grense for <span class="mw-stub-example">stubblenkeformatering</span>:',
'recentchangesdays'        => 'Antall dager som skal vises i siste endringer:',
'recentchangescount'       => 'Antall redigeringer som skal vises i «Siste endringer», historikker og logger.',
'savedprefs'               => 'Innstillingene ble lagret.',
'timezonelegend'           => 'Tidssone',
'timezonetext'             => '¹Tast inn antall timer lokaltid differerer fra tjenertiden (UTC).',
'localtime'                => 'Lokaltid',
'timezoneoffset'           => 'Forskjell',
'servertime'               => 'Tjenerens tid er nå',
'guesstimezone'            => 'Hent tidssone fra nettleseren',
'allowemail'               => 'Tillat andre å sende meg e-post',
'prefs-searchoptions'      => 'Søkealternativ',
'prefs-namespaces'         => 'Navnerom',
'defaultns'                => 'Søk i disse navnerommene som standard:',
'default'                  => 'standard',
'files'                    => 'Filer',

# User rights
'userrights'                     => 'Brukerrettighetskontroll', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'         => 'Ordne brukergrupper',
'userrights-user-editname'       => 'Skriv inn et brukernavn:',
'editusergroup'                  => 'Endre brukergrupper',
'editinguser'                    => "Endrer brukerrettighetene til '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup'       => 'Rediger brukergrupper',
'saveusergroups'                 => 'Lagre brukergrupper',
'userrights-groupsmember'        => 'Medlem av:',
'userrights-groups-help'         => 'Du kan endre hvilke grupper denne brukeren er medlem av.
* En avkrysset boks betyr at brukeren er medlem av gruppen.
* En uavkrysset boks betyr at brukeren ikke er medlem av gruppen.
* En * betyr at du ikke kan fjerne gruppemedlemskapet når du har lagt det til, eller vice versa.',
'userrights-reason'              => 'Endringsgrunn:',
'userrights-no-interwiki'        => 'Du har ikke tillatelse til å endre brukerrettigheter på andre wikier.',
'userrights-nodatabase'          => 'Databasen $1 finnes ikke, eller er ikke lokal.',
'userrights-nologin'             => 'Du må [[Special:UserLogin|logge inn]] med en administratorkonto for å endre brukerrettigheter.',
'userrights-notallowed'          => 'Kontoen din har ikke tillatelse til å endre brukerrettigheter.',
'userrights-changeable-col'      => 'Grupper du kan endre',
'userrights-unchangeable-col'    => 'Grupper du ikke kan endre',
'userrights-irreversible-marker' => '$1 *',

# Groups
'group'               => 'Gruppe:',
'group-user'          => 'Brukere',
'group-autoconfirmed' => 'Autobekreftede brukere',
'group-bot'           => 'Roboter',
'group-sysop'         => 'Administratorer',
'group-bureaucrat'    => 'Byråkrater',
'group-suppress'      => 'Sidefjernere',
'group-all'           => '(alle)',

'group-user-member'          => 'bruker',
'group-autoconfirmed-member' => 'autobekreftet bruker',
'group-bot-member'           => 'robot',
'group-sysop-member'         => 'administrator',
'group-bureaucrat-member'    => 'byråkrat',
'group-suppress-member'      => 'revisjonsfjerner',

'grouppage-user'          => '{{ns:project}}:Brukere',
'grouppage-autoconfirmed' => '{{ns:project}}:Autobekreftede brukere',
'grouppage-bot'           => '{{ns:project}}:Roboter',
'grouppage-sysop'         => '{{ns:project}}:Administratorer',
'grouppage-bureaucrat'    => '{{ns:project}}:Byråkrater',
'grouppage-suppress'      => '{{ns:project}}:Sidefjerning',

# Rights
'right-read'                 => 'Se sider',
'right-edit'                 => 'Redigere sider',
'right-createpage'           => 'Opprette sider (som ikke er diskusjonssider)',
'right-createtalk'           => 'Opprette diskusjonssider',
'right-createaccount'        => 'Opprette nye kontoer',
'right-minoredit'            => 'Marker endringer som mindre',
'right-move'                 => 'Flytte sider',
'right-move-subpages'        => 'Flytte sider med undersider',
'right-suppressredirect'     => 'Behøver ikke å opprette omdirigeringer ved sideflytting',
'right-upload'               => 'Laste opp filer',
'right-reupload'             => 'Skrive over eksisterende filer',
'right-reupload-own'         => 'Skrive over egne filer',
'right-reupload-shared'      => 'Skrive over delte filer lokalt',
'right-upload_by_url'        => 'Laste opp en fil via URL',
'right-purge'                => 'Rense mellomlageret for sider',
'right-autoconfirmed'        => 'Redigere halvlåste sider',
'right-bot'                  => 'Bli behandlet som en automatisk prosess',
'right-nominornewtalk'       => 'Får ikke «Du har nye meldinger»-beskjeden ved mindre endringer på diskusjonsside',
'right-apihighlimits'        => 'Bruke API med høyere grenser',
'right-writeapi'             => 'Redigere via API',
'right-delete'               => 'Slette sider',
'right-bigdelete'            => 'Slette sider med stor historikk',
'right-deleterevision'       => 'Slette og gjenopprette enkeltrevisjoner av sider',
'right-deletedhistory'       => 'Se slettet sidehistorikk uten tilhørende sidetekst',
'right-browsearchive'        => 'Søke i slettede sider',
'right-undelete'             => 'Gjenopprette sider',
'right-suppressrevision'     => 'Se og gjenopprette skjulte siderevisjoner',
'right-suppressionlog'       => 'Se private logger',
'right-block'                => 'Blokkere andre brukere fra å redigere',
'right-blockemail'           => 'Blokkere brukere fra å sende e-post',
'right-hideuser'             => 'Blokkere et brukernavn og skjule det fra det offentlige',
'right-ipblock-exempt'       => 'Kan redigere fra blokkerte IP-adresser',
'right-proxyunbannable'      => 'Kan redigere fra blokkerte proxyer',
'right-protect'              => 'Endre beskyttelsesnivåer',
'right-editprotected'        => 'Redigere beskyttede sider',
'right-editinterface'        => 'Redigere brukergrensesnittet',
'right-editusercssjs'        => 'Redigere andre brukeres CSS- og JS-filer',
'right-rollback'             => 'Raskt tilbakestille den siste brukeren som har redigert en gitt side',
'right-markbotedits'         => 'Markere tilbakestillinger som robotredigeringer',
'right-noratelimit'          => 'Påvirkes ikke av hastighetsgrenser',
'right-import'               => 'Importere sider fra andre wikier',
'right-importupload'         => 'Importere sider via opplasting',
'right-patrol'               => 'Markere redigeringer som patruljerte',
'right-autopatrol'           => 'Får sine egne redigeringer merket som patruljerte',
'right-patrolmarks'          => 'Bruke patruljeringsfunksjoner i siste endringer',
'right-unwatchedpages'       => 'Se listen over uovervåkede sider',
'right-trackback'            => 'Gi tilbakemelding',
'right-mergehistory'         => 'Flette sidehistorikker',
'right-userrights'           => 'Redigere alle brukerrettigheter',
'right-userrights-interwiki' => 'Redigere rettigheter for brukere på andre wikier',
'right-siteadmin'            => 'Låse og låse opp databasen',

# User rights log
'rightslog'      => 'Rettighetslogg',
'rightslogtext'  => 'Dette er en logg over forandringer i brukerrettigheter.',
'rightslogentry' => 'endret gruppe for $1 fra $2 til $3',
'rightsnone'     => '(ingen)',

# Recent changes
'nchanges'                          => '$1 {{PLURAL:$1|endring|endringer}}',
'recentchanges'                     => 'Siste endringer',
'recentchangestext'                 => 'Vis de siste endringene til denne siden',
'recentchanges-feed-description'    => 'Følg med på siste endringer i denne wikien med denne feed-en.',
'rcnote'                            => "Nedenfor vises {{PLURAL:$1|én endring|de siste '''$1''' endringene}} fra {{PLURAL:$2|det siste døgnet|de siste '''$2''' døgnene}} per $5 $4.",
'rcnotefrom'                        => "Nedenfor er endringene fra '''$2''' til '''$1''' vist.",
'rclistfrom'                        => 'Vis nye endringer med start fra $1',
'rcshowhideminor'                   => '$1 mindre endringer',
'rcshowhidebots'                    => '$1 roboter',
'rcshowhideliu'                     => '$1 innloggede brukere',
'rcshowhideanons'                   => '$1 uregistrerte brukere',
'rcshowhidepatr'                    => '$1 godkjente endringer',
'rcshowhidemine'                    => '$1 mine endringer',
'rclinks'                           => 'Vis siste $1 endringer i de siste $2 dagene<br />$3',
'diff'                              => 'diff',
'hist'                              => 'hist',
'hide'                              => 'skjul',
'show'                              => 'vis',
'minoreditletter'                   => 'm',
'newpageletter'                     => 'N',
'boteditletter'                     => 'b',
'number_of_watching_users_pageview' => '[$1 overvåkende {{PLURAL:$1|bruker|brukere}}]',
'rc_categories'                     => 'Begrens til kategorier (skilletegn: «|»)',
'rc_categories_any'                 => 'Alle',
'newsectionsummary'                 => '/* $1 */ ny seksjon',

# Recent changes linked
'recentchangeslinked'          => 'Relaterte endringer',
'recentchangeslinked-title'    => 'Endringer relatert til «$1»',
'recentchangeslinked-noresult' => 'Ingen endringer på lenkede sider i den gitte perioden.',
'recentchangeslinked-summary'  => "Denne spesialsiden lister opp alle de siste endringene som har skjedd på sider som ''lenkes til'' fra denne.
Om den gitte siden er en kategori vises de siste endringene på sidene i kategorien i stedet.
Sider som også er på din [[Special:Watchlist|overvåkningsliste]] vises i '''fet skrift'''.",
'recentchangeslinked-page'     => 'Sidenavn:',
'recentchangeslinked-to'       => 'Vis endringer på sider som lenker til den gitte siden i stedet',

# Upload
'upload'                      => 'Last opp fil',
'uploadbtn'                   => 'Last opp fil',
'reupload'                    => 'Last opp fil igjen',
'reuploaddesc'                => 'Avbryt opplasting og gå tilbake til opplastingsskjemaet',
'uploadnologin'               => 'Ikke logget inn',
'uploadnologintext'           => 'Du må være [[Special:UserLogin|loggett inn]] for å kunne laste opp filer.',
'upload_directory_missing'    => 'Oppplastingsmappen ($1) mangler og kunne ikke opprettes av tjeneren.',
'upload_directory_read_only'  => 'Opplastingsmappa ($1) er ikke skrivbar for tjeneren.',
'uploaderror'                 => 'Feil under opplasting av fil',
'uploadtext'                  => "Bruk skjemaet nedenfor for å laste opp filer.
For å se eller søke i eksisterende filer, gå til [[Special:ImageList|listen over filer]]. Opplastinger lagres også i [[Special:Log/upload|opplastingsloggen]].

For å inkludere en fil på en side, bruk en slik lenke:
*'''<tt><nowiki>[[</nowiki>{{ns:image}}:Filnavn.jpg<nowiki>]]</nowiki></tt>''' for å bruke bildet i opprinnelig form
*'''<tt><nowiki>[[</nowiki>{{ns:image}}:Filnavn.png|200px|thumb|left|Alternativ tekst<nowiki>]]</nowiki></tt>''' for å bruke bildet med en bredde på 200&nbsp;piksler, venstrestilt og med «Alternativ tekst» som beskrivelse
*'''<tt><nowiki>[[</nowiki>{{ns:media}}:Filnavn.ogg<nowiki>]]</nowiki></tt>''' for å lenke direkte til filen uten å vise den",
'upload-permitted'            => 'Tillatte filtyper: $1.',
'upload-preferred'            => 'Foretrukne filtyper: $1',
'upload-prohibited'           => 'Forbudte filtyper: $1.',
'uploadlog'                   => 'opplastingslogg',
'uploadlogpage'               => 'Opplastingslogg',
'uploadlogpagetext'           => 'Her er en liste over de siste opplastede filene.
Se [[Special:NewImages|galleriet over nye filer]] for en mer visuell visning',
'filename'                    => 'Filnavn',
'filedesc'                    => 'Beskrivelse',
'fileuploadsummary'           => 'Beskrivelse:',
'filestatus'                  => 'Opphavsrettsstatus:',
'filesource'                  => 'Kilde:',
'uploadedfiles'               => 'Filer som er lastet opp',
'ignorewarning'               => 'Lagre fila likevel',
'ignorewarnings'              => 'Ignorer eventuelle advarsler',
'minlength1'                  => 'Filnavn må være på minst én bokstav.',
'illegalfilename'             => 'Filnavnet «$1» inneholder ugyldige tegn; gi fila et nytt navn og prøv igjen.',
'badfilename'                 => 'Navnet på filen er blitt endret til «$1».',
'filetype-badmime'            => 'Filer av typen «$1» kan ikke lastes opp.',
'filetype-unwanted-type'      => "'''«.$1»''' er en uønsket filtype.
{{PLURAL:$3|Foretrukken filtype|Foretrukne filtyper}} er $2.",
'filetype-banned-type'        => "'''«$1»''' er ikke en tillatt filtype.
{{PLURAL:$3|Tillatt filtype|Tillatte filtyper}} er $2.",
'filetype-missing'            => 'Filen har ingen endelse (som «.jpg»).',
'large-file'                  => 'Det er anbefalt at filen ikke er større enn $1; denne filen er $2.',
'largefileserver'             => 'Denne fila er større enn det tjeneren er satt opp til å tillate.',
'emptyfile'                   => 'Fila du lastet opp ser ut til å være tom. Dette kan komme av en skrivefeil i filnavnet. Sjekk om du virkelig vil laste opp denne fila.',
'fileexists'                  => 'Ei fil med dette navnet finnes allerede. Sjekk <strong><tt>$1</tt></strong> hvis du ikke er sikker på at du vil forandre den.',
'filepageexists'              => 'Beskrivelsessiden for denne filen finnes allerede på <strong><tt>$1</tt></strong>, men ingen filer med dette navnet finnes. Sammendragen du skriver iknn vil ikke vises på beskrivelsessiden. For at det skal dukke opp der må du skrive det inn manuelt etter å ha lastet opp filen.',
'fileexists-extension'        => 'En fil med et lignende navn finnes:<br />
Navnet på din fil: <strong><tt>$1</tt></strong><br />
Navn på eksisterende fil: <strong><tt>$2</tt></strong><br />
Den eneste forskjellen ligger i store/små bokstaver i filendelsen. Vennligst sjekk filene for likheter.',
'fileexists-thumb'            => "<center>'''Eksisterende fil'''</center>",
'fileexists-thumbnail-yes'    => 'Filen ser ut til å være et bilde av redusert størrelse. Vennligst sjekk filen <strong><tt>$1</tt></strong>.<br />
Om filen du sjekket er det samme bildet, men i opprinnelig størrelse, er det ikke nødvendig å laste opp en ekstra fil.',
'file-thumbnail-no'           => 'Filnavnet begynner med <strong><tt>$1</tt></strong>.
Det virker som om det er et bilde av redusert størrelse <i>(miniatyrbilde)</i>.
Om du har dette bildet i stor utgave, last opp det, eller endre filnavnet på denne filen.',
'fileexists-forbidden'        => 'En fil med dette navnet finnes fra før; gå tilbake og last opp filen under et nytt navn. [[Image:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => 'Ei fil med dette navnet finnes fra før i det delte fillageret.
Om du fortsatt ønsker å laste opp fila, gå tilbake og last den opp under et nytt navn. [[Image:$1|thumb|center|$1]]',
'file-exists-duplicate'       => 'Denne filen er en dublett av følgende {{PLURAL:$1|fil|filer}}:',
'successfulupload'            => 'Opplastingen er gjennomført',
'uploadwarning'               => 'Opplastingsadvarsel',
'savefile'                    => 'Lagre fil',
'uploadedimage'               => 'Lastet opp «[[$1]]»',
'overwroteimage'              => 'last opp en ny versjon av «[[$1]]»',
'uploaddisabled'              => 'Opplastingsfunksjonen er slått av',
'uploaddisabledtext'          => 'Opplasting er slått av på {{SITENAME}}.',
'uploadscripted'              => 'Denne fila inneholder HTML eller skripting som kan feiltolkes av en nettleser.',
'uploadcorrupt'               => 'Denne fila er ødelagt eller er en ugyldig filtype. Sjekk fila og last den opp på nytt.',
'uploadvirus'                 => 'Denne fila inneholder virus! Detaljer: $1',
'sourcefilename'              => 'Velg en fil:',
'destfilename'                => 'Ønsket filnavn:',
'upload-maxfilesize'          => 'Maksimal filstørrelse: $1',
'watchthisupload'             => 'Overvåk denne siden',
'filewasdeleted'              => 'Ei fil ved dette navnet har blitt lastet opp tidligere, og så slettet. Sjekk $1 før du forsøker å laste det opp igjen.',
'upload-wasdeleted'           => "'''Advarsel: Du laster opp en fil som tidligere har blitt slettet.'''

Vurder om det er riktig å fortsette å laste opp denne filen. Slettingsloggen for filen gis nedenunder:",
'filename-bad-prefix'         => 'Navnet på filen du laster opp begynner med <strong>«$1»</strong>, hvilket er et ikke-beksrivende navn som vanligvis brukes automatisk av digitalkameraer. Vennligst bruk et mer beskrivende navn på filen.',
'filename-prefix-blacklist'   => ' #<!-- leave this line exactly as it is --> <pre>
# Syntaksen er som følger:
#   * Alt fra tegnet «#» til slutten av linja er en kommentar
#   * Alle linjer som ikke er blanke er et prefiks som vanligvis brukes automatisk av digitale kameraer
CIMG # Casio
DSC_ # Nikon
DSCF # Fuji
DSCN # Nikon
DUW # noen mobiltelefontyper
IMG # generisk
JD # Jenoptik
MGP # Pentax
PICT # div.
 #</pre> <!-- leave this line exactly as it is -->',

'upload-proto-error'      => 'Gal protokoll',
'upload-proto-error-text' => 'Fjernopplasting behøver adresser som begynner med <code>http://</code> eller <code>ftp://</code>.',
'upload-file-error'       => 'Intern feil',
'upload-file-error-text'  => 'En intern feil oppsto under forsøk på å lage en midlertidig fil på tjeneren. Vennligst kontakt en [[Special:ListUsers/sysop|administrator]].',
'upload-misc-error'       => 'Ukjent opplastingsfeil',
'upload-misc-error-text'  => 'En ukjent feil forekom under opplastingen.
Bekreft at adressen er gyldig og tilgjengelig, og prøv igjen.
Om problemet fortsetter, kontakt en [[Special:ListUsers/sysop|administrator]].',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => 'Kunne ikke nå adressen',
'upload-curl-error6-text'  => 'Adressen kunne ikke nås. Vennligst dobbelsjekk at adressen er korrekt og at siden er oppe.',
'upload-curl-error28'      => 'Opplastingstimeout',
'upload-curl-error28-text' => 'Siden brukte for lang tid på å reagere. Vennligst sjekk at siden er oppe, og vent en kort stund for du prøver igjen. Vurder å prøve på en mindre hektisk tid.',

'license'            => 'Lisens:',
'nolicense'          => 'Ingen spesifisert',
'license-nopreview'  => '(Forhåndsvisning ikke tilgjengelig)',
'upload_source_url'  => ' (en gyldig, offentlig tilgjengelig adresse)',
'upload_source_file' => ' (en fil på din datamaskin)',

# Special:ImageList
'imagelist-summary'     => 'Denne spesialsiden viser alle opplastede filer. De sist opplastede filene vises på toppen som standard. Klikk på en kolonneoverskrift for å endre sorteringsmetode.',
'imagelist_search_for'  => 'Søk etter filnavn:',
'imgfile'               => 'fil',
'imagelist'             => 'Filliste',
'imagelist_date'        => 'Dato',
'imagelist_name'        => 'Navn',
'imagelist_user'        => 'Bruker',
'imagelist_size'        => 'Størrelse (bytes)',
'imagelist_description' => 'Beskrivelse',

# Image description page
'filehist'                       => 'Filhistorikk',
'filehist-help'                  => 'Klikk på en dato/klokkeslett for å se filen slik den var på den tiden.',
'filehist-deleteall'             => 'slett alt',
'filehist-deleteone'             => 'slett',
'filehist-revert'                => 'tilbakestill',
'filehist-current'               => 'nåværende',
'filehist-datetime'              => 'Dato/tid',
'filehist-user'                  => 'Bruker',
'filehist-dimensions'            => 'Dimensjoner',
'filehist-filesize'              => 'Filstørrelse',
'filehist-comment'               => 'Kommentar',
'imagelinks'                     => 'Lenker',
'linkstoimage'                   => 'Følgende {{PLURAL:$1|side|$1 sider}} har lenker til denne fila:',
'nolinkstoimage'                 => 'Det er ingen sider som bruker denne fila.',
'morelinkstoimage'               => 'Vis [[Special:WhatLinksHere/$1|flere lenker]] til denne filen.',
'redirectstofile'                => 'Følgende {{PLURAL:$1|fil er en omdirigering|filer er omdirigeringer}} til denne filen:',
'duplicatesoffile'               => 'Følgende {{PLURAL:$1|fil er en dublett|filer er dubletter}} av denne filen:',
'sharedupload'                   => 'Denne filen er en delt opplasting og kan brukes av andre prosjekter.',
'shareduploadwiki'               => 'Se $1 for mer informasjon.',
'shareduploadwiki-desc'          => 'Beskrivelsen som vist på dens $1 vises nedenfor.',
'shareduploadwiki-linktext'      => 'filbeskrivelsesside',
'shareduploadduplicate'          => 'Denne filen er en duplikat av $1 fra delt lagringsområde.',
'shareduploadduplicate-linktext' => 'en annen fil',
'shareduploadconflict'           => 'Denne filen har samme navn som $1 fra det delte lagringsområdet.',
'shareduploadconflict-linktext'  => 'en annen fil',
'noimage'                        => 'Ingen fil ved dette navnet finnes, men du kan $1.',
'noimage-linktext'               => 'laste opp ett',
'uploadnewversion-linktext'      => 'Last opp en ny versjon av denne fila',
'imagepage-searchdupe'           => 'Søk etter duplikatfiler',

# File reversion
'filerevert'                => 'Tilbakestill $1',
'filerevert-legend'         => 'Tilbakestill fil',
'filerevert-intro'          => "Du tilbakestiller '''[[Media:$1|$1]]''' til [$4 versjonen à $2, $3].",
'filerevert-comment'        => 'Kommentar:',
'filerevert-defaultcomment' => 'Tilbakestilte til versjonen à $1, $2',
'filerevert-submit'         => 'Tilbakestill',
'filerevert-success'        => "'''[[Media:$1|$1]]''' ble tilbakestilt til [$4 versjonen à $2, $3].",
'filerevert-badversion'     => 'Det er ingen tidligere lokal versjon av denne filen med det gitte tidstrykket.',

# File deletion
'filedelete'                  => 'Slett $1',
'filedelete-legend'           => 'Slett fil',
'filedelete-intro'            => "Du sletter '''[[Media:$1|$1]]'''.",
'filedelete-intro-old'        => "Du sletter versjonen av '''[[Media:$1|$1]]''' à [$4 $3, $2].",
'filedelete-comment'          => 'Slettingsårsak:',
'filedelete-submit'           => 'Slett',
'filedelete-success'          => "'''$1''' ble slettet.",
'filedelete-success-old'      => "Versjonen av '''[[Media:$1|$1]]''' à $3, $2 ble slettet.",
'filedelete-nofile'           => "'''$1''' finnes ikke.",
'filedelete-nofile-old'       => "Det er ingen arkivert versjon av '''$1''' med de gitte attributtene.",
'filedelete-iscurrent'        => 'Du forsøker å slette den nyeste versjonen av denne filen. Vennligst tilbakestill til en eldre versjon først.',
'filedelete-otherreason'      => 'Annen/utdypende grunn:',
'filedelete-reason-otherlist' => 'Annen grunn',
'filedelete-reason-dropdown'  => '*Vanlige slettingsgrunner
** Opphavsrettsbrudd
** Duplikatfil',
'filedelete-edit-reasonlist'  => 'Rediger begrunnelser for sletting',

# MIME search
'mimesearch'         => 'MIME-søk',
'mimesearch-summary' => 'Denne siden muliggjør filtrering av filer per MIME-type. Skriv inn: innholdstype/undertype, for eksempel <tt>image/jpeg</tt>.',
'mimetype'           => 'MIME-type:',
'download'           => 'last ned',

# Unwatched pages
'unwatchedpages' => 'Sider som ikke er overvåket',

# List redirects
'listredirects' => 'Liste over omdirigeringer',

# Unused templates
'unusedtemplates'     => 'Ubrukte maler',
'unusedtemplatestext' => 'Denne siden lister opp alle sider i malnavnerommet som ikke er inkludert på en annen side. Husk å sjekke for andre slags lenker til malen før du sletter den.',
'unusedtemplateswlh'  => 'andre lenker',

# Random page
'randompage'         => 'Tilfeldig side',
'randompage-nopages' => 'Det er ingen sider i dette navnerommet.',

# Random redirect
'randomredirect'         => 'Tilfeldig omdirigering',
'randomredirect-nopages' => 'Det er ingen omdirigeringer i dette navnerommet.',

# Statistics
'statistics'             => 'Statistikk',
'sitestats'              => '{{SITENAME}}-statistikk',
'userstats'              => 'Brukerstatistikk',
'sitestatstext'          => "Det er til sammen {{PLURAL:$1|'''én''' side|'''$1''' sider}} i databasen. Dette inkluderer diskusjonssider, sider om {{SITENAME}}, små stubbsider, omdirigeringer, og annet som antagligvis ikke gjelder som ordentlig innhold. Om man ikke regner med disse, er det {{PLURAL:$2|'''én''' side|'''$2''' sider}} som sannsynligvis er {{PLURAL:$2|en ordentlig innholdsside|ordentlige innholdssider}}.

{{PLURAL:$8|'''Én''' fil|'''$8''' filer}} har blitt lastet opp.

Det har vært totalt {{PLURAL:$3|'''én''' sidevisning|'''$3''' sidevisninger}}, og {{PLURAL:$4|'''én''' redigering|'''$4''' redigeringer}} siden wikien ble satt opp. Det blir i snitt {{PLURAL:$5|'''én''' redigering|'''$5''' redigeringer}} per side, og {{PLURAL:$6|'''én''' visning|'''$6''' visninger}} per redigering.

[http://www.mediawiki.org/wiki/Manual:Job_queue Arbeidskøen] er på '''$7'''.",
'userstatstext'          => "Det er {{PLURAL:$1|'''én''' registrert bruker|'''$1''' registrerte brukere}}, hvorav '''$2''' (eller '''$4&nbsp;%''') har {{lc:$5rettigheter}}.",
'statistics-mostpopular' => 'Mest viste sider',

'disambiguations'      => 'Artikler med flertydige titler',
'disambiguationspage'  => 'Template:Peker',
'disambiguations-text' => "Følgende sider lenker til en '''pekerside'''.
De burde i stedet lenke til en passende innholdsside.<br />
En side anses om en pekerside om den inneholder en mal som det lenkes til fra [[MediaWiki:Disambiguationspage]]",

'doubleredirects'            => 'Doble omdirigeringer',
'doubleredirectstext'        => "'''NB:''' Denne listen kan inneholde gale resultater. Det er som regel fordi siden inneholder ekstra tekst under den første <tt>#redirect</tt>.<br />Hver linje inneholder lenker til den første og den andre omdirigeringen, og den første linjen fra den andre omdirigeringsteksten. Det gir som regel den «riktige» målartikkelen, som den første omdirigeringen skulle ha pekt på.",
'double-redirect-fixed-move' => '[[$1]] har blitt flyttet, og er nå en omdirigering til [[$2]]',
'double-redirect-fixer'      => 'Omdirigeringsfikser',

'brokenredirects'        => 'Brutte omdirigeringer',
'brokenredirectstext'    => 'Følgende omdirigeringer peker til ikkeeksisterende sider.',
'brokenredirects-edit'   => '(rediger)',
'brokenredirects-delete' => '(slett)',

'withoutinterwiki'         => 'Sider uten lenker til andre språk',
'withoutinterwiki-summary' => 'Følgende sider lenker ikke til andre språkversjoner:',
'withoutinterwiki-legend'  => 'Prefiks',
'withoutinterwiki-submit'  => 'Vis',

'fewestrevisions' => 'Artikler med færrest revisjoner',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|byte|bytes}}',
'ncategories'             => '$1 {{PLURAL:$1|kategori|kategorier}}',
'nlinks'                  => '$1 {{PLURAL:$1|lenke|lenker}}',
'nmembers'                => '$1 {{PLURAL:$1|medlem|medlemmer}}',
'nrevisions'              => '$1 {{PLURAL:$1|revisjon|revisjoner}}',
'nviews'                  => '$1 {{PLURAL:$1|visning|visninger}}',
'specialpage-empty'       => 'Denne siden er tom.',
'lonelypages'             => 'Foreldreløse sider',
'lonelypagestext'         => 'Følgende sider blir ikke lenket til fra andre sider på {{SITENAME}}.',
'uncategorizedpages'      => 'Ukategoriserte sider',
'uncategorizedcategories' => 'Ukategoriserte kategorier',
'uncategorizedimages'     => 'Ukategoriserte filer',
'uncategorizedtemplates'  => 'Ukategoriserte maler',
'unusedcategories'        => 'Ubrukte kategorier',
'unusedimages'            => 'Ubrukte filer',
'popularpages'            => 'Populære sider',
'wantedcategories'        => 'Ønskede kategorier',
'wantedpages'             => 'Etterspurte sider',
'missingfiles'            => 'Manglende filer',
'mostlinked'              => 'Sider med flest lenker til seg',
'mostlinkedcategories'    => 'Kategorier med flest sider',
'mostlinkedtemplates'     => 'Mest brukte maler',
'mostcategories'          => 'Sider med flest kategorier',
'mostimages'              => 'Mest brukte filer',
'mostrevisions'           => 'Artikler med flest revisjoner',
'prefixindex'             => 'Prefiksindeks',
'shortpages'              => 'Korte sider',
'longpages'               => 'Lange sider',
'deadendpages'            => 'Blindveisider',
'deadendpagestext'        => 'Følgende sider lenker ikke til andre sider på {{SITENAME}}.',
'protectedpages'          => 'Låste sider',
'protectedpages-indef'    => 'Kun beskyttelser på ubestemt tid',
'protectedpagestext'      => 'Følgende sider er låst for flytting eller redigering',
'protectedpagesempty'     => 'Ingen sider er for øyeblikket låst med disse paramterne.',
'protectedtitles'         => 'Beskyttede titler',
'protectedtitlestext'     => 'Følgende titler er beskyttet fra opprettelse',
'protectedtitlesempty'    => 'Ingen titler beskyttes med disse parameterne for øyeblikket.',
'listusers'               => 'Brukerliste',
'newpages'                => 'Nye sider',
'newpages-username'       => 'Brukernavn:',
'ancientpages'            => 'Eldste sider',
'move'                    => 'Flytt',
'movethispage'            => 'Flytt denne siden',
'unusedimagestext'        => 'Merk at andre sider kanskje lenker til en fil med en direkte lenke, så filen listes her selv om den faktisk er i bruk.',
'unusedcategoriestext'    => 'Følgende kategorier finnes, men det er ingen sider i dem.',
'notargettitle'           => 'Intet mål',
'notargettext'            => 'Du oppga ikke en målside eller bruker å utføre denne funksjonen på.',
'nopagetitle'             => 'Målsiden finnes ikke',
'nopagetext'              => 'Siden du ville flytte finnes ikke.',
'pager-newer-n'           => '{{PLURAL:$1|1 nyere|$1 nyere}}',
'pager-older-n'           => '{{PLURAL:$1|1 eldre|$1 eldre}}',
'suppress'                => 'Sidefjerning',

# Book sources
'booksources'               => 'Bokkilder',
'booksources-search-legend' => 'Søk etter bokkilder',
'booksources-go'            => 'Gå',
'booksources-text'          => 'Under er en liste over lenker til andre sider som selger nye og brukte bøker, og kan også ha videre informasjon om bøker du leter etter:',

# Special:Log
'specialloguserlabel'  => 'Bruker:',
'speciallogtitlelabel' => 'Tittel:',
'log'                  => 'Logger',
'all-logs-page'        => 'Alle logger',
'log-search-legend'    => 'Søk i loggene.',
'log-search-submit'    => 'Gå',
'alllogstext'          => 'Kombinert visning av alle loggene på {{SITENAME}}.
Du kan minske antallet resultater ved å velge loggtype, brukernavn eller den siden som er påvirket (husk å skille mellom store og små boktaver).',
'logempty'             => 'Ingen elementer i loggen.',
'log-title-wildcard'   => 'Søk i titler som starter med denne teksten',

# Special:AllPages
'allpages'          => 'Alle sider',
'alphaindexline'    => '$1 til $2',
'nextpage'          => 'Neste side ($1)',
'prevpage'          => 'Forrige side ($1)',
'allpagesfrom'      => 'Vis sider fra og med:',
'allarticles'       => 'Alle sider',
'allinnamespace'    => 'Alle sider i $1-navnerommet',
'allnotinnamespace' => 'Alle sider (ikke i $1-navnerommet)',
'allpagesprev'      => 'Forrige',
'allpagesnext'      => 'Neste',
'allpagessubmit'    => 'Gå',
'allpagesprefix'    => 'Vis sider med prefikset:',
'allpagesbadtitle'  => 'Den angitte sidetittelen var ugyldig eller hadde et interwiki-prefiks. Den kan inneholde ett eller flere tegn som ikke kan brukes i titler.',
'allpages-bad-ns'   => '{{SITENAME}} har ikke navnerommet «$1».',

# Special:Categories
'categories'                    => 'Kategorier',
'categoriespagetext'            => 'Følgende kategorier inneholder sider eller media.
[[Special:UnusedCategories|Ubrukte kategorier]] vises ikke her.
Se også [[Special:WantedCategories|ønskede kategorier]].',
'categoriesfrom'                => 'Vis kategorier fra og med:',
'special-categories-sort-count' => 'soter etter antall',
'special-categories-sort-abc'   => 'sorter alfabetisk',

# Special:ListUsers
'listusersfrom'      => 'Vis brukere fra og med:',
'listusers-submit'   => 'Vis',
'listusers-noresult' => 'Ingen bruker funnet.',

# Special:ListGroupRights
'listgrouprights'          => 'Rettigheter for brukergrupper',
'listgrouprights-summary'  => 'Følgende er en liste over brukergrupper som er definert på denne wikien, og hvilke rettigheter de har.
Mer informasjon om de enkelte rettighetstypene kan finnes [[{{MediaWiki:Listgrouprights-helppage}}|her]].',
'listgrouprights-group'    => 'Gruppe',
'listgrouprights-rights'   => 'Rettigheter',
'listgrouprights-helppage' => 'Help:Grupperettigheter',
'listgrouprights-members'  => '(liste over medlemmer)',

# E-mail user
'mailnologin'     => 'Ingen avsenderadresse',
'mailnologintext' => 'Du må være [[Special:UserLogin|logget inn]] og ha en gyldig e-postadresse satt i [[Special:Preferences|brukerinnstillingene]] for å sende e-post til andre brukere.',
'emailuser'       => 'E-post til denne brukeren',
'emailpage'       => 'E-post til bruker',
'emailpagetext'   => 'Hvis denne brukeren har oppgitt en gyldig e-postadresse i sine innstillinger, vil dette skjemaet sende én beskjed.
Den e-postadressen du har satt i [[Special:Preferences|innstillingene dine]] vil dukke opp i «fra»-feltet på denne e-posten, så mottakeren er i stand til å svare.',
'usermailererror' => 'E-postobjekt returnerte feilen:',
'defemailsubject' => 'E-post fra {{SITENAME}}',
'noemailtitle'    => 'Ingen e-postadresse',
'noemailtext'     => 'Dene brukeren har ikke oppgitt en gyldig e-postadresse, eller har valgt å ikke motta e-post fra andre brukere.',
'emailfrom'       => 'Fra:',
'emailto'         => 'Til:',
'emailsubject'    => 'Emne:',
'emailmessage'    => 'Beskjed:',
'emailsend'       => 'Send',
'emailccme'       => 'Send meg en kopi av beskjeden min.',
'emailccsubject'  => 'Kopi av din beskjed til $1: $2',
'emailsent'       => 'E-post sendt',
'emailsenttext'   => 'E-postbeskjeden er sendt',
'emailuserfooter' => 'E-posten ble sendt av $1 til $2 via «Send e-post»-funksjonen på {{SITENAME}}.',

# Watchlist
'watchlist'            => 'Overvåkningsliste',
'mywatchlist'          => 'Overvåkningsliste',
'watchlistfor'         => "(for '''$1''')",
'nowatchlist'          => 'Du har ingenting i overvåkningslisten.',
'watchlistanontext'    => 'Vennligst $1 for å vise eller redigere sider på overvåkningslisten din.',
'watchnologin'         => 'Ikke logget inn',
'watchnologintext'     => 'Du må være [[Special:UserLogin|logget inn]] for å kunne endre overvåkningslisten.',
'addedwatch'           => 'Lagt til overvåkningslisten.',
'addedwatchtext'       => "Siden «[[:$1]]» er lagt til [[Special:Watchlist|overvåkningslisten]]. Fremtidige endringer til denne siden og den tilhørende diskusjonssiden blir listet opp her, og siden vil fremstå '''uthevet''' i [[Special:RecentChanges|listen over de siste endringene]] for å gjøre det lettere å finne den.

Hvis du senere vil fjerne siden fra overvåkningslisten, klikk «Avslutt overvåkning» på den aktuelle siden.",
'removedwatch'         => 'Fjernet fra overvåkningslisten',
'removedwatchtext'     => 'Siden «[[:$1]]» er fjernet fra [[Special:Watchlist|overvåkningslisten din]].',
'watch'                => 'Overvåk',
'watchthispage'        => 'Overvåk denne siden',
'unwatch'              => 'Avslutt overvåkning',
'unwatchthispage'      => 'Fjerner overvåkning',
'notanarticle'         => 'Ikke en artikkel',
'notvisiblerev'        => 'Revisjonen er slettet',
'watchnochange'        => 'Ingen av sidene i overvåkningslisten er endret i den valgte perioden.',
'watchlist-details'    => '{{PLURAL:$1|Én side|$1 sider}} overvåket, utenom diskusjonssider.',
'wlheader-enotif'      => '* E-postnotifikasjon er slått på.',
'wlheader-showupdated' => "* Sider som har blitt forandret siden du sist besøkte dem vises i '''fet tekst'''",
'watchmethod-recent'   => 'sjekker siste endringer for sider i overvåkningslisten',
'watchmethod-list'     => 'sjekker siste endringer for sider i overvåkningslisten',
'watchlistcontains'    => 'Overvåkningslisten inneholder $1 {{PLURAL:$1|side|sider}}.',
'iteminvalidname'      => 'Problem med «$1», ugyldig navn&nbsp;…',
'wlnote'               => "Nedenfor er {{PLURAL:$1|den siste endringen|de siste $1 endringene}} {{PLURAL:$2|den siste timen|de siste '''$2''' timene}}.",
'wlshowlast'           => 'Vis siste $1 timer $2 dager $3',
'watchlist-show-bots'  => 'Vis robotredigeringer',
'watchlist-hide-bots'  => 'Skjul robotredigeringer',
'watchlist-show-own'   => 'Vis mine redigeringer',
'watchlist-hide-own'   => 'Skjul mine redigeringer',
'watchlist-show-minor' => 'Vis mindre redigeringer',
'watchlist-hide-minor' => 'Skjul mindre redigeringer',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Overvåker…',
'unwatching' => 'Fjerner fra overvåkningsliste…',

'enotif_mailer'                => '{{SITENAME}} påminnelsessystem',
'enotif_reset'                 => 'Merk alle sider som besøkt',
'enotif_newpagetext'           => 'Dette er en ny side.',
'enotif_impersonal_salutation' => '{{SITENAME}}-bruker',
'changed'                      => 'endret',
'created'                      => 'opprettet',
'enotif_subject'               => '{{SITENAME}}-siden $PAGETITLE har blitt $CHANGEDORCREATED av $PAGEEDITOR',
'enotif_lastvisited'           => 'Se $1 for alle endringer siden ditt forrige besøk.',
'enotif_lastdiff'              => 'Se $1 for å se denne endringen.',
'enotif_anon_editor'           => 'anonym bruker $1',
'enotif_body'                  => '$WATCHINGUSERNAME,

{{SITENAME}}-siden $PAGETITLE har blitt $CHANGEDORCREATED $PAGEEDITDATE av $PAGEEDITOR, se $PAGETITLE_URL for den nåværende versjonen.

$NEWPAGE

Redigeringssammendrag: $PAGESUMMARY $PAGEMINOREDIT

Kontakt brukeren:
e-post: $PAGEEDITOR_EMAIL
wiki: $PAGEEDITOR_WIKI

Det vil ikke komme flere påminnelser om endringer på denne siden med mindre du besøker den. Du kan også fjerne påminnelsesflagg for alle sider i overvåkningslisten din.

Med vennlig hilsen,
{{SITENAME}}s påminnelsessystem

--
For å endre innstillingene i overvåkningslisten din, besøk {{fullurl:Special:Watchlist/edit}}

Tilbakemeldinger og videre assistanse:
{{fullurl:Project:Hjelp}}',

# Delete/protect/revert
'deletepage'                  => 'Slett side',
'confirm'                     => 'Bekreft',
'excontent'                   => 'Innholdet var: «$1»',
'excontentauthor'             => 'innholdet var «$1» (og eneste bidragsyter var [[Special:Contributions/$2|$2]])',
'exbeforeblank'               => 'innholdet før siden ble tømt var: «$1»',
'exblank'                     => 'siden var tom',
'delete-confirm'              => 'Slett «$1»',
'delete-legend'               => 'Slett',
'historywarning'              => 'Advarsel: Siden du er i ferd med å slette har en historikk:',
'confirmdeletetext'           => 'Du holder på å slette en side eller et bilde sammen med historikken. Bilder som slettes kan ikke gjenopprettes, men alle andre sider som slettes på denne måten kan gjenopprettes. Bekreft at du virkelig vil slette denne siden, og at du gjør det i samsvar med [[{{MediaWiki:Policy-url}}|retningslinjene]].',
'actioncomplete'              => 'Gjennomført',
'deletedtext'                 => '«<nowiki>$1</nowiki>» er slettet. Se $2 for en oversikt over de siste slettingene.',
'deletedarticle'              => 'slettet «[[$1]]»',
'suppressedarticle'           => 'fjernet «[[$1]]»',
'dellogpage'                  => 'Slettingslogg',
'dellogpagetext'              => 'Under er ei liste over nylige slettinger.',
'deletionlog'                 => 'slettingslogg',
'reverted'                    => 'Gjenopprettet en tidligere versjon',
'deletecomment'               => 'Slettingsårsak:',
'deleteotherreason'           => 'Annen/utdypende grunn:',
'deletereasonotherlist'       => 'Annen grunn',
'deletereason-dropdown'       => '* Vanlige grunner for sletting
** På forfatters forespørsel
** Opphavsrettsbrudd
** Vandalisme',
'delete-edit-reasonlist'      => 'Rediger begrunnelser for sletting',
'delete-toobig'               => 'Denne siden har en stor redigeringshistorikk, med over {{PLURAL:$1|$1&nbsp;revisjon|$1&nbsp;revisjoner}}. Muligheten til å slette slike sider er begrenset for å unngå utilsiktet forstyrring av {{SITENAME}}.',
'delete-warning-toobig'       => 'Denne siden har en stor redigeringshistorikk, med over {{PLURAL:$1|$1&nbsp;revisjon|$1&nbsp;revisjoner}}. Sletting av denne siden kan forstyrre databasen til {{SITENAME}}; vær varsom.',
'rollback'                    => 'Fjern redigeringer',
'rollback_short'              => 'Tilbakestill',
'rollbacklink'                => 'tilbakestill',
'rollbackfailed'              => 'Kunne ikke tilbakestille',
'cantrollback'                => 'Kan ikke fjerne redigering; den siste brukeren er den eneste forfatteren.',
'alreadyrolled'               => 'Kan ikke fjerne den siste redigeringen på [[$1]] av [[User:$2|$2]] ([[User talk:$2|diskusjon]] | [[Special:Contributions/$2|{{int:contribslink}}]]); en annen har allerede redigert siden eller fjernet redigeringen.

Den siste redigeringen ble foretatt av [[User:$3|$3]] ([[User talk:$3|diskusjon]] | [[Special:Contributions/$3|{{int:contribslink}}]]).',
'editcomment'                 => "Redigeringskommentaren var: «''$1''»", # only shown if there is an edit comment
'revertpage'                  => 'Tilbakestilte endring av [[Special:Contributions/$2|$2]] ([[User talk:$2|diskusjon]]) til siste versjon av [[User:$1|$1]]', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => 'Tilbakestilte endringer av $1; endret til siste versjon av $2.',
'sessionfailure'              => "Det ser ut til å være et problem med innloggingen din, og den ble avbrutt av sikkerhetshensyn. Trykk ''Tilbake'' i nettleseren din, oppdater siden og prøv igjen.",
'protectlogpage'              => 'Låsingslogg',
'protectlogtext'              => 'Her er en liste over sider som er blitt beskyttet eller har fått fjernet beskyttelsen. Se [[Special:ProtectedPages|listen over låste sider]] for en liste over nåværende låste sider.',
'protectedarticle'            => 'låste [[$1]]',
'modifiedarticleprotection'   => 'endret beskyttelsesnivå for «[[$1]]»',
'unprotectedarticle'          => 'åpnet [[$1]]',
'protect-title'               => 'Låser «$1»',
'protect-legend'              => 'Bekreft låsing',
'protectcomment'              => 'Kommentar:',
'protectexpiry'               => 'Utgår:',
'protect_expiry_invalid'      => 'Utgangstiden er ugyldig.',
'protect_expiry_old'          => 'Utgangstiden har allerede vært.',
'protect-unchain'             => 'Spesielle flyttingstillatelser',
'protect-text'                => 'Du kan se og forandre beskyttelsesnivået for siden <strong><nowiki>$1</nowiki></strong> her.',
'protect-locked-blocked'      => 'Du kan ikke endre beskyttelsesnivåer mens du er blokkert. Dette er de nåværende innstillingene for siden <strong>$1</strong>:',
'protect-locked-dblock'       => 'Beskyttelsesnivåer kan ikke endres under en aktiv databasebeskyttelse. Dette er de nåværende innstillingene for siden <strong>$1</strong>:',
'protect-locked-access'       => 'Kontoen din har ikke tillatelse til å endre beskyttelsesnivåer. Dette er de nåværende innstillingene for siden <strong>$1</strong>:',
'protect-cascadeon'           => 'Denne siden er låst fordi den er inkludert på følgende {{PLURAL:$1|side|sider}} som har dypbeskyttelse slått på. Du kan endre sidens låsingsnivå, men det vil ikke påvirke dypbeskyttelsen.',
'protect-default'             => '(standard)',
'protect-fallback'            => 'Må ha «$1»-tillatelse',
'protect-level-autoconfirmed' => 'Blokker uregistrerte brukere',
'protect-level-sysop'         => 'Kun administratorer',
'protect-summary-cascade'     => 'dypbeskyttelse',
'protect-expiring'            => 'utgår $1 (UTC)',
'protect-cascade'             => 'Dypbeskyttelse – beskytter alle sider som er inkludert på denne siden.',
'protect-cantedit'            => 'Du kan ikke endre beskyttelsesnivået til denne siden fordi du ikke har tillatelse til å redigere den.',
'restriction-type'            => 'Tillatelse',
'restriction-level'           => 'Restriksjonsnivå',
'minimum-size'                => 'Minimumstørrelse',
'maximum-size'                => 'Maksimumstørrelse:',
'pagesize'                    => '(byte)',

# Restrictions (nouns)
'restriction-edit'   => 'Redigering',
'restriction-move'   => 'Flytting',
'restriction-create' => 'Opprett',
'restriction-upload' => 'Last opp',

# Restriction levels
'restriction-level-sysop'         => 'fullstendig låst',
'restriction-level-autoconfirmed' => 'halvlåst',
'restriction-level-all'           => 'alle nivåer',

# Undelete
'undelete'                     => 'Vis slettede sider',
'undeletepage'                 => 'Se og gjenopprett slettede sider',
'undeletepagetitle'            => "'''Følgende innhold er slettede revisjoner av [[:$1]].'''",
'viewdeletedpage'              => 'Vis slettede sider',
'undeletepagetext'             => 'Følgende sider er slettet, men finnes fortsatt i arkivet og kan gjenopprettes. Arkivet blir periodevis slettet.',
'undelete-fieldset-title'      => 'Gjenopprett revisjoner',
'undeleteextrahelp'            => "For å gjenopprette hele siden, la alle boksene være som de er, og klikk '''''Gjenopprett'''''.
For å gjenopprette kun deler, kryss av revisjonenes bokser, og klikk '''''Gjenopprett'''''.
Å klikke '''''Nullstill''''' vil føre til at alle tekstfelt og bokser gjøres blanke.",
'undeleterevisions'            => '{{PLURAL:$1|Én versjon arkivert|$1 versjoner arkiverte}}',
'undeletehistory'              => 'Om du gjenoppretter siden vil alle revisjoner gjenopprettes i historikken.
Dersom en ny side ved samme navn har blitt oprettet etter slettingen, vil de gjenopprettede revisjonene dukke opp før denne i redigeringshistorikken.',
'undeleterevdel'               => 'Gjenoppretting kan ikke utføres dersom det resulterer i at den øverste revisjonen blir delvis slettet. I slike tilfeller må du fjerne merkingen av den nyeste slettede revisjonen.',
'undeletehistorynoadmin'       => 'Denne artikkelen har blitt slettet. Grunnen for slettingen vises i oppsummeringen nedenfor, sammen med detaljer om brukerne som redigerte siden før den ble slettet. Teksten i disse slettede revisjonene er kun tilgjengelig for administratorer.',
'undelete-revision'            => 'Slettet revisjon av $1 av $3 (fra $2):',
'undeleterevision-missing'     => 'Ugyldig eller manglende revisjon. Du kan ha en ødelagt lenke, eller revisjonen har blitt fjernet fra arkivet.',
'undelete-nodiff'              => 'Ingen tidligere revisjoner funnet.',
'undeletebtn'                  => 'Gjenopprett',
'undeletelink'                 => 'gjenopprett',
'undeletereset'                => 'Nullstill',
'undeletecomment'              => 'Kommentar:',
'undeletedarticle'             => 'gjenopprettet «[[$1]]»',
'undeletedrevisions'           => '{{PLURAL:$1|Én revisjon|$1 revisjoner}} gjenopprettet',
'undeletedrevisions-files'     => '{{PLURAL:$1|Én revisjon|$1 revisjoner}} og {{PLURAL:$2|én fil|$2 filer}} gjenopprettet',
'undeletedfiles'               => '{{PLURAL:$1|Én fil|$1 filer}} gjenopprettet',
'cannotundelete'               => 'Kunne ikke gjenopprette siden (den kan være gjenopprettet av noen andre).',
'undeletedpage'                => "<big>'''$1 ble gjenopprettet'''</big>

Sjekk [[Special:Log/delete|slettingsloggen]] for en liste over nylige slettinger og gjenopprettelser.",
'undelete-header'              => 'Se [[Special:Log/delete|slettingsloggen]] for nylig slettede sider.',
'undelete-search-box'          => 'Søk i slettede sider',
'undelete-search-prefix'       => 'Vis sider som starter med:',
'undelete-search-submit'       => 'Søk',
'undelete-no-results'          => 'Ingen passende sider funnet i slettingsarkivet.',
'undelete-filename-mismatch'   => 'Kan ikke gjenopprette filrevisjon med tidstrykk $1: ikke samsvarende filnavn',
'undelete-bad-store-key'       => 'Kan ikke gjenopprette filrevisjon med tidstrykk $1: fil manglet før sletting',
'undelete-cleanup-error'       => 'Feil i sletting av ubrukt arkivfil «$1».',
'undelete-missing-filearchive' => 'Klarte ikke å gjenopprette filarkivet med ID $1 fordi det ikke er i databasen. Det kan ha blitt gjenopprettet tidligere.',
'undelete-error-short'         => 'Feil under filgjenoppretting: $1',
'undelete-error-long'          => 'Feil oppsto under filgjenoppretting:

$1',

# Namespace form on various pages
'namespace'      => 'Navnerom:',
'invert'         => 'Inverter',
'blanknamespace' => '(Hoved)',

# Contributions
'contributions' => 'Bidrag',
'mycontris'     => 'Egne bidrag',
'contribsub2'   => 'For $1 ($2)',
'nocontribs'    => 'Ingen endringer er funnet som passer disse kriteriene.',
'uctop'         => '(siste)',
'month'         => 'Måned:',
'year'          => 'År:',

'sp-contributions-newbies'     => 'Vis kun bidrag fra nye kontoer',
'sp-contributions-newbies-sub' => 'For nybegynnere',
'sp-contributions-blocklog'    => 'Blokkeringslogg',
'sp-contributions-search'      => 'Søk etter bidrag',
'sp-contributions-username'    => 'IP-adresse eller brukernavn:',
'sp-contributions-submit'      => 'Søk',

# What links here
'whatlinkshere'            => 'Lenker hit',
'whatlinkshere-title'      => 'Sider som lenker til «$1»',
'whatlinkshere-page'       => 'Side:',
'linklistsub'              => '(Liste over lenker)',
'linkshere'                => "Følgende sider lenker til '''[[:$1]]''':",
'nolinkshere'              => "Ingen sider lenker til '''[[:$1]]'''.",
'nolinkshere-ns'           => "Ingen sider lenker til '''[[:$1]]''' i valgte navnerom.",
'isredirect'               => 'omdirigeringsside',
'istemplate'               => 'inkludert som mal',
'isimage'                  => 'bildelenke',
'whatlinkshere-prev'       => '{{PLURAL:$1|forrige|forrige $1}}',
'whatlinkshere-next'       => '{{PLURAL:$1|neste|neste $1}}',
'whatlinkshere-links'      => '← lenker',
'whatlinkshere-hideredirs' => '$1 omdirigeringer',
'whatlinkshere-hidetrans'  => '$1 inkluderinger',
'whatlinkshere-hidelinks'  => '$1 lenker',
'whatlinkshere-hideimages' => '$1 fillenker',
'whatlinkshere-filters'    => 'Filtere',

# Block/unblock
'blockip'                         => 'Blokker bruker',
'blockip-legend'                  => 'Blokker bruker',
'blockiptext'                     => 'Bruk skjemaet under for å blokkere en IP-adresses tilgang til å redigere artikler. Dette må kun gjøres for å forhindre hærverk, og i overensstemmelse med [[{{MediaWiki:Policy-url}}|retningslinjene]]. Fyll ut en spesiell begrunnelse under.',
'ipaddress'                       => 'IP-adresse',
'ipadressorusername'              => 'IP-adresse eller brukernavn',
'ipbexpiry'                       => 'Varighet:',
'ipbreason'                       => 'Årsak:',
'ipbreasonotherlist'              => 'Annen grunn',
'ipbreason-dropdown'              => '*Vanlige blokkeringsgrunner
** Legger inn feilinformasjon
** Fjerner innhold fra sider
** Lenkespam
** Legger inn vås
** Truende oppførsel
** Misbruk av flere kontoer
** Uakseptabelt brukernavn',
'ipbanononly'                     => 'Blokker kun anonyme brukere',
'ipbcreateaccount'                => 'Hindre kontoopprettelse',
'ipbemailban'                     => 'Forhindre brukeren fra å sende e-post',
'ipbenableautoblock'              => 'Blokker forrige IP-adresse brukt av denne brukeren automatisk, samt alle IP-adresser brukeren forsøker å redigere med i framtiden',
'ipbsubmit'                       => 'Blokker denne brukeren',
'ipbother'                        => 'Annen tid',
'ipboptions'                      => '2 timer:2 hours,1 dag:1 day,3 dager:3 days,1 uke:1 week,2 uker:2 weeks,1 måned:1 month,3 måneder:3 months,6 måneder:6 months,1 år:1 year,uendelig:infinite', # display1:time1,display2:time2,...
'ipbotheroption'                  => 'annet',
'ipbotherreason'                  => 'Annen/utdypende grunn:',
'ipbhidename'                     => 'Skjul brukernavn i blokkeringsloggen, blokkeringslisten og brukerlisten',
'ipbwatchuser'                    => 'Overvåk brukerens brukerside og diskusjonsside',
'badipaddress'                    => 'Ugyldig IP-adresse.',
'blockipsuccesssub'               => 'Blokkering utført',
'blockipsuccesstext'              => 'IP-adressen «$1» er blokkert. Se [[Special:IPBlockList|blokkeringslisten]] for alle blokkeringer.',
'ipb-edit-dropdown'               => 'Rediger blokkeringsgrunner',
'ipb-unblock-addr'                => 'Avblokker $1',
'ipb-unblock'                     => 'Avblokker et brukernavn eller en IP-adresse',
'ipb-blocklist-addr'              => 'Vis gjeldende blokkeringer for $1',
'ipb-blocklist'                   => 'Vis gjeldende blokkeringer',
'unblockip'                       => 'Opphev blokkering',
'unblockiptext'                   => 'Bruk skjemaet under for å gjenopprette skriveadgangen for en tidligere blokkert adresse eller bruker.',
'ipusubmit'                       => 'Opphev blokkeringen av denne adressen',
'unblocked'                       => '[[User:$1|$1]] ble avblokkert',
'unblocked-id'                    => 'Blokkering $1 ble fjernet',
'ipblocklist'                     => 'Blokkerte IP-adresser og brukere',
'ipblocklist-legend'              => 'Finn en blokkert bruker',
'ipblocklist-username'            => 'Brukernavn eller IP-adresse:',
'ipblocklist-submit'              => 'Søk',
'blocklistline'                   => '$1, $2 blokkerte $3 ($4)',
'infiniteblock'                   => 'uendelig',
'expiringblock'                   => 'utgår $1',
'anononlyblock'                   => 'kun uregistrerte',
'noautoblockblock'                => 'autoblokkering slått av',
'createaccountblock'              => 'kontooppretting blokkert',
'emailblock'                      => 'e-post blokkert',
'ipblocklist-empty'               => 'Blokkeringslisten er tom.',
'ipblocklist-no-results'          => 'Den angitte IP-adressen eller brukeren er ikke blokkert.',
'blocklink'                       => 'blokker',
'unblocklink'                     => 'opphev blokkering',
'contribslink'                    => 'bidrag',
'autoblocker'                     => 'Du ble automatisk blokkert fordi du deler IP-adresse med «[[User:$1|$1]]». Grunnen som ble gitt til at «$1» ble blokkert var: «$2».',
'blocklogpage'                    => 'Blokkeringslogg',
'blocklogentry'                   => 'blokkerte «[[$1]]» med en varighet på $2 $3',
'blocklogtext'                    => 'Dette er en logg som viser hvilke brukere som har blitt blokkert og avblokkert. Automatisk blokkerte IP-adresser vises ikke. Se [[Special:IPBlockList|blokkeringslisten]] for en liste over IP-adresser som er blokkert akkurat nå.',
'unblocklogentry'                 => 'opphevet blokkeringen av $1',
'block-log-flags-anononly'        => 'kun uregistrerte brukere',
'block-log-flags-nocreate'        => 'kontooppretting slått av',
'block-log-flags-noautoblock'     => 'autoblokkering slått av',
'block-log-flags-noemail'         => 'e-post blokkert',
'block-log-flags-angry-autoblock' => 'utvidet autoblokkering aktivert',
'range_block_disabled'            => 'Muligheten til å blokkere flere IP-adresser om gangen er slått av.',
'ipb_expiry_invalid'              => 'Ugyldig utløpstid.',
'ipb_expiry_temp'                 => 'For å skjule brukernavnet må blokkeringen være permanent.',
'ipb_already_blocked'             => '«$1» er allerede blokkert',
'ipb_cant_unblock'                => 'Feil: Blokk-ID $1 ikke funnet. Kan ha blitt avblokkert allerede.',
'ipb_blocked_as_range'            => 'Feil: IP-en $1 er ikke blokkert direkte, og kan ikke avblokkeres. Den er imidlertid blokkert som del av blokkeringa av IP-rangen $2, som kan avblokkeres.',
'ip_range_invalid'                => 'Ugyldig IP-rad.',
'blockme'                         => 'Blokker meg',
'proxyblocker'                    => 'Proxyblokker',
'proxyblocker-disabled'           => 'Denne funksjonen er slått av.',
'proxyblockreason'                => 'IP-adressen din ble blokkert fordi den er en åpen proxy. Kontakt internettleverandøren din eller teknisk støtte og informer dem om dette alvorlige sikkerhetsproblemet.',
'proxyblocksuccess'               => 'Utført.',
'sorbsreason'                     => 'Din IP-adresse angis som en åpen proxy i DNSBL-en brukt av {{SITENAME}}.',
'sorbs_create_account_reason'     => 'Din IP-adresse angis som en åpen proxy i DNSBL-en brukt av {{SITENAME}}. Du kan ikke opprette en konto',

# Developer tools
'lockdb'              => 'Lås database',
'unlockdb'            => 'Åpne database',
'lockdbtext'          => 'Å låse databasen vil avbryte alle brukere fra å kunne
redigere sider, endre deres innstillinger, redigere deres
overvåkningsliste, og andre ting som krever endringer i databasen.
Bekreft at du har til hensikt å gjøre dette, og at du vil
låse opp databasen når vedlikeholdet er utført.',
'unlockdbtext'        => 'Å låse opp databasen vil si at alle brukere igjen
kan redigere sider, endre sine innstillinger, redigere sin
overvåkningsliste, og andre ting som krever endringer i databasen.
Bekreft at du har til hensikt å gjøre dette.',
'lockconfirm'         => 'Ja, jeg vil virkelig låse databasen.',
'unlockconfirm'       => 'Ja, jeg vil virkelig låse opp databasen.',
'lockbtn'             => 'Lås databasen',
'unlockbtn'           => 'Åpne databasen',
'locknoconfirm'       => 'Du har ikke bekreftet handlingen.',
'lockdbsuccesssub'    => 'Databasen er nå låst',
'unlockdbsuccesssub'  => 'Databasen er nå lås opp',
'lockdbsuccesstext'   => 'Databasen er låst.<br />Husk å [[Special:UnlockDB|låse den opp]] når du er ferdig med vedlikeholdet.',
'unlockdbsuccesstext' => 'Databasen er låst opp.',
'lockfilenotwritable' => 'Kan ikke skrive til databasen. For å låse eller åpne databasen, må denne kunne skrives til av tjeneren.',
'databasenotlocked'   => 'Databasen er ikke låst.',

# Move page
'move-page'               => 'Flytt $1',
'move-page-legend'        => 'Flytt side',
'movepagetext'            => "Når du bruker skjemaet under, vil du få omdøpt en side og flyttet hele historikken til det nye navnet.
Den gamle tittelen blir en omdirigeringsside til den nye tittelen.
Du kan oppdatere omdirigeringer som peker til den originale tittelen automatisk.
Om du velger å ikke gjøre det, sjekk at flyttingen ikke skaper noen [[Special:DoubleRedirects|doble]] eller [[Special:BrokenRedirects|ødelagte omdirigeringer]].
Du er ansvarlig for at lenker fortsetter å peke til de sidene de er ment å peke til.

Legg merke til at siden '''ikke''' kan flyttes hvis det allerede finnes en side med den nye tittelen, med mindre den siden er tom eller er en omdirigering uten noen historikk.
Det betyr at du kan flytte en side tilbake dit den kom fra hvis du gjør en feil.

'''ADVARSEL!'''
Dette kan være en drastisk og uventet endring for en populær side;
vær sikker på at du forstår konsekvensene av dette før du fortsetter.",
'movepagetalktext'        => "Den tilhørende diskusjonssiden, hvis den finnes, vil automatisk bli flyttet med siden '''med mindre:'''
*Det allerede finnes en diskusjonsside som ikke er tom med det nye navnet, eller
*Du fjerner markeringen i boksen nedenunder.

I disse tilfellene er du nødt til å flytte eller flette sammen siden manuelt.",
'movearticle'             => 'Flytt side:',
'movenotallowed'          => 'Du har ikke tillatelse til å flytte sider.',
'newtitle'                => 'Ny tittel',
'move-watch'              => 'Overvåk denne siden',
'movepagebtn'             => 'Flytt side',
'pagemovedsub'            => 'Flytting gjennomført',
'movepage-moved'          => "<big>'''«$1» ble flyttet til «$2»'''</big>", # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => 'En side med det navnet finnes fra før, eller valgte navn er ugyldig. Velg et annet navn.',
'cantmove-titleprotected' => 'Du kan ikke flytte en side til dette navnet, fordi den nye tittelen er beskyttet fra opprettelse.',
'talkexists'              => "'''Siden ble flyttet korrekt, men den tilhørende diskusjonssiden kunne ikke flyttes, fordi det allerede finnes en med den nye tittelen. Du er nødt til å flette dem sammen manuelt.'''",
'movedto'                 => 'flyttet til',
'movetalk'                => 'Flytt også diskusjonssiden, hvis den finnes.',
'move-subpages'           => 'Flytt alle undersider, om det finnes noen',
'move-talk-subpages'      => 'Flytt alle undersider av diskusjonssiden, om det finnes noen',
'movepage-page-exists'    => 'Siden $1 finnes allerede og kan ikke overskrives automatisk.',
'movepage-page-moved'     => 'Siden $1 har blitt flyttet til $2.',
'movepage-page-unmoved'   => 'Siden $1 kunne ikke flyttes til $2.',
'movepage-max-pages'      => 'Grensen på {{PLURAL:$1|én side|$1 sider}} er nådd; ingen flere sider vil bli flyttet automatisk.',
'1movedto2'               => '[[$1]] flyttet til [[$2]]',
'1movedto2_redir'         => '[[$1]] flyttet til [[$2]] over omdirigeringsside',
'movelogpage'             => 'Flyttelogg',
'movelogpagetext'         => 'Her er ei liste over sider som har blitt flyttet.',
'movereason'              => 'Årsak:',
'revertmove'              => 'tilbakestill',
'delete_and_move'         => 'Slett og flytt',
'delete_and_move_text'    => '==Sletting nødvendig==
Målsiden «[[:$1]]» finnes allerede. Vil du slette den så denne siden kan flyttes dit?',
'delete_and_move_confirm' => 'Ja, slett siden',
'delete_and_move_reason'  => 'Slettet grunnet flytting',
'selfmove'                => 'Kilde- og destinasjonstittel er den samme; kan ikke flytte siden.',
'immobile_namespace'      => 'Sider kan ikke flyttes til dette navnerommet.',
'imagenocrossnamespace'   => 'Kan ikke flytte bilder til andre navnerom enn bildenavnerommet',
'imagetypemismatch'       => 'Den nye filendelsen tilsvarer ikke filtypen',
'imageinvalidfilename'    => 'Målnavnet er ugyldig',
'fix-double-redirects'    => 'Oppdater omdirigeringer som fører til den gamle tittelen',

# Export
'export'            => 'Eksportsider',
'exporttext'        => 'Du kan eksportere teksten og redigeringshistorikken for en bestemt side eller en gruppe sider i XML.
Dette kan senere importeres til en annen wiki som bruker MediaWiki ved hjelp av [[Special:Import|importsiden]].

For å eksportere sider, skriv inn titler i tekstboksen under, én tittel per linje, og velg om du vil ha kun nåværende versjon, eller alle versjoner i historikken.

Dersom du bare vil ha nåværende versjon, kan du også bruke en lenke, for eksempel [[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]] for siden «[[{{MediaWiki:Mainpage}}]]».',
'exportcuronly'     => 'Ta bare med den nåværende versjonen, ikke hele historikken.',
'exportnohistory'   => "----
'''Merk:''' Eksportering av hele historikken gjennom dette skjemaet har blitt slått av av ytelsesgrunner.",
'export-submit'     => 'Eksporter',
'export-addcattext' => 'Legg til sider fra kategori:',
'export-addcat'     => 'Legg til',
'export-download'   => 'Lagre som fil',
'export-templates'  => 'Ta med maler',

# Namespace 8 related
'allmessages'               => 'Systemmeldinger',
'allmessagesname'           => 'Navn',
'allmessagesdefault'        => 'Standardtekst',
'allmessagescurrent'        => 'Nåværende tekst',
'allmessagestext'           => 'Dette er en liste over tilgjengelige systemmeldinger i MediaWiki-navnerommet.
Besøk [http://translatewiki.net Betawiki] om du ønsker å bidra med oversettelse av MediaWiki.',
'allmessagesnotsupportedDB' => "''{{ns:special}}:Allmessages'' kan ikke brukes fordi '''\$wgUseDatabaseMessages''' er slått av.",
'allmessagesfilter'         => 'Filter:',
'allmessagesmodified'       => 'Vis kun endrede',

# Thumbnails
'thumbnail-more'           => 'Forstørr',
'filemissing'              => 'Fila mangler',
'thumbnail_error'          => 'Feil under oppretting av miniatyrbilde: $1',
'djvu_page_error'          => 'DjVu-side ute av rekkevidde',
'djvu_no_xml'              => 'Klarte ikke å hente XML for DjVu-fil',
'thumbnail_invalid_params' => 'Ugyldige miniatyrparametere, eller PNG-fil med flere piksler enn 12,5 millioner.',
'thumbnail_dest_directory' => 'Klarte ikke å opprette målmappe',

# Special:Import
'import'                     => 'Importer sider',
'importinterwiki'            => 'Transwiki-importering',
'import-interwiki-text'      => 'Velg en wiki og en side å importere. Revisjonsdatoer og bidragsyteres navn blir bevart. Alle transwiki-importeringer listes i [[Special:Log/import|importloggen]].',
'import-interwiki-history'   => 'Kopier all historikk for denne siden',
'import-interwiki-submit'    => 'Importer',
'import-interwiki-namespace' => 'Flytt sidene til navnerommet:',
'importtext'                 => 'Importer fila fra kildewikien med [[Special:Export|eksporteringsverktøyet]], lagre den på den egen datamaskin, og last den opp hit.',
'importstart'                => 'Importerer sider&nbsp;…',
'import-revision-count'      => '({{PLURAL:$1|Én revisjon|$1 revisjoner}})',
'importnopages'              => 'Ingen sider å importere.',
'importfailed'               => 'Importering mislyktes: $1',
'importunknownsource'        => 'Ukjent importkildetype',
'importcantopen'             => 'Kunne ikke åpne importfil',
'importbadinterwiki'         => 'Ugyldig interwikilenke',
'importnotext'               => 'Tom eller ingen tekst',
'importsuccess'              => 'Importering ferdig.',
'importhistoryconflict'      => 'Motstridende revisjoner finnes (siden kan ha blitt importert tidligere)',
'importnosources'            => 'Ingen transwikiimportkilder er angitt, og direkte historikkimporteringer er slått av.',
'importnofile'               => 'Ingen importfil opplastet.',
'importuploaderrorsize'      => 'Importfilopplasting mislyktes. Filen er større enn tillatt opplastingsstørrelse.',
'importuploaderrorpartial'   => 'Importfilopplasting mislyktes. Filen ble kun delvis opplastet.',
'importuploaderrortemp'      => 'Importfilopplasting mislyktes. En midlertidig mappe mangler.',
'import-parse-failure'       => 'Tolkningsfeil ved XML-import',
'import-noarticle'           => 'Ingen side å importere!',
'import-nonewrevisions'      => 'Alle revisjoner var importert fra før.',
'xml-error-string'           => '$1 på linje $2, kolonne $3 (byte: $4): $5',
'import-upload'              => 'Last opp XML-data',

# Import log
'importlogpage'                    => 'Importlogg',
'importlogpagetext'                => 'Administrativ import av sider med redigeringshistorikk fra andre wikier.',
'import-logentry-upload'           => 'importerte [[$1]] ved opplasting',
'import-logentry-upload-detail'    => 'Importerte {{PLURAL:$1|én revisjon|$1 revisjoner}}',
'import-logentry-interwiki'        => 'transwikiimporterte $1',
'import-logentry-interwiki-detail' => '{{PLURAL:$1|Én revisjon|$1 revisjoner}} fra $2',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'Min brukerside',
'tooltip-pt-anonuserpage'         => 'Brukersiden for IP-adressen du redigerer fra',
'tooltip-pt-mytalk'               => 'Min diskusjonsside',
'tooltip-pt-anontalk'             => 'Diskusjon om redigeringer fra denne IP-adressen',
'tooltip-pt-preferences'          => 'Mine innstillinger',
'tooltip-pt-watchlist'            => 'Liste over sider du overvåker for endringer.',
'tooltip-pt-mycontris'            => 'Liste over mine bidrag',
'tooltip-pt-login'                => 'Du oppfordres til å logge inn, men det er ikke obligatorisk.',
'tooltip-pt-anonlogin'            => 'Du oppfordres til å logge inn, men det er ikke obligatorisk.',
'tooltip-pt-logout'               => 'Logg ut',
'tooltip-ca-talk'                 => 'Diskusjon om innholdssiden',
'tooltip-ca-edit'                 => 'Du kan redigere denne siden. Vennligst bruk Forhåndsvis-knappen før du lagrer.',
'tooltip-ca-addsection'           => 'Legg til et diskusjonsinnlegg.',
'tooltip-ca-viewsource'           => 'Denne siden er beskyttet. Du kan se kildeteksten.',
'tooltip-ca-history'              => 'Tidligere revisjoner av denne siden.',
'tooltip-ca-protect'              => 'Beskytt denne siden',
'tooltip-ca-delete'               => 'Slette denne siden',
'tooltip-ca-undelete'             => 'Gjenopprett redigerenge som ble gjort på denne siden før den ble slettet.',
'tooltip-ca-move'                 => 'Flytt denne siden',
'tooltip-ca-watch'                => 'Legg denne siden til overvåkningslisten din',
'tooltip-ca-unwatch'              => 'Fjern denne siden fra din overvåkningsliste.',
'tooltip-search'                  => 'Søk i {{SITENAME}}',
'tooltip-search-go'               => 'Gå til en side med dette navnet dersom det finnes',
'tooltip-search-fulltext'         => 'Søk etter denne teksten',
'tooltip-p-logo'                  => 'Hovedside',
'tooltip-n-mainpage'              => 'Gå til hovedsiden',
'tooltip-n-portal'                => 'Om prosjektet; hva du kan gjøre og hvor du kan finne ting',
'tooltip-n-currentevents'         => 'Finn bakgrunnsinformasjon om aktuelle hendelser',
'tooltip-n-recentchanges'         => 'Liste over siste endringer på wikien.',
'tooltip-n-randompage'            => 'Gå inn på en tilfeldig side',
'tooltip-n-help'                  => 'Stedet for å få hjelp.',
'tooltip-t-whatlinkshere'         => 'Liste over alle sider som lenker hit',
'tooltip-t-recentchangeslinked'   => 'Siste endringer i sider som blir lenket fra denne siden',
'tooltip-feed-rss'                => 'RSS-kilde for denne siden',
'tooltip-feed-atom'               => 'Atom-kilde for denne siden',
'tooltip-t-contributions'         => 'Vis liste over bidrag fra denne brukeren',
'tooltip-t-emailuser'             => 'Send en e-post til denne brukeren',
'tooltip-t-upload'                => 'Last opp filer',
'tooltip-t-specialpages'          => 'Liste over alle spesialsider',
'tooltip-t-print'                 => 'Utskriftsvennlig versjon av denne siden',
'tooltip-t-permalink'             => 'Permanent lenke til denne versjonen av siden',
'tooltip-ca-nstab-main'           => 'Vis innholdssiden',
'tooltip-ca-nstab-user'           => 'Vis brukersiden',
'tooltip-ca-nstab-media'          => 'Vis mediasiden',
'tooltip-ca-nstab-special'        => 'Dette er en spesialside, og kan ikke redigeres.',
'tooltip-ca-nstab-project'        => 'Vis prosjektsiden',
'tooltip-ca-nstab-image'          => 'Vis filsiden',
'tooltip-ca-nstab-mediawiki'      => 'Vis systembeskjeden',
'tooltip-ca-nstab-template'       => 'Vis malen',
'tooltip-ca-nstab-help'           => 'Vis hjelpesiden',
'tooltip-ca-nstab-category'       => 'Vis kategorisiden',
'tooltip-minoredit'               => 'Merk dette som en mindre endring',
'tooltip-save'                    => 'Lagre endringer',
'tooltip-preview'                 => 'Forhåndsvis endringene, vennligst bruk denne funksjonen før du lagrer!',
'tooltip-diff'                    => 'Vis hvilke endringer du har gjort på teksten.',
'tooltip-compareselectedversions' => 'Se forskjellene mellom de to valgte versjonene av denne siden.',
'tooltip-watch'                   => 'Legg denne siden til overvåkningslisten din',
'tooltip-recreate'                => 'Gjenopprett siden til tross for at den har blitt slettet',
'tooltip-upload'                  => 'Start opplasting',

# Stylesheets
'common.css'      => '/* CSS plassert i denne fila vil gjelde for alle utseender. */',
'standard.css'    => '/* CSS i denne fila vil gjelde alle som bruker drakta Standard */',
'nostalgia.css'   => '/* CSS i denne fila vil gjelde alle som bruker drakta Nostalgia */',
'cologneblue.css' => '/* CSS i denne fila vil gjelde alle som bruker drakta Kølnerblå */',
'monobook.css'    => '/* CSS i denne fila vil gjelde alle som bruker drakta Monobook */',
'myskin.css'      => '/* CSS i denne fila vil gjelde alle som bruker drakta Myskin */',
'chick.css'       => '/* CSS i denne fila vil gjelde alle som bruker drakta Chick */',
'simple.css'      => '/* CSS i denne fila vil gjelde alle som bruker drakta Simple */',
'modern.css'      => '/* CSS i denne fila vil gjelde alle som bruker drakta Modern */',

# Scripts
'common.js'      => '/* Javascript i denne fila vil gjelde for alle drakter. */',
'standard.js'    => '/* Javascript i denne fila vil gjelde for brukere av drakta Standard */',
'nostalgia.js'   => '/* Javascript i denne fila vil gjelde for brukere av drakta Nostalgia */',
'cologneblue.js' => '/* Javascript i denne fila vil gjelde for brukere av drakta Kølnerblå */',
'monobook.js'    => '/* Javascript i denne fila vil gjelde for brukere av drakta Monobook */',
'myskin.js'      => '/* Javascript i denne fila vil gjelde for brukere av drakta Myskin */',
'chick.js'       => '/* Javascript i denne fila vil gjelde for brukere av drakta Chick */',
'simple.js'      => '/* Javascript i denne fila vil gjelde for brukere av drakta Simple */',
'modern.js'      => '/* Javascript i denne fila vil gjelde for brukere av drakta Modern */',

# Metadata
'nodublincore'      => 'Dublin Core RDF-metadata er slått av på denne tjeneren.',
'nocreativecommons' => 'Create Commons RDF-metadata er slått av på denne tjeneren.',
'notacceptable'     => 'Tjeneren har ingen mulige måter å vise data i din nettleser.',

# Attribution
'anonymous'        => 'Anonym(e) bruker(e) av {{SITENAME}}',
'siteuser'         => '{{SITENAME}}-bruker $1',
'lastmodifiedatby' => 'Denne siden ble sist redigert $1 kl. $2 av $3.', # $1 date, $2 time, $3 user
'othercontribs'    => 'Basert på arbeid av $1.',
'others'           => 'andre',
'siteusers'        => '{{SITENAME}}-bruker(e) $1',
'creditspage'      => 'Sidekrediteringer',
'nocredits'        => 'Ingen krediteringer er tilgjengelig for denne siden.',

# Spam protection
'spamprotectiontitle' => 'Søppelpostfilter',
'spamprotectiontext'  => 'Siden du ønsket å lagre ble blokkert av spamfilteret.
Dette er sannsynligvis forårsaket av en lenke til et svartelistet eksternt nettsted.',
'spamprotectionmatch' => 'Følgende tekst er det som aktiverte spamfilteret: $1',
'spambot_username'    => 'MediaWikis spamopprydning',
'spam_reverting'      => 'Tilbakestiller til siste versjon uten lenke til $1',
'spam_blanking'       => 'Alle revisjoner inneholdt lenke til $1, tømmer siden',

# Info page
'infosubtitle'   => 'Sideinformasjon',
'numedits'       => 'Antall redigeringer (artikkel): $1',
'numtalkedits'   => 'Antall redigeringer (diskusjonsside): $1',
'numwatchers'    => 'Antall overvåkere: $1',
'numauthors'     => 'Antall forskjellige bidragsytere (artikkel): $1',
'numtalkauthors' => 'Antall forskjellige bidragsytere (diskusjonsside): $1',

# Math options
'mw_math_png'    => 'Vis alltid som PNG',
'mw_math_simple' => 'HTML hvis veldig enkel, ellers PNG',
'mw_math_html'   => 'HTML hvis mulig, ellers PNG',
'mw_math_source' => 'Behold som TeX (for tekst-nettlesere)',
'mw_math_modern' => 'Anbefalt for moderne nettlesere',
'mw_math_mathml' => 'MathML hvis mulig',

# Patrolling
'markaspatrolleddiff'                 => 'Godkjenn endringen',
'markaspatrolledtext'                 => 'Godkjenn denne siden',
'markedaspatrolled'                   => 'Merket som godkjent',
'markedaspatrolledtext'               => 'Endringen er merket som godkjent.',
'rcpatroldisabled'                    => 'Siste endringer-patruljering er slått av',
'rcpatroldisabledtext'                => 'Siste endringer-patruljeringsfunksjonen er slått av.',
'markedaspatrollederror'              => 'Kan ikke merke som godkjent',
'markedaspatrollederrortext'          => 'Du må spesifisere en versjon å merke som godkjent.',
'markedaspatrollederror-noautopatrol' => 'Du kan ikke merke dine egne endringer som godkjente.',

# Patrol log
'patrol-log-page'   => 'Godkjenningslogg',
'patrol-log-header' => 'Dette er en logg over patruljerte sideversjoner.',
'patrol-log-line'   => 'merket $1 av $2 godkjent $3',
'patrol-log-auto'   => '(automatisk)',

# Image deletion
'deletedrevision'                 => 'Slettet gammel revisjon $1.',
'filedeleteerror-short'           => 'Feil under filsletting: $1',
'filedeleteerror-long'            => 'Feil oppsto under filsletting:

$1',
'filedelete-missing'              => 'Filen «$1» kan ikke slettes fordi den ikke finnes.',
'filedelete-old-unregistered'     => 'Filrevisjonen «$1» finnes ikke i databasen.',
'filedelete-current-unregistered' => 'Filen «$1» finnes ikke i databasen.',
'filedelete-archive-read-only'    => 'Arkivmappa «$1» kan ikke skrives av tjeneren.',

# Browsing diffs
'previousdiff' => '← Eldre redigering',
'nextdiff'     => 'Nyere redigering →',

# Media information
'mediawarning'         => "'''Advarsel''': Denne fila kan inneholde farlig kode; ved å åpne den kan systemet ditt kompromitteres.<hr />",
'imagemaxsize'         => 'Begrens bilder på filbeskrivelsessider til:',
'thumbsize'            => 'Miniatyrbildestørrelse:',
'widthheightpage'      => '$1×$2, {{PLURAL:$3|én side|$3 sider}}',
'file-info'            => '(filstørrelse: $1, MIME-type: $2)',
'file-info-size'       => '($1 × $2 piksler, filstørrelse: $3, MIME-type: $4)',
'file-nohires'         => '<small>Ingen høyere oppløsning tilgjengelig.</small>',
'svg-long-desc'        => '(SVG-fil, standardoppløsning $1 × $2 piksler, filstørrelse $3)',
'show-big-image'       => 'Full oppløsning',
'show-big-image-thumb' => '<small>Størrelse på denne forhåndsvisningen: $1 × $2 piksler</small>',

# Special:NewImages
'newimages'             => 'Galleri over nye filer',
'imagelisttext'         => "Dete er en liste med '''$1''' {{PLURAL:$1|fil|filer}} sortert $2.",
'newimages-summary'     => 'Denne spesialsiden viser de sist opplastede filene.',
'showhidebots'          => '($1 roboter)',
'noimages'              => 'Ingenting å se.',
'ilsubmit'              => 'Søk',
'bydate'                => 'etter dato',
'sp-newimages-showfrom' => 'Vis nye filer fra og med $2 $1',

# Video information, used by Language::formatTimePeriod() to format lengths in the above messages
'hours-abbrev' => 't',

# Bad image list
'bad_image_list' => 'Formatet er slik:

Kun listeelementer (linjer som starter med *) tas med. Den første lenka på en linje må være en lenke til en dårlig fil. Alle andre linker på samme linje anses å være unntak, altså artikler hvor filen er tillatt brukt.',

# Metadata
'metadata'          => 'Metadata',
'metadata-help'     => 'Denne filen inneholder tilleggsinformasjon, antagligvis fra digitalkameraet eller skanneren brukt til å lage eller digitalisere det. Hvis filen har blitt forandret fra utgangspunktet, kan enkelte detaljer kanskje være unøyaktige.',
'metadata-expand'   => 'Vis detaljer',
'metadata-collapse' => 'Skjul detaljer',
'metadata-fields'   => 'EXIF-metadatafelt i denne beskjeden inkluderes på bildesiden mens metadatatabellen er slått sammen. Andre vil skjules som standard.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => 'Bredde',
'exif-imagelength'                 => 'Høyde',
'exif-bitspersample'               => 'Bits per komponent',
'exif-compression'                 => 'Kompresjonsskjema',
'exif-photometricinterpretation'   => 'Pixelsammensetning',
'exif-orientation'                 => 'Retning',
'exif-samplesperpixel'             => 'Antall komponenter',
'exif-planarconfiguration'         => 'Dataarrangement',
'exif-ycbcrsubsampling'            => 'Subsamplingsforhold mellom Y og C',
'exif-ycbcrpositioning'            => 'Y- og C-posisjonering',
'exif-xresolution'                 => 'Horisontal oppløsning',
'exif-yresolution'                 => 'Vertikal oppløsning',
'exif-resolutionunit'              => 'Enhet for X- og Y-oppløsning',
'exif-stripoffsets'                => 'Plassering for bildedata',
'exif-rowsperstrip'                => 'Antall rader per stripe',
'exif-stripbytecounts'             => 'Antall byte per kompresserte stripe',
'exif-jpeginterchangeformat'       => 'Offset til JPEG SOI',
'exif-jpeginterchangeformatlength' => 'Byte med JPEG-data',
'exif-transferfunction'            => 'Overføringsfunksjon',
'exif-whitepoint'                  => 'Hvitpunktkromatisitet',
'exif-primarychromaticities'       => 'Primærfargenes renhet',
'exif-ycbcrcoefficients'           => 'Koeffisienter fr fargeromstransformasjonsmatrise',
'exif-referenceblackwhite'         => 'Par av svarte og hvite referanseverdier',
'exif-datetime'                    => 'Dato og tid for filendring',
'exif-imagedescription'            => 'Bildetittel',
'exif-make'                        => 'Kameraprodusent',
'exif-model'                       => 'Kameramodell',
'exif-software'                    => 'Programvare brukt',
'exif-artist'                      => 'Skaper',
'exif-copyright'                   => 'Opphavsbeskyttelse tilhører',
'exif-exifversion'                 => 'Exif-versjon',
'exif-flashpixversion'             => 'Støttet Flashpix-versjon',
'exif-colorspace'                  => 'Fargerom',
'exif-componentsconfiguration'     => 'Betydning av hver komponent',
'exif-compressedbitsperpixel'      => 'Bildekompresjonsmodus',
'exif-pixelydimension'             => 'Gyldig bildebredde',
'exif-pixelxdimension'             => 'Gyldig bildehøyde',
'exif-makernote'                   => 'Fabrikkmerknader',
'exif-usercomment'                 => 'Brukerkommentarer',
'exif-relatedsoundfile'            => 'Relatert lydfil',
'exif-datetimeoriginal'            => 'Dato og tid for datagenerering',
'exif-datetimedigitized'           => 'Dato og tid for digitalisering',
'exif-subsectime'                  => 'Endringstidspunkt, sekunddeler',
'exif-subsectimeoriginal'          => 'Eksponeringstidspunkt, sekunddeler',
'exif-subsectimedigitized'         => 'Digitaliseringstidspunkt, sekunddeler',
'exif-exposuretime'                => 'Eksponeringstid',
'exif-exposuretime-format'         => '$1 sek ($2)',
'exif-fnumber'                     => 'F-nummer',
'exif-exposureprogram'             => 'Eksponeringsprogram',
'exif-spectralsensitivity'         => 'Spektralsensitivitet',
'exif-isospeedratings'             => 'Filmhastighet (ISO)',
'exif-oecf'                        => 'Optoelektronisk konversjonsfaktor',
'exif-shutterspeedvalue'           => 'Lukkerhastighet',
'exif-aperturevalue'               => 'Apertur',
'exif-brightnessvalue'             => 'Lysstyrke',
'exif-exposurebiasvalue'           => 'Eksponeringsbias',
'exif-maxaperturevalue'            => 'Maksimal blender',
'exif-subjectdistance'             => 'Avstand til subjekt',
'exif-meteringmode'                => 'Målingsmodus',
'exif-lightsource'                 => 'Lyskilde',
'exif-flash'                       => 'Blits',
'exif-focallength'                 => 'Linsens brennvidde',
'exif-subjectarea'                 => 'Motivområde',
'exif-flashenergy'                 => 'Blitsenergi',
'exif-spatialfrequencyresponse'    => 'Romslig frekvensrespons',
'exif-focalplanexresolution'       => 'Oppløsning i fokalplan X',
'exif-focalplaneyresolution'       => 'Oppløsning i fokalplan Y',
'exif-focalplaneresolutionunit'    => 'Enhet for oppløsning i fokalplan',
'exif-subjectlocation'             => 'Motivets beliggenhet',
'exif-exposureindex'               => 'Eksponeringsindeks',
'exif-sensingmethod'               => 'Avkjenningsmetode',
'exif-filesource'                  => 'Filkilde',
'exif-scenetype'                   => 'Scenetype',
'exif-cfapattern'                  => 'CFA-mønster',
'exif-customrendered'              => 'Tilpasset bildebehandling',
'exif-exposuremode'                => 'Eksponeringsmodus',
'exif-whitebalance'                => 'Hvit balanse',
'exif-digitalzoomratio'            => 'Digitalt zoomomfang',
'exif-focallengthin35mmfilm'       => 'Brennvidde på 35 mm-film',
'exif-scenecapturetype'            => 'Motivprogram',
'exif-gaincontrol'                 => 'Scenekontroll',
'exif-contrast'                    => 'Kontrast',
'exif-saturation'                  => 'Metning',
'exif-sharpness'                   => 'Skarphet',
'exif-devicesettingdescription'    => 'Beskrivelse av apparatets innstilling',
'exif-subjectdistancerange'        => 'Avstandsintervall til motiv',
'exif-imageuniqueid'               => 'Unik bilde-ID',
'exif-gpsversionid'                => 'Versjon for GPS-tagger',
'exif-gpslatituderef'              => 'nordlig eller sørlig breddegrad',
'exif-gpslatitude'                 => 'Breddegrad',
'exif-gpslongituderef'             => 'Østlig eller vestlig breddegrad',
'exif-gpslongitude'                => 'Lengdegrad',
'exif-gpsaltituderef'              => 'Høydereferanse',
'exif-gpsaltitude'                 => 'Høyde',
'exif-gpstimestamp'                => 'GPS-tid (atomklokke)',
'exif-gpssatellites'               => 'Satelitter brukt i måling',
'exif-gpsstatus'                   => 'Mottakerstatus',
'exif-gpsmeasuremode'              => 'Målingsmodus',
'exif-gpsdop'                      => 'Målingspresisjon',
'exif-gpsspeedref'                 => 'Fartsenhet',
'exif-gpsspeed'                    => 'GPS-mottakerens hastighet',
'exif-gpstrackref'                 => 'Referanse for bevegelsesretning',
'exif-gpstrack'                    => 'Bevegelsesretning',
'exif-gpsimgdirectionref'          => 'Referanse for bilderetning',
'exif-gpsimgdirection'             => 'Bilderetning',
'exif-gpsmapdatum'                 => 'Brukt geodetisk data',
'exif-gpsdestlatituderef'          => 'Referanse for målbreddegrad',
'exif-gpsdestlatitude'             => 'Målbreddegrad',
'exif-gpsdestlongituderef'         => 'Referanse for mållengdegrad',
'exif-gpsdestlongitude'            => 'Mållengdegrad',
'exif-gpsdestbearingref'           => 'Referanse for retning mot målet',
'exif-gpsdestbearing'              => 'Retning mot målet',
'exif-gpsdestdistanceref'          => 'Referanse for lengde til mål',
'exif-gpsdestdistance'             => 'Lengde til mål',
'exif-gpsprocessingmethod'         => 'Navn på GPS-prosesseringsmetode',
'exif-gpsareainformation'          => 'Navn på GPS-område',
'exif-gpsdatestamp'                => 'GPS-dato',
'exif-gpsdifferential'             => 'Differentiell GPS-korreksjon',

# EXIF attributes
'exif-compression-1' => 'Ukomprimert',

'exif-unknowndate' => 'Ukjent dato',

'exif-orientation-1' => 'Normal', # 0th row: top; 0th column: left
'exif-orientation-2' => 'Snudd horisontalt', # 0th row: top; 0th column: right
'exif-orientation-3' => 'Rotert 180°', # 0th row: bottom; 0th column: right
'exif-orientation-4' => 'Snudd vertikalt', # 0th row: bottom; 0th column: left
'exif-orientation-5' => 'Rotated 90° CCW and flipped vertically

Rotert 90° mot klokka og vridd vertikalt', # 0th row: left; 0th column: top
'exif-orientation-6' => 'Rotert 90° med klokka', # 0th row: right; 0th column: top
'exif-orientation-7' => 'Rotert 90° med klokka og vridd vertikalt', # 0th row: right; 0th column: bottom
'exif-orientation-8' => 'Rotert 90° mot klokka', # 0th row: left; 0th column: bottom

'exif-planarconfiguration-1' => 'chunkformat',
'exif-planarconfiguration-2' => 'planærformat',

'exif-componentsconfiguration-0' => 'finnes ikke',

'exif-exposureprogram-0' => 'Ikke angitt',
'exif-exposureprogram-1' => 'Manuell',
'exif-exposureprogram-2' => 'Normalt program',
'exif-exposureprogram-3' => 'Blenderprioritet',
'exif-exposureprogram-4' => 'Slutterprioritet',
'exif-exposureprogram-5' => 'Kunstnerlig program (prioriterer skarphetsdyp)',
'exif-exposureprogram-6' => 'Bevegelsesprogram (prioriterer kortere sluttertid)',
'exif-exposureprogram-7' => 'Portrettmodus (for nærbilder med ufokusert bakgrunn)',
'exif-exposureprogram-8' => 'Landskapsmodus (for landskapsbilder med fokusert bakgrunn)',

'exif-subjectdistance-value' => '$1 meter',

'exif-meteringmode-0'   => 'Ukjent',
'exif-meteringmode-1'   => 'Gjennomsnitt',
'exif-meteringmode-2'   => 'Sentrumsveid gjennomsnitt',
'exif-meteringmode-3'   => 'Spot',
'exif-meteringmode-4'   => 'Multispot',
'exif-meteringmode-5'   => 'Mønster',
'exif-meteringmode-6'   => 'Delvis',
'exif-meteringmode-255' => 'Annet',

'exif-lightsource-0'   => 'Ukjent',
'exif-lightsource-1'   => 'Dagslys',
'exif-lightsource-2'   => 'Lysrør',
'exif-lightsource-3'   => 'Glødelampe',
'exif-lightsource-4'   => 'Blits',
'exif-lightsource-9'   => 'Fint vær',
'exif-lightsource-10'  => 'Overskyet',
'exif-lightsource-11'  => 'Skygge',
'exif-lightsource-12'  => 'Dagslyslysrør (D 5700 – 7100K)',
'exif-lightsource-13'  => 'Dagshvitt lysrør (N 4600 – 5400K)',
'exif-lightsource-14'  => 'Kaldhvitt lysrør (W 3900 – 4500K)',
'exif-lightsource-15'  => 'Hvitt lysrør (WW 3200 – 3700K)',
'exif-lightsource-17'  => 'Standardlys A',
'exif-lightsource-18'  => 'Standardlys B',
'exif-lightsource-19'  => 'Standardlys C',
'exif-lightsource-24'  => 'ISO studiobelysning',
'exif-lightsource-255' => 'Annen lyskilde',

'exif-focalplaneresolutionunit-2' => 'tommer',

'exif-sensingmethod-1' => 'Ikke angitt',
'exif-sensingmethod-2' => 'Énchipsfargesensor',
'exif-sensingmethod-3' => 'Tochipsfargesensor',
'exif-sensingmethod-4' => 'Trechipsfargesensor',
'exif-sensingmethod-5' => 'Fargesekvensiell områdesensor',
'exif-sensingmethod-7' => 'Trilineær sensor',
'exif-sensingmethod-8' => 'Fargesekvensiell linær sensor',

'exif-scenetype-1' => 'Direktefotografert bilde',

'exif-customrendered-0' => 'Normal prosess',
'exif-customrendered-1' => 'Tilpasset prosess',

'exif-exposuremode-0' => 'Automatisk eksponering',
'exif-exposuremode-1' => 'Manuell eksponering',
'exif-exposuremode-2' => 'Automatisk alternativeksponering',

'exif-whitebalance-0' => 'Automatisk hvitbalanse',
'exif-whitebalance-1' => 'Manuell hvitbalanse',

'exif-scenecapturetype-0' => 'Standard',
'exif-scenecapturetype-1' => 'Landskap',
'exif-scenecapturetype-2' => 'Portrett',
'exif-scenecapturetype-3' => 'Nattscene',

'exif-gaincontrol-0' => 'Ingen',
'exif-gaincontrol-1' => 'Økning av lavnivåforsterkning',
'exif-gaincontrol-2' => 'Økning av høynivåforsterkning',
'exif-gaincontrol-3' => 'Senkning av lavnivåforsterkning',
'exif-gaincontrol-4' => 'Senkning av høynivåforsterkning',

'exif-contrast-0' => 'Normal',
'exif-contrast-1' => 'Myk',
'exif-contrast-2' => 'Hard',

'exif-saturation-0' => 'Normal',
'exif-saturation-1' => 'Lav metningsgrad',
'exif-saturation-2' => 'Høy metningsgrad',

'exif-sharpness-0' => 'Normal',
'exif-sharpness-1' => 'Myk',
'exif-sharpness-2' => 'Hard',

'exif-subjectdistancerange-0' => 'Ukjent',
'exif-subjectdistancerange-1' => 'Makro',
'exif-subjectdistancerange-2' => 'Nærbilde',
'exif-subjectdistancerange-3' => 'Fjernbilde',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'Nordlig breddegrad',
'exif-gpslatitude-s' => 'Sørlig breddegrad',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'Østlig lengdegrad',
'exif-gpslongitude-w' => 'Vestlig lengdegrad',

'exif-gpsstatus-a' => 'Måling pågår',
'exif-gpsstatus-v' => 'Målingsinteroperabilitet',

'exif-gpsmeasuremode-2' => 'todimensjonell måling',
'exif-gpsmeasuremode-3' => 'tredimensjonell måling',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'Kilometer per time',
'exif-gpsspeed-m' => 'Miles per time',
'exif-gpsspeed-n' => 'Knop',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => 'Sann retning',
'exif-gpsdirection-m' => 'Magnetisk retning',

# External editor support
'edit-externally'      => 'Rediger denne fila med et eksternt program',
'edit-externally-help' => 'Se [http://www.mediawiki.org/wiki/Manual:External_editors oppsettsinstruksjonene] for mer informasjon.',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'alle',
'imagelistall'     => 'alle',
'watchlistall2'    => 'alle',
'namespacesall'    => 'alle',
'monthsall'        => 'alle',

# E-mail address confirmation
'confirmemail'             => 'Bekreft e-postadresse',
'confirmemail_noemail'     => 'Du har ikke oppgitt en gyldig e-postadresse i [[Special:Preferences|innstillingene dine]].',
'confirmemail_text'        => 'Du må bekrefte e-postadressen din før du kan benytte deg av e-posttjenester på {{SITENAME}}. Trykk på knappen under for å sende en bekreftelsesmelding til e-postadressen din. Meldingen vil inneholde en lenke med en kode; følg lenken for å bekrefte at e-postadressen er gyldig.',
'confirmemail_pending'     => '<div class="error">
En bekreftelseskode har allerede blitt sendt til deg på e-post; om du nylig opprettet kontoen din, kan du ønske å vente noen minutter før du spør om ny kode.
</div>',
'confirmemail_send'        => 'Send en bekreftelseskode.',
'confirmemail_sent'        => 'Bekreftelsesmelding sendt.',
'confirmemail_oncreate'    => 'En bekreftelseskode ble sendt til din e-postadresse. Denne koden er ikke nødvendig for å logge inn, men er nødvendig for å slå på e-postbaserte tjenester i denne wikien.',
'confirmemail_sendfailed'  => '{{SITENAME}} klarte ikke å sende bekreftelseskode.
Sjekk e-postadressen for ugyldige tegn.

E-postsenderen ga følgende melding: $1',
'confirmemail_invalid'     => 'Ugyldig bekreftelseskode. Koden kan ha utløpt.',
'confirmemail_needlogin'   => 'Du må $1 for å bekrefte e-postadressen din.',
'confirmemail_success'     => 'Din e-postadresse er nå bekreftet. Du kan nå logge inn og nyte wikien.',
'confirmemail_loggedin'    => 'E-postadressen din er bekreftet.',
'confirmemail_error'       => 'Noe gikk galt under lagringen av din bekreftelse.',
'confirmemail_subject'     => 'Bekreftelsesmelding fra {{SITENAME}}',
'confirmemail_body'        => 'Noen, antageligvis deg, har registrert kontoen «$2» på {{SITENAME}}, fra IP-adressen $1.

For å bekrefte at denne kontoen tilhører deg og for å aktivere e-posttjenester på {{SITENAME}}, åpne følgende lenke i nettleseren din:

$3

Om du *ikke* registrerte kontoen, følg denne lenken for å avbryte bekreftelse av e-postadresse:

$5

Denne bekreftelseskoden utgår $4.',
'confirmemail_invalidated' => 'Bekreftelse av e-postadresse avbrutt',
'invalidateemail'          => 'Avbryt bekreftelse av e-postadresse',

# Scary transclusion
'scarytranscludedisabled' => '[Interwiki-transkludering er slått av]',
'scarytranscludefailed'   => '[Malen kunne ikke hentes for $1]',
'scarytranscludetoolong'  => '[URL-en er for lang]',

# Trackbacks
'trackbackbox'      => '<div id="mw_trackbacks">
Tilbakesporinger for denne artikkelen:<br />
$1
</div>',
'trackbackremove'   => ' ([$1 Slett])',
'trackbacklink'     => 'Tilbakesporing',
'trackbackdeleteok' => 'Tilbakesporingen ble slettet.',

# Delete conflict
'deletedwhileediting' => "'''Advarsel:''' Denne siden har blitt slettet etter at du begynte å redigere den!",
'confirmrecreate'     => '«[[User:$1|$1]]» ([[User talk:$1|diskusjon]]) slettet siden etter at du begynte å redigere den, med begrunnelsen «$2». Vennligst bekreft at du vil gjenopprette siden.',
'recreate'            => 'Gjenopprett',

# HTML dump
'redirectingto' => 'Omdirigerer til [[:$1]]&nbsp;…',

# action=purge
'confirm_purge'        => "Vil du slette tjenerens mellomlagrede versjon (''cache'') av denne siden? $1",
'confirm_purge_button' => 'OK',

# AJAX search
'searchcontaining' => "Søk etter artikler som inneholder ''$1''.",
'searchnamed'      => "Søk for artikler ved navn ''$1''.",
'articletitles'    => "Artikler som begynner med ''$1''",
'hideresults'      => 'Skjul resultater',
'useajaxsearch'    => 'Bruk AJAX-søk',

# Multipage image navigation
'imgmultipageprev' => '← forrige side',
'imgmultipagenext' => 'neste side &rarr;',
'imgmultigo'       => 'Gå!',
'imgmultigoto'     => 'Gå til siden $1',

# Table pager
'ascending_abbrev'         => 'stig.',
'descending_abbrev'        => 'synk.',
'table_pager_next'         => 'Neste side',
'table_pager_prev'         => 'Forrige side',
'table_pager_first'        => 'Første side',
'table_pager_last'         => 'Siste side',
'table_pager_limit'        => 'Vis $1 elementer per side',
'table_pager_limit_submit' => 'Gå',
'table_pager_empty'        => 'Ingen resultater',

# Auto-summaries
'autosumm-blank'   => 'Tømmer siden',
'autosumm-replace' => 'Erstatter siden med «$1»',
'autoredircomment' => 'Omdirigerer til [[$1]]',
'autosumm-new'     => 'Ny side: $1',

# Live preview
'livepreview-loading' => 'Laster&nbsp;…',
'livepreview-ready'   => 'Laster&nbsp;… Klar!',
'livepreview-failed'  => 'Levende forhåndsvisning mislyktes. Prøv vanlig forhåndsvisning.',
'livepreview-error'   => 'Tilkobling mislyktes: $1 «$2»
Prøv vanlig forhåndsvisning.',

# Friendlier slave lag warnings
'lag-warn-normal' => 'Endringer nyere enn $1 {{PLURAL:$1|sekund|sekunder}} vises muligens ikke i denne listen.',
'lag-warn-high'   => 'På grunn av stor databaseforsinkelse, vil ikke endringer som er nyere enn $1 {{PLURAL:$1|sekund|sekunder}} vises i denne listen.',

# Watchlist editor
'watchlistedit-numitems'       => 'Overvåkningslisten din inneholder {{PLURAL:$1|én tittel|$1 titler}}, ikke inkludert diskusjonssider.',
'watchlistedit-noitems'        => 'Overvåkningslisten din inneholder ingen titler.',
'watchlistedit-normal-title'   => 'Rediger overvåkningsliste',
'watchlistedit-normal-legend'  => 'Fjern titler fra overvåkninglisten',
'watchlistedit-normal-explain' => 'Titler på overvåkningslisten din vises nedenunder. For å fjerne en tittel, merk av boksen ved siden av den og klikk «fjern titler». Du kan også [[Special:Watchlist/raw|redigere den rå overvåkningslisten]].',
'watchlistedit-normal-submit'  => 'Fjern titler',
'watchlistedit-normal-done'    => '{{PLURAL:$1|Én tittel|$1 titler}} ble fjernet fra overvåkningslisten din:',
'watchlistedit-raw-title'      => 'Rediger rå overvåkningsliste',
'watchlistedit-raw-legend'     => 'Rediger rå overvåkningsliste',
'watchlistedit-raw-explain'    => 'Titler på overvåkningslisten din vises nedenunder, og kan redigeres ved å legge til eller fjerne fra listen; én tittel per linje. Når du er ferdig, trykk Oppdater overvåkningsliste. Du kan også [[Special:Watchlist/edit|bruke standardverktøyet]].',
'watchlistedit-raw-titles'     => 'Titler:',
'watchlistedit-raw-submit'     => 'Oppdater overvåkningsliste',
'watchlistedit-raw-done'       => 'Overvåkningslisten din er oppdatert.',
'watchlistedit-raw-added'      => '{{PLURAL:$1|Én tittel|$1 titler}} ble lagt til:',
'watchlistedit-raw-removed'    => '{{PLURAL:$1|Én tittel|$1 titler}} ble fjernet:',

# Watchlist editing tools
'watchlisttools-view' => 'Vis relevante endringer',
'watchlisttools-edit' => 'Vis og rediger overvåkningsliste',
'watchlisttools-raw'  => 'Rediger rå overvåkningsliste',

# Hebrew month names
'hebrew-calendar-m1'      => 'Tisjri',
'hebrew-calendar-m2'      => 'Hesjván',
'hebrew-calendar-m3'      => 'Kislév',
'hebrew-calendar-m4'      => 'Tebét',
'hebrew-calendar-m5'      => 'Sjebát',
'hebrew-calendar-m6'      => 'Adár',
'hebrew-calendar-m6a'     => 'Adár I',
'hebrew-calendar-m6b'     => 'Adár II',
'hebrew-calendar-m7'      => 'Nisán',
'hebrew-calendar-m8'      => 'Ijár',
'hebrew-calendar-m9'      => 'Siván',
'hebrew-calendar-m10'     => 'Tammúz',
'hebrew-calendar-m11'     => 'Ab',
'hebrew-calendar-m12'     => 'Elúl',
'hebrew-calendar-m1-gen'  => 'Tisjri',
'hebrew-calendar-m2-gen'  => 'Hesjván',
'hebrew-calendar-m3-gen'  => 'Kislév',
'hebrew-calendar-m4-gen'  => 'Tebét',
'hebrew-calendar-m5-gen'  => 'Sjebát',
'hebrew-calendar-m6-gen'  => 'Adár',
'hebrew-calendar-m6a-gen' => 'Adár I',
'hebrew-calendar-m6b-gen' => 'Adár II',
'hebrew-calendar-m7-gen'  => 'Nisán',
'hebrew-calendar-m8-gen'  => 'Ijár',
'hebrew-calendar-m9-gen'  => 'Siván',
'hebrew-calendar-m10-gen' => 'Tammúz',
'hebrew-calendar-m11-gen' => 'Ab',
'hebrew-calendar-m12-gen' => 'Elúl',

# Core parser functions
'unknown_extension_tag' => 'Ukjent tilleggsmerking «$1»',

# Special:Version
'version'                          => 'Versjon', # Not used as normal message but as header for the special page itself
'version-extensions'               => 'Installerte utvidelser',
'version-specialpages'             => 'Spesialsider',
'version-parserhooks'              => 'Parsertillegg',
'version-variables'                => 'Variabler',
'version-other'                    => 'Annet',
'version-mediahandlers'            => 'Mediahåndterere',
'version-hooks'                    => 'Haker',
'version-extension-functions'      => 'Tilleggsfunksjoner',
'version-parser-extensiontags'     => 'Tilleggstagger',
'version-parser-function-hooks'    => 'Parserfunksjoner',
'version-skin-extension-functions' => 'Skalltilleggsfunksjoner',
'version-hook-name'                => 'Navn',
'version-hook-subscribedby'        => 'Brukes av',
'version-version'                  => 'versjon',
'version-license'                  => 'Lisens',
'version-software'                 => 'Installert programvare',
'version-software-product'         => 'Produkt',
'version-software-version'         => 'Versjon',

# Special:FilePath
'filepath'         => 'Filsti',
'filepath-page'    => 'Fil:',
'filepath-submit'  => 'Sti',
'filepath-summary' => 'Denne spesialsiden gir den fullstendige stien for en fil. Bilder vises i full oppløsning; andre filtyper startes direkte i sine assosierte programmer.

	Skriv inn filnavnet uten «{{ns:image}}:»-prefikset.',

# Special:FileDuplicateSearch
'fileduplicatesearch'          => 'Søk etter duplikatfiler',
'fileduplicatesearch-summary'  => 'Søk etter duplikatfiler basert på dets hash-verdi.

Skriv inn filnavn uten «{{ns:image}}:»-prefikset.',
'fileduplicatesearch-legend'   => 'Søk etter en duplikatfil',
'fileduplicatesearch-filename' => 'Filnavn:',
'fileduplicatesearch-submit'   => 'Søk',
'fileduplicatesearch-info'     => '$1 × $2 piksler<br />Filstørrelse: $3<br />MIME-type: $4',
'fileduplicatesearch-result-1' => 'Det er ingen duplikater av «$1».',
'fileduplicatesearch-result-n' => 'Det er {{PLURAL:$2|ett duplikat|$2 duplikater}} av «$1».',

# Special:SpecialPages
'specialpages'                   => 'Spesialsider',
'specialpages-note'              => '----
* <span class="mw-specialpagerestricted">Markerte spesialsider har begrenset tilgang.</span>',
'specialpages-group-maintenance' => 'Vedlikeholdsrapporter',
'specialpages-group-other'       => 'Andre spesialsider',
'specialpages-group-login'       => 'Innlogging / registrering',
'specialpages-group-changes'     => 'Siste endringer og logger',
'specialpages-group-media'       => 'Mediarapporter og opplastinger',
'specialpages-group-users'       => 'Brukere og rettigheter',
'specialpages-group-highuse'     => 'Ofte brukte sider',
'specialpages-group-pages'       => 'Sidelister',
'specialpages-group-pagetools'   => 'Sideverktøy',
'specialpages-group-wiki'        => 'Informasjon og verktøy for wikien',
'specialpages-group-redirects'   => 'Omdirigerende spesialsider',
'specialpages-group-spam'        => 'Spamverktøy',

# Special:BlankPage
'blankpage'              => 'Tom side',
'intentionallyblankpage' => 'Denne siden er tom med vilje',

);
