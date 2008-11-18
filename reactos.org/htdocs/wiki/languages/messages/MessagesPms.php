<?php
/** Piedmontese (Piemontèis)
 *
 * @ingroup Language
 * @file
 *
 * @author Bèrto 'd Sèra
 * @author SabineCretella
 * @author Teak
 * @author Ævar Arnfjörð Bjarmason <avarab@gmail.com>, Jens Frank
 * @author לערי ריינהארט
 */

$fallback = 'it';

$namespaceNames = array(
	NS_MEDIA            => 'Media',
	NS_SPECIAL          => 'Special',
	NS_MAIN             => '',
	NS_TALK             => 'Discussion',
	NS_USER             => 'Utent',
	NS_USER_TALK        => 'Ciaciarade',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK     => 'Discussion_ant_sla_$1',
	NS_IMAGE            => 'Figura',
	NS_IMAGE_TALK       => 'Discussion_dla_figura',
	NS_MEDIAWIKI        => 'MediaWiki',
	NS_MEDIAWIKI_TALK   => 'Discussion_dla_MediaWiki',
	NS_TEMPLATE         => 'Stamp',
	NS_TEMPLATE_TALK    => 'Discussion_dlë_stamp',
	NS_HELP             => 'Agiut',
	NS_HELP_TALK        => 'Discussion_ant_sl\'agiut',
	NS_CATEGORY         => 'Categorìa',
	NS_CATEGORY_TALK    => 'Discussion_ant_sla_categorìa'
);

$messages = array(
# User preference toggles
'tog-underline'               => 'Anliure con la sotliniadura',
'tog-highlightbroken'         => "Buta an evidensa j'anliure che a men-o a<br />
dj'artìcol ancó pa scrit",
'tog-justify'                 => 'Paràgraf: giustificà',
'tog-hideminor'               => 'Stërma le modifiche cite<br />ant sla pàgina "Ùltime Modìfiche"',
'tog-extendwatchlist'         => 'Slarga la funsion "ten sot euj" an manera che a la smon-a tute le modìfiche che as peulo fesse',
'tog-usenewrc'                => 'Ùltime modìfiche an bela forma (a-i va Javascript)',
'tog-numberheadings'          => 'Tìtoj ëd paràgraf<br />che as nùmero daspërlor',
'tog-showtoolbar'             => "Mostra la bara dj'utiss (a-i va Javascript)",
'tog-editondblclick'          => "Dobia sgnacà për modifiché l'artìcol<br />(a-i va JavaScript)",
'tog-editsection'             => "Abìlita la modìfica dle session con j'anliure [modìfica]",
'tog-editsectiononrightclick' => 'Abilité la modìfica dle session ën sgnacand-je ansima<br />  al tìtol col tast drit dël rat (a-i va Javascript)',
'tog-showtoc'                 => "Buta le tàole dij contnù<br />(për j'artìcoj che l'han pì che 3 session)",
'tog-rememberpassword'        => "Vis-te la ciav<br />(nen mach për na session<br />- a l'ha da manca dij cookies)",
'tog-editwidth'               => 'Quàder ëd modìfica slargà<br />al màssim',
'tog-watchcreations'          => 'Gionta le pàgine che i creo mi a la lista ëd lòn che im ten-o sot euj',
'tog-watchdefault'            => "Notìfica dj'articoli neuv e ëd coj modificà",
'tog-watchmoves'              => 'Gionta le pàgine che i tramudo a lòn che im ten-o sot euj',
'tog-watchdeletion'           => 'Gionta le pàgine che i scancelo via a la lista ëd lòn che im ten-o sot euj',
'tog-minordefault'            => 'Marca tute le modìfica coma cite<br />(mach coma predefinission dla casela)',
'tog-previewontop'            => 'Smon-e la preuva dzora al quàder ëd modìfica dël test e nen sota',
'tog-previewonfirst'          => 'Smon na preuva la prima vira che as fa na modìfica',
'tog-nocache'                 => "Dòvra pa la memorisassion ''cache'' për le pàgine",
'tog-enotifwatchlistpages'    => 'Mand-me un messagi an pòsta eletrònica quand a-i son dle modìfiche a le pàgine',
'tog-enotifusertalkpages'     => 'Mand-me un messagi ëd pòsta eletrònica quand a-i son dle modìfiche a mia pàgina dle ciaciarade',
'tog-enotifminoredits'        => 'Mand-me un messagi an pòsta bele che për le modìfiche cite',
'tog-enotifrevealaddr'        => 'Lassa che a së s-ciàira mia adrëssa ëd pòsta eletrònica ant ij messagi ëd notìfica',
'tog-shownumberswatching'     => "Smon ël nùmer d'utent che as ten-o la pàgina sot euj",
'tog-fancysig'                => "Modìfica mai la firma da coma a l'é ambelessì (as dòvra për fesse na firma fòra stàndard)",
'tog-externaleditor'          => "Dòvra coma stàndard n'editor ëd test estern",
'tog-externaldiff'            => 'Dòvra për stàndard un programa "diff" estern',
'tog-showjumplinks'           => 'Dòvra j\'anliure d\'acessibilità dla sòrt "Va a"',
'tog-uselivepreview'          => "Dòvra la funsion ''Preuva dal viv'' (a-i va JavaScript e a l'é mach sperimental)",
'tog-forceeditsummary'        => "Ciama conferma se ël somari dla modìfica a l'é veujd",
'tog-watchlisthideown'        => 'Stërma mie modìfiche ant la ròba che im ten-o sot euj',
'tog-watchlisthidebots'       => 'Stërma le modìfiche faite daj trigomiro ant la lista dle ròbe che im ten-o sot euj',
'tog-watchlisthideminor'      => "Scond le modìfiche cite da 'nt lòn che im ten-o sot euj",
'tog-nolangconversion'        => 'Fërma la conversion antra variant lenghìstiche',
'tog-ccmeonemails'            => "Mand-me còpia dij messagi ëd pòsta eletrònica che i-j mando a j'àotri utent",
'tog-diffonly'                => 'Smon pa ël contnù dla pàgina dapress a le diferense',
'tog-showhiddencats'          => 'Smon le categorìe stërmà',

'underline-always'  => 'Sempe',
'underline-never'   => 'Mai',
'underline-default' => 'Dòvra lë stàndard dël programma ëd navigassion (browser)',

'skinpreview' => '(Preuva)',

# Dates
'sunday'        => 'Dumìnica',
'monday'        => 'Lun-es',
'tuesday'       => 'Martes',
'wednesday'     => 'Merco',
'thursday'      => 'Giòbia',
'friday'        => 'Vënner',
'saturday'      => 'Saba',
'sun'           => 'Domi',
'mon'           => 'Lun-',
'tue'           => 'Mart',
'wed'           => 'Mër',
'thu'           => 'Giòb',
'fri'           => 'Vënn',
'sat'           => 'Saba',
'january'       => 'Gené',
'february'      => 'Fërvé',
'march'         => 'Mars',
'april'         => 'Avril',
'may_long'      => 'Magg',
'june'          => 'Giugn',
'july'          => 'Luj',
'august'        => 'Aost',
'september'     => 'Stémber',
'october'       => 'Otóber',
'november'      => 'Novémber',
'december'      => 'Dzémber',
'january-gen'   => 'Gené',
'february-gen'  => 'Fërvé',
'march-gen'     => 'Mars',
'april-gen'     => 'Avril',
'may-gen'       => 'Magg',
'june-gen'      => 'Giugn',
'july-gen'      => 'Luj',
'august-gen'    => 'Aost',
'september-gen' => 'Stémber',
'october-gen'   => 'Otóber',
'november-gen'  => 'Novémber',
'december-gen'  => 'Dzémber',
'jan'           => 'Gen',
'feb'           => 'Fër',
'mar'           => 'Mar',
'apr'           => 'Avr',
'may'           => 'Mag',
'jun'           => 'Giu',
'jul'           => 'Luj',
'aug'           => 'Aos',
'sep'           => 'Ste',
'oct'           => 'Oto',
'nov'           => 'Nov',
'dec'           => 'Dze',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|Categorìa|Categorìe}}',
'category_header'                => 'Artìcoj ant la categorìa "$1"',
'subcategories'                  => 'Sotacategorìe',
'category-media-header'          => 'Archivi ant la categorìa "$1"',
'category-empty'                 => "''Al dì d'ancheuj la categorìa a l'ha pa andrinta nì 'd pàgine, nì d'archivi multimojen.''",
'hidden-categories'              => '{{PLURAL:$1|Categorìa stërmà|Categorìe stërmà}}',
'hidden-category-category'       => 'Categorìe stërmà', # Name of the category where hidden categories will be listed
'category-subcat-count'          => "{{PLURAL:$2|Sta categorìa-sì a l'ha mach na sot-categorìa, listà ambelessì sota.|Sta categorìa-sì a l'ha {{PLURAL:$1|na sot-categorìa|$1 sot-categorìe}}, ëd $2 ch'a-i në j'é an total.}}",
'category-subcat-count-limited'  => "Sta categorìa-sì a l'ha {{PLURAL:$1|la sot-categorìa|le $1 la sot-categorìe}} lista ambelessì sota.",
'category-article-count'         => "{{PLURAL:$2|Sta categorìa-sì a l'ha mach sta pàgina.|Ant sta categorìa-sì a-i {{PLURAL:$1|intra mach sta pàgina|intro $1 pàgine}} ëd $2 ch'a-i në j'é an total.}}",
'category-article-count-limited' => 'Ant sta categorìa-sì a-i {{PLURAL:$1|resta mach sta pàgina|resto $1 pàgine}}.',
'category-file-count'            => "{{PLURAL:$2|Sta categorìa-sì a l'ha nomach st'archivi.|Sta categorìa-sì a l'ha {{PLURAL:$1|n'|$1}} archivi, ëd $2 ch'a-i në j'é an total.}}",
'category-file-count-limited'    => "Ant sta categorìa-sì a-i {{PLURAL:$1|intra mach st'|intro $1}} archivi.",
'listingcontinuesabbrev'         => ' anans',

'mainpagetext'      => "<big>'''MediaWiki a l'é staita anstalà a la perfession.'''</big>",
'mainpagedocfooter' => "Che a varda la [http://meta.wikimedia.org/wiki/MediaWiki_User%27s_Guide User's Guide] (belavans për adess a-i é mach n'anglèis) për avej dj'anformassion suplementar ant sël coma dovré ël programa dla wiki.

== Për anandiesse a travajé ==

