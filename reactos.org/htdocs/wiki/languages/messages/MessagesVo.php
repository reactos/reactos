<?php
/** Volapük (Volapük)
 *
 * @ingroup Language
 * @file
 *
 * @author Malafaya
 * @author Smeira
 * @author לערי ריינהארט
 */

$namespaceNames = array(
	NS_MEDIA          => 'Nünamakanäd',
	NS_SPECIAL        => 'Patikos',
	NS_MAIN           => '',
	NS_TALK           => 'Bespik',
	NS_USER           => 'Geban',
	NS_USER_TALK      => 'Gebanibespik',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK   => 'Bespik_dö_$1',
	NS_IMAGE          => 'Magod',
	NS_IMAGE_TALK     => 'Magodibespik',
	NS_MEDIAWIKI      => 'Sitanuns',
	NS_MEDIAWIKI_TALK => 'Bespik_dö_sitanuns',
	NS_TEMPLATE       => 'Samafomot',
	NS_TEMPLATE_TALK  => 'Samafomotibespik',
	NS_HELP           => 'Yuf',
	NS_HELP_TALK      => 'Yufibespik',
	NS_CATEGORY       => 'Klad',
	NS_CATEGORY_TALK  => 'Kladibespik',
);

$datePreferences = array(
	'default',
	'vo',
	'vo plain',
	'ISO 8601',
);

$defaultDateFormat = 'vo';

$dateFormats = array(
	'vo time' => 'H:i',
	'vo date' => 'Y F j"id"',
	'vo both' => 'H:i, Y F j"id"',

	'vo plain time' => 'H:i',
	'vo plain date' => 'Y F j',
	'vo plain both' => 'H:i, Y F j',
);

$specialPageAliases = array(
	'DoubleRedirects'           => array( 'Lüodükömstelik', 'Lüodüköms_telik' ),
	'BrokenRedirects'           => array( 'Lüodükömsdädik', 'Lüodüköms_dädik' ),
	'Disambiguations'           => array( 'Telplänovs' ),
	'Userlogin'                 => array( 'Gebananunäd' ),
	'Userlogout'                => array( 'Gebanasenunäd' ),
	'Preferences'               => array( 'Buükams' ),
	'Watchlist'                 => array( 'Galädalised' ),
	'Recentchanges'             => array( 'Votükamsnulik' ),
	'Upload'                    => array( 'Löpükön' ),
	'Imagelist'                 => array( 'Magodalised' ),
	'Newimages'                 => array( 'Magodsnulik', 'Magods_nulik' ),
	'Listusers'                 => array( 'Gebanalised' ),
	'Statistics'                => array( 'Statits' ),
	'Randompage'                => array( 'Padfädik', 'Pad_fädik', 'Fädik' ),
	'Lonelypages'               => array( 'Padssoelöl', 'Pads_soelöl' ),
	'Uncategorizedpages'        => array( 'Padsnenklads', 'Pads_nen_klads' ),
	'Uncategorizedcategories'   => array( 'Kladsnenklads', 'Klads_nen_klads' ),
	'Uncategorizedimages'       => array( 'Magodsnenklads', 'Magods_nen_klads' ),
	'Uncategorizedtemplates'    => array( 'Samafomotsnenklads', 'Samafomots_nen_klads' ),
	'Unusedcategories'          => array( 'Kladsnopageböls', 'Klad_no_pageböls' ),
	'Unusedimages'              => array( 'Magodsnopageböls', 'Magods_no_pageböls' ),
	'Wantedpages'               => array( 'Padspavilöl', 'Yümsdädik', 'Pads_pavilöl', 'Yüms_dädik' ),
	'Wantedcategories'          => array( 'Kladspavilöl', 'Klads_pavilöl' ),
	'Mostlinked'                => array( 'Suvüno_peyümöls' ),
	'Mostlinkedcategories'      => array( 'Klads_suvüno_peyümöls' ),
	'Mostlinkedtemplates'       => array( 'Samafomots_suvüno_peyümöls' ),
	'Shortpages'                => array( 'Padsbrefik' ),
	'Longpages'                 => array( 'Padslunik' ),
	'Newpages'                  => array( 'Padsnulik' ),
	'Ancientpages'              => array( 'Padsbäldik' ),
	'Protectedpages'            => array( 'Padspejelöl' ),
	'Protectedtitles'           => array( 'Tiädspejelöl' ),
	'Allpages'                  => array( 'Padsvalik' ),
	'Specialpages'              => array( 'Padspatik' ),
	'Contributions'             => array( 'Keblünots' ),
	'Confirmemail'              => array( 'Fümedönladeti' ),
	'Whatlinkshere'             => array( 'Yümsisio', 'Isio' ),
	'Movepage'                  => array( 'Topätükön' ),
	'Categories'                => array( 'Klads' ),
	'Version'                   => array( 'Fomam' ),
	'Allmessages'               => array( 'Nünsvalik' ),
	'Log'                       => array( 'Jenotalised', 'Jenotaliseds' ),
	'Mypage'                    => array( 'Padobik' ),
	'Mytalk'                    => array( 'Bespikobik' ),
	'Mycontributions'           => array( 'Keblünotsobik' ),
	'Search'                    => array( 'Suk' ),
);

$messages = array(
# User preference toggles
'tog-underline'               => 'Dislienükolöd yümis:',
'tog-highlightbroken'         => 'Jonön yümis dädik <a href="" class="new">ön mod at</a> (voto: ön mod at<a href="" class="internal">?</a>).',
'tog-justify'                 => 'Lonedükön bagafis',
'tog-hideminor'               => 'Klänedön redakamis pülik su lised votükamas nulik',
'tog-extendwatchlist'         => 'Stäänükön galädalisedi ad jonön votükamis tefik valik',
'tog-usenewrc'                => 'Lised pamenodöl votükamas nulik (JavaScript)',
'tog-numberheadings'          => 'Givön itjäfidiko nümis dilädatiädes',
'tog-showtoolbar'             => 'Jonön redakastumemi (JavaScript)',
'tog-editondblclick'          => 'Dälön redakön padis pö drän telik mugaknopa (JavaScript)',
'tog-editsection'             => 'Dälön redakami dilädas me yüms: [redakön]',
'tog-editsectiononrightclick' => 'Dälön redakami diläda me klik mugaknopa detik su dilädatiäds (JavaScript)',
'tog-showtoc'                 => 'Jonön ninädalisedi (su pads labü diläds plu 3)',
'tog-rememberpassword'        => 'Dakipön nunädamanünis obik in nünöm at',
'tog-editwidth'               => 'Redakaspad labon vidoti lölöfik',
'tog-watchcreations'          => 'Läükolöd padis fa ob pejafölis lä galädalised obik',
'tog-watchdefault'            => 'Läükolöd padis fa ob peredakölis la galädalised obik',
'tog-watchmoves'              => 'Läükolöd padis fa ob petopätükölis lä galädalised obik',
'tog-watchdeletion'           => 'Läükolöd padis fa ob pemoükölis lä galädalised obik',
'tog-minordefault'            => 'Lelogolöd redakamis no pebepenölis valikis asä pülikis',
'tog-previewontop'            => 'Jonolöd büologedi bü redakaspad',
'tog-previewonfirst'          => 'Jonolöd büologedi pö redakam balid',
'tog-nocache'                 => 'Nejäfidükön el caché padas',
'tog-enotifwatchlistpages'    => 'Sedolös obe penedi leäktronik ven ek votükon padi se galädalised obik',
'tog-enotifusertalkpages'     => 'Sedolös obe penedi leäktronik ven gebanapad obik pavotükon',
'tog-enotifminoredits'        => 'Sedolös obe penedi leäktronik igo pö padavotükams pülik',
'tog-enotifrevealaddr'        => 'Jonön ladeti leäktronik oba in nunapeneds.',
'tog-shownumberswatching'     => 'Jonön numi gebanas galädöl',
'tog-fancysig'                => 'Dispenäd balugik (nen yüms lü gebanapad)',
'tog-externaleditor'          => 'Gebön nomiko redakömi plödik',
'tog-externaldiff'            => 'Gebön nomiko difi plödik',
'tog-showjumplinks'           => 'Dälolöd lügolovi me yüms "lübunöl"',
'tog-uselivepreview'          => 'Gebön büologedi itjäfidik (JavaScript) (Sperimäntik)',
'tog-forceeditsummary'        => 'Sagolös obe, ven redakaplän brefik vagon',
'tog-watchlisthideown'        => 'No jonolöd redakamis obik in galädalised',
'tog-watchlisthidebots'       => 'No jonolöd redakamis mäikamenas in galädalised',
'tog-watchlisthideminor'      => 'Klänolöd redakamis pülik se galädalised',
'tog-ccmeonemails'            => 'Sedolös obe kopiedis penedas, kelis sedob gebanes votik',
'tog-diffonly'                => 'No jonön padaninädi dis difs',
'tog-showhiddencats'          => 'Jonön kladis peklänedöl',

'underline-always'  => 'Pö jenets valik',
'underline-never'   => 'Neföro',
'underline-default' => 'Ma bevüresodatävöm',

'skinpreview' => '(Büologed)',

# Dates
'sunday'        => 'sudel',
'monday'        => 'mudel',
'tuesday'       => 'tudel',
'wednesday'     => 'vedel',
'thursday'      => 'dödel',
'friday'        => 'fridel',
'saturday'      => 'zädel',
'sun'           => 'sud',
'mon'           => 'mud',
'tue'           => 'tud',
'wed'           => 'ved',
'thu'           => 'död',
'fri'           => 'fri',
'sat'           => 'zäd',
'january'       => 'yanul',
'february'      => 'febul',
'march'         => 'mäzul',
'april'         => 'prilul',
'may_long'      => 'mayul',
'june'          => 'yunul',
'july'          => 'yulul',
'august'        => 'gustul',
'september'     => 'setul',
'october'       => 'tobul',
'november'      => 'novul',
'december'      => 'dekul',
'january-gen'   => 'yanul',
'february-gen'  => 'febul',
'march-gen'     => 'mäzul',
'april-gen'     => 'prilul',
'may-gen'       => 'mayul',
'june-gen'      => 'yunul',
'july-gen'      => 'yulul',
'august-gen'    => 'gustul',
'september-gen' => 'setul',
'october-gen'   => 'tobul',
'november-gen'  => 'novul',
'december-gen'  => 'dekul',
'jan'           => 'yan',
'feb'           => 'feb',
'mar'           => 'mäz',
'apr'           => 'pri',
'may'           => 'may',
'jun'           => 'yun',
'jul'           => 'yul',
'aug'           => 'gus',
'sep'           => 'set',
'oct'           => 'tob',
'nov'           => 'nov',
'dec'           => 'dek',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|Klad|Klads}}',
'category_header'                => 'Pads in klad: „$1“',
'subcategories'                  => 'Donaklads',
'category-media-header'          => 'Media in klad: "$1"',
'category-empty'                 => "''Klad at anu ninädon padis e ragivis nonikis.''",
'hidden-categories'              => '{{PLURAL:$1|Klad|Klads}} peklänedöl',
'hidden-category-category'       => 'Klads peklänedöl', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|Klad at labon te donakladi sököl.|Klad at labon {{PLURAL:$1|donakladi sököl|donakladis sököl $1}}, se $2.}}',
'category-subcat-count-limited'  => 'Klad at labon {{PLURAL:$1|donakladi|donakladis}} sököl.',
'category-article-count'         => '{{PLURAL:$2|Klad at labon te padi sököl.|{{PLURAL:$1|Pad sököl binon|Pads sököl $1 binons}} in klad at, se $2.}}',
'category-article-count-limited' => '{{PLURAL:$1|Pad sököl binon|Pads sököl $1 binons}} in klad at.',
'category-file-count'            => '{{PLURAL:$2|Klad at labon te ragivi sököl.|{{PLURAL:$1|Ragiv sököl binon |Ragivs sököl $1 binons}} in klad at, se $2.}}',
'category-file-count-limited'    => '{{PLURAL:$1|Ragiv sököl binon|Ragivs sököl $1 binons}} in klad at.',
'listingcontinuesabbrev'         => '(fov.)',

'mainpagetext'      => "<big>'''El MediaWiki pestiton benosekiko.'''</big>",
'mainpagedocfooter' => 'Konsultolös [http://meta.wikimedia.org/wiki/Help:Contents Gebanageidian] ad tuvön nünis dö geb programema vükik.

== Nüdugot ==

