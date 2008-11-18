<?php
/** Jutish (Jysk)
 *
 * @ingroup Language
 * @file
 *
 * @author Huslåke
 * @author Ælsån
 */

$fallback = 'da';

$messages = array(
# User preference toggles
'tog-underline'               => 'Understreg henvesnenger',
'tog-highlightbroken'         => 'Brug røde henvesnenger til tømme sider',
'tog-justify'                 => 'Ves ertikler ve lege margener',
'tog-hideminor'               => "Skjul mendre ændrenger i'n liste åver seneste ændrenger",
'tog-extendwatchlist'         => 'Udvedet liste ve seneste ændrenger',
'tog-usenewrc'                => 'Førbedret liste åver seneste ændrenger (JavaScript)',
'tog-numberheadings'          => 'Åtåmatisk nåmererenge åf åverskrefter',
'tog-showtoolbar'             => 'Ves værktøjslenje til redigærenge (JavaScript)',
'tog-editondblclick'          => 'Redigær sider ve dåbeltklik (JavaScript)',
'tog-editsection'             => 'Redigær åfsnet ve hjælp åf [redigær]-henvesnenger',
'tog-editsectiononrightclick' => 'Redigær åfsnet ve at klikke på deres titler (JavaScript)',
'tog-showtoc'                 => 'Ves endholtsførtegnelse (i artikler ve mære end tre åfsnet)',
'tog-rememberpassword'        => 'Husk adgengskode til næste besøĝ frå denne kompjuter',
'tog-editwidth'               => 'Redigærengsboksen har fuld bredde',
'tog-watchcreations'          => 'Tilføj sider a åpretter til miin åvervågnengsliste',
'tog-watchdefault'            => 'Tilføj sider a redigærer til miin åvervågnengsliste',
'tog-watchmoves'              => 'Tilføj sider a flytter til miin åvervågnengsliste',
'tog-watchdeletion'           => 'Tilføj sider a sletter til miin åvervågnengsliste',
'tog-minordefault'            => 'Markær søm standård ål redigærenge søm mendre',
'tog-previewontop'            => 'Ves førhåndsvesnenge åver æ rædigerengsboks',
'tog-previewonfirst'          => 'Ves førhåndsvesnenge når du stårtst ve at redigære',
'tog-nocache'                 => 'Slå caching åf sider frå',
'tog-enotifwatchlistpages'    => 'Send mig en e-mail ve sideændrenger',
'tog-enotifusertalkpages'     => 'Send mig en e-mail når miin brugerdiskusjeside ændres',
'tog-enotifminoredits'        => 'Send mig også en e-mail ve mendre ændrenger åf åvervågede sider',
'tog-enotifrevealaddr'        => "Ves miin e-mail-adresse i mails ve besked ændrenger'm",
'tog-shownumberswatching'     => 'Ves åntal brugere, der åvervåger',
'tog-fancysig'                => 'Signaturer uden åtåmatisk henvesnenge',
'tog-externaleditor'          => 'Brug ekstern redigærengsmåskiin åtåmatisk',
'tog-externaldiff'            => 'Brug ekstern førskelsvesnenge åtåmatisk',
'tog-showjumplinks'           => 'Ves tilgængelegheds-henvesnenger',
'tog-uselivepreview'          => 'Brug åtåmatisk førhåndsvesnenge (JavaScript) (eksperimentel)',
'tog-forceeditsummary'        => 'Advar, hves sammenfatnenge mangler ve gemnenge',
'tog-watchlisthideown'        => "Skjul egne ændrenger i'n åvervågnengsliste",
'tog-watchlisthidebots'       => "Skjul ændrenger frå bots i'n åvervågnengsliste",
'tog-watchlisthideminor'      => "Skjul mendre ændrenger i'n åvervågnengsliste",
'tog-ccmeonemails'            => 'Send mig kopier åf e-mails, søm a sender til andre brugere.',
'tog-diffonly'                => "Ves ve versjesammenlegnenger kun førskelle, ekke'n hele side",
'tog-showhiddencats'          => 'Ves skjulte klynger',

'underline-always'  => 'åltid',
'underline-never'   => 'åldreg',
'underline-default' => 'æfter brovserendstellenge',

'skinpreview' => '(Førhåndsvesnenge)',

# Dates
'sunday'        => 'søndåg',
'monday'        => 'måndåg',
'tuesday'       => 'tirsdåg',
'wednesday'     => 'ønsdåg',
'thursday'      => 'tårsdåg',
'friday'        => 'fredåg',
'saturday'      => 'lørsdåg',
'sun'           => 'søn',
'mon'           => 'mån',
'tue'           => 'tir',
'wed'           => 'øns',
'thu'           => 'tår',
'fri'           => 'fre',
'sat'           => 'lør',
'january'       => 'januar',
'february'      => 'februar',
'march'         => 'mårts',
'april'         => 'åpril',
'may_long'      => 'mæ',
'june'          => 'juni',
'july'          => 'juli',
'august'        => 'ågust',
'september'     => 'september',
'october'       => 'oktober',
'november'      => 'november',
'december'      => 'desember',
'january-gen'   => 'januars',
'february-gen'  => 'februars',
'march-gen'     => 'mårtses',
'april-gen'     => 'åprils',
'may-gen'       => 'mæs',
'june-gen'      => 'juniis',
'july-gen'      => 'juliis',
'august-gen'    => 'ågusts',
'september-gen' => 'septembers',
'october-gen'   => 'oktobers',
'november-gen'  => 'novembers',
'december-gen'  => 'desembers',
'jan'           => 'jan',
'feb'           => 'feb',
'mar'           => 'mår',
'apr'           => 'åpr',
'may'           => 'mæ',
'jun'           => 'jun',
'jul'           => 'jul',
'aug'           => 'ågu',
'sep'           => 'sep',
'oct'           => 'okt',
'nov'           => 'nov',
'dec'           => 'des',

# Categories related messages
'pagecategories'           => '{{PLURAL:$1|Klynge|Klynger}}',
'category_header'          => 'Ertikler i\'n klynge "$1"',
'subcategories'            => 'Underklynger',
'category-media-header'    => "Medier i'n klynge „$1“",
'category-empty'           => "''Denne klynge endeholter før øjeblikket æ verke sider æller medie-gøret.''",
'hidden-categories'        => '{{PLURAL:$1|Skjult klynge|Skjulte klynger}}',
'hidden-category-category' => 'Skjulte klynger', # Name of the category where hidden categories will be listed
'listingcontinuesabbrev'   => 'førtgøte',

'mainpagetext'      => 'MediaWiki er nu installeret.',
'mainpagedocfooter' => "Se vores engelskspråĝede [http://meta.wikimedia.org/wiki/MediaWiki_localisation dokumentåsje tilpasnenge'm åf æ brugergrænseflade] og [http://meta.wikimedia.org/wiki/MediaWiki_User%27s_Guide æ brugervejlednenge] før åplysnenger åpsætnenge'm og anvendelse.",

'about'          => 'Åm',
'article'        => 'Ertikel',
'newwindow'      => '(åbner i et nyt vendue)',
'cancel'         => 'Åfbryd',
'qbfind'         => 'Søĝ',
'qbbrowse'       => 'Gennemse',
'qbedit'         => 'Redigær',
'qbpageoptions'  => 'Endstellenger før side',
'qbpageinfo'     => "Informåsje side'm",
'qbmyoptions'    => 'Miine endstellenger',
'qbspecialpages' => 'Sonst sider',
'moredotdotdot'  => 'Mære...',
'mypage'         => 'Miin side',
'mytalk'         => 'Min diskusje',
'anontalk'       => 'Diskusjeside før denne IP-adresse',
'navigation'     => 'Navigasje',
'and'            => 'og',

# Metadata in edit box
'metadata_help' => 'Metadata:',

'errorpagetitle'    => 'Fejl',
'returnto'          => 'Tilbage til $1.',
'tagline'           => 'Frå {{SITENAME}}',
'help'              => 'Hjælp',
'search'            => 'Søĝ',
'searchbutton'      => 'Søĝ',
'go'                => 'Gå til',
'searcharticle'     => 'Gå til',
'history'           => 'Skigt',
'history_short'     => 'Skigte',
'updatedmarker'     => '(ændret)',
'info_short'        => 'Informåsje',
'printableversion'  => 'Utskreftsvelig utgåf',
'permalink'         => 'Permanent henvesnenge',
'print'             => 'Udskrev',
'edit'              => 'Redigær',
'create'            => 'Skep',
'editthispage'      => 'Redigær side',
'create-this-page'  => 'Skep denne side',
'delete'            => 'Slet',
'deletethispage'    => 'Slet side',
'undelete_short'    => 'Førtryd sletnenge åf {{PLURAL:$1|$1 versje|$1 versje}}',
'protect'           => 'Beskyt',
'protect_change'    => 'Ændret beskyttelse',
'protectthispage'   => 'Beskyt side',
'unprotect'         => 'Fjern beskyttelse',
'unprotectthispage' => 'Frigæv side',
'newpage'           => 'Ny side',
'talkpage'          => 'Diskusje',
'talkpagelinktext'  => 'diskusje',
'specialpage'       => 'Sonst side',
'personaltools'     => "Personlige værktø'r",
'postcomment'       => 'Tilføj en biskrevselenger',
'articlepage'       => "Se'n ertikel",
'talk'              => 'Diskusje',
'views'             => 'Vesnenger',
'toolbox'           => "Værktø'r",
'userpage'          => "Se'n brugerside",
'projectpage'       => "Se'n projektside",
'imagepage'         => "Se'n billetside",
'mediawikipage'     => 'Vese endholtsside',
'templatepage'      => 'Vese skablånside',
'viewhelppage'      => 'Vese hjælpeside',
'categorypage'      => 'Vese klyngeside',
'viewtalkpage'      => "Se'n diskusje",
'otherlanguages'    => 'Andre språĝ',
'redirectedfrom'    => '(Åmstyret frå $1)',
'redirectpagesub'   => 'Åmstyrenge',
'lastmodifiedat'    => 'Denne side blev senest ændret den $2, $1.', # $1 date, $2 time
'viewcount'         => 'Æ side er vest i alt $1 {{PLURAL:$1|geng|genger}}.',
'protectedpage'     => 'Beskyttet side',
'jumpto'            => 'Skeft til:',
'jumptonavigation'  => 'navigasje',
'jumptosearch'      => 'Søĝnenge',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => "{{SITENAME}}'m",
'aboutpage'            => 'Project:Åm',
'bugreports'           => 'Fejlgåde',
'bugreportspage'       => 'Project:Fejlgåde',
'copyright'            => 'Endholtet er udgævet under $1.',
'copyrightpagename'    => '{{SITENAME}} åphavsret',
'copyrightpage'        => '{{ns:project}}:Åphavsret',
'currentevents'        => 'Nænte begevenheder',
'currentevents-url'    => 'Project:Nænte begevenheder',
'disclaimers'          => 'Førbeholt',
'disclaimerpage'       => 'Project:Huses førbeholt',
'edithelp'             => 'Hjælp til redigærenge',
'edithelppage'         => "Help:Vordan redigærer a'n side",
'faq'                  => 'VSF',
'faqpage'              => 'Project:Vøl stellen fråĝer (VSF)',
'helppage'             => 'Help:Hjælpførside',
'mainpage'             => 'Førsit',
'mainpage-description' => 'Førsit',
'policy-url'           => 'Project:Politik',
'portal'               => 'Førside før skrebenter',
'portal-url'           => 'Project:Førside før skrebenter',
'privacy'              => 'Behandlenge åf personlige åplysnenger',
'privacypage'          => 'Project:Behandlinge åf personlige åplysnenger',

'badaccess'        => 'Manglende rettigheder',
'badaccess-group0' => 'Du harst ekke de nødvendege rettegheder til denne håndlenge.',
'badaccess-group1' => "Denne håndlenge ken kun udføres åf brugere, søm tilhører'n gruppe „$1“.",
'badaccess-group2' => 'Denne håndlenge ken kun udføres åf brugere, søm tilhører en åf grupperne „$1“.',
'badaccess-groups' => 'Denne håndlenge ken kun udføres åf brugere, søm tilhører en åf grupperne „$1“.',

'versionrequired'     => 'Kræver versje $1 åf MediaWiki',
'versionrequiredtext' => "Versje $1 åf MediaWiki er påkrævet, før at bruge denne side. Se'n [[Special:Version|versjeside]]",

'ok'                      => 'Er åkæ',
'retrievedfrom'           => 'Hæntet frå "$1"',
'youhavenewmessages'      => 'Du har $1 ($2).',
'newmessageslink'         => 'nye beskeder',
'newmessagesdifflink'     => 'ændrenger æ side sedste vesnenge',
'youhavenewmessagesmulti' => 'Der er nye meddelelser til dig: $1',
'editsection'             => 'redigær',
'editold'                 => 'redigær',
'viewsourceold'           => 'ves æ kelde',
'editsectionhint'         => 'Redigær åfsnet: $1',
'toc'                     => 'Endholtsførtegnelse',
'showtoc'                 => 'ves',
'hidetoc'                 => 'skjul',
'thisisdeleted'           => 'Se æller gendan $1?',
'viewdeleted'             => 'Ves $1?',
'restorelink'             => '{{PLURAL:$1|en slettet ændrenge|$1 slettede ændrenger}}',
'feedlinks'               => 'Fiid:',
'feed-invalid'            => 'Ugyldeg abånmentstype.',
'feed-unavailable'        => 'RSS og Atåm fiid er ekke tilgængelege på {{SITENAME}}',
'site-rss-feed'           => '$1 RSS-fiid',
'site-atom-feed'          => '$1 Atom-fiid',
'page-rss-feed'           => '"$1" RSS-fiid',
'page-atom-feed'          => '"$1" Atom-fiid',
'red-link-title'          => '$1 (ekke skrevet endnu)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'ertikel',
'nstab-user'      => 'brugerside',
'nstab-media'     => 'medie',
'nstab-special'   => 'sonst',
'nstab-project'   => 'åm',
'nstab-image'     => 'billet',
'nstab-mediawiki' => 'besked',
'nstab-template'  => 'skablån',
'nstab-help'      => 'hjælp',
'nstab-category'  => 'Klynge',

# Main script and global functions
'nosuchaction'      => 'Æ funksje fendes ekke',
'nosuchactiontext'  => "Funksje ångævet i'n URL ken ekke genkendes åf æ MediaWiki-softwær",
'nosuchspecialpage' => 'En sådan sonstside fendes ekke',
'nospecialpagetext' => "Du harst bedt en sonstside'm, der ekke ken genkendes åf æ MediaWiki-softwær.",

# General errors
'error'                => 'Fejl',
'databaseerror'        => 'Databasefejl',
'dberrortext'          => 'Der er åpstået en syntaksfejl i en databaseførespørgsel.
Dette ken være på grund åf en ugyldeg førespørgsel,
æller det ken betyde en fejl i\'n softwær. 
Den seneste førsøĝte databaseførespørgsel var:
<blockquote><tt>$1</tt></blockquote>
frå\'n funksje "<tt>$2</tt>". 
MySQL æ returnerede fejl "<tt>$3: $4</tt>".',
'dberrortextcl'        => 'Der er åpstået en syntaksfejl i en databaseførespørgsel. 
Den seneste førsøĝte databaseførespørgsel var: "$1" frå\'n funksje "$2". 
MySQL æ returnerede fejl "$3: $4".',
'noconnect'            => "Der er pråblæmer ve {{SITENAME}} han database, vi kan desværre ekke kåm i kontakt ve den før øjeblikket. Prøv ig'n senere. <br />$1",
'nodb'                 => "Kan ekke vælge'n database $1",
'cachederror'          => 'Det følgende er en gemt kopi åf den ønskede side, og er måske ekke helt åpdateret.',
'laggedslavemode'      => 'Bemærk: Den veste side endholter mulegves ekke de nyeste ændrenger.',
'readonly'             => 'Æ database er skrevebeskyttet',
'enterlockreason'      => "Skrev en begrundelse før æ skrevebeskyttelse, ve samt en vurderenge åf, hvornår æ skrevebeskyttelse åphæves ig'n",
'readonlytext'         => 'Æ database er midlertedegt skrevebeskyttet. Førsøĝ venlegst senere.

Årsag til æ spærrenge: $1',
'readonly_lag'         => "Æ database er åtåmatisk blevet låst mens slæfdatabaseserverne synkroniserer ve'n master database",
'internalerror'        => 'Intern fejl',
'internalerror_info'   => 'Intern fejl: $1',
'filecopyerror'        => 'Kan ekke kopiere\'n file "$1" til "$2".',
'filerenameerror'      => 'Kan ekke omdøbe\'n file "$1" til "$2".',
'filedeleteerror'      => 'Kan ekke slette\'n file "$1".',
'directorycreateerror' => 'Kan ekke åprette katalåget "$1".',
'filenotfound'         => 'Kan ekke finde\'n file "$1".',
'fileexistserror'      => 'Kan ekke åprette "$1": æ file findes ålrede',
'unexpected'           => 'Uventet værdi: "$1"="$2".',
'formerror'            => 'Fejl: Kan ekke åfsende formulær',
'badtitle'             => 'Førkert skrevselenger',
'badtitletext'         => 'Den ønskede sides nav var ekke tilladt, tøm æller æ side er førkert henvest frå en {{SITENAME}} på et andet språĝ.',
'wrong_wfQuery_params' => 'Ugyldeg paramæter til wfQuery()<br />
Funksje: $1<br />
Førespørgsel: $2',
'viewsource'           => 'Ves æ kelde',
'viewsourcefor'        => 'før $1',
'viewsourcetext'       => "Du ken dog se og åfskreve'n keldekode til æ side:",

# Login and logout pages
'yourname'                => 'Dit brugernav',
'yourpassword'            => 'Din adgangskode',
'remembermypassword'      => 'Husk min adgangskode til næste gang.',
'login'                   => 'Loĝ på',
'nav-login-createaccount' => 'Åpret æ konto æller loĝ på',
'loginprompt'             => 'Du skal have cookies slået til før at kunne loĝge på {{SITENAME}}.',
'userlogin'               => 'Åpret æ konto æller loĝ på',
'logout'                  => 'Loĝ åf',
'userlogout'              => 'Loĝ åf',
'nologin'                 => 'Du har engen brugerkonto? $1.',
'nologinlink'             => 'Åpret ny brugerkonto',
'createaccount'           => 'Åpret en ny konto',
'gotaccount'              => 'Du har ålerede en brugerkonto? $1.',
'gotaccountlink'          => 'Loĝ på',
'youremail'               => 'E-mail:',
'yourrealname'            => 'Dit rigtege navn*',
'prefs-help-realname'     => '* <strong>Dit rigtege navn</strong> (valgfrit): Hves du vælger at åplyse dit navn hvil dette bleve brugt til at tilskreve dig dit arbejde.',
'loginsuccesstitle'       => 'Du er nu loĝget på',
'loginsuccess'            => 'Du er nu loĝget på {{SITENAME}} søm "$1".',
'nosuchuser'              => 'Der er ig\'n bruger ve navnet "$1". Kontrollér æ stavemåde ig\'n, æller brug æ formulår herunder til at åprette en ny brugerkonto.',
'nosuchusershort'         => 'Der er ig\'n bruger ve navn "<nowiki>$1</nowiki>". Tjek din stavnenge.',
'nouserspecified'         => 'Angæv venlegst et brugernavn.',
'wrongpassword'           => "Den endtastede adgangskode var førkert. Prøv ig'n.",
'wrongpasswordempty'      => "Du glemte at endtaste password. Prøv ig'n.",
'passwordtooshort'        => 'Dit kodeort er før kårt. Det skal være mendst $1 tegn langt.',
'mailmypassword'          => 'Send et nyt adgangskode til min e-mail-adresse',
'passwordremindertitle'   => 'Nyt password til {{SITENAME}}',
'passwordremindertext'    => 'Nogen (sandsynlegves dig, frå\'n IP-addresse $1)
har bedt at vi sender dig en ny adgangskode til at loĝge på {{SITENAME}} ($4)\'m.
Æ adgangskode før bruger "$2" er nu "$3".
Du bør loĝge på nu og ændre din adgangskode.,

Hves en anden har bestilt den nye adgangskode æller hves du er kåmet i tanke dit gamle password og ekke mære vil ændre det\'m, 
kenst du bare ignorere denne mail og førtsætte ve at bruge dit gamle password.',
'noemail'                 => 'Der er ekke åplyst en e-mail-adresse før bruger "$1".',
'passwordsent'            => 'En ny adgangskode er sendt til æ e-mail-adresse,
søm er registræret før "$1".
Du bør loĝge på og ændre din adgangskode straks æfter du harst modtaget æ e-mail.',
'eauthentsent'            => 'En bekrftelsesmail er sendt til den angævne e-mail-adresse.

Før en e-mail ken modtages åf andre brugere åf æ {{SITENAME}}-mailfunksje, skel æ adresse og dens tilhørsførholt til denne bruger bekræftes. Følg venlegst anvesnengerne i denne mail.',

# Edit page toolbar
'bold_sample'     => 'Fed skrevselenger',
'bold_tip'        => 'Fed skrevselenger',
'italic_sample'   => 'Skyn skrevselenger',
'italic_tip'      => 'Skyn skrevselenger',
'link_sample'     => 'Henvesnenge',
'link_tip'        => 'Ensende henvesnenge',
'extlink_sample'  => 'http://www.example.com Skrevselenger på henvesnenge',
'extlink_tip'     => 'Utsende henvesnenge (husk http:// førgøret)',
'headline_sample' => 'Skrevselenger til åverskreft',
'headline_tip'    => 'Skå 2 åverskreft',
'math_sample'     => 'Endsæt åpstælsel her (LaTeX)',
'math_tip'        => 'Matematisk åpstælsel (LaTeX)',
'nowiki_sample'   => 'Endsæt skrevselenger her søm ekke skal redigær påke wikiskrevselenger',
'nowiki_tip'      => 'Ekke wikiskrevselenger utse',
'image_tip'       => 'Endlejret billet',
'media_tip'       => 'Henvesnenge til multimediagøret',
'sig_tip'         => 'Din håndstep ve tidsstep',
'hr_tip'          => 'Plat lenje (brug den sparsåmt)',

# Edit pages
'summary'                => 'Beskrevelse',
'subject'                => 'Emne/åverskreft',
'minoredit'              => "Dette'r en mendre æller lile ændrenge.",
'watchthis'              => 'Åvervåg denne ertikel',
'savearticle'            => 'Gem side',
'preview'                => 'Førhåndsvesnenge',
'showpreview'            => 'Førhåndsvesnenge',
'showdiff'               => 'Ves ændrenger',
'anoneditwarning'        => "Du arbejder uden at være loĝget på. Estedet før brugernav veses så'n IP-adresse i'n hersenengerskigt.",
'summary-preview'        => 'Førhåndsvesnenge åf beskrevelselejne',
'blockedtext'            => "<big>'''Dit brugernav æller din IP-adresse er blevet blokeret.'''</big>

Æ blokerenge er lavet åf $1. Æ begrundelse er ''$2''.

Æ blokerenge starter: $8
Æ blokerenge udløber: $6
Æ blokerenge er rettet mod: $7

Du ken kåle $1 æller en åf de andre [[{{MediaWiki:Grouppage-sysop}}|administratårer]] før at diskutere æ blokerenge.
Du ken ekke bruge æ funksje 'e-mail til denne bruger' vemendre der er ångevet en gyldig email-addresse i dine
[[Special:Preferences|kontoendstellenger]]. Din nuværende IP-addresse er $3, og blokerengs-ID er #$5. Ångev venlegst en æller begge i åle henvendelser.",
'newarticle'             => '(Ny)',
'newarticletext'         => "'''{{SITENAME}} har endnu ekke nogen {{NAMESPACE}}-side ve nav {{PAGENAME}}.'''<br /> Du ken begynde en side ve at skreve i'n boks herunder. (se'n [[{{MediaWiki:Helppage}}|hjælp]] før yderligere åplysnenger).<br /> Æller du ken [[Special:Search/{{PAGENAME}}|søĝe æfter {{PAGENAME}} i {{SITENAME}}]].<br /> Ves det ekke var din meneng, så tryk på æ '''Tilbage'''- æller æ '''Back'''-knåp.",
'noarticletext'          => "'''{{SITENAME}} har ekke nogen side ve prånt dette nav.''' 
* Du ken '''[{{fullurl:{{FULLPAGENAME}}|action=edit}} starte æ side {{PAGENAME}}]''' 
* Æller [[Special:Search/{{PAGENAME}}|søĝe æfter {{PAGENAME}}]] i andre ertikler 
---- 
* Ves du har åprettet denne ertikel endenfør de sedste få minutter, så ken de skyldes at der er ledt førsenkelse i'n åpdaterenge åf {{SITENAME}}s cache. Vent venligst og tjek igen senere'n ertikel'm dukker åp, enden du førsøĝer at åprette'n ertikel igen.",
'previewnote'            => '<strong>Husk at dette er kun en førhåndsvesnenge, æ side er ekke gemt endnu!</strong>',
'editing'                => 'Redigærer $1',
'editingsection'         => 'Redigærer $1 (åfsnet)',
'copyrightwarning'       => "<strong>Husk: <big>åpskrev engen websider</big>, søm ekke tilhører dig selv, brug <big>engen åphavsretsligt beskyttede værker</big> uden tilladelse frå'n ejer!</strong><br />
Du lover os hermed, at du selv <strong>har skrevet skrevselenger</strong>, at skrevselenger tilhører ålmenheden, er (<strong>åpværer hus</strong>), æller at æ <strong>åphavsrets-endehaver</strong> har gevet sen <strong>tilladelse</strong>. Ves denne skrevselenger ålerede er åfentliggkort andre steder, skrev det venligst på æ diskusjesside.
<i>Bemærk venligst, at ål {{SITENAME}}-ertikler åtomatisk står under „$2“ (se $1 før lileskrevselenger). Ves du ekke vel, at dit arbejde her ændres og udbredes åf andre, så tryk ekke på „Gem“.</i>",
'longpagewarning'        => "<strong>ADVARSEL: Denne side er $1 kilobyte stor; nogle browsere ken have pårblæmer ve at redigære sider der nærmer sig æller er større end 32 Kb. 
Åvervej æ side'm ken åpdeles i mendre dæle.</strong>",
'templatesused'          => 'Skablåner der er brugt på denne side:',
'templatesusedpreview'   => 'Følgende skablåner bruges åf denne ertikelførhåndsvesnenge:',
'template-protected'     => '(skrevebeskyttet)',
'template-semiprotected' => '(skrevebeskyttet før ekke ånmeldte og nye brugere)',
'nocreatetext'           => "Æ'n åpdiin har begrænset åprettelse åf nye sider. Bestående sider ken ændres æller [[Special:UserLogin|loĝge på]].",
'recreate-deleted-warn'  => "'''Advarsel: Du er ve at genskabe en tidligere slettet side.'''
 
Åvervej det'm er passende at genåprette'n side. De slettede hersenenger før 
denne side er vest nedenfør:",

# History pages
'viewpagelogs'        => 'Ves loglister før denne side',
'currentrev'          => 'Nuværende hersenenge',
'revisionasof'        => 'Hersenenger frå $1',
'revision-info'       => 'Hersenenge frå $1 til $2',
'previousrevision'    => '←Ældre hersenenge',
'nextrevision'        => 'Nyere hersenenge→',
'currentrevisionlink' => 'se nuværende hersenenge',
'cur'                 => 'nuværende',
'last'                => 'forrige',
'page_first'          => 'Startem',
'page_last'           => 'Enden',
'histlegend'          => 'Førklårenge: (nuværende) = førskel til den nuværende
hersenenge, (førge) = førskel til den førge hersenenge, l = lile til mendre ændrenge',
'histfirst'           => 'Ældste',
'histlast'            => 'Nyeste',

# Revision feed
'history-feed-item-nocomment' => '$1 ve $2', # user at time

# Diffs
'history-title'           => 'Hersengsskigte før "$1"',
'difference'              => '(Førskelle mellem hersenenger)',
'lineno'                  => 'Lenje $1:',
'compareselectedversions' => 'Sammenlign valgte hersenenger',
'editundo'                => 'baĝgøt',
'diff-multi'              => '(Æ hersenengssammenlegnenge vetåger {{PLURAL:$1|en mellemleggende hersenenge|$1 mellemleggende hersenenger}}.)',

# Search results
'noexactmatch' => "'''{{SITENAME}} har engen ertikel ve dette nav.''' Du ken [[:$1|åprette en ertikel ve dette nav]].",
'prevn'        => 'førge $1',
'nextn'        => 'nægste $1',
'viewprevnext' => 'Ves ($1) ($2) ($3)',
'searchall'    => 'ål',
'powersearch'  => 'Søĝ',

# Preferences page
'preferences'   => 'Endstellenger',
'mypreferences' => 'Endstellenger',
'retypenew'     => 'Gentag ny adgangskode',

'grouppage-sysop' => '{{ns:project}}:Administråtorer',

# User rights log
'rightslog' => 'Rettigheds-logbåĝ',

# Recent changes
'nchanges'                       => '$1 {{PLURAL:$1|ændrenge|ændrenger}}',
'recentchanges'                  => 'Seneste ændrenger',
'recentchanges-feed-description' => 'Ve dette fiid ken du følge de seneste ændrenger på {{SITENAME}}.',
'rcnote'                         => "Herunder ses {{PLURAL:$1|'''1''' ændrenge|de sedste '''$1''' ændrenger}} frå {{PLURAL:$2|i dåg|de sedste '''$2''' dåg}}, søm i $3.",
'rcnotefrom'                     => "Nedenfør ses ændrengerne frå '''$2''' til '''$1''' vest.",
'rclistfrom'                     => 'Ves nye ændrenger startende frå $1',
'rcshowhideminor'                => '$1 lile ændrenger',
'rcshowhidebots'                 => '$1 råbotter',
'rcshowhideliu'                  => '$1 regestrerede brugere',
'rcshowhideanons'                => '$1 anonyme brugere',
'rcshowhidepatr'                 => '$1 bekiiknurede ændrenger',
'rcshowhidemine'                 => '$1 egne bidråg',
'rclinks'                        => 'Ves seneste $1 ændrenger i de sedste $2 dåg<br />$3',
'diff'                           => 'førskel',
'hist'                           => 'skigte',
'hide'                           => 'skjul',
'show'                           => 'ves',
'minoreditletter'                => 'l',
'newpageletter'                  => 'N',
'boteditletter'                  => 'b',

# Recent changes linked
'recentchangeslinked'          => 'Relaterede ændrenger',
'recentchangeslinked-title'    => 'Ændrenger der vegånde til "$1"',
'recentchangeslinked-noresult' => 'I det udvalgte tidsrum blev der ekke føretaget ændrenger på siderne der henveses til.',
'recentchangeslinked-summary'  => "Denne sonstside beser de seneste ændrenger på de sider der henveses til. Sider på din åvervågnengsliste er vest ve '''fed''' skreft.",

# Upload
'upload'        => 'Læĝ æ billet åp',
'uploadbtn'     => 'Læĝ æ gøret åp',
'uploadlogpage' => 'Åplægnengslog',
'uploadedimage' => 'Låĝde "[[$1]]" åp',

# Special:ImageList
'imagelist' => 'Billetliste',

# Image description page
'filehist'                  => 'Billetskigt',
'filehist-help'             => "Klik på'n dato/tid før at se den hersenenge åf gøret.",
'filehist-current'          => 'nuværende',
'filehist-datetime'         => 'Dato/tid',
'filehist-user'             => 'Bruger',
'filehist-dimensions'       => 'Treflåksjener',
'filehist-filesize'         => 'Gøretstørrelse',
'filehist-comment'          => 'Biskrevselenge',
'imagelinks'                => 'Billethenvesnenger',
'linkstoimage'              => 'De følgende sider henveser til dette billet:',
'nolinkstoimage'            => 'Der er engen sider der henveser til dette billet.',
'sharedupload'              => 'Denne gøret er en fælles læĝenge og ken bruges åf andre projekter.',
'noimage'                   => 'Der er engen gøret ve dette nav, du ken $1',
'noimage-linktext'          => 'læĝge den åp',
'uploadnewversion-linktext' => 'Læĝ en ny hersenenge åf denne gøret åp',

# MIME search
'mimesearch' => 'Søĝe æfter MIME-sårt',

# List redirects
'listredirects' => 'Henvesnengsliste',

# Unused templates
'unusedtemplates' => 'Ekke brugte skablåner',

# Random page
'randompage' => 'Tilfældig ertikel',

# Random redirect
'randomredirect' => 'Tilfældige henvesnenger',

# Statistics
'statistics' => 'Sensje',

'disambiguations' => 'Ertikler ve flertydige skrevselenger',

'doubleredirects' => 'Dåbbelte åmstyrenger',

'brokenredirects' => 'Bråken åmstyrenger',

'withoutinterwiki' => 'Sider uden henvesnenger til andre språĝ',

'fewestrevisions' => 'Sider ve de færreste hersenenger',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|åg|åger}}',
'nlinks'                  => '{{PLURAL:$1|1 henvesnenge|$1 henvesnenger}}',
'nmembers'                => '- {{PLURAL:$1|1 ertikel|$1 ertikler}}',
'lonelypages'             => 'Førældreløse ertikler',
'uncategorizedpages'      => 'Uklyngede sider',
'uncategorizedcategories' => 'Uklyngede klynger',
'uncategorizedimages'     => 'Ekke klyngede gøret',
'uncategorizedtemplates'  => 'Ekke klyngede skablåner',
'unusedcategories'        => 'Ubrugte klynger',
'unusedimages'            => 'Ubrugte billeter',
'wantedcategories'        => 'Brugte men ekke ånlagte klynger',
'wantedpages'             => 'Ønskede ertikler',
'mostlinked'              => 'Sider ve flest henvesnenger',
'mostlinkedcategories'    => 'Mest brugte klynger',
'mostlinkedtemplates'     => 'Hyppigst brugte skablåner',
'mostcategories'          => 'Mest brugte sider',
'mostimages'              => 'Mest brugte gøret',
'mostrevisions'           => 'Sider ve de fleste ændrenger',
'prefixindex'             => 'Åle sider (ve førgøret)',
'shortpages'              => 'Kårte ertikler',
'longpages'               => 'Långe ertikler',
'deadendpages'            => 'Blendgydesider',
'protectedpages'          => 'Skrevebeskyttede sider',
'listusers'               => 'Brugerliste',
'newpages'                => 'Nyeste ertikler',
'ancientpages'            => 'Ældste ertikler',
'move'                    => 'Flyt',
'movethispage'            => 'Flyt side',