* [http://www.mediawiki.org/wiki/Manual:Configuration_settings Configuration settings list]
* [http://www.mediawiki.org/wiki/Manual:FAQ MediaWiki FAQ]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce MediaWiki release mailing list]",

'about'          => 'A propòsit ëd',
'article'        => 'Pàgina ëd contnù',
'newwindow'      => '(as deurb ant na fnestra neuva)',
'cancel'         => 'Scancela',
'qbfind'         => 'Treuva',
'qbbrowse'       => 'Sfeuja',
'qbedit'         => 'Modìfica',
'qbpageoptions'  => 'Opsion dla pàgina',
'qbpageinfo'     => 'Informassioni rësguard a la pagina',
'qbmyoptions'    => 'Mie opsion',
'qbspecialpages' => 'Pàgine speciaj',
'moredotdotdot'  => 'Dë pì...',
'mypage'         => 'Mia pàgina',
'mytalk'         => 'Mie ciaciarade',
'anontalk'       => "Ciaciarade për st'adrëssa IP-sì",
'navigation'     => 'Navigassion',
'and'            => 'e',

# Metadata in edit box
'metadata_help' => 'Metadat:',

'errorpagetitle'    => 'Eror',
'returnto'          => 'Torna andré a $1.',
'tagline'           => 'Da {{SITENAME}}.',
'help'              => 'Agiut',
'search'            => 'Sërca',
'searchbutton'      => 'Sërca',
'go'                => 'Va',
'searcharticle'     => 'Va',
'history'           => 'Version pì veje',
'history_short'     => 'Stòria',
'updatedmarker'     => "Agiornà da 'nt l'ùltima vira che i son passà",
'info_short'        => 'Anformassion',
'printableversion'  => 'Version bon-a për stampé',
'permalink'         => 'Anliura fissa',
'print'             => 'Stampa',
'edit'              => 'Modìfica',
'create'            => 'Creé',
'editthispage'      => "Modìfica st'artìcol-sì",
'create-this-page'  => 'Creé sta pàgina',
'delete'            => 'Scancela',
'deletethispage'    => 'Scancela pàgina',
'undelete_short'    => 'Disdëscancela {{PLURAL:$1|na modìfica|$1 modìfiche}}',
'protect'           => 'Protegg',
'protect_change'    => 'modìfica protession',
'protectthispage'   => 'Protegg sta pàgina-sì',
'unprotect'         => 'Gava la protession',
'unprotectthispage' => 'Gava via la protession',
'newpage'           => 'Pàgina neuva',
'talkpage'          => 'Discussion',
'talkpagelinktext'  => 'discussion',
'specialpage'       => 'Pàgina Special',
'personaltools'     => 'Utiss personaj',
'postcomment'       => 'Gionta un coment',
'articlepage'       => "Che a varda l'articol",
'talk'              => 'Discussion',
'views'             => 'vìsite',
'toolbox'           => 'utiss',
'userpage'          => 'Che a varda la pàgina Utent',
'projectpage'       => 'Che a varda la pàgina ëd servissi',
'imagepage'         => 'Pàgina dla figura',
'mediawikipage'     => 'Mostra ël messagi',
'templatepage'      => 'Mostra lë stamp',
'viewhelppage'      => "Smon la pàgina d'agiut",
'categorypage'      => 'Fa vëdde la categorìa',
'viewtalkpage'      => 'Vardé la discussion',
'otherlanguages'    => 'Àutre lenghe',
'redirectedfrom'    => '(Ridiression da $1)',
'redirectpagesub'   => 'Pàgina ëd ridiression',
'lastmodifiedat'    => "Modificà l'ùltima vira al $2, $1.", # $1 date, $2 time
'viewcount'         => "St'artìcol-sì a l'é stait lesù {{PLURAL:$1|na vira|$1 vire}}.",
'protectedpage'     => 'Pàgina proteta',
'jumpto'            => 'Va a:',
'jumptonavigation'  => 'navigassion',
'jumptosearch'      => 'arsërca',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'A propòsit ëd {{SITENAME}}',
'aboutpage'            => 'Project:A propòsit',
'bugreports'           => 'Malfunsionament',
'bugreportspage'       => 'Project:Malfonsionament',
'copyright'            => 'Ël contnù a resta disponibil sota a na licensa $1.',
'copyrightpagename'    => "Drit d'autor ëd {{SITENAME}}",
'copyrightpage'        => "{{ns:project}}:Drit d'autor",
'currentevents'        => 'Neuve',
'currentevents-url'    => 'Project:Neuve',
'disclaimers'          => 'Difide',
'disclaimerpage'       => 'Project:Avertense generaj',
'edithelp'             => 'Manual dë spiegassion',
'edithelppage'         => "Help:Coma scrive n'artìcol",
'faq'                  => 'FAQ',
'faqpage'              => 'Project:Soèns An Ciamo',
'helppage'             => 'Help:Agiut',
'mainpage'             => 'Intrada',
'mainpage-description' => 'Intrada',
'policy-url'           => 'Project:Deuit',
'portal'               => 'Piòla',
'portal-url'           => 'Project:Piòla',
'privacy'              => 'Polìtica ëd confindensialità',
'privacypage'          => 'Project:Polìtica ëd confidensialità',

'badaccess'        => 'Përmess nen giust',
'badaccess-group0' => "A l'ha pa ij përmess dont a fa dë manca për fé st'operassion-sì.",
'badaccess-group1' => "Costa funsion-sì a l'é riservà a j'utent dla partìa $1.",
'badaccess-group2' => "Costa funsion-sì a l'é riservà a j'utent dle partìe $1.",
'badaccess-groups' => "Costa funsion-sì a l'é riservà a j'utent che a sio almanch ant un-a dle partìe: $1.",

'versionrequired'     => 'A-i va për fòrsa la version $1 ëd MediaWiki',
'versionrequiredtext' => 'Për dovrè sta pàgina-sì a-i va la version $1 dël programa MediaWiki. Che a varda [[Special:Version]]',

'ok'                      => 'Va bin',
'retrievedfrom'           => 'Pijait da  "$1"',
'youhavenewmessages'      => "A l'ha $1 ($2).",
'newmessageslink'         => 'ëd messagi neuv',
'newmessagesdifflink'     => "A-i é chèich-còs ëd diferent da 'nt l'ùltima revision",
'youhavenewmessagesmulti' => "A l'ha dij neuv messagi an $1",
'editsection'             => 'modìfica',
'editold'                 => 'modìfica',
'editsectionhint'         => 'I soma dapress a modifiché la session: $1',
'toc'                     => 'Contnù',
'showtoc'                 => 'smon',
'hidetoc'                 => 'stërma',
'thisisdeleted'           => 'Veul-lo vardé ò ripristiné $1?',
'viewdeleted'             => 'Veul-lo vardé $1?',
'restorelink'             => '{{PLURAL:$1|na modìfica scancelà|$1 modìfiche scancelà}}',
'feedlinks'               => 'Fluss:',
'feed-invalid'            => 'Modalità ëd sotoscrission dël fluss nen vàlida.',
'feed-unavailable'        => 'Ij fluss ëd neuve a son nen disponìbij ansima a {{SITENAME}}',
'site-rss-feed'           => 'Emission RSS $1',
'site-atom-feed'          => 'Emission Atom $1',
'page-rss-feed'           => 'Emission RSS "$1"',
'page-atom-feed'          => 'Emission Atom "$1"',
'red-link-title'          => "$1 (pàgina ch'a-i é anco' pa)",

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Artìcol',
'nstab-user'      => "Pàgina dl'utent",
'nstab-media'     => 'Pàgina multimedial',
'nstab-special'   => 'Special',
'nstab-project'   => 'Pàgina ëd servissi',
'nstab-image'     => 'Figura',
'nstab-mediawiki' => 'Messagi',
'nstab-template'  => 'Stamp',
'nstab-help'      => 'Agiut',
'nstab-category'  => 'Categorìa',

# Main script and global functions
'nosuchaction'      => 'Operassione nen arconossùa',
'nosuchactiontext'  => "L'operassion che a l'ha ciamà a l'é nen arconossùa dal programa MediaWiki",
'nosuchspecialpage' => "A-i é pa gnun-a pàgina special tan-me cola che chiel a l'ha ciamà.",
'nospecialpagetext' => "A l'ha ciamà na pàgina special che a l'é pa staita arconossùa dal programa MediaWiki, ò pura a l'é nen disponibila.",

# General errors
'error'                => 'Eror',
'databaseerror'        => 'Eror ant la base dat',
'dberrortext'          => 'Eror ëd sintassi ant la domanda mandà a la base dat.
L\'ùltima domanda mandà a la base dat a l\'é staita:
<blockquote><tt>$1</tt></blockquote>
da \'nt la funsion "<tt>$2</tt>".
MySQL a l\'ha dane andré n\'eror "<tt>$3: $4</tt>".',
'dberrortextcl'        => 'A-i é staje n\'eror ant la sintassi d\'anterogassion dla base dat.
L\'ùltima anterogassion a l\'é staita:
"$1"
da andrinta a la funsion "$2".
MySQL a l\'ha dane n\'eror "$3: $4"',
'noconnect'            => 'Conession a la base dat falà ansima a $1',
'nodb'                 => 'Selession da la base dat $1 falìa',
'cachederror'          => "Costa a l'é mach na còpia memorisà dla pàgina che a l'ha ciamà, donca a podrìa ëdcò nen esse agiornà.",
'laggedslavemode'      => 'Avis: la pàgina a podrìa ëdcò nen mostré tute soe modìfiche.',
'readonly'             => 'Acess a la base dat sërà për chèich temp.',
'enterlockreason'      => 'Che a buta na rason për ël blocagi, con andrinta data e ora ëd quand che a stima che a sarà gavà.',
'readonlytext'         => "La base dat ëd {{SITENAME}} për adess a l'é blocà, e as peulo pa fesse nì dle neuve imission, nì dle modìfiche, con tute le probabilità për n'operassion ëd manutension dël server. Se a l'é parej motobin ampressa la base a sarà torna doèrta.<br />
L'aministrator che a l'ha blocala a l'ha lassà sto messagi-sì:
<p>:$1",
'readonly_lag'         => "La base dat a l'é staita blocà n'automàtich antramentr che che le màchine dël circuito secondari (slave) as buto an pari con cole dël prinsipal (master)",
'internalerror'        => 'Eror intern',
'internalerror_info'   => 'Eror antern: $1',
'filecopyerror'        => 'A l\'é pa stait possibil copié l\'archivi "$1" coma "$2".',
'filerenameerror'      => 'A l\'é pa podusse cangeje nòm a l\'archivi "$1" an "$2".',
'filedeleteerror'      => 'A l\'é pa podusse scancelé l\'archivi "$1".',
'directorycreateerror' => 'A l\'é pa podusse fé ël dossié "$1".',
'filenotfound'         => ' A l\'é pa trovasse l\'archivi "$1".',
'fileexistserror'      => 'As peul pa scriv-se l\'archivi "$1": a-i é già',
'unexpected'           => 'Valor che i së spitavo pa: "$1"="$2".',
'formerror'            => "Eror: la domanda a l'é staita mandà mal",
'badarticleerror'      => "N'operassion parej as peul pa fesse ansima a sta pàgina-sì.",
'cannotdelete'         => "As peul pa scancelesse la pàgina, l'archivi ò la figura che a veul scancelé.",
'badtitle'             => 'Tìtol nen giust',
'badtitletext'         => "La pàgina che a l'ha ciamà a peul pa esse mostrà. A podrìa tratesse ëd na pàgina nen bon-a, veujda, ò pura a podrìa ëdcò esse n'eror ant n'anliura antra lenghe diferente ò tra diferente version ëd {{SITENAME}}.",
'perfdisabled'         => "An dëspias, ma costa funsion a l'é nen disponibila ant j'ore ëd pì gran acess a la base dat, për nen ralenté l'acess dj'Utent!<br />Che a preuva torna antra 2 bot e 4 ore dòp mesdì (UTC).<br /><br />Mersì.",
'perfcached'           => "Sòn a l'é stait memorisà an local e podrìa ëdcò nen esse agiornà:",
'perfcachedts'         => "Lòn che a-j ven dapress a sossì a l'é pijait da 'nt na còpia local \"cache\" dla base dat. L'ùltim agiornament a l'é dël: \$1.",
'querypage-no-updates' => "J'agiornament për sta pàgina-sì për adess a travajo nen. Ij dat ambelessì a saran nen rinfrescà.",
'wrong_wfQuery_params' => 'Paràmetro nen giust për wfQuery()<br />
Funsion: $1<br />
Query: $2',
'viewsource'           => 'Vardé la sorgiss',
'viewsourcefor'        => 'ëd $1',
'actionthrottled'      => 'Assion frenà',
'actionthrottledtext'  => "Për evité che gent ò màchine an carìo dla rumenta, st'assion-sì as peul nen fesse tròp ëd soèns, e chiel/chila a l'ha arpetula tròpe vire. Ch'a sia gentil, ch'a preuva torna antra dontre minute.",
'protectedpagetext'    => "Sta pàgina-sì a l'è staita blocà për evité che a-j faso dle modìfiche.",
'viewsourcetext'       => 'A peul vardé e copié la sorgiss dë sta pàgina:',
'protectedinterface'   => "Costa pàgina-sì a l'ha andrinta un chèich-còs che a fa part d'antërfacia dël programa che a dòvro tùit; donca a l'é proteta për evité che a-i rivo dle ròbe brute.",
'editinginterface'     => "'''Dosman!''' A l'é dapress ch'a-i travaja ansima a na pàgina ch'as dòvra për generé ël test dl'antërfacia dël programa. Le modìfiche fàite ambelessì a-j bogio l'antërfacia a tuti j'utent. Se sò but a l'é col ëd fé na tradussion, për piasì ch'a considerà la possibilità dë dovré [http://translatewiki.net/wiki/Main_Page?setlang=pms Betawiki], ël proget ëd localisassion ëd MediaWiki.",
'sqlhidden'            => "(l'anterogassion SQL a l'é stërmà)",
'cascadeprotected'     => 'Ant sta pàgina-sì as peulo pa fé ëd modìfiche, përché a-i intra ant {{PLURAL:$1|la pàgina|le pàgine}} butà sot a protession con la fonsion "a tombé" viscà ansima a: $2',
'namespaceprotected'   => "A l'ha nen ël përmess dë feje dle modìfiche a le pàgine dlë spassi nominal '''$1'''.",
'customcssjsprotected' => "Ch'a varda ch'a l'ha pa ël përmess ëd modifiché sta pàgina-sì, për via ch'a l'ha andrinta ij gust ëd n'àotr utent.",
'ns-specialprotected'  => 'As peulo nen modifichesse le pàgine dlë spassi nominal {{ns:special}}.',
'titleprotected'       => "La creassion ëd pàgine con ës tìtol-sì a l'é stàita proibìa da [[User:$1|$1]].
Coma rason a l'ha butà: ''$2''.",

# Login and logout pages
'logouttitle'                => "Seurte da 'nt ël sistema",
'logouttext'                 => "A l'é sortù da 'nt ël sistema.
A peul tiré anans a dovré {{SITENAME}} coma Utent anonim, ò pura a peul rintré torna ant ël sistema con l'istess stranòm che a dovrava prima, ò con un diferent.",
'welcomecreation'            => '<h2>Bin avnù, $1!</h2><p>Sò cont a l\'è stait creà.<br />Mersì për avej sërnù dë giutene a fé chërse {{SITENAME}}.<br />Për fé {{SITENAME}} pì soa, e përchè a sia pì belfé dovrela, che as dësmentia nen dë compilé la pàgina dij "sò gust".',
'loginpagetitle'             => 'rintré ant ël sistema',
'yourname'                   => 'Sò stranòm',
'yourpassword'               => 'Soa ciav',
'yourpasswordagain'          => 'Che a bata torna soa ciav',
'remembermypassword'         => "Vis-te mia ciav për vàire session (për podej felo a fa da manca che un a l'abia ij ''cookies'' abilità).",
'yourdomainname'             => 'Sò domini',
'externaldberror'            => "Ò che a l'é rivaje n'eror d'autenticassion esterna, ò pura a l'é chiel (chila) che a l'é nen autorisà a agiornesse sò cont estern.",
'loginproblem'               => "<b>A l'é staje n'eror dëmentré che as provava a rintré ant ël sistema.</b><br />
Che a preuva n'àutra vira, miraco che sta vira a andèissa mai bin!",
'login'                      => 'Rintré ant ël sistema',
'nav-login-createaccount'    => 'rintré ant ël sistema',
'loginprompt'                => 'Che a varda mach che a venta avej ij cookies abilità për podej rintré an {{SITENAME}}.',
'userlogin'                  => 'rintré ant ël sistema',
'logout'                     => "Seurte da 'nt ël sistema",
'userlogout'                 => 'seurte dal sistema',
'notloggedin'                => "a l'é pa ant ël sistema",
'nologin'                    => 'Ha-lo ancó nen sò cont? $1.',
'nologinlink'                => 'creésse un cont.',
'createaccount'              => 'Crea un cont neuv',
'gotaccount'                 => 'Ha-lo già un sò cont? $1.',
'gotaccountlink'             => 'Rintré ant ël sistema',
'createaccountmail'          => 'për pòsta eletrònica',
'badretype'                  => "Le doe ciav che a l'ha scrivù a resto diferente antra lor, e a venta che a sio mideme.",
'userexists'                 => "An dëspias.<br />Lë stranòm che a l'ha sërnusse a l'é già dovrà da n'àutr Utent.<br />
Për son i-j ciamoma dë sërn-se në stranòm diferent.",
'youremail'                  => 'Soa adrëssa ëd pòsta eletrònica',
'username'                   => 'Stranòm:',
'uid'                        => "ID dl'utent:",
'yourrealname'               => 'Nòm vèir *',
'yourlanguage'               => 'Lenga:',
'yournick'                   => 'Sò stranòm (për firmé)',
'badsig'                     => "Soa forma a l'é nen giusta, che a controla le istrussion HTML.",
'badsiglength'               => 'Stranòm esagerà longh; a dev esse pì curt che $1 caràter.',
'email'                      => 'pòsta eletrònica',
'prefs-help-realname'        => '* Nòm vèir (opsional): se i sërne da butelo ambelessì a sarà dovrà për deve mérit ëd vòstr travaj.',
'loginerror'                 => 'Eror ën rintrand ant ël sistema',
'prefs-help-email'           => "* Adrëssa ëd pòsta eletrònica (opsional): ën butandlo i feve an manera che la gent a peula contateve passand për vòstra pàgina dle ciaciarade sensa dë manca che a sapia chi i seve e che adrëssa che i l'eve.",
'prefs-help-email-required'  => "A-i va l'adrëssa ëd pòsta eletrònica.",
'nocookiesnew'               => "Sò cont a l'é doèrt, ma chiel (ò chila) a l'ha nen podù rintré ant ël sistema.
{{SITENAME}} a dòvra ij cookies për fé rintré la gent ant sò sistema. Belavans chiel a l'ha pa ij cookies abilità.
Për piasì, che as j'abìlita e peuj che a preuva torna a rintré con sò stranòm e soa ciav.",
'nocookieslogin'             => "{{SITENAME}} a dòvra ij cookies për fé rintré la gent ant sò sistema. Belavans chiel a l'ha pa ij cookies abilità. Pëër piasì, che a j'abìlita e peuj che a preuva torna.",
'noname'                     => "Lë stranòm che a l'ha batù as peul pa dovresse, as peul nen creésse un cont Utent con ës nòm-sì.",
'loginsuccesstitle'          => "Compliment! A l'é pen-a rintrà ant ël sistema. A-i é pa staje gnun eror.",
'loginsuccess'               => 'A l\'ha avù ël përmess ëd conession al server ëd {{SITENAME}} con lë stranòm utent ëd "$1".',
'nosuchuser'                 => 'Atension<br /><br /> dapress a na verìfica, a n\'arsulta pa gnin Utent che a l\'abia stranòm "$1".<br /><br />
Për piasì, che a contròla ël nòm che a l\'ha batù, ò pura che a dòvra la domanda ambelessì sota për fé un cont Utent neuv.',
'nosuchusershort'            => 'A-i é pa gnun utent che as ciama "<nowiki>$1</nowiki>". Për piasì, che a contròla se a l\'ha scrit tut giust.',
'nouserspecified'            => 'A venta che a specìfica në stranòm utent',
'wrongpassword'              => "La ciav batùa a l'é pa giusta.<br /><br />Che a preuva torna, për piasì.",
'wrongpasswordempty'         => "A l'ha butà na ciav veujda. Për piasì, che a preuva torna.",
'passwordtooshort'           => "Soa ciav a l'é pa assé longa.
A la dev avej almanch $1 caràter.",
'mailmypassword'             => 'Mandme na neuva ciav con un messagi ëd pòsta eletrònica',
'passwordremindertitle'      => 'Servissi për visé la paròla ciav ëd {{SITENAME}}',
'passwordremindertext'       => 'Cheidun (a l\'é belfé che a sia stait pròpe chiel, da \'nt l\'adrëssa IP $1)
a l\'ha ciamà che i-j mandèisso na neuva paròla ciav për rintré ant ël sistema ëd {{SITENAME}} ($4).
La ciav për l\'Utent "$2" adess a resta "$3".
Për rason ëd sicurëssa, a sarìa mej che chiel a la dovrèissa për rintré ant ël sistema pì ampressa che a peul, e che tut sùbit as la cambièissa con un-a che a sern daspërchiel.',
'noemail'                    => 'An arsulta pa gnun-a casela ëd pòsta eletrònica për l\'Utent "$1".',
'passwordsent'               => "Na neuva paròla ciav a l'é staita mandà a l'adrëssa eletrònica registrà për l'Utent \"\$1\".
Për piasì, che a la dòvra sùbit për rintré ant ël sistema pen-a che a l'arsèiv.",
'blocked-mailpassword'       => "Për evité dj'assion nen corete as peul pa dovresse la funsion \"Mand-me na ciav neuva\" da 'nt n'adrëssa IP ëd cole blocà.",
'eauthentsent'               => "A l'adrëssa che a l'ha dane i l'oma mandaje un messagi ëd pòsta eletrònica për conferma.
Anans che qualsëssìa àutr messagi ëd pòsta a ven-a mandà a 's cont-sì, a venta che a a fasa coma che a-j diso dë fé ant ël messagi, për confermé che ës cont a l'é da bon sò.",
'throttled-mailpassword'     => 'Na ciav neuva a l\'é gia staita mandà da manch che $1 ore. Për evité dij dovré nen regolar, la funsion "Mand-me na ciav neuva" as peul dovresse mach vira $1 ore.',
'mailerror'                  => 'Eror ën mandand via un messagi ëd pòsta eletrònica: $1',
'acct_creation_throttle_hit' => "Darmagi, ma chiel (chila) a l'ha già creasse $1 cont. A peul pa pì deurb-ne dj'àutri.",
'emailauthenticated'         => "Soa adrëssa ëd pòsta eletrònica a l'é staita autenticà ël $1.",
'emailnotauthenticated'      => "Soa adrëssa ëd pòsta eletrònica a l'é ancó pa staita autenticà.
Da qualsëssìa ëd coste funsion a sarà mandà gnun messagi fin che chiel (chila) a s'auténtica nen.",
'noemailprefs'               => "<strong>Che a specìfica n'adrëssa ëd pòsta eletrònica se a veul dovré coste funsion-sì.</strong>",
'emailconfirmlink'           => 'Che an conferma sa adrëssa ëd pòsta eletrònica',
'invalidemailaddress'        => "Costa adrëssa ëd pòsta eletrònica-sì as peul nen pijesse përchè a l'ha na forma nen bon-a.
Për piasì che a buta n'adrëssa scrita giusta ò che a lassa ël camp veujd.",
'accountcreated'             => 'Cont creà',
'accountcreatedtext'         => "Ël cont Utent për $1 a l'é stait creà.",
'createaccount-title'        => 'Creassion ëd cont për {{SITENAME}}',
'createaccount-text'         => 'Cheidun a l\'ha dorbù un cont për st\'adrëssa ëd pòsta eletrònica-sì ansima a {{SITENAME}} ($4) butand da stranòm "$2" e da ciav "$3". A dovrìa rintré ant ël sistema e cambiesse soa ciav pì ampressa ch\'a peul.

Se sòn a l\'é rivà për eror, a peul lassé sté e fe gnente sensa problema.',
'loginlanguagelabel'         => 'Lenga: $1',

# Password reset dialog
'resetpass'               => 'Buté la ciav a sò valor për sòlit',
'resetpass_announce'      => "A l'é rintrà ant ël sistema con na ciav provisòria mandà via për pòsta eletrònica. Për podej finì la procedura a l'ha da butesse na ciav neuva ambelessì:",
'resetpass_text'          => '<!-- Gionté dël test ambelessì -->',
'resetpass_header'        => 'Buta ël valor për sòlit',
'resetpass_submit'        => 'Registra la ciav e rintra ant ël sistema',
'resetpass_success'       => "Soa ciav a l'é staita registrà sensa problema. I soma dapress a rintré ant ël sistema...",
'resetpass_bad_temporary' => "Ciav provisòria nen bon-a. A peul esse che a l'abia già cambiasse soa ciav, ò pura che a l'abia ciamà na ciav provisòria neuva.",
'resetpass_forbidden'     => 'An {{SITENAME}} le ciav as peulo pa cambiesse.',
'resetpass_missing'       => "Ël mòdulo a l'avìa gnun dat andrinta (ò pura a son përdusse për la stra).",

# Edit page toolbar
'bold_sample'     => 'Test an grassèt',
'bold_tip'        => 'Test an grassèt',
'italic_sample'   => 'Test an corsiv',
'italic_tip'      => 'Test an corsiv',
'link_sample'     => "Tìtol dl'anliura",
'link_tip'        => 'Anliura interna',
'extlink_sample'  => "http://www.example.com tìtol dl'anliura",
'extlink_tip'     => 'Anliura esterna (che as visa dë buté ël prefiss http://)',
'headline_sample' => "Antestassion dl'artìcol",
'headline_tip'    => 'Antestassion dë scond livel',
'math_sample'     => 'Che a buta la fòrmula ambelessì',
'math_tip'        => 'Fòrmula matemàtica (LaTeX)',
'nowiki_sample'   => 'Che a buta ël test nen formatà ambelessì',
'nowiki_tip'      => 'Lassé un tòch ëd test fòra dla formatassion dla wiki',
'image_sample'    => 'Esempi.jpg',
'image_tip'       => 'Figura anglobà ant ël test',
'media_sample'    => 'Esempi.ogg',
'media_tip'       => "Anliura a n'archivi multimedial",
'sig_tip'         => 'Firma butand data e ora',
'hr_tip'          => 'Riga orisontal (da dovresse nen tròp soèns)',

# Edit pages
'summary'                   => 'Somari',
'subject'                   => 'Sogèt',
'minoredit'                 => "Costa-sì a l'è na modìfica cita",
'watchthis'                 => "Ten sot euj st'artìcol-sì",
'savearticle'               => 'Salva sta pàgina',
'preview'                   => 'Preuva',
'showpreview'               => 'Mostra na preuva',
'showlivepreview'           => "Funsion ''Preuva dal viv''",
'showdiff'                  => 'Smon-me le modìfiche',
'anoneditwarning'           => "A l'é ancó nen rintrà ant ël sistema. Soa adrëssa IP a sarà registrà ant la stòria dle modìfiche dë sta pàgina-sì.",
'missingsummary'            => "'''Nòta:''' a l'ha pa butà gnun somari dla modìfica. Se a sgnaca Salva n'àutra vira, soa modìfica a resterà salvà sensa pa ëd somari.",
'missingcommenttext'        => 'Për piasì che a buta un coment ambelessì sota.',
'missingcommentheader'      => "'''A l'euj!:''' ës coment-sì a l'é sensa intestassion. Se a sgnaca torna \"Salva sta pàgina\" soa modìfica a sarà salvà sensa gnun-a intestassion.",
'summary-preview'           => "Preuva dl'oget",
'subject-preview'           => "Preuva d'oget/intestassion",
'blockedtitle'              => "Belavans cost ëstranòm-sì a resta col ëd n'utent che a l'é stait disabilità a fé 'd modìfiche a j'articoj.",
'blockedtext'               => "<big>'''Sò stranòm ò pura adrëssa IP a l'é stait blocà.'''</big>

Ël blòcagi a l'é stait fait da \$1. Coma rason a l'ha butà ''\$2''.
* Blocà a parte dal: \$8
* Fin al: \$6<br />
* As veul blochesse: \$7
A peul butesse an contact con \$1 ò pura n'àotr [[{{MediaWiki:Grouppage-sysop}}|aministrator]] për discute ëd sò blocagi.
Ch'a ten-a present ch'a podrà dovré la fonsion \"mandeje un messagi ëd pòsta a l'utent\" mach s'a l'ha specificà n'adrëssa ëd pòsta vàlida ant
[[Special:Preferences|sò gust]] e se sta fonsion a l'é nen ëstàita blocà 'cò chila.
Soa adrëssa IP corenta a l'é \$3, e l'identificativ dël blocagi a l'é #\$5. Për piasì, ch'a-j buta tut e doj ant soe comunicassion ant sta question-sì.",
'autoblockedtext'           => "Soa adrëssa IP a l'è staita blocà n'aotomàtich ën essend ch'a l'era dovrà da n'àotr utent, che a l'é stait blocà da \$1.
La rason butà për ël blòch a l'é:''\$2''
* Ël blòch a part dël: \$8
* A va a la fin dël: \$6

A peul contaté \$1 ò pura n'àotr dj'
[[{{MediaWiki:Grouppage-sysop}}|aministrator]] për discute d'ës blòch.

Ch'a varda mach ch'a peul nen dovré l'opsion ëd \"mandeje un messagi a l'utent\" se a l'ha nen n'adrëssa ëd pòsta eletrònica registra e verificà ant [[Special:Preferences|sò gust]].
Sò nùmer ëd blòch a l'é \$5. Për piasì, ës nùmer-sì ch'a lo buta sempe an tute le comunicassion andova ch'as parla ëd sò blòch.",
'blockednoreason'           => "a l'han pa butà gnun-a rason",
'blockedoriginalsource'     => "La sorgiss ëd '''$1''' a së s-ciàira ambelessì sota:",
'blockededitsource'         => "Ël test ëd le '''soe modìfiche''' a '''$1''' a së s-ciàira ambelessì sota:",
'whitelistedittitle'        => 'Sòn as peul pa fesse nen rintrand ant ël sistema',
'whitelistedittext'         => 'A venta $1 për podej fé dle modìfiche.',
'confirmedittitle'          => "Confermé l'adrëssa postal për podej fé dle modìfiche",
'confirmedittext'           => 'A dev confermé soa adrëssa ëd pòsta eletrònica, anans che modifiché dle pàgine. Për piasì, che a convalida soa adrëssa ën dovrand la pàgina [[Special:Preferences|mè gust]].',
'nosuchsectiontitle'        => 'Pa gnun-a session parèj',
'nosuchsectiontext'         => "A l'ha provasse a modifichè na session ch'a-i é pa. Ën essend che la session $1 a-i é nen, a-i é pa gnanca andova ch'as peula salvesse soa modìfica.",
'loginreqtitle'             => 'a venta rintré ant ël sistema',
'loginreqlink'              => 'rintré ant ël sistema',
'loginreqpagetext'          => "Che a pòrta passiensa, ma a dev $1 për podej vëdde dj'àutre pàgine.",
'accmailtitle'              => 'Ciav spedìa.',
'accmailtext'               => 'La paròla ciav për "$1" a l\'é staita mandà a $2.',
'newarticle'                => '(Neuv)',
'newarticletext'            => 'Che a scriva sò test ambelessì.',
'anontalkpagetext'          => "----''Costa a l'é la pàgina ëd ciaciarade che a s-ciàira n'utent anònim che a l'é ancó pa dorbusse un cont, ò pura che a lo dòvra nen. Nen savend chi che a sia chiel (chila) i l'oma da dovré ël nùmer IP address për deje n'identificassion. Belavans, ës nùmer-sì a peul esse dovrà da vàire Utent. J'Utent anònim che a l'han l'impression d'arsèive dij coment sensa sust a dovrìo [[Special:UserLogin|creésse sò cont ò pura rintré ant ël sistema]] për evité dë fé confusion con dj'àutri Utent che a peulo avej l'istess nùmer IP.''",
'noarticletext'             => "(St'artìcol-sì a l'é veujd, a podrìa për gentilëssa anandielo chiel, ò pura ciamé la scancelassion dë sta pàgina)",
'userpage-userdoesnotexist' => 'Lë stranòm "$1" a l\'é pa registrà. Për piasì ch\'a varda se da bon a veul creé/modifiché sta pàgina.',
'clearyourcache'            => "'''Nòta:''' na vira che i l'ha salvà, a peul esse che a-j fasa da manca da passé via la memorisassion (cache) dël sò programa ëd navigassion (browser) për podej ës-ciairé le modìfiche.
*'''Mozilla / Firefox / Safari:''' Che a ten-a sgnacà ''Shift'' antramentr che a sgnaca col rat ansima a ''Reload'', ò pura che a sgnaca tut ansema ''Ctrl-Shift-R'' (''Cmd-Shift-R'' ansima a j'Apple Mac);
*'''IE:''' che a ten-a sgnacà ''Ctrl'' antramentr che a sgnaca col rat ansima a ''Refresh'', ò pura che a sgnaca tut ansema ''Ctrl-F5'';
*'''Konqueror:''': a basta mach sgnaché ël boton ''Reload'', ò pura sgnaché ''F5'';
*'''Opera''' j'utent a peulo avej da manca dë veujdé 'd continùo soa memorisassion (cache) andrinta a ''Tools&rarr;Preferences''.",
'usercssjsyoucanpreview'    => "<strong>Drita:</strong> che a dòvra ël boton 'Mostra na preuva' për controlé l'efet ëd sò còdes CSS/JS anans che salvelo.",
'usercsspreview'            => "'''Che a varda che a lòn che a s-ciàira a l'é nomach na preuva ëd sò CSS, che salvà a resta ancó nen!'''",
'userjspreview'             => "'''Che as visa che a l'é mach antramentr che as fa na preuva ëd sò Javascript, che a l'é ancó pa stait salvà!'''",
'userinvalidcssjstitle'     => "'''Avis:''' A-i é pa gnun-a facia \"\$1\". Che as visa che le pàgine .css e .js che un as fa daspërchiel a dòvro tute minùscole për tìtol, pr'esempi {{ns:user}}:Scaramacaj/monobook.css nopà che {{ns:user}}:Scaramacaj/Monobook.css.",
'updated'                   => '(Agiornà)',
'note'                      => '<strong>NÒTA:</strong>',
'previewnote'               => "<strong>Che a ten-a mach present che costa-sì a l'é nomach na PREUVA, e che soa version a l'é ancó pa staita salvà!</strong>",
'previewconflict'           => "Costa preuva a-j mostra ël test dl'articol ambelessì dzora. Se a sërn dë salvelo, a l'é parej che a lo s-ciaireran ëdcò tuti j'àutri Utent.",
'session_fail_preview'      => "<strong>Darmagi! I l'oma pa podù processé soa modìfica per via che a son përdusse për la stra ij dat ëd session.
Për piasì che a preuva n'àutra vira. Se a dovèissa mai torna riveje sossì, che a preuva a seurte dal sistema e peuj torna a rintré.</strong>",
'session_fail_preview_html' => "<strong>Darmagi! I l'oma nen podù processé soa modìfica ën essend che a son përdusse për la stra ij dat ëd session.</strong>

''Për via che {{SITENAME}} a lassa mostré còdes HTML nen filtrà, la preuva a ven ëstarmà coma precaussion contra a dij possibij atach fait an Javascript.''

<strong>Se sòn a l'èra na modìfica normal, për piasì che a preuva a fela n'àutra vira. Se a dovèissa mai torna deje dle gran-e, che a preuva a seurte da 'nt ël sistema e peuj torna a rintré.</strong>",
'token_suffix_mismatch'     => "<strong>Soa modìfica a l'é nen staita acetà përché sò navigator a l'hai fait ciadel con ij pont e le vìrgole
ant ël quàder ëd modìfica. La rason che a l'é nen stait acetà a l'r për evité ch'a-i fasa darmagi al
test ch'a-i é già. Sossì dle vire a riva quand un a dòvra un programa proxy ëd coj un pòch dla Bajòna.</strong>",
'editing'                   => 'Modìfica ëd $1',
'editingsection'            => 'I soma dapress a modifiché $1 (session)',
'editingcomment'            => 'I soma dapress a modifiché $1 (coment)',
'editconflict'              => "Conflit d'edission: $1",
'explainconflict'           => "Cheidun d'àutr a l'ha salvà soa version dl'artìcol antramentré che chiel (chila) as prontava la soa.<br />
Ël quàder ëd modìfica dë dzora a mostra ël test ëd l'articol coma a resta adess (visadì, lòn che a-i é ant sla Ragnà). Soe modìfiche a stan ant ël quàder dë sota.
Ën volend a peul gionté soe modìfiche ant ël quàder dë dzora.
<b>Mach</b> ël test ant ël quàder dë dzora a sarà salvà, ën sgnacand ël boton \"Salva\".<br />",
'yourtext'                  => 'Sò test',
'storedversion'             => 'Version memorisà',
'nonunicodebrowser'         => "<strong>A L'EUJ! Sò programa ëd navigassion (browser) a travaja pa giust con lë stàndard unicode. I soma obligà a dovré dij truschin përchè a peula salvesse sò artìcoj sensa problema: ij caràter che a son nen ASCII a jë s-ciairerà ant ël quàder ëd modìfica test coma còdes esadecimaj.</strong>",
'editingold'                => "<strong>CHE A FASA MACH ATENSION: che a sta fasend-je dle modìfiche a na version nen agiornà dl'artìcol.<br />
Se a la salva parej, lòn che a l'era stait fait dapress a sta revision-sì as përderà d'autut.</strong>",
'yourdiff'                  => 'Diferense',
'copyrightwarning'          => "Che a ten-a për piasì present che tute le contribussion a {{SITENAME}} as considero daite sota a na licensa ëd la sòrt $2 (che a varda $1 për avej pì 'd detaj).
Se a veul nen che sò test a peula esse modificà e distribuì da qualsëssìa person-a sensa gnun-a limitassion ëd gnun-a sòrt, che a lo buta pa ansima a {{SITENAME}}, ma pitòst che as lo pùblica ansima a un sò sit personal.<br />
Ën mandand ës test-sì chiel (chila) as fa garant sota soa responsabilità che ël test a l'ha scrivusslo despërchiel (daspërchila) coma original, ò pura che a l'ha tracopialo da na sorgiss ëd pùblich domini, ò da n'àutra sorgiss dla midema sòrt, ò pura che chiel (chila) a l'ha arseivù autorisassion scrita a dovré sto test e che sòn a peul dimostrelo.<br />
<strong>DOVRÉ PA MAI DËL MATERIAL COERTÀ DA DRIT D'AUTOR (c) SENSA AVEJ N'AUTORISASSION SCRITA PËR FELO!!!</strong>",
'copyrightwarning2'         => "Për piasì, che a ten-a present che tute le contribussion a {{SITENAME}} a peulo esse modificà ò scancelà da dj'àutri contributor. Se a veul nen che lòn che a scriv a ven-a modificà sensa limitassion ëd gnun-a sòrt, che a lo manda nen ambelessì.<br />
Ant l'istess temp, ën mandand dël material un as pija la responsabilità dë dì che a l'ha scrivusslo daspërchiel (ò daspërchila), ò pura che a l'ha copialo da na sorgiss ëd domini pùblich, ò pura da 'nt n'àutra sorgiss dla midema sòrt (che a varda $1 për avej pì d'anformassion).
<strong>CHE A MANDA PA DËL MATERIAL COERTÀ DA DRIT D'AUTOR SENSA AVEJ AVÙ ËL PËRMESS SCRIT DË FELO!</strong>",
'longpagewarning'           => "<strong>CHE A TEN-A PRESENT!: Sta pàgina-sì a l'é longa $1 kb; chèich
programa ëd navigassion a podrìa avej dle gran-e a modifiché dle pàgine che a-j rivo a brus
ò pura a passo ij 32 kb.
Për piasì che a varda se a-i fussa mai la possibilità dë divide sto paginon an vàire tòch pì cit.</strong>",
'longpageerror'             => "<strong>EROR: Ël test che a l'ha mandà a l'é longh $1 kb, che a resta pì che ël
lìmit màssim ëd $2 kb. Parej as peul nen salvesse. A venta che a në fasa vàire
pàgine diferente për rintré ant ij lìmit tècnich.</strong>",
'readonlywarning'           => "<strong>AVIS: La base dat a l'é staita blocà për manutension,
e donca a peudrà pa salvesse soe modìfiche tut sùbit. A peul esse che
a-j ven-a còmod copiesse via sò test e butesslo da na part për salvelo peuj.</strong>",
'protectedpagewarning'      => "<strong>AVIS: costa pàgina-sì a l'é staita blocà an manera che mach j'utent con la qualìfica da aministrator a peulo feje dle modìfiche.</strong>",
'semiprotectedpagewarning'  => "'''Nòta:'''costa pàgina-sì a l'é staita protegiùa an manera che mach j'utent registrà a peulo modifichela.",
'cascadeprotectedwarning'   => "'''Tension:''' sta pàgina-sì a l'è staita blocà an manera che mach j'utent con la qualìfica da aministrator a peulo modifichela, për via che {{PLURAL:\$1|a l'é proteta|a-i intra ant le pàgine protete}} col sistema \"a cascada\":",
'titleprotectedwarning'     => "<strong>DOSMAN: sta pàgina-sì a l'é protegiùa an manera che mach chèich utent a peul creéla.</strong>",
'templatesused'             => 'Stamp dovrà dzora a sta pàgina-sì:',
'templatesusedpreview'      => 'Stamp dovrà ant sta preuva-sì:',
'templatesusedsection'      => 'Stamp dovrà ant sta session-sì:',
'template-protected'        => '(protet)',
'template-semiprotected'    => '(mes-protet)',
'hiddencategories'          => 'Sta pàgina-sì a fa part ëd {{PLURAL:$1|na categorìa|$1 categorìe}} stërmà:',
'edittools'                 => "<!-- Test ch'a së s-ciàira sot a ij mòduj ëd mòdifica e 'd càrich d'archivi. -->",
'nocreatetitle'             => 'Creassion ëd pàgine limità',
'nocreatetext'              => "Cost sit-sì a l'ha limità la possibilità ëd creé dle pàgine neuve.
A peul torné andaré e modifiché na pàgine che a-i é già, ò pura [[Special:UserLogin|rintré ant ël sistema ò deurb-se un cont]].",
'nocreate-loggedin'         => "A l'ha pa ij përmess dont a fa da manca për podej creé dle pàgine neuve an {{SITENAME}}.",
'permissionserrors'         => 'Eror ant ij përmess',
'permissionserrorstext'     => "A l'ha pa ij përmess dont a fa da manca për {{PLURAL:$1|via che|via che}}:",
'recreate-deleted-warn'     => "'''Ch'a fasa atension: a l'é an brova d'arcreé na pàgina ch'a l'era staita scancelà.'''

Ch'a varda d'esse sigur ch'a vala la pen-a dë travajé ant sna pàgina parej.
Për soa comodità i-j mostroma la lista djë scancelament ch'a toco sta pàgina-sì:",

# "Undo" feature
'undo-success' => "Sta modìfica-sì as peul scancelesse. Për piasì, ch'a contròla ambelessì sota për esse sigur che a l'é pro lòn che a veul fé, e peuj ch'as salva lòn ch'a l'ha butà chiel/chila për finì dë scancelé la modìfica ch'a-i era.",
'undo-failure' => "Sta modìfica a l'é nen podusse scancelé për via che a-i son dle contradission antra version antrames.",
'undo-summary' => 'Gavé la revision $1 faita da [[Special:Contributions/$2|$2]] ([[User talk:$2|Ciaciarade]])',

# Account creation failure
'cantcreateaccounttitle' => "As peul pa registresse d'utent",
'cantcreateaccount-text' => "La cression ëd cont neuv a parte da st'adrëssa IP-sì ('''$1''') a l'é staita blocà da [[User:$3|$3]].

La rason butà da $3 për ël blocagi a l'é staita: ''$2''",

# History pages
'viewpagelogs'        => 'Smon ij registr dë sta pàgina-sì',
'nohistory'           => "La stòria dle version dë sta pàgina-sì a l'é pa trovasse.",
'revnotfound'         => 'Version nen trovà',
'revnotfoundtext'     => "La version prima dl'artìcol che a l'ha ciamà a l'é pa staita trovà.
Che as controla për piasì l'adrëssa (URL) che a l'ha dovrà për rivé a sta pàgina-sì.",
'currentrev'          => "Versione dël dì d'ancheuj",
'revisionasof'        => 'Revision $1',
'revision-info'       => 'Revision al $1; $2',
'previousrevision'    => '←Version pì veja',
'nextrevision'        => 'Revision pì neuve→',
'currentrevisionlink' => 'vardé la version corenta',
'cur'                 => 'cor',
'next'                => 'anans',
'last'                => 'andaré',
'page_first'          => 'prima',
'page_last'           => 'ùltima',
'histlegend'          => 'Confront antra version diferente: che as selession-a le casele dle version che a veul e peui che a sgnaca ël boton për anandié ël process.<br />
Legenda: (cor) = diferense con la version corenta,
(prim) = diferense con la version prima, M = modìfica cita',
'deletedrev'          => '[scancelà]',
'histfirst'           => 'Prima',
'histlast'            => 'Ùltima',
'historysize'         => '({{PLURAL:$1|1|$1}} byte)',
'historyempty'        => '(veujda)',

# Revision feed
'history-feed-title'          => 'Stòria',
'history-feed-description'    => 'Stòria dla pàgina ansima a sto sit-sì',
'history-feed-item-nocomment' => '$1 al $2', # user at time
'history-feed-empty'          => "La pàgina che a l'ha ciamà a-i é pa; a podrìa esse staita scancelà da 'nt ël sit, ò pura tramudà a n'àutr nòm.

Che a verìfica con la [[Special:Search|pàgina d'arserca]] se a-i fusso mai dj'àutre pàgine che a podèisso andeje bin.",

# Revision deletion
'rev-deleted-comment'         => '(coment gavà)',
'rev-deleted-user'            => '(stranòm gavà)',
'rev-deleted-event'           => '(element gavà)',
'rev-deleted-text-permission' => "<div class=\"mw-warning plainlinks\">
Costa revision  dla pàgina-sì a l'é staita gavà via da 'nt j'archivi pùblich.
A peul esse che a sio restajne chèich marca ant ël [{{fullurl:Special:Log/delete|page={{PAGENAMEE}}}} Registr ëd jë scancelament].
</div>",
'rev-deleted-text-view'       => "<div class=\"mw-warning plainlinks\">
Costa revision dla pàgina-sì a l'é staita gavà via da 'nt j'archivi pùblich.
Coma aministrator d'ës sit-sì chiel a peul ës-ciairela;
a peul esse che a sio restajne chèich marca ant ël [{{fullurl:Special:Log/delete|page={{PAGENAMEE}}}} Registr ëd jë scancelament].
</div>",
'rev-delundel'                => 'mostra/stërma',
'revisiondelete'              => 'Scancela/disdëscancela revision',
'revdelete-nooldid-title'     => 'Version nen specificà',
'revdelete-nooldid-text'      => "A l'ha pa visasse dë dì ansima a che version dla pàgina che a venta fé sossì.",
'revdelete-selected'          => '{{PLURAL:$2|Revision|Revision}} selessionà për [[:$1]]:',
'logdelete-selected'          => '{{PLURAL:$1|Event|Event}} dël registr selessionà:',
'revdelete-text'              => "Le version scancelà a së s-ciaireran sempe ant la stòria dla pàgina,
ma sò test al pùblich a-j andran pì nen.

J'àutri aministrator ëd sta wiki-sì a saran ancó sempe bon a s-ciairé ël contnù stërmà
e a podran disdëscancelelo andré con la midema antërfacia, sempe che a sia nen staita butà
na restrission adissional da j'operator dël sit.",
'revdelete-legend'            => 'But-je coste limitassion-sì a le version scancelà:',
'revdelete-hide-text'         => 'Stërma ël test dla revision',
'revdelete-hide-name'         => 'Stërma assion e oget',
'revdelete-hide-comment'      => 'Stërma ël coment a la modìfica',
'revdelete-hide-user'         => "Stërma lë stranòm ò l'adrëssa IP dël contributor",
'revdelete-hide-restricted'   => "But-je ste restrission-sì a j'aministrator tan-me a j'àutri",
'revdelete-suppress'          => "Smon-je pa ij dat gnanca a j'aministrator",
'revdelete-hide-image'        => "Stërma ël contnù dl'archivi",
'revdelete-unsuppress'        => "Gava le limitassion da 'nt le version ciapà andaré",
'revdelete-log'               => 'Coment për ël registr:',
'revdelete-submit'            => 'But-jlo a la version selessionà',
'revdelete-logentry'          => 'visibilità dla revision cangià për [[$1]]',
'logdelete-logentry'          => "a l'ha cangiaje visibilità a l'event [[$1]]",
'revdelete-success'           => "'''Visibilità dla revision butà coma ch'as dev.'''",
'logdelete-success'           => "'''Visibilità dla revision butà coma ch'as dev.'''",
'pagehist'                    => 'Stòria dla pàgina',
'deletedhist'                 => 'Stòria scancelà',

# History merging
'mergehistory'                     => 'Buté ansema jë stòrie',
'mergehistory-header'              => "Sta pàgina-sì a lassa fene buté le revision ëd na pàgina ansema a cole 'd n'àutra.
Ch'a varda mach che a-i ven-a nen fòra rabel ant la continuità stòrica.",
'mergehistory-box'                 => 'Fene un-a dle stòrie ëd doe pàgine:',
'mergehistory-from'                => 'Pàgina sorgiss:',
'mergehistory-into'                => "Pàgina 'd destinassion:",
'mergehistory-list'                => "Stòria ch'as peul unifichesse",
'mergehistory-merge'               => "Ambelessì sota a-i sone le revision ëd [[:$1]] ch'as peulo butesse ansema a [[:$2]]. Ch'a dòvra la casela për marché l'ùltima version da gionté. Ch'a ten-a present che se a nàviga via da sta pàgina-sì sta colòna as veujda.",
'mergehistory-go'                  => "Smon le modìfiche ch'as peulo butesse ansema",
'mergehistory-submit'              => 'Buta ansema le revision',
'mergehistory-empty'               => "Pa gnun-a revision ch'as peula butesse ansema.",
'mergehistory-success'             => '$3 revision ëd [[:$1]] a son ëstàite butà ansema a [[:$2]] sensa problema.',
'mergehistory-fail'                => "A l'é nen riessusse a buté ansema le revision, për piasì, ch'as contròla la pàgina e ij temp.",
'mergehistory-no-source'           => 'La pàgina sorgiss $1 a-i é pa.',
'mergehistory-no-destination'      => 'La pàgina ëd destinassion $1 a-i é pa.',
'mergehistory-invalid-source'      => "La pàgina sorgiss a l'ha d'avej un tìtol bon.",
'mergehistory-invalid-destination' => "La pàgina ëd destinassion a l'ha d'avej un tìtol bon.",

# Merge log
'mergelog'           => "Registr dj'union",
'pagemerge-logentry' => "a l'ha butà [[$1]] ansema a [[$2]] (revision fin-a a la $3)",
'revertmerge'        => 'Gavé da ansema',
'mergelogpagetext'   => "Ambelessì sota a-i é na lista dj'ùltime vire che la stòria ëd na pàgina a l'é stàita butà ansema a cola 'd n'àutra.",

# Diffs
'history-title'           => 'Cronologìa dle modìfiche ëd "$1"',
'difference'              => '(Diferense antra revision)',
'lineno'                  => 'Riga $1:',
'compareselectedversions' => 'Paragon-a le version selessionà',
'editundo'                => "buta 'me ch'a l'era",
'diff-multi'              => '({{PLURAL:$1|Na revision antërmedia|$1 revision antërmedie}} pa mostrà.)',

# Search results
'searchresults'         => "Arsultà dl'arserca",
'searchresulttext'      => "Per avej pì d'anformassion ant sl'arserca interna ëd {{SITENAME}}, che a varda [[{{MediaWiki:Helppage}}|Arserca ant la {{SITENAME}}]].",
'searchsubtitle'        => 'Domanda "[[:$1]]"',
'searchsubtitleinvalid' => 'Domanda "$1"',
'noexactmatch'          => "'''La pàgina \"\$1\" a-i é pa.''' As peul [[:\$1|creéla d'amblé]].",
'noexactmatch-nocreate' => "'''A-i é pa gnun-a pàgina ciamà \"\$1\".'''",
'toomanymatches'        => "Parej a-i ven fòra tròpa ròba, për piasì, ch'a prova n'arsërca diferenta.",
'titlematches'          => "Ant ij tìtoj dj'artìcoj",
'notitlematches'        => "La vos che a l'ha ciamà a l'é pa trovasse antrames aj tìtoj dj'articol",
'textmatches'           => "Ant ël test ëd j'artìcoj",
'notextmatches'         => "La vos che a l'ha ciamà a l'é pa trovasse antrames aj test dj'articol",
'prevn'                 => 'ij $1 prima',
'nextn'                 => 'ij $1 peuj',
'viewprevnext'          => 'Che a varda ($1) ($2) ($3).',
'search-result-size'    => '$1 ({{PLURAL:$2|un|$2}} mòt)',
'search-result-score'   => 'Arlevansa: $1%',
'search-redirect'       => '(ridiression $1)',
'search-section'        => '(session $1)',
'search-suggest'        => 'Vorìi-lo pa dì: $1',
'searchall'             => 'tuti',
'showingresults'        => "Ambelessì sota a treuva fin a {{PLURAL:$1|'''1'''|'''$1'''}} arzultà, a parte dal nùmer #'''$2'''.",
'showingresultsnum'     => "Ambelessì sota a treuva {{PLURAL:$3|'''1'''|'''$3'''}} arzultà a parte da #'''$2'''.",
'showingresultstotal'   => "Ambelessì sota as ë-sciàiro j'arzultà '''da $1 a $2''' ëd '''$3''' ch'a-i në j'é.",
'nonefound'             => '<strong>Nòta</strong>: l\'arserchè dle paròle soèns dovrà, coma "avej" ò "esse", che a son pa indicisà, a peul dé n\'arsultà negativ, tan-me buté pì che na paròla da arserché (che a ven-o fòra mach cole pàgine andoa le paròle arsercà a-i son tute ansema).',
'powersearch'           => 'Arserca',
'powersearch-legend'    => 'Arsërca avansà',
'searchdisabled'        => "L'arserca anterna ëd {{SITENAME}} a l'é nen abilità; për adess a peul prové a dovré un motor d'arserca estern coma Google. (Però che a ten-a present che ij contnù ëd {{SITENAME}} listà ant ij motor pùblich a podrìo ëdcò esse nen d'autut agiornà)",

# Preferences page
'preferences'              => 'Mè gust',
'mypreferences'            => 'mè gust',
'prefs-edits'              => 'Modìfiche faite:',
'prefsnologin'             => "A l'é ancó pa rintrà ant ël sistema",
'prefsnologintext'         => 'A dev [[Special:UserLogin|rintré ant ël sistema]]
për podej specifiché ij sò gust.',
'prefsreset'               => 'Ij "sò gust" a son stait pijait andré da \'nt la memòria dël server ëd {{SITENAME}}.',
'qbsettings'               => 'Regolassion dla bara dij menù',
'qbsettings-none'          => 'Gnun',
'qbsettings-fixedleft'     => 'Fissà a la man ësnista',
'qbsettings-fixedright'    => 'Fissà a la man drita',
'qbsettings-floatingleft'  => 'Flotant a la man ësnista',
'qbsettings-floatingright' => 'Flotant a la man drita',
'changepassword'           => 'Cambia ciav',
'skin'                     => 'Facia',
'math'                     => 'Fòrmule ëd matemàtica',
'dateformat'               => 'Forma dla data',
'datedefault'              => "franch l'istess",
'datetime'                 => 'Data e ora',
'math_failure'             => 'Parsificassion falà',
'math_unknown_error'       => 'Eror nen conossù',
'math_unknown_function'    => 'funsion che as sa pa lòn che a la sia',
'math_lexing_error'        => 'eror ëd léssich',
'math_syntax_error'        => 'eror ëd sintassi',
'math_image_error'         => 'Conversion a PNG falà; che a contròla che latex, dvips, gs, e convert a sio instalà giust',
'math_bad_tmpdir'          => "Ël sistema a-i la fa pa a creé la diretriss '''math temp''', ò pura a-i la fa nen a scriv-je andrinta",
'math_bad_output'          => "Ël sistema a-i la fa pa a creé la diretriss '''math output''', ò pura a-i la fa nen a scriv-je andrinta",
'math_notexvc'             => 'Pa gnun texvc executable; për piasì, che a contròla math/README për la configurassion.',
'prefs-personal'           => "Profil dl'utent",
'prefs-rc'                 => 'Ùltime modìfiche',
'prefs-watchlist'          => 'Ròba che as ten sot euj',
'prefs-watchlist-days'     => 'Vàire dì che a veul ës-ciairé an soa lista ëd lòn che as ten sot euj:',
'prefs-watchlist-edits'    => 'Vàire modìfiche che a veul ës-ciairé con le funsion avansà:',
'prefs-misc'               => 'Sòn e lòn',
'saveprefs'                => 'Salvé ij sò gust',
'resetprefs'               => 'Buta torna ij "mè gust" coma a-i ero al prinsipi',
'oldpassword'              => 'Veja ciav',
'newpassword'              => 'Neuva ciav',
'retypenew'                => 'Che a scriva torna soa neuva ciav',
'textboxsize'              => 'Amzure dël quàder ëd modìfica dël test',
'rows'                     => 'Righe',
'columns'                  => 'Colòne',
'searchresultshead'        => "Specifiché soe preferense d'arserca",
'resultsperpage'           => 'Arsultà da mostré për vira pàgina',
'contextlines'             => 'Righe ëd test për vira arsultà',
'contextchars'             => 'Caràter për riga',
'stub-threshold'           => 'Valor mìnim për j\'<a href="#" class="stub">anliure a jë sbòss</a>:',
'recentchangesdays'        => "Vàire dì smon-e ant j'ùltime modìfiche:",
'recentchangescount'       => "Nùmer ëd tìtoj ant j'ùltime modìfiche",
'savedprefs'               => 'Ij sò gust a son stait salvà.',
'timezonelegend'           => 'Fus orari',
'timezonetext'             => "Che a buta ël nùmer d'ore ëd diferensa antra soa ora local e l'ora dël server (UTC).",
'localtime'                => 'Ora Local',
'timezoneoffset'           => 'Diferensa oraria<sup>1</sup>',
'servertime'               => 'Ora dël server',
'guesstimezone'            => "Ciapa sù l'ora da 'nt ël mè programa ëd navigassion (browser)",
'allowemail'               => "Lassa che j'àutri Utent am mando ëd pòsta eletrònica",
'defaultns'                => 'Se as dis nen divers, as sërca ant costi spassi nominaj-sì:',
'default'                  => 'stàndard',
'files'                    => 'Archivi',

# User rights
'userrights'               => "Gestion dij drit dj'utent", # Not used as normal message but as header for the special page itself
'userrights-lookup-user'   => "Gestion dle partìe d'utent",
'userrights-user-editname' => 'Che a buta në stranòm:',
'editusergroup'            => "Modifiché le partìe d'Utent",
'editinguser'              => "Modìfica ëd '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup' => "Modìfiché le partìe dj'utent",
'saveusergroups'           => "Salva le partìe d'utent",
'userrights-groupsmember'  => "A l'é andrinta a:",
'userrights-reason'        => 'Rason dla modìfica:',
'userrights-no-interwiki'  => "A l'ha pa ij përmess dont a fa da manca për podej cambieje ij drit a dj'utent ansima a dj'àutre wiki.",
'userrights-nodatabase'    => "La base dat $1 a-i é pa, ò pura a l'é nen local.",
'userrights-nologin'       => "A l'ha da [[Special:UserLogin|rintré ant ël sistema]] con un cont da aministrator për podej-je dé dij drit a j'utent.",
'userrights-notallowed'    => "A l'ha pa ij përmess dont a fa da manca për podej-je dé dij drit a j'utent.",

# Groups
'group'               => 'Partìa:',
'group-autoconfirmed' => "Utent ch'a son convalidasse daspërlor",
'group-bot'           => 'Trigomiro',
'group-sysop'         => 'Aministrator',
'group-bureaucrat'    => 'Mangiapapé',
'group-all'           => '(utent)',

'group-autoconfirmed-member' => "Utent ch'a l'é convalidasse daspërchiel/chila",
'group-bot-member'           => 'Trigomiro',
'group-sysop-member'         => 'Aministrator',
'group-bureaucrat-member'    => 'Mangiapapé',

'grouppage-autoconfirmed' => "{{ns:project}}:Utent ch'a son convalidasse daspërlor",
'grouppage-bot'           => '{{ns:project}}:Trigomiro',
'grouppage-sysop'         => '{{ns:project}}:Aministrator',
'grouppage-bureaucrat'    => '{{ns:project}}:Mangiapapé',

# User rights log
'rightslog'      => "Drit dj'utent",
'rightslogtext'  => "Sòn a l'é na lista dij cambiament aj drit dj'utent.",
'rightslogentry' => "a l'ha tramudà $1 da 'nt la partìa $2 a la partìa $3",
'rightsnone'     => '(gnun)',

# Recent changes
'nchanges'                          => '$1 {{PLURAL:$1|modìfica|modìfiche}}',
'recentchanges'                     => 'Ùltime Modìfiche',
'recentchangestext'                 => "Costa a l'é la pàgina che a ten ël registr dij cambiament a la wiki pì davsin ant ël temp.",
'recentchanges-feed-description'    => 'Tracé le modìfiche dla wiki pì davsin ant ël temp ant sta score-sì.',
'rcnote'                            => "Ambelessì sota a-i {{PLURAL:$1|é l'ùltima pàgina|son j'ùltime '''$1''' pàgine}} modificà ant {{PLURAL:$2|l'ùltim|j'ùltim '''$2'''}} dì, a fé data dël $3.",
'rcnotefrom'                        => ' Ambelessì sota a-i é la lista dle modìfiche da <b>$2</b> (fin-a a <b>$1</b>).',
'rclistfrom'                        => 'Most-me le modìfiche a parte da $1',
'rcshowhideminor'                   => '$1 le modìfiche cite',
'rcshowhidebots'                    => '$1 ij trigomiro',
'rcshowhideliu'                     => "$1 j'utent registrà",
'rcshowhideanons'                   => "$1 j'utent anònim",
'rcshowhidepatr'                    => '$1 le modìfiche verificà',
'rcshowhidemine'                    => '$1 mie modìfiche',
'rclinks'                           => "Most-me j'ùltime $1 modìfiche ëd j'ùltim $2 dì<br />$3",
'diff'                              => 'dif.',
'hist'                              => 'stòria',
'hide'                              => 'stërma',
'show'                              => 'smon',
'minoreditletter'                   => 'c',
'newpageletter'                     => 'N',
'boteditletter'                     => 'b',
'number_of_watching_users_pageview' => '[$1 utent che as ten-o sossì sot euj]',
'rc_categories'                     => 'Limité a le categorìe (che a jë scriva separand-je antra lor con un "|")',
'rc_categories_any'                 => 'Qualsëssìa',
'newsectionsummary'                 => '/* $1 */ session neuva',

# Recent changes linked
'recentchangeslinked'          => 'Modìfiche colegà',
'recentchangeslinked-title'    => 'Modìfiche ch\'a-i intro con "$1"',
'recentchangeslinked-noresult' => "Ant ël moment dont ës parla a-i é pa staie gnun-a modìfica a le pàgine con dj'anliure ch'a men-o ambelessì.",
'recentchangeslinked-summary'  => "Sta pàgina special-sì a la smon j'ùltime modìfiche a le pàgine ch'a son colegà për anliura a costa. Le pàgine che chiel/chila as ten sot euj a resto marcà an '''grassèt'''.",

# Upload
'upload'                      => 'Carié',
'uploadbtn'                   => 'Carié',
'reupload'                    => 'Caria torna',
'reuploaddesc'                => 'Torné al mòdulo ëd domanda për carié archivi',
'uploadnologin'               => "A dev rintré ant ël sistema për podej fé st'operassion-sì",
'uploadnologintext'           => "A dev [[Special:UserLogin|rintré ant ël sistema]]
për podej carié dj'archivi.",
'upload_directory_read_only'  => 'Ël programa webserver a-i la fa nen a scrive ansima a la diretriss ëd càrich ($1).',
'uploaderror'                 => 'Eror dëmentré che as cariava',
'uploadtext'                  => "'''DOSMAN!''' Anans che carié dla ròba ansima a {{SITENAME}}, che a sia motobin sigur d'avej bin lesù e capì
[[{{MediaWiki:Policy-url}}|ël regolament ëd {{SITENAME}} ansima al dovré dle figure]].

Për vardé ò pura sërché figure già carià ant sla {{SITENAME}}, che a vada ant sla [[Special:ImageList|lista dle figure]].
Lòn che as caria e së scancela a resta marcà ant ël [[Special:Log/upload|registr dij càrich]].

Che a dòvra ël mòdulo ambelessì sota për carié neuv archivi con figure da dovré për fé pì bej e bin spiegà ij sò artìcoj.
Ant sla pì part dij programa ëd navigassion dla Ragnà (browsers) a dovr ia s-ciairesse un boton con scrit \"Browse...\" (ò pura \"Sfeuja...\", se i l'eve un sistema n'italian) che av deurb la sòlita fnestra che as dòvra për carié dj'archivi.<br />

Ën sërnend un dj'archivi che i l'eve ant sij vòstri disco, ël nòm a vnirà scrit n'automàtich ant la casela ëd test da fianch dël boton.<p>

'''A dev ëdcò selessioné la casela ëd conferma che a dis che l'archivi a-j va nen contra a gnun-a nòrma ant sël drit d'autor.'''<p>

Fait lolì, che a sgnaca ël boton \"Carié\" për completé l'operassion.
Ël càrich a podrìa duré ëdcò chèich minuta, se chiel (chila) a l'avèissa na conession che a va pian, ò pura se la figura a la fussa tròp gròssa (figure parej as conseja dë nen carieje).<p>

Le sòrt d'archivi che as preferisso a son ël JPEG për le fotografìe, ël PNG për ij dissègn, j'icòne e ij simboj, l'OGG për j'archivi sonòr.<p>

Për piasì, anans che carieje, che a rinòmina ij sò archivi con dij nòm che diso lòn che a son, për evité dë fé confusion.
Për buté na neuva figura ant n'articol, dovré n'anliura ant la forma
'''<nowiki>[[image:archivi.jpg]]</nowiki>''' ò pura
'''<nowiki>[[image:archivi.png|alt text, test alternativ]]</nowiki>''' ò pura
'''<nowiki>[[media:archivi.ogg]]</nowiki>''' per ij son.<p>

Che a ten-a present che tan-me për tuti ij contnù ëd la {{SITENAME}}, qualsëssìa person-a a peul modifiché, cangé ò pura scancelé ij sò archivi, se a jë smija che sòn a sia ant j'anteressi ëd l'enciclopedìa. Che a ten-a ëdcò da ment che, se a-i fusso dij comportament nen conformà a le nòrme, ò pura se a-i fussa na caria tròp gròssa për ël sistema, a podrìa esse blocà (ant sël pat d'esse perseguì se a-i fusso dle responsabilita legaj).",
'upload-permitted'            => "Sòrt d'archivi consentìe: $1.",
'upload-preferred'            => "Sòrt d'archivi preferìe: $1.",
'upload-prohibited'           => "Sòrt d'archivi proibìe: $1.",
'uploadlog'                   => 'Registr dij càrich',
'uploadlogpage'               => 'Registr dij càrich',
'uploadlogpagetext'           => "Ambelessì sota a-i é la lista dj'ùltim archivi carià ant sël server ëd {{SITENAME}}.",
'filename'                    => "Nòm dl'archivi",
'filedesc'                    => 'Oget',
'fileuploadsummary'           => "Detaj dl'archivi:",
'filestatus'                  => "Situassion dij drit d'autor:",
'filesource'                  => 'Sorgiss:',
'uploadedfiles'               => 'Archivi carià ant la {{SITENAME}}',
'ignorewarning'               => "Piantla-lì con j'avis e salva an tute le manere",
'ignorewarnings'              => "Lassa sté j'avis",
'minlength1'                  => "Ël nòm dl'archivi a dev esse longh almanch un caràter.",
'illegalfilename'             => 'Ël nòm d\'archivi "$1" a l\'ha andrinta dij caràter che as peulo pa dovresse ant ij tìtoj dle pàgine. Për piasì che a-j cangia nòm e peui che a torna a carielo.',
'badfilename'                 => 'Ël nòm dl\'archivi a l\'é stait cambià an "$1".',
'filetype-badmime'            => 'J\'archivi dla sòrt MIME "$1" as peulo nen carié.',
'filetype-unwanted-type'      => "'''\".\$1\"''' a l'é na sòrt d'archivi ch'as pija nen ëd bon-a veuja. Le sòrt preferìe a son \$2.",
'filetype-banned-type'        => "'''\".\$1\"''' a l'é na sòrt d'archivi proibìa. Cole consentìe a son \$2.",
'filetype-missing'            => "A l'archivi a-j manca l'estension (pr'es. \".jpg\").",
'large-file'                  => "La racomandassion a l'é che j'archivi a sio nen pì gròss che $1; st'archivi-sì a l'amzura $2.",
'largefileserver'             => "St'archivi-sì a resta pì gròss che lòn che la màchina sentral a përmet.",
'emptyfile'                   => "L'archivi che a l'ha pen-a carià a smija veujd.
Sòn a podrìa esse rivà përchè che chiel a l'ha scrivù mal ël nòm dl'archivi midem.
Për piasì che a contròla se a l'é pro cost l'archivi che a veul carié.",
'fileexists'                  => "N'archivi con ës nòm-sì a-i é già, për piasì che as contròla <strong><tt>$1</tt></strong> se a l'é pa sigur dë volej cangelo.",
'filepageexists'              => "La pàgina ëd descrission për st'archivi-sì a l'é già stàita creà an <strong><tt>$1</tt></strong>, mach ch'a-i é pa gnun archivi ch'as ciama parej. Lòn ch'a buta për somari as ës-ciairerà nen ant la pàgina ëd descrission. Për podej buté sò somari a l'ha da modifichesse la pàgina a man",
'fileexists-extension'        => "N'archivi con ës nòm-s a-i é già:<br />
Nòm dl'archivi ch'as carìa: <strong><tt>$1</tt></strong><br />
Nòm dl'archivi ch'a-i é già: <strong><tt>$2</tt></strong><br />
Për piasì, ch'a serna un nòm diferent.",
'fileexists-thumb'            => "<center>'''Figura ch'a-i é'''</center>",
'fileexists-thumbnail-yes'    => "L'archivi a-j ëmsija a na <i>figurin-a</i>. Për piasì, ch'a contròla l'archivi <strong><tt>$1</tt></strong>.<br />
S'a l'é la midema figura a amzura pijn-a, a veul dì ch'a fa nen dë manca dë carié na figurin-a.",
'file-thumbnail-no'           => "Ël nòm dl'archivi as anandia con <strong><tt>$1</tt></strong>. A-j ësmija a na <i>figurin-a</i>.
Se a l'ha na figura a amzura pijn-a a l'é mej ch'a carìa cola-lì, dësnò ch'a-j cangia nòm a l'archivi, për piasì.",
'fileexists-forbidden'        => "Belavans n'archivi con ës nòm-sì a-i é già, donca ël nòm as peul pa pì dovresse; për piasì che a torna andré e che as caria sò archivi con un nòm diferent. [[Image:$1|thumb|center|$1]]",
'fileexists-shared-forbidden' => "Belavans n'archivi con ës nòm-sì ant la diretriss dj'archivi condivis a-i é già, donca ël nòm as peul pa pì dovresse; për piasì che a torna andré e che as caria sò archivi con un nòm diferent.
[[Image:$1|thumb|center|$1]]",
'successfulupload'            => 'Carià complet',
'uploadwarning'               => 'Avis che i soma dapress a carié',
'savefile'                    => "Salva l'archivi",
'uploadedimage'               => 'a l\'ha carià "[[$1]]"',
'overwroteimage'              => 'a l\'ha carìa na version neuva ëd "[[$1]]"',
'uploaddisabled'              => 'Càrich blocà',
'uploaddisabledtext'          => "La possibilità ëd carié dj'archivi ansima a sta wiki-sì a l'é staita disabilità.",
'uploadscripted'              => "St'archivi-sì a l'ha andrinta chèich-còs (dël còdes HTML ò pura dlë script) che a podrìe esse travajà mal da chèich programa ëd navigassion (browser).",
'uploadcorrupt'               => "St'archivi-sì ò che a l'é falà ò che a l'ha n'estension cioca. Për piasì, che as contròla l'archivi e peuj che a preuva torna a carielo.",
'uploadvirus'                 => "St'archivi-sì a l'han andrinta un '''vìrus!''' Detaj: $1",
'sourcefilename'              => "Nòm dl'archivi sorgiss:",
'destfilename'                => "Nòm dl'archivi ëd destinassion:",
'watchthisupload'             => "Gionta sossì a lòn ch'im ten-o sot euj",
'filewasdeleted'              => "N'archivi con ës nòm-sì a l'é gia stait caria e peui scancelà. Për piasì, che a verìfica $1 anans che carielo n'àutra vira.",
'upload-wasdeleted'           => "'''Dosman: a l'é antramentr ch'a carìa torna n'archivi ch'a l'era dëscancelasse.'''

Për piasì, ch'a contròla s'a val la pen-a dë felo.
Për soa comodità, ambelessì a-i son ij dat dla scancelament:",
'filename-bad-prefix'         => "Ël nòm dl'archivi ch'a l'é dapress a carié as anandia për <strong>\"\$1\"</strong>, ch'a l'é un nòm sensa sust, për sòlit butà-lì n'aotomàtich da le màchine fotogràfiche digitaj, basta ch'a-i në sia un. Për piasì, ch'a-j daga a sò archivi un nòm ch'a disa lòn ch'a l'é.",
'filename-prefix-blacklist'   => " #<!-- ch'a lassa sta riga-sì tanme ch'a l'é --> <pre>
# La sintassi a l'é:
#   * Tut lòn ch'a-i ven dapress al segn \"#\" fin a la fin dla riga a resta mach un coment
#   * Na riga nen veujda a la resta un prefiss ëd nòm d'archivi për sòlit dovrà da na chèich màchina fotogràfica digitala
CIMG # Casio
DSC_ # Nikon
DSCF # Fuji
DSCN # Nikon
DUW # chèich celular
IMG # genérich
JD # Jenoptik
MGP # Pentax
PICT # vàire marche diferente
 #</pre> <!-- ch'a lassa sta riga-sì tanme ch'a l'é -->",

'upload-proto-error'      => 'Protocòl cioch',
'upload-proto-error-text' => "Për carié da dij servent lontan a venta buté dj'anliure ch'as anandio për <code>http://</code> ò pura <code>ftp://</code>.",
'upload-file-error'       => 'Eror antern',
'upload-file-error-text'  => "A l'é rivaie n'eror antern dëmentrè che as fasìa n'archivi provisòri ant sël servent. Për piasì, ch'as butà an comunicassion con j'aministrator.",
'upload-misc-error'       => "Eror nen identificà antramentr ch'as cariava",
'upload-misc-error-text'  => "A l'é staie n'eror nen identificà dëmentrè ch'as cariava chèich-còs. Për piasì, ch'a varda che soa anliura a sia bon-a e che a l'arsponda e peuj ch'a preuva torna. Se a-i riva sossì n'àotra vira, ch'as buta an comunicassion con j'aministrator.",

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => "L'anliura a l'arspond pa",
'upload-curl-error6-text'  => "L'anliura che a l'ha butà a la travaja pa. Për piasì, ch'a contròla che st'anliura a la sia scrita giusta e che ël sit al funsion-a.",
'upload-curl-error28'      => "A l'é finìe ël temp chas peul dovresse për carié",
'upload-curl-error28-text' => "Ël sit a-i buta tròp temp a arspònde. Për piasì, ch'a contròla che a funsion-a, ch'a speta na minuta e peuj che a torna a prové. A peul esse che a-j ven-a a taj serne un moment che ës sit a sia nen tant crarià ëd tràfich.",

'license'            => 'Licensa:',
'nolicense'          => 'Pa gnun-a selession faita',
'license-nopreview'  => "(Gnun-a preuva ch'as peula smon-se)",
'upload_source_url'  => "  (n'anliura bon-a e che as peula dovresse)",
'upload_source_file' => " (n'archivi da sò calcolator)",

# Special:ImageList
'imagelist-summary'     => "Sta pàgina special-sì a la smon tuti j'archivi ch'a ëstàit carià.
Për sòlit j'ùltim carià a resto an sima.
Ch'a-i bata 'n colp col rat ansima a j'antestassion dle colòne për cangé órdin.",
'imagelist_search_for'  => "Arsërca për nòm d'archivi multimojen:",
'imgfile'               => 'archivi',
'imagelist'             => 'Lista dle figure',
'imagelist_date'        => 'Data',
'imagelist_name'        => 'Nòm',
'imagelist_user'        => 'Utent',
'imagelist_size'        => 'Amzura an byte',
'imagelist_description' => 'Descrission',

# Image description page
'filehist'                  => "Stòria dl'archivi",
'filehist-help'             => "Ch'a-i daga un colp col rat ant sna cobia data/ora për ës-ciairé coma restèissa l'archivi ant col moment-là.",
'filehist-deleteall'        => 'dëscancelé tut',
'filehist-deleteone'        => 'dëscancelé sta version-sì',
'filehist-revert'           => "buté torna 'me ch'a lera",
'filehist-current'          => "dël dì d'ancheuj",
'filehist-datetime'         => 'Data e Ora',
'filehist-user'             => 'Utent',
'filehist-dimensions'       => 'Amzure',
'filehist-filesize'         => "Amzure dl'archivi",
'filehist-comment'          => 'Oget',
'imagelinks'                => 'Anliure a le figure',
'linkstoimage'              => "Le pàgine sì sota a l'han andrinta dj'anliure a sta figura-sì:",
'nolinkstoimage'            => "Pa gnun-a pàgina che a l'abia n'anliura a sta figura-sì.",
'sharedupload'              => "St'archivi-sì a l'é stait carià an comun; donca a peul esse dovrà antra vàire proget wiki diferent.",
'shareduploadwiki'          => 'Che as varda $1 për savejne dë pì.',
'shareduploadwiki-desc'     => "La descrission ch'a-i resta ansima a $1 as ës-ciàira ambelessì sota.",
'shareduploadwiki-linktext' => "pàgina dë spiegon dl'archivi",
'noimage'                   => 'A-i é pa gnun archivi che as ciama parej, a peul $1.',
'noimage-linktext'          => 'carijlo',
'uploadnewversion-linktext' => "Carié na version neuva dë st'archivi-sì",
'imagepage-searchdupe'      => "Arsërca dj'archivi dobi",

# File reversion
'filerevert'                => "Buta torna $1 tanme ch'a l'era",
'filerevert-legend'         => "Buté torna l'archivi tanme ch'a l'era",
'filerevert-intro'          => "A l'é dapress a buté torna l'archivi  '''[[Media:$1|$1]]''' com a l'era ant la [version $4 dël $2, $3].",
'filerevert-comment'        => 'Oget:',
'filerevert-defaultcomment' => 'Version dël $1, $2 butà torna coma corenta.',
'filerevert-submit'         => "Buté tanme ch'a l'era",
'filerevert-success'        => "'''L<nowiki>'</nowiki>archivi [[Media:$1|$1]]''' a l'é stait torna butà com a l'era ant la [$4 version dël $2, $3].",
'filerevert-badversion'     => "A-i é pa gnun-a version locala dl'archivi ch'a l'abia un marcatemp parej.",

# File deletion
'filedelete'                  => 'Dëscancelé $1',
'filedelete-legend'           => "Dëscancelé l'archivi",
'filedelete-intro'            => "A l'é an brova dë scancelé '''[[Media:$1|$1]]'''.",
'filedelete-intro-old'        => "A l'é dapress ch'a scancela l'archivi '''[[Media:$1|$1]]''' dël [$4 $3, $2].",
'filedelete-comment'          => 'Rason:',
'filedelete-submit'           => 'Dëscancelé',
'filedelete-success'          => "A l'é dëscancelasse l'archivi '''$1'''.",
'filedelete-success-old'      => "<span class=\"plainlinks\">A l'é dëscancelasse la version dël \$3, \$2 ëd l'archivi '''[[Media:\$1|\$1]]'''.</span>",
'filedelete-nofile'           => "A-i é pa gnun archivi ch'as ciama: $1",
'filedelete-nofile-old'       => "A-i é pa gnun-a version parej ëd l'archivi '''$1'''",
'filedelete-iscurrent'        => "A l'é antramentr ch'as preuva a scancelé la version pì giovna dl'archivi. Për piasì, anans dë fé sòn, ch'a arpija andaré na version pì veja, an manera che costa-sì a la resta pì nen corenta.",
'filedelete-otherreason'      => 'Rason àutra/adissional:',
'filedelete-reason-otherlist' => 'Àutra rason',
'filedelete-reason-dropdown'  => "*Për sòlit la ròba ës ëscancela për
** violassion dij drit d'autor
** duplicassion (visadì ch'a-i era già)",
'filedelete-edit-reasonlist'  => 'Modifiché la rason ëd lë scancelament',

# MIME search
'mimesearch'         => 'Arsërca për sòrt MIME',
'mimesearch-summary' => "Sta pàgina-sì a lassa filtré j'archivi për sòrt MIME. Buté: sòrt/sotasòrt, pr'es. <tt>image/jpeg</tt>.",
'mimetype'           => 'Sòrt MIME:',
'download'           => 'dëscarié',

# Unwatched pages
'unwatchedpages' => 'Pàgine che as ten-o pì nen sot euj',

# List redirects
'listredirects' => 'Lista dle ridiression',

# Unused templates
'unusedtemplates'     => 'Stamp nen dovrà',
'unusedtemplatestext' => "Sta pàgina-sì a la smon tuti jë stamp (pàgine dlë spassi nominal Stamp) che a son pa dovrà andrinta a gnun-a pàgina. Mej verifiché che në stamp a-j serva nen a dj'àutri stamp (che dle vire në stamp gròss a l'é fait ëd vàire cit sotastamp), anans che fé che ranchelo via.",
'unusedtemplateswlh'  => 'àutre anliure',

# Random page
'randompage'         => 'Na pàgina qualsëssìa',
'randompage-nopages' => 'A-i é pa gnun-a pàgina an stë spassi nominal-sì.',

# Random redirect
'randomredirect'         => 'Na ridiression qualsëssìa',
'randomredirect-nopages' => 'A-i é pa gnun-a ridiression ant stë spassi nominal-sì.',

# Statistics
'statistics'             => 'Statìstiche',
'sitestats'              => 'Statìstiche dël sit',
'userstats'              => 'Statìstiche ëd {{SITENAME}}',
'sitestatstext'          => "A-i é la blëssa ëd {{PLURAL:\$1|na pàgina|'''\$1'''pàgine totaj}} ant la base dat.
Ës nùmer-sì a ciapa andrinta le pàgine ëd ciaciarada, cole ansima a {{SITENAME}}, artìcoj curt (che ant ël parlé técnich dla wiki as ciamo \"sbòss\"), ardiression, e àutre pàgine ch'a l'é belfé ch'a sio pa dj'artìcoj.
Gavà coste, a {{PLURAL:\$2|resta na pàgina ch'a l'ha|resto '''\$2''' pàgine ch'a l'han}} tuta l'ària d'esse {{PLURAL:\$2|n'artìcol|dj'artìcoj}} da bon.

'''\$8''' archivi a {{PLURAL:\$8|l'é|son}} stàit carià.

Da che la wiki a l'é duvèrta, i l'oma avù 'n total {{PLURAL:\$3|d'un-a pàgina|ëd '''\$3''' pàgine}} consultà e ëd {{PLURAL:\$4|na modìfica|'''\$4''' modìfiche}} a j'artìcoj.
Coma valor mojen i l'oma <b>\$5</b> modìfiche për artìcol, e minca artìcol a l'é stait lesù <b>\$6</b> vire për modìfica.

Ant la [http://www.mediawiki.org/wiki/Manual:Job_queue coa] a-i {{PLURAL:\$7|é|son}} '''\$7''' process.",
'userstatstext'          => "A-i {{PLURAL:$1|é '''1''' |son '''$1'''}} utent registrà, dont
'''$2''' (visadì ël '''$4%''') a l'{{PLURAL:$2|ha|han}} la qualìfica da $5.",
'statistics-mostpopular' => "Pàgine ch'a 'ncontro dë pì",

'disambiguations'      => 'Pàgine për la gestion dij sinònim',
'disambiguationspage'  => 'Template:Gestion dij sinònim',
'disambiguations-text' => "Ste pàgine-sì a men-o a në '''pàgina ëd gestion dij sinònim''', mach che a dovrìo ëmné bele drit a n'artìcol.<br />
Na pàgina as trata coma \"pàgina ëd gestion dij sinònim\" se a dòvra në stamp dont anliura as treuva ant ël [[MediaWiki:Disambiguationspage]]",

'doubleredirects'     => 'Ridiression dobie',
'doubleredirectstext' => "<b>Pieve varda:</b> costa lista-sì dle vire a peul avej andrinta dj'arsultà nen giust. Sòn a peul rivé miraco përchè a-i sio dj'anliure ò pura dël test giontà dapress a l'istrussion #REDIRECT.<br />
Vira riga a l'ha andrinta j'anliure a la prima e a la sconda rediression, ant sël pat ëd la prima riga ëd test dla seconda rediression, che për sòlit a l'ha andrinta l'artìcol ëd destinassion vèir, col andoa che a dovrìa ëmné ëdcò la prima reiression.",

'brokenredirects'        => 'Ridiression nen giuste',
'brokenredirectstext'    => "Coste ridiression-sì a men-o a dj'articoj ancó pa creà.",
'brokenredirects-edit'   => '(modìfica)',
'brokenredirects-delete' => '(scancela)',

'withoutinterwiki'         => "Pàgine ch'a l'han gnun-a anliura interwiki",
'withoutinterwiki-summary' => "Le pàgine ambelessì sota a l'han gnun-a anliura a dj'àotre lenghe:",
'withoutinterwiki-submit'  => 'Smon',

'fewestrevisions' => 'Artìcoj con manch ëd modìfiche',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|byte|bytes}}',
'ncategories'             => '$1 {{PLURAL:$1|categorìa|categorìe}}',
'nlinks'                  => '$1 {{PLURAL:$1|anliura|anliure}}',
'nmembers'                => '$1 {{PLURAL:$1|element|element}}',
'nrevisions'              => '{{PLURAL:$1|na revision|$1 revision}}',
'nviews'                  => '{{PLURAL:$1|na consultassion|$1 consultassion}}',
'specialpage-empty'       => 'Pàgina veujda.',
'lonelypages'             => 'Pàgine daspërlor',
'lonelypagestext'         => "Le pàgine ambelessì sota a l'han pa gnun-a anliura che a-i riva da dj'àotre pàgine ëd {{SITENAME}}.",
'uncategorizedpages'      => 'Pàgine che a son nen assignà a na categorìa',
'uncategorizedcategories' => 'Categorìe che a son pa assignà a na categorìa',
'uncategorizedimages'     => 'Archivi multimojen nen categorisà',
'uncategorizedtemplates'  => "Stamp sensa pa 'd categorìe",
'unusedcategories'        => 'Categorìe nen dovrà',
'unusedimages'            => 'Figure nen dovrà',
'popularpages'            => 'Pàgine pì s-ciairà',
'wantedcategories'        => 'Categorìe dont a fa da manca',
'wantedpages'             => 'Artìcoj pì ciamà',
'mostlinked'              => "Pàgine che a l'han pì d'anliure che a-i men-o la gent ansima",
'mostlinkedcategories'    => "Categorìe che a l'han pì d'anliure che a-i men-o la gent ansima",
'mostlinkedtemplates'     => 'Stamp pì dovrà',
'mostcategories'          => 'Artìcoj che a son marcà an pì categorìe',
'mostimages'              => 'Figure pì dovrà',
'mostrevisions'           => 'Artìcoj pì modificà',
'prefixindex'             => 'Ìndess për inissiaj',
'shortpages'              => 'Pàgine curte',
'longpages'               => 'Pàgine longhe',
'deadendpages'            => 'Pàgine che a men-o da gnun-a part',
'deadendpagestext'        => "Le pàgine ambelessì sota a l'han pa d'anliure anvers a j'àutre pàgine ëd {{SITENAME}}.",
'protectedpages'          => 'Pàgine sota protession',
'protectedpagestext'      => "Ambelessì sota a-i é na lista ëd pàgine ch'a son protegiùe përchè as peulo nen modifichesse ò pura tramudesse",
'protectedpagesempty'     => 'Për adess a-i é pa gnun-a pàgina protegiùa',
'protectedtitles'         => 'Tìtoj protegiù',
'protectedtitlestext'     => 'Ij tìtoj ambelessì sota a peulo pa creesse',
'protectedtitlesempty'    => "A-i é pa gnun tìtol protegiù ch'a-i intra coi criteri ch'a l'ha butà.",
'listusers'               => "Lista dj'utent",
'newpages'                => 'Pàgine neuve',
'newpages-username'       => 'Stranòm:',
'ancientpages'            => 'Le pàgine pì veje',
'move'                    => 'Tramuda',
'movethispage'            => 'Tramuda costa pàgina-sì',
'unusedimagestext'        => "<p>Che ten-a present che dj'àutri sit ant sla Ragnà, coma la {{SITENAME}} antërnassional, a podrìo avej butà n'anliura a na figura con n'adrëssa direta, e donca a peul esse che le figure ant costa lista-sì, contut che son nen dovrà ant costa version-sì dla {{SITENAME}}, a sio però dovrà ant chèich àutr pòst.",
'unusedcategoriestext'    => "Le pàgine ëd coste categorìe-sì a son fasse ma peuj a l'han andrinta nì d'artìcoj, nì ëd sotacategorìe.",
'notargettitle'           => 'A manco ij dat',
'notargettext'            => "A l'ha pa dit a che pàgina ò Utent apliché l'operassion ciamà.",
'pager-newer-n'           => '{{PLURAL:$1|1|$1}} pì neuv',
'pager-older-n'           => '{{PLURAL:$1|1|$1}} pì vej',

# Book sources
'booksources'               => 'Andoa trové dij lìber',
'booksources-search-legend' => 'Sërca dle sorgiss ëd lìber',
'booksources-go'            => 'Va',
'booksources-text'          => "Ambelessì sota a-i é na lista d'àotri sit che a vendo lìber neuv e dë sconda man, e che a peulo ëdcò smon-e dj'anformassion rësgoard a ij test che a l'é antramentr che al sërca:",

# Special:Log
'specialloguserlabel'  => 'Utent:',
'speciallogtitlelabel' => 'Tìtol:',
'log'                  => 'Registr',
'all-logs-page'        => 'Tuti ij registr',
'log-search-legend'    => 'Sërchè ant ij registr',
'log-search-submit'    => 'Va',
'alllogstext'          => "Son a mostra na combinassion dij registr ëd lòn che a l'é cariasse, scancelasse, blocasse e ëd lòn che a l'han fait j'aministrator.
A peul sern-se n'arsultà pì strèit ën selessionand na sòrt ëd registr sola, un nòm Utent ò pura la pàgina che a-j anteressa.",
'logempty'             => 'Pa gnun element parej che a sia trovasse ant ij registr.',
'log-title-wildcard'   => "Sërca ant ij tìtoj ch'as anandio për",

# Special:AllPages
'allpages'          => 'Tute le pàgine',
'alphaindexline'    => '$1 a $2',
'nextpage'          => 'Pàgina che a-i ven ($1)',
'prevpage'          => 'Pàgina anans ($1)',
'allpagesfrom'      => 'Most-me la pàgine ën partend da:',
'allarticles'       => "Tùit j'artìcoj",
'allinnamespace'    => 'Tute le pàgine (spassi nominal $1)',
'allnotinnamespace' => 'Tute le pàgine (che a son nen ant lë spassi nominal $1)',
'allpagesprev'      => 'Cole prima',
'allpagesnext'      => 'Cole che a ven-o',
'allpagessubmit'    => 'Va',
'allpagesprefix'    => "Most-me la pàgine che a l'ha prefiss:",
'allpagesbadtitle'  => "Ël tìtol che a l'ha daje a la pàgina a va nen bin, ò pura a l'ha andrinta un prefiss inter-lenga ò inter-wiki. A peul esse ëdcò che a l'abia andrinta dij caràter che as peulo nen dovresse ant ij tìtoj.",
'allpages-bad-ns'   => '{{SITENAME}} a l\'ha pa gnun ëspassi nominal "$1".',

# Special:Categories
'categories'                    => 'Categorìe',
'categoriespagetext'            => "Coste categorìe-sì a l'han andrinta dle pàgine ò dj'archivi.",
'special-categories-sort-count' => 'ordiné për nùmer',
'special-categories-sort-abc'   => 'òrdiné për alfabétich',

# Special:ListUsers
'listusersfrom'      => "Smon-me j'utent a parte da:",
'listusers-submit'   => 'Smon',
'listusers-noresult' => 'Pa gnun utent parej.',

# E-mail user
'mailnologin'     => 'A-i é pa gnun-a adrëssa për mandé ël messagi',
'mailnologintext' => "A dev [[Special:UserLogin|rintré ant ël sistema]]
e avej registrà n'adrëssa ëd pòsta eletrònica vàlida ant ij [[Special:Preferences|sò gust]] për podej mandé dij messagi ëd pòsta eletrònica a j'àutri Utent.",
'emailuser'       => "Mand-je un messagi eletrònich a st'Utent-sì",
'emailpage'       => "Mand-je un messagi ëd pòsta eletrònica a st'utent-sì",
'emailpagetext'   => "Se st'Utent-sì a l'ha registrà na soa casela ëd pòsta eletrònica, i peule scriv-je un messagi con ël mòdulo ambelessì sota.
L'adrëssa eletrònica che a l'ha specificà ant ij sò \"gust\" a sarà butà coma mitent, an manera che ël destinatari, ën volend, a peula arspond-je.",
'usermailererror' => "L'oget che a goèrna la pòsta eletrònica a l'ha dait eror:",
'defemailsubject' => 'Messagi da {{SITENAME}}',
'noemailtitle'    => 'Pa gnun-a adrëssa ëd pòsta eletrònica',
'noemailtext'     => "Cost Utent-sì a l'ha nen registrà gnun-a casela ëd pòsta eletrònica, ò pura a l'ha sërnù ëd nen fesse mandé pòsta da j'àutri Utent.",
'emailfrom'       => 'Da',
'emailto'         => 'A',
'emailsubject'    => 'Oget',
'emailmessage'    => 'Messagi',
'emailsend'       => 'Manda',
'emailccme'       => 'Mand-ne na còpia ëdcò a mia adrëssa.',
'emailccsubject'  => 'Còpia dël messagi mandà a $1: $2',
'emailsent'       => 'Messagi eletrònich mandà',
'emailsenttext'   => "Sò messagi eletrònich a l'é stait mandà",

# Watchlist
'watchlist'            => 'Ròba che im ten-o sot euj',
'mywatchlist'          => 'Ròba che im ten-o sot euj',
'watchlistfor'         => "(për '''$1''')",
'nowatchlist'          => 'A l\'ha ancó pa marcà dj\'artìcoj coma "ròba da tnì sot euj".',
'watchlistanontext'    => "Për piasì, $1 për ës-ciairé ò pura modifiché j'element ëd soa lista dla ròba che as ten sot euj.",
'watchnologin'         => "A l'é ancó nen rintrà ant ël sistema",
'watchnologintext'     => "A l'ha da manca prima ëd tut dë [[Special:UserLogin|rintré ant ël sistema]]
për podej modifiché soa lista dla ròba dë tnì sot euj.",
'addedwatch'           => "Sòn a l'é stait giontà a le pàgine che it ten-e sot euj",
'addedwatchtext'       => " La pàgina  \"<nowiki>\$1</nowiki>\" a l'é staita giontà a tua [[Special:Watchlist|lista dla ròba da tnì sot euj]].
Le modìfiche che a-i vniran ant costa pàgina-sì e ant soa pàgina ëd discussion a saran listà ambelessì, e la pàgina a së s-ciairerà ën <b>grassèt</b> ant la pàgina ëd j'[[Special:RecentChanges|ùltime modìfiche]] përchè che a resta belfé a ten-la d'euj.

Se a vorèissa mai gavé st'articol-sì da 'nt la lista dij ''Sot Euj'', che a sgnaca \" Chita da tnì sot euj \" ant sla bara dij menù.",
'removedwatch'         => "Gavà via da 'nt la lista dla ròba da tnì sot euj",
'removedwatchtext'     => 'La pàgina  "<nowiki>$1</nowiki>" a l\'è staita gavà via da soa lista dla ròba da tnì sot euj.',
'watch'                => 'ten sot euj',
'watchthispage'        => "Ten sot euj st'artìcol-sì",
'unwatch'              => 'Chita-lì da ten-e sossì sot euj',
'unwatchthispage'      => 'Chita-lì da ten-e sossì sot euj',
'notanarticle'         => "Sòn a l'é pa n'artìcol",
'notvisiblerev'        => "La revision a l'é stàita scancelà",
'watchnochange'        => 'Pa gnun-a dle ròbe che as ten sot euj che a sia staita modificà ant ël temp indicà.',
'watchlist-details'    => "A l'é dëmentrè ch'as ten sot euj {{PLURAL:$1|na pàgina|$1 pàgine}}, nen contand cole ëd discussion.",
'wlheader-enotif'      => '* Le notìfiche për pòsta eletrònica a son abilità.',
'wlheader-showupdated' => "* Cole pàgine che a son staite modificà da quand che a l'é passa l'ùltima vira a resto marcà an '''grassèt'''",
'watchmethod-recent'   => "controland j'ùltime modìfiche faite a le pàgine che as ten sot euj",
'watchmethod-list'     => 'controland le pàgine che as ten sot euj për vëdde se a-i sio mai staje dle modìfiche',
'watchlistcontains'    => "Soa lista dla ròba ch'as ten sot euj a l'ha andrinta {{PLURAL:$1|na pàgina|$1 pàgine}}.",
'iteminvalidname'      => "Problema con l'element '$1', nòm nen vàlid...",
'wlnote'               => "Ambelessì sota a-i {{PLURAL:$1|é l'ùltima modìfica|son j'ùltime '''$1''' modìfiche}} ant {{PLURAL:$2|l'ùltima ora|j'ùltime '''$2''' ore}}.",
'wlshowlast'           => "Most-me j'ùltime $1 ore $2 dì $3",
'watchlist-show-bots'  => 'Smon ëdcò ël travaj dij trigomiro',
'watchlist-hide-bots'  => 'Stërma ël travaj dij trigomiro',
'watchlist-show-own'   => 'Smon mie modìfiche',
'watchlist-hide-own'   => 'Stërma mie modìfiche',
'watchlist-show-minor' => 'Smon le modìfiche cite',
'watchlist-hide-minor' => 'Stërma le modìfiche cite',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Sot euj...',
'unwatching' => "Ën gavand da lòn ch'as ten sot euj...",

'enotif_mailer'                => '{{SITENAME}} - Servissi ëd Notìfica Postal',
'enotif_reset'                 => 'March-me tute le pàgine visità',
'enotif_newpagetext'           => "Costa-sì a l'é na pàgina neuva",
'enotif_impersonal_salutation' => 'utent ëd {{SITENAME}}',
'changed'                      => 'cangià',
'created'                      => 'creà',
'enotif_subject'               => 'La pàgina $PAGETITLE ëd {{SITENAME}} a l\'é staita $CHANGEDORCREATED da $PAGEEDITOR',
'enotif_lastvisited'           => "Che as varda $1 për ës-ciaré tute le modìfiche da 'nt l'ùltima vira che a l'é passà.",
'enotif_lastdiff'              => "Ch'a varda $1 për visioné sta modìfica.",
'enotif_anon_editor'           => 'utent anònim $1',
'enotif_body'                  => 'A l\'atension ëd $WATCHINGUSERNAME,

La pàgina $PAGETITLE dël sit {{SITENAME}} a l\'é staita $CHANGEDORCREATED al $PAGEEDITDATE da $PAGEEDITOR, che a varda $PAGETITLE_URL për la version corenta.

$NEWPAGE

Somari dl\'editor: $PAGESUMMARY $PAGEMINOREDIT

Për contaté l\'editor:
Pòsta eletrònica: $PAGEEDITOR_EMAIL
wiki: $PAGEEDITOR_WIKI

Se chiel (chila) a visitèissa nen la pàgina modificà për contròl a-i sarìa pì gnun-a notìfica ëd modìfiche che a podèisso riveje dapress a costa.
Che as visa che a peul cangeje ij setagi dle notìfiche a le pàgine che as ten sot-euj ansima a soa lista dla ròba da ten-e sot euj.

             Comunicassion dël sistema ëd notìfica da {{SITENAME}}

--
Për cangé ij setagi ëd lòn che as ten sot euj che a vada ansima a
{{fullurl:Special:Watchlist/edit}}

Për fé dle comunicassion ëd servissi e avej pì d\'agiut:
{{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => 'Scancela pàgina',
'confirm'                     => 'Conferma',
'excontent'                   => "Ël contnù a l'era: '$1'",
'excontentauthor'             => "ël contnù a l'era: '$1' (e l'ùnich contributor a l'era stait '$2')",
'exbeforeblank'               => "Anans d'esse dësvojdà ël contnù a l'era: '$1'",
'exblank'                     => "La pàgina a l'era veujda",
'delete-confirm'              => 'Scancela "$1"',
'delete-legend'               => 'Scancela',
'historywarning'              => "Avis: la pàgina che a l'é antramentr che a scancela a l'ha na stòria:",
'confirmdeletetext'           => "A sta për scancelé d'autut da 'nt la base dat na pàgina ò pura na figura, ansema a tuta soa cronologìa.<p>
Për piasì, che an conferma che sòn a l'é da bon sò but, che a as rend cont ëd le conseguense ëd lòn che a fa, e che sòn a resta an pien an régola con lòn che a l'é stabilì ant la [[{{MediaWiki:Policy-url}}]].",
'actioncomplete'              => 'Travaj fait e finì',
'deletedtext'                 => 'La pàgina "<nowiki>$1</nowiki>" a l\'é staita scancelà.
Che a varda $2 për na lista dle pàgine scancelà ant j\'ùltim temp.',
'deletedarticle'              => 'Scancelà "$1"',
'dellogpage'                  => 'Registr djë scancelament',
'dellogpagetext'              => "Ambelessì sota na lista dle pàgine scancelà ant j'ùltim temp.
Ij temp a son conforma a l'ora dël server.",
'deletionlog'                 => 'Registr djë scancelament',
'reverted'                    => 'Version prima butà torna sù',
'deletecomment'               => 'Rason dlë scancelament:',
'deleteotherreason'           => 'Rason àutra/adissional:',
'deletereasonotherlist'       => 'Àutra rason',
'deletereason-dropdown'       => "*Rason sòlite ch'as ëscancela la ròba
** a lo ciama l'àutor
** violassion dij drit d'autor
** vanadalism",
'delete-edit-reasonlist'      => 'Modifiché la rason dlë scancelament',
'delete-toobig'               => "Sta pàgina-sì a l'ha na stòria motobin longa, bele pì che $1 revision. Lë scancelassion ëd pàgine parej a l'é stàita limità për evité ch'as fasa darmagi për eror a {{SITENAME}}.",
'delete-warning-toobig'       => "Sta pàgina-sì a l'ha na stòria motobin longa, bele pì che $1 revision. A scancelela as peul fesse darmagi a j'operassion dla base dat ëd {{SITENAME}}; ch'a fasa euj a lòn ch'a fa.",
'rollback'                    => 'Dòvra na revision pì veja',
'rollback_short'              => 'Ripristinè',
'rollbacklink'                => "ripristiné j'archivi",
'rollbackfailed'              => "A l'é pa podusse ripristiné",
'cantrollback'                => "As peul pa tornesse a na version pì veja: l'ùltima modìfica a l'ha fala l'ùnich utent che a l'abia travajà a cost artìcol-sì.",
'alreadyrolled'               => "As peulo pa anulé j'Ultime modìfiche ëd [[:$1]]
faite da [[User:$2|$2]] ([[User talk:$2|Talk]]); Cheidun d'àutr a l'ha già modificà ò pura anulà le modìfiche a sta pàgina-sì.

L'ùltima modìfica a l'é staita faita da [[User:$3|$3]] ([[User talk:$3|Talk]]).",
'editcomment'                 => 'Ël coment dla modìfica a l\'era: "<i>$1</i>".', # only shown if there is an edit comment
'revertpage'                  => "Gavà via le modìfiche dl'utent [[Special:Contributions/$2|$2]] ([[User_talk:$2|Talk]]); ël contnù a l'é stait tirà andarè a l'ùltima version dl'utent [[User:$1|$1]]", # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => "Modìfiche anulà da $1; tirà andré a l'ùltima version da $2.",
'sessionfailure'              => "A-i son ëstaje dle gran-e con la session che a identìfica sò acess; ël sistema a l'ha nen eseguì l'ordin che a l'ha daje për precaussion. Che a torna andaré a la pàgina prima con ël boton \"andaré\" ëd sò programa ëd navigassion (browser), peuj che as carìa n'àutra vira costa pàgina-sì e che a preuva torna a fé lòn che vorìa fé.",
'protectlogpage'              => 'Registr dle protession',
'protectlogtext'              => "Ambelessì sota a-i é na lista d'event ëd protession e dësprotession ëd pàgine.
Ch'a varda la [[Special:ProtectedPages|Lista dle pàgine protegiùe]] për ës-ciairé le protession corente.",
'protectedarticle'            => '"[[$1]]" a l\'é protet',
'modifiedarticleprotection'   => 'A l\'é cambia-ie ël livel ëd protession për "[[$1]]"',
'unprotectedarticle'          => 'Dësprotegiù "[[$1]]"',
'protect-title'               => 'I soma antramentr che i protegioma "$1"',
'protect-legend'              => 'Che an conferma la protession',
'protectcomment'              => 'Motiv dla protession',
'protectexpiry'               => 'Scadensa:',
'protect_expiry_invalid'      => 'Scadensa pa bon-a.',
'protect_expiry_old'          => 'Scadensa già passà.',
'protect-unchain'             => 'Dësbloché ij permess ëd tramudé dla ròba',
'protect-text'                => 'Ambelessì a peul vardé e cangé ël livel ëd protession dla pàgina <strong><nowiki>$1</nowiki></strong>.',
'protect-locked-blocked'      => "Un a peul pa modifiché ij livel ëd protession antramentr ch'a l'é blocà chiel. Ambelessì a-i son le regolassion corente për la pàgina <strong>$1</strong>:",
'protect-locked-dblock'       => "Ij livej ëd protession as peulo nen cambiesse antramentr che la base dat a l'é blocà.
Ambelessì a-i son le regolassion corente për la pàgina <strong>$1</strong>:",
'protect-locked-access'       => "Sò cont a l'ha pa la qualìfica për podej cambié ij livej ëd protession.
Ambelessì a-i son le regolassion corente për la pàgina <strong>$1</strong>:",
'protect-cascadeon'           => "Sta pàgina për adess a l'é blocà përchè a-i intra an {{PLURAL:$1|la pàgina sì sota, ch'a l'ha|le pàgine sì sota, ch'a l'han}} na protession a sàut avisca. A peul cambie-je sò livel ëd protession a sta pàgina-sì ma lòn a tochërà pa la protession a sàut.",
'protect-default'             => '(stàndard)',
'protect-fallback'            => 'A-i va ël përmess "$1"',
'protect-level-autoconfirmed' => "Bloché j'utent nen registrà",
'protect-level-sysop'         => "mach për j'aministrator",
'protect-summary-cascade'     => 'a sàut',
'protect-expiring'            => 'scadensa: $1 (UTC)',
'protect-cascade'             => "Protege le pàgine ch'a fan part ëd costa (protession a sàut)",
'protect-cantedit'            => "A peul pa cambieje livel ëd protession a sta pàgina-sì, për via ch'a l'ha nen ël përmess dë modifichela.",
'restriction-type'            => 'Përmess',
'restriction-level'           => 'Livel ëd restrission',
'minimum-size'                => 'Amzura mìnima',
'maximum-size'                => 'Amzura màssima:',
'pagesize'                    => '(byte)',

# Restrictions (nouns)
'restriction-edit'   => 'Modìfica',
'restriction-move'   => 'Tramuda',
'restriction-create' => 'Creé',

# Restriction levels
'restriction-level-sysop'         => 'protegiùa',
'restriction-level-autoconfirmed' => 'mesa-protegiùa',
'restriction-level-all'           => 'tuti ij livej',

# Undelete
'undelete'                     => 'Pija andré na pàgina scancelà',
'undeletepage'                 => 'S-ciàira e pija andaré le pàgine scancelà',
'undeletepagetitle'            => "'''Lòn ch'a-i é ambelessì a son tute revision scancelà ëd [[:$1]]'''.",
'viewdeletedpage'              => 'Smon le pàgine scancelà',
'undeletepagetext'             => 'Le pàgine ambelessì sota a son staite scancelà, ma a resto ancó memorisà e donca as peulo pijesse andaré. La memòria a ven polidà passaje un pòch ëd temp.',
'undeleteextrahelp'            => "Për ripristiné la pàgina antrega, che a lassa tute le casele nen selessionà e che a jë sgnaca ansima a '''''Buta coma a l'era '''''.
Për ripristiné mach chèich-còs, che a selession-a lòn che a veul ripristiné anans che sgnaché. Ën sgacand-je ansima a '''''Veujda casele''''' peul polidesse d'amblé tute le casele selessionà e dësvojdé ël coment.",
'undeleterevisions'            => '{{PLURAL:$1|Na|$1}} revision memorisà',
'undeletehistory'              => "Se a pija andré st'articol-sì, ëdcò tute soe revision a saran pijaite andaré ansema a chiel ant soa cronologìa.<br />
Se a fussa mai staita creà na pàgina neuva con l'istess nòm dòp che la veja a l'era staita scancelà, le revision a saran buta ant la cronologìa e la version pùblica dla pàgina a sarà nen modificà.",
'undeleterevdel'               => "Ël dëscancelament as farà pa s'a-i intrèissa në scancelament parsial dla version corenta dla pàgina. Quand a-i riva lolì, un a dev gave-ie la crosëtta da 'nt la pì neuva dle version scancelà, ò pura gavela da stërmà.",
'undeletehistorynoadmin'       => "Sta pàgina-sì a l'é staita scancelà.
Ël motiv che a l'é scancelasse as peul savejsse ën vardand ël somari ambelessì sota, andoa che a së s-ciàira ëdcò chi che a 
l'avìa travaje ansima anans che a la scancelèisso.
Ël test che a-i era ant le vàire version a peulo s-ciairelo mach j'aministrator.",
'undelete-revision'            => 'Revision scancelà ëd $1 (dël $2) da $3:',
'undeleterevision-missing'     => "Revision nen bon-a ò ch'a-i é nen d'autut. A peul esse ch'a l'abia n'anliura cioca, ma a peul ëdcò esse che la revision a la sia staita dëscancelà ò gavà via da 'nt la base dat.",
'undelete-nodiff'              => 'Pa gnun-a revision anans ëd costa.',
'undeletebtn'                  => 'Ripristiné',
'undeletelink'                 => 'buté andré',
'undeletereset'                => 'Gava tute le selession',
'undeletecomment'              => 'Coment:',
'undeletedarticle'             => 'Pijaita andré "$1"',
'undeletedrevisions'           => '{{PLURAL:$1|Na revision pijaita|$1 revision pijaite}} andré',
'undeletedrevisions-files'     => "{{PLURAL:$1|Na|$1}} revision e {{PLURAL:$2|n'|$2&nbsp;}}archivi pijait andré",
'undeletedfiles'               => "{{PLURAL:$1|N'|$1&nbsp;}}archivi pijait andaré",
'cannotundelete'               => "Disdëscancelament falì; a peul esse che i fusse antra doi a felo ant l'istess temp e l'àutr a sia riva prima.",
'undeletedpage'                => "<big>'''$1 a l'é stait pijait andaré'''</big>

Che as varda ël [[Special:Log/delete|Registr djë scancelament]] për ës-ciairé j'ùltim scancelament e disdëscancelament.",
'undelete-header'              => "Ch'a varda [[Special:Log/delete|ël registr djë scancelament]] për ës-ciairé j'ùltim dëscancelament.",
'undelete-search-box'          => 'Arsërca ant le pàgine scancelà',
'undelete-search-prefix'       => "Smon le pàgine ch'as anandio për:",
'undelete-search-submit'       => 'Sërca',
'undelete-no-results'          => "A-i é pa gnun-a pàgina parej ant l'archivi djë scancelassion.",
'undelete-filename-mismatch'   => "As peul nen disdëscancelesse la revision d'archivi col marcatemp $1: sòrt d'archivi nen giusta",
'undelete-bad-store-key'       => "As peul pa disdëscancelesse la revision d'archivi col marcatemp $1: l'archivi a-i era già pì anans d'esse scancelà.",
'undelete-cleanup-error'       => 'Eror ën scanceland l\'archivi nen dovrà "$1".',
'undelete-missing-filearchive' => "As peul nen ricuperesse l'archivi con l'identità $1 përchè a-i é pa ant la base dat. A peul esse ch'a l'abio già disdëscancelalo.",
'undelete-error-short'         => "Eror ën disdëscanceland l'archivi: $1",
'undelete-error-long'          => "Eror antramentr ch'as disdëscancelava l'archivi:

$1",

# Namespace form on various pages
'namespace'      => 'Spassi nominal:',
'invert'         => 'Anvert la selession',
'blanknamespace' => '(Prinsipal)',

# Contributions
'contributions' => "Contribussion dë st'Utent-sì",
'mycontris'     => 'Mie contribussion',
'contribsub2'   => 'Për $1 ($2)',
'nocontribs'    => "A l'é pa trovasse gnun-a modìfica che a fussa conforma a costi criteri-sì",
'uctop'         => ' (ùltima dla pàgina)',
'month'         => 'Mèis:',
'year'          => 'Ann:',

'sp-contributions-newbies'     => 'Smon mach ël travaj dij cont neuv',
'sp-contributions-newbies-sub' => "Për j'utent neuv",
'sp-contributions-blocklog'    => "Fërma l'agiornament dij registr",
'sp-contributions-search'      => 'Arsërca contribussion',
'sp-contributions-username'    => 'Adrëssa IP ò nòm utent:',
'sp-contributions-submit'      => 'Arsërca',

# What links here
'whatlinkshere'       => "Pàgine con dj'anliure che a men-o a costa-sì",
'whatlinkshere-title' => "Pàgine ch'a men-o a $1",
'whatlinkshere-page'  => 'Pàgina:',
'linklistsub'         => "(Lista d'anliure)",
'linkshere'           => "Le pàgine sì sota a l'han andrinta dj'anliure che a men-o a '''[[:$1]]''':",
'nolinkshere'         => "A-i é pa gnun-a pàgina che a l'abia dj'anliure che a men-o a '''[[:$1]]'''.",
'nolinkshere-ns'      => "An stë spassi nominal-sì a-i è pa gnun-a pagina con dj'anliure ch'a men-o a '''[[:$1]]'''.",
'isredirect'          => 'ridiression',
'istemplate'          => 'inclusion',
'whatlinkshere-prev'  => "{{PLURAL:$1|d'un andré|andré ëd $1}}",
'whatlinkshere-next'  => "{{PLURAL:$1|d'un anans|anans ëd $1}}",
'whatlinkshere-links' => '← anliure',

# Block/unblock
'blockip'                     => "Blochè n'adrëssa IP",
'blockip-legend'              => "Bloché l'utent",
'blockiptext'                 => "Che a dòvra ël mòdulo ëd domanda 'd blocagi ambelessì sota për bloché l'acess con drit dë scritura da na chèich adrëssa IP.<br />
Ës blocagi-sì as dev dovresse MACH për evité dij comportament vandàlich, ën strèita osservansa ëd tùit ij prinsipi dla [[{{MediaWiki:Policy-url}}|polìtica ëd {{SITENAME}}]].<br />
Ël blocagi a peul nen ën gnun-a manera esse dovrà për dle question d'ideologìa.

Che a scriva codì che st'adrëssa IP-sì a dev second chiel (chila) esse blocà (pr'esempi, che a buta ij tìtoj ëd pàgine che a l'abio già patì dj'at vandàlich da cost'adrëssa IP-sì).",
'ipaddress'                   => 'Adrëssa IP',
'ipadressorusername'          => 'Adrëssa IP ò stranòm',
'ipbexpiry'                   => 'Fin-a al',
'ipbreason'                   => 'Motiv dël blocagi',
'ipbreasonotherlist'          => 'Àotr motiv',
'ipbreason-dropdown'          => "*Motiv sòlit për ij blòch
** Avej butà d'anformassion fàosse
** Avej gavà contnù da 'nt le pàgine
** Buté porcherìa coma anliure ëd reclam
** Avej butà test sensa sust ant le pàgine
** Avej un deuit da bërsach con la gent
** Avej dovrà vàire cont fòra dij deuit
** Stranòm ch'as peul nen acetesse",
'ipbanononly'                 => "Blòca mach j'utent anònim",
'ipbcreateaccount'            => 'Lassa pa pi creé dij cont neuv',
'ipbemailban'                 => "Nen lassé che l'utent a peula mandé ëd messagi ëd pòsta eletrònica",
'ipbenableautoblock'          => "Blòca an automàtich la dariera adrëssa IP dovrà da l'utent e tute cole dont peuj cheidun as preuva a fé dle modìfiche",
'ipbsubmit'                   => "Bloca st'adrëssa IP-sì",
'ipbother'                    => "N'àutra durà",
'ipboptions'                  => "2 ore:2 ore,1 dì:1 dì,3 dì:3 dì,na sman-a:na sman-a,2 sman-e:2 sman-e,1 mèis:1 mèis,3 mèis:3 mèis,6 mèis:6 mèis,n'ann:n'ann,për sempe:për sempe", # display1:time1,display2:time2,...
'ipbotheroption'              => "d'àutr",
'ipbotherreason'              => 'Àotri motiv/spiegon',
'ipbhidename'                 => "Stërma lë stranòm da 'nt ël registr dij blòch, da col dij blòch ativ e da 'nt la lista dj'utent",
'badipaddress'                => "L'adrëssa IP che a l'ha dane a l'é nen giusta.",
'blockipsuccesssub'           => 'Blocagi fait',
'blockipsuccesstext'          => ' L\'adrëssa IP "$1" a l\'é staita blocà.<br />
Che a varda la [[Special:IPBlockList|lista dj\'IP blocà]].',
'ipb-edit-dropdown'           => 'Motiv dël blòch',
'ipb-unblock-addr'            => 'Dësbloché $1',
'ipb-unblock'                 => "Dësbloché n'utent ò n'adrëssa IP",
'ipb-blocklist-addr'          => 'Vardé ij blòch për $1',
'ipb-blocklist'               => 'Vardé ij blòch ativ',
'unblockip'                   => "Dësblòca st'adrëssa IP-sì",
'unblockiptext'               => "Che a dòvra ël mòdulo ëd domanda ambelessì sota për deje andé al drit dë scritura a n'adrëssa IP che a l'era staita blocà.",
'ipusubmit'                   => "Dësblòca st'adrëssa IP-sì",
'unblocked'                   => "[[User:$1|$1]] a l'é stait dësblocà",
'unblocked-id'                => "Ël blòch $1 a l'é stait gavà via.",
'ipblocklist'                 => "Lista dj'adrësse IP blocà",
'ipblocklist-legend'          => "Trové n'utent blocà",
'ipblocklist-username'        => 'Stranòm ò pura adrëssa IP:',
'ipblocklist-submit'          => 'Arsërca',
'blocklistline'               => "$1, $2 a l'ha blocà $3 ($4)",
'infiniteblock'               => 'për sempe',
'expiringblock'               => 'fin-a al $1',
'anononlyblock'               => "mach j'utent anònim",
'noautoblockblock'            => 'blòch automàtich nen ativ',
'createaccountblock'          => 'creassion dij cont blocà',
'emailblock'                  => 'pòsta eletrònica blocà',
'ipblocklist-empty'           => "La lista dij blòch a l'é veujda.",
'ipblocklist-no-results'      => "L'adrëssa IP ò lë stranòm ch'a l'ha ciamà a l'é pa blocà.",
'blocklink'                   => 'blòca',
'unblocklink'                 => 'dësblòca',
'contribslink'                => 'contribussion',
'autoblocker'                 => "A l'é scataje un blocagi përchè soa adrëssa IP a l'é staita dovrà ant j'ùltim temp da l'Utent \"[[User:\$1|\$1]]\". Ël motiv për bloché \$1 a l'é stait: \"'''\$2'''\"",
'blocklogpage'                => 'Registr dij blocagi',
'blocklogentry'               => '"[[$1]]" a l\'é stait blocà për $2 $3',
'blocklogtext'                => "Sossì a l'é ël registr dij blocagi e dësblocagi dj'Utent. J'adrësse che
a son staite blocà n'automàtich ambelessì a së s-ciàiro nen.
Che a varda la [[Special:IPBlockList|lista dj'adrësse IP blocà]] për vëdde
coj che sio ij blocagi ativ al dì d'ancheuj.",
'unblocklogentry'             => "a l'ha dësblocà $1",
'block-log-flags-anononly'    => 'mach utent anònim',
'block-log-flags-nocreate'    => 'creassion ëd cont neuv blocà',
'block-log-flags-noautoblock' => "blòch n'autòmatich dësmòrt",
'block-log-flags-noemail'     => 'pòsta eletrònica blocà',
'range_block_disabled'        => "La possibilità che n'aministrator a fasa dij blocagi a ragg a l'é disabilità.",
'ipb_expiry_invalid'          => 'Temp dë scadensa nen bon.',
'ipb_already_blocked'         => 'L\'utent "$1" a l\'è già blocà',
'ipb_cant_unblock'            => 'Eror: As treuva nen ël blòch con identificativ $1. A peul esse che a sia un blòch già gavà via.',
'ipb_blocked_as_range'        => "Eror: L'adrëssa IP $1 a l'ha gnun blòch diret ansima e donca a peul pa esse dësblocà. A resta blocà mach për via ch'a l'é ciapà andrinta al ragg $2, e lolì as peul pa dësblochesse.",
'ip_range_invalid'            => 'Nùmer IP nen bon.',
'blockme'                     => 'Blòch-me',
'proxyblocker'                => "Bloché j'arpetitor (Proxy) doèrt",
'proxyblocker-disabled'       => "Sta funsion-sì a l'é pa abilità.",
'proxyblockreason'            => "Soa adrëssa IP a l'é staita bloca përchè a l'é cola ëd n'arpetitor (proxy) doèrt. Për piasì che a contata al sò fornitor ëd conession e che a lo anforma. As trata d'un problema ëd siguressa motobin serio.",
'proxyblocksuccess'           => 'Bele fait.',
'sorbsreason'                 => "Soa adrëssa IP a l'é listà coma arpetitor doèrt (open proxy) ansima al DNSBL dovrà da {{SITENAME}}.",
'sorbs_create_account_reason' => "Soa adrëssa IP a l'é listà coma arpetitor doèrt (open proxy) ansima al DNSBL dovrà da {{SITENAME}}. A peul nen creésse un cont.",

# Developer tools
'lockdb'              => 'Blòca la base dat',
'unlockdb'            => 'Dësblòca la base dat',
'lockdbtext'          => "Ën blocand la base dat as fërma la possibilità che tuti j'Utent a peulo modifiché le pàgine ò pura fene 'd neuve, che a peulo cambiesse ij \"sò gust\", che a peulo modifichesse soe liste dla ròba da tnì sot euj, e an general gnun a podrà pì fé dj'operassion che a ciamo dë modifiché la base dat.<br /><br />
Për piasì, che an conferma che sossì a l'é pròpe lòn che a veul fé, e dzortut che a sblocherà la base dat pì ampressa che a peul, an manera che tut a funsion-a torna coma che as dev, pen-a che a l'avrà finisse soa manutension.",
'unlockdbtext'        => "Ën dësblocand la base dat as darà andaré a tuti j'Utent la possibilità dë fé 'd modìfiche a le pàgine ò dë fene ëd neuve, ëd cangé ij \"sò gust\", ëd modifiché soe liste 'd ròba da tnì sot euj, e pì an general dë fé tute cole operassion che a l'han da manca dë fé 'd modìfiche a la base dat.
Për piasì, che an conferma che sòn a l'é da bon lòn che chiel (chila) a veul fé.",
'lockconfirm'         => 'É, i veuj da bon, e sota mia responsabilità, bloché la base dat.',
'unlockconfirm'       => ' É, da bon i veuj dësbloché la base dat, sota mia responsabilità personal.',
'lockbtn'             => 'Blòca la base dat',
'unlockbtn'           => 'Dësblòca la base dat',
'locknoconfirm'       => "Che a varda che a l'é dësmentiasse dë spunté ël quadrèt ëd conferma.",
'lockdbsuccesssub'    => 'Blocagi dla base dat fait',
'unlockdbsuccesssub'  => "Dësblocagi dla base dat fait, ël blòch a l'é stait gavà",
'lockdbsuccesstext'   => "La base dat ëd {{SITENAME}} a l'è staita blocà.
<br />Che as visa mach dë gavé ël blocagi pen-a che a l'ha finì soa manutension.",
'unlockdbsuccesstext' => " La base dat ëd {{SITENAME}} a l'è staita dësblocà.",
'lockfilenotwritable' => "As peul nen ëscrive ant sl'archivi ëd blòch dla base dat. A fa dë manca d'avej n'acess an scritura a st'archivi për podej bloché e dësbloché la base dat.",
'databasenotlocked'   => "La base dat a l'é nen blocà.",

# Move page
'move-page'               => 'Tramud ëd $1',
'move-page-legend'        => 'Tramudé na pàgina',
'movepagetext'            => "Con ël mòdulo ëd domanda ambelessì sota a peul cangeje nòm a na pàgina, tramudand-je dapress ëdcò tuta soa cronologìa anvers al nòm neuv.
Ël vej tìtol a resterà trasformà ant na ridiression che a men-a al tìtol neuv.
J'anliure a la veja pàgina a saran NEN agiornà (e donca a men-eran la gent a la ridiression);
che a fasa atension dë
controlé con cura che as creo pa dle ridiression dobie ò dle ridiression che men-o da gnun-a part.
A resta soa responsabilità cola dë esse sigur che j'anliure a men-o la gent andoa che a devo mnela.

Noté bin: la pàgina a sarà '''nen''' tramudà se a-i fussa già mai n'articol che a l'ha ël nòm neuv, gavà col cas che a sia na pàgina veujda ò pura na ridiression, sempre che bele che essend mach parej a l'abia già nen na soa cronologìa.
Sòn a veul dì che, se a l'avèissa mai da fé n'operassion nen giusta, a podrìa sempe torné a rinominé la pàgina col nòm vej, ma ant gnun cas a podrìa coerté na pàgina che a-i é già.

'''ATENSION!'''
Un cambiament dràstich parej a podrìa dé dle gran-e che un a së speta pa gnanca.
Sòn dzortut se a fussa fait dzora a na pàgina motobin visità.
Che a varda mach dë esse pì che sigur d'avej presente le conseguense, prima che fé che fé.
Se a l'ha dij dùbit, che a contata pura n'aministrator për ciameje 'd consej.",
'movepagetalktext'        => "La pàgina ëd discussion tacà a costa pàgina d'articol, se a-i é, a sarà tramudà n'automatich ansema a l'artìcol, '''gavà costi cas-sì''':
*quand as tramuda la pàgina tra diferent spassi nominal,
*quand na pàgina ëd discussion nen veujda a-i é già për ël nòm neuv, ò pura
*a l'ha deselessionà ël quadrèt ëd conferma ambelessì sota.
Ant costi cas-sì, se a chërd dë felo, a-j farà da manca dë tramudesse la pàgina ëd discussion daspërchiel, a man.",
'movearticle'             => "Cang-je nòm a l'artìcol",
'movenotallowed'          => "A l'ha pa ij përmess dont a fa da manca për tramudé pàgine.",
'newtitle'                => 'Neuv tìtol ëd',
'move-watch'              => 'Ten sot euj sta pàgina-sì',
'movepagebtn'             => 'Tramuda sta pàgina-sì',
'pagemovedsub'            => 'San Martin bele finì!',
'articleexists'           => "Na pàgina che as ciama parej a-i é già, ò pura ël nòm che a l'ha sërnù a va nen bin.<br />
Che as sërna, për piasì, un nòm diferent për st'articol.",
'cantmove-titleprotected' => "As peul pa fesse San Martin ambelelì, për via che col tìtol-lì a l'é stàit proibì e a peul pa ess-ie na pàgina ciamà parej",
'talkexists'              => "La pàgina a l'é staita bin tramudà, ma a l'é pa podusse tramudé soa pàgina ëd discussion, përchè a-i në j'é già n'àutra ant la pàgina con ël tìtol neuv. Për piasì, che a modìfica a man ij contnù dle doe pàgine ëd discussion, an manera che as perdo nen dij pensé anteressant.",
'movedto'                 => 'tramudà a',
'movetalk'                => "Podend, tramuda ëdcò la pàgina ëd discussion che a l'ha tacà.",
'1movedto2'               => '[[$1]] Tramudà a [[$2]]',
'1movedto2_redir'         => '[[$1]] tramudà a [[$2]] ën passand për na ridiression',
'movelogpage'             => 'Registr dij San Martin',
'movelogpagetext'         => 'Ambelessì sota a-i é na lista ëd pàgine che a son staite tramudà.',
'movereason'              => 'Motiv',
'revertmove'              => "buta torna coma a l'era",
'delete_and_move'         => 'Scancela e tramuda',
'delete_and_move_text'    => '==A fa da manca dë scancelé==

L\'artìcol ëd destinassion "[[:$1]]" a-i é già. Veul-lo scancelelo për avej ëd pòst për tramudé l\'àutr?',
'delete_and_move_confirm' => 'É, scancela la pàgina',
'delete_and_move_reason'  => "Scancelà për liberé ël pòst për tramudene n'àutra",
'selfmove'                => "Tìtol neuv e tìtol vej a resto midem antra lor; as peul pa tramudesse na pàgina butand-la andoa che a l'é già.",
'immobile_namespace'      => "Belavans ël tìtol ëd destinassion a l'é ëd na sòrt riservà; as peulo pa tramudé dle pàgine anvers a col ëspassi nominal-lì.",

# Export
'export'            => 'Esporté dle pàgine',
'exporttext'        => "A peul esporté ël test e modifiché la stòria ëd na pàgina ò pura
ëd n'ansema ëd pàgine gropa ant n'archivi XML. Sòn a peul peuj amportesse
ant n'àutra wiki ën dovrand la funsion Special:Ampòrta pàgina.

Për esporté le pàgine, che a së scriva ij tìtoj ant ël quàder ambelessì sota, butand-ji un tìtol për riga,
e che as serna se a veul la version corenta ansema a cole veje, con le righe che conto la stòria dla pàgina,
ò pura mach l'anformassion ant sël quand che a sia staje l'ùltima modìfica.

Se costa ùltima possibilità a fussa lòn che a-j serv, a podrìa ëdcò dovré n'anliura, pr'esempi [[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]] për la pàgina \"[[{{MediaWiki:Mainpage}}]]\".",
'exportcuronly'     => 'Ciapa sù mach la version corenta, pa tuta la stòria',
'exportnohistory'   => "----
'''Nòta:''' la possibilità d'esporté la stòria completa dle pàgine a l'é staita gavà për dle question corelà a le prestassion dël sistema.",
'export-submit'     => 'Esporté',
'export-addcattext' => "Gionta pàgine da 'nt la categorìa:",
'export-addcat'     => 'Gionta',
'export-download'   => 'Ciamé dë salvelo coma archivi',
'export-templates'  => 'Ciapa andrinta jë stamp',

# Namespace 8 related
'allmessages'               => 'Messagi ëd sistema',
'allmessagesname'           => 'Nòm',
'allmessagesdefault'        => "Test che a-i sarìa se a-i fusso pa 'd modìfiche",
'allmessagescurrent'        => 'Test corent',
'allmessagestext'           => "Costa-sì a l'é na lista ëd tùit ij messagi ëd sistema ant lë spassi nominal MediaWiki:",
'allmessagesnotsupportedDB' => "'''{{ns:special}}:Allmessages''' a travaja nen përchè a-i é ël component '''\$wgUseDatabaseMessages''' frëmm.",
'allmessagesfilter'         => 'Seletor dël nòm dël messagi:',
'allmessagesmodified'       => "Most-me mach lòn che a l'é modificasse",

# Thumbnails
'thumbnail-more'           => 'Slarga',
'filemissing'              => 'Archivi che a manca',
'thumbnail_error'          => 'Eror antramentr che as fasìa la figurin-a: $1',
'djvu_page_error'          => 'Pàgina DjVu fòra dij lìmit',
'djvu_no_xml'              => "As rièss pa a carié l'XML për l'archivi DjVu",
'thumbnail_invalid_params' => 'Paràmetro dla figurin-a pa giust',
'thumbnail_dest_directory' => 'As peul pa fesse ël dossié ëd destinassion',

# Special:Import
'import'                     => 'Amportassion ëd pàgine',
'importinterwiki'            => 'Amportassion da wiki diferente',
'import-interwiki-text'      => "Che a selession-a na wiki e ël tìtol dla pàgina da amporté.
Date dle revision e stranòm dj'editor a resteran piajit sù 'cò lor.
Tute le amportassion antra wiki diferente a resto marcà ant ël [[Special:Log/import|Registr dj'amportassion]].",
'import-interwiki-history'   => 'Còpia tute le version stòriche dë sta pàgina-sì',
'import-interwiki-submit'    => 'Amporté',
'import-interwiki-namespace' => 'Tramuda ste pàgine-sì ant lë spassi nominal:',
'importtext'                 => "Për piasì, che as espòrta l'archivi da 'nt la sorgiss wiki esterna ën dovrand l'utiss  Special:Esportassion, che as lo salva ansima a sò disch e peui che a lo caria ambelessì.",
'importstart'                => 'I soma antramentr che amportoma le pàgine...',
'import-revision-count'      => '{{PLURAL:$1|Na|$1}} revision',
'importnopages'              => 'Pa gnun-a pàgina da amporté',
'importfailed'               => 'Amportassion falìa: $1',
'importunknownsource'        => "Sorgiss d'amportassion ëd na sòrt nen conossùa",
'importcantopen'             => "L'archivi da amporté a l'é pa podusse deurbe",
'importbadinterwiki'         => 'Anliura antra wiki diferente malfaita',
'importnotext'               => 'Veujd ò sensa pa gnun test',
'importsuccess'              => 'Amportassion andaita a bon fin!',
'importhistoryconflict'      => "A-i son dle stòrie dë sta pàgina-sì che as contradisso un-a con l'àutra (a peul esse che sta pàgina-sì a l'avèissa già amportala)",
'importnosources'            => "A l'é pa staita definìa gnun-a sorgiss d'amportassion da na wiki diferenta, e carié mach le stòrie as peul nen.",
'importnofile'               => "Pa gnun archivi d'amportassion carià.",
'importuploaderrorsize'      => "A l'é falìe la caria dl'archivi d'amporté. L'archivi a resta pì gròss che lòn ch'as peul cariesse.",
'importuploaderrorpartial'   => "A l'é falìe la caria dl'archivi d'amporté. L'archivi a resta carià mach për un tòch.",
'importuploaderrortemp'      => "A l'é falìe la caria dl'archivi d'amporté. A-i manca un dossié provisòri.",
'import-parse-failure'       => "Eror dë scomposission XML ant l'amportassion",
'import-noarticle'           => "Pa gnun-a pàgina d'amporté.",
'import-nonewrevisions'      => "Tute le revision a l'ero già stàite amportà.",
'xml-error-string'           => '$1 ant la riga $2, colòna $3 (byte $4): $5',

# Import log
'importlogpage'                    => "Registr dj'amportassion",
'importlogpagetext'                => "Amportassion aministrative ëd pàgine e ëd soa stòria da dj'àutre wiki.",
'import-logentry-upload'           => "amportà $1 con un càrich d'archivi",
'import-logentry-upload-detail'    => '$1 revision',
'import-logentry-interwiki'        => "Amportà da n'àutra wiki $1",
'import-logentry-interwiki-detail' => '$1 revision da $2',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'Mia pàgina Utent.',
'tooltip-pt-anonuserpage'         => 'Pàgina Utent për l',
'tooltip-pt-mytalk'               => 'Mia pàgina ëd discussion e ciaciarade.',
'tooltip-pt-anontalk'             => 'Pàgina ëd ciaciarade për l',
'tooltip-pt-preferences'          => 'Coma che i veuj mia {{SITENAME}}.',
'tooltip-pt-watchlist'            => 'Lista dle pàgine che chiel as ten sot euj.',
'tooltip-pt-mycontris'            => 'Sòn i l',
'tooltip-pt-login'                => "Un a l'é nen obligà a rintré ant al sistema, ma se a lo fa a l",
'tooltip-pt-anonlogin'            => "Un a l'é nen obligà a rintré ant al sistema, ma se a lo fa a l",
'tooltip-pt-logout'               => 'Seurte da',
'tooltip-ca-talk'                 => 'Discussion ansima a sta pàgina ëd contnù.',
'tooltip-ca-edit'                 => 'Modifiché sta pàgina-sì. Për piasì, che as fasa na preuva anans che salvé .',
'tooltip-ca-addsection'           => 'Gionteje un coment a sta discussion-sì.',
'tooltip-ca-viewsource'           => 'Sta pàgina-sì a l',
'tooltip-ca-history'              => 'Veje version dla pàgina.',
'tooltip-ca-protect'              => 'Për protege sta pàgina-sì.',
'tooltip-ca-delete'               => 'Scancelé sta pàgina-sì',
'tooltip-ca-undelete'             => 'Pijé andré le modìfiche faite a sta pàgina-sì, anans che a fussa scancelà.',
'tooltip-ca-move'                 => 'Tramudé sta pàgina, visadì cangeje tìtol.',
'tooltip-ca-watch'                => 'Gionté sta pàgina-sì a la lista dle ròbe che as ten-o sot euj.',
'tooltip-ca-unwatch'              => 'Gavé via sta pàgina da',
'tooltip-search'                  => 'Sërca an {{SITENAME}}',
'tooltip-search-go'               => "Andé a na pàgina ch'as ciama parej, sempe ch'a-i në sia un-a",
'tooltip-search-fulltext'         => 'Sërché ës test-sì antra le pàgine dël sit',
'tooltip-p-logo'                  => 'Pàgina prinsipal.',
'tooltip-n-mainpage'              => 'Visité la pàgina prinsipal.',
'tooltip-n-portal'                => 'Rësguard al proget, lòn che a peul fé, andoa trové còsa.',
'tooltip-n-currentevents'         => 'Informassion ansima a lòn che a-i riva.',
'tooltip-n-recentchanges'         => 'Lista dj',
'tooltip-n-randompage'            => 'Carié na pàgina basta che a sia.',
'tooltip-n-help'                  => 'Ël pòst për capì.',
'tooltip-t-whatlinkshere'         => 'Lista ëd tute le pàgine dla wiki che a men-o ambelessì.',
'tooltip-t-recentchangeslinked'   => 'Ùltime modìfiche dle pàgine andoa as peul andesse da costa.',
'tooltip-feed-rss'                => 'RSS feed për sta pàgina-sì.',
'tooltip-feed-atom'               => 'Atom feed për sta pàgina-sì.',
'tooltip-t-contributions'         => 'Vardé la lista dle contribussion dë st',
'tooltip-t-emailuser'             => 'Mandeje un messagi ëd pòsta a st',
'tooltip-t-upload'                => 'Carié archivi ëd figure ò son.',
'tooltip-t-specialpages'          => 'Lista ëd tute le pàgine speciaj.',
'tooltip-t-print'                 => 'Version bon-a da stampé dë sta pàgina',
'tooltip-t-permalink'             => 'Anliura fissa a sta version-i dla pàgina',
'tooltip-ca-nstab-main'           => 'Vardé la pàgina ëd contnù.',
'tooltip-ca-nstab-user'           => 'Vardé la pàgina Utent.',
'tooltip-ca-nstab-media'          => 'Vardé la pàgina dl',
'tooltip-ca-nstab-special'        => 'Costa a l',
'tooltip-ca-nstab-project'        => 'Vardé la pàgina proteta.',
'tooltip-ca-nstab-image'          => 'Vardé la pàgina dl',
'tooltip-ca-nstab-mediawiki'      => 'Vardé ël messagi ëd sistema.',
'tooltip-ca-nstab-template'       => 'Vardé lë stamp.',
'tooltip-ca-nstab-help'           => 'Vardé la pàgina d',
'tooltip-ca-nstab-category'       => 'Vardé la pàgina dla categorìa.',
'tooltip-minoredit'               => 'Marca sossì coma modìfica cita',
'tooltip-save'                    => 'Salva le modìfiche',
'tooltip-preview'                 => 'Preuva dle modìfiche (mej sempe fela, prima che fé che salvé!)',
'tooltip-diff'                    => "Fame vëdde che modìfiche che i l'hai faje al test.",
'tooltip-compareselectedversions' => 'Fame ël paragon dle diferense antra le version selessionà.',
'tooltip-watch'                   => 'Gionta sta pàgina-sì a la lista dle ròbe che im ten-o sot euj',
'tooltip-recreate'                => 'Creé torna la pàgina contut che a la sia staita scancelà',
'tooltip-upload'                  => 'Anandiesse a carié',

# Stylesheets
'common.css'   => '/** Ël còdes CSS che as buta ambelessì a resta dovrà ant tute le "facie" */',
'monobook.css' => "/* cangé st'archivi-sì për modifiché la formatassion dël sit antregh */",

# Scripts
'common.js'   => "/* Ël còdes JavaScript ch'as buta ambelessì a ven carià da vira utent për vira pàgina */",
'monobook.js' => "/* Ës messagi-sì as dovrìa pa pì dovrelo; a sò pòst ch'a dòvra [[MediaWiki:common.js]] */",

# Metadata
'nodublincore'      => "Ij metadat dla sòrt '''Dublin Core RDF''' a son disabilità ansima a sta màchina-sì.",
'nocreativecommons' => "Ij metadat dla sòrt '''Creative Commons RDF''' a son disabilità ansima a sta màchina-sì.",
'notacceptable'     => 'Ël server dla wiki a-i la fa pa a provëdde dij dat ant na forma che sò programa local a peula lese.',

# Attribution
'anonymous'        => 'Utent anònim ëd la {{SITENAME}}',
'siteuser'         => '$1, utent ëd {{SITENAME}}',
'lastmodifiedatby' => "Sta pàgina-sì a l'é staita modificà l'ùltima vira al $2, $1 da $3.", # $1 date, $2 time, $3 user
'othercontribs'    => 'Basà ant sëj travaj ëd $1.',
'others'           => 'àutri',
'siteusers'        => '$1, utent ëd {{SITENAME}}',
'creditspage'      => 'Credit dla pàgina',
'nocredits'        => 'A-i é pa gnun crédit për sta pagina-sì.',

# Spam protection
'spamprotectiontitle' => 'Filtror dla rumenta',
'spamprotectiontext'  => "La pàgina che a vorìa salvé a l'é staita blocà dal filtror dla rumenta. Sòn a l'é motobin belfé che a sia rivà përchè a-i era n'anliura a un sit estern ëd coj blocà.",
'spamprotectionmatch' => "Cost-sì a l'é ël test che a l'é restà ciapà andrinta al filtror dla rumenta: $1",
'spambot_username'    => 'MediaWiki - trigomiro che a-j dà deuit a la rumenta',
'spam_reverting'      => "Buta andaré a l'ùltima version che a l'avèissa pa andrinta dj'anliure a $1",
'spam_blanking'       => "Pàgina dësveujdà, che tute le version a l'avìo andrinta dj'anliure a $1",

# Info page
'infosubtitle'   => 'Anformassion për la pàgina',
'numedits'       => 'Nùmer ëd modìfiche (artìcol): $1',
'numtalkedits'   => 'Nùmer ëd modìfiche (pàgina ëd discussion): $1',
'numwatchers'    => "Nùmer d'utent che as ten-o sossì sot euj: $1",
'numauthors'     => "Nùmer d'autor diferent (artìcol): $1",
'numtalkauthors' => "Nùmer d'autor distint (pàgina ëd discussion): $1",

# Math options
'mw_math_png'    => 'Most-lo sempe coma PNG',
'mw_math_simple' => "But-lo an HTML se a l'é motobin belfé a fesse, dësnò but-lo an PNG",
'mw_math_html'   => 'But-lo an HTML se as peul, dësnò an PNG',
'mw_math_source' => 'Lass-lo coma TeX (për ij programa ëd navigassion testual)',
'mw_math_modern' => 'As racomanda për ij programa ëd navigassion pì modern',
'mw_math_mathml' => 'But-lo an MathML se as peul (sperimental)',

# Patrolling
'markaspatrolleddiff'                 => 'Marca coma verificà',
'markaspatrolledtext'                 => "Marca st'artìcol-sì coma verificà",
'markedaspatrolled'                   => 'Marca dla verìfica butà',
'markedaspatrolledtext'               => "La version selessionà a l'é staita marcà coma verificà.",
'rcpatroldisabled'                    => "Verìfica dj'ùltime modìfiche disabilità",
'rcpatroldisabledtext'                => "La possibilità ëd verifichè j'ùltime modìfiche a l'é disabilità.",
'markedaspatrollederror'              => 'As peul pa marchè verificà',
'markedaspatrollederrortext'          => 'A venta che a specìfica che version che a veul marchè verificà.',
'markedaspatrollederror-noautopatrol' => 'A l\'ha nen ël përmess dë marchesse soe modìfiche coma "controlà".',

# Patrol log
'patrol-log-page' => 'Registr dij contròj',
'patrol-log-line' => "a l'ha marcà la $1 ëd $2 coma controlà $3",
'patrol-log-auto' => '(automàtich)',
'patrol-log-diff' => 'modìfica $1',

# Image deletion
'deletedrevision'                 => 'Veja version scancelà $1',
'filedeleteerror-short'           => "Eror ën scanceland l'archivi: $1",
'filedeleteerror-long'            => "A son ësta-ie dj'eror ën scanceland l'archivi:

$1",
'filedelete-missing'              => 'L\'archivi "$1" as peul pa dëscancelesse, për via ch\'a-i é nen.',
'filedelete-old-unregistered'     => 'La revision d\'archivi specificà "$1" ant la base dat a-i é nen.',
'filedelete-current-unregistered' => 'Ant la base dat l\'archivi "$1" ch\'a l\'é specificasse a-i é pa.',
'filedelete-archive-read-only'    => 'Ël servent dla Ragnà a peul pa scriv-ie ant ël dossié dj\'archivi "$1".',

# Browsing diffs
'previousdiff' => '← Diferensa prima',
'nextdiff'     => 'Diferensa che a-i ven →',

# Media information
'mediawarning'         => "'''Atension!''': st'archivi-sì a podrìa avej andrinta dël còdes butà-lì da cheidun për fé ëd darmagi, e se parej a fussa, ën fasend-lo travajé ansima a sò calcolador chiel a podrìa porteje ëd dann a sò sistema.
<hr />",
'imagemaxsize'         => 'Ten le figure andrinta a le pàgine ëd descrission dle figure ant ël lìmit ëd:',
'thumbsize'            => 'Amzura dle figurin-e:',
'widthheightpage'      => '$1×$2, $3 pàgine',
'file-info'            => "(amzura dl'archivi: $1, sòrt MIME: $2)",
'file-info-size'       => '($1 × $2 pixel, amzure: $3, sòrt MIME: $4)',
'file-nohires'         => '<small>Gnun-a risolussion pì bela disponibila.</small>',
'svg-long-desc'        => "(archivi an forma SVG, amzure nominaj $1 × $2 pixel, amzura dl'archivi: $3)",
'show-big-image'       => 'Version a amzura pijn-a',
'show-big-image-thumb' => '<small>Amzure dë sta figurin-a: $1 × $2 pixel</small>',

# Special:NewImages
'newimages'             => 'Galerìa ëd figure e son neuv',
'imagelisttext'         => "Ambelessì sota a-i é {{PLURAL:$1|l'ùnica figura che a-i sia|na lista ëd '''$1''' figure, ordinà për $2}}.",
'newimages-summary'     => "Sta pàgina special-sì a la smon j'ùltim archivi carià.",
'showhidebots'          => '($1 trigomiro)',
'noimages'              => 'Pa gnente da vëdde.',
'ilsubmit'              => 'Sërca',
'bydate'                => 'për data',
'sp-newimages-showfrom' => "Smon j'ùltim archivi multimojen a anandiesse da $2 dël $1",

# Bad image list
'bad_image_list' => "La forma a l'é costa-sì:

As ten-o bon-e mach le liste pontà (cole faite ëd righe ch'as ancamin-o për *). La prima anliura ëd minca riga a l'ha da mné a n'archivi multimojen nen bon.
J'anliure ch'a-i ven-o dapress, ant sla midema riga, as conto për ecession (visadì, për pàgine andova st'archivi as peul butesse).",

# Metadata
'metadata'          => 'Dat adissionaj',
'metadata-help'     => "Costi-sì a son dij dat adissionaj, che a l'é belfé che a sio stait giontà da la màchina fotogràfica digital ò pura da lë scanner che a l'é stiat dovrà për creé la figura digital. Se la figura a fussa mai staita modificà da 'nt soa forma original, a podrìa ëdcò riveje che chèich detaj a fussa ancò butà coma ant l'original, donca sensa pa ten-e cont ëd le modìfiche.",
'metadata-expand'   => 'Most-me tùit ij dat',
'metadata-collapse' => 'Stërma ij dat adissionaj',
'metadata-fields'   => "Ij camp dij metadat EXIF lista ant ës messagi-sì a sarà smonù ant sla pàgina dla figura quand la tabela dij metadat a l'é scondùa. J'àotri a saran scondù.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength", # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => 'Larghëssa',
'exif-imagelength'                 => 'Autëssa',
'exif-bitspersample'               => 'Bit për campion',
'exif-compression'                 => 'Schema ëd compression',
'exif-photometricinterpretation'   => 'Composission dij pixel',
'exif-orientation'                 => 'Orientament',
'exif-samplesperpixel'             => 'Nùmer ëd component',
'exif-planarconfiguration'         => 'Sistemassion dij dat',
'exif-ycbcrsubsampling'            => 'Rapòrt ëd campionament antra Y e C',
'exif-ycbcrpositioning'            => 'Posissionament Y e C',
'exif-xresolution'                 => 'Risolussion orizontal',
'exif-yresolution'                 => 'Risolussion vertical',
'exif-resolutionunit'              => "Unità d'amzura për le coordinà X e Y",
'exif-stripoffsets'                => 'Posission dij dat dla figura',
'exif-rowsperstrip'                => 'Nùmer ëd righe për banda',
'exif-stripbytecounts'             => 'Bytes për banda compressa',
'exif-jpeginterchangeformat'       => 'Diferensa posissional anvers al SOI dël JPEG',
'exif-jpeginterchangeformatlength' => 'Byte ëd dat an formà JPEG',
'exif-transferfunction'            => 'Funsion ëd trasferiment',
'exif-whitepoint'                  => 'Pont cromàtich dël bianch',
'exif-primarychromaticities'       => 'Coordinà cromàtiche dij color primari',
'exif-ycbcrcoefficients'           => 'Coeficent dla matriss ëd trasformassion dlë spassi color',
'exif-referenceblackwhite'         => "Pàira ëd valor d'arferiment për bianch e nèir",
'exif-datetime'                    => 'Data e ora dle modìfiche',
'exif-imagedescription'            => 'Tìtol dla figura',
'exif-make'                        => 'Fabricant dla màchina fotogràfica ò videocàmera',
'exif-model'                       => 'Model dla màchina',
'exif-software'                    => 'Programa dovrà',
'exif-artist'                      => 'Autor',
'exif-copyright'                   => "Titolar dël drit d'autor",
'exif-exifversion'                 => 'Version dël formà Exif',
'exif-flashpixversion'             => 'A riva a la version Flashpix',
'exif-colorspace'                  => 'Spassi color',
'exif-componentsconfiguration'     => 'Sust ëd vira component',
'exif-compressedbitsperpixel'      => 'Sistema ëd compression dle figure',
'exif-pixelydimension'             => 'Larghëssa vàlida dla figura',
'exif-pixelxdimension'             => 'Autëssa vàlida dla figura',
'exif-makernote'                   => 'Nòte dël fabricant',
'exif-usercomment'                 => 'Nòte lìbere',
'exif-relatedsoundfile'            => 'Archivi audio colegà',
'exif-datetimeoriginal'            => 'Data e ora dla generassion dij dat',
'exif-datetimedigitized'           => 'Data e ora dla digitalisassion',
'exif-subsectime'                  => 'Data, ora e frassion ëd second',
'exif-subsectimeoriginal'          => 'Data e ora ëd creassion, con frassion ëd second',
'exif-subsectimedigitized'         => 'Data e ora ëd digitalisassion, con frassion ëd second',
'exif-exposuretime'                => "Temp d'esposission",
'exif-exposuretime-format'         => '$1 sec ($2)',
'exif-fnumber'                     => "Nùmer d'F",
'exif-exposureprogram'             => "Programa d'esposission",
'exif-spectralsensitivity'         => 'Sensibilità dë spetro',
'exif-isospeedratings'             => 'Sensibilità ISO',
'exif-oecf'                        => 'Fator ëd conversion optoeletrònica',
'exif-shutterspeedvalue'           => 'Temp dë scat',
'exif-aperturevalue'               => 'Diaframa',
'exif-brightnessvalue'             => 'Luminosità',
'exif-exposurebiasvalue'           => "Coression dl'esposission",
'exif-maxaperturevalue'            => 'Apertura màssima',
'exif-subjectdistance'             => 'Distansa dël soget',
'exif-meteringmode'                => "Càlcol dl'espossision",
'exif-lightsource'                 => "Sorgiss d'anluminassion",
'exif-flash'                       => 'Flash',
'exif-focallength'                 => 'Lunghëssa focal dle lent',
'exif-subjectarea'                 => "Spassi d'anquadratura dël soget",
'exif-flashenergy'                 => 'Potensa dël flash',
'exif-spatialfrequencyresponse'    => 'Arspòsta an frequensa spassial',
'exif-focalplanexresolution'       => 'Resolussion dla coordinà X ant sël pian dla focal',
'exif-focalplaneyresolution'       => 'Resolussion dla coordinà Y ant sël pian dla focal',
'exif-focalplaneresolutionunit'    => "Unità d'amzura për ël pian dla focal",
'exif-subjectlocation'             => 'Posission dël soget',
'exif-exposureindex'               => "Ìndes dl'esposission",
'exif-sensingmethod'               => 'Metod ëd campionament',
'exif-filesource'                  => "Sorgiss dl'archivi",
'exif-scenetype'                   => "Sòrt d'anquadratura",
'exif-cfapattern'                  => 'Schema CFA',
'exif-customrendered'              => 'Process dla figura particolar',
'exif-exposuremode'                => "Modalità dl'esposission",
'exif-whitebalance'                => 'Balansa dël bianch',
'exif-digitalzoomratio'            => 'Rapòrt ëd lë zoom digital',
'exif-focallengthin35mmfilm'       => 'Lunghëssa focal an film da 35 mm',
'exif-scenecapturetype'            => 'Sistema ëd campionament',
'exif-gaincontrol'                 => 'Contròl ëd sienari',
'exif-contrast'                    => 'Contrast',
'exif-saturation'                  => 'Saturassion',
'exif-sharpness'                   => 'Definission dij bòrd',
'exif-devicesettingdescription'    => "Nòm dla configurassion dl'aparechiatura",
'exif-subjectdistancerange'        => 'Ragg ëd distansa dël soget',
'exif-imageuniqueid'               => 'Identificator ùnich dla figura',
'exif-gpsversionid'                => 'Version dël GPS',
'exif-gpslatituderef'              => 'Latitùdin setentrional ò meridional',
'exif-gpslatitude'                 => 'Latitùdin',
'exif-gpslongituderef'             => 'Longitùdin oriental ò ossidental',
'exif-gpslongitude'                => 'Longitùdin',
'exif-gpsaltituderef'              => "Arferiment d'autëssa",
'exif-gpsaltitude'                 => 'Autëssa',
'exif-gpstimestamp'                => 'Ora dël GPS (mostra atòmica)',
'exif-gpssatellites'               => "Satélit dovrà për l'amzura",
'exif-gpsstatus'                   => 'Condission dël ricevitor',
'exif-gpsmeasuremode'              => "Sistema d'amzura",
'exif-gpsdop'                      => "Precision dl'amzura",
'exif-gpsspeedref'                 => "Unità d'amzura për la velocità",
'exif-gpsspeed'                    => 'Velocità dël ricevitor GPS',
'exif-gpstrackref'                 => 'Arferiment për la diression dël moviment',
'exif-gpstrack'                    => 'Diression dël moviment',
'exif-gpsimgdirectionref'          => 'Arferiment për la diression dla figura',
'exif-gpsimgdirection'             => 'Diression dla figura',
'exif-gpsmapdatum'                 => "Dat dl'amzura geodética che a son dovrà",
'exif-gpsdestlatituderef'          => 'Arferiment për la latitùdin dla destinassion',
'exif-gpsdestlatitude'             => 'Latitùdin dla destinassion',
'exif-gpsdestlongituderef'         => 'Arferiment për la longitùdin dla destinassion',
'exif-gpsdestlongitude'            => 'Longitùdin dla destinassion',
'exif-gpsdestbearingref'           => "Arferiment për l'orientament a destinassion",
'exif-gpsdestbearing'              => 'Orientament anvers a la destinassion',
'exif-gpsdestdistanceref'          => "Arferiment për la lontanansa da 'nt la destinassion",
'exif-gpsdestdistance'             => "Lontanansa da 'nt la destinassion",
'exif-gpsprocessingmethod'         => 'Nòm dël sistema ëd process an GPS',
'exif-gpsareainformation'          => 'Nòm dlë spassi GPS',
'exif-gpsdatestamp'                => 'Data dël GPS',
'exif-gpsdifferential'             => 'Coression diferensial dël GPS',

# EXIF attributes
'exif-compression-1' => 'Pa compress',

'exif-unknowndate' => 'Data nen conossùa',

'exif-orientation-1' => 'Normal', # 0th row: top; 0th column: left
'exif-orientation-2' => 'Specolar', # 0th row: top; 0th column: right
'exif-orientation-3' => 'Arvirà ëd 180°', # 0th row: bottom; 0th column: right
'exif-orientation-4' => 'Arvirà dzorsuta', # 0th row: bottom; 0th column: left
'exif-orientation-5' => 'Arvirà dzorsota e ëd 90° contramostra', # 0th row: left; 0th column: top
'exif-orientation-6' => 'Arvirà ëd 90° ant ël sens dla mostra', # 0th row: right; 0th column: top
'exif-orientation-7' => 'Arvirà dzorsota e ëd 90° ant ël sens dla mostra', # 0th row: right; 0th column: bottom
'exif-orientation-8' => 'Arvirà ëd 90° contramostra', # 0th row: left; 0th column: bottom

'exif-planarconfiguration-1' => 'për blòch (chunky)',
'exif-planarconfiguration-2' => 'an planar',

'exif-xyresolution-i' => '$1 pont për pòles (dpi)',
'exif-xyresolution-c' => '$1 pont për centim (dpc)',

'exif-colorspace-ffff.h' => 'Nen calibrà',

'exif-componentsconfiguration-0' => 'a esist pa',

'exif-exposureprogram-0' => 'Nen definì',
'exif-exposureprogram-1' => 'Manual',
'exif-exposureprogram-2' => 'Programa normal',
'exif-exposureprogram-3' => 'Priorità ëd temp',
'exif-exposureprogram-4' => 'Priorità ëd diaframa',
'exif-exposureprogram-5' => "Programa creativ (coregiù për avej pì ëd profondità 'd camp)",
'exif-exposureprogram-6' => "Programa d'assion (coregiù për avej ël temp pì curt che as peul)",
'exif-exposureprogram-7' => 'Programa ritrat (për fotografìe pijaite da davsin, con lë sfond fòra feu)',
'exif-exposureprogram-8' => 'Panorama (sogèt lontan e con lë sfond a feu)',

'exif-subjectdistance-value' => '$1 méter',

'exif-meteringmode-0'   => 'as sa nen coma',
'exif-meteringmode-1'   => 'Media',
'exif-meteringmode-2'   => 'Media centrà',
'exif-meteringmode-3'   => 'Quadrèt (Spot)',
'exif-meteringmode-4'   => 'Vàire quadrèt (MultiSpot)',
'exif-meteringmode-5'   => 'Schema (Pattern)',
'exif-meteringmode-6'   => 'Parsial',
'exif-meteringmode-255' => "n'àutr",

'exif-lightsource-0'   => 'Nen marcà',
'exif-lightsource-1'   => 'Lus dël dì',
'exif-lightsource-2'   => 'Fluoressenta',
'exif-lightsource-3'   => 'Lus al tungsten (a incandessensa)',
'exif-lightsource-4'   => 'Flash',
'exif-lightsource-9'   => 'Temp bel',
'exif-lightsource-10'  => 'Temp an-nivolà',
'exif-lightsource-11'  => 'Ombra',
'exif-lightsource-12'  => 'Fluoressensa tipo lus dël dì (D 5700 – 7100K)',
'exif-lightsource-13'  => 'Fluoressensa bianca për ël dì (N 4600 – 5400K)',
'exif-lightsource-14'  => 'Fluoressensa bianca frèida (W 3900 – 4500K)',
'exif-lightsource-15'  => 'Fluoressensa bianca (WW 3200 – 3700K)',
'exif-lightsource-17'  => 'Lus stàndard sòrt A',
'exif-lightsource-18'  => 'Lus stàndard sòrt B',
'exif-lightsource-19'  => 'Lus stàndard sòrt C',
'exif-lightsource-20'  => 'Anluminant D55',
'exif-lightsource-21'  => 'Anluminant D65',
'exif-lightsource-22'  => 'Anluminant D75',
'exif-lightsource-23'  => 'Anluminant D50',
'exif-lightsource-24'  => 'Làmpada da studio ISO al tungsten',
'exif-lightsource-255' => "Aùtra sorgiss d'anluminassion",

'exif-focalplaneresolutionunit-2' => 'pòles anglèis (inches)',

'exif-sensingmethod-1' => 'Nen definì',
'exif-sensingmethod-2' => 'Sensor dlë spassi color a 1 processor',
'exif-sensingmethod-3' => 'Sensor dlë spassi color a 2 processor',
'exif-sensingmethod-4' => 'Sensor dlë spassi color a 3 processor',
'exif-sensingmethod-5' => 'Sensor sequensial dlë spassi color',
'exif-sensingmethod-7' => 'Sensor trilinear',
'exif-sensingmethod-8' => 'Sensor linear ëd color sequensiaj',

'exif-scenetype-1' => 'Fotografìa an diret',

'exif-customrendered-0' => 'Process normal',
'exif-customrendered-1' => 'Process particular',

'exif-exposuremode-0' => 'Esposission automàtica',
'exif-exposuremode-1' => 'Esposission manual',
'exif-exposuremode-2' => 'Esposission automàtica (auto bracket)',

'exif-whitebalance-0' => "Balansa dël bianch n'automàtich",
'exif-whitebalance-1' => 'Balansa dël bianch an manual',

'exif-scenecapturetype-0' => 'Stàndard',
'exif-scenecapturetype-1' => 'Paisagi',
'exif-scenecapturetype-2' => 'Ritrat',
'exif-scenecapturetype-3' => 'La neuit',

'exif-gaincontrol-0' => 'Gnun',
'exif-gaincontrol-1' => 'Sparé ij contrast bass',
'exif-gaincontrol-2' => 'Sparé ij contrast fòrt',
'exif-gaincontrol-3' => 'Bassé ij contrast bass',
'exif-gaincontrol-4' => 'Bassé ij contrast fòrt',

'exif-contrast-0' => 'Normal',
'exif-contrast-1' => 'dosman',
'exif-contrast-2' => 'contrastà fòrt',

'exif-saturation-0' => 'Normal',
'exif-saturation-1' => 'Saturassion bassa',
'exif-saturation-2' => 'Saturassion àuta',

'exif-sharpness-0' => 'Normal',
'exif-sharpness-1' => 'dossa',
'exif-sharpness-2' => 'contrastà',

'exif-subjectdistancerange-0' => 'Nen specificà',
'exif-subjectdistancerange-1' => 'Macro',
'exif-subjectdistancerange-2' => 'Prim pian',
'exif-subjectdistancerange-3' => 'Anquadratura a soget lontan',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'Latitùdin setentrional',
'exif-gpslatitude-s' => 'Latitùdin meridional',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'Longitùdin oriental',
'exif-gpslongitude-w' => 'Longitùdin ossidental',

'exif-gpsstatus-a' => 'Amzura antramentr che as fa',
'exif-gpsstatus-v' => "Interoperabilità dl'amzura",

'exif-gpsmeasuremode-2' => 'amzura bidimensional',
'exif-gpsmeasuremode-3' => 'amzura tridimensional',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'Km/h',
'exif-gpsspeed-m' => 'mija/h',
'exif-gpsspeed-n' => 'Grop (marin)',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => 'Diression vèira',
'exif-gpsdirection-m' => 'Diression magnética',

# External editor support
'edit-externally'      => "Modifiché st'archivi con un programa estern",
'edit-externally-help' => "Che a varda [http://www.mediawiki.org/wiki/Manual:External_editors setup instructions] për avej pì d'anformassion.",

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'tute',
'imagelistall'     => 'tùit/tute',
'watchlistall2'    => 'tute',
'namespacesall'    => 'tùit',
'monthsall'        => 'tuti',

# E-mail address confirmation
'confirmemail'            => "Confermé l'adrëssa postal",
'confirmemail_noemail'    => "A l'ha pa butà gnun-a adrëssa vàlida ëd pòsta eletrònica ant ij [[Special:Preferences|sò gust]].",
'confirmemail_text'       => "Costa wiki a ciama che chiel a convalida n'adrëssa postal anans che
dovré lòn che toca la pòsta. Che a sgnaca ël boton ambelessì sota
për fesse mandé un messa ëd conferma a soa adrëssa eletrònica.
Andrinta al messagi a-i sara n'anliura (URL) con andrinta un còdes.
Che a deurba st'anliura andrinta a sò programa ëd navigassion (browser)
për confermé che soa adrëssa a l'é pròpe cola.",
'confirmemail_pending'    => '<div class="error">
I l\'oma già mandaje sò còdes ëd conferma;
se a l\'ha pen-a creasse sò cont, miraco a venta che a speta dontre minute che a-j riva ant la pòsta, nopà che ciamene un neuv.</div>',
'confirmemail_send'       => 'Manda un còdes ëd conferma për pòsta eletrònica',
'confirmemail_sent'       => "Ël messagi ëd conferma a l'é stait mandà.",
'confirmemail_oncreate'   => "Un còdes ëd conferma a l'é stait mandà a soa adrëssa ëd pòsta eletrònica.
D'ës còdes a fa pa dë manca për rintré ant ël sistema, ma a ventrà che a lo mostra al sistema për podej abilité cole funsion dla wiki che a son basà ant sla pòsta eletrònica.",
'confirmemail_sendfailed' => "A l'é pa podusse mandé ël còdes ëd conferma. Che a controla l'adrëssa che a l'ha dane, mai che a-i fusso dij caràter nen vàlid.

Ël programa ëd pòsta a l'ha arspondù: $1",
'confirmemail_invalid'    => 'Còdes ëd conferma nen vàlid. A podrìa ëdcò mach esse scadù.',
'confirmemail_needlogin'  => 'A venta che a fasa $1 për confermé soa addrëssa postal eletrònica.',
'confirmemail_success'    => "Soa adrëssa postal a l'é staita confermà, adess a peul rintré ant ël sistema e i-j auguroma da fessla bin ant la wiki!",
'confirmemail_loggedin'   => "Motobin mersì. Soa adrëssa ëd pòsta eletrònica adess a l'é confermà.",
'confirmemail_error'      => "Cheich-còs a l'é andà mal ën salvand soa conferma.",
'confirmemail_subject'    => "Conferma dl'adrëssa postal da 'nt la {{SITENAME}}",
'confirmemail_body'       => "Cheidun, a l'é belfé che a sia stait pròpe chiel (ò chila)
da 'nt l'adrëssa IP \$1, a l'ha doertà un cont utent \"\$2\"
ansima a {{SITENAME}}, lassand-ne st'adrëssa ëd pòsta eletrònica-sì.

Për confermé che ës cont a l'é da bon sò e për ativé le possibilità
corelà a la pòsta eletrònica ansima a {{SITENAME}}, che a deurba
st'adrëssa-sì andrinta a sò programa ëd navigassion (browser)

\$3

Se a fussa *nen* stait chiel a deurbe ël cont, anlora che a fasa gnente.
Follow this link to cancel the e-mail address confirmation:

\$5

Cost còdes ëd conferma a l'é bon fin-a al \$4.",

# Scary transclusion
'scarytranscludedisabled' => "[L'inclusion ëd pàgine antra wiki diferente a l'é nen abilità]",
'scarytranscludefailed'   => "[Darmagi, ma lë stamp $1 a l'é pa podusse carié]",
'scarytranscludetoolong'  => '[Eror: anliura tròp longa]',

# Trackbacks
'trackbackbox'      => '<div id="mw_trackbacks">
Anformassion për feje ël traciament a sta vos-sì:<br />
$1
</div>',
'trackbackremove'   => ' ([$1 Gava via])',
'trackbacklink'     => 'Traciament',
'trackbackdeleteok' => "J'anformassion për fé traciament a son staite gavà via.",

# Delete conflict
'deletedwhileediting' => "Avertensa: sta pàgina-sì a l'é staita scancelà quand che chiel (chila) a l'avìa già anandiasse a modifichela!",
'confirmrecreate'     => "L'utent [[User:$1|$1]] ([[User talk:$1|talk]]) a l'ha scancelà st'articol-sì quand che chiel (chila) a l'avia già anandiasse a modifichelo, dand coma motiv ëd la scancelament:
''$2''
Për piasì, che an conferma che da bon a veul torna creélo.",
'recreate'            => "Créa n'àutra vira",

# HTML dump
'redirectingto' => 'I soma antramentr che i foma na ridiression a [[:$1]]...',

# action=purge
'confirm_purge'        => 'Veujdé la memorisassion dë sta pàgina-sì?

$1',
'confirm_purge_button' => 'Va bin',

# AJAX search
'searchcontaining' => "Sërca le vos che a l'han andrinta ''$1''.",
'searchnamed'      => "Sërca le vos che a l'han për tìtol ''$1''.",
'articletitles'    => "Artìcoj che as anandio për ''$1''",
'hideresults'      => "Stërma j'arsultà",
'useajaxsearch'    => "Dovré l'arsërca AJAX",

# Multipage image navigation
'imgmultipageprev' => '← pàgina andré',
'imgmultipagenext' => 'pàgina anans →',
'imgmultigo'       => 'Va',

# Table pager
'ascending_abbrev'         => 'a chërse',
'descending_abbrev'        => 'a calé',
'table_pager_next'         => 'Pàgina anans',
'table_pager_prev'         => 'Pàgina andré',
'table_pager_first'        => 'Prima pàgina',
'table_pager_last'         => 'Ùltima pàgina',
'table_pager_limit'        => 'Smon-me $1 archivi për pàgina',
'table_pager_limit_submit' => 'Va',
'table_pager_empty'        => 'Pa gnun arsultà',

# Auto-summaries
'autosumm-blank'   => "Pàgina dësveujdà d'autut",
'autosumm-replace' => "Pàgina cambià con '$1'",
'autoredircomment' => 'Ridiression anvers a [[$1]]',
'autosumm-new'     => 'Pàgina neuva: $1',

# Size units
'size-bytes'     => '$1 Byte',
'size-kilobytes' => '$1 KByte',
'size-megabytes' => '$1 MByte',
'size-gigabytes' => '$1 GByte',

# Live preview
'livepreview-loading' => "Antramentr ch'as caria…",
'livepreview-ready'   => "Antramentr ch'as caria… Carià.",
'livepreview-failed'  => 'La "preuva dal viv" a l\'é falìa!
Ch\'a preuva an manera sòlita.',
'livepreview-error'   => 'Conession falà: $1 "$2"
Ch\'a preuva an manera sòlita.',

# Friendlier slave lag warnings
'lag-warn-normal' => "Le modìfiche faite ant j'ùltim $1 second a podrìo ëdcò nen ess-ie ant sta lista-sì.",
'lag-warn-high'   => "Për via che la màchina serventa a tarda a dene d'arspòsta, le modìfiche pì giovne che $1 second fa
a podrìo ëdcò nen ess-ie ant sta lista -sì.",

# Watchlist editor
'watchlistedit-numitems'       => "A l'é antramentr ch'a ten sot ëuj {{PLURAL:$1|1 tìtol|$1 tìtoj}}, nen contand le pàgine ëd discussion.",
'watchlistedit-noitems'        => "A-i é pa gnun tìtol ch'as ten-a sot euj.",
'watchlistedit-normal-title'   => "Modifiché la lista ëd lòn ch'as ten sot euj",
'watchlistedit-normal-legend'  => "Gavé via ij tìtoj da 'nt la lista ëd lòn ch'as ten sot euj",
'watchlistedit-normal-explain' => "Ij tìtoj ch'a l'é dapress a ten-se sot euj a son ambelessì sota. Për gavene via un ch'a-i fasa la crosëtta ant la casela ch'a l'ha aranda, e peuj ch'ai bata ansima a \"Gavé via ij titoj\". As peul ëdcò [[Special:Watchlist/raw|modifiché la lista ampressa]].",
'watchlistedit-normal-submit'  => 'Gavé via ij tìtoj',
'watchlistedit-normal-done'    => "{{PLURAL:$1|1 tìtol a l'é|$1 tìtoj a son}} stait gavà via da 'nt la lista ëd lòn ch'as ten sot euj:",
'watchlistedit-raw-title'      => "Modifiché ampressa la lista ëd lòn ch'as ten sot euj",
'watchlistedit-raw-legend'     => "Modifiché ampressa la lista ëd lòn ch'as ten sot euj",
'watchlistedit-raw-explain'    => "Ij tìtoj ch'a l'é antramentr ch'as ten sot euj a son ambelessì sota, e a peulo modifichesse
	ën giontand-ne e gavand-ne via da 'nt la lista; un tìtol për riga. Quand a l'ha finì, ch'ai bata ansima a Agiorna la Lista.
	As peul ëdcò [[Special:Watchlist/edit|dovré l'editor sòlit]].",
'watchlistedit-raw-titles'     => 'Tìtoj:',
'watchlistedit-raw-submit'     => 'Agiorné la Lista',
'watchlistedit-raw-done'       => "La lista ëd lòn ch'as ten sot euj a l'é staita agiornà.",
'watchlistedit-raw-added'      => "A {{PLURAL:$1|l'é|son}} giontasse {{PLURAL:$1|1 tìtol|$1 tìtoj}}:",
'watchlistedit-raw-removed'    => "A {{PLURAL:$1|l'é|son}} gavasse via {{PLURAL:$1|1 tìtol|$1 tìtoj}}:",

# Watchlist editing tools
'watchlisttools-view' => 'S-ciairé le modifiché amportante',
'watchlisttools-edit' => "Vardé e modifiché la lista ëd lòn ch'as ten sot euj",
'watchlisttools-raw'  => "Modifiché ampressa la lista ëd lòn ch'as ten sot euj",

# Iranian month names
'iranian-calendar-m1'  => 'Prim mèis Jalāli',
'iranian-calendar-m2'  => 'Scond mèis Jalāli',
'iranian-calendar-m3'  => 'Tèrs mèis Jalāli',
'iranian-calendar-m4'  => 'Quart mèis Jalāli',
'iranian-calendar-m5'  => 'Quint mèis Jalāli',
'iranian-calendar-m6'  => "Mèis Jalāli ch'a fa ses",
'iranian-calendar-m7'  => "Mèis Jalāli ch'a fa set",
'iranian-calendar-m8'  => "Mèis Jalāli ch'a fa eut",
'iranian-calendar-m9'  => "Mèis Jalāli ch'a fa neuv",
'iranian-calendar-m10' => "Mèis Jalāli ch'a fa des",
'iranian-calendar-m11' => "Mèis Jalāli ch'a fa óndes",
'iranian-calendar-m12' => "Meis Jalāli ch'a fa dódes",

# Core parser functions
'unknown_extension_tag' => 'Tacolèt d\'estension "$1" pa conossù',

# Special:Version
'version'                          => 'Version', # Not used as normal message but as header for the special page itself
'version-extensions'               => 'Estension anstalà',
'version-specialpages'             => 'Pàgine speciaj',
'version-parserhooks'              => 'Gancio dlë scompositor',
'version-variables'                => 'Variàbij',
'version-other'                    => 'Aùtr',
'version-mediahandlers'            => 'Gestor multimojen',
'version-hooks'                    => 'Gancio',
'version-extension-functions'      => "Fonsion dj'estension",
'version-parser-extensiontags'     => "Tacolèt dj'estension conossùe da lë scompositor",
'version-parser-function-hooks'    => 'Gancio për le fonsion dlë scompositor',
'version-skin-extension-functions' => "Fonsion për j'estension dle facie",
'version-hook-name'                => 'Nòm dël gancio',
'version-hook-subscribedby'        => 'A son scrivusse',
'version-version'                  => 'Version',
'version-license'                  => 'Licensa',
'version-software'                 => 'Programa anstalà',
'version-software-product'         => 'Prodot',
'version-software-version'         => 'Version',

# Special:FilePath
'filepath'         => "Përcors d'archivi",
'filepath-page'    => 'Archivi:',
'filepath-submit'  => 'Përcors',
'filepath-summary' => "Sta pàgina special-sì a la smon ël përcors complet për rive-ie a n'archivi.
Le figure as ësmon-o a amzura pijn-a, j'àotre sòrt d'archivi a ven-o faite parte da sò programa a pòsta.

Ch'a buta mach ël nòm dl'archivi sensa pa ël prefiss \"{{ns:image}}:\".",

# Special:FileDuplicateSearch
'fileduplicatesearch'          => "Arsërca dj'archivi dobi",
'fileduplicatesearch-summary'  => "Arsërca dj'archivi dobi a parte dal valor d'ordinament.

Ch'a bata 'l nòm dl'archivi sensa pa bute-ie 'l prefiss \"{{ns:image}}:\".",
'fileduplicatesearch-legend'   => 'Arsërca ëd na dobia',
'fileduplicatesearch-filename' => "Nòm dl'archivi:",
'fileduplicatesearch-submit'   => 'Arsërca',
'fileduplicatesearch-info'     => '$1 × $2 pixel<br />Amzure: $3<br />Sòrt MIME: $4',
'fileduplicatesearch-result-1' => 'Pa gnun-a dobia për l\'archivi "$1".',
'fileduplicatesearch-result-n' => 'A-i {{PLURAL:$2|é \'n dobion midem|son $2 dobion midem}} ëd l\'archivi "$1".',

# Special:SpecialPages
'specialpages' => 'Pàgine Speciaj',

);
