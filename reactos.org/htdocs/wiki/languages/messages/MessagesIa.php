<?php
/** Interlingua (Interlingua)
 *
 * @ingroup Language
 * @file
 *
 * @author Julian Mendez
 * @author Malafaya
 * @author McDutchie
 * @author לערי ריינהארט
 */

$skinNames = array(
	'cologneblue' => 'Blau Colonia',
);

$namespaceNames = array(
	NS_MEDIA          => 'Media',
	NS_SPECIAL        => 'Special',
	NS_MAIN           => '',
	NS_TALK           => 'Discussion',
	NS_USER           => 'Usator',
	NS_USER_TALK      => 'Discussion_Usator',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK   => 'Discussion_$1',
	NS_IMAGE          => 'Imagine',
	NS_IMAGE_TALK     => 'Discussion_Imagine',
	NS_MEDIAWIKI      => 'MediaWiki',
	NS_MEDIAWIKI_TALK => 'Discussion_MediaWiki',
	NS_TEMPLATE       => 'Patrono',
	NS_TEMPLATE_TALK  => 'Discussion_Patrono',
	NS_HELP           => 'Adjuta',
	NS_HELP_TALK      => 'Discussion_Adjuta',
	NS_CATEGORY       => 'Categoria',
	NS_CATEGORY_TALK  => 'Discussion_Categoria'
);
$linkTrail = "/^([a-z]+)(.*)\$/sD";

$messages = array(
# User preference toggles
'tog-underline'               => 'Sublinear ligamines:',
'tog-highlightbroken'         => 'Formatar ligamines rupte <a href="" class="new">assi</a> (alternativemente: assi<a href="" class="internal">?</a>).',
'tog-justify'                 => 'Justificar paragraphos',
'tog-hideminor'               => 'Celar modificationes recente minor',
'tog-extendwatchlist'         => 'Expander le observatorio a tote le modificationes applicabile',
'tog-usenewrc'                => 'Modificationes recente meliorate (JavaScript)',
'tog-numberheadings'          => 'Numerar titulos automaticamente',
'tog-showtoolbar'             => 'Monstrar barra de instrumentos pro modification (JavaScript)',
'tog-editondblclick'          => 'Duple clic pro modificar un pagina (JavaScript)',
'tog-editsection'             => 'Activar le modification de sectiones con ligamines [modificar]',
'tog-editsectiononrightclick' => 'Activar modification de sectiones con clic-a-derecta super lor titulos (JavaScript)',
'tog-showtoc'                 => 'Monstrar tabula de contento (in paginas con plus de 3 sectiones)',
'tog-rememberpassword'        => 'Memorar mi contrasigno in iste computator',
'tog-editwidth'               => 'Le quadro de modification occupa tote le latitude del fenestra',
'tog-watchcreations'          => 'Adder le paginas que io crea a mi observatorio',
'tog-watchdefault'            => 'Adder le paginas que io modifica a mi observatorio',
'tog-watchmoves'              => 'Adder le paginas que io renomina a mi observatorio',
'tog-watchdeletion'           => 'Adder le paginas que io dele a mi observatorio',
'tog-minordefault'            => 'Marcar omne modificationes initialmente como minor',
'tog-previewontop'            => 'Monstrar previsualisation ante le quadro de modification',
'tog-previewonfirst'          => 'Monstrar previsualisation al prime modification',
'tog-nocache'                 => "Disactivar le ''cache'' de paginas",
'tog-enotifwatchlistpages'    => 'Notificar me via e-mail quando se cambia un pagina in mi observatorio',
'tog-enotifusertalkpages'     => 'Notificar me via e-mail quando se cambia mi pagina de discussion',
'tog-enotifminoredits'        => 'Notificar me etiam de modificationes minor',
'tog-enotifrevealaddr'        => 'Revelar mi adresse de e-mail in messages de notification',
'tog-shownumberswatching'     => 'Monstrar le numero de usatores que observa le pagina',
'tog-fancysig'                => 'Signaturas crude (sin ligamine automatic)',
'tog-externaleditor'          => 'Usar editor externe qua standard (pro expertos solmente, necessita configuration special in tu computator)',
'tog-externaldiff'            => "Usar un programma ''diff'' externe qua standard (pro expertos solmente, necessita configuration special in tu computator)",
'tog-showjumplinks'           => 'Activar ligamines de accessibilitate "saltar a"',
'tog-uselivepreview'          => 'Usar previsualisation directe (JavaScript) (Experimental)',
'tog-forceeditsummary'        => 'Prevenir me quando io entra un summario de modification vacue',
'tog-watchlisthideown'        => 'Excluder mi proprie modificationes del observatorio',
'tog-watchlisthidebots'       => 'Excluder le modificationes per bots del observatorio',
'tog-watchlisthideminor'      => 'Excluder le modificationes minor del observatorio',
'tog-nolangconversion'        => 'Disactivar conversion de variantes',
'tog-ccmeonemails'            => 'Inviar me copias del messages de e-mail que io invia a altere usatores',
'tog-diffonly'                => 'Non monstrar le contento del pagina sub le comparation de duo versiones',
'tog-showhiddencats'          => 'Monstrar categorias celate',

'underline-always'  => 'Sempre',
'underline-never'   => 'Nunquam',
'underline-default' => 'Secundo le configuration del navigator',

'skinpreview' => '(Previsualisation)',

# Dates
'sunday'        => 'dominica',
'monday'        => 'lunedi',
'tuesday'       => 'martedi',
'wednesday'     => 'mercuridi',
'thursday'      => 'jovedi',
'friday'        => 'venerdi',
'saturday'      => 'sabbato',
'sun'           => 'dom',
'mon'           => 'lun',
'tue'           => 'mar',
'wed'           => 'mer',
'thu'           => 'jov',
'fri'           => 'ven',
'sat'           => 'sab',
'january'       => 'januario',
'february'      => 'februario',
'march'         => 'martio',
'april'         => 'april',
'may_long'      => 'maio',
'june'          => 'junio',
'july'          => 'julio',
'august'        => 'augusto',
'september'     => 'septembre',
'october'       => 'octobre',
'november'      => 'novembre',
'december'      => 'decembre',
'january-gen'   => 'januario',
'february-gen'  => 'februario',
'march-gen'     => 'martio',
'april-gen'     => 'april',
'may-gen'       => 'maio',
'june-gen'      => 'junio',
'july-gen'      => 'julio',
'august-gen'    => 'augusto',
'september-gen' => 'septembre',
'october-gen'   => 'octobre',
'november-gen'  => 'novembre',
'december-gen'  => 'decembre',
'jan'           => 'jan',
'feb'           => 'feb',
'mar'           => 'mar',
'apr'           => 'apr',
'may'           => 'mai',
'jun'           => 'jun',
'jul'           => 'jul',
'aug'           => 'aug',
'sep'           => 'sep',
'oct'           => 'oct',
'nov'           => 'nov',
'dec'           => 'dec',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|Categoria|Categorias}}',
'category_header'                => 'Articulos in le categoria "$1"',
'subcategories'                  => 'Subcategorias',
'category-media-header'          => 'Media in categoria "$1"',
'category-empty'                 => "''Iste categoria non contine alcun paginas o media al momento.''",
'hidden-categories'              => '{{PLURAL:$1|Categoria celate|Categorias celate}}',
'hidden-category-category'       => 'Categorias celate', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|Iste categoria ha solmente le sequente subcategoria.|Iste categoria ha le sequente {{PLURAL:$1|subcategoria|$1 subcategorias}}, ex $2 in total.}}',
'category-subcat-count-limited'  => 'Iste categoria ha le sequente {{PLURAL:$1|subcategoria|$1 subcategorias}}.',
'category-article-count'         => '{{PLURAL:$2|Iste categoria contine solmente le sequente pagina.|Le sequente {{PLURAL:$1|pagina es|$1 paginas es}} in iste categora, ex $2 in total.}}',
'category-article-count-limited' => 'Le sequente {{PLURAL:$1|pagina es|$1 paginas es}} in le categoria actual.',
'category-file-count'            => '{{PLURAL:$2|Iste categoria contine solmente le sequente file.|Le sequente {{PLURAL:$1|file es|$1 files es}} in iste categoria, ex $2 in total.}}',
'category-file-count-limited'    => 'Le sequente {{PLURAL:$1|file es|$1 files es}} in le categoria actual.',
'listingcontinuesabbrev'         => 'cont.',

'mainpagetext'      => "<big>'''MediaWiki ha essite installate con successo.'''</big>",
'mainpagedocfooter' => 'Consulta le [http://meta.wikimedia.org/wiki/Help:Contents Guida del usator] pro informationes super le uso del software wiki.