# Book sources
'booksources' => 'Boĝkelder',

# Special:Log
'specialloguserlabel'  => 'Bruger:',
'speciallogtitlelabel' => 'Skrevselenge:',
'log'                  => 'Loglister',
'all-logs-page'        => 'Åle loglister',

# Special:AllPages
'allpages'       => 'Åle ertikler',
'alphaindexline' => '$1 til $2',
'nextpage'       => 'Næste side ($1)',
'prevpage'       => 'Førge side ($1)',
'allpagesfrom'   => 'Ves sider startende frå:',
'allarticles'    => 'Åle ertikler',
'allpagessubmit' => 'Ves',
'allpagesprefix' => 'Ves sider ve førgøret:',

# Special:Categories
'categories' => 'Klynger',

# E-mail user
'emailuser' => 'E-mail til denne bruger',

# Watchlist
'watchlist'            => 'Åvervågnengsliste',
'mywatchlist'          => 'Åvervågnengsliste',
'watchlistfor'         => "(før '''$1''')",
'addedwatch'           => 'Tilføjet til din åvervågnengsliste',
'addedwatchtext'       => "Æ side \"[[:\$1]]\" er blevet tilføjet til din [[Special:Watchlist|åvervågningsliste]]. Fremtidige ændrenger til denne side og den tilhørende diskusjeside hvil bleve listet der, og æ side hvil fremstå '''fremhævet''' i'n [[Special:RecentChanges|liste ve de seneste ændrenger]] før at gøre det lettere at finde den. Hves du senere hvilst fjerne'n side frå din åvervågningsliste, så klik \"Fjern åvervågnenge\".",
'removedwatch'         => 'Fjernet frå åvervågnengsliste',
'removedwatchtext'     => 'Æ side "<nowiki>$1</nowiki>" er blevet fjernet frå din åvervågnengsliste.',
'watch'                => 'Åvervåg',
'watchthispage'        => 'Åvervåg side',
'unwatch'              => 'Fjern åvervågnenge',
'watchlist-details'    => 'Du har $1 {{PLURAL:$1|side|sider}} på din åvervågnengsliste (øn diskusjesider).',
'wlshowlast'           => 'Ves de seneste $1 têmer $2 dåg $3',
'watchlist-hide-bots'  => 'Skjule bot-ændrenger',
'watchlist-hide-own'   => 'skjule egne ændrenger',
'watchlist-hide-minor' => 'skjule små ændrenger',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Åvervåge …',
'unwatching' => 'Ekke åvervåge …',

