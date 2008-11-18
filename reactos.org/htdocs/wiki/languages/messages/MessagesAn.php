<?php
/** Aragonese (Aragonés)
 *
 * @ingroup Language
 * @file
 *
 * @author Juanpabl
 * @author Willtron
 * @author לערי ריינהארט
 */

$fallback = 'es';

$skinNames = array(
	'standard'    => 'Clasica (Classic)',
	'nostalgia'   => 'Recosiros (Nostalgia)',
	'cologneblue' => 'Colonia Azul (Cologne Blue)',
	'myskin'      => 'A mía aparenzia (MySkin)',
	'simple'      => 'Simpla (Simple)',
);

$namespaceNames = array(
	NS_MEDIA          => 'Media',
	NS_SPECIAL        => 'Espezial',
	NS_MAIN           => '',
	NS_TALK           => 'Descusión',
	NS_USER           => 'Usuario',
	NS_USER_TALK      => 'Descusión_usuario',
	# NS_PROJECT set by \$wgMetaNamespace
	NS_PROJECT_TALK   => 'Descusión_$1',
	NS_IMAGE          => 'Imachen',
	NS_IMAGE_TALK     => 'Descusión_imachen',
	NS_MEDIAWIKI      => 'MediaWiki',
	NS_MEDIAWIKI_TALK => 'Descusión_MediaWiki',
	NS_TEMPLATE       => 'Plantilla',
	NS_TEMPLATE_TALK  => 'Descusión_plantilla',
	NS_HELP           => 'Aduya',
	NS_HELP_TALK      => 'Descusión_aduya',
	NS_CATEGORY       => 'Categoría',
	NS_CATEGORY_TALK  => 'Descusión_categoría',
);

$magicWords = array(
	'namespace'           => array( '1', 'NAMESPACE', 'ESPAZIODENOMBRES' ),
	'namespacee'          => array( '1', 'NAMESPACEE', 'ESPAZIODENOMBRESE' ),
	'img_right'           => array( '1', 'right', 'dreita' ),
	'img_left'            => array( '1', 'left', 'cucha', 'zurda' ),
	'ns'                  => array( '0', 'NS:', 'EN:', 'EDN:' ),
	'displaytitle'        => array( '1', 'DISPLAYTITLE', 'TÍTOL' ),
	'currentversion'      => array( '1', 'CURRENTVERSION', 'BERSIÓNAUTUAL', 'BERSIONAUTUAL' ),
	'language'            => array( '0', '#LANGUAGE:', '#LENGUACHE:', '#LUENGA:', '#IDIOMA:' ),
	'special'             => array( '0', 'special', 'espezial' ),
	'defaultsort'         => array( '1', 'DEFAULTSORT:', 'DEFAULTSORTKEY:', 'DEFAULTCATEGORYSORT:', 'ORDENAR:' ),
);

$specialPageAliases = array(
	'DoubleRedirects'           => array( 'Reendrezeras_dobles', 'Dobles_reendrezeras', 'Endrezeras_dobles', 'Dobles_endrezeras' ),
	'BrokenRedirects'           => array( 'Reendrezeras_trencatas', 'Endrezeras_trencatas', 'Reendrezeras_crebatas', 'Endrezeras_crebatas', 'Endrezeras_trencadas', 'Endrezeras_crebadas' ),
	'Disambiguations'           => array( 'Desambigazions', 'Pachinas_de_desambigazión' ),
	'Userlogin'                 => array( 'Enzetar_sesión', 'Dentrar' ),
	'Userlogout'                => array( 'Salir', 'Rematar_sesión' ),
	'Preferences'               => array( 'Preferenzias' ),
	'Watchlist'                 => array( 'Lista_de_seguimiento' ),
	'Recentchanges'             => array( 'Zaguers_cambeos', 'cambeos_rezients' ),
	'Upload'                    => array( 'Cargar', 'Puyar' ),
	'Imagelist'                 => array( 'Lista_d\'imáchens', 'Lista_d\'imachens' ),
	'Listusers'                 => array( 'Lista_d\'usuarios' ),
	'Statistics'                => array( 'Estadistica', 'Estatistica', 'Estadisticas', 'Estatisticas' ),
	'Randompage'                => array( 'Pachina_aleatoria', 'Pachina_aliatoria', 'Pachina_á_l\'azar' ),
	'Lonelypages'               => array( 'Pachinas_popiellas' ),
	'Uncategorizedpages'        => array( 'Pachinas_sin_categorías', 'Pachinas_sin_categorizar' ),
	'Uncategorizedcategories'   => array( 'Categorías_sin_categorías', 'Categorías_sin_categorizar' ),
	'Uncategorizedimages'       => array( 'Imáchens_sin_categorías', 'Imáchens_sin_categorías', 'Imachens_sin_categorizar', 'Imáchens_sin_categorizar' ),
	'Uncategorizedtemplates'    => array( 'Plantillas_sin_categorías', 'Plantillas_sin_categorizar' ),
	'Unusedcategories'          => array( 'Categorías_no_emplegatas', 'Categorías_sin_emplegar' ),
	'Unusedimages'              => array( 'Imáchens_no_emplegatas', 'Imáchens_sin_emplegar' ),
	'Wantedpages'               => array( 'Pachinas_requiestas', 'Pachinas_demandatas', 'Binclos_crebatos', 'Binclos_trencatos' ),
	'Wantedcategories'          => array( 'Categorías_requiestas', 'Categorías_demandatas' ),
	'Mostlinked'                => array( 'Pachinas_más_enlazatas', 'Pachinas_más_binculatas' ),
	'Mostlinkedcategories'      => array( 'Categorías_más_enlazatas', 'Categorías_más_binculatas' ),
	'Mostlinkedtemplates'       => array( 'Plantillas_más_enlazatas', 'Plantillas_más_binculatas' ),
	'Mostcategories'            => array( 'Pachinas_con_más_categorías' ),
	'Mostimages'                => array( 'Imáchens_más_emplegatas', 'Imachens_más_emplegatas' ),
	'Mostrevisions'             => array( 'Pachinas_con_más_edizions', 'Pachinas_más_editatas', 'Pachinas_con_más_bersions' ),
	'Fewestrevisions'           => array( 'Pachinas_con_menos_edizions', 'Pachinas_menos_editatas', 'Pachinas_con_menos_bersions' ),
	'Shortpages'                => array( 'Pachinas_más_cortas' ),
	'Longpages'                 => array( 'Pachinas_más_largas' ),
	'Newpages'                  => array( 'Pachinas_nuebas', 'Pachinas_más_nuebas', 'Pachinas_más_rezients', 'Pachinas_rezients' ),
	'Ancientpages'              => array( 'Pachinas_más_biellas', 'Pachinas_biellas', 'Pachinas_más_antigas', 'Pachinas_antigas' ),
	'Deadendpages'              => array( 'Pachinas_sin_salida', 'Pachinas_sin_de_salida' ),
	'Protectedpages'            => array( 'Pachinas_protechitas', 'Pachinas_protechitas', 'Pachinas_protechidas' ),
	'Protectedtitles'           => array( 'Títols_protechitos', 'Títols_protexitos', 'Títols_protechius' ),
	'Allpages'                  => array( 'Todas_as_pachinas' ),
	'Prefixindex'               => array( 'Pachinas_por_prefixo', 'Mirar_por_prefixo' ),
	'Ipblocklist'               => array( 'Lista_d\'IPs_bloqueyatas', 'Lista_d\'IPs_bloquiatas', 'Lista_d\'adrezas_IP_bloqueyatas', 'Lista_d\'adrezas_IP_bloquiatas' ),
	'Specialpages'              => array( 'Pachinas_espezials' ),
	'Contributions'             => array( 'Contrebuzions' ),
	'Emailuser'                 => array( 'Nimbía_mensache' ),
	'Movepage'                  => array( 'Renombrar_pachina', 'Mober_pachina', 'Tresladar_pachina' ),
	'Categories'                => array( 'Categorías' ),
	'Export'                    => array( 'Esportar' ),
	'Version'                   => array( 'Bersión' ),
	'Allmessages'               => array( 'Toz_os_mensaches' ),
	'Import'                    => array( 'Importar' ),
	'Mypage'                    => array( 'A_mía_pachina', 'A_mía_pachina_d\'usuario' ),
	'Mytalk'                    => array( 'A_mía_descusión', 'A_mía_pachina_de_descusión' ),
	'Mycontributions'           => array( 'As_mías_contrebuzions' ),
	'Listadmins'                => array( 'Lista_d\'almenistradors' ),
	'Listbots'                  => array( 'Lista_de_bots' ),
	'Popularpages'              => array( 'Pachinas_populars', 'Pachinas_más_populars' ),
	'Search'                    => array( 'Mirar' ),
);

$messages = array(
# User preference toggles
'tog-underline'               => 'Subrayar os binclos:',
'tog-highlightbroken'         => 'Formateyar os binclos trencatos <a href="" class="new"> d\'ista traza </a> (y si no, asinas <a href="" class="internal">?</a>).',
'tog-justify'                 => 'Achustar parrafos',
'tog-hideminor'               => 'Amagar edizions menors en a pachina de "zaguers cambeos"',
'tog-extendwatchlist'         => 'Enamplar a lista de seguimiento ta amostrar toz os cambeos afeutatos.',
'tog-usenewrc'                => 'Presentazión amillorada d\'os "zaguers cambeos" (cal JavaScript)',
'tog-numberheadings'          => 'Numerar automaticament os encabezaus',
'tog-showtoolbar'             => "Amostrar a barra d'ainas d'edizión (cal JavaScript)",
'tog-editondblclick'          => 'Autibar edizión de pachinas fendo-ie doble click (cal JavaScript)',
'tog-editsection'             => 'Autibar a edizión por sezions usando os binclos [editar]',
'tog-editsectiononrightclick' => "Autibar a edizión de sezions punchando con o botón dreito d'a rateta <br /> en os títols de sezions (cal JavaScript)",
'tog-showtoc'                 => 'Amostrar o endize de contenius (ta pachinas con más de 3 encabezaus)',
'tog-rememberpassword'        => 'Remerar a palabra de paso entre sesions',
'tog-editwidth'               => "O cuatrón d'edizión tien l'amplaria masima",
'tog-watchcreations'          => 'Cosirar as pachinas que creye',
'tog-watchdefault'            => 'Cosirar as pachinas que edite',
'tog-watchmoves'              => 'Cosirar as pachinas que treslade',
'tog-watchdeletion'           => 'Cosirar as pachinas que borre',
'tog-minordefault'            => 'Siñalar por defeuto totas as edizions como menors',
'tog-previewontop'            => "Amostrar l'ambiesta prebia antes d'o cuatrón d'edizión",
'tog-previewonfirst'          => "Amostrar l'ambiesta prebia de l'articlo en a primera edizión",
'tog-nocache'                 => "Desautibar a ''caché'' de pachinas",
'tog-enotifwatchlistpages'    => 'Rezibir un correu cuan se faigan cambios en una pachina cosirata por yo',
'tog-enotifusertalkpages'     => 'Nimbiar-me un correu cuan cambee a mía pachina de descusión',
'tog-enotifminoredits'        => 'Nimbiar-me un correu tamién cuan bi aiga edizions menors de pachinas',
'tog-enotifrevealaddr'        => 'Fer beyer a mía adreza de correu-e en os correus de notificazión',
'tog-shownumberswatching'     => "Amostrar o numero d'usuarios que cosiran un articlo",
'tog-fancysig'                => 'Siñaduras simplas (sin de binclo automatico)',
'tog-externaleditor'          => "Fer serbir l'editor esterno por defeuto (nomás ta espiertos, cal confegurar o suyo ordenador).",
'tog-externaldiff'            => 'Fer serbir o bisualizador de cambeos esterno por defeuto (nomás ta espiertos, cal confegurar o suyo ordenador)',
'tog-showjumplinks'           => 'Autibar binclos d\'azesibilidat "blincar enta"',
'tog-uselivepreview'          => 'Autibar prebisualizazión automatica (cal JavaScript) (Esperimental)',
'tog-forceeditsummary'        => 'Abisar-me cuan o campo de resumen siga buedo.',
'tog-watchlisthideown'        => 'Amagar as mías edizions en a lista de seguimiento',
'tog-watchlisthidebots'       => 'Amagar edizions de bots en a lista de seguimiento',
'tog-watchlisthideminor'      => 'Amagar edizions menors en a lista de seguimiento',
'tog-nolangconversion'        => 'Desautibar conversión de bariants',
'tog-ccmeonemails'            => 'Rezibir copias de os correus que nimbío ta atros usuarios',
'tog-diffonly'                => "No amostrar o conteniu d'a pachina debaxo d'as esferenzias",
'tog-showhiddencats'          => 'Amostrar categorías amagatas',

'underline-always'  => 'Siempre',
'underline-never'   => 'Nunca',
'underline-default' => "Confegurazión por defeuto d'o nabegador",

'skinpreview' => '(Fer una prebatina)',

# Dates
'sunday'        => 'domingo',
'monday'        => 'luns',
'tuesday'       => 'martes',
'wednesday'     => 'miércols',
'thursday'      => 'chuebes',
'friday'        => 'biernes',
'saturday'      => 'sabado',
'sun'           => 'dom',
'mon'           => 'lun',
'tue'           => 'mar',
'wed'           => 'mie',
'thu'           => 'chu',
'fri'           => 'bie',
'sat'           => 'sab',
'january'       => 'chinero',
'february'      => 'febrero',
'march'         => 'marzo',
'april'         => 'abril',
'may_long'      => 'mayo',
'june'          => 'chunio',
'july'          => 'chulio',
'august'        => 'agosto',
'september'     => 'setiembre',
'october'       => 'otubre',
'november'      => 'nobiembre',
'december'      => 'abiento',
'january-gen'   => 'de chinero',
'february-gen'  => 'de febrero',
'march-gen'     => 'de marzo',
'april-gen'     => "d'abril",
'may-gen'       => 'de mayo',
'june-gen'      => 'de chunio',
'july-gen'      => 'de chulio',
'august-gen'    => "d'agosto",
'september-gen' => 'de setiembre',
'october-gen'   => "d'otubre",
'november-gen'  => 'de nobiembre',
'december-gen'  => "d'abiento",
'jan'           => 'chi',
'feb'           => 'feb',
'mar'           => 'mar',
'apr'           => 'abr',
'may'           => 'may',
'jun'           => 'chun',
'jul'           => 'chul',
'aug'           => 'ago',
'sep'           => 'set',
'oct'           => 'otu',
'nov'           => 'nob',
'dec'           => 'abi',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|Categoría|Categorías}}',
'category_header'                => 'Articlos en a categoría "$1"',
'subcategories'                  => 'Subcategorías',
'category-media-header'          => 'Contenius multimedia en a categoría "$1"',
'category-empty'                 => "''Ista categoría no tiene por agora garra articlo ni conteniu multimedia''",
'hidden-categories'              => '{{PLURAL:$1|Categoría amagata|Categorías amagatas}}',
'hidden-category-category'       => 'Categorías amagatas', # Name of the category where hidden categories will be listed
'category-subcat-count'          => "{{PLURAL:$2|Ista categoría contiene nomás a siguient subcategoría.|Ista categoría encluye {{PLURAL:$1|a siguient subcategoría|as siguients $1 subcategorías}}, d'un total de $2.}}",
'category-subcat-count-limited'  => 'Ista categoría contiene {{PLURAL:$1|a siguient subcategoría|as siguients $1 subcategorías}}.',
'category-article-count'         => "{{PLURAL:$2|Ista categoría nomás encluye a pachina siguient.|{{PLURAL:$1|A pachina siguient fa parte|As pachinas siguients fan parte}} d'esta categoría, d'un total de $2.}}",
'category-article-count-limited' => "{{PLURAL:$1|A pachina siguient fa parte|As $1 pachinas siguients fan parte}} d'ista categoría.",
'category-file-count'            => "{{PLURAL:$2|Ista categoría nomás contiene l'archibo siguient.|{{PLURAL:$1|L'archibo siguient fa parte|Os $1 archibos siguients fan parte}} d'ista categoría, d'un total de $2.}}",
'category-file-count-limited'    => "{{PLURAL:$1|L'archibo siguient fa parte|Os $1 archibos siguients fan parte}} d'ista categoría.",
'listingcontinuesabbrev'         => 'cont.',

'mainpagetext'      => "O programa MediaWiki s'ha instalato correutament.",
'mainpagedocfooter' => "Consulta a [http://meta.wikimedia.org/wiki/Help:Contents Guía d'usuario] ta mirar informazión sobre cómo usar o software wiki.

== Ta prenzipiar ==