* [http://www.mediawiki.org/wiki/Manual:Configuration_settings Parametalised]
* [http://www.mediawiki.org/wiki/Manual:FAQ MediaWiki: SSP]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce Potalised tefü fomams nulik ela MediaWiki]',

'about'          => 'Tefü',
'article'        => 'Ninädapad',
'newwindow'      => '(maifikon in fenät nulik)',
'cancel'         => 'Stöpädön',
'qbfind'         => 'Tuvön',
'qbbrowse'       => 'Padön',
'qbedit'         => 'Redakön',
'qbpageoptions'  => 'Pad at',
'qbpageinfo'     => 'Yumed',
'qbmyoptions'    => 'Pads obik',
'qbspecialpages' => 'Pads patik',
'moredotdotdot'  => 'Plu...',
'mypage'         => 'Pad obik',
'mytalk'         => 'Bespiks obik',
'anontalk'       => 'Bespiks ela IP at',
'navigation'     => 'Nafam',
'and'            => 'e',

# Metadata in edit box
'metadata_help' => 'Metanünods:',

'errorpagetitle'    => 'Pöl',
'returnto'          => 'Geikön lü $1.',
'tagline'           => 'Se {{SITENAME}}',
'help'              => 'Yuf',
'search'            => 'Suk',
'searchbutton'      => 'Sukolöd',
'go'                => 'Gololöd',
'searcharticle'     => 'Maifükön padi',
'history'           => 'Padajenotem',
'history_short'     => 'Jenotem',
'updatedmarker'     => 'pävotükon pos visit lätik oba',
'info_short'        => 'Nün',
'printableversion'  => 'Fom dabükovik',
'permalink'         => 'Yüm laidüpik',
'print'             => 'Bükön',
'edit'              => 'Redakön',
'create'            => 'Jafön',
'editthispage'      => 'Redakolöd padi at',
'create-this-page'  => 'Jafön padi at',
'delete'            => 'Moükön',
'deletethispage'    => 'Moükolös padi at',
'undelete_short'    => 'Sädunön moükami {{PLURAL:$1|redakama bal|redakamas $1}}',
'protect'           => 'Jelön',
'protect_change'    => 'votükön',
'protectthispage'   => 'Jelön padi at',
'unprotect'         => 'säjelön',
'unprotectthispage' => 'Säjelolöd padi at',
'newpage'           => 'Pad nulik',
'talkpage'          => 'Bespikolöd padi at',
'talkpagelinktext'  => 'Bespik',
'specialpage'       => 'Pad patik',
'personaltools'     => 'Stums pösodik',
'postcomment'       => 'Sedön küpeti',
'articlepage'       => 'Jonön ninädapadi',
'talk'              => 'Bespik',
'views'             => 'Logams',
'toolbox'           => 'Stumem',
'userpage'          => 'Logön gebanapadi',
'projectpage'       => 'Logön proyegapadi',
'imagepage'         => 'Jonön magodapad',
'mediawikipage'     => 'Logön nunapadi',
'templatepage'      => 'Logön samafomotapadi',
'viewhelppage'      => 'Jonön yufapadi',
'categorypage'      => 'Jonolöd kladapadi',
'viewtalkpage'      => 'Logön bespikami',
'otherlanguages'    => 'In püks votik',
'redirectedfrom'    => '(Pelüodükon de pad: $1)',
'redirectpagesub'   => 'Lüodükömapad',
'lastmodifiedat'    => 'Pad at pävotükon lätiküno tü düp $2, ün $1.', # $1 date, $2 time
'viewcount'         => 'Pad at pelogon {{PLURAL:$1|balna|$1na}}.',
'protectedpage'     => 'Pad pejelöl',
'jumpto'            => 'Bunön lü:',
'jumptonavigation'  => 'nafam',
'jumptosearch'      => 'suk',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'Tefü {{SITENAME}}',
'aboutpage'            => 'Project:Tefü',
'bugreports'           => 'Nunods dö programapöks',
'bugreportspage'       => 'Project:Nunods dö programapöks',
'copyright'            => 'Ninäd gebidon ma el $1.',
'copyrightpagename'    => 'Kopiedagität {{SITENAME}}a',
'copyrightpage'        => '{{ns:project}}:Kopiedagitäts',
'currentevents'        => 'Jenots nuik',
'currentevents-url'    => 'Project:Jenots nuik',
'disclaimers'          => 'Nuneds',
'disclaimerpage'       => 'Project:Gididimiedükam valemik',
'edithelp'             => 'Redakamayuf',
'edithelppage'         => 'Help:Redakam',
'faq'                  => 'Säks suvo pasäköls',
'faqpage'              => 'Project:FAQ',
'helppage'             => 'Help:Ninäd',
'mainpage'             => 'Cifapad',
'mainpage-description' => 'Cifapad',
'policy-url'           => 'Project:Dunamod',
'portal'               => 'Komotanefaleyan',
'portal-url'           => 'Project:Komotanefaleyan',
'privacy'              => 'Dunamod demü soelöf',
'privacypage'          => 'Project:Dunamod_demü_soelöf',

'badaccess'        => 'Dälapöl',
'badaccess-group0' => 'No pedälol ad ledunön atosi, kelosi ebegol.',
'badaccess-group1' => 'Dun, keli eflagol, padälon te gebanes grupa: $1.',
'badaccess-group2' => 'Dun fa ol pebegöl pemiedükon ad gebans grupas $1.',
'badaccess-groups' => 'Utos, kelosi vilol dunön, padälon te gebanes dutöl lü bal grupas: $1.',

'versionrequired'     => 'Fomam: $1 ela MediaWiki paflagon',
'versionrequiredtext' => 'Fomam: $1 ela MediaWiki zesüdon ad gebön padi at. Logolös [[Special:Version|fomamapadi]].',

'ok'                      => 'Si!',
'retrievedfrom'           => 'Pekopiedon se "$1"',
'youhavenewmessages'      => 'Su pad ola binons $1 ($2).',
'newmessageslink'         => 'nuns nulik',
'newmessagesdifflink'     => 'votükam lätik',
'youhavenewmessagesmulti' => 'Labol nunis nulik su $1',
'editsection'             => 'redakön',
'editold'                 => 'redakön',
'viewsourceold'           => 'logön fonätavödemi',
'editsectionhint'         => 'Redakolöd dilädi: $1',
'toc'                     => 'Ninäd',
'showtoc'                 => 'jonolöd',
'hidetoc'                 => 'klänedolöd',
'thisisdeleted'           => 'Jonön u sädunön moükami $1?',
'viewdeleted'             => 'Logön eli $1?',
'restorelink'             => '{{PLURAL:$1|redakama bal|redakamas $1}}',
'feedlinks'               => 'Kanad:',
'feed-invalid'            => 'Kanadabonedam no lonöfon.',
'feed-unavailable'        => 'Nünamakanads no gebidons in {{SITENAME}}',
'site-rss-feed'           => 'Kanad (RSS): $1',
'site-atom-feed'          => 'Kanad (Atom): $1',
'page-rss-feed'           => 'Kanad (RSS): "$1"',
'page-atom-feed'          => 'Kanad (Atom) „$1“',
'red-link-title'          => '$1 (no nog pepenon)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Yeged',
'nstab-user'      => 'Gebanapad',
'nstab-media'     => 'Nünamakanädapad',
'nstab-special'   => 'Patik',
'nstab-project'   => 'Proyegapad',
'nstab-image'     => 'Ragiv',
'nstab-mediawiki' => 'Vödem',
'nstab-template'  => 'Samafomot',
'nstab-help'      => 'Yufapad',
'nstab-category'  => 'Klad',

# Main script and global functions
'nosuchaction'      => 'Atos no mögon',
'nosuchactiontext'  => 'Dun peflagöl fa el URL no sevädon vüke',
'nosuchspecialpage' => 'Pad patik at no dabinon',
'nospecialpagetext' => 'Esukol padi patik no dabinöli. Lised padas patik dabinöl binon su pad: [[Special:SpecialPages]].',

# General errors
'error'                => 'Pöl',
'databaseerror'        => 'Pöl in nünodem',
'dberrortext'          => 'Süntagapök pö geb vüka at ejenon.
Atos ba sinifön, das dabinon säkäd pö program.
Steifül lätik ad gebön vüki äbinon:
<blockquote><tt>$1</tt></blockquote>
se dunod: "<tt>$2</tt>".
El MySQL ägesedon pökanuni: "<tt>$3: $4</tt>".',
'dberrortextcl'        => 'Süntagapök pö geb vüka at ejenon.
Steifül lätik ad gebön vüki at äbinon:
"$1"
se dunod: "$2".
El MySQL ägesedon pökanuni: "$3: $4"',
'noconnect'            => 'Liedo vük at labon anu fikulis kaenik e no fägon ad kosädön ko zänodanünöm nünodema. <br />
$1',
'nodb'                 => 'No eplöpos ad välön nünodemi: $1',
'cachederror'          => 'Sökölos binon kopied pasetik pada pevipöl. Mögos, das no binon fomam lätikün.',
'laggedslavemode'      => 'Nuned: pad ba labon votükamis brefabüik',
'readonly'             => 'Vük pefärmükon',
'enterlockreason'      => 'Penolös kodi löka, keninükamü täxet dula onik e dela, kü pomoükon',
'readonlytext'         => 'Vük pefärmükon: yegeds e votükams nuliks no kanons padakipön. Atos ejenon bo pro kosididaduns, pos kels vük ogeikon ad stad kösömik.

Guvan, kel äfärmükon vüki, äplänon osi ön mod sököl: $1',
'missingarticle-rev'   => '(fomamanüm: $1)',
'missingarticle-diff'  => '(Dif: $1, $2)',
'readonly_lag'         => 'Vük pefärmükon itjäfidiko du dünanünöms slafik kosädons ko mastanünöm.',
'internalerror'        => 'Pöl ninik',
'internalerror_info'   => 'Pöl ninik: $1',
'filecopyerror'        => 'No emögos ad kopiedön ragivi "$1" ad "$2".',
'filerenameerror'      => 'No eplöpos ad votanemön ragivi: "$1" ad: "$2".',
'filedeleteerror'      => 'No emögos ad moükön ragivi "$1".',
'directorycreateerror' => 'No eplöpos ad jafön ragiviäri: "$1".',
'filenotfound'         => 'No eplöpos ad tuvön ragivi: "$1".',
'fileexistserror'      => 'No eplöpos ad dakipön ragivi: "$1": ragiv ya dabinon',
'unexpected'           => 'Völad no pespetöl: „$1“=„$2“.',
'formerror'            => 'PÖL: no emögos ad bevobön fometi at.',
'badarticleerror'      => 'Dun at no kanon paledunön su pad at.',
'cannotdelete'         => 'No emögos ad moükön padi/ragivi pevälöl. (Ba ya pemoükon fa geban votik.)',
'badtitle'             => 'Tiäd badik',
'badtitletext'         => 'Padatiäd peflagöl äbinon nelonöfik, vägik, u ba yüm bevüpükik u bevüvükik dädik. Mögos, das ninädon malati(s), kel(s) no dalon(s) pagebön ad jafön tiädis.',
'perfdisabled'         => 'Dun at penemögükon nelaidüpo bi nevifükon vüki so vemo, das nek kanon gebön oni.',
'perfcached'           => 'Nüns sököl ekömons se el caché e ba no binons anuik.',
'perfcachedts'         => 'Nüns sököl kömons se mem nelaidüpik e päbevobons lätiküno ün: $1.',
'querypage-no-updates' => 'Atimükam pada at penemögükon. Nünods isik no poflifedükons suno.',
'wrong_wfQuery_params' => 'Paramets neverätik lü wfQuery()<br />
Dun: $1<br />
Beg: $2',
'viewsource'           => 'Logön fonäti',
'viewsourcefor'        => 'tefü $1',
'actionthrottled'      => 'Dun pemiedükon',
'actionthrottledtext'  => 'Ad tadunön reklamami itjäfidik (el „spam“), dunot at no padälon tu suvo dü brefüp. Ya erivol miedi gretikün. Steifülolös nogna pos minuts anik.',
'protectedpagetext'    => 'Pad at pejelon ad neletön redakami.',
'viewsourcetext'       => 'Kanol logön e kopiedön fonätakoti pada at:',
'protectedinterface'   => 'Pad at jafon vödemis sitanünas, ed anu pelökofärmükon ad vitön migebis.',
'editinginterface'     => "'''Nuned:''' Anu redakol padi, kel labükon vödemi bevüik pro programem. Votükams pada at oflunons logoti gebanasita pro gebans votik.",
'sqlhidden'            => '(SQL beg peklänedon)',
'cascadeprotected'     => 'Pad at pejelon ta redakam, bi pakeninükon fa {{PLURAL:$1|pad|pads}} sököl, kels pejelons ma „jänajel“: $2',
'namespaceprotected'   => "No dalol redakön padis in nemaspad: '''$1'''.",
'customcssjsprotected' => 'No dalol redakön padi at, bi keninükon parametis pösodik gebana votik.',
'ns-specialprotected'  => 'Pads patik no kanons paredakön.',
'titleprotected'       => "Jaf tiäda at penemögükon fa geban: [[User:$1|$1]].
Kod binon: ''$2''.",

# Login and logout pages
'logouttitle'                => 'Senunädön oki',
'logouttext'                 => '<strong>Esenunädol oli.</strong><br />
Kanol laigebön {{SITENAME}} nennemiko, u kanol nunädön oli dönu me gebananem votik. Küpälolös, das pads anik ba nog pojenons äsva no esenunädol oli, jüs uklinükol memi no laidüpik bevüresodanaföma olik.',
'welcomecreation'            => '== Benokömö, o $1! ==

Kal olik pejafon. No glömolöd ad votükön buükamis olik in {{SITENAME}}.',
'loginpagetitle'             => 'Nunädön oki',
'yourname'                   => 'Gebananem',
'yourpassword'               => 'Letavöd',
'yourpasswordagain'          => 'Klavolös dönu letavödi',
'remembermypassword'         => 'Dakipolöd ninädamanünis obik in nünöm at',
'yourdomainname'             => 'Domen olik:',
'externaldberror'            => 'U ejenon fümükamapöl plödik nünödema, u no dalol atimükön kali plödik ola.',
'loginproblem'               => '<b>No eplöpos ad nunädön oli.</b><br />Steifülolös dönu!',
'login'                      => 'Nunädolös obi',
'nav-login-createaccount'    => 'Nunädön oki / jafön kali',
'loginprompt'                => 'Mutol mögükön „kekilis“ ad kanön nunädön oli in {{SITENAME}}.',
'userlogin'                  => 'Nunädön oki / jafön kali',
'logout'                     => 'Senunädön oki',
'userlogout'                 => 'Senunädön oki',
'notloggedin'                => 'No enunädol oli',
'nologin'                    => 'No labol-li kali? $1.',
'nologinlink'                => 'Jafolös bali',
'createaccount'              => 'Jafön kali',
'gotaccount'                 => 'Ya labol-li kali? $1.',
'gotaccountlink'             => 'Nunädolös obi',
'createaccountmail'          => 'me pot leäktronik',
'badretype'                  => 'Letavöds fa ol pepenöls no leigons.',
'userexists'                 => 'Gebananem at ya dabinon. Välolös, begö! nemik votik.',
'youremail'                  => 'Ladet leäktronik *:',
'username'                   => 'Gebananem:',
'uid'                        => 'Gebanadientif:',
'prefs-memberingroups'       => 'Liman {{PLURAL:$1|grupa|grupas}}:',
'yourrealname'               => 'Nem jenöfik *:',
'yourlanguage'               => 'Pük:',
'yournick'                   => 'Länem:',
'badsig'                     => 'Dispenäd no lonöföl: dönulogolös eli HTML.',
'badsiglength'               => 'Länem binon tu lunik.
Muton labön {{PLURAL:$1|malati|malatis}} läs $1.',
'email'                      => 'Ladet leäktronik',
'prefs-help-realname'        => 'Nem jenöfik no binon zesüdik. If vilol givön oni, pogebon ad dasevön vobi olik.',
'loginerror'                 => 'Nunädamapöl',
'prefs-help-email'           => '* Ladet leäktronik (if vilol): dälon votikanes ad kosikön ko ol
yufü gebanapad u gebanabespikapad olik nes sävilupol dientifi olik.',
'prefs-help-email-required'  => 'Ladet leäktronik paflagon.',
'nocookiesnew'               => 'Gebanakal pejafon, ab no enunädol oli. {{SITENAME}} gebon „kekilis“ pö nunädam gebanas. Pö bevüresodanaföm olik ye geb kekilas penemogükon. Mogükolös oni e nunädolös oli me gebananem e letavöd nuliks ola.',
'nocookieslogin'             => '{{SITENAME}} gebon „kekilis“ ad nunädön gebanis. Anu geb kekilas nemögon. Mögükolös onis e steifülolös nogna.',
'noname'                     => 'No egivol gebananemi lonöföl.',
'loginsuccesstitle'          => 'Enunädol oli benosekiko',
'loginsuccess'               => "'''Binol anu in {{SITENAME}} as \"\$1\".'''",
'nosuchuser'                 => 'No dabinon geban labü nem: "$1". Koräkolös tonatami nema at, u jafolös kali nulik.',
'nosuchusershort'            => 'No dabinon geban labü nem: "<nowiki>$1</nowiki>". Koräkolös tonatami nema at.',
'nouserspecified'            => 'Mutol välön gebananemi.',
'wrongpassword'              => 'Letavöd neveräton. Steifülolös dönu.',
'wrongpasswordempty'         => 'Letavöd vagon. Steifülolös dönu.',
'passwordtooshort'           => 'Letavöd olik no lonöfon u binon tu brefik.
Muton binädon me {{PLURAL:$1|malat|malats}} pu $1 e difön de gebananem olik.',
'mailmypassword'             => 'Sedön letavödi nulik',
'passwordremindertitle'      => 'Letavöd nulik nelaidik in {{SITENAME}}',
'passwordremindertext'       => 'Ek (luveratiko ol, se ladet-IP: $1)
ebegon, das osedobs ole letavödi nulik pro {{SITENAME}} ($4).
Letavöd gebana: "$2" binon anu "$3".
Anu kanol nunädön oli e votükön letavödi olik.

If no ol, ab pösod votik ebegon letavödi nulik, ud if ememol letavödi olik e no plu vilol votükön oni, kanol nedemön penedi at e laigebön letavödi rigik ola.',
'noemail'                    => 'Ladet leäktronik nonik peregistaron pro geban "$1".',
'passwordsent'               => 'Letavöd nulik pesedon ladete leäktronik fa "$1" peregistaröle.<br />
Nunädolös oli dönu posä ogetol oni.',
'blocked-mailpassword'       => 'Redakam me ladet-IP olik peblokon; sekü atos, ad neletön migebi, no dalol gebön oni ad gegetön letavödi olik.',
'eauthentsent'               => 'Pened leäktronik pesedon ladete pegivöl ad fümükön dabini onik.
Büä pened votik alseimik okanon pasedön kale at, omutol dunön valikosi in pened at peflagöli, ad fümükön, das kal binon jenöfo olik.',
'throttled-mailpassword'     => 'Mebapened tefü letavöd olik ya pesedon, dü {{PLURAL:$1|düp lätik|düps lätik $1}}.
Ad neletön migebi, mebapened te bal a {{PLURAL:$1|düp|düps $1}} dalon pasedön.',
'mailerror'                  => 'Pöl dü sedam pota: $1',
'acct_creation_throttle_hit' => 'Säkusädolös, ya ejafol kalis $1. No plu kanol jafön kali nulik.',
'emailauthenticated'         => 'Ladet leäktronik olik päfümükon tü düp $1.',
'emailnotauthenticated'      => 'Ladet leäktronik ola no nog pefümedon. Pened nonik posedon me pads sököl.',
'noemailprefs'               => 'Givolös ladeti leäktronik, dat pads at okanons pagebön.',
'emailconfirmlink'           => 'Fümedolös ladeti leäktronik ola',
'invalidemailaddress'        => 'Ladet leäktronik no kanon pazepön bi jiniko labon fomäti no lonöföli. Vagükolös penamaspadi at, u penolös ladeti labü fomät verätik.',
'accountcreated'             => 'Kal pejafon',
'accountcreatedtext'         => 'Gebanakal pro $1 pejafon.',
'createaccount-title'        => 'Kalijafam in {{SITENAME}}',
'createaccount-text'         => 'Ek ejafon kali pro ladet leäktronik ola in {{SITENAME}} ($4) labü nem: „$2“ e letavöd: „$3“. Kanol nunädön oli e votükön letavödi olik anu.

Kanol nedemön penedi at, üf jafam kala at binon pöl.',
'loginlanguagelabel'         => 'Pük: $1',

# Password reset dialog
'resetpass'               => 'Dönuvälön kalaletavödi',
'resetpass_announce'      => 'Enunädol oli me kot nelaidüpik pisedöl ole. Ad finükön nunädami, mutol välön letavödi nulik is:',
'resetpass_header'        => 'Dönuvälön letavödi',
'resetpass_submit'        => 'Välön letavödi e nunädön omi',
'resetpass_success'       => 'Letavöd olik pevotükon benosekiko! Anu sit nunädon oli...',
'resetpass_bad_temporary' => 'Letavöd nelaidüpik no lonöföl. Ba ya evotükol letavödi olik, u ba ya ebegol letavödi nelaidüpik nulik.',
'resetpass_forbidden'     => 'Letavöds no kanons pavotükön in {{SITENAME}}',
'resetpass_missing'       => 'Fomet labon nünis nonik.',

# Edit page toolbar
'bold_sample'     => 'Vödem bigik',
'bold_tip'        => 'Vödem bigik',
'italic_sample'   => 'Korsiv',
'italic_tip'      => 'Korsiv',
'link_sample'     => 'Yümatiäd',
'link_tip'        => 'Yüm ninik',
'extlink_sample'  => 'http://www.example.com yümatiäd',
'extlink_tip'     => 'Yüm plödik (memolös foyümoti: http://)',
'headline_sample' => 'Tiädavödem',
'headline_tip'    => 'Tiäd nivoda 2id',
'math_sample'     => 'Pladolös malatami isio',
'math_tip'        => 'Malatam matematik (LaTeX)',
'nowiki_sample'   => 'Pladolös isio vödemi no pefomätöli',
'nowiki_tip'      => 'Nedemön vükifomätami',
'image_tip'       => 'Magod penüpladöl',
'media_tip'       => 'Yüm lü ragiv mediatik',
'sig_tip'         => 'Dispenäd olik kobü dät e tim',
'hr_tip'          => 'Lien horitätik (no gebolös tu suvo)',

# Edit pages
'summary'                   => 'Plän brefik',
'subject'                   => 'Subyet/tiäd',
'minoredit'                 => 'Votükam pülik',
'watchthis'                 => 'Galädolöd padi at',
'savearticle'               => 'Dakipolöd padi',
'preview'                   => 'Büologed',
'showpreview'               => 'Jonolöd padalogoti',
'showlivepreview'           => 'Büologed vifik',
'showdiff'                  => 'Jonolöd votükamis',
'anoneditwarning'           => "'''Nuned:''' No enunädol oli. Ladet-IP olik poregistaron su redakamajenotem pada at.",
'missingsummary'            => "'''Noet:''' No epenol redakamipläni. If ovälol dönu knopi: Dakipolöd, redakam olik podakipon nen plän.",
'missingcommenttext'        => 'Penolös, begö! küpeti dono.',
'missingcommentheader'      => "'''Noet:''' No epenol yegädi/tiädi küpete at. If ovälol dönu knopi: Dakipolöd, redakam olik podakipon nen on.",
'summary-preview'           => 'Büologed brefik',
'subject-preview'           => 'Büologed yegäda/diläda',
'blockedtitle'              => 'Geban peblokon',
'blockedtext'               => "<big>'''Gebananam u ladet-IP olik(s) peblokon(s).'''</big>

Blokam at pejenükon fa $1. Kod binon ''$2''.

* Prim blokama: $8
* Fin blokama: $6
* Geban desinik: $7

Kanol penön ele $1, u [[{{MediaWiki:Grouppage-sysop}}|guvanes]], ad bespikön blokami.
Kanol gebön yümi: 'penön gebane at' bisä ladet leäktronik verätik lonöföl patuvon in [[Special:Preferences|buükams kala]] olik. Ladet-IP nuik ola binon $3 e nüm blokama binon #$5. Mäniotolös oni pö säks valik.",
'autoblockedtext'           => "Ladet-IP olik peblokon itjäfidiko bi pägebon fa geban, kel peblokon fa geban: $1.
Kod blokama äbinon:

:''$2''

* Prim bloküpa: $8
* Fin bloküpa: $6

Dalol penön gebane: $1 u balane [[{{MediaWiki:Grouppage-sysop}}|guvanas votik]] ad bespikön bloki at.

Küpälolös, das no dalol gebön yümi: „penön gebane at“ if no labol ladet leäktronik lonöföl in [[Special:Preferences|büukams olik]] ed if geb onik fa ol no peblokon.

Blokamanüm olik binon $5. Mäniotolös, begö! oni in peneds valik olik.",
'blockednoreason'           => 'kod nonik pegivon',
'blockedoriginalsource'     => "Fonät pada: '''$1''' pajonon dono:",
'blockededitsource'         => "Vödem '''redakamas olik''' pada: '''$1''' pajonon dono:",
'whitelistedittitle'        => 'Mutol nunädön oli ad redakön',
'whitelistedittext'         => 'Mutol $1 ad redakön padis.',
'confirmedittitle'          => 'Fümedam me pot leäktronik zesüdon ad redakön',
'confirmedittext'           => 'Mutol fümedön ladeti leäktronik ola büä okanol redakön padis. Pladölos e lonöfükölos ladeti olik in [[Special:Preferences|buükams olik]].',
'nosuchsectiontitle'        => 'Diläd at no dabinon',
'nosuchsectiontext'         => 'Esteifülol ad redakön dilädi no dabinöli. Bi diläd: $1 no dabinon, redakam onik no kanon padakipön.',
'loginreqtitle'             => 'Nunädam Paflagon',
'loginreqlink'              => 'ninädolös obi',
'loginreqpagetext'          => 'Mutol $1 ad logön padis votik.',
'accmailtitle'              => 'Letavöd pesedon.',
'accmailtext'               => 'Letavöd pro "$1" pasedon lü $2.',
'newarticle'                => '(Nulik)',
'newarticletext'            => "Esökol yümi lü pad, kel no nog dabinon.
Ad jafön padi at, primolös ad klavön vödemi olik in penaspad dono (logolöd [[{{MediaWiki:Helppage}}|yufapadi]] tefü nüns tefik votik).
If binol is pölo, välolös knopi: '''geikön''' bevüresodatävöma olik.",
'anontalkpagetext'          => "----''Bespikapad at duton lü geban nennemik, kel no nog ejafon kali, u no vilon labön u gebön oni. Sekü atos pemütobs ad gebön ladeti-IP ad dientifükön gebani at. Ladets-IP kanons pagebön fa gebans difik. If binol geban nennemik e cedol, das küpets netefik pelüodükons ole, [[Special:UserLogin|jafolös, begö! kali u nunädolös oli]] ad vitön kofudi ko gebans nennemik votik.''",
'noarticletext'             => 'Atimo no dabinon vödem su pad at. Kanol [[Special:Search/{{PAGENAME}}|sukön padatiädi at]] su pads votik u [{{fullurl:{{FULLPAGENAME}}|action=edit}} redakön padi at].',
'userpage-userdoesnotexist' => 'Gebanakal: "$1" no peregistaron. Fümükolös, va vilol jäfön/redakön padi at.',
'clearyourcache'            => "'''Prudö!''' Pos dakip buükamas, mögos, das ozesüdos ad nedemön memi nelaidüpik bevüresodatävöma ad logön votükamis. '''Mozilla / Firefox / Safari:''' kipolöd klavi ''Shift'' dono e välolöd eli ''Reload'' (= dönulodön) me mugaparat, u dränolöd klävis ''Ctrl-Shift-R'' (''Cmd-Shift-R'' pö el Apple Mac); pro el '''IE:''' (Internet Explorer) kipolöd klavi ''Ctrl'' dono e välolöd eli ''Refresh'' (= flifädükön) me mugaparat, u dränolöd klavis ''Ctrl-F5''; '''Konqueror:''' välolöd eli ''Reload'' (= dönulodön) me mugaparat, u dränolöd klavi ''F5''; gebans ela '''Opera''' ba nedons vagükön lölöfiko memi nelaidüpik me ''Tools→Preferences'' (Stumem->Buükams).",
'usercssjsyoucanpreview'    => '<strong>Mob:</strong> Välolös eli „Jonön büologedi“ ad blufön eli CSS/JS nulik olik bü dakip.',
'usercsspreview'            => "'''Memolös, das anu te büologol eli CSS olik.'''
'''No nog pedakipon!'''",
'userjspreview'             => "'''Memolös, das anu te blufol/büologol eli JavaScript olik, no nog pedakipon!'''",
'userinvalidcssjstitle'     => "'''Nuned:''' No dabinon fomät: \"\$1\".
Memolös, das pads: .css e .js mutons labön tiädi minudik: {{ns:user}}:Foo/monobook.css, no {{ns:user}}:Foo/Monobook.css.",
'updated'                   => '(peatimükon)',
'note'                      => '<strong>Penet:</strong>',
'previewnote'               => '<strong>Is pajonon te büologed; votükams no nog pedakipons!</strong>',
'previewconflict'           => 'Büologed at jonon vödemi in redakamaspad löpik soäsä opubon if odakipol oni.',
'session_fail_preview'      => '<strong>Pidö! No emögos ad lasumön votükamis olik kodü per redakamanünas.<br />Steifülolös dönu. If no oplöpol, tän senunädolös e genunädolös oli, e steifülolös nogna.</strong>',
'session_fail_preview_html' => "<strong>Liedo no eplöpos ad zepön redakami olik kodü per nünodas.</strong>

''Bi {{SITENAME}} emogükon gebi kota: HTML krüdik, büologed peklänedon as jel ta tataks me el JavaScript.

<strong>If evilol dunön redakami legik, steifülolös dönu. If no jäfidon, senunädolös oli e nunädolös oli dönu.</strong>",
'editing'                   => 'Redakam pada: $1',
'editingsection'            => 'Redakam pada: $1 (diläd)',
'editingcomment'            => 'Redakam pada: $1 (küpet)',
'editconflict'              => 'Redakamakonflit: $1',
'explainconflict'           => 'Ek evotükon padi at sisä äprimol ad redakön oni. Vödem balid jonon padi soäsä dabinon anu. Votükams olik pajonons in vödem telid. Sludolös, vio fomams tel at mutons pabalön. Kanol kopiedön se vödem telid ini balid.
<b>Te vödem balid podakipon!</b><br />',
'yourtext'                  => 'Vödem olik',
'storedversion'             => 'Fomam pedakipöl',
'nonunicodebrowser'         => '<strong>NÜNED: Bevüresodatävöm olik no kanon gebön eli Unicode.
Ad dälön ole ad redakön padis, malats no-ASCII opubons in redakamabog as kots degmälnumatik.</strong>',
'editingold'                => '<strong>NUNED: Anu redakol fomami büik pada at. If dakipol oni, votükams posik onepubons.</strong>',
'yourdiff'                  => 'Difs',
'copyrightwarning'          => 'Demolös, das keblünots valik lü Vükiped padasumons ma el $2 (logolöd eli $1 tefü notets). If no vilol, das vödems olik poredakons nenmisero e poseagivons ma vil alana, tän no pladolös oni isio.<br />
Garanol obes, das ol it epenol atosi, u das ekopiedol atosi se räyun notidik u se fon libik sümik.<br />
<strong>NO PLADOLÖD ISIO NEN DÄL LAUTANA VÖDEMIS LABÜ KOPIEDAGITÄT!</strong>',
'copyrightwarning2'         => 'Demolös, das keblünots valik lü {{SITENAME}} padasumons ma el $2 (logolöd eli $1 tefü notets).
If no vilol, das vödems olik poredakons nenmisero e poseagivons ma vil alana, tän no pladolös onis isio.<br />
Garanol obes, das ol it epenol atosi, u das ekopiedol atosi se räyun notidik u se fon libik sümik (logolös $1 pro notets).
<strong>NO PLADOLÖD ISIO NEN DÄL LAUTANA VÖDEMIS LABÜ KOPIEDAGITÄT!</strong>',
'longpagewarning'           => '<strong>NUNED: Pad at labon lunoti miljölätas $1; bevüresodatävöms anik ba no fägons ad redakön nendsäkädo padis lunotü miljölats plu 32. Betikolös dilami pada at ad pads smalikum.</strong>',
'longpageerror'             => '<strong>PÖL: Vödem fa ol pesedöl labon lunoti miljölätas $1, kelos pluon leigodü völad muik pedälöl miljölätas $2. No kanon padakipön.</strong>',
'readonlywarning'           => '<strong>NUNED: Vük pefärmükon kodü kodididazesüd. No kanol dakipön votükamis olik anu. Kopiedolös vödemi nulik ini program votik e dakipolös oni in nünöm olik. Poso okanol dönu steifülön ad pladön oni isio.</strong>',
'protectedpagewarning'      => '<strong>NUNED: Pad at pejelon, dat te gebans labü guvanagitäts kanons redakön oni.</strong>',
'semiprotectedpagewarning'  => "'''Noet:''' Pad at pefärmükon. Te gebans peregistaröl kanons redakön oni.",
'cascadeprotectedwarning'   => "'''Nuned:''' Pad at pefärmükon löko (te guvans dalons redakön oni) bi binon dil {{PLURAL:$1|pada|padas}} sököl, me sökodajel {{PLURAL:$1|pejelöla|pejelölas}}:",
'titleprotectedwarning'     => '<strong>NUNED: Pad at pejelon, dat te gebans anik kanons jafön oni.</strong>',
'templatesused'             => 'Samafomots su pad at pegeböls:',
'templatesusedpreview'      => 'Samafomots in büologed at pageböls:',
'templatesusedsection'      => 'Samafomots in diläd at pageböls:',
'template-protected'        => '(pejelon)',
'template-semiprotected'    => '(dilo pejelon)',
'hiddencategories'          => 'Pad at duton lü {{PLURAL:$1|klad peklänedöl 1|klads peklänedöl $1}}:',
'nocreatetitle'             => 'Padijafam pemiedükon',
'nocreatetext'              => '{{SITENAME}} emiedükon mögi ad jafön padis nulik.
Kanol redakön padi dabinöl, u [[Special:UserLogin|nunädön oli u jafön kali]].',
'nocreate-loggedin'         => 'No dalol jafön padis nulik in {{SITENAME}}.',
'permissionserrors'         => 'Dälapöls',
'permissionserrorstext'     => 'No dalol dunön atosi sekü {{PLURAL:$1|kod|kods}} sököl:',
'recreate-deleted-warn'     => "'''NUNED: Dönujafol padi pemoüköl.'''

Vätälolös, va binos pötik ad lairedakön padi at.
Jenotalised moükama pada at pajonon is as yuf.",

# Parser/template warnings
'expensive-parserfunction-warning'  => 'Nuned: pad at vokon „parser“-sekätis tusuvo.

Muton labön vokis läs $2, ab labon anu vokis $1.',
'expensive-parserfunction-category' => 'Pads, kels vokons tusuvo „parser“-sekätis jerik',

# "Undo" feature
'undo-success' => 'Redakam at kanon pasädunön. Reidolös leigodi dono ad fümükön, va vilol vo dunön atosi, e poso dakipolös votükamis ad fisädunön redakami.',
'undo-failure' => 'No eplöpos ad sädunön redakami at sekü konflits vü redakams vüik.',
'undo-norev'   => 'No eplöpos ad sädunön redakami at, bi no dabinon u pämoükon.',
'undo-summary' => 'Äsädunon votükami $1 fa [[Special:Contributions/$2|$2]] ([[User talk:$2|Bespikapad]])',

# Account creation failure
'cantcreateaccounttitle' => 'Kal no kanon pajafön',
'cantcreateaccount-text' => "Kalijaf se ladet-IP at ('''$1''') peblokon fa geban: [[User:$3|$3]].

Kod blokama fa el $3 pegivöl binon ''$2''",

# History pages
'viewpagelogs'        => 'Jonön jenotalisedis pada at',
'nohistory'           => 'Pad at no labon redakamajenotemi.',
'revnotfound'         => 'Fomam no petuvon',
'revnotfoundtext'     => 'Padafomam büik fa ol peflagöl no petuvon. Kontrololös, begö! ladeti-URL, keli egebol ad logön padi at.',
'currentrev'          => 'Fomam anuik',
'revisionasof'        => 'Fomam dätü $1',
'revision-info'       => 'Fomam timü $1 fa el $2',
'previousrevision'    => '←Fomam vönedikum',
'nextrevision'        => 'Fomam nulikum→',
'currentrevisionlink' => 'Fomam anuik',
'cur'                 => 'nuik',
'next'                => 'sököl',
'last'                => 'lätik',
'page_first'          => 'balid',
'page_last'           => 'lätik',
'histlegend'          => 'Difiväl: välolös fomamis ad paleigodön e gebolös klavi: "Enter" u knopi dono.<br />
Plän: (anuik) = dif tefü fomam anuik,
(lätik) = dif tefü fomam büik, p = redakam pülik.',
'deletedrev'          => '[pemoüköl]',
'histfirst'           => 'Balid',
'histlast'            => 'Lätik',
'historysize'         => '({{PLURAL:$1|jölät 1|jöläts $1}})',
'historyempty'        => '(vagik)',

# Revision feed
'history-feed-title'          => 'Revidajenotem',
'history-feed-description'    => 'Revidajenotem pada at in vük',
'history-feed-item-nocomment' => '$1 ün $2', # user at time
'history-feed-empty'          => 'Pad pevipöl no dabinon.
Ba pemoükon se ragivs, u ba pevotanemon.
Kanol [[Special:Search|sukön]] padis nulik tefik.',

# Revision deletion
'rev-deleted-comment'         => '(küpet pemoükon)',
'rev-deleted-user'            => '(gebananem pemoükon)',
'rev-deleted-event'           => '(lisedadun pemoükon)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">
Padafomam at pemoükon se ragivs notidik.
Pats tefik ba patuvons in [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} jenotalised moükamas].</div>',
'rev-deleted-text-view'       => '<div class="mw-warning plainlinks">
Padafomam at pemoükon se registar notidik. As guvan in {{SITENAME}}, kanol logön oni. Pats tefik ba binons in  [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} jenotalised moükamas].</div>',
'rev-delundel'                => 'jonolöd/klänedolöd',
'revisiondelete'              => 'Moükön/sädunön moükami fomamas',
'revdelete-nooldid-title'     => 'Zeilafomam no lonöfon',
'revdelete-nooldid-text'      => 'U no elevälol zeilafomami(s) pro dun at, u fomam pelevälöl no dabinon, u steifülol ad klänedön fomami anuik.',
'revdelete-selected'          => '{{PLURAL:$2|Fomam|Fomams}} pevalöl pada: [[:$1]]:',
'logdelete-selected'          => '{{PLURAL:$1|Lisedajenot|Lisedajenots}} pevälöl:',
'revdelete-text'              => 'Revids pemoüköl nog opubons in padajenotem, ab ninäd (vödem) onsik no gebidons publüge.

