<?php
/** Franco-Provençal (Arpetan)
 *
 * @ingroup Language
 * @file
 *
 * @author ChrisPtDe
 * @author לערי ריינהארט
 */

$fallback = 'fr';

$skinNames = array(
    'standard'    => 'Estandârd',
    'nostalgia'   => 'Encrêt',
    'cologneblue' => 'Cologne Blu',
    'chick'       => 'Pugin',
    'simple'      => 'Simplo'
);

$bookstoreList = array(
    'Amazon.fr'    => 'http://www.amazon.fr/exec/obidos/ISBN=$1',
    'alapage.fr'   => 'http://www.alapage.com/mx/?tp=F&type=101&l_isbn=$1&donnee_appel=ALASQ&devise=&',
    'fnac.com'     => 'http://www3.fnac.com/advanced/book.do?isbn=$1',
    'chapitre.com' => 'http://www.chapitre.com/frame_rec.asp?isbn=$1',
);

$namespaceNames = array(
    NS_MEDIA          => 'Mèdia',
    NS_SPECIAL        => 'Spèciâl',
    NS_MAIN           => '',
    NS_TALK           => 'Discutar',
    NS_USER           => 'Utilisator',
    NS_USER_TALK      => 'Discussion_Utilisator',
    # NS_PROJECT set by $wgMetaNamespace
    NS_PROJECT_TALK   => 'Discussion_$1',
    NS_IMAGE          => 'Émâge',
    NS_IMAGE_TALK     => 'Discussion_Émâge',
    NS_MEDIAWIKI      => 'MediaWiki',
    NS_MEDIAWIKI_TALK => 'Discussion_MediaWiki',
    NS_TEMPLATE       => 'Modèlo',
    NS_TEMPLATE_TALK  => 'Discussion_Modèlo',
    NS_HELP           => 'Éde',
    NS_HELP_TALK      => 'Discussion_Éde',
    NS_CATEGORY       => 'Catègorie',
    NS_CATEGORY_TALK  => 'Discussion_Catègorie'
);

$linkTrail = '/^([a-zàâçéèêîœôû·’æäåāăëēïīòöōùü‘]+)(.*)$/sDu';

$dateFormats = array(
    'mdy time' => 'H:i',
    'mdy date' => 'F j, Y',
    'mdy both' => 'F j, Y "a" H:i',

    'dmy time' => 'H:i',
    'dmy date' => 'j F Y',
    'dmy both' => 'j F Y "a" H:i',

    'ymd time' => 'H:i',
    'ymd date' => 'Y F j',
    'ymd both' => 'Y F j "a" H:i',
);

$separatorTransformTable = array( ',' => "\xc2\xa0", '.' => ',' );

$messages = array(
# User preference toggles
'tog-underline'               => 'Solegnér los lims :',
'tog-highlightbroken'         => 'Afichiér <a href="" class="new">en rojo</a> los lims vers les pâges pas ègzistentes (ôtrament : d’ense<a href="" class="internal">?</a>)',
'tog-justify'                 => 'Justifiar los paragrafos',
'tog-hideminor'               => 'Cachiér les novèles modificacions minores',
'tog-extendwatchlist'         => 'Utilisar la lista de siuvu mèlyorâ',
'tog-usenewrc'                => 'Utilisar los dèrriérs changements mèlyorâs (JavaScript)',
'tog-numberheadings'          => 'Numerotar ôtomaticament los titros',
'tog-showtoolbar'             => 'Montrar la bârra de menu d’èdicion (JavaScript)',
'tog-editondblclick'          => 'Doblo-clicar por modifiar una pâge (JavaScript)',
'tog-editsection'             => 'Modifiar una sèccion per los lims [modifiar]',
'tog-editsectiononrightclick' => 'Modifiar una sèccion en fassent un clic drêt sur son titro (JavaScript)',
'tog-showtoc'                 => 'Afichiér la trâbla de les matiéres (por les pâges èyent més de 3 sèccions)',
'tog-rememberpassword'        => "Sè rapelar de mon mot de pâssa (tèmouen (''cookie''))",
'tog-editwidth'               => 'Afichiér la fenétra d’èdicion en plêna largior',
'tog-watchcreations'          => 'Apondre les pâges que crèo a ma lista de siuvu',
'tog-watchdefault'            => 'Apondre les pâges que modifio a ma lista de siuvu',
'tog-watchmoves'              => 'Apondre les pâges que renomo a ma lista de siuvu',
'tog-watchdeletion'           => 'Apondre les pâges que suprimo a ma lista de siuvu',
'tog-minordefault'            => 'Considèrar mes modificacions coment minores per dèfôt',
'tog-previewontop'            => 'Montrar la prèvisualisacion en-dessus de la bouèta d’èdicion',
'tog-previewonfirst'          => 'Montrar la prèvisualisacion pendent la premiére èdicion',
'tog-nocache'                 => 'Dèsactivar la cache de les pâges',
'tog-enotifwatchlistpages'    => 'Ôtorisar l’èxpèdicion de mèl quand una pâge de voutra lista de siuvu est modifiâ',
'tog-enotifusertalkpages'     => 'M’avèrtir per mèl en câs de modificacion de ma pâge de discussion',
'tog-enotifminoredits'        => 'M’avèrtir per mèl mémo en câs de modificacion minora',
'tog-enotifrevealaddr'        => 'Afichiér mon adrèce èlèctronica dens los mèls d’avèrtissement',
'tog-shownumberswatching'     => 'Afichiér lo nombro d’utilisators que siuvont les pâges',
'tog-fancysig'                => 'Signatura bruta (sen lim ôtomatico)',
'tog-externaleditor'          => 'Utilisar un èditor de defôr per dèfôt',
'tog-externaldiff'            => 'Utilisar un comparator de defôr per dèfôt',
'tog-showjumplinks'           => 'Activar los lims « navigacion » et « rechèrche » d’amont de pâge (entèrfaces MySkin et ôtres)',
'tog-uselivepreview'          => 'Utilisar la vua rapida (JavaScript) (èxpèrimentâl)',
'tog-forceeditsummary'        => 'M’avèrtir quand j/y’é pas complètâ lo contegnu de la bouèta de rèsumâ',
'tog-watchlisthideown'        => 'Mâscar mes prôpres modificacions dens la lista de siuvu',
'tog-watchlisthidebots'       => 'Mâscar les modificacions fêtes per los bots dens la lista de siuvu',
'tog-watchlisthideminor'      => 'Mâscar les modificacions minores dens la lista de siuvu',
'tog-nolangconversion'        => 'Dèsactivar la convèrsion de les variantes de lengoua',
'tog-ccmeonemails'            => 'M’emmandar una copia des mèls que j/y’emmando ux ôtros utilisators',
'tog-diffonly'                => 'Pas montrar lo contegnu de les pâges desot los difs',
'tog-showhiddencats'          => 'Afichiér les catègories cachiês',

'underline-always'  => 'tojorn',
'underline-never'   => 'jamés',
'underline-default' => 'd’aprés lo navigator',

'skinpreview' => '(Prèvisualisar)',

# Dates
'sunday'        => 'demenge',
'monday'        => 'delon',
'tuesday'       => 'demârs',
'wednesday'     => 'demécro',
'thursday'      => 'dejô',
'friday'        => 'devendro',
'saturday'      => 'dessando',
'sun'           => 'dg',
'mon'           => 'dl',
'tue'           => 'dr',
'wed'           => 'dc',
'thu'           => 'dj',
'fri'           => 'dv',
'sat'           => 'ds',
'january'       => 'de janviér',
'february'      => 'de fevriér',
'march'         => 'de mârs',
'april'         => 'd’avril',
'may_long'      => 'de mê',
'june'          => 'de jouen',
'july'          => 'de julyèt',
'august'        => 'd’oût',
'september'     => 'de septembro',
'october'       => 'd’octobro',
'november'      => 'de novembro',
'december'      => 'de dècembro',
'january-gen'   => 'de janviér',
'february-gen'  => 'de fevriér',
'march-gen'     => 'de mârs',
'april-gen'     => 'd’avril',
'may-gen'       => 'de mê',
'june-gen'      => 'de jouen',
'july-gen'      => 'de julyèt',
'august-gen'    => 'd’oût',
'september-gen' => 'de septembro',
'october-gen'   => 'd’octobro',
'november-gen'  => 'de novembro',
'december-gen'  => 'de dècembro',
'jan'           => 'jan',
'feb'           => 'fev',
'mar'           => 'mâr',
'apr'           => 'avr',
'may'           => 'mê',
'jun'           => 'jou',
'jul'           => 'jul',
'aug'           => 'oût',
'sep'           => 'sep',
'oct'           => 'oct',
'nov'           => 'nov',
'dec'           => 'dèc',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|Catègorie|Catègories}}',
'category_header'                => 'Pâges dens la catègorie « $1 »',
'subcategories'                  => 'Sot-catègories',
'category-media-header'          => 'Fichiérs multimèdia dens la catègorie « $1 »',
'category-empty'                 => "''Ceta catègorie contint gins d’articllo, de sot-catègorie ou de fichiér multimèdia.''",
'hidden-categories'              => '{{PLURAL:$1|Catègorie cachiê|Catègories cachiês}}',
'hidden-category-category'       => 'Catègories cachiês', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|Ceta catègorie at ren que de la sot-catègorie siuventa.|Ceta catègorie at de {{PLURAL:$1|sot-catègorie|$1 sot-catègories}}, sur una soma de $2.}}',
'category-subcat-count-limited'  => 'Ceta catègorie at {{PLURAL:$1|de yona sot-catègorie|de $1 sot-catègories}}.',
'category-article-count'         => '{{PLURAL:$2|Ceta catègorie contint ren que la pâge siuventa.|{{PLURAL:$1|La pâge siuventa figure|Les $1 pâges siuventes figuront}} dens ceta catègorie, sur una soma de $2.}}',
'category-article-count-limited' => '{{PLURAL:$1|La pâge siuventa figure|Les $1 pâges siuventes figuront}} dens la presenta catègorie.',
'category-file-count'            => '{{PLURAL:$2|Ceta catègorie contint ren que lo fichiér siuvent.|{{PLURAL:$1|Lo fichiér siuvent figure|Los $1 fichiérs siuvents figuront}} dens ceta catègorie, sur una soma de $2.}}',
'category-file-count-limited'    => '{{PLURAL:$1|Lo fichiér siuvent figure|Los $1 fichiérs siuvents figuront}} dens la presenta catègorie.',
'listingcontinuesabbrev'         => '(suita)',

'mainpagetext'      => "<big>'''MediaWiki at étâ enstalâ avouéc reusséta.'''</big>",
'mainpagedocfooter' => 'Consultâd lo [http://meta.wikimedia.org/wiki/Aide:Contenu guido d’utilisator] por més d’enformacions sur l’usâjo de la programeria vouiqui.

== Dèmarrar avouéc MediaWiki ==