* [http://www.mediawiki.org/wiki/Manual:Configuration_settings Lista de carauteristicas confegurables]
* [http://www.mediawiki.org/wiki/Manual:FAQ Preguntas cutianas sobre MediaWiki (FAQ)]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce Lista de correu sobre ta anunzios de MediaWiki]",

'about'          => 'Informazión sobre',
'article'        => 'Articlo',
'newwindow'      => "(s'ubre en una nueba finestra)",
'cancel'         => 'Anular',
'qbfind'         => 'Mirar',
'qbbrowse'       => 'Nabegar',
'qbedit'         => 'Editar',
'qbpageoptions'  => 'Ista pachina',
'qbpageinfo'     => "Informazión d'a pachina",
'qbmyoptions'    => 'Pachinas propias',
'qbspecialpages' => 'Pachinas espezials',
'moredotdotdot'  => 'Más...',
'mypage'         => 'A mía pachina',
'mytalk'         => 'Pachina de descusión',
'anontalk'       => "Pachina de descusión d'ista IP",
'navigation'     => 'Nabego',
'and'            => 'y',

# Metadata in edit box
'metadata_help' => 'Metadatos:',

'errorpagetitle'    => 'Error',
'returnto'          => 'Tornar ta $1.',
'tagline'           => 'De {{SITENAME}}',
'help'              => 'Aduya',
'search'            => 'Mirar',
'searchbutton'      => 'Mirar-lo',
'go'                => 'Ir-ie',
'searcharticle'     => 'Ir-ie',
'history'           => 'Istorial de cambeos',
'history_short'     => 'Istorial',
'updatedmarker'     => 'esbiellato dende a zaguera besita',
'info_short'        => 'Informazión',
'printableversion'  => 'Bersión ta imprentar',
'permalink'         => 'Binclo permanent',
'print'             => 'Imprentar',
'edit'              => 'Editar',
'create'            => 'Creyar',
'editthispage'      => 'Editar ista pachina',
'create-this-page'  => 'Creyar ista pachina',
'delete'            => 'Borrar',
'deletethispage'    => 'Borrar ista pachina',
'undelete_short'    => 'Restaurar {{PLURAL:$1|una edizión|$1 edizions}}',
'protect'           => 'Protecher',
'protect_change'    => 'cambiar a protezión',
'protectthispage'   => 'Protecher ista pachina',
'unprotect'         => 'esprotecher',
'unprotectthispage' => 'Esprotecher ista pachina',
'newpage'           => 'Pachina nueba',
'talkpage'          => "Descusión d'ista pachina",
'talkpagelinktext'  => 'Descutir',
'specialpage'       => 'Pachina Espezial',
'personaltools'     => 'Ainas presonals',
'postcomment'       => 'Adibir un comentario',
'articlepage'       => "Beyer l'articlo",
'talk'              => 'Descusión',
'views'             => 'Bisualizazions',
'toolbox'           => 'Ainas',
'userpage'          => "Beyer a pachina d'usuario",
'projectpage'       => "Beyer a pachina d'o procheuto",
'imagepage'         => "Beyer a pachina de l'archibo",
'mediawikipage'     => "Beyer a pachina d'o mensache",
'templatepage'      => "Beyer a pachina d'a plantilla",
'viewhelppage'      => "Beyer a pachina d'aduya",
'categorypage'      => "Beyer a pachina d'a categoría",
'viewtalkpage'      => 'Beyer a pachina de descusión',
'otherlanguages'    => 'En atras luengas',
'redirectedfrom'    => '(Reendrezato dende $1)',
'redirectpagesub'   => 'Pachina reendrezata',
'lastmodifiedat'    => "Zaguera edizión d'ista pachina: $2, $1.", # $1 date, $2 time
'viewcount'         => 'Ista pachina ha tenito {{PLURAL:$1|una besita|$1 besitas}}.',
'protectedpage'     => 'Pachina protechita',
'jumpto'            => 'Ir ta:',
'jumptonavigation'  => 'nabego',
'jumptosearch'      => 'busca',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'Informazión sobre {{SITENAME}}',
'aboutpage'            => 'Project:Sobre',
'bugreports'           => "Informes d'errors d'o software",
'bugreportspage'       => "Project:Informes d'errors",
'copyright'            => 'O conteniu ye disponible baxo a lizenzia $1.',
'copyrightpagename'    => "Dreitos d'autor de {{SITENAME}}",
'copyrightpage'        => "{{ns:project}}:Dreitos d'autor",
'currentevents'        => 'Autualidat',
'currentevents-url'    => 'Project:Autualidat',
'disclaimers'          => 'Abiso legal',
'disclaimerpage'       => 'Project:Abiso legal',
'edithelp'             => 'Aduya ta editar pachinas',
'edithelppage'         => "Help:Cómo s'edita una pachina",
'faq'                  => 'Preguntas cutianas',
'faqpage'              => 'Project:Preguntas cutianas',
'helppage'             => 'Help:Aduya',
'mainpage'             => 'Portalada',
'mainpage-description' => 'Portalada',
'policy-url'           => 'Project:Politicas y normas',
'portal'               => "Portal d'a comunidat",
'portal-url'           => "Project:Portal d'a comunidat",
'privacy'              => 'Politica de pribazidat',
'privacypage'          => 'Project:Politica de pribazidat',

'badaccess'        => 'Error de premisos',
'badaccess-group0' => "No tiene premisos ta fer l'aizión que ha demandato.",
'badaccess-group1' => "Ista aizión que ha demandato nomás ye premitita ta os usuarios d'a colla $1.",
'badaccess-group2' => "Ista aizión nomás ye premitita ta usuarios de beluna d'istas collas: $1.",
'badaccess-groups' => "L'aizión que ha demandato nomás ye premitita ta os usuarios de beluna d'as collas: $1.",

'versionrequired'     => 'Cal a bersión $1 de MediaWiki',
'versionrequiredtext' => 'Cal a bersión $1 de MediaWiki ta fer serbir ista pachina. Ta más informazión, consulte [[Special:Version]]',

'ok'                      => "D'alcuerdo",
'retrievedfrom'           => 'Otenito de "$1"',
'youhavenewmessages'      => 'Tiene $1 ($2).',
'newmessageslink'         => 'mensaches nuebos',
'newmessagesdifflink'     => 'Esferenzias con a bersión anterior',
'youhavenewmessagesmulti' => 'Tiene nuebos mensaches en $1',
'editsection'             => 'editar',
'editold'                 => 'editar',
'viewsourceold'           => 'beyer codigo fuent',
'editsectionhint'         => 'Editar a sezión: $1',
'toc'                     => 'Contenius',
'showtoc'                 => 'amostrar',
'hidetoc'                 => 'amagar',
'thisisdeleted'           => 'Quiere amostrar u restaurar $1?',
'viewdeleted'             => 'Quiere amostrar $1?',
'restorelink'             => '{{PLURAL:$1|una edizión borrata|$1 edizions borratas}}',
'feedlinks'               => 'Sendicazión como fuent de notizias:',
'feed-invalid'            => 'Sendicazión como fuent de notizias no conforme.',
'feed-unavailable'        => 'As fuents de sendicazión no son disponibles en {{SITENAME}}',
'site-rss-feed'           => 'Canal RSS $1',
'site-atom-feed'          => 'Canal Atom $1',
'page-rss-feed'           => 'Canal RSS "$1"',
'page-atom-feed'          => 'Canal Atom "$1"',
'red-link-title'          => '$1 (encara no escrita)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Articlo',
'nstab-user'      => "Pachina d'usuario",
'nstab-media'     => 'Pachina multimedia',
'nstab-special'   => 'Espezial',
'nstab-project'   => "Pachina d'o proyeuto",
'nstab-image'     => 'Imachen',
'nstab-mediawiki' => 'Mensache',
'nstab-template'  => 'Plantilla',
'nstab-help'      => 'Aduya',
'nstab-category'  => 'Categoría',

# Main script and global functions
'nosuchaction'      => 'No se reconoxe ista aizión',
'nosuchactiontext'  => "{{SITENAME}} no reconoxe l'aizión espezificata en l'adreza URL",
'nosuchspecialpage' => 'No esiste ixa pachina espezial',
'nospecialpagetext' => "<big>'''A pachina espezial que ha demandato no esiste.'''</big>

Puede trobar una lista de pachinas espezials en [[Special:SpecialPages|{{int:specialpages}}]].",

# General errors
'error'                => 'Error',
'databaseerror'        => "Error d'a base de datos",
'dberrortext'          => 'Ha escaizito una error de sintacsis en una consulta á la base de datos.
Isto podría endicar una error en o programa.
A zaguera consulta que se miró de fer estió: <blockquote><tt>$1</tt></blockquote> aintro d\'a funzión "<tt>$2</tt>". A error tornata por a base de datos estió "<tt>$3: $4</tt>".',
'dberrortextcl'        => 'Ha escaizito una error de sintacsis en una consulta á la base de datos. A zaguera consulta que se miró de fer estió:
"$1"
aintro d\'a funzión "$2".
A base de datos retornó a error "<tt>$3: $4</tt>".',
'noconnect'            => "A wiki tiene agora bellas dificultaz tecnicas, y no se podió contautar con o serbidor d'a base de datos. <br />
$1",
'nodb'                 => 'No se podió trigar a base de datos $1',
'cachederror'          => "Ista ye una copia en caché d'a pachina demandata, y puestar que no siga esbiellata.",
'laggedslavemode'      => "Pare cuenta: podrían faltar as zagueras edizions d'ista pachina.",
'readonly'             => 'Base de datos bloqueyata',
'enterlockreason'      => "Esplique a causa d'o bloqueyo, encluyendo una estimazión de cuán se produzirá o desbloqueyo",
'readonlytext'         => "A base de datos de {{SITENAME}} ye bloqueyata temporalment, probablement por mantenimiento rutinario, dimpués d'ixo tornará á la normalidat.
L'almenistrador que la bloqueyó dió ista esplicazión:
<p>$1",
'missing-article'      => "No s'ha trobato en a base de datos o testo d'una pachina que abría d'estar-ie: \"\$1\" \$2.

Cal que a razón d'isto siga que s'ha seguito un diff masiau antigo u un binclo ta l'istorial d'una pachina que ya s'ha borrato.

Si no ye iste o caso, puede que aiga trobato un error en o software. Por fabor, informe d'isto á un [[Special:ListUsers/sysop|almenistrador]], endicando-le a URL.",
'missingarticle-rev'   => '(bersión#: $1)',
'missingarticle-diff'  => '(Esf: $1, $2)',
'readonly_lag'         => 'A base de datos ye bloqueyata temporalment entre que os serbidors se sincronizan.',
'internalerror'        => 'Error interna',
'internalerror_info'   => 'Error interna: $1',
'filecopyerror'        => 'No s\'ha puesto copiar l\'archibo "$1" ta "$2".',
'filerenameerror'      => 'No s\'ha puesto cambiar o nombre de l\'archibo "$1" á "$2".',
'filedeleteerror'      => 'No s\'ha puesto borrar l\'archibo "$1".',
'directorycreateerror' => 'No s\'ha puesto crear o direutorio "$1".',
'filenotfound'         => 'No s\'ha puesto trobar l\'archibo "$1".',
'fileexistserror'      => 'No s\'ha puesto escribir en l\'archibo "$1": l\'archibo ya esiste',
'unexpected'           => 'Balura no prebista: "$1"="$2".',
'formerror'            => 'Error: no se podió nimbiar o formulario',
'badarticleerror'      => 'Ista aizión no se puede no se puede reyalizar en ista pachina.',
'cannotdelete'         => "No se podió borrar a pachina u l'archibo espezificato. (Puestar que belatro usuario l'aiga borrato dinantes)",
'badtitle'             => 'Títol incorreuto',
'badtitletext'         => "O títol d'a pachina demandata ye buedo, incorreuto, u tiene un binclo interwiki mal feito. Puede contener uno u más carauters que no se pueden fer serbir en títols.",
'perfdisabled'         => "S'ha desautibato temporalment ista opzión porque fa lenta a base de datos de traza que dengún no puede usar o wiki.",
'perfcached'           => 'Os datos que siguen son en caché, y podrían no estar esbiellatos:',
'perfcachedts'         => 'Istos datos se troban en a caché, que estió esbiellata por zaguer begada o $1.',
'querypage-no-updates' => "S'han desautibato as autualizazions d'ista pachina. Por ixo, no s'esta esbiellando os datos.",
'wrong_wfQuery_params' => 'Parametros incorreutos ta wfQuery()<br />
Funzión: $1<br />
Consulta: $2',
'viewsource'           => 'Beyer codigo fuen',
'viewsourcefor'        => 'ta $1',
'actionthrottled'      => 'Aizión afogata',
'actionthrottledtext'  => "Como mida anti-spam, bi ha un limite en o numero de begadas que puede fer ista aizión en un curto espazio de tiempo, y ha brincato d'iste limite. Aspere bels menutos y prebe de fer-lo nuebament.",
'protectedpagetext'    => 'Ista pachina ha estato protechita ta aprebenir a suya edizión.',
'viewsourcetext'       => "Puede beyer y copiar o codigo fuent d'ista pachina:",
'protectedinterface'   => "Ista pachina furne o testo d'a interfaz ta o software. Ye protechita ta pribar o bandalismo. Si creye que bi ha bella error, contaute con un almenistrador.",
'editinginterface'     => "'''Pare cuenta:''' Ye editando una pachina emplegata ta furnir o testo d'a interfaz de {{SITENAME}}. Os cambeos en ista pachina tendrán efeuto en l'aparenzia d'a interfaz ta os atros usuarios. Ta fer traduzions d'a interfaz, puede considerar fer serbir [http://translatewiki.net/wiki/Main_Page?setlang=an Betawiki], o procheuto de localizazión de MediaWiki.",
'sqlhidden'            => '(Consulta SQL amagata)',
'cascadeprotected'     => 'Ista pachina ye protechita y no se puede editar porque ye encluyita en {{PLURAL:$1|a siguient pachina|as siguients pachinas}}, que son protechitas con a opzión de "cascada": $2',
'namespaceprotected'   => "No tiene premiso ta editar as pachinas d'o espazio de nombres '''$1'''.",
'customcssjsprotected' => "No tiene premiso ta editar ista pachina porque contiene a confegurazión presonal d'atro usuario.",
'ns-specialprotected'  => "No ye posible editar as pachinas d'o espazio de nombres {{ns:special}}.",
'titleprotected'       => "Iste títol no puede creyar-se porque ye estato protechito por [[User:$1|$1]].
A razón data ye ''$2''.",

# Virus scanner
'virus-badscanner'     => 'Confegurazión incorreuta: rastriador de birus esconoixito: <i>$1</i>',
'virus-scanfailed'     => 'o rastreyo ha fallato (codigo $1)',
'virus-unknownscanner' => 'antibirus esconoixito:',

# Login and logout pages
'logouttitle'                => "Fin d'a sesión",
'logouttext'                 => "<strong>Ha rematato a sesión.</strong>

Puede continar nabegando por {{SITENAME}} anonimament, u puede [[Special:UserLogin|enzetar]] una nueba sesión con o mesmo nombre d'usuario u unatro diferent. Pare cuenta que, entre que se limpia a caché d'o nabegador, puet estar que bellas pachinas s'amuestren como si encara continase en a sesión anterior.",
'welcomecreation'            => "== ¡Bienbeniu(da), $1! ==
S'ha creyato a suya cuenta.
No xublide presonalizar [[Special:Preferences|as suyas preferenzias en {{SITENAME}}]].",
'loginpagetitle'             => 'Enzetar a sesión',
'yourname'                   => "Nombre d'usuario:",
'yourpassword'               => 'Palabra de paso:',
'yourpasswordagain'          => 'Torne á escribir a palabra de paso:',
'remembermypassword'         => "Remerar datos d'usuario entre sesions.",
'yourdomainname'             => 'Dominio:',
'externaldberror'            => "Bi abió una error d'autenticazión esterna d'a base de datos u bien no tiene premisos ta esbiellar a suya cuenta esterna.",
'loginproblem'               => '<b>Escaizió un problema con a suya autenticazión.</b><br />¡Prebe unatra begada!',
'login'                      => 'Enzetar sesión',
'nav-login-createaccount'    => 'Enzetar una sesión / creyar cuenta',
'loginprompt'                => "Ta rechistrar-se en {{SITENAME}} ha d'autibar as cookies en o nabegador.",
'userlogin'                  => 'Enzetar una sesión / creyar cuenta',
'logout'                     => "Salir d'a sesión",
'userlogout'                 => 'Salir',
'notloggedin'                => 'No ha dentrato en o sistema',
'nologin'                    => 'No tiene garra cuenta? $1.',
'nologinlink'                => 'Creyar una nueba cuenta',
'createaccount'              => 'Creyar una nueba cuenta',
'gotaccount'                 => 'Tiene ya una cuenta? $1.',
'gotaccountlink'             => 'Identificar-se y enzetar sesión',
'createaccountmail'          => 'por correu electronico',
'badretype'                  => 'As palabras de paso que ha escrito no son iguals.',
'userexists'                 => 'Ixe nombre ya ye en uso. Por fabor, meta un nombre diferent.',
'youremail'                  => 'Adreza de correu electronico:',
'username'                   => "Nombre d'usuario:",
'uid'                        => "ID d'usuario:",
'prefs-memberingroups'       => "Miembro {{PLURAL:$1|d'a colla|d'as collas}}:",
'yourrealname'               => 'Nombre reyal:',
'yourlanguage'               => 'Luenga:',
'yourvariant'                => 'Modalidat linguistica:',
'yournick'                   => 'Siñadura:',
'badsig'                     => 'A suya siñadura no ye conforme; comprebe as etiquetas HTML.',
'badsiglength'               => 'A siñadura ye masiau larga. No abría de tener más de $1 {{PLURAL:$1|caráuter|caráuters}}.',
'email'                      => 'Adreza de correu-e',
'prefs-help-realname'        => "* Nombre reyal (opzional): si esliche escribir-lo, se ferá serbir ta l'atribuzión d'a suya faina.",
'loginerror'                 => 'Error en enzetar a sesión',
'prefs-help-email'           => "Adreza de correu-e (opzional): Premite á atros usuarios nimbiar-le correus electronicos por meyo de a suya pachina d'usuario u de descusión d'usuario sin d'aber de rebelar a suya identidá.",
'prefs-help-email-required'  => 'Cal una adreza de correu-e.',
'nocookiesnew'               => "A cuenta d'usuario s'ha creyata, pero encara no ye indentificato. {{SITENAME}} fa serbir <em>cookies</em> ta identificar á os usuario rechistratos, pero pareix que las tiene desautibatas. Por fabor, autibe-las e identifique-se con o suyo nombre d'usuario y palabra de paso.",
'nocookieslogin'             => "{{SITENAME}} fa serbir <em>cookies</em> ta la identificazión d'usuarios. Tiene as <em>cookies</em> desautibatas en o nabegador. Por fabor, autibe-las y prebe á identificar-se de nuebas.",
'noname'                     => "No ha escrito un nombre d'usuario correuto.",
'loginsuccesstitle'          => "S'ha identificato correutament",
'loginsuccess'               => 'Ha enzetato una sesión en {{SITENAME}} como "$1".',
'nosuchuser'                 => 'No bi ha garra usuario clamato "$1".
Comprebe si ha escrito bien o nombre u creye una nueba cuenta d\'usuario.',
'nosuchusershort'            => 'No bi ha garra usuario con o nombre "<nowiki>$1</nowiki>". Comprebe si o nombre ye bien escrito.',
'nouserspecified'            => "Ha d'escribir un nombre d'usuario.",
'wrongpassword'              => 'A palabra de paso endicata no ye correuta. Prebe unatra begada.',
'wrongpasswordempty'         => 'No ha escrito garra palabra de paso. Prebe unatra begada.',
'passwordtooshort'           => "A suya palabra de paso no ye conforme u ye masiau curta. Ha de tener como menimo {{PLURAL:$1|1 caráuter|$1 caráuters}} y no puede estar o suyo nombre d'usuario.",
'mailmypassword'             => 'Nimbía-me una nueba palabra de paso por correu electronico',
'passwordremindertitle'      => 'Nueba palabra de paso temporal de {{SITENAME}}',
'passwordremindertext'       => 'Belún (probablement busté, dende l\'adreza IP $1) demandó que li nimbiásenos una nueba palabra de paso ta la suya cuenta en {{SITENAME}} ($4).
A palabra de paso ta l\'usuario "$2" ye agora "$3".
Li consellamos que enzete agora una sesión y cambee a suya palabra de paso.

Si iste mensache fue demandato por otri, u si ya se\'n ha alcordato d\'a palabra de paso y ya no deseya cambiar-la, puede innorar iste mensache y continar fendo serbir l\'antiga palabra de paso.',
'noemail'                    => 'No bi ha garra adreza de correu electronico rechistrada ta "$1".',
'passwordsent'               => 'Una nueba palabra de paso plega de nimbiar-se ta o correu electronico de "$1".
Por fabor, identifique-se unatra bez malas que la reculla.',
'blocked-mailpassword'       => "A suya adreza IP ye bloqueyata y, ta pribar abusos, no se li premite emplegar d'a funzión de recuperazión de palabras de paso.",
'eauthentsent'               => "S'ha nimbiato un correu electronico de confirmazión ta l'adreza espezificata. Antes que no se nimbíe dengún atro correu ta ixa cuenta, ha de confirmar que ixa adreza te pertenexe. Ta ixo, cal que siga as instruzions que trobará en o mensache.",
'throttled-mailpassword'     => "Ya s'ha nimbiato un correu recordatorio con a suya palabra de paso fa menos de {{PLURAL:$1|1 ora|$1 oras}}. Ta escusar abusos, nomás se nimbia un recordatorio cada {{PLURAL:$1|ora|$1 oras}}.",
'mailerror'                  => 'Error en nimbiar o correu: $1',
'acct_creation_throttle_hit' => 'Lo sentimos, ya ha creyato $1 cuentas. No puede creyar más cuentas.',
'emailauthenticated'         => 'A suya adreza de correu-e estió confirmata o $1.',
'emailnotauthenticated'      => "A suya adreza de correu-e <strong> no ye encara confirmata </strong>. No podrá recullir garra correu t'as siguients funzions.",
'noemailprefs'               => '<strong>Escriba una adreza de correu-e ta autibar istas carauteristicas.</strong>',
'emailconfirmlink'           => 'Confirme a suya adreza de correu-e',
'invalidemailaddress'        => "L'adreza de correu-e no puede estar azeutata pues tiene un formato incorreuto. Por favor, escriba una adreza bien formateyata, u dixe buedo ixe campo.",
'accountcreated'             => 'Cuenta creyata',
'accountcreatedtext'         => "S'ha creyato a cuenta d'usuario de $1.",
'createaccount-title'        => 'Creyar una cuenta en {{SITENAME}}',
'createaccount-text'         => 'Belún ha creyato una cuenta con o nombre "$2" en {{SITENAME}} ($4), con a palabra de paso "$3" y endicando a suya adreza de correu. Abría de dentrar-ie agora y cambiar a suya palabra de paso.

Si a cuenta s\'ha creyato por error, simplament innore iste mensache.',
'loginlanguagelabel'         => 'Idioma: $1',

# Password reset dialog
'resetpass'               => "Restablir a palabra de paso d'a cuenta d'usuario",
'resetpass_announce'      => 'Ha enzetato una sesión con una palabra de paso temporal que fue nimbiata por correu electronico. Por fabor, escriba aquí una nueba palabra de paso:',
'resetpass_text'          => '<!-- Adiba aquí o testo -->',
'resetpass_header'        => 'Restablir a palabra de paso',
'resetpass_submit'        => 'Cambiar a palabra de paso e identificar-se',
'resetpass_success'       => 'A suya palabra de paso ya ye cambiata. Agora ya puede dentrar-ie...',
'resetpass_bad_temporary' => "A palabra de paso temporal no ye conforme. Puede estar que ya aiga cambiato a suya palabra de paso u que aiga demandato o nimbío d'un atra.",
'resetpass_forbidden'     => 'No se pueden cambiar as palabras de paso en {{SITENAME}}',
'resetpass_missing'       => 'No ha escrito datos en o formulario.',

# Edit page toolbar
'bold_sample'     => 'Testo en negreta',
'bold_tip'        => 'Testo en negreta',
'italic_sample'   => 'Testo en cursiba',
'italic_tip'      => 'Testo en cursiba',
'link_sample'     => "Títol d'o binclo",
'link_tip'        => 'Binclo interno',
'extlink_sample'  => "http://www.example.com Títol d'o binclo",
'extlink_tip'     => "Binclo esterno (alcuerde-se d'adibir o prefixo http://)",
'headline_sample' => 'Testo de tetular',
'headline_tip'    => 'Tetular de libel 2',
'math_sample'     => 'Escriba aquí a formula',
'math_tip'        => 'Formula matematica (LaTeX)',
'nowiki_sample'   => 'Escriba aquí testo sin de formato',
'nowiki_tip'      => 'Inorar o formato wiki',
'image_sample'    => 'Exemplo.jpg',
'image_tip'       => 'Imachen encrustata',
'media_sample'    => 'Exemplo.ogg',
'media_tip'       => "Binclo ta l'archibo",
'sig_tip'         => 'Siñadura, calendata y ora',
'hr_tip'          => 'Linia orizontal (en faiga un emplego amoderau)',

# Edit pages
'summary'                          => 'Resumen',
'subject'                          => 'Tema/títol',
'minoredit'                        => 'He feito una edizión menor',
'watchthis'                        => 'Cosirar ista pachina',
'savearticle'                      => 'Alzar pachina',
'preview'                          => 'Bisualizazión prebia',
'showpreview'                      => 'Bisualizazión prebia',
'showlivepreview'                  => 'Ambiesta prebia rapeda',
'showdiff'                         => 'Amostrar cambeos',
'anoneditwarning'                  => "''Pare cuenta:'' No s'ha identificato con un nombre d'usuario. A suya adreza IP s'alzará en o istorial d'a pachina.",
'missingsummary'                   => "'''Pare cuenta:''' No ha escrito garra resumen d'edizión. Si fa clic nuebament en «{{MediaWiki:Savearticle}}» a suya edizión se grabará sin resumen.",
'missingcommenttext'               => 'Por fabor, escriba o testo astí baxo.',
'missingcommentheader'             => "'''Pare cuenta:''' No ha escrito garra títol ta iste comentario. Si puncha un atra bez en con a rateta en \"Alzar\", a suya edizión se grabará sin títol.",
'summary-preview'                  => "Beyer ambiesta prebia d'o resumen",
'subject-preview'                  => "Ambiesta prebia d'o tema/títol",
'blockedtitle'                     => "L'usuario ye bloqueyato",
'blockedtext'                      => "<big>'''O suyo nombre d'usuario u adreza IP ye bloqueyato.'''</big>

O bloqueyo lo fazió $1. 
A razón data ye ''$2''.

* Prenzipio d'o bloqueyo: $8
* Fin d'o bloqueyo: $6
* Indentificazión bloqueyata: $7

Puede contautar con $1 u con atro [[{{MediaWiki:Grouppage-sysop}}|almenistrador]] ta letigar sobre o bloqueyo.
No puede fer serbir o binclo 'nimbiar correu electronico ta iste usuario' si no ha rechistrato una adreza conforme de correu electronico en as suyas [[Special:Preferences|preferenzias]] y si no se l'ha bedau d'emplegar-la. A suya adreza IP autual ye $3, y o identificador d'o bloqueyo ye #$5. Por fabor encluiga ixos datos cuan faga cualsiquier consulta.",
'autoblockedtext'                  => "A suya adreza IP s'ha bloqueyata automaticament porque la eba feito serbir un atro usuario bloqueyato por \$1.

A razón d'o bloqueyo ye ista:

:''\$2''

* Prenzipio d'o bloqueyo: \$8
* Fin d'o bloqueyo: \$6
* Usuario que se prebaba de bloqueyar: \$7

Puede contautar con \$1 u con atro d'os [[{{MediaWiki:Grouppage-sysop}}|almenistradors]] ta litigar sobre o bloqueyo.

Pare cuenta que no puede emplegar a funzión \"Nimbiar correu electronico ta iste usuario\" si no tiene una adreza de correu electronico conforme rechistrada en as suyas [[Special:Preferences|preferenzias d'usuario]] u si se l'ha bedato d'emplegar ista funzión.

A suya adreza IP autual ye \$3, y o identificador de bloqueyo ye #\$5. Por fabor encluiga os datos anteriors cuan faga cualsiquier consulta.",
'blockednoreason'                  => "No s'ha dato garra causa",
'blockedoriginalsource'            => "Contino s'amuestra o codigo fuent de  '''$1''':",
'blockededitsource'                => "Contino s'amuestra o testo d'as suyas '''edizions''' á '''$1''':",
'whitelistedittitle'               => 'Cal enzetar una sesión ta poder editar.',
'whitelistedittext'                => 'Ha de $1 ta poder editar pachinas.',
'confirmedittitle'                 => 'Cal que confirme a suya adreza de correu-e ta poder editar',
'confirmedittext'                  => "Ha de confirmar a suya adreza de correu-e antis de poder editar pachinas. Por fabor, establa y confirme una adreza de correu-e a trabiés d'as suyas [[Special:Preferences|preferenzias d'usuario]].",
'nosuchsectiontitle'               => 'No esiste ixa sezión',
'nosuchsectiontext'                => "Has prebato d'editar una sezión que no existe. Como no bi ha sezión $1, as suyas edizions no se pueden alzar en garra puesto.",
'loginreqtitle'                    => 'Cal que enzete una sesión',
'loginreqlink'                     => 'enzetar una sesión',
'loginreqpagetext'                 => 'Ha de $1 ta beyer atras pachinas.',
'accmailtitle'                     => 'A palabra de paso ha estato nimbiata.',
'accmailtext'                      => "A palabra de paso de '$1' s'ha nimbiato á $2.",
'newarticle'                       => '(Nuebo)',
'newarticletext'                   => "Ha siguito un binclo ta una pachina que encara no esiste.
Ta creyar a pachina, prenzipie á escribir en a caxa d'abaxo
(mire-se l'[[{{MediaWiki:Helppage}}|aduya]] ta más informazión).
Si bi ha plegau por error, punche o botón d'o suyo nabegador ta tornar entazaga.",
'anontalkpagetext'                 => "----''Ista ye a pachina de descusión d'un usuario anonimo que encara no ha creyato una cuenta, u no l'ha feito serbir. Por ixo, emos d'emplegar a suya adreza IP ta identificar-lo/a. 
Barios usuarios pueden compartir una mesma adreza IP. 
Si busté ye un usuario anonimo y creye que l'han escrito comentarios no relebants, [[Special:UserLogin/signup|creye una cuenta]] u [[Special:UserLogin/signup|identifique-se]] ta pribar confusions futuras con atros usuarios anonimos.''",
'noarticletext'                    => 'Por agora no bi ha testo en ista pachina. Puede [[Special:Search/{{PAGENAME}}|mirar o títol]] en atras pachinas u [{{fullurl:{{FULLPAGENAME}}|action=edit}} prenzipiar á escribir en ista pachina].',
'userpage-userdoesnotexist'        => 'A cuenta d\'usuario "$1" no ye rechistrada. Piense si quiere creyar u editar ista pachina.',
'clearyourcache'                   => "'''Pare cuenta: Si quiere beyer os cambeos dimpués d'alzar l'archibo, puede estar que tienga que refrescar a caché d'o suyo nabegador ta beyer os cambeos.''' '''Mozilla / Firefox / Safari:''' prete a tecla de ''Mayusclas'' mientras puncha ''Reload,'' u prete '''Ctrl-Fr''' u '''Ctrl-R''' (''Command-R'' en un Macintosh); '''Konqueror: ''' punche ''Reload'' u prete ''F5;'' '''Opera:''' limpiar a caché en ''Tools → Preferences;'' '''Internet Explorer:''' prete ''Ctrl'' mientres puncha ''Refresh,'' u prete ''Ctrl-F5.''",
'usercssjsyoucanpreview'           => '<strong>Consello:</strong> Faga serbir o botón «Amostrar prebisualizazión» ta prebar o nuebo css/js antes de grabar-lo.',
'usercsspreview'                   => "'''Remere que sólo ye prebisualizando o suyo css d'usuario y encara no ye grabato!'''",
'userjspreview'                    => "'''Remere que sólo ye prebisualizando o suyo javascript d'usuario y encara no ye grabato!'''",
'userinvalidcssjstitle'            => "'''Pare cuenta:''' No bi ha garra aparenzia clamata \"\$1\". Remere que as pachinas presonalizatas .css y .js tienen un títol en minusclas, p.e. {{ns:user}}:Foo/monobook.css en cuenta de {{ns:user}}:Foo/Monobook.css.",
'updated'                          => '(Esbiellato)',
'note'                             => '<strong>Nota:</strong>',
'previewnote'                      => "<strong>Pare cuenta que isto sólo ye que l'ambiesta prebia d'a pachina; os cambeos encara no han estato alzatos!</strong>",
'previewconflict'                  => "L'ambiesta prebia li amostrará l'aparenzia d'o testo dimpués d'alzar os cambeos.",
'session_fail_preview'             => "<strong>Ya lo sentimos, pero no emos puesto alzar a suya edizión por una perduga d'os datos de sesion. Por fabor, prebe de fer-lo una atra bez, y si encara no funziona, [[Special:UserLogout|salga d'a sesión]] y torne á identificar-se.</strong>",
'session_fail_preview_html'        => "<strong>Ya lo sentimos, pero no emos puesto prozesar a suya edizión porque os datos de sesión s'han trafegato.</strong>

''Como {{SITENAME}} tiene l'HTML puro autibato, s'ha amagato l'ambiesta prebia ta aprebenir ataques en JavaScript.''

<strong>Si ye mirando d'editar lechitimament, por fabor, prebe una atra bez. Si encara no funzionase alabez, prebe de [[Special:UserLogout|zarrar a sesión]] y dentrar-ie identificando-se de nuebas.</strong>",
'token_suffix_mismatch'            => "<strong>S'ha refusato a suya edizión porque o suyo client ha esbarafundiato os caráuters de puntuazión en o editor. A edizión s'ha refusata ta pribar a corrompizión d'a pachina de testo. Isto gosa escaizer cuan se fa serbir un serbizio de proxy defeutuoso alazetato en a web.</strong>",
'editing'                          => 'Editando $1',
'editingsection'                   => 'Editando $1 (sezión)',
'editingcomment'                   => 'Editando $1 (comentario)',
'editconflict'                     => "Conflito d'edizión: $1",
'explainconflict'                  => "Bel atro usuario ha cambiato ista pachina dende que bustet prenzipió á editar-la.
O cuatrón de testo superior contiene o testo d'a pachina como ye autualment.
Os suyos cambeos s'amuestran en o cuatrón de testo inferior.
Abrá d'encorporar os suyos cambeos en o testo esistent.
'''Nomás''' o testo en o cuatrón superior s'alzará cuan prete o botón \"Alzar a pachina\".",
'yourtext'                         => 'O testo suyo',
'storedversion'                    => 'Bersión almadazenata',
'nonunicodebrowser'                => "<strong>Pare cuenta: O suyo nabegador no cumple a norma Unicode. S'ha autibato un sistema d'edizión alternatibo que li premitirá d'editar articlos con seguridat: os caráuters no ASCII aparixerán en a caxa d'edizión como codigos exadezimals.</strong>",
'editingold'                       => "<strong>PARE CUENTA: Ye editando una bersión antiga d'ista pachina. Si alza a pachina, toz os cambeos feitos dende ixa rebisión se tresbatirán.</strong>",
'yourdiff'                         => 'Esferenzias',
'copyrightwarning'                 => "Por fabor, pare cuenta que todas as contrebuzions á {{SITENAME}} se consideran publicatas baxo a lizenzia $2 (beyer detalles en $1). Si no deseya que atra chent corricha os suyos escritos sin piedat y los destribuiga librement, alabez, no debería meter-los aquí. En publicar aquí, tamién ye declarando que busté mesmo escribió iste testo y ye dueño d'os dreitos d'autor, u bien lo copió dende o dominio publico u cualsiquier atra fuent libre.
<strong>NO COPIE SIN PREMISO ESCRITOS CON DREITOS D'AUTOR!</strong><br />",
'copyrightwarning2'                => "Por fabor, pare cuenta que todas as contrebuzions á {{SITENAME}} pueden estar editatas, cambiatas u borratas por atros colaboradors. Si no deseya que atra chent corricha os suyos escritos sin piedat y los destribuiga librement, alabez, no debería meter-los aquí. <br /> En publicar aquí, tamién ye declarando que busté mesmo escribió iste testo y ye o dueño d'os dreitos d'autor, u bien lo copió dende o dominio publico u cualsiquier atra fuent libre (beyer $1 ta más informazión). <br />
<strong>NO COPIE SIN PREMISO ESCRITOS CON DREITOS D'AUTOR!</strong>",
'longpagewarning'                  => '<strong>Pare cuenta: Ista pachina tiene ya $1 kilobytes; bels nabegadors pueden tener problemas en editar pachinas de 32KB o más.
Considere, por fabor, a posibilidat de troxar ista pachina en trestallos más chicoz.</strong>',
'longpageerror'                    => '<strong>ERROR: O testo que ha escrito ye de $1 kilobytes, que ye mayor que a grandaria maisima de $2 kilobytes. No se puede alzar.</strong>',
'readonlywarning'                  => '<strong>Pare cuenta: A base de datos ha estato bloqueyata por custions de mantenimiento. Por ixo, en iste inte ye imposible alzar as suyas edizions. Puede copiar y apegar o testo en un archibo y alzar-lo ta dimpués.</strong>',
'protectedpagewarning'             => "<strong>PARE CUENTA: Ista pachina ha estato protechita ta que sólo os usuarios con premisos d'almenistrador puedan editar-la.</strong>",
'semiprotectedpagewarning'         => "'''Nota:''' Ista pachina ha estato protechita ta que nomás usuarios rechistratos puedan editar-la.",
'cascadeprotectedwarning'          => "'''Pare cuenta:''' Ista pachina ye protechita ta que nomás os almenistrador puedan editar-la, porque ye encluyita en {{PLURAL:$1|a siguient pachina, protechita|as siguients pachinas, protechitas}} con a opzión de ''cascada'' :",
'titleprotectedwarning'            => '<strong>PARE CUENTA:  Ista pachina ye bloqueyata ta que sólo bels usuarios puedan creyar-la.</strong>',
'templatesused'                    => 'Plantillas emplegatas en ista pachina:',
'templatesusedpreview'             => 'Plantillas emplegatas en ista ambiesta prebia:',
'templatesusedsection'             => 'Plantillas usatas en ista sezión:',
'template-protected'               => '(protechita)',
'template-semiprotected'           => '(semiprotechita)',
'hiddencategories'                 => 'Ista pachina fa parte de {{PLURAL:$1|1 categoría amagata|$1 categorías amagatas}}:',
'edittools'                        => "<!-- Iste testo amanixerá baxo os formularios d'edizión y carga. -->",
'nocreatetitle'                    => "S'ha restrinchito a creyazión de pachinas",
'nocreatetext'                     => '{{SITENAME}} ha restrinchito a creyazión de nuebas pachinas. Puede tornar entazaga y editar una pachina ya esistent, [[Special:UserLogin|identificarse u creyar una cuenta]].',
'nocreate-loggedin'                => 'No tiene premisos ta creyar nuebas pachinas en {{SITENAME}}.',
'permissionserrors'                => 'Errors de premisos',
'permissionserrorstext'            => 'No tiene premisos ta fer-lo, por {{PLURAL:$1|ista razón|istas razons}}:',
'permissionserrorstext-withaction' => 'No tiene premisos ta $2, por {{PLURAL:$1|ista razón|istas razons}}:',
'recreate-deleted-warn'            => "'''Pare cuenta: ye creyando una pachina que ya ha estato borrata denantes.'''

Abría de considerar si ye reyalment nezesario continar editando ista pachina.
Puede consultar o rechistro de borraus que s'amuestra a continuazión:",

# Parser/template warnings
'expensive-parserfunction-warning'        => 'Pare cuenta: Ista pachina tiene masiadas cridas á funzions de preprozeso (parser functions) costosas.

Abría de tener-ne menos de $2, por agora en tiene $1.',
'expensive-parserfunction-category'       => 'Pachinas con masiadas cridas á funzions de preprozeso (parser functions) costosas',
'post-expand-template-inclusion-warning'  => "Pare cuenta: A mida d'enclusión d'a plantilla ye masiau gran.
Bellas plantillas no se bi encluyen.",
'post-expand-template-inclusion-category' => "Pachinas an que se brinca a mida d'enclusión d'as plantillas",
'post-expand-template-argument-warning'   => "Pare cuenta: Ista pachina contiene á lo menos un argumento de plantilla con una mida d'espansión masiau gran. S'han omeso estos argumentos.",
'post-expand-template-argument-category'  => 'Pachinas con argumentos de plantilla omesos',

# "Undo" feature
'undo-success' => "A edizión puede esfer-se. Antis d'esfer a edizión, mire-se a siguient comparanza ta comprebar que ye ixo o que quiere fer reyalment. Alabez, puede alzar os cambeos ta esfer a edizión.",
'undo-failure' => 'No se puede esfer a edizión pues un atro usuario ha feito una edizión intermeya.',
'undo-norev'   => "No s'ha puesto esfer a edizión porque no esistiba u ya s'eba borrato.",
'undo-summary' => 'Esfeita a edizión $1 de [[Special:Contributions/$2|$2]] ([[User talk:$2|desc.]])',

# Account creation failure
'cantcreateaccounttitle' => 'No se puede creyar a cuenta',
'cantcreateaccount-text' => "A creyazión de cuentas dende ixa adreza IP ('''$1''') estió bloqueyata por [[User:$3|$3]].

A razón endicata por $3 ye ''$2''",

# History pages
'viewpagelogs'        => "Beyer os rechistros d'ista pachina",
'nohistory'           => "Ista pachina no tiene un istorial d'edizions.",
'revnotfound'         => 'Bersión no trobata',
'revnotfoundtext'     => "No se pudo trobar a bersión antiga d'a pachina demandata.
Por fabor, rebise l'adreza que fazió serbir t'aczeder á ista pachina.",
'currentrev'          => 'Bersión autual',
'revisionasof'        => "Bersión d'o $1",
'revision-info'       => "Bersión d'o $1 feita por $2",
'previousrevision'    => '← Bersión anterior',
'nextrevision'        => 'Bersión siguient →',
'currentrevisionlink' => 'Beyer bersión autual',
'cur'                 => 'aut',
'next'                => 'siguient',
'last'                => 'ant',
'page_first'          => 'primeras',
'page_last'           => 'zagueras',
'histlegend'          => 'Leyenda: (aut) = esferenzias con a bersión autual,
(ant) = diferenzias con a bersión anterior, m = edizión menor',
'deletedrev'          => '[borrato]',
'histfirst'           => 'Primeras contrebuzions',
'histlast'            => 'Zagueras',
'historysize'         => '({{PLURAL:$1|1 byte|$1 bytes}})',
'historyempty'        => '(buedo)',

# Revision feed
'history-feed-title'          => 'Istorial de bersions',
'history-feed-description'    => "Istorial de bersions d'ista pachina en o wiki",
'history-feed-item-nocomment' => '$1 en $2', # user at time
'history-feed-empty'          => "A pachina demandata no esiste.
Puede que aiga estato borrata d'o wiki u renombrata.
Prebe de [[Special:Search|mirar en o wiki]] atras pachinas relebants.",

# Revision deletion
'rev-deleted-comment'         => "(s'ha sacato iste comentario)",
'rev-deleted-user'            => "(s'ha sacato iste nombre d'usuario)",
'rev-deleted-event'           => "(Aizión borrata d'o rechistro)",
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">
Ista bersión d\'a pachina ye estata sacata d\'os archibos publicos.
Puede trobar más detalles en o [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} rechistro de borrau].</div>',
'rev-deleted-text-view'       => "<div class=\"mw-warning plainlinks\">
Ista bersión d'a pachina ye sacata d'os archibos publicos.
Puede beyer-la porque ye almenistrador/a d'iste wiki;
puede trobar más detalles en o [{{fullurl:Special:Log/delete|page={{PAGENAMEE}}}} rechistro de borrau].</div>",
'rev-delundel'                => 'amostrar/amagar',
'revisiondelete'              => 'Borrar/esfer borrau de bersions',
'revdelete-nooldid-title'     => 'A bersión de destino no ye conforme',
'revdelete-nooldid-text'      => 'No ha espezificato garra bersión de destino ta aplicar-le ista funzión, a bersión espezificata no esiste u ye mirando de amagar a bersión autual.',
'revdelete-selected'          => '{{PLURAL:$2|Bersión trigata|Bersions trigatas}} de [[:$1]]:',
'logdelete-selected'          => "{{PLURAL:$1|Escaizimiento d'o rechistro trigato|Escaizimientos d'o rechistro trigatos}}:",
'revdelete-text'              => "As bersions borratas encara aparixerán en o istorial y o rechistro d'a pachina, pero os suyos contenius no serán azesibles ta o publico.

Atros almenistradors de {{SITENAME}} encara podrán azeder t'o contineiu amagato y podrán esfer o borrau á trabiés d'a mesma interfaz, fueras de si os operadors establen restrizions adizionals.",
'revdelete-legend'            => 'Establir as restrizions de bisibilidat:',
'revdelete-hide-text'         => "Amagar o testo d'a bersión",
'revdelete-hide-name'         => 'Amagar aizión y obchetibo',
'revdelete-hide-comment'      => "Amagar comentario d'edizión",
'revdelete-hide-user'         => "Amagar o nombre/l'adreza IP d'o editor",
'revdelete-hide-restricted'   => 'Aplicar istas restrizions á os almenistradors y bloqueyar ista interfaz',
'revdelete-suppress'          => "Sacar os datos d'os almenistradors igual como os d'a resta d'usuarios",
'revdelete-hide-image'        => "Amagar o conteniu de l'archibo",
'revdelete-unsuppress'        => "Sacar restrizions d'as bersions restauradas",
'revdelete-log'               => "Comentario d'o rechistro:",
'revdelete-submit'            => 'Aplicar á la bersión trigata',
'revdelete-logentry'          => "S'ha cambiato a bisibilidat d'a bersión de [[$1]]",
'logdelete-logentry'          => "S'ha cambiato a bisibilidat d'escaizimientos de [[$1]]",
'revdelete-success'           => "'''S'ha cambiato correutament a bisibilidat d'as bersions.'''",
'logdelete-success'           => "'''S'ha cambiato correutament a bisibilidat d'os escaizimientos.'''",
'revdel-restore'              => 'Cambiar a bisibilidat',
'pagehist'                    => 'Istorial',
'deletedhist'                 => 'Istorial de borrau',
'revdelete-content'           => 'conteniu',
'revdelete-summary'           => 'editar resumen',
'revdelete-uname'             => "nombre d'usuario",
'revdelete-restricted'        => "S'han aplicato as restrizions ta almenistradors",
'revdelete-unrestricted'      => "S'han borrato as restrizions ta almenistradors",
'revdelete-hid'               => 'amagar $1',
'revdelete-unhid'             => 'amostrar $1',
'revdelete-log-message'       => '$1 ta $2 {{PLURAL:$2|bersión|bersions}}',
'logdelete-log-message'       => '$1 ta $2 {{PLURAL:$2|esdebenimiento|esdebenimientos}}',

# Suppression log
'suppressionlog'     => 'Rechistro de supresions',
'suppressionlogtext' => "En o cobaxo bi ye una lista de borraus y bloqueyos referitos á contenius amagaus ta os almenistradors. Mire-se a [[Special:IPBlockList|lista d'adrezas IP bloqueyatas]] ta beyer a lista de bloqueyos y bedas bichents.",

# History merging
'mergehistory'                     => 'Aunir istorials',
'mergehistory-header'              => "Ista pachina li premite aunir bersions d'o istorial d'una pachina d'orichen con una nueba pachina.
Asegure-se que iste cambio no crebará a continidat de l'istorial d'a pachina.",
'mergehistory-box'                 => 'Aunir as bersions de dos pachinas:',
'mergehistory-from'                => "Pachina d'orichen:",
'mergehistory-into'                => 'Pachina de destino:',
'mergehistory-list'                => "Istorial d'edizions aunible",
'mergehistory-merge'               => "As siguients bersions de [[:$1]] pueden aunir-se con [[:$2]]. Faiga serbir a columna de botons de radio ta aunir nomás as bersions creyadas antis d'un tiempo espezificato. Pare cuenta que si emplega os binclos de nabegazión meterá os botons en o suyo estau orichinal.",
'mergehistory-go'                  => 'Amostrar edizions aunibles',
'mergehistory-submit'              => 'Aunir bersions',
'mergehistory-empty'               => 'No puede aunir-se garra rebisión.',
'mergehistory-success'             => '$3 {{PLURAL:$3|rebisión|rebisions}} de [[:$1]] {{PLURAL:$3|combinata|combinatas}} correutament con [[:$2]].',
'mergehistory-fail'                => "No s'ha puesto aunir os dos istorials, por fabor comprebe a pachina y os parametros de tiempo.",
'mergehistory-no-source'           => "A pachina d'orichen $1 no esiste.",
'mergehistory-no-destination'      => 'A pachina de destino $1 no esiste.',
'mergehistory-invalid-source'      => "A pachina d'orichen ha de tener un títol correuto.",
'mergehistory-invalid-destination' => 'A pachina de destino ha de tener un títol correuto.',
'mergehistory-autocomment'         => "S'ha combinato [[:$1]] en [[:$2]]",
'mergehistory-comment'             => "S'ha combinato [[:$1]] en [[:$2]]: $3",

# Merge log
'mergelog'           => "Rechistro d'unions",
'pagemerge-logentry' => "s'ha aunito [[$1]] con [[$2]] (rebisions dica $3)",
'revertmerge'        => 'Esfer a unión',
'mergelogpagetext'   => "Contino s'amuestra una lista d'as pachinas más rezients que os suyos istorials s'han aunito con o d'atra pachina.",

# Diffs
'history-title'           => 'Istorial de bersions de "$1"',
'difference'              => '(Esferenzias entre bersions)',
'lineno'                  => 'Linia $1:',
'compareselectedversions' => 'Confrontar as bersions trigatas',
'editundo'                => 'esfer',
'diff-multi'              => "(S'ha amagato {{PLURAL:$1|una edizión entremeya|$1 edizions entremeyas}}.)",

# Search results
'searchresults'             => 'Resultau de mirar',
'searchresulttext'          => "Ta más informazión sobre cómo mirar pachinas en {{SITENAME}}, consulte l'[[{{MediaWiki:Helppage}}|{{int:help}}]].",
'searchsubtitle'            => 'Ha mirato \'\'\'[[:$1]]\'\'\' ([[Special:Prefixindex/$1|todas as pachinas que prenzipian con "$1"]] | [[Special:WhatLinksHere/$1|todas as pachinas con binclos enta "$1"]])',
'searchsubtitleinvalid'     => 'Ha mirato "$1"',
'noexactmatch'              => "'''No esiste garra pachina tetulata \"\$1\".''' Puede aduyar [[:\$1|creyando-la]].",
'noexactmatch-nocreate'     => "'''No bi ha garra pachina tetulata \"\$1\".'''",
'toomanymatches'            => "S'ha retornato masiadas coinzidenzias, por fabor, torne á prebar con una consulta diferent",
'titlematches'              => 'Consonanzias de títols de pachina',
'notitlematches'            => "No bi ha garra consonanzia en os títols d'as pachinas",
'textmatches'               => "Consonanzias en o testo d'as pachinas",
'notextmatches'             => "No bi ha garra consonanzia en os testos d'as pachinas",
'prevn'                     => 'anteriors $1',
'nextn'                     => 'siguiens $1',
'viewprevnext'              => 'Beyer ($1) ($2) ($3)',
'search-result-size'        => '$1 ({{PLURAL:$2|1 palabra|$2 palabras}})',
'search-result-score'       => 'Relebanzia: $1%',
'search-redirect'           => '(reendreza $1)',
'search-section'            => '(sezion $1)',
'search-suggest'            => 'Quereba dezir $1?',
'search-interwiki-caption'  => 'Procheutos chermans',
'search-interwiki-default'  => '$1 resultaus:',
'search-interwiki-more'     => '(más)',
'search-mwsuggest-enabled'  => 'con socherenzias',
'search-mwsuggest-disabled' => 'Garra socherenzia',
'search-relatedarticle'     => 'Relazionato',
'mwsuggest-disable'         => "Desautibar as socherenzias d'AJAX",
'searchrelated'             => 'relazionato',
'searchall'                 => 'toz',
'showingresults'            => "Contino se bi {{PLURAL:$1|amuestra '''1''' resultau|amuestran '''$1''' resultaus}} prenzipiando por o numero '''$2'''.",
'showingresultsnum'         => "Contino se bi {{PLURAL:$3|amuestra '''1''' resultau|amuestran os '''$3''' resultaus}} prenzipiando por o numero '''$2'''.",
'showingresultstotal'       => "{{PLURAL:$3|S'amuestra contino o resultau '''$1''' de '''$3'''|S'amuestran contino os resultaus '''$1 - $2''' de '''$3'''}}",
'nonefound'                 => "'''Pare cuenta''': Por defeuto nomás se mira en bels espazios de nombres. Si quiere mirar en toz os contenius (encluyendo pachinas de descusión, plantillas, etc), adiba o prefixo ''all:'' u clabe como prefixo o espazio de nombres deseyau.",
'powersearch'               => 'Busca abanzata',
'powersearch-legend'        => 'Busca abanzata',
'powersearch-ns'            => 'Mirar en os espazios de nombres:',
'powersearch-redir'         => 'Listar reendrezeras',
'powersearch-field'         => 'Mirar',
'search-external'           => 'Busca externa',
'searchdisabled'            => 'A busca en {{SITENAME}} ye temporalment desautibata. Entremistanto, puede mirar en {{SITENAME}} fendo serbir buscadors esternos, pero pare cuenta que os suyos endizes de {{SITENAME}} puede no estar esbiellatos.',

# Preferences page
'preferences'              => 'Preferenzias',
'mypreferences'            => 'Preferenzias',
'prefs-edits'              => "Numero d'edizions:",
'prefsnologin'             => 'No ye identificato',
'prefsnologintext'         => "Ha d'estar [[Special:UserLogin|rechistrau]] y aber enzetau una sesión ta cambiar as preferenzias d'usuario.",
'prefsreset'               => "S'ha tornato as preferenzias t'as suyas baluras almadazenatas.",
'qbsettings'               => 'Preferenzias de "Quickbar"',
'qbsettings-none'          => 'Denguna',
'qbsettings-fixedleft'     => 'Fixa á la zurda',
'qbsettings-fixedright'    => 'Fixa á la dreita',
'qbsettings-floatingleft'  => 'Flotant á la zurda',
'qbsettings-floatingright' => 'Flotant á la dreita',
'changepassword'           => 'Cambiar a palabra de paso',
'skin'                     => 'Aparenzia',
'math'                     => 'Esprisions matematicas',
'dateformat'               => 'Formato de calendata',
'datedefault'              => 'Sin de preferenzias',
'datetime'                 => 'Calendata y ora',
'math_failure'             => 'Error en o codigo',
'math_unknown_error'       => 'error esconoxita',
'math_unknown_function'    => 'funzión esconoxita',
'math_lexing_error'        => 'error de lesico',
'math_syntax_error'        => 'error de sintacsis',
'math_image_error'         => "Bi abió una error en a combersión enta o formato PNG; comprebe que ''latex'', ''dvips'', ''gs'', y ''convert'' sigan instalatos correutament.",
'math_bad_tmpdir'          => "No s'ha puesto escribir u creyar o direutorio temporal d'esprisions matematicas",
'math_bad_output'          => "No s'ha puesto escribir u creyar o direutorio de salida d'esprisions matematicas",
'math_notexvc'             => "No s'ha trobato l'archibo executable ''texvc''. Por fabor, leiga <em>math/README</em> ta confegurar-lo correutament.",
'prefs-personal'           => 'Datos presonals',
'prefs-rc'                 => 'Zaguers cambeos',
'prefs-watchlist'          => 'Lista de seguimiento',
'prefs-watchlist-days'     => "Numero de días que s'amostrarán en a lista de seguimiento:",
'prefs-watchlist-edits'    => "Numero d'edizions que s'amostrarán en a lista ixamplata:",
'prefs-misc'               => 'Atras preferenzias',
'saveprefs'                => 'Alzar preferenzias',
'resetprefs'               => "Tornar t'as preferenzias por defeuto",
'oldpassword'              => 'Palabra de paso antiga:',
'newpassword'              => 'Nueba palabra de paso:',
'retypenew'                => 'Torne á escribir a nueba palabra de paso:',
'textboxsize'              => 'Edizión',
'rows'                     => 'Ringleras:',
'columns'                  => 'Colunnas:',
'searchresultshead'        => 'Mirar',
'resultsperpage'           => "Resultaus que s'amostrarán por pachina:",
'contextlines'             => "Linias de contexto que s'amostrarán por resultau",
'contextchars'             => 'Caráuters de contesto por linia',
'stub-threshold'           => 'Branquil superior ta o formateyo de <a href="#" class="stub">binclos ta borradors</a> (en bytes):',
'recentchangesdays'        => "Días que s'amostrarán en ''zaguers cambeos'':",
'recentchangescount'       => "Numero d'edizions que s'amostrarán en as pachinas de ''zaguers cambeos'', istorials y rechistros:",
'savedprefs'               => "S'han alzato as suyas preferenzias.",
'timezonelegend'           => 'Fuso orario',
'timezonetext'             => "¹Escriba a esferenzia (en oras) entre a suya ora local y a d'o serbidor (UTC).",
'localtime'                => 'Ora local',
'timezoneoffset'           => 'Esferenzia¹',
'servertime'               => 'A ora en o serbidor ye',
'guesstimezone'            => "Emplir-lo con a ora d'o nabegador",
'allowemail'               => "Autibar a rezepzión de correu d'atros usuarios",
'prefs-searchoptions'      => 'Opzions de busca',
'prefs-namespaces'         => 'Espazios de nombres',
'defaultns'                => 'Mirar por defeuto en istos espazios de nombres:',
'default'                  => 'por defeuto',
'files'                    => 'Archibos',

# User rights
'userrights'                  => "Confegurazión d'os dreitos d'os usuarios", # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => "Confegurar collas d'usuarios",
'userrights-user-editname'    => "Escriba un nombre d'usuario:",
'editusergroup'               => "Editar as collas d'usuarios",
'editinguser'                 => "S'esta cambiando os dreitos de l'usuario  '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup'    => "Editar as collas d'usuarios",
'saveusergroups'              => "Alzar as collas d'usuarios",
'userrights-groupsmember'     => 'Miembro de:',
'userrights-groups-help'      => "Puede cambiar as collas an que bi ye iste usuario.
* Un caxa siñalata sinnifica que l'usuario bi ye en ixa colla.
* Una caxa no siñalata sinnifica que l'usuario no ye en ixa colla.
* Un * endica que bustet no puede sacar a colla dimpués d'adibir-la, u bize-bersa.",
'userrights-reason'           => 'Razón ta o cambeo:',
'userrights-no-interwiki'     => "No tiene premiso ta editar os dreitos d'usuario en atras wikis.",
'userrights-nodatabase'       => 'A base de datos $1 no esiste u no ye local.',
'userrights-nologin'          => "Ha d'[[Special:UserLogin|enzetar una sesión]] con una cuenta d'almenistrador ta poder dar dreitos d'usuario.",
'userrights-notallowed'       => "A suya cuenta no tiene premisos ta dar dreitos d'usuario.",
'userrights-changeable-col'   => 'Grupos que puede cambiar',
'userrights-unchangeable-col' => 'Collas que no puede cambiar',

# Groups
'group'               => 'Colla:',
'group-user'          => 'Usuarios',
'group-autoconfirmed' => 'Usuarios Autoconfirmatos',
'group-bot'           => 'Bots',
'group-sysop'         => 'Almenistradors',
'group-bureaucrat'    => 'Burocratas',
'group-suppress'      => 'Superbisors',
'group-all'           => '(toz)',

'group-user-member'          => 'Usuario',
'group-autoconfirmed-member' => 'Usuario autoconfirmato',
'group-bot-member'           => 'Bot',
'group-sysop-member'         => 'Almenistrador',
'group-bureaucrat-member'    => 'Burocrata',
'group-suppress-member'      => 'Superbisor',

'grouppage-user'          => '{{ns:project}}:Usuarios',
'grouppage-autoconfirmed' => '{{ns:project}}:Usuarios autoconfirmatos',
'grouppage-bot'           => '{{ns:project}}:Bots',
'grouppage-sysop'         => '{{ns:project}}:Almenistradors',
'grouppage-bureaucrat'    => '{{ns:project}}:Burocratas',
'grouppage-suppress'      => '{{ns:project}}:Superbisors',

# Rights
'right-read'                 => 'Leyer pachinas',
'right-edit'                 => 'Editar pachinas',
'right-createpage'           => 'Creyar pachinas (que no sían pachinas de descusión)',
'right-createtalk'           => 'Creyar pachinas de descusión',
'right-createaccount'        => "Creyar nuebas cuentas d'usuario",
'right-minoredit'            => 'Siñalar como edizions menors',
'right-move'                 => 'Tresladar pachinas',
'right-move-subpages'        => 'Tresladar as pachinas con a suyas sozpachinas',
'right-suppressredirect'     => 'No creyar una reendrezera dende o nombre antigo cuan se treslade una pachina',
'right-upload'               => 'Cargar archibos',
'right-reupload'             => "Cargar denzima d'un archibo esistent",
'right-reupload-own'         => "Cargar denzima d'un archibo que ya eba cargau o mesmo usuario",
'right-reupload-shared'      => 'Cargar localment archibos con un nombre emplegato en o repositorio multimedia compartito',
'right-upload_by_url'        => 'Cargar un archibo dende una adreza URL',
'right-purge'                => 'Porgar a memoria caché ta una pachina sin nezesidat de confirmar-la',
'right-autoconfirmed'        => 'Editar pachinas semiprotechitas',
'right-bot'                  => 'Ser tratato como un prozeso automatico (bot)',
'right-nominornewtalk'       => 'Fer que as edizions menors en pachinas de descusión no cheneren l\'abiso de "nuebos mensaches"',
'right-apihighlimits'        => 'Usar limites más altos en consultas API',
'right-writeapi'             => "Emplego de l'API d'escritura",
'right-delete'               => 'Borrar pachinas',
'right-bigdelete'            => 'Borrar pachinas con istorials largos',
'right-deleterevision'       => "Borrar y recuperar bersions espezificas d'una pachina",
'right-deletedhistory'       => "Beyer as dentradas borratas de l'istorial, sin o suyo testo asoziato",
'right-browsearchive'        => 'Mirar pachinas borratas',
'right-undelete'             => 'Recuperar una pachina',
'right-suppressrevision'     => 'Rebisar y recuperar bersions amagatas ta os Almenistradors',
'right-suppressionlog'       => 'Beyer os rechistros pribatos',
'right-block'                => "Bloqueyar á atros usuarios ta pribar-les d'editar",
'right-blockemail'           => 'Bloqueyar á un usuario ta pribar-le de nimbiar correus',
'right-hideuser'             => "Bloqueyar un nombre d'usuario, amagando-lo d'o publico",
'right-ipblock-exempt'       => "Inorar os bloqueyos d'adrezas IP, os autobloqueyos y os bloqueyos de rangos de IPs.",
'right-proxyunbannable'      => 'Inorar os bloqueyos automaticos de proxies',
'right-protect'              => 'Cambiar os libels de protezión y editar pachinas protechitas',
'right-editprotected'        => 'Editar pachinas protechitas (sin de protezión en cascada)',
'right-editinterface'        => "Editar a interfizie d'usuario",
'right-editusercssjs'        => "Editar os archibos CSS y JS d'atros usuarios",
'right-rollback'             => "Esfer á escape a edizión d'a zaguer usuario que cambió una pachina",
'right-markbotedits'         => 'Siñalar as edizions esfeitas como edizions de bot',
'right-noratelimit'          => "No se les aplican as tasas masimas d'edizions",
'right-import'               => 'Importar pachinas dende atros wikis',
'right-importupload'         => "Importar pacihnas d'archibos cargatos",
'right-patrol'               => 'Siñalar edizions como patrullatas',
'right-autopatrol'           => 'Siñalar automaticament as edizions como patrullatas',
'right-patrolmarks'          => 'Amostrar os siñals de patrullache en os zaguers cambeos',
'right-unwatchedpages'       => 'Amostrar una lista de pachinas sin cosirar',
'right-trackback'            => 'Adibir un trackback',
'right-mergehistory'         => "Combinar l'istorial d'as pachinas",
'right-userrights'           => "Editar toz os dreitos d'usuario",
'right-userrights-interwiki' => "Editar os dreitos d'usuario d'os usuarios d'atros wikis",
'right-siteadmin'            => 'Trancar y estrancar a base de datos',

# User rights log
'rightslog'      => "Rechistro de cambios en os dreitos d'os usuarios",
'rightslogtext'  => "Iste ye un rechistro d'os cambios en os dreitos d'os usuarios",
'rightslogentry' => "ha cambiato os dreitos d'usuario de $1: de $2 a $3",
'rightsnone'     => '(denguno)',

# Recent changes
'nchanges'                          => '$1 {{PLURAL:$1|cambeo|cambeos}}',
'recentchanges'                     => 'Zaguers cambeos',
'recentchangestext'                 => "Siga os cambeos más rezients d'a wiki en ista pachina.",
'recentchanges-feed-description'    => "Seguir en ista canal de notizias os cambeos más rezients d'o wiki.",
'rcnote'                            => "Contino {{PLURAL:$1|s'amuestra o unico cambeo feito|s'amuestran os zaguers '''$1''' cambeos feitos}} en {{PLURAL:$2|o zaguer día|os zaguers '''$2''' días}}, dica o $5, $4.",
'rcnotefrom'                        => "Contino s'amuestran os cambeos dende '''$2''' (dica '''$1''').",
'rclistfrom'                        => 'Amostrar cambeos rezients dende $1',
'rcshowhideminor'                   => '$1 edizions menors',
'rcshowhidebots'                    => '$1 bots',
'rcshowhideliu'                     => '$1 usuarios rechistraus',
'rcshowhideanons'                   => '$1 usuarios anonimos',
'rcshowhidepatr'                    => '$1 edizions controlatas',
'rcshowhidemine'                    => '$1 as mías edizions',
'rclinks'                           => 'Amostrar os zaguers $1 cambeos en os zaguers $2 días.<br />$3',
'diff'                              => 'esf',
'hist'                              => 'ist',
'hide'                              => 'amagar',
'show'                              => 'Amostrar',
'minoreditletter'                   => 'm',
'newpageletter'                     => 'N',
'boteditletter'                     => 'b',
'number_of_watching_users_pageview' => '[$1 {{PLURAL:$1|usuario|usuarios}} cosirando]',
'rc_categories'                     => 'Limite d\'as categorías (deseparatas por "|")',
'rc_categories_any'                 => 'Todas',
'newsectionsummary'                 => 'Nueba sezión: /* $1 */',

# Recent changes linked
'recentchangeslinked'          => 'Cambeos en pachinas relazionadas',
'recentchangeslinked-title'    => 'Cambeos relazionatos con "$1"',
'recentchangeslinked-noresult' => 'No bi abió cambeos en as pachinas binculatas en o entrebalo de tiempo endicato.',
'recentchangeslinked-summary'  => "Ista ye una lista de cambios rezients en pachinas con binclos dende una pachina espezifica (u a miembros d'una categoría espezificata).  S'amuestran en '''negreta''' as pachinas d'a suya [[Special:Watchlist|lista de seguimiento]].",
'recentchangeslinked-page'     => "Nombre d'a pachina:",
'recentchangeslinked-to'       => 'Amostrar en cuenta os cambeos en pachinas binculatas con a pachina data',

# Upload
'upload'                      => 'Cargar archibo',
'uploadbtn'                   => 'Cargar un archibo',
'reupload'                    => 'Cargar un atra begada',
'reuploaddesc'                => "Anular a carga y tornar ta o formulario de carga d'archibos.",
'uploadnologin'               => 'No ha enzetato una sesión',
'uploadnologintext'           => "Ha d'estar [[Special:UserLogin|rechistrau]] ta cargar archibos.",
'upload_directory_missing'    => 'O direutorio de carga ($1) no esiste y no lo puede creyar o serbidor web.',
'upload_directory_read_only'  => "O serbidor web no puede escribir en o direutorio de carga d'archibos ($1).",
'uploaderror'                 => "S'ha produzito una error en cargar l'archibo",
'uploadtext'                  => "Faiga serbir o formulario d'o cobaxo ta cargar archibos.
Ta beyer u mirar imáchens cargatas denantes baiga t'a [[Special:ImageList|lista d'archibos cargatos]]. As cargas y recargas tamién se rechistran en o [[Special:Log/upload|rechistro de cargas]], y os borraus en o [[Special:Log/delete|rechistro de borraus]].