# Delete/protect/revert
'deletepage'                  => 'Slet side',
'historywarning'              => 'Advarsel: Æ side du erst ve at slette har en skigte:',
'confirmdeletetext'           => "Du erst ve permanent at slette en side
æller et billet sammen ve hæle den tilhørende skigte frå'n database. Bekræft venlegst at du virkelg hvilst gøre dette, at du førstårst konsekvenserne, og at du gør dette i åverensstemmelse ve
[[{{MediaWiki:Policy-url}}]].",
'actioncomplete'              => 'Gennemført',
'deletedtext'                 => '"<nowiki>$1</nowiki>" er slettet. Sæg $2 før en førtegnelse åver de nyeste sletnenger.',
'deletedarticle'              => 'slettede "[[$1]]"',
'dellogpage'                  => 'Sletnengslog',
'deletecomment'               => 'Begrundelse før sletnenge:',
'deleteotherreason'           => 'Anden/uddybende begrundelse:',
'deletereasonotherlist'       => 'Anden begrundelse',
'rollbacklink'                => 'fjern redigærenge',
'protectlogpage'              => 'Liste åver beskyttede sider',
'protectcomment'              => 'Begrundelse før beskyttelse',
'protectexpiry'               => 'Udløb:',
'protect_expiry_invalid'      => 'Æ udløbstiid er ugyldeg.',
'protect_expiry_old'          => "Æ udløbstiid legger i'n førtiid.",
'protect-unchain'             => 'Ændre flytnengsbeskyttelse',
'protect-text'                => "Her ken beskyttelsesståt før æ side '''<nowiki>$1</nowiki>''' ses og ændres.",
'protect-locked-access'       => 'Den brugerkonto har ekke de nødvendege rettegheder til at æ ændre sidebeskyttelse. Her er de aktuelle beskyttelsesendstellenger før æ side <strong>„$1“:</strong>',
'protect-cascadeon'           => 'Denne side er del åf en nedarvet skrevebeskyttelse. Wen er endeholt i nedenstående {{PLURAL:$1|side|sider}}, søm er skrevebeskyttet ve tilvalg åf "nedarvende sidebeskyttelse" Æ sidebeskyttelse ken ændres før denne side, det påverker dog ekke\'n kaskadespærrenge:',
'protect-default'             => 'Ål (standård)',
'protect-fallback'            => 'Kræv "$1"-tilladelse',
'protect-level-autoconfirmed' => 'Spærrenge før ekke registrærede brugere',
'protect-level-sysop'         => 'Kan administratårer',
'protect-summary-cascade'     => 'nedarvende',
'protect-expiring'            => 'til $1 (UTC)',
'protect-cascade'             => 'Nedarvende spærrenge – ål skabelåner, søm er endbundet i denne side spærres også.',
'protect-cantedit'            => 'Du kenst ekke ændre beskyttelsesnivå før denne side, da du ekke kenst redigære føden.',
'restriction-type'            => 'Beskyttelsesståt',
'restriction-level'           => 'Beskyttelseshøjde',

