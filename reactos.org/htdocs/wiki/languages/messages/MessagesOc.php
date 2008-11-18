<?php
/** Occitan (Occitan)
 *
 * @ingroup Language
 * @file
 *
 * @author Cedric31
 * @author ChrisPtDe
 * @author Spacebirdy
 * @author Горан Анђелковић
 * @author לערי ריינהארט
 */

$skinNames = array(
	'standard'    => 'Estandard',
	'nostalgia'   => 'Nostalgia',
	'cologneblue' => 'Colonha Blau',
	'monobook'    => 'Monobook',
	'myskin'      => 'Mon interfàcia',
	'chick'       => 'Poleton',
	'simple'      => 'Simple',
	'modern'      => 'Modèrn',
);

$bookstoreList = array(
	'Amazon.fr' => 'http://www.amazon.fr/exec/obidos/ISBN=$1'
);

$namespaceNames = array(
	NS_MEDIA          => 'Mèdia',
	NS_SPECIAL        => 'Especial',
	NS_MAIN           => '',
	NS_TALK           => 'Discutir',
	NS_USER           => 'Utilizaire',
	NS_USER_TALK      => 'Discussion_Utilizaire',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK   => 'Discussion_$1',
	NS_IMAGE          => 'Imatge',
	NS_IMAGE_TALK     => 'Discussion_Imatge',
	NS_MEDIAWIKI      => 'MediaWiki',
	NS_MEDIAWIKI_TALK => 'Discussion_MediaWiki',
	NS_TEMPLATE       => 'Modèl',
	NS_TEMPLATE_TALK  => 'Discussion_Modèl',
	NS_HELP           => 'Ajuda',
	NS_HELP_TALK      => 'Discussion_Ajuda',
	NS_CATEGORY       => 'Categoria',
	NS_CATEGORY_TALK  => 'Discussion_Categoria',
);

$namespaceAliases = array(
	'Utilisator'            => NS_USER,
	'Discussion_Utilisator' => NS_USER_TALK,
	'Discutida_Utilisator' => NS_USER_TALK,
	'Discutida_Imatge'     => NS_IMAGE_TALK,
	'Mediaòiqui'           => NS_MEDIAWIKI,
	'Discussion_Mediaòiqui' => NS_MEDIAWIKI_TALK,
	'Discutida_Mediaòiqui' => NS_MEDIAWIKI_TALK,
	'Discutida_Modèl'      => NS_TEMPLATE_TALK,
	'Discutida_Ajuda'      => NS_HELP_TALK,
	'Discutida_Categoria'  => NS_CATEGORY_TALK,
);

$specialPageAliases = array(
	'DoubleRedirects'           => array( 'Redireccions_doblas' ),
	'BrokenRedirects'           => array( 'Redireccions_copadas' ),
	'Disambiguations'           => array( 'Omonimia' ),
	'Userlogin'                 => array( 'Nom_d\'utilizaire' ),
	'Userlogout'                => array( 'Desconnexion' ),
	'CreateAccount'             => array( 'Crear_un_compte', 'CrearUnCompte', 'CrearCompte' ),
	'Preferences'               => array( 'Preferéncias' ),
	'Watchlist'                 => array( 'Lista_de_seguit' ),
	'Recentchanges'             => array( 'Darrièrs_cambiaments' ),
	'Upload'                    => array( 'Telecargament' ),
	'Imagelist'                 => array( 'Lista_dels_imatges' ),
	'Newimages'                 => array( 'Imatges_novèls' ),
	'Listusers'                 => array( 'Lista_dels_utilizaires' ),
	'Listgrouprights'           => array( 'Lista_dels_gropes_utilizaire', 'ListadelsGropesUtilizaire', 'ListaGropesUtilizaire', 'Tièra_dels_gropes_utilizaire', 'TièradelsGropesUtilizaire', 'TièraGropesUtilizaire' ),
	'Statistics'                => array( 'Estatisticas' ),
	'Randompage'                => array( 'Pagina_a_l\'azard' ),
	'Lonelypages'               => array( 'Paginas_orfanèlas' ),
	'Uncategorizedpages'        => array( 'Paginas_sens_categoria' ),
	'Uncategorizedcategories'   => array( 'Categorias_sens_categoria' ),
	'Uncategorizedimages'       => array( 'Imatges_sens_categoria' ),
	'Uncategorizedtemplates'    => array( 'Modèls_sens_categoria' ),
	'Unusedcategories'          => array( 'Categorias_inutilizadas' ),
	'Unusedimages'              => array( 'Imatges_inutilizats' ),
	'Wantedpages'               => array( 'Paginas_demandadas' ),
	'Wantedcategories'          => array( 'Categorias_demandadas' ),
	'Mostlinked'                => array( 'Imatges_mai_utilizats' ),
	'Mostlinkedcategories'      => array( 'Categorias__mai_utilizadas' ),
	'Mostlinkedtemplates'       => array( 'Modèls__mai_utilizats' ),
	'Mostcategories'            => array( 'Mai_de_categorias' ),
	'Mostimages'                => array( 'Mai_d\'imatges' ),
	'Mostrevisions'             => array( 'Mai_de_revisions' ),
	'Fewestrevisions'           => array( 'Mens_de_revisions' ),
	'Shortpages'                => array( 'Articles_brèus' ),
	'Longpages'                 => array( 'Articles_longs' ),
	'Newpages'                  => array( 'Paginas_novèlas' ),
	'Ancientpages'              => array( 'Paginas_ancianas' ),
	'Deadendpages'              => array( 'Paginas_sul_camin_d\'enlòc' ),
	'Protectedpages'            => array( 'Paginas_protegidas' ),
	'Protectedtitles'           => array( 'Títols_protegits', 'Títols_protegits', 'Títolsprotegits', 'Títolsprotegits' ),
	'Allpages'                  => array( 'Totas_las_paginas' ),
	'Prefixindex'               => array( 'Indèx' ),
	'Ipblocklist'               => array( 'Utilizaires_blocats' ),
	'Specialpages'              => array( 'Paginas_especialas' ),
	'Contributions'             => array( 'Contribucions' ),
	'Emailuser'                 => array( 'Corrièr_electronic', 'Email', 'Emèl', 'Emèil' ),
	'Confirmemail'              => array( 'Confirmar_lo_corrièr_electronic', 'Confirmarlocorrièrelectronic', 'ConfirmarCorrièrElectronic' ),
	'Whatlinkshere'             => array( 'Paginas_ligadas' ),
	'Recentchangeslinked'       => array( 'Seguit_dels_ligams' ),
	'Movepage'                  => array( 'Tornar_nomenar', 'Renomenatge' ),
	'Blockme'                   => array( 'Blocatz_me', 'Blocatzme' ),
	'Booksources'               => array( 'Obratge_de_referéncia', 'Obratges_de_referéncia' ),
	'Categories'                => array( 'Categorias' ),
	'Export'                    => array( 'Exportar', 'Exportacion' ),
	'Version'                   => array( 'Version' ),
	'Allmessages'               => array( 'Messatge_sistèma', 'Messatge_del_sistèma' ),
	'Log'                       => array( 'Jornal', 'Jornals' ),
	'Blockip'                   => array( 'Blocar', 'Blocatge' ),
	'Undelete'                  => array( 'Restablir', 'Restabliment' ),
	'Import'                    => array( 'Impòrt', 'Importacion' ),
	'Lockdb'                    => array( 'Varrolhar_la_banca' ),
	'Unlockdb'                  => array( 'Desvarrolhar_la_banca' ),
	'Userrights'                => array( 'Dreches', 'Permission' ),
	'MIMEsearch'                => array( 'Recèrca_MIME' ),
	'FileDuplicateSearch'       => array( 'Recèrca_fichièr_en_doble', 'RecèrcaFichièrEnDoble' ),
	'Unwatchedpages'            => array( 'Paginas_pas_seguidas' ),
	'Listredirects'             => array( 'Lista_de_las_redireccions', 'Listadelasredireccions', 'Lista_dels_redirects', 'Listadelsredirects', 'Lista_redireccions', 'Listaredireccions', 'Lista_redirects', 'Listaredirects' ),
	'Revisiondelete'            => array( 'Versions_suprimidas' ),
	'Unusedtemplates'           => array( 'Modèls_inutilizats', 'Modèlsinutilizats', 'Models_inutilizats', 'Modelsinutilizats', 'Modèls_pas_utilizats', 'Modèlspasutilizats', 'Models_pas_utilizats', 'Modelspasutilizats' ),
	'Randomredirect'            => array( 'Redireccion_a_l\'azard', 'Redirect_a_l\'azard' ),
	'Mypage'                    => array( 'Ma_pagina', 'Mapagina' ),
	'Mytalk'                    => array( 'Mas_discussions', 'Masdiscussions' ),
	'Mycontributions'           => array( 'Mas_contribucions', 'Mascontribucions' ),
	'Listadmins'                => array( 'Lista_dels_administrators', 'Listadelsadministrators', 'Lista_dels_admins', 'Listadelsadmins', 'Lista_admins', 'Listaadmins' ),
	'Listbots'                  => array( 'Lista_dels_Bòts', 'ListadelsBòts' ),
	'Popularpages'              => array( 'Paginas_mai_visitadas', 'Paginas_las_mai_visitadas', 'Paginasmaivisitadas' ),
	'Search'                    => array( 'Recèrca', 'Recercar', 'Cercar' ),
	'Resetpass'                 => array( 'Reïnicializacion_del_senhal', 'Reinicializaciondelsenhal' ),
	'Withoutinterwiki'          => array( 'Sens_interwiki', 'Sensinterwiki', 'Sens_interwikis', 'Sensinterwikis' ),
	'MergeHistory'              => array( 'Fusionar_l\'istoric', 'Fusionarlistoric' ),
	'Filepath'                  => array( 'Camin_del_Fichièr', 'CamindelFichièr', 'CaminFichièr' ),
	'Invalidateemail'           => array( 'Invalidar_Corrièr_electronic', 'InvalidarCorrièrElectronic' ),
);

$magicWords = array(
	'redirect'            => array( '0', '#REDIRECT', '#REDIRECCION' ),
	'notoc'               => array( '0', '__NOTOC__', '__CAPDETAULA__' ),
	'nogallery'           => array( '0', '__NOGALLERY__', '__CAPDEGALARIÁ__' ),
	'forcetoc'            => array( '0', '__FORCETOC__', '__FORÇARTAULA__' ),
	'toc'                 => array( '0', '__TOC__', '__TAULA__' ),
	'noeditsection'       => array( '0', '__NOEDITSECTION__', '__SECCIONNONEDITABLA__' ),
	'currentmonth'        => array( '1', 'CURRENTMONTH', 'MESCORRENT' ),
	'currentmonthname'    => array( '1', 'CURRENTMONTHNAME', 'NOMMESCORRENT' ),
	'currentday'          => array( '1', 'CURRENTDAY', 'JORNCORRENT' ),
	'currentday2'         => array( '1', 'CURRENTDAY2', 'JORNCORRENT2' ),
	'currentdayname'      => array( '1', 'CURRENTDAYNAME', 'NOMJORNCORRENT' ),
	'currentyear'         => array( '1', 'CURRENTYEAR', 'ANNADACORRENTA' ),
	'currenttime'         => array( '1', 'CURRENTTIME', 'DATACORRENTA' ),
	'currenthour'         => array( '1', 'CURRENTHOUR', 'ORACORRENTA' ),
	'numberofpages'       => array( '1', 'NUMBEROFPAGES', 'NOMBREPAGINAS' ),
	'numberofarticles'    => array( '1', 'NUMBEROFARTICLES', 'NOMBREARTICLES' ),
	'numberoffiles'       => array( '1', 'NUMBEROFFILES', 'NOMBREFICHIÈRS' ),
	'numberofusers'       => array( '1', 'NUMBEROFUSERS', 'NOMBREUTILIZAIRES' ),
	'numberofedits'       => array( '1', 'NUMBEROFEDITS', 'NOMBREEDICIONS' ),
	'pagename'            => array( '1', 'PAGENAME', 'NOMPAGINA' ),
	'namespace'           => array( '1', 'NAMESPACE', 'ESPACINOMENATGE' ),
	'talkspace'           => array( '1', 'TALKSPACE', 'ESPACIDISCUSSION' ),
	'img_right'           => array( '1', 'right', 'drecha', 'dreta' ),
	'img_left'            => array( '1', 'left', 'esquèrra', 'senèstra' ),
	'img_framed'          => array( '1', 'framed', 'enframed', 'frame', 'quadre' ),
	'img_frameless'       => array( '1', 'frameless', 'sens_quadre' ),
	'img_border'          => array( '1', 'border', 'bordadura' ),
	'server'              => array( '0', 'SERVER', 'SERVEIRE' ),
	'servername'          => array( '0', 'SERVERNAME', 'NOMSERVEIRE' ),
	'scriptpath'          => array( '0', 'SCRIPTPATH', 'CAMINESCRIPT' ),
	'grammar'             => array( '0', 'GRAMMAR:', 'GRAMATICA:' ),
	'currentweek'         => array( '1', 'CURRENTWEEK', 'SETMANACORRENTA' ),
	'revisionid'          => array( '1', 'REVISIONID', 'NUMÈROVERSION' ),
	'revisionday'         => array( '1', 'REVISIONDAY', 'DATAVERSION' ),
	'revisionday2'        => array( '1', 'REVISIONDAY2', 'DATAVERSION2' ),
	'revisionmonth'       => array( '1', 'REVISIONMONTH', 'MESREVISION' ),
	'revisionyear'        => array( '1', 'REVISIONYEAR', 'ANNADAREVISION' ),
	'revisiontimestamp'   => array( '1', 'REVISIONTIMESTAMP', 'ORAREVISION' ),
	'plural'              => array( '0', 'PLURAL:' ),
	'raw'                 => array( '0', 'RAW:', 'LINHA:' ),
	'displaytitle'        => array( '1', 'DISPLAYTITLE', 'AFICHARTÍTOL' ),
	'newsectionlink'      => array( '1', '__NEWSECTIONLINK__', '__LIGAMSECCIONNOVÈLA__' ),
	'currentversion'      => array( '1', 'CURRENTVERSION', 'VERSIONACTUALA' ),
	'currenttimestamp'    => array( '1', 'CURRENTTIMESTAMP', 'ORAACTUALA' ),
	'localtimestamp'      => array( '1', 'LOCALTIMESTAMP', 'ORALOCALA' ),
	'language'            => array( '0', '#LANGUAGE:', '#LENGA:' ),
	'numberofadmins'      => array( '1', 'NUMBEROFADMINS', 'NOMBREADMINS' ),
	'formatnum'           => array( '0', 'FORMATNUM', 'FORMATNOMBRE' ),
	'defaultsort'         => array( '1', 'DEFAULTSORT:', 'DEFAULTSORTKEY:', 'DEFAULTCATEGORYSORT:', 'ORDENA:' ),
	'filepath'            => array( '0', 'FILEPATH:', 'CAMIN:' ),
	'tag'                 => array( '0', 'tag', 'balisa' ),
	'hiddencat'           => array( '1', '__HIDDENCAT__', '__CATAMAGADA__' ),
	'pagesincategory'     => array( '1', 'PAGESINCATEGORY', 'PAGESINCAT', 'PAGINASDINSCAT' ),
);

$dateFormats = array(
	'mdy time' => 'H:i',
	'mdy date' => 'M j, Y',
	'mdy both' => 'M j, Y "a" H:i',

	'dmy time' => 'H:i',
	'dmy date' => 'j M Y',
	'dmy both' => 'j M Y "a" H:i',

	'ymd time' => 'H:i',
	'ymd date' => 'Y M j',
	'ymd both' => 'Y M j "a" H:i',
);

$separatorTransformTable = array( ',' => "\xc2\xa0", '.' => ',' );

$linkTrail = "/^([a-zàâçéèêîôû]+)(.*)\$/sDu";

$messages = array(
# User preference toggles
'tog-underline'               => 'Soslinhar los ligams :',
'tog-highlightbroken'         => 'Afichar <a href="" class="new">en roge</a> los ligams cap a las paginas inexistentas (siquenon :  coma aquò<a href="" class="internal">?</a>)',
'tog-justify'                 => 'Justificar los paragrafs',
'tog-hideminor'               => 'Amagar los darrièrs cambiaments menors',
'tog-extendwatchlist'         => 'Utilizar la lista de seguit melhorada',
'tog-usenewrc'                => 'Utilizar los darrièrs cambiaments melhorats (JavaScript)',
'tog-numberheadings'          => 'Numerotar automaticament los títols',
'tog-showtoolbar'             => 'Mostrar la barra de menut de modificacion (JavaScript)',
'tog-editondblclick'          => 'Modificar una pagina amb un clic doble (JavaScript)',
'tog-editsection'             => 'Modificar una seccion via los ligams [modificar]',
'tog-editsectiononrightclick' => 'Modificar una seccion en fasent un clic drech sus son títol (JavaScript)',
'tog-showtoc'                 => "Afichar l'ensenhador (per las paginas de mai de 3 seccions)",
'tog-rememberpassword'        => 'Se remembrar de mon senhal sus aqueste ordenador (cookie)',
'tog-editwidth'               => "Afichar la fenèstra d'edicion en plena largor",
'tog-watchcreations'          => 'Apondre las paginas que creï a ma lista de seguit',
'tog-watchdefault'            => 'Apondre las paginas que modifiqui a ma lista de seguit',
'tog-watchmoves'              => 'Apondre las paginas que tòrni nomenar a ma lista de seguit',
'tog-watchdeletion'           => 'Apondre las paginas que suprimissi de ma lista de seguit',
'tog-minordefault'            => 'Considerar mas modificacions coma menoras per defaut',
'tog-previewontop'            => 'Mostrar la previsualizacion al dessús de la zòna de modificacion',
'tog-previewonfirst'          => 'Mostrar la previsualizacion al moment de la primièra edicion',
'tog-nocache'                 => "Desactivar l'amagatal de paginas",
'tog-enotifwatchlistpages'    => 'M’avertir per corrièr electronic quand una pagina de ma lista de seguit es modificada',
'tog-enotifusertalkpages'     => 'M’avertir per corrièr electronic en cas de modificacion de ma pagina de discussion',
'tog-enotifminoredits'        => 'M’avertir per corrièr electronic quitament en cas de modificacions menoras',
'tog-enotifrevealaddr'        => 'Afichar mon adreça electronica dins la los corrièrs electronics d’avertiment',
'tog-shownumberswatching'     => "Afichar lo nombre d'utilizaires que seguisson aquesta pagina",
'tog-fancysig'                => 'Signatura bruta (sens ligam automatic)',
'tog-externaleditor'          => 'Utilizar un editor extèrn per defaut (pels utilizaires avançats, necessita una configuracion especiala sus vòstre ordenador)',
'tog-externaldiff'            => 'Utilizar un comparator extèrn per defaut (pels utilizaires avançats, necessita una configuracion especiala sus vòstre ordenador)',
'tog-showjumplinks'           => 'Activar los ligams « navigacion » e « recèrca » en naut de pagina (aparéncias Myskin e autres)',
'tog-uselivepreview'          => 'Utilizar l’apercebut rapid (JavaScript) (experimental)',
'tog-forceeditsummary'        => "M'avertir quand ai pas completat lo contengut de la bóstia de comentaris",
'tog-watchlisthideown'        => 'Amagar mas pròprias modificacions dins la lista de seguit',
'tog-watchlisthidebots'       => 'Amagar los cambiaments faches pels bòts dins la lista de seguit',
'tog-watchlisthideminor'      => 'Amagar las modificacions menoras dins la lista de seguit',
'tog-nolangconversion'        => 'Desactivar la conversion de las variantas de lenga',
'tog-ccmeonemails'            => 'Me mandar una còpia dels corrièrs electronics que mandi als autres utilizaires',
'tog-diffonly'                => 'Mostrar pas lo contengut de las paginas jos las difs',
'tog-showhiddencats'          => 'Afichar las categorias amagadas',

'underline-always'  => 'Totjorn',
'underline-never'   => 'Pas jamai',
'underline-default' => 'Segon lo navigador',

'skinpreview' => '(Previsualizar)',

# Dates
'sunday'        => 'dimenge',
'monday'        => 'diluns',
'tuesday'       => 'dimars',
'wednesday'     => 'dimècres',
'thursday'      => 'dijòus',
'friday'        => 'divendres',
'saturday'      => 'dissabte',
'sun'           => 'Dimg',
'mon'           => 'Dil',
'tue'           => 'Dima',
'wed'           => 'Dimè',
'thu'           => 'Dij',
'fri'           => 'Div',
'sat'           => 'Diss',
'january'       => 'genièr',
'february'      => 'febrièr',
'march'         => 'març',
'april'         => 'abril',
'may_long'      => 'mai',
'june'          => 'junh',
'july'          => 'julhet',
'august'        => 'agost',
'september'     => 'setembre',
'october'       => 'octobre',
'november'      => 'novembre',
'december'      => 'decembre',
'january-gen'   => 'Genièr',
'february-gen'  => 'Febrièr',
'march-gen'     => 'Març',
'april-gen'     => 'Abril',
'may-gen'       => 'Mai',
'june-gen'      => 'Junh',
'july-gen'      => 'Julhet',
'august-gen'    => 'Agost',
'september-gen' => 'Setembre',
'october-gen'   => 'Octobre',
'november-gen'  => 'Novembre',
'december-gen'  => 'Decembre',
'jan'           => 'gen',
'feb'           => 'feb',
'mar'           => 'març',
'apr'           => 'abr',
'may'           => 'mai',
'jun'           => 'junh',
'jul'           => 'julh',
'aug'           => 'ago',
'sep'           => 'set',
'oct'           => 'oct',
'nov'           => 'nov',
'dec'           => 'dec',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|Categoria|Categorias}}',
'category_header'                => 'Articles dins la categoria « $1 »',
'subcategories'                  => 'Soscategorias',
'category-media-header'          => 'Fichièrs multimèdia dins la categoria « $1 »',
'category-empty'                 => "''Actualament, aquesta categoria conten pas cap d'articles, de soscategoria o de fichièr multimèdia.''",
'hidden-categories'              => '{{PLURAL:$1|Categoria amagada|Categorias amagadas}}',
'hidden-category-category'       => 'Categorias amagadas', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|Aquesta categoria dispausa pas que de la soscategoria seguenta.|Aquesta categoria dispausa de {{PLURAL:$1|soscategoria|$1 soscategorias}}, sus un total de $2.}}',
'category-subcat-count-limited'  => 'Aquesta categoria dispausa {{PLURAL:$1|d’una soscategoria|de $1 soscategorias}}.',
'category-article-count'         => '{{PLURAL:$2|Aquesta categoria conten unicament la pagina seguenta.|{{PLURAL:$1|La pagina seguenta figura|Las $1 paginas seguentas figuran}} dins aquesta categoria, sus un total de $2.}}',
'category-article-count-limited' => '{{PLURAL:$1|La pagina seguenta figura|Las $1 paginas seguentas figuran}} dins la presenta categoria.',
'category-file-count'            => '{{PLURAL:$2|Aquesta categoria conten unicament lo fichièr seguent.|{{PLURAL:$1|Lo fichièr seguent figura|Los $1 fichièrs seguents figuran}} dins aquesta categoria, sus una soma de $2.}}',
'category-file-count-limited'    => '{{PLURAL:$1|Lo fichièr seguent figura|Los $1 fichièrs seguents figuran}} dins la presenta categoria.',
'listingcontinuesabbrev'         => '(seguida)',

'mainpagetext'      => "<big>'''MediaWiki es estat installat amb succès.'''</big>",
'mainpagedocfooter' => "Consultatz lo [http://meta.wikimedia.org/wiki/Ajuda:Contengut Guida de l'utilizaire] per mai d'entresenhas sus l'utilizacion d'aqueste logicial.

== Començar amb MediaWiki ==