Ta encluyir un archibo u imachen en una pachina, emplegue un binclo d'una d'istas trazas 
*'''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:Archibo.jpg]]</nowiki></tt>''' ta usar a bersion completa de l'archibo, 
*'''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:Archibo.png|200px|thumb|left|testo alternatibo]]</nowiki></tt>''' ta usar una bersión de 200 píxels d'amplaria en una caixa a la marguin cucha con 'testo alternatibo' como descripzión
*'''<tt><nowiki>[[</nowiki>{{ns:media}}<nowiki>:Archibo.ogg]]</nowiki></tt>''' ta fer un binclo dreitament ta l'archibo sin amostrar-lo.",
'upload-permitted'            => "Tipos d'archibo premititos: $1.",
'upload-preferred'            => "Tipos d'archibo preferitos: $1.",
'upload-prohibited'           => "Tipos d'archibo biedatos: $1.",
'uploadlog'                   => 'rechistro de cargas',
'uploadlogpage'               => "Rechistro de cargas d'archibos",
'uploadlogpagetext'           => "Contino ye una lista d'os zaguers archibos cargatos. Mire-se a [[Special:NewImages|galería d'archibos nuebos]] ta tener una ambiesta más bisual.",
'filename'                    => "Nombre de l'archibo",
'filedesc'                    => 'Resumen',
'fileuploadsummary'           => 'Resumen:',
'filestatus'                  => "Estau d'os dreitos d'autor (copyright):",
'filesource'                  => 'Fuent:',
'uploadedfiles'               => 'Archibos cargatos',
'ignorewarning'               => "Inorar l'abiso y alzar l'archibo en cualsiquier caso",
'ignorewarnings'              => 'Inorar cualsiquier abiso',
'minlength1'                  => "Os nombres d'archibo han de tener á lo menos una letra.",
'illegalfilename'             => "O nombre d'archibo «$1» tiene caráuters no premititos en títols de pachinas. Por fabor, cambee o nombre de l'archibo y mire de tornar á cargarlo.",
'badfilename'                 => 'O nombre d\'a imachen s\'ha cambiato por "$1".',
'filetype-badmime'            => 'No se premite cargar archibos de tipo MIME "$1".',
'filetype-unwanted-type'      => "Os '''\".\$1\"''' son un tipo d'archibo no deseyato.  Se prefieren os archibos {{PLURAL:\$3|de tipo|d'os tipos}} \$2.",
'filetype-banned-type'        => "No se premiten os archibos de tipo '''\".\$1\"'''. {{PLURAL:\$3|O tipo premitito ye|Os tipos premititos son}} \$2.",
'filetype-missing'            => 'L\'archibo no tiene garra estensión (como ".jpg").',
'large-file'                  => 'Se consella que os archibos no sigan mayors de $1; iste archibo ocupa $2.',
'largefileserver'             => "A grandaria d'iste archibo ye mayor d'a que a confegurazión d'iste serbidor premite.",
'emptyfile'                   => "Parixe que l'archibo que se miraba de cargar ye buedo; por fabor, comprebe que ixe ye reyalment l'archibo que quereba cargar.",
'fileexists'                  => "Ya bi ha un archibo con ixe nombre. Por fabor, Por favor mire-se l'archibo esistent <strong><tt>$1</tt></strong> si no ye seguro de querer sustituyir-lo.",
'filepageexists'              => "A pachina de descripzión ta iste archibo ya ye creyata en <strong><tt>$1</tt></strong>, pero no esiste garra archibo con iste nombre. O resumen que escriba no amaneixerá en a pachina de descripzión. Si quiere que o suyo resumen amaneixca aquí, abrá d'editar-lo manualment",
'fileexists-extension'        => "Ya bi ha un archibo con un nombre parexiu:<br />
Nombre de l'archibo que ye cargando: <strong><tt>$1</tt></strong><br />
Nombre de l'archibo ya esistent: <strong><tt>$2</tt></strong><br />
Por fabor, trigue un nombre diferent.",
'fileexists-thumb'            => "<center>'''Archibo esistent'''</center>",
'fileexists-thumbnail-yes'    => "Parixe que l'archibo ye una imachen prou chicota <i>(miniatura)</i>. Comprebe por fabor l'archibo <strong><tt>$1</tt></strong>.<br />
Si l'archibo comprebato ye a mesma imachen en tamaño orichinal no cal cargar una nueba miniatura.",
'file-thumbnail-no'           => "O nombre de l'archibo prenzipia con <strong><tt>$1</tt></strong>. 
Pareix que estase una imachen no guaire gran <i>(thumbnail)</i>.
Si tiene ista imachen a toda resoluzión, cargue-la, si no, por fabor, cambee o nombre de l'archibo.",
'fileexists-forbidden'        => "Ya bi ha un archibo con iste nombre. Por fabor, cambee o nombre de l'archibo y torne á cargar-lo. [[Image:$1|thumb|center|$1]]",
'fileexists-shared-forbidden' => "Ya bi ha un archibo con ixe nombre en o repositorio compartito; por fabor, torne t'a pachina anterior y cargue o suyo archibo con atro nombre. [[Image:$1|thumb|center|$1]]",
'file-exists-duplicate'       => "Iste archibo ye un duplicau {{PLURAL:$1|d'o siguient archibo|d'os siguients archibos}}:",
'successfulupload'            => 'Cargata correutament',
'uploadwarning'               => "Albertenzia de carga d'archibo",
'savefile'                    => 'Alzar archibo',
'uploadedimage'               => '«[[$1]]» cargato.',
'overwroteimage'              => 's\'ha cargato una nueba bersión de "[[$1]]"',
'uploaddisabled'              => "A carga d'archibos ye desautibata",
'uploaddisabledtext'          => 'No ye posible cargar archibos en {{SITENAME}}.',
'uploadscripted'              => 'Iste archibo contiene codigo de script u HTML que puede estar interpretado incorreutament por un nabegador.',
'uploadcorrupt'               => "Iste archibo ye corrompito u tiene una estensión incorreuta. Por fabor, comprebe l'archibo y cargue-lo una atra begada.",
'uploadvirus'                 => 'Iste archibo tiene un birus! Detalles: $1',
'sourcefilename'              => "Nombre de l'archibo d'orichen:",
'destfilename'                => "Nombre de l'archibo de destín:",
'upload-maxfilesize'          => "Masima grandaria de l'archibo: $1",
'watchthisupload'             => 'Cosirar ista pachina',
'filewasdeleted'              => 'Una archibo con iste mesmo nombre ya se cargó denantes y estió borrato dimpués. Abría de comprebar $1 antes de tornar á cargar-lo una atra begada.',
'upload-wasdeleted'           => "'''Pare cuenta: Ye cargando un archibo que ya estió borrato d'antes más.'''

