<?php
/** Samogitian (Žemaitėška)
 *
 * @ingroup Language
 * @file
 *
 * @author Hugo.arg
 * @author Zordsdavini
 * @author לערי ריינהארט
 */

$fallback = 'lt';

$namespaceNames = array(
//	NS_MEDIA            => '',
	NS_SPECIAL          => 'Specēlos',
	NS_MAIN             => '',
	NS_TALK             => 'Aptarėms',
	NS_USER             => 'Nauduotuos',
	NS_USER_TALK        => 'Nauduotuojė_aptarėms',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK     => '$1_aptarėms',
	NS_IMAGE            => 'Abruozdielis',
	NS_IMAGE_TALK       => 'Abruozdielė_aptarėms',
	NS_MEDIAWIKI        => 'MediaWiki',
	NS_MEDIAWIKI_TALK   => 'MediaWiki_aptarėms',
	NS_TEMPLATE         => 'Šabluons',
	NS_TEMPLATE_TALK    => 'Šabluona_aptarėms',
	NS_HELP             => 'Pagelba',
	NS_HELP_TALK        => 'Pagelbas_aptarėms',
	NS_CATEGORY         => 'Kateguorėjė',
	NS_CATEGORY_TALK    => 'Kateguorėjės_aptarėms'
);

/**
  * Aliases from the fallback language 'lt' to avoid breakage of links
  */

$namespaceAliases = array(
	'Specialus'             => NS_SPECIAL,
	'Aptarimas'             => NS_TALK,
	'Naudotojas'            => NS_USER,
	'Naudotojo_aptarimas'   => NS_USER_TALK,
	'$1_aptarimas'          => NS_PROJECT_TALK,
	'Vaizdas'               => NS_IMAGE,
	'Vaizdo_aptarimas'      => NS_IMAGE_TALK,
	'MediaWiki_aptarimas'   => NS_MEDIAWIKI_TALK,
	'Šablonas'              => NS_TEMPLATE,
	'Šablono_aptarimas'     => NS_TEMPLATE_TALK,
	'Pagalba'               => NS_HELP,
	'Pagalbos_aptarimas'    => NS_HELP_TALK,
	'Kategorija'            => NS_CATEGORY,
	'Kategorijos_aptarimas' => NS_CATEGORY_TALK,
);