# Undelete
'undeletebtn' => 'Gendan!',

# Namespace form on various pages
'namespace'      => 'Naverum:',
'invert'         => 'Næbiiner udvalg',
'blanknamespace' => '(Ertikler)',

# Contributions
'contributions' => 'Brugerbidråg',
'mycontris'     => 'Mine bidråg',
'contribsub2'   => 'Før $1 ($2)',
'uctop'         => '(seneste)',
'month'         => 'Måned:',
'year'          => 'År:',

'sp-contributions-newbies-sub' => 'Før nybegyndere',
'sp-contributions-blocklog'    => 'Blokerengslog',

# What links here
'whatlinkshere'       => 'Vat henveser hertil',
'whatlinkshere-title' => 'Sider der henveser til $1',
'linklistsub'         => '(Henvesnengsliste)',
'linkshere'           => "De følgende sider henveser til '''„[[:$1]]“''':",
'nolinkshere'         => "Engen sider henveser til '''„[[:$1]]“'''.",
'isredirect'          => 'åmstyrsside',
'istemplate'          => 'Skablånvetagnenge',
'whatlinkshere-prev'  => '{{PLURAL:$1|førge|førge $1}}',
'whatlinkshere-next'  => '{{PLURAL:$1|nægste|nægste $1}}',
'whatlinkshere-links' => '← henvesnenger',