Abría de repensar si ye apropiato continar con a carga d'iste archibo. Aquí tiene o rechistro de borrau d'iste archibo ta que pueda comprebar a razón que se dio ta borrar-lo:",
'filename-bad-prefix'         => 'O nombre de l\'archibo que ye cargando prenzipia por <strong>"$1"</strong>, que ye un nombre no descriptibo que gosa clabar automaticament as camaras dichitals. Por fabor, trigue un nombre más descriptibo ta iste archibo.',
'filename-prefix-blacklist'   => ' #<!-- dixe ista linia esautament igual como ye --> <pre>
# A sintacsis ye asinas:
#   * Tot o que prenzipia por un caráuter "#" dica la fin d\'a linia ye un comentario
#   * As atras linias tienen os prefixos que claban automaticament as camaras dichitals
CIMG # Casio
DSC_ # Nikon
DSCF # Fuji
DSCN # Nikon
DUW # bels telefonos móbils
IMG # chenerica
JD # Jenoptik
MGP # Pentax
PICT # misz.
 #</pre> <!-- dixe ista linia esautament igual como ye -->',

'upload-proto-error'      => 'Protocolo incorreuto',
'upload-proto-error-text' => 'Si quiere cargar archibos dende atra pachina, a URL ha de prenzipiar por <code>http://</code> u <code>ftp://</code>.',
'upload-file-error'       => 'Error interna',
'upload-file-error-text'  => "Ha escaizito una error interna entre que se prebaba de creyar un archibo temporal en o serbidor. Por fabor, contaute con un [[Special:ListUsers/sysop|almenistrador]] d'o sistema.",
'upload-misc-error'       => 'Error esconoixita en a carga',
'upload-misc-error-text'  => "Ha escaizito una error entre que se cargaba l'archibo. Por fabor, comprebe que a URL ye conforme y aczesible y dimpués prebe de fer-lo una atra begada. Si o problema contina, contaute con un [[Special:ListUsers/sysop|almenistrador]] d'o sistema.",

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => 'No se podió aczeder dica la URL',
'upload-curl-error6-text'  => 'No se podió plegar dica a URL. Por fabor, comprebe que a URL sía correuta y o sitio web sía funzionando.',
'upload-curl-error28'      => "Tiempo d'aspera sobrexito",
'upload-curl-error28-text' => "O tiempo de respuesta d'a pachina ye masiau gran. Por fabor, comprebe si o serbidor ye funzionando, aspere bel tiempo y mire de tornar á fer-lo.  Talment deseye prebar de nuebo cuan o rete tienga menos carga.",

'license'            => 'Lizenzia:',
'nolicense'          => "No s'en ha trigato garra",
'license-nopreview'  => '(Ambiesta prebia no disponible)',
'upload_source_url'  => ' (una URL conforme y publicament aczesible)',
'upload_source_file' => ' (un archibo en o suyo ordenador)',

# Special:ImageList
'imagelist-summary'     => "Ista pachina espezial amuestra toz os archibos cargatos.
Por defeuto os zaguers archibos cargatos s'amuestran en o cobalto d'a lista.
Fendo click en un encabezau de colunna se cambia o criterio d'ordenazión.",
'imagelist_search_for'  => "Mirar por nombre de l'archibo:",
'imgfile'               => 'archibo',
'imagelist'             => 'Lista de imachens',
'imagelist_date'        => 'Calendata',
'imagelist_name'        => 'Nombre',
'imagelist_user'        => 'Usuario',
'imagelist_size'        => 'Grandaria (bytes)',
'imagelist_description' => 'Descripzión',

