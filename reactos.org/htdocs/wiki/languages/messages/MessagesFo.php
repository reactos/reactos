<?php
/** Faroese (Føroyskt)
 *
 * @ingroup Language
 * @file
 *
 * @author Krun
 * @author Quackor
 * @author S.Örvarr.S
 * @author Spacebirdy
 * @author לערי ריינהארט
 */

$skinNames = array(
	'standard'    => 'Standardur',
	'nostalgia'   => 'Nostalgiskur',
	'cologneblue' => 'Cologne-bláur'
);

$bookstoreList = array(
	'Bokasolan.fo' => 'http://www.bokasolan.fo/vleitari.asp?haattur=bok.alfa&Heiti=&Hovindur=&Forlag=&innbinding=Oell&bolkur=Allir&prisur=Allir&Aarstal=Oell&mal=Oell&status=Oell&ISBN=$1',
	'inherit' => true,
);

$namespaceNames = array(
	NS_MEDIA          => 'Miðil',
	NS_SPECIAL        => 'Serstakur',
	NS_MAIN           => '',
	NS_TALK           => 'Kjak',
	NS_USER           => 'Brúkari',
	NS_USER_TALK      => 'Brúkari_kjak',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK   => '$1_kjak',
	NS_IMAGE          => 'Mynd',
	NS_IMAGE_TALK     => 'Mynd_kjak',
	NS_MEDIAWIKI      => 'MidiaWiki',
	NS_MEDIAWIKI_TALK => 'MidiaWiki_kjak',
	NS_TEMPLATE       => 'Fyrimynd',
	NS_TEMPLATE_TALK  => 'Fyrimynd_kjak',
	NS_HELP           => 'Hjálp',
	NS_HELP_TALK      => 'Hjálp kjak',
	NS_CATEGORY       => 'Bólkur',
	NS_CATEGORY_TALK  => 'Bólkur_kjak',
);

$skinNames = array(
	'standard' => 'Standardur',
	'nostalgia' => 'Nostalgiskur',
	'cologneblue' => 'Cologne-bláur',
);

$datePreferences = false;
$defaultDateFormat = 'dmy';
$dateFormats = array(
	'dmy time' => 'H:i',
	'dmy date' => 'j. M Y',
	'dmy both' => 'j. M Y "kl." H:i',
);

$specialPageAliases = array(
	'DoubleRedirects'           => array( 'Tvífaldað_ávísing' ),
	'BrokenRedirects'           => array( 'Brotnar_ávísingar' ),
	'Disambiguations'           => array( 'Síður_við_fleirfaldum_týdningi' ),
	'Userlogin'                 => array( 'Stovna_kontu_ella_rita_inn' ),
	'Userlogout'                => array( 'Rita_út' ),
	'Preferences'               => array( 'Innstillingar' ),
	'Watchlist'                 => array( 'Mítt_eftirlit' ),
	'Recentchanges'             => array( 'Seinastu_broytingar' ),
	'Upload'                    => array( 'Legg_fílu_upp' ),
	'Imagelist'                 => array( 'Myndalisti' ),
	'Newimages'                 => array( 'Nýggjar_myndir' ),
	'Listusers'                 => array( 'Brúkaralisti' ),
	'Statistics'                => array( 'Hagtøl' ),
	'Randompage'                => array( 'Tilvildarlig_síða' ),
	'Lonelypages'               => array( 'Foreldraleysar_síður' ),
	'Uncategorizedpages'        => array( 'Óbólkaðar_síður' ),
	'Uncategorizedcategories'   => array( 'Óbólkaðir_bólkar' ),
	'Uncategorizedimages'       => array( 'Óbólkaðar_myndir' ),
	'Uncategorizedtemplates'    => array( 'Óbólkaðar_fyrimyndir' ),
	'Unusedcategories'          => array( 'Óbrúktir_bólkar' ),
	'Unusedimages'              => array( 'Óbrúktar_myndir' ),
	'Wantedpages'               => array( 'Ynsktar_síður' ),
	'Mostcategories'            => array( 'Greinir_við_flest_bólkum' ),
	'Mostrevisions'             => array( 'Greinir_við_flest_útgávum' ),
	'Fewestrevisions'           => array( 'Greinir_við_minst_útgávum' ),
	'Shortpages'                => array( 'Stuttar_síður' ),
	'Longpages'                 => array( 'Langar_síður' ),
	'Newpages'                  => array( 'Nýggjar_síður' ),
	'Ancientpages'              => array( 'Elstu_síður' ),
	'Deadendpages'              => array( 'Gøtubotns_síður' ),
	'Allpages'                  => array( 'Allar_síður' ),
	'Ipblocklist'               => array( 'Bannað_brúkaranøvn_og_IP-adressur' ),
	'Specialpages'              => array( 'Serligar_síður' ),
	'Contributions'             => array( 'Brúkaraíkast' ),
	'Emailuser'                 => array( 'Send_t-post_til_brúkara' ),
	'Movepage'                  => array( 'Flyt_síðu' ),
	'Booksources'               => array( 'Bóka_keldur' ),
	'Categories'                => array( 'Bólkar' ),
	'Export'                    => array( 'Útflutningssíður' ),
	'Version'                   => array( 'Útgáva' ),
	'Allmessages'               => array( 'Øll_kervisboð' ),
	'Blockip'                   => array( 'Banna_brúkara' ),
	'Undelete'                  => array( 'Endurstovna_strikaðar_síður' ),
	'Search'                    => array( 'Leita' ),
);

$linkTrail = '/^([áðíóúýæøa-z]+)(.*)$/sDu';