* [http://www.mediawiki.org/wiki/Manual:Configuration_settings Lista des paramètres de configuracion]
* [http://www.mediawiki.org/wiki/Manual:FAQ/fr FdeQ sur MediaWiki]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce Lista de discussion de les parucions de MediaWiki]',

'about'          => 'A propôs',
'article'        => 'Articllo',
'newwindow'      => '(ôvre una fenétra novèla)',
'cancel'         => 'Anular',
'qbfind'         => 'Rechèrchiér',
'qbbrowse'       => 'Dèfelar',
'qbedit'         => 'Èditar/modifiar',
'qbpageoptions'  => 'Pâge de chouèx',
'qbpageinfo'     => 'Pâge d’enformacion',
'qbmyoptions'    => 'Mos chouèx',
'qbspecialpages' => 'Pâges spèciâles',
'moredotdotdot'  => 'Et ples...',
'mypage'         => 'Pâge a sè',
'mytalk'         => 'Pâge de discussion',
'anontalk'       => 'Discussion avouéc ceta adrèce IP',
'navigation'     => 'Navigacion',
'and'            => 'et',

# Metadata in edit box
'metadata_help' => 'Mètabalyês :',

'errorpagetitle'    => 'Èrror de titro',
'returnto'          => 'Tornar a la pâge $1.',
'tagline'           => 'De {{SITENAME}}.',
'help'              => 'Éde',
'search'            => 'Rechèrche',
'searchbutton'      => 'Chèrchiér',
'go'                => 'Alar',
'searcharticle'     => 'Alar',
'history'           => 'Historico de la pâge',
'history_short'     => 'Historico',
'updatedmarker'     => 'betâ a jorn dês ma dèrriére visita',
'info_short'        => 'Enformacions',
'printableversion'  => 'Vèrsion emprimâbla',
'permalink'         => 'Lim fixo',
'print'             => 'Emprimar',
'edit'              => 'Èditar/modifiar',
'create'            => 'Crèar',
'editthispage'      => 'Modifiar ceta pâge',
'create-this-page'  => 'Crèar ceta pâge',
'delete'            => 'Suprimar',
'deletethispage'    => 'Suprimar ceta pâge',
'undelete_short'    => 'Rèstorar {{PLURAL:$1|yona modificacion|$1 modificacions}}',
'protect'           => 'Protègiér',
'protect_change'    => 'modifiar la protèccion',
'protectthispage'   => 'Protègiér ceta pâge',
'unprotect'         => 'Dèprotègiér',
'unprotectthispage' => 'Dèprotègiér ceta pâge',
'newpage'           => 'Novèla pâge',
'talkpage'          => 'Pâge de discussion',
'talkpagelinktext'  => 'Discutar',
'specialpage'       => 'Pâge spèciâla',
'personaltools'     => 'Outils a sè',
'postcomment'       => 'Apondre un comentèro',
'articlepage'       => 'Vêde l’articllo',
'talk'              => 'Discussion',
'views'             => 'Afichâjos',
'toolbox'           => 'Bouèta d’outils',
'userpage'          => 'Pâge utilisator',
'projectpage'       => 'Pâge mèta',
'imagepage'         => 'Pâge émâge',
'mediawikipage'     => 'Vêde la pâge du mèssâjo',
'templatepage'      => 'Vêde la pâge du modèlo',
'viewhelppage'      => 'Vêde la pâge d’éde',
'categorypage'      => 'Vêde la pâge de la catègorie',
'viewtalkpage'      => 'Pâge de discussion',
'otherlanguages'    => 'Ôtres lengoues',
'redirectedfrom'    => '(Redirigiê dês $1)',
'redirectpagesub'   => 'Pâge de redirèccion',
'lastmodifiedat'    => 'Dèrriére modificacion de ceta pâge lo $1 a $2.<br />', # $1 date, $2 time
'viewcount'         => 'Ceta pâge at étâ consultâ {{PLURAL:$1|yon côp|$1 côps}}.',
'protectedpage'     => 'Pâge protègiê',
'jumpto'            => 'Alar a :',
'jumptonavigation'  => 'Navigacion',
'jumptosearch'      => 'Rechèrche',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'A propôs de {{SITENAME}}',
'aboutpage'            => 'Project:A propôs',
'bugreports'           => 'Rapôrt d’èrrors',
'bugreportspage'       => 'Project:Rapôrt d’èrrors',
'copyright'            => 'Lo contegnu est disponiblo d’aprés los tèrmos de la licence $1.',
'copyrightpagename'    => 'licence {{SITENAME}}',
'copyrightpage'        => '{{ns:project}}:Drêt d’ôtor',
'currentevents'        => 'Novèles',
'currentevents-url'    => 'Project:Novèles',
'disclaimers'          => 'Avèrtissements',
'disclaimerpage'       => 'Project:Avèrtissements g·ènèrals',
'edithelp'             => 'Éde',
'edithelppage'         => 'Help:Coment èditar/modifiar una pâge',
'faq'                  => 'FdeQ',
'faqpage'              => 'Project:FdeQ',
'helppage'             => 'Help:Somèro',
'mainpage'             => 'Reçua',
'mainpage-description' => 'Reçua',
'policy-url'           => 'Project:Règlles de dedens',
'portal'               => 'Comunôtât',
'portal-url'           => 'Project:Reçua',
'privacy'              => 'Politica de confidencialitât',
'privacypage'          => 'Project:Politica de confidencialitât',

'badaccess'        => 'Èrror de pèrmission',
'badaccess-group0' => 'Vos avéd pas los drêts sufisents por rèalisar l’accion que vos demandâd.',
'badaccess-group1' => 'L’accion que vos tâchiéd de rèalisar est accèssibla ren qu’ux utilisators de la tropa $1.',
'badaccess-group2' => 'L’accion que vos tâchiéd de rèalisar est accèssibla ren qu’ux utilisators de les tropes $1.',
'badaccess-groups' => 'L’accion que vos tâchiéd de rèalisar est accèssibla ren qu’ux utilisators de les tropes $1.',

'versionrequired'     => 'Vèrsion $1 de MediaWiki nècèssèra',
'versionrequiredtext' => 'La vèrsion $1 de MediaWiki est nècèssèra por utilisar ceta pâge. Consultâd [[Special:Version|la pâge de les vèrsions]].',

'ok'                      => 'D’acôrd',
'retrievedfrom'           => 'Rècupèrâ de « $1 »',
'youhavenewmessages'      => 'Vos avéd de $1 ($2).',
'newmessageslink'         => 'mèssâjos novéls',
'newmessagesdifflink'     => 'dèrriére modificacion',
'youhavenewmessagesmulti' => 'Vos avéd de mèssâjos novéls dessus $1.',
'editsection'             => 'modifiar',
'editold'                 => 'modifiar',
'viewsourceold'           => 'vêre la sôrsa',
'editsectionhint'         => 'Modifiar la sèccion : $1',
'toc'                     => 'Somèro',
'showtoc'                 => 'afichiér',
'hidetoc'                 => 'mâscar',
'thisisdeleted'           => 'Dèsirâd-vos afichiér ou rèstorar $1 ?',
'viewdeleted'             => 'Vêre $1 ?',
'restorelink'             => '{{PLURAL:$1|yona modificacion èfaciê|$1 modificacions èfaciês}}',
'feedlinks'               => 'Flux :',
'feed-invalid'            => 'Tipo de flux envalido.',
'feed-unavailable'        => 'Los flux de sindicacion sont pas disponiblos dessus {{SITENAME}}',
'site-rss-feed'           => 'Flux RSS de $1',
'site-atom-feed'          => 'Flux Atom de $1',
'page-rss-feed'           => 'Flux RSS de « $1 »',
'page-atom-feed'          => 'Flux Atom de « $1 »',
'red-link-title'          => '$1 (portant pas rèdigiê)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Articllo',
'nstab-user'      => 'Pâge utilisator',
'nstab-media'     => 'Mèdia',
'nstab-special'   => 'Spèciâl',
'nstab-project'   => 'A propôs',
'nstab-image'     => 'Fichiér',
'nstab-mediawiki' => 'Mèssâjo',
'nstab-template'  => 'Modèlo',
'nstab-help'      => 'Éde',
'nstab-category'  => 'Catègorie',

# Main script and global functions
'nosuchaction'      => 'Accion encognua',
'nosuchactiontext'  => 'L’accion spècefiâ dens l’URL est pas recognua per lo vouiqui.',
'nosuchspecialpage' => 'Pâge spèciâla pas ègzistenta',
'nospecialpagetext' => "<big>'''Vos éd demandâ una pâge spèciâla qu’est pas recognua per lo vouiqui.'''</big>

Una lista de les pâges spèciâles pôt étre trovâ dessus [[Special:SpecialPages]].",

# General errors
'error'                => 'Èrror',
'databaseerror'        => 'Èrror de la bâsa de balyês',
'dberrortext'          => 'Èrror de sintaxa dens la bâsa de balyês.
Ceta èrror est pôt-étre diua a una requéta de rechèrche fôssa ou una èrror dens la programeria.
La dèrriére requéta trètâ per la bâsa de balyês ére :
<blockquote><tt>$1</tt></blockquote>
dês la fonccion « <tt>$2</tt> ».
MySQL at retornâ l’èrror « <tt>$3 : $4</tt> ».',
'dberrortextcl'        => 'Una requéta a la bâsa de balyês presente una èrror de sintaxa.
La dèrriére requéta emmandâ ére :
« $1 »
fêta per la fonccion « $2 ».
MySQL at retornâ l’èrror « $3 : $4 ».',
'noconnect'            => 'Dèsolâ ! Aprés des problèmos tècnicos, o est empossiblo de sè conèctar a la bâsa de balyês por lo moment.<br />
$1',
'nodb'                 => 'Empossiblo de sèlèccionar la bâsa de balyês $1',
'cachederror'          => 'Cela pâge est una vèrsion en cache et pôt pas étre a jorn.',
'laggedslavemode'      => 'Atencion : cela pâge pôt pas contegnir totes les dèrriéres modificacions fêtes.',
'readonly'             => 'Bâsa de balyês vèrrolyê',
'enterlockreason'      => 'Endicâd la rêson du vèrrolyâjo et pués una èstimacion de sa durâ',
'readonlytext'         => 'Ora les aponses et mises a jorn sur la bâsa de balyês sont blocâs, probâblament por pèrmetre la mantegnence de la bâsa, aprés què, tot rentrerat dedens l’ôrdre.

L’administrator èyent vèrrolyê la bâsa de balyês at balyê l’èxpllicacion siuventa : $1',
'missingarticle-rev'   => '(vèrsion # : $1)',
'missingarticle-diff'  => '(Dif : $1, $2)',
'readonly_lag'         => 'La bâsa de balyês at étâ ôtomaticament vèrrolyê pendent que los sèrviors secondèros ratrapont lor retârd sur lo sèrvior principâl.',
'internalerror'        => 'Èrror de dedens',
'internalerror_info'   => 'Èrror de dedens : $1',
'filecopyerror'        => 'Empossiblo de copiyér lo fichiér « $1 » vers « $2 ».',
'filerenameerror'      => 'Empossiblo de renomar lo fichiér « $1 » en « $2 ».',
'filedeleteerror'      => 'Empossiblo de suprimar lo fichiér « $1 ».',
'directorycreateerror' => 'Empossiblo de crèar lo dossiér « $1 ».',
'filenotfound'         => 'Empossiblo de trovar lo fichiér « $1 ».',
'fileexistserror'      => 'Empossiblo d’ècrire dens lo dossiér « $1 » : lo fichiér ègziste.',
'unexpected'           => 'Valor emprèvua : « $1 » = « $2 ».',
'formerror'            => 'Èrror : empossiblo de sometre lo formulèro',
'badarticleerror'      => 'Cela accion pôt pas étre fêta sur ceta pâge.',
'cannotdelete'         => 'Empossiblo de suprimar la pâge ou lo fichiér endicâ. (La suprèssion at pôt-étre ja étâ fêta per quârqu’un d’ôtro.)',
'badtitle'             => 'Môvés titro',
'badtitletext'         => 'Lo titro de la pâge demandâ est envalido, vouedo ou ben s’ag·ét d’un titro entèrlengoua ou entèrprojèt mâl-liyê. Contint pôt-étre yon ou plusiors caractèros que pôvont pas étre utilisâs dens los titros.',
'perfdisabled'         => 'Dèsolâ ! Cela fonccionalitât est temporèrament dèsactivâ perce que frène la bâsa de balyês que pas més nion pôt utilisar lo vouiqui.',
'perfcached'           => 'Cen est una vèrsion en cache et el est pôt-étre pas a jorn.',
'perfcachedts'         => 'Les balyês siuventes sont en cache, sont vêr pas per fôrce a jorn. La dèrriére actualisacion dâte du $1.',
'querypage-no-updates' => 'Ora les mises a jorn por ceta pâge sont dèsactivâs. Les balyês ce-desot sont pas betâs a jorn.',
'wrong_wfQuery_params' => 'Paramètres fôx dessus wfQuery()<br />
Fonccion : $1<br />
Requéta : $2',
'viewsource'           => 'Vêre lo tèxte sôrsa',
'viewsourcefor'        => 'por $1',
'actionthrottled'      => 'Accion limitâ',
'actionthrottledtext'  => 'Por combatre lo spame, l’usâjo de cela accion est limitâ a un cèrtin nombro de côps dens un moment prod côrt. S’acomplét que vos éd dèpassâ ceta limita. Tornâd èprovar dens doux-três menutes.',
'protectedpagetext'    => 'Ceta pâge at étâ protègiê por empachiér sa modificacion.',
'viewsourcetext'       => 'Vos pouede vêre et copiyér lo contegnu de la pâge por povêr travalyér dessus :',
'protectedinterface'   => 'Ceta pâge fornét du tèxte d’entèrface por la programeria et est protègiê por èvitar los abus.',
'editinginterface'     => "'''Atencion :''' vos éte aprés èditar una pâge utilisâ por crèar lo tèxte de l’entèrface de la programeria. Los changements sè cognetront, d’aprés lo contèxte, sur totes ou cèrtênes pâges visibles per los ôtros utilisators. Por les traduccions, nos vos envitens a utilisar lo seto [http://translatewiki.net/wiki/Main_Page?setlang=frp Betawiki], lo projèt d’entèrnacionalisacion des mèssâjos de MediaWiki.",
'sqlhidden'            => '(Requéta SQL cachiê)',
'cascadeprotected'     => 'Ora, ceta pâge est protègiê perce qu’el est encllua dens {{PLURAL:$1|la pâge siuventa|les pâges siuventes}}, èyent étâ protègiê{{PLURAL:$1||s}} avouéc lo chouèx « protèccion en cascâda » activâ :
$2',
'namespaceprotected'   => "Vos avéd pas la pèrmission de modifiar les pâges de l’èspâço de nom '''« $1 »'''.",
'customcssjsprotected' => 'Vos avéd pas la pèrmission d’èditar ceta pâge perce que contint des prèferences d’ôtros utilisators.',
'ns-specialprotected'  => 'Les pâges dens l’èspâço de nom « {{ns:special}} » pôvont pas étre modifiâs.',
'titleprotected'       => "Cél titro at étâ protègiê a la crèacion per [[User:$1|$1]].
La rêson avanciê est « ''$2'' ».",

# Login and logout pages
'logouttitle'                => 'Dèconèccion',
'logouttext'                 => "'''Orendrêt, vos éte dèconèctâ.'''<br />
Vos pouede continuar a utilisar {{SITENAME}} de façon anonima, ou ben vos tornar conèctar desot lo mémo nom ou un ôtro.",
'welcomecreation'            => '== Benvegnua, $1 ! ==

Voutron compto utilisator at étâ crèâ. Oubliâd pas de pèrsonalisar voutres prèferences dessus {{SITENAME}}.',
'loginpagetitle'             => 'Conèccion',
'yourname'                   => 'Voutron nom d’utilisator :',
'yourpassword'               => 'Voutron mot de pâssa :',
'yourpasswordagain'          => 'Tornâd entrar voutron mot de pâssa :',
'remembermypassword'         => "Sè rapelar de mon mot de pâssa (tèmouen (''cookie''))",
'yourdomainname'             => 'Voutron domêno :',
'externaldberror'            => 'Ou ben una èrror est arrevâ avouéc la bâsa de balyês d’ôtentificacion de defôr, ou ben vos éte pas ôtorisâ a betar a jorn voutron compto de defôr.',
'loginproblem'               => '<b>Problèmo d’identificacion.</b><br />Tornâd èprovar !',
'login'                      => 'Identificacion',
'nav-login-createaccount'    => 'Sè conèctar ou crèar un compto',
'loginprompt'                => "Vos dête activar los tèmouens (''cookies'') por étre conèctâ ôtomaticament a {{SITENAME}}.",
'userlogin'                  => 'Sè conèctar ou crèar un compto',
'logout'                     => 'Sè dèconèctar',
'userlogout'                 => 'Dèconèccion',
'notloggedin'                => 'Pas conèctâ',
'nologin'                    => 'Vos avéd pas de compto ? $1 (u chouèx).',
'nologinlink'                => 'Crèâd un compto',
'createaccount'              => 'Crèar un compto (u chouèx)',
'gotaccount'                 => 'Vos avéd ja un compto ? $1.',
'gotaccountlink'             => 'Identifiâd-vos',
'createaccountmail'          => 'per mèl',
'badretype'                  => 'Los mots de pâssa que vos éd buchiês sont pas identicos.',
'userexists'                 => 'Lo nom d’utilisator que vos éd buchiê est ja utilisâ. Volyéd nen chouèsir/cièrdre un ôtro.',
'youremail'                  => 'Adrèce de mèl :',
'username'                   => 'Nom d’utilisator :',
'uid'                        => 'Numerô d’utilisator :',
'yourrealname'               => 'Veré nom :',
'yourlanguage'               => 'Lengoua de l’entèrface :',
'yourvariant'                => 'Varianta :',
'yournick'                   => 'Signatura por les discussions :',
'badsig'                     => 'Signatura bruta fôssa ; controlâd voutres balises HTML.',
'badsiglength'               => 'Voutra signatura est trop longe : la talye la ples granta est de $1 caractèros.',
'email'                      => 'Mèl',
'prefs-help-realname'        => '(u chouèx) : se vos lo spècefiâd, serat utilisâ por l’atribucion de voutres contribucions.',
'loginerror'                 => 'Èrror d’identificacion',
'prefs-help-email'           => '(u chouèx) : pèrmèt de sè veriér vers vos dês lo seto sen dèvouèlar voutra identitât.',
'prefs-help-email-required'  => 'Una adrèce de mèl est requisa.',
'nocookiesnew'               => "Lo compto utilisator at étâ crèâ, mas vos éte pas conèctâ. {{SITENAME}} utilise des tèmouens (''cookies'') por la conèccion mas vos los éd dèsactivâs. Volyéd los activar et pués vos tornar conèctar avouéc lo mémo nom et lo mémo mot de pâssa.",
'nocookieslogin'             => "{{SITENAME}} utilise des tèmouens (''cookies'') por la conèccion mas vos los éd dèsactivâs. Volyéd los activar et pués vos tornar conèctar.",
'noname'                     => 'Vos éd pas buchiê un nom d’utilisator valido.',
'loginsuccesstitle'          => 'Identificacion reussia.',
'loginsuccess'               => "'''Orendrêt, vos éte conèctâ dessus {{SITENAME}} a titro de « $1 ».'''",
'nosuchuser'                 => 'L’utilisator « $1 » ègziste pas.
Controlâd que vos éd bien ortografiâ lo nom, ou ben utilisâd lo formulèro ce-desot por crèar un novél compto utilisator.',
'nosuchusershort'            => 'Y at pas de contributor avouéc lo nom « <nowiki>$1</nowiki> ». Volyéd controlar l’ortografia.',
'nouserspecified'            => 'Vos dête buchiér un nom d’utilisator.',
'wrongpassword'              => 'Lo mot de pâssa est fôx. Volyéd tornar èprovar.',
'wrongpasswordempty'         => 'Vos éd pas entrâ de mot de pâssa. Volyéd tornar èprovar.',
'passwordtooshort'           => 'Voutron mot de pâssa est trop côrt. Dêt contegnir u muens $1 caractèros et étre difèrent de voutron nom d’utilisator.',
'mailmypassword'             => 'Emmandâd-mè un novél mot de pâssa',
'passwordremindertitle'      => 'Voutron novél mot de pâssa temporèro dessus {{SITENAME}}',
'passwordremindertext'       => 'Quârqu’un (probâblament vos) èyent l’adrèce IP $1 at demandâ a cen qu’un novél mot de pâssa vos seye emmandâ por {{SITENAME}} ($4).
Lo mot de pâssa de l’utilisator « $2 » est ora « $3 ».
Nos vos conselyens de vos conèctar et de modifiar cél mot de pâssa setout que possiblo.

Se vos éte pas l’ôtor de cela demanda, ou se vos vos rapelâd ora de voutron viely mot de pâssa et que vos souhètâd pas més nen changiér, vos pouede ignorar ceti mèssâjo et continuar a utilisar voutron viely mot de pâssa.',
'noemail'                    => 'Niona adrèce de mèl at étâ enregistrâ por l’utilisator « $1 ».',
'passwordsent'               => 'Un novél mot de pâssa at étâ emmandâ a l’adrèce de mèl de l’utilisator « $1 ». Volyéd vos tornar conèctar aprés l’avêr reçu.',
'blocked-mailpassword'       => 'Voutra adrèce IP est blocâ en èdicion, la fonccion de rapèl du mot de pâssa est vêr dèsactivâ por èvitar los abus.',
'eauthentsent'               => 'Un mèl de confirmacion at étâ emmandâ a l’adrèce endicâ.
Devant qu’un ôtro mèl seye emmandâ a cél compto, vos devréd siuvre les enstruccions du mèl et confirmar que lo compto est ben lo voutro.',
'throttled-mailpassword'     => 'Un mèl de rapèl de voutron mot de pâssa at ja étâ emmandâ pendent les $1 hores passâs. Por èvitar los abus, solament yon mèl de rapèl serat emmandâ en $1 hores.',
'mailerror'                  => 'Èrror en emmandent lo mèl : $1',
'acct_creation_throttle_hit' => 'Dèsolâ, vos éd ja crèâ {{PLURAL:$1|yon compto|$1 comptos}}. Vos pouede pas nen crèar d’ôtros.',
'emailauthenticated'         => 'Voutra adrèce de mèl at étâ ôtentifiâ lo $1.',
'emailnotauthenticated'      => 'Voutra adrèce de mèl est <strong>p’oncor ôtentifiâ</strong>. Nion mèl serat emmandâ por châcuna de les fonccions siuventes.',
'noemailprefs'               => '<strong>Niona adrèce èlèctronica at étâ endicâ,</strong> les fonccions siuventes seront pas disponibles.',
'emailconfirmlink'           => 'Confirmâd voutra adrèce de mèl',
'invalidemailaddress'        => 'Ceta adrèce de mèl pôt pas étre accèptâ perce que semble avêr un format envalido. Volyéd entrar una adrèce valida ou lèssiér cél champ vouedo.',
'accountcreated'             => 'Compto crèâ.',
'accountcreatedtext'         => 'Lo compto utilisator por $1 at étâ crèâ.',
'createaccount-title'        => 'Crèacion d’un compto por {{SITENAME}}',
'createaccount-text'         => 'Quârqu’un at crèâ un compto por $2 dessus {{SITENAME}}
($4). Lo mot de pâssa por « $2 » est « $3 ». Vos devriâd uvrir una sèssion et pués changiér dês ora ceti mot de pâssa.

Ignorâd ceti mèssâjo se ceti compto at étâ crèâ per èrror.',
'loginlanguagelabel'         => 'Lengoua : $1',

# Password reset dialog
'resetpass'               => 'Remisa a zérô du mot de pâssa',
'resetpass_announce'      => 'Vos vos éte enregistrâ avouéc un mot de pâssa temporèro emmandâ per mèl. Por chavonar l’enregistrament, vos dête entrar un novél mot de pâssa ique :',
'resetpass_text'          => '<!-- Apond de tèxte ique -->',
'resetpass_header'        => 'Remisa a zérô du mot de pâssa',
'resetpass_submit'        => 'Changiér lo mot de pâssa et s’enregistrar',
'resetpass_success'       => 'Voutron mot de pâssa at étâ changiê avouéc reusséta ! Enregistrament en cors...',
'resetpass_bad_temporary' => 'Mot de pâssa temporèro envalido. Vos éd pôt-étre ja changiê voutron mot de pâssa avouéc reusséta, ou ben demandâ un novél mot de pâssa temporèro.',
'resetpass_forbidden'     => 'Los mots de pâssa pôvont pas étre changiês dessus {{SITENAME}}.',
'resetpass_missing'       => 'Niona balyê entrâ.',

# Edit page toolbar
'bold_sample'     => 'Tèxte grâs',
'bold_tip'        => 'Tèxte grâs',
'italic_sample'   => 'Tèxte étalico',
'italic_tip'      => 'Tèxte étalico',
'link_sample'     => 'Titro du lim',
'link_tip'        => 'Lim de dedens',
'extlink_sample'  => 'http://www.example.com titro du lim',
'extlink_tip'     => 'Lim de defôr (oubliâd pas lo prèfixe http://)',
'headline_sample' => 'Tèxte de sot-titro',
'headline_tip'    => 'Sot-titro nivô 2',
'math_sample'     => 'Entrâd voutra formula ique',
'math_tip'        => 'Formula matèmatica (LaTeX)',
'nowiki_sample'   => 'Entrâd lo tèxte pas formatâ ique',
'nowiki_tip'      => 'Ignorar la sintaxa vouiqui',
'image_sample'    => 'Ègzemplo.jpg',
'image_tip'       => 'Émâge entrebetâ',
'media_sample'    => 'Ègzemplo.ogg',
'media_tip'       => 'Lim vers un fichiér multimèdia',
'sig_tip'         => 'Voutra signatura avouéc la dâta',
'hr_tip'          => 'Legne plana (pas nen abusar)',

# Edit pages
'summary'                   => 'Rèsumâ&nbsp;',
'subject'                   => 'Sujèt/titro',
'minoredit'                 => 'Modificacion minora',
'watchthis'                 => 'Siuvre ceta pâge',
'savearticle'               => 'Sôvar ceta pâge',
'preview'                   => 'Prèvisualisacion',
'showpreview'               => 'Prèvisualisacion',
'showlivepreview'           => 'Vua rapida',
'showdiff'                  => 'Changements en cors',
'anoneditwarning'           => "'''Atencion :''' vos éte pas identifiâ. Voutra adrèce IP serat enregistrâ dens l’historico de ceta pâge.",
'missingsummary'            => "'''Atencion :''' vos éd pas modifiâ lo rèsumâ de voutra modificacion. Se vos tornâd clicar sur lo boton « Sôvar ceta pâge », la pâge serat sôvâ sen novél avèrtissement.",
'missingcommenttext'        => 'Volyéd fâre voutron comentèro ce-desot.',
'missingcommentheader'      => "'''Rapèl :''' vos éd pas forni de sujèt/titro a ceti comentèro. Se vos tornâd clicar dessus « Sôvar ceta pâge », voutra èdicion serat enregistrâ sen titro.",
'summary-preview'           => 'Prèvisualisacion du rèsumâ ',
'subject-preview'           => 'Prèvisualisacion du sujèt/titro ',
'blockedtitle'              => 'L’utilisator est blocâ.',
'blockedtext'               => "<big>'''Voutron compto utilisator ou voutra adrèce IP at étâ blocâ.'''</big>

Lo blocâjo at étâ fêt per $1 por la rêson siuventa : ''$2''.

* Comencement du blocâjo : $8
* Èxpiracion du blocâjo : $6
* Compto blocâ : $7

Vos pouede vos veriér vers $1 ou yon des ôtros [[{{MediaWiki:Grouppage-sysop}}|administrators]] por nen discutar. Vos pouede utilisar la fonccion « Emmandar un mèssâjo a ceti utilisator » ren que s’una adrèce de mèl valida est spècefiâ dens voutres [[Special:Preferences|prèferences]]. Voutra adrèce IP d’ora est $3 et voutron identifiant de blocâjo est #$5. Volyéd los spècefiar dens tota requéta.",
'autoblockedtext'           => "Voutra adrèce IP at étâ blocâ ôtomaticament perce qu’el at étâ utilisâ per un ôtro utilisator, lui-mémo blocâ per $1.
La rêson balyê est :

:''$2''

* Comencement du blocâjo : $8
* Èxpiracion du blocâjo : $6

Vos pouede vos veriér vers $1 ou yon des ôtros [[{{MediaWiki:Grouppage-sysop}}|administrators]] por nen discutar.

Se vos éd balyê una adrèce de mèl valida dens voutres [[Special:Preferences|prèferences]] et que son usâjo vos est pas dèfendu, vos pouede utilisar la fonccion « Emmandar un mèssâjo a ceti utilisator » por vos veriér vers un administrator.

Voutron identifiant de blocâjo est #$5.
Volyéd los spècefiar dens tota requéta.",
'blockednoreason'           => 'Niona rêson balyê',
'blockedoriginalsource'     => "Lo code sôrsa de '''$1''' est endicâ ce-desot :",
'blockededitsource'         => "Lo contegnu de '''voutres modificacions''' aplicâs a '''$1''' est endicâ ce-desot :",
'whitelistedittitle'        => 'Enregistrament nècèssèro por modifiar lo contegnu',
'whitelistedittext'         => 'Vos dête étre $1 por avêr la pèrmission de modifiar lo contegnu.',
'confirmedittitle'          => 'Validacion de l’adrèce de mèl nècèssèra por modifiar lo contegnu',
'confirmedittext'           => 'Vos dête confirmar voutra adrèce de mèl devant que modifiar {{SITENAME}}. Volyéd entrar et validar voutra adrèce èlèctronica avouéc la pâge [[Special:Preferences|prèferences]].',
'nosuchsectiontitle'        => 'Sèccion manquenta',
'nosuchsectiontext'         => 'Vos éd tâchiê de modifiar una sèccion qu’ègziste pas. Puésqu’y at pas de sèccion $1, y at pas d’endrêt yô que sôvar voutres modificacions.',
'loginreqtitle'             => 'Enregistrament nècèssèro',
'loginreqlink'              => 'conèctar',
'loginreqpagetext'          => 'Vos dête vos $1 por vêre les ôtres pâges.',
'accmailtitle'              => 'Mot de pâssa emmandâ.',
'accmailtext'               => 'Lo mot de pâssa de « $1 » at étâ emmandâ a l’adrèce $2.',
'newarticle'                => '(Novél)',
'newarticletext'            => "Vos éd siuvu un lim vers una pâge qu’ègziste p’oncor. Por crèar cela pâge, entrâd voutron tèxte dens la bouèta ce-desot (vos pouede consultar la [[{{MediaWiki:Helppage}}|pâge d’éde]] por més d’enformacion). Se vos éte arrevâ ice per èrror, clicâd sur lo boton '''retôrn''' de voutron navigator.",
'anontalkpagetext'          => "---- ''Vos éte sur la pâge de discussion d’un utilisator pas enregistrâ qu’at p’oncor crèâ un compto ou que l’utilise pas. Por cela rêson, nos devens utilisar son adrèce IP por l’identifiar. Una adrèce IP pôt étre partagiê per plusiors utilisators. Se vos éte un utilisator pas enregistrâ et se vos constatâd que des comentèros que vos regârdont pas vos ont étâ adrèciês, vos pouede vos [[Special:UserLogin|conèctar ou crèar un compto]] por èvitar tota confusion futura avouéc d’ôtros contributors pas enregistrâs.''",
'noarticletext'             => 'Y at por lo moment gins de tèxte sur ceta pâge ; vos pouede [[Special:Search/{{PAGENAME}}|fâre una rechèrche sur lo titro de ceta pâge]] ou ben [{{fullurl:{{NAMESPACE}}:{{FULLPAGENAME}}|action=edit}} la modifiar].',
'userpage-userdoesnotexist' => 'Lo compto utilisator « $1 » est pas enregistrâ. Endicâd se vos voléd crèar ou ben èditar cela pâge.',
'clearyourcache'            => "'''Nota :''' aprés avêr sôvâ, vos dête forciér lo rechargement de la pâge por vêre los changements : '''Mozilla / Firefox :''' ''Shift-Ctrl-R'' (''Shift-Cmd-R'' en '''Apple Mac'''), '''IE :''' ''Ctrl-F5'', '''Opera :''' ''F5'', '''Safari :''' ''⌘-R'', '''Konqueror :''' ''Ctrl-R''.",
'usercssjsyoucanpreview'    => "'''Combina :''' utilisâd lo boton « Prèvisualisacion » por èprovar voutra novèla fôlye CSS/JS devant que l’enregistrar.",
'usercsspreview'            => "'''Rapelâd-vos que vos éte aprés prèvisualisar voutra prôpra fôlye CSS et qu’el at p’oncor étâ enregistrâ !'''",
'userjspreview'             => "'''Rapelâd-vos que vos éte aprés visualisar ou èprovar voutron code JavaScript et qu’il at p’oncor étâ enregistrâ !'''",
'userinvalidcssjstitle'     => "'''Atencion :''' ègziste pas d’entèrface « $1 ». Rapelâd-vos que les pâges a sè avouéc èxtensions .css et .js utilisont des titros en petiôtes lètres aprés lo nom d’utilisator et la bârra de fraccion /.<br />D’ense, {{ns:user}}:Foo/monobook.css est valido, pendent que {{ns:user}}:Foo/Monobook.css serat una fôlye de stilo envalida.",
'updated'                   => '(Betâ a jorn)',
'note'                      => '<strong>Nota :</strong>',
'previewnote'               => '<strong>Atencion, ceti tèxte est ren qu’una prèvisualisacion et at p’oncor étâ sôvâ !</strong>',
'previewconflict'           => 'Ceta prèvisualisacion montre lo tèxte de la bouèta d’èdicion de d’amont tâl qu’aparètrat se vos chouèsésséd/cièrde de lo sôvar.',
'session_fail_preview'      => '<strong>Dèsolâ ! Nos povens pas enregistrar voutra modificacion a côsa d’una pèrta d’enformacions regardent voutra sèssion. Volyéd tornar èprovar. Se cen tôrne pas reussir, volyéd vos dèconèctar, et pués vos tornar conèctar.</strong>',
'session_fail_preview_html' => "<strong>Dèsolâ ! Nos povens pas enregistrar voutra modificacion a côsa d’una pèrta d’enformacions regardent voutra sèssion.</strong>

''L’HTML bruto étent activâ dessus {{SITENAME}}, la prèvisualisacion at étâ mâscâ por prèvegnir una ataca per JavaScript.''

<strong>Se la tentativa de modificacion ére lèg·itima, volyéd tornar èprovar. Se cen tôrne pas reussir, volyéd vos dèconèctar, et pués vos tornar conèctar.</strong>",
'token_suffix_mismatch'     => '<strong>Voutra èdicion at pas étâ accèptâ perce que voutron navigator at mècllâ los caractèros de ponctuacion dens l’identifiant d’èdicion. L’èdicion at étâ refusâ por empachiér la corrupcion du tèxte de l’articllo. Ceti problèmo arreve quand vos utilisâd un proxy anonimo avouéc problèmo.</strong>',
'editing'                   => 'Modificacion de $1',
'editingsection'            => 'Modificacion de $1 (sèccion)',
'editingcomment'            => 'Modificacion de $1 (comentèro)',
'editconflict'              => 'Conflit de modificacion : $1',
'explainconflict'           => "Ceta pâge at étâ sôvâ aprés que vos èyâd comenciê a la modifiar.
La zona d’èdicion de d’amont contint lo tèxte tâl qu’il est enregistrâ ora dens la bâsa de balyês.
Voutres modificacions aparèssont dens la zona d’èdicion de desot.
Vos voléd devêr aduire voutres modificacions u tèxte ègzistent.
'''Solèt''' lo tèxte de la zona de d’amont serat sôvâ.",
'yourtext'                  => 'Voutron tèxte',
'storedversion'             => 'Vèrsion enregistrâ',
'nonunicodebrowser'         => '<strong>ATENCION : voutron navigator supôrte pas l’unicode. Una solucion temporèra at étâ trovâ por vos pèrmetre de modifiar en tota suretât un articllo : los caractèros nan-ASCII aparètront dens voutra bouèta d’èdicion a titro de codes hèxadècimâls. Vos devriâd utilisar un navigator ples novél.</strong>',
'editingold'                => '<strong>ATENCION : vos éte aprés modifiar una vielye vèrsion de ceta pâge. Se vos sôvâd, totes les modificacions fêtes dês ceta vèrsion seront pèrdues.</strong>',
'yourdiff'                  => 'Difèrences',
'copyrightwarning'          => 'Totes les contribucions a {{SITENAME}} sont considèrâs coment publeyês desot los tèrmos de la $2 (vêde $1 por més de dètalys). Se vos dèsirâd pas que voutros ècrits seyont modifiâs et distribuâs a volontât, marci de pas los sometre ique.<br />
Vos nos assurâd asse-ben que vos éd cen ècrit vos-mémo, ou ben que vos l’éd copiyê d’una sôrsa que vint du domêno publico, ou d’una ressôrsa abada.<br />
<strong>UTILISÂD PAS D’ÔVRES DESOT DRÊT D’ÔTOR SEN ÔTORISACION ÈXPRÈSSA !</strong>',
'copyrightwarning2'         => 'Totes les contribucions a {{SITENAME}} pôvont étre modifiâs ou suprimâs per d’ôtros utilisators. Se vos dèsirâd pas que voutros ècrits seyont modifiâs et distribuâs a volontât, marci de pas los sometre ique.<br />
Vos nos assurâd asse-ben que vos éd cen ècrit vos-mémo, ou ben que vos l’éd copiyê d’una sôrsa que vint du domêno publico, ou d’una ressôrsa abada (vêde $1 por més de dètalys).<br />
<strong>UTILISÂD PAS D’ÔVRES DESOT DRÊT D’ÔTOR SEN ÔTORISACION ÈXPRÈSSA !</strong>',
'longpagewarning'           => '<strong>ATENCION : ceta pâge at una longior de $1 ko ;
cèrtins navigators g·èront mâl la modificacion de les pâges aprochient ou dèpassent 32 ko.
Pôt-étre devriâd-vos divisar la pâge en sèccions ples petiôtes.</strong>',
'longpageerror'             => '<strong>ÈRROR : lo tèxte que vos éd somês fât $1 ko, cen que dèpâsse la limita fixâ a $2 ko. Lo tèxte pôt pas étre sôvâ.</strong>',
'readonlywarning'           => '<strong>ATENCION : la bâsa de balyês at étâ vèrrolyê por mantegnence,
vos porréd vêr pas sôvar voutres modificacions d’abôrd. Vos pouede copiyér lo contegnu de la pâge dens un fichiér tèxte et pués lo sôvar por ples târd.</strong>',
'protectedpagewarning'      => '<strong>ATENCION : ceta pâge est protègiê.
Solèts los utilisators èyent lo statut d’administrator pôvont la modifiar.</strong>',
'semiprotectedpagewarning'  => "'''Nota :''' ceta pâge at étâ protègiê de façon que solèts los contributors enregistrâs pouessont la modifiar.",
'cascadeprotectedwarning'   => "'''ATENCION :''' ceta pâge at étâ protègiê por cen que solèts los administrators pouessont l’èditar. Cela protèccion at étâ fêta perce que ceta pâge est encllua dens {{PLURAL:$1|una pâge protègiê|des pâges protègiês}} avouéc la « protèccion en cascâda » activâ.",
'titleprotectedwarning'     => '<strong>ATENCION : ceta pâge at étâ protègiê de façon que solèts cèrtins utilisators pouessont la crèar.</strong>',
'templatesused'             => 'Modèlos utilisâs sur ceta pâge :',
'templatesusedpreview'      => 'Modèlos utilisâs dens ceta prèvisualisacion :',
'templatesusedsection'      => 'Modèlos utilisâs dens ceta sèccion :',
'template-protected'        => '(protègiê)',
'template-semiprotected'    => '(mié-protègiê)',
'hiddencategories'          => '{{PLURAL:$1|Catègorie cachiê|Catègories cachiês}} que ceta pâge est avouéc :',
'edittools'                 => '<!-- Tot tèxte entrâ ique serat afichiê desot les bouètes d’èdicion ou d’impôrt de fichiér. -->',
'nocreatetitle'             => 'Crèacion de pâge limitâ',
'nocreatetext'              => '{{SITENAME}} at rètrent la possibilitât de crèar de novèles pâges.
Vos pouede tornar arriér et pués modifiar una pâge ègzistenta, ou ben vos [[Special:UserLogin|conèctar ou crèar un compto]].',
'nocreate-loggedin'         => 'Vos avéd pas la pèrmission de crèar de novèles pâges dessus {{SITENAME}}.',
'permissionserrors'         => 'Èrror de pèrmissions',
'permissionserrorstext'     => 'Vos avéd pas la pèrmission de fâre l’opèracion demandâ por {{PLURAL:$1|la rêson siuventa|les rêsons siuventes}} :',
'recreate-deleted-warn'     => "'''Atencion : vos éte aprés recrèar una pâge qu’at étâ prècèdament suprimâ.'''

Demandâd-vos s’o est verément convegnâblo de la recrèar en vos refèrent u jornal de les suprèssions afichiê ce-desot :",

# "Undo" feature
'undo-success' => 'Cela modificacion vôt étre dèfêta. Volyéd confirmar los changements (visiblos d’avâl de ceta pâge), et pués sôvar se vos éte d’acôrd. Marci d’èxplicar l’anulacion dens la bouèta de rèsumâ.',
'undo-failure' => 'Cela modificacion pôt pas étre dèfêta : cen rentrerêt en conflit avouéc les modificacions entèrmèdières.',
'undo-summary' => 'Anulacion de les modificacions $1 de [[Special:Contributions/$2|$2]] ([[User talk:$2|Discussion]])',

# Account creation failure
'cantcreateaccounttitle' => 'Vos pouede pas crèar un compto.',
'cantcreateaccount-text' => "La crèacion de compto dês ceta adrèce IP ('''$1''') at étâ blocâ per [[User:$3|$3]].

La rêson balyê per $3 ére ''$2''.",

# History pages
'viewpagelogs'        => 'Vêde lo jornal de ceta pâge',
'nohistory'           => 'Ègziste pas d’historico por ceta pâge.',
'revnotfound'         => 'Vèrsion entrovâbla',
'revnotfoundtext'     => 'La vèrsion prècèdenta de cela pâge at pas possu étre retrovâ.
Volyéd controlar l’URL que vos éd utilisâ por arrevar a ceta pâge.',
'currentrev'          => 'Vèrsion d’ora',
'revisionasof'        => 'Vèrsion du $1',
'revision-info'       => 'Vèrsion du $1 per $2',
'previousrevision'    => '← Vèrsion prècèdenta',
'nextrevision'        => 'Vèrsion siuventa →',
'currentrevisionlink' => 'vêde la vèrsion corenta',
'cur'                 => 'ora',
'next'                => 'siuv',
'last'                => 'dif',
'page_first'          => 'prem',
'page_last'           => 'dèrr',
'histlegend'          => 'Lègenda : (ora) = difèrence avouéc la vèrsion d’ora,
(dif) = difèrence avouéc la vèrsion prècèdenta, <b>m</b> = modificacion minora.',
'deletedrev'          => '[suprimâ]',
'histfirst'           => 'Premiéres contribucions',
'histlast'            => 'Dèrriéres contribucions',
'historysize'         => '({{PLURAL:$1|1 octèt|$1 octèts}})',
'historyempty'        => '(vouedo)',

# Revision feed
'history-feed-title'          => 'Historico de les vèrsions',
'history-feed-description'    => 'Historico por ceta pâge sur lo vouiqui',
'history-feed-item-nocomment' => '$1 lo $2', # user at time
'history-feed-empty'          => 'La pâge demandâ ègziste pas.
El at pôt-étre étâ suprimâ du vouiqui ou renomâ.
Vos pouede tâchiér de [[Special:Search|rechèrchiér dens lo vouiqui]] des novèles pâges que vont avouéc.',

# Revision deletion
'rev-deleted-comment'         => '(comentèro suprimâ)',
'rev-deleted-user'            => '(nom d’utilisator suprimâ)',
'rev-deleted-event'           => '(entrâ suprimâ)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">
Ceta vèrsion de la pâge at étâ enlevâ des arch·ives publiques.
Pôt y avêr des dètalys dens lo [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} jornal de les suprèssions].
</div>',
'rev-deleted-text-view'       => '<div class="mw-warning plainlinks">
Ceta vèrsion de la pâge at étâ enlevâ des arch·ives publiques.
A titro d’administrator de ceti seto, vos pouede la visualisar ;
pôt y avêr des dètalys dens lo [{{fullurl:Special:Log/delete|page={{PAGENAMEE}}}} jornal de les suprèssions].
</div>',
'rev-delundel'                => 'afichiér/mâscar',
'revisiondelete'              => 'Suprimar/rèstorar des vèrsions',
'revdelete-nooldid-title'     => 'Pas de ciba por la vèrsion',
'revdelete-nooldid-text'      => 'Vos éd pas spècefiâ la vèrsion ciba ou ben les vèrsions cibes por utilisar cela fonccion.',
'revdelete-selected'          => "{{PLURAL:$2|Vèrsion sèlèccionâ|Vèrsions sèlèccionâs}} de '''$1''' :",
'logdelete-selected'          => '{{PLURAL:$1|Èvènement de jornal sèlèccionâ|Èvènements de jornal sèlèccionâs}}:',
'revdelete-text'              => 'Les vèrsions et los èvènements suprimâs aparètront adés dens l’historico de l’articllo et los jornals,
mas lor contegnu tèxtuèl serat pas accèssiblo u publico.

D’ôtros administrators de {{SITENAME}} porront tojorn arrevar u contegnu cachiê et lo tornar rèstorar
a travèrs de cela méma entèrface, a muens qu’una rèstriccion suplèmentèra seye betâ en place per los opèrators du seto.',
'revdelete-legend'            => 'Betar en place des rèstriccions de vèrsion',
'revdelete-hide-text'         => 'Mâscar lo tèxte de la vèrsion',
'revdelete-hide-name'         => 'Mâscar l’accion et la ciba',
'revdelete-hide-comment'      => 'Mâscar lo comentèro de modificacion',
'revdelete-hide-user'         => 'Mâscar lo psudô ou l’adrèce IP du contributor.',
'revdelete-hide-restricted'   => 'Aplicar cetes rèstriccions ux administrators et pués ux ôtros utilisators',
'revdelete-suppress'          => 'Suprimar les balyês des administrators et des ôtros',
'revdelete-hide-image'        => 'Mâscar lo contegnu du fichiér',
'revdelete-unsuppress'        => 'Enlevar les rèstriccions sur les vèrsions rèstorâs',
'revdelete-log'               => 'Comentèro por lo jornal :',
'revdelete-submit'            => 'Aplicar a la vèrsion sèlèccionâ',
'revdelete-logentry'          => 'La visibilitât de la vèrsion at étâ modifiâ por [[$1]]',
'logdelete-logentry'          => 'La visibilitât de l’èvènement at étâ modifiâ por [[$1]]',
'revdelete-success'           => "'''Visibilitât de les vèrsions changiê avouéc reusséta.'''",
'logdelete-success'           => "'''Visibilitât des èvènements changiê avouéc reusséta.'''",

# History merging
'mergehistory'                     => 'Fusion des historicos d’una pâge',
'mergehistory-header'              => 'Ceta pâge vos pèrmèt de fusionar les vèrsions de l’historico d’una pâge d’origina vers una novèla.
Assurâd-vos que cél changement pouesse consèrvar la continuitât de l’historico.',
'mergehistory-box'                 => 'Fusionar les vèrsions de doves pâges :',
'mergehistory-from'                => 'Pâge d’origina :',
'mergehistory-into'                => 'Pâge de dèstinacion :',
'mergehistory-list'                => 'Èdicion des historicos fusionâblos',
'mergehistory-merge'               => 'Les vèrsions siuventes de [[:$1]] pôvont étre fusionâs avouéc [[:$2]]. Utilisâd lo boton de chouèx de la colona por fusionar ren que les vèrsions crèâs du comencement tant qu’a la dâta endicâ. Notâd bien que l’usâjo des lims de navigacion tornerat inicialisar la colona.',
'mergehistory-go'                  => 'Vêre les èdicions fusionâbles',
'mergehistory-submit'              => 'Fusionar les vèrsions',
'mergehistory-empty'               => 'Niona vèrsion pôt étre fusionâ.',
'mergehistory-success'             => '$3 {{PLURAL:$3|vèrsion|vèrsions}} de [[:$1]] {{PLURAL:$3|fusionâ|fusionâs}} avouéc reusséta avouéc [[:$2]].',
'mergehistory-fail'                => 'Empossiblo de fâre la fusion des historicos. Tornâd sèlèccionar la pâge et pués los paramètres de dâta.',
'mergehistory-no-source'           => 'La pâge d’origina $1 ègziste pas.',
'mergehistory-no-destination'      => 'La pâge de dèstinacion $1 ègziste pas.',
'mergehistory-invalid-source'      => 'La pâge d’origina dêt avêr un titro valido.',
'mergehistory-invalid-destination' => 'La pâge de dèstinacion dêt avêr un titro valido.',

# Merge log
'mergelog'           => 'Jornal de les fusions',
'pagemerge-logentry' => '[[$1]] fusionâ avouéc [[$2]] (vèrsions tant qu’u $3)',
'revertmerge'        => 'Sèparar',
'mergelogpagetext'   => 'Vê-que, ce-desot, la lista de les fusions les ples novèles de l’historico d’una pâge avouéc una ôtra.',

# Diffs
'history-title'           => 'Historico de les vèrsions de « $1 »',
'difference'              => '(Difèrences entre les vèrsions)',
'lineno'                  => 'Legne $1 :',
'compareselectedversions' => 'Comparar les vèrsions sèlèccionâs',
'editundo'                => 'dèfâre',
'diff-multi'              => '({{PLURAL:$1|Yona vèrsion entèrmèdièra mâscâ|$1 vèrsions entèrmèdières mâscâs}}.)',

# Search results
'searchresults'         => 'Rèsultats de la rechèrche',
'searchresulttext'      => 'Por més d’enformacions sur la rechèrche dens {{SITENAME}}, vêde [[{{MediaWiki:Helppage}}|{{int:help}}]].',
'searchsubtitle'        => "Vos éd rechèrchiê '''« [[:$1]] »'''.",
'searchsubtitleinvalid' => "Vos éd rechèrchiê '''« $1 »'''.",
'noexactmatch'          => "'''Niona pâge avouéc lo titro « $1 » ègziste.''' Vos pouede [[:$1|crèar cél articllo]].",
'noexactmatch-nocreate' => "'''Ègziste gins de pâge avouéc lo titro « $1 ».'''",
'toomanymatches'        => 'Trop d’ocasions ont étâ trovâs, vos éte preyê de sometre una requéta difèrenta.',
'titlematches'          => 'Corrèspondances dens los titros d’articllos',
'notitlematches'        => 'Nion titro d’articllo corrèspond a la rechèrche.',
'textmatches'           => 'Corrèspondances dens lo tèxte d’articllos',
'notextmatches'         => 'Nion tèxte d’articllo corrèspond a la rechèrche.',
'prevn'                 => '$1 prècèdents',
'nextn'                 => '$1 siuvents',
'viewprevnext'          => 'Vêre ($1) ($2) ($3).',
'search-result-size'    => '$1 ({{PLURAL:$2|1 mot|$2 mots}})',
'search-result-score'   => 'Rapôrt : $1%',
'searchall'             => 'Tôs',
'showingresults'        => 'Afichâjo de <b>$1</b> {{PLURAL:$1|rèsultat|rèsultats}} dês lo #<b>$2</b>.',
'showingresultsnum'     => 'Afichâjo de <b>$3</b> {{PLURAL:$3|rèsultat|rèsultats}} dês lo #<b>$2</b>.',
'nonefound'             => '<strong>Nota :</strong> l’absence de rèsultat est sovent diua a l’usâjo de tèrmos de rechèrche trop corents,
coment « a » ou « de », que sont pas endèxâs, ou ben a l’usâjo de plusiors tèrmos de rechèrche
(solètes les pâges contegnent tôs los tèrmos aparèssont dens los rèsultats).',
'powersearch'           => 'Rechèrche avanciê',
'searchdisabled'        => 'La rechèrche dessus {{SITENAME}} est dèsactivâ. En atendent la rèactivacion, vos pouede fâre una rechèrche per Google.
Atencion, lor endèxacion du contegnu de {{SITENAME}} pôt pas étre a jorn.',

# Preferences page
'preferences'              => 'Prèferences',
'mypreferences'            => 'Prèferences',
'prefs-edits'              => 'Nombro d’èdicions :',
'prefsnologin'             => 'Pas conèctâ',
'prefsnologintext'         => 'Vos dête étre [[Special:UserLogin|conèctâ]] por modifiar voutres prèferences d’utilisator.',
'prefsreset'               => 'Les prèferences ont étâ rètablies dês la vèrsion enregistrâ.',
'qbsettings'               => 'Bârra d’outils',
'qbsettings-none'          => 'Niona',
'qbsettings-fixedleft'     => 'Gôche',
'qbsettings-fixedright'    => 'Drêta',
'qbsettings-floatingleft'  => 'Fllotenta a gôche',
'qbsettings-floatingright' => 'Fllotenta a drêta',
'changepassword'           => 'Modificacion du mot de pâssa',
'skin'                     => 'Entèrface',
'math'                     => 'Rendu de les formules matèmatiques',
'dateformat'               => 'Format de dâta',
'datedefault'              => 'Niona prèference',
'datetime'                 => 'Dâta et hora',
'math_failure'             => 'Èrror d’analisa sintaxica',
'math_unknown_error'       => 'èrror endètèrmenâ',
'math_unknown_function'    => 'fonccion encognua',
'math_lexing_error'        => 'èrror lèxicâla',
'math_syntax_error'        => 'èrror de sintaxa',
'math_image_error'         => 'La convèrsion en PNG at pas reussia ; controlâd l’enstalacion de LaTeX, dvips, gs et convert',
'math_bad_tmpdir'          => 'Empossiblo de crèar ou d’ècrire dens lo rèpèrtouèro math temporèro',
'math_bad_output'          => 'Empossiblo de crèar ou d’ècrire dens lo rèpèrtouèro math de sortia',
'math_notexvc'             => 'L’ègzécutâblo « texvc » est entrovâblo. Liéséd math/README por lo configurar.',
'prefs-personal'           => 'Enformacions a sè',
'prefs-rc'                 => 'Dèrriérs changements',
'prefs-watchlist'          => 'Lista de siuvu',
'prefs-watchlist-days'     => 'Nombro de jorns a afichiér dens la lista de siuvu :',
'prefs-watchlist-edits'    => 'Nombro de modificacions a afichiér dens la lista de siuvu ètendua :',
'prefs-misc'               => 'Prèferences de totes sôrtes',
'saveprefs'                => 'Enregistrar les prèferences',
'resetprefs'               => 'Rètablir les prèferences',
'oldpassword'              => 'Viely mot de pâssa :',
'newpassword'              => 'Novél mot de pâssa :',
'retypenew'                => 'Confirmar lo novél mot de pâssa :',
'textboxsize'              => 'Fenétra d’èdicion',
'rows'                     => 'Renchiês :',
'columns'                  => 'Colones :',
'searchresultshead'        => 'Rechèrche',
'resultsperpage'           => 'Nombro de rèponses per pâge :',
'contextlines'             => 'Nombro de legnes per rèponsa :',
'contextchars'             => 'Nombro de caractèros de contèxte per legne :',
'stub-threshold'           => 'Limita supèriora por los <a href="#" class="stub">lims vers los començons</a> (octèts) :',
'recentchangesdays'        => 'Nombro de jorns a afichiér dens los dèrriérs changements :',
'recentchangescount'       => 'Nombro de modificacions a afichiér dens los dèrriérs changements :',
'savedprefs'               => 'Les prèferences ont étâ sôvâs.',
'timezonelegend'           => 'Fus horèro',
'timezonetext'             => '¹Nombro d’hores de dècalâjo entre-mié voutra hora locala et l’hora du sèrvior (UTC).',
'localtime'                => 'Hora locala :',
'timezoneoffset'           => 'Dècalâjo horèro¹ :',
'servertime'               => 'Hora du sèrvior :',
'guesstimezone'            => 'Utilisar la valor du navigator',
'allowemail'               => 'Ôtorisar l’èxpèdicion de mèl vegnent d’ôtros utilisators',
'defaultns'                => 'Rechèrchiér per dèfôt dens cetos èspâços de nom :',
'default'                  => 'dèfôt',
'files'                    => 'Fichiérs',

# User rights
'userrights'               => 'Maneyance des drêts d’utilisator', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'   => 'Maneyance des drêts d’utilisator',
'userrights-user-editname' => 'Entrâd un nom d’utilisator :',
'editusergroup'            => 'Modificacion de les tropes d’utilisators',
'editinguser'              => "Modificacion des drêts d’utilisator de '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup' => 'Modifiar les tropes de l’utilisator',
'saveusergroups'           => 'Sôvar les tropes de l’utilisator',
'userrights-groupsmember'  => 'Membro de :',
'userrights-reason'        => 'Rêson du changement :',
'userrights-no-interwiki'  => 'Vos avéd pas la pèrmission de modifiar los drêts des utilisators dessus d’ôtros vouiquis.',
'userrights-nodatabase'    => 'La bâsa de balyês « $1 » ègziste pas ou ben el est pas una bâsa de balyês locala.',
'userrights-nologin'       => 'Vos dête vos [[Special:UserLogin|conèctar]] avouéc un compto administrator por balyér los drêts d’utilisator.',
'userrights-notallowed'    => 'Voutron compto at pas la pèrmission de balyér des drêts d’utilisator.',

# Groups
'group'               => 'Tropa :',
'group-autoconfirmed' => 'Utilisators enregistrâs',
'group-bot'           => 'Bots',
'group-sysop'         => 'Administrators',
'group-bureaucrat'    => 'Burôcrates',
'group-all'           => 'Tôs',

'group-autoconfirmed-member' => 'Utilisator enregistrâ',
'group-bot-member'           => 'Bot',
'group-sysop-member'         => 'Administrator',
'group-bureaucrat-member'    => 'Burôcrate',

'grouppage-autoconfirmed' => '{{ns:project}}:Utilisators enregistrâs',
'grouppage-bot'           => '{{ns:project}}:Bots',
'grouppage-sysop'         => '{{ns:project}}:Administrators',
'grouppage-bureaucrat'    => '{{ns:project}}:Burôcrates',

# User rights log
'rightslog'      => 'Historico de les modificacions de statut',
'rightslogtext'  => 'Cen est un jornal de les modificacions de statut d’utilisator.',
'rightslogentry' => 'at modifiâ los drêts de l’utilisator « $1 » de $2 a $3',
'rightsnone'     => '(nion)',

# Recent changes
'nchanges'                          => '$1 {{PLURAL:$1|modificacion|modificacions}}',
'recentchanges'                     => 'Dèrriérs changements',
'recentchangestext'                 => 'Siude sur ceta pâge los dèrriérs changements de {{SITENAME}}.',
'recentchanges-feed-description'    => 'Siude los dèrriérs changements de ceti vouiqui dens un flux.',
'rcnote'                            => 'Vê-que {{PLURAL:$1|la dèrriére modificacion|les <b>$1</b> dèrriéres modificacions}} dês {{PLURAL:$2|lo jorn passâ|los <b>$2</b> jorns passâs}}, dètèrmenâ{{PLURAL:$1||s}} ceti $3.',
'rcnotefrom'                        => "Vê-que les modificacions fêtes dês lo '''$2''' ('''$1''' u fin ples).",
'rclistfrom'                        => 'Afichiér les novèles modificacions dês lo $1.',
'rcshowhideminor'                   => '$1 les modificacions minores',
'rcshowhidebots'                    => '$1 los bots',
'rcshowhideliu'                     => '$1 los utilisators enregistrâs',
'rcshowhideanons'                   => '$1 les contribucions d’IP',
'rcshowhidepatr'                    => '$1 les èdicions survelyês',
'rcshowhidemine'                    => '$1 mes contribucions',
'rclinks'                           => 'Afichiér les $1 dèrriéres modificacions fêtes pendent los $2 jorns passâs&nbsp;;<br/ >$3.',
'diff'                              => 'dif',
'hist'                              => 'hist',
'hide'                              => 'mâscar',
'show'                              => 'afichiér',
'minoreditletter'                   => 'm',
'newpageletter'                     => 'N',
'boteditletter'                     => 'b',
'number_of_watching_users_pageview' => '[$1 {{PLURAL:$1|utilisator siuvent|utilisators siuvents}}]',
'rc_categories'                     => 'Limita de les catègories (sèparacion avouéc « | »)',
'rc_categories_any'                 => 'Totes',
'newsectionsummary'                 => '/* $1 */ novèla sèccion',

# Recent changes linked
'recentchangeslinked'          => 'Siuvu des lims',
'recentchangeslinked-title'    => 'Siuvu des lims liyês a "$1"',
'recentchangeslinked-noresult' => 'Nion changement sur les pâges liyês pendent la pèrioda chouèsia/cièrdua.',
'recentchangeslinked-summary'  => "Ceta pâge spèciâla montre los dèrriérs changements sur les pâges que sont liyês. Les pâges de voutra lista de siuvu sont '''en grâs'''.",

# Upload
'upload'                      => 'Importar un fichiér',
'uploadbtn'                   => 'Importar lo fichiér',
'reupload'                    => 'Relevar',
'reuploaddesc'                => 'Retôrn u formulèro.',
'uploadnologin'               => 'Pas conèctâ',
'uploadnologintext'           => 'Vos dête étre [[Special:UserLogin|conèctâ]] por copiyér des fichiérs sur lo sèrvior.',
'upload_directory_read_only'  => 'Lo sèrvior Vouèbe pôt pas ècrire dens lo dossiér ciba ($1).',
'uploaderror'                 => 'Èrror',
'uploadtext'                  => "Utilisâd ceti formulèro por copiyér des fichiérs, por vêre ou rechèrchiér des émâges prècèdament copiyês consultâd la [[Special:ImageList|lista de fichiérs copiyês]], les copies et suprèssions sont asse-ben enregistrâs dens lo [[Special:Log/upload|jornal de les copies]].

Por encllure una émâge dens una pâge, utilisâd un lim de la fôrma :
* '''<nowiki>[[</nowiki>{{ns:image}}<nowiki>:Fichiér.jpg]]</nowiki>'''
* '''<nowiki>[[</nowiki>{{ns:image}}<nowiki>:Fichiér.png|tèxte altèrnatif]]</nowiki>'''
ou ben por liyér tot drêt vers lo fichiér :
* '''<nowiki>[[</nowiki>{{ns:media}}<nowiki>:Fichiér.ogg]]</nowiki>'''",
'upload-permitted'            => 'Formats de fichiérs ôtorisâs : $1.',
'upload-preferred'            => 'Formats de fichiérs prèferâs : $1.',
'upload-prohibited'           => 'Formats de fichiérs dèfendus : $1.',
'uploadlog'                   => 'Historico de les importacions',
'uploadlogpage'               => 'Historico de les importacions de fichiérs multimèdia',
'uploadlogpagetext'           => 'Vê-que la lista des dèrriérs fichiérs copiyês sur lo sèrvior.',
'filename'                    => 'Nom du fichiér',
'filedesc'                    => 'Dèscripcion',
'fileuploadsummary'           => 'Dèscripcion, sôrsa (ôtor, seto Malyâjo...) :',
'filestatus'                  => 'Statut du drêt d’ôtor&nbsp;:',
'filesource'                  => 'Sôrsa&nbsp;:',
'uploadedfiles'               => 'Fichiérs copiyês',
'ignorewarning'               => 'Ignorar l’avèrtissement et sôvar lo fichiér',
'ignorewarnings'              => 'Ignorar los avèrtissements pendent l’impôrt',
'minlength1'                  => 'Los noms de fichiér dêvont comprendre u muens yona lètra.',
'illegalfilename'             => 'Lo nom de fichiér « $1 » contint des caractèros dèfendus dens los titros de pâges. Marci de lo renomar et de lo relevar.',
'badfilename'                 => 'L’émâge at étâ renomâ en « $1 ».',
'filetype-badmime'            => 'Los fichiérs du tipo MIME « $1 » pôvont pas étre importâs.',
'filetype-unwanted-type'      => "'''« .$1 »''' est d’un format pas dèsirâ. Celos que sont prèferâs sont $2.",
'filetype-banned-type'        => "'''« .$1 »''' est dens un format pas admês. Celos que sont accèptâs sont $2.",
'filetype-missing'            => 'Lo fichiér at gins d’èxtension (coment « .jpg » per ègzemplo).',
'large-file'                  => 'Los fichiérs importâs devriant pas étre ples grôs que $1 ; ceti fichiér fât $2.',
'largefileserver'             => 'La talye de ceti fichiér est d’amont lo nivô lo ples hôt ôtorisâ.',
'emptyfile'                   => 'Lo fichiér que vos voléd importar semble vouedo. Cen pôt étre diu a una èrror dens lo nom du fichiér. Volyéd controlar que vos dèsirâd franc copiyér ceti fichiér.',
'fileexists'                  => 'Un fichiér avouéc ceti nom ègziste ja. Marci de controlar <strong><tt>$1</tt></strong>. Éte-vos de sûr de volêr lo modifiar ?',
'filepageexists'              => 'La pâge de dèscripcion por ceti fichiér at ja étâ crèâ ique <strong><tt>$1</tt></strong>, mas nion fichiér de ceti nom ègziste ora. Lo rèsumâ que vos voléd ècrire remplacierat pas lo tèxte prècèdent ; por cen fâre vos devréd èditar manuèlament la pâge.',
'fileexists-extension'        => 'Un fichiér avouéc un nom semblâblo ègziste ja :<br />
Nom du fichiér a importar : <strong><tt>$1</tt></strong><br />
Nom du fichiér ègzistent : <strong><tt>$2</tt></strong><br />
la solèta difèrence est la câssa (grantes lètres / petiôtes lètres) de l’èxtension. Volyéd controlar que lo fichiér est difèrent et changiér son nom.',
'fileexists-thumb'            => "<center>'''Émâge ègzistenta'''</center>",
'fileexists-thumbnail-yes'    => 'Lo fichiér semble étre una émâge en talye rèduita <i>(figura)</i>. Volyéd controlar lo fichiér <strong><tt>$1</tt></strong>.<br />
Se lo fichiér controlâ est la méma émâge (dens una rèsolucion mèlyora), y at pas fôta d’importar una vèrsion rèduita.',
'file-thumbnail-no'           => 'Lo nom du fichiér comence per <strong><tt>$1</tt></strong>. O est possiblo que s’ag·ésse d’una vèrsion rèduita <i>(figura)</i>.
Se vos disposâd du fichiér en rèsolucion hôta, importâd-lo, ôtrament volyéd changiér lo nom du fichiér.',
'fileexists-forbidden'        => 'Un fichiér avouéc ceti nom ègziste ja ; marci de tornar arriér et de copiyér lo fichiér desot un novél nom. [[Image:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => 'Un fichiér portent lo mémo nom ègziste ja dens la bâsa de balyês comena ; volyéd tornar arriér et pués l’emmandar desot un novél nom. [[Image:$1|thumb|center|$1]]',
'successfulupload'            => 'Copia reussia',
'uploadwarning'               => 'Atencion !',
'savefile'                    => 'Sôvar lo fichiér',
'uploadedimage'               => 'at importâ « [[$1]] »',
'overwroteimage'              => 'at importâ una novèla vèrsion de « [[$1]] »',
'uploaddisabled'              => 'Dèsolâ, l’èxpèdicion de fichiér est dèsactivâ.',
'uploaddisabledtext'          => 'L’èxpèdicion de fichiérs est dèsactivâ dessus {{SITENAME}}.',
'uploadscripted'              => 'Ceti fichiér contint de code HTML ou ben un script que porrêt étre entèrprètâ de façon fôssa per un navigator Malyâjo.',
'uploadcorrupt'               => 'Ceti fichiér est corrompu, il at una talye nula ou ben una èxtension envalida.
Volyéd controlar lo fichiér.',
'uploadvirus'                 => 'Ceti fichiér contint un virus ! Por més de dètalys, consultâd : $1',
'sourcefilename'              => 'Nom du fichiér a importar&nbsp;:',
'destfilename'                => 'Nom desot loquint lo fichiér serat enregistrâ&nbsp;:',
'watchthisupload'             => 'Siuvre ceti fichiér',
'filewasdeleted'              => 'Un fichiér avouéc ceti nom at ja étâ copiyê, et pués suprimâ. Vos devriâd controlar lo $1 devant que fâre una novèla copia.',
'upload-wasdeleted'           => "'''Atencion : vos éte aprés importar un fichiér qu’at ja étâ suprimâ dês devant.'''

Vos devriâd considèrar s’o est convegnâblo de continuar l’impôrt de cél fichiér. Lo jornal de les suprèssions vos barat los èlèments d’enformacion.",
'filename-bad-prefix'         => 'Lo nom du fichiér que vos importâd comence per <strong>« $1 »</strong> qu’est un nom g·ènèralament balyê per los aparèlys-fotô numericos et que dècrit pas lo fichiér. Volyéd chouèsir/cièrdre un nom de fichiér dècrisent voutron fichiér.',
'filename-prefix-blacklist'   => '  #<!-- lèssiéd ceta legne justo d’ense --> <pre>
# La sintaxa est la siuventa :
#  * Tot caractèro siuvent « # » tant qu’a la fin de la legne serat entèrprètâ coment un comentèro.
#  * Tota legne pas voueda est un prèfixe por noms de fichiér qu’est g·ènèralament balyê per los aparèlys-fotô numericos.
CIMG # Casio
DSC_ # Nikon
DSCF # Fuji
DSCN # Nikon
DUW # cèrtins enfatâblos
IMG # comon
JD # Jenoptik
MGP # Pentax
PICT # de totes sôrtes
  #</pre> <!-- lèssiéd ceta legne justo d’ense -->',

'upload-proto-error'      => 'Protocolo fôx',
'upload-proto-error-text' => 'L’impôrt requerét des URLs comencient per <code>http://</code> ou ben <code>ftp://</code>.',
'upload-file-error'       => 'Èrror de dedens',
'upload-file-error-text'  => 'Una èrror de dedens est arrevâ en volent crèar un fichiér temporèro sur lo sèrvior. Volyéd vos veriér vers un administrator sistèmo.',
'upload-misc-error'       => 'Èrror d’impôrt encognua',
'upload-misc-error-text'  => 'Una èrror encognua est arrevâ pendent l’impôrt. Volyéd controlar que l’URL est valida et accèssibla, et pués tornâd èprovar. Se lo problèmo continue, veriéd-vos vers un administrator sistèmo.',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => 'Pôt pas avengiér l’URL',
'upload-curl-error6-text'  => 'L’URL fornia pôt pas étre avengiê. Volyéd controlar que l’URL est corrècta et que lo seto est en legne.',
'upload-curl-error28'      => 'Dèpassement du dèlê pendent l’impôrt',
'upload-curl-error28-text' => 'Lo seto at betâ trop grant-temps a rèpondre. Controlâd que lo seto est en legne, atendéd un pou et pués tornâd èprovar. Vos pouede asse-ben tornar èprovar a una hora de muendra afluence.',

'license'            => 'Licence&nbsp;:',
'nolicense'          => 'Niona licence sèlèccionâ',
'license-nopreview'  => '(Prèvisualisacion empossibla)',
'upload_source_url'  => ' (una URL valida et accèssibla publicament)',
'upload_source_file' => ' (un fichiér sur voutron ordenator)',

# Special:ImageList
'imagelist_search_for'  => 'Rechèrche por l’émâge apelâ :',
'imgfile'               => 'fichiér',
'imagelist'             => 'Lista de les émâges',
'imagelist_date'        => 'Dâta',
'imagelist_name'        => 'Nom',
'imagelist_user'        => 'Utilisator',
'imagelist_size'        => 'Octèts',
'imagelist_description' => 'Dèscripcion',

# Image description page
'filehist'                  => 'Historico du fichiér',
'filehist-help'             => 'Clicar sur una dâta et una hora por vêre lo fichiér tâl qu’il ére a cél moment.',
'filehist-deleteall'        => 'tot suprimar',
'filehist-deleteone'        => 'suprimar cen',
'filehist-revert'           => 'rèvocar',
'filehist-current'          => 'ora',
'filehist-datetime'         => 'Dâta et hora',
'filehist-user'             => 'Utilisator',
'filehist-dimensions'       => 'Dimensions',
'filehist-filesize'         => 'Talye du fichiér',
'filehist-comment'          => 'Comentèro',
'imagelinks'                => 'Pâges contegnent l’émâge',
'linkstoimage'              => 'Les pâges ce-desot contegnont ceta émâge :',
'nolinkstoimage'            => 'Niona pâge contint ceta émâge.',
'sharedupload'              => 'Ceti fichiér est partagiê et pôt étre utilisâ per d’ôtros projèts.',
'shareduploadwiki'          => 'Reportâd-vos a la [$1 pâge de dèscripcion] por més d’enformacions.',
'shareduploadwiki-desc'     => 'La dèscripcion de son $1 est lé afichiê ce-desot.',
'shareduploadwiki-linktext' => 'Pâge de dèscripcion du fichiér',
'noimage'                   => 'Nion fichiér èyent cél nom ègziste, vos pouede $1.',
'noimage-linktext'          => 'nen importar yon',
'uploadnewversion-linktext' => 'Copiyér una novèla vèrsion de ceti fichiér',

# File reversion
'filerevert'                => 'Rèvocar $1',
'filerevert-legend'         => 'Rèvocar lo fichiér',
'filerevert-intro'          => "Vos voléd rèvocar '''[[Media:$1|$1]]''' tant qu’a [$4 la vèrsion du $2 a $3].",
'filerevert-comment'        => 'Comentèro :',
'filerevert-defaultcomment' => 'Rèvocâ tant qu’a la vèrsion du $1 a $2',
'filerevert-submit'         => 'Rèvocar',
'filerevert-success'        => "'''[[Media:$1|$1]]''' at étâ rèvocâ tant qu’a [$4 la vèrsion du $2 a $3].",
'filerevert-badversion'     => 'Y at pas de vèrsion ples vielye du fichiér avouéc la dâta balyê.',

# File deletion
'filedelete'                  => 'Suprime $1',
'filedelete-legend'           => 'Suprimar lo fichiér',
'filedelete-intro'            => "Vos éte aprés suprimar '''[[Media:$1|$1]]'''.",
'filedelete-intro-old'        => "Vos éte aprés èfaciér la vèrsion de '''[[Media:$1|$1]]''' du [$4 $2 a $3].",
'filedelete-comment'          => 'Comentèro :',
'filedelete-submit'           => 'Suprimar',
'filedelete-success'          => "'''$1''' at étâ suprimâ.",
'filedelete-success-old'      => '<span class="plainlinks">La vèrsion de \'\'\'[[Media:$1|$1]]\'\'\' du $2 a $3 at étâ suprimâ.</span>',
'filedelete-nofile'           => "'''$1''' ègziste pas dessus {{SITENAME}}.",
'filedelete-nofile-old'       => "Ègziste gins de vèrsion arch·ivâ de '''$1''' avouéc los atributs endicâs.",
'filedelete-iscurrent'        => 'Vos éte aprés tâchiér de suprimar la vèrsion la ples novèla de ceti fichiér. Vos dête, prècèdament, rètablir una vielye vèrsion de ceti.',
'filedelete-otherreason'      => 'Rêson difèrenta ou suplèmentèra :',
'filedelete-reason-otherlist' => 'Rêson difèrenta',
'filedelete-reason-dropdown'  => '*Rêsons de suprèssion les ples corentes
** Violacion des drêts d’ôtor
** Fichiér en doblo',
'filedelete-edit-reasonlist'  => 'Modifie les rêsons de la suprèssion',

# MIME search
'mimesearch'         => 'Rechèrche per tipo MIME',
'mimesearch-summary' => 'Ceta pâge spèciâla pèrmèt de chèrchiér des fichiérs d’aprés lor tipo MIME. Entrâ : tipo/sot-tipo, per ègzemplo <tt>image/jpeg</tt>.',
'mimetype'           => 'Tipo MIME :',
'download'           => 'Tèlèchargement',

# Unwatched pages
'unwatchedpages' => 'Pâges pas siuvues',

# List redirects
'listredirects' => 'Lista de les redirèccions',

# Unused templates
'unusedtemplates'     => 'Modèlos inutilisâs',
'unusedtemplatestext' => 'Ceta pâge liste totes les pâges de l’èspâço de nom « Modèlo » que sont pas encllues dens niona ôtra pâge. Oubliâd pas de controlar s’y at pas d’ôtros lims vers los modèlos devant que los suprimar.',
'unusedtemplateswlh'  => 'ôtros lims',

# Random page
'randompage'         => 'Una pâge a l’hasârd',
'randompage-nopages' => 'Y at gins de pâge dens ceti èspâço de nom.',

# Random redirect
'randomredirect'         => 'Una pâge de redirèccion a l’hasârd',
'randomredirect-nopages' => 'Y at gins de pâge de redirèccion dens ceti èspâço de nom.',

# Statistics
'statistics'             => 'Statistiques',
'sitestats'              => 'Statistiques de {{SITENAME}}',
'userstats'              => 'Statistiques utilisator',
'sitestatstext'          => "Orendrêt, la bâsa de balyês contint {{PLURAL:$1|'''1''' pâge|'''$1''' pâges}}.

Ceti chifro encllut les pâges de discussion, les pâges sur {{SITENAME}}, les pâges côrtes (« començons »), les pâges de redirèccion, et pués d’ôtres pâges que sont pas considèrâs coment des articllos.
S’on èxcllut celes pâges, réste {{PLURAL:$2|'''1''' pâge qu’est probâblament un veretâblo articllo|'''$2''' pâges que sont probâblament de veretâblos articllos}}.<p>

{{PLURAL:$8|'''1''' fichiér at étâ tèlèchargiê|'''$8''' fichiérs ont étâ tèlèchargiês}}.

{{PLURAL:$3|'''1''' pâge at étâ consultâ|'''$3''' pâges ont étâ consultâs}} et {{PLURAL:$4|'''1''' pâge modifiâ|'''$4''' pâges modifiâs}} dês la crèacion de {{SITENAME}}.

Cen reprèsente una moyena de {{PLURAL:$5|'''1''' modificacion|'''$5''' modificacions}} per pâge et de {{PLURAL:$6|'''1''' consulta|'''$6''' consultes}} por una modificacion.</p>

<p>Y at ora {{PLURAL:$7|'''1''' ovrâjo|'''$7''' ovrâjos}} dens la [http://www.mediawiki.org/wiki/Manual:Job_queue fela d’atenta des travâlys].</p>",
'userstatstext'          => "Y at {{PLURAL:$1|'''1''' [[Special:ListUsers|utilisator enregistrâ]]|'''$1''' [[Special:ListUsers|utilisators enregistrâs]]}}. Permié cetos, '''$2''' (ou ben '''$4%''') {{PLURAL:$2|est|sont}} $5.",
'statistics-mostpopular' => 'Pâges les ples consultâs',

'disambiguations'      => 'Pâges d’homonimia',
'disambiguationspage'  => 'Template:Homonimia',
'disambiguations-text' => 'Les pâges siuventes liyont vers una <i>pâge d’homonimia</i>. Devriant pletout liyér vers una pâge que vat avouéc.<br /> Una pâge est trètâ coment una pâge d’homonimia s’el est liyê dês $1.<br /> Los lims dês d’ôtros èspâços de nom <i>sont pas</i> listâs ique.',

'doubleredirects'     => 'Redirèccions dobles',
'doubleredirectstext' => 'Châque câsa contint des lims vers la premiére et la seconda redirèccion, et pués la premiére legne de tèxte de la seconda pâge, cen que fornét habituèlament la « veré » pâge ciba, vers laquinta la premiére redirèccion devrêt redirigiér.',

'brokenredirects'        => 'Redirèccions câsses',
'brokenredirectstext'    => 'Cetes redirèccions mènont vers des pâges qu’ègzistont pas :',
'brokenredirects-edit'   => '(modifiar)',
'brokenredirects-delete' => '(suprimar)',

'withoutinterwiki'         => 'Pâges sen lims entèrlengoues',
'withoutinterwiki-summary' => 'Les pâges siuventes ont pas de lims vers d’ôtres lengoues :',
'withoutinterwiki-submit'  => 'Afichiér',

'fewestrevisions' => 'Articllos los muens modifiâs',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|octèt|octèts}}',
'ncategories'             => '$1 {{PLURAL:$1|catègorie|catègories}}',
'nlinks'                  => '$1 {{PLURAL:$1|lim|lims}}',
'nmembers'                => '$1 {{PLURAL:$1|pâge|pâges}} per dedens',
'nrevisions'              => '$1 {{PLURAL:$1|vèrsion|vèrsions}}',
'nviews'                  => '$1 {{PLURAL:$1|consulta|consultes}}',
'specialpage-empty'       => 'Ceta pâge est voueda.',
'lonelypages'             => 'Pâges orfenes',
'lonelypagestext'         => 'Les pâges siuventes sont pas liyês dês d’ôtres pâges de {{SITENAME}}.',
'uncategorizedpages'      => 'Pâges sen catègorie',
'uncategorizedcategories' => 'Catègories sen catègorie',
'uncategorizedimages'     => 'Émâges sen catègorie',
'uncategorizedtemplates'  => 'Modèlos sen catègorie',
'unusedcategories'        => 'Catègories inutilisâs',
'unusedimages'            => 'Émâges orfenes',
'popularpages'            => 'Pâges les ples consultâs',
'wantedcategories'        => 'Catègories les ples demandâs',
'wantedpages'             => 'Pâges les ples demandâs',
'mostlinked'              => 'Pâges les ples liyês',
'mostlinkedcategories'    => 'Catègories les ples utilisâs',
'mostlinkedtemplates'     => 'Modèlos los ples utilisâs',
'mostcategories'          => 'Articllos utilisent lo més de catègories',
'mostimages'              => 'Émâges les ples utilisâs',
'mostrevisions'           => 'Articllos los ples modifiâs',
'prefixindex'             => 'Totes les pâges per premiéres lètres',
'shortpages'              => 'Pâges côrtes',
'longpages'               => 'Pâges longes',
'deadendpages'            => 'Pâges en cul-de-sac',
'deadendpagestext'        => 'Les pâges siuventes ont gins de lim vers d’ôtres pâges de {{SITENAME}}.',
'protectedpages'          => 'Pâges protègiês',
'protectedpagestext'      => 'Les pâges siuventes sont protègiês contre les modificacions et/ou lo renomâjo :',
'protectedpagesempty'     => 'Niona pâge est protègiê orendrêt.',
'protectedtitles'         => 'Titros protègiês',
'protectedtitlestext'     => 'Los titros siuvents sont protègiês a la crèacion',
'protectedtitlesempty'    => 'Nion titro est protègiê orendrêt avouéc celos paramètres.',
'listusers'               => 'Lista des participents',
'newpages'                => 'Novèles pâges',
'newpages-username'       => 'Utilisator :',
'ancientpages'            => 'Articllos los muens dèrriérement modifiâs',
'move'                    => 'Renomar',
'movethispage'            => 'Renomar la pâge',
'unusedimagestext'        => 'Oubliâd pas que d’ôtros setos pôvont contegnir un lim drêt vers cela émâge, et que ceta pôt étre placiê dens ceta lista pendent qu’el est en rèalitât utilisâ.',
'unusedcategoriestext'    => 'Les catègories siuventes ègzistont mas nion articllo ou ben niona catègorie les utilise.',
'notargettitle'           => 'Pas de ciba',
'notargettext'            => 'Endicâd una pâge ciba ou ben un utilisator ciba.',
'pager-newer-n'           => '{{PLURAL:$1|1 ples novél|$1 ples novéls}}',
'pager-older-n'           => '{{PLURAL:$1|1 ples viely|$1 ples vielys}}',