$messages = array(
# User preference toggles
'tog-underline'               => 'Pabrauktė nūruodas:',
'tog-highlightbroken'         => 'Fuormoutė nasontiu poslapiu nūruodas <a href="#" class="new">šėtēp</a> (prīšėngā - šėtēp <a href="#" class="internal">?</a>).',
'tog-justify'                 => 'Līgintė pastraipas palē abi poses',
'tog-hideminor'               => 'Pakavuotė mažus pataisėmus vielībūju taisīmu sārašė',
'tog-extendwatchlist'         => 'Ėšpliestė keravuojamu sāraša, kū ruodītu vėsus tėnkamus pakeitėmus',
'tog-usenewrc'                => 'Pažongē ruodomė vielibė̅jė pakeitėmā (JavaScript)',
'tog-numberheadings'          => 'Autuomatėškā numeroutė skėrsnelios',
'tog-showtoolbar'             => 'Ruodītė redagavėma rakondinė (JavaScript)',
'tog-editondblclick'          => 'Poslapiu redagavėms dvėgobu paspaudėmu (JavaScript)',
'tog-editsection'             => 'Ijongtė skėrsneliu redagavėma nauduojant nūruodas [taisītė]',
'tog-editsectiononrightclick' => 'Ijongtė skėrsneliu redagavėma paspaudos skėrsnelė pavadėnėma<br />dešėniouju pelies klavėšu (JavaScript)',
'tog-showtoc'                 => 'Ruodītė torėni, jē poslapī daugiau kāp 3 skėrsnelē',
'tog-rememberpassword'        => 'Atmintė prėsėjongėma infuormacėjė šėtom kuompioterī',
'tog-editwidth'               => 'Pėlna pluotė redagavėma lauks',
'tog-watchcreations'          => 'Pridietė poslapius, katrūs sokorio, i keravuojamu sāraša',
'tog-watchdefault'            => 'Pridietė poslapius, katrūs taisau, i keravuojamu sāraša',
'tog-watchmoves'              => 'Pridietė poslapius, katrūs parkelio, i keravuojamu sāraša',
'tog-watchdeletion'           => 'Pridietė poslapius, katrūs ėštrino, i keravuojamu sāraša',
'tog-minordefault'            => 'Palē nutīliejėma pažīmietė redagavėmus kāp mažus',
'tog-previewontop'            => 'Ruodītė parvaiza vėrš redagavėma lauka',
'tog-previewonfirst'          => 'Ruodītė straipsnė parvėiza pėrmu redagavėmu',
'tog-nocache'                 => "Nenauduotė poslapiu kaupėma (''caching'')",
'tog-enotifwatchlistpages'    => 'Siōstė mon gromata, kūmet pakeitams poslapis, katra stebiu',
'tog-enotifusertalkpages'     => 'Siōstė mon gromata, kūmet pakaitams mona nauduotuojė aptarėma poslapis',
'tog-enotifminoredits'        => 'Siōstė mon gromata, kūmet poslapė keitėms īr mažos',
'tog-enotifrevealaddr'        => 'Ruodītė mona el. pašta adresa primėnėma gromatuos',
'tog-shownumberswatching'     => 'Ruodītė keravuojantiu nauduotuoju skatliu',
'tog-fancysig'                => 'Parašos be autuomatėniu nūruodu',
'tog-externaleditor'          => 'Palē nutīliejėma nauduotė ėšuorini radaktuoriu',
'tog-externaldiff'            => 'Palē nutīliejėma nauduotė ėšuorinė skėrtomu ruodīma pruograma',
'tog-showjumplinks'           => 'Ijongtė „paršuoktė i“ pasėikiamoma nūruodas',
'tog-uselivepreview'          => 'Nauduotė tėisiogėne parvėiza (JavaScript) (Eksperimentėnis)',
'tog-forceeditsummary'        => 'Klaustė, kumet palėiku toščē pakeitėma kuomentara',
'tog-watchlisthideown'        => 'Kavuotė mona pakeitėmos keravuojamu sarašė',
'tog-watchlisthidebots'       => 'Kavuotė robotu pakeitėmos keravuojamu sārašė',
'tog-watchlisthideminor'      => 'Kavuotė mažos pakeitėmos keravuojamu sarašė',
'tog-nolangconversion'        => 'Ėšjongtė variantu keitėma',
'tog-ccmeonemails'            => 'Siōstė mon gromatu kopėjės, katros siontiu kėtėims nauduotojams',
'tog-diffonly'                => 'Neruodītė poslapė torėnė puo skėrtomās',
'tog-showhiddencats'          => 'Ruodītė pakavuotas kateguorėjės',

'underline-always'  => 'Visumet',
'underline-never'   => 'Nikumet',
'underline-default' => 'Palē naršīklės nostatīmos',

'skinpreview' => '(Parveiza)',

# Dates
'sunday'        => 'sekma dėina',
'monday'        => 'pėrmadėinis',
'tuesday'       => 'ontradėinis',
'wednesday'     => 'trečiadėinis',
'thursday'      => 'ketvėrtadėinis',
'friday'        => 'pėnktadėinis',
'saturday'      => 'subata',
'sun'           => 'Sekm',
'mon'           => 'Pėrm',
'tue'           => 'Ontr',
'wed'           => 'Treč',
'thu'           => 'Ketv',
'fri'           => 'Pėnk',
'sat'           => 'Sub',
'january'       => 'sausė',
'february'      => 'vasarė',
'march'         => 'kuova',
'april'         => 'balondė',
'may_long'      => 'gegožės',
'june'          => 'bėrželė',
'july'          => 'lėipas',
'august'        => 'rogpjūtė',
'september'     => 'siejės',
'october'       => 'spalė',
'november'      => 'lapkrėstė',
'december'      => 'groudė',
'january-gen'   => 'Sausis',
'february-gen'  => 'Vasaris',
'march-gen'     => 'Kuovs',
'april-gen'     => 'Balondis',
'may-gen'       => 'Gegožė',
'june-gen'      => 'Bėrželis',
'july-gen'      => 'Lėipa',
'august-gen'    => 'Rogpjūtis',
'september-gen' => 'Siejė',
'october-gen'   => 'Spalis',
'november-gen'  => 'Lapkrėstis',
'december-gen'  => 'Groudis',
'jan'           => 'sau',
'feb'           => 'vas',
'mar'           => 'kuo',
'apr'           => 'bal',
'may'           => 'geg',
'jun'           => 'bėr',
'jul'           => 'lėi',
'aug'           => 'rgp',
'sep'           => 'sie',
'oct'           => 'spa',
'nov'           => 'lap',
'dec'           => 'grd',

# Categories related messages
'pagecategories'           => '{{PLURAL:$1|Kateguorėjė|Kateguorėjės|Kateguorėju}}',
'category_header'          => 'Kateguorėjės „$1“ straipsnē',
'subcategories'            => 'Subkateguorėjės',
'category-media-header'    => 'Abruozdielis kateguorėjuo „$1“',
'category-empty'           => "''Šėta kateguorėjė nūnā netor nė vėina straipsnė a faila.''",
'hidden-categories'        => '{{PLURAL:$1|Pakavuota kateguorėjė|Pakavuotas kateguorėjės}}',
'hidden-category-category' => 'Pakavuotas kateguorėjės', # Name of the category where hidden categories will be listed
'listingcontinuesabbrev'   => 'tes.',

'about'          => 'Aple',
'article'        => 'Straipsnis',
'newwindow'      => '(īr atverams naujam longė)',
'cancel'         => 'Nutrauktė',
'qbfind'         => 'Ėiškuotė',
'qbbrowse'       => 'Naršītė',
'qbedit'         => 'Taisītė',
'qbpageoptions'  => 'Tas poslapis',
'qbpageinfo'     => 'Konteksts',
'qbmyoptions'    => 'Mona poslapē',
'qbspecialpages' => 'Specēlė̅jė poslapē',
'moredotdotdot'  => 'Daugiau...',
'mypage'         => 'Mona poslapis',
'mytalk'         => 'Mona aptarėms',
'anontalk'       => 'Šėta IP aptarėms',
'navigation'     => 'Navigacėjė',
'and'            => 'ėr',

# Metadata in edit box
'metadata_help' => 'Metadoumenīs:',

'errorpagetitle'    => 'Klaida',
'returnto'          => 'Grīžtė i $1.',
'tagline'           => 'Straipsnis ėš {{SITENAME}}.',
'help'              => 'Pagelba',
'search'            => 'Ėiškuotė',
'searchbutton'      => 'Ėiškuok',
'go'                => 'Ēk',
'searcharticle'     => 'Ēk',
'history'           => 'Poslapė istorėjė',
'history_short'     => 'Istuorėjė',
'updatedmarker'     => 'atnaujėnta nu paskotėnė mona apsėlonkīma',
'info_short'        => 'Infuormacėjė',
'printableversion'  => 'Versėjė spausdintė',
'permalink'         => 'Nulatėnė nūruoda',
'print'             => 'Spausdintė',
'edit'              => 'Taisītė',
'create'            => 'Sokortė',
'editthispage'      => 'Taisītė ton poslapė',
'create-this-page'  => 'Sokortė ta poslapi',
'delete'            => 'Trintė',
'deletethispage'    => 'Trintė ton poslapė',
'protect'           => 'Ožrakintė',
'protect_change'    => 'pakeistė apsauga',
'protectthispage'   => 'Ožrakintė šėta poslapi',
'unprotect'         => 'Atrakėntė',
'unprotectthispage' => 'Atrakėntė šėta poslapi',
'newpage'           => 'Naus poslapis',
'talkpage'          => 'Aptartė šėta poslapi',
'talkpagelinktext'  => 'Aptarėms',
'specialpage'       => 'Specēlosis poslapis',
'personaltools'     => 'Persuonalėnē rakondā',
'postcomment'       => 'Rašītė kuomentara',
'articlepage'       => 'Veizietė straipsnė',
'talk'              => 'Aptarėms',
'views'             => 'Parveizėtė',
'toolbox'           => 'Rakondā',
'userpage'          => 'Ruodītė nauduotoja poslapi',
'projectpage'       => 'Ruodītė pruojekta poslapi',
'imagepage'         => 'Veizietė abruozdielė poslapi',
'mediawikipage'     => 'Ruodītė pranešėma poslapi',
'templatepage'      => 'Ruodītė šabluona poslapi',
'viewhelppage'      => 'Ruodītė pagelbuos poslapi',
'categorypage'      => 'Ruodītė kateguorėjės poslapi',
'viewtalkpage'      => 'Ruodītė aptarėma poslapi',
'otherlanguages'    => 'Kėtuom kalbuom',
'redirectedfrom'    => '(Nokreipta ėš $1)',
'redirectpagesub'   => 'Nokreipėma poslapis',
'lastmodifiedat'    => 'Šėts poslapis paskotini karta pakeists $1 $2.', # $1 date, $2 time
'protectedpage'     => 'Ožrakints poslapis',
'jumpto'            => 'Paršuoktė i:',
'jumptonavigation'  => 'navėgacėjė',
'jumptosearch'      => 'paėiška',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'Aple {{SITENAME}}',
'aboutpage'            => 'Project:Aple',
'bugreports'           => 'Praneštė aple klaida',
'bugreportspage'       => 'Project:Klaidū pranešėmā',
'copyright'            => 'Turinīs pateikts so $1 licencėjė.',
'copyrightpagename'    => '{{SITENAME}} autorėnės teisės',
'copyrightpage'        => '{{ns:project}}:Autuoriu teisės',
'currentevents'        => '** Vielībė̅jė ivīkē **',
'currentevents-url'    => 'Project:Vielībė̅jė ivīkē',
'disclaimers'          => 'Atsakuomībės aprėbuojims',
'disclaimerpage'       => 'Project:Atsakuomībės aprėbuojims',
'edithelp'             => 'Kāp redagoutė',
'edithelppage'         => 'Help:Redagavėms',
'faq'                  => 'DOK',
'faqpage'              => 'Project:DOK',
'helppage'             => 'Help:Torėnīs',
'mainpage'             => 'Pėrms poslapis',
'mainpage-description' => 'Pėrms poslapis',
'policy-url'           => 'Project:Puolitėka',
'portal'               => 'Kuolektīvs',
'portal-url'           => 'Project:Kuolektīvs',
'privacy'              => 'Privatoma puolitėka',
'privacypage'          => 'Project:Privatoma puolitėka',

'badaccess'        => 'Privėlėju klaida',
'badaccess-group0' => 'Tomstā nelēdama ivīkdītė veiksma, katruo prašiet.',

'ok'                      => 'Gerā',
'retrievedfrom'           => 'Gautė ėš „$1“',
'youhavenewmessages'      => 'Tamsta toret $1 ($2).',
'newmessageslink'         => 'naujū žėnotiu',
'newmessagesdifflink'     => 'paskotinis pakeitėms',
'youhavenewmessagesmulti' => 'Toret naujū žėnotiu $1',
'editsection'             => 'taisītė',
'editold'                 => 'taisītė',
'editsectionhint'         => 'Redagoutė skirsneli: $1',
'toc'                     => 'Torėnīs',
'showtoc'                 => 'ruodītė',
'hidetoc'                 => 'kavuotė',
'thisisdeleted'           => 'Veizėtė a atkortė $1?',
'viewdeleted'             => 'Ruodītė $1?',
'feedlinks'               => 'Šaltėnis:',
'site-rss-feed'           => '$1 RSS šaltėnis',
'site-atom-feed'          => '$1 Atom šaltėnis',
'page-rss-feed'           => '„$1“ RSS šaltėnis',
'page-atom-feed'          => '„$1“ Atom šaltėnis',
'red-link-title'          => '$1 (da neparašīts)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Poslapis',
'nstab-user'      => 'Nauduotuojė poslapis',
'nstab-media'     => 'Abruozdielė poslapis',
'nstab-special'   => 'Specēlos',
'nstab-project'   => 'Proujekta poslapis',
'nstab-image'     => 'Fails',
'nstab-mediawiki' => 'Teksts',
'nstab-template'  => 'Šabluons',
'nstab-help'      => 'Pagelbuos poslapis',
'nstab-category'  => 'Kateguorėjė',

# Main script and global functions
'nosuchaction'      => 'Nier tuokė veiksma',
'nosuchspecialpage' => 'Nier tuokė specēlėjė poslapė',
'nospecialpagetext' => 'Tamsta prašiet nelaistėna specēlė̅jė poslapė, laistėnū specēliūju poslapiu sōraša rasėt [[Special:SpecialPages|specēliūju poslapiu sārošė]].',

# General errors
'error'                => 'Klaida',
'databaseerror'        => 'Doumenū bazės klaida',
'laggedslavemode'      => 'Diemesė: Poslapī gal nesmatītė naujausiu pakeitėmu.',
'readonly'             => 'Doumenū bazė ožrakėnta',
'enterlockreason'      => 'Iveskėt ožrakėnėma prižasti, tēpuogi kumet daugmaž bus atrokėnta',
'readonlytext'         => 'Doumenū bazė daba īr ožrakėnta naujėm irašam a kėtėm keitėmam,
mažo doumenū bazės techninē pruofilaktėkā,
puo tuo vėsks griš i sava viežes.
Ožrakėnusiuojo admėnėstratuoriaus pateikts rakėnima paaiškėnims: $1',
'readonly_lag'         => 'Doumenū bazė bova autuomatėškā ožrakėnta, kuol pagelbinės doumenū bazės pasvīs pagrėndine',
'internalerror'        => 'Vėdėnė klaida',
'unexpected'           => 'Natėkieta raikšmie: „$1“=„$2“.',
'cannotdelete'         => 'Nepavīka ėštrintė nuruodīta poslapė a faila. (Mažo kažkas padarė pėrmesnis šėta)',
'badtitle'             => 'Bluogs pavadėnėms',
'badtitletext'         => 'Nuruodīts poslapė pavadėnėms bova neleistėns, toščės a neteisėngā sojongts terpkalbinis a terppruojektėnis pavadėnėms. Anamė gal būtė vėins a daugiau sėmbuoliu, neleistėnū pavadėnėmūs',
'perfdisabled'         => 'Atsiprašuom, no šėta funkcėjė īr laikėnā ėšjongta, nes tas ėpatėngā solietina doumenū bazė tēp, kū daugiau nėiks negal nauduotės pruojekto.',
'perfcachedts'         => 'Ruodoma ėšsauguota doumenū kopėjė, katra bova atnaujėnta $1.',
'wrong_wfQuery_params' => 'Netaisingė parametrā i funkcėjė wfQuery()<br />
Funkcėjė: $1<br />
Ožklausėms: $2',
'viewsource'           => 'Veizėtė kuoda',
'viewsourcefor'        => 'poslapiō $1',
'protectedpagetext'    => 'Šėts poslapis īr ožrakints, saugont anū nū redagavėma.',
'viewsourcetext'       => 'Tomsta galėt veizietė ėr kopėjoutė poslapė kuoda:',
'protectedinterface'   => 'Šėtom poslapi īr pruogramėnės ironguos sasajuos teksts katros īr apsauguots, kū neprietelē anū nasogadėntu.',
'editinginterface'     => "'''Diemesė:''' Tamsta redagoujat poslapi, katros īr nauduojams pruogramėnės irongos sōsėjės tekstė. Pakeitėmā šėtam poslapī tēpuogi pakeis nauduotuojė sōsėjės ėšruoda ė kėtėm nauduotuojam.",
'sqlhidden'            => '(SQL ožklausa pakavuota)',
'namespaceprotected'   => "Tamsta netorėt teisiu keistė poslapiu '''$1''' srėtī.",
'ns-specialprotected'  => '„{{ns:special}}“ vardū srėtī poslapē negal būtė keitamė.',

# Login and logout pages
'logouttitle'                => 'Nauduotuojė atsėjongėms',
'loginpagetitle'             => 'Prisėjongėms',
'yourname'                   => 'Nauduotuojė vards:',
'yourpassword'               => 'Slaptažuodis:',
'yourpasswordagain'          => 'Pakartuoket slaptažuodė:',
'remembermypassword'         => 'Atmintė šėta infuormacėjė šėtom kuompioteri',
'yourdomainname'             => 'Tamstas domens:',
'loginproblem'               => '<b>Biedas so Tamstas prisėjongėmo.</b><br />Pabandīkėt ėš naujė!',
'login'                      => 'Prisėjongtė',
'nav-login-createaccount'    => 'Prėsėjongtė / sokortė paskīra',
'loginprompt'                => 'Ijonkėt pakavukus, jēgo nuorėt prisėjongtė pri {{SITENAME}}.',
'userlogin'                  => 'Prėsėjongtė / sokortė paskīra',
'logout'                     => 'Atsėjongtė',
'userlogout'                 => 'Atsėjongtė',
'notloggedin'                => 'Neprisėjongis',
'nologin'                    => 'Netorėt prisėjongėma varda? $1.',
'nologinlink'                => 'Sokorkėt paskīra',
'createaccount'              => 'Sokortė paskīra',
'gotaccount'                 => 'Jau torėt paskīra? $1.',
'gotaccountlink'             => 'Prisėjonkėt',
'badretype'                  => 'Ivestė slaptažuodē nesotamp.',
'userexists'                 => 'Ivestasės nauduotoja vards jau nauduojams. Prašuom pasirinktė kėta varda.',
'youremail'                  => 'El. pašts:',
'username'                   => 'Nauduotuojė vards:',
'uid'                        => 'Nauduotuojė ID:',
'yourrealname'               => 'Tėkros vards:',
'yourlanguage'               => 'Aplėnkuos kalba:',
'yourvariant'                => 'Variants',
'yournick'                   => 'Pasėrinkts slapīvardis:',
'badsig'                     => 'Neteisings parašas; patėkrinkėt HTML žīmės.',
'badsiglength'               => 'Nauduotuojė vards par ėlgs; tor būtė lėg $1 simbuoliu.',
'email'                      => 'El. pašts',
'prefs-help-realname'        => 'Tėkrs vards nier privaluoms, vuo jēgo Tamsta ana ivesėt, ons bus nauduojams Tamstas darba pažīmiejėmō.',
'loginerror'                 => 'Prisėjongėma klaida',
'prefs-help-email'           => 'El. pašta adresos nier privaluoms, no ans laid kėtėm prietelem pasiektė Tamsta par Tamstuos nauduotuoja a nauduotuoja aptarėma poslapi neotskleidont Tamstuos tapatoma.',
'nocookieslogin'             => "Vikipedėjė nauduo pakavukus (''cookies''), kū prijongtu nauduotuojus. Tamsta esat ėšjongės anūs. Prašuom ijongtė pakavukus ė pamiegītė viel.",
'loginsuccesstitle'          => 'Siekmingā prisėjongiet.',
'loginsuccess'               => "'''Nūnā Tamsta esot prisėjongės pri {{SITENAME}} kāp „$1“.'''",
'nosuchuser'                 => 'Nier anėjuokė nauduotuojė pavadėnta „$1“. 
Patikrėnkėt rašība, aba [[Special:Userlogin/signup|sokorkėt naujė paskīra]].',
'nosuchusershort'            => 'Nier juokė nauduotuojė, pavadėnta „$1“. Patėkrinkėt rašība.',
'nouserspecified'            => 'Tamstā reik nuroudītė nauduotoja varda.',
'wrongpassword'              => 'Ivests neteisings slaptažuodis. Pameginket dā karta.',
'wrongpasswordempty'         => 'Ivests slaptažuodis īr tošts. Pameginket vielėk.',
'passwordtooshort'           => 'Tamstas slaptažuodis nier laistėns aba par tromps īr. Ans tor būtė nuors {{PLURAL:$1|1 sėmbuolė|$1 sėmbuoliu}} ėlgoma ė skėrtės nū Tamstas nauduotuojė varda.',
'mailmypassword'             => 'Atsiōstė naujė slaptažuodi pašto',
'passwordremindertitle'      => 'Laikėns {{SITENAME}} slaptažuodis',
'passwordremindertext'       => 'Kažkastā (tėkriausē Tamsta, IP adreso $1)
paprašė, kū atsiōstomiet naujė slaptažuodi pruojektō {{SITENAME}} ($4).
Nauduotuojė „$2“ slaptažuodis nūnā īr „$3“.
Tamsta torietomiet prisėjongtė ė daba pakeistė sava slaptažuodi.

Jēgo kažkas kėts atlėka ta prašīma aba Tamsta prisėmėniet sava slaptažuodi ė
nebnuorėt ana pakeistė, Tamsta galėt tėisiuog nekreiptė diemiesė ė šėta gruomata ė tuoliau
nauduotis sava senu slaptažuodžiu.',
'noemail'                    => 'Nier anėjuokė el. pašta adresa ivesta nauduotuojō „$1“.',
'passwordsent'               => 'Naus slaptažuodis bova nusiōsts i el. pašta adresa,
ožregėstrouta nauduotuojė „$1“.
Prašuom prisėjongtė vielē, kumet Tamsta gausėt anū.',
'blocked-mailpassword'       => 'Tamstas IP adresos īr ožblokouts nū redagavėma, tudie neleidama nauduotė slaptažuodė priminėma funkcėjės, kū apsėsauguotomė nū pėktnaudžēvėma.',
'eauthentsent'               => 'Patvėrtėnėma gruomata bova nusiōsta i paskėrta el. pašta adresa.
Prīš ėšsiontiant kėta gruomata i Tamstas diežote, Tamsta torėt vīkdītė nuruodīmus gruomatuo, kū patvėrtėntomiet, kū diežotė tėkrā īr Tamstas.',
'throttled-mailpassword'     => 'Slaptažuodžė priminims jau bova ėšsiōsts, par paskotėnes $1 adīnas. Nuorint apsėsauguotė nū pėktnaudžēvėma, slaptažuodė priminims gal būt ėšsiōsts tėk kas $1 adīnas.',
'mailerror'                  => 'Klaida siontiant pašta: $1',
'acct_creation_throttle_hit' => 'Tamsta jau sokūriet $1 prisėjongėma varda. Daugiau nebgalėma.',
'emailauthenticated'         => 'Tamstas el. pašta adresos bova ožtvirtėnts $1.',
'emailnotauthenticated'      => 'Tamstas el. pašta adresos da nier patvėrtėnts. Anėjuokės gruomatas
nebus siontamas ni vėinam žemiau ėšvardėntam puoslaugiō.',
'noemailprefs'               => 'Nuruodėkīt el. pašta adresa, kū vėiktu šėtos funkcėjės.',
'emailconfirmlink'           => 'Patvėrtinkėt sava el. pašta adresa',
'accountcreated'             => 'Nauduotuos sokorts',
'accountcreatedtext'         => 'Nauduotuos $1 sokorts.',
'loginlanguagelabel'         => 'Kalba: $1',

# Password reset dialog
'resetpass_header'  => 'Atstatītė slaptažuodi',
'resetpass_submit'  => 'Nostatītė slaptažuodi ė prėsėjongtė',
'resetpass_success' => 'Tamstas slaptažuodis pakeists siekmėngā! Daba prėsėjongiama...',
'resetpass_missing' => 'Nier fuormas doumenū.',

# Edit page toolbar
'bold_sample'     => 'Pastuorints teksts',
'bold_tip'        => 'Pastuorintė teksta',
'italic_sample'   => 'Teksts kursīvu',
'italic_tip'      => 'Teksts kursīvu',
'link_sample'     => 'Nūruodas pavadinėms',
'link_tip'        => 'Vėdinė nūruoda',
'extlink_sample'  => 'http://www.example.com nūruodas pavadėnėms',
'extlink_tip'     => 'Ėšuorėnė nūruoda (nepamėrškėt http:// priraša)',
'headline_sample' => 'Skīrė pavadėnėms',
'headline_tip'    => 'Ontra līgė skīrė pavadėnėms',
'math_sample'     => 'Iveskėt fuormolė',
'math_tip'        => 'Matematinė fuormolė (LaTeX fuormato)',
'nowiki_sample'   => 'Iterpkėt nefuormouta teksta čė',
'nowiki_tip'      => 'Ėgnoroutė wiki fuormata',
'image_sample'    => 'Pavīzdīs.jpg',
'image_tip'       => 'Idietė abruozdieli',
'media_sample'    => 'Pavīzdīs.ogg',
'media_tip'       => 'Nūruoda i media faila',
'sig_tip'         => 'Tomstas parašos ėr čiesos',
'hr_tip'          => 'Guorizuontali linėjė (nenauduokėt ba reikala)',

# Edit pages
'summary'                   => 'Kuomentars',
'subject'                   => 'Tema/ontraštė',
'minoredit'                 => 'Mažos pataisims',
'watchthis'                 => 'Keravuotė šėta poslapė',
'savearticle'               => 'Ėšsauguotė poslapė',
'preview'                   => 'Parveiza',
'showpreview'               => 'Ruodītė parveiza',
'showlivepreview'           => 'Tėisiuogėnė parvaiza',
'showdiff'                  => 'Ruodītė skėrtomus',
'anoneditwarning'           => "'''Diemesė:''' Tomsta nesat prisėjungės. Jūsa IP adresos būs irašīts i šiuo poslapė istuorėjė.",
'missingsummary'            => "'''Priminėms:''' Tamsta nenuruodiet pakeitėma kuomentara. Jēgo viel paspausėt ''Ėšsauguotė'', Tamstas pakeitėms bus ėšsauguots ba anuo.",
'missingcommenttext'        => 'Prašuom ivestė kuomentara.',
'summary-preview'           => 'Kuomentara parvaiza',
'subject-preview'           => 'Skėrsnelė/ontraštės parvaiza',
'blockedtitle'              => 'Nauduotuos īr ožblokouts',
'blockedtext'               => "<big>'''Tamstas nauduotuojė vards a IP adresos īr ožblokouts.'''</big>

Ožbluokava $1. 
Nuruodīta prižastis īr ''$2''.

* Bluokavėma pradžia: $8
* Bluokavėma pabenga: $6
* Numatīts bluokoujamasės: $7

Tamsta galėt sosėsėiktė so $1 a kėtu
[[{{MediaWiki:Grouppage-sysop}}|adminėstratuoriom]], kū aptartė ožbluokavėma.
Tamsta negalėt nauduotės funkcėjė „Rašītė laiška tam nauduotuojō“, jēgo nesot pateikis tėkra sava el. pašta adresa sava [[Special:Preferences|paskīruos nustatīmūs]] ė nesot ožblokouts nu anuos nauduojėma.
Tamstas dabartėnis IP adresos īr $3, a bluokavėma ID īr #$5. Prašuom nuruodītė šėtā, kumet kreipiatės diel atbluokavėma.",
'blockedoriginalsource'     => "Žemiau īr ruodoms '''$1''' torėnīs:",
'blockededitsource'         => "''Tamstas keitimu'' teksts poslapiui '''$1''' īr ruodoms žemiau:",
'whitelistedittitle'        => 'Nuorėnt redagoutė rēk prisėjongtė',
'loginreqlink'              => 'prisėjongtė',
'accmailtitle'              => 'Slaptažuodis ėšsiūsts īr.',
'accmailtext'               => "Nauduotuojė '$1' slaptažuodis nusiūsts i $2 īr.",
'newarticle'                => '(Naus)',
'newarticletext'            => "Tamsta pakliovuot i nūnā neesoti poslapi.
Nuoriedamė sokortė poslapi, pradiekėt rašītė žemiau esontiamė ivedima pluotė
(platiau [[{{MediaWiki:Helppage}}|pagelbas poslapī]]).
Jēgo pakliovuot čė netīčiuom, paprastiausē paspauskėt naršīklės mīgtoka '''atgal'''.",
'noarticletext'             => 'Tuo čiesu tamė poslapī nier juokė teksta, Tamsta galėt [[Special:Search/{{PAGENAME}}|ėiškuotė šėta poslapė pavadėnėma]] kėtūs poslapiūs a [{{fullurl:{{FULLPAGENAME}}|action=edit}} keistė ta poslapi].',
'clearyourcache'            => "'''Diemesė:''' ėšsauguojus Tamstā gal prireiktė ėšvalītė Tamstas naršīklės rėnktovė, kū paveizėtomėt pakeitėmus. '''Mozilla / Safari / Konqueror:''' laikīdami ''Shift'' pasėrinkėt ''Atsiōstė ėš nauja'', a paspauskėt ''Ctrl-Shift-R'' (sėstemuo Apple Mac ''Cmd-Shift-R''); '''IE:''' laikīdamė ''Ctrl'' paspauskėt ''Atnaujėntė'', o paspauskėt ''Ctrl-F5''; '''Konqueror:''' paprastiausē paspauskėt ''Perkrautė'' mīgtoka, o paspauskėt ''F5''; '''Opera''' nauduotuojam gal prireiktė pėlnā ėšvalītė anū rėnktovė ''Rakondā→Nustatīmā''.",
'usercssjsyoucanpreview'    => '<strong>Patarėms:</strong> Nauduokit „Ruodītė parvaiza“ mīgtoka, kū ėšmiegintomiet sava naujaji CSS/JS priš ėšsaugont.',
'usercsspreview'            => "'''Napamirškėt, kū Tamsta tėk parveizėt sava nauduotoja CSS, ans da nabova ėšsauguots!'''",
'userjspreview'             => "'''Nepamirškėt, kū Tamsta tėk testoujat/parvaizėt sava nauduotoja ''JavaScript'', ans da nabova ėšsauguots!'''",
'userinvalidcssjstitle'     => "'''Diemesė:''' Nė juokės ėšruodos „$1“. Napamirškėt, kū sava .css ėr .js poslapē nauduo pavadėnėma mažuosiomės raidiemis, pvz., Nauduotuos:Foo/monobook.css, o ne Nauduotuos:Foo/Monobook.css.",
'updated'                   => '(Atnaujėnta)',
'note'                      => '<strong>Pastebiejims:</strong>',
'previewnote'               => '<strong>Nepamėrškėt, kū tas tėktās pervaiza, pakeitėmā da nier ėšsauguotė!</strong>',
'previewconflict'           => 'Šėta parvaiza paruod teksta ėš vėršotinėjė teksta redagavėma lauka tēp, kāp ans bus ruodoms, jei pasirinksėt anū ėšsauguotė.',
'session_fail_preview'      => '<strong>Atsiprašuom! Mes nagalėm vīkdītė Tamstas keitėma diel sesėjės doumenū praradima.
Prašuom pamiegintė vielēk. Jei šėtā napaded, pamieginkėt atsėjongtė ėr prėsėjongtė atgal.</strong>',
'session_fail_preview_html' => "<strong>Atsėprašuom! Mes nagalėm apdoroutė Tamstas keitėma diel sesėjės doumenū praradėma.</strong>
''Kadaogi šėtom pruojekte grīnasės HTML īr ijongts, parveiza īr pasliepta kāp atsargoma prėimonė priš JavaScript atakas.''
<strong>Jei tā teisiets keitėma bandīms, prašuom pamiegint viel. Jei šėtā napaded, pamieginkėt atsėjongtė ėr prėsėjongtė atgal.</strong>",
'editing'                   => 'Taisuoms straipsnis - $1',
'editingsection'            => 'Taisuoms $1 (skėrsnelis)',
'editingcomment'            => 'Taisuoms $1 (kuomentars)',
'editconflict'              => 'Ėšpreskėt kuonflėkta: $1',
'yourtext'                  => 'Tamstas teksts',
'storedversion'             => 'Ėšsauguota versėjė',
'editingold'                => '<strong>ISPIEJIMS: Tamsta keitat ne naujausė poslapė versėjė.
Jēgo ėšsauguosėt sava pakeitėmus, paskum darītė pakeitėmā prapols.</strong>',
'yourdiff'                  => 'Skėrtomā',
'copyrightwarning'          => 'Primenam, kū vėsks, kas patenk i {{SITENAME}}, īr laikuoma pavėišėnto palē $2 (platiau - $1). Jēgo nenuorit, kū Tamstas duovis būtou ba pasėgailiejėma keitams ė platėnams, nerašīkėt čė.<br />
Tamsta tēpuogi pasėžadat, kū tas īr Tamstas patėis rašīts torėnīs a kuopėjouts ėš vėišū a panašiū valnū šaltėniu.
<strong>NEKOPĖJOUKĖT AUTUORĖNIEM TEISIEM APSAUGUOTU DARBŪ BA LEIDĖMA!</strong>',
'longpagewarning'           => '<strong>DIEMESĖ: Tas poslapis īr $1 kilobaitu ėlgoma; katruos nekatruos
naršīklės gal torietė biedū redagounant poslapius bavēk a vėrš 32 KB.
Prašuom pamiegītė poslapi padalėntė i keleta smolkesniū daliū.</strong>',
'readonlywarning'           => '<strong>DIEMESĖ: Doumenū bazė bova ožrakėnta teknėnē pruofilaktėkā,
tudie negaliesėt ėšsauguotė sava pakeitėmu daba. Tamsta galėt nosėkopėjoutė teksta i tekstėni faila
ė paskum ikeltė ana čė.</strong>',
'protectedpagewarning'      => '<strong>DIEMESĖ: Šėts poslapis īr ožrakints ėr anū redagoutė gal tėk admėnėstratuorė teises torėntīs prietelē.</strong>',
'semiprotectedpagewarning'  => "'''Pastebiejėms:''' Šėts poslapis bova ožrakėnts ėr anuo gal redagoutė tėk regėstroutė nauduotojā.",
'titleprotectedwarning'     => '<strong>DIEMESĖ: Tas poslapis bova ožrakėnts tēp, ka tėktās kāpkatrė nauduotuojē galietu ana sokortė.</strong>',
'templatesused'             => 'Straipsnī nauduojami šabluonā:',
'templatesusedpreview'      => 'Šabluonā, nauduotė šėtuo parvaizuo:',
'templatesusedsection'      => 'Šabluonā, nauduotė šėtom skėrsnelī:',
'template-protected'        => '(apsauguots)',
'template-semiprotected'    => '(posiau apsauguots)',
'nocreatetitle'             => 'Poslapiu kūrims aprėbuots',
'nocreatetext'              => '{{SITENAME}} aprėbuojė galėmībe kortė naujus poslapius.
Tamsta galėt grīžtė ė redagoutė nūnā esonti poslapi, a [[Special:UserLogin|prėsėjongtė a sokortė paskīra]].',
'recreate-deleted-warn'     => "<font color =darkred>'''Diemesė: Tomsta atkoriat poslapi, katros onkstiau bova ėštrints.'''</font>
Tomsta torėt nosprēst, a pritėnk tuoliau redagoutė šėta poslapi.
Šėta poslapė šalėnėmu istuorėjė īr pateikta patuogoma vardan:",

# "Undo" feature
'undo-success' => 'Keitėms gal būtė atšaukts. Prašuom patėkrėntė palīgėnėma, asonti žemiau, kū patvėrtėntomiet, kū Tamsta šėta ė nuorėt padarītė, ė tumet ėšsauguokit pakeitėmos, asontios žemiau, kū ožbėngtomiet keitėma atšaukėma.',
'undo-failure' => 'Keitėms nagal būt atšaukts diel konflėktounantiu tarpėniu pakeitėmu.',
'undo-summary' => 'Atšauktė [[Special:Contributions/$2|$2]] ([[User talk:$2|Aptarėms]]) versėje $1',

# History pages
'viewpagelogs'        => 'Ruodītė šėtuo poslapė specēliōsios vaiksmos',
'nohistory'           => 'Šėts poslapis netor keitėmu istuorėjės.',
'revnotfound'         => 'Versėjė narasta',
'revnotfoundtext'     => 'Nuorima poslapė versėjė narasta. Patėkrėnkėt URL, katro patekuot i šėta poslapi.',
'currentrev'          => 'Dabartėnė versėjė',
'revisionasof'        => '$1 versėjė',
'revision-info'       => '$1 versėjė nauduotuojė $2',
'previousrevision'    => '←Onkstesnė versėjė',
'nextrevision'        => 'Paskesnė versėjė→',
'currentrevisionlink' => 'Dabartėnė versėjė',
'cur'                 => 'dab',
'next'                => 'kėts',
'last'                => 'pask',
'page_first'          => 'pėrm',
'page_last'           => 'pask',
'histlegend'          => "Skėrtomā terp versėju: pažīmiekit līginamas versėjės ė spauskėt ''Enter'' klavėša a mīgtuka apatiuo.<br />
Žīmiejimā: (dab) = palīginims so vielibiausė versėjė,
(pask) = palīginims so priš ta bovosia versėjė, S = mažos pataisims.",
'deletedrev'          => '[ėštrinta]',
'histfirst'           => 'Seniausė',
'histlast'            => 'Vielibė̅jė',
'historyempty'        => '(nieka nier)',

# Revision feed
'history-feed-title'          => 'Versėju istuorėjė',
'history-feed-item-nocomment' => '$1 $2', # user at time

# Revision deletion
'revisiondelete'       => 'Trintė/atkortė versėjės',
'revdelete-text'       => 'Ėštrintuos versėjės ėr ivīkē vistėik da bus ruodomė poslapė istuorėjuo ėr specēliūju veiksmū istuorėjuo, no anū torėnė dalīs nabus vėišā pasėikiamos.
Kėtė admėnėstratuorē šėtom pruojekte vėsdar galės pasėiktė pasliepta torėni ėr galės ana atkortė viel par šėta pate sasaja, nabent īr nostatītė papėlduomė aprėbuojėmā.',
'revdelete-unsuppress' => 'Šalėntė apribuojėmos atkortuos versėjės',
'logdelete-logentry'   => 'pakeists [[$1]] atsėtėkima veiziemoms',

# History merging
'mergehistory-success' => '$3 [[:$1]] versėju siekmėngā sojongta so [[:$2]].',

# Diffs
'history-title'           => 'Poslapė „$1“ istuorėjė',
'difference'              => '(Skėrtomā terp versėju)',
'lineno'                  => 'Eilotė $1:',
'compareselectedversions' => 'Palīgintė pasėrinktas versėjės',
'editundo'                => 'atšauktė',
'diff-multi'              => '($1 {{PLURAL:$1|tarpėnis keitėms nier ruoduoms|tarpėnē keitėmā nier ruoduomė|tarpėniu keitėmu nier ruoduoma}}.)',

# Search results
'searchresults'         => 'Paėiškuos rezoltatā',
'searchsubtitle'        => 'Ėiškuoma „[[:$1]]“',
'searchsubtitleinvalid' => 'Jėškuom „$1“',
'noexactmatch'          => "'''Nier anėjuokė poslapė, pavadėnta „$1“.''' Tamsta galėt [[:$1|sokortė ta poslapi]].",
'titlematches'          => 'Straipsniu pavadėnėmu atitėkmenīs',
'notitlematches'        => 'Juokiū pavadinėma atitikmenū',
'textmatches'           => 'Poslapė torėnė atėtikmenīs',
'notextmatches'         => 'Juokiū poslapė teksta atitikmenū',
'prevn'                 => 'onkstesnius $1',
'nextn'                 => 'paskesnius $1',
'viewprevnext'          => 'Veizėtė ($1) ($2) ($3).',
'showingresults'        => "Žemiau ruodoma lėgė '''$1''' rezoltatu pradedant #'''$2'''.",
'showingresultsnum'     => "Žemiau ruodoma '''$3''' {{PLURAL:$3|rezoltata|rezoltatu|rezoltatu}} pradedant #'''$2'''.",
'powersearch'           => 'Ėiškuotė',

# Preferences page
'preferences'           => 'Nustatīmā',
'mypreferences'         => 'Mona nustatīmā',
'prefsnologin'          => 'Naprisėjongis',
'prefsnologintext'      => 'Tomstā reik būtė [[Special:UserLogin|prisėjongosiam]], kū galietomiet keistė sava nustatīmus.',
'qbsettings-none'       => 'Neruodītė',
'changepassword'        => 'Pakeistė slaptažuodė',
'skin'                  => 'Ėšruoda',
'math'                  => 'Matematėka',
'dateformat'            => 'Datuos fuormats',
'datetime'              => 'Data ė čiesos',
'math_failure'          => 'Nepavīka apdoruotė',
'math_unknown_error'    => 'nežinuoma klaida',
'math_unknown_function' => 'nežinuoma funkcėjė',
'prefs-personal'        => 'Nauduotuojė pruopilis',
'prefs-rc'              => 'Vielībė̅jė pakeitėmā',
'prefs-watchlist'       => 'Keravuojamu sārašos',
'prefs-watchlist-days'  => 'Kėik dėinū ruodītė keravuojamu sārašė:',
'prefs-watchlist-edits' => 'Kėik pakeitėmu ruodītė ėšpliestiniam keravuojamu sārašė:',
'prefs-misc'            => 'Ivairė nustatīmā',
'saveprefs'             => 'Ėšsauguotė',
'resetprefs'            => 'Atstatītė nostatīmos',
'oldpassword'           => 'Sens slaptažuodis:',
'newpassword'           => 'Naus slaptažuodis:',
'retypenew'             => 'Pakartuokėt nauja slaptažuodi:',
'textboxsize'           => 'Redagavėms',
'rows'                  => 'Eilotės:',
'columns'               => 'Štolpalē:',
'searchresultshead'     => 'Paėiškuos nostatīmā',
'resultsperpage'        => 'Rezoltatu poslapie:',
'contextlines'          => 'Eilotiu rezoltatė:',
'stub-threshold'        => 'Minimums <a href="#" class="stub">nabėngta poslapė</a> fuormatavėmō:',
'recentchangesdays'     => 'Ruodomas dėinas vielībūju pakeitėmu sārašė:',
'recentchangescount'    => 'Kėik pakeitėmū ruodoma vielībūju kėitėmu poslapī',
'savedprefs'            => 'Nostatīmā siekmėngā ėšsauguotė.',
'timezonelegend'        => 'Čiesa zuona',
'timezonetext'          => '¹Iveskitė kėik adīnu Tamstas vėitins čiesos skėrės nu serverė čiesa (UTC).',
'localtime'             => 'Vėitinis čiesos',
'timezoneoffset'        => 'Skėrtoms¹',
'servertime'            => 'Serverė čiesos',
'guesstimezone'         => 'Paimtė ėš naršīklės',
'allowemail'            => 'Lēstė siūstė el. gramuotelės ėš kėtū nauduotuoju',
'defaultns'             => 'Palē nutīliejėma ėiškuotė šėtuosė vardū srėtīsė:',
'default'               => 'palē nūtīliejėma',
'files'                 => 'Failā',

# User rights
'userrights'               => 'Nauduotuoju teisiu valdīms', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'   => 'Tvarkītė nauduotuojė gropės',
'userrights-user-editname' => 'Iveskėt nauduotuojė varda:',
'editusergroup'            => 'Redagoutė nauduotuojė gropes',
'editinguser'              => "Taisuoms nauduotuos '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup' => 'Keistė nauduotuoju gropes',
'userrights-groupsmember'  => 'Narīs:',
'userrights-reason'        => 'Keitėma prižastis:',

# Groups
'group'            => 'Gropė:',
'group-bot'        => 'Buotā',
'group-sysop'      => 'Adminėstratuorē',
'group-bureaucrat' => 'Biorokratā',
'group-all'        => '(vėsė)',

'group-bot-member'        => 'Buots',
'group-sysop-member'      => 'Adminėstratuorius',
'group-bureaucrat-member' => 'Biorokrats',

'grouppage-autoconfirmed' => '{{ns:project}}:Automatėškā patvėrtintė nauduotuojē',
'grouppage-bot'           => '{{ns:project}}:Robuotā',
'grouppage-sysop'         => '{{ns:project}}:Adminėstratuorē',
'grouppage-bureaucrat'    => '{{ns:project}}:Biorokratā',

# User rights log
'rightslog'      => 'Nauduotuoju teisiu istuorėjė',
'rightslogtext'  => 'Pateikiams nauduotuoju teisiu pakeitėmu sārašos.',
'rightslogentry' => 'pakeista $1 gropės narīstė ėš $2 i $3. Sveikėnam!',
'rightsnone'     => '(juokiū)',

# Recent changes
'nchanges'                          => '$1 {{PLURAL:$1|pakeitims|pakeitimā|pakeitimu}}',
'recentchanges'                     => 'Vielībė̅jė pakeitėmā',
'recentchanges-feed-description'    => 'Keravuokėt patius vielībiausius pakeitėmus pruojektō tamė šaltėnī.',
'rcnote'                            => "Žemiau īr '''$1''' {{PLURAL:$1|paskotinis pakeitims|paskotinē pakeitimā|paskotiniu pakeitimu}} par $2 {{PLURAL:$2|paskotinė̅jė dėina|paskotėniasės '''$2''' dėinas|paskotėniuju '''$2''' dėinū}} skaitlioujant nū $4, $5.",
'rcnotefrom'                        => 'Žemiau īr pakeitėma pradedant nū <b>$2</b> (ruodom lėgė <b>$1</b> pakeitėmu).',
'rclistfrom'                        => 'Ruodītė naujus pakeitėmus pradedant nū $1',
'rcshowhideminor'                   => '$1 mažus pakeitėmus',
'rcshowhidebots'                    => '$1 robuotus',
'rcshowhideliu'                     => '$1 prėsėjongusiūm nauduotuojūm pakeitėmus',
'rcshowhideanons'                   => '$1 anuonimėnius nauduotuojus',
'rcshowhidepatr'                    => '$1 patikrėntus pakeitėmus',
'rcshowhidemine'                    => '$1 mona pakeitėmus',
'rclinks'                           => 'Ruodītė paskotėnius $1 pakeitėmu par paskotėnė̅sēs $2 dėinū<br />$3',
'diff'                              => 'skėrt',
'hist'                              => 'ist',
'hide'                              => 'Kavuotė',
'show'                              => 'Ruodītė',
'minoreditletter'                   => 'm',
'newpageletter'                     => 'N',
'boteditletter'                     => 'r',
'number_of_watching_users_pageview' => '[$1 keravuojantiu nauduotuoju]',
'rc_categories'                     => 'Ruodītė tėk šėtas kateguorėjės (atskirkit nauduodamė „|“)',
'rc_categories_any'                 => 'Bikuokė',
'newsectionsummary'                 => '/* $1 */ naus skėrsnelis',

# Recent changes linked
'recentchangeslinked'          => 'Sosėjėn pakeitėmā',
'recentchangeslinked-title'    => 'So $1 sosėje pakeitimā',
'recentchangeslinked-noresult' => 'Nier juokiū pakeitėmu sosėitous poslapious douto čieso.',
'recentchangeslinked-summary'  => "Šėtom specēliajam poslapi ruodomė vielībė̅jė pakeitėmā poslapiūs, i katrūs īr nuruodoma. Poslapē ėš Tamstas [[Special:Watchlist|keravuojamu sāraša]] īr '''pastuorėntė'''.",

# Upload
'upload'                     => 'Ikeltė faila',
'uploadbtn'                  => 'Ikeltė faila',
'reupload'                   => 'Pakartuotė ikielima',
'reuploaddesc'               => 'Sogrīžtė i ikielima fuorma.',
'uploadnologin'              => 'Naprėsėjongis',
'uploadnologintext'          => 'Nuoriedamė ikeltė faila, torėt būt [[Special:UserLogin|prėsėjongis]].',
'upload_directory_read_only' => 'Tėnklapė serveris nagal rašītė i ikielima papke ($1).',
'uploaderror'                => 'Ikielima soklīdims',
'upload-permitted'           => 'Laistėnė failu tėpā: $1.',
'uploadlog'                  => 'ikielimu istuorėjė',
'uploadlogpage'              => 'Ikielimu istuorėjė',
'uploadlogpagetext'          => 'Žemiau pateikiam paskotėniu failu ikielima istuorėjė.',
'filename'                   => 'Faila vards',
'filedesc'                   => 'Kuomentars',
'fileuploadsummary'          => 'Kuomentars:',
'uploadedfiles'              => 'Ikeltė failā',
'ignorewarnings'             => 'Nekrėiptė diemesė i vėsuokius perspiejimos',
'minlength1'                 => 'Faila pavadinėms tor būtė nuors vėina raidie.',
'filetype-missing'           => 'Fails netor galūnės (kāp pavīzdīs „.jpg“).',
'emptyfile'                  => 'Panašu, ka fails, katra ikieliet īr toščias. Tas gal būtė diel klaiduos faila pavadėnėmė. Pasėtėkrinkėt a tėkrā nuorėt ikeltė šėta faila.',
'fileexists'                 => 'Fails so tuokiu vardu jau īr, prašuom paveizėtė <strong><tt>$1</tt></strong>, jēgo nesat ožtėkrėnts, a nuorit ana parrašītė.',
'successfulupload'           => 'Ikelt siekmėngā',
'uploadwarning'              => '<font color=red>Diemesė',
'savefile'                   => 'Ėšsauguotė faila',
'uploadedimage'              => 'ikielė „[[$1]]“',
'overwroteimage'             => 'ikruovė nauja „[[$1]]“ versėjė',
'uploadscripted'             => 'Šėts failos tor HTML a programėni kuoda, katros gal būtė klaidėngā soprasts interneta naršīklės.',
'uploadcorrupt'              => 'Fails īr pažeists a tor neteisėnga galūne. Prašuom patėkrėntė faila ėr ikeltė ana par naujė.',
'uploadvirus'                => 'Šėtom faile īr virosas! Ėšsamiau: $1',
'sourcefilename'             => 'Ikeliams fails',
'destfilename'               => 'Nuorims faila pavadinims',
'watchthisupload'            => 'Keravuotė šėta poslapė',

'upload-proto-error'      => 'Nateisėngs protuokols',
'upload-proto-error-text' => 'Nutuolinē ikielims raikalaun, kū URL prasėdietu <code>http://</code> o <code>ftp://</code>.',
'upload-file-error'       => 'Vėdėnė klaida',
'upload-file-error-text'  => 'Ivīka vėdėnė klaida bandont sokortė laikinaji faila serverī. Prašuom sosėsėiktė so sistemuos admėnėstratuoriom.',
'upload-misc-error'       => 'Nažėnuoma ikielėma klaida',
'upload-misc-error-text'  => 'Ivīka nežėnuoma klaida vīkstont ikielėmō. Prašuom patėkrėnt, kū URL teisėngs teipuogi pasėikiams ėr pamiegīkit viel. Jē bieda ėšlėik, sosėsėikėt so sistemuos admėnėstratuoriom.',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => 'Napavīkst pasėiktė URL',
'upload-curl-error6-text'  => 'Pataikts URL nagal būt pasėikts. Prašuom patėkrėntė, kū URL īr teisings ėr svetainė veik.',
'upload-curl-error28'      => 'Par ėlgā ikeliama',
'upload-curl-error28-text' => 'Atsakontė svetainė ožtronk par ėlgā. Patėkrėnkėt, a svetainė veik, palaukėt tropoti ė vielē pamiegīkit. Mažo Tamstā rēktu pamiegītė ne tuokio apkrauto čieso.',

'license'            => 'Licensėjė',
'nolicense'          => 'Nepasėrėnkt',
'license-nopreview'  => '(Parveiza negalėma)',
'upload_source_url'  => ' (tėkrs, vėišā priėinams URL)',
'upload_source_file' => ' (fails Tamstas kompioterī)',

# Special:ImageList
'imgfile'               => 'fails',
'imagelist'             => 'Failu sārašos',
'imagelist_name'        => 'Pavadinėms',
'imagelist_user'        => 'Nauduotuos',
'imagelist_size'        => 'Dėdoms',
'imagelist_description' => 'Aprašīms',

# Image description page
'filehist'                  => 'Abruozdielė istuorėjė',
'filehist-help'             => 'Paspauskėt ont datas/čiesa, ka paveizietomėt faila tuoki, kokis ons bova tū čiesu.',
'filehist-deleteall'        => 'trintė vėsus',
'filehist-deleteone'        => 'trintė šėta',
'filehist-revert'           => 'sogōžėntė',
'filehist-current'          => 'dabartėnis',
'filehist-datetime'         => 'Data/Čiesos',
'filehist-user'             => 'Nauduotuos',
'filehist-dimensions'       => 'Mierā',
'filehist-filesize'         => 'Faila dėdoms',
'filehist-comment'          => 'Kuomentars',
'imagelinks'                => 'Nūroudas',
'linkstoimage'              => '{{PLURAL:$1|Šėts poslapis|Šėtė poslapē}} nuruod i šėta faila:',
'nolinkstoimage'            => 'I faila neruod anėjuoks poslapis.',
'sharedupload'              => 'Tas fails īr ikelts bendram nauduojėmō ė gal būtė nauduojams kėtūs pruojektūs.',
'shareduploadwiki'          => 'Veizėkiet $1 tolėmesnē infuormacėjē.',
'shareduploadwiki-linktext' => 'faila aprašīma poslapi',
'noimage'                   => 'Nier faila so šėtokio pavadėnėmo. Tamsta galėt $1.',
'noimage-linktext'          => 'ikeltė ana',
'uploadnewversion-linktext' => 'Ikeltė nauja faila versėje',

# File deletion
'filedelete'         => 'Trintė $1',
'filedelete-legend'  => 'Trintė faila',
'filedelete-comment' => 'Kuomentars:',
'filedelete-submit'  => 'Trintė',
'filedelete-success' => "'''$1''' bova ėštrints.",

# MIME search
'mimesearch'         => 'MIME paėiška',
'mimesearch-summary' => 'Šėts poslapis laid ruodīti failus vagol anū MIME tipa. Iveskėt: torėnėtips/potipis, pvz. <tt>image/jpeg</tt>.',
'mimetype'           => 'MIME tips:',

# Unwatched pages
'unwatchedpages' => 'Nekeravuojėmė poslapē',

# List redirects
'listredirects' => 'Paradresavėmu sārašos',

# Unused templates
'unusedtemplates'     => 'Nenauduojamė šabluonā',
'unusedtemplatestext' => 'Šėts poslapis ruod sāraša poslapiu, esontiu šabluonu vardū srėtī, katrė nė iterptė i juoki kėta poslapi. Nepamėrškėt patėkrėntė kėtū nūruodu priš anūs ėštrėnont.',
'unusedtemplateswlh'  => 'kėtas nūruodas',

# Random page
'randompage'         => 'Bikuoks poslapis',
'randompage-nopages' => 'Šėtuo vardū srėti nier anėjuokiu poslapiu.',

# Random redirect
'randomredirect'         => 'Bikuoks paradresavėms',
'randomredirect-nopages' => 'Šėtuo vardū srėti nier anėjuokiū paradresavėmu.',

# Statistics
'statistics'             => 'Statėstėka',
'sitestats'              => 'Tėnklalapė statėstėka',
'userstats'              => 'Nauduotuoju statėstėka',
'statistics-mostpopular' => 'Daugiausē ruodītė poslapē',

'disambiguations' => 'Daugiareikšmiu žuodiu poslapē',

'doubleredirects' => 'Dvėgobė paradresavėmā',

'brokenredirects'        => 'Neveikiantīs paradresavėmā',
'brokenredirectstext'    => 'Žemiau ėšvardintė paradresavėma poslapē ruod i nasontius poslapius:',
'brokenredirects-edit'   => '(redagoutė)',
'brokenredirects-delete' => '(trintė)',

'withoutinterwiki'         => 'Poslapē ba kalbū nūruodu',
'withoutinterwiki-summary' => 'Šėtė poslapē neruod i kėtū kalbū versėjės:',

'fewestrevisions' => 'Straipsnē so mažiausė pakeitėmu',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|baits|baitā|baitu}}',
'ncategories'             => '$1 kateguorėju',
'nlinks'                  => '$1 {{PLURAL:$1|nūruoda|nūruodas|nūruodu}}',
'nmembers'                => '$1 {{PLURAL:$1|narīs|narē|nariū}}',
'nrevisions'              => '$1 pakeitėmu',
'nviews'                  => '$1 paruodīmu',
'specialpage-empty'       => 'Šėtā ataskaitā nie rezoltatu.',
'lonelypages'             => 'Vėinišė straipsnē',
'lonelypagestext'         => 'I šėtuos poslapius nier nūruodu ėš kėtū šėta pruojekta poslapiu.',
'uncategorizedpages'      => 'Poslapē, napriskėrtė juokē kateguorėjē',
'uncategorizedcategories' => 'Kateguorėjės, naprėskėrtas juokē kateguorėjē',
'uncategorizedimages'     => 'Abruozdielē, nepriskėrtė juokē kateguorėjē',
'uncategorizedtemplates'  => 'Šabluonā, nepriskėrtė juokē kateguorėjē',
'unusedcategories'        => 'Nenauduojamas kateguorėjės',
'unusedimages'            => 'Nenauduojamė failā',
'wantedcategories'        => 'Nuorėmiausės kateguorėjės',
'wantedpages'             => 'Nuorėmiausē poslapē',
'mostlinked'              => 'Daugiausē ruodomė straipsnē',
'mostlinkedcategories'    => 'Daugiausē ruodomas kateguorėjės',
'mostlinkedtemplates'     => 'Daugiausē ruodomė šabluonā',
'mostcategories'          => 'Straipsnē so daugiausē kateguorėju',
'mostimages'              => 'Daugiausē ruodomė abruozdielē',
'mostrevisions'           => 'Straipsnē so daugiausē keitėmu',
'prefixindex'             => 'Ruodīklė palē pavadinėma pradē',
'shortpages'              => 'Trompiausė poslapē',
'longpages'               => 'Ėlgiausė poslapē',
'deadendpages'            => 'Straipsnē-aklavėitės',
'deadendpagestext'        => 'Tė poslapē netor nūruodu i kėtus poslapius šėtom pruojektė.',
'protectedpages'          => 'Apsauguotė poslapē',
'protectedpagestext'      => 'Šėtē poslapē īr apsauguotė nū parkielėma a redagavėma',
'protectedpagesempty'     => 'Šėtu čiesu nier apsauguots anėjuoks fails so šėtās parametrās.',
'listusers'               => 'Sārašos nauduotuoju',
'newpages'                => 'Naujausė straipsnē',
'newpages-username'       => 'Nauduotuojė vards:',
'ancientpages'            => 'Seniausė poslapē',
'move'                    => 'Parvadintė',
'movethispage'            => 'Parvadintė šėta poslapi',
'unusedimagestext'        => 'Primenam, kū kėtas svetainės gal būtė nuruodiosės i abruozdieli tėisiogėniu URL, no vėstėik gal būtė šėtom sārašė, nuors ėr īr aktīvē naudounams.',
'unusedcategoriestext'    => 'Šėtū kateguorėju poslapē sokortė, nuors juoks kėts straipsnis a kateguorėjė ana nenauduo.',
'notargettitle'           => 'Nenuruodīts objekts',
'notargettext'            => 'Tamsta nenuruodiet nuorima poslapė a nauduotuojė,
katram ivīkdītė šėta funkcėjė.',
'pager-newer-n'           => '$1 {{PLURAL:$1|paskesnis|paskesni|paskesniū}}',
'pager-older-n'           => '{{PLURAL:$1|senesnis|senesni|senesniū}}',