$messages = array(
# User preference toggles
'tog-underline'               => 'Undurstrika ávísingar',
'tog-highlightbroken'         => 'Brúka reyða ávísing til tómar síður',
'tog-justify'                 => 'Stilla greinpart',
'tog-hideminor'               => 'Goym minni broytingar í seinast broytt listanum',
'tog-extendwatchlist'         => 'Víðkað eftirlit',
'tog-usenewrc'                => 'víðka seinastu broytingar lista (ikki til alla kagarar)',
'tog-numberheadings'          => 'Sjálvtalmerking av yvirskrift',
'tog-showtoolbar'             => 'Vís amboðslinju í rætting',
'tog-editondblclick'          => 'Rætta síðu við at tvíklikkja (JavaScript)',
'tog-editsection'             => 'Rætta greinpart við hjálp av [rætta]-ávísing',
'tog-editsectiononrightclick' => 'Rætta greinpart við at høgraklikkja á yvirskrift av greinparti (JavaScript)',
'tog-showtoc'                 => 'Vís innihaldsyvurlit (Til greinir við meira enn trimun greinpartum)',
'tog-rememberpassword'        => 'Minst til loyniorð næstu ferð',
'tog-editwidth'               => 'Rættingarkassin hevur fulla breid',
'tog-watchcreations'          => 'Legg síður, sum eg stovni, í mítt eftirlit',
'tog-watchdefault'            => 'Vaka yvur nýggjum og broyttum greinum',
'tog-minordefault'            => 'Merk sum standard allar broytingar sum smærri',
'tog-previewontop'            => 'Vís forhondsvísning áðren rættingarkassan',
'tog-previewonfirst'          => 'Sýn forskoðan við fyrstu broyting',
'tog-nocache'                 => 'Minst ikki til síðurnar til næstu ferð',
'tog-fancysig'                => 'Rá undirskrift (uttan sjálvvirkandi slóð)',
'tog-externaleditor'          => 'Nýt útvortis ritil sum fyrimynd',
'tog-externaldiff'            => 'Nýt útvortis diff sum fyrimynd',
'tog-showjumplinks'           => 'Ger "far til"-tilgongd virkna',
'tog-forceeditsummary'        => 'Gev mær boð, um eg ikki havi skrivað ein samandrátt um mína rætting',
'tog-watchlisthideown'        => 'Fjal mínar rættingar frá eftirliti',
'tog-watchlisthidebots'       => 'Fjal bot rættingar frá eftirliti',
'tog-watchlisthideminor'      => 'Fjal minni rættingar frá eftirliti',

'underline-always'  => 'Altíð',
'underline-never'   => 'Ongantíð',
'underline-default' => 'Kagarastandard',

'skinpreview' => '(Forskoðan)',

# Dates
'sunday'        => 'sunnudagur',
'monday'        => 'mánadagur',
'tuesday'       => 'týsdagur',
'wednesday'     => 'mikudagur',
'thursday'      => 'hósdagur',
'friday'        => 'fríggjadagur',
'saturday'      => 'leygardagur',
'sun'           => 'sun',
'mon'           => 'mán',
'tue'           => 'týs',
'wed'           => 'mik',
'thu'           => 'hós',
'fri'           => 'frí',
'sat'           => 'ley',
'january'       => 'januar',
'february'      => 'februar',
'march'         => 'mars',
'april'         => 'apríl',
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
'april-gen'     => 'apríl',
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
'pagecategories'         => '{{PLURAL:$1|Bólkur|Bólkar}}',
'category_header'        => 'Greinir í bólki "$1"',
'subcategories'          => 'Undirbólkur',
'category-media-header'  => 'Media í bólkur "$1"',
'category-empty'         => "''Hesin bólkur inniheldur ongar greinir ella miðlar í løtuni.''",
'listingcontinuesabbrev' => 'frh.',

'mainpagetext' => "<big>'''Innlegging av Wiki-ritbúnaði væleydnað.'''</big>",

'about'          => 'Um',
'article'        => 'Innihaldssíða',
'newwindow'      => '(kemur í nýggjan glugga)',
'cancel'         => 'Ógilda',
'qbfind'         => 'Finn',
'qbbrowse'       => 'Kaga',
'qbedit'         => 'Rætta',
'qbpageoptions'  => 'Henda síðan',
'qbpageinfo'     => 'Samanhangur',
'qbmyoptions'    => 'Mínar síður',
'qbspecialpages' => 'Serstakar síður',
'moredotdotdot'  => 'Meira...',
'mypage'         => 'Mín síða',
'mytalk'         => 'Mítt kjak',
'anontalk'       => 'Kjak til hesa ip-adressuna',
'navigation'     => 'Navigatión',
'and'            => 'og',

'errorpagetitle'    => 'Villa',
'returnto'          => 'Vend aftur til $1.',
'tagline'           => 'Frá {{SITENAME}}',
'help'              => 'Hjálp',
'search'            => 'Leita',
'searchbutton'      => 'Leita',
'go'                => 'Far til',
'searcharticle'     => 'Far',
'history'           => 'Síðusøga',
'history_short'     => 'Søga',
'info_short'        => 'Upplýsingar',
'printableversion'  => 'Prentvinarlig útgáva',
'permalink'         => 'Støðug slóð',
'print'             => 'Prenta',
'edit'              => 'Rætta',
'create'            => 'Stovna',
'editthispage'      => 'Rætta hesa síðuna',
'create-this-page'  => 'Stovna hesa síðuna',
'delete'            => 'Strika',
'deletethispage'    => 'Strika hesa síðuna',
'protect'           => 'Friða',
'protectthispage'   => 'Friða hesa síðuna',
'unprotect'         => 'Strika friðing',
'unprotectthispage' => 'Ófriða hesa síðuna',
'newpage'           => 'Nýggj síða',
'talkpage'          => 'Kjakast um hesa síðuna',
'talkpagelinktext'  => 'Kjak',
'specialpage'       => 'Serlig síða',
'personaltools'     => 'Persónlig amboð',
'postcomment'       => 'Skriva eina viðmerking',
'articlepage'       => 'Skoða innihaldssíðuna',
'talk'              => 'Kjak',
'views'             => 'Skoðanir',
'toolbox'           => 'Amboð',
'userpage'          => 'Vís brúkarisíðu',
'projectpage'       => 'Vís verkætlanarsíðu',
'imagepage'         => 'Vís myndasíðu',
'mediawikipage'     => 'Vís kervisboðsíðu',
'templatepage'      => 'Vís fyrimyndsíðu',
'viewhelppage'      => 'Vís hjálpsíðu',
'categorypage'      => 'Vís bólkursíðu',
'viewtalkpage'      => 'Vís kjak',
'otherlanguages'    => 'Onnur mál',
'redirectedfrom'    => '(Ávíst frá $1)',
'redirectpagesub'   => 'Ávísingarsíða',
'lastmodifiedat'    => 'Hendan síðan var seinast broytt $2, $1.', # $1 date, $2 time
'protectedpage'     => 'Friðað síða',
'jumpto'            => 'Far til:',
'jumptonavigation'  => 'navigatión',
'jumptosearch'      => 'leita',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'Um {{SITENAME}}',
'aboutpage'            => 'Project:Um',
'bugreports'           => 'Lúsafrágreiðingar',
'bugreportspage'       => 'Project:Lúsafrágreiðingar',
'copyright'            => 'Innihald er tøkt undir $1.',
'copyrightpagename'    => '{{SITENAME}} útgávurættur',
'copyrightpage'        => '{{ns:project}}:Útgávurættur',
'currentevents'        => 'Núverandi hendingar',
'currentevents-url'    => 'Project:Núverandi hendingar',
'disclaimers'          => 'Fyrivarni',
'disclaimerpage'       => 'Project:Fyrivarni',
'edithelp'             => 'Rættihjálp',
'edithelppage'         => 'Help:Rættihjálp',
'faq'                  => 'OSS',
'faqpage'              => 'Project:OSS',
'helppage'             => 'Help:Innihald',
'mainpage'             => 'Forsíða',
'mainpage-description' => 'Forsíða',
'policy-url'           => 'Project:Handfaring av persónligum upplýsingum',
'portal'               => 'Forsíða fyri høvundar',
'portal-url'           => 'Project:Forsíða fyri høvundar',
'privacy'              => 'Handfaring av persónligum upplýsingum',
'privacypage'          => 'Project:Handfaring av persónligum upplýsingum',

'badaccess' => 'Loyvisbrek',

'ok'                      => 'Í lagi',
'retrievedfrom'           => 'Heinta frá "$1"',
'youhavenewmessages'      => 'Tú hevur $1 ($2).',
'newmessageslink'         => 'nýggj boð',
'newmessagesdifflink'     => 'seinasta broyting',
'youhavenewmessagesmulti' => 'Tú hevur nýggj boð á $1',
'editsection'             => 'rætta',
'editold'                 => 'rætta',
'viewsourceold'           => 'vís keldu',
'editsectionhint'         => 'Rætta part: $1',
'toc'                     => 'Innihaldsyvirlit',
'showtoc'                 => 'skoða',
'hidetoc'                 => 'fjal',
'thisisdeleted'           => 'Sí ella endurstovna $1?',
'viewdeleted'             => 'Vís $1?',
'restorelink'             => '{{PLURAL:$1|strikaða rætting|$1 strikaðar rættingar}}',
'feedlinks'               => 'Føðing:',
'site-rss-feed'           => '$1 RSS Fóðurið',
'site-atom-feed'          => '$1 Atom Fóðurið',
'page-rss-feed'           => '"$1" RSS Feed',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Grein',
'nstab-user'      => 'Brúkarasíða',
'nstab-media'     => 'Miðil',
'nstab-special'   => 'Serstøk',
'nstab-project'   => 'Verkætlanarsíða',
'nstab-image'     => 'Mynd',
'nstab-mediawiki' => 'Grein',
'nstab-template'  => 'Formur',
'nstab-help'      => 'Hjálp',
'nstab-category'  => 'Flokkur',

# Main script and global functions
'nosuchaction'      => 'Ongin slík gerð',
'nosuchactiontext'  => 'Gerðin, ið tilskilað er í url, er ikki
afturkend av wiki',
'nosuchspecialpage' => 'Ongin slík serlig síða',
'nospecialpagetext' => "<big>'''Tú hevur biðið um eina serliga síðu, sum wiki ikki kennir aftur.'''</big>

<!-- A list of valid special pages can be found at [[Special:SpecialPages]]. -->",

# General errors
'error'             => 'Villa',
'databaseerror'     => 'Villa í dátagrunni',
'internalerror'     => 'Innvortis brek',
'filecopyerror'     => 'Kundi ikki avrita fíluna "$1" til "$2".',
'filerenameerror'   => 'Kundi ikki umdoypa fílu "$1" til "$2".',
'filedeleteerror'   => 'Kundi ikki strika fíluna "$1".',
'filenotfound'      => 'Kundi ikki finna fílu "$1".',
'badarticleerror'   => 'Hendan gerðin kann ikki fremjast á hesi síðu.',
'cannotdelete'      => 'Síðan ella myndin kundi ikki strikast. (Møguliga hevur onkur annar longu strikað hana.)',
'badtitle'          => 'Ógyldugt heiti',
'badtitletext'      => 'Umbidna síðan er ógyldugt, tómt ella skeivt tilslóðað heiti millum mál ella wikur.',
'perfdisabled'      => 'Tíverri er hesin hentleikin fyribils óvirkin! Hetta tí at hann seinkar dátugrunnin so nígv, at wiki ikki virkar sum hon skal.',
'perfcachedts'      => 'Fylgjandi dáta er goymt, og var seinast goymt $1.',
'viewsource'        => 'Vís keldu',
'viewsourcefor'     => 'fyri $1',
'protectedpagetext' => 'Hendan síða er læst fyri at steðga rættingum.',
'viewsourcetext'    => 'Tú kanst síggja og avrita kelduna til hesa grein:',

# Login and logout pages
'logouttitle'                => 'Brúkaraútritan',
'logouttext'                 => '<strong>Tú hevur nú ritað út.</strong><br />
Tú kanst halda áfram at nýta {{SITENAME}} dulnevnt.
Ella kanst tú rita inn aftur sum sami ella annar brúkari.
Legg til merkis at summar síður kunnu framhaldandi síggja út
sum tú hevur ritað inn til goymslan í sneytara tínum er ruddað.',
'welcomecreation'            => '== Vælkomin, $1! ==

Tín konto er nú stovnað. Gloym ikki at broyta tínar {{SITENAME}} innstillingar.',
'loginpagetitle'             => 'Brúkarainnritan',
'yourname'                   => 'Títt brúkaranavn:',
'yourpassword'               => 'Títt loyniorð:',
'yourpasswordagain'          => 'Skriva loyniorð umaftur:',
'remembermypassword'         => 'Minst til loyniorðið hjá mær.',
'loginproblem'               => '<b>Trupulleikar vóru við tíni innritan.</b><br />Royn aftur!',
'login'                      => 'Rita inn',
'nav-login-createaccount'    => 'Stovna kontu ella rita inn',
'loginprompt'                => 'Cookies má verða sett til fyri at innrita á {{SITENAME}}.',
'userlogin'                  => 'Stovna kontu ella rita inn',
'logout'                     => 'Útrita',
'userlogout'                 => 'Rita út',
'notloggedin'                => 'Ikki ritað inn',
'nologin'                    => 'Hevur tú ikki eina kontu? $1.',
'nologinlink'                => 'Stovna eina kontu',
'createaccount'              => 'Stovna nýggja kontu',
'gotaccount'                 => 'Hevur tú longu eina kontu? $1.',
'gotaccountlink'             => 'Rita inn',
'createaccountmail'          => 'eftur t-posti',
'badretype'                  => 'Loyniorðið tú hevur skriva er ikki rætt.',
'youremail'                  => 'T-postur (sjálvboðið)*:',
'username'                   => 'Brúkaranavn:',
'uid'                        => 'Brúkara ID:',
'yourrealname'               => 'Títt navn*:',
'yourlanguage'               => 'Mál til brúkaraflatu:',
'yournick'                   => 'Títt eyknevni (til undirskriftir):',
'email'                      => 'T-post',
'loginerror'                 => 'Innritanarbrek',
'prefs-help-email'           => 'T-postur (valfríður): Loyvir øðrum at seta seg í samband við teg gjøgnum brúkara tín ella brúkarakjaksíðu uttan at avdúka samleika tín.',
'noname'                     => 'Tú hevur ikki skrivað eitt gyldugt brúkaranavn.',
'loginsuccesstitle'          => 'Innritan væleydnað',
'loginsuccess'               => "'''Tú hevur nú ritað inn í {{SITENAME}} sum \"\$1\".'''",
'nosuchuser'                 => 'Eingin brúkari er við navninum "$1". Kanna stavseting ella nýt frymilin niðanfyri til at stovna nýggja kontu.',
'nosuchusershort'            => 'Eingin brúkari er við navninum "<nowiki>$1</nowiki>". Kanna stavseting.',
'wrongpassword'              => 'Loyniorðið, sum tú skrivaði, er skeivt. Vinaliga royn aftur.',
'wrongpasswordempty'         => 'Loyniorð manglar. Vinarliga royn aftur.',
'mailmypassword'             => 'Send mær eitt nýtt loyniorð',
'passwordremindertitle'      => 'Loyniorðsámining frá {{SITENAME}}',
'passwordsent'               => 'Eitt nýtt loyniorð er sent til t-postadressuna,
sum er skrásett fyri "$1".
Vinarliga rita inn eftir at tú hevur fingið hana.',
'acct_creation_throttle_hit' => 'Tíverri hevur tú longu stovnað $1 kontur. Tú kanst ikki stovna fleiri.',
'emailauthenticated'         => 'Tín t-post adressa fekk gildi $1.',
'emailnotauthenticated'      => 'Tín t-post adressa er enn ikki komin í gildi. Ongin t-postur
verður sendur fyri nakað av fylgjandi hentleikum.',
'emailconfirmlink'           => 'Vátta tína t-post adressu',
'accountcreated'             => 'Konto upprættað',
'loginlanguagelabel'         => 'Mál: $1',

# Edit page toolbar
'bold_sample'     => 'Feitir stavir',
'bold_tip'        => 'Feitir stavir',
'italic_sample'   => 'Skákstavir',
'italic_tip'      => 'Skákstavir',
'link_sample'     => 'Slóðarheiti',
'link_tip'        => 'Innanhýsis slóð',
'extlink_sample'  => 'http://www.example.com slóðarheiti',
'extlink_tip'     => 'Útvortis slóð (minst til http:// forskoytið)',
'headline_sample' => 'Yvirskriftartekstur',
'headline_tip'    => 'Annars stigs yvirskrift',
'math_sample'     => 'Set formil her',
'math_tip'        => 'Støddfrøðiligur formil (LaTeX)',
'nowiki_tip'      => 'Ignorera wiki-forsniðan',
'image_sample'    => 'Dømi.jpg',
'image_tip'       => 'Innset mynd',
'media_sample'    => 'Dømi.ogg',
'media_tip'       => 'Miðlafíluslóð',
'sig_tip'         => 'Tín undirskrift við tíðarstempli',
'hr_tip'          => 'Vatnrøtt linja (vera sparin við)',

# Edit pages
'summary'                  => 'Samandráttur',
'subject'                  => 'Evni/heiti',
'minoredit'                => 'Hetta er smábroyting',
'watchthis'                => 'Hav eftirlit við hesi síðuni',
'savearticle'              => 'Goym síðu',
'preview'                  => 'Forskoðan',
'showpreview'              => 'Forskoðan',
'showlivepreview'          => 'Beinleiðis forskoðan',
'showdiff'                 => 'Sýn broytingar',
'anoneditwarning'          => "'''Ávaring:''' Tú hevur ikki ritað inn.
Tín IP-adressa verður goymd í rættisøguni fyri hesa síðuna.",
'summary-preview'          => 'Samandráttaforskoðan',
'blockedtitle'             => 'Brúkarin er bannaður',
'loginreqtitle'            => 'Innritan kravd',
'loginreqlink'             => 'rita inn',
'accmailtitle'             => 'Loyniorð sent.',
'accmailtext'              => 'Loyniorð fyri "$1" er sent til $2.',
'newarticle'               => '(Nýggj)',
'newarticletext'           => "Tú ert komin eftir eini slóð til eina síðu, ið ikki er til enn. Skriva í kassan niðanfyri, um tú vilt byrja uppá hesa síðuna.
(Sí [[{{MediaWiki:Helppage}}|hjálparsíðuna]] um tú ynskir fleiri upplýsingar).
Ert tú komin higar av einum mistaki, kanst tú trýsta á '''aftur'''-knøttin á kagaranum.",
'anontalkpagetext'         => "----''Hetta er ein kjaksíða hjá einum dulnevndum brúkara, sum ikki hevur stovnað eina kontu enn, ella ikki brúkar hana. Tí noyðast vit at brúka nummerisku IP-adressuna hjá honum ella henni.
Ein slík IP-adressa kann verða brúkt av fleiri brúkarum.
Ert tú ein dulnevndur brúkari, og kennir, at óvikomandi viðmerkingar eru vendar til tín, so vinarliga [[Special:UserLogin|stovna eina kontu]] fyri at sleppa undan samanblanding við aðrar dulnevndar brúkarar í framtíðini.''",
'clearyourcache'           => "'''Viðmerking:''' Eftir at hava goymt mást tú fara uttanum minnið á sneytara tínum fyri at síggja broytingarnar. '''Mozilla/Safari/Konqueror:''' halt knøttinum ''Shift'' niðri meðan tú trýstir á ''Reload'' (ella trýst ''Ctrl-Shift-R''), '''IE:''' trýst ''Ctrl-F5'', '''Opera:''' trýst F5.",
'note'                     => '<strong>Viðmerking:</strong>',
'previewnote'              => '<strong>Minst til at hetta bara er ein forskoðan, sum enn ikki er goymd!</strong>',
'previewconflict'          => 'Henda forskoðanin vísir tekstin í erva soleiðis sum hann sær út, um tú velur at goyma.',
'editing'                  => 'Tú rættar $1',
'editingsection'           => 'Tú rættar $1 (partur)',
'editingcomment'           => 'Tú rættar $1 (viðmerking)',
'yourtext'                 => 'Tín tekstur',
'storedversion'            => 'Goymd útgáva',
'yourdiff'                 => 'Munir',
'copyrightwarning'         => "Alt íkast til {{SITENAME}} er útgivið undir $2 (sí $1 fyri smálutir). Vilt tú ikki hava skriving tína broytta miskunnarleyst og endurspjadda frítt, so send hana ikki inn.<br />
Við at senda arbeiði títt inn, lovar tú, at tú hevur skrivað tað, ella at tú hevur avritað tað frá tilfeingi ið er almenn ogn &mdash; hetta umfatar '''ikki''' flestu vevsíður.
<strong>SEND IKKI UPPHAVSRÆTTARVART TILFAR UTTAN LOYVI!</strong>",
'longpagewarning'          => '<strong>ÁVARING: Henda síðan er $1 kilobýt long.
Summir sneytarar kunnu hava trupulleikar við at viðgerða síður upp ímóti ella longri enn 32kb.
Vinarliga umhugsa at býta síðuna sundur í styttri pettir.</strong>',
'protectedpagewarning'     => '<strong>ÁVARING: Henda síðan er friðað, so at einans brúkarar við umboðsstjóraheimildum kunnu broyta hana.</strong>',
'semiprotectedpagewarning' => "'''Viðmerking:''' Hendan grein er læst soleiðis at bert skrásetir brúkaris kunnu rætta hana.",
'templatesused'            => 'Fyrimyndir brúktar á hesu síðu:',
'templatesusedpreview'     => 'Fyrimyndir brúktar í hesari forskoðan:',
'template-protected'       => '(friðað)',

# History pages
'viewpagelogs'        => 'Sí logg fyri hesa grein',
'nohistory'           => 'Eingin broytisøga er til hesa síðuna.',
'currentrev'          => 'Núverandi endurskoðan',
'revisionasof'        => 'Endurskoðan frá $1',
'previousrevision'    => '←Eldri endurskoðan',
'nextrevision'        => 'Nýggjari endurskoðan→',
'currentrevisionlink' => 'Skoða verandi endurskoðan',
'cur'                 => 'nú',
'next'                => 'næst',
'last'                => 'síðst',
'page_first'          => 'fyrsta',
'page_last'           => 'síðsta',
'histlegend'          => 'Frágreiðing:<br />
(nú) = munur til núverandi útgávu,
(síðst) = munur til síðsta útgávu, m = minni rættingar',
'deletedrev'          => '[strikað]',
'histfirst'           => 'Elsta',
'histlast'            => 'Nýggjasta',
'historysize'         => '({{PLURAL:$1|1 být|$1 být}})',
'historyempty'        => '(tóm)',

# Revision deletion
'rev-delundel' => 'skoða/fjal',

# History merging
'mergehistory-from' => 'Keldusíða:',

# Diffs
'difference'              => '(Munur millum endurskoðanir)',
'lineno'                  => 'Linja $1:',
'compareselectedversions' => 'Bera saman valdar útgávur',
'editundo'                => 'afturstilla',

# Search results
'searchresults'         => 'Leitúrslit',
'searchresulttext'      => 'Ynskir tú fleiri upplýsingar um leiting á {{SITENAME}}, kanst tú skoða [[{{MediaWiki:Helppage}}|{{int:help}}]].',
'searchsubtitle'        => "Tú leitaði eftur '''[[:$1]]'''",
'searchsubtitleinvalid' => "Tú leitaði eftur '''$1'''",
'noexactmatch'          => "'''Eingin síða við heitinum \"\$1\" er til.''' Tú kanst [[:\$1|byrja at skriva eina grein við hesum heitinum]].",
'notitlematches'        => 'Onki síðuheiti samsvarar',
'notextmatches'         => 'Ongin síðutekstur samsvarar',
'prevn'                 => 'undanfarnu $1',
'nextn'                 => 'næstu $1',
'viewprevnext'          => 'Vís ($1) ($2) ($3).',
'search-result-size'    => '$1 ({{PLURAL:$2|1 orð|$2 orð}})',
'showingresults'        => "Niðanfyri standa upp til {{PLURAL:$1|'''$1''' úrslit, sum byrjar|'''$1''' úrslit, sum byrja}} við #<b>$2</b>.",
'showingresultsnum'     => "Niðanfyri standa {{PLURAL:$3|'''1''' úrslit, sum byrjar|'''$3''' úrslit, sum byrja}} við #<b>$2</b>.",
'powersearch'           => 'Leita',

# Preferences page
'preferences'             => 'Innstillingar',
'mypreferences'           => 'Mínar innstillingar',
'prefsnologin'            => 'Tú hevur ikki ritað inn',
'qbsettings'              => 'Skundfjøl innstillingar',
'qbsettings-none'         => 'Eingin',
'qbsettings-fixedleft'    => 'Fast vinstru',
'qbsettings-fixedright'   => 'Fast høgru',
'qbsettings-floatingleft' => 'Flótandi vinstru',
'changepassword'          => 'Broyt loyniorð',
'skin'                    => 'Hamur',
'math'                    => 'Støddfrøðiligir formlar',
'dateformat'              => 'Dato forsnið',
'datetime'                => 'Dato og tíð',
'prefs-personal'          => 'Brúkaradáta',
'prefs-rc'                => 'Nýkomnar broytingar og stubbaskoðan',
'prefs-watchlist'         => 'Eftirlit',
'prefs-watchlist-days'    => 'Tal av døgum, sum skula vísast í eftirliti:',
'prefs-watchlist-edits'   => 'Tal av rættingum, sum skula vísast í víðkaðum eftirliti:',
'prefs-misc'              => 'Ymiskar innstillingar',
'saveprefs'               => 'Goym innstillingar',
'resetprefs'              => 'Endurset innstillingar',
'oldpassword'             => 'Gamalt loyniorð:',
'newpassword'             => 'Nýtt loyniorð:',
'retypenew'               => 'Skriva nýtt loyniorð umaftur:',
'textboxsize'             => 'Broyting av greinum',
'rows'                    => 'Røð:',
'columns'                 => 'Teigar:',
'searchresultshead'       => 'Leita',
'resultsperpage'          => 'Úrslit fyri hvørja síðu:',
'contextlines'            => 'Linjur fyri hvørt úrslit:',
'contextchars'            => 'Tekin fyri hvørja linju í úrslitinum:',
'recentchangescount'      => 'Heiti í seinastu broytingum:',
'savedprefs'              => 'Tínar innstillingar eru goymdar.',
'timezonelegend'          => 'Lokal tíð',
'timezonetext'            => '¹Talið av tímum, ið tín lokala tíð víkir frá ambætaratíð (UTC).',
'localtime'               => 'Lokal klokka',
'timezoneoffset'          => 'Frávik¹',
'servertime'              => 'Ambætaraklokkan er nú',
'guesstimezone'           => 'Fyll út við kagara',
'allowemail'              => 'Tilset t-post frá øðrum brúkarum',
'defaultns'               => 'Leita í hesum navnarúminum sum fyrisett mál:',
'files'                   => 'Fílur',

# User rights
'saveusergroups' => 'Goym brúkaraflokk',

# Groups
'group'            => 'Bólkur:',
'group-bot'        => 'Bottar',
'group-sysop'      => 'Umboðsstjórar',
'group-bureaucrat' => 'Embætismenn',
'group-all'        => '(allir)',

'group-bot-member'        => 'Bottur',
'group-sysop-member'      => 'Umboðsstjóri',
'group-bureaucrat-member' => 'Embætismaður',

'grouppage-bot'        => '{{ns:project}}:Bottar',
'grouppage-sysop'      => '{{ns:project}}:Umboðsstjórar',
'grouppage-bureaucrat' => '{{ns:project}}:Embætismenn',

# Recent changes
'nchanges'          => '$1 {{PLURAL:$1|broyting|broytingar}}',
'recentchanges'     => 'Seinastu broytingar',
'rcnote'            => "Niðanfyri {{PLURAL:$1|stendur '''1''' tann seinasta broytingin|standa '''$1''' tær seinastu broytingarnar}} {{PLURAL:$2|seinasta dagin|seinastu '''$2''' dagarnar}}, frá $3.",
'rcnotefrom'        => "Niðanfyri standa broytingarnar síðani '''$2''', (upp til '''$1''' er sýndar).",
'rclistfrom'        => 'Sýn nýggjar broytingar byrjandi við $1',
'rcshowhideminor'   => '$1 minni rættingar',
'rcshowhidebots'    => '$1 bottar',
'rcshowhideliu'     => '$1 skrásettar brúkarar',
'rcshowhideanons'   => '$1 navnleysar brúkarar',
'rcshowhidemine'    => '$1 mínar rættingar',
'rclinks'           => 'Sýn seinastu $1 broytingarnar seinastu $2 dagarnar<br />$3',
'diff'              => 'munur',
'hist'              => 'søga',
'hide'              => 'fjal',
'show'              => 'Skoða',
'minoreditletter'   => 's',
'newpageletter'     => 'N',
'boteditletter'     => 'b',
'rc_categories_any' => 'Nakar',

# Recent changes linked
'recentchangeslinked' => 'Viðkomandi broytingar',

# Upload
'upload'            => 'Legg fílu upp',
'uploadbtn'         => 'Legg fílu upp',
'uploadnologin'     => 'Ikki ritað inn',
'uploadnologintext' => 'Tú mást hava [[Special:UserLogin|ritað inn]]
fyri at leggja fílur upp.',
'uploadlog'         => 'fílu logg',
'uploadlogpage'     => 'Fílugerðabók',
'filename'          => 'Fílunavn',
'filedesc'          => 'Samandráttur',
'fileuploadsummary' => 'Samandráttur:',
'filestatus'        => 'Upphavsrættar støða:',
'filesource'        => 'Kelda:',
'uploadedfiles'     => 'Upplagdar fílur',
'ignorewarnings'    => 'Ikki vísa ávaringar',
'badfilename'       => 'Myndin er umnevnd til "$1".',
'successfulupload'  => 'Upplegging væleydnað',
'savefile'          => 'Goym fílu',
'uploadedimage'     => 'sent "[[$1]]" upp',
'sourcefilename'    => 'Keldufílunavn:',
'destfilename'      => 'Málfílunavn:',
'watchthisupload'   => 'Hav eftirlit við hesi síðuni',

'upload-file-error' => 'Innvortis brek',

'license'   => 'Loyvi:',
'nolicense' => 'Onki valt',

# Special:ImageList
'imagelist'      => 'Myndalisti',
'imagelist_name' => 'Navn',
'imagelist_user' => 'Brúkari',

# Image description page
'filehist'          => 'Søga fílu',
'filehist-current'  => 'streymur',
'filehist-datetime' => 'Dagur/Tíð',
'filehist-user'     => 'Brúkari',
'filehist-filesize' => 'Stødd fílu',
'filehist-comment'  => 'Viðmerking',
'imagelinks'        => 'Slóðir',
'linkstoimage'      => 'Hesar síður slóða til hesa mynd:',
'nolinkstoimage'    => 'Ongar síður slóða til hesa myndina.',
'sharedupload'      => 'This file is a shared upload and may be used by other projects.',

# File deletion
'filedelete'        => 'Strika $1',
'filedelete-submit' => 'Strika',

# MIME search
'mimesearch' => 'MIME-leit',

# List redirects
'listredirects' => 'Sýn ávísingar',

# Unused templates
'unusedtemplates'    => 'Óbrúktar fyrimyndir',
'unusedtemplateswlh' => 'aðrar slóðir',

# Random page
'randompage' => 'Tilvildarlig síða',

# Random redirect
'randomredirect' => 'Tilvildarlig ávísingarsíða',

# Statistics
'statistics'    => 'Hagtøl',
'sitestats'     => '{{SITENAME}} síðuhagtøl',
'userstats'     => 'Brúkarahagtøl',
'sitestatstext' => "Tilsamans {{PLURAL:$1|'''1''' síða er|'''$1''' síður eru}} í dátugrunninum.
Hetta umfatar kjaksíður, síður um {{SITENAME}}, heilt stuttar stubbasíður,
ávísingar og aðrar, sum helst ikki kunnu metast sum innihaldssíður.
Verða tær tiknar burtur úr, {{PLURAL:$2|er '''1''' síða|eru '''$2''' síður}}, sum kunnu metast sum
{{PLURAL:$2|innihaldssíða|innihaldssíður}}.

<!--'''$8''' {{PLURAL:$8|file has|files have}} been uploaded.-->

Tilsamans '''$3''' {{PLURAL:$3|síðuskoðan hevur|síðuskoðanir hava}} verið og '''$4''' {{PLURAL:$4|síðubroyting|síðubroytingar}}
síðani henda wikan varð sett up.
Tað gevur í miðal '''$5''' broytingar fyri hvørja síðu og '''$6''' skoðanir fyri hvørja broyting.

<!--The [http://www.mediawiki.org/wiki/Manual:Job_queue job queue] length is '''$7'''.-->",
'userstatstext' => "Tilsamans  {{PLURAL:$1|er '''1''' skrásettur [[Special:ListUsers|brúkari]]|eru '''$1''' skrásettir [[Special:ListUsers|brúkarar]]}}. '''$2''' (ella '''$4%''') av hesum {{PLURAL:$2|er umboðsstjóri|eru umboðsstjórar}} (sí $5).",

'disambiguations'     => 'Síður við fleirfaldum týdningi',
'disambiguationspage' => 'Template:fleiri týdningar',

'doubleredirects'     => 'Tvífaldað ávísing',
'doubleredirectstext' => '<b>Gevið gætur:</b> Hetta yvirlitið kann innihalda skeiv úrslit. Tað merkir vanliga at síðan hevur eyka tekst niðanfyri fyrsta #REDIRECT.<br />
Hvørt rað inniheldur slóðir til fyrstu og aðru ávísing, umframt tekstin á fyrstu reglu í aðru ávísing, sum vanliga vísir til "veruligu" málsíðuna, sum fyrsta ávísingin eigur at vísa til.',

'brokenredirects'        => 'Brotnar ávísingar',
'brokenredirectstext'    => 'Hesar ávísingarnar slóða til síður, ið ikki eru til.',
'brokenredirects-edit'   => '(rætta)',
'brokenredirects-delete' => '(strika)',

'withoutinterwiki'         => 'Síður uttan málslóðir',
'withoutinterwiki-summary' => 'Fylgjandi síður slóða ikki til útgávur á øðrum málum:',
'withoutinterwiki-submit'  => 'Skoða',

'fewestrevisions' => 'Greinir við minstum útgávum',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|být|být}}',
'ncategories'             => '$1 {{PLURAL:$1|bólkur|bólkar}}',
'nlinks'                  => '$1 {{PLURAL:$1|slóð|slóðir}}',
'nmembers'                => '$1 {{PLURAL:$1|limur|limir}}',
'nviews'                  => '$1 {{PLURAL:$1|skoðan|skoðanir}}',
'lonelypages'             => 'Foreldraleysar síður',
'uncategorizedpages'      => 'Óbólkaðar síður',
'uncategorizedcategories' => 'Óbólkaðir bólkar',
'unusedimages'            => 'Óbrúktar fílur',
'popularpages'            => 'Umtóktar síður',
'wantedcategories'        => 'Ynsktir bólkar',
'wantedpages'             => 'Ynsktar síður',
'mostcategories'          => 'Greinir við flest bólkum',
'mostrevisions'           => 'Greinir við flestum útgávum',
'shortpages'              => 'Stuttar síður',
'longpages'               => 'Langar síður',
'deadendpages'            => 'Gøtubotnssíður',
'protectedpages'          => 'Friðaðar síður',
'listusers'               => 'Brúkaralisti',
'newpages'                => 'Nýggjar síður',
'newpages-username'       => 'Brúkaranavn:',
'ancientpages'            => 'Elstu síður',
'move'                    => 'Flyt',
'movethispage'            => 'Flyt hesa síðuna',
'unusedimagestext'        => 'Vinarliga legg merki til, at vevsíður kunnu slóða til eina mynd við beinleiðis URL, so hon kann síggjast her hóast at hon er í regluligari nýtslu.',
'notargettitle'           => 'Onki mál',

