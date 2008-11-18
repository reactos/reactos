<?php
/** Catalan (Català)
 *
 * @ingroup Language
 * @file
 *
 * @author Aleator
 * @author Iradigalesc
 * @author Jordi Roqué
 * @author Juanpabl
 * @author Martorell
 * @author McDutchie
 * @author Pasqual (ca)
 * @author Paucabot
 * @author Pérez
 * @author SMP
 * @author Smeira
 * @author Spacebirdy
 * @author Toniher
 * @author Vriullop
 * @author לערי ריינהארט
 */

$skinNames = array(
	'standard'    => 'Clàssic',
	'nostalgia'   => 'Nostàlgia',
	'cologneblue' => 'Colònia blava',
);

$bookstoreList = array(
	'Catàleg Col·lectiu de les Universitats de Catalunya' => 'http://ccuc.cbuc.es/cgi-bin/vtls.web.gateway?searchtype=control+numcard&searcharg=$1',
	'Totselsllibres.com' => 'http://www.totselsllibres.com/tel/publi/busquedaAvanzadaLibros.do?ISBN=$1',
	'inherit' => true,
);

$namespaceNames = array(
	NS_MEDIA          => 'Media',
	NS_SPECIAL        => 'Especial',
	NS_MAIN           => '',
	NS_TALK           => 'Discussió',
	NS_USER           => 'Usuari',
	NS_USER_TALK      => 'Usuari_Discussió',
	# NS_PROJECT set by \$wgMetaNamespace
	NS_PROJECT_TALK   => '$1_Discussió',
	NS_IMAGE          => 'Imatge',
	NS_IMAGE_TALK     => 'Imatge_Discussió',
	NS_MEDIAWIKI      => 'MediaWiki',
	NS_MEDIAWIKI_TALK => 'MediaWiki_Discussió',
	NS_TEMPLATE       => 'Plantilla',
	NS_TEMPLATE_TALK  => 'Plantilla_Discussió',
	NS_HELP           => 'Ajuda',
	NS_HELP_TALK      => 'Ajuda_Discussió',
	NS_CATEGORY       => 'Categoria',
	NS_CATEGORY_TALK  => 'Categoria_Discussió',
);

$separatorTransformTable = array(',' => '.', '.' => ',' );

$dateFormats = array(
	'mdy time' => 'H:i',
	'mdy date' => 'M j, Y',
	'mdy both' => 'H:i, M j, Y',

	'dmy time' => 'H:i',
	'dmy date' => 'j M Y',
	'dmy both' => 'H:i, j M Y',

	'ymd time' => 'H:i',
	'ymd date' => 'Y M j',
	'ymd both' => 'H:i, Y M j',
);

$magicWords = array(
	'img_right'           => array( '1', 'right', 'dreta' ),
	'img_left'            => array( '1', 'left', 'esquerra' ),
	'displaytitle'        => array( '1', 'DISPLAYTITLE', 'TÍTOL' ),
	'defaultsort'         => array( '1', 'DEFAULTSORT:', 'DEFAULTSORTKEY:', 'DEFAULTCATEGORYSORT:', 'ORDENA:' ),
);

$specialPageAliases = array(
	'DoubleRedirects'           => array( 'Redireccions_dobles' ),
	'BrokenRedirects'           => array( 'Redireccions_rompudes' ),
	'Disambiguations'           => array( 'Desambiguacions' ),
	'Userlogin'                 => array( 'Registre_i_entrada' ),
	'Userlogout'                => array( 'Finalitza_sessió' ),
	'Preferences'               => array( 'Preferències' ),
	'Watchlist'                 => array( 'Llista_de_seguiment' ),
	'Recentchanges'             => array( 'Canvis_recents' ),
	'Upload'                    => array( 'Carrega' ),
	'Imagelist'                 => array( 'Imatges' ),
	'Newimages'                 => array( 'Imatges_noves' ),
	'Listusers'                 => array( 'Usuaris' ),
	'Statistics'                => array( 'Estadístiques' ),
	'Randompage'                => array( 'Article_aleatori', 'Atzar', 'Aleatori' ),
	'Lonelypages'               => array( 'Pàgines_òrfenes' ),
	'Uncategorizedpages'        => array( 'Pàgines_sense_categoria' ),
	'Uncategorizedcategories'   => array( 'Categories_sense_categoria' ),
	'Uncategorizedimages'       => array( 'Imatges_sense_categoria' ),
	'Uncategorizedtemplates'    => array( 'Plantilles_sense_categoria' ),
	'Unusedcategories'          => array( 'Categories_no_usades' ),
	'Unusedimages'              => array( 'Imatges_no_usades' ),
	'Wantedpages'               => array( 'Pàgines_demanades' ),
	'Wantedcategories'          => array( 'Categories_demanades' ),
	'Mostlinked'                => array( 'Pàgines_més_enllaçades' ),
	'Mostlinkedcategories'      => array( 'Categories_més_útils' ),
	'Mostlinkedtemplates'       => array( 'Plantilles_més_útils' ),
	'Mostcategories'            => array( 'Pàgines_amb_més_categories' ),
	'Mostimages'                => array( 'Imatges_més_útils' ),
	'Mostrevisions'             => array( 'Pàgines_més_editades' ),
	'Fewestrevisions'           => array( 'Pàgines_menys_editades' ),
	'Shortpages'                => array( 'Pàgines_curtes' ),
	'Longpages'                 => array( 'Pàgines_llargues' ),
	'Newpages'                  => array( 'Pàgines_noves' ),
	'Ancientpages'              => array( 'Pàgines_velles' ),
	'Deadendpages'              => array( 'Atzucacs' ),
	'Protectedpages'            => array( 'Pàgines_protegides' ),
	'Allpages'                  => array( 'Llista_de_pàgines' ),
	'Prefixindex'               => array( 'Cerca_per_prefix' ),
	'Ipblocklist'               => array( 'Usuaris_blocats' ),
	'Specialpages'              => array( 'Pàgines_especials' ),
	'Contributions'             => array( 'Contribucions' ),
	'Emailuser'                 => array( 'Envia_missatge' ),
	'Whatlinkshere'             => array( 'Enllaços' ),
	'Recentchangeslinked'       => array( 'Seguiment' ),
	'Movepage'                  => array( 'Reanomena' ),
	'Blockme'                   => array( 'Bloca\'m' ),
	'Booksources'               => array( 'Fonts_bibliogràfiques' ),
	'Categories'                => array( 'Categories' ),
	'Export'                    => array( 'Exporta' ),
	'Version'                   => array( 'Versió' ),
	'Allmessages'               => array( 'Missatges', 'MediaWiki' ),
	'Log'                       => array( 'Registre' ),
	'Blockip'                   => array( 'Bloca' ),
	'Undelete'                  => array( 'Restaura' ),
	'Import'                    => array( 'Importa' ),
	'Lockdb'                    => array( 'Bloca_bd' ),
	'Unlockdb'                  => array( 'Desbloca_bd' ),
	'Userrights'                => array( 'Drets' ),
	'MIMEsearch'                => array( 'Cerca_MIME' ),
	'Unwatchedpages'            => array( 'Pàgines_desateses' ),
	'Listredirects'             => array( 'Redireccions' ),
	'Revisiondelete'            => array( 'Esborra_versió' ),
	'Unusedtemplates'           => array( 'Plantilles_no_usades' ),
	'Randomredirect'            => array( 'Redirecció_aleatòria' ),
	'Mypage'                    => array( 'Pàgina_personal' ),
	'Mytalk'                    => array( 'Discussió_personal' ),
	'Mycontributions'           => array( 'Contribucions_pròpies' ),
	'Listadmins'                => array( 'Administradors' ),
	'Popularpages'              => array( 'Pàgines_populars' ),
	'Search'                    => array( 'Cerca' ),
	'Resetpass'                 => array( 'Reinicia_contrasenya' ),
	'Withoutinterwiki'          => array( 'Sense_interwiki' ),
);

$linkTrail = '/^([a-zàèéíòóúç·ïü\']+)(.*)$/sDu';

$messages = array(
# User preference toggles
'tog-underline'               => 'Subratlla els enllaços:',
'tog-highlightbroken'         => 'Formata els enllaços trencats  <a href="" class="new">d\'aquesta manera</a> (altrament, es faria d\'aquesta altra manera<a href="" class="internal">?</a>).',
'tog-justify'                 => 'Alineació justificada dels paràgrafs',
'tog-hideminor'               => 'Amaga les edicions menors en la pàgina de canvis recents',
'tog-extendwatchlist'         => 'Desplega la llista de seguiment per a mostrar tots els canvis afectats',
'tog-usenewrc'                => 'Presentació millorada dels canvis recents (cal JavaScript)',
'tog-numberheadings'          => 'Enumera automàticament els encapçalaments',
'tog-showtoolbar'             => "Mostra la barra d'eines d'edició (cal JavaScript)",
'tog-editondblclick'          => 'Edita les pàgines amb un doble clic (cal JavaScript)',
'tog-editsection'             => "Activa l'edició per seccions mitjançant els enllaços [edita]",
'tog-editsectiononrightclick' => "Habilita l'edició per seccions en clicar amb el botó dret sobre els títols de les seccions (cal JavaScript)",
'tog-showtoc'                 => "Mostrar l'índex de continguts a les pàgines amb més de 3 seccions",
'tog-rememberpassword'        => 'Recorda la contrasenya entre sessions',
'tog-editwidth'               => "Amplia al màxim el quadre d'edició",
'tog-watchcreations'          => 'Vigila les pàgines que he creat',
'tog-watchdefault'            => 'Afegeix les pàgines que edito a la meua llista de seguiment',
'tog-watchmoves'              => 'Afegeix les pàgines que reanomeni a la llista de seguiment',
'tog-watchdeletion'           => 'Afegeix les pàgines que elimini a la llista de seguiment',
'tog-minordefault'            => 'Marca totes les contribucions com a edicions menors per defecte',
'tog-previewontop'            => "Mostra una previsualització abans del quadre d'edició",
'tog-previewonfirst'          => 'Mostra una previsualització en la primera edició',
'tog-nocache'                 => 'Inhabilita la memòria cau de les pàgines',
'tog-enotifwatchlistpages'    => "Notifica'm per correu electrònic dels canvis a les pàgines que vigili",
'tog-enotifusertalkpages'     => "Notifica'm per correu quan hi hagi modificacions a la pàgina de discussió del meu compte d'usuari",
'tog-enotifminoredits'        => "Notifica'm per correu també en casos d'edicions menors",
'tog-enotifrevealaddr'        => "Mostra la meua adreça electrònica en els missatges d'avís per correu",
'tog-shownumberswatching'     => "Mostra el nombre d'usuaris que hi vigilen",
'tog-fancysig'                => 'Signatures netes (sense enllaç automàtic)',
'tog-externaleditor'          => "Utilitza per defecte un editor extern (opció per a experts, requereix la configuració adient de l'ordinador)",
'tog-externaldiff'            => "Utilitza per defecte un altre visualitzador de diferències (opció per a experts, requereix la configuració adient de l'ordinador)",
'tog-showjumplinks'           => "Habilita els enllaços de dreceres d'accessibilitat",
'tog-uselivepreview'          => 'Utilitza la previsualització automàtica (cal JavaScript) (experimental)',
'tog-forceeditsummary'        => "Avisa'm en introduir un camp de resum en blanc",
'tog-watchlisthideown'        => 'Amaga les meues edicions de la llista de seguiment',
'tog-watchlisthidebots'       => 'Amaga de la llista de seguiment les edicions fetes per usuaris bots',
'tog-watchlisthideminor'      => 'Amaga les edicions menors de la llista de seguiment',
'tog-nolangconversion'        => 'Desactiva la conversió de variants',
'tog-ccmeonemails'            => "Envia'm còpies dels missatges que enviï als altres usuaris.",
'tog-diffonly'                => 'Amaga el contingut de la pàgina davall de la taula de diferències',
'tog-showhiddencats'          => 'Mostra les categories ocultes',

'underline-always'  => 'Sempre',
'underline-never'   => 'Mai',
'underline-default' => 'Configuració per defecte del navegador',

'skinpreview' => '(prova)',

# Dates
'sunday'        => 'diumenge',
'monday'        => 'dilluns',
'tuesday'       => 'dimarts',
'wednesday'     => 'dimecres',
'thursday'      => 'dijous',
'friday'        => 'divendres',
'saturday'      => 'dissabte',
'sun'           => 'dg',
'mon'           => 'dl',
'tue'           => 'dt',
'wed'           => 'dc',
'thu'           => 'dj',
'fri'           => 'dv',
'sat'           => 'ds',
'january'       => 'gener',
'february'      => 'febrer',
'march'         => 'març',
'april'         => 'abril',
'may_long'      => 'maig',
'june'          => 'juny',
'july'          => 'juliol',
'august'        => 'agost',
'september'     => 'setembre',
'october'       => 'octubre',
'november'      => 'novembre',
'december'      => 'desembre',
'january-gen'   => 'gener',
'february-gen'  => 'febrer',
'march-gen'     => 'març',
'april-gen'     => 'abril',
'may-gen'       => 'maig',
'june-gen'      => 'juny',
'july-gen'      => 'juliol',
'august-gen'    => 'agost',
'september-gen' => 'setembre',
'october-gen'   => 'octubre',
'november-gen'  => 'novembre',
'december-gen'  => 'desembre',
'jan'           => 'gen',
'feb'           => 'feb',
'mar'           => 'març',
'apr'           => 'abr',
'may'           => 'maig',
'jun'           => 'juny',
'jul'           => 'jul',
'aug'           => 'ago',
'sep'           => 'set',
'oct'           => 'oct',
'nov'           => 'nov',
'dec'           => 'des',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|Categoria|Categories}}',
'category_header'                => 'Pàgines a la categoria «$1»',
'subcategories'                  => 'Subcategories',
'category-media-header'          => 'Contingut multimèdia en la categoria «$1»',
'category-empty'                 => "''Aquesta categoria no té cap pàgina ni fitxer.''",
'hidden-categories'              => '{{PLURAL:$1|Categoria oculta|Categories ocultes}}',
'hidden-category-category'       => 'Categories ocultes', # Name of the category where hidden categories will be listed
'category-subcat-count'          => "{{PLURAL:$2|Aquesta categoria només té la següent subcategoria.|Aquesta categoria conté {{PLURAL:$1|la següent subcategoria|les següents $1 subcategories}}, d'un total de $2.}}",
'category-subcat-count-limited'  => 'Aquesta categoria conté {{PLURAL:$1|la següent subcategoria|les següents $1 subcategories}}.',
'category-article-count'         => "{{PLURAL:$2|Aquesta categoria només té la següent pàgina.|{{PLURAL:$1|La següent pàgina és|Les següents $1 pàgines són}} dins d'aquesta categoria, d'un total de $2.}}",
'category-article-count-limited' => '{{PLURAL:$1|La següent pàgina és|Les següents $1 pàgines són}} dins la categoria actual.',
'category-file-count'            => "{{PLURAL:$2|Aquesta categoria només té el següent fitxer.|{{PLURAL:$1|El següent fitxer és|Els següents $1 fitxers són}} dins d'aquesta categoria, d'un total de $2.}}",
'category-file-count-limited'    => '{{PLURAL:$1|El següent fitxer és|Els següents $1 fitxers són}} dins la categoria actual.',
'listingcontinuesabbrev'         => ' cont.',

'mainpagetext'      => "<big>'''El programari del MediaWiki s'ha instal·lat correctament.'''</big>",
'mainpagedocfooter' => "Consulteu la [http://meta.wikimedia.org/wiki/Help:Contents Guia d'Usuari] per a més informació sobre com utilitzar-lo.

== Per a començar ==

