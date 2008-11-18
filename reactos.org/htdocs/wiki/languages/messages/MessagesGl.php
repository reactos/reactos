<?php
/** Galician (Galego)
 *
 * @ingroup Language
 * @file
 *
 * @author Alma
 * @author Lameiro
 * @author Prevert
 * @author Toliño
 * @author Xosé
 * @author לערי ריינהארט
 */

$namespaceNames = array(
	NS_MEDIA          => 'Media',
	NS_SPECIAL        => 'Especial',
	NS_MAIN           => '',
	NS_TALK           => 'Conversa',
	NS_USER           => 'Usuario',
	NS_USER_TALK      => 'Conversa_Usuario',
	# NS_PROJECT set by \$wgMetaNamespace
	NS_PROJECT_TALK   => 'Conversa_$1',
	NS_IMAGE          => 'Imaxe',
	NS_IMAGE_TALK     => 'Conversa_Imaxe',
	NS_MEDIAWIKI      => 'MediaWiki',
	NS_MEDIAWIKI_TALK => 'Conversa_MediaWiki',
	NS_TEMPLATE       => 'Modelo',
	NS_TEMPLATE_TALK  => 'Conversa_Modelo',
	NS_HELP           => 'Axuda',
	NS_HELP_TALK      => 'Conversa_Axuda',
	NS_CATEGORY       => 'Categoría',
	NS_CATEGORY_TALK  => 'Conversa_Categoría',
);

$messages = array(
# User preference toggles
'tog-underline'               => 'Subliñar ligazóns:',
'tog-highlightbroken'         => 'Darlles formato ás ligazóns crebadas <a href="" class="new">deste xeito</a> (alternativa: así<a href="" class="internal">?</a>).',
'tog-justify'                 => 'Xustificar parágrafos',
'tog-hideminor'               => 'Agochar as edicións pequenas na páxina de cambios recentes',
'tog-extendwatchlist'         => 'Listaxe de vixilancia estendida',
'tog-usenewrc'                => 'Cambios recentes avanzados (JavaScript)',
'tog-numberheadings'          => 'Numerar automaticamente as cabeceiras',
'tog-showtoolbar'             => 'Mostrar a caixa de ferramentas de edición (JavaScript)',
'tog-editondblclick'          => 'Editar as páxinas logo de facer dobre clic (JavaScript)',
'tog-editsection'             => 'Permitir a edición de seccións vía as ligazóns [editar]',
'tog-editsectiononrightclick' => 'Permitir a edición de seccións premendo co botón dereito <br /> nos títulos das seccións (JavaScript)',
'tog-showtoc'                 => 'Mostrar o índice (para páxinas con máis de tres cabeceiras)',
'tog-rememberpassword'        => 'Lembrar o meu contrasinal neste ordenador',
'tog-editwidth'               => 'A caixa de edición ten largo total',
'tog-watchcreations'          => 'Engadir as páxinas creadas por min á miña listaxe de artigos vixiados',
'tog-watchdefault'            => 'Engadir as páxinas que edite á miña listaxe de vixilancia',
'tog-watchmoves'              => 'Engadir as páxinas que mova á miña listaxe de vixilancia',
'tog-watchdeletion'           => 'Engadir as páxinas que borre á miña listaxe de vixilancia',
'tog-minordefault'            => 'Marcar por omisión todas as edicións como pequenas',
'tog-previewontop'            => 'Mostrar o botón de vista previa antes da caixa de edición e non despois dela',
'tog-previewonfirst'          => 'Mostrar a vista previa na primeira edición',
'tog-nocache'                 => 'Deshabilitar a memoria caché das páxinas',
'tog-enotifwatchlistpages'    => 'Envíenme unha mensaxe de correo electrónico cando unha páxina da miña listaxe de vixilancia cambie',
'tog-enotifusertalkpages'     => 'Envíenme unha mensaxe de correo electrónico cando a miña páxina de conversa cambie',
'tog-enotifminoredits'        => 'Envíenme tamén unha mensaxe de correo electrónico cando se produzan pequenos cambios nas páxinas',
'tog-enotifrevealaddr'        => 'Revelar o meu enderezo de correo electrónico nos correos de notificación',
'tog-shownumberswatching'     => 'Mostrar o número de usuarios que están a vixiar',
'tog-fancysig'                => 'Sinatura tal como está, sen ligazón automática',
'tog-externaleditor'          => 'Usar un editor externo por omisión (só para expertos, precisa duns parámetros especiais no seu computador)',
'tog-externaldiff'            => 'Usar diferenzas externas (dif) por omisión (só para expertos, precisa duns parámetros especiais no seu computador)',
'tog-showjumplinks'           => 'Permitir as ligazóns de accesibilidade "ir a"',
'tog-uselivepreview'          => 'Usar <i>live preview</i> (JavaScript) (Experimental)',
'tog-forceeditsummary'        => 'Avisarme cando o campo resumo estea baleiro',
'tog-watchlisthideown'        => 'Agochar as edicións propias na listaxe de vixilancia',
'tog-watchlisthidebots'       => 'Agochar as edicións dos bots na listaxe de vixilancia',
'tog-watchlisthideminor'      => 'Agochar as edicións pequenas na listaxe de vixilancia',
'tog-nolangconversion'        => 'Desactivar a conversión de variantes',
'tog-ccmeonemails'            => 'Enviar ao meu enderezo copia das mensaxes que envíe a outros usuarios',
'tog-diffonly'                => 'Non mostrar o contido da páxina debaixo das diferenzas entre edicións (dif)',
'tog-showhiddencats'          => 'Mostrar as categorías ocultas',

'underline-always'  => 'Sempre',
'underline-never'   => 'Nunca',
'underline-default' => 'Opción do propio navegador',

'skinpreview' => '(Probar)',

# Dates
'sunday'        => 'Domingo',
'monday'        => 'Luns',
'tuesday'       => 'Martes',
'wednesday'     => 'Mércores',
'thursday'      => 'Xoves',
'friday'        => 'Venres',
'saturday'      => 'Sábado',
'sun'           => 'Dom',
'mon'           => 'Lun',
'tue'           => 'Mar',
'wed'           => 'Mér',
'thu'           => 'Xov',
'fri'           => 'Ven',
'sat'           => 'Sáb',
'january'       => 'xaneiro',
'february'      => 'febreiro',
'march'         => 'marzo',
'april'         => 'abril',
'may_long'      => 'maio',
'june'          => 'xuño',
'july'          => 'xullo',
'august'        => 'agosto',
'september'     => 'setembro',
'october'       => 'outubro',
'november'      => 'novembro',
'december'      => 'decembro',
'january-gen'   => 'Xaneiro',
'february-gen'  => 'Febreiro',
'march-gen'     => 'Marzo',
'april-gen'     => 'Abril',
'may-gen'       => 'Maio',
'june-gen'      => 'Xuño',
'july-gen'      => 'Xullo',
'august-gen'    => 'Agosto',
'september-gen' => 'Setembro',
'october-gen'   => 'Outubro',
'november-gen'  => 'Novembro',
'december-gen'  => 'Decembro',
'jan'           => 'Xan',
'feb'           => 'Feb',
'mar'           => 'Mar',
'apr'           => 'Abr',
'may'           => 'Mai',
'jun'           => 'Xuñ',
'jul'           => 'Xul',
'aug'           => 'Ago',
'sep'           => 'Set',
'oct'           => 'Out',
'nov'           => 'Nov',
'dec'           => 'Dec',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|Categoría|Categorías}}',
'category_header'                => 'Artigos na categoría "$1"',
'subcategories'                  => 'Subcategorías',
'category-media-header'          => 'Multimedia na categoría "$1"',
'category-empty'                 => "''Actualmente esta categoría non conta con ningunha páxina ou ficheiro multimedia.''",
'hidden-categories'              => '{{PLURAL:$1|Categoría oculta|Categorías ocultas}}',
'hidden-category-category'       => 'Categorías ocultas', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|Esta categoría só ten a seguinte subcategoría.|Esta categoría ten {{PLURAL:$1|a seguinte subcategoría|as seguintes $1 subcategorías}}, dun total de $2.}}',
'category-subcat-count-limited'  => 'Esta categoría ten {{PLURAL:$1|a seguinte subcategoría|as seguintes $1 subcategorías}}.',
'category-article-count'         => '{{PLURAL:$2|Esta categoría só contén a seguinte páxina.|{{PLURAL:$1|A seguinte páxina está|As seguintes $1 páxinas están}} nesta categoría, dun total de $2.}}',
'category-article-count-limited' => '{{PLURAL:$1|A seguinte páxina está|As seguintes $1 páxinas están}} na categoría actual.',
'category-file-count'            => '{{PLURAL:$2|Esta categoría só contén o seguinte ficheiro.|{{PLURAL:$1|O seguinte ficheiro está|Os seguintes $1 ficheiros están}} nesta categoría, dun total de $2.}}',
'category-file-count-limited'    => '{{PLURAL:$1|O seguinte ficheiro está|Os seguintes $1 ficheiros están}} na categoría actual.',
'listingcontinuesabbrev'         => 'cont.',

'mainpagetext'      => "<big>'''O programa Wiki foi instalado con éxito.'''</big>",
'mainpagedocfooter' => 'Consulte a [http://meta.wikimedia.org/wiki/Help:Contents Guía do usuario] para máis información sobre como usar o software wiki.