# Book sources
'booksources'    => 'Bókakeldur',
'booksources-go' => 'Far',

# Special:Log
'specialloguserlabel'  => 'Brúkari:',
'speciallogtitlelabel' => 'Heitið:',
'log'                  => 'Gerðabøkur',
'all-logs-page'        => 'Allar gerðabøkur',
'log-search-submit'    => 'Far',
'alllogstext'          => 'Samansett sýning av upplegging, striking, friðing, forðing og sysop-gerðabókum.
Tú kanst avmarka sýningina við at velja gerðabókaslag, brúkaranavn ella ávirkaðu síðuna.',

# Special:AllPages
'allpages'       => 'Allar síður',
'alphaindexline' => '$1 til $2',
'nextpage'       => 'Næsta síða ($1)',
'prevpage'       => 'Fyrrverandi síða ($1)',
'allarticles'    => 'Allar greinir',
'allinnamespace' => 'Allar síður ($1 navnarúm)',
'allpagesprev'   => 'Undanfarnu',
'allpagesnext'   => 'Næstu',
'allpagessubmit' => 'Far',

# Special:Categories
'categories'         => 'Bólkar',
'categoriespagetext' => 'Eftirfylgjandi bólkar eru í hesu wiki.',

# Special:ListUsers
'listusersfrom'      => 'Vís brúkarar ið byrja við:',
'listusers-submit'   => 'Sýna',
'listusers-noresult' => 'Ongin brúkari var funnin.',