* [http://www.mediawiki.org/wiki/Manual:Configuration_settings Llista de característiques configurables]
* [http://www.mediawiki.org/wiki/Manual:FAQ PMF del MediaWiki]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce Llista de correu (''listserv'') per a anuncis del MediaWiki]",

'about'          => 'Quant a',
'article'        => 'Contingut de la pàgina',
'newwindow'      => '(obre en una nova finestra)',
'cancel'         => 'Anul·la',
'qbfind'         => 'Cerca',
'qbbrowse'       => 'Navega',
'qbedit'         => 'Edita',
'qbpageoptions'  => 'Opcions de pàgina',
'qbpageinfo'     => 'Informació de pàgina',
'qbmyoptions'    => 'Pàgines pròpies',
'qbspecialpages' => 'Pàgines especials',
'moredotdotdot'  => 'Més...',
'mypage'         => 'Pàgina personal',
'mytalk'         => 'Discussió',
'anontalk'       => "Discussió d'aquesta IP",
'navigation'     => 'Navegació',
'and'            => 'i',

# Metadata in edit box
'metadata_help' => 'Metadades:',

'errorpagetitle'    => 'Error',
'returnto'          => 'Torna cap a $1.',
'tagline'           => 'De {{SITENAME}}',
'help'              => 'Ajuda',
'search'            => 'Cerca',
'searchbutton'      => 'Cerca',
'go'                => 'Vés-hi',
'searcharticle'     => 'Vés-hi',
'history'           => 'Historial de canvis',
'history_short'     => 'Historial',
'updatedmarker'     => 'actualitzat des de la darrera visita',
'info_short'        => 'Informació',
'printableversion'  => 'Versió per a impressora',
'permalink'         => 'Enllaç permanent',
'print'             => "Envia aquesta pàgina a la cua d'impressió",
'edit'              => 'Edita',
'create'            => 'Crea',
'editthispage'      => 'Edita la pàgina',
'create-this-page'  => 'Crea aquesta pàgina',
'delete'            => 'Elimina',
'deletethispage'    => 'Elimina la pàgina',
'undelete_short'    => "Restaura {{PLURAL:$1|l'edició eliminada|$1 edicions eliminades}}",
'protect'           => 'Protecció',
'protect_change'    => 'canvia',
'protectthispage'   => 'Protecció de la pàgina',
'unprotect'         => 'Desprotecció',
'unprotectthispage' => 'Desprotecció de la pàgina',
'newpage'           => 'Pàgina nova',
'talkpage'          => 'Discussió de la pàgina',
'talkpagelinktext'  => 'Discussió',
'specialpage'       => 'Pàgina especial',
'personaltools'     => "Eines de l'usuari",
'postcomment'       => 'Envia un comentari',
'articlepage'       => 'Mostra la pàgina',
'talk'              => 'Discussió',
'views'             => 'Vistes',
'toolbox'           => 'Eines',
'userpage'          => "Visualitza la pàgina d'usuari",
'projectpage'       => 'Visualitza la pàgina del projecte',
'imagepage'         => 'Visualitza la pàgina del fitxer multimèdia',
'mediawikipage'     => 'Visualitza la pàgina de missatges',
'templatepage'      => 'Visualitza la pàgina de plantilla',
'viewhelppage'      => "Visualitza la pàgina d'ajuda",
'categorypage'      => 'Visualitza la pàgina de la categoria',
'viewtalkpage'      => 'Visualitza la pàgina de discussió',
'otherlanguages'    => 'En altres llengües',
'redirectedfrom'    => "(S'ha redirigit des de $1)",
'redirectpagesub'   => 'Pàgina de redirecció',
'lastmodifiedat'    => 'Darrera modificació de la pàgina: $2, $1.', # $1 date, $2 time
'viewcount'         => 'Aquesta pàgina ha estat visitada {{PLURAL:$1|una vegada|$1 vegades}}.',
'protectedpage'     => 'Pàgina protegida',
'jumpto'            => 'Dreceres ràpides:',
'jumptonavigation'  => 'navegació',
'jumptosearch'      => 'cerca',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'Quant al projecte {{SITENAME}}',
'aboutpage'            => 'Project:Quant a',
'bugreports'           => "Informes d'errors del programari",
'bugreportspage'       => "Project:Informes d'errors",
'copyright'            => "El contingut és disponible sota els termes d'una llicència $1",
'copyrightpagename'    => '{{SITENAME}}, tots els drets reservats',
'copyrightpage'        => "{{ns:project}}:Drets d'autor",
'currentevents'        => 'Actualitat',
'currentevents-url'    => 'Project:Actualitat',
'disclaimers'          => 'Avís general',
'disclaimerpage'       => 'Project:Avís general',
'edithelp'             => 'Ajuda per a editar pàgines',
'edithelppage'         => "Help:Com s'edita una pàgina",
'faq'                  => 'PMF',
'faqpage'              => 'Project:PMF',
'helppage'             => 'Help:Ajuda',
'mainpage'             => 'Pàgina principal',
'mainpage-description' => 'Pàgina principal',
'policy-url'           => 'Project:Polítiques',
'portal'               => 'Portal comunitari',
'portal-url'           => 'Project:Portal',
'privacy'              => 'Política de privadesa',
'privacypage'          => 'Project:Política de privadesa',

'badaccess'        => 'Error de permisos',
'badaccess-group0' => "No teniu permisos per a executar l'acció que heu sol·licitat.",
'badaccess-group1' => "L'acció que heu sol·licitat es limita als usuaris del grup $1.",
'badaccess-group2' => "L'acció que heu sol·licitat es limita als usuaris d'algun dels grups següents: $1.",
'badaccess-groups' => "L'acció que heu sol·licitat es limita als usuaris d'un dels grups $1.",

'versionrequired'     => 'Cal la versió $1 del MediaWiki',
'versionrequiredtext' => 'Cal la versió $1 del MediaWiki per a utilitzar aquesta pàgina. Vegeu [[Special:Version]]',

'ok'                      => "D'acord",
'retrievedfrom'           => 'Obtingut de «$1»',
'youhavenewmessages'      => 'Teniu $1 ($2).',
'newmessageslink'         => 'nous missatges',
'newmessagesdifflink'     => 'últims canvis',
'youhavenewmessagesmulti' => 'Teniu nous missatges a $1',
'editsection'             => 'edita',
'editold'                 => 'edita',
'viewsourceold'           => 'mostra codi font',
'editsectionhint'         => 'Edita la secció: $1',
'toc'                     => 'Contingut',
'showtoc'                 => 'desplega',
'hidetoc'                 => 'amaga',
'thisisdeleted'           => 'Voleu mostrar o restaurar $1?',
'viewdeleted'             => 'Voleu mostrar $1?',
'restorelink'             => '{{PLURAL:$1|una versió esborrada|$1 versions esborrades}}',
'feedlinks'               => 'Sindicament:',
'feed-invalid'            => 'La subscripció no és vàlida pel tipus de sindicament.',
'feed-unavailable'        => 'Els canals de sindicació no estan disponibles',
'site-rss-feed'           => 'Canal RSS $1',
'site-atom-feed'          => 'Canal Atom $1',
'page-rss-feed'           => '«$1» RSS Feed',
'page-atom-feed'          => 'Canal Atom «$1»',
'red-link-title'          => "$1 (no s'ha escrit encara)",

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Pàgina',
'nstab-user'      => "Pàgina d'usuari",
'nstab-media'     => 'Pàgina de multimèdia',
'nstab-special'   => 'Pàgina especial',
'nstab-project'   => 'Pàgina del projecte',
'nstab-image'     => 'Fitxer',
'nstab-mediawiki' => 'Missatge',
'nstab-template'  => 'Plantilla',
'nstab-help'      => 'Ajuda',
'nstab-category'  => 'Categoria',

# Main script and global functions
'nosuchaction'      => 'No es reconeix aquesta operació',
'nosuchactiontext'  => "El programari wiki que fa servir {{SITENAME}} no reconeix l'operació especificada per l'adreça URL",
'nosuchspecialpage' => 'No es troba la pàgina especial que busqueu',
'nospecialpagetext' => 'La pàgina especial que demaneu no és vàlida. Vegeu la llista de pàgines especials en [[Special:SpecialPages]].',

# General errors
'error'                => 'Error',
'databaseerror'        => "S'ha produït un error en la base de dades",
'dberrortext'          => "S'ha produït un error de sintaxi en una consulta a la base de dades.
Açò podria indicar un error en el programari.
La darrera consulta que s'ha intentat fer ha estat:
<blockquote><tt>$1</tt></blockquote>
des de la funció «<tt>$2</tt>».
L'error de retorn de MySQL ha estat «<tt>$3: $4</tt>».",
'dberrortextcl'        => "S'ha produït un error de sintaxi en una consulta a la base de dades.
La darrera consulta que s'ha intentat fer ha estat:
<blockquote><tt>$1</tt></blockquote>
des de la funció «<tt>$2</tt>».
L'error de retorn de MySQL ha estat «<tt>$3: $4</tt>».",
'noconnect'            => "Ho sentim! Al programari wiki hi ha algun problema tècnic, i no s'ha pogut contactar amb el servidor de la base de dades. <br />
$1",
'nodb'                 => "No s'ha pogut seleccionar la base de dades $1",
'cachederror'          => 'Tot seguit és una còpia provinent de la memòria cau de la pàgina que hi heu demanat i, per això, podria no estar actualitzada.',
'laggedslavemode'      => 'Avís: La pàgina podria mancar de modificacions recents.',
'readonly'             => 'La base de dades es troba bloquejada',
'enterlockreason'      => 'Escriviu una raó pel bloqueig, així com una estimació de quan tindrà lloc el desbloqueig',
'readonlytext'         => "La base de dades està temporalment bloquejada segurament per tasques de manteniment, després de les quals es tornarà a la normalitat.

L'administrador que l'ha bloquejada ha donat aquesta explicació: $1",
'missing-article'      => "La base de dades no ha trobat el text d'una pàgina que hauria d'haver trobat, anomenada «$1» $2.

Normalment això passa perquè s'ha seguit una diferència desactualitzada o un enllaç d'historial a una pàgina que s'ha suprimit.

Si no fos el cas, podríeu haver trobat un error en el programari.
Aviseu-ho llavors a un [[Special:ListUsers/sysop|administrador]], deixant-li clar l'adreça URL causant del problema.",
'missingarticle-rev'   => '(revisió#: $1)',
'missingarticle-diff'  => '(dif: $1, $2)',
'readonly_lag'         => "La base de dades s'ha bloquejat automàticament mentre els servidors esclaus se sincronitzen amb el mestre",
'internalerror'        => 'Error intern',
'internalerror_info'   => 'Error intern: $1',
'filecopyerror'        => "No s'ha pogut copiar el fitxer «$1» com «$2».",
'filerenameerror'      => "No s'ha pogut reanomenar el fitxer «$1» com «$2».",
'filedeleteerror'      => "No s'ha pogut eliminar el fitxer «$1».",
'directorycreateerror' => "No s'ha pogut crear el directori «$1».",
'filenotfound'         => "No s'ha pogut trobar el fitxer «$1».",
'fileexistserror'      => "No s'ha pogut escriure al fitxer «$1»: ja existeix",
'unexpected'           => "S'ha trobat un valor imprevist: «$1»=«$2».",
'formerror'            => "Error: no s'ha pogut enviar les dades del formulari",
'badarticleerror'      => 'Aquesta operació no es pot dur a terme en aquesta pàgina',
'cannotdelete'         => "No s'ha pogut esborrar la pàgina o el fitxer especificat, o potser ja ha estat esborrat per algú altre.",
'badtitle'             => 'El títol no és correcte',
'badtitletext'         => 'El títol de la pàgina que heu introduït no és correcte, és en blanc o conté un enllaç trencat amb un altre projecte. També podria contenir algun caràcter no acceptat als títols de pàgina.',
'perfdisabled'         => "S'ha inhabilitat temporalment aquesta funcionalitat perquè sobrecarrega la base de dades fins al punt d'inutilitzar el programari wiki.",
'perfcached'           => 'Tot seguit es mostren les dades que es troben a la memòria cau, i podria no tenir els últims canvis del dia:',
'perfcachedts'         => 'Tot seguit es mostra les dades que es troben a la memòria cau, la darrera actualització de la qual fou el $1.',
'querypage-no-updates' => "S'ha inhabilitat l'actualització d'aquesta pàgina. Les dades que hi contenen podrien no estar al dia.",
'wrong_wfQuery_params' => 'Paràmetres incorrectes per a wfQuery()<br />
Funció: $1<br />
Consulta: $2',
'viewsource'           => 'Mostra la font',
'viewsourcefor'        => 'per a $1',
'actionthrottled'      => 'Acció limitada',
'actionthrottledtext'  => "Com a mesura per a prevenir la propaganda indiscriminada (spam), no podeu fer aquesta acció tantes vegades en un període de temps tan curt. Torneu-ho a intentar d'ací uns minuts.",
'protectedpagetext'    => 'Aquesta pàgina està protegida i no pot ser editada.',
'viewsourcetext'       => "Podeu visualitzar i copiar la font d'aquesta pàgina:",
'protectedinterface'   => "Aquesta pàgina conté cadenes de text per a la interfície del programari, i és protegida per a previndre'n abusos.",
'editinginterface'     => "'''Avís:''' Esteu editant una pàgina que conté cadenes de text per a la interfície d'aquest programari. Tingueu en compte que els canvis que es fan a aquesta pàgina afecten a l'aparença de la interfície d'altres usuaris. Pel que fa a les traduccions, plantegeu-vos utilitzar la [http://translatewiki.net/wiki/Main_Page?setlang=ca Betawiki], el projecte de traducció de MediaWiki.",
'sqlhidden'            => '(consulta SQL oculta)',
'cascadeprotected'     => "Aquesta pàgina està protegida i no es pot editar perquè està inclosa en {{PLURAL:$1|la següent pàgina, que té|les següents pàgines, que tenen}} activada l'opció de «protecció en cascada»:
$2",
'namespaceprotected'   => "No teniu permís per a editar pàgines en l'espai de noms '''$1'''.",
'customcssjsprotected' => "No teniu permís per a editar aquesta pàgina, perquè conté paràmetres personals d'un altre usuari.",
'ns-specialprotected'  => "No poden editar-se les pàgines en l'espai de noms {{ns:special}}.",
'titleprotected'       => "La creació d'aquesta pàgina està protegida per [[User:$1|$1]].
Els seus motius han estat: «''$2''».",

# Virus scanner
'virus-badscanner'     => 'Mala configuració: antivirus desconegut: <i>$1</i>',
'virus-scanfailed'     => 'escaneig fallit (codi $1)',
'virus-unknownscanner' => 'antivirus desconegut:',

# Login and logout pages
'logouttitle'                => 'Fi de la sessió',
'logouttext'                 => '<strong>Heu finalitzat la vostra sessió.</strong><br />
Podeu continuar utilitzant {{SITENAME}} de forma anònima, o podeu [[Special:UserLogin|iniciar una sessió una altra vegada]] amb el mateix o un altre usuari. 
Tingueu en compte que algunes pàgines poden continuar mostrant-se com si encara estiguéssiu en una sessió, fins que buideu la memòria cau del vostre navegador.',
'welcomecreation'            => "== Us donem la benvinguda, $1! ==

S'ha creat el vostre compte. 
No oblideu de canviar les vostres [[Special:Preferences|preferències de {{SITENAME}}]].",
'loginpagetitle'             => 'Inici de sessió',
'yourname'                   => "Nom d'usuari",
'yourpassword'               => 'Contrasenya',
'yourpasswordagain'          => 'Escriviu una altra vegada la contrasenya',
'remembermypassword'         => 'Recorda la contrasenya entre sessions',
'yourdomainname'             => 'El vostre domini',
'externaldberror'            => "Hi ha hagut una fallida en el servidor d'autenticació externa de la base de dades i no teniu permís per a actualitzar el vostre compte d'accès extern.",
'loginproblem'               => "<strong>S'ha produït un problema en iniciar la sessió.</strong><br />Proveu-ho de nou!",
'login'                      => 'Inici de sessió',
'nav-login-createaccount'    => 'Inicia una sessió / crea un compte',
'loginprompt'                => 'Heu de tenir les galetes habilitades per a poder iniciar una sessió a {{SITENAME}}.',
'userlogin'                  => 'Inicia una sessió / crea un compte',
'logout'                     => 'Finalitza la sessió',
'userlogout'                 => 'Finalitza la sessió',
'notloggedin'                => 'No us heu identificat',
'nologin'                    => 'No teniu un compte? $1.',
'nologinlink'                => 'Crea un compte',
'createaccount'              => 'Crea un compte',
'gotaccount'                 => 'Ja teniu un compte? $1.',
'gotaccountlink'             => 'Inicia una sessió',
'createaccountmail'          => 'per correu electrònic',
'badretype'                  => 'Les contrasenyes que heu introduït no coincideixen.',
'userexists'                 => 'El nom que heu entrat ja és en ús. Escolliu-ne un de diferent.',
'youremail'                  => 'Adreça electrònica *',
'username'                   => "Nom d'usuari:",
'uid'                        => "Identificador d'usuari:",
'prefs-memberingroups'       => 'Membre dels {{PLURAL:$1|grup|grups}}:',
'yourrealname'               => 'Nom real *',
'yourlanguage'               => 'Llengua:',
'yourvariant'                => 'Variant lingüística:',
'yournick'                   => 'Signatura:',
'badsig'                     => 'La signatura que heu inserit no és vàlida; verifiqueu les etiquetes HTML que heu emprat.',
'badsiglength'               => "La signatura és massa llarga.
Ha de tenir menys {{PLURAL:$1|d'$1 càracter|de $1 caràcters}}.",
'email'                      => 'Adreça electrònica',
'prefs-help-realname'        => "* Nom real (opcional): si escolliu donar aquesta informació serà utilitzada per a donar-vos l'atribució de la vostra feina.",
'loginerror'                 => "Error d'inici de sessió",
'prefs-help-email'           => "L'adreça electrònica és opcional, però permet l'enviament d'una nova contrasenya en cas d'oblit de l'actual.
També podeu contactar amb altres usuaris a través de la vostra pàgina d'usuari o de discussió, sense que així calgui revelar la vostra identitat.",
'prefs-help-email-required'  => 'Cal una adreça de correu electrònic.',
'nocookiesnew'               => "S'ha creat el compte d'usuari, però no esteu enregistrat. El projecte {{SITENAME}} usa galetes per enregistrar els usuaris. Si us plau activeu-les, per a poder enregistrar-vos amb el vostre nom d'usuari i la clau.",
'nocookieslogin'             => 'El programari {{SITENAME}} utilitza galetes per enregistrar usuaris. Teniu les galetes desactivades. Activeu-les i torneu a provar.',
'noname'                     => "No heu especificat un nom vàlid d'usuari.",
'loginsuccesstitle'          => "S'ha iniciat la sessió amb èxit",
'loginsuccess'               => 'Heu iniciat la sessió a {{SITENAME}} com a «$1».',
'nosuchuser'                 => 'No hi ha cap usuari anomenat "$1".
Reviseu-ne l\'ortografia, o [[Special:Userlogin/signup|creeu un compte d\'usuari nou]].',
'nosuchusershort'            => 'No hi ha cap usuari anomenat «<nowiki>$1</nowiki>». Comproveu que ho hàgiu escrit correctament.',
'nouserspecified'            => "Heu d'especificar un nom d'usuari.",
'wrongpassword'              => 'La contrasenya que heu introduït és incorrecta. Torneu-ho a provar.',
'wrongpasswordempty'         => "La contrasenya que s'ha introduït estava en blanc. Torneu-ho a provar.",
'passwordtooshort'           => "La contrasenya és massa curta o invàlida.
Ha de tenir un mínim {{PLURAL:$1|d'un caràcter|de $1 caràcters}} i ésser diferent del vostre nom d'usuari.",
'mailmypassword'             => "Envia'm una nova contrasenya per correu electrònic",
'passwordremindertitle'      => 'Nova contrasenya temporal per al projecte {{SITENAME}}',
'passwordremindertext'       => "Algú (vós mateix segurament, des de l'adreça l'IP $1) ha sol·licitat que us enviéssim una nova contrasenya per a iniciar la sessió al projecte {{SITENAME}} ($4).
La contrasenya per a l'usuari «$2» és ara «$3». Si aquesta fou la vostra intenció, ara hauríeu d'iniciar la sessió i canviar la vostra contrasenya.

Si algú altre hagués fet aquesta sol·licitud o si ja haguéssiu recordat la vostra contrasenya i
no volguéssiu canviar-la, ignoreu aquest missatge i continueu utilitzant
la vostra antiga contrasenya.",
'noemail'                    => "No hi ha cap adreça electrònica registrada de l'usuari «$1».",
'passwordsent'               => "S'ha enviat una nova contrasenya a l'adreça electrònica registrada per «$1».
Inicieu una sessió després que la rebeu.",
'blocked-mailpassword'       => 'La vostra adreça IP ha estat blocada. Se us ha desactivat la funció de recuperació de contrasenya per a prevenir abusos.',
'eauthentsent'               => "S'ha enviat un correu electrònic a la direcció especificada. Abans no s'envïi cap altre correu electrònic a aquesta adreça, cal verificar que és realment vostra. Per tant, cal que seguiu les instruccions presents en el correu electrònic que se us ha enviat.",
'throttled-mailpassword'     => "Ja se us ha enviat un recordatori de contrasenya en {{PLURAL:$1|l'última hora|les últimes $1 hores}}. Per a prevenir abusos, només s'envia un recordatori de contrasenya cada {{PLURAL:$1|hora|$1 hores}}.",
'mailerror'                  => "S'ha produït un error en enviar el missatge: $1",
'acct_creation_throttle_hit' => 'Ho sentim, ja teniu $1 comptes creats i no és permès de tenir-ne més.',
'emailauthenticated'         => "S'ha autenticat la vostra adreça electrònica a $1.",
'emailnotauthenticated'      => 'La vostra adreça de correu electrònic <strong>encara no està autenticada</strong>. No rebrà cap missatge de correu electrònic per a cap de les següents funcionalitats.',
'noemailprefs'               => 'Especifiqueu una adreça electrònica per a activar aquestes característiques.',
'emailconfirmlink'           => 'Confirmeu la vostra adreça electrònica',
'invalidemailaddress'        => "No es pot acceptar l'adreça electrònica perquè sembla que té un format no vàlid.
Introduïu una adreça amb un format adequat o bé buideu el camp.",
'accountcreated'             => "S'ha creat el compte",
'accountcreatedtext'         => "S'ha creat el compte d'usuari de $1.",
'createaccount-title'        => "Creació d'un compte a {{SITENAME}}",
'createaccount-text'         => "Algú ha creat un compte d'usuari anomenat $2 al projecte {{SITENAME}}
($4) amb la vostra adreça de correu electrònic. La contrasenya per a l'usuari «$2» és «$3». Hauríeu d'accedir al compte i canviar-vos aquesta contrasenya quan abans millor.

Si no hi teniu cap relació i aquest compte ha estat creat per error, simplement ignoreu el missatge.",
'loginlanguagelabel'         => 'Llengua: $1',

# Password reset dialog
'resetpass'               => 'Reinicia la contrasenya del compte',
'resetpass_announce'      => 'Heu iniciat la sessió amb un codi temporal enviat per correu electrònic. Per a finalitzar-la, heu de definir una nova contrasenya ací:',
'resetpass_text'          => '<!-- Afegiu-hi un text -->',
'resetpass_header'        => 'Reinicia la contrasenya',
'resetpass_submit'        => 'Definiu una contrasenya i inicieu una sessió',
'resetpass_success'       => "S'ha canviat la vostra contrasenya amb èxit! Ara ja podeu iniciar-hi una sessió...",
'resetpass_bad_temporary' => 'La contrasenya temporal no és vàlida. Potser ja havíeu canviat la vostra contrasenya o heu sol·licitat una nova contrasenya temporal.',
'resetpass_forbidden'     => 'No poden canviar-se les contrasenyes',
'resetpass_missing'       => 'No hi ha cap dada de formulari.',

# Edit page toolbar
'bold_sample'     => 'Text en negreta',
'bold_tip'        => 'Text en negreta',
'italic_sample'   => 'Text en cursiva',
'italic_tip'      => 'Text en cursiva',
'link_sample'     => "Títol de l'enllaç",
'link_tip'        => 'Enllaç intern',
'extlink_sample'  => "http://www.example.com títol de l'enllaç",
'extlink_tip'     => 'Enllaç extern (recordeu el prefix http://)',
'headline_sample' => "Text per a l'encapçalament",
'headline_tip'    => 'Encapçalat de secció de 2n nivell',
'math_sample'     => 'Inseriu una fórmula ací',
'math_tip'        => 'Fórmula matemàtica (LaTeX)',
'nowiki_sample'   => 'Inseriu ací text sense format',
'nowiki_tip'      => 'Ignora el format wiki',
'image_sample'    => 'Exemple.jpg',
'image_tip'       => 'Fitxer incrustat',
'media_sample'    => 'Exemple.ogg',
'media_tip'       => 'Enllaç del fitxer',
'sig_tip'         => 'La vostra signatura amb marca horària',
'hr_tip'          => 'Línia horitzontal (feu-la servir amb moderació)',

# Edit pages
'summary'                          => 'Resum',
'subject'                          => 'Tema/capçalera',
'minoredit'                        => 'Aquesta és una edició menor',
'watchthis'                        => 'Vigila aquesta pàgina',
'savearticle'                      => 'Desa la pàgina',
'preview'                          => 'Previsualització',
'showpreview'                      => 'Mostra una previsualització',
'showlivepreview'                  => 'Vista ràpida',
'showdiff'                         => 'Mostra els canvis',
'anoneditwarning'                  => "'''Avís:''' No esteu identificats amb un compte d'usuari. Es mostrarà la vostra adreça IP en l'historial d'aquesta pàgina.",
'missingsummary'                   => "'''Recordatori''': Heu deixat en blanc el resum de l'edició. Si torneu a clicar al botó de desar, l'edició es guardarà sense resum.",
'missingcommenttext'               => 'Introduïu un comentari a continuació.',
'missingcommentheader'             => "'''Recordatori:''' No heu proporcionat un assumpte/encapçalament per al comentari. Si cliqueu al botó Torna a desar, la vostra contribució se desarà sense cap.",
'summary-preview'                  => 'Previsualització del resum',
'subject-preview'                  => 'Previsualització de tema/capçalera',
'blockedtitle'                     => "L'usuari està blocat",
'blockedtext'                      => "<big>'''S'ha procedit al blocatge del vostre compte d'usuari o la vostra adreça IP.'''</big>

El blocatge l'ha dut a terme l'usuari $1.
El motiu donat és ''$2''.

* Inici del blocatge: $8
* Final del blocatge: $6
* Compte blocat: $7

Podeu contactar amb $1 o un dels [[{{MediaWiki:Grouppage-sysop}}|administradors]] per a discutir-ho.

Tingueu en compte que no podeu fer servir el formulari d'enviament de missatges de correu electrònic a cap usuari, a menys que tingueu una adreça de correu vàlida registrada a les vostres [[Special:Preferences|preferències d'usuari]] i no ho tingueu tampoc blocat.

La vostra adreça IP actual és $3, i el número d'identificació del blocatge és #$5. 
Si us plau, incloeu aquestes dades en totes les consultes que feu.",
'autoblockedtext'                  => "La vostra adreça IP ha estat blocada automàticament perquè va ser usada per un usuari actualment bloquejat. Aquest usuari va ser blocat per l'administrador $1. El motiu donat per al bloqueig ha estat:

:''$2''

* Inici del bloqueig: $8
* Final del bloqueig: $6
* Usuari bloquejat: $7

Podeu contactar l'usuari $1 o algun altre dels [[{{MediaWiki:Grouppage-sysop}}|administradors]] per a discutir el bloqueig.

Recordeu que per a poder usar l'opció «Envia un missatge de correu electrònic a aquest usuari» haureu d'haver validat una adreça de correu electrònic a les vostres [[Special:Preferences|preferències]].

El número d'identificació de la vostra adreça IP és $3, i l'ID del bloqueig és #$5. Si us plau, incloeu aquestes dades en totes les consultes que feu.",
'blockednoreason'                  => "no s'ha donat cap motiu",
'blockedoriginalsource'            => "La font de '''$1''' es mostra a sota:",
'blockededitsource'                => "El text de les vostres edicions a '''$1''' es mostra a continuació:",
'whitelistedittitle'               => 'Cal iniciar una sessió per a poder editar-hi',
'whitelistedittext'                => 'Heu de $1 per editar pàgines.',
'confirmedittitle'                 => "Cal una confirmació de l'adreça electrònica per a poder editar",
'confirmedittext'                  => "Heu de confirmar la vostra adreça electrònica abans de poder editar pàgines. Definiu i valideu la vostra adreça electrònica a través de les vostres [[Special:Preferences|preferències d'usuari]].",
'nosuchsectiontitle'               => 'No hi ha cap secció',
'nosuchsectiontext'                => 'Esteu intentant editar una secció que no existeix. Com que no hi ha la secció $1, no es poden desar les vostres edicions.',
'loginreqtitle'                    => 'Cal que inicieu una sessió',
'loginreqlink'                     => 'inicia una sessió',
'loginreqpagetext'                 => 'Heu de ser $1 per a visualitzar altres pàgines.',
'accmailtitle'                     => "S'ha enviat una contrasenya.",
'accmailtext'                      => "S'ha enviat a $2 la contrasenya per a «$1».",
'newarticle'                       => '(Nou)',
'newarticletext'                   => "Heu seguit un enllaç a una pàgina que encara no existeix.
Per a crear-la, comenceu a escriure en l'espai de sota
(vegeu l'[[{{MediaWiki:Helppage}}|ajuda]] per a més informació).
Si sou ací per error, simplement cliqueu al botó «Enrere» del vostre navegador.",
'anontalkpagetext'                 => "----''Aquesta és la pàgina de discussió d'un usuari anònim que encara no ha creat un compte o que no fa servir el seu nom registrat. Per tant, hem de fer servir la seua adreça IP numèrica per a identificar-lo. Una adreça IP pot ser compartida per molts usuaris. Si sou un usuari anònim, i trobeu que us han adreçat comentaris inoportuns, si us plau, [[Special:UserLogin/signup|creeu-vos un compte]], o [[Special:UserLogin|entreu en el vostre compte]] si ja en teniu un, per a evitar futures confusions amb altres usuaris anònims.''",
'noarticletext'                    => 'En aquest moment no hi ha text en aquesta pàgina. Podeu [[Special:Search/{{PAGENAME}}|cercar-ne el títol]] en altres pàgines o [{{fullurl:{{FULLPAGENAME}}|action=edit}} començar a escriure-hi].',
'userpage-userdoesnotexist'        => "Atenció: El compte d'usuari «$1» no està registrat. En principi no hauríeu de crear ni editar aquesta pàgina.",
'clearyourcache'                   => "'''Nota:''' Després de desar, heu de posar al dia la memòria cau del vostre navegador per veure els canvis. '''Mozilla / Firefox / Safari:''' Premeu ''Shift'' mentre cliqueu ''Actualitza'' (Reload), o premeu ''Ctrl+F5'' o ''Ctrl+R'' (''Cmd+R'' en un Mac Apple); '''Internet Explorer:''' premeu ''Ctrl'' mentre cliqueu ''Actualitza'' (Refresh), o premeu ''Ctrl+F5''; '''Konqueror:''': simplement cliqueu el botó ''Recarregar'' (Reload), o premeu ''F5''; '''Opera''' haureu d'esborrar completament la vostra memòria cau (caché) a ''Tools→Preferences''.",
'usercssjsyoucanpreview'           => '<strong>Consell:</strong> Utilitzeu el botó «Mostra previsualització» per probar el vostre nou CSS/JS abans de desar-lo.',
'usercsspreview'                   => "'''Recordeu que esteu previsualitzant el vostre CSS d'usuari.'''
'''Encara no s'ha desat!'''",
'userjspreview'                    => "'''Recordeu que només estau provant/previsualitzant el vostre JavaScript, encara no ho heu desat!'''",
'userinvalidcssjstitle'            => "'''Atenció:''' No existeix l'aparença «$1». Recordeu que les subpàgines personalitzades amb extensions .css i .js utilitzen el títol en minúscules, per exemple, {{ns:user}}:NOM/monobook.css no és el mateix que {{ns:user}}:NOM/Monobook.css.",
'updated'                          => '(Actualitzat)',
'note'                             => '<strong>Nota:</strong>',
'previewnote'                      => "<strong>Açò només és una previsualització, els canvis de la qual encara no s'han desat!</strong>",
'previewconflict'                  => "Aquesta previsualització reflecteix, a l'àrea
d'edició superior, el text tal i com apareixerà si trieu desar-lo.",
'session_fail_preview'             => "<strong>No s'ha pogut processar la vostra edició a causa d'una pèrdua de dades de la sessió.
Si us plau, proveu-ho una altra vegada. Si continués sense funcionar, proveu de [[Special:UserLogout|finalitzar la sessió]] i torneu a iniciar-ne una.</strong>",
'session_fail_preview_html'        => "<strong>Ho sentim, no s'han pogut processar les vostres modificacions a causa d'una pèrdua de dades de la sessió.</strong>

''Com que el projecte {{SITENAME}} té habilitat l'ús de codi HTML cru, s'ha amagat la previsualització com a prevenció contra atacs mitjançant codis JavaScript.''

<strong>Si es tracta d'una contribució legítima, si us plau, intenteu-ho una altra vegada. Si continua havent-hi problemes, [[Special:UserLogout|finalitzeu la sessió]] i torneu a iniciar-ne una.</strong>",
'token_suffix_mismatch'            => "<strong>S'ha rebutjat la vostra edició perquè el vostre client ha fet malbé els caràcters de puntuació en el testimoni d'edició. S'ha rebutjat l'edició per a evitar la corrupció del text de la pàgina. Açò passa a vegades quan s'utilitza un servei web de servidor intermediari anònim amb problemes.</strong>",
'editing'                          => "S'està editant $1",
'editingsection'                   => "S'està editant $1 (secció)",
'editingcomment'                   => "S'està editant $1 (comentari)",
'editconflict'                     => "Conflicte d'edició: $1",
'explainconflict'                  => "Algú més ha canviat aquesta pàgina des que l'heu editada.
L'àrea de text superior conté el text de la pàgina com existeix actualment.
Els vostres canvis es mostren en l'àrea de text inferior.
Haureu de fusionar els vostres canvis en el text existent.
'''Només''' el text de l'àrea superior es desarà quan premeu el botó «Desa la pàgina».",
'yourtext'                         => 'El vostre text',
'storedversion'                    => 'Versió emmagatzemada',
'nonunicodebrowser'                => "<strong>ALERTA: El vostre navegador no és compatible amb unicode, si us plau canvieu-lo abans d'editar cap pàgina: els caràcters que no són ASCII apareixeran en el quadre d'edició com a codis hexadecimals.</strong>",
'editingold'                       => '<strong>AVÍS: Esteu editant una revisió desactualitzada de la pàgina.
Si la deseu, es perdran els canvis que hàgiu fet des de llavors.</strong>',
'yourdiff'                         => 'Diferències',
'copyrightwarning'                 => "Si us plau, tingueu en compte que totes les contribucions per al projecte {{SITENAME}} es consideren com a publicades sota els termes de la llicència $2 (vegeu-ne més detalls a $1). Si no desitgeu la modificació i distribució lliure dels vostres escrits sense el vostre consentiment, no els poseu ací.<br />
A més a més, en enviar el vostre text, doneu fe que és vostra l'autoria, o bé de fonts en el domini públic o recursos lliures similars. Heu de saber que aquest <strong>no</strong> és el cas de la majoria de pàgines que hi ha a Internet.
<strong>No feu servir textos amb drets d'autor sense permís!</strong>",
'copyrightwarning2'                => "Si us plau, tingueu en compte que totes les contribucions al projecte {{SITENAME}} poden ser corregides, alterades o esborrades per altres usuaris. Si no desitgeu la modificació i distribució lliure dels vostres escrits sense el vostre consentiment, no els poseu ací.<br />
A més a més, en enviar el vostre text, doneu fe que és vostra l'autoria, o bé de fonts en el domini públic o altres recursos lliures similars (consulteu $1 per a més detalls).
<strong>No feu servir textos amb drets d'autor sense permís!</strong>",
'longpagewarning'                  => "<strong>ATENCIÓ: Aquesta pàgina fa $1 kB; hi ha navegadors que poden presentar problemes editant pàgines que s'acostin o sobrepassin els 32 kB. Intenteu, si és possible, dividir la pàgina en seccions més petites.</strong>",
'longpageerror'                    => '<strong>ERROR: El text que heu introduït és de $1 kB i  sobrepassa el màxim permès de $2 kB. Per tant, no es desarà.</strong>',
'readonlywarning'                  => '<strong>ADVERTÈNCIA: La base de dades està tancada per manteniment
i no podeu desar les vostres contribucions en aquests moments. podeu retallar i enganxar el codi
en un fitxer de text i desar-lo més tard.</strong>',
'protectedpagewarning'             => '<strong>ATENCIÓ: Aquesta pàgina està bloquejada i només pot ser editada per usuaris administradors.</strong>',
'semiprotectedpagewarning'         => "'''Atenció:''' Aquesta pàgina està bloquejada i només pot ser editada per usuaris registrats.",
'cascadeprotectedwarning'          => "'''Atenció:''' Aquesta pàgina està protegida de forma que només la poden editar els administradors, ja que està inclosa a {{PLURAL:$1|la següent pàgina|les següents pàgines}} amb l'opció de «protecció en cascada» activada:",
'titleprotectedwarning'            => '<strong>ATENCIÓ: Aquesta pàgina està protegida de tal manera que només certs usuaris poden crear-la.</strong>',
'templatesused'                    => 'Aquesta pàgina fa servir les següents plantilles:',
'templatesusedpreview'             => 'Plantilles usades en aquesta previsualització:',
'templatesusedsection'             => 'Plantilles usades en aquesta secció:',
'template-protected'               => '(protegida)',
'template-semiprotected'           => '(semiprotegida)',
'hiddencategories'                 => 'Aquesta pàgina forma part de {{PLURAL:$1|la següent categoria oculta|les següents categories ocultes}}:',
'edittools'                        => "<!-- Es mostrarà als formularis d'edició i de càrrega el text que hi haja després d'aquesta línia. -->",
'nocreatetitle'                    => "S'ha limitat la creació de pàgines",
'nocreatetext'                     => "El projecte {{SITENAME}} ha restringit la possibilitat de crear noves pàgines.
Podeu editar les planes ja existents o bé [[Special:UserLogin|entrar en un compte d'usuari]].",
'nocreate-loggedin'                => 'No teniu permisos per a crear pàgines noves.',
'permissionserrors'                => 'Error de permisos',
'permissionserrorstext'            => 'No teniu permisos per a fer-ho, {{PLURAL:$1|pel següent motiu|pels següents motius}}:',
'permissionserrorstext-withaction' => 'No teniu permís per a $2, {{PLURAL:$1|pel motiu següent|pels motius següents}}:',
'recreate-deleted-warn'            => "'''Avís: Esteu desant una pàgina que ha estat prèviament esborrada.'''

Hauríeu de considerar si és realment necessari continuar editant aquesta pàgina.
A continuació s'ofereix el registre d'esborraments de la pàgina:",

# Parser/template warnings
'expensive-parserfunction-warning'        => "Atenció: Aquesta pàgina conté massa crides a funcions parserfunction complexes.

Actualment hi ha $1 crides i n'haurien de ser menys de $2.",
'expensive-parserfunction-category'       => 'Pàgines amb massa crides de parser function',
'post-expand-template-inclusion-warning'  => "Avís: La mida d'inclusió de la plantilla és massa gran.
No s'inclouran algunes plantilles.",
'post-expand-template-inclusion-category' => "Pàgines on s'excedeix la mida d'inclusió de les plantilles",
'post-expand-template-argument-warning'   => "Avís: Aquesta pàgina conté com a mínim un argument de plantilla que té una mida d'expansió massa llarga.
Se n'han omès els arguments.",
'post-expand-template-argument-category'  => "Pàgines que contenen arguments de plantilla que s'han omès",

# "Undo" feature
'undo-success' => "Pot desfer-se la modificació. Si us plau, reviseu la comparació de sota per a assegurar-vos que és el que voleu fer; llavors deseu els canvis per a finalitzar la desfeta de l'edició.",
'undo-failure' => 'No pot desfer-se la modificació perquè hi ha edicions entre mig que hi entren en conflicte.',
'undo-norev'   => "No s'ha pogut desfer l'edició perquè no existeix o ha estat esborrada.",
'undo-summary' => 'Es desfà la revisió $1 de [[Special:Contributions/$2|$2]] ([[User talk:$2|Discussió]])',

# Account creation failure
'cantcreateaccounttitle' => 'No es pot crear el compte',
'cantcreateaccount-text' => "[[User:$3|$3]] ha bloquejat la creació de comptes des d'aquesta adreça IP ('''$1''').

El motiu donat per $3 és ''$2''",

# History pages
'viewpagelogs'        => "Visualitza els registres d'aquesta pàgina",
'nohistory'           => 'No hi ha un historial de revisions per a aquesta pàgina.',
'revnotfound'         => 'Revisió no trobada',
'revnotfoundtext'     => "No s'ha pogut trobar la revisió antiga de la pàgina que demanàveu.
Reviseu l'URL que heu emprat per a accedir-hi.",
'currentrev'          => 'Revisió actual',
'revisionasof'        => 'Revisió de $1',
'revision-info'       => 'Revisió de $1; $2',
'previousrevision'    => '←Versió més antiga',
'nextrevision'        => 'Versió més nova→',
'currentrevisionlink' => 'Versió actual',
'cur'                 => 'act',
'next'                => 'seg',
'last'                => 'prev',
'page_first'          => 'primera',
'page_last'           => 'última',
'histlegend'          => 'Simbologia: (act) = diferència amb la versió actual,
(prev) = diferència amb la versió anterior, m = edició menor',
'deletedrev'          => '[suprimit]',
'histfirst'           => 'El primer',
'histlast'            => 'El darrer',
'historysize'         => '({{PLURAL:$1|1 octet|$1 octets}})',
'historyempty'        => '(buit)',

# Revision feed
'history-feed-title'          => 'Historial de revisió',
'history-feed-description'    => 'Historial de revisió per a aquesta pàgina del wiki',
'history-feed-item-nocomment' => '$1 a $2', # user at time
'history-feed-empty'          => 'La pàgina demanada no existeix.
Potser ha estat esborrada o reanomenada.
Intenteu [[Special:Search|cercar al mateix wiki]] per a noves pàgines rellevants.',

# Revision deletion
'rev-deleted-comment'         => "(s'ha suprimit el comentari)",
'rev-deleted-user'            => "(s'ha suprimit el nom d'usuari)",
'rev-deleted-event'           => "(s'ha suprimit el registre d'accions)",
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">
Aquesta versió de la pàgina ha estat eliminada dels arxius públics. Vegeu més detalls al [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} registre d\'esborrats].
</div>',
'rev-deleted-text-view'       => '<div class="mw-warning plainlinks">
Aquesta versió de la pàgina ha estat eliminada dels arxius públics. Com a administrador d\'aquest wiki podeu veure-la; vegeu-ne més detalls al [{{fullurl:Special:Log/delete|page={{PAGENAMEE}}}} registre d\'esborrats].
</div>',
'rev-delundel'                => 'mostra/amaga',
'revisiondelete'              => 'Esborrar/restaurar revisions',
'revdelete-nooldid-title'     => 'La revisió objectiu no és vàlida',
'revdelete-nooldid-text'      => "No heu especificat unes revisions objectius per a realitzar aquesta
funció, la revisió especificada no existeix, o bé esteu provant d'amagar l'actual revisió.",
'revdelete-selected'          => '{{PLURAL:$2|Revisió seleccionada|Revisions seleccionades}} de [[:$1]]:',
'logdelete-selected'          => '{{PLURAL:$1|Esdeveniment del registre seleccionat|Esdeveniments del registre seleccionats}}:',
'revdelete-text'              => 'Les versions esborrades es mostraran encara als historial i registres de les pàgines, si bé part del seu contingut serà inaccessible al públic.

Els altres administradors del projecte {{SITENAME}} encara podrien accedir al contingut amagat i restituir-lo de nou mitjançant aquesta mateixa interfície, si no hi ha cap altra restricció addicional pels operadors del lloc web.',
'revdelete-legend'            => 'Defineix restriccions en la visibilitat',
'revdelete-hide-text'         => 'Amaga el text de revisió',
'revdelete-hide-name'         => "Acció d'amagar i objectiu",
'revdelete-hide-comment'      => "Amaga el comentari de l'edició",
'revdelete-hide-user'         => "Amaga el nom d'usuari o la IP de l'editor",
'revdelete-hide-restricted'   => 'Aplica aquestes restriccions als administradors i bloqueja aquesta interfície',
'revdelete-suppress'          => 'Suprimeix també les dades dels administradors',
'revdelete-hide-image'        => 'Amaga el contingut del fitxer',
'revdelete-unsuppress'        => 'Suprimir les restriccions de les revisions restaurades',
'revdelete-log'               => 'Comentari del registre:',
'revdelete-submit'            => 'Aplica a la revisió seleccionada',
'revdelete-logentry'          => "s'ha canviat la visibilitat de la revisió de [[$1]]",
'logdelete-logentry'          => "s'ha canviat la visibilitat de [[$1]]",
'revdelete-success'           => "'''S'ha establert correctament la visibilitat d'aquesta revissió.'''",
'logdelete-success'           => "'''S'ha establert correctament la visibilitat d'aquest element.'''",
'revdel-restore'              => "Canvia'n la visibilitat",
'pagehist'                    => 'Historial',
'deletedhist'                 => "Historial d'esborrat",
'revdelete-content'           => 'el contingut',
'revdelete-summary'           => "el resum d'edició",
'revdelete-uname'             => "el nom d'usuari",
'revdelete-restricted'        => 'ha aplicat restriccions al administradors',
'revdelete-unrestricted'      => 'ha esborrat les restriccions per a administradors',
'revdelete-hid'               => 'ha amagat $1',
'revdelete-unhid'             => 'ha tornat a mostrar $1',
'revdelete-log-message'       => '$1 de {{PLURAL:$2|la revisió|les revisions}}
$2',
'logdelete-log-message'       => "$1 per {{PLURAL:$2|l'esdeveniment|els esdeveniments}} $2",

# Suppression log
'suppressionlog'     => 'Registre de supressió',
'suppressionlogtext' => 'A continuació hi ha una llista de les eliminacions i bloquejos que impliquen un contingut amagat als administradors. Vegeu la [[Special:IPBlockList|llista de bloquejos]] per a consultar la llista de bandejos i bloquejos actualment en curs.',

# History merging
'mergehistory'                     => 'Fusiona els historials de les pàgines',
'mergehistory-header'              => "Aquesta pàgina us permet fusionar les revisions de l'historial d'una pàgina origen en una més nova.
Assegureu-vos que aquest canvi mantindrà la continuïtat històrica de la pàgina.",
'mergehistory-box'                 => 'Fusiona les revisions de dues pàgines:',
'mergehistory-from'                => "Pàgina d'origen:",
'mergehistory-into'                => 'Pàgina de destinació:',
'mergehistory-list'                => "Historial d'edició que es pot fusionar",
'mergehistory-merge'               => "Les revisions següents de [[:$1]] poden fusionar-se en [[:$2]]. Feu servir la columna de botó d'opció per a fusionar només les revisions creades en el moment especificat o anteriors. Teniu en comptes que els enllaços de navegació reiniciaran aquesta columna.",
'mergehistory-go'                  => 'Mostra les edicions que es poden fusionar',
'mergehistory-submit'              => 'Fusiona les revisions',
'mergehistory-empty'               => 'No pot fusionar-se cap revisió.',
'mergehistory-success'             => "$3 {{PLURAL:$3|revisió|revisions}} de [[:$1]] s'han fusionat amb èxit a [[:$2]].",
'mergehistory-fail'                => "No s'ha pogut realitzar la fusió de l'historial, comproveu la pàgina i els paràmetres horaris.",
'mergehistory-no-source'           => "La pàgina d'origen $1 no existeix.",
'mergehistory-no-destination'      => 'La pàgina de destinació $1 no existeix.',
'mergehistory-invalid-source'      => "La pàgina d'origen ha de tenir un títol vàlid.",
'mergehistory-invalid-destination' => 'La pàgina de destinació ha de tenir un títol vàlid.',
'mergehistory-autocomment'         => '[[:$1]] fusionat en [[:$2]]',
'mergehistory-comment'             => '[[:$1]] fusionat en [[:$2]]: $3',

# Merge log
'mergelog'           => 'Registre de fusions',
'pagemerge-logentry' => "s'ha fusionat [[$1]] en [[$2]] (revisions fins a $3)",
'revertmerge'        => 'Desfusiona',
'mergelogpagetext'   => "A sota hi ha una llista de les fusions més recents d'una pàgina d'historial en una altra.",

# Diffs
'history-title'           => 'Historial de versions de «$1»',
'difference'              => '(Diferència entre revisions)',
'lineno'                  => 'Línia $1:',
'compareselectedversions' => 'Compara les versions seleccionades',
'editundo'                => 'desfés',
'diff-multi'              => '(Hi ha {{PLURAL:$1|una revisió intermèdia|$1 revisions intermèdies}})',

# Search results
'searchresults'             => 'Resultats de la cerca',
'searchresulttext'          => 'Per a més informació de les cerques del projecte {{SITENAME}}, aneu a [[{{MediaWiki:Helppage}}|{{int:help}}]].',
'searchsubtitle'            => 'Heu cercat \'\'\'[[:$1]]\'\'\'  ([[Special:Prefixindex/$1|totes les pàgines que comencen amb "$1"]] | [[Special:WhatLinksHere/$1|totes les pàgines que enllacen amb "$1"]])',
'searchsubtitleinvalid'     => 'Per consulta "$1"',
'noexactmatch'              => "'''No hi ha cap pàgina anomenada «$1».''' Si voleu, podeu ajudar [[:$1|creant-la]].",
'noexactmatch-nocreate'     => "'''No hi ha cap pàgina amb títol «$1».'''",
'toomanymatches'            => "S'han retornat masses coincidències. Proveu-ho amb una consulta diferent.",
'titlematches'              => 'Coincidències de títol de la pàgina',
'notitlematches'            => 'No hi ha cap coincidència de títol de pàgina',
'textmatches'               => 'Coincidències de text de pàgina',
'notextmatches'             => 'No hi ha cap coincidència de text de pàgina',
'prevn'                     => '$1 anteriors',
'nextn'                     => '$1 següents',
'viewprevnext'              => 'Vés a ($1) ($2) ($3).',
'search-result-size'        => '$1 ({{PLURAL:$2|1 paraula|$2 paraules}})',
'search-result-score'       => 'Rellevància: $1%',
'search-redirect'           => '(redirigeix a $1)',
'search-section'            => '(secció $1)',
'search-suggest'            => 'Volíeu dir: $1',
'search-interwiki-caption'  => 'Projectes germans',
'search-interwiki-default'  => '$1 resultats:',
'search-interwiki-more'     => '(més)',
'search-mwsuggest-enabled'  => 'amb suggeriments',
'search-mwsuggest-disabled' => 'cap suggeriment',
'search-relatedarticle'     => 'Relacionat',
'mwsuggest-disable'         => 'Inhabilita els suggeriments en AJAX',
'searchrelated'             => 'relacionat',
'searchall'                 => 'tots',
'showingresults'            => 'Tot seguit es {{PLURAL:$1|mostra el resultat|mostren els <b>$1</b> resultats començant pel número <b>$2</b>}}.',
'showingresultsnum'         => 'Tot seguit es {{PLURAL:$3|llista el resultat|llisten els <b>$3</b> resultats començant pel número <b>$2</b>}}.',
'showingresultstotal'       => "A continuació {{PLURAL:$3|es mostra el resultat '''$1''' de '''$3'''|es mostren els resultats '''$1 - $2''' de '''$3'''}}",
'nonefound'                 => "'''Nota''': Només se cerca en alguns espais de noms per defecte. Proveu d'afegir el prefix ''all:'' a la vostra consulta per a cercar a tot el contingut (incloent-hi les pàgines de discussió, les plantilles, etc.), o feu servir l'espai de noms on vulgueu cercar com a prefix.",
'powersearch'               => 'Cerca avançada',
'powersearch-legend'        => 'Cerca avançada',
'powersearch-ns'            => 'Cerca als espais de noms:',
'powersearch-redir'         => 'Mostra redireccions',
'powersearch-field'         => 'Cerca',
'search-external'           => 'Cerca externa',
'searchdisabled'            => 'La cerca dins el projecte {{SITENAME}} està inhabilitada. Mentrestant, podeu cercar a través de Google, però tingueu en compte que la seua base de dades no estarà actualitzada.',

# Preferences page
'preferences'              => 'Preferències',
'mypreferences'            => 'Preferències',
'prefs-edits'              => "Nombre d'edicions:",
'prefsnologin'             => 'No heu iniciat cap sessió',
'prefsnologintext'         => 'Heu d\'estar <span class="plainlinks">[{{fullurl:Special:Userlogin|returnto=$1}} autenticats]</span> per a seleccionar les preferències d\'usuari.',
'prefsreset'               => "Les preferències han estat respostes des d'emmagatzematge.",
'qbsettings'               => 'Preferències de "Quickbar"',
'qbsettings-none'          => 'Cap',
'qbsettings-fixedleft'     => 'Fixa a la esquerra',
'qbsettings-fixedright'    => 'Fixa a la dreta',
'qbsettings-floatingleft'  => "Surant a l'esquerra",
'qbsettings-floatingright' => 'Surant a la dreta',
'changepassword'           => 'Canvia la contrasenya',
'skin'                     => 'Aparença',
'math'                     => 'Com es mostren les fórmules',
'dateformat'               => 'Format de la data',
'datedefault'              => 'Cap preferència',
'datetime'                 => 'Data i hora',
'math_failure'             => "No s'ha pogut entendre",
'math_unknown_error'       => 'error desconegut',
'math_unknown_function'    => 'funció desconeguda',
'math_lexing_error'        => 'error de lèxic',
'math_syntax_error'        => 'error de sintaxi',
'math_image_error'         => "Hi ha hagut una errada en la conversió cap el format PNG; verifiqueu la instal·lació de ''Latex'', ''dvips'', ''gs'' i ''convert''.",
'math_bad_tmpdir'          => 'No ha estat possible crear el directori temporal de math o escriure-hi dins.',
'math_bad_output'          => "No ha estat possible crear el directori d'eixida de math o escriure-hi dins.",
'math_notexvc'             => "No s'ha trobat el fitxer executable ''texvc''; si us plau, vegeu math/README per a configurar-lo.",
'prefs-personal'           => "Perfil d'usuari",
'prefs-rc'                 => 'Canvis recents',
'prefs-watchlist'          => 'Llista de seguiment',
'prefs-watchlist-days'     => 'Nombre de dies per mostrar en la llista de seguiment:',
'prefs-watchlist-edits'    => 'Nombre de modificacions a mostrar en una llista estesa de seguiment:',
'prefs-misc'               => 'Altres preferències',
'saveprefs'                => 'Desa les preferències',
'resetprefs'               => 'Esborra els canvis no guardats',
'oldpassword'              => 'Contrasenya antiga',
'newpassword'              => 'Contrasenya nova',
'retypenew'                => 'Torneu a escriure la nova contrasenya:',
'textboxsize'              => 'Dimensions de la caixa de text',
'rows'                     => 'Files',
'columns'                  => 'Columnes',
'searchresultshead'        => 'Preferències de la cerca',
'resultsperpage'           => 'Resultats a mostrar per pàgina',
'contextlines'             => 'Línies a mostrar per resultat',
'contextchars'             => 'Caràcters de context per línia',
'stub-threshold'           => 'Límit per a formatar l\'enllaç com <a href="#" class="stub">esborrany</a> (en octets):',
'recentchangesdays'        => 'Dies a mostrar en els canvis recents:',
'recentchangescount'       => 'Nombre de títols en canvis recents',
'savedprefs'               => "S'han desat les vostres preferències",
'timezonelegend'           => 'Fus horari',
'timezonetext'             => "¹El nombre d'hores de diferència entre la vostra hora local i la del servidor (UTC).",
'localtime'                => 'Hora local',
'timezoneoffset'           => 'Diferència',
'servertime'               => 'Hora del servidor',
'guesstimezone'            => 'Omple-ho des del navegador',
'allowemail'               => "Habilita el correu electrònic des d'altres usuaris",
'prefs-searchoptions'      => 'Preferències de la cerca',
'prefs-namespaces'         => 'Espais de noms',
'defaultns'                => 'Busca per defecte en els següents espais de noms:',
'default'                  => 'per defecte',
'files'                    => 'Fitxers',

# User rights
'userrights'                  => "Gestió dels permisos d'usuari", # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => "Gestiona els grups d'usuari",
'userrights-user-editname'    => "Introduïu un nom d'usuari:",
'editusergroup'               => "Edita els grups d'usuaris",
'editinguser'                 => "S'està canviant els permisos de l'usuari '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup'    => "Edita els grups d'usuaris",
'saveusergroups'              => "Desa els grups d'usuari",
'userrights-groupsmember'     => 'Membre de:',
'userrights-groups-help'      => "Podeu modificar els grups als quals pertany aquest usuari.
* Els requadres marcats indiquen que l'usuari és dins del grup.
* Els requadres sense macar indiquen que l'usuari no hi pertany.
* Un asterisc (*) indica que no el podeu treure del grup una vegada l'hàgiu afegit o viceversa.",
'userrights-reason'           => 'Motiu del canvi:',
'userrights-no-interwiki'     => "No teniu permisos per a editar els permisos d'usuari d'altres wikis.",
'userrights-nodatabase'       => 'La base de dades $1 no existeix o no és local.',
'userrights-nologin'          => "Heu [[Special:UserLogin|d'iniciar una sessió]] amb un compte d'administrador per a poder assignar permisos d'usuari.",
'userrights-notallowed'       => "El vostre compte no té permisos per a assignar permisos d'usuari.",
'userrights-changeable-col'   => 'Grups que podeu canviar',
'userrights-unchangeable-col' => 'Grups que no podeu canviar',

# Groups
'group'               => 'Grup:',
'group-user'          => 'Usuaris',
'group-autoconfirmed' => 'Usuaris autoconfirmats',
'group-bot'           => 'bots',
'group-sysop'         => 'administradors',
'group-bureaucrat'    => 'buròcrates',
'group-suppress'      => 'Oversights',
'group-all'           => '(tots)',

'group-user-member'          => 'Usuari',
'group-autoconfirmed-member' => 'Usuari autoconfirmat',
'group-bot-member'           => 'bot',
'group-sysop-member'         => 'administrador',
'group-bureaucrat-member'    => 'buròcrata',
'group-suppress-member'      => 'Oversight',

'grouppage-user'          => '{{ns:project}}:Usuaris',
'grouppage-autoconfirmed' => '{{ns:project}}:Usuaris autoconfirmats',
'grouppage-bot'           => '{{ns:project}}:Bots',
'grouppage-sysop'         => '{{ns:project}}:Administradors',
'grouppage-bureaucrat'    => '{{ns:project}}:Buròcrates',
'grouppage-suppress'      => '{{ns:project}}:Oversight',

# Rights
'right-read'                 => 'Llegir pàgines',
'right-edit'                 => 'Editar pàgines',
'right-createpage'           => 'Crear pàgines (que no són de discussió)',
'right-createtalk'           => 'Crear pàgines de discussió',
'right-createaccount'        => 'Crear nous comptes',
'right-minoredit'            => 'Marcar les edicions com a menors',
'right-move'                 => 'Moure pàgines',
'right-move-subpages'        => 'Moure pàgines amb les seves subpàgines',
'right-suppressredirect'     => 'No crear redireccions quan es reanomena una pàgina',
'right-upload'               => 'Carregar fitxers',
'right-reupload'             => "Carregar al damunt d'un fitxer existent",
'right-reupload-own'         => "Carregar al damunt d'un fitxer que havia carregat el propi usuari",
'right-reupload-shared'      => 'Carregar localment fitxers amb un nom usat en el repostori multimèdia compartit',
'right-upload_by_url'        => "Carregar un fitxer des de l'adreça URL",
'right-purge'                => 'Purgar la memòria cau del lloc web sense pàgina de confirmació',
'right-autoconfirmed'        => 'Editar pàgines semiprotegides',
'right-bot'                  => 'Ésser tractat com a procés automatitzat',
'right-nominornewtalk'       => "Les edicions menors en pàgines de discussió d'usuari no generen l'avís de nous missatges",
'right-apihighlimits'        => "Utilitza límits més alts en les consultes a l'API",
'right-writeapi'             => "Fer servir l'escriptura a l'API",
'right-delete'               => 'Esborrar pàgines',
'right-bigdelete'            => 'Esborrar pàgines amb historials grans',
'right-deleterevision'       => 'Esborrar i restaurar versions específiques de pàgines',
'right-deletedhistory'       => 'Veure els historials esborrats sense consultar-ne el text',
'right-browsearchive'        => 'Cercar pàgines esborrades',
'right-undelete'             => 'Restaurar pàgines esborrades',
'right-suppressrevision'     => 'Revisar i restaurar les versions amagades als administradors',
'right-suppressionlog'       => 'Veure registres privats',
'right-block'                => "Blocar altres usuaris per a impedir-los l'edició",
'right-blockemail'           => 'Impedir que un usuari envii correu electrònic',
'right-hideuser'             => "Blocar un nom d'usuari amagant-lo del públic",
'right-ipblock-exempt'       => "Evitar blocatges d'IP, de rang i automàtics",
'right-proxyunbannable'      => 'Evitar els blocatges automàtics a proxies',
'right-protect'              => 'Canviar el nivell de protecció i editar pàgines protegides',
'right-editprotected'        => 'Editar pàgines protegides (sense protecció de cascada)',
'right-editinterface'        => "Editar la interfície d'usuari",
'right-editusercssjs'        => "Editar els fitxer de configuració CSS i JS d'altres usuaris",
'right-rollback'             => "Revertir ràpidament l'últim editor d'una pàgina particular",
'right-markbotedits'         => 'Marcar les reversions com a edicions de bot',
'right-noratelimit'          => "No es veu afectat pels límits d'accions.",
'right-import'               => "Importar pàgines d'altres wikis",
'right-importupload'         => "Importar pàgines carregant-les d'un fitxer",
'right-patrol'               => 'Marcar com a patrullades les edicions',
'right-autopatrol'           => 'Que les edicions pròpies es marquin automàticament com a patrullades',
'right-patrolmarks'          => 'Veure quins canvis han estat patrullats',
'right-unwatchedpages'       => 'Veure la llista de les pàgines no vigilades',
'right-trackback'            => 'Trametre un trackback',
'right-mergehistory'         => "Fusionar l'historial de les pàgines",
'right-userrights'           => 'Editar els drets dels usuaris',
'right-userrights-interwiki' => "Editar els drets dels usuaris d'altres wikis",
'right-siteadmin'            => 'Blocar i desblocar la base de dades',

# User rights log
'rightslog'      => "Registre dels permisos d'usuari",
'rightslogtext'  => "Aquest és un registre de canvis dels permisos d'usuari.",
'rightslogentry' => "heu modificat els drets de l'usuari «$1» del grup $2 al de $3",
'rightsnone'     => '(cap)',

# Recent changes
'nchanges'                          => '$1 {{PLURAL:$1|canvi|canvis}}',
'recentchanges'                     => 'Canvis recents',
'recentchangestext'                 => 'Seguiu els canvis recents del projecte {{SITENAME}} en aquesta pàgina.',
'recentchanges-feed-description'    => 'Segueix en aquest canal els canvis més recents del wiki.',
'rcnote'                            => 'A continuació hi ha {{PLURAL:$1|el darrer canvi|els darrers <strong>$1</strong> canvis}} en {{PLURAL:$2|el darrer dia|els darrers <strong>$2</strong> dies}}, actualitzats a les $5 del $4.',
'rcnotefrom'                        => 'A sota hi ha els canvis des de <b>$2</b> (es mostren fins <b>$1</b>).',
'rclistfrom'                        => 'Mostra els canvis nous des de $1',
'rcshowhideminor'                   => '$1 edicions menors',
'rcshowhidebots'                    => '$1 bots',
'rcshowhideliu'                     => '$1 usuaris identificats',
'rcshowhideanons'                   => '$1 usuaris anònims',
'rcshowhidepatr'                    => '$1 edicions supervisades',
'rcshowhidemine'                    => '$1 edicions pròpies',
'rclinks'                           => 'Mostra els darrers $1 canvis en els darrers $2 dies<br />$3',
'diff'                              => 'dif',
'hist'                              => 'hist',
'hide'                              => 'amaga',
'show'                              => 'mostra',
'minoreditletter'                   => 'm',
'newpageletter'                     => 'N',
'boteditletter'                     => 'b',
'number_of_watching_users_pageview' => '[{{PLURAL:$1|Un usuari vigila|$1 usuaris vigilen}} aquesta pàgina]',
'rc_categories'                     => 'Limita a les categories (separades amb "|")',
'rc_categories_any'                 => 'Qualsevol',
'newsectionsummary'                 => '/* $1 */ secció nova',

# Recent changes linked
'recentchangeslinked'          => "Seguiment d'enllaços",
'recentchangeslinked-title'    => 'Canvis relacionats amb «$1»',
'recentchangeslinked-noresult' => 'No ha hagut cap canvi a les pàgines enllaçades durant el període de temps.',
'recentchangeslinked-summary'  => "A continuació trobareu una llista dels canvis recents a les pàgines enllaçades des de la pàgina donada (o entre els membres d'una categoria especificada).
Les pàgines de la vostra [[Special:Watchlist|llista de seguiment]] apareixen en '''negreta'''.",
'recentchangeslinked-page'     => 'Nom de la pàgina:',
'recentchangeslinked-to'       => 'Mostra els canvis de les pàgines enllaçades amb la pàgina donada',

# Upload
'upload'                      => 'Carrega',
'uploadbtn'                   => 'Carrega un fitxer',
'reupload'                    => 'Carrega de nou',
'reuploaddesc'                => 'Torna al formulari per apujar.',
'uploadnologin'               => 'No heu iniciat una sessió',
'uploadnologintext'           => "Heu d'[[Special:UserLogin|iniciar una sessió]]
per a penjar-hi fitxers.",
'upload_directory_missing'    => "No s'ha trobat el directori de càrrega ($1) i tampoc no ha pogut ser creat pel servidor web.",
'upload_directory_read_only'  => 'El servidor web no pot escriure al directori de càrrega ($1)',
'uploaderror'                 => "S'ha produït un error en l'intent de carregar",
'uploadtext'                  => "Feu servir el formulari de sota per a carregar fitxers.
Per a visualitzar o cercar fitxers que s'hagen carregat prèviament, aneu a la [[Special:ImageList|llista de fitxers carregats]]. Les càrregues es registren en el [[Special:Log/upload|registre de càrregues]] i els fitxers esborrats en el [[Special:Log/delete|registre d'esborrats]].

Per a incloure una imatge en una pàgina, feu un enllaç en una de les formes següents:
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:Fitxer.jpg]]</nowiki></tt>''' per a usar la versió completa del fitxer;
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:Fitxer.png|200px|thumb|esquerra|text alternatiu]]</nowiki></tt>''' per una presentació de 200 píxels d'amplada en un requadre justificat a l'esquerra amb \"text alternatiu\" com a descripció;
* '''<tt><nowiki>[[</nowiki>{{ns:media}}<nowiki>:Fitxer.ogg]]</nowiki></tt>''' per a enllaçar directament amb un fitxer de so.",
'upload-permitted'            => 'Tipus de fitxer permesos: $1.',
'upload-preferred'            => 'Tipus de fitxer preferits: $1.',
'upload-prohibited'           => 'Tipus de fitxer prohibits: $1.',
'uploadlog'                   => 'registre de càrregues',
'uploadlogpage'               => 'Registre de càrregues',
'uploadlogpagetext'           => "A sota hi ha una llista dels fitxers que s'han carregat més recentment.
Vegeu la [[Special:NewImages|galeria de nous fitxers]] per a una presentació més visual.",
'filename'                    => 'Nom de fitxer',
'filedesc'                    => 'Resum',
'fileuploadsummary'           => 'Resum:',
'filestatus'                  => "Situació dels drets d'autor:",
'filesource'                  => 'Font:',
'uploadedfiles'               => 'Fitxers carregats',
'ignorewarning'               => 'Ignora qualsevol avís i desa el fitxer igualment',
'ignorewarnings'              => 'Ignora qualsevol avís',
'minlength1'                  => "Els noms de fitxer han de ser de com a mínim d'una lletra.",
'illegalfilename'             => 'El nom del fitxer «$1» conté caràcters que no estan permesos en els títols de pàgines. Si us plau, canvieu el nom al fitxer i torneu a carregar-lo.',
'badfilename'                 => 'El nom de la imatge s\'ha canviat a "$1".',
'filetype-badmime'            => 'Els fitxers del tipus MIME «$1» no poden penjar-se.',
'filetype-unwanted-type'      => "Els fitxers del tipus «'''.$1'''» no són desitjats. {{PLURAL:$3|Es prefereix el tipus de fitxer|Els tipus de fitxer preferits són}} $2.",
'filetype-banned-type'        => "Els fitxers del tipus «'''.$1'''» no estan permesos. {{PLURAL:$3|Només s'admeten els fitxers del tipus|Els tipus de fitxer permesos són}} $2.",
'filetype-missing'            => 'El fitxer no té extensió (com ara «.jpg»).',
'large-file'                  => 'Els fitxers importants no haurien de ser més grans de $1; aquest fitxer ocupa $2.',
'largefileserver'             => 'Aquest fitxer és més gran del que el servidor permet.',
'emptyfile'                   => 'El fitxer que heu carregat sembla estar buit. Açò por ser degut a un mal caràcter en el nom del fitxer. Si us plau, reviseu si realment voleu carregar aquest arxiu.',
'fileexists'                  => 'Ja hi existeix un fitxer amb aquest nom, si us plau, verifiqueu <strong><tt>$1</tt></strong> si no esteu segurs de voler substituir-lo.',
'filepageexists'              => "La pàgina de descripció d'aquest fitxer ja ha estat creada (<strong><tt>$1</tt></strong>), però de moment no hi ha cap arxiu amb aquest nom. La descripció que heu posat no apareixerà a la pàgina de descripció. Si voleu que hi aparegui haureu d'editar-la manualment.",
'fileexists-extension'        => 'Ja existeix un fitxer amb un nom semblant:<br />
Nom del fitxer que es puja: <strong><tt>$1</tt></strong><br />
Nom del fitxer existent: <strong><tt>$2</tt></strong><br />
Si us plau, trieu un nom diferent.',
'fileexists-thumb'            => "<center>'''Fitxer existent'''</center>",
'fileexists-thumbnail-yes'    => 'Aquest fitxer sembla ser una imatge en mida reduïda (<em>miniatura</em>). Comproveu si us plau el fitxer <strong><tt>$1</tt></strong>.<br />
Si el fitxer és la mateixa imatge a mida original, no cal carregar cap miniatura més.',
'file-thumbnail-no'           => 'El nom del fitxer comença per <strong><tt>$1</tt></strong>.
Sembla ser una imatge de mida reduïda <i>(miniatura)</i>.
Si teniu la imatge en resolució completa, pugeu-la, sinó mireu de canviar-li el nom, si us plau.',
'fileexists-forbidden'        => 'Ja hi existeix un fitxer amb aquest nom; si us plau, torneu enrere i carregueu aquest fitxer sota un altre nom. [[Image:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => 'Ja hi ha un fitxer amb aquest nom al fons comú de fitxers.
Si us plau, si encara desitgeu carregar el vostre fitxer, torneu enrera i carregueu-ne una còpia amb un altre nom. [[Image:$1|thumb|center|$1]]',
'file-exists-duplicate'       => 'Aquest fitxer és un duplicat {{PLURAL:$1|del fitxer |dels següents fitxers:}}',
'successfulupload'            => "El fitxer s'ha carregat amb èxit",
'uploadwarning'               => 'Avís de càrrega',
'savefile'                    => 'Desa el fitxer',
'uploadedimage'               => '"[[$1]]" carregat.',
'overwroteimage'              => "s'ha penjat una nova versió de «[[$1]]»",
'uploaddisabled'              => "S'ha inhabilitat la càrrega",
'uploaddisabledtext'          => "S'ha inhabilitat la càrrega de fitxers.",
'uploadscripted'              => 'Aquest fitxer conté codi HTML o de seqüències que pot ser interpretat equivocadament per un navegador.',
'uploadcorrupt'               => 'El fitxer està corrupte o té una extensió incorrecte. Reviseu-lo i torneu-lo a pujar.',
'uploadvirus'                 => 'El fitxer conté un virus! Detalls: $1',
'sourcefilename'              => 'Nom del fitxer font:',
'destfilename'                => 'Nom del fitxer de destinació:',
'upload-maxfilesize'          => 'Mida màxima de fitxer: $1',
'watchthisupload'             => 'Vigila aquesta pàgina',
'filewasdeleted'              => "Prèviament es va carregar un fitxer d'aquest nom i després va ser esborrat. Hauríeu de verificar $1 abans de procedir a carregar-lo una altra vegada.",
'upload-wasdeleted'           => "'''Atenció: Esteu carregant un fitxer que s'havia eliminat abans.'''

Hauríeu de considerar si és realment adequat continuar carregant aquest fitxer, perquè potser també acaba eliminat.
A continuació teniu el registre d'eliminació per a que pugueu comprovar els motius que van portar a la seua eliminació:",
'filename-bad-prefix'         => 'El nom del fitxer que esteu penjant comença amb <strong>«$1»</strong>, que és un nom no descriptiu que les càmeres digitals normalment assignen de forma automàtica. Trieu un de més descriptiu per al vostre fitxer.',

'upload-proto-error'      => 'El protocol és incorrecte',
'upload-proto-error-text' => 'Per a les càrregues remotes cal que els URL comencin amb <code>http://</code> o <code>ftp://</code>.',
'upload-file-error'       => "S'ha produït un error intern",
'upload-file-error-text'  => "S'ha produït un error de càrrega desconegut quan s'intentava crear un fitxer temporal al servidor. Poseu-vos en contacte amb un [[Special:ListUsers/sysop|administrador]].",
'upload-misc-error'       => "S'ha produït un error de càrrega desconegut",
'upload-misc-error-text'  => "S'ha produït un error desconegut durant la càrrega. Verifiqueu que l'URL és vàlid i accessible, i torneu-ho a provar. Si el problema persisteix, adreceu-vos a un [[Special:ListUsers/sysop|administrador]].",

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => "No s'ha pogut accedir a l'URL",
'upload-curl-error6-text'  => "No s'ha pogut accedir a l'URL que s'ha proporcionat. Torneu a comprovar que sigui correcte i que el lloc estigui funcionant.",
'upload-curl-error28'      => "S'ha excedit el temps d'espera de la càrrega",
'upload-curl-error28-text' => "El lloc ha trigat massa a respondre. Comproveu que està funcionant, espereu una estona i torneu-ho a provar. Podeu mirar d'intentar-ho quan hi hagi menys trànsit a la xarxa.",

'license'            => 'Llicència:',
'nolicense'          => "No se n'ha seleccionat cap",
'license-nopreview'  => '(La previsualització no està disponible)',
'upload_source_url'  => ' (un URL vàlid i accessible públicament)',
'upload_source_file' => ' (un fitxer en el vostre ordinador)',

# Special:ImageList
'imagelist-summary'     => "Aquesta pàgina especial mostra tots els fitxers carregats.
Per defecte, els darrers en ser carregats apareixen al principi de la llista.
Clicant al capdamunt de les columnes podeu canviar-ne l'ordenació.",
'imagelist_search_for'  => "Cerca el nom d'un fitxer de medis:",
'imgfile'               => 'fitxer',
'imagelist'             => 'Llista de fitxers',
'imagelist_date'        => 'Data',
'imagelist_name'        => 'Nom',
'imagelist_user'        => 'Usuari',
'imagelist_size'        => 'Mida (octets)',
'imagelist_description' => 'Descripció',

# Image description page
'filehist'                       => 'Historial del fitxer',
'filehist-help'                  => 'Cliqueu una data/hora per veure el fitxer tal com era aleshores.',
'filehist-deleteall'             => 'elimina-ho tot',
'filehist-deleteone'             => 'elimina',
'filehist-revert'                => 'reverteix',
'filehist-current'               => 'actual',
'filehist-datetime'              => 'Data/hora',
'filehist-user'                  => 'Usuari',
'filehist-dimensions'            => 'Dimensions',
'filehist-filesize'              => 'Mida del fitxer',
'filehist-comment'               => 'Comentari',
'imagelinks'                     => 'Enllaços a la imatge',
'linkstoimage'                   => '{{PLURAL:$1|La següent pàgina enllaça|Les següents pàgines enllacen}} a aquesta imatge:',
'nolinkstoimage'                 => 'No hi ha pàgines que enllacin aquesta imatge.',
'morelinkstoimage'               => 'Visualitza [[Special:WhatLinksHere/$1|més enllaços]] que porten al fitxer.',
'redirectstofile'                => '{{PLURAL:$1|El fitxer següent redirigeix cap aquest fitxer|Els següents $1 fitxers redirigeixen cap aquest fitxer:}}',
'duplicatesoffile'               => "{{PLURAL:$1|Aquest fitxer és un duplicat de|A continuació s'indiquen els $1 duplicats d'aquest fitxer:}}",
'sharedupload'                   => 'Aquest fitxer està compartit i poden utilitzar-lo altres projectes.',
'shareduploadwiki'               => 'Consulteu $1 per a més informació.',
'shareduploadwiki-desc'          => 'La descripció a la $1 del repositori compartit es mostra a continuació.',
'shareduploadwiki-linktext'      => 'pàgina de descripció del fitxer',
'shareduploadduplicate'          => "El fitxer és un duplicat de $1 d'un repositori compartit.",
'shareduploadduplicate-linktext' => 'un altre fitxer',
'shareduploadconflict'           => 'El fitxer té el mateix nom que té $1 del repositori compartit.',
'shareduploadconflict-linktext'  => 'un altre fitxer',
'noimage'                        => 'No existeix cap fitxer amb aquest nom, però podeu $1.',
'noimage-linktext'               => "Carrega'n una",
'uploadnewversion-linktext'      => "Carrega una nova versió d'aquest fitxer",
'imagepage-searchdupe'           => 'Cerca fitxers duplicats',

# File reversion
'filerevert'                => 'Reverteix $1',
'filerevert-legend'         => 'Reverteix el fitxer',
'filerevert-intro'          => "Esteu revertint '''[[Media:$1|$1]]''' a la [$4 versió de  $3, $2].",
'filerevert-comment'        => 'Comentari:',
'filerevert-defaultcomment' => "S'ha revertit a la versió com de $2, $1",
'filerevert-submit'         => 'Reverteix',
'filerevert-success'        => "'''[[Media:$1|$1]]''' ha estat revertit a la [$4 versió de $3, $2].",
'filerevert-badversion'     => "No hi ha cap versió local anterior d'aquest fitxer amb la marca horària que es proporciona.",

# File deletion
'filedelete'                  => 'Suprimeix $1',
'filedelete-legend'           => 'Suprimeix el fitxer',
'filedelete-intro'            => "Esteu eliminant '''[[Media:$1|$1]]'''.",
'filedelete-intro-old'        => "Esteu eliminant la versió de '''[[Media:$1|$1]]''' com de [$4 $3, $2].",
'filedelete-comment'          => 'Comentari:',
'filedelete-submit'           => 'Suprimeix',
'filedelete-success'          => "'''$1''' s'ha eliminat.",
'filedelete-success-old'      => "<span class=\"plainlinks\">La versió de '''[[Media:\$1|\$1]]''' s'ha eliminat el \$2 a les \$3.</span>",
'filedelete-nofile'           => "'''$1''' no existeix.",
'filedelete-nofile-old'       => "No hi ha cap versió arxivada de '''$1''' amb els atributs especificats.",
'filedelete-iscurrent'        => "Esteu provant de suprimir la versió més recent d'aquest fitxer. Revertiu a una versió més antiga abans.",
'filedelete-otherreason'      => 'Motius alternatius/addicionals:',
'filedelete-reason-otherlist' => 'Altres motius',
'filedelete-reason-dropdown'  => "*Motius d'eliminació comuns
** Violació dels drets d'autor / copyright
** Fitxer duplicat",
'filedelete-edit-reasonlist'  => "Edita els motius d'eliminació",

# MIME search
'mimesearch'         => 'Cerca per MIME',
'mimesearch-summary' => 'Aquesta pàgina habilita el filtratge de fitxers per llur tipus MIME. Contingut: contenttype/subtype, ex. <tt>image/jpeg</tt>.',
'mimetype'           => 'Tipus MIME:',
'download'           => 'baixada',

# Unwatched pages
'unwatchedpages' => 'Pàgines desateses',

# List redirects
'listredirects' => 'Llista de redireccions',

# Unused templates
'unusedtemplates'     => 'Plantilles no utilitzades',
'unusedtemplatestext' => "Aquesta pàgina mostra les pàgines en l'espai de noms de plantilles, que no estan incloses en cap altra pàgina. Recordeu de comprovar les pàgines que hi enllacen abans d'esborrar-les.",
'unusedtemplateswlh'  => 'altres enllaços',

# Random page
'randompage'         => "Pàgina a l'atzar",
'randompage-nopages' => "No hi ha cap pàgina en l'espai de noms.",

# Random redirect
'randomredirect'         => "Redirecció a l'atzar",
'randomredirect-nopages' => "No hi ha cap redirecció a l'espai de noms.",

# Statistics
'statistics'             => 'Estadístiques',
'sitestats'              => 'Estadístiques del lloc',
'userstats'              => "Estadístiques d'usuari",
'sitestatstext'          => "Hi ha {{PLURAL:$1|una única pàgina|un total de '''$1''' pàgines}} en la base de dades.
Això inclou pàgines de discussió, pàgines sobre el projecte {{SITENAME}}, pàgines mínimes,
redireccions, i altres que probablement no es poden classificar com a articles.
Excloent-les, hi ha {{PLURAL:$2|una única pàgina que es pugui considerar article legítim|'''$2''' pàgines que probablement són articles legítims}}.

S'{{PLURAL:$8|ha penjat un únic fitxer|han penjat '''$8''' fitxers}}.

Hi ha hagut un total d{{PLURAL:$3|'una única visita|e '''$3''' visites}} a pàgines, i {{PLURAL:$4|una edició|'''$4''' edicions}} de pàgina
des que el programari s'ha configurat.
Això resulta en una mitjana {{PLURAL:$5|d'una edició|de '''$5''' edicions}} per pàgina,
i {{PLURAL:$6|'''$6''' visita|'''$6''' visites}} per edició.

La mida de la [http://www.mediawiki.org/wiki/Manual:Job_queue cua de treballs] és '''$7'''.",
'userstatstext'          => "Hi ha {{PLURAL:$1|'''1''' usuari registrat i, a més,|'''$1''' usuaris registrats, dels quals}} {{PLURAL:$2|un (el '''$4%''') té|'''$2''' (el '''$4%''') tenen}} drets de: $5.",
'statistics-mostpopular' => 'Pàgines més visualitzades',

'disambiguations'      => 'Pàgines de desambiguació',
'disambiguationspage'  => 'Template:Desambiguació',
'disambiguations-text' => "Les següents pàgines enllacen a una '''pàgina de desambiguació'''.
Per això, caldria que enllacessin al tema apropiat.<br />
Una pàgina es tracta com de desambiguació si utilitza una plantilla que està enllaçada a [[MediaWiki:Disambiguationspage]]",

'doubleredirects'            => 'Redireccions dobles',
'doubleredirectstext'        => '<b>Atenció:</b> aquesta llista pot contenir falsos positius. Això normalment significa que hi ha text addicional amb enllaços sota el primer #REDIRECT.<br />
Cada fila conté enllaços a la segona i tercera redirecció, així com la primera línia de la segona redirecció, la qual cosa dóna normalment l\'article "real", al que el primer redirecció hauria d\'apuntar.',
'double-redirect-fixed-move' => "S'ha reanomenat [[$1]], ara és una redirecció a [[$2]]",
'double-redirect-fixer'      => 'Supressor de dobles redireccions',

'brokenredirects'        => 'Redireccions rompudes',
'brokenredirectstext'    => 'Les següents redireccions enllacen a pàgines inexistents:',
'brokenredirects-edit'   => '(edita)',
'brokenredirects-delete' => '(elimina)',

'withoutinterwiki'         => 'Pàgines sense enllaços a altres llengües',
'withoutinterwiki-summary' => "Les pàgines següents no enllacen a versions d'altres llengües:",
'withoutinterwiki-legend'  => 'Prefix',
'withoutinterwiki-submit'  => 'Mostra',

'fewestrevisions' => 'Pàgines amb menys revisions',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|octet|octets}}',
'ncategories'             => '$1 {{PLURAL:$1|categoria|categories}}',
'nlinks'                  => '$1 {{PLURAL:$1|enllaç|enllaços}}',
'nmembers'                => '$1 {{PLURAL:$1|membre|membres}}',
'nrevisions'              => '$1 {{PLURAL:$1|revisió|revisions}}',
'nviews'                  => '$1 {{PLURAL:$1|visita|visites}}',
'specialpage-empty'       => 'Aquesta pàgina és buida.',
'lonelypages'             => 'Pàgines òrfenes',
'lonelypagestext'         => "Les següents pàgines no s'enllacen des d'altres pàgines del projecte {{SITENAME}}.",
'uncategorizedpages'      => 'Pàgines sense categoria',
'uncategorizedcategories' => 'Categories sense categoria',
'uncategorizedimages'     => 'Fitxers sense categoria',
'uncategorizedtemplates'  => 'Plantilles sense categoria',
'unusedcategories'        => 'Categories sense cap ús',
'unusedimages'            => 'Imatges sense ús',
'popularpages'            => 'Pàgines populars',
'wantedcategories'        => 'Categories demanades',
'wantedpages'             => 'Pàgines demanades',
'missingfiles'            => 'Arxius que falten',
'mostlinked'              => 'Pàgines més enllaçades',
'mostlinkedcategories'    => 'Categories més utilitzades',
'mostlinkedtemplates'     => 'Plantilles més usades',
'mostcategories'          => 'Pàgines que utilitzen més categories',
'mostimages'              => 'Més enllaçat a fitxers',
'mostrevisions'           => 'Pàgines més modificades',
'prefixindex'             => 'Cercar per prefix',
'shortpages'              => 'Pàgines curtes',
'longpages'               => 'Pàgines llargues',
'deadendpages'            => 'Pàgines atzucac',
'deadendpagestext'        => "Aquestes pàgines no tenen enllaços a d'altres pàgines del projecte {{SITENAME}}.",
'protectedpages'          => 'Pàgines protegides',
'protectedpages-indef'    => 'Només proteccions indefinides',
'protectedpagestext'      => 'Les pàgines següents estan protegides perquè no es puguin editar o reanomenar',
'protectedpagesempty'     => 'No hi ha cap pàgina protegida per ara',
'protectedtitles'         => 'Títols protegits',
'protectedtitlestext'     => 'Els títols següents estan protegits de crear-se',
'protectedtitlesempty'    => 'No hi ha cap títol protegit actualment amb aquests paràmetres.',
'listusers'               => "Llistat d'usuaris",
'newpages'                => 'Pàgines noves',
'newpages-username'       => "Nom d'usuari:",
'ancientpages'            => 'Pàgines més antigues',
'move'                    => 'Reanomena',
'movethispage'            => 'Trasllada la pàgina',
'unusedimagestext'        => 'Tingueu en compte que altres llocs web poden enllaçar un fitxer amb un URL directe i estar llistat ací tot i estar en ús actiu.',
'unusedcategoriestext'    => 'Les pàgines de categoria següents existeixen encara que cap altra pàgina o categoria les utilitza.',
'notargettitle'           => 'No hi ha pàgina en blanc',
'notargettext'            => 'No heu especificat a quina pàgina dur a terme aquesta funció.',
'nopagetitle'             => 'No existeix aquesta pàgina',
'nopagetext'              => 'La pàgina que heu especificat no existeix.',
'pager-newer-n'           => '{{PLURAL:$1|1 posterior|$1 posteriors}}',
'pager-older-n'           => '{{PLURAL:$1|anterior|$1 anteriors}}',
'suppress'                => 'Oversight',

# Book sources
'booksources'               => 'Obres de referència',
'booksources-search-legend' => 'Cerca fonts de llibres',
'booksources-go'            => 'Vés-hi',
'booksources-text'          => "A sota hi ha una llista d'enllaços d'altres llocs que venen llibres nous i de segona mà, i també podrien tenir més informació dels llibres que esteu cercant:",

# Special:Log
'specialloguserlabel'  => 'Usuari:',
'speciallogtitlelabel' => 'Títol:',
'log'                  => 'Registres',
'all-logs-page'        => 'Tots els registres',
'log-search-legend'    => 'Cerca als registres',
'log-search-submit'    => 'Vés-hi',
'alllogstext'          => "Presentació combinada de tots els registres disponibles de {{SITENAME}}.
Podeu reduir l'extensió seleccionant el tipus de registre, el nom del usuari (distingeix entre majúscules i minúscules), o la pàgina afectada (també en distingeix).",
'logempty'             => 'No hi ha cap coincidència en el registre.',
'log-title-wildcard'   => 'Cerca els títols que comencin amb aquest text',

# Special:AllPages
'allpages'          => 'Totes les pàgines',
'alphaindexline'    => '$1 a $2',
'nextpage'          => 'Pàgina següent ($1)',
'prevpage'          => 'Pàgina anterior ($1)',
'allpagesfrom'      => 'Mostra les pàgines que comencin per:',
'allarticles'       => 'Totes les pàgines',
'allinnamespace'    => "Totes les pàgines (de l'espai de noms $1)",
'allnotinnamespace' => "Totes les pàgines (que no són a l'espai de noms $1)",
'allpagesprev'      => 'Anterior',
'allpagesnext'      => 'Següent',
'allpagessubmit'    => 'Vés-hi',
'allpagesprefix'    => 'Mostra les pàgines amb prefix:',
'allpagesbadtitle'  => "El títol de la pàgina que heu inserit no és vàlid o conté un prefix d'enllaç amb un altre projecte. També pot passar que contingui un o més caràcters que no es puguin fer servir en títols de pàgina.",
'allpages-bad-ns'   => "El projecte {{SITENAME}} no disposa de l'espai de noms «$1».",

# Special:Categories
'categories'                    => 'Categories',
'categoriespagetext'            => "Les categories següents contenen pàgines, o fitxers multimèdia.
[[Special:UnusedCategories|Les categories no usades]] no s'hi mostren.
Vegeu també [[Special:WantedCategories|les categories sol·licitades]].",
'categoriesfrom'                => 'Mostra les categories que comencen a:',
'special-categories-sort-count' => 'ordena per recompte',
'special-categories-sort-abc'   => 'ordena alfabèticament',

# Special:ListUsers
'listusersfrom'      => 'Mostra usuaris començant per:',
'listusers-submit'   => 'Mostra',
'listusers-noresult' => "No s'han trobat coincidències de noms d'usuaris. Si us plau, busqueu també amb variacions per majúscules i minúscules.",

# Special:ListGroupRights
'listgrouprights'          => "Drets dels grups d'usuaris",
'listgrouprights-summary'  => "A continuació hi ha una llista dels grups d'usuaris definits en aquest wiki, així com dels seus drets d'accés associats.
Pot ser que hi hagi més informació sobre drets individuals [[{{MediaWiki:Listgrouprights-helppage}}|aquí]].",
'listgrouprights-group'    => 'Grup',
'listgrouprights-rights'   => 'Drets',
'listgrouprights-helppage' => 'Help:Drets del grup',
'listgrouprights-members'  => '(llista de membres)',

# E-mail user
'mailnologin'     => "No enviïs l'adreça",
'mailnologintext' => "Heu d'haver [[Special:UserLogin|entrat]]
i tenir una direcció electrònica vàlida en les vostres [[Special:Preferences|preferències]]
per enviar un correu electrònic a altres usuaris.",
'emailuser'       => 'Envia un missatge de correu electrònic a aquest usuari',
'emailpage'       => 'Correu electrònic a usuari',
'emailpagetext'   => "Si aquest usuari ha entrat una adreça electrònica vàlida en les seves preferències d'usuari, el següent formulari enviarà un únic missatge.
L'adreça electrònica que heu entrat en [[Special:Preferences|les vostres preferències d'usuari]] apareixerà en el remitent del correu electrònic, de manera que el destinatari us podrà respondre directament.",
'usermailererror' => "L'objecte de correu ha retornat un error:",
'defemailsubject' => 'Adreça correl de {{SITENAME}}',
'noemailtitle'    => 'No hi ha cap adreça electrònica',
'noemailtext'     => "Aquest usuari no ha especificat una adreça electrònica vàlida, o ha escollit no rebre correu electrònic d'altres usuaris

.",
'emailfrom'       => 'De:',
'emailto'         => 'Per a:',
'emailsubject'    => 'Assumpte:',
'emailmessage'    => 'Missatge:',
'emailsend'       => 'Envia',
'emailccme'       => "Envia'm una còpia del meu missatge.",
'emailccsubject'  => 'Còpia del vostre missatge a $1: $2',
'emailsent'       => 'Correu electrònic enviat',
'emailsenttext'   => 'El vostre correu electrònic ha estat enviat.',
'emailuserfooter' => "Aquest missatge de correu electrònic l'ha enviat $1 a $2 amb la funció «e-mail» del projecte {{SITENAME}}.",

# Watchlist
'watchlist'            => 'Llista de seguiment',
'mywatchlist'          => 'Llista de seguiment',
'watchlistfor'         => "(per a '''$1''')",
'nowatchlist'          => 'No teniu cap element en la vostra llista de seguiment.',
'watchlistanontext'    => 'Premeu $1 per a visualitzar o editar elements de la vostra llista de seguiment.',
'watchnologin'         => 'No heu iniciat la sessió',
'watchnologintext'     => "Heu d'[[Special:UserLogin|entrar]]
per modificar el vostre llistat de seguiment.",
'addedwatch'           => "S'ha afegit la pàgina a la llista de seguiment",
'addedwatchtext'       => "S'ha afegit la pàgina «[[:$1]]» a la vostra [[Special:Watchlist|llista de seguiment]].

Els canvis futurs que tinguin lloc en aquesta pàgina i la seua corresponent discussió sortiran en la vostra [[Special:Watchlist|llista de seguiment]]. A més la pàgina estarà ressaltada '''en negreta''' dins la [[Special:RecentChanges|llista de canvis recents]] perquè pugueu adonar-vos-en amb més facilitat dels canvis que tingui.

Si voleu deixar de vigilar la pàgina, cliqueu sobre l'enllaç de «Desatén» de la barra lateral.",
'removedwatch'         => "S'ha tret de la llista de seguiment",
'removedwatchtext'     => 'S\'ha tret la pàgina "[[:$1]]" de la vostra llista de seguiment.',
'watch'                => 'Vigila',
'watchthispage'        => 'Vigila aquesta pàgina',
'unwatch'              => 'Desatén',
'unwatchthispage'      => 'Desatén',
'notanarticle'         => 'No és una pàgina amb contingut',
'notvisiblerev'        => 'La versió ha estat esborrada',
'watchnochange'        => "No s'ha editat cap dels elements que vigileu en el període de temps que es mostra.",
'watchlist-details'    => '{{PLURAL:$1|$1 pàgina|$1 pàgines}} vigilades, sense comptar les pàgines de discussió',
'wlheader-enotif'      => "* S'ha habilitat la notificació per correu electrònic.",
'wlheader-showupdated' => "* Les pàgines que s'han canviat des de la vostra darrera visita es mostren '''en negreta'''",
'watchmethod-recent'   => "s'està comprovant si ha pàgines vigilades en les edicions recents",
'watchmethod-list'     => "s'està comprovant si hi ha edicions recents en les pàgines vigilades",
'watchlistcontains'    => 'La vostra llista de seguiment conté {{PLURAL:$1|una única pàgina|$1 pàgines}}.',
'iteminvalidname'      => "Hi ha un problema amb l'element '$1': el nom no és vàlid...",
'wlnote'               => 'A sota hi ha {{PLURAL:$1|el darrer canvi|els darrers $1 canvis}} en {{PLURAL:$2|la darrera hora|les darreres $2 hores}}.',
'wlshowlast'           => '<small>- Mostra les darreres $1 hores, els darrers $2 dies o $3</small>',
'watchlist-show-bots'  => 'Mostra les edicions dels bots',
'watchlist-hide-bots'  => 'Amaga les edicions dels bots',
'watchlist-show-own'   => 'Mostra les edicions pròpies',
'watchlist-hide-own'   => 'Amaga les edicions pròpies',
'watchlist-show-minor' => 'Mostra les edicions menors',
'watchlist-hide-minor' => 'Amaga les edicions menors',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => "S'està vigilant...",
'unwatching' => "S'està desatenent...",

'enotif_mailer'                => 'Sistema de notificació per correl de {{SITENAME}}',
'enotif_reset'                 => 'Marca totes les pàgines com a visitades',
'enotif_newpagetext'           => 'Aquesta és una nova pàgina.',
'enotif_impersonal_salutation' => 'usuari de la {{SITENAME}}',
'changed'                      => 'modificat',
'created'                      => 'publicat',
'enotif_subject'               => '$PAGEEDITOR ha $CHANGEDORCREATED la pàgina $PAGETITLE en {{SITENAME}}',
'enotif_lastvisited'           => "Vegeu $1 per a tots els canvis que s'han fet d'ença de la vostra darrera visita.",
'enotif_lastdiff'              => 'Consulteu $1 per a visualitzar aquest canvi.',
'enotif_anon_editor'           => 'usuari anònim $1',
'enotif_body'                  => 'Benvolgut $WATCHINGUSERNAME,

La pàgina $PAGETITLE del projecte {{SITENAME}} ha estat $CHANGEDORCREATED el dia $PAGEEDITDATE per $PAGEEDITOR, vegeu $PAGETITLE_URL per la versió actual.

$NEWPAGE

Resum ofert per l\'editor: $PAGESUMMARY $PAGEMINOREDIT

Contacteu amb l\'editor:
correu: $PAGEEDITOR_EMAIL
pàgina d\'usuari: $PAGEEDITOR_WIKI

No rebreu més notificacions de futurs canvis si no visiteu la pàgina. També podeu canviar el mode de notificació de les pàgines que vigileu en la vostra llista de seguiment.

             El servei de notificació del projecte {{SITENAME}}

--
Per a canviar les opcions de la vostra llista de seguiment aneu a:
{{fullurl:Special:Watchlist/edit}}

Suggeriments i ajuda:
{{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => 'Elimina la pàgina',
'confirm'                     => 'Confirma',
'excontent'                   => 'el contingut era: «$1»',
'excontentauthor'             => "el contingut era: «$1» (i l'única persona qui hi ha editat ha estat «[[Special:Contributions/$2|$2]]»)",
'exbeforeblank'               => "el contingut abans d'estar en blanc era: '$1'",
'exblank'                     => 'la pàgina estava en blanc',
'delete-confirm'              => 'Elimina «$1»',
'delete-legend'               => 'Elimina',
'historywarning'              => 'Avís: La pàgina que eliminareu té un historial:',
'confirmdeletetext'           => "Esteu a punt d'esborrar de forma permanent una pàgina o imatge i tot el seu historial de la base de dades.
Confirmeu que realment ho voleu fer, que enteneu les
conseqüències, i que el que esteu fent està d'acord amb la [[{{MediaWiki:Policy-url}}|política]] del projecte.",
'actioncomplete'              => "S'ha realitzat l'acció de manera satisfactòria.",
'deletedtext'                 => '"<nowiki>$1</nowiki>" ha estat esborrat.
Mostra $2 per a un registre dels esborrats més recents.',
'deletedarticle'              => 'eliminat "[[$1]]"',
'suppressedarticle'           => "s'ha suprimit «[[$1]]»",
'dellogpage'                  => "Registre d'eliminació",
'dellogpagetext'              => 'Davall hi ha una llista dels esborraments més recents.',
'deletionlog'                 => "Registre d'esborrats",
'reverted'                    => 'Invertit amb una revisió anterior',
'deletecomment'               => 'Motiu per a ser esborrat:',
'deleteotherreason'           => 'Motius diferents o addicionals:',
'deletereasonotherlist'       => 'Altres motius',
'deletereason-dropdown'       => "*Motius freqüents d'esborrat
** Demanada per l'autor
** Violació del copyright
** Vandalisme
** Proves
** Error en el nom
** Fer lloc a un trasllat",
'delete-edit-reasonlist'      => "Edita els motius d'eliminació",
'delete-toobig'               => "Aquesta pàgina té un historial d'edicions molt gran, amb més de $1 {{PLURAL:$1|canvi|canvis}}. L'eliminació d'aquestes pàgines està restringida per a prevenir que hi pugui haver un desajustament seriós de la base de dades de tot el projecte {{SITENAME}} per accident.",
'delete-warning-toobig'       => "Aquesta pàgina té un historial d'edicions molt gran, amb més de $1 {{PLURAL:$1|canvi|canvis}}. Eliminar-la podria suposar un seriós desajustament de la base de dades de tot el projecte {{SITENAME}}; aneu en compte abans dur a terme l'acció.",
'rollback'                    => 'Reverteix edicions',
'rollback_short'              => 'Revoca',
'rollbacklink'                => 'Reverteix',
'rollbackfailed'              => "No s'ha pogut revocar",
'cantrollback'                => "No s'ha pogut revertir les edicions; el darrer col·laborador és l'únic autor de la pàgina.",
'alreadyrolled'               => "No es pot revertir a la darrera edició de [[:$1]]
per l'usuari [[User:$2|$2]] ([[User talk:$2|Discussió]]); algú altre ha editat o revertit la pàgina.

La darrera edició ha estat feta per l'usuari [[User:$3|$3]] ([[User talk:$3|Discussió]] | [[Special:Contributions/$3|{{int:contribslink}}]]).",
'editcomment'                 => 'El comentari d\'edició ha estat: "<i>$1</i>".', # only shown if there is an edit comment
'revertpage'                  => "Revertides les edicions de [[Special:Contributions/$2|$2]] ([[User talk:$2|discussió]]). S'ha recuperat la darrera versió de l'usuari [[User:$1|$1]]", # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => "Edicions revertides de $1; s'ha canviat a la darrera versió de $2.",
'sessionfailure'              => 'Sembla que hi ha problema amb la vostra sessió; aquesta acció ha estat anulada en prevenció de pirateig de sessió. Si us plau, pitgueu "Torna", i recarregueu la pàgina des d\'on veniu, aleshores intenteu-lo de nou.',
'protectlogpage'              => 'Registre de protecció',
'protectlogtext'              => 'Aquest és el registre de proteccions i desproteccions. Vegeu la [[Special:ProtectedPages|llista de pàgines protegides]] per a la llista de les pàgines que actualment tenen alguna protecció.',
'protectedarticle'            => 'protegit «[[$1]]»',
'modifiedarticleprotection'   => "s'ha canviat el nivell de protecció «[[$1]]»",
'unprotectedarticle'          => '«[[$1]]» desprotegida',
'protect-title'               => 'Canviant la protecció de «$1»',
'protect-legend'              => 'Confirmeu la protecció',
'protectcomment'              => 'Motiu de la protecció',
'protectexpiry'               => "Data d'expiració",
'protect_expiry_invalid'      => "Data d'expiració no vàlida",
'protect_expiry_old'          => 'El temps de termini ja ha passat.',
'protect-unchain'             => 'Permet diferent nivell de protecció per editar i per moure',
'protect-text'                => 'Aquí podeu visualitzar i canviar el nivell de protecció de la pàgina «<nowiki>$1</nowiki>». Assegureu-vos de seguir les polítiques existents.',
'protect-locked-blocked'      => 'No podeu canviar els nivells de protecció mentre estigueu bloquejats. Ací hi ha els
paràmetres actuals de la pàgina <strong>$1</strong>:',
'protect-locked-dblock'       => "No poden canviar-se els nivells de protecció a casa d'un bloqueig actiu de la base de dades.
Ací hi ha els paràmetres actuals de la pàgina <strong>$1</strong>:",
'protect-locked-access'       => 'El vostre compte no té permisos per a canviar els nivells de protecció de la pàgina.
Ací es troben els paràmetres actuals de la pàgina <strong>$1</strong>:',
'protect-cascadeon'           => "Aquesta pàgina es troba protegida perquè està inclosa en {{PLURAL:$1|la següent pàgina que té|les següents pàgines que tenen}} activada una protecció en cascada. Podeu canviar el nivell de protecció d'aquesta pàgina però això no afectarà la protecció en cascada.",
'protect-default'             => '(per defecte)',
'protect-fallback'            => 'Cal el permís de «$1»',
'protect-level-autoconfirmed' => 'Bloca els usuaris no registrats',
'protect-level-sysop'         => 'Bloqueja tots els usuaris excepte administradors',
'protect-summary-cascade'     => 'en cascada',
'protect-expiring'            => 'expira el dia $1 (UTC)',
'protect-cascade'             => 'Protecció en cascada: protegeix totes les pàgines i plantilles incloses en aquesta.',
'protect-cantedit'            => "No podeu canviar els nivells de protecció d'aquesta pàgina, perquè no teniu permisos per a editar-la.",
'restriction-type'            => 'Permís:',
'restriction-level'           => 'Nivell de restricció:',
'minimum-size'                => 'Mida mínima',
'maximum-size'                => 'Mida màxima:',
'pagesize'                    => '(bytes)',

# Restrictions (nouns)
'restriction-edit'   => 'Edita',
'restriction-move'   => 'Reanomena',
'restriction-create' => 'Crea',
'restriction-upload' => 'Carrega',

# Restriction levels
'restriction-level-sysop'         => 'protegida',
'restriction-level-autoconfirmed' => 'semiprotegida',
'restriction-level-all'           => 'qualsevol nivell',

# Undelete
'undelete'                     => 'Restaura una pàgina esborrada',
'undeletepage'                 => 'Mostra i restaura pàgines esborrades',
'undeletepagetitle'            => "'''A continuació teniu revisions eliminades de [[:$1]]'''.",
'viewdeletedpage'              => 'Visualitza les pàgines eliminades',
'undeletepagetext'             => "S'han eliminat les pàgines següents però encara són a l'arxiu i poden ser restaurades. Pot netejar-se l'arxiu periòdicament.",
'undelete-fieldset-title'      => 'Restaura revisions',
'undeleteextrahelp'            => "Per a restaurar la pàgina sencera, deixeu totes les caselles sense seleccionar i
cliqueu a  '''''Restaura'''''.
Per a realitzar una restauració selectiva, marqueu les caselles que corresponguin
a les revisions que voleu recuperar, i feu clic a '''''Restaura'''''.
Si cliqueu '''''Reinicia''''' es netejarà el camp de comentari i es desmarcaran totes les caselles.",
'undeleterevisions'            => '{{PLURAL:$1|Una revisió arxivada|$1 revisions arxivades}}',
'undeletehistory'              => "Si restaureu la pàgina, totes les revisions seran restaurades a l'historial.

Si s'hagués creat una nova pàgina amb el mateix nom d'ençà que la vàreu esborrar, les versions restaurades apareixeran abans a l'historial.",
'undeleterevdel'               => "No es revertirà l'eliminació si això resulta que la pàgina superior se suprimeixi parcialment.

En aqueixos casos, heu de desmarcar o mostrar les revisions eliminades més noves.",
'undeletehistorynoadmin'       => "S'ha eliminat la pàgina. El motiu es mostra
al resum a continuació, juntament amb detalls dels usuaris que l'havien editat abans de la seua eliminació. El text de les revisions eliminades només és accessible als administradors.",
'undelete-revision'            => "S'ha eliminat la revisió de $1 de $2 (per $3):",
'undeleterevision-missing'     => "La revisió no és vàlida o no hi és. Podeu tenir-hi un enllaç incorrecte, o bé pot haver-se restaurat o eliminat de l'arxiu.",
'undelete-nodiff'              => "No s'ha trobat cap revisió anterior.",
'undeletebtn'                  => 'Restaura!',
'undeletelink'                 => 'restaura',
'undeletereset'                => 'Reinicia',
'undeletecomment'              => 'Comentari:',
'undeletedarticle'             => 'restaurat "$1"',
'undeletedrevisions'           => '{{PLURAL:$1|Una revisió restaurada|$1 revisions restaurades}}',
'undeletedrevisions-files'     => '{{PLURAL:$1|Una revisió|$1 revisions}} i {{PLURAL:$2|un arxiu|$2 arxius}} restaurats',
'undeletedfiles'               => '$1 {{PLURAL:$1|fitxer restaurat|fitxers restaurats}}',
'cannotundelete'               => "No s'ha pogut restaurar; algú altre pot estar restaurant la mateixa pàgina.",
'undeletedpage'                => "<big>'''S'ha restaurat «$1»'''</big>

Consulteu el [[Special:Log/delete|registre d'esborraments]] per a veure els esborraments i els restauraments més recents.",
'undelete-header'              => "Vegeu [[Special:Log/delete|el registre d'eliminació]] per a veure les pàgines eliminades recentment.",
'undelete-search-box'          => 'Cerca pàgines esborrades',
'undelete-search-prefix'       => 'Mostra pàgines que comencin:',
'undelete-search-submit'       => 'Cerca',
'undelete-no-results'          => "No s'ha trobat cap pàgina que hi coincideixi a l'arxiu d'eliminació.",
'undelete-filename-mismatch'   => "No es pot revertir l'eliminació de la revisió de fitxer amb marca horària $1: no coincideix el nom de fitxer",
'undelete-bad-store-key'       => 'No es pot revertir la revisió de fitxer amb marca horària $1: el fitxer no hi era abans i tot de ser eliminat.',
'undelete-cleanup-error'       => "S'ha produït un error en eliminar el fitxer d'arxiu sense utilitzar «$1».",
'undelete-missing-filearchive' => "No s'ha pogut restaurar l'identificador $1 d'arxiu de fitxers perquè no es troba a la base de dades. Podria ser que ja s'hagués revertit l'eliminació.",
'undelete-error-short'         => "S'ha produït un error en revertir l'eliminació del fitxer: $1",
'undelete-error-long'          => "S'han produït errors en revertir la supressió del fitxer:

$1",

# Namespace form on various pages
'namespace'      => 'Espai de noms:',
'invert'         => 'Inverteix la selecció',
'blanknamespace' => '(Principal)',

# Contributions
'contributions' => "Contribucions de l'usuari",
'mycontris'     => 'Contribucions',
'contribsub2'   => 'Per $1 ($2)',
'nocontribs'    => "No s'ha trobat canvis que encaixessin amb aquests criteris.",
'uctop'         => '(actual)',
'month'         => 'Mes (i anteriors):',
'year'          => 'Any (i anteriors):',

'sp-contributions-newbies'     => 'Mostra les contribucions dels usuaris novells',
'sp-contributions-newbies-sub' => 'Per a novells',
'sp-contributions-blocklog'    => 'Registre de bloquejos',
'sp-contributions-search'      => 'Cerca les contribucions',
'sp-contributions-username'    => "Adreça IP o nom d'usuari:",
'sp-contributions-submit'      => 'Cerca',

# What links here
'whatlinkshere'            => 'Què hi enllaça',
'whatlinkshere-title'      => 'Pàgines que enllacen amb $1',
'whatlinkshere-page'       => 'Pàgina:',
'linklistsub'              => "(Llista d'enllaços)",
'linkshere'                => "Les següents pàgines enllacen amb '''[[:$1]]''':",
'nolinkshere'              => "Cap pàgina no enllaça amb '''[[:$1]]'''.",
'nolinkshere-ns'           => "No s'enllaça cap pàgina a '''[[:$1]]''' en l'espai de noms triat.",
'isredirect'               => 'pàgina redirigida',
'istemplate'               => 'inclosa',
'isimage'                  => 'enllaç a imatge',
'whatlinkshere-prev'       => '{{PLURAL:$1|anterior|anteriors $1}}',
'whatlinkshere-next'       => '{{PLURAL:$1|següent|següents $1}}',
'whatlinkshere-links'      => '← enllaços',
'whatlinkshere-hideredirs' => '$1 redireccions',
'whatlinkshere-hidetrans'  => '$1 inclusions',
'whatlinkshere-hidelinks'  => '$1 enllaços',
'whatlinkshere-hideimages' => '$1 enllaços a imatge',
'whatlinkshere-filters'    => 'Filtres',

# Block/unblock
'blockip'                         => "Bloqueig d'usuaris",
'blockip-legend'                  => "Bloca l'usuari",
'blockiptext'                     => "Empreu el següent formulari per blocar l'accés
d'escriptura des d'una adreça IP específica o des d'un usuari determinat.
això només s'hauria de fer per prevenir el vandalisme, i
d'acord amb la [[{{MediaWiki:Policy-url}}|política del projecte]].
Empleneu el diàleg de sota amb un motiu específic (per exemple, citant
quines pàgines en concret estan sent vandalitzades).",
'ipaddress'                       => 'Adreça IP',
'ipadressorusername'              => "Adreça IP o nom de l'usuari",
'ipbexpiry'                       => 'Venciment',
'ipbreason'                       => 'Motiu',
'ipbreasonotherlist'              => 'Un altre motiu',
'ipbreason-dropdown'              => "*Motius de bloqueig més freqüents
** Inserció d'informació falsa
** Supressió de contingut sense justificació
** Inserció d'enllaços promocionals (spam)
** Inserció de contingut sense cap sentit
** Conducta intimidatòria o hostil
** Abús de comptes d'usuari múltiples
** Nom d'usuari no acceptable",
'ipbanononly'                     => 'Bloca només els usuaris anònims',
'ipbcreateaccount'                => 'Evita la creació de comptes',
'ipbemailban'                     => "Evita que l'usuari enviï correu electrònic",
'ipbenableautoblock'              => "Bloca l'adreça IP d'aquest usuari, i totes les subseqüents adreces des de les quals intenti registrar-se",
'ipbsubmit'                       => 'Bloqueja aquesta adreça',
'ipbother'                        => 'Un altre termini',
'ipboptions'                      => '2 hores:2 hours,1 dia:1 day,3 dies:3 days,1 setmana:1 week,2 setmanes:2 weeks,1 mes:1 month,3 mesos:3 months,6 mesos:6 months,1 any:1 year,infinit:infinite', # display1:time1,display2:time2,...
'ipbotheroption'                  => 'un altre',
'ipbotherreason'                  => 'Altres motius o addicionals:',
'ipbhidename'                     => "Amaga el nom d'usuari del registre de bloquejos, activa la llista de bloqueig i d'usuaris",
'ipbwatchuser'                    => "Vigila les pàgines d'usuari i de discussió de l'usuari",
'badipaddress'                    => "L'adreça IP no té el format correcte.",
'blockipsuccesssub'               => "S'ha blocat amb èxit",
'blockipsuccesstext'              => 'L\'usuari "[[Special:Contributions/$1|$1]]" ha estat blocat.
<br />Vegeu la [[Special:IPBlockList|llista d\'IP blocades]] per revisar els bloquejos.',
'ipb-edit-dropdown'               => 'Edita les raons per a blocar',
'ipb-unblock-addr'                => 'Desbloca $1',
'ipb-unblock'                     => 'Desbloca un usuari o una adreça IP',
'ipb-blocklist-addr'              => 'Llista els bloquejos existents per $1',
'ipb-blocklist'                   => 'Llista els bloquejos existents',
'unblockip'                       => "Desbloca l'usuari",
'unblockiptext'                   => "Empreu el següent formulari per restaurar
l'accés a l'escriptura a una adreça IP o un usuari prèviament bloquejat.",
'ipusubmit'                       => 'Desbloca aquesta adreça',
'unblocked'                       => "S'ha desbloquejat l'usuari [[User:$1|$1]]",
'unblocked-id'                    => "S'ha eliminat el bloqueig de $1",
'ipblocklist'                     => "Llista d'adreces IP i noms d'usuaris blocats",
'ipblocklist-legend'              => 'Cerca un usuari blocat',
'ipblocklist-username'            => "Nom d'usuari o adreça IP:",
'ipblocklist-submit'              => 'Cerca',
'blocklistline'                   => '$1, $2 bloca $3 ($4)',
'infiniteblock'                   => 'infinit',
'expiringblock'                   => 'venç el $1',
'anononlyblock'                   => 'només usuari anònim',
'noautoblockblock'                => "S'ha inhabilitat el bloqueig automàtic",
'createaccountblock'              => "s'ha blocat la creació de nous comptes",
'emailblock'                      => "s'ha blocat l'enviament de correus electrònics",
'ipblocklist-empty'               => 'La llista de bloqueig està buida.',
'ipblocklist-no-results'          => "La adreça IP sol·licitada o nom d'usuari està bloquejada.",
'blocklink'                       => 'bloca',
'unblocklink'                     => 'desbloca',
'contribslink'                    => 'contribucions',
'autoblocker'                     => 'Heu estat blocat perquè compartiu adreça IP amb «$1». Motiu: «$2»',
'blocklogpage'                    => 'Registre de bloquejos',
'blocklogentry'                   => "s'ha blocat «[[$1]]» per a un període de $2 $3",
'blocklogtext'                    => "Això és una relació de accions de bloqueig i desbloqueig. Les adreces IP bloquejades automàticament no apareixen. Vegeu la [[Special:IPBlockList|llista d'usuaris actualment bloquejats]].",
'unblocklogentry'                 => 'desbloquejat $1',
'block-log-flags-anononly'        => 'només els usuaris anònims',
'block-log-flags-nocreate'        => "s'ha desactivat la creació de comptes",
'block-log-flags-noautoblock'     => 'sense bloqueig automàtic',
'block-log-flags-noemail'         => 'correu-e blocat',
'block-log-flags-angry-autoblock' => 'autoblocatge avançat activat',
'range_block_disabled'            => 'La facultat dels administradors per a crear bloquejos de rang està desactivada.',
'ipb_expiry_invalid'              => "Data d'acabament no vàlida.",
'ipb_expiry_temp'                 => "Els blocatges amb ocultació de nom d'usuari haurien de ser permanents.",
'ipb_already_blocked'             => '«$1» ja està blocat',
'ipb_cant_unblock'                => "Errada: No s'ha trobat el núm. ID de bloqueig $1. És possible que ja s'haguera desblocat.",
'ipb_blocked_as_range'            => "Error: L'adreça IP $1 no està blocada directament i per tant no pot ésser desbloquejada. Ara bé, sí que ho està per formar part del rang $2 que sí que pot ser desblocat.",
'ip_range_invalid'                => 'Rang de IP no vàlid.',
'blockme'                         => "Bloca'm",
'proxyblocker'                    => 'Bloqueig de proxy',
'proxyblocker-disabled'           => "S'ha inhabilitat la funció.",
'proxyblockreason'                => "La vostra adreça IP ha estat bloquejada perquè és un proxy obert. Si us plau contactau el vostre proveïdor d'Internet o servei tècnic i informau-los d'aquest seriós problema de seguretat.",
'proxyblocksuccess'               => 'Fet.',
'sorbsreason'                     => "La vostra adreça IP està llistada com a servidor intermediari (''proxy'') obert dins la llista negra de DNS que fa servir el projecte {{SITENAME}}.",
'sorbs_create_account_reason'     => "La vostra adreça IP està llistada com a servidor intermediari (''proxy'') obert a la llista negra de DNS que utilitza el projecte {{SITENAME}}. No podeu crear-vos-hi un compte",

# Developer tools
'lockdb'              => 'Bloca la base de dades',
'unlockdb'            => 'Desbloca la base de dades',
'lockdbtext'          => "Blocant la base de dades es suspendrà la capacitat de tots els
usuaris d'editar pàgines, canviar les preferències, editar la llista de seguiment, i
altres canvis que requereixin modificacions en la base de dades.
Confirmeu que això és el que voleu fer, i sobretot no us oblideu
de desblocar la base de dades quan acabeu el manteniment.",
'unlockdbtext'        => "Desblocant la base de dades es restaurarà l'habilitat de tots
els usuaris d'editar pàgines, canviar les preferències, editar els llistats de seguiment, i
altres accions que requereixen canvis en la base de dades.
Confirmeu que això és el que voleu fer.",
'lockconfirm'         => 'Sí, realment vull blocar la base de dades.',
'unlockconfirm'       => 'Sí, realment vull desblocar la base dades.',
'lockbtn'             => 'Bloca la base de dades',
'unlockbtn'           => 'Desbloca la base de dades',
'locknoconfirm'       => 'No heu respost al diàleg de confirmació.',
'lockdbsuccesssub'    => "S'ha bloquejat la base de dades",
'unlockdbsuccesssub'  => "S'ha eliminat el bloqueig de la base de dades",
'lockdbsuccesstext'   => "S'ha bloquejat la base de dades.<br />
Recordeu-vos de [[Special:UnlockDB|treure el bloqueig]] quan hàgiu acabat el manteniment.",
'unlockdbsuccesstext' => "S'ha desbloquejat la base de dades del projecte {{SITENAME}}.",
'lockfilenotwritable' => 'No es pot modificar el fitxer de la base de dades de bloquejos. Per a blocar o desblocar la base de dades, heu de donar-ne permís de modificació al servidor web.',
'databasenotlocked'   => 'La base de dades no està bloquejada.',

# Move page
'move-page'               => 'Mou $1',
'move-page-legend'        => 'Reanomena la pàgina',
'movepagetext'            => "Amb el formulari següent reanomenareu una pàgina, movent tot el seu historial al nou nom.
El títol anterior es convertirà en una redirecció al títol que hàgiu creat.
Podeu actualitzar automàticament els enllaços a l'antic títol de la pàgina.
Si no ho feu, assegureu-vos de verificar que no deixeu redireccions [[Special:DoubleRedirects|dobles]] o [[Special:BrokenRedirects|trencades]].
Sou el responsable de fer que els enllaços segueixin apuntant on se suposa que ho han de fer.

Tingueu en compte que la pàgina '''no''' serà traslladada si ja existeix una pàgina amb el títol nou, a no ser que sigui una pàgina buida o una ''redirecció'' sense historial.
Això significa que podeu reanomenar de nou una pàgina al seu títol original si cometeu un error, i que no podeu sobreescriure una pàgina existent.

'''ADVERTÈNCIA!'''
Açò pot ser un canvi dràstic i inesperat en una pàgina que sigui popular;
assegureu-vos d'entendre les conseqüències que comporta abans de seguir endavant.",
'movepagetalktext'        => "La pàgina de discussió associada, si existeix, serà traslladada automàticament '''a menys que:'''
*Ja existeixi una pàgina de discussió no buida amb el nom nou, o
*Hàgiu desseleccionat la opció de sota.

En aquests casos, haureu de traslladar o fusionar la pàgina manualment si ho desitgeu.",
'movearticle'             => 'Reanomena la pàgina',
'movenotallowed'          => 'No teniu permís per a moure pàgines.',
'newtitle'                => 'A títol nou',
'move-watch'              => 'Vigila aquesta pàgina',
'movepagebtn'             => 'Reanomena la pàgina',
'pagemovedsub'            => 'Reanomenament amb èxit',
'movepage-moved'          => "<big>'''«$1» s'ha mogut a «$2»'''</big>", # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => 'Ja existeix una pàgina amb aquest nom, o el nom que heu triat no és vàlid.
Trieu-ne un altre, si us plau.',
'cantmove-titleprotected' => "No podeu moure una pàgina a aquesta ubicació, perquè s'ha protegit la creació del títol nou",
'talkexists'              => "S'ha reanomenat la pàgina amb èxit, però la pàgina de discussió no s'ha pogut moure car ja no existeix en el títol nou.

Incorporeu-les manualment, si us plau.",
'movedto'                 => 'reanomenat a',
'movetalk'                => 'Mou la pàgina de discussió associada',
'move-subpages'           => "Mou totes les pàgines (si s'escau)",
'move-talk-subpages'      => "Mou totes les subpàgines de la discussió (si s'escau)",
'movepage-page-exists'    => "La pàgina $1 ja existeix i no pot sobreescriure's automàticament.",
'movepage-page-moved'     => 'La pàgina $1 ha estat traslladada a $2.',
'movepage-page-unmoved'   => "La pàgina $1 no s'ha pogut moure a $2.",
'movepage-max-pages'      => "{{PLURAL:$1|S'ha mogut una pàgina|S'han mogut $1 pàgines}} que és el nombre màxim, i per tant no se'n mourà automàticament cap més.",
'1movedto2'               => "[[$1]] s'ha reanomenat com [[$2]]",
'1movedto2_redir'         => "[[$1]] s'ha reanomenat com [[$2]] amb una redirecció",
'movelogpage'             => 'Registre de reanomenaments',
'movelogpagetext'         => 'Vegeu la llista de les darreres pàgines reanomenades.',
'movereason'              => 'Motiu',
'revertmove'              => 'reverteix',
'delete_and_move'         => 'Elimina i trasllada',
'delete_and_move_text'    => "==Cal l'eliminació==

La pàgina de destinació, «[[:$1]]», ja existeix. Voleu eliminar-la per a fer lloc al trasllat?",
'delete_and_move_confirm' => 'Sí, esborra la pàgina',
'delete_and_move_reason'  => "S'ha eliminat per a permetre el reanomenament",
'selfmove'                => "Els títols d'origen i de destinació coincideixen: no és possible de reanomenar una pàgina a si mateixa.",
'immobile_namespace'      => "El títol d'origen o de destinació és d'un tipus especial; no és possible reanomenar pàgines a aquest espai de noms.",
'imagenocrossnamespace'   => 'No es pot moure la imatge a un espai de noms on no li correspon',
'imagetypemismatch'       => 'La nova extensió de fitxer no coincideix amb el seu tipus',
'imageinvalidfilename'    => 'El nom de fitxer indicat no és vàlid',
'fix-double-redirects'    => "Actualitza també les redireccions que apuntin a l'article original",

# Export
'export'            => 'Exporta les pàgines',
'exporttext'        => "Podeu exportar a XML el text i l'historial d'una pàgina en concret o d'un conjunt de pàgines; aleshores el resultat pot importar-se en un altre lloc web basat en wiki amb programari de MediaWiki mitjançant la [[Special:Import|pàgina d'importació]].

Per a exportar pàgines, escriviu els títols que desitgeu al quadre de text de sota, un títol per línia, i seleccioneu si desitgeu o no la versió actual juntament amb totes les versions antigues, amb la pàgina d'historial, o només la pàgina actual amb la informació de la darrera modificació.

En el darrer cas, podeu fer servir un enllaç com ara [[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]] per a la pàgina «[[{{MediaWiki:Mainpage}}]]».",
'exportcuronly'     => "Exporta únicament la versió actual en voltes de l'historial sencer",
'exportnohistory'   => "----
'''Nota:''' s'ha inhabilitat l'exportació sencera d'historial de pàgines mitjançant aquest formulari a causa de problemes de rendiment del servidor.",
'export-submit'     => 'Exporta',
'export-addcattext' => 'Afegeix pàgines de la categoria:',
'export-addcat'     => 'Afegeix',
'export-download'   => 'Ofereix desar com a fitxer',
'export-templates'  => 'Inclou les plantilles',

# Namespace 8 related
'allmessages'               => 'Tots els missatges del sistema',
'allmessagesname'           => 'Etiqueta',
'allmessagesdefault'        => 'Text per defecte',
'allmessagescurrent'        => 'Text actual',
'allmessagestext'           => "Tot seguit hi ha una llista dels missatges del sistema que es troben a l'espai de noms ''MediaWiki''. La traducció genèrica d'aquests missatges no s'hauria de fer localment sinó a la traducció del programari MediaWiki. Si voleu ajudar-hi visiteu [http://www.mediawiki.org/wiki/Localisation MediaWiki Localisation] i [http://translatewiki.net Betawiki].",
'allmessagesnotsupportedDB' => "No es pot processar '''{{ns:special}}:Allmessages''' perquè la variable '''\$wgUseDatabaseMessages''' està desactivada.",
'allmessagesfilter'         => 'Cerca etiqueta de missatge:',
'allmessagesmodified'       => 'Mostra només missatges modificats',

# Thumbnails
'thumbnail-more'           => 'Amplia',
'filemissing'              => 'Fitxer inexistent',
'thumbnail_error'          => "S'ha produït un error en crear la miniatura: $1",
'djvu_page_error'          => "La pàgina DjVu està fora de l'abast",
'djvu_no_xml'              => "No s'ha pogut recollir l'XML per al fitxer DjVu",
'thumbnail_invalid_params' => 'Els paràmetres de les miniatures no són vàlids',
'thumbnail_dest_directory' => "No s'ha pogut crear el directori de destinació",

# Special:Import
'import'                     => 'Importa les pàgines',
'importinterwiki'            => 'Importa interwiki',
'import-interwiki-text'      => "Trieu un web basat en wiki i un títol de pàgina per a importar.
Es conservaran les dates de les versions i els noms dels editors.
Totes les accions d'importació interwiki es conserven al [[Special:Log/import|registre d'importacions]].",
'import-interwiki-history'   => "Copia totes les versions de l'historial d'aquesta pàgina",
'import-interwiki-submit'    => 'Importa',
'import-interwiki-namespace' => "Transfereix les pàgines a l'espai de noms:",
'importtext'                 => "Exporteu el fitxer des del wiki d'origen utilitzant l'[[Special:Export|eina d'exportació]]. 
Deseu-lo al vostre ordinador i carregueu-ne una còpia ací.",
'importstart'                => "S'estan important pàgines...",
'import-revision-count'      => '$1 {{PLURAL:$1|revisió|revisions}}',
'importnopages'              => 'No hi ha cap pàgina per importar.',
'importfailed'               => 'La importació ha fallat: $1',
'importunknownsource'        => "No es reconeix el tipus de la font d'importació",
'importcantopen'             => "No ha estat possible d'obrir el fitxer a importar",
'importbadinterwiki'         => "Enllaç d'interwiki incorrecte",
'importnotext'               => 'Buit o sense text',
'importsuccess'              => "S'ha acabat d'importar.",
'importhistoryconflict'      => "Hi ha un conflicte de versions en l'historial (la pàgina podria haver sigut importada abans)",
'importnosources'            => "No s'ha definit cap font d'origen interwiki i s'ha inhabilitat la càrrega directa d'una còpia de l'historial",
'importnofile'               => "No s'ha pujat cap fitxer d'importació.",
'importuploaderrorsize'      => "La càrrega del fitxer d'importació ha fallat. El fitxer és més gran que la mida de càrrega permesa.",
'importuploaderrorpartial'   => "La càrrega del fitxer d'importació ha fallat. El fitxer s'ha penjat només parcialment.",
'importuploaderrortemp'      => "La càrrega del fitxer d'importació ha fallat. Manca una carpeta temporal.",
'import-parse-failure'       => "error a en importar l'XML",
'import-noarticle'           => 'No hi ha cap pàgina per importar!',
'import-nonewrevisions'      => "Totes les revisions s'havien importat abans.",
'xml-error-string'           => '$1 a la línia $2, columna $3 (byte $4): $5',
'import-upload'              => 'Carrega dades XML',

# Import log
'importlogpage'                    => "Registre d'importació",
'importlogpagetext'                => "Importacions administratives de pàgines amb l'historial des d'altres wikis.",
'import-logentry-upload'           => "s'ha importat [[$1]] per càrrega de fitxers",
'import-logentry-upload-detail'    => '$1 {{PLURAL:$1|revisió|revisions}}',
'import-logentry-interwiki'        => "s'ha importat $1 via interwiki",
'import-logentry-interwiki-detail' => '$1 {{PLURAL:$1|revisió|revisions}} de $2',

# Tooltip help for the actions
'tooltip-pt-userpage'             => "La meua pàgina d'usuari",
'tooltip-pt-anonuserpage'         => "La pàgina d'usuari per la ip que utilitzeu",
'tooltip-pt-mytalk'               => 'La vostra pàgina de discussió.',
'tooltip-pt-anontalk'             => 'Discussió sobre les edicions per aquesta adreça ip.',
'tooltip-pt-preferences'          => 'Les vostres preferències.',
'tooltip-pt-watchlist'            => 'La llista de pàgines de les que estau vigilant els canvis.',
'tooltip-pt-mycontris'            => 'Llista de les vostres contribucions.',
'tooltip-pt-login'                => 'Us animem a registrar-vos, però no és obligatori.',
'tooltip-pt-anonlogin'            => 'Us animem a registrar-vos, però no és obligatori.',
'tooltip-pt-logout'               => "Finalitza la sessió d'usuari",
'tooltip-ca-talk'                 => "Discussió sobre el contingut d'aquesta pàgina.",
'tooltip-ca-edit'                 => 'Podeu editar aquesta pàgina. Si us plau, previsualitzeu abans de desar.',
'tooltip-ca-addsection'           => 'Afegeix un comentari a aquesta discussió.',
'tooltip-ca-viewsource'           => 'Aquesta pàgina està protegida. Podeu veure el seu codi font.',
'tooltip-ca-history'              => "Versions antigues d'aquesta pàgina.",
'tooltip-ca-protect'              => 'Protegeix aquesta pàgina.',
'tooltip-ca-delete'               => 'Elimina aquesta pàgina',
'tooltip-ca-undelete'             => 'Restaura les edicions fetes a aquesta pàgina abans de que fos esborrada.',
'tooltip-ca-move'                 => 'Reanomena aquesta pàgina',
'tooltip-ca-watch'                => 'Afegiu aquesta pàgina a la vostra llista de seguiment.',
'tooltip-ca-unwatch'              => 'Suprimiu aquesta pàgina de la vostra llista de seguiment',
'tooltip-search'                  => 'Cerca en el projecte {{SITENAME}}',
'tooltip-search-go'               => 'Vés a una pàgina amb aquest nom exacte si existeix',
'tooltip-search-fulltext'         => 'Cerca a les pàgines aquest text',
'tooltip-p-logo'                  => 'Pàgina principal',
'tooltip-n-mainpage'              => 'Visiteu la pàgina principal.',
'tooltip-n-portal'                => 'Sobre el projecte, què podeu fer, on podeu trobar coses.',
'tooltip-n-currentevents'         => "Per trobar informació general sobre l'actualitat.",
'tooltip-n-recentchanges'         => 'La llista de canvis recents a la wiki.',
'tooltip-n-randompage'            => 'Vés a una pàgina aleatòria.',
'tooltip-n-help'                  => 'El lloc per esbrinar.',
'tooltip-t-whatlinkshere'         => 'Llista de totes les pàgines viqui que enllacen ací.',
'tooltip-t-recentchangeslinked'   => 'Canvis recents a pàgines que enllacen amb aquesta pàgina.',
'tooltip-feed-rss'                => "Canal RSS d'aquesta pàgina",
'tooltip-feed-atom'               => "Canal Atom d'aquesta pàgina",
'tooltip-t-contributions'         => "Vegeu la llista de contribucions d'aquest usuari.",
'tooltip-t-emailuser'             => 'Envia un correu en aquest usuari.',
'tooltip-t-upload'                => "Càrrega d'imatges o altres fitxers.",
'tooltip-t-specialpages'          => 'Llista de totes les pàgines especials.',
'tooltip-t-print'                 => "Versió per a impressió d'aquesta pàgina",
'tooltip-t-permalink'             => 'Enllaç permanent a aquesta versió de la pàgina',
'tooltip-ca-nstab-main'           => 'Vegeu el contingut de la pàgina.',
'tooltip-ca-nstab-user'           => "Vegeu la pàgina de l'usuari.",
'tooltip-ca-nstab-media'          => "Vegeu la pàgina de l'element multimèdia",
'tooltip-ca-nstab-special'        => 'Aquesta pàgina és una pàgina especial, no podeu editar-la',
'tooltip-ca-nstab-project'        => 'Vegeu la pàgina del projecte',
'tooltip-ca-nstab-image'          => 'Visualitza la pàgina del fitxer',
'tooltip-ca-nstab-mediawiki'      => 'Vegeu el missatge de sistema',
'tooltip-ca-nstab-template'       => 'Vegeu la plantilla',
'tooltip-ca-nstab-help'           => "Vegeu la pàgina d'ajuda",
'tooltip-ca-nstab-category'       => 'Vegeu la pàgina de la categoria',
'tooltip-minoredit'               => 'Marca-ho com una edició menor',
'tooltip-save'                    => 'Desa els vostres canvis',
'tooltip-preview'                 => 'Reviseu els vostres canvis, feu-ho abans de desar res!',
'tooltip-diff'                    => 'Mostra quins canvis heu fet al text',
'tooltip-compareselectedversions' => "Vegeu les diferències entre les dues versions seleccionades d'aquesta pàgina.",
'tooltip-watch'                   => 'Afegiu aquesta pàgina a la vostra llista de seguiment',
'tooltip-recreate'                => 'Recrea la pàgina malgrat hagi estat suprimida',
'tooltip-upload'                  => 'Inicia la càrrega',

# Stylesheets
'common.css'   => '/* Editeu aquest fitxer per personalitzar totes les aparences per al lloc sencer */',
'monobook.css' => "/* Editeu aquest fitxer per personalitzar l'aparença del monobook per a tot el lloc sencer */",

# Scripts
'common.js' => "/* Es carregarà per a tots els usuaris, i per a qualsevol pàgina, el codi JavaScript que hi haja després d'aquesta línia. */",

# Metadata
'nodublincore'      => "S'han inhabilitat les metadades RDF de Dublin Core del servidor.",
'nocreativecommons' => "S'han inhabilitat les metadades RDF de Creative Commons del servidor.",
'notacceptable'     => 'El servidor wiki no pot oferir dades en un format que el client no pot llegir.',

# Attribution
'anonymous'        => 'Usuaris anònims del projecte {{SITENAME}}',
'siteuser'         => 'Usuari $1 del projecte {{SITENAME}}',
'lastmodifiedatby' => 'Va modificar-se la pàgina per darrera vegada el $2, $1 per $3.', # $1 date, $2 time, $3 user
'othercontribs'    => 'Basat en les contribucions de $1.',
'others'           => 'altres',
'siteusers'        => '{{SITENAME}} usuaris $1',
'creditspage'      => 'Títols de la pàgina',
'nocredits'        => 'No hi ha títols disponibles per aquesta pàgina.',

# Spam protection
'spamprotectiontitle' => 'Filtre de protecció de brossa',
'spamprotectiontext'  => 'La pàgina que volíeu desar va ser blocada pel filtre de brossa.
Això deu ser degut per un enllaç a un lloc extern inclòs a la llista negra.',
'spamprotectionmatch' => 'El següent text és el que va disparar el nostre filtre de brossa: $1',
'spambot_username'    => 'Neteja de brossa del MediaWiki',
'spam_reverting'      => 'Es reverteix a la darrera versió que no conté enllaços a $1',
'spam_blanking'       => "Totes les revisions contenien enllaços $1, s'està deixant en blanc",

# Info page
'infosubtitle'   => 'Informació de la pàgina',
'numedits'       => "Nombre d'edicions (pàgina): $1",
'numtalkedits'   => "Nombre d'edicions (pàgina de discussió): $1",
'numwatchers'    => "Nombre d'usuaris que l'estan vigilant: $1",
'numauthors'     => "Nombre d'autors (pàgina): $1",
'numtalkauthors' => "Nombre d'autors (pàgina de discussió): $1",

# Math options
'mw_math_png'    => 'Produeix sempre PNG',
'mw_math_simple' => 'HTML si és molt simple, si no PNG',
'mw_math_html'   => 'HTML si és possible, si no PNG',
'mw_math_source' => 'Deixa com a TeX (per a navegadors de text)',
'mw_math_modern' => 'Recomanat per navegadors moderns',
'mw_math_mathml' => 'MathML si és possible (experimental)',

# Patrolling
'markaspatrolleddiff'                 => 'Marca com a supervisat',
'markaspatrolledtext'                 => 'Marca la pàgina com a supervisada',
'markedaspatrolled'                   => 'Marca com a supervisat',
'markedaspatrolledtext'               => "S'ha marcat la revisió seleccionada com supervisada.",
'rcpatroldisabled'                    => "S'ha inhabilitat la supervisió dels canvis recents",
'rcpatroldisabledtext'                => 'La funció de supervisió de canvis recents està actualment inhabilitada.',
'markedaspatrollederror'              => 'No es pot marcar com a supervisat',
'markedaspatrollederrortext'          => 'Cal que especifiqueu una versió per a marcar-la com a supervisada.',
'markedaspatrollederror-noautopatrol' => 'No podeu marcar les vostres pròpies modificacions com a supervisades.',

# Patrol log
'patrol-log-page'   => 'Registre de supervisió',
'patrol-log-header' => 'Això és un registre de les revisions patrullades.',
'patrol-log-line'   => "s'ha marcat la versió $1 de $2 com a supervisat $3",
'patrol-log-auto'   => '(automàtic)',

# Image deletion
'deletedrevision'                 => "S'ha eliminat la revisió antiga $1.",
'filedeleteerror-short'           => "S'ha produït un error en suprimir el fitxer: $1",
'filedeleteerror-long'            => "S'han produït errors en suprimir el fitxer:

$1",
'filedelete-missing'              => 'No es pot suprimir el fitxer «$1», perquè no existeix.',
'filedelete-old-unregistered'     => 'La revisió de fitxer especificada «$1» no es troba a la base de dades.',
'filedelete-current-unregistered' => 'El fitxer especificat «$1» no es troba a la base de dades.',
'filedelete-archive-read-only'    => "El directori d'arxiu «$1» no té permisos d'escriptura per al servidor web.",

# Browsing diffs
'previousdiff' => "← Vés a l'edició anterior",
'nextdiff'     => "Vés a l'edició següent →",

# Media information
'mediawarning'         => "'''Advertència''': Aquest fitxer podria contenir codi maliciós, si l'executeu podeu comprometre la seguretat del vostre sistema.<hr />",
'imagemaxsize'         => "Limita les imatges de les pàgines de descripció d'imatges a:",
'thumbsize'            => 'Mida de la miniatura:',
'widthheightpage'      => '$1×$2, $3 {{PLURAL:$3|pàgina|pàgines}}',
'file-info'            => '(mida: $1, tipus MIME: $2)',
'file-info-size'       => '($1 × $2 píxels, mida del fitxer: $3, tipus MIME: $4)',
'file-nohires'         => '<small>No hi ha cap versió amb una resolució més gran.</small>',
'svg-long-desc'        => '(fitxer SVG, nominalment $1 × $2 píxels, mida del fitxer: $3)',
'show-big-image'       => 'Imatge en màxima resolució',
'show-big-image-thumb' => "<small>Mida d'aquesta previsualització: $1 × $2 píxels</small>",

# Special:NewImages
'newimages'             => 'Galeria de nous fitxers',
'imagelisttext'         => "Llista {{PLURAL:$1|d'un sol fitxer|de '''$1''' fitxers ordenats $2}}.",
'newimages-summary'     => 'Aquesta pàgina especial mostra els darrers fitxers carregats.',
'showhidebots'          => '($1 bots)',
'noimages'              => 'Res per veure.',
'ilsubmit'              => 'Cerca',
'bydate'                => 'per data',
'sp-newimages-showfrom' => 'Mostra fitxers nous des del $1 a les $2',

# Video information, used by Language::formatTimePeriod() to format lengths in the above messages
'minutes-abbrev' => 'min',

# Bad image list
'bad_image_list' => "El format ha de ser el següent:

Només els elements de llista (les línies que comencin amb un *) es prenen en consideració. El primer enllaç de cada línia ha de ser el d'un fitxer dolent.
La resta d'enllaços de la línia són les excepcions, és a dir, les pàgines on s'hi pot encabir el fitxer.",

# Metadata
'metadata'          => 'Metadades',
'metadata-help'     => "Aquest fitxer conté informació addicional, probablement afegida per la càmera digital o l'escàner utilitzat per a crear-lo o digitalitzar-lo. Si s'ha modificat posteriorment, alguns detalls poden no reflectir les dades reals del fitxer modificat.",
'metadata-expand'   => 'Mostra els detalls estesos',
'metadata-collapse' => 'Amaga els detalls estesos',
'metadata-fields'   => 'Els camps de metadades EXIF llistats en aquest missatge es mostraran en la pàgina de descripció de la imatge fins i tot quan la taula estigui plegada. La resta estaran ocults però es podran desplegar.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => 'Amplada',
'exif-imagelength'                 => 'Alçada',
'exif-bitspersample'               => 'Octets per component',
'exif-compression'                 => 'Esquema de compressió',
'exif-photometricinterpretation'   => 'Composició dels píxels',
'exif-orientation'                 => 'Orientació',
'exif-samplesperpixel'             => 'Nombre de components',
'exif-planarconfiguration'         => 'Ordenament de dades',
'exif-ycbcrsubsampling'            => 'Proporció de mostreig secundari de Y amb C',
'exif-ycbcrpositioning'            => 'Posició YCbCr',
'exif-xresolution'                 => 'Resolució horitzontal',
'exif-yresolution'                 => 'Resolució vertical',
'exif-resolutionunit'              => 'Unitats de les resolucions X i Y',
'exif-stripoffsets'                => 'Ubicació de les dades de la imatge',
'exif-rowsperstrip'                => 'Nombre de fileres per franja',
'exif-stripbytecounts'             => 'Octets per franja comprimida',
'exif-jpeginterchangeformat'       => 'Ancorament del JPEG SOI',
'exif-jpeginterchangeformatlength' => 'Octets de dades JPEG',
'exif-transferfunction'            => 'Funció de transferència',
'exif-whitepoint'                  => 'Cromositat del punt blanc',
'exif-primarychromaticities'       => 'Coordenada cromàtica del color primari',
'exif-ycbcrcoefficients'           => "Quoficients de la matriu de transformació de l'espai colorimètric",
'exif-referenceblackwhite'         => 'Valors de referència negre i blanc',
'exif-datetime'                    => 'Data i hora de modificació del fitxer',
'exif-imagedescription'            => 'Títol de la imatge',
'exif-make'                        => 'Fabricant de la càmera',
'exif-model'                       => 'Model de càmera',
'exif-software'                    => 'Programari utilitzat',
'exif-artist'                      => 'Autor',
'exif-copyright'                   => "Titular dels drets d'autor",
'exif-exifversion'                 => 'Versió Exif',
'exif-flashpixversion'             => 'Versió Flashpix admesa',
'exif-colorspace'                  => 'Espai de color',
'exif-componentsconfiguration'     => 'Significat de cada component',
'exif-compressedbitsperpixel'      => "Mode de compressió d'imatge",
'exif-pixelydimension'             => 'Amplada de la imatge',
'exif-pixelxdimension'             => 'Alçada de la imatge',
'exif-makernote'                   => 'Notes del fabricant',
'exif-usercomment'                 => "Comentaris de l'usuari",
'exif-relatedsoundfile'            => "Fitxer d'àudio relacionat",
'exif-datetimeoriginal'            => 'Dia i hora de generació de les dades',
'exif-datetimedigitized'           => 'Dia i hora de digitalització',
'exif-subsectime'                  => 'Data i hora, fraccions de segon',
'exif-subsectimeoriginal'          => 'Data i hora de creació, fraccions de segon',
'exif-subsectimedigitized'         => 'Data i hora de digitalització, fraccions de segon',
'exif-exposuretime'                => "Temps d'exposició",
'exif-exposuretime-format'         => '$1 s ($2)',
'exif-fnumber'                     => 'Obertura del diafragma',
'exif-exposureprogram'             => "Programa d'exposició",
'exif-spectralsensitivity'         => 'Sensibilitat espectral',
'exif-isospeedratings'             => 'Sensibilitat ISO',
'exif-oecf'                        => 'Factor de conversió optoelectrònic',
'exif-shutterspeedvalue'           => "Temps d'exposició",
'exif-aperturevalue'               => 'Obertura',
'exif-brightnessvalue'             => 'Brillantor',
'exif-exposurebiasvalue'           => "Correcció d'exposició",
'exif-maxaperturevalue'            => "Camp d'obertura màxim",
'exif-subjectdistance'             => 'Distància del subjecte',
'exif-meteringmode'                => 'Mode de mesura',
'exif-lightsource'                 => 'Font de llum',
'exif-flash'                       => 'Flaix',
'exif-focallength'                 => 'Longitud focal de la lent',
'exif-subjectarea'                 => 'Enquadre del subjecte',
'exif-flashenergy'                 => 'Energia del flash',
'exif-spatialfrequencyresponse'    => 'Resposta en freqüència espacial',
'exif-focalplanexresolution'       => 'Resolució X del pla focal',
'exif-focalplaneyresolution'       => 'Resolució Y del pla focal',
'exif-focalplaneresolutionunit'    => 'Unitat de resolució del pla focal',
'exif-subjectlocation'             => 'Posició del subjecte',
'exif-exposureindex'               => "Índex d'exposició",
'exif-sensingmethod'               => 'Mètode de detecció',
'exif-filesource'                  => 'Font del fitxer',
'exif-scenetype'                   => "Tipus d'escena",
'exif-cfapattern'                  => 'Patró CFA',
'exif-customrendered'              => "Processament d'imatge personalitzat",
'exif-exposuremode'                => "Mode d'exposició",
'exif-whitebalance'                => 'Balanç de blancs',
'exif-digitalzoomratio'            => "Escala d'ampliació digital (zoom)",
'exif-focallengthin35mmfilm'       => 'Distància focal per a pel·lícula de 35 mm',
'exif-scenecapturetype'            => "Tipus de captura d'escena",
'exif-gaincontrol'                 => "Control d'escena",
'exif-contrast'                    => 'Contrast',
'exif-saturation'                  => 'Saturació',
'exif-sharpness'                   => 'Nitidesa',
'exif-devicesettingdescription'    => 'Descripció dels paràmetres del dispositiu',
'exif-subjectdistancerange'        => 'Escala de distància del subjecte',
'exif-imageuniqueid'               => 'Identificador únic de la imatge',
'exif-gpsversionid'                => 'Versió del tag GPS',
'exif-gpslatituderef'              => 'Latitud nord o sud',
'exif-gpslatitude'                 => 'Latitud',
'exif-gpslongituderef'             => 'Longitud est o oest',
'exif-gpslongitude'                => 'Longitud',
'exif-gpsaltituderef'              => "Referència d'altitud",
'exif-gpsaltitude'                 => 'Altitud',
'exif-gpstimestamp'                => 'Hora GPS (rellotge atòmic)',
'exif-gpssatellites'               => 'Satèl·lits utilitzats en la mesura',
'exif-gpsstatus'                   => 'Estat del receptor',
'exif-gpsmeasuremode'              => 'Mode de mesura',
'exif-gpsdop'                      => 'Precisió de la mesura',
'exif-gpsspeedref'                 => 'Unitats de velocitat',
'exif-gpsspeed'                    => 'Velocitat del receptor GPS',
'exif-gpstrackref'                 => 'Referència per la direcció del moviment',
'exif-gpstrack'                    => 'Direcció del moviment',
'exif-gpsimgdirectionref'          => 'Referència per la direcció de la imatge',
'exif-gpsimgdirection'             => 'Direcció de la imatge',
'exif-gpsmapdatum'                 => "S'han utilitzat dades d'informes geodètics",
'exif-gpsdestlatituderef'          => 'Referència per a la latitud de la destinació',
'exif-gpsdestlatitude'             => 'Latitud de la destinació',
'exif-gpsdestlongituderef'         => 'Referència per a la longitud de la destinació',
'exif-gpsdestlongitude'            => 'Longitud de la destinació',
'exif-gpsdestbearingref'           => "Referència per a l'orientació de la destinació",
'exif-gpsdestbearing'              => 'Orientació de la destinació',
'exif-gpsdestdistanceref'          => 'Referència de la distància a la destinació',
'exif-gpsdestdistance'             => 'Distància a la destinació',
'exif-gpsprocessingmethod'         => 'Nom del mètode de processament GPS',
'exif-gpsareainformation'          => "Nom de l'àrea GPS",
'exif-gpsdatestamp'                => 'Data GPS',
'exif-gpsdifferential'             => 'Correcció diferencial GPS',

# EXIF attributes
'exif-compression-1' => 'Sense compressió',

'exif-unknowndate' => 'Data desconeguda',

'exif-orientation-1' => 'Normal', # 0th row: top; 0th column: left
'exif-orientation-2' => 'Invertit horitzontalment', # 0th row: top; 0th column: right
'exif-orientation-3' => 'Girat 180°', # 0th row: bottom; 0th column: right
'exif-orientation-4' => 'Invertit verticalment', # 0th row: bottom; 0th column: left
'exif-orientation-5' => 'Rotat 90° en sentit antihorari i invertit verticalment', # 0th row: left; 0th column: top
'exif-orientation-6' => 'Rotat 90° en sentit horari', # 0th row: right; 0th column: top
'exif-orientation-7' => 'Rotat 90° en sentit horari i invertit verticalment', # 0th row: right; 0th column: bottom
'exif-orientation-8' => 'Rotat 90° en sentit antihorari', # 0th row: left; 0th column: bottom

'exif-planarconfiguration-1' => 'a blocs densos (chunky)',
'exif-planarconfiguration-2' => 'format pla',

'exif-xyresolution-i' => '$1 ppp',
'exif-xyresolution-c' => '$1 ppc',

'exif-componentsconfiguration-0' => 'no existeix',

'exif-exposureprogram-0' => 'No definit',
'exif-exposureprogram-1' => 'Manual',
'exif-exposureprogram-2' => 'Programa normal',
'exif-exposureprogram-3' => "amb prioritat d'obertura",
'exif-exposureprogram-4' => "amb prioritat de velocitat d'obturació",
'exif-exposureprogram-5' => 'Programa creatiu (preferència a la profunditat de camp)',
'exif-exposureprogram-6' => "Programa acció (preferència a la velocitat d'obturació)",
'exif-exposureprogram-7' => 'Mode retrat (per primers plans amb fons desenfocat)',
'exif-exposureprogram-8' => 'Mode paisatge (per fotos de paisatges amb el fons enfocat)',

'exif-subjectdistance-value' => '$1 metres',

'exif-meteringmode-0'   => 'Desconegut',
'exif-meteringmode-1'   => 'Mitjana',
'exif-meteringmode-2'   => 'Mesura central mitjana',
'exif-meteringmode-3'   => 'Puntual',
'exif-meteringmode-4'   => 'Multipuntual',
'exif-meteringmode-5'   => 'Patró',
'exif-meteringmode-6'   => 'Parcial',
'exif-meteringmode-255' => 'Altres',

'exif-lightsource-0'   => 'Desconegut',
'exif-lightsource-1'   => 'Llum de dia',
'exif-lightsource-2'   => 'Fluorescent',
'exif-lightsource-3'   => 'Tungstè (llum incandescent)',
'exif-lightsource-4'   => 'Flaix',
'exif-lightsource-9'   => 'Clar',
'exif-lightsource-10'  => 'Ennuvolat',
'exif-lightsource-11'  => 'Ombra',
'exif-lightsource-12'  => 'Fluorescent de llum del dia (D 5700 – 7100K)',
'exif-lightsource-13'  => 'Fluorescent de llum blanca (N 4600 – 5400K)',
'exif-lightsource-14'  => 'Fluorescent blanc fred (W 3900 – 4500K)',
'exif-lightsource-15'  => 'Fluorescent blanc (WW 3200 – 3700K)',
'exif-lightsource-17'  => 'Llum estàndard A',
'exif-lightsource-18'  => 'Llum estàndard B',
'exif-lightsource-19'  => 'Llum estàndard C',
'exif-lightsource-24'  => "Bombeta de tungstè d'estudi ISO",
'exif-lightsource-255' => 'Altre font de llum',

'exif-focalplaneresolutionunit-2' => 'polzades',

'exif-sensingmethod-1' => 'Indefinit',
'exif-sensingmethod-2' => "Sensor d'àrea de color a un xip",
'exif-sensingmethod-3' => "Sensor d'àrea de color a dos xips",
'exif-sensingmethod-4' => "Sensor d'àrea de color a tres xips",
'exif-sensingmethod-5' => "Sensor d'àrea de color per seqüències",
'exif-sensingmethod-7' => 'Sensor trilineal',
'exif-sensingmethod-8' => 'Sensor linear de color per seqüències',

'exif-scenetype-1' => 'Una imatge fotografiada directament',

'exif-customrendered-0' => 'Procés normal',
'exif-customrendered-1' => 'Processament personalitzat',

'exif-exposuremode-0' => 'Exposició automàtica',
'exif-exposuremode-1' => 'Exposició manual',
'exif-exposuremode-2' => 'Bracketting automàtic',

'exif-whitebalance-0' => 'Balanç automàtic de blancs',
'exif-whitebalance-1' => 'Balanç manual de blancs',

'exif-scenecapturetype-0' => 'Estàndard',
'exif-scenecapturetype-1' => 'Paisatge',
'exif-scenecapturetype-2' => 'Retrat',
'exif-scenecapturetype-3' => 'Escena nocturna',

'exif-gaincontrol-0' => 'Cap',
'exif-gaincontrol-1' => 'Baix augment del guany',
'exif-gaincontrol-2' => 'Fort augment del guany',
'exif-gaincontrol-3' => 'Baixa reducció del guany',
'exif-gaincontrol-4' => 'Fort augment del guany',

'exif-contrast-0' => 'Normal',
'exif-contrast-1' => 'Suau',
'exif-contrast-2' => 'Fort',

'exif-saturation-0' => 'Normal',
'exif-saturation-1' => 'Baixa saturació',
'exif-saturation-2' => 'Alta saturació',

'exif-sharpness-0' => 'Normal',
'exif-sharpness-1' => 'Suau',
'exif-sharpness-2' => 'Fort',

'exif-subjectdistancerange-0' => 'Desconeguda',
'exif-subjectdistancerange-1' => 'Macro',
'exif-subjectdistancerange-2' => 'Subjecte a prop',
'exif-subjectdistancerange-3' => 'Subjecte lluny',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'Latitud nord',
'exif-gpslatitude-s' => 'Latitud sud',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'Longitud est',
'exif-gpslongitude-w' => 'Longitud oest',

'exif-gpsstatus-a' => 'Mesura en curs',
'exif-gpsstatus-v' => 'Interoperabilitat de mesura',

'exif-gpsmeasuremode-2' => 'Mesura bidimensional',
'exif-gpsmeasuremode-3' => 'Mesura tridimensional',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'Quilòmetres per hora',
'exif-gpsspeed-m' => 'Milles per hora',
'exif-gpsspeed-n' => 'Nusos',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => 'Direcció real',
'exif-gpsdirection-m' => 'Direcció magnètica',

# External editor support
'edit-externally'      => 'Edita aquest fitxer fent servir una aplicació externa',
'edit-externally-help' => 'Vegeu les [http://www.mediawiki.org/wiki/Manual:External_editors instruccions de configuració] per a més informació.',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'tots',
'imagelistall'     => 'totes',
'watchlistall2'    => 'totes',
'namespacesall'    => 'tots',
'monthsall'        => 'tots',

# E-mail address confirmation
'confirmemail'             => "Confirma l'adreça de correu electrònic",
'confirmemail_noemail'     => "No heu introduït una direcció vàlida de correu electrònic en les vostres [[Special:Preferences|preferències d'usuari]].",
'confirmemail_text'        => "El projecte {{SITENAME}} necessita que valideu la vostra adreça de correu
electrònic per a poder gaudir d'algunes facilitats. Cliqueu el botó inferior
per a enviar un codi de confirmació a la vostra adreça. Seguiu l'enllaç que
hi haurà al missatge enviat per a confirmar que el vostre correu és correcte.",
'confirmemail_pending'     => "<div class=\"error\">
Ja s'ha enviat el vostre codi de confirmació per correu electrònic; si
fa poc hi heu creat el vostre compte, abans de mirar de demanar un nou
codi, primer hauríeu d'esperar alguns minuts per a rebre'l.
</div>",
'confirmemail_send'        => 'Envia per correu electrònic un codi de confirmació',
'confirmemail_sent'        => "S'ha enviat un missatge de confirmació.",
'confirmemail_oncreate'    => "S'ha enviat un codi de confirmació a la vostra adreça de correu electrònic.
No es requereix aquest codi per a autenticar-s'hi, però vos caldrà proporcionar-lo
abans d'activar qualsevol funcionalitat del wiki basada en missatges
de correu electrònic.",
'confirmemail_sendfailed'  => "{{SITENAME}} no ha pogut enviar el vostre missatge de confirmació.
Comproveu que l'adreça no tingui caràcters no vàlids.

El programari de correu retornà el següent missatge: $1",
'confirmemail_invalid'     => 'El codi de confirmació no és vàlid. Aquest podria haver vençut.',
'confirmemail_needlogin'   => 'Necessiteu $1 per a confirmar la vostra adreça electrònica.',
'confirmemail_success'     => "S'ha confirmat la vostra adreça electrònica. Ara podeu iniciar una sessió i gaudir del wiki.",
'confirmemail_loggedin'    => "Ja s'ha confirmat la vostra adreça electrònica.",
'confirmemail_error'       => 'Quelcom ha fallat en desar la vostra confirmació.',
'confirmemail_subject'     => "Confirmació de l'adreça electrònica del projecte {{SITENAME}}",
'confirmemail_body'        => "Algú, segurament vós, ha registrat el compte «$2» al projecte {{SITENAME}}
amb aquesta adreça electrònica des de l'adreça IP $1.

Per a confirmar que aquesta adreça electrònica us pertany realment
i així activar les opcions de correu del programari, seguiu aquest enllaç:

$3

Si *no* heu estat qui ho ha fet, seguiu aquest altre enllaç per a cancel·lar la confirmació demanada:

$5

Aquest codi de confirmació caducarà a $4.",
'confirmemail_invalidated' => "Confirmació d'adreça electrònica cancel·lada",
'invalidateemail'          => "Cancel·la la confirmació d'adreça electrònica",

# Scary transclusion
'scarytranscludedisabled' => "[S'ha inhabilitat la transclusió interwiki]",
'scarytranscludefailed'   => '[Ha fallat la recuperació de la plantilla per a $1]',
'scarytranscludetoolong'  => "[L'URL és massa llarg]",

# Trackbacks
'trackbackbox'      => '<div id="mw_trackbacks">
Referències d\'aquesta pàgina:<br />
$1
</div>',
'trackbackremove'   => ' ([$1 eliminada])',
'trackbacklink'     => 'Referència',
'trackbackdeleteok' => "La referència s'ha eliminat amb èxit.",

# Delete conflict
'deletedwhileediting' => "'''Avís''': S'ha eliminat aquesta pàgina abans que haguéssiu començat a editar-la!",
'confirmrecreate'     => "L'usuari [[User:$1|$1]] ([[User talk:$1|discussió]]) va eliminar aquesta pàgina que havíeu creat donant-ne el següent motiu:
: ''$2''
Confirmeu que realment voleu tornar-la a crear.",
'recreate'            => 'Torna a crear',

# HTML dump
'redirectingto' => "S'està redirigint a [[:$1]]...",

# action=purge
'confirm_purge'        => "Voleu buidar la memòria cau d'aquesta pàgina?

$1",
'confirm_purge_button' => "D'acord",

# AJAX search
'searchcontaining' => "Cerca pàgines que continguin ''$1''.",
'searchnamed'      => "Cerca pàgines que s'anomenin ''$1''.",
'articletitles'    => "Pàgines que comencen amb ''$1''",
'hideresults'      => 'Amaga els resultats',
'useajaxsearch'    => 'Utilitza la cerca en AJAX',

# Multipage image navigation
'imgmultipageprev' => '← pàgina anterior',
'imgmultipagenext' => 'pàgina següent →',
'imgmultigo'       => 'Vés-hi!',
'imgmultigoto'     => 'Vés a la pàgina $1',

# Table pager
'ascending_abbrev'         => 'asc',
'descending_abbrev'        => 'desc',
'table_pager_next'         => 'Pàgina següent',
'table_pager_prev'         => 'Pàgina anterior',
'table_pager_first'        => 'Primera pàgina',
'table_pager_last'         => 'Darrera pàgina',
'table_pager_limit'        => 'Mostra $1 elements per pàgina',
'table_pager_limit_submit' => 'Vés-hi',
'table_pager_empty'        => 'Sense resultats',

# Auto-summaries
'autosumm-blank'   => "S'ha suprimit tot el contingut de la pàgina",
'autosumm-replace' => 'Contingut canviat per «$1».',
'autoredircomment' => 'Redirecció a [[$1]]',
'autosumm-new'     => 'Pàgina nova, amb el contingut: «$1».',

# Live preview
'livepreview-loading' => "S'està carregant…",
'livepreview-ready'   => "S'està carregant… Preparat!",
'livepreview-failed'  => 'Ha fallat la vista ràpida!
Proveu-ho amb la previsualització normal.',
'livepreview-error'   => 'La connexió no ha estat possible: $1 «$2»
Proveu-ho amb la previsualització normal.',

# Friendlier slave lag warnings
'lag-warn-normal' => 'Els canvis més nous de $1 {{PLURAL:$1|segon|segons}} podrien no mostrar-se a la llista.',
'lag-warn-high'   => 'A causa de la lenta resposta del servidor de base de dades, els canvis més nous de $1 {{PLURAL:$1|segon|segons}} potser no es mostren aquesta llista.',

# Watchlist editor
'watchlistedit-numitems'       => 'La vostra llista de seguiment conté {{PLURAL:$1|1 títol|$1 títols}}, excloent-ne les pàgines de discussió.',
'watchlistedit-noitems'        => 'La vostra llista de seguiment no té cap títol.',
'watchlistedit-normal-title'   => 'Edita la llista de seguiment',
'watchlistedit-normal-legend'  => 'Esborra els títols de la llista de seguiment',
'watchlistedit-normal-explain' => 'Els títols de la vostra llista de seguiment es mostren a continuació. Per a eliminar un títol, marqueu
	el quadre del costat, i feu clic a Elimina els títols. Podeu també [[Special:Watchlist/raw|editar-ne la llista crua]].',
'watchlistedit-normal-submit'  => 'Esborra els títols',
'watchlistedit-normal-done'    => "{{PLURAL:$1|1 títol s'ha|$1 títols s'han}} eliminat de la vostra llista de seguiment:",
'watchlistedit-raw-title'      => 'Edita la llista de seguiment crua',
'watchlistedit-raw-legend'     => 'Edita la llista de seguiment crua',
'watchlistedit-raw-explain'    => "Els títols de la vostra llista de seguiment es mostren a continuació, i poden editar-se afegint-los o suprimint-los de la llista; un títol per línia. En acabar, feu clic a Actualitza la llista de seguiment.
També podeu [[Special:Watchlist/edit|utilitzar l'editor estàndard]].",
'watchlistedit-raw-titles'     => 'Títols:',
'watchlistedit-raw-submit'     => 'Actualitza la llista de seguiment',
'watchlistedit-raw-done'       => "S'ha actualitzat la vostra llista de seguiment.",
'watchlistedit-raw-added'      => "{{PLURAL:$1|1 títol s'ha|$1 títols s'han}} afegit:",
'watchlistedit-raw-removed'    => "{{PLURAL:$1|1 títol s'ha|$1 títols s'han}} eliminat:",

# Watchlist editing tools
'watchlisttools-view' => 'Visualitza els canvis rellevants',
'watchlisttools-edit' => 'Visualitza i edita la llista de seguiment',
'watchlisttools-raw'  => 'Edita la llista de seguiment sense format',

# Core parser functions
'unknown_extension_tag' => "Etiqueta d'extensió desconeguda «$1»",

# Special:Version
'version'                          => 'Versió', # Not used as normal message but as header for the special page itself
'version-extensions'               => 'Extensions instaŀlades',
'version-specialpages'             => 'Pàgines especials',
'version-parserhooks'              => "Lligams de l'analitzador",
'version-variables'                => 'Variables',
'version-other'                    => 'Altres',
'version-mediahandlers'            => 'Connectors multimèdia',
'version-hooks'                    => 'Lligams',
'version-extension-functions'      => "Funcions d'extensió",
'version-parser-extensiontags'     => "Etiquetes d'extensió de l'analitzador",
'version-parser-function-hooks'    => "Lligams funcionals de l'analitzador",
'version-skin-extension-functions' => "Funcions d'extensió per l'aparença (skin)",
'version-hook-name'                => 'Nom del lligam',
'version-hook-subscribedby'        => 'Subscrit per',
'version-version'                  => 'Versió',
'version-license'                  => 'Llicència',
'version-software'                 => 'Programari instal·lat',
'version-software-product'         => 'Producte',
'version-software-version'         => 'Versió',

# Special:FilePath
'filepath'         => 'Camí del fitxer',
'filepath-page'    => 'Fitxer:',
'filepath-submit'  => 'Camí',
'filepath-summary' => "Aquesta pàgina especial retorna un camí complet d'un fitxer.
Les imatges es mostren en plena resolució; altres tipus de fitxer s'incien amb el seu programa associat directament.

Introduïu el nom del fitxer sense el prefix «{{ns:image}}»:",

# Special:FileDuplicateSearch
'fileduplicatesearch'          => 'Cerca fitxers duplicats',
'fileduplicatesearch-summary'  => "Cerca fitxers duplicats d'acord amb el seu valor de resum.

Introduïu el nom del fitxer sense el prefix «{{ns:image}}:».",
'fileduplicatesearch-legend'   => 'Cerca duplicats',
'fileduplicatesearch-filename' => 'Nom del fitxer:',
'fileduplicatesearch-submit'   => 'Cerca',
'fileduplicatesearch-info'     => '$1 × $2 píxels<br />Mida del fitxer: $3<br />Tipus MIME: $4',
'fileduplicatesearch-result-1' => 'El fitxer «$1» no té cap duplicació idèntica.',
'fileduplicatesearch-result-n' => 'El fitxer «$1» té {{PLURAL:$2|1 duplicació idèntica|$2 duplicacions idèntiques}}.',

# Special:SpecialPages
'specialpages'                   => 'Pàgines especials',
'specialpages-note'              => '----
* Pàgines especials normals.
* <span class="mw-specialpagerestricted">Pàgines especials restringides.</span>',
'specialpages-group-maintenance' => 'Informes de manteniment',
'specialpages-group-other'       => 'Altres pàgines especials',
'specialpages-group-login'       => 'Inici de sessió / Registre',
'specialpages-group-changes'     => 'Canvis recents i registres',
'specialpages-group-media'       => 'Informes multimèdia i càrregues',
'specialpages-group-users'       => 'Usuaris i drets',
'specialpages-group-highuse'     => "Pàgines d'alt ús",
'specialpages-group-pages'       => 'Llista de pàgines',
'specialpages-group-pagetools'   => "Pàgines d'eines",
'specialpages-group-wiki'        => 'Eines i dades del wiki',
'specialpages-group-redirects'   => 'Pàgines especials de redirecció',
'specialpages-group-spam'        => 'Eines de spam',

# Special:BlankPage
'blankpage'              => 'Pàgina en blanc',
'intentionallyblankpage' => 'Pàgina intencionadament en blanc',

);