# Image description page
'filehist'                       => "Istorial de l'archibo",
'filehist-help'                  => "Punche en una calendata/ora ta beyer l'archibo como amanixeba por ixas engüeltas.",
'filehist-deleteall'             => 'borrar-lo tot',
'filehist-deleteone'             => 'borrar',
'filehist-revert'                => 'esfer',
'filehist-current'               => 'autual',
'filehist-datetime'              => 'Calendata/Ora',
'filehist-user'                  => 'Usuario',
'filehist-dimensions'            => 'Dimensions',
'filehist-filesize'              => "Grandaria d'o fichero",
'filehist-comment'               => 'Comentario',
'imagelinks'                     => 'Binclos ta la imachen',
'linkstoimage'                   => "{{PLURAL:$1|A pachina siguient tiene|Contino s'amuestran $1 pachinas que tienen}} binclos ta iste archibo:",
'nolinkstoimage'                 => 'Denguna pachina tiene un binclo ta ista imachen.',
'morelinkstoimage'               => 'Amostrar [[Special:WhatLinksHere/$1|más binclos]] ta iste archibo.',
'redirectstofile'                => '{{PLURAL:$1|O siguient archibo reendreza|Os siguients $1 archibos reendrezan}} enta iste archibo:',
'duplicatesoffile'               => "{{PLURAL:$1|O siguient archibo ye un duplicau|Os siguients $1 archibos son duplicaus}} d'iste archibo:",
'sharedupload'                   => 'Iste archibo ye compartito y puede estar que siga emplegato en atros procheutos.',
'shareduploadwiki'               => 'Ta más informazión, consulte $1.',
'shareduploadwiki-desc'          => "A descripzión d'a $1 en o repositorio compartito s'amuestra en o cobaxo.",
'shareduploadwiki-linktext'      => "pachina de descripzión de l'archibo",
'shareduploadduplicate'          => "Este archibo ye un duplicato de $1 d'un repositorio compartito.",
'shareduploadduplicate-linktext' => 'atro archibo',
'shareduploadconflict'           => "Iste archibo tiene o mesmo nombre que $1 d'o repositorio compartito.",
'shareduploadconflict-linktext'  => 'atro archibo',
'noimage'                        => 'No bi ha garra archibo con ixe nombre, pero puede $1.',
'noimage-linktext'               => 'cargar-lo',
'uploadnewversion-linktext'      => "Cargar una nueba bersión d'iste archibo",
'imagepage-searchdupe'           => 'Mirar archibos duplicatos',

# File reversion
'filerevert'                => 'Rebertir $1',
'filerevert-legend'         => 'Rebertir fichero',
'filerevert-intro'          => "Ye rebertindo '''[[Media:$1|$1]]''' á la [$4 bersion de $3, $2].",
'filerevert-comment'        => 'Comentario:',
'filerevert-defaultcomment' => 'Rebertito á la bersión de $2, $1',
'filerevert-submit'         => 'Rebertir',
'filerevert-success'        => "S'ha rebertito '''[[Media:$1|$1]]''' á la [$4 bersión de $3, $2].",
'filerevert-badversion'     => "No bi ha garra bersión antiga d'o archibo con ixa calendata y ora.",

# File deletion
'filedelete'                  => 'Borrar $1',
'filedelete-legend'           => 'Borrar archibo',
'filedelete-intro'            => "Ye borrando '''[[Media:$1|$1]]'''.",
'filedelete-intro-old'        => "Ye borrando a bersión de '''[[Media:$1|$1]]''' de [$4 $3, $2].",
'filedelete-comment'          => 'Causa:',
'filedelete-submit'           => 'Borrar',
'filedelete-success'          => "S'ha borrato '''$1'''.",
'filedelete-success-old'      => "S'ha borrato a bersión de '''[[Media:$1|$1]]''' de $3, $2.",
'filedelete-nofile'           => "'''$1''' no esiste en {{SITENAME}}.",
'filedelete-nofile-old'       => "No bi ha garra bersión alzata de '''$1''' con ixos atributos.",
'filedelete-iscurrent'        => "Ye prebando de borrar a bersión más rezient d'iste archibo. Por fabor, torne en primeras ta una bersión anterior.",
'filedelete-otherreason'      => 'Atras razons:',
'filedelete-reason-otherlist' => 'Atra razón',
'filedelete-reason-dropdown'  => "*Razons comuns ta borrar archibos
** Dreitos d'autor no respetatos
** Archibo duplicato",
'filedelete-edit-reasonlist'  => "Editar as razons d'o borrau",

# MIME search
'mimesearch'         => 'Mirar por tipo MIME',
'mimesearch-summary' => 'Ista pachina premite filtrar archibos seguntes o suyo tipo MIME. Escribir: tipodeconteniu/subtipo, por exemplo <tt>image/jpeg</tt>.',
'mimetype'           => 'Tipo MIME:',
'download'           => 'escargar',

# Unwatched pages
'unwatchedpages' => 'Pachinas no cosiratas',

# List redirects
'listredirects' => 'Lista de reendrezeras',

# Unused templates
'unusedtemplates'     => 'Plantillas sin de uso',
'unusedtemplatestext' => "En ista pachina se fa una lista de todas as pachinas en o espazio de nombres de plantillas que no s'encluyen en denguna atra pachina. Alcuerde-se de mirar as pachinas que tiengan binclos ta una plantilla antis de borrar-la.",
'unusedtemplateswlh'  => 'atros binclos',

# Random page
'randompage'         => "Una pachina á l'azar",
'randompage-nopages' => 'No bi ha garra pachina en iste espazio de nombres.',

# Random redirect
'randomredirect'         => 'Ir-ie á una adreza cualsiquiera',
'randomredirect-nopages' => 'No bi ha garra reendrezera en iste espazio de nombres.',

# Statistics
'statistics'             => 'Estadisticas',
'sitestats'              => 'Estadisticas de {{SITENAME}}',
'userstats'              => "Estadisticas d'usuario",
'sitestatstext'          => "Bi ha un total de {{PLURAL:$1|'''1''' pachina|'''$1''' pachinas}} en a base de datos.
Isto encluye pachinas de descusión, pachinas sobre {{SITENAME}}, borradors menimos, reendrezeras y atras que cal que no puedan estar consideratas pachinas de contenius.
Sacando ixas pachinas, regular que bi aiga {{PLURAL:$2|1 pachina|'''$2''' pachinas}} de conteniu lechitimo.

Bi ha '''$8''' {{PLURAL:$8|archibo alzato|archibos alzatos}} en o serbidor.

Dende a debantadera d'o wiki bi ha abito un total de '''$3''' {{PLURAL:$3|besitas|besitas}} y '''$4''' {{PLURAL:$4|edizión de pachina|edizions de pachinas}}.
Isto resulta en una meya de '''$5''' {{PLURAL:$5|edizión|edizions}} por pachina y '''$6''' {{PLURAL:$6|besita|besitas}} por edizión.

A longaria d'a [http://www.mediawiki.org/wiki/Manual:Job_queue coda de quefers] ye de '''$7'''",
'userstatstext'          => "Bi ha {{PLURAL:$1|'''1''' usuario rechistrato|'''$1''' usuarios rechistratos}},
d'os que '''$2''' (o '''$4%''') {{PLURAL:$1|en ye $5|en son $5}}.",
'statistics-mostpopular' => 'Pachinas más bistas',

'disambiguations'      => 'Pachinas de desambigazión',
'disambiguationspage'  => 'Template:Desambigazión',
'disambiguations-text' => "As siguients pachinas tienen binclos ta una '''pachina de desambigazión'''.
Ixos binclos abrían de ir millor t'a pachina espezifica apropiada.<br />
Una pachina se considera pachina de desambigazión si fa serbir una plantilla probenient de  [[MediaWiki:Disambiguationspage]].",

'doubleredirects'            => 'Reendrezeras dobles',
'doubleredirectstext'        => "En ista pachina s'amuestran as pachinas que son reendrezatas enta atras reendrezeras.  Cada ringlera contiene o binclo t'a primer y segunda reendrezeras, y tamién o destino d'a segunda reendrezera, que ye á sobent a pachina \"reyal\" á la que a primer pachina abría d'endrezar.",
'double-redirect-fixed-move' => "S'ha tresladau [[$1]], agora ye una endrezera ta [[$2]]",
'double-redirect-fixer'      => 'Apañador de reendrezeras',

'brokenredirects'        => 'Reendrezeras crebatas',
'brokenredirectstext'    => 'As siguients reendrezeras leban enta pachinas inesistents.',
'brokenredirects-edit'   => '(editar)',
'brokenredirects-delete' => '(borrar)',

'withoutinterwiki'         => "Pachinas sin d'interwikis",
'withoutinterwiki-summary' => 'As siguients pachinas no tienen binclos ta bersions en atras luengas:',
'withoutinterwiki-legend'  => 'Prefixo',
'withoutinterwiki-submit'  => 'Amostrar',

'fewestrevisions' => 'Articlos con menos edizions',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|byte|bytes}}',
'ncategories'             => '$1 {{PLURAL:$1|categoría|categorías}}',
'nlinks'                  => '$1 {{PLURAL:$1|binclo|binclos}}',
'nmembers'                => '$1 {{PLURAL:$1|miembro|miembros}}',
'nrevisions'              => '$1 {{PLURAL:$1|bersión|bersions}}',
'nviews'                  => '$1 {{PLURAL:$1|besita|besitas}}',
'specialpage-empty'       => 'Ista pachina ye bueda.',
'lonelypages'             => 'Pachinas popiellas',
'lonelypagestext'         => "Garra pachina de {{SITENAME}} tiene binclos t'as pachinas que se listan contino.",
'uncategorizedpages'      => 'Pachinas sin categorizar',
'uncategorizedcategories' => 'Categorías sin categorizar',
'uncategorizedimages'     => 'Archibos sin categorizar',
'uncategorizedtemplates'  => 'Plantillas sin categorizar',
'unusedcategories'        => 'Categorías sin emplegar',
'unusedimages'            => 'Imachens sin uso',
'popularpages'            => 'Pachinas populars',
'wantedcategories'        => 'Categorías requiestas',
'wantedpages'             => 'Pachinas requiestas',
'missingfiles'            => 'Archibos que faltan',
'mostlinked'              => 'Pachinas más enlazadas',
'mostlinkedcategories'    => 'Categorías más enlazadas',
'mostlinkedtemplates'     => 'Plantillas más binculatas',
'mostcategories'          => 'Pachinas con más categorías',
'mostimages'              => 'Archibos más emplegatos',
'mostrevisions'           => 'Pachinas con más edizions',
'prefixindex'             => 'Pachinas por prefixo',
'shortpages'              => 'Pachinas más curtas',
'longpages'               => 'Pachinas más largas',
'deadendpages'            => 'Pachinas sin salida',
'deadendpagestext'        => 'As siguients pachinas no tienen binclos ta denguna atra pachina de {{SITENAME}}.',
'protectedpages'          => 'Pachinas protechitas',
'protectedpages-indef'    => 'Nomás protezions indefinitas',
'protectedpagestext'      => 'As siguients pachinas son protechitas contra edizions u treslaus',
'protectedpagesempty'     => 'En iste inte no bi ha garra pachina protechita con ixos parametros.',
'protectedtitles'         => 'Títols protechitos',
'protectedtitlestext'     => 'Os siguients títols son protechitos ta pribar a suya creyazión',
'protectedtitlesempty'    => 'En iste inte no bi ha garra títol protechito con ixos parametros.',
'listusers'               => "Lista d'usuarios",
'newpages'                => 'Pachinas nuebas',
'newpages-username'       => "Nombre d'usuario",
'ancientpages'            => 'Pachinas más biellas',
'move'                    => 'Tresladar',
'movethispage'            => 'Tresladar ista pachina',
'unusedimagestext'        => 'Por fabor, pare cuenta que atros puestos web pueden tener binclos ta imachens con una URL dreita y, por ixo, podrían amanixer en ista lista encara que sí se faigan serbir autibament.',
'unusedcategoriestext'    => 'As siguients categoría son creyatas, pero no bi ha garra articlo u categoría que las faiga serbir.',
'notargettitle'           => 'No bi ha garra pachina de destino',
'notargettext'            => 'No ha espezificato en que pachina quiere aplicar ista funzión.',
'nopagetitle'             => 'No esiste ixa pachina',
'nopagetext'              => 'A pachina que ha espezificato no esiste.',
'pager-newer-n'           => '{{PLURAL:$1|1 más rezient|$1 más rezients}}',
'pager-older-n'           => '{{PLURAL:$1|1 más antiga|$1 más antigas}}',
'suppress'                => 'Superbisión',

# Book sources
'booksources'               => 'Fuents de libros',
'booksources-search-legend' => 'Mirar fuents de libros',
'booksources-go'            => 'Ir-ie',
'booksources-text'          => 'Contino ye una lista de binclos ta atros puestos an que benden libros nuebos y usatos, talment bi aiga más informazión sobre os libros que ye mirando.',

# Special:Log
'specialloguserlabel'  => 'Usuario:',
'speciallogtitlelabel' => 'Títol:',
'log'                  => 'Rechistros',
'all-logs-page'        => 'Toz os rechistros',
'log-search-legend'    => 'Mirar rechistros',
'log-search-submit'    => 'Ir-ie',
'alllogstext'          => "Presentazión conchunta de toz os rechistros de  {{SITENAME}}.
Ta reduzir o listau puede trigar un tipo de rechistro, o nombre de l'usuario u a pachina afeutata.",
'logempty'             => 'No bi ha garra elemento en o rechistro con ixas carauteristicas.',
'log-title-wildcard'   => 'Mirar títols que prenzipien con iste testo',

# Special:AllPages
'allpages'          => 'Todas as pachinas',
'alphaindexline'    => '$1 á $2',
'nextpage'          => 'Siguient pachina ($1)',
'prevpage'          => 'Pachina anterior ($1)',
'allpagesfrom'      => 'Amostrar pachinas que prenzipien por:',
'allarticles'       => 'Toz os articlos',
'allinnamespace'    => 'Todas as pachinas (espazio $1)',
'allnotinnamespace' => "Todas as pachinas (fueras d'o espazio de nombres $1)",
'allpagesprev'      => 'Anterior',
'allpagesnext'      => 'Siguient',
'allpagessubmit'    => 'Amostrar',
'allpagesprefix'    => 'Amostrar pachinas con o prefixo:',
'allpagesbadtitle'  => 'O títol yera incorreuto u teneba un prefixo de binclo inter-luenga u inter-wiki. Puede contener uno u más caráuters que no se pueden emplegar en títols.',
'allpages-bad-ns'   => '{{SITENAME}} no tiene o espazio de nombres "$1".',

# Special:Categories
'categories'                    => 'Categorías',
'categoriespagetext'            => "As siguients categorías contienen bella pachina u archibo multimedia.
No s'amuestran aquí as [[Special:UnusedCategories|categorías no emplegatas]].
Se beigan tamién as [[Special:WantedCategories|categorías requiestas]].",
'categoriesfrom'                => 'Amostrar as categoría que prenzipien por:',
'special-categories-sort-count' => 'ordenar por recuento',
'special-categories-sort-abc'   => 'ordenar alfabeticament',

# Special:ListUsers
'listusersfrom'      => 'Amostrar usuarios que o nombre suyo prenzipie por:',
'listusers-submit'   => 'Amostrar',
'listusers-noresult' => "No s'ha trobato ixe usuario.",

# Special:ListGroupRights
'listgrouprights'          => "Dreitos d'a colla d'usuarios",
'listgrouprights-summary'  => "Contino bi ye una lista de collas d'usuario definitas en iste wiki, con os suyos dreitos d'aczeso asoziatos. Tamién puet trobar aquí [[{{MediaWiki:Listgrouprights-helppage}}|informazión adizional]] sobre os dreitos indibiduals.",
'listgrouprights-group'    => 'Colla',
'listgrouprights-rights'   => 'Dreitos',
'listgrouprights-helppage' => "Help:Dreitos d'a colla",
'listgrouprights-members'  => '(listau de miembros)',

# E-mail user
'mailnologin'     => "No nimbiar l'adreza",
'mailnologintext' => "Ha d'aber [[Special:UserLogin|enzetato una sesión]] y tener una adreza de correu-e conforme en as suyas [[Special:Preferences|preferenzias]] ta nimbiar un correu eletronico ta atros usuarios.",
'emailuser'       => 'Nimbiar un correu electronico ta iste usuario',
'emailpage'       => "Nimbiar correu ta l'usuario",
'emailpagetext'   => "Si iste usuario ese rechistrato una adreza de correu-e conforme en as suyas preferenzias d'usuario, puede nimbiar-le un mensache con iste formulario.
L'adreza de correu-e que endicó en as suyas [[Special:Preferences|preferenzias d'usuario]] amaneixerá en o campo 'remitent' ta que o destinatario pueda responder-le.",
'usermailererror' => "L'ocheto de correu retornó una error:",
'defemailsubject' => 'Correu de {{SITENAME}}',
'noemailtitle'    => 'No bi ha garra adreza de correu eletronico',
'noemailtext'     => "Iste usuario no ha espezificato una adreza conforme de correu electronico, u s'ha estimato más no recullir correu electronico d'atros usuarios.",
'emailfrom'       => 'De:',
'emailto'         => 'Ta:',
'emailsubject'    => 'Afer:',
'emailmessage'    => 'Mensache:',
'emailsend'       => 'Nimbiar',
'emailccme'       => "Nimbiar-me una copia d'o mío mensache.",
'emailccsubject'  => "Copia d'o suyo mensache ta $1: $2",
'emailsent'       => 'Mensache de correu nimbiato',
'emailsenttext'   => "S'ha nimbiato o suyo correu.",
'emailuserfooter' => 'Iste correu-e s\'ha nimbiato por $1 ta $2 fendo serbir a funzión "Email user" de {{SITENAME}}.',

# Watchlist
'watchlist'            => 'Lista de seguimiento',
'mywatchlist'          => 'Lista de seguimiento',
'watchlistfor'         => "(de '''$1''')",
'nowatchlist'          => 'No tiens denguna pachina en a lista de seguimiento.',
'watchlistanontext'    => "Ha de $1 ta beyer u editar as dentradas d'a suya lista de seguimiento.",
'watchnologin'         => 'No ha enzetato a sesión',
'watchnologintext'     => "Ha d'estar [[Special:UserLogin|identificato]] ta poder cambiar a suya lista de seguimiento.",
'addedwatch'           => 'Adibiu á la suya lista de seguimiento',
'addedwatchtext'       => "A pachina «[[:\$1]]» s'ha adibito t'a suya [[Special:Watchlist|lista de seguimiento]]. Os cambios esdebenideros en ista pachina y en a suya pachina de descusión asoziata s'endicarán astí, y a pachina amanixerá '''en negreta''' en a [[Special:RecentChanges|lista de cambeos rezients]] ta que se beiga millor. <p>Si nunca quiere borrar a pachina d'a suya lista de seguimiento, punche \"Deixar de cosirar\" en o menú.",
'removedwatch'         => "Borrata d'a lista de seguimiento",
'removedwatchtext'     => 'A pachina "[[:$1]]" ha estato borrata d\'a suya lista de seguimiento.',
'watch'                => 'Cosirar',
'watchthispage'        => 'Cosirar ista pachina',
'unwatch'              => 'Deixar de cosirar',
'unwatchthispage'      => 'Deixar de cosirar',
'notanarticle'         => 'No ye una pachina de conteniu',
'notvisiblerev'        => "S'ha borrau ixa bersión",
'watchnochange'        => "Dengún d'os articlos d'a suya lista de seguimiento no s'ha editoato en o periodo de tiempo amostrato.",
'watchlist-details'    => '{{PLURAL:$1|$1 pachina cosirata|$1 pachinas cosiratas}} (sin contar-ie as pachinas de descusión).',
'wlheader-enotif'      => '* A notificazión por correu eletronico ye autibata',
'wlheader-showupdated' => "* Las pachinas cambiadas dende a suya zaguer besita s'amuestran en '''negreta'''",
'watchmethod-recent'   => 'Mirando pachinas cosiratas en os zaguers cambeos',
'watchmethod-list'     => 'mirando edizions rezients en as pachinas cosiratas',
'watchlistcontains'    => 'A suya lista de seguimiento tiene $1 {{PLURAL:$1|pachina|pachinas}}.',
'iteminvalidname'      => "Bi ha un problema con l'articlo '$1', o nombre no ye conforme...",
'wlnote'               => "Contino se i {{PLURAL:$1|amuestra o zaguer cambeo|amuestran os zaguers '''$1''' cambeos}} en {{PLURAL:$2|a zaguer ora|as zagueras '''$2''' oras}}.",
'wlshowlast'           => 'Amostrar as zagueras $1 horas, $2 días u $3',
'watchlist-show-bots'  => 'Amostrar as edizions feitas por bots',
'watchlist-hide-bots'  => 'Amagar as edizions de bots',
'watchlist-show-own'   => 'Amostrar as mías edizions',
'watchlist-hide-own'   => 'Amagar as mías edizions',
'watchlist-show-minor' => 'Amostrar as edizions menors',
'watchlist-hide-minor' => 'Amagar edizions menors',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Cosirando...',
'unwatching' => 'Deixar de cosirar...',

'enotif_mailer'                => 'Sistema de notificazión por correu de {{SITENAME}}',
'enotif_reset'                 => 'Marcar todas as pachinas como besitatas',
'enotif_newpagetext'           => 'Ista ye una nueba pachina.',
'enotif_impersonal_salutation' => 'usuario de {{SITENAME}}',
'changed'                      => 'editata',
'created'                      => 'creyata',
'enotif_subject'               => 'A pachina $PAGETITLE de {{SITENAME}} ha estato $CHANGEDORCREATED por $PAGEEDITOR',
'enotif_lastvisited'           => 'Baiga ta $1 ta beyer toz os cambeos dende a suya zaguer besita.',
'enotif_lastdiff'              => 'Baiga ta $1 ta beyer iste cambeo.',
'enotif_anon_editor'           => 'usuario anonimo $1',
'enotif_body'                  => 'Quiesto/a $WATCHINGUSERNAME,

A pachina «$PAGETITLE» de {{SITENAME}}
ha estato $CHANGEDORCREATED por l\'usuario $PAGEEDITOR o $PAGEEDITDATE.
Puede trobar a bersión autual en {{fullurl:$PAGETITLE}}

$NEWPAGE

O resumen d\'a edizión ye: $PAGESUMMARY $PAGEMINOREDIT

Ta comunicar-se con l\'usuario:
por correu: {{fullurl:Special:Emailuser|target=$PAGEEDITOR}}
en o wiki: {{fullurl:User:$PAGEEDITOR}}

Ta recullir nuebas notificazions de cambios d\'ista pachina abrá de besitar-la nuebament.
Tamién puede cambiar, en a su lista de seguimiento, as opzions de notificazión d\'as pachinas que ye cosirando.

Atentament,
 O sistema de notificazión de {{SITENAME}}.

--
Ta cambiar as opzions d\'a suya lista de seguimiento en:
{{fullurl:Special:Watchlist|edit=yes}}

Ta obtenir más informazión y aduya:
{{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => 'Borrar ista pachina',
'confirm'                     => 'Confirmar',
'excontent'                   => "O conteniu yera: '$1'",
'excontentauthor'             => "O conteniu yera: '$1' (y o suyo unico autor '$2')",
'exbeforeblank'               => "O conteniu antis de blanquiar yera: '$1'",
'exblank'                     => 'a pachina yera bueda',
'delete-confirm'              => 'Borrar "$1"',
'delete-legend'               => 'Borrar',
'historywarning'              => 'Pare cuenta: A pachina que ba a borrar tiene un istorial de cambeos:',
'confirmdeletetext'           => "Ye amanato á borrar d'a base de datos una pachina con tot o suyo istorial.
Por fabor, confirme que reyalment ye mirando de fer ixo, que entiende as consecuenzias, y que lo fa d'alcuerdo con as [[{{MediaWiki:Policy-url}}|politicas]] d'o wiki.",
'actioncomplete'              => 'Aizión rematada',
'deletedtext'                 => '"<nowiki>$1</nowiki>" ha estato borrato.
Se beiga en $2 un rechistro d\'os borraus rezients.',
'deletedarticle'              => 'borrato "$1"',
'suppressedarticle'           => 's\'ha supreso "[[$1]]"',
'dellogpage'                  => 'Rechistro de borraus',
'dellogpagetext'              => "Contino se i amuestra una lista d'os borraus más rezients.",
'deletionlog'                 => 'rechistro de borraus',
'reverted'                    => 'Tornato ta una bersión anterior',
'deletecomment'               => 'Razón ta borrar:',
'deleteotherreason'           => 'Otras/Más razons:',
'deletereasonotherlist'       => 'Otra razón',
'deletereason-dropdown'       => "*Razons comuns de borrau
** Á demanda d'o mesmo autor
** trencadura de copyright
** Bandalismo",
'delete-edit-reasonlist'      => "Editar as razons d'o borrau",
'delete-toobig'               => "Ista pachina tiene un istorial d'edizión prou largo, con más de $1 {{PLURAL:$1|bersión|bersions}}. S'ha restrinchito o borrau d'ista mena de pachinas ta aprebenir d'a corrompizión azidental de {{SITENAME}}.",
'delete-warning-toobig'       => "Ista pachina tiene un istorial d'edizión prou largo, con más de $1 {{PLURAL:$1|bersión|bersions}}. Si la borra puede corromper as operazions d'a base de datos de {{SITENAME}}; contine con ficazio.",
'rollback'                    => 'Esfer edizions',
'rollback_short'              => 'Esfer',
'rollbacklink'                => 'Esfer',
'rollbackfailed'              => "No s'ha puesto esfer",
'cantrollback'                => "No se pueden esfer as edizions; o zaguer colaborador ye o unico autor d'iste articlo.",
'alreadyrolled'               => 'No se puede esfer a zaguer edizión de [[:$1]] feita por [[User:$2|$2]] ([[User talk:$2|descusión]]|[[Special:Contributions/$2|{{int:contribslink}}]]); belatro usuario ya ha editato u esfeito una edizión en ixa pachina. 

A zaguer edizión la fazió [[User:$3|$3]] ([[User talk:$3|descusión]]|[[Special:Contributions/$3|{{int:contribslink}}]]).',
'editcomment'                 => 'O comentario d\'a edizión ye: "<i>$1</i>".', # only shown if there is an edit comment
'revertpage'                  => "S'han esfeito as edizions de [[Special:Contributions/$2|$2]] ([[User talk:$2|Descusión]]); retornando t'a zaguera bersión editada por [[User:$1|$1]]", # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => "Esfeitas as edizions de $1; s'ha retornato á la zaguer bersión de $2.",
'sessionfailure'              => 'Pareix que bi ha un problema con a suya sesión;
s\'ha anulato ista aizión como mida de precura contra secuestros de sesión.
Por fabor, prete "Entazaga", recargue a pachina d\'a que benió, y torne á prebar alabez.',
'protectlogpage'              => 'Protezions de pachinas',
'protectlogtext'              => 'Contino se i amuestra una lista de protezions y esprotezions de pachinas. Se beiga [[Special:ProtectedPages|lista de pachinas protechitas]] ta más informazión.',
'protectedarticle'            => "s'ha protechito [[$1]]",
'modifiedarticleprotection'   => 's\'ha cambiato o libel de protezión de "[[$1]]"',
'unprotectedarticle'          => "s'ha esprotechito [[$1]]",
'protect-title'               => 'Protechendo "$1"',
'protect-legend'              => 'Confirmar protezión',
'protectcomment'              => 'Razón:',
'protectexpiry'               => 'Calendata de caduzidat:',
'protect_expiry_invalid'      => 'Tiempo de zircunduzión incorreuto.',
'protect_expiry_old'          => 'O tiempo de caduzidat ye una calendata ya pasata.',
'protect-unchain'             => 'Confegurar premisos ta treslaus',
'protect-text'                => "Puede beyer y cambiar o libel e protezión d'a pachina <strong><nowiki>$1</nowiki></strong>.",
'protect-locked-blocked'      => "No puede cambiar os libels de protezión mientres ye bloqueyato. Contino se i amuestran as opzions autuals d'a pachina <strong>$1</strong>:",
'protect-locked-dblock'       => "Os libels de protezión no se pueden cambiar por un bloqueyo autibo d'a base de datos.
Contino se i amuestran as opzions autuals d'a pachina <strong>$1</strong>:",
'protect-locked-access'       => "A suya cuenta no tiene premiso ta cambiar os libels de protezión d'as pachinas. Aquí bi son as propiedaz autuals d'a pachina <strong>$1</strong>:",
'protect-cascadeon'           => "Ista pachina ye autualment protechita por estar encluyita en {{PLURAL:$1|a siguient pachina|as siguients pachinas}}, que tienen autibata a opzión de protezión en cascada. Puede cambiar o libel de protezión d'ista pachina, pero no afeutará á la protezión en cascada.",
'protect-default'             => '(por defeuto)',
'protect-fallback'            => 'Amenista o premiso "$1"',
'protect-level-autoconfirmed' => 'bloqueyar usuarios no rechistratos',
'protect-level-sysop'         => 'Sólo almenistradors',
'protect-summary-cascade'     => 'en cascada',
'protect-expiring'            => 'caduca o $1 (UTC)',
'protect-cascade'             => 'Protezión en cascada - protecher totas as pachinas encluyidas en ista.',
'protect-cantedit'            => "No puede cambiar os libels de protezión d'ista pachina, porque no tiene premiso ta editar-la.",
'restriction-type'            => 'Premiso:',
'restriction-level'           => 'Libel de restrizión:',
'minimum-size'                => 'Grandaria menima',
'maximum-size'                => 'Grandaria maisima:',
'pagesize'                    => '(bytes)',

# Restrictions (nouns)
'restriction-edit'   => 'Editar',
'restriction-move'   => 'Tresladar',
'restriction-create' => 'Creyar',
'restriction-upload' => 'Carga',

# Restriction levels
'restriction-level-sysop'         => 'protechita de tot',
'restriction-level-autoconfirmed' => 'semiprotechita',
'restriction-level-all'           => 'cualsiquier libel',

# Undelete
'undelete'                     => 'Beyer pachinas borratas',
'undeletepage'                 => 'Beyer y restaurar pachinas borratas',
'undeletepagetitle'            => "'''Contino s'amuestran as bersión borratas de [[:$1]]'''.",
'viewdeletedpage'              => 'Beyer pachinas borratas',
'undeletepagetext'             => "As pachinas siguiens han siu borradas, pero encara son en l'archibo y podría estar restauradas. El archibo se borra periodicamén.",
'undelete-fieldset-title'      => 'Restaurar bersions',
'undeleteextrahelp'            => "Ta restaurar a pachina antera con todas as bersions, deixe todas as caixetas sin siñalar y prete '''''Restaurar!'''''. Ta restaurar sólo belunas d'as bersions, siñale as caixetas correspondients á las bersions que quiere restaurar y punche dimpués '''''Restaurar!'''''. Punchando '''''Prenzipiar''''' se borrará o comentario y se tirarán os siñals d'as caixetas.",
'undeleterevisions'            => '$1 {{PLURAL:$1|bersión|bersions}} archibatas',
'undeletehistory'              => "Si restaura a pachina, se restaurarán  todas as bersions en o suyo istorial. 
Si s'ha creyato una nueba pachina con o mesmo nombre dende que se borró a orichinal, as bersions restauradas amaneixerán antes en o istorial.",
'undeleterevdel'               => "No s'esfará o borrau si isto resulta en o borrau parzial d'a pachina d'alto u a rebisión de l'archibo. En ixe caso, deselezione u amuestre as bersions borratas más rezients.",
'undeletehistorynoadmin'       => "Esta pachina ye borrata. A razón d'o suyo borrau s'amuestra más t'abaixo en o resumen, asinas como os detalles d'os usuarios que eban editato a pachina antes d'o borrau. O testo completo d'istas edizions borratas ye disponible nomás ta os almenistradors.",
'undelete-revision'            => "S'ha borrato a bersión de $1 de $2 (por $3):",
'undeleterevision-missing'     => "Bersión no conforme u no trobata. Regular que o binclo sia incorreuto u que a bersión aiga estato restaurata u borrata de l'archibo.",
'undelete-nodiff'              => "No s'ha trobato garra bersión anterior.",
'undeletebtn'                  => 'Restaurar!',
'undeletelink'                 => 'restaurar',
'undeletereset'                => 'Prenzipiar',
'undeletecomment'              => 'Razón ta restaurar:',
'undeletedarticle'             => 'restaurata "$1"',
'undeletedrevisions'           => '{{PLURAL:$1|Una edizión restaurata|$1 edizions restauratas}}',
'undeletedrevisions-files'     => '$1 {{PLURAL:$1|rebisión|rebisions}} y $2 {{PLURAL:$2|archibo|archibos}} restauratos',
'undeletedfiles'               => '$1 {{PLURAL:$1|archibo restaurato|archibos restauratos}}',
'cannotundelete'               => "No s'ha puesto esfer o borrau; belatro usuario puede aber esfeito antis o borrau.",
'undeletedpage'                => "<big>'''S'ha restaurato $1'''</big>