# E-mail user
'mailnologintext' => 'Tú mást hava [[Special:UserLogin|ritað inn]]
og hava virkandi teldupostadressu í [[Special:Preferences|innstillingum]] tínum
fyri at senda teldupost til aðrar brúkarar.',
'emailuser'       => 'Send t-post til brúkara',
'emailpage'       => 'Send t-post til brúkara',
'defemailsubject' => '{{SITENAME}} t-postur',
'noemailtitle'    => 'Ongin t-post adressa',
'noemailtext'     => 'Hesin brúkarin hevur ikki upplýst eina gylduga t-post-adressu,
ella hevur hann valt ikki at taka ímóti t-posti frá øðrum brúkarum.',
'emailfrom'       => 'Frá',
'emailto'         => 'Til',
'emailsubject'    => 'Evni',
'emailmessage'    => 'Boð',
'emailsent'       => 'T-postur sendur',
'emailsenttext'   => 'Títt t-post boð er sent.',

# Watchlist
'watchlist'            => 'Mítt eftirlit',
'mywatchlist'          => 'Mítt eftirlit',
'watchlistfor'         => "(fyri '''$1''')",
'nowatchlist'          => 'Tú hevur ongar lutir í eftirlitinum.',
'watchnologin'         => 'Tú hevur ikki ritað inn',
'addedwatch'           => 'Lagt undir eftirlit',
'addedwatchtext'       => "Síðan \"<nowiki>\$1</nowiki>\" er løgd undir [[Special:Watchlist|eftirlit]] hjá tær.
Framtíðar broytingar á hesi síðu og tilknýttu kjaksíðuni verða at síggja her.
Tá sæst síðan sum '''feit skrift''' í [[Special:RecentChanges|broytingaryvirlitinum]] fyri at gera hana lættari at síggja.