# Book sources
'booksources'               => 'Ôvres de refèrence',
'booksources-search-legend' => 'Rechèrchiér permié des ôvres de refèrence',
'booksources-isbn'          => 'ISBN :',
'booksources-go'            => 'Validar',
'booksources-text'          => 'Vê-que una lista de lims vers d’ôtros setos que vendont des lévros nôfs et d’ocasion et sur losquints vos troveréd pôt-étre des enformacions sur les ôvres que vos chèrchiéd. {{SITENAME}} étent pas liyê a gins de celes sociètâts, el at pas du tot l’entencion de nen fâre la recllâma.',

# Special:Log
'specialloguserlabel'  => 'Utilisator :',
'speciallogtitlelabel' => 'Titro :',
'log'                  => 'Jornals',
'all-logs-page'        => 'Tôs los jornals',
'log-search-legend'    => 'Chèrchiér dens los jornals',
'log-search-submit'    => 'D’acôrd',
'alllogstext'          => 'Afichâjo combinâ des jornals de copia, de suprèssion, de protèccion, de blocâjo et d’administrator. Vos pouede rètrendre la vua en sèlèccionent un tipo de jornal, un nom d’utilisator ou la pâge regardâ.',
'logempty'             => 'Y at ren dens l’historico por ceta pâge.',
'log-title-wildcard'   => 'Chèrchiér los titros comencient per lo tèxte siuvent',

