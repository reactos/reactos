<?php
/** Gheg Albanian (Gegë)
 *
 * @ingroup Language
 * @file
 *
 * @author Cradel
 * @author Dardan
 */

$fallback = 'sq';

$messages = array(
# User preference toggles
'tog-underline'               => 'Nënvizoji lidhjet',
'tog-highlightbroken'         => 'Shfaqi lidhjet për në faqe të zbrazëta <a href="" class="new">kështu </a> (ndryshe: kështu<a href="" class="internal">?</a>).',
'tog-justify'                 => 'Drejtoji kryeradhët',
'tog-hideminor'               => 'Mshefi redaktimet e vogla të bame së voni',
'tog-extendwatchlist'         => 'Zgjano listën mbikëqyrëse me i pa të tana ndryshimet përkatëse',
'tog-usenewrc'                => 'Ndryshimet e mëdhaja të bame së voni (JavaScript)',
'tog-numberheadings'          => 'Vetshenjo me numër mbititujt',
'tog-showtoolbar'             => 'Shfaqi veglat për redaktim (JavaScript)',
'tog-editondblclick'          => 'Redaktoji faqet me klikim të dyfishtë (JavaScript)',
'tog-editsection'             => 'Lejoje redaktimin e seksioneve me opcionin [redaktoje]',
'tog-editsectiononrightclick' => 'Lejoje redaktimin e seksioneve tue klikue me të djathtë mbi titull (JavaScript)',
'tog-showtoc'                 => 'Shfaqe përmbajtjen<br />(për faqet me ma shum se 3 tituj)',
'tog-rememberpassword'        => 'Rueje fjalëkalimin në këtë kompjuter',
'tog-editwidth'               => 'Kutia për redaktim ka gjanësi të plotë',
'tog-watchcreations'          => 'Shtoji në listë mbikëqyrëse faqet që i krijoj',
'tog-watchdefault'            => 'Shtoji në listë mbikëqyrëse faqet që i redaktoj',
'tog-watchmoves'              => 'Shtoji në listë mbikëqyrëse faqet që i zhvendosi',
'tog-watchdeletion'           => 'Shtoji në listë mbikëqyrëse faqet që i fshij',
'tog-minordefault'            => 'Shënoji paraprakisht si të vogla të tana redaktimet',
'tog-previewontop'            => 'Vendose parapamjen përpara kutisë redaktuese',
'tog-previewonfirst'          => 'Shfaqe parapamjen në redaktimin e parë',
'tog-nocache'                 => 'Mos ruej kopje të faqeve',
'tog-enotifwatchlistpages'    => 'Njoftomë me email kur ndryshojnë faqet nën mbikëqyrje',
'tog-enotifusertalkpages'     => 'Njoftomë me email kur ndryshon faqja ime e diskutimit',
'tog-enotifminoredits'        => 'Njoftomë me email për redaktime të vogla të faqeve',
'tog-enotifrevealaddr'        => 'Shfaqe adresën time në emailat njoftues',
'tog-shownumberswatching'     => 'Shfaqe numrin e përdoruesve mbikëqyrës',
'tog-fancysig'                => 'Mos e përpuno nënshkrimin për formatim',
'tog-externaleditor'          => 'Përdor program të jashtem për redaktime',
'tog-externaldiff'            => 'Përdor program të jashtem për të tréguar ndryshimét',
'tog-showjumplinks'           => 'Lejo lidhjet é afrueshmerisë "kapërce tek"',
'tog-uselivepreview'          => 'Trego parapamjén meniheré (JavaScript) (Eksperimentale)',
'tog-forceeditsummary'        => 'Pyetem kur e le përmbledhjen e redaktimit zbrazt',
'tog-watchlisthideown'        => "M'sheh redaktimet e mia nga lista mbikqyrëse",
'tog-watchlisthidebots'       => "M'sheh redaktimet e robotave nga lista mbikqyrëse",
'tog-watchlisthideminor'      => "M'sheh redaktimet e vogla nga lista mbikqyrëse",
'tog-ccmeonemails'            => 'Më ço kopje të mesazhevé qi u dërgoj të tjerëve',
'tog-diffonly'                => 'Mos e trego përmbájtjen e fáqes nën ndryshimin',
'tog-showhiddencats'          => "Trego katégoritë e m'shefta",

'underline-always'  => 'gjithmonë',
'underline-never'   => 'kurrë',
'underline-default' => 'sipas shfletuesit',

'skinpreview' => '(Parapamje)',

# Dates
'sunday'        => 'E diel',
'monday'        => 'E háne',
'tuesday'       => 'E márte',
'wednesday'     => 'E mërkure',
'thursday'      => 'E énjte',
'friday'        => 'E prémte',
'saturday'      => 'E shtuné',
'sun'           => 'Diel',
'mon'           => 'Hán',
'tue'           => 'Már',
'wed'           => 'Mër',
'thu'           => 'Énj',
'fri'           => 'Pré',
'sat'           => 'Sht',
'january'       => 'kallnor',
'february'      => 'shkurt',
'march'         => 'mars',
'april'         => 'Prill',
'may_long'      => 'Maj',
'june'          => 'Qershor',
'july'          => 'Korrik',
'august'        => 'Gusht',
'september'     => 'Shtator',
'october'       => 'Tetor',
'november'      => 'Nëntor',
'december'      => 'Dhjetor',
'january-gen'   => 'kallnorit',
'february-gen'  => 'shkurtit',
'march-gen'     => 'marsit',
'april-gen'     => 'prillit',
'may-gen'       => 'majit',
'june-gen'      => 'qershorit',
'july-gen'      => 'korrikut',
'august-gen'    => 'Gusht',
'september-gen' => 'Shtator',
'october-gen'   => 'Tetor',
'november-gen'  => 'Nëntor',
'december-gen'  => 'Dhétor',
'jan'           => 'Jan',
'feb'           => 'Shk',
'mar'           => 'Mar',
'apr'           => 'Pri',
'may'           => 'Maj',
'jun'           => 'Qer',
'jul'           => 'Kor',
'aug'           => 'Gush',
'sep'           => 'Sht',
'oct'           => 'Tet',
'nov'           => 'Nën',
'dec'           => 'Dhj',

# Categories related messages
'pagecategories'           => '{{PLURAL:$1|Kategoria|Kategoritë}}',
'category_header'          => 'Artikuj në kategorinë "$1"',
'subcategories'            => 'Nën-kategori',
'category-media-header'    => 'Skeda në kategori "$1"',
'category-empty'           => "''Kjo kategori tashpërtash nuk përmban asnji faqe apo media.''",
'hidden-categories'        => '{{PLURAL:$1|Kategoritë e mshehta|Kategoritë e mshehta}}',
'hidden-category-category' => 'Kategori të mshehta', # Name of the category where hidden categories will be listed
'listingcontinuesabbrev'   => 'vazh.',

'mainpagetext'      => 'Wiki software u instalue me sukses.',
'mainpagedocfooter' => 'Për ma shumë informata rreth përdorimit të softwerit wiki , ju lutem shikoni [http://meta.wikimedia.org/wiki/Help:Contents dokumentacionin përkatës].


== Fillimisht ==

* [http://www.mediawiki.org/wiki/Help:Configuration_settings Parazgjedhjet e MediaWiki-t]
* [http://www.mediawiki.org/wiki/Help:FAQ Pyetjet e shpeshta rreth MediaWiki-t]
* [http://mail.wikimedia.org/mailman/listinfo/mediawiki-announce Njoftime rreth MediaWiki-t]',

'about'          => 'Rreth',
'article'        => 'Artikulli',
'newwindow'      => '(çelet në një dritare të re)',
'cancel'         => 'Harroji',
'qbfind'         => 'Kërko',
'qbbrowse'       => 'Shfletoni',
'qbedit'         => 'Redaktoni',
'qbpageoptions'  => 'Opsionet e faqes',
'qbpageinfo'     => 'Informacion mbi faqen',
'qbmyoptions'    => 'Opsionet e mia',
'qbspecialpages' => 'Fáqet speciále',
'moredotdotdot'  => 'Ma shumë...',
'mypage'         => 'Fáqja jémé',
'mytalk'         => 'Diskutimet e mia',
'anontalk'       => 'Diskutimet për këtë IP',
'navigation'     => 'Shfleto',
'and'            => 'dhe',

# Metadata in edit box
'metadata_help' => 'Metadata:',

'errorpagetitle'    => 'Gabim',
'returnto'          => 'Kthehu te $1.',
'tagline'           => 'Nga {{SITENAME}}, Enciklopedia e Lirë',
'help'              => 'Ndihmë',
'search'            => 'Kërko',
'searchbutton'      => 'Kërko',
'go'                => 'Shko',
'searcharticle'     => 'Shko',
'history'           => 'Historiku i faqes',
'history_short'     => 'Historiku',
'updatedmarker'     => 'ndryshuar nga vizita e fundit',
'info_short'        => 'Informacion',
'printableversion'  => 'Version shtypi',
'permalink'         => 'Lidhja e përhershme',
'print'             => 'Shtype',
'edit'              => 'Redaktoni',
'editthispage'      => 'Redaktoni faqen',
'delete'            => "ç'kyje",
'deletethispage'    => "Ç'kyje faqen",
'undelete_short'    => 'Restauroni $1 redaktime',
'protect'           => 'Mbroje',
'protect_change'    => 'ndrysho nivelin e mbrojtjes',
'protectthispage'   => 'Mbroje faqen',
'unprotect'         => 'Çliroje',
'unprotectthispage' => 'Çliroje faqen',
'newpage'           => 'Faqe e re',
'talkpage'          => 'Diskutoni faqen',
'talkpagelinktext'  => 'Diskuto',
'specialpage'       => 'Faqe speciale',
'personaltools'     => 'Mjete vetjake ( personale )',
'postcomment'       => 'Shtoni koment',
'articlepage'       => 'Shikoni artikullin',
'talk'              => 'Diskutimet',
'views'             => 'Shikime',
'toolbox'           => 'Mjete',
'userpage'          => 'Shikoni faqen',
'projectpage'       => 'Shikoni projekt-faqen',
'imagepage'         => 'Shikoni faqen e figurës',
'mediawikipage'     => 'Shikoni faqen e mesazhit',
'templatepage'      => 'Shiko faqen e stampës',
'viewhelppage'      => 'Shiko faqen për ndihmë',
'categorypage'      => 'Shiko faqen e kategorisë',
'viewtalkpage'      => 'Shikoni diskutimet',
'otherlanguages'    => "N'gjuhë tjera",
'redirectedfrom'    => '(Përcjellë nga $1)',
'redirectpagesub'   => 'Faqe përcjellëse',
'lastmodifiedat'    => 'Kjo faqe asht ndryshue për herë të fundit më $2, $1.', # $1 date, $2 time
'viewcount'         => 'Kjo faqe asht pá $1 herë.',
'protectedpage'     => 'Faqe e mbrojtme',
'jumpto'            => 'Shko te:',
'jumptonavigation'  => 'navigacion',
'jumptosearch'      => 'kërko',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'Rreth {{SITENAME}}',
'aboutpage'            => 'Project:Rreth',
'bugreports'           => 'Kontakt',
'bugreportspage'       => 'Project:Kontakt',
'copyright'            => 'Përmbajtja asht në disponim nëpërmjet liqencës $1.',
'copyrightpagename'    => '{{SITENAME}} Të drejta autori',
'copyrightpage'        => '{{ns:project}}:Të drejta autori',
'currentevents'        => 'Ngjarjet e tashme',
'currentevents-url'    => 'Project:Ngjarjet e tashme',
'disclaimers'          => 'Shfajësimet',
'disclaimerpage'       => 'Project:Shfajësimet e përgjithshme',
'edithelp'             => 'Ndihmë për redaktim',
'edithelppage'         => 'Help:Si me redaktue një faqe',
'faq'                  => 'Pyetje e Përgjegjje',
'faqpage'              => 'Project:Pyetje e Përgjegjje',
'helppage'             => 'Help:Ndihmë',
'mainpage'             => 'Faqja Kryesore',
'mainpage-description' => 'Faqja Kryesore',
'policy-url'           => 'Project:Policy',
'portal'               => 'Wikiportal',
'portal-url'           => 'Project:Wikiportal',
'privacy'              => 'Rreth të dhanave vetjake',
'privacypage'          => 'Project:Politika vetjake',

'badaccess'        => 'Gabim leje',
'badaccess-group0' => 'Nuk jeni lejue me e bá kët veprim.',
'badaccess-group1' => 'Ky veprim asht i limituem për përdoruesit e grupit $1',
'badaccess-group2' => 'Veprimi i kërkuem asht i limituem për përdoruesit e grupit $1.',
'badaccess-groups' => 'Ky veprim asht i limituem për përdoruesit e grupit $1.',

'versionrequired'     => 'Nevojitet versioni $1 i MediaWiki-it',
'versionrequiredtext' => 'Nevojitet versioni $1 i MediaWiki-it për përdorimin e kësaj faqeje. Shikoni [[Special:Version|versionin]] tuej.',

'ok'                      => 'Ani',
'retrievedfrom'           => 'Marrë nga "$1"',
'youhavenewmessages'      => 'Keni $1 ($2).',
'newmessageslink'         => 'porosi të reja',
'newmessagesdifflink'     => 'ndryshimi i fundit',
'youhavenewmessagesmulti' => 'Keni porosi të reja në $1',
'editsection'             => 'redaktoni',
'editold'                 => 'redaktoni',
'editsectionhint'         => 'Redaktoni seksionin: 
Edit section: $1',
'toc'                     => 'Tabela e përmbajtjeve',
'showtoc'                 => 'kallzo',
'hidetoc'                 => 'mshehe',
'thisisdeleted'           => 'Shikoni ose restauroni $1?',
'viewdeleted'             => 'A don me pa $1?',
'restorelink'             => '$1 redaktime të grisme',
'feedlinks'               => 'Ushqyes:',
'feed-invalid'            => 'Lloji i burimit të pajtimit asht i pavlefshëm.',
'feed-unavailable'        => 'Syndication feeds nuk pranohen në {{SITENAME}}',
'site-rss-feed'           => '$1 RSS Feed',
'site-atom-feed'          => '$1 Atom Feed',
'page-rss-feed'           => '"$1" RSS Feed',
'page-atom-feed'          => '"$1" Atom Feed',
'red-link-title'          => '$1 (i pashkruem)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Artikulli',
'nstab-user'      => 'Përdoruesi',
'nstab-media'     => 'Media-faqe',
'nstab-special'   => 'Speciale',
'nstab-project'   => 'Projekt-faqe',
'nstab-image'     => 'Figura',
'nstab-mediawiki' => 'Porosia',
'nstab-template'  => 'Stampa',
'nstab-help'      => 'Ndihmë',
'nstab-category'  => 'Kategori',

# Main script and global functions
'nosuchaction'      => 'Nuk ekziston ky veprim',
'nosuchactiontext'  => 'Veprimi i caktuem nga URL nuk
njihet nga wiki software',
'nosuchspecialpage' => 'Nuk ekziston kjo faqe speciale',
'nospecialpagetext' => 'Keni kërkue një faqe speciale qi nuk njihet nga wiki software.',

# General errors
'error'                => 'Gabim',
'databaseerror'        => 'Gabim regjistri',
'dberrortext'          => 'Ka ndodh një gabim me pyetjen e regjistrit. Kjo munem me ndodh nëse pyetja nuk asht e vlehshme (shikoni $5),
ose munet me kanë një yçkël e softuerit. Pyetja e fundit që i keni bá regjistrit ishte:
<blockquote><tt>$1</tt></blockquote>
nga funksioni "<tt>$2</tt>".
MySQL kthehu gabimin "<tt>$3: $4</tt>".',
'dberrortextcl'        => 'Ka ndodh një gabim me formatin e pyetjes së regjistrit. Pyetja e fundit qe i keni bá regjistrit ishte:
"$1"
nga funksioni "$2".
MySQL kthehu gabimin "$3: $4".',
'noconnect'            => 'Ju kërkojmë të falme! Defekt teknik, rifillojmë së shpejti.<br />
$1',
'nodb'                 => 'Nuk mujta me zgjidh regjistrin $1',
'cachederror'          => 'Kjo asht një kopje e faqes së kërkueme dhe munet me kánë e vjetër.',
'laggedslavemode'      => 'Kujdes: Kjo faqe munet mos me kánë e përtërime nga shërbyesi kryesor dhe munet me pas informacion të vjetër',
'readonly'             => 'Regjistri i bllokuem',
'enterlockreason'      => 'Futni një arsye për bllokimin, gjithashtu futni edhe kohën se kur pritet të çbllokohet',
'readonlytext'         => "Regjistri i {{SITENAME}}-s asht i bllokuem dhe nuk lejon redaktime dhe
artikuj t'ri. Munet qi asht bllokuar për mirëmbajtje,
dhe do të kthehet në gjèndje normale mas mirëmbajtjes.

Mirëmbajtësi i cili e ka bllokue dha këtë arsye: $1",
'readonly_lag'         => "Regjistri asht bllokue automatikisht për m'i dhánë kohë shërbyesve skllevër me arrit kryesorin. Ju lutemi provojeni prap ma vonë.",
'internalerror'        => 'Gabim i brendshëm',
'internalerror_info'   => 'Gabimi i brenshëm: $1',
'filecopyerror'        => 'Nuk mujta me kopjue skedën "$1" te "$2".',
'filerenameerror'      => 'Nuk mujta me ndërrue emrin e skedës "$1" në "$2".',
'filedeleteerror'      => 'Nuk mujta me çky skedën "$1".',
'directorycreateerror' => 'Nuk munet me u kriju direktoria "$1".',
'filenotfound'         => 'Nuk mujta me gjetë skedën "$1".',
'fileexistserror'      => 'Dosja "$1" nuk munet me u shkru : Kjo Dosje ekziston',
'unexpected'           => 'Vlerë e papritur: "$1"="$2".',
'formerror'            => 'Gabim: nuk mujta me dërgue formularin',
'badarticleerror'      => 'Ky veprim nuk munet me u bá në kët faqe.',
'cannotdelete'         => 'Nuk mujta me gris këtë faqe ose figurë të dhënë. (Munet qi asht e grisur nga dikush tjeter.)',
'badtitle'             => 'Titull i gabuem',
'badtitletext'         => 'Titulli i faqes qi kërkuet nuk ishte i saktë, ishte bosh, ose ishte një lidhje gabim me një titull wiki internacional.',
'perfdisabled'         => "Ju kërkoj të falme! Ky veprim asht bllokue përkohsisht se e ngadalëson regjistrin aq shumë sa s'munet me përdor kërrkush tjetër.",
'perfcached'           => 'Informacioni i mëposhtëm asht kopje e ruajtme dhe munet mos me kán e freskët:',
'perfcachedts'         => 'Informacioni i mëposhtëm asht një kopje e rifreskueme me $1.',
'wrong_wfQuery_params' => 'Parametra gabim te wfQuery()<br />
Funksioni: $1<br />
Pyetja: $2',
'viewsource'           => 'Shikoni tekstin',
'viewsourcefor'        => 'e $1',
'protectedpagetext'    => 'Kjo faqe asht mbyll për me ndal redaktimin.',
'viewsourcetext'       => 'Ju mund të shikoni dhe kopjoni tekstin burimor të kësaj faqe:',
'protectedinterface'   => 'Kjo faqe përmban tekst për pamjen gjuhësorë të softuerit dhe asht e mbrojtme për të pengu keqpërdorimet.',
'editinginterface'     => "'''Kujdes:''' Po redaktoni një faqe qi përdoret për tekstin ose pamjen e softuerit. Ndryshimet e kësaj faqeje do të prekin tekstin ose pamjen për të gjithë përdoruesit e tjerë.",
'sqlhidden'            => '(Pyetje SQL e mshehur)',
'customcssjsprotected' => 'Nuk keni leje me ndryshu këtë faqe sepse përmban informata personale të një përdoruesi tjetër',

# Login and logout pages
'logouttitle'             => 'Përdoruesi ka dál',
'logouttext'              => 'Keni dálë jashtë {{SITENAME}}-s. Muneni me vazhdu me përdor {{SITENAME}}-n anonimisht, ose muneni me hy brenda prap.',
'welcomecreation'         => '== Mirësevini, $1! ==

Llogaria juej asht hap. Mos harroni me ndryshu parapëlqimet e {{SITENAME}}-s.',
'yourpassword'            => 'Futni fjalëkalimin tuej',
'yourpasswordagain'       => 'Futni fjalëkalimin prap',
'remembermypassword'      => 'Mbaj mend fjalëkalimin tim për krejt vizitat e ardhshme.',
'yourdomainname'          => 'Faqja juej',
'externaldberror'         => 'Ose kishte një gabim te regjistri i identifikimit të jashtëm, ose nuk ju lejohet të përtërini llogarinë tuej të jashtme.',
'login'                   => 'Hyni',
'nav-login-createaccount' => 'Hyni ose çeleni një llogari',
'userlogin'               => 'Hyni ose çeleni një llogari',
'logout'                  => 'Dalje',
'userlogout'              => 'Dalje',
'notloggedin'             => 'Nuk keni hy brenda',
'nologinlink'             => 'Çeleni',
'gotaccount'              => 'A keni një llogari? $1.',
'gotaccountlink'          => 'Hyni',
'createaccountmail'       => 'me email',
'userexists'              => 'Nofka që përdorët asht në përdorim. Zgjidhni një nofkë tjetër.',
'youremail'               => 'Adresa e email-it*',
'username'                => 'Nofka e përdoruesit:',
'uid'                     => 'Nr. i identifikimit:',
'yourrealname'            => 'Emri juej i vërtetë*',
'yourlanguage'            => 'Ndërfaqja gjuhësore',
'yournick'                => 'Nofka :',
'badsig'                  => 'Sintaksa e nënshkrimit asht e pavlefshme, kontrolloni HTML-n.',
'badsiglength'            => 'Emri i zgjedhun asht shumë i gjatë; duhet me pas ma pak se $1 shkronja',
'email'                   => 'Email',
'nocookieslogin'          => '{{SITENAME}} përdor "biskota" për me futë brenda përdoruesit. Prandaj, duhet të pranoni "biskota" dhe të provoni prap.',

# Edit page toolbar
'bold_sample'     => 'Tekst i trashë',
'bold_tip'        => 'Tekst i trashë',
'italic_sample'   => 'Tekst i pjerrët',
'italic_tip'      => 'Tekst i pjerrët',
'link_sample'     => 'Titulli i lidhjes',
'link_tip'        => 'Lidhje e brendshme',
'extlink_sample'  => 'http://www.example.com Titulli i lidhjes',
'extlink_tip'     => 'Lidhje e jashtme (mos e harro prefiksin http://)',
'headline_sample' => 'Titull shembull',
'headline_tip'    => 'Titull i nivelit 2',
'math_sample'     => 'Vendos formulën këtu',
'math_tip'        => 'Formulë matematike (LaTeX)',
'nowiki_sample'   => 'Vendos tekst qi nuk duhet me u formatue',
'nowiki_tip'      => 'Mos përdor format wiki',
'image_tip'       => 'Vendose një figurë',
'media_tip'       => 'Lidhje media-skedave',
'sig_tip'         => 'Firma juej dhe koha e firmosjes',
'hr_tip'          => 'vijë horizontale (përdoreni rallë)',

# Edit pages
'summary'            => 'Përmbledhje',
'subject'            => 'Subjekt/Titull',
'minoredit'          => 'Ky asht një redaktim i vogël',
'watchthis'          => 'Mbikqyre kët faqe',
'showpreview'        => 'Trego parapamjen',
'showdiff'           => 'Trego ndryshimet',
'anoneditwarning'    => 'Ju nuk jeni regjistruem. IP adresa juej do të regjistrohet në historinë e redaktimeve të kësaj faqe.',
'newarticletext'     => "{{SITENAME}} nuk ka një ''{{NAMESPACE}} faqe'' të quajtme '''{{PAGENAME}}'''. Shtypni '''redaktoni''' ma sipër ose [[Special:Search/{{PAGENAME}}|bani një kërkim për {{PAGENAME}}]]",
'noarticletext'      => 'Tash për tash nuk ka tekst në kët faqe, muneni me [[Special:Search/{{PAGENAME}}|kërkue]] kët titull në faqe të tjera ose muneni me [{{fullurl:{{FULLPAGENAME}}|action=edit}} fillu] atë.',
'editing'            => 'Tuj redaktue $1',
'copyrightwarning'   => "Kontributet te {{SITENAME}} janë të konsiderueme të dhana nën licensën $2 (shikoni $1 për hollësirat).<br />
'''NDALOHET DHËNIA E PUNIMEVE PA PAS LEJE NGA AUTORI NË MOSPËRPUTHJE ME KËTË LICENSË!'''<br />",
'template-protected' => '(e mbrojtme)',

# History pages
'revisionasof'     => 'Versioni i $1',
'revision-info'    => 'Versioni me $1 nga $2',
'previousrevision' => '← Verzion ma i vjetër',
'cur'              => 'tash',
'last'             => 'fund',

# Diffs
'lineno'                  => 'Rreshti $1:',
'compareselectedversions' => 'Krahasoni versionet e zgjedhme',
'editundo'                => 'ktheje',

# Search results
'noexactmatch' => '<span style="font-size: 135%; font-weight: bold; margin-left: .6em">Faqja me atë titull nuk asht krijue </span>

<span style="display: block; margin: 1.5em 2em">
Muneni me [[$1|fillu një artikull]] me kët titull.

<span style="display:block; font-size: 89%; margin-left:.2em">Ju lutem kërkoni {{SITENAME}}-n para se me krijue një artikull të ri se munet me kánë nën një titull tjetër.</span>
</span>',
'viewprevnext' => 'Shikoni ($1) ($2) ($3).',
'powersearch'  => 'Kërko',

# Preferences page
'mypreferences' => 'Parapëlqimet',

# Recent changes
'recentchanges'   => 'Ndryshimet e fundit',
'rcnote'          => 'Ma poshtë janë <strong>$1</strong> ndryshimt e fundit gjatë <strong>$2</strong> ditëve sipas të dhanave nga $3.',
'rcshowhideminor' => '$1 redaktimet e vogla',
'rcshowhidepatr'  => '$1 redaktime të patrullueme',
'rclinks'         => 'Trego $1 ndryshime gjatë $2 ditëve<br />$3',
'diff'            => 'ndrysh',
'hist'            => 'hist',
'hide'            => 'msheh',
'show'            => 'kallzo',
'minoreditletter' => 'v',
'newpageletter'   => 'R',
'boteditletter'   => 'b',

# Recent changes linked
'recentchangeslinked'       => 'Ndryshimet fqinje',
'recentchangeslinked-title' => 'Ndryshimet në lidhje me "$1"',

# Upload
'upload' => 'Ngarkoni skeda',

# Image description page
'filehist'            => 'Historiku i dosjes',
'filehist-datetime'   => 'Data/Ora',
'filehist-user'       => 'Përdoruesi',
'filehist-dimensions' => 'Dimenzionet',
'filehist-filesize'   => 'Madhësia e figurës/skedës',
'filehist-comment'    => 'Koment',
'imagelinks'          => 'Lidhje e skedave',
'linkstoimage'        => "K'to faqe lidhen te kjo figurë/skedë:",
'sharedupload'        => 'Kjo skedë asht një ngarkim i përbashkët dhe munet me u përdor nga projekte të tjera.',

# File deletion
'filedelete-reason-otherlist' => 'Arsyje tjera',

# MIME search
'download' => 'shkarkim',

# Random page
'randompage' => 'Artikull i rastit',

# Statistics
'statistics' => 'Statistika',

'withoutinterwiki' => 'Artikuj pa lidhje interwiki',

# Miscellaneous special pages
'nbytes'   => '$1 bytes',
'nlinks'   => '$1 lidhje',
'nmembers' => '$1 anëtarë',
'move'     => 'Zhvendose',

# Special:AllPages
'alphaindexline' => '$1 deri në $2',
'allpagessubmit' => 'Shko',

# Special:Categories
'categories' => 'Kategori',

# Watchlist
'mywatchlist'          => 'Lista mbikqyrëse',
'addedwatch'           => 'U shtu te lista mbikqyrëse',
'removedwatch'         => 'U hjek nga lista mibkqyrëse',
'removedwatchtext'     => 'Faqja "<nowiki>$1</nowiki>" asht hjek nga lista mbikqyrëse e juej.',
'watch'                => 'Mbikqyre',
'unwatch'              => 'Çmbikqyre',
'watchlist-hide-own'   => 'Mshehi redaktimet e mija',
'watchlist-hide-minor' => 'Mshehi redaktimet e vogla',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Tuj mbikqyrë...',
'unwatching' => 'Tuj çmbikqyrë...',

# Delete/protect/revert
'deletedarticle'              => 'grisi "$1"',
'protect-legend'              => 'Konfirmoni',
'protectcomment'              => 'Arsyja:',
'protectexpiry'               => 'Afáti',
'protect_expiry_invalid'      => 'Data e skadimit asht e pasaktë.',
'protect_expiry_old'          => 'Data e skadimit asht në kohën kalueme.',
'protect-unchain'             => 'Ndryshoje lejen e zhvendosjeve',
'protect-text'                => 'Këtu muneni me shiku dhe me ndryshu nivelin e mbrojtjes për faqen <strong><nowiki>$1</nowiki></strong>.',
'protect-locked-access'       => 'Llogaria juej nuk ka privilegjet e nevojitme për me ndryshu nivelin e mbrojtjes. Kufizimet e kësaj faqe janë <strong>$1</strong>:',
'protect-default'             => '(parazgjedhje)',
'protect-level-autoconfirmed' => 'Blloko përdoruesit pa llogari',
'protect-level-sysop'         => 'Lejo veç administruesit',
'protect-expiring'            => 'skadon me $1 (UTC)',
'protect-cascade'             => 'Mbrojtje e ndërlidhme - mbroj çdo faqe që përfshihet në këtë faqe.',
'protect-cantedit'            => 'Nuk nuk muneni me ndryshu nivelin e mbrojtjes në kët faqe, sepse nuk keni leje.',
'restriction-type'            => 'Lejet:',
'restriction-level'           => 'Mbrojtjet:',

# Namespace form on various pages
'namespace'      => 'Hapësira:',
'blanknamespace' => '(Artikujt)',

# Contributions
'contributions' => 'Kontributet',
'mycontris'     => 'Redaktimet e mia',

# What links here
'whatlinkshere'       => "Lidhjet k'tu",
'whatlinkshere-title' => 'Faqe qi lidhen me $1',
'linklistsub'         => '(Listë e lidhjeve)',
'linkshere'           => "Faqet e mëposhtme lidhen k'tu '''[[:$1]]''':",
'isredirect'          => 'faqe përcjellëse',
'istemplate'          => 'përfshirë',
'whatlinkshere-links' => '← lidhje',

# Block/unblock
'blocklink'    => 'bllokoje',
'contribslink' => 'kontribute',

# Move page
'movearticle' => 'Zhvendose faqen',
'newtitle'    => 'Te titulli i ri',
'move-watch'  => 'Mbikqyre kët faqe',
'movepagebtn' => 'Zhvendose faqen',
'movedto'     => 'zhvendosur te',
'movereason'  => 'Arsyja',

# Thumbnails
'thumbnail-more'  => 'Zmadho',
'thumbnail_error' => 'Gabim gjatë krijimit të figurës përmbledhëse: $1',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'Faqja juej e përdoruesit',
'tooltip-pt-mytalk'               => 'Faqja juej e diskutimeve',
'tooltip-pt-preferences'          => 'Parapëlqimet tuaja',
'tooltip-pt-watchlist'            => 'Lista e faqeve nën mbikqyrjen tuej.',
'tooltip-pt-mycontris'            => 'Lista e kontributeve tueja',
'tooltip-pt-login'                => 'Me hy brenda nuk asht e detyrueshme, po ká shumë përparësi.',
'tooltip-pt-logout'               => 'Dalje',
'tooltip-ca-talk'                 => 'Diskuto për përmbajtjen e faqes',
'tooltip-ca-edit'                 => "Ju muneni me redaktue kët faqe. Përdorni butonin >>Trego parapamjen<< para se t'i kryni ndryshimet.",
'tooltip-ca-addsection'           => "Nis një temë t're diskutimi.",
'tooltip-ca-viewsource'           => 'Kjo faqe asht e mbrojtme. Ju muneni veç ta shikoni burimin e tekstit.',
'tooltip-ca-move'                 => 'Zhvendose faqen',
'tooltip-ca-watch'                => 'Shtoje kët faqe në lisën e faqeve nën mbikqyrje',
'tooltip-search'                  => 'Kërko në projekt',
'tooltip-n-mainpage'              => 'Vizitojeni Faqen kryesore',
'tooltip-n-portal'                => 'Mbi projektin, çka muneni me bá për të dhe ku gjénden faqet.',
'tooltip-n-currentevents'         => 'Informacion rreth ngjarjeve aktuale.',
'tooltip-n-recentchanges'         => 'Lista e ndryshimeve të fundme në projekt',
'tooltip-n-randompage'            => 'Shikoni një artikull të rastit.',
'tooltip-n-help'                  => 'Vendi ku muneni me gjetë ndihmë.',
'tooltip-t-whatlinkshere'         => 'Lista e faqeve qi lidhen te kjo faqe',
'tooltip-t-upload'                => 'Ngarkoni figura ose skeda tjera',
'tooltip-t-specialpages'          => 'Lista e krejt faqeve speciale.',
'tooltip-ca-nstab-image'          => 'Shikoni faqen e figurës',
'tooltip-ca-nstab-category'       => 'Shikoni faqen e kategorisë',
'tooltip-save'                    => 'Kryej ndryshimet',
'tooltip-preview'                 => 'Shiko parapamjen e ndryshimeve, përdore këtë para se me kry ndryshimet!',
'tooltip-diff'                    => 'Trego ndryshimet që Ju i keni bá tekstit.',
'tooltip-compareselectedversions' => 'Shikoni krahasimin midis dy versioneve të zgjedhme të kësaj faqe.',

# Browsing diffs
'previousdiff' => '← Nryshimi ma përpara',

# Media information
'file-nohires'   => '<small>Rezolucioni i plotë.</small>',
'show-big-image' => 'Rezolucion i plotë',

# Metadata
'metadata'        => 'Metadata',
'metadata-help'   => 'Kjo skedë përmban hollësira tjera të cilat munen qi jan shtue nga kamera ose skaneri dixhital që është përdorur për ta krijuar. Nëse se skeda asht ndryshue nga gjendja origjinale, disa hollësira munen mos me pasqyru skedën e tashme.',
'metadata-expand' => 'Tregoji detajet',

# External editor support
'edit-externally'      => 'Ndryshoni kët figurë/skedë me një mjet të jashtëm',
'edit-externally-help' => 'Shikoni [http://www.mediawiki.org/wiki/Manual:External_editors udhëzimet e instalimit] për ma shumë informacion.',

# 'all' in various places, this might be different for inflected languages
'watchlistall2' => 'krejt',
'namespacesall' => 'krejt',

# Special:SpecialPages
'specialpages' => 'Faqet speciale',

);