Vilt tú flyta síðuna undan tínum eftirliti, kanst tú trýsta á \"Strika eftirlit\" á síðuni.",
'removedwatch'         => 'Strikað úr eftirliti',
'removedwatchtext'     => 'Síðan "[[:$1]]" er strikað úr tínum eftirliti.',
'watch'                => 'Eftirlit',
'watchthispage'        => 'Hav eftirlit við hesi síðuni',
'unwatch'              => 'strika eftirlit',
'notanarticle'         => 'Ongin innihaldssíða',
'watchnochange'        => 'Ongin grein í tínum eftirliti er rætta innanfyri hetta tíðarskeiði.',
'watchmethod-list'     => 'kannar síður undir eftirliti fyri feskar broytingar',
'watchlistcontains'    => 'Títt eftirlit inniheldur {{PLURAL:$1|eina síðu|$1 síður}}.',
'wlnote'               => "Niðanfyri {{PLURAL:$1|stendur seinastu broytingina|standa seinastu '''$1''' broytingarnar}} {{PLURAL:$2|seinasta tíman|seinastu '''$2''' tímarnar}}.",
'wlshowlast'           => 'Vís seinastu $1 tímar $2 dagar $3',
'watchlist-show-bots'  => 'Vís bot rættingar',
'watchlist-hide-bots'  => 'Fjal bottarættingar',
'watchlist-show-own'   => 'Vís mínar rættingar',
'watchlist-hide-own'   => 'Fjal mínar rættingar',
'watchlist-show-minor' => 'Vís minni rættingar',
'watchlist-hide-minor' => 'Fjal minni rættingar',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Eftirlitir...',
'unwatching' => 'Strikar eftirlit...',