Ninäd peklänedöl at binon ye nog lügolovik guvanes votik vüka at: kanons nog geükön oni medü pads patik, üf miedöfükams u neletians pluiks no pepladons.',
'revdelete-legend'            => 'Levälön miedükamis logova:',
'revdelete-hide-text'         => 'Klänedön vödemi revida',
'revdelete-hide-name'         => 'Klänedön duni e zeili',
'revdelete-hide-comment'      => 'Klänedön redakamaküpeti',
'revdelete-hide-user'         => 'Klänedön gebananemi u ladeti-IP redakana',
'revdelete-hide-restricted'   => 'Gebön miedükamis at i demü guvans e lökofärmükön fometi at',
'revdelete-suppress'          => 'Klänedön moükamakodis i de guvans (äsi de votikans)',
'revdelete-hide-image'        => 'Klänedön ragivaninädi',
'revdelete-unsuppress'        => 'Moükön miedükamis fomamas pegegetöl',
'revdelete-log'               => 'Küpet jenotalisedik:',
'revdelete-submit'            => 'Gebön me fomam pevälöl',
'revdelete-logentry'          => 'logov fomamas pada: [[$1]] pevotükon',
'logdelete-logentry'          => 'logov jenota: [[$1]] pevotükon',
'revdelete-success'           => 'Logov padafomama pelonon benosekiko.',
'logdelete-success'           => 'Logov jenotaliseda pelonon benosekiko.',
'revdel-restore'              => 'Votükön logovi',
'pagehist'                    => 'Padajenotem',
'deletedhist'                 => 'Jenotem pemoüköl',
'revdelete-content'           => 'ninäd',
'revdelete-summary'           => 'plän redakama',
'revdelete-uname'             => 'gebananem',
'revdelete-restricted'        => 'miedükams pelonöfükons pro guvans',
'revdelete-unrestricted'      => 'miedükams pro guvans pemoükons',
'revdelete-hid'               => '$1 peklänedon',
'revdelete-unhid'             => '$1 pesäklänedon',
'revdelete-log-message'       => '$1 tefü {{PLURAL:$2|fomam|fomams}} $2',
'logdelete-log-message'       => '$1 tefü {{PLURAL:$2|jenot|jenots}} $2',

# Suppression log
'suppressionlog'     => 'Lovelogam-jenotalised',
'suppressionlogtext' => 'Is palisedons moükams e blokams lätik, kels ätefons ninädi de guvans peklänedöli. Logolös [[Special:IPBlockList|lisedi ladetas-IP pebloköl]], kö pajonons blokams anu lonöföls.',

# History merging
'mergehistory'                     => 'Balön padajenotemis',
'mergehistory-header'              => 'Pad at mogükon balami fomamis se jenotem fonätapada ad fomön padi nulik.
Kontrololös, va votükam at okipon fovöfi padajenotema.',
'mergehistory-box'                 => 'Balön fomamis padas tel:',
'mergehistory-from'                => 'Fonätapad:',
'mergehistory-into'                => 'Zeilapad:',
'mergehistory-list'                => 'Redakamajenotem balovik',
'mergehistory-merge'               => 'Fomams sököl pada: [[:$1]] kanons pabalön ini pad: [[:$2]]. Välolös ad balön te fomamis pejaföl ün u bü tim pegivöl. Demolös, das geb nafamayümas osädunon väli olik.',
'mergehistory-go'                  => 'Jonön redakamis balovik',
'mergehistory-submit'              => 'Balön fomamis',
'mergehistory-empty'               => 'Fomams nonik kanons pabalön.',
'mergehistory-success'             => '{{PLURAL:$3|Fomam 1|Fomams $3}} pada: [[:$1]] {{PLURAL:$3|pebalon|pebalons}} benosekiko ini pad: [[:$2]].',
'mergehistory-fail'                => 'No eplöpos ad ledunön balami jenotemas, kontrololös pada- e timaparametis.',
'mergehistory-no-source'           => 'Fonätapad: $1 no dabinon.',
'mergehistory-no-destination'      => 'Zeilapad: $1 no dabinon.',
'mergehistory-invalid-source'      => 'Fonätapad muton labön tiädi lonöföl',
'mergehistory-invalid-destination' => 'Zeilapad muton labön tiädi lonöföl.',
'mergehistory-autocomment'         => 'Pad: [[:$1]] peninükon ini pad: [[:$2]].',
'mergehistory-comment'             => 'Pad: [[:$1]] peninükon ini pad: [[:$2]]: $3',

# Merge log
'mergelog'           => 'Jenotalised padibalamas',
'pagemerge-logentry' => 'Pad: [[$1]] pebalon ad [[$2]] (fomams jüesa $3)',
'revertmerge'        => 'Säbalön',
'mergelogpagetext'   => 'Is palisedon balamis brefabüikün jenotema pada bal ini votik.',

# Diffs
'history-title'           => 'Revidajenotem pada: "$1"',
'difference'              => '(dif vü revids)',
'lineno'                  => 'Lien $1:',
'compareselectedversions' => 'Leigodolöd fomamis pevälöl',
'editundo'                => 'sädunön',
'diff-multi'              => '({{PLURAL:$1|Revid vüik bal no pejonon|Revids vüik $1 no pejonons}}.)',

