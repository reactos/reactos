<?php
/** Walloon (Walon)
 *
 * @ingroup Language
 * @file
 *
 * @author Srtxg
 * @author לערי ריינהארט
 */

$fallback = 'fr';

# lists "no preferences", normall (long) walloon date,
# short walloon date, and ISO format
# MW_DATE_DMY is alias for long format, as it is dd mmmmm yyyy.
$datePreferences = array(
	'default',
	'dmy',
	'walloon short',
	'ISO 8601'
);

$datePreferenceMigrationMap = array(
	0 => 'default',
	2 => 'dmy',
	4 => 'walloon short',
);
$defaultDateFormat = 'dmy';

$dateFormats = array(
	'walloon short time' => 'H:i'
);

$namespaceNames = array(
	NS_MEDIA          => "Media", /* Media */
	NS_SPECIAL        => "Sipeciås", /* Special */
	NS_MAIN           => "",
	NS_TALK           => "Copene", /* Talk */
	NS_USER	          => "Uzeu", /* User */
	NS_USER_TALK      => "Uzeu_copene", /* User_talk */
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK   => '$1_copene',
	NS_IMAGE          => "Imådje", /* Image */
	NS_IMAGE_TALK     => "Imådje_copene", /* Image_talk */
	NS_MEDIAWIKI      => "MediaWiki", /* MediaWiki */
	NS_MEDIAWIKI_TALK => "MediaWiki_copene", /* MediaWiki_talk */
	NS_TEMPLATE       => "Modele",
	NS_TEMPLATE_TALK  => "Modele_copene",
	NS_HELP           => "Aidance",
	NS_HELP_TALK      => "Aidance_copene",
	NS_CATEGORY       => "Categoreye",
	NS_CATEGORY_TALK  => "Categoreye_copene",
);

# definixha del cogne po les limeros
# (number format definition)
# en: 12,345.67 -> wa: 12 345,67
$separatorTransformTable = array(',' => "\xc2\xa0", '.' => ',' );

#$linkTrail = '/^([a-zåâêîôûçéèA-ZÅÂÊÎÔÛÇÉÈ]+)(.*)$/sDu';
$linkTrail = '/^([a-zåâêîôûçéè]+)(.*)$/sDu';

#
# NOTE:
# sysop = manaedjeu
# bureaucrat = mwaisse-manaedjeu
#