# Book sources
'booksources'               => 'Knīngu šaltinē',
'booksources-search-legend' => 'Knīngu šaltiniu paėiška',
'booksources-go'            => 'Ēk!',

# Special:Log
'specialloguserlabel'  => 'Nauduotuos:',
'speciallogtitlelabel' => 'Pavadėnims:',
'log'                  => 'Specēliūju veiksmū istuorėjė',
'all-logs-page'        => 'Vėsos istuorėjės',
'log-search-legend'    => 'Ėiškuotė istuorėjuosė',
'log-search-submit'    => 'Ēk!',
'alllogstext'          => 'Bėndra idietu failu, ėštrīnėmu, ožrakėnėmu, bluokavėmu ė prėvėlėju soteikėmu istuorėjė.
Īr galėmībė somažintė rezoltatu skaitliu patėkslėnont vēksma tėpa, nauduotuojė a sosėjosė poslapė.',
'logempty'             => 'Istuorėjuo nier anėjuokiū atitinkontiu atsėtėkimu.',
'log-title-wildcard'   => 'Ėiškuotė pavadinėmu, katrė prasėded šėtuo teksto',

# Special:AllPages
'allpages'          => 'Vėsė straipsnē',
'alphaindexline'    => 'Nu $1 lėg $2',
'nextpage'          => 'Kėts poslapis ($1)',
'prevpage'          => 'Onkstesnis poslapis ($1)',
'allpagesfrom'      => 'Ruodītė poslapius pradedont nu:',
'allarticles'       => 'Vėsė straipsnē',
'allinnamespace'    => 'Vėsė poslapē (srėtis - $1)',
'allnotinnamespace' => 'Vėsė poslapē (nesontīs šiuo srėtie - $1)',
'allpagesprev'      => 'Onkstesnis',
'allpagesnext'      => 'Sekontis',
'allpagessubmit'    => 'Tink',
'allpagesprefix'    => 'Ruodītė poslapios so prīdelēs:',
'allpagesbadtitle'  => 'Douts poslapė pavadėnėms īr neteisings a tor terpkalbėnė a terppruojektėnė prīdielė. Anamė īr vėns a kelė žėnklā, katrū negal nauduotė pavadėnėmūs.',

