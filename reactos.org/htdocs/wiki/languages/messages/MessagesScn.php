<?php
/** Sicilian (Sicilianu)
 *
 * @ingroup Language
 * @file
 *
 * @author Melos
 * @author Sarvaturi
 * @author Tonyfroio
 * @author לערי ריינהארט
 */

$fallback = 'it';

$namespaceNames = array(
	NS_MEDIA          => 'Mèdia',
	NS_SPECIAL        => 'Spiciali',
	NS_MAIN           => '',
	NS_TALK           => 'Discussioni',
	NS_USER           => 'Utenti',
	NS_USER_TALK      => 'Discussioni_utenti',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK   => 'Discussioni_$1',
	NS_IMAGE          => 'Mmàggini',
	NS_IMAGE_TALK     => 'Discussioni_mmàggini',
	NS_MEDIAWIKI      => 'MediaWiki',
	NS_MEDIAWIKI_TALK => 'Discussioni_MediaWiki',
	NS_TEMPLATE       => 'Template',
	NS_TEMPLATE_TALK  => 'Discussioni_template',
	NS_HELP           => 'Aiutu',
	NS_HELP_TALK      => 'Discussioni_aiutu',
	NS_CATEGORY       => 'Catigurìa',
	NS_CATEGORY_TALK  => 'Discussioni_catigurìa',
);

$namespaceAliases = array(
	'Discussioni_Utenti' => NS_USER_TALK,
	'Discussioni_Template' => NS_TEMPLATE_TALK,
	'Discussioni_Aiutu' => NS_HELP_TALK,
	'Discussioni_Catigurìa' => NS_CATEGORY_TALK,
);

$skinNames = array(
	'standard'    => 'Classicu',
	'simple'      => 'Sìmprici',
);

$specialPageAliases = array(
	'DoubleRedirects'           => array( 'RinnirizzamentiDuppi' ),
	'BrokenRedirects'           => array( 'RinnirizzamentiSbagghiati' ),
	'Disambiguations'           => array( 'Disambiguazzioni' ),
	'Userlogin'                 => array( 'Tràsi,Login' ),
	'Userlogout'                => array( 'Nesci,Logout' ),
	'Preferences'               => array( 'Prifirenzi' ),
	'Watchlist'                 => array( 'ArtìculiTalìati' ),
	'Recentchanges'             => array( 'ÙrtimiCanciamenti' ),
	'Upload'                    => array( 'Càrrica' ),
	'Imagelist'                 => array( 'Mmàggini' ),
	'Newimages'                 => array( 'MmàgginiRicenti' ),
	'Listusers'                 => array( 'Utilizzatura,ListaUtilizzatura' ),
	'Statistics'                => array( 'Statistichi' ),
	'Randompage'                => array( 'PàgginaAmmuzzu' ),
	'Lonelypages'               => array( 'PàgginiOrfani' ),
	'Uncategorizedpages'        => array( 'PàgginiSenzaCatigurìi' ),
	'Uncategorizedcategories'   => array( 'CatigurìiSenzaCatigurìi' ),
	'Uncategorizedimages'       => array( 'MmàgginiSenzaCatigurìi' ),
	'Uncategorizedtemplates'    => array( 'TemplateSenzaCatigurìi' ),
	'Unusedcategories'          => array( 'CatigurìiNonUsati' ),
	'Unusedimages'              => array( 'MmàgginiNonUsati' ),
	'Wantedpages'               => array( 'PàgginiRichiesti' ),
	'Wantedcategories'          => array( 'CatigurìiRichiesti' ),
	'Mostlinked'                => array( 'PàgginiCchiùrRichiamati' ),
	'Mostlinkedcategories'      => array( 'CatigurìiCchiùrRichiamati' ),
	'Mostlinkedtemplates'       => array( 'TemplateCchiùRichiamati' ),
	'Mostcategories'            => array( 'PàgginiCuCchiùCatigurìi' ),
	'Mostimages'                => array( 'MmàgginiCchiùRichiamati' ),
	'Mostrevisions'             => array( 'PàgginiCuCchiùRivisioni' ),
	'Fewestrevisions'           => array( 'PàgginiCuMenuRivisioni,PàgginiCuMenoRivisioni' ),
	'Shortpages'                => array( 'PàgginiCchiùCurti' ),
	'Longpages'                 => array( 'PàgginiCchiùLonghi' ),
	'Newpages'                  => array( 'PàgginiCchiùRicenti,PàgginiCchiùRecenti' ),
	'Ancientpages'              => array( 'PàgginiMenuRecenti,PàgginiMenoRicenti,PàgginiMenuRecenti,PàgginiMenoRicenti' ),
	'Deadendpages'              => array( 'PàgginiSenzaNisciuta' ),
	'Protectedpages'            => array( 'PàgginiPrutetti' ),
	'Allpages'                  => array( 'TuttiLiPàggini' ),
	'Prefixindex'               => array( 'Prifissi,Prefissi' ),
	'Ipblocklist'               => array( 'IPBluccati' ),
	'Specialpages'              => array( 'PàgginiSpiciali' ),
	'Contributions'             => array( 'Cuntribbuti,CuntribbutiUtenti' ),
	'Emailuser'                 => array( 'MannaEmail' ),
	'Whatlinkshere'             => array( 'ChiPuntaCcà' ),
	'Recentchangeslinked'       => array( 'CancaimentiCurrilati' ),
	'Movepage'                  => array( 'Sposta,Rinomina' ),
	'Blockme'                   => array( 'BloccaProxy' ),
	'Booksources'               => array( 'RicercaISBN' ),
	'Categories'                => array( 'Catigurìi' ),
	'Export'                    => array( 'Esporta' ),
	'Version'                   => array( 'Virsioni' ),
	'Allmessages'               => array( 'Missaggi' ),
	'Log'                       => array( 'Log,Riggistri,Riggistro' ),
	'Blockip'                   => array( 'Blocca' ),
	'Undelete'                  => array( 'Riprìstina' ),
	'Import'                    => array( 'Mporta' ),
	'Lockdb'                    => array( 'BloccaDB,BloccaDatabase' ),
	'Unlockdb'                  => array( 'SbloccaDB,SbloccaDatabase' ),
	'Userrights'                => array( 'PirmissiUtenti' ),
	'MIMEsearch'                => array( 'RicercaMIME' ),
	'Unwatchedpages'            => array( 'PàgginiNunOssirvati' ),
	'Listredirects'             => array( 'Rinnirizzamenti,ListaRinnirizzamenti' ),
	'Revisiondelete'            => array( 'CancellaRivisioni' ),
	'Unusedtemplates'           => array( 'TemplateNunUsati' ),
	'Randomredirect'            => array( 'RedirectAmmuzzu' ),
	'Mypage'                    => array( 'MèPàgginaUtenti' ),
	'Mytalk'                    => array( 'MèDiscussioni' ),
	'Mycontributions'           => array( 'MèCuntribbuti' ),
	'Listadmins'                => array( 'Amministratura' ),
	'Popularpages'              => array( 'PàgginiCchiùVisitati' ),
	'Search'                    => array( 'Ricerca,Cerca' ),
	'Resetpass'                 => array( 'ReimpostaPassword' ),
	'Withoutinterwiki'          => array( 'SenzaInterwiki' ),
);

$messages = array(
# User preference toggles
'tog-underline'               => 'Suttalìnia li culligamenti:',
'tog-highlightbroken'         => 'Furmatta <a href="" class="new">accussì</a> (o accussì<a href="" class="internal">?</a>) li culligamenti ca pùntanu a artìculi ancora a scrìviri.',
'tog-justify'                 => 'Alliniamentu dû paràgrafu: giustificatu',
'tog-hideminor'               => "Ammuccia li canciamenti nichi nta l'ùrtimi canciamenti",
'tog-extendwatchlist'         => "Attiva li funzioni avanzati pi l'ossirvati spiciali",
'tog-usenewrc'                => "''Ùrtimi canciamenti'' avanzati (arcuni browser ponnu aviri prubbremi ntô visualizzàrili)",
'tog-numberheadings'          => 'Nummirazzioni automàtica dî tìtuli di paràgrafu',
'tog-showtoolbar'             => 'Ammustra la barra dî strumenta pi lu canciamentu',
'tog-editondblclick'          => "Duppiu click pi canciari l'artìculu (richiedi Javascript)",
'tog-editsection'             => 'Abbìlita lu canciamentu dî sezzioni tràmiti lu culligamentu [cancia]',
'tog-editsectiononrightclick' => 'Abbìlita lu canciamentu dî sezzioni tràmiti duppiu click supra lu tìtulu dâ sezzioni (richiedi Javascript)',
'tog-showtoc'                 => "Ammustra l'ìndici (pi artìculi cu cchiù di 3 sezzioni)",
'tog-rememberpassword'        => "Arricorda la password (richiedi l'usu di cookie)",
'tog-editwidth'               => 'Aumenta a lu màssimu la larghizza dâ casella di canciamentu',
'tog-watchcreations'          => "Agghiunci li pàggini criati a l'ossirvati spiciali",
'tog-watchdefault'            => "Agghiunci li pàggini canciati a l'ossirvati spiciali",
'tog-watchmoves'              => "Agghiunci li pàggini spustati a l'ossirvati spiciali",
'tog-watchdeletion'           => "Agghiunci li pàggini di mìa cancillati a l'ossirvati spiciali",
'tog-minordefault'            => 'Ìndica ogni canciamentu comu nicu (sulu comu pridifinitu)',
'tog-previewontop'            => "Ammustra l'antiprima prima dâ casella di canciamentu e nun doppu",
'tog-previewonfirst'          => "Ammustra l'antiprima supra lu primu canciamentu",
'tog-nocache'                 => 'Disabbìlita lu caching dî pàggini',
'tog-enotifwatchlistpages'    => 'Mànnami na e-mail siddu la pàggina subbisci canciamenti',
'tog-enotifusertalkpages'     => 'Mànnimi nu missaggiu email quannu la mè pàggina di discussioni è canciata',
'tog-enotifminoredits'        => 'Mànnami na e-mail macari pi li canciamenti nichi di sta pàggina',
'tog-enotifrevealaddr'        => 'Rivela lu mè ndirizzu e-mail ntê mail di nutificazzioni',
'tog-shownumberswatching'     => 'Ammustra lu nùmmiru di utenti ca sèquinu la pàggina',
'tog-fancysig'                => 'Nun canciari lu markup dâ firma (usari pi firmi nun standard)',
'tog-externaleditor'          => 'Usa di default un editor sternu',
'tog-externaldiff'            => 'Usa di default un prugramma di diff sternu',
'tog-showjumplinks'           => "Attiva li culligamenti accissìbbili 'và a'",
'tog-uselivepreview'          => "Attiva la funzioni ''Live preview'' (richiedi JavaScript; spirimintali)",
'tog-forceeditsummary'        => "Chiedi cunferma siddu l'uggettu dû canciamentu è vacanti",
'tog-watchlisthideown'        => "Ammuccia li mè canciamenti nta l'ossirvati spiciali",
'tog-watchlisthidebots'       => "Ammuccia li canciamenti dî bot nta l'ossirvati spiciali",
'tog-watchlisthideminor'      => "Ammuccia li canciamenti nichi nta l'ossirvati spiciali",
'tog-nolangconversion'        => 'Disattiva la cunvirsioni tra varianti linguìstichi',
'tog-ccmeonemails'            => "Mànnami na copia dî missaggi spiditi a l'àutri utenti",
'tog-diffonly'                => "Nun visualizzari lu cuntinutu dâ pàggina quannu s'esequi na ''diff'' tra dui virsioni",
'tog-showhiddencats'          => 'Ammustra li catigurìi ammucciati.',

'underline-always'  => 'sempri',
'underline-never'   => 'mai',
'underline-default' => 'manteni li mpustazzioni dû browser',

'skinpreview' => '(Antiprima)',

# Dates
'sunday'        => 'Duminicadìa',
'monday'        => 'Lunidìa',
'tuesday'       => 'Martidìa',
'wednesday'     => 'Mercuridìa',
'thursday'      => 'Jovidìa',
'friday'        => 'Venniridìa',
'saturday'      => 'Sabbatudìa',
'sun'           => 'dum',
'mon'           => 'lun',
'tue'           => 'mar',
'wed'           => 'mer',
'thu'           => 'jov',
'fri'           => 'vènn',
'sat'           => 'sabb',
'january'       => 'jinnaru',
'february'      => 'Frivaru',
'march'         => 'Marzu',
'april'         => 'Aprili',
'may_long'      => 'Maiu',
'june'          => 'Giugnu',
'july'          => 'Giugnettu',
'august'        => 'Austu',
'september'     => 'Sittèmmiru',
'october'       => 'Uttùviru',
'november'      => 'Nuvèmmiru',
'december'      => 'Dicèmmiru',
'january-gen'   => 'jinnaru',
'february-gen'  => 'frivaru',
'march-gen'     => 'marzu',
'april-gen'     => 'Aprili',
'may-gen'       => 'Maiu',
'june-gen'      => 'giugnu',
'july-gen'      => 'giugnettu',
'august-gen'    => 'Austu',
'september-gen' => 'sittèmmiru',
'october-gen'   => 'uttùviru',
'november-gen'  => 'nuvèmmiru',
'december-gen'  => 'Dicèmmiru',
'jan'           => 'jin',
'feb'           => 'Friv',
'mar'           => 'mar',
'apr'           => 'apr',
'may'           => 'Maiu',
'jun'           => 'giu',
'jul'           => 'giugn',
'aug'           => 'Au',
'sep'           => 'Sitt',
'oct'           => 'utt',
'nov'           => 'nuv',
'dec'           => 'Dic',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|Catigurìa|Catigurìi}}',
'category_header'                => 'Artìculi ntâ catigurìa "$1"',
'subcategories'                  => 'Suttacatigurìi',
'category-media-header'          => 'File multimidiali ntâ catigurìa "$1"',
'category-empty'                 => "''Sta catigurìa attuarmenti nun havi artìculi o \"media\".''",
'hidden-categories'              => '{{PLURAL:$1|Catigurìa ammucciata|Catigurìi ammuciati}}',
'hidden-category-category'       => 'Catigurìi ammucciati', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|Sta catigurìa cunteni na sula suttacatigurìa, nnicata ccà sutta.|Sta catigurìa cunteni {{PLURAL:$1|la suttacatigurìa|li $1 suttacatigurìi nnicati}} ccà sutta, sùpira nu tutali di $2.}}',
'category-subcat-count-limited'  => 'Sta catigurìa cunteni {{PLURAL:$1|na suttacatigurìa, nnicata|$1 suttacatigurìi, nnicati}} ccà sutta.',
'category-article-count'         => '{{PLURAL:$2|Sta catigurìa cunteni na pàggina sula, nnicata ccà sutta.|Sta catigurìa cunteni {{PLURAL:$1|la pàggina nnicata|li $1 pàggini nnicati}} di sècutu, supra nu tutali di $2.}}',
'category-article-count-limited' => 'Stâ catiguria cunteni {{PLURAL:$1|la pàggina ndicata|li $1 pàggini ndicati}} ccà sutta.',
'category-file-count'            => '{{PLURAL:$2|Sta catigurìa cunteni nu sulu file, ndicatu ccà sutta.|Sta catigurìa cunteni {{PLURAL:$1|nu file, ndicatu|$1 file, ndicati}} ccà sutta, su nu totali di $2.}}',
'category-file-count-limited'    => 'Sta catigurìa cunteni {{PLURAL:$1|lu file ndicatu|li $1 file ndicati}} ccà sutta.',
'listingcontinuesabbrev'         => ' cunt.',

'mainpagetext'      => 'Nstallazzioni di MediaWiki cumplitata currettamenti.',
'mainpagedocfooter' => "Pi favuri taliari [http://meta.wikimedia.org/wiki/Help:Contents Guida utenti] pi aiutu supra l'usu e la cunfigurazzioni di stu software wiki. 