* [http://www.mediawiki.org/wiki/Manual:Configuration_settings Lista dels paramètres de configuracion]
* [http://www.mediawiki.org/wiki/Manual:FAQ/fr FAQ MediaWiki]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce Lista de discussions de las parucions de MediaWiki]",

'about'          => 'A prepaus',
'article'        => 'Article',
'newwindow'      => '(dobrís una fenèstra novèla)',
'cancel'         => 'Anullar',
'qbfind'         => 'Recercar',
'qbbrowse'       => 'Far desfilar',
'qbedit'         => 'Modificar',
'qbpageoptions'  => 'Opcions de la pagina',
'qbpageinfo'     => 'Pagina d’entresenhas',
'qbmyoptions'    => 'Mas opcions',
'qbspecialpages' => 'Paginas especialas',
'moredotdotdot'  => 'E mai...',
'mypage'         => 'Ma pagina',
'mytalk'         => 'Ma pagina de discussion',
'anontalk'       => 'Discussion amb aquesta adreça IP',
'navigation'     => 'Navigacion',
'and'            => 'e',

# Metadata in edit box
'metadata_help' => 'Metadonadas :',

'errorpagetitle'    => 'Error de títol',
'returnto'          => 'Tornar a la pagina $1.',
'tagline'           => 'Un article de {{SITENAME}}.',
'help'              => 'Ajuda',
'search'            => 'Recercar',
'searchbutton'      => 'Recercar',
'go'                => 'Consultar',
'searcharticle'     => 'Consultar',
'history'           => 'Istoric',
'history_short'     => 'Istoric',
'updatedmarker'     => 'modificat dempuèi ma darrièra visita',
'info_short'        => 'Entresenhas',
'printableversion'  => 'Version imprimibla',
'permalink'         => 'Ligam istoric',
'print'             => 'Imprimir',
'edit'              => 'Modificar',
'create'            => 'Crear',
'editthispage'      => 'Modificar aquesta pagina',
'create-this-page'  => 'Crear aquesta pagina',
'delete'            => 'Suprimir',
'deletethispage'    => 'Suprimir aquesta pagina',
'undelete_short'    => 'Restablir {{PLURAL:$1|1 modificacion| $1 modificacions}}',
'protect'           => 'Protegir',
'protect_change'    => 'modificar',
'protectthispage'   => 'Protegir aquesta pagina',
'unprotect'         => 'Desprotegir',
'unprotectthispage' => 'Desprotegir aquesta pagina',
'newpage'           => 'Pagina novèla',
'talkpage'          => 'Pagina de discussion',
'talkpagelinktext'  => 'Discussion',
'specialpage'       => 'Pagina especiala',
'personaltools'     => 'Espleches personals',
'postcomment'       => 'Apondre un comentari',
'articlepage'       => "Vejatz l'article",
'talk'              => 'Discussion',
'views'             => 'Afichatges',
'toolbox'           => "Bóstia d'espleches",
'userpage'          => "Pagina d'utilizaire",
'projectpage'       => 'Pagina meta',
'imagepage'         => 'Pagina del mèdia',
'mediawikipage'     => 'Vejatz la pagina dels messatges',
'templatepage'      => 'Vejatz la pagina del modèl',
'viewhelppage'      => "Vejatz la pagina d'ajuda",
'categorypage'      => 'Vejatz la pagina de las categorias',
'viewtalkpage'      => 'Pagina de discussion',
'otherlanguages'    => 'Autras lengas',
'redirectedfrom'    => '(Redirigit dempuèi $1)',
'redirectpagesub'   => 'Pagina de redireccion',
'lastmodifiedat'    => "Darrièr cambiament d'aquesta pagina lo $1, a $2.", # $1 date, $2 time
'viewcount'         => 'Aquesta pagina es estada consultada {{PLURAL:$1|un còp|$1 còps}}.',
'protectedpage'     => 'Pagina protegida',
'jumpto'            => 'Anar a :',
'jumptonavigation'  => 'navigacion',
'jumptosearch'      => 'Recercar',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'A prepaus de {{SITENAME}}',
'aboutpage'            => 'Project:A prepaus',
'bugreports'           => "Rapòrt d'errors",
'bugreportspage'       => "Project:Rapòrt d'errors",
'copyright'            => 'Lo contengut es disponible segon los tèrmes de la licéncia $1.',
'copyrightpagename'    => '{{SITENAME}}, totes los dreches reservats',
'copyrightpage'        => '{{ns:project}}:Copyright',
'currentevents'        => 'Actualitats',
'currentevents-url'    => 'Project:Actualitats',
'disclaimers'          => 'Avertiments',
'disclaimerpage'       => 'Project:Avertiments generals',
'edithelp'             => 'Ajuda',
'edithelppage'         => 'Help:Cossí modificar una pagina',
'faq'                  => 'FAQ',
'faqpage'              => 'Project:FAQ',
'helppage'             => 'Help:Acuèlh',
'mainpage'             => 'Acuèlh',
'mainpage-description' => 'Acuèlh',
'policy-url'           => 'Project:Règlas',
'portal'               => 'Comunautat',
'portal-url'           => 'Project:Acuèlh',
'privacy'              => 'Politica de confidencialitat',
'privacypage'          => 'Project:Confidencialitat',

'badaccess'        => 'Error de permission',
'badaccess-group0' => 'Avètz pas los dreches sufisents per realizar l’accion que demandatz.',
'badaccess-group1' => "L’accion qu'ensajatz de realizar es pas accessibla qu’als utilizaires del grop $1.",
'badaccess-group2' => "L’accion qu'ensajatz de realizar es pas accessibla qu’als utilizaires dels gropes $1.",
'badaccess-groups' => "L’accion qu'ensajatz de realizar es pas accessibla qu’als utilizaires dels gropes $1.",

'versionrequired'     => 'Version $1 de MediaWiki necessària',
'versionrequiredtext' => 'La version $1 de MediaWiki es necessària per utilizar aquesta pagina. Consultatz [[Special:Version|la pagina de las versions]]',

'ok'                      => "D'acòrdi",
'retrievedfrom'           => 'Recuperada de « $1 »',
'youhavenewmessages'      => 'Avètz $1 ($2).',
'newmessageslink'         => 'de messatges novèls',
'newmessagesdifflink'     => 'darrièr cambiament',
'youhavenewmessagesmulti' => 'Avètz de messatges novèls sus $1',
'editsection'             => 'modificar',
'editold'                 => 'modificar',
'viewsourceold'           => 'veire la font',
'editsectionhint'         => 'Modificar la seccion : $1',
'toc'                     => 'Somari',
'showtoc'                 => 'afichar',
'hidetoc'                 => 'amagar',
'thisisdeleted'           => 'Desiratz afichar o restablir $1?',
'viewdeleted'             => 'Veire $1?',
'restorelink'             => '{{PLURAL:$1|una edicion escafada|$1 edicions escafadas}}',
'feedlinks'               => 'Flus :',
'feed-invalid'            => 'Tipe de flus invalid.',
'feed-unavailable'        => 'Los fluses de sindicacion son pas disponibles',
'site-rss-feed'           => 'Flus RSS de $1',
'site-atom-feed'          => 'Flus Atom de $1',
'page-rss-feed'           => 'Flus RSS de "$1"',
'page-atom-feed'          => 'Flus Atom de "$1"',
'red-link-title'          => '$1 (pagina pas encara redigida)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Article',
'nstab-user'      => "Pagina d'utilizaire",
'nstab-media'     => 'Pagina del mèdia',
'nstab-special'   => 'Especial',
'nstab-project'   => 'A prepaus',
'nstab-image'     => 'Fichièr',
'nstab-mediawiki' => 'Messatge',
'nstab-template'  => 'Modèl',
'nstab-help'      => 'Ajuda',
'nstab-category'  => 'Categoria',

# Main script and global functions
'nosuchaction'      => 'Accion desconeguda',
'nosuchactiontext'  => "L'accion especificada dins l'Url es pas reconeguda pel logicial {{SITENAME}}.",
'nosuchspecialpage' => 'Pagina especiala inexistanta',
'nospecialpagetext' => "<big>'''Avètz demandat una pagina especiala qu'es pas reconeguda pel logicial {{SITENAME}}.'''</big>

Una lista de las paginas especialas pòt èsser trobada sus [[Special:SpecialPages|{{int:specialpages}}]].",

# General errors
'error'                => 'Error',
'databaseerror'        => 'Error de la banca de donadas',
'dberrortext'          => 'Error de sintaxi dins la banca de donadas.
Benlèu qu\'aquesta error es deguda a una requèsta de recèrca incorrècta o a una error dins lo logicial.
La darrièra requèsta tractada per la banca de donadas èra :
<blockquote><tt>$1</tt></blockquote>
dempuèi la foncion "<tt>$2</tt>".
MySQL a renviat l\'error "<tt>$3: $4</tt>".',
'dberrortextcl'        => 'Una requèsta a la banca de donadas compòrta una error de sintaxi.
La darrièra requèsta mandada èra :
« $1 »
efectuada per la foncion « $2 ».
MySQL a retornat l\'error "$3: $4"',
'noconnect'            => 'Lo wiki rencontra actualament qualques dificultats tecnicas, e se pòt pas connectar al servidor de la banca de donadas. <br />
$1',
'nodb'                 => 'Impossible de seleccionar la banca de donadas $1',
'cachederror'          => 'Aquò es una còpia de la pagina demandada (version en amagatal) e poiriá èsser pas mesa a jorn.',
'laggedslavemode'      => 'Atencion : Aquesta pagina pòt conténer pas totes los darrièrs cambiaments efectuats.',
'readonly'             => 'Mesas a jorn blocadas sus la banca de donadas',
'enterlockreason'      => 'Indicatz la rason del blocatge, e mai una estimacion de sa durada',
'readonlytext'         => "Los ajustons e mesas a jorn de la banca de donadas son actualament blocats, probablament per permetre la mantenença de la banca, aprèp aquò, tot dintrarà dins l'òrdre. 

L’administrator qu'a varrolhat la banca de donadas a balhat l’explicacion seguenta : $1",
'missing-article'      => "La banca de donada a pas trobat lo tèxt d’una pagina qu’auriá degut trobar, intitolada « $1 » $2.

Aquò es, en principi, causat en seguissent lo ligam perimit d'un diff o de l’istoric cap a una pagina qu'es estada suprimida.

S'es pas lo cas, belèu avètz trobat un bòg dins lo programa.
Informatz-ne un [[Special:ListUsers/sysop|administrator]] aprèp aver notada l’adreça cibla del ligam.",
'missingarticle-rev'   => '(revision#: $1)',
'missingarticle-diff'  => '(Diff: $1, $2)',
'readonly_lag'         => 'La banca de donadas es estada automaticament clavada pendent que los servidors segondaris ratrapan lor retard sul servidor principal.',
'internalerror'        => 'Error intèrna',
'internalerror_info'   => 'Error intèrna: $1',
'filecopyerror'        => 'Impossible de copiar lo fichièr « $1 » cap a « $2 ».',
'filerenameerror'      => 'Impossible de tornar nomenar lo fichièr « $1 » en « $2 ».',
'filedeleteerror'      => 'Impossible de suprimir lo fichièr « $1 ».',
'directorycreateerror' => 'Impossible de crear lo dorsièr « $1 ».',
'filenotfound'         => 'Impossible de trobar lo fichièr « $1 ».',
'fileexistserror'      => 'Impossible d’escriure dins lo dorsièr « $1 » : lo fichièr existís',
'unexpected'           => 'Valor imprevista : « $1 » = « $2 ».',
'formerror'            => 'Error: Impossible de sometre lo formulari',
'badarticleerror'      => 'Aquesta accion pòt pas èsser efectuada sus aquesta pagina.',
'cannotdelete'         => 'Impossible de suprimir la pagina o lo fichièr indicat. (Benlèu la supression ja es estada efectuada per qualqu’un d’autre.)',
'badtitle'             => 'Títol marrit',
'badtitletext'         => 'Lo títol de la pagina demandada es invalid, void o s’agís d’un títol interlenga o interprojècte mal ligat. Benlèu conten un o maites caractèrs que pòdon pas èsser utilizats dins los títols.',
'perfdisabled'         => 'O planhèm ! Aquesta foncionalitat es temporàriament desactivada perque alentís la banca de donadas a un punt tal que degun pòt pas mai utilizar lo wiki.',
'perfcached'           => 'Aquò es una version en amagatal e benlèu es pas a jorn.',
'perfcachedts'         => 'Las donadas seguentas son en amagatal, son doncas pas obligatòriament a jorn. La darrièra actualizacion data del $1.',
'querypage-no-updates' => 'Las mesas a jorn per aquesta pagina son actualamnt desactivadas. Las donadas çaijós son pas mesas a jorn.',
'wrong_wfQuery_params' => 'Paramètres incorrèctes sus wfQuery()<br />
Foncion : $1<br />
Requèsta : $2',
'viewsource'           => 'Vejatz lo tèxt font',
'viewsourcefor'        => 'per $1',
'actionthrottled'      => 'Accion limitada',
'actionthrottledtext'  => "Per luchar contra lo spam, l’utilizacion d'aquesta accion es limitada a un cèrt nombre de còps dins una sosta pro corta. S'avèra qu'avètz depassat aquesta limita. Ensajatz tornamai dins qualques minutas.",
'protectedpagetext'    => 'Aquesta pagina es estada protegida per empachar sa modificacion.',
'viewsourcetext'       => 'Podètz veire e copiar lo contengut de l’article per poder trabalhar dessús :',
'protectedinterface'   => 'Aquesta pagina fornís de tèxt d’interfàcia pel logicial e es protegida per evitar los abuses.',
'editinginterface'     => "'''Atencion :''' sètz a editar una pagina utilizada per crear lo tèxt de l’interfàcia del logicial. Los cambiaments se repercutaràn, segon lo contèxt, sus totas o cèrtas paginas visiblas pels autres utilizaires. Per las traduccions, vos convidam a utilizar lo projècte Mediawiki d'internacionalizacion dels messatges [http://translatewiki.net/wiki/Main_Page?setlang=oc Betawiki].",
'sqlhidden'            => '(Requèsta SQL amagada)',
'cascadeprotected'     => "Aquesta pagina es actualament protegida perque es inclusa dins {{PLURAL:$1|la pagina seguenta|las paginas seguentas}}, {{PLURAL:$1|qu'es estada protegida|que son estadas protegidas}} amb l’opcion « proteccion en cascada » activada :
$2",
'namespaceprotected'   => "Avètz pas la permission de modificar las paginas de l’espaci de noms « '''$1''' ».",
'customcssjsprotected' => "Avètz pas la permission d'editar aquesta pagina perque conten de preferéncias d’autres utilizaires.",
'ns-specialprotected'  => 'Las paginas dins l’espaci de noms « {{ns:special}} » pòdon pas èsser modificadas',
'titleprotected'       => "Aqueste títol es estat protegit a la creacion per [[User:$1|$1]].
Lo motiu avançat es « ''$2'' ».",

# Virus scanner
'virus-badscanner'     => 'Marrida configuracion : escaner de virús desconegut : <i>$1</i>',
'virus-scanfailed'     => 'Fracàs de la recèrca (còde $1)',
'virus-unknownscanner' => 'antivirús desconegut :',

# Login and logout pages
'logouttitle'                => 'Desconnexion',
'logouttext'                 => "<strong>Ara, sètz desconnectat(ada).</strong>

Podètz contunhar d'utilizar {{SITENAME}} anonimament, o vos podètz [[Special:UserLogin|tornar connectar]] jol meteis nom o amb un autre nom.",
'welcomecreation'            => "== Benvenguda, $1 ! ==
Vòstre compte d'utilizaire es estat creat.
Doblidetz pas de personalizar vòstras [[Special:Preferences|{{SITENAME}} preferéncias]].",
'loginpagetitle'             => "S'enregistrar/Entrar",
'yourname'                   => "Vòstre nom d'utilizaire :",
'yourpassword'               => 'Vòstre senhal :',
'yourpasswordagain'          => 'Picatz vòstre senhal tornarmai :',
'remembermypassword'         => 'Se remembrar de mon senhal (cookie)',
'yourdomainname'             => 'Vòstre domeni',
'externaldberror'            => 'Siá una error s’es producha amb la banca de donadas d’autentificacion extèrna, siá sètz pas autorizat a metre a jorn vòstre compte extèrn.',
'loginproblem'               => '<b>Problèma d’identificacion.</b><br />Ensajatz tornarmai !',
'login'                      => 'Identificacion',
'nav-login-createaccount'    => 'Crear un compte o se connectar',
'loginprompt'                => 'Vos cal activar los cookies per vos connectar a {{SITENAME}}.',
'userlogin'                  => 'Crear un compte o se connectar',
'logout'                     => 'Se desconnectar',
'userlogout'                 => 'Desconnexion',
'notloggedin'                => 'Vos sètz pas identificat(ada)',
'nologin'                    => 'Avètz pas un compte ? $1.',
'nologinlink'                => 'Creatz un compte',
'createaccount'              => 'Crear un compte novèl',
'gotaccount'                 => 'Ja avètz un compte ? $1.',
'gotaccountlink'             => 'Identificatz-vos',
'createaccountmail'          => 'per corrièr electronic',
'badretype'                  => "Los senhals qu'avètz picats son pas identics.",
'userexists'                 => "Lo nom d'utilizaire qu'avètz picat ja es utilizat.
Causissètz-ne un autre.",
'youremail'                  => 'Adreça de corrièr electronic :',
'username'                   => "Nom de l'utilizaire :",
'uid'                        => 'Numèro de l’utilizaire :',
'prefs-memberingroups'       => 'Membre {{PLURAL:$1|del grop|dels gropes}} :',
'yourrealname'               => 'Nom vertadièr :',
'yourlanguage'               => "Lenga de l'interfàcia :",
'yourvariant'                => 'Varianta lingüistica :',
'yournick'                   => 'Signatura per las discussions :',
'badsig'                     => 'Signatura bruta incorrècta, verificatz vòstras balisas HTML.',
'badsiglength'               => 'Vòstra signatura es tròp longa.
Sa talha maximala deu èsser de $1 {{PLURAL:$1|caractèr|caractèrs}}.',
'email'                      => 'Corrièr electronic',
'prefs-help-realname'        => "(facultatiu) : se l'especificatz, serà utilizat per vos atribuir vòstras contribucions.",
'loginerror'                 => "Error d'identificacion",
'prefs-help-email'           => "L’adreça de corrièr electronic es facultativa mas permet de vos far adreçar vòstre senhal s'o doblidatz.
Tanben podètz causir de permetre a d’autres de vos contactar amb l'ajuda de vòstra pagina d’utilizaire principala o la de discussion sens aver besonh de revelar vòstra idenditat.",
'prefs-help-email-required'  => 'Una adreça de corrièr electronic es requesa.',
'nocookiesnew'               => "Lo compte d'utilizaire es estat creat, mas sètz pas connectat. {{SITENAME}} utiliza de cookies per la connexion mas los avètz desactivats. Activatz-los e reconnectatz-vos amb lo meteis nom e lo meteis senhal.",
'nocookieslogin'             => '{{SITENAME}} utiliza de cookies per la connexion mas avètz los cookies desactivats. Activatz-los e reconnectatz-vos.',
'noname'                     => "Avètz pas picat de nom d'utilizaire valid.",
'loginsuccesstitle'          => 'Identificacion capitada.',
'loginsuccess'               => 'Sètz actualament connectat(ada) sus {{SITENAME}} en tant que « $1 ».',
'nosuchuser'                 => "L'utilizaire « $1 » existís pas.
Verificatz qu'avètz plan ortografiat lo nom, o [[Special:Userlogin/signup|creatz-vos un compte novèl]].",
'nosuchusershort'            => 'I a pas de contributor amb lo nom « <nowiki>$1</nowiki> ». Verificatz l’ortografia.',
'nouserspecified'            => "Vos cal especificar vòstre nom d'utilizaire.",
'wrongpassword'              => 'Lo senhal es incorrècte. Ensajatz tornarmai.',
'wrongpasswordempty'         => 'Lo senhal picat èra void. Se vos plai, ensajatz tornarmai.',
'passwordtooshort'           => 'Vòstre senhal es tròp cort.
Deu conténer almens $1 caractèr{{PLURAL:$1||s}} e èsser diferent de vòtre nom d’utilizaire.',
'mailmypassword'             => 'Mandar un senhal novèl per corrièr electronic',
'passwordremindertitle'      => 'Senhal temporari novèl sus {{SITENAME}}',
'passwordremindertext'       => "Qualqu'un (probablament vos, amb l'adreça IP $1) a demandat un senhal novèl per {{SITENAME}} ($4).
Un senhal temporari es estat creat per
l’utilizaire « $2 » e es « $3 ». S'aquò èra vòstra intencion, vos caldrà
vos connectar e causir un senhal novèl.

Se sètz pas l’autor d'aquesta demanda, o se vos remembratz ara
de vòstre senhal ancian e que desiratz pas mai ne cambiar, podètz ignorar aqueste messatge e contunhar d'utilizar vòstre senhal ancian.",
'noemail'                    => "Cap d'adreça electronica es pas estada enregistrada per l'utilizaire « $1 ».",
'passwordsent'               => "Un senhal novèl es estat mandat a l'adreça electronica de l'utilizaire « $1 ».
Identificatz-vos tre que l'aurètz recebut.",
'blocked-mailpassword'       => 'Vòstra adreça IP es blocada en edicion, la foncion de rapèl del senhal es doncas desactivada per evitar los abuses.',
'eauthentsent'               => 'Un corrièr de confirmacion es estat mandat a l’adreça indicada.
Abans qu’un autre corrièr sià mandat a aqueste compte, deuretz seguir las instruccions donadas dins lo messatge per confirmar que sètz plan lo titular.',
'throttled-mailpassword'     => 'Un corrièr electronic de rapèl de vòstre senhal ja es estat mandat durant {{PLURAL:$1|la darrièra ora|las $1 darrièras oras}}. Per evitar los abuses, un sol corrièr de rapèl serà mandat per {{PLURAL:$1|ora|interval de $1 oras}}.',
'mailerror'                  => 'Error en mandant lo corrièr electronic : $1',
'acct_creation_throttle_hit' => "O planhèm, ja avètz {{PLURAL:$1|$1 compte creat|$1 comptes creats}}. Ne podètz pas crear d'autres.",
'emailauthenticated'         => 'Vòstra adreça de corrièr electronic es estada autentificada lo $1.',
'emailnotauthenticated'      => 'Vòstra adreça de corrièr electronic es <strong>pas encara autentificada</strong>. Cap corrièr serà pas mandat per caduna de las foncions seguentas.',
'noemailprefs'               => "<strong>Cap d'adreça electronica es pas estada indicada,</strong> las foncions seguentas seràn pas disponiblas.",
'emailconfirmlink'           => 'Confirmatz vòstra adreça de corrièr electronic',
'invalidemailaddress'        => "Aquesta adreça de corrièr electronic pòt pas èsser acceptada perque sembla qu'a un format incorrècte.
Picatz una adreça plan formatada o daissatz aqueste camp void.",
'accountcreated'             => 'Compte creat.',
'accountcreatedtext'         => "Lo compte d'utilizaire de $1 es estat creat.",
'createaccount-title'        => "Creacion d'un compte per {{SITENAME}}",
'createaccount-text'         => "Qualqu'un a creat un compte per vòstra adreça de corrièr electronic sus {{SITENAME}} ($4) intitolat « $2 », amb per senhal « $3 ». Deuriaz dobrir una sessilha e cambiar, tre ara, aqueste senhal.

Ignoratz aqueste messatge se aqueste compte es estat creat per error.",
'loginlanguagelabel'         => 'Lenga: $1',

# Password reset dialog
'resetpass'               => 'Remesa a zèro del senhal',
'resetpass_announce'      => 'Vos sètz enregistrat amb un senhal temporari mandat per corrièr electronic. Per acabar l’enregistrament, vos cal picar un senhal novèl aicí :',
'resetpass_text'          => '<!-- Apondètz lo tèxt aicí -->',
'resetpass_header'        => 'Remesa a zèro del senhal',
'resetpass_submit'        => 'Cambiar lo senhal e s’enregistrar',
'resetpass_success'       => 'Vòstre senhal es estat cambiat amb succès ! Enregistrament en cors...',
'resetpass_bad_temporary' => 'Senhal temporari invalid. Benlèu que ja avètz cambiat vòstre senhal amb succès, o demandat un senhal temporari novèl.',
'resetpass_forbidden'     => 'Los senhals pòdon pas èsser cambiats',
'resetpass_missing'       => 'Cap de donada pas picada.',

# Edit page toolbar
'bold_sample'     => 'Tèxt en gras',
'bold_tip'        => 'Tèxt en gras',
'italic_sample'   => 'Tèxt en italica',
'italic_tip'      => 'Tèxt en italica',
'link_sample'     => 'Títol del ligam',
'link_tip'        => 'Ligam intèrn',
'extlink_sample'  => 'http://www.example.com títol del ligam',
'extlink_tip'     => 'Ligam extèrn (doblidez pas lo prefix http://)',
'headline_sample' => 'Tèxt de sostítol',
'headline_tip'    => 'Sostítol nivèl 2',
'math_sample'     => 'Picatz vòstra formula aicí',
'math_tip'        => 'Formula matematica (LaTeX)',
'nowiki_sample'   => 'Picatz lo tèxt pas formatat aicí',
'nowiki_tip'      => 'Ignorar la sintaxi wiki',
'image_sample'    => 'Exemple.jpg',
'image_tip'       => 'Imatge inserit',
'media_sample'    => 'Exemple.ogg',
'media_tip'       => 'Ligam cap a un fichièr mèdia',
'sig_tip'         => 'Vòstra signatura amb la data',
'hr_tip'          => "Linha orizontala (n'abusetz pas)",

# Edit pages
'summary'                          => 'Resumit&nbsp;',
'subject'                          => 'Subjècte/títol',
'minoredit'                        => 'Aquò es un cambiament menor',
'watchthis'                        => 'Seguir aquesta pagina',
'savearticle'                      => 'Salvar',
'preview'                          => 'Previsualizar',
'showpreview'                      => 'Previsualizacion',
'showlivepreview'                  => 'Apercebut rapid',
'showdiff'                         => 'Cambiaments en cors',
'anoneditwarning'                  => "'''Atencion :''' sètz pas identificat(ada).
Vòstra adreça IP serà enregistrada dins l’istoric d'aquesta pagina.",
'missingsummary'                   => "'''Atencion :''' avètz pas modificat lo resumit de vòstra modificacion. Se clicatz tornarmai sul boton « Salvar », lo salvament serà facha sens avertiment novèl.",
'missingcommenttext'               => 'Mercé de metre un comentari çaijós.',
'missingcommentheader'             => "'''Rampèl :''' Avètz pas provesit de subjècte/títol per aqueste comentari. Se clicatz tornamai sus ''Salvar'', vòstra edicion serà enregistrada sens aquò.",
'summary-preview'                  => 'Previsualizacion del resumit',
'subject-preview'                  => 'Previsualizacion del subjècte/títol',
'blockedtitle'                     => "L'utilizaire es blocat",
'blockedtext'                      => "<big>'''Vòstre compte d'utilizaire o vòstra adreça IP es estat blocat'''</big>

Lo blocatge es estat efectuat per $1.
La rason invocada es la seguenta : ''$2''.

* Començament del blocatge : $8
* Expiracion del blocatge : $6
* Compte blocat : $7.

Podètz contactar $1 o un autre [[{{MediaWiki:Grouppage-sysop}}|administrator]] per ne discutir.
Podètz pas utilizar la foncion « Mandar un corrièr electronic a aqueste utilizaire » que se una adreça de corrièr valida es especificada dins vòstras [[Special:Preferences|preferéncias]].
Vòstra adreça IP actuala es $3 e vòstre identificant de blocatge es #$5.
Incluissètz aquesta adreça dins tota requèsta.",
'autoblockedtext'                  => 'Vòstra adreça IP es estada blocada automaticament perque es estada utilizada per un autre utilizaire, ele-meteis blocat per $1.
La rason invocadaa es :

:\'\'$2\'\'

* Començament del blocatge : $8
* Expiracion del blocatge : $6
* Compte blocat : $7

Podètz contactar $1 o un dels autres [[{{MediaWiki:Grouppage-sysop}}|administrators]] per discutir d\'aqueste blocatge.

Notatz que podètz pas utilizar la foncionalitat "Mandar un messatge a aqueste utilizaire" tant qu\'auretz pas  una adreça e-mail enregistrada dins vòstras [[Special:Preferences|preferéncias]] e tant que seretz pas blocat per son utilizacion.

Vòstra adreça IP actuala es $3, e lo numèro de blocatge es $5.
Precisatz aquestas indicacions dins totas las requèstas que faretz.',
'blockednoreason'                  => 'Cap de rason balhada',
'blockedoriginalsource'            => "Lo còde font de '''$1''' es indicat çaijós :",
'blockededitsource'                => "Lo contengut de '''vòstras modificacions''' aportadas a '''$1''' es indicat çaijós :",
'whitelistedittitle'               => 'Connexion necessària per modificar lo contengut',
'whitelistedittext'                => 'Vos cal èsser $1 per modificar las paginas.',
'confirmedittitle'                 => "Validacion de l'adreça de corrièr electronic necessària per modificar lo contengut",
'confirmedittext'                  => "Vos cal confirmar vòstra adreça electronica abans de modificar l'enciclopèdia. Picatz e validatz vòstra adreça electronica amb l'ajuda de la pagina [[Special:Preferences|preferéncias]].",
'nosuchsectiontitle'               => 'Seccion mancanta',
'nosuchsectiontext'                => "Avètz ensajat de modificar una seccion qu’existís pas. Coma i a pas de seccion $1, i a pas d'endrech ont salvar vòstras modificacions.",
'loginreqtitle'                    => 'Connexion necessària',
'loginreqlink'                     => 'connectar',
'loginreqpagetext'                 => 'Vos cal vos $1 per veire las autras paginas.',
'accmailtitle'                     => 'Senhal mandat.',
'accmailtext'                      => 'Lo senhal de « $1 » es estat mandat a $2.',
'newarticle'                       => '(Novèl)',
'newarticletext'                   => "Avètz seguit un ligam vèrs una pagina qu’existís pas encara o qu'es estada [{{fullurl:Special:Log|type=delete&page={{FULLPAGENAMEE}}}} escafada].
Per crear aquesta pagina, picatz vòstre tèxt dins la bóstia çaijós (podètz consultar [[{{MediaWiki:Helppage}}|la pagina d’ajuda]] per mai d’entresenhas).
Se sètz arribat(ada) aicí per error, clicatz sul boton '''retorn''' de vòstre navigador.",
'anontalkpagetext'                 => "---- ''Sètz sus la pagina de discussion d'un utilizaire anonim qu'a pas encara creat un compte o que n'utiliza pas.
Per aquesta rason, devèm utilizar son adreça IP per l'identificar. Una adreça d'aqueste tipe pòt èsser partejada entre mantuns utilizaires. Se sètz un utilizaire anonim e se constatatz que de comentaris que vos concernisson pas vos son estats adreçats, podètz [[Special:UserLogin/signup|crear un compte]] o [[Special:UserLogin|vos connectar]] per evitar tota confusion venenta amb d’autres contributors anonims.''",
'noarticletext'                    => "Pel moment, i a pas cap de tèxt sus aquesta pagina ; podètz [[Special:Search/{{PAGENAME}}|aviar una recèrca sul títol d'aquesta pagina]], verificar qu’es pas estada [{{fullurl:Special:Log|type=delete&page={{FULLPAGENAMEE}}}} suprimida] o [{{fullurl:{{FULLPAGENAME}}|action=edit}} modificar aquesta pagina].",
'userpage-userdoesnotexist'        => "Lo compte d'utilizaire « $1 » es pas enregistrat. Indicatz se volètz crear o editar aquesta pagina.",
'clearyourcache'                   => "'''Nòta :''' Aprèp aver publicat la pagina, vos cal forçar son recargament complet tot ignorant lo contengut actual de l'amagatal de vòstre navigador per veire los cambiaments : '''Mozilla / Firefox / Konqueror / Safari :''' mantenètz la tòca ''Majuscula'' (''Shift'') en clicant lo boton ''Actualizar'' (''Reload,'') o quichatz ''Maj-Ctrl-R'' (''Maj-Cmd-R'' sus Apple Mac) ; '''Internet Explorer / Opera :''' mantenètz la tòca ''Ctrl'' en clicant lo boton ''Actualizar'' o quichatz ''Ctrl-F5''.",
'usercssjsyoucanpreview'           => "<strong>Astúcia :</strong> Utilizatz lo boton 'Previsualizacion' per testar vòstre fuèlh novèl css/js abans de l'enregistrar.",
'usercsspreview'                   => "'''Remembratz-vos que sètz a previsualizar vòstre pròpri fuèlh CSS !'''
'''Es pas estada encara enregistrada !'''",
'userjspreview'                    => "'''Remembratz-vos que sètz a visualizar o testar vòstre còde JavaScript e qu’es pas encara estat enregistrat !'''",
'userinvalidcssjstitle'            => "'''Atencion :''' existís pas d'estil « $1 ». Remembratz-vos que las paginas personalas amb extensions .css e .js utilizan de títols en minusculas, per exemple, {{ns:user}}:Foo/monobook.css e non pas {{ns:user}}:Foo/Monobook.css.",
'updated'                          => '(Mes a jorn)',
'note'                             => '<strong>Nòta :</strong>',
'previewnote'                      => "<strong>Atencion, aqueste tèxt es pas qu'una previsualizacion e es pas encara estat salvat !</strong>",
'previewconflict'                  => 'Aquesta previsualizacion mòstra lo tèxt de la bóstia de modificacion superiora coma apareisserà se causissètz de lo salvar.',
'session_fail_preview'             => "<strong>Podèm pas enregistrar vòstra modificacion a causa d’una pèrda d’informacions concernent vòstra sesilha. 
Ensajatz tornarmai.
S'aquò fracassa encara, desconnectatz-vos, puèi [[Special:UserLogout|connectatz-vos]] tornamai.</strong>",
'session_fail_preview_html'        => "<strong>Podèm pas enregistrar vòstra modificacion a causa d’una pèrda d’informacions que concernís vòstra sesilha.</strong>

''Perque {{SITENAME}} a activat l’HTML brut, la previsualizacion es estada amagada per prevenir un atac per JavaScript.''

<strong>Se la temptativa de modificacion èra legitima, ensajatz encara.
S'aquò capita pas un còp de mai, [[Special:UserLogout|desconnectatz-vos]], puèi connectatz-vos tornamai.</strong>",
'token_suffix_mismatch'            => '<strong>Vòstra modificacion es pas estada acceptada perque vòstre navigador a mesclat los caractèrs de ponctuacion dins l’identificant d’edicion. La modificacion es estada regetada per empachar la corrupcion del tèxt de l’article. Aqueste problèma se produtz quand utilizatz un mandatari (proxy) anonim problematic.</strong>',
'editing'                          => 'Modificacion de $1',
'editingsection'                   => 'Modificacion de $1 (seccion)',
'editingcomment'                   => 'Modificacion de $1 (comentari)',
'editconflict'                     => 'Conflicte de modificacion : $1',
'explainconflict'                  => "Aqueste pagina es estada salvada aprèp qu'avètz començat de la modificar.
La zòna d'edicion superiora conten lo tèxt tal coma es enregistrat actualament dins la banca de donadas.
Vòstras modificacions apareisson dins la zòna d'edicion inferiora.
Anatz dever aportar vòstras modificacions al tèxt existent.
'''Sol''' lo tèxt de la zòna superiora serà salvat.",
'yourtext'                         => 'Vòstre tèxt',
'storedversion'                    => 'Version enregistrada',
'nonunicodebrowser'                => '<strong>Atencion : Vòstre navigador supòrta pas l’unicode. Una solucion temporària es estada trobada per vos permetre de modificar un article en tota seguretat : los caractèrs non-ASCII apareisseràn dins vòstra bóstia de modificacion en tant que còdes exadecimals. Deuriatz utilizar un navigador mai recent.</strong>',
'editingold'                       => "<strong>Atencion : sètz a modificar una version obsolèta d'aquesta pagina. Se salvatz, totas las modificacions efectuadas dempuèi aquesta version seràn perdudas.</strong>",
'yourdiff'                         => 'Diferéncias',
'copyrightwarning'                 => "Totas las contribucions a {{SITENAME}} son consideradas coma publicadas jols tèrmes de la $2 (vejatz $1 per mai de detalhs). Se desiratz pas que vòstres escriches sián modificats e distribuits a volontat, mercés de los sometre pas aicí.<br /> Nos prometètz tanben qu'avètz escrich aquò vos-meteis, o que l’avètz copiat d’una font provenent del domeni public, o d’una ressorsa liura.<strong>UTILIZETZ PAS DE TRABALHS JOS COPYRIGHT SENS AUTORIZACION EXPRÈSSA !</strong>",
'copyrightwarning2'                => "Totas las contribucions a {{SITENAME}} pòdon èsser modificadas o suprimidas per d’autres utilizaires. Se desiratz pas que vòstres escriches sián modificats e distribuits a volontat, mercés de los sometre pas aicí.<br /> Tanben nos prometètz qu'avètz escrich aquò vos-meteis, o que l’avètz copiat d’una font provenent del domeni public, o d’una ressorsa liura. (vejatz $1 per mai de detalhs). <strong>UTILIZETZ PAS DE TRABALHS JOS COPYRIGHT SENS AUTORIZACION EXPRÈSSA !</strong>",
'longpagewarning'                  => "<strong>AVERTIMENT : aquesta pagina a una longor de $1 ko.
De delà de 32 ko, es preferible per d'unes navigadors de devesir aquesta pagina en seccions mai pichonas. Benlèu deuriatz devesir la pagina en seccions mai pichonas.</strong>",
'longpageerror'                    => "<strong>ERROR: Lo tèxt qu'avètz mandat es de $1 Ko, e despassa doncas lo limit autorizat dels $2 Ko. Lo tèxt pòt pas èsser salvat.</strong>",
'readonlywarning'                  => '<strong>AVERTIMENT : La banca de donadas es estada varrolhada per mantenença,
doncas poiretz pas salvar vòstras modificacions ara. Podètz copiar lo tèxt dins un fichièr tèxt e lo salvar per mai tard.</strong>',
'protectedpagewarning'             => "<strong>AVERTIMENT : Aquesta pagina es protegida.
Sols los utilizaires amb l'estatut d'administrator la pòdon modificar. Asseguratz-vos que seguissètz las directivas concernent las paginas protegidas.</strong>",
'semiprotectedpagewarning'         => "'''Nòta:''' Aquesta pagina es estada blocada, pòt pas èsser editada que pels utilizaires enregistats.",
'cascadeprotectedwarning'          => "'''ATENCION :''' Aquesta pagina es estada protegida de biais que sols los administrators pòscan l’editar.
Aquesta proteccion es estada facha perque aquesta pagina es inclusa dins {{PLURAL:$1|una pagina protegida|de paginas protegidas}} amb la « proteccion en cascada » activada.",
'titleprotectedwarning'            => '<strong>ATENCION : Aquesta pagina es estada protegida de tal biais que sols cèrts utilizaires pòscan la crear.</strong>',
'templatesused'                    => 'Modèls utilizats sus aquesta pagina :',
'templatesusedpreview'             => 'Modèls utilizats dins aquesta previsualizacion :',
'templatesusedsection'             => 'Modèls utilizats dins aquesta seccion :',
'template-protected'               => '(protegit)',
'template-semiprotected'           => '(semiprotegit)',
'hiddencategories'                 => "{{PLURAL:$1|Categoria amagada|Categorias amagadas}} qu'aquesta pagina ne fa partida :",
'edittools'                        => '<!-- Tot tèxt picat aicí serà afichat jos las bóstias de modificacion o d’impòrt de fichièr. -->',
'nocreatetitle'                    => 'Creacion de pagina limitada',
'nocreatetext'                     => '{{SITENAME}} a restrencha la possibilitat de crear de paginas novèlas.
Podètz tonar en rèire e modificar una pagina existenta, [[Special:UserLogin|vos connectar o crear un compte]].',
'nocreate-loggedin'                => 'Avètz pas la permission de crear de paginas novèlas.',
'permissionserrors'                => 'Error de permissions',
'permissionserrorstext'            => 'Avètz pas la permission d’efectuar l’operacion demandada per {{PLURAL:$1|la rason seguenta|las rasons seguentas}} :',
'permissionserrorstext-withaction' => 'Sètz pas autorizat(ada) a $2, per {{PLURAL:$1|la rason seguenta|las rasons seguentas}} :',
'recreate-deleted-warn'            => "'''Atencion : sètz a tornar crear una pagina qu'es estada suprimida precedentament.'''

Demandatz-vos se es vertadièrament apropriat de la tornar crear en vos referissent al jornal de las supressions afichat çaijós :",

# Parser/template warnings
'expensive-parserfunction-warning'        => 'Atencion : Aquesta pagina conten tròp d’apèls dispendioses de foncions parsaires.

Ne deurià aver mens de $2 sul nombre actual $1.',
'expensive-parserfunction-category'       => 'Paginas amb tròp d’apèls dispendioses de foncions parsaires',
'post-expand-template-inclusion-warning'  => "Atencion : Aquesta pagina conten tròp d'inclusions de modèls.
D'unas inclusions seràn pas efectuadas.",
'post-expand-template-inclusion-category' => "Paginas que contenon tròp d'inclusions de modèls",
'post-expand-template-argument-warning'   => "Atencion : Aquesta pagina conten al mens un paramètre de modèl que l'inclusion es renduda impossibla. Aprèp extension, aqueste auriá produch un resultat tròp long, doncas, es pas estat inclut.",
'post-expand-template-argument-category'  => 'Paginas que contenon al mens un paramètre de modèl pas evaluat',

# "Undo" feature
'undo-success' => "Aquesta modificacion va èsser desfacha. Confirmatz los cambiaments (visibles en bas d'aquesta pagina), puèi salvatz se sètz d’acòrdi. Mercés de motivar l’anullacion dins la bóstia de resumit.",
'undo-failure' => 'Aquesta modificacion a pas pogut èsser desfacha a causa de conflictes amb de modificacions intermediàrias.',
'undo-norev'   => 'La modificacion a pas pogut èsser desfacha perque siá es inexistenta siá es estada suprimida.',
'undo-summary' => 'Anullacion de las modificacions $1 de [[Special:Contributions/$2|$2]] ([[User talk:$2|discutir]] | [[Special:Contributions/$2|{{MediaWiki:Contribslink}}]])',

# Account creation failure
'cantcreateaccounttitle' => 'Podètz pas crear de compte.',
'cantcreateaccount-text' => "La creacion de compte dempuèi aquesta adreça IP ('''$1''') es estada blocada per [[User:$3|$3]].

La rason balhada per $3 èra ''$2''.",

# History pages
'viewpagelogs'        => 'Vejatz las operacions per aquesta pagina',
'nohistory'           => "Existís pas d'istoric per aquesta pagina.",
'revnotfound'         => 'Version introbabla',
'revnotfoundtext'     => "La version precedenta d'aquesta pagina a pas pogut èsser retrobada. Verificatz l'URL qu'avètz utilizada per accedir a aquesta pagina.",
'currentrev'          => 'Version actuala',
'revisionasof'        => 'Version del $1',
'revision-info'       => 'Version del $1 per $2',
'previousrevision'    => '← Version precedenta',
'nextrevision'        => 'Version seguenta →',
'currentrevisionlink' => 'vejatz la version correnta',
'cur'                 => 'actu',
'next'                => 'seg',
'last'                => 'darr',
'page_first'          => 'primièra',
'page_last'           => 'darrièra',
'histlegend'          => 'Legenda : ({{MediaWiki:Cur}}) = diferéncia amb la version actuala ,
({{MediaWiki:Last}}) = diferéncia amb la version precedenta, <b>m</b> = cambiament menor',
'deletedrev'          => '[suprimit]',
'histfirst'           => 'Primièras contribucions',
'histlast'            => 'Darrièras contribucions',
'historysize'         => '({{PLURAL:$1|1 octet|$1 octets}})',
'historyempty'        => '(void)',

# Revision feed
'history-feed-title'          => 'Istoric de las versions',
'history-feed-description'    => 'Istoric per aquesta pagina sul wiki',
'history-feed-item-nocomment' => '$1 lo $2', # user at time
'history-feed-empty'          => 'La pagina demandada existís pas.
Benlèu es estada escafada o renomenada.
Ensajatz de [[Special:Search|recercar sul wiki]] per trobar de paginas en rapòrt.',

# Revision deletion
'rev-deleted-comment'         => '(comentari suprimit)',
'rev-deleted-user'            => '(nom d’utilizaire suprimit)',
'rev-deleted-event'           => '(entrada suprimida)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">
Aquesta version de la pagina es estada levada dels archius publics.
I Pòt aver de detalhs dins lo [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} jornal de las supressions].
</div>',
'rev-deleted-text-view'       => '<div class="mw-warning plainlinks"> Aquesta version de la pagina es estada levada dels archius publics. En tant qu’administrator d\'aqueste sit, la podètz visualizar ; i pòt aver de detahls dins lo [{{fullurl:Special:Log/delete|page={{PAGENAMEE}}}} jornal de las supressions]. </div>',
'rev-delundel'                => 'afichar/amagar',
'revisiondelete'              => 'Suprimir/Restablir de versions',
'revdelete-nooldid-title'     => 'Cibla per la revision invalida',
'revdelete-nooldid-text'      => "Avètz pas precisat la o las revision(s) cibla(s) per utilizar aquesta foncion, la revision cibla existís pas, o alara la revision cibla es la qu'es en cors.",
'revdelete-selected'          => '{{PLURAL:$2|Version seleccionada|Versions seleccionadas}} de [[:$1]] :',
'logdelete-selected'          => "{{PLURAL:$1|Eveniment d'istoric seleccionat|Eveniments d'istoric seleccionats}} :",
'revdelete-text'              => "Las versions suprimidas apareisseràn encara dins l’istoric de l’article, mas lor contengut textual serà inaccessible al public.

D’autres administrators sus {{SITENAME}} poiràn totjorn accedir al contengut amagat e lo restablir tornarmai a travèrs d'aquesta meteissa interfàcia, a mens qu’una restriccion suplementària siá mesa en plaça pels operators del sit.",
'revdelete-legend'            => 'Metre en plaça de restriccions de version :',
'revdelete-hide-text'         => 'Amagar lo tèxt de la version',
'revdelete-hide-name'         => 'Amagar l’accion e la cibla',
'revdelete-hide-comment'      => 'Amagar lo comentari de modificacion',
'revdelete-hide-user'         => 'Amagar lo pseudonim o l’adreça IP del contributor.',
'revdelete-hide-restricted'   => 'Aplicar aquestas restriccions als administrators e varrolhar aquesta interfàcia',
'revdelete-suppress'          => 'Suprimir las donadas dels administrators e tanben dels autres utilizaires',
'revdelete-hide-image'        => 'Amagar lo contengut del fichièr',
'revdelete-unsuppress'        => 'Levar las restriccions sus las versions restablidas',
'revdelete-log'               => "Comentari per l'istoric :",
'revdelete-submit'            => 'Aplicar a la version seleccionada',
'revdelete-logentry'          => 'La visibilitat de la version es estada modificada per [[$1]]',
'logdelete-logentry'          => 'La visibilitat de l’eveniment es estada modificada per [[$1]]',
'revdelete-success'           => "'''Visibilitat de las versions cambiadas amb succès.'''",
'logdelete-success'           => "'''Jornal de las visibilitat parametrat amb succès.'''",
'revdel-restore'              => 'Modificar la visibilitat',
'pagehist'                    => 'Istoric de la pagina',
'deletedhist'                 => 'Istoric de las supressions',
'revdelete-content'           => 'contengut',
'revdelete-summary'           => 'modificar lo somari',
'revdelete-uname'             => 'nom d’utilizaire',
'revdelete-restricted'        => 'aplicar las restriccions als administrators',
'revdelete-unrestricted'      => 'restriccions levadas pels administrators',
'revdelete-hid'               => 'amagar $1',
'revdelete-unhid'             => 'afichar $1',
'revdelete-log-message'       => '$1 per $2 {{PLURAL:$2|revision|revisions}}',
'logdelete-log-message'       => '$1 sus $2 {{PLURAL:$2|eveniment|eveniments}}',

# Suppression log
'suppressionlog'     => 'Jornal de las supressions',
'suppressionlogtext' => 'Çaijós, se tròba la tièra de las supressions e dels blocatges que comprenon las revisions amagadas als administrators. Vejatz [[Special:IPBlockList|la lista dels blocatges de las IP]] per la lista dels fòrabandiments e dels blocatges operacionals.',

# History merging
'mergehistory'                     => "Fusion dels istorics d'una pagina",
'mergehistory-header'              => "Aquesta pagina vos permet de fusionar las revisions de l'istoric d'una pagina d'origina vèrs una novèla.
Asseguratz-vos qu'aqueste cambiament pòsca conservar la continuitat de l'istoric.",
'mergehistory-box'                 => 'Fusionar las versions de doas paginas :',
'mergehistory-from'                => "Pagina d'origina :",
'mergehistory-into'                => 'Pagina de destinacion :',
'mergehistory-list'                => 'Edicion dels istorics fusionables',
'mergehistory-merge'               => "Las versions seguentas de [[:$1]] pòdon èsser fusionadas amb [[:$2]]. Utilizatz lo boton ràdio de la colomna per fusionar unicament las versions creadas del començament fins a la data indicada. Notatz plan que l'utilizacion dels ligams de navigacion reïnicializarà la colomna.",
'mergehistory-go'                  => 'Veire las edicions fusionablas',
'mergehistory-submit'              => 'Fusionar las revisions',
'mergehistory-empty'               => 'Cap de revision pòt pas èsser fusionada.',
'mergehistory-success'             => '$3 {{PLURAL:$3|revision|revisions}} de [[:$1]] {{PLURAL:$3|fusionada|fusionadas}} amb succès amb [[:$2]].',
'mergehistory-fail'                => 'Impossible de procedir a la fusion dels istorics. Seleccionatz  tornamai la pagina e mai los paramètres de data.',
'mergehistory-no-source'           => "La pagina d'origina $1 existís pas.",
'mergehistory-no-destination'      => 'La pagina de destinacion $1 existís pas.',
'mergehistory-invalid-source'      => 'La pagina d’origina deu aver un títol valid.',
'mergehistory-invalid-destination' => 'La pagina de destinacion deu aver un títol valid.',
'mergehistory-autocomment'         => '[[:$1]] fusionat amb [[:$2]]',
'mergehistory-comment'             => '[[:$1]] fusionat amb [[:$2]] : $3',

# Merge log
'mergelog'           => 'Istoric de las fusions',
'pagemerge-logentry' => '[[$1]] fusionada amb [[$2]] (revisions fins al $3)',
'revertmerge'        => 'Separar',
'mergelogpagetext'   => "Vaquí, çaijós, la lista de las fusions las mai recentas de l'istoric d'una pagina amb una autra.",

# Diffs
'history-title'           => 'Istoric de las versions de « $1 »',
'difference'              => '(Diferéncias entre las versions)',
'lineno'                  => 'Linha $1 :',
'compareselectedversions' => 'Comparar las versions seleccionadas',
'editundo'                => 'desfar',
'diff-multi'              => '({{PLURAL:$1|Una revision intermediària amagada|$1 revisions intermediàrias amagadas}})',

# Search results
'searchresults'             => 'Resultats de la recèrca',
'searchresulttext'          => "Per mai d'informacions sus la recèrca dins {{SITENAME}}, vejatz [[{{MediaWiki:Helppage}}|{{int:help}}]].",
'searchsubtitle'            => "Avètz recercat « '''[[:$1]]''' » ([[Special:Prefixindex/$1|totas las paginas que començan per « $1 »]] | [[Special:WhatLinksHere/$1|totas las paginas qu'an un ligam cap a « $1 »]])",
'searchsubtitleinvalid'     => 'Avètz recercat « $1 »',
'noexactmatch'              => "'''Cap de pagina amb lo títol « $1 » existís pas.
''' Podètz [[:$1|crear aqueste article]].",
'noexactmatch-nocreate'     => "'''I a pas de pagina intitolada « $1 ».'''",
'toomanymatches'            => 'Tròp d’ocuréncias son estadas trobadas, sètz pregat de sometre una requèsta diferenta.',
'titlematches'              => "Correspondéncias dins los títols d'articles",
'notitlematches'            => "Cap de títol d'article correspon pas a la recèrca.",
'textmatches'               => "Correspondéncias dins los tèxtes d'articles",
'notextmatches'             => "Cap de tèxt d'article correspon pas a la recèrca",
'prevn'                     => '$1 precedents',
'nextn'                     => '$1 seguents',
'viewprevnext'              => 'Veire ($1) ($2) ($3).',
'search-result-size'        => '$1 ({{PLURAL:$2|1 mot|$2 mots}})',
'search-result-score'       => 'Pertinéncia : $1%',
'search-redirect'           => '(redireccion vèrs $1)',
'search-section'            => '(seccion $1)',
'search-suggest'            => 'Avètz volgut dire : $1',
'search-interwiki-caption'  => 'Projèctes fraires',
'search-interwiki-default'  => '$1 resultats :',
'search-interwiki-more'     => '(mai)',
'search-mwsuggest-enabled'  => 'amb suggestions',
'search-mwsuggest-disabled' => 'sens suggestion',
'search-relatedarticle'     => 'Relatat',
'mwsuggest-disable'         => 'Desactivar las suggestions AJAX',
'searchrelated'             => 'relatat',
'searchall'                 => 'Totes',
'showingresults'            => "Afichatge {{PLURAL:$1|d''''1''' resultat|de '''$1''' resultats}} a partir del #'''$2'''.",
'showingresultsnum'         => "Afichatge {{PLURAL:$3|d''''1''' resultat|de '''$3''' resultats}} a partir del #'''$2'''.",
'showingresultstotal'       => "Visionament çaijós {{PLURAL:$3|del resultat '''$1''' de '''$3'''|dels resultats de '''$1 - $2''' de '''$3'''}}",
'nonefound'                 => '<strong>Nòta</strong>: l\'abséncia de resultat es sovent deguda a l\'emplec de tèrmes de recèrca tròp corrents, coma "a" o "de",
que son pas indexats, o a l\'emplec de mantun tèrme de recèrca (solas las paginas que
contenon totes los tèrmes apareisson dins los resultats).',
'powersearch'               => 'Recèrca avançada',
'powersearch-legend'        => 'Recèrca avançada',
'powersearch-ns'            => 'Recercar dins los espacis de nom :',
'powersearch-redir'         => 'Lista de las redireccions',
'powersearch-field'         => 'Recercar',
'search-external'           => 'Recèrca extèrna',
'searchdisabled'            => 'La recèrca sus {{SITENAME}} es desactivada.
En esperant la reactivacion, podètz efectuar una recèrca via Google.
Atencion, lor indexacion de contengut {{SITENAME}} benlèu es pas a jorn.',