# Special:Categories
'categories'         => 'Kateguorėjės',
'categoriespagetext' => 'Pruojekte īr šėtuos kateguorėjės.',

# Special:ListUsers
'listusersfrom'      => 'Ruodītė nauduotuojus pradedont nū:',
'listusers-submit'   => 'Ruodītė',
'listusers-noresult' => 'Nerast anėjuokiū nauduotuoju.',

# E-mail user
'mailnologin'     => 'Nier adresa',
'mailnologintext' => 'Tamstā reik būtė [[Special:UserLogin|prisėjongosiam]]
ė tor būtė ivests teisings el. pašta adresos Tamstas [[Special:Preferences|nustatīmuos]],
kū siōstomiet el. gruomatas kėtėm nauduotuojam.',
'emailuser'       => 'Rašītė gruomata šėtam nauduotuojō',
'emailpage'       => 'Siūstė el. gruomata nauduotuojui',
'usermailererror' => 'Pašta objekts grōžėna klaida:',
'noemailtitle'    => 'Nier el. pašta adreso',
'noemailtext'     => 'Šėts nauduotuos nier nuruodės teisėnga el.pašta adresa a īr pasėrinkės negautė el. pašta ėš kėtū nauduotuoju.',
'emailfrom'       => 'Nū',
'emailmessage'    => 'Teksts',
'emailsend'       => 'Siōstė',
'emailccme'       => 'Siōstė monei mona gruomatas kuopėjė.',
'emailccsubject'  => 'Gruomatas kuopėjė nauduotuojō $1: $2',
'emailsent'       => 'El. gruomata ėšsiōsta',
'emailsenttext'   => 'Tamstas el. pašta žėnotė ėšsiōsta.',