'enotif_newpagetext'           => 'Hetta er ein nýggj síða.',
'enotif_impersonal_salutation' => '{{SITENAME}}brúkari',
'created'                      => 'stovnað',

# Delete/protect/revert
'deletepage'          => 'Strika síðu',
'confirm'             => 'Vátta',
'excontent'           => "innihald var: '$1'",
'excontentauthor'     => "innihaldið var: '$1' (og einasti rithøvundur var '[[Special:Contributions/$2|$2]]')",
'exblank'             => 'síðan var tóm',
'historywarning'      => 'Ávaring: Síðan, ið tú ert í gongd við at strika, hevur eina søgu:',
'confirmdeletetext'   => 'Tú ert í gongd við endaliga at strika ein a síðu
ella mynd saman við allari søgu úr dátugrunninum.
Vinarliga vátta at tú ætlar at gera hetta, at tú skilur
avleiðingarnar og at tú gert tað í tráð við
[[{{MediaWiki:Policy-url}}]].',
'actioncomplete'      => 'Verkið er fullgjørt',
'deletedtext'         => '"<nowiki>$1</nowiki>" er nú strikað.
Sí $2 fyri fulla skráseting av strikingum.',
'deletedarticle'      => 'strikaði "[[$1]]"',
'dellogpage'          => 'Striku logg',
'deletionlog'         => 'striku logg',
'deletecomment'       => 'Orsøk til striking:',
'rollback'            => 'Rulla broytingar aftur',
'rollback_short'      => 'Rulla aftur',
'rollbacklink'        => 'afturrulling',
'rollbackfailed'      => 'Afturrulling miseydnað',
'protectlogpage'      => 'Friðingarbók',
'protectedarticle'    => 'friðaði "[[$1]]"',
'unprotectedarticle'  => 'ófriðaði "[[$1]]"',
'protect-title'       => 'Friðar "$1"',
'protect-legend'      => 'Vátta friðing',
'protectcomment'      => 'Orsøk til friðing:',
'protectexpiry'       => 'Gongur út:',
'protect-default'     => '(fyridømi)',
'protect-level-sysop' => 'Bert umboðsstjórar',
'protect-expiring'    => 'gongur út $1 (UTC)',
'restriction-type'    => 'Verndstøða:',
'pagesize'            => '(být)',