# Preferences page
'preferences'              => 'Preferéncias',
'mypreferences'            => 'Mas preferéncias',
'prefs-edits'              => 'Nombre d’edicions :',
'prefsnologin'             => 'Vos sètz pas identificat(ada)',
'prefsnologintext'         => 'Vos cal èsser <span class="plainlinks">[{{fullurl:Special:Userlogin|returnto=$1}} connectat(ada)]</span> per modificar vòstras preferéncias d’utilizaire.',
'prefsreset'               => 'Las preferéncias son estadas restablidas a partir de la version enregistrada.',
'qbsettings'               => "Barra d'espleches",
'qbsettings-none'          => 'Cap',
'qbsettings-fixedleft'     => 'Esquèrra',
'qbsettings-fixedright'    => 'Drecha',
'qbsettings-floatingleft'  => 'Flotanta a esquèrra',
'qbsettings-floatingright' => 'Flotanta a drecha',
'changepassword'           => 'Modificacion del senhal',
'skin'                     => 'Aparéncia',
'math'                     => 'Rendut de las matas',
'dateformat'               => 'Format de data',
'datedefault'              => 'Cap de preferéncia',
'datetime'                 => 'Data e ora',
'math_failure'             => 'Error matas',
'math_unknown_error'       => 'error indeterminada',
'math_unknown_function'    => 'foncion desconeguda',
'math_lexing_error'        => 'error lexicala',
'math_syntax_error'        => 'error de sintaxi',
'math_image_error'         => 'La conversion en PNG a pas capitat ; verificatz l’installacion de Latex, dvips, gs e convert',
'math_bad_tmpdir'          => 'Impossible de crear o d’escriure dins lo repertòri math temporari',
'math_bad_output'          => 'Impossible de crear o d’escriure dins lo repertòri math de sortida',
'math_notexvc'             => 'L’executable « texvc » es introbable. Legissètz math/README per lo configurar.',
'prefs-personal'           => 'Entresenhas personalas',
'prefs-rc'                 => 'Darrièrs cambiaments',
'prefs-watchlist'          => 'Lista de seguit',
'prefs-watchlist-days'     => "Nombre de jorns d'afichar dins la lista de seguit :",
'prefs-watchlist-edits'    => "Nombre de modificacions d'afichar dins la lista de seguit espandida :",
'prefs-misc'               => 'Preferéncias divèrsas',
'saveprefs'                => 'Enregistrar las preferéncias',
'resetprefs'               => 'Restablir las preferéncias',
'oldpassword'              => 'Senhal ancian :',
'newpassword'              => 'Senhal novèl :',
'retypenew'                => 'Confirmar lo senhal novèl :',
'textboxsize'              => 'Fenèstra de modificacion',
'rows'                     => 'Rengadas :',
'columns'                  => 'Colomnas :',
'searchresultshead'        => 'Recèrca',
'resultsperpage'           => 'Nombre de responsas per pagina :',
'contextlines'             => 'Nombre de linhas per responsa :',
'contextchars'             => 'Nombre de caractèrs de contèxt per linha :',
'stub-threshold'           => 'Limita superiora pels <a href="#" class="stub">ligams vèrs los esbòses</a> (octets) :',
'recentchangesdays'        => "Nombre de jorns d'afichar dins los darrièrs cambiaments :",
'recentchangescount'       => "Nombre de modificacions d'afichar dins los darrièrs cambiaments :",
'savedprefs'               => 'Las preferéncias son estadas salvadas.',
'timezonelegend'           => 'Zòna orària',
'timezonetext'             => '¹Nombre d’oras de decalatge entre vòstra ora locala e l’ora del servidor (UTC).',
'localtime'                => 'Ora locala',
'timezoneoffset'           => 'Decalatge orari¹ :',
'servertime'               => 'Ora del servidor',
'guesstimezone'            => 'Utilizar la valor del navigador',
'allowemail'               => 'Autorizar lo mandadís de corrièr electronic venent d’autres utilizaires',
'prefs-searchoptions'      => 'Opcions de recèrca',
'prefs-namespaces'         => 'Noms d’espacis',
'defaultns'                => 'Per defaut, recercar dins aquestes espacis :',
'default'                  => 'defaut',
'files'                    => 'Fichièrs',