# Watchlist
'watchlist'            => 'Keravuojamė straipsnē',
'mywatchlist'          => 'Keravuojamė poslapē',
'watchlistfor'         => "(nauduotuojė '''$1''')",
'nowatchlist'          => 'Netorėt anėvėina keravuojama poslapė.',
'watchlistanontext'    => 'Prašuom $1, ka parveizietomėt a pakeistomiet elementus sava keravuojamu sārašė.',
'watchnologin'         => 'Neprisėjongės',
'watchnologintext'     => 'Tamstā rēk būtė [[Special:UserLogin|prisėjongosiam]], ka pakeistomiet sava keravuojamu sāraša.',
'addedwatch'           => 'Pridieta pri keravuojamu',
'addedwatchtext'       => "Poslapis \"[[\$1]]\" idiets i [[Special:Watchlist|stebėmū sāraša]].
Būsėmė poslapė ėr atėtinkama aptarėma poslapė pakeitėmā būs paruoduomė stebėmū poslapiu sārašė,
tāp pat būs '''parīškintė''' [[Special:RecentChanges|vielībūju pakeitėmu sārašė]], ka ėšsėskėrtom ėš kėtū straipsniu.
Jė bikumet ožsėnuorietomiet liautėis stebietė straipsnė, spostelkat \"Nebstebietė\" vėršotėniam meniu.",
'removedwatch'         => 'Pašalėntė ėš keravuojamu',
'removedwatchtext'     => 'Poslapis „[[:$1]]“ pašalėnts ėš Tomstas stebėmūju sōraša.',
'watch'                => 'Keravuotė',
'watchthispage'        => 'Keravuotė šėta poslapė',
'unwatch'              => 'Nebkeravuotė',
'unwatchthispage'      => 'Nustuotė keravuotė',
'notanarticle'         => 'Ne torėnė poslapis',
'watchnochange'        => 'Pasėrėnkto čieso nebova redagouts nė vėins keravuojams straipsnis.',
'watchlist-details'    => 'Keravuojama $1 {{PLURAL:$1|poslapis|$1 poslapē|$1 poslapiu}} neskaitlioujant aptarėmu poslapiu.',
'wlheader-enotif'      => '* El. pašta primėnėmā ijongtė īr.',
'wlheader-showupdated' => "* Poslapē, katrėi pakeistė nu Tamstas paskotėnė apsėlonkėma čiesa anūs, īr pažīmietė '''pastuorintā'''",
'watchmethod-recent'   => 'tėkrėnamė vielībė̅jė pakeitėmā keravuojamiems poslapiams',
'watchmethod-list'     => 'Ėiškuoma vielībūju pakeitėmu keravuojamūs poslapiūs',
'watchlistcontains'    => 'Tamsta kervuojamu sārašė īr $1 {{PLURAL:$1|poslapis|poslapē|poslapiu}}.',
'wlnote'               => "Ruoduoma '''$1''' paskotėniu pakeitėmu, atlėktū par '''$2''' paskotėniu adīnu.",
'wlshowlast'           => 'Ruodītė paskotėniu $1 adīnu, $2 dėinū a $3 pakeitėmus',
'watchlist-show-bots'  => 'Ruodītė robotu keitėmos',
'watchlist-hide-bots'  => 'Kavuotė robotu keitėmos',
'watchlist-show-own'   => 'Ruodītė mona keitėmos',
'watchlist-hide-own'   => 'Kavuotė mona keitėmos',
'watchlist-show-minor' => 'Ruodītė mažos keitėmos',
'watchlist-hide-minor' => 'Kavuotė mažos keitėmos',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Itraukiama i keravuojamu sāraša...',
'unwatching' => 'Šalėnama ėš keravuojamu sāraša...',