$messages = array(
# User preference toggles
'tog-underline'               => 'Sorlignî les loyéns',
'tog-highlightbroken'         => 'Håyner les vudes loyéns <a href="" class="new">come çouchal</a><br /> &nbsp;&nbsp;&nbsp; (oudonbén: come çouchal<a href="" class="internal">?</a>).',
'tog-justify'                 => 'Djustifyî les hagnons',
'tog-hideminor'               => 'Èn nén mostrer les <i>dierins candjmints</i> mineurs',
'tog-extendwatchlist'         => "Ragrandi l' djivêye po mostrer tos les candjmints",
'tog-usenewrc'                => 'Ramidrés <i>dierins candjmints</i> (JavaScript)',
'tog-numberheadings'          => 'Limerotaedje otomatike des tites',
'tog-showtoolbar'             => "Mostrer l' bår d' usteyes e môde candjmint (JavaScript)",
'tog-editondblclick'          => 'Candjî les pådjes avou on dobe-clitch (JavaScript)',
'tog-editsection'             => "Eployî les loyéns «[candjî]» po candjî rén k' ene seccion",
'tog-editsectiononrightclick' => 'Candjî les seccions avou on dobe-clitch sol tite (JavaScript)',
'tog-showtoc'                 => "Mostrer l' tåvlea d' ådvins<br />(po ls årtikes avou pus di 3 seccions)",
'tog-rememberpassword'        => 'Rimimbrer li scret inte les sessions',
'tog-editwidth'               => "Li boesse d' aspougnaedje prind tote li lårdjeu",
'tog-watchcreations'          => "Mete les pådjes ki dj' askepeye dins l' djivêye des pådjes shuvowes",
'tog-watchdefault'            => "Shuve les årtikes ki dj' fwai ou ki dj' candje",
'tog-watchmoves'              => "Radjouter a m' djivêye des shuvous les pådjes ki dji displaece",
'tog-watchdeletion'           => "Radjouter a m' djivêye des shuvous les pådjes ki dji disface",
'tog-minordefault'            => 'Prémete mes candjmints come mineurs',
'tog-previewontop'            => "Prévey l' årtike å dzeu del boesse d' aspougnaedje",
'tog-previewonfirst'          => "Prévey l' årtike å prumî candjmint",
'tog-nocache'                 => "Èn nén eployî d' muchete pol håynaedje des pådjes",
'tog-enotifwatchlistpages'    => "M' emiler cwand ene pådje shuvowe candje",
'tog-enotifusertalkpages'     => "M' emiler cwand l' pådje di copene da minne candje",
'tog-enotifminoredits'        => "M' emiler eto po les ptits candjmints",
'tog-enotifrevealaddr'        => 'Mostrer mi adresse emile dins les emiles di notifiaedje',
'tog-shownumberswatching'     => "Mostrer l' nombe d' uzeus ki shuvèt l' pådje",
'tog-fancysig'                => 'Sinateure brute (sins loyén otomatike)',
'tog-externaleditor'          => "Eployî on dfoûtrin aspougneu d' tecse come prémetowe dujhance",
'tog-externaldiff'            => 'Eployî on dfoûtrin programe di diferinces come prémetowe dujhance',
'tog-showjumplinks'           => 'Mete en alaedje les loyéns di naiviaedje «potchî a» å dzeu del pådje (pol pea «Myskin» et ds ôtes)',
'tog-uselivepreview'          => "Eployî l' prévoeyaedje abeye (JavaScript) (Esperimintå)",
'tog-forceeditsummary'        => "M' advierti cwand dji lai vude on rascourti",
'tog-watchlisthideown'        => 'Èn nén mostrer les candjmints da minne',
'tog-watchlisthidebots'       => 'Èn nén mostrer les candjmints des robots',
'tog-watchlisthideminor'      => "Catchî les candjmints mineurs dins l' djivêye des shuvous",
'tog-ccmeonemails'            => "m' evoyî ene copeye des emiles ki dj' evoye ås ôtes",
'tog-diffonly'                => "Èn nén håyner l' contnou del pådje pa dzo l' pådje des diferinces",

'underline-always'  => 'Tofer',
'underline-never'   => 'Måy',
'underline-default' => 'Valixhance do betchteu',

'skinpreview' => '(vey divant)',

# Dates
'sunday'        => 'dimegne',
'monday'        => 'londi',
'tuesday'       => 'mårdi',
'wednesday'     => 'mierkidi',
'thursday'      => 'djudi',
'friday'        => 'vénrdi',
'saturday'      => 'semdi',
'sun'           => 'dim',
'mon'           => 'lon',
'tue'           => 'mår',
'wed'           => 'mie',
'thu'           => 'dju',
'fri'           => 'vén',
'sat'           => 'sem',
'january'       => 'djanvî',
'february'      => 'fevrî',
'march'         => 'måss',
'april'         => 'avri',
'may_long'      => 'may',
'june'          => 'djun',
'july'          => 'djulete',
'august'        => 'awousse',
'september'     => 'setimbe',
'october'       => 'octôbe',
'november'      => 'nôvimbe',
'december'      => 'decimbe',
'january-gen'   => 'djanvî',
'february-gen'  => 'fevrî',
'march-gen'     => 'måss',
'april-gen'     => 'avri',
'may-gen'       => 'may',
'june-gen'      => 'djun',
'july-gen'      => 'djulete',
'august-gen'    => 'awousse',
'september-gen' => 'setimbe',
'october-gen'   => 'octôbe',
'november-gen'  => 'nôvimbe',
'december-gen'  => 'decimbe',
'jan'           => 'dja',
'feb'           => 'fev',
'mar'           => 'mås',
'apr'           => 'avr',
'may'           => 'may',
'jun'           => 'djn',
'jul'           => 'djl',
'aug'           => 'awo',
'sep'           => 'set',
'oct'           => 'oct',
'nov'           => 'nôv',
'dec'           => 'dec',

# Categories related messages
'pagecategories'        => '{{PLURAL:$1|Categoreye|Categoreyes}}',
'category_header'       => 'Årtikes el categoreye «$1»',
'subcategories'         => 'Dizo-categoreyes',
'category-media-header' => 'Media el categoreye «$1»',
'category-empty'        => "''Cisse categoreye ci n' a pol moumint nol årtike ni media.''",

'mainpagetext' => "<big>'''Li programe Wiki a stî astalé a l' idêye.'''</big>",

'about'          => 'Åd fwait',
'article'        => 'Årtike',
'newwindow'      => '(drovant en on novea purnea)',
'cancel'         => 'Rinoncî',
'qbfind'         => 'Trover',
'qbbrowse'       => 'Foyter',
'qbedit'         => 'Candjî',
'qbpageoptions'  => 'Cisse pådje ci',
'qbpageinfo'     => 'Contecse',
'qbmyoptions'    => 'Mes pådjes',
'qbspecialpages' => 'Pådjes sipeciåles',
'moredotdotdot'  => 'Co dpus...',
'mypage'         => 'Mi pådje',
'mytalk'         => 'Mi copinaedje',
'anontalk'       => 'Pådje di copene po ciste adresse IP',
'navigation'     => 'Naiviaedje',
'and'            => 'eyet',

# Metadata in edit box
'metadata_help' => 'Meta-dnêyes :',

'errorpagetitle'    => 'Aroke',
'returnto'          => 'Rivni al pådje «$1».',
'tagline'           => 'Èn årtike di {{SITENAME}}.',
'help'              => 'Aidance',
'search'            => 'Cweri',
'searchbutton'      => 'Cweri',
'go'                => 'Potchî',
'searcharticle'     => 'Potchî',
'history'           => 'Istwere del pådje',
'history_short'     => 'Istwere',
'updatedmarker'     => 'candjî dispoy mi dierinne vizite',
'info_short'        => 'Infôrmåcions',
'printableversion'  => 'Modêye sicrirece-amiståve',
'permalink'         => 'Hårdêye viè cisse modêye ci',
'print'             => 'Imprimer',
'edit'              => 'Candjî',
'editthispage'      => "Candjî l' pådje",
'delete'            => 'Disfacer',
'deletethispage'    => "Disfacer l' pådje",
'undelete_short'    => 'Rapexhî {{PLURAL:$1|on candjmint|$1 candjmints}}',
'protect'           => 'Protedjî',
'protect_change'    => "candjî l' protedjaedje",
'protectthispage'   => "Protedjî l' pådje",
'unprotect'         => 'Disprotedjî',
'unprotectthispage' => "Disprotedjî l' pådje",
'newpage'           => 'Novele pådje',
'talkpage'          => 'Copene sol pådje',
'talkpagelinktext'  => 'Copiner',
'specialpage'       => 'Pådje sipeciåle',
'personaltools'     => 'Usteyes da vosse',
'postcomment'       => 'Sicrire on comintaire',
'articlepage'       => "Vey l' årtike",
'talk'              => 'Copene',
'toolbox'           => 'Boesse ås usteyes',
'userpage'          => "Vey li pådje di l' uzeu",
'projectpage'       => 'Vey li pådje do pordjet',
'imagepage'         => "Vey li pådje di l' imådje",
'viewtalkpage'      => 'Vey li pådje di copene',
'otherlanguages'    => 'Ôtes lingaedjes',
'redirectedfrom'    => '(Redjiblé di $1)',
'redirectpagesub'   => 'Pådje di redjiblaedje',
'lastmodifiedat'    => 'Cisse pådje a stî candjeye pol dierin côp li $2, $1.', # $1 date, $2 time
'viewcount'         => 'Cisse pådje la a stî léjhowe {{PLURAL:$1|on côp|$1 côps}}.',
'protectedpage'     => 'Pådje protedjeye',
'jumpto'            => 'Potchî a:',
'jumptonavigation'  => 'naiviaedje',
'jumptosearch'      => 'cweri',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'Åd fwait di {{SITENAME}}',
'aboutpage'            => 'Project:Åd fwait',
'bugreports'           => 'Rapoirts di bugs',
'bugreportspage'       => 'Project:Rapoirts di bugs',
'copyright'            => "Li contnou est dizo l' $1.",
'copyrightpagename'    => 'Abondroets {{SITENAME}}',
'copyrightpage'        => '{{ns:project}}:Abondroets',
'currentevents'        => 'Actouwålités',
'currentevents-url'    => 'Project:Actouwålités',
'edithelp'             => 'Aidance',
'edithelppage'         => 'Help:Kimint candjî ene pådje',
'helppage'             => 'Help:Aidance',
'mainpage'             => 'Mwaisse pådje',
'mainpage-description' => 'Mwaisse pådje',
'portal'               => 'Inte di nozôtes',
'portal-url'           => 'Project:Inte di nozôtes',

'badaccess' => "Åk n' a nén stî avou les permissions",

'versionrequired'     => "I vs fåt l' modêye $1 di MediaWiki",
'versionrequiredtext' => "I vs fåt l' modêye $1 di MediaWiki po-z eployî cisse pådje ci. Loukîz a [[Special:Version]]",

'ok'                      => "'l est bon",
'retrievedfrom'           => 'Prin del pådje «$1»',
'youhavenewmessages'      => 'Vos avoz des $1 ($2).',
'newmessageslink'         => 'noveas messaedjes',
'newmessagesdifflink'     => 'dierin candjmint',
'youhavenewmessagesmulti' => 'Vos avoz des noveas messaedjes so $1',
'editsection'             => 'candjî',
'editold'                 => 'candjî',
'editsectionhint'         => "Candjî l' seccion: $1",
'toc'                     => 'Ådvins',
'showtoc'                 => 'mostrer',
'hidetoc'                 => 'catchî',
'thisisdeleted'           => 'Vey ou rapexhî $1?',
'viewdeleted'             => 'Vey $1?',
'restorelink'             => '{{PLURAL:$1|on candjmint disfacé|$1 candjmints disfacés}}',
'feedlinks'               => 'Sindicåcion:',
'feed-invalid'            => 'Sôre di sindicåcion nén valide.',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Årtike',
'nstab-user'      => "Pådje di l' uzeu",
'nstab-media'     => 'Media',
'nstab-special'   => 'Sipeciås',
'nstab-project'   => 'Pådje',
'nstab-image'     => 'Imådje',
'nstab-mediawiki' => 'Messaedje',
'nstab-template'  => 'Modele',
'nstab-help'      => 'Aidance',
'nstab-category'  => 'Categoreye',

# Main script and global functions
'nosuchaction'      => 'Nole sifwaite accion',
'nosuchactiontext'  => "L' accion specifieye pal hårdêye n' est nén ricnoxhowe på wiki.",
'nosuchspecialpage' => 'Nole sifwaite pådje',
'nospecialpagetext' => 'Vos avoz dmandé ene pådje sipeciåle nén valide, po ene djivêye des pådjes sipeciåles valides, loukîz a [[Special:SpecialPages]].',

# General errors
'error'                => 'Aroke',
'databaseerror'        => "Åk n' a nén stî avou l' båze di dnêyes",
'dberrortext'          => "Åk n' a nén stî avou l' sintacse do cweraedje del båze di dnêyes.
Çoula pout esse cåze d' on bug dins l' programe.
Li dierin cweraedje del båze di dnêyes di sayî esteut:
<blockquote><tt>$1</tt></blockquote>
a pårti del fonccion «<tt>$2</tt>».
MySQL a rtourné l' aroke «<tt>$3: $4</tt>».",
'dberrortextcl'        => "Åk n' a nén stî avou l' sintacse do cweraedje del båze di dnêyes.
Li dierin cweraedje del båze di dnêyes di sayî esteut:
«$1»
a pårti del fonccion «$2».
MySQL a rtourné l' aroke «$3: $4».",
'noconnect'            => "Mande escuzes! Li wiki a des rujhes tecnikes pol moumint, eyet c' est nén possibe di s' raloyî al båze di dnêyes. <br />
$1",
'nodb'                 => "Dji n' sai tchoezi l' båze di dnêyes $1",
'cachederror'          => "Çou ki shût c' est ene copeye e muchete del pådje k' a stî dmandêye, et ça s' pout ki ça n' soeye nén a djoû.",
'laggedslavemode'      => "Asteme: I s' pout ki l' pådje n' åye nén co les dierins candjmints.",
'internalerror'        => 'Divintrinne aroke',
'filecopyerror'        => "Dji n' a savou copyî l' fitchî «$1» viè «$2».",
'filerenameerror'      => "Dji n' a savou rlomer l' fitchî «$1» e «$2».",
'filedeleteerror'      => "Dji n' a savou disfacer l' fitchî «$1».",
'filenotfound'         => "Dji n' a savou trover l' fitchî «$1».",
'unexpected'           => 'Valixhance nén ratindowe: «$1»=«$2».',
'badarticleerror'      => "Cisse accion la n' si pout nén fé so cisse pådje ci.",
'cannotdelete'         => "Dji n' sai disfacer l' pådje ou l' imådje dimandêye (ça s' pôreut k' ene ôte sakî l' a ddja disfacé).",
'badtitle'             => 'Måva tite',
'badtitletext'         => "Li tite del pådje dimandêye n' esteut nén valide, il estet vude, oudonbén c' esteut on cron loyén eterlingaedje ou eterwiki. Ça s' pout k' il åye onk ou sacwants caracteres ki n' polèt nén esse eployîs dins les tites.",
'perfdisabled'         => "Mande escuzes! mins cisse fonccionålité ci a stî essoctêye pol moumint
pask' ele est trop pezante pol båze di dnêyes, ki dvént si télmint
londjinne k' on s' endè pout pus siervi a môde di djin.",
'perfcached'           => "Les dnêyes ki shuvèt c' est ene copeye e muchete, et ça s' pout ki ça n' soeye nén ttafwaitmint a djoû.",
'perfcachedts'         => "Les dnêyes ki shuvèt c' est ene copeye e muchete, ey elle ont stî metowes a djoû pol dierin côp li $1.",
'wrong_wfQuery_params' => 'Parametes incoreks po wfQuery()<br />
Fonccion: $1<br />
Cweraedje: $2',
'viewsource'           => 'Vey côde sourdant',
'viewsourcefor'        => 'po $1',
'protectedinterface'   => "Cisse pådje ci dene on tecse d' eterface pol programe, eyet elle a stî protedjeye po s' waeranti siconte des abus.",
'editinginterface'     => "'''Asteme:''' Vos candjîz ene pådje k' est eployeye po dner on tecse d' eterface pol programe. Les candjmints a cisse pådje ci vont-st aveur èn efet so l' eterface d' uzeu des ôtes uzeus.",

# Login and logout pages
'logouttitle'                => 'Dislodjaedje',
'logouttext'                 => "<strong>Vos vs avoz dislodjî.</strong><br />
Vos ploz continouwer a naivyî so {{SITENAME}} anonimmint, oudonbén
vos relodjî dizo l' minme uzeu ou dizo èn uzeu diferin. Notez ki des
pådjes k' i gn a si pôrént continowuer a vey come si vos estîz elodjî,
disk' a tant ki vos vudrîz l' muchete di vosse betchteu waibe.",
'welcomecreation'            => '== Bénvnowe, $1! ==

Vosse conte a stî ahivé.
Èn rovyîz nén di candjî les preferinces di {{SITENAME}} a vosse môde.',
'loginpagetitle'             => 'Elodjaedje',
'yourname'                   => "Vosse no d' elodjaedje",
'yourpassword'               => 'Vosse sicret',
'yourpasswordagain'          => 'Ritapez vosse sicret',
'remembermypassword'         => "Rimimbrer m' sicret inte les sessions.",
'yourdomainname'             => 'Vosse dominne',
'loginproblem'               => "<b>Åk n' a nén stî tot vs elodjant.</b><br />Rissayîz s' i vs plait!",
'login'                      => "S' elodjî",
'loginprompt'                => 'Vos dvoz permete les coûkes po vs elodjî so {{SITENAME}}.',
'userlogin'                  => "S' elodjî",
'logout'                     => 'Si dislodjî',
'userlogout'                 => 'Si dislodjî',
'notloggedin'                => 'Nén elodjî',
'nologin'                    => "Vos n' avoz nén d' conte so ç' wiki ci? $1.",
'nologinlink'                => 'Ahivez on conte da vosse',
'createaccount'              => 'Ahiver on novea conte',
'gotaccount'                 => "Vos avoz ddja on conte so ç' wiki ci? $1.",
'gotaccountlink'             => 'Elodjîz vs',
'createaccountmail'          => 'pa emile',
'badretype'                  => 'Vos avoz dné deus screts diferins.',
'userexists'                 => "Li no d' uzeu ki vs avoz tchoezi est ddja eployî. Tchoezixhoz è èn ôte s' i vs plait.",
'youremail'                  => 'Vost emile*',
'username'                   => "No d' elodjaedje:",
'uid'                        => "Limero d' l' uzeu:",
'yourrealname'               => 'Li vraiy no da vosse*',
'yourlanguage'               => "Lingaedje po l' eterface",
'yourvariant'                => 'Variante do lingaedje',
'yournick'                   => 'Vosse no metou (po les sinateures)',
'badsig'                     => 'Sinateure brute nén valide; verifyîz les etiketes HTML.',
'email'                      => 'Emile',
'prefs-help-realname'        => '* Li vraiy no da vosse (opcionel): si vos tchoezixhoz del diner i serè-st eployî po les contribouwaedjes da vosse.',
'loginerror'                 => "Aroke d' elodjaedje",
'prefs-help-email'           => "* Emile (opcionel): Permete di rçure des emiles ki ds ôtes uzeus vos polèt evoyî a pårti del pådje d' uzeu da vosse, sins ki voste adresse emile ni soeye håynêye.",
'nocookiesnew'               => "Li conte a stî ahivé, mins vos n' estoz nén elodjî. {{SITENAME}} eploye des coûkes po l' elodjaedje des uzeus. Vos avoz dismetou l' sopoirt des coûkes dins vosse betchteu waibe; rimetoz l' en alaedje et relodjîz vs avou vosse novea no d' elodjaedje eyet scret, s' i vs plait.",
'nocookieslogin'             => "{{SITENAME}} eploye des coûkes po l' elodjaedje des uzeus. Vos avoz dismetou l' sopoirt des coûkes dins vosse betchteu waibe; rimetoz l' en alaedje et relodjîz vs s' i vs plait.",
'noname'                     => "Vos n' avoz nén dné di no d' uzeu valide.",
'loginsuccesstitle'          => 'Vos estoz elodjî',
'loginsuccess'               => "'''L' elodjaedje a stî comifåt, asteure vos estoz elodjî dins {{SITENAME}} dizo l' no d' uzeu «$1».'''",
'nosuchuser'                 => "I g na nou uzeu dizo l' no «$1».
Verifyîz çou k' vos avoz tapé, oudonbén rimplixhoz les ôtes tchamps
et clitchîz sol boton po-z ahiver on novea conte.",
'nosuchusershort'            => "I g na nou uzeu dizo l' no «<nowiki>$1</nowiki>». Verifyîz çou k' vos avoz tapé.",
'nouserspecified'            => "Vos dvoz dner on no d' elodjaedje.",
'wrongpassword'              => "Li scret ki vs avoz dné est måva. Rissayîz s' i vs plait.",
'wrongpasswordempty'         => "Vos avoz dné on vude sicret. Rissayîz s' i vs plait.",
'passwordtooshort'           => 'Li scret est pår trop court. I doet esse di pol moens $1 caracteres.',
'mailmypassword'             => "M' emiler on novea scret",
'passwordremindertitle'      => 'Rimimbraedje do scret po {{SITENAME}}',
'passwordremindertext'       => "Ene sakî (probåblumint vos-minme, avou l' adresse IP $1) a dmandé k' on vs emile on novea scret po {{SITENAME}} ($4).
Li scret po l' uzeu «$2» est asteure «$3».
Po pus di såvrité, vos vos dvrîz elodjî eyet rcandjî vosse sicret å pus abeye.

Si ene ôte sakî a fwait l' dimande, ou si vos vs avoz rtrové l' vî scret eyet nel pus vleur candjî, vos ploz djusse ignorer ci messaedje ci eyet continouwer avou l' vî scret.",
'noemail'                    => "I n' a pont d' adresse emile di cnoxhowe po l' uzeu «$1».",
'passwordsent'               => "On novea scret a stî emilé a l' adresse emile
racsegneye po l' uzeu «$1».
Relodjîz vs avou ç' noû scret on côp ki vos l' åroz rçuvou s' i vs plait.",
'eauthentsent'               => "Èn emile d' acertinaedje a stî evoyî a l' adresse emile tchoezeye.
Divant d' poleur evoyî èn ôte emile a ci conte la, vos dvroz shure les instruccions di l' emile ki vos alez rçure, po-z acertiner ki l' conte est bén da vosse.",
'mailerror'                  => "Åk n' a nén stî tot-z evoyant l' emile: $1",
'acct_creation_throttle_hit' => "Mande escuzes, mins vos avoz ddja ahivé $1 contes. Vos n' endè ploz nén fé des ôtes.",
'emailauthenticated'         => 'Voste adresse emile a stî acertinêye li $1.',
'emailnotauthenticated'      => "Voste adresse emile n' a nén co stî acertinêye. Nol emile ni serè-st evoyî po les fonccions shuvantes.",
'noemailprefs'               => 'Dinez ene adresse emile po ces fonccions si mete en alaedje.',
'emailconfirmlink'           => 'Acertinaedje di voste adresse emile',
'invalidemailaddress'        => "L' adresse emile ni pout nén esse acceptêye la k' i shonnreut k' ele soeye dins ene cogne nén valide. Tapez ene adresse emile sicrîte comifåt oudobén vudîz l' tchamp, s' i vs plait.",
'accountcreated'             => 'Conte ahivé',
'accountcreatedtext'         => "Li conte d' uzeu «$1» a stî ahivé.",
'loginlanguagelabel'         => 'Lingaedje: $1',

# Edit page toolbar
'bold_sample'     => 'Cråssès letes',
'bold_tip'        => 'Tecse e cråssès letes',
'italic_sample'   => 'Clintcheyès letes',
'italic_tip'      => 'Tecse e clintcheyès letes',
'link_sample'     => 'Tecse pol loyén',
'link_tip'        => 'Divintrin loyén',
'extlink_sample'  => 'http://www.example.com tecse pol hårdêye',
'extlink_tip'     => 'Difoûtrinne hårdêye (en rovyîz nén di mete «http://» pa dvant)',
'headline_sample' => 'Tecse di tite',
'headline_tip'    => 'Tite di 2inme livea',
'math_sample'     => "Tapez l' formule matematike chal",
'math_tip'        => 'Formule matematike (LaTeX)',
'nowiki_sample'   => "Tapez l' tecse nén wiki chal",
'nowiki_tip'      => 'Èn nén analijhî des côdes wiki, eyet purade les håyner sins fôrmater',
'image_sample'    => 'Egzimpe.jpg',
'image_tip'       => 'Ravalêye imådje',
'media_sample'    => 'Egzimpe.ogg',
'media_tip'       => 'Loyén viè on fitchî multimedia (come do son evnd)',
'sig_tip'         => "Li sinateure da vosse, avou l' date et l' eure",
'hr_tip'          => "Roye di coûtchî (a n' nén eployî d' trop)",

# Edit pages
'summary'                   => 'Rascourti',
'subject'                   => 'Sudjet/tiestire',
'minoredit'                 => "Ci n' est k' ene tchitcheye",
'watchthis'                 => 'Shuve cist årtike',
'savearticle'               => "Schaper l' pådje",
'preview'                   => 'Vey divant',
'showpreview'               => 'Vey divant',
'showlivepreview'           => 'Vey divant',
'showdiff'                  => 'Vey les candjmints',
'anoneditwarning'           => "'''Asteme:''' Vos n' estoz nén elodjî. Voste adresse IP serè rashiowe dins l' istwere di cisse pådje ci.",
'missingsummary'            => "'''Asteme:''' Vos n' avoz nén dné on tecse di rascourti po vosse candjmint. Si vos rclitchîz sol boton «Schaper», li candjmint da vosse serè schapé sins nou tecse di rascourti po l' istwere del pådje.",
'missingcommenttext'        => "Tapez on comintaire chal pa dzo s' i vs plait.",
'blockedtitle'              => "L' uzeu est bloké",
'blockedtext'               => "Vosse no d' uzeu ou voste adresse IP a stî blokêye pa $1.
Li råjhon dnêye est:<br />''$2''<p>Vos ploz contacter $1 oudonbén onk des
[[{{MediaWiki:Grouppage-sysop}}|manaedjeus]] po discuter do blocaedje.

Notez ki vos n' poloz nén eployî l' fonccion «emiler a l' uzeu» a moens ki vos åyîz ene adresse emile valide dins vos [[Special:Preferences|preferinces]].

Voste adresse IP est $3. S' i vs plait racsegnoz ciste adresse IP la dins les dmandes ki vos frîz.",
'blockedoriginalsource'     => "Li sourdant di '''$1''' est håyné chal pa dzo:",
'blockededitsource'         => "Li tecse des '''candjmints da vosse''' di '''$1''' est håyné chal pa dzo:",
'whitelistedittitle'        => "S' elodjî po candjî",
'whitelistedittext'         => 'I vs fåt $1 po pleur candjî les årtikes.',
'confirmedittitle'          => 'Acertiner vost emile po candjî',
'confirmedittext'           => "I vs fåt acertiner vost emile po pleur candjî les årtikes. Dinez èn emile eyet l' acertiner dins vos [[Special:Preferences|preferinces d' uzeu]].",
'loginreqtitle'             => 'I vs fåt esse elodjî',
'loginreqlink'              => 'elodjî',
'loginreqpagetext'          => 'Vos vs divoz $1 po vey des ôtès pådjes.',
'accmailtitle'              => 'Li scret a stî evoyî.',
'accmailtext'               => 'Li scret po «$1» a stî evoyî a $2.',
'newarticle'                => '(Novea)',
'newarticletext'            => "Vos avoz clitchî so on loyén viè ene pådje ki n' egzistêye nén co.
Mins '''vos''' l' poloz askepyî! Po çoula, vos n' avoz k' a cmincî a taper vosse tecse dins l' boesse di tecse chal pa dzo (alez vey li [[{{MediaWiki:Helppage}}|pådje d' aidance]] po pus d' infôrmåcion).
Si vos n' voloz nén scrire cisse pådje chal, clitchîz simplumint sol boton '''En erî''' di vosse betchteu waibe po rivni al pådje di dvant.",
'anontalkpagetext'          => "---- ''Çouchal, c' est li pådje di copene po èn uzeu anonime ki n' a nén (co) fwait on conte por lu s' elodjî, ou ki n' l' eploye nén. Ça fwait k' on doet eployî si adresse IP limerike po l' idintifyî. Come ene sifwaite adresse IP pout esse eployeye pa pus d' èn uzeu, i s' pout ki vos voeyoz chal des rmåkes et des messaedjes ki n' sont nén por vos. Loukîz s' i vs plait po [[Special:UserLogin|fé on novea conte ou s' elodjî]] po n' pus aveur d' ecramiaedje avou des ôtes uzeus anonimes.''",
'noarticletext'             => "I gn a pol moumint nou tecse e cisse pådje chal, vos ploz [[Special:Search/{{PAGENAME}}|cweri après l' tite di cisse pådje ci]] dins des ôtès pådjes, oudonbén [{{fullurl:{{FULLPAGENAME}}|action=edit}} ahiver l' pådje].",
'clearyourcache'            => "'''Note:''' après aveur schapé l' pådje, vos l' divoz rafrister, po pleur vey les candjmints dins vosse betchteu waibe: '''Mozilla / Firefox / Safari:''' tchôkîz so ''Shift'' to clitchant so ''Rafrister'', ou co fjhoz ''Ctrl-Shift-R'' (''Cmd-Shift-R'' so on Macintosh); '''IE:''' tchôkîz so ''Ctrl'' tot clitchant so ''Rafrister'', ou co fjhoz ''Ctrl-F5''; '''Konqueror:''' simplumint clitchîz so ''Rafrister'' ou l' tape ''F5''; les uzeus d' '''Opera''' dvront motoit netyî pår leu muchete, dins ''Usteyes→Preferinces''.",
'usercssjsyoucanpreview'    => "<strong>Racsegne:</strong> eployîz l' boton «Vey divant» po sayî vosse novea CSS/JS divant del schaper.",
'usercsspreview'            => "'''Èn rovyîz nén ki c' est djusse on prévoeyaedje di vosse stîle CSS d' uzeu, i n' a nén co stî schapé!'''",
'userjspreview'             => "'''Èn rovyîz nén ki c' est djusse on prévoeyaedje/saye di vosse JavaScript d' uzeu, i n' a nén co stî schapé!'''",
'userinvalidcssjstitle'     => "'''Asteme:''' I n' a pont d' pea lomêye «$1». Tuzez ki les pådjes .css eyet .js des uzeus eployèt des tite e ptitès letes, metans {{ns:user}}:Toto/monobook.css et nén {{ns:user}}:Toto/Monobook.css.",
'updated'                   => '(Ramidré)',
'previewnote'               => "<strong>Èn rovyîz nén ki c' est djusse on prévoeyaedje, li pådje n' est nén co schapêye!</strong>",
'previewconflict'           => 'Ci prévoeyaedje ci mostere kimint kel tecse del boesse di tecse do dzeu sereut håyné si vos decidez di clitchî so «schaper».',
'session_fail_preview'      => "<strong>Mande escuzes! Mins dji n' a nén polou traitî vosse candjmint paski les dnêyes del session ont stî pierdowes.
Rissayîz s' i vs plait. Si çoula n' va todi nén, sayîz di vs dislodjî eyet di vs relodjî.</strong>",
'session_fail_preview_html' => "<strong>Mande escuzes! Mins dji n' a nén polou traitî vosse candjmint paski les dnêyes del session ont stî pierdowes.</strong>

''Come ci wiki chal a-st en alaedje li HTML brut, li prévoeyaedje est catchî, come proteccion siconte des atakes JavaScript.''

<strong>Si c' est ene saye oniesse di candjî l' pådje, rissayîz s' i vs plait. Si çoula n' va todi nén, sayîz di vs dislodjî eyet di vs relodjî.</strong>",
'editing'                   => 'Candjant $1',
'editingsection'            => 'Candjant $1 (seccion)',
'editingcomment'            => 'Candjant $1 (comintaire)',
'editconflict'              => 'Conflit inte deus candjmints: $1',
'explainconflict'           => "Ene sakî a candjî l' pådje do tins ki vos estîz a scrire.
Li boesse di tecse do dzeur mostere li tecse del pådje come il est
pol moumint sol sierveu. Li tecse da vosse est sol boesse di tecse do dzo.
Les diferinces sont håynêyes å mitan. Vos dvoz mete vos candjmints dins
l' tecse d' asteure (å dzeur) si vos lez vloz co evoyî.
<b>Seulmint</b> li tecse do dzeur serè candjî cwand vos clitchroz sol
boton «Schaper l' pådje».<br />",
'yourtext'                  => 'Li tecse da vosse',
'storedversion'             => 'Modêye sol sierveu',
'nonunicodebrowser'         => "<strong>ASTEME: li betchteu waibe da vosse ni sopoite nén l' ecôdaedje unicôde, cåze di çoula les caracteres nén-ASCII vont aparexhe dins l' boesse di tecse come des côdes hecsadecimås, insi vos pôroz tot l' minme candjî l' pådje.</strong>",
'editingold'                => "<strong>ASTEME: Vos estoz ki candje ene viye modêye del pådje.
Si vos l' schapez, tos les candjmints k' ont stî fwaits
dispoy adon si vont piede.</strong>",
'yourdiff'                  => 'Diferinces',
'copyrightwarning'          => "Notez ki tos les contribouwaedjes fwaits po {{SITENAME}} dvèt esse dizo l' licince $2 (loukîz $1 po pus di racsegnes).
Si vos n' voloz nén ki vosse tecse poye esse candjî eyet spårdou pa tot l' minme kî, adon nel evoyîz nén chal.<br />
Vos nos acertinez eto ki vos avoz scrît l' tecse vos-minme, oudonbén l' avoz copyî d' on sourdant libe (dominne publik ou on sourdant pareymint libe).
<strong>N' EVOYÎZ NÉN DES TECSES DIZO ABONDROETS SINS PERMISSION!</strong>",
'copyrightwarning2'         => "Notez ki tos les contribouwaedjes fwaits po {{SITENAME}} polèt esse esse candjîs ou disfacés pa des ôtes contribouweus.
Si vos n' voloz nén scrire des årtikes ki polèt esse candjîs pa des ôtes, adon nels evoyîz nén chal.<br />
Vos nos acertinez eto ki vos avoz scrît l' tecse vos-minme, oudonbén l' avoz copyî d' on sourdant libe (voeyoz $1 po pus di racsegnes).
<strong>N' EVOYÎZ NÉN DES TECSES DIZO ABONDROETS SINS PERMISSION!</strong>",
'longpagewarning'           => "<strong>ASTEME: Cisse pådje fwait $1 kilo-octets; des
betchteus waibes k' i gn a polèt aveut des rujhes po-z aspougnî
des pådjes k' aprepièt ou di pus di 32Ko.
Vos dvrîz tuzer a pårti l' pådje e pus ptits bokets.</strong>",
'longpageerror'             => "<strong>AROKE: Li tecse ki vos avoz evoyî fwait di pus d' $1 kilo-octets, çou k' est pus ki l' macsimom di $2 kilo-octets. C' est nén possible del schaper sol sierveu.</strong>",
'readonlywarning'           => "<strong>ASTEME: On-z overe sol båze di dnêyes pol moumint, ey elle a stî metowe e mode seulmint-lére.
Do côp, vos n' såroz schaper vos candjmints asteure; motoit vos dvrîz copyî et aclaper l' tecse dins on fitchî da vosse pol poleur rimete pus tård.</strong>",
'protectedpagewarning'      => '<strong>ASTEME: Cisse pådje chal a stî protedjeye siconte des candjmints, seulmint les uzeus avou èn accès di manaedjeu el polèt candjî.</strong>',
'semiprotectedpagewarning'  => "'''Note:''' cisse pådje ci a stî protedjeye po k' seulmint les uzeus edjîstrés el polexhe candjî.",
'templatesused'             => 'Modeles eployîs e cisse pådje ci:',
'template-protected'        => '(protedjî)',
'template-semiprotected'    => '(dimey-protedjî)',
'nocreatetitle'             => 'Ahivaedje di pådjes limité',
'nocreatetext'              => "Cisse waibe ci a limité l' possibilité d' ahiver des novelès pådjes. Vos ploz rivni en erî eyet candjî ene pådje k' egzistêye dedja, oudonbén, [[Special:UserLogin|vos elodjî ou ahiver on conte d' uzeu]].",

# History pages
'viewpagelogs'        => 'Vey les djournås po cisse pådje ci',
'nohistory'           => "I n' a pont d' istwere des modêyes po cisse pådje chal.",
'revnotfound'         => 'Modêye nén trovêye',
'revnotfoundtext'     => "Li viye modêye del pådje ki vos avoz dmandé n' a nén stî trovêye.
Verifyîz l' hårdêye ki vs avoz eployî po-z ariver sol pådje s' i vs plait.",
'currentrev'          => "Modêye d' asteure",
'revisionasof'        => 'Modêye do $1',
'previousrevision'    => '←Modêye di dvant',
'nextrevision'        => 'Modêye shuvante→',
'currentrevisionlink' => "Modêye d' asteure",
'cur'                 => 'ast.',
'next'                => 'shuv.',
'last'                => 'dif.',
'page_first'          => 'prumî',
'page_last'           => 'dierin',
'histlegend'          => "Tchoezi les modêyes a comparer: clitchîz so les botons radio des deus modêyes
ki vos vloz comparer et s' tchôkîz sol tape «enter» ou clitchîz sol
boton do dzo.<br />
Ledjinde: (ast.) = diferince avou l' modêye d' asteure,
(dif.) = diferince avou l' modêye di dvant, M = candjmint mineur.",
'deletedrev'          => '[disfacé]',
'histfirst'           => 'li pus vî',
'histlast'            => 'li dierin',
'historysize'         => '({{PLURAL:$1|1 octet|$1 octets}})',
'historyempty'        => '(vude)',

# Revision feed
'history-feed-item-nocomment' => '$1 li $2', # user at time

# Revision deletion
'rev-deleted-comment'         => '(comintaire oisté)',
'rev-deleted-user'            => "(no d' elodjaedje oisté)",
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">
Cisse modêye ci del pådje a stî oistêye foû des årtchives publikes.
I gn a motoit des racsegnes sol [{{fullurl:Special:Log/delete|page={{PAGENAMEE}}}} djournå des disfaçaedjes].
</div>',
'rev-deleted-text-view'       => '<div class="mw-warning plainlinks">
Cisse modêye ci del pådje a stî oistêye foû des årtchives publikes.
Come manaedjeu so ç\' wiki ci, vos avoz l\' droet del vey; i gn a motoit des detays sol [{{fullurl:Special:Log/delete|page={{PAGENAMEE}}}} djournå des disfaçaedjes].
</div>',
'rev-delundel'                => 'mostrer/catchî',
'revisiondelete'              => 'Disfacer/rapexhî des modêyes',
'revdelete-selected'          => 'Tchoezeye modêye di [[:$1]]:',
'logdelete-selected'          => "{{PLURAL:$2|Evenmint tchoezi|Evenmints tchoezis}} ezès djournås po '''$1:'''",
'revdelete-text'              => "Les disfacêyès modêyes vont continouwer d' aparexhe dins l' pådje di l' istwere, mins leu contnou n' serè nén veyåve do publik.

Les ôtes manaedjeus so ç' wiki ci pôront todi vey li contnou catchî eyet l' rapexhî åd triviè di cisse minme eterface ci, a moens k' ene restriccion di pus ni soeye metowe en alaedje pås mwaisses-manaedjeus del waibe.",
'revdelete-legend'            => 'Defini des restriccions sol modêye',
'revdelete-hide-text'         => "Catchî l' tecse del modêye",
'revdelete-hide-comment'      => "Catchî l' comintaire di candjmint",
'revdelete-hide-user'         => "Catchî l' no d' uzeu/adresse IP do candjeu",
'revdelete-hide-restricted'   => 'Apliker ces restrictions ossu åzès manaedjeus',
'revdelete-log'               => 'Comintaire pol djournå:',
'revdelete-submit'            => 'Apliker al modêye tchoezeye',
'revdelete-logentry'          => 'li veyåvisté des modêyes a stî candjeye po [[$1]]',

# Diffs
'difference'              => '(Diferinces inte les modêyes)',
'lineno'                  => 'Roye $1:',
'compareselectedversions' => 'Comparer les modêyes tchoezeyes',
'editundo'                => 'disfé',

# Search results
'searchresults'         => 'Rizultats do cweraedje',
'searchresulttext'      => 'Po pus di racsegnes sol manire di fé des cweraedjes so {{SITENAME}}, loukîz [[{{MediaWiki:Helppage}}|{{int:help}}]].',
'searchsubtitle'        => 'Pol cweraedje «[[$1]]»',
'searchsubtitleinvalid' => 'Pol cweraedje «$1»',
'noexactmatch'          => "'''I n' a nole pådje avou l' tite «$1».''' Vos poloz [[:$1|ahiver cisse pådje la]].",
'titlematches'          => 'Årtikes avou on tite ki corespond',
'notitlematches'        => 'Nol årtike avou on tite ki corespond',
'textmatches'           => 'Årtikes avou do tecse ki corespond',
'notextmatches'         => 'Nol årtike avou do tecse ki corespond',
'prevn'                 => '$1 di dvant',
'nextn'                 => '$1 shuvants',
'viewprevnext'          => 'Vey ($1) ($2) ($3).',
'showingresults'        => 'Chal pa dzo <b>$1</b> rizultats a pårti do limero <b>$2</b>.',
'showingresultsnum'     => 'Chal pa dzo <b>$3</b> rizultats a pårti do limero <b>$2</b>.',
'nonefound'             => "'''Note''': des cweraedjes ki n' dinèt nou rzultat c' est sovint li cweraedje di ptits mots trop corants (come «les», «des») ki n' sont nén indecsés, oudonbén des cweraedjes di pus d' on mot (seulmint les pådjes avou tos les mots dmandés sront håynêyes dins l' rizultat do cweraedje).",
'powersearch'           => 'Cweri',
'searchdisabled'        => "Mande escuzes! Li cweraedje å dvins des årtikes a stî dismetou pol moumint, cåze ki l' sierveu est fortcherdjî. Tot ratindant, vos ploz eployî Google po fé les rcweraedjes so {{SITENAME}}, mins çoula pout esse ene miete vî.",

# Preferences page
'preferences'              => 'Preferinces',
'mypreferences'            => 'Mes preferinces',
'prefs-edits'              => 'Nombe di candjmints:',
'prefsnologin'             => "Vos n' estoz nén elodjî",
'prefsnologintext'         => 'I vs fåt esse [[Special:UserLogin|elodjî]] po pleur candjî vos preferinces.',
'prefsreset'               => "Les preferinces ont stî rmetowes come d' avance a pårti des wårdêyès valixhances.",
'qbsettings'               => 'Apontiaedjes pol bår di menu',
'qbsettings-none'          => 'Nole bår',
'qbsettings-fixedleft'     => 'Aclawêye a hintche',
'qbsettings-fixedright'    => 'Aclawêye a droete',
'qbsettings-floatingleft'  => 'Flotante a hintche',
'qbsettings-floatingright' => 'Flotante a droete',
'changepassword'           => "Candjî l' sicret",
'skin'                     => 'Pea',
'math'                     => 'Formules matematikes',
'dateformat'               => 'Cogne del date',
'datedefault'              => 'Nole preferince',
'datetime'                 => 'Cogne del date',
'math_unknown_error'       => 'aroke nén cnoxhowe',
'math_unknown_function'    => 'fonccion nén cnoxhowe',
'math_syntax_error'        => 'aroke di sintacse',
'math_image_error'         => 'Li cviersaedje e PNG a fwait berwete; verifyîz ki les programes latex, dvips, gs eyet convert ont stî astalés comifåt',
'math_bad_tmpdir'          => "Dji n' sai nén scrire ou ahiver l' ridant timporaire po les formules matematikes",
'math_bad_output'          => "Dji n' sai nén scrire ou ahiver l' ridant po les fitchîs di rexhowe des formules matematikes",
'math_notexvc'             => 'I manke li fitchî enondåve texvc; lijhoz math/README po-z apontyî.',
'prefs-personal'           => 'Dinêyes da vosse',
'prefs-rc'                 => 'Håynaedje des dierins candjmints',
'prefs-watchlist'          => 'Djivêye des shuvous',
'prefs-watchlist-days'     => "Nombe di djoûs a mostrer dins l' djivêye:",
'prefs-watchlist-edits'    => "Nombe di candjmints a mostrer dins l' djivêye:",
'prefs-misc'               => 'Totes sôres',
'saveprefs'                => 'Schaper les preferinces',
'resetprefs'               => 'Rimete les prémetowès valixhances',
'oldpassword'              => 'Vî scret',
'newpassword'              => 'Noû scret',
'retypenew'                => "Ritapez l' noû scret",
'textboxsize'              => 'Grandeu del boesse di tecse',
'rows'                     => 'Royes',
'columns'                  => 'Colones',
'searchresultshead'        => 'Håynaedje des rzultats di cweraedje',
'resultsperpage'           => 'Nombe di responses a håyner so ene pådje',
'contextlines'             => 'Nombe di royes a håyner po ene response',
'contextchars'             => 'Nombe di caracteres di contecse pa roye',
'recentchangesdays'        => 'Nombe di djoûs po les dierins candjmints:',
'recentchangescount'       => 'Nombe di tites dins les dierins candjmints',
'savedprefs'               => 'Vos preferinces ont stî schapêyes.',
'timezonelegend'           => "Coisse d' eureye",
'timezonetext'             => "¹Tapez li nombe d' eures di diferince avou l' tins univiersel (UTC).",
'localtime'                => "Håyner l' eure locåle",
'timezoneoffset'           => "Diferince d' eures¹",
'servertime'               => "L' eure sol sierveu",
'guesstimezone'            => "Prinde d' après l' betchteu",
'allowemail'               => "Permete di rçure des emiles d' ôtes uzeus",
'defaultns'                => 'Prémetous spåces di nos pol cweraedje:',
'default'                  => 'prémetou',
'files'                    => 'Fitchîs',

# User rights
'userrights'               => 'Manaedjî les liveas des uzeus', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'   => "Manaedjî les groupes d' èn uzeu",
'userrights-user-editname' => "Tapez on no d' uzeu:",
'editusergroup'            => "Candjî les groupes di l' uzeu",
'editinguser'              => "Candjant '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup' => "Candjî groupes d' uzeus",
'saveusergroups'           => "Schaper des groupes d' uzeus",
'userrights-groupsmember'  => 'Mimbes di:',
'userrights-reason'        => 'Råjhon do candjmint:',

# Groups
'group'            => 'Groupe:',
'group-bot'        => 'Robots',
'group-sysop'      => 'Manaedjeus',
'group-bureaucrat' => 'Mwaisse-manaedjeus',
'group-all'        => '(tertos)',

'group-bot-member'        => 'robot',
'group-sysop-member'      => 'manaedjeu',
'group-bureaucrat-member' => 'mwaisse-manaedjeu',

'grouppage-bot'        => '{{ns:project}}:Robots',
'grouppage-sysop'      => '{{ns:project}}:Manaedjeus',
'grouppage-bureaucrat' => '{{ns:project}}:Mwaisse-manaedjeus',

# User rights log
'rightslog'      => 'Djournå des droets des uzeus',
'rightslogtext'  => "Çouchal, c' est on djournå des candjmints des droets des uzeus.",
'rightslogentry' => "l' uzeu «$1» a stî candjî do groupe «$2» viè «$3»",
'rightsnone'     => '(nouk)',

# Recent changes
'nchanges'                          => '$1 {{PLURAL:$1|candjmint|candjmints}}',
'recentchanges'                     => 'Dierins candjmints',
'recentchangestext'                 => "Shuvoz chal les dierins candjmints k' i gn a yeu dsu {{SITENAME}}.",
'rcnote'                            => 'Chal pa dzo les <strong>$1</strong> dierins candjmints des dierins <strong>$2</strong> djoûs, å $3.',
'rcnotefrom'                        => "Chal pa dzo les candjmints dispoy li <b>$2</b> (disk' a <b>$1</b> di mostrés).",
'rclistfrom'                        => "Mostrer les candjmints k' i gn a yeu a pårti do $1",
'rcshowhideminor'                   => '$1 candjmints mineurs',
'rcshowhidebots'                    => '$1 robots',
'rcshowhideliu'                     => '$1 uzeus eredjîstrés',
'rcshowhideanons'                   => '$1 uzeus anonimes',
'rcshowhidepatr'                    => '$1 candjmints rwaitîs',
'rcshowhidemine'                    => '$1 candjmints da minne',
'rclinks'                           => 'Mostrer les $1 dierins candjmints des dierins $2 djoûs.<br />$3',
'diff'                              => 'dif.',
'hist'                              => 'ist.',
'hide'                              => 'catch.',
'show'                              => 'håy.',
'number_of_watching_users_pageview' => '[shuvou pa $1 uzeu(s)]',
'rc_categories'                     => 'Limiter åzès categoreyes (separer avou des «|»)',
'rc_categories_any'                 => 'Totes',
'newsectionsummary'                 => '/* $1 */ novele seccion',

# Recent changes linked
'recentchangeslinked' => 'Candjmints aloyîs',

# Upload
'upload'                      => 'Eberweter on fitchî',
'uploadbtn'                   => 'Eberweter',
'reupload'                    => 'En erî',
'reuploaddesc'                => "Rivni al pådje d' eberwetaedje.",
'uploadnologin'               => 'Nén elodjî',
'uploadnologintext'           => 'I vs fåt esse [[Special:UserLogin|elodjî]] por vos pleur eberweter des fitchîs.',
'upload_directory_read_only'  => "Li sierveu waibe èn pout nén scrire sol ridant d' eberwetaedje ($1).",
'uploaderror'                 => "Aroke d' eberwetaedje",
'uploadtext'                  => "Eployîz les boesses d' intrêye chal pa dzo po-z eberweter des noveas fitchîs d' imådjes po vos årtikes. Sol plupårt des betchteus, vos voeroz on boton «Foyter...» (ou «Browse...») ki vs permetrè di foyter dins les ridants del deure plake da vosse po tchoezi l' fitchî, çou ki rimplirè otomaticmint li tchamp do no do fitchî k' est a costé.

Po håyner ou cweri des imådjes k' ont ddja stî rçuvowes, alez sol [[Special:ImageList|djivêye des imådjes dedja eberwetêyes]]. Les eberwetaedjes et disfaçaedjes sont metous èn on [[Special:Log/upload|djournå des eberwetaedjes]].

Po håyner l' imådje dins èn årtike, eployîz on loyén del foûme
* '''<nowiki>[[</nowiki>{{ns:image}}<nowiki>:Fitchî.jpg]]</nowiki>'''
* '''<nowiki>[[</nowiki>{{ns:image}}<nowiki>:Fitchî.png|thumb|tecse a mete padzo]]</nowiki>'''
ou co po les sons
* '''<nowiki>[[</nowiki>{{ns:media}}<nowiki>:Fitchî.ogg]]</nowiki>'''",
'uploadlog'                   => 'djournå des eberwetaedjes',
'uploadlogpage'               => 'Djournå des eberwetaedjes',
'uploadlogpagetext'           => 'Chal pa dzo li djivêye des dierins eberwetaedjes.',
'filename'                    => 'No do fitchî',
'filedesc'                    => 'Discrijhaedje',
'fileuploadsummary'           => 'Discrijhaedje:',
'filestatus'                  => 'Abondroets ey eployaedje:',
'filesource'                  => 'Sourdant:',
'uploadedfiles'               => 'Fitchîs eberwetés',
'ignorewarning'               => "Passer houte des adviertixhmints eyet schaper tot l' minme li fitchî.",
'ignorewarnings'              => 'Passer houte des adviertixhmints',
'illegalfilename'             => "Li no d' fitchî «$1» a des caracteres ki n' si polèt nén eployî dins l' tite d' ene pådje. Candjîz l' no do fitchî eyet sayîz del reberweter s' i vs plait.",
'badfilename'                 => "Li no d' l' imådje a stî candjî a «$1».",
'largefileserver'             => "Ci fitchî ci est pus pezant ki çou k' li sierveu est apontyî po-z accepter.",
'emptyfile'                   => "I shonnreut kel fitchî k' vos eberwetez soeye vude. Çoula pout esse cåze d' ene aroke di tapaedje dins l' no do fitchî. Acertinez si vos vloz evoyî po do bon ç' fitchî ci, s' i vs plait.",
'fileexists'                  => "On fitchî avou ç' no la egzistêye dedja, loukîz s' i vs plait a <strong><tt>$1</tt></strong> po vs acertiner ki vos vloz bén replaecî l' fitchî avou l' ci ki vos eberwetez asteure, oubén si vos l' voloz eberweter dizo èn ôte no.",
'fileexists-forbidden'        => "I gn a ddja on fitchî avou ç' no la; rivnoz s' i vs plait en erî et s' reberwetez l' fitchî dizo èn ôte no. [[Image:$1|thumb|center|$1]]",
'fileexists-shared-forbidden' => "I gn a ddja on fitchî avou ç' no la e ridant des fitchîs pårtaedjîs; rivnoz s' i vs plait en erî et s' reberwetez l' fitchî dizo èn ôte no. [[Image:$1|thumb|center|$1]]",
'successfulupload'            => "L' eberwetaedje a stî comifåt",
'uploadwarning'               => "Adviertixhmint so l' eberwetaedje",
'savefile'                    => "Schaper l' fitchî",
'uploadedimage'               => 'eberwetaedje di «[[$1]]»',
'uploaddisabled'              => 'Eberwetaedje di fitchîs dismetou',
'uploaddisabledtext'          => "Mande escuzes, mins l' eberwetaedje di fitchîs a stî dismetou pol moumint.",
'uploadscripted'              => 'Ci fitchî ci a-st å dvins do côde HTML ou on scripe ki pôreut esse må comprin pa on betchteu waibe.',
'uploadcorrupt'               => "Li fitchî est cron oudonbén il a-st ene mwaijhe cawete. Verifyîz l' fitchî eyet l' reberweter s' i vs plait.",
'uploadvirus'                 => 'Li fitchî a-st on virusse! Detays: $1',
'sourcefilename'              => "No d' fitchî so vosse copiutrece:",
'destfilename'                => "No d' fitchî a eployî so {{SITENAME}}:",
'filewasdeleted'              => "On fitchî avou ç' no la a ddja stî disfacé. Vos dvrîz loukî å $1 divant d' continouwer.",

'upload-file-error' => 'Divintrinne aroke',

'license'            => "Licince di l' imådje",
'nolicense'          => 'Nole licince tchoezeye',
'upload_source_file' => ' (on fitchî sol copiutrece da vosse)',

# Special:ImageList
'imagelist_search_for'  => "Cweri l' no d' imådje:",
'imgfile'               => 'fitchî',
'imagelist'             => 'Djivêye des imådjes',
'imagelist_name'        => 'No',
'imagelist_user'        => 'Uzeu',
'imagelist_size'        => 'Grandeu',
'imagelist_description' => 'Discrijhaedje',

# Image description page
'filehist-deleteall'        => 'disfacer ttafwait',
'filehist-deleteone'        => 'disfacer çouci',
'filehist-datetime'         => 'Date/Eure',
'filehist-user'             => 'Uzeu',
'filehist-filesize'         => 'Grandeu do fitchî',
'filehist-comment'          => 'Comintaire',
'imagelinks'                => 'Loyéns viè ciste imådje chal',
'linkstoimage'              => 'Les pådjes shuvantes eployèt ciste imådje chal:',
'nolinkstoimage'            => "I n' a nole pådje k' eploye ciste imådje chal.",
'sharedupload'              => "Ci fitchî ci est so on ridant pårtaedjî ey i s' pout k' i soeye eployî pa ds ôtes pordjets.",
'shareduploadwiki'          => 'Loukîz li $1 po pus di racsegnes.',
'shareduploadwiki-linktext' => 'pådje di discrijhaedje',
'noimage'                   => "I n' a nou fitchî avou ç' no la, vos l' poloz $1",
'noimage-linktext'          => 'eberweter',
'uploadnewversion-linktext' => 'Eberweter ene nouve modêye di ci fitchî ci',

# File reversion
'filerevert-comment' => 'Comintaire:',

# File deletion
'filedelete'         => 'Disfacer $1',
'filedelete-legend'  => 'Disfacer fitchî',
'filedelete-comment' => 'Comintaire:',
'filedelete-submit'  => 'Disfacer',

# MIME search
'mimesearch' => 'Cweraedje MIME',
'mimetype'   => 'sôre MIME:',
'download'   => 'aberweter',

# Unwatched pages
'unwatchedpages' => 'Pådjes nén shuvowes',

# List redirects
'listredirects' => 'Djivêye des redjiblaedjes',

# Unused templates
'unusedtemplates'     => 'Modeles nén eployîs',
'unusedtemplatestext' => "Cisse pådje ci mostere totes les pådjes di modele (espåce di lomaedje «{{ns:template}}») ki n' sont nén eployîs dins ene ôte pådje. Rimimbrez vs di verifyî s' i n' a nén des ôtes loyéns divant delzès disfacer.",
'unusedtemplateswlh'  => 'ôtes loyéns',

# Random page
'randompage' => "Årtike a l' astcheyance",

# Random redirect
'randomredirect' => "Redjiblaedje a l' astcheyance",

# Statistics
'statistics'             => 'Sitatistikes',
'sitestats'              => 'Sitatistikes di {{SITENAME}}',
'userstats'              => 'Sitatistikes des uzeus',
'sitestatstext'          => "I gn a '''$1''' pådjes å totå el båze di dnêyes.
Çoula tot contant les pådjes di «Copenes», les pådjes åd fwait di {{SITENAME}}, les pådjes «djermons» (pådjes sins waire di contnou), les redjiblaedjes, eyet co ds ôtes ki n' sont nén vormint des årtikes.
Si on n' conte nén ces la, i gn a '''$2''' pådjes ki sont
probåblumint des vraiys årtikes.

'''$8''' fitchîz ont stî eberwetés.

I gn a-st avou å totå '''$3''' riwaitaedjes di pådjes, eyet '''$4''' candjmints do contnou des pådjes dispoy ki ci wiki chal est en alaedje.
Dj' ô bén k' i gn a ene moyene di '''$5''' candjmints par pådje, eyet '''$6''' riwaitaedjes po on candjmint.

Li longueur del [http://www.mediawiki.org/wiki/Manual:Job_queue cawêye des bouyes] est di '''$7'''.",
'userstatstext'          => "I gn a '''$1''' uzeus d' eredjîstrés.
'''$2''' (ou '''$4%''') di zels sont eto des manaedjeus (riloukîz a $3).",
'statistics-mostpopular' => 'Pådjes les pus veyowes',

'disambiguations'     => "Pådjes d' omonimeye",
'disambiguationspage' => 'Template:Omonimeye',

'doubleredirects'     => 'Dobes redjiblaedjes',
'doubleredirectstext' => "Tchaeke roye a-st on loyén viè l' prumî eyet l' deujhinme redjiblaedje, avou on mostraedje del prumire roye do tecse do deujhinme redjiblaedje, çou ki å pus sovint dene li «vraiy» årtike såme, ki l' prumî redjiblaedje divreut evoyî viè lu.",

'brokenredirects'        => 'Pierdous redjiblaedjes',
'brokenredirectstext'    => "Les redjiblaedjes shuvants evoyèt so ene pådje ki n' egzistêye nén.",
'brokenredirects-edit'   => '(candjî)',
'brokenredirects-delete' => '(disfacer)',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|octet|octets}}',
'ncategories'             => '$1 {{PLURAL:$1|categoreye|categoreyes}}',
'nlinks'                  => '$1 {{PLURAL:$1|loyén|loyéns}}',
'nmembers'                => '$1 {{PLURAL:$1|mimbe|mimbes}}',
'nrevisions'              => '$1 {{PLURAL:$1|modêye|modêyes}}',
'nviews'                  => 'léjhowe $1 {{PLURAL:$1|côp|côps}}',
'lonelypages'             => 'Pådjes ôrfulinnes',
'uncategorizedpages'      => 'Pådjes sins nole categoreye',
'uncategorizedcategories' => 'Categoreyes nén categorijheyes',
'uncategorizedimages'     => 'Imådjes nén categorijheyes',
'uncategorizedtemplates'  => 'Modele nén categorijhî',
'unusedcategories'        => 'Categoreyes nén eployeyes',
'unusedimages'            => 'Imådjes nén eployeyes',
'popularpages'            => 'Pådjes les pus léjhowes',
'wantedcategories'        => 'Categoreyes les pus rcwerowes',
'wantedpages'             => 'Pådjes les pus rcwerowes',
'mostlinked'              => 'Pådjes les pus loyeyes',
'mostlinkedcategories'    => 'Categoreyes les pus loyeyes',
'mostlinkedtemplates'     => 'Modeles les pus eployîs',
'mostcategories'          => "Årtikes avou l' pus di categoreyes",
'mostimages'              => 'Imådjes les pus loyeyes',
'mostrevisions'           => "Årtikes avou l' pus di candjmints",
'prefixindex'             => 'Indecse pa betchete',
'shortpages'              => 'Coûtès pådjes',
'longpages'               => 'Longowès pådjes',
'deadendpages'            => 'Pådjes sins nou loyén wiki',
'protectedpages'          => 'Pådjes protedjeyes',
'listusers'               => 'Djivêye des uzeus',
'newpages'                => 'Novelès pådjes',
'ancientpages'            => 'Viyès pådjes',
'move'                    => 'Displaecî',
'movethispage'            => 'Displaecî cisse pådje',
'unusedimagestext'        => "Notez tot l' minme ki d' ôtès waibes polèt aveur des loyéns viè ces imådjes la gråcès a ene direke hårdêye. Do côp, ces imådjes aparexhèt chal, mågré k' ele soeyexhe eployeyes.",
'unusedcategoriestext'    => "Les pådjes di categoreye shuvantes egzistént, mins i n' a nol årtike ni categoreye å dvins.",

# Book sources
'booksources' => 'Sourdants po les lives',

# Special:Log
'specialloguserlabel'  => 'Uzeu:',
'speciallogtitlelabel' => 'Tite:',
'log'                  => 'Djournås',
'alllogstext'          => "Håynaedje etercroejhlé des djournås d' eberwetaedje, disfaçaedje, protedjaedje, blocaedje eyet manaedjeus.
Vos ploz limiter l' håynaedje tot tchoezixhant ene sôre di djournå, on no d' uzeu, ou l' tite d' ene pådje.",
'logempty'             => "Rén n' corespond dins l' djournå.",

# Special:AllPages
'allpages'          => 'Totes les pådjes',
'alphaindexline'    => 'di $1 a $2',
'nextpage'          => 'Pådje shuvante ($1)',
'allpagesfrom'      => 'Håyner les pådjes a pårti di:',
'allarticles'       => 'Tos les årtikes',
'allinnamespace'    => 'Totes les pådjes (espåce di lomaedje $1)',
'allnotinnamespace' => "Totes les pådjes (foû d' l' espåce di lomaedje $1)",
'allpagesprev'      => 'Di dvant',
'allpagesnext'      => 'Shuvant',
'allpagessubmit'    => 'I va',
'allpagesprefix'    => "Håyner les pådjes avou l' betchete:",
'allpagesbadtitle'  => "Li tite di pådje diné n' est nén valide oudonbén il a-st ene betchete di loyén eterlingaedje ou eterwiki. Ça s' pout k' il åye onk ou d' pus d' caracteres ki n' si polèt nén eployî dins les tites.",

# Special:Categories
'categories'         => 'Categoreyes',
'categoriespagetext' => 'I gn a les categoreyes shuvantes sol wiki.',

# E-mail user
'mailnologin'     => "Nole adresse d' evoyeu",
'mailnologintext' => "Po-z evoyî èn emile a èn ôte uzeu i vs fåt esse [[Special:UserLogin|elodjî]] eyet aveur ene adresse emile d' evoyeu ki soeye valide dins vos [[Special:Preferences|preferinces]].",
'emailuser'       => "Emiler a l' uzeu",
'emailpage'       => 'Emilaedje a èn uzeu',
'emailpagetext'   => "Si cist uzeu chal a dné ene adresse emile valide dins
ses preferinces, vos lyi ploz evoyî èn emile a pårti di cisse pådje chal.
L' adresse emile k' i gn a dins vos preferinces serè-st eployeye
come adresse di l' evoyeu (adresse «From:» di l' emile),
po ki l' riçuveu poye risponde.",
'usermailererror' => "Åk n' a nén stî tot voyant l' emile:",
'defemailsubject' => 'Emile da {{SITENAME}}',
'noemailtitle'    => "Pont d' adresse emile",
'noemailtext'     => "Cist uzeu chal n' a nén dné d' adresse emile
valide, ou n' vout nén rçure des emiles des ôtes uzeus.
Do côp, c' est nén possibe di lyi evoyî èn emile.",
'emailfrom'       => 'Di',
'emailto'         => 'Po',
'emailsubject'    => 'Sudjet',
'emailmessage'    => 'Messaedje',
'emailsend'       => 'Evoyî',
'emailsent'       => 'Emile evoyî',
'emailsenttext'   => 'Vost emilaedje a stî evoyî comifåt.',

# Watchlist
'watchlist'            => 'Pådjes shuvowes',
'mywatchlist'          => 'Pådjes shuvowes',
'watchlistfor'         => "(po l' uzeu «'''$1'''»)",
'nowatchlist'          => 'Vosse djivêye des pådjes a shuve est vude.',
'watchlistanontext'    => 'I vs fåt $1 po vey ou candjî les cayets di vosse djivêye des shuvous.',
'watchnologin'         => "Vos n' estoz nén elodjî",
'watchnologintext'     => 'I vs fåt esse [[Special:UserLogin|elodjî]] po pleur candjî vosse djivêye des pådjes a shuve.',
'addedwatch'           => 'Radjouté ås shuvous',
'addedwatchtext'       => "Li pådje «<nowiki>$1</nowiki>» a stî radjoutêye a vosse [[Special:Watchlist|djivêye des pådjes a shuve]].
Tos les candjmints k' i gn årè di cisse pådje chal,
eyet di si pådje di copene, seront håynés chal, eyet li pådje serè metowe e '''cråssès letes'''
el [[Special:RecentChanges|djivêye des dierins candjmints]] po k' ça soeye pus åjhey por vos del rimårker.

Si vos vloz bodjî l' pådje foû di vosse djivêye des shuvous, clitchîz so «Èn pus shuve li pådje» dins l' bår di menu sol costé.",
'removedwatch'         => 'Bodjî foû des shuvous',
'removedwatchtext'     => 'Li pådje «<nowiki>$1</nowiki>» a stî bodjeye foû di vosse djivêye des pådjes a shuve.',
'watch'                => 'Shuve',
'watchthispage'        => 'Shuve cisse pådje',
'unwatch'              => 'Èn pus shuve',
'unwatchthispage'      => 'Èn pus shuve li pådje',
'notanarticle'         => 'Nén èn årtike',
'watchnochange'        => "Nole des pådjes di vosse djivêye di pådjes a shuve n' a stî candjeye dins l' termene di tins dmandêye.",
'watchlist-details'    => '{{PLURAL:$1|$1 pådje shuvowe|$1 pådjes shuvowes}} (sins conter les pådjes di copene).',
'wlheader-enotif'      => '* Li notifiaedje pa emile est en alaedje.',
'wlheader-showupdated' => "* Les pådjes k' ont candjî dispoy vosse dierinne vizite sont metowes e '''cråssès letes'''",
'watchmethod-recent'   => "Cwerant après les pådjes k' ont stî candjeyes dierinnmint ki sont eto des pådjes shuvowes",
'watchmethod-list'     => "Cwerant après les pådjes shuvowes k' ont stî candjeyes dierinnmint",
'watchlistcontains'    => 'I gn a {{PLURAL:$1|$1 pådje|$1 pådjes}} e vosse djivêye des pådjes a shuve.',
'iteminvalidname'      => "Åk n' a nén stî avou «$1», li no n' est nén valide...",
'wlnote'               => 'Chal pa dzo les $1 dierins candjmints des <b>$2</b> dierinnès eures.',
'wlshowlast'           => 'Mostrer les dierin(nè)s $1 eures, $2 djoûs $3',

'enotif_mailer'      => 'Notifiaedje pa emile di {{SITENAME}}',
'enotif_reset'       => 'Mårker totes les pådjes come vizitêyes',
'enotif_newpagetext' => "C' est ene nouve pådje.",
'changed'            => 'candjeye',
'created'            => 'ahivêye',
'enotif_subject'     => 'Li pådje «$PAGETITLE» so {{SITENAME}} a stî $CHANGEDORCREATED pa $PAGEEDITOR',
'enotif_lastvisited' => 'Loukîz $1 po tos les candjmints dispoy vosse dierinne vizite.',
'enotif_body'        => 'Binamé $WATCHINGUSERNAME,

Li pådje «$PAGETITLE» so {{SITENAME}} a stî $CHANGEDORCREATED li $PAGEEDITDATE pa $PAGEEDITOR, loukîz $PAGETITLE_URL pol modêye do moumint.

$NEWPAGE

Comintaire do candjeu: $PAGESUMMARY $PAGEMINOREDIT

Contak do candjeu:
emile: $PAGEEDITOR_EMAIL
wiki: $PAGEEDITOR_WIKI

I n\' årè nén d\' ôtes notifiaedjes po ds ôtes candjmints di ç\' minme pådje ci tant k\' vos n\' l\' åroz nén vizitêye. Vos ploz eto rimete a noû les drapeas di notifiaedje po totes les pådjes di vosse djivêye des pådjes a shuve.


         Vosse binamé sistinme di notifiaedje so {{SITENAME}}

--
Po candjî l\' apontiaedje di vosse djivêye a shuve, loukîz
{{fullurl:{{ns:special}}:Watchlist/edit}}

Po pus d\' aidance:
{{fullurl:{{ns:help}}:Aidance}}',

# Delete/protect/revert
'deletepage'                  => "Disfacer l' pådje",
'confirm'                     => 'Acertiner',
'excontent'                   => 'li contnou esteut: «$1»',
'excontentauthor'             => "li contnou esteut: «$1» (eyet l' seu contribouweu esteut «$2»)",
'exbeforeblank'               => "li contnou dvant l' disfaçaedje esteut: «$1»",
'exblank'                     => 'li pådje esteut vude',
'historywarning'              => 'Asteme: Li pådje ki vos alez disfacer a-st ene istwere:',
'confirmdeletetext'           => "Vos alez disfacer po tofer del båze di dnêyes ene
pådje ou ene imådje, avou tote si istwere.
Acertinez s' i vs plait ki c' est bén çoula ki vos vloz fé,
ki vos comprindoz les consecwinces, et ki vos fjhoz çoula
tot [[{{MediaWiki:Policy-url}}|shuvant les rîles]].",
'actioncomplete'              => 'Fwait',
'deletedtext'                 => 'Li pådje «<nowiki>$1</nowiki>» a stî disfacêye. Loukîz li $2 po ene
djivêye des dierins disfaçaedjes.',
'deletedarticle'              => 'pådje «$1» disfacêye',
'dellogpage'                  => 'Djournå des disfaçaedjes',
'dellogpagetext'              => "Chal pa dzo c' est l' djivêye des dierins disfaçaedjes.",
'deletionlog'                 => 'djournå des disfaçaedjes',
'reverted'                    => 'Rimetou ene modêye di dvant',
'deletecomment'               => 'Råjhon do disfaçaedje',
'cantrollback'                => "Dji n' sai disfé les candjmints; li dierin contribouweu est li seu oteur po cist årtike ci.",
'alreadyrolled'               => "Dji n' sai disfé li dierin candjmint di [[$1]] fwait pa [[User:$2|$2]] ([[User talk:$2|Copene]]); 
ene sakî d' ôte a ddja candjî l' årtike ou ddja rmetou l' modêye di dvant.

Li dierin candjmint a stî fwait pa [[User:$3|$3]] ([[User talk:$3|Copene]]).",
'editcomment'                 => 'Li comintaire do candjmint esteut: «<i>$1</i>».', # only shown if there is an edit comment
'revertpage'                  => 'Disfwait li candjmint da [[Special:Contributions/$2|$2]] ([[User talk:$2|copene]]); li dierin candjmint est asteure da [[User:$1|$1]]', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'protectlogpage'              => 'Djournå des protedjaedjes',
'protectlogtext'              => "Chal pa dzo c' est ene djivêye des protedjaedjes et disprotedjaedjes des pådjes.",
'protectedarticle'            => '«[[$1]]» protedjî',
'unprotectedarticle'          => '«[[$1]]» disprotedjî',
'protect-title'               => 'Protedjant «$1»',
'protect-legend'              => "Acertinez l' protedjaedje",
'protectcomment'              => 'Råjhon po protedjî',
'protect-unchain'             => 'Disbloker les permissions di displaeçaedje',
'protect-text'                => "Vos ploz droci vey eyet candjî l' livea d' protedjaedje pol pådje <strong><nowiki>$1</nowiki></strong>.",
'protect-default'             => '(prémetou)',
'protect-level-autoconfirmed' => 'Bloker les uzeus nén eredjîstrés',
'protect-level-sysop'         => 'Seulmint les manaedjeus',
'pagesize'                    => '(octets)',

# Restrictions (nouns)
'restriction-edit' => 'Candjî',
'restriction-move' => 'Displaecî',

# Undelete
'undelete'                 => 'Rapexhî des disfacêyès pådjes',
'undeletepage'             => 'Vey et rapexhî des disfacêyès pådjes',
'viewdeletedpage'          => 'Vey les disfacêyès pådjes',
'undeletepagetext'         => 'Les pådjes shuvantes ont stî disfacêyes mins ele sont co ezès årtchives, do côp ele polèt esse rapexheyes.',
'undeleteextrahelp'        => "Po rapexhî l' pådje etire, leyîz vudes totes les boesses a clitchî eyet clitchîz sol boton «'''Rapexhî'''». Po rapexhî seulmint des modêyes k' i gn a, tchoezixhoz les cenes ki vos vloz avou les boesses a clitchî, eyet poy clitchîz sol boton «'''Rapexhî'''». Si vos clitchîz sol boton «'''Netyî'''», çoula neteyrè l' tchamp d' comintaire eyet totes les boesses a clitchî.",
'undeleterevisions'        => '$1 modêyes ezès årtchives',
'undeletehistory'          => "Si vos rapexhîz l' pådje, l' istwere del pådje
serè rapexheye eto, avou totes les modêyes co ezès årtchives.
Si ene novele pådje avou l' minme no a stî askepieye dispoy li disfaçaedje
di cisse chal, les rapexheyès modêyes seront metowes e l' istwere mins
c' est l' modêye do moumint, et nén l' cisse rapexheye, ki
srè håynêye.",
'undeletehistorynoadmin'   => "Cist årtike a stî disfacé. Li råjhon do
disfaçaedje est dnêye chal pa dzo, avou les detays des uzeus k' ont
candjî l' pådje divant do disfaçaedje. Li tecse di ces modêyes disfacêyes
ni pout esse veyou ki des manaedjeus.",
'undeletebtn'              => 'Rapexhî!',
'undeletereset'            => 'Netyî',
'undeletecomment'          => 'Comintaire:',
'undeletedarticle'         => "a rapexhî l' pådje «[[$1]]»",
'undeletedrevisions'       => '$1 modêye(s) di rapexheyes',
'undeletedrevisions-files' => '$1 modêye(s) et $2 fitchî(s) di rapexhîs',
'undeletedfiles'           => '$1 fitchî(s) di rapexhîs',
'cannotundelete'           => "Li rapexhaedje a fwait berwete; motoit bén k' ene ôte sakî l' a ddja rapexhî.",
'undeletedpage'            => "<big>'''Li pådje $1 a stî rapexheye.'''

Loukîz l' [[Special:Log/delete|djournå des disfaçaedjes]] po ene djivêye des dierins disfaçaedjes eyet rapexhaedjes.",
'undelete-search-submit'   => 'Cweri',

# Namespace form on various pages
'namespace'      => 'Espåce di lomaedje:',
'invert'         => 'Tchuze å rvier',
'blanknamespace' => '(Mwaisse)',

# Contributions
'contributions' => "Ovraedjes di l' uzeu",
'mycontris'     => 'Mi ovraedje',
'contribsub2'   => "Po l' uzeu $1 ($2)",
'nocontribs'    => "Nou candjmint di trové ki corespondreut a ç' critere la.",

'sp-contributions-submit' => 'Cweri',

# What links here
'whatlinkshere' => 'Pådjes ki loynut chal',
'linklistsub'   => '(Djivêye des loyéns)',
'linkshere'     => 'Les pådjes ki shuvèt ont des loyéns viè cisse ci:',
'nolinkshere'   => 'Nole pådje avou des loyéns viè cisse ci.',
'isredirect'    => 'pådje di redjiblaedje',

# Block/unblock
'blockip'                     => 'Bloker èn uzeu',
'blockiptext'                 => "Rimplixhoz les tchamps chal pa dzo po bloker
l' accès e scrijhaedje d' èn uzeu dné ou a pårt d' ene
adresse IP dnêye. Çouci èn doet esse fwait ki po-z arester les
vandales, et çoula doet esse fwait tot shuvant les
[[{{MediaWiki:Policy-url}}|rîles]].
Dinez ene råjhon do blocaedje (eg: dijhoz les pådjes k' ont
stî vandalijheyes).",
'ipaddress'                   => "Adresse IP/no d' uzeu",
'ipadressorusername'          => "Adresse IP ou no d' uzeu",
'ipbexpiry'                   => 'Tins do blocaedje',
'ipbreason'                   => 'Råjhon',
'ipbsubmit'                   => 'Bloker cist uzeu',
'ipbother'                    => 'Ôte termene',
'ipboptions'                  => '2 eures:2 hours,1 djoû:1 day,3 djoûs:3 days,1 samwinne:1 week,2 samwinnes:2 weeks,1 moes:1 month,3 moes:3 months,6 moes:6 months,1 anêye:1 year,po todi:infinite', # display1:time1,display2:time2,...
'ipbotheroption'              => 'ôte',
'badipaddress'                => "Nol uzeu avou ç' no la, ou adresse IP nén valide",
'blockipsuccesssub'           => 'Li blocaedje a stî comifåt',
'blockipsuccesstext'          => '«[[Special:Contributions/$1|$1]]» a stî bloké.<br />Loukîz li [[Special:IPBlockList|djivêye des blocaedjes]] po candjî on blocaedje.',
'unblockip'                   => 'Disbloker èn uzeu',
'unblockiptext'               => "Rimplixhoz les tchamps chal pa dzo po ridner accès e scrijhaedje a èn uzeu ou adresse IP k' estént blokés.",
'ipusubmit'                   => 'Disbloker ciste adresse ci',
'unblocked'                   => '«[[User:$1|$1]]» a stî disbloké',
'ipblocklist'                 => "Djivêye d' adresses IP et di nos d' uzeus ki sont blokés",
'ipblocklist-submit'          => 'Cweri',
'blocklistline'               => '$1, $2 a bloké $3 ($4)',
'infiniteblock'               => 'po todi',
'expiringblock'               => "disk' å $1",
'blocklink'                   => 'bloker',
'unblocklink'                 => 'disbloker',
'contribslink'                => 'contribouwaedjes',
'autoblocker'                 => "Bloké otomaticmint paski vos eployîz li minme adresse IP ki «[[User:$1|$1]]». Råjhon do blocaedje «'''$2'''».",
'blocklogpage'                => 'Djournå des blocaedjes',
'blocklogentry'               => '«[[$1]]» a stî bloké po ene termene di $2',
'blocklogtext'                => "Çouchal, c' est on djournå des blocaedjes eyet disblocaedjes d' uzeus. Les adresses IP blokêyes otomaticmint èn sont nén håynêyes. Loukîz li [[Special:IPBlockList|djivêye des adresses IP blokêyes]] po vey les blocaedjes d' adresses IP do moumint.",
'unblocklogentry'             => '«$1» a stî disbloké',
'range_block_disabled'        => "Li possibilité po les manaedjeus di bloker des fortchetes d' adresses IP a stî dismetowe.",
'ipb_expiry_invalid'          => 'Tins di blocaedje nén valide.',
'ip_range_invalid'            => "Fortchete d' adresses IP nén valide.",
'proxyblocker'                => 'Blocaedje di procsi',
'proxyblockreason'            => "Voste adresse IP a stî blokêye paski c' est on procsi k' est å lådje. Contactez vost ahesseu Internet ou l' siervice di sopoirt tecnike eyet lzî dire po çoula, la k' c' est on problinme di såvrité serieus.",
'proxyblocksuccess'           => 'Fwait.',
'sorbsreason'                 => "Voste adresse IP si trove dins l' djivêye des procsis å lådje di DNSBL.",
'sorbs_create_account_reason' => "Voste adresse IP si trove dins l' djivêye des procsis å lådje di DNSBL. Vos n' poloz nén ahiver on conte d' uzeu.",

# Move page
'move-page-legend'        => "Displaecî l' pådje",
'movepagetext'            => "Chal vos ploz candjî l' no d' ene pådje, dj' ô bén displaecî l' pådje, eyet si istwere, viè l' novea no.
Li vî tite divénrè-st ene pådje di redjiblaedje viè l' novele.
Les loyéns viè l' viye pådje èn seront nén candjîs; acertinez vs di verifyî s' i n' a nén des dobes ou crons redjiblaedjes.
Vos estoz responsåve di fé çou k' i fåt po k' les loyéns continouwexhe di moenner la k' i fåt.

Notez k' el pådje èn serè '''nén''' displaeceye s' i gn a ddja ene pådje avou l' novea tite, a moens k' ele soeye vude, ou ene pådje di redjiblaedje, et k' ele n' åye nole istwere.
Çoula vout dire ki vos ploz ri-displaecî ene pådje viè l' no k' ele aveut djusse divant, et insi disfé vosse prumî displaeçaedje, å cas ou vos vs rindrîz conte ki vos avoz fwait ene flotche;
ey eto ki vos n' poloz nén spotchî par accidint ene pådje k' egzistêye dedja.

'''ASTEME!'''
On displaeçaedje pout esse on consecant et nén atindou candjmint po ene pådje foirt léjhowe;
s' i vs plait tuzez bén åzès consecwinces divant d' continouwer.",
'movepagetalktext'        => "Li pådje di copene associeye serè
displaeceye otomaticmint avou, '''a moens ki:'''
*Ene pådje di copene nén vude egzistêye dedja dizo l' novea no,
*Vos disclitchrîz l' boesse a clitchî chal pa dzo.

Dins ces cas la, vos dvroz displaecî l' pådje di copene al mwin, ou rcopyî
si contnou, si vos l' vloz mete adlé l' novea no
d' l' årtike.",
'movearticle'             => 'Displaecî di',
'newtitle'                => "Viè l' novea tite",
'movepagebtn'             => 'Displaecî',
'pagemovedsub'            => 'Li displaçaedje a stî comifåt',
'articleexists'           => "Ene pådje egzistêye dedja avou ç' no la, oudonbén
li no k' vos avoz tchoezi n' est nén valide.
Tchoezixhoz è èn ôte s' i vs plait.",
'talkexists'              => "'''Li pådje leye minme a stî displaeceye comifåt, mins nén li pådje di copene, ca i gn aveut ddja ene pådje di copene k' egzistéve al novele plaece. I vs fårè copyî l' tecse del pådje di copene al mwin.'''",
'movedto'                 => 'displaecî viè',
'movetalk'                => 'Displaecî li pådje di copene avou, si ça astchait.',
'1movedto2'               => '[[$1]] displaecî viè [[$2]]',
'1movedto2_redir'         => '[[$1]] displaecî viè [[$2]] pa dsu on redjiblaedje',
'movelogpage'             => 'Djournå des displaçaedjes',
'movelogpagetext'         => "Chal pa dzo c' est ene djivêye des pådjes k' on stî displaceyes.",
'movereason'              => 'Råjhon',
'revertmove'              => 'disfé',
'delete_and_move'         => 'Disfacer et displaecî',
'delete_and_move_text'    => "==I gn a mezåjhe di disfacer==

L' årtike såme «[[:$1]]» egzistêye dedja. El voloz vs disfacer po vs permete di displaecî l' ôte?",
'delete_and_move_confirm' => "Oyi, disfacer l' pådje",
'delete_and_move_reason'  => 'Disfacé po permete on displaeçaedje',
'selfmove'                => 'Les tites sourdant et såme sont les minmes; ene pådje ni pout nén esse displaeceye so leye minme.',
'immobile_namespace'      => "Li tite såme est d' ene sôre especiåle; on n' pout nén displaecî des pådjes dins cist espåce di lomaedje la.",

# Export
'export'          => 'Ricopyî des pådjes foû',
'exporttext'      => "Vos ploz rcopyî foû l' tecse eyet l' istwere des candjmints d' ene pådje dinêye, ou co di sacwantes pådjes, eyet l' aveur dins on fitchî e cogne XML. Çoula pout adon esse ristitchî dins èn ôte wiki k' eploye MediaWiki, åd triviè del pådje di rstitchaedje (Special:Import).

Po rcopyî des pådjes foû, metoz les tites des pådjes dins l' boesse di tecse chal pa dzo, on tite pa roye, eyet tchoezixhoz si vos vloz totes les modêyes avou l' istwere, ou rén kel dierinne modêye avou fok les racsegnes sol dierin candjmint.

Dins ç' dierin cas, vos ploz eto eployî ene hårdêye, eg: [[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]] pol pådje «[[{{MediaWiki:Mainpage}}]]».",
'exportcuronly'   => "Inclure fok li modêye do moumint, nén tote l' istwere",
'exportnohistory' => "----
'''Note:''' li rcopiaedje foû di tote l' istwere des pådjes a stî dismetou cåze di problinmes di tchedje des sierveus.",
'export-submit'   => 'Ricopyî foû',
'export-addcat'   => 'Radjouter',

# Namespace 8 related
'allmessages'               => 'Tos les messaedjes ratournåves',
'allmessagesname'           => 'No del variåve',
'allmessagesdefault'        => 'Tecse prémetou',
'allmessagescurrent'        => 'Tecse pol moumint',
'allmessagestext'           => "Çouchal est ene djivêye di tos les messaedjes k' i gn a dins l' espåce di lomaedje ''MediaWiki:''",
'allmessagesnotsupportedDB' => "'''{{ns:special}}:AllMessages''' n' est nén sopoirté paski '''\$wgUseDatabaseMessages''' est dismetou.",
'allmessagesfilter'         => 'Erîlêye ratourneure pol passete:',
'allmessagesmodified'       => 'Seulmint les cis candjîs',

# Thumbnails
'thumbnail-more'  => 'Ragrandi',
'filemissing'     => 'Fitchî mancant',
'thumbnail_error' => "Åk n' a nén stî tot fjhant l' pitite imådje: $1",

# Special:Import
'import'                   => 'Ristitchî des pådjes',
'importinterwiki'          => 'Ricopiaedje eterwiki',
'import-interwiki-text'    => "Tchoezixhoz on wiki eyet on tite di pådje did wice ricopyî l' contnou a stitchî chal.
Les dates des diferinnès modêyes eyet les nos des contribouweus seront consiervés.
Totes les accions di rcopiaedje eterwiki sont metowes e [[Special:Log/import|djournå des ristitchaedjes]].",
'import-interwiki-history' => "Ristitchî avou l' istwere di totes les modêyes",
'import-interwiki-submit'  => 'Ristitchî',
'importtext'               => "S' vs plait ricopyîz l' fitchî foû do sourdant wiki avou l' usteye di rcopiaedje foû (Special:Export), el schaper so voste éndjole, et poy l' eberweter droci.",
'importstart'              => "Dj' enonde li ristitchaedje...",
'import-revision-count'    => '$1 modêye(s)',
'importnopages'            => 'Nole pådje a ristitchî.',
'importfailed'             => 'Li ristitchaedje a fwait berwete: $1',
'importunknownsource'      => 'Sourdant nén cnoxhou pol ristitchaedje',
'importcantopen'           => "Dji n' sai drovi l' fitchî a ristitchî",
'importbadinterwiki'       => 'Cron loyén eterwiki',
'importnotext'             => "Vude ou pont d' tecse",
'importsuccess'            => 'Li ristitchaedje a stî comifåt!',

# Tooltip help for the actions
'tooltip-pt-userpage'             => "Pådje d' uzeu da minne",
'tooltip-pt-anonuserpage'         => "Li pådje d' uzeu po l' adresse IP ki vos eployîz pol moumint",
'tooltip-pt-mytalk'               => 'Pådje di copene da minne',
'tooltip-pt-anontalk'             => 'Pådje di copene po les candjmints fwaits a pårti di ciste adresse IP ci',
'tooltip-pt-preferences'          => 'Mes preferinces',
'tooltip-pt-watchlist'            => 'Li djivêye des pådjes ki vos shujhoz po cwand ele sont candjeyes.',
'tooltip-pt-mycontris'            => 'Djivêye des ovraedjes da minne',
'tooltip-pt-login'                => "Vos estoz ecoraedjî d' vos elodjî, mins nerén, c' est nén oblidjî.",
'tooltip-pt-anonlogin'            => "Vos estoz ecoraedjî d' vos elodjî, mins nerén, c' est nén oblidjî.",
'tooltip-pt-logout'               => 'Vos dislodjî',
'tooltip-ca-talk'                 => 'Copene åd fwait do contnou del pådje',
'tooltip-ca-edit'                 => "Vos ploz candjî cisse pådje ci. S' i vs plait, eployîz l' boton «Vey divant» po vs acertiner k' tot est comifåt dvant d' schaper vos candjmints.",
'tooltip-ca-addsection'           => 'Radjouter on comintaire a cisse copene ci.',
'tooltip-ca-viewsource'           => "Cisse pådje ci est protedjeye. Vos ploz seulmint vey li côde sourdant, mins nén l' candjî.",
'tooltip-ca-history'              => 'Viyès modêyes del pådje.',
'tooltip-ca-protect'              => 'Protedjî cisse pådje ci',
'tooltip-ca-delete'               => 'Disfacer ci pådje ci',
'tooltip-ca-undelete'             => "Rapexhî les candjmitns fwaits al pådje divant k' ele soeyexhe disfacêye",
'tooltip-ca-move'                 => 'Displaecî cisse pådje ci',
'tooltip-ca-watch'                => 'Radjouter cisse pådje ci al djivêye di vos årtikes shuvous',
'tooltip-ca-unwatch'              => 'Bodjî cisse pådje ci di vosse djivêye des årtikes shuvous',
'tooltip-search'                  => 'Cweri so ci wiki chal',
'tooltip-p-logo'                  => 'Mwaisse pådje',
'tooltip-n-mainpage'              => 'Vizitez li Mwaisse pådje',
'tooltip-n-portal'                => "Åd fwait do pordjet, çou k' vos ploz fé, wice trover des sacwès",
'tooltip-n-currentevents'         => "Des infôrmåcions so des evenmints d' actouwålité",
'tooltip-n-recentchanges'         => "Li djivêye des dierins candjmints k' i gn a-st avou sol wiki.",
'tooltip-n-randompage'            => "Tcherdjî ene pådje a l' astcheyance",
'tooltip-n-help'                  => "Li plaece po trover les responses a vos kesses so l' eployaedje do wiki.",
'tooltip-t-whatlinkshere'         => "Djivêye di totes les pådjes k' ont des loyéns viè cisse pådje ci",
'tooltip-t-recentchangeslinked'   => 'Dierins candjmints fwaits so des pådjes ki cisse pådje ci a des loyéns viè zeles',
'tooltip-feed-rss'                => 'Sindicåcion RSS po cisse pådje ci',
'tooltip-feed-atom'               => 'Sindicåcion Atom po cisse pådje ci',
'tooltip-t-contributions'         => 'Vey li djivêye des ovraedjes fwait pa cist uzeu ci',
'tooltip-t-emailuser'             => 'Evoyî èn emile a cist uzeu ci',
'tooltip-t-upload'                => 'Eberweter sol sierveu des imådjes ou fitchîs media',
'tooltip-t-specialpages'          => 'Djivêye di totes les pådjes sipeciåles',
'tooltip-ca-nstab-main'           => 'Vey li pådje di contnou',
'tooltip-ca-nstab-user'           => "Vey li pådje di l' uzeu",
'tooltip-ca-nstab-media'          => 'Vey li pådje di media',
'tooltip-ca-nstab-special'        => "Çouchal, c' est ene pådje sipeciåle, vos n' poloz nén candjî l' pådje leyminme.",
'tooltip-ca-nstab-project'        => 'Vey li pådje di pordjet',
'tooltip-ca-nstab-image'          => "Vey li pådje d' imådje",
'tooltip-ca-nstab-mediawiki'      => 'Vey li messaedje ratournåve do sistinme',
'tooltip-ca-nstab-template'       => 'Vey li modele',
'tooltip-ca-nstab-help'           => "Vey li pådje d' aidance",
'tooltip-ca-nstab-category'       => 'Vey li pådje di categoreye',
'tooltip-minoredit'               => 'Mete çouci come on candjmint mineur [alt-i]',
'tooltip-save'                    => 'Schaper vos candjmints [alt-s]',
'tooltip-preview'                 => "Prévey vos candjmints, fijhoz l' divant d' schaper s' i vs plait! [alt-p]",
'tooltip-diff'                    => 'Mostrer les candjmints ki vos avoz fwait e tecse. [alt-v]',
'tooltip-compareselectedversions' => 'Mostrer les diferinces etur les deus modêyes tchoezeyes di cisse pådje ci. [alt-v]',
'tooltip-watch'                   => 'Radjouter cisse pådje ci a vosse djivêye des shuvous [alt-w]',
'tooltip-recreate'                => "Rifé cisse pådje ci mågré k' ele åye sitî disfacêye",

# Stylesheets
'common.css'   => '/* li côde CSS metou chal serè eployî pa totes les peas et tos les uzeus */',
'monobook.css' => "/* li côde CSS metou chal serè eployî pa tos les uzeus eployant l' pea «monobook» */",

# Metadata
'notacceptable' => 'Li sierveu wiki èn vos pout nén dner les dnêyes dins ene cogne ki vosse cliyint sait lére.',

# Attribution
'anonymous'        => 'Uzeu(s) anonime(s) di {{SITENAME}}',
'siteuser'         => "Uzeu d' {{SITENAME}} «$1»",
'lastmodifiedatby' => 'Cisse pådje a stî candjeye pol dierin côp li $2, $1 pa $3.', # $1 date, $2 time, $3 user
'othercontribs'    => "Båzé so l' ovraedje da $1.",
'others'           => 'des ôtes',
'siteusers'        => "Uzeu(s) d' {{SITENAME}} «$1»",
'creditspage'      => 'Pådje di credits',
'nocredits'        => "I n' a pont d' infôrmåcion di credits po cisse pådje ci.",

# Info page
'infosubtitle'   => 'Infôrmåcions pol pådje',
'numedits'       => 'Nombe di candjmints (årtike): $1',
'numtalkedits'   => 'Nombe di candjmints (pådje di copene): $1',
'numwatchers'    => 'Nombe di shuveus: $1',
'numauthors'     => "Nombe d' oteurs diferins (årtike): $1",
'numtalkauthors' => "Nombe d' oteurs diferins (pådje di copene): $1",

# Math options
'mw_math_png'    => 'Håyner tofer come ene imådje PNG',
'mw_math_simple' => "Håyner en HTML si c' est foirt simpe, ôtmint e PNG",
'mw_math_html'   => "Håyner en HTML si c' est possibe, ôtmint e PNG",
'mw_math_source' => 'El leyî e TeX (po les betchteus e môde tecse)',
'mw_math_modern' => 'Ricmandé po les betchteus modienes',
'mw_math_mathml' => "Eployî MathML si c' est possibe (esperimintå)",

# Image deletion
'deletedrevision' => 'Viye modêye $1 disfacêye',

# Browsing diffs
'previousdiff' => '← Diferinces des candjmints di dvant',
'nextdiff'     => 'Diferinces des candjmints shuvants →',

# Media information
'mediawarning' => "'''Asteme''': Ci fitchî chal pôreut esse evirussé, si vos l' enondez vos pôrîz infecter l' sistinme da vosse.<hr />",
'imagemaxsize' => "Limite pol håynaedje ezès pådjes d' imådje:",
'thumbsize'    => 'Grandeu po les imådjetes (thumb):',

# Special:NewImages
'newimages'             => 'Galreye des nouvès imådjes',
'imagelisttext'         => "Chal pa dzo c' est ene djivêye di '''$1''' {{PLURAL:$1|imådje relîte|imådjes relîtes}} $2.",
'showhidebots'          => '($1 robots)',
'noimages'              => "I n' a rén a vey.",
'ilsubmit'              => 'Cweri',
'bydate'                => 'pazès dates',
'sp-newimages-showfrom' => 'Mostrer les nouvès imådjes a pårti do $1',

# Metadata
'metadata'          => 'Meta-dnêyes',
'metadata-help'     => "Ci fitchî chal a des infôrmåcions di rawete, motoit bén radjoutêyes pa l' aparey foto limerike ou l' sicanrece eployeye po fé l' imådje. Si l' imådje a stî candjeye dispoy adon, i s' pout ki sacwants detays ni corespondexhe pus totafwait.",
'metadata-expand'   => 'Mostrer les stindous detays',
'metadata-collapse' => 'Catchî les stindous detays',
'metadata-fields'   => "Les tchamps di meta-dnêyes EXIF metous chal vont esse
håynés ezès pådjes d' imådje cwand l' tåvlea di meta-dnêyes
est raptiti. Les ôtes seront catchîs.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength", # Do not translate list items

# EXIF tags
'exif-imagewidth'                => 'Lårdjeur',
'exif-imagelength'               => 'Hôteur',
'exif-bitspersample'             => 'Bits pa compôzant',
'exif-photometricinterpretation' => 'Compôzaedje des picsels',
'exif-orientation'               => 'Oryintåcion',
'exif-samplesperpixel'           => 'Nombe di compôzants',
'exif-xresolution'               => "Finté d' coûtchî",
'exif-yresolution'               => "Finté d' astampé",
'exif-resolutionunit'            => "Unité pol finté d' coûtchî/astampé",
'exif-datetime'                  => "Date ey eure ki l' fitchî a stî candjî",
'exif-imagedescription'          => "Tite di l' imådje",
'exif-make'                      => 'Måke del camera',
'exif-model'                     => 'Modele del camera',
'exif-software'                  => 'Programe eployî',
'exif-artist'                    => 'Oteur',
'exif-copyright'                 => 'Ditinteu des abondroets',
'exif-exifversion'               => "Modêye d' exif",
'exif-colorspace'                => 'Espåce di coleurs',
'exif-makernote'                 => 'Notes do fabricant',
'exif-usercomment'               => "Comintaires di l' uzeu",
'exif-datetimeoriginal'          => 'Date ey eure ki les dnêyes ont stî fwaites',
'exif-datetimedigitized'         => 'Date ey eure do scanaedje',
'exif-exposuretime-format'       => '$1 seg ($2)',
'exif-lightsource'               => 'Sourdant del loumire',
'exif-filesource'                => 'Fitchî sourdant',
'exif-scenetype'                 => 'Sôre di sinne',
'exif-whitebalance'              => 'Balance di blancs',
'exif-digitalzoomratio'          => 'Rapoirt di zoumaedje limerike',
'exif-contrast'                  => 'Contrasse',
'exif-saturation'                => 'Saturaedje',
'exif-gpslatituderef'            => 'Latitude Nôr ou Sud',
'exif-gpslongituderef'           => 'Londjitude Ess ou Ouwess',
'exif-gpslongitude'              => 'Londjitude',
'exif-gpsaltituderef'            => 'Referince di hôteur',
'exif-gpsaltitude'               => 'Hôteur',
'exif-gpstimestamp'              => 'Tins do GPS (ôrlodje atomike)',
'exif-gpssatellites'             => 'Sipoutniks eployîs pol mezuraedje',
'exif-gpsmeasuremode'            => 'Môde di mzuraedje',
'exif-gpsdop'                    => 'Precizion di mzuraedje',
'exif-gpsareainformation'        => 'No del redjon GPS',
'exif-gpsdatestamp'              => 'Date do GPS',
'exif-gpsdifferential'           => 'Coridjaedje diferenciel do GPS',

'exif-orientation-1' => 'Normå', # 0th row: top; 0th column: left
'exif-orientation-3' => 'Tourné di 180°', # 0th row: bottom; 0th column: right

'exif-componentsconfiguration-0' => "n' egzistêye nén",

'exif-exposureprogram-0' => 'Nén defini',
'exif-exposureprogram-1' => 'Al mwin',
'exif-exposureprogram-2' => 'Programaedje normå',

'exif-subjectdistance-value' => '$1 metes',

'exif-meteringmode-0'   => 'Nén cnoxhou',
'exif-meteringmode-1'   => 'Moyene',
'exif-meteringmode-255' => 'Ôte',

'exif-lightsource-0'   => 'Nén cnoxhou',
'exif-lightsource-1'   => 'Loumire do djoû',
'exif-lightsource-9'   => 'Bon tins',
'exif-lightsource-10'  => 'Tins avou des nûlêyes',
'exif-lightsource-17'  => 'Loumire standård A',
'exif-lightsource-18'  => 'Loumire standård B',
'exif-lightsource-19'  => 'Loumire standård C',
'exif-lightsource-255' => "Ôte sourdant d' loumire",

'exif-focalplaneresolutionunit-2' => 'pôces',

'exif-sensingmethod-1' => 'Nén defineye',

'exif-scenetype-1' => 'On poitrait saetchî directumint',

'exif-whitebalance-0' => 'Balance di blancs otomatike',
'exif-whitebalance-1' => 'Balance di blancs al mwin',

'exif-scenecapturetype-3' => 'Sinne di nute',

'exif-contrast-0' => 'Normå',
'exif-contrast-1' => 'Doûs',
'exif-contrast-2' => 'Deur',

'exif-saturation-0' => 'Normå',
'exif-saturation-1' => 'Fwebe saturaedje',
'exif-saturation-2' => 'Foirt saturaedje',

'exif-sharpness-0' => 'Normåle',
'exif-sharpness-1' => 'Doûce',
'exif-sharpness-2' => 'Deure',

'exif-subjectdistancerange-0' => 'Nén cnoxhowe',
'exif-subjectdistancerange-2' => 'Did près',
'exif-subjectdistancerange-3' => 'Did lon',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'Latitude Nôr',
'exif-gpslatitude-s' => 'Latitude Sud',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'Londjitude Ess',
'exif-gpslongitude-w' => 'Londjitude Ouwess',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'km/h',
'exif-gpsspeed-m' => 'miles/h',
'exif-gpsspeed-n' => 'nuks',

# External editor support
'edit-externally'      => "Candjî ç' fitchî ci avou on dfoûtrin programe",
'edit-externally-help' => "Loukîz les [http://www.mediawiki.org/wiki/Manual:External_editors instruccions d' apontiaedje] po pus di racsegnes.",

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'totafwait',
'imagelistall'     => 'totafwait',
'watchlistall2'    => 'totafwait',
'namespacesall'    => 'tos',
'monthsall'        => 'tos',

# E-mail address confirmation
'confirmemail'            => "Acertinaedje di l' adresse emile",
'confirmemail_text'       => "Ci wiki chal a mezåjhe ki vos acertinîz voste adresse emile
divant d' poleur eployî les fonccions d' emilaedje. Clitchîz sol boton
chal pa dzo po-z evoyî èn emile d' acertinaedje a voste adresse.
Li messaedje a-st å dvins ene hårdêye (loyén) avou on côde;
tcherdjîz l' hårdêye dins l' betchteu waibe da vosse, eyet
acertinez ki l' adresse emile est djusse tot dnant l' côde.",
'confirmemail_send'       => "Emiler on côde d' acertinaedje",
'confirmemail_sent'       => "L' emile d' acertinaedje a stî evoyî.",
'confirmemail_sendfailed' => "Dji n' a savou evoyî l' emile d' acertinaedje. Verifyîz ki l' adresse est bén djusse.",
'confirmemail_invalid'    => "Côde d' acertinaedje nén valide. Motoit k' il esteut trop vî.",
'confirmemail_needlogin'  => 'I vs fåt $1 po pleur acertiner voste adresse emile.',
'confirmemail_success'    => 'Voste adresse emile a stî acertinêye. Vos vs poloz asteure elodjî eyet profiter do wiki.',
'confirmemail_loggedin'   => 'Voste adresse emile a stî acertinêye.',
'confirmemail_subject'    => "Acertinaedje di l' adresse emile po {{SITENAME}}",
'confirmemail_body'       => "Ene sakî, probåblumint vos-minme, avou l' adresse IP $1,
a-st ahivé on conte so {{SITENAME}} avou ciste adresse
emile ci eyet come no d' elodjaedje «$2».

Po-z acertiner ki ç' conte ci est bén da vosse eyet mete
en alaedje les fonccions d' emilaedje so {{SITENAME}},
alez drovî avou vosse betchteu waibe li hårdêye ki shût:

$3

Si c' est *nén* vos k' a-st ahivé l' conte, adon èn shuvoz
nén l' hårdêye. Ci côde d' acertinaedje ci va-st espirer
po l' $4.",

# Scary transclusion
'scarytranscludetoolong' => '[Li hårdêye est pår trop longue]',

# Delete conflict
'deletedwhileediting' => 'Asteme: Cisse pådje ci a stî disfacêye sol tins ki vos scrijhîz!',
'confirmrecreate'     => "L' uzeu [[User:$1|$1]] ([[User talk:$1|copene]]) a disfacé cisse pådje ci après ki vos avoz cmincî a scrire, li råjhon k' il a dné c' est:
: ''$2''.
Acertinez s' i vs plait ki vos vloz vormint rifé cisse pådje ci.",
'recreate'            => 'Rifé',

# HTML dump
'redirectingto' => 'Redjiblant viè [[:$1]]...',

# action=purge
'confirm_purge'        => "Netyî l' muchete di cisse pådje ci?

$1",
'confirm_purge_button' => "'l est bon",

# AJAX search
'searchcontaining' => "Cweri après des årtikes k' ont «''$1''» å dvins.",
'searchnamed'      => "Cweri après des årtikes lomés «''$1''».",
'articletitles'    => "Årtikes ki cmincèt avou «''$1''»",
'hideresults'      => 'Catchî les rzultats',

# Multipage image navigation
'imgmultipageprev' => '← pådje di dvant',
'imgmultipagenext' => 'pådje shuvante →',

# Table pager
'table_pager_next'  => 'Pådje shuvante',
'table_pager_prev'  => 'Pådje di dvant',
'table_pager_first' => 'Prumire pådje',
'table_pager_last'  => 'Dierinne pådje',

# Auto-summaries
'autoredircomment' => 'Redjiblaedje viè [[$1]]',
'autosumm-new'     => 'Novele pådje: $1',

# Size units
'size-bytes'     => '$1 o',
'size-kilobytes' => '$1 Ko',
'size-megabytes' => '$1 Mo',
'size-gigabytes' => '$1 Go',

# Watchlist editor
'watchlistedit-raw-titles' => 'Tites:',

# Special:Version
'version' => 'Modêye des programes', # Not used as normal message but as header for the special page itself

# Special:SpecialPages
'specialpages' => 'Pådjes sipeciåles',

);
