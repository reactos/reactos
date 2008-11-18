<?php
/** Guarani (Avañe'ẽ)
 *
 * @ingroup Language
 * @file
 *
 * @author Hugo.arg
 */

$fallback = 'es';

$namespaceNames = array(
	# NS_MEDIA            => 'Media',
	NS_SPECIAL          => 'Mba\'echĩchĩ',
	NS_MAIN             => '',
	NS_TALK             => 'Myangekõi',
	NS_USER             => 'Puruhára',
	NS_USER_TALK        => 'Puruhára_myangekõi',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK     => '$1_myangekõi',
	NS_IMAGE            => 'Ta\'ãnga',
	NS_IMAGE_TALK       => 'Ta\'ãnga_myangekõi',
	NS_MEDIAWIKI        => 'MediaWiki',
	NS_MEDIAWIKI_TALK   => 'MediaWiki_myangekõi',
	NS_TEMPLATE         => 'Tembiecharã',
	NS_TEMPLATE_TALK    => 'Tembiecharã_myangekõi',
	NS_HELP             => 'Pytyvõ',
	NS_HELP_TALK        => 'Pytyvõ_myangekõi',
	NS_CATEGORY         => 'Ñemohenda',
	NS_CATEGORY_TALK    => 'Ñemohenda_myangekõi'
);