# Search results
'searchresults'             => 'Sukaseks',
'searchresulttext'          => 'Ad lärnön mödikumosi dö suks in {{SITENAME}}, logolös [[{{MediaWiki:Helppage}}|Suks in {{SITENAME}}]].',
'searchsubtitle'            => "Esukol padi: '''[[:$1]]'''",
'searchsubtitleinvalid'     => "Esukol padi: '''$1'''",
'noexactmatch'              => "'''No dabinon pad tiädü \"\$1\".''' Kanol [[:\$1|jafön oni]].",
'noexactmatch-nocreate'     => "'''No dabinon pad tiädü \"\$1\".'''",
'toomanymatches'            => 'Pads tu mödiks labü vöd(s) pesuköl petuvons. Sukolös vödi(s) votik.',
'titlematches'              => 'Leigon ko padatiäd',
'notitlematches'            => 'Leigon ko padatiäds nonik',
'textmatches'               => 'Leigon ko dil padavödema',
'notextmatches'             => 'Leigon ko nos in padavödem',
'prevn'                     => 'büik $1',
'nextn'                     => 'sököl $1',
'viewprevnext'              => 'Logön padis ($1) ($2) ($3).',
'search-result-size'        => '$1 ({{PLURAL:$2|vöd 1|vöds $2}})',
'search-result-score'       => 'Demäd: $1%',
'search-redirect'           => '(lüodüköm: $1)',
'search-section'            => '(diläd: $1)',
'search-suggest'            => 'Ediseinol-li: $1 ?',
'search-interwiki-caption'  => 'Svistaproyegs',
'search-interwiki-default'  => 'Seks se $1:',
'search-interwiki-more'     => '(pluikos)',
'search-mwsuggest-enabled'  => 'sa mobs',
'search-mwsuggest-disabled' => 'nen mobs',
'search-relatedarticle'     => 'Tefik',
'mwsuggest-disable'         => 'Nemögükön mobis ela AJAX',
'searchrelated'             => 'tefik',
'searchall'                 => 'valik',
'showingresults'            => "Pajonons dono jü {{PLURAL:$1|sukasek '''1'''|sukaseks '''$1'''}}, primölo me nüm #'''$2'''.",
'showingresultsnum'         => "Dono pajonons {{PLURAL:$3:|sek '''1'''|seks '''$3'''}}, primölo me nüm: '''$2'''.",
'showingresultstotal'       => "Is palisedons {{PLURAL:$3|sukasek nüm: '''$1''' se '''$3'''|sukaseks nüm: '''$1 - $2''' se '''$3'''}}",
'nonefound'                 => "'''Noet''': Suks no benosekiks suvo pakodons dub steifüls ad tuvön vödis suvik äs „binon“ u „at“, tefü kels komataibs no padunons, u dub suk vöda plu bala. Te pads labü vöds pasuköl valiks polisedons.",
'powersearch'               => 'Suk',
'powersearch-legend'        => 'Suk komplitikum',
'search-external'           => 'Suk plödik',
'searchdisabled'            => 'Suk in {{SITENAME}} penemogükon. Vütimo kanol sukön yufü el Google. Demolös, das liseds onik tefü ninäd in {{SITENAME}} ba no binon anuik.',

# Preferences page
'preferences'              => 'Buükams',
'mypreferences'            => 'Buükams obik',
'prefs-edits'              => 'Num redakamas:',
'prefsnologin'             => 'No enunädon oki',
'prefsnologintext'         => 'Nedol [[Special:UserLogin|nunädön oli]] büä kanol votükön gebanabuükamis.',
'prefsreset'               => 'Buükams egekömons ad stad büik peregistaröl.',
'qbsettings'               => 'Stumem',
'qbsettings-none'          => 'Nonik',
'qbsettings-fixedleft'     => 'nedeto (fimiko)',
'qbsettings-fixedright'    => 'Deto (fimiko)',
'qbsettings-floatingleft'  => 'nedeto (vebölo)',
'qbsettings-floatingright' => 'deto (vebölo)',
'changepassword'           => 'Votükön letavödi',
'skin'                     => 'Fomät',
'math'                     => 'Logot formülas',
'dateformat'               => 'Dätafomät',
'datedefault'              => 'Buükam nonik',
'datetime'                 => 'Dät e Tim',
'math_failure'             => 'Diletam fomüla no eplöpon',
'math_unknown_error'       => 'pök nesevädik',
'math_unknown_function'    => 'dun nesevädik',
'math_lexing_error'        => 'vödidiletam no eplöpon',
'math_syntax_error'        => 'süntagapöl',
'math_image_error'         => 'Feajafam ela PNG no eplöpon;
vestigolös stitami verätik ela latex, ela dvips, ela gs, e feajafön',
'math_bad_tmpdir'          => 'No mögos ad penön ini / jafön ragiviär(i) matematik nelaidüpik.',
'math_bad_output'          => 'No mögos ad penön ini / jafön ragiviär(i) matematik labü seks',
'prefs-personal'           => 'Gebananüns',
'prefs-rc'                 => 'Votükams nulik',
'prefs-watchlist'          => 'Galädalised',
'prefs-watchlist-days'     => 'Num delas ad pajonön in galädalised:',
'prefs-watchlist-edits'    => 'Num redakamas ad pajonön in galädalised pestäänüköl:',
'prefs-misc'               => 'Votikos',
'saveprefs'                => 'Dakipolöd',
'resetprefs'               => 'Buükams rigik',
'oldpassword'              => 'Letavöd büik:',
'newpassword'              => 'Letavöd nulik:',
'retypenew'                => 'Klavolöd dönu letavödi nulik:',
'textboxsize'              => 'Redakam',
'rows'                     => 'Kedets:',
'columns'                  => 'Padüls:',
'searchresultshead'        => 'Suk',
'resultsperpage'           => 'Tiäds petuvöl a pad:',
'contextlines'             => 'Kedets a pad petuvöl:',
'contextchars'             => 'Kevödem a kedet:',
'stub-threshold'           => 'Soliad pro fomätam <a href="#" class="stub">sidayümas</a> (jöläts):',
'recentchangesdays'        => 'Dels ad pajonön in votükams nulik:',
'recentchangescount'       => 'Tiäds in lised votükamas nulik:',
'savedprefs'               => 'Buükams olik pedakipons.',
'timezonelegend'           => 'Timatopäd',
'timezonetext'             => 'Num düpas, mö kel tim topik difon de tim dünanünöma (UTC).',
'localtime'                => 'Tim topik',
'timezoneoffset'           => 'Näedot¹',
'servertime'               => 'Tim dünanünöma',
'guesstimezone'            => 'Benüpenolös yufü befüresodatävöm',
'allowemail'               => 'Fägükolös siti ad getön poti leäktronik de gebans votik',
'prefs-namespaces'         => 'Nemaspads',
'defaultns'                => 'Sukolös nomiko in nemaspads at:',
'default'                  => 'stad kösömik',
'files'                    => 'Ragivs',

# User rights
'userrights'                  => 'Guvam gebanagitätas', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => 'Guvön gebanagrupis',
'userrights-user-editname'    => 'Penolös gebananemi:',
'editusergroup'               => 'Redakön Gebanagrupis',
'editinguser'                 => "Votükam gitätas gebana: '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup'    => 'Redakön gebanagrupis',
'saveusergroups'              => 'Dakipolöd gebanagrupis',
'userrights-groupsmember'     => 'Liman grupa(s):',
'userrights-groups-help'      => 'Dalol votükön grupis, lü kels geban at duton.
* Bügil fulik sinifon, das geban duton lü grup tefik.
* Bügil vagik sinifon, das geban no duton lü grup tefik.
* El * sinifon, das no kanol moükön grupi posä iläükol oni, u güo.',
'userrights-reason'           => 'Kod votükama:',
'userrights-no-interwiki'     => 'No labol däli ad votükön gebanagitätis in vüks votik.',
'userrights-nodatabase'       => 'Nünodem: $1 no dabinon, u no binon topik.',
'userrights-nologin'          => 'Mutol [[Special:UserLogin|nunädön oli]] me guvanakal ad dalön gevön gitätis gebanes.',
'userrights-notallowed'       => 'Kal olik no labon däli ad votükön gebanagitätis.',
'userrights-changeable-col'   => 'Grups fa ol votükoviks',
'userrights-unchangeable-col' => 'Grups fa ol nevotükoviks',

# Groups
'group'               => 'Grup:',
'group-user'          => 'Gebans',
'group-autoconfirmed' => 'Gebans itjäfidiko pezepöls',
'group-bot'           => 'Bots',
'group-sysop'         => 'Guvans',
'group-bureaucrat'    => 'Bürans',
'group-suppress'      => 'Lovelogams',
'group-all'           => '(valik)',

'group-user-member'          => 'Geban',
'group-autoconfirmed-member' => 'Geban itjäfidiko pezepöl',
'group-bot-member'           => 'Bot',
'group-sysop-member'         => 'Guvan',
'group-bureaucrat-member'    => 'Büran',
'group-suppress-member'      => 'Lovelogam',

'grouppage-user'          => '{{ns:project}}:Gebans',
'grouppage-autoconfirmed' => '{{ns:project}}:Gebans itjäfidiko pezepöls',
'grouppage-bot'           => '{{ns:project}}:Bots',
'grouppage-sysop'         => '{{ns:project}}:Guvans',
'grouppage-bureaucrat'    => '{{ns:project}}:Bürans',
'grouppage-suppress'      => '{{ns:project}}:Lovelogam',

# Rights
'right-read'             => 'Reidön padis',
'right-edit'             => 'Redakön padis',
'right-createpage'       => 'Jafön padis (no bespikapadis)',
'right-createtalk'       => 'Jafön bespikapadis',
'right-createaccount'    => 'Jafön gebanakalis nulik',
'right-minoredit'        => 'Malön redakamis as püliks.',
'right-move'             => 'Topätükön padis',
'right-suppressredirect' => 'No jafön lüodükömi de nem büik posä pad petopätükon',
'right-upload'           => 'Löpükön ragivis',
'right-upload_by_url'    => 'Löpükön ragivi se ladet-URL.',
'right-autoconfirmed'    => 'Redakön padis dilo pejelölis',
'right-bot'              => 'Palelogön as dun itjäfidik',
'right-delete'           => 'Moükön padis',
'right-bigdelete'        => 'Moükön padis labü jenotems lunik',
'right-deleterevision'   => 'Moükön u sädunön moükami padafomamas pevälöl',
'right-deletedhistory'   => 'Logön jenotemis pemoüköl nen vödems tefik',
'right-browsearchive'    => 'Sukön padis pemoüköl',
'right-undelete'         => 'Sädunön padimoükami',
'right-block'            => 'Blokön redakamagitäti gebanas votik',
'right-blockemail'       => 'Blokön gitäti gebana ad sedön penedis leäktronik',
'right-hideuser'         => 'Blokön gebananemi, klänedölo oni de votikans',
'right-protect'          => 'Votükön jelanivodis e redakön padis pejelöl',
'right-editusercssjs'    => 'Redakön ragivis-CSS e -JS gebanas votik',
'right-patrol'           => 'Zepön redakamis',
'right-mergehistory'     => 'Kobükön padajenotemis',
'right-userrights'       => 'Redakön gebanagitätis valik',

# User rights log
'rightslog'      => 'Jenotalised gebanagitätas',
'rightslogtext'  => 'Is palisedons votükams gebanagitätas.',
'rightslogentry' => 'grupalimanam gebana: $1 pevotükon de $2 ad $3',
'rightsnone'     => '(nonik)',

# Recent changes
'nchanges'                          => '{{PLURAL:$1|votükam|votükams}} $1',
'recentchanges'                     => 'Votükams nulik',
'recentchangestext'                 => 'Su pad at binons votükams nulikün in vüki at.',
'recentchanges-feed-description'    => 'Getön votükamis nulikün in vük at me nünakanad at.',
'rcnote'                            => "Dono {{PLURAL:$1|binon votükam '''1'''|binons votükams '''$1'''}} lätikün {{PLURAL:$2|dela|delas '''$2'''}} lätikün, pänumädöls tü $5, $4.",
'rcnotefrom'                        => "Is palisedons votükams sis '''$2''' (jü '''$1''').",
'rclistfrom'                        => 'Jonolöd votükamis nulik, primölo tü düp $1',
'rcshowhideminor'                   => '$1 votükams pülik',
'rcshowhidebots'                    => '$1 elis bot',
'rcshowhideliu'                     => '$1 gebanis penunädöl',
'rcshowhideanons'                   => '$1 gebanis nennemik',
'rcshowhidepatr'                    => 'Redakams $1 pekontrolons',
'rcshowhidemine'                    => '$1 redakamis obik',
'rclinks'                           => 'Jonolöd votükamis lätik $1 ün dels lätik $2<br />$3',
'diff'                              => 'dif',
'hist'                              => 'jen',
'hide'                              => 'Klänedolöd',
'show'                              => 'Jonolöd',
'minoreditletter'                   => 'p',
'newpageletter'                     => 'N',
'boteditletter'                     => 'b',
'number_of_watching_users_pageview' => '[{{PLURAL:$1|geban|gebans}} galädöl $1]',
'rc_categories'                     => 'Te klads fovik (ditolös me el "|")',
'rc_categories_any'                 => 'Alseimik',
'newsectionsummary'                 => '/* $1 */ diläd nulik',

# Recent changes linked
'recentchangeslinked'          => 'Votükams teföl',
'recentchangeslinked-title'    => 'Votükams tefü pad: "$1"',
'recentchangeslinked-noresult' => 'Pads ad pad at peyümöls no pevotükons ün period at.',
'recentchangeslinked-summary'  => "Su pad patik at palisedons votükams padas, lü kels pad pevälöl yumon. 
If ye pad pevälöl binon klad, palisedons is votükams nulik padas in klad at.
Pads [[Special:Watchlist|galädaliseda olik]] '''pakazetons'''.",
'recentchangeslinked-page'     => 'Padanem:',
'recentchangeslinked-to'       => 'Jonön güo votükamis padas, kels yumons ad pad pevälöl',

# Upload
'upload'                      => 'Löpükön ragivi',
'uploadbtn'                   => 'Löpükön ragivi',
'reupload'                    => 'Löpükön dönu',
'reuploaddesc'                => 'Nosükon lopükami e geikön lü löpükamafomet.',
'uploadnologin'               => 'No enunädon oki',
'uploadnologintext'           => 'Mutol [[Special:UserLogin|nunädön oli]] ad löpükön ragivis.',
'upload_directory_read_only'  => 'Ragiviär lopükama ($1) no kanon papenön fa dünanünöm bevüresodik.',
'uploaderror'                 => 'Pök pö löpükam',
'uploadtext'                  => "Gebolös fometi dono ad löpükön ragivis. Ad logön u sukön ragivis ya pelöpükölis, gololös lü [[Special:ImageList|lised ragivas pelöpüköl]].
Löpükams e moükams padakipons id in  [[Special:Log/upload|jenotalised löpükamas]].

Ad pladön magodi at ini pad semik, gebolös yümi fomätü:
'''<nowiki>[[</nowiki>{{ns:image}}<nowiki>:File.jpg]]</nowiki>''',
'''<nowiki>[[</nowiki>{{ns:image}}<nowiki>:File.png|alt text]]</nowiki>''' u
'''<nowiki>[[</nowiki>{{ns:media}}<nowiki>:File.ogg]]</nowiki>''' ad yümön stedöfiko ko ragiv.",
'upload-permitted'            => 'Ragivasots pedälöl: $1.',
'upload-preferred'            => 'Ragivasots buik: $1.',
'upload-prohibited'           => 'Ragivasots peproiböl: $1.',
'uploadlog'                   => 'jenotalised löpükamas',
'uploadlogpage'               => 'Jenotalised löpükamas',
'uploadlogpagetext'           => 'Dono binon lised ravigalöpükamas nulikün.',
'filename'                    => 'Ragivanem',
'filedesc'                    => 'Plän brefik',
'fileuploadsummary'           => 'Plän brefik:',
'filestatus'                  => 'Stad tefü kopiedagität:',
'filesource'                  => 'Fon:',
'uploadedfiles'               => 'Ragivs pelöpüköl',
'ignorewarning'               => 'Nedemön nunedi e dakipön ragivi',
'ignorewarnings'              => 'Nedemolöd nunedis alseimik',
'minlength1'                  => 'Ragivanems mutons labön tonati pu bali.',
'illegalfilename'             => 'Ragivanem: „$1“ labon malatis no pedälölis pö padatiäds. Votanemolös ragivi e steifülolös ad löpükön oni dönu.',
'badfilename'                 => 'Ragivanem pevotükon ad "$1".',
'filetype-badmime'            => 'Ragivs MIME-pateda "$1" no dalons palöpükön.',
'filetype-unwanted-type'      => "'''\".\$1\"''' binon ragivasot no pavipöl.
{{PLURAL:\$3|Ragivasot pabuüköl binon|Ragivasots pabuüköl binons}} \$2.",
'filetype-banned-type'        => "'''\".\$1\"''' binon ragivasot no pedälöl.
{{PLURAL:\$3|Ragivasot pedälöl binon|Ragivasots pedälöl binons}} \$2.",
'filetype-missing'            => 'Ragiv no labon stäänükoti (äs el „.jpg“).',
'large-file'                  => 'Pakomandos, das ragivs no binons gretikums ka mö $1; ragiv at binon mö $2.',
'largefileserver'             => 'Ragiv at binon tu gretik: dünanünöm no kanon dälon oni.',
'emptyfile'                   => 'Ragiv fa ol pelöpüköl binon jiniko vägik. Kod atosa äbinon ba pöl pö ragivanem. Vilol-li jenöfo löpükön ragivi at?',
'fileexists'                  => 'Ragiv labü nem at ya dabinon, logolös, begö! <strong><tt>$1</tt></strong> üf no sevol fümiko, va vilol votükön oni.',
'filepageexists'              => 'Bepenamapad ragiva at ya pejafon (<strong><tt>$1</tt></strong>), ab ragiv nonik labü nem at abinon anu. Naböfodönuam olik no opubon su bepenamapad. Ad pübön oni us, onedol redakön oni ol it.',
'fileexists-extension'        => 'Ragiv labü nem sümik ya dabinon:<br />
Nem ragiva palöpüköl: <strong><tt>$1</tt></strong><br />
Nem ragiva dabinöl: <strong><tt>$2</tt></strong><br />
Välolös, begö! nemi difik.',
'fileexists-thumb'            => "<center>'''Magod dabinöl'''</center>",
'fileexists-thumbnail-yes'    => 'Ragiv at binon jiniko magoda gretota smalik <i>(magodil)</i>. Logolös, begö! ragivi ya dabinöli: <strong><tt>$1</tt></strong>.<br />
If ragiv ya dabinöli binon magod ot gretota rigik, no zesüdos ad löpükön magodili pluik.',
'file-thumbnail-no'           => 'Ragivanem primon me <strong><tt>$1</tt></strong>. Binon jiniko magod gretota smalik <i>(magodil)</i>.
Üf labol magodi at gretota rigik, löpükölos oni, pläo votükolös ragivanemi.',
'fileexists-forbidden'        => 'Ragiv labü nem at ya dabinon; geikolös e löpükolös ragivi at me nem votik.[[Image:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => 'Ragiv labü nem at ya dabinon in ragivastok kobädik; geikolös e löpükolös ragivi at me nem votik. [[Image:$1|thumb|center|$1]]',
'successfulupload'            => 'Löpükam eplöpon',
'uploadwarning'               => 'Löpükamanuned',
'savefile'                    => 'Dakipolöd ragivi',
'uploadedimage'               => '"[[$1]]" pelöpüköl',
'overwroteimage'              => 'fomami nulik ragiva: „[[$1]]“ pelöpükon',
'uploaddisabled'              => 'Löpükam penemögükon',
'uploaddisabledtext'          => 'Löpükam ragivas penemögükon in {{SITENAME}}.',
'uploadscripted'              => 'Ragiv at ninükon eli HTML u vödis programapüka, kelis bevüresodanaföm ba opölanätäpreton',
'uploadcorrupt'               => 'Ragiv binon dädik u duton lü sot no lonöföl. Kontrololös ragivi e löpükolös oni dönu.',
'uploadvirus'                 => 'Ragiv at labon virudi! Pats: $1',
'sourcefilename'              => 'Ragivanem rigik:',
'destfilename'                => 'Ragivanem nulik:',
'upload-maxfilesize'          => 'Ragivagretot gretikün: $1',
'watchthisupload'             => 'Galädolöd padi at',
'filewasdeleted'              => 'Ragiv labü nem at büo pelöpükon e poso pemoükon. Kontrololös eli $1 büä olöpükol oni dönu.',
'upload-wasdeleted'           => "'''Nuned: Löpükol ragivi büo pimoüköl.'''