Consulte o [[Special:Log/delete|rechistro de borraus]] ta beyer una lista d'os zaguers borraus y restaurazions.",
'undelete-header'              => 'En o [[Special:Log/delete|rechistro de borraus]] se listan as pachina borratas fa poco tiempo.',
'undelete-search-box'          => 'Mirar en as pachinas borratas',
'undelete-search-prefix'       => 'Amostrar as pachinas que prenzipien por:',
'undelete-search-submit'       => 'Mirar',
'undelete-no-results'          => "No s'han trobato pachinas borratas con ixos criterios.",
'undelete-filename-mismatch'   => "No se pueden restaurar a rebisión d'archibo con calendata $1: o nombre d'archibo no consona",
'undelete-bad-store-key'       => "No se puede restaurar a bersión de l'archibo con calendata $1: l'archibo ya no se i trobaba antis d'o borrau.",
'undelete-cleanup-error'       => 'Bi abió una error mientres se borraba l\'archibo "$1".',
'undelete-missing-filearchive' => "No ye posible restaurar l'archibo con ID $1 porque no bi ye en a base de datos. Puede que ya s'aiga restaurato.",
'undelete-error-short'         => "Error mientres se restauraba l'archibo: $1",
'undelete-error-long'          => 'Bi abió errors mientres se borraban os archibos:

$1',

# Namespace form on various pages
'namespace'      => 'Espazio de nombres:',
'invert'         => 'Contornar selezión',
'blanknamespace' => '(Prenzipal)',

# Contributions
'contributions' => "Contrebuzions de l'usuario",
'mycontris'     => 'Contrebuzions',
'contribsub2'   => 'De $1 ($2)',
'nocontribs'    => "No s'han trobato cambeos que concordasen con ixos criterios",
'uctop'         => '(zaguer cambeo)',
'month'         => 'Dende o mes (y anteriors):',
'year'          => "Dende l'año (y anteriors):",

'sp-contributions-newbies'     => "Amostrar nomás as contrebuzions d'os usuarios nuebos",
'sp-contributions-newbies-sub' => 'Por usuarios nuebos',
'sp-contributions-blocklog'    => 'Rechistro de bloqueyos',
'sp-contributions-search'      => 'Mirar contrebuzions',
'sp-contributions-username'    => "Adreza IP u nombre d'usuario:",
'sp-contributions-submit'      => 'Mirar',

# What links here
'whatlinkshere'            => 'Pachinas que enlazan con ista',
'whatlinkshere-title'      => 'Pachinas que tienen binclos ta $1',
'whatlinkshere-page'       => 'Pachina:',
'linklistsub'              => '(Lista de binclos)',
'linkshere'                => "As siguients pachinas tienen binclos enta '''[[:$1]]''':",
'nolinkshere'              => "Denguna pachina tiene binclos ta '''[[:$1]]'''.",
'nolinkshere-ns'           => "Denguna pachina d'o espazio de nombres trigato tiene binclos ta '''[[:$1]]'''.",
'isredirect'               => 'pachina reendrezata',
'istemplate'               => 'encluyida',
'isimage'                  => 'binclo ta imachen',
'whatlinkshere-prev'       => '{{PLURAL:$1|anterior|anteriors $1}}',
'whatlinkshere-next'       => '{{PLURAL:$1|siguient|siguients $1}}',
'whatlinkshere-links'      => '← binclos',
'whatlinkshere-hideredirs' => '$1 reendrezeras',
'whatlinkshere-hidetrans'  => '$1 transclusions',
'whatlinkshere-hidelinks'  => '$1 binclos',
'whatlinkshere-hideimages' => '$1 binclos ta imachens',
'whatlinkshere-filters'    => 'Filtros',