== Comezando ==
* [http://www.mediawiki.org/wiki/Manual:Configuration_settings Listaxe de opcións de configuración]
* [http://www.mediawiki.org/wiki/Manual:FAQ Preguntas frecuentes sobre MediaWiki]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce Listaxe de correo das edicións de MediaWiki]',

'about'          => 'Acerca de',
'article'        => 'Artigo',
'newwindow'      => '(abre unha ventá nova)',
'cancel'         => 'Cancelar',
'qbfind'         => 'Procurar',
'qbbrowse'       => 'Navegar',
'qbedit'         => 'Editar',
'qbpageoptions'  => 'Esta páxina',
'qbpageinfo'     => 'Contexto',
'qbmyoptions'    => 'As miñas páxinas',
'qbspecialpages' => 'Páxinas especiais',
'moredotdotdot'  => 'Máis...',
'mypage'         => 'A miña páxina',
'mytalk'         => 'A miña conversa',
'anontalk'       => 'Conversa con este enderezo IP',
'navigation'     => 'Navegación',
'and'            => 'e',

# Metadata in edit box
'metadata_help' => 'Metadatos:',

'errorpagetitle'    => 'Erro',
'returnto'          => 'Voltar a "$1".',
'tagline'           => 'De {{SITENAME}}',
'help'              => 'Axuda',
'search'            => 'Procura',
'searchbutton'      => 'Procurar',
'go'                => 'Artigo',
'searcharticle'     => 'Artigo',
'history'           => 'Historial da páxina',
'history_short'     => 'Historial',
'updatedmarker'     => 'actualizado desde a miña última visita',
'info_short'        => 'Información',
'printableversion'  => 'Versión para imprimir',
'permalink'         => 'Ligazón permanente',
'print'             => 'Imprimir',
'edit'              => 'Editar',
'create'            => 'Crear',
'editthispage'      => 'Editar esta páxina',
'create-this-page'  => 'Crear esta páxina',
'delete'            => 'Borrar',
'deletethispage'    => 'Borrar esta páxina',
'undelete_short'    => 'Restaurar {{PLURAL:$1|unha edición|$1 edicións}}',
'protect'           => 'Protexer',
'protect_change'    => 'cambiar',
'protectthispage'   => 'Protexer esta páxina',
'unprotect'         => 'desprotexer',
'unprotectthispage' => 'Desprotexer esta páxina',
'newpage'           => 'Páxina nova',
'talkpage'          => 'Conversar sobre esta páxina',
'talkpagelinktext'  => 'Conversa',
'specialpage'       => 'Páxina especial',
'personaltools'     => 'Ferramentas persoais',
'postcomment'       => 'Engadir un comentario',
'articlepage'       => 'Ver artigo',
'talk'              => 'Conversa',
'views'             => 'Vistas',
'toolbox'           => 'Caixa de ferramentas',
'userpage'          => 'Ver páxina de usuario',
'projectpage'       => 'Ver páxina do proxecto',
'imagepage'         => 'Ver a páxina de multimedia',
'mediawikipage'     => 'Ver a páxina da mensaxe',
'templatepage'      => 'Ver a páxina do modelo',
'viewhelppage'      => 'Ver a páxina de axuda',
'categorypage'      => 'Ver páxina de categoría',
'viewtalkpage'      => 'Ver a conversa',
'otherlanguages'    => 'Outras linguas',
'redirectedfrom'    => '(Redirixido desde "$1")',
'redirectpagesub'   => 'Páxina de redirección',
'lastmodifiedat'    => 'A última modificación desta páxina foi o $1 ás $2.', # $1 date, $2 time
'viewcount'         => 'Esta páxina foi visitada {{PLURAL:$1|unha vez|$1 veces}}.',
'protectedpage'     => 'Páxina protexida',
'jumpto'            => 'Ir a:',
'jumptonavigation'  => 'navegación',
'jumptosearch'      => 'procurar',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'Acerca de {{SITENAME}}',
'aboutpage'            => 'Project:Acerca de',
'bugreports'           => 'Informes de erro',
'bugreportspage'       => 'Project:Informe de erros',
'copyright'            => 'Todo o texto está dispoñíbel baixo $1.',
'copyrightpagename'    => 'Dereitos de autor (copyright) de {{SITENAME}}',
'copyrightpage'        => '{{ns:project}}: Dereitos de autor (Copyrights)',
'currentevents'        => 'Actualidade',
'currentevents-url'    => 'Project:Actualidade',
'disclaimers'          => 'Advertencias',
'disclaimerpage'       => 'Project:Advertencia xeral',
'edithelp'             => 'Axuda de edición',
'edithelppage'         => 'Help:Como editar unha páxina',
'faq'                  => 'PMF',
'faqpage'              => 'Project:PMF',
'helppage'             => 'Help:Axuda',
'mainpage'             => 'Portada',
'mainpage-description' => 'Portada',
'policy-url'           => 'Project:Política e normas',
'portal'               => 'Portal da comunidade',
'portal-url'           => 'Project:Portal da comunidade',
'privacy'              => 'Política de privacidade',
'privacypage'          => 'Project:Política de privacidade',

'badaccess'        => 'Erro de permisos',
'badaccess-group0' => 'Non ten autorización para executar a acción que solicitou.',
'badaccess-group1' => 'A acción solicitada está limitada aos usuarios do grupo $1.',
'badaccess-group2' => 'A acción solicitada está limitada aos usuarios nalgún dos grupos $1.',
'badaccess-groups' => 'A acción solicitada está limitada aos usuarios nalgún dos grupos $1.',

'versionrequired'     => 'Necesítase a versión $1 de MediaWiki',
'versionrequiredtext' => 'Necesítase a versión $1 de MediaWiki para utilizar esta páxina. Vexa [[Special:Version|a páxina da versión]].',

'ok'                      => 'Aceptar',
'retrievedfrom'           => 'Traído desde "$1"',
'youhavenewmessages'      => 'Ten $1 ($2).',
'newmessageslink'         => 'mensaxes novas',
'newmessagesdifflink'     => 'diferenzas coa revisión anterior',
'youhavenewmessagesmulti' => 'Ten mensaxes novas en $1',
'editsection'             => 'editar',
'editold'                 => 'editar',
'viewsourceold'           => 'ver código fonte',
'editsectionhint'         => 'Editar a sección: "$1"',
'toc'                     => 'Índice',
'showtoc'                 => 'amosar',
'hidetoc'                 => 'agochar',
'thisisdeleted'           => 'Ver ou restaurar $1?',
'viewdeleted'             => 'Ver $1?',
'restorelink'             => '{{PLURAL:$1|unha edición borrada|$1 edicións borradas}}',
'feedlinks'               => 'Sindicalización:',
'feed-invalid'            => 'Tipo de fonte de noticias non válido.',
'feed-unavailable'        => 'As fontes de noticias non están dispoñibles',
'site-rss-feed'           => 'Fonte de noticias RSS de $1',
'site-atom-feed'          => 'Fonte de noticias Atom de $1',
'page-rss-feed'           => 'Fonte de noticias RSS para "$1"',
'page-atom-feed'          => 'Fonte de noticias Atom para "$1"',
'red-link-title'          => '$1 (aínda non escrito)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Artigo',
'nstab-user'      => 'Páxina de usuario',
'nstab-media'     => 'Páxina multimedia',
'nstab-special'   => 'Páxina especial',
'nstab-project'   => 'Páxina do proxecto',
'nstab-image'     => 'Imaxe',
'nstab-mediawiki' => 'Mensaxe',
'nstab-template'  => 'Modelo',
'nstab-help'      => 'Axuda',
'nstab-category'  => 'Categoría',

# Main script and global functions
'nosuchaction'      => 'Non existe esa acción',
'nosuchactiontext'  => 'A acción especificada polo URL non é recoñecida polo wiki',
'nosuchspecialpage' => 'Non existe esa páxina especial',
'nospecialpagetext' => "<big>'''Solicitou unha páxina especial que non está recoñecida polo wiki.'''</big>

Pode atopar unha lista coas páxinas especiais válidas en [[Special:SpecialPages|{{int:specialpages}}]].",

# General errors
'error'                => 'Erro',
'databaseerror'        => 'Erro na base de datos',
'dberrortext'          => 'Ocorreu un erro de sintaxe na consulta á base de datos. Isto pódese deber a un erro no programa.
A última consulta á base de datos foi:
<blockquote><tt>$1</tt></blockquote>
desde a función "<tt>$2</tt>".
MySQL retornou o erro "<tt>$3: $4</tt>".',
'dberrortextcl'        => 'Ocorreu un erro de sintaxe na consulta.
A última consulta á base de datos foi:
"$1"
desde a función "$2".
MySQL retornou o erro "$3: $4".',
'noconnect'            => 'O wiki está experimentando algunhas dificultades técnicas e non se pode contactar co servidor da base de datos.<br />
$1',
'nodb'                 => 'Non se pode seleccionar a base de datos $1',
'cachederror'          => 'Esta é unha copia gardada da páxina requirida e pode non estar ao día.',
'laggedslavemode'      => 'Aviso: a páxina pode non conter actualizacións recentes.',
'readonly'             => 'Base de datos fechada',
'enterlockreason'      => 'Dea unha razón para o fechamento, incluíndo unha estimación de até cando se manterá.',
'readonlytext'         => 'Nestes momentos a base de datos está pechada a novas entradas e outras modificacións, probabelmente debido a rutinas de mantemento da base de datos, tras as que voltará á normalidade.

O administrador que a pechou deu esta explicación: $1',
'missing-article'      => 'A base de datos non atopa o texto da páxina chamada "$1" $2, que debera ter atopado.

Normalmente, isto é causado por seguir unha ligazón cara a unha diferenza vella ou a unha páxina que foi borrada.

Se este non é o caso, pode ter atopado un erro no software.
Por favor, comuníquello a un [[Special:ListUsers/sysop|administrador]] tomando nota da dirección URL.',
'missingarticle-rev'   => '(revisión#: $1)',
'missingarticle-diff'  => '(Dif: $1, $2)',
'readonly_lag'         => 'A base de datos bloqueouse automaticamente mentres os servidores escravos da base de datos se actualizan desde o máster',
'internalerror'        => 'Erro interno',
'internalerror_info'   => 'Erro interno: $1',
'filecopyerror'        => 'Non se deu copiado o ficheiro "$1" a "$2".',
'filerenameerror'      => 'Non se pode cambiar o nome do ficheiro "$1" a "$2".',
'filedeleteerror'      => 'Non se deu borrado o ficheiro "$1".',
'directorycreateerror' => 'Non se puido crear o directorio "$1".',
'filenotfound'         => 'Non se deu atopado o ficheiro "$1".',
'fileexistserror'      => 'Resultou imposíbel escribir no ficheiro "$1": o ficheiro xa existe',
'unexpected'           => 'Valor inesperado: "$1"="$2".',
'formerror'            => 'Erro: non se pode enviar o formulario',
'badarticleerror'      => 'Non pode efectuarse esta acción nesta páxina.',
'cannotdelete'         => 'Non se pode borrar a páxina ou imaxe especificada.
Se cadra, xa foi borrada por alguén.',
'badtitle'             => 'Título incorrecto',
'badtitletext'         => 'O título da páxina pedida non era válido, estaba baleiro ou proviña dunha ligazón interlingua ou interwiki incorrecta. Pode conter un ou máis caracteres dos que non se poden empregar nos títulos.',
'perfdisabled'         => 'Sentímolo! Esta funcionalidade foi deshabilitada temporalmente porque fai moi lenta a base de datos até o punto no que non se pode usar o wiki.',
'perfcached'           => 'A información seguinte é da memoria caché e pode ser que non estea completamente actualizada.',
'perfcachedts'         => 'Esta información é da memoria caché. Última actualización: $1.',
'querypage-no-updates' => 'Neste momento están desactivadas as actualizacións nesta páxina. O seu contido non se modificará.',
'wrong_wfQuery_params' => 'Parámetros Incorrectos para wfQuery()<br />
Función: $1<br />
Procura: $2',
'viewsource'           => 'Ver o código fonte',
'viewsourcefor'        => 'de $1',
'actionthrottled'      => 'Acción ocasional',
'actionthrottledtext'  => "Como unha medida de loita contra o ''spam'', limítase a realización desta acción a un número determinado de veces nun curto espazo de tempo, e vostede superou este límite. Ténteo de novo nuns minutos.",
'protectedpagetext'    => 'Esta páxina foi protexida para evitar a edición.',
'viewsourcetext'       => 'Pode ver e copiar o código fonte desta páxina:',
'protectedinterface'   => 'Esta páxina fornece o texto da interface do software e está protexida para evitar o seu abuso.',
'editinginterface'     => "'''Aviso:''' está editando unha páxina usada para fornecer o texto da interface do software.
Os cambios nesta páxina afectarán á aparencia da interface para os outros usuarios.
Para traducións, considere usar [http://translatewiki.net/wiki/Main_Page?setlang=gl Betawiki], o proxecto de localización de Mediawiki.",
'sqlhidden'            => '(Procura SQL agochada)',
'cascadeprotected'     => 'Esta páxina foi protexida fronte á edición debido a que está incluída {{PLURAL:$1|na seguinte páxina protexida, que ten|nas seguintes páxinas protexidas, que teñen}} a "protección en serie" activada:
$2',
'namespaceprotected'   => "Non dispón de permisos para modificar páxinas no espazo de nomes '''$1'''.",
'customcssjsprotected' => 'Non dispón de permisos para modificar esta páxina, dado que contén a configuración persoal doutro usuario.',
'ns-specialprotected'  => 'Non se poden editar as páxinas no espazo de nomes {{ns:special}}.',
'titleprotected'       => "Este título foi protexido da creación por [[User:$1|$1]].
A razón dada foi ''$2''.",

# Virus scanner
'virus-badscanner'     => 'Configuración errónea: escáner de virus descoñecido: <i>$1</i>',
'virus-scanfailed'     => 'fallou o escaneado (código $1)',
'virus-unknownscanner' => 'antivirus descoñecido:',

# Login and logout pages
'logouttitle'                => 'Saída de usuario a anónimo',
'logouttext'                 => '<strong>Agora está fóra do sistema.</strong>

Pode continuar usando {{SITENAME}} de xeito anónimo, ou pode [[Special:UserLogin|acceder de novo]] co mesmo nome de usuario ou con outro.
Teña en conta que mentres non se limpa a memoria caché do seu navegador algunhas páxinas poden continuar a ser amosadas como se aínda estivesen dentro do sistema.',
'welcomecreation'            => '== Reciba a nosa benvida, $1! ==
A súa conta foi creada correctamente.
Non esqueza personalizar as súas [[Special:Preferences|preferencias de {{SITENAME}}]].',
'loginpagetitle'             => 'Acceso de usuario',
'yourname'                   => 'O seu nome de usuario:',
'yourpassword'               => 'O seu contrasinal:',
'yourpasswordagain'          => 'Insira o seu contrasinal outra vez:',
'remembermypassword'         => 'Lembrar o meu contrasinal neste ordenador',
'yourdomainname'             => 'O seu dominio',
'externaldberror'            => 'Ou ben se produciu un erro da base de datos na autenticación externa ou ben non se lle permite actualizar a súa conta externa.',
'loginproblem'               => '<b>Houbo algún problema co seu acceso.</b><br />Ténteo de novo!',
'login'                      => 'Acceder ao sistema',
'nav-login-createaccount'    => 'Rexistro',
'loginprompt'                => "Debe habilitar as ''cookies'' para acceder a {{SITENAME}}.",
'userlogin'                  => 'Rexistro',
'logout'                     => 'Saír do sistema',
'userlogout'                 => 'Saír ao anonimato',
'notloggedin'                => 'Fóra do sistema',
'nologin'                    => 'Non está rexistrado? $1.',
'nologinlink'                => 'Cree unha conta',
'createaccount'              => 'Crear unha conta nova',
'gotaccount'                 => 'Xa ten unha conta? $1.',
'gotaccountlink'             => 'Acceda ao sistema',
'createaccountmail'          => 'por correo electrónico',
'badretype'                  => 'Os contrasinais que inseriu non coinciden entre si.',
'userexists'                 => 'O nome de usuario que pretende usar xa está en uso.
Escolla un nome diferente.',
'youremail'                  => 'Correo electrónico:',
'username'                   => 'Nome de usuario:',
'uid'                        => 'ID do usuario:',
'prefs-memberingroups'       => 'Membro {{PLURAL:$1|do grupo|dos grupos}}:',
'yourrealname'               => 'Nome real:',
'yourlanguage'               => 'Lingua da interface:',
'yourvariant'                => 'Variante de idioma:',
'yournick'                   => 'Sinatura:',
'badsig'                     => 'Sinatura non válida; comprobe o código HTML utilizado.',
'badsiglength'               => 'A súa sinatura é demasiado longa.
Ha de ter menos {{PLURAL:$1|dun carácter|de $1 caracteres}}.',
'email'                      => 'Correo electrónico',
'prefs-help-realname'        => 'O seu nome real é opcional, pero se escolle dalo utilizarase para atribuírlle o seu traballo.',
'loginerror'                 => 'Erro ao acceder ao sistema',
'prefs-help-email'           => 'O enderezo de correo electrónico é opcional, pero permite que se lle envíe un contrasinal novo se se esquece del.
Tamén pode deixar que outras persoas se poñan en contacto con vostede desde a súa páxina de usuario ou de conversa sen necesidade de revelar a súa identidade.',
'prefs-help-email-required'  => 'O enderezo de correo electrónico é requirido.',
'nocookiesnew'               => "A conta de usuario foi creada, pero non está rexistrado. {{SITENAME}} usa ''cookies'' para o rexistro. Vostede ten deshabilitadas as ''cookies''. Por favor, habilíteas, e logo rexístrese co seu novo nome de usuario e contrasinal.",
'nocookieslogin'             => '{{SITENAME}} usa cookies para rexistrar os usuarios. Vostede ten as cookies deshabilitadas. Por favor, habilíteas e ténteo de novo.',
'noname'                     => 'Non especificou un nome de usuario válido.',
'loginsuccesstitle'          => 'Acceso exitoso',
'loginsuccess'               => "'''Accedeu ao sistema {{SITENAME}} como \"\$1\".'''",
'nosuchuser'                 => 'non hai ningún usuario chamado "$1".
Verifique o nome que inseriu ou [[Special:Userlogin/signup|cree unha nova conta]].',
'nosuchusershort'            => 'non hai ningún usuario chamado "<nowiki>$1</nowiki>".
Verifique o nome que inseriu.',
'nouserspecified'            => 'Debe especificar un nome de usuario.',
'wrongpassword'              => 'o contrasinal escrito é incorrecto.
Por favor, insira outro.',
'wrongpasswordempty'         => 'o campo do contrasinal estaba en branco.
Por favor, ténteo de novo.',
'passwordtooshort'           => 'O seu contrasinal é inválido ou demasiado curto.
Debe conter como mínimo {{PLURAL:$1|1 carácter|$1 caracteres}} e ten que ser diferente do seu nome de usuario.',
'mailmypassword'             => 'Enviádeme un contrasinal novo por correo',
'passwordremindertitle'      => 'Novo contrasinal temporal para {{SITENAME}}',
'passwordremindertext'       => 'Alguén (probablemente vostede, desde o enderezo IP $1) pediu un novo
contrasinal para entrar en {{SITENAME}} ($4). Un contrasinal temporal do usuario
"$2" foi creado e fixado como "$3". Se esa foi a súa
intención, necesitará entrar no sistema e escoller un novo contrasinal agora.

Se foi alguén diferente o que fixo esta solicitude ou se xa se lembra do seu contrasinal
e non o quere modificar, pode ignorar esta mensaxe e
continuar a utilizar o seu contrasinal vello.',
'noemail'                    => 'O usuario "$1" non posúe ningún enderezo de correo electrónico rexistrado.',
'passwordsent'               => 'Envióuselle un contrasinal novo ao enderezo de correo electrónico rexistrado de "$1".
Por favor, acceda ao sistema de novo tras recibilo.',
'blocked-mailpassword'       => 'O seu enderezo IP está bloqueado e ten restrinxida a edición de artigos. Tampouco se lle permite usar a función de recuperación do contrasinal para evitar abusos do sistema.',
'eauthentsent'               => 'Envióuselle un correo electrónico de configuración ao enderezo mencionado.
Antes de enviar outro a esta conta terá que seguir as instrucións que aparecen nese correo para confirmar que a conta é realmente súa.',
'throttled-mailpassword'     => 'Enviouse un aviso co contrasinal {{PLURAL:$1|na última hora|nas últimas $1 horas}}.
Para evitar o abuso do sistema só se envía unha mensaxe cada {{PLURAL:$1|hora|$1 horas}}.',
'mailerror'                  => 'Produciuse un erro ao enviar o correo electrónico: $1',
'acct_creation_throttle_hit' => 'Sentímolo, pero xa ten creadas $1 contas. Non pode crear máis.',
'emailauthenticated'         => 'O seu enderezo de correo electrónico foi autenticado ($1).',
'emailnotauthenticated'      => 'O seu enderezo de correo electrónico aínda <strong>non foi autenticado</strong>. Non se enviou ningunha mensaxe por algunha das seguintes razóns.',
'noemailprefs'               => 'Especifique un enderezo de correo electrónico se quere que funcione esta opción.',
'emailconfirmlink'           => 'Confirmar o enderezo de correo electrónico',
'invalidemailaddress'        => 'Non se pode aceptar o enderezo de correo electrónico porque parece ter un formato incorrecto.
Introduza un enderezo cun formato válido ou limpe ese campo.',
'accountcreated'             => 'Conta creada',
'accountcreatedtext'         => 'A conta de usuario para $1 foi creada.',
'createaccount-title'        => 'Creación da conta para {{SITENAME}}',
'createaccount-text'         => 'Alguén creou unha conta chamada "$2" para o seu enderezo de correo electrónico en {{SITENAME}} ($4), e con contrasinal "$3".
Debe acceder ao sistema e mudar o contrasinal agora.

Pode facer caso omiso desta mensaxe se se creou esta conta por erro.',
'loginlanguagelabel'         => 'Linguas: $1',

# Password reset dialog
'resetpass'               => 'Borrar o contrasinal da conta',
'resetpass_announce'      => 'Debe rexistrarse co código temporal que recibiu por correo electrónico. Para finalizar o rexistro debe indicar un novo contrasinal aquí:',
'resetpass_text'          => '<!-- Engadir texto aquí -->',
'resetpass_header'        => 'Contrasinal borrado',
'resetpass_submit'        => 'Poñer o contrasinal e entrar',
'resetpass_success'       => 'O cambio do contrasinal realizouse con éxito! Agora pode entrar...',
'resetpass_bad_temporary' => 'O contrasinal provisorio non é válido. Isto pode deberse a que xa mudou o contrasinal con éxito ou a que solicitou un novo contrasinal provisorio.',
'resetpass_forbidden'     => 'Os contrasinais non poden ser mudados',
'resetpass_missing'       => 'O formulario está baleiro.',

# Edit page toolbar
'bold_sample'     => 'Texto en negra',
'bold_tip'        => 'Texto en negra',
'italic_sample'   => 'Texto en cursiva',
'italic_tip'      => 'Texto en cursiva',
'link_sample'     => 'Título de ligazón',
'link_tip'        => 'Ligazón interna',
'extlink_sample'  => 'http://www.example.com título de ligazón',
'extlink_tip'     => 'Ligazón externa (lembre o prefixo http://)',
'headline_sample' => 'Texto de cabeceira',
'headline_tip'    => 'Cabeceira de nivel 2',
'math_sample'     => 'Insira unha fórmula aquí',
'math_tip'        => 'Fórmula matemática (LaTeX)',
'nowiki_sample'   => 'Insira aquí un texto sen formato',
'nowiki_tip'      => 'Ignorar o formato wiki',
'image_sample'    => 'Exemplo.jpg',
'image_tip'       => 'Ficheiro embebido',
'media_sample'    => 'Exemplo.mp3',
'media_tip'       => 'Ligazón a un ficheiro',
'sig_tip'         => 'A súa sinatura con selo temporal',
'hr_tip'          => 'Liña horizontal (úsea con moderación)',

# Edit pages
'summary'                          => 'Resumo',
'subject'                          => 'Asunto/cabeceira',
'minoredit'                        => 'Esta é unha edición pequena',
'watchthis'                        => 'Vixiar esta páxina',
'savearticle'                      => 'Gardar a páxina',
'preview'                          => 'Vista previa',
'showpreview'                      => 'Mostrar a vista previa',
'showlivepreview'                  => 'Vista previa',
'showdiff'                         => 'Mostrar os cambios',
'anoneditwarning'                  => "'''Aviso:''' non accedeu ao sistema.
O seu enderezo IP quedará rexistrado no historial das revisións desta páxina.",
'missingsummary'                   => "'''Aviso:''' esqueceu incluír o texto do campo resumo.
Se preme en \"Gardar a páxina\" a súa edición gardarase sen ningunha descrición da edición.",
'missingcommenttext'               => 'Por favor escriba un comentario a continuación.',
'missingcommentheader'             => "'''Aviso:''' non escribiu ningún texto no asunto/cabeceira deste comentario.
Se preme en \"Gardar a páxina\", a súa edición gardarase sen el.",
'summary-preview'                  => 'Vista previa do resumo',
'subject-preview'                  => 'Vista previa do asunto/cabeceira',
'blockedtitle'                     => 'O usuario está bloqueado',
'blockedtext'                      => '<big>\'\'\'O seu nome de usuario ou enderezo IP foi bloqueado.\'\'\'</big>

O bloqueo foi realizado por $1.
A razón que deu foi \'\'$2\'\'.

* Inicio do bloqueo: $8
* Caducidade do bloqueo: $6
* Pretendeuse bloquear: $7

Pode contactar con $1 ou con calquera outro [[{{MediaWiki:Grouppage-sysop}}|administrador]] para discutir este bloqueo.
Non pode empregar a característica "enviarlle un correo electrónico a este usuario" a non ser que dispoña dun enderezo electrónico válido rexistrado nas súas [[Special:Preferences|preferencias de usuario]] e que o seu uso non fose bloqueado.
O seu enderezo IP actual é $3 e o ID do bloqueo é #$5.
Por favor, inclúa eses datos nas consultas que faga.',
'autoblockedtext'                  => 'O seu enderezo IP foi bloqueado automaticamente porque foi empregado por outro usuario que foi bloqueado por $1.
A razón que deu foi a seguinte:

:\'\'$2\'\'

* Inicio do bloqueo: $8
* Caducidade do bloqueo: $6
* Pretendeuse bloquear: $7 

Pode contactar con $1 ou con calquera outro [[{{MediaWiki:Grouppage-sysop}}|administrador]] para discutir este bloqueo.

Teña en conta que non pode empregar "enviarlle un correo electrónico a este usuario" a non ser que dispoña dun enderezo electrónico válido rexistrado nas súas [[Special:Preferences|preferencias de usuario]] e e que o seu uso non fose bloqueado.

O seu enderezo IP actual é $3 e o ID do bloqueo é #$5.
Por favor, inclúa eses datos nas consultas que faga.',
'blockednoreason'                  => 'non foi dada ningunha razón',
'blockedoriginalsource'            => "O código fonte de '''$1''' móstrase a continuación:",
'blockededitsource'                => "O texto das '''súas edicións''' en '''$1''' móstrase a continuación:",
'whitelistedittitle'               => 'Cómpre acceder ao sistema para poder editar',
'whitelistedittext'                => 'Ten que $1 para poder editar páxinas.',
'confirmedittitle'                 => 'Requírese confirmar o enderezo electrónico para editar',
'confirmedittext'                  => 'Debe confirmar o correo electrónico antes de comezar a editar. Por favor, configure e dea validez ao correo mediante as súas [[Special:Preferences|preferencias de usuario]].',
'nosuchsectiontitle'               => 'Non existe tal sección',
'nosuchsectiontext'                => 'Tentou editar unha sección inexistente. Dado que non existe a sección $1, non hai onde gardar a súa edición.',
'loginreqtitle'                    => 'Cómpre acceder ao sistema',
'loginreqlink'                     => 'acceder ao sistema',
'loginreqpagetext'                 => 'Debe $1 para ver outras páxinas.',
'accmailtitle'                     => 'O contrasinal foi enviado.',
'accmailtext'                      => 'O contrasinal para "$1" foi enviado a $2.',
'newarticle'                       => '(Novo)',
'newarticletext'                   => "Seguiu unha ligazón a unha páxina que aínda non existe.
Para crear a páxina, comece a escribir na caixa de embaixo (vexa a [[{{MediaWiki:Helppage}}|páxina de axuda]] para máis información).
Se chegou aquí por erro, simplemente prema no botón '''atrás''' do seu navegador.",
'anontalkpagetext'                 => "----''Esta é a páxina de conversa dun usuario anónimo que aínda non creou unha conta ou que non a usa. Polo tanto, empregamos o enderezo IP para a súa identificación. Este enderezo IP pódenno compartir varios usuarios distintos. Se pensa que foron dirixidos contra a súa persoa comentarios inadecuados, por favor, [[Special:UserLogin/signup|cree unha conta]] ou [[Special:UserLogin|acceda ao sistema]] para evitar futuras confusións con outros usuarios anónimos.''",
'noarticletext'                    => 'Actualmente non existe texto nesta páxina. Pode [[Special:Search/{{PAGENAME}}|procurar polo título desta páxina]] noutras páxinas ou [{{fullurl:{{FULLPAGENAME}}|action=edit}} editala].',
'userpage-userdoesnotexist'        => 'A conta do usuario "$1" non está rexistrada. Comprobe se desexa crear/editar esta páxina.',
'clearyourcache'                   => "'''Nota: despois de gravar cómpre limpar a memoria caché do seu navegador para ver os cambios.''' '''Mozilla / Firefox / Safari:''' prema ''Maiúsculas'' á vez que en ''Recargar'', ou prema en ''Ctrl-F5'' ou ''Ctrl-R'' (''Command-R'' nos Macintosh); '''Konqueror:''' faga clic en ''Recargar'' ou prema en ''F5''; '''Opera:''' limpe a súa memoria caché en ''Ferramentas → Preferencias''; '''Internet Explorer:''' prema ''Ctrl'' ao tempo que fai clic en ''Refrescar'', ou prema ''Ctrl-F5''.",
'usercssjsyoucanpreview'           => '<strong>Nota:</strong> use o botón "Mostrar a vista previa" para verificar o novo CSS/JS antes de gardalo.',
'usercsspreview'                   => "'''Lembre que só está ven do a vista previa do seu CSS de usuario. Aínda non foi gardado!'''",
'userjspreview'                    => "'''Lembre que só está testando/previsualizando o seu javascript de usuario, non foi aínda gardado!'''",
'userinvalidcssjstitle'            => "'''Aviso:''' Non hai ningún tema \"\$1\". Lembre que as páxinas .css e .js utilizan un título en minúsculas, como por exemplo {{ns:user}}:Foo/monobook.css no canto de {{ns:user}}:Foo/Monobook.css.",
'updated'                          => '(Actualizado)',
'note'                             => '<strong>Nota:</strong>',
'previewnote'                      => '<strong>Lembre que esta é só unha vista previa e que os seus cambios aínda non foron gardados!</strong>',
'previewconflict'                  => 'Esta vista previa amosa o texto na área superior tal e como aparecerá se escolle gardar.',
'session_fail_preview'             => '<strong>O sistema non pode procesar a súa edición porque se perderon os datos de inicio da sesión.
Por favor, ténteo de novo.
Se segue sen funcionar, probe a [[Special:UserLogout|saír do sistema]] e volver entrar.</strong>',
'session_fail_preview_html'        => "<strong>O sistema non pode procesar a súa edición porque se perderon os datos de inicio da sesión.</strong>

''Dado que {{SITENAME}} ten activado o HTML simple, agóchase a vista previa como precaución contra ataques mediante JavaScript.''

<strong>Se este é un intento de facer unha edición lexítima, por favor, ténteo de novo.
Se segue sen funcionar, probe a [[Special:UserLogout|saír do sistema]] e volver entrar.</strong>",
'token_suffix_mismatch'            => "<strong>Rexeitouse a súa edición porque o seu cliente confundiu os signos de puntuación na edición.
Rexeitouse a edición para evitar que se corrompa o texto do artigo. Isto pode acontecer porque estea a
empregar un servizo de ''proxy'' anónimo defectuoso baseado na web.</strong>",
'editing'                          => 'Editando "$1"',
'editingsection'                   => 'Editando unha sección de "$1"',
'editingcomment'                   => 'Deixando un comentario en "$1"',
'editconflict'                     => 'Conflito de edición: "$1"',
'explainconflict'                  => "Alguén cambiou esta páxina desde que comezou a editala.
A área de texto superior contén o texto da páxina tal e como existe na actualidade.
Os seus cambios móstranse na área inferior.
Pode mesturar os seus cambios co texto existente.
'''Só''' se gardará o texto na área superior cando prema \"Gardar a páxina\".",
'yourtext'                         => 'O seu texto',
'storedversion'                    => 'Versión gardada',
'nonunicodebrowser'                => '<strong>ATENCIÓN: o seu navegador non soporta Unicode.
Existe unha solución que lle permite editar páxinas con seguridade: os caracteres non incluídos no ASCII aparecerán na caixa de edición como códigos hexadecimais.</strong>',
'editingold'                       => '<strong>ATENCIÓN: está editando unha revisión non actualizada desta páxina.
Se a garda, perderanse os cambios realizados tras esta revisión.</strong>',
'yourdiff'                         => 'Diferenzas',
'copyrightwarning'                 => 'Por favor, teña en conta que todas as contribucións a {{SITENAME}} considéranse publicadas baixo a $2 (vexa $1 para máis detalles). Se non quere que o que escriba se edite sen piedade e se redistribúa sen límites, entón non o envíe aquí.<br />
Ao mesmo tempo, prométanos que o que escribiu é da súa autoría ou que está copiado dun recurso do dominio público ou que permite unha liberdade semellante.
<strong>NON ENVÍE MATERIAL CON DEREITOS DE AUTOR SEN PERMISO!</strong>',
'copyrightwarning2'                => 'Por favor, decátese de que todas as súas contribucións a {{SITENAME}} poden ser editadas, alteradas ou eliminadas por outras persoas. Se non quere que os seus escritos sexan editados sen piedade, non os publique aquí.<br />
Do mesmo xeito, comprométese a que o que vostede escriba sexa da súa autoría ou copiado dunha fonte de dominio público ou recurso público semellante (vexa $1 para detalles).
<strong>NON ENVÍE SEN PERMISO TRABALLOS CON DEREITOS DE COPIA!</strong>',
'longpagewarning'                  => '<strong>ATENCIÓN: esta páxina ten $1 kilobytes;
algúns navegadores poden ter problemas editando páxinas de 32kb ou máis.
Por favor, considere partir a páxina en seccións máis pequenas.</strong>',
'longpageerror'                    => '<strong>ERRO: o texto que pretende gardar supera en $1 kilobytes o permitido.
Hai un límite máximo de $2 kilobytes;
polo tanto, non se pode gardar.</strong>',
'readonlywarning'                  => '<strong>ATENCIÓN: a base de datos foi fechada para facer mantemento, polo que non vai poder gardar as súas edicións polo de agora.
Se cadra, pode cortar e pegar o texto nun ficheiro de texto e gardalo para despois.</strong>',
'protectedpagewarning'             => '<strong>ATENCIÓN: esta páxina foi fechada de xeito que só os usuarios con privilexios de administrador do sistema poden editala.</strong>',
'semiprotectedpagewarning'         => "'''Nota:''' esta páxina foi bloqueada e só os usuarios rexistrados poden editala.",
'cascadeprotectedwarning'          => "'''Aviso:''' esta páxina foi protexida de xeito que só a poden editar os usuarios con privilexios de administrador debido a que está incluída {{PLURAL:\$1|na seguinte páxina protexida|nas seguintes páxinas protexidas}} coa opción \"protección en serie\" activada:",
'titleprotectedwarning'            => '<strong>AVISO: bloqueouse esta páxina para que só algúns usuarios a poidan crear.</strong>',
'templatesused'                    => 'Modelos usados nesta páxina:',
'templatesusedpreview'             => 'Modelos usados nesta vista previa:',
'templatesusedsection'             => 'Modelos usados nesta sección:',
'template-protected'               => '(protexido)',
'template-semiprotected'           => '(semiprotexido)',
'hiddencategories'                 => 'Esta páxina forma parte {{PLURAL:$1|dunha categoría oculta|de $1 categorías ocultas}}:',
'edittools'                        => '<!-- O texto que apareza aquí mostrarase por debaixo dos formularios de edición e envío. -->',
'nocreatetitle'                    => 'Limitada a creación de páxinas',
'nocreatetext'                     => '{{SITENAME}} ten restrinxida a posibilidade de crear páxinas novas.
Pode voltar e editar unha páxina que xa existe ou, se non, [[Special:UserLogin|rexistrarse ou crear unha conta]].',
'nocreate-loggedin'                => 'Non dispón dos permisos necesarios para crear páxinas novas.',
'permissionserrors'                => 'Erros de permisos',
'permissionserrorstext'            => 'Non dispón de permiso para facelo por {{PLURAL:$1|esta razón|estas razóns}}:',
'permissionserrorstext-withaction' => 'Non ten permiso para $2, {{PLURAL:$1|pola seguinte razón|polas seguintes razóns}}:',
'recreate-deleted-warn'            => "'''Atención: vai volver crear unha páxina que xa foi eliminada anteriormente.

Debería considerar se é apropiado continuar a editar esta páxina.
Velaquí está o rexistro de borrado desta páxina, por se quere consultalo:",

# Parser/template warnings
'expensive-parserfunction-warning'        => 'Aviso: esta páxina contén moitos analizadores de funcións de chamadas moi caros.

Debe ter menos de $2, e agora hai $1.',
'expensive-parserfunction-category'       => 'Páxinas con moitos analizadores de funcións de chamadas moi caros',
'post-expand-template-inclusion-warning'  => 'Aviso: o tamaño do modelo incluído é moi grande.
Algúns modelos non serán incluídos.',
'post-expand-template-inclusion-category' => 'Páxinas onde o tamaño dos modelos incluídos é excedido',
'post-expand-template-argument-warning'   => 'Aviso: esta páxina contén, polo menos, un argumento dun modelo que ten un tamaño e expansión moi grande.
Estes argumentos serán omitidos.',
'post-expand-template-argument-category'  => 'Páxinas que conteñen argumentos de modelo omitidos',

# "Undo" feature
'undo-success' => 'A edición pode ser desfeita.
Por favor, comprobe a comparación que aparece a continuación para confirmar que isto é o que desexa facer, despois, garde os cambios para desfacer a edición.',
'undo-failure' => 'A edición non pode ser desfeita debido a un conflito con algunha das edicións intermedias.',
'undo-norev'   => 'A edición non se pode desfacer porque non existe ou foi eliminada.',
'undo-summary' => 'Desfíxose a edición $1 de [[Special:Contributions/$2|$2]] ([[User talk:$2|conversa]])',

# Account creation failure
'cantcreateaccounttitle' => 'Non pode crear unha conta de usuario',
'cantcreateaccount-text' => "A creación de contas desde este enderezo IP ('''$1''') foi bloqueada por [[User:$3|$3]].

A razón dada por $3 foi ''$2''",

# History pages
'viewpagelogs'        => 'Ver os rexistros desta páxina',
'nohistory'           => 'Esta páxina non posúe ningún historial de edicións.',
'revnotfound'         => 'A revisión non foi atopada',
'revnotfoundtext'     => 'A revisión vella que pediu non se deu atopado.
Por favor verifique o URL que utilizou para acceder a esta páxina.',
'currentrev'          => 'Revisión actual',
'revisionasof'        => 'Revisión como estaba ás $1',
'revision-info'       => 'Revisión feita por $2 ás $1',
'previousrevision'    => '← Revisión máis antiga',
'nextrevision'        => 'Revisión máis nova →',
'currentrevisionlink' => 'Ver revisión actual',
'cur'                 => 'actual',
'next'                => 'seguinte',
'last'                => 'última',
'page_first'          => 'primeira',
'page_last'           => 'derradeira',
'histlegend'          => 'Selección de diferenzas: marque as versións que queira comparar e prema no botón ao final.<br />
Lenda: (actual) = diferenza coa versión actual,
(última) = diferenza coa versión precedente, m = edición pequena.',
'deletedrev'          => '[borrado]',
'histfirst'           => 'Primeiras',
'histlast'            => 'Últimas',
'historysize'         => '({{PLURAL:$1|1 byte|$1 bytes}})',
'historyempty'        => '(baleiro)',

# Revision feed
'history-feed-title'          => 'Historial de revisións',
'history-feed-description'    => 'Historial de revisións desta páxina no wiki',
'history-feed-item-nocomment' => '$1 en $2', # user at time
'history-feed-empty'          => 'A páxina solicitada non existe.
Puido borrarse ou moverse a outro nome.
Probe a [[Special:Search|buscar no wiki]] para atopar as páxinas relacionadas.',

# Revision deletion
'rev-deleted-comment'         => '(comentario eliminado)',
'rev-deleted-user'            => '(nome de usuario eliminado)',
'rev-deleted-event'           => '(rexistro de evento eliminado)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">
Esta revisión da páxina foi eliminada dos arquivos públicos.
Pode ampliar detalles no [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} rexistro de borrados].
</div>',
'rev-deleted-text-view'       => '<div class="mw-warning plainlinks">
Esta revisión da páxina foi eliminada dos arquivos públicos.
Como administrador deste wiki pode vela;
se quere ampliar detalles, visite o [{{fullurl:Special:Log/delete|page={{PAGENAMEE}}}} rexistro de borrados].
</div>',
'rev-delundel'                => 'mostrar/agochar',
'revisiondelete'              => 'Borrar/restaurar revisións',
'revdelete-nooldid-title'     => 'Revisión inválida',
'revdelete-nooldid-text'      => 'Non indicou a revisión ou revisións sobre as que realizar esta
función, a revisión especificada non existe, ou está intentando agochar a revisión actual.',
'revdelete-selected'          => '{{PLURAL:$2|Revisión seleccionada|Revisións seleccionadas}} de [[:$1]]:',
'logdelete-selected'          => '{{PLURAL:$1|Rexistro de evento seleccionado|Rexistro de eventos seleccionados}}:',
'revdelete-text'              => 'As revisión eliminadas aínda aparecerán no historial da páxina, pero o contido do seu texto será inaccesíbel ao público.

Outros administradores de {{SITENAME}} poderán acceder aínda ao contido oculto e poderán volver atrás esa eliminación a través desta mesma interface, a non ser que os operadores do sitio leven a cabo unha restrición adicional.',
'revdelete-legend'            => 'Aplicar restricións de visibilidade',
'revdelete-hide-text'         => 'Agochar texto da revisión',
'revdelete-hide-name'         => 'Agochar acción e destino',
'revdelete-hide-comment'      => 'Agochar comentario da edición',
'revdelete-hide-user'         => 'Agochar nome de usuario/IP do editor',
'revdelete-hide-restricted'   => 'Aplicar estas restricións aos administradores e bloquear esta interface',
'revdelete-suppress'          => 'Eliminar os datos tanto dos administradores como dos demais',
'revdelete-hide-image'        => 'Agochar o contido do ficheiro',
'revdelete-unsuppress'        => 'Retirar as restricións sobre as revisións restauradas',
'revdelete-log'               => 'Comentario do rexistro:',
'revdelete-submit'            => 'Aplicar á revisión seleccionada',
'revdelete-logentry'          => 'mudou a visibilidade dunha revisión de "[[$1]]"',
'logdelete-logentry'          => 'mudouse a visibilidade do evento para [[$1]]',
'revdelete-success'           => "'''Configurouse sen problemas a visibilidade da revisión.'''",
'logdelete-success'           => "'''Configurouse a visibilidade do evento sen problemas.'''",
'revdel-restore'              => 'Cambiar visibilidade',
'pagehist'                    => 'Historial da páxina',
'deletedhist'                 => 'Historial de borrado',
'revdelete-content'           => 'contido',
'revdelete-summary'           => 'resumo de edición',
'revdelete-uname'             => 'nome de usuario',
'revdelete-restricted'        => 'aplicadas as restricións aos administradores',
'revdelete-unrestricted'      => 'eliminadas as restricións aos administradores',
'revdelete-hid'               => 'agochar $1',
'revdelete-unhid'             => 'amosar $1',
'revdelete-log-message'       => '$1 para $2 {{PLURAL:$2|revisión|revisións}}',
'logdelete-log-message'       => '$1 para $2 {{PLURAL:$2|evento|eventos}}',

# Suppression log
'suppressionlog'     => 'Rexistro de supresión',
'suppressionlogtext' => 'Embaixo amósase unha listaxe coas eliminacións e cos bloqueos recentes, que inclúen contido oculto dos administradores.
Vexa a [[Special:IPBlockList|listaxe de enderezos IP bloqueados]] para comprobar as prohibicións e os bloqueos vixentes.',

# History merging
'mergehistory'                     => 'Fusionar historiais das páxinas',
'mergehistory-header'              => 'Esta páxina permítelle fusionar revisións dos historiais da páxina de orixe nunha nova páxina.
Asegúrese de que esta modificación da páxina mantén a continuidade histórica.',
'mergehistory-box'                 => 'Fusionar as revisións de dúas páxinas:',
'mergehistory-from'                => 'Páxina de orixe:',
'mergehistory-into'                => 'Páxina de destino:',
'mergehistory-list'                => 'Historial de edicións fusionábeis',
'mergehistory-merge'               => 'As revisións seguintes de [[:$1]] pódense fusionar con [[:$2]]. Use a columna de botóns de selección para fusionar só as revisións creadasen e antes da hora indicada. Teña en conta que se usa as ligazóns de navegación a columna limparase.',
'mergehistory-go'                  => 'Amosar edicións fusionábeis',
'mergehistory-submit'              => 'Fusionar revisións',
'mergehistory-empty'               => 'Non hai revisións que se poidan fusionar.',
'mergehistory-success'             => '{{PLURAL:$3|Unha revisión|$3 revisións}} de [[:$1]] {{PLURAL:$3|fusionouse|fusionáronse}} sen problemas en [[:$2]].',
'mergehistory-fail'                => 'Non se puido fusionar o historial; comprobe outra vez os parámetros de páxina e hora.',
'mergehistory-no-source'           => 'Non existe a páxina de orixe $1.',
'mergehistory-no-destination'      => 'Non existe a páxina de destino $1.',
'mergehistory-invalid-source'      => 'A páxina de orixe ten que ter un título válido.',
'mergehistory-invalid-destination' => 'A páxina de destino ten que ter un título válido.',
'mergehistory-autocomment'         => '[[:$1]] fusionouse en [[:$2]]',
'mergehistory-comment'             => '[[:$1]] fusionouse en [[:$2]]: $3',

# Merge log
'mergelog'           => 'Rexistro de fusións',
'pagemerge-logentry' => 'fusionouse [[$1]] con [[$2]] (revisións até $3)',
'revertmerge'        => 'Desfacer a fusión',
'mergelogpagetext'   => 'Embaixo hai unha lista coas fusións máis recentes do historial dunha páxina co doutra.',

# Diffs
'history-title'           => 'Historial das revisións de "$1"',
'difference'              => '(Diferenzas entre revisións)',
'lineno'                  => 'Liña $1:',
'compareselectedversions' => 'Comparar as versións seleccionadas',
'editundo'                => 'desfacer',
'diff-multi'              => '(Non se {{PLURAL:$1|mostra unha revisión|mostran $1 revisións}} do historial.)',

# Search results
'searchresults'             => 'Resultados da procura',
'searchresulttext'          => 'Para máis información sobre como realizar procuras en {{SITENAME}}, vexa [[{{MediaWiki:Helppage}}|{{int:help}}]].',
'searchsubtitle'            => 'A súa busca de "\'\'\'[[:$1]]\'\'\'" ([[Special:Prefixindex/$1|todas as páxinas que comezan por "$1"]] | [[Special:WhatLinksHere/$1|todas as páxinas que ligan con "$1"]])',
'searchsubtitleinvalid'     => "A súa busca de '''$1'''",
'noexactmatch'              => "'''Non hai ningunha páxina titulada \"\$1\".'''
Se quere, pode [[:\$1|creala]].",
'noexactmatch-nocreate'     => "'''Non hai ningunha páxina titulada \"\$1\".'''",
'toomanymatches'            => 'Demasiadas coincidencias foron devoltas, por favor tente unha consulta diferente',
'titlematches'              => 'O título do artigo coincide',
'notitlematches'            => 'Non coincide ningún título de páxina',
'textmatches'               => 'O texto da páxina coincide',
'notextmatches'             => 'Non se atopou o texto en ningunha páxina',
'prevn'                     => '$1 anteriores',
'nextn'                     => '$1 seguintes',
'viewprevnext'              => 'Ver as ($1) ($2) ($3)',
'search-result-size'        => '$1 ({{PLURAL:$2|1 palabra|$2 palabras}})',
'search-result-score'       => 'Relevancia: $1%',
'search-redirect'           => '(redirixir $1)',
'search-section'            => '(sección $1)',
'search-suggest'            => 'Quizais quixo dicir: $1',
'search-interwiki-caption'  => 'Proxectos irmáns',
'search-interwiki-default'  => '$1 resultados:',
'search-interwiki-more'     => '(máis)',
'search-mwsuggest-enabled'  => 'con suxestións',
'search-mwsuggest-disabled' => 'sen suxestións',
'search-relatedarticle'     => 'Relacionado',
'mwsuggest-disable'         => 'Deshabilitar as suxestións AJAX',
'searchrelated'             => 'relacionado',
'searchall'                 => 'todo',
'showingresults'            => "Amósanse {{PLURAL:$1|'''1''' resultado|'''$1''' resultados}} comezando polo número '''$2'''.",
'showingresultsnum'         => "Embaixo {{PLURAL:$3|amósase '''1''' resultado|amósanse '''$3''' resultados}}, comezando polo número '''$2'''.",
'showingresultstotal'       => "Embaixo {{PLURAL:$3|amósase o resultado '''$1''', dun total de '''$3'''|amósanse os resultados do '''$1''' ao '''$2''', dun total de '''$3'''}}",
'nonefound'                 => "'''Nota:''' só algúns espazos de nomes son procurados por omisión. Probe a fixar a súa petición con ''(Principal)'' para procurar en todo o contido (incluíndo páxinas de conversa, modelos, etc.) ou use como prefixo o espazo de nomes desexado.",
'powersearch'               => 'Procurar',
'powersearch-legend'        => 'Busca avanzada',
'powersearch-ns'            => 'Procurar nos espazos de nomes:',
'powersearch-redir'         => 'Listar as redireccións',
'powersearch-field'         => 'Procurar por',
'search-external'           => 'Procura externa',
'searchdisabled'            => '<p style="margin: 1.5em 2em 1em">As procuras en {{SITENAME}} están deshabilitadas por cuestións de rendemento. Mentres tanto pode procurar usando o Google.
<span style="font-size: 89%; display: block; margin-left: .2em">Note que os seus índices do contido de {{SITENAME}} poden estar desactualizados.</span></p>',

# Preferences page
'preferences'              => 'Preferencias',
'mypreferences'            => 'As miñas preferencias',
'prefs-edits'              => 'Número de edicións:',
'prefsnologin'             => 'Non está dentro do sistema',
'prefsnologintext'         => 'Debe <span class="plainlinks">[{{fullurl:Special:UserLogin|returnto=$1}} acceder ao sistema]</span> para modificar as preferencias de usuario.',
'prefsreset'               => 'As preferencias foron postas cos valores orixinais.',
'qbsettings'               => 'Opcións da barra rápida',
'qbsettings-none'          => 'Ningunha',
'qbsettings-fixedleft'     => 'Fixa á esquerda',
'qbsettings-fixedright'    => 'Fixa á dereita',
'qbsettings-floatingleft'  => 'Flotante á esquerda',
'qbsettings-floatingright' => 'Flotante á dereita',
'changepassword'           => 'Cambiar o meu contrasinal',
'skin'                     => 'Aparencia',
'math'                     => 'Fórmulas matemáticas',
'dateformat'               => 'Formato da data',
'datedefault'              => 'Ningunha preferencia',
'datetime'                 => 'Data e hora',
'math_failure'             => 'Fallou a conversión do código',
'math_unknown_error'       => 'erro descoñecido',
'math_unknown_function'    => 'función descoñecida',
'math_lexing_error'        => 'erro de léxico',
'math_syntax_error'        => 'erro de sintaxe',
'math_image_error'         => 'Fallou a conversión a PNG; comprobe que latex, dvips, gs e convert están ben instalados',
'math_bad_tmpdir'          => 'Non se puido crear ou escribir no directorio temporal de fórmulas',
'math_bad_output'          => 'Non se puido crear ou escribir no directorio de saída de fórmulas',
'math_notexvc'             => 'Falta o executable texvc. Por favor consulte math/README para configurar.',
'prefs-personal'           => 'Información do usuario',
'prefs-rc'                 => 'Cambios recentes',
'prefs-watchlist'          => 'Listaxe de vixilancia',
'prefs-watchlist-days'     => 'Días para amosar na listaxe de vixilancia:',
'prefs-watchlist-edits'    => 'Número de edicións para mostrar na listaxe de vixilancia completa:',
'prefs-misc'               => 'Preferencias varias',
'saveprefs'                => 'Gardar as preferencias',
'resetprefs'               => 'Eliminar os cambios non gardados',
'oldpassword'              => 'Contrasinal antigo:',
'newpassword'              => 'Contrasinal novo:',
'retypenew'                => 'Insira outra vez o novo contrasinal:',
'textboxsize'              => 'Edición',
'rows'                     => 'Filas:',
'columns'                  => 'Columnas:',
'searchresultshead'        => 'Procurar',
'resultsperpage'           => 'Cantidade de peticións a amosar por páxina:',
'contextlines'             => 'Cantidade de liñas a amosar por resultado:',
'contextchars'             => 'Caracteres de contexto por liña:',
'stub-threshold'           => 'Umbral para o formatado de <a href="#" class="stub">ligazón de bosquexo</a> (bytes):',
'recentchangesdays'        => 'Número de días para mostrar nos cambios recentes:',
'recentchangescount'       => 'Número de edicións para mostrar nos cambios recentes, nos historiais e nas páxinas de rexistros:',
'savedprefs'               => 'As súas preferencias foron gardadas.',
'timezonelegend'           => 'Zona horaria',
'timezonetext'             => '¹Insira o número de horas de diferenza entre a súa hora local e a do servidor (UTC).',
'localtime'                => 'Visualización da hora local',
'timezoneoffset'           => 'Desprazamento¹',
'servertime'               => 'A hora do servidor agora é',
'guesstimezone'            => 'Encher desde o navegador',
'allowemail'               => 'Admitir mensaxes de correo electrónico doutros usuarios',
'prefs-searchoptions'      => 'Opcións na procura',
'prefs-namespaces'         => 'Espazos de nomes',
'defaultns'                => 'Procurar por omisión nestes espazos de nomes:',
'default'                  => 'predeterminado',
'files'                    => 'Ficheiros',

# User rights
'userrights'                  => 'Xestión dos dereitos de usuario', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => 'Administrar os grupos do usuario',
'userrights-user-editname'    => 'Escriba o nome do usuario:',
'editusergroup'               => 'Editar os grupos do usuario',
'editinguser'                 => "Mudando os dereitos do usuario '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup'    => 'Editar os grupos do usuario',
'saveusergroups'              => 'Gardar os grupos do usuario',
'userrights-groupsmember'     => 'Membro de:',
'userrights-groups-help'      => 'Pode cambiar os grupos aos que o usuario pertence:
* Se a caixa ten un sinal (✓) significa que o usuario pertence a ese grupo.
* Se, pola contra, non o ten, significa que non pertence.
* Un asterisco (*) indica que non pode eliminar o grupo unha vez que o engadiu, e viceversa.',
'userrights-reason'           => 'Razón para a modificación:',
'userrights-no-interwiki'     => 'Non dispón de permiso para editar dereitos de usuarios noutros wikis.',
'userrights-nodatabase'       => 'A base de datos $1 non existe ou non é local.',
'userrights-nologin'          => 'Debe [[Special:UserLogin|acceder ao sistema]] cunta conta de administrador para asignar dereitos de usuario.',
'userrights-notallowed'       => 'A súa conta non dispón de permiso para asignar dereitos de usuario.',
'userrights-changeable-col'   => 'Os grupos que pode cambiar',
'userrights-unchangeable-col' => 'Os grupos que non pode cambiar',

# Groups
'group'               => 'Grupo:',
'group-user'          => 'Usuarios',
'group-autoconfirmed' => 'Usuarios autoconfirmados',
'group-bot'           => 'Bots',
'group-sysop'         => 'Administradores',
'group-bureaucrat'    => 'Burócratas',
'group-suppress'      => 'Supervisores',
'group-all'           => '(todos)',

'group-user-member'          => 'Usuario',
'group-autoconfirmed-member' => 'Usuario autoconfirmado',
'group-bot-member'           => 'Bot',
'group-sysop-member'         => 'Administrador',
'group-bureaucrat-member'    => 'Burócrata',
'group-suppress-member'      => 'Supervisor',

'grouppage-user'          => '{{ns:project}}:Usuarios',
'grouppage-autoconfirmed' => '{{ns:project}}:Usuarios autoconfirmados',
'grouppage-bot'           => '{{ns:project}}:Bots',
'grouppage-sysop'         => '{{ns:project}}:Administradores',
'grouppage-bureaucrat'    => '{{ns:project}}:Burócratas',
'grouppage-suppress'      => '{{ns:project}}:Supervisor',

# Rights
'right-read'                 => 'Ler páxinas',
'right-edit'                 => 'Editar páxinas',
'right-createpage'           => 'Crear páxinas (que non son de conversa)',
'right-createtalk'           => 'Crear páxinas de conversa',
'right-createaccount'        => 'Crear novas contas de usuario',
'right-minoredit'            => 'Marcar as edicións como pequenas',
'right-move'                 => 'Mover páxinas',
'right-move-subpages'        => 'Mover páxinas coas súas subpáxinas',
'right-suppressredirect'     => 'Non crear unha redirección dende o nome vello ao mover unha páxina',
'right-upload'               => 'Cargar ficheiros',
'right-reupload'             => 'Sobreescribir un ficheiro existente',
'right-reupload-own'         => 'Sobreescribir un ficheiro existente cargado polo mesmo usuario',
'right-reupload-shared'      => 'Sobreescribir localmente ficheiros do repositorio multimedia',
'right-upload_by_url'        => 'Cargar un ficheiro dende un enderezo URL',
'right-purge'                => 'Purgar a caché dunha páxina do wiki sen a páxina de confirmación',
'right-autoconfirmed'        => 'Editar páxinas semiprotexidas',
'right-bot'                  => 'Ser tratado coma un proceso automatizado',
'right-nominornewtalk'       => 'As edicións pequenas nas páxinas de conversa non lanzan o aviso de mensaxes novas',
'right-apihighlimits'        => 'Usar os límites superiores nas peticións API',
'right-writeapi'             => 'Usar o API para modificar o wiki',
'right-delete'               => 'Borrar páxinas',
'right-bigdelete'            => 'Borrar páxinas con historiais grandes',
'right-deleterevision'       => 'Borrar e restaurar versións específicas de páxinas',
'right-deletedhistory'       => 'Ver as entradas borradas do historial, sen o seu texto asociado',
'right-browsearchive'        => 'Procurar páxinas borradas',
'right-undelete'             => 'Restaurar unha páxina',
'right-suppressrevision'     => 'Revisar e restaurar as revisións agochadas dos administradores',
'right-suppressionlog'       => 'Ver rexistros privados',
'right-block'                => 'Bloquear outros usuarios fronte á edición',
'right-blockemail'           => 'Bloquear un usuario fronte ao envío dun correo electrónico',
'right-hideuser'             => 'Bloquear un usuario, agochándollo ao público',
'right-ipblock-exempt'       => 'Evitar bloqueos de IPs, autobloqueos e bloqueos de rango',
'right-proxyunbannable'      => 'Evitar os bloqueos autamáticos a proxies',
'right-protect'              => 'Trocar os niveis de protección e editar páxinas protexidas',
'right-editprotected'        => 'Editar páxinas protexidas (que non teñan protección en serie)',
'right-editinterface'        => 'Editar a interface de usuario',
'right-editusercssjs'        => 'Editar os ficheiros CSS e JS doutros usuarios',
'right-rollback'             => 'Reversión rápida da edición dun usuario dunha páxina particular',
'right-markbotedits'         => 'Marcar as edicións desfeitas como edicións dun bot',
'right-noratelimit'          => 'Non lle afectan os límites superiores',
'right-import'               => 'Importar páxinas doutros wikis',
'right-importupload'         => 'Importar páxinas desde un ficheiro cargado',
'right-patrol'               => 'Marcar edicións como patrulladas',
'right-autopatrol'           => 'Ter as edicións marcadas automaticamente como patrulladas',
'right-patrolmarks'          => 'Ver os cambios que están marcados coma patrullados',
'right-unwatchedpages'       => 'Ver unha listaxe de páxinas que non están vixiadas',
'right-trackback'            => 'Enviar un "trackback"',
'right-mergehistory'         => 'Fusionar o historial das páxinas',
'right-userrights'           => 'Editar todos os dereitos de usuario',
'right-userrights-interwiki' => 'Editar os dereitos de usuario dos usuarios doutros wikis',
'right-siteadmin'            => 'Fechar e abrir a base de datos',

# User rights log
'rightslog'      => 'Rexistro de dereitos de usuario',
'rightslogtext'  => 'Este é un rexistro de permisos dos usuarios.',
'rightslogentry' => 'cambiou o grupo ao que pertence "$1" de $2 a $3',
'rightsnone'     => '(ningún)',

# Recent changes
'nchanges'                          => '$1 {{PLURAL:$1|cambio|cambios}}',
'recentchanges'                     => 'Cambios recentes',
'recentchangestext'                 => 'Sigue, nesta páxina, as modificacións máis recentes no wiki.',
'recentchanges-feed-description'    => 'Siga os cambios máis recentes deste wiki con esta fonte de noticias.',
'rcnote'                            => "Embaixo {{PLURAL:$1|amósase '''1''' cambio|amósanse os últimos '''$1''' cambios}} {{PLURAL:$2|no último día|nos últimos '''$2''' días}} ata as $5 do $4.",
'rcnotefrom'                        => "Abaixo amósanse os cambios desde '''$2''' (móstranse ata '''$1''').",
'rclistfrom'                        => 'Mostrar os cambios novos desde $1',
'rcshowhideminor'                   => '$1 as edicións pequenas',
'rcshowhidebots'                    => '$1 os bots',
'rcshowhideliu'                     => '$1 os usuarios rexistrados',
'rcshowhideanons'                   => '$1 os usuarios anónimos',
'rcshowhidepatr'                    => '$1 edicións revisadas',
'rcshowhidemine'                    => '$1 as edicións propias',
'rclinks'                           => 'Mostrar os últimos $1 cambios nos últimos $2 días.<br />$3',
'diff'                              => 'dif',
'hist'                              => 'hist',
'hide'                              => 'Agochar',
'show'                              => 'Amosar',
'minoreditletter'                   => 'm',
'newpageletter'                     => 'N',
'boteditletter'                     => 'b',
'number_of_watching_users_pageview' => '[$1 {{PLURAL:$1|usuario|usuarios}} vixiando]',
'rc_categories'                     => 'Límite para categorías (separado con "|")',
'rc_categories_any'                 => 'Calquera',
'newsectionsummary'                 => 'Nova sección: /* $1 */',

# Recent changes linked
'recentchangeslinked'          => 'Cambios relacionados',
'recentchangeslinked-title'    => 'Cambios relacionados con "$1"',
'recentchangeslinked-noresult' => 'Non se produciron cambios nas páxinas vinculadas a esta durante o período de tempo seleccionado.',
'recentchangeslinked-summary'  => "Esta é unha listaxe dos cambios que se realizaron recentemente nas páxinas vinculadas a esta (ou dos membros da categoría especificada).
As páxinas da súa [[Special:Watchlist|listaxe de vixilancia]] aparecen en '''negra'''.",
'recentchangeslinked-page'     => 'Nome da páxina:',
'recentchangeslinked-to'       => 'Amosar os cambios relacionados das páxinas que ligan coa dada',

# Upload
'upload'                      => 'Cargar un ficheiro',
'uploadbtn'                   => 'Cargar o ficheiro',
'reupload'                    => 'Volver cargar',
'reuploaddesc'                => 'Cancelar a carga e voltar ao formulario de carga',
'uploadnologin'               => 'Non está dentro do sistema',
'uploadnologintext'           => 'Debe [[Special:UserLogin|acceder ao sistema]] para poder cargar ficheiros.',
'upload_directory_missing'    => 'Falta o directorio de carga ($1) e non pode ser creado polo servidor da páxina web.',
'upload_directory_read_only'  => 'Non se pode escribir no directorio de subida ($1) do servidor web.',
'uploaderror'                 => 'Erro ao cargar',
'uploadtext'                  => "Use o formulario de embaixo para cargar ficheiros.
Para ver ou procurar imaxes subidas con anterioridade vaia á [[Special:ImageList|listaxe de imaxes]]; os envíos tamén se rexistran no [[Special:Log/upload|rexistro de carga]], e as eliminacións no [[Special:Log/delete|rexistro de borrado]].

Para incluír un ficheiro nunha páxina, use unha ligazón do seguinte xeito:
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:File.jpg]]</nowiki></tt>''' para usar a versión completa do ficheiro
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:File.png|200px|thumb|left|texto alternativo]]</nowiki></tt>''' para usar unha resolución de 200 píxeles de ancho nunha caixa na marxe esquerda cunha descrición (\"texto alternativo\")
* '''<tt><nowiki>[[</nowiki>{{ns:media}}<nowiki>:File.ogg]]</nowiki></tt>''' para ligar directamente co ficheiro sen que este saia na páxina",
'upload-permitted'            => 'Tipos de ficheiro permitidos: $1.',
'upload-preferred'            => 'Tipos de arquivos preferidos: $1.',
'upload-prohibited'           => 'Tipos de arquivos prohibidos: $1.',
'uploadlog'                   => 'rexistro de cargas',
'uploadlogpage'               => 'Rexistro de cargas',
'uploadlogpagetext'           => 'Embaixo hai unha lista cos ficheiros subidos máis recentemente.
Vexa a [[Special:NewImages|galería de imaxes novas]] para unha visión máis xeral.',
'filename'                    => 'Nome do ficheiro',
'filedesc'                    => 'Resumo',
'fileuploadsummary'           => 'Descrición:',
'filestatus'                  => 'Status do Copyright:',
'filesource'                  => 'Fonte:',
'uploadedfiles'               => 'Ficheiros cargados en {{SITENAME}}',
'ignorewarning'               => 'Ignorar a advertencia e gardar o ficheiro de calquera xeito',
'ignorewarnings'              => 'Ignorar os avisos',
'minlength1'                  => 'Os nomes dos ficheiros deben ter cando menos unha letra.',
'illegalfilename'             => 'O nome de ficheiro "$1" contén caracteres que non están permitidos nos títulos das páxinas. Por favor cambie o nome do ficheiro e tente cargalo outra vez.',
'badfilename'                 => 'O nome desta imaxe cambiouse a "$1".',
'filetype-badmime'            => 'Non se permite enviar ficheiros de tipo MIME "$1".',
'filetype-unwanted-type'      => "'''\".\$1\"''' é un tipo de ficheiro non desexado.
{{PLURAL:\$3|O tipo de ficheiro preferido é|Os tipos de ficheiro preferidos son}} \$2.",
'filetype-banned-type'        => "'''\".\$1\"''' non é un tipo de ficheiro permitido.
{{PLURAL:\$3|O tipo de ficheiro permitido é|Os tipos de ficheiros permitidos son}} \$2.",
'filetype-missing'            => 'O ficheiro non conta cunha extensión (como ".jpg").',
'large-file'                  => 'Recoméndase que o tamaño dos ficheiros non supere $1; este ficheiro ocupa $2.',
'largefileserver'             => 'Este ficheiro é de maior tamaño có permitido pola configuración do servidor.',
'emptyfile'                   => 'O ficheiro que cargou semella estar baleiro. Isto pode deberse a un erro ortográfico no seu nome.
Por favor verifique se realmente quere cargar este ficheiro.',
'fileexists'                  => 'Xa existe un ficheiro con ese nome. Por favor, verifique <strong><tt>$1</tt></strong> se non está seguro de que quere cambialo.',
'filepageexists'              => 'A páxina de descrición deste ficheiro xa foi creada en <strong><tt>$1</tt></strong>, pero polo de agora non existe ningún ficheiro con este nome. O resumo que escribiu non aparecerá na páxina de descrición. Para facer que o resumo apareza alí, necesitará editar a páxina manualmente',
'fileexists-extension'        => 'Xa existe un ficheiro cun nome semellante:<br />
Nome do ficheiro que tenta cargar: <strong><tt>$1</tt></strong><br />
Nome de ficheiro existente: <strong><tt>$2</tt></strong><br />
Por favor, escolla un nome diferente.',
'fileexists-thumb'            => "<center>'''Imaxe existente'''</center>",
'fileexists-thumbnail-yes'    => 'Parece que o ficheiro é unha imaxe de tamaño reducido <i>(miniatura)</i>. Comprobe o ficheiro <strong><tt>$1</tt></strong>.<br />
Se o ficheiro seleccionado é a mesma imaxe de tamaño orixinal non é preciso enviar unha miniatura adicional.',
'file-thumbnail-no'           => 'O nome do ficheiro comeza por <strong><tt>$1</tt></strong>.
Parece tratarse dunha imaxe de tamaño reducido <i>(miniatura)</i>.
Se dispón dunha versión desta imaxe de maior resolución, se non, múdelle o nome ao ficheiro.',
'fileexists-forbidden'        => 'Xa hai un ficheiro co mesmo nome; por favor retroceda e cargue o ficheiro cun novo nome. [[Image:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => 'Xa existe un ficheiro con este nome no depósito de ficheiros compartidos.
Se aínda quere cargar o seu ficheiro, por favor, volte atrás e use outro nome.
[[Image:$1|thumb|center|$1]]',
'file-exists-duplicate'       => 'Este ficheiro é un duplicado {{PLURAL:$1|do seguinte|dos seguintes}}:',
'successfulupload'            => 'Carga con éxito',
'uploadwarning'               => 'Advertencia ao cargar o ficheiro',
'savefile'                    => 'Gardar o ficheiro',
'uploadedimage'               => 'cargou "[[$1]]"',
'overwroteimage'              => 'enviou unha nova versión de "[[$1]]"',
'uploaddisabled'              => 'Sentímolo, a subida de ficheiros está desactivada.',
'uploaddisabledtext'          => 'A carga de ficheiros está deshabilitada.',
'uploadscripted'              => 'Este ficheiro contén HTML ou código (script code) que pode producir erros ao ser interpretado polo navegador.',
'uploadcorrupt'               => 'O ficheiro está corrompido ou ten unha extensión incorrecta. Por favor verifique o ficheiro e súbao de novo.',
'uploadvirus'                 => 'O ficheiro contén un virus! Detalles: $1',
'sourcefilename'              => 'Nome do ficheiro a cargar:',
'destfilename'                => 'Nome do ficheiro de destino:',
'upload-maxfilesize'          => 'Tamaño máximo para o ficheiro: $1',
'watchthisupload'             => 'Vixiar esta páxina',
'filewasdeleted'              => 'Un ficheiro con ese nome foi cargado con anterioridade e a continuación borrado.
Debe comprobar o $1 antes de proceder a cargalo outra vez.',
'upload-wasdeleted'           => "'''Aviso: está enviando un ficheiro que foi previamente borrado.'''

Debe considerar se é apropiado continuar enviando este ficheiro.
O rexistro de borrado proporciónase aquí por se quere consultalo:",
'filename-bad-prefix'         => 'O nome do ficheiro que está cargando comeza con <strong>"$1"</strong>, que é un típico nome non descritivo asignado automaticamente polas cámaras dixitais. Por favor, escolla un nome máis descritivo para o seu ficheiro.',
'filename-prefix-blacklist'   => ' #<!-- deixe esta liña exactamente como está --> <pre>
# A sintaxe é a seguinte:
#   * Todo o que estea desde o carácter "#" até o final da liña é un comentario
#   * Cada liña que non está en branco é un prefixo para os nomes típicos dos ficheiros asignados automaticamente polas cámaras dixitais
CIMG # Casio
DSC_ # Nikon
DSCF # Fuji
DSCN # Nikon
DUW # algúns teléfonos móbiles
IMG # xenérico
JD # Jenoptik
MGP # Pentax
PICT # varias
 #</pre> <!-- deixe esta liña exactamente como está -->',

'upload-proto-error'      => 'Protocolo erróneo',
'upload-proto-error-text' => 'A carga remota require URLs que comecen por <code>http://</code> ou <code>ftp://</code>.',
'upload-file-error'       => 'Erro interno',
'upload-file-error-text'  => 'Produciuse un erro interno ao tentar crear un ficheiro temporal no servidor.
Por favor, contacte cun [[Special:ListUsers/sysop|administrador]] do sistema.',
'upload-misc-error'       => 'Erro de carga descoñecido',
'upload-misc-error-text'  => 'Durante a carga ocorreu un erro descoñecido.
Por favor, comprobe que o enderezo URL é válido e está dispoñíbel e, despois, ténteo de novo.
Se o problema persiste contacte cun [[Special:ListUsers/sysop|administrador]] do sistema.',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => 'Non se logrou acceder a ese URL',
'upload-curl-error6-text'  => 'Non se logrou acceder ao URL que indicou. Comprobe que ese URL é correcto e que o sitio está activo.',
'upload-curl-error28'      => 'Rematou o tempo de espera',
'upload-curl-error28-text' => 'O sitio tardou demasiado en responder.
Por favor, comprobe que está activo, agarde un anaco e ténteo de novo.
Tamén pode reintentalo cando haxa menos actividade.',

'license'            => 'Licenza:',
'nolicense'          => 'Ningunha (os ficheiros sen licenza teñen que ser eliminados)',
'license-nopreview'  => '(Vista previa non dispoñíbel)',
'upload_source_url'  => ' (un URL válido, accesíbel publicamente)',
'upload_source_file' => ' (un ficheiro no seu ordenador)',

# Special:ImageList
'imagelist-summary'     => 'Esta páxina especial amosa todos os ficheiros cargados.
Por omisión, os ficheiros enviados máis recentemente aparecen no alto da listaxe.
Premendo nunha cabeceira da columna cambia a ordenación.',
'imagelist_search_for'  => 'Buscar polo nome do ficheiro multimedia:',
'imgfile'               => 'ficheiro',
'imagelist'             => 'Listaxe de imaxes',
'imagelist_date'        => 'Data',
'imagelist_name'        => 'Nome',
'imagelist_user'        => 'Usuario',
'imagelist_size'        => 'Tamaño (bytes)',
'imagelist_description' => 'Descrición',

# Image description page
'filehist'                       => 'Historial do ficheiro',
'filehist-help'                  => 'Faga clic nunha data/hora para ver o ficheiro tal e como estaba nese momento.',
'filehist-deleteall'             => 'borrar todo',
'filehist-deleteone'             => 'borrar',
'filehist-revert'                => 'reverter',
'filehist-current'               => 'actual',
'filehist-datetime'              => 'Data/Hora',
'filehist-user'                  => 'Usuario',
'filehist-dimensions'            => 'Dimensións',
'filehist-filesize'              => 'Tamaño do ficheiro',
'filehist-comment'               => 'Comentario',
'imagelinks'                     => 'Ligazóns da imaxe',
'linkstoimage'                   => '{{PLURAL:$1|A seguinte páxina liga|As seguintes $1 páxinas ligan}} con esta imaxe:',
'nolinkstoimage'                 => 'Ningunha páxina liga con este ficheiro.',
'morelinkstoimage'               => 'Ver [[Special:WhatLinksHere/$1|máis ligazóns]] cara a este ficheiro.',
'redirectstofile'                => '{{PLURAL:$1|O seguinte ficheiro redirixe|Os seguintes $1 ficheiros redirixen}} cara a este:',
'duplicatesoffile'               => '{{PLURAL:$1|O seguinte ficheiro é un duplicado|Os seguintes $1 ficheiros son duplicados}} destoutro:',
'sharedupload'                   => 'Este ficheiro é un envío compartido e pode ser usado por outros proxectos.',
'shareduploadwiki'               => 'Por favor, vexa a $1 para máis información.',
'shareduploadwiki-desc'          => 'Embaixo móstrase a descrición da $1 no repositorio de imaxes.',
'shareduploadwiki-linktext'      => 'páxina de descrición do ficheiro',
'shareduploadduplicate'          => 'Este ficheiro é un duplicado $1 que está no repositorio.',
'shareduploadduplicate-linktext' => 'doutro ficheiro',
'shareduploadconflict'           => 'Este ficheiro comparte o nome $1 que está no repositorio.',
'shareduploadconflict-linktext'  => 'doutro ficheiro',
'noimage'                        => 'Non existe ningún ficheiro con ese nome, pero pode $1.',
'noimage-linktext'               => 'cargar un',
'uploadnewversion-linktext'      => 'Cargar unha nova versión deste ficheiro',
'imagepage-searchdupe'           => 'Procurar ficheiros duplicados',

# File reversion
'filerevert'                => 'Desfacer $1',
'filerevert-legend'         => 'Reverter ficheiro',
'filerevert-intro'          => 'Está revertendo "\'\'\'[[Media:$1|$1]]\'\'\'", vai volver á versión [$4 de $2, ás $3].',
'filerevert-comment'        => 'Comentario:',
'filerevert-defaultcomment' => 'Volveuse á versión do $1 ás $2',
'filerevert-submit'         => 'Reverter',
'filerevert-success'        => 'Reverteuse "\'\'\'[[Media:$1|$1]]\'\'\'" á versión [$4 de $2, ás $3].',
'filerevert-badversion'     => 'Non existe unha versión local anterior deste ficheiro coa data e hora indicadas.',

# File deletion
'filedelete'                  => 'Eliminar "$1"',
'filedelete-legend'           => 'Eliminar un ficheiro',
'filedelete-intro'            => "Vai eliminar \"'''[[Media:\$1|\$1]]'''\".",
'filedelete-intro-old'        => 'Vai eliminar a versión de "\'\'\'[[Media:$1|$1]]\'\'\'" do [$4 $2, ás $3].',
'filedelete-comment'          => 'Comentario:',
'filedelete-submit'           => 'Eliminar',
'filedelete-success'          => "Eliminouse '''$1'''.",
'filedelete-success-old'      => 'Eliminouse a versión de "\'\'\'[[Media:$1|$1]]\'\'\'" do $2 ás $3.',
'filedelete-nofile'           => "\"'''\$1'''\" non existe.",
'filedelete-nofile-old'       => "Non existe unha versión arquivada de \"'''\$1'''\" cos atributos especificados.",
'filedelete-iscurrent'        => 'Tentou eliminar a versión máis recente deste ficheiro. Volva antes a unha versión máis antiga.',
'filedelete-otherreason'      => 'Outro motivo:',
'filedelete-reason-otherlist' => 'Outra razón',
'filedelete-reason-dropdown'  => '*Motivos frecuentes para borrar
** Violación do copyright
** Ficheiro duplicado',
'filedelete-edit-reasonlist'  => 'Editar os motivos de borrado',

# MIME search
'mimesearch'         => 'Busca MIME',
'mimesearch-summary' => 'Esta páxina permite filtrar os ficheiros segundo o seu tipo MIME.
Entrada: tipodecontido/subtipo, p.ex. <tt>image/jpeg</tt>.',
'mimetype'           => 'Tipo MIME:',
'download'           => 'descargar',

# Unwatched pages
'unwatchedpages' => 'Páxinas non vixiadas',

# List redirects
'listredirects' => 'Listaxe de redireccións',

# Unused templates
'unusedtemplates'     => 'Modelos sen uso',
'unusedtemplatestext' => 'Esta páxina contén unha listaxe de todas as páxinas no espazo de nomes modelo que non están incluídas en ningunha outra páxina. Lembre verificar outros enlaces cara aos modelos antes de borralos.',
'unusedtemplateswlh'  => 'outras ligazóns',

# Random page
'randompage'         => 'Páxina aleatoria',
'randompage-nopages' => 'Non hai páxinas neste espazo de nomes.',

# Random redirect
'randomredirect'         => 'Redirección aleatoria',
'randomredirect-nopages' => 'Non hai redireccións neste espazo de nomes.',

# Statistics
'statistics'             => 'Estatísticas',
'sitestats'              => 'Estatísticas de {{SITENAME}}',
'userstats'              => 'Estatísticas dos usuarios',
'sitestatstext'          => "Actualmente hai {{PLURAL:\$1|'''1''' páxina|'''\$1''' páxinas en total}} na base de datos.
Isto inclúe as páxinas de \"conversa\", as páxinas acerca de {{SITENAME}}, as páxinas de \"contido mínimo\", as redireccións e outras que probabelmente non deberían considerarse como páxinas con contido.
Excluíndo todo isto, hai {{PLURAL:\$2|'''1''' páxina que é|'''\$2''' páxinas que son}}, probabelmente, {{PLURAL:\$2|páxina|páxinas}} con contido lexítimo.

{{PLURAL:\$8|Foi cargado|Foron cargados}} '''\$8''' {{PLURAL:\$8|ficheiro|ficheiros}}.

Houbo un total de '''\$3''' {{PLURAL:\$3|páxina vista|páxinas vistas}} e '''\$4''' {{PLURAL:\$4|edición|edicións}} desde que se creou {{SITENAME}}.
Isto resulta nunha media de '''\$5''' edicións por páxina e '''\$6''' visionados por edición.

A lonxitude da [http://www.mediawiki.org/wiki/Manual:Job_queue cola de traballos] é de '''\$7'''.",
'userstatstext'          => "Hai {{PLURAL:$1|'''1''' [[Special:ListUsers|usuario]] rexistrado|'''$1''' [[Special:ListUsers|usuarios]] rexistrados}}, dos cales '''$2''' (ou o '''$4%''') {{PLURAL:$2|ten|teñen}} dereitos de $5.",
'statistics-mostpopular' => 'Páxinas máis vistas',

'disambiguations'      => 'Páxinas de homónimos',
'disambiguationspage'  => 'Template:Homónimos',
'disambiguations-text' => "As seguintes páxinas ligan cunha '''páxina de homónimos'''.
No canto de ligar cos homónimos deben apuntar cara á páxina apropiada.<br />
Unha páxina trátase como páxina de homónimos cando nela se usa un modelo que está ligado desde a [[MediaWiki:Disambiguationspage|páxina de homónimos]].",

'doubleredirects'            => 'Redireccións dobres',
'doubleredirectstext'        => 'Esta páxina contén as páxinas que redirixen cara a outras páxinas de redirección. Cada ringleira contén ligazóns cara á primeira e segunda redireccións, e tamén á primeira liña da segunda redirección, que é usualmente o artigo "real", á que a primeira redirección debería apuntar.',
'double-redirect-fixed-move' => 'A páxina "[[$1]]" foi movida, agora é unha redirección cara a "[[$2]]"',
'double-redirect-fixer'      => 'Amañador de redireccións',

'brokenredirects'        => 'Redireccións rotas',
'brokenredirectstext'    => 'Estas redireccións ligan cara a unha páxina que non existe:',
'brokenredirects-edit'   => '(editar)',
'brokenredirects-delete' => '(borrar)',

'withoutinterwiki'         => 'Páxinas sen ligazóns interwiki',
'withoutinterwiki-summary' => 'As seguintes páxinas non ligan con ningunha versión noutra lingua.',
'withoutinterwiki-legend'  => 'Prefixo',
'withoutinterwiki-submit'  => 'Amosar',

'fewestrevisions' => 'Artigos con menos revisións',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|byte|bytes}}',
'ncategories'             => '$1 {{PLURAL:$1|categoría|categorías}}',
'nlinks'                  => '$1 {{PLURAL:$1|ligazón|ligazóns}}',
'nmembers'                => '$1 {{PLURAL:$1|páxina|páxinas}}',
'nrevisions'              => '$1 {{PLURAL:$1|revisión|revisións}}',
'nviews'                  => 'vista {{PLURAL:$1|unha vez|$1 veces}}',
'specialpage-empty'       => 'Non hai resultados para o que solicitou.',
'lonelypages'             => 'Páxinas orfas',
'lonelypagestext'         => 'As seguintes páxinas están illadas, non están enlazadas desde outras páxinas de {{SITENAME}}.',
'uncategorizedpages'      => 'Páxinas sen categorías',
'uncategorizedcategories' => 'Categorías sen categorías',
'uncategorizedimages'     => 'Ficheiros sen categorizar',
'uncategorizedtemplates'  => 'Modelos sen categorizar',
'unusedcategories'        => 'Categorías sen uso',
'unusedimages'            => 'Imaxes sen uso',
'popularpages'            => 'Páxinas populares',
'wantedcategories'        => 'Categorías requiridas',
'wantedpages'             => 'Páxinas requiridas',
'missingfiles'            => 'Ficheiros que faltan',
'mostlinked'              => 'Páxinas máis enlazadas',
'mostlinkedcategories'    => 'Categorías máis enlazadas',
'mostlinkedtemplates'     => 'Modelos máis enlazados',
'mostcategories'          => 'Artigos con máis categorías',
'mostimages'              => 'Ficheiros máis enlazados',
'mostrevisions'           => 'Artigos con máis revisións',
'prefixindex'             => 'Mostrar páxinas clasificadas polas letras iniciais',
'shortpages'              => 'Páxinas curtas',
'longpages'               => 'Páxinas longas',
'deadendpages'            => 'Páxinas sen ligazóns cara a outras',
'deadendpagestext'        => 'Estas páxinas non ligan con ningunha outra páxina de {{SITENAME}}.',
'protectedpages'          => 'Páxinas protexidas',
'protectedpages-indef'    => 'Só as proteccións indefinidas',
'protectedpagestext'      => 'As seguintes páxinas están protexidas fronte á edición ou traslado',
'protectedpagesempty'     => 'Non hai páxinas protexidas neste momento',
'protectedtitles'         => 'Títulos protexidos',
'protectedtitlestext'     => 'Os seguintes títulos están protexidos da creación',
'protectedtitlesempty'    => 'Actualmente non están protexidos títulos con eses parámetros.',
'listusers'               => 'Listaxe de usuarios',
'newpages'                => 'Páxinas novas',
'newpages-username'       => 'Nome de usuario:',
'ancientpages'            => 'Artigos máis antigos',
'move'                    => 'Mover',
'movethispage'            => 'Mover esta páxina',
'unusedimagestext'        => 'Por favor, teña en conta que outros sitios web poden ligar a un ficheiro mediante un enderezo URL e por iso poden aparecer listados aquí, mesmo estando en uso.',
'unusedcategoriestext'    => 'Existen as seguintes categorías, aínda que ningún artigo ou categoría as emprega.',
'notargettitle'           => 'Sen obxectivo',
'notargettext'            => 'Non especificou a páxina ou o usuario no cal levar a cabo esta función.',
'nopagetitle'             => 'Non existe esa páxina',
'nopagetext'              => 'A páxina que especificou non existe.',
'pager-newer-n'           => '{{PLURAL:$1|1 máis recente|$1 máis recentes}}',
'pager-older-n'           => '{{PLURAL:$1|1 máis vella|$1 máis vellas}}',
'suppress'                => 'Supervisor',