# Block/unblock
'blockip'       => 'Bloker bruger',
'ipboptions'    => '1 tême:1 hour,2 têmer:2 hours,6 têmer:6 hours,1 dåĝ:1 day,3 dåĝ:3 days,1 uge:1 week,2 uger:2 weeks,1 måned:1 month,3 måneder:3 months,1 år:1 year,ubegrænset:indefinite', # display1:time1,display2:time2,...
'ipblocklist'   => 'Blokerede IP-adresser og brugernave',
'blocklink'     => 'blåker',
'unblocklink'   => 'åphæv blokerenge',
'contribslink'  => 'bidråĝ',
'blocklogpage'  => 'Blokerengslog',
'blocklogentry' => 'blokerede "[[$1]]" ve\'n udløbstid på $2 $3',

# Move page
'movepagetext'     => "Når du brugerst æ formulær herunder hvilst du få omdøbt en side og flyttet æ hæle side han skigte til det nye navn.
Den gamle titel hvil bleve en omdirigærengsside til den nye titel.
Henvesnenger til den gamle titel hvil ekke bleve ændret.
Sørg før at tjekke før dåbelte æller dårlege omdirigærenger.
Du erst ansvarleg før, at ål henvesnenger stadeg pæger derhen, hvår det er æ mænenge de skal pæge.

