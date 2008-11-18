<?php
/** Romani (Romani)
 *
 * @ingroup Language
 * @file
 *
 * @author Desiphral
 * @author Niklas Laxström
 * @author לערי ריינהארט
 */

$fallback = 'ro';

$namespaceNames = array(
	NS_MEDIA          => 'Mediya',
	NS_SPECIAL        => 'Uzalutno',
	NS_MAIN           => '',
	NS_TALK           => 'Vakyarimata',
	NS_USER           => 'Jeno',
	NS_USER_TALK      => 'Jeno_vakyarimata',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK   => '{{grammar:genitive-pl|$1}}_vakyarimata',
	NS_IMAGE          => 'Chitro',
	NS_IMAGE_TALK     => 'Chitro_vakyarimata',
	NS_MEDIAWIKI      => 'MediyaViki',
	NS_MEDIAWIKI_TALK => 'MediyaViki_vakyarimata',
	NS_TEMPLATE       => 'Sikavno',
	NS_TEMPLATE_TALK  => 'Sikavno_vakyarimata',
	NS_HELP           => 'Zhutipen',
	NS_HELP_TALK      => 'Zhutipen_vakyarimata',
	NS_CATEGORY       => 'Shopni',
	NS_CATEGORY_TALK  => 'Shopni_vakyarimata'
);