# Book sources
'booksources'               => 'Fontes bibliográficas',
'booksources-search-legend' => 'Procurar fontes bibliográficas',
'booksources-go'            => 'Ir',
'booksources-text'          => 'A continuación aparece unha listaxe de ligazóns cara a outros sitios web que venden libros novos e usados, neles tamén pode obter máis información sobre as obras que está a buscar:',

# Special:Log
'specialloguserlabel'  => 'Usuario:',
'speciallogtitlelabel' => 'Título:',
'log'                  => 'Rexistros',
'all-logs-page'        => 'Todos os rexistros',
'log-search-legend'    => 'Procurar rexistros',
'log-search-submit'    => 'Executar',
'alllogstext'          => 'Vista combinada de todos os rexistros dipoñibles en {{SITENAME}}.
Pode precisar máis a vista seleccionando o tipo de rexistro, o nome do usuario ou o título da páxina afectada.',
'logempty'             => 'Non se atopou ningún ítem relacionado no rexistro.',
'log-title-wildcard'   => 'Procurar os títulos que comecen con este texto',

# Special:AllPages
'allpages'          => 'Todas as páxinas',
'alphaindexline'    => '$1 a $2',
'nextpage'          => 'Páxina seguinte ($1)',
'prevpage'          => 'Páxina anterior ($1)',
'allpagesfrom'      => 'Mostrar as páxinas que comecen por:',
'allarticles'       => 'Todos os artigos',
'allinnamespace'    => 'Todas as páxinas (espazo de nomes $1)',
'allnotinnamespace' => 'Todas as páxinas (que non están no espazo de nomes $1)',
'allpagesprev'      => 'Anterior',
'allpagesnext'      => 'Seguinte',
'allpagessubmit'    => 'Amosar',
'allpagesprefix'    => 'Mostrar páxinas no espazo de nomes:',
'allpagesbadtitle'  => 'O título dado á páxina non era válido ou contiña un prefixo inter-linguas ou inter-wikis. Pode que conteña un ou máis caracteres que non se poden empregar nos títulos.',
'allpages-bad-ns'   => '{{SITENAME}} carece do espazo de nomes "$1".',