'enotif_reset'       => 'Pažīmietė vėsus poslapius kāp aplonkītus',
'enotif_newpagetext' => 'Tas īr naus poslapis.',
'created'            => 'sokūrė',

# Delete/protect/revert
'deletepage'                  => 'Trintė poslapi',
'confirm'                     => 'Ožtvėrtinu',
'excontent'                   => 'boves torinīs: „$1“',
'excontentauthor'             => 'boves torinīs: „$1“ (redagava tėktās „[[Special:Contributions/$2|$2]]“)',
'exbeforeblank'               => 'priš ėštrinant torinīs bova: „$1“',
'exblank'                     => 'poslapis bova tuščes',
'delete-confirm'              => 'Eštrėnta "$1"',
'historywarning'              => 'Diemesė: Trėnams poslapis tor istuorėjė:',
'confirmdeletetext'           => 'Tamsta pasėrėnkuot ėštrėntė poslapi a abruozdieli draugum so vėsa anuo istuorėjė.
Prašuom patvėrtėntė, kū Tamsta tėkrā nuorėt šėtu padarītė, žėnuot aple galėmus padarėnius, ė kū Tamsta šėtā daruot atsėžvelgdamė i [[{{MediaWiki:Policy-url}}|puolitėka]].',
'actioncomplete'              => 'Vēksmos atlėkts īr',
'deletedtext'                 => '„$1“ ėštrints.
Paskotiniu pašalinėmu istuorėjė - $2.',
'deletedarticle'              => 'ėštrīnė „[[$1]]“',
'dellogpage'                  => 'Pašalinėmu istuorėjė',
'dellogpagetext'              => 'Žemiau īr pateikiams paskotiniu ėštrīnimu sārašos.',
'deletionlog'                 => 'pašalinėmu istuorėjė',
'reverted'                    => 'Atkorta i onkstesne versėje',
'deletecomment'               => 'Trīnima prižastis',
'deleteotherreason'           => 'Kėta/papėlduoma prižastis:',
'deletereasonotherlist'       => 'Kėta prižastis',
'deletereason-dropdown'       => '*Dažnas trīnėma prižastīs
** Autorė prašīms
** Autorėniu teisiu pažeidėms
** Vandalėzmos',
'rollback'                    => 'Atmestė pakeitėmos',
'rollback_short'              => 'Atmestė',
'rollbacklink'                => 'atmestė',
'rollbackfailed'              => 'Atmetims napavīka',
'alreadyrolled'               => 'Nepavīka atmestė paskotėnė [[User:$2|$2]] ([[User talk:$2|Aptarėms]]) darīta straipsnė [[$1]] keitėma;
kažkas jau pakeitė straipsnė arba sospiejė pėrmiesnis atmestė keitėma.

Galėnis keitėms dėrbts nauduotuojė [[User:$3|$3]] ([[User talk:$3|Aptarėms]]).',
'editcomment'                 => 'Redagavėma kuomentars: „<i>$1</i>“.', # only shown if there is an edit comment
'revertpage'                  => 'Atmests [[Special:Contributions/$2|$2]] ([[User talk:$2|Aptarėms]]) pakeitėms; sogrōžėnta nauduotoja [[User:$1|$1]] versėjė', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => 'Atmestė $1 keitėmā; grōžėnta i paskotėne $2 versėje.',
'sessionfailure'              => 'Atruod kū īr biedū so Tamstas prėsėjongėma sesėjė; šėts veiksmos bova atšaukts kāp atsargoma prėimonė priš sesėjės vuogėma.
Prašoum paspaustė „atgal“ ėr parkrautė poslapi ėš katruo atiejot, ė pamieginkėt vielē.',
'protectlogpage'              => 'Rakinėmu istuorėjė',
'protectlogtext'              => 'Žemiau īr poslapė ožrakinėmu teipuogi atrakinėmu istuorėjė. Daba veikiantiu poslapiu apsaugū sōraša sorasit [[Special:ProtectedPages|apsauguotu poslapiu sōrašuo]].',
'protectedarticle'            => 'ožrakina „[[$1]]“',
'modifiedarticleprotection'   => 'pakeists „[[$1]]“ apsauguos līgis',
'unprotectedarticle'          => 'atrakėna „[[$1]]“',
'protectcomment'              => 'Kuomentars:',
'protectexpiry'               => 'Beng galiuotė:',
'protect_expiry_invalid'      => 'Galiuojėma čiesos īr nateisėngs.',
'protect_expiry_old'          => 'Galiuojėma čiesos īr praėitī.',
'protect-unchain'             => 'Atrakintė parvadinėma teises',
'protect-text'                => 'Čė Tamsta galėt paveizėtė ė pakeistė apsauguos līgi šėtuo poslapio <strong>$1</strong>.',
'protect-locked-access'       => 'Tamstas paskīra netor teisiu keistė poslapiu apsauguos līgiu.
Čė īr dabartėnē nustatīmā poslapiō <strong>$1</strong>:',
'protect-cascadeon'           => 'Tas poslapis nūnā īr apsauguots, kadongi ons īr itraukts i {{PLURAL:$1|ta poslapi, apsauguota|tūs poslapiūs, apsauguotus}} „pakuopėnės apsauguos“ pasėrėnkėmu. Tamsta galėt pakeistė šėta poslapė apsauguos līgi, no tas nepaveiks pakuopėnės apsauguos.',
'protect-default'             => '(palē nutīliejėma)',
'protect-fallback'            => 'Rēkalautė „$1“ teisės',
'protect-level-autoconfirmed' => 'Nalaistė neregėstroutėm nauduotuojam',
'protect-level-sysop'         => 'Tėktās adminėstratuorē',
'protect-summary-cascade'     => 'pakuopėnė apsauga',
'protect-expiring'            => 'beng galiuotė $1 (UTC)',
'protect-cascade'             => 'Apsaugotė poslapius, itrauktus i šėta poslapi (pakuopėnė apsauga).',
'protect-cantedit'            => 'Tamsta negalėt keistė šėta poslapė apsauguojėma līgiu, kagongi netorėt teisiu anuo redagoutė.',
'restriction-type'            => 'Laidėms:',
'restriction-level'           => 'Aprėbuojėma līgis:',
'minimum-size'                => 'Minėmalus dėdoms',
'maximum-size'                => 'Dėdliausis dėdoms',
'pagesize'                    => '(baitās)',

# Restrictions (nouns)
'restriction-edit'   => 'Redagavėms',
'restriction-move'   => 'Parvadėnėms',
'restriction-create' => 'Sokortė',

# Restriction levels
'restriction-level-sysop'         => 'pėlnā apsauguota',
'restriction-level-autoconfirmed' => 'posiau apsauguota',
'restriction-level-all'           => 'bikuoks',

# Undelete
'undelete'                 => 'Atstatītė ėštrinta poslapi',
'undeletepage'             => 'Ruodītė ė atkortė ėštrintos poslapios',
'viewdeletedpage'          => 'Ruodītė ėštrintos poslapios',
'undeletepagetext'         => 'Žemiau ėšvardėntė poslapē īr ėštrėntė, no da laikuomi
arkīve, tudie anie gal būt atstatītė. Arkīvs gal būt perēodėškā valuoms.',
'undeleteextrahelp'        => "Nuoriedamė atkortė vėsa poslapi, palikit vėsas varnales napažīmietas ėr
spauskėt '''''Atkortė'''''. Nuoriedamiė atlėktė pasirėnktini atstatīma, pažīmiekit varnales šėtū versėju, katras nuorietomiet atstatītė, ėr spauskėt '''''Atkortė'''''. Paspaudus
'''''Ėš naujė''''' bos ėšvalītuos vėsos varnalės ėr kuomentara lauks.",
'undeleterevisions'        => '$1 versėju soarkīvouta',
'undeletehistory'          => 'Jē atstatīsėt straipsni, istuorėjuo bos atstatītuos vėsos versėjės.
Jē puo ėštrīnima bova sokuots straipsnis tuokiuo patio pavadėnėmo,
atstatītuos versėjės atsiras onkstesnie istuorėjuo, o dabartėnė
versėjė lėks napakeista. Atkoriant īr prarondamė apribuojimā failu versėjuom.',
'undeleterevdel'           => 'Atkorėms nebus ivīkdīts, jē šėtā nulems paskotėnės poslapė versėjės dalini ėštrīnima.
Tuokēs atvejās, Tamstā rēk atžīmietė a atkavuotė naujausēs ėštrintas versėjės.
Failu versėjės, katrū netorėt teisiu veizėtė, nebus atkortas.',
'undeletehistorynoadmin'   => 'Šėts straipsnis bova ėštrints. Trīnima prižastis
ruodoma žemiau, teipuogi kas redagava poslapi
lėgė trīnima. Ėštrintū poslapiu tekstos īr galėmas tėk admėnėstratuoriam.',
'undeleterevision-missing' => 'Neteisėnga a dėngosė versėjė. Tamsta mažo torėt bluoga nūruoda, a versėjė bova atkorta a pašalėnta ėš arkīva.',
'undeletebtn'              => 'Atkortė',
'undeletereset'            => 'Ėš naujė',
'undeletecomment'          => 'Kuomentars:',
'undeletedarticle'         => 'atkorta „[[$1]]“',
'undeletedrevisions'       => 'atkorta $1 versėju',
'undeletedrevisions-files' => 'atkorta $1 versėju ėr $2 failu',
'undeletedfiles'           => 'atkorta $1 failu',
'undeletedpage'            => "<big>'''$1 bova atkurts'''</big>
Parveizėkiet [[Special:Log/delete|trīnimu sāraša]], nuoriedamė rastė paskotėniu trīnimu ėr atkorėmu sāraša.",
'undelete-header'          => 'Veizėkit [[Special:Log/delete|trīnima istuorėjuo]] paskoteniausē ėštrintū poslapiu.',
'undelete-search-box'      => 'Ėiškuotė ėštrintū poslapiu',
'undelete-search-prefix'   => 'Ruodītė poslapios pradedant so:',
'undelete-search-submit'   => 'Ėiškuotė',
'undelete-no-results'      => 'Nabova rasta juokė atėtėnkontė poslapė ėštrīnima arkīve.',

# Namespace form on various pages
'namespace'      => 'Vardū srėtis:',
'invert'         => 'Žīmietė prėišingā',
'blanknamespace' => '(Pagrėndinė)',

# Contributions
'contributions' => 'Nauduotuojė duovis',
'mycontris'     => 'Mona duovis',
'contribsub2'   => 'Nauduotuojė $1 ($2)',
'uctop'         => ' (paskotinis)',
'month'         => 'Nu mienėsė (ėr onkstiau):',
'year'          => 'Nu metu (ėr onkstiau):',

'sp-contributions-newbies'     => 'Ruodītė tėk naujū prieteliu duovios',
'sp-contributions-newbies-sub' => 'Naujuoms paskīruoms',
'sp-contributions-blocklog'    => 'Bluokavėmu istuorėjė',
'sp-contributions-search'      => 'Ėiškuotė duovė',
'sp-contributions-username'    => 'IP adresos a nauduotuojė vards:',
'sp-contributions-submit'      => 'Ėiškuotė',