# User rights
'userrights'                  => "Gestion dels dreches d'utilizaire", # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => "Gestion dels dreches d'utilizaire",
'userrights-user-editname'    => 'Entrar un nom d’utilizaire :',
'editusergroup'               => "Modificacion dels gropes d'utilizaires",
'editinguser'                 => "Cambiament dels dreches de l'utilizaire '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup'    => 'Modificar los gropes de l’utilizaire',
'saveusergroups'              => "Salvar los gropes d'utilizaires",
'userrights-groupsmember'     => 'Membre de :',
'userrights-groups-help'      => "Podètz modificar los gropes alsquals aparten aqueste utilizaire.
* Una casa marcada significa que l'utilizaire se tròba dins aqueste grop.
* Una casa pas marcada significa, al contrari, que s’i tròba pas.
* Una * indica que podretz pas levar aqueste grop un còp que l'auretz apondut e vice-versa.",
'userrights-reason'           => 'Motiu del cambiament :',
'userrights-no-interwiki'     => "Sètz pas abilitat per modificar los dreches dels utilizaires sus d'autres wikis.",
'userrights-nodatabase'       => 'La banca de donadas « $1 » existís pas o es pas en local.',
'userrights-nologin'          => "Vos cal [[Special:UserLogin|vos connectar]] amb un compte d'administrator per balhar los dreches d'utilizaire.",
'userrights-notallowed'       => "Vòstre compte es pas abilitat per modificar de dreches d'utilizaire.",
'userrights-changeable-col'   => 'Los gropes que podètz cambiar',
'userrights-unchangeable-col' => 'Los gropes que podètz pas cambiar',

# Groups
'group'               => 'Grop :',
'group-user'          => 'Utilizaires',
'group-autoconfirmed' => 'Utilizaires enregistrats',
'group-bot'           => 'Bòts',
'group-sysop'         => 'Administrators',
'group-bureaucrat'    => 'Burocratas',
'group-suppress'      => 'Supervisors',
'group-all'           => '(totes)',

'group-user-member'          => 'Utilizaire',
'group-autoconfirmed-member' => 'Utilizaire enregistrat',
'group-bot-member'           => 'Bòt',
'group-sysop-member'         => 'Administrator',
'group-bureaucrat-member'    => 'Burocrata',
'group-suppress-member'      => 'Supervisor',

'grouppage-user'          => '{{ns:project}}:Utilizaires',
'grouppage-autoconfirmed' => '{{ns:project}}:Utilizaires enregistrats',
'grouppage-bot'           => '{{ns:project}}:Bòts',
'grouppage-sysop'         => '{{ns:project}}:Administrators',
'grouppage-bureaucrat'    => '{{ns:project}}:Burocratas',
'grouppage-suppress'      => '{{ns:project}}:Supervisor',

# Rights
'right-read'                 => 'Legir las paginas',
'right-edit'                 => 'Modificar las paginas',
'right-createpage'           => 'Crear de paginas (que son pas de paginas de discussion)',
'right-createtalk'           => 'Crear de paginas de discussion',
'right-createaccount'        => "Crear de comptes d'utilizaire novèls",
'right-minoredit'            => 'Marcar de cambiaments coma menors',
'right-move'                 => 'Tornar nomenar de paginas',
'right-move-subpages'        => 'Desplaçar de paginas amb lor sospaginas',
'right-suppressredirect'     => 'Crear pas de redireccion dempuèi la pagina anciana en renomenant la pagina',
'right-upload'               => 'Telecargar de fichièrs',
'right-reupload'             => 'Espotir un fichièr existent',
'right-reupload-own'         => 'Espotir un fichièr telecargat pel meteis utilizaire',
'right-reupload-shared'      => 'Espotir localament un fichièr present sus un depaus partejat',
'right-upload_by_url'        => 'Importar un fichièr dempuèi una adreça URL',
'right-purge'                => "Purgar l'amagatal de las paginas sens l'aver de confirmar",
'right-autoconfirmed'        => 'Modificar las paginas semiprotegidas',
'right-bot'                  => 'Èsser tractat coma un procediment automatizat',
'right-nominornewtalk'       => 'Desenclavar pas lo bendèl "Avètz de messatges novèls" al moment d\'un cambiament menor sus una pagina de discussion d\'un utilizaire',
'right-apihighlimits'        => "Utilizar de limits superiors dins las requèstas l'API",
'right-writeapi'             => "Utilizar l'API per modificar lo wiki",
'right-delete'               => 'Suprimir de paginas',
'right-bigdelete'            => "Suprimir de paginas amb d'istorics grands",
'right-deleterevision'       => "Suprimir e restablir una revision especifica d'una pagina",
'right-deletedhistory'       => 'Veire las entradas dels istorics suprimits mas sens lor tèxt',
'right-browsearchive'        => 'Recercar de paginas suprimidas',
'right-undelete'             => 'Restablir una pagina',
'right-suppressrevision'     => 'Examinar e restablir las revisions amagadas als administrators',
'right-suppressionlog'       => 'Veire los jornals privats',
'right-block'                => "Blocar d'autres utilizaires en escritura",
'right-blockemail'           => 'Empachar un utilizaire de mandar de corrièrs electronics',
'right-hideuser'             => 'Blocar un utilizaire en amagant son nom al public',
'right-ipblock-exempt'       => "Èsser pas afectat per las IP blocadas, los blocatges automatics e los blocatges de plajas d'IP",
'right-proxyunbannable'      => 'Èsser pas afectat pels blocatges automatics de servidors mandataris',
'right-protect'              => 'Modificar lo nivèl de proteccion de las paginas e modificar las paginas protegidas',
'right-editprotected'        => 'Modificar las paginas protegidas (sens proteccion en cascada)',
'right-editinterface'        => "Modificar l'interfàcia d'utilizaire",
'right-editusercssjs'        => "Modificar los fichièrs CSS e JS d'autres utilizaires",
'right-rollback'             => "Revocacion rapida del darrièr utilizaire qu'a modificat una pagina particulara",
'right-markbotedits'         => 'Marcar los cambiaments revocats coma de cambiaments que son estats fachs per de robòts',
'right-noratelimit'          => 'Pas afectat pels limits de taus',
'right-import'               => "Importar de paginas dempuèi d'autres wikis",
'right-importupload'         => 'Importar de paginas dempuèi un fichièr',
'right-patrol'               => 'Marcar de cambiaments coma verificats',
'right-autopatrol'           => 'Aver sos cambiaments marcats automaticament coma verificats',
'right-patrolmarks'          => 'Utilizar las foncionalitats de la patrolha dels darrièrs cambiaments',
'right-unwatchedpages'       => 'Veire la tièra de las paginas pas seguidas',
'right-trackback'            => 'Apondre de retroligams',
'right-mergehistory'         => 'Fusionar los istorics de las paginas',
'right-userrights'           => "Modificar totes los dreches d'un utilizaire",
'right-userrights-interwiki' => "Modificar los dreches d'utilizaires que son sus un autre wiki",
'right-siteadmin'            => 'Varrolhar e desvarrolhar la banca de donadas',

# User rights log
'rightslog'      => "Istoric de las modificacions d'estatut",
'rightslogtext'  => "Aquò es un jornal dels cambiaments d'estatut d’utilizaire.",
'rightslogentry' => 'a modificat los dreches de l’utilizaire « $1 » de $2 a $3',
'rightsnone'     => '(cap)',

# Recent changes
'nchanges'                          => '$1 {{PLURAL:$1|cambiament|cambiaments}}',
'recentchanges'                     => 'Darrièrs cambiaments',
'recentchangestext'                 => 'Vaquí sus aquesta pagina, los darrièrs cambiaments de {{SITENAME}}.',
'recentchanges-feed-description'    => "Seguissètz los darrièrs cambiaments d'aqueste wiki dins un flus.",
'rcnote'                            => 'Vaquí {{PLURAL:$1|lo darrièr cambiament|los $1 darrièrs cambiaments}} dempuèi {{PLURAL:$2|lo darrièr jorn|los <b>$2</b> darrièrs jorns}}, determinat{{PLURAL:$1||s}} lo $4, a $5.',
'rcnotefrom'                        => "Vaquí los cambiaments efectuats dempuèi lo '''$2''' ('''$1''' al maximom).",
'rclistfrom'                        => 'Afichar las modificacions novèlas dempuèi lo $1.',
'rcshowhideminor'                   => '$1 cambiaments menors',
'rcshowhidebots'                    => '$1 robòts',
'rcshowhideliu'                     => '$1 utilizaires enregistrats',
'rcshowhideanons'                   => '$1 contribucions d’IP',
'rcshowhidepatr'                    => '$1 edicions susvelhadas',
'rcshowhidemine'                    => '$1 mas edicions',
'rclinks'                           => 'Afichar los $1 darrièrs cambiaments efectuats al cors dels $2 darrièrs jorns; $3 cambiaments menors.',
'diff'                              => 'dif',
'hist'                              => 'ist',
'hide'                              => 'amagar',
'show'                              => 'mostrar',
'minoreditletter'                   => 'm',
'newpageletter'                     => 'N',
'boteditletter'                     => 'b',
'number_of_watching_users_pageview' => '[$1 {{PLURAL:$1|utilizaire seguent|utilizaires seguents}}]',
'rc_categories'                     => 'Limit de las categorias (separacion amb « | »)',
'rc_categories_any'                 => 'Totas',
'newsectionsummary'                 => '/* $1 */ seccion novèla',

# Recent changes linked
'recentchangeslinked'          => 'Seguit dels ligams',
'recentchangeslinked-title'    => 'Seguit dels ligams associats a "$1"',
'recentchangeslinked-noresult' => 'Cap de cambiament sus las paginas ligadas pendent lo periòde causit.',
'recentchangeslinked-summary'  => "Aquesta pagina especiala mòstra los darrièrs cambiaments sus las paginas que son ligadas. Las paginas de [[Special:Watchlist|vòstra tièra de seguit]] son '''en gras'''.",
'recentchangeslinked-page'     => 'Nom de la pagina :',
'recentchangeslinked-to'       => 'Afichar los cambiaments vèrs las paginas ligadas al luòc de la pagina donada',

# Upload
'upload'                      => 'Importar un fichièr',
'uploadbtn'                   => 'Importar lo fichièr',
'reupload'                    => 'Importar tornarmai',
'reuploaddesc'                => 'Anullar lo cargament e tornar al formulari.',
'uploadnologin'               => 'Vos sètz pas identificat(ada)',
'uploadnologintext'           => 'Vos cal èsser [[Special:UserLogin|connectat(ada)]]
per copiar de fichièrs sul servidor.',
'upload_directory_missing'    => 'Lo repertòri d’impòrt ($1) es mancant e a pas pogut èsser creat pel servidor web.',
'upload_directory_read_only'  => 'Lo servidor Web pòt escriure dins lo dorsièr cibla ($1).',
'uploaderror'                 => 'Error',
'uploadtext'                  => "Utilizatz lo formulari çaijós per importar de fichièrs sul servidor.
Per veire o recercar d'imatges precedentament mandats, consultatz [[Special:ImageList|la tièra dels imatges]]. Las còpias e las supressions tanben son enregistradas dins l'[[Special:Log/upload|istoric dels impòrts]], les supressions dins l’[[Special:Log/delete|istoric de las supressions]].

