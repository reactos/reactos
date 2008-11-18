<?php
/** Võro (Võro)
 *
 * @ingroup Language
 * @file
 *
 * @author Niklas Laxsröm
 * @author Sulev Iva (Võrok)
 * @author Võrok
 */

$fallback = 'et';

$namespaceNames = array(
	NS_MEDIA            => 'Meediä',
	NS_SPECIAL          => 'Tallituslehekülg',
	NS_MAIN             => '',
	NS_TALK             => 'Arotus',
	NS_USER             => 'Pruukja',
	NS_USER_TALK        => 'Pruukja_arotus',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK     => '$1_arotus',
	NS_IMAGE            => 'Pilt',
	NS_IMAGE_TALK       => 'Pildi_arotus',
	NS_MEDIAWIKI        => 'MediaWiki',
	NS_MEDIAWIKI_TALK   => 'MediaWiki_arotus',
	NS_TEMPLATE         => 'Näüdüs',
	NS_TEMPLATE_TALK    => 'Näüdüse_arotus',
	NS_HELP             => 'Oppus',
	NS_HELP_TALK        => 'Oppusõ_arotus',
	NS_CATEGORY         => 'Katõgooria',
	NS_CATEGORY_TALK    => 'Katõgooria_arotus'
);

$skinNames = array(
	'standard'    => array( "Harilik" ),
	'cologneblue' => array( "Kölni sinine" ),
	'myskin'      => array( "Mu uma kujondus" ),
);

$magicWords = array(
	'redirect'            => array( "0", "#redirect", "#saadaq" ),
);

$messages = array(
# User preference toggles
'tog-underline'               => 'Lingiq ala tõmmadaq',
'tog-highlightbroken'         => 'Parandaq vigadsõq lingiq <a href="" class="new">nii</a> (vai nii: <a href="" class="internal">?</a>)',
'tog-justify'                 => 'Lõiguveereq sirgõs',
'tog-hideminor'               => 'Käkiq perämäidsin muutmiisin ärq väikuq parandusõq',
'tog-extendwatchlist'         => 'Näütäq perräkaemisnimekirän kõiki muutuisi',
'tog-usenewrc'                => 'Laendõduq perämädseq muutmisõq (olõ-i kõigin võrgokaejin)',
'tog-numberheadings'          => 'Päälkirjo automaatnummõrdus',
'tog-showtoolbar'             => 'Näütäq toimõndusõ riistakasti',
'tog-editondblclick'          => 'Artiklidõ toimõndaminõ topõltklõpsu pääle (JavaScript)',
'tog-editsection'             => 'Lupaq lõikõ toimõndaq [toimõndaq]-linkõga',
'tog-editsectiononrightclick' => 'Lupaq lõikõ toimõndaq hüäpoolidsõ klõpsutusõga <br /> lõigu päälkirä pääl (JavaScript)',
'tog-showtoc'                 => 'Näütäq sisukõrda (rohkõmb ku kolmõ vaihõpäälkiräga lehile)',
'tog-rememberpassword'        => 'Salasõna miildejätmine tulõvaidsis kõrros',
'tog-editwidth'               => 'Täüslakjusõga toimõnduskast',
'tog-watchcreations'          => 'Panõq mu luuduq leheq mu perräkaemisnimekirjä',
'tog-watchdefault'            => 'Kaeq vahtsidõ ja muudõtuidõ artiklidõ perrä',
'tog-watchmoves'              => 'Panõq mu ümbrenõstõduq leheküleq mu perräkaemisnimekirjä',
'tog-watchdeletion'           => 'Panõq mu kistutõduq leheküleq mu perräkaemisnimekirjä',
'tog-minordefault'            => 'Märgiq kõik parandusõq vaikimiisi väikeisis paranduisis',
'tog-previewontop'            => 'Näütäq proovikaehust inne, mitte perän toimõnduskasti',
'tog-previewonfirst'          => 'Näütäq edimädse toimõndusõ aigo proovikaehust',
'tog-nocache'                 => 'Pästku-i lehekülgi vaihõmällo',
'tog-enotifwatchlistpages'    => 'Saadaq mullõ e-kiri, ku muq perräkaetavat lehte muudõtas',
'tog-enotifusertalkpages'     => 'Saadaq mullõ e-kiri, ku mu arotuslehte muudõtas',
'tog-enotifminoredits'        => 'Saadaq mullõ e-kiri ka väikeisi muutmiisi kotsilõ',
'tog-enotifrevealaddr'        => 'Näütäq mu e-postiaadrõssit tõisilõ saadõtuin teedüssin',
'tog-shownumberswatching'     => "Näütäq, ku pall'o pruukjit taa lehe perrä kaes",
'tog-fancysig'                => 'Pruugiq lihtsit allkirjo (ilma lingeldä pruukjalehe pääle)',
'tog-externaleditor'          => "Pruugiq vaikimiisi välist tekstitoimõndajat (õnnõ as'atundjilõ, nõud suq puutri ümbresäädmist)",
'tog-externaldiff'            => "Pruugiq vaikimiisi välist võrrõlusprogrammi (õnnõ as'atundjilõ, nõud su puutri ümbresäädmist)",
'tog-showjumplinks'           => 'Panõq lehe algustõ kipõqlingiq',
'tog-uselivepreview'          => 'Pruugiq kipõkaehust (JavaScript) (proomi)',
'tog-forceeditsummary'        => 'Annaq teedäq, ku olõ-i kirotõt kokkovõtõt',
'tog-watchlisthideown'        => 'Näüdäku-i perräkaemisnimekirän mu hindä toimõnduisi',
'tog-watchlisthidebots'       => 'Näüdäku-i perräkaemisnimekirän robotidõ toimõnduisi',
'tog-watchlisthideminor'      => 'Näüdäku-i perräkaemisnimekirän väikeisi muutmiisi',
'tog-nolangconversion'        => 'Jätäq ärq variantõ võrrõlus',
'tog-ccmeonemails'            => "Saadaq mullõ kopiq e-kir'ost, miä ma saada tõisilõ pruukjilõ",
'tog-diffonly'                => 'Näüdäku-i lahkominekide lehe all lehe täüt sissu',
'tog-showhiddencats'          => 'Näütäq käkitüid katõgoorijit',

'underline-always'  => 'Kõgõ',
'underline-never'   => 'Ei kunagi',
'underline-default' => 'Võrgokaeja perrä',

'skinpreview' => '(Kaeminõ)',

# Dates
'sunday'        => 'pühäpäiv',
'monday'        => 'iispäiv',
'tuesday'       => 'tõõsõpäiv',
'wednesday'     => 'kolmapäiv',
'thursday'      => 'nelapäiv',
'friday'        => 'riidi',
'saturday'      => 'puulpäiv',
'sun'           => 'Pü',
'mon'           => 'I',
'tue'           => 'T',
'wed'           => 'K',
'thu'           => 'N',
'fri'           => 'R',
'sat'           => 'Pu',
'january'       => 'vahtsõaastakuu',
'february'      => 'radokuu',
'march'         => 'urbõkuu',
'april'         => 'mahlakuu',
'may_long'      => 'lehekuu',
'june'          => 'piimäkuu',
'july'          => 'hainakuu',
'august'        => 'põimukuu',
'september'     => 'süküskuu',
'october'       => 'rehekuu',
'november'      => 'märtekuu',
'december'      => 'joulukuu',
'january-gen'   => 'vahtsõaastakuu',
'february-gen'  => 'radokuu',
'march-gen'     => 'urbõkuu',
'april-gen'     => 'mahlakuu',
'may-gen'       => 'lehekuu',
'june-gen'      => 'piimäkuu',
'july-gen'      => 'hainakuu',
'august-gen'    => 'põimukuu',
'september-gen' => 'süküskuu',
'october-gen'   => 'rehekuu',
'november-gen'  => 'märtekuu',
'december-gen'  => 'joulukuu',
'jan'           => 'vahts',
'feb'           => 'radok',
'mar'           => 'urbõk',
'apr'           => 'mahlak',
'may'           => 'lehek',
'jun'           => 'piimäk',
'jul'           => 'hainak',
'aug'           => 'põimuk',
'sep'           => 'süküsk',
'oct'           => 'rehek',
'nov'           => 'märtek',
'dec'           => 'jouluk',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|Katõgooria|Katõgooriaq}}',
'category_header'                => 'Katõgooria "$1" artikliq',
'subcategories'                  => 'Allkatõgooriaq',
'category-media-header'          => 'Kiräkotus katõgoorian "$1"',
'category-empty'                 => "''Seon katõgoorian olõ-i parhilla artikliid ega teedüstüid.''",
'hidden-categories'              => '{{PLURAL:$1|Käkit katõgooria|Käkidüq katõgooriaq}}',
'hidden-category-category'       => 'Käkidüq katõgooriaq', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|Seol katõgoorial om õnnõ järgmäne allkatõgooria.|Seol katõgoorial  {{PLURAL:$1|om järgmäne allkatõgooria|ommaq järgmädsed $1 allkatõgooriat}} (kokko $2).}}',
'category-subcat-count-limited'  => 'Seol katõgoorial {{PLURAL:$1|om järgmäne allkatõgooria|ommaq järgmädsed $1 allkatõgooriaq}}.',
'category-article-count'         => '{{PLURAL:$2|Seon katõgoorian om õnnõ järgmäne lehekülg.|Seon katõgoorian {{PLURAL:$1|om järgmäne lehekülg|ommaq järgmädseq $1 lehekülge}} (kokko $2).}}',
'category-article-count-limited' => 'Seon katõgoorian {{PLURAL:$1|om järgmäne lehekülg|ommaq järgmädseq $1 lehekülge}}.',
'category-file-count'            => '{{PLURAL:$2|Seon katõgoorian om õnnõ järgmäne teedüstü.|{{PLURAL:$1|Järgmäne teedüstü om |Järgmädseq $1 teedüstüt ommaq}} seon katõgoorian (kokko $2).}}',
'category-file-count-limited'    => '{{PLURAL:$1|Järgmäne teedüstü om|Järgmädseq $1 teedüstüt}} ommaq seon katõgoorian.',
'listingcontinuesabbrev'         => 'lätt edesi',