$messages = array(
'underline-always' => 'Akói',
'underline-never'  => "Araka'eve",

# Dates
'sunday'       => 'arateĩ',
'monday'       => 'arakői',
'tuesday'      => 'araapy',
'wednesday'    => 'ararundy',
'thursday'     => 'arapo',
'friday'       => 'arapoteĩ',
'saturday'     => 'arapokői',
'january'      => 'jasyteĩ',
'february'     => 'jasykői',
'march'        => 'jasyapy',
'april'        => 'jasyrundy',
'may_long'     => 'jasypo',
'june'         => 'jasypoteĩ',
'july'         => 'jasypokői',
'august'       => 'jasypoapy',
'september'    => 'jasyporundy',
'october'      => 'jasypa',
'november'     => 'jasypateĩ',
'december'     => 'jasypakői',
'january-gen'  => 'jasyteĩ',
'february-gen' => 'jasykõi',
'jan'          => 'jasyteĩ',
'feb'          => 'jasykõi',
'may'          => 'jasypo',
'dec'          => 'jasypakõi',

# Categories related messages
'pagecategories' => '{{PLURAL:$1|Ñemohenda|Ñemohendakuéra}}',

'about'          => 'Rehegua',
'article'        => 'Kuatiahai',
'newwindow'      => "(Ojepe'a peteĩ ovetã pyahu)",
'cancel'         => 'Mbotove',
'qbfind'         => 'Heka',
'qbedit'         => 'Jehaijey',
'qbspecialpages' => 'Kuatiarogue hekochĩchĩva',
'mypage'         => 'Che kuatiarogue',
'mytalk'         => 'Che myangekõi',
'navigation'     => 'Kundaharã',

'help'             => 'Pytyvõhára',
'search'           => 'Heka',
'searchbutton'     => 'Heka',
'go'               => 'Ha',
'searcharticle'    => 'Ha',
'history'          => 'Tembiasakue',
'history_short'    => 'Tembiasakue',
'edit'             => 'Jehaijey',
'delete'           => "Mboje'o",
'undelete_short'   => 'Restaurar $1 ediciones',
'talkpagelinktext' => "ñe'ẽ",
'specialpage'      => "Kuatiarogue mba'echĩchĩ",
'talk'             => 'Myangekõi',
'toolbox'          => 'Tembiporu',
'mediawikipage'    => 'Hecha kuatiarogue marandu',
'viewtalkpage'     => 'Hecha myangekõi',
'otherlanguages'   => "Ambue ñe'ẽ",
'redirectedfrom'   => '(Oñembohapejeýva $1)',
'lastmodifiedat'   => 'Ko kuatiarogue oñemoambuejeýkuri: $2, $1.', # $1 date, $2 time
'viewcount'        => 'Esta página ha sido visitada $1 veces.',
'jumpto'           => 'Kundaharãme jeho',
'jumptonavigation' => 'kundaharã',
'jumptosearch'     => 'Jeheka',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutpage'            => 'Project:Rehegua',
'copyright'            => "Tembikuaa'aty ikatu ojepuru <i>$1</i> rekópe",
'currentevents'        => 'Ag̃agua',
'disclaimers'          => 'Marandu leiguigua',
'edithelp'             => 'Jehairã ñepytyvõ',
'mainpage'             => 'Ape',
'mainpage-description' => 'Ape',
'portal'               => 'Tekohapegua',
'privacy'              => 'Polítika marandu ñeñangareko rehegua',
'privacypage'          => 'Project:Polítika marandu ñeñangareko rehegua',

'newmessageslink'         => 'marandu pyahu',
'newmessagesdifflink'     => 'Joavy oĩva mokõive jehai paha apytépe',
'youhavenewmessagesmulti' => 'Reguereko marandu pyahu $1',
'editsection'             => 'jehaijey',
'editsection-brackets'    => '($1)',
'editold'                 => 'jehaijey',
'editsectionhint'         => 'Jehaijey vore: $1',
'toc'                     => "Tembikuaa'aty rechaukaha",
'showtoc'                 => 'hechauka',
'hidetoc'                 => 'toñemi',
'restorelink'             => '$1 ediciones borradas',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Kuatiahai',
'nstab-user'      => 'Puruhára',
'nstab-special'   => "Mba'echĩchĩ",
'nstab-image'     => "Ta'ãnga",
'nstab-mediawiki' => 'Marandu',
'nstab-template'  => 'Tembiecharã',
'nstab-category'  => 'Ñemohenda',

# General errors
'ns-specialprotected' => 'Las páginas en el espacio de nombres Especial no se pueden editar.',

# Login and logout pages
'loginpagetitle'          => 'Terañemboguapy/Ke',
'yourname'                => 'Hero',
'yourpassword'            => 'Ne remiñemi',
'yourpasswordagain'       => 'Repita ne remiñemi',
'remembermypassword'      => "Aipota chemomandu'ajepi amba'apo jave.",
'login'                   => 'Terañemboguapy/Ke',
'nav-login-createaccount' => 'Terañemboguapy/Ke',
'userlogin'               => 'Terañemboguapy/Ke',
'logout'                  => 'Sẽ',
'userlogout'              => 'Sẽ',
'nologin'                 => "¿Ne'ĩrãpa remohenda nde réra? $1.",
'nologinlink'             => 'Téra ñemohenda',
'youremail'               => "Ñe'ẽveve",
'yourlanguage'            => "Ñe'ẽ:",
'email'                   => 'Pareha eleytróniko',
'loginsuccesstitle'       => 'Remoñepyrũ hekopete ne rembiapo',
'nosuchusershort'         => 'No hay un usuario con el nombre "$1". Compruebe que lo ha escrito correctamente.',
'mailmypassword'          => "Embou chéve ñe'ẽveve rupive peteĩ temiñemĩ pyahu",
'loginlanguagelabel'      => "Ñe'ẽ: $1",

# Edit page toolbar
'bold_sample'   => 'Haipyre oñemohũvéva',
'bold_tip'      => 'Haipyre oñemohũvéva',
'italic_sample' => 'Haipyre ikarẽva',
'italic_tip'    => 'Haipyre ikarẽva',
'link_tip'      => 'Joaju hyepyguávandi',
'extlink_tip'   => 'Joaju okapeguávandi (recuerde añadir el prefijo http://)',
'headline_tip'  => 'Teraete mokõiha',
'math_tip'      => 'Matemátika kuaareko (LaTeX)',
'nowiki_tip'    => "Viki jehaireko ñembo'yke",
'image_tip'     => "Ta'ãnga moĩngepyréva",
'media_tip'     => "Joaju jehai'aty multimediaguándi",
'sig_tip'       => 'Teraguapy, arange, aravo',
'hr_tip'        => 'Haipuku oñenóva (eipurúke tekotevẽ javénte)',

# Edit pages
'summary'               => 'Jehaimombyky',
'subject'               => "Mba'ekuaarã/teraete",
'minoredit'             => "Kóva ha'e peteĩ jehai mbyky",
'watchthis'             => 'Toñeñangareko ko tembiapóre',
'savearticle'           => 'Hai',
'showpreview'           => 'Tojechauka jehai ñemboguapy mboyve',
'showdiff'              => 'Tojechauka ñemoambue',
'missingsummary'        => "'''Atención:''' No has escrito un resumen de edición. Si haces clic nuevamente en «Hai» tu edición se grabará sin él.",
'subject-preview'       => "Previsualización del mba'ekuaarã/teraete",
'newarticletext'        => "Rehapykuehókuri peteĩ joaju peteĩ kuatiarogue ndaipórivape.
Nde remoheñoisérõ ko kuatiarogue, eñepyrũkatu ehai.
Reikotevẽvérõ marandu, emoñe'ẽ kuatiarogue ñepytyvõ rehegua. Oiméramo reikereínte térã rejavyhaguére, upéicharõ terehojey [[{{MediaWiki:Helppage}}|kuatiarogue mboyveguápe]].",
'userinvalidcssjstitle' => "'''Aviso:''' No existe la piel \"\$1\". Recuerda que las páginas personalizadas .css y .js tienen un título en minúsculas, p.e. Usuario:Foo/monobook.css en vez de  Usuario:Foo/Monobook.css.",
'editing'               => 'Ojehaihína $1',
'editingsection'        => 'Ojehaihína $1 (vore)',
'editingcomment'        => 'Ojehaihína $1 (comentario)',
'yourtext'              => "Mba'ehaipyre",
'longpagewarning'       => "<strong>Ejesarekóke: ko kuatiarogue, tuichakuépe, oguereko $1 kb; heta kundahára ikatu iñapañuãi jehaijeýpe kuatiarogue ohaságui 32 kb.
Aipórõ, eñeha'ãna emboja'o ne rembiapo, vore michĩvévape.</strong>",

# History pages
'deletedrev'  => '[oñembogue]',
'historysize' => '($1 bytes)',

# Revision deletion
'rev-delundel'       => 'hechauka/toñemi',
'logdelete-selected' => 'Seleccionados $2 eventos de registro:',

# Diffs
'difference' => "(Mba'épe ojaovy oñemyatyrõva'ekue)",
'editundo'   => 'embyai',
'diff-multi' => '($1 ediciones intermedias no se muestran.)',

# Search results
'searchresults'     => 'Ojejuhúva jeheka',
'searchsubtitle'    => "Nde reporandúkuri: '''[[:$1]]-re'''",
'noexactmatch'      => "'''Ndaipóri kuatiarogue hérava: \"\$1\".''' Ikatu ndetevoi [[:\$1|rehai]].",
'prevn'             => '$1 mboyvegua',
'viewprevnext'      => 'Hecha ($1) ($2) ($3).',
'showingresults'    => "Abajo se muestran hasta '''$1''' resultados empezando por el nº '''$2'''.",
'showingresultsnum' => "Abajo se muestran los '''$3''' resultados empezando por el nº '''$2'''.",
'powersearch'       => 'Jeheka',

# Preferences page
'preferences'       => 'Mbohoryha',
'mypreferences'     => 'Che mbohoryha',
'prefs-rc'          => 'Oñemoambue pyahúva',
'searchresultshead' => 'Jeheka',

# User rights
'userrights-lookup-user'   => 'Configurar grupos de usuarios',
'userrights-user-editname' => 'Ehaimi peteĩ téra puruháragua:',
'editusergroup'            => 'Modificar grupos de usuarios',
'editinguser'              => "Ojehaihína '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup' => 'Modificar grupos de usuarios',
'saveusergroups'           => 'Guardar grupos de usuarios',
'userrights-groupsmember'  => 'Pehengue:',
'userrights-reason'        => 'Motivo para el cambio:',

# Groups
'group-all' => '(opaite)',

# Recent changes
'nchanges'          => '$1 ñemoambue',
'recentchanges'     => 'Oñemoambue pyahúva',
'rcnote'            => 'Iguýpe oĩ umi <b>$1</b> oñemoambue pyahúva ko ara<b>$2</b>ndýpe, hekopyahúva $3',
'rclistfrom'        => 'Tojehechauka oñemoambue pyahúva $1 guive',
'rcshowhideminor'   => '$1 jehaijey michĩva',
'rcshowhideliu'     => '$1 puruhára ohejáva teraguapy',
'rcshowhideanons'   => "$1 puruhára ojekuaa'ỹva",
'rcshowhidemine'    => '$1 che jehaijey',
'rclinks'           => "Ápe ojehechakuaa umi $1 oñemoambue pyahúva $2 ára ohasava'ekuépe.<br />$3",
'hide'              => 'toñemi',
'show'              => 'hechauka',
'newsectionsummary' => 'Pyahuvore: /* $1 */',

# Recent changes linked
'recentchangeslinked-title'   => 'Ñemoambue $1 rehegua',
'recentchangeslinked-summary' => "Ko kuatiarogue hekochĩchĩvape oñembohysýi umi ñemoambue ipyahúva ko'ã kuatiarogue ojoajúvape. Kuatiarogue oĩva tapykueho rysýipe oĩ '''haipyre oñemohũvape'''.",

# Upload
'upload'     => "Tojehupi jehai'aty",
'uploadtext' => "Eipuru pe tembipuru oĩva iguýpe ehupi hag̃ua jehai'aty, rehecha térã reheka hag̃ua ta'ãnga ojehupipyrémava eike jehai'aty jehupipyre rysýipe, umi ihupipyréva ha oñemboguémava avei oñemboguapy [[Special:Log/upload|jehai'aty jehupipyrépe]].
Reomĩsérõ ta'ãnga peteĩ kuatiaroguépe, eipuru peteĩ joaju: 
'''<nowiki>[[</nowiki>Imagen<nowiki>:Archivo.jpg]]</nowiki>''', '''<nowiki>[[</nowiki>Imagen<nowiki>:Archivo.png|texto alternativo]]</nowiki>''' o
'''<nowiki>[[</nowiki>Media<nowiki>:Archivo.ogg]]</nowiki>''' ojoaju hag̃ua hekopete pe jehai'atýre.",
'filename'   => "Téra jehai'aty",

# Special:ImageList
'imagelist'      => "Ta'ãnga rysýi",
'imagelist_user' => 'Puruhára',

# Image description page
'filehist-revert' => 'embojevy',
'filehist-user'   => 'Puruhára',

# File reversion
'filerevert' => 'Embojevy $1',

# File deletion
'filedelete-legend'  => "Mboje'o jehai'aty",
'filedelete-success' => "'''$1''' oñembogue'akue",

# MIME search
'mimesearch' => 'Jeheka MIME',

# List redirects
'listredirects' => 'Ñembohapejey rysýi',

# Unused templates
'unusedtemplates' => 'Tembiecharã ndojepurúiva',

# Random page
'randompage' => "Kuatiarogue oñembosako'íva",

# Random redirect
'randomredirect' => 'Oimerãe ñembohapejeýpe jeho',

# Statistics
'statistics' => 'Papyrekokuaa',

'disambiguations'     => 'Kuatiarogue mohesakãporãha',
'disambiguationspage' => 'Template:Disambig',

'doubleredirects' => "Ñembohapejey jo'apyre",

'brokenredirects'        => "Ñembohapejey hekopegua'ỹva",
'brokenredirects-edit'   => '(jehaijey)',
'brokenredirects-delete' => "(mboje'o)",

'withoutinterwiki' => 'Kuatiarogue ndorekóiva interwiki',

'fewestrevisions' => "Kuatiahai sa'ive ijehaijeýva",

# Miscellaneous special pages
'nbytes'                  => '$1 bytes',
'ncategories'             => '$1 ñemohendakuéra',
'nlinks'                  => '$1 joaju',
'nmembers'                => '$1 kuatiahai',
'nrevisions'              => '$1 revisiones',
'nviews'                  => '$1 vistas',
'lonelypages'             => "Kuatiarogue ityre'ỹva",
'uncategorizedpages'      => "Kuatiarogue oñemohenda'ỹva",
'uncategorizedcategories' => 'Ñemohenda noñemohendáiva',
'uncategorizedimages'     => "Ta'ãnga ne'ĩrã oñemohendáva",
'uncategorizedtemplates'  => 'Tembiecharã noñemohendáiva',
'unusedcategories'        => "Ñemohenda ojepuru'ỹva",
'unusedimages'            => "Ta'ãnga ndojepurúiva",
'wantedcategories'        => 'Ñemohenda oñekotevẽva',
'wantedpages'             => 'Kuatiarogue oñekotevẽva',
'mostlinked'              => 'Kuatiahai ojoaju hetavéva',
'mostlinkedcategories'    => 'Ñemohenda ojoaju hetavéva',
'mostlinkedtemplates'     => 'Tembiecharã ojoaju hetavéva',
'mostcategories'          => 'Kuatiarogue iñemohenda hetavéva',
'mostimages'              => "Ta'ãnga ojepuruvéva",
'mostrevisions'           => 'Kuatiahai hetave ijehaijeýva',
'prefixindex'             => 'Kuatiarogue henondepyguáva',
'shortpages'              => 'Kuatiarogue mbykýva',
'longpages'               => 'Kuatiarogue ipukúva',
'deadendpages'            => "Kuatiarogue ñesẽ'ỹva",
'protectedpages'          => 'Kuatiarogue oñeñangarekóva',
'listusers'               => 'Puruhára rysýi',
'newpages'                => 'Kuatiarogue ipyahúva',
'newpages-username'       => 'Hero',
'ancientpages'            => "Kuatiahai hi'arevéva",
'move'                    => 'Guerova',
'movethispage'            => 'Guerova kuatiarogue',

# Book sources
'booksources' => "Heñóiva kuatiañe'ẽme",

# Special:Log
'log' => 'Ñemboguapypyre',

# Special:AllPages
'allpages'          => 'Opaite kuatiarogue',
'allnotinnamespace' => 'Opaite kuatiarogue (fuera del espacio $1)',
'allpagesnext'      => 'Upeigua',
'allpagessubmit'    => 'Hechauka',

# Special:Categories
'categories' => 'Ñemohendakuéra',

# Special:ListUsers
'listusers-submit' => 'Hechauka',

# E-mail user
'emailuser'    => "Tojeguerahauka ñe'ẽveve ko puruhárape",
'emailpage'    => 'Pareha eleytrónico',
'emailmessage' => 'Marandu',

# Watchlist
'watchlist'   => 'Tapykueho rysýi',
'mywatchlist' => 'Tapykueho rysýi',
'watch'       => 'Ñangareko',

# Delete/protect/revert
'deletedarticle' => 'oñembogue "[[$1]]"',
'dellogpage'     => 'Ñemboguepyre ñonagatupy',
'deletionlog'    => 'ñemboguepyre ñonagatupy',
'rollback_short' => 'Embojevy',
'rollbacklink'   => 'Embojevy',
'protect-text'   => 'Puedes ver y modificar el nivel de protección de la página <strong>$1</strong>.',

# Undelete
'undeletedrevisions'       => '$1 ediciones restauradas',
'undeletedrevisions-files' => '$1 ediciones y $2 archivos restaurados',
'undeletedfiles'           => '$1 archivos restaurados',
'undelete-search-submit'   => 'Heka',

# Namespace form on various pages
'namespace'      => 'Téra rendagua:',
'invert'         => "Toñembo'ovývo mba'eporavopyre",
'blanknamespace' => '(Tenondeguáva)',

# Contributions
'contributions' => "Puruhára mba'emoĩmbyre",
'mycontris'     => "Che mba'emoĩmbyre",

'sp-contributions-search' => "Heka mba'emoĩmbyre",
'sp-contributions-submit' => 'Heka',

# What links here
'whatlinkshere'       => "Oñembojoajukuaáva ko'ápe",
'whatlinkshere-title' => 'Kuatiarogue ojoajúva "$1" rehe',
'whatlinkshere-page'  => 'Kuatiarogue:',
'linklistsub'         => '(Joajukuéra rysýi)',
'linkshere'           => "Ko'ã kuatiarogue ojoaju '''[[:$1]]''' rehe:",
'whatlinkshere-prev'  => 'mboyvegua $1',
'whatlinkshere-next'  => 'upeigua $1',

# Block/unblock
'blockip'            => 'Ejoko puruhára',
'ipblocklist'        => 'IP mbohape rysýi imbotypyréva',
'ipblocklist-submit' => 'Heka',
'blocklink'          => 'ejoko',
'contribslink'       => "mba'emoĩmbyre",
'blocklogtext'       => 'Esto es un registro de bloqueos y desbloqueos de usuarios. Las direcciones bloqueadas automáticamente no aparecen aquí. Consulte la [[Special:IPBlockList|IP mbohape rysýi imbotypyréva]] para ver la lista de prohibiciones y bloqueos actualmente vigente.',

# Move page
'movearticle'     => 'Guerova kuatiarogue',
'move-watch'      => 'Toñeñangareko ko tembiapóre',
'movepagebtn'     => 'Guerova kuatiarogue',
'1movedto2'       => '[[$1]] oñembohasa [[$2]]-pe',
'revertmove'      => 'embojevy',
'delete_and_move' => "Mboje'o ha guerova",

# Export
'export' => 'Kuatiarogue ñemondo',

# Namespace 8 related
'allmessages' => 'Opaite marandu MediaWikigua',

# Special:Import
'import-revision-count' => '$1 revisiones',

# Import log
'import-logentry-upload-detail'    => '$1 revisiones',
'import-logentry-interwiki-detail' => '$1 revisiones desde $2',

# Tooltip help for the actions
'tooltip-pt-mytalk'    => 'Che kuatiarogue myangekõi',
'tooltip-pt-mycontris' => "Tysỹi che mba'emoĩmbyre",
'tooltip-ca-move'      => 'Guerova kuatiarogue',
'tooltip-p-logo'       => 'Ape',

# Spam protection
'spamprotectiontitle' => 'Filtro de protección contra spam',
'spamprotectiontext'  => 'La página que intentas guardar ha sido bloqueada por el filtro de spam. Esto se debe probablemente a alguno de los un enlaces externos incluidos en ella.',
'spamprotectionmatch' => "El siguiente texto es el que activó nuestro filtro ''anti-spam'' (contra la publicidad no solicitada): $1",
'spambot_username'    => 'Limpieza de spam de MediaWiki',
'spam_reverting'      => 'Revirtiendo a la última versión que no contenga enlaces a $1',
'spam_blanking'       => 'Todas las revisiones contienen enlaces a $1, blanqueando',

# Media information
'file-info' => "(tamaño de jehai'aty: $1; tipo MIME: $2)",

# Special:NewImages
'newimages' => "Ta'ãnga pyahu renda",
'ilsubmit'  => 'Jeheka',

# 'all' in various places, this might be different for inflected languages
'namespacesall' => 'opaite',

# Auto-summaries
'autosumm-new' => 'Kuatiarogue ipyahúva: $1',

# Watchlist editor
'watchlistedit-numitems'      => 'Tu lista de seguimiento tiene $1 páginas, excluyendo las páginas de discusión.',
'watchlistedit-normal-title'  => 'Moambue tapykueho rysýi',
'watchlistedit-normal-submit' => "Mboje'o kuatiarogue",
'watchlistedit-normal-done'   => '$1 páginas han sido borradas de tu lista de seguimiento:',
'watchlistedit-raw-titles'    => 'Kuatiarogue:',
'watchlistedit-raw-added'     => 'Se han añadido $1 páginas:',
'watchlistedit-raw-removed'   => '$1 páginas han sido borradas:',

# Special:Version
'version' => "Mba'ereko", # Not used as normal message but as header for the special page itself

# Special:FilePath
'filepath'        => 'Ruta de archivo',
'filepath-page'   => 'Archivo:',
'filepath-submit' => 'Ruta',

# Special:SpecialPages
'specialpages' => 'Kuatiarogue hekochĩchĩva',

);