# Special:AllPages
'allpages'          => 'Totes les pâges',
'alphaindexline'    => '$1 a $2',
'nextpage'          => 'Pâge siuventa ($1)',
'prevpage'          => 'Pâge prècèdenta ($1)',
'allpagesfrom'      => 'Afichiér les pâges dês :',
'allarticles'       => 'Tôs los articllos',
'allinnamespace'    => 'Totes les pâges (èspâço de nom $1)',
'allnotinnamespace' => 'Totes les pâges (étent pas dens l’èspâço de nom $1)',
'allpagesprev'      => 'Prècèdent',
'allpagesnext'      => 'Siuvent',
'allpagessubmit'    => 'Validar',
'allpagesprefix'    => 'Afichiér les pâges comencient per lo prèfixe :',
'allpagesbadtitle'  => 'Lo titro rensègnê por la pâge est fôx ou at un prèfixe resèrvâ. Contint sûrement yon ou plusiors caractèros spèciâls que pôvont pas étre utilisâs dens los titros.',
'allpages-bad-ns'   => '{{SITENAME}} at pas d’èspâço de nom « $1 ».',

# Special:Categories
'categories'                    => 'Catègories',
'categoriespagetext'            => 'Les catègories siuventes contegnont des pâges ou des fichiérs multimèdia.',
'special-categories-sort-count' => 'tri per compto',
'special-categories-sort-abc'   => 'tri alfabètico',