Per enclure un imatge dins una pagina, utilizatz un ligam de la forma
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:fichièr.jpg]]</nowiki></tt>''',
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:fichièr.png|200px|thumb|left|tèxt descriptiu]]</nowiki></tt>''' per utilizar una miniatura de 200 pixèls de larg dins una bóstia a esquèrra amb 'tèxt descriptiu' coma descripcion
* '''<tt><nowiki>[[</nowiki>{{ns:media}}<nowiki>:fichièr.ogg]]</nowiki></tt>''' per ligar dirèctament cap al fichièr sens l'afichar.",
'upload-permitted'            => 'Formats de fichièrs autorizats : $1.',
'upload-preferred'            => 'Formats de fichièrs preferits : $1.',
'upload-prohibited'           => 'Formats de fichièrs interdiches : $1.',
'uploadlog'                   => 'Istoric de las importacions',
'uploadlogpage'               => 'Istoric de las importacions de fichièrs multimèdia',
'uploadlogpagetext'           => 'Vaquí la tièra dels darrièrs fichièrs copiats sul servidor.
Vejatz la [[Special:NewImages|galariá dels imatges novèls]] per una presentacion mai visuala.',
'filename'                    => 'Nom del fichièr',
'filedesc'                    => 'Descripcion',
'fileuploadsummary'           => 'Resumit :',
'filestatus'                  => "Estatut dels dreches d'autor :",
'filesource'                  => 'Font :',
'uploadedfiles'               => 'Fichièrs importats',
'ignorewarning'               => 'Ignorar l’avertiment e salvar lo fichièr',
'ignorewarnings'              => 'Ignorar los avertiments al moment de l’impòrt',
'minlength1'                  => 'Los noms de fichièrs devon comprendre almens una letra.',
'illegalfilename'             => 'Lo nom de fichièr « $1 » conten de caractèrs interdiches dins los títols de paginas. Mercé de lo tornar nomenar e de lo copiar tornarmai.',
'badfilename'                 => "L'imatge es estat renomenat « $1 ».",
'filetype-badmime'            => 'Los fichièrs del tipe MIME « $1 » pòdon pas èsser importats.',
'filetype-unwanted-type'      => "«.$1»''' es un format de fichièr pas desirat.
{{PLURAL:$3|Lo tipe de fichièr preconizat es|Los tipes de fichièrs preconizats son}} $2.",
'filetype-banned-type'        => "'''\".\$1\"''' es dins un format pas admes.
{{PLURAL:\$3|Lo qu'es acceptat es|Los que son acceptats son}} \$2.",
'filetype-missing'            => "Lo fichièr a pas cap d'extension (coma « .jpg » per exemple).",
'large-file'                  => 'Los fichièrs importats deurián pas èsser mai gros que $1 ; aqueste fichièr fa $2.',
'largefileserver'             => "La talha d'aqueste fichièr es superiora al maximom autorizat.",
'emptyfile'                   => 'Lo fichièr que volètz importar sembla void. Aquò pòt èsser degut a una error dins lo nom del fichièr. Verificatz que desiratz vertadièrament copiar aqueste fichièr.',
'fileexists'                  => 'Un fichièr amb aqueste nom existís ja. Mercé de verificar <strong><tt>$1</tt></strong>. Sètz segur de voler modificar aqueste fichièr ?',
'filepageexists'              => "La pagina de descripcion per aqueste fichièr ja es estada creada aicí <strong><tt>$1</tt></strong>, mas cap de fichièr d'aqueste nom existís pas actualament. Lo resumit qu'anatz escriure remplaçarà pas lo tèxt precedent ; per aquò far, deuretz editar manualament la pagina.",
'fileexists-extension'        => "Un fichièr amb un nom similar existís ja :<br />
Nom del fichièr d'importar : <strong><tt>$1</tt></strong><br />
Nom del fichièr existent : <strong><tt>$2</tt></strong><br />
la sola diferéncia es la cassa (majusculas / minusculas) de l’extension. Verificatz que lo fichièr es diferent e cambiatz son nom.",
'fileexists-thumb'            => "<center>'''Imatge existent'''</center>",
'fileexists-thumbnail-yes'    => 'Lo fichièr sembla èsser un imatge en talha reducha <i>(thumbnail)</i>. Verificatz lo fichièr <strong><tt>$1</tt></strong>.<br /> Se lo fichièr verificat es lo meteis imatge (dins una resolucion melhora), es pas de besonh d’importar una version reducha.',
'file-thumbnail-no'           => 'Lo nom del fichièr comença per <strong><tt>$1</tt></strong>.
Es possible que s’agisca d’una version reducha <i>(miniatura)</i>.
Se dispausatz del fichièr en resolucion nauta, importatz-lo, si que non cambiatz lo nom del fichièr.',
'fileexists-forbidden'        => 'Un fichièr amb aqueste nom existís ja ; mercé de tornar en arrièr e de copiar lo fichièr jos un nom novèl. [[Image:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => "Un fichièr amb lo meteis nom existís ja dins la banca de donadas comuna.
S'o volètz importar tornamai, tornatz en rèire e importatz-lo jos un autre nom. [[Image:$1|thumb|center|$1]]",
'file-exists-duplicate'       => 'Aqueste fichièr es un doble {{PLURAL:$1|del fichièr seguent|dels fichièrs seguents}} :',
'successfulupload'            => 'Importacion capitada',
'uploadwarning'               => 'Atencion !',
'savefile'                    => 'Salvar lo fichièr',
'uploadedimage'               => '«[[$1]]» copiat sul servidor',
'overwroteimage'              => 'a importat una version novèla de « [[$1]] »',
'uploaddisabled'              => 'O planhèm, lo mandadís de fichièr es desactivat.',
'uploaddisabledtext'          => "L'impòrt de fichièrs cap al servidor es desactivat.",
'uploadscripted'              => "Aqueste fichièr conten de còde HTML o un escript que poiriá èsser interpretat d'un biais incorrècte per un navigador Internet.",
'uploadcorrupt'               => 'Aqueste fichièr es corromput, a una talha nulla o a una extension invalida. Verificatz lo fichièr.',
'uploadvirus'                 => 'Aqueste fichièr conten un virús ! Per mai de detalhs, consultatz : $1',
'sourcefilename'              => 'Nom del fichièr font :',
'destfilename'                => 'Nom jolqual lo fichièr serà enregistrat&nbsp;:',
'upload-maxfilesize'          => 'Talha maximala del fichièr : $1',
'watchthisupload'             => 'Seguir aquesta pagina',
'filewasdeleted'              => 'Un fichièr amb aqueste nom ja es estat copiat, puèi suprimit. Vos caldriá verificar lo $1 abans de procedir a una còpia novèla.',
'upload-wasdeleted'           => "'''Atencion : Sètz a importar un fichièr que ja es estat suprimit deperabans.'''

Deuriatz considerar se es oportun de contunhar l'impòrt d'aqueste fichièr. Lo jornal de las supressions vos donarà los elements d'informacion.",
'filename-bad-prefix'         => 'Lo nom del fichièr qu\'importatz comença per <strong>"$1"</strong> qu\'es un nom generalament donat pels aparelhs de fòto numerica e que decritz pas lo fichièr. Causissetz un nom de fichièr descrivent vòstre fichièr.',
'filename-prefix-blacklist'   => ' #<!-- daissatz aquesta linha coma es --> <pre>
# La sintaxi es la seguenta :
#   * Tot çò que seguís lo caractèr "#" fins a la fin de la linha es un comentari
#   * Tota linha non vioda es un prefix tipic de nom de fichièr assignat automaticament pels aparelhs numerics
CIMG # Casio
DSC_ # Nikon
DSCF # Fuji
DSCN # Nikon
DUW # some mobil phones
IMG # generic
JD # Jenoptik
MGP # Pentax
PICT # misc.
 #</pre> <!-- daissatz aquesta linha coma es -->',

'upload-proto-error'      => 'Protocòl incorrècte',
'upload-proto-error-text' => "L’impòrt requerís d'URLs començant per <code>http://</code> o <code>ftp://</code>.",
'upload-file-error'       => 'Error intèrna',
'upload-file-error-text'  => 'Una error intèrna es subrevenguda en volent crear un fichièr temporari sul servidor. Contactatz un [[Special:ListUsers/sysop|administrator de sistèma]].',
'upload-misc-error'       => 'Error d’impòrt desconeguda',
'upload-misc-error-text'  => 'Una error desconeguda es subrevenguda pendent l’impòrt.
Verificatz que l’URL es valida e accessibla, puèi ensajatz tornarmai.
Se lo problèma persistís, contactatz un [[Special:ListUsers/sysop|administrator del sistèma]].',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => 'Pòt pas aténher l’URL',
'upload-curl-error6-text'  => 'L’URL fornida pòt pas èsser atenhuda. Verificatz que l’URL es corrècta e que lo sit es en linha.',
'upload-curl-error28'      => 'Depassament de la sosta al moment de l’impòrt',
'upload-curl-error28-text' => "Lo sit a mes tròp de temps per respondre. Verificatz que lo sit es en linha, esperatz un pauc e ensajatz tornarmai. Tanben podètz ensajar a una ora d'afluéncia mendra.",

'license'            => 'Licéncia&nbsp;:',
'nolicense'          => 'Cap de licéncia seleccionada',
'license-nopreview'  => '(Previsualizacion impossibla)',
'upload_source_url'  => ' (una URL valida e accessibla publicament)',
'upload_source_file' => ' (un fichièr sus vòstre ordenador)',

# Special:ImageList
'imagelist-summary'     => 'Aquesta pagina especiala mòstra totes los fichièrs importats.
Per defaut, las darrièrs fichièrs importats son afichats en naut de la lista.
Un clic en tèsta de colomna càmbia l’òrdre d’afichatge.',
'imagelist_search_for'  => 'Recèrca del mèdia nomenat :',
'imgfile'               => 'fichièr',
'imagelist'             => 'Lista dels imatges',
'imagelist_date'        => 'Data',
'imagelist_name'        => 'Nom',
'imagelist_user'        => 'Utilizaire',
'imagelist_size'        => 'Talha (en octets)',
'imagelist_description' => 'Descripcion',

# Image description page
'filehist'                       => 'Istoric del fichièr',
'filehist-help'                  => 'Clicar sus una data e una ora per veire lo fichièr tal coma èra a aqueste moment',
'filehist-deleteall'             => 'suprimir tot',
'filehist-deleteone'             => 'suprimir',
'filehist-revert'                => 'revocar',
'filehist-current'               => 'actual',
'filehist-datetime'              => 'Data e ora',
'filehist-user'                  => 'Utilizaire',
'filehist-dimensions'            => 'Dimensions',
'filehist-filesize'              => 'Talha del fichièr',
'filehist-comment'               => 'Comentari',
'imagelinks'                     => "Paginas que contenon l'imatge",
'linkstoimage'                   => '{{PLURAL:$1|La pagina çaijós compòrta|Las paginas çaijós compòrtan}} aqueste imatge :',
'nolinkstoimage'                 => 'Cap de pagina compòrta pas de ligam vèrs aqueste imatge.',
'morelinkstoimage'               => 'Vejatz [[Special:WhatLinksHere/$1|mai de ligams]] vèrs aqueste imatge.',
'redirectstofile'                => '{{PLURAL:$1|Lo fichièr seguent redirigís|Los fichièrs seguents redirigisson}} cap a aqueste fichièr :',
'duplicatesoffile'               => "{{PLURAL:$1|Lo fichièr seguent es un duplicata|Los fichièrs seguents son de duplicatas}} d'aqueste :",
'sharedupload'                   => 'Aqueste fichièr es partejat e pòt èsser utilizat per d’autres projèctes.',
'shareduploadwiki'               => 'Reportatz-vos a la $1 per mai d’informacion.',
'shareduploadwiki-desc'          => 'La descripcion de sa $1 dins lo repertòri partejat es afichada çaijós.',
'shareduploadwiki-linktext'      => 'pagina de descripcion del fichièr',
'shareduploadduplicate'          => "Aqueste fichièr es un doblon de $1 d'un depaus partejat.",
'shareduploadduplicate-linktext' => 'un autre fichièr',
'shareduploadconflict'           => "Aqueste fichièr a lo meteis nom que $1 qu'es dins un depaus partejat.",
'shareduploadconflict-linktext'  => 'un autre fichièr',
'noimage'                        => 'Cap de fichièr amb aqueste nom existís pas, mas podètz $1.',
'noimage-linktext'               => "n'importar un",
'uploadnewversion-linktext'      => "Importar una version novèla d'aqueste fichièr",
'imagepage-searchdupe'           => 'Recèrca dels fichièrs en doble',

# File reversion
'filerevert'                => 'Revocar $1',
'filerevert-legend'         => 'Revocar lo fichièr',
'filerevert-intro'          => "Anatz revocar '''[[Media:$1|$1]]''' fins a [$4 la version del $2 a $3].",
'filerevert-comment'        => 'Comentari :',
'filerevert-defaultcomment' => 'Revocat fins a la version del $1 a $2',
'filerevert-submit'         => 'Revocar',
'filerevert-success'        => "'''[[Media:$1|$1]]''' es estat revocat fins a [$4 la version del $2 a $3].",
'filerevert-badversion'     => 'I a pas de version mai anciana del fichièr amb lo Timestamp donat.',

# File deletion
'filedelete'                  => 'Suprimir $1',
'filedelete-legend'           => 'Suprimir lo fichièr',
'filedelete-intro'            => "Sètz a suprimir '''[[Media:$1|$1]]'''.",
'filedelete-intro-old'        => "Sètz a escafar la version de '''[[Media:$1|$1]]''' del [$4 $2 a $3].",
'filedelete-comment'          => "Motiu de l'escafament :",
'filedelete-submit'           => 'Suprimir',
'filedelete-success'          => "'''$1''' es estat suprimit.",
'filedelete-success-old'      => "La version de '''[[Media:$1|$1]]''' del $2 a $3 es estada suprimida.",
'filedelete-nofile'           => "'''$1''' existís pas.",
'filedelete-nofile-old'       => "Existís pas cap de version archivada de '''$1''' amb los atributs indicats.",
'filedelete-iscurrent'        => "Sètz a ensajar de suprimir la version mai recenta d'aqueste fichièr. Vos cal, deperabans, restablir una version anciana d'aqueste.",
'filedelete-otherreason'      => 'Rason diferenta/suplementària :',
'filedelete-reason-otherlist' => 'Autra rason',
'filedelete-reason-dropdown'  => '*Motius de supression costumièrs
** Violacion de drech d’autor
** Fichièr duplicat',
'filedelete-edit-reasonlist'  => 'Modifica los motius de la supression',

# MIME search
'mimesearch'         => 'Recèrca per tipe MIME',
'mimesearch-summary' => 'Aquesta pagina especiala permet de cercar de fichièrs en foncion de lor tipe MIME. Entrada : tipe/sostipe, per exemple <tt>image/jpeg</tt>.',
'mimetype'           => 'Tipe MIME :',
'download'           => 'telecargament',

# Unwatched pages
'unwatchedpages' => 'Paginas pas seguidas',

# List redirects
'listredirects' => 'Lista de las redireccions',

# Unused templates
'unusedtemplates'     => 'Modèls inutilizats',
'unusedtemplatestext' => "Aquesta pagina lista totas las paginas de l’espaci de noms « Modèl » que son pas inclusas dins cap d'autra pagina. Doblidetz pas de verificar se i a pas d’autre ligam cap als modèls abans de los suprimir.",
'unusedtemplateswlh'  => 'autres ligams',

# Random page
'randompage'         => "Una pagina a l'azard",
'randompage-nopages' => 'I a pas cap de pagina dins aqueste espaci de nom.',

# Random redirect
'randomredirect'         => "Una pagina de redireccion a l'azard",
'randomredirect-nopages' => 'I a pas cap de redireccion dins aqueste espaci de nom.',

# Statistics
'statistics'             => 'Estatisticas',
'sitestats'              => 'Estatisticas de {{SITENAME}}',
'userstats'              => "Estatisticas d'utilizaire",
'sitestatstext'          => "La banca de donadas conten actualament <b>{{PLURAL:\$1|'''1''' pagina|'''\$1''' paginas}}</b>.

Aquesta chifra inclutz las paginas \"discussion\", las paginas relativas a {{SITENAME}}, las paginas minimalas (\"esbòsses\"),  las paginas de redireccion, e mai d'autras paginas que pòdon sens dobte pas èsser consideradas coma d'articles.
Se s'exclutz aquestes paginas,  <b>{{PLURAL:\$2|'''\$2''' pagina es probablament un article vertadièr|'''\$2''' paginas son probablament d'articles vertadièrs}}.

{{PLURAL:\$8|'''\$8''' fichièr es estat telecargat|'''\$8''' fichièrs son estats telecargats}}.

{{PLURAL:\$3|'''1''' pagina es estada consultada|'''\$3''' paginas son estadas consultadas}} e {{PLURAL:\$4| '''1''' pagina modificada|'''\$4''' paginas modificadas}}.

Aquò representa una mejana de {{PLURAL:\$5|'''\$5''' modificacion|'''\$5''' modificacions}} per pagina e de {{PLURAL:\$6|'''\$6''' consultacion|'''\$6''' consultacions}} per una modificacion.

I a {{PLURAL:\$7|'''\$7''' article|'''\$7''' articles}} dins [http://www.mediawiki.org/wiki/Manual:Job_queue la fila de prètzfaches].",
'userstatstext'          => "I a {{PLURAL:$1|'''$1''' [[Special:ListUsers|utilizaire enregistrat]]. I a '''$2''' (o '''$4%''') que es|'''$1''' [[Special:ListUsers|utilizaires enregistrats]]. Demest eles, '''$2''' (o '''$4%''') son}} $5.",
'statistics-mostpopular' => 'Paginas mai consultadas',

'disambiguations'      => "Paginas d'omonimia",
'disambiguationspage'  => 'Template:Omonimia',
'disambiguations-text' => "Las paginas seguentas puntan cap a una '''pagina d’omonimia'''.
Deurián puslèu puntar cap a una pagina apropriada.<br />
Una pagina es tractada coma una pagina d’omonimia s'utiliza un modèl qu'es ligat a partir de [[MediaWiki:Disambiguationspage]]",

'doubleredirects'            => 'Redireccions doblas',
'doubleredirectstext'        => 'Cada casa conten de ligams vèrs la primièra e la segonda redireccion, e mai la primièra linha de tèxt de la segonda pagina, costumièrament, aquò provesís la « vertadièra » pagina cibla, vèrs laquala la primièra redireccion deuriá redirigir.',
'double-redirect-fixed-move' => '[[$1]] es estat renomenat, aquò es ara una redireccion cap a [[$2]]',
'double-redirect-fixer'      => 'Corrector de redireccion',

'brokenredirects'        => 'Redireccions copadas',
'brokenredirectstext'    => "Aquestas redireccions mènan a una pagina qu'existís pas.",
'brokenredirects-edit'   => '(modificar)',
'brokenredirects-delete' => '(suprimir)',

'withoutinterwiki'         => 'Paginas sens ligams interlengas',
'withoutinterwiki-summary' => "Las paginas seguentas an pas de ligams cap a las versions dins d'autras lengas.",
'withoutinterwiki-legend'  => 'Prefix',
'withoutinterwiki-submit'  => 'Afichar',

'fewestrevisions' => 'Articles mens modificats',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|octet|octets}}',
'ncategories'             => '$1 {{PLURAL:$1|categoria|categorias}}',
'nlinks'                  => '$1 {{PLURAL:$1|ligam|ligams}}',
'nmembers'                => '$1 {{PLURAL:$1|membre|membres}}',
'nrevisions'              => '$1 {{PLURAL:$1|revision|revisions}}',
'nviews'                  => '$1 {{PLURAL:$1|consultacion|consultacions}}',
'specialpage-empty'       => 'Aquesta pagina es voida.',
'lonelypages'             => 'Paginas orfanèlas',
'lonelypagestext'         => 'Las paginas seguentas son pas ligadas a partir d’autras paginas de {{SITENAME}}.',
'uncategorizedpages'      => 'Paginas sens categorias',
'uncategorizedcategories' => 'Categorias sens categorias',
'uncategorizedimages'     => 'Imatges sens categorias',
'uncategorizedtemplates'  => 'Modèls sens categoria',
'unusedcategories'        => 'Categorias inutilizadas',
'unusedimages'            => 'Imatges orfanèls',
'popularpages'            => 'Paginas mai consultadas',
'wantedcategories'        => 'Categorias mai demandadas',
'wantedpages'             => 'Paginas mai demandadas',
'missingfiles'            => 'Fichièrs mancants',
'mostlinked'              => 'Paginas mai ligadas',
'mostlinkedcategories'    => 'Categorias mai utilizadas',
'mostlinkedtemplates'     => 'Modèls mai utilizats',
'mostcategories'          => 'Articles utilizant mai de categorias',
'mostimages'              => 'Fichièrs mai utilizats',
'mostrevisions'           => 'Articles mai modificats',
'prefixindex'             => 'Totas las paginas per primièras letras',
'shortpages'              => 'Paginas brèvas',
'longpages'               => 'Paginas longas',
'deadendpages'            => "Paginas sul camin d'enlòc",
'deadendpagestext'        => 'Las paginas seguentas contenon pas cap de ligam cap a d’autras paginas de {{SITENAME}}.',
'protectedpages'          => 'Paginas protegidas',
'protectedpages-indef'    => 'Unicament las proteccions permanentas',
'protectedpagestext'      => 'Las paginas seguentas son protegidas contra las modificacions e/o lo cambiament de nom :',
'protectedpagesempty'     => 'Cap de pagina es pas protegida actualament.',
'protectedtitles'         => 'Títols protegits',
'protectedtitlestext'     => 'Los títols seguents son protegits a la creacion',
'protectedtitlesempty'    => 'Cap de títol es pas actualament protegit amb aquestes paramètres.',
'listusers'               => 'Lista dels participants',
'newpages'                => 'Paginas novèlas',
'newpages-username'       => "Nom d'utilizaire :",
'ancientpages'            => 'Articles mai ancians',
'move'                    => 'Tornar nomenar',
'movethispage'            => 'Tornar nomenar la pagina',
'unusedimagestext'        => "Doblidetz pas que d'autres sits pòdon conténer un ligam dirèct vèrs aqueste imatge, e qu'aqueste pòt èsser plaçat dins aquesta lista alara qu'es en realitat utilizada.",
'unusedcategoriestext'    => "Las categorias seguentas existisson mas cap d'article o de categoria los utilizan pas.",
'notargettitle'           => 'Pas de cibla',
'notargettext'            => 'Indicatz una pagina cibla o un utilizaire cibla.',
'nopagetitle'             => 'Cap de pagina cibla',
'nopagetext'              => "La pagina cibla qu'avètz indicada existís pas.",
'pager-newer-n'           => '{{PLURAL:$1|1 mai recenta|$1 mai recentas}}',
'pager-older-n'           => '{{PLURAL:$1|1 mai anciana|$1 mai ancianas}}',
'suppress'                => 'Supervisor',

# Book sources
'booksources'               => 'Obratges de referéncia',
'booksources-search-legend' => "Recercar demest d'obratges de referéncia",
'booksources-isbn'          => 'ISBN :',
'booksources-go'            => 'Validar',
'booksources-text'          => "Vaquí una lista de ligams cap a d’autres sits que vendon de libres nous e d’occasion e sulsquals trobarètz benlèu d'entresenhas suls obratges que cercatz. {{SITENAME}} es pas ligada a cap d'aquestas societats, a pas l’intencion de ne far la promocion.",

# Special:Log
'specialloguserlabel'  => 'Utilizaire :',
'speciallogtitlelabel' => 'Títol :',
'log'                  => 'Jornals',
'all-logs-page'        => 'Totes los jornals',
'log-search-legend'    => "Recèrca d'istorics",
'log-search-submit'    => 'Anar',
'alllogstext'          => 'Afichatge combinat de totes los jornals de {{SITENAME}}.
Podètz restrénher la vista en seleccionant un tipe de jornal, un nom d’utilizaire (cassa sensibla) o una pagina ciblada (idem).',
'logempty'             => 'I a pas res dins l’istoric per aquesta pagina.',
'log-title-wildcard'   => 'Recercar de títols que començan per aqueste tèxt',

# Special:AllPages
'allpages'          => 'Totas las paginas',
'alphaindexline'    => '$1 a $2',
'nextpage'          => 'Pagina seguenta ($1)',
'prevpage'          => 'Pagina precedenta ($1)',
'allpagesfrom'      => 'Afichar las paginas a partir de :',
'allarticles'       => 'Totas las paginas',
'allinnamespace'    => 'Totas las paginas (espaci de noms $1)',
'allnotinnamespace' => 'Totas las paginas (que son pas dins l’espaci de noms $1)',
'allpagesprev'      => 'Precedent',
'allpagesnext'      => 'Seguent',
'allpagessubmit'    => 'Validar',
'allpagesprefix'    => 'Afichar las paginas que començan pel prefix :',
'allpagesbadtitle'  => 'Lo títol rensenhat per la pagina es incorrècte o possedís un prefix reservat. Conten segurament un o mantun caractèr especial que pòt pas èsser utilizats dins los títols.',
'allpages-bad-ns'   => '{{SITENAME}} a pas d’espaci de noms « $1 ».',

# Special:Categories
'categories'                    => 'Categorias',
'categoriespagetext'            => 'Las categorias seguentas contenon de paginas o de fichièrs.
[[Special:UnusedCategories|Las categorias inutilizadas]] son pas afichadas aicí.
Vejatz tanben [[Special:WantedCategories|las categorias demandadas]].',
'categoriesfrom'                => 'Afichar las categorias que començan a :',
'special-categories-sort-count' => 'triada per compte',
'special-categories-sort-abc'   => 'triada alfabetica',

# Special:ListUsers
'listusersfrom'      => 'Afichar los utilizaires a partir de :',
'listusers-submit'   => 'Mostrar',
'listusers-noresult' => "S'es pas trobat de noms d'utilizaires correspondents. Cercatz tanben amb de majusculas e minusculas.",

# Special:ListGroupRights
'listgrouprights'          => "Dreches dels gropes d'utilizaires",
'listgrouprights-summary'  => "Aquesta pagina conten una tièra de gropes definits sus aqueste wiki e mai los dreches d'accès qu'i son associats.
I pòt aver [[{{MediaWiki:Listgrouprights-helppage}}|d'entresenhas complementàrias]] a prepaus dels dreches.",
'listgrouprights-group'    => 'Grop',
'listgrouprights-rights'   => 'Dreches associats',
'listgrouprights-helppage' => 'Help:Dreches dels gropes',
'listgrouprights-members'  => '(lista dels membres)',

# E-mail user
'mailnologin'     => "Pas d'adreça",
'mailnologintext' => 'Vos cal èsser [[Special:UserLogin|connectat(ada)]]
e aver indicat una adreça electronica valida dins vòstras [[Special:Preferences|preferéncias]]
per poder mandar un messatge a un autre utilizaire.',
'emailuser'       => 'Mandar un messatge a aqueste utilizaire',
'emailpage'       => 'Mandar un corrièr electronic a l’utilizaire',
'emailpagetext'   => "S'aqueste utilizaire a indicat una adreça electronica valida dins sas preferéncias, lo formulari çaijós li mandarà un messatge.
L'adreça electronica qu'avètz indicada dins [[Special:Preferences|vòstras preferéncias]] apareisserà dins lo camp « Expeditor » de vòstre messatge. E mai, lo destinatari vos poirà respondre dirèctament.",
'usermailererror' => 'Error dins lo subjècte del corrièr electronic :',
'defemailsubject' => 'Corrièr electronic mandat dempuèi {{SITENAME}}',
'noemailtitle'    => "Pas d'adreça electronica",
'noemailtext'     => "Aquesta utilizaire a pas especificat d'adreça electronica valida o a causit de recebre pas de corrièr electronic dels autres utilizaires.",
'emailfrom'       => 'Expeditor :',
'emailto'         => 'Destinatari :',
'emailsubject'    => 'Subjècte :',
'emailmessage'    => 'Messatge :',
'emailsend'       => 'Mandar',
'emailccme'       => 'Me mandar per corrièr electronic una còpia de mon messatge.',
'emailccsubject'  => 'Còpia de vòstre messatge a $1 : $2',
'emailsent'       => 'Messatge mandat',
'emailsenttext'   => 'Vòstre messatge es estat mandat.',
'emailuserfooter' => 'Aqueste corrièr electronic es estat mandat per « $1 » a « $2 » per la foncion « Mandar un corrièr electronic a l’utilizaire » sus {{SITENAME}}.',

# Watchlist
'watchlist'            => 'Lista de seguit',
'mywatchlist'          => 'Lista de seguit',
'watchlistfor'         => "(per l’utilizaire '''$1''')",
'nowatchlist'          => "Vòstra lista de seguit conten pas cap d'article.",
'watchlistanontext'    => 'Per poder afichar o editar los elements de vòstra lista de seguit, vos devètz $1.',
'watchnologin'         => 'Vos sètz pas identificat(ada)',
'watchnologintext'     => 'Vos cal èsser [[Special:UserLogin|connectat(ada)]]
per modificar vòstra lista de seguit.',
'addedwatch'           => 'Apondut a la tièra',
'addedwatchtext'       => 'La pagina "[[:$1]]" es estada aponduda a vòstra [[Special:Watchlist|lista de seguit]].
Las modificacions venentas d\'aquesta pagina e de la pagina de discussion associada seràn repertoriadas aicí, e la pagina apareisserà <b>en gras</b> dins la [[Special:RecentChanges|tièra dels darrièrs cambiaments]] per èsser localizada mai aisidament.',
'removedwatch'         => 'Suprimida de la lista de seguit',
'removedwatchtext'     => 'La pagina "[[:$1]]" es estada suprimida de vòstra lista de seguit.',
'watch'                => 'Seguir',
'watchthispage'        => 'Seguir aquesta pagina',
'unwatch'              => 'Arrestar de seguir',
'unwatchthispage'      => 'Arrestar de seguir',
'notanarticle'         => "Pas cap d'article",
'notvisiblerev'        => 'Version suprimida',
'watchnochange'        => 'Cap de las paginas que seguissètz son pas estadas modificadas pendent lo periòde afichat.',
'watchlist-details'    => 'I a {{PLURAL:$1|pagina|paginas}} dins vòstra lista de seguit, sens comptar las paginas de discussion.',
'wlheader-enotif'      => '* La notificacion per corrièr electronic es activada.',
'wlheader-showupdated' => '* Las paginas que son estadas modificadas dempuèi vòstra darrièra visita son mostradas en <b>gras</b>',
'watchmethod-recent'   => 'verificacion dels darrièrs cambiaments de las paginas seguidas',
'watchmethod-list'     => 'verificacion de las paginas seguidas per de modificacions recentas',
'watchlistcontains'    => 'Vòstra lista de seguit conten $1 {{PLURAL:$1|pagina|paginas}}.',
'iteminvalidname'      => "Problèma amb l'article « $1 » : lo nom es invalid...",
'wlnote'               => 'Çaijós se {{PLURAL:$1|tròba la darrièra modificacion|tròban las $1 darrièras modificacions}} dempuèi {{PLURAL:$2|la darrièra ora|las <b>$2</b> darrièras oras}}.',
'wlshowlast'           => 'Mostrar las darrièras $1 oras, los darrièrs $2 jorns, o $3.',
'watchlist-show-bots'  => 'Afichar las contribucions dels bòts',
'watchlist-hide-bots'  => 'Amagar las contribucions dels bòts',
'watchlist-show-own'   => 'Afichar mas modificacions',
'watchlist-hide-own'   => 'Amagar mas modificacions',
'watchlist-show-minor' => 'Afichar las modificacions menoras',
'watchlist-hide-minor' => 'Amagar las modificacions menoras',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Seguit...',
'unwatching' => 'Fin del seguit...',

'enotif_mailer'                => 'Sistèma d’expedicion de notificacion de {{SITENAME}}',
'enotif_reset'                 => 'Marcar totas las paginas coma visitadas',
'enotif_newpagetext'           => 'Aquò es una pagina novèla.',
'enotif_impersonal_salutation' => 'Utilizaire de {{SITENAME}}',
'changed'                      => 'modificada',
'created'                      => 'creada',
'enotif_subject'               => 'La pagina $PAGETITLE de {{SITENAME}} es estada $CHANGEDORCREATED per $PAGEEDITOR',
'enotif_lastvisited'           => 'Consultatz $1 per totes los cambiaments dempuèi vòstra darrièra visita.',
'enotif_lastdiff'              => 'Consultatz $1 per veire aquesta modificacion.',
'enotif_anon_editor'           => 'utilizaire anonim $1',
'enotif_body'                  => 'Car $WATCHINGUSERNAME,

la pagina de {{SITENAME}} $PAGETITLE es estada $CHANGEDORCREATED lo $PAGEEDITDATE per $PAGEEDITOR, vejatz $PAGETITLE_URL per la version actuala.

$NEWPAGE

Resumit de l’editor : $PAGESUMMARY $PAGEMINOREDIT

Contactatz l’editor :
corrièr electronic : $PAGEEDITOR_EMAIL
wiki : $PAGEEDITOR_WIKI

I aurà pas de notificacions novèlas en cas d’autras modificacions a mens que visitetz aquesta pagina. Podètz tanben remetre a zèro lo notificator per totas las paginas de vòstra lista de seguit.

             Vòstre {{SITENAME}} sistèma de notificacion

--
Per modificar los paramètres de vòstra lista de seguit, visitatz
{{fullurl:Special:Watchlist/edit}}

Retorn e assisténcia :
{{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => 'Suprimir la pagina',
'confirm'                     => 'Confirmar',
'excontent'                   => "contenent '$1'",
'excontentauthor'             => "lo contengut èra : « $1 » (e l'unic contributor èra « [[Special:Contributions/$2|$2]] »)",
'exbeforeblank'               => "lo contengut abans blanquiment èra :'$1'",
'exblank'                     => 'pagina voida',
'delete-confirm'              => 'Escafar «$1»',
'delete-legend'               => 'Escafar',
'historywarning'              => 'Atencion : La pagina que sètz a mand de suprimir a un istoric :',
'confirmdeletetext'           => "Sètz a mand de suprimir definitivament de la banca de donadas una pagina
o un imatge, e mai totas sas versions anterioras.
Confirmatz qu'es plan çò que volètz far, que ne comprenètz las consequéncias e que fasètz aquò en acòrdi amb las [[{{MediaWiki:Policy-url|règlas intèrnas}}]].",
'actioncomplete'              => 'Accion efectuada',
'deletedtext'                 => '"<nowiki>$1</nowiki>" es estat suprimit.
Vejatz $2 per una lista de las supressions recentas.',
'deletedarticle'              => 'a escafat «[[$1]]»',
'suppressedarticle'           => 'amagat  « [[$1]] »',
'dellogpage'                  => 'Istoric dels escafaments',
'dellogpagetext'              => 'Vaquí çaijós la lista de las supressions recentas.',
'deletionlog'                 => 'istoric dels escafaments',
'reverted'                    => 'Restabliment de la version precedenta',
'deletecomment'               => 'Motiu de la supression :',
'deleteotherreason'           => 'Motius suplementaris o autres :',
'deletereasonotherlist'       => 'Autre motiu',
'deletereason-dropdown'       => "*Motius de supression mai corrents
** Demanda de l'autor
** Violacion dels dreches d'autor
** Vandalisme",
'delete-edit-reasonlist'      => 'Modifica los motius de la supression',
'delete-toobig'               => "Aquesta pagina dispausa d'un istoric important, depassant {{PLURAL:$1|revision|revisions}}.
La supression de talas paginas es estada limitada per evitar de perturbacions accidentalas de {{SITENAME}}.",
'delete-warning-toobig'       => "Aquesta pagina dispausa d'un istoric important, depassant {{PLURAL:$1|revision|revisions}}.
La suprimir pòt perturbar lo foncionament de la banca de donada de {{SITENAME}}.
D'efectuar amb prudéncia.",
'rollback'                    => 'Anullar las modificacions',
'rollback_short'              => 'Anullar',
'rollbacklink'                => 'anullar',
'rollbackfailed'              => "L'anullacion a pas capitat",
'cantrollback'                => "Impossible d'anullar : l'autor es la sola persona a aver efectuat de modificacions sus aqueste article",
'alreadyrolled'               => "Impossible d'anullar la darrièra modificacion de l'article « [[$1]] » efectuada per [[User:$2|$2]] ([[User talk:$2|Discussion]]) ; qualqu'un d'autre ja a modificat o revocat l'article.

La darrièra modificacion es estada efectuada per [[User:$3|$3]] ([[User talk:$3|Discussion]]).",
'editcomment'                 => 'Lo resumit de la modificacion èra: "<i>$1</i>".', # only shown if there is an edit comment
'revertpage'                  => 'Anullacion de las modificacions de [[Special:Contributions/$2|$2]] ([[User talk:$2|Discussion]]) vèrs la darrièra version de [[User:$1|$1]]', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => 'Anullacion de las modificacions de $1 ; retorn a la version de $2.',
'sessionfailure'              => 'Vòstra sesilha de connexion sembla aver de problèmas ;
aquesta accion es estada anullada en prevencion d’un piratatge de sesilha.
Clicatz sus « Precedent » e tornatz cargar la pagina d’ont venètz, puèi ensajatz tornarmai.',
'protectlogpage'              => 'Istoric de las proteccions',
'protectlogtext'              => 'Vejatz las [[Special:ProtectedPages|directivas]] per mai d’informacion.',
'protectedarticle'            => 'a protegit « [[$1]] »',
'modifiedarticleprotection'   => 'a modificat lo nivèl de proteccion de « [[$1]] »',
'unprotectedarticle'          => 'a desprotegit « [[$1]] »',
'protect-title'               => 'Cambiar lo nivèl de proteccion de « $1 »',
'protect-legend'              => 'Confirmar la proteccion',
'protectcomment'              => 'Motiu de la proteccion :',
'protectexpiry'               => 'Expiracion (expira pas per defaut)',
'protect_expiry_invalid'      => 'Lo temps d’expiracion es invalid',
'protect_expiry_old'          => 'Lo temps d’expiracion ja es passat.',
'protect-unchain'             => 'Desblocar las permissions de cambiament de nom',
'protect-text'                => 'Podètz consultar e modificar lo nivèl de proteccion de la pagina <strong><nowiki>$1</nowiki></strong>. Asseguratz-vos que seguissètz las règlas intèrnas.',
'protect-locked-blocked'      => 'Podètz pas modificar lo nivèl de proteccion tant que sètz blocat. Vaquí los reglatges actuals de la pagina <strong>$1</strong> :',
'protect-locked-dblock'       => 'Lo nivèl de proteccion pòt pas èsser modificat perque la banca de donadas es blocada. Vaquí los reglatges actuals de la pagina <strong>$1</strong> :',
'protect-locked-access'       => 'Avètz pas los dreches necessaris per modificar la proteccion de la pagina. Vaquí los reglatges actuals de la pagina <strong>$1</strong> :',
'protect-cascadeon'           => "Aquesta pagina es actualament protegida perque es inclusa dins {{PLURAL:$1|la pagina seguenta|las paginas seguentas}}, {{PLURAL:$1|qu'es estada protegida|que son estadas protegidas}} amb l’opcion « proteccion en cascada » activada. Podètz cambiar lo nivèl de proteccion d'aquesta pagina sens qu'aquò afècte la proteccion en cascada.",
'protect-default'             => 'Pas de proteccion',
'protect-fallback'            => 'Necessita l’abilitacion «$1»',
'protect-level-autoconfirmed' => 'Semiproteccion',
'protect-level-sysop'         => 'Administrators unicament',
'protect-summary-cascade'     => 'proteccion en cascada',
'protect-expiring'            => 'expira lo $1',
'protect-cascade'             => 'Proteccion en cascada - Protegís totas las paginas inclusas dins aquesta.',
'protect-cantedit'            => "Podètz pas modificar los nivèls de proteccion d'aquesta pagina perque avètz pas la permission de l'editar.",
'restriction-type'            => 'Permission :',
'restriction-level'           => 'Nivèl de restriccion :',
'minimum-size'                => 'Talha minimoma',
'maximum-size'                => 'Talha maximala :',
'pagesize'                    => '(octets)',

# Restrictions (nouns)
'restriction-edit'   => 'Modificacion',
'restriction-move'   => 'Cambiament de nom',
'restriction-create' => 'Crear',
'restriction-upload' => 'Importar',

# Restriction levels
'restriction-level-sysop'         => 'Proteccion complèta',
'restriction-level-autoconfirmed' => 'Semiproteccion',
'restriction-level-all'           => 'Totes',

# Undelete
'undelete'                     => 'Veire las paginas escafadas',
'undeletepage'                 => 'Veire e restablir la pagina escafada',
'undeletepagetitle'            => "'''La lista seguenta se compausa de versions suprimidas de [[:$1]]'''.",
'viewdeletedpage'              => 'Istoric de la pagina suprimida',
'undeletepagetext'             => "Aquestas paginas son estadas escafadas e se tròban dins l'archiu. Figuran totjorn dins la banca de donada e pòdon èsser restablidas.
L'archiu pòt èsser escafat periodicament.",
'undelete-fieldset-title'      => 'Restablir las versions',
'undeleteextrahelp'            => "Per restablir l'istoric complet d'aquesta pagina, daissatz vèrjas totas las casas de marcar, puèi clicatz sus '''''Restablir'''''.
Per restablir pas que d'unas versions, marcatz las casas que correspondon a las versions que son de restablir, puèi clicatz sus '''''Restablir'''''.
En clicant sul boton '''''Reïnicializar''''', la bóstia de resumit e las casas marcadas seràn remesas a zèro.",
'undeleterevisions'            => '$1 {{PLURAL:$1|revision archivada|revisions archivadas}}',
'undeletehistory'              => "Se restablissètz la pagina, totas las revisions seràn plaçadas tornamai dins l'istoric.

S'una pagina novèla amb lo meteis nom es estada creada dempuèi la supression, las revisions restablidas apareisseràn dins l'istoric anterior e la version correnta serà pas automaticament remplaçada.",
'undeleterevdel'               => 'Lo restabliment serà pas efectuat se, fin finala, la version mai recenta de la pagina es parcialament suprimida. Dins aqueste cas, vos cal deseleccionatz las versions mai recentas (en naut). Las versions dels fichièrs a lasqualas avètz pas accès seràn pas restablidas.',
'undeletehistorynoadmin'       => "Aqueste article es estat suprimit. Lo motiu de la supression es indicat dins lo resumit çaijós, amb los detalhs dels utilizaires que l’an modificat abans sa supression. Lo contengut d'aquestas versions es pas accessible qu’als administrators.",
'undelete-revision'            => 'Version suprimida de $1, (revision del $2) per $3 :',
'undeleterevision-missing'     => 'Version invalida o mancanta. Benlèu avètz un ligam marrit, o la version es estada restablida o suprimida de l’archiu.',
'undelete-nodiff'              => 'Cap de revision precedenta pas trobada.',
'undeletebtn'                  => 'Restablir',
'undeletelink'                 => 'restablir',
'undeletereset'                => 'Reïnicializar',
'undeletecomment'              => 'Comentari :',
'undeletedarticle'             => 'a restablit « [[$1]] »',
'undeletedrevisions'           => '{{PLURAL:$1|1 revision restablida|$1 revisions restablidas}}',
'undeletedrevisions-files'     => '{{PLURAL:$1|1 revision|$1 revisions}} e {{PLURAL:$2|1 fichièr restablit|$2 fichièrs restablits}}',
'undeletedfiles'               => '$1 {{PLURAL:$1|fichièr restablit|fichièrs restablits}}',
'cannotundelete'               => 'Lo restabliment a pas capitat. Un autre utilizaire a probablament restablit la pagina abans.',
'undeletedpage'                => "<big>'''La pagina $1 es estada restablida'''.</big>

Consultatz l’[[Special:Log/delete|istoric de las supressions]] per veire las paginas recentament suprimidas e restablidas.",
'undelete-header'              => 'Consultatz l’[[Special:Log/delete|istoric de las supressions]] per veire las paginas recentament suprimidas.',
'undelete-search-box'          => 'Cercar una pagina suprimida',
'undelete-search-prefix'       => 'Mostrar las paginas que començan per :',
'undelete-search-submit'       => 'Cercar',
'undelete-no-results'          => 'Cap de pagina correspondent a la recèrca es pas estada trobada dins los archius.',
'undelete-filename-mismatch'   => 'Impossible de restablir lo fichièr datat del $1 : fichièr introbable',
'undelete-bad-store-key'       => 'Impossible de restablir lo fichièr datat del $1 : lo fichièr èra absent abans la supression.',
'undelete-cleanup-error'       => 'Error al moment de la supression de l’archiu inutilizada « $1 ».',
'undelete-missing-filearchive' => 'Impossible de restablir lo fichièr amb l’ID $1 perque es pas dins la banca de donadas. Benlèu ja i es estat restablit.',
'undelete-error-short'         => 'Error al moment del restabliment del fichièr : $1',
'undelete-error-long'          => "D'errors son estadas rencontradas al moment del restabliment del fichièr :

$1",

# Namespace form on various pages
'namespace'      => 'Espaci de noms :',
'invert'         => 'Inversar la seleccion',
'blanknamespace' => '(Principal)',

# Contributions
'contributions' => "Contribucions d'aqueste contributor",
'mycontris'     => 'Mas contribucions',
'contribsub2'   => 'Lista de las contribucions de $1 ($2). Las paginas que son estadas escafadas son pas afichadas.',
'nocontribs'    => 'Cap de modificacion correspondenta a aquestes critèris es pas estada trobada.',
'uctop'         => '(darrièra)',
'month'         => 'A partir del mes (e precedents) :',
'year'          => 'A partir de l’annada (e precedentas) :',

'sp-contributions-newbies'     => 'Mostrar pas que las contribucions dels utilizaires novèls',
'sp-contributions-newbies-sub' => 'Lista de las contribucions dels utilizaires novèls. Las paginas que son estadas suprimidas son pas afichadas.',
'sp-contributions-blocklog'    => 'Istoric dels blocatges',
'sp-contributions-search'      => 'Cercar las contribucions',
'sp-contributions-username'    => 'Adreça IP o nom d’utilizaire :',
'sp-contributions-submit'      => 'Cercar',

# What links here
'whatlinkshere'            => 'Paginas ligadas a aquesta',
'whatlinkshere-title'      => 'Paginas que puntan cap a « $1 »',
'whatlinkshere-page'       => 'Pagina :',
'whatlinkshere-barrow'     => '>',
'linklistsub'              => '(Lista de ligams)',
'linkshere'                => "Las paginas çaijós contenon un ligam vèrs '''[[:$1]]''':",
'nolinkshere'              => "Cap de pagina conten pas de ligam vèrs '''[[:$1]]'''.",
'nolinkshere-ns'           => "Cap de pagina conten pas de ligam cap a '''[[:$1]]''' dins l’espaci de nom causit.",
'isredirect'               => 'pagina de redireccion',
'istemplate'               => 'inclusion',
'isimage'                  => 'ligam del fichièr',
'whatlinkshere-prev'       => '{{PLURAL:$1|precedent|$1 precedents}}',
'whatlinkshere-next'       => '{{PLURAL:$1|seguent|$1 seguents}}',
'whatlinkshere-links'      => '← ligams',
'whatlinkshere-hideredirs' => '$1 redireccions',
'whatlinkshere-hidetrans'  => '$1 transclusions',
'whatlinkshere-hidelinks'  => '$1 ligams',
'whatlinkshere-hideimages' => '$1 ligams de fichièrs',
'whatlinkshere-filters'    => 'Filtres',

# Block/unblock
'blockip'                         => 'Blocar en escritura',
'blockip-legend'                  => 'Blocar en escritura',
'blockiptext'                     => "Utilizatz lo formulari çaijós per blocar l'accès en escritura a partir d'una adreça IP donada.
Una tala mesura deu pas èsser presa pas que per empachar lo vandalisme e en acòrdi amb las [[{{MediaWiki:Policy-url|règlas intèrnas}}]].
Donatz çaijós una rason precisa (per exemple en indicant las paginas que son estadas vandalizadas).",
'ipaddress'                       => 'Adreça IP :',
'ipadressorusername'              => 'Adreça IP o nom d’utilizaire :',
'ipbexpiry'                       => 'Durada del blocatge :',
'ipbreason'                       => 'Motiu :',
'ipbreasonotherlist'              => 'Autre motiu',
'ipbreason-dropdown'              => '* Motius de blocatge mai frequents
** Vandalisme
** Insercion d’informacions faussas
** Supression de contengut sens justificacion
** Insercion repetida de ligams extèrnes publicitaris (spam)
** Insercion de contengut sens cap de sens
** Temptativa d’intimidacion o agarriment
** Abús d’utilizacion de comptes multiples
** Nom d’utilizaire inacceptable, injuriós o difamant',
'ipbanononly'                     => 'Blocar unicament los utilizaires anonims',
'ipbcreateaccount'                => 'Empachar la creacion de compte',
'ipbemailban'                     => 'Empachar l’utilizaire de mandar de corrièrs electronics',
'ipbenableautoblock'              => 'Blocar automaticament las adreças IP utilizadas per aqueste utilizaire',
'ipbsubmit'                       => 'Blocar aqueste utilizaire',
'ipbother'                        => 'Autra durada',
'ipboptions'                      => '2 oras:2 hours,1 jorn:1 day,3 jorns:3 days,1 setmana:1 week,2 setmanas:2 weeks,1 mes:1 month,3 meses:3 months,6 meses:6 months,1 an:1 year,indefinidament:infinite', # display1:time1,display2:time2,...
'ipbotheroption'                  => 'autre',
'ipbotherreason'                  => 'Motiu diferent o suplementari',
'ipbhidename'                     => "Amagar lo nom d’utilizaire de l'istoric de blocatge, de la lista dels blocatges actius e de la lista dels utilizaires",
'ipbwatchuser'                    => "Seguir las paginas d'utilizaire e de discussion d'aqueste utilizaire",
'badipaddress'                    => "L'adreça IP es incorrècta",
'blockipsuccesssub'               => 'Blocatge capitat',
'blockipsuccesstext'              => '[[Special:Contributions/$1|$1]] es estat blocat.<br />
Podètz consultar la [[Special:IPBlockList|lista dels comptes e de las adreças IP blocats]].',
'ipb-edit-dropdown'               => 'Modificar los motius de blocatge per defaut',
'ipb-unblock-addr'                => 'Desblocar $1',
'ipb-unblock'                     => "Desblocar un compte d'utilizaire o una adreça IP",
'ipb-blocklist-addr'              => 'Vejatz los blocatges existents per $1',
'ipb-blocklist'                   => 'Vejatz los blocatges existents',
'unblockip'                       => 'Desblocar un utilizaire o una adreça IP',
'unblockiptext'                   => "Utilizatz lo formulari çaijós per restablir l'accès en escritura
a partir d'una adreça IP precedentament blocada.",
'ipusubmit'                       => 'Desblocar aquesta adreça',
'unblocked'                       => '[[User:$1|$1]] es estat desblocat',
'unblocked-id'                    => 'Lo blocatge $1 es estat levat',
'ipblocklist'                     => 'Adreças IP e dels utilizaires blocats',
'ipblocklist-legend'              => 'Cercar un utilizaire blocat',
'ipblocklist-username'            => 'Nom de l’utilizaire o adreça IP :',
'ipblocklist-submit'              => 'Recercar',
'blocklistline'                   => '$1, $2 a blocat $3 ($4)',
'infiniteblock'                   => 'permanent',
'expiringblock'                   => 'expira lo $1',
'anononlyblock'                   => 'utilizaire anonim unicament',
'noautoblockblock'                => 'blocatge automatic desactivat',
'createaccountblock'              => 'La creacion de compte es blocada.',
'emailblock'                      => 'mandadís de corrièr electronic blocat',
'ipblocklist-empty'               => 'La lista dels blocatges es voida.',
'ipblocklist-no-results'          => 'L’adreça IP o l’utilizaire es pas esta blocat.',
'blocklink'                       => 'blocar',
'unblocklink'                     => 'desblocar',
'contribslink'                    => 'contribucions',
'autoblocker'                     => 'Sètz estat autoblocat perque partejatz una adreça IP amb "[[User:$1|$1]]".
La rason balhada per $1 es : « $2 ».',
'blocklogpage'                    => 'Istoric dels blocatges',
'blocklogentry'                   => 'a blocat « [[$1]] » - durada : $2 $3',
'blocklogtext'                    => "Aquò es l'istoric dels blocatges e desblocatges dels utilizaires. Las adreças IP automaticament blocadas son pas listadas. Consultatz la [[Special:IPBlockList|lista dels utilizaires blocats]] per veire qui es actualament efectivament blocat.",
'unblocklogentry'                 => 'a desblocat « $1 »',
'block-log-flags-anononly'        => 'utilizaires anonims solament',
'block-log-flags-nocreate'        => 'creacion de compte interdicha',
'block-log-flags-noautoblock'     => 'autoblocatge de las IP desactivat',
'block-log-flags-noemail'         => 'Mandadís de corrièr electronic blocat',
'block-log-flags-angry-autoblock' => 'autoblocatge melhorat en servici',
'range_block_disabled'            => "Lo blocatge de plajas d'IP es estat desactivat.",
'ipb_expiry_invalid'              => 'Temps d’expiracion invalid.',
'ipb_expiry_temp'                 => 'Las plajas dels utilizaires amagats deurián èsser permanentas.',
'ipb_already_blocked'             => '« $1 » ja es blocat',
'ipb_cant_unblock'                => 'Error : Lo blocatge d’ID $1 existís pas. Es possible qu’un desblocatge ja siá estat efectuat.',
'ipb_blocked_as_range'            => "Error : L'adreça IP $1 es pas estada blocada dirèctament e doncas pòt pas èsser deblocada. Çaquelà, es estada blocada per la plaja $2 laquala pòt èsser deblocada.",
'ip_range_invalid'                => 'Plaja IP incorrècta.',
'blockme'                         => 'Blocatz-me',
'proxyblocker'                    => 'Blocaire de mandatari (proxy)',
'proxyblocker-disabled'           => 'Aquesta foncion es desactivada.',
'proxyblockreason'                => "Vòstra ip es estada blocada perque s’agís d’un proxy dobert. Mercé de contactar vòstre fornidor d’accès internet o vòstre supòrt tecnic e de l’informar d'aqueste problèma de seguretat.",
'proxyblocksuccess'               => 'Acabat.',
'sorbsreason'                     => 'Vòstra adreça IP es listada en tant que mandatari (proxy) dobert DNSBL per {{SITENAME}}.',
'sorbs_create_account_reason'     => 'Vòstra adreça IP es listada en tant que mandatari (proxy) dobert DNSBL per {{SITENAME}}.
Podètz pas crear un compte',

# Developer tools
'lockdb'              => 'Varrolhar la banca',
'unlockdb'            => 'Desvarrolhar la banca',
'lockdbtext'          => "Lo clavatge de la banca de donadas empacharà totes los utilizaires de modificar las paginas, de salvar lors preferéncias, de modificar lor lista de seguit e d'efectuar totas las autras operacions necessitant de modificacions dins la banca de donadas.
Confirmatz qu'es plan çò que volètz far e que desblocaretz la banca tre que vòstra operacion de mantenença serà acabada.",
'unlockdbtext'        => "Lo desclavatge de la banca de donadas permetrà a totes los utilizaires de modificar tornarmai de paginas, de metre a jorn lors preferéncias e lor lista de seguit, e mai d'efectuar las autras operacions necessitant de modificacions dins la banca de donadas.
Confirmatz qu'es plan çò que volètz far.",
'lockconfirm'         => 'Òc, confirmi que desiri varrolhar la banca de donadas.',
'unlockconfirm'       => 'Òc, confirmi que desiri desvarrolhar la banca de donadas.',
'lockbtn'             => 'Varrolhar la banca',
'unlockbtn'           => 'Desvarrolhar la banca',
'locknoconfirm'       => 'Avètz pas marcat la casa de confirmacion.',
'lockdbsuccesssub'    => 'Varrolhatge de la banca capitat.',
'unlockdbsuccesssub'  => 'Banca desvarrolhada.',
'lockdbsuccesstext'   => 'La banca de donadas de {{SITENAME}} es varrolhada.

Doblidetz pas de la desvarrolhar quand auretz acabat vòstra operacion de mantenença.',
'unlockdbsuccesstext' => 'La banca de donadas de {{SITENAME}} es desvarrolhada.',
'lockfilenotwritable' => 'Lo fichièr de blocatge de la banca de donadas es pas inscriptible. Per blocar o desblocar la banca de donadas, vos cal poder escriure sul servidor web.',
'databasenotlocked'   => 'La banca de donadas es pas varrolhada.',

# Move page
'move-page'               => 'Tornar nomenar $1',
'move-page-legend'        => 'Tornar nomenar una pagina',
'movepagetext'            => "Utilizatz lo formulari çaijós per tornar nomenar una pagina, en desplaçant tot son istoric cap al nom novèl. Lo títol ancian vendrà una pagina de redireccion cap al títol novèl. Los ligams cap al títol de la pagina anciana seràn pas cambiats ; verificatz qu'aqueste desplaçament a pas creat de [[Special:DoubleRedirects|redireccion dobla]] o de [[Special:BrokenRedirects|redireccion copada]].

Avètz la responsabilitat de vos assegurar que los ligams contunhen de puntar cap a lor destinacion supausada. Una pagina serà pas desplaçada se la pagina del títol novèl existís ja, a mens qu'aquesta darrièra siá voida o en redireccion, e qu’aja pas d’istoric. Aquò vòl dire que podètz tornar nomenar una pagina vèrs sa posicion d’origina s'avètz fach una error, mas que podètz pas escafar una pagina qu'existís ja amb aqueste procediment.

'''ATENCION !''' Aquò pòt provocar un cambiament radical e imprevist per una pagina consultada frequentament. Asseguratz-vos de n'aver comprés las consequéncias abans de contunhar.",
'movepagetalktext'        => "La pagina de discussion associada, se presenta, serà automaticament desplaçada amb '''en defòra de se:'''
*Desplaçatz una pagina vèrs un autre espaci,
*Una pagina de discussion ja existís amb lo nom novèl, o
*Avètz deseleccionat lo boton çaijós.

Dins aqueste cas, deuretz desplaçar o fusionar la pagina manualament se o volètz.",
'movearticle'             => "Tornar nomenar l'article",
'movenotallowed'          => 'Avètz pas la permission de tornar nomenar de paginas.',
'newtitle'                => 'Títol novèl',
'move-watch'              => 'Seguir aquesta pagina',
'movepagebtn'             => "Tornar nomenar l'article",
'pagemovedsub'            => 'Cambiament de nom capitat',
'movepage-moved'          => 'La pagina « $1 » es estada renomenada en « $2 ».', # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => "Existís ja un article que pòrta aqueste títol, o lo títol qu'avètz causit es pas valid.
Causissètz-ne un autre.",
'cantmove-titleprotected' => 'Avètz pas la possibilitat de desplaçar una pagina vèrs aqueste emplaçament perque lo títol es estat protegit a la creacion.',
'talkexists'              => "La pagina ela-meteissa es estada desplaçada amb succès, mas
la pagina de discussion a pas pogut èsser desplaçada perque ja n'existissiá una
jol nom novèl. Se vos plai, fusionatz-las manualament.",
'movedto'                 => 'renomenat en',
'movetalk'                => 'Tornar nomenar tanben la pagina de discussion associada',
'move-subpages'           => 'Tornar nomenar, se fa mestièr, totas las sospaginas',
'move-talk-subpages'      => 'Tornar nomenar, se fa mestièr, totas las sospaginas de las paginas de discussion',
'movepage-page-exists'    => 'La pagina $1 existís ja e pòt pas èsser espotida automaticament.',
'movepage-page-moved'     => 'La pagina $1 es estada renomenada en $2.',
'movepage-page-unmoved'   => 'La pagina $1 pòt èsser renomenada en $2.',
'movepage-max-pages'      => "Lo maximom de $1 {{PLURAL:$1|pagina es estat renomenat|paginas son estadas renomenadas}} e cap d'autra o poirà pas èsser automaticament.",
'1movedto2'               => 'a renomenat [[$1]] en [[$2]]',
'1movedto2_redir'         => 'a redirigit [[$1]] cap a [[$2]]',
'movelogpage'             => 'Istoric dels cambiaments de nom',
'movelogpagetext'         => 'Vaquí la lista de las darrièras paginas renomenadas.',
'movereason'              => 'Motiu :',
'revertmove'              => 'anullar',
'delete_and_move'         => 'Suprimir e tornar nomenar',
'delete_and_move_text'    => '==Supression requesida==
L’article de destinacion « [[:$1]] » existís ja.
Lo volètz suprimir per permetre lo cambiament de nom ?',
'delete_and_move_confirm' => 'Òc, accèpti de suprimir la pagina de destinacion per permetre lo cambiament de nom.',
'delete_and_move_reason'  => 'Pagina suprimida per permetre un cambiament de nom',
'selfmove'                => 'Los títols d’origina e de destinacion son los meteisses : impossible de tornar nomenar una pagina sus ela-meteissa.',
'immobile_namespace'      => 'Lo títol de destinacion es d’un tipe especial ; es impossible de tornar nomenar de paginas cap a aqueste espaci de noms.',
'imagenocrossnamespace'   => 'Pòt pas desplaçar un imatge vèrs un espaci de nomenatge que siá pas un imatge.',
'imagetypemismatch'       => "L'extension novèla d'aqueste fichièr reconeis pas aqueste format.",
'imageinvalidfilename'    => 'Lo nom del fichièr cibla es incorrècte',
'fix-double-redirects'    => 'Metre a jorn las redireccions que puntant cap al títol ancian',

# Export
'export'            => 'Exportar de paginas',
'exporttext'        => "Podètz exportar en XML lo tèxt e l’istoric d’una pagina o d’un ensemble de paginas; lo resultat pòt alara èsser importat dins un autre wiki foncionant amb lo logicial MediaWiki.

Per exportar de paginas, entratz lors títols dins la bóstia de tèxt çaijós, un títol per linha, e seleccionatz s'o desiratz o pas la version actuala amb totas las versions ancianas, amb la pagina d’istoric, o simplament la pagina actuala amb d'informacions sus la darrièra modificacion.

Dins aqueste darrièr cas, podètz tanben utilizar un ligam, coma [[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]] per la pagina [[{{MediaWiki:Mainpage}}]].",
'exportcuronly'     => 'Exportar unicament la version correnta sens l’istoric complet',
'exportnohistory'   => "----
'''Nòta :''' l’exportacion complèta de l’istoric de las paginas amb l’ajuda d'aqueste formulari es estada desactivada per de rasons de performàncias.",
'export-submit'     => 'Exportar',
'export-addcattext' => 'Apondre las paginas de la categoria :',
'export-addcat'     => 'Apondre',
'export-download'   => 'Salvar en tant que fichièr',
'export-templates'  => 'Enclure los modèls',

# Namespace 8 related
'allmessages'               => 'Lista dels messatges del sistèma',
'allmessagesname'           => 'Nom del camp',
'allmessagesdefault'        => 'Messatge per defaut',
'allmessagescurrent'        => 'Messatge actual',
'allmessagestext'           => 'Aquò es la lista de totes los messatges disponibles dins l’espaci MediaWiki.
Visitatz la [http://www.mediawiki.org/wiki/Localisation Localizacion MèdiaWiki] e [http://translatewiki.net Betawiki] se desiratz contribuir a la localizacion MèdiaWiki generica.',
'allmessagesnotsupportedDB' => "'''{{ns:special}}:Allmessages''' es pas disponible perque '''\$wgUseDatabaseMessages''' es desactivat.",
'allmessagesfilter'         => 'Filtre d’expression racionala :',
'allmessagesmodified'       => 'Afichar pas que las modificacions',

# Thumbnails
'thumbnail-more'           => 'Agrandir',
'filemissing'              => 'Fichièr absent',
'thumbnail_error'          => 'Error al moment de la creacion de la miniatura : $1',
'djvu_page_error'          => 'Pagina DjVu fòra limits',
'djvu_no_xml'              => "Impossible d’obténer l'XML pel fichièr DjVu",
'thumbnail_invalid_params' => 'Paramètres de la miniatura invalids',
'thumbnail_dest_directory' => 'Impossible de crear lo repertòri de destinacion',

# Special:Import
'import'                     => 'Importar de paginas',
'importinterwiki'            => 'Impòrt interwiki',
'import-interwiki-text'      => "Seleccionatz un wiki e un títol de pagina d'importar.
Las datas de las versions e los noms dels editors seràn preservats.
Totas las accions d’importacion interwiki son conservadas dins lo [[Special:Log/import|jornal d’impòrt]].",
'import-interwiki-history'   => "Copiar totas las versions de l'istoric d'aquesta pagina",
'import-interwiki-submit'    => 'Importar',
'import-interwiki-namespace' => 'Transferir las paginas dins l’espaci de nom :',
'importtext'                 => 'Exportatz lo fichièr dempuèi lo wiki d’origina en utilizant l’esplech Special:Export, salvatz-lo sus vòstre disc dur e copiatz-lo aicí.',
'importstart'                => 'Impòrt de las paginas...',
'import-revision-count'      => '$1 {{PLURAL:$1|version|versions}}',
'importnopages'              => "Cap de pagina d'importar.",
'importfailed'               => 'Fracàs de l’impòrt : $1',
'importunknownsource'        => 'Tipe de la font d’impòrt desconegut',
'importcantopen'             => "Impossible de dobrir lo fichièr d'importar",
'importbadinterwiki'         => 'Ligam interwiki marrit',
'importnotext'               => 'Void o sens tèxt',
'importsuccess'              => "L'impòrt a capitat !",
'importhistoryconflict'      => "I a un conflicte dins l'istoric de las versions (aquesta pagina a pogut èsser importada de per abans).",
'importnosources'            => 'Cap de font interwiki es pas estada definida e la còpia dirècta d’istoric es desactivada.',
'importnofile'               => 'Cap de fichièr es pas estat importat.',
'importuploaderrorsize'      => "Lo telecargament del fichièr d'importar a pas capitat. Sa talha es mai granda que la autorizada.",
'importuploaderrorpartial'   => "Lo telecargament del fichièr d'importar a pas capitat. Aqueste o es pas estat que parcialament.",
'importuploaderrortemp'      => "Lo telecargament del fichièr d'importar a pas capitat. Un dorsièr temporari es mancant.",
'import-parse-failure'       => "Ruptura dins l'analisi de l'impòrt XML",
'import-noarticle'           => "Pas de pagina d'importar !",
'import-nonewrevisions'      => 'Totas las revisions son estadas importadas deperabans.',
'xml-error-string'           => '$1 a la linha $2, col $3 (octet $4) : $5',
'import-upload'              => "Impòrt d'un fichier XML",

# Import log
'importlogpage'                    => 'Istoric de las importacions de paginas',
'importlogpagetext'                => 'Impòrts administratius de paginas amb l’istoric a partir dels autres wikis.',
'import-logentry-upload'           => 'a importat (telecargament) [[$1]]',
'import-logentry-upload-detail'    => '$1 {{PLURAL:$1|version|versions}}',
'import-logentry-interwiki'        => 'a importat (transwiki) [[$1]]',
'import-logentry-interwiki-detail' => '$1 {{PLURAL:$1|version|versions}} dempuèi $2',

# Tooltip help for the actions
'tooltip-pt-userpage'             => "Ma pagina d'utilizaire",
'tooltip-pt-anonuserpage'         => "La pagina d'utilizare de l’IP amb laquala contribuissètz",
'tooltip-pt-mytalk'               => 'Ma pagina de discussion',
'tooltip-pt-anontalk'             => 'La pagina de discussion per aquesta adreça IP',
'tooltip-pt-preferences'          => 'Mas preferéncias',
'tooltip-pt-watchlist'            => 'La lista de las paginas que seguissètz',
'tooltip-pt-mycontris'            => 'Lista de mas contribucions',
'tooltip-pt-login'                => 'Sètz convidat(ada) a vos identificar, mas es pas obligatòri.',
'tooltip-pt-anonlogin'            => 'Sètz convidat(ada) a vos identificar, mas es pas obligatòri.',
'tooltip-pt-logout'               => 'Se desconnectar',
'tooltip-ca-talk'                 => "Discussion a prepaus d'aquesta pagina",
'tooltip-ca-edit'                 => 'Podètz modificar aquesta pagina. Mercé de previsualizar abans d’enregistrar.',
'tooltip-ca-addsection'           => 'Apondre un comentari a aquesta discussion.',
'tooltip-ca-viewsource'           => 'Aquesta pagina es protegida. Çaquelà, ne podètz veire lo contengut.',
'tooltip-ca-history'              => "Los autors e versions precedentas d'aquesta pagina.",
'tooltip-ca-protect'              => 'Protegir aquesta pagina',
'tooltip-ca-delete'               => 'Suprimir aquesta pagina',
'tooltip-ca-undelete'             => 'Restablir aquesta pagina',
'tooltip-ca-move'                 => 'Tornar nomenar aquesta pagina',
'tooltip-ca-watch'                => 'Apondètz aquesta pagina a vòstra lista de seguit',
'tooltip-ca-unwatch'              => 'Levatz aquesta pagina de vòstra lista de seguit',
'tooltip-search'                  => 'Cercar dins {{SITENAME}}',
'tooltip-search-go'               => 'Anar vèrs una pagina portant exactament aqueste nom se existís.',
'tooltip-search-fulltext'         => 'Recercar las paginas comportant aqueste tèxt.',
'tooltip-p-logo'                  => 'Pagina principala',
'tooltip-n-mainpage'              => 'Visitatz la pagina principala',
'tooltip-n-portal'                => 'A prepaus del projècte',
'tooltip-n-currentevents'         => "Trobar d'entresenhas suls eveniments actuals",
'tooltip-n-recentchanges'         => 'Lista dels darrièrs cambiaments sul wiki.',
'tooltip-n-randompage'            => "Afichar una pagina a l'azard",
'tooltip-n-help'                  => "L'endrech per s'assabentar.",
'tooltip-t-whatlinkshere'         => 'Lista de las paginas ligadas a aquesta',
'tooltip-t-recentchangeslinked'   => 'Lista dels darrièrs cambiaments de las paginas ligadas a aquesta',
'tooltip-feed-rss'                => 'Flus RSS per aquesta pagina',
'tooltip-feed-atom'               => 'Flus Atom per aquesta pagina',
'tooltip-t-contributions'         => "Veire la lista de las contribucions d'aqueste utilizaire",
'tooltip-t-emailuser'             => 'Mandar un corrièr electronic a aqueste utilizaire',
'tooltip-t-upload'                => 'Mandar un imatge o fichièr mèdia sul servidor',
'tooltip-t-specialpages'          => 'Lista de totas las paginas especialas',
'tooltip-t-print'                 => "Version imprimibla d'aquesta pagina",
'tooltip-t-permalink'             => 'Ligam permanent vèrs aquesta version de la pagina',
'tooltip-ca-nstab-main'           => 'Veire l’article',
'tooltip-ca-nstab-user'           => "Veire la pagina d'utilizaire",
'tooltip-ca-nstab-media'          => 'Veire la pagina del mèdia',
'tooltip-ca-nstab-special'        => 'Aquò es una pagina especiala, la podètz pas modificar.',
'tooltip-ca-nstab-project'        => 'Veire la pagina del projècte',
'tooltip-ca-nstab-image'          => 'Veire la pagina del fichièr',
'tooltip-ca-nstab-mediawiki'      => 'Vejatz lo messatge del sistèma',
'tooltip-ca-nstab-template'       => 'Vejatz lo modèl',
'tooltip-ca-nstab-help'           => 'Vejatz la pagina d’ajuda',
'tooltip-ca-nstab-category'       => 'Vejatz la pagina de la categoria',
'tooltip-minoredit'               => 'Marcar mas modificacions coma un cambiament menor',
'tooltip-save'                    => 'Salvar vòstras modificacions',
'tooltip-preview'                 => 'Mercé de previsualizar vòstras modificacions abans de salvar!',
'tooltip-diff'                    => "Permet de visualizar los cambiaments qu'avètz efectuats",
'tooltip-compareselectedversions' => "Afichar las diferéncias entre doas versions d'aquesta pagina",
'tooltip-watch'                   => 'Apondre aquesta pagina a vòstra lista de seguit',
'tooltip-recreate'                => 'Tornar crear la pagina, quitament se es estada escafada',
'tooltip-upload'                  => 'Amodar lo mandadís',

# Stylesheets
'common.css'      => '/** Lo CSS plaçat aicí serà aplicat a totas las aparéncias. */',
'standard.css'    => '/* Lo CSS plaçat aicí afectarà los utilizaires de l’abilhatge Estandard. */',
'nostalgia.css'   => '/* Lo CSS plaçat aicí afectarà los utilizaires de l’abilhatge Nostalgia. */',
'cologneblue.css' => '/* Lo CSS plaçat aicí afectarà los utilizaires de l’abilhatge Cologne Blue */',
'monobook.css'    => '/* Lo CSS plaçat aicí afectarà los utilizaires del skin Monobook */',
'myskin.css'      => '/* Lo CSS plaçat aicí afectarà los utilizaires de l’abilhatge Myskin */',
'chick.css'       => '/* Lo CSS plaçat aicí afectarà los utilizaires de l’abilhatge Chick */',
'simple.css'      => '/* Lo CSS plaçat aicí afectarà los utilizaires de l’abilhatge Simple */',
'modern.css'      => '/* Lo CSS plaçat aicí afectarà los utilizaires de l’abilhatge Modern */',

# Scripts
'common.js'   => '/* Un JavaScript quin que siá aicí serà cargat per un utilizaire quin que siá e per cada pagina accedida. */',
'monobook.js' => '/* Tot JavaScript aicí serà cargat amb las paginas accedidas pels utilizaires de l’abilhatge MonoBook unicament. */',

# Metadata
'nodublincore'      => 'Las metadonadas « Dublin Core RDF » son desactivadas sus aqueste servidor.',
'nocreativecommons' => 'Las metadonadas « Creative Commons RDF » son desactivadas sus aqueste servidor.',
'notacceptable'     => 'Aqueste servidor wiki pòt pas fornir las donadas dins un format que vòstre client es capable de legir.',

# Attribution
'anonymous'        => 'Utilizaire(s) anonim(s) de {{SITENAME}}',
'siteuser'         => 'Utilizaire $1 de {{SITENAME}}',
'lastmodifiedatby' => 'Aquesta pagina es estada modificada pel darrièr còp lo $1 a $2 per $3.', # $1 date, $2 time, $3 user
'othercontribs'    => 'Basat sul trabalh de $1.',
'others'           => 'autres',
'siteusers'        => 'Utilizaire(s) $1',
'creditspage'      => 'Pagina de crèdits',
'nocredits'        => 'I a pas d’entresenhas d’atribucion disponiblas per aquesta pagina.',

# Spam protection
'spamprotectiontitle' => 'Pagina protegida automaticament per causa de spam',
'spamprotectiontext'  => "La pagina qu'avètz ensajat de publicar es estada blocada pel filtre anti-spam.
Aquò es probablament causat per un ligam sus lista negra que punta cap a un sit extèrn.",
'spamprotectionmatch' => "La cadena de caractèrs « '''$1''' » a desenclavat lo detector de spam.",
'spambot_username'    => 'Netejatge de spam de MediaWiki',
'spam_reverting'      => 'Restabliment de la darrièra version que conten pas de ligam vèrs $1',
'spam_blanking'       => 'Totas las versions que contenon de ligams vèrs $1 son blanquidas',

# Info page
'infosubtitle'   => 'Entresenhas per la pagina',
'numedits'       => 'Nombre de modificacions : $1',
'numtalkedits'   => 'Nombre de modificacions (pagina de discussion) : $1',
'numwatchers'    => "Nombre de contributors qu'an la pagina dins lor lista de seguit : $1",
'numauthors'     => 'Nombre d’autors distints : $1',
'numtalkauthors' => 'Nombre d’autors distints (pagina de discussion) : $1',

# Math options
'mw_math_png'    => 'Totjorn produire un imatge PNG',
'mw_math_simple' => 'HTML se plan simpla, si que non PNG',
'mw_math_html'   => 'HTML se possible, si que non PNG',
'mw_math_source' => "Daissar lo còde TeX d'origina",
'mw_math_modern' => 'Pels navigadors modèrnes',
'mw_math_mathml' => 'MathML',

# Patrolling
'markaspatrolleddiff'                 => 'Marcar coma essent pas un vandalisme',
'markaspatrolledtext'                 => 'Marcar aqueste article coma pas vandalizat',
'markedaspatrolled'                   => 'Marcat coma pas vandalizat',
'markedaspatrolledtext'               => 'La version seleccionada es estada marcada coma pas vandalizada.',
'rcpatroldisabled'                    => 'La foncion de patrolha dels darrièrs cambiaments es pas activada.',
'rcpatroldisabledtext'                => 'La foncionalitat de susvelhança dels darrièrs cambiaments es pas activada.',
'markedaspatrollederror'              => 'Pòt pas èsser marcat coma pas vandalizat',
'markedaspatrollederrortext'          => 'Vos cal seleccionar una version per poder la marcar coma pas vandalizada.',
'markedaspatrollederror-noautopatrol' => 'Avètz pas lo drech de marcar vòstras pròprias modificacions coma susvelhadas.',

# Patrol log
'patrol-log-page'   => 'Istoric de las versions patrolhadas',
'patrol-log-header' => 'Vaquí un jornal de las versions patrolhadas.',
'patrol-log-line'   => 'a marcat la version $1 de $2 coma verificada $3',
'patrol-log-auto'   => '(automatic)',
'patrol-log-diff'   => 'v$1',

# Image deletion
'deletedrevision'                 => 'La version anciana $1 es estada suprimida.',
'filedeleteerror-short'           => 'Error al moment de la supression del fichièr : $1',
'filedeleteerror-long'            => "D'errors son estadas rencontradas al moment de la supression del fichièr :

$1",
'filedelete-missing'              => 'Lo fichièr « $1 » pòt pas èsser suprimit perque existís pas.',
'filedelete-old-unregistered'     => 'La revision del fichièr especificat « $1 » es pas dins la banca de donadas.',
'filedelete-current-unregistered' => 'Lo fichièr especificat « $1 » es pas dins la banca de donadas.',
'filedelete-archive-read-only'    => 'Lo dorsièr d’archivatge « $1 » es pas modificable pel servidor.',

# Browsing diffs
'previousdiff' => '← Cambiament precedent',
'nextdiff'     => 'Cambiament seguent →',

# Media information
'mediawarning'         => '<b>Atencion</b>: Aqueste fichièr pòt conténer de còde malvolent, vòstre sistèma pòt èsser mes en dangièr per son execucion. <hr />',
'imagemaxsize'         => 'Format maximal pels imatges dins las paginas de descripcion d’imatges :',
'thumbsize'            => 'Talha de la miniatura :',
'widthheightpage'      => '$1×$2, $3 {{PLURAL:$3|pagina|paginas}}',
'file-info'            => 'Talha del fichièr: $1, tipe MIME: $2',
'file-info-size'       => '($1 × $2 pixèl, talha del fichièr: $3, tipe MIME: $4)',
'file-nohires'         => '<small>Pas de resolucion mai nauta disponibla.</small>',
'svg-long-desc'        => '(Fichièr SVG, resolucion de $1 × $2 pixèls, talha : $3)',
'show-big-image'       => 'Imatge en resolucion mai nauta',
'show-big-image-thumb' => "<small>Talha d'aqueste apercebut : $1 × $2 pixèls</small>",

# Special:NewImages
'newimages'             => 'Galariá dels fichièrs novèls',
'imagelisttext'         => "Vaquí una lista de '''$1''' {{PLURAL:$1|fichièr|fichièrs}} classats $2.",
'newimages-summary'     => 'Aquesta pagina especiala aficha los darrièrs fichièrs importats.',
'showhidebots'          => '($1 bòts)',
'noimages'              => "Cap d'imatge d'afichar pas.",
'ilsubmit'              => 'Cercar',
'bydate'                => 'per data',
'sp-newimages-showfrom' => 'Afichar los imatges novèls importats dempuèi lo $2, $1',

# Bad image list
'bad_image_list' => "Lo format es lo seguent :

Solas las listas d'enumeracion (las linhas començant per *) son presas en compte. Lo primièr ligam d'una linha deu èsser cap a un imatge marrit.
Los autres ligams sus la meteissa linha son considerats coma d'excepcions, per exemple d'articles sulsquals l'imatge deu aparéisser.",

/*
Short names for language variants used for language conversion links.
To disable showing a particular link, set it to 'disable', e.g.
'variantname-zh-sg' => 'disable',
Variants for Chinese language
*/
'variantname-zh-hans' => 'hans',
'variantname-zh-hant' => 'hant',
'variantname-zh-cn'   => 'cn',
'variantname-zh-tw'   => 'tw',
'variantname-zh-hk'   => 'hk',
'variantname-zh-sg'   => 'sg',
'variantname-zh'      => 'zh',

# Variants for Serbian language
'variantname-sr-ec' => 'sr-ec',
'variantname-sr-el' => 'sr-el',
'variantname-sr'    => 'sr',

# Variants for Kazakh language
'variantname-kk-kz'   => 'kk-kz',
'variantname-kk-tr'   => 'kk-tr',
'variantname-kk-cn'   => 'kk-cn',
'variantname-kk-cyrl' => 'kk-cyrl',
'variantname-kk-latn' => 'kk-latn',
'variantname-kk-arab' => 'kk-arab',
'variantname-kk'      => 'kk',

# Variants for Kurdish language
'variantname-ku-arab' => 'ku-Arab',

# Metadata
'metadata'          => 'Metadonadas',
'metadata-help'     => "Aqueste fichièr conten d'entresenhas suplementàrias probablament apondudas per l’aparelh de fòto numeric o l'escanèr que las a aquesas. Se lo fichièr es estat modificat dempuèi son estat original, d'unes detalhs pòdon reflectir pas entièrament l’imatge modificat.",
'metadata-expand'   => 'Mostrar las entresenhas detalhadas',
'metadata-collapse' => 'Amagar las entresenhas detalhadas',
'metadata-fields'   => 'Los camps de metadonadas d’EXIF listats dins aqueste message seràn incluses dins la pagina de descripcion de l’imatge quand la taula de metadonadas serà reduccha. Los autres camps seràn amagats per defaut.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => 'Largor',
'exif-imagelength'                 => 'Nautor',
'exif-bitspersample'               => 'Bits per compausanta',
'exif-compression'                 => 'Tipe de compression',
'exif-photometricinterpretation'   => 'Composicion dels pixèls (Modèl colorimetric)',
'exif-orientation'                 => 'Orientacion',
'exif-samplesperpixel'             => 'Nombre de compausants (Compausantas per pixèl)',
'exif-planarconfiguration'         => 'Arrengament de las donadas',
'exif-ycbcrsubsampling'            => 'Taus d’escandalhatge de las compausantas de la crominança',
'exif-ycbcrpositioning'            => 'Posicionament YCbCr',
'exif-xresolution'                 => 'Resolucion orizontala',
'exif-yresolution'                 => 'Resolucion verticala',
'exif-resolutionunit'              => 'Unitats de resolucion X e Y',
'exif-stripoffsets'                => 'Emplaçament de las donadas de l’imatge',
'exif-rowsperstrip'                => 'Nombre de linhas per benda',
'exif-stripbytecounts'             => 'Talha en octets per benda',
'exif-jpeginterchangeformat'       => 'Posicion del SOI JPEG',
'exif-jpeginterchangeformatlength' => 'Talha en octet de las donadas JPEG',
'exif-transferfunction'            => 'Foncion de transferiment',
'exif-whitepoint'                  => 'Cromaticitat del punt blanc',
'exif-primarychromaticities'       => 'Cromaticitats de las colors primàrias',
'exif-ycbcrcoefficients'           => 'Coeficients de la matritz de transformacion de l’espaci colorimetric (YCbCr)',
'exif-referenceblackwhite'         => 'Valors de referéncia blanc e negre',
'exif-datetime'                    => 'Data e ora de cambiament del fichièr',
'exif-imagedescription'            => 'Títol de l’imatge',
'exif-make'                        => 'Fabricant de l’aparelh',
'exif-model'                       => 'Modèl de l’aparelh',
'exif-software'                    => 'Logicial utilizat',
'exif-artist'                      => 'Autor',
'exif-copyright'                   => 'Detentor del copyright',
'exif-exifversion'                 => 'Version exif',
'exif-flashpixversion'             => 'Version Flashpix suportada',
'exif-colorspace'                  => 'Espaci colorimetric',
'exif-componentsconfiguration'     => 'Significacion de cada compausanta',
'exif-compressedbitsperpixel'      => 'Mòde de compression de l’imatge',
'exif-pixelydimension'             => 'Largor d’imatge valida',
'exif-pixelxdimension'             => 'Nautor d’imatge valida',
'exif-makernote'                   => 'Nòtas del fabricant',
'exif-usercomment'                 => "Comentaris de l'utilizaire",
'exif-relatedsoundfile'            => 'Fichièr àudio associat',
'exif-datetimeoriginal'            => 'Data e ora de la generacion de donadas',
'exif-datetimedigitized'           => 'Data e ora de numerizacion',
'exif-subsectime'                  => 'Data de darrièr cambiament',
'exif-subsectimeoriginal'          => 'Data de la presa originala',
'exif-subsectimedigitized'         => 'Data de la numerizacion',
'exif-exposuretime'                => "Temps d'exposicion",
'exif-exposuretime-format'         => '$1 seg ($2)',
'exif-fnumber'                     => 'Nombre f (Focala)',
'exif-exposureprogram'             => 'Programa d’exposicion',
'exif-spectralsensitivity'         => 'Sensibilitat espectrala',
'exif-isospeedratings'             => 'Sensibilitat ISO',
'exif-oecf'                        => 'Factor de conversion optoelectronic',
'exif-shutterspeedvalue'           => 'Velocitat d’obturacion',
'exif-aperturevalue'               => 'Dobertura',
'exif-brightnessvalue'             => 'Luminositat',
'exif-exposurebiasvalue'           => 'Correccion d’exposicion',
'exif-maxaperturevalue'            => 'Camp de dobertura maximal',
'exif-subjectdistance'             => 'Distància del subjècte',
'exif-meteringmode'                => 'Mòde de mesura',
'exif-lightsource'                 => 'Font de lutz',
'exif-flash'                       => 'Flash',
'exif-focallength'                 => 'Longor de focala',
'exif-subjectarea'                 => 'Emplaçament del subjècte',
'exif-flashenergy'                 => 'Energia del flash',
'exif-spatialfrequencyresponse'    => 'Responsa en frequéncia espaciala',
'exif-focalplanexresolution'       => 'Resolucion orizontala focala plana',
'exif-focalplaneyresolution'       => 'Resolucion verticala focala plana',
'exif-focalplaneresolutionunit'    => 'Unitat de resolucion de focala plana',
'exif-subjectlocation'             => 'Posicion del subjècte',
'exif-exposureindex'               => 'Indèx d’exposicion',
'exif-sensingmethod'               => 'Tipe de captador',
'exif-filesource'                  => 'Font del fichièr',
'exif-scenetype'                   => 'Tipe de scèna',
'exif-cfapattern'                  => 'Matritz de filtratge de color',
'exif-customrendered'              => 'Tractament d’imatge personalizat',
'exif-exposuremode'                => 'Mòde d’exposicion',
'exif-whitebalance'                => 'Balança dels blancs',
'exif-digitalzoomratio'            => 'Taus d’agrandiment numeric (zoom)',
'exif-focallengthin35mmfilm'       => 'Longor de focala per un filme 35 mm',
'exif-scenecapturetype'            => 'Tipe de captura de la scèna',
'exif-gaincontrol'                 => 'Contraròtle de luminositat',
'exif-contrast'                    => 'Contraste',
'exif-saturation'                  => 'Saturacion',
'exif-sharpness'                   => 'Netetat',
'exif-devicesettingdescription'    => 'Descripcion de la configuracion del dispositiu',
'exif-subjectdistancerange'        => 'Distància del subjècte',
'exif-imageuniqueid'               => 'Identificant unic de l’imatge',
'exif-gpsversionid'                => 'Version de la balisa (tag) GPS',
'exif-gpslatituderef'              => 'Referéncia per la Latitud (Nòrd o Sud)',
'exif-gpslatitude'                 => 'Latitud',
'exif-gpslongituderef'             => 'Referéncia per la longitud (Èst o Oèst)',
'exif-gpslongitude'                => 'Longitud',
'exif-gpsaltituderef'              => 'Referéncia d’altitud',
'exif-gpsaltitude'                 => 'Altitud',
'exif-gpstimestamp'                => 'Ora GPS (relòtge atomic)',
'exif-gpssatellites'               => 'Satellits utilizats per la mesura',
'exif-gpsstatus'                   => 'Estat del receptor',
'exif-gpsmeasuremode'              => 'Mòde de mesura',
'exif-gpsdop'                      => 'Precision de la mesura',
'exif-gpsspeedref'                 => 'Unitat de velocitat del receptor GPS',
'exif-gpsspeed'                    => 'Velocitat del receptor GPS',
'exif-gpstrackref'                 => 'Referéncia per la direccion del movement',
'exif-gpstrack'                    => 'Direccion del movement',
'exif-gpsimgdirectionref'          => 'Referéncia per l’orientacion de l’imatge',
'exif-gpsimgdirection'             => 'Direccion de l’imatge',
'exif-gpsmapdatum'                 => 'Sistèma geodesic utilizat',
'exif-gpsdestlatituderef'          => 'Referéncia per la latitud de la destinacion',
'exif-gpsdestlatitude'             => 'Latitud de la destinacion',
'exif-gpsdestlongituderef'         => 'Referéncia per la longitud de la destinacion',
'exif-gpsdestlongitude'            => 'Longitud de la destinacion',
'exif-gpsdestbearingref'           => 'Referéncia pel relevament de la destinacion',
'exif-gpsdestbearing'              => 'Relevament de la destinacion',
'exif-gpsdestdistanceref'          => 'Referéncia per la distància de la destinacion',
'exif-gpsdestdistance'             => 'Distància a la destinacion',
'exif-gpsprocessingmethod'         => 'Nom del metòde de tractament del GPS',
'exif-gpsareainformation'          => 'Nom de la zòna GPS',
'exif-gpsdatestamp'                => 'Data GPS',
'exif-gpsdifferential'             => 'Correccion diferenciala GPS',

# EXIF attributes
'exif-compression-1' => 'Sens compression',

'exif-unknowndate' => 'Data desconeguda',

'exif-orientation-1' => 'Normala', # 0th row: top; 0th column: left
'exif-orientation-2' => 'Inversada orizontalament', # 0th row: top; 0th column: right
'exif-orientation-3' => 'Virada de 180°', # 0th row: bottom; 0th column: right
'exif-orientation-4' => 'Inversada verticalament', # 0th row: bottom; 0th column: left
'exif-orientation-5' => 'Virada de 90° dins lo sens antiorari e inversada verticalament', # 0th row: left; 0th column: top
'exif-orientation-6' => 'Virada de 90° dins lo sens orari', # 0th row: right; 0th column: top
'exif-orientation-7' => 'Virada de 90° dins lo sens orari e inversada verticalament', # 0th row: right; 0th column: bottom
'exif-orientation-8' => 'Virada de 90° dins lo sens antiorari', # 0th row: left; 0th column: bottom

'exif-planarconfiguration-1' => 'Donadas atenentas',
'exif-planarconfiguration-2' => 'Donadas separadas',

'exif-xyresolution-c' => '$1 dpc',

'exif-colorspace-ffff.h' => 'Pas calibrat',

'exif-componentsconfiguration-0' => 'existís pas',
'exif-componentsconfiguration-5' => 'V',

'exif-exposureprogram-0' => 'Indefinit',
'exif-exposureprogram-1' => 'Manual',
'exif-exposureprogram-2' => 'Programa normal',
'exif-exposureprogram-3' => 'Prioritat a la dobertura',
'exif-exposureprogram-4' => 'Prioritat a l’obturacion',
'exif-exposureprogram-5' => 'Programa de creacion (preferéncia a la prigondor de camp)',
'exif-exposureprogram-6' => "Programa d'accion (preferéncia a la velocitat d’obturacion)",
'exif-exposureprogram-7' => 'Mòde retrach (per clichats de prèp amb rèire plan fosc)',
'exif-exposureprogram-8' => 'Mòde paisatge (per de clichats de paisatges nets)',

'exif-subjectdistance-value' => '{{PLURAL:$1|$1 mètre|$1 mètres}}',

'exif-meteringmode-0'   => 'Desconegut',
'exif-meteringmode-1'   => 'Mejana',
'exif-meteringmode-2'   => 'Mesura centrala mejana',
'exif-meteringmode-3'   => 'Espòt',
'exif-meteringmode-4'   => 'MultiEspòt',
'exif-meteringmode-5'   => 'Paleta',
'exif-meteringmode-6'   => 'Parcial',
'exif-meteringmode-255' => 'Autra',

'exif-lightsource-0'   => 'Desconeguda',
'exif-lightsource-1'   => 'Lutz del jorn',
'exif-lightsource-2'   => 'Fluorescent',
'exif-lightsource-3'   => 'Tungstèn (lum incandescent)',
'exif-lightsource-4'   => 'Flash',
'exif-lightsource-9'   => 'Temps clar',
'exif-lightsource-10'  => 'Temps ennivolat',
'exif-lightsource-11'  => 'Ombra',
'exif-lightsource-12'  => 'Esclairatge fluorescent lutz del jorn (D 5700 – 7100K)',
'exif-lightsource-13'  => 'Esclairatge fluorescent blanc (N 4600 – 5400K)',
'exif-lightsource-14'  => 'Esclairatge fluorescent blanc freg (W 3900 – 4500K)',
'exif-lightsource-15'  => 'Esclairatge fluorescent blanc (WW 3200 – 3700K)',
'exif-lightsource-17'  => 'Lum estandard A',
'exif-lightsource-18'  => 'Lum estandard B',
'exif-lightsource-19'  => 'Lum estandard C',
'exif-lightsource-22'  => 'D75',
'exif-lightsource-23'  => 'D50',
'exif-lightsource-24'  => "Tungstèni ISO d'estudiò",
'exif-lightsource-255' => 'Autra font de lum',

'exif-focalplaneresolutionunit-2' => 'poce',

'exif-sensingmethod-1' => 'Pas definit',
'exif-sensingmethod-2' => 'Captador de zòna de colors monocromaticas',
'exif-sensingmethod-3' => 'Captador de zòna de colors bicromaticas',
'exif-sensingmethod-4' => 'Captador de zòna de colors tricromaticas',
'exif-sensingmethod-5' => 'Captador de color sequencial',
'exif-sensingmethod-7' => 'Captador trilinear',
'exif-sensingmethod-8' => 'Captador de color linear sequencial',

'exif-filesource-3' => 'Aparelh fotografic numeric',

'exif-scenetype-1' => 'Imatge dirèctament fotografiat',

'exif-customrendered-0' => 'Procediment normal',
'exif-customrendered-1' => 'Procediment personalizat',

'exif-exposuremode-0' => 'Exposicion automatica',
'exif-exposuremode-1' => 'Exposicion manuala',
'exif-exposuremode-2' => 'Forqueta (Bracketting) automatica',

'exif-whitebalance-0' => 'Balança dels blancs automatica',
'exif-whitebalance-1' => 'Balança dels blancs manuala',

'exif-scenecapturetype-0' => 'Estandard',
'exif-scenecapturetype-1' => 'Paisatge',
'exif-scenecapturetype-2' => 'Retrach',
'exif-scenecapturetype-3' => 'Scèna nuechenca',

'exif-gaincontrol-0' => 'Cap',
'exif-gaincontrol-1' => 'Augmentacion febla de l’aquisicion',
'exif-gaincontrol-2' => 'Augmentacion fòrta de l’aquisicion',
'exif-gaincontrol-3' => 'Reduccion febla de l’aquisicion',
'exif-gaincontrol-4' => 'Reduccion fòrta de l’aquisicion',

'exif-contrast-0' => 'Normal',
'exif-contrast-1' => 'Feble',
'exif-contrast-2' => 'Fòrt',

'exif-saturation-0' => 'Normal',
'exif-saturation-1' => 'Saturacion febla',
'exif-saturation-2' => 'Saturacion elevada',

'exif-sharpness-0' => 'Normala',
'exif-sharpness-1' => 'Doça',
'exif-sharpness-2' => 'Dura',

'exif-subjectdistancerange-0' => 'Desconegut',
'exif-subjectdistancerange-1' => 'Macrò',
'exif-subjectdistancerange-2' => 'Sarrat',
'exif-subjectdistancerange-3' => 'Luenhenc',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'Latitud Nòrd',
'exif-gpslatitude-s' => 'Latitud Sud',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'Longitud Èst',
'exif-gpslongitude-w' => 'Longitud Oèst',

'exif-gpsstatus-a' => 'Mesura en cors',
'exif-gpsstatus-v' => 'Interoperabilitat de la mesura',

'exif-gpsmeasuremode-2' => 'Mesura de 2 dimensions',
'exif-gpsmeasuremode-3' => 'Mesura de 3 dimensions',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'Quilomètres per ora',
'exif-gpsspeed-m' => 'Miles per ora',
'exif-gpsspeed-n' => 'Noses',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => 'Direccion vertadièra',
'exif-gpsdirection-m' => 'Nòrd magnetic',

# External editor support
'edit-externally'      => 'Modificar aqueste fichièr en utilizant una aplicacion extèrna',
'edit-externally-help' => 'Vejatz [http://www.mediawiki.org/wiki/Manual:External_editors las instruccions] per mai d’informacions.',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'totes',
'imagelistall'     => 'totes',
'watchlistall2'    => 'tot',
'namespacesall'    => 'Totes',
'monthsall'        => 'totes',

# E-mail address confirmation
'confirmemail'             => "Confirmar l'adreça de corrièr electronic",
'confirmemail_noemail'     => 'L’adreça de corrièr electronic configurada dins vòstras [[Special:Preferences|preferéncias]] es pas valida.',
'confirmemail_text'        => '{{SITENAME}} necessita la verificacion de vòstra adreça de corrièr electronic abans de poder utilizar tota foncion de messatjariá. Utilizatz lo boton çaijós per mandar un corrièr electronic de confirmacion a vòstra adreça. Lo corrièr contendrà un ligam contenent un còde, cargatz aqueste ligam dins vòstre navigador per validar vòstra adreça.',
'confirmemail_pending'     => '<div class="error">
Un còde de confirmacion ja vos es estat mandat per corrièr electronic ; se venètz de crear vòstre compte, esperatz qualques minutas que l’e-mail arribe abans de demandar un còde novèl. </div>',
'confirmemail_send'        => 'Mandar un còde de confirmacion',
'confirmemail_sent'        => 'Corrièr electronic de confirmacion mandat.',
'confirmemail_oncreate'    => "Un còde de confirmacion es estat mandat a vòstra adreça de corrièr electronic.
Aqueste còde es pas requesit per se connectar, mas n'aurètz besonh per activar las foncionalitats ligadas als corrièrs electronics sus aqueste wiki.",
'confirmemail_sendfailed'  => '{{SITENAME}} pòt pas mandar lo corrièr de confirmacion.
Verificatz se vòstra adreça conten pas de caractèrs interdiches.

Retorn del programa de corrièr : $1',
'confirmemail_invalid'     => 'Còde de confirmacion incorrècte. Benlèu lo còde a expirat.',
'confirmemail_needlogin'   => 'Vos cal vos $1 per confirmar vòstra adreça de corrièr electronic.',
'confirmemail_success'     => 'Vòstra adreça de corrièr electronic es confirmada. Ara, vos podètz connectar e aprofechar del wiki.',
'confirmemail_loggedin'    => 'Ara, vòstra adreça es confirmada',
'confirmemail_error'       => 'Un problèma es subrevengut en volent enregistrar vòstra confirmacion',
'confirmemail_subject'     => 'Confirmacion d’adreça de corrièr electronic per {{SITENAME}}',
'confirmemail_body'        => "Qualqu’un, probablament vos amb l’adreça IP $1, a enregistrat un compte « $2 » amb aquesta adreça de corrièr electronic sul sit {{SITENAME}}.

Per confirmar qu'aqueste compte vos aparten vertadièrament e activar las foncions de messatjariá sus {{SITENAME}}, seguissètz lo ligam çaijós dins vòstre navigador :

$3

Se s’agís pas de vos, dobrissetz pas lo ligam.
Aqueste còde de confirmacion expirarà lo $4, seguissètz l’autre ligam çaijós dins vòstre navigador :

$5

Aqueste còde de confirmacion expirarà lo $4.",
'confirmemail_invalidated' => 'Confirmacion de l’adreça de corrièr electronic anullada',
'invalidateemail'          => 'Anullar la confirmacion del corrièr electronic',

# Scary transclusion
'scarytranscludedisabled' => '[La transclusion interwiki es desactivada]',
'scarytranscludefailed'   => '[La recuperacion de modèl a pas capitat per $1]',
'scarytranscludetoolong'  => '[L’URL es tròp longa]',

# Trackbacks
'trackbackbox'      => '<div id="mw_trackbacks">
Retroligams cap a aquesta pagina :<br />
$1
</div>',
'trackbackremove'   => '([$1 Suprimir])',
'trackbacklink'     => 'Retroligam',
'trackbackdeleteok' => 'Lo retroligam es estat suprimit amb succès.',

# Delete conflict
'deletedwhileediting' => "'''Atencion''' : aquesta pagina es estada suprimida aprèp qu'avètz començat de la modificar !",
'confirmrecreate'     => "L'utilizaire [[User:$1|$1]] ([[User talk:$1|talk]]) a suprimit aquesta pagina, alara que l'aviatz començat d'editar, pel motiu seguent:
: ''$2''
Confirmatz que desiratz tornar crear aqueste article.",
'recreate'            => 'Tornar crear',

# HTML dump
'redirectingto' => 'Redireccion cap a [[:$1]]...',

# action=purge
'confirm_purge'        => "Volètz refrescar aquesta pagina (purgar l'amagatal) ?

$1",
'confirm_purge_button' => 'Confirmar',

# AJAX search
'searchcontaining' => 'Cercar los articles que contenon « $1 ».',
'searchnamed'      => 'Cercar los articles nomenats « $1 ».',
'articletitles'    => 'Articles que començan per « $1 »',
'hideresults'      => 'Amagar los resultats',
'useajaxsearch'    => 'Utilizar la recèrca AJAX',

# Separators for various lists, etc.
'colon-separator'    => '&nbsp;:&#32;',
'autocomment-prefix' => '-',

# Multipage image navigation
'imgmultipageprev' => '← pagina precedenta',
'imgmultipagenext' => 'pagina seguenta →',
'imgmultigo'       => 'Accedir !',
'imgmultigoto'     => 'Anar a la pagina $1',

# Table pager
'ascending_abbrev'         => 'creissent',
'descending_abbrev'        => 'descreissent',
'table_pager_next'         => 'Pagina seguenta',
'table_pager_prev'         => 'Pagina precedenta',
'table_pager_first'        => 'Primièra pagina',
'table_pager_last'         => 'Darrièra pagina',
'table_pager_limit'        => 'Mostrar $1 elements per pagina',
'table_pager_limit_submit' => 'Accedir',
'table_pager_empty'        => 'Cap de resultat',

# Auto-summaries
'autosumm-blank'   => 'Resumit automatic : blanquiment',
'autosumm-replace' => 'Resumit automatic : contengut remplaçat per « $1 ».',
'autoredircomment' => 'Redireccion cap a [[$1]]',
'autosumm-new'     => 'Pagina novèla : $1',

# Size units
'size-bytes'     => '$1 o',
'size-kilobytes' => '$1 Ko',
'size-megabytes' => '$1 Mo',
'size-gigabytes' => '$1 Go',

# Live preview
'livepreview-loading' => 'Cargament…',
'livepreview-ready'   => 'Cargament… Acabat!',
'livepreview-failed'  => 'L’apercebut rapid a pas capitat!
Ensajatz la previsualizacion normala.',
'livepreview-error'   => 'Impossible de se connectar : $1 "$2"
Ensajatz la previsualizacion normala.',

# Friendlier slave lag warnings
'lag-warn-normal' => 'Los cambiaments que datan de mens de $1 {{PLURAL:$1|segonda|segondas}} pòdon aparéisser pas dins aquesta tièra.',
'lag-warn-high'   => 'En rason d’una fòrta carga de las bancas de donadas, los cambiaments que datan de mens de $1 {{PLURAL:$1|segonda|segondas}} pòdon aparéisser pas dins aquesta tièra.',

# Watchlist editor
'watchlistedit-numitems'       => 'Vòstra lista de seguit conten {{PLURAL:$1|una pagina|$1 paginas}}, sens comptar las paginas de discussion',
'watchlistedit-noitems'        => 'Vòstra lista de seguit conten pas cap de pagina.',
'watchlistedit-normal-title'   => 'Modificacion de la lista de seguit',
'watchlistedit-normal-legend'  => 'Levar de paginas de la lista de seguit',
'watchlistedit-normal-explain' => 'Las paginas de vòstra lista de seguit son visiblas çaijós, classadas per espaci de noms. Per levar una pagina (e sa pagina de discussion) de la lista, seleccionatz la casa al costat puèi clicatz sul boton en bas. Tanben podètz [[Special:Watchlist/raw|la modificar en mòde brut]].',
'watchlistedit-normal-submit'  => 'Levar las paginas seleccionadas',
'watchlistedit-normal-done'    => '{{PLURAL:$1|Una pagina es estada levada|$1 paginas son estadas levadas}} de vòstra lista de seguit :',
'watchlistedit-raw-title'      => 'Modificacion de la lista de seguit (mòde brut)',
'watchlistedit-raw-legend'     => 'Modificacion de la lista de seguit en mòde brut',
'watchlistedit-raw-explain'    => 'La lista de las paginas de vòstra lista de seguit es mostrada çaijós, sens las paginas de discussion (automaticament inclusas) e destriadas per espaci de noms. Podètz modificar la lista : apondètz las paginas que volètz seguir (pauc impòrta ont), una pagina per linha, e levatz las paginas que volètz pas mai seguir. Quand avètz acabat, clicatz sul boton en bas per metre la lista a jorn. Tanben podètz utilizar [[Special:Watchlist/edit|l’editaire normal]].',
'watchlistedit-raw-titles'     => 'Títols :',
'watchlistedit-raw-submit'     => 'Metre la lista a jorn',
'watchlistedit-raw-done'       => 'Vòstra lista de seguit es estada mesa a jorn.',
'watchlistedit-raw-added'      => '{{PLURAL:$1|Una pagina es estada aponduda|$1 paginas son estadas apondudas}} :',
'watchlistedit-raw-removed'    => '{{PLURAL:$1|Una pagina es estada levada|$1 paginas son estadas levadas}} :',

# Watchlist editing tools
'watchlisttools-view' => 'Lista de seguit',
'watchlisttools-edit' => 'Veire e modificar la lista de seguit',
'watchlisttools-raw'  => 'Modificar la lista (mòde brut)',

# Core parser functions
'unknown_extension_tag' => "Balisa d'extension « $1 » desconeguda",

# Special:Version
'version'                          => 'Version', # Not used as normal message but as header for the special page itself
'version-extensions'               => 'Extensions installadas',
'version-specialpages'             => 'Paginas especialas',
'version-parserhooks'              => 'Extensions del parser',
'version-variables'                => 'Variablas',
'version-other'                    => 'Divèrs',
'version-mediahandlers'            => 'Supòrts mèdia',
'version-hooks'                    => 'Croquets',
'version-extension-functions'      => 'Foncions de las extensions',
'version-parser-extensiontags'     => 'Balisas suplementàrias del parser',
'version-parser-function-hooks'    => 'Croquets de las foncions del parser',
'version-skin-extension-functions' => "Foncions d'extension de l'interfàcia",
'version-hook-name'                => 'Nom del croquet',
'version-hook-subscribedby'        => 'Definit per',
'version-version'                  => 'Version',
'version-license'                  => 'Licéncia',
'version-software'                 => 'Logicial installat',
'version-software-product'         => 'Produch',
'version-software-version'         => 'Version',

# Special:FilePath
'filepath'         => "Camin d'accès d'un fichièr",
'filepath-page'    => 'Fichièr :',
'filepath-submit'  => "Camin d'accès",
'filepath-summary' => "Aquesta pagina especiala balha lo camin d'accès complet d’un fichièr ; los imatges son mostrats en nauta resolucion, los fichièrs audiò e vidèo s’executan amb lor programa associat.

Picatz lo nom del fichièr sens lo prefix « {{ns:image}}: »",

# Special:FileDuplicateSearch
'fileduplicatesearch'          => 'Recèrca dels fichièrs en doble',
'fileduplicatesearch-summary'  => 'Recèrca per de fichièrs en doble sus la banca de valors fragmentàrias.

Picatz lo nom del fichièr sens lo prefix « {{ns:image}}: ».',
'fileduplicatesearch-legend'   => 'Recèrca d’un doble',
'fileduplicatesearch-filename' => 'Nom del fichièr :',
'fileduplicatesearch-submit'   => 'Recercar',
'fileduplicatesearch-info'     => '$1 × $2 pixèls<br />Talha del fichièr : $3<br />MIME type : $4',
'fileduplicatesearch-result-1' => 'Lo fichièr « $1 » a pas de doble identic.',
'fileduplicatesearch-result-n' => 'Lo fichièr « $1 » a {{PLURAL:$2|1 doble identic|$2 dobles identics}}.',

# Special:SpecialPages
'specialpages'                   => 'Paginas especialas',
'specialpages-note'              => '----
* Las paginas especialas 
* <span class="mw-specialpagerestricted">en gras</span> son restrenhudas.',
'specialpages-group-maintenance' => 'Rapòrts de mantenença',
'specialpages-group-other'       => 'Autras paginas especialas',
'specialpages-group-login'       => 'Se connectar / s’enregistrar',
'specialpages-group-changes'     => 'Darrièrs cambiaments e jornals',
'specialpages-group-media'       => 'Rapòrts dels fichièrs de mèdias e dels impòrts',
'specialpages-group-users'       => 'Utilizaires e dreches estacats',
'specialpages-group-highuse'     => 'Utilizacion intensa de las paginas',
'specialpages-group-pages'       => 'Lista de paginas',
'specialpages-group-pagetools'   => 'Espleches per las paginas',
'specialpages-group-wiki'        => 'Donadas del wiki e espleches',
'specialpages-group-redirects'   => 'Redireccions',
'specialpages-group-spam'        => 'Espleches pel spam',

# Special:BlankPage
'blankpage'              => 'Pagina voida',
'intentionallyblankpage' => 'Aquesta pagina es intencionalament voida e es utilizada coma un tèst de performància, eca.',

);