Vätälolös, va pötos ad löpükön ragivi at. Kodü koveniäl, jenotalised tefü moükam ragiva at pagivon is.",
'filename-bad-prefix'         => 'Nem ragiva fa ol palöpüköl primon me <strong>"$1"</strong>: nem no bepenöl nomiko pagevöl itjäfidiko fa käms nulädik. Välolös, begö! nemi bepenöl pro ragiv olik.',

'upload-proto-error'      => 'Protok neverätik',
'upload-proto-error-text' => 'Löpükam flagon elis URLs me <code>http://</code> u <code>ftp://</code> primölis.',
'upload-file-error'       => 'Pöl ninik',
'upload-file-error-text'  => 'Pöl ninik äjenon dü steifül ad jafön ragivi nelaidüpik pö dünanünöm.
Begolös yufi [[Special:ListUsers/sysop|guvana]].',
'upload-misc-error'       => 'Pök nesevädik pö löpükam',
'upload-misc-error-text'  => 'Pöl nesevädik äjenon dü löpükam.
Fümedolös, begö! das el URL lonöfon e kanon palogön, e poso steifülolös nogna.
If säkäd at laibinon, kosikolös guvani tefü on.',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => 'No eplöpos ad rivön eli URL',
'upload-curl-error6-text'  => 'No eplöpos ad rivön eli URL at. Kontrololös, va el URL veräton e bevüresodatopäd dabinon.',
'upload-curl-error28'      => 'Löpükamatüp efinikon',
'upload-curl-error28-text' => 'Geükam se bevüresodatopäd at ya pestebedon tu lunüpiko.
Kontrololös, begö! va bevüresodatopäd at jäfidon, stebedolös timüli e steifülolös dönu.
Binosöv gudikum, if steifülolöv dönu ün tim votik läs jäfädik.',

'license'            => 'Dälastad:',
'nolicense'          => 'Nonik pelevälon',
'license-nopreview'  => '(Büologed no gebidon)',
'upload_source_url'  => ' (el URL lonöföl ä fa valans gebovik)',
'upload_source_file' => ' (ragiv pö nünöm olik)',

# Special:ImageList
'imagelist-summary'     => 'Su pad patik at ragivs pelöpüköl valik pelisedons.
Nomiko ragivs pelöpüköl lätikün palisedons primü lised.
Klikolös tiädi padüla ad votükön sökaleodi at.',
'imagelist_search_for'  => 'Sukön ragivanemi:',
'imgfile'               => 'ragiv',
'imagelist'             => 'Ragivalised',
'imagelist_date'        => 'Dät',
'imagelist_name'        => 'Nem',
'imagelist_user'        => 'Geban',
'imagelist_size'        => 'Gretot',
'imagelist_description' => 'Bepenam',

# Image description page
'filehist'                  => 'Jenotem ragiva',
'filehist-help'             => 'Välolös däti/timi ad logön ragivi soäsä äbinon ün tim at.',
'filehist-deleteall'        => 'moükön valikis',
'filehist-deleteone'        => 'moükön atosi',
'filehist-revert'           => 'sädunön valikosi',
'filehist-current'          => 'anuik',
'filehist-datetime'         => 'Dät/Tim',
'filehist-user'             => 'Geban',
'filehist-dimensions'       => 'Mafots',
'filehist-filesize'         => 'Ragivagret',
'filehist-comment'          => 'Küpet',
'imagelinks'                => 'Yüms',
'linkstoimage'              => '{{PLURAL:$1|Pad sököl payümon|Pads sököl payümons}} ko pad at:',
'nolinkstoimage'            => 'Pads nonik peyümons ad ragiv at.',
'sharedupload'              => 'Ragiv at binon komunik e kanon pagebön fa proyegs votik.',
'shareduploadwiki'          => 'Logolös eli $1 ad getön nünis pluik.',
'shareduploadwiki-desc'     => 'Bepenam su $1 usik ona pajonon dono.',
'shareduploadwiki-linktext' => 'bepenamapad ragiva',
'noimage'                   => 'Ragiv labü nem at no dabinon, ab kanol $1.',
'noimage-linktext'          => 'löpükön bali',
'uploadnewversion-linktext' => 'Löpükön fomami nulik ragiva at',
'imagepage-searchdupe'      => 'Sukön ragivis pedönuöl',

# File reversion
'filerevert'                => 'Geükön padi: $1',
'filerevert-legend'         => 'Geükön ragivi',
'filerevert-intro'          => "Anu geükol padi: '''[[Media:$1|$1]]''' ad [fomam $4: $3, $2].",
'filerevert-comment'        => 'Küpet:',
'filerevert-defaultcomment' => 'Pegeükon ad fomam: $2, $1',
'filerevert-submit'         => 'Geükön',
'filerevert-success'        => "Pad: '''[[Media:$1|$1]]''' pegeükon ad [fomam $4: $3, $2].",
'filerevert-badversion'     => 'No dabinon fomam topik büik ragiva at labü timamäk pegevöl',

# File deletion
'filedelete'                  => 'Moükön padi: $1',
'filedelete-legend'           => 'Moükön ragivi',
'filedelete-intro'            => "Moükol padi: '''[[Media:$1|$1]]'''.",
'filedelete-intro-old'        => "Anu moükol fomami pada: '''[[Media:$1|$1]]''' [$4 $3, $2].",
'filedelete-comment'          => 'Küpet:',
'filedelete-submit'           => 'Moükön',
'filedelete-success'          => "'''$1''' pemoükon.",
'filedelete-success-old'      => "Fomam ela '''[[Media:$1|$1]]''' timü $3, $2 pemoükon.",
'filedelete-nofile'           => "'''$1''' no dabinon.",
'filedelete-nofile-old'       => "No dabinon fomam peregistaröl pada: '''$1''' labü pats pevipöl.",
'filedelete-iscurrent'        => 'Steifülol ad moükön fomami nulikün ragiva at. Mutol büo geikön ad fomam büik.',
'filedelete-otherreason'      => 'Kod votik/zuik:',
'filedelete-reason-otherlist' => 'Kod votik',
'filedelete-reason-dropdown'  => '*Kods kösömik moükama
** Nedem kopiedagitäta
** Ragiv petelöl',
'filedelete-edit-reasonlist'  => 'Redakön kodis moükama',

# MIME search
'mimesearch' => 'Sukön (MIME)',
'mimetype'   => 'Klad ela MIME:',
'download'   => 'donükön',

# Unwatched pages
'unwatchedpages' => 'Pads no pagalädöls',

# List redirects
'listredirects' => 'Lised lüodükömas',

# Unused templates
'unusedtemplates'     => 'Samafomots no pageböls',
'unusedtemplatestext' => 'Pad at jonon padis valik in nemaspad "samafomot", kels no paninükons in pad votik. Kontrololös, va dabinons yüms votik lü samafomots at büä omoükol onis.',
'unusedtemplateswlh'  => 'yüms votik',

# Random page
'randompage'         => 'Pad fädik',
'randompage-nopages' => 'Pads nonik dabinons in nemaspad at.',

# Random redirect
'randomredirect'         => 'Lüodüköm fädik',
'randomredirect-nopages' => 'Lüodüköms nonik dabinons in nemaspad at.',

# Statistics
'statistics'             => 'Statits',
'sitestats'              => 'Statits {{SITENAME}}',
'userstats'              => 'Gebanastatits',
'sitestatstext'          => "{{PLURAL:\$1|Dabinon pad '''1'''|Dabinons valodo pads '''\$1'''}} in {{SITENAME}}.
Atos ninükon i \"bespikapadis\", padis dö Vükiped it, padis go smalikis (\"sidis\"), lüodükömis, e votikis, kels luveratiko no kanons palelogön as pads ninädilabik.
Atis fakipölo, retons nog {{PLURAL:\$2|pad '''1''', kel luveratiko binon legiko ninädilabik|pads '''\$2''', kels luveratiko binons legiko ninädilabiks}}.

{{PLURAL:\$8|Ragiv '''1''' pelöpükon|Ragivs '''\$8''' pelöpükons}}.

Ejenons valodo {{PLURAL:\$3|padilogam '''1'''|padilogams '''\$3'''}}, e {{PLURAL:\$4|padiredakam '''1'''|padiredakams '''\$4'''}}, sisä vük at pästiton.
Kludo, zänedo ebinons redakams '''\$5'''  a pad, e logams '''\$6''' a redakam.