# Undelete
'undelete'               => 'Endurstovna strikaðar síður',
'undeletebtn'            => 'Endurstovna',
'undeletereset'          => 'Endurset',
'undeletedarticle'       => 'endurstovnaði "[[$1]]"',
'undeletedfiles'         => '{{PLURAL:$1|1 fíla endurstovna|$1 fílur endurstovnaðar}}',
'undelete-search-submit' => 'Leita',

# Namespace form on various pages
'namespace'      => 'Navnarúm:',
'invert'         => 'Umvend val',
'blanknamespace' => '(Greinir)',

# Contributions
'contributions' => 'Brúkaraíkast',
'mycontris'     => 'Mítt íkast',
'contribsub2'   => 'Eftir $1 ($2)',
'uctop'         => '(ovast)',
'month'         => 'Frá mánaði (og áðrenn):',
'year'          => 'Frá ár (og áðrenn):',

'sp-contributions-newbies'  => 'Vís bert íkast frá nýggjum kontoum',
'sp-contributions-blocklog' => 'Bannagerðabók',
'sp-contributions-search'   => 'Leita eftir íkøstum',
'sp-contributions-username' => 'IP adressa ella brúkaranavn:',
'sp-contributions-submit'   => 'Leita',

# What links here
'whatlinkshere'       => 'Hvat slóðar higar',
'linklistsub'         => '(Listi av slóðum)',
'linkshere'           => "Hesar síður slóða til '''[[:$1]]''':",
'nolinkshere'         => "Ongar síður slóða til '''[[:$1]]'''.",
'isredirect'          => 'ávísingarsíða',
'whatlinkshere-prev'  => '{{PLURAL:$1|fyrrverandi|fyrrverandi $1}}',
'whatlinkshere-next'  => '{{PLURAL:$1|næst|næstu $1}}',
'whatlinkshere-links' => '← slóðir',

# Block/unblock
'blockip'              => 'Banna brúkara',
'ipaddress'            => 'IP-adressa:',
'ipadressorusername'   => 'IP-adressa ella brúkaranavn:',
'ipbreason'            => 'Orsøk:',
'ipbsubmit'            => 'Banna henda brúkaran',
'badipaddress'         => 'Ógyldug IP-adressa',
'blockipsuccesssub'    => 'Banning framd',
'ipb-unblock-addr'     => 'Óbanna $1',
'ipusubmit'            => 'Óbanna hesa adressuna',
'ipblocklist'          => 'Bannað brúkaranøvn og IP-adressur',
'ipblocklist-username' => 'Brúkaranavn ella IP-adressa:',
'ipblocklist-submit'   => 'Leita',
'expiringblock'        => 'gongur út $1',
'blocklink'            => 'banna',
'unblocklink'          => 'óbanna',
'contribslink'         => 'íkøst',
'blocklogpage'         => 'Bannagerðabók',
'unblocklogentry'      => 'óbannaði $1',
'proxyblocksuccess'    => 'Liðugt.',

# Developer tools
'lockdbtext'        => 'At læsa dátugrunnin steðgar møguleikanum hjá øllum
brúkarum at broyta síður, broyta innstillingar sínar, broyta sínar eftirlitslistar og
onnur ting, ið krevja broytingar í dátugrunninum.
Vinarliga vátta, at hetta er tað, ið tú ætlar at gera, og at tú fert
at læsa dátugrunnin upp aftur tá ið viðgerðin er liðug.',
'locknoconfirm'     => 'Tú krossaði ikki váttanarkassan.',
'lockdbsuccesstext' => 'Dátugrunnurin er læstur.
<br />Minst til at [[Special:UnlockDB|læsa upp]] aftur, tá ið viðgerðin er liðug.',

# Move page
'move-page-legend'        => 'Flyt síðu',
'movepagetext'            => "Við frymlinum niðanfyri kanst tú umnevna eina síðu og flyta alla hennara søgu við til nýggja navnið.
Gamla navnið verður ein tilvísingarsíða til ta nýggju.
Slóðirnar til gomlu síðuna verða ikki broyttar.
Ansa eftir at kanna um tvífaldar ella brotnar tilvísingar eru.
Tú hevur ábyrgdina fyri at ansa eftir at slóðir framvegis fara hagar, tær skulu.

Legg merki til at síðan '''ikki''' verður flutt, um ein síða longu er við nýggja navninum, uttan at hon er tóm og onga søgu hevur.
Hetta merkir at tú kanst umnevna eina síðu aftur hagani hon kom, um tú gjørdi eitt mistak.
Tú kanst ikki skriva yvir eina verandi síðu.

'''ÁVARING!'''
Hetta kann vera ein ógvuslig og óvæntað flyting av einari vældámdari síðu.
Vinarliga tryggja tær, at tú skilur avleiðingarnar av hesum áðrenn tú heldur áfam.",
'movearticle'             => 'Flyt síðu:',
'newtitle'                => 'Til nýtt heiti:',
'move-watch'              => 'Hav eftirlit við hesi síðuni',
'movepagebtn'             => 'Flyt síðu',
'pagemovedsub'            => 'Flyting væleydnað',
'articleexists'           => 'Ein síða finst longu við hasum navninum,
ella er navnið tú valdi ógyldugt.
Vinarliga vel eitt annað navn.',
'movedto'                 => 'flyt til',
'movetalk'                => 'Flyt kjaksíðuna eisini, um hon er til.',
'1movedto2'               => '$1 flutt til $2',
'1movedto2_redir'         => '$1 flutt til $2 um ávísing',
'movelogpage'             => 'Flyt gerðabók',
'movereason'              => 'Orsøk:',
'delete_and_move'         => 'Strika og flyt',
'delete_and_move_text'    => '==Striking krevst==

Grein við navninum "[[:$1]]" finst longu. Ynskir tú at strika hana til tess at skapa pláss til flytingina?',
'delete_and_move_confirm' => 'Ja, strika hesa síðuna',
'delete_and_move_reason'  => 'Strika til at gera pláss til flyting',

# Export
'export' => 'Útflutningssíður',