# What links here
'whatlinkshere'       => 'Sosėjėn straipsnē',
'whatlinkshere-title' => 'Poslapē, katrėi ruod i "$1"',
'whatlinkshere-page'  => 'Poslapis:',
'linklistsub'         => '(Nūruodu sārašos)',
'linkshere'           => "Šėtė poslapē ruod i '''[[:$1]]''':",
'nolinkshere'         => "I '''[[:$1]]''' nūruodu nier.",
'nolinkshere-ns'      => "Nurodītuo vardū srėtī anė vėins poslapis neruod i '''[[:$1]]'''.",
'isredirect'          => 'nukreipēmasės poslapis',
'istemplate'          => 'iterpims',
'whatlinkshere-prev'  => '$1 {{PLURAL:$1|onkstesnis|onkstesni|onkstesniū}}',
'whatlinkshere-next'  => '$1 {{PLURAL:$1|kėts|kėtė|kėtū}}',
'whatlinkshere-links' => '← nūruodas',

# Block/unblock
'blockip'                     => 'Ožblokoutė nauduotuoja',
'blockiptext'                 => 'Nauduokėt šėta fuorma noriedamė oždraustė redagavėma teises nuruodīto IP adreso a nauduotuojo. Tas torietu būt atlėikama tam, kū sostabdītomiet vandalėzma, ė vagol [[{{ns:project}}:Puolitėka|puolitėka]].
Žemiau nuruodīkėt tėkslē prižastė.',
'ipaddress'                   => 'IP adresos',
'ipadressorusername'          => 'IP adresos a nauduotuojė vards',
'ipbexpiry'                   => 'Galiuojėma čiesos',
'ipbreason'                   => 'Prižastis',
'ipbreasonotherlist'          => 'Kėta prižastis',
'ipbreason-dropdown'          => '*Dažniausės bluokavėma prižastīs
** Melagėngas infuormacėjės rašīms
** Torėnė trīnims ėš poslapiu
** Spaminims
** Zaunu/bikuo rašīms i poslapios
** Gondinėmā/Pėktžuodiavėmā
** Pėktnaudžiavėms paskėruomis
** Netėnkams nauduotuojė vards',
'ipbanononly'                 => 'Blokoutė tėktās anuonimėnius nauduotuojus',
'ipbcreateaccount'            => 'Nelaistė kortė paskīrū',
'ipbsubmit'                   => 'Blokoutė šėta nauduotuoja',
'ipbother'                    => 'Kėtuoks čiesos',
'ipboptions'                  => '2 adīnas:2 hours,1 dėina:1 day,3 dėinas:3 days,1 nedielė:1 week,2 nedielės:2 weeks,1 mienou:1 month,3 mienesē:3 months,6 mienesē:6 months,1 metā:1 year,omžėms:infinite', # display1:time1,display2:time2,...
'ipbotheroption'              => 'kėta',
'ipbotherreason'              => 'Kėta/papėlduoma prižastis',
'blockipsuccesssub'           => 'Ožblokavėms pavīka',
'blockipsuccesstext'          => '[[Special:Contributions/$1|$1]] bova ožblokouts.
<br />Aplonkīkėt [[Special:IPBlockList|IP blokavėmu istuorėjė]] noriedamė ana parveizėtė.',
'ipb-unblock-addr'            => 'Atblokoutė $1',
'ipb-unblock'                 => 'Atblokoutė nauduotuojė varda a IP adresa',
'ipb-blocklist-addr'          => 'Ruodītė esontius $1 bluokavėmus',
'ipb-blocklist'               => 'Ruodītė asontius bluokavėmus',
'unblockip'                   => 'Atbluokoutė nauduotuoja',
'unblockiptext'               => 'Nauduokėt šėta fuorma, kū atkortomiet rašīma teises
onkstiau ožbluokoutam IP adresō a nauduotuojō.',
'ipusubmit'                   => 'Atblokoutė šėta adresa',
'unblocked'                   => '[[User:$1|$1]] bova atbluokouts',
'unblocked-id'                => 'Bluokavėms $1 bova pašalėnts',
'ipblocklist'                 => 'Blokoutė IP adresā ė nauduotuojē',
'ipblocklist-username'        => 'Nauduotuos a IP adresos:',
'blocklistline'               => '$1, $2 ožblokava $3 ($4)',
'anononlyblock'               => 'vėn anuonėmā',
'noautoblockblock'            => 'autuomatinis blokavėms ėšjongts',
'createaccountblock'          => 'paskīrū korėms oždrausts īr',
'emailblock'                  => 'el. pašts ožblokouts',
'ipblocklist-empty'           => 'Blokavėmu sarašos toščias.',
'blocklink'                   => 'ožblokoutė',
'unblocklink'                 => 'atbluokoutė',
'contribslink'                => 'duovis',
'autoblocker'                 => 'Autuomatėnis ožbluokavėms, nes dalėnaties IP adreso so nauduotuojo "$1". Prīžastės - "$2".',
'blocklogpage'                => 'Ožblokavėmu istuorėjė',
'blocklogentry'               => 'ožblokava „[[$1]]“, blokavėma čiesos - $2 $3',
'blocklogtext'                => 'Čė īr nauduotuoju blokavėma ėr atblokavėma sārašos. Autuomatėškā blokoutė IP adresā nier ėšvardėntė. Jeigu nuorėt paveizėtė nūnā blokoujamus adresus, veizėkėt [[Special:IPBlockList|IP ožbluokavėmu istuorėjė]].',
'unblocklogentry'             => 'atbluokava $1',
'block-log-flags-anononly'    => 'vėn anonėmėnē nauduotuojē',
'block-log-flags-nocreate'    => 'privėlėju kūrėms ėšjungts',
'block-log-flags-noautoblock' => 'automatėnis blokavėms ėšjungts',
'block-log-flags-noemail'     => 'e-pašts bluokouts īr',
'ipb_expiry_invalid'          => 'Galiuojėma čiesos nelaistėns.',
'ipb_already_blocked'         => '„$1“ jau ožblokouts',
'proxyblocksuccess'           => 'Padarīt.',

# Developer tools
'unlockdbtext'        => 'Atrakėnos doumenū baze grōžėns galimībe vėsėm
nauduotuojam redagoutė poslapios, keistė anū nostatīmos, keistė anū keravuojamu sāraša ė
kėtos dalīkos, rēkalaujontios pakeitėmu doumenū bazė.
Prašuom patvėrtėntė šėtā, kū ketinat padarītė.',
'locknoconfirm'       => 'Tamsta neoždiejot patvėrtinėma varnalės.',
'unlockdbsuccesstext' => 'Doumenū bazė bova atrakėnta.',

# Move page
'movepagetext'            => "Nauduodamė žemiau pateikta fuorma, parvadinsėt poslapi neprarasdamė anuo istuorėjės.
Senasā pavadinėms pataps nukrēpiamouju - ruodīs i naujīji.
Tamsta esat atsakėngs ož šėta, kū nūruodas ruodītu i ten, kor ė nuorieta.

Primenam, kū poslapis '''nebus''' parvadints, jēgo jau īr poslapis naujo pavadinėmo, nebent tas poslapis īr tuščės a nukreipēmasis ė netor redagavėma istuorėjės.
Tumet, Tamsta galėt parvadintė poslapi seniau nauduota vardu, jēgo priš šėta ons bova par klaida parvadints, a egzėstounantiu poslapiu sogadintė negalėt.