Bemærk at æ side '''ekke''' ken flyttes hves der ålrede er en side ve den nye titel, medmendre den side er tøm æller er en omdirigærenge uden nogen skigte.
Det betyder at du kenst flytte en side tilbåge hvår den kåm frå, hves du kåmer til at lave en fejl.

'''ADVARSEL!'''
Dette ken være en drastisk og uventet ændrenge før en populær side; vær sekker på, at du førstår konsekvenserne åf dette før du førtsætter.",
'movepagetalktext' => "Den tilhørende diskusjeside, hves der er en, hvil åtåmatisk bleve flyttet ve'n side '''medmendre:''' 
*Du flytter æ side til et andet navnerum,
*En ekke-tøm diskusjeside ålrede eksisterer under det nye navn, æller
*Du fjerner æ markærenge i'n boks nedenunder.

I disse tilfælde er du nødt til at flytte æller sammenflette'n side manuelt.",
'movearticle'      => 'Flyt side:',
'newtitle'         => 'Til ny titel:',
'move-watch'       => 'Denne side åvervåges',
'movepagebtn'      => 'Flyt side',
'pagemovedsub'     => 'Flytnenge gennemført',
'movepage-moved'   => '<big>Æ side \'\'\'"$1" er flyttet til "$2"\'\'\'</big>', # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'    => 'En side ve det navn eksisterer ålrede, æller det
navn du harst valgt er ekke gyldegt. Vælg et andet navn.',
'talkexists'       => 'Æ side blev flyttet korrekt, men den tilhørende diskusjeside ken ekke flyttes, førdi der ålrede eksisterer en ve den nye titel. Du erst nødt til at flette dem sammen manuelt.',
'movedto'          => 'flyttet til',
'movetalk'         => 'Flyt også\'n "diskusjeside", hves den eksisterer.',
'1movedto2'        => '[[$1]] flyttet til [[$2]]',
'movelogpage'      => 'Flyttelog',
'movereason'       => 'Begrundelse:',
'revertmove'       => 'gendan',