# Namespace 8 related
'allmessages'               => 'Øll kervisboð',
'allmessagesname'           => 'Navn',
'allmessagesdefault'        => 'Enskur tekstur',
'allmessagescurrent'        => 'Verandi tekstur',
'allmessagestext'           => 'Hetta er eitt yvirlit av tøkum kervisboðum í MediaWiki-navnarúmi.
Please visit [http://www.mediawiki.org/wiki/Localisation MediaWiki Localisation] and [http://translatewiki.net Betawiki] if you wish to contribute to the generic MediaWiki localisation.',
'allmessagesnotsupportedDB' => "'''{{ns:special}}:AllMessages''' er ikki stuðlað orsakað av at '''\$wgUseDatabaseMessages''' er sløkt.",
'allmessagesfilter'         => 'Boð navn filtur:',
'allmessagesmodified'       => 'Vís bert broytt',

# Thumbnails
'thumbnail-more' => 'Víðka',
'filemissing'    => 'Fíla vantar',

# Special:Import
'import'                  => 'Innflyt síður',
'import-interwiki-submit' => 'Innflyta',
'importfailed'            => 'Innflutningur miseydnaður: $1',
'importsuccess'           => 'Innflutningur væleydnaður!',

# Tooltip help for the actions
'tooltip-pt-userpage'           => 'Mín brúkarasíða',
'tooltip-pt-mytalk'             => 'Mín kjaksíða',
'tooltip-pt-preferences'        => 'Mínir stillingar',
'tooltip-pt-mycontris'          => 'Yvirlit yvir mítt íkast',
'tooltip-pt-logout'             => 'Rita út',
'tooltip-ca-talk'               => 'Umrøða av innihaldssíðuni',
'tooltip-ca-edit'               => 'Tú kanst broyta hesa síðuna. Vinarliga nýt forskoðanarknøttin áðrenn tú goymir.',
'tooltip-ca-addsection'         => 'Skriva viðmerking til hesa umrøðuna.',
'tooltip-ca-viewsource'         => 'Henda síðan er friðað. Tú kanst síggja keldukotuna.',
'tooltip-ca-history'            => 'Fyrrverandi útgávur av hesi síðu.',
'tooltip-ca-protect'            => 'Friða hesa síðuna',
'tooltip-ca-delete'             => 'Strika hesa síðuna',
'tooltip-ca-undelete'           => 'Endurnýggja skrivingina á hesi síðu áðrenn hon varð strikað',
'tooltip-ca-move'               => 'Flyt hesa síðuna',
'tooltip-ca-watch'              => 'Legg hesa síðuna undir mítt eftirlit',
'tooltip-ca-unwatch'            => 'Fá hesa síðuna úr mínum eftirliti',
'tooltip-search'                => 'Leita í {{SITENAME}}',
'tooltip-p-logo'                => 'Forsíða',
'tooltip-n-mainpage'            => 'Vitja forsíðuna',
'tooltip-n-portal'              => 'Um verkætlanina, hvat tú kanst gera, hvar tú finnur ymiskt',
'tooltip-n-currentevents'       => 'Finn bakgrundsupplýsingar um aktuellar hendingar',
'tooltip-n-recentchanges'       => 'Listi av teimum seinastu broytingunum í wikinum.',
'tooltip-n-randompage'          => 'Far til tilvildarliga síðu',
'tooltip-n-help'                => 'Staðurin at finna út.',
'tooltip-t-whatlinkshere'       => 'Yvirlit yvir allar wikisíður, ið slóða higar',
'tooltip-t-recentchangeslinked' => 'Broytingar á síðum, ið slóða higar, í seinastuni',
'tooltip-feed-rss'              => 'RSS-fóðurið til hesa síðuna',
'tooltip-feed-atom'             => 'Atom-fóðurið til hesa síðuna',
'tooltip-t-contributions'       => 'Skoða yvirlit yvir íkast hjá hesum brúkara',
'tooltip-t-emailuser'           => 'Send teldupost til henda brúkaran',
'tooltip-t-upload'              => 'Legg myndir ella miðlafílur upp',
'tooltip-t-specialpages'        => 'Yvirlit yvir serliga síður',
'tooltip-ca-nstab-main'         => 'Skoða innihaldssíðuna',
'tooltip-ca-nstab-user'         => 'Skoða brúkarasíðuna',
'tooltip-ca-nstab-media'        => 'Skoða miðlasíðuna',
'tooltip-ca-nstab-special'      => 'Hetta er ein serlig síða. Tú kanst ikki broyta síðuna sjálv/ur.',
'tooltip-ca-nstab-project'      => 'Skoða verkætlanarsíðuna',
'tooltip-ca-nstab-image'        => 'Skoða myndasíðuna',
'tooltip-ca-nstab-mediawiki'    => 'Skoða kervisamboðini',
'tooltip-ca-nstab-template'     => 'Brúka formin',
'tooltip-ca-nstab-help'         => 'Skoða hjálparsíðuna',
'tooltip-ca-nstab-category'     => 'Skoða bólkasíðuna',
'tooltip-save'                  => 'Goym broytingar mínar',

# Attribution
'anonymous'     => 'Dulnevndir brúkarar í {{SITENAME}}',
'siteuser'      => '{{SITENAME}}brúkari $1',
'othercontribs' => 'Grundað á arbeiði eftir $1.',
'others'        => 'onnur',
'siteusers'     => '{{SITENAME}}brúkari(ar) $1',

# Info page
'infosubtitle' => 'Upplýsingar um síðu',

# Math options
'mw_math_png'    => 'Vís altíð sum PNG',
'mw_math_simple' => 'HTML um sera einfalt annars PNG',
'mw_math_html'   => 'HTML um møguligt annars PNG',
'mw_math_source' => 'Lat verða sum TeX (til tekstkagara)',
'mw_math_modern' => 'Tilmælt nýtíðarkagara',
'mw_math_mathml' => 'MathML um møguligt (roynd)',

# Patrolling
'rcpatroldisabled'     => 'Ansanin eftir nýkomnum broytingum er óvirkin',
'rcpatroldisabledtext' => 'Hentleikin við ansing eftir nýkomnum broytingum er óvirkin í løtuni.',

# Browsing diffs
'previousdiff' => '← Far til fyrra mun',
'nextdiff'     => 'Far til næsta mun →',

# Media information
'imagemaxsize'   => 'Avmarka myndir á myndalýsingarsíðum til:',
'thumbsize'      => 'Smámyndastødd:',
'file-info-size' => '($1 × $2 pixel, stødd fílu: $3, MIME-slag: $4)',
'svg-long-desc'  => '(SVG fíle, nominelt $1 × $2 pixel, fíle stødd: $3)',

# Special:NewImages
'newimages' => 'Nýggjar myndir',
'noimages'  => 'Einki at síggja.',
'ilsubmit'  => 'Leita',
'bydate'    => 'eftir dato',

# Metadata
'metadata' => 'Metadáta',

# EXIF tags
'exif-artist'    => 'Rithøvundur',
'exif-copyright' => 'Upphavsrætt haldari',

# 'all' in various places, this might be different for inflected languages
'watchlistall2' => 'alt',
'namespacesall' => 'alt',
'monthsall'     => 'allir',

# E-mail address confirmation
'confirmemail'          => 'Vátta t-post adressu',
'confirmemail_send'     => 'Send eina váttanarkotu',
'confirmemail_sent'     => 'Játtanar t-postur sendur.',
'confirmemail_oncreate' => 'Ein staðfesingar kota er send til tína T-post adressu.
Tað er ikki neyðugt at hava hesa kodu fyri at rita inn, men tú mást veita hana áðrenn
tú kanst nýta nakran T-post-grundaðan hentleika í hesi wiki.',
'confirmemail_loggedin' => 'Tín t-post adressa er nú váttað.',
'confirmemail_subject'  => '{{SITENAME}} váttan av T-post adressu',
'confirmemail_body'     => 'Onkur, væntandi tú frá IP adressu $1, hevur skráset eina
konti "$2" við hesu T-post adressu á {{SITENAME}}.

Fyri at vátta at hendan konti veruliga hoyrur til tín,
og fyri at aktivera T-post funktiónir á {{SITENAME}}, so skalt
tú trýsta á fylgjandi slóð í tínum kagara:

$3

Um hetta *ikki* er tú, skalt tú ikki trýsta á slóðina. Hendan váttanarkoda
fer úr gildi tann $4.',

# action=purge
'confirm_purge_button' => 'Í lagi',

# AJAX search
'hideresults' => 'Fjal úrslit',

# Multipage image navigation
'imgmultipageprev' => '← fyrrverandi síða',
'imgmultipagenext' => 'næsta síða →',
'imgmultigo'       => 'Far!',

# Table pager
'table_pager_next'         => 'Næsta síða',
'table_pager_prev'         => 'Fyrrverandi síða',
'table_pager_limit_submit' => 'Far',

# Auto-summaries
'autosumm-new' => 'Nýggj síða: $1',

# Watchlist editor
'watchlistedit-normal-title' => 'Rætta eftirlit',
'watchlistedit-raw-title'    => 'Rætta rátt eftirlit',
'watchlistedit-raw-legend'   => 'Rætta rátt eftirlit',

# Watchlist editing tools
'watchlisttools-view' => 'Vís viðkomandi broytingar',
'watchlisttools-edit' => 'Vís og rætta eftirlit',
'watchlisttools-raw'  => 'Rætta rátt eftirlit',

# Special:Version
'version'                  => 'Útgáva', # Not used as normal message but as header for the special page itself
'version-hooks'            => 'Krókur',
'version-hook-name'        => 'Krókurnavn',
'version-version'          => 'Útgáva',
'version-software-version' => 'Útgáva',

# Special:FilePath
'filepath-page' => 'Fíla:',

# Special:SpecialPages
'specialpages' => 'Serligar síður',

);