# Special:Categories
'categories'                    => 'Categorías',
'categoriespagetext'            => 'As seguintes categorías conteñen páxinas ou contidos multimedia.
Aquí non se amosan as [[Special:UnusedCategories|categorías sen uso]].
Véxanse tamén as [[Special:WantedCategories|categorías requiridas]].',
'categoriesfrom'                => 'Amosar as categorías comezando por:',
'special-categories-sort-count' => 'ordenar por número',
'special-categories-sort-abc'   => 'ordenar alfabeticamente',

# Special:ListUsers
'listusersfrom'      => 'Mostrar os usuarios comezando por:',
'listusers-submit'   => 'Amosar',
'listusers-noresult' => 'Non se atopou ningún usuario. Comprobe tamén as variantes con maiúsculas e minúsculas.',

# Special:ListGroupRights
'listgrouprights'          => 'Dereitos dun usuario segundo o seu grupo',
'listgrouprights-summary'  => 'A seguinte lista mostra os grupos de usuario definidos neste wiki, cos seus dereitos de acceso asociados.
Se quere máis información acerca dos dereitos individuais, pode atopala [[{{MediaWiki:Listgrouprights-helppage}}|aquí]].',
'listgrouprights-group'    => 'Grupo',
'listgrouprights-rights'   => 'Dereitos',
'listgrouprights-helppage' => 'Help:Dereitos do grupo',
'listgrouprights-members'  => '(lista de membros)',