# Export
'export' => 'Utgøter sider',

# Namespace 8 related
'allmessages' => 'Åle beskeder',

# Thumbnails
'thumbnail-more'  => 'Førstør',
'thumbnail_error' => 'Fejl ve åprettelse åf thumbnail: $1',

# Import log
'importlogpage' => 'Importlog',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'Min brugersside',
'tooltip-pt-mytalk'               => 'Min diskusjesside',
'tooltip-pt-preferences'          => 'Min endstellenger',
'tooltip-pt-watchlist'            => 'Æ liste åver sider du åvervåger før ændrenger.',
'tooltip-pt-mycontris'            => 'Æ liste åver dine bidråg',
'tooltip-pt-login'                => 'Du åpførdres til at loĝge på, men dat er ekke plechnenge.',
'tooltip-pt-logout'               => 'Loĝ åf',
'tooltip-ca-talk'                 => "Diskusje æ indholt'm på'n side",
'tooltip-ca-edit'                 => 'Du ken redigære denne side. Brug venligst førhåndsvesnenge før du gemmer.',
'tooltip-ca-addsection'           => 'Tilføj en biskrevselenge til denne diskusje.',
'tooltip-ca-viewsource'           => "Denne side er beskyttet. Du ken kegge på'n keldekode.",
'tooltip-ca-protect'              => 'Beskyt denne side',
'tooltip-ca-delete'               => 'Slet denne side',
'tooltip-ca-move'                 => 'Flyt denne side',
'tooltip-ca-watch'                => 'Sæt denne side på din åvervågnengsliste',
'tooltip-ca-unwatch'              => 'Fjern denne side frå din åvervågnengsliste',
'tooltip-search'                  => 'Søĝ på {{SITENAME}}',
'tooltip-n-mainpage'              => 'Besøĝ æ Førsit',
'tooltip-n-portal'                => "Æ projekt'm, vat du ken gøre, vår tenger fendes",
'tooltip-n-currentevents'         => "Find baĝgrundsinformasje nessende begivenheder'm",
'tooltip-n-recentchanges'         => "Æ liste åver de seneste ændrenger æ'n wiki.",
'tooltip-n-randompage'            => 'Gå til æ tilfældig ertikel',
'tooltip-n-help'                  => 'Vordan gør a ...',
'tooltip-t-whatlinkshere'         => 'Liste ve ål sider søm henveser hertil',
'tooltip-t-contributions'         => 'Se denne brugers bidråg',
'tooltip-t-emailuser'             => 'Send en e-mail til denne bruger',
'tooltip-t-upload'                => 'Læĝ æ billet, æ sunnåm æller anden mediagøret åp',
'tooltip-t-specialpages'          => 'Liste ve ål sonst sider',
'tooltip-ca-nstab-user'           => "Se'n brugerside",
'tooltip-ca-nstab-project'        => "Vese'n wiki'mside",
'tooltip-ca-nstab-image'          => "Se'n billetside",
'tooltip-ca-nstab-template'       => "Se'n skablån",
'tooltip-ca-nstab-help'           => "Se'n hjælpeside",
'tooltip-ca-nstab-category'       => "Se'n klyngeside",
'tooltip-minoredit'               => 'Marker dette søm en mendre ændrenge',
'tooltip-save'                    => 'Gem dine ændrenger',
'tooltip-preview'                 => 'Førhånds dine ændrenger, brug venligst denne funksje enden du gemmer!',
'tooltip-diff'                    => 'Ves velke ændrenger du har lavet i æ skrevselenger.',
'tooltip-compareselectedversions' => 'Se førskellene imellem de to valgte hersenenger åf denne side.',
'tooltip-watch'                   => 'Tilføj denne side til din åvervågnengsliste',