== Pro initiar ==
* [http://www.mediawiki.org/wiki/Manual:Configuration_settings Lista de configurationes]
* [http://www.mediawiki.org/wiki/Manual:FAQ FAQ a proposito de MediaWiki]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce Lista de diffusion pro annuncios de nove versiones de MediaWiki]',

'about'          => 'A proposito',
'article'        => 'Pagina de contento',
'newwindow'      => '(se aperi in un nove fenestra)',
'cancel'         => 'Cancellar',
'qbfind'         => 'Cercar',
'qbbrowse'       => 'Foliar',
'qbedit'         => 'Modificar',
'qbpageoptions'  => 'Iste pagina',
'qbpageinfo'     => 'Contexto',
'qbmyoptions'    => 'Mi paginas',
'qbspecialpages' => 'Paginas special',
'moredotdotdot'  => 'Plus...',
'mypage'         => 'Mi pagina',
'mytalk'         => 'Mi discussion',
'anontalk'       => 'Discussion pro iste adresse IP',
'navigation'     => 'Navigation',
'and'            => 'e',

# Metadata in edit box
'metadata_help' => 'Metadatos:',

'errorpagetitle'    => 'Error',
'returnto'          => 'Retornar a $1.',
'tagline'           => 'De {{SITENAME}}',
'help'              => 'Adjuta',
'search'            => 'Recerca',
'searchbutton'      => 'Cercar',
'go'                => 'Ir',
'searcharticle'     => 'Ir',
'history'           => 'Historia del pagina',
'history_short'     => 'Historia',
'updatedmarker'     => 'actualisate post mi ultime visita',
'info_short'        => 'Information',
'printableversion'  => 'Version imprimibile',
'permalink'         => 'Ligamine permanente',
'print'             => 'Imprimer',
'edit'              => 'Modificar',
'create'            => 'Crear',
'editthispage'      => 'Modificar iste pagina',
'create-this-page'  => 'Crear iste pagina',
'delete'            => 'Deler',
'deletethispage'    => 'Deler iste pagina',
'undelete_short'    => 'Restaurar {{PLURAL:$1|un modification|$1 modificationes}}',
'protect'           => 'Proteger',
'protect_change'    => 'cambiar',
'protectthispage'   => 'Proteger iste pagina',
'unprotect'         => 'Disproteger',
'unprotectthispage' => 'Disproteger iste pagina',
'newpage'           => 'Nove pagina',
'talkpage'          => 'Discuter iste pagina',
'talkpagelinktext'  => 'Discussion',
'specialpage'       => 'Pagina special',
'personaltools'     => 'Instrumentos personal',
'postcomment'       => 'Publicar un commento',
'articlepage'       => 'Vider pagina de contento',
'talk'              => 'Discussion',
'views'             => 'Visitas',
'toolbox'           => 'Instrumentario',
'userpage'          => 'Vider pagina del usator',
'projectpage'       => 'Vider pagina de projecto',
'imagepage'         => 'Vider pagina de media',
'mediawikipage'     => 'Vider pagina de message',
'templatepage'      => 'Vider pagina de patrono',
'viewhelppage'      => 'Vider pagina de adjuta',
'categorypage'      => 'Vider pagina de categoria',
'viewtalkpage'      => 'Vider discussion',
'otherlanguages'    => 'In altere linguas',
'redirectedfrom'    => '(Redirigite ab $1)',
'redirectpagesub'   => 'Pagina de redirection',
'lastmodifiedat'    => 'Ultime modification de iste pagina: le $1 a $2.', # $1 date, $2 time
'viewcount'         => 'Iste pagina ha essite visitate {{PLURAL:$1|un vice|$1 vices}}.',
'protectedpage'     => 'Pagina protegite',
'jumpto'            => 'Saltar a:',
'jumptonavigation'  => 'navigation',
'jumptosearch'      => 'cercar',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'A proposito de {{SITENAME}}',
'aboutpage'            => 'Project:A proposito',
'bugreports'           => 'Reportos de disfunctiones',
'bugreportspage'       => 'Project:Reportos de disfunctiones',
'copyright'            => 'Le contento es disponibile sub $1.',
'copyrightpagename'    => 'Copyright de {{SITENAME}}',
'copyrightpage'        => '{{ns:project}}:Copyright',
'currentevents'        => 'Actualitates',
'currentevents-url'    => 'Project:Actualitates',
'disclaimers'          => 'Declarationes de non-responsabilitate',
'disclaimerpage'       => 'Project:Declaration general de non-responsabilitate',
'edithelp'             => 'Adjuta de modification',
'edithelppage'         => 'Help:Modification',
'faq'                  => 'FAQ',
'faqpage'              => 'Project:FAQ',
'helppage'             => 'Help:Contento',
'mainpage'             => 'Pagina principal',
'mainpage-description' => 'Pagina principal',
'policy-url'           => 'Project:Politica',
'portal'               => 'Portal del communitate',
'portal-url'           => 'Project:Portal del communitate',
'privacy'              => 'Politica de confidentialitate',
'privacypage'          => 'Project:Politica de confidentialitate',

'badaccess'        => 'Error de permission',
'badaccess-group0' => 'Tu non ha le permission de executar le action que tu ha requestate.',
'badaccess-group1' => 'Le action que tu ha requestate es limitate al usatores in le gruppo $1.',
'badaccess-group2' => 'Le action que tu ha requestate es limitate al usatores in un del gruppos $1.',
'badaccess-groups' => 'Le action que tu ha requestate es limitate al usatores in un del gruppos $1.',

'versionrequired'     => 'Version $1 de MediaWiki requirite',
'versionrequiredtext' => 'Le version $1 de MediaWiki es requirite pro usar iste pagina. Vide [[Special:Version|le pagina de version]].',

'ok'                      => 'OK',
'retrievedfrom'           => 'Recuperate de "$1"',
'youhavenewmessages'      => 'Tu ha $1 ($2).',
'newmessageslink'         => 'nove messages',
'newmessagesdifflink'     => 'ultime modification',
'youhavenewmessagesmulti' => 'Tu ha nove messages in $1',
'editsection'             => 'modificar',
'editold'                 => 'modificar',
'viewsourceold'           => 'vider codice-fonte',
'editsectionhint'         => 'Modificar section: $1',
'toc'                     => 'Contento',
'showtoc'                 => 'revelar',
'hidetoc'                 => 'celar',
'thisisdeleted'           => 'Vider o restaurar $1?',
'viewdeleted'             => 'Vider $1?',
'restorelink'             => '{{PLURAL:$1|un modification|$1 modificationes}} delite',
'feedlinks'               => 'Syndication:',
'feed-invalid'            => 'Typo de syndication invalide.',
'feed-unavailable'        => 'Le syndicationes non es disponibile',
'site-rss-feed'           => 'Syndication RSS de $1',
'site-atom-feed'          => 'Syndication Atom de $1',
'page-rss-feed'           => 'Syndication RSS de "$1"',
'page-atom-feed'          => 'Syndication Atom de "$1"',
'red-link-title'          => '$1 (non ancora scribite)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Pagina',
'nstab-user'      => 'Pagina de usator',
'nstab-media'     => 'Pagina de media',
'nstab-special'   => 'Special',
'nstab-project'   => 'Pagina de projecto',
'nstab-image'     => 'File',
'nstab-mediawiki' => 'Message',
'nstab-template'  => 'Patrono',
'nstab-help'      => 'Pagina de adjuta',
'nstab-category'  => 'Categoria',

# Main script and global functions
'nosuchaction'      => 'Non existe tal action',
'nosuchactiontext'  => 'Le action specificate in le adresse URL non es recognoscite per le wiki',
'nosuchspecialpage' => 'Pagina special invalide',
'nospecialpagetext' => "<big>'''Tu ha requestate un pagina special que es non es valide.'''</big>

Un lista de paginas special valide se trova a [[Special:SpecialPages|{{int:specialpages}}]].",

# General errors
'error'                => 'Error',
'databaseerror'        => 'Error de base de datos',
'dberrortext'          => 'Un error de syntaxe occurreva durante un consulta del base de datos.
Isto poterea indicar le presentia de un error in le software.
Le ultime consulta que se tentava es:
<blockquote><tt>$1</tt></blockquote>
effectuate per le function "<tt>$2</tt>".
MySQL retornava le error "<tt>$3: $4</tt>".',
'dberrortextcl'        => 'Un error de syntaxe occurreva durante un consulta del base de datos.
Le ultime consulta que se tentava es:
"$1"
effectuate per le function "$2".
MySQL retornava le error "$3: $4"',
'noconnect'            => 'Le wiki ha difficultates technic al momento, e non pote contactar le servitor del base de datos.<br />
$1',
'nodb'                 => 'Non poteva seliger le base de datos $1',
'cachederror'          => 'Le sequente copia del pagina se recuperava del cache, e possibilemente non es actual.',
'laggedslavemode'      => 'Attention: Es possibile que le pagina non contine actualisationes recente.',
'readonly'             => 'Base de datos blocate',
'enterlockreason'      => 'Describe le motivo del blocada, includente un estimation
de quando illo essera terminate',
'readonlytext'         => 'Al momento, le base de datos es blocate contra nove entratas e altere modificationes, probabilemente pro mantenentia routinari del base de datos, post le qual illo retornara al normal.

Le administrator responsabile dava iste explication: $1',
'missing-article'      => 'Le base de datos non ha trovate le texto de un pagina que illo deberea haber trovate, nominate "$1" $2.

Causas normal de iste problema es: tu ha consultate un \'\'diff\'\' obsolete, o tu sequeva un ligamine de historia verso un pagina que ha essite delite.

Si isto non es le caso, es possibile que tu ha trovate un error in le software.
Per favor reporta isto a un [[Special:ListUsers/sysop|administrator]], faciente nota del adresse URL.',
'missingarticle-rev'   => '(numero del revision: $1)',
'missingarticle-diff'  => '(Diff: $1, $2)',
'readonly_lag'         => 'Le base de datos ha essite automaticamente blocate durante que le servitores de base de datos secundari se synchronisa con le servitor principal.',
'internalerror'        => 'Error interne',
'internalerror_info'   => 'Error interne: $1',
'filecopyerror'        => 'Impossibile copiar file "$1" a "$2".',
'filerenameerror'      => 'Impossibile renominar file "$1" a "$2".',
'filedeleteerror'      => 'Impossibile deler file "$1".',
'directorycreateerror' => 'Impossibile crear le directorio "$1".',
'filenotfound'         => 'Impossibile trovar file "$1".',
'fileexistserror'      => 'Impossibile scriber in le file "$1": le file ja existe',
'unexpected'           => 'Valor impreviste: "$1"="$2".',
'formerror'            => 'Error: impossibile submitter formulario',
'badarticleerror'      => 'Iste action non pote esser effectuate super iste pagina.',
'cannotdelete'         => 'Impossibile deler le pagina o file specificate.
Es possibile que un altere persona ha ja delite lo.',
'badtitle'             => 'Titulo incorrecte',
'badtitletext'         => 'Le titulo de pagina demandate esseva invalide, vacue, o constitueva un ligamine interlingual o interwiki incorrecte.
Es possibile que illo contine un o plure characteres que non pote esser usate in titulos.',
'perfdisabled'         => 'Pardono! Iste functionalitate ha essite temporarimente disactivate proque illo rende le operation del base de datos tanto lente que necuno pote usar le wiki.',
'perfcached'           => 'Le sequente datos se recuperava del cache e possibilemente non es actual.',
'perfcachedts'         => 'Le sequente datos se recuperava del cache. Ultime actualisation: le $1.',
'querypage-no-updates' => 'Le actualisationes pro iste pagina es disactivate. Pro le momento, le datos hic non se cambiara.',
'wrong_wfQuery_params' => 'Parametros incorrecte a wfQuery()<br />
Function: $1<br />
Consulta: $2',
'viewsource'           => 'Vider codice-fonte',
'viewsourcefor'        => 'de $1',
'actionthrottled'      => 'Action limitate',
'actionthrottledtext'  => 'Como mesura anti-spam, tu es limitate de executar iste action troppo de vices durante un curte periodo de tempore, e tu ha excedite iste limite.
Per favor reprova post alcun minutas.',
'protectedpagetext'    => 'Iste pagina ha essite protegite contra modificationes.',
'viewsourcetext'       => 'Tu pote vider e copiar le codice-fonte de iste pagina:',
'protectedinterface'   => 'Iste pagina contine texto pro le interfacie del software, e es protegite pro impedir le abuso.',
'editinginterface'     => "'''Attention:''' Tu va modificar un pagina que se usa pro texto del interfacie pro le software.
Omne modification a iste pagina cambiara le apparentia del interfacie pro altere usatores.
Pro traductiones, per favor considera usar [http://translatewiki.net/wiki/Main_Page?setlang=ia Betawiki], le projecto pro localisar MediaWiki.",
'sqlhidden'            => '(Consulta SQL celate)',
'cascadeprotected'     => 'Iste pagina ha essite protegite contra modificationes, proque illo es includite in le sequente {{PLURAL:$1|pagina, le qual|paginas, le quales}} es protegite usante le option "cascada":
$2',
'namespaceprotected'   => "Tu non ha le permission de modificar paginas in le spatio de nomines '''$1'''.",
'customcssjsprotected' => 'Tu non ha le permission de modificar iste pagina, proque illo contine le configurationes personal de un altere usator.',
'ns-specialprotected'  => 'Le paginas special non es modificabile.',
'titleprotected'       => "Iste titulo ha essite protegite contra creation per [[User:$1|$1]].
Le ration date es ''$2''.",

# Virus scanner
'virus-badscanner'     => 'Configuration incorrecte: programma antivirus non cognoscite: <i>$1</i>',
'virus-scanfailed'     => 'scansion fallite (codice $1)',
'virus-unknownscanner' => 'antivirus non cognoscite:',

# Login and logout pages
'logouttitle'                => 'Session claudite',
'logouttext'                 => '<strong>Tu ha claudite tu session.</strong>

Tu pote continuar a usar {{SITENAME}} anonymemente, o tu pote [[Special:UserLogin|initiar un nove session]] como le mesme o como un altere usator.
Nota que alcun paginas pote continuar a monstrar se como si le session esserea ancora active. Pro remediar isto, tu pote vacuar le cache de tu navigator.',
'welcomecreation'            => '== Benvenite, $1! ==
Tu conto ha essite create.
Non oblida personalisar tu [[Special:Preferences|preferentias in {{SITENAME}}]].',
'loginpagetitle'             => 'Aperir session',
'yourname'                   => 'Nomine de usator:',
'yourpassword'               => 'Contrasigno:',
'yourpasswordagain'          => 'Repete contrasigno:',
'remembermypassword'         => 'Memorar mi contrasigno in iste computator',
'yourdomainname'             => 'Tu dominio:',
'externaldberror'            => 'O il occureva un error in le base de datos de authentification externe, o tu non ha le autorisation de actualisar tu conto externe.',
'loginproblem'               => '<b>Un problema occureva con tu session.</b><br />Per favor reprova!',
'login'                      => 'Aperir session',
'nav-login-createaccount'    => 'Aperir session / crear conto',
'loginprompt'                => 'Tu debe haber activate le cookies pro poter identificar te a {{SITENAME}}.',
'userlogin'                  => 'Aperir session / crear conto',
'logout'                     => 'Clauder session',
'userlogout'                 => 'Clauder session',
'notloggedin'                => 'Tu non ha aperite un session',
'nologin'                    => 'Tu non ha un conto? $1.',
'nologinlink'                => 'Crear un conto',
'createaccount'              => 'Crear nove conto',
'gotaccount'                 => 'Tu jam ha un conto? $1.',
'gotaccountlink'             => 'Aperir un session',
'createaccountmail'          => 'per e-mail',
'badretype'                  => 'Le duo contrasignos que tu scribeva non es identic.',
'userexists'                 => 'Le nomine de usator que tu entrava es ja in uso.
Selige un altere nomine.',
'youremail'                  => 'E-mail:',
'username'                   => 'Nomine de usator:',
'uid'                        => 'ID del usator:',
'prefs-memberingroups'       => 'Membro de {{PLURAL:$1|gruppo|gruppos}}:',
'yourrealname'               => 'Nomine real:',
'yourlanguage'               => 'Lingua:',
'yourvariant'                => 'Variante:',
'yournick'                   => 'Signatura:',
'badsig'                     => 'Signatura crude invalide; verificar le etiquettas HTML.',
'badsiglength'               => 'Le signatura es troppo longe.
Illo debe haber minus de $1 {{PLURAL:$1|character|characteres}}.',
'email'                      => 'E-mail',
'prefs-help-realname'        => 'Le nomine real es optional.
Si tu opta pro dar lo, isto essera usate pro dar te attribution pro tu contributiones.',
'loginerror'                 => 'Error in le apertura del session',
'prefs-help-email'           => 'Le adresse de e-mail es optional, sed permitte facer inviar te tu contrasigno in caso que tu lo oblida. Tu pote etiam optar pro permitter que altere personas te contacta via tu pagina de usator o de discussion, sin necessitate de revelar tu identitate.',
'prefs-help-email-required'  => 'Le adresse de e-mail es requirite.',
'nocookiesnew'               => "Le conto de usator ha essite create, sed tu non ha aperite un session.
{{SITENAME}} usa ''cookies'' pro mantener le sessiones del usatores.
Tu ha disactivate le functionalitate del ''cookies''.
Per favor activa lo, postea aperi un session con tu nove nomine de usator e contrasigno.",
'nocookieslogin'             => "{{SITENAME}} usa ''cookies'' pro mantener le sessiones del usatores.
Tu ha disactivate le functionalitate del ''cookies''.
Per favor activa lo e reprova.",
'noname'                     => 'Tu non specificava un nomine de usator valide.',
'loginsuccesstitle'          => 'Session aperite con successo',
'loginsuccess'               => "'''Tu es ora identificate in {{SITENAME}} como \"\$1\".'''",
'nosuchuser'                 => 'Non existe un usator con le nomine "$1".
Verifica le orthographia, o [[Special:Userlogin/signup|crea un nove conto]].',
'nosuchusershort'            => 'Non existe un usator con le nomine "<nowiki>$1</nowiki>".
Verifica le orthographia.',
'nouserspecified'            => 'Tu debe specificar un nomine de usator.',
'wrongpassword'              => 'Le contrasigno que tu entrava es incorrecte. Per favor reprova.',
'wrongpasswordempty'         => 'Tu non entrava un contrasigno. Per favor reprova.',
'passwordtooshort'           => 'Tu contrasigno es invalide o troppo curte.
Illo debe haber al minus {{PLURAL:$1|1 character|$1 characteres}} e debe differer de tu nomine de usator.',
'mailmypassword'             => 'Inviar un nove contrasigno in e-mail',
'passwordremindertitle'      => 'Nove contrasigno temporari pro {{SITENAME}}',
'passwordremindertext'       => 'Alcuno (probabilemente tu, ab le adresse IP $1) requestava un nove
contrasigno pro {{SITENAME}} ($4). Un contrasigno temporari pro le usator
"$2" ha essite create, le qual es "$3". Si isto esseva tu
intention, tu debe ora aperir un session e seliger un nove contrasigno.

Si un altere persona ha facite iste requesta, o si tu te ha rememorate tu contrasigno,
e tu non vole plus cambiar lo, tu pote ignorar iste message e
continuar a usar tu contrasigno original.',
'noemail'                    => 'Il non ha un adresse de e-mail registrate pro le usator "$1".',
'passwordsent'               => 'Un nove contrasigno esseva inviate al adresse de e-mail
registrate pro "$1".
Per favor initia un session post reciper lo.',
'blocked-mailpassword'       => 'Tu adresse IP es blocate de facer modificationes, e pro impedir le abuso, le uso del function pro recuperar contrasignos es equalmente blocate.',
'eauthentsent'               => 'Un e-mail de confirmation ha essite inviate al adresse de e-mail nominate.
Ante que alcun altere e-mail se invia al conto, tu debera sequer le instructiones in le e-mail, pro confirmar que le conto es de facto tue.',
'throttled-mailpassword'     => 'Un memento del contrasigno jam esseva inviate durante le ultime {{PLURAL:$1|hora|$1 horas}}.
Pro impedir le abuso, nos invia solmente un memento de contrasigno per {{PLURAL:$1|hora|$1 horas}}.',
'mailerror'                  => 'Error de inviar e-mail: $1',
'acct_creation_throttle_hit' => 'Excusa, tu jam ha create $1 contos.
Tu non pote facer plus.',
'emailauthenticated'         => 'Tu adresse de e-mail se authentificava le $1.',
'emailnotauthenticated'      => 'Tu adresse de e-mail non ha essite authentificate ancora.
Nos non inviara e-mail pro alcun del sequente functiones.',
'noemailprefs'               => 'Specifica un adresse de e-mail pro poter executar iste functiones.',
'emailconfirmlink'           => 'Confirmar tu adresse de e-mail',
'invalidemailaddress'        => 'Le adresse de e-mail ha un formato invalide e non pote esser acceptate.
Entra un adresse ben formatate, o vacua ille campo.',
'accountcreated'             => 'Conto create',
'accountcreatedtext'         => 'Le conto del usator $1 ha essite create.',
'createaccount-title'        => 'Creation de contos pro {{SITENAME}}',
'createaccount-text'         => 'Un persona ha create un conto in tu adresse de e-mail a {{SITENAME}} ($4) denominate "$2", con le contrasigno "$3".
Tu deberea aperir un session e cambiar tu contrasigno ora.

Tu pote ignorar iste message si iste conto se creava in error.',
'loginlanguagelabel'         => 'Lingua: $1',

# Password reset dialog
'resetpass'               => 'Redefinir contrasigno del conto',
'resetpass_announce'      => 'Tu ha aperite un session con un codice temporari que tu recipeva in e-mail.
Pro completar le session, tu debe definir un nove contrasigno hic:',
'resetpass_text'          => '<!-- Adde texto hic -->',
'resetpass_header'        => 'Reinitiar contrasigno',
'resetpass_submit'        => 'Definir contrasigno e aperir un session',
'resetpass_success'       => 'Tu contrasigno ha essite cambiate! Ora se aperi tu session...',
'resetpass_bad_temporary' => 'Contrasigno temporari invalide.
Es possibile que tu ha ja cambiate tu contrasigno o ha requestate un nove contrasigno temporari.',
'resetpass_forbidden'     => 'Le contrasignos non pote esser cambiate',
'resetpass_missing'       => 'Le formulario non contineva alcun datos.',

# Edit page toolbar
'bold_sample'     => 'Texto grasse',
'bold_tip'        => 'Texto grasse',
'italic_sample'   => 'Texto italic',
'italic_tip'      => 'Texto italic',
'link_sample'     => 'Titulo del ligamine',
'link_tip'        => 'Ligamine interne',
'extlink_sample'  => 'http://www.example.com titulo del ligamine',
'extlink_tip'     => 'Ligamine externe (non oblida le prefixo http://)',
'headline_sample' => 'Texto del titulo',
'headline_tip'    => 'Titulo de nivello 2',
'math_sample'     => 'Inserer formula hic',
'math_tip'        => 'Formula mathematic (LaTeX)',
'nowiki_sample'   => 'Inserer texto non formatate hic',
'nowiki_tip'      => 'Ignorar formatation wiki',
'image_sample'    => 'Exemplo.jpg',
'image_tip'       => 'File incastrate',
'media_sample'    => 'Exemplo.ogg',
'media_tip'       => 'Ligamine a un file',
'sig_tip'         => 'Tu signatura con data e hora',
'hr_tip'          => 'Linea horizontal (usa con moderation)',

# Edit pages
'summary'                          => 'Summario',
'subject'                          => 'Subjecto/titulo',
'minoredit'                        => 'Isto es un modification minor',
'watchthis'                        => 'Observar iste pagina',
'savearticle'                      => 'Publicar articulo',
'preview'                          => 'Previsualisar',
'showpreview'                      => 'Monstrar previsualisation',
'showlivepreview'                  => 'Previsualisation directe',
'showdiff'                         => 'Detaliar modificationes',
'anoneditwarning'                  => "'''Attention:''' Tu non te ha identificate.
Tu adresse IP essera registrate in le historia de modificationes de iste pagina.",
'missingsummary'                   => "'''Memento:''' Tu non entrava alcun summario del modification.
Si tu clicca super Publicar de novo, le modification essera publicate sin summario.",
'missingcommenttext'               => 'Per favor entra un commento infra.',
'missingcommentheader'             => "'''Memento:''' Tu non entrava un subjecto/titulo pro iste commento.
Si tu clicca super Publicar de novo, tu commento essera publicate sin subjecto/titulo.",
'summary-preview'                  => 'Previsualisation del summario',
'subject-preview'                  => 'Previsualisation del subjecto/titulo',
'blockedtitle'                     => 'Le usator es blocate',
'blockedtext'                      => "<big>'''Tu nomine de usator o adresse IP ha essite blocate.'''</big>

Le blocada esseva facite per $1.
Le motivo presentate es ''$2''.

* Initio del blocada: $8
* Expiration del blocada: $6
* Le blocato intendite: $7

Tu pote contactar $1 o un altere [[{{MediaWiki:Grouppage-sysop}}|administrator]] pro discuter le blocada.
Tu non pote usar le function 'inviar e-mail a iste usator' salvo que un adresse de e-mail valide es specificate in le
[[Special:Preferences|preferentias de tu conto]] e que tu non ha essite blocate de usar lo.
Tu adresse IP actual es $3, e le ID del blocada es #$5.
Per favor include tote le detalios supra specificate in omne correspondentia.",
'autoblockedtext'                  => 'Tu adresse de IP ha essite automaticamente blocate proque un altere usator lo usava qui esseva blocate per $1.
Le motivo presentate es:

:\'\'$2\'\'

* Initio del blocada: $8
* Expiration del blocada: $6
* Blocato intendite: $7

Tu pote contactar $1 o un del altere [[{{MediaWiki:Grouppage-sysop}}|administratores]] pro discuter le blocada.

Nota que tu non pote utilisar le function "inviar e-mail a iste usator" salvo que tu ha registrate un adresse de e-mail valide in tu [[Special:Preferences|preferentias de usator]] e que tu non ha essite blocate de usar lo.

Tu adresse IP actual es $3, e le ID del blocada es #$5.
Per favor include tote le detalios supra specificate in omne correspondentia.',
'blockednoreason'                  => 'nulle ration date',
'blockedoriginalsource'            => "Le codice-fonte de '''$1''' se monstra infra:",
'blockededitsource'                => "Le texto de '''tu modificationes''' in '''$1''' se monstra infra:",
'whitelistedittitle'               => 'Identification requirite pro modificar',
'whitelistedittext'                => 'Tu debe $1 pro poter modificar paginas.',
'confirmedittitle'                 => 'Confirmation del adresse de e-mail es requirite pro poter modificar',
'confirmedittext'                  => 'Tu debe confirmar tu adresse de e-mail pro poter modificar paginas.
Per favor defini e valida tu adresse de e-mail per medio de tu [[Special:Preferences|preferentias de usator]].',
'nosuchsectiontitle'               => 'Non existe tal section',
'nosuchsectiontext'                => 'Tu essayava modificar un section que non existe.
Viste que il non ha alcun section $1, il non ha alcun location pro publicar tu modification.',
'loginreqtitle'                    => 'Identification requirite',
'loginreqlink'                     => 'aperir un session',
'loginreqpagetext'                 => 'Tu debe $1 pro poter vider altere paginas.',
'accmailtitle'                     => 'Contrasigno inviate.',
'accmailtext'                      => 'Le contrasigno pro "$1" ha essite inviate a $2.',
'newarticle'                       => '(Nove)',
'newarticletext'                   => "Tu ha sequite un ligamine verso un pagina que non existe ancora.
Pro crear iste pagina, comencia a scriber in le quadro infra (consulta le [[{{MediaWiki:Helppage}}|pagina de adjuta]] pro plus informationes).
Si tu ha arrivate hic per error, clicca le button '''Retornar''' de tu navigator.",
'anontalkpagetext'                 => "---- ''Isto es le pagina de discussion pro un usator anonyme qui non ha ancora create un conto, o qui non lo usa. Consequentemente nos debe usar le adresse IP numeric pro identificar le/la.
Un tal adresse IP pote esser usate in commun per varie personas.
Si tu es un usator anonyme e pensa que commentos irrelevante ha essite dirigite a te, per favor [[Special:UserLogin/signup|crea un conto]] o [[Special:UserLogin|aperi un session]] pro evitar futur confusiones con altere usatores anonyme.''",
'noarticletext'                    => 'Actualmente il non ha texto in iste pagina. Tu pote [[Special:Search/{{PAGENAME}}|cercar iste titulo]] in le texto de altere paginas o [{{fullurl:{{FULLPAGENAME}}|action=edit}} modificar iste pagina].',
'userpage-userdoesnotexist'        => 'Le conto de usator "$1" non es registrate. Per favor verifica que tu vole crear/modificar iste pagina.',
'clearyourcache'                   => "'''Nota - Post confirmar, il pote esser necessari refrescar le ''cache'' de tu navigator pro vider le cambiamentos.''' '''Mozilla / Firefox / Safari:''' tenente ''Shift'' clicca ''Reload,'' o preme ''Ctrl-F5'' o ''Ctrl-R'' (''Command-R'' in un Macintosh); '''Konqueror: '''clicca ''Reload'' o preme ''F5;'' '''Opera:''' vacua le ''cache'' in ''Tools → Preferences;'' '''Internet Explorer:''' tenente ''Ctrl'' clicca ''Refresh,'' o preme ''Ctrl-F5.''",
'usercssjsyoucanpreview'           => "<strong>Consilio:</strong> Usa le button 'Monstrar previsualisation' pro testar tu nove CSS/JS ante de publicar lo.",
'usercsspreview'                   => "'''Non oblida que isto es solmente un previsualisation de tu CSS personalisate.
Le modificationes non ha ancora essite immagazinate!'''",
'userjspreview'                    => "'''Memora que isto es solmente un test/previsualisation de tu JavaScript personalisate, illo non ha ancora essite immagazinate!'''",
'userinvalidcssjstitle'            => "'''Attention:''' Le stilo \"\$1\" non existe.
Memora que le paginas .css and .js personalisate usa un titulo in minusculas, p.ex. {{ns:user}}:Foo/monobook.css e non {{ns:user}}:Foo/Monobook.css.",
'updated'                          => '(Actualisate)',
'note'                             => '<strong>Nota:</strong>',
'previewnote'                      => '<strong>Isto es solmente un previsualisation;
le modificationes non ha ancora essite publicate!</strong>',
'previewconflict'                  => 'Iste previsualisation reflecte le apparentia final del texto in le area de modification superior
si tu opta pro publicar lo.',
'session_fail_preview'             => '<strong>Nos non poteva processar tu modification proque nos perdeva le datos del session.
Per favor reprova.
Si illo ancora non va, prova [[Special:UserLogout|clauder tu session]] e aperir un nove session.</strong>',
'session_fail_preview_html'        => "<strong>Nos non poteva processar tu modification proque nos perdeva le datos del session.</strong>

''Post que HTML crude es active in {{SITENAME}}, le previsualisation es celate como precaution contra attaccos via JavaScript.''

<strong>Si isto es un tentativa de modification legitime, per favor reprova lo.
Si illo ancora non va, prova [[Special:UserLogout|clauder tu session]] e aperir un nove session.</strong>",
'token_suffix_mismatch'            => "<strong>Tu modification ha essite refusate proque tu cliente corrumpeva le characteres de punctuation in le indicio de modification.
Iste refusa es pro evitar le corruption del texto del pagina.
Isto pote occurrer quando tu usa un servicio problematic de ''proxy'' anonyme a base de web.</strong>",
'editing'                          => 'Modification de $1',
'editingsection'                   => 'Modification de $1 (section)',
'editingcomment'                   => 'Modification de $1 (commento)',
'editconflict'                     => 'Conflicto de modification: $1',
'explainconflict'                  => "Alicuno ha modificate iste pagina post que tu
ha comenciate a modificar lo.
Le area de texto superior contine le texto del pagina como illo existe actualmente.
Tu modificationes se monstra in le area de texto inferior.
Tu debera incorporar tu modificationes in le texto existente.
'''Solmente''' le texto del area superior essera publicate
quando tu clicca super \"Publicar articulo\".",
'yourtext'                         => 'Tu texto',
'storedversion'                    => 'Version immagazinate',
'nonunicodebrowser'                => '<strong>ATTENTION: Tu utilisa un navigator non compatibile con le characteres Unicode.
Se ha activate un systema de modification alternative que te permittera modificar articulos con securitate: le characteres non-ASCII apparera in le quadro de modification como codices hexadecimal.</strong>',
'editingold'                       => '<strong>ATTENTION: Tu va modificar un version obsolete de iste pagina.
Si tu lo publica, tote le modificationes facite post iste revision essera perdite.</strong>',
'yourdiff'                         => 'Differentias',
'copyrightwarning'                 => 'Nota ben que tote le contributiones a {{SITENAME}} se considera publicate sub le $2 (vide plus detalios in $1).
Si tu non vole que tu scripto sia modificate impietosemente e redistribuite a voluntate, alora non lo submitte hic.<br />
In addition, tu nos garanti que tu es le autor de isto, o que tu lo ha copiate de un ressource a dominio public o alteremente libere de derectos.
<strong>NON SUBMITTE MATERIAL SUBJECTE A COPYRIGHT SIN AUTORISATION EXPRESSE!</strong>',
'copyrightwarning2'                => 'Nota ben que tote le contributiones a {{SITENAME}} pote esser redigite, alterate, o eliminate per altere contributores.
Si tu non vole que tu scripto sia modificate impietosemente, alora non lo submitte hic.<br />
In addition, tu nos garanti que tu es le autor de isto, o que tu lo ha copiate de un ressource a dominio public o alteremente libere de derectos (vide detalios in $1).
<strong>NON SUBMITTE MATERIAL SUBJECTE A COPYRIGHT SIN AUTORISATION EXPRESSE!</strong>',
'longpagewarning'                  => '<strong>ATTENTION: Iste pagina occupa $1 kilobytes;
alcun navigatores pote presentar problemas in modificar paginas que approxima o excede 32 kilobytes.
Per favor considera divider le pagina in sectiones minus grande.</strong>',
'longpageerror'                    => '<strong>ERROR: Le texto que tu submitteva occupa $1 kilobytes, excedente le maximo de $2 kilobytes.
Illo non pote esser immagazinate.</strong>',
'readonlywarning'                  => '<strong>ATTENTION: Le base de datos ha essite blocate pro mantenentia, ergo tu non pote immagazinar tu modificationes justo nunc.
Nos recommenda copiar-e-collar le texto pro salveguardar lo in un file de texto, assi que tu potera publicar lo plus tarde.</strong>',
'protectedpagewarning'             => '<strong>ATTENTION:  Iste pagina ha essite protegite. Solmente administratores pote modificar lo.</strong>',
'semiprotectedpagewarning'         => "'''Nota:''' Iste pagina ha essite protegite de maniera que solmente usatores registrate pote modificar lo.",
'cascadeprotectedwarning'          => "'''Attention:''' Iste pagina ha essite protegite de maniera que solmente administratores pote modificar lo, proque illo es includite in le protection in cascada del sequente {{PLURAL:$1|pagina|paginas}}:",
'titleprotectedwarning'            => '<strong>ATTENTION:  Iste pagina ha essite protegite de maniera que solmente certe usatores specific pote crear lo.</strong>',
'templatesused'                    => 'Patronos usate in iste pagina:',
'templatesusedpreview'             => 'Patronos usate in iste previsualisation:',
'templatesusedsection'             => 'Patronos usate in iste section:',
'template-protected'               => '(protegite)',
'template-semiprotected'           => '(semi-protegite)',
'hiddencategories'                 => 'Iste pagina es membro de {{PLURAL:$1|1 categoria|$1 categorias}} celate:',
'edittools'                        => '<!-- Iste texto se monstrara sub le formularios de modificar articulos e de cargar files. -->',
'nocreatetitle'                    => 'Creation de paginas limitate',
'nocreatetext'                     => '{{SITENAME}} ha restringite le permission de crear nove paginas.
Tu pote retornar e modificar un pagina existente, o [[Special:UserLogin|identificar te, o crear un conto]].',
'nocreate-loggedin'                => 'Tu non ha le permission de crear nove paginas.',
'permissionserrors'                => 'Errores de permissiones',
'permissionserrorstext'            => 'Tu non ha le permission de facer isto, pro le sequente {{PLURAL:$1|motivo|motivos}}:',
'permissionserrorstext-withaction' => 'Tu non ha le permission de $2, pro le sequente {{PLURAL:$1|motivo|motivos}}:',
'recreate-deleted-warn'            => "'''Attention: Tu va recrear un pagina que esseva anteriormente delite.'''

Tu deberea considerar si il es appropriate crear iste pagina de novo.
Le registro de deletiones pro iste pagina se trova infra pro major commoditate:",

# Parser/template warnings
'expensive-parserfunction-warning'        => 'Attention: Iste pagina contine troppo de appellos costose al functiones del analysator syntactic.

Illo debe haber minus de $2, sed al momento ha $1.',
'expensive-parserfunction-category'       => 'Paginas con troppo de appellos costose al functiones del analysator syntactic',
'post-expand-template-inclusion-warning'  => 'Attention: Le grandor del patronos includite ha excedite le maximo.
Alcun patronos non essera includite.',
'post-expand-template-inclusion-category' => 'Paginas excedente le grandor maximal del patronos includite',
'post-expand-template-argument-warning'   => 'Attention: Iste pagina contine al minus un parametro de patrono que ha un grandor de expansion excessive.
Iste parametros ha essite omittite.',
'post-expand-template-argument-category'  => 'Paginas que omitte alcun parametros de patrono',

# "Undo" feature
'undo-success' => 'Le modification pote esser annullate.
Per favor controla le comparation infra pro verificar que tu vole facer isto, e alora immagazina le modificationes infra pro assi annullar le modification.',
'undo-failure' => 'Le modification non poteva esser annullate a causa de conflicto con modificationes intermedie.',
'undo-norev'   => 'Impossibile annullar le modification proque illo non existe o esseva delite.',
'undo-summary' => 'Annullava le revision $1 per [[Special:Contributions/$2|$2]] ([[User talk:$2|Discussion]] | [[Special:Contributions/$2|{{MediaWiki:Contribslink}}]])',

# Account creation failure
'cantcreateaccounttitle' => 'Non pote crear conto',
'cantcreateaccount-text' => "Le creation de contos desde iste adresse IP ('''$1''') ha essite blocate per [[User:$3|$3]].

Le motivo que $3 dava es ''$2''",

# History pages
'viewpagelogs'        => 'Vider le registro de iste pagina',
'nohistory'           => 'Non existe un historia de modificationes pro iste pagina.',
'revnotfound'         => 'Revision non trovate',
'revnotfoundtext'     => 'Impossibile trovar le version anterior del pagina que tu ha demandate.
Verifica le adresse URL que tu ha usate pro acceder a iste pagina.',
'currentrev'          => 'Revision actual',
'revisionasof'        => 'Revision del $1',
'revision-info'       => 'Revision del $1 per $2',
'previousrevision'    => '←Revision precedente',
'nextrevision'        => 'Revision sequente→',
'currentrevisionlink' => 'Revision actual',
'cur'                 => 'actu',
'next'                => 'sequ',
'last'                => 'prec',
'page_first'          => 'prime',
'page_last'           => 'ultime',
'histlegend'          => 'Pro detaliar le differentias inter duo versiones: marca lor circulos correspondente, e preme <code>Enter</code> o clicca le button in basso.<br />
Legenda: (actu) = comparar con le version actual,
(prec) = comparar con le version precedente, M = modification minor.',
'deletedrev'          => '[delite]',
'histfirst'           => 'Prime',
'histlast'            => 'Ultime',
'historysize'         => '({{PLURAL:$1|1 byte|$1 bytes}})',
'historyempty'        => '(vacue)',

# Revision feed
'history-feed-title'          => 'Historia de revisiones',
'history-feed-description'    => 'Historia de revisiones de iste pagina in le wiki',
'history-feed-item-nocomment' => '$1 a $2', # user at time
'history-feed-empty'          => 'Le pagina que tu requestava non existe.
Es possibile que illo esseva delite del wiki, o renominate.
Prova [[Special:Search|cercar nove paginas relevante]] in le wiki.',

# Revision deletion
'rev-deleted-comment'         => '(commento eliminate)',
'rev-deleted-user'            => '(nomine de usator eliminate)',
'rev-deleted-event'           => '(entrata eliminate)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">
Iste revision del pagina ha essite eliminate del archivos public.
Es possibile que se trova detalios in le [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} registro de deletiones].</div>',
'rev-deleted-text-view'       => '<div class="mw-warning plainlinks">
Iste revision del pagina ha essite eliminate del archivos public.
Como administrator in {{SITENAME}} tu pote vider lo;
es possibile que se trova detalios in le [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} registro de deletiones].</div>',
'rev-delundel'                => 'revelar/celar',
'revisiondelete'              => 'Deler/restaurar revisiones',
'revdelete-nooldid-title'     => 'Le revision de destination es invalide',
'revdelete-nooldid-text'      => 'O tu non ha specificate alcun revision(es) de destination pro executar iste 
function, o le revision specificate non existe, o tu essaya celar le revision actual.',
'revdelete-selected'          => '{{PLURAL:$2|Revision seligite|Revisiones seligite}} de [[:$1]]:',
'logdelete-selected'          => '{{PLURAL:$1|Evento|Eventos}} de registro seligite:',
'revdelete-text'              => 'Le revisiones e eventos delite continuara a apparer in le historia e registro del pagina, sed partes de lor contento essera inaccessibile al publico.

Altere administratores in {{SITENAME}} continuara a poter acceder al contento celate e pote restaurar lo per medio de iste mesme interfacie, si non se ha definite restrictiones additional.',
'revdelete-legend'            => 'Definir restrictiones de visibilitate',
'revdelete-hide-text'         => 'Celar texto del revision',
'revdelete-hide-name'         => 'Celar action e objectivo',
'revdelete-hide-comment'      => 'Celar commento de modification',
'revdelete-hide-user'         => 'Celar nomine de usator o adresse IP del modificator',
'revdelete-hide-restricted'   => 'Applicar iste restrictiones al administratores e blocar iste interfacie',
'revdelete-suppress'          => 'Supprimer datos e de Administratores e de alteres',
'revdelete-hide-image'        => 'Celar contento del file',
'revdelete-unsuppress'        => 'Eliminar restrictiones super revisiones restaurate',
'revdelete-log'               => 'Commento pro registro:',
'revdelete-submit'            => 'Applicar al revision seligite',
'revdelete-logentry'          => 'cambiava le visibilitate de revisiones pro [[$1]]',
'logdelete-logentry'          => 'cambiava le visibilitate de eventos pro [[$1]]',
'revdelete-success'           => "'''Le visibilitate de revisiones ha essite definite con successo.'''",
'logdelete-success'           => "'''Le visibilitate del registro ha essite definite con successo.'''",
'revdel-restore'              => 'Cambiar visibilitate',
'pagehist'                    => 'Historia del pagina',
'deletedhist'                 => 'Historia delite',
'revdelete-content'           => 'contento',
'revdelete-summary'           => 'summario del modification',
'revdelete-uname'             => 'nomine de usator',
'revdelete-restricted'        => 'restrictiones applicate al administratores',
'revdelete-unrestricted'      => 'restrictiones eliminate pro administratores',
'revdelete-hid'               => 'celava $1',
'revdelete-unhid'             => 'revelava $1',
'revdelete-log-message'       => '$1 pro $2 {{PLURAL:$2|revision|revisiones}}',
'logdelete-log-message'       => '$1 pro $2 {{PLURAL:$2|evento|eventos}}',

# Suppression log
'suppressionlog'     => 'Registro de suppressiones',
'suppressionlogtext' => 'Infra es un lista de deletiones e blocadas que involve contento que es celate de administratores.
Vide le [[Special:IPBlockList|lista de blocadas IP]] pro le lista de bannimentos e blocadas actualmente in operation.',

# History merging
'mergehistory'                     => 'Fusionar historias del paginas',
'mergehistory-header'              => 'Iste pagina te permitte fusionar revisiones del historia de un pagina de origine in un pagina plus nove.
Assecura te que iste cambio mantenera le continuitate historic del pagina.',
'mergehistory-box'                 => 'Fusionar le revisiones de duo paginas:',
'mergehistory-from'                => 'Pagina de origine:',
'mergehistory-into'                => 'Pagina de destination:',
'mergehistory-list'                => 'Historia de modificationes fusionabile',
'mergehistory-merge'               => 'Le sequente revisiones de [[:$1]] pote esser fusionate in [[:$2]].
Usa le columna de buttones radio pro fusionar solmente le revisiones create in e ante le tempore specificate.
Nota que le uso del ligamines de navigation causara le perdita de tote cambios in iste columna.',
'mergehistory-go'                  => 'Revelar modificationes fusionabile',
'mergehistory-submit'              => 'Fusionar revisiones',
'mergehistory-empty'               => 'Nulle revisiones pote esser fusionate.',
'mergehistory-success'             => '$3 {{PLURAL:$3|revision|revisiones}} de [[:$1]] fusionate in [[:$2]] con successo.',
'mergehistory-fail'                => 'Impossibile executar le fusion del historia. Per favor reverifica le parametros del pagina e del tempore.',
'mergehistory-no-source'           => 'Le pagina de origine $1 non existe.',
'mergehistory-no-destination'      => 'Le pagina de destination $1 non existe.',
'mergehistory-invalid-source'      => 'Le pagina de origine debe esser un titulo valide.',
'mergehistory-invalid-destination' => 'Le pagina de destination debe esser un titulo valide.',
'mergehistory-autocomment'         => 'Fusionava [[:$1]] in [[:$2]]',
'mergehistory-comment'             => 'Fusionava [[:$1]] in [[:$2]]: $3',

# Merge log
'mergelog'           => 'Registro de fusiones',
'pagemerge-logentry' => 'fusionava [[$1]] in [[$2]] (revisiones usque a $3)',
'revertmerge'        => 'Reverter fusion',
'mergelogpagetext'   => 'Infra es un lista del fusiones le plus recente de un historia de pagina in un altere.',

# Diffs
'history-title'           => 'Historia de revisiones de "$1"',
'difference'              => '(Differentia inter revisiones)',
'lineno'                  => 'Linea $1:',
'compareselectedversions' => 'Comparar versiones seligite',
'editundo'                => 'annullar',
'diff-multi'              => '({{PLURAL:$1|Un revision intermedie|$1 revisiones intermedie}} non se revela.)',

# Search results
'searchresults'             => 'Resultatos del recerca',
'searchresulttext'          => 'Pro plus informationes super le recerca in {{SITENAME}}, vide [[{{MediaWiki:Helppage}}|{{int:help}}]].',
'searchsubtitle'            => 'Tu cercava \'\'\'[[:$1]]\'\'\' ([[Special:Prefixindex/$1|tote le paginas que comencia con "$1"]] | [[Special:WhatLinksHere/$1|tote le paginas con ligamines a "$1"]])',
'searchsubtitleinvalid'     => "Tu cercava '''$1'''",
'noexactmatch'              => "'''Non existe un pagina con le titulo \"\$1\".'''
Tu pote [[:\$1|crear iste pagina]].",
'noexactmatch-nocreate'     => "'''Non existe un pagina con titulo \"\$1\".'''",
'toomanymatches'            => 'Se retornava troppo de resultatos. Per favor prova un altere consulta.',
'titlematches'              => 'Correspondentias in le titulos de paginas',
'notitlematches'            => 'Nulle correspondentias in le titulos de paginas',
'textmatches'               => 'Resultatos in le texto de paginas',
'notextmatches'             => 'Nulle resultato in le texto de paginas',
'prevn'                     => '$1 {{PLURAL:$1|precedente|precedentes}}',
'nextn'                     => '$1 {{PLURAL:$1|sequente|sequentes}}',
'viewprevnext'              => 'Vider ($1) ($2) ($3).',
'search-result-size'        => '$1 ({{PLURAL:$2|1 parola|$2 parolas}})',
'search-result-score'       => 'Relevantia: $1%',
'search-redirect'           => '(redirection verso $1)',
'search-section'            => '(section $1)',
'search-suggest'            => 'Esque tu vole dicer: $1',
'search-interwiki-caption'  => 'Projectos fratres',
'search-interwiki-default'  => 'Resultatos de $1:',
'search-interwiki-more'     => '(plus)',
'search-mwsuggest-enabled'  => 'con suggestiones',
'search-mwsuggest-disabled' => 'sin suggestiones',
'search-relatedarticle'     => 'Connexe',
'mwsuggest-disable'         => 'Disactivar suggestiones via AJAX',
'searchrelated'             => 'connexe',
'searchall'                 => 'totes',
'showingresults'            => "Infra se monstra non plus de {{PLURAL:$1|'''1''' resultato|'''$1''' resultatos}} a partir del numero '''$2'''.",
'showingresultsnum'         => "Infra se monstra {{PLURAL:$3|'''1''' resultato|'''$3''' resultatos}} a partir del numero '''$2'''.",
'showingresultstotal'       => "Infra se monstra le {{PLURAL:$3|resultato '''$1''' de '''$3'''|resultatos '''$1 - $2''' de '''$3'''}}",
'nonefound'                 => "'''Nota:''' Normalmente, se cerca solmente in alcun spatios de nomines. Prova prefixar tu consulta con ''all:'' pro cercar in tote le contento (includente paginas de discussion, patronos, etc.), o usa le spatio de nomines desirate como prefixo.",
'powersearch'               => 'Recerca avantiate',
'powersearch-legend'        => 'Recerca avantiate',
'powersearch-ns'            => 'Cercar in spatios de nomines:',
'powersearch-redir'         => 'Listar redirectiones',
'powersearch-field'         => 'Cercar',
'search-external'           => 'Recerca externe',
'searchdisabled'            => 'Le recerca in {{SITENAME}} es disactivate.
Tu pote cercar via Google in le interim.
Nota que lor indices del contento de {{SITENAME}} pote esser obsolete.',

# Preferences page
'preferences'              => 'Preferentias',
'mypreferences'            => 'Mi preferentias',
'prefs-edits'              => 'Numero de modificationes:',
'prefsnologin'             => 'Tu non te ha identificate',
'prefsnologintext'         => 'Tu debe <span class="plainlinks">[{{fullurl:Special:Userlogin|returnto=$1}} aperir un session] pro poter configurar tu preferentias.',
'prefsreset'               => 'Tu preferentias anterior ha essite restaurate.',
'qbsettings'               => 'Barra rapide',
'qbsettings-none'          => 'Necun',
'qbsettings-fixedleft'     => 'Fixe a sinistra',
'qbsettings-fixedright'    => 'Fixe a derecta',
'qbsettings-floatingleft'  => 'Flottante a sinistra',
'qbsettings-floatingright' => 'Flottante a derecta',
'changepassword'           => 'Cambiar contrasigno',
'skin'                     => 'Stilo',
'math'                     => 'Mathematica',
'dateformat'               => 'Formato de datas',
'datedefault'              => 'Nulle preferentia',
'datetime'                 => 'Data e hora',
'math_failure'             => 'Error durante le analyse del syntaxe',
'math_unknown_error'       => 'error incognite',
'math_unknown_function'    => 'function incognite',
'math_lexing_error'        => 'error lexic',
'math_syntax_error'        => 'error de syntaxe',
'math_image_error'         => "Le conversion in PNG ha fallite;
verifica que le installation sia correcte del programmas ''latex, dvips, gs,'' e ''convert''.",
'math_bad_tmpdir'          => 'Non pote scriber in o crear le directorio temporari "math".',
'math_bad_output'          => 'Non pote scriber in o crear le directorio de output "math".',
'math_notexvc'             => "Le executabile ''texvc'' manca;
per favor vide math/README pro configurar lo.",
'prefs-personal'           => 'Profilo del usator',
'prefs-rc'                 => 'Modificationes recente',
'prefs-watchlist'          => 'Observatorio',
'prefs-watchlist-days'     => 'Numero de dies a monstrar in le observatorio:',
'prefs-watchlist-edits'    => 'Numero maximal de modificationes a monstrar in le observatorio expandite:',
'prefs-misc'               => 'Misc',
'saveprefs'                => 'Confirmar',
'resetprefs'               => 'Reverter cambios',
'oldpassword'              => 'Contrasigno actual:',
'newpassword'              => 'Nove contrasigno:',
'retypenew'                => 'Repete le nove contrasigno:',
'textboxsize'              => 'Modification',
'rows'                     => 'Lineas:',
'columns'                  => 'Columnas:',
'searchresultshead'        => 'Recerca',
'resultsperpage'           => 'Resultatos per pagina:',
'contextlines'             => 'Lineas per resultato:',
'contextchars'             => 'Characteres de contexto per linea:',
'stub-threshold'           => 'Limite pro formatar le ligamines in <a href="#" class="stub">stilo de peciettas</a> (bytes):',
'recentchangesdays'        => 'Numero de dies a monstrar in modificationes recente:',
'recentchangescount'       => 'Numero de modificationes a monstrar in paginas de modificationes recente, de historia e de registro:',
'savedprefs'               => 'Tu preferentias ha essite confirmate.',
'timezonelegend'           => 'Fuso horari',
'timezonetext'             => '¹Le numero de horas inter tu hora local e le hora del servitor (UTC).',
'localtime'                => 'Hora local',
'timezoneoffset'           => 'Differentia¹',
'servertime'               => 'Hora del servitor',
'guesstimezone'            => 'Obtener del navigator',
'allowemail'               => 'Activar reception de e-mail de altere usatores',
'prefs-searchoptions'      => 'Optiones de recerca',
'prefs-namespaces'         => 'Spatios de nomines',
'defaultns'                => 'Cercar initialmente in iste spatios de nomines:',
'default'                  => 'predefinition',
'files'                    => 'Files',

# User rights
'userrights'                  => 'Gestion de derectos de usator', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => 'Gerer gruppos de usatores',
'userrights-user-editname'    => 'Entra un nomine de usator:',
'editusergroup'               => 'Modificar gruppos de usatores',
'editinguser'                 => "Cambiamento del derectos del usator '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup'    => 'Modificar gruppos de usatores',
'saveusergroups'              => 'Immagazinar gruppos de usatores',
'userrights-groupsmember'     => 'Membro de:',
'userrights-groups-help'      => 'Tu pote alterar le gruppos del quales iste usator es membro:
* Un quadrato marcate significa que le usator es membro del gruppo in question.
* Un non marcate significa que ille non es membro de illo.
* Un * indica que tu non potera eliminar le gruppo quando tu lo ha addite, o vice versa.',
'userrights-reason'           => 'Motivo pro le cambio:',
'userrights-no-interwiki'     => 'Tu non ha le permission de modificar le derectos de usatores in altere wikis.',
'userrights-nodatabase'       => 'Le base de datos $1 non existe o non es local.',
'userrights-nologin'          => 'Tu debe [[Special:UserLogin|aperir un session]] con un conto de administrator pro poter assignar derectos de usator.',
'userrights-notallowed'       => 'Tu conto non ha le permission de assignar derectos de usator.',
'userrights-changeable-col'   => 'Gruppos que tu pote modificar',
'userrights-unchangeable-col' => 'Gruppos que tu non pote modificar',

# Groups
'group'               => 'Gruppo:',
'group-user'          => 'Usatores',
'group-autoconfirmed' => 'Usatores autoconfirmate',
'group-bot'           => 'Bots',
'group-sysop'         => 'Administratores',
'group-bureaucrat'    => 'Bureaucrates',
'group-suppress'      => 'Supervisores',
'group-all'           => '(totes)',

'group-user-member'          => 'Usator',
'group-autoconfirmed-member' => 'Usator autoconfirmate',
'group-bot-member'           => 'Bot',
'group-sysop-member'         => 'Administrator',
'group-bureaucrat-member'    => 'Bureaucrate',
'group-suppress-member'      => 'Supervisor',

'grouppage-user'          => '{{ns:project}}:Usatores',
'grouppage-autoconfirmed' => '{{ns:project}}:Usatores autoconfirmate',
'grouppage-bot'           => '{{ns:project}}:Bots',
'grouppage-sysop'         => '{{ns:project}}:Administratores',
'grouppage-bureaucrat'    => '{{ns:project}}:Bureaucrates',
'grouppage-suppress'      => '{{ns:project}}:Supervisores',

# Rights
'right-read'                 => 'Leger paginas',
'right-edit'                 => 'Modificar paginas',
'right-createpage'           => 'Crear paginas (non discussion)',
'right-createtalk'           => 'Crear paginas de discussion',
'right-createaccount'        => 'Crear nove contos de usator',
'right-minoredit'            => 'Marcar modificationes como minor',
'right-move'                 => 'Renominar paginas',
'right-move-subpages'        => 'Renominar paginas con lor subpaginas',
'right-suppressredirect'     => 'Non rediriger le ancian nomine verso le nove quando se renomina un pagina',
'right-upload'               => 'Cargar files',
'right-reupload'             => 'Superscriber un file existente',
'right-reupload-own'         => 'Superscriber un file anteriormente cargate per uno mesme',
'right-reupload-shared'      => 'Supplantar localmente le files del respositorio commun de media',
'right-upload_by_url'        => 'Cargar un file ab un adresse URL',
'right-purge'                => 'Purgar le cache de un pagina in le sito sin confirmation',
'right-autoconfirmed'        => 'Modificar paginas semiprotegite',
'right-bot'                  => 'Esser tractate como processo automatic',
'right-nominornewtalk'       => 'Non reciper notification de nove messages quando se face modificationes minor in le pagina de discussion',
'right-apihighlimits'        => 'Usar limites plus alte in consultas via API',
'right-writeapi'             => 'Uso del API pro modificar le wiki',
'right-delete'               => 'Deler paginas',
'right-bigdelete'            => 'Deler paginas con historias longe',
'right-deleterevision'       => 'Deler e restaurar revisiones specific de paginas',
'right-deletedhistory'       => 'Vider entratas de historia delite, sin lor texto associate',
'right-browsearchive'        => 'Cercar in paginas delite',
'right-undelete'             => 'Restaurar un pagina',
'right-suppressrevision'     => 'Revider e restaurar revisiones celate ab administratores',
'right-suppressionlog'       => 'Vider registros private',
'right-block'                => 'Blocar altere usatores de facer modificationes',
'right-blockemail'           => 'Blocar un usator de inviar e-mail',
'right-hideuser'             => 'Blocar un nomine de usator, celante lo del publico',
'right-ipblock-exempt'       => 'Contornar le blocadas de adresses IP, blocadas automatic e blocadas de intervallos IP',
'right-proxyunbannable'      => 'Contornar le blocadas automatic de proxy',
'right-protect'              => 'Cambiar nivellos de protection e modificar paginas protegite',
'right-editprotected'        => 'Modificar paginas protegite (sin cascada)',
'right-editinterface'        => 'Modificar le interfacie de usator',
'right-editusercssjs'        => 'Modificar le files CSS e JS de altere usatores',
'right-rollback'             => 'Rapidemente revocar le modificationes del ultime usator que modificava un pagina particular',
'right-markbotedits'         => 'Marcar modificationes de reversion como facite per un bot',
'right-noratelimit'          => 'Non esser subjecte al limites de frequentia de actiones',
'right-import'               => 'Importar paginas de altere wikis',
'right-importupload'         => 'Importar paginas specificate in un file que tu carga',
'right-patrol'               => 'Marcar le modificationes de alteres como patruliate',
'right-autopatrol'           => 'Marcar automaticamente le proprie modificationes como patruliate',
'right-patrolmarks'          => 'Vider marcas de patrulia in le modificationes recente',
'right-unwatchedpages'       => 'Vider un lista de paginas non observate',
'right-trackback'            => 'Submitter un retroligamine',
'right-mergehistory'         => 'Fusionar le historia de paginas',
'right-userrights'           => 'Modificar tote le derectos de usator',
'right-userrights-interwiki' => 'Modificar le derectos de usatores in altere wikis',
'right-siteadmin'            => 'Blocar e disblocar le base de datos',

# User rights log
'rightslog'      => 'Registro de derectos de usator',
'rightslogtext'  => 'Isto es un registro de cambios in derectos de usator.',
'rightslogentry' => 'cambiava le gruppos del quales $1 es membro de $2 a $3',
'rightsnone'     => '(nulle)',

# Recent changes
'nchanges'                          => '$1 {{PLURAL:$1|modification|modificationes}}',
'recentchanges'                     => 'Modificationes recente',
'recentchangestext'                 => 'Seque le plus recente modificationes a {{SITENAME}} in iste pagina.',
'recentchanges-feed-description'    => 'Seque le modificationes le plus recente al wiki in iste syndication.',
'rcnote'                            => "Infra es {{PLURAL:$1|'''1''' modification|le ultime '''$1''' modificationes}} in le ultime {{PLURAL:$2|die|'''$2''' dies}}, actualisate le $4 a $5.",
'rcnotefrom'                        => 'infra es le modificationes a partir de <b>$2</b> (usque a <b>$1</b>).',
'rclistfrom'                        => 'Monstrar nove modificationes a partir de $1',
'rcshowhideminor'                   => '$1 modificationes minor',
'rcshowhidebots'                    => '$1 bots',
'rcshowhideliu'                     => '$1 usatores registrate',
'rcshowhideanons'                   => '$1 usatores anonyme',
'rcshowhidepatr'                    => '$1 modificationes patruliate',
'rcshowhidemine'                    => '$1 mi modificationes',
'rclinks'                           => 'Monstrar le $1 ultime modificationes in le $2 ultime dies<br />$3',
'diff'                              => 'diff',
'hist'                              => 'hist',
'hide'                              => 'Celar',
'show'                              => 'Revelar',
'minoreditletter'                   => 'm',
'newpageletter'                     => 'N',
'boteditletter'                     => 'b',
'number_of_watching_users_pageview' => '[observate per $1 {{PLURAL:$1|usator|usatores}}]',
'rc_categories'                     => 'Limite a categorias (separar con "|")',
'rc_categories_any'                 => 'Qualcunque',
'newsectionsummary'                 => '/* $1 */ nove section',

# Recent changes linked
'recentchangeslinked'          => 'Modificationes correlate',
'recentchangeslinked-title'    => 'Modificationes associate a "$1"',
'recentchangeslinked-noresult' => 'Nulle modificationes in paginas ligate durante iste periodo.',
'recentchangeslinked-summary'  => "Isto es un lista de modificationes facite recentemente a paginas al quales se refere ligamines in un altere pagina specific (o a membros de un categoria specific).
Le paginas presente in [[Special:Watchlist|tu observatorio]] se revela in litteras '''grasse'''.",
'recentchangeslinked-page'     => 'Nomine del pagina:',
'recentchangeslinked-to'       => 'Monstrar modificationes in le paginas al quales le pagina que tu specificava contine ligamines',

# Upload
'upload'                      => 'Cargar file',
'uploadbtn'                   => 'Cargar file',
'reupload'                    => 'Recargar',
'reuploaddesc'                => 'Cancellar le carga e retornar al formulario de carga',
'uploadnologin'               => 'Tu non te ha identificate',
'uploadnologintext'           => 'Tu debe [[Special:UserLogin|aperir un session]] pro poter cargar files.',
'upload_directory_missing'    => 'Le directorio de cargamento ($1) manca, e le servitor de web non poteva crear lo.',
'upload_directory_read_only'  => 'Le servitor de web non ha le permission de scriber in le directorio de cargamento ($1).',
'uploaderror'                 => 'Error de carga',
'uploadtext'                  => "Tu pote cargar files con le formulario infra.
Pro vider o cercar imagines cargate anteriormente, visita le [[Special:ImageList|lista de imagines cargate]]. In ultra, le (re)cargas es registrate in le [[Special:Log/upload|registro de cargas]], le deletiones in le [[Special:Log/delete|registro de deletiones]].

Pro includer un file in un articulo, usa un ligamine in un del sequente formas:
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:File.jpg]]</nowiki></tt>''' pro usar le version complete del file
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:File.png|200px|thumb|left|texto alternative]]</nowiki></tt>''' pro usar un rendition a largor de 200 pixel in un quadro in le margine sinistre con 'texto alternative' qua description
* '''<tt><nowiki>[[</nowiki>{{ns:media}}<nowiki>:File.ogg]]</nowiki></tt>''' pro facer un ligamine directe al file sin monstrar le file",
'upload-permitted'            => 'Typos de file permittite: $1.',
'upload-preferred'            => 'Typos de file preferite: $1.',
'upload-prohibited'           => 'Typos de file prohibite: $1.',
'uploadlog'                   => 'registro de cargas',
'uploadlogpage'               => 'Registro de cargas',
'uploadlogpagetext'           => 'Infra es un lista del plus recente cargas de files.
Vide le [[Special:NewImages|galeria de nove files]] pro un presentation plus visual.',
'filename'                    => 'Nomine del file',
'filedesc'                    => 'Summario',
'fileuploadsummary'           => 'Summario:',
'filestatus'                  => 'Stato de copyright:',
'filesource'                  => 'Origine:',
'uploadedfiles'               => 'Files cargate',
'ignorewarning'               => 'Ignorar le advertimento e immagazinar totevia le file',
'ignorewarnings'              => 'Ignorar omne advertimentos',
'minlength1'                  => 'Le nomines de file debe haber al minus un littera.',
'illegalfilename'             => 'Le nomine de file "$1" contine characteres que non es permittite in le titulos de paginas.
Per favor renomina le file e prova recargar lo.',
'badfilename'                 => 'Le nomine del imagine esseva cambiate a "$1".',
'filetype-badmime'            => 'Non es permittite cargar files del typo MIME "$1".',
'filetype-unwanted-type'      => "'''\".\$1\"''' es un typo de file non desirate.
Le {{PLURAL:\$3|typo|typos}} de file preferite es \$2.",
'filetype-banned-type'        => "Le typo de file '''\".\$1\"''' non es permittite.
Le {{PLURAL:\$3|typo|typos}} de file permittite es \$2.",
'filetype-missing'            => 'Le nomine del file non ha un extension (como ".jpg").',
'large-file'                  => 'Es recommendate que le files non sia plus grande de $1;
iste file occupa $2.',
'largefileserver'             => 'Le grandor de iste file excede le limite configurate in le servitor.',
'emptyfile'                   => 'Le file que tu cargava pare esser vacue.
Isto pote esser debite a un error in le nomine del file.
Per favor verifica que tu realmente vole cargar iste file.',
'fileexists'                  => 'Un file con iste nomine existe ja. Per favor verifica <strong><tt>$1</tt></strong> si tu non es secur de voler cambiar lo.',
'filepageexists'              => 'Le pagina de description correspondente a iste file ha jam essite create a <strong><tt>$1</tt></strong>, sed un file con iste nomine non existe al momento.
Le summario que tu entra non apparera in le pagina de description.
Si tu vole que illo appare, tu debe inserer lo manualmente.',
'fileexists-extension'        => 'Un file con un nomine similar existe ja:<br />
Nomine del file que tu carga: <strong><tt>$1</tt></strong><br />
Nomine del file existente: <strong><tt>$2</tt></strong><br />
Per favor selige un altere nomine.',
'fileexists-thumb'            => "<center>'''File existente'''</center>",
'fileexists-thumbnail-yes'    => 'Iste file pare esser un imagine a grandor reducite <i>(miniatura)</i>.
Per favor verifica le file <strong><tt>$1</tt></strong>.<br />
Si le file verificate es le mesme imagine a grandor original, non es necessari cargar un miniatura additional.',
'file-thumbnail-no'           => 'Le nomine del file comencia con <strong><tt>$1</tt></strong>.
Illo pare esser un imagine a grandor reducite <i>(miniatura)</i>.
Si tu possede iste imagine in plen resolution, carga lo, alteremente cambia le nomine del file per favor.',
'fileexists-forbidden'        => 'Un file con iste nomine existe ja;
per favor retorna e carga iste file sub un altere nomine. [[Image:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => 'Un file con iste nomine existe ja in le repositorio de files commun.
Si tu vole totevia cargar iste file, per favor retorna e usa un nove nomine. [[Image:$1|thumb|center|$1]]',
'file-exists-duplicate'       => 'Iste file es un duplicato del sequente {{PLURAL:$1|file|files}}:',
'successfulupload'            => 'Cargamento succedite',
'uploadwarning'               => 'Advertimento de cargamento',
'savefile'                    => 'Immagazinar file',
'uploadedimage'               => '"[[$1]]" cargate',
'overwroteimage'              => 'cargava un nove version de "[[$1]]"',
'uploaddisabled'              => 'Cargamentos disactivate',
'uploaddisabledtext'          => 'Le cargamento de files es disactivate.',
'uploadscripted'              => 'Iste file contine codice de HTML o de script que pote esser interpretate erroneemente per un navigator del web.',
'uploadcorrupt'               => 'Le file es corrupte o su nomine ha un extension incorrecte.
Per favor verifica le file e recarga lo.',
'uploadvirus'                 => 'Le file contine un virus! Detalios: $1',
'sourcefilename'              => 'Nomine del file de origine:',
'destfilename'                => 'Nomine del file de destination:',
'upload-maxfilesize'          => 'Grandor maximal del files: $1',
'watchthisupload'             => 'Observar iste pagina',
'filewasdeleted'              => 'Un file con iste nomine ha anteriormente essite cargate e postea delite.
Tu debe verificar le $1 ante de proceder e recargar lo.',
'upload-wasdeleted'           => "'''Attention: Tu va cargar un file que esseva anteriormente delite.'''

Tu debe considerar si es appropriate continuar a cargar iste file.
Pro major commoditate se trova hic le registro de deletiones correspondente a iste file:",
'filename-bad-prefix'         => 'Le nomine del file que tu va cargar comencia con <strong>"$1"</strong>, le qual es un nomine non descriptive, typicamente assignate automaticamente per le cameras digital.
Per favor selige un nomine plus descriptive pro tu file.',
'filename-prefix-blacklist'   => ' #<!-- non modificar de alcun modo iste linea --> <pre>
# Le syntaxe es como seque:
#   * Toto a partir de un character "#" usque al fin del linea es un commento
#   * Cata linea non vacue es un prefixo pro tal nomines de file como automaticamente assignate per cameras digital
CIMG # Casio
DSC_ # Nikon
DSCF # Fuji
DSCN # Nikon
DUW # alcun telephonos mobile
IMG # generic
JD # Jenoptik
MGP # Pentax
PICT # misc.
 #</pre> <!-- non modificar de alcun modo iste linea -->',

'upload-proto-error'      => 'Protocollo incorrecte',
'upload-proto-error-text' => 'Le cargamento remote require que le adresses URL comencia con <code>http://</code> o <code>ftp://</code>.',
'upload-file-error'       => 'Error interne',
'upload-file-error-text'  => 'Un error interne occurreva quando se tentava crear un file temporari in le servitor.
Per favor contacta un [[Special:ListUsers/sysop|administrator]].',
'upload-misc-error'       => 'Error de cargamento non cognoscite',
'upload-misc-error-text'  => 'Un error non cognoscite occurreva durante le cargamento.
Per favor verifica que le adresse URL sia valide e accessible, e reprova.
Si le problema persiste, contacta un [[Special:ListUsers/sysop|administrator]].',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => 'Non poteva acceder al URL',
'upload-curl-error6-text'  => 'Le adresse URL fornite es inaccessibile.
Per favor reverifica que le adresse URL sia correcte e que le sito sia in operation.',
'upload-curl-error28'      => 'Le cargamento se prolongava troppo',
'upload-curl-error28-text' => 'Le sito non respondeva intra le limite de tempore.
Per favor controla que le sito sia in operation, attende un poco e reprova.
Pote esser preferite reprovar quando le sito es minus occupate.',

'license'            => 'Licentia:',
'nolicense'          => 'Necun licentia seligite',
'license-nopreview'  => '(Previsualisation non disponibile)',
'upload_source_url'  => ' (un adresse URL valide e publicamente accessibile)',
'upload_source_file' => ' (un file in tu computator)',

# Special:ImageList
'imagelist-summary'     => 'Iste pagina special monstra tote le files cargate.
Per predefinition le ultime files cargate se monstra al initio del lista.
Tu pote reordinar le lista con un clic super le titulo de un columna.',
'imagelist_search_for'  => 'Cercar un nomine de media:',
'imgfile'               => 'file',
'imagelist'             => 'Lista de files',
'imagelist_date'        => 'Data',
'imagelist_name'        => 'Nomine',
'imagelist_user'        => 'Usator',
'imagelist_size'        => 'Grandor',
'imagelist_description' => 'Description',

# Image description page
'filehist'                       => 'Historia del file',
'filehist-help'                  => 'Clicca super un data/hora pro vider le file como appareva a ille tempore.',
'filehist-deleteall'             => 'deler totes',
'filehist-deleteone'             => 'deler',
'filehist-revert'                => 'reverter',
'filehist-current'               => 'actual',
'filehist-datetime'              => 'Data/Hora',
'filehist-user'                  => 'Usator',
'filehist-dimensions'            => 'Dimensiones',
'filehist-filesize'              => 'Grandor del file',
'filehist-comment'               => 'Commento',
'imagelinks'                     => 'Ligamines',
'linkstoimage'                   => 'Le sequente {{PLURAL:$1|pagina ha un ligamine|$1 paginas ha ligamines}} verso iste file:',
'nolinkstoimage'                 => 'Nulle pagina se liga verso iste file.',
'morelinkstoimage'               => 'Vider [[Special:WhatLinksHere/$1|plus ligamines]] a iste file.',
'redirectstofile'                => 'Le sequente {{PLURAL:$1|file|$1 files}} se redirige a iste file:',
'duplicatesoffile'               => 'Le sequente {{PLURAL:$1|files es un duplicato|$1 files es duplicatos}} de iste file:',
'sharedupload'                   => 'Iste file ha essite cargate pro uso in commun; altere projectos pote usar lo.',
'shareduploadwiki'               => 'Per favor vide le $1 pro ulterior informationes.',
'shareduploadwiki-desc'          => 'Infra se monstra le description in su $1 in le repositorio commun.',
'shareduploadwiki-linktext'      => 'pagina de description del file',
'shareduploadduplicate'          => 'Iste file es un duplicato de $1 del repositorio commun.',
'shareduploadduplicate-linktext' => 'un altere file',
'shareduploadconflict'           => 'Iste file ha le mesme nomine que $1 del repositorio commun.',
'shareduploadconflict-linktext'  => 'un altere file',
'noimage'                        => 'Non existe un file con iste nomine, sed tu pote $1.',
'noimage-linktext'               => 'cargar un',
'uploadnewversion-linktext'      => 'Cargar un nove version de iste file',
'imagepage-searchdupe'           => 'Cercar files duplicate',

# File reversion
'filerevert'                => 'Reverter $1',
'filerevert-legend'         => 'Reverter file',
'filerevert-intro'          => "Tu reverte '''[[Media:$1|$1]]''' al [$4 version del $3 a $2].",
'filerevert-comment'        => 'Commento:',
'filerevert-defaultcomment' => 'Revertite al version del $2 a $1',
'filerevert-submit'         => 'Reverter',
'filerevert-success'        => "'''[[Media:$1|$1]]''' ha essite revertite al [$4 version del $3 a $2].",
'filerevert-badversion'     => 'Non existe un version local anterior de iste file con le data e hora providite.',

# File deletion
'filedelete'                  => 'Deler $1',
'filedelete-legend'           => 'Deler file',
'filedelete-intro'            => "Tu va deler '''[[Media:$1|$1]]'''.",
'filedelete-intro-old'        => "Tu va deler le version de '''[[Media:$1|$1]]''' del [$4 $3 a $2].",
'filedelete-comment'          => 'Motivo pro deletion:',
'filedelete-submit'           => 'Deler',
'filedelete-success'          => "'''$1''' ha essite delite.",
'filedelete-success-old'      => "Le version de '''[[Media:$1|$1]]''' del $3 a $2 ha essite delite.",
'filedelete-nofile'           => "'''$1''' non existe.",
'filedelete-nofile-old'       => "Non existe un version archivate de '''$1''' con le attributos specificate.",
'filedelete-iscurrent'        => 'Tu essaya deler le version le plus recente de iste file.
Per favor reverte lo primo a un version anterior.',
'filedelete-otherreason'      => 'Motivo altere/additional:',
'filedelete-reason-otherlist' => 'Altere motivo',
'filedelete-reason-dropdown'  => '*Motivos habitual pro deletion
** Violation de copyright
** File duplicate',
'filedelete-edit-reasonlist'  => 'Modificar motivos pro deletion',

# MIME search
'mimesearch'         => 'Recerca de typo MIME',
'mimesearch-summary' => 'Iste pagina permitte filtrar le files a base de lor typos MIME.
Syntaxe: typo/subtypo, p.ex. <tt>image/jpeg</tt>.',
'mimetype'           => 'Typo MIME:',
'download'           => 'discargar',

# Unwatched pages
'unwatchedpages' => 'Paginas non observate',

# List redirects
'listredirects' => 'Listar redirectiones',

# Unused templates
'unusedtemplates'     => 'Patronos non usate',
'unusedtemplatestext' => 'Iste pagina es un lista de tote le paginas in le spatio de nomines "Patrono" que non es includite in un altere pagina.
Memora verificar que non existe altere ligamines al patronos ante que tu los dele.',
'unusedtemplateswlh'  => 'altere ligamines',

# Random page
'randompage'         => 'Pagina aleatori',
'randompage-nopages' => 'Il non ha paginas in iste spatio de nomines.',

# Random redirect
'randomredirect'         => 'Redirection aleatori',
'randomredirect-nopages' => 'Il non ha redirectiones in iste spatio de nomines.',

# Statistics
'statistics'             => 'Statisticas',
'sitestats'              => 'Statisticas de accesso',
'userstats'              => 'Statisticas de usatores',
'sitestatstext'          => "Le base de datos contine un total de {{PLURAL:\$1|'''1''' pagina|'''\$1''' paginas}}.
Iste numero include paginas de \"discussion\", paginas super {{SITENAME}}, \"peciettas\"
minimal, redirectiones, e altere paginas que probabilemente non se qualifica como articulos.
Excludente {{PLURAL:\$1|iste|istes}}, il remane {{PLURAL:\$2|'''1''' pagina|'''\$2''' paginas}} que probabilemente es
{{PLURAL:\$2|un articulo|articulos}} legitime.

'''\$8''' {{PLURAL:\$8|file|files}} ha essite cargate.

Il habeva un total de '''\$3''' {{PLURAL:\$3|visita a un pagina|visitas a paginas}}, e '''\$4''' {{PLURAL:\$4|modification de un pagina|modificationes de paginas}}
desde le establimento de {{SITENAME}}.
Isto representa un media de '''\$5''' modificationes per pagina, e '''\$6''' visitas per modification.

Le longor del [http://www.mediawiki.org/wiki/Manual:Job_queue cauda de actiones] es '''\$7'''.",
'userstatstext'          => "Il ha {{PLURAL:$1|'''1''' [[Special:ListUsers|usator]]|'''$1''' [[Special:ListUsers|usatores]]}} registrate, del quales '''$2''' (i.e. '''$4%''') ha le derectos de $5.",
'statistics-mostpopular' => 'Le paginas plus visitate',

'disambiguations'      => 'Paginas de disambiguation',
'disambiguationspage'  => 'Template:Disambiguation',
'disambiguations-text' => "Le sequente paginas ha ligamines a un '''pagina de disambiguation'''.
Istes deberea esser reimplaciate con ligamines al topicos appropriate.<br />
Un pagina se tracta como pagina de disambiguation si illo usa un patrono al qual [[MediaWiki:Disambiguationspage]] ha un ligamine.",

'doubleredirects'            => 'Redirectiones duple',
'doubleredirectstext'        => 'Iste pagina lista paginas de redirection verso altere paginas de redirection.
Cata linea contine ligamines al prime e al secunde redirection, con le destination del secunde redirection, le qual es normalmente un "ver" pagina de destination, verso le qual le prime redirection deberea punctar.',
'double-redirect-fixed-move' => '[[$1]] ha essite renominate, illo es ora un redirection verso [[$2]]',
'double-redirect-fixer'      => 'Corrector de redirectiones',

'brokenredirects'        => 'Redirectiones rupte',
'brokenredirectstext'    => 'Le redirectiones sequente se liga verso articulos inexistente.',
'brokenredirects-edit'   => '(modificar)',
'brokenredirects-delete' => '(deler)',

'withoutinterwiki'         => 'Paginas sin ligamines de linguas',
'withoutinterwiki-summary' => 'Le sequente paginas non ha ligamines a versiones in altere linguas:',
'withoutinterwiki-legend'  => 'Prefixo',
'withoutinterwiki-submit'  => 'Revelar',

'fewestrevisions' => 'Paginas le minus modificate',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|byte|bytes}}',
'ncategories'             => '$1 {{PLURAL:$1|categoria|categorias}}',
'nlinks'                  => '$1 {{PLURAL:$1|ligamine|ligamines}}',
'nmembers'                => '$1 {{PLURAL:$1|membro|membros}}',
'nrevisions'              => '$1 {{PLURAL:$1|revision|revisiones}}',
'nviews'                  => '$1 {{PLURAL:$1|visita|visitas}}',
'specialpage-empty'       => 'Il non ha resultatos pro iste reporto.',
'lonelypages'             => 'Paginas orphanate',
'lonelypagestext'         => 'Nulle pagina in {{SITENAME}} ha ligamines al paginas sequente.',
'uncategorizedpages'      => 'Paginas non classificate',
'uncategorizedcategories' => 'Categorias non classificate',
'uncategorizedimages'     => 'Files non categorisate',
'uncategorizedtemplates'  => 'Patronos non classificate',
'unusedcategories'        => 'Categorias non usate',
'unusedimages'            => 'Imagines non usate',
'popularpages'            => 'Paginas popular',
'wantedcategories'        => 'Categorias plus demandate',
'wantedpages'             => 'Paginas plus demandate',
'missingfiles'            => 'Files mancante',
'mostlinked'              => 'Paginas le plus ligate',
'mostlinkedcategories'    => 'Categorias le plus ligate',
'mostlinkedtemplates'     => 'Patronos le plus utilisate',
'mostcategories'          => 'Paginas con le plus categorias',
'mostimages'              => 'Files le plus utilisate',
'mostrevisions'           => 'Paginas le plus modificate',
'prefixindex'             => 'Indice de prefixos',
'shortpages'              => 'Paginas curte',
'longpages'               => 'Paginas longe',
'deadendpages'            => 'Paginas sin exito',
'deadendpagestext'        => 'Le sequente paginas non ha ligamines a altere paginas in {{SITENAME}}.',
'protectedpages'          => 'Paginas protegite',
'protectedpages-indef'    => 'Solmente protectiones infinite',
'protectedpagestext'      => 'Le sequente paginas es protegite de esser renominate o modificate',
'protectedpagesempty'     => 'Nulle paginas es actualmente protegite con iste parametros.',
'protectedtitles'         => 'Titulos protegite',
'protectedtitlestext'     => 'Le sequente titulos es protegite de esser create',
'protectedtitlesempty'    => 'Nulle titulos es actualmente protegite con iste parametros.',
'listusers'               => 'Lista de usatores',
'newpages'                => 'Nove paginas',
'newpages-username'       => 'Nomine de usator:',
'ancientpages'            => 'Paginas le plus ancian',
'move'                    => 'Renominar',
'movethispage'            => 'Renominar iste pagina',
'unusedimagestext'        => 'Per favor nota que altere sitos web pote ligar se a un file con un adresse URL directe. Ergo, tal files pote figurar hic malgrado esser in uso active.',
'unusedcategoriestext'    => 'Le sequente paginas de categoria existe ben que nulle altere pagina o categoria los utilisa.',
'notargettitle'           => 'Sin scopo',
'notargettext'            => 'Tu non ha specificate un pagina o usator super le qual
executar iste function.',
'nopagetitle'             => 'Le pagina de destination non existe',
'nopagetext'              => 'Le pagina de destination que tu ha specificate non existe.',
'pager-newer-n'           => '{{PLURAL:$1|1 plus recente|$1 plus recentes}}',
'pager-older-n'           => '{{PLURAL:$1|1 minus recente|$1 minus recentes}}',
'suppress'                => 'Supervisor',

# Book sources
'booksources'               => 'Fontes de libros',
'booksources-search-legend' => 'Cercar fontes de libros',
'booksources-go'            => 'Ir',
'booksources-text'          => 'Infra es un lista de ligamines a altere sitos que vende libros nove e usate, e pote etiam haber altere informationes super libros que tu cerca:',

# Special:Log
'specialloguserlabel'  => 'Usator:',
'speciallogtitlelabel' => 'Titulo:',
'log'                  => 'Registros',
'all-logs-page'        => 'Tote le registros',
'log-search-legend'    => 'Cercar registros',
'log-search-submit'    => 'Ir',
'alllogstext'          => 'Presentation combinate de tote le registros disponibile de {{SITENAME}}.
Pro restringer le presentation, selige un typo de registro, le nomine de usator (sensibile al majusculas e minusculas), o le pagina in question (etiam sensibile al majusculas e minusculas).',
'logempty'             => 'Le registro contine nihil pro iste pagina.',
'log-title-wildcard'   => 'Cercar titulos que comencia con iste texto',

# Special:AllPages
'allpages'          => 'Tote le paginas',
'alphaindexline'    => '$1 a $2',
'nextpage'          => 'Sequente pagina ($1)',
'prevpage'          => 'Precedente pagina ($1)',
'allpagesfrom'      => 'Monstrar le paginas a partir de:',
'allarticles'       => 'Tote le paginas',
'allinnamespace'    => 'Tote le paginas (del spatio de nomines $1)',
'allnotinnamespace' => 'Tote le paginas (non in le spatio de nomines $1)',
'allpagesprev'      => 'Previe',
'allpagesnext'      => 'Sequente',
'allpagessubmit'    => 'Ir',
'allpagesprefix'    => 'Monstrar le paginas con prefixo:',
'allpagesbadtitle'  => 'Le titulo de pagina date esseva invalide o habeva un prefixo interlingual o interwiki.
Es possibile que illo contine un o plus characteres que non pote esser usate in titulos.',
'allpages-bad-ns'   => '{{SITENAME}} non ha un spatio e nomines "$1".',

# Special:Categories
'categories'                    => 'Categorias',
'categoriespagetext'            => 'Le sequente categorias contine paginas o media.
Le [[Special:UnusedCategories|categorias non usate]] non se monstra hic.
Vide etiam le [[Special:WantedCategories|categorias desirate]].',
'categoriesfrom'                => 'Monstrar categorias a partir de:',
'special-categories-sort-count' => 'ordinar per numero',
'special-categories-sort-abc'   => 'ordinar alphabeticamente',

# Special:ListUsers
'listusersfrom'      => 'Monstrar usatores a partir de:',
'listusers-submit'   => 'Revelar',
'listusers-noresult' => 'Nulle usator trovate.',

# Special:ListGroupRights
'listgrouprights'          => 'Derectos del gruppos de usatores',
'listgrouprights-summary'  => 'Lo sequente es un lista de gruppos de usatores definite in iste wiki, con lor derectos de accesso associate.
Il pote haber [[{{MediaWiki:Listgrouprights-helppage}}|informationes additional]] super derectos individual.',
'listgrouprights-group'    => 'Gruppo',
'listgrouprights-rights'   => 'Derectos',
'listgrouprights-helppage' => 'Help:Derectos de gruppos',
'listgrouprights-members'  => '(lista de membros)',

# E-mail user
'mailnologin'     => 'Necun adresse de invio',
'mailnologintext' => 'Tu debe [[Special:UserLogin|aperir un session]]
e haber un adresse de e-mail valide in tu [[Special:Preferences|preferentias]]
pro inviar e-mail a altere usatores.',
'emailuser'       => 'Inviar e-mail a iste usator',
'emailpage'       => 'Inviar e-mail al usator',
'emailpagetext'   => 'Si iste usator forniva un adresse de e-mail valide in su preferentias de usator, le formulario infra le/la inviara un singule message.
Le adresse de e-mail que tu forniva in [[Special:Preferences|tu preferentias de usator]] apparera
como le adresse del expeditor del e-mail, a fin que le destinatario pote responder directemente a te.',
'usermailererror' => 'Le objecto de e-mail retornava le error:',
'defemailsubject' => 'E-mail de {{SITENAME}}',
'noemailtitle'    => 'Nulle adresse de e-mail',
'noemailtext'     => 'Iste usator non ha specificate un adresse de e-mail valide,
o ha optate pro non reciper e-mail de altere usatores.',
'emailfrom'       => 'Expeditor:',
'emailto'         => 'Destinatario:',
'emailsubject'    => 'Subjecto:',
'emailmessage'    => 'Message:',
'emailsend'       => 'Inviar',
'emailccme'       => 'Inviar me un copia de mi message.',
'emailccsubject'  => 'Copia de tu message a $1: $2',
'emailsent'       => 'E-mail inviate',
'emailsenttext'   => 'Tu message de e-mail ha essite inviate.',
'emailuserfooter' => 'Iste e-mail esseva inviate per $1 a $2 con le function "Inviar e-mail al usator" a {{SITENAME}}.',

# Watchlist
'watchlist'            => 'Mi observatorio',
'mywatchlist'          => 'Mi observatorio',
'watchlistfor'         => "(pro '''$1''')",
'nowatchlist'          => 'Tu non ha paginas sub observation.',
'watchlistanontext'    => 'Tu debe $1 pro poter vider o modificar entratas in tu observatorio.',
'watchnologin'         => 'Tu non ha aperite un session',
'watchnologintext'     => 'Tu debe [[Special:UserLogin|aperir un session]] pro modificar tu observatorio.',
'addedwatch'           => 'Addite al observatorio',
'addedwatchtext'       => "Le pagina \"<nowiki>\$1</nowiki>\" es ora in tu [[Special:Watchlist|observatorio]].
Omne modificationes futur a iste pagina e su pagina de discussion associate essera listate ibi,
e le pagina apparera '''in litteras grasse''' in le [[Special:RecentChanges|lista de modificationes recente]] pro
facilitar su identification.",
'removedwatch'         => 'Eliminate del observatorio',
'removedwatchtext'     => 'Le pagina "<nowiki>$1</nowiki>" non es plus sub observation.',
'watch'                => 'Observar',
'watchthispage'        => 'Observar iste pagina',
'unwatch'              => 'Disobservar',
'unwatchthispage'      => 'Cancellar observation',
'notanarticle'         => 'Non es un articulo',
'notvisiblerev'        => 'Le revision ha essite delite',
'watchnochange'        => 'Nulle articulo que tu observa esseva modificate durante le periodo de tempore indicate.',
'watchlist-details'    => '{{PLURAL:$1|$1 pagina|$1 paginas}} es in tu observatorio, sin contar le paginas de discussion.',
'wlheader-enotif'      => '* Le notificationes via e-mail es active.',
'wlheader-showupdated' => "* Le paginas que ha essite modificate post tu ultime visita se monstra in litteras '''grasse'''",
'watchmethod-recent'   => 'cerca paginas sub observation in modificationes recente',
'watchmethod-list'     => 'cerca modificationes recente in paginas sub observation',
'watchlistcontains'    => 'Tu observatorio contine $1 {{PLURAL:$1|pagina|paginas}}.',
'iteminvalidname'      => "Problema con entrata '$1', nomine invalide...",
'wlnote'               => "Infra es le ultime {{PLURAL:$1|modification|'''$1''' modificationes}} durante le ultime {{PLURAL:$2|hora|'''$2''' horas}}.",
'wlshowlast'           => 'Revelar ultime $1 horas $2 dies $3',
'watchlist-show-bots'  => 'Monstrar modificationes per bots',
'watchlist-hide-bots'  => 'Celar modificationes per bots',
'watchlist-show-own'   => 'Monstrar mi modificationes',
'watchlist-hide-own'   => 'Celar mi modificationes',
'watchlist-show-minor' => 'Monstrar modificationes minor',
'watchlist-hide-minor' => 'Celar modificationes minor',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Observation in curso...',
'unwatching' => 'Disobservation in curso...',

'enotif_mailer'                => 'Systema de notification via e-mail de {{SITENAME}}',
'enotif_reset'                 => 'Marcar tote le paginas como visitate',
'enotif_newpagetext'           => 'Isto es un nove pagina.',
'enotif_impersonal_salutation' => 'Usator de {{SITENAME}}',
'changed'                      => 'modificate',
'created'                      => 'create',
'enotif_subject'               => 'Le pagina $PAGETITLE de {{SITENAME}} ha essite $CHANGEDORCREATED per $PAGEEDITOR',
'enotif_lastvisited'           => 'Vide $1 pro tote le modificationes depost tu ultime visita.',
'enotif_lastdiff'              => 'Vide $1 pro revider iste modification.',
'enotif_anon_editor'           => 'usator anonyme $1',
'enotif_body'                  => 'Car $WATCHINGUSERNAME,


Le pagina de {{SITENAME}} titulate $PAGETITLE ha essite $CHANGEDORCREATED le $PAGEEDITDATE per $PAGEEDITOR. Vide $PAGETITLE_URL pro le version actual.

$NEWPAGE

Summario del redactor: $PAGESUMMARY $PAGEMINOREDIT

Contactar le redactor:
e-mail: $PAGEEDITOR_EMAIL
wiki: $PAGEEDITOR_WIKI

Si tu non visita iste pagina, tu non recipera altere notificationes in caso de modificationes ulterior.
Como alternativa tu pote reinitialisar le optiones de notification pro tote le paginas in tu observatorio.

             Le systema de notification de {{SITENAME}}, a tu servicio

--
Pro cambiar le configuration de tu observatorio, visita
{{fullurl:{{ns:special}}:Watchlist/edit}}

Reactiones e ulterior assistentia:
{{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => 'Deler pagina',
'confirm'                     => 'Confirmar',
'excontent'                   => "contento esseva: '$1'",
'excontentauthor'             => "contento esseva: '$1' (e le sol contributor esseva '[[Special:Contributions/$2|$2]]')",
'exbeforeblank'               => "contento ante radimento esseva: '$1'",
'exblank'                     => 'pagina esseva vacue',
'delete-confirm'              => 'Deler "$1"',
'delete-legend'               => 'Deler',
'historywarning'              => 'Attention: Le pagina que tu va deler ha un historia:',
'confirmdeletetext'           => 'Tu va deler un pagina con tote su historia.
Per favor confirma que tu intende facer isto, que tu comprende le consequentias, e que tu face isto in accordo con [[{{MediaWiki:Policy-url}}|le politicas]].',
'actioncomplete'              => 'Action complete',
'deletedtext'                 => '"<nowiki>$1</nowiki>" ha essite delite.
Vide $2 pro un registro de deletiones recente.',
'deletedarticle'              => 'deleva "[[$1]]"',
'suppressedarticle'           => 'supprimeva "[[$1]]"',
'dellogpage'                  => 'Registro de deletiones',
'dellogpagetext'              => 'Infra es un lista del plus recente deletiones.
Tote le horas es in le fuso horari del servitor.',
'deletionlog'                 => 'registro de deletiones',
'reverted'                    => 'Revertite a revision anterior',
'deletecomment'               => 'Motivo pro deletion:',
'deleteotherreason'           => 'Motivo altere/additional:',
'deletereasonotherlist'       => 'Altere motivo',
'deletereason-dropdown'       => '*Motivos habitual pro deler paginas
** Requesta del autor
** Violation de copyright
** Vandalismo',
'delete-edit-reasonlist'      => 'Modificar le motivos pro deletion',
'delete-toobig'               => 'Iste pagina ha un grande historia de modificationes con plus de $1 {{PLURAL:$1|revision|revisiones}}.
Le deletion de tal paginas ha essite restringite pro impedir le disruption accidental de {{SITENAME}}.',
'delete-warning-toobig'       => 'Iste pagina ha un grande historia de modificationes con plus de $1 {{PLURAL:$1|revision|revisiones}}.
Le deletion de illo pote disrumper le operationes del base de datos de {{SITENAME}};
procede con caution.',
'rollback'                    => 'Revocar modificationes',
'rollback_short'              => 'Revocar',
'rollbacklink'                => 'revocar',
'rollbackfailed'              => 'Revocation fallite',
'cantrollback'                => 'Impossibile revocar le modification;
le ultime contributor es le sol autor de iste pagina.',
'alreadyrolled'               => 'Non pote revocar le ultime modification de [[:$1]] per [[User:$2|$2]] ([[User talk:$2|discussion]] | [[Special:Contributions/$2|{{int:contribslink}}]]);
un altere persona ha ja modificate o revocate le pagina.

Le ultime modification esseva facite per [[User:$3|$3]] ([[User talk:$3|discussion]] | [[Special:Contributions/$3|{{int:contribslink}}]]).',
'editcomment'                 => 'Le commento del modification esseva: "<i>$1</i>".', # only shown if there is an edit comment
'revertpage'                  => 'Reverteva modificationes per [[Special:Contributions/$2|$2]] ([[User talk:$2|Discussion]]) al ultime version per [[User:$1|$1]]', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => 'Revocava modificationes per $1;
retornava al version per $2.',
'sessionfailure'              => 'Il pare haber un problema con tu session de conto;
iste action ha essite cancellate como precaution contra le sequestramento de sessiones.
Per favor preme "retro" e recarga le pagina de ubi tu ha venite, postea reprova.',
'protectlogpage'              => 'Registro de protectiones',
'protectlogtext'              => 'Infra es un lista de protection e disprotection de paginas.
Vide le [[Special:ProtectedPages|lista de paginas protegite]] pro le lista de protectiones de paginas actualmente in operation.',
'protectedarticle'            => 'protegeva "[[$1]]"',
'modifiedarticleprotection'   => 'cambiava nivello de protection de "[[$1]]"',
'unprotectedarticle'          => 'disprotegeva "[[$1]]"',
'protect-title'               => 'Cambiar nivello de protection de "$1"',
'protect-legend'              => 'Confirmar protection',
'protectcomment'              => 'Commento:',
'protectexpiry'               => 'Expira:',
'protect_expiry_invalid'      => 'Le tempore de expiration es invalide.',
'protect_expiry_old'          => 'Le tempore de expiration es in le passato.',
'protect-unchain'             => 'Disserrar permissiones de renomination',
'protect-text'                => 'Tu pote vider e cambiar hic le nivello de protection del pagina <strong><nowiki>$1</nowiki></strong>.',
'protect-locked-blocked'      => 'Tu non pote cambiar le nivellos de protection durante que tu es blocate.
Ecce le configurationes actual del pagina <strong>$1</strong>:',
'protect-locked-dblock'       => 'Le nivellos de protection non pote esser cambiate proque es active un blocada del base de datos.
Ecce le configurationes actual del pagina <strong>$1</strong>:',
'protect-locked-access'       => 'Tu conto non ha permission a cambiar le nivellos de protection de paginas.
Ecce le configurationes actual del pagina <strong>$1</strong>:',
'protect-cascadeon'           => 'Iste pagina es actualmente protegite proque es includite in le sequente {{PLURAL:$1|pagina, le qual|paginas, le quales}} ha activate le protection in cascada.
Tu pote cambiar le nivello de protection de iste pagina, sed isto non cambiara le effecto del protection in cascada.',
'protect-default'             => '(predefinition)',
'protect-fallback'            => 'Requirer permission de "$1"',
'protect-level-autoconfirmed' => 'Blocar usatores non registrate',
'protect-level-sysop'         => 'Administratores solmente',
'protect-summary-cascade'     => 'in cascada',
'protect-expiring'            => 'expira le $1 (UTC)',
'protect-cascade'             => 'Proteger le paginas includite in iste pagina (protection in cascada)',
'protect-cantedit'            => 'Tu non pote cambiar le nivellos de protection de iste pagina, proque tu non ha le autorisation de modificar le pagina.',
'restriction-type'            => 'Permission:',
'restriction-level'           => 'Nivello de restriction:',
'minimum-size'                => 'Grandor minime',
'maximum-size'                => 'Grandor maxime:',
'pagesize'                    => '(bytes)',

# Restrictions (nouns)
'restriction-edit'   => 'Modificar',
'restriction-move'   => 'Renominar',
'restriction-create' => 'Crear',
'restriction-upload' => 'Cargar',

# Restriction levels
'restriction-level-sysop'         => 'completemente protegite',
'restriction-level-autoconfirmed' => 'semiprotegite',
'restriction-level-all'           => 'omne nivello',

# Undelete
'undelete'                     => 'Vider paginas delite',
'undeletepage'                 => 'Vider e restaurar paginas delite',
'undeletepagetitle'            => "'''Lo sequente consiste de revisiones delite de [[:$1|$1]]'''.",
'viewdeletedpage'              => 'Vider paginas delite',
'undeletepagetext'             => 'Le paginas sequente ha essite delite, sed es ancora in le archivo e pote esser restaurate.
Le archivo pote esser vacuate periodicamente.',
'undelete-fieldset-title'      => 'Restaurar revisiones',
'undeleteextrahelp'            => "Pro restaurar le historia integre del pagina, lassa tote le quadratos dismarcate e clicca '''''Restaurar'''''.
Pro executar un restauration selective, marca le quadratos correspondente al versiones pro restaurar, e clicca '''''Restaurar'''''.
Le button '''''Reinitiar''''' face rader le campo de commento e tote le quadratos.",
'undeleterevisions'            => '$1 {{PLURAL:$1|revision|revisiones}} archivate',
'undeletehistory'              => 'Si tu restaura un pagina, tote le revisiones essera restaurate al historia.
Si un nove pagina con le mesme nomine ha essite create post le deletion, le revisiones
restaurate apparera in le historia anterior.',
'undeleterevdel'               => 'Le restauration non essera executate si illo resultara in le deletion partial del revision le plus recente del pagina o del file.
In tal casos, tu debe dismarcar o revelar le revision delite le plus recente.',
'undeletehistorynoadmin'       => 'Iste pagina ha essite delite.
Le motivo del deletion se monstra in le summario infra, con le detalios del usatores que habeva modificate iste pagina ante le deletion.
Le texto complete de iste revisiones delite es solmente disponibile al administratores.',
'undelete-revision'            => 'Revision delite del pagina $1 (facite le $2) per $3:',
'undeleterevision-missing'     => 'Revision invalide o mancante.
Es possibile que le adresse URL es invalide, o que le revision ha essite restaurate o eliminate del archivo.',
'undelete-nodiff'              => 'Nulle revision precedente trovate.',
'undeletebtn'                  => 'Restaurar',
'undeletelink'                 => 'restaurar',
'undeletereset'                => 'Reinitiar',
'undeletecomment'              => 'Commento:',
'undeletedarticle'             => 'restaurava "[[$1]]"',
'undeletedrevisions'           => '{{PLURAL:$1|1 revision|$1 revisiones}} restaurate',
'undeletedrevisions-files'     => '{{PLURAL:$1|1 revision|$1 revisiones}} e {{PLURAL:$2|1 file|$2 files}} restaurate',
'undeletedfiles'               => '$1 {{PLURAL:$1|archivo|archivos}} restaurate',
'cannotundelete'               => 'Le restauration ha fallite;
es possibile que un altere persona ha ja restaurate le pagina.',
'undeletedpage'                => "<big>'''$1 ha essite restaurate'''</big>

Consulta le [[Special:Log/delete|registro de deletiones]] pro un lista de deletiones e restaurationes recente.",
'undelete-header'              => 'Vide [[Special:Log/delete|le registro de deletiones]] pro un lista de paginas recentemente delite.',
'undelete-search-box'          => 'Cercar paginas delite',
'undelete-search-prefix'       => 'Monstrar paginas que comencia con:',
'undelete-search-submit'       => 'Cercar',
'undelete-no-results'          => 'Nulle paginas correspondente trovate in le archivo de deletiones.',
'undelete-filename-mismatch'   => 'Non pote restaurar le revision del file con data e hora $1: le nomine del file non corresponde',
'undelete-bad-store-key'       => 'Non pote restaurar le revision del file con data e hora $1: le file mancava ja ante le deletion.',
'undelete-cleanup-error'       => 'Error durante le deletion del file de archivo non usate "$1".',
'undelete-missing-filearchive' => 'Impossibile restaurar le file con ID de archvo $1 proque illo non es presente in le base de datos.
Es possibile que illo ha ja essite restaurate.',
'undelete-error-short'         => 'Error durante le restauration del file: $1',
'undelete-error-long'          => 'Se incontrava errores durante le restauration del file:

$1',

# Namespace form on various pages
'namespace'      => 'Spatio de nomine:',
'invert'         => 'Inverter selection',
'blanknamespace' => '(Principal)',

# Contributions
'contributions' => 'Contributiones del usator',
'mycontris'     => 'Mi contributiones',
'contribsub2'   => 'Pro $1 ($2)',
'nocontribs'    => 'Necun modification ha essite trovate secundo iste criterios.',
'uctop'         => '(ultime)',
'month'         => 'A partir del mense (e anterior):',
'year'          => 'A partir del anno (e anterior):',

'sp-contributions-newbies'     => 'Monstrar contributiones de nove contos solmente',
'sp-contributions-newbies-sub' => 'Pro nove contos',
'sp-contributions-blocklog'    => 'Registro de blocadas',
'sp-contributions-search'      => 'Cercar contributiones',
'sp-contributions-username'    => 'Adresse IP o nomine de usator:',
'sp-contributions-submit'      => 'Cercar',

# What links here
'whatlinkshere'            => 'Referentias a iste pagina',
'whatlinkshere-title'      => 'Paginas con ligamines verso $1',
'whatlinkshere-page'       => 'Pagina:',
'linklistsub'              => '(Lista de ligamines)',
'linkshere'                => "Le paginas sequente se liga a '''[[:$1]]''':",
'nolinkshere'              => "Necun pagina se liga a '''[[:$1]]'''.",
'nolinkshere-ns'           => "Nulle pagina liga a '''[[:$1]]''' in le spatio de nomines seligite.",
'isredirect'               => 'pagina de redirection',
'istemplate'               => 'inclusion',
'isimage'                  => 'ligamine verso un imagine',
'whatlinkshere-prev'       => '{{PLURAL:$1|precedente|precedente $1}}',
'whatlinkshere-next'       => '{{PLURAL:$1|sequente|sequente $1}}',
'whatlinkshere-links'      => '← ligamines',
'whatlinkshere-hideredirs' => '$1 redirectiones',
'whatlinkshere-hidetrans'  => '$1 transclusiones',
'whatlinkshere-hidelinks'  => '$1 ligamines',
'whatlinkshere-hideimages' => '$1 ligamines verso imagines',
'whatlinkshere-filters'    => 'Filtros',

# Block/unblock
'blockip'                         => 'Blocar usator',
'blockip-legend'                  => 'Blocar usator',
'blockiptext'                     => 'Usa le formulario infra pro blocar le accesso de scriptura
a partir de un adresse IP specific.
Isto debe esser facite solmente pro impedir vandalismo, e de
accordo con le [[{{MediaWiki:Policy-url}}|politica de {{SITENAME}}]].
Scribe un motivo specific infra (per exemplo, citante paginas
specific que ha essite vandalisate).',
'ipaddress'                       => 'Adresse IP:',
'ipadressorusername'              => 'Adresse IP o nomine de usator:',
'ipbexpiry'                       => 'Expiration:',
'ipbreason'                       => 'Motivo:',
'ipbreasonotherlist'              => 'Altere motivo',
'ipbreason-dropdown'              => "*Motivos frequente pro blocar
** Insertion de informationes false
** Elimination de contento de paginas
** Ligamines ''spam'' verso sitos externe
** Insertion de nonsenso/absurditates in paginas
** Comportamento intimidatori/molestation
** Abuso de contos multiple
** Nomine de usator inacceptabile",
'ipbanononly'                     => 'Blocar solmente usatores anonyme',
'ipbcreateaccount'                => 'Impedir creation de contos',
'ipbemailban'                     => 'Impedir que le usator invia e-mail',
'ipbenableautoblock'              => 'Blocar automaticamente le adresse IP usate le plus recentemente per iste usator, e omne IPs successive desde le quales ille/-a tenta facer modificationes',
'ipbsubmit'                       => 'Blocar iste adresse',
'ipbother'                        => 'Altere tempore:',
'ipboptions'                      => '2 horas:2 hours,1 die:1 day,3 dies:3 days,1 septimana:1 week,2 septimanas:2 weeks,1 mense:1 month,3 menses:3 months,6 menses:6 months,1 anno:1 year,infinite:infinite', # display1:time1,display2:time2,...
'ipbotheroption'                  => 'altere',
'ipbotherreason'                  => 'Motivo altere/additional:',
'ipbhidename'                     => 'Celar le nomine del usator del registro de blodadas, del lista de blocadas active e del lista de usatores',
'ipbwatchuser'                    => 'Observar le paginas de usator e de discussion de iste usator',
'badipaddress'                    => 'Adresse IP mal formate.',
'blockipsuccesssub'               => 'Blocada succedite',
'blockipsuccesstext'              => 'Le adresse IP "$1" ha essite blocate.
<br />Vide [[Special:IPBlockList|Lista de IPs blocate]] pro revider le blocadas.',
'ipb-edit-dropdown'               => 'Modificar le motivos pro blocar',
'ipb-unblock-addr'                => 'Disblocar $1',
'ipb-unblock'                     => 'Disblocar un nomine de usator o un adresse IP',
'ipb-blocklist-addr'              => 'Vider blocadas existente pro $1',
'ipb-blocklist'                   => 'Vider blocadas existente',
'unblockip'                       => 'Disblocar adresse IP',
'unblockiptext'                   => 'Usa le formulario infra pro restaurar le accesso de scriptura
a un adresse IP blocate previemente.',
'ipusubmit'                       => 'Disblocar iste adresse',
'unblocked'                       => '[[User:$1|$1]] ha essite disblocate',
'unblocked-id'                    => 'Le blocada $1 ha essite eliminate',
'ipblocklist'                     => 'Adresses IP e nomines de usator blocate',
'ipblocklist-legend'              => 'Cercar un usator blocate',
'ipblocklist-username'            => 'Nomine de usator o adresse IP:',
'ipblocklist-submit'              => 'Cercar',
'blocklistline'                   => '$1, $2 blocava $3 ($4)',
'infiniteblock'                   => 'infinite',
'expiringblock'                   => 'expira le $1',
'anononlyblock'                   => 'anon. solmente',
'noautoblockblock'                => 'autoblocadas disactivate',
'createaccountblock'              => 'creation de contos blocate',
'emailblock'                      => 'e-mail blocate',
'ipblocklist-empty'               => 'Le lista de blocadas es vacue.',
'ipblocklist-no-results'          => 'Le adresse IP o nomine de usator que tu requestava non es blocate.',
'blocklink'                       => 'blocar',
'unblocklink'                     => 'disblocar',
'contribslink'                    => 'contributiones',
'autoblocker'                     => 'Autoblocate proque tu adresse IP ha recentemente essite usate per "[[User:$1|$1]]".
Le ration date pro le blocada de $1 es: "$2"',
'blocklogpage'                    => 'Registro de blocadas',
'blocklogentry'                   => 'blocava [[$1]] con un tempore de expiration de $2 $3',
'blocklogtext'                    => 'Isto es un registro de blocadas e disblocadas de usatores.
Le adresses IP automaticamente blocate non es includite.
Vide le [[Special:IPBlockList|lista de blocadas IP]] pro le lista de bannimentos e blocadas actualmente in operation.',
'unblocklogentry'                 => 'disblocava $1',
'block-log-flags-anononly'        => 'usatores anonyme solmente',
'block-log-flags-nocreate'        => 'creation de contos disactivate',
'block-log-flags-noautoblock'     => 'autoblocadas disactivate',
'block-log-flags-noemail'         => 'e-mail blocate',
'block-log-flags-angry-autoblock' => 'autoblocadas avantiate activate',
'range_block_disabled'            => 'Le capacitate del administratores a blocar intervallos de adresses IP es disactivate.',
'ipb_expiry_invalid'              => 'Tempore de expiration invalide.',
'ipb_expiry_temp'                 => 'Le blocadas de nomines de usator celate debe esser permanente.',
'ipb_already_blocked'             => '"$1" es ja blocate',
'ipb_cant_unblock'                => 'Error: ID de blocada $1 non trovate. Es possibile que illo ha ja essite disblocate.',
'ipb_blocked_as_range'            => 'Error: Le IP $1 non es blocate directemente e non pote esser disblocate.
Illo es, nonobstante, blocate como parte del intervallo $2, le qual pote esser disblocate.',
'ip_range_invalid'                => 'Intervallo de adresses IP invalide.',
'blockme'                         => 'Blocar me',
'proxyblocker'                    => 'Blocator de proxy',
'proxyblocker-disabled'           => 'Iste function is disactivate.',
'proxyblockreason'                => 'Tu adresse IP ha essite blocate proque illo es un proxy aperte.
Per favor contacta tu providitor de servicio internet o supporto technic e informa les de iste problema grave de securitate.',
'proxyblocksuccess'               => 'Succedite.',
'sorbsreason'                     => 'Tu adresse IP es listate como proxy aperte in le DNSBL usate per {{SITENAME}}.',
'sorbs_create_account_reason'     => 'Tu adresse IP es listate como proxy aperte in le DNSBL usate per {{SITENAME}}.
Tu non pote crear un conto',

# Developer tools
'lockdb'              => 'Blocar base de datos',
'unlockdb'            => 'Disblocar base de datos',
'lockdbtext'          => 'Le blocada del base de datos suspendera le capacitate de tote
le usatores de modificar paginas, modificar lor preferentias e observatorios,
e altere actiones que require modificationes in le base de datos.
Per favor confirma que isto es tu intention, e que tu disblocara le
base de datos immediatemente post completar tu mantenentia.',
'unlockdbtext'        => 'Le disblocada del base de datos restaurara le capacitate de tote
le usatores de modificar paginas, modificar lor preferentias e observatorios,
e altere actiones que require modificationes in le base de datos.
Per favor confirma que isto es tu intention.',
'lockconfirm'         => 'Si, io realmente vole blocar le base de datos.',
'unlockconfirm'       => 'Si, io realmente vole disblocar le base de datos.',
'lockbtn'             => 'Blocar base de datos',
'unlockbtn'           => 'Disblocar base de datos',
'locknoconfirm'       => 'Tu non ha marcate le quadrato de confirmation.',
'lockdbsuccesssub'    => 'Base de datos blocate con successo',
'unlockdbsuccesssub'  => 'Base de datos disblocate con successo',
'lockdbsuccesstext'   => 'Le base de datos de {{SITENAME}} ha essite blocate.
<br />Rememora te de disblocar lo post completar tu mantenentia.',
'unlockdbsuccesstext' => 'Le base de datos de {{SITENAME}} ha essite disblocate.',
'lockfilenotwritable' => 'Impossibile scriber al file de blocada del base de datos.
Pro blocar o disblocar le base de datos, le servitor web debe poter scriber a iste file.',
'databasenotlocked'   => 'Le base de datos non es blocate.',

# Move page
'move-page'               => 'Renominar $1',
'move-page-legend'        => 'Renominar pagina',
'movepagetext'            => "Per medio del formulario infra tu pote renominar un pagina, transferente tote su historia al nove nomine.
Le titulo anterior devenira un pagina de redirection verso le nove titulo.
Tu pote actualisar automaticamente le redirectiones que puncta verso le titulo original.
Si tu opta contra facer lo, assecura te de reparar omne redirectiones [[Special:DoubleRedirects|duple]] o [[Special:BrokenRedirects|defecte]].
Tu es responsabile pro assecurar que le ligamines continua a punctar verso ubi illos deberea.

Nota que le pagina '''non''' essera renominate si existe ja un pagina sub le nove titulo, salvo si illo es vacue o un redirection e non ha un historia de modificationes passate.
Isto significa que tu pote renominar un pagina a su titulo original si tu lo ha renominate per error, e que tu non pote superscriber un pagina existente.

'''ATTENTION!'''
Isto pote esser un cambio drastic e inexpectate pro un pagina popular;
per favor assecura te que tu comprende le consequentias de isto ante que tu procede.",
'movepagetalktext'        => "Le pagina de discussion associate essera automaticamente renominate conjunctemente con illo '''a minus que''':
*Un pagina de discussion non vacue ja existe sub le nove nomine, o
*Tu dismarca le quadrato infra.

Il tal casos, tu debera renominar o fusionar le pagina manualmente si desirate.",
'movearticle'             => 'Renominar pagina:',
'movenotallowed'          => 'Tu non ha le permission de renominar paginas.',
'newtitle'                => 'Al nove titulo:',
'move-watch'              => 'Observar iste pagina',
'movepagebtn'             => 'Renominar pagina',
'pagemovedsub'            => 'Renomination succedite',
'movepage-moved'          => '<big>\'\'\'"$1" ha essite renominate a "$2"\'\'\'</big>', # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => 'Un pagina con iste nomine ja existe, o le nomine seligite non es valide.
Per favor selige un altere nomine.',
'cantmove-titleprotected' => 'Tu non pote renominar un pagina a iste titulo, proque le nove titulo ha essite protegite contra creation',
'talkexists'              => "'''Le pagina mesme ha essite renominate con successo, mais le pagina de discussion associate non ha essite renominate proque ja existe un sub le nove titulo.
Per favor fusiona los manualmente.'''",
'movedto'                 => 'renominate a',
'movetalk'                => 'Renominar etiam le pagina de discussion associate',
'move-subpages'           => 'Renominar tote le subpaginas, si applicabile',
'move-talk-subpages'      => 'Renominar tote le subpaginas del pagina de discussion, si applicabile',
'movepage-page-exists'    => 'Le pagina $1 existe ja e non pote esser automaticamente superscribite.',
'movepage-page-moved'     => 'Le pagina $1 ha essite renominate a $2.',
'movepage-page-unmoved'   => 'Le pagina $1 non poteva esser renominate a $2.',
'movepage-max-pages'      => 'Le maximo de $1 {{PLURAL:$1|pagina|paginas}} ha essite renominate e nulle altere pagina pote esser renominate automaticamente.',
'1movedto2'               => 'renominava [[$1]] verso [[$2]]',
'1movedto2_redir'         => 'renominava [[$1]] verso [[$2]] trans redirection',
'movelogpage'             => 'Registro de renominationes',
'movelogpagetext'         => 'Infra es un lista de paginas renominate.',
'movereason'              => 'Motivo:',
'revertmove'              => 'reverter',
'delete_and_move'         => 'Deler e renominar',
'delete_and_move_text'    => '==Deletion requirite==
Le pagina de destination "[[:$1]]" existe ja.
Esque tu vole deler lo pro permitter le renomination?',
'delete_and_move_confirm' => 'Si, deler le pagina',
'delete_and_move_reason'  => 'Delite pro permitter renomination',
'selfmove'                => 'Le titulos de origine e de destination es identic;
non pote renominar un pagina al mesme titulo.',
'immobile_namespace'      => 'Le titulo de origine o de destination es de un typo special;
es impossibile cambiar le spatio de nomines de tal paginas.',
'imagenocrossnamespace'   => 'Non pote renominar file verso un spatio de nomines non-file',
'imagetypemismatch'       => 'Le nove extension del nomine del file non corresponde al typo del file',
'imageinvalidfilename'    => 'Le nomine del file de destination es invalide',
'fix-double-redirects'    => 'Actualisar tote le redirectiones que puncta verso le titulo original',

# Export
'export'            => 'Exportar paginas',
'exporttext'        => 'Tu pote exportar le texto e historia de modificationes de un pagina particular o collection de paginas, incapsulate in un poco de XML.
Isto pote esser importate in un altere wiki que usa MediaWiki via le [[Special:Import|pagina pro importar]].

Pro exportar paginas, entra le titulos in le quadro de texto infra, un titulo per linea, e indica si tu vole haber le version currente con tote le versiones ancian, con le lineas de historia de paginas, o simplemente le version actual con le informationes super le ultime modification.

In le secunde caso tu pote etiam usar un ligamine, p.ex. [[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]] pro le pagina "[[{{MediaWiki:Mainpage}}]]".',
'exportcuronly'     => 'Includer solmente le revision actual, non le historia complete',
'exportnohistory'   => "----
'''Nota:''' Le exportation del historia de paginas complete per medio de iste formulario ha essite disactivate pro motivos concernente le prestationes del servitor.",
'export-submit'     => 'Exportar',
'export-addcattext' => 'Adder paginas del categoria:',
'export-addcat'     => 'Adder',
'export-download'   => 'Immagazinar como file',
'export-templates'  => 'Includer patronos',

# Namespace 8 related
'allmessages'               => 'Messages del systema',
'allmessagesname'           => 'Nomine',
'allmessagesdefault'        => 'Texto predefinite',
'allmessagescurrent'        => 'Texto actual',
'allmessagestext'           => 'Isto es un lista de messages de systema disponibile in le spatio de nomines MediaWiki.
Per favor visita [http://www.mediawiki.org/wiki/Localisation MediaWiki Localisation] e [http://translatewiki.net Betawiki] si tu desira contribuer al localisation general de MediaWiki.',
'allmessagesnotsupportedDB' => "Iste pagina non pote esser usate proque '''\$wgUseDatabaseMessages''' ha essite disactivate.",
'allmessagesfilter'         => 'Filtro de nomine de message:',
'allmessagesmodified'       => 'Monstrar solmente modificates',

# Thumbnails
'thumbnail-more'           => 'Aggrandir',
'filemissing'              => 'File manca',
'thumbnail_error'          => 'Error durante le creation del miniatura: $1',
'djvu_page_error'          => 'Pagina DjVu foras de limite',
'djvu_no_xml'              => 'Impossibile obtener XML pro file DjVu',
'thumbnail_invalid_params' => 'Parametros de miniatura invalide',
'thumbnail_dest_directory' => 'Impossibile crear directorio de destination',

# Special:Import
'import'                     => 'Importar paginas',
'importinterwiki'            => 'Importation transwiki',
'import-interwiki-text'      => 'Selige le wiki e le titulo del pagina a importar.
Le datas del revisiones e nomines del contributores essera preservate.
Tote le actiones de importation transwiki se registra in le [[Special:Log/import|registro de importationes]].',
'import-interwiki-history'   => 'Copiar tote le versiones del historia de iste pagina',
'import-interwiki-submit'    => 'Importar',
'import-interwiki-namespace' => 'Transferer paginas verso le spatio de nomines:',
'importtext'                 => 'Per favor exporta le file del wiki de origine con le [[Special:Export|facilitate pro exportar]].
Immagazina lo in tu disco e carga lo hic.',
'importstart'                => 'Importation de paginas in curso…',
'import-revision-count'      => '$1 {{PLURAL:$1|revision|revisiones}}',
'importnopages'              => 'Nulle paginas a importar.',
'importfailed'               => 'Importation fallite: <nowiki>$1</nowiki>',
'importunknownsource'        => 'Typo del origine de importation non cognoscite',
'importcantopen'             => 'Impossibile aperir le file de importation',
'importbadinterwiki'         => 'Ligamine interwiki invalide',
'importnotext'               => 'Texto vacue o mancante',
'importsuccess'              => 'Importation complete!',
'importhistoryconflict'      => 'Existe un conflicto in le historia de revisiones (es possibile que iste pagina ha essite importate anteriormente)',
'importnosources'            => 'Nulle origine de importation transwiki ha essite definite e le cargas de historia directe es disactivate.',
'importnofile'               => 'Nulle file de importation esseva cargate.',
'importuploaderrorsize'      => 'Le carga del file de importation ha fallite. Le grandor del file excede le limite pro cargas.',
'importuploaderrorpartial'   => 'Le carga del file de importation ha fallite. Le file esseva cargate solmente partialmente.',
'importuploaderrortemp'      => 'Le carga del file de importation ha fallite. Un directorio temporari manca.',
'import-parse-failure'       => 'Error syntactic durante importation XML',
'import-noarticle'           => 'Nulle pagina a importar!',
'import-nonewrevisions'      => 'Tote le revisiones habeva ja essite importate anteriormente.',
'xml-error-string'           => '$1 al linea $2, col $3 (byte $4): $5',
'import-upload'              => 'Cargar datos XML',

# Import log
'importlogpage'                    => 'Registro de importationes',
'importlogpagetext'                => 'Importationes administrative de paginas con historia de modificationes desde altere wikis.',
'import-logentry-upload'           => 'importava [[$1]] per medio de carga de file',
'import-logentry-upload-detail'    => '$1 {{PLURAL:$1|revision|revisiones}}',
'import-logentry-interwiki'        => 'importava $1 transwiki',
'import-logentry-interwiki-detail' => '$1 {{PLURAL:$1|revision|revisiones}} desde $2',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'Mi pagina de usator',
'tooltip-pt-anonuserpage'         => 'Le pagina de usator pro le adresse IP desde le qual tu face modificationes',
'tooltip-pt-mytalk'               => 'Mi pagina de discussion',
'tooltip-pt-anontalk'             => 'Discussion super le modificationes facite desde iste adresse IP',
'tooltip-pt-preferences'          => 'Mi preferentias',
'tooltip-pt-watchlist'            => 'Le lista de paginas que tu survelia pro modificationes',
'tooltip-pt-mycontris'            => 'Lista de mi contributiones',
'tooltip-pt-login'                => 'Nos recommenda que tu te identifica, ma il non es obligatori.',
'tooltip-pt-anonlogin'            => 'Nos te invita a aperir un session, ma il non es obligatori.',
'tooltip-pt-logout'               => 'Clauder session',
'tooltip-ca-talk'                 => 'Discussiones a proposito del pagina de contento',
'tooltip-ca-edit'                 => 'Tu pote modificar iste pagina. Per favor usa le button "Monstrar previsualisation" ante que tu publica tu modificationes.',
'tooltip-ca-addsection'           => 'Adder un commento a iste discussion.',
'tooltip-ca-viewsource'           => 'Iste pagina es protegite. Tu pote vider le codice-fonte de illo.',
'tooltip-ca-history'              => 'Versiones anterior de iste pagina.',
'tooltip-ca-protect'              => 'Proteger iste pagina',
'tooltip-ca-delete'               => 'Deler iste pagina',
'tooltip-ca-undelete'             => 'Restaurar le modificationes facite a iste pagina ante que illo esseva delite',
'tooltip-ca-move'                 => 'Renominar iste pagina',
'tooltip-ca-watch'                => 'Adder iste pagina a tu observatorio',
'tooltip-ca-unwatch'              => 'Eliminar iste pagina de tu observatorio',
'tooltip-search'                  => 'Cercar in {{SITENAME}}',
'tooltip-search-go'               => 'Visitar un pagina con iste nomine exacte si existe',
'tooltip-search-fulltext'         => 'Cercar iste texto in le paginas',
'tooltip-p-logo'                  => 'Pagina principal',
'tooltip-n-mainpage'              => 'Visitar le pagina principal',
'tooltip-n-portal'                => 'A proposito del projecto, que tu pote facer, ubi trovar cosas',
'tooltip-n-currentevents'         => 'Cerca informationes de fundo relative al actualitate',
'tooltip-n-recentchanges'         => 'Le lista de modificationes recente in le wiki.',
'tooltip-n-randompage'            => 'Visitar un pagina qualcunque',
'tooltip-n-help'                  => 'Le solutiones de vostre problemas.',
'tooltip-t-whatlinkshere'         => 'Lista de tote le paginas wiki con ligamines a iste pagina',
'tooltip-t-recentchangeslinked'   => 'Modificationes recente in le paginas al quales iste pagina ha ligamines',
'tooltip-feed-rss'                => 'Syndication RSS pro iste pagina',
'tooltip-feed-atom'               => 'Syndication Atom pro iste pagina',
'tooltip-t-contributions'         => 'Vider le lista de contributiones de iste usator',
'tooltip-t-emailuser'             => 'Inviar un e-mail a iste usator',
'tooltip-t-upload'                => 'Cargar files',
'tooltip-t-specialpages'          => 'Lista de tote le paginas special',
'tooltip-t-print'                 => 'Version imprimibile de iste pagina',
'tooltip-t-permalink'             => 'Ligamine permanente a iste version del pagina',
'tooltip-ca-nstab-main'           => 'Vider le pagina de contento',
'tooltip-ca-nstab-user'           => 'Vider le pagina de usator',
'tooltip-ca-nstab-media'          => 'Vider le pagina de media',
'tooltip-ca-nstab-special'        => 'Isto es un pagina special, tu non pote modificar le pagina mesme',
'tooltip-ca-nstab-project'        => 'Vider le pagina de projecto',
'tooltip-ca-nstab-image'          => 'Vider le pagina del file',
'tooltip-ca-nstab-mediawiki'      => 'Vider le message del systema',
'tooltip-ca-nstab-template'       => 'Vider le patrono',
'tooltip-ca-nstab-help'           => 'Vider le pagina de adjuta',
'tooltip-ca-nstab-category'       => 'Vider le pagina del categoria',
'tooltip-minoredit'               => 'Marcar iste modification como minor',
'tooltip-save'                    => 'Confirmar tu modificationes',
'tooltip-preview'                 => 'Per favor verifica tu modificationes ante que tu los publica!',
'tooltip-diff'                    => 'Detaliar le modificationes que tu ha facite in le texto.',
'tooltip-compareselectedversions' => 'Vider le differentias inter le seligite duo versiones de iste pagina.',
'tooltip-watch'                   => 'Adder iste pagina a tu observatorio',
'tooltip-recreate'                => 'Recrear le pagina nonobstante que illo ha essite delite',
'tooltip-upload'                  => 'Comencia cargar',

# Stylesheets
'common.css'      => '/* Le CSS placiate hic se applicara a tote le stilos */',
'standard.css'    => '/* Le CSS placiate hic afficera le usatores del stilo Standard */',
'nostalgia.css'   => '/* Le CSS placiate hic afficera le usatores del stilo Nostalgia */',
'cologneblue.css' => '/* Le CSS placiate hic afficera le usatores del stilo Cologne Blue */',
'monobook.css'    => '/* Le CSS placiate hic afficera le usatores del stilo Monobook */',
'myskin.css'      => '/* Le CSS placiate hic afficera le usatores del stilo Myskin */',
'chick.css'       => '/* Le CSS placiate hic afficera le usatores del stilo Chick */',
'simple.css'      => '/* Le CSS placiate hic afficera le usatores del stilo Simple */',
'modern.css'      => '/* Le CSS placiate hic afficera le usatores del stilo Modern */',

# Scripts
'common.js'      => '/* Omne JavaScript hic se executara pro tote le usatores a cata carga de pagina. */',
'standard.js'    => '/* Omne JavaScript hic se executara pro le usatores del stilo Standard */',
'nostalgia.js'   => '/* Omne JavaScript hic se executara pro le usatores del stilo Nostalgia */',
'cologneblue.js' => '/* Omne JavaScript hic se executara pro le usatores del stilo Cologne Blue */',
'monobook.js'    => '/* Omne JavaScript hic se executara pro le usatores del stilo MonoBook */',
'myskin.js'      => '/* Omne JavaScript hic se executara pro le usatores del stilo Myskin */',
'chick.js'       => '/* Omne JavaScript hic se executara pro le usatores del stilo Chick */',
'simple.js'      => '/* Omne JavaScript hic se executara pro le usatores del stilo Simple */',
'modern.js'      => '/* Omne JavaScript hic se executara pro le usatores del stilo Modern */',

# Metadata
'nodublincore'      => 'Le metadatos Dublin Core RDF ha essite disactivate in iste servitor.',
'nocreativecommons' => 'Le metadatos Creative Commons RDF ha essite disactivate in iste servitor.',
'notacceptable'     => 'Le servitor wiki non pote provider datos in un formato que tu cliente sape leger.',

# Attribution
'anonymous'        => 'Usator(es) anonyme de {{SITENAME}}',
'siteuser'         => 'Usator $1 de {{SITENAME}}',
'lastmodifiedatby' => 'Le modification le plus recente de iste pagina esseva facite le $1 a $2 per $3.', # $1 date, $2 time, $3 user
'othercontribs'    => 'A base de contributiones per $1.',
'others'           => 'alteres',
'siteusers'        => 'Usator(es) de {{SITENAME}} $1',
'creditspage'      => 'Autores del pagina',
'nocredits'        => 'Nulle information es disponibile super le autores de iste pagina.',

# Spam protection
'spamprotectiontitle' => 'Filtro de protection antispam',
'spamprotectiontext'  => 'Le pagina que tu voleva immagazinar esseva blocate per le filtro antispam.
Le causa es probabilemente un ligamine verso un sito externe que es presente in un lista nigre.',
'spamprotectionmatch' => 'Le sequente texto es lo que activava nostre filtro antispam: $1',
'spambot_username'    => 'Nettamento de spam in MediaWiki',
'spam_reverting'      => 'Revertite al ultime version que non contine ligamines a $1',
'spam_blanking'       => 'Tote le revisiones contineva ligamines a $1. Le pagina ha essite vacuate.',

# Info page
'infosubtitle'   => 'Informationes del pagina',
'numedits'       => 'Numero de modificationes (pagina): $1',
'numtalkedits'   => 'Numero de modificationes (pagina de discussion): $1',
'numwatchers'    => 'Numero de observatores: $1',
'numauthors'     => 'Numero de autores distincte (pagina): $1',
'numtalkauthors' => 'Numero de autores distincte (pagina de discussion): $1',

# Math options
'mw_math_png'    => 'Sempre producer PNG',
'mw_math_simple' => 'HTML si multo simple, alteremente PNG',
'mw_math_html'   => 'HTML si possibile, alteremente PNG',
'mw_math_source' => 'Lassa lo como TeX (pro navigatores in modo texto)',
'mw_math_modern' => 'Recommendate pro navigatores moderne',
'mw_math_mathml' => 'MathML',

# Patrolling
'markaspatrolleddiff'                 => 'Marcar como patruliate',
'markaspatrolledtext'                 => 'Marcar iste pagina como patruliate',
'markedaspatrolled'                   => 'Marcate como patruliate',
'markedaspatrolledtext'               => 'Le revision seligite ha essite marcate como patruliate.',
'rcpatroldisabled'                    => 'Patrulia de modificationes recente disactivate',
'rcpatroldisabledtext'                => 'Le functionalitate de patrulia de modificationes recente es disactivate al momento.',
'markedaspatrollederror'              => 'Impossibile marcar como patruliate',
'markedaspatrollederrortext'          => 'Tu debe specificar un revision a marcar como patruliate.',
'markedaspatrollederror-noautopatrol' => 'Tu non es permittite a marcar tu proprie modificationes como patruliate.',

# Patrol log
'patrol-log-page'   => 'Registro de patrulia',
'patrol-log-header' => 'Isto es un registro de revisiones patruliate.',
'patrol-log-line'   => 'marcava $1 de $2 como patruliate $3',
'patrol-log-auto'   => '(automaticamente)',

# Image deletion
'deletedrevision'                 => 'Deleva le ancian revision $1',
'filedeleteerror-short'           => 'Error durante le deletion del file: $1',
'filedeleteerror-long'            => 'Se incontrava errores durante le deletion del file:

$1',
'filedelete-missing'              => 'Le file "$1" non pote esser delite, proque illo non existe.',
'filedelete-old-unregistered'     => 'Le revision del file specificate "$1" non existe in le base de datos.',
'filedelete-current-unregistered' => 'Le file specificate "$1" non existe in le base de datos.',
'filedelete-archive-read-only'    => 'Le servitor de web non pote scriber al directorio de archivo "$1".',

# Browsing diffs
'previousdiff' => '← Version plus ancian',
'nextdiff'     => 'Version plus nove →',

# Media information
'mediawarning'         => "'''Attention''': Iste file pote continer codice maligne. Si tu lo executa, tu systema pote esser compromittite.<hr />",
'imagemaxsize'         => 'Limitar le imagines in paginas de description de files a:',
'thumbsize'            => 'Grandor del miniaturas:',
'widthheightpage'      => '$1×$2, $3 {{PLURAL:$3|pagina|paginas}}',
'file-info'            => '(grandor del file: $1, typo MIME: $2)',
'file-info-size'       => '($1 × $2 pixel, grandor del file: $3, typo MIME: $4)',
'file-nohires'         => '<small>Non es disponibile un resolution plus alte.</small>',
'svg-long-desc'        => '(File SVG, dimensiones nominal: $1 × $2 pixels, grandor del file: $3)',
'show-big-image'       => 'Plen resolution',
'show-big-image-thumb' => '<small>Dimensiones de iste previsualisation: $1 × $2 pixels</small>',

# Special:NewImages
'newimages'             => 'Galeria de nove files',
'imagelisttext'         => "Infra es un lista de '''$1''' {{PLURAL:$1|imagine|imagines}} ordinate $2.",
'newimages-summary'     => 'Iste pagina special detalia le recente files cargate.',
'showhidebots'          => '($1 bots)',
'noimages'              => 'Nihil a vider.',
'ilsubmit'              => 'Cercar',
'bydate'                => 'per data',
'sp-newimages-showfrom' => 'Monstrar nove files a partir del $1 a $2',

# Bad image list
'bad_image_list' => 'Le formato es como seque:

Solmente punctos de lista (lineas que comencia con *) es considerate.
Le prime ligamine in un linea debe esser un ligamine a un file invalide.
Omne ligamines posterior in le mesme linea es considerate como exceptiones, i.e. paginas in que le file pote esser directemente incorporate.',

# Metadata
'metadata'          => 'Metadatos',
'metadata-help'     => 'Iste file contine informationes additional, que probabilemente ha venite del camera digital o scanner usate pro crear o digitalisar lo.
Si le file ha essite modificate de su stato original, es possibile que alcun detalios non reflecte completemente le file modificate.',
'metadata-expand'   => 'Revelar detalios extense',
'metadata-collapse' => 'Celar detalios extense',
'metadata-fields'   => 'Le campos de metadatos EXIF listate in iste message se revelara in le visualisation del pagina de imagine quando se collabe le tabula de metadatos.
Le alteres essera initialmente celate.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => 'Latitude',
'exif-imagelength'                 => 'Altitude',
'exif-bitspersample'               => 'Bits per componente',
'exif-compression'                 => 'Schema de compression',
'exif-photometricinterpretation'   => 'Composition de pixel',
'exif-orientation'                 => 'Orientation',
'exif-samplesperpixel'             => 'Numero de componentes',
'exif-planarconfiguration'         => 'Arrangiamento del datos',
'exif-ycbcrsubsampling'            => 'Ration de reduction de Y a C',
'exif-ycbcrpositioning'            => 'Positionamento Y e C',
'exif-xresolution'                 => 'Resolution horizontal',
'exif-yresolution'                 => 'Resolution vertical',
'exif-resolutionunit'              => 'Unitate de resolution X e Y',
'exif-stripoffsets'                => 'Location del datos del imagine',
'exif-rowsperstrip'                => 'Numero de lineas per banda',
'exif-stripbytecounts'             => 'Bytes per banda comprimite',
'exif-jpeginterchangeformat'       => 'Position de JPEG SOI',
'exif-jpeginterchangeformatlength' => 'Bytes del datos JPEG',
'exif-transferfunction'            => 'Function de transferimento',
'exif-whitepoint'                  => 'Chromaticitate del puncto blanc',
'exif-primarychromaticities'       => 'Chromaticitates del colores primari',
'exif-ycbcrcoefficients'           => 'Coefficientes del matrice de transformation del spatio de colores',
'exif-referenceblackwhite'         => 'Par de valores de referentia nigre e blanc',
'exif-datetime'                    => 'Data e hora de modification del file',
'exif-imagedescription'            => 'Titulo del imagine',
'exif-make'                        => 'Fabricante del camera',
'exif-model'                       => 'Modello del camera',
'exif-software'                    => 'Software usate',
'exif-artist'                      => 'Autor',
'exif-copyright'                   => 'Titular del copyright',
'exif-exifversion'                 => 'Version de Exif',
'exif-flashpixversion'             => 'Version supportate de Flashpix',
'exif-colorspace'                  => 'Spatio de colores',
'exif-componentsconfiguration'     => 'Significato de cata componente',
'exif-compressedbitsperpixel'      => 'Modo de compression del imagine',
'exif-pixelydimension'             => 'Latitude valide del imagine',
'exif-pixelxdimension'             => 'Altitude valide del imagine',
'exif-makernote'                   => 'Notas del fabricante',
'exif-usercomment'                 => 'Commentos del usator',
'exif-relatedsoundfile'            => 'File audio connexe',
'exif-datetimeoriginal'            => 'Data e hora del generation del datos',
'exif-datetimedigitized'           => 'Data e hora del digitalisation',
'exif-subsectime'                  => 'Fractiones de secundas DateTime',
'exif-subsectimeoriginal'          => 'Fractiones de secundas DateTimeOriginal',
'exif-subsectimedigitized'         => 'Fractiones de secundas DateTimeDigitized',
'exif-exposuretime'                => 'Tempore de exposition',
'exif-exposuretime-format'         => '$1 sec ($2)',
'exif-fnumber'                     => 'Numero F',
'exif-exposureprogram'             => 'Programma de exposition',
'exif-spectralsensitivity'         => 'Sensibilitate spectral',
'exif-isospeedratings'             => 'Classification de velocitate ISO',
'exif-oecf'                        => 'Factor de conversion optoelectronic',
'exif-shutterspeedvalue'           => 'Velocitate del obturator',
'exif-aperturevalue'               => 'Apertura',
'exif-brightnessvalue'             => 'Luminositate',
'exif-exposurebiasvalue'           => 'Correction de exposition',
'exif-maxaperturevalue'            => 'Apertura maxime pro terra',
'exif-subjectdistance'             => 'Distantia del subjecto',
'exif-meteringmode'                => 'Modo de mesura',
'exif-lightsource'                 => 'Fonte de lumine',
'exif-flash'                       => 'Flash',
'exif-focallength'                 => 'Longitude focal del lente',
'exif-subjectarea'                 => 'Area de subjecto',
'exif-flashenergy'                 => 'Energia del flash',
'exif-spatialfrequencyresponse'    => 'Responsa de frequentia spatial',
'exif-focalplanexresolution'       => 'Resolution X del plano focal',
'exif-focalplaneyresolution'       => 'Resolution Y del plano focal',
'exif-focalplaneresolutionunit'    => 'Unitate del resolution del plano focal',
'exif-subjectlocation'             => 'Location del subjecto',
'exif-exposureindex'               => 'Indice de exposition',
'exif-sensingmethod'               => 'Methodo de sensor',
'exif-filesource'                  => 'Origine del file',
'exif-scenetype'                   => 'Typo de scena',
'exif-cfapattern'                  => 'Patrono CFA',
'exif-customrendered'              => 'Processamento de imagines personalisate',
'exif-exposuremode'                => 'Modo de exposition',
'exif-whitebalance'                => 'Balancia de blanc',
'exif-digitalzoomratio'            => 'Ration de zoom digital',
'exif-focallengthin35mmfilm'       => 'Longitude focal in film de 35 mm',
'exif-scenecapturetype'            => 'Typo de captura de scena',
'exif-gaincontrol'                 => 'Controlo de scena',
'exif-contrast'                    => 'Contrasto',
'exif-saturation'                  => 'Saturation',
'exif-sharpness'                   => 'Nitiditate',
'exif-devicesettingdescription'    => 'Description del configurationes del apparato',
'exif-subjectdistancerange'        => 'Intervallo de distantia del subjecto',
'exif-imageuniqueid'               => 'ID unic del imagine',
'exif-gpsversionid'                => 'Version del etiquetta GPS',
'exif-gpslatituderef'              => 'Latitude nord o sud',
'exif-gpslatitude'                 => 'Latitude',
'exif-gpslongituderef'             => 'Longitude est o west',
'exif-gpslongitude'                => 'Longitude',
'exif-gpsaltituderef'              => 'Referentia de altitude',
'exif-gpsaltitude'                 => 'Altitude',
'exif-gpstimestamp'                => 'Hora GPS (horologio atomic)',
'exif-gpssatellites'               => 'Satellites usate pro mesura',
'exif-gpsstatus'                   => 'Stato del receptor',
'exif-gpsmeasuremode'              => 'Modo de mesura',
'exif-gpsdop'                      => 'Precision de mesura',
'exif-gpsspeedref'                 => 'Unitate de velocitate',
'exif-gpsspeed'                    => 'Velocitate del receptor GPS',
'exif-gpstrackref'                 => 'Referentia pro direction de movimento',
'exif-gpstrack'                    => 'Direction de movimento',
'exif-gpsimgdirectionref'          => 'Referentia pro direction de imagine',
'exif-gpsimgdirection'             => 'Direction de imagine',
'exif-gpsmapdatum'                 => 'Datos de examination geodesic usate',
'exif-gpsdestlatituderef'          => 'Referentia pro latitude de destination',
'exif-gpsdestlatitude'             => 'Latitude de destination',
'exif-gpsdestlongituderef'         => 'Referentia pro longitude de destination',
'exif-gpsdestlongitude'            => 'Longitude de destination',
'exif-gpsdestbearingref'           => 'Referentia pro relevamento de destination',
'exif-gpsdestbearing'              => 'Relevamento de destination',
'exif-gpsdestdistanceref'          => 'Referentia pro distantia a destination',
'exif-gpsdestdistance'             => 'Distantia a destination',
'exif-gpsprocessingmethod'         => 'Nomine de methodo de processamento GPS',
'exif-gpsareainformation'          => 'Nomine de area GPS',
'exif-gpsdatestamp'                => 'Data GPS',
'exif-gpsdifferential'             => 'Correction differential GPS',

# EXIF attributes
'exif-compression-1' => 'Non comprimite',

'exif-unknowndate' => 'Data incognite',

'exif-orientation-1' => 'Normal', # 0th row: top; 0th column: left
'exif-orientation-2' => 'Invertite horizontalmente', # 0th row: top; 0th column: right
'exif-orientation-3' => 'Rotate 180°', # 0th row: bottom; 0th column: right
'exif-orientation-4' => 'Invertite verticalmente', # 0th row: bottom; 0th column: left
'exif-orientation-5' => 'Rotate 90° in senso antihorologic e invertite verticalmente', # 0th row: left; 0th column: top
'exif-orientation-6' => 'Rotate 90° in senso horologic', # 0th row: right; 0th column: top
'exif-orientation-7' => 'Rotate 90° in senso horologic e invertite verticalmente', # 0th row: right; 0th column: bottom
'exif-orientation-8' => 'Rotate 90° in senso antihorologic', # 0th row: left; 0th column: bottom

'exif-planarconfiguration-1' => 'formato a blocos (chunky)',
'exif-planarconfiguration-2' => 'formato planar',

'exif-componentsconfiguration-0' => 'non existe',

'exif-exposureprogram-0' => 'Non definite',
'exif-exposureprogram-1' => 'Manual',
'exif-exposureprogram-2' => 'Programma normal',
'exif-exposureprogram-3' => 'Prioritate del apertura',
'exif-exposureprogram-4' => 'Prioritate del obturator',
'exif-exposureprogram-5' => 'Programma creative (preferentia verso profunditate de campo)',
'exif-exposureprogram-6' => 'Programma de action (preferentia verso rapiditate del obturator)',
'exif-exposureprogram-7' => 'Modo de portrait (pro subjectos vicin con fundo foras de foco)',
'exif-exposureprogram-8' => 'Modo panorama (pro photos de panoramas con fundo in foco)',

'exif-subjectdistance-value' => '$1 metros',

'exif-meteringmode-0'   => 'Incognite',
'exif-meteringmode-1'   => 'Media',
'exif-meteringmode-2'   => 'Media pesate in centro',
'exif-meteringmode-3'   => 'Puncto',
'exif-meteringmode-4'   => 'MultiPuncto',
'exif-meteringmode-5'   => 'Patrono',
'exif-meteringmode-6'   => 'Partial',
'exif-meteringmode-255' => 'Altere',

'exif-lightsource-0'   => 'Incognite',
'exif-lightsource-1'   => 'Lumine diurne',
'exif-lightsource-2'   => 'Fluorescente',
'exif-lightsource-3'   => 'Tungsten (lumine incandescente)',
'exif-lightsource-4'   => 'Flash',
'exif-lightsource-9'   => 'Tempore clar',
'exif-lightsource-10'  => 'Tempore nubilose',
'exif-lightsource-11'  => 'Umbra',
'exif-lightsource-12'  => 'Fluorescente de lumine diurne (D 5700 – 7100K)',
'exif-lightsource-13'  => 'Fluorescente blanc diurne (N 4600 – 5400K)',
'exif-lightsource-14'  => 'Fluorescente blanc fresc (W 3900 – 4500K)',
'exif-lightsource-15'  => 'Fluorescente blanc (WW 3200 – 3700K)',
'exif-lightsource-17'  => 'Lumine standard A',
'exif-lightsource-18'  => 'Lumine standard B',
'exif-lightsource-19'  => 'Lumine standard C',
'exif-lightsource-24'  => 'Tungsten de studio ISO',
'exif-lightsource-255' => 'Altere origine de lumine',

'exif-focalplaneresolutionunit-2' => 'uncias',

'exif-sensingmethod-1' => 'Non definite',
'exif-sensingmethod-2' => 'Sensor de area de colores a singule chip',
'exif-sensingmethod-3' => 'Sensor de area de colores a duo chips',
'exif-sensingmethod-4' => 'Sensor de area de colores a tres chips',
'exif-sensingmethod-5' => 'Sensor de area sequential de colores',
'exif-sensingmethod-7' => 'Sensor trilinear',
'exif-sensingmethod-8' => 'Sensor de color linear sequential',

'exif-scenetype-1' => 'Un imagine directemente photographiate',

'exif-customrendered-0' => 'Processo normal',
'exif-customrendered-1' => 'Processo personalisate',

'exif-exposuremode-0' => 'Exposition automatic',
'exif-exposuremode-1' => 'Exposition manual',
'exif-exposuremode-2' => 'Bracketing automatic',

'exif-whitebalance-0' => 'Balancia de blanc automatic',
'exif-whitebalance-1' => 'Balancia de blanc manual',

'exif-scenecapturetype-0' => 'Standard',
'exif-scenecapturetype-1' => 'Panorama',
'exif-scenecapturetype-2' => 'Portrait',
'exif-scenecapturetype-3' => 'Scena nocturne',

'exif-gaincontrol-0' => 'Nulle',
'exif-gaincontrol-1' => 'Basse ganio positive',
'exif-gaincontrol-2' => 'Alte ganio positive',
'exif-gaincontrol-3' => 'Basse ganio negative',
'exif-gaincontrol-4' => 'Alte ganio negative',

'exif-contrast-0' => 'Normal',
'exif-contrast-1' => 'Suave',
'exif-contrast-2' => 'Forte',

'exif-saturation-0' => 'Normal',
'exif-saturation-1' => 'Basse saturation',
'exif-saturation-2' => 'Alte saturation',

'exif-sharpness-0' => 'Normal',
'exif-sharpness-1' => 'Dulce',
'exif-sharpness-2' => 'Dur',

'exif-subjectdistancerange-0' => 'Incognite',
'exif-subjectdistancerange-1' => 'Macro',
'exif-subjectdistancerange-2' => 'Vista proxime',
'exif-subjectdistancerange-3' => 'Vista distante',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'Latitude nord',
'exif-gpslatitude-s' => 'Latitude sud',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'Longitude est',
'exif-gpslongitude-w' => 'Longitude west',

'exif-gpsstatus-a' => 'Mesura in curso',
'exif-gpsstatus-v' => 'Interoperabilitate del mesura',

'exif-gpsmeasuremode-2' => 'Mesura bidimensional',
'exif-gpsmeasuremode-3' => 'Mesura tridimensional',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'Kilometros per hora',
'exif-gpsspeed-m' => 'Millias per hora',
'exif-gpsspeed-n' => 'Nodos',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => 'Direction real',
'exif-gpsdirection-m' => 'Direction magnetic',

# External editor support
'edit-externally'      => 'Modificar iste file con un programma externe',
'edit-externally-help' => 'Vide le [http://www.mediawiki.org/wiki/Manual:External_editors instructiones de configuration] pro ulterior informationes.',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'totes',
'imagelistall'     => 'totes',
'watchlistall2'    => 'totes',
'namespacesall'    => 'totes',
'monthsall'        => 'totes',

# E-mail address confirmation
'confirmemail'             => 'Confirmar adresse de e-mail',
'confirmemail_noemail'     => 'Tu non ha configurate un adresse de e-mail valide in tu [[Special:Preferences|preferentias de usator]].',
'confirmemail_text'        => '{{SITENAME}} require que tu valida tu adresse de e-mail ante que tu usa functiones involvente e-mail.
Activa le button infra pro inviar un message de confirmation a tu adresse.
Le message includera un ligamine continente un codice;
visita le ligamine in tu navigator pro confirmar que tu adresse de e-mail es valide.',
'confirmemail_pending'     => '<div class="error">Un codice de confirmation ha ja essite inviate a te;
si tu ha recentemente create tu conto, es recommendate attender le arrivata de illo durante alcun minutas ante de provar requestar un nove codice.</div>',
'confirmemail_send'        => 'Inviar un codice de confirmation',
'confirmemail_sent'        => 'Message de confirmation inviate.',
'confirmemail_oncreate'    => 'Un codice de confirmation ha essite inviate a tu adresse de e-mail.
Iste codice non es necessari pro aperir un session, ma es requirite pro activar omne functiones a base de e-mail in le wiki.',
'confirmemail_sendfailed'  => '{{SITENAME}} non poteva inviar te le message de confirmation.
Per favor verifica que tu adresse de e-mail non ha characteres invalide.

Le servitor de e-mail retornava: $1',
'confirmemail_invalid'     => 'Codice de confirmation invalide.
Es possibile que le codice ha expirate.',
'confirmemail_needlogin'   => 'Tu debe $1 pro confirmar tu adresse de e-mail.',
'confirmemail_success'     => 'Tu adresse de e-mail ha essite confirmate.
Tu pote ora aperir un session e fruer te del wiki.',
'confirmemail_loggedin'    => 'Tu adresse de e-mail ha ora essite confirmate.',
'confirmemail_error'       => 'Un problema occureva durante le immagazinage de tu confirmation.',
'confirmemail_subject'     => 'Confirmation del adresse de e-mail pro {{SITENAME}}',
'confirmemail_body'        => 'Un persona, probabilemente tu, usante le adresse IP $1,
ha registrate un conto "$2" con iste adresse de e-mail in {{SITENAME}}.

Pro confirmar que iste conto es de facto tue, e pro activar le functiones
de e-mail in {{SITENAME}}, visita iste ligamine in tu navigator:

$3

Si tu *non* ha registrate le conto, seque iste ligamine
pro cancellar le confirmation del adresse de e-mail:

$5

Iste codice de confirmation expirara a $4.',
'confirmemail_invalidated' => 'Confirmation del adresse de e-mail cancellate',
'invalidateemail'          => 'Cancellar confirmation del adresse de e-mail',

# Scary transclusion
'scarytranscludedisabled' => '[Le transclusion interwiki es disactivate]',
'scarytranscludefailed'   => '[Falleva de obtener le patrono pro $1]',
'scarytranscludetoolong'  => '[URL es troppo longe]',

# Trackbacks
'trackbackbox'      => '<div id="mw_trackbacks">Retroligamines a iste pagina:<br />
$1
</div>',
'trackbackremove'   => ' ([$1 Deler])',
'trackbacklink'     => 'Retroligamine',
'trackbackdeleteok' => 'Le retroligamine ha essite delite con successo.',

# Delete conflict
'deletedwhileediting' => "'''Attention:''' Iste pagina esseva delite post que tu comenciava a modificar lo!",
'confirmrecreate'     => "Le usator [[User:$1|$1]] ([[User talk:$1|discussion]]) ha delite iste pagina post que tu comenciava a modificar lo, dante le motivo:
: ''$2''
Per favor confirma que tu realmente vole recrear iste pagina.",
'recreate'            => 'Recrear',

# HTML dump
'redirectingto' => 'Redirection verso [[:$1]] in curso…',

# action=purge
'confirm_purge'        => 'Rader le cache de iste pagina?

$1',
'confirm_purge_button' => 'OK',

# AJAX search
'searchcontaining' => "Cercar paginas continente ''$1''.",
'searchnamed'      => "Cercar paginas nominate ''$1''.",
'articletitles'    => "Paginas comenciante con ''$1''",
'hideresults'      => 'Celar resultatos',
'useajaxsearch'    => 'Usar cerca con AJAX',

# Multipage image navigation
'imgmultipageprev' => '← precedente pagina',
'imgmultipagenext' => 'sequente pagina →',
'imgmultigo'       => 'Ir!',
'imgmultigoto'     => 'Visitar pagina $1',

# Table pager
'ascending_abbrev'         => 'asc',
'descending_abbrev'        => 'desc',
'table_pager_next'         => 'Sequente pagina',
'table_pager_prev'         => 'Precedente pagina',
'table_pager_first'        => 'Prime pagina',
'table_pager_last'         => 'Ultime pagina',
'table_pager_limit'        => 'Monstrar $1 entratas per pagina',
'table_pager_limit_submit' => 'Ir',
'table_pager_empty'        => 'Nulle resultatos',

# Auto-summaries
'autosumm-blank'   => 'Tote le contento es removite del pagina',
'autosumm-replace' => "Reimplacia contento del pagina con '$1'",
'autoredircomment' => 'Redirection verso [[$1]]',
'autosumm-new'     => 'Nove pagina: $1',

# Live preview
'livepreview-loading' => 'Cargamento in curso…',
'livepreview-ready'   => 'Cargamento in curso… Preste!',
'livepreview-failed'  => 'Le previsualisation directe ha fallite! Prova le previsualisation normal.',
'livepreview-error'   => 'Impossibile connecter: $1 "$2". Prova le previsualisation normal.',

# Friendlier slave lag warnings
'lag-warn-normal' => 'Le modificationes plus nove que $1 {{PLURAL:$1|secunda|secundas}} possibilemente non se revela in iste lista.',
'lag-warn-high'   => 'A causa de un alte latentia del servitor de base de datos, le modificationes plus nove que $1 {{PLURAL:$1|secunda|secundas}} possibilemente non se revela in iste lista.',

# Watchlist editor
'watchlistedit-numitems'       => 'Tu observatorio contine {{PLURAL:$1|1 titulo|$1 titulos}}, excludente le paginas de discussion.',
'watchlistedit-noitems'        => 'Tu observatorio contine nulle titulos.',
'watchlistedit-normal-title'   => 'Modificar observatorio',
'watchlistedit-normal-legend'  => 'Eliminar titulos del observatorio',
'watchlistedit-normal-explain' => 'Le titulos in tu observatorio se monstra infra.
Pro eliminar un titulo, marca le quadrato correspondente, e clicca "Eliminar titulos".
Tu pote etiam [[Special:Watchlist/raw|modificar le lista in forma crude]].',
'watchlistedit-normal-submit'  => 'Eliminar titulos',
'watchlistedit-normal-done'    => '{{PLURAL:$1|1 titulo|$1 titulos}} ha essite eliminate de tu observatorio:',
'watchlistedit-raw-title'      => 'Modification del observatorio in forma crude',
'watchlistedit-raw-legend'     => 'Modification del observatorio in forma de un lista simple de titulos',
'watchlistedit-raw-explain'    => 'Le titulos in tu observatorio se monstra infra, e tu pote adder e eliminar entratas del lista;
un titulo per linea.
Quando tu ha finite, clicca "Actualisar observatorio".
Tu pote etiam [[Special:Watchlist/edit|usar le editor standard]].',
'watchlistedit-raw-titles'     => 'Titulos:',
'watchlistedit-raw-submit'     => 'Actualisar observatorio',
'watchlistedit-raw-done'       => 'Tu observatorio ha essite actualisate.',
'watchlistedit-raw-added'      => '{{PLURAL:$1|1 titulo|$1 titulos}} ha essite addite:',
'watchlistedit-raw-removed'    => '{{PLURAL:$1|1 titulo|$1 titulos}} ha essite eliminate:',

# Watchlist editing tools
'watchlisttools-view' => 'Vider modificationes pertinente',
'watchlisttools-edit' => 'Vider e modificar le observatorio',
'watchlisttools-raw'  => 'Modificar observatorio crude',

# Core parser functions
'unknown_extension_tag' => 'Etiquetta de extension incognite "$1"',

# Special:Version
'version'                          => 'Version', # Not used as normal message but as header for the special page itself
'version-extensions'               => 'Extensiones installate',
'version-specialpages'             => 'Paginas special',
'version-parserhooks'              => 'Uncinos del analysator syntactic',
'version-variables'                => 'Variabiles',
'version-other'                    => 'Altere',
'version-mediahandlers'            => 'Executores de media',
'version-hooks'                    => 'Uncinos',
'version-extension-functions'      => 'Functiones de extensiones',
'version-parser-extensiontags'     => 'Etiquettas de extension del analysator syntactic',
'version-parser-function-hooks'    => 'Uncinos de functiones del analysator syntactic',
'version-skin-extension-functions' => 'Functiones de extension de stilos',
'version-hook-name'                => 'Nomine del uncino',
'version-hook-subscribedby'        => 'Subscribite per',
'version-version'                  => 'Version',
'version-license'                  => 'Licentia',
'version-software'                 => 'Software installate',
'version-software-product'         => 'Producto',
'version-software-version'         => 'Version',

# Special:FilePath
'filepath'         => 'Cammino del file',
'filepath-page'    => 'File:',
'filepath-submit'  => 'Cammino',
'filepath-summary' => 'Iste pagina special contine le cammino complete de un file.
Le imagines se monstra in plen resolution, le altere typos de file se executa directemente con lor programmas associate.

Entra le nomine del file sin le prefixo "{{ns:image}}:".',

# Special:FileDuplicateSearch
'fileduplicatesearch'          => 'Cercar files duplicate',
'fileduplicatesearch-summary'  => "Cercar files duplicate a base de lor summas de verification ''(hash).''

Entra le nomine del file sin le prefixo \"{{ns:image}}:\".",
'fileduplicatesearch-legend'   => 'Cercar un duplicato',
'fileduplicatesearch-filename' => 'Nomine del file:',
'fileduplicatesearch-submit'   => 'Cercar',
'fileduplicatesearch-info'     => '$1 × $2 pixel<br />Grandor del file: $3<br />Typo MIME: $4',
'fileduplicatesearch-result-1' => 'Le file "$1" ha nulle duplicato identic.',
'fileduplicatesearch-result-n' => 'Le file "$1" ha {{PLURAL:$2|1 duplicato|$2 duplicatos}} identic.',

# Special:SpecialPages
'specialpages'                   => 'Paginas special',
'specialpages-note'              => '----
* Paginas special normal.
* <span class="mw-specialpagerestricted">Paginas special restringite.</span>',
'specialpages-group-maintenance' => 'Reportos de mantenentia',
'specialpages-group-other'       => 'Altere paginas special',
'specialpages-group-login'       => 'Aperir session / crear conto',
'specialpages-group-changes'     => 'Modificationes recente e registros',
'specialpages-group-media'       => 'Reportos de media e cargas',
'specialpages-group-users'       => 'Usatores e derectos',
'specialpages-group-highuse'     => 'Paginas multo usate',
'specialpages-group-pages'       => 'Lista de paginas',
'specialpages-group-pagetools'   => 'Instrumentos pro paginas',
'specialpages-group-wiki'        => 'Datos e instrumentos pro Wiki',
'specialpages-group-redirects'   => 'Redirection de paginas special',
'specialpages-group-spam'        => 'Instrumentos antispam',

# Special:BlankPage
'blankpage'              => 'Pagina vacue',
'intentionallyblankpage' => 'Iste pagina es intentionalmente vacue',

);