# E-mail user
'mailnologin'     => 'Non existe enderezo para o envío',
'mailnologintext' => 'Debe [[Special:UserLogin|acceder ao sistema]] e ter rexistrado un enderezo de correo electrónico válido nas súas [[Special:Preferences|preferencias]] para enviar correos electrónicos a outros usuarios.',
'emailuser'       => 'Enviar un correo electrónico a este usuario',
'emailpage'       => 'Enviar un correo electrónico a un usuario',
'emailpagetext'   => 'Se o usuario introduciu un enderezo de correo electrónico válido nas súas preferencias, este formulario serve para enviarlle unha única mensaxe.
O correo electrónico que inseriu [[Special:Preferences|nas súas preferencias]] aparecerá no campo "De:" do correo, polo que o receptor da mensaxe poderalle responder.',
'usermailererror' => 'O obxecto enviado deu unha mensaxe de erro:',
'defemailsubject' => 'Correo electrónico de {{SITENAME}}',
'noemailtitle'    => 'Sen enderezo de correo electrónico',
'noemailtext'     => 'Este usuario non rexistrou un enderezo de correo electrónico válido ou elixiu non recibir correos electrónicos doutros usuarios.',
'emailfrom'       => 'De:',
'emailto'         => 'Para:',
'emailsubject'    => 'Asunto:',
'emailmessage'    => 'Mensaxe:',
'emailsend'       => 'Enviar',
'emailccme'       => 'Enviar unha copia da mensaxe para min.',
'emailccsubject'  => 'Copia da mensaxe para $1: $2',
'emailsent'       => 'Mensaxe enviada',
'emailsenttext'   => 'A súa mensaxe de correo electrónico foi enviada.',
'emailuserfooter' => 'Este correo electrónico foi enviado por $1 a $2 mediante a función "Enviar un correo electrónico a este usuario" de {{SITENAME}}.',

# Watchlist
'watchlist'            => 'A miña listaxe de vixilancia',
'mywatchlist'          => 'A miña listaxe de vixilancia',
'watchlistfor'         => "(de '''$1''')",
'nowatchlist'          => 'Non ten ítems na súa listaxe de vixilancia.',
'watchlistanontext'    => 'Faga o favor de $1 no sistema para ver ou editar os ítems da súa listaxe de vixilancia.',
'watchnologin'         => 'Non accedeu ao sistema',
'watchnologintext'     => 'Debe [[Special:UserLogin|acceder ao sistema]] para modificar a súa listaxe de vixilancia.',
'addedwatch'           => 'Engadido á listaxe de vixilancia',
'addedwatchtext'       => "A páxina \"[[:\$1]]\" foi engadida á súa [[Special:Watchlist|listaxe de vixilancia]].
Os cambios futuros nesta páxina e na súa páxina de conversa asociada serán listados alí, e a páxina aparecerá en '''negra''' na [[Special:RecentChanges|listaxe de cambios recentes]] para facer máis sinxela a súa sinalización.",
'removedwatch'         => 'Quitado da listaxe de vixilancia',
'removedwatchtext'     => 'A páxina "[[:$1]]" foi eliminada [[Special:Watchlist|da súa listaxe de vixilancia]].',
'watch'                => 'Vixiar',
'watchthispage'        => 'Vixiar esta páxina',
'unwatch'              => 'Deixar de vixiar',
'unwatchthispage'      => 'Deixar de vixiar',
'notanarticle'         => 'Non é unha páxina de contido',
'notvisiblerev'        => 'A revisión foi borrada',
'watchnochange'        => 'Ningún dos elementos baixo vixilancia foi editado no período de tempo amosado.',
'watchlist-details'    => 'Hai {{PLURAL:$1|unha páxina|$1 páxinas}} na súa lista de vixilancia, sen contar as de conversa.',
'wlheader-enotif'      => '* Está dispoñíbel a notificación por correo electrónico.',
'wlheader-showupdated' => "* As páxinas que cambiaron desde a súa última visita amósanse en '''negra'''",
'watchmethod-recent'   => 'buscando edicións recentes das páxinas vixiadas',
'watchmethod-list'     => 'buscando nas páxinas vixiadas por edicións recentes',
'watchlistcontains'    => 'A súa listaxe de vixilancia ten $1 {{PLURAL:$1|páxina|páxinas}}.',
'iteminvalidname'      => "Hai un problema co ítem '$1', nome non válido...",
'wlnote'               => "Abaixo {{PLURAL:$1|está a última modificación|están as últimas '''$1''' modificacións}} {{PLURAL:$2|na última hora|nas últimas '''$2''' horas}}.",
'wlshowlast'           => 'Amosar as últimas $1 horas $2 días $3',
'watchlist-show-bots'  => 'Mostrar os bots',
'watchlist-hide-bots'  => 'Agochar os bots',
'watchlist-show-own'   => 'Mostrar as edicións propias',
'watchlist-hide-own'   => 'Agochar as edicións propias',
'watchlist-show-minor' => 'Mostrar as edicións pequenas',
'watchlist-hide-minor' => 'Agochar as edicións pequenas',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Vixiando...',
'unwatching' => 'Deixando de vixiar...',

'enotif_mailer'                => 'Correo de aviso de {{SITENAME}}',
'enotif_reset'                 => 'Marcar todas as páxinas como visitadas',
'enotif_newpagetext'           => 'Esta é unha páxina nova.',
'enotif_impersonal_salutation' => 'usuario de {{SITENAME}}',
'changed'                      => 'modificado',
'created'                      => 'creado',
'enotif_subject'               => 'A páxina da {{SITENAME}} co título $PAGETITLE foi $CHANGEDORCREATED por $PAGEEDITOR',
'enotif_lastvisited'           => 'Vexa $1 para comprobar todos os cambios desde a súa última visita.',
'enotif_lastdiff'              => 'Vexa $1 para visualizar esta modificación.',
'enotif_anon_editor'           => 'usuario anónimo $1',
'enotif_body'                  => 'Estimado $WATCHINGUSERNAME,

a páxina da {{SITENAME}} "$PAGETITLE" cambiou $CHANGEDORCREATED o $PAGEEDITDATE por unha edición de $PAGEEDITOR, vexa $PAGETITLE_URL para comprobar a versión actual.

$NEWPAGE

Resumo de edición: $PAGESUMMARY $PAGEMINOREDIT

Contactar co editor:
correo electrónico: $PAGEEDITOR_EMAIL
wiki: $PAGEEDITOR_WIKI

Non se producirán novas notificacións cando haxa novos cambios ata que vostede visite a páxina. Pode borrar os indicadores de aviso de notificación para o conxunto das páxinas marcadas na súa listaxe de vixilancia.

             O sistema de aviso de {{SITENAME}}

--
Para cambiar a súa lista de vixilancia, visite
{{fullurl:{{ns:special}}:Watchlist/edit}}