# Browsing diffs
'previousdiff' => '← Gå til førge førskel',
'nextdiff'     => 'Gå til næste førskel →',

# Media information
'file-info-size'       => '($1 × $2 pixel, gøretstørrelse: $3, MIME-senenge: $4)',
'file-nohires'         => '<small>Engen højere åpløsnenge fundet.</small>',
'svg-long-desc'        => '(SVG gøret, wønetstørrelse $1 × $2 pixel, gøretstørrelse: $3)',
'show-big-image'       => 'Hersenenge i større åpløsnenge',
'show-big-image-thumb' => '<small>Størrelse åf førhåndsvesnenge: $1 × $2 pixel</small>',

# Special:NewImages
'newimages' => 'Liste ve de nyeste billeter',

# Bad image list
'bad_image_list' => "Æ førmåt er:

Kun endholtet åf æ liste (lenjer startende ve *) bliver brugt. Den første henvesnenge på'n lenje er til det uønskede billet. Æfterfølgende lenker på samme lenjer er undtagelser, dvs. sider vår æ billet må åptræde.",

# Metadata
'metadata'          => 'Metadata',
'metadata-help'     => "Denne gøret endeholder yderligere informasje, der søm regel stammer frå lysnåmer æller den brugte skænner. Ve'n æfterføgende bearbejdnenge ken nogle data være ændret.",
'metadata-expand'   => 'Ves udvedede data',
'metadata-collapse' => 'Skjul udvedede data',
'metadata-fields'   => 'Æ følgende felter frå EXIF-metadata i denne MediaWiki-beskedskrevselenger veses på billetbeskrevelsessider; yderligere lileskrevselenger ken veses. 
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# External editor support
'edit-externally'      => "Redigær denne gøret ve'n utsende redigærstøme",
'edit-externally-help' => 'Se [http://www.mediawiki.org/wiki/Manual:External_editors setup hjælpje] før mære informasje.',

# 'all' in various places, this might be different for inflected languages
'watchlistall2' => 'åle',
'namespacesall' => 'åle',
'monthsall'     => 'åle',

# Watchlist editing tools
'watchlisttools-view' => "Se ændrede sider i'n åvervågnengsliste",
'watchlisttools-edit' => 'Redigær åvervågnengsliste',
'watchlisttools-raw'  => 'Redigær rå åvervågnengsliste',

# Special:Version
'version' => "Informasje MediaWiki'm", # Not used as normal message but as header for the special page itself

# Special:SpecialPages
'specialpages' => 'Sonst sider',

);