Lunot [http://www.mediawiki.org/wiki/Manual:Job_queue vobodapoodkeda] binon '''\$7'''.",
'userstatstext'          => "Dabinon{{PLURAL:$1| [[Special:ListUsers|geban]] peregistaröl '''1'''|s [[Special:ListUsers|gebans]] peregistaröl '''$1'''}}; '''$2''' (ü '''$4%''') {{PLURAL:$2|binon|binons}} $5.",
'statistics-mostpopular' => 'Pads suvüno palogöls:',

'disambiguations'      => 'Telplänovapads',
'disambiguationspage'  => 'Template:Telplänov',
'disambiguations-text' => "Pads sököl payümons ad '''telplanövapad'''.
Sötons plao payümon lü yeged pötik.<br />
Pad palelogon telplänovapad if gebon samafomoti, lü kel payümon pad [[MediaWiki:Disambiguationspage]].",

'doubleredirects'     => 'Lüodüköms telik',
'doubleredirectstext' => 'Kedet alik labon yümis lü lüodüköm balid e telid, ed i kedeti balid vödema lüodüköma telid, kel nomiko ninädon padi, ko kel lüodüköm balid söton payümön.',

'brokenredirects'        => 'Lüodüköms dädik',
'brokenredirectstext'    => 'Lüodüköms sököl dugons lü pads no dabinöls:',
'brokenredirects-edit'   => '(redakön)',
'brokenredirects-delete' => '(moükön)',

'withoutinterwiki'         => 'Pads nen yüms bevüpükik',
'withoutinterwiki-summary' => 'Pads sököl no yumons lü fomams in püks votik.',
'withoutinterwiki-legend'  => 'Foyümot',
'withoutinterwiki-submit'  => 'Jonolöd',

'fewestrevisions' => 'Yegeds labü revids nemödikün',

# Miscellaneous special pages
'nbytes'                  => '{{PLURAL:$1|jölät|jöläts}} $1',
'ncategories'             => '{{PLURAL:$1|klad|klads}} $1',
'nlinks'                  => '{{PLURAL:$1|yüm|yüms}} $1',
'nmembers'                => '{{PLURAL:$1|liman|limans}} $1',
'nrevisions'              => '{{PLURAL:$1|fomam|fomams}} $1',
'nviews'                  => '{{PLURAL:$1|logam|logams}} $1',
'specialpage-empty'       => 'Pad at vagon.',
'lonelypages'             => 'Pads, lü kels yüms nonik dugons',
'lonelypagestext'         => 'Pads nonik in vüki at peyümons ad pads sököl.',
'uncategorizedpages'      => 'Pads nen klad',
'uncategorizedcategories' => 'Klads nen klad löpikum',
'uncategorizedimages'     => 'Magods nen klad',
'uncategorizedtemplates'  => 'Samafomots nen klad',
'unusedcategories'        => 'Klads no pageböls',
'unusedimages'            => 'Ragivs no pageböls',
'popularpages'            => 'Pads suvüno pelogöls',
'wantedcategories'        => 'Klads mekabik',
'wantedpages'             => 'Pads mekabik',
'mostlinked'              => 'Pads suvüno peyümöls',
'mostlinkedcategories'    => 'Klads suvüno peyümöls',
'mostlinkedtemplates'     => 'Samafomots suvüno pegeböls',
'mostcategories'          => 'Yegeds labü klads mödikün',
'mostimages'              => 'Magods suvüno peyümöls',
'mostrevisions'           => 'Yegeds suvüno perevidöls',
'prefixindex'             => 'Lised ma foyümots',
'shortpages'              => 'Pads brefik',
'longpages'               => 'Pads lunik',
'deadendpages'            => 'Pads nen yüms lü votiks',
'deadendpagestext'        => 'Pads sököl no labons yümis ad pads votik in vüki at.',
'protectedpages'          => 'Pads pejelöl',
'protectedpages-indef'    => 'Te jels nefümik',
'protectedpagestext'      => 'Pads fovik pejelons e no kanons patöpätükön u paredakön',
'protectedpagesempty'     => 'Pads nonik pejelons',
'protectedtitles'         => 'Tiäds pejelöl',
'protectedtitlestext'     => 'Tiäds sököl no dalons pajafön:',
'protectedtitlesempty'    => 'Tiäds nonik pejelons me paramets at.',
'listusers'               => 'Gebanalised',
'newpages'                => 'Pads nulik',
'newpages-username'       => 'Gebananem:',
'ancientpages'            => 'Pads bäldikün',
'move'                    => 'Topätükön',
'movethispage'            => 'Topätükolöd padi at',
'unusedcategoriestext'    => 'Kladapads sököl dabinons do yeged u klad votik nonik gebon oni.',
'notargettitle'           => 'No dabinon zeilapad',
'notargettext'            => 'No evälol fonätapadi u fonätagebani, keli dun at otefon:',
'pager-newer-n'           => '{{PLURAL:$1|nulikum 1|nulikum $1}}',
'pager-older-n'           => '{{PLURAL:$1|büikum 1|büikum $1}}',
'suppress'                => 'Lovelogam',

# Book sources
'booksources'               => 'Bukafons',
'booksources-search-legend' => 'Sukön bukafonis:',
'booksources-go'            => 'Getolöd',
'booksources-text'          => 'Is palisedons bevüresodatopäds votik, kels selons bukis nulik e pegebölis, e kels ba labons nünis pluik dö buks fa ol pasuköls:',

# Special:Log
'specialloguserlabel'  => 'Geban:',
'speciallogtitlelabel' => 'Tiäd:',
'log'                  => 'Jenotaliseds',
'all-logs-page'        => 'Jenotaliseds valik',
'log-search-legend'    => 'Sukön jenotalisedis',
'log-search-submit'    => 'Maifükön padi',
'alllogstext'          => 'Kobojonam jenotalisedas löpükamas, moükamas, jelodamas, blokamas e guvanas.
Ad brefükam lisedi, kanoy välön lisedasoti, gebananemi, u padi tefik.',
'logempty'             => 'No dabinons notets in jenotalised at.',
'log-title-wildcard'   => 'Sukön tiäds primöl me:',

# Special:AllPages
'allpages'          => 'Pads valik',
'alphaindexline'    => '$1 jü $2',
'nextpage'          => 'Pad sököl ($1)',
'prevpage'          => 'Pad büik ($1)',
'allpagesfrom'      => 'Jonolöd padis, primöl me:',
'allarticles'       => 'Yegeds valik',
'allinnamespace'    => 'Pads valik ($1 nemaspad)',
'allnotinnamespace' => 'Pads valik ($1 nemaspad)',
'allpagesprev'      => 'Büik',
'allpagesnext'      => 'Sököl',
'allpagessubmit'    => 'Jonolöd',
'allpagesprefix'    => 'Jonolöd padis labü foyümot:',
'allpagesbadtitle'  => 'Tiäd pegivöl no lonöfon, u ba labon foyümoti vüpükik u vü-vükik. Mögos i, das labon tonatis u malülis no pedälölis ad penön tiädis.',
'allpages-bad-ns'   => '{{SITENAME}} no labon nemaspadi: "$1".',

# Special:Categories
'categories'                    => 'Klads',
'categoriespagetext'            => 'Klads sököl dabinons in vüki at.',
'special-categories-sort-count' => 'leodükön ma num',
'special-categories-sort-abc'   => 'leodükön ma lafab',

# Special:ListUsers
'listusersfrom'      => 'Jonolöd gebanis primölo me:',
'listusers-submit'   => 'Jonolöd',
'listusers-noresult' => 'Geban nonik petuvon.',

# Special:ListGroupRights
'listgrouprights'          => 'Gitäts gebanagrupa',
'listgrouprights-summary'  => 'Is palisedons gebanagrups in vük at dabinöls, sa gitäts tefik onsik.
Nüns pluik tefü gebanagitäts patuvons [[{{MediaWiki:Listgrouprights-helppage}}|is]].',
'listgrouprights-group'    => 'Grup',
'listgrouprights-rights'   => 'Gitäts',
'listgrouprights-helppage' => 'Help:Grupagitäts',
'listgrouprights-members'  => '(lised limanas)',

# E-mail user
'mailnologin'     => 'Ladet nonik ad sedön',
'mailnologintext' => 'Mutol [[Special:UserLogin|nunädön oli]] e labön ladeti leäktronik lonöföl pö [[Special:Preferences|buükams olik]] ad dalön sedön poti leäktronik gebanes votik.',
'emailuser'       => 'Penön gebane at',
'emailpage'       => 'Penön gebane',
'emailpagetext'   => 'If gebane at egivon ladeti leäktronik lonöföl in gebanabuükams onik,
fomet at osedon one penedi bal. Ladet leäktronik in gebanabuükams olik opubon as fonät (el "De:") peneda at, dat getan okanon gepenön.',
'usermailererror' => 'Potayeg egesedon pöli:',
'defemailsubject' => 'Ladet leäktronik ela {{SITENAME}}',
'noemailtitle'    => 'Ladet no dabinon',
'noemailtext'     => 'Geban at no egivon ladeti leäktronik lonöföl, ud ebuükon ad no getön penedis de gebans votik.',
'emailfrom'       => 'De el',
'emailto'         => 'Ele',
'emailsubject'    => 'Yegäd',
'emailmessage'    => 'Nun',
'emailsend'       => 'Sedolöd',
'emailccme'       => 'Sedolöd obe kopiedi peneda obik.',
'emailccsubject'  => 'Kopied peneda olik ele $1: $2',
'emailsent'       => 'Pened pesedon',
'emailsenttext'   => 'Pened leäktronik ola pesedon.',

# Watchlist
'watchlist'            => 'Galädalised obik',
'mywatchlist'          => 'Galädalised obik',
'watchlistfor'         => "(tefü '''$1''')",
'nowatchlist'          => 'Labol nosi in galädalised olik.',
'watchlistanontext'    => '$1 ad logön u redakön lienis galädaliseda olik',
'watchnologin'         => 'No enunädon oki',
'watchnologintext'     => 'Mutol [[Special:UserLogin|nunädön oli]] büä kanol votükön galädalisedi olik.',
'addedwatch'           => 'Peläüköl lä galädalised',
'addedwatchtext'       => "Pad: \"[[:\$1]]\" peläükon lä [[Special:Watchlist|galädalised]] olik.
Votükams fütürik pada at, äsi bespikapada onik, polisedons us, e pad popenon '''me tonats dagik'''  in [[Special:RecentChanges|lised votükamas nulik]] ad fasilükön tuvi ona.

If vilol poso moükön padi de galädalised olik, välolös lä on knopi: „negalädön“.",
'removedwatch'         => 'Pemoükon de galädalised',
'removedwatchtext'     => 'Pad: „[[:$1]]“ pemoükon se galädalised olik.',
'watch'                => 'Galädön',
'watchthispage'        => 'Galädolöd padi at',
'unwatch'              => 'Negalädön',
'unwatchthispage'      => 'No plu galädön',
'notanarticle'         => 'No binon pad ninädilabik',
'notvisiblerev'        => 'Fomam pemoükon',
'watchnochange'        => 'Nonik padas pagalädöl olik peredakon dü period löpo pejonöl.',
'watchlist-details'    => '{{PLURAL:$1|pad $1|pads $1}} su galädalised, plä bespikapads.',
'wlheader-enotif'      => '* Nunam medü pot leäktronik pemögükon.',
'wlheader-showupdated' => "* Pads pos visit lätik ola pevotüköls papenons '''me tonats bigik'''",
'watchmethod-recent'   => 'vestigam redakamas brefabüik padas galädaliseda',
'watchmethod-list'     => 'vestigam votükamas brefabüik padas galädaliseda',
'watchlistcontains'    => 'Galädalised olik labon {{PLURAL:$1|padi|padis}} $1.',
'iteminvalidname'      => "Fikul tefü el '$1': nem no lonöföl...",
'wlnote'               => "Is palisedons votükam{{PLURAL:$1| lätik|s lätik '''$1'''}} dü düp{{PLURAL:$2| lätik|s lätik '''$2'''}}.",
'wlshowlast'           => 'Jonolöd: düpis lätik $1, delis lätik $2, $3',
'watchlist-show-bots'  => 'Jonolöd redakamis elas bots',
'watchlist-hide-bots'  => 'Klänolöd redakamis elas bots',
'watchlist-show-own'   => 'Jonolöd redakamis obik',
'watchlist-hide-own'   => 'Klänolöd redakamis obik',
'watchlist-show-minor' => 'Jonolöd redakamis pülik',
'watchlist-hide-minor' => 'Klänolöd redakamis pülik',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Papladon ini galädalised...',
'unwatching' => 'Pamoükon se galädalised...',

'enotif_mailer'                => 'Nunamasit ela {{SITENAME}}',
'enotif_newpagetext'           => 'Atos binon pad nulik.',
'enotif_impersonal_salutation' => 'Geban {{SITENAME}}-a',
'changed'                      => 'pevotüköl',
'created'                      => 'pejafon',
'enotif_subject'               => 'In {{SITENAME}}, pad: $PAGETITLE $CHANGEDORCREATED fa el $PAGEEDITOR',
'enotif_lastvisited'           => 'Logolös eli $1 ad tuvön lisedi votükamas valik pos visit lätik ola.',
'enotif_lastdiff'              => 'Logolös eli $1 ad tuvön votükami at.',
'enotif_anon_editor'           => 'geban nennemik: $1',
'enotif_body'                  => 'O $WATCHINGUSERNAME löfik!


Pad: $PAGETITLE in {{SITENAME}} $CHANGEDORCREATED tü $PAGEEDITDATE fa geban: $PAGEEDITOR; otuvol fomami anuik in $PAGETITLE_URL.

$NEWPAGE

Naböfodönuam redakana: $PAGESUMMARY $PAGEMINOREDIT

Kanol penön gebane:
pot leäktronik: $PAGEEDITOR_EMAIL
pad in vük: $PAGEEDITOR_WIKI

Votükams fütürik no ponunons ole if no ovisitol dönu padi at.
Kanol i geükön nunamastänis padas valik galädaliseda olik.

             Nunamasit flenöfik ela {{SITENAME}} olik

--
Ad votükön parametami galädaliseda olik, loglös
{{fullurl:{{ns:special}}:Watchlist/edit}}

Küpets e yuf pluik:
{{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => 'Moükolöd padi',
'confirm'                     => 'Fümedolös',
'excontent'                   => "ninäd äbinon: '$1'",
'excontentauthor'             => "ninäd äbinon: '$1' (e keblünan teik äbinon '[[Special:Contributions/$2|$2]]')",
'exbeforeblank'               => "ninäd bü vagükam äbinon: '$1'",
'exblank'                     => 'pad ävagon',
'delete-confirm'              => 'Moükön padi: "$1"',
'delete-legend'               => 'Moükön',
'historywarning'              => 'Nuned: pad, keli vilol moükön, labon jenotemi:',
'confirmdeletetext'           => 'Primikol ad moükön laidüpiko padi u magodi sa jenotem valik ona. Fümedolös, das desinol ad dunön atosi, das suemol sekis, e das dunol atosi bai [[{{MediaWiki:Policy-url}}]].',
'actioncomplete'              => 'Peledunon',
'deletedtext'                 => 'Pad: "<nowiki>$1</nowiki>" pemoükon;
$2 jonon moükamis nulik.',
'deletedarticle'              => 'Pad: "[[$1]]" pemoükon',
'suppressedarticle'           => 'logov pada: „[[$1]]“ pevotükon',
'dellogpage'                  => 'Jenotalised moükamas',
'dellogpagetext'              => 'Dono binon lised moükamas nulikün.',
'deletionlog'                 => 'jenotalised moükamas',
'reverted'                    => 'Pegeükon ad revid büik',
'deletecomment'               => 'Kod moükama',
'deleteotherreason'           => 'Kod votik:',
'deletereasonotherlist'       => 'Kod votik',
'deletereason-dropdown'       => '* Kods kösömik moükama
** Beg lautana
** Kopiedagitäts
** Vandalim',
'delete-edit-reasonlist'      => 'Redakön kodis moükama',
'delete-toobig'               => 'Pad at labon redakamajenotemi lunik ({{PLURAL:$1|revid|revids}} plu $1).
Moükam padas somik pemiedükon ad vitön däropami pö {{SITENAME}}.',
'delete-warning-toobig'       => 'Pad at labon jenotemi lunik: {{PLURAL:$1|revid|revids}} plu $1.
Prudö! Moükam onik ba osäkädükon jäfidi nünodema: {{SITENAME}}.',
'rollback'                    => 'Sädunön redakamis',
'rollback_short'              => 'Sädunön vali',
'rollbacklink'                => 'sädunön vali',
'rollbackfailed'              => 'Sädunam no eplöpon',
'cantrollback'                => 'Redakam no kanon pasädunön; keblünan lätik binon lautan teik pada at.',
'alreadyrolled'               => 'No eplöpos ad sädunön redakami lätik pada: [[:$1]] fa geban: [[User:$2|$2]] ([[User talk:$2|Bespikapad]]); ek ya eredakon ud esädunon padi at.

Redakam lätik päjenükon fa geban: [[User:$3|$3]] ([[User talk:$3|Bespikapad]]).',
'editcomment'                 => 'Redakamaküpet äbinon: "<i>$1</i>".', # only shown if there is an edit comment
'revertpage'                  => 'Redakams ela [[Special:Contributions/$2|$2]] ([[User talk:$2|Bespik]]) pegeükons; pad labon nu fomami ma redakam lätik ela [[User:$1|$1]]', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => 'Redakams gebana: $1 pesädunons; pad pevotükon ad fomam lätik fa geban: $2.',
'protectlogpage'              => 'Jenotalised jelodamas',
'protectlogtext'              => 'Is palisedons pads pelökofärmüköl e pemaifüköls.
Logolös [[Special:ProtectedPages|lisedi padas pejelöl]], kö pajonons padijelams anu lonöföls.',
'protectedarticle'            => 'ejelon padi: "[[$1]]"',
'modifiedarticleprotection'   => 'evotükon jelanivodi pada: "[[$1]]"',
'unprotectedarticle'          => 'Pad: "[[$1]]" pesäjelon.',
'protect-title'               => 'lonon jelanivodi pada: "$1"',
'protect-legend'              => 'Fümedolös jeli',
'protectcomment'              => 'Küpet:',
'protectexpiry'               => 'Dul:',
'protect_expiry_invalid'      => 'Dul no lonöfon.',
'protect_expiry_old'          => 'Dul ya epasetikon.',
'protect-unchain'             => 'Mögükön dälis ad topätükön',
'protect-text'                => 'Kanol logön e votükön is jelanivodi pada: <strong><nowiki>$1</nowiki></strong>.',
'protect-locked-blocked'      => 'No kanol votükön jelanivodi bi peblokol. Ekö! paramets anuik pada: <strong>$1</strong>:',
'protect-locked-dblock'       => 'Jelanivods no kanons pavotükön sekü lökofärmükam vüka at. Ekö! paramets anuik pada: <strong>$1</strong>:',
'protect-locked-access'       => 'Kal olik no labon däli ad votükön jelanivodi padas.
Ekö! parametem anuik pada: <strong>$1</strong>:',
'protect-cascadeon'           => 'Pad at atimo pajelon bi duton lü {{PLURAL:$1|pad sököl, kel labon|pads sököl, kels labons}} jänajeli jäfidik. Kanol votükön jelanivodi pada at, ab atos no oflunon jänajeli.',
'protect-default'             => '(pebuüköl)',
'protect-fallback'            => 'Däl: "$1" zesüdon',
'protect-level-autoconfirmed' => 'Blokön gebanis no peregistarölis',
'protect-level-sysop'         => 'Te guvans',
'protect-summary-cascade'     => 'as jän',
'protect-expiring'            => 'dul jü $1 (UTC)',
'protect-cascade'             => 'Jelön padis in pad at pekeninükölis (jänajelam)',
'protect-cantedit'            => 'No kanol votükön jelanivodi pada at bi no labol däli ad redakön oni.',
'restriction-type'            => 'Däl:',
'restriction-level'           => 'Miedükamanivod:',
'minimum-size'                => 'Gretot smalikün',
'maximum-size'                => 'Gretot gretikün:',
'pagesize'                    => '(jöläts)',

# Restrictions (nouns)
'restriction-edit'   => 'Redakön',
'restriction-move'   => 'Topätükön',
'restriction-create' => 'Jafön',

# Restriction levels
'restriction-level-sysop'         => 'pejelon lölöfiko',
'restriction-level-autoconfirmed' => 'pejelon dilo',
'restriction-level-all'           => 'nivod alseimik',

# Undelete
'undelete'                     => 'Jonön padis pemoüköl',
'undeletepage'                 => 'Jonön e sädunön padimoükamis',
'undeletepagetitle'            => "'''Sökölos binädon me fomams pemoüköl pada: [[:$1]]'''.",
'viewdeletedpage'              => 'Jonön padis pemoüköl',
'undeletepagetext'             => 'Pads sököl pemoükons ab binons nog in registar: moükam onas kanon pasädunön.
Registar pavagükon periodiko.',
'undeleteextrahelp'            => "Ad sädunön moükami pada lölik, vagükolös bügilis valik e välolös me mugaparat knopi: '''''Sädunolöd moükami'''''. Ad sädunön moükami no lölöfik, välolös me mugaparat bügilis revidas pavipöl, e tän knopi: '''''Sädunolöd moükami'''''. Knop: '''''Vagükolöd vali''''' vagükön küpeti e bügilis valik.",
'undeleterevisions'            => '{{PLURAL:$1|revid 1 peregistaron|revids $1 peregistarons}}',
'undeletehistory'              => 'If osädunol moükami pada at, revids valik ogepubons in jenotem onik.
If pad nulik labü tiäd ot pejafon pos moükam at, revids ogepubons in jenotem pada nulik at, e fomam nuik ona no poplaädon itjäfidiko.',
'undeleterevdel'               => 'Sädunam moükama no poledunon if okodon moükami dila padafomama lätik.
Ön jenets at, nedol sävälön u säklänedön fomamis pemoüköl nulikün.',
'undeletehistorynoadmin'       => 'Yeged at pemoükon. Kod moükama pajonon dono, kobü pats gebanas, kels iredakons padi at büä pämoükon. Vödem redakamas pemoüköl at gebidon te guvanes.',
'undelete-revision'            => 'Pemoükon fomam pada: $1 (dätü $2) pejaföl fa geban: $3:',
'undeleterevision-missing'     => 'Fomam no lonöföl u no dabinöl.
Ba labol yümi dädik, u ba fomam pegepübon u pemoükon se registar.',
'undelete-nodiff'              => 'Fomams büik no petuvons.',
'undeletebtn'                  => 'Sädunön moükami',
'undeletelink'                 => 'sädunön moükami',
'undeletereset'                => 'Vagükolöd vali',
'undeletecomment'              => 'Küpet:',
'undeletedarticle'             => 'Moükam pada: "[[$1]]" pesädunon',
'undeletedrevisions'           => 'Moükam {{PLURAL:$1|revida 1 pesädunon|revidas $1 pesädunons}}',
'undeletedrevisions-files'     => 'Moükam {{PLURAL:$1|revida 1|revidas $1}} e {{PLURAL:$2|ragiva 1|ragivas $2}} pesädunons',
'undeletedfiles'               => 'Moükam {{PLURAL:$1|ragiva 1|ragivas $1}} pesädunon',
'cannotundelete'               => 'Sädunam moükama no eplöpon. Ba ek ya esädunon moükami at.',
'undeletedpage'                => "<big>'''Moükam pada: $1 pesädunon'''</big>

Logolös [[Special:Log/delete|lisedi moükamas]] if vilol kontrolön moükamis e sädunamis brefabüikis.",
'undelete-header'              => 'Logolös [[Special:Log/delete|jenotalisedi moükamas]] ad tuvön padis brefabüo pemoükölis.',
'undelete-search-box'          => 'Sukön padis pemoüköl',
'undelete-search-prefix'       => 'Jonön padis primölo me:',
'undelete-search-submit'       => 'Sukolöd',
'undelete-no-results'          => 'Pads leigöl nonik petuvons in registar moükamas.',
'undelete-cleanup-error'       => 'Pöl dü moükam ragiva no pageböla: "$1".',
'undelete-missing-filearchive' => 'No emögos ad sädunön moükami ragiva: $1 bi no binon in nünodem.
Moükam onik ba ya pesädunon.',
'undelete-error-short'         => 'Pöl dü sädunam moükama ragiva: $1',
'undelete-error-long'          => 'Pöls äjenons dü sädunam moükama ragiva:

$1',

# Namespace form on various pages
'namespace'      => 'Nemaspad:',
'invert'         => 'Güükön väloti',
'blanknamespace' => '(Cifik)',

# Contributions
'contributions' => 'Gebanakeblünots',
'mycontris'     => 'Keblünots obik',
'contribsub2'   => 'Tefü $1 ($2)',
'nocontribs'    => 'Votükams nonik petuvons me paramets at.',
'uctop'         => '(lätik)',
'month'         => 'De mul (e büiks):',
'year'          => 'De yel (e büiks):',

'sp-contributions-newbies'     => 'Jonolöd te keblünotis kalas nulik',
'sp-contributions-newbies-sub' => 'Tefü kals nulik',
'sp-contributions-blocklog'    => 'Jenotalised blokamas',
'sp-contributions-search'      => 'Sukön keblünotis',
'sp-contributions-username'    => 'Ladet-IP u gebananem:',
'sp-contributions-submit'      => 'Suk',

# What links here
'whatlinkshere'            => 'Yüms isio',
'whatlinkshere-title'      => 'Pads ad "$1" yumöls',
'whatlinkshere-page'       => 'Pad:',
'linklistsub'              => '(Yümalised)',
'linkshere'                => "Pads sököl payümons ko '''[[:$1]]''':",
'nolinkshere'              => "Pads nonik peyümons lü '''[[:$1]]'''.",
'nolinkshere-ns'           => "Pads nonik yumons lü pad: '''[[:$1]]''' in nemaspad pevälöl.",
'isredirect'               => 'lüodükömapad',
'istemplate'               => 'ninükam',
'whatlinkshere-prev'       => '{{PLURAL:$1|büik|büik $1}}',
'whatlinkshere-next'       => '{{PLURAL:$1|sököl|sököl $1}}',
'whatlinkshere-links'      => '← yüms',
'whatlinkshere-hideredirs' => '$1 lüodükömis',
'whatlinkshere-hidelinks'  => '$1 yümis',
'whatlinkshere-hideimages' => '$1 yümis magodas',

# Block/unblock
'blockip'                     => 'Blokön gebani',
'blockip-legend'              => 'Blokön gebani',
'blockiptext'                 => 'Gebolös padi at ad blokön redakamagitäti gebananema u ladeta-IP semikas. Atos söton padunön teiko ad vitön vandalimi, e bai [[{{MediaWiki:Policy-url}}|dunalesets {{SITENAME}}]]. Penolös dono kodi patik pro blokam (a. s., mäniotolös padis pedobüköl).',
'ipaddress'                   => 'Ladet-IP',
'ipadressorusername'          => 'Ladet-IP u gebananem',
'ipbexpiry'                   => 'Dü',
'ipbreason'                   => 'Kod',
'ipbreasonotherlist'          => 'Kod votik',
'ipbanononly'                 => 'Blokön te gebanis nen gebananem',
'ipbcreateaccount'            => 'Neletön kalijafi',
'ipbemailban'                 => 'Nemögükön gebane sedi pota leäktronik',
'ipbenableautoblock'          => 'Blokön itjäfidiko ladeti-IP lätik fa geban at pegeböli, äsi ladetis-IP fovik valik, yufü kels osteifülon ad redakön',
'ipbsubmit'                   => 'Blokön gebani at',
'ipbother'                    => 'Dul votik',
'ipboptions'                  => 'düps 2:2 hours,del 1:1 day,dels 3:3 days,vig 1:1 week,vigs 2:2 weeks,mul 1:1 month,muls 3:3 months,muls 6:6 months,yel 1:1 year,laidüp:infinite', # display1:time1,display2:time2,...
'ipbotheroption'              => 'dul votik',
'ipbotherreason'              => 'Kod(s) votik',
'ipbhidename'                 => 'Klänedön gebani u ladeti-IP se jenotalised blokamas, blokamalised anuik e gebanalised',
'badipaddress'                => 'Ladet-IP no lonöfon',
'blockipsuccesssub'           => 'Blokam eplöpon',
'blockipsuccesstext'          => '[[Special:Contributions/$1|$1]] peblokon.
<br />Logolös [[Special:IPBlockList|lisedi ladetas-IP pebloköl]] ad vestigön blokamis.',
'ipb-edit-dropdown'           => 'Redakön kodis blokama',
'ipb-unblock-addr'            => 'Säblokön eli $1',
'ipb-unblock'                 => 'Säblokön gebananemi u ladeti-IP',
'ipb-blocklist-addr'          => 'Logön blokamis dabinöl tefü el $1',
'ipb-blocklist'               => 'Logön blokamis dabinöl',
'unblockip'                   => 'Säblokön gebani',
'unblockiptext'               => 'Gebolös padi at ad gegivön redakamafägi gebane (u ladete-IP) büo pibloköle.',
'ipusubmit'                   => 'Säblokön ladeti at',
'unblocked'                   => '[[User:$1|$1]] pesäblokon',
'unblocked-id'                => 'Blokam: $1 pesädunon',
'ipblocklist'                 => 'Ladets-IP e gebananems pebloköls',
'ipblocklist-legend'          => 'Tuvön gebani pebloköl',
'ipblocklist-username'        => 'Gebananem u ladet IP:',
'ipblocklist-submit'          => 'Suk',
'blocklistline'               => '$1, $2 äblokon $3 ($4)',
'infiniteblock'               => 'laidüpo',
'anononlyblock'               => 'te nennemans',
'noautoblockblock'            => 'Blokam itjäfidik penemögukon',
'createaccountblock'          => 'kalijaf peblokon',
'emailblock'                  => 'ladet leäktronik peblokon',
'ipblocklist-empty'           => 'Blokamalised vagon.',
'ipblocklist-no-results'      => 'Ladet-IP u gebananem peflagöl no peblokon.',
'blocklink'                   => 'blokön',
'unblocklink'                 => 'säblokön',
'contribslink'                => 'keblünots',
'autoblocker'                 => 'Peblokon bi ladet-IP olik pegebon brefabüo fa geban: „[[User:$1|$1]]“. Kod blokama ela $1 binon: „$2“',
'blocklogpage'                => 'Jenotalised blokamas',
'blocklogentry'               => '"[[$1]]" peblokon dü: $2 $3',
'blocklogtext'                => 'Is binon lised gebanablokamas e gebanasäblokamas. Ladets-IP itjäfidiko pebloköls no pajonons. Logolös blokamis e xilis anu lonöfölis in [[Special:IPBlockList|lised IP-blokamas]].',
'unblocklogentry'             => '$1 pesäblokon',
'block-log-flags-anononly'    => 'te gebans nennemik',
'block-log-flags-nocreate'    => 'kalijaf penemögükon',
'block-log-flags-noautoblock' => 'blokam itjäfidik penemögükon',
'block-log-flags-noemail'     => 'ladet leäktronik peblokon',
'range_block_disabled'        => 'Fäg guvana ad jafön ladetemis penemögükon.',
'ipb_expiry_invalid'          => 'Blokamadul no lonöfon.',
'ipb_already_blocked'         => '"$1" ya peblokon',
'ipb_cant_unblock'            => 'Pöl: Bokamadientif: $1 no petuvon. Ba ya pesäblokon.',
'ipb_blocked_as_range'        => 'Pöl: ladet-IP $1 no peblokon stedöfiko e no kanon pasäblokön.
Peblokon ye as dil ladetema: $2, kel kanon pasäblokön.',
'ip_range_invalid'            => 'Ladetem-IP no lonöföl.',
'blockme'                     => 'Blokolöd obi',
'proxyblocker-disabled'       => 'Dun at penemogükon.',
'proxyblocksuccess'           => 'Peledunon.',

# Developer tools
'lockdb'              => 'Lökofärmükön nünodemi',
'unlockdb'            => 'Maifükön nünödemi',
'lockdbtext'          => 'Lökofärmükam nünodema onemogükon gebanes valik redakami padas, votükami buükamas, redakami galädalisedas e dinas votükovik votik in nünodem,
ven ufinükol vobi olik.',
'unlockdbtext'        => 'Maifükmam nünodema omögükon gebanes valik redakami padas, votükami buükamas, redakami galädalisedas e dinas votükovik votik in nünodem.
Fümükolös, begö! das vilol vo dunön atosi.',
'lockconfirm'         => 'Si! Vo vilob lökofärmükön nünodemi.',
'unlockconfirm'       => 'Si! Vo vilob maifükön nünodemi.',
'lockbtn'             => 'Lökofärmükön nünodemi',
'unlockbtn'           => 'Maifükön nünodemi',
'locknoconfirm'       => 'No evälol fümedabokili.',
'lockdbsuccesssub'    => 'Lökofärmükam nünodema eplöpon',
'unlockdbsuccesssub'  => 'Maifükam nünodema eplöpon',
'lockdbsuccesstext'   => 'Nünodem pelökofärmükon.<br />
No glömolös ad [[Special:UnlockDB|maifükön oni]] ven ufinükol vobi olik.',
'unlockdbsuccesstext' => 'Nünodem pemaifükon.',
'lockfilenotwritable' => 'Ragiv lökofärmükamas no votükovon. Ad lökofärmükön u maifükön nünodemi, ragiv at muton binön votükovik (dub dünanünöm).',
'databasenotlocked'   => 'Vük at no pefärmükon.',

# Move page
'move-page'               => 'Topätükön padi: $1',
'move-page-legend'        => 'Topätükolöd padi',
'movepagetext'            => "Me fomet at kanoy votükön padanemi, ottimo feapladölo jenotemi lölöfik ona disi nem nulik. Tiäd büik ovedon lüodüköm lü tiäd nulik. Yüms lü padatiäd büik no povotükons; kontrolös dabini lüodükömas telik u dädikas. Gididol ad garanön, das yüms blebons lüodükön lü pads, lü kels mutons lüodükön.

Küpälolös, das pad '''no''' potopätükon if ya dabinon pad labü tiäd nulik, bisä vagon u binon lüodüköm e no labon jenotemi. Atos sinifon, das, if pölol, nog kanol gepladön padi usio, kö äbinon büo, e das no kanol pladön padi nulik sui pad ya dabinöl.

<b>NUNED!</b>
Votükam at kanon binön mu staböfik ä no paspetöl pö pad pöpedik. Suemolös, begö! gudiko sekis duna at büä ofövol oni.",
'movepagetalktext'        => "Bespikapad tefik potopätükön itjäfidiko kobü pad at '''pläsif:'''
* bespikapad no vägik labü tiäd nulik ya dabinon, u
* vagükol anu bokili dono.

Ön jenets at, if vilol topätükön bespikapadi u balön oni e padi ya dabinöl, ol it omutol dunön osi.",
'movearticle'             => 'Topätükolöd padi',
'movenotallowed'          => 'No dalol topätükön padis.',
'newtitle'                => 'Lü tiäd nulik',
'move-watch'              => 'Pladolöd padi at ini galädalised',
'movepagebtn'             => 'Topätükolöd padi',
'pagemovedsub'            => 'Topätükam eplöpon',
'movepage-moved'          => '<big>\'\'\'"$1" petopätükon lü "$2"\'\'\'</big>', # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => 'Pad labü nem at ya dabinon, u nem fa ol pevälöl no lonöfon.
Välolös nemi votik.',
'cantmove-titleprotected' => 'No kanol topätükön padi bi jafam tiäda nulik at penemögükon.',
'talkexists'              => "'''Pad it petopätükon benosekiko, ab bespikapad onik no petopätükon bi ya dabinon pad labü tiäd ona. Ol it balolös onis.'''",
'movedto'                 => 'petöpätükon lü',
'movetalk'                => 'Topätükolöd bespikapadi tefik',
'movepage-page-moved'     => 'Pad: $1 petopätükon lü $2.',
'1movedto2'               => '[[$1]] petopätükon lü [[$2]]',
'1movedto2_redir'         => '[[$1]] petopätükon lü [[$2]] vegü lüodüköm',
'movelogpage'             => 'Jenotalised topätükamas',
'movelogpagetext'         => 'Is palisedons pads petopätüköl.',
'movereason'              => 'Kod:',
'revertmove'              => 'sädunön',
'delete_and_move'         => 'Moükolöd e topätükolöd',
'delete_and_move_text'    => '==Moükam peflagon==

Yeged nulik "[[:$1]]" ya dabinon. Vilol-li moükön oni ad jafön spadi pro topätükam?',
'delete_and_move_confirm' => 'Si! moükolöd padi',
'delete_and_move_reason'  => 'Pemoükon ad jafön spadi pro topätükam',
'selfmove'                => 'Tiäds nulik e bäldik binons ots; pad no kanon patopätükön sui ok it.',
'immobile_namespace'      => 'Fonät e/u zeil binon padasots patik: no kanoy topätükön padis ini u se nemaspad at.',

# Export
'export'            => 'Seveigön padis',
'exporttext'        => 'Kanol seveigön vödemi e redakajenotemi padi u pademi patädik gebölo eli XML. Kanons poso panüveigön ini vük votik medü el MediaWiki me Patikos:Nüveigön padi.

Ad seveigön padis, penolös tiädis in penamaspad dono, tiädi bal a kedet, e välolös, va vilol fomami anuik kobü fomams büik valik, ko kedets padajenotema, u te fomami anuik kobü nüns dö redakam lätikün.

Ön jenet lätik, kanol i gebön yümi, a.s.: [[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]] pro pad "[[{{MediaWiki:Mainpage}}]]".',
'exportcuronly'     => 'Ninükolöd te revidi anuik, no jenotemi valik',
'exportnohistory'   => "----
'''Noet:''' Seveig padajenotema lölik medü fomet at penemögükon ad gudükumön duinafägi.",
'export-submit'     => 'Seveigolöd',
'export-addcattext' => 'Läükön padis se klad:',
'export-addcat'     => 'Läükön',
'export-download'   => 'Dakipön as ragiv',
'export-templates'  => 'Keninükön samafomotis',

# Namespace 8 related
'allmessages'               => 'Sitanuns',
'allmessagesname'           => 'Nem',
'allmessagesdefault'        => 'Vödem rigädik',
'allmessagescurrent'        => 'Vödem nuik',
'allmessagestext'           => 'Is binon lised sitanunas valik lonöföl in nemaspad: Sitanuns.',
'allmessagesnotsupportedDB' => "Pad at no kanon pagebön bi el '''\$wgUseDatabaseMessages''' penemögükon.",
'allmessagesfilter'         => 'Te nunanems labü:',
'allmessagesmodified'       => 'Jonolöd te pevotükölis',

# Thumbnails
'thumbnail-more'           => 'Gretükön',
'filemissing'              => 'Ragiv deföl',
'thumbnail_error'          => 'Pöl pö jafam magodila: $1',
'thumbnail_invalid_params' => 'Paramets magodila no lonöfons',
'thumbnail_dest_directory' => 'No emögos ad jafön zeilaragiviäri',

# Special:Import
'import'                     => 'Nüveigön padis',
'importinterwiki'            => 'Nüveigam vü vüks',
'import-interwiki-text'      => 'Levälolös vüki e padatiädi ad nüveigön.
Däts fomamas e nems redakanas pokipedons.
Nüveigs vüvükik valik pajonons su [[Special:Log/import|nüveigamalised]].',
'import-interwiki-history'   => 'Kopiedön fomamis valik jenotema pada at',
'import-interwiki-submit'    => 'Nüveigön',
'import-interwiki-namespace' => 'Topätükon padis ini nemaspad:',
'importtext'                 => 'Seveigolös ragivi se fonätavük me [[Special:Export|stum seveiga]].
Dakipolös oni su nünöm olik e löpükolös oni isio.',
'importstart'                => 'Nüveigölo padis...',
'import-revision-count'      => '{{PLURAL:$1|fomam|fomams}} $1',
'importnopages'              => 'Pads nonik ad nüveigön.',
'importfailed'               => 'Nüveigam no eplöpon: <nowiki>$1</nowiki>',
'importunknownsource'        => 'Sot nüveigamafonäta nesevädon',
'importcantopen'             => 'No eplöpos ad maifükön ragivi nüveigabik',
'importbadinterwiki'         => 'Yüm vüvükik dädik',
'importnotext'               => 'Vödem vagik',
'importsuccess'              => 'Nüveigam efinikon!',
'importhistoryconflict'      => 'Dabinon konflit jenotemas (pad at ba ya pänüveigon balna ün paset)',
'importnosources'            => 'Nüveigafonäts vüvükik nonik pelevälons e löpükam stedöfik jenotemas penemögükon.',
'importnofile'               => 'Ragiv nüveigabik nonik pelöpükon.',
'importuploaderrorsize'      => 'Löpükam ragiva nüveigabik no eplöpon. Gretot ragiva pluon demü gretot gretikün pedälöl.',
'importuploaderrorpartial'   => 'Löpükam ragiva nüveigabik no eplöpon. Ragiv pelöpükon te dilo.',
'importuploaderrortemp'      => 'Löpükam ragiva nüveigabik no eplöpon. Ragiviär nelaidüpik nekomon.',
'import-parse-failure'       => 'Pöl pö nüveigam ela XML',
'import-noarticle'           => 'Pad nüveigabik nonik!',
'import-nonewrevisions'      => 'Fomams valik ya pinüveigons.',
'xml-error-string'           => '$1 pö lien: $2, kolum: $3 (jölat: $4): $5',

# Import log
'importlogpage'                    => 'Jenotalised nüveigamas',
'importlogpagetext'                => 'Nüveigam guverik padas labü redakamajenotem se vüks votik',
'import-logentry-upload'           => 'pad: [[$1]] penüveigon medü ragivilöpükam',
'import-logentry-upload-detail'    => '{{PLURAL:$1|fomam|fomams}} $1',
'import-logentry-interwiki'        => 'pevotavükükon: $1',
'import-logentry-interwiki-detail' => '{{PLURAL:$1|fomam|fomams}} $1 se $2',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'Gebanapad obik',
'tooltip-pt-anonuserpage'         => 'Gebanapad ladeta-IP, me kel redakol',
'tooltip-pt-mytalk'               => 'Bespiks obik',
'tooltip-pt-anontalk'             => 'Bespik votükamas me ladet-IP at pejenükölas',
'tooltip-pt-preferences'          => 'Buükams obik',
'tooltip-pt-watchlist'            => 'Lised padas, kö galädol tefü votükams',
'tooltip-pt-mycontris'            => 'Lised keblünotas obik',
'tooltip-pt-login'                => 'Binos gudik, ab no bligik, ad nunädön oyi.',
'tooltip-pt-anonlogin'            => 'Binos gudik - ab no zesüdik - ad nunädön oli.',
'tooltip-pt-logout'               => 'Senunädön oki',
'tooltip-ca-talk'                 => 'Bespik dö ninädapad',
'tooltip-ca-edit'                 => 'Kanol redakön padi at. Gebolös, begö! büologedi bü dakip.',
'tooltip-ca-addsection'           => 'Lüükön küpeti bespike at.',
'tooltip-ca-viewsource'           => 'Pad at pejelon. Kanol logön fonätakoti onik.',
'tooltip-ca-history'              => 'Fomams büik pada at.',
'tooltip-ca-protect'              => 'Jelön padi at',
'tooltip-ca-delete'               => 'Moükön padi at',
'tooltip-ca-undelete'             => 'Gegetön redakamis pada at büä pämoükon',
'tooltip-ca-move'                 => 'Topätükön padi at',
'tooltip-ca-watch'                => 'Lüükolös padi at lü galädalised olik',
'tooltip-ca-unwatch'              => 'Moükön padi at se galädalised olik',
'tooltip-search'                  => 'Sukön in {{SITENAME}}',
'tooltip-search-go'               => 'Tuvön padi labü nem at if dabinon',
'tooltip-search-fulltext'         => 'Sukön vödemi at su pads',
'tooltip-p-logo'                  => 'Cifapad',
'tooltip-n-mainpage'              => 'Visitolös Cifapadi',
'tooltip-n-portal'                => 'Tefü proyek, kio kanol-li dunön, kiplado tuvön dinis',
'tooltip-n-currentevents'         => 'Tuvön nünis valemik tefü jenots anuik',
'tooltip-n-recentchanges'         => 'Lised votükamas nulik in vüki.',
'tooltip-n-randompage'            => 'Lodön padi fädik',
'tooltip-n-help'                  => 'Is kanoy tuvön yufi e nünis.',
'tooltip-t-whatlinkshere'         => 'Lised padas valik, kels yumons isio',
'tooltip-t-recentchangeslinked'   => 'Votükams nulik padas, lü kels pad at yumon',
'tooltip-t-contributions'         => 'Logön keblünotalisedi gebana at',
'tooltip-t-emailuser'             => 'Sedolös penedi gebane at',
'tooltip-t-upload'                => 'Löpükön ragivis',
'tooltip-t-specialpages'          => 'Lised padas patik valik',
'tooltip-t-print'                 => 'Fomam dabükovik pada at',
'tooltip-t-permalink'             => 'Yüm laidüpik lü padafomam at',
'tooltip-ca-nstab-main'           => 'Logön ninädapadi',
'tooltip-ca-nstab-user'           => 'Logön gebanapadi',
'tooltip-ca-nstab-media'          => 'Logön ragivapadi',
'tooltip-ca-nstab-special'        => 'Atos binon pad patik, no kanol redakön oni',
'tooltip-ca-nstab-project'        => 'Logön proyegapadi',
'tooltip-ca-nstab-image'          => 'Logön padi ragiva',
'tooltip-ca-nstab-mediawiki'      => 'Logön sitanuni',
'tooltip-ca-nstab-template'       => 'Logön samafomoti',
'tooltip-ca-nstab-help'           => 'Logön yufapadi',
'tooltip-ca-nstab-category'       => 'Logön kladapadi',
'tooltip-minoredit'               => 'Nemön atosi votükami pülik',
'tooltip-save'                    => 'Dakipolös votükamis olik',
'tooltip-preview'                 => 'Büologed votükamas olik. Gebolös bü dakip, begö!',
'tooltip-diff'                    => 'Jonön votükamis olik in vödem at.',
'tooltip-compareselectedversions' => 'Logön difis vü fomams pevälöl tel pada at.',
'tooltip-watch'                   => 'Lüükön padi at galädalisede olik',
'tooltip-recreate'                => 'Dönujafön padi do ya balna emoükon',
'tooltip-upload'                  => 'Primön löpükami.',

# Stylesheets
'common.css'   => '/** El CSS isio peplädöl pogebon pro padafomäts valik */',
'monobook.css' => '/* El CSS isio pepladöl otefon gebanis padafomäta: Monobook */',

# Scripts
'common.js' => '/* El JavaScript isik alseimik pogebon pro gebans valik pö padilogam valik. */',

# Metadata
'notacceptable' => 'Dünanünömi vüka no fägon ad blünön nünodis ma fomät, keli nünöm olik kanon reidön.',

# Attribution
'anonymous'        => 'Geban(s) nennemik {{SITENAME}}a',
'siteuser'         => 'Geban ela {{SITENAME}}: $1',
'lastmodifiedatby' => 'Pad at pävotükon lätiküno tü dÜp $1, ün $2, fa el $3.', # $1 date, $2 time, $3 user
'othercontribs'    => 'Stabü vob gebana: $1.',
'others'           => 'votiks',
'siteusers'        => 'Geban(s) ela {{SITENAME}}: $1',

# Spam protection
'spam_reverting' => 'Geükön ad fomam lätik, kel no älabon yümis lü $1',

# Info page
'infosubtitle'   => 'Nüns tefü pad',
'numedits'       => 'Redakamanum (pad): $1',
'numtalkedits'   => 'Redakamanum (bespikapad): $1',
'numwatchers'    => 'Num galädanas: $1',
'numauthors'     => 'Num lautanas distik (pad): $1',
'numtalkauthors' => 'Num lautanas distik (bespikapad): $1',

# Math options
'mw_math_png'    => 'Ai el PNG',
'mw_math_simple' => 'El HTML if go balugik, voto eli PNG',
'mw_math_html'   => 'El HTML if mögos, voto eli PNG',
'mw_math_source' => 'Dakipolöd oni as TeX (pro bevüresodatävöms fomätü vödem)',
'mw_math_modern' => 'Pakomandöl pro bevüresodatävöms nulädik',
'mw_math_mathml' => 'El MathML if mögos (nog sperimänt)',

# Patrolling
'markaspatrolleddiff'                 => 'Zepön',
'markaspatrolledtext'                 => 'Zepön padi at',
'markedaspatrolled'                   => 'Pezepon',
'markedaspatrolledtext'               => 'Fomam pevälöl pezepon.',
'rcpatroldisabled'                    => 'Patrul Votükamas Nulik penegebidükon',
'rcpatroldisabledtext'                => 'Patrul Votükamas Nulik binon anu negebidik.',
'markedaspatrollederror'              => 'No kanon pezepön',
'markedaspatrollederrortext'          => 'Nedol välön fomami ad pazepön.',
'markedaspatrollederror-noautopatrol' => 'No dalol zepön votükamis lönik ola.',

# Patrol log
'patrol-log-page' => 'Jenotalised zepamas',
'patrol-log-line' => 'Fomam: $1 pada: $2 pezepon $3',
'patrol-log-auto' => '(itjäfidik)',

# Image deletion
'deletedrevision'                 => 'Fomam büik: $1 pemoükon.',
'filedeleteerror-short'           => 'Pöl pö moükam ragiva: $1',
'filedeleteerror-long'            => 'Pöls petuvons dü moükam ragiva:

$1',
'filedelete-missing'              => 'Ragiv: "$1" no kanon pamoükön bi no dabinon.',
'filedelete-old-unregistered'     => 'Ragivafomam: "$1" no binon in nünodem.',
'filedelete-current-unregistered' => 'Ragiv: "$1" no binon in nünodem.',
'filedelete-archive-read-only'    => 'Ragiviär: "$1" no kanon papenön fa dünanünöm bevuresodik.',

# Browsing diffs
'previousdiff' => '← Dif vönädikum',
'nextdiff'     => 'Dif nulikum →',

# Media information
'mediawarning'         => "'''Nuned''': Ragiv at ba ninükon programi(s) badälik; if ojäfidükol oni, nünömasit olik ba podämükon.<hr />",
'imagemaxsize'         => 'Miedükön magodis su pads magodis bepenöls ad:',
'thumbsize'            => 'Gretot magodüla:',
'widthheightpage'      => '$1×$2, {{PLURAL:$3|pad|pads}} $3',
'file-info'            => '(ragivagretot: $1, MIME-pated: $2)',
'file-info-size'       => '($1 × $2 pixel, ragivagret: $3, pated MIME: $4)',
'file-nohires'         => '<small>Gretot gudikum no pagebidon.</small>',
'svg-long-desc'        => '(ragiv in fomät: SVG, magodaziöbs $1 × $2, gretot: $3)',
'show-big-image'       => 'Gretot gudikün',
'show-big-image-thumb' => '<small>Gretot büologeda at: magodaziöbs $1 × $2</small>',

# Special:NewImages
'newimages'             => 'Pänotem ragivas nulik',
'imagelisttext'         => "Dono binon lised '''$1''' {{PLURAL:$1|ragiva|ragivas}} $2 pedilädölas.",
'newimages-summary'     => 'Pad patik at lisedon ragivis pelöpüköl lätik.',
'showhidebots'          => '($1 mäikamenis)',
'noimages'              => 'Nos ad logön.',
'ilsubmit'              => 'Sukolöd',
'bydate'                => 'ma dät',
'sp-newimages-showfrom' => 'Jonolöd ragivis nulik, primölo tü düp $2, $1',

# Bad image list
'bad_image_list' => 'Fomät pabevobon ön mod soik:

Te lisedaliens (liens me * primöl) pabevobons. Yüm balid liena muton binön yüm ad magod badik. Yüms votik valik su lien ot palelogons as pläams, a.s. pads, in kelas vödems magod dalon pagebön.',

# Metadata
'metadata'          => 'Ragivanüns',
'metadata-help'     => 'Ragiv at keninükon nünis pluik, luveratiko se käm u numatüköm me kel päjafon. If ragiv at ya pevotükon e no plu leigon ko rigädastad okik, mögos, das pats anik is palisedöls no plu bepenons ragivi in stad anuik.',
'metadata-expand'   => 'Jonön patis pluik',
'metadata-collapse' => 'Klänedön patis pluik',
'metadata-fields'   => 'Nünabinets fomäta: EXIF is palisedöls pojonons su bespikapad magoda ifi nünataib pufärmükon. Nünabinets votik poklänedons.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => 'Vidot',
'exif-imagelength'                 => 'Geilot',
'exif-compression'                 => 'Skemat kobopedama',
'exif-stripoffsets'                => 'Topam magodanünodas',
'exif-jpeginterchangeformatlength' => 'Jöläts nünodas: JPEG',
'exif-datetime'                    => 'Dät e tim votükama ragiva',
'exif-imagedescription'            => 'Tiäd magoda',
'exif-software'                    => 'Nünömaprogram pegeböl',
'exif-artist'                      => 'Lautan',
'exif-copyright'                   => 'Dalaban kopiedagitäta',
'exif-exifversion'                 => 'Fomam-Exif',
'exif-colorspace'                  => 'Kölaspad',
'exif-compressedbitsperpixel'      => 'Mod kobopedama magoda',
'exif-pixelydimension'             => 'Magodavidot lonöföl',
'exif-pixelxdimension'             => 'Magodageilot lonöföl',
'exif-usercomment'                 => 'Küpets gebana',
'exif-relatedsoundfile'            => 'Tonaragiv tefik',
'exif-datetimeoriginal'            => 'Dät e tim jafama nünodas',
'exif-datetimedigitized'           => 'Dät e tim numatükama',
'exif-exposuretime-format'         => '$1 sek ($2)',
'exif-fnumber'                     => 'Num-F',
'exif-lightsource'                 => 'Litafonät',
'exif-flash'                       => 'Kämalelit',
'exif-flashenergy'                 => 'Nämet kämalelita',
'exif-filesource'                  => 'Fonät ragiva',
'exif-imageuniqueid'               => 'Magodadientifäd balik',
'exif-gpslatituderef'              => 'Videt Nolüdik u Sulüdik',
'exif-gpslatitude'                 => 'Videt',
'exif-gpslongituderef'             => 'Lunet Lofüdik u Vesüdik',
'exif-gpslongitude'                => 'Lunet',
'exif-gpsaltitude'                 => 'Geilot',
'exif-gpstimestamp'                => 'tim-GPS (glok taumik)',
'exif-gpssatellites'               => 'Muneds pö mafam pegeböls',
'exif-gpsdop'                      => 'Kurat mafama',
'exif-gpsspeedref'                 => 'Vifotastabäd',
'exif-gpstrack'                    => 'Mufalüod',
'exif-gpsimgdirection'             => 'Lüod magoda',
'exif-gpsdestlatitude'             => 'Zeilavidet',
'exif-gpsdestlongitude'            => 'Zeilalunet',
'exif-gpsdestdistance'             => 'Fagot jü lükömöp',
'exif-gpsareainformation'          => 'Nem topäda: GPS',
'exif-gpsdatestamp'                => 'Dät ela GPS',

# EXIF attributes
'exif-compression-1' => 'No pekobopedöl',

'exif-unknowndate' => 'Dät nesevädik',

'exif-orientation-1' => 'Nomik', # 0th row: top; 0th column: left
'exif-orientation-3' => 'Mö 180° pefleköl', # 0th row: bottom; 0th column: right

'exif-componentsconfiguration-0' => 'no dabinon',

'exif-exposureprogram-0' => 'No pemiedetöl',
'exif-exposureprogram-2' => 'Program nomöfik',

'exif-subjectdistance-value' => 'Mets $1',

'exif-meteringmode-0'   => 'Nesevädik',
'exif-meteringmode-1'   => 'Zäned',
'exif-meteringmode-6'   => 'Dilik',
'exif-meteringmode-255' => 'Votik',

'exif-lightsource-0'   => 'Nesevädik',
'exif-lightsource-1'   => 'Delalit',
'exif-lightsource-4'   => 'Kämalelit',
'exif-lightsource-9'   => 'Stom gudik',
'exif-lightsource-10'  => 'Stom lefogagik',
'exif-lightsource-11'  => 'Jad',
'exif-lightsource-255' => 'Litafonät votik',

'exif-focalplaneresolutionunit-2' => 'puids',

'exif-sensingmethod-1' => 'No pemiedetöl',

'exif-scenecapturetype-2' => 'Pöträt',
'exif-scenecapturetype-3' => 'Ün neit',

'exif-gaincontrol-0' => 'Nonik',

'exif-contrast-0' => 'Nomik',

'exif-saturation-0' => 'Nomik',

'exif-sharpness-0' => 'Nomik',

'exif-subjectdistancerange-0' => 'Nesevädik',
'exif-subjectdistancerange-2' => 'Loged nilik',
'exif-subjectdistancerange-3' => 'Loged fägik',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'Videt nolüdik',
'exif-gpslatitude-s' => 'Videt  Sulüdik',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'lunet lofüdik',
'exif-gpslongitude-w' => 'lunet vesüdik',

'exif-gpsstatus-a' => 'Mafam padunon',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'Milmets a düp',
'exif-gpsspeed-m' => 'Liöls a düp',
'exif-gpsspeed-n' => 'Snobs',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => 'Lüod veratik',
'exif-gpsdirection-m' => 'Lüod magnetik',

# External editor support
'edit-externally'      => 'Votükön ragivi at me nünömaprogram plödik',
'edit-externally-help' => 'Reidolös eli [http://www.mediawiki.org/wiki/Manual:External_editors setup instructions] (in Linglänapük) ad tuvön nünis pluik.',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'valik',
'imagelistall'     => 'valik',
'watchlistall2'    => 'valikis',
'namespacesall'    => 'valik',
'monthsall'        => 'valik',

# E-mail address confirmation
'confirmemail'             => 'Fümedolös ladeti leäktronik',
'confirmemail_noemail'     => 'No labol ladeti leäktronik lonöföl in [[Special:Preferences|gebanabuükams olik]].',
'confirmemail_text'        => 'Vük at flagon, das ofümedol ladeti leäktronik ola büä odälon ole ad gebön poti leäktronik.
Välolös me mugaparat knopi dono ad sedön fümedapenedi ladete olik. Pened oninädon yümi labü fümedakot; sökolös yümi ad fümedön, das ladet olik lonöfon.',
'confirmemail_pending'     => '<div class="error">Fümedakot ya pesedon ole medü pot leäktronik; if äjafol kali olik brefabüo, stebedolös dü minuts anik, dat fümedakot olükömon, büä osteifülol ad begön koti nulik.</div>',
'confirmemail_send'        => 'Sedön fümedakoti me pot leäktronik',
'confirmemail_sent'        => 'Fümedapened pesedon.',
'confirmemail_oncreate'    => 'Fümedakot pesedon lü ladet leäktronik ola. Kot at no zesüdon ad nunädön oli, ab omutol klavön oni büä okanol gebön ladeti leäktronik ola in vük at.',
'confirmemail_sendfailed'  => 'No eplöpos ad sedön fümedapenedi. Ba ädabinons malats no lonöföls in ladet.

Potanünöm egesedon: $1',
'confirmemail_invalid'     => 'Fümedakot no lonöfon. Jiniko binon tu bäldik.',
'confirmemail_needlogin'   => 'Nedol $1 ad fümedön ladeti leäktronik ola.',
'confirmemail_success'     => 'Ladet leäktronik ola pefümedon. Nu kanol nunädön oli e juitön vüki at.',
'confirmemail_loggedin'    => 'Ladeti leäktronik ola nu pefümedon.',
'confirmemail_error'       => 'Bos no eplöpon pö registaram fümedama olik.',
'confirmemail_subject'     => 'Fümedam ladeta leäktronik pro: {{SITENAME}}',
'confirmemail_body'        => 'Ek, bo ol, se ladet-IP: $1, ejafon kali: „$2‟ me ladeti leäktronik at lä {{SITENAME}}.

Ad fümedön, das kal at binon jenöfiko olik, ed ad dalön gebön
poti leäktronik in {{SITENAME}}, sökolös yümi fovik me bevüresodatävöm olik:

$3

If *no* binol utan, kel ejafon kali, sökolös yümi fovik ad sädunön fümedami leäktronik:

$5

Fümedakot at operon lonöfi okik ün $4.',
'confirmemail_invalidated' => 'Fümedam ladeta leäktronik penegebidükon',
'invalidateemail'          => 'Negebidükon fümedami ladeta leäktronik',

# Scary transclusion
'scarytranscludetoolong' => '[el URL binon tu lunik]',

# Trackbacks
'trackbackremove' => ' ([$1 Moükön])',

# Delete conflict
'deletedwhileediting' => 'Nuned: Pad at pemoükon posä äprimol ad redakön oni!',
'confirmrecreate'     => "Geban: [[User:$1|$1]] ([[User talk:$1|talk]]) ämoükon padi at posä äprimol ad redakön oni sekü kod sököl:
: ''$2''
Fümedolös, das jenöfo vilol dönujafön padi at.",
'recreate'            => 'Dönujafön',

# HTML dump
'redirectingto' => 'Lüodükölo lü: [[:$1]]...',

# action=purge
'confirm_purge'        => 'Vagükön eli caché pada at?

$1',
'confirm_purge_button' => 'Si!',

# AJAX search
'searchcontaining' => "Sukön padis labü ''$1''.",
'searchnamed'      => "Sukön padis tiädü ''$1''.",
'articletitles'    => "Yegeds me ''$1'' primöls",
'hideresults'      => 'Klänedön sekis',
'useajaxsearch'    => 'Gebön suki ela AJAX',

# Multipage image navigation
'imgmultipageprev' => '← pad büik',
'imgmultipagenext' => 'pad sököl →',
'imgmultigo'       => 'Gololöd!',

# Table pager
'ascending_abbrev'         => 'löpio',
'descending_abbrev'        => 'donio',
'table_pager_next'         => 'Pad sököl',
'table_pager_prev'         => 'Pad büik',
'table_pager_first'        => 'Pad balid',
'table_pager_last'         => 'Pad lätik',
'table_pager_limit'        => 'Jonön lienis $1 a pad',
'table_pager_limit_submit' => 'Gololöd',
'table_pager_empty'        => 'Seks nonik',

# Auto-summaries
'autosumm-blank'   => 'Ninäd valik pemoükon se pad',
'autosumm-replace' => "Pad pepläadon me '$1'",
'autoredircomment' => 'Lüodükon lü [[$1]]',
'autosumm-new'     => 'Pad nulik: $1',

# Live preview
'livepreview-loading' => 'Pabelodon…',
'livepreview-ready'   => 'Pabelodon… Efinikon!',
'livepreview-failed'  => 'Büologed vifik no eplöpon! Gebolös büologedi kösömik.',
'livepreview-error'   => 'Yümätam no eplöpon: $1 „$2“. Steifülolös me büologed kösömik.',

# Friendlier slave lag warnings
'lag-warn-normal' => 'Votükams ün {{PLURAL:$1|sekun|sekuns}} lätik $1 ba no polisedons is.',

# Watchlist editor
'watchlistedit-numitems'       => 'Galädalised olik labon {{PLURAL:$1|tiädi bal|tiädis $1}}, fakipü bespikapads.',
'watchlistedit-noitems'        => 'Galädalised olik keninükon tiädis nonik.',
'watchlistedit-normal-title'   => 'Redakön galädalisedi',
'watchlistedit-normal-legend'  => 'Moükön tiädis se galädalised',
'watchlistedit-normal-explain' => 'Tiäds galädalised olik palisedons dono. Ad moükön tiädi, välolös bugili nilü on e klikolös: Moükön Tiädis. Kanol i [[Special:Watchlist/raw|redakön lisedafonäti]].',
'watchlistedit-normal-submit'  => 'Moükön Tiädis',
'watchlistedit-normal-done'    => '{{PLURAL:$1|tiäd bal pemoükon|tiäds $1 pemoükons}} se galädalised olik:',
'watchlistedit-raw-title'      => 'Redakön fonäti galädaliseda',
'watchlistedit-raw-legend'     => 'Redakön fonäti galädaliseda',
'watchlistedit-raw-explain'    => 'Tiäds galädaliseda olik pajonons dono, e kanons paredakön - paläükön u pamoükön se lised (ai tiäd bal a lien). Pos redakam, klikolös Votükön Galädalisedi.
Kanol i [[Special:Watchlist/edit|gebön redakametodi kösömik]].',
'watchlistedit-raw-titles'     => 'Tiäds:',
'watchlistedit-raw-submit'     => 'Votükön Galädalisedi',
'watchlistedit-raw-done'       => 'Galädalised olik pevotükon.',
'watchlistedit-raw-added'      => '{{PLURAL:$1|Tiäd bal peläükon|Tiäds $1 peläükons}}:',
'watchlistedit-raw-removed'    => '{{PLURAL:$1|Tiäd bal pemoükon|Tiäds $1 pemoükons}}:',

# Watchlist editing tools
'watchlisttools-view' => 'Logön votükamis teföl',
'watchlisttools-edit' => 'Logön e redakön galädalisedi',
'watchlisttools-raw'  => 'Redakön galädalisedi nen fomät',

# Special:Version
'version'                  => 'Fomam', # Not used as normal message but as header for the special page itself
'version-specialpages'     => 'Pads patik',
'version-other'            => 'Votik',
'version-version'          => 'Fomam',
'version-license'          => 'Dälazöt',
'version-software-product' => 'Prodäd',
'version-software-version' => 'Fomam',

# Special:FilePath
'filepath'         => 'Ragivaluveg',
'filepath-page'    => 'Ragiv:',
'filepath-submit'  => 'Luveg',
'filepath-summary' => 'Pad patik at tuvon luvegi lölöfik ragiva. Magods pajonons ma fomät gudikün, ragivasots votik pamaifükons stedöfo kobü programs onsik.

Penolös ragivanemi nen foyümot: „{{ns:image}}:“',

# Special:FileDuplicateSearch
'fileduplicatesearch-filename' => 'Ragivanem:',
'fileduplicatesearch-submit'   => 'Sukön',
'fileduplicatesearch-info'     => 'pixels $1 × $2 <br />Ragivagretot: $3<br />MIME-sot: $4',

# Special:SpecialPages
'specialpages'               => 'Pads patik',
'specialpages-group-other'   => 'Pads patik votik',
'specialpages-group-login'   => 'Nunädön oki / jafön kali',
'specialpages-group-changes' => 'Votükams nulik e jenotaliseds',
'specialpages-group-users'   => 'Gebans e gitäts',
'specialpages-group-highuse' => 'Pads suvo pegeböls',
'specialpages-group-pages'   => 'Padalised',

# Special:BlankPage
'blankpage' => 'Pad vagik',

);