Axuda:
{{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => 'Borrar a páxina',
'confirm'                     => 'Confirmar',
'excontent'                   => 'o contido era: "$1"',
'excontentauthor'             => 'o contido era: "$1" (e o único editor foi "[[Special:Contributions/$2|$2]]")',
'exbeforeblank'               => 'o contido antes do baleiramento era: "$1"',
'exblank'                     => 'a páxina estaba baleira',
'delete-confirm'              => 'Borrar "$1"',
'delete-legend'               => 'Borrar',
'historywarning'              => 'Atención: a páxina que vai borrar ten un historial:',
'confirmdeletetext'           => 'Está a piques de borrar de xeito permanente unha páxina ou imaxe con todo o seu historial na base de datos.
Por favor, confirme que é realmente a súa intención, que comprende as consecuencias e que está obrando de acordo coas regras [[{{MediaWiki:Policy-url}}|da política e normas]].',
'actioncomplete'              => 'A acción foi completada',
'deletedtext'                 => '"<nowiki>$1</nowiki>" foi borrado.
No $2 pode ver unha listaxe dos borrados máis recentes.',
'deletedarticle'              => 'borrou "[[$1]]"',
'suppressedarticle'           => 'suprimiu "[[$1]]"',
'dellogpage'                  => 'Rexistro de borrados',
'dellogpagetext'              => 'Abaixo está a listaxe dos borrados máis recentes.',
'deletionlog'                 => 'rexistro de borrados',
'reverted'                    => 'Devolto a unha versión anterior',
'deletecomment'               => 'Razón para o borrado:',
'deleteotherreason'           => 'Outro motivo:',
'deletereasonotherlist'       => 'Outro motivo',
'deletereason-dropdown'       => '
*Motivos frecuentes para borrar
** Petición do autor
** Violación de copyright
** Vandalismo',
'delete-edit-reasonlist'      => 'Editar os motivos de borrado',
'delete-toobig'               => 'Esta páxina conta cun historial longo, de máis {{PLURAL:$1|dunha revisión|de $1 revisións}}.
Limitouse a eliminación destas páxinas para previr problemas de funcionamento accidentais en {{SITENAME}}.',
'delete-warning-toobig'       => 'Esta páxina conta cun historial de edicións longo, de máis {{PLURAL:$1|dunha revisión|de $1 revisións}}.
Ao eliminala pódense provocar problemas de funcionamento nas operacións da base de datos de {{SITENAME}};
proceda con coidado.',
'rollback'                    => 'Reverter as edicións',
'rollback_short'              => 'Reverter',
'rollbacklink'                => 'reverter',
'rollbackfailed'              => 'Houbo un fallo ao reverter as edicións',
'cantrollback'                => 'Non se pode desfacer a edición; o último contribuínte é o único autor desta páxina.',
'alreadyrolled'               => 'Non se pode desfacer a edición en "[[:$1]]" feita por [[User:$2|$2]] ([[User talk:$2|conversa]] | [[Special:Contributions/$2|{{int:contribslink}}]]); alguén máis editou ou desfixo os cambios desta páxina.

A última edición fíxoa [[User:$3|$3]] ([[User talk:$3|conversa]] | [[Special:Contributions/$2|{{int:contribslink}}]]).',
'editcomment'                 => 'O comentario da edición era: "<i>$1</i>".', # only shown if there is an edit comment
'revertpage'                  => 'Desfixéronse as edicións de [[Special:Contributions/$2|$2]] ([[User talk:$2|conversa]]); cambiado á última versión feita por [[User:$1|$1]]', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => 'Desfixéronse as edicións de $1;
volveuse á última edición, feita por $2.',
'sessionfailure'              => 'Parece que hai un problema co rexistro da súa sesión; esta acción cancelouse como precaución fronte ao secuestro de sesións. Prema no botón "atrás", volva cargar a páxina da que proviña e ténteo de novo.',
'protectlogpage'              => 'Rexistro de proteccións',
'protectlogtext'              => 'Embaixo móstrase unha lista dos bloqueos e desbloqueos de páxinas.
Vexa a [[Special:ProtectedPages|lista de páxinas protexidas]] se quere obter a lista coas proteccións de páxinas vixentes.',
'protectedarticle'            => 'protexeu "[[$1]]"',
'modifiedarticleprotection'   => 'modificou o nivel de protección de "[[$1]]"',
'unprotectedarticle'          => 'desprotexeu "[[$1]]"',
'protect-title'               => 'Cambiar o nivel de protección de "$1"',
'protect-legend'              => 'Confirmar protección',
'protectcomment'              => 'Motivo:',
'protectexpiry'               => 'Caducidade:',
'protect_expiry_invalid'      => 'O tempo de duración da protección non e válido.',
'protect_expiry_old'          => 'O momento de remate da protección corresponde ao pasado.',
'protect-unchain'             => 'Desbloquear os permisos de traslado',
'protect-text'                => 'Aquí é onde pode ver e cambiar os niveis de protección da páxina chamada "<strong><nowiki>$1</nowiki></strong>".',
'protect-locked-blocked'      => 'Non pode modificar os niveis de protección mentres exista un bloqueo. Velaquí a configuración actual da páxina  <strong>$1</strong>:',
'protect-locked-dblock'       => 'Os niveis de protección non se poden modificar debido a un bloqueo da base de datos activa.
Velaquí a configuración actual da páxina <strong>$1</strong>:',
'protect-locked-access'       => 'A súa conta non dispón de permisos para mudar os niveis de protección.
Velaquí a configuración actual da páxina <strong>$1</strong>:',
'protect-cascadeon'           => 'Esta páxina está protexida neste momento porque está incluída {{PLURAL:$1|na seguinte páxina, que foi protexida|páxinas, que foron protexidas}} coa opción protección en serie activada. Pode mudar o nivel de protección da páxina pero iso non afectará á protección en serie.',
'protect-default'             => '(predeterminado)',
'protect-fallback'            => 'Require permisos de "$1"',
'protect-level-autoconfirmed' => 'Bloquear usuarios non rexistrados',
'protect-level-sysop'         => 'Só os administradores',
'protect-summary-cascade'     => 'protección en serie',
'protect-expiring'            => 'remata $1 (UTC)',
'protect-cascade'             => 'Protexer as páxinas incluídas nesta (protección en serie)',
'protect-cantedit'            => 'Non pode modificar os niveis de protección desta páxina porque non ten permiso para editala.',
'restriction-type'            => 'Permiso',
'restriction-level'           => 'Nivel de protección:',
'minimum-size'                => 'Tamaño mínimo',
'maximum-size'                => 'Tamaño máximo:',
'pagesize'                    => '(bytes)',

# Restrictions (nouns)
'restriction-edit'   => 'Editar',
'restriction-move'   => 'Mover',
'restriction-create' => 'Crear',
'restriction-upload' => 'Cargar',

# Restriction levels
'restriction-level-sysop'         => 'protección completa',
'restriction-level-autoconfirmed' => 'semiprotexida',
'restriction-level-all'           => 'todos',

# Undelete
'undelete'                     => 'Ver as páxinas borradas',
'undeletepage'                 => 'Ver e restaurar páxinas borradas',
'undeletepagetitle'            => "'''A continuación amósanse as revisións eliminadas de''' \"'''[[:\$1|\$1]]'''\".",
'viewdeletedpage'              => 'Ver as páxinas borradas',
'undeletepagetext'             => 'As seguintes páxinas foron borradas, pero aínda están no arquivo e poden ser restauradas.
O arquivo será limpado periodicamente.',
'undelete-fieldset-title'      => 'Restaurar as revisións',
'undeleteextrahelp'            => "Para restaurar o historial dunha páxina ao completo, deixe todas as caixas sen marcar e prema en '''''Restaurar'''''.
Para realizar unha recuperación parcial, marque só aquelas caixas que correspondan ás revisións que se queiran recuperar e prema en '''''Restaurar'''''.
Ao premer en '''''Limpar''''', bórranse o campo do comentario e todas as caixas.",
'undeleterevisions'            => '$1 {{PLURAL:$1|revisión arquivada|revisións arquivadas}}',
'undeletehistory'              => 'Se restaura a páxina, todas as revisións van ser restauradas no historial.
Se se creou unha páxina nova co mesmo nome desde o seu borrado, as revisións restauradas van aparecer no historial anterior.',
'undeleterevdel'               => 'Non se levará a cabo a reversión do borrado se ocasiona que a última revisión da páxina ou ficheiro se elimine parcialmente.
Nestes casos, debe retirar a selección ou quitar a ocultación das revisións borradas máis recentes.',
'undeletehistorynoadmin'       => 'Esta páxina foi borrada. O motivo do borrado consta no resumo de embaixo, xunto cos detalles dos usuarios que editaron esta páxina antes da súa eliminación.
O texto das revisións eliminadas só está á disposición dos administradores.',
'undelete-revision'            => 'Revisión eliminada de "$1" (ás $2) feita por $3:',
'undeleterevision-missing'     => 'Revisión non válida ou inexistente. Pode que a ligazón conteña un erro ou que a revisión se restaurase ou eliminase do arquivo.',
'undelete-nodiff'              => 'Non se atopou ningunha revisión anterior.',
'undeletebtn'                  => 'Restaurar',
'undeletelink'                 => 'restaurar',
'undeletereset'                => 'Limpar',
'undeletecomment'              => 'Razón para desprotexer:',
'undeletedarticle'             => 'restaurou "[[$1]]"',
'undeletedrevisions'           => '$1 {{PLURAL:$1|revisión restaurada|revisións restauradas}}',
'undeletedrevisions-files'     => '$1 {{PLURAL:$1|revisión|revisións}} e $2 {{PLURAL:$2|ficheiro restaurado|ficheiros restaurados}}',
'undeletedfiles'               => '$1 {{PLURAL:$1|ficheiro restaurado|ficheiros restaurados}}',
'cannotundelete'               => 'Non se restaurou a páxina porque alguén xa o fixo antes.',
'undeletedpage'                => "<big>'''$1 foi restaurado'''</big>

Comprobe o [[Special:Log/delete|rexistro de borrados]] para ver as entradas recentes no rexistro de páxinas eliminadas e restauradas.",
'undelete-header'              => 'Vexa [[Special:Log/delete|no rexistro de borrados]] as páxinas eliminadas recentemente.',
'undelete-search-box'          => 'Buscar páxinas borradas',
'undelete-search-prefix'       => 'Mostrar as páxinas que comecen por:',
'undelete-search-submit'       => 'Procurar',
'undelete-no-results'          => 'Non se atoparon páxinas coincidentes no arquivo de eliminacións.',
'undelete-filename-mismatch'   => 'Non se pode desfacer a eliminación da revisión do ficheiro datada en $1: non corresponde o nome do ficheiro',
'undelete-bad-store-key'       => 'Non se pode desfacer o borrado da revisión do ficheiro datada en $1: o ficheiro faltaba antes de proceder a borralo.',
'undelete-cleanup-error'       => 'Erro ao eliminar o ficheiro do arquivo sen usar "$1".',
'undelete-missing-filearchive' => 'Non foi posíbel restaurar o ID do arquivo do ficheiro $1 porque non figura na base de datos. Pode que xa se desfixese a eliminación con anterioridade.',
'undelete-error-short'         => 'Erro ao desfacer a eliminación do ficheiro: $1',
'undelete-error-long'          => 'Atopáronse erros ao desfacer a eliminación do ficheiro:

$1',

# Namespace form on various pages
'namespace'      => 'Espazo de nomes:',
'invert'         => 'Invertir a selección',
'blanknamespace' => '(Principal)',

# Contributions
'contributions' => 'Contribucións do usuario',
'mycontris'     => 'As miñas contribucións',
'contribsub2'   => 'De $1 ($2)',
'nocontribs'    => 'Non se deron atopado cambios con eses criterios.',
'uctop'         => '(última revisión)',
'month'         => 'Desde o mes de (e anteriores):',
'year'          => 'Desde o ano (e anteriores):',

'sp-contributions-newbies'     => 'Mostrar só as contribucións das contas de usuario novas',
'sp-contributions-newbies-sub' => 'Contribucións dos usuarios novos',
'sp-contributions-blocklog'    => 'Rexistro de bloqueos',
'sp-contributions-search'      => 'Busca de contribucións',
'sp-contributions-username'    => 'Enderezo IP ou nome de usuario:',
'sp-contributions-submit'      => 'Procurar',

# What links here
'whatlinkshere'            => 'Páxinas que ligan con esta',
'whatlinkshere-title'      => 'Páxinas que ligan con "$1"',
'whatlinkshere-page'       => 'Páxina:',
'linklistsub'              => '(Listaxe de ligazóns)',
'linkshere'                => "As seguintes páxinas ligan con '''[[:$1]]''':",
'nolinkshere'              => "Ningunha páxina liga con \"'''[[:\$1]]'''\".",
'nolinkshere-ns'           => "Ningunha páxina liga con \"'''[[:\$1]]'''\" no espazo de nomes elixido.",
'isredirect'               => 'páxina redirixida',
'istemplate'               => 'inclusión',
'isimage'                  => 'ligazón á imaxe',
'whatlinkshere-prev'       => '{{PLURAL:$1|anterior|$1 anteriores}}',
'whatlinkshere-next'       => '{{PLURAL:$1|seguinte|$1 seguintes}}',
'whatlinkshere-links'      => '← ligazóns',
'whatlinkshere-hideredirs' => '$1 as redireccións',
'whatlinkshere-hidetrans'  => '$1 as inclusións',
'whatlinkshere-hidelinks'  => '$1 as ligazóns',
'whatlinkshere-hideimages' => '$1 as ligazóns á imaxe',
'whatlinkshere-filters'    => 'Filtros',

# Block/unblock
'blockip'                         => 'Bloquear un usuario',
'blockip-legend'                  => 'Bloquear un usuario',
'blockiptext'                     => 'Use o seguinte formulario para bloquear o acceso de escritura desde un enderezo IP ou para bloquear a un usuario específico.
Isto debería facerse só para previr vandalismo, e de acordo coa [[{{MediaWiki:Policy-url}}|política e normas]] vixentes.
Explique a razón específica do bloqueo (por exemplo, citando as páxinas concretas que sufriron vandalismo).',
'ipaddress'                       => 'Enderezo IP:',
'ipadressorusername'              => 'Enderezo IP ou nome de usuario:',
'ipbexpiry'                       => 'Remate:',
'ipbreason'                       => 'Razón:',
'ipbreasonotherlist'              => 'Outro motivo',
'ipbreason-dropdown'              => '
*Mensaxes de bloqueo comúns
** Inserir información falsa
** Eliminar o contido de páxinas
** Ligazóns lixo a sitios externos
** Inserir textos sen sentido ou inintelixíbeis
** Comportamento intimidatorio/acoso
** Abuso de múltiples contas de usuario
** Nome de usuario inaceptábel',
'ipbanononly'                     => 'Bloquear os usuarios anónimos unicamente',
'ipbcreateaccount'                => 'Previr a creación de contas',
'ipbemailban'                     => 'Impedir que o usuario envíe correos electrónicos',
'ipbenableautoblock'              => 'Bloquear automaticamente o último enderezo IP utilizado por este usuario, e calquera outro enderezo desde o que intente editar',
'ipbsubmit'                       => 'Bloquear este usuario',
'ipbother'                        => 'Outro período de tempo:',
'ipboptions'                      => '2 horas:2 hours,1 día:1 day,3 días:3 days,1 semana:1 week,2 semanas:2 weeks,1 mes:1 month,3 meses:3 months,6 meses:6 months,1 ano:1 year,para sempre:infinite', # display1:time1,display2:time2,...
'ipbotheroption'                  => 'outra',
'ipbotherreason'                  => 'Outro motivo:',
'ipbhidename'                     => 'Agochar nome de usuario no rexistro de bloqueos, na listaxe de bloqueos activos e na listaxe de usuarios',
'ipbwatchuser'                    => 'Vixiar a páxina de usuario e a de conversa deste usuario',
'badipaddress'                    => 'O enderezo IP non é válido',
'blockipsuccesssub'               => 'Bloqueo con éxito',
'blockipsuccesstext'              => 'O enderezo IP [[Special:Contributions/$1|$1]] foi bloqueado.<br />
Olle a [[Special:IPBlockList|lista de enderezos IP e usuarios bloqueados]] para revisalo.',
'ipb-edit-dropdown'               => 'Editar os motivos de bloqueo',
'ipb-unblock-addr'                => 'Desbloquear a "$1"',
'ipb-unblock'                     => 'Desbloquear un usuario ou enderezo IP',
'ipb-blocklist-addr'              => 'Ver os bloqueos vixentes de "$1"',
'ipb-blocklist'                   => 'Ver bloqueos vixentes',
'unblockip'                       => 'Desbloquear o usuario',
'unblockiptext'                   => 'Use o seguinte formulario para dar de novo acceso de escritura a un enderezo IP ou usuario que estea bloqueado.',
'ipusubmit'                       => 'Desbloquear este enderezo',
'unblocked'                       => '[[User:$1|$1]] foi desbloqueado',
'unblocked-id'                    => 'Eliminouse o bloqueo de $1',
'ipblocklist'                     => 'Enderezos IP e usuarios bloqueados',
'ipblocklist-legend'              => 'Buscar un usuario bloqueado',
'ipblocklist-username'            => 'Nome de usuario ou enderezo IP:',
'ipblocklist-submit'              => 'Procurar',
'blocklistline'                   => '$1, $2 bloqueou a "$3" ($4)',
'infiniteblock'                   => 'para sempre',
'expiringblock'                   => 'remata $1',
'anononlyblock'                   => 'só anón.',
'noautoblockblock'                => 'autobloqueo desactivado',
'createaccountblock'              => 'bloqueada a creación de contas',
'emailblock'                      => 'correo electrónico bloqueado',
'ipblocklist-empty'               => 'A listaxe de bloqueos está baleira.',
'ipblocklist-no-results'          => 'Nin o enderezo IP nin o nome de usuario solicitados están bloqueados.',
'blocklink'                       => 'bloquear',
'unblocklink'                     => 'desbloquear',
'contribslink'                    => 'contribucións',
'autoblocker'                     => 'Autobloqueado porque "[[User:$1|$1]]" usou recentemente este enderezo IP. O motivo do bloqueo de $1 é: "$2".',
'blocklogpage'                    => 'Rexistro de bloqueos',
'blocklogentry'                   => 'bloqueou a "[[$1]]" cun tempo de duración de $2 $3',
'blocklogtext'                    => 'Este é o rexistro das accións de bloqueo e desbloqueo de usuarios.
Non se listan os enderezos IP bloqueados automaticamente.
Olle a [[Special:IPBlockList|lista de enderezos IP e usuarios bloqueados]] se quere comprobar a lista cos bloqueos vixentes.',
'unblocklogentry'                 => 'desbloqueou a "$1"',
'block-log-flags-anononly'        => 'só usuarios anónimos',
'block-log-flags-nocreate'        => 'desactivada a creación de contas de usuario',
'block-log-flags-noautoblock'     => 'bloqueo automático deshabilitado',
'block-log-flags-noemail'         => 'correo electrónico bloqueado',
'block-log-flags-angry-autoblock' => 'realzou o autobloqueo permitido',
'range_block_disabled'            => 'A funcionalidade de administrador de crear rangos de bloqueos está deshabilitada.',
'ipb_expiry_invalid'              => 'Tempo de duración non válido.',
'ipb_expiry_temp'                 => 'Os bloqueos a nomes de usuario agochados deberían ser permanentes.',
'ipb_already_blocked'             => '"$1" xa está bloqueado',
'ipb_cant_unblock'                => 'Erro: Non se atopa o Block ID $1. Posiblemente xa foi desbloqueado.',
'ipb_blocked_as_range'            => 'Erro: O enderezo IP $1 non está bloqueado directamente e non se pode desbloquear. Porén, está bloqueado por estar no rango $2, que si se pode desbloquear.',
'ip_range_invalid'                => 'Rango IP non válido.',
'blockme'                         => 'Bloquearme',
'proxyblocker'                    => 'Bloqueador de proxy',
'proxyblocker-disabled'           => 'Esta función está desactivada.',
'proxyblockreason'                => 'O seu enderezo de IP foi bloqueado porque é un proxy aberto. Por favor contacte co seu fornecedor de acceso a internet ou co seu soporte técnico e informe deste grave problema de seguridade.',
'proxyblocksuccess'               => 'Feito.',
'sorbsreason'                     => 'O seu enderezo IP está rexistrado na listaxe DNSBL usada por {{SITENAME}}.',
'sorbs_create_account_reason'     => "O seu enderezo IP está rexistrado como un ''proxy'' aberto na listaxe DNSBL usada por {{SITENAME}}. Polo tanto non pode crear unha conta de acceso",

# Developer tools
'lockdb'              => 'Fechar base de datos',
'unlockdb'            => 'Desbloquear a base de datos',
'lockdbtext'          => 'Fechar a base de datos vai quitarlles aos usuarios a posibilidade de editar páxinas,cambiar as súas preferencias, editar as súas listaxes de vixilancia e outras cousas que requiren cambios na base de datos.
Por favor confirme que é o que realmente quere facer, e que vai quitar o fechamento da base de datos cando o mantemento estea rematado.',
'unlockdbtext'        => 'O desbloqueo da base de datos vai permitir que os usuarios poidan editar páxinas, cambiar as súas preferencias, editar as súas listaxes de vixilancia e outras accións que requiran cambios na base de datos.
Por favor confirme que isto é o que quere facer.',
'lockconfirm'         => 'Si, realmente quero fechar a base de datos.',
'unlockconfirm'       => 'Si, realmente quero desbloquear a base de datos',
'lockbtn'             => 'Fechar base de datos',
'unlockbtn'           => 'Desbloquear a base de datos',
'locknoconfirm'       => 'Vostede non marcou o sinal de confirmación.',
'lockdbsuccesssub'    => 'A base de datos foi fechada con éxito',
'unlockdbsuccesssub'  => 'Quitouse a protección da base de datos',
'lockdbsuccesstext'   => 'A base de datos foi fechada.<br />
Lembre [[Special:UnlockDB|eliminar o bloqueo]] unha vez completado o seu mantemento.',
'unlockdbsuccesstext' => 'A base de datos foi desbloqueada.',
'lockfilenotwritable' => 'Non se pode escribir no ficheiro de bloqueo da base de datos. Para bloquear ou desbloquear a base de datos, o servidor web ten que poder escribir neste ficheiro.',
'databasenotlocked'   => 'A base de datos non está bloqueada.',

# Move page
'move-page'               => 'Mover "$1"',
'move-page-legend'        => 'Mover páxina',
'movepagetext'            => "Ao usar o formulario de embaixo vai cambiar o nome da páxina, movendo todo o seu historial ao novo nome.
O título vello vaise converter nunha páxina de redirección ao novo título.
Pode actualizar automaticamente as redireccións que van dar ao título orixinal.
Se escolle non facelo, asegúrese de verificar que non hai redireccións [[Special:DoubleRedirects|dobres]] ou [[Special:BrokenRedirects|crebadas]].
Vostede é responsábel de asegurarse de que as ligazóns continúan a apuntar cara a onde se supón que deberían.

Teña en conta que a páxina '''non''' será movida se xa existe unha páxina co novo título, a menos que estea baleira ou sexa unha redirección e que non teña historial de edicións.
Isto significa que pode volver renomear unha páxina ao seu nome antigo se comete un erro, e que non pode sobreescribir nunha páxina que xa existe.

'''ATENCIÓN!'''
Este cambio nunha páxina popular pode ser drástico e inesperado;
por favor, asegúrese de que entende as consecuencias disto antes de proseguir.",
'movepagetalktext'        => "A páxina de conversa asociada, se existe, será automaticamente movida con esta '''agás que''':
*Estea a mover a páxina empregando espazos de nomes,
*Xa exista unha páxina de conversa con ese nome, ou
*Desactive a opción de abaixo.

Nestes casos, terá que mover ou mesturar a páxina manualmente se o desexa.",
'movearticle'             => 'Mover esta páxina:',
'movenotallowed'          => 'Non ten os permisos necesarios para mover páxinas.',
'newtitle'                => 'Ao novo título:',
'move-watch'              => 'Vixiar esta páxina',
'movepagebtn'             => 'Mover a páxina',
'pagemovedsub'            => 'O movemento foi un éxito',
'movepage-moved'          => '<big>\'\'\'"$1" foi movida a "$2"\'\'\'</big>', # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => 'Xa existe unha páxina con ese nome, ou o nome que escolleu non é válido.
Por favor escolla outro nome.',
'cantmove-titleprotected' => 'Vostede non pode mover a páxina a esta ubicación, porque o novo título foi protexido da creación',
'talkexists'              => "'''Só foi movida con éxito a páxina, pero a páxina de conserva non puido ser movida porque xa existe unha co novo título. Por favor, mestúreas de xeito manual.'''",
'movedto'                 => 'movido a',
'movetalk'                => 'Mover a páxina de conversa, se cómpre',
'move-subpages'           => 'Mover todas as subpáxinas, se cómpre',
'move-talk-subpages'      => 'Mover todas as subpáxinas da páxina de conversa, se cómpre',
'movepage-page-exists'    => 'A páxina "$1" xa existe e non pode ser sobreescrita automaticamente.',
'movepage-page-moved'     => 'A páxina "$1" foi movida a "$2".',
'movepage-page-unmoved'   => 'A páxina "$1" non pode ser movida a "$2".',
'movepage-max-pages'      => 'Foi movido o número máximo {{PLURAL:$1|dunha páxina|de $1 páxinas}} e non poderán ser movidas automaticamente máis.',
'1movedto2'               => 'moveu "[[$1]]" a "[[$2]]"',
'1movedto2_redir'         => 'moveu "[[$1]]" a "[[$2]]" sobre unha redirección',
'movelogpage'             => 'Rexistro de traslados',
'movelogpagetext'         => 'Abaixo móstrase unha listaxe de páxinas trasladadas.',
'movereason'              => 'Motivo:',
'revertmove'              => 'reverter',
'delete_and_move'         => 'Borrar e mover',
'delete_and_move_text'    => '==Precísase borrar==
A páxina de destino, chamada "[[:$1]]", xa existe.
Quérea eliminar para facer sitio para mover?',
'delete_and_move_confirm' => 'Si, borrar a páxina',
'delete_and_move_reason'  => 'Eliminado para facer sitio para mover',
'selfmove'                => 'O título de orixe e o de destino é o mesmo; non se pode mover unha páxina sobre si mesma.',
'immobile_namespace'      => 'O título de orixe ou o de destino son dunha clase especial; non poden moverse as páxinas desde ou a ese espazo de nomes.',
'imagenocrossnamespace'   => 'Non se pode mover o ficheiro a un espazo de nomes que non o admite',
'imagetypemismatch'       => 'A nova extensión do fiheiro non coincide co seu tipo',
'imageinvalidfilename'    => 'O nome da imaxe é inválido',
'fix-double-redirects'    => 'Actualizar calquera redirección que apunte cara ao título orixinal',

# Export
'export'            => 'Exportar páxinas',
'exporttext'        => 'Pode exportar o texto e o historial de edición dunha páxina calquera ou un conxunto de páxinas agrupadas nalgún ficheiro XML. Este pódese importar noutro wiki que utilice o programa MediaWiki mediante a [[Special:Import|páxina de importación]].

Para exportar páxinas, insira os títulos na caixa de texto que está máis abaixo, poñendo un título por liña, e se quere seleccione a versión actual e todas as versións vellas, coas liñas do historial da páxina, ou só a versión actual con información sobre a última edición.

No último caso, pode usar tamén unha ligazón, por exemplo [[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]], para a páxina "[[{{MediaWiki:Mainpage}}]]".',
'exportcuronly'     => 'Incluír só a revisión actual, non o historial completo',
'exportnohistory'   => "----
'''Aviso:''' foi desactivada a exportación do historial completo das páxinas con este formulario debido a razóns relacionadas co rendemento do servidor.",
'export-submit'     => 'Exportar',
'export-addcattext' => 'Engadir páxinas da categoría:',
'export-addcat'     => 'Engadir',
'export-download'   => 'Ofrecer gardar como un ficheiro',
'export-templates'  => 'Incluír os modelos',

# Namespace 8 related
'allmessages'               => 'Todas as mensaxes do sistema',
'allmessagesname'           => 'Nome',
'allmessagesdefault'        => 'Texto predeterminado',
'allmessagescurrent'        => 'Texto actual',
'allmessagestext'           => 'Esta é unha listaxe de todas as mensaxes dispoñíbeis no espazo de nomes MediaWiki.
Por favor, visite a [http://www.mediawiki.org/wiki/Localisation localización MediaWiki] e [http://translatewiki.net Betawiki] se quere contribuír á localización xenérica de MediaWiki.',
'allmessagesnotsupportedDB' => "'''{{ns:special}}:Allmessages''' non está dispoñíbel porque '''\$wgUseDatabaseMessages''' está desactivado.",
'allmessagesfilter'         => 'Filtrar polo nome da mensaxe:',
'allmessagesmodified'       => 'Amosar só as modificadas',

# Thumbnails
'thumbnail-more'           => 'Agrandado',
'filemissing'              => 'O ficheiro non se dá atopado',
'thumbnail_error'          => 'Erro ao crear a imaxe en miniatura: $1',
'djvu_page_error'          => 'Páxina DjVu fóra de rango',
'djvu_no_xml'              => 'Foi imposíbel obter o XML para o ficheiro DjVu',
'thumbnail_invalid_params' => 'Parámetros de miniatura non válidos',
'thumbnail_dest_directory' => 'Foi imposíbel crear un directorio de destino',

# Special:Import
'import'                     => 'Importar páxinas',
'importinterwiki'            => 'Importación transwiki',
'import-interwiki-text'      => 'Seleccione o wiki e o título da páxina que queira importar.
As datas das revisións e os nomes dos editores mantéranse.
Todas as accións relacionadas coa importación entre wikis poden verse no [[Special:Log/import|rexistro de importacións]].',
'import-interwiki-history'   => 'Copiar todas as versións que hai no historial desta páxina',
'import-interwiki-submit'    => 'Importar',
'import-interwiki-namespace' => 'Transferir páxinas ao espazo de nomes:',
'importtext'                 => 'Por favor, exporte o ficheiro do wiki de orixe usando a [[Special:Export|ferramenta para exportar]].
Gráveo no seu disco duro e cárgueo aquí.',
'importstart'                => 'Importando páxinas...',
'import-revision-count'      => '$1 {{PLURAL:$1|revisión|revisións}}',
'importnopages'              => 'Non hai páxinas para importar.',
'importfailed'               => 'A importación fallou: $1',
'importunknownsource'        => 'Fonte de importación descoñecida',
'importcantopen'             => 'Non se pode abrir o ficheiro importado',
'importbadinterwiki'         => 'Ligazón entre wikis incorrecta',
'importnotext'               => 'Texto baleiro ou inexistente',
'importsuccess'              => 'A importación rematou!',
'importhistoryconflict'      => 'Existe un conflito no historial de revisións (por ter importado esta páxina antes)',
'importnosources'            => 'Non se definiron fontes de importación transwiki e están desactivados os envíos directos dos historiais.',
'importnofile'               => 'Non se enviou ningún ficheiro de importación.',
'importuploaderrorsize'      => 'Fallou o envío do ficheiro de importación. O ficheiro é máis grande que o tamaño de envío permitido.',
'importuploaderrorpartial'   => 'Fallou o envío do ficheiro de importación. O ficheiro só se enviou parcialmente.',
'importuploaderrortemp'      => 'Fallou o envío do ficheiro de importación. Falta un cartafol temporal.',
'import-parse-failure'       => 'Fallo de análise da importación de XML',
'import-noarticle'           => 'Ningunha páxina para importar!',
'import-nonewrevisions'      => 'Todas as revisións son previamente importadas.',
'xml-error-string'           => '$1 na liña $2, col $3 (byte $4): $5',
'import-upload'              => 'Cargar datos XML',

# Import log
'importlogpage'                    => 'Rexistro de importacións',
'importlogpagetext'                => 'Rexistro de importación de páxinas xunto co seu historial de edicións procedentes doutros wikis.',
'import-logentry-upload'           => 'importou "[[$1]]" mediante a carga dun ficheiro',
'import-logentry-upload-detail'    => '$1 {{PLURAL:$1|revisión|revisións}}',
'import-logentry-interwiki'        => 'importada $1',
'import-logentry-interwiki-detail' => '$1 {{PLURAL:$1|revisión|revisións}} de $2',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'A miña páxina de usuario',
'tooltip-pt-anonuserpage'         => 'A páxina de usuario da IP desde a que está a editar',
'tooltip-pt-mytalk'               => 'A miña páxina de conversa',
'tooltip-pt-anontalk'             => 'Conversa acerca de edicións feitas desde este enderezo IP',
'tooltip-pt-preferences'          => 'As miñas preferencias',
'tooltip-pt-watchlist'            => 'Listaxe de páxinas cuxas modificacións estou a seguir',
'tooltip-pt-mycontris'            => 'Listaxe das miñas contribucións',
'tooltip-pt-login'                => 'Recoméndaselle que acceda ao sistema, porén, non é obrigatorio.',
'tooltip-pt-anonlogin'            => 'Recoméndaselle rexistrarse, se ben non é obrigatorio.',
'tooltip-pt-logout'               => 'Saír do sistema',
'tooltip-ca-talk'                 => 'Conversa acerca do contido desta páxina',
'tooltip-ca-edit'                 => 'Pode modificar esta páxina; antes de gardala, por favor, utilice o botón de vista previa',
'tooltip-ca-addsection'           => 'Contribúa cun comentario a esta conversa.',
'tooltip-ca-viewsource'           => 'Esta páxina está protexida. Pode ver o código fonte.',
'tooltip-ca-history'              => 'Versións anteriores desta páxina',
'tooltip-ca-protect'              => 'Protexer esta páxina',
'tooltip-ca-delete'               => 'Eliminar esta páxina',
'tooltip-ca-undelete'             => 'Restaurar as edicións feitas nesta páxina antes de que fose eliminada',
'tooltip-ca-move'                 => 'Mover esta páxina',
'tooltip-ca-watch'                => 'Engadir esta páxina á listaxe de vixilancia',
'tooltip-ca-unwatch'              => 'Eliminar esta páxina da súa listaxe de vixilancia',
'tooltip-search'                  => 'Procurar en {{SITENAME}}',
'tooltip-search-go'               => 'Ir a unha páxina con este texto exacto, se existe',
'tooltip-search-fulltext'         => 'Procurar este texto nas páxinas',
'tooltip-p-logo'                  => 'Portada',
'tooltip-n-mainpage'              => 'Visitar a Portada',
'tooltip-n-portal'                => 'Acerca do proxecto, o que vostede pode facer, onde atopar cousas',
'tooltip-n-currentevents'         => 'Atopar documentación acerca de acontecementos de actualidade',
'tooltip-n-recentchanges'         => 'A listaxe de modificacións recentes no wiki.',
'tooltip-n-randompage'            => 'Carregar unha páxina ao chou',
'tooltip-n-help'                  => 'O lugar para informarse.',
'tooltip-t-whatlinkshere'         => 'Listaxe de todas as páxinas do wiki que ligan cara a aquí',
'tooltip-t-recentchangeslinked'   => 'Cambios recentes nas páxinas ligadas desde esta',
'tooltip-feed-rss'                => 'Fonte de noticias RSS para esta páxina',
'tooltip-feed-atom'               => 'Fonte de noticias Atom para esta páxina',
'tooltip-t-contributions'         => 'Ver a listaxe de contribucións deste usuario',
'tooltip-t-emailuser'             => 'Enviarlle unha mensaxe a este usuario por correo electrónico',
'tooltip-t-upload'                => 'Enviar ficheiros',
'tooltip-t-specialpages'          => 'Listaxe de todas as páxinas especiais',
'tooltip-t-print'                 => 'Versión imprimíbel desta páxina',
'tooltip-t-permalink'             => 'Ligazón permanente a esta versión da páxina',
'tooltip-ca-nstab-main'           => 'Ver o contido da páxina',
'tooltip-ca-nstab-user'           => 'Ver a páxina do usuario',
'tooltip-ca-nstab-media'          => 'Ver a páxina con contido multimedia',
'tooltip-ca-nstab-special'        => 'Esta é unha páxina especial, polo que non a pode editar',
'tooltip-ca-nstab-project'        => 'Ver a páxina do proxecto',
'tooltip-ca-nstab-image'          => 'Ver a páxina do ficheiro',
'tooltip-ca-nstab-mediawiki'      => 'Ver a mensaxe do sistema',
'tooltip-ca-nstab-template'       => 'Ver o modelo',
'tooltip-ca-nstab-help'           => 'Ver a páxina de axuda',
'tooltip-ca-nstab-category'       => 'Ver a páxina da categoría',
'tooltip-minoredit'               => 'Marcar isto coma unha edición pequena',
'tooltip-save'                    => 'Gravar os seus cambios',
'tooltip-preview'                 => 'Vista previa dos seus cambios; por favor, úsea antes de gravalos!',
'tooltip-diff'                    => 'Mostrar os cambios que fixo no texto',
'tooltip-compareselectedversions' => 'Ver as diferenzas entre as dúas versións seleccionadas desta páxina',
'tooltip-watch'                   => 'Engadir esta páxina á súa listaxe de vixilancia [alt-w]',
'tooltip-recreate'                => 'Recrear a páxina a pesar de que foi borrada',
'tooltip-upload'                  => 'Comezar a enviar',

# Stylesheets
'common.css'   => '/** O CSS que se coloque aquí será aplicado a todas as aparencias */',
'monobook.css' => '/* O CSS que se coloque aquí afectará a quen use a aparencia Monobook */',

# Scripts
'common.js'   => '/* Calquera JavaScript será cargado para todos os usuarios en cada páxina cargada. */',
'monobook.js' => '/* O JavaScript que apareza aquí só será cargado aos usuarios que usan a apariencia MonoBook. */',

# Metadata
'nodublincore'      => 'A opción de metadatos RDF do Dublin Core está desactivada neste servidor.',
'nocreativecommons' => 'A opción de metadatos Creative Commons RDF está desactivada neste servidor.',
'notacceptable'     => 'O servidor wiki non pode fornecer datos nun formato que o seu cliente poida ler.',

# Attribution
'anonymous'        => 'Usuario(s) anónimo(s) de {{SITENAME}}',
'siteuser'         => '{{SITENAME}} usuario $1',
'lastmodifiedatby' => 'A última modificación desta páxina foi o $1 as $2 por $3.', # $1 date, $2 time, $3 user
'othercontribs'    => 'Baseado no traballo de $1.',
'others'           => 'outros',
'siteusers'        => '{{SITENAME}} usuario(s) $1',
'creditspage'      => 'Páxina de créditos',
'nocredits'        => 'Non hai información de créditos dispoñíbel para esta páxina.',

# Spam protection
'spamprotectiontitle' => "Filtro de protección de ''spam''",
'spamprotectiontext'  => "A páxina que quixo gardar foi bloqueada polo filtro ''antispam''.
Isto, probabelmente, se debe a unha ligazón cara a un sitio externo que está na lista negra.",
'spamprotectionmatch' => "O seguinte texto foi o que activou o noso filtro de ''spam'': $1",
'spambot_username'    => "MediaWiki limpeza de ''spam''",
'spam_reverting'      => 'Revertida á última edición sen ligazóns a $1',
'spam_blanking'       => 'Limpáronse todas as revisións con ligazóns a "$1"',

# Info page
'infosubtitle'   => 'Información da páxina',
'numedits'       => 'Número de edicións (artigo): $1',
'numtalkedits'   => 'Número de edicións (páxina de conversa): $1',
'numwatchers'    => 'Número de vixiantes: $1',
'numauthors'     => 'Número de autores distintos (artigo): $1',
'numtalkauthors' => 'Número de autores distintos (páxina de conversa): $1',

# Math options
'mw_math_png'    => 'Orixinar sempre unha imaxe PNG',
'mw_math_simple' => 'HTML se é moi simple, en caso contrario PNG',
'mw_math_html'   => 'Se é posible HTML, se non PNG',
'mw_math_source' => 'Deixalo como TeX (para navegadores de texto)',
'mw_math_modern' => 'Recomendado para as versións recentes dos navegadores',
'mw_math_mathml' => 'MathML se é posible (experimental)',

# Patrolling
'markaspatrolleddiff'                 => 'Marcar como revisada',
'markaspatrolledtext'                 => 'Marcar este artigo coma revisado',
'markedaspatrolled'                   => 'Marcar coma revisado',
'markedaspatrolledtext'               => 'A revisión seleccionada foi marcada como revisada.',
'rcpatroldisabled'                    => 'Patrulla de Cambios Recentes deshabilitada',
'rcpatroldisabledtext'                => 'A funcionalidade da Patrulla de Cambios Recentes está deshabilitada actualmente.',
'markedaspatrollederror'              => 'Non se pode marcar coma revisada',
'markedaspatrollederrortext'          => 'É preciso especificar unha revisión para marcala como revisada.',
'markedaspatrollederror-noautopatrol' => 'Non está permitido que un mesmo marque as propias edicións como revisadas.',

# Patrol log
'patrol-log-page'   => 'Rexistro de revisións',
'patrol-log-header' => 'Este é un rexistro das revisións patrulladas.',
'patrol-log-line'   => 'marcou a $1 de "$2" como revisada $3',
'patrol-log-auto'   => '(automático)',

# Image deletion
'deletedrevision'                 => 'A revisión vella $1 foi borrada.',
'filedeleteerror-short'           => 'Erro ao eliminar o ficheiro: $1',
'filedeleteerror-long'            => 'Atopáronse erros ao eliminar o ficheiro:

$1',
'filedelete-missing'              => 'Non se pode eliminar o ficheiro "$1" porque non existe.',
'filedelete-old-unregistered'     => 'A versión do ficheiro especificada, "$1", non figura na base de datos.',
'filedelete-current-unregistered' => 'O ficheiro especificado, "$1", non figura na base de datos.',
'filedelete-archive-read-only'    => 'O servidor web non pode escribir no directorio de arquivo "$1".',

# Browsing diffs
'previousdiff' => '← Edición máis vella',
'nextdiff'     => 'Edición máis nova →',

# Media information
'mediawarning'         => "'''Aviso''': este ficheiro pode conter código malicioso; o seu sistema pode quedar comprometido se chega a executalo.<hr />",
'imagemaxsize'         => 'Limitar as imaxes nas páxinas de descrición de ficheiros a:',
'thumbsize'            => 'Tamaño da miniatura:',
'widthheightpage'      => '$1×$2, $3 {{PLURAL:$3|páxina|páxinas}}',
'file-info'            => 'Tamaño do ficheiro: $1, tipo MIME: $2',
'file-info-size'       => '($1 × $2 píxeles, tamaño do ficheiro: $3, tipo MIME: $4)',
'file-nohires'         => '<small>Non se dispón dunha resolución máis grande.</small>',
'svg-long-desc'        => '(ficheiro SVG, nominalmente $1 × $2 píxeles, tamaño do ficheiro: $3)',
'show-big-image'       => 'Imaxe na máxima resolución',
'show-big-image-thumb' => '<small>Tamaño desta presentación da imaxe: $1 × $2 píxeles</small>',

# Special:NewImages
'newimages'             => 'Galería de imaxes novas',
'imagelisttext'         => "Abaixo amósase unha listaxe de '''$1''' {{PLURAL:$1|ficheiro|ficheiros}} ordenados $2.",
'newimages-summary'     => 'Esta páxina especial amosa os ficheiros cargados máis recentemente.',
'showhidebots'          => '($1 os bots)',
'noimages'              => 'Non hai imaxes para ver.',
'ilsubmit'              => 'Procurar',
'bydate'                => 'por data',
'sp-newimages-showfrom' => 'Mostrar os novos ficheiros comezando polo $1 ás $2',

# Video information, used by Language::formatTimePeriod() to format lengths in the above messages
'minutes-abbrev' => 'min',

# Bad image list
'bad_image_list' => 'O formato é o seguinte:

Só se consideran os elementos dunha listaxe (liñas que comezan por *). A primeira ligazón dunha liña ten que apuntar para unha imaxe mala.
As ligazóns posteriores da mesma liña considéranse excepcións, isto é, páxinas nas que o ficheiro pode aparecer inserido na liña.',

# Metadata
'metadata'          => 'Metadatos',
'metadata-help'     => 'Este ficheiro contén información adicional, probabelmente engadida pola cámara dixital ou polo escáner usado para crear ou dixitalizar a imaxe. Se o ficheiro orixinal foi modificado, pode que algúns detalles non se reflictan no ficheiro modificado.',
'metadata-expand'   => 'Mostrar os detalles',
'metadata-collapse' => 'Agochar os detalles',
'metadata-fields'   => 'Os campos de datos meta EXIF listados nesta mensaxe incluiranse ao exhibir a páxina da imaxe cando se reduza a táboa dos datos meta.
Outros agocharanse por omisión.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => 'Ancho',
'exif-imagelength'                 => 'Alto',
'exif-bitspersample'               => 'Bits por compoñente',
'exif-compression'                 => 'Esquema de compresión',
'exif-photometricinterpretation'   => 'Composición do píxel',
'exif-orientation'                 => 'Orientación',
'exif-samplesperpixel'             => 'Número de compoñentes',
'exif-planarconfiguration'         => 'Disposición dos datos',
'exif-ycbcrsubsampling'            => 'Razón de submostraxe de Y a C',
'exif-ycbcrpositioning'            => 'Posicionamentos Y e C',
'exif-xresolution'                 => 'Resolución horizontal',
'exif-yresolution'                 => 'Resolución vertical',
'exif-resolutionunit'              => 'Unidade de resolución X e Y',
'exif-stripoffsets'                => 'Localización dos datos da imaxe',
'exif-rowsperstrip'                => 'Número de filas por tira',
'exif-stripbytecounts'             => 'Bytes por tira comprimida',
'exif-jpeginterchangeformat'       => 'Distancia ao inicio (SOI) do JPEG',
'exif-jpeginterchangeformatlength' => 'Bytes de datos JPEG',
'exif-transferfunction'            => 'Función de transferencia',
'exif-whitepoint'                  => 'Coordenadas cromáticas de referencia do branco',
'exif-primarychromaticities'       => 'Cromacidades primarias',
'exif-ycbcrcoefficients'           => 'Coeficientes da matriz de transformación do espazo de cores',
'exif-referenceblackwhite'         => 'Par de valores de referencia branco e negro',
'exif-datetime'                    => 'Data e hora de modificación do ficheiro',
'exif-imagedescription'            => 'Título da imaxe',
'exif-make'                        => 'Fabricante da cámara',
'exif-model'                       => 'Modelo da cámara',
'exif-software'                    => 'Software utilizado',
'exif-artist'                      => 'Autor',
'exif-copyright'                   => 'Titular dos dereitos de autor (copyright)',
'exif-exifversion'                 => 'Versión Exif',
'exif-flashpixversion'             => 'Versión de Flashpix soportada',
'exif-colorspace'                  => 'Espazo de cor',
'exif-componentsconfiguration'     => 'Significado de cada compoñente',
'exif-compressedbitsperpixel'      => 'Modo de compresión da imaxe',
'exif-pixelydimension'             => 'Anchura da imaxe válida',
'exif-pixelxdimension'             => 'Altura da imaxe válida',
'exif-makernote'                   => 'Notas do fabricante',
'exif-usercomment'                 => 'Comentarios do usuario',
'exif-relatedsoundfile'            => 'Ficheiro de audio relacionado',
'exif-datetimeoriginal'            => 'Data e hora de xeración do ficheiro',
'exif-datetimedigitized'           => 'Data e hora de dixitalización',
'exif-subsectime'                  => 'DataHora subsegundos',
'exif-subsectimeoriginal'          => 'DataHoraOrixinal subsegundos',
'exif-subsectimedigitized'         => 'DataHoraDixitalización subsegundos',
'exif-exposuretime'                => 'Tempo de exposición',
'exif-exposuretime-format'         => '$1 seg. ($2)',
'exif-fnumber'                     => 'Número f',
'exif-exposureprogram'             => 'Programa de exposición',
'exif-spectralsensitivity'         => 'Sensibilidade espectral',
'exif-isospeedratings'             => 'Relación da velocidade ISO',
'exif-oecf'                        => 'Factor de conversión optoelectrónica',
'exif-shutterspeedvalue'           => 'Velocidade de obturación electrónica',
'exif-aperturevalue'               => 'Apertura',
'exif-brightnessvalue'             => 'Brillo',
'exif-exposurebiasvalue'           => 'Corrección da exposición',
'exif-maxaperturevalue'            => 'Máxima apertura do diafragma',
'exif-subjectdistance'             => 'Distancia do suxeito',
'exif-meteringmode'                => 'Modo de medida da exposición',
'exif-lightsource'                 => 'Fonte da luz',
'exif-flash'                       => 'Flash',
'exif-focallength'                 => 'Lonxitude focal',
'exif-subjectarea'                 => 'Área do suxeito',
'exif-flashenergy'                 => 'Enerxía do flash',
'exif-spatialfrequencyresponse'    => 'Resposta de frecuencia espacial',
'exif-focalplanexresolution'       => 'Resolución X do plano focal',
'exif-focalplaneyresolution'       => 'Resolución Y do plano focal',
'exif-focalplaneresolutionunit'    => 'Unidade de resolución do plano focal',
'exif-subjectlocation'             => 'Posición do suxeito',
'exif-exposureindex'               => 'Índice de exposición',
'exif-sensingmethod'               => 'Tipo de sensor',
'exif-filesource'                  => 'Fonte do ficheiro',
'exif-scenetype'                   => 'Tipo de escena',
'exif-cfapattern'                  => 'Patrón da matriz de filtro de cor',
'exif-customrendered'              => 'Procesamento da imaxe personalizado',
'exif-exposuremode'                => 'Modo de exposición',
'exif-whitebalance'                => 'Balance de brancos',
'exif-digitalzoomratio'            => 'Valor do zoom dixital',
'exif-focallengthin35mmfilm'       => 'Lonxitude focal na película de 35 mm',
'exif-scenecapturetype'            => 'Tipo de captura da escena',
'exif-gaincontrol'                 => 'Control de escena',
'exif-contrast'                    => 'Contraste',
'exif-saturation'                  => 'Saturación',
'exif-sharpness'                   => 'Nitidez',
'exif-devicesettingdescription'    => 'Descrición da configuración do dispositivo',
'exif-subjectdistancerange'        => 'Distancia ao suxeito',
'exif-imageuniqueid'               => 'ID única da imaxe',
'exif-gpsversionid'                => 'Versión da etiqueta GPS',
'exif-gpslatituderef'              => 'Latitude norte ou sur',
'exif-gpslatitude'                 => 'Latitude',
'exif-gpslongituderef'             => 'Lonxitude leste ou oeste',
'exif-gpslongitude'                => 'Lonxitude',
'exif-gpsaltituderef'              => 'Referencia da altitude',
'exif-gpsaltitude'                 => 'Altitude',
'exif-gpstimestamp'                => 'Hora GPS (reloxio atómico)',
'exif-gpssatellites'               => 'Satélites utilizados para a medida',
'exif-gpsstatus'                   => 'Estado do receptor',
'exif-gpsmeasuremode'              => 'Modo de medida',
'exif-gpsdop'                      => 'Precisión da medida',
'exif-gpsspeedref'                 => 'Unidade de velocidade',
'exif-gpsspeed'                    => 'Velocidade do receptor GPS',
'exif-gpstrackref'                 => 'Referencia para a dirección do movemento',
'exif-gpstrack'                    => 'Dirección do movemento',
'exif-gpsimgdirectionref'          => 'Referencia para a dirección da imaxe',
'exif-gpsimgdirection'             => 'Dirección da imaxe',
'exif-gpsmapdatum'                 => 'Usados datos xeodésicos de enquisas',
'exif-gpsdestlatituderef'          => 'Referencia para a latitude do destino',
'exif-gpsdestlatitude'             => 'Latitude do destino',
'exif-gpsdestlongituderef'         => 'Referencia para a lonxitude do destino',
'exif-gpsdestlongitude'            => 'Lonxitude do destino',
'exif-gpsdestbearingref'           => 'Referencia para a coordenada de destino',
'exif-gpsdestbearing'              => 'Coordenada de destino',
'exif-gpsdestdistanceref'          => 'Referencia para a distancia ao destino',
'exif-gpsdestdistance'             => 'Distancia ao destino',
'exif-gpsprocessingmethod'         => 'Nome do método de procesamento GPS',
'exif-gpsareainformation'          => 'Nome da área GPS',
'exif-gpsdatestamp'                => 'Data do GPS',
'exif-gpsdifferential'             => 'Corrección diferencial do GPS',

# EXIF attributes
'exif-compression-1' => 'Sen comprimir',

'exif-unknowndate' => 'Data descoñecida',

'exif-orientation-1' => 'Normal', # 0th row: top; 0th column: left
'exif-orientation-2' => 'Volteada horizontalmente', # 0th row: top; 0th column: right
'exif-orientation-3' => 'Rotada 180°', # 0th row: bottom; 0th column: right
'exif-orientation-4' => 'Volteada verticalmente', # 0th row: bottom; 0th column: left
'exif-orientation-5' => 'Rotada 90° CCW e volteada verticalmente', # 0th row: left; 0th column: top
'exif-orientation-6' => 'Rotada 90° CW', # 0th row: right; 0th column: top
'exif-orientation-7' => 'Rotada 90° CW e volteada verticalmente', # 0th row: right; 0th column: bottom
'exif-orientation-8' => 'Rotada 90° CCW', # 0th row: left; 0th column: bottom

'exif-planarconfiguration-1' => 'Formato de paquete de píxeles',
'exif-planarconfiguration-2' => 'Formato de planos',

'exif-componentsconfiguration-0' => 'non hai',

'exif-exposureprogram-0' => 'Sen definir',
'exif-exposureprogram-1' => 'Manual',
'exif-exposureprogram-2' => 'Programa normal',
'exif-exposureprogram-3' => 'Prioridade da apertura',
'exif-exposureprogram-4' => 'Prioridade da obturación',
'exif-exposureprogram-5' => 'Programa creativo (preferencia pola profundidade de campo)',
'exif-exposureprogram-6' => 'Programa de acción (preferencia por unha velocidade de exposición máis rápida)',
'exif-exposureprogram-7' => 'Modo retrato (para primeiros planos co fondo fóra de foco)',
'exif-exposureprogram-8' => 'Modo paisaxe (para paisaxes co fondo enfocado)',

'exif-subjectdistance-value' => '$1 metros',

'exif-meteringmode-0'   => 'Descoñecido',
'exif-meteringmode-1'   => 'Media',
'exif-meteringmode-2'   => 'Ponderado no centro',
'exif-meteringmode-3'   => 'Un punto',
'exif-meteringmode-4'   => 'Varios puntos',
'exif-meteringmode-5'   => 'Patrón de medición',
'exif-meteringmode-6'   => 'Parcial',
'exif-meteringmode-255' => 'Outro',

'exif-lightsource-0'   => 'Descoñecida',
'exif-lightsource-1'   => 'Luz do día',
'exif-lightsource-2'   => 'Fluorescente',
'exif-lightsource-3'   => 'Tungsteno (luz incandescente)',
'exif-lightsource-4'   => 'Flash',
'exif-lightsource-9'   => 'Bo tempo',
'exif-lightsource-10'  => 'Tempo anubrado',
'exif-lightsource-11'  => 'Sombra',
'exif-lightsource-12'  => 'Fluorescente luz de día (D 5700 – 7100K)',
'exif-lightsource-13'  => 'Fluorescente branco día (N 4600 – 5400K)',
'exif-lightsource-14'  => 'Fluorescente branco frío (W 3900 – 4500K)',
'exif-lightsource-15'  => 'Fluorescente branco (WW 3200 – 3700K)',
'exif-lightsource-17'  => 'Luz estándar A',
'exif-lightsource-18'  => 'Luz estándar B',
'exif-lightsource-19'  => 'Luz estándar C',
'exif-lightsource-24'  => 'Tungsteno de estudio ISO',
'exif-lightsource-255' => 'Outra fonte de luz',

'exif-focalplaneresolutionunit-2' => 'polgadas',

'exif-sensingmethod-1' => 'Sen definir',
'exif-sensingmethod-2' => 'Sensor da área de cor dun chip',
'exif-sensingmethod-3' => 'Sensor da área de cor de dous chips',
'exif-sensingmethod-4' => 'Sensor da área de cor de tres chips',
'exif-sensingmethod-5' => 'Sensor secuencial da área de cor',
'exif-sensingmethod-7' => 'Sensor trilineal',
'exif-sensingmethod-8' => 'Sensor secuencial da liña de cor',

'exif-scenetype-1' => 'Unha imaxe fotografada directamente',

'exif-customrendered-0' => 'Procesamento normal',
'exif-customrendered-1' => 'Procesamento personalizado',

'exif-exposuremode-0' => 'Exposición automática',
'exif-exposuremode-1' => 'Exposición manual',
'exif-exposuremode-2' => 'Compensación de exposición automática',

'exif-whitebalance-0' => 'Balance de brancos automático',
'exif-whitebalance-1' => 'Balance de brancos manual',

'exif-scenecapturetype-0' => 'Estándar',
'exif-scenecapturetype-1' => 'Paisaxe',
'exif-scenecapturetype-2' => 'Retrato',
'exif-scenecapturetype-3' => 'Escena nocturna',

'exif-gaincontrol-0' => 'Ningunha',
'exif-gaincontrol-1' => 'Baixa ganancia superior',
'exif-gaincontrol-2' => 'Alta ganancia superior',
'exif-gaincontrol-3' => 'Baixa ganancia inferior',
'exif-gaincontrol-4' => 'Alta ganancia inferior',

'exif-contrast-0' => 'Normal',
'exif-contrast-1' => 'Suave',
'exif-contrast-2' => 'Forte',

'exif-saturation-0' => 'Normal',
'exif-saturation-1' => 'Saturación baixa',
'exif-saturation-2' => 'Saturación alta',

'exif-sharpness-0' => 'Normal',
'exif-sharpness-1' => 'Suave',
'exif-sharpness-2' => 'Forte',

'exif-subjectdistancerange-0' => 'Descoñecida',
'exif-subjectdistancerange-1' => 'Macro',
'exif-subjectdistancerange-2' => 'Primeiro plano',
'exif-subjectdistancerange-3' => 'Paisaxe',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'Latitude norte',
'exif-gpslatitude-s' => 'Latitude sur',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'Lonxitude leste',
'exif-gpslongitude-w' => 'Lonxitude oeste',

'exif-gpsstatus-a' => 'Medida en progreso',
'exif-gpsstatus-v' => 'Interoperabilidade da medida',

'exif-gpsmeasuremode-2' => 'Medida bidimensional',
'exif-gpsmeasuremode-3' => 'Medida tridimensional',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'Quilómetros por hora',
'exif-gpsspeed-m' => 'Millas por hora',
'exif-gpsspeed-n' => 'Nós',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => 'Dirección verdadeira',
'exif-gpsdirection-m' => 'Dirección magnética',

# External editor support
'edit-externally'      => 'Editar este ficheiro cunha aplicación externa',
'edit-externally-help' => 'Vexa as seguintes [http://www.mediawiki.org/wiki/Manual:External_editors instrucións] <small>(en inglés)</small> para máis información.',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'todos',
'imagelistall'     => 'todas',
'watchlistall2'    => 'todo',
'namespacesall'    => 'todos',
'monthsall'        => 'todos',

# E-mail address confirmation
'confirmemail'             => 'Confirmar o enderezo de correo electrónico',
'confirmemail_noemail'     => 'Non ten rexistrado ningún enderezo de correo electrónico válido nas súas [[Special:Preferences|preferencias de usuario]].',
'confirmemail_text'        => '{{SITENAME}} require que lle dea validez ao seu enderezo de correo electrónico antes de utilizar as funcións relacionadas con el. Prema no botón de embaixo para enviar un correo de confirmación ao seu enderezo. O correo incluirá unha ligazón cun código: faga clic nesta ligazón para abrila no seu navegador web e así confirmar que o seu enderezo é válido.',
'confirmemail_pending'     => '<div class="error"> Envióuselle un código de confirmación ao enderezo de correo electrónico; se creou a conta hai pouco debe esperar uns minutos antes de solicitar un novo código.</div>',
'confirmemail_send'        => 'Enviar por correo elecrónico un código de confirmación',
'confirmemail_sent'        => 'Correo electrónico de confirmación enviado.',
'confirmemail_oncreate'    => 'Envióuselle un código de confirmación ao enderezo de correo electrónico. Este código non é imprescindible para entrar no wiki, pero é preciso para activar as funcións do wiki baseadas no correo.',
'confirmemail_sendfailed'  => '{{SITENAME}} non puido enviar a mensaxe de confirmación do correo.
Por favor, comprobe que no enderezo de correo electrónico non haxa caracteres inválidos.

O programa de correo informa do seguinte: $1',
'confirmemail_invalid'     => 'O código de confirmación non é válido.
Pode ser que caducase.',
'confirmemail_needlogin'   => 'Necesita $1 para confirmar o seu enderezo de correo electrónico.',
'confirmemail_success'     => 'Confirmouse o seu enderezo de correo electrónico. Agora xa pode [[Special:UserLogin|acceder ao sistema]] e facer uso do wiki.',
'confirmemail_loggedin'    => 'Xa se confirmou o seu enderezo de correo electrónico.',
'confirmemail_error'       => 'Houbo un problema ao gardar a súa confirmación.',
'confirmemail_subject'     => '{{SITENAME}} - Verificación do enderezo de correo electrónico',
'confirmemail_body'        => 'Alguén, probablemente vostede, desde o enderezo IP $1,
rexistrou a conta "$2" con este enderezo de correo electrónico en {{SITENAME}}.

Para confirmar que esta conta realmente lle pertence e así poder activar
as funcións de correo electrónico en {{SITENAME}}, abra esta ligazón no seu navegador:

$3

Se *non* rexistrou a conta siga estoutra ligazón
para cancelar a confirmación do enderezo de correo electrónico:

$5

Este código de confirmación caducará ás $4.',
'confirmemail_invalidated' => 'A confirmación do enderezo de correo electrónico foi cancelada',
'invalidateemail'          => 'Cancelar a confirmación do correo electrónico',

# Scary transclusion
'scarytranscludedisabled' => '[A transclusión interwiki está desactivada]',
'scarytranscludefailed'   => '[Fallou a busca do modelo "$1"]',
'scarytranscludetoolong'  => '[O enderezo URL é demasiado longo]',

# Trackbacks
'trackbackbox'      => '<div id="mw_trackbacks">
Trackbacks para este artigo:<br />
$1
</div>',
'trackbackremove'   => '  ([$1 Borrar])',
'trackbacklink'     => 'Trackback',
'trackbackdeleteok' => 'Eliminouse o trackback sen problemas.',

# Delete conflict
'deletedwhileediting' => "'''Aviso:''' esta páxina foi borrada despois de que comezase a editala!",
'confirmrecreate'     => "O usuario [[User:$1|$1]] ([[User talk:$1|disc.]]) borrou este artigo despois de que vostede comezara a editalo, polo seguinte motivo:
: ''$2'' 
Por favor confirme que realmente quere crear o artigo de novo.",
'recreate'            => 'Recrear',

# HTML dump
'redirectingto' => 'Redirixindo cara a "[[$1]]"...',

# action=purge
'confirm_purge'        => 'Está seguro de que desexa limpar a memoria caché desta páxina?

$1',
'confirm_purge_button' => 'Si',

# AJAX search
'searchcontaining' => "Procurar artigos que conteñan ''$1''.",
'searchnamed'      => "Buscar artigos chamados ''$1''.",
'articletitles'    => "Artigos que comezan por ''$1''",
'hideresults'      => 'Agochar resultados',
'useajaxsearch'    => 'Usar a procura AJAX',

# Multipage image navigation
'imgmultipageprev' => '← páxina anterior',
'imgmultipagenext' => 'seguinte páxina →',
'imgmultigo'       => 'Ir!',
'imgmultigoto'     => 'Ir á páxina $1',

# Table pager
'ascending_abbrev'         => 'asc',
'descending_abbrev'        => 'desc',
'table_pager_next'         => 'Páxina seguinte',
'table_pager_prev'         => 'Páxina anterior',
'table_pager_first'        => 'Primeira páxina',
'table_pager_last'         => 'Última páxina',
'table_pager_limit'        => 'Mostrar $1 ítems por páxina',
'table_pager_limit_submit' => 'Ir',
'table_pager_empty'        => 'Sen resultados',

# Auto-summaries
'autosumm-blank'   => 'O contido da páxina foi eliminado',
'autosumm-replace' => 'O contido da páxina foi substituído por "$1"',
'autoredircomment' => 'Redirixida cara a "[[$1]]"',
'autosumm-new'     => 'Nova páxina: $1',

# Live preview
'livepreview-loading' => 'Cargando…',
'livepreview-ready'   => 'Cargando… Listo!',
'livepreview-failed'  => 'Fallou a vista previa en tempo real!
Tente a vista previa normal.',
'livepreview-error'   => 'Fallou a conexión: $1 "$2"
Tente a vista previa normal.',

# Friendlier slave lag warnings
'lag-warn-normal' => 'Pode que os cambios feitos {{PLURAL:$1|no último segundo|nos últimos $1 segundos}} non aparezan nesta lista.',
'lag-warn-high'   => 'Debido a unha gran demora do servidor da base de datos, pode que nesta lista non aparezan os cambios feitos {{PLURAL:$1|no último segundo|nos últimos $1 segundos}}.',

# Watchlist editor
'watchlistedit-numitems'       => 'A súa listaxe de vixilancia inclúe {{PLURAL:$1|un título|$1 títulos}}, excluíndo as páxinas de conversa.',
'watchlistedit-noitems'        => 'A súa listaxe de vixilancia non contén ningún título.',
'watchlistedit-normal-title'   => 'Editar a listaxe de vixilancia',
'watchlistedit-normal-legend'  => 'Eliminar títulos da listaxe de vixilancia',
'watchlistedit-normal-explain' => 'Os títulos da súa listaxe de vixilancia aparecen embaixo.
Para eliminar un título, escóllao na súa caixa de selección e prema en "Eliminar os títulos".
Tamén pode [[Special:Watchlist/raw|editar a listaxe simple]].',
'watchlistedit-normal-submit'  => 'Eliminar os títulos',
'watchlistedit-normal-done'    => '{{PLURAL:$1|Eliminouse un título|Elimináronse $1 títulos}} da súa listaxe de vixilancia:',
'watchlistedit-raw-title'      => 'Editar a listaxe de vixilancia simple',
'watchlistedit-raw-legend'     => 'Editar a listaxe de vixilancia simple',
'watchlistedit-raw-explain'    => 'Os títulos da súa listaxe de vixilancia aparecen embaixo e pódense editar engadíndoos ou retirándoos da listaxe; un título por liña.
Ao rematar, prema en "Actualizar a listaxe de vixilancia".
Tamén pode [[Special:Watchlist/edit|empregar o editor normal]].',
'watchlistedit-raw-titles'     => 'Títulos:',
'watchlistedit-raw-submit'     => 'Actualizar a listaxe de vixilancia',
'watchlistedit-raw-done'       => 'Actualizouse a súa listaxe de vixilancia.',
'watchlistedit-raw-added'      => '{{PLURAL:$1|Engadiuse un título|Engadíronse $1 títulos}}:',
'watchlistedit-raw-removed'    => '{{PLURAL:$1|Eliminouse un título|Elimináronse $1 títulos}}:',

# Watchlist editing tools
'watchlisttools-view' => 'Ver as modificacións relevantes',
'watchlisttools-edit' => 'Ver e editar a listaxe de vixilancia',
'watchlisttools-raw'  => 'Editar a listaxe de vixilancia simple',

# Core parser functions
'unknown_extension_tag' => 'Etiqueta de extensión descoñecida "$1"',

# Special:Version
'version'                          => 'Versión', # Not used as normal message but as header for the special page itself
'version-extensions'               => 'Extensións instaladas',
'version-specialpages'             => 'Páxinas especiais',
'version-parserhooks'              => 'Hooks do analizador (parser)',
'version-variables'                => 'Variábeis',
'version-other'                    => 'Outro',
'version-mediahandlers'            => 'Executadores de multimedia',
'version-hooks'                    => 'Hooks',
'version-extension-functions'      => 'Funcións das extensións',
'version-parser-extensiontags'     => 'Etiquetas das extensións do analizador (parser)',
'version-parser-function-hooks'    => 'Hooks da función do analizador',
'version-skin-extension-functions' => 'Funcións da extensión da aparencia',
'version-hook-name'                => 'Nome do hook',
'version-hook-subscribedby'        => 'Subscrito por',
'version-version'                  => 'Versión',
'version-license'                  => 'Licenza',
'version-software'                 => 'Software instalado',
'version-software-product'         => 'Produto',
'version-software-version'         => 'Versión',

# Special:FilePath
'filepath'         => 'Ruta do ficheiro',
'filepath-page'    => 'Ficheiro:',
'filepath-submit'  => 'Ruta',
'filepath-summary' => 'Esta páxina especial devolve a ruta completa a un ficheiro.
As imaxes móstranse na súa resolución completa; outros tipos de ficheiros inícianse directamente co seu programa asociado.

Introduza o nome do ficheiro sen o prefixo "{{ns:image}}:"',

# Special:FileDuplicateSearch
'fileduplicatesearch'          => 'Procurar ficheiros duplicados',
'fileduplicatesearch-summary'  => 'Procurar ficheiros duplicados a partir do valor de <i>hash</i> (un mecanismo de comprobación).

Introduza o nome do ficheiro sen o prefixo "{{ns:image}}:".',
'fileduplicatesearch-legend'   => 'Procurar un duplicado',
'fileduplicatesearch-filename' => 'Nome do ficheiro:',
'fileduplicatesearch-submit'   => 'Procurar',
'fileduplicatesearch-info'     => '$1 × $2 píxeles<br />Tamaño do ficheiro: $3<br />Tipo MIME: $4',
'fileduplicatesearch-result-1' => 'O ficheiro "$1" non ten un duplicado idéntico.',
'fileduplicatesearch-result-n' => 'O ficheiro "$1" ten {{PLURAL:$2|1 duplicado idéntico|$2 duplicados idénticos}}.',

# Special:SpecialPages
'specialpages'                   => 'Páxinas especiais',
'specialpages-note'              => '----
* Páxinas especiais normais.
* <span class="mw-specialpagerestricted">Páxinas especiais restrinxidas.</span>',
'specialpages-group-maintenance' => 'Informes de mantemento',
'specialpages-group-other'       => 'Outras páxinas especiais',
'specialpages-group-login'       => 'Rexistro',
'specialpages-group-changes'     => 'Cambios recentes e rexistros',
'specialpages-group-media'       => 'Informes multimedia e cargas',
'specialpages-group-users'       => 'Usuarios e dereitos',
'specialpages-group-highuse'     => 'Páxinas con máis uso',
'specialpages-group-pages'       => 'Listas de páxinas',
'specialpages-group-pagetools'   => 'Ferramentas das páxinas',
'specialpages-group-wiki'        => 'Datos do wiki e ferramentas',
'specialpages-group-redirects'   => 'Páxinas de redirección especiais',
'specialpages-group-spam'        => "Ferramentas contra o ''spam''",

# Special:BlankPage
'blankpage'              => 'Baleirar a páxina',
'intentionallyblankpage' => 'Esta páxina foi baleirada intencionadamente',

);