'''DIEMESĖ!'''
Jēgo parvadinat puopoliaru poslapi, tas gal sokeltė nepagēdaunamu šalotiniu efektu, tudie šėta veiksma vīkdīkit tėk isitėkine,
kū soprantat vėsas pasiekmes.",
'movepagetalktext'        => "Sosėits aptarėma poslapis bus autuomatėškā parkelts draugom so ano, '''ėšskīrus:''':
*Poslapis nauju pavadinėmo tor netoštė aptarėma poslapi, a
*Paliksėt žemiau asontė varnale nepažīmieta.
Šėtās atviejās Tamsta sava nužiūra torėt parkeltė a apjongtė aptarėma poslapi.",
'movearticle'             => 'Parvadintė poslapi:',
'newtitle'                => 'Naus pavadėnėms:',
'move-watch'              => 'Keravuotė šėta poslapi',
'movepagebtn'             => 'Parvadintė poslapė',
'pagemovedsub'            => 'Parvadinta siekmingā',
'movepage-moved'          => '<big>\'\'\'"$1" bova parvadints i "$2"\'\'\'</big>', # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => 'Straipsnis so tuokiu vardo jau īr
a parinktāsis vards īr bluogs.
Parinkat kėta varda.',
'talkexists'              => "'''Patsā poslapis bova siekmėngā parvadints, no aptarėmu poslapis nabova parkelts, kadongi nauja
pavadėnėma straipsnis jau tor aptarėmu poslapi.
Prašuom sojongtė šėtuos poslapios.'''",
'movedto'                 => 'parvadints i',
'movetalk'                => 'Parkeltė sosėta aptarėma poslapi.',
'1movedto2'               => 'Straipsnis [[$1]] parvadints i [[$2]]',
'1movedto2_redir'         => "'$1' parvadints i '$2' (onkstiau bova nukrēpamāsis)",
'movelogpage'             => 'Parvardinėmu istuorėjė',
'movelogpagetext'         => 'Sārašos parvadintu poslapiu.',
'movereason'              => 'Prižastis:',
'revertmove'              => 'atmestė',
'delete_and_move'         => 'Ėštrintė ė parkeltė',
'delete_and_move_text'    => '==Rēkalings ėštrīnims==
Paskėrties straipsnis „[[:$1]]“ jau īr. A nuorėt ana ėštrintė, kū galietomiet parvadintė?',
'delete_and_move_confirm' => 'Tēp, trintė poslapi',
'delete_and_move_reason'  => 'Ėštrinta diel parkielima',

# Export
'export' => 'Ekspuortoutė poslapius',

# Namespace 8 related
'allmessages'               => 'Vėsė sėstemas tekstā ė pranešėmā',
'allmessagesname'           => 'Pavadėnėms',
'allmessagesdefault'        => 'Pradėnis teksts',
'allmessagescurrent'        => 'Dabartėnis teksts',
'allmessagestext'           => 'Čė pateikamė sėstemėniu pranešėmu sārašos, esontis MediaWiki srėtie.',
'allmessagesnotsupportedDB' => "'''{{ns:special}}:Allmessages''' nepalaikuoms īr, nes nustatīms '''\$wgUseDatabaseMessages''' ėšjungts īr.",
'allmessagesfilter'         => 'Tekstu pavadėnėmu atsėjuotuos:',
'allmessagesmodified'       => 'Ruodītė vėn pakeistus',

# Thumbnails
'thumbnail-more'           => 'Padėdintė',
'thumbnail_error'          => 'Klaida koriant somažėnta pavēkslieli: $1',
'thumbnail_invalid_params' => 'Nalaistieni miniatiūras parametrā',
'thumbnail_dest_directory' => 'Nepavīkst sokortė paskėrtėis papkes',

# Special:Import
'import-revision-count' => '$1 {{PLURAL:$1|versėjė|versėjės|versėju}}',

# Import log
'importlogpage'                    => 'Impuorta istuorėjė',
'import-logentry-upload-detail'    => '$1 {{PLURAL:$1|keitims|keitimā|keitimu}}',
'import-logentry-interwiki-detail' => '$1 {{PLURAL:$1|keitims|keitimā|keitimu}} ėš $2',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'Mona nauduotuojė poslapis',
'tooltip-pt-anonuserpage'         => 'Nauduotuojė poslapis Tamstas IP adresō',
'tooltip-pt-mytalk'               => 'Mona aptarėma poslapis',
'tooltip-pt-preferences'          => 'Mona nostatīmā',
'tooltip-pt-watchlist'            => 'Poslapiu sārašos, katrūs Tamsta pasėrėnkuot keravuotė.',
'tooltip-pt-mycontris'            => 'Mona darītu keitimu sārašos',
'tooltip-pt-login'                => 'Rekuomendoujam prėsėjongtė, nuors tas nėr privaluoma.',
'tooltip-pt-logout'               => 'Atsėjongtė',
'tooltip-ca-talk'                 => 'Poslapė torėnė aptarėms',
'tooltip-ca-edit'                 => 'Tamsta galėt keistė ta poslapi. Nepamėrškėt paspaustė parvaizuos mīgtoka priš ėšsauguodamė.',
'tooltip-ca-addsection'           => 'Pridietė kuomentara i aptarėma.',
'tooltip-ca-viewsource'           => 'Poslapis īr ožrakints. Galėt parveizėt torini.',
'tooltip-ca-protect'              => 'Ožrakėntė šėta poslapi',
'tooltip-ca-delete'               => 'Trėntė ta poslapi',
'tooltip-ca-move'                 => 'Parvadėntė poslapi',
'tooltip-ca-watch'                => 'Pridietė poslapi i keravuojamu sāraša',
'tooltip-ca-unwatch'              => 'Pašalėntė poslapi ėš keravuojamu sāraša',
'tooltip-search'                  => 'Ėiškuotė šėtom pruojektė',
'tooltip-p-logo'                  => 'Pėrms poslapis',
'tooltip-n-mainpage'              => 'Aplonkītė pėrma poslapi',
'tooltip-n-portal'                => 'Aple pruojekta, ka galėma vēktė, kamė ka rastė',
'tooltip-n-currentevents'         => 'Raskėt naujausė infuormacėjė',
'tooltip-n-recentchanges'         => 'Vielībūju pakeitėmu sārašos tamė projektė.',
'tooltip-n-randompage'            => 'Atidarītė bikuoki straipsni',
'tooltip-n-help'                  => 'Vėita, katruo rasėt rūpėmus atsakīmus.',
'tooltip-t-whatlinkshere'         => 'Poslapiu sārašos, ruodantiu i čė',
'tooltip-t-recentchangeslinked'   => 'Paskotėnē pakeitėmā straipsnious, pasėikiamous ėš šėta straipsnė',
'tooltip-t-contributions'         => 'Ruodītė šėta nauduotuojė keitėmu sāraša',
'tooltip-t-emailuser'             => 'Siōstė gromata šėtom prietėliō',
'tooltip-t-upload'                => 'Idietė abruozdielios a medėjės failos',
'tooltip-t-specialpages'          => 'Specēliūju poslapiu sārašos',
'tooltip-t-print'                 => 'Šėta poslapė versėjė spausdėnėmō',
'tooltip-t-permalink'             => 'Vėslaikėnė nūruoda i šėta poslapė versėje',
'tooltip-ca-nstab-user'           => 'Ruodītė nauduotuojė poslapi',
'tooltip-ca-nstab-special'        => 'Šėts poslapis īr specēlosis - anuo nagalėm redagoutė.',
'tooltip-ca-nstab-project'        => 'Ruodītė pruojekta poslapi',
'tooltip-ca-nstab-image'          => 'Ruodītė abruozdielė poslapi',
'tooltip-ca-nstab-template'       => 'Ruodītė šabluona',
'tooltip-ca-nstab-help'           => 'Ruodītė pagelbas poslapi',
'tooltip-ca-nstab-category'       => 'Ruodītė kateguorėjės poslapi',
'tooltip-minoredit'               => 'Pažīmietė pakeitėma kāp maža',
'tooltip-save'                    => 'Ėšsauguotė pakeitėmos',
'tooltip-preview'                 => 'Pakeitėmu parveiza, prašuom parveizėt priš ėšsaugont!',
'tooltip-diff'                    => 'Ruod, kuokios pakeitėmos padariet tekste.',
'tooltip-compareselectedversions' => 'Veizėtė abodvėju pasėrėnktū poslapė versėju skėrtomos.',
'tooltip-watch'                   => 'Pridietė šėta poslapi i keravuojamu sāraša',
'tooltip-recreate'                => 'Atkortė poslapi napaisant šėto, kū ans bova ėštrints',

# Attribution
'anonymous'        => 'Neregėstrouts nauduotuos',
'lastmodifiedatby' => 'Šėta poslapi paskotini karta redagava $3 $2, $1.', # $1 date, $2 time, $3 user
'others'           => 'kėtė',
'creditspage'      => 'Poslapė kūriejē',

# Spam protection
'spamprotectiontitle' => 'Prišreklamėnis fėltros',
'spamprotectiontext'  => 'Poslapis, katra nuoriejot ėšsauguotė bova ožblokouts prišreklamėnė fėltra. Šėtā tėkriausē sokielė nūruoda i kėta svetaine. Ėšėmkit nūruoda ė pamieginkėt viel.',
'spamprotectionmatch' => 'Šėts tekstos bova atpažėnts prišreklamėnė fėltra: $1',
'spambot_username'    => "''MediaWiki'' reklamu šalėnėms",
'spam_reverting'      => 'Atkoriama i onkstesne versėje, katra nator nūruodu i $1',
'spam_blanking'       => 'Vėsos versėjės toriejė nūruodu i $1. Ėšvaluoma',

# Info page
'numedits'     => 'Pakeitimu skaitlius (straipsnis): $1',
'numtalkedits' => 'Pakeitėmu skaitlius (aptarėma poslapis): $1',
'numwatchers'  => 'Keravuojantiu skaitlius: $1',

# Math options
'mw_math_png'    => 'Vėsumet fuormuotė PNG',
'mw_math_simple' => 'HTML paprastās atvejās, kėtēp - PNG',
'mw_math_html'   => 'HTML kumet imanuoma, kėtēp - PNG',
'mw_math_source' => 'Paliktė TeX fuormata (tekstinems naršīklems)',
'mw_math_modern' => 'Rekomendounama muodernioms naršīklems',
'mw_math_mathml' => 'MathML jēgo imanuoma (ekspermentinis)',

# Patrolling
'markaspatrolledtext'   => 'Pažīmietė, ka poslapis patėkrėnts īr',
'markedaspatrolled'     => 'Pažīmiets kāp patėkrints',
'markedaspatrolledtext' => 'Pasėrinkta versėjė siekmingā pažīmieta kāp patėkrinta',

# Patrol log
'patrol-log-page' => 'Patikrinėma istuorėjė',
'patrol-log-line' => 'Poslapė „$2“ $1 pažīmieta kāp patėkrinta $3',
'patrol-log-auto' => '(autuomatėškā)',
'patrol-log-diff' => 'versėjė $1',

# Image deletion
'deletedrevision' => 'Ėštrinta sena versėjė $1.',

# Browsing diffs
'previousdiff' => '← Onkstesnis pakeitėms',
'nextdiff'     => 'Paskesinis pakeitėms →',

# Media information
'mediawarning'         => "'''Diemesė''': Šėts fails gal torietė kenksmėnga kuoda, anū palaidus Tamstas sėstėma gal būtė sogadinta.<hr />",
'thumbsize'            => 'Somažėntu pavēkslieliu didums:',
'widthheightpage'      => '$1×$2, $3 poslapē',
'file-info'            => '(faila dėdoms: $1, MIME tips: $2)',
'file-info-size'       => '($1 × $2 taškū, faila dėdoms: $3, MIME tips: $4)',
'file-nohires'         => '<small>Geresnis ėšraiškėms negalėms.</small>',
'svg-long-desc'        => '(SVG fails, fuormalē $1 × $2 puškiu, faila dėdoms: $3)',
'show-big-image'       => 'Pėlns ėšraiškėms',
'show-big-image-thumb' => '<small>Šėtuos parvaizos dėdums: $1 × $2 puškiu</small>',

# Special:NewImages
'newimages'             => 'Naujausiu abruozdieliu galerėjė',
'imagelisttext'         => "Žemiau īr '''$1''' failu sārašos, sorūšiouts $2.",
'showhidebots'          => '($1 robotos)',
'ilsubmit'              => 'Ėiškoutė',
'bydate'                => 'palē data',
'sp-newimages-showfrom' => 'Ruodītė naujus abruozdielius pradedant nū $2, $1',

# Bad image list
'bad_image_list' => 'Fuormats tuoks īr:

Tėk eilotės, prasėdedantės *, īr itraukiamas. Pėrmuojė nūruoda eilotie tor būtė nūruoda i bluoga abruozdieli.
Vėsas kėtas nūoruodas tuo patiuo eilotie īr laikomas ėšėmtim, tas rēšk ka poslapē, katrūs leidama iterptė abruozdieli.',

# Metadata
'metadata'          => 'Metadoumenīs',
'metadata-help'     => 'Šėtom failė īr papėlduomos infuormacėjės, tikriausē pridietos skaitmeninės kameruos a skanėrė, katros bova nauduots anam sokortė a parkeltė i skaitmenėni fuormata. Jēgo fails bova pakeists ėš pradėnės versėjės, katruos nekatruos datalės gal nepėlnā atspėndietė nauja faila.',
'metadata-expand'   => 'Ruodītė ėšpliestinė infuormacėjė',
'metadata-collapse' => 'Kavuotė ėšpliestinė infuormacėjė',
'metadata-fields'   => 'EXIF metadoumenū laukā, nuruodītė tamė pranešėmė, bus itrauktė i abruozdielė poslapi, kumet metadoumenū lentelė bus suskleista. Palē nutīliejėma kėtė laukā bus pakavuotė.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-orientation'            => 'Pasokims',
'exif-xresolution'            => 'Gorizuontalus ėšraiškėms',
'exif-datetime'               => 'Faila keitėma data ė čiesos',
'exif-imagedescription'       => 'Abruozdielė pavadėnėms',
'exif-make'                   => 'Kameras gamėntuos',
'exif-model'                  => 'Kameras muodelis',
'exif-colorspace'             => 'Spalvū pristatīms',
'exif-compressedbitsperpixel' => 'Abruozdielė sospaudėma rėžėms',
'exif-datetimeoriginal'       => 'Doumenū generavėma data ė čiesos',
'exif-exposuretime'           => 'Ėšlaikīma čiesos',
'exif-fnumber'                => 'F skaitlius',
'exif-brightnessvalue'        => 'Švėisoms',
'exif-lightsource'            => 'Švėisuos šaltėnis',
'exif-flash'                  => 'Blėcos',
'exif-focallength'            => 'Žėdinė nutuolėms',
'exif-flashenergy'            => 'Blėca energėjė',
'exif-contrast'               => 'Kuontrasts',

'exif-orientation-1' => 'Standartėšks', # 0th row: top; 0th column: left

'exif-xyresolution-i' => '$1 puškē cuolī',
'exif-xyresolution-c' => '$1 puškē centėmetrė',

'exif-exposureprogram-0' => 'Nenūruodīta',

'exif-contrast-1' => 'Mažos',
'exif-contrast-2' => 'Dėdlis',

# External editor support
'edit-externally'      => 'Atdarītė ėšuoriniam redaktuorio',
'edit-externally-help' => 'Nuoriedamė gautė daugiau infuormacėjės, veiziekėt [http://www.mediawiki.org/wiki/Manual:External_editors kruovėma instrokcėjės].',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'vėsos',
'imagelistall'     => 'vėsė',
'watchlistall2'    => 'vėsos',
'namespacesall'    => 'vėsas',
'monthsall'        => 'vėsė',

# E-mail address confirmation
'confirmemail_loggedin' => 'Tamstas el. pašta adresos ožtvėrtints īr.',

# Trackbacks
'trackbackremove' => ' ([$1 Trintė])',

# Delete conflict
'deletedwhileediting' => 'Diemesė: Šėts poslapis ėštrints po šėta, kumet pradiejot redagoutė!',
'recreate'            => 'Atkortė',

# HTML dump
'redirectingto' => 'Paradresounama i [[:$1]]...',

# action=purge
'confirm_purge_button' => 'Tink',

# AJAX search
'searchcontaining' => "Ėiškuotė straipsniu, katrė prasided ''$1''.",
'searchnamed'      => "Ėiškuotė straipsniu, so pavadėnėmu ''$1''.",
'articletitles'    => "Straipsnē, pradedont nu ''$1''",
'hideresults'      => 'Kavuotė rezoltatus',

# Multipage image navigation
'imgmultipageprev' => '← onkstesnis poslapis',
'imgmultipagenext' => 'kėts poslapis →',

# Table pager
'ascending_abbrev'         => 'dėdiejėma tvarka',
'descending_abbrev'        => 'mažiejontė tvarka',
'table_pager_next'         => 'Kėts poslapis',
'table_pager_prev'         => 'Onkstesnis poslapis',
'table_pager_first'        => 'Pėrms poslapis',
'table_pager_last'         => 'Paskotėnis poslapis',
'table_pager_limit'        => 'Ruodītė $1 elementu par poslapi',
'table_pager_limit_submit' => 'Ruodītė',
'table_pager_empty'        => 'Juokiū rezoltatu',

# Auto-summaries
'autosumm-blank'   => 'Šalėnams ciels straipsnė torėnīs',
'autosumm-replace' => "Poslapis keitams so '$1'",
'autoredircomment' => 'Nukreipama i [[$1]]',
'autosumm-new'     => 'Naus poslapis: $1',

# Live preview
'livepreview-loading' => 'Kraunama īr…',
'livepreview-ready'   => 'Ikeliama… Padarīta!',

# Friendlier slave lag warnings
'lag-warn-normal' => 'Pakeitėmā, naujesnė nego $1 sekondiu, šėtom sārašė gal būtė neruodomė.',

# Watchlist editor
'watchlistedit-numitems'       => 'Tamstas keravuojamu sārašė īr $1 poslapiu neskaitliuojant aptarėmu poslapiu.',
'watchlistedit-noitems'        => 'Tamstas keravuojamu sārašė nė juokiū poslapiu.',
'watchlistedit-normal-title'   => 'Redagoutė stebimūju sarōša',
'watchlistedit-normal-legend'  => 'Šalėntė poslapios ėš keravuojamu sāraša',
'watchlistedit-normal-explain' => 'Žemiau īr ruodomė poslapē Tamstas keravuojamu sārašė.
Nuoriedamė pašalėntė poslapi, pri anuo oždiekėt varnale ė paspauskėt „Šalėntė poslapios“.
Tamsta tēpuogi galėt [[Special:Watchlist/raw|redagoutė grīnaji keravuojamu sāraša]].',
'watchlistedit-normal-submit'  => 'Šalėntė poslapios',
'watchlistedit-normal-done'    => '$1 poslapiu bova pašalėnta ėš Tamstas keravuojmu sāraša:',
'watchlistedit-raw-title'      => 'Keistė grīnōjė keravuojamu sāraša',
'watchlistedit-raw-legend'     => 'Keistė grīnōjė keravuojamu sāraša',
'watchlistedit-raw-explain'    => 'Žemiau ruodomė poslapē Tamstas keravuojamu sārašė, ė gal būtė pridietė i a pašalėntė ėš sāraša; vėins poslapis eilotie. Bėngė paspauskėt „Atnaujėntė keravuojamu sāraša“. Tamsta tēpuogi galėt [[Special:Watchlist/edit|nauduotė standartėni radaktuoriu]].',
'watchlistedit-raw-titles'     => 'Poslapē:',
'watchlistedit-raw-submit'     => 'Atnaujėntė keravuojamu sāraša',
'watchlistedit-raw-done'       => 'Tamstas keravuojamu sārošos bova atnaujėnts.',
'watchlistedit-raw-added'      => '$1 poslapiu bova pridiet:',
'watchlistedit-raw-removed'    => '$1 poslapiu bova pašalėnt:',

# Watchlist editing tools
'watchlisttools-view' => 'Veizietė sosėjosius pakeitėmus',
'watchlisttools-edit' => 'Veizietė ėr keistė keravuojamu straipsniu sāraša',
'watchlisttools-raw'  => 'Keistė nebėngta keravuojamu straipsniu sāraša',

# Special:Version
'version' => 'Versėjė', # Not used as normal message but as header for the special page itself

# Special:FilePath
'filepath' => 'Faila maršrots',

# Special:SpecialPages
'specialpages' => 'Specēlė̅jė poslapē',

);