# Special:ListUsers
'listusersfrom'      => 'Afichiér los utilisators dês :',
'listusers-submit'   => 'Montrar',
'listusers-noresult' => 'Nion utilisator trovâ. Controlâd asse-ben les variantes en grantes lètres / petiôtes lètres.',

# E-mail user
'mailnologin'     => 'Pas d’adrèce',
'mailnologintext' => 'Vos dête étre [[Special:UserLogin|conèctâ]]
et avêr endicâ una adrèce èlèctronica valida dens voutres [[Special:Preferences|prèferences]]
por avêr la pèrmission d’emmandar un mèssâjo a un ôtro utilisator.',
'emailuser'       => 'Emmandar un mèssâjo a ceti utilisator',
'emailpage'       => 'Emmandar un mèl a l’utilisator',
'emailpagetext'   => 'Se ceti utilisator at endicâ una adrèce èlèctronica valida dens ses prèferences, lo formulèro ce-desot lui emmanderat un mèssâjo.
L’adrèce èlèctronica que vos éd endicâ dens voutres prèferences aparètrat dens lo champ « Èxpèdior » de voutron mèssâjo por que lo dèstinatèro pouesse vos rèpondre.',
'usermailererror' => 'Èrror dens lo sujèt du mèl :',
'defemailsubject' => 'Mèl emmandâ dês {{SITENAME}}',
'noemailtitle'    => 'Pas d’adrèce èlèctronica',
'noemailtext'     => 'Vos pouede pas apelar ceti utilisator per mèl :
* ou ben perce qu’il at pas spècefiâ d’adrèce èlèctronica valida (et ôtentifiâ),
* ou ben perce qu’il at chouèsi/cièrdu, dens ses prèferences utilisator, de pas recêvre de mèl des ôtros utilisators.',
'emailfrom'       => 'Èxpèdior&nbsp;',
'emailto'         => 'Dèstinatèro&nbsp;',
'emailsubject'    => 'Sujèt&nbsp;',
'emailmessage'    => 'Mèssâjo&nbsp;',
'emailsend'       => 'Emmandar',
'emailccme'       => 'M’emmandar per mèl una copia de mon mèssâjo.',
'emailccsubject'  => 'Copia de voutron mèssâjo a $1 : $2',
'emailsent'       => 'Mèssâjo emmandâ',
'emailsenttext'   => 'Voutron mèssâjo at étâ emmandâ.',

# Watchlist
'watchlist'            => 'Lista de siuvu',
'mywatchlist'          => 'Lista de siuvu',
'watchlistfor'         => "(por l’utilisator '''$1''')",
'nowatchlist'          => 'Voutra lista de siuvu contint gins d’articllo.',
'watchlistanontext'    => 'Por povêr afichiér ou èditar los èlèments de voutra lista de siuvu, vos dête vos $1.',
'watchnologin'         => 'Pas conèctâ',
'watchnologintext'     => 'Vos dête étre [[Special:UserLogin|conèctâ]] por modifiar voutra lista de siuvu.',
'addedwatch'           => 'Apondua a la lista de siuvu',
'addedwatchtext'       => "La pâge « [[:$1]] » at étâ apondua a voutra [[Special:Watchlist|lista de siuvu]].

Les modificacions que vegnont de cela pâge et de la pâge de discussion associyê y seront listâs, et la pâge aparètrat '''en grâs''' dens la [[Special:RecentChanges|lista des dèrriérs changements]] por étre repèrâ ples facilament.

Por suprimar cela pâge de voutra lista de siuvu, clicâd dessus « pas més siuvre » dens lo câdro de navigacion.",
'removedwatch'         => 'Enlevâ de la lista de siuvu',
'removedwatchtext'     => 'La pâge « [[:$1]] » at étâ enlevâ de voutra lista de siuvu.',
'watch'                => 'Siuvre',
'watchthispage'        => 'Siuvre ceta pâge',
'unwatch'              => 'Pas més siuvre',
'unwatchthispage'      => 'Pas més siuvre',
'notanarticle'         => 'Pas un articllo',
'notvisiblerev'        => 'Vèrsion suprimâ',
'watchnochange'        => 'Niona de les pâges que vos siude at étâ modifiâ pendent la pèrioda afichiê.',
'watchlist-details'    => 'Vos siude <b>$1</b> {{PLURAL:$1|pâge|pâges}}, sen comptar les pâges de discussion.',
'wlheader-enotif'      => '* La notificacion per mèl est activâ.',
'wlheader-showupdated' => '* Les pâges qu’ont étâ modifiâs dês voutra dèrriére visita sont montrâs en <b>grâs</b>.',
'watchmethod-recent'   => 'contrôlo de les novèles modificacions de les pâges siuvues',
'watchmethod-list'     => 'contrôlo de les pâges siuvues por des novèles modificacions',
'watchlistcontains'    => "Voutra lista de siuvu contint '''$1''' {{PLURAL:$1|pâge|pâges}}.",
'iteminvalidname'      => 'Problèmo avouéc l’articllo « $1 » : lo nom est envalido...',
'wlnote'               => 'Ce-desot sè {{PLURAL:$1|trove la dèrriére modificacion|trovont les $1 dèrriéres modificacions}} dês {{PLURAL:$2|l’hora passâ|les <b>$2</b> hores passâs}}.',
'wlshowlast'           => 'Montrar les $1 hores passâs, los $2 jorns passâs, ou ben $3 ;',
'watchlist-show-bots'  => 'afichiér les contribucions de bots',
'watchlist-hide-bots'  => 'mâscar les contribucions de bots',
'watchlist-show-own'   => 'afichiér mes contribucions',
'watchlist-hide-own'   => 'mâscar mes contribucions',
'watchlist-show-minor' => 'afichiér les modificacions minores.',
'watchlist-hide-minor' => 'mâscar les modificacions minores.',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Siuvu...',
'unwatching' => 'Fin du siuvu...',

'enotif_mailer'                => 'Sistèmo d’èxpèdicion de notificacion de {{SITENAME}}',
'enotif_reset'                 => 'Marcar totes les pâges coment visitâs',
'enotif_newpagetext'           => 'Cen est una novèla pâge.',
'enotif_impersonal_salutation' => 'Utilisator de {{SITENAME}}',
'changed'                      => 'modifiâ',
'created'                      => 'crèâ',
'enotif_subject'               => 'La pâge « $PAGETITLE » de {{SITENAME}} at étâ $CHANGEDORCREATED per $PAGEEDITOR',
'enotif_lastvisited'           => 'Consultâd $1 por tôs los changements dês voutra dèrriére visita.',
'enotif_lastdiff'              => 'Consultâd $1 por vêre cela modificacion.',
'enotif_anon_editor'           => 'utilisator pas enregistrâ $1',
'enotif_body'                  => 'Chier(a) $WATCHINGUSERNAME,

la pâge « $PAGETITLE » de {{SITENAME}} at étâ $CHANGEDORCREATED lo $PAGEEDITDATE per $PAGEEDITOR, vêde $PAGETITLE_URL por la vèrsion d’ora.

$NEWPAGE

Rèsumâ de l’èditor : $PAGESUMMARY $PAGEMINOREDIT

Veriéd-vos vers l’èditor :
mèl : $PAGEEDITOR_EMAIL
vouiqui : $PAGEEDITOR_WIKI

Y arat pas de novèles notificacions en câs d’ôtres modificacions a muens que vos visitâd cela pâge. Vos pouede asse-ben remetre a zérô lo notifior por totes les pâges de voutra lista de siuvu.

             Voutron sistèmo de notificacion de {{SITENAME}}

--
Por modifiar los paramètres de voutra lista de siuvu, visitâd
{{fullurl:Special:Watchlist/edit}}