$messages = array(
'underline-always'  => 'Savaxt',
'underline-never'   => 'Ni ekhvar',
'underline-default' => 'Browseresko standardo',

# Dates
'sunday'    => 'purano kurko',
'monday'    => 'lui',
'tuesday'   => 'marci',
'wednesday' => 'tetradī',
'thursday'  => 'zhoi',
'friday'    => 'parashtui',
'saturday'  => 'savato',
'january'   => 'pervonai',
'february'  => 'duitonai',
'march'     => 'tritonai',
'april'     => 'shtartonai',
'may_long'  => 'panjtonai',
'june'      => 'shovtonai',
'july'      => 'eftatonai',
'august'    => 'oxtotonai',
'september' => 'enyatonai',
'october'   => 'deshtonai',
'november'  => 'deshuekhtonai',
'december'  => 'deshuduitonai',
'jan'       => 'perv',
'feb'       => 'dui',
'mar'       => 'tri',
'apr'       => 'shta',
'may'       => 'panj',
'jun'       => 'shov',
'jul'       => 'efta',
'aug'       => 'oxt',
'sep'       => 'enya',
'oct'       => 'desh',
'nov'       => 'dekh',
'dec'       => 'ddui',

# Categories related messages
'subcategories' => 'Telekategoriye',

'about'          => 'Andar',
'article'        => 'Lekh',
'newwindow'      => '(inklel aver filiyastra)',
'cancel'         => 'Mekh la',
'qbedit'         => 'Editisar',
'qbpageinfo'     => 'Patrinyake janglimata',
'qbspecialpages' => 'Uzalutne patrya',
'mypage'         => 'Miri patrin',
'mytalk'         => 'Mire vakyarimata',
'navigation'     => 'Phiripen',
'and'            => 'thai',

'errorpagetitle'   => 'Dosh',
'returnto'         => 'Ja palpale kai $1.',
'help'             => 'Zhutipen',
'search'           => 'Rod',
'searchbutton'     => 'Rod',
'go'               => 'Ja',
'searcharticle'    => 'Ja',
'history'          => 'Puraneder versiye',
'history_short'    => 'Puranipen',
'printableversion' => 'Printisaripnaski versiya',
'permalink'        => 'Savaxtutno phandipen',
'print'            => 'Printisaripen',
'edit'             => 'Editisar i patrin',
'editthispage'     => 'Editisar i patrin',
'delete'           => 'Khosipen',
'deletethispage'   => 'Khos i patrin',
'undelete_short'   => 'Na mai khos le editisarimata $1',
'protect'          => 'Brakhipen',
'unprotect'        => 'Na mai brakh',
'newpage'          => 'Nevi patrin',
'specialpage'      => 'Uzalutni patrin',
'personaltools'    => 'Mire labne',
'articlepage'      => 'Dikh o lekh',
'talk'             => 'Vakyarimata',
'toolbox'          => 'Labnengo moxton',
'userpage'         => 'Dikh i jeneski patrin',
'viewtalkpage'     => 'Dikh i diskucia',
'otherlanguages'   => 'Avre ćhibande',
'lastmodifiedat'   => 'O palutno paruvipen $2, $1.', # $1 date, $2 time
'viewcount'        => 'Kadaya patrin dikhlilyas {{PLURAL:$1|one time|$1var}}.',
'jumpto'           => 'Ja kai:',
'jumptonavigation' => 'phiripen',
'jumptosearch'     => 'rodipen',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'Andar {{SITENAME}}',
'aboutpage'            => 'Project:Andar',
'copyright'            => 'Ander dino tar o $1.',
'copyrightpage'        => '{{ns:project}}:Autorenge xakaya (chachimata)',
'currentevents'        => 'Nevimata',
'currentevents-url'    => 'Project:Nevimata',
'disclaimers'          => 'Termenurya',
'disclaimerpage'       => 'Project:Termenurya',
'edithelp'             => 'Editisaripnasko zhutipen',
'edithelppage'         => 'Help:Sar te editisares ek patrin',
'helppage'             => 'Help:Zhutipen',
'mainpage'             => 'Sherutni patrin',
'mainpage-description' => 'Sherutni patrin',
'portal'               => 'Maladipnasko than',
'portal-url'           => 'Project:Maladipnasko than',
'privacy'              => 'Pativyako forovipen',

'retrievedfrom'   => 'Lino katar "$1"',
'editsection'     => 'editisar',
'editsectionhint' => 'Editisar o kotor: $1',
'toc'             => 'Ander',
'showtoc'         => 'dikh',
'hidetoc'         => 'garav',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Lekh',
'nstab-user'      => 'Jeneski patrin',
'nstab-media'     => 'Mediya patrin',
'nstab-special'   => 'Uzalutno',
'nstab-project'   => 'Projekto',
'nstab-image'     => 'Chitro',
'nstab-mediawiki' => 'Duma',
'nstab-template'  => 'Sikavno',
'nstab-help'      => 'Zhutipen',
'nstab-category'  => 'Shopni',

# Main script and global functions
'nospecialpagetext' => 'Manglyas ekh [[Special:SpecialPages|uzalutni patrin]] so na arakhel pes kai {{SITENAME}}.',

# General errors
'wrong_wfQuery_params' => 'Doshalo gin le parametrengo ko wfQuery()<br />I function: $1<br />Query: $2',
'viewsource'           => 'Dikh i sursa',

# Login and logout pages
'logouttitle'                => 'San avri akana',
'logouttext'                 => 'Akana san avryal i {{SITENAME}}. Shai te labyares {{SITENAME}} sar ekh bijanglo jeno vai shai te prinjares tut palem sar o jeno le kadale navesa vai le aver navesa.',
'welcomecreation'            => '== Mishto avilyan, $1! ==

Akana si tuke ekh akont. Te na bistares te paruves, kana trebul tuke, tire kamimata kai {{SITENAME}}.',
'loginpagetitle'             => 'Jenesko prinjaripen',
'yourname'                   => 'Tiro anav',
'yourpassword'               => 'O nakhavipnasko lav',
'yourpasswordagain'          => 'O nakhavipnasko lav de nevo',
'externaldberror'            => 'Sas ekh dosh kai datengi baza le avrutne prinjaripnyange vai nai tuke drom te akanutnisares o avrutno akonto.',
'loginproblem'               => '<b>Sas ek problem kai tiro prinjaripen</b><br />Ker les de nevo!',
'login'                      => 'Prinjaripen',
'loginprompt'                => "Trebul te das drom le phandimatenge ''cookie'' te das andre kai {{SITENAME}}.",
'userlogin'                  => 'Prinjaripen / Ker ek akount',
'logout'                     => 'De avri',
'userlogout'                 => 'De avri',
'nologinlink'                => 'Ker ek akount',
'createaccount'              => 'Ker ek nevo akount',
'gotaccount'                 => 'Si tuke akana ekh akonto? $1.',
'gotaccountlink'             => 'De andre',
'createaccountmail'          => 'palal o e-mail',
'badretype'                  => 'Le nakhavipnaske lava so lekhavdyan nai myazutne.',
'youremail'                  => 'Emailesko adress (kana kames)*',
'yourrealname'               => 'Tiro chacho anav*',
'yourlanguage'               => 'Ćhib:',
'yournick'                   => 'I xarni versyunya, le semnaturenge',
'badsig'                     => 'Bilachhi semnatura; dikh le tagurya HTML.',
'loginerror'                 => 'Prinjaripnaski dosh',
'nocookiesnew'               => "O tiro akont sas kerdo, pale tu nai prinjardo/i. {{SITENAME}} labyarel ''cookies'' te astarel le manusha prinjarde. O tiro browser na astarel le cookies. Si mishto te das les drom te astarel le ''cookies'' thai, palal kodya, te zumaves vi ekh var, labyarindoi o nav thai o nakhavipnaso lav.",
'nocookieslogin'             => "{{SITENAME}} labyarel ''cookies'' te prinjaren le manusha so aven kathe. O tiro browser chi astarel len. Si mishto te das les drom te astarel le ''cookies'' thai, palal kodya, te zumaves vi ekh var.",
'loginsuccesstitle'          => 'Prinjaripen kerdo',
'loginsuccess'               => 'Akana san prijardo kai {{SITENAME}} sar "$1".',
'wrongpassword'              => 'O nakhavipnasko lav so thovdyan si doshalo. Mangas tuke te zumaves vi ekvar.',
'mailmypassword'             => 'Bichhal ma o nakhavipnasko lav e-mail-estar!',
'passwordremindertitle'      => 'Astaripen le tire nakhavipnaske lavesko kai {{SITENAME}}',
'passwordremindertext'       => 'Varekon (shai te aves tu, katar i adresa $1)
manglyas ek nevo nakahvipnasko lav katar {{SITENAME}}.
O nakhavipnasko lav le jenesko "$2" akana si "$3".
Mishto si te jas kai {{SITENAME}} thai te paruves tiro lav sigo.',
'noemail'                    => 'Nai ni ekh adresa e-mail prinjarde le jeneske "$1".',
'eauthentsent'               => 'Ekh prinjaripnasko e-mail bichhaldo kai tiri e-maileski adresa. Kashte avel tuke e-mailuya le avre jenendar trebul te prinjares tiri adresa (dikh buteder ando bichhaldo e-mail).',
'mailerror'                  => 'Dosh kana sas bichhaldo o e-mail: $1',
'acct_creation_throttle_hit' => 'Fal ame nasul, akana si tut $1 akounturya. Nashti te keres aver.',
'emailauthenticated'         => 'Tiro e-mail sas prinjardo kai $1.',
'emailnotauthenticated'      => 'Tiri e-maileski adresa <strong>nas prinjardi ji akana</strong>. Ni ekh e-mail nashti te avel tuke ji kana prinjares la.',
'noemailprefs'               => 'Thov ekh adresa e-mail te keren buti le kadale labne.',
'emailconfirmlink'           => 'Prinjar o e-mail',
'invalidemailaddress'        => 'Le e-maileski adresa nas lino anda kodoya ke nas lake ekh lachhi forma. Si mishto te thos ekh e-mail le lachhe formasa vai te khoses so lekhvdyas pe kodo than.',
'accountcreated'             => 'Akount kerdo',
'accountcreatedtext'         => 'Kerdo o akonto le jenesko ko $1.',

# Edit page toolbar
'image_sample' => 'Misal.jpg',

# Edit pages
'summary'            => 'Xarno xalyaripen',
'minoredit'          => 'Kadava si ek tikno editisarimos',
'watchthis'          => 'Dikh kadaya patrin',
'savearticle'        => 'Uxtav i patrin',
'showpreview'        => 'Dikh sar avelas i patrin',
'showlivepreview'    => 'Jivutno angledikhipen',
'showdiff'           => 'Dikh le paruvimata',
'whitelistedittitle' => 'Trebul o autentifikaripen kashte editisares',
'whitelistedittext'  => 'Trebul te [[Special:UserLogin|autentifikisares]] kashte editisares artikolurya.',
'accmailtitle'       => 'O nakhavipnasko lav bićhaldo.',
'accmailtext'        => "O nakhavipnasko lav andar '$1' bićhaldo ko $2.",
'newarticle'         => '(Nevo)',
'newarticletext'     => 'Avilyan kai ek patrin so na si.
Te keres la, shai te shirdes (astares) te lekhaves ando telutno moxton (dikh [[{{MediaWiki:Helppage}}|zhutipnaski patrin]] te janes buteder).
Kana avilyan kathe doshatar, ja palpale.',
'noarticletext'      => "Andi '''{{SITENAME}}''' nai ji akana ek lekh kadale anavesa.
* Te shirdes (astares) te keres o lekh, ker klik  '''[{{fullurl:{{FULLPAGENAME}}|action=edit}} kathe]'''.",
'editing'            => 'Editisaripen $1',
'yourtext'           => 'Tiro teksto',
'storedversion'      => 'Akanutni versiya',
'yourdiff'           => 'Ververimata',

# History pages
'revnotfoundtext'  => 'I puraneder versiya la patrinyaki so tu manglyan na arakhel pes. Mangas tuke te palemdikhes o phandipen so labyardyan kana avilyan kathe.',
'previousrevision' => '← Purano paruvipen',
'nextrevision'     => 'Nevi paruvipen →',
'cur'              => 'akanutni',
'last'             => 'purani',
'histlegend'       => 'Xalyaripen: (akanutni) = ververimata mamui i akanutni versiya,
(purani) = ververimata mamui i puraneder versiya, T = tikno editisaripen',
'deletedrev'       => '[khoslo]',
'histfirst'        => 'O mai purano',
'histlast'         => 'O mai nevo',

# Revision deletion
'revdelete-submit' => 'Ker kadya le alosarde paruvimatenge',

# Diffs
'compareselectedversions' => 'Dikh ververimata mashkar alosarde versiye',

# Search results
'prevn'             => 'mai neve $1',
'nextn'             => 'mai purane $1',
'viewprevnext'      => 'Dikh ($1) ($2) ($3).',
'showingresults'    => 'Tele si <b>$1</b> rezultaturya shirdindoi le ginestar <b>$2</b>.',
'showingresultsnum' => 'Tele si <b>$3</b> rezultaturya shirdindoi le ginestar <b>$2</b>.',
'powersearch'       => 'Rod',

# Preferences page
'preferences'           => 'Kamimata',
'changepassword'        => 'Paruv o nakhavipnasko lav',
'skin'                  => 'Dikhimos',
'math'                  => 'Matematika',
'dateformat'            => 'Datengi forma',
'datedefault'           => 'Ni ekh kamipen',
'datetime'              => 'Dives thai chaso',
'math_unknown_error'    => 'bijangli dosh',
'math_unknown_function' => 'bijangli funkciya',
'math_syntax_error'     => 'sintaksaki dosh',
'math_bad_output'       => 'Nashti te kerel pes vai te lekhavel po matematikano direktoro kai del pes avri.',
'math_notexvc'          => 'Nai o kerditori (eksekutabilo) texvc; dikh math/README te labyares les.',
'prefs-rc'              => 'Neve paruvimata',
'saveprefs'             => 'Uxtav le kamimata',
'resetprefs'            => 'Thov le kamimata sar ko shirdipen',
'oldpassword'           => 'Purano nakahvipnasko lav',
'newpassword'           => 'Nevo nakhavipnasko lav:',
'columns'               => 'Uche vortorina:',
'contextlines'          => 'Vortorinyango gin pe avimos:',
'contextchars'          => 'Grafemengo gin pe ekh vortorin:',
'localtime'             => 'Thanutno vaxt',
'timezoneoffset'        => 'Ververipen',
'guesstimezone'         => 'Le les katar o browser',
'allowemail'            => 'De drom te aven e-mailurya katar aver jene',
'defaultns'             => 'Rod savaxt vi kai kadale riga:',
'default'               => 'acharuno',
'files'                 => 'Failurya',

# User rights
'editinguser' => "Editisaripen '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",

# Groups
'group'            => 'Grupo:',
'group-bot'        => 'Boturya',
'group-sysop'      => 'Administratorurya',
'group-bureaucrat' => 'Birokraturya:',
'group-all'        => '(sa)',

'group-sysop-member'      => 'Administratoro',
'group-bureaucrat-member' => 'Birokrato',

'grouppage-bot'        => '{{ns:project}}:Boturya',
'grouppage-sysop'      => '{{ns:project}}:Administratorurya',
'grouppage-bureaucrat' => '{{ns:project}}:Birokraturya',

# Recent changes
'recentchanges'                     => 'Neve paruvimata',
'recentchangestext'                 => 'Andi kadaya patrin shai te dikhes le neve paruvimata andi romani {{SITENAME}}.',
'rcnote'                            => 'Tele si le palutne <strong>$1</strong> paruvimata andar le palutne <strong>$2</strong> divesa.',
'rcnotefrom'                        => "Tele si le averutnimata katar '''$2''' (inklen '''$1''' averutnimata, shai te paruves o gin alosarindoi aver tele).",
'rclistfrom'                        => 'Dikh le paruvimata ji kai $1',
'rcshowhideminor'                   => '$1 tikne editisaripena',
'rcshowhidebots'                    => '$1 (ro)boturya',
'rcshowhideliu'                     => '$1 prinjarde jene',
'rcshowhideanons'                   => '$1 bijangle jene',
'rcshowhidepatr'                    => '$1 dikhle paruvimata',
'rcshowhidemine'                    => '$1 mire editisaripena',
'rclinks'                           => 'Dikh le palutne $1 paruvimata andar le palutne $2 divesa.<br />$3',
'diff'                              => 'ververipen',
'hist'                              => 'puranipen',
'hide'                              => 'garav',
'show'                              => 'dikh',
'minoreditletter'                   => 't',
'number_of_watching_users_pageview' => '[$1 jeno/e kon len vurma e patrinyaki]',
'rc_categories'                     => 'Numa le shopnya (rigyarde katar "|")',
'rc_categories_any'                 => 'Savegodi',

# Recent changes linked
'recentchangeslinked' => 'Pashvipnaske paruvimata',

# Upload
'upload'      => 'Bichhal file',
'uploadbtn'   => 'Bichhal file',
'reupload'    => 'Pale bichhal',
'filedesc'    => 'Xarno xalyaripen',
'badfilename' => 'O chitrosko anav sas paruvdo; o nevo anav si "$1".',
'savefile'    => 'Uxtav file',

# Special:ImageList
'imagelist' => 'Patrinipen le chitrengo',

# Image description page
'imagelinks' => 'Chitroske phandimata',

# Unused templates
'unusedtemplates'    => 'Bilabyarde sikavne',
'unusedtemplateswlh' => 'aver phandimata',

# Random page
'randompage' => 'Ekh patrin savigodi',

# Statistics
'statistics'    => 'Beshimata',
'sitestats'     => 'Site-ske beshimata',
'userstatstext' => 'Si <b>$1</b> jene rejistrime (lekhavde).
Mashkar lende <b>$2</b> si administratorurya (dikh $3).',

# Miscellaneous special pages
'wantedpages'  => 'Kamle pajine',
'shortpages'   => 'Xarne patrya',
'deadendpages' => 'Biphandimatenge patrya',
'listusers'    => 'Jenengo patrinipen',
'newpages'     => 'Neve patrya',
'ancientpages' => 'E puraneder lekha',
'move'         => 'Ingerdipen',

# Special:AllPages
'allpages'       => 'Savore patrya',
'nextpage'       => 'Anglutni patrin ($1)',
'allarticles'    => 'Sa le artikolurya',
'allpagessubmit' => 'Ja',

# E-mail user
'emailuser' => 'Bichhal les/la e-mail',
'emailfrom' => 'Katar',
'emailto'   => 'Karing',
'emailsend' => 'Bichhal',

# Watchlist
'watchlist'        => 'Dikhipnaske lekha',
'mywatchlist'      => 'Dikhipnaske lekha',
'addedwatch'       => 'Thovdi ando patrinipen le patrinyange so arakhav len',
'addedwatchtext'   => 'I patrin "[[:$1]]" sas thovdi andi tiri lista [[Special:Watchlist|le artikolengi so dikhes len]].
Le neve paruvimata andar kadale patrya thai andar lenge vakyarimatenge patrya thona kathe, vi dikhena pen le <b>thule semnurenca</b> andi patrin le [[Special:RecentChanges|neve paruvimatenge]].

Kana kamesa te khoses kadaya patrin andar tiri lista le patryange so arakhes len ker click kai "Na mai arakh" (opre, kana i patrin dikhel pes).',
'removedwatchtext' => 'I patrin "[[:$1]]" sas khosli katar o patrinipen le dikhipnaske lekhenca (artikolurya).',
'watch'            => 'Dikh la',
'unwatch'          => 'Na mai dikh',
'unwatchthispage'  => 'Na mai dikh',
'wlnote'           => 'Tele si le palutne $1 paruvimata ande palutne <b>$2</b> ore.',

'enotif_reset'       => 'Thov semno kai patrya so dikhlem',
'enotif_newpagetext' => 'Kadaya si ek nevi patrin.',

# Delete/protect/revert
'deletepage'      => 'Khos i patrin',
'confirm'         => 'Ja',
'excontent'       => "o ander sas: '$1'",
'excontentauthor' => "o ander sas: '$1' (thai o korkoro butyarno sas '$2')",
'exblank'         => 'i patrin sas chuchi',
'historywarning'  => 'Dikh! La patrya so kames to khoses la si la puranipen:',
'actioncomplete'  => 'Agorisardi buti',
'deletedtext'     => '"<nowiki>$1</nowiki>" sas khosli.
Dikh ando $2 ek patrinipen le palutne butyange khosle.',
'deletedarticle'  => '"$1" sas khosli.',
'rollback_short'  => 'Palemavilipen',
'rollbacklink'    => 'palemavilipen',
'rollbackfailed'  => 'O palemavilipen nashtisardyas te kerel pes.',

# Undelete
'undelete'      => 'Dikh le khosle patrya',
'undeletebtn'   => 'Le palpale',
'undeletereset' => 'Khos le paruvimata',

# Namespace form on various pages
'namespace' => 'Rig:',
'invert'    => 'Bi rigyako:',

# Contributions
'contributions' => 'Jeneske butya',
'mycontris'     => 'Mire butya',
'contribsub2'   => 'Katar $1 ($2)',
'uctop'         => '(opre)',

# What links here
'whatlinkshere' => 'So phandel pes kathe',
'nolinkshere'   => 'Ni ek patrin phandel pes (avel) kathe.',

# Block/unblock
'blockip'      => 'De avri jenes/IP',
'ipbsubmit'    => 'De avri kadava jenes',
'ipusubmit'    => 'Na mai brakh i adresa',
'contribslink' => 'butya',

# Developer tools
'lockbtn'   => 'Brakh i database',
'unlockbtn' => 'Na mai brakh i database',

# Move page
'movearticle'     => 'Inger i patrin',
'movepagebtn'     => 'Inger i patrin',
'pagemovedsub'    => 'I patrin sas bićhaldi.',
'movedto'         => 'ingerdi kai',
'1movedto2'       => '[[$1]] bichhaldo kai [[$2]]',
'delete_and_move' => 'Khos thai inger',

# Export
'export-submit' => 'Bichhal avri',

# Namespace 8 related
'allmessages'     => 'Sistemoske duma',
'allmessagesname' => 'Anav',

# Special:Import
'import-interwiki-submit' => 'Le andre',

# Tooltip help for the actions
'tooltip-pt-userpage'           => 'Miri labyarneski pajina',
'tooltip-pt-anonuserpage'       => 'Miri labyarneski pajina ki akanutni IP adress',
'tooltip-pt-mytalk'             => 'Miri diskuciyaki pajina',
'tooltip-pt-anontalk'           => 'Diskucie le editisarimatenge ki akanutni IP adress',
'tooltip-pt-preferences'        => 'Sar kamav te dikhel pes miri pajina',
'tooltip-pt-watchlist'          => 'I lista le pajinenge so dikhav lendar (monitorizav).',
'tooltip-pt-mycontris'          => 'Le mire editisarimata',
'tooltip-pt-login'              => 'Mishto si te identifikares tut, pale na si musai.',
'tooltip-pt-anonlogin'          => 'Mishto si te identifikares tut, pale na si musai.',
'tooltip-pt-logout'             => 'Kathe aćhaves i sesiyunya',
'tooltip-ca-talk'               => 'Diskuciya le artikoleske',
'tooltip-ca-edit'               => 'Shai te editisares kadaya pajina. Mangas te paledikhes o teksto anglal te uxtaves les.',
'tooltip-ca-addsection'         => 'Kathe shai te thos ek komentaryo ki kadaya diskuciya.',
'tooltip-ca-viewsource'         => 'Kadaya pajina si brakhli. Shai numa te dikhes o source-code.',
'tooltip-ca-history'            => 'Purane versiune le dokumenteske.',
'tooltip-ca-protect'            => 'Brakh kadava dokumento.',
'tooltip-ca-delete'             => 'Khos kadava dokumento.',
'tooltip-ca-undelete'           => 'Palemthav le editisarimata kerdine le kadale dokumenteske sar sas anglal lesko khosipen.',
'tooltip-ca-move'               => 'Trade kadava dokumento.',
'tooltip-ca-watch'              => 'Thav kadava dokumento andi monitorizaripnaski lista.',
'tooltip-ca-unwatch'            => 'Khos kadava dokumento andar i monitorizaripnaski lista.',
'tooltip-search'                => 'Rod andi kadaya Wiki',
'tooltip-p-logo'                => 'I sherutni pajina',
'tooltip-n-mainpage'            => 'Dikh i sherutni pajina',
'tooltip-n-portal'              => 'O proyekto, so shai te keres, kai arakhes solucie.',
'tooltip-n-currentevents'       => 'Arakh janglimata le akanutne evenimenturenge',
'tooltip-n-recentchanges'       => 'I lista le neve paruvimatenge kerdini andi kadaya wiki.',
'tooltip-n-randompage'          => 'Ja ki ek aleatori pajina',
'tooltip-n-help'                => 'O than kai arakhes zhutipen.',
'tooltip-t-whatlinkshere'       => 'I lista sa le wiki pajinenge so aven (si phande) vi kathe',
'tooltip-t-recentchangeslinked' => 'Neve paruvimata andi kadaya pajina',
'tooltip-feed-rss'              => 'Kathe te pravares o RSS flukso le kadale pajinyako',
'tooltip-feed-atom'             => 'Kathe te pravares o Atom flukso le kadale pajinyako',
'tooltip-t-contributions'       => 'Dikh i lista le editisarimatenge le kadale labyaresko',
'tooltip-t-emailuser'           => 'Bićhal ek emailo le kadale labyareske',
'tooltip-t-upload'              => 'Bićhal imajine vai media files',
'tooltip-t-specialpages'        => 'I lista sa le spechiale pajinengi',
'tooltip-ca-nstab-main'         => 'Dikh o artikolo',
'tooltip-ca-nstab-user'         => 'Dikh i labyarengi pajina',
'tooltip-ca-nstab-media'        => 'Dikh i pajina media',
'tooltip-ca-nstab-special'      => 'Kadaya si ek spechiali pajina, nashti te editisares la.',
'tooltip-ca-nstab-project'      => 'Dikh i pajina le proyekteski',
'tooltip-ca-nstab-image'        => 'Dikh i imajinyaki pajina',
'tooltip-ca-nstab-mediawiki'    => 'Dikh o mesajo le sistemesko',
'tooltip-ca-nstab-template'     => 'Dikh o formato',
'tooltip-ca-nstab-help'         => 'Dikh i zhutipnaski pajina',
'tooltip-ca-nstab-category'     => 'Dikh i kategoriya',

# Attribution
'anonymous'        => 'Bijangle labyarne kai {{SITENAME}}',
'siteuser'         => 'Jeno kai {{SITENAME}} $1',
'lastmodifiedatby' => 'Kadaya patrin sas paruvdi agoreste $2, $1 katar $3.', # $1 date, $2 time, $3 user
'others'           => 'aver',
'siteusers'        => 'Jeno/e kai {{SITENAME}} $1',

# Image deletion
'deletedrevision' => 'Khoslo o purano paruvipen $1',

# Browsing diffs
'previousdiff' => '← Purano ververipen',
'nextdiff'     => 'Anglutno paruvipen →',

# Special:NewImages
'showhidebots' => '($1 boturya)',
'ilsubmit'     => 'Rod',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'sa',
'imagelistall'     => 'savore',
'watchlistall2'    => 'savore',
'namespacesall'    => 'savore',

# Trackbacks
'trackbacklink' => 'Vurma',

# Delete conflict
'deletedwhileediting' => 'Dikh: Kadaya patrin sas khosli de kana shirdyas (astardyas) te editisares la!',

# action=purge
'confirm_purge_button' => 'Va',

# Special:Version
'version' => 'Versiya', # Not used as normal message but as header for the special page itself

# Special:SpecialPages
'specialpages' => 'Uzalutne patrya',

);