== P'accuminzari == 
* [http://www.mediawiki.org/wiki/Manual:Configuration_settings Alencu di mpustazzioni di cunfigurazzioni] 
* [http://www.mediawiki.org/wiki/Manual:FAQ MediaWiki FAQ] 
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce Mailing list dî rilassi di MediaWiki]",

'about'          => 'pàggina',
'article'        => 'artìculu',
'newwindow'      => '(grapi na finestra nova)',
'cancel'         => 'annulla',
'qbfind'         => 'Attrova',
'qbbrowse'       => 'Sfogghia',
'qbedit'         => 'Cancia',
'qbpageoptions'  => 'Opzioni pàggina',
'qbpageinfo'     => 'Nfurmazzioni supra la pàggina',
'qbmyoptions'    => 'Li mè pàggini',
'qbspecialpages' => 'Pàggini spiciali',
'moredotdotdot'  => 'Àutru...',
'mypage'         => 'La mè pàggina',
'mytalk'         => 'la mè pàggina di discussioni',
'anontalk'       => 'Discussione pi stu IP',
'navigation'     => 'Navigazzioni',
'and'            => 'e',

# Metadata in edit box
'metadata_help' => 'Metadati:',

'errorpagetitle'    => 'Erruri',
'returnto'          => 'Ritorna a $1.',
'tagline'           => 'Di {{SITENAME}}',
'help'              => 'Aiutu',
'search'            => 'Trova',
'searchbutton'      => "Va' cerca",
'go'                => 'Trova',
'searcharticle'     => 'Vai',
'history'           => 'cronuluggìa',
'history_short'     => 'storia',
'updatedmarker'     => 'canciata dâ mè ùrtima vìsita',
'info_short'        => 'Nfurmazzioni',
'printableversion'  => 'Virsioni stampàbbili',
'permalink'         => 'Liami pirmanenti',
'print'             => 'Stampa',
'edit'              => 'cancia',
'create'            => 'Crea',
'editthispage'      => 'Cancia sta pàggina',
'create-this-page'  => 'Crea sta pàggina',
'delete'            => 'elìmina',
'deletethispage'    => 'Elìmina sta pàggina',
'undelete_short'    => 'Ricùpira {{PLURAL:$1|na rivisioni|$1 rivisioni}}',
'protect'           => 'Pruteggi',
'protect_change'    => 'cancia',
'protectthispage'   => 'Pruteggi sta pàggina',
'unprotect'         => 'livari la prutizzioni',
'unprotectthispage' => 'Sblocca sta pàggina',
'newpage'           => 'pàggina nova',
'talkpage'          => 'Pàggina di discussioni',
'talkpagelinktext'  => 'Discussioni',
'specialpage'       => 'Pàggina spiciali',
'personaltools'     => 'Strumenta pirsunali',
'postcomment'       => 'Manna un cummentu',
'articlepage'       => 'artìculu',
'talk'              => 'discussioni',
'views'             => 'Vìsiti',
'toolbox'           => 'Strummenta',
'userpage'          => 'Visualizza la pàggina utenti',
'projectpage'       => 'Visualizza la pàggina di sirvizziu',
'imagepage'         => 'Visualizza la pàggina di discrizzioni dâ mmàggini',
'mediawikipage'     => 'Visualizza lu missaggiu',
'templatepage'      => 'Visualizza lu template',
'viewhelppage'      => "Visualizza la pàggina d'aiutu",
'categorypage'      => 'Visualizza la catigurìa',
'viewtalkpage'      => 'Vidi discussioni',
'otherlanguages'    => 'Àutri lingui',
'redirectedfrom'    => '(Rinnirizzata di $1)',
'redirectpagesub'   => 'Pàggina di rinnirizzamentu',
'lastmodifiedat'    => 'Sta pàggina fu canciata a $2 di lu $1.', # $1 date, $2 time
'viewcount'         => 'Sta pàggina hà statu liggiuta {{PLURAL:$1|una vota|$1 voti}}.',
'protectedpage'     => 'Pàggina bluccata',
'jumpto'            => "Va' a:",
'jumptonavigation'  => 'navigazzioni',
'jumptosearch'      => "Va' cerca",

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'Àutri nfurmazzioni supra {{SITENAME}}',
'aboutpage'            => 'Project:Àutri nformazzioni',
'bugreports'           => 'Malifunziunamenti',
'bugreportspage'       => 'Project:Malifunziunamenti',
'copyright'            => 'Lu cuntinutu è utilizzàbbili secunnu la $1.',
'copyrightpagename'    => 'Lu copyright supra {{SITENAME}}',
'copyrightpage'        => '{{ns:project}}:Copyright',
'currentevents'        => 'Nutizzî',
'currentevents-url'    => 'Project:Nutizzî',
'disclaimers'          => 'Avvirtenzi',
'disclaimerpage'       => 'Project:Avvirtenzi ginirali',
'edithelp'             => 'Guida',
'edithelppage'         => 'Help:Canciamentu',
'faq'                  => 'Dumanni cumuni',
'faqpage'              => 'Project:Dumanni comuni',
'helppage'             => 'Help:Cuntinuti',
'mainpage'             => 'Pàggina principali',
'mainpage-description' => 'Pàggina principali',
'policy-url'           => 'Project:Policy',
'portal'               => 'Porta dâ cumunitati',
'portal-url'           => 'Project:Porta dâ cumunitati',
'privacy'              => 'Pulìtica supra la privacy',
'privacypage'          => 'Project:Pulìtica rilativa â privacy',

'badaccess'        => 'Pirmessi nun sufficienti',
'badaccess-group0' => "Nun hai li pirmessi nicissari p'esèquiri l'azzioni addumannata.",
'badaccess-group1' => "La funzioni addumannata è risirvata a l'utenti ca appartèninu a lu gruppu $1.",
'badaccess-group2' => "La funzioni addumannata è risirvata a l'utenti di unu dî gruppi $1.",
'badaccess-groups' => "La funzioni addumannata è risirvata a l'utenti ca appartèninu a unu dî siquenti gruppi: $1.",

'versionrequired'     => 'È nicissaria la virsioni $1 dû software MediaWiki',
'versionrequiredtext' => "P'usari sta pàggina ci voli la virsioni $1 dû software MediaWiki. Talìa [[Special:Version|sta pàggina]]",

'ok'                      => 'OK',
'retrievedfrom'           => 'Estrattu di "$1"',
'youhavenewmessages'      => 'Ricivìsti $1 ($2).',
'newmessageslink'         => 'missaggi novi',
'newmessagesdifflink'     => 'ùrtimi canciamenti',
'youhavenewmessagesmulti' => 'Hai missaggi novi supra $1',
'editsection'             => 'cancia',
'editold'                 => 'cancia',
'viewsourceold'           => 'talìa la fonti',
'editsectionhint'         => 'Cancia la sezzioni $1',
'toc'                     => 'Ìndici',
'showtoc'                 => 'ammustra',
'hidetoc'                 => 'ammuccia',
'thisisdeleted'           => 'Vidi e/o riprìstina $1?',
'viewdeleted'             => 'Vidi $1?',
'restorelink'             => '{{PLURAL:$1|nu canciamentu annullatu|$1 canciamenti annullati}}',
'feedlinks'               => 'Feed:',
'feed-invalid'            => 'Mudalitati di suttascrizzioni dû feed nun vàlida.',
'feed-unavailable'        => 'Nun sunu dispunibili li feed pi li cuntinuti di {{SITENAME}}',
'site-rss-feed'           => 'Feed RSS di $1',
'site-atom-feed'          => 'Feed Atom di $1',
'page-rss-feed'           => 'Feed RSS pi "$1"',
'page-atom-feed'          => 'Feed Atom pi "$1"',
'red-link-title'          => '$1 (ancora nun scrivutu)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'artìculu',
'nstab-user'      => "Pàggina d'utenti",
'nstab-media'     => 'File multimidiali',
'nstab-special'   => 'spiciali',
'nstab-project'   => 'pàggina',
'nstab-image'     => 'mmàggini',
'nstab-mediawiki' => 'missagiu',
'nstab-template'  => 'template',
'nstab-help'      => 'aiutu',
'nstab-category'  => 'Catigurìa',

# Main script and global functions
'nosuchaction'      => 'Opirazzioni nun ricanusciuta',
'nosuchactiontext'  => 'La URL mmessa nun currispunni a un cumannu ricanusciutu dû software MediaWiki',
'nosuchspecialpage' => 'Sta pàggina spiciali nun è dispunìbbili',
'nospecialpagetext' => "<big>'''Hai cercatu na pàggina spiciali nun vàlida.'''</big>

L'alencu dî pàggini spiciali vàlidi s'attrova 'n [[Special:SpecialPages|Alencu dî pàggini spiciali]].",

# General errors
'error'                => 'Erruri',
'databaseerror'        => 'Erruri dû database',
'dberrortext'          => 'Erruri di sintassi ntâ richiesta nultrata a lu database. Chistu putissi innicari la prisenza d\'un bug ntô software. L\'ùrtima query mannata a lu database hà stata: <blockquote><tt>$1</tt></blockquote> richiamata dâ funzioni "<tt>$2</tt>". MySQL hà ristituitu lu siquenti erruri "<tt>$3: $4</tt>".',
'dberrortextcl'        => 'Erruri di sintassi ntâ richiesta nultrata a lu database. L\'ùrtima query mannata a lu database hà stata: "$1" richiamata dâ funzioni "$2". MySQL hà ristituitu lu siquenti erruri "$3: $4".',
'noconnect'            => 'Cunnissioni ô databbasi nun arrinisciuta a càusa di nu prubbrema tècnicu dô situ.<br />$1',
'nodb'                 => 'Silizzioni dû database $1 nun arrinisciuta',
'cachederror'          => "Chidda prisintata di sèquitu è na copia ''cache'' dâ pàggina richiesta; putissi quinni nun èssiri aggiurnata.",
'laggedslavemode'      => "Accura: La pàggina putissi nun ripurtari l'aggiurnamenti cchiù ricenti.",
'readonly'             => 'Database bluccatu',
'enterlockreason'      => "Ìnnica lu mutivu dû bloccu, spicificannu lu mumentu 'n cui è prisumìbbili ca veni rimossu.",
'readonlytext'         => "Nta stu mumentu lu database è bluccatu e nun sunnu pussìbbili junti o canciamenti a li pàggini. Lu bloccu è di sòlitu ligatu a upirazzioni di manutinzioni urdinaria, a lu tèrmini dî quali lu database è di novu accissìbbili. L'amministraturi di sistema c'ha mpostu lu bloccu hà furnitu sta spiegazzioni: $1",
'missingarticle-rev'   => '(rivisioni#: $1)',
'missingarticle-diff'  => '(Diff: $1, $2)',
'readonly_lag'         => 'Lu database hà statu bluccatu automaticamenti, mentri li server cu li database slave si sincrunìzzanu cu lu master',
'internalerror'        => 'Erruri nternu',
'internalerror_info'   => 'Erruri nternu: $1',
'filecopyerror'        => 'Mpussìbbili cupiari lu file "$1" n "$2".',
'filerenameerror'      => 'Mpussìbbili rinuminari lu file "$1" \'n "$2".',
'filedeleteerror'      => 'Mpussìbbili cancillari lu file "$1".',
'directorycreateerror' => 'Mpussìbbili criari la directory "$1".',
'filenotfound'         => 'File "$1" nun attruvatu.',
'fileexistserror'      => 'Mpussìbbili scrìviri lu file "$1": lu file esisti già',
'unexpected'           => 'Valuri mpruvistu: "$1"="$2".',
'formerror'            => 'Erruri: mpussìbbili mannari lu mòdulu',
'badarticleerror'      => 'Opirazzioni nun cunzintita pi sta pàggina.',
'cannotdelete'         => 'Mpussìbbili cancillari la pàggina o lu file addumannatu. (Putissi aviri statu già cancillatu.)',
'badtitle'             => 'Tìtulu nun currettu',
'badtitletext'         => "Lu tìtulu dâ pàggina addumannata è vacanti, erratu o cu caràttiri nun ammessi oppuru diriva di n'erruri ntê culligamenti tra siti wiki diversi o virsioni n lingui diversi dû stissu situ.",
'perfdisabled'         => "Semu spiacenti, sta funziunalitati è timpuraniamenti disabbilitata pirchì lu sò usu rallenta lu database finu a rìnniri lu situ nutilizzàbbili pi tutti l'utenti.",
'perfcached'           => "'''Nota:''' li dati ca sèquinu sunnu stratti di na copia ''cache'' dû database, nun aggiurnati n tempu riali.",
'perfcachedts'         => 'Li dati ccà sutta foru attruvati e sunnu aggiurnati ô $1.',
'querypage-no-updates' => "L'aggiurnamenti dâ pàggina sunnu timpuraniamenti suspisi. Li dati 'n chidda cuntinuti nun vèninu aggiurnati.",
'wrong_wfQuery_params' => 'Paràmitri errati pi wfQuery()<br />
Funzioni: $1<br />
Query: $2',
'viewsource'           => 'Talìa la fonti',
'viewsourcefor'        => 'di $1',
'actionthrottled'      => 'Azzioni ritardata',
'actionthrottledtext'  => "Comu misura di sicurezza contru lu spam, l'esecuzioni di alcuni azzionu è limitata a nu nùmmuru massimu di voti ni nu determinatu piriudu du tempu, limiti ca ni stu casu fu supiratu. Si prega di ripruvari tra qualchi minutu.",
'protectedpagetext'    => 'Sta pàggina fu bluccata pi privèniri li canciamenti.',
'viewsourcetext'       => 'È pussìbbili visualizzari e cupiari lu còdici surgenti di sta pàggina:',
'protectedinterface'   => "Sta pàggina cunteni un elementu ca fà parti dâ nterfaccia utenti dû software; è quinni prutetta p'evitari pussìbbili abbusi.",
'editinginterface'     => "'''Accura:''' Lu testu di sta pàggina fà parti d l'interfaccia utenti dû situ. Tutti li canciamenti appurtati a sta pàggina si riflèttinu supra li missaggi visualizzati pi tutti l'utenti.",
'sqlhidden'            => '(la query SQL hà statu ammucciata)',
'cascadeprotected'     => 'Supra sta pàggina nun è pussìbbili effittuari canciamenti pirchì veni nclusa nt{{PLURAL:$1|â pàggina siquenti, ca fu prutetta|ê pàggini siquenti, ca foru prutetti}} silizziunannu la prutizzioni "ricursiva":
$2',
'namespaceprotected'   => "Nun hai lu pirmissu pi canciari li pàggini ntô namespace '''$1'''.",
'customcssjsprotected' => "Nun hai lu pirmissu di canciari sta pàggina, picchì cunteni li mpustazzioni pirsunali di n'àutru utenti.",
'ns-specialprotected'  => 'Li pàggini ntô namespace {{ns:special}} non ponnu èssiri canciati.',
'titleprotected'       => "La criazzioni di sta pàggina cu stu tìtulu fu bluccata da [[User:$1|$1]].
La mutivazzioni è chista: ''$2''.",

# Login and logout pages
'logouttitle'                => 'Logout utenti',
'logouttext'                 => "<strong>Ora tu niscisti.</strong><br />
Poi cuntinuari a usari {{SITENAME}} di manera anònima, o poi tràsiri n'àutra vota cu lu stissu o cu n'àutru nomu d'utenti. Accura chi quarchi pàggina pò cuntinuari a èssiri ammustrata comu si nun avissi nisciutu nzinu a quannu tu nun cancelli tutta la mimoria dû tò browser.",
'welcomecreation'            => "== Bonvinutu, $1! ==

L'account hà statu criatu currettamenti. Nun ti scurdari di pirsunalizzari li [[Special:Preferences|prifirenzi di {{SITENAME}}]].",
'loginpagetitle'             => 'Login utenti',
'yourname'                   => "Lu tò nomu d'utenti (''user name'')",
'yourpassword'               => "La tò ''password''",
'yourpasswordagain'          => "Scrivi la password n'àutra vota",
'remembermypassword'         => "Ricòrdami la mè ''password'' duranti li sissioni.",
'yourdomainname'             => 'Lu tò dominiu',
'externaldberror'            => "S'havi virificatu n'erruri cû server d'autinticazzioni sternu, oppuru nun si disponi di l'auturizzazzioni nicissari p'aggiurnari lu propiu accessu sternu.",
'loginproblem'               => "<b>S'hà virificatu n'erruri duranti l'accessu.</b><br />Ripruvari.",
'login'                      => 'Trasi',
'nav-login-createaccount'    => 'Riggìstrati o trasi',
'loginprompt'                => "Tu hai a abbilitari li ''cookies'' pi tràsiri ntâ {{SITENAME}}.",
'userlogin'                  => 'Riggìstrati o trasi',
'logout'                     => 'Nesci',
'userlogout'                 => 'Nesci',
'notloggedin'                => "Nun v'aviti riggistratu",
'nologin'                    => "Nun nn'aviti nu cuntu pi ccà? $1.",
'nologinlink'                => 'Criati nu cuntu sùbbitu',
'createaccount'              => 'Criati un cuntu novu',
'gotaccount'                 => 'Hai già nu cuntu? $1.',
'gotaccountlink'             => 'Trasi',
'createaccountmail'          => 'via e-mail',
'badretype'                  => "La ''password'' chi mittisti nun è bona.",
'userexists'                 => 'Lu nomu utenti nzeritu è già usatu. Ti prijamu pirciò di vuliri scègghiri nu nomu utenti diversu.',
'youremail'                  => 'Lu tò nnirizzu email:',
'username'                   => "Nomu d'utenti:",
'uid'                        => 'ID utenti:',
'yourrealname'               => 'Lu tò nomu veru*',
'yourlanguage'               => 'Lingua dâ nterfaccia:',
'yourvariant'                => 'Varianti:',
'yournick'                   => 'Suprannomu (nickname):',
'badsig'                     => 'Erruri ntâ firma nun standard, virificari li tag HTML.',
'badsiglength'               => 'Lu Nickname è troppu longu. Nun pò aviri cchiù di $1 {{PLURAL:$1|caràttiri|caràttiri}}.',
'email'                      => 'Nnirizzu email',
'prefs-help-realname'        => '* Nomu veru (upziunali): siddu scegghi di furnìrilu veni usatu pi dàriti crèditu dû tò travagghiu.',
'loginerror'                 => "Erruri nta l'accessu",
'prefs-help-email'           => "* Imeil (opziunali): abbìlita l'àutri utenti a cuntattàriti attraversu la tò pàggina d'utenti o di discussioni, senza pi chissu rivilari la tò idintitati.",
'prefs-help-email-required'  => 'Lu nnirizzu email è nicissariu.',
'nocookiesnew'               => 'Lu nomu utenti pi tràsiri fu criatu, ma nun hai effittuatu lu log in. {{SITENAME}} usa li cookies pi gistiri li log in. Lu tò browser havi li cookies disabbilitati. Abbìlita li cookies, appoi effèttua lu login cu li tò username e password novi.',
'nocookieslogin'             => '{{SITENAME}} usa li cookies pi gistiri lu log in. Lu tò browser havi li cookies disabbilitati. Abbìlita li cookies, appoi effèttua lu login cu li tò username e password.',
'noname'                     => 'Lu nomu utenti innicatu nun è vàlidu, nun è pussìbbili criari un account a stu nomu.',
'loginsuccesstitle'          => 'Trasuta rinisciuta',
'loginsuccess'               => "'''Ora trasisti nta {{SITENAME}} comu \"\$1\".'''",
'nosuchuser'                 => 'Nun è riggistratu arcunu utenti di nomu "$1". Virificari lu nomu nziritu o criari un novu accessu.',
'nosuchusershort'            => 'Nun c\'è nuddu utenti di nomu "<nowiki>$1</nowiki>". Cuntrolla l\'ortugrafìa.',
'nouserspecified'            => 'È nicissariu spicificari un nomu utenti.',
'wrongpassword'              => "La ''password'' chi mittisti nun è giusta. Prova n'àutra vota.",
'wrongpasswordempty'         => 'Nun hà statu nzirita arcuna password. Ripruvari.',
'passwordtooshort'           => "La tò password nun è valida o è troppu brivi. Havi a cuntèniri armenu {{PLURAL:$1|1 caràttiri|$1 caràttiri}} e èssiri diversa dô tò nomu d'utenti.",
'mailmypassword'             => "Mànnimi n'àutra password",
'passwordremindertitle'      => 'Sirvizziu Password Reminder di {{SITENAME}}',
'passwordremindertext'       => 'Quarcunu (prubbabbirmenti tu, cu ndirizzu IP $1) hà addumannatu lu mannu di na password d\'accessu nova a {{SITENAME}} ($4). La password pi l\'utenti "$2" hà statu mpustata a "$3". È appurtunu esèquiri un accessu quantu prima e canciari la password mmidiatamenti. Siddu nun sî statu tu a fari la richiesta, oppuru hai ritruvatu la password e nun addisìi cchiù canciàrila, poi gnurari stu missaggiu e cuntinuari a usari la password vecchia.',
'noemail'                    => 'Nuddu ndirizzu e-mail riggistratu pi l\'utenti "$1".',
'passwordsent'               => 'Na password nova hà statu mannata a lu ndirizzu e-mail riggistratu pi l\'utenti "$1". Pi favuri, effèttua un accessu nun appena l\'arricevi.',
'blocked-mailpassword'       => 'Pi privèniri abbusi, nun è cunzititu usari la funzioni "Nvia nova password" d\'un ndirizzu IP bluccatu.',
'eauthentsent'               => "Un missaggiu e-mail di cunferma hà statu spiditu a lu ndirizzu ndicatu. Pi abbilitari la mannata di missaggi e-mail pi st'accessu è nicissariu sèquiri li istruzzioni ca vi sunnu ndicati, 'n modu di cunfirmari ca s'è li liggìttimi prupitari di lu ndirizzu",
'throttled-mailpassword'     => 'Na password nova hà già statu mannata di menu di {{PLURAL:$1|1 ura|$1 uri}}. Pi privèniri abbusi, la funzioni "Manna password nova" pò èssiri usata sulu una vota ogni {{PLURAL:$1|1 ura|$1 uri}}.',
'mailerror'                  => 'Erruri nta lu mannu dû missaggiu: $1',
'acct_creation_throttle_hit' => 'Semu spiacenti, ma hai già criatu $1 account. Nun poi criàrinni àutri.',
'emailauthenticated'         => 'Lu ndirizzu e-mail hà statu cunfirmatu lu $1.',
'emailnotauthenticated'      => 'Lu tò ndrizzu imeil nun hà statu ancora autinticatu. Nun vannu a èssiri mannati missaggi imeil pi sti funzioni.',
'noemailprefs'               => "Innicari un ndirizzu e-mail p'attivari sti funzioni.",
'emailconfirmlink'           => 'Cunfirmari lu tò ndrizzu imeil',
'invalidemailaddress'        => 'Lu nnirizzu email nun pò èssiri accittatu ca ci hà un furmatu nun vàlidu.
Pi favuri nziriti nu nnirizzu vàlidu o svacantati la casella.',
'accountcreated'             => 'Cuntu criatu',
'accountcreatedtext'         => "Fu criatu n'accessu pi l'utenti $1.",
'createaccount-title'        => "Criazzioni di n'accessu a {{SITENAME}}",
'createaccount-text'         => 'Qualcuno criau n\'accessu a {{SITENAME}} ($4) a nomu di $2, associatu cu stu ndirizzu di posta elettronica. La password pi l\'utenti "$2" è mpustata a "$3". È opportunu trasiri quantu prima e canciari la password subbutu.

Si l\'accessu fu criatu pi sbagghiu, si può gnurari stu missaggiu.',
'loginlanguagelabel'         => 'Lingua: $1',

# Password reset dialog
'resetpass'               => 'Rimposta la password',
'resetpass_announce'      => "Hai effittuatu l'accessu cu na password timpurània ca t'hà statu mannata via email. Pi tirminari l'accessu, hai a nziriri na password nova ccà:",
'resetpass_text'          => '<!-- Agghiunci lu testu ccà -->',
'resetpass_header'        => 'Rimposta la password',
'resetpass_submit'        => 'Mposta la password e accedi',
'resetpass_success'       => "Lu canciu password hà statu effittuatu cu successu! Ora stai effittuannu l'accessu...",
'resetpass_bad_temporary' => 'Password timpurània nun vàlida. Putissi aviri già canciatu la password o addumannatu na password nova timpurània.',
'resetpass_forbidden'     => 'Li password nun ponnu èssiri canciati supra sta wiki',
'resetpass_missing'       => 'Dati mancanti ntô mòdulu.',

# Edit page toolbar
'bold_sample'     => 'Grassettu',
'bold_tip'        => 'Grassettu',
'italic_sample'   => 'Cursivu',
'italic_tip'      => 'Cursivu',
'link_sample'     => 'Nomu dû link',
'link_tip'        => 'Link nternu',
'extlink_sample'  => 'http://www.example.com tìtulu dû culligamentu',
'extlink_tip'     => 'Culligamentu sternu (nutari lu prifissu http:// )',
'headline_sample' => 'Ntistazzioni',
'headline_tip'    => 'Suttantistazzioni',
'math_sample'     => 'Nzirisci ccà na fòrmula',
'math_tip'        => 'Fòrmula matimàtica (LaTeX)',
'nowiki_sample'   => 'Nzirisci ccà lu testu nun furmattatu',
'nowiki_tip'      => 'Gnora la furmattazzioni wiki',
'image_sample'    => 'Asempiu.jpg',
'image_tip'       => 'Mmàggini ncurpurata',
'media_sample'    => 'Asempiu.ogg',
'media_tip'       => 'Culligamentu a file multimidiali',
'sig_tip'         => 'Firma cu data e ura',
'hr_tip'          => 'Lìnia urizzuntali (usari cu giudizziu)',

# Edit pages
'summary'                          => 'Discrizzioni',
'subject'                          => 'Suggettu/ntistazzioni',
'minoredit'                        => 'Chistu è nu canciamentu nicu',
'watchthis'                        => 'talìa sta pàggina',
'savearticle'                      => 'sarva la pàggina',
'preview'                          => 'visuali',
'showpreview'                      => 'ammustra la visuali prima di sarvari',
'showlivepreview'                  => "Funzioni ''Live preview''",
'showdiff'                         => 'Ammustra li canciamenti',
'anoneditwarning'                  => "'''Accura''': nun hai esiquitu lu login. Lu tò ndirizzu IP veni riggistratu ntâ cronoluggìa di sta pàggina.",
'missingsummary'                   => "'''Accura:''' Nun hà statu spicificatu l'uggettu di stu canciamentu. Primennu di novu '''Sarva''' lu canciamentu veni sarvatu cu l'uggettu vacanti.",
'missingcommenttext'               => 'Nziriri un cummentu ccà sutta.',
'missingcommentheader'             => "'''Accura:''' Nun hà statu spicificatu la ntistazzioni di stu cummentu. Primennu di novu '''Sarva''' lu canciamentu veni saravtu senza ntistazzioni.",
'summary-preview'                  => 'Antiprima uggettu',
'subject-preview'                  => 'Antiprima suggettu/ntistazzioni',
'blockedtitle'                     => 'Utenti bluccatu.',
'blockedtext'                      => "<big>'''Stu nomu d'utenti o nnirizzu IP havi statu bluccatu.'''</big>

Lu bloccu fu fattu di $1. Lu mutivu dû bloccu è: ''$2''.

* Accuminzata dû bloccu: $8
* Fini dû bloccu: $6
* Ntirvallu dû bloccu: $7

Poi cuntattari a $1 o a n'àutru [[{{MediaWiki:Grouppage-sysop}}|amministraturi]] pi discùtiri dû bloccu.

Nun poi usari la carattirìstica 'manna n'email a st'utenti' siddu nun è spicificatu nu nnirizzu email vàlidu nta li toi [[Special:Preferences|prifirenzi]] e siddu nun hai statu bluccatu di l'usari.

Lu tò nnirizzu IP attuali è $3, e lu nùmmiru ID dû bloccu è #$5. 

Spicìfica tutti li dittagghi pricidenti nta quarsiasi addumannata di chiarimenti.",
'autoblockedtext'                  => "Lu tò nnirizzu IP hà statu bluccatu automaticamenti picchì fu usatu di n'àutru utenti, chi fu bluccatu di $1.
Lu mutivu è chistu:

:''$2''

* Accuminzata dû bloccu: $8
* Fini dû bloccu: $6
* Ntirvallu dû bloccu: $7

Poi cuntattari a $1 o a n'àutru [[{{MediaWiki:Grouppage-sysop}}|amministraturi]] pi discùtiri dû bloccu.

Nun poi usari la carattirìstica 'manna n'email a st'utenti' siddu nun è spicificatu nu nnirizzu email vàlidu nta li toi [[Special:Preferences|prifirenzi]] e siddu nun hai statu bluccatu di l'usari.

L'ID dû bloccu è $5. Pi favuri nclùdilu nta tutti li dumanni.",
'blockednoreason'                  => 'nudda motivazioni ndicata',
'blockedoriginalsource'            => "Di sèquitu veni ammustratu lu còdici surgenti dâ pàggina '''$1''':",
'blockededitsource'                => "Di sèquitu vèninu ammustrati li '''canciamenti appurtati''' â pàggina '''$1''':",
'whitelistedittitle'               => 'Ci voli èssiri riggistrati pi putiri canciari la pàggina.',
'whitelistedittext'                => "Hai a $1 pi canciari l'artìculi.",
'confirmedittitle'                 => 'Cunferma dâ e-mail nicissaria pi lu canciamentu dî pàggini',
'confirmedittext'                  => "P'èssiri abbilitati a lu canciamentu dî pàggini è nicissariu cunfirmari lu propiu ndirizzu e-mail. Pi mpustari e cunfirmari lu ndirizzu sirvìrisi dî [[Special:Preferences|prifirenzi]].",
'nosuchsectiontitle'               => 'Sta sezzioni nun esisti',
'nosuchsectiontext'                => 'Pruvasti a canciari na sezzioni chi nun esisti. Li tò canciamenti nun ponnu èssiri sarvati, picchì nun esisti la sezzioni $1.',
'loginreqtitle'                    => 'Login nicissariu',
'loginreqlink'                     => "esèquiri l'accessu",
'loginreqpagetext'                 => 'Pi vìdiri àutri pàggini è nicissariu $1.',
'accmailtitle'                     => 'Password nviata.',
'accmailtext'                      => 'La password pi l\'utenti "$1" fu nviata a lu ndirizzu $2.',
'newarticle'                       => '(Novu)',
'newarticletext'                   => "Sta pàggina ancora nun esisti. 
Pi criari na pàggina cu stu tìtulu, accumenza a scrìviri ccassutta (talìa la [[{{MediaWiki:Helppage}}|pàggina d'aiutu]] pi aviri maiuri nfurmazzioni).
Si agghicasti ccà pi sbagghiu, clicca lu buttuni ''''n arreri (back)''' dû tò browser.",
'anontalkpagetext'                 => "----''Chista è la pàggina di discussioni di n’utenti anònimu, ca nun hà ancora criatu n’accessu o comu è gghiè nun l’usa. P’idintificàrilu è quinni nicissariu usari lu nùmmiru di lu sò nnirizzu IP. Li nnirizzi IP ponnu pirò èssiri cunnivisi di cchiù utenti. Siddu sî n’utenti anònimu e riteni ca li cummenti prisenti nta sta pàggina nun si rifirìscinu a tia, [[Special:UserLogin|crea n’accessu novu o trasi]] cu chiddu ca già hai p’evitari d’èssiri cunfusu cu àutri utenti anònimi ‘n futuru''",
'noarticletext'                    => "Nta stu mumentu la pàggina richiesta è vacanti. È pussìbbili [[Special:Search/{{PAGENAME}}|circari stu tìtulu]] nta l'àutri pàggini dû situ oppuru [{{fullurl:{{FULLPAGENAME}}|action=edit}} canciari la pàggina ora].",
'userpage-userdoesnotexist'        => 'L\'account "$1" nun currispunni a n\'utenti riggistratu. Virificari si si voli criari o canciari sta pàggina.',
'clearyourcache'                   => "'''Nota:''' doppu aviri sarvatu è nicissariu puliri la cache dû propiu browser pi vìdiri li canciamenti. Pi '''Mozilla / Firefox / Safari''': fari clic supra ''Ricarica'' tinnennu primutu lu tastu dî maiùsculi, oppuru prèmiri ''Ctrl-Maiusc-R'' (''Cmd-Maiusc-R'' supra Mac); pi '''Internet Explorer:''' mantèniri primutu lu tastu ''Ctrl'' mentri si premi lu pulsanti ''Aggiorna'' o prèmiri ''Ctrl-F5''; pi '''Konqueror''': prèmiri lu pulsanti ''Ricarica'' o lu tastu ''F5''; pi '''Opera''' pò èssiri nicissariu svacantari cumpletamenti la cache dû menu ''Strumenti → Preferenze''.",
'usercssjsyoucanpreview'           => "<strong>Suggirimentu:</strong> Usa lu tastu 'Visualizza antiprima' pi pruvari li novi css/js prima di sarvàrili.",
'usercsspreview'                   => "'''Arricorda ca stai sulu visualizzannu n'antiprima dû tò CSS pirsunali.'''
'''Nun hà ancora statu sarvatu!'''",
'userjspreview'                    => "'''Arricorda ca stai sulu tistanno/vidennu 'n antiprima lu tò javascript pirsunali, nun hà statu ancora sarvatu!'''",
'userinvalidcssjstitle'            => "'''Accura:''' Nun esisti arcuna skin cu nomu \"\$1\". S'arricorda ca li pàggini pi li .css e .js pirsunalizzati hannu la nizziali dû tìtulu minùscula, p'asempiu {{ns:user}}:Asempiu/monobook.js e nun {{ns:user}}:Asempiu/Monobook.css.",
'updated'                          => '(Aggiurnatu)',
'note'                             => '<strong>Accura:</strong>',
'previewnote'                      => "<strong>Ricurdàtivi ca chista è sulu n'antiprima, e ca nun hà statu ancora sarvata!</strong>",
'previewconflict'                  => "L'antiprima currispunni a lu testu prisenti ntâ casella di canciamentu supiriuri e rapprisenta la pàggina comu appari siddu si scegghi di prèmiri 'Sarva' 'n stu mumentu.",
'session_fail_preview'             => "<strong>Purtroppu nun hà statu pussìbbili sarvari li tò canciamenti pirchì li dati dâ sissioni hannu jutu pirduti. Pi favuri, riprova. Siddu arricevi stu missaggiu d'erruri cchiù voti, prova a sculligàriti e a culligàriti novamenti.</strong>",
'session_fail_preview_html'        => "<strong>Semu spiacenti, nun hà statu pussìbbili elabburari lu canciamentu pirchì hannu jutu pirduti li dati rilativi â sissioni.</strong>

''Poichì nta stu situ è abbilitatu l'usu di HTML senza limitazzioni, l'antiprima nun veni visualizzata; si tratta di na misura di sicurizza contra l'attacchi JavaScript.''

<strong>Siddu chistu è nu tintativu liggìttimu di canciamentu, arriprova. Siddu lu prubbrema pirsisti, si pò pruvari a [[Special:UserLogout|sculligàrisi]] e effittuari n'accessu novu.</strong>",
'token_suffix_mismatch'            => "<strong>Lu canciamentu nun fu sarvatu pirchì lu client ammustrau di gèstiri 'n modu sbagghiatu li caràttiri di puntiggiatura nta lu token assuciatu a iddu. P'evitari na curruzzioni pussìbbili dô testu dâ pàggina, fu rifiutatu tuttu lu canciamentu. Sta situazzioni pò virificàrisi, certi voti, quannu s'adòpiranu arcuni sirvizza di proxy anònimi via web chi prisèntanu bug.</strong>",
'editing'                          => 'Canciu di la vuci "$1"',
'editingsection'                   => 'Canciamentu di $1 (sezzioni)',
'editingcomment'                   => 'Canciu di $1 (cummentu)',
'editconflict'                     => "Cunflittu d'edizzioni supra $1",
'explainconflict'                  => "N'àutru utenti havi sarvatu na virsioni nova dâ pàggina mentri stavi effittuannu li canciamenti.<br /> La casella di canciamentu supiriuri cunteni lu testu dâ pàggina attuarmenti online, accussì comu hà statu aggiurnatu di l'àutru utenti. La virsioni cu li tò canciamenti è mmeci ripurtata ntâ casella di canciamentu nfiriuri. Siddu addisìi cunfirmàrili, hai a ripurtari li tò canciamenti ntô testu asistenti (casella supiriuri). Primennu lu pulsanti 'Sarva la pàggina', veni sarvatu <b>sulu</b> lu testu cuntinutu ntâ casella di canciamentu supiriuri.<br />",
'yourtext'                         => 'Lu tò testu',
'storedversion'                    => 'La virsioni mimurizzata',
'nonunicodebrowser'                => "<strong>'''ACCURA: Lu tò browser nun supporta unicode, li caràttiri nun-ASCII appàrinu nta lu box di canciamentu comu còdici esadicimali.'''</strong>",
'editingold'                       => '<strong>Accura: si sta canciannu na virsioni nun aggiurnata dâ pàggina.<br /> Siddu si scegghi di sarvàrila, tutti li canciamenti appurtati doppu sta rivisioni vannu pirduti.</strong>',
'yourdiff'                         => 'Diffirenzi',
'copyrightwarning'                 => "Nutati chi tutti li cuntribbuti a {{SITENAME}} s'hannu a cunzidirari sutta la licenza d'usu $2 (talìa $1 pî dittagghi). Si nun vuliti chi lu vostru travagghiu curri lu rìsicu di vèniri ritravagghiatu e/o ridistribbuitu, nun suttamittìtilu ccà.<br />
Vuatri prumittiti puru chi lu scrivìstivu chî vostri palori, o chi lu cupiàstivu di nu duminiu pùbbricu o di risursi sìmili
<strong>NUN SUTTAMITTÌTI MATIRIALI SUTTA COPYRIGHT SENZA PIRMISSU!</strong>",
'copyrightwarning2'                => "Nota: tutti li cuntribbuti mannati a {{SITENAME}} ponnu èssiri mudificati o cancillati di parti di l'àutri participanti. Siddu nun addisìi ca li tò testi ponnu èssiri mudificati senza arcunu riguardu, nun mannàrili a stu situ.<br /> Cu la mannata dû testu dichiari noltri, sutta la tò rispunzabbilitati, ca lu testu hà statu scrittu di tia pirsunalmenti oppuru c'hà statu cupiatu di na fonti di pùbbricu dominiu o analucamenti lìbbira. (vidi $1 pi maiuri dittagghi) <strong>NUN MANNARI MATIRIALI CUPERTU DI DRITTU D'AUTURI SENZA AUTURIZZAZZIONI!</strong>",
'longpagewarning'                  => "<strong>ACCURA: Sta pàggina è longa $1 kilobyte. Arcuni browser putìssiru prisintari dî prubbremi ntô canciari pàggini ca s'avvicìnanu o sùpiranu 32kb. Pi favuri pigghia n cunzidirazzioni la pussibbilitati di suddivìdiri la pàggina n sezzioni cchiù nichi.</strong>",
'longpageerror'                    => "<strong>ERRURI: Lu testu ca hai suttamissu è longu $1 kilobyte, ch'è cchiù dû màssimu di $2 kilobyte. Nun pò èssiri sarvatu.</strong>",
'readonlywarning'                  => "<strong>ACCURA: lu database è fermu pi manutinzioni, pirciò nun poi sarvari li tò canciamenti nta stu mumentu. La cosa megghia è fari un copia e ncolla dû testu nta n'àutru prugramma e sarvàrilu pi quannu lu database è accissìbbili.</strong>",
'protectedpagewarning'             => "<strong>ACCURA: Sta pàggina havi na prutizzioni spiciali e sulu l'utenti chi hannu lu status di amministraturi ponnu canciàrila.</strong>",
'semiprotectedpagewarning'         => "'''ACCURA:''' Sta pàggina hà statu bluccata n modu ca sulu li utenti riggistrati ponnu canciàrila.",
'cascadeprotectedwarning'          => "'''Accura:''' Sta pàggina havi stata bluccata n modu ca sulu li utenti cu privileggi di amministraturi ponnu mudificàrila, pirchì veni nclusa {{PLURAL:\$1|nta siquente pàggina ca hà stata prutiggiuta|ntê siquenti pàggini ca hannu stati prutiggiuti}} silizziunannu la prutizzioni \"ricursiva\":",
'titleprotectedwarning'            => '<strong>ATTENZIONI:  Sta pàggina fu bluccata n modu tali ca sulu alcuni catigurìi di utenti la ponu criari.</strong>',
'templatesused'                    => "Template utilizzati 'n sta pàggina:",
'templatesusedpreview'             => "Template utilizzati 'n st'antiprima:",
'templatesusedsection'             => "Template utilizzati 'n sta sezzioni:",
'template-protected'               => '(prutettu)',
'template-semiprotected'           => '(semiprutettu)',
'hiddencategories'                 => 'Sta pàggina apparteni a {{PLURAL:$1|na catigurìa ammuciata|$1 catigurìi ammuciati}}:',
'edittools'                        => '<!-- Chistu testu cumpari sutta li moduli di canciu e carricamentu. -->',
'nocreatetitle'                    => 'Criazzioni dî pàggini limitata',
'nocreatetext'                     => "La pussibbilitati di criari pàggini novi nta {{SITENAME}} è limitata a l'utenti riggistrati. Poi turnari 'n arreri e canciari na pàggina esistenti, oppuru [[Special:UserLogin|tràsiri o criari nu cuntu novu]].",
'nocreate-loggedin'                => 'Nun hai lu pirmissu pi criari pàggini novi nta {{SITENAME}}.',
'permissionserrors'                => 'Erruri di pirmissu',
'permissionserrorstext'            => 'Nun hai lu pirmissu pi fari chistu, pi {{PLURAL:$1|chistu motivu|sti mutivi}}:',
'permissionserrorstext-withaction' => 'Nun hai lu pirmessu di fari $2, pi {{PLURAL:$1|lu siguenti mutivu|li siguenti mutivi}}:',
'recreate-deleted-warn'            => "'''Accura: stai pi criari na pàggina chi fu cancillata 'n passatu.'''

Accuràtivi ch'è uppurtunu cuntinuari a canciari sta pàggina.
L'alencu dî cancillazzioni rilativi veni ripurtatu ccà pi cummudità:",

# Parser/template warnings
'expensive-parserfunction-warning'        => 'Attenzioni: Sta pàggina cunteni troppi chiamati ê parser functions.

Avissi essiri menu di $2, al momentu ci sunu $1.',
'expensive-parserfunction-category'       => 'Pàggini cu troppi chiamati ê parser functions',
'post-expand-template-inclusion-category' => 'Pàggini unni la diminsioni dê template nclusi supira lu limiti cunsintutu',
'post-expand-template-argument-warning'   => "Attenzioni: Sta pàggina cunteni almenu n'argomentu di nu template ca havi na diminsioni troppu rossa pi essiri espansu. St'argomenti verrannu omessi.",

# "Undo" feature
'undo-success' => "Lu canciamentu hà statu annullatu cu successu. Virificati lu cunfruntu prisintatu ccà sutta p'accuràrivi ca lu cuntinutu è chiddu addisiatu e doppu sarvati la pàggina pi cumplitari l'annullamentu.",
'undo-failure' => "Lu canciamentu nun pò èssiri annullatu a càusa d'un cunflittu cu li canciamenti ntermedi.",
'undo-norev'   => 'Lu canciamentu nun pò essiri annullatu pirchì nun esisti o fù cancillato.',
'undo-summary' => 'Annullatu lu canciamentu $1 di [[Special:Contributions/$2|$2]] ([[User talk:$2|discussioni]])',

# Account creation failure
'cantcreateaccounttitle' => "Mpussìbbili riggistrari n'utenti",
'cantcreateaccount-text' => "La criazzioni di account di stu nnirizzu IP ('''$1''') fu bluccata di [[User:$3|$3]].

Lu mutivu è ''$2''",

# History pages
'viewpagelogs'        => 'Vidi li log rilativi a sta pàggina',
'nohistory'           => 'Cronoluggìa dî virsioni di sta pàggina nun ripirìbbili.',
'revnotfound'         => 'Virsioni nun attruvata',
'revnotfoundtext'     => "La virsioni pricidenti di st'artìculu c'hai addumannatu nun hà statu attruvata. Cuntrolla pi favuri la URL c'hai usatu p'accèdiri a sta pàggina.",
'currentrev'          => 'Virsioni currenti',
'revisionasof'        => 'Virsioni dû $1',
'revision-info'       => 'Virsioni dû $1 di $2',
'previousrevision'    => '← Virsioni menu ricenti',
'nextrevision'        => 'Virsioni cchiù ricenti →',
'currentrevisionlink' => 'Virsioni currenti',
'cur'                 => 'curr',
'next'                => 'pròssimu',
'last'                => 'pric',
'page_first'          => 'prima',
'page_last'           => 'ùrtima',
'histlegend'          => "Cunfrontu tra virsioni: silizziunari li caselli currispunnenti ê virsioni addisiati e prèmiri Mannu o lu pulsanti a basciu.<br /> Liggenna: (curr) = diffirenzi cu la virsioni attuali, (pric) = diffirenzi cu la virsioni pricidenti, '''m''' = canciamentu nicu",
'deletedrev'          => '[cancillata]',
'histfirst'           => 'Prima',
'histlast'            => 'Ùrtima',
'historysize'         => '({{PLURAL:$1|1 byte|$1 byte}})',
'historyempty'        => '(vacanti)',

# Revision feed
'history-feed-title'          => 'Lista dî canciamenti',
'history-feed-description'    => 'Cronoluggìa dâ pàggina supra stu situ',
'history-feed-item-nocomment' => '$1 lu $2', # user at time
'history-feed-empty'          => 'La pàggina richiesta nun asisti; putissi aviri stata cancillata dû situ o rinuminata. Virificari cu la [[Special:Search|pàggina di ricerca]] siddu ci sunnu novi pàggini.',

# Revision deletion
'rev-deleted-comment'         => '(cummentu rimussu)',
'rev-deleted-user'            => '(nomu utenti rimussu)',
'rev-deleted-event'           => '(elementu cancillatu)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks"> Sta virsioni dâ pàggina hà statu rimussa di l\'archivi visìbbili a lu pùbbricu. Cunzurtari lu [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} log di cancillazzioni] pi ultiriuri dittagghi. </div>',
'rev-deleted-text-view'       => '<div class="mw-warning plainlinks"> Sta virsioni dâ pàggina hà statu rimussa di l\'archivi visìbbili a lu pùbbricu. Lu testu pò èssiri visualizzatu surtantu di l\'amministratura dû situ. Cunzurtari lu [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} log di cancillazzioni] pi ultiriuri dittagghi. </div>',
'rev-delundel'                => 'ammustra/ammuccia',
'revisiondelete'              => 'Cancella o riprìstina virsioni',
'revdelete-nooldid-title'     => 'Virsioni nun spicificata',
'revdelete-nooldid-text'      => 'Nun hà statu spicificata arcuna virsioni dâ pàggina supra cui esèquiri sta funzioni.',
'revdelete-selected'          => '{{PLURAL:$2|Virsioni silizziunata|Virsioni silizziunati}} di [[:$1]]:',
'logdelete-selected'          => '{{PLURAL:$1|Eventu dû riggistru silizziunatu|Eventi dû riggistru silizziunati}}:',
'revdelete-text'              => "Li virsioni cancillati rèstanu visìbbili ntâ cronoluggìa dâ pàggina, mentri lu testu cuntinutu nun è accissìbbili a lu pùbbricu. L'àutri amministratura dû situ ponnu accèdiri comu è gghiè a li cuntinuti ammucciati e ripristinàrili attraversu sta stissa nterfaccia, siddu nun hannu statu mpustati àutri limitazzioni n fasi di nstallazzioni dû situ.",
'revdelete-legend'            => 'Mposta li limitazzioni siquenti supra li virsioni cancillati:',
'revdelete-hide-text'         => 'Ammuccia lu testu dâ virsioni',
'revdelete-hide-name'         => 'Ammuccia azione e uggettu dâ stissa',
'revdelete-hide-comment'      => "Ammuccia l'uggettu dû canciamentu",
'revdelete-hide-user'         => "Ammuccia lu nomu o lu ndirizzu IP di l'auturi",
'revdelete-hide-restricted'   => 'Àpplica li limitazzioni innicati macari a li amministratura',
'revdelete-suppress'          => "Ammuccia nformazioni puru all'amministratura",
'revdelete-hide-image'        => 'Ammuccia lu cuntinutu dû file',
'revdelete-unsuppress'        => 'Elìmina le limitazzioni su li rivisioni ripristinati',
'revdelete-log'               => 'Cummentu pi lu log:',
'revdelete-submit'            => 'Àpplica â rivisioni silizziunata',
'revdelete-logentry'          => 'hà canciatu la visibbilitati pi na rivisioni di [[$1]]',
'logdelete-logentry'          => "hà canciatu la visibbilitati de l'eventu [[$1]]",
'revdelete-success'           => "'''Visibbilitati dâ rivisioni mpustata currittamenti.'''",
'logdelete-success'           => "'''Visibbilitati de l'eventu mpustata currittamenti.'''",
'revdel-restore'              => 'Cancia la visibbilità',
'pagehist'                    => 'Storia dâ pàggina',
'deletedhist'                 => 'Storia cancillata',
'revdelete-content'           => 'cuntinutu',
'revdelete-summary'           => 'riassuntu dô canciamentu',
'revdelete-uname'             => 'nnomu utenti',
'revdelete-restricted'        => 'ristrizzioni ai suli amministratura attivate',
'revdelete-unrestricted'      => 'ristrizzioni pi suli amministraturi rimossi',
'revdelete-hid'               => 'ammuccia $1',
'revdelete-unhid'             => 'renni visibbili $1',
'revdelete-log-message'       => '$1 pi $2 {{PLURAL:$2|rivisione|rivisioni}}',
'logdelete-log-message'       => '$1 pi $2 {{PLURAL:$2|eventu|eventi}}',

# Suppression log
'suppressionlog'     => 'Log dê supprissioni',
'suppressionlogtext' => "Ccà veni prisintatu n'elencu dê cancillazioni e dê blocchi cchiù ricenti supra cuntinuti ammucciati d'amministraturi. Vidi l'[[Special:IPBlockList|elenco d'IP bloccati]] pi l'elencu dî ban e dî blocchi attivi.",

# History merging
'mergehistory'                     => 'Unioni storie',
'mergehistory-header'              => "Sta pàggina fa junciri li rivisioni dâ storia di na pàggina (ditta macari pàggina d'origini) cu na pàggina cchiù ricenti.
S'havi accirtari ca la cuntinuità storica di la pàggina nun veni altirata.",
'mergehistory-box'                 => 'Junci li storii di dui pàggini:',
'mergehistory-from'                => 'Pàggina di origgini:',
'mergehistory-into'                => 'Pàggina di distinazioni:',
'mergehistory-list'                => "Storia a cui è applicabili l'unioni",
'mergehistory-merge'               => 'È possibili junciri li rivisioni di [[:$1]] ndicati ccà â storia di [[:$2]]. Usari la colunna cu li pulsanti di opzioni pi junciri tutti li rivisioni finu â data e ura ndicati. Talìa ca si venunu usati li pulsanti di navigazzioni, la colonna di li pulsanti di opzioni veni azzirata.',
'mergehistory-go'                  => 'Vidi li canciamenti ca ponu essiri junciuti',
'mergehistory-submit'              => 'Junci li rivisioni',
'mergehistory-empty'               => 'Nudda rivisioni da junciri.',
'mergehistory-success'             => '{{PLURAL:$3|Na rivisioni di [[:$1]] fu junciuta|$3 rivisioni di [[:$1]] sunu stati junciuti}} â storia di [[:$2]].',
'mergehistory-fail'                => 'Impossibbili junciri li storii. Virificari la pàggina e li parametri temporali.',
'mergehistory-no-source'           => 'La pàggina di origgini $1 nun esisti.',
'mergehistory-no-destination'      => 'La pàggina di distinazzioni $1 nun esisti.',
'mergehistory-invalid-source'      => 'La pàggina di origgini havi aviri nu titulu currettu.',
'mergehistory-invalid-destination' => 'La pàggina di distinazzioni havi aviri nu tìtulu currettu.',
'mergehistory-autocomment'         => 'Unioni di [[:$1]] ni [[:$2]]',
'mergehistory-comment'             => 'Unioni di [[:$1]] in [[:$2]]: $3',

# Merge log
'mergelog'           => "Log d'unioni",
'pagemerge-logentry' => 'havi iunciutu [[$1]] a [[$2]] (rivisioni finu a $3)',
'revertmerge'        => 'Annulla unioni',
'mergelogpagetext'   => "Appressu veni ammustrata na lista dî operazioni cchiù ricenti di unioni dâ storia di na pàggina ni n'autra.",

# Diffs
'history-title'           => 'Crunoluggìa dî canciamenti di "$1"',
'difference'              => '(Diffirenzi tra li rivisioni)',
'lineno'                  => 'Lìnia $1:',
'compareselectedversions' => 'Fari lu paraguni',
'editundo'                => 'annulla',
'diff-multi'              => '({{PLURAL:$1|Na rivisioni ntermedia nun ammustrata|$1 rivisioni ntermedi nun ammustrati}}.)',

# Search results
'searchresults'             => 'Risurtati dâ circata',
'searchresulttext'          => 'Pi maiuri nformazzioni supra la ricerca nterna di {{SITENAME}}, talìa [[{{MediaWiki:Helppage}}|{{int:help}}]].',
'searchsubtitle'            => "Pruvasti a circari: '''[[$1]]'''",
'searchsubtitleinvalid'     => "Circata di '''$1'''",
'noexactmatch'              => "'''Nun c'è na pàggina chi si ntìtula \"\$1\".''' Putiti [[:\$1|criari sta pàggina]].",
'noexactmatch-nocreate'     => "'''La pàggina cu lu tìtulu \"\$1\" nun esisti.'''",
'toomanymatches'            => 'Troppi currispunnenzi. Cancia la richiesta.',
'titlematches'              => "Ntê tìtuli di l'artìculi",
'notitlematches'            => 'Nudda currispunnenza ntê tìtuli dî pàggini',
'textmatches'               => "Ntô testu di l'artìculi",
'notextmatches'             => 'Nudda currispunnenza ntô testu dî pàggini',
'prevn'                     => 'li pricidenti $1',
'nextn'                     => 'li pròssimi $1',
'viewprevnext'              => 'Talìa ($1) ($2) ($3).',
'search-result-size'        => '$1 ({{PLURAL:$2|na parola|$2 paroli}})',
'search-result-score'       => 'Rilivanza: $1%',
'search-redirect'           => '(redirect $1)',
'search-section'            => '(sizzioni $1)',
'search-suggest'            => 'Forsi circavutu: $1',
'search-interwiki-caption'  => 'Pruggetti frati',
'search-interwiki-default'  => 'Risultati da $1:',
'search-interwiki-more'     => '(cchiù)',
'search-mwsuggest-enabled'  => 'cu suggirimenti',
'search-mwsuggest-disabled' => 'senza suggirimenti',
'search-relatedarticle'     => 'Risultati currilati',
'mwsuggest-disable'         => 'Astuta suggirimenti AJAX',
'searchrelated'             => 'currilati',
'searchall'                 => 'tutti',
'showingresults'            => "Ammustra nzinu a {{PLURAL:$1|'''1''' risurtatu|'''$1''' risurtati}} a pàrtiri dô nùmmuru '''$2'''.",
'showingresultsnum'         => "L'alencu cunteni {{PLURAL:$3|'''1''' risurtatu|'''$3''' risurtati}} a pàrtiri dû nùmmuru '''$2'''.",
'showingresultstotal'       => "Appressu {{PLURAL:$3|veni ammustratu lu risurtatu '''$1''' di '''$3'''|venunu ammustrati li risultati '''$1 - $2''' di '''$3'''}}",
'nonefound'                 => "'''Nota''': la circata è effittuata pi default sulu nta arcuni namespace. Prova a primèttiri ''all:'' ô testu dâ circata pi circari nta tutti li namespace (cumprisi pàggini di discussioni, template, ecc) oppuru usa lu namespace disidiratu comu prifissu.",
'powersearch'               => 'Arriscedi',
'powersearch-legend'        => 'Ricerca avanzata',
'search-external'           => 'Ricerca sterna',
'searchdisabled'            => 'La circata nterna di {{SITENAME}} hà statu disabbilitata. Nta stu mentri, poi usari la circata supra Google o supra àutri muturi di circata. Accura ca li sò ìnnici dê cuntinuti di {{SITENAME}} ponnu nun èssiri aggiurnati.',

# Preferences page
'preferences'              => 'prifirenzi',
'mypreferences'            => 'Li mè prifirenzi',
'prefs-edits'              => 'Nùmmuru di canciamenti:',
'prefsnologin'             => 'Accessu nun effittuatu',
'prefsnologintext'         => "Pi putiri pirsunalizzari li prifirenzi è nicissariu effittuari l'[[Special:UserLogin|accessu]].",
'prefsreset'               => 'Li prifirenzi hannu statu ripristinati a li valura pridifiniti.',
'qbsettings'               => 'Pusizzioni QuickBar',
'qbsettings-none'          => 'Nuddu',
'qbsettings-fixedleft'     => 'Fissu a manu manca',
'qbsettings-fixedright'    => 'Fissu a manu dritta',
'qbsettings-floatingleft'  => 'Fluttuanti a manu manca',
'qbsettings-floatingright' => 'Fluttuanti a manu dritta',
'changepassword'           => 'Cancia la password',
'skin'                     => 'Aspettu',
'math'                     => 'Fòrmuli',
'dateformat'               => 'Furmatu dâ data',
'datedefault'              => 'Nudda prifirenza',
'datetime'                 => 'Data e ura',
'math_failure'             => "S'hà virificatu un erruri ntô parsing",
'math_unknown_error'       => 'erruri scanusciutu',
'math_unknown_function'    => 'funzioni scanusciuta',
'math_lexing_error'        => 'erruri lissicali',
'math_syntax_error'        => 'erruri di sintassi',
'math_image_error'         => "Cunvirsioni 'n PNG fallita; virificati la curretta nstallazzioni dî siquenti prugrammi: latex, dvips, gs e convert.",
'math_bad_tmpdir'          => 'Mpussìbbili scrìviri o criari la directory timpurània pi math',
'math_bad_output'          => 'Mpussìbbili scrìviri o criari la directory di output pi math',
'math_notexvc'             => 'Esiquìbbili texvc mancanti; pi favuri cunzurtari math/README pi la cunfigurazzioni.',
'prefs-personal'           => 'Prufilu utenti',
'prefs-rc'                 => 'Ùrtimi canciamenti',
'prefs-watchlist'          => 'Ossirvati spiciali',
'prefs-watchlist-days'     => "Nùmmiru di jorna ammustrati nta l'ossirvati spiciali:",
'prefs-watchlist-edits'    => 'Nùmmaru di canciamenti a ammustrari cu li funzioni avanzati:',
'prefs-misc'               => 'Vari',
'saveprefs'                => 'Sarva li prifirenzi',
'resetprefs'               => 'Annulla',
'oldpassword'              => 'Password vecchia:',
'newpassword'              => 'Password nova:',
'retypenew'                => "Scrivi n'àutra vota la password",
'textboxsize'              => 'Cancia',
'rows'                     => 'Righi:',
'columns'                  => 'Culonni:',
'searchresultshead'        => 'Circata',
'resultsperpage'           => 'Nùmmiru di risurtati pi pàggina:',
'contextlines'             => 'Righi di testu pi ognunu risurtatu:',
'contextchars'             => 'Nùmmaru di caràttiri di cuntestu:',
'stub-threshold'           => 'Valuri minimu pî <a href="#" class="stub">liami a li stub</a>:',
'recentchangesdays'        => "Nùmmuru di jorna a ammustrari nte l'urtimi cancaiamenti:",
'recentchangescount'       => "Nùmmiru di righi nta l'ùrtimi canciamenti",
'savedprefs'               => 'Li tò prifirenzi foru sarvati.',
'timezonelegend'           => 'Zona oraria',
'timezonetext'             => "Mmetti lu nùmmiru d'uri di diffirenza tra la tò ura lucali e l'ura dû server (UTC).",
'localtime'                => 'Ura lucali',
'timezoneoffset'           => 'Uri di diffirenza¹',
'servertime'               => 'Ura dû server',
'guesstimezone'            => "Usa l'ura dû tò browser",
'allowemail'               => 'Cunzenti la ricezzioni di e-mail di àutri utenti',
'defaultns'                => 'Namespace pridifiniti pi la ricerca:',
'default'                  => 'pridifinitu',
'files'                    => 'Mmàggini',

# User rights
'userrights'                  => 'Gistioni dî dritti utenti', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => "Gistisci li gruppi di l'utenti",
'userrights-user-editname'    => "Trasi nu nomu d'utenti:",
'editusergroup'               => 'Cancia gruppi utenti',
'editinguser'                 => "Canciamentu dî dritti di l'utenti '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup'    => "Cancia li gruppi di l'utenti",
'saveusergroups'              => 'Sarva gruppi utenti',
'userrights-groupsmember'     => 'Membru di:',
'userrights-groups-help'      => "È pussibili canciari li gruppi cui è assegnatu l'utenti.
* Na casedda di spunta silizzionata ndica l'appartinenza dill'utenti ô gruppu
* Na casedda di spunta nun silizzionata ndica la sou mancata appartinenza ô gruppu.
* Lu simbulu * ndica ca nun è pussibili livari l'appartinenza ô gruppo dopo avirla junciuta (o vici versa).",
'userrights-reason'           => 'Mutivu dû canciu:',
'userrights-no-interwiki'     => "Nun si disponi di li pirmessi nicissari pi canciari li diritti di l'utenti ni autri siti.",
'userrights-nodatabase'       => 'Lu database $1 nu esisti o nun è lu database locali.',
'userrights-nologin'          => "Pi assignari li diritti di l'utenti è nicissariu [[Special:UserLogin|trasiri]] comu amministraturi.",
'userrights-notallowed'       => "L'utenti nun disponi dê pirmessi nicissari pi assignari diritti all'utenti.",
'userrights-changeable-col'   => 'Gruppi canciabili',
'userrights-unchangeable-col' => 'Gruppi nun canciabili',

# Groups
'group'               => 'Gruppu:',
'group-user'          => 'Utenti',
'group-autoconfirmed' => 'Utenti autocunfirmati',
'group-bot'           => 'Bot',
'group-sysop'         => 'Amministratura',
'group-bureaucrat'    => 'Buròcrati',
'group-suppress'      => 'Oversight',
'group-all'           => 'Utenti',

'group-user-member'          => 'Utenti',
'group-autoconfirmed-member' => 'Utenti autocunfirmatu',
'group-bot-member'           => 'Bot',
'group-sysop-member'         => 'Amministraturi',
'group-bureaucrat-member'    => 'Buròcrati',
'group-suppress-member'      => 'Oversight',

'grouppage-user'          => '{{ns:project}}:Utenti',
'grouppage-autoconfirmed' => '{{ns:project}}:Utenti autocunfirmati',
'grouppage-bot'           => '{{ns:project}}:Bot',
'grouppage-sysop'         => '{{ns:project}}:Amministratura',
'grouppage-bureaucrat'    => '{{ns:project}}:Buròcrati',
'grouppage-suppress'      => '{{ns:project}}:Oversight',

# Rights
'right-read'             => 'Leggi pàggini',
'right-edit'             => 'Cancia pàggini',
'right-createpage'       => 'Crea pàggini',
'right-createtalk'       => 'Crea pàggini di discussioni',
'right-createaccount'    => 'Crea novi account utenti',
'right-minoredit'        => 'Segna li canciamenti comu nichi',
'right-move'             => 'Sposta pàggini',
'right-suppressredirect' => 'Cancella nu redirect quannu sposti na pàggina a du tìtulu',
'right-upload'           => 'Carica file',
'right-reupload'         => 'Sovrascrivi nu file esistenti',
'right-reupload-own'     => 'Sovrascrivi nu file esistenti caricatu dô stissu utenti',
'right-upload_by_url'    => 'Carica nu file da nu ndirizzu URL',
'right-autoconfirmed'    => 'Cancia pàggini semiprotetti',
'right-browsearchive'    => 'Talìa pàggini cancillati',
'right-undelete'         => 'Riprìstina na pàggina',
'right-suppressrevision' => 'Ritalìa e riprìstina virsioni ammucciati',
'right-suppressionlog'   => 'Talìa li log privati',
'right-block'            => 'Blocca li canciamenti da parti di autri utenti',

# User rights log
'rightslog'      => "Dritti di l'utenti",
'rightslogtext'  => "Chistu è un log dî canciamenti a li dritti di l'utenti.",
'rightslogentry' => "hà canciatu l'appartinenza di $1 dû gruppu $2 a lu gruppu $3",
'rightsnone'     => '(nuddu)',

# Recent changes
'nchanges'                          => '$1 {{PLURAL:$1|canciamentu|canciamenti}}',
'recentchanges'                     => 'Ùrtimi canciamenti',
'recentchangestext'                 => 'Chista pàggina prisenta li canci cchiù ricenti ê cuntinuti dô situ.',
'recentchanges-feed-description'    => 'Stu feed riporta li canciamenti cchiù ricenti a li cuntinuti dû situ.',
'rcnote'                            => "Ccà sutta {{PLURAL:$1|c'è lu canciamentu cchiù ricenti appurtatu|cci sunnu l'ùrtimi '''$1''' canciamenti appurtati}} ô situ {{PLURAL:$2|nta l'ùrtimi 24 uri|nta l'ùrtimi '''$2''' giorni}}; li dati sunnu aggiurnati ê $5 dû $4.",
'rcnotefrom'                        => 'Ccà sutta cci sunnu li canciamenti a pàrtiri dû <b>$2</b> (ammustrati nzinu ô <b>$1</b>).',
'rclistfrom'                        => 'Ammustra li canciamenti novi a pàrtiri di $1',
'rcshowhideminor'                   => '$1 li canciamenti nichi',
'rcshowhidebots'                    => '$1 li bot',
'rcshowhideliu'                     => "$1 l'utilizzatura cû nomu",
'rcshowhideanons'                   => "$1 l'utilizzatura anònimi",
'rcshowhidepatr'                    => '$1 li canciamenti cuntrullati',
'rcshowhidemine'                    => '$1 li mè canciamenti',
'rclinks'                           => "Ammustra l'ùrtimi $1 canciamenti nta l'ùrtimi $2 jorna <br />$3",
'diff'                              => 'diff',
'hist'                              => 'storia',
'hide'                              => 'ammuccia',
'show'                              => 'ammustra',
'minoreditletter'                   => 'n',
'newpageletter'                     => 'N',
'boteditletter'                     => 'b',
'number_of_watching_users_pageview' => '[ossirvata di {{PLURAL:$1|nu utenti|$1 utenti}}]',
'rc_categories'                     => 'Lìmita a li catigurìi (siparati di "|")',
'rc_categories_any'                 => 'Qualisiasi',
'newsectionsummary'                 => '/* $1 */ sizzioni nova',

# Recent changes linked
'recentchangeslinked'          => 'Canciamenti culligati',
'recentchangeslinked-title'    => 'Canciamenti culligati a "$1"',
'recentchangeslinked-noresult' => 'Nuddu canciamentu ê pàggini culligati ntô pirìudu spicificatu.',
'recentchangeslinked-summary'  => "Chista pàggina spiciali ammustra li canciamenti cchiù ricenti ê pàggini culligati a chidda spicificata. Li pàggini taliati ni la tou [[Special:Watchlist|lista taliata]] sunu evidenziati 'n '''grassettu'''.",
'recentchangeslinked-page'     => 'Nnomu dâ pàggina:',
'recentchangeslinked-to'       => 'Vidi sulu li canciamenti ê pàggini culligati a chidda spicificata',

# Upload
'upload'                      => 'Càrrica nu file',
'uploadbtn'                   => 'Càrrica',
'reupload'                    => 'Càrrica di novu',
'reuploaddesc'                => 'Torna a lu mòdulu pi lu carricamentu.',
'uploadnologin'               => 'Accessu nun effittuatu',
'uploadnologintext'           => 'Hai a esèquiri [[Special:UserLogin|lu login]] pi carricari mmàggini o àutri files multimidiali.',
'upload_directory_read_only'  => 'Lu server web nun è n gradu di scrìviri ntâ directory di upload ($1).',
'uploaderror'                 => 'Erruri ntô carricamentu',
'uploadtext'                  => "Usa lu mòdulu ccà sutta pi carricari file novi. Pi vìdiri o circari li file già carricati, talìa lu [[Special:ImageList|log dî file carricati]]. Carricamenti di file e di virsioni novi di file sunnu riggistrati ntô [[Special:Log/upload|log di l'upload]], li cancillazzioni di file sunnu 
riggistrati [[Special:Log/delete|ccà]].

Pi nziriri nu file nta na pàggina, fai nu lijami accussì:
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:File.jpg]]</nowiki></tt>''' p'usari la virsioni ntera dû file
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:File.png|200px|thumb|left|testu altirnativu]]</nowiki></tt>''' p'usari na virsioni làrica 200 pixel nziruta nta nu box, alliniata a manu manca e cu 'testu altirnativu' comu didascalìa
* '''<tt><nowiki>[[</nowiki>{{ns:media}}<nowiki>:File.ogg]]</nowiki></tt>''' pi culligari direttamenti a lu file senza vidìrilu.",
'uploadlog'                   => 'File carricati',
'uploadlogpage'               => 'File carricati',
'uploadlogpagetext'           => "Ccà sutta la lista di l'ùrtimi file carricati. Talìa la [[Special:NewImages|gallarìa dî file novi]] pi na visioni ginirali.",
'filename'                    => 'Nomu dû file',
'filedesc'                    => 'Discrizzioni',
'fileuploadsummary'           => "Discrizzioni (auturi, fonti, discrizzioni, licenza d'usu, noti) dû file:",
'filestatus'                  => 'Nfurmazzioni supra lu copyright:',
'filesource'                  => 'Fonti:',
'uploadedfiles'               => 'File carricati',
'ignorewarning'               => "Gnora l'avvisu e sarva comu è gghiè lu file. La virsioni asistenti veni suvrascritta.",
'ignorewarnings'              => "Gnora li missaggi d'avvirtimentu dû sistema",
'minlength1'                  => 'Lu nomu dô file hà a èssiri cumpostu di armenu na lìttera.',
'illegalfilename'             => 'Lu nomu "$1" cunteni dî caràttiri nun ammessi ntê tìtuli dî pàggini. Dari a lu file un nomu diversu e pruvari a carricàrilu di novu.',
'badfilename'                 => 'Lu nomu dû file è statu cummirtutu n "$1".',
'filetype-badmime'            => 'Nun è cunzintitu carricari file di tipu MIME "$1".',
'filetype-missing'            => 'Lu file è privu d\'estinzioni (p\'asempiu ".jpg").',
'large-file'                  => 'Si raccumanna di nun supirari li diminzioni di $1 pi ognunu file; stu file è granni $2.',
'largefileserver'             => 'Lu file sùpira li diminzioni cunzintiti dâ cunfigurazzioni dû server.',
'emptyfile'                   => "Lu file appena carricatu pari èssiri vacanti. Chistu putissi èssiri duvutu a n'erruri ntô nomu dû file. Virificari ca si ntenni riarmenti carricari stu file.",
'fileexists'                  => 'Nu file cu stu nomu asisti già, pi favuri cuntrolla <strong><tt>$1</tt></strong> siddu nun sî sicuru di vulìrilu suvrascrìviri.',
'fileexists-extension'        => "Nu file cu nu nomu simili a chistu esisti già; l'unica diffirenza è l'usu dê maiusculi nte l'estensioni:<br />
Nomu dû file carricatu: <strong><tt>$1</tt></strong><br />
Nome dû file esistenti: <strong><tt>$2</tt></strong><br />
Pi favuri scegghiti n'àutru nomu.",
'fileexists-thumb'            => "<center>'''Mmagini esistenti'''</center>",
'fileexists-thumbnail-yes'    => "Lu file carricato sembra èssiri lu risurtatu di n'antiprima <i>(thumbnail)</i>. Virificari, pi cunfruntu, lu file <strong><tt>$1</tt></strong>.<br />
Siduu si tratta dâ stissa mmagini, nte dimenzioni urigginali, nun è nicissariu carricara àutri antiprimi.",
'file-thumbnail-no'           => "Lu nomu dô file accumenza cu <strong><tt>$1</tt></strong>. 
Pari quinni èssiri lu risurtatu di n'antiprima <i>(thumbnail)</i>.
Siddu si disponi dâ mmàggini ntâ risuluzzioni urigginali, si prega di carricàrila. 'N casu cuntrariu, si prega di canciari lu nomu dô file.",
'fileexists-forbidden'        => "Nu file cu stu nomu asisti già. Turnari n'arreri e canciari lu nomu cu lu quali carricari lu file. [[Image:$1|thumb|center|$1]]",
'fileexists-shared-forbidden' => "Nu file cu stu nomu asisti già nta l'archiviu dî risursi multimidiali cundivisi. Siddu voi ancora carricari lu file, pi favuri torna n'arreri e cancia lu nomu ca voi dari a lu file. [[Image:$1|thumb|center|$1]]",
'successfulupload'            => 'Carricamentu cumplitatu',
'uploadwarning'               => 'Avvisu di Upload',
'savefile'                    => 'Sarva file',
'uploadedimage'               => 'hà carricatu "[[$1]]"',
'overwroteimage'              => 'carricata na nova virsioni di "[[$1]]"',
'uploaddisabled'              => 'Semu spiacenti, ma lu carricamentu di file è timpuraniamenti suspisu.',
'uploaddisabledtext'          => 'Lu carricamentu dî file nun è attivu supra stu situ.',
'uploadscripted'              => "Stu file cunteni còdici HTML o di script, ca putissi èssiri nterpritato erroniamenti d'un browser web.",
'uploadcorrupt'               => 'Lu file è currumputu o hà na stinzioni nun curretta. Pi favuri cuntrolla lu file e esequi di novu lu carricamentu.',
'uploadvirus'                 => 'Lu file cunteni un virus! Ultiriuri nfurmazzioni: $1',
'sourcefilename'              => "Nomu dû file d'orìggini:",
'destfilename'                => 'Nomu dû file di distinazzioni:',
'watchthisupload'             => 'Talìa sta pàggina',
'filewasdeleted'              => 'Nu file cu stu nomu hà statu già carricatu e cancillatu n passatu. Virificari $1 prima di carricàrilu di novu.',
'upload-wasdeleted'           => "'''Accura: stai carricannu nu file chi fu già cancillatu.'''

Virifica pi favuri la nicissitati di continuari cu lu carricamentu di chistu file.
Pi tua cumoditati cca c'è la riggistrazioni dâ cancillazioni:",
'filename-bad-prefix'         => 'Lu nomu dô file chi stai carricannu ncigna cu <strong>"$1"</strong>, chi è nu nomu non descrittivu assignatu, di solitu, automaticamenti dê màchini fotugràfici diggitali. Pi favuri scegghia nu nomu cchiù descrtittivu pi lu tò file.',
'filename-prefix-blacklist'   => ' #<!-- dassa sta lìnia comu è già --> <pre>
# Chista di sèquitu è la sintassi: 
#   * Tutti li scritti a pàrtiri dô carattiri "#" sugnu commenti
#   * Tutti li lìnii non vacanti sugnu prefissi pi tipici nomi di file assignati automaticamenti dê màchini fotugràfici diggitali
CIMG # Casio
DSC_ # Nikon
DSCF # Fuji
DSCN # Nikon
DUW # arcuni cellulari
IMG # genericu
JD # Jenoptik
MGP # Pentax
PICT # arcuni
 #</pre> <!-- dassa sta lìnia comu è già -->',

'upload-proto-error'      => 'Protucollu erratu',
'upload-proto-error-text' => "Pi l'upload rimotu è nicissariu spicificari URL ca nìzzianu cu <code>http://</code> oppuru <code>ftp://</code>.",
'upload-file-error'       => 'Erruri nternu',
'upload-file-error-text'  => "S'hà virificatu un erruri nternu duranti la criazzioni d'un file timpuràniu supra lu server. Cuntattari un amministraturi di sistema.",
'upload-misc-error'       => "Erruri nun idintificatu pi l'upload",
'upload-misc-error-text'  => "S'hà virificatu un erruri nun idintificatu duranti lu carricamentu dû file. Virificari ca la URL è curretta e accissìbbili e pruvari di novu. Siddu lu prubbrema pirsisti, cuntattari un amministraturi di sistema.",

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => 'URL nun ragghiuncìbbili',
'upload-curl-error6-text'  => 'Mpussìbbili ragghiùnciri la URL spicificata. Virificari ca la URL è scritta currettamenti e ca lu situ n chistioni è attivu.',
'upload-curl-error28'      => "Tempu scadutu pi l'upload",
'upload-curl-error28-text' => 'Lu situ rimotu hà mpiegatu troppu tempu a arrispùnniri. Virificari ca lu situ è attivu, attènniri quarchi minutu e pruvari di novu, eventuarmenti nta un mumentu di tràfficu nicu.',

'license'            => "Licenza d'usu:",
'nolicense'          => 'Nudda silizzioni',
'license-nopreview'  => '(Antiprima nun disponibbili)',
'upload_source_url'  => '(na URL curretta e accissìbbili)',
'upload_source_file' => '(un file supra lu propiu computer)',

# Special:ImageList
'imagelist_search_for'  => 'Ricerca dâ mmàggini di nomu:',
'imgfile'               => 'file',
'imagelist'             => 'Alencu dî file',
'imagelist_date'        => 'Data',
'imagelist_name'        => 'Nomu',
'imagelist_user'        => 'Utenti',
'imagelist_size'        => 'Diminzioni (bytes)',
'imagelist_description' => 'Discrizzioni',

# Image description page
'filehist'                  => 'Crunoluggìa dô file',
'filehist-help'             => 'Fari clic supra nu gruppu data/ura pi vìdiri lu file comu si prisintava ntô mumentu nnicatu.',
'filehist-deleteall'        => 'cancilla tuttu',
'filehist-deleteone'        => 'cancella',
'filehist-revert'           => 'riprìstina',
'filehist-current'          => 'correnti',
'filehist-datetime'         => 'Data/Ura',
'filehist-user'             => 'Utenti',
'filehist-dimensions'       => 'Diminsioni',
'filehist-filesize'         => 'Dimensioni dû file',
'filehist-comment'          => 'Oggettu',
'imagelinks'                => "Pàggini c'ùsanu sta mmàggini",
'linkstoimage'              => '{{PLURAL:$1|La pàggina siquenti richiàma|Li $1 pàggini siquenti richiàmanu}} sta mmàggini:',
'nolinkstoimage'            => 'Nudda pàggina cunteni sta mmàggini.',
'sharedupload'              => "Chistu file è n'upload condivisu; pò èssiri quinni utilizzatu di cchiù pruggetti wiki.",
'shareduploadwiki'          => 'Si preja di taliari $1 pi ultiriuri nfurmazzioni.',
'shareduploadwiki-desc'     => 'La discrizzioni supra $1 ca appari nta dda sedi veni ammustrata sutta.',
'shareduploadwiki-linktext' => "pàggina di discrizzioni dû ''file''",
'noimage'                   => 'Un file cu stu nomu nun esisti, ma è pussìbbili, voi $1 tu?',
'noimage-linktext'          => 'carricàrilu ora',
'uploadnewversion-linktext' => 'Càrrica na virsioni nova di stu file',
'imagepage-searchdupe'      => 'Ricerca di file duplicati',

# File reversion
'filerevert'                => 'Riprìstina $1',
'filerevert-legend'         => 'Riprìstina file',
'filerevert-intro'          => "Stai pi ripristinari lu file '''[[Media:$1|$1]]''' â [virsioni $4 dô $2, $3].",
'filerevert-comment'        => 'Oggettu:',
'filerevert-defaultcomment' => 'Ripristinata la virsioni dô $1, $2',
'filerevert-submit'         => 'Riprìstina',
'filerevert-success'        => "'''Lu file [[Media:$1|$1]]''' hà statu ripristinatu â [$4 virsioni dô $2, $3].",
'filerevert-badversion'     => 'Nun esistanu virsiona locali pricidenti dô file cû timestamp richiestu.',

# File deletion
'filedelete'                  => 'Cancella $1',
'filedelete-legend'           => 'Cancella lu file',
'filedelete-intro'            => "Stai pi cancillari '''[[Media:$1|$1]]'''.",
'filedelete-intro-old'        => "Stai cancillannu la virsioni di '''[[Media:$1|$1]]''' dô [$4 $3, $2].",
'filedelete-comment'          => 'Mutivu:',
'filedelete-submit'           => 'Cancella',
'filedelete-success'          => "Lu file '''$1''' hà statu cancillatu.",
'filedelete-success-old'      => '<span class="plainlinks">La virsioni dô file \'\'\'[[Media:$1|$1]]\'\'\' dô $2, $3 hà statu cancillata.</span>',
'filedelete-nofile'           => "Nta {{SITENAME}} nun c'è nuddu file $1",
'filedelete-nofile-old'       => "'N archiviu nun ci sugnu virsioni di '''$1''' cu li carattiristichi nnicati.",
'filedelete-iscurrent'        => 'Sta pruvannu a cancillari la virsioni cchiù ricenti di chistu file. Pi favuri, prima riturnàrilu a na virsioni pricidenti.',
'filedelete-otherreason'      => 'Autra mutivazioni o mutivazioni n più:',
'filedelete-reason-otherlist' => 'Autra mutivazioni',
'filedelete-reason-dropdown'  => '*Mutivazzioni cchiù cumuni
** Viulazzioni di copyright
** File duplicatu',
'filedelete-edit-reasonlist'  => 'Cancia li mutivazioni pi la cancillazzioni',

# MIME search
'mimesearch'         => "Circata 'n basi a lu tipu MIME",
'mimesearch-summary' => "Sta pàggina cunzenti di filtrari li file 'n basi a lu tipu MIME. Nziriri la stringa di ricerca ntâ forma tipu/suttatipu, p'asempiu <tt>image/jpeg</tt>.",
'mimetype'           => 'Tipu MIME:',
'download'           => 'scarica',

# Unwatched pages
'unwatchedpages' => 'Pàggini nun taliati',

# List redirects
'listredirects' => 'Alencu di tutti li redirect',

# Unused templates
'unusedtemplates'     => 'Template nun utilizzati',
'unusedtemplatestext' => 'Nta sta pàggina vèninu alincati tutti li template (pàggini dû namespace Template) ca nun sunnu nclusi n nudda pàggina. Prima di cancillàrili è appurtunu virificari ca li sìnguli template nun hannu àutri culligamenti trasenti.',
'unusedtemplateswlh'  => 'àutri liami',

# Random page
'randompage'         => 'Na pàggina ammuzzu',
'randompage-nopages' => 'Nudda pàggina ntô namespace silizziunatu.',

# Random redirect
'randomredirect'         => 'Un redirect a muzzu',
'randomredirect-nopages' => 'Nuddu rinnirizzamentu ntô namespace silizziunatu.',

# Statistics
'statistics'             => 'Statìstichi',
'sitestats'              => 'Li statìstichi di {{SITENAME}}',
'userstats'              => "Li statìstichi di l'utilizzatura",
'sitestatstext'          => "C{{PLURAL:\$1|'è na pàggina|i sunnu '''\$1''' pàggini}} ntô databbasi.
Chisti nclùdunu li pàggini di discussioni, li pàggini supra {{SITENAME}}, li \"stub\" minimali, li redirects, e àutri pàggini chi nun ponnu qualificàrisi comu pàggini di cuntinutu. Escludennu chissi, c{{PLURAL:\$2|'è '''1''' pàggina chi si pò|i sunnu '''\$2''' pàggini chi si ponnu}} qualificari comu pàggini di cuntinutu.

{{PLURAL:\$8|Hà statu puru carricatu|Hannu stati puru carricati}} '''\$8''' file.

Dâ nstallazzioni dô situ nzinu a stu mumentu {{PLURAL:\$3|hà stata visitata '''1''' pàggina|hannu statu visitati '''\$3''' pàggini}} e {{PLURAL:\$4|fattu '''1''' canciamentu|fatti '''\$4''' canciamenti}}, pari a na media di '''\$5''' canciamenti pi pàggina e '''\$6''' richiesti di littura p'ogni canciamentu.

La cuda dî prucessi a esèquiri 'n background cunteni {{PLURAL:\$7|'''1''' elementu|'''\$7''' elementi}}.",
'userstatstext'          => "C{{PLURAL:$1|'è '''1''' [[Special:ListUsers|utilizzaturi]] riggistratu|i sunnu '''$1''' [[Special:ListUsers|utilizzatura]] riggistrati}}; di chisti '''$2''' (o lu '''$4%''') {{PLURAL:$2|havi|hannu}} li diritti di lu gruppu $5.",
'statistics-mostpopular' => 'Pàggini cchiù visitati',

'disambiguations'      => 'Pàggini cu liami ambìgui',
'disambiguationspage'  => 'Template:Disambigua',
'disambiguations-text' => "Li pàggini ntâ lista ca sequi cuntèninu dî culligamenti a '''pàggini di disambiguazzioni''' e nun a l'argumentu cui avìssiru a fari rifirimentu.<br />
Vèninu cunzidirati pàggini di disambiguazzioni tutti chiddi ca cuntèninu li template alincati 'n [[MediaWiki:Disambiguationspage]]",

'doubleredirects'     => 'Rinnirizzamenti duppi',
'doubleredirectstext' => 'Chista pàggina alenca li pàggini chi rinnirìzzanu a àutri pàggini di rinnirizzamentu. Ognuna riga cunteni li culligamenti a lu primu e a lu secunnu redirect, oltri â prima riga di testu dû secunnu redirect ca di sòlitu cunteni la pàggina di distinazzioni "curretta" â quali avissi a puntari macari lu primu redirect.',

'brokenredirects'        => "Riinnirizzamenti (''redirects'') rumputi.",
'brokenredirectstext'    => 'Li rinnirizzamenti siquenti pùntanu a pàggini ca nun asìstinu:',
'brokenredirects-edit'   => '(cancia)',
'brokenredirects-delete' => '(cancella)',

'withoutinterwiki'         => 'Pàggini senza interwiki',
'withoutinterwiki-summary' => 'Li pàggini nnicati ccà nun hànnu liami ê virsioni nta àutri lingui:',
'withoutinterwiki-submit'  => 'Ammustra',

'fewestrevisions' => 'Pàggini cu menu rivisioni',

# Miscellaneous special pages
'ncategories'             => '$1 {{PLURAL:$1|catigurìa|catigurìi}}',
'nlinks'                  => '$1 {{PLURAL:$1|culligamentu|culligamenti}}',
'nmembers'                => '$1 {{PLURAL:$1|elementu|elementi}}',
'nrevisions'              => '$1 {{PLURAL:$1|rivisioni|rivisioni}}',
'nviews'                  => '$1 {{PLURAL:$1|vìsita|vìsiti}}',
'specialpage-empty'       => 'Sta pàggina spiciali è attuarmenti vacanti.',
'lonelypages'             => 'Pàggini òrfani',
'lonelypagestext'         => "Li pàggini nnicati ccà sutta nun hannu lijami ca vèninu d'àutri pàggini di {{SITENAME}}.",
'uncategorizedpages'      => 'Pàggini nun catigurizzati',
'uncategorizedcategories' => 'Catigurìi nun catigurizzati',
'uncategorizedimages'     => 'Mmàggini nun catigurizzati',
'uncategorizedtemplates'  => 'Template senza catigurìi',
'unusedcategories'        => 'Catigurìi vacanti',
'unusedimages'            => 'File nun utilizzati',
'popularpages'            => 'Pàggini cchiù visitati',
'wantedcategories'        => 'Catigurìi addumannati',
'wantedpages'             => 'Artìculi cchiù addumannati',
'mostlinked'              => 'Pàggini supra cui agghìcanu cchiù liami',
'mostlinkedcategories'    => 'Catigurìi cchiù richiamati',
'mostlinkedtemplates'     => 'Template cchiù usati',
'mostcategories'          => 'Artìculi urdinati secunnu chiddi chi hannu cchiù catigurìi',
'mostimages'              => 'Mmàggini cchiù richiamati',
'mostrevisions'           => 'Artìculi urdinati secunnu chiddi chi hannu cchiù canciamenti',
'prefixindex'             => 'Ìnnici secunnu un prifissu',
'shortpages'              => 'Artìculi urdinati secunnu la lunchizza (li cchiù curti prima)',
'longpages'               => 'Artìculi urdinati secunnu la lunchizza (li cchiù lonchi prima)',
'deadendpages'            => 'Pàggini senza nisciuta',
'deadendpagestext'        => 'Li pàggini ndicati di sèquitu sunnu privi di culligamenti versu àutri pàggini dû situ.',
'protectedpages'          => 'Pàggini prutetti',
'protectedpagestext'      => 'Sta pàggina hà statu prutiggiuta pi mpidìrinni lu canciamentu.',
'protectedpagesempty'     => 'A lu mumentu nun ci sunnu pàggini prutetti',
'listusers'               => 'Lista di utilizzatura',
'newpages'                => 'pàggini cchiù ricenti',
'newpages-username'       => 'Utenti:',
'ancientpages'            => 'pàggini cchiù vecchi',
'move'                    => 'sposta',
'movethispage'            => 'Sposta sta pàggina',
'unusedimagestext'        => "Accura, è pussìbbili fari lijami a li file d'àutri siti, usannu direttamenti la URL; 
chisti putìssiru quinni èssiri utilizzati puru siddu cumpàrinu nta l'alencu.",
'unusedcategoriestext'    => 'Li siquenti pàggini dî catigurìi esìstinu, sibbeni li catigurìi currispunnenti sunnu vacanti.',
'notargettitle'           => 'Dati mancanti',
'notargettext'            => "Nun hà statu innicata na pàggina o un utenti 'n rilazzioni a lu quali esèquiri l'opirazzioni addumannata.",
'suppress'                => 'Oversight',

# Book sources
'booksources'               => 'Libbra secunnu lu còdici ISBN',
'booksources-search-legend' => 'Ricerca di fonti libbrari',
'booksources-isbn'          => 'Còdici ISBN:',
'booksources-go'            => 'Vai',
'booksources-text'          => "Di sèquitu veni prisintatu n'alencu di culligamenti versu siti sterni ca vìnninu libbra novi e usati, attraversu li quali è pussìbbili ottèniri maiuri nfurmazzioni supra lu testu circatu:",

# Special:Log
'specialloguserlabel'  => 'Utenti:',
'speciallogtitlelabel' => 'Tìtulu:',
'log'                  => 'Log',
'all-logs-page'        => 'Tutti li log',
'log-search-legend'    => "Va' cerca nte riggistri",
'log-search-submit'    => 'Vai',
'alllogstext'          => "Prisintazzioni unificata di tutti li riggistri di {{SITENAME}}. Poi limitari li criteri di circata silizziunannu lu tipu di riggistru, l'utenti ca fici l'azzioni (case-sensitive), e/o la pàggina ntirissata (pur'idda case-sensitive).",
'logempty'             => 'Lu log nun cunteni elementi currispunnenti â ricerca.',
'log-title-wildcard'   => 'Attrova tituli chi ncignanu cu',

# Special:AllPages
'allpages'          => 'Tutti li paggini',
'alphaindexline'    => 'di $1 a $2',
'nextpage'          => 'Pàggina doppu ($1)',
'prevpage'          => 'Pàggina pricidenti ($1)',
'allpagesfrom'      => 'Ammustra li pàggini a pàrtiri di:',
'allarticles'       => "Tutti l'artìculi",
'allinnamespace'    => 'Tutti li pàggini dû namespace $1',
'allnotinnamespace' => 'Tutti li pàggini, sparti lu namespace $1',
'allpagesprev'      => "'n arreri",
'allpagesnext'      => "'n avanti",
'allpagessubmit'    => 'Vai',
'allpagesprefix'    => 'Ammustra li pàggini chi accumìnzanu cu:',
'allpagesbadtitle'  => 'Lu tìtulu ndicatu pi la pàggina nun è vàlidu o cunteni prifissi interlingua o interwiki. Putissi noltri cuntèniri unu o cchiù caràttiri lu cui usu nun è ammissu ntê tìtuli.',
'allpages-bad-ns'   => 'Lu namespace "$1" nun asisti supra {{SITENAME}}.',

# Special:Categories
'categories'                    => 'Catigurìi',
'categoriespagetext'            => 'Li catigurìi ccassutta cuntèninu pàggini o file multimidiali.
Li [[Special:UnusedCategories|catigurìi vacanti]] nun sunnu ammustrati ccà.
Talìa macari li [[Special:WantedCategories|catigurìi addumannati]].',
'special-categories-sort-count' => 'ordina pi nùmmuru',
'special-categories-sort-abc'   => 'ordina alfabbeticamenti',

# Special:ListUsers
'listusersfrom'      => "Ammustra l'utenti a pàrtiri di:",
'listusers-submit'   => 'Ammustra',
'listusers-noresult' => "Nuddu utenti attruvatu. Virificari l'usu di caràttiri maiùsculi/minùsculi.",

# Special:ListGroupRights
'listgrouprights'          => 'Diritti dô gruppu utenti',
'listgrouprights-summary'  => "Ccà sutta sunnu elincati li gruppi utenti difiniti pi sta wiki, cu li dritti d'accessu assuciati a iddi. Pi sapìrinni chiossai supra li dritti, lèggiti [[{{MediaWiki:Listgrouprights-helppage}}|sta pàggina]].",
'listgrouprights-group'    => 'Gruppu',
'listgrouprights-rights'   => 'Diritti',
'listgrouprights-helppage' => 'Help:Diritti dô gruppu',

# E-mail user
'mailnologin'     => 'Nuddu ndirizzu cui mannari lu missaggiu',
'mailnologintext' => 'Hai a fari lu [[Special:UserLogin|login]] e aver riggistratu na casella e-mail vàlida ntê tò [[Special:Preferences|prifirenzi]] pi mannari posta alittrònica a àutri Utenti.',
'emailuser'       => "Manna n'imail a stu utenti",
'emailpage'       => "Manna un missaggiu e-mail a l'utenti",
'emailpagetext'   => "Siddu st'utenti lassau nu nnirizzu email vàlidu ntê sò prifirenzi, ci putiti mannari nu missaggiu. Lu nnirizzu email chi lassasti ntê tò [[Special:Preferences|prifirenzi]] và a cumpàriri comu mittenti di lu email, di manera chi lu distinatariu ti pò arrispùnniri.",
'usermailererror' => "L'uggettu mail hà ristituitu l'erruri:",
'defemailsubject' => 'Missaggiu di {{SITENAME}}',
'noemailtitle'    => 'Nuddu ndirizzu e-mail',
'noemailtext'     => "St'utilizzaturi nun spicificau nu nnirizzu email vàlidu, o scigghìu di nun ricìviri email di àutri utilizzatura.",
'emailfrom'       => 'Di:',
'emailto'         => 'A:',
'emailsubject'    => 'Uggettu:',
'emailmessage'    => 'Missaggiu:',
'emailsend'       => 'Mannari',
'emailccme'       => 'Mànnami na copia dû missaggiu.',
'emailccsubject'  => 'Copia dû missaggiu mannatu a $1: $2',
'emailsent'       => 'Imeil mannata',
'emailsenttext'   => 'Lu tò missaggiu imeil ha statu mannatu.',

# Watchlist
'watchlist'            => 'Lista taliata mia',
'mywatchlist'          => 'Lista taliata mia',
'watchlistfor'         => "(di l'utenti '''$1''')",
'nowatchlist'          => "Nun hai innicatu pàggini a tèniri d'occhiu.",
'watchlistanontext'    => "Pi visualizzari e canciari l'alencu di l'ossirvati spiciali è nicissariu $1.",
'watchnologin'         => 'Nun hai effittuatu lu login',
'watchnologintext'     => 'Hai a fari prima lu [[Special:UserLogin|login]] pi canciari la tò lista di ossirvati spiciali.',
'addedwatch'           => "Pàggina agghiunciuta â lista di l'ossirvati spiciali",
'addedwatchtext'       => "La pàggina \"[[:\$1]]\" è stata agghiunciuta â propia [[Special:Watchlist|lista di l'ossirvati spiciali]]. D'ora n poi, li mudìfichi appurtati â pàggina e â sò discussioni vèninu alincati n chidda sedi; lu tìtulu dâ pàggina appari n '''grassettu''' ntâ pàggina di l' [[Special:RecentChanges|ùrtimi canciamenti]] pi rinnìrilu cchiù visìbbili. Siddu n un secunnu tempu s'addisìa eliminari la pàggina dâ lista di l'ossirvati spiciali, fari clic supra \"nun sèquiri\" ntâ barra n àutu.",
'removedwatch'         => 'Livata dâ lista dî pàggini di cuntrullari',
'removedwatchtext'     => 'La pàggina "[[:$1]]" hà statu eliminata dâ lista di l\'ossirvati spiciali.',
'watch'                => 'talìa',
'watchthispage'        => 'talìa sta pàggina',
'unwatch'              => 'Nun taliari',
'unwatchthispage'      => 'Smetti di sèquiri',
'notanarticle'         => "Nun è n'artìculu",
'notvisiblerev'        => 'La revisioni fu cancillata',
'watchnochange'        => 'Nudda dî pàggini ossirvati hà statu canciata ntô pirìudu cunzidiratu.',
'watchlist-details'    => 'La lista dê pàggini taliati cunteni {{PLURAL:$1|na pàggina (cu la rispettiva pàggina di discussioni)|$1 pàggini (cu li rispettivi pàggini di discussioni)}}.',
'wlheader-enotif'      => '* La nutìfica via e-mail è attivata.',
'wlheader-showupdated' => "* Li pàggini ca hannu statu canciati dâ tò ùrtima vìsita sunnu evidinziati 'n '''grassettu'''",
'watchmethod-recent'   => "cuntrollu dî canciamenti ricenti pi l'ossirvati spiciali",
'watchmethod-list'     => "cuntrollu di l'ossirvati spiciali pi canciamenti ricenti",
'watchlistcontains'    => 'La tò lista di ossirvati spiciali cunteni {{PLURAL:$1|na pàggina|$1 pàggini}}.',
'iteminvalidname'      => "Prubbremi cu la pàggina '$1', nomu nun vàlidu...",
'wlnote'               => "Sutta attrovi l'ùrtim{{PLURAL:$1|u canciamentu|i $1 canciamenti}}, nta l'ùrtim{{PLURAL:$1|a ura|i '''$2''' uri}}.",
'wlshowlast'           => "Ammustra l'ùrtimi $1 uri $2 jorna $3",
'watchlist-show-bots'  => 'Ammustra li canciamenti dî bot',
'watchlist-hide-bots'  => 'Ammuccia li canciamenti dî bot',
'watchlist-show-own'   => 'Ammustra li mè canciamenti',
'watchlist-hide-own'   => 'Ammuccia li mè canciamenti',
'watchlist-show-minor' => 'Ammustra li canciamenti nichi',
'watchlist-hide-minor' => 'Ammuccia li canciamenti nichi',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => "Junta a l'ossirvati spiciali...",
'unwatching' => "Eliminazzioni di l'ossirvati spiciali...",

'enotif_mailer'                => 'Sistema di nutìfica via e-mail di {{SITENAME}}',
'enotif_reset'                 => 'Segna tutti li pàggini comu già visitati',
'enotif_newpagetext'           => 'Chista è na pàggina nova.',
'enotif_impersonal_salutation' => 'Utenti di {{SITENAME}}',
'changed'                      => 'canciatu',
'created'                      => 'criatu',
'enotif_subject'               => 'La pàggina $PAGETITLE di {{SITENAME}} hà stata $CHANGEDORCREATED di $PAGEEDITOR',
'enotif_lastvisited'           => 'Cunzurta $1 pi vìdiri tutti li canciamenti dâ tò ùrtima vìsita.',
'enotif_lastdiff'              => 'Vìdiri $1 pi visualizzari lu canciamentu.',
'enotif_anon_editor'           => 'utenti anonimu $1',
'enotif_body'                  => 'Gintili $WATCHINGUSERNAME, 

la pàggina $PAGETITLE di {{SITENAME}} hà stata $CHANGEDORCREATED \'n data $PAGEEDITDATE di $PAGEEDITOR; la virsioni attuali s\'attrova a lu ndirizzu $PAGETITLE_URL.

$NEWPAGE

Riassuntu dû canciamentu, nziritu di l\'auturi: $PAGESUMMARY $PAGEMINOREDIT

Cuntatta l\'auturi dû canciamentu:
via e-mail: $PAGEEDITOR_EMAIL
supra lu situ: $PAGEEDITOR_WIKI

Nun vèninu mannati àutri canciamenti \'n caso di ultiriuri canciamenti, a menu ca tu nun vìsiti la pàggina. Noltri, è pussìbbili rimpustari l\'avvisu di nutìfica pi tutti li pàggini ntâ lista di l\'ossirvati spiciali. 

             Lu sistema di nutìfica di {{SITENAME}}, a lu tò sirvizziu 

-- 
Pi mudificari li mpustazzioni dâ lista di l\'ossirvati spiciali, vìsita 
{{fullurl:{{ns:special}}:Watchlist/edit}} 

Pi dari lu tò feedback e arricèviri ultiriuri assistenza:
{{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => 'Elìmina la pàggina',
'confirm'                     => 'Cunferma',
'excontent'                   => "Lu cuntinutu era: '$1'",
'excontentauthor'             => "Lu cuntinutu era: '$1' (e lu sulu cuntribbuturi era '[[Special:Contributions/$2|$2]]')",
'exbeforeblank'               => "Lu cuntinutu prima dû svacantamentu era: '$1'",
'exblank'                     => 'la pàggina era vacanti',
'delete-confirm'              => 'Cancella "$1"',
'delete-legend'               => 'Cancella',
'historywarning'              => 'Accura: La pàggina ca stai pi cancillari havi na cronoluggìa:',
'confirmdeletetext'           => "Stai cancillannu dû databbasi na pàggina o na mmàggini cu tutta la sò storia di manera pirmanenti. Pi fauri, cunferma ca tu ntenni fari sta cosa, ca tu hai caputu li cunziquenzi, e chi lu fai secunnu li linìi guida stabbiliti 'n [[{{MediaWiki:Policy-url}}]].",
'actioncomplete'              => 'Azzioni cumpritata',
'deletedtext'                 => '"<nowiki>$1</nowiki>" ha statu cancillatu.
Talìa $2 pi na lista di cancillazzioni ricenti.',
'deletedarticle'              => 'Hà cancillatu "[[$1]]"',
'suppressedarticle'           => 'suppressu "[[$1]]"',
'dellogpage'                  => 'Cancillazzioni',
'dellogpagetext'              => 'Di sèquitu sunnu alincati li pàggini cancillati di ricenti.',
'deletionlog'                 => 'Log dî cancillazzioni',
'reverted'                    => 'Ripristinata la virsioni pricidenti',
'deletecomment'               => 'Mutivazzioni pi cancillari',
'deleteotherreason'           => 'Autra mutivazioni o mutivazioni in più:',
'deletereasonotherlist'       => 'Autra mutivazioni',
'deletereason-dropdown'       => "*Mutivazzioni cchiù cumuni pi la cancillazzioni
** Dumanna di l'auturi
** Viulazzioni di copyright
** Vannalismu",
'delete-edit-reasonlist'      => 'Cancia li mutivazzioni pi la cancillazioni',
'delete-warning-toobig'       => 'La storia di sta pàggina è assai longa (ortri $1 {{PLURAL:$1|rivisioni|rivisioni}}). La sò cancillazzioni pò causari prubbremi di funziunamentu ô database di {{SITENAME}}; prucèdiri attentamenti.',
'rollback'                    => 'Annulla li canciamenti',
'rollback_short'              => "Canciu n'arreri",
'rollbacklink'                => "canciu n'arreri",
'rollbackfailed'              => "Canciu 'n arreri fallitu",
'cantrollback'                => "Mpussìbbili annullari li canciamenti; l'utenti ca l'effittuau è l'ùnicu a aviri cuntribbuiutu â pàggina.",
'alreadyrolled'               => "Nun è pussìbbili annullari li canciamenti appurtati â pàggina [[:$1]] di parti di [[User:$2|$2]] ([[User talk:$2|Discussioni]] | [[Special:Contributions/$2|{{int:contribslink}}]]); n'àutru utenti hà già canciatu la pàggina oppuru hà effittuatu lu rollback.

Lu canciamentu cchiù ricenti â pàggina fu appurtata di [[User:$3|$3]] ([[User talk:$3|discussioni]]).",
'editcomment'                 => 'Lu cummentu â mudìfica era: "<i>$1</i>".', # only shown if there is an edit comment
'revertpage'                  => "Canciu narrè di [[Special:Contributions/$2|$2]] ([[User talk:$2|Discussioni]]) cu l'ùrtima virsioni di [[User:$1|$1]]", # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => 'Annullati li canciamenti di $1; ritornata â virsioni pricidenti di $2.',
'sessionfailure'              => 'S\'hà virificatu un prubbrema cu la tò sissioni di login;
lu sistema nun hà esiquitu lu cumannu mpartitu pi pricauzzioni.
Pi favuri utilizza lu tastu "\'n arreri" dû tò browser, ricàrrica la pàggina e riprova di novu.',
'protectlogpage'              => 'Pàggini prutetti',
'protectlogtext'              => 'Lista di prutezzioni/sprutezzioni dî pàggini. Vidi macari la [[Special:ProtectedPages|lista dî pàggini prutetti]].',
'protectedarticle'            => 'hà prutettu [[$1]]',
'modifiedarticleprotection'   => 'canciàu lu liveddu di prutizzioni di "[[$1]]"',
'unprotectedarticle'          => 'hà sprutettu [[$1]]',
'protect-title'               => 'Prutezzioni di "$1"',
'protect-legend'              => 'Cunferma la prutezzioni',
'protectcomment'              => 'Mutivu dâ prutezzioni',
'protectexpiry'               => 'Scadenza',
'protect_expiry_invalid'      => 'Scadenza nun vàlida.',
'protect_expiry_old'          => 'Scadenza già trascursa.',
'protect-unchain'             => 'Sblocca pirmissu di spustamentu',
'protect-text'                => 'Ccà poi vìdiri e canciari lu liveddu di prutezzioni pi la pàggina <strong><nowiki>$1</nowiki></strong>.',
'protect-locked-blocked'      => 'Nun pò canciari li liveddi di prutizzioni quannu sî bloccatu. Li mpostazzioni correnti pâ pàggina sugnu <strong>$1</strong>:',
'protect-locked-dblock'       => 'Mpussibbili canciari li liveddi di prutizzioni pi nu bloccu dô database.
Li mpostazzioni correnti pâ pàggina sugnu <strong>$1</strong>:',
'protect-locked-access'       => 'Nun hai li pirmessi nicissari pi canciari li liveddi di prutizzioni dâ pàggina.
Li mpostazzioni correnti pâ pàggina sugnu <strong>$1</strong>:',
'protect-cascadeon'           => 'A lu mumentu sta pàggina è bluccata, poichì nclusa nt{{PLURAL:$1|â pàggina innicata di sèquitu, supra la quala|ê pàggini innicati di sèquitu, supra li quali}} hà statu attivata la prutezzioni ricursiva. È pussìbbili mudificari lu liveddu di prutezzioni di sta pàggina ma lu liveddu di prutezzioni arresta chiddu difinitu dâ prutezzioni ricursiva, siddu la stissa nun veni canciata.',
'protect-default'             => '(pridifinitu)',
'protect-fallback'            => 'Richiedi lu pirmissu "$1"',
'protect-level-autoconfirmed' => "Blocca l'utenti nun riggistrati",
'protect-level-sysop'         => 'Sulu li amministratura',
'protect-summary-cascade'     => 'ricursiva',
'protect-expiring'            => 'scadi a li $1 (UTC)',
'protect-cascade'             => 'Prutezzioni ricursiva (pruteggi tutti li pàggini nclusi nta chista).',
'protect-cantedit'            => 'Nun è possibili canciari li livelli di prutizzioni pi la pàggina n quantu nun si disponi dî pirmessi necissari pi canciari la pàggina stissa.',
'restriction-type'            => 'Pirmissu',
'restriction-level'           => 'Liveddu di ristrizzioni:',
'minimum-size'                => 'Dimensioni minima',
'maximum-size'                => 'Dimensioni massima:',
'pagesize'                    => '(byte)',

# Restrictions (nouns)
'restriction-edit'   => 'Cancia',
'restriction-move'   => 'Sposta',
'restriction-create' => 'Criazioni',

# Restriction levels
'restriction-level-sysop'         => 'prutetta',
'restriction-level-autoconfirmed' => 'semi-prutetta',
'restriction-level-all'           => 'tutti li liveddi',

# Undelete
'undelete'                     => 'Visualizza pàggini cancillati',
'undeletepage'                 => 'Talìa e ricùpira li pàggini cancillati',
'undeletepagetitle'            => "'''Quantu segui è compostu da rivisioni cancillati di [[:$1]]'''.",
'viewdeletedpage'              => 'Talìa li pàggini cancillati',
'undeletepagetext'             => "Li pàggini innicati di sèquitu hannu statu cancillati, ma sunnu ancora n archiviu e pirtantu ponnu èssiri ricupirati. L'archiviu pò èssiri svacantatu piriodicamenti.",
'undeleteextrahelp'            => "Pi ricupirari la storia ntera dâ pàggina, fari clic supra '''''Riprìstina''''' senza silizziunari nudda casella. P'effittuari un riprìstinu silittivu, silizziunari li caselli currispunnenti a li rivisioni a ripristinari e fari clic supra '''''Riprìstina'''''. Facennu clic supra '''''Reset''''' vèninu disilizziunati tutti li caselli e svacantatu lu spazziu pi lu cummentu.",
'undeleterevisions'            => '{{PLURAL:$1|Na rivisioni|$1 rivisioni}} n archiviu',
'undeletehistory'              => "Siddu ricùpiri st'artìculu, tutti li sò rivisioni vèninu ricupirati ntâ cronoluggìa rilativa. Siddu doppu la cancillazzioni na pàggina nova cu lu stissu tìtulu fu criata, li rivisioni ricupirati sunnu nziriti ntâ cronoluggìa e la virsioni attuarmenti online dâ pàggina nun veni canciata.",
'undeleterevdel'               => 'Lu riprìstinu nun è fattu siddu cancella parziarmenti la virsioni currenti dâ pàggina o dû file. Nta stu casu, è nicissariu livari lu signu di spunta o lu scuramentu dê rivisioni cancillati cchiù ricenti.',
'undeletehistorynoadmin'       => "Sta pàggina hà statu cancillata. Lu mutivu dâ cancillazzioni è ammustratu ccà sutta, nzèmmula a li dittagghi di l'utenti c'hà canciatu sta pàggina prima dâ cancillazzioni. Lu testu cuntinutu ntê rivisioni cancillati è dispunìbbili sulu a li amministratura.",
'undelete-revision'            => 'Rivisioni cancillata di $1 (scritta lu $2 di $3):',
'undeleterevision-missing'     => "Rivisioni errata o mancanti. Lu culligamentu è erratu oppuru la rivisioni hà statu già ripristinata o eliminata di l'archiviu.",
'undelete-nodiff'              => "Nun s'havi attruvatu na rivisioni pricidenti.",
'undeletebtn'                  => 'Riprìstina!',
'undeletelink'                 => 'ripristina',
'undeletereset'                => 'Reimposta',
'undeletecomment'              => 'Cummentu:',
'undeletedarticle'             => 'hà ricupiratu "[[$1]]"',
'undeletedrevisions'           => '$1 rivisioni ricupirat{{PLURAL:$1|a|i}}',
'undeletedrevisions-files'     => '{{PLURAL:$1|na rivisioni|$1 rivisioni}} e {{PLURAL:$2|nu file ricupiratu|$2 file ricupirati}}',
'undeletedfiles'               => '{{PLURAL:$1|un file ricupiratu|$1 file ricupirati}}',
'cannotundelete'               => 'Lu ricùpiru nun è arrinisciutu: quarcunu àutru putissi aviri già ricupiratu la pàggina.',
'undeletedpage'                => "<big>'''La pàggina $1 hà statu ricupirata'''</big> Cunzurta lu [[Special:Log/delete|log dî cancillazzioni]] pi vìdiri li cancillazzioni e li ricùpiri cchiù ricenti.",
'undelete-header'              => 'Vidi lu [[Special:Log/delete|log dî cancillazzioni]] pi li pàggini cancillati di ricenti.',
'undelete-search-box'          => 'Cerca li pàggini cancillati',
'undelete-search-prefix'       => 'Ammustra li pàggini lu cui tìtulu nizzia cu:',
'undelete-search-submit'       => 'Cerca',
'undelete-no-results'          => "Nuddu risurtatu attruvatu nta l'archiviu dî pàggini cancillati.",
'undelete-filename-mismatch'   => 'Mpussibbili annullari la cancillazzioni dâ rivisioni dô file cû timestamp $1: nomu file nun currispunnenti.',
'undelete-bad-store-key'       => 'Mpussibile annullari la cancillazzioni dâ rivisioni dû file cû timestamp $1: file nun dispunibbili prima dâ cancillazzioni.',
'undelete-cleanup-error'       => 'Erruri ntâ cancillazzioni dû file d\'archiviu nun usatu "$1".',
'undelete-missing-filearchive' => "Mpussibbili ripristinari l'ID $1 de l'archiviu file picchì nun è ntô databbasi. Pò èssiri già statu ripristinatu.",
'undelete-error-short'         => 'Erruri ntô ripristinu dû file: $1',
'undelete-error-long'          => 'Si virificaru erruri ntô tentativu di annullari la cancillazzioni dô file:

$1',

# Namespace form on various pages
'namespace'      => 'Tipu di pàggina:',
'invert'         => 'scancia la silizzioni',
'blanknamespace' => '(Principali)',

# Contributions
'contributions' => 'cuntribbuti',
'mycontris'     => 'Li mei cuntribbuti',
'contribsub2'   => 'Pi $1 ($2)',
'nocontribs'    => 'Secunnu sti criteri nun ci sunnu canci o cuntribbuti.',
'uctop'         => '(ùrtima pi la pàggina)',
'month'         => 'A pàrtiri dô mese (e pricidenti):',
'year'          => "A pàrtiri di l'annu (e pricidenti):",

'sp-contributions-newbies'     => "Ammustra sulu li cuntribbuti di l'utenti novi",
'sp-contributions-newbies-sub' => 'Pi li utenti novi',
'sp-contributions-blocklog'    => 'log dî blocchi',
'sp-contributions-search'      => 'Ricerca cuntribbuti',
'sp-contributions-username'    => 'Nnirizzu IP o nomu utenti:',
'sp-contributions-submit'      => 'Ricerca',

# What links here
'whatlinkshere'            => 'Chi punta ccà',
'whatlinkshere-title'      => 'Pàggini ca pùntanu a "$1"',
'whatlinkshere-page'       => 'Pàggina:',
'linklistsub'              => '(Lista di liami)',
'linkshere'                => "Sti pàggini hannu nu liami a '''[[:$1]]''':",
'nolinkshere'              => "Nudda pàggina havi nu liami a '''[[:$1]]'''.",
'nolinkshere-ns'           => "Nun ci sugnu pàggini chi puntano a '''[[:$1]]''' ntô namespace silizziunatu.",
'isredirect'               => 'pàggina di rinnirizzamentu',
'istemplate'               => 'nchiusioni',
'whatlinkshere-prev'       => '{{PLURAL:$1|pricidenti|pricidenti $1}}',
'whatlinkshere-next'       => '{{PLURAL:$1|succissivu|succissivi $1}}',
'whatlinkshere-links'      => '← liami',
'whatlinkshere-hideredirs' => '$1 redirect',
'whatlinkshere-hidetrans'  => '$1 nclusioni',
'whatlinkshere-hidelinks'  => '$1 link',
'whatlinkshere-filters'    => 'Filtri',

# Block/unblock
'blockip'                     => "Blocca l'utenti",
'blockip-legend'              => "Blocca l'utenti",
'blockiptext'                 => "Usa lu mòdulu cassutta pi bluccari la pussibbilità di scrìviri pi n'utenti o pi nu ndirizzu IP spicìficu. Chistu s'havi a fari sulu pi privèniri lu vannalismu e secunnu la [[{{MediaWiki:Policy-url}}|pulìtica di {{SITENAME}}]]. Scrivi na raggiùni valida ccà sutta (pi asempiu, cita li pàggini chi foru vannalizzati).",
'ipaddress'                   => 'Ndirizzu IP:',
'ipadressorusername'          => 'Ndirizzu IP o nomu utenti:',
'ipbexpiry'                   => 'Durata dû bloccu:',
'ipbreason'                   => 'Mutivu dû bloccu:',
'ipbreasonotherlist'          => 'Àutru mutivu',
'ipbreason-dropdown'          => '*Mutivi cchiù cumuni pî blocchi
** Nzerimentu di nformazziuni falsi
** Cancillazzioni di cuntinuti dê pàggini
** Liami prumozziunalu a siti sterni
** Nzserimentu di cuntinuti privi di sensu
** Cumportamenti ntimidatori o molestie
** Usu ndebitu di cchiù cunti
** Nomu utenti nun accittabbili',
'ipbanononly'                 => "Blocca sulu l'utenti anònimi (l'utenti riggistrati ca cundivìdinu lu stissu IP nun vèninu bluccati)",
'ipbcreateaccount'            => 'Mpidisci la criazzioni di àutri account',
'ipbemailban'                 => "Mpedisci a l'utenti l'inviu di email",
'ipbenableautoblock'          => "Blocca automaticamenti l'ùrtimu ndirizzu IP usatu di l'utenti e li succissivi cu cui vèninu tintati canciamenti",
'ipbsubmit'                   => "Blocca st'utenti",
'ipbother'                    => 'Durata nun n alencu',
'ipboptions'                  => '2 uri:2 hours,1 jornu:1 day,3 jorna:3 days,1 simana:1 week,2 simani:2 weeks,1 misi:1 month,3 misi:3 months,6 misi:6 months,1 annu:1 year,nfinitu:infinite', # display1:time1,display2:time2,...
'ipbotheroption'              => 'àutru',
'ipbotherreason'              => 'Àutri mutivi/dittagghi:',
'ipbhidename'                 => "Ammuccia lu nomu utenti dô log dî blocchi, di l'alencu dî blocchi attivi e di l'alencu utenti.",
'badipaddress'                => 'Ndirizzu IP nun vàlidu.',
'blockipsuccesssub'           => 'Bloccu esiquitu',
'blockipsuccesstext'          => "[[Special:Contributions/$1|$1]] fu bluccatu.<br />
Pi maggiuri nfurmazzioni, talìa la [[Special:IPBlockList|lista di l'IP bluccati]] .",
'ipb-edit-dropdown'           => 'Mutivi pô bloccu',
'ipb-unblock'                 => "Sblocca n'utenti o nu ndirizzu IP",
'ipb-blocklist-addr'          => 'Alenca li blocchi attivi pi $1',
'ipb-blocklist'               => 'Alenca li blocchi attivi',
'unblockip'                   => 'Sblocca ndirizzu IP',
'unblockiptext'               => "Usari lu mòdulu suttastanti pi ristituiri l'accessu n scrittura a un utenti o ndirizzu IP bluccatu.",
'ipusubmit'                   => "Sblocca l'utenti",
'unblocked'                   => "L'utenti [[User:$1|$1]] hà statu sbluccatu",
'unblocked-id'                => 'Lu bloccu $1 hà statu cacciatu',
'ipblocklist'                 => 'Utenti e nnirizzi IP bluccati',
'ipblocklist-legend'          => "Atrova n'utenti bluccatu",
'ipblocklist-username'        => 'Nomu utenti o nnirizzu IP:',
'blocklistline'               => '$1, $2 hà bluccatu $3 ($4)',
'infiniteblock'               => 'nfinitu',
'expiringblock'               => 'scadenza: $1',
'anononlyblock'               => 'sulu anònimi',
'noautoblockblock'            => 'bloccu automàticu disabbilitatu',
'createaccountblock'          => 'criazzioni account bluccata',
'emailblock'                  => 'email bluccati',
'ipblocklist-empty'           => "L'alencu dî blocchi è vacanti.",
'ipblocklist-no-results'      => 'Lu nnirizzu IP o nomu utenti richiestu nun è bluccatu.',
'blocklink'                   => 'blocca',
'unblocklink'                 => 'sblocca',
'contribslink'                => 'cuntribbuti',
'autoblocker'                 => 'Bluccatu automaticamenti pirchì lu ndirizzu IP è cundivisu cu l\'utenti "[[User:$1|$1]]". Lu bloccu di l\'utenti $1 fu mpostu pi lu siquenti mutivu: "\'\'\'$2\'\'\'".',
'blocklogpage'                => 'Blocchi',
'blocklogentry'               => 'hà bluccatu [[$1]]; scadenza $2 $3',
'blocklogtext'                => "Chistu è l'alencu di l'azzioni di bloccu e sbloccu utenti. Li ndirizzi IP bluccati automaticamenti nun sunu alincati. Cunzurtari l'[[Special:IPBlockList|alencu IP bluccati]] pi l'alencu dî ndirizzi e noma utenti lu cui bloccu è opirativu.",
'unblocklogentry'             => 'hà sbluccatu "$1"',
'block-log-flags-anononly'    => 'sulu utenti anònimi',
'block-log-flags-nocreate'    => 'criazzioni account bluccata',
'block-log-flags-noautoblock' => 'bloccu automàticu disattivatu',
'block-log-flags-noemail'     => 'email bluccati',
'range_block_disabled'        => 'La pussibbilitati di bluccari ntervalli di ndirizzi IP è disattiva a lu mumentu.',
'ipb_expiry_invalid'          => 'Durata o scadenza dû bloccu nun vàlida.',
'ipb_already_blocked'         => 'L\'utenti "$1" è già bluccatu',
'ipb_cant_unblock'            => 'Erruri: Mpussìbbili attruvari lu bloccu cu ID $1. Putissi aviri già statu sbluccatu.',
'ip_range_invalid'            => 'Ntervallu di ndirizzi IP nun vàlidu.',
'proxyblocker'                => 'Blocca proxy',
'proxyblockreason'            => "Lu tò ndirizzu IP hà statu bluccatu pirchì è un open proxy. Pi favuri cuntatta lu tò furnituri d'accessu a Internet o lu supportu tècnicu e nfòrmali di stu gravi prubbrema di sicurizza.",
'proxyblocksuccess'           => 'Esiquitu.',
'sorbsreason'                 => 'Lu tò ndirizzu IP è alincatu comu proxy apertu ntâ lista DNSBL.',
'sorbs_create_account_reason' => 'Lu tò ndirizzu IP è alincatu comu open proxy ntâ DNSBL. Nun poi criari un utenti.',

# Developer tools
'lockdb'              => 'Blocca lu database',
'unlockdb'            => 'Sblocca lu database',
'lockdbtext'          => "Lu bloccu dû database cumporta la nterruzzioni, pi tutti l'utenti, dâ pussibbilitati di mudificari li pàggini o di criàrinni di novi, di canciari li prifirenzi e mudificari li listi di l'ossirvati spiciali, e n ginirali di tutti l'upirazzioni ca richièdinu canciamenti a lu database. Pi favuri, cunferma ca chistu currispunni effittivamenti a l'azzioni di tia richiesta e ca a lu tèrmini dâ manutinzzioni pruvidi a  lu sbloccu dû database.",
'unlockdbtext'        => "Lu sbloccu dû database cunzenti di novu a tutti li utenti di canciari li pàggini o di criàrinni di novi, di canciari li prifirenzi e canciari li listi di l'ossirvati spiciali, e n ginirali di còmpiri tutti li upirazzioni ca richièdinu canciamenti a lu database. Pi curtisìa, cunferma ca chistu currispunni effittivamenti a l'azzioni di tìa addumannata.",
'lockconfirm'         => 'Sì, ntennu effittivamenti bluccari lu database.',
'unlockconfirm'       => 'Sì, effittivamenti ntennu, sutta la mè rispunzabbilitati, sbluccari lu database.',
'lockbtn'             => 'Blocca lu database',
'unlockbtn'           => 'Sblocca lu database',
'locknoconfirm'       => 'Nun hà statu spuntata la casillina di cunferma.',
'lockdbsuccesssub'    => 'Bloccu dû database esiquitu',
'unlockdbsuccesssub'  => 'Sbloccu dû database esiquitu',
'lockdbsuccesstext'   => "Lu database hà statu bluccatu. 
<br />Arricorda di [[Special:UnlockDB|rimòviri lu bloccu]] doppu aviri accabbatu l'upirazzioni di manutinzioni.",
'unlockdbsuccesstext' => 'Lu database hà statu sbluccatu.',
'lockfilenotwritable' => "Mpussìbbili scrìviri supra lu file di ''lock'' dû database. L'accessu n scrittura a tali file di parti dû server web è nicissariu pi bluccari e sbluccari lu database.",
'databasenotlocked'   => 'Lu database nun è bluccatu.',

# Move page
'move-page-legend'        => 'Sposta la pàggina',
'movepagetext'            => "Usannu lu mòdulu ccà sutta vui canciati lu nomu dâ pàggina, e spustati tutta la sò storia versu la pàggina nova. Lu tìtulu vecchiu addiventa na pàggina di ''redirect'' versu lu tìtulu novu.
Li liami â pàggina vecchia nun càncianu.
Assicuràtivi ca lu spustamentu nun havi criatu [[Special:DoubleRedirects|redirect duppi]] o [[Special:BrokenRedirects|redirect rumputi]]. Vui siti rispunzàbbili dî liami chi avìssiru a puntari â pàggina giusta.

La pàggina '''nun''' è spustata siddu cc'è già na pàggina cu lu tìtulu novu, tranni chi la pàggina 'n chistioni è vacanti o è na pàggina di ''redirect'' e nun havi n'archiviu di canciamenti.
Chistu signìfica chi vui putiti rinuminari la pàggina cu lu nomu vecchiu si aviti sbagghiatu, e chi nun putiti suprascrìviri nta na pàggina chi esisti già.

'''Accura!'''
Chistu pò èssiri nu canciamentu dràsticu pi na pàggina pupulari; aviti a èssiri sicuri di capiri li cunziquenzi prima di cuntinuari.",
'movepagetalktext'        => "La pàggina di discussioni assuciata, siddu esisti, veni spustata automaticamenti nzèmmula, '''a menu chi:'''
*Na pàggina nun-vacanti di discussioni già esisti cu lu nomu novu,
*Hai disilizziunatu lu quatratu ccà sutta.

Nta sti casi, tu hai a spustari o agghiùnciri manuarmenti la pàggina di discussioni.",
'movearticle'             => 'Sposta la pàggina',
'movenotallowed'          => 'Nun hai li pirmessi nicissari a lu spustamentu dê pàggini.',
'newtitle'                => 'Cu lu tìtulu novu di',
'move-watch'              => 'Talìa sta pàggina',
'movepagebtn'             => 'Sposta la pàggina',
'pagemovedsub'            => 'Lu spustamentu riniscìu.',
'articleexists'           => "Na pàggina cu stu nomu esisti già, oppuru lu nomu scigghiutu nun è vàlidu. Scègghiri n'àutru tìtulu.",
'talkexists'              => "'''La pàggina hà statu spustata currettamenti, ma nun hà statu pussìbbili spustari la pàggina di discussioni pirchì nn'esisti già n'àutra cu lu tìtulu novu. Ntigrari manuarmenti li cuntinuti dî dui pàggini.'''",
'movedto'                 => 'spustata a',
'movetalk'                => 'Sposta puru la pàggina di discussioni, eventuarmenti.',
'1movedto2'               => '[[$1]] spustatu a [[$2]]',
'1movedto2_redir'         => '[[$1]] spustatu a [[$2]] supra rinnirizzamentu',
'movelogpage'             => 'Spustamenti',
'movelogpagetext'         => "Chistu è l'alencu dî pàggini spustati.",
'movereason'              => 'Pi stu mutivu',
'revertmove'              => 'riprìstina',
'delete_and_move_text'    => '==Richiesta di cancillazzioni==

La pàggina di distinazzioni "[[:$1]]" asisti già. S\'addisìa cancillàrila pi rènniri pussìbbili lu spustamentu?',
'delete_and_move_confirm' => 'Sì, suvrascrivi la pàggina asistenti',
'delete_and_move_reason'  => 'Cancillata pi rènniri pussìbbili lu spustamentu',
'selfmove'                => 'Lu tìtulu di distinazzioni nziritu è agguali a chiddu di pruvinenza; mpossibbili spustari la pàggina su idda stissa.',
'immobile_namespace'      => 'Lu novu tìtulu currispunni a na pàggina spiciali; mpussìbbili spustari pàggini nta ddu namespace.',

# Export
'export'            => 'Esporta pàggini',
'exporttext'        => "È pussìbbili espurtari lu testu e la cronoluggìa dî canciamenti di na pàggina o d'un gruppu di pàggini n furmatu XML pi mpurtàrili n àutri siti ca utilìzzanu lu software MediaWiki, attraversu la pàggina [[Special:Import|d'importu]].

P'espurtari li pàggini innicari li tìtuli ntâ casella di testu suttastanti, unu pi riga, e spicificari siddu s'addisìa attèniri la virsioni currenti e tutti li virsioni pricidenti, cu li dati dâ cronoluggìa dâ pàggina, oppuru surtantu l'ùrtima virsioni e li dati currispunnenti a l'ùrtimu canciamentu. 

Nta st'ùrtimu casu si pò macari utilizzari un culligamentu, p'asempiu [[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]] p'espurtari \"[[{{MediaWiki:Mainpage}}]]\".",
'exportcuronly'     => 'Ncludi sulu la rivisioni attuali, nun la ntera cronoluggìa',
'exportnohistory'   => "---- '''Nota:''' l'espurtazzioni dâ ntera cronoluggìa dî pàggini attraversu sta nterfaccia hà stata disattivata pi mutivi ligati a li pristazzioni dû sistema.",
'export-submit'     => 'Espurtazzioni',
'export-addcattext' => 'Agghiunci pàggini dâ catigurìa:',
'export-addcat'     => 'Agghiunci',
'export-download'   => 'Offri di sarvari comu file',

# Namespace 8 related
'allmessages'               => 'Missaggi di sistema',
'allmessagesname'           => 'Nomu',
'allmessagesdefault'        => 'Testu pridifinitu',
'allmessagescurrent'        => 'Testu attuali',
'allmessagestext'           => "Chista è na lista di missaggi di sistema chi s'attròvanu sutta MediaWiki:''nomu''.",
'allmessagesnotsupportedDB' => "'''{{ns:special}}:Allmessages''' nun è suppurtatu pirchì lu flag '''\$wgUseDatabaseMessages''' nun è attivu.",
'allmessagesfilter'         => 'Filtru supra li missaggi:',
'allmessagesmodified'       => 'Ammustra sulu chiddi mudificati',

# Thumbnails
'thumbnail-more'           => 'Ngrannisci',
'filemissing'              => 'File mancanti',
'thumbnail_error'          => 'Erruri ntâ criazzioni dâ miniatura: $1',
'djvu_page_error'          => 'Nùmmuru di pàggina DjVu erratu',
'djvu_no_xml'              => 'Mpussibbili òtteniri lu XML pô file DjVu',
'thumbnail_invalid_params' => 'Parametri antiprima nun validi',
'thumbnail_dest_directory' => 'Mpussibbili criari la directory di distinazzioni',

# Special:Import
'import'                     => 'Mporta pàggini',
'importinterwiki'            => 'Mpurtazzioni transwiki',
'import-interwiki-text'      => "Silizziunari un pruggettu wiki e lu tìtulu dâ pàggina a mpurtari. Li dati di pubbricazzioni e li noma di l'autura dî vari virsioni sunnu sarvati. Tutti l'opirazzioni di mpurtazzioni trans-wiki sunnu riggistrati ntô [[Special:Log/import|log di mpurtazzioni]].",
'import-interwiki-history'   => 'Copia la ntera cronoluggìa di sta pàggina',
'import-interwiki-submit'    => 'Mporta',
'import-interwiki-namespace' => 'Trasfirisci li pàggini ntô namespace:',
'importtext'                 => "Pi favuri, esporta lu file dâ wiki d'orìggini usannu l'utility Speciale:Export, sàrvalu supra lu tò discu e carrìcalu ccà",
'importstart'                => 'Mpurtazzioni dî pàggini n cursu...',
'import-revision-count'      => '{{PLURAL:$1|na rivisioni mpurtata|$1 rivisioni mpurtati}}',
'importnopages'              => 'Nudda pàggina a mpurtari.',
'importfailed'               => 'Mpurtazzioni nun arrinisciuta: $1',
'importunknownsource'        => "Tipu d'orìggini scanusciutu pi la mpurtazzioni",
'importcantopen'             => 'Mpussìbbili grapiri lu file di mpurtazzioni',
'importbadinterwiki'         => 'Culligamentu inter-wiki erratu',
'importnotext'               => 'Testu vacanti o mancanti',
'importsuccess'              => 'Mpurtazzioni arrinisciuta.',
'importhistoryconflict'      => 'Asìstinu rivisioni dâ cronoluggìa n cunflittu (sta pàggina putissi aviri già statu mpurtata)',
'importnosources'            => 'Nun hà statu difinita na fonti pi la mpurtazzioni transwiki; la mpurtazzioni diretta dâ cronoluggìa nun è attiva.',
'importnofile'               => 'Nun hà statu carrcatu nuddu file pi la mpurtazzioni.',

# Import log
'importlogpage'                    => 'Mpurtazzioni',
'importlogpagetext'                => "Riggistru dî mpurtazzioni d'ufficiu di pàggini pruvinenti d'àutri wiki, cumpleti di cronoluggìa.",
'import-logentry-upload'           => 'hà mpurtatu $1 tràmiti upload',
'import-logentry-upload-detail'    => '{{PLURAL:$1|na rivisioni mpurtata|$1 rivisioni mpurtati}}',
'import-logentry-interwiki'        => 'hà trasfiritu di àutra wiki la pàggina $1',
'import-logentry-interwiki-detail' => '{{PLURAL:$1|na rivisioni mpurtata|$1 rivisioni mpurtati}} di $2',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'La tò pàggina utenti',
'tooltip-pt-anonuserpage'         => 'La pàggina utenti di stu ndirizzu IP',
'tooltip-pt-mytalk'               => 'La mè pàggina di discussioni',
'tooltip-pt-anontalk'             => 'Discussioni supra li canciamenti fatti di stu ndirizzu IP',
'tooltip-pt-preferences'          => 'Li mè prifirenzi',
'tooltip-pt-watchlist'            => 'La lista dî pàggini ca stai tinennu sutta ossirvazzioni',
'tooltip-pt-mycontris'            => "L'alencu dî tò cuntribbuti",
'tooltip-pt-login'                => 'La riggistrazzioni è cunzigghiata, puru siddu nun obbrigatoria.',
'tooltip-pt-anonlogin'            => 'La riggistrazzioni è cunzigghiata, puru siddu nun obbrigatoria.',
'tooltip-pt-logout'               => 'Nisciuta (logout)',
'tooltip-ca-talk'                 => 'Vidi li discussioni rilativi a sta pàggina',
'tooltip-ca-edit'                 => "Poi canciari sta pàggina. Pi favuri usa lu pulsanti d'antiprima prima di sarvari.",
'tooltip-ca-addsection'           => 'Agghiunci un cummentu a sta discussioni.',
'tooltip-ca-viewsource'           => 'Sta pàggina è prutetta, ma poi vìdiri lu sò còdici surgenti.',
'tooltip-ca-history'              => 'Virsioni pricidenti di sta pàggina.',
'tooltip-ca-protect'              => 'Pruteggi sta pàggina',
'tooltip-ca-delete'               => 'Cancella sta pàggina',
'tooltip-ca-undelete'             => "Riprìstina la pàggina com'era prima dâ cancillazzioni",
'tooltip-ca-move'                 => 'Sposta sta pàggina (cancia tìtulu)',
'tooltip-ca-watch'                => 'Agghiunci sta pàggina â tò lista di ossirvati spiciali',
'tooltip-ca-unwatch'              => 'Elìmina sta pàggina dâ tò lista di ossirvati spiciali',
'tooltip-search'                  => "Cerca 'n {{SITENAME}}",
'tooltip-search-go'               => 'Vai a na pàggina cu chistu nomu esattu si asisti',
'tooltip-search-fulltext'         => 'Attrova pàggini pi chistu testu',
'tooltip-p-logo'                  => 'Pàggina principali',
'tooltip-n-mainpage'              => 'Vìsita la pàggina principali',
'tooltip-n-portal'                => 'Discrizzioni dû pruggettu, zoccu poi fari, unni attruvari li cosi',
'tooltip-n-currentevents'         => "Nfurmazzioni supra l'avvinimenti d'attualitati",
'tooltip-n-recentchanges'         => "Alencu di l'ùrtimi canciamenti dû situ.",
'tooltip-n-randompage'            => 'Ammustra na pàggina a muzzu',
'tooltip-n-help'                  => "Pàggini d'aiutu.",
'tooltip-t-whatlinkshere'         => 'Alencu di tutti li pàggini ca sunnu culligati a chista',
'tooltip-t-recentchangeslinked'   => "Alencu di l'ùrtimi canciamenti a li pàggini culligati a chista",
'tooltip-feed-rss'                => 'Feed RSS pi sta pàggina',
'tooltip-feed-atom'               => 'Feed Atom pi sta pàggina',
'tooltip-t-contributions'         => 'Lista dî cuntribbuti di stu utenti',
'tooltip-t-emailuser'             => 'Manna un missaggiu e-mail a stu utenti',
'tooltip-t-upload'                => 'Càrrica mmàggini o file multimidiali',
'tooltip-t-specialpages'          => 'Lista di tutti li pàggini spiciali',
'tooltip-t-print'                 => 'Virsioni stampabbili di chista pàggina',
'tooltip-t-permalink'             => 'Liami pirmanenti a chista virsioni dâ pàggina',
'tooltip-ca-nstab-main'           => "Vidi l'artìculu",
'tooltip-ca-nstab-user'           => 'Vidi la pàggina utenti',
'tooltip-ca-nstab-media'          => 'Vidi la pàggina dû file multimidiali',
'tooltip-ca-nstab-special'        => 'Chista è na pàggina spiciali, nun pò èssiri canciata',
'tooltip-ca-nstab-project'        => 'Vidi la pàggina di sirvizziu',
'tooltip-ca-nstab-image'          => 'Vidi la pàggina dâ mmàggini',
'tooltip-ca-nstab-mediawiki'      => 'Vidi lu missaggiu di sistema',
'tooltip-ca-nstab-template'       => 'Vidi lu template',
'tooltip-ca-nstab-help'           => "Vidi la pàggina d'aiutu",
'tooltip-ca-nstab-category'       => 'Vidi la pàggina dâ catigurìa',
'tooltip-minoredit'               => 'Signala comu canciamentu nicu',
'tooltip-save'                    => 'Sarva li canciamenti',
'tooltip-preview'                 => 'Antiprima dî canciamenti, ùsala prima di sarvari!',
'tooltip-diff'                    => "Talìa (mudalitati diff) li canciamenti c'hai fattu.",
'tooltip-compareselectedversions' => 'Talìa li diffirenzi tra li dui virsioni silizziunati di sta pàggina.',
'tooltip-watch'                   => "Agghiunci sta pàggina â lista di l'ossirvati spiciali",
'tooltip-recreate'                => 'Ricrea la pàggina puru siddu hà statu cancillata',
'tooltip-upload'                  => 'Ncigna carricamentu',

# Stylesheets
'common.css'   => "/* Li stili CSS nziriti ccà s'àpplicanu a tutti li skin */",
'monobook.css' => "/* Li stili CSS nziriti ccà s'àpplicanu a l'utenti chi usanu la skin Monobook */",

# Scripts
'common.js'   => "/* Lu còdici JavaScript nziritu ccà veni carricatu di ognuna pàggina, pi tutti l'utenti. */",
'monobook.js' => "/* Lu còdici JavaScript nzirutu ccà veni carricatu di l'utenti c'ùsanu la skin MonoBook */",

# Metadata
'nodublincore'      => 'Dublin Core RDF metadata disabbilitatu pi stu server.',
'nocreativecommons' => 'Creative Commons RDF metadata disabbilitatu pi stu server.',
'notacceptable'     => 'Lu server wiki nun pò furniri dati nta un furmatu liggìbbili dû tò client.',

# Attribution
'anonymous'        => 'unu o cchiù utenti anònimi di {{SITENAME}}',
'siteuser'         => '$1, utenti di {{SITENAME}}',
'lastmodifiedatby' => "Sta pàggina hà statu canciata pi l'ùrtima vota lu $2, $1 di $3.", # $1 date, $2 time, $3 user
'othercontribs'    => 'Basatu supra lu travagghiu di $1.',
'others'           => 'àutri',
'creditspage'      => 'Li autura dâ pàggina',
'nocredits'        => 'Nudda nfurmazzioni supra li crèditi dispunìbbili pi sta pàggina.',

# Spam protection
'spamprotectiontitle' => 'Filtru anti-spam',
'spamprotectiontext'  => 'La pàggina ca vulevi sarvari hà statu bluccata dû filtru anti-spam. Chistu è prubbabbirmenti duvutu â prisenza di nu liami a nu situ sternu bluccatu.',
'spamprotectionmatch' => 'Lu nostru filtru anti-spam hà ndividuatu lu testu siquenti: $1',
'spambot_username'    => 'MediaWiki - sistema di rimuzzioni spam',
'spam_reverting'      => "Ripristinata l'ùrtima virsioni priva di culligamenti a $1",
'spam_blanking'       => 'Pàggina svacantata, tutti li virsioni cuntinìanu culligamenti a $1',

# Info page
'infosubtitle'   => 'Nfurmazzioni pi la pàggina',
'numedits'       => 'Nùmmuru di canciamenti (artìculu): $1',
'numtalkedits'   => 'Nùmmuru di canciamenti (pàggina di discussioni): $1',
'numwatchers'    => "Nùmmuru d'ossirvatura: $1",
'numauthors'     => "Nùmmuru d'autura distinti (artìculu): $1",
'numtalkauthors' => "Nùmmuru d'autura distinti (pàggina di discussioni): $1",

# Math options
'mw_math_png'    => "Ammustra sempri 'n PNG",
'mw_math_simple' => 'HTML siddu veru sìmplici, oppuru PNG',
'mw_math_html'   => 'HTML siddu pussìbbili, oppuru PNG',
'mw_math_source' => 'Lassa comu TeX (pi browser tistuali)',
'mw_math_modern' => 'Raccumannatu pi li browser muderni',
'mw_math_mathml' => 'MathML siddu pussìbbili (spirimintali)',

# Patrolling
'markaspatrolleddiff'                 => 'Segna lu canciamentu comu virificatu',
'markaspatrolledtext'                 => 'Segna sta pàggina comu virificata',
'markedaspatrolled'                   => 'Canciamentu virificatu',
'markedaspatrolledtext'               => 'Lu canciamentu silizziunatu hà statu signatu comu virificatu.',
'rcpatroldisabled'                    => "La virìfica di l'ùrtimi canciamenti è disattivata",
'rcpatroldisabledtext'                => "La funzioni di virìfica di l'ùrtimi canciamenti a lu mumentu nun è attiva.",
'markedaspatrollederror'              => 'Mpussìbbili contrassignari lu canciamentu comu virificatu',
'markedaspatrollederrortext'          => 'Ci voli spicificari un canciamentu a contrassignari comu virificatu.',
'markedaspatrollederror-noautopatrol' => 'Nun si disponi dî pirmissi nicissari pi signari li propi canciamenti comu virificati.',

# Patrol log
'patrol-log-page' => 'Canciamenti virificati',
'patrol-log-line' => 'hà signatu la $1 di $2 comu virificata $3',
'patrol-log-auto' => '(virìfica automàtica)',
'patrol-log-diff' => 'virsioni $1',

# Image deletion
'deletedrevision'                 => 'Rivisioni pricidenti, cancillata: $1.',
'filedeleteerror-short'           => 'Erruri ntâ cancillazzioni dû file: $1',
'filedeleteerror-long'            => 'Si virificaru erruri ntô tentativu di cancillari lu file:

$1',
'filedelete-missing'              => 'Mpussibbili cancillari lu file "$1" pirchì nun asisti.',
'filedelete-old-unregistered'     => 'La rivisioni dô file nnicata, "$1", nun è cuntinuta ntô databbasi.',
'filedelete-current-unregistered' => 'Lu file spicificatu, "$1", nun è cuntinutu ntô databbasi.',
'filedelete-archive-read-only'    => 'Lu server Web nun è capaci di scrìviri ntâ directory d\'archiviu "$1".',

# Browsing diffs
'previousdiff' => '← Diffirenza pricidenti',
'nextdiff'     => 'Diffirenza siquenti →',

# Media information
'mediawarning'         => "'''Accura''': Stu file pò cuntèniri còdici malignu, esiquènnulu lu vostru sistema putisi vèniri cumprumissu. <hr />",
'imagemaxsize'         => 'Diminzioni màssima dî mmàggini supra li rilativi pàggini di o:',
'thumbsize'            => 'Grannizza dî miniaturi:',
'widthheightpage'      => '$1×$2, $3 {{PLURAL:$3|pàggina|pàggini}}',
'file-info'            => '(Diminzioni: $1, tipu MIME: $2)',
'file-info-size'       => '($1 × $2 pixel, diminzioni: $3, tipu MIME: $4)',
'file-nohires'         => '<small>Nun sunnu dispunìbbili virsioni a risuluzzioni cchiù elivata.</small>',
'svg-long-desc'        => '(file SVG, dimensioni nominali $1 × $2 pixel, dimensioni dô file: $3)',
'show-big-image'       => 'Virsioni a àuta risuluzzioni',
'show-big-image-thumb' => "<small>Diminzioni di st'antiprima: $1 × $2 pixel</small>",

# Special:NewImages
'newimages'             => 'Gallarìa dî file novi',
'imagelisttext'         => "Di sèquitu veni prisintata na lista di '''$1''' file urdinat{{PLURAL:$1|u|i}} pi $2.",
'showhidebots'          => '($1 li bot)',
'noimages'              => 'Nenti a vìdiri.',
'ilsubmit'              => "Va' cerca",
'bydate'                => 'pi data',
'sp-newimages-showfrom' => "Ammustra li mmàggini cchiù ricenti a pàrtiri d'uri $2 dô $1",

# Bad image list
'bad_image_list' => "Lu furmatu è lu siquenti:

Vèninu cunzidirati sulu l'alenchi puntati (righi ca accumènzanu cû sìmmulu *). Lu primu lijami supra ogni riga havi a èssiri nu lijami a nu file nun addisiatu.
Li lijami succissivi, supra la stissa riga, sunnu cunzidirati comu eccizzioni (pàggini ntê quali lu file pò èssiri richiamatu 'n modu nurmali).",

# Metadata
'metadata-help'     => 'Stu file cunteni nfurmazzioni agghiuntivi, prubbabbirmenti junti dâ fotucàmira o dû scanner usati pi criàrila o diggitalizzàrila. Siddu lu file hà statu canciatu, arcuni dittagghi putìssiru nun currispùnniri â rialitati.',
'metadata-expand'   => 'Ammustra dittagghi',
'metadata-collapse' => 'Ammuccia dittagghi',
'metadata-fields'   => "Li campi rilativi a li metadati EXIF alincati 'n stu missaggiu vèninu ammustrati supra la pàggina dâ mmàggini quannu la tabbella dî metadati è prisintata ntâ forma brivi. Pi mpustazzioni pridifinita, l'àutri campi vèninu ammucciati.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength", # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => 'Larghizza',
'exif-imagelength'                 => 'Autizza',
'exif-bitspersample'               => 'Bit pi campiuni',
'exif-compression'                 => 'Miccanismu di cumprissioni',
'exif-photometricinterpretation'   => 'Struttura dî pixel',
'exif-orientation'                 => 'Urientamentu',
'exif-samplesperpixel'             => 'Nùmmuru dî cumpunenti',
'exif-planarconfiguration'         => 'Dispusizzioni dî dati',
'exif-ycbcrsubsampling'            => 'Rapportu di campiunamentu Y / C',
'exif-ycbcrpositioning'            => 'Pusizziunamentu cumpunenti Y e C',
'exif-xresolution'                 => 'Risuluzzioni urizzuntali',
'exif-yresolution'                 => 'Risuluzzioni virticali',
'exif-resolutionunit'              => 'Unitati di misura risuluzzioni X e Y',
'exif-stripoffsets'                => 'Pusizzioni dî dati mmàggini',
'exif-rowsperstrip'                => 'Nùmmiru righi pi striscia',
'exif-stripbytecounts'             => 'Nùmmiru di byte pi striscia cumpressa',
'exif-jpeginterchangeformat'       => 'Pusizzioni byte SOI JPEG',
'exif-jpeginterchangeformatlength' => 'Nùmmuru di byte di dati JPEG',
'exif-transferfunction'            => 'Funzioni di trasfirimentu',
'exif-whitepoint'                  => 'Cuurdinati crumàtichi dû puntu di jancu',
'exif-primarychromaticities'       => 'Cuurdinati crumàtichi dî culuri primari',
'exif-ycbcrcoefficients'           => 'Cuefficienti matrici di trasfurmazzioni spazzi dî culuri',
'exif-referenceblackwhite'         => 'Cucchia di valuri di rifirimentu (nìuru e jancu)',
'exif-datetime'                    => 'Data e ura di canciamentu dû file',
'exif-imagedescription'            => 'Discrizzioni dâ mmàggini',
'exif-make'                        => 'Prudutturi fotucàmira',
'exif-model'                       => 'Mudellu fotucàmira',
'exif-software'                    => 'Software',
'exif-artist'                      => 'Auturi',
'exif-copyright'                   => 'Nfurmazzioni supra lu copyright',
'exif-exifversion'                 => 'Virsioni dû furmatu Exif',
'exif-flashpixversion'             => 'Virsioni Flashpix suppurtata',
'exif-colorspace'                  => 'Spazziu dî culuri',
'exif-componentsconfiguration'     => "Significatu d'ognuna cumpunenti",
'exif-compressedbitsperpixel'      => 'Mudalitati di cumprissioni dâ mmàggini',
'exif-pixelydimension'             => 'Larghizza effittiva mmàggini',
'exif-pixelxdimension'             => 'Autizza effittiva mmàggini',
'exif-makernote'                   => 'Noti dû prudutturi',
'exif-usercomment'                 => "Noti di l'utenti",
'exif-relatedsoundfile'            => 'File audiu culligatu',
'exif-datetimeoriginal'            => 'Data e ura di criazzioni dî dati',
'exif-datetimedigitized'           => 'Data e ura di diggitalizzazzioni',
'exif-subsectime'                  => 'Data e ura, frazzioni di secunnu',
'exif-subsectimeoriginal'          => 'Data e ura di criazzioni, frazzioni di secunnu',
'exif-subsectimedigitized'         => 'Data e ura di diggitalizzazzioni, frazzioni di secunnu',
'exif-exposuretime'                => "Tempu d'espusizzioni",
'exif-fnumber'                     => 'Rapportu fucali',
'exif-exposureprogram'             => "Prugramma d'espusizzioni",
'exif-spectralsensitivity'         => 'Sinzibbilitati spittrali',
'exif-isospeedratings'             => 'Sinzibbilitati ISO',
'exif-oecf'                        => 'Fatturi di cunvirsioni optoalittrònica',
'exif-shutterspeedvalue'           => "Tempu d'espusizzioni",
'exif-aperturevalue'               => 'Apirtura',
'exif-brightnessvalue'             => 'Luminusitati',
'exif-exposurebiasvalue'           => 'Currezzioni espusizzioni',
'exif-maxaperturevalue'            => 'Apirtura màssima',
'exif-subjectdistance'             => 'Distanza dû suggettu',
'exif-meteringmode'                => 'Mètudu di misurazzioni',
'exif-lightsource'                 => 'Surgenti luminusa',
'exif-flash'                       => 'Carattirìstichi e statu dû flash',
'exif-focallength'                 => 'Distanza fucali obbittivu',
'exif-subjectarea'                 => 'Ària nquatranti lu suggettu',
'exif-flashenergy'                 => 'Putenza dû flash',
'exif-spatialfrequencyresponse'    => 'Risposta n friquenza spazziali',
'exif-focalplanexresolution'       => 'Risuluzzioni X supra lu chianu fucali',
'exif-focalplaneyresolution'       => 'Risuluzzioni Y supra lu chianu fucali',
'exif-focalplaneresolutionunit'    => 'Unitati di misura risuluzzioni supra lu chianu fucali',
'exif-subjectlocation'             => 'Pusizzioni dû suggettu',
'exif-exposureindex'               => 'Sinzibbilitati mpustata',
'exif-sensingmethod'               => 'Mètudu di rilivazzioni',
'exif-filesource'                  => 'Orìggini dû file',
'exif-scenetype'                   => 'Tipu di nquatratura',
'exif-cfapattern'                  => 'Dispusizzioni filtru culuri',
'exif-customrendered'              => 'Elabburazzioni pirsunalizzata',
'exif-exposuremode'                => "Mudalitati d'espusizzioni",
'exif-whitebalance'                => 'Valanzamentu dû jancu',
'exif-digitalzoomratio'            => 'Rapportu zoom diggitali',
'exif-focallengthin35mmfilm'       => 'Fucali equivalenti supra 35 mm',
'exif-scenecapturetype'            => "Tipu d'accanzu",
'exif-gaincontrol'                 => 'Cuntrollu nquatratura',
'exif-contrast'                    => 'Cuntrollu cuntrastu',
'exif-saturation'                  => 'Cuntrollu saturazzioni',
'exif-sharpness'                   => 'Cuntrollu nititizza',
'exif-devicesettingdescription'    => 'Discrizzioni mpustazzioni dispusitivu',
'exif-subjectdistancerange'        => 'Scala distanza suggettu',
'exif-imageuniqueid'               => 'ID unìvucu mmàggini',
'exif-gpsversionid'                => 'Virsioni dî tag GPS',
'exif-gpslatituderef'              => 'Latitùtini Nord o Sud',
'exif-gpslatitude'                 => 'Latitùtini',
'exif-gpslongituderef'             => 'Lungitùtini Est o Ovest',
'exif-gpslongitude'                => 'Lungitùtini',
'exif-gpsaltituderef'              => "Rifirimentu pi l'autitùtini",
'exif-gpsaltitude'                 => 'Autitùtini',
'exif-gpstimestamp'                => 'Ura GPS (ruloggiu atòmicu)',
'exif-gpssatellites'               => 'Satèlliti usati pi la misurazzioni',
'exif-gpsstatus'                   => 'Statu dû ricivituri',
'exif-gpsmeasuremode'              => 'Mudalitati di misurazzioni',
'exif-gpsdop'                      => 'Pricisioni dâ misurazzioni',
'exif-gpsspeedref'                 => 'Unitati di misura dâ vilucitati',
'exif-gpsspeed'                    => 'Vilucitati dû ricivituri GPS',
'exif-gpstrackref'                 => 'Rifirimentu pi la direzzioni movimentu',
'exif-gpstrack'                    => 'Direzzioni dû movimentu',
'exif-gpsimgdirectionref'          => 'Rifirimentu pi la direzzioni dâ mmàggini',
'exif-gpsimgdirection'             => 'Direzzioni dâ mmàggini',
'exif-gpsmapdatum'                 => 'Rilivamentu giodèticu usatu',
'exif-gpsdestlatituderef'          => 'Rifirimentu pi la latitùtini dâ distinazzioni',
'exif-gpsdestlatitude'             => 'Latitùtini dâ distinazzioni',
'exif-gpsdestlongituderef'         => 'Rifirimentu pi la lungitùtini dâ distinazzioni',
'exif-gpsdestlongitude'            => 'Lungitùtini dâ distinazzioni',
'exif-gpsdestbearingref'           => 'Rifirimentu pi la direzzioni dâ distinazzioni',
'exif-gpsdestbearing'              => 'Direzzioni dâ distinazzioni',
'exif-gpsdestdistanceref'          => 'Rifirimentu pi la distanza dâ distinazzioni',
'exif-gpsdestdistance'             => 'Distanza dâ distinazzioni',
'exif-gpsprocessingmethod'         => "Nomu dû mètudu d'elabburazzioni GPS",
'exif-gpsareainformation'          => 'Nomu dâ zona GPS',
'exif-gpsdifferential'             => 'Currezzioni diffirinziali GPS',

# EXIF attributes
'exif-compression-1' => 'Nuddu',

'exif-unknowndate' => 'Data scanusciuta',

'exif-orientation-1' => 'Nurmali', # 0th row: top; 0th column: left
'exif-orientation-2' => 'Capuvortu urizzontarmenti', # 0th row: top; 0th column: right
'exif-orientation-3' => 'Rutatu di 180°', # 0th row: bottom; 0th column: right
'exif-orientation-4' => 'Capuvortu virticarmenti', # 0th row: bottom; 0th column: left
'exif-orientation-5' => "Rotatu 90° 'n sensu antiurariu e capuvortu virticarmenti", # 0th row: left; 0th column: top
'exif-orientation-6' => "Rutatu 90° 'n senzu orariu", # 0th row: right; 0th column: top
'exif-orientation-7' => "Rotatu 90° 'n sensu urariu e capuvortu virticarmenti", # 0th row: right; 0th column: bottom
'exif-orientation-8' => "Rutatu 90° 'n senzu antiorariu", # 0th row: left; 0th column: bottom

'exif-planarconfiguration-2' => 'liniari (planar)',

'exif-xyresolution-i' => '$1 punti pi puseri (dpi)',
'exif-xyresolution-c' => '$1 punti pi cintìmitru (dpc)',

'exif-colorspace-ffff.h' => 'Nun calibbratu',

'exif-componentsconfiguration-0' => 'assenti',

'exif-exposureprogram-0' => 'Nun difinitu',
'exif-exposureprogram-1' => 'Manuali',
'exif-exposureprogram-3' => 'Priuritati a lu diaframma',
'exif-exposureprogram-4' => "Priuritati a l'espusizzioni",
'exif-exposureprogram-5' => 'Artìsticu (urientatu â prufunnitati di campu)',
'exif-exposureprogram-6' => 'Spurtivu (urientatu â vilucitati di ripresa)',
'exif-exposureprogram-7' => 'Ritrattu (suggetti vicini cu sfunnu fora focu)',
'exif-exposureprogram-8' => 'Panurama (suggetti luntani cu sfunnu a focu)',

'exif-meteringmode-0'   => 'Scanusciutu',
'exif-meteringmode-2'   => 'Media pisata cintrata',
'exif-meteringmode-5'   => 'Pattern',
'exif-meteringmode-6'   => 'Parziali',
'exif-meteringmode-255' => 'Àutru',

'exif-lightsource-0'   => 'Scanusciuta',
'exif-lightsource-1'   => 'Luci sulari',
'exif-lightsource-2'   => 'Làmpara a fluoriscenza',
'exif-lightsource-3'   => 'Làmpara a lu tungstenu (a ncanniscenza)',
'exif-lightsource-4'   => 'Flash',
'exif-lightsource-9'   => 'Bonu tempu',
'exif-lightsource-10'  => 'Nigghiusu',
'exif-lightsource-11'  => "'N ùmmira",
'exif-lightsource-17'  => 'Luci standard A',
'exif-lightsource-18'  => 'Luci standard B',
'exif-lightsource-19'  => 'Luci standard C',
'exif-lightsource-20'  => 'Alluminanti D55',
'exif-lightsource-21'  => 'Alluminanti D65',
'exif-lightsource-22'  => 'Alluminanti D75',
'exif-lightsource-23'  => 'Alluminanti D50',
'exif-lightsource-24'  => 'Làmpara di studiu ISO a lu tungstenu',
'exif-lightsource-255' => 'Àutra surgenti luminusa',

'exif-focalplaneresolutionunit-2' => 'puseri',

'exif-sensingmethod-1' => 'Nun difinitu',
'exif-sensingmethod-2' => 'Sinzuri ària culuri a 1 chip',
'exif-sensingmethod-3' => 'Sinzuri ària culuri a 2 chip',
'exif-sensingmethod-4' => 'Sinzuri ària culuri a 3 chip',
'exif-sensingmethod-5' => 'Sinzuri ària culuri siquinziali',
'exif-sensingmethod-7' => 'Sinzuri triliniari',
'exif-sensingmethod-8' => 'Sinzuri liniari culuri siquinziali',

'exif-scenetype-1' => 'Fotugrafìa diretta',

'exif-customrendered-0' => 'Prucessu nurmali',
'exif-customrendered-1' => 'Prucessu pirsunalizzatu',

'exif-exposuremode-0' => 'Espusizzioni automàtica',
'exif-exposuremode-1' => 'Espusizzioni manuali',
'exif-exposuremode-2' => 'Bracketing automàticu',

'exif-whitebalance-0' => 'Valanzamentu dû jancu automàticu',
'exif-whitebalance-1' => 'Valanzamentu dû jancu manuali',

'exif-scenecapturetype-0' => 'Standard',
'exif-scenecapturetype-1' => 'Panurama',
'exif-scenecapturetype-2' => 'Ritrattu',
'exif-scenecapturetype-3' => 'Nutturna',

'exif-gaincontrol-0' => 'Nuddu',
'exif-gaincontrol-1' => 'Ènfasi pi accanzu vasciu',
'exif-gaincontrol-2' => 'Ènfasi pi accanzu àutu',
'exif-gaincontrol-3' => 'Diènfasi pi accanzu vasciu',
'exif-gaincontrol-4' => 'Diènfasi pi accanzu àutu',

'exif-contrast-0' => 'Nurmali',
'exif-contrast-1' => 'Cuntrastu àutu',
'exif-contrast-2' => 'Cuntrastu vasciu',

'exif-saturation-0' => 'Nurmali',
'exif-saturation-1' => 'Saturazzioni vascia',
'exif-saturation-2' => 'Saturazzioni àuta',

'exif-sharpness-0' => 'Nurmali',
'exif-sharpness-1' => 'Ntitizza minuri',
'exif-sharpness-2' => 'Nititizza maiuri',

'exif-subjectdistancerange-0' => 'Scanusciuta',
'exif-subjectdistancerange-1' => 'Macru',
'exif-subjectdistancerange-2' => 'Suggettu vicinu',
'exif-subjectdistancerange-3' => 'Suggettu luntanu',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'Latitùtini Nord',
'exif-gpslatitude-s' => 'Latitùtini Sud',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'Lungitùtini Est',
'exif-gpslongitude-w' => 'Lungitùtini Ovest',

'exif-gpsstatus-a' => 'Misurazzioni n cursu',
'exif-gpsstatus-v' => 'Misurazzioni nteropiràbbili',

'exif-gpsmeasuremode-2' => 'Misurazzioni bidiminziunali',
'exif-gpsmeasuremode-3' => 'Misurazzioni tridiminziunali',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'Chilòmitri orari',
'exif-gpsspeed-m' => 'Migghia orari',
'exif-gpsspeed-n' => 'Gruppa',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => 'Direzzioni riali',
'exif-gpsdirection-m' => 'Direzzioni magnètica',

# External editor support
'edit-externally'      => 'Cancia stu file usannu un prugramma sternu',
'edit-externally-help' => "Pi maiuri nfurmazzioni cunzurtari li [http://www.mediawiki.org/wiki/Manual:External_editors istruzzioni] ('n ngrisi)",

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'tutti',
'imagelistall'     => 'tutti',
'watchlistall2'    => 'tutti',
'namespacesall'    => 'Tutti',
'monthsall'        => 'tutti',

# E-mail address confirmation
'confirmemail'            => 'Cunferma ndirizzu e-mail',
'confirmemail_noemail'    => 'Nun hà statu ndicatu un ndirizzu e-mail vàlidu ntê propi [[Special:Preferences|prifirenzi]].',
'confirmemail_text'       => "Stu situ richiedi la virìfica di l ndirizzu e-mail prima di putiri usari li funzioni cunnessi a l'email. Prèmiri lu pulsanti ccà sutta pi mannari na richiesta di cunferma a lu propiu ndirizzu; ntô missaggiu è prisenti un culligamenti ca cunteni un còdici. Visitari lu culligamentu cu lu propiu browser pi cunfirmari ca lu ndirizzu e-mail è vàlidu.",
'confirmemail_pending'    => "<div class=\"error\"> Lu còdici di cunferma hà già statu spiditu via posta alittrònica; siddu l'account hà statu criatu di ricenti, si preja d'attènniri l'arrivu dû còdici pi quarchi minutu prima di tintari d'addumannàrinni unu novu. </div>",
'confirmemail_send'       => 'Manna un còdici di cunferma via e-mail.',
'confirmemail_sent'       => 'Missaggiu e-mail di cunferma mannatu.',
'confirmemail_oncreate'   => 'Un còdici di cunferma hà statu spiditu a lu ndirizzu di posta alittrònica ndicatu. Lu còdici nun è nicissariu pi tràsiri lu situ, ma è nicissariu furnirilu pi putiri abbilitari tutti li funzioni dû situ ca fannu usu dâ posta alittrònica.',
'confirmemail_sendfailed' => "{{SITENAME}} nun pò mannari lu missaggiu e-mail di cunferma. Virificari ca lu nnirizzu nun cunteni caràttiri nun vàlidi.

Missaggiu d'erruri dû mailer: $1",
'confirmemail_invalid'    => 'Còdici di cunferma nun vàlidu. Lu còdici putissi èssiri scadutu.',
'confirmemail_needlogin'  => 'È nicissariu $1 pi cunfirmari lu propiu ndirizzu e-mail.',
'confirmemail_success'    => "Lu ndirizzu e-mail è cunfirmatu. Ora è pussìbbili esèquiri l'accessu e fari chinu usu dû situ.",
'confirmemail_loggedin'   => 'Lu tò nnirizzu email fu ora cunfirmatu.',
'confirmemail_error'      => 'Erruri ntô sarvataggiu dâ cunferma.',
'confirmemail_subject'    => '{{SITENAME}}: richiesta di cunferma di lu ndirizzu',
'confirmemail_body'       => 'Quarcunu, prubbabbirmenti tu stissu di lu ndirizzu IP $1, hà riggistratu l\'account "$2" supra {{SITENAME}} ndicannu stu ndirizzu e-mail. 

Pi cunfirmari ca l\'account t\'apparteni e attivari li funzioni rilativi a lu nvìu di e-mail supra {{SITENAME}}, grapi lu culligamentu siquenti cu lu tò browser: 

$3 

Siddu l\'account *nun* t\'apparteni, grapi lu siguenti culligamentu:

$5

Stu còdici di cunferma scadi automaticamenti a li $4.',

# Scary transclusion
'scarytranscludedisabled' => '[La nchiusioni di pàggini tra siti wiki nun è attiva]',
'scarytranscludefailed'   => '[Erruri: Mpussìbbili uttèniri lu template $1]',
'scarytranscludetoolong'  => '[URL troppu longu]',

# Trackbacks
'trackbackbox'      => "<div id='mw_trackbacks'> Trackback pi sta pàggina:<br /> $1 </div>",
'trackbackremove'   => '([$1 Elìmina])',
'trackbackdeleteok' => 'Nfurmazzioni di trackback eliminati currettamenti.',

# Delete conflict
'deletedwhileediting' => "'''Accura''': Sta pàggina hà statu cancillata doppu c'hai accuminzatu a canciàrila!",
'confirmrecreate'     => "L'utenti [[User:$1|$1]] ([[User talk:$1|discussioni]]) hà cancillatu sta pàggina doppu ca hai nizziatu a canciàrila, pi lu siquenti mutivu: ''$2'' Pi favuri, cunferma ca addisìi veramenti criari n'àutra vota sta pàggina.",

# HTML dump
'redirectingto' => 'Rinnirizzamentu a [[:$1]]...',

# action=purge
'confirm_purge'        => "S'addisìa puliri la cache di sta pàggina? $1",
'confirm_purge_button' => 'Cunferma',

# AJAX search
'searchcontaining' => "Circata di l'artìculi ca cuntèninu ''$1''.",
'searchnamed'      => "Circata d'artìculi ca si chiàmanu ''$1''.",
'articletitles'    => "Ricerca di l'artìculi ca accumènzanu cu ''$1''",
'hideresults'      => 'Ammuccia li risurtati',

# Multipage image navigation
'imgmultipageprev' => '← pàggina pricidenti',
'imgmultipagenext' => 'pàggina siquenti →',
'imgmultigo'       => "Va'",
'imgmultigoto'     => 'Vai a pàggina $1',

# Table pager
'ascending_abbrev'         => 'crisc',
'descending_abbrev'        => 'dicrisc',
'table_pager_next'         => 'Pàggina succissiva',
'table_pager_prev'         => 'Pàggina pricidenti',
'table_pager_first'        => 'Prima pàggina',
'table_pager_last'         => 'Ùrtima pàggina',
'table_pager_limit'        => 'Ammustra $1 file pi pàggina',
'table_pager_limit_submit' => "Va'",
'table_pager_empty'        => 'Nuddu risurtatu',

# Auto-summaries
'autosumm-blank'   => 'Cuntinutu cancillatu',
'autosumm-replace' => "Pàggina sustituita cu '$1'",
'autoredircomment' => 'Rinnirizzamentu â pàggina [[$1]]',
'autosumm-new'     => 'Pàggina nova: $1',

# Live preview
'livepreview-loading' => "Carricamentu 'n cursu...",
'livepreview-ready'   => 'Carricamentu n cursu… Prontu.',
'livepreview-failed'  => "Erruri ntâ funzioni Live preview. Usari l'antiprima standard.",
'livepreview-error'   => 'Mpussìbbili effittuari lu culligamentu: $1 "$2" Usari l\'antiprima standard.',

# Friendlier slave lag warnings
'lag-warn-normal' => "Li canciamenti appurtati {{PLURAL:$1|nta l'ùrtimu secundu|nta l'ùrtimi $1 secundi}} ponnu nun èssiri nta sta lista.",
'lag-warn-high'   => "A càusa di nu ritardu eccissivu nta l'aggiurnamentu dô server di databbasi, li canciamenti appurtati {{PLURAL:$1|nta l'ùrtimu secundu|nta l'ùrtimi $1 secundi}} ponnu nun èssiri nta sta lista.",

# Watchlist editor
'watchlistedit-numitems'       => 'La lista dê pàggini taliati cunteni {{PLURAL:$1|na pàggina (cu la rispettiva pàggina di discussioni)|$1 pàggini (cu li rispettivi pàggini di discussioni)}}.',
'watchlistedit-noitems'        => 'La lista dê pàggini taliati è vacanti.',
'watchlistedit-normal-title'   => 'Cancia pàggini taliati',
'watchlistedit-normal-legend'  => 'Eliminazzioni di pàggini dâ lista dê pàggini taliati',
'watchlistedit-normal-explain' => "Ccà sutta sugnu alincati tutti li pàggine taliati. Pi eliminari una o cchiù pàggini dâ lista, silizziunari li casiddi accantu e fari clic supra lu buttuni 'Elìmina pàggini' 'n fundu all'alencu. Accura ca è puru possibbili [[Special:Watchlist/raw|canciari la lista 'n forma testuali]].",
'watchlistedit-normal-submit'  => 'Elìmina pàggini',
'watchlistedit-normal-done'    => 'Dâ lista dê pàggini taliati hà{{PLURAL:$1|&nbsp;stata eliminata na pàggina|nnu stati eliminati $1 pàggini}}:',
'watchlistedit-raw-title'      => "Cancia li pàggini taliati 'n forma testuali",
'watchlistedit-raw-legend'     => 'Canciamentu testuali pàggini taliati',
'watchlistedit-raw-explain'    => "Ccà sutta sugnu alincati tutti li pàggine taliati. Pi canciari la lista agghiunciri o rimòviri li rispettivi tituli, unu pi riga. Quannu funisci, fà clic supra 'Aggiorna la lista' 'n fundu all'alencu. Accura ca è puru possibbili [[Special:Watchlist/edit|canciari la lista câ 'nterfaccia standard]].",
'watchlistedit-raw-titles'     => 'Pàggini:',
'watchlistedit-raw-done'       => 'La tò lista dê pàggini taliati hà stata aggiornata.',
'watchlistedit-raw-added'      => 'Hà{{PLURAL:$1|&nbsp;stata agghiunciuta na pàggina|nnu stati agghiunciuti $1 pàggini}}:',
'watchlistedit-raw-removed'    => 'Hà{{PLURAL:$1|&nbsp;stata eliminata na pàggina|nnu stati eliminati $1 pàggini}}:',

# Watchlist editing tools
'watchlisttools-view' => 'Vidi li canciamenti rilivanti',
'watchlisttools-edit' => 'Vidi e cancia la lista',
'watchlisttools-raw'  => "Cancia la lista 'n forma testuali",

# Special:Version
'version'                  => 'virsioni', # Not used as normal message but as header for the special page itself
'version-specialpages'     => 'Pàggini spiciali',
'version-variables'        => 'Variabili',
'version-license'          => 'Licenza',
'version-software'         => 'Software nstallatu',
'version-software-product' => 'Prodottu',
'version-software-version' => 'Virsioni',

# Special:FilePath
'filepath'        => 'Pircorsu di nu file',
'filepath-page'   => 'Nnomu dô file:',
'filepath-submit' => 'Pircorsu',

# Special:SpecialPages
'specialpages'                   => 'Pàggini spiciali',
'specialpages-group-maintenance' => 'Resocunti di manutinzioni',
'specialpages-group-other'       => 'Autri pàggini spiciali',
'specialpages-group-login'       => 'Trasi / riggìstrazzioni',
'specialpages-group-changes'     => 'Ùrtimi canciamenti e riggistri',
'specialpages-group-users'       => 'Utenti e diritti',
'specialpages-group-highuse'     => 'Pàggini cchiù usati',

);