Retôrn et assistance :
{{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => 'Suprimar una pâge',
'confirm'                     => 'Confirmar',
'excontent'                   => 'contegnent « $1 »',
'excontentauthor'             => 'lo contegnu ére : « $1 » et lo solèt contributor nen ére « [[Special:Contributions/$2|$2]] ».',
'exbeforeblank'               => 'contegnéve devant blanchiment : « $1 »',
'exblank'                     => 'pâge voueda',
'delete-confirm'              => 'Suprimar « $1 »',
'delete-legend'               => 'Suprèssion',
'historywarning'              => 'Atencion : la pâge que vos éte prèst a suprimar at un historico :',
'confirmdeletetext'           => 'Vos éte prèst a suprimar por de bon de la bâsa de balyês una pâge ou una émâge, et pués totes ses vèrsions prècèdentes. Volyéd confirmar qu’o est franc cen que vos voléd fâre, que vos en compregnéd les consèquences et que vos féte cen en acôrd avouéc les [[{{MediaWiki:Policy-url}}|règlles de dedens]].',
'actioncomplete'              => 'Accion fêta',
'deletedtext'                 => '« <nowiki>$1</nowiki> » at étâ suprimâ.
Vêde l’$2 por una lista de les novèles suprèssions.',
'deletedarticle'              => 'at èfaciê « [[$1]] »',
'dellogpage'                  => 'Historico de les suprèssions',
'dellogpagetext'              => 'Vê-que la lista de les novèles suprèssions.
L’hora endicâ est cela du sèrvior.',
'deletionlog'                 => 'historico des èfacements',
'reverted'                    => 'Rètablissement de la vèrsion prècèdenta',
'deletecomment'               => 'Rêson de la suprèssion :',
'deleteotherreason'           => 'Rêson difèrenta ou suplèmentèra :',
'deletereasonotherlist'       => 'Ôtra rêson',
'deletereason-dropdown'       => '*Rêsons de suprèssion les ples corentes
** Demanda de l’ôtor
** Violacion des drêts d’ôtor
** Vandalismo',
'delete-edit-reasonlist'      => 'Modifie les rêsons de la suprèssion',
'delete-toobig'               => 'Ceta pâge dispôse d’un historico important, dèpassent $1 vèrsions. La suprèssion de tâles pâges at étâ limitâ por èvitar des pèrturbacions emprèvues de {{SITENAME}}.',
'delete-warning-toobig'       => 'Ceta pâge dispôse d’un historico important, dèpassent $1 vèrsions. La suprimar pôt troblar lo fonccionement de la bâsa de balyês de {{SITENAME}}. A fâre avouéc prudence.',
'rollback'                    => 'rèvocar modificacions',
'rollback_short'              => 'Rèvocar',
'rollbacklink'                => 'rèvocar',
'rollbackfailed'              => 'La rèvocacion at pas reussia',
'cantrollback'                => 'Empossiblo de rèvocar : l’ôtor est la solèta pèrsona a avêr fêt des modificacions sur ceta pâge.',
'alreadyrolled'               => 'Empossiblo de rèvocar la dèrriére modificacion de l’articllo « [[$1]] » fêta per [[User:$2|$2]] ([[User talk:$2|Discussion]]) ; quârqu’un d’ôtro at ja modifiâ ou rèvocâ l’articllo.

La dèrriére modificacion at étâ fêta per [[User:$3|$3]] ([[User talk:$3|Discussion]]).',
'editcomment'                 => 'Lo rèsumâ de la modificacion ére : <i>« $1 »</i>.', # only shown if there is an edit comment
'revertpage'                  => 'Rèvocacion de les modificacions de [[Special:Contributions/$2|$2]] ([[User talk:$2|Discussion]]) (retôrn a la vèrsion prècèdenta de [[User:$1|$1]])', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => 'Rèvocacion de les modificacions de $1 ; retôrn a la vèrsion de $2.',
'sessionfailure'              => 'Voutra sèssion de conèccion semble avêr des problèmos ;
cela accion at étâ anulâ en prèvencion d’un piratâjo de sèssion.
Clicâd dessus « Prècèdent » et rechargiéd la pâge de yô que vos vegnéd, et pués tornâd èprovar.',
'protectlogpage'              => 'Historico de les protèccions',
'protectlogtext'              => 'Vêde les [[Special:ProtectedPages|dirèctives]] por més d’enformacion.',
'protectedarticle'            => 'at protègiê « [[$1]] »',
'modifiedarticleprotection'   => 'at modifiâ lo nivô de protèccion de « [[$1]] »',
'unprotectedarticle'          => 'at dèprotègiê « [[$1]] »',
'protect-title'               => 'Protègiér « $1 »',
'protect-legend'              => 'Confirmar la protèccion',
'protectcomment'              => 'Rêson de la protèccion :',
'protectexpiry'               => 'Èxpiracion (èxpire pas per dèfôt) :',
'protect_expiry_invalid'      => 'Lo temps d’èxpiracion est envalido.',
'protect_expiry_old'          => 'Lo temps d’èxpiracion est ja passâ.',
'protect-unchain'             => 'Dèblocar les pèrmissions de renomâjo',
'protect-text'                => 'Vos pouede consultar et modifiar lo nivô de protèccion de la pâge <strong><nowiki>$1</nowiki></strong>.',
'protect-locked-blocked'      => 'Vos pouede pas modifiar lo nivô de protèccion tant que vos éte blocâ.
Vê-que los règllâjos d’ora de la pâge <strong>$1</strong> :',
'protect-locked-dblock'       => 'Lo nivô de protèccion pôt pas étre modifiâ perce que la bâsa de balyês est blocâ.
Vê-que los règllâjos d’ora de la pâge <strong>$1</strong> :',
'protect-locked-access'       => 'Vos avéd pas los drêts nècèssèros por modifiar la protèccion de la pâge.
Vê-que los règllâjos d’ora de la pâge <strong>$1</strong> :',
'protect-cascadeon'           => 'Ora, ceta pâge est protègiê perce qu’el est encllua dens {{PLURAL:$1|la pâge siuventa|les pâges siuventes}}, èyent étâ protègiê avouéc lo chouèx « Protèccion en cascâda » activâ. Vos pouede changiér lo nivô de protèccion de ceta pâge sen que cen afècte la protèccion en cascâda.',
'protect-default'             => 'Pas de protèccion',
'protect-fallback'            => 'At fôta de la pèrmission « $1 »',
'protect-level-autoconfirmed' => 'Mié-protèccion',
'protect-level-sysop'         => 'Administrators solament',
'protect-summary-cascade'     => 'protèccion en cascâda',
'protect-expiring'            => 'èxpire lo $1 (UTC)',
'protect-cascade'             => 'Protèccion en cascâda - Protège totes les pâges encllues dens ceta.',
'protect-cantedit'            => 'Vos pouede pas modifiar los nivôs de protèccion de cela pâge perce que vos avéd pas la pèrmission de l’èditar.',
'restriction-type'            => 'Pèrmission :',
'restriction-level'           => 'Nivô de rèstriccion :',
'minimum-size'                => 'Talye la ples petiôta',
'maximum-size'                => 'Talye la ples granta:',
'pagesize'                    => '(octèts)',

# Restrictions (nouns)
'restriction-edit'   => 'Modificacion',
'restriction-move'   => 'Renomâjo',
'restriction-create' => 'Crèar',

# Restriction levels
'restriction-level-sysop'         => 'Protèccion complèta',
'restriction-level-autoconfirmed' => 'Mié-protèccion',
'restriction-level-all'           => 'Tôs',

# Undelete
'undelete'                     => 'Vêre les pâges suprimâs',
'undeletepage'                 => 'Vêre et rèstorar la pâge suprimâ',
'undeletepagetitle'            => "'''La lista siuventa contint des vèrsions suprimâs de [[:$1]].'''",
'viewdeletedpage'              => 'Historico de la pâge suprimâ',
'undeletepagetext'             => 'Cetes pâges ont étâ suprimâs et sè trovont dens les arch·ives, sont adés dens la bâsa de balyês et pôvont étre rèstorâs.
Les arch·ives pôvont étre èfaciês règuliérement.',
'undeleteextrahelp'            => "Por rèstorar totes les vèrsions de cela pâge, lèssiéd vouedes totes les câses a marcar, et pués clicâd dessus '''''Rèstorar !'''''.<br /> Por fâre una rèstoracion sèlèctiva, marcâd les câses corrèspondent a les vèrsions que sont a rèstorar, et pués clicâd dessus '''''Rèstorar !'''''.<br /> En cliquent sur lo boton '''''Tornar inicialisar''''', la bouèta de rèsumâ et les câses marcâs seront remêses a zérô.",
'undeleterevisions'            => '$1 {{PLURAL:$1|vèrsion arch·ivâ|vèrsions arch·ivâs}}',
'undeletehistory'              => 'Se vos rèstorâd la pâge, totes les vèrsions seront replaciês dens l’historico.

S’una novèla pâge avouéc lo mémo nom at étâ crèâ dês la suprèssion,
les vèrsions rèstorâs aparètront dens l’historico prècèdent et la vèrsion corenta serat pas ôtomaticament remplaciê.',
'undeleterevdel'               => 'La rèstoracion serat pas fêta se, a la fin, la vèrsion la ples novèla de la pâge serat a mêtiêt suprimâ. Dens cél câs, vos dête dèsèlèccionar les vèrsions les ples novèles (d’amont). Les vèrsions des fichiérs a lesquintes vos avéd pas accès seront pas rèstorâs.',
'undeletehistorynoadmin'       => 'Ceti articllo at étâ suprimâ. La rêson de la suprèssion est endicâ dens lo rèsumâ ce-desot, avouéc los dètalys des utilisators que l’ont modifiâ devant sa suprèssion. Lo contegnu de cetes vèrsions est accèssiblo ren qu’ux administrators.',
'undelete-revision'            => 'Vèrsion suprimâ de $1, (vèrsion du $2) per $3 :',
'undeleterevision-missing'     => 'Vèrsion envalida ou manquenta. Vos avéd pôt-étre un môvés lim, ou la vèrsion at étâ rèstorâ ou suprimâ de les arch·ives.',
'undelete-nodiff'              => 'Niona vèrsion prècèdenta trovâ.',
'undeletebtn'                  => 'Rèstorar !',
'undeletelink'                 => 'rèstorar',
'undeletereset'                => 'Tornar inicialisar',
'undeletecomment'              => 'Rèsumâ :',
'undeletedarticle'             => 'at rèstorâ « [[$1]] »',
'undeletedrevisions'           => '$1 {{PLURAL:$1|vèrsion rèstorâ|vèrsions rèstorâs}}',
'undeletedrevisions-files'     => '$1 {{PLURAL:$1|vèrsion|vèrsions}} et $2 {{PLURAL:$2|fichiér|fichiérs}} rèstorâs',
'undeletedfiles'               => '$1 {{PLURAL:$1|fichiér rèstorâ|fichiérs rèstorâs}}',
'cannotundelete'               => 'La rèstoracion at pas reussia. Un ôtro utilisator at probâblament rèstorâ la pâge devant.',
'undeletedpage'                => "<big>'''La pâge $1 at étâ rèstorâ.'''</big>

Consultâd l’[[Special:Log/delete|historico des èfacements]] por vêre la lista des novéls èfacements et de les novèles rèstoracions de pâges.",
'undelete-header'              => 'Consultâd lo [[Special:Log/delete|jornal de les suprèssions]] por vêre les pâges dèrriérement suprimâs.',
'undelete-search-box'          => 'Chèrchiér una pâge suprimâ',
'undelete-search-prefix'       => 'Montrar les pâges comencient per :',
'undelete-search-submit'       => 'Chèrchiér',
'undelete-no-results'          => 'Niona pâge corrèspondent a la rechèrche at étâ trovâ dens les arch·ives.',
'undelete-filename-mismatch'   => 'Empossiblo de rèstorar lo fichiér avouéc la dâta $1 : fichiér entrovâblo',
'undelete-bad-store-key'       => 'Empossiblo de rèstorar lo fichiér avouéc la dâta $1 : lo fichiér ére absent devant la suprèssion.',
'undelete-cleanup-error'       => 'Èrror pendent la suprèssion de les arch·ives inutilisâs de « $1 ».',
'undelete-missing-filearchive' => 'Empossiblo de rèstorar lo fichiér avouéc l’ID $1 perce qu’il est pas dens la bâsa de balyês. Il at pôt-étre ja étâ rèstorâ.',
'undelete-error-short'         => 'Èrror pendent la rèstoracion du fichiér : $1',
'undelete-error-long'          => 'Des èrrors ont étâ rencontrâs pendent la rèstoracion du fichiér :

$1',

# Namespace form on various pages
'namespace'      => 'Èspâço de nom :',
'invert'         => 'Envèrsar la sèlèccion',
'blanknamespace' => '(Principâl)',

# Contributions
'contributions' => 'Contribucions de ceti utilisator',
'mycontris'     => 'Contribucions',
'contribsub2'   => 'Lista de les contribucions de $1 ($2). Les pâges qu’ont étâ èfaciês sont pas afichiês.',
'nocontribs'    => 'Niona modificacion corrèspondent a cetos critèros at étâ trovâ.',
'uctop'         => '(dèrriére)',
'month'         => 'Dês lo mês (et prècèdents) :',
'year'          => 'Dês l’an (et prècèdents) :',

'sp-contributions-newbies'     => 'Montrar ren que les contribucions des novéls utilisators',
'sp-contributions-newbies-sub' => 'Lista de les contribucions des novéls utilisators. Les pâges qu’ont étâ suprimâs sont pas afichiês.',
'sp-contributions-blocklog'    => 'Jornal des blocâjos',
'sp-contributions-search'      => 'Chèrchiér les contribucions',
'sp-contributions-username'    => 'Adrèce IP ou nom d’utilisator :',
'sp-contributions-submit'      => 'Chèrchiér',

# What links here
'whatlinkshere'       => 'Pâges liyês',
'whatlinkshere-title' => 'Pâges liyês a $1',
'whatlinkshere-page'  => 'Pâge :',
'linklistsub'         => '(Lista de lims)',
'linkshere'           => 'Les pâges ce-desot contegnont un lim vers <b>[[:$1]]</b> :',
'nolinkshere'         => 'Niona pâge contint de lim vers <b>[[:$1]]</b>.',
'nolinkshere-ns'      => "Niona pâge contint de lim vers '''[[:$1]]''' dens l’èspâço de nom chouèsi/cièrdu.",
'isredirect'          => 'pâge de redirèccion',
'istemplate'          => 'encllusion',
'whatlinkshere-prev'  => '{{PLURAL:$1|prècèdent|$1 prècèdents}}',
'whatlinkshere-next'  => '{{PLURAL:$1|siuvent|$1 siuvents}}',
'whatlinkshere-links' => '← lims',

# Block/unblock
'blockip'                     => 'Blocar una adrèce IP ou un utilisator',
'blockip-legend'              => 'Blocar en ècritura',
'blockiptext'                 => 'Utilisâd lo formulèro ce-desot por blocar l’accès en ècritura dês una adrèce IP balyê ou un nom d’utilisator.

Una tâla mesera dêt étre prêsa ren que por empachiér lo vandalismo et en acôrd avouéc les [[{{MediaWiki:Policy-url}}|règlles de dedens]].
Balyéd ce-desot una rêson cllâra (per ègzemplo en endiquent les pâges qu’ont étâ vandalisâs).',
'ipaddress'                   => 'Adrèce IP :',
'ipadressorusername'          => 'Adrèce IP ou nom d’utilisator :',
'ipbexpiry'                   => 'Durâ du blocâjo :',
'ipbreason'                   => 'Rêson :',
'ipbreasonotherlist'          => 'Ôtra rêson',
'ipbreason-dropdown'          => '*Rêsons de blocâjo les ples corentes
** Vandalismo
** Entrebetâ d’enformacions fôsses
** Suprèssion de contegnu sen èxplicacion
** Entrebetâ rèpètâ de lims de defôr publicitèros (spame)
** Entrebetâ de contegnu sen pouent de significacion
** Tentativa d’entimidacion ou de pèrsècucion
** Abus d’usâjo de comptos multiplos
** Nom d’utilisator pas accèptâblo, ofensent ou difament',
'ipbanononly'                 => 'Blocar ren que los utilisators pas enregistrâs',
'ipbcreateaccount'            => 'Empachiér la crèacion de compto',
'ipbemailban'                 => 'Empachiér l’utilisator d’emmandar des mèls',
'ipbenableautoblock'          => 'Blocar ôtomaticament les adrèces IP utilisâs per ceti utilisator',
'ipbsubmit'                   => 'Blocar ceti utilisator',
'ipbother'                    => 'Ôtra durâ :',
'ipboptions'                  => '2 hores:2 hours,1 jorn:1 day,3 jorns:3 days,1 semana:1 week,2 semanes:2 weeks,1 mês:1 month,3 mês:3 months,6 mês:6 months,1 an:1 year,sen fin:infinite', # display1:time1,display2:time2,...
'ipbotheroption'              => 'ôtra',
'ipbotherreason'              => 'Rêson difèrenta ou suplèmentèra :',
'ipbhidename'                 => 'Mâscar lo nom d’utilisator de l’historico des blocâjos, de la lista des blocâjos actifs et de la lista des utilisators',
'badipaddress'                => 'L’adrèce IP est fôssa.',
'blockipsuccesssub'           => 'Blocâjo reussi',
'blockipsuccesstext'          => '[[Special:Contributions/$1|$1]] at étâ blocâ.
<br />Vos pouede consultar la [[Special:IPBlockList|lista des comptos et de les adrèces IP blocâs]].',
'ipb-edit-dropdown'           => 'Modifiâd les rêsons de blocâjo per dèfôt',
'ipb-unblock-addr'            => 'Dèblocâd $1',
'ipb-unblock'                 => 'Dèblocâd un compto utilisator ou una adrèce IP',
'ipb-blocklist-addr'          => 'Vêde los blocâjos ègzistents por $1',
'ipb-blocklist'               => 'Vêde los blocâjos ègzistents',
'unblockip'                   => 'Dèblocar un utilisator ou una adrèce IP',
'unblockiptext'               => 'Utilisâd lo formulèro ce-desot por rètablir l’accès en ècritura
d’una adrèce IP prècèdament blocâ.',
'ipusubmit'                   => 'Dèblocar ceta adrèce',
'unblocked'                   => '[[User:$1|$1]] at étâ dèblocâ',
'unblocked-id'                => 'Lo blocâjo $1 at étâ enlevâ',
'ipblocklist'                 => 'Lista des utilisators blocâs',
'ipblocklist-legend'          => 'Chèrchiér un utilisator blocâ',
'ipblocklist-username'        => 'Nom d’utilisator ou adrèce IP :',
'ipblocklist-submit'          => 'Chèrchiér',
'blocklistline'               => '$1 ($4) : $2 at blocâ $3',
'infiniteblock'               => 'sen fin',
'expiringblock'               => 'èxpire lo $1',
'anononlyblock'               => 'utilisator pas enregistrâ solament',
'noautoblockblock'            => 'blocâjo ôtomatico dèsactivâ',
'createaccountblock'          => 'crèacion de compto blocâ',
'emailblock'                  => 'mèl blocâ',
'ipblocklist-empty'           => 'Orendrêt, la lista de les adrèces blocâs est voueda.',
'ipblocklist-no-results'      => 'L’adrèce IP ou l’utilisator at pas étâ blocâ.',
'blocklink'                   => 'Blocar',
'unblocklink'                 => 'dèblocar',
'contribslink'                => 'Contribucions',
'autoblocker'                 => 'Vos avéd étâ blocâ ôtomaticament perce que voutra adrèce IP at étâ dèrriérement utilisâ per « [[User:$1|$1]] ». La rêson fornia por lo blocâjo de $1 est : « $2 ».',
'blocklogpage'                => 'Historico des blocâjos',
'blocklogentry'               => 'at blocâ « [[$1]] » - durâ : $2 $3',
'blocklogtext'                => 'Cen est l’historico des blocâjos et dèblocâjos des utilisators. Les adrèces IP ôtomaticament blocâs sont pas listâs. Consultâd la [[Special:IPBlockList|lista des utilisators blocâs]] por vêre qui est en èfèt blocâ houé.',
'unblocklogentry'             => 'at dèblocâ « $1 »',
'block-log-flags-anononly'    => 'utilisators pas enregistrâs solament',
'block-log-flags-nocreate'    => 'crèacion de compto dèfendua',
'block-log-flags-noautoblock' => 'ôtoblocâjo de les adrèces IP dèsactivâ',
'block-log-flags-noemail'     => 'èxpèdicion de mèl dèfendua',
'range_block_disabled'        => 'Lo blocâjo de plages d’IP at étâ dèsactivâ.',
'ipb_expiry_invalid'          => 'Temps d’èxpiracion envalido.',
'ipb_already_blocked'         => '« $1 » est ja blocâ',
'ipb_cant_unblock'            => 'Èrror : lo blocâjo d’ID $1 ègziste pas. O est possiblo qu’un dèblocâjo èye ja étâ fêt.',
'ipb_blocked_as_range'        => 'Èrror : l’adrèce IP $1 at pas étâ blocâ tot drêt et pôt vêr pas étre dèblocâ. Portant el at étâ blocâ per la plage $2 laquinta pôt étre dèblocâ.',
'ip_range_invalid'            => 'Bloco IP fôx.',
'blockme'                     => 'Blocâd-mè',
'proxyblocker'                => 'Bloquior de proxy',
'proxyblocker-disabled'       => 'Cela fonccion est dèsactivâ.',
'proxyblockreason'            => 'Voutra adrèce IP at étâ blocâ perce qu’el est un proxy uvèrt. Marci de vos veriér vers voutron fornissor d’accès u Malyâjo ou voutron supôrt tècnico et de l’enformar de ceti problèmo de sècuritât.',
'proxyblocksuccess'           => 'Chavonâ.',
'sorbsreason'                 => 'Voutra adrèce IP est listâ a titro de proxy uvèrt dens lo DNSBL utilisâ per {{SITENAME}}.',
'sorbs_create_account_reason' => 'Voutra adrèce IP est listâ a titro de proxy uvèrt dens lo DNSBL utilisâ per {{SITENAME}}. Vos pouede pas crèar un compto utilisator.',

# Developer tools
'lockdb'              => 'Vèrrolyér la bâsa',
'unlockdb'            => 'Dèvèrrolyér la bâsa',
'lockdbtext'          => 'Lo vèrrolyâjo de la bâsa de balyês empachierat tôs los utilisators de modifiar des pâges, de sôvar lors prèferences, de modifiar lor lista de siuvu et de fâre totes les ôtres opèracions èyent fôta des modificacions dens la bâsa de balyês.

Volyéd confirmar qu’o est franc cen que vos voléd fâre et que vos dèbloqueréd la bâsa setout que voutra opèracion de mantegnence serat chavonâ.',
'unlockdbtext'        => 'Lo dèvèrrolyâjo de la bâsa de balyês tornerat pèrmetre a tôs los utilisators de modifiar des pâges, de betar a jorn lors prèferences et lor lista de siuvu, et pués de fâre les ôtres opèracions èyent fôta des modificacions dens la bâsa de balyês.

Volyéd confirmar qu’o est franc cen que vos voléd fâre.',
'lockconfirm'         => 'Ouè, confirmo que souhèto vèrrolyér la bâsa de balyês.',
'unlockconfirm'       => 'Ouè, confirmo que souhèto dèvèrrolyér la bâsa de balyês.',
'lockbtn'             => 'Vèrrolyér la bâsa',
'unlockbtn'           => 'Dèvèrrolyér la bâsa',
'locknoconfirm'       => 'Vos éd pas marcâ la câsa de confirmacion.',
'lockdbsuccesssub'    => 'Vèrrolyâjo de la bâsa reussi.',
'unlockdbsuccesssub'  => 'Bâsa dèvèrrolyê.',
'lockdbsuccesstext'   => 'La bâsa de balyês de {{SITENAME}} est vèrrolyê.
<br />Oubliâd pas de [[Special:UnlockDB|la dèvèrrolyér]] quand vos aréd chavonâ voutra opèracion de mantegnence.',
'unlockdbsuccesstext' => 'La bâsa de balyês de {{SITENAME}} est dèvèrrolyê.',
'lockfilenotwritable' => 'Lo fichiér de blocâjo de la bâsa de balyês est pas enscriptiblo. Por blocar ou dèblocar la bâsa de balyês, vos dête povêr ècrire sur lo sèrvior Vouèbe.',
'databasenotlocked'   => 'La bâsa de balyês est pas vèrrolyê.',

# Move page
'move-page'               => 'Renomar $1',
'move-page-legend'        => 'Renomar una pâge',
'movepagetext'            => "Utilisâd lo formulèro ce-desot por renomar una pâge, en dèplacient tot son historico vers lo novél nom.
Lo viely titro vindrat una pâge de redirèccion vers lo novél titro.
Los lims vers lo titro de la vielye pâge seront pas changiês ;
volyéd controlar que cél dèplacement at pas crèâ una redirèccion dobla ou câssa.
Vos dête vos assurar que los lims continuont de pouentar vers lor dèstinacion suposâ.

Una pâge serat '''pas''' dèplaciê s’y at ja una pâge avouéc lo novél titro, a muens que la pâge seye voueda, ou ben una redirèccion, et qu’el èye pas d’historico.
Cen vôt dére que vos pouede renomar una pâge vers sa posicion d’origina se vos éd fêt una èrror, et que vos pouede pas èfaciér una pâge ja ègzistenta per cela mètoda.

'''ATENCION !'''
Pôt s’ag·ir d’un changement radicâl et emprèvu por un articllo sovent consultâ ;
volyéd vos assurar que vos en compregnéd bien les consèquences devant que procèdar.",
'movepagetalktext'        => 'La pâge de discussion associyê, se presente, serat ôtomaticament renomâ avouéc, <b>a muens que :</b>
*Vos renomâd una pâge vers un ôtro èspâço,
*Una pâge de discussion ègziste ja avouéc lo novél nom, ou ben
*Vos éd dèsèlèccionâ lo boton ce-desot.

Dens cél câs, vos devréd renomar ou fusionar la pâge manuèlament se vos lo dèsirâd.',
'movearticle'             => 'Renomar l’articllo :',
'movenotallowed'          => 'Vos avéd pas la pèrmission de renomar des pâges.',
'newtitle'                => 'Novél titro :',
'move-watch'              => 'Siuvre ceta pâge',
'movepagebtn'             => 'Renomar l’articllo',
'pagemovedsub'            => 'Renomâjo reussi',
'articleexists'           => 'Ègziste ja un articllo portent cél titro, ou ben lo titro
que vos éd chouèsi/cièrdu est envalido.
Volyéd nen chouèsir/cièrdre un ôtro.',
'cantmove-titleprotected' => 'Vos avéd pas la possibilitât de dèplaciér una pâge vers cél emplacement perce que lo novél titro at étâ protègiê a la crèacion.',
'talkexists'              => "'''La pâge lyé-méma at étâ dèplaciê avouéc reusséta, mas la pâge de discussion at pas possu étre dèplaciê perce que nen ègzistâve ja yona desot lo novél nom. Volyéd les fusionar manuèlament.'''",
'movedto'                 => 'renomâ en',
'movetalk'                => 'Renomar asse-ben la pâge de discussion associyê',
'1movedto2'               => 'at renomâ [[$1]] en [[$2]]',
'1movedto2_redir'         => 'at redirigiê [[$1]] vers [[$2]]',
'movelogpage'             => 'Historico des renomâjos',
'movelogpagetext'         => 'Vê-que la lista de les dèrriéres pâges renomâs.',
'movereason'              => 'Rêson du renomâjo :',
'revertmove'              => 'anular',
'delete_and_move'         => 'Suprimar et renomar',
'delete_and_move_text'    => '== Suprèssion requisa ==

L’articllo de dèstinacion « [[:$1]] » ègziste ja. Voléd-vos lo suprimar por pèrmetre lo renomâjo ?',
'delete_and_move_confirm' => 'Ouè, j/y’accèpto de suprimar la pâge de dèstinacion por pèrmetre lo renomâjo.',
'delete_and_move_reason'  => 'Pâge suprimâ por pèrmetre un renomâjo',
'selfmove'                => 'Los titros d’origina et de dèstinacion sont los mémos : empossiblo de renomar una pâge sur lyé-méma.',
'immobile_namespace'      => 'Lo titro de dèstinacion est d’un tipo spèciâl ; o est empossiblo de renomar des pâges vers cél èspâço de nom.',

# Export
'export'            => 'Èxportar des pâges',
'exporttext'        => 'Vos pouede èxportar en XML lo tèxte et l’historico d’una pâge ou d’un ensemblo de pâges ; lo rèsultat pôt adonc étre importâ dens un ôtro vouiqui fonccionent avouéc la programeria MediaWiki.

Por èxportar des pâges, entrâd lors titros dens la bouèta de tèxte ce-desot, yon titro per legne, et pués sèlèccionâd, se vos dèsirâd ou pas, la vèrsion d’ora avouéc totes les vielyes vèrsions, avouéc la pâge d’historico, ou simplament la pâge d’ora avouéc des enformacions sur la dèrriére modificacion.

Dens cél dèrriér câs, vos pouede asse-ben utilisar un lim, coment [[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]] por la pâge "[[{{MediaWiki:Mainpage}}]]".',
'exportcuronly'     => 'Èxportar ren que la vèrsion corenta sen l’historico complèt',
'exportnohistory'   => "----
'''Nota :''' l’èxportacion complèta de l’historico de les pâges avouéc ceti formulèro at étâ dèsactivâ por des rêsons de pèrformences.",
'export-submit'     => 'Èxportar',
'export-addcattext' => 'Apondre les pâges de la catègorie :',
'export-addcat'     => 'Apondre',
'export-download'   => 'Sôvar a titro de fichiér',
'export-templates'  => 'Encllure los modèlos',

# Namespace 8 related
'allmessages'               => 'Lista des mèssâjos sistèmo',
'allmessagesname'           => 'Nom du champ',
'allmessagesdefault'        => 'Mèssâjo per dèfôt',
'allmessagescurrent'        => 'Mèssâjo d’ora',
'allmessagestext'           => 'Cen est la lista de tôs los mèssâjos sistèmo disponiblos dens l’èspâço MediaWiki.
Please visit [http://www.mediawiki.org/wiki/Localisation MediaWiki Localisation] and [http://translatewiki.net Betawiki] if you wish to contribute to the generic MediaWiki localisation.',
'allmessagesnotsupportedDB' => "'''{{ns:special}}:Allmessages''' est pas disponiblo perce que '''\$wgUseDatabaseMessages''' est dèsactivâ.",
'allmessagesfilter'         => 'Filtro d’èxprèssion racionèla :',
'allmessagesmodified'       => 'Afichiér ren que les modificacions',

# Thumbnails
'thumbnail-more'           => 'Agrantir',
'filemissing'              => 'Fichiér absent',
'thumbnail_error'          => 'Èrror pendent la crèacion de la figura : $1',
'djvu_page_error'          => 'Pâge DjVu en defôr de les limites',
'djvu_no_xml'              => 'Empossiblo d’obtegnir lo XML por lo fichiér DjVu',
'thumbnail_invalid_params' => 'Paramètres de la figura envalidos',
'thumbnail_dest_directory' => 'Empossiblo de crèar lo rèpèrtouèro de dèstinacion',

# Special:Import
'import'                     => 'Importar des pâges',
'importinterwiki'            => 'Impôrt entèrvouiqui',
'import-interwiki-text'      => 'Sèlèccionâd un vouiqui et un titro de pâge a importar.
Les dâtes de les vèrsions et los noms des èditors seront presèrvâs.
Totes les accions d’importacion entèrvouiqui sont consèrvâs dens lo [[Special:Log/import|jornal d’impôrt]].',
'import-interwiki-history'   => 'Copiyér totes les vèrsions de l’historico de ceta pâge',
'import-interwiki-submit'    => 'Importar',
'import-interwiki-namespace' => 'Transferar les pâges dens l’èspâço de nom :',
'importtext'                 => 'Volyéd èxportar lo fichiér dês lo vouiqui d’origina en utilisent l’outil [[Special:Export]], lo sôvar sur voutron disco dur et pués lo copiyér ique.',
'importstart'                => 'Impôrt de les pâges...',
'import-revision-count'      => '$1 {{PLURAL:$1|vèrsion|vèrsions}}',
'importnopages'              => 'Niona pâge a importar.',
'importfailed'               => 'Falyita de l’impôrt : $1',
'importunknownsource'        => 'Tipo de la sôrsa d’impôrt encognu',
'importcantopen'             => 'Empossiblo d’uvrir lo fichiér a importar',
'importbadinterwiki'         => 'Môvés lim entèrvouiqui',
'importnotext'               => 'Vouedo ou sen tèxte',
'importsuccess'              => 'L’impôrt at reussi !',
'importhistoryconflict'      => 'Y at un conflit dens l’historico de les vèrsions (ceta pâge at possu étre importâ dês devant).',
'importnosources'            => 'Niona sôrsa entèrvouiqui at étâ dèfenia et la copia drêta d’historico est dèsactivâ.',
'importnofile'               => 'Nion fichiér at étâ importâ.',
'importuploaderrorsize'      => 'Lo tèlèchargement du fichiér a importar at pas reussi. Sa talye est ples granta que cela ôtorisâ.',
'importuploaderrorpartial'   => 'Lo tèlèchargement du fichiér a importar at pas reussi. Ceti l’at étâ ren qu’a mêtiêt.',
'importuploaderrortemp'      => 'Lo tèlèchargement du fichiér a importar at pas reussi. Un dossiér temporèro est manquent.',
'import-parse-failure'       => 'Arrét dens l’analisa de l’impôrt XML',
'import-noarticle'           => 'Niona pâge a importar !',
'import-nonewrevisions'      => 'Totes les vèrsions ont étâ importâs dês devant.',
'xml-error-string'           => '$1 a la legne $2, col $3 (octèt $4) : $5',

# Import log
'importlogpage'                    => 'Historico de les importacions de pâges',
'importlogpagetext'                => 'Impôrts administratifs de pâges avouéc l’historico dês los ôtros vouiquis.',
'import-logentry-upload'           => 'at importâ (tèlèchargement) [[$1]]',
'import-logentry-upload-detail'    => '$1 {{PLURAL:$1|vèrsion|vèrsions}}',
'import-logentry-interwiki'        => 'at importâ (entèrvouiqui) [[$1]]',
'import-logentry-interwiki-detail' => '$1 {{PLURAL:$1|vèrsion|vèrsions}} dês $2',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'Ma pâge utilisator',
'tooltip-pt-anonuserpage'         => 'La pâge utilisator de l’IP avouéc laquinta vos contribuâd',
'tooltip-pt-mytalk'               => 'Ma pâge de discussion',
'tooltip-pt-anontalk'             => 'La pâge de discussion por ceta adrèce IP',
'tooltip-pt-preferences'          => 'Mes prèferences',
'tooltip-pt-watchlist'            => 'La lista de les pâges que vos siude',
'tooltip-pt-mycontris'            => 'La lista de mes contribucions',
'tooltip-pt-login'                => 'Vos éte envitâ a vos identifiar, mas cen est pas oblegatouèro.',
'tooltip-pt-anonlogin'            => 'Vos éte envitâ a vos identifiar, mas cen est pas oblegatouèro.',
'tooltip-pt-logout'               => 'Sè dèconèctar',
'tooltip-ca-talk'                 => 'Discussion a propôs de ceta pâge',
'tooltip-ca-edit'                 => 'Vos pouede modifiar ceta pâge. Marci de prèvisualisar devant qu’enregistrar.',
'tooltip-ca-addsection'           => 'Apondre un comentèro a ceta discussion',
'tooltip-ca-viewsource'           => 'Ceta pâge est protègiê. Portant vos pouede nen vêre lo contegnu.',
'tooltip-ca-history'              => 'Los ôtors et les vèrsions prècèdentes de ceta pâge',
'tooltip-ca-protect'              => 'Protègiér ceta pâge',
'tooltip-ca-delete'               => 'Suprimar ceta pâge',
'tooltip-ca-undelete'             => 'Rèstorar ceta pâge',
'tooltip-ca-move'                 => 'Renomar ceta pâge',
'tooltip-ca-watch'                => 'Apondéd ceta pâge a voutra lista de siuvu.',
'tooltip-ca-unwatch'              => 'Enlevâd ceta pâge de voutra lista de siuvu.',
'tooltip-search'                  => 'Chèrchiér dens {{SITENAME}}',
'tooltip-search-go'               => 'Alâd vers una pâge portent justo ceti nom s’ègziste.',
'tooltip-search-fulltext'         => 'Chèrchiéd les pâges presentent ceti tèxte.',
'tooltip-p-logo'                  => 'Pâge principâla',
'tooltip-n-mainpage'              => 'Visitâd la pâge principâla.',
'tooltip-n-portal'                => 'A propôs du projèt',
'tooltip-n-currentevents'         => 'Trovar des enformacions sur les dèrriéres novèles',
'tooltip-n-recentchanges'         => 'Lista des dèrriérs changements sur lo vouiqui',
'tooltip-n-randompage'            => 'Afichiér una pâge a l’hasârd',
'tooltip-n-help'                  => 'Éde a propôs du projèt',
'tooltip-t-whatlinkshere'         => 'Lista de les pâges liyês a ceta',
'tooltip-t-recentchangeslinked'   => 'Lista des dèrriérs changements de les pâges liyês a ceta',
'tooltip-feed-rss'                => 'Flux RSS por ceta pâge',
'tooltip-feed-atom'               => 'Flux Atom por ceta pâge',
'tooltip-t-contributions'         => 'Vêde la lista de les contribucions de ceti utilisator.',
'tooltip-t-emailuser'             => 'Emmandâd un mèl a ceti utilisator.',
'tooltip-t-upload'                => 'Importâd una émâge ou un fichiér multimèdia sur lo sèrvior.',
'tooltip-t-specialpages'          => 'Lista de totes les pâges spèciâles',
'tooltip-t-print'                 => 'Vèrsion emprimâbla de ceta pâge',
'tooltip-t-permalink'             => 'Lim fixo vers ceta vèrsion de la pâge',
'tooltip-ca-nstab-main'           => 'Vêre l’articllo',
'tooltip-ca-nstab-user'           => 'Vêre la pâge utilisator',
'tooltip-ca-nstab-media'          => 'Vêre la pâge mèdia',
'tooltip-ca-nstab-special'        => 'Cen est una pâge spèciâla, vos pouede pas la modifiar.',
'tooltip-ca-nstab-project'        => 'Vêre la pâge du projèt',
'tooltip-ca-nstab-image'          => 'Vêre la pâge de l’émâge',
'tooltip-ca-nstab-mediawiki'      => 'Vêre lo mèssâjo sistèmo',
'tooltip-ca-nstab-template'       => 'Vêre lo modèlo',
'tooltip-ca-nstab-help'           => 'Vêre la pâge d’éde',
'tooltip-ca-nstab-category'       => 'Vêre la pâge de la catègorie',
'tooltip-minoredit'               => 'Marcar mes modificacions coment minores',
'tooltip-save'                    => 'Sôve les modificacions (éd-vos prèvisualisâ dês devant ?).',
'tooltip-preview'                 => 'Marci de prèvisualisar voutres modificacions devant que sôvar !',
'tooltip-diff'                    => 'Pèrmèt de visualisar los changements que vos éd fêts.',
'tooltip-compareselectedversions' => 'Afichiér les difèrences entre doves vèrsions de ceta pâge',
'tooltip-watch'                   => 'Apondre ceta pâge a voutra lista de siuvu',
'tooltip-recreate'                => 'Recrèar la pâge mémo se ceta at étâ èfaciê',
'tooltip-upload'                  => 'Comenciér l’impôrt',

# Stylesheets
'common.css'   => '/* Lo CSS placiê ique serat aplicâ a totes les entèrfaces. */',
'monobook.css' => '/* Lo CSS placiê ique afècterat los utilisators de l’entèrface Monobook. */',

# Scripts
'common.js'   => '/* Quint que seye lo JavaScript placiê ique serat chargiê por tôs los utilisators et por châque pâge accèdâ. */',
'monobook.js' => '/* Dèplaciê vers [[MediaWiki:Common.js]]. */',

# Metadata
'nodublincore'      => 'Les mètabalyês « Dublin Core RDF » sont dèsactivâs sur ceti sèrvior.',
'nocreativecommons' => 'Les mètabalyês « Creative Commons RDF » sont dèsactivâs sur ceti sèrvior.',
'notacceptable'     => 'Lo sèrvior vouiqui pôt pas fornir les balyês dens un format que voutron cliant est capâblo de liére.',

# Attribution
'anonymous'        => 'Utilisator(s) pas enregistrâ(s) de {{SITENAME}}',
'siteuser'         => 'Utilisator $1 de {{SITENAME}}',
'lastmodifiedatby' => 'Ceta pâge at étâ modifiâ por lo dèrriér côp lo $1 a $2 per $3.', # $1 date, $2 time, $3 user
'othercontribs'    => 'Basâ sur l’ôvra de $1.',
'others'           => 'ôtros',
'siteusers'        => 'Utilisator(s) $1 de {{SITENAME}}',
'creditspage'      => 'Pâge de crèdits',
'nocredits'        => 'Y at pas d’enformacions d’atribucion disponibles por ceta pâge.',

# Spam protection
'spamprotectiontitle' => 'Pâge ôtomaticament protègiê a côsa de spame',
'spamprotectiontext'  => 'La pâge que vos éd tâchiê de sôvar at étâ blocâ per lo filtro antispame. Cen est probâblament côsâ per un lim vers un seto de defôr.',
'spamprotectionmatch' => "La chêna de caractèros '''« $1 »''' at dècllenchiê lo dècelior de spame.",
'spambot_username'    => 'Neteyâjo de spame per MediaWiki',
'spam_reverting'      => 'Rèstoracion de la dèrriére vèrsion contegnent pas de lim vers $1.',
'spam_blanking'       => 'Totes les vèrsions contegnent des lims vers $1 sont blanchies.',

# Info page
'infosubtitle'   => 'Enformacions por la pâge',
'numedits'       => 'Nombro de modificacions : $1',
'numtalkedits'   => 'Nombro de modificacions (pâge de discussion) : $1',
'numwatchers'    => 'Nombro de contributors èyent la pâge dens lor lista de siuvu : $1',
'numauthors'     => 'Nombro d’ôtors difèrents : $1',
'numtalkauthors' => 'Nombro d’ôtors difèrents (pâge de discussion) : $1',

# Math options
'mw_math_png'    => 'Tojorn fâre una émâge PNG',
'mw_math_simple' => 'HTML se prod simplo, ôtrament PNG',
'mw_math_html'   => 'HTML se possiblo, ôtrament PNG',
'mw_math_source' => 'Lèssiér lo code TeX originâl',
'mw_math_modern' => 'Por los navigators modèrnos',
'mw_math_mathml' => 'MathML se possiblo (èxpèrimentâl)',

# Patrolling
'markaspatrolleddiff'                 => 'Marcar coment étent pas un vandalismo',
'markaspatrolledtext'                 => 'Marcar ceti articllo coment pas vandalisâ',
'markedaspatrolled'                   => 'Marcâ coment pas vandalisâ',
'markedaspatrolledtext'               => 'La vèrsion sèlèccionâ at étâ marcâ coment pas vandalisâ.',
'rcpatroldisabled'                    => 'La fonccion de patrolye des dèrriérs changements est pas activâ.',
'rcpatroldisabledtext'                => 'La fonccionalitât de survelyence des dèrriérs changements est pas activâ.',
'markedaspatrollederror'              => 'Pôt pas étre marcâ coment pas vandalisâ.',
'markedaspatrollederrortext'          => 'Vos dête sèlèccionar una vèrsion por povêr la marcar coment pas vandalisâ.',
'markedaspatrollederror-noautopatrol' => 'Vos avéd pas lo drêt de marcar voutres prôpres modificacions coment survelyês.',

# Patrol log
'patrol-log-page' => 'Historico de les vèrsions patrolyês',
'patrol-log-line' => 'at marcâ la vèrsion $1 de $2 coment survelyê $3',
'patrol-log-auto' => '(ôtomatico)',
'patrol-log-diff' => '$1',

# Image deletion
'deletedrevision'                 => 'La vielye vèrsion $1 at étâ suprimâ.',
'filedeleteerror-short'           => 'Èrror pendent la suprèssion du fichiér : $1',
'filedeleteerror-long'            => 'Des èrrors ont étâ rencontrâs pendent la suprèssion du fichiér :\\n\\n$1\\n',
'filedelete-missing'              => 'Lo fichiér « $1 » pôt pas étre suprimâ perce qu’ègziste pas.',
'filedelete-old-unregistered'     => 'La vèrsion du fichiér spècefiâ « $1 » est pas dens la bâsa de balyês.',
'filedelete-current-unregistered' => 'Lo fichiér spècefiâ « $1 » est pas dens la bâsa de balyês.',
'filedelete-archive-read-only'    => 'Lo dossiér d’arch·ivâjo « $1 » est pas modifiâblo per lo sèrvior.',

# Browsing diffs
'previousdiff' => '← Difèrence prècèdenta',
'nextdiff'     => 'Difèrence siuventa →',

# Media information
'mediawarning'         => '<b>Atencion :</b> ceti fichiér pôt contegnir de code mâlvelyent, voutron sistèmo pôt étre betâ en dangiér per son ègzécucion.
<hr />',
'imagemaxsize'         => 'Format lo ples grant por les émâges dens les pâges de dèscripcion d’émâges :',
'thumbsize'            => 'Talye de la figura :',
'widthheightpage'      => '$1×$2, $3 pâges',
'file-info'            => '(Talye du fichiér : $1, tipo MIME : $2)',
'file-info-size'       => '($1 × $2 pixèles, talye du fichiér : $3, tipo MIME : $4)',
'file-nohires'         => '<small>Pas de rèsolucion ples hôta disponibla.</small>',
'svg-long-desc'        => '(Fichiér SVG, rèsolucion de $1 × $2 pixèles, talye : $3)',
'show-big-image'       => 'Émâge en rèsolucion ples hôta',
'show-big-image-thumb' => '<small>Talye de ceta vua : $1 × $2 pixèles</small>',

# Special:NewImages
'newimages'             => 'Galerie des novéls fichiérs',
'imagelisttext'         => "Vê-que una lista de '''$1''' {{PLURAL:$1|fichiér cllassiê|fichiérs cllassiês}} $2.",
'showhidebots'          => '($1 bots)',
'noimages'              => 'Niona émâge a afichiér.',
'ilsubmit'              => 'Chèrchiér',
'bydate'                => 'per dâta',
'sp-newimages-showfrom' => 'Afichiér les émâges importâs dês lo $1',

# Bad image list
'bad_image_list' => 'Lo format est lo siuvent :

Solament les listes d’ènumèracion (les legnes comencient per *) sont tegnues compto. Lo premiér lim d’una legne dêt étre vers celi d’una môvésa émâge.
Los ôtros lims sur la méma legne sont considèrâs coment des èxcèpcions, per ègzemplo des articllos sur losquints l’émâge dêt aparêtre.',

# Metadata
'metadata'          => 'Mètabalyês',
'metadata-help'     => 'Ceti fichiér contint des enformacions suplèmentères probâblament apondues per l’aparèly-fotô numerico ou lo scanor que l’at fêt. Se lo fichiér at étâ modifiâ dês son ètat originâl, cèrtins dètalys pôvont pas reflètar a chavon l’émâge modifiâ.',
'metadata-expand'   => 'Montrar les enformacions dètalyês',
'metadata-collapse' => 'Cachiér les enformacions dètalyês',
'metadata-fields'   => 'Los champs de mètabalyês d’EXIF listâs dens ceti mèssâjo seront
encllus dens la pâge de dèscripcion de l’émâge quand la trâbla de mètabalyês
serat rèduita. Los ôtros champs seront cachiês per dèfôt.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => 'Largior',
'exif-imagelength'                 => 'Hôtior',
'exif-bitspersample'               => 'Bits per èchantelyon',
'exif-compression'                 => 'Tipo de comprèssion',
'exif-photometricinterpretation'   => 'Modèlo colorimètrico',
'exif-orientation'                 => 'Oriantacion',
'exif-samplesperpixel'             => 'Composentes per pixèle',
'exif-planarconfiguration'         => 'Arrengement de les balyês',
'exif-ycbcrsubsampling'            => 'Quota d’èchantelyonâjo de les composentes de la crominance',
'exif-ycbcrpositioning'            => 'Posicionement YCbCr',
'exif-xresolution'                 => 'Rèsolucion plana',
'exif-yresolution'                 => 'Rèsolucion drêta',
'exif-resolutionunit'              => 'Unitât de rèsolucion',
'exif-stripoffsets'                => 'Emplacement de les balyês de l’émâge',
'exif-rowsperstrip'                => 'Nombro de legnes per benda',
'exif-stripbytecounts'             => 'Talye en octèts per benda',
'exif-jpeginterchangeformat'       => 'Posicion du SOI JPEG',
'exif-jpeginterchangeformatlength' => 'Talye en octèts de les balyês JPEG',
'exif-transferfunction'            => 'Fonccion de transfèrt',
'exif-whitepoint'                  => 'Cromaticitât du pouent blanc',
'exif-primarychromaticities'       => 'Cromaticitât de les colors primères',
'exif-ycbcrcoefficients'           => 'Coèficients YCbCr',
'exif-referenceblackwhite'         => 'Valors de refèrence nêr et blanc',
'exif-datetime'                    => 'Dâta et hora de changement du fichiér',
'exif-imagedescription'            => 'Dèscripcion de l’émâge',
'exif-make'                        => 'Fabrecant de l’aparèly',
'exif-model'                       => 'Modèlo de l’aparèly',
'exif-software'                    => 'Programeria utilisâ',
'exif-artist'                      => 'Ôtor',
'exif-copyright'                   => 'Dètentor du drêt d’ôtor',
'exif-exifversion'                 => 'Vèrsion EXIF',
'exif-flashpixversion'             => 'Vèrsion FlashPix',
'exif-colorspace'                  => 'Èspâço colorimètrico',
'exif-componentsconfiguration'     => 'Configuracion de les composentes',
'exif-compressedbitsperpixel'      => 'Valor de comprèssion de l’émâge',
'exif-pixelydimension'             => 'Hôtior d’émâge valida',
'exif-pixelxdimension'             => 'Largior d’émâge valida',
'exif-makernote'                   => 'Notes du fabrecant',
'exif-usercomment'                 => 'Comentèros de l’utilisator',
'exif-relatedsoundfile'            => 'Fichiér ôdiô associyê',
'exif-datetimeoriginal'            => 'Dâta et hora de la g·ènèracion de balyês',
'exif-datetimedigitized'           => 'Dâta et hora de numerisacion',
'exif-subsectime'                  => 'Dâta de dèrriére modificacion',
'exif-subsectimeoriginal'          => 'Dâta de la prêsa originèla',
'exif-subsectimedigitized'         => 'Dâta de la numerisacion',
'exif-exposuretime'                => 'Temps d’èxposicion',
'exif-exposuretime-format'         => '$1 s ($2)',
'exif-fnumber'                     => 'Nombro f',
'exif-exposureprogram'             => 'Programe d’èxposicion',
'exif-spectralsensitivity'         => 'Sensibilitât spèctrâla',
'exif-isospeedratings'             => 'Sensibilitât ISO',
'exif-oecf'                        => 'Fonccion de convèrsion optoèlèctronica',
'exif-shutterspeedvalue'           => 'Vitèsse d’ètopâ',
'exif-aperturevalue'               => 'Uvèrtura',
'exif-brightnessvalue'             => 'Luminositât',
'exif-exposurebiasvalue'           => 'Corrèccion d’èxposicion',
'exif-maxaperturevalue'            => 'Uvèrtura la ples granta',
'exif-subjectdistance'             => 'Distance du sujèt',
'exif-meteringmode'                => 'Fôrma de mesera',
'exif-lightsource'                 => 'Sôrsa de lumiére',
'exif-flash'                       => 'Èludo',
'exif-focallength'                 => 'Longior focâla',
'exif-subjectarea'                 => 'Emplacement du sujèt',
'exif-flashenergy'                 => 'Ènèrg·ie de l’èludo',
'exif-spatialfrequencyresponse'    => 'Frèquence espaciâla',
'exif-focalplanexresolution'       => 'Rèsolucion X de focâla plana',
'exif-focalplaneyresolution'       => 'Rèsolucion Y de focâla plana',
'exif-focalplaneresolutionunit'    => 'Unitât de rèsolucion de focâla plana',
'exif-subjectlocation'             => 'Posicion du sujèt',
'exif-exposureindex'               => 'Endèxe d’èxposicion',
'exif-sensingmethod'               => 'Tipo de captior',
'exif-filesource'                  => 'Sôrsa du fichiér',
'exif-scenetype'                   => 'Tipo de scèna',
'exif-cfapattern'                  => 'Matrice de filtrâjo de color',
'exif-customrendered'              => 'Rendu pèrsonalisâ',
'exif-exposuremode'                => 'Fôrma d’èxposicion',
'exif-whitebalance'                => 'Balance des blancs',
'exif-digitalzoomratio'            => 'Quota d’agrantissement numerica (zoom)',
'exif-focallengthin35mmfilm'       => 'Longior de focâla por un filme 35 mm',
'exif-scenecapturetype'            => 'Tipo de prêsa de la scèna',
'exif-gaincontrol'                 => 'Contrôlo de luminositât',
'exif-contrast'                    => 'Contraste',
'exif-saturation'                  => 'Saturacion',
'exif-sharpness'                   => 'Prècision',
'exif-devicesettingdescription'    => 'Dèscripcion de la configuracion du dispositif',
'exif-subjectdistancerange'        => 'Distance du sujèt',
'exif-imageuniqueid'               => 'Identifiant unico de l’émâge',
'exif-gpsversionid'                => 'Vèrsion du tag GPS',
'exif-gpslatituderef'              => 'Refèrence por la latituda',
'exif-gpslatitude'                 => 'Latituda',
'exif-gpslongituderef'             => 'Refèrence por la longituda',
'exif-gpslongitude'                => 'Longituda',
'exif-gpsaltituderef'              => 'Refèrence d’hôtior',
'exif-gpsaltitude'                 => 'Hôtior',
'exif-gpstimestamp'                => 'Hora GPS (relojo atomico)',
'exif-gpssatellites'               => 'Satèlites utilisâs por la mesera',
'exif-gpsstatus'                   => 'Statut recevior',
'exif-gpsmeasuremode'              => 'Fôrma de mesera',
'exif-gpsdop'                      => 'Prècision de la mesera',
'exif-gpsspeedref'                 => 'Unitât de vitèsse du recevior GPS',
'exif-gpsspeed'                    => 'Vitèsse du recevior GPS',
'exif-gpstrackref'                 => 'Refèrence por la dirèccion du mouvement',
'exif-gpstrack'                    => 'Dirèccion du mouvement',
'exif-gpsimgdirectionref'          => 'Refèrence por l’oriantacion de l’émâge',
'exif-gpsimgdirection'             => 'Oriantacion de l’émâge',
'exif-gpsmapdatum'                 => 'Sistèmo g·eodèsico utilisâ',
'exif-gpsdestlatituderef'          => 'Refèrence por la latituda de la dèstinacion',
'exif-gpsdestlatitude'             => 'Latituda de la dèstinacion',
'exif-gpsdestlongituderef'         => 'Refèrence por la longituda de la dèstinacion',
'exif-gpsdestlongitude'            => 'Longituda de la dèstinacion',
'exif-gpsdestbearingref'           => 'Refèrence por lo relevament de la dèstinacion',
'exif-gpsdestbearing'              => 'Relevament de la dèstinacion',
'exif-gpsdestdistanceref'          => 'Refèrence por la distance a la dèstinacion',
'exif-gpsdestdistance'             => 'Distance a la dèstinacion',
'exif-gpsprocessingmethod'         => 'Nom de la mètoda de trètament du GPS',
'exif-gpsareainformation'          => 'Nom de la zona GPS',
'exif-gpsdatestamp'                => 'Dâta GPS',
'exif-gpsdifferential'             => 'Corrèccion difèrencièla GPS',

# EXIF attributes
'exif-compression-1' => 'Pas comprèssâ',

'exif-unknowndate' => 'Dâta encognua',

'exif-orientation-1' => 'Normala', # 0th row: top; 0th column: left
'exif-orientation-2' => 'Envèrsâ d’aplan', # 0th row: top; 0th column: right
'exif-orientation-3' => 'Veriê de 180°', # 0th row: bottom; 0th column: right
'exif-orientation-4' => 'Envèrsâ d’aplomb', # 0th row: bottom; 0th column: left
'exif-orientation-5' => 'Veriê de 90° a gôche et envèrsâ d’aplomb', # 0th row: left; 0th column: top
'exif-orientation-6' => 'Veriê de 90° a drêta', # 0th row: right; 0th column: top
'exif-orientation-7' => 'Veriê de 90° a drêta et envèrsâ d’aplomb', # 0th row: right; 0th column: bottom
'exif-orientation-8' => 'Veriê de 90° a gôche', # 0th row: left; 0th column: bottom

'exif-planarconfiguration-1' => 'Balyês ategnentes',
'exif-planarconfiguration-2' => 'Balyês sèparâs',

'exif-colorspace-ffff.h' => 'Pas calibrâ',

'exif-componentsconfiguration-0' => 'Ègziste pas',

'exif-exposureprogram-0' => 'Pas dèfeni',
'exif-exposureprogram-1' => 'Manuèl',
'exif-exposureprogram-2' => 'Programe normal',
'exif-exposureprogram-3' => 'Prioritât a l’uvèrtura',
'exif-exposureprogram-4' => 'Prioritât a l’ètopior',
'exif-exposureprogram-5' => 'Programe crèacion (prèference a la provondior de champ)',
'exif-exposureprogram-6' => 'Programe accion (prèference a la vitèsse d’ètopâ)',
'exif-exposureprogram-7' => 'Môdo portrèt (por clich·ês de prés avouéc fond troblo)',
'exif-exposureprogram-8' => 'Môdo payisâjo (por des clich·ês de payisâjos nèts)',

'exif-subjectdistance-value' => '{{PLURAL:$1|$1 mètre|$1 mètres}}',

'exif-meteringmode-0'   => 'Encognua',
'exif-meteringmode-1'   => 'Moyena',
'exif-meteringmode-2'   => 'Mesera centrâla moyena',
'exif-meteringmode-3'   => 'Pouent',
'exif-meteringmode-4'   => 'MultiPouent',
'exif-meteringmode-5'   => 'Palèta',
'exif-meteringmode-6'   => 'Parcièla',
'exif-meteringmode-255' => 'Ôtra',

'exif-lightsource-0'   => 'Encognua',
'exif-lightsource-1'   => 'Lumiére du jorn',
'exif-lightsource-2'   => 'Fluorèscenta',
'exif-lightsource-3'   => 'Tungstène (lumiére chôdâ a blanc)',
'exif-lightsource-4'   => 'Èludo',
'exif-lightsource-9'   => 'Temps cllâr',
'exif-lightsource-10'  => 'Temps enneblo',
'exif-lightsource-11'  => 'Ombra',
'exif-lightsource-12'  => 'Ècllèrâjo fluorèscent « lumiére du jorn » (D 5700 – 7100K)',
'exif-lightsource-13'  => 'Ècllèrâjo fluorèscent blanc « jorn » (N 4600 – 5400K)',
'exif-lightsource-14'  => 'Ècllèrâjo fluorèscent blanc « frêd » (W 3900 – 4500K)',
'exif-lightsource-15'  => 'Ècllèrâjo fluorèscent blanc (WW 3200 – 3700K)',
'exif-lightsource-17'  => 'Lumiére estandârd A',
'exif-lightsource-18'  => 'Lumiére estandârd B',
'exif-lightsource-19'  => 'Lumiére estandârd C',
'exif-lightsource-24'  => 'Tungstène ISO de studiô',
'exif-lightsource-255' => 'Ôtra sôrsa de lumiére',

'exif-focalplaneresolutionunit-2' => 'pôjos',

'exif-sensingmethod-1' => 'Pas dèfeni',
'exif-sensingmethod-2' => 'Captior de zona de colors monocromatiques',
'exif-sensingmethod-3' => 'Captior de zona de colors bicromatiques',
'exif-sensingmethod-4' => 'Captior de zona de colors tricromatiques',
'exif-sensingmethod-5' => 'Captior color sèquencièl',
'exif-sensingmethod-7' => 'Captior trilinèâr',
'exif-sensingmethod-8' => 'Captior color linèâr sèquencièl',

'exif-filesource-3' => 'Aparèly-fotô numerico',

'exif-scenetype-1' => 'Émâge tot drêt fotografiâ',

'exif-customrendered-0' => 'Mètoda normala',
'exif-customrendered-1' => 'Mètoda pèrsonalisâ',

'exif-exposuremode-0' => 'Ôtomatica',
'exif-exposuremode-1' => 'Manuèla',
'exif-exposuremode-2' => 'Misa entre-mié parentèses ôtomatica',

'exif-whitebalance-0' => 'Ôtomatica',
'exif-whitebalance-1' => 'Manuèla',

'exif-scenecapturetype-0' => 'Estandârd',
'exif-scenecapturetype-1' => 'Payisâjo',
'exif-scenecapturetype-2' => 'Portrèt',
'exif-scenecapturetype-3' => 'Scèna de nuet',

'exif-gaincontrol-0' => 'Nion',
'exif-gaincontrol-1' => 'Ôgmentacion fêbla de l’aquis',
'exif-gaincontrol-2' => 'Ôgmentacion fôrta de l’aquis',
'exif-gaincontrol-3' => 'Rèduccion fêbla de l’aquis',
'exif-gaincontrol-4' => 'Rèduccion fôrta de l’aquis',

'exif-contrast-0' => 'Normal',
'exif-contrast-1' => 'Fêblo',
'exif-contrast-2' => 'Fôrt',

'exif-saturation-0' => 'Normala',
'exif-saturation-1' => 'Fêbla',
'exif-saturation-2' => 'Hôta',

'exif-sharpness-0' => 'Normala',
'exif-sharpness-1' => 'Doce',
'exif-sharpness-2' => 'Dura',

'exif-subjectdistancerange-0' => 'Encognua',
'exif-subjectdistancerange-1' => 'Vision en grôs',
'exif-subjectdistancerange-2' => 'Vision de prés',
'exif-subjectdistancerange-3' => 'Vision de luen',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => "Bise (''nord'')",
'exif-gpslatitude-s' => "Mié-jorn (''sud'')",

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => "Levant (''èst'')",
'exif-gpslongitude-w' => "Ponant (''ouèst'')",

'exif-gpsstatus-a' => 'Mesera en cors',
'exif-gpsstatus-v' => 'Entèropèrabilitât de la mesera',

'exif-gpsmeasuremode-2' => 'Mesera a 2 dimensions',
'exif-gpsmeasuremode-3' => 'Mesera a 3 dimensions',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'Kilomètre per hora',
'exif-gpsspeed-m' => 'Mile per hora',
'exif-gpsspeed-n' => 'Nuod',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => 'Veré dirèccion',
'exif-gpsdirection-m' => "Bise magnètica (''nord magnètico'')",

# External editor support
'edit-externally'      => 'Modifiar ceti fichiér en utilisent una aplicacion de defôr',
'edit-externally-help' => 'Vêde les [http://www.mediawiki.org/wiki/Manual:External_editors enstruccions] por més d’enformacions.',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'totes',
'imagelistall'     => 'totes',
'watchlistall2'    => 'tot',
'namespacesall'    => 'Tôs',
'monthsall'        => 'tôs',

# E-mail address confirmation
'confirmemail'            => 'Confirmar l’adrèce de mèl',
'confirmemail_noemail'    => 'L’adrèce de mèl configurâ dens voutres [[Special:Preferences|prèferences]] est envalida.',
'confirmemail_text'       => '{{SITENAME}} at fôta du contrôlo de voutra adrèce de mèl devant que povêr utilisar tota fonccion de mèssageria. Utilisâd lo boton ce-desot por emmandar un mèl de confirmacion a voutra adrèce. Lo mèl contindrat un lim contegnent un code, chargiéd cél lim dens voutron navigator por validar voutra adrèce.',
'confirmemail_pending'    => '<div class="error">
Un code de confirmacion vos at ja étâ emmandâ per mèl ; se vos vegnéd de crèar voutron compto, volyéd atendre doux-três menutes que lo mèl arreve devant que demandar un code novél.
</div>',
'confirmemail_send'       => 'Emmandar un code de confirmacion',
'confirmemail_sent'       => 'Mèl de confirmacion emmandâ.',
'confirmemail_oncreate'   => 'Un code de confirmacion at étâ emmandâ a voutra adrèce de mèl. Cél code est pas requis por sè conèctar, mas vos en aréd fôta por activar les fonccionalitâts liyês ux mèls sur ceti vouiqui.',
'confirmemail_sendfailed' => 'Empossiblo d’emmandar lo mèl de confirmacion. Controlâd voutra adrèce.

Retôrn du programe de mèl : $1',
'confirmemail_invalid'    => 'Code de confirmacion fôx. Lo code at pôt-étre èxpirâ.',
'confirmemail_needlogin'  => 'Vos dête vos $1 por confirmar voutra adrèce de mèl.',
'confirmemail_success'    => 'Voutra adrèce de mèl est confirmâ. Orendrêt, vos pouede vos conèctar et profitar du vouiqui.',
'confirmemail_loggedin'   => 'Voutra adrèce est confirmâ ora.',
'confirmemail_error'      => 'Un problèmo est arrevâ en volent enregistrar voutra confirmacion.',
'confirmemail_subject'    => 'Confirmacion d’adrèce de mèl por {{SITENAME}}',
'confirmemail_body'       => 'Quârqu’un, probâblament vos avouéc l’adrèce IP $1, at enregistrâ un compto « $2 » avouéc ceta adrèce de mèl sur lo seto {{SITENAME}}.

Por confirmar que cél compto est franc a vos et activar les fonccions de mèssageria dessus {{SITENAME}}, volyéd siuvre lo lim ce-desot dens voutron navigator :

$3

Se s’ag·ét pas de vos, uvréd pas lo lim. Cél code de confirmacion èxpirerat lo $4.',

# Scary transclusion
'scarytranscludedisabled' => '[La transcllusion entèrvouiqui est dèsactivâ]',
'scarytranscludefailed'   => '[La rècupèracion de modèlo at pas reussia por $1 ; dèsolâ]',
'scarytranscludetoolong'  => '[L’URL est trop longe ; dèsolâ]',

# Trackbacks
'trackbackbox'      => '<div id="mw_trackbacks">
Rètrolims vers ceti articllo :<br />
$1
</div>',
'trackbackremove'   => '([$1 Suprimar])',
'trackbacklink'     => 'Rètrolim',
'trackbackdeleteok' => 'Lo rètrolim at étâ suprimâ avouéc reusséta.',

# Delete conflict
'deletedwhileediting' => 'Atencion : ceta pâge at étâ suprimâ aprés que vos éd comenciê a la modifiar !',
'confirmrecreate'     => "L’utilisator [[User:$1|$1]] ([[User talk:$1|Discussion]]) at suprimâ ceta pâge, pendent que vos aviâd comenciê a l’èditar, por la rêson siuventa :
: ''$2''
Volyéd confirmar que vos dèsirâd recrèar ceti articllo.",
'recreate'            => 'Recrèar',

# HTML dump
'redirectingto' => 'Redirèccion vers [[:$1]]...',

# action=purge
'confirm_purge'        => 'Voléd-vos rafrèchir ceta pâge (purgiér lo cache) ?

$1',
'confirm_purge_button' => 'Confirmar',

# AJAX search
'searchcontaining' => 'Chèrchiér los articllos contegnent « $1 ».',
'searchnamed'      => 'Chèrchiér los articllos apelâs « $1 ».',
'articletitles'    => 'Articllos comencient per « $1 »',
'hideresults'      => 'Cachiér los rèsultats',
'useajaxsearch'    => 'Utilisar la rechèrche AJAX',

# Multipage image navigation
'imgmultipageprev' => '← pâge prècèdenta',
'imgmultipagenext' => 'pâge siuventa →',
'imgmultigo'       => 'Accèdar !',

# Table pager
'ascending_abbrev'         => 'crès',
'descending_abbrev'        => 'dècr',
'table_pager_next'         => 'Pâge siuventa',
'table_pager_prev'         => 'Pâge prècèdenta',
'table_pager_first'        => 'Premiére pâge',
'table_pager_last'         => 'Dèrriére pâge',
'table_pager_limit'        => 'Montrar $1 èlèments per pâge',
'table_pager_limit_submit' => 'Accèdar',
'table_pager_empty'        => 'Nion rèsultat',

# Auto-summaries
'autosumm-blank'   => 'Rèsumâ ôtomatico : blanchiment',
'autosumm-replace' => 'Rèsumâ ôtomatico : contegnu remplaciê per « $1 ».',
'autoredircomment' => 'Redirèccion vers [[$1]]',
'autosumm-new'     => 'Novèla pâge : $1',

# Size units
'size-bytes'     => '$1 o',
'size-kilobytes' => '$1 ko',
'size-megabytes' => '$1 Mo',
'size-gigabytes' => '$1 Go',

# Live preview
'livepreview-loading' => 'Chargement…',
'livepreview-ready'   => 'Chargement… chavonâ !',
'livepreview-failed'  => 'La vua rapida at pas reussia !
Èprovâd la prèvisualisacion normala.',
'livepreview-error'   => 'Empossiblo de sè conèctar : $1 « $2 ».
Èprovâd la prèvisualisacion normala.',

# Friendlier slave lag warnings
'lag-warn-normal' => 'Les modificacions datent de muens de $1 secondes pôvont pas aparêtre dens ceta lista.',
'lag-warn-high'   => 'En rêson d’una fôrta charge de les bâses de balyês, les modificacions datent de muens de $1 secondes pôvont pas aparêtre dens ceta lista.',

# Watchlist editor
'watchlistedit-numitems'       => 'Voutra lista de siuvu contint {{PLURAL:$1|yona pâge|$1 pâges}}, sen comptar les pâges de discussion.',
'watchlistedit-noitems'        => 'Voutra lista de siuvu contint gins de pâge.',
'watchlistedit-normal-title'   => 'Modificacion de la lista de siuvu',
'watchlistedit-normal-legend'  => 'Enlevar des pâges de la lista de siuvu',
'watchlistedit-normal-explain' => 'Les pâges de voutra lista de siuvu sont visibles ce-desot, cllassiês per èspâço de nom. Por enlevar una pâge (et sa pâge de discussion) de la lista, sèlèccionâd la câsa a coutâ et pués clicâd sur lo boton d’avâl. Vos pouede asse-ben [[Special:Watchlist/raw|la modifiar en fôrma bruta]].',
'watchlistedit-normal-submit'  => 'Enlevar les pâges sèlèccionâs',
'watchlistedit-normal-done'    => '{{PLURAL:$1|Yona pâge at étâ enlevâ|$1 pâges ont étâ enlevâs}} de voutra lista de siuvu :',
'watchlistedit-raw-title'      => 'Modificacion de la lista de siuvu (fôrma bruta)',
'watchlistedit-raw-legend'     => 'Modificacion de la lista de siuvu en fôrma bruta',
'watchlistedit-raw-explain'    => 'La lista de les pâges de voutra lista de siuvu est montrâ ce-desot, sen les pâges de discussion (ôtomaticament encllues) et triyês per èspâço de nom. Vos pouede modifiar la lista : apondéd les pâges que vos voléd siuvre (yô que seye), yona pâge per legne, et enlevâd les pâges que vos voléd pas més siuvre. Quand vos éd feni, clicâd sur lo boton d’avâl por betar la lista a jorn. Vos pouede asse-ben utilisar l’[[Special:Watchlist/edit|èditor normal]].',
'watchlistedit-raw-titles'     => 'Pâges :',
'watchlistedit-raw-submit'     => 'Betar a jorn la lista de siuvu',
'watchlistedit-raw-done'       => 'Voutra lista de siuvu at étâ betâ a jorn.',
'watchlistedit-raw-added'      => '{{PLURAL:$1|Yona pâge at étâ apondua|$1 pâges ont étâ apondues}} :',
'watchlistedit-raw-removed'    => '{{PLURAL:$1|Yona pâge at étâ enlevâ|$1 pâges ont étâ enlevâs}} :',

# Watchlist editing tools
'watchlisttools-view' => 'Lista de siuvu',
'watchlisttools-edit' => 'Vêre et modifiar la lista de siuvu',
'watchlisttools-raw'  => 'Modifiar la lista de siuvu (fôrma bruta)',

# Iranian month names
'iranian-calendar-m1'  => 'de farvardin',
'iranian-calendar-m2'  => 'd’ordibehèch·et',
'iranian-calendar-m3'  => 'de c’hordād',
'iranian-calendar-m4'  => 'de tir',
'iranian-calendar-m5'  => 'de mordād',
'iranian-calendar-m6'  => 'de ch·ahrivar',
'iranian-calendar-m7'  => 'de mèhr',
'iranian-calendar-m8'  => 'd’ābān',
'iranian-calendar-m9'  => 'd’āzar',
'iranian-calendar-m10' => 'de dèy',
'iranian-calendar-m11' => 'de bahman',
'iranian-calendar-m12' => 'd’èsfand',

# Hebrew month names
'hebrew-calendar-m1'      => 'de tich·eri',
'hebrew-calendar-m2'      => 'd’hèch·evan',
'hebrew-calendar-m3'      => 'de quislèv',
'hebrew-calendar-m4'      => 'de tevèt',
'hebrew-calendar-m5'      => 'de ch·evât',
'hebrew-calendar-m6'      => 'd’adâr',
'hebrew-calendar-m6a'     => 'd’adâr I',
'hebrew-calendar-m6b'     => 'd’adâr II',
'hebrew-calendar-m7'      => 'de nissan',
'hebrew-calendar-m8'      => 'd’iyâr',
'hebrew-calendar-m9'      => 'de sivan',
'hebrew-calendar-m10'     => 'de tamouz',
'hebrew-calendar-m11'     => 'd’âv',
'hebrew-calendar-m12'     => 'd’èloul',
'hebrew-calendar-m1-gen'  => 'de tich·eri',
'hebrew-calendar-m2-gen'  => 'd’hèch·evan',
'hebrew-calendar-m3-gen'  => 'de quislèv',
'hebrew-calendar-m4-gen'  => 'de tevèt',
'hebrew-calendar-m5-gen'  => 'de ch·evât',
'hebrew-calendar-m6-gen'  => 'd’adâr',
'hebrew-calendar-m6a-gen' => 'd’adâr I',
'hebrew-calendar-m6b-gen' => 'd’adâr II',
'hebrew-calendar-m7-gen'  => 'de nissan',
'hebrew-calendar-m8-gen'  => 'd’iyâr',
'hebrew-calendar-m9-gen'  => 'de sivan',
'hebrew-calendar-m10-gen' => 'de tamouz',
'hebrew-calendar-m11-gen' => 'd’âv',
'hebrew-calendar-m12-gen' => 'd’èloul',

# Core parser functions
'unknown_extension_tag' => 'Balisa d’èxtension « $1 » encognua',

# Special:Version
'version'                          => 'Vèrsion', # Not used as normal message but as header for the special page itself
'version-extensions'               => 'Vèrsions enstalâs',
'version-specialpages'             => 'Pâges spèciâles',
'version-parserhooks'              => 'Apèls d’analises',
'version-variables'                => 'Variâbles',
'version-other'                    => 'De totes sôrtes',
'version-mediahandlers'            => 'Maneyors de mèdia',
'version-hooks'                    => 'Apèls',
'version-extension-functions'      => 'Fonccions de les èxtensions',
'version-parser-extensiontags'     => 'Balises d’analises vegnent de les èxtensions',
'version-parser-function-hooks'    => 'Apèls de les fonccions d’analisa',
'version-skin-extension-functions' => 'Fonccions d’entèrface d’èxtensions',
'version-hook-name'                => 'Nom de l’apèl',
'version-hook-subscribedby'        => 'Dèfeni per',
'version-version'                  => 'Vèrsion',
'version-license'                  => 'Licence',
'version-software'                 => 'Programeria enstalâ',
'version-software-product'         => 'Produit',
'version-software-version'         => 'Vèrsion',

# Special:FilePath
'filepath'         => 'Chemin d’accès d’un fichiér',
'filepath-page'    => 'Fichiér :',
'filepath-submit'  => 'Chemin d’accès',
'filepath-summary' => 'Ceta pâge afiche lo chemin d’accès complèt d’un fichiér ; les émâges sont montrâs en rèsolucion hôta, los fichiérs ôdiô et vidèô s’ègzécutont avouéc lor programe associyê.

Entrâd lo nom du fichiér sen lo prèfixe « {{ns:image}}: ».',

# Special:SpecialPages
'specialpages' => 'Pâges spèciâles',

);