# Block/unblock
'blockip'                         => 'Bloqueyar usuario',
'blockip-legend'                  => 'Bloqueyar usuario',
'blockiptext'                     => "Replene o siguient formulario ta bloqueyar l'azeso
d'escritura dende una cuenta d'usuario u una adreza IP espezifica.
Isto abría de fer-se sólo ta pribar bandalismos, y d'alcuerdo con
as [[{{MediaWiki:Policy-url}}|politicas]].
Escriba a razón espezifica ta o bloqueyo (por exemplo, cuaternando
as pachinas que s'han bandalizato).",
'ipaddress'                       => 'Adreza IP',
'ipadressorusername'              => "Adreza IP u nombre d'usuario",
'ipbexpiry'                       => 'Zircunduzión:',
'ipbreason'                       => 'Razón:',
'ipbreasonotherlist'              => 'Atra razón',
'ipbreason-dropdown'              => "*Razons comuns de bloqueyo
** Meter informazión falsa
** Borrar conteniu d'as pachinas
** Fer publizidat ficando binclos con atras pachinas web
** Meter sinconisions u basuera en as pachinas
** Portar-se de traza intimidatoria u biolenta / atosegar
** Abusar de multiples cuentas
** Nombre d'usuario inazeutable",
'ipbanononly'                     => 'Bloqueyar nomás os usuarios anonimos',
'ipbcreateaccount'                => "Aprebenir a creyazión de cuentas d'usuario.",
'ipbemailban'                     => 'Pribar que os usuarios nimbíen correus electronicos',
'ipbenableautoblock'              => "bloqueyar automaticament l'adreza IP emplegata por iste usuario, y cualsiquier IP posterior dende a que prebe d'editar",
'ipbsubmit'                       => 'bloqueyar á iste usuario',
'ipbother'                        => 'Espezificar atro periodo',
'ipboptions'                      => '2 oras:2 hours,1 día:1 day,3 días:3 days,1 semana:1 week,2 semanas:2 weeks,1 mes:1 month,3 meses:3 months,6 meses:6 months,1 año:1 year,ta cutio:infinite', # display1:time1,display2:time2,...
'ipbotheroption'                  => 'atro',
'ipbotherreason'                  => 'Razons diferens u adizionals',
'ipbhidename'                     => "Amagar usuario en o rechistro de bloqueyos, a lista de bloqueyos autibos y a lista d'usuarios",
'ipbwatchuser'                    => "Cosirar as pachinas d'usuario y de descusión d'iste usuario",
'badipaddress'                    => "L'adreza IP no ye conforme.",
'blockipsuccesssub'               => "O bloqueyo s'ha feito correutament",
'blockipsuccesstext'              => "L'adreza IP [[Special:Contributions/$1|$1]] ye bloqueyata. <br />Ir t'a [[Special:IPBlockList|lista d'adrezas IP bloqueyatas]] ta beyer os bloqueyos.",
'ipb-edit-dropdown'               => "Editar as razons d'o bloqueyo",
'ipb-unblock-addr'                => 'Esbloqueyar $1',
'ipb-unblock'                     => 'Esbloqueyar un usuario u una IP',
'ipb-blocklist-addr'              => 'Amostrar bloqueyos autuals de $1',
'ipb-blocklist'                   => 'Amostrar bloqueyos autuals',
'unblockip'                       => 'Esbloqueyar usuario',
'unblockiptext'                   => "Replene o formulario que bi ha contino ta tornar os premisos d'escritura ta una adreza IP u cuenta d'usuario que aiga estato bloqueyata.",
'ipusubmit'                       => 'Esbloqueyar ista adreza',
'unblocked'                       => '[[User:$1|$1]] ha estato esbloqueyato',
'unblocked-id'                    => "S'ha sacato o bloqueyo $1",
'ipblocklist'                     => "Adrezas IP y nombres d'usuario bloqueyatos",
'ipblocklist-legend'              => 'Mirar un usuario bloqueyato',
'ipblocklist-username'            => "Nombre d'usuario u adreza IP:",
'ipblocklist-submit'              => 'Mirar',
'blocklistline'                   => '$1, $2 ha bloqueyato á $3 ($4)',
'infiniteblock'                   => 'infinito',
'expiringblock'                   => 'zircunduze o $1',
'anononlyblock'                   => 'nomás anon.',
'noautoblockblock'                => 'Bloqueyo automatico desautibato',
'createaccountblock'              => "S'ha bloqueyato a creyazión de nuebas cuentas",
'emailblock'                      => "S'ha bloqueyato o nimbió de correus electronicos",
'ipblocklist-empty'               => 'A lista de bloqueyos ye bueda.',
'ipblocklist-no-results'          => "A cuenta d'usuario u adreza IP endicata no ye bloqueyata.",
'blocklink'                       => 'bloqueyar',
'unblocklink'                     => 'esbloqueyar',
'contribslink'                    => 'contrebuzions',
'autoblocker'                     => 'Ye bloqueyato automaticament porque a suya adreza IP l\'ha feito serbir rezientement "[[User:$1|$1]]". A razón data ta bloqueyar á "[[User:$1|$1]]" estió "$2".',
'blocklogpage'                    => 'Rechistro de bloqueyos',
'blocklogentry'                   => "S'ha bloqueyato á [[$1]] con una durada de $2 $3",
'blocklogtext'                    => "Isto ye un rechistro de bloqueyos y esbloqueyos d'usuarios. As adrezas bloqueyatas automaticament no amaneixen aquí. Mire-se a [[Special:IPBlockList|lista d'adrezas IP bloqueyatas]] ta beyer a lista autual de biedas y bloqueyos.",
'unblocklogentry'                 => 'ha esbloqueyato á "$1"',
'block-log-flags-anononly'        => 'nomás os usuarios anonimos',
'block-log-flags-nocreate'        => "s'ha desautibato a creyazión de cuentas",
'block-log-flags-noautoblock'     => "s'ha desautibato o bloqueyo automatico",
'block-log-flags-noemail'         => "s'ha desautibato o nimbío de mensaches por correu electronico",
'block-log-flags-angry-autoblock' => "s'ha autibato l'autobloqueyo amillorato",
'range_block_disabled'            => "A posibilidat d'os almenistradors de bloqueyar rangos d'adrezas IP ye desautibata.",
'ipb_expiry_invalid'              => 'O tiempo de zircunduzión no ye conforme.',
'ipb_expiry_temp'                 => "Os bloqueyos con nombre d'usuario amagato abría d'estar ta cutio.",
'ipb_already_blocked'             => '"$1" ya yera bloqueyato',
'ipb_cant_unblock'                => "'''Error''': no s'ha trobato o ID de bloqueyo $1. Talment sía ya esbloqueyato.",
'ipb_blocked_as_range'            => "Error: L'adreza IP $1 no s'ha bloqueyato dreitament y por ixo no se puede esbloqueyar. Manimenos, ye bloqueyata por estar parte d'o rango $2, que sí buede esbloqueyar-se de conchunta.",
'ip_range_invalid'                => "O rango d'adrezas IP no ye conforme.",
'blockme'                         => 'bloqueyar-me',
'proxyblocker'                    => 'bloqueyador de proxies',
'proxyblocker-disabled'           => 'Ista funzión ye desautibata.',
'proxyblockreason'                => "S'ha bloqueyato a suya adreza IP porque ye un proxy ubierto. Por fabor, contaute on o suyo furnidor de serbizios d'Internet u con o suyo serbizio d'asistenzia tecnica e informe-les d'iste grau problema de seguridat.",
'proxyblocksuccess'               => 'Feito.',
'sorbsreason'                     => 'A suya adreza IP ye en a lista de proxies ubiertos en a DNSBL de {{SITENAME}}.',
'sorbs_create_account_reason'     => 'A suya adreza IP ye en a lista de proxies ubiertos en a DNSBL de {{SITENAME}}. No puede creyar una cuenta',

# Developer tools
'lockdb'              => 'Trancar a base de datos',
'unlockdb'            => 'Estrancar a base de datos',
'lockdbtext'          => "Trancando a base de datos pribará á toz os usuarios d'editar pachinas, cambiar as preferenzias, cambiar as listas de seguimiento y cualsiquier atra funzión que ameniste fer cambios en a base de datos. Por fabor, confirme que isto ye mesmament o que se mira de fer y que estrancará a base de datos malas que aiga rematato con a faina de mantenimiento.",
'unlockdbtext'        => "Estrancando a base de datos premitirá á toz os usuarios d'editar pachinas, cambiar as preferenzias y as listas de seguimiento, y cualsiquier atra funzión que ameniste cambiar a base de datos. Por fabor, confirme que isto ye mesmament o que se mira de fer.",
'lockconfirm'         => 'Sí, de berdat quiero trancar a base de datos.',
'unlockconfirm'       => 'Sí, de berdat quiero estrancar a base de datos.',
'lockbtn'             => 'Trancar a base de datos',
'unlockbtn'           => 'Estrancar a base de datos',
'locknoconfirm'       => 'No ha siñalato a caixeta de confirmazión.',
'lockdbsuccesssub'    => "A base de datos s'ha trancato correutament",
'unlockdbsuccesssub'  => "A base de datos s'ha estrancato correutament",
'lockdbsuccesstext'   => "Ha trancato a base de datos de {{SITENAME}}.
Alcuerde-se-ne d'[[Special:UnlockDB|estrancar a base de datos]] dimpués de rematar as fayenas de mantenimiento.",
'unlockdbsuccesstext' => "S'ha estrancato a base de datos de {{SITENAME}}.",
'lockfilenotwritable' => "O rechistro de trancamientos d'a base de datos no tiene premiso d'escritura. Ta trancar u estrancar a base de datos, iste archibo ha de tener premisos d'escritura en o serbidor web.",
'databasenotlocked'   => 'A base de datos no ye trancata.',

# Move page
'move-page'               => 'Tresladar $1',
'move-page-legend'        => 'Tresladar pachina',
'movepagetext'            => "Fendo serbir o formulario siguient se cambiará o nombre d'a pachina, tresladando tot o suyo istorial t'o nuebo nombre.
O títol anterior se tornará en una reendrezera ta o nuebo títol.
Puede esbiellar automaticament as reendrezeras que plegan ta o títol orichina.
Si s'estima más de no fer-lo, asegure-se de no deixar [[Special:DoubleRedirects|reendrezeras dobles]] u [[Special:BrokenRedirects|crebatas]].
Ye a suya responsabilidat d'asegurar-se que os binclos continan endrezando t'an que abrían de fer-lo.

Remere que a pachina '''no''' se renombrará si ya esiste una pachina con o nuebo títol, si no ye que estase una pachina bueda u una ''reendrezera'' sin istorial.
Isto senifica que podrá tresladar una pachina á lo suyo títol orichinal si ha feito una error, pero no podrá escribir denzima d'una pachina ya esistent.

'''¡PARE CUENTA!'''
Iste puede estar un cambio drastico e inasperato ta una pachina popular;
por fabor, asegure-se d'acatar as consecuenzias que acarriará ista aizión antis de seguir entadebant.",
'movepagetalktext'        => "A pachina de descusión asoziata será tresladata automaticament '''de no estar que:'''

*Ya esista una pachina de descusión no bueda con o nombre nuebo, u
*Desautibe a caixeta d'abaxo.

En ixos casos, si lo deseya, abrá de tresladar u combinar manualment o conteniu d'a pachina de descusión.",
'movearticle'             => 'Tresladar pachina:',
'movenotallowed'          => 'No tiene premisos ta tresladar pachinas.',
'newtitle'                => 'Ta o nuebo títol',
'move-watch'              => 'Cosirar iste articlo',
'movepagebtn'             => 'Tresladar pachina',
'pagemovedsub'            => 'Treslado feito correutament',
'movepage-moved'          => "<big>S'ha tresladato '''\"\$1\"  ta \"\$2\"'''</big>", # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => 'Ya bi ha una pachina con ixe nombre u o nombre que ha eslechito no ye conforme. Por fabor trigue un atro nombre.',
'cantmove-titleprotected' => 'No puede tresladar una pachina ta íste títol porque o nuebo títol ye protechito y no puede estar creyato',
'talkexists'              => "A pachina s'ha tresladato correutament, pero a descusión no s'ha puesto tresladar porque ya esiste una con o nuebo títol. Por fabor, encorpore manualment o suyo conteniu.",
'movedto'                 => 'tresladato ta',
'movetalk'                => 'Tresladar a pachina de descusión asoziata.',
'move-subpages'           => 'Tresladar todas as sozpachinas (si ye posible)',
'move-talk-subpages'      => "Tresladar todas as sozpachinas d'a descusión (si ye posible)",
'movepage-page-exists'    => 'A pachina $1 ya esiste y no se puede sobrescribir automaticament.',
'movepage-page-moved'     => "S'ha tresladato a pachina $1 ta $2.",
'movepage-page-unmoved'   => "No s'ha puesto tresladar a pachina $1 ta $2.",
'movepage-max-pages'      => "S'han tresladato o masimo posible de $1 {{PLURAL:$1|pachina|pachinas}} y no se tresladarán más automaticament.",
'1movedto2'               => '[[$1]] tresladada á [[$2]]',
'1movedto2_redir'         => '[[$1]] tresladada á [[$2]] sobre una reendrezera',
'movelogpage'             => 'Rechistro de treslatos',
'movelogpagetext'         => 'Contino se i amuestra una lista de pachinas tresladatas.',
'movereason'              => 'Razón:',
'revertmove'              => 'esfer',
'delete_and_move'         => 'Borrar y tresladar',
'delete_and_move_text'    => '==S\'amenista borrar a pachina==

A pachina de destino ("[[:$1]]") ya esiste. Quiere borrar-la ta premitir o treslau?',
'delete_and_move_confirm' => 'Sí, borrar a pachina',
'delete_and_move_reason'  => 'Borrata ta premitir o treslau',
'selfmove'                => "Os títols d'orichen y destino son os mesmos. No se puede tresladar una pachina ta ella mesma.",
'immobile_namespace'      => "O títol de destino ye d'una mena espezial. No se puede tresladar pachinas ta iste espazio de nombres.",
'imagenocrossnamespace'   => "No se puede tresladar un archibo ta un espazio de nombres que no sía t'archibos",
'imagetypemismatch'       => "A nueba estensión no concuerda con o tipo d'archibo",
'imageinvalidfilename'    => "O nombre de l'archibo obchetibo no ye conforme",
'fix-double-redirects'    => 'Esbiellar todas as reendrezeras que plegan ta o títol orichinal',

# Export
'export'            => 'Esportar as pachinas',
'exporttext'        => "Puede esportar o testo y l'istorial d'edizions d'una pachina u conchunto de pachinas ta un testo XML. Iste testo XML puede importar-se ta atro wiki que faiga serbir MediaWiki á trabiés d'a [[Special:Import|pachina d'importazión]].

Ta esportar pachinas, escriba os títols en a caixa de testo que bi ha más ta baixo, metendo un títol en cada linia, y eslicha si quiere esportar a bersión autual con as bersions anteriors y as lineas de l'istorial u nomás a bersión autual con a informazión sobre a zaguer edizión.

En iste zaguer caso tamién puede usar un binclo, por exemplo [[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]] t'a pachina \"[[{{MediaWiki:Mainpage}}]]\".",
'exportcuronly'     => "Encluye nomás a bersión autual, no l'istorial de bersions completo.",
'exportnohistory'   => "----
'''Nota:''' A esportazión de istorials de pachinas á trabiés d'iste formulario ye desautibata por problemas en o rendimiento d'o serbidor.",
'export-submit'     => 'Esportar',
'export-addcattext' => 'Adibir pachinas dende a categoría:',
'export-addcat'     => 'Adibir',
'export-download'   => 'Alzar como un archibo',
'export-templates'  => 'Encluyir-ie plantillas',

# Namespace 8 related
'allmessages'               => "Mensaches d'o sistema",
'allmessagesname'           => 'Nombre',
'allmessagesdefault'        => 'Testo por defeuto',
'allmessagescurrent'        => 'Testo autual',
'allmessagestext'           => "Ista ye una lista de toz os mensaches disponibles en o espazio de nombres MediaWiki.
Besite por fabor [http://www.mediawiki.org/wiki/Localisation a pachina sobre localizazión de MediaWiki] y  [http://translatewiki.net Betawiki] si deseya contrebuyir t'a localizazión cheneral de MediaWiki.",
'allmessagesnotsupportedDB' => 'Ista pachina no ye disponible porque wgUseDatabaseMessages ye desautibato.',
'allmessagesfilter'         => "Filtrar por a etiqueta d'os mensaches:",
'allmessagesmodified'       => 'Amostrar nomás os mensaches cambiatos',

# Thumbnails
'thumbnail-more'           => 'Fer más gran',
'filemissing'              => 'Archibo no trobato',
'thumbnail_error'          => "S'ha produzito una error en creyar a miniatura: $1",
'djvu_page_error'          => "Pachina DjVu difuera d'o rango",
'djvu_no_xml'              => "No s'ha puesto replegar o XML ta l'archibo DjVu",
'thumbnail_invalid_params' => "Os parametros d'as miniatura no son correutos",
'thumbnail_dest_directory' => "No s'ha puesto creyar o direutorio de destino",

# Special:Import
'import'                     => 'Importar pachinas',
'importinterwiki'            => 'Importazión interwiki',
'import-interwiki-text'      => "Trigue un wiki y un títol de pachina ta importar.
As calendatas d'as bersions y os nombres d'editors se mantendrán.
Todas as importazions interwiki se rechistran en o [[Special:Log/import|rechistro d'importazions]].",
'import-interwiki-history'   => "Copiar todas as bersions de l'istorial d'ista pachina",
'import-interwiki-submit'    => 'Importar',
'import-interwiki-namespace' => "Transferir pachinas t'o espazio de nombres:",
'importtext'                 => "Por fabor, esporte l'archibo dende o wiki d'orichen fendo serbir a [[Special:Export|ferramienta d'esportazión]]. Alze-lo en o suyo ordenador y cargue-lo aquí.",
'importstart'                => 'Importando pachinas...',
'import-revision-count'      => '$1 {{PLURAL:$1|bersión|bersions}}',
'importnopages'              => 'No bi ha garra pachina ta importar.',
'importfailed'               => 'Ha fallato a importazión: $1',
'importunknownsource'        => "O tipo de fuent d'a importazión ye esconoixito",
'importcantopen'             => "No s'ha puesto importar iste archibo",
'importbadinterwiki'         => 'Binclo interwiki incorreuto',
'importnotext'               => 'Buendo y sin de testo',
'importsuccess'              => "S'ha rematato a importazión!",
'importhistoryconflict'      => "Bi ha un conflito de bersions en o istorial (talment ista pachina s'aiga importata antes)",
'importnosources'            => "No bi ha fuents d'importazión interwiki y no ye premitito cargar o istorial dreitament.",
'importnofile'               => "No s'ha cargato os archibos d'importazión.",
'importuploaderrorsize'      => "Ha fallato a carga de l'archibo importato. L'archibo brinca d'a grandaria de carga premitita.",
'importuploaderrorpartial'   => "Ha fallato a carga de l'archibo importato. Sólo una parte de l'archibo s'ha cargato.",
'importuploaderrortemp'      => "Ha fallato a carga de l'archibo importato. No se troba o direutorio temporal.",
'import-parse-failure'       => "Fallo en o parseyo d'a importazión XML",
'import-noarticle'           => 'No bi ha garra pachina ta importar!',
'import-nonewrevisions'      => "Ya s'eban importato denantes todas as bersions.",
'xml-error-string'           => '$1 en a linia $2, col $3 (byte $4): $5',
'import-upload'              => 'Datos XML cargatos',

# Import log
'importlogpage'                    => "Rechistro d'importazions",
'importlogpagetext'                => 'Importazions almenistratibas de pachinas con istorial dende atros wikis.',
'import-logentry-upload'           => 'importata [[$1]] cargando un archibo',
'import-logentry-upload-detail'    => '$1 {{PLURAL:$1|bersión|bersions}}',
'import-logentry-interwiki'        => 'Importata $1 entre wikis',
'import-logentry-interwiki-detail' => '$1 {{PLURAL:$1|bersión|bersions}} dende $2',

# Tooltip help for the actions
'tooltip-pt-userpage'             => "A mía pachina d'usuario",
'tooltip-pt-anonuserpage'         => "A pachina d'usuario de l'adreza IP dende a que ye editando",
'tooltip-pt-mytalk'               => 'A mía pachina de descusión',
'tooltip-pt-anontalk'             => 'Descusión sobre edizions feitas dende ista adreza IP',
'tooltip-pt-preferences'          => 'As mías preferenzias',
'tooltip-pt-watchlist'            => 'A lista de pachinas que en ye cosirando os cambeos',
'tooltip-pt-mycontris'            => "Lista d'as mías contribuzions",
'tooltip-pt-login'                => 'Li recomendamos rechistrar-se, encara que no ye obligatorio',
'tooltip-pt-anonlogin'            => 'Li alentamos á rechistrar-se, anque no ye obligatorio',
'tooltip-pt-logout'               => 'Rematar a sesión',
'tooltip-ca-talk'                 => "Descusión sobre l'articlo",
'tooltip-ca-edit'                 => 'Puede editar ista pachina. Por fabor, faga serbir o botón de bisualizazión prebia antes de grabar.',
'tooltip-ca-addsection'           => 'Adibir un comentario ta ista descusión',
'tooltip-ca-viewsource'           => 'Ista pachina ye protechita, nomás puede beyer o codigo fuent',
'tooltip-ca-history'              => "Bersions anteriors d'ista pachina.",
'tooltip-ca-protect'              => 'Protecher ista pachina',
'tooltip-ca-delete'               => 'Borrar ista pachina',
'tooltip-ca-undelete'             => 'Restaurar as edizions feitas á ista pachina antis que no estase borrata',
'tooltip-ca-move'                 => 'Tresladar (renombrar) ista pachina',
'tooltip-ca-watch'                => 'Adibir ista pachina á la suya lista de seguimiento',
'tooltip-ca-unwatch'              => "Borrar ista pachina d'a suya lista de seguimiento",
'tooltip-search'                  => 'Mirar en {{SITENAME}}',
'tooltip-search-go'               => "Ir t'a pachina con iste títol esauto, si esiste",
'tooltip-search-fulltext'         => 'Mirar iste testo en as pachinas',
'tooltip-p-logo'                  => 'Portalada',
'tooltip-n-mainpage'              => 'Besitar a Portalada',
'tooltip-n-portal'                => 'Sobre o procheuto, que puede fer, án trobar as cosas',
'tooltip-n-currentevents'         => 'Trobar informazión cheneral sobre escaizimientos autuals',
'tooltip-n-recentchanges'         => "A lista d'os zaguers cambeos en o wiki",
'tooltip-n-randompage'            => 'Cargar una pachina aleatoriament',
'tooltip-n-help'                  => 'O puesto ta saber más.',
'tooltip-t-whatlinkshere'         => "Lista de todas as pachinas d'o wiki binculatas con ista",
'tooltip-t-recentchangeslinked'   => 'Zaguers cambeos en as pachinas que tienen binclos enta ista',
'tooltip-feed-rss'                => "Canal RSS d'ista pachina",
'tooltip-feed-atom'               => "Canal Atom d'ista pachina",
'tooltip-t-contributions'         => "Beyer a lista de contrebuzions d'iste usuario",
'tooltip-t-emailuser'             => 'Nimbiar un correu electronico ta iste usuario',
'tooltip-t-upload'                => 'Cargar archibos',
'tooltip-t-specialpages'          => 'Lista de todas as pachinas espezials',
'tooltip-t-print'                 => "Bersión imprentable d'a pachina",
'tooltip-t-permalink'             => "Binclo permanet ta ista bersión d'a pachina",
'tooltip-ca-nstab-main'           => "Beyer l'articlo",
'tooltip-ca-nstab-user'           => "Beyer a pachina d'usuario",
'tooltip-ca-nstab-media'          => "Beyer a pachina d'o elemento multimedia",
'tooltip-ca-nstab-special'        => 'Ista ye una pachina espezial, y no puede editar-la',
'tooltip-ca-nstab-project'        => "Beyer a pachina d'o procheuto",
'tooltip-ca-nstab-image'          => "Beyer a pachina de l'archibo",
'tooltip-ca-nstab-mediawiki'      => 'Beyer o mensache de sistema',
'tooltip-ca-nstab-template'       => 'Beyer a plantilla',
'tooltip-ca-nstab-help'           => "Beyer a pachina d'aduya",
'tooltip-ca-nstab-category'       => "Beyer a pachina d'a categoría",
'tooltip-minoredit'               => 'Siñalar ista edizión como cambeo menor',
'tooltip-save'                    => 'Alzar os cambeos',
'tooltip-preview'                 => 'Rebise os suyos cambeos, por fabor, faga serbir isto antes de grabar!',
'tooltip-diff'                    => 'Amuestra os cambeos que ha feito en o testo.',
'tooltip-compareselectedversions' => "Beyer as esferenzias entre as dos bersions trigatas d'ista pachina.",
'tooltip-watch'                   => 'Adibir ista pachina á la suya lista de seguimiento',
'tooltip-recreate'                => 'Recreya una pachina mesmo si ya ha estato borrata dinantes',
'tooltip-upload'                  => 'Prenzipia a carga',

# Metadata
'nodublincore'      => 'Metadatos Dublin Core RDF desautibatos en iste serbidor.',
'nocreativecommons' => 'Metadatos Creative Commons RDF desautibatos en iste serbidor.',
'notacceptable'     => 'O serbidor wiki no puede ufrir os datos en un formato que o suyo client (nabegador) pueda leyer.',

# Attribution
'anonymous'        => 'Usuario(s) anonimo(s) de {{SITENAME}}',
'siteuser'         => 'Usuario $1 de {{SITENAME}}',
'lastmodifiedatby' => 'Ista pachina estió modificata por zaguer begada á $2, $1 por $3.', # $1 date, $2 time, $3 user
'othercontribs'    => 'Basato en o treballo de $1.',
'others'           => 'atros',
'siteusers'        => 'Usuario(s) $1 de {{SITENAME}}',
'creditspage'      => "Creditos d'a pachina",
'nocredits'        => 'No bi ha informazión de creditos ta ista pachina.',

# Spam protection
'spamprotectiontitle' => 'Filtro de protezión contra o spam',
'spamprotectiontext'  => "A pachina que mira d'alzar ha estato bloqueyata por o filtro de spam.  Regular que a causa sía en bel binclo esterno.",
'spamprotectionmatch' => 'O testo siguient ye o que autibó o nuestro filtro de spam: $1',
'spambot_username'    => 'Esporga de spam de MediaWiki',
'spam_reverting'      => "Tornando t'a zaguera bersión sin de binclos ta $1",
'spam_blanking'       => 'Todas as bersions contienen binclos ta $1, se blanquea a pachina',

# Info page
'infosubtitle'   => "Informazión d'a pachina",
'numedits'       => "Numero d'edizions (articlo): $1",
'numtalkedits'   => "Numero d'edizions (pachina de descusión): $1",
'numwatchers'    => "Número d'usuario cosirando: $1",
'numauthors'     => "Numero d'autors (articlo): $1",
'numtalkauthors' => "Numero d'autors (pachina de descusión): $1",

# Math options
'mw_math_png'    => 'Produzir siempre PNG',
'mw_math_simple' => "HTML si ye muit simple, si no'n ye, PNG",
'mw_math_html'   => "HTML si ye posible, si no'n ye, PNG",
'mw_math_source' => 'Deixar como TeX (ta nabegadores en formato testo)',
'mw_math_modern' => 'Recomendato ta nabegadors modernos',
'mw_math_mathml' => 'MathML si ye posible (esperimental)',

# Patrolling
'markaspatrolleddiff'                 => 'Siñalar como ya controlato',
'markaspatrolledtext'                 => 'Siñalar iste articlo como controlato',
'markedaspatrolled'                   => 'Siñalato como controlato',
'markedaspatrolledtext'               => 'A bersión trigata ye siñalata como controlata.',
'rcpatroldisabled'                    => "S'ha desautibato o control d'os zagurers cambeos",
'rcpatroldisabledtext'                => "A funzión de control d'os zaguers cambeos ye desautibata en iste inte.",
'markedaspatrollederror'              => 'No se puede siñalar como controlata',
'markedaspatrollederrortext'          => "Ha d'espezificar una bersión ta siñalar-la como controlata.",
'markedaspatrollederror-noautopatrol' => 'No tiene premisos ta siñalar os suyos propios cambios como controlatos.',

# Patrol log
'patrol-log-page'   => 'Rechistro de control de bersions',
'patrol-log-header' => 'Iste ye un rechistro de rebisions patrullatas.',
'patrol-log-line'   => "s'ha siñalato a bersión $1 de $2 como controlata $3",
'patrol-log-auto'   => '(automatico)',

# Image deletion
'deletedrevision'                 => "S'ha borrato a bersión antiga $1",
'filedeleteerror-short'           => "Error borrando l'archibo: $1",
'filedeleteerror-long'            => "Se troboron errors borrando l'archibo:

$1",
'filedelete-missing'              => 'L\'archibo "$1" no se puede borrar porque no esiste.',
'filedelete-old-unregistered'     => 'A bersión de l\'archibo espezificata "$1" no ye en a base de datos.',
'filedelete-current-unregistered' => 'L\'archibo espezificato "$1" no ye en a base de datos.',
'filedelete-archive-read-only'    => 'O direutorio d\'archibo "$1" no puede escribir-se en o serbidor web.',

# Browsing diffs
'previousdiff' => '← Ir ta esferenzias anteriors',
'nextdiff'     => "Ir t'as siguients esferenzias →",

# Media information
'mediawarning'         => "'''Pare cuenta''': Iste archibo puede contener codigo endino; si l'executa, podría meter en un contornillo a seguridat d'o suyo sistema.<hr />",
'imagemaxsize'         => "Limitar as imachens en as pachinas de descripzión d'archibos á:",
'thumbsize'            => "Midas d'a miniatura:",
'widthheightpage'      => '$1×$2, $3 {{PLURAL:$3|pachina|pachinas}}',
'file-info'            => "(grandaria de l'archibo: $1; tipo MIME: $2)",
'file-info-size'       => "($1 × $2 píxels; grandaria de l'archibo: $3; tipo MIME: $4)",
'file-nohires'         => '<small>No bi ha garra bersión con mayor resoluzión.</small>',
'svg-long-desc'        => '(archibo SVG, nominalment $1 × $2 píxels, grandaria: $3)',
'show-big-image'       => 'Imachen en a maisima resoluzión',
'show-big-image-thumb' => "<small>Grandaria d'ista ambiesta prebia: $1 × $2 píxels</small>",

# Special:NewImages
'newimages'             => 'Galería de nuebas imachens',
'imagelisttext'         => "Contino bi ha una lista de '''$1''' {{PLURAL:$1|imachen ordenata|imachens ordenatas}} $2.",
'newimages-summary'     => 'Ista pachina espezial amuestra os zaguers archibos cargatos.',
'showhidebots'          => '($1 bots)',
'noimages'              => 'No bi ha cosa á beyer.',
'ilsubmit'              => 'Mirar',
'bydate'                => 'por a calendata',
'sp-newimages-showfrom' => "Amostrar archibos nuebos dende as $2 d'o $1",

# Bad image list
'bad_image_list' => "O formato ye asinas:

Se consideran nomás os elementos d'una lista (linias que escomienzan por *). O primer binclo de cada linia ha d'estar un binclo ta un archibo malo. Cualsiquier atros binclos en a mesma linia se consideran eszepzions, i.e. pachinas an que l'archibo puede amanexer encrustato.",

# Metadata
'metadata'          => 'Metadatos',
'metadata-help'     => "Iste archibo contiene informazión adizional, probablement adibida por a camara dichital, o escáner u o programa emplegato ta creyar-lo u dichitalizar-lo.  Si l'archibo ha estato modificato dende o suyo estau orichinal, bels detalles podrían no reflexar completament l'archibo modificato.",
'metadata-expand'   => 'Amostrar informazión detallata',
'metadata-collapse' => 'Amagar a informazión detallata',
'metadata-fields'   => "Os campos de metadatos EXIF que amanixen en iste mensache s'amuestrarán en a pachina de descripzión d'a imachen, mesmo si a tabla ye plegata. Bi ha atros campos que remanirán amagatos por defeuto.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength", # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => 'Amplaria',
'exif-imagelength'                 => 'Altaria',
'exif-bitspersample'               => 'Bits por component',
'exif-compression'                 => 'Esquema de compresión',
'exif-photometricinterpretation'   => "Composizión d'os pixels",
'exif-orientation'                 => 'Orientazión',
'exif-samplesperpixel'             => 'Numero de components por píxel',
'exif-planarconfiguration'         => 'Ordinazión de datos',
'exif-ycbcrsubsampling'            => 'Razón de submuestreyo de Y á C',
'exif-ycbcrpositioning'            => 'Posizión de Y y C',
'exif-xresolution'                 => 'Resoluzión orizontal',
'exif-yresolution'                 => 'Resoluzión bertical',
'exif-resolutionunit'              => "Unidaz d'as resoluzions en X e Y",
'exif-stripoffsets'                => "Localizazión d'os datos d'a imachen",
'exif-rowsperstrip'                => 'Numero de ringleras por faixa',
'exif-stripbytecounts'             => 'Bytes por faixa comprimita',
'exif-jpeginterchangeformat'       => "Offset d'o JPEG SOI",
'exif-jpeginterchangeformatlength' => 'Bytes de datos JPEG',
'exif-transferfunction'            => 'Funzión de transferenzia',
'exif-whitepoint'                  => "Coordinatas cromaticas d'o punto blanco",
'exif-primarychromaticities'       => "Coordinatas cromaticas d'as colors primarias",
'exif-ycbcrcoefficients'           => "Coefizients d'a matriz de transformazión d'o espazio de colors",
'exif-referenceblackwhite'         => 'Parella de baluras blanco/negro de referenzia',
'exif-datetime'                    => "Calendata y ora d'o zaguer cambeo de l'archibo",
'exif-imagedescription'            => "Títol d'a imachen",
'exif-make'                        => "Fabriquero d'a maquina",
'exif-model'                       => 'Modelo de maquina',
'exif-software'                    => 'Software emplegato',
'exif-artist'                      => 'Autor',
'exif-copyright'                   => "Dueño d'os dreitos d'autor (copyright)",
'exif-exifversion'                 => 'Bersión Exif',
'exif-flashpixversion'             => 'Bersión de Flashpix almitita',
'exif-colorspace'                  => 'Espazio de colors',
'exif-componentsconfiguration'     => 'Sinnificazión de cada component',
'exif-compressedbitsperpixel'      => "Modo de compresión d'a imachen",
'exif-pixelydimension'             => "Amplaria conforme d'a imachen",
'exif-pixelxdimension'             => "Altaria conforme d'a imachen",
'exif-makernote'                   => "Notas d'o fabriquero",
'exif-usercomment'                 => "Comentarios de l'usuario",
'exif-relatedsoundfile'            => "Archibo d'audio relazionato",
'exif-datetimeoriginal'            => "Calendata y ora de chenerazión d'os datos",
'exif-datetimedigitized'           => "Calendata y ora d'a dichitalizazión",
'exif-subsectime'                  => 'Calendata y ora (frazions de segundo)',
'exif-subsectimeoriginal'          => "Calendata y ora d'a chenerazión d'os datos (frazions de segundo)",
'exif-subsectimedigitized'         => "Calendata y ora d'a dichitalizazión (frazions de segundo)",
'exif-exposuretime'                => "Tiempo d'esposizión",
'exif-exposuretime-format'         => '$1 seg ($2)',
'exif-fnumber'                     => 'Numero F',
'exif-exposureprogram'             => "Programa d'esposizión",
'exif-spectralsensitivity'         => 'Sensibilidat espeutral',
'exif-isospeedratings'             => 'Sensibilidat ISO',
'exif-oecf'                        => 'Fautor de combersión optoelectronica',
'exif-shutterspeedvalue'           => "Belozidat de l'obturador",
'exif-aperturevalue'               => 'Obredura',
'exif-brightnessvalue'             => 'Brilura',
'exif-exposurebiasvalue'           => "Siesco d'esposizión",
'exif-maxaperturevalue'            => 'Obredura maisima',
'exif-subjectdistance'             => 'Distanzia á o sucheto',
'exif-meteringmode'                => 'Modo de mesura',
'exif-lightsource'                 => 'Fuent de luz',
'exif-flash'                       => 'Flash',
'exif-focallength'                 => "Longaria d'o lente focal",
'exif-subjectarea'                 => "Aria d'o sucheto",
'exif-flashenergy'                 => "Enerchía d'o flash",
'exif-spatialfrequencyresponse'    => 'Respuesta frecuenzial espazial',
'exif-focalplanexresolution'       => 'Resoluzión en o plano focal X',
'exif-focalplaneyresolution'       => 'Resolución en o plano focal Y',
'exif-focalplaneresolutionunit'    => "Unidaz d'a resoluzión en o plano focal",
'exif-subjectlocation'             => "Posizión d'o sucheto",
'exif-exposureindex'               => "Endize d'esposizión",
'exif-sensingmethod'               => 'Metodo de sensache',
'exif-filesource'                  => "Fuent de l'archibo",
'exif-scenetype'                   => "Mena d'eszena",
'exif-cfapattern'                  => 'Patrón CFA',
'exif-customrendered'              => "Prozesau d'imachen presonalizato",
'exif-exposuremode'                => "Modo d'esposizión",
'exif-whitebalance'                => 'Balanze de blancos',
'exif-digitalzoomratio'            => 'Ratio de zoom dichital',
'exif-focallengthin35mmfilm'       => 'Longaria focal equibalent á zinta de 35 mm',
'exif-scenecapturetype'            => "Mena de captura d'a eszena",
'exif-gaincontrol'                 => "Control d'eszena",
'exif-contrast'                    => 'Contraste',
'exif-saturation'                  => 'Saturazión',
'exif-sharpness'                   => 'Nitideza',
'exif-devicesettingdescription'    => "Descripzión d'os achustes d'o dispositibo",
'exif-subjectdistancerange'        => 'Rango de distancias á o sucheto',
'exif-imageuniqueid'               => "ID unico d'a imachen",
'exif-gpsversionid'                => "Bersión d'as etiquetas de GPS",
'exif-gpslatituderef'              => 'Latitut norte/sud',
'exif-gpslatitude'                 => 'Latitut',
'exif-gpslongituderef'             => 'Lonchitut este/ueste',
'exif-gpslongitude'                => 'Lonchitut',
'exif-gpsaltituderef'              => "Referenzia d'a altitut",
'exif-gpsaltitude'                 => 'Altitut',
'exif-gpstimestamp'                => 'Tiempo GPS (reloch atomico)',
'exif-gpssatellites'               => 'Satelites emplegatos en a mida',
'exif-gpsstatus'                   => "Estau d'o rezeptor",
'exif-gpsmeasuremode'              => 'Modo de mesura',
'exif-gpsdop'                      => "Prezisión d'a mida",
'exif-gpsspeedref'                 => 'Unidaz de belozidat',
'exif-gpsspeed'                    => "Belozidat d'o rezeptor GPS",
'exif-gpstrackref'                 => "Referenzia d'a endrezera d'o mobimiento",
'exif-gpstrack'                    => "Endrezera d'o mobimiento",
'exif-gpsimgdirectionref'          => "Referenzia d'a orientazión d'a imachen",
'exif-gpsimgdirection'             => "Orientazión d'a imachen",
'exif-gpsmapdatum'                 => 'Emplegatos datos de mesura cheodesica',
'exif-gpsdestlatituderef'          => "Referenzia t'a latitut d'o destino",
'exif-gpsdestlatitude'             => "Latitut d'o destino",
'exif-gpsdestlongituderef'         => "Referenzia d'a lonchitut d'o destino",
'exif-gpsdestlongitude'            => "Lonchitut d'o destino",
'exif-gpsdestbearingref'           => "Referenzia d'a orientazión á o destino",
'exif-gpsdestbearing'              => "Orientazión d'o destino",
'exif-gpsdestdistanceref'          => "Referenzia d'a distanzia á o destino",
'exif-gpsdestdistance'             => 'Distanzia á o destino',
'exif-gpsprocessingmethod'         => "Nombre d'o metodo de prozesamiento GPS",
'exif-gpsareainformation'          => "Nombre d'aria GPS",
'exif-gpsdatestamp'                => 'Calendata GPS',
'exif-gpsdifferential'             => 'Correzión diferenzial de GPS',

# EXIF attributes
'exif-compression-1' => 'Sin de compresión',

'exif-unknowndate' => 'Calendata esconoixita',

'exif-orientation-1' => 'Normal', # 0th row: top; 0th column: left
'exif-orientation-2' => 'Contornata orizontalment', # 0th row: top; 0th column: right
'exif-orientation-3' => 'Chirata 180º', # 0th row: bottom; 0th column: right
'exif-orientation-4' => 'Contornata berticalment', # 0th row: bottom; 0th column: left
'exif-orientation-5' => "Chirata 90° en contra d'as agullas d'o reloch y contornata berticalment", # 0th row: left; 0th column: top
'exif-orientation-6' => "Chirata 90° como as agullas d'o reloch", # 0th row: right; 0th column: top
'exif-orientation-7' => "Chirata 90° como as agullas d'o reloch y contornata berticalment", # 0th row: right; 0th column: bottom
'exif-orientation-8' => "Chirata 90° en contra d'as agullas d'o reloch", # 0th row: left; 0th column: bottom

'exif-planarconfiguration-1' => 'formato de paquez de píxels',
'exif-planarconfiguration-2' => 'formato plano',

'exif-componentsconfiguration-0' => 'no esiste',

'exif-exposureprogram-0' => 'No definito',
'exif-exposureprogram-1' => 'Manual',
'exif-exposureprogram-2' => 'Modo normal',
'exif-exposureprogram-3' => "Prioridat á l'obredura",
'exif-exposureprogram-4' => "Prioridat á l'obturador",
'exif-exposureprogram-5' => 'Modo creatibo (con prioridat á la fondura de campo)',
'exif-exposureprogram-6' => "Modo aizión (alta belozidat de l'obturador)",
'exif-exposureprogram-7' => 'Modo retrato (ta primers planos con o fundo desenfocato)',
'exif-exposureprogram-8' => 'Modo paisache (ta fotos de paisaches con o fundo enfocato)',

'exif-subjectdistance-value' => '$1 metros',

'exif-meteringmode-0'   => 'Esconoixito',
'exif-meteringmode-1'   => 'Meya',
'exif-meteringmode-2'   => 'Meya aponderata á o zentro',
'exif-meteringmode-3'   => 'Puntual',
'exif-meteringmode-4'   => 'Multipunto',
'exif-meteringmode-5'   => 'Patrón',
'exif-meteringmode-6'   => 'Parzial',
'exif-meteringmode-255' => 'Atros',

'exif-lightsource-0'   => 'Esconoixito',
'exif-lightsource-1'   => 'Luz de día',
'exif-lightsource-2'   => 'Fluoreszent',
'exif-lightsource-3'   => 'Tungsteno (luz incandeszent)',
'exif-lightsource-4'   => 'Flash',
'exif-lightsource-9'   => 'Buen orache',
'exif-lightsource-10'  => 'Orache nublo',
'exif-lightsource-11'  => 'Guambra',
'exif-lightsource-12'  => 'Fluorescente de luz de día (D 5700 – 7100K)',
'exif-lightsource-13'  => 'Fluoreszent blanco de día (N 4600 – 5400K)',
'exif-lightsource-14'  => 'Fluoreszent blanco fredo (W 3900 – 4500K)',
'exif-lightsource-15'  => 'Fluoreszent blanco (WW 3200 – 3700K)',
'exif-lightsource-17'  => 'Luz estándar A',
'exif-lightsource-18'  => 'Luz estándar B',
'exif-lightsource-19'  => 'Luz estándar C',
'exif-lightsource-24'  => "Bombeta de tungsteno d'estudeo ISO",
'exif-lightsource-255' => 'Atra fuent de luz',

'exif-focalplaneresolutionunit-2' => 'pulgadas',

'exif-sensingmethod-1' => 'No definito',
'exif-sensingmethod-2' => "Sensor d'aria de color d'un chip",
'exif-sensingmethod-3' => "Sensor d'aria de color de dos chips",
'exif-sensingmethod-4' => "Sensor d'aria de color de tres chips",
'exif-sensingmethod-5' => "Sensor d'aria de color secuenzial",
'exif-sensingmethod-7' => 'Sensor trilinial',
'exif-sensingmethod-8' => 'Sensor linial de color secuenzial',

'exif-scenetype-1' => 'Una imachen fotiata dreitament',

'exif-customrendered-0' => 'Prozeso normal',
'exif-customrendered-1' => 'Prozeso presonalizato',

'exif-exposuremode-0' => 'Esposizión automatica',
'exif-exposuremode-1' => 'Esposizión manual',
'exif-exposuremode-2' => 'Bracketting automatico',

'exif-whitebalance-0' => 'Balanze automatico de blancos',
'exif-whitebalance-1' => 'Balanze manual de blancos',

'exif-scenecapturetype-0' => 'Estándar',
'exif-scenecapturetype-1' => 'Ambiesta (orizontal)',
'exif-scenecapturetype-2' => 'Retrato (bertical)',
'exif-scenecapturetype-3' => 'Eszena de nueits',

'exif-gaincontrol-0' => 'Denguna',
'exif-gaincontrol-1' => 'Gananzia baixa ta baluras altas (low gain up)',
'exif-gaincontrol-2' => 'Gananzia alta ta baluras altas (high gain up)',
'exif-gaincontrol-3' => 'Gananzia baixa ta baluras baixas (low gain down)',
'exif-gaincontrol-4' => 'Gananzia alta ta baluras baixas (high gain down)',

'exif-contrast-0' => 'Normal',
'exif-contrast-1' => 'Suabe',
'exif-contrast-2' => 'Fuerte',

'exif-saturation-0' => 'Normal',
'exif-saturation-1' => 'Baixa saturazión',
'exif-saturation-2' => 'Alta saturazión',

'exif-sharpness-0' => 'Normal',
'exif-sharpness-1' => 'Suabe',
'exif-sharpness-2' => 'Fuerte',

'exif-subjectdistancerange-0' => 'Esconoixita',
'exif-subjectdistancerange-1' => 'Macro',
'exif-subjectdistancerange-2' => 'Ambista zercana',
'exif-subjectdistancerange-3' => 'Ambista leixana',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'Latitut norte',
'exif-gpslatitude-s' => 'Latitut sud',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'Lonchitut este',
'exif-gpslongitude-w' => 'Lonchitut ueste',

'exif-gpsstatus-a' => "S'está fendo a mida",
'exif-gpsstatus-v' => 'Interoperabilitat de mesura',

'exif-gpsmeasuremode-2' => 'Mesura bidimensional',
'exif-gpsmeasuremode-3' => 'Mesura tridimensional',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'Quilometros por ora',
'exif-gpsspeed-m' => 'Millas por ora',
'exif-gpsspeed-n' => 'Nugos',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => 'Endrezera reyal',
'exif-gpsdirection-m' => 'Endrezera magnetica',

# External editor support
'edit-externally'      => 'Editar iste archibo fendo serbir una aplicazión esterna',
'edit-externally-help' => 'Leiga as [http://www.mediawiki.org/wiki/Manual:External_editors instruzions de confegurazión] (en anglés) ta más informazión.',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'toz',
'imagelistall'     => 'todas',
'watchlistall2'    => 'toz',
'namespacesall'    => 'todo',
'monthsall'        => '(toz)',

# E-mail address confirmation
'confirmemail'             => 'Confirmar adreza de correu-e',
'confirmemail_noemail'     => "No tiene una adreza de correu-e conforme en as suyas [[Special:Preferences|preferenzias d'usuario]].",
'confirmemail_text'        => "{{SITENAME}} requiere que confirme a suya adreza de correu-e antis de poder usar as funzions de correu-e. Punche o botón de baxo ta nimbiar un mensache de confirmazión t'a suya adreza. O mensache encluirá un binclo con un codigo. Escriba-lo ta confirmar que a suya adreza ye conforme.",
'confirmemail_pending'     => '<div class="error">
Ya se le ha nimbiato un codigo de confirmazión; si creyó una cuenta fa poco tiempo, puede que s\'estime más asperar bels menutos á beyer si le plega antis de pedir un nuebo codigo.
</div>',
'confirmemail_send'        => 'Nimbiar un codigo de confirmazión.',
'confirmemail_sent'        => "S'ha nimbiato un correu de confirmazión.",
'confirmemail_oncreate'    => "S'ha nimbiato un codigo de confirmazión t'a suya adreza de correu-e.
Iste codigo no ye nezesario ta dentrar, pero amenistará escribir-lo antis d'autibar cualsiquier funzión d'o wiki basata en o correu electronico.",
'confirmemail_sendfailed'  => "No s'ha puesto nimbiar o mensache de confirmazión. Por fabor, comprebe que no bi aiga carauters no conformes en l'adreza de correu electronico endicata.

Correu tornato: $1",
'confirmemail_invalid'     => 'O codigo de confirmazión no ye conforme. Regular que o codigo sía zircunduzito.',
'confirmemail_needlogin'   => 'Amenistar $1 ta confirmar a suya adreza de correu-e.',
'confirmemail_success'     => 'A suya adreza de correu-e ya ye confirmata. Agora puede dentrar en o wiki y espleitiar-lo.',
'confirmemail_loggedin'    => 'A suya adreza de correu-e ya ye confirmata.',
'confirmemail_error'       => 'Bella cosa falló en alzar a suya confirmazión.',
'confirmemail_subject'     => "confirmazión de l'adreza de correu-e de {{SITENAME}}",
'confirmemail_body'        => 'Belún, probablement busté mesmo, ha rechistrato una cuenta "$2" con ista adreza de correu-e en {{SITENAME}} dende l\'adreza IP $1.

Ta confirmar que ista cuenta reyalment le perteneixe y autibar as funzions de correu-e en {{SITENAME}}, ubra iste binclo en o suyo nabegador:

$3

Si a cuenta *no* ye suya, siga iste atro binclo ta anular a confirmazión d\'adreza de correu-e:

$5

Iste codigo de confirmazión zircunduzirá en $4.',
'confirmemail_invalidated' => "Anular a confirmazión d'adreza de correu-e",
'invalidateemail'          => 'Anular a confirmazión de correu-e',

# Scary transclusion
'scarytranscludedisabled' => "[S'ha desautibato a transclusión interwiki]",
'scarytranscludefailed'   => "[Ha fallato a recuperazión d'a plantilla ta $1; lo sentimos]",
'scarytranscludetoolong'  => '[A URL ye masiau larga; lo sentimos]',

# Trackbacks
'trackbackbox'      => '<div id="mw_trackbacks">
Retrobinclos (trackbacks) ta iste articlo:<br />
$1
</div>',
'trackbackremove'   => ' ([$1 Borrar])',
'trackbacklink'     => 'Retrobinclo (Trackback)',
'trackbackdeleteok' => "O retrobinclo (trackback) s'ha borrato correutament.",

# Delete conflict
'deletedwhileediting' => 'Pare cuenta: Ista pachina ye estata borrata dimpués de que enzetase a edizión!',
'confirmrecreate'     => "O ususario [[User:$1|$1]] ([[User talk:$1|descusión]]) ha borrato iste articlo dimpués que bustet prenzipió á editarlo, y a razón que ha dato ye:
: ''$2''
Por fabor, confirme que reyalment deseya creyar l'articlo nuebament.",
'recreate'            => 'Creyar nuebament',

# HTML dump
'redirectingto' => 'Reendrezando ta [[:$1]]...',

# action=purge
'confirm_purge'        => "Limpiar a caché d'ista pachina?

$1",
'confirm_purge_button' => 'Confirmar',

# AJAX search
'searchcontaining' => "Mirar articlos que contiengan ''$1''.",
'searchnamed'      => "Mirar articlos con o títol ''$1''.",
'articletitles'    => "Articlos que prenzipian por ''$1''",
'hideresults'      => 'Amagar resultaus',
'useajaxsearch'    => 'Faiga serbir a busca en AJAX',

# Multipage image navigation
'imgmultipageprev' => '← pachina anterior',
'imgmultipagenext' => 'pachina siguient →',
'imgmultigo'       => 'Ir-ie!',
'imgmultigoto'     => "Ir t'a pachina $1",

# Table pager
'ascending_abbrev'         => 'asz',
'descending_abbrev'        => 'desz',
'table_pager_next'         => 'Pachina siguient',
'table_pager_prev'         => 'Pachina anterior',
'table_pager_first'        => 'Primera pachina',
'table_pager_last'         => 'Zaguer pachina',
'table_pager_limit'        => 'Amostrar $1 elementos por pachina',
'table_pager_limit_submit' => 'Ir-ie',
'table_pager_empty'        => 'No bi ha garra resultau',

# Auto-summaries
'autosumm-blank'   => 'Pachina blanquiata',
'autosumm-replace' => 'O conteniu s\'ha cambiato por "$1"',
'autoredircomment' => 'Reendrezando ta [[$1]]',
'autosumm-new'     => 'Pachina nueba: $1',

# Live preview
'livepreview-loading' => 'Cargando…',
'livepreview-ready'   => 'Cargando… ya!',
'livepreview-failed'  => "A prebisualizazión á l'inte falló!
Prebe con a prebisualizazión normal.",
'livepreview-error'   => 'No s\'ha puesto coneutar: $1 "$2". Prebe con l\'ambiesta prebia normal.',

# Friendlier slave lag warnings
'lag-warn-normal' => "Talment no s'amuestren en ista lista as edizions feitas en {{PLURAL:$1|o zaguer segundo|os zaguers $1 segundos}}.",
'lag-warn-high'   => "Por o retardo d'o serbidor d'a base de datos, talment no s'amuestren en ista lista as edizions feitas en {{PLURAL:$1|o zaguer segundo|os zaguers $1 segundos}}.",

# Watchlist editor
'watchlistedit-numitems'       => 'A suya lista de seguimiento tiene {{PLURAL:$1|una pachina |$1 pachinas}}, sin contar-ie as pachinas de descusión.',
'watchlistedit-noitems'        => 'A suya lista de seguimiento ye bueda.',
'watchlistedit-normal-title'   => 'Editar a lista de seguimiento',
'watchlistedit-normal-legend'  => "Borrar títols d'a lista de seguimiento",
'watchlistedit-normal-explain' => "As pachinas d'a suya lista de seguimiento s'amuestran contino. Ta sacar-ne una pachina, marque o cuatrón que ye a o canto d'a pachina, y punche con a rateta en ''Borrar pachinas''. Tamién puede [[Special:Watchlist/raw|editar dreitament o testo d'a pachina]].",
'watchlistedit-normal-submit'  => 'Borrar pachinas',
'watchlistedit-normal-done'    => "{{PLURAL:$1|S'ha borrato 1 pachina|s'han borrato $1 pachinas}} d'a suya lista de seguimiento:",
'watchlistedit-raw-title'      => 'Editar a lista de seguimiento en formato testo',
'watchlistedit-raw-legend'     => 'Editar a lista de seguimiento en formato testo',
'watchlistedit-raw-explain'    => "Contino s'amuestran as pachinas d'a suya lista de seguimiento.
Puede editar ista lista adibiendo u borrando líneas d'a lista; una pachina por linia.
Cuan remate, punche ''esbiellar lista de seguimiento''.
Tamién puede fer serbir o [[Special:Watchlist/edit|editor estándar]].",
'watchlistedit-raw-titles'     => 'Pachinas:',
'watchlistedit-raw-submit'     => 'Esbiellar lista de seguimiento',
'watchlistedit-raw-done'       => "S'ha esbiellato a suya lista de seguimiento.",
'watchlistedit-raw-added'      => "{{PLURAL:$1|S'ha esbiellato una pachina|S'ha esbiellato $1 pachinas}}:",
'watchlistedit-raw-removed'    => "{{PLURAL:$1|S'ha borrato una pachina|S'ha borrato $1 pachinas}}:",

# Watchlist editing tools
'watchlisttools-view' => 'Amostrar cambeos',
'watchlisttools-edit' => 'Beyer y editar a lista de seguimiento',
'watchlisttools-raw'  => 'Editar a lista de seguimiento en formato testo',

# Core parser functions
'unknown_extension_tag' => 'Etiqueta d\'estensión "$1" esconoixita',

# Special:Version
'version'                          => 'Bersión', # Not used as normal message but as header for the special page itself
'version-extensions'               => 'Estensions instalatas',
'version-specialpages'             => 'Pachinas espezials',
'version-parserhooks'              => "Grifios d'o parser (parser hooks)",
'version-variables'                => 'Bariables',
'version-other'                    => 'Atros',
'version-mediahandlers'            => "Maneyador d'archibos multimedia",
'version-hooks'                    => 'Grifios (Hooks)',
'version-extension-functions'      => "Funzions d'a estensión",
'version-parser-extensiontags'     => "Etiquetas d'estensión d'o parseyador",
'version-parser-function-hooks'    => "Grifios d'as funzions d'o parseyador",
'version-skin-extension-functions' => "Funzions d'estensión de l'aparenzia (Skin)",
'version-hook-name'                => "Nombre d'o grifio",
'version-hook-subscribedby'        => 'Suscrito por',
'version-version'                  => 'Bersión',
'version-license'                  => 'Lizenzia',
'version-software'                 => 'Software instalato',
'version-software-product'         => 'Produto',
'version-software-version'         => 'Bersión',

# Special:FilePath
'filepath'         => "Camín de l'archibo",
'filepath-page'    => 'Archibo:',
'filepath-submit'  => 'Camín',
'filepath-summary' => "Ista pachina espezial le retorna o camín completo d'un archibo.
As imachens s'amuestran en resoluzión completa, a resta d'archibos fan enzetar dreitament os suyos programas asoziatos.

Escriba o nombre de l'archibo sin o prefixo \"{{ns:image}}:\".",

# Special:FileDuplicateSearch
'fileduplicatesearch'          => 'Mirar archibos duplicatos',
'fileduplicatesearch-summary'  => 'Mirar achibos duplicatos basatos en a suya balura hash.

Escriba o nombre de l\'archibo sin o prefixo "{{ns:image}}:".',
'fileduplicatesearch-legend'   => 'Mirar duplicatos',
'fileduplicatesearch-filename' => "Nombre de l'archibo:",
'fileduplicatesearch-submit'   => 'Mirar',
'fileduplicatesearch-info'     => "$1 × $2 pixels<br />Grandaria de l'archibo: $3<br />tipo MIME: $4",
'fileduplicatesearch-result-1' => 'L\'archibo "$1" no en tiene de duplicaus identicos.',
'fileduplicatesearch-result-n' => 'L\'archibo "$1" tiene {{PLURAL:$2|1 duplicau identico|$2 duplicaus identicos}}.',

# Special:SpecialPages
'specialpages'                   => 'Pachinas espezials',
'specialpages-note'              => '----
* Pachinas espezials normals.
* <span class="mw-specialpagerestricted">Pachinas espezials restrinchitas.</span>',
'specialpages-group-maintenance' => 'Informes de mantenimiento',
'specialpages-group-other'       => 'Atras pachinas espezials',
'specialpages-group-login'       => 'Inizio de sesión / rechistro',
'specialpages-group-changes'     => 'Zaguers cambios y rechistros',
'specialpages-group-media'       => "Informes d'archibos multimedias y cargas",
'specialpages-group-users'       => 'Usuarios y dreitos',
'specialpages-group-highuse'     => 'Pachinas con muito uso',
'specialpages-group-pages'       => 'Listas de pachinas',
'specialpages-group-pagetools'   => "Ainas t'as pachinas",
'specialpages-group-wiki'        => 'Datos sobre a wiki y ainas',
'specialpages-group-redirects'   => 'Reendrezando as pachinas espezials',
'specialpages-group-spam'        => 'Ainas de spam',

# Special:BlankPage
'blankpage'              => 'Pachina en blanco',
'intentionallyblankpage' => "Esta pachina s'ha deixato en blanco aldredes y se fa serbir ta fer prebatinas, ezt.",

);
