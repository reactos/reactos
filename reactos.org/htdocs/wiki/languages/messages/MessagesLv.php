<?php
/** Latvian (Latviešu)
 *
 * @ingroup Language
 * @file
 *
 * @author Knakts
 * @author Xil
 * @author Yyy
 * @author לערי ריינהארט
 */

/*
 * @copyright Copyright © 2006, Niklas Laxström
 * @license http://www.gnu.org/copyleft/gpl.html GNU General Public License 2.0 or later
 */

$namespaceNames = array(
	NS_MEDIA            => 'Media',
	NS_SPECIAL          => 'Special',
	NS_MAIN             => '',
	NS_TALK             => 'Diskusija',
	NS_USER             => 'Lietotājs',
	NS_USER_TALK        => 'Lietotāja_diskusija',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK     => '{{grammar:ģenitīvs|$1}}_diskusija',
	NS_IMAGE            => 'Attēls',
	NS_IMAGE_TALK       => 'Attēla_diskusija',
	NS_MEDIAWIKI        => 'MediaWiki',
	NS_MEDIAWIKI_TALK   => 'MediaWiki_diskusija',
	NS_TEMPLATE         => 'Veidne',
	NS_TEMPLATE_TALK    => 'Veidnes_diskusija',
	NS_HELP             => 'Palīdzība',
	NS_HELP_TALK        => 'Palīdzības_diskusija',
	NS_CATEGORY         => 'Kategorija',
	NS_CATEGORY_TALK    => 'Kategorijas_diskusija',
);
$separatorTransformTable = array(',' => "\xc2\xa0", '.' => ',' );

$messages = array(
# User preference toggles
'tog-underline'               => 'Pasvītrot saites:',
'tog-highlightbroken'         => 'Saites uz neesošām lapām rādīt <a href="" class="new">šādi</a> (alternatīva: šādi<a href="" class="internal">?</a>).',
'tog-justify'                 => 'Taisnot rindkopas',
'tog-hideminor'               => 'Paslēpt maznozīmīgus labojumus pēdējo izmaiņu lapā',
'tog-extendwatchlist'         => 'Izvērst uzraugāmo sarakstu, lai parādītu visas veiktās izmaiņas',
'tog-usenewrc'                => "Uzlabotas pēdējās izmaiņas (izmanto ''JavaScript'')",
'tog-numberheadings'          => 'Automātiski numurēt virsrakstus.',
'tog-showtoolbar'             => 'Rādīt rediģēšanas rīkjoslu',
'tog-editondblclick'          => "Atvērt rediģēšanas lapu ar dubultklikšķi (izmanto ''JavaScript'')",
'tog-editsection'             => 'Rādīt sadaļu izmainīšanas saites "izmainīt šo sadaļu"',
'tog-editsectiononrightclick' => 'Atvērt sadaļas izmainīšanas lapu, uzklikšķinot ar labo pogu uz sadaļas virsraksta (JavaScript)',
'tog-showtoc'                 => 'Parādīt satura rādītāju (lapām, kurās ir vairāk par 3 virsrakstiem)',
'tog-rememberpassword'        => 'Atcerēties paroli pēc pārlūka aizvēršanas',
'tog-editwidth'               => 'Parādīt izmainīšanas logu pilnā platumā',
'tog-watchcreations'          => 'Pievienot tevis radītās lapas uzraugāmo lapu sarakstam',
'tog-watchdefault'            => 'Pievienot tevis izmainītās lapas uzraugāmo lapu sarakstam',
'tog-watchmoves'              => 'Pievienot manis pārvietotās lapas uzraugāmajiem rakstiem',
'tog-watchdeletion'           => 'Pievienot manis izdzēstās lapas uzraugāmajiem rakstiem',
'tog-minordefault'            => 'Atzīmēt visus labojumus jau sākotnēji par maznozīmīgiem',
'tog-previewontop'            => 'Parādīt priekšskatījumu virs rediģēšanas loga, nevis zem.',
'tog-previewonfirst'          => 'Parādīt priekšskatījumu jau sākotnējā labošanā.',
'tog-nocache'                 => 'Neļaut pārlūkam saglabāt lapas kešatmiņā',
'tog-enotifwatchlistpages'    => 'Paziņot pa e-pastu par uzraugāmo rakstu sarakstā esošo rakstu izmaiņām',
'tog-enotifusertalkpages'     => 'Paziņot pa e-pastu par izmaiņām manā diskusiju lapā',
'tog-enotifminoredits'        => 'Paziņot pa e-pastu arī par maznozīmīgiem rakstu labojumiem',
'tog-enotifrevealaddr'        => 'Atklāt manu e-pasta adresi paziņojumu vēstulēs',
'tog-shownumberswatching'     => 'Rādīt uzraudzītāju skaitu',
'tog-fancysig'                => 'Vienkāršs paraksts (bez automātiskās saites)',
'tog-externaleditor'          => 'Pēc noklusējuma izmantot ārēju programmu lapu izmainīšanai (tikai pieredzējušiem lietotājiem, nepieciešami speciāli uzstādījumi tavā datorā (lai tas darbotos))',
'tog-externaldiff'            => 'Pēc noklusējuma izmantot ārēju programmu izmaiņu parādīšanai (tikai pieredzējušiem lietotājiem, nepieciešami speciāli uzstādījumi tavā datorā (lai tas darbotos))',
'tog-showjumplinks'           => 'Rādīt pārlēkšanas saites',
'tog-uselivepreview'          => 'Lietot tūlītējo priekšskatījumu (izmanto "JavaScript"; eksperimentāla iespēja).',
'tog-forceeditsummary'        => 'Atgādināt man, ja kopsavilkuma ailīte ir tukša',
'tog-watchlisthideown'        => 'Paslēpt manus labojumus manā uzraugāmo sarakstā.',
'tog-watchlisthidebots'       => 'Paslēpt botu labojumus manā uzraugāmo sarakstā.',
'tog-watchlisthideminor'      => 'Paslēpt maznozīmīgos labojumus manā uzraugāmo sarakstā',
'tog-ccmeonemails'            => 'Sūtīt sev, citiem lietotājiem nosūtīto epastu, kopijas',
'tog-diffonly'                => 'Nerādīt lapu saturu zem izmaiņām',
'tog-showhiddencats'          => 'Rādīt slēptās kategorijas',

'underline-always'  => 'vienmēr',
'underline-never'   => 'nekad',
'underline-default' => 'Kā pārlūkā',

'skinpreview' => '(Priekšskats)',

# Dates
'sunday'        => 'svētdiena',
'monday'        => 'Pirmdiena',
'tuesday'       => 'otrdiena',
'wednesday'     => 'trešdiena',
'thursday'      => 'ceturtdiena',
'friday'        => 'piektdiena',
'saturday'      => 'sestdiena',
'sun'           => 'Sv',
'mon'           => 'Pr',
'tue'           => 'Ot',
'wed'           => 'Tr',
'thu'           => 'Ce',
'fri'           => 'Pk',
'sat'           => 'Se',
'january'       => 'janvārī',
'february'      => 'februārī',
'march'         => 'martā',
'april'         => 'aprīlī',
'may_long'      => 'maijā',
'june'          => 'jūnijā',
'july'          => 'jūlijā',
'august'        => 'augustā',
'september'     => 'septembrī',
'october'       => 'oktobrī',
'november'      => 'novembrī',
'december'      => 'decembrī',
'january-gen'   => 'Janvāra',
'february-gen'  => 'Februāra',
'march-gen'     => 'Marta',
'april-gen'     => 'Aprīļa',
'may-gen'       => 'Maija',
'june-gen'      => 'Jūnija',
'july-gen'      => 'Jūlija',
'august-gen'    => 'Augusta',
'september-gen' => 'Septembra',
'october-gen'   => 'Oktobra',
'november-gen'  => 'Novembra',
'december-gen'  => 'Decembra',
'jan'           => 'janvārī,',
'feb'           => 'februārī,',
'mar'           => 'martā,',
'apr'           => 'aprīlī,',
'may'           => 'maijā,',
'jun'           => 'jūnijā,',
'jul'           => 'jūlijā,',
'aug'           => 'augustā,',
'sep'           => 'septembrī,',
'oct'           => 'oktobrī,',
'nov'           => 'novembrī,',
'dec'           => 'decembrī,',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|Kategorija|Kategorijas}}',
'category_header'                => 'Raksti, kas ietverti kategorijā "$1".',
'subcategories'                  => 'Apakškategorijas',
'category-media-header'          => 'Faili kategorijā "$1"',
'category-empty'                 => "''Šī kategorija šobrīd nesatur ne lapas, ne failus''",
'hidden-categories'              => '{{PLURAL:$1|Slēpta kategorija|Slēptas kategorijas}}',
'hidden-category-category'       => 'Slēptās kategorijas', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|Šajai kategorijai ir tikai viena apakškategorija.|Šajai kategorijai ir $2 apakškategorijas, no kurām ir {{PLURAL:$1|redzama viena|redzamas $1}}.}}',
'category-subcat-count-limited'  => 'Šai kategorijai ir {{PLURAL:$1|viena apakškategorija|$1 apakškategorijas}}.',
'category-article-count'         => '{{PLURAL:$2|Šī kategorija satur tikai šo vienu lapu.|Šajā kategorijā kopā ir $2 lapas, šobrīd ir {{PLURAL:$1|redzama viena no tām|redzamas $1 no tām}}.}}',
'category-article-count-limited' => 'Šajā kategorijā ir {{PLURAL:$1|šī viena lapa|šīs $1 lapas}}.',
'category-file-count'            => '{{PLURAL:$2|Šī kategorija satur tikai šo vienu failu.|Šajā kategorijā ir $2 faili, no kuriem {{PLURAL:$1|redzams ir viens|ir redzami $1}}.}}',
'category-file-count-limited'    => 'Šajā kategorijā atrodas {{PLURAL:$1|tikai šis fails|šie $1 faili}}.',
'listingcontinuesabbrev'         => ' (turpinājums)',

'mainpagetext'      => "<big>'''MediaWiki veiksmīgi ieinstalēts'''</big>",
'mainpagedocfooter' => 'Izlasi [http://meta.wikimedia.org/wiki/Help:Contents Lietotāja pamācību], lai iegūtu vairāk informācijas par Wiki programmatūras lietošanu.