'mainpagetext'      => 'Wiki tarkvara paika säet.',
'mainpagedocfooter' => 'Vikitarkvara pruukmisõ kotsilõ loeq mano:
* [http://meta.wikimedia.org/wiki/MediaWiki_User%27s_Guide MediaWiki pruukmisoppus (inglüse keelen)].
* [http://www.mediawiki.org/wiki/Manual:Configuration_settings Säädmiisi oppus (inglüse keelen)]
* [http://www.mediawiki.org/wiki/Manual:FAQ MediaWiki kõgõ küsütümbäq küsümiseq (inglüse keelen)]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce E-postilist, minka andas teedäq MediaWiki vahtsist kujõst].',

'about'          => 'Pääteedüs',
'article'        => 'Sisu',
'newwindow'      => '(tulõ vallalõ vahtsõn aknõn)',
'cancel'         => 'Jätäq katski',
'qbfind'         => 'Otsiq',
'qbbrowse'       => 'Kaeq',
'qbedit'         => 'Toimõndaq',
'qbpageoptions'  => 'Leheküle säädmine',
'qbpageinfo'     => 'Leheküle teedüs',
'qbmyoptions'    => 'Mu säädmiseq',
'qbspecialpages' => 'Tallitusleheküleq',
'moredotdotdot'  => 'Viil...',
'mypage'         => 'Muq lehekülg',
'mytalk'         => 'Mu arotus',
'anontalk'       => 'Seo puutri võrgoaadrõsi arotus',
'navigation'     => 'Juhtminõ',
'and'            => 'ja',

# Metadata in edit box
'metadata_help' => 'Metateedüs:',

'errorpagetitle'    => 'Viga',
'returnto'          => 'Tagasi lehe manoq $1.',
'tagline'           => 'Läteq: {{SITENAME}}',
'help'              => 'Abi',
'search'            => 'Otsiq',
'searchbutton'      => 'Otsiq',
'go'                => 'Mineq',
'searcharticle'     => 'Mineq',
'history'           => 'Artikli aolugu',
'history_short'     => 'Aolugu',
'updatedmarker'     => 'toimõndõt päält mu perämäst kaemist',
'info_short'        => 'Teedüs',
'printableversion'  => 'Trükükujo',
'permalink'         => 'Püsülink',
'print'             => 'Trüküq vällä',
'edit'              => 'Toimõndaq',
'create'            => 'Luuq leht',
'editthispage'      => 'Toimõndaq seod artiklit',
'create-this-page'  => 'Luuq seo leht',
'delete'            => 'Kistudaq ärq',
'deletethispage'    => 'Kistudaq seo artikli ärq',
'undelete_short'    => 'Võtaq tagasi {{PLURAL:$1|üts muutminõ|$1 muutmist}}',
'protect'           => 'Kaidsaq',
'protect_change'    => 'kirotuskaidsõq',
'protectthispage'   => 'Kaidsaq seod artiklit',
'unprotect'         => 'Kaitsku-i',
'unprotectthispage' => 'Kaitsku-i seod artiklit',
'newpage'           => 'Vahtsõnõ artikli',
'talkpage'          => 'Seo artikli arotus',
'talkpagelinktext'  => 'Arotus',
'specialpage'       => 'Tallituslehekülg',
'personaltools'     => 'Erätüüriistaq',
'postcomment'       => 'Panõq kommõntaar',
'articlepage'       => 'Artiklilehekülg',
'talk'              => 'Arotus',
'views'             => 'Kaemisõq',
'toolbox'           => 'Tüüriistakast',
'userpage'          => 'Pruukjalehekülg',
'projectpage'       => 'Tallituslehekülg',
'imagepage'         => 'Näütäq pildilehekülge',
'mediawikipage'     => 'Näütäq sõnomilehekülge',
'templatepage'      => 'Näütäq näüdüselehekülge',
'viewhelppage'      => 'Näütäq abilehekülge',
'categorypage'      => 'Näütäq katõgoorialehekülge',
'viewtalkpage'      => 'Arotuslehekülg',
'otherlanguages'    => 'Tõisin keelin',
'redirectedfrom'    => '(Ümbre saadõt artiklist $1)',
'redirectpagesub'   => 'Ümbresaatmislehekülg',
'lastmodifiedat'    => 'Seo leht om viimäte muudõt $2, $1.', # $1 date, $2 time
'viewcount'         => 'Seo lehe pääl om käüt $1 {{PLURAL:$1|kõrd|kõrda}}.',
'protectedpage'     => 'Kaidsõt artikli',
'jumpto'            => 'Mineq üle:',
'jumptonavigation'  => 'juhtminõ',
'jumptosearch'      => 'otsminõ',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => '{{SITENAME}} tutvustus',
'aboutpage'            => 'Project:Pääteedüs',
'bugreports'           => 'Viateedüseq',
'bugreportspage'       => 'Project:Viateedüseq',
'copyright'            => "Teksti või vabalt pruukiq $1'i perrä.",
'copyrightpagename'    => '{{SITENAME}} ja tegijäõigusõq',
'copyrightpage'        => '{{ns:project}}:Tegijäõigusõq',
'currentevents'        => 'Miä sünnüs',
'currentevents-url'    => 'Project:Miä sünnüs',
'disclaimers'          => 'Hoiatuisi',
'disclaimerpage'       => 'Project:Üledseq hoiatusõq',
'edithelp'             => 'Toimõndamisoppus',
'edithelppage'         => 'Help:Kuis_artiklit_toimõndaq',
'faq'                  => 'Sagõhõhe küsüdüq küsümiseq',
'faqpage'              => 'Project:KKK',
'helppage'             => 'Help:Oppus',
'mainpage'             => 'Pääleht',
'mainpage-description' => 'Pääleht',
'policy-url'           => 'Project:Säädüseq',
'portal'               => 'Arotusõtarõ',
'portal-url'           => 'Project:Arotusõtarõ',
'privacy'              => 'Eräteedüse kaitsminõ',
'privacypage'          => 'Project:Eräteedüse kaitsminõ',

'badaccess'        => 'Lubamalda tallitus',
'badaccess-group0' => 'Sul olõ-i õigust seod tallitust tetäq.',
'badaccess-group1' => 'Seod tallitust võivaq tetäq õnnõ rühmä $1 pruukjaq.',
'badaccess-group2' => 'Seod tallitust saavaq tetäq õnnõ rühmi $1 liikmõq.',
'badaccess-groups' => 'Seod tallitust saavaq tetäq õnnõ rühmä $1 liikmõq.',

'versionrequired'     => 'Om vaia MediaWiki kujjo $1',
'versionrequiredtext' => 'Seo lehe kaemisõs om vaia MediaWiki kujjo $1. Kaeq [[Special:Version|kujoteedüst]].',

'ok'                      => 'Hää külh',
'retrievedfrom'           => 'Vällä otsit teedüskogost "$1"',
'youhavenewmessages'      => 'Sul om $1 ($2).',
'newmessageslink'         => 'vahtsit sõnomiid',
'newmessagesdifflink'     => 'perämäne muutminõ',
'youhavenewmessagesmulti' => 'Sullõ om vahtsit sõnomit lehe pääl $1',
'editsection'             => 'toimõndaq',
'editold'                 => 'toimõndaq',
'viewsourceold'           => 'näütäq lättekuudi',
'editsectionhint'         => 'Toimõndaq lõiku: $1',
'toc'                     => 'Sisukõrd',
'showtoc'                 => 'näütäq',
'hidetoc'                 => 'käkiq',
'thisisdeleted'           => 'Kaeq vai tiiq tagasi $1?',
'viewdeleted'             => 'Näüdädäq $1?',
'restorelink'             => 'Kistutõduid muutmiisi: $1',
'feedlinks'               => 'Sisseandminõ:',
'feed-invalid'            => 'Viganõ sisseandminõ.',
'site-rss-feed'           => '$1-RSS-söödüs',
'site-atom-feed'          => '$1-Atom-söödüs',
'page-rss-feed'           => '$1 (RSS-söödüs)',
'page-atom-feed'          => '$1 (Atom-söödüs)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Artikli',
'nstab-user'      => 'Pruukjalehekülg',
'nstab-media'     => 'Meediä',
'nstab-special'   => 'Tallituslehekülg',
'nstab-project'   => 'Nimileht',
'nstab-image'     => 'Pilt',
'nstab-mediawiki' => 'Teedüs',
'nstab-template'  => 'Näüdüs',
'nstab-help'      => 'Oppus',
'nstab-category'  => 'Katõgooria',

# Main script and global functions
'nosuchaction'      => 'Säänest tallitust olõ-i.',
'nosuchactiontext'  => 'Viki tunnõ-i taa aadrõsi manoq käüvät tallitust.',
'nosuchspecialpage' => 'Säänest tallituslehekülge olõ-i.',
'nospecialpagetext' => 'Viki tunnõ-i säänest tallituslehekülge.',

# General errors
'error'                => 'Viga',
'databaseerror'        => 'Teedüskogo viga',
'dberrortext'          => 'Teedüskogo perräküsümisen oll\' süntaksiviga.
Perräküsümine oll\' viganõ vai om tarkvaran viga.
Viimäne teedüskogo perräküsümine oll\':
<blockquote><tt>$1</tt></blockquote>
ja tuu tetti funktsioonist "<tt>$2</tt>".
MySQL and\' via "<tt>$3: $4</tt>".',
'dberrortextcl'        => 'Teedüskogo perräküsümisen oll\' süntaksiviga.
Viimäne teedüskogo perräküsümine oll\':
"$1"
ja tuu tetti funktsioonist "$2".
MySQL and\' via "$3: $4".',
'noconnect'            => 'Wiki saa ei teedüskogoserverit $1 kätte',
'nodb'                 => 'Saa es teedüskoko $1 kätte',
'cachederror'          => 'Taa lehekülg om puhvõrdõt kopi ja ei pruugiq tuuperäst ollaq kõgõ värskimb.',
'laggedslavemode'      => 'Hoiatus: Taa lehe pääl pruugi-i ollaq perämäidsi muutmiisi.',
'readonly'             => 'Teedüskogo kirotuskaitsõ all',
'enterlockreason'      => 'Kirodaq lukkupandmisõ põhjus ja ligikaudnõ vallalõvõtmisõ aig',
'readonlytext'         => "Teedüskogo om kirotuskaitsõ all, arvadaq niikavvas ku tedä parandõdas.
Kõrraldaja, kiä taa kirotuskaitsõ alaq võtt', and' sääntse selgütüse:
<p>$1",
'readonly_lag'         => 'Teedüskogo panti automaatsõhe kinniq, et kõik teedüskogoserveriq saasiq kätte kõik värskiq muutmisõq',
'internalerror'        => 'Sisemäne viga',
'internalerror_info'   => 'Viga: $1',
'filecopyerror'        => 'Es saaq teedüstüt "$1" teedüstüs "$2" kopidaq.',
'filerenameerror'      => 'Es saaq teedüstüt "$1" teedüstüs "$2" ümbre nimetäq.',
'filedeleteerror'      => 'Teedüstüt nimega "$1" saa-i ärq kistutaq.',
'directorycreateerror' => 'Saa-s luvvaq kausta "$1".',
'filenotfound'         => 'Lövvä es teedüstüt "$1".',
'fileexistserror'      => 'Saa-i kirotaq teedüstühe "$1": teedüstü om olõman',
'unexpected'           => 'Uutmaldaq väärtüs: "$1"="$2".',
'formerror'            => 'Viga: vormi saa es pästäq',
'badarticleerror'      => 'Taad tallitust saa ei seo leheküle pääl tetäq.',
'cannotdelete'         => "Seod lehekülge vai pilti saa ei ärq kistutaq. (Või-ollaq kiäki tõõnõ jo kistut' taa ärq.)",
'badtitle'             => 'Viganõ päälkiri',
'badtitletext'         => "Küsüt artiklipäälkiri oll' kas viganõ, tühi vai sis
võlssi näüdät kiili- vai wikidevaihõlinõ päälkiri.",
'perfdisabled'         => "Annaq andis! Seo tallitus parhillaq ei tüütäq, selle et tä tege teedüskogo pruukmisõ pall'o aigladsõs. Programmi tävvendedäs ligembädsel aol. Või-ollaq tiit tuud esiq!",
'perfcached'           => 'Järgmäne teedüs om puhvõrdõt ja pruugi ei ollaq kõgõ värskimb:',
'perfcachedts'         => 'Järgmäne teedüs om puhvõrdõt ja om viimäte muudõt $1.',
'querypage-no-updates' => 'Taad lehe teedüst parhilla värskis ei tetäq.',
'wrong_wfQuery_params' => 'Võlss suurusõq tallitusõlõ wfQuery()<br />
Tallitus: $1<br />
Perräküsümine: $2',
'viewsource'           => 'Kaeq lätteteksti',
'viewsourcefor'        => 'lehele $1',
'actionthrottled'      => 'Tallitusõ kibõhus piirõt',
'actionthrottledtext'  => "Taa tallitusõ mitmit kõrdo tegemine om prahipandjidõ peräst ärq keelet. Olõt taad lühkü ao seen pall'o hulga tennüq. Prooviq veidükese ao peräst vahtsõst.",
'protectedpagetext'    => 'Taa lehekülg om kirotuskaidsõt.',
'viewsourcetext'       => 'Võit kaiaq ja kopidaq taa lehe lättekoodi:',
'protectedinterface'   => "Taa lehe pääl om tarkvara pruukjapalgõ tekst. Leht om lukku pant, et taad saasi-i ärq ts'urkiq.",
'editinginterface'     => "'''Hoiatus:''' Sa toimõndat tarkvara pruukjapalgõ tekstiga lehte. Ku siin midä muudat, mõotas tuu pruukjapalõt. Ümbrepandmisõs tasos pruukiq MediaWiki ümbrepandmisõ tüüriista [http://translatewiki.net/wiki/Main_Page?setlang=fiu-vro Betawiki].",
'sqlhidden'            => '(SQL-perräküsümine käkit)',
'cascadeprotected'     => 'Taa leht om kirotuskaidsõt, selle et taa kuulus alanolõvidõ kaidsõtuidõ lehti hulka:',
'namespaceprotected'   => "Sul olõ-i lubat toimõndaq nimeruumi '''$1''' lehti.",
'customcssjsprotected' => 'Sul olõ-i lubat toimõndaq taad lehte, selle et tan om seen tõõsõ pruukja säädmiisi.',
'ns-specialprotected'  => 'Tallituslehekülgi ei saaq toimõndaq.',
'titleprotected'       => "Pruukja [[User:$1|$1]] om sääntse nimega lehe luumisõ ärq kiildnüq põhjusõga: ''$2''.",

# Login and logout pages
'logouttitle'                => 'Nime alt välläminek',
'logouttext'                 => '<strong>Olõt nime alt vällä lännüq.</strong>

Võit {{SITENAME}}t ilma nimeldä edesi toimõndaq vai [[Special:UserLogin|vahtsõst sama vai tõõsõ nimega sisse minnäq]]. 
Tähelepandmisõs: niikavva, ku sa olõ-i tühäs tennüq uma võrgokaeja vaihõmällo, võivaq mõnõq leheküleq iks viil näüdädäq, nigu sa olõsi nimega seen.',
'welcomecreation'            => '<h2>Tereq, $1!</h2><p>Su konto om valmis. Võit taa hindä perrä sisse säädäq.',
'loginpagetitle'             => 'Nimega sisseminek',
'yourname'                   => 'Pruukjanimi',
'yourpassword'               => 'Salasõna',
'yourpasswordagain'          => 'Kirodaq viilkõrd salasõna',
'remembermypassword'         => 'Salasõna miildejätmine järgmäidsis kõrros',
'yourdomainname'             => 'Võrgonimi',
'externaldberror'            => 'Välitsen kimmästegemisteedüskogon om viga vai olõ-i sul lubat umma pruukjanimme muutaq.',
'loginproblem'               => '<b>Es saaq sisse.</b><br />Prooviq vahtsõst!',
'login'                      => 'Nimega sisseminek',
'nav-login-createaccount'    => 'Mineq nimega sisse',
'loginprompt'                => '{{SITENAME}} lask nimega sisse õnnõ sis, ku lubatas valmistuisi.',
'userlogin'                  => 'Mineq nimega sisse',
'logout'                     => 'Nime alt välläminek',
'userlogout'                 => 'Mineq nime alt vällä',
'notloggedin'                => 'Olõ-i nimega sisse mint',
'nologin'                    => 'Olõ-i inne nimega sisse lännüq? $1.',
'nologinlink'                => 'Tiiq hindäle pruukjanimi',
'createaccount'              => 'Tiiq pruukjanimi ärq',
'gotaccount'                 => 'Ku sul jo om uma pruukjanimi, sis $1.',
'gotaccountlink'             => 'võit nimega sisse minnäq',
'createaccountmail'          => 'e-postiga',
'badretype'                  => 'Kirotõduq salasõnaq ei klapiq kokko.',
'userexists'                 => 'Kirotõt pruukjanimme jo pruugitas. Võtaq tõõnõ nimi.',
'youremail'                  => 'Suq e-posti aadrõs *',
'username'                   => 'Pruukjanimi:',
'uid'                        => 'Pruukjanummõr:',
'yourrealname'               => 'Peris nimi *',
'yourlanguage'               => 'Pruukjapalgõ kiil:',
'yourvariant'                => 'Keelevariant:',
'yournick'                   => 'Suq kutsmisnimi (alakirotamisõs)',
'badsig'                     => 'Seo alakirotus olõ-i masva.',
'badsiglength'               => "Alakirotus om pall'o pikk – tohe-i ollaq rohkõmb ku $1 märki.",
'email'                      => 'e-posti aadrõs',
'prefs-help-realname'        => "* <strong>Peris nimi</strong> (piä-i kirotama): ku taa teedäq annat, sis pruugitas taad pruukjanime asõmõl lehekülgi tegijide nimekir'on.",
'loginerror'                 => 'Sisseminemise viga',
'prefs-help-email'           => '* <strong>E-post</strong> (piä-i kirotama): tõõsõq pruukjaq saavaq sullõ kirotaq ilma su aadrõssit nägemäldäq. Taast om sis kah kassu, ku uma salasõna ärq johtut unõhtama.',
'prefs-help-email-required'  => 'E-postiaadrõs piät olõma.',
'nocookiesnew'               => 'Pruukjakonto om valmis, a sa päse-s sisse, selle et {{SITENAME}} tarvitas pruukjidõ kimmästegemises valmistuisi. Suq võrgokaejan ommaq valmistusõq ärq keeledüq. Säeq valmistusõq lubatus ja mineq sis uma vahtsõ pruukjanime ja salasõnaga sisse.',
'nocookieslogin'             => '{{SITENAME}} tarvitas pruukjidõ kimmästegemises valmistuisi. Suq võrgokaejan ommaq valmistusõq keeledüq. Säeq valmistusõq lubatus ja prooviq vahtsõst.',
'noname'                     => 'Võlssi kirotõt pruukjanimi.',
'loginsuccesstitle'          => "Sisseminek läts' kõrda",
'loginsuccess'               => 'Olõt nimega sisse lännüq. Suq pruukjanimi om "$1".',
'nosuchuser'                 => ' "$1" nimelist pruukjat olõ-i olõman. Kaeq kiräpilt üle vai pruugiq alanolõvat vormi vahtsõ konto luumisõs.',
'nosuchusershort'            => '"<nowiki>$1</nowiki>" nimelist pruukjat olõ-i olõman. Kas kirotit iks nime õigõhe?',
'nouserspecified'            => 'Olõ-i kirotõt pruukjanimme.',
'wrongpassword'              => 'Kirotõt võlss salasõna. Prooviq vahtsõst.',
'wrongpasswordempty'         => 'Salasõna tohe-i tühi ollaq.',
'passwordtooshort'           => "Salasõna om viganõ vai pall'o lühkü. Taan piät olõma vähämbält {{PLURAL:$1|1 märk|$1 märki}} ja taa tohe-i ollaq sama, miä su pruukjanimi.",
'mailmypassword'             => 'Saadaq mullõ vahtsõnõ salasõna',
'passwordremindertitle'      => '{{SITENAME}} salasõna miildetulõtus',
'passwordremindertext'       => "Kiäki (arvadaq saq esiq, puutri võrgonummõr $1),
pallõl', et {{SITENAME}} ($4) saatnuq sullõ vahtsõ sisseminegi salasõna.
Pruukja $2 salasõna om noq $3. Ku olõt nimega sisse lännüq, võit taa aotlidsõ salasõna ärq muutaq.

Ku taa pallõmisõ om tennüq kiä tõõnõ vai ku olõt uma salasõna miilde tulõtanuq ja taha-i taad inämb muutaq, sis teku-i seost sõnomist vällä ja pruugiq umma vanna salasõnna edesi.",
'noemail'                    => 'Kah\'os olõ-i meil pruukja "$1" e-postiaadrõssit.',
'passwordsent'               => 'Vahtsõnõ salasõna om saadõt pruukja "$1" kirotõdu e-postiaadrõsi pääle. Ku olõt salasõna kätte saanuq, mineq nimega sisse.',
'blocked-mailpassword'       => 'Su võrgonumbrilõ om pant pääle toimõndamiskiild, miä lasõ-i salasõnna miilde tulõtaq.',
'eauthentsent'               => 'Sullõ om saadõt kinnütüskiri. Muid kirjo saadõta-i inne, ku olõt tennüq nii, kuis kirän opat ja kinnütänüq, et taa om suq e-postiaadrõs.',
'throttled-mailpassword'     => '$1 tunni seen om saadõt salasõna miildetulõtus. Sääntsit miildetulõtuisi saadõtas õnnõ $1 tunni takast.',
'mailerror'                  => 'Kirä saatmisõ viga: $1',
'acct_creation_throttle_hit' => 'Sa olõt tennüq jo $1 kontot. Rohkõmb ei saaq.',
'emailauthenticated'         => 'Su e-postiaadrõs kinnütedi ärq $1.',
'emailnotauthenticated'      => "Su e-postiaadrõssit olõ-i viil kinnütet. Alanolõvi as'on e-kirjo ei saadõtaq.",
'noemailprefs'               => 'Olõ-i ant e-postiaadrõssit.',
'emailconfirmlink'           => 'Kinnüdäq uma e-postiaadrõs.',
'invalidemailaddress'        => 'Olõ-i kõrralik e-postiaadrõs. Kirodaq õigõ e-postiaadrõs vai jätäq rivi rühäs.',
'accountcreated'             => 'Pruukjanimi luudi',
'accountcreatedtext'         => 'Luudi pruukjanimi pruukjalõ $1.',
'createaccount-title'        => 'Vahtsõ {{SITENAME}} pruukjanime luuminõ',
'createaccount-text'         => 'Kiäki om loonuq pruukjanime $2 lehistüle {{SITENAME}} ($4). Pruukjanime "$2" salasõna om "$3".
Mineq nimega sisse ja vaihtaq salasõna ärq.

Ku taa pruukjanimi om luud kogõmaldaq, olõ-i sul vaia taast sõnomist vällä tetäq.',
'loginlanguagelabel'         => 'Kiil: $1',

# Password reset dialog
'resetpass'               => 'Salasõna vahtsõndus',
'resetpass_announce'      => 'Sa lätsit sisse e-postiga saadõdu aotlidsõ koodiga. Kõrdapiten sisseminekis tulõ sul siin tetäq hindäle  vahtsõnõ salasõna:',
'resetpass_text'          => '<!-- Kirodaq siiäq -->',
'resetpass_header'        => 'Salasõna vahtsõndus',
'resetpass_submit'        => 'Kirodaq salasõna ja mineq nimega sisse',
'resetpass_success'       => 'Salasõna vaihtaminõ läts kõrda.',
'resetpass_bad_temporary' => 'Taa aotlinõ salasõna kõlba-i. Sa olõt jo saanuq vahtsõ salasõna vai küsünüq vahtsõ aotlidsõ salasõna.',
'resetpass_forbidden'     => '{{SITENAME}} salasõnno saa-i vaihtaq.',
'resetpass_missing'       => 'Olõ-i teksti ant.',

# Edit page toolbar
'bold_sample'     => 'Paks kiri',
'bold_tip'        => 'Paks kiri',
'italic_sample'   => 'Liuhkakiri',
'italic_tip'      => 'Liuhkakiri',
'link_sample'     => 'Lingitäv päälkiri',
'link_tip'        => 'Siselink',
'extlink_sample'  => 'http://www.example.com Lingi nimi',
'extlink_tip'     => 'Välislink (unõhtagu-i ette pandaq http://)',
'headline_sample' => 'Päälkiri',
'headline_tip'    => 'Tõõsõ tasõmõ päälkiri',
'math_sample'     => 'Kirodaq vallõm siiäq',
'math_tip'        => 'Matõmaatigatekst (LaTeX)',
'nowiki_sample'   => 'Kirodaq kujondamalda tekst',
'nowiki_tip'      => 'Tunnistagu-i viki kujondust',
'image_sample'    => 'Näüdüs.jpg',
'image_tip'       => 'Pästet pilt',
'media_sample'    => 'Näüdüs.mp3',
'media_tip'       => 'Meediäteedüstü',
'sig_tip'         => 'Suq allkiri üten aotempliga',
'hr_tip'          => 'Horisontaaljuun',

# Edit pages
'summary'                   => 'Kokkovõtõq',
'subject'                   => 'Päälkiri',
'minoredit'                 => 'Taa om väiku parandus',
'watchthis'                 => 'Kaeq taa lehe perrä',
'savearticle'               => 'Pästäq',
'preview'                   => 'Proovikaehus',
'showpreview'               => 'Näütäq proovikaehust',
'showlivepreview'           => 'Kipõkaehus',
'showdiff'                  => 'Näütäq muutmiisi',
'anoneditwarning'           => "'''Hoiatus:''' sa olõ-i nimega sisse lännüq, seo lehe aolukku pandas su puutri aadrõs.",
'missingsummary'            => "'''Miildetulõtus:'''sa olõ-i kirotanuq uma toimõndamisõ kokkovõtõt. Ku klõpsahtat viil kõrra nuppi Pästäq, sis pästetäs su toimõndus ilma kokkovõttõldaq.",
'missingcommenttext'        => 'Olõq hää, kirodaq kokkovõtõq.',
'missingcommentheader'      => 'Sa olõ-i andnuq umalõ kokkovõttõlõ päälkirjä. Ku klõpsahtat nuppi <em>Pästäq</em>, pästetäs toimõndus ilma päälkiräldä.',
'summary-preview'           => 'Kokkovõttõ kaeminõ',
'subject-preview'           => 'Päälkirä kaeminõ',
'blockedtitle'              => 'Pruukja om kinniq peet',
'blockedtext'               => "<big>'''Su pruukjanimi vai puutri võrgoaadrõs om kinniq pant.'''</big>

Kinniqpandja om $1. 
Timä põhjõndus om sääne: ''$2''.

* Kinniqpandmisõ algus: $8
* Kinniqpandmisõ lõpp: $6
* Kinnipandja: $7

Küsümüst saat arotaq $1 vai mõnõ tõõsõ [[{{MediaWiki:Grouppage-sysop}}|kõrraldajaga]].
Panõq tähele, et sa saa-i taalõ pruukjalõ sõnomit saataq, ku sa olõ-i kirjä pandnuq umma [[Special:Preferences|säädmislehe]] e-posti aadrõssit.
Suq puutri võrgoaadrõs om $3 ja kinnipandmistunnus om #$5. Panõq naaq kõiki perräküsümiisi manoq, midä tiit.",
'autoblockedtext'           => "Su puutri võrgoaadrõs peeti automaatsõhe kinniq, selle et taad om tarvitanuq kiäki pruukja, kink om kinniq pidänüq $1.
Kinniqpidämise põhjus:

:''$2''

Kinniqpidämise aig: $6

Taa kinniqpidämise kotsilõ perräküsümises ja taa arotamisõs võit kirotaq kõrraldajalõ $1 vai mõnõlõ
[[{{MediaWiki:Grouppage-sysop}}|tõõsõlõ kõrraldajalõ]].

Rehkendäq tuud, et sa saa-i tõisilõ pruukjilõ e-kirjo saataq, ku sa olõ-i ummi [[Special:Preferences|säädmiisihe]] kirjä pandnuq suq hindä masvat e-postiaadrõssit.

Suq kinniqpidämise tunnusnummõr om $5. Olõq hää, kirodaq taa nummõr egä perräküsümise mano, miä sa tiit.",
'blockednoreason'           => 'põhjust olõ-i näüdät',
'blockedoriginalsource'     => "Lehe '''$1''' lättekuud:",
'blockededitsource'         => "Su tett toimõndus lehe '''$1''' pääl:",
'whitelistedittitle'        => 'Toimõndamisõs piät nimega sisse minemä',
'whitelistedittext'         => 'Lehekülgi toimõndamisõs $1.',
'confirmedittitle'          => 'E-posti kinnütüs',
'confirmedittext'           => 'Sa saa-i inne lehekülgi toimõndaq, ku olõt kinnütänüq ärq uma e-postiaadrõsi. Tuud saat tetäq uma [[Special:Preferences|säädmislehe]] pääl.',
'nosuchsectiontitle'        => 'Olõ-i säänest lõiku',
'nosuchsectiontext'         => 'Sa proovõq toimõndaq lõiku, midä olõ-i olõman, a ku lõiku $1 olõ-i olõman, sis olõ-i su toimõndust kohe pandaq.',
'loginreqtitle'             => 'Piät nimega sisse minemä',
'loginreqlink'              => 'nimega sisse minemä',
'loginreqpagetext'          => 'Tõisi lehekülgi kaemisõs piät $1.',
'accmailtitle'              => 'Salasõna saadõt.',
'accmailtext'               => "Pruukja '$1' salasyna saadõti aadrõsi pääle $2.",
'newarticle'                => '(Vahtsõnõ)',
'newarticletext'            => "Taad lehekülge olõ-i viil.
Leheküle luumisõs nakkaq kirotama alanolõvahe kasti.
Ku sa johtuq siiäq kogõmaldaq, sis klõpsaq võrgokaeja '''Tagasi'''-nuppi.",
'anontalkpagetext'          => "---- ''Taa om arotusleht nimeldä pruukja kotsilõ, kiä olõ-i loonuq kontot vai pruugi-i tuud. Tuuperäst tulõ meil pruukja kimmästegemises pruukiq timä puutri võrgoaadrõssit. Taa aadrõs või ollaq mitmõ pruukja pääle ütine. Ku olõt nimeldä pruukja ja lövvät, et taa leheküle pääle kirotõt jutt käü suq kotsilõ, sis olõq hää, [[Special:UserLogin|luuq konto vai mineq nimega sisse]], et edespiten segähüisi ärq hoitaq.''",
'noarticletext'             => 'Seo leht om parlaq tühi. Võit [[Special:Search/{{PAGENAME}}|otsiq seo lehe nimme]] tõisi lehti päält vai [{{fullurl:{{FULLPAGENAME}}|action=edit}} naataq seod lehte esiq kirotama].',
'userpage-userdoesnotexist' => 'Pruukjanimme "$1" olõ-i kirjä pant. Kaeq perrä, kas olõt iks kimmäs, et tahat taad lehte toimõndaq.',
'clearyourcache'            => "'''Panõq tähele:''' perän pästmist piät muutmiisi nägemises uma võrgokaeja vaihõmälo tühäs tegemä: '''Mozilla:''' vaodaq ''reload''  vai ''ctrl-r'', '''IE / Opera:''' ''ctrl-f5'', '''Safari:''' ''cmd-r'', '''Konqueror''' ''ctrl-r''.",
'usercssjsyoucanpreview'    => "<strong>Nõvvoannõq:</strong> Pruugiq nuppi 'Näütäq proovikaehust' uma vahtsõ CCS-i vai JavaScripti ülekaemisõs, inne ku taa ärq pästät.",
'usercsspreview'            => "'''Unõhtagu-i, et seod kujjo su umast stiililehest olõ-i viil pästet!'''",
'userjspreview'             => "'''Unõhtagu-i, et seo kujo su umast javascriptist om viil pästmäldäq!'''",
'userinvalidcssjstitle'     => "'''Miildetulõtus:''' Olõ-i stiili nimega \"\$1\". Piäq meelen, et pruukja säedüq .css- and .js-leheq piät nakkama väiku algustähega.",
'updated'                   => '(Värskis tett)',
'note'                      => '<strong>Miildetulõtus:</strong>',
'previewnote'               => '<strong>Taa om õnnõ proovikaehus; muutmisõq olõ-i pästedüq!</strong>',
'previewconflict'           => "Taa proovikaehus näütäs, kuis ülembädsen toimõtuskastin ollõv tekst' päält pästmist vällä nägemä nakkas.",
'session_fail_preview'      => '<strong>Annaq andis! Su toimõndust saa-s pästäq, selle et su tüükõrra teedüs om kaoma lännüq. Olõq hää, proomiq viilkõrd. Ku tuust olõ-i kassu, proomiq nii, et läät nime alt vällä ja sis jälq tagasi sisse.</strong>',
'session_fail_preview_html' => "<strong>Annaq andis, mi saa-i tallitaq su toimõndust, selle et toimõnduskõrra teedüs om kaoma lännüq.</strong>

''Kuna taan vikin om käügin lihtsä HTML, sis om näütämist piiret JavaScript-i ründämiisi kaitsõs.''

<strong>Ku taa om õigõ toimõnduskatsõq, prooviq viilkõrd. Ku iks tüütä-i, prooviq nime alt vällä minekit ja vahtsõst sissetulõkit.</strong>",
'editing'                   => 'Toimõndõdas artiklit $1',
'editingsection'            => 'Toimõndõdas lõiku artiklist $1',
'editingcomment'            => 'Toimõndõdas kommõntaari lehe $1 pääl',
'editconflict'              => 'Toimõndamisvastaolo: $1',
'explainconflict'           => "Kiäki om muutnuq seod lehte perän tuud, ku saq taad toimõndama naksiq.
Ülemädsen toimõnduskastin om teksti perämäne kujo.
Suq muutmisõq ommaq alomadsõn kastin.
Sul tulõ naaq viimätsehe kujjo üle viiäq.
Ku klõpsahtat nuppi \"Pästäq\", sis pästetäs '''õnnõ''' ülembädse toimõnduskasti tekst.",
'yourtext'                  => 'Suq tekst',
'storedversion'             => 'Pästet kujo',
'nonunicodebrowser'         => "<strong>Hoiatus: su võrgokaeja tukõ-i Unicode'i. Olõq hüä, võtaq toimõndamisõs leht vallalõ tõõsõn võrgokaejan.</strong>",
'editingold'                => '<strong>KAEQ ETTE! Toimõndat parhilla taa lehe vanna kujjo. Ku taa ärq pästät, sis lätväq kõik päält taad kujjo tettüq muutmisõq kaoma.</strong>',
'yourdiff'                  => 'Lahkominegiq',
'copyrightwarning'          => 'Pruukjapalgõ ümbrepandmisõq loetasõq avaldõdus $2 perrä
(täpsämbähe kaeq $1). Muud sissu või pruukiq tävveste vabalt, ku olõ-i tõisildõ näüdät.',
'copyrightwarning2'         => 'Rehkendäq tuud, et kõiki seo lehe pääle tettüid kirotuisi ja toimõnduisi või kiä taht muutaq vai ärq kistutaq. Ku sa taha-i, et su tüüd armuhiitmäldä ümbre tetäs ja uma ärqnägemise perrä pruugitas, sis pästku-i taad siiäq. Sa piät ka lubama, et kirotit uma jutu esiq vai võtit kopimiskeelüldä paigast (täpsämbält kaeq $1). <strong>PANGU-I TAAHA TEGIJÄÕIGUISIGA KAIDSÕTUT MATÕRJAALI ILMA LUALDA!</strong>',
'longpagewarning'           => '<center>HOIATUS: Seo lehe suurus om $1 kilobaiti. Mõnõ võrgokaejaga või ollaq hätä jo 32-kilobaididsõ lehe toimõndamisõga. Märgiq perrä, kas seod lehte andnuq jakaq vähämbis lehis.</center>',
'longpageerror'             => '<strong>VIGA: Lehe suurus om $1 kilobaiti. Taad saa-i pästäq, selle et kõgõ suurõmb lubat suurus om $2 kilobaiti.</strong>',
'readonlywarning'           => '<strong>HOIATUS: Teedüskogo om huuldustöie jaos lukku pant, nii et parhilla saa-i paranduisi pästäq. Võit teksti alalõ hoitaq tekstifailin ja pästäq taa siiäq peränpoolõ.</strong>',
'protectedpagewarning'      => '<center><small>Taa leht om lukun. Taad saavaq toimõndaq õnnõ kõrraldajaõiguisiga pruukjaq.</small></center>',
'semiprotectedpagewarning'  => 'Seod lehte saavaq muutaq õnnõ nimega sisse lännüq pruukjaq.',
'cascadeprotectedwarning'   => 'Taad lehte võivaq toimõndaq õnnõ kõrraldajaõiguisiga pruukjaq, selle et taa kuulus $1 järgmädse kaidsõdu lehe hulka:',
'templatesused'             => 'Seo lehe pääl pruugiduq näüdüseq:',
'templatesusedpreview'      => 'Proovikaehusõn pruugiduq näüdüseq:',
'templatesusedsection'      => 'Seon lõigun pruugiduq näüdüseq:',
'template-protected'        => '(ärqkaidsõt)',
'template-semiprotected'    => '(ärqkaidsõduq nimeldä ja vahtsõq pruukjaq)',
'nocreatetitle'             => 'Lehekülgi luuminõ piiret',
'nocreatetext'              => '{{SITENAME}} lupa-i luvvaq vahtsit lehti.
Võit toimõndaq olõmanolõvit lehti vai [[Special:UserLogin|minnäq nimega sisse]].',
'nocreate-loggedin'         => 'Sul olõ-i lupa luvvaq vahtsit {{SITENAME}} lehti.',
'permissionserrors'         => 'Õigusõq ei klapiq',
'permissionserrorstext'     => 'Sul olõ-i lubat taad tetäq, {{PLURAL:$1|tuuperäst, et|tuuperäst, et}}:',
'recreate-deleted-warn'     => "'''Hoiatus: Sa proovit vahtsõst luvvaq lehte, miä om ärq kistutõt.'''

Kas tahat taad lehte tõtõstõ toimõndaq? Kaeq ka sissekirotust seo lehe ärqkistutamisõ kotsilõ:",

# "Undo" feature
'undo-success' => "Tagasivõtminõ läts' kõrda. Kaeq üle, kas taa om tuu, midä sa tetäq tahtsõt ja pästäq muutusõq.",
'undo-failure' => 'Tagasivõtminõ lää-s kõrda samal aol tettüide muutmiisi vastaolo peräst. Võit muutusõq käsilde tagasi võttaq.',
'undo-summary' => "Tagasi võet muutminõ #$1, mink tekk' [[Special:Contributions/$2|$2]] ([[User talk:$2|Arotus]])",

# Account creation failure
'cantcreateaccounttitle' => 'Pruukjanime luuminõ lää-s kõrda',
'cantcreateaccount-text' => "Pruukjanime luuminõ taa puutri võrgoaadrõsi päält ('''$1''') om ärq keelet. Kiildjä: [[User:$3|$3]].

$3 kirjäpant põhjus: ''$2''",

# History pages
'viewpagelogs'        => 'Kaeq seo lehe muutmisnimekirjä.',
'nohistory'           => 'Seo leheküle pääl ei olõq vanõmbit kujjõ.',
'revnotfound'         => 'Es lövväq kujjo',
'revnotfoundtext'     => 'Es lövväq su otsitut vanna kujjo.
Kaeq üle aadrõs, kost sa taad löüdäq proovõq.',
'currentrev'          => 'Viimäne kujo',
'revisionasof'        => 'Kujo $1',
'revision-info'       => 'Kujo aost $1 - tennüq $2',
'previousrevision'    => '←Vanõmb kujo',
'nextrevision'        => 'Vahtsõmb kujo→',
'currentrevisionlink' => 'Viimäne kujo',
'cur'                 => 'viim',
'next'                => 'järgm',
'last'                => 'minev',
'page_first'          => 'edimäne leht',
'page_last'           => 'viimäne leht',
'histlegend'          => "Märgiq ärq kujoq, midä tahat kõrvo säädiq ja vaodaq võrdõlõmisnuppi.
Seletüs: (viim) = lahkominegiq viimätsest kujost,
(minev) = lahkominegiq minevädsest kujost, ts = väiku (tsill'okõnõ) muutminõ",
'deletedrev'          => '[kistutõt]',
'histfirst'           => 'Edimädseq',
'histlast'            => 'Viimädseq',
'historysize'         => '({{PLURAL:$1|1 bait|$1 baiti}})',
'historyempty'        => '(tühi)',

# Revision feed
'history-feed-title'          => 'Muutmislugu',
'history-feed-description'    => 'Seo lehe muutmislugu',
'history-feed-item-nocomment' => '$1 ($2)', # user at time
'history-feed-empty'          => 'Säänest lehte olõ-i. Taa või ollaq ärq kistutõt vai ümbre nimetet. Võit pruumiq [[Special:Search|otsiq]] lehti, miä võivaq ollaq taa lehega köüdedüq.',

# Revision deletion
'rev-deleted-comment'         => '(kommõntaar ärq kistutõt)',
'rev-deleted-user'            => '(pruukjanimi ärq kistutõt)',
'rev-deleted-event'           => '(kiräkotus ärq kistutõt)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">
Lehe taa kujo om avaligust arhiivist ärq kistutõt.
Lisateedüst või ollaq [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} kistutamisnimekirän].
</div>',
'rev-deleted-text-view'       => '<div class="mw-warning plainlinks">Taa kujo om avaligust pruugist ärq kistutõt, a kõrraldajaq saavaq taad nätäq. As\'a kotsilõ või teedüst olla [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} kistutusnimekirän] </div>',
'rev-delundel'                => 'näütäq/käkiq',
'revisiondelete'              => 'Kistudaq/võtaq tagasi lehe kujjõ',
'revdelete-nooldid-title'     => 'Olõ-i säänest kujjo',
'revdelete-nooldid-text'      => 'Sa olõ-i valinuq kujjo vai kujjõ.',
'revdelete-selected'          => "{{PLURAL:$2|Valit kujo|Validuq kujoq}} lehele '''$1:'''",
'logdelete-selected'          => '{{PLURAL:$1|Valit muutminõ|Validuq muutmisõq}}:',
'revdelete-text'              => 'Kistudõduq kujoq ommaq olõman lehe aoluun, a näide sissu saa-i avaligult nätäq. Seo viki tõõsõq kõrraldajaq saavaq taad käkitüt teksti lukõq ja taa tagasi avaligult nättäväs tetäq, ku olõ-i säet muid piirdmiisi.',
'revdelete-legend'            => 'Nättävüse piirdmiseq',
'revdelete-hide-text'         => 'Käkiq kujo sisu',
'revdelete-hide-name'         => 'Käkiq kujo nimi',
'revdelete-hide-comment'      => 'Käkiq kokkovõtõq',
'revdelete-hide-user'         => 'Käkiq toimõndaja pruukjanimi vai puutri võrgoaadrõs',
'revdelete-hide-restricted'   => 'Panõq naaq piirdmiseq pääle ka kõrraldajilõ',
'revdelete-suppress'          => 'Panõq teedüs lukku ka kõrraldajilõ',
'revdelete-hide-image'        => 'Käkiq teedüstü sissu',
'revdelete-unsuppress'        => 'Võtaq tagasitettüisi kujjõ päält piirdmisõq maaha',
'revdelete-log'               => 'Muutmisnimekirä märgüs:',
'revdelete-submit'            => 'Võtaq käüki valitulõ kujolõ',
'revdelete-logentry'          => 'muudõt lehe [[$1]] kujo nättävüst',
'logdelete-logentry'          => 'muudõt lehe [[$1]] muutmiisi nättävüst',
'revdelete-success'           => "'''Kujo nättävüs paika säet.'''",
'logdelete-success'           => "'''Muutmiisi nättävüs paika säet.'''",
'revdelete-uname'             => 'pruukjanimi',

# History merging
'mergehistory'       => 'Panõq lehti aoluuq kokko',
'mergehistory-box'   => 'Panõq katõ lehe muutmiisi aolugu kokko:',
'mergehistory-from'  => 'Lätteleht:',
'mergehistory-into'  => 'Tsihtleht:',
'mergehistory-list'  => 'Liidetäv muutmiisi aolugu',
'mergehistory-merge' => 'Järgmädseq lehe [[:$1]] muutmisõq või mano pandaq lehe [[:$2]] muutmisaolukku. Võit valliq kujo, minkast vahtsõmbit kujjõ kokko ei pandaq, a võrgokaeja linke pruukminõ kaotas taa teedüse ärq.',
'mergehistory-go'    => 'Näütäq kokkopantavit muutuisi',

# Diffs
'history-title'           => '"$1" muutmiisi nimekiri',
'difference'              => '(Kujjõ lahkominegiq)',
'lineno'                  => 'Rida $1:',
'compareselectedversions' => 'Võrdõlõq valituid kujjõ',
'editundo'                => 'võtaq tagasi',
'diff-multi'              => '(Kujjõ vaihõl {{PLURAL:$1|üts näütämäldä muutminõ|$1 näütämäldä muutmist}}.)',

# Search results
'searchresults'             => 'Otsmisõ tulõmusõq',
'searchresulttext'          => 'Lisateedüst otsmisõ kotsilõ kaeq [[{{MediaWiki:Helppage}}|{{SITENAME}} otsmisoppusõst]].',
'searchsubtitle'            => "Otsminõ '''[[:$1]]''' perrä",
'searchsubtitleinvalid'     => 'Otsminõ "$1"',
'noexactmatch'              => "'''Olõ-i lehte päälkiräga \"\$1\".''' Võit tuu [[:\$1|esiq luvvaq]].",
'titlematches'              => "Artiklipäälkir'ost löüt",
'notitlematches'            => "Artiklipäälkir'ost es lövväq",
'textmatches'               => 'Artiklitekstest löüt',
'notextmatches'             => 'Artiklitekstest es lövväq',
'prevn'                     => 'minevädseq $1',
'nextn'                     => 'järgmädseq $1',
'viewprevnext'              => 'Näütäq ($1) ($2) ($3).',
'search-interwiki-more'     => '(viil)',
'search-mwsuggest-enabled'  => 'näütäq soovituisi',
'search-mwsuggest-disabled' => 'ilma soovituisilda',
'search-relatedarticle'     => 'Otsiq samasugutsit lehti',
'mwsuggest-disable'         => 'Näüdäku-i AJAX-i soovituisi',
'searchrelated'             => 'samasugunõ',
'searchall'                 => 'kõik',
'showingresults'            => "{{PLURAL:$1|'''Üts''' tulõmus|'''$1''' tulõmust}} (tulõmusõst '''$2''' pääle).",
'showingresultsnum'         => "Näüdätäs {{PLURAL:$3|'''1''' tulõmus|'''$3''' tulõmust}} tulõmusõst #'''$2''' pääle.",
'showingresultstotal'       => "Tan ommaq tulõmusõq '''$1 - $2''' (kokko '''$3''')",
'nonefound'                 => '<strong>Hoiatus</strong>: otsmishäti sakõs põhjusõs om tuu, et väega sagehõhe ettetulõvit sõnno võta-i massin otsmisõ man arvõhe. Tõõnõ põhjus või ollaq
mitmõ otsmissõna pruukminõ (sis ilmusõq õnnõ leheküleq, kon ommaq kõik otsiduq sõnaq).',
'powersearch'               => 'Otsminõ',
'powersearch-legend'        => 'Laendõt otsminõ',
'search-external'           => 'Väline otsminõ',
'searchdisabled'            => "{{SITENAME}} otsminõ parhillaq ei tüütäq. Niikavva, ku otsminõ jälq tüüle saa, võit pruukiq otsmisõs alanolõvat Google'i otsikasti, a näide teedüs {{SITENAME}} sisust pruugi-i ollaq alasi kõgõ värskimb.",

# Preferences page
'preferences'              => 'Säädmine',
'mypreferences'            => 'Mu säädmiseq',
'prefs-edits'              => 'Tõimõndamiisi arv:',
'prefsnologin'             => 'Sa olõ-i nimega sisse lännüq',
'prefsnologintext'         => 'Et säädmiisi tetäq, tulõ sul [[Special:UserLogin|nimega sisse minnäq]].',
'prefsreset'               => 'Su säädmiseq ommaq puutrimälo perrä tagasi tettüq.',
'qbsettings'               => 'Kipõriba säädmine',
'qbsettings-none'          => 'Olõ-i',
'qbsettings-fixedleft'     => 'Kõgõ kural puul',
'qbsettings-fixedright'    => 'Kõgõ hüäl puul',
'qbsettings-floatingleft'  => 'Ujovahe kural puul',
'qbsettings-floatingright' => 'Ujovahe hüäl puul',
'changepassword'           => 'Muudaq salasõnna',
'skin'                     => 'Vällänägemine',
'math'                     => 'Valõmidõ näütämine',
'dateformat'               => 'Kuupäävä muud',
'datedefault'              => 'Ütskõik',
'datetime'                 => 'Kuupäiv ja kelläaig',
'math_failure'             => 'Arvosaamalda süntaks',
'math_unknown_error'       => 'Tundmalda viga',
'math_unknown_function'    => 'Tundmalda tallitus',
'math_lexing_error'        => 'Vällälugõmisviga',
'math_syntax_error'        => 'Süntaksiviga',
'math_image_error'         => 'PNG-muutus lää-s kõrda; kaeq üle, et latex, dvips, gs ja convert ommaq õigõhe paika säedüq',
'math_bad_tmpdir'          => 'Matõmaatigateksti kirotaminõ aotlistõ kausta vai taa kausta luuminõ ei lääq kõrdaq',
'math_bad_output'          => 'Matõmaatigateksti kirotaminõ välläandmiskausta vai sääntse kausta luuminõ ei lääq kõrda',
'math_notexvc'             => 'Olõ-i texvc-tüüriista; loeq tuu paikasäädmise kotsilõ math/README-st.',
'prefs-personal'           => 'Pruukjateedüs',
'prefs-rc'                 => 'Perämädseq muutmisõq',
'prefs-watchlist'          => 'Perräkaemisnimekiri',
'prefs-watchlist-days'     => 'Mitmõ päävä muutmiisi näüdädäq perräkaemisnimekirän:',
'prefs-watchlist-edits'    => 'Perräkaemisnimekirän näüdätävide muutuisi hulk:',
'prefs-misc'               => 'Muuq säädmiseq',
'saveprefs'                => 'Pästäq säädmiseq ärq',
'resetprefs'               => 'Võtaq säädmiseq tagasi',
'oldpassword'              => 'Vana salasõna',
'newpassword'              => 'Vahtsõnõ salasõna',
'retypenew'                => 'Kirodaq viilkõrd vahtsõnõ salasõna',
'textboxsize'              => 'Toimõnduskasti suurus',
'rows'                     => 'Rito',
'columns'                  => 'Tulpõ',
'searchresultshead'        => 'Otsminõ',
'resultsperpage'           => 'Tulõmuisi leheküle kotsilõ',
'contextlines'             => 'Rito tulõmusõn',
'contextchars'             => 'Konteksti pikkus ria pääl',
'stub-threshold'           => '<a href="#" class="stub">Kehväkese lehe</a> näütämispiir (baidõn):',
'recentchangesdays'        => 'Päivi, midä näüdädäq viimätsin muutmiisin',
'recentchangescount'       => 'Päälkirjo hulk viimätsin muutmiisin',
'savedprefs'               => 'Su muutmisõq ommaq pästedüq.',
'timezonelegend'           => 'Aovüü',
'timezonetext'             => 'Paikligu ao ja serveri ao (maailmaao) vaheq (tunniq).',
'localtime'                => 'Paiklik aig',
'timezoneoffset'           => 'Aovaheq',
'servertime'               => 'Serveri aig',
'guesstimezone'            => 'Võtaq aig võrgokaejast',
'allowemail'               => 'Lupaq tõisil pruukjil mullõ e-posti saataq',
'defaultns'                => 'Otsiq vaikimiisi naist nimeruumõst:',
'default'                  => 'vaikimiisi',
'files'                    => 'Teedüstüq',

# User rights
'userrights'               => 'Pruukja õiguisi muutminõ', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'   => 'Pruukjaõiguisi muutminõ',
'userrights-user-editname' => 'Kirodaq pruukjanimi:',
'editusergroup'            => 'Muudaq pruukjidõ rühmi',
'editinguser'              => "Pruukja '''[[User:$1|$1]]''' õigusõq ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup' => 'Pruukjidõrühmä valik',
'saveusergroups'           => 'Pästäq pruukjidõrühmä muutmisõq',
'userrights-groupsmember'  => 'Kuulus rühmä:',
'userrights-reason'        => 'Muutmisõ põhjus:',

# Groups
'group'            => 'Rühm:',
'group-bot'        => 'Robodiq',
'group-sysop'      => 'Kõrraldajaq',
'group-bureaucrat' => 'Pääkõrraldajaq',
'group-all'        => '(kõik)',

'group-bot-member'        => 'Robot',
'group-sysop-member'      => 'Kõrraldaja',
'group-bureaucrat-member' => 'Pääkõrraldaja',

'grouppage-bot'        => '{{ns:project}}:Robodiq',
'grouppage-sysop'      => '{{ns:project}}:Kõrraldajaq',
'grouppage-bureaucrat' => '{{ns:project}}:Pääkõrraldajaq',

# User rights log
'rightslog'      => 'Pruukmisõiguisi muutmisõ nimekiri',
'rightslogtext'  => 'Taa om pruukmisõiguisi muutmiisi nimekiri.',
'rightslogentry' => 'Pruukja $1 õigusõq muudõti ümbre rühmäst $2 rühmä $3',
'rightsnone'     => '(olõ-i õiguisi)',

# Recent changes
'nchanges'                          => '$1 {{PLURAL:$1|muutminõ|muutmiisi}}',
'recentchanges'                     => 'Viimädseq muutmisõq',
'recentchangestext'                 => 'Kaeq seo lehe pääl viimätsit muutmiisi.',
'recentchanges-feed-description'    => 'Kaeq seo lehe pääl {{SITENAME}} viimätsit muutmiisi.',
'rcnote'                            => 'Tan ommaq {{PLURAL:$1|üts muutus|$1 viimäst muutmist}}, miä ommaq tettüq {{PLURAL:$2|üte viimädse päävä|$2 viimädse päävä}} seen (kuupääväst $5, $4 lugõma naatõn).',
'rcnotefrom'                        => "Tan ommaq muutmisõq kuupääväst '''$2''' pääle (näüdätäs kooniq '''$1''' muutmist).",
'rclistfrom'                        => 'Näütäq muutmiisi kuupääväst $1 pääle',
'rcshowhideminor'                   => '$1 väikuq parandusõq',
'rcshowhidebots'                    => '$1 robodiq',
'rcshowhideliu'                     => '$1 nimega pruukjaq',
'rcshowhideanons'                   => '$1 nimeldä pruukjaq',
'rcshowhidepatr'                    => '$1 kontrolliduq muutmisõq',
'rcshowhidemine'                    => '$1 mu toimõndusõq.',
'rclinks'                           => 'Näütäq viimädseq $1 muutmist, miä ommaq tettüq viimädse $2 päävä seen. $3',
'diff'                              => 'lahk',
'hist'                              => 'aol',
'hide'                              => 'Käkitäseq',
'show'                              => 'Näüdätäseq',
'minoreditletter'                   => 'ts',
'newpageletter'                     => 'V',
'boteditletter'                     => 'rb',
'number_of_watching_users_pageview' => '[{{PLURAL:$1|$1 perräkaejat|üts perräkaeja}}]',
'rc_categories'                     => 'Õnnõ katõgoorijist (eräldedäs märgiga "|")',
'rc_categories_any'                 => 'Miä taht',

# Recent changes linked
'recentchangeslinked'          => 'Siiäq putvaq muutmisõq',
'recentchangeslinked-title'    => 'Muutusõq noidõ lehti pääl, kohe näüdätäs lähe päält "$1"',
'recentchangeslinked-noresult' => 'Taaha putvit lehti olõ-i taa ao seen muudõt.',
'recentchangeslinked-summary'  => "Taan nimekirän ommaq noidõ lehti muutmisõq, mink pääle näütäs seo lehe päält linke. Naad leheq ommaq [[Special:Watchlist|perräkaemisnimekirän]] märgidüq '''paksu kiräga'''.",

# Upload
'upload'                      => 'Teedüstü üleslaatminõ',
'uploadbtn'                   => 'Üleslaatminõ',
'reupload'                    => 'Vahtsõst üleslaatminõ',
'reuploaddesc'                => 'Tagasi üleslaatmisõ vormi mano.',
'uploadnologin'               => 'Sa olõ-i nimega sisse lännüq',
'uploadnologintext'           => 'Kui tahat teedüstüid üles laatiq, piät [[Special:UserLogin|nimega sisse minemä]].',
'upload_directory_read_only'  => 'Serveril olõ-i üleslaatmiskausta ($1) kirotamisõ õigust.',
'uploaderror'                 => 'Üleslaatmisviga',
'uploadtext'                  => '<strong>PIÄQ KINNIQ!</strong> Inne ülelaatmist kaeq, et taa käünüq {{SITENAME}} [[{{MediaWiki:Policy-url}}|pilte pruukmisõ kõrra]] perrä.
<p>Innembält üleslaadiduq pildiq lövvät [[Special:ImageList|pilte nimekiräst]].
<p>Järgmädse vormi abiga saat laatiq üles vahtsit pilte ummi artiklide ilostamisõs. Inämbüsel võrgokaejil näet nuppi "Browse..." vai "Valiq...", miä vii sinno
su opõratsioonisüsteemi standardsõhe teedüstüide vallalõtegemise aknõhe. Teedüstü valimisõs pandas timä nimi tekstivälä pääle, miä om nupi kõrval.
Piät ka kastikõistõ märgi tegemä, et kinnütät,
et sa riku-i taad teedüstüt üles laatõn kinkagi tegijäõiguisi. Üleslaatmisõs vaodaq nupi pääle "Üleslaatminõ". Taa või võttaq piso aigo, esiqeräle sis, kui sul om aiglanõ võrgoliin. <p>Soovitõdus kujos om pääväpildel JPEG, joonistuisil
ja ikooni muudu pildel PNG, helle jaos OGG.
Nimedäq umaq teedüstüq nii, et nimi ütelnüq midägi selgehe teedüstü sisu kotsilõ. Taa avitas segähüisi ärq hoitaq. Ku panõt artiklilõ pildi mano, pruugiq sääntse kujoga linki: <b>[[image:pilt.jpg]]</b> vai <b>[[image:pilt.png|alt. tekst]]</b>.
Helüteedüstü puhul: <b>[[media:teedüstü.ogg]]</b>.
<p>Panõq tähele, et nigu ka tõisi {{SITENAME}} lehekülgi pääl, võivaq tõõsõq su laadituid teedüstüid leheküle jaos muutaq vai ärq kistutaq. {{SITENAME}} kur\'astõ pruukjalõ võidas manoqpäsemine kinniq pandaq.',
'uploadlog'                   => 'Üleslaatmiisi nimekiri',
'uploadlogpage'               => 'Üleslaatmiisi nimekiri',
'uploadlogpagetext'           => 'Nimekiri viimätsist üleslaatmiisist. Kelläaoq ommaq märgidüq serveri aoarvamisõ perrä.',
'filename'                    => 'Teedüstü nimi',
'filedesc'                    => 'Kokkovõtõq',
'fileuploadsummary'           => 'Kokkovõtõq:',
'filestatus'                  => 'Teedüstü tegijäõigusõq:',
'filesource'                  => 'Kost peri:',
'uploadedfiles'               => 'Üleslaadiduq teedüstüq',
'ignorewarning'               => 'Pangu-i hoiatust tähele ja pästäq tuugiperäst.',
'ignorewarnings'              => 'Pangu-i üttegi hoiatust tähele',
'minlength1'                  => 'Teedüstünimen piät olõma vähämbält üts täht.',
'illegalfilename'             => 'Teedüstü nimen "$1" om lehenime jaos lubamaldaq märke. Vaihtaq teedüstü nimme ja prooviq taa vahtsõst üles laatiq.',
'badfilename'                 => 'Teedüstü nimi om ärq muudõt. Vahtsõnõ nimi om "$1".',
'filetype-badmime'            => 'Teedüstüid, mink MIME-tüüp om "$1" tohe-i üles laatiq.',
'filetype-missing'            => 'Teedüstül olõ-i laendust (nt ".jpg").',
'large-file'                  => 'Teedüstüq tohe-i ollaq suurõmbaq, ku $1, a taa teedüstü om $2.',
'largefileserver'             => 'Teedüstü om suurõmb ku server lupa.',
'emptyfile'                   => "Teedüstü, midä sa proovõq üles laatiq paistus ollõv tühi. Kaeq üle, et kirotit nime õigõhe ja et taa olõ-i serverile pall'o suur.",
'fileexists'                  => 'Sama nimega teedüstü om jo olõman. Katso <strong><tt>$1</tt></strong>, ku sa olõ-i kimmäs, et tahat taad muutaq.',
'fileexists-extension'        => 'Sääntse nimega teedüstü om jo olõman:<br />
Üleslaaditava teedüstü nimi: <strong><tt>$1</tt></strong><br />
Olõmanolõva teedüstü nimi: <strong><tt>$2</tt></strong><br />
Ainugõnõ vaih om laendusõ suurõ/väiku algustähe man. Kaeq perrä, kas naaq ommaq üts ja tuusama teedüstü.',
'fileexists-thumb'            => "<center>'''Olõmanollõv pilt'''</center>",
'fileexists-thumbnail-yes'    => 'Taa paistus ollõv vähändet pilt <i>(thumbnail)</i>. Kaeq teedüstü <strong><tt>$1</tt></strong>üle.<br />
Ku ülekaet teedüstü om sama pilt alguperälidsen suurusõn, sis olõ-i vaia eräle vähändedüt pilti üles laatiq.',
'file-thumbnail-no'           => 'Teedüstü nimi nakkas pääle <strong><tt>$1</tt></strong>. Taa paistus ollõv vähändet pilt <i>(thumbnail)</i>. Ku sul om olõman taa pilt tävven suurusõn, sis laadiq üles tuu, ku olõ-i, sis muudaq teedüstü nimi ärq.',
'fileexists-forbidden'        => 'Sääntse nimega teedüstü om jo olõman. Pästäq teedüstü tõõsõ nimega. Parhillanõ teedüstü: [[Image:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => 'Sama nimega teedüstü om jo olõman jaetuidõ teedüstüide hulgan. Pästäq teedüstü mõnõ tõõsõ nime ala. Parhillanõ teedüstü: [[Image:$1|thumb|center|$1]]',
'successfulupload'            => "Üleslaatminõ läts' kõrda",
'uploadwarning'               => 'Üleslaatmishoiatus',
'savefile'                    => 'Pästäq teedüstü ärq',
'uploadedimage'               => 'laadõ üles "$1"',
'overwroteimage'              => 'üles laadit "[[$1]]" vahtsõnõ kujo',
'uploaddisabled'              => 'Üleslaatminõ lää-s kõrda',
'uploaddisabledtext'          => '{{SITENAME}} lupa-i parhilla teedüstüid üles laatiq.',
'uploadscripted'              => 'Seol teedüstül om HTML-kuud vai skripte, minkast võrgokaeja või võlssi arvo saiaq.',
'uploadcorrupt'               => 'Teedüstü om viganõ vai om täl võlss laendus. Olõq hää, kaeq tä üle ja laadiq vahtsõst üles.',
'uploadvirus'                 => 'Teedüstül om viirus man! Kaeq: $1',
'sourcefilename'              => 'Teedüstü nimi:',
'destfilename'                => 'Teedüstü nimi vikin:',
'watchthisupload'             => 'Kaeq taa lehe perrä',
'filewasdeleted'              => 'Sääntse nimega teedüstü om jo üles laadit ja sis ärq kistutõt. Kaeq üle $1 inne ku nakkat jälq üles laatma.',
'upload-wasdeleted'           => "'''Hoiatus: Sa proovit üles laatiq teedüstüt, miä om innemb ärq kistutõt.'''

Kas olõt kimmäs, et tahat taad üles laatiq? Kaeq ka sissekirotust taa teedüstü ärqkistutamisõ kotsilõ:",

'upload-proto-error'      => 'Viganõ protokoll',
'upload-proto-error-text' => 'Üles saa laatiq õnnõ aadrõssidõ päält, mink alostusõn om <code>http://</code> vai <code>ftp://</code>.',
'upload-file-error'       => 'Sisemäne viga',
'upload-file-error-text'  => 'Aotlidsõ teedüstü luuminõ lää-s kõrda. Küsüq api kõrraldaja käest.',
'upload-misc-error'       => 'Üleslaatmisõ viga',
'upload-misc-error-text'  => 'Teedüstü üleslaatminõ lää-s kõrda. Kaeq üle, kas su ant aadrõs om masva ja õigõhe kirotõt. Ku viga iks ärq kao-i, küsüq api kõrraldaja käest.',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => 'Lövvä-s säänest aadrõssit',
'upload-curl-error6-text'  => 'Lövvä-s säänest aadrõssit. Kaeq üle, kas aadrõss om iks õigõ ja tüütäs.',
'upload-curl-error28'      => 'Saa-s ao pääle üles laaditus',
'upload-curl-error28-text' => 'Taa aadrõsi päält saa-s ao pääle vastust. Oodaq vähä ja prooviq vahtsõst.',

'license'            => 'Litsents:',
'nolicense'          => 'Olõ-i litsentsi valit',
'license-nopreview'  => '(Saa-i kaiaq)',
'upload_source_url'  => ' (avalik tüütäv võrgoaadrõs)',
'upload_source_file' => ' (teedüstü su puutrin)',

# Special:ImageList
'imagelist_search_for'  => 'Pildi nime otsminõ:',
'imgfile'               => 'teedüstü',
'imagelist'             => 'Pilte nimekiri',
'imagelist_date'        => 'Kuupäiv',
'imagelist_name'        => 'Nimi',
'imagelist_user'        => 'Pruukja',
'imagelist_size'        => 'Suurus (baidõn)',
'imagelist_description' => 'Seletüs',

# Image description page
'filehist'                  => 'Teedüstü aolugu',
'filehist-help'             => "Klõpsaq kuupäävä/kelläao pääl, et nätäq määne taa teedüstü sis oll'.",
'filehist-deleteall'        => 'kistudaq kõik ärq',
'filehist-deleteone'        => 'kistudaq taa ärq',
'filehist-revert'           => 'võtaq tagasi',
'filehist-current'          => 'parhillanõ',
'filehist-datetime'         => 'Kuupäiv/Kelläaig',
'filehist-user'             => 'Pruukja',
'filehist-dimensions'       => 'Suurus',
'filehist-filesize'         => 'Teedüstü suurus',
'filehist-comment'          => 'Seletüs:',
'imagelinks'                => 'Pildilingiq',
'linkstoimage'              => 'Taa pildi pääle {{PLURAL:$1|näütäs lehekülg|näütäseq leheküleq}}:',
'nolinkstoimage'            => 'Taa pildi pääle näütä-i ütski lehekülg.',
'sharedupload'              => 'Taa om ütine teedüstü, taad võivaq pruukiq ka tõõsõq vikiq.',
'shareduploadwiki'          => 'Taa kotsilõ saa lähkümbält kaiaq $1.',
'shareduploadwiki-linktext' => 'seletüsleheküle päält',
'noimage'                   => 'Olõ-i säänest teedüstüt, võit taa esiq {{SITENAME}}he $1.',
'noimage-linktext'          => 'üles laatiq',
'uploadnewversion-linktext' => 'Laadiq taa teedüstü vahtsõnõ kujo',
'imagepage-searchdupe'      => 'Otsiq ütesugutsit teedüstüid',

# File reversion
'filerevert'         => 'Võtaq tagasi $1',
'filerevert-legend'  => 'Võtaq tagasi teedüstü',
'filerevert-comment' => 'Põhjus:',
'filerevert-submit'  => 'Võtaq tagasi',

# File deletion
'filedelete'         => 'Kistudaq ärq $1',
'filedelete-legend'  => 'Kistudaq teedüstü ärq',
'filedelete-intro'   => "Sa kistutat ärq '''[[Media:$1|$1]]'''.",
'filedelete-comment' => 'Seletüs:',
'filedelete-submit'  => 'Kistudaq',
'filedelete-success' => "'''$1''' om ärq kistutõt.",
'filedelete-nofile'  => "'''$1''' olõ-i seo lehe pääl.",

# MIME search
'mimesearch'         => 'MIME-otsminõ',
'mimesearch-summary' => 'Taa lehe pääl saat otsiq teedüstüid näide MIME-tüübi perrä. Kirodaq: sisutüüp/alltüüp, nt <tt>image/jpeg</tt>.',
'mimetype'           => 'MIME-tüüp:',
'download'           => 'laat',

# Unwatched pages
'unwatchedpages' => 'Perräkaemisõlda leheq',

# List redirects
'listredirects' => 'Ümbresaatmisõq',

# Unused templates
'unusedtemplates'     => 'Pruukmalda näüdüseq',
'unusedtemplatestext' => 'Tan ommaq kirän kõik näüdüseq, midä olõ-i ütegi lehe pääle pant. Inne ku naaq ärq kistutat, kaeq perrä, kas näide pääle kost määnest linki näütä-i.',
'unusedtemplateswlh'  => 'muuq lingiq',

# Random page
'randompage'         => 'Johuslinõ artikli',
'randompage-nopages' => 'Seon nimeruumin olõ-i üttegi lehte.',

# Random redirect
'randomredirect'         => 'Johuslinõ ümbresaatminõ',
'randomredirect-nopages' => 'Seon nimeruumin olõ-i üttegi ümbresaatmist.',

# Statistics
'statistics'             => 'Statistiga',
'sitestats'              => 'Lehekülgi statistiga',
'userstats'              => 'Pruukjidõ statistiga',
'sitestatstext'          => "Teedüskogon om kokko <b>$1</b> lehekülge.

Taa numbri seen ommaq ka arotusküleq, abiartikliq, väega lühkeseq leheküleq, ümbresaatmisleheküleq ja muuq leheq, mink pääl arvadaq olõ-i entsüklopeediäartiklit. Ilma naid rehkendämäldä om parhilla '''$2''' {{SITENAME}} lehekülge, midä või pitäq artiklis. Üles om laadit '''$8''' teedüstüt. Lehti om kaet kokko '''$3''' kõrda ja toimõndõt '''$4''' kõrda. Tuu om keskmädselt '''$5''' kaemist lehe kotsilõ ja '''$6''' kaemist toimõndusõ kotsilõ. Hoolõkandõtallituisi om järekõrran '''$7'''.",
'userstatstext'          => "Kirjäpantuid pruukjit om '''$1'''. Naist '''$2''' ($4%) ommaq kõrraldaja õiguisiga pruukjaq ($5).",
'statistics-mostpopular' => 'Kõgõ kaetumbaq leheq',

'disambiguations'      => 'Lingiq, miä näütäseq täpsüstüslehekülgi pääle',
'disambiguationspage'  => 'Template:Linke täpsüstüslehekülile',
'disambiguations-text' => "Naaq leheq näütäseq '''täpsüstüslehti''' pääle.
Tuu asõmal pidänüq nä näütämä as'a sisu pääle.<br />
Lehte peetäs täpsüstüslehes, ku timän om pruugit näüdüst, kohe näütäs link lehelt [[MediaWiki:Disambiguationspage]].",

'doubleredirects'     => 'Katõkõrdsõq ümbresaatmisõq',
'doubleredirectstext' => 'Egä ria pääl om ärq tuud edimäne ja tõõnõ ümbresaatmisleht ja niisama tõõsõ ümbresaatmislehe link, miä näütäs hariligult kotusõ pääle, kohe edimäne ümbersaatmisleht pidänüq õkva näütämä.',

'brokenredirects'        => 'Vigadsõq ümbresaatmisõq',
'brokenredirectstext'    => 'Naaq ümbresaatmisõq näütäseq lehti pääle, midä olõ-i olõman:',
'brokenredirects-edit'   => '(toimõndaq)',
'brokenredirects-delete' => '(kistudaq ärq)',

'withoutinterwiki'         => 'Keelelingeldä leheq',
'withoutinterwiki-summary' => 'Nail lehil olõ-i linke tõisi kiili lehti pääle:',

'fewestrevisions' => 'Kõgõ veidemb kõrdo toimõndõduq leheq',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|bait|baiti}}',
'ncategories'             => '$1 {{PLURAL:$1|katõgooria|katõgooriaq}}',
'nlinks'                  => '$1 {{PLURAL:$1|link|linki}}',
'nmembers'                => '$1 {{PLURAL:$1|liigõq|liigõt}}',
'nrevisions'              => '$1 {{PLURAL:$1|muutminõ|muutmist}}',
'nviews'                  => 'Käümiisi: $1',
'specialpage-empty'       => 'Taa leht om tühi.',
'lonelypages'             => 'Artikliq, kohe olõ-i linke',
'lonelypagestext'         => 'Nailõ lehile olõ-i muialt vikist linke.',
'uncategorizedpages'      => 'Katõgoorijilda leheq',
'uncategorizedcategories' => 'Katõgoorijilda katõgooriaq',
'uncategorizedimages'     => 'Katõgoorijilda teedüstüq',
'uncategorizedtemplates'  => 'Katõgoorialdaq näüdüseq',
'unusedcategories'        => 'Pruukmalda katõgooriaq',
'unusedimages'            => 'Pruukmaldaq pildiq',
'popularpages'            => "Pall'okäütüq leheküleq",
'wantedcategories'        => 'Kõgõ tahetumbaq katõgooriaq',
'wantedpages'             => 'Kõgõ tahetumbaq artikliq',
'mostlinked'              => 'Leheq, kohe om kõgõ rohkõmb linke',
'mostlinkedcategories'    => 'Katõgooriaq, kohe om kõgõ rohkõmb linke',
'mostlinkedtemplates'     => 'Näüdüseq, kohe näütäs kõgõ rohkõmb linke',
'mostcategories'          => 'Artikliq, mil om kõgõ rohkõmb katõgoorijit',
'mostimages'              => 'Kõgõ inämb pruugiduq teedüstüq',
'mostrevisions'           => 'Artikliq, mil om kõgõ rohkõmb toimõnduisi',
'prefixindex'             => 'Leheq päälkirä algusõ perrä',
'shortpages'              => 'Lühküq artikliq',
'longpages'               => 'Pikäq artikliq',
'deadendpages'            => 'Leheq, kon olõ-i linke',
'deadendpagestext'        => 'Nail lehil olõ-i linke tõisi viki lehti pääle.',
'protectedpages'          => 'Kaidsõduq leheq',
'protectedpagestext'      => 'Naaq leheq kaidsõtasõq ärq tõistõ paika panõkist ja muutmisõst.',
'protectedpagesempty'     => 'Olõ-i kaidsõtuid lehti.',
'listusers'               => 'Pruukjaq',
'newpages'                => 'Vahtsõq leheküleq',
'newpages-username'       => 'Pruukjanimi:',
'ancientpages'            => 'Kõgõ vanõmbaq leheküleq',
'move'                    => 'Nõstaq ümbre',
'movethispage'            => 'Panõq lehekülg tõistõ paika',
'unusedimagestext'        => 'Panõq tähele, et tõõsõq leheküleq, nigu tõisi maiõ Vikipeediäq, võivaq pandaq siiäq lehekülgi pääle õkvalinke, tuuperäst võidas siin antuid pilte ka parhilla aktiivsõhe pruukiq.',
'unusedcategoriestext'    => 'Naaq katõgooriaq ommaq olõman, a naid pruugita-i.',
'notargettitle'           => 'Otsitut lehte olõ-i',
'notargettext'            => 'Sa olõ-i andnuq lehte ega pruukjat, minka taad tallitust tetäq.',
'nopagetext'              => 'Säänest lehte olõ-i olõman.',

# Book sources
'booksources'               => 'Raamaduq',
'booksources-search-legend' => 'Otsiq raamatut',
'booksources-go'            => 'Otsiq',
'booksources-text'          => 'Tan om linke lehekülile, kon müvväs raamatit vai andas raamatidõ kotsilõ teedüst.',

# Special:Log
'specialloguserlabel'  => 'Pruukja:',
'speciallogtitlelabel' => 'Päälkiri:',
'log'                  => 'Muutmisnimekiri',
'all-logs-page'        => 'Kõik muutmisõq',
'log-search-legend'    => 'Muutmiisi otsminõ',
'log-search-submit'    => 'Otsiq',
'alllogstext'          => '{{SITENAME}} kõiki muutmiisi - kistutamiisi, kaitsmiisi, kinniqpidämiisi ja kõrraldamiisi ütine nimekiri. Võit valliq ka eräle muutmistüübi, pruukjanime vai lehe päälkirä perrä.',
'logempty'             => 'Muutmisnimekirän olõ-i sääntsit kiräkotussit.',
'log-title-wildcard'   => 'Otsiq päälkirjo, miä alostasõq taa tekstiga',

# Special:AllPages
'allpages'          => 'Kõik artikliq',
'alphaindexline'    => '$1 kooniq $2',
'nextpage'          => 'Järgmäne lehekülg ($1)',
'prevpage'          => 'Mineväne lehekülg ($1)',
'allpagesfrom'      => 'Nakkaq näütämä lehekülest:',
'allarticles'       => 'Kõik artikliq',
'allinnamespace'    => 'Kõik nimeruumi $1 leheq',
'allnotinnamespace' => 'Kõik leheq, midä olõ-i nimeruumin $1',
'allpagesprev'      => 'Mineväne',
'allpagesnext'      => 'Järgmäne',
'allpagessubmit'    => 'Näütäq',
'allpagesprefix'    => 'Näütäq lehti, mink alostusõn om:',
'allpagesbadtitle'  => "Taa päälkiri oll' viganõ vai vikidevaihõlidsõ edejakuga. Tan või ollaq märke, midä tohe-i päälkir'on pruukiq.",
'allpages-bad-ns'   => '{{SITENAME}}n olõ-i nimeruumi "$1".',

# Special:Categories
'categories'         => 'Katõgooriaq',
'categoriespagetext' => 'Seon vikin ommaq sääntseq katõgooriaq:',

# Special:ListUsers
'listusersfrom'      => 'Näütäq pruukjit alostõn:',
'listusers-submit'   => 'Näütäq',
'listusers-noresult' => 'Olõ-s pruukjit.',

# Special:ListGroupRights
'listgrouprights' => 'Pruukjarühmi õigusõq',

# E-mail user
'mailnologin'     => 'Olõ-i saatja aadrõssit',
'mailnologintext' => 'Sa piät olõma [[Special:UserLogin|nimega sisse lännüq]]
ja sul piät umin [[Special:Preferences|säädmiisin]] olõma e-postiaadrõs, et sa saasiq tõisilõ pruukjilõ e-kirjo saataq.',
'emailuser'       => 'Kirodaq taalõ pruukjalõ e-kiri',
'emailpage'       => 'Kirodaq pruukjalõ e-kiri',
'emailpagetext'   => 'Ku taa pruukja om ummi säädmiisihe pandnuq uma tüütävä e-postiaadrõsi, saa taa vormi abiga tälle saataq üte kirä. Kirän jääs nätäq saatja aadrõs, et kirä saaja saanuq kiräle vastadaq.',
'usermailererror' => 'Saatmisõ viga:',
'defemailsubject' => '{{SITENAME}} e-post',
'noemailtitle'    => 'Olõ-i e-postiaadrõssit',
'noemailtext'     => 'Taa pruukja olõ-i andnuq umma e-postiaadrõssit.',
'emailfrom'       => 'Kink käest',
'emailto'         => 'Kinkalõ',
'emailsubject'    => 'Teema',
'emailmessage'    => 'Sõnnom',
'emailsend'       => 'Saadaq',
'emailccme'       => 'Saadaq mullõ kopi mu e-kiräst.',
'emailccsubject'  => 'Kopi su kiräst aadrõsi pääle $1: $2',
'emailsent'       => 'E-post saadõt',
'emailsenttext'   => 'Sõnnom saadõt.',

# Watchlist
'watchlist'            => 'Perräkaemisnimekiri',
'mywatchlist'          => 'mu perräkaemisnimekiri',
'watchlistfor'         => "(pruukjalõ '''$1''')",
'nowatchlist'          => 'Perräkaemisnimekiri om tühi.',
'watchlistanontext'    => 'Perräkaemisnimekirä pruukmisõs $1.',
'watchnologin'         => 'Olõ-i nimega sisse mint',
'watchnologintext'     => 'Perräkaemisnimekirä muutmisõs piät [[Special:UserLogin|nimega sisse minemä]].',
'addedwatch'           => 'Perräkaemisnimekirjä pant',
'addedwatchtext'       => "Lehekülg \"<nowiki>\$1</nowiki>\" om pant su [[Special:Watchlist|perräkaemisnimekirjä]]. Edespididseq muutmisõq seo lehe ja tä arotuskülgi pääl pandasõq ritta siin ja [[Special:RecentChanges|viimätside muutmiisi lehe pääl]] tuvvasõq '''paksun kirän'''. Ku tahat taad lehte perräkaemisnimekiräst vällä võttaq, klõpsaq nuppi \"Lõpõdaq perräkaeminõ ärq\".",
'removedwatch'         => 'Perräkaemisnimekiräst vällä võet',
'removedwatchtext'     => 'Lehekülg "<nowiki>$1</nowiki>" om su perräkaemisnimekiräst vällä võet.',
'watch'                => 'Kaeq perrä',
'watchthispage'        => 'Kaeq taad lehekülge perrä',
'unwatch'              => 'Lõpõdaq perräkaeminõ ärq',
'unwatchthispage'      => 'Lõpõdaq perräkaeminõ ärq',
'notanarticle'         => 'Olõ-i artikli',
'watchnochange'        => 'Taa ao seen olõ-i üttegi perräkaetavat lehte muudõt.',
'watchlist-details'    => 'Perräkaemisnimekirän om {{PLURAL:$1|$1 leht|$1 lehte}} (rehkendämäldä arotuslehti).',
'wlheader-enotif'      => '* E-postiga teedäqandmisõq ommaq käügin.',
'wlheader-showupdated' => "* Leheq, midä om muudõt päält su viimäst käümist, ommaq '''paksun kirän'''",
'watchmethod-recent'   => 'kontrollitas perräkaetavidõ lehti perämäidsi muutmiisi',
'watchmethod-list'     => 'perräkaetavidõ lehti perämädseq muutmisõq',
'watchlistcontains'    => 'Perräkaemisnimekirän om $1 {{PLURAL:$1|leht|lehte}}.',
'iteminvalidname'      => "Hädä lehega '$1'! Lehe nimen om viga.",
'wlnote'               => "Tan om '''$1''' {{PLURAL:$1|muutminõ|muutmist}} viimädse '''$2''' tunni ao seen.",
'wlshowlast'           => 'Näütäq viimädseq $1 tunni $2 päivä $3',
'watchlist-show-bots'  => 'Näütäq robotidõ toimõnduisi',
'watchlist-hide-bots'  => 'Näüdäku-i robotidõ toimõnduisi',
'watchlist-show-own'   => 'Näütäq muq toimõnduisi',
'watchlist-hide-own'   => 'Näüdäku-i muq toimõnduisi',
'watchlist-show-minor' => "Näütäq tsill'okõisi muutmiisi",
'watchlist-hide-minor' => "Näüdäku-i tsill'okõisi muutmiisi",

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Pandas perräkaemisnimekirjä...',
'unwatching' => 'Võetas perräkaemisõ alt maaha...',

'enotif_mailer'                => '{{SITENAME}} lehe muutumisteedüs',
'enotif_reset'                 => 'Märgiq kõik leheq ülekaetuis',
'enotif_newpagetext'           => 'Taa om vahtsõnõ leht.',
'enotif_impersonal_salutation' => '{{SITENAME}} pruukja',
'changed'                      => 'lehte muutnuq',
'created'                      => 'lehe loonuq',
'enotif_subject'               => '$PAGEEDITOR om $CHANGEDORCREATED $PAGETITLE',
'enotif_lastvisited'           => 'Lehel $1 ommaq kõik päält suq perämäst käümist tettüq muutmisõq.',
'enotif_lastdiff'              => 'Taa muutusõ nägemises kaeq: $1.',
'enotif_anon_editor'           => 'nimeldä pruukja $1',
'enotif_body'                  => 'Hüä $WATCHINGUSERNAME,

{{SITENAME}} lehte $PAGETITLE $CHANGEDORCREATED $PAGEEDITDATE $PAGEEDITOR, parhillast kujjo kaeq $PAGETITLE_URL.

$NEWPAGE

Muutja kokkovõtõq: $PAGESUMMARY $PAGEMINOREDIT

Kirodaq muutjalõ:
e-post: $PAGEEDITOR_EMAIL
viki: $PAGEEDITOR_WIKI

Inämb seo lehe kotsilõ teedäqandmiisi saadõta-i. Võit ka kõik su perräkaetavidõ lehti muutmisõ kuulutusõq ärq keeldäq.

{{SITENAME}} teedäqandmiskõrraldus

Perräkaemisnimekirä säädmiisi saat muutaq lehe pääl: {{fullurl:Special:Watchlist/edit}}

As\'a kotsilõ mano kaiaq ja küssü saat lehe päält: {{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => 'Kistudaq lehekülg ärq',
'confirm'                     => 'Kinnüdäq',
'excontent'                   => "sisu oll': '$1'",
'excontentauthor'             => "sisu oll': '$1' (ja ainugõnõ toimõndaja oll' '[[Special:Contributions/$2|$2]]')",
'exbeforeblank'               => "inne tühästegemist oll': '$1'",
'exblank'                     => "leht oll' tühi",
'historywarning'              => 'Hoiatus: Lehel, midä tahat ärq kistutaq, om olõman aolugu:',
'confirmdeletetext'           => 'Sa kistutat teedüskogost periselt ärq lehe vai pildi üten kõgõ timä aoluuga.
Kinnüdäq, et sa tahat tuud tõtõstõ tetäq, et sa saat arvo, miä tuust tullaq või ja et tuu, miä sa tiit, klapis [[{{MediaWiki:Policy-url}}|sisekõrraga]].',
'actioncomplete'              => 'Tallitus valmis',
'deletedtext'                 => '"<nowiki>$1</nowiki>" om ärq kistutõt.
Perämäidsi kistutuisi nimekirjä näet siist: $2.',
'deletedarticle'              => '"$1" kistutõt',
'dellogpage'                  => 'Kistutõduq leheküleq',
'dellogpagetext'              => 'Naaq ommaq perämädseq kistutamisõq.
Kelläaoq ummaq serveriao perrä.',
'deletionlog'                 => 'Kistutõduq leheküleq',
'reverted'                    => 'Minti tagasi vana kujo pääle',
'deletecomment'               => 'Kistutamisõ põhjus',
'deleteotherreason'           => 'Muu põhjus vai täpsüstüs:',
'deletereasonotherlist'       => 'Muu põhjus',
'deletereason-dropdown'       => "*Hariliguq kistutamisõ põhjusõq
** Kirotaja hindä palvõl
** Tegijäõigusõ rikminõ
** Lehe ts'urkminõ",
'rollback'                    => 'Mineq tagasi vana kujo pääle',
'rollback_short'              => 'Võtaq tagasi',
'rollbacklink'                => 'võtaq tagasi vana kujo',
'rollbackfailed'              => 'Muutmiisi tagasivõtminõ lää-s kõrda',
'cantrollback'                => 'Saa-i muutmiisi tagasi pöördäq; viimäne muutja om lehe ainugõnõ toimõndaja.',
'alreadyrolled'               => 'Pruukja [[User:$2|$2]] ([[User talk:$2|arotus]]) tettüid lehe [[:$1]] muutmiisi saa-i tagasi võttaq, selle et pruukja [[User:$3|$3]] ([[User talk:$3|arotus]]) om tennüq vahtsõmbit muutmiisi.',
'editcomment'                 => 'Toimõndamiskokkovõtõq oll\': "<i>$1</i>".', # only shown if there is an edit comment
'revertpage'                  => 'Pruukja [[Special:Contributions/$2|$2]] ([[User_talk:$2|arotus]]) toimõndusõq pöörediq tagasi ja leht panti tagasi pruukja [[User:$1|$1]] tettü kujo pääle.', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'sessionfailure'              => 'Paistus ollõv määnegi hädä su toimõndamiskõrraga, tuuperäst om viimäne muutminõ egäs johtumisõs jätet tegemäldä. Vaodaq võrgokaeja "tagasi"-nuppi, laadiq üle lehekülg, kost sa tullit ja prooviq vahtsõst.',
'protectlogpage'              => 'Lehti kaitsmiisi nimekiri',
'protectlogtext'              => 'Tan om nimekiri lehti kaitsmiisist ja kaitsmisõ maahavõtmiisist. Parhilla kaitsõ all olõvidõ lehti nimekirä lövvät [[Special:ProtectedPages|tast]].',
'protectedarticle'            => 'pand\' lehe "[[$1]]" kaitsõ ala',
'unprotectedarticle'          => 'võtt\' lehe "[[$1]]" kaitsõ alt maaha',
'protect-title'               => 'Lehe "$1" kaitsminõ',
'protect-legend'              => 'Kinnüdäq kaitsõ ala pandmist',
'protectcomment'              => 'Kaitsõ ala pandmisõ põhjus',
'protectexpiry'               => 'Tähtaig',
'protect_expiry_invalid'      => 'Kõlbmaldaq tähtaig.',
'protect_expiry_old'          => 'Tähtaig om joba läbi.',
'protect-unchain'             => 'Pruugiq tõistõ paika pandmisõ kaidsõt',
'protect-text'                => 'Tan saat kaiaq ja säädäq lehe <strong><nowiki>$1</nowiki></strong> kaitsmist.',
'protect-locked-blocked'      => 'Kinniqpeetült saa-i kaitsmiisi muutaq. Tan ommaq lehe <strong>$1</strong> parhilladsõq säädmiseq:',
'protect-locked-dblock'       => 'Kaitsmiisi saa-i muuta, selle et teedüskogo om lukun. Tan ommaq lehe <strong>$1</strong> parhilladsõq säädmiseq:',
'protect-locked-access'       => 'Sul olõ-i õigust kaitsmiisi muutaq.
Tan ommaq lehe <strong>$1</strong> parhilladsõq säädmiseq:',
'protect-cascadeon'           => 'Taa leht om kaitsõ all, selle tä om pant {{PLURAL:$1|taa kaidsõdu lehe | naidõ kaidsõtuidõ lehti}} pääle. Võit muutaq taa lehe kaitsmiisi, a tä jääs tuugiperäst kaitsõ ala, selle et tä om taan nimekirän.',
'protect-default'             => '(harilik)',
'protect-fallback'            => 'Om vaia "$1"-õigust',
'protect-level-autoconfirmed' => 'Piäq kinniq vahtsõq ja kirjäpandmalda pruukjaq',
'protect-level-sysop'         => 'Õnnõ kõrraldajaq',
'protect-summary-cascade'     => 'laendõt',
'protect-expiring'            => 'tähtaig $1',
'protect-cascade'             => 'Laendaq kaitsmist - võtaq kaitsõ ala kõik seo lehe pääl olõvaq leheq.',
'protect-cantedit'            => 'Sa tohe-i muutaq seo lehe kaitsmistasõt, selle et sul olõ-i õigust seod lehte muutaq.',
'restriction-type'            => 'Luba',
'restriction-level'           => 'Piirdmisastõq',
'minimum-size'                => 'Kõgõ vähämb maht',
'maximum-size'                => 'Kõgõ suurõmb lubat suurus:',
'pagesize'                    => '(baiti)',

# Restrictions (nouns)
'restriction-edit' => 'Toimõndus',
'restriction-move' => 'Tõistõ paika pandminõ',

# Restriction levels
'restriction-level-sysop'         => 'tävveligult kaidsõt',
'restriction-level-autoconfirmed' => 'puulkaidsõt',
'restriction-level-all'           => 'kõik astmõq',

# Undelete
'undelete'                 => 'Tiiq kistutõt lehekülg tagasi',
'undeletepage'             => 'Kistutõduidõ lehekülgi kaeminõ ja tagasitegemine',
'viewdeletedpage'          => 'Kaeq kistutõduid lehti',
'undeletepagetext'         => 'Naaq leheküleq ommaq ärq kistudõduq, a arhiivin
viil olõman, naid saa tagasi tetäq niikavva ku naid olõ-i viil arhiivist ärq visat.',
'undeleteextrahelp'        => 'Võtaq leht tagasi vaotõn nuppi  <b><i>Võtaq tagasi</i></b>. Võit lehe kujjõ valliq ja tagasi võttaq õnnõ nuuq kujoq, miä esiq vällä valit.',
'undeleterevisions'        => '{{PLURAL:$1|Kujo|$1 kujjo}} arhiivi pant.',
'undeletehistory'          => 'Ku tiit leheküle tagasi, tulõvaq kõik kujoq tagasi artikli aolukku. Ku vaihõpääl om luud vahtsõnõ sama nimega lehekülg, ilmusõq tagasitettüq kujoq vanõmba leheküle aoluun. Olõmanolõvat kujjo automaatsõhe vällä ei vaihtõdaq. Teedüstüide kujopiiranguq kaosõq ärq.',
'undeleterevdel'           => 'Kistutõduist tagasituuminõ jätetäs tegemäldäq, ku tuuperäst kistus ärq mõni osa lehe kõgõ vahtsõmbast kujost. Ku om nii, sis tulõ vahtsõmbidõ kistudõduisi kujjõ märgistüs vai käkmine maaha võttaq. Sa saa-i kistutõduist tagasi tetäq ka teddüstükujjõ, midä sul olõ-i õigust nätäq.',
'undeletehistorynoadmin'   => 'Taa leht om ärq kistutõt. Kistutamisõ põhjust näet kokkovõttõn, kost om nätäq ka tuu, kiä ommaq taad lehte toimõndanuq inne kistutamist. Taa lehe sissu saavaq kaiaq õnnõ kõrraldajaq.',
'undelete-revision'        => 'Kistutõt kujo $1 aost $2',
'undeleterevision-missing' => 'Viganõ vai olõmaldaq kujo. Taa või ollaq tagasi tett vai arhiivist ärq kistutõt.',
'undeletebtn'              => 'Tiiq tagasi',
'undeletereset'            => 'Tiiq tühäs',
'undeletecomment'          => 'Kommõntaar:',
'undeletedarticle'         => '"$1" tagasi tett',
'undeletedrevisions'       => '$1 {{PLURAL:$1|kujo|kujjo}} tagasi tett',
'undeletedrevisions-files' => '$1 {{PLURAL:$1|kujo|kujjo}} ja $2 {{PLURAL:$2|teedüstü|teedüstüt}} tagasi tett',
'undeletedfiles'           => '$1 {{PLURAL:$1|teedüstü|teedüstüt}} tagasi tett',
'cannotundelete'           => 'Tagasitegemine lää-s kõrda; kiäki tõõnõ või-ollaq lehe jo tagasi tennüq.',
'undeletedpage'            => "<big>'''$1 om tagasi tett'''</big>

Perämäidsi kistutuisi ja tagasitegemiisi saat kaiaq [[Special:Log/delete|kistutamiisi nimekiräst]].",
'undelete-header'          => 'Perämäidsi kistutuisi saat kaiaq [[Special:Log/delete|kistutamiisi nimekiräst]].',
'undelete-search-box'      => 'Otsiq kistutõduid lehekülgi',
'undelete-search-prefix'   => 'Näütäq lehti, miä nakkasõq pääle:',
'undelete-search-submit'   => 'Otsiq',
'undelete-no-results'      => 'Kistutamiisi nimekiräst lövvetä-s säänest lehte.',

# Namespace form on various pages
'namespace'      => 'Nimeruum:',
'invert'         => 'Näütäq kõiki päält validu nimeruumi',
'blanknamespace' => '(Artikliq)',

# Contributions
'contributions' => 'Pruukja kirotusõq',
'mycontris'     => 'Mu kirotusõq',
'contribsub2'   => 'Pruukja "$1 ($2)" kirotusõq',
'nocontribs'    => 'Sääntsit muutmiisi es lövväq.',
'uctop'         => '(kõgõ vahtsõmb)',
'month'         => 'Alostõn kuust (ja varrampa):',
'year'          => 'Alostõn aastagast (ja varrampa):',

'sp-contributions-newbies'     => 'Näütäq õnnõ vahtsidõ pruukjidõ toimõnduisi',
'sp-contributions-newbies-sub' => 'Vahtsidõ pruukjidõ toimõndusõq',
'sp-contributions-blocklog'    => 'Kinniqpidämisnimekiri',
'sp-contributions-search'      => 'Otsiq muutmiisi',
'sp-contributions-username'    => 'Puutri võrgoaadrõs vai pruukjanimi:',
'sp-contributions-submit'      => 'Otsiq',

# What links here
'whatlinkshere'       => 'Siiäq näütäjäq lingiq',
'whatlinkshere-title' => 'Leheq, miä näütäseq lehe "$1" pääle',
'whatlinkshere-page'  => 'Leht:',
'linklistsub'         => '(Linke nimekiri)',
'linkshere'           => 'Lehe <b>[[:$1]]</b> pääle näütäseq lingiq lehti päält:',
'nolinkshere'         => 'Lehe <b>[[:$1]]</b> pääle näütä-i linke ütegi lehe päält.',
'nolinkshere-ns'      => "Valitun nimeruumin näütä-i ütegi lehe päält linke lehe '''[[:$1]]''' pääle.",
'isredirect'          => 'ümbresaatmislehekülg',
'istemplate'          => 'pruugit näüdüssen',
'whatlinkshere-prev'  => '← {{PLURAL:$1|mineväne leht|$1 mineväst lehte}}',
'whatlinkshere-next'  => '{{PLURAL:$1|mineväne leht|$1 mineväst lehte}} →',
'whatlinkshere-links' => '← lingiq',

# Block/unblock
'blockip'                     => 'Piäq puutri võrgoaadrõs kinniq',
'blockiptext'                 => "Taa vorm om kimmä puutri võrgoaadrõsi päält tettüisi kirotuisi kinniqpidämises. '''Taad tohis tetäq õnnõ lehti ts'urkmisõ vasta ni [[{{MediaWiki:Policy-url}}|{{SITENAME}} sisekõrra perrä]]'''. Kimmähe tulõ täütäq ka rida \"põhjus\". Sinnäq võinuq pandaq nt lingiq noilõ lehile, midä rikuti.",
'ipaddress'                   => 'Puutri võrgoaadrõs (IP)',
'ipadressorusername'          => 'Puutri võrgoaadrõs vai pruukjanimi',
'ipbexpiry'                   => 'Tähtaig',
'ipbreason'                   => 'Põhjus',
'ipbreasonotherlist'          => 'Muu põhjus',
'ipbreason-dropdown'          => "*Hariliguq kinniqpidämise põhjusõq
** Võlss teedüse kirotaminõ
** Lehti sisu ärqkistutaminõ
** Reklaamilinkõ pandminõ
** Mõttõlda jutu vai prahi pandminõ
** Segämine ja ts'urkminõ
** Mitmõ pruukjanime võlsspruukminõ
** Sündümäldäq pruukjanimi",
'ipbanononly'                 => 'Piäq kinniq õnnõ ilma nimeldä pruukjaq',
'ipbcreateaccount'            => 'Lasku-i pruukjanimme luvvaq',
'ipbemailban'                 => 'Lubagu-i pruukjal e-posti saataq',
'ipbenableautoblock'          => 'Piäq kinniq viimäne puutri võrgoaadrõs, kost pruukja om toimõnduisi tennüq, ja edespiten aadrõsiq, kost tä viil pruuv toimõnduisi tetäq.',
'ipbsubmit'                   => 'Piäq taa aadrõs kinniq',
'ipbother'                    => 'Muu tähtaig',
'ipboptions'                  => '15 minotit:15 minutes,1 päiv:1 day,3 päivä:3 days,1 nätäl:1 week,2 nädälit:2 weeks,1 kuu:1 month,3 kuud:3 months,6 kuud:6 months,1 aastak:1 year,igävene:infinite', # display1:time1,display2:time2,...
'ipbotheroption'              => 'Muu tähtaig',
'ipbotherreason'              => 'Muu põhjus',
'ipbhidename'                 => 'Käkiq pruukjanimi vai puutri võrgoaadrõs ärq kinniqpidämis-, toimõndus-, ja pruukjanimekiräst',
'badipaddress'                => 'Puutri võrgoaadrõs om võlssi kirotõt.',
'blockipsuccesssub'           => 'Kinniqpidämine läts kõrda',
'blockipsuccesstext'          => 'Puutri võrgoaadrõs "$1" om kinniq peet.
<br />Kõik parhilladsõq kinniqpidämiseq lövvät [[Special:IPBlockList|kinniqpidämiisi nimekiräst]].',
'ipb-edit-dropdown'           => 'Toimõndaq kinniqpidämise põhjuisi',
'ipb-unblock-addr'            => 'Lõpõdaq pruukja $1 kinniqpidämine ärq',
'ipb-unblock'                 => 'Lõpõdaq pruukja vai puutri võrgoaadrõasi kinniqpidämine ärq',
'ipb-blocklist-addr'          => 'Näütäq pruukja $1 kinniqpidämiisi',
'ipb-blocklist'               => 'Näütäq kinnniqpidämiisi',
'unblockip'                   => 'Lõpõdaq puutri võrgoaadrõsi kinniqpidämine ärq',
'unblockiptext'               => 'Täüdäq ärq taa vorm, et lõpõtaq ärq pruukja vai puutri võrgoaadrõsi kinniqpidämine',
'ipusubmit'                   => 'Lõpõdaq kinniqpidämine ärq',
'unblocked'                   => 'Pruukja [[User:$1|$1]] kinniqpidämine om ärq lõpõtõt',
'unblocked-id'                => '$1 kinniqpidämine võeti maaha',
'ipblocklist'                 => 'Kinniqpeetüisi IP-aadrõssidõ ja pruukjanimmi nimekiri',
'ipblocklist-legend'          => 'Otsiq kinniqpeetüt pruukjat',
'ipblocklist-username'        => 'Pruukjanimi vai puutri võrgoaadrõs:',
'ipblocklist-submit'          => 'Otsiq',
'blocklistline'               => '$1 — $2 om kinniq pidänüq pruukja $3 ($4)',
'infiniteblock'               => 'igäveste',
'expiringblock'               => 'tähtaig om $1',
'anononlyblock'               => 'õnnõ nimeldä pruukjaq',
'noautoblockblock'            => 'automaatsõ kinniqpidämiseldä',
'createaccountblock'          => 'pruukjanime luuminõ kinniq pant',
'emailblock'                  => 'e-post kinniq peet',
'ipblocklist-empty'           => 'Kinniqpidämiisi nimekiri om tühi.',
'ipblocklist-no-results'      => 'Taa puutri võrgoaadrõss vai pruukjanimi olõ-i kinniq peet.',
'blocklink'                   => 'piäq kinniq',
'unblocklink'                 => 'võtaq kinniqpidämine maaha',
'contribslink'                => 'kirotusõq',
'autoblocker'                 => 'Olõt automaatsõhe kinniq peet, selle et jaat puutri võrgoaadrõssit pruukjaga $1. Kinniqpidämise põhjus: $2.',
'blocklogpage'                => 'Kinniqpidämiisi nimekiri',
'blocklogentry'               => 'pidi kinniq pruukja vai puutri võrgoaadrõsi "[[$1]]". Kinniqpidämise tähtaig $2 $3',
'blocklogtext'                => 'Taa om kinniqpidämiisi ja naidõ maahavõtmiisi nimekiri. Automaatsõhe kinniqpeetüisi puutridõ võrgoaadrõssiid tan näüdätä-i, noid kaeq [[Special:IPBlockList|puutridõ võrgoaadrõssidõ kinniqpidämise nimekiräst]].',
'unblocklogentry'             => "lõpõt' pruukja $1 kinniqpidämise ärq",
'block-log-flags-anononly'    => 'õnnõ nimeldä pruukjaq',
'block-log-flags-nocreate'    => 'pruukjanime luuminõ kinniq peet',
'block-log-flags-noautoblock' => 'automaatnõ kinniqpidämine maaha võet',
'block-log-flags-noemail'     => 'e-post kinniq peet',
'range_block_disabled'        => 'Kõrraldaja kinniqpidämisõigusõq olõ-i masma pantuq',
'ipb_expiry_invalid'          => 'Viganõ tähtaig.',
'ipb_already_blocked'         => '"$1" om jo kinniq peet',
'ipb_cant_unblock'            => 'Lövvä-s kinniqpidämist $1. Taa või ollaq jo maaha võet.',
'ip_range_invalid'            => 'Viganõ puutri võrgoaadrõsi kujo.',
'proxyblocker'                => 'Vaihõserveri kinniqpidämine',
'proxyblockreason'            => "Su puutri võrgoaadrõs om kinniq peet, selle et taa om avalik vaihõserver. Otsiq üles uma võrgoliini pakja vai puutrias'atundja ja kõnõlõq näile taast hädäst.",
'proxyblocksuccess'           => 'Valmis.',
'sorbsreason'                 => 'Su puutri võrgoaadrõs om SORBS-i mustan nimekirän ku avalik vaihõserver.',
'sorbs_create_account_reason' => 'Su puutri võrgoaadrõs om pant SORBS-i musta nimekirjä ku avalik vaihõserver. Sa saa-i pruukjanimme tetäq',

# Developer tools
'lockdb'              => 'Panõq teedüskogo lukku',
'unlockdb'            => 'Tiiq teedüskogo lukust vallalõ',
'lockdbtext'          => 'Teedüskogo lukkupandminõ lasõ-i pruukjil lehti ja perräkaemisnimekirjo toimõndaq, säädmiisi vaihtaq ega muid teedüskoko muutvit tallituisi tetäq. Olõq hää ja kinnüdäq, et sa tahat taad tetäq ja et sa lasõt teedüskogo vallalõ, ku olõt umaq tarvilidsõq tallitusõq ärq tennüq.',
'unlockdbtext'        => 'Ku teedüskogo vallalõ laskõq, saavaq pruukjaq lehti ja perräkaemisnimekirjo toimõndaq, vaihtaq säädmiisi ja tetäq muid teedüskoko muutvit tallituisi. Olõq hää ja kinnüdäq, et sa tahat taad tetäq.',
'lockconfirm'         => 'Jah, ma taha tõtõstõ teedüskogo lukku pandaq.',
'unlockconfirm'       => 'Jah, ma taha tõtõstõ teedüskogo lukust vallalõ laskõq.',
'lockbtn'             => 'Panõq teedüskogo lukku',
'unlockbtn'           => 'Lasõq teedüskogo lukust vallalõ',
'locknoconfirm'       => 'Sa olõ-i kinnütüskasti ärq märknüq.',
'lockdbsuccesssub'    => 'Teedüskogo om lukun',
'unlockdbsuccesssub'  => 'Teedüskogo om vallalõ',
'lockdbsuccesstext'   => 'Teedüskogo om noq lukun.
<br />Ku su huuldustüü saa tettüs, sis unõhtagu-i teedüskoko jälq lukust vallalõ laskõq!',
'unlockdbsuccesstext' => 'Teedüskogo om lukust vallalõ last.',
'lockfilenotwritable' => 'Saa-i kirotaq teedüskogo lukkupandmisõ teedüstüt. Kaeq üle, kas sul om tuus õigus.',
'databasenotlocked'   => 'Teedüskoko panda-s lukku.',

# Move page
'move-page-legend'        => 'Nõstaq artikli tõistõ paika',
'movepagetext'            => "Taad vormi pruukin saat lehe ümbre nimetäq. Lehe aolugu pandas kah vahtsõ päälkirä ala.
Vana päälkiräga lehest saa vahtsõ lehe pääle ümbresaatmisõ leht.
Võit säädäq lehe pääle näütäjäq lingiq automaatsõhe näütämä vahtsõ nime pääle.
Ku sa taha-i taad tetäq automaatsõhe, unõhtagu-i üle kaiaq [[Special:DoubleRedirects|katõkõrdsit]] vai [[Special:BrokenRedirects|vigatsit]] ümbresaatmiisi.
Sa piät kaema, et kõik näütämiseq jäänüq tüütämä nigu inne ümbrenimetämist.

Lehte '''nimetedä-i ümbre''', ku vahtsõ nimega leht om jo olõman.
Erängus om tuu, ku vana leht om tühi vai om esiq ümbresaatmisleht ja täl olõ-i toimõndamisaoluku.
Tuu tähendäs, et sa saa-i kogõmalda üle kirotaq jo olõmanolõvat lehte, a saat halvastõ lännü ümbrenimetämise tagasi pöördäq.

'''KAEQ ETTE!'''
Või ollaq, et sa nakkat tegemä suurt ja uutmalda muutmist väega loetavahe artiklihe;
inne, ku midä muudat, märgiq perrä, miä tuust tullaq või.",
'movepagetalktext'        => "Üten artiklilehekülega pandas tõistõ paika ka arotuskülg, '''vällä arvat sis, ku:'''
*panõt lehe ütest nimeruumist tõistõ,
*vahtsõ nime all om jo olõman arotuskülg, kohe om jo midägi kirotõt, vai ku
*jätät alomadsõ kastikõsõ märgistämäldäq.

Kui om nii, sis panõq vana arotuskülg eräle vai panõq taa kokko vahtsõ arotuskülega.",
'movearticle'             => 'Panõq artiklilehekülg tõistõ paika',
'movenotallowed'          => 'Sul olõ-i lupa {{SITENAME}} lehti tõistõ paika nõstaq.',
'newtitle'                => 'Vahtsõ päälkirä ala',
'move-watch'              => 'Kaeq taa lehe perrä',
'movepagebtn'             => 'Panõq artikli tõistõ paika',
'pagemovedsub'            => 'Artikli om tõistõ paika pant',
'movepage-moved'          => "<big>'''$1 om pant nime ala $2'''</big>", # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => 'Sääntse nimega artikli om jo olõman vai olõ-i lubat säänest nimme valliq. Valiq vahtsõnõ nimi.',
'talkexists'              => 'Artikli om tõistõ paika pant, a arotuslehekülge saa-s pandaq, selle et vahtsõ nime all om jo arotuskülg. Panõq arotusküleq esiq kokko.',
'movedto'                 => 'Pant päälkirä ala:',
'movetalk'                => 'Panõq ka "arotus", ku saa.',
'1movedto2'               => "pand' lehe [[$1]] vahtsõ nime [[$2]] ala",
'1movedto2_redir'         => "pand' lehe [[$1]] ümbresaatmislehe [[$2]] pääle",
'movelogpage'             => 'Tõistõ paika pandmiisi nimekiri',
'movelogpagetext'         => 'Taa om lehti tõistõ paika pandmiisi nimekiri.',
'movereason'              => 'Põhjus',
'revertmove'              => 'võtaq tagasi',
'delete_and_move'         => 'Kistudaq tsihtlehekülg ärq ja panõq timä asõmalõ taa leht',
'delete_and_move_text'    => 'Tsihtlehekülg  "[[:$1]]" om jo olõman, kas tahat tuu ärq kistutaq, et taa leht timä asõmalõ pandaq?',
'delete_and_move_confirm' => 'Jah, kistudaq tuu leht ärq',
'delete_and_move_reason'  => 'Ärq kistutõt, et tõõnõ timä asõmalõ pandaq',
'selfmove'                => 'Lätte- ja tsihtnimi ommaq samaq; saa-i lehte timä hindä pääle pandaq.',
'immobile_namespace'      => 'Taaha nimeruumi saa-i lehti pandaq.',

# Export
'export'            => 'Lehti viimine',
'exporttext'        => 'Võit viiäq lehti teksti ja toimõndusaoluu [[Special:Import|üleviimislehe]] kaudu XML-moodun tõistõ MediaWiki kõrra peri tüütäjähe vikihte.

Kirodaq taaha kasti lehti päälkiräq, kost tahat sissu üle viiäq, egä ria pääle üts, ja valiq, kas tahat viiäq lehe kõiki kujjõ vai õnnõ kõgõ vahtsõmbat.

Viimädse johtumisõ kõrral võit ka pruukiq linki, nt leht {{MediaWiki:Mainpage}} saa viidüs lingiga
[[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]].',
'exportcuronly'     => 'Võtku-i kõiki kujjõ, a õnnõ kõgõ vahtsõmb',
'exportnohistory'   => "----
'''Viga:''' Tulõ-i lehti terve aoluu viimisega toimõ.",
'export-submit'     => 'Viiq',
'export-addcattext' => 'Võtaq leheq katõgooriast:',
'export-addcat'     => 'Panõq mano',
'export-download'   => 'Pästäq teedüstüs',

# Namespace 8 related
'allmessages'               => 'Tallitusteedüseq',
'allmessagesname'           => 'Nimi',
'allmessagesdefault'        => 'Vaikimiisi tekst',
'allmessagescurrent'        => 'Parhillanõ tekst',
'allmessagestext'           => 'Taan nimekirän ommaq kõik MediaWiki nimeruumi tallitusteedüseq.
Please visit [http://www.mediawiki.org/wiki/Localisation MediaWiki Localisation] and [http://translatewiki.net Betawiki] if you wish to contribute to the generic MediaWiki localisation.',
'allmessagesnotsupportedDB' => 'Taad lehte saa-i pruukiq, selle et <tt>$wgUseDatabaseMessages</tt>-säädmine om välän.',
'allmessagesfilter'         => 'Teedüsenimmi sõgluminõ:',
'allmessagesmodified'       => 'Näütäq õnnõ muudõtuid',

# Thumbnails
'thumbnail-more'           => 'Suurõndaq',
'filemissing'              => 'Olõ-i teedüstüt',
'thumbnail_error'          => 'Väikupildi luuminõ lää-s kõrda: $1',
'djvu_page_error'          => 'DjVu lehe viga',
'djvu_no_xml'              => 'Saa-s DjVu-teedüstü jaos XML-i kätte',
'thumbnail_invalid_params' => 'Võlss väikupildi parametriq',
'thumbnail_dest_directory' => 'Saa-i tsihtkausta luvvaq',

# Special:Import
'import'                     => 'Tuuq lehti',
'importinterwiki'            => 'Tuuq lehti tõõsõst vikist',
'import-interwiki-text'      => 'Valiq viki ja lehe nimi. Kujjõ kuupääväq ja toimõndajidõ nimeq hoiõtasõq alalõ. Kõik tõisist vikidest tuumisõq pandasõq kirjä [[Special:Log/import|tuumiisi nimekirjä]].',
'import-interwiki-history'   => 'Kopiq lehe terveq aolugu',
'import-interwiki-submit'    => 'Tuuq',
'import-interwiki-namespace' => 'Panõq leheq nimeruumi:',
'importtext'                 => 'Viiq lättevikist lehti [[Special:Export|viimis]]-tüüriistaga. Pästäq teedüs nii uman puutrin ku siin.',
'importstart'                => 'Tuvvas lehti...',
'import-revision-count'      => '$1 {{PLURAL:$1|kujo|kujjo}}',
'importnopages'              => 'Olõ-i lehti, midä tuvvaq.',
'importfailed'               => 'Tuuminõ lää-s kõrda: $1',
'importunknownsource'        => 'Tundmaldaq tuumisõ lättetüüp',
'importcantopen'             => 'Saa-s tuudut teedüstüt vallalõ',
'importbadinterwiki'         => 'Kõlbmalda vikidevaihõlinõ link',
'importnotext'               => 'Tühi vai tekstildä',
'importsuccess'              => "Tuuminõ läts' kõrda!",
'importhistoryconflict'      => 'Lehest om olõman tuuduga vastaolon kujo. Taad lehte või ollaq jo inne tuud.',
'importnosources'            => 'Olõ-i vikidevaihõliidsi tuumislättit ja aoluu õkva pästmine tüütä-i.',
'importnofile'               => 'Olõ-i üttegi tuudut teedüstüt.',
'importuploaderrorsize'      => 'Teedüstü üleslaatminõ lää-s kõrda, taa om suurõmb, ku lubat.',
'importuploaderrorpartial'   => 'Teedüstü üleslaatminõ lää-s kõrda. Õnnõ osa teedüstüst laaditi üles.',
'importuploaderrortemp'      => 'Teedüstü üleslaatminõ lää-s kõrda. Olõ-i aotlist kausta.',

# Import log
'importlogpage'                    => 'Tuumiisi nimekiri',
'importlogpagetext'                => 'Tõisist vikidest tuuduisi lehti nimekiri.',
'import-logentry-upload'           => 'tõi lehe [[$1]] saatõn teedüstü',
'import-logentry-upload-detail'    => '$1 {{PLURAL:$1|kujo|kujjo}}',
'import-logentry-interwiki'        => 'tõi tõõsõst vikist lehe ”$1”',
'import-logentry-interwiki-detail' => '$1 {{PLURAL:$1|kujo|kujjo}} lehest $2',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'Mu pruukjaleht',
'tooltip-pt-anonuserpage'         => 'Su puutri võrgoaadrõsi pruukjaleht',
'tooltip-pt-mytalk'               => 'Mu arotuskülg',
'tooltip-pt-anontalk'             => 'Arotus taa puutri võrgoaadrõsi päält tettüisi toimõnduisi üle',
'tooltip-pt-preferences'          => 'Mu säädmiseq',
'tooltip-pt-watchlist'            => 'Nimekiri lehist, mil tahtnuq silmä pääl hoitaq',
'tooltip-pt-mycontris'            => 'Mu ummi toimõnduisi nimekiri',
'tooltip-pt-login'                => 'Mineq nimega sisse vai tiiq hindäle pruukjanimi (soovitav).',
'tooltip-pt-anonlogin'            => 'Mineq nimega sisse vai tiiq hindäle pruukjanimi (soovitav).',
'tooltip-pt-logout'               => 'Mineq nime alt vällä',
'tooltip-ca-talk'                 => 'Arotus lehe sisu üle',
'tooltip-ca-edit'                 => 'Saa võit taad lehte toimõndaq.',
'tooltip-ca-addsection'           => 'Jätäq taalõ lehele kommõntaar.',
'tooltip-ca-viewsource'           => 'Taa om kaidsõt leht. Saat kaiaq õnnõ taa lättekuudi.',
'tooltip-ca-history'              => 'Taa lehe vanõmbaq kujoq.',
'tooltip-ca-protect'              => 'Võtaq taa leht kaitsõ ala',
'tooltip-ca-delete'               => 'Kistudaq taa leht ärq',
'tooltip-ca-undelete'             => 'Tuuq taa leht kistutõduist tagasi',
'tooltip-ca-move'                 => 'Panõq taa leht tõistõ paika',
'tooltip-ca-watch'                => 'Panõq taa leht umma perräkaemisnimekirjä',
'tooltip-ca-unwatch'              => 'Võtaq taa leht perräkaemisnimekiräst maaha',
'tooltip-search'                  => 'Otsiq vikist {{SITENAME}}',
'tooltip-search-go'               => 'Mineq täpsähe sääntse nimega lehe pääle, ku sääne olõman om',
'tooltip-search-fulltext'         => 'Otsiq lehti päält seod teksti',
'tooltip-p-logo'                  => 'Pääleht',
'tooltip-n-mainpage'              => 'Mineq päälehele',
'tooltip-n-portal'                => 'Taa viki arotusõkotus',
'tooltip-n-currentevents'         => 'Tiidmist tuu kotsilõ, miä parhilla sünnüs',
'tooltip-n-recentchanges'         => 'Perämäidsi muutmiisi nimekiri',
'tooltip-n-randompage'            => 'Tiiq vallalõ johuslinõ lehekülg',
'tooltip-n-help'                  => 'Abiotsmisõ kotus',
'tooltip-t-whatlinkshere'         => 'Siiäq näütäjide linkega lehti nimekiri',
'tooltip-t-recentchangeslinked'   => 'Viimädseq muutmisõq lehile, mink pääle näüdätäs linkega seo lehe päält',
'tooltip-feed-rss'                => 'Taa lehe RSS-kujo',
'tooltip-feed-atom'               => 'Taa lehe Atom-kujo',
'tooltip-t-contributions'         => 'Näütäq taa pruukja toimõnduisi nimekirjä',
'tooltip-t-emailuser'             => 'Saadaq taalõ pruukjalõ e-kiri',
'tooltip-t-upload'                => 'Laadiq üles teedüstüid',
'tooltip-t-specialpages'          => 'Näütäq tallituslehekülgi',
'tooltip-t-print'                 => 'Taa lehe trükükujo',
'tooltip-t-permalink'             => 'Seo lehekujo püsülink',
'tooltip-ca-nstab-main'           => 'Näütäq sisulehekülge',
'tooltip-ca-nstab-user'           => 'Näütäq pruukjalehekülge',
'tooltip-ca-nstab-media'          => 'Näütäq meediälehekülge',
'tooltip-ca-nstab-special'        => 'Taa om tallituslehekülg',
'tooltip-ca-nstab-project'        => 'Näütäq projektilehekülge',
'tooltip-ca-nstab-image'          => 'Näütäq teedüstü lehekülge',
'tooltip-ca-nstab-mediawiki'      => 'Näütäq tallitusteedüst',
'tooltip-ca-nstab-template'       => 'Näütäq näüdüst',
'tooltip-ca-nstab-help'           => 'Näütäq abilehekülge',
'tooltip-ca-nstab-category'       => 'Näütäq katõgoorialehekülge',
'tooltip-minoredit'               => "Märgiq taa ärq ku tsill'okõnõ muutminõ",
'tooltip-save'                    => 'Pästäq muutmisõq',
'tooltip-preview'                 => 'Kaeq umaq toimõndusõq inne pästmist üle!',
'tooltip-diff'                    => 'Näütäq tettüid muutmiisi',
'tooltip-compareselectedversions' => 'Näütäq seo lehe valituidõ kuiõ lahkominekit.',
'tooltip-watch'                   => 'Panõq taa leht umma perräkaemisnimekirjä',
'tooltip-recreate'                => 'Tuuq taa leht kisutõduist tagasi',
'tooltip-upload'                  => 'Nakkaq üles laatma',

# Stylesheets
'common.css'   => '/* Taa lehe pääl om tervet taad vikit muutvit kujonduisi */',
'monobook.css' => '/* Taa lehe pääl om Monobook-vällänägemist muutvit kujonduisi. */',

# Scripts
'common.js'   => '/* Taa lehe kuud pandas mano egäle lehelaatmisõlõ */',
'monobook.js' => '/* Olõi soovitõt; pruugiq [[MediaWiki:common.js]] */',

# Metadata
'nodublincore'      => 'Taan serverin olõ-i Dublin Core RDF-metateedüst tüüle pant.',
'nocreativecommons' => 'Taan serverin olõ-i Creative Commonsi RDF-metateedüst tüüle pant.',
'notacceptable'     => 'Wikiserver saa-i näüdädäq teedüst sääntsen moodun, midä su programm saasiq lukõq.',

# Attribution
'anonymous'        => '{{SITENAME}} nimeldäq pruukjaq',
'siteuser'         => '{{SITENAME}} pruukja $1',
'lastmodifiedatby' => "Taad lehte toimõnd' viimäte ”$3” $2 kell $1.", # $1 date, $2 time, $3 user
'othercontribs'    => 'Tennüq pruukja $1.',
'others'           => 'tõõsõq',
'siteusers'        => '{{SITENAME}} pruukja(q) $1',
'creditspage'      => 'Lehe tegijide nimekiri',
'nocredits'        => 'Taa lehe tegijide nimekirjä olõ-i.',

# Spam protection
'spamprotectiontitle' => 'Prahisõgõl',
'spamprotectiontext'  => 'Prahisõgõl om lehe kinniq pidänüq ja lasõ-i taad pästäq. Tuu põhjus om arvadaq vikist välläpoolõ näütäjä link.',
'spamprotectionmatch' => 'Tekst, midä prahisõgõl läbi lasõ-s: $1',
'spambot_username'    => 'MediaWiki prahihäötäjä',
'spam_reverting'      => 'Tagasi pööret viimädse kujo pääle, koh olõ-i linke lehele $1',
'spam_blanking'       => "Kõigin kujõn oll' linke lehele $1. Leht tühäs tett.",

# Info page
'infosubtitle'   => 'Teedüs lehe kotsilõ',
'numedits'       => 'Lehele tettüid toimõnduisi: $1',
'numtalkedits'   => 'Arotuskülele tettüid toimõnduisi: $1',
'numwatchers'    => 'Perräkaejit: $1',
'numauthors'     => 'Lehele eräle kirotajit: $1',
'numtalkauthors' => 'Arotuskülele eräle kirotajit: $1',

# Math options
'mw_math_png'    => 'Kõgõ PNG',
'mw_math_simple' => 'Ku väega lihtsä, sis HTML, muido PNG',
'mw_math_html'   => 'Ku saa, sis HTML, muido PNG',
'mw_math_source' => 'Alalõ hoitaq TeX (tekstikaejin)',
'mw_math_modern' => 'Vahtsõmbilõ võrgokaejilõ soovitõt',

# Patrolling
'markaspatrolleddiff'                 => 'Märgiq ülekaetus',
'markaspatrolledtext'                 => 'Märgiq toimõndus ülekaetus',
'markedaspatrolled'                   => 'Märgit ülekaetus',
'markedaspatrolledtext'               => 'Valit kujo om üle kaet.',
'rcpatroldisabled'                    => 'Vahtsidõ muutmiisi ülekaemist olõ-i tüüle säet.',
'rcpatroldisabledtext'                => 'Vahtsidõ muutmiisi ülekaemist olõ-i tüüle säet.',
'markedaspatrollederror'              => 'Muutuisi ülekaetus märkmine lää-s kõrda',
'markedaspatrollederrortext'          => 'Olõ-i ant lehe muutmiskujjo, midä ülekaetus märkiq.',
'markedaspatrollederror-noautopatrol' => 'Esiq tohe-i ummi muutmiisi ülekaetus märkiq.',

# Patrol log
'patrol-log-page' => 'Muutmiisi ülekaemiisi nimekiri',
'patrol-log-line' => 'märke lehe $2 muutmisõ $1 ülekaetus $3',
'patrol-log-auto' => '(automaatnõ)',

# Image deletion
'deletedrevision'    => 'Kistutõdi ärq vana kujo $1.',
'filedelete-missing' => 'Teedüstüt "$1" saa-i kistutaq, taad olõ-i olõman.',

# Browsing diffs
'previousdiff' => '← Innembäne muutminõ',
'nextdiff'     => 'Järgmäne muutminõ →',

# Media information
'mediawarning'         => "'''Kaeq ette''': Taan teedüstün või ollaq sisen ohtlik kuud, miä või su programmilõ vika tetäq.<hr />",
'imagemaxsize'         => 'Pildi seletüslehe pääl näütämise suuruspiir:',
'thumbsize'            => 'Väikupildi suurus:',
'file-info'            => '$1, MIME-tüüp: $2',
'file-info-size'       => '($1×$2 pikslit, $3, MIME-tüüp: $4)',
'file-nohires'         => '<small>Taast terävämpä pilti olõ-i saiaq.</small>',
'svg-long-desc'        => '(SVG-teedüstü, põhisuurus $1 × $2 pikslit, teedüstü suurus $3)',
'show-big-image'       => 'Täüsterräv kujo',
'show-big-image-thumb' => '<small>Proovikaemisõ suurus: $1×$2 pikslit</small>',

# Special:NewImages
'newimages'             => 'Vahtsõq pildiq',
'imagelisttext'         => 'Pilte nimekirän $1 (sordiduq $2).',
'showhidebots'          => '($1 robodiq)',
'noimages'              => 'Olõ-i vahtsit pilte.',
'ilsubmit'              => 'Otsminõ',
'bydate'                => 'kuupäävä perrä',
'sp-newimages-showfrom' => 'Näütäq vahtsit pilte kuupääväst $1 pääle',

# Bad image list
'bad_image_list' => 'Nimekirä muud om sääne:

Õnnõ *-märgiga algajaq riaq võetasõq arvõhe. Edimäne link piät näütämä keeledü teedüstü pääle. Kõik tõõsõq lingiq ommaq eränguq, tuu tähendäs leheq, kon pilti või pruukiq.',

# Metadata
'metadata'          => 'Sisuseletüseq',
'metadata-help'     => 'Seon teedüstün om lisateedüst, miä om arvadaq peri pildinudsijast, digikaamõrast vai pilditoimõndusprogrammist. Ku teedüstüt om peräst timä tegemist muudõt, sis pruugi-i taa teedüs inämb õigõ ollaq.',
'metadata-expand'   => 'Näütäq kõiki sisuseletüisi',
'metadata-collapse' => 'Näütäq õnnõ tähtsämbit sisuseletüisi',
'metadata-fields'   => 'Naaq riaq ommaq nätäq pildilehe pääl, ku sisuseletüse tapõl om tühi.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => 'Lakjus',
'exif-imagelength'                 => 'Korgus',
'exif-bitspersample'               => 'Bitti osa kotsilõ',
'exif-compression'                 => 'Kokkopakmisviis',
'exif-photometricinterpretation'   => 'Pildipunktõ ülesehitüs',
'exif-orientation'                 => 'Tsiht',
'exif-samplesperpixel'             => 'Ossõ arv',
'exif-planarconfiguration'         => 'Teedüse kõrraldaminõ',
'exif-ycbcrsubsampling'            => 'Y ja C alanäütüsvaihõkõrd',
'exif-ycbcrpositioning'            => 'Y ja C paikasäädmine',
'exif-xresolution'                 => 'Pildi terävüs lajoldõ',
'exif-yresolution'                 => 'Pildi terävüs pikuldõ',
'exif-resolutionunit'              => 'Terävusosa X- ja Y-tsihin',
'exif-stripoffsets'                => 'Pilditeedüse kotus',
'exif-rowsperstrip'                => 'Riban rivve',
'exif-stripbytecounts'             => 'Baitõ kokkopakitun riban',
'exif-jpeginterchangeformat'       => 'Kavvus JPEG SOI-st',
'exif-jpeginterchangeformatlength' => 'JPEG-teedüssen baitõ',
'exif-transferfunction'            => 'Ülekandõfunktsiuun',
'exif-whitepoint'                  => 'Valgõ punkti värmiarv',
'exif-primarychromaticities'       => 'Päävärme värmiarvoq',
'exif-ycbcrcoefficients'           => 'Värmiruumi tõõsõndusmaatriksi elemendiq',
'exif-referenceblackwhite'         => 'Musta-valgõpaari võrrõlusarvoq',
'exif-datetime'                    => 'Viimäte muudõt',
'exif-imagedescription'            => 'Pildiallkiri',
'exif-make'                        => 'Kaamõra tekij',
'exif-model'                       => 'Kaamõra mutõl',
'exif-software'                    => 'Pruugit tarkvara',
'exif-artist'                      => 'Tekij',
'exif-copyright'                   => 'Tegijäõigusõ umanik',
'exif-exifversion'                 => 'Exif-kujo',
'exif-flashpixversion'             => 'Toet Flashpix-kujo',
'exif-colorspace'                  => 'Värmiruum',
'exif-componentsconfiguration'     => 'Egä osa tähendüs',
'exif-compressedbitsperpixel'      => 'Pildi kokkopakmismuud',
'exif-pixelydimension'             => 'Kõlbolinõ pildi lakjus',
'exif-pixelxdimension'             => 'Kõlbolinõ pildi korgus',
'exif-makernote'                   => 'Tegijä seletüseq',
'exif-usercomment'                 => 'Pruukja kommõntaariq',
'exif-relatedsoundfile'            => 'Manopant helüteedüstü',
'exif-datetimeoriginal'            => 'Luumisaig',
'exif-datetimedigitized'           => 'Digitalisiirmisaig',
'exif-subsectime'                  => 'Ao sekundiosaq',
'exif-subsectimeoriginal'          => 'Edimält olnuq ao sekundiosaq',
'exif-subsectimedigitized'         => 'Digitalisiirmisao sekundiosaq',
'exif-exposuretime'                => 'Valgustusaig',
'exif-exposuretime-format'         => '$1 sek ($2)',
'exif-fnumber'                     => 'Mulguvaihõkõrd',
'exif-exposureprogram'             => 'Valgustusprogramm',
'exif-spectralsensitivity'         => 'Spektri herküs',
'exif-isospeedratings'             => 'Herküs (ISO)',
'exif-oecf'                        => 'Optoelektroonilinõ muutumiskõrdaja',
'exif-shutterspeedvalue'           => 'Katigu kibõhus',
'exif-aperturevalue'               => 'Läbilaskmismulk',
'exif-brightnessvalue'             => 'Helehüs',
'exif-exposurebiasvalue'           => 'Valgustusõ parandus',
'exif-maxaperturevalue'            => 'Kõgõ suurõmb läbilaskmismulk',
'exif-subjectdistance'             => 'Tsihtmärgi kavvus',
'exif-meteringmode'                => 'Mõõtmisviis',
'exif-lightsource'                 => 'Valgusläteq',
'exif-flash'                       => 'Välk',
'exif-focallength'                 => 'Läädse palotuslakjus',
'exif-subjectarea'                 => 'Tsihtmärgi ala',
'exif-flashenergy'                 => 'Välgü vägi',
'exif-spatialfrequencyresponse'    => 'Ruumifrekvendsi vastõq',
'exif-focalplanexresolution'       => 'Täpsüstüsastmõ X-resolutsiuun',
'exif-focalplaneyresolution'       => 'Täpsüstüstasõmõ Y-resolutsiuun',
'exif-focalplaneresolutionunit'    => 'Täpsüstüstasõmõ resolutsiooni mõõt',
'exif-subjectlocation'             => 'Tsihtmärgi kotus',
'exif-exposureindex'               => 'Valgustusindeks',
'exif-sensingmethod'               => 'Mõõtmisviis',
'exif-filesource'                  => 'Teedüstüläteq',
'exif-scenetype'                   => 'Pilditüüp',
'exif-cfapattern'                  => 'CFA-kujond',
'exif-customrendered'              => 'Hindäperi pilditoimõndus',
'exif-exposuremode'                => 'Valgustusviis',
'exif-whitebalance'                => 'Valgõ tasakaal',
'exif-digitalzoomratio'            => 'Digitaalnõ suurõnduskõrdaja',
'exif-focallengthin35mmfilm'       => '35 mm-dse filmi palotusvaheq',
'exif-scenecapturetype'            => 'Pildi sissevõtmisviis',
'exif-gaincontrol'                 => 'Pildi säädmine',
'exif-contrast'                    => 'Kontrast',
'exif-saturation'                  => 'Värmikülläsüs',
'exif-sharpness'                   => 'Terävüs',
'exif-devicesettingdescription'    => 'Kaamõra säädmiisi seletüs',
'exif-subjectdistancerange'        => 'Tsihtmärgi kavvusvaih',
'exif-imageuniqueid'               => 'Pildi tunnusnummõr',
'exif-gpsversionid'                => 'GPS-koodi kujo',
'exif-gpslatituderef'              => "Põh'a- vai lõunalakjus",
'exif-gpslatitude'                 => 'Lakjus',
'exif-gpslongituderef'             => 'Hummogu- vai õdagupikkus',
'exif-gpslongitude'                => 'Pikkus',
'exif-gpsaltituderef'              => 'Korgusõ võrrõluspunkt',
'exif-gpsaltitude'                 => 'Korgus',
'exif-gpstimestamp'                => 'GPS-aig (aatomikell)',
'exif-gpssatellites'               => 'Mõõtmisõs pruugiduq satõlliidiq',
'exif-gpsstatus'                   => 'Vastavõtja sais',
'exif-gpsmeasuremode'              => 'Mõõtmisviis',
'exif-gpsdop'                      => 'Mõõtmistäpsüs',
'exif-gpsspeedref'                 => 'Kibõhusmõõt',
'exif-gpsspeed'                    => 'GPS-vastavõtja kibõhus',
'exif-gpstrackref'                 => 'Liikmistsihi võrrõluspunkt',
'exif-gpstrack'                    => 'Liikmistsiht',
'exif-gpsimgdirectionref'          => 'Pildi tsihi võrrõluspunkt',
'exif-gpsimgdirection'             => 'Pildi tsiht',
'exif-gpsmapdatum'                 => 'Pruugit geodeetiline maamõõtmisteedüs',
'exif-gpsdestlatituderef'          => 'Tsihtmärgi lakjusõ võrrõluspunkt',
'exif-gpsdestlatitude'             => 'Tsihtmärgi lakjus',
'exif-gpsdestlongituderef'         => 'Tsihtmärgi pikkusõ võrrõluspunkt',
'exif-gpsdestlongitude'            => 'Tsihtmärgi pikkus',
'exif-gpsdestbearingref'           => 'Tsihtmärgi vällätimmise võrrõluspunkt',
'exif-gpsdestbearing'              => 'Tsihtmärgi vällätimmine',
'exif-gpsdestdistanceref'          => 'Tsihtmärgi kavvusõ võrrõluspunkt',
'exif-gpsdestdistance'             => 'Tsihtmärgi kavvus',
'exif-gpsprocessingmethod'         => 'GPS-i tüümoodu nimi',
'exif-gpsareainformation'          => 'GPS-ala nimi',
'exif-gpsdatestamp'                => 'GPS-kuupäiv',
'exif-gpsdifferential'             => 'GPS-differentsiaalparandus',

# EXIF attributes
'exif-compression-1' => 'Kokkopakmalda',

'exif-unknowndate' => 'Tundmalda kuupäiv',

'exif-orientation-1' => 'Harilik', # 0th row: top; 0th column: left
'exif-orientation-2' => 'Pikäle käänet', # 0th row: top; 0th column: right
'exif-orientation-3' => '180° käänet', # 0th row: bottom; 0th column: right
'exif-orientation-4' => 'Pistü käänet', # 0th row: bottom; 0th column: left
'exif-orientation-5' => 'Käänet 90° vastapäivä ja pistü', # 0th row: left; 0th column: top
'exif-orientation-6' => 'Käänet 90° peripäivä', # 0th row: right; 0th column: top
'exif-orientation-7' => 'Käänet 90° peripäivä ja pistü', # 0th row: right; 0th column: bottom
'exif-orientation-8' => 'Käänet 90° vastapäivä', # 0th row: left; 0th column: bottom

'exif-planarconfiguration-1' => "''chunky''-formaat",
'exif-planarconfiguration-2' => "''planar''-formaat",

'exif-componentsconfiguration-0' => 'olõ-i',

'exif-exposureprogram-0' => 'Olõ-i paika säet',
'exif-exposureprogram-1' => 'Käsilde paikasäet',
'exif-exposureprogram-2' => 'Põhiprogramm',
'exif-exposureprogram-3' => 'Läbilaskmismulgu põhilisus',
'exif-exposureprogram-4' => 'Katiguao põhilisus',
'exif-exposureprogram-5' => 'Luuva programm (suurõndõt süvvüsterävüst)',
'exif-exposureprogram-6' => 'Liikmisprogramm (suurõndõt katiguao kibõhust)',
'exif-exposureprogram-7' => 'Rinnapildimuud (lähipildele, kon tagapõhi om hägonõ)',
'exif-exposureprogram-8' => 'Maastigumuud (maastigupildele, kon tagapõhi om selge)',

'exif-subjectdistance-value' => '$1 miitrit',

'exif-meteringmode-0'   => 'Tiidmäldä',
'exif-meteringmode-1'   => 'Keskmäne',
'exif-meteringmode-2'   => 'Keskkotusõperine keskmäne',
'exif-meteringmode-3'   => 'Täpp',
'exif-meteringmode-4'   => 'Mitmõtäpiline',
'exif-meteringmode-5'   => 'Kujond',
'exif-meteringmode-6'   => 'Osalinõ',
'exif-meteringmode-255' => 'Muu',

'exif-lightsource-0'   => 'Tiidmäldä',
'exif-lightsource-1'   => 'Päävävalgus',
'exif-lightsource-2'   => 'Päävävalguslamp',
'exif-lightsource-3'   => 'Hõõglamp (kunstvalgus)',
'exif-lightsource-4'   => 'Välk',
'exif-lightsource-9'   => 'Selge ilm',
'exif-lightsource-10'  => 'Pilvine ilm',
'exif-lightsource-11'  => 'Vari',
'exif-lightsource-12'  => 'Päävävalguslamp (D 5700 – 7100K)',
'exif-lightsource-13'  => 'Päävävalguslamp (N 4600 – 5400K)',
'exif-lightsource-14'  => 'Külmvalgõ päävävalguslamp (W 3900 – 4500K)',
'exif-lightsource-15'  => 'Valgõ päävävalguslamp (WW 3200 – 3700K)',
'exif-lightsource-17'  => 'Standardvalgus A',
'exif-lightsource-18'  => 'Standardvalgus B',
'exif-lightsource-19'  => 'Standardvalgus C',
'exif-lightsource-24'  => 'ISO stuudiohõõglamp',
'exif-lightsource-255' => 'Muu valgus',

'exif-focalplaneresolutionunit-2' => 'tolli',

'exif-sensingmethod-1' => 'Paikasäädmäldä',
'exif-sensingmethod-2' => 'Ütene värmisensor',
'exif-sensingmethod-3' => 'Katõnõ värmisensor',
'exif-sensingmethod-4' => 'Kolmõnõ värmisensor',
'exif-sensingmethod-5' => 'Sarivärmisensor',
'exif-sensingmethod-7' => 'Trilineaarsensor',
'exif-sensingmethod-8' => 'Sarilineaarsensor',

'exif-scenetype-1' => 'Õkva pildistet pilt',

'exif-customrendered-0' => 'Harilik tallitus',
'exif-customrendered-1' => 'Hindäsäet tallitus',

'exif-exposuremode-0' => 'Automaatnõ valgustus',
'exif-exposuremode-1' => 'Hindäsäet valgustus',
'exif-exposuremode-2' => 'Automaatnõ haardminõ',

'exif-whitebalance-0' => 'Automaatnõ valgõ tasakaal',
'exif-whitebalance-1' => 'Hindäsäet valgõ tasakaal',

'exif-scenecapturetype-0' => 'Harilik',
'exif-scenecapturetype-1' => 'Maastik',
'exif-scenecapturetype-2' => 'Rinnapilt',
'exif-scenecapturetype-3' => 'Üüpilt',

'exif-gaincontrol-0' => 'Olõ-i',
'exif-gaincontrol-1' => 'Matal üläkinnütüs',
'exif-gaincontrol-2' => 'Korgõ üläkinnütüs',
'exif-gaincontrol-3' => 'Matal alakinnütüs',
'exif-gaincontrol-4' => 'Korgõ alakinnütüs',

'exif-contrast-0' => 'Harilik',
'exif-contrast-1' => 'Pehmeq',
'exif-contrast-2' => 'Kõva',

'exif-saturation-0' => 'Harilik',
'exif-saturation-1' => 'Väiku värmikülläsüs',
'exif-saturation-2' => 'Suur värmikülläsüs',

'exif-sharpness-0' => 'Harilik',
'exif-sharpness-1' => 'Pehmeq',
'exif-sharpness-2' => 'Kõva',

'exif-subjectdistancerange-0' => 'Tiidmäldäq',
'exif-subjectdistancerange-1' => 'Makro',
'exif-subjectdistancerange-2' => 'Lähküpilt',
'exif-subjectdistancerange-3' => 'Kavvõpilt',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => "Põh'lakjust",
'exif-gpslatitude-s' => 'Lõunalakjust',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'Hummogupikkust',
'exif-gpslongitude-w' => 'Õdagupikkust',

'exif-gpsstatus-a' => 'Mõõtminõ käü',
'exif-gpsstatus-v' => 'Ristimõõtminõ',

'exif-gpsmeasuremode-2' => 'Katõmõõtmõlinõ mõõtminõ',
'exif-gpsmeasuremode-3' => 'Kolmõmõõtmõlinõ mõõtminõ',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'kilomiitrit tunnin',
'exif-gpsspeed-m' => 'miili tunnin',
'exif-gpsspeed-n' => 'sõlmõ',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => 'Peris tsiht',
'exif-gpsdirection-m' => 'Magnõttsiht',

# External editor support
'edit-externally'      => 'Toimõndaq taad teedüstüt välidse programmiga',
'edit-externally-help' => 'Lisateedüst: [http://www.mediawiki.org/wiki/Manual:External_editors kaeq siist].',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'kõik',
'imagelistall'     => 'kõik',
'watchlistall2'    => ', terveq aolugu',
'namespacesall'    => 'kõik',
'monthsall'        => 'kõik',

# E-mail address confirmation
'confirmemail'            => 'Kinnüdäq e-postiaadrõssit',
'confirmemail_noemail'    => 'Sul olõ-i [[Special:Preferences|ummi säädmiisihe]] pant kõlbolist e-postiaadrõssit.',
'confirmemail_text'       => 'Taa viki nõud e-postiaadrõsi kinnütämist, inne ku e-posti pruukiq võit. Saadaq alanolõva nupi pääle vaotõn uma aadrõsi pääle kinnütüse küsümise kiri. Säält lövvät lingi, mink vaotamisõga kinnütät uma e-postiaadrõsi.',
'confirmemail_pending'    => '<div class="error">Kinnütüskiri om jo ärq saadõt. Ku lõit õkva vahtsõ pruukjanime, oodaq mõni minot sõnomi tulõkit, inne ku proovit vahtsõst.</div>',
'confirmemail_send'       => 'Saadaq kinnütüskiri ärq',
'confirmemail_sent'       => 'Kinnütüskiri ärq saadõt.',
'confirmemail_oncreate'   => 'Kinnütüskiri saadõti su e-postiaadrõsi pääle. Kinnütüskuudi olõ-i joht vajja nimega sisseminekis, a tuu tulõ sul ärq saataq, ku tahat, et sa saanuq taan vikin e-posti saataq.',
'confirmemail_sendfailed' => 'Kinnütüskiri jäi saatmalda. Kaeq, kas su annõtun aadrõssin olõ-i keeletüid märke. Postiprogramm saat tagasi: $1',
'confirmemail_invalid'    => 'Kõlbmalda kinnütüskuud. Taa või ollaq vanaslännüq.',
'confirmemail_needlogin'  => 'Uma e-postiaadrõsi kinnütämises $1.',
'confirmemail_success'    => 'Su e-postiaadrõs om no ärq kinnütet. Võit nimega sisse minnäq.',
'confirmemail_loggedin'   => 'Su e-postiaadrõs om no ärq kinnütet.',
'confirmemail_error'      => "Su e-postiaadrõsi kinnütämisega läts' midägi võlssi.",
'confirmemail_subject'    => '{{SITENAME}} e-postiaadrõsi kinnütämine',
'confirmemail_body'       => 'Kiäki, arvadaq saq esiq, lõi puutri võrgoaadrõsi $1 päält {{SITENAME}} pruukjanime $2. Ku taa om tõtõstõ suq pruukjanimi, tiiq vallalõ link: $3. Ku taa *olõ-i* suq luud pruukjanimi, sis teku-i midägi. Kinnütüskuud lätt vanas $4.',

# Scary transclusion
'scarytranscludedisabled' => '[Vikidevaihõlinõ teedüsepruukminõ olõ-i käügin]',
'scarytranscludefailed'   => '[Saa-s näüdüst kätte: $1]',
'scarytranscludetoolong'  => "[Võrgoaadrõs om pall'o pikk]",

# Trackbacks
'trackbackbox'      => "<div id=\"mw_trackbacks\">Artikli pääle pantuisi linke näütämine (''trackbackiq''):<br />\$1</div>",
'trackbackremove'   => ' ([$1 kistutus])',
'trackbacklink'     => "Artikli pääle pantuisi linke näütämine (''trackback'')",
'trackbackdeleteok' => "Artikli pääle pantuisi linke näütämine (''trackback'') kistutõdi ärq.",

# Delete conflict
'deletedwhileediting' => "<center>'''Hoiatus''': taa leht om ärq kistutõt päält tuud, ku sa taad toimõndama naksit!</center>",
'confirmrecreate'     => "Pruukja '''[[User:$1|$1]]''' ([[User talk:$1|arotus]]) kistut' taa lehe ärq päält tuud, ku sa naksit taad toimõndama. Põhjus oll':
: ''$2''
Olõq hää, kinnüdäq, et tahat taad lehte vahtsõst luvvaq.",
'recreate'            => 'Luuq vahtsõst',

# HTML dump
'redirectingto' => 'Saadõtas ümbre lehe pääle [[:$1]]...',

# action=purge
'confirm_purge'        => 'Kas taa lehe vaihõmälokujoq tulõvaq ärq kistutaq?

$1',
'confirm_purge_button' => 'Hää külh',

# AJAX search
'searchcontaining' => "Otsiq artikliid, kon om seen ''$1''.",
'searchnamed'      => "Otsiq artikliid nimega ''$1''.",
'articletitles'    => "Artikliq, miä nakkasõq pääle ''$1''",
'hideresults'      => 'Käkiq tulõmusõq ärq',

# Multipage image navigation
'imgmultipageprev' => '← mineväne leht',
'imgmultipagenext' => 'järgmäne leht →',
'imgmultigo'       => 'Mineq!',

# Table pager
'ascending_abbrev'         => 'ülespoolõ',
'descending_abbrev'        => 'allapoolõ',
'table_pager_next'         => 'Järgmäne leht',
'table_pager_prev'         => 'Mineväne leht',
'table_pager_first'        => 'Edimäne leht',
'table_pager_last'         => 'Perämäne leht',
'table_pager_limit'        => 'Näütäq $1 ütsüst lehe kotsilõ',
'table_pager_limit_submit' => 'Mineq',
'table_pager_empty'        => 'Olõ-i tulõmuisi',

# Auto-summaries
'autosumm-blank'   => 'Leht tetti tühäs',
'autosumm-replace' => "Asõmalõ panti '$1'",
'autoredircomment' => 'Ümbresaatminõ lehele [[$1]]',
'autosumm-new'     => 'Vahtsõnõ leht: $1',

# Live preview
'livepreview-loading' => 'Laat…',
'livepreview-ready'   => 'Laat… Valmis!',
'livepreview-failed'  => 'Kipõkaehus lää-s käümä!
Prooviq harilikku kaehust.',
'livepreview-error'   => 'Ütistämine lää-s kõrda: $1 "$2"
Prooviq harilikku kaehust.',

# Friendlier slave lag warnings
'lag-warn-normal' => 'Muutmiisi, miä ommaq vahtsõmbaq ku $1 sekundit, pruugi-i taan nimekirän nätäq ollaq.',
'lag-warn-high'   => 'Teedüskogoserveri aiglusõ peräst pruugi-i $1 sekundist värskimbit muutmiisi nimekirän nätäq ollaq.',

# Watchlist editor
'watchlistedit-numitems'      => 'Su perräkaemisnimekirän om {{PLURAL:$1|1 päälkiri|$1 päälkirjä}}, arotusleheq vällä arvaduq.',
'watchlistedit-noitems'       => 'Perräkaemisnimekirän olõ-i üttegi päälkirjä.',
'watchlistedit-normal-title'  => 'Toimõndaq perräkaemisnimekirjä',
'watchlistedit-normal-legend' => 'Kistudaq päälkiräq perräkaemisnimekiräst ärq',
'watchlistedit-normal-submit' => 'Kistudaq päälkiräq ärq',
'watchlistedit-raw-titles'    => 'Päälkiräq:',
'watchlistedit-raw-submit'    => 'Vahtsõndaq perräkaemisnimekirjä',
'watchlistedit-raw-done'      => 'Perräkaemisniekiri om ärq vahtsõndõt.',
'watchlistedit-raw-added'     => 'Mano pant {{PLURAL:$1|1 päälkiri|$1 päälkirjä}}:',
'watchlistedit-raw-removed'   => 'Ärq kistutõt {{PLURAL:$1|1 päälkiri|$1 päälkirjä}}:',

# Watchlist editing tools
'watchlisttools-view' => 'Näütäq muutmiisi',
'watchlisttools-edit' => 'Kaeq ja toimõndaq perräkaemisnimekirjä',
'watchlisttools-raw'  => 'Toimõndaq lätteteedüstüt',

# Special:Version
'version'                  => 'Kujo', # Not used as normal message but as header for the special page itself
'version-version'          => 'Kujo',
'version-software-version' => 'Kujo',

# Special:FilePath
'filepath'        => 'Teedüstü aadrõs',
'filepath-page'   => 'Teedüstü:',
'filepath-submit' => 'Aadrõs',

# Special:FileDuplicateSearch
'fileduplicatesearch-filename' => 'Teedüstünimi:',
'fileduplicatesearch-submit'   => 'Otsiq',

# Special:SpecialPages
'specialpages'                   => 'Tallitusleheküleq',
'specialpages-note'              => '----
* Hariliguq tallitusleheq.
* <span class="mw-specialpagerestricted">Piiredüq tallitusleheq.</span>',
'specialpages-group-maintenance' => 'Kõrranpidämisteedüseq',
'specialpages-group-other'       => 'Muuq tallitusleheq',
'specialpages-group-login'       => 'Nimega sisseminek / Pruukjanime luuminõ',
'specialpages-group-changes'     => 'Muutmisõq ja muutmisnimekiräq',
'specialpages-group-media'       => 'Meediäteedüstüq',
'specialpages-group-users'       => 'Pruukjaq ja õigusõq',
'specialpages-group-highuse'     => 'Rohkõmbpruugiduq leheq',
'specialpages-group-pages'       => 'Lehenimekiräq',
'specialpages-group-pagetools'   => 'Lehetüüriistaq',
'specialpages-group-wiki'        => 'Vikiteedüseq ja tüüriistaq',
'specialpages-group-redirects'   => 'Ümbrenäütämistallitusleheq',

);