== Pirmie soļi ==
* [http://www.mediawiki.org/wiki/Manual:Configuration_settings Konfigurācijas iespēju saraksts]
* [http://www.mediawiki.org/wiki/Manual:FAQ MediaWiki J&A]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce Parakstīties uz paziņojumiem par jaunām MediaWiki versijām]',

'about'          => 'Par',
'article'        => 'Raksts',
'newwindow'      => '(atveras jaunā logā)',
'cancel'         => 'Atcelt',
'qbfind'         => 'Meklēšana',
'qbbrowse'       => 'Navigācija',
'qbedit'         => 'Izmainīšana',
'qbpageoptions'  => 'Šī lapa',
'qbpageinfo'     => 'Konteksts',
'qbmyoptions'    => 'Manas lapas',
'qbspecialpages' => 'Īpašās lapas',
'moredotdotdot'  => 'Vairāk...',
'mypage'         => 'Mana lapa',
'mytalk'         => 'Mana diskusija',
'anontalk'       => 'Šīs IP adreses diskusija',
'navigation'     => 'Navigācija',
'and'            => 'un',

'errorpagetitle'    => 'Kļūda',
'returnto'          => 'Atgriezties: $1.',
'tagline'           => "No ''{{grammar:ģenitīvs|{{SITENAME}}}}''",
'help'              => 'Palīdzība',
'search'            => 'Meklēt',
'searchbutton'      => 'Meklēt',
'go'                => 'Aiziet!',
'searcharticle'     => 'Aiziet!',
'history'           => 'hronoloģija',
'history_short'     => 'Hronoloģija',
'updatedmarker'     => 'atjaunināti kopš pēdējā apmeklējuma',
'info_short'        => 'Informācija',
'printableversion'  => 'Drukājama versija',
'permalink'         => 'Pastāvīgā saite',
'print'             => 'Drukāt',
'edit'              => 'Izmainīt šo lapu',
'create'            => 'Izveidot',
'editthispage'      => 'Izmainīt šo lapu',
'create-this-page'  => 'Izveidot šo lapu',
'delete'            => 'Dzēst',
'deletethispage'    => 'Dzēst šo lapu',
'undelete_short'    => 'Atjaunot $1 {{PLURAL:$1|versiju|versijas}}',
'protect'           => 'Aizsargāt',
'protect_change'    => 'izmainīt',
'protectthispage'   => 'Aizsargāt šo lapu',
'unprotect'         => 'Neaizsargāt',
'unprotectthispage' => 'Neaizsargāt šo lapu',
'newpage'           => 'Jauna lapa',
'talkpage'          => 'Diskusija par šo lapu',
'talkpagelinktext'  => 'Diskusija',
'specialpage'       => 'Īpašā Lapa',
'personaltools'     => 'Lietotāja rīki',
'postcomment'       => 'Pievienot komentāru',
'articlepage'       => 'Apskatīt rakstu',
'talk'              => 'Diskusija',
'views'             => 'Apskates',
'toolbox'           => 'Rīki',
'userpage'          => 'Skatīt lietotāja lapu',
'projectpage'       => 'Skatīt projekta lapu',
'imagepage'         => 'Aplūkot attēla lapu',
'viewtalkpage'      => 'Skatīt diskusiju',
'otherlanguages'    => 'Citās valodās',
'redirectedfrom'    => '(Pāradresēts no $1)',
'redirectpagesub'   => 'Pāradresācijas lapa',
'lastmodifiedat'    => 'Šajā lapā pēdējās izmaiņas izdarītas $2, $1.', # $1 date, $2 time
'viewcount'         => 'Šī lapa ir tikusi apskatīta $1 {{PLURAL:$1|reizi|reizes}}.',
'protectedpage'     => 'Aizsargāta lapa',
'jumpto'            => 'Pārlēkt uz:',
'jumptonavigation'  => 'navigācija',
'jumptosearch'      => 'meklēt',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'Par {{grammar:akuzatīvs|{{SITENAME}}}}',
'aboutpage'            => 'Project:Par',
'bugreports'           => 'Kļūdu paziņojumi',
'copyright'            => 'Saturs ir pieejams saskaņā ar $1.',
'copyrightpagename'    => '{{grammar:ģenitīvs|{{SITENAME}}}} autortiesības',
'copyrightpage'        => '{{ns:project}}:Autortiesības',
'currentevents'        => 'Aktualitātes',
'currentevents-url'    => 'Project:Aktualitātes',
'disclaimers'          => 'Saistību atrunas',
'edithelp'             => 'Palīdzība izmaiņām',
'edithelppage'         => 'Help:Rediģēšana',
'helppage'             => 'Help:Saturs',
'mainpage'             => 'Sākumlapa',
'mainpage-description' => 'Sākumlapa',
'portal'               => 'Kopienas portāls',
'portal-url'           => 'Project:Kopienas portāls',
'privacy'              => 'Privātuma politika',
'privacypage'          => 'Project:Privātuma politika',

'badaccess'        => 'Atļaujas kļūda',
'badaccess-group0' => 'Tev nav atļauts izpildīt darbību, kuru tu pieprasīji.',

'versionrequired'     => "Nepieciešamā ''MediaWiki'' versija: $1.",
'versionrequiredtext' => "Lai lietotu šo lapu, nepieciešama ''MediaWiki'' versija $1. Sk. [[Special:Version|versija]].",

'ok'                  => 'Labi',
'retrievedfrom'       => 'Saturs iegūts no "$1"',
'youhavenewmessages'  => 'Tev ir $1 (skat. $2).',
'newmessageslink'     => 'jauns vēstījums',
'newmessagesdifflink' => 'izmaiņu lapu, lai redzētu, kas jauns',
'editsection'         => 'izmainīt šo sadaļu',
'editold'             => 'rediģēt',
'viewsourceold'       => 'aplūkot kodu',
'editsectionhint'     => 'Rediģēt sadaļu: $1',
'toc'                 => 'Satura rādītājs',
'showtoc'             => 'parādīt',
'hidetoc'             => 'paslēpt',
'thisisdeleted'       => 'Apskatīt vai atjaunot $1?',
'viewdeleted'         => 'Skatīt $1?',
'restorelink'         => '$1 {{PLURAL:$1|dzēsto versiju|dzēstās versijas}}',
'feedlinks'           => 'Barotne:',
'page-rss-feed'       => '"$1" RSS barotne',
'page-atom-feed'      => '"$1" Atom barotne',
'red-link-title'      => '$1 (vēl nav uzrakstīts)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Raksts',
'nstab-user'      => 'Lietotāja lapa',
'nstab-media'     => 'Multivides lapa',
'nstab-special'   => 'Īpašā lapa',
'nstab-project'   => 'Projekta lapa',
'nstab-image'     => 'Attēls',
'nstab-mediawiki' => 'paziņojums',
'nstab-template'  => 'Veidne',
'nstab-help'      => 'palīdzība',
'nstab-category'  => 'Kategorija',

# Main script and global functions
'nosuchaction'      => 'Šādas darbības nav.',
'nosuchactiontext'  => 'Wiki neatpazīst URL norādīto darbību',
'nosuchspecialpage' => 'Nav tādas īpašās lapas',
'nospecialpagetext' => 'Tu esi pieprasījis īpašo lapu, ko wiki neatpazīst.',

# General errors
'error'                => 'Kļūda',
'databaseerror'        => 'Datu bāzes kļūda',
'noconnect'            => 'Šajā wiki ir radušās tehniskas grūtības un nav iespējams savienoties ar datubāžu serveri. <br />
$1',
'cachederror'          => 'Šī ir lapas saglabātā versija, iespējams, ka tā nav atjaunināta.',
'laggedslavemode'      => 'Uzmanību: Iespējams, šajā lapā nav redzami nesen izdarītie papildinājumi.',
'readonly'             => 'Datubāze bloķēta',
'readonlytext'         => 'Datubāze šobrīd ir bloķēta pret jauniem ierakstiem un citām izmaiņām. Visdrīzāk iemesls ir parasts datubāzes uzturēšanas pasākums, pēc kura tā tiks atjaunota normālā stāvoklī. Administrators, kurš nobloķēja datubāzi, norādīja šādu iemeslu:
<p>$1',
'internalerror'        => 'Iekšēja kļūda',
'internalerror_info'   => 'Iekšējā kļūda: $1',
'filecopyerror'        => 'Nav iespējams nokopēt failu "$1" uz "$2"',
'filerenameerror'      => 'Neizdevās pārdēvēt failu "$1" par "$2".',
'filedeleteerror'      => 'Nevar izdzēst failu "$1".',
'filenotfound'         => 'Neizdevās atrast failu "$1".',
'formerror'            => 'Kļūda: neizdevās nosūtīt saturu',
'badarticleerror'      => 'Šo darbību nevar veikt šajā lapā.',
'cannotdelete'         => 'Nevar izdzēst norādīto lapu vai failu. (Iespējams, to jau ir izdzēsis kāds cits)',
'badtitle'             => 'Nepiemērots nosaukums',
'perfcached'           => 'Šie dati ir no servera kešatmiņas un var būt novecojuši:',
'perfcachedts'         => "Šie dati ir no servera kešatmiņas (''cache''), kas pēdējo reizi bija atjaunota $1.",
'querypage-no-updates' => 'Šīs lapas atjaunošana pagaidām ir atslēgta. Te esošie dati tuvākajā laikā netiks atjaunoti.',
'viewsource'           => 'Aplūkot kodu',
'viewsourcefor'        => 'Lapa: $1',
'protectedpagetext'    => 'Šī lapa ir aizsargāta lai novērstu tās izmainīšanu.',
'viewsourcetext'       => 'Tu vari apskatīties un nokopēt šīs lapas wikitekstu:',
'protectedinterface'   => 'Šī lapa satur programmatūras interfeisā lietotu tekstu un ir bloķēta pret izmaiņām, lai pasargātu no bojājumiem.',
'editinginterface'     => "'''Brīdinājums:''' Tu izmaini lapu, kuras saturu izmanto wiki programmatūras lietotāja saskarnē (''interfeisā''). Šīs lapas izmaiņas ietekmēs lietotāja saskarni citiem lietotājiem. Pēc modificēšanas, šīs izmaiņas būtu lietderīgi pievienot arī [http://translatewiki.net/wiki/Main_Page?setlang=en Betawiki], kas ir MediaWiki lokalizēšanas projekts.",
'namespaceprotected'   => "Tev nav tiesību izmainīt lapas, kas atrodas '''$1''' ''namespacē''.",
'customcssjsprotected' => "Tev nav tiesību izmainīt šo lapu, tāpēc, ka tā satur cita lietotāja personīgos uzstādījumus (''settings'').",
'ns-specialprotected'  => 'Nevar izmainīt īpašās lapas.',
'titleprotected'       => "Šī lapa ir aizsargāta pret izveidošanu. To aizsargāja [[User:$1|$1]].
Norādītais iemesls bija ''$2''.",

# Login and logout pages
'logouttitle'                => 'Lietotāja iziešana',
'logouttext'                 => "'''Tu esi izgājis no {{grammar:ģenitīvs|{{SITENAME}}}}.'''

Vari turpināt to izmantot anonīmi, vari [[Special:UserLogin|atgriezties]] kā cits lietotājs vai varbūt tas pats.
Ņem vērā, ka arī pēc iziešanas, dažas lapas var tikt parādītas tā, it kā tu vēl būtu iekšā, līdz tiks iztīrīta pārlūka kešatmiņa.",
'welcomecreation'            => '== Laipni lūdzam, $1! ==

Tavs lietotāja konts ir izveidots. Neaizmirsti, ka ir iespējams mainīt [[Special:Preferences|{{grammar:ģenitīvs|{{SITENAME}}}} izmantošanas izvēles]].',
'loginpagetitle'             => 'Lietotāja ieiešana',
'yourname'                   => 'Tavs lietotājvārds',
'yourpassword'               => 'Tava parole',
'yourpasswordagain'          => 'Atkārto paroli',
'remembermypassword'         => 'Atcerēties manu paroli pēc pārlūka aizvēršanas.',
'yourdomainname'             => 'Tavs domēns',
'externaldberror'            => 'Notikusi vai nu ārējās autentifikācijas datubāzes kļūda, vai arī tev nav atļauts izmainīt savu ārējo kontu.',
'loginproblem'               => '<b>Radās problēma ar ieiešanu.</b><br />Mēģini vēlreiz!',
'login'                      => 'Ieiet',
'nav-login-createaccount'    => 'Izveidot jaunu lietotāju vai doties iekšā',
'loginprompt'                => 'Lai ieietu {{grammar:lokatīvs|{{SITENAME}}}}, tavam datoram ir jāpieņem sīkdatnes (<i>cookies</i>).',
'userlogin'                  => 'Izveidot jaunu lietotāju vai doties iekšā',
'logout'                     => 'Iziet',
'userlogout'                 => 'Iziet',
'notloggedin'                => 'Neesi iegājis',
'nologin'                    => 'Nav lietotājvārda? $1.',
'nologinlink'                => 'Reģistrējies',
'createaccount'              => 'Izveidot jaunu lietotāju',
'gotaccount'                 => 'Tev jau ir lietotājvārds? $1!',
'gotaccountlink'             => 'Dodies iekšā',
'createaccountmail'          => 'pa e-pastu',
'badretype'                  => 'Tevis ievadītās paroles nesakrīt.',
'userexists'                 => 'Šāds lietotāja vārds jau eksistē. Izvēlies citu vārdu.',
'youremail'                  => 'Tava e-pasta adrese*',
'username'                   => 'Lietotājvārds:',
'uid'                        => 'Lietotāja ID:',
'yourrealname'               => 'Tavs īstais vārds*',
'yourlanguage'               => 'Lietotāja saskarnes valoda:',
'yournick'                   => 'Tavs paraksts (tāds kāds parādīsies uzrakstot 3~):',
'badsig'                     => "Kļūdains ''paraksta'' kods; pārbaudi HTML (ja tāds ir lietots).",
'badsiglength'               => 'Paraksts ir pārāk garš.
Tam ir jābūt īsākam par  $1 {{PLURAL:$1|simbolu|simboliem}}.',
'email'                      => 'E-pasts',
'prefs-help-realname'        => 'Īstais vārds nav obligāts.
Ja tu izvēlies to norādīt, šo lietos lai identificētu tavu darbu (ieguldījumu {{grammar:lokatīvs|{{SITENAME}}}}).',
'loginerror'                 => 'Neveiksmīga ieiešana',
'prefs-help-email'           => '* E-pasts nav obligāti jānorāda, taču nodrošina iespēju atsūtīt paroli, ja tu to esi aizmirsis. Šī iespēja arī ļauj citiem sazināties ar tevi, izmantojot tavu lietotāja lapu vai lietotāja diskusiju lapu, tev nekur neatklājot savu identitāti.',
'prefs-help-email-required'  => 'E-pasta adrese ir obligāta.',
'nocookiesnew'               => 'Lietotājvārds tika izveidots, bet tu neesi iegājis iekšā. {{SITENAME}} izmanto sīkdatnes (<i>cookies</i>), lai lietotāji varētu tajā ieiet. Tavs pārlūks nepieņem tās. Lūdzu, atļauj to pieņemšanu un tad nāc iekšā ar savu lietotājvārdu un paroli.',
'nocookieslogin'             => '{{SITENAME}} izmanto sīkdatnes (<i>cookies</i>), lai lietotāji varētu ieiet tajā. Diemžēl tavs pārlūks tos nepieņem. Lūdzu, atļauj to pieņemšanu un mēģini vēlreiz.',
'noname'                     => 'Tu neesi norādījis derīgu lietotāja vārdu.',
'loginsuccesstitle'          => 'Ieiešana veiksmīga',
'loginsuccess'               => 'Tu esi ienācis {{grammar:lokatīvs|{{SITENAME}}}} kā "$1".',
'nosuchuser'                 => 'Šeit nav lietotāja ar vārdu "$1". Pārbaudi, vai pareizi uzrakstīts, vai arī [[Special:Userlogin/signup|izveido jaunu kontu]].',
'nosuchusershort'            => 'Šeit nav lietotāja ar vārdu "<nowiki>$1</nowiki>". Pārbaudi, vai nav drukas kļūda.',
'nouserspecified'            => 'Tev jānorāda lietotājvārds.',
'wrongpassword'              => 'Tu ievadīji nepareizu paroli. Lūdzu, mēģini vēlreiz.',
'wrongpasswordempty'         => 'Parole bija tukša. Lūdzu mēģini vēlreiz.',
'passwordtooshort'           => 'Tava parole ir nederīga vai pārāk īsa. Tajā jābūt vismaz {{PLURAL:$1|1 zīmei|$1 zīmēm}} un jābūt atšķirīgai no tava lietotāja vārda.',
'mailmypassword'             => 'Atsūtīt man jaunu paroli',
'passwordremindertitle'      => 'Jauna pagaidu parole no {{SITENAME}}s',
'passwordremindertext'       => 'Kads (iespejams, Tu pats, no IP adreses $1)
ludza, lai nosutam Tev jaunu {{SITENAME}} ({{SERVER}}) ($4) paroli.
Lietotajam $2 parole tagad ir $3.
Ludzu, nomaini paroli, kad esi veiksmigi iekluvis ieksa.',
'noemail'                    => 'Lietotājs "$1" nav reģistrējis e-pasta adresi.',
'passwordsent'               => 'Esam nosūtījuši jaunu paroli uz e-pasta adresi, kuru ir norādījis lietotājs $1. Lūdzu, nāc iekšā ar jauno paroli, kad būsi to saņēmis.',
'blocked-mailpassword'       => "Tava IP adrese ir bloķēta un tāpēc nevar lietot paroles atjaunošanas (''recovery'') funkciju, lai nevarētu apiet bloku.",
'eauthentsent'               => "Apstiprinājuma e-pasts tika nosūtīts uz norādīto e-pasta adresi. Lai varētu saņemt citus ''meilus'', izpildi vēstulē norādītās instrukcijas, lai apstiprinātu, ka šī tiešām ir tava e-pasta adrese.",
'throttled-mailpassword'     => 'Paroles atgādinājums jau ir ticis nosūtīts {{PLURAL:$1|pēdējās stundas|pēdējo $1 stundu}} laikā.
Lai novērstu šīs funkcijas ļaunprātīgu izmantošanu, iespējams nosūtīt tikai vienu paroles atgādinājumu, {{PLURAL:$1|katru stundu|katras $1 stundas}}.',
'mailerror'                  => 'E-pasta sūtīšanas kļūda: $1',
'acct_creation_throttle_hit' => 'Tu jau esi izveidojis $1 kontus. Vairāk nevar.',
'emailauthenticated'         => 'Tava e-pasta adrese tika apstiprināta $1.',
'emailnotauthenticated'      => 'Tava e-pasta adrese <strong>vēl nav apstiprināta</strong> un zemāk norādītās iespējas nav pieejamas.',
'noemailprefs'               => '<strong>Norādi e-pasta adresi, lai lietotu šīs iespējas.</strong>',
'emailconfirmlink'           => 'Apstiprināt tavu e-pasta adresi',
'invalidemailaddress'        => 'E-pasta adrese nevar tikt apstiprināta, jo izskatās nederīga. Lūdzu ievadi korekti noformētu e-pasta adresi, vai arī atstāj to lauku tukšu.',
'accountcreated'             => 'Konts izveidots',
'accountcreatedtext'         => 'Lietotāja konts priekš $1 tika izveidots.',
'createaccount-title'        => 'Lietotāja konta izveidošana {{grammar:lokatīvs|{{SITENAME}}}}',
'loginlanguagelabel'         => 'Valoda: $1',

# Edit page toolbar
'bold_sample'     => 'Teksts boldā',
'bold_tip'        => 'Teksts boldā',
'italic_sample'   => 'Teksts kursīvā',
'italic_tip'      => 'Teksts kursīvā',
'link_sample'     => 'Lapas nosaukums',
'link_tip'        => 'Iekšējā saite',
'extlink_sample'  => 'http://www.example.com saites apraksts',
'extlink_tip'     => 'Ārējā saite (neaizmirsti sākumā pierakstīt "http://")',
'headline_sample' => 'Virsraksta teksts',
'headline_tip'    => '2. līmeņa virsraksts',
'math_sample'     => 'Šeit ievieto formulu',
'math_tip'        => 'Matemātikas formula (LaTeX)',
'nowiki_sample'   => 'Šeit raksti neformatētu tekstu',
'nowiki_tip'      => 'Ignorēt wiki formatējumu',
'image_sample'    => 'Piemers.jpg',
'image_tip'       => 'Ievietots attēls',
'media_sample'    => 'Piemers.ogg',
'media_tip'       => 'Saite uz multimēdiju failu',
'sig_tip'         => 'Tavs paraksts ar laika atzīmi',
'hr_tip'          => 'Horizontāla līnija (neizmanto lieki)',

# Edit pages
'summary'                   => 'Kopsavilkums',
'subject'                   => 'Tēma/virsraksts',
'minoredit'                 => 'maznozīmīgs labojums',
'watchthis'                 => 'uzraudzīt',
'savearticle'               => 'Saglabāt lapu',
'preview'                   => 'Pirmskats',
'showpreview'               => 'Rādīt pirmskatu',
'showlivepreview'           => 'Tūlītējs pirmskats',
'showdiff'                  => 'Rādīt izmaiņas',
'anoneditwarning'           => "'''Uzmanību:''' tu neesi iegājis. Lapas hronoloģijā tiks ierakstīta tava IP adrese.",
'missingsummary'            => "'''Atgādinājums''': Tu neesi norādījis izmaiņu kopsavilkumu. Vēlreiz klikšķinot uz \"Saglabāt lapu\", Tavas izmaiņas tiks saglabātas bez kopsavilkuma.",
'missingcommenttext'        => 'Lūdzu, ievadi tekstu zemāk redzamajā logā!',
'missingcommentheader'      => "'''Atgādinājums:''' Tu šim komentāram neesi norādījis virsrakstu/tematu.
Ja tu vēlreiz uzspiedīsi uz Saglabāt, tavas izmaiņas tiks saglabātas bez tā virsraksta.",
'summary-preview'           => 'Kopsavilkuma pirmskats',
'blockedtitle'              => 'Lietotājs ir bloķēts.',
'blockedtext'               => "<big>'''Tavs lietotāja vārds vai IP adrese ir nobloķēta.'''</big>

\$1 nobloķēja tavu lietotāja vārdu vai IP adresi.
Bloķējot norādītais iemesls bija: ''\$2''.

*Bloka sākums: \$8
*Bloka beigas: \$6
*Ar šo mēģināja nobloķēt: \$7

Tu vari sazināties ar \$1 vai kādu citu [[{{MediaWiki:Grouppage-sysop}}|administratoru]] lai apspriestu šo bloku.

Pievērs uzmanību, tam, ka ja tu neesi norādījis derīgu e-pasta adresi ''[[Special:Preferences|manās izvēlēs]]'', tev nedarbosies \"sūtīt e-pastu\" iespēja.

Tava IP adrese ir \$3 un bloka identifikators ir #\$5. Lūdzu iekļauj vienu no tiem, vai abus, visos turpmākajos pieprasījumos.",
'autoblockedtext'           => 'Tava IP adrese ir tikusi automātiski nobloķēta, tāpēc, ka to (nupat kā) ir lietojis cits lietotājs, kuru nobloķēja $1.
Norādītais bloķēšanas iemesls bija:

:\'\'$2\'\'

* Bloka sākums: $8
* Bloka beigas: $6
* Bija domāts nobloķēt: $7

Tu vari sazināties ar $1 vai kādu citu [[{{MediaWiki:Grouppage-sysop}}|adminu]] lai apspriestu šo bloku.

Atceries, ka tu nevari lietot "sūtīt e-pastu šim lietotājam" iespēju, ja tu neesi norādījis derīgu e-pasta adresi savās [[Special:Preferences|lietotāja izvelēs]] un bloķējot tev nav aizbloķēta iespēja sūtīt e-pastu.

Tava pašreizējā IP adrese ir $3 un  bloka ID ir $5.
Lūdzu iekļauj šos visos ziņojumos, kurus sūti adminiem, apspriežot šo bloku.',
'whitelistedittitle'        => 'Lai varētu rediģēt, šeit jāielogojas.',
'whitelistedittext'         => 'Tev $1 lai varētu rediģēt lapas.',
'loginreqtitle'             => 'Nepieciešama ieiešana',
'loginreqlink'              => 'login',
'accmailtitle'              => 'Parole izsūtīta.',
'accmailtext'               => '$1 parole tika nosūtīta uz $2.',
'newarticle'                => '(Jauns raksts)',
'newarticletext'            => "Tu šeit nonāci sekojot saitei uz, pagaidām vēl neuzrakstītu, lapu.
Lai izveidotu lapu, sāc rakstīt teksta logā apakšā (par teksta formatēšanu un sīkākai informācija skatīt [[{{MediaWiki:Helppage}}|palīdzības lapu]]).
Ja tu šeit nonāci kļūdas pēc, vienkārši uzspied '''back''' pogu pārlūkprogrammā.",
'anontalkpagetext'          => "----''Šī ir diskusiju lapa anonīmam lietotājam, kurš vēl nav kļuvis par reģistrētu lietotāju vai arī neizmanto savu lietotājvārdu. Tādēļ mums ir jāizmanto skaitliskā IP adrese, lai viņu identificētu.
Šāda IP adrese var būt vairākiem lietotājiem.
Ja tu esi anonīms lietotājs un uzskati, ka tev ir adresēti neatbilstoši komentāri, lūdzu, [[Special:UserLogin/signup|kļūsti par lietotāju]] vai arī [[Special:UserLogin|izmanto jau izveidotu lietotājvārdu]], lai izvairītos no turpmākām neskaidrībām un tu netiktu sajaukts ar citiem anonīmiem lietotājiem.''",
'noarticletext'             => 'Šajā lapā šobrīd nav nekāda teksta, tu vari [[Special:Search/{{PAGENAME}}|meklēt citās lapās pēc šīs lapas nosaukuma]], vai arī [{{fullurl:{{FULLPAGENAME}}|action=edit}} sākt rediģēt šo lapu].',
'userpage-userdoesnotexist' => 'Lietotājs "$1" nav reģistrēts.
Lūdzu, pārliecinies vai vēlies izveidot/izmainīt šo lapu.',
'clearyourcache'            => "'''Piezīme - Pēc saglabāšanas, lai būtu redzamas izmaiņas, var būt nepieciešamas iztīrīt pārlūka kešatmiņu.''' '''Mozilla / Firefox / Safari:''' turi nospiestu ''Shift'' un klikšķini ''Reload,'' vai arī spied ''Ctrl-F5'' vai ''Ctrl-R'' (''Command-R'' uz Macintosh); '''Konqueror: '''klikšķini ''Reload'' vai spied uz ''F5;'' '''Opera:''' kešu var iztīrīt ''Tools → Preferences;'' '''Internet Explorer:''' turi nospiestu ''Ctrl'' un klikšķini ''Refresh,'' vai spied ''Ctrl-F5.''",
'usercssjsyoucanpreview'    => '<strong>Ieteikums:</strong> Lieto pirmsskata pogu, lai pārbaudītu savu jauno CSS/JS pirms saglabāšanas.',
'usercsspreview'            => "'''Atceries, ka šis ir tikai tava lietotāja CSS pirmskats, lapa vēl nav saglabāta!'''",
'userjspreview'             => "'''Atceries, ka šis ir tikai tava lietotāja JavaScript pirmskats/tests, lapa vēl nav saglabāta!'''",
'updated'                   => '(Atjaunots)',
'note'                      => '<strong>Piezīme: </strong>',
'previewnote'               => '<strong>Atceries, ka šis ir tikai pirmskats un vēl nav saglabāts!</strong>',
'session_fail_preview'      => '<strong>Neizdevās apstrādāt tavas izmaiņas, jo tika pazaudēti sesijas dati.
Lūdzu mēģini vēlreiz.
Ja tas joprojām nedarbojas, mēģini [[Special:UserLogout|izlogoties ārā]] un ielogoties no jauna.</strong>',
'session_fail_preview_html' => "<strong>Neizdevās apstrādāt tavas izmaiņas, jo tika pazaudēti sesijas dati.</strong>

''Tā, kā {{grammar:ģenitīvs|{{SITENAME}}}} darbojas neapstrādāts HTML, pirmskats ir paslēpts, lai aizsargātos no JavaScripta  uzbrukumiem.''

<strong>Ja šis bija parasts rediģēšanas mēģinājums, mēģini vēlreiz.
Ja tas joprojām nedarbojas, mēģini [[Special:UserLogout|izlogoties ārā]] un ielogoties no jauna.</strong>",
'editing'                   => 'Izmainīt $1',
'editingsection'            => 'Izmainīt $1 (sadaļa)',
'editingcomment'            => 'Izmainīt $1 (komentārs)',
'editconflict'              => 'Izmaiņu konflikts: $1',
'explainconflict'           => "Kāds cits ir izmainījis šo lapu pēc tam, kad tu sāki to mainīt.
Augšējā teksta logā ir lapas teksts tā pašreizējā versijā.
Tevis veiktās izmaiņas ir redzamas apakšējā teksta logā.
Lai saglabātu savas izmaiņas, tev ir jāapvieno savs teksts ar saglabāto pašreizējo variantu.
Kad spiedīsi pogu \"Saglabāt lapu\", tiks saglabāts '''tikai''' teksts, kas ir augšējā teksta logā.",
'yourtext'                  => 'Tavs teksts',
'storedversion'             => 'Saglabātā versija',
'nonunicodebrowser'         => '<strong>Brīdinājums: Tavs pārlūks neatbalsta unikodu.
Ir pieejams risinājums, kas ļaus tev droši rediģēt lapas: zīmes, kas nav ASCII, parādīsies izmaiņu logā kā heksadecimāli kodi.</strong>',
'editingold'                => '<strong>BRĪDINĀJUMS: Saglabājot šo lapu, tu izmainīsi šīs lapas novecojušu versiju, un ar to tiks dzēstas visas izmaiņas, kas izdarītas pēc šīs versijas.</strong>',
'yourdiff'                  => 'Atšķirības',
'copyrightwarning'          => 'Lūdzu, ņem vērā, ka viss ieguldījums, kas veikts {{grammar:lokatīvs|{{SITENAME}}}}, ir uzskatāms par publiskotu saskaņā ar $2 (vairāk info skat. $1).
Ja nevēlies, lai Tevis rakstīto kāds rediģē un izplata tālāk, tad, lūdzu, nepievieno to šeit!<br />

Izvēloties "Saglabāt lapu", Tu apliecini, ka šo rakstu esi rakstījis vai papildinājis pats vai izmantojis informāciju no darba, ko neaizsargā autortiesības, vai tamlīdzīga brīvi pieejama resursa.
<strong>BEZ ATĻAUJAS NEPIEVIENO DARBU, KO AIZSARGĀ AUTORTIESĪBAS!</strong>',
'copyrightwarning2'         => "Lūdz ņem vērā, ka visu ieguldījumu {{grammar:lokatīvs|{{SITENAME}}}} var rediģēt, mainīt vai izdzēst citi lietotāji. Ja negribi lai ar tavu rakstīto tā izrīkojas, nepievieno to šeit.

Tu apliecini, ka šo rakstu esi rakstījis vai papildinājis pats vai izmantojis informāciju no darba, ko neaizsargā autortiesības, vai tamlīdzīga brīvi pieejama resursa (sīkāk skatīt $1).

'''BEZ ATĻAUJAS NEPIEVIENO DARBU, KO AIZSARGĀ AUTORTIESĪBAS!'''",
'longpagewarning'           => '<strong>Šī lapa ir $1 kilobaitus liela. Tas var būt vairāk par lapas optimālo izmēru. Lūdzu apsver iespēju sašķelt to mazākās sekcijās.</strong>',
'longpageerror'             => '<strong>Kļūda: Teksts, kuru tu mēģināji saglabāt, ir $1 kilobaitus garš, kas ir vairāk nekā pieļaujamie $2 kilobaiti.
Tas nevar tikt saglabāts.</strong>',
'readonlywarning'           => '<strong>Brīdinājums: Datubāze ir slēgta apkopei, tāpēc tu tagad nevarēsi saglabāt veiktās izmaiņas.
Tu varētu nokopēt tekstu un saglabāt kā teksta failu vēlākam laikam.</strong>',
'protectedpagewarning'      => "'''BRĪDINĀJUMS: Šī lapa ir bloķēta pret izmaiņām, tikai lietotāji ar admina privilēģijām var to izmainīt. To darot, noteikti ievēro [[Project:Norādījumi par aizsargātajām lapām|norādījumus par aizsargātajām lapām]].'''",
'semiprotectedpagewarning'  => "'''Piezīme:''' Izmaiņu veikšana šajā lapā ir atļauta tikai reģistrētiem lietotājiem.",
'titleprotectedwarning'     => '<strong>Brīdinājums: Šī lapa ir slēgta un to var izveidot tikai noteikti lietotāji.</strong>',
'templatesused'             => '<br />Šajā lapā izmantotās veidnes:',
'templatesusedpreview'      => 'Šajā pirmskatā izmantotās veidnes:',
'templatesusedsection'      => 'Šajā sadaļā izmantotās veidnes:',
'template-protected'        => '(aizsargāta)',
'template-semiprotected'    => '(daļēji aizsargāta)',
'hiddencategories'          => 'Šī lapa ietilpst {{PLURAL:$1|1 slēptajā kategorijā|$1 slēptajās kategorijās}}:',
'nocreatetext'              => '{{grammar:lokatīvs|{{SITENAME}}}} ir atslēgta iespēja izveidot jauinas lapas.
Tu vari atgriezties atpakaļ un izmainīt esošu lapu, vai arī [[Special:UserLogin|ielogoties, vai izveidot kontu]].',
'recreate-deleted-warn'     => "'''Brīdinājums: Tu atjauno lapu, kas ir tikusi izdzēsta'''

Tev vajadzētu pārliecināties, vai ir lietderīgi turpināt izmainīt šo lapu.
Te var apskatīties dzēšanas reģistru, kurā jābūt datiem par to kas, kad un kāpēc šo lapu izdzēsa.",

# "Undo" feature
'undo-success' => 'Šo izmaiņu var atcellt.
Lūdzu, pārbaudi zemāk redzamajā salīdzinājumā vai tu to tiešām vēlies darīt un pēc tam saglabā izmaiņas, lai to atceltu.',
'undo-norev'   => 'Šo izmaiņu nevar atcelt, jo tādas nav vai tā ir izdzēsta.',
'undo-summary' => 'Atcēlu [[Special:Contributions/$2|$2]] ([[User talk:$2|Diskusija]]) izdarīto izmaiņu $1',

# Account creation failure
'cantcreateaccounttitle' => 'Nevar izveidot lietotāju',
'cantcreateaccount-text' => "[[Lietotājs:$3|$3]] ir bloķējis lietotāja izveidošanu no šīs IP adreses ('''$1''').

$3 norādītais iemesls ir ''$2''",

# History pages
'viewpagelogs'        => 'Apskatīties ar šo lapu saistītos reģistru ierakstus',
'nohistory'           => 'Šai lapai nav pieejama versiju hronoloģija.',
'revnotfound'         => 'Versija nav atrasta',
'revnotfoundtext'     => 'Meklētā vecā lapas versija netika atrasta. Lūdzu pārbaudi lietoto URL.',
'currentrev'          => 'Pašreizējā versija',
'revisionasof'        => 'Versija, kas saglabāta $1',
'revision-info'       => 'Versija $1 laikā, kādu to atstāja $2',
'previousrevision'    => '←Senāka versija',
'nextrevision'        => 'Jaunāka versija→',
'currentrevisionlink' => 'skatīt pašreizējo versiju',
'cur'                 => 'ar pašreizējo',
'next'                => 'nākamais',
'last'                => 'ar iepriekšējo',
'page_first'          => 'pirmā',
'page_last'           => 'pēdējā',
'histlegend'          => 'Atšķirību izvēle: atzīmē vajadzīgo versiju apaļās pogas un spied "Salīdzināt izvēlētās versijas".<br />
Apzīmējumi:
"ar pašreizējo" = salīdzināt ar pašreizējo versiju,
"ar iepriekšējo" = salīdzināt ar iepriekšējo versiju,
m = maznozīmīgs labojums.',
'deletedrev'          => '[izdzēsta]',
'histfirst'           => 'Senākās',
'histlast'            => 'Jaunākās',
'historysize'         => '({{PLURAL:$1|1 baits|$1 baiti}})',
'historyempty'        => '(tukša)',

# Revision feed
'history-feed-title'          => 'Versiju hronoloģija',
'history-feed-description'    => 'Šīs wiki lapas versiju hronoloģija',
'history-feed-item-nocomment' => '$1 : $2', # user at time

# Revision deletion
'rev-deleted-comment' => '(komentārs nodzēsts)',
'rev-deleted-user'    => '(lietotāja vārds nodzēsts)',
'rev-delundel'        => 'rādīt/slēpt',

# Diffs
'history-title'           => '"$1" versiju hronoloģija',
'difference'              => '(Atšķirības starp versijām)',
'lineno'                  => '$1. rindiņa:',
'compareselectedversions' => 'Salīdzināt izvēlētās versijas',
'editundo'                => 'atcelt',
'diff-multi'              => '({{PLURAL:$1|Viena starpversija nav parādīta|$1 starpversijas nav parādītas}}.)',

# Search results
'searchresults'         => 'Meklēšanas rezultāti',
'searchresulttext'      => 'Lai iegūtu vairāk informācijas par meklēšanu {{grammar:akuzatīvs|{{SITENAME}}}}, skat. [[{{MediaWiki:Helppage}}|{{grammar:ģenitīvs|{{SITENAME}}}} meklēšana]].',
'searchsubtitle'        => 'Pieprasījums: \'\'\'[[:$1]]\'\'\' ([[Special:Prefixindex/$1|visas lapas, kas sākas ar "$1"]] | [[Special:WhatLinksHere/$1|visas lapas, kurās ir saite uz "$1"]])',
'searchsubtitleinvalid' => 'Pieprasījums: $1',
'noexactmatch'          => "'''Lapas ar nosaukumu \"\$1\" šeit nav.''' Tu vari to [[:\$1|izveidot]].",
'noexactmatch-nocreate' => "'''Šeit nav lapas ar nosaukumu \"\$1\".'''",
'toomanymatches'        => 'Tika atgriezti poārāk daudzi rezultāti, lūdzu pamēģini citādāku pieprasījumu',
'titlematches'          => 'Rezultāti virsrakstos',
'notitlematches'        => 'Neviena rezultāta, meklējot lapas virsrakstā',
'textmatches'           => 'Rezultāti lapu tekstos',
'notextmatches'         => 'Neviena rezultāta, meklējot lapas tekstā',
'prevn'                 => 'iepriekšējās $1',
'nextn'                 => 'nākamās $1',
'viewprevnext'          => 'Skatīt ($1) ($2) ($3 vienā lapā).',
'showingresults'        => 'Šobrīd ir redzamas <b>$1</b> {{PLURAL:$1|lapa|lapas}}, sākot ar #<b>$2</b>.',
'showingresultsnum'     => "Šobrīd ir redzamas '''$3''' {{PLURAL:$3|lapa|lapas}}, sākot ar #'''$2'''>.",
'showingresultstotal'   => "Rāda {{PLURAL:$3|rezultātu '''$1''' no '''$3'''|rezultātus '''$1 - $2''' no '''$3'''}}",
'nonefound'             => "'''Piezīme:''' bieži vien meklēšana ir neveiksmīga, meklējot plaši izplatītus vārdus, piemēram, \"un\" vai \"ir\", jo tie netiek iekļauti meklēšanas datubāzē, vai arī meklējot vairāk par vienu vārdu (jo rezultātos parādīsies tikai lapas, kurās ir visi meklētie vārdi). Vēl, pēc noklusējuma, pārmeklē tikai dažas ''namespaces''. Lai meklētu visās, meklēšanas pieprasījumam priekšā jāieliek ''all:'', vai arī analogā veidā jānorāda pārmeklējamo ''namespaci''.",
'powersearch'           => 'Izvērstā meklēšana',
'powersearch-legend'    => 'Izvērstā meklēšana',
'searchdisabled'        => '<p style="margin: 1.5em 2em 1em">Meklēšana {{grammar:lokatīvs|{{SITENAME}}}} šobrīd ir atslēgta darbības traucējumu dēļ. Pagaidām vari meklēt, izmantojot Google vai Yahoo.
<span style="font-size: 89%; display: block; margin-left: .2em">Ņem vērā, ka meklētāju indeksētais {{grammar:ģenitīvs|{{SITENAME}}}} saturs var būt novecojis.</span></p>',

# Preferences page
'preferences'           => 'Izvēles',
'mypreferences'         => 'manas izvēles',
'prefs-edits'           => 'Izmaiņu skaits:',
'prefsnologin'          => 'Neesi iegājis',
'prefsnologintext'      => 'Tev jābūt <span class="plainlinks">[{{fullurl:Special:Userlogin|returnto=$1}} iegājušam], lai mainītu lietotāja izvēles.',
'prefsreset'            => 'Sākotnējās izvēles ir atjaunotas.',
'qbsettings'            => 'Rīku joslas stāvoklis',
'changepassword'        => 'Mainīt paroli',
'skin'                  => 'Apdare',
'math'                  => 'Formulas',
'dateformat'            => 'Datuma formāts',
'datedefault'           => 'Vienalga',
'datetime'              => 'Datums un laiks',
'math_unknown_error'    => 'nezināma kļūda',
'math_unknown_function' => 'nezināma funkcija',
'math_syntax_error'     => 'sintakses kļūda',
'prefs-personal'        => 'Lietotāja dati',
'prefs-rc'              => 'Pēdējās izmaiņas',
'prefs-watchlist'       => 'Uzraugāmie raksti',
'prefs-watchlist-days'  => 'Dienu skaits, kuras parādīt uzraugāmo rakstu sarakstā:',
'prefs-watchlist-edits' => 'Izmaiņu skaits, kuras rādīt izvērstajā uzraugāmo rakstu sarakstā:',
'prefs-misc'            => 'Dažādi',
'saveprefs'             => 'Saglabāt izvēles',
'resetprefs'            => 'Atcelt nesaglabātās izmaiņas',
'oldpassword'           => 'Vecā parole',
'newpassword'           => 'Jaunā parole',
'retypenew'             => 'Atkārto jauno paroli',
'textboxsize'           => 'Rediģēšana',
'rows'                  => 'Rindiņas',
'columns'               => 'Simbolu skaits rindiņā',
'searchresultshead'     => 'Meklēšana',
'resultsperpage'        => 'Lappusē parādāmo rezultātu skaits',
'contextlines'          => 'Cik rindiņas parādīt katram atrastajam rezultātam',
'contextchars'          => 'Konteksta simbolu skaits vienā rindiņā',
'recentchangesdays'     => 'Dienu skaits, kuru rādīt pēdējajās izmaiņās:',
'recentchangescount'    => 'Virsrakstu skaits pēdējo izmaiņu, hronoloģiju un reģistru lapās:',
'savedprefs'            => 'Tavas izvēles ir saglabātas.',
'timezonelegend'        => 'Laika josla',
'timezonetext'          => '¹Ieraksti, par cik stundām tavs vietējais laiks atšķiras no servera laika (UTC).',
'localtime'             => 'Attēlotais vietējais laiks',
'timezoneoffset'        => 'Starpība¹',
'servertime'            => 'Servera laiks šobrīd',
'guesstimezone'         => 'Izmantot datora sistēmas laiku',
'allowemail'            => 'Atļaut saņemt e-pastus no citiem lietotājiem.',
'prefs-searchoptions'   => 'Meklēšanas opcijas',
'defaultns'             => 'Meklēt šajās palīglapās pēc noklusējuma:',
'default'               => 'pēc noklusējuma',
'files'                 => 'Attēli',

# User rights
'userrights'                  => 'Lietotāju tiesību pārvaldība', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => 'Pārvaldīt lietotāja grupas',
'userrights-user-editname'    => 'Ievadi lietotājvārdu:',
'editusergroup'               => 'Izmainīt lietotāja grupas',
'editinguser'                 => "Izmainīt lietotāja '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]]) statusu",
'userrights-editusergroup'    => 'Izmainīt lietotāja grupas',
'saveusergroups'              => 'Saglabāt lietotāja grupas',
'userrights-groupsmember'     => 'Šobrīd ietilpst grupās:',
'userrights-groups-help'      => 'Tu vari izmainīt kādās grupās šis lietotājs ir:
* Ieķeksēts lauciņš noāda, ka lietotājs ir attiecīgajā grupā.
* Neieķeksēts lauciņš norāda, ka lietotājs nav attiecīgajā grupā.
* * norāda, ka šo grupu tu nevarēsi noņemt, pēc tam, kad to būsi pielicis, vai otrādāk (tu nevarēsi atcelt savas izmaiņas).',
'userrights-reason'           => 'Izmaiņas iemesls:',
'userrights-no-interwiki'     => 'Tev nav tiesību izmainīt lietotāju tiesības citos wiki.',
'userrights-nodatabase'       => 'Datubāze $1 neeksistē vai nav lokāla.',
'userrights-nologin'          => 'Tev ir [[Special:UserLogin|jāieiet iekšā]] kā adminam, lai varētu izmainīt lietotāju grupas.',
'userrights-notallowed'       => 'Tavam lietotājvārdam nav tiesību izmainīt lietotāju grupas.',
'userrights-changeable-col'   => 'Grupas, kuras tu vari izmainīt',
'userrights-unchangeable-col' => 'Grupas, kuras tu nevari izmainīt',

# Groups
'group'            => 'Grupa:',
'group-user'       => 'Lietotāji',
'group-bot'        => 'Boti',
'group-sysop'      => 'Administratori',
'group-bureaucrat' => 'Birokrāti',

'group-user-member'       => 'Lietotājs',
'group-bot-member'        => 'Bots',
'group-sysop-member'      => 'Administrators',
'group-bureaucrat-member' => 'Birokrāts',

# Rights
'right-read'             => 'Lasīt lapas',
'right-edit'             => 'Izmainīt lapas',
'right-createpage'       => 'Izveidot lapas (kuras nav diskusiju lapas)',
'right-createtalk'       => 'Izveidot diskusiju lapas',
'right-createaccount'    => 'Izveidot jaunus lietotāja kontus',
'right-minoredit'        => 'Atzīmēt izmaiņas kā maznozīmīgas',
'right-move'             => 'Pārvietot lapas',
'right-move-subpages'    => 'Pārvietot lapas kopā ar to apakšlapām',
'right-suppressredirect' => 'Neveidot pāradresāciju no vecā nosaukuma, pārvietojot lapu',
'right-upload'           => 'Augšuplādēt failus',
'right-reupload'         => 'Pārrakstīt esošu failu',
'right-reupload-own'     => 'Pārrakstīt paša augšuplādētu esošu failu',
'right-upload_by_url'    => 'Augšuplādēt failu no URL',
'right-autoconfirmed'    => 'Izmainīt daļēji aizsargātas lapas',
'right-delete'           => 'Dzēst lapas',
'right-bigdelete'        => 'Dzēst lapas ar lielām hronoloģijām',
'right-deleterevision'   => 'Dzēst un atjaunot lapu noteiktas versijas',
'right-deletedhistory'   => 'Skatīt izdzēstos hronoloģijas ierakstus, bez tiem piesaistītā teksta',
'right-undelete'         => 'Atjaunot lapu',
'right-suppressrevision' => 'Apskatīt un atjaunot versijas, kas paslēptas no adminiem',
'right-block'            => 'Bloķēt citus lietotājus (lapu izmainīšana)',
'right-blockemail'       => 'Bloķēt citus lietotājus (iespēja sūtīt e-pastu)',
'right-ipblock-exempt'   => 'Apiet IP blokus, autoblokus un IP apgabalu blokus',
'right-proxyunbannable'  => "Apiet ''proxy'' automātiskos blokus",
'right-protect'          => 'Izmainīt aizsargātās lapas un to aizsardzības līmeni',
'right-editinterface'    => 'Izmainīt lietotāja interfeisu',
'right-editusercssjs'    => 'Izmainīt citu lietotāju CSS un JS failus',
'right-import'           => 'Importēt lapas no citiem wiki',
'right-importupload'     => 'Importēt lapas no failu augšuplādes',

# User rights log
'rightslog'      => 'Lietotāju tiesību reģistrs',
'rightslogtext'  => 'Šis ir lietotāju tiesību izmaiņu reģistrs.',
'rightslogentry' => 'izmainīja $1 grupas no $2 uz $3',

# Recent changes
'nchanges'                          => '$1 {{PLURAL:$1|izmaiņa|izmaiņas}}',
'recentchanges'                     => 'Pēdējās izmaiņas',
'recentchangestext'                 => 'Šajā lapā ir šitajā viki izdarītās pēdējās izmaiņas.',
'rcnote'                            => 'Šobrīd ir {{PLURAL:$1|redzama pēdējā <strong>$1</strong> izmaiņa, kas izdarīta|redzamas pēdējās <strong>$1</strong> izmaiņas, kas izdarītas}} {{PLURAL:$2|pēdējā|pēdējās}} <strong>$2</strong> {{PLURAL:$2|dienā|dienās}} (līdz $4, $5).',
'rcnotefrom'                        => "Šobrīd redzamas izmaiņas kopš '''$2''' (parādītas ne vairāk par '''$1''').",
'rclistfrom'                        => 'Parādīt jaunas izmaiņas kopš $1',
'rcshowhideminor'                   => '$1 maznozīmīgus',
'rcshowhidebots'                    => '$1 botus',
'rcshowhideliu'                     => '$1 reģistrētos',
'rcshowhideanons'                   => '$1 anonīmos',
'rcshowhidemine'                    => '$1 manus',
'rclinks'                           => 'Parādīt pēdējās $1 izmaiņas {{PLURAL:$2|pēdējā|pēdējās}} $2 {{PLURAL:$2|dienā|dienās}}.<br />$3',
'diff'                              => 'izmaiņas',
'hist'                              => 'hronoloģija',
'hide'                              => 'paslēpt',
'show'                              => 'parādīt',
'minoreditletter'                   => 'm',
'newpageletter'                     => 'J',
'boteditletter'                     => 'b',
'number_of_watching_users_pageview' => '[šo lapu uzrauga $1 {{PLURAL:$1|lietotājs|lietotāji}}]',
'newsectionsummary'                 => '/* $1 */ jauna sadaļa',

# Recent changes linked
'recentchangeslinked'          => 'Saistītās izmaiņas',
'recentchangeslinked-title'    => 'Izmaiņas, kas saistītas ar "$1"',
'recentchangeslinked-noresult' => 'Norādītajā laika periodā saistītajās lapās izmaiņu nebija.',
'recentchangeslinked-summary'  => "Šiet ir nesen izdarītās izmaiņas lapās, uz kurām ir saites no norādītās lapas (vai norādītajā kategorijā ietilpstošās lapas).
Lapas, kas ir tavā [[Special:Watchlist|uzraugāmo rakstu sarakstā]] ir '''treknas'''.",
'recentchangeslinked-page'     => 'Lapas nosaukums:',
'recentchangeslinked-to'       => 'Rādīt izmaiņas lapās, kurās ir saites uz šo lapu (nevs lapās uz kurām ir saites no šīs lapas)',

# Upload
'upload'                 => 'Augšuplādēt failu',
'uploadbtn'              => 'Augšuplādēt',
'reupload'               => 'Vēlreiz augšuplādēt',
'reuploaddesc'           => 'Atcelt augšupielādi un atgriezties pie augšupielādes veidnes.',
'uploadnologin'          => 'Neesi iegājis',
'uploadnologintext'      => 'Tev jābūt [[Special:UserLogin|iegājušam]], lai augšuplādētu failus.',
'uploaderror'            => 'Augšupielādes kļūda',
'uploadtext'             => "'''STOP!''' Pirms tu kaut ko augšupielādē, noteikti izlasi un ievēro [[Project:Attēlu izmantošanas noteikumi|attēlu izmantošanas noteikumus]].

Lai aplūkotu vai meklētu agrāk augšuplādētus attēlus,
dodies uz [[Special:ImageList|augšupielādēto attēlu sarakstu]].
Augšupielādes un dzēšanas tiek reģistrētas [[Special:Log/upload|augšupielādes reģistrā]].

Izmanto šo veidni, lai augšupielādētu jaunus attēlu failus, ar kuriem ilustrēt tevis izmainītās lapas.
Gandrīz visos pārlūkos tev vajadzētu redzēt pogu '''\"Choose...\",''' kuru spiežot parādīsies faila atvēršanas dialogs.
Izvēloties kādu failu, tā adrese parādīsies ailītē blakus šai pogai.
Tev ir arī jāatzīmē ailīte, kas apstiprina, ka tu nepārkāp nekādas autortiesības, augšupielādējot šo failu.
Spied pogu '''Augšuplādēt''', lai pabeigtu augšupielādi.
Tas var ieilgt, ja tavs interneta pieslēgums ir lēns.

Ieteicamie formāti ir:
* JPEG - ja tā ir fotogrāfija, 
* PNG - ja tas ir zīmējums vai kāda ikona, un 
* OGG - ja tas ir skaņas fails.

Lūdzu, pārliecinies, ka faila nosaukums ir pietiekami aprakstošs, lai izvairītos no neskaidrībām. Lai attēlu pēc tam ievietotu kādā lapā, izmanto šādi noformētu linkus:
* '''<nowiki>[[</nowiki>{{ns:image}}<nowiki>:Fails.jpg|paskaidrojošs teksts]]</nowiki>'''
* '''<nowiki>[[</nowiki>{{ns:image}}<nowiki>:Fails.png|paskaidrojošs teksts]]</nowiki>'''
vai skaņām
* '''<nowiki>[[</nowiki>{{ns:media}}<nowiki>:Fails.ogg]]</nowiki>'''

Lūdzu, ņem vērā, ka tāpat kā citas wiki lapas arī tevis augšuplādētos failus citi var mainīt vai dzēst, ja uzskata, ka tas nāktu par labu šim projektam, kā arī atceries, ka tev var tikt liegta augšupielādes iespēja, ja tu šo sistēmu.",
'upload-permitted'       => 'Atļautie failu tipi: $1.',
'upload-preferred'       => 'Ieteicamie failu tipi: $1.',
'upload-prohibited'      => 'Aizliegtie failu tipi: $1.',
'uploadlog'              => 'augšupielādes reģistrs',
'uploadlogpage'          => 'Augšupielādes reģistrs',
'uploadlogpagetext'      => 'Failu augšupielādes reģistrs.',
'filename'               => 'Faila nosaukums',
'filedesc'               => 'Kopsavilkums',
'fileuploadsummary'      => 'Informācija par failu:',
'filestatus'             => 'Autortiesību statuss:',
'filesource'             => 'Izejas kods:',
'uploadedfiles'          => 'Augšupielādēja failus',
'ignorewarning'          => 'Ignorēt brīdinājumu un saglabāt failu',
'ignorewarnings'         => 'Ignorēt visus brīdinājumus',
'minlength1'             => 'Failu vārdiem jābūt vismaz vienu simbolu gariem.',
'illegalfilename'        => 'Faila nosaukumā "$1" ir simboli, kas nav atļauti virsrakstos. Lūdzu, pārdēvē failu un mēģini to vēlreiz augšuplādēt.',
'badfilename'            => 'Attēla nosaukums ir nomainīts, tagad tas ir "$1".',
'filetype-badmime'       => 'Šeit nav atļauts augšuplādēt failus ar MIME tipu "$1".',
'filetype-unwanted-type' => "'''\".\$1\"''' ir nevēlams failu tips.  {{PLURAL:\$3|Ieteicamais faila tips|Ieteicamie failu tipi}} ir \$2.",
'filetype-banned-type'   => "'''\".\$1\"''' nav atļautais failu tips.  {{PLURAL:\$3|Atļautais faila tips|Atļautie failu tipi}} ir \$2.",
'filetype-missing'       => 'Failam nav paplašinājuma (piem. tāda kā ".jpg").',
'large-file'             => 'Ieteicams, lai faili nebūtu lielāki par $1;
šī faila izmērs ir $2.',
'largefileserver'        => 'Šis fails ir lielāks nekā serveris ņem pretī.',
'emptyfile'              => 'Šķiet, ka tu esi augšuplādējis tukšu failu. Iespējams, faila nosaukumā esi pieļāvis kļūdu. Lūdzu, pārbaudi, vai tiešām tu vēlies augšuplādēt tieši šo failu.',
'fileexists'             => 'Fails ar šādu nosaukumu jau pastāv, lūdzu, pārbaudi <strong><tt>$1</tt></strong>, ja neesi drošs, ka vēlies to mainīt.',
'file-thumbnail-no'      => 'Faila vārds sākas ar <strong><tt>$1</tt></strong>.
Izskatās, ka šis ir samazināts attēls <i>(thumbnail)</i>.
Ja tev ir šis pats attēls pilnā izmērā, augšuplādē to, ja nav, tad nomaini faila vārdu.',
'fileexists-forbidden'   => 'Fails ar šādu nosaukumu jau eksistē, mēģini kādu citu nosaukumu. [[Image:$1|thumb|center|$1]]',
'successfulupload'       => 'Augšupielāde veiksmīga',
'uploadwarning'          => 'Augšupielādes brīdinājums',
'savefile'               => 'Saglabāt failu',
'uploadedimage'          => 'augšupielādēju "$1"',
'overwroteimage'         => 'augšupielādēta jauna "[[$1]]" versija',
'uploaddisabled'         => 'Augšupielāde atslēgta',
'uploaddisabledtext'     => 'Failu augšupielāde šajā wiki ir atslēgta.',
'uploadscripted'         => 'Šis fails satur HTML vai skriptu kodu, kuru, interneta pārlūks, var kļūdas pēc, mēģināt interpretēt (ar potenciāli sliktām sekām).',
'uploadcorrupt'          => 'Šis fails ir bojāts, vai arī tam ir nekorekts paplašinājums. Lūdzu pārbaudi failu un augšupielādē vēlreiz.',
'uploadvirus'            => 'Šis fails satur vīrusu! Sīkāk: $1',
'sourcefilename'         => 'Augšuplādējamais fails:',
'destfilename'           => 'Vajadzīgais faila nosaukums:',
'upload-maxfilesize'     => 'Maksimālais faila izmērs: $1',
'watchthisupload'        => 'Uzraudzīt šo lapu',
'filewasdeleted'         => 'Fails ar šādu nosaukumu jau ir bijis augšuplādēts un pēc tam izdzēsts.
Apskaties $1 pirms turpini šo failu augšuplādēt atkārtoti.',
'upload-wasdeleted'      => "'''Brīdinājums: Tu augšuplādē failu kas agrāk jau ir ticis izdzēsts.'''

Apdomā labi, vai tiešām ir lietderīgi turpināt šī faila augšuplādi.
Te var apskatīties dzēšanas reģistru, lai noskaidrotu kāpēc šo failu toreiz izdzēsa:",
'filename-bad-prefix'    => 'Faila vārds failam, kuru tu mēģini augšpulādēt, sākas ar <strong>"$1"</strong>, kas ir neaprakstošs vārds, kādu parasti uzģenerē digitālais fotoaparāts.
Lūdzu izvēlies aprakstošāku vārdu šim failam.',

'license' => 'Licence:',

# Special:ImageList
'imagelist-summary'     => 'Šajā lapā ir redzami visi augšuplādētie faili.
Pēc noklusējuma, pēdējie ielādētie faili atrodas saraksta augšā.
Uzklikšķinot uz kādas kolonnas virsraksta, var sakārtot pēc kāda cita parametra.',
'imagelist_search_for'  => 'Meklēt failu pēc vārda:',
'imgfile'               => 'fails',
'imagelist'             => 'Attēlu uzskaitījums',
'imagelist_date'        => 'Datums',
'imagelist_name'        => 'Nosaukums',
'imagelist_user'        => 'Lietotājs',
'imagelist_size'        => 'Izmērs',
'imagelist_description' => 'Apraksts',

# Image description page
'filehist'                  => 'Faila hronoloģija',
'filehist-help'             => 'Uzklikšķini uz datums/laiks kolonnā esošās saites, lai apskatītos, kā šis fails izskatījās tad.',
'filehist-deleteall'        => 'dzēst visus',
'filehist-deleteone'        => 'dzēst',
'filehist-revert'           => 'atjaunot',
'filehist-current'          => 'tagadējais',
'filehist-datetime'         => 'Datums/Laiks',
'filehist-user'             => 'Lietotājs',
'filehist-dimensions'       => 'Izmēri',
'filehist-filesize'         => 'Faila izmērs',
'filehist-comment'          => 'Komentārs',
'imagelinks'                => 'Attēlu saites',
'linkstoimage'              => '{{PLURAL:$1|Šajā lapā ir saite|Šajās $1 lapās ir saites}} uz šo failu:',
'nolinkstoimage'            => 'Nevienā lapā nav norāžu uz šo attēlu.',
'sharedupload'              => 'Šis fails ir no *** [[literal]] translation',
'noimage'                   => 'Ar šādu nosaukumu nav neviena faila, bet tu vari [$1].',
'noimage-linktext'          => 'augšuplādēt to',
'uploadnewversion-linktext' => 'Augšupielādēt jaunu šī faila versiju',
'imagepage-searchdupe'      => 'Meklēt šī faila kopijas',

# File reversion
'filerevert'                => 'Atjaunot $1',
'filerevert-legend'         => 'Atjaunot failu',
'filerevert-intro'          => "Tu atjauno failu '''[[Media:$1|$1]]''' uz [$4 versiju kāda bija $3, $2].",
'filerevert-comment'        => 'Komentārs:',
'filerevert-defaultcomment' => 'Atjaunots uz $2, $1 versiju',
'filerevert-submit'         => 'Atjaunot',
'filerevert-success'        => "Fails '''[[Media:$1|$1]]''' tika atjaunots uz [$4 versiju, kāda tā bija $3, $2].",
'filerevert-badversion'     => 'Šajam failam nav iepriekšējās versijas, kas atbilstu norādītajam datumam un laikam.',

# File deletion
'filedelete'                  => 'Dzēst $1',
'filedelete-legend'           => 'Dzēst failu',
'filedelete-intro'            => "Tu taisies izdzēst '''[[Media:$1|$1]]'''.",
'filedelete-intro-old'        => "Tu tagad taisies izdzēst faila '''[[Media:$1|$1]]''' versiju, kas tika augšuplādēta [$4 $3, $2].",
'filedelete-comment'          => 'Dzēšanas iemesls:',
'filedelete-submit'           => 'Izdzēst',
'filedelete-success'          => "'''$1''' tika veiksmīgi izdzēsts.",
'filedelete-success-old'      => "Faila '''[[Media:$1|$1]]''' versija $3, $2 tika izdzēsta.",
'filedelete-nofile'           => "'''$1''' {{grammar:lokatīvs|{{SITENAME}}}} nav atrodams.",
'filedelete-nofile-old'       => "Failam '''$1''' nav vecas versijas ar norādītajiem parametriem.",
'filedelete-iscurrent'        => 'Tu mēģini izdzēst šī faila vissvaigāko versiju.
Lūdzu, vispirms atjauno vecāku versiju.',
'filedelete-otherreason'      => 'Cits/papildu iemesls:',
'filedelete-reason-otherlist' => 'Cits iemesls',
'filedelete-reason-dropdown'  => '*Izplatīti dzēšanas iemesli
** Autortiesību pārkāpums
** Viens tāds jau ir',
'filedelete-edit-reasonlist'  => 'Izmainīt dzēšanas iemeslus',

# Unwatched pages
'unwatchedpages' => 'Neuzraudzītās lapas',

# List redirects
'listredirects' => 'Pāradresāciju uzskaitījums',

# Unused templates
'unusedtemplates'     => 'Neizmantotās veidnes',
'unusedtemplatestext' => 'Šajā lapā ir uzskaitītas visas veidnes, kas nav iekļautas nevienā citā lapā. Pirms dzēšanas jāpārbauda citu veidu saites.',
'unusedtemplateswlh'  => 'citas saites',

# Random page
'randompage' => 'Nejauša lapa',

# Random redirect
'randomredirect' => 'Nejauša pāradresācijas lapa',

# Statistics
'statistics'    => 'Statistika',
'sitestats'     => '{{grammar:ģenitīvs|{{SITENAME}}}} statistika',
'userstats'     => 'Statistika par lietotājiem',
'sitestatstext' => "Datubāzē kopā ir '''\$1''' {{PLURAL:\$1|lapa|lapas}}, ieskaitot diskusiju lapas, lapas par {{GRAMMAR:akuzatīvs|{{SITENAME}}}}, nelielas \"aizmetņu\" lapas (''stubs''), pāradresācijas lapas, kā arī citas lapas, kuras, iespējams, nevar nosaukt par pilnvērtīgām satura lapām. Neskaitot iepriekš minētās, {{grammar:lokatīvs|{{SITENAME}}}} ir '''\$2''' {{PLURAL:\$2|lapa|lapas}}, {{PLURAL:\$2|kuru|kuras}} var uzskatīt par pamatsatura {{PLURAL:\$2|lapu|lapām}}.

Augšupielādēti '''\$8''' faili.

Kopš {{grammar:ģenitīvs|{{SITENAME}}}} izveidošanas lapas ir tikušas apskatītas '''\$3''' reizes un lietotāji ir izdarījuši '''\$4''' {{PLURAL:\$4|labojumu|labojumus}} (katra lapa ir labota vidēji '''\$5''' reizes).
Vidēji tas ir '''\$5''' labojumi uz lapu un apskatīšanas/labojumu attiecība ir '''\$6'''.

The [http://www.mediawiki.org/wiki/Manual:Job_queue job queue] length is '''\$7'''.",
'userstatstext' => "Reģistrēto [[Special:ListUsers|lietotāju]] skaits ir '''$1'''. No tiem '''$2''' (jeb '''$4%''') ir {{PLURAL:$2|administrators|administratori}} (skat. $5).",

'disambiguations'      => 'Nozīmju atdalīšanas lapas',
'disambiguationspage'  => 'Template:Disambig',
'disambiguations-text' => "Šeit esošajās lapās ir saite uz '''nozīmju atdalīšanas lapu'''.
Šīs saites vajadzētu izlabot, lai tās vestu tieši uz attiecīgo lapu.<br />
Lapu uzskata par nozīmju atdalīšanas lapu, ja tā satur veidni, uz kuru ir saite no [[MediaWiki:Disambiguationspage]].",

'doubleredirects'            => 'Divkāršas pāradresācijas lapas',
'doubleredirectstext'        => 'Katrā rindiņā ir saites uz pirmo un otro pāradresācijas lapu, kā arī pirmā rindiņa no otrās pāradresācijas lapas teksta, kas parasti ir faktiskā "gala" lapa, uz kuru vajadzētu būt saitei pirmajā lapā.',
'double-redirect-fixed-move' => '[[$1]] bija ticis pārvietots, tas tagad ir pāradresācija uz [[$2]]',

'brokenredirects'     => 'Kļūdainas pāradresācijas',
'brokenredirectstext' => 'Šīs ir pāradresācijas lapas uz neesošām lapām.',

'withoutinterwiki'         => 'Lapas bez interwiki',
'withoutinterwiki-summary' => "Šajās lapās nav saišu uz citu valodu projektiem (''interwiki''):",

'fewestrevisions' => 'Lapas, kurām ir vismazāk veco versiju',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|baits|baitu}}',
'ncategories'             => '$1 {{PLURAL:$1|kategorija|kategorijas}}',
'nlinks'                  => '$1 {{PLURAL:$1|saite|saites}}',
'nmembers'                => '$1 {{PLURAL:$1|lapa|lapas}}',
'nrevisions'              => '$1 {{PLURAL:$1|versija|versijas}}',
'nviews'                  => 'skatīta $1 {{PLURAL:$1|reizi|reizes}}',
'lonelypages'             => 'Lapas bez saitēm uz tām',
'uncategorizedpages'      => 'Nekategorizētās lapas',
'uncategorizedcategories' => 'Nekategorizētās kategorijas',
'uncategorizedimages'     => 'Nekategorizētie attēli',
'uncategorizedtemplates'  => 'Nekategorizētās veidnes',
'unusedcategories'        => 'Neizmantotas kategorijas',
'unusedimages'            => 'Neizmantoti attēli',
'popularpages'            => 'Populārākās lapas',
'wantedcategories'        => 'Sarkanas kategorijas',
'wantedpages'             => 'Pieprasītās lapas',
'mostlinked'              => 'Lapas, uz kurām ir visvairāk norāžu',
'mostlinkedcategories'    => 'Kategorijas, uz kurām ir visvairāk saišu',
'mostcategories'          => 'Raksti ar visvairāk kategorijām',
'mostimages'              => 'Attēli, uz kuriem ir visvairāk saišu',
'mostrevisions'           => 'Raksti, kuriem ir visvairāk iepriekšēju versiju',
'prefixindex'             => 'Meklēt pēc virsraksta pirmajiem burtiem',
'shortpages'              => 'Īsākās lapas',
'longpages'               => 'Garākās lapas',
'deadendpages'            => 'Lapas bez izejošām saitēm',
'protectedpages'          => 'Aizsargātās lapas',
'protectedtitles'         => 'Aizsargātie nosaukumi',
'protectedtitlestext'     => 'Lapas ar šādiem nosaukumiem ir aizsargātas pret lapas izveidošanu',
'protectedtitlesempty'    => 'Pagaidām nevienas lapas nosaukums nav aizsargāts ar šiem paraametriem.',
'listusers'               => 'Lietotāju uzskaitījums',
'newpages'                => 'Jaunas lapas',
'newpages-username'       => 'Lietotājs:',
'ancientpages'            => 'Senākās lapas',
'move'                    => 'Pārvietot',
'movethispage'            => 'Pārvietot šo lapu',
'unusedcategoriestext'    => 'Šīs kategorijas eksistē, tomēr nevienā rakstā vai kategorijās tās nav izmantotas.',
'pager-newer-n'           => '{{PLURAL:$1|jaunāko 1|jaunākās $1}}',
'pager-older-n'           => '{{PLURAL:$1|senāko 1|senākās $1}}',

# Book sources
'booksources' => 'Grāmatu avoti',

# Special:Log
'specialloguserlabel'  => 'Lietotājs:',
'speciallogtitlelabel' => 'Virsraksts:',
'log'                  => 'Reģistri',
'all-logs-page'        => 'Visi reģistri',
'alllogstext'          => 'Visi pieejamie {{grammar:akuzatīvs{{SITENAME}}}} reģistri.
Tu vari sašaurināt aplūkojamo reģistru, izvēloties reģistra veidu, lietotāja vārdu vai reģistrēto lapu. Visi teksta lauki izšķir lielos un mazos burtus.',
'logempty'             => 'Reģistrā nav atbilstošu ierakstu.',

# Special:AllPages
'allpages'       => 'Visas lapas',
'alphaindexline' => 'no $1 līdz $2',
'nextpage'       => 'Nākamā lapa ($1)',
'prevpage'       => 'Iepriekšējā lapa ($1)',
'allpagesfrom'   => 'Parādīt lapas sākot ar:',
'allarticles'    => 'Visi raksti',
'allpagessubmit' => 'Aiziet!',
'allpagesprefix' => 'Parādīt lapas ar šādu virsraksta sākumu:',

# Special:Categories
'categories'         => 'Kategorijas',
'categoriespagetext' => "Šīs kategorijas satur lapas vai failus.
Šeit nav parādītas [[Special:UnusedCategories|neizmantotās kategorijas]].
Skatīt arī [[Special:WantedCategories|''sarkanās'' kategorijas]].",
'categoriesfrom'     => 'Parādīt kategorijas sākot ar:',

# Special:ListUsers
'listusersfrom' => 'Parādīt lietotājus sākot ar:',

# Special:ListGroupRights
'listgrouprights'         => 'Lietotāju grupu tiesības',
'listgrouprights-summary' => 'Šis ir šajā wiki definēto lietotāju grupu uskaitījums, kopā ar tām atbilstošajām piekļuves tiesībām.
Papildu informāciju par katru individuālu piekļuves tiesību veidu, iespējams, var atrast [[{{MediaWiki:Listgrouprights-helppage}}]].',
'listgrouprights-group'   => 'Grupa',
'listgrouprights-rights'  => 'Tiesības',

# E-mail user
'mailnologin'     => 'Nav adreses, uz kuru sūtīt',
'mailnologintext' => 'Tev jābūt [[Special:UserLogin|iegājušam]], kā arī tev jābūt [[Special:Preferences|norādītai]] derīgai e-pasta adresei, lai sūtītu e-pastu citiem lietotājiem.',
'emailuser'       => 'Sūtīt e-pastu šim lietotājam',
'emailpage'       => 'Sūtīt e-pastu lietotājam',
'emailpagetext'   => 'Ja šis lietotājs ir norādījis reālu e-pasta adresi savu izvēļu lapā, tad ar šo veidni ir iespējams viņam nosūtīt e-pastu. Tā e-pasta adrese, kuru tu esi norādījis [[Special:Preferences|savā izvēļu lapā]], parādīsies e-pasta "From" lauciņā, tādejādi saņēmējs varēs tev atbildēt.',
'defemailsubject' => 'E-pasts par {{grammar:akuzatīvs|{{SITENAME}}}}',
'noemailtitle'    => 'Nav e-pasta adreses',
'noemailtext'     => 'Šis lietotājs nav norādījis derīgu e-pasta adresi vai arī ir izvēlējies nesaņemt e-pastu no citiem lietotājiem.',
'emailfrom'       => 'No:',
'emailto'         => 'Kam:',
'emailsubject'    => 'Temats:',
'emailmessage'    => 'Vēstījums:',
'emailsend'       => 'Nosūtīt',
'emailsent'       => 'E-pasts nosūtīts',
'emailsenttext'   => 'Tavs e-pasts ir nosūtīts.',

# Watchlist
'watchlist'            => 'Mani uzraugāmie raksti',
'mywatchlist'          => 'Mani uzraugāmie raksti',
'watchlistfor'         => "(priekš '''$1''')",
'nowatchlist'          => 'Tavā uzraugāmo rakstu sarakstā nav neviena raksta.',
'watchnologin'         => 'Neesi iegājis',
'watchnologintext'     => 'Tev ir [[Special:UserLogin|jāieiet]], lai mainītu uzraugāmo lapu sarakstu.',
'addedwatch'           => 'Pievienots uzraugāmo sarakstam.',
'addedwatchtext'       => "Lapa \"<nowiki>\$1</nowiki>\" ir pievienota [[Special:Watchlist|tevis uzraudzītajām lapām]], kur tiks parādītas izmaiņas, kas izdarītas šajā lapā vai šīs lapas diskusiju lapā, kā arī šī lapa tiks iezīmēta '''pustrekna''' [[Special:RecentChanges|pēdējo izmaiņu lapā]], lai to būtu vieglāk pamanīt.

Ja vēlāk pārdomāsi un nevēlēsies vairs uzraudzīt šo lapu, klikšķini uz saites '''neuzraudzīt''' rīku joslā.",
'removedwatch'         => 'Lapa vairs netiek uzraudzīta',
'removedwatchtext'     => 'Lapa "<nowiki>$1</nowiki>" ir izņemta no tava uzraugāmo lapu saraksta.',
'watch'                => 'Uzraudzīt',
'watchthispage'        => 'Uzraudzīt šo lapu',
'unwatch'              => 'Neuzraudzīt',
'unwatchthispage'      => 'Pārtraukt uzraudzīšanu',
'watchnochange'        => 'Neviena no tevis uzraudzītajām lapām nav mainīta parādītajā laika posmā.',
'watchlist-details'    => '(Tu uzraugi $1 {{PLURAL:$1|lapu|lapas}}, neieskaitot diskusiju lapas.)',
'wlheader-showupdated' => "* Lapas, kuras ir tikušas izmainītas, kopš tu tās pēdējoreiz apskatījies, te rādās ar '''pustrekniem''' burtiem",
'watchlistcontains'    => 'Tavā uzraugāmo lapu sarakstā ir $1 {{PLURAL:$1|lapa|lapas}}.',
'wlshowlast'           => 'Parādīt izmaiņas pēdējo $1 stundu laikā vai $2 dienu laikā, vai arī $3.',
'watchlist-show-bots'  => 'Parādīt botu izmaiņas',
'watchlist-hide-bots'  => 'Paslēpt botu izmaiņas',
'watchlist-show-own'   => 'Parādīt manas izmaiņas',
'watchlist-hide-own'   => 'Paslēpt manas izmaiņas',
'watchlist-show-minor' => 'Parādīt maznozīmīgās izmaiņas',
'watchlist-hide-minor' => 'Paslēpt maznozīmīgās izmaiņas',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Uzrauga...',
'unwatching' => 'Neuzrauga...',

'enotif_reset'       => 'Atzīmēt visas lapas kā apskatītas',
'enotif_newpagetext' => 'Šī ir jauna lapa.',
'changed'            => 'izmainīja',
'created'            => 'izveidoja',
'enotif_subject'     => '{{grammar:ģenitīvs|{{SITENAME}}}} lapu $PAGETITLE $CHANGEDORCREATED lietotājs $PAGEEDITOR',
'enotif_lastvisited' => '$1 lai apskatītos visas izmaiņas kopš tava pēdējā apmeklējuma.',
'enotif_lastdiff'    => '$1 lai apskatītos šo izmaiņu.',
'enotif_body'        => '$WATCHINGUSERNAME,


{{grammar:ģenitīvs|{{SITENAME}}}} lapu $PAGETITLE $CHANGEDORCREATED $PAGEEDITOR, $PAGEEDITDATE, pašreizējā versja ir $PAGETITLE_URL.

$NEWPAGE

Izmaiņu kopsavilkums bija: $PAGESUMMARY $PAGEMINOREDIT

Sazināties ar attiecīgo lietotāju:
e-pasts: $PAGEEDITOR_EMAIL
wiki: $PAGEEDITOR_WIKI

Ja šo uzraugāmo lapu izmainīs vēl, turpmāku paziņojumu par to nebūs, kamēr tu to neatvērsi.
Tu arī vari noresetot visu uzraugāmo lapu paziņojumu statusus uzraugāmo lapu sarakstā.

             {{grammar:ģenitīvs|{{SITENAME}}}} paziņojumu sistēma

--
Lai izmainītu uzraugāmo lapu saraksta uzstādījumus:
{{fullurl:{{ns:special}}:Watchlist/edit}}

Papildus informācija:
{{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => 'Dzēst lapu',
'confirm'                     => 'Apstiprināt',
'excontent'                   => "lapas saturs bija: '$1'",
'excontentauthor'             => 'saturs bija: "$1" (vienīgais autors: [[Special:Contributions/$2|$2]])',
'exbeforeblank'               => "lapas saturs pirms satura dzēšanas bija šāds: '$1'",
'exblank'                     => 'lapa bija tukša',
'delete-confirm'              => 'Dzēst "$1"',
'delete-legend'               => 'Dzēšana',
'historywarning'              => 'Brīdinājums: Tu dzēsīsi lapu, kurai ir saglabātas iepriekšējas versijas.',
'confirmdeletetext'           => 'Tu tūlīt no datubāzes dzēsīsi lapu vai attēlu, kā arī to iepriekšējās versijas. Lūdzu, apstiprini, ka tu tiešām to vēlies darīt, ka tu apzinies sekas un ka tu to dari saskaņā ar [[Project:Vadlīnijas|vadlīnijām]].',
'actioncomplete'              => 'Darbība pabeigta',
'deletedtext'                 => 'Lapa "<nowiki>$1</nowiki>" ir izdzēsta.
Šeit var apskatīties pēdējos izdzēstos: "$2".',
'deletedarticle'              => 'izdzēsu "$1"',
'dellogpage'                  => 'Dzēšanas reģistrs',
'dellogpagetext'              => 'Šajā lapā ir pēdējo dzēsto lapu saraksts.',
'deletionlog'                 => 'dzēšanas reģistrs',
'reverted'                    => 'Atjaunots uz iepriekšējo versiju',
'deletecomment'               => 'Dzēšanas iemesls',
'deleteotherreason'           => 'Cits/papildu iemesls:',
'deletereasonotherlist'       => 'Cits iemesls',
'deletereason-dropdown'       => '*Izplatīti dzēšanas iemesli
** Autora pieprsījums
** Autortiesību pārkāpums
** Vandālisms',
'delete-edit-reasonlist'      => 'Izmainīt dzēšanas iemeslus',
'delete-toobig'               => 'Šai lapai ir liela izmaiņu hronoloģija, vairāk nekā $1 {{PLURAL:$1|versija|versijas}}.
Šādu lapu dzēšana ir atslēgta, lai novērstu nejaušus traucējumus {{grammar:lokatīvs|{{SITENAME}}}}.',
'rollback'                    => 'Novērst labojumus',
'rollback_short'              => 'Novērst',
'rollbacklink'                => 'novērst',
'rollbackfailed'              => 'Novēršana neizdevās',
'cantrollback'                => 'Nav iespējams novērst labojumu; iepriekšējais labotājs ir vienīgais lapas autors.',
'alreadyrolled'               => 'Nav iespējams novērst pēdējās izmaiņas, ko lapā [[:$1]] saglabāja [[User:$2|$2]] ([[User talk:$2|Diskusija]] | [[Special:Contributions/$2|{{int:contribslink}}]]). Kāds cits jau ir rediģējis šo lapu vai novērsis izmaiņas.

Pēdējās izmaiņas saglabāja [[User:$3|$3]] ([[User talk:$3|diskusija]] | [[Special:Contributions/$3|{{int:contribslink}}]]).',
'editcomment'                 => 'Attiecīgās izmaiņas kopsavilkums bija: "<i>$1</i>".', # only shown if there is an edit comment
'revertpage'                  => 'Novērsu izmaiņas, ko izdarīja [[Special:Contributions/$2|$2]] ([[User talk:$2|Diskusija]]), atjaunoju versiju, ko saglabāja [[User:$1|$1]]', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => 'Tika novērstas $1 izdarītās izmaiņas;
un tika atjaunota iepriekšējā versija, kuru bija izveidojis $2.',
'sessionfailure'              => "Ir radusies problēma ar sesijas autentifikāciju;
šī darbība ir atcelta, lai novērstu lietotājvārda iespējami ļaunprātīgu izmantošanu.
Lūdzu, spied \"''back''\" un atjaunini iepriekšējo lapu. Tad mēģini vēlreiz.",
'protectlogpage'              => 'Aizsargāšanas reģistrs',
'protectedarticle'            => 'aizsargāja $1',
'unprotectedarticle'          => 'atcēla aizsardzību: $1',
'protect-title'               => 'Izmainīt "$1" aizsargāšanas līmeni?',
'protect-legend'              => 'Apstiprināt aizsargāšanu',
'protectcomment'              => 'Aizsargāšanas iemesls',
'protectexpiry'               => 'Beidzas:',
'protect_expiry_invalid'      => 'Beigu termiņš ir nederīgs.',
'protect_expiry_old'          => 'Beigu termiņs ir pagātnē.',
'protect-text'                => 'Šeit var apskatīties un izmainīt lapas <strong><nowiki>$1</nowiki></strong> aizsardzības līmeni.',
'protect-level-autoconfirmed' => 'Bloķēt nereģistētos lietotājus',
'protect-level-sysop'         => 'Tikai adminiem',
'protect-cascade'             => "Aizsargāt šajā lapā iekļautās lapas (veidnes) ''(cascading protection)''",
'protect-cantedit'            => 'Tu nevari izmainīt šīs lapas aizsardzības līmeņus, tāpēc, ka tur nevari izmainīt šo lapu.',
'restriction-level'           => 'Aizsardzības līmenis:',

# Restrictions (nouns)
'restriction-edit'   => 'Izmainīt',
'restriction-move'   => 'Pārvietot',
'restriction-create' => 'Izveidot',
'restriction-upload' => 'Augšuplādēt',

# Restriction levels
'restriction-level-sysop'         => 'pilnā aizsardzība',
'restriction-level-autoconfirmed' => 'daļējā aizsardzība',

# Undelete
'undelete'                 => 'Atjaunot dzēstu lapu',
'undeletepage'             => 'Skatīt un atjaunot dzēstās lapas',
'undeletepagetitle'        => "'''Šeit ir [[:$1|$1]] izdzēstās versijas'''.",
'viewdeletedpage'          => 'Skatīt izdzēstās lapas',
'undeletepagetext'         => 'Šīs lapas ir dzēstas, bet ir saglabātas arhīvā. Tās ir iespējams atjaunot, bet ņemiet vērā, ka arhīvs reizēm tiek tīrīts.',
'undeleteextrahelp'        => "Lai atjaunotu visu lapu, atstāj visus ķekšus (pie \"Lapas hronoloģija\") neieķeksētus uz uzspied uz '''''Atjaunot!'''''.
Lai atjaunotu tikai noteiktas versijas, ieķeksē vajadzīgās versijas un spied uz '''''Atjaunot!'''''. Uzspiešana uz '''''Notīrīt''''' notīrīs komentāru lauku un visus keķšus.",
'undeleterevisions'        => '$1 {{PLURAL:$1|versija|versijas}} {{PLURAL:$1|arhivēta|arhivētas}}',
'undeletehistory'          => 'Ja tu atjauno lapu, visas versijas tiks atjaunotas tās hronoloģijā.
Ja pēc dzēšanas ir izveidota jauna lapa ar tādu pašu nosaukumu, atjaunotās versijas tiks ievietotas lapas hronoloģijā attiecīgā secībā un konkrētās lapas pašreizējā versija netiks automātiski nomainīta.',
'undeleterevdel'           => 'Atjaunošana nenotiks, ja tas izraisīs jaunākās versijas izdzēšanu.
Šādos gadījumos ir vai nu jāizņem ķeksis no jaunākās versijas, vai arī jāatslēpj jaunākā versija.',
'undeletehistorynoadmin'   => 'Šī lapa ir tikusi izdzēsta. 
Dzēšanas iemesls ir redzams apakšā, kopsavilkumā, kopā ar informāciju par lietotājiem, kas bija rediģējuši šo lapu pirs tās izdzēšanas. 
Šo izdzēsto versiju teksts ir pieejams tikai administratoriem.',
'undelete-revision'        => 'Lapas $1 izdzēstā versija (kāda tā bija $2) (autors $3):',
'undeleterevision-missing' => 'Nederīga vai neeksistējoša versija.
Vai nu tu šeit esi nonācis lietojot kļūdainu saiti, vai arī šī versija jau ir tikusi atjaunota, vai arī tā ir izdzēsta pavisam.',
'undelete-nodiff'          => 'Netika atrastas iepriekšējās versijas.',
'undeletebtn'              => 'Atjaunot!',
'undeletelink'             => 'atjaunot',
'undeletereset'            => 'Notīrīt',
'undeletecomment'          => 'Komentārs:',
'undeletedarticle'         => 'atjaunoju "$1"',
'undeletedrevisions'       => '$1 {{PLURAL:$1|versija|versijas}} {{PLURAL:$1|atjaunota|atjaunotas}}',
'undeletedrevisions-files' => '{{PLURAL:$1|1 versija|$1 versijas}} un {{PLURAL:$2|1 fails|$2 faili}} atjaunoti',
'undeletedfiles'           => '{{PLURAL:$1|1 fails atjaunots|$1 faili atjaunoti}}',
'cannotundelete'           => 'Atjaunošana neizdevās;
kāds cits iespējams to ir atjaunojis ātrāk.',
'undeletedpage'            => "<big>'''$1 tika atjaunots'''</big>

[[Special:Log/delete|Dzēšanas reģistrā]] ir informācija par pēdējām dzēšanām un atjaunošanām.",

# Namespace form on various pages
'namespace'      => 'Lapas veids:',
'invert'         => 'Izvēlēties pretēji',
'blanknamespace' => '(Pamatlapa)',

# Contributions
'contributions' => 'Lietotāja devums',
'mycontris'     => 'Mans devums',
'contribsub2'   => 'Lietotājs: $1 ($2)',
'nocontribs'    => 'Netika atrastas izmaiņas, kas atbilstu šiem kritērijiem.',
'uctop'         => '(pēdējā izmaiņa)',
'month'         => 'No mēneša (un senāki):',
'year'          => 'No gada (un senāki):',

'sp-contributions-newbies'     => 'Rādīt jauno lietotāju devumu',
'sp-contributions-newbies-sub' => 'Jaunie lietotāji',
'sp-contributions-blocklog'    => 'Bloķēšanas reģistrs',
'sp-contributions-search'      => 'Meklēt lietotāju veiktās izmaiņas',
'sp-contributions-username'    => 'IP adrese vai lietotāja vārds:',
'sp-contributions-submit'      => 'Meklēt',

# What links here
'whatlinkshere'            => 'Norādes uz šo rakstu',
'whatlinkshere-title'      => 'Lapas, kurās ir saites uz lapu "$1"',
'whatlinkshere-page'       => 'Lapa:',
'linklistsub'              => '(Saišu uzskaitījums)',
'linkshere'                => "Šajās lapās ir norādes uz lapu '''[[:$1]]''':",
'nolinkshere'              => "Nevienā lapā nav norāžu uz lapu '''[[:$1]]'''.",
'isredirect'               => 'pāradresācijas lapa',
'istemplate'               => 'izsaukts',
'whatlinkshere-prev'       => '{{PLURAL:$1|iepriekšējo|iepriekšējos $1}}',
'whatlinkshere-next'       => '{{PLURAL:$1|nākamo|nākamos $1}}',
'whatlinkshere-links'      => '← saites',
'whatlinkshere-hideredirs' => '$1 pāradresācijas',
'whatlinkshere-hidelinks'  => '$1 saites',
'whatlinkshere-filters'    => 'Filtri',

# Block/unblock
'blockip'              => 'Bloķēt lietotāju',
'blockip-legend'       => 'Bloķēt lietotāju',
'blockiptext'          => 'Šo veidni izmanto, lai bloķētu kādas IP adreses vai lietotājvārda piekļuvi wiki lapu saglabāšanai. Dari to tikai, lai novērstu vandālismu atbilstoši [[Project:Vadlīnijas|noteikumiem]].
Norādi konkrētu iemeslu (piemēram, linkus uz vandalizētajām lapām).',
'ipaddress'            => 'IP adrese/lietotājvārds',
'ipadressorusername'   => 'IP adrese vai lietotājvārds',
'ipbexpiry'            => 'Termiņš',
'ipbreason'            => 'Iemesls',
'ipbreasonotherlist'   => 'Cits iemesls',
'ipbreason-dropdown'   => '*Biežākie bloķēšanas iemesli
** Ievieto nepatiesu informāciju
** Dzēš lapu saturu
** Spamo ārējās saitēs
** Ievieto nesakarīgus simbolus sakopojumus',
'ipbanononly'          => 'Bloķēt tikai anonīmos lietotājus',
'ipbcreateaccount'     => 'Neļaut izveidot lietotājvārdu',
'ipbemailban'          => 'Neļaut lietotājam sūtīt e-pastu',
'ipbenableautoblock'   => 'Automātiski bloķēt lietotāja pēdējo IP adresi un jebkuru IP adresi, no kuras šis lietotājs piekļūst šim wiki',
'ipbsubmit'            => 'Bloķēt šo lietotāju',
'ipbother'             => 'Cits laiks',
'ipboptions'           => '2 stundas:2 hours,1 diena:1 day,3 dienas:3 days,1 nedēļa:1 week,2 nedēļas:2 weeks,1 mēnesis:1 month,3 mēneši:3 months,6 mēneši:6 months,1 gads:1 year,uz nenoteiktu laiku:infinite', # display1:time1,display2:time2,...
'ipbotheroption'       => 'cits',
'ipbotherreason'       => 'Cits/papildu iemesls:',
'ipbwatchuser'         => 'Uzraudzīt šī lietotāja lietotāja un lietotāja diskusijas lapas',
'badipaddress'         => 'Nederīga IP adrese',
'blockipsuccesssub'    => 'Nobloķēts veiksmīgi',
'blockipsuccesstext'   => '[[Special:Contributions/$1|$1]] tika nobloķēts.<br />
Visus blokus var apskatīties [[Special:IPBlockList|IP bloku sarakstā]].',
'ipb-edit-dropdown'    => 'Izmainīt bloķēšanas iemeslus',
'ipb-unblock-addr'     => 'Atbloķēt $1',
'ipb-unblock'          => 'Atbloķēt lietotāju vai IP adresi',
'ipb-blocklist-addr'   => 'Skatīt $1 uzliktos, esošos blokus',
'ipb-blocklist'        => 'Apskatīties esošos blokus',
'unblockip'            => 'Atbloķēt lietotāju',
'ipusubmit'            => 'Atbloķēt šo adresi',
'unblocked'            => '[[Lietotājs:$1|$1]] tika atbloķēts',
'unblocked-id'         => 'Bloks $1 tika noņemts',
'ipblocklist'          => 'Bloķētās IP adreses un lietotājvārdi',
'ipblocklist-username' => 'Lietotāja vārds vai IP adrese:',
'blocklistline'        => '$1 $2 bloķēja $3 (termiņš $4)',
'expiringblock'        => 'beidzas $1',
'blocklink'            => 'bloķēt',
'unblocklink'          => 'atbloķēt',
'contribslink'         => 'devums',
'autoblocker'          => 'Tava IP ir nobloķēta automātiski, tāpēc, ka to nesen lietojis "[[User:$1|$1]]".
Viņa bloķēšanas iemesls bija: "$2"',
'blocklogpage'         => 'Bloķēšanas reģistrs',
'blocklogentry'        => 'nobloķēja [[$1]] uz $2 $3',
'blocklogtext'         => 'Šajā lapā ir pēdējo nobloķēto un atbloķēto lietotāju un IP adrešu saraksts. Te neparādās automātiski nobloķētās IP adreses.
Šobrīd aktīvos blokus var apskatīties [[Special:IPBlockList|bloķēto lietotāju un IP adrešu sarakstā]].',
'ipb_expiry_invalid'   => 'Nederīgs beigu termiņš',
'ip_range_invalid'     => 'Nederīgs IP diapazons',
'proxyblocker'         => 'Starpniekservera bloķētājs',
'proxyblocksuccess'    => 'Darīts.',

# Move page
'move-page'               => 'Pārvietot $1',
'move-page-legend'        => 'Pārvietot lapu',
'movepagetext'            => "Šajā lapā tu vari pārdēvēt vai pārvietot lapu, kopā tās izmaiņu hronoloģiju pārvietojot to uz citu nosaukumu.
Iepriekšējā lapa kļūs par lapu, kas pāradresēs uz jauno lapu.
Šeit var automātiski izmainīt visas pāradresācijas (redirektus) uz šo lapu (2. ķeksis apakšā).
Saites pārējās lapās uz iepriekšējo lapu netiks mainītas. Ja izvēlies neizmainīt pāradresācijas automātiski, noteikti pārbaudi un izlabo, izskaužot [[Special:DoubleRedirects|dubultu pāradresāciju]] vai [[Special:BrokenRedirects|pāradresāciju uz neesošu lapu]].
Tev ir jāpārliecinās, vai saites vēl aizvien ved tur, kur tās ir paredzētas.

Ņem vērā, ka lapa '''netiks''' pārvietota, ja jau eksistē kāda cita lapa ar vēlamo nosaukumu (izņemot gadījumus, kad tā ir tukša vai kad tā ir pāradresācijas lapa, kā arī tad, ja tai nav izmaiņu hronoloģijas).
Tas nozīmē, ka tu vari pārvietot lapu atpakaļ, no kurienes tu jau reiz to esi pārvietojis, ja būsi kļūdījies, bet tu nevari pārrakstīt jau esošu lapu.

'''BRĪDINĀJUMS!'''
Populārām lapām tā var būt krasa un negaidīta pārmaiņa;
pirms turpināšanas vēlreiz pārdomā, vai tu izproti visas iespējamās sekas.",
'movepagetalktext'        => "Saistītā diskusiju lapa, ja tāda eksistē, tiks automātiski pārvietota, '''izņemot gadījumus, kad''':
*tu pārvieto lapu uz citu palīglapu,
*ar jauno nosaukumu jau eksistē diskusiju lapa, vai arī
*atzīmēsi zemāk atrodamo lauciņu.

Ja tomēr vēlēsies, tad tev šī diskusiju lapa būs jāpārvieto vai jāapvieno pašam.",
'movearticle'             => 'Pārvietot lapu',
'movenotallowed'          => 'Tev {{grammar:lokatīvs|{{SITENAME}}}} nav tiesību pārvietot lapas .',
'newtitle'                => 'Uz šādu lapu',
'move-watch'              => 'Uzraudzīt šo lapu',
'movepagebtn'             => 'Pārvietot lapu',
'pagemovedsub'            => 'Pārvietošana notikusi veiksmīgi',
'movepage-moved'          => '<big>\'\'\'"$1" tika pārvietots uz "$2"\'\'\'</big>', # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => 'Lapa ar tādu nosaukumu jau pastāv vai arī tevis izvēlētais nosaukums ir nederīgs. Lūdzu, izvēlies citu nosaukumu.',
'cantmove-titleprotected' => 'Tu nevari pārvietot lapu uz šo nosaukumu, tāpēc, ka jaunais nosaukums (lapa) ir aizsargāta pret izveidošanu',
'talkexists'              => "'''Šī lapa pati tika pārvietota veiksmīgi, bet tās diskusiju lapu nevarēja pārvietot, tapēc, ka jaunā nosaukuma lapai jau ir diskusiju lapa. Lūdzu apvieno šīs diskusiju lapas manuāli.'''",
'movedto'                 => 'pārvietota uz',
'movetalk'                => 'Pārvietot arī diskusiju lapu, ja tāda ir.',
'move-subpages'           => 'Pārvietot visas apakšlapas, ja tādas ir',
'move-talk-subpages'      => 'Pārvietot visas diskusiju lapas apakšlapas, ja tādas ir',
'movepage-page-exists'    => 'Lapa $1 jau eksistē un to nevar pārrakstīt automātiski.',
'movepage-page-moved'     => 'Lapa $1 tika pārvietota uz $2.',
'movepage-page-unmoved'   => 'Lapu $1 nevarēja pārvietot uz $2.',
'1movedto2'               => '"[[$1]]" pārdēvēju par "[[$2]]"',
'1movedto2_redir'         => '$1 pārdēvēju par $2, izmantojot pāradresāciju',
'movelogpage'             => 'Pārvietošanas reģistrs',
'movelogpagetext'         => 'Lapu pārvietošanas (pārdēvēšanas) reģistrs.',
'movereason'              => 'Iemesls',
'revertmove'              => 'atcelt',
'delete_and_move'         => 'Dzēst un pārvietot',
'delete_and_move_text'    => '==Nepieciešama dzēšana==
Mērķa lapa "[[:$1]]" jau eksistē.
Vai tu to gribi izdzēst, lai atbrīvotu vietu pārvietošanai?',
'delete_and_move_confirm' => 'Jā, dzēst lapu',
'delete_and_move_reason'  => 'Izdzēsts, lai atbrīvotu vietu parvietošanai',
'selfmove'                => 'Izejas un mērķa lapu nosaukumi ir vienādi;
nevar pārvietot lapu uz sevi.',
'immobile_namespace'      => "Izejas vai mērķa lapa ir īpašā lapa;
nevar pārvietot lapas no un lapas uz to ''namespaci''.",
'fix-double-redirects'    => 'Automātiski izmainīt visas pāradresācijas, kas ved uz sākotnējo nosaukumu',

# Export
'export'            => 'Eksportēt lapas',
'exporttext'        => 'Šeit var eksportēt kādas noteiktas lapas vai lapu kopas tekstus un rediģēšanas hronoloģijas, XML formātā.
Šādus datus pēc tam varēs ieimportēt citā MediaWiki wiki lietojot [[Special:Import|Importēt lapas]]

Lai eksportētu lapas, šajā laukā ievadi to nosaukumus, katrā rindiņā pa vienam, un izvēlies vai gribi tikai pašreizējo versiju ar informāciju par pēdējo izmaiņu, vai arī pašreizējo versiju kopā ar visām vecajām versijām un hronoloģiju

Pirmajā gadījumā var arī lietot šādu metodi, piem., [[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]] lapai "[[{{MediaWiki:Mainpage}}]]".',
'exportcuronly'     => 'Iekļaut tikai esošo versiju (bez pilnās hronoloģijas)',
'exportnohistory'   => "----
'''Piezīme:''' Lapu eksportēšana kopā ar visu hronoloģiju šobrīd ir atslēgta, jo tas bremzē serveri.",
'export-submit'     => 'Eksportēt',
'export-addcattext' => 'Pievienot lapas no kategorijas:',
'export-addcat'     => 'Pievienot',
'export-download'   => 'Saglabāt kā failu',
'export-templates'  => 'Iekļaut veidnes',

# Namespace 8 related
'allmessages'               => 'Visi sistēmas paziņojumi',
'allmessagesname'           => 'Nosaukums',
'allmessagesdefault'        => 'Sākotnējais teksts',
'allmessagescurrent'        => 'Pašreizējais teksts',
'allmessagestext'           => "Šajā lapā ir visu \"'''Mediawiki:'''\" lapās atrodamo sistēmas paziņojumu uzskaitījums.
Šos paziņojumus var izmainīt tikai admini. Izmainot tos šeit, tie tiks izmainīti tikai šajā mediawiki instalācijā. Lai tos izmainītu visām pārējām, apskatieties [http://www.mediawiki.org/wiki/Localisation MediaWiki Localisation] un [http://translatewiki.net Betawiki].",
'allmessagesnotsupportedDB' => "Šī lapa nedarbojas, tāpēc, ka '''wgUseDatabaseMessages''' nedarbojas.",
'allmessagesfilter'         => 'Paziņojumu nosaukuma filtrs:',
'allmessagesmodified'       => 'Rādīt tikai izmainītos',

# Thumbnails
'thumbnail-more' => 'Palielināt',
'filemissing'    => 'Trūkst faila',

# Special:Import
'import'          => 'Importēt lapas',
'importnosources' => "Tiešā hronoloģijas augšuplāde ir atslēgta. Nav definēts neviens ''Transwiki'' importa avots (''source'').",

# Import log
'importlogpage' => 'Importēšanas reģistrs',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'Mana lietotāja lapa',
'tooltip-pt-anonuserpage'         => 'Manas IP adreses lietotāja lapa',
'tooltip-pt-mytalk'               => 'Mana diskusiju lapa',
'tooltip-pt-anontalk'             => 'Diskusija par labojumiem, kas izdarīti no šīs IP adreses',
'tooltip-pt-preferences'          => 'Manas izvēles',
'tooltip-pt-watchlist'            => 'Manis uzraudzītās lapas.',
'tooltip-pt-mycontris'            => 'Mani ieguldījumi',
'tooltip-pt-login'                => 'Aicinām tevi ieiet {{grammar:lokatīvs|{{SITENAME}}}}, tomēr tas nav obligāti.',
'tooltip-pt-anonlogin'            => 'Aicinām tevi ieiet {{grammar:lokatīvs|{{SITENAME}}}}, tomēr tas nav obligāti.',
'tooltip-pt-logout'               => 'Iziet',
'tooltip-ca-talk'                 => 'Diskusija par šī raksta lapu',
'tooltip-ca-edit'                 => 'Izmainīt šo lapu. Lūdzam izmantot pirmskatu pirms lapas saglabāšanas.',
'tooltip-ca-addsection'           => 'Pievienot komentāru šai diskusijai.',
'tooltip-ca-viewsource'           => 'Šī lapa ir aizsargāta. Tu vari apskatīties tās izejas kodu.',
'tooltip-ca-history'              => 'Šīs lapas iepriekšējās versijas.',
'tooltip-ca-protect'              => 'Aizsargāt šo lapu',
'tooltip-ca-delete'               => 'Dzēst šo lapu',
'tooltip-ca-undelete'             => 'Atjaunot labojumus, kas izdarīti šajā lapā pirms lapas dzēšanas.',
'tooltip-ca-move'                 => 'Pārvietot šo lapu',
'tooltip-ca-watch'                => 'Pievienot šo lapu manis uzraudzītajām lapām',
'tooltip-ca-unwatch'              => 'Izņemt šo lapu no uzraudzītajām lapām',
'tooltip-search'                  => 'Meklēt šajā wiki',
'tooltip-p-logo'                  => 'Sākumlapa',
'tooltip-n-mainpage'              => 'Iet uz sākumlapu',
'tooltip-n-portal'                => 'Par šo projektu, par to, ko tu vari šeit darīt un kur ko atrast',
'tooltip-n-currentevents'         => 'Uzzini papildinformāciju par šobrīd aktuālajiem notikumiem',
'tooltip-n-recentchanges'         => 'Izmaiņas, kas nesen izdarītas šajā wiki.',
'tooltip-n-randompage'            => 'Iet uz nejauši izvēlētu lapu',
'tooltip-n-help'                  => 'Vieta, kur uzzināt.',
'tooltip-t-whatlinkshere'         => 'Visas wiki lapas, kurās ir saites uz šejieni',
'tooltip-t-recentchangeslinked'   => 'Izmaiņas, kas nesen izdarītas lapās, kurās ir saites uz šo lapu',
'tooltip-feed-rss'                => 'Šīs lapas RSS barotne',
'tooltip-feed-atom'               => 'Šīs lapas Atom barotne',
'tooltip-t-contributions'         => 'Apskatīt šā lietotāja ieguldījumu uzskaitījumu.',
'tooltip-t-emailuser'             => 'Sūtīt e-pastu šim lietotājam',
'tooltip-t-upload'                => 'Augšuplādēt attēlus vai multimēdiju failus',
'tooltip-t-specialpages'          => 'Visu īpašo lapu uzskaitījums',
'tooltip-ca-nstab-main'           => 'Apskatīt rakstu',
'tooltip-ca-nstab-user'           => 'Apskatīt lietotāja lapu',
'tooltip-ca-nstab-media'          => 'Apskatīt multimēdiju lapu',
'tooltip-ca-nstab-special'        => 'Šī ir īpašā lapa, tu nevari izmainīt pašu lapu.',
'tooltip-ca-nstab-project'        => 'Apskatīt projekta lapu',
'tooltip-ca-nstab-image'          => 'Apskatīt attēla lapu',
'tooltip-ca-nstab-mediawiki'      => 'Apskatīt sistēmas paziņojumu',
'tooltip-ca-nstab-template'       => 'Apskatīt veidni',
'tooltip-ca-nstab-help'           => 'Apskatīt palīdzības lapu',
'tooltip-ca-nstab-category'       => 'Apskatīt kategorijas lapu',
'tooltip-minoredit'               => 'Atzīmēt šo par maznozīmīgu labojumu',
'tooltip-save'                    => 'Saglabāt veiktās izmaiņas',
'tooltip-preview'                 => 'Parādīt izmaiņu priekšskatījumu. Lūdzam izmantot šo iespēju pirms saglabāšanas.',
'tooltip-diff'                    => 'Parādīt, kā esi izmainījis tekstu.',
'tooltip-compareselectedversions' => 'Aplūkot atšķirības starp divām izvēlētajām lapas versijām.',
'tooltip-watch'                   => 'Pievienot šo lapu uzraugāmo lapu sarakstam',
'tooltip-recreate'                => 'Atjaunot lapu, lai arī tā ir bijusi izdzēsta',
'tooltip-upload'                  => 'Sākt augšuplādi',

# Attribution
'anonymous'        => 'Anonīmie {{grammar:ģenitīvs|{{SITENAME}}}} lietotāji(s)',
'siteuser'         => '{{grammar:ģenitīvs|{{SITENAME}}}} lietotājs $1',
'lastmodifiedatby' => 'Šo lapu pēdējoreiz izmainīja $3, $2, $1.', # $1 date, $2 time, $3 user

# Spam protection
'spamprotectiontitle' => 'Spama filtrs',
'spamprotectiontext'  => 'Lapu, kuru tu gribēji saglabāt, nobloķēja spama filtrs.
To visticamāk izraisīja ārēja saite uz melnajā sarakstā esošu interneta vietni.',
'spamprotectionmatch' => 'Spama filtram radās iebildumi pret šo tekstu: $1',
'spam_reverting'      => 'Atjauno iepriekšējo versiju, kas nesatur saiti uz $1',

# Info page
'numwatchers'    => 'Uzraudzītāju skaits: $1',
'numauthors'     => 'Atsevišķu autoru skaits (lapai): $1',
'numtalkauthors' => 'Atsevišķu autoru skaits (diskusiju lapai): $1',

# Math options
'mw_math_png'    => 'Vienmēr attēlot PNG',
'mw_math_simple' => 'HTML, ja ļoti vienkārši, vai arī PNG',
'mw_math_html'   => 'HTML, ja iespējams, vai arī PNG',
'mw_math_source' => 'Saglabāt kā TeX (teksta pārlūkiem)',
'mw_math_modern' => 'Moderniem pārlūkiem ieteiktais variants',
'mw_math_mathml' => 'MathML, ja iespējams (eksperimentāla iespēja)',

# Browsing diffs
'previousdiff' => '← Vecāka versija',
'nextdiff'     => 'Jaunāka versija →',

# Media information
'mediawarning'         => "'''Brīdinājums''': Šis fails var saturēt kaitīgu kodu, kuru izpildot tavā datorā var salīst vīrusi (un citas nejaucības).<hr />",
'imagemaxsize'         => 'Attēlu apraksta lapās parādāmo attēlu maksimālais izmērs:',
'thumbsize'            => 'Sīkbildes (<i>thumbnail</i>) izmērs:',
'widthheightpage'      => '$1×$2, $3 {{PLURAL:$3|lapa|lapas}}',
'file-info'            => '(faila izmērs: $1, MIME tips: $2)',
'file-info-size'       => '($1 × $2 pikseļi, faila izmērs: $3, MIME tips: $4)',
'file-nohires'         => '<small>Augstāka izšķirtspēja nav pieejama.</small>',
'svg-long-desc'        => '(SVG fails, definētais izmērs $1 × $2 pikseļi, faila izmērs: $3)',
'show-big-image'       => 'Pilnā izmērā',
'show-big-image-thumb' => '<small>Šī priekšskata izmērs: $1 × $2 pikseļi</small>',

# Special:NewImages
'newimages'             => 'Jauno attēlu galerija',
'imagelisttext'         => 'Šobrīd redzams $1 {{PLURAL:$1|attēla|attēlu}} uzskaitījums, kas sakārtots $2.',
'newimages-summary'     => 'Šeit var apskatīties pēdējos šeit augšuplādētos failus.',
'showhidebots'          => '($1 botus)',
'noimages'              => 'Nav nekā ko redzēt.',
'ilsubmit'              => 'Meklēt',
'bydate'                => '<b>pēc datuma</b>',
'sp-newimages-showfrom' => 'Rādīt jaunos attēlus sākot no $1, $2',

# Metadata
'metadata'          => 'Metadati',
'metadata-help'     => 'Šis fails satur papildu informāciju, kuru visticamk ir pievienojis digitālais fotoaparāts vai skeneris, kas šo failu izveidoja. Ja šis fails pēc tam ir ticis modificēts, šie dati var neatbilst izmaiņām (var būt novecojuši).',
'metadata-expand'   => 'Parādīt papildu detaļas',
'metadata-collapse' => 'Paslēpt papildu detaļas',
'metadata-fields'   => 'Šajā paziņojumā esošie metadatu lauki būs redzami attēla lapā arī tad, kad metadatu tabula būs sakļauta.
Pārējie lauki, pēc noklusējuma, būs paslēpti.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-imagewidth'       => 'platums',
'exif-imagelength'      => 'augstums',
'exif-bitspersample'    => 'biti komponentē',
'exif-compression'      => 'Saspiešanas veids',
'exif-orientation'      => 'Orientācija',
'exif-samplesperpixel'  => 'Komponentu skaits',
'exif-xresolution'      => 'Horizontālā izšķirtspēja',
'exif-yresolution'      => 'Vertikālā izšķirtspēja',
'exif-resolutionunit'   => 'X un Y izšķirtspējas mērvienība',
'exif-make'             => 'Fotoaparāta ražotājs',
'exif-model'            => 'Fotoaparāta modelis',
'exif-software'         => 'Lietotā programma',
'exif-artist'           => 'Autors',
'exif-copyright'        => 'Autortiesību īpašnieks',
'exif-exifversion'      => 'EXIF versija',
'exif-pixelxdimension'  => 'Valind image height',
'exif-datetimeoriginal' => 'Izveidošanas datums un laiks',
'exif-contrast'         => 'Kontrasts',
'exif-saturation'       => 'Piesātinājums',
'exif-sharpness'        => 'Asums',
'exif-gpslatituderef'   => 'Ziemeļu vai dienvidu platums',
'exif-gpslatitude'      => 'Platums',
'exif-gpslongituderef'  => 'Austrumu vai rietumu garums',
'exif-gpslongitude'     => 'Garums',
'exif-gpsaltitude'      => 'Augstums',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'Ziemeļu platums',
'exif-gpslatitude-s' => 'Dienvidu platums',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'Austrumu garums',
'exif-gpslongitude-w' => 'Rietumu garums',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'Kilometri stundā',
'exif-gpsspeed-m' => 'Jūdzes stundā',

# External editor support
'edit-externally'      => 'Izmainīt šo failu ar ārēju programmu',
'edit-externally-help' => 'Skat. [http://www.mediawiki.org/wiki/Manual:External_editors instrukcijas] Meta-Wiki, lai iegūtu vairāk informācijas.',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'visi',
'imagelistall'     => 'visas',
'watchlistall2'    => 'visas',
'namespacesall'    => 'visas',
'monthsall'        => 'visi',

# E-mail address confirmation
'confirmemail'             => 'Apstiprini e-pasta adresi',
'confirmemail_noemail'     => '[[Special:Preferences|Tavās izvēlēs]] nav norādīta derīga e-pasta adrese.',
'confirmemail_text'        => 'Šajā wiki ir nepieciešams apstiprināt savu e-pasta adresi, lai izmantotu e-pasta funkcijas. 
Spied uz zemāk esošās pogas, lai uz tavu e-pasta adresi nosūtītu apstiprināšanas e-pastu. 
Tajā būs saite ar kodu; spied uz tās saites vai atver to savā interneta pārlūkā, 
lai apstiprinātu tavas e-pasta adreses derīgumu.',
'confirmemail_pending'     => '<div class="error">Apstiprināšanas kods jau tev tika nosūtīts pa e-pastu;
ja tu nupat izveidoji savu kontu, varētu drusku pagaidīt, kamēr tas kods pienāk, pirms mēģināt dabūt jaunu.</div>',
'confirmemail_send'        => 'Nosūtīt apstiprināšanas kodu',
'confirmemail_sent'        => 'Apstiprināšanas e-pasts nosūtīts.',
'confirmemail_oncreate'    => 'Apstiprinājuma kods tika nosūtīts uz tavu e-pasta adresi.
Šīs kods nav nepieciešams, lai varētu ielogoties, bet tas būs vajadzīgs lai pieslēgtu visas e-pasta bāzētās funkcijas šajā wiki.',
'confirmemail_sendfailed'  => '{{SITENAME}} nevarēja nosūtīt apstiprināšanas e-pastu. Pārbaudi, vai adresē nav kāds nepareizs simbols.

Nosūtīšanas programma atmeta atpakaļ: $1',
'confirmemail_invalid'     => 'Nederīgs apstiprināšanas kods. Iespējams, beidzies tā termiņš.',
'confirmemail_needlogin'   => 'Lai apstiprinātu e-pasta adresi, tev vispirms jāielogojas ($1).',
'confirmemail_success'     => 'Tava e-pasta adrese ir apstiprināta. Tagad vari doties iekšā ar savu lietotājvārdu un pilnvērtīgi izmantot wiki iespējas.',
'confirmemail_loggedin'    => 'Tava e-pasta adrese tagad ir apstiprināta.',
'confirmemail_error'       => 'Notikusi kāda kļūme ar tava apstiprinājuma saglabāšanu.',
'confirmemail_subject'     => 'E-pasta adreses apstiprinajums no {{grammar:ģenitīvs|{{SITENAME}}}}',
'confirmemail_body'        => 'Kads, iespejams, tu pats, no IP adreses $1 ir registrejis {{grammar:ģenitīvs|{{SITENAME}}}} lietotaja vardu "$2" ar so e-pasta adresi.

Lai apstiprinatu, ka so lietotaja vardu esi izveidojis tu pats, un aktivizetu e-pasta izmantosanu {{SITENAME}}, atver so saiti sava interneta parluka:

$3

Ja tu *neesi* registrejis sadu lietotaja vardu, atver sho saiti savaa interneta browserii, lai atceltu shiis e-pasta adreses apstiprinaashanu:

$5

Si apstiprinajuma koda deriguma termins ir $4.',
'confirmemail_invalidated' => 'E-pasta adreses apstiprināšana atcelta',
'invalidateemail'          => 'Atcelt e-pasta adreses apstiprināšanu',

# Scary transclusion
'scarytranscludedisabled' => '[Starpviki saišu iekļaušana ir atspējota.]',
'scarytranscludefailed'   => '[Neizdevās ienest veidni $1.]',
'scarytranscludetoolong'  => '[URL adrese ir pārāk gara.]',

# Delete conflict
'deletedwhileediting' => "'''Brīdinājums:''' Šī lapa tika izdzēsta, pēc tam, kad tu to sāki izmainīt!",
'confirmrecreate'     => "Lietotājs [[User:$1|$1]] ([[User talk:$1|diskusija]]) izdzēssaa šo lapu, pēc tam, kad tu to biji sācis rediģēt, ar iemeslu:
: ''$2''
Lūdzu apstiprini, ka tiešām gribi izveidot šo lapu no jauna.",
'recreate'            => 'Izveidot no jauna',

# HTML dump
'redirectingto' => 'Pāradresē uz [[:$1]]...',

# action=purge
'confirm_purge'        => "Iztīrīt šīs lapas kešu (''cache'')?

$1",
'confirm_purge_button' => 'OK',

# AJAX search
'searchcontaining' => "Meklēt rakstus, kas satur ''$1''.",
'searchnamed'      => "Meklēt rakstus ar nosaukumu ''$1''.",
'articletitles'    => "Raksti, kas sākas ar ''$1''",
'hideresults'      => 'Paslēpt rezultātus',

# Multipage image navigation
'imgmultipageprev' => '← iepriekšējā lapa',
'imgmultipagenext' => 'nākamā lapa →',

# Table pager
'table_pager_next'         => 'Nākamā lapa',
'table_pager_prev'         => 'Iepriekšējā lapa',
'table_pager_first'        => 'Pirmā lapa',
'table_pager_last'         => 'Pēdējā lapa',
'table_pager_limit'        => 'Rādīt $1 ierakstus vienā lapā',
'table_pager_limit_submit' => 'Parādīt',
'table_pager_empty'        => 'Neko neatrada',

# Auto-summaries
'autosumm-blank'   => 'Nodzēsa lapu pa tīro',
'autosumm-replace' => "Aizvieto lapas saturu ar '$1'",
'autoredircomment' => 'Pāradresē uz [[$1]]',
'autosumm-new'     => 'Jauna lapa: $1',

# Live preview
'livepreview-loading' => 'Ielādējas…',
'livepreview-ready'   => 'Ielādējas… Gatavs!',
'livepreview-failed'  => 'Tūlītējais pirmskats nobruka! Pamēģini parasto pirmskatu.',
'livepreview-error'   => 'Neizdevās pievienoties: $1 "$2". Pamēģini parasto pirmskatu.',

# Friendlier slave lag warnings
'lag-warn-normal' => 'Izmaiņas, kas ir jaunākas par  $1 {{PLURAL:$1|sekundi|sekundēm}}, var neparādīties šajā sarakstā.',
'lag-warn-high'   => 'Sakarā ar lielu datubāzes servera lagu, izmaiņas, kas svaigākas par $1 {{PLURAL:$1|sekundi|sekundēm}}, šajā sarakstā var neparādīties.',

# Watchlist editor
'watchlistedit-numitems'       => 'Tavs uzraugāmo lapu saraksts satur {{PLURAL:$1|1 lapu|$1 lapas}}, neieskaitot diskusiju lapas.',
'watchlistedit-noitems'        => 'Tavs uzraugāmo rakstu saraksts ir tukšs.',
'watchlistedit-normal-title'   => 'Izmainīt uzraugāmo rakstu sarakstu',
'watchlistedit-normal-legend'  => 'Noņemt lapas (virsrakstus) no uzraugāmo rakstu saraksta',
'watchlistedit-normal-explain' => 'Tavā uzraugāmo rakstu sarakstā esošās lapas ir redzamas zemāk.
Lai noņemtu lapu, ieķeksē lodziņā pretī lapai un uzspied Noņemt lapas.
Var arī izmainīt [[Special:Watchlist/raw|neapstrādātu sarakstu]] (viens liels teksta lauks).',
'watchlistedit-normal-submit'  => 'Noņemt lapas',
'watchlistedit-normal-done'    => '{{PLURAL:$1|1 lapa tika noņemta|$1 lapas tika noņemtas}} no uzraugāmo rakstu saraksta:',
'watchlistedit-raw-title'      => 'Izmainīt uzraugāmo rakstu saraksta kodu',
'watchlistedit-raw-legend'     => 'Izmainīt uzraugāmo rakstu saraksta kodu',
'watchlistedit-raw-explain'    => 'Uzraugāmo rakstu sarakstā esošās lapas ir redzamas zemāk, un šo sarakstu var izmainīt lapas pievienojot vai izdzēšot no saraksta;
katrai rindai te atbilst viena lapa.
Tad, kad pabeigts, uzspied Atjaunot sarakstu.
Var arī lietot [[Special:Watchlist/edit|standarta izmainīšanas lapu]].',
'watchlistedit-raw-titles'     => 'Lapas:',
'watchlistedit-raw-submit'     => 'Atjaunot sarakstu',
'watchlistedit-raw-done'       => 'Tavs uzraugāmo rakstu saraksts tika atjaunots.',
'watchlistedit-raw-added'      => '{{PLURAL:$1|1 lapa tika pievienota|$1 lapas tika pievienotas}}:',
'watchlistedit-raw-removed'    => '{{PLURAL:$1|1 lapa tika noņemta|$1 lapas tika noņemtas}}:',

# Watchlist editing tools
'watchlisttools-view' => 'Skatīt atbilstošās izmaiņas',
'watchlisttools-edit' => 'Apskatīt un izmainīt uzraugāmo rakstu sarakstu',
'watchlisttools-raw'  => 'Izmainīt uzraugāmo rakstu saraksta kodu',

# Special:Version
'version'              => 'Versija', # Not used as normal message but as header for the special page itself
'version-extensions'   => 'Ieinstalētie paplašinājumi',
'version-specialpages' => 'Īpašās lapas',

# Special:FilePath
'filepath'        => 'Failu adreses',
'filepath-page'   => 'Fails:',
'filepath-submit' => 'Atrast adresi',

# Special:FileDuplicateSearch
'fileduplicatesearch-filename' => 'Faila vārds:',

# Special:SpecialPages
'specialpages'                   => 'Īpašās lapas',
'specialpages-note'              => '----
* Normālas īpašās lapas.
* <span class="mw-specialpagerestricted">Ierobežotas pieejas īpašās lapas.</span>',
'specialpages-group-maintenance' => 'Uzturēšanas atskaites',
'specialpages-group-other'       => 'Citas īpašās lapas',
'specialpages-group-changes'     => 'Pēdējās izmaiņas un reģistri',
'specialpages-group-media'       => 'Failu atskaites un augšuplāde',
'specialpages-group-users'       => 'Lietotāji un piekļuves tiesības',
'specialpages-group-pages'       => 'Lapu saraksti',
'specialpages-group-wiki'        => 'Wiki dati un rīki',
'specialpages-group-redirects'   => 'Pāradresējošas īpašās lapas',
'specialpages-group-spam'        => 'Spama rīki',

);
