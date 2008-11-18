<?php
/** Italian (Italiano)
 *
 * @ingroup Language
 * @file
 *
 * @author .anaconda
 * @author Broc
 * @author BrokenArrow
 * @author Candalua
 * @author Cruccone
 * @author Cryptex
 * @author Darth Kule
 * @author Felis
 * @author Gianfranco
 * @author Martorell
 * @author Melos
 * @author Nemo bis
 * @author Nick1915
 * @author Pietrodn
 * @author Ramac
 * @author Remember the dot
 * @author S.Örvarr.S
 * @author SabineCretella
 * @author Tonyfroio
 * @author Xpensive
 * @author לערי ריינהארט
 */

$namespaceNames = array(
	NS_MEDIA            => 'Media',
	NS_SPECIAL          => 'Speciale',
	NS_MAIN             => '',
	NS_TALK             => 'Discussione',
	NS_USER             => 'Utente',
	NS_USER_TALK        => 'Discussioni_utente',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK     => 'Discussioni_$1',
	NS_IMAGE            => 'Immagine',
	NS_IMAGE_TALK       => 'Discussioni_immagine',
	NS_MEDIAWIKI        => 'MediaWiki',
	NS_MEDIAWIKI_TALK   => 'Discussioni_MediaWiki',
	NS_TEMPLATE         => 'Template',
	NS_TEMPLATE_TALK    => 'Discussioni_template',
	NS_HELP             => 'Aiuto',
	NS_HELP_TALK        => 'Discussioni_aiuto',
	NS_CATEGORY         => 'Categoria',
	NS_CATEGORY_TALK    => 'Discussioni_categoria'
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

$specialPageAliases = array(
	'DoubleRedirects'           => array( 'RedirectDoppi' ),
	'BrokenRedirects'           => array( 'RedirectErrati' ),
	'Disambiguations'           => array( 'Disambigue' ),
	'Userlogin'                 => array( 'Entra', 'Login' ),
	'Userlogout'                => array( 'Esci', 'Logout' ),
	'Preferences'               => array( 'Preferenze' ),
	'Watchlist'                 => array( 'OsservatiSpeciali' ),
	'Recentchanges'             => array( 'UltimeModifiche' ),
	'Upload'                    => array( 'Carica' ),
	'Imagelist'                 => array( 'Immagini' ),
	'Newimages'                 => array( 'ImmaginiRecenti' ),
	'Listusers'                 => array( 'Utenti', 'ElencoUtenti' ),
	'Statistics'                => array( 'Statistiche' ),
	'Randompage'                => array( 'PaginaCasuale' ),
	'Lonelypages'               => array( 'PagineOrfane' ),
	'Uncategorizedpages'        => array( 'PagineSenzaCategorie' ),
	'Uncategorizedcategories'   => array( 'CategorieSenzaCategorie' ),
	'Uncategorizedimages'       => array( 'ImmaginiSenzaCategorie' ),
	'Unusedcategories'          => array( 'CategorieNonUsate' ),
	'Unusedimages'              => array( 'ImmaginiNonUsate' ),
	'Wantedpages'               => array( 'PagineRichieste' ),
	'Wantedcategories'          => array( 'CategorieRichieste' ),
	'Mostlinked'                => array( 'PaginePiùRichiamate' ),
	'Mostlinkedcategories'      => array( 'CategoriePiùRichiamate' ),
	'Mostcategories'            => array( 'PagineConPiùCategorie'),
	'Mostimages'                => array( 'ImmaginiPiùRichiamate' ),
	'Mostrevisions'             => array( 'PagineConPiùRevisioni' ),
	'Shortpages'                => array( 'PaginePiùCorte' ),
	'Longpages'                 => array( 'PaginePiùLunghe' ),
	'Newpages'                  => array( 'PaginePiùRecenti' ),
	'Ancientpages'              => array( 'PagineMenoRecenti' ),
	'Deadendpages'              => array( 'PagineSenzaUscita' ),
	'Allpages'                  => array( 'TutteLePagine' ),
	'Prefixindex'               => array( 'Prefissi' ) ,
	'Ipblocklist'               => array( 'IPBloccati' ),
	'Specialpages'              => array( 'PagineSpeciali' ),
	'Contributions'             => array( 'Contributi', 'ContributiUtente' ),
	'Emailuser'                 => array( 'InviaEMail' ),
	'Whatlinkshere'             => array( 'PuntanoQui' ),
	'Recentchangeslinked'       => array( 'ModificheCorrelate' ),
	'Movepage'                  => array( 'Sposta', 'Rinomina' ),
	'Blockme'                   => array( 'BloccaProxy' ),
	'Booksources'               => array( 'RicercaISBN' ),
	'Categories'                => array( 'Categorie' ),
	'Export'                    => array( 'Esporta' ),
	'Version'                   => array( 'Versione' ),
	'Allmessages'               => array( 'Messaggi' ),
	'Log'                       => array( 'Registri', 'Registro' ),
	'Blockip'                   => array( 'Blocca' ),
	'Undelete'                  => array( 'Ripristina' ),
	'Import'                    => array( 'Importa' ),
	'Lockdb'                    => array( 'BloccaDB' ),
	'Unlockdb'                  => array( 'SbloccaDB' ),
	'Userrights'                => array( 'PermessiUtente' ),
	'MIMEsearch'                => array( 'RicercaMIME' ),
	'Unwatchedpages'            => array( 'PagineNonOsservate' ),
	'Listredirects'             => array( 'Redirect' ),
	'Listinterwikis'            => array( 'Interwiki' ),
	'Revisiondelete'            => array( 'CancellaRevisione' ),
	'Unusedtemplates'           => array( 'TemplateNonUsati' ),
	'Randomredirect'            => array( 'RedirectCasuale' ),
	'Mypage'                    => array( 'MiaPaginaUtente' ),
	'Mytalk'                    => array( 'MieDiscussioni' ),
	'Mycontributions'           => array( 'MieiContributi' ),
	'Listadmins'                => array( 'Amministratori' ),
	'Popularpages'              => array( 'PaginePiùVisitate' ),
	'Search'                    => array( 'Ricerca', 'Cerca' ),
	'Resetpass'                 => array( 'ReimpostaPassword' ),
);

$linkTrail = '/^([a-zàéèíîìóòúù]+)(.*)$/sDu';

$messages = array(
# User preference toggles
'tog-underline'               => 'Sottolinea i collegamenti',
'tog-highlightbroken'         => 'Evidenzia <a href="" class="new">così</a> i collegamenti a pagine inesistenti (se disattivato: così<a href="" class="internal">?</a>).',
'tog-justify'                 => 'Allineamento dei paragrafi giustificato',
'tog-hideminor'               => 'Nascondi le modifiche minori nelle ultime modifiche',
'tog-extendwatchlist'         => "Mostra tutte le modifiche agli osservati speciali, non solo l'ultima",
'tog-usenewrc'                => 'Ultime modifiche avanzate (richiede JavaScript)',
'tog-numberheadings'          => 'Numerazione automatica dei titoli di sezione',
'tog-showtoolbar'             => 'Mostra barra degli strumenti di modifica (richiede JavaScript)',
'tog-editondblclick'          => 'Modifica delle pagine tramite doppio clic (richiede JavaScript)',
'tog-editsection'             => 'Modifica delle sezioni tramite il collegamento [modifica]',
'tog-editsectiononrightclick' => 'Modifica delle sezioni tramite clic destro sul titolo (richiede JavaScript)',
'tog-showtoc'                 => "Mostra l'indice per le pagine con più di 3 sezioni",
'tog-rememberpassword'        => 'Ricorda la password su questo computer (richiede di accettare i cookie)',
'tog-editwidth'               => 'Aumenta al massimo la larghezza della casella di modifica',
'tog-watchcreations'          => 'Aggiungi le pagine create agli osservati speciali',
'tog-watchdefault'            => 'Aggiungi le pagine modificate agli osservati speciali',
'tog-watchmoves'              => 'Aggiungi le pagine spostate agli osservati speciali',
'tog-watchdeletion'           => 'Aggiungi le pagine cancellate agli osservati speciali',
'tog-minordefault'            => 'Indica ogni modifica come minore (solo come predefinito)',
'tog-previewontop'            => "Mostra l'anteprima sopra la casella di modifica e non sotto",
'tog-previewonfirst'          => "Mostra l'anteprima per la prima modifica",
'tog-nocache'                 => "Disattiva la ''cache'' per le pagine",
'tog-enotifwatchlistpages'    => 'Segnalami via e-mail le modifiche alle pagine osservate',
'tog-enotifusertalkpages'     => 'Segnalami via e-mail le modifiche alla mia pagina di discussione',
'tog-enotifminoredits'        => 'Segnalami via e-mail anche le modifiche minori',
'tog-enotifrevealaddr'        => 'Rivela il mio indirizzo e-mail nei messaggi di avviso',
'tog-shownumberswatching'     => 'Mostra il numero di utenti che hanno la pagina in osservazione',
'tog-fancysig'                => 'Non modificare il markup della firma (usare per firme non standard)',
'tog-externaleditor'          => 'Usa per default un editor di testi esterno (solo per utenti esperti, ha bisogno di impostazioni speciali sul tuo computer)',
'tog-externaldiff'            => 'Usa per default un programma di diff esterno (solo per utenti esperti, ha bisogno di impostazioni speciali sul tuo computer)',
'tog-showjumplinks'           => 'Attiva i collegamenti accessibili "vai a"',
'tog-uselivepreview'          => "Attiva la funzione ''Live preview'' (richiede JavaScript; sperimentale)",
'tog-forceeditsummary'        => "Chiedi conferma se l'oggetto della modifica è vuoto",
'tog-watchlisthideown'        => 'Nascondi le mie modifiche negli osservati speciali',
'tog-watchlisthidebots'       => 'Nascondi le modifiche dei bot negli osservati speciali',
'tog-watchlisthideminor'      => 'Nascondi le modifiche minori negli osservati speciali',
'tog-nolangconversion'        => 'Disattiva la conversione tra varianti linguistiche',
'tog-ccmeonemails'            => 'Inviami una copia dei messaggi spediti agli altri utenti',
'tog-diffonly'                => 'Non visualizzare il contenuto della pagina dopo il confronto tra versioni',
'tog-showhiddencats'          => 'Mostra categorie nascoste',

'underline-always'  => 'Sempre',
'underline-never'   => 'Mai',
'underline-default' => 'Mantieni le impostazioni del browser',

'skinpreview' => '(anteprima)',

# Dates
'sunday'        => 'domenica',
'monday'        => 'lunedì',
'tuesday'       => 'martedì',
'wednesday'     => 'mercoledì',
'thursday'      => 'giovedì',
'friday'        => 'venerdì',
'saturday'      => 'sabato',
'sun'           => 'dom',
'mon'           => 'lun',
'tue'           => 'mar',
'wed'           => 'mer',
'thu'           => 'gio',
'fri'           => 'ven',
'sat'           => 'sab',
'january'       => 'gennaio',
'february'      => 'febbraio',
'march'         => 'marzo',
'april'         => 'aprile',
'may_long'      => 'maggio',
'june'          => 'giugno',
'july'          => 'luglio',
'august'        => 'agosto',
'september'     => 'settembre',
'october'       => 'ottobre',
'november'      => 'novembre',
'december'      => 'dicembre',
'january-gen'   => 'gennaio',
'february-gen'  => 'febbraio',
'march-gen'     => 'marzo',
'april-gen'     => 'aprile',
'may-gen'       => 'maggio',
'june-gen'      => 'giugno',
'july-gen'      => 'luglio',
'august-gen'    => 'agosto',
'september-gen' => 'settembre',
'october-gen'   => 'ottobre',
'november-gen'  => 'novembre',
'december-gen'  => 'dicembre',
'jan'           => 'gen',
'feb'           => 'feb',
'mar'           => 'mar',
'apr'           => 'apr',
'may'           => 'mag',
'jun'           => 'giu',
'jul'           => 'lug',
'aug'           => 'ago',
'sep'           => 'set',
'oct'           => 'ott',
'nov'           => 'nov',
'dec'           => 'dic',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|Categoria|Categorie}}',
'category_header'                => 'Pagine nella categoria "$1"',
'subcategories'                  => 'Sottocategorie',
'category-media-header'          => 'File nella categoria "$1"',
'category-empty'                 => "''Al momento la categoria non contiene alcuna pagina né file multimediale.''",
'hidden-categories'              => '{{PLURAL:$1|Categoria nascosta|Categorie nascoste}}',
'hidden-category-category'       => 'Categorie nascoste', # Name of the category where hidden categories will be listed
'category-subcat-count'          => "{{PLURAL:$2|Questa categoria contiene un'unica sottocategoria, indicata di seguito.|Questa categoria contiene {{PLURAL:$1|la sottocategoria indicata|le $1 sottocategorie indicate}} di seguito, su un totale di $2.}}",
'category-subcat-count-limited'  => 'Questa categoria contiene {{PLURAL:$1|una sottocategoria, indicata|$1 sottocategorie, indicate}} di seguito.',
'category-article-count'         => "{{PLURAL:$2|Questa categoria contiene un'unica pagina, indicata di seguito.|Questa categoria contiene {{PLURAL:$1|la pagina indicata|le $1 pagine indicate}} di seguito, su un totale di $2.}}",
'category-article-count-limited' => 'Questa categoria contiene {{PLURAL:$1|la pagina indicata|le $1 pagine indicate}} di seguito.',
'category-file-count'            => '{{PLURAL:$2|Questa categoria contiene un solo file, indicato di seguito.|Questa categoria contiene {{PLURAL:$1|un file, indicato|$1 file, indicati}} di seguito, su un totale di $2.}}',
'category-file-count-limited'    => 'Questa categoria contiene {{PLURAL:$1|il file indicato|i $1 file indicati}} di seguito.',
'listingcontinuesabbrev'         => 'cont.',

'mainpagetext'      => "<big>'''Installazione di MediaWiki completata correttamente.'''</big>",
'mainpagedocfooter' => "Consultare la [http://meta.wikimedia.org/wiki/Aiuto:Sommario Guida utente] per maggiori informazioni sull'uso di questo software wiki.

== Per iniziare ==
I seguenti collegamenti sono in lingua inglese:

* [http://www.mediawiki.org/wiki/Manual:Configuration_settings Impostazioni di configurazione]
* [http://www.mediawiki.org/wiki/Manual:FAQ Domande frequenti su MediaWiki]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce Mailing list annunci MediaWiki]",

'about'          => 'Informazioni',
'article'        => 'Voce',
'newwindow'      => '(si apre in una nuova finestra)',
'cancel'         => 'Annulla',
'qbfind'         => 'Trova',
'qbbrowse'       => 'Sfoglia',
'qbedit'         => 'Modifica',
'qbpageoptions'  => 'Opzioni pagina',
'qbpageinfo'     => 'Informazioni sulla pagina',
'qbmyoptions'    => 'Le mie pagine',
'qbspecialpages' => 'Pagine speciali',
'moredotdotdot'  => 'Altro...',
'mypage'         => 'La mia pagina',
'mytalk'         => 'mie discussioni',
'anontalk'       => 'Discussioni per questo IP',
'navigation'     => 'Navigazione',
'and'            => 'e',

# Metadata in edit box
'metadata_help' => 'Metadati:',

'errorpagetitle'    => 'Errore',
'returnto'          => 'Torna a $1.',
'tagline'           => 'Da {{SITENAME}}.',
'help'              => 'Aiuto',
'search'            => 'Ricerca',
'searchbutton'      => 'Ricerca',
'go'                => 'Vai',
'searcharticle'     => 'Vai',
'history'           => 'Versioni precedenti',
'history_short'     => 'Cronologia',
'updatedmarker'     => 'modificata dalla mia ultima visita',
'info_short'        => 'Informazioni',
'printableversion'  => 'Versione stampabile',
'permalink'         => 'Link permanente',
'print'             => 'Stampa',
'edit'              => 'Modifica',
'create'            => 'Crea',
'editthispage'      => 'Modifica questa pagina',
'create-this-page'  => 'Crea questa pagina',
'delete'            => 'Cancella',
'deletethispage'    => 'Cancella questa pagina',
'undelete_short'    => 'Recupera {{PLURAL:$1|una revisione|$1 revisioni}}',
'protect'           => 'Proteggi',
'protect_change'    => 'cambia',
'protectthispage'   => 'Proteggi questa pagina',
'unprotect'         => 'Sblocca',
'unprotectthispage' => 'Togli la protezione a questa pagina',
'newpage'           => 'Nuova pagina',
'talkpage'          => 'Pagina di discussione',
'talkpagelinktext'  => 'discussione',
'specialpage'       => 'Pagina speciale',
'personaltools'     => 'Strumenti personali',
'postcomment'       => 'Aggiungi un commento',
'articlepage'       => 'Vedi la voce',
'talk'              => 'Discussione',
'views'             => 'Visite',
'toolbox'           => 'Strumenti',
'userpage'          => 'Visualizza la pagina utente',
'projectpage'       => 'Visualizza la pagina di servizio',
'imagepage'         => 'Visualizza la pagina del file',
'mediawikipage'     => 'Visualizza il messaggio',
'templatepage'      => 'Visualizza il template',
'viewhelppage'      => 'Visualizza la pagina di aiuto',
'categorypage'      => 'Visualizza la categoria',
'viewtalkpage'      => 'Visualizza la pagina di discussione',
'otherlanguages'    => 'Altre lingue',
'redirectedfrom'    => '(Reindirizzamento da <b>$1</b>)',
'redirectpagesub'   => 'Pagina di reindirizzamento',
'lastmodifiedat'    => 'Ultima modifica per la pagina: $2, $1.', # $1 date, $2 time
'viewcount'         => 'Questa pagina è stata letta {{PLURAL:$1|una volta|$1 volte}}.',
'protectedpage'     => 'Pagina bloccata',
'jumpto'            => 'Vai a:',
'jumptonavigation'  => 'navigazione',
'jumptosearch'      => 'ricerca',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'Informazioni su {{SITENAME}}',
'aboutpage'            => 'Project:Informazioni',
'bugreports'           => 'Malfunzionamenti',
'bugreportspage'       => 'Project:Malfunzionamenti',
'copyright'            => "Contenuti soggetti a licenza d'uso $1.",
'copyrightpagename'    => 'Il copyright su {{SITENAME}}',
'copyrightpage'        => '{{ns:project}}:Copyright',
'currentevents'        => 'Attualità',
'currentevents-url'    => 'Project:Attualità',
'disclaimers'          => 'Avvertenze',
'disclaimerpage'       => 'Project:Avvertenze generali',
'edithelp'             => 'Guida',
'edithelppage'         => 'Help:Modifica',
'faq'                  => 'Domande frequenti',
'faqpage'              => 'Project:Domande frequenti',
'helppage'             => 'Help:Indice',
'mainpage'             => 'Pagina principale',
'mainpage-description' => 'Pagina principale',
'policy-url'           => 'Project:Policy',
'portal'               => 'Portale comunità',
'portal-url'           => 'Project:Portale comunità',
'privacy'              => 'Informazioni sulla privacy',
'privacypage'          => 'Project:Informazioni sulla privacy',

'badaccess'        => 'Permessi non sufficienti',
'badaccess-group0' => "Non si dispone dei permessi necessari per eseguire l'azione richiesta.",
'badaccess-group1' => 'La funzione richiesta è riservata agli utenti che appartengono al gruppo $1.',
'badaccess-group2' => 'La funzione richiesta è riservata agli utenti che appartengono ai gruppi $1.',
'badaccess-groups' => 'La funzione richiesta è riservata agli utenti che appartengono a uno dei seguenti gruppi: $1.',

'versionrequired'     => 'Versione $1 di MediaWiki richiesta',
'versionrequiredtext' => "Per usare questa pagina è necessario disporre della versione $1 del software MediaWiki. Vedi [[Special:Version|l'apposita pagina]].",

'ok'                      => 'OK',
'retrievedfrom'           => 'Estratto da "$1"',
'youhavenewmessages'      => 'Hai $1 ($2).',
'newmessageslink'         => 'nuovi messaggi',
'newmessagesdifflink'     => 'differenza con la revisione precedente',
'youhavenewmessagesmulti' => 'Hai nuovi messaggi su $1',
'editsection'             => 'modifica',
'editold'                 => 'modifica',
'viewsourceold'           => 'visualizza sorgente',
'editsectionhint'         => 'Modifica la sezione $1',
'toc'                     => 'Indice',
'showtoc'                 => 'mostra',
'hidetoc'                 => 'nascondi',
'thisisdeleted'           => 'Vedi o ripristina $1?',
'viewdeleted'             => 'Vedi $1?',
'restorelink'             => '{{PLURAL:$1|una modifica cancellata|$1 modifiche cancellate}}',
'feedlinks'               => 'Feed:',
'feed-invalid'            => 'Modalità di sottoscrizione del feed non valida.',
'feed-unavailable'        => 'Non sono disponibili feed',
'site-rss-feed'           => 'Feed RSS di $1',
'site-atom-feed'          => 'Feed Atom di $1',
'page-rss-feed'           => 'Feed RSS per "$1"',
'page-atom-feed'          => 'Feed Atom per "$1"',
'red-link-title'          => '$1 (ancora da scrivere)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Voce',
'nstab-user'      => 'Utente',
'nstab-media'     => 'File multimediale',
'nstab-special'   => 'Speciale',
'nstab-project'   => 'pagina di servizio',
'nstab-image'     => 'File',
'nstab-mediawiki' => 'Messaggio',
'nstab-template'  => 'Template',
'nstab-help'      => 'Aiuto',
'nstab-category'  => 'Categoria',

# Main script and global functions
'nosuchaction'      => 'Operazione non riconosciuta',
'nosuchactiontext'  => 'La URL immessa non corrisponde a un comando riconosciuto dal software MediaWiki',
'nosuchspecialpage' => 'Pagina speciale non disponibile',
'nospecialpagetext' => "La pagina speciale richiesta non è stata riconosciuta dal software MediaWiki; l'elenco delle pagine speciali valide si trova in [[Special:SpecialPages|Elenco delle pagine speciali]].",

# General errors
'error'                => 'Errore',
'databaseerror'        => 'Errore del database',
'dberrortext'          => 'Errore di sintassi nella richiesta inoltrata al database.
Ciò potrebbe indicare la presenza di un bug nel software.
L\'ultima query inviata al database è stata:
<blockquote><tt>$1</tt></blockquote>
richiamata dalla funzione "<tt>$2</tt>".
MySQL ha restituito il seguente errore "<tt>$3: $4</tt>".',
'dberrortextcl'        => 'Errore di sintassi nella richiesta inoltrata al database.
L\'ultima query inviata al database è stata:
"$1"
richiamata dalla funzione "$2".
MySQL ha restituito il seguente errore "$3: $4".',
'noconnect'            => 'Connessione al database non riuscita a causa di un problema tecnico del sito.<br />$1',
'nodb'                 => 'Selezione del database $1 non riuscita',
'cachederror'          => "Quella presentata di seguito è una copia ''cache'' della pagina richiesta; potrebbe quindi non essere aggiornata.",
'laggedslavemode'      => 'Attenzione: la pagina potrebbe non contenere gli ultimi aggiornamenti.',
'readonly'             => 'Database bloccato',
'enterlockreason'      => 'Indica il motivo del blocco, specificando il momento in cui è presumibile che venga rimosso',
'readonlytext'         => "In questo momento il database è bloccato e non sono possibili aggiunte o modifiche alle pagine. Il blocco è di solito legato a operazioni di manutenzione ordinaria, al termine delle quali il database è di nuovo accessibile.

L'amministratore di sistema che ha imposto il blocco ha fornito questa spiegazione: $1",
'missing-article'      => 'Il database non ha trovato il testo di una pagina che avrebbe dovuto trovare sotto il nome di "$1" $2.

Di solito ciò si verifica quando viene richiamato, a partire dalla cronologia o dal confronto tra revisioni, un collegamento a una pagina cancellata, a un confronto tra revisioni inesistenti o a un confronto tra revisioni ripulite dalla cronologia.

In caso contrario, si è probabilmente scoperto un errore del software MediaWiki.
Si prega di segnalare l\'accaduto a un [[Special:ListUsers/sysop|amministratore]] specificando la URL in questione.',
'missingarticle-rev'   => '(numero della revisione: $1)',
'missingarticle-diff'  => '(Diff: $1, $2)',
'readonly_lag'         => 'Il database è stato bloccato automaticamente per consentire ai server con i database slave di sincronizzarsi con il master',
'internalerror'        => 'Errore interno',
'internalerror_info'   => 'Errore interno: $1',
'filecopyerror'        => 'Impossibile copiare il file "$1" in "$2".',
'filerenameerror'      => 'Impossibile rinominare il file "$1" in "$2".',
'filedeleteerror'      => 'Impossibile cancellare il file "$1".',
'directorycreateerror' => 'Impossibile creare la directory "$1".',
'filenotfound'         => 'File "$1" non trovato.',
'fileexistserror'      => 'Impossibile scrivere il file "$1": il file esiste già',
'unexpected'           => 'Valore imprevisto: "$1"="$2".',
'formerror'            => 'Errore: impossibile inviare il modulo',
'badarticleerror'      => 'Operazione non consentita per questa pagina.',
'cannotdelete'         => 'Impossibile cancellare la pagina o il file richiesto (potrebbe essere stato già cancellato).',
'badtitle'             => 'Titolo non corretto',
'badtitletext'         => 'Il titolo della pagina richiesta è vuoto, errato o con caratteri non ammessi oppure deriva da un errore nei collegamenti tra siti wiki diversi o versioni in lingue diverse dello stesso sito.',
'perfdisabled'         => 'Siamo spiacenti, questa funzionalità è temporaneamente disabilitata perché il suo uso rallenta il database fino a rendere il sito inutilizzabile per tutti gli utenti.',
'perfcached'           => "I dati che seguono sono estratti da una copia ''cache'' del database, non aggiornati in tempo reale.",
'perfcachedts'         => "I dati che seguono sono estratti da una copia ''cache'' del database. Ultimo aggiornamento: $1.",
'querypage-no-updates' => 'Gli aggiornamenti della pagina sono temporaneamente sospesi. I dati in essa contenuti non verranno aggiornati.',
'wrong_wfQuery_params' => 'Errore nei parametri inviati alla funzione wfQuery()<br />
Funzione: $1<br />
Query: $2',
'viewsource'           => 'Visualizza sorgente',
'viewsourcefor'        => 'di $1',
'actionthrottled'      => 'Azione ritardata',
'actionthrottledtext'  => "Come misura di sicurezza contro lo spam, l'esecuzione di alcune azioni è limitata a un numero massimo di volte in un determinato periodo di tempo, limite che in questo caso è stato superato. Si prega di riprovare tra qualche minuto.",
'protectedpagetext'    => 'Questa pagina è stata protetta per impedirne la modifica.',
'viewsourcetext'       => 'È possibile visualizzare e copiare il codice sorgente di questa pagina:',
'protectedinterface'   => "Questa pagina contiene un elemento che fa parte dell'interfaccia utente del software; è quindi protetta per evitare possibili abusi.",
'editinginterface'     => "'''Attenzione:''' Il testo di questa pagina fa parte dell'interfaccia utente del sito. Tutte le modifiche apportate a questa pagina si riflettono sui messaggi visualizzati per tutti gli utenti.
Per le traduzioni, considera la possibilità di usare [http://translatewiki.net/wiki/Main_Page?setlang=it Betawiki], il progetto MediaWiki per la localizzazione.",
'sqlhidden'            => '(la query SQL è stata nascosta)',
'cascadeprotected'     => 'Su questa pagina non è possibile effettuare modifiche perché è stata inclusa {{PLURAL:$1|nella pagina indicata di seguito, che è stata protetta|nelle pagine indicate di seguito, che sono state protette}} selezionando la protezione "ricorsiva":
$2',
'namespaceprotected'   => "Non si dispone dei permessi necessari per modificare le pagine del namespace '''$1'''.",
'customcssjsprotected' => 'Non si dispone dei permessi necessari alla modifica della pagina, in quanto contiene le impostazioni personali di un altro utente.',
'ns-specialprotected'  => 'Non è possibile modificare le pagine speciali.',
'titleprotected'       => "La creazione di una pagina con questo titolo è stata bloccata da [[User:$1|$1]].
La motivazione è la seguente: ''$2''.",

# Virus scanner
'virus-badscanner'     => 'Errore di configurazione: antivirus sconosciuto: <i>$1</i>',
'virus-scanfailed'     => 'scansione fallita (codice $1)',
'virus-unknownscanner' => 'antivirus sconosciuto:',

# Login and logout pages
'logouttitle'                => 'Logout utente',
'logouttext'                 => '<strong>Logout effettuato.</strong><br />
Si può continuare ad usare {{SITENAME}} come utente anonimo oppure eseguire un nuovo accesso, con lo stesso nome utente o un nome diverso.

Alcune pagine potrebbero continuare ad apparire come se il logout non fosse avvenuto finché non viene pulita la cache del proprio browser.',
'welcomecreation'            => "== Benvenuto, $1! ==

L'account è stato creato correttamente. Non dimenticare di personalizzare le preferenze di {{SITENAME}}.",
'loginpagetitle'             => 'Login utente',
'yourname'                   => 'Nome utente',
'yourpassword'               => 'Password:',
'yourpasswordagain'          => 'Ripeti la password',
'remembermypassword'         => 'Ricorda la password su questo computer',
'yourdomainname'             => 'Specificare il dominio',
'externaldberror'            => 'Si è verificato un errore con il server di autenticazione esterno, oppure non si dispone delle autorizzazioni necessarie per aggiornare il proprio accesso esterno.',
'loginproblem'               => "<b>Si è verificato un errore durante l'accesso.</b><br />Riprovare.",
'login'                      => 'Entra',
'nav-login-createaccount'    => 'Entra / Registrati',
'loginprompt'                => 'Per accedere a {{SITENAME}} è necessario abilitare i cookie.',
'userlogin'                  => 'Entra / Registrati',
'logout'                     => 'Esci',
'userlogout'                 => 'esci',
'notloggedin'                => 'Accesso non effettuato',
'nologin'                    => 'Non hai ancora un accesso? $1.',
'nologinlink'                => 'Crealo ora',
'createaccount'              => 'Crea un nuovo accesso',
'gotaccount'                 => 'Hai già un accesso? $1.',
'gotaccountlink'             => 'Entra',
'createaccountmail'          => 'via e-mail',
'badretype'                  => 'Le password inserite non coincidono tra loro.',
'userexists'                 => 'Il nome utente inserito è già utilizzato. Si scelga un nome utente diverso.',
'youremail'                  => 'Indirizzo e-mail:',
'username'                   => 'Nome utente:',
'uid'                        => 'ID utente:',
'prefs-memberingroups'       => 'Membro {{PLURAL:$1|del gruppo|dei gruppi}}:',
'yourrealname'               => 'Nome vero:',
'yourlanguage'               => "Lingua dell'interfaccia:",
'yourvariant'                => 'Variante:',
'yournick'                   => 'Soprannome (nickname):',
'badsig'                     => 'Errore nella firma non standard, verificare i tag HTML.',
'badsiglength'               => 'Il soprannome scelto è troppo lungo, non deve superare $1 {{PLURAL:$1|carattere|caratteri}}.',
'email'                      => 'Indirizzo e-mail',
'prefs-help-realname'        => "L'indicazione del proprio nome vero è opzionale; se si sceglie di inserirlo, verrà utilizzato per attribuire la paternità dei contenuti inviati.",
'loginerror'                 => "Errore nell'accesso",
'prefs-help-email'           => "L'inserimento del proprio indirizzo e-mail è opzionale ma permette di ricevere la propria password via e-mail qualora venisse dimenticata. È inoltre possibile permettere di essere contattati dagli altri utenti attraverso un link nella propria pagina utente o nella relativa pagina di discussione, senza dover rivelare la propria identità.",
'prefs-help-email-required'  => 'Indirizzo e-mail necessario.',
'nocookiesnew'               => "La registrazione è stata completata, ma non è stato possibile accedere a {{SITENAME}} perché i cookie sono disattivati. Riprovare l'accesso con il nome utente e la password appena creati dopo aver attivato i cookie nel proprio browser.",
'nocookieslogin'             => "L'accesso a {{SITENAME}} richiede l'uso dei cookie, che risultano disattivati. Riprovare l'accesso dopo aver attivato i cookie nel proprio browser.",
'noname'                     => 'Il nome utente indicato non è valido, non è possibile creare un accesso a questo nome.',
'loginsuccesstitle'          => 'Accesso effettuato',
'loginsuccess'               => "'''Sei stato connesso al server di {{SITENAME}} con il nome utente di \"\$1\".'''",
'nosuchuser'                 => 'Non è registrato alcun utente di nome "$1". Verificare il nome inserito o [[Special:Userlogin/signup|creare un nuovo accesso]].',
'nosuchusershort'            => 'Non è registrato alcun utente di nome "<nowiki>$1</nowiki>". Verificare il nome inserito.',
'nouserspecified'            => 'È necessario specificare un nome utente.',
'wrongpassword'              => 'La password inserita non è corretta. Riprovare.',
'wrongpasswordempty'         => 'Non è stata inserita alcuna password. Riprovare.',
'passwordtooshort'           => 'La password inserita non è valida o è troppo breve. 
Deve contenere almeno {{PLURAL:$1|1 carattere|$1 caratteri}} ed essere diversa dal tuo nome utente.',
'mailmypassword'             => 'Invia una nuova password al mio indirizzo e-mail',
'passwordremindertitle'      => 'Servizio Password Reminder di {{SITENAME}}',
'passwordremindertext'       => 'Qualcuno (probabilmente tu, con indirizzo IP $1) ha richiesto l\'invio di una nuova password di accesso a {{SITENAME}} ($4).
Una password temporanea per l\'utente "$2" è stata impostata a "$3".
È opportuno eseguire un accesso quanto prima e cambiare la password immediatamente.

Se non sei stato tu a fare la richiesta, oppure hai ritrovato la password e non desideri più cambiarla, puoi ignorare questo messaggio e continuare a usare la vecchia password.',
'noemail'                    => 'Nessun indirizzo e-mail registrato per l\'utente "$1".',
'passwordsent'               => 'Una nuova password è stata inviata all\'indirizzo e-mail registrato per l\'utente "$1".
Per favore, effettua un accesso non appena la ricevi.',
'blocked-mailpassword'       => 'Per prevenire abusi, non è consentito usare la funzione "Invia nuova password" da un indirizzo IP bloccato.',
'eauthentsent'               => "Un messaggio e-mail di conferma è stato spedito all'indirizzo indicato.
Per abilitare l'invio di messaggi e-mail per questo accesso è necessario seguire le istruzioni che vi sono indicate, in modo da confermare che si è i legittimi proprietari dell'indirizzo",
'throttled-mailpassword'     => 'Una nuova password è già stata inviata da meno di {{PLURAL:$1|1 ora|$1 ore}}.
Per prevenire abusi, la funzione "Invia nuova password" può essere usata solo una volta ogni {{PLURAL:$1|ora|$1 ore}}.',
'mailerror'                  => "Errore nell'invio del messaggio: $1",
'acct_creation_throttle_hit' => 'Spiacente, hai già creato $1 account. Non puoi crearne altri.',
'emailauthenticated'         => "L'indirizzo e-mail è stato confermato il $1.",
'emailnotauthenticated'      => "L'indirizzo e-mail non è stato ancora confermato. Non verranno inviati messaggi e-mail attraverso le funzioni elencate di seguito.",
'noemailprefs'               => 'Indicare un indirizzo e-mail per attivare queste funzioni.',
'emailconfirmlink'           => 'Confermare il proprio indirizzo e-mail',
'invalidemailaddress'        => "L'indirizzo e-mail indicato ha un formato non valido. Inserire un indirizzo valido o svuotare la casella.",
'accountcreated'             => 'Accesso creato',
'accountcreatedtext'         => "È stato creato un accesso per l'utente $1.",
'createaccount-title'        => 'Creazione di un accesso a {{SITENAME}}',
'createaccount-text'         => 'Qualcuno ha creato un accesso a {{SITENAME}} ($4) a nome di $2, associato a questo indirizzo di posta elettronica. La password per l\'utente "$2" è impostata a "$3".
È opportuno eseguire un accesso quanto prima e cambiare la password immediatamente.

Se l\'accesso è stato creato per errore, si può ignorare questo messaggio.',
'loginlanguagelabel'         => 'Lingua: $1',

# Password reset dialog
'resetpass'               => 'Reimposta la password',
'resetpass_announce'      => "L'accesso è stato effettuato con un codice temporaneo, inviato via e-mail. Per completare l'accesso è necessario impostare una nuova password:",
'resetpass_text'          => '<!-- Aggiungere il testo qui -->',
'resetpass_header'        => 'Reimposta password',
'resetpass_submit'        => 'Imposta la password e accedi al sito',
'resetpass_success'       => 'La password è stata modificata. Accesso in corso...',
'resetpass_bad_temporary' => 'Password temporanea non valida. La password potrebbe essere stata già cambiata, oppure potrebbe essere stata richiesta una nuova password temporanea.',
'resetpass_forbidden'     => 'Non è possibile modificare le password',
'resetpass_missing'       => 'Dati mancanti nel modulo.',

# Edit page toolbar
'bold_sample'     => 'Grassetto',
'bold_tip'        => 'Grassetto',
'italic_sample'   => 'Corsivo',
'italic_tip'      => 'Corsivo',
'link_sample'     => 'Titolo del collegamento',
'link_tip'        => 'Collegamento interno',
'extlink_sample'  => 'http://www.example.com titolo del collegamento',
'extlink_tip'     => 'Collegamento esterno (ricorda il prefisso http:// )',
'headline_sample' => 'Intestazione',
'headline_tip'    => 'Intestazione di 2° livello',
'math_sample'     => 'Inserire qui la formula',
'math_tip'        => 'Formula matematica (LaTeX)',
'nowiki_sample'   => 'Inserire qui il testo non formattato',
'nowiki_tip'      => 'Ignora la formattazione wiki',
'image_sample'    => 'Esempio.jpg',
'image_tip'       => 'File incorporato',
'media_sample'    => 'Esempio.ogg',
'media_tip'       => 'Collegamento a file multimediale',
'sig_tip'         => 'Firma con data e ora',
'hr_tip'          => 'Linea orizzontale (usare con giudizio)',

# Edit pages
'summary'                          => 'Oggetto',
'subject'                          => 'Argomento (intestazione)',
'minoredit'                        => 'Questa è una modifica minore',
'watchthis'                        => 'Aggiungi agli osservati speciali',
'savearticle'                      => 'Salva la pagina',
'preview'                          => 'Anteprima',
'showpreview'                      => 'Visualizza anteprima',
'showlivepreview'                  => "Funzione ''Live preview''",
'showdiff'                         => 'Mostra cambiamenti',
'anoneditwarning'                  => "'''Attenzione:''' Accesso non effettuato. Nella cronologia della pagina verrà registrato l'indirizzo IP.",
'missingsummary'                   => "'''Attenzione:''' non è stato specificato l'oggetto di questa modifica. Premendo di nuovo '''Salva la pagina''' la modifica verrà salvata con l'oggetto vuoto.",
'missingcommenttext'               => 'Inserire un commento qui sotto.',
'missingcommentheader'             => "'''Attenzione:''' Non è stata specificata l'intestazione di questo commento. Premendo di nuovo '''Salva la pagina''' la modifica verrà salvata senza intestazione.",
'summary-preview'                  => 'Anteprima oggetto',
'subject-preview'                  => 'Anteprima oggetto/intestazione',
'blockedtitle'                     => 'Utente bloccato.',
'blockedtext'                      => "<big>'''Questo nome utente o indirizzo IP sono stati bloccati.'''</big>

Il blocco è stato imposto da $1. La motivazione del blocco è la seguente: ''$2''

* Inizio del blocco: $8
* Scadenza del blocco: $6
* Intervallo di blocco: $7

Se lo si desidera, è possibile contattare $1 o un altro [[{{MediaWiki:Grouppage-sysop}}|amministratore]] per discutere del blocco.

Si noti che la funzione 'Scrivi all'utente' non è attiva se non è stato registrato un indirizzo e-mail valido nelle proprie [[Special:Preferences|preferenze]] e se si è stato bloccati dal suo utilizzo.

L'indirizzo IP attuale è $3, il numero ID del blocco è #$5.
Si prega di specificare tutti i dettagli precedenti in qualsiasi richiesta di chiarimenti.",
'autoblockedtext'                  => "Questo indirizzo IP è stato bloccato automaticamente perché condiviso con un altro utente, a sua volta bloccato da $1.
La motivazione del blocco è la seguente:

:''$2''

* Inizio del blocco: $8
* Scadenza del blocco: $6
* Intervallo di blocco: $7

È possibile contattare $1 o un altro [[{{MediaWiki:Grouppage-sysop}}|amministratore]] per discutere del blocco.

Si noti che la funzione 'Scrivi all'utente' non è attiva se non è stato registrato un indirizzo e-mail valido nelle proprie [[Special:Preferences|preferenze]] e se si è stato bloccati dal suo utilizzo.

L'indirizzo IP attuale è $3, il numero ID del blocco è #$5
Si prega di specificare tutti i dettagli precedenti in qualsiasi richiesta di chiarimenti.",
'blockednoreason'                  => 'nessuna motivazione indicata',
'blockedoriginalsource'            => "Di seguito viene mostrato il codice sorgente della pagina '''$1''':",
'blockededitsource'                => "Di seguito vengono mostrate le '''modifiche apportate''' alla pagina '''$1''':",
'whitelistedittitle'               => 'Accesso necessario per la modifica delle pagine',
'whitelistedittext'                => 'Per modificare le pagine è necessario $1.',
'confirmedittitle'                 => 'Conferma della e-mail necessaria per la modifica delle pagine',
'confirmedittext'                  => "Per essere abilitati alla modifica delle pagine è necessario confermare il proprio indirizzo e-mail. Per impostare e confermare l'indirizzo servirsi delle [[Special:Preferences|preferenze]].",
'nosuchsectiontitle'               => 'La sezione non esiste',
'nosuchsectiontext'                => 'Si è tentato di modificare una sezione inesistente. Non è possibile salvare le modifiche in quanto la sezione $1 non esiste.',
'loginreqtitle'                    => "Per modificare questa pagina è necessario eseguire l'accesso al sito.",
'loginreqlink'                     => "eseguire l'accesso",
'loginreqpagetext'                 => 'Per vedere altre pagine è necessario $1.',
'accmailtitle'                     => 'Password inviata.',
'accmailtext'                      => 'La password per l\'utente "$1" è stata inviata all\'indirizzo $2.',
'newarticle'                       => '(Nuovo)',
'newarticletext'                   => "Il collegamento appena seguito corrisponde a una pagina non ancora esistente.
Se si desidera creare la pagina ora, basta cominciare a scrivere il testo nella casella qui sotto
(fare riferimento alle [[{{MediaWiki:Helppage}}|pagine di aiuto]] per maggiori informazioni).
Se il collegamento è stato seguito per errore, è sufficiente fare clic sul pulsante '''Indietro''' del proprio browser.",
'anontalkpagetext'                 => "----''Questa è la pagina di discussione di un utente anonimo, che non ha ancora creato un accesso o comunque non lo usa. Per identificarlo è quindi necessario usare il numero del suo indirizzo IP. Gli indirizzi IP possono però essere condivisi da più utenti. Se sei un utente anonimo e ritieni che i commenti presenti in questa pagina non si riferiscano a te, [[Special:UserLogin|crea un nuovo accesso o entra]] con quello che già hai per evitare di essere confuso con altri utenti anonimi in futuro''",
'noarticletext'                    => 'In questo momento la pagina richiesta è vuota. È possibile [[Special:Search/{{PAGENAME}}|cercare questo titolo]] nelle altre pagine del sito oppure [{{fullurl:{{FULLPAGENAME}}|action=edit}} modificare la pagina ora].',
'userpage-userdoesnotexist'        => 'L\'account "$1" non corrisponde a un utente registrato. Verificare che si intenda davvero creare o modificare questa pagina.',
'clearyourcache'                   => "'''Nota: dopo aver salvato è necessario pulire la cache del proprio browser per vedere i cambiamenti.''' Per '''Mozilla / Firefox / Safari''': fare clic su ''Ricarica'' tenendo premuto il tasto delle maiuscole, oppure premere ''Ctrl-F5'' o ''Ctrl-R'' (''Command-R'' su Mac); per '''Konqueror''': premere il pulsante ''Ricarica'' o il tasto ''F5''; per '''Opera''' può essere necessario svuotare completamente la cache dal menu ''Strumenti → Preferenze''; per '''Internet Explorer:''' mantenere premuto il tasto ''Ctrl'' mentre si preme il pulsante ''Aggiorna'' o premere ''Ctrl-F5''.",
'usercssjsyoucanpreview'           => "<strong>Suggerimento:</strong> si consiglia di usare il pulsante 'Visualizza anteprima' per provare i nuovi CSS o JavaScript prima di salvarli.",
'usercsspreview'                   => "'''Questa è solo un'anteprima del proprio CSS personale. Le modifiche non sono ancora state salvate!'''",
'userjspreview'                    => "'''Questa è solo un'anteprima per provare il proprio JavaScript personale; le modifiche non sono ancora state salvate!'''",
'userinvalidcssjstitle'            => "'''Attenzione:'''  Non esiste alcuna skin con nome \"\$1\". Si noti che le pagine per i .css e .js personalizzati hanno l'iniziale del titolo minuscola, ad esempio {{ns:user}}:Esempio/monobook.css e non {{ns:user}}:Esempio/Monobook.css.",
'updated'                          => '(Aggiornato)',
'note'                             => '<strong>NOTA:</strong>',
'previewnote'                      => '<strong>Questa è solo una anteprima; le modifiche alla pagina NON sono ancora state salvate!</strong>',
'previewconflict'                  => 'L\'anteprima corrisponde al testo presente nella casella di modifica superiore e rappresenta la pagina come apparirà se si sceglie di premere "Salva la pagina" in questo momento.',
'session_fail_preview'             => '<strong>Non è stato possibile elaborare la modifica perché sono andati persi i dati relativi alla sessione. Se il problema persiste, si può provare a [[Special:UserLogout|scollegarsi]] ed effettuare un nuovo accesso.</strong>',
'session_fail_preview_html'        => "<strong>Non è stato possibile elaborare la modifica perché sono andati persi i dati relativi alla sessione.</strong>

''Poiché in {{SITENAME}} è abilitato l'uso di HTML senza limitazioni, l'anteprima non viene visualizzata; si tratta di una misura di sicurezza contro gli attacchi JavaScript.''

<strong>Se questo è un legittimo tentativo di modifica, riprovare. Se il problema persiste, si può provare a [[Special:UserLogout|scollegarsi]] ed effettuare un nuovo accesso.</strong>",
'token_suffix_mismatch'            => "<strong>La modifica non è stata salvata perché il client ha mostrato di gestire in modo errato i caratteri di punteggiatura nel token associato alla stessa. Per evitare una possibile corruzione del testo della pagina, è stata rifiutata l'intera modifica. Questa situazione può verificarsi, talvolta, quando vengono usati alcuni servizi di proxy anonimi via web che presentano dei bug.</strong>",
'editing'                          => 'Modifica di $1',
'editingsection'                   => 'Modifica di $1 (sezione)',
'editingcomment'                   => 'Modifica di $1 (commento)',
'editconflict'                     => 'Conflitto di edizione su $1',
'explainconflict'                  => "Un altro utente ha salvato una nuova versione della pagina mentre stavi effettuando le modifiche.<br />
La casella di modifica superiore contiene il testo della pagina attualmente online, così come è stato aggiornato dall'altro utente. La versione con le tue modifiche è invece riportata nella casella di modifica inferiore. Se desideri confermarle, devi riportare le tue modifiche nel testo esistente (casella superiore).
Premendo il pulsante 'Salva la pagina', verrà salvato <b>solo</b> il testo contenuto nella casella di modifica superiore.<br />",
'yourtext'                         => 'Il tuo testo',
'storedversion'                    => 'La versione memorizzata',
'nonunicodebrowser'                => '<strong>Attenzione: si sta utilizzando un browser non compatibile con i caratteri Unicode. Per consentire la modifica delle pagine senza creare inconvenienti, i caratteri non ASCII vengono visualizzati nella casella di modifica sotto forma di codici esadecimali.</strong>',
'editingold'                       => '<strong>Attenzione: si sta modificando una versione non aggiornata della pagina.<br />
Se si sceglie di salvarla, tutti i cambiamenti apportati dopo questa revisione andranno perduti.</strong>',
'yourdiff'                         => 'Differenze',
'copyrightwarning'                 => "Nota: tutti i contributi a {{SITENAME}} si considerano rilasciati nei termini della licenza d'uso $2 (vedi $1 per maggiori dettagli). Se non desideri che i tuoi testi possano essere modificati e ridistribuiti da chiunque senza alcuna limitazione, non inviarli a {{SITENAME}}.<br />
Con l'invio del testo dichiari inoltre, sotto la tua responsabilità, che il testo è stato scritto da te personalmente oppure che è stato copiato da una fonte di pubblico dominio o analogamente libera.
<strong>NON INVIARE MATERIALE COPERTO DA DIRITTO DI AUTORE SENZA AUTORIZZAZIONE!</strong>",
'copyrightwarning2'                => "Nota: tutti i contributi inviati a {{SITENAME}} possono essere modificati, stravolti o cancellati da parte degli altri partecipanti. Se non desideri che i tuoi testi possano essere modificati senza alcun riguardo, non inviarli a questo sito.<br />
Con l'invio del testo dichiari inoltre, sotto la tua responsabilità, che il testo è stato scritto da te personalmente oppure che è stato copiato da una fonte di pubblico dominio o analogamente libera. (vedi $1 per maggiori dettagli)
<strong>NON INVIARE MATERIALE COPERTO DA DIRITTO DI AUTORE SENZA AUTORIZZAZIONE!</strong>",
'longpagewarning'                  => "<strong>ATTENZIONE: Questa pagina è lunga $1 kilobyte; alcuni browser potrebbero presentare dei problemi nella modifica di pagine che si avvicinano o superano i 32 KB. Valuta l'opportunità di suddividere la pagina in sezioni più piccole.</strong>",
'longpageerror'                    => '<strong>ERRORE: Il testo inviato è lungo $1 kilobyte, più della dimensione massima consentita ($2 kilobyte). Il testo non può essere salvato.</strong>',
'readonlywarning'                  => '<strong>ATTENZIONE: Il database è stato bloccato per manutenzione, è quindi impossibile salvare le modifiche in questo momento. Per non perderle, è possibile copiare quanto inserito finora nella casella di modifica, incollarlo in un programma di elaborazione testi e salvarlo in attesa dello sblocco del database.</strong>',
'protectedpagewarning'             => '<strong>ATTENZIONE: Questa pagina è stata bloccata in modo che solo gli utenti con privilegi di amministratore possano modificarla.</strong>',
'semiprotectedpagewarning'         => "'''Nota:''' Questa pagina è stata bloccata in modo che solo gli utenti registrati possano modificarla.",
'cascadeprotectedwarning'          => "'''Attenzione:''' Questa pagina è stata bloccata in modo che solo gli utenti con privilegi di amministratore possano modificarla. Ciò avviene perché la pagina è inclusa {{PLURAL:\$1|nella pagina indicata di seguito, che è stata protetta|nelle pagine indicate di seguito, che sono state protette}} selezionando la protezione \"ricorsiva\":",
'titleprotectedwarning'            => '<strong>ATTENZIONE:  Questa pagina è stata bloccata in modo che solo alcune categorie di utenti possano crearla.</strong>',
'templatesused'                    => 'Template utilizzati in questa pagina:',
'templatesusedpreview'             => 'Template utilizzati in questa anteprima:',
'templatesusedsection'             => 'Template utilizzati in questa sezione:',
'template-protected'               => '(protetto)',
'template-semiprotected'           => '(semiprotetto)',
'hiddencategories'                 => 'Questa pagina appartiene a {{PLURAL:$1|una categoria nascosta|$1 categorie nascoste}}:',
'edittools'                        => '<!-- Testo che appare al di sotto del modulo di modifica e di upload. -->',
'nocreatetitle'                    => 'Creazione delle pagine limitata',
'nocreatetext'                     => 'La possibilità di creare nuove pagine su {{SITENAME}} è stata limitata ai soli utenti registrati. È possibile tornare indietro e modificare una pagina esistente, oppure [[Special:UserLogin|entrare o registrarsi]].',
'nocreate-loggedin'                => 'Non si dispone dei permessi necessari a creare nuove pagine.',
'permissionserrors'                => 'Errore nei permessi',
'permissionserrorstext'            => "Non si dispone dei permessi necessari ad eseguire l'azione richiesta, per {{PLURAL:$1|il seguente motivo|i seguenti motivi}}:",
'permissionserrorstext-withaction' => 'Non hai il permesso di fare $2, per {{PLURAL:$1|il seguente motivo|i seguenti motivi}}:',
'recreate-deleted-warn'            => "'''Attenzione: si sta per ricreare una pagina già cancellata in passato.'''

Accertarsi che sia davvero opportuno continuare a modificare questa pagina.
L'elenco delle relative cancellazioni viene riportato di seguito per comodità:",

# Parser/template warnings
'expensive-parserfunction-warning'        => 'Attenzione: Questa pagina contiene troppe chiamate alle parser functions.

Dovrebbe averne meno di $2, al momento ce ne sono $1.',
'expensive-parserfunction-category'       => 'Pagine con troppe chiamate alle parser functions',
'post-expand-template-inclusion-warning'  => 'Attenzione: la dimensione dei template inclusi è troppo grande.
Alcuni template non verranno inclusi.',
'post-expand-template-inclusion-category' => 'Pagine dove la dimensione dei template inclusi supera il limite consentito',
'post-expand-template-argument-warning'   => 'Attenzione: questa pagina contiene almeno un argomento di un template che ha una dimensione troppo grande per essere espanso. Questi argomenti verranno omessi.',
'post-expand-template-argument-category'  => 'Pagine contenenti template con argomenti mancanti',

# "Undo" feature
'undo-success' => 'Questa modifica può essere annullata. Verificare il confronto presentato di seguito per accertarsi che il contenuto corrisponda a quanto desiderato e quindi salvare le modifiche per completare la procedura di annullamento.',
'undo-failure' => 'Impossibile annullare la modifica a causa di un conflitto con modifiche intermedie.',
'undo-norev'   => 'La modifica non può essere annullata perché non esiste o è stata cancellata.',
'undo-summary' => 'Annullata la modifica $1 di [[Special:Contributions/$2|$2]] ([[User talk:$2|discussione]])',

# Account creation failure
'cantcreateaccounttitle' => 'Impossibile registrare un utente',
'cantcreateaccount-text' => "La creazione di nuovi account a partire da questo indirizzo IP ('''$1''') è stata bloccata da [[User:$3|$3]].

La motivazione del blocco fornita da $3 è la seguente: ''$2''",

# History pages
'viewpagelogs'        => 'Visualizza i log relativi a questa pagina.',
'nohistory'           => 'Cronologia delle versioni di questa pagina non reperibile.',
'revnotfound'         => 'Versione non trovata',
'revnotfoundtext'     => 'La versione richiesta della pagina non è stata trovata.
Verificare la URL usata per accedere a questa pagina.',
'currentrev'          => 'Versione corrente',
'revisionasof'        => 'Versione del $1',
'revision-info'       => 'Versione del $1, autore: $2',
'previousrevision'    => '← Versione meno recente',
'nextrevision'        => 'Versione più recente →',
'currentrevisionlink' => 'Versione corrente',
'cur'                 => 'corr',
'next'                => 'succ',
'last'                => 'prec',
'page_first'          => 'prima',
'page_last'           => 'ultima',
'histlegend'          => "Confronto tra versioni: selezionare le caselle corrispondenti alle versioni desiderate e premere Invio o il pulsante in basso.

Legenda: (corr) = differenze con la versione corrente, (prec) = differenze con la versione precedente, '''m''' = modifica minore",
'deletedrev'          => '[cancellata]',
'histfirst'           => 'Prima',
'histlast'            => 'Ultima',
'historysize'         => '({{PLURAL:$1|1 byte|$1 byte}})',
'historyempty'        => '(vuota)',

# Revision feed
'history-feed-title'          => 'Cronologia',
'history-feed-description'    => 'Cronologia della pagina su questo sito',
'history-feed-item-nocomment' => '$1 il $2', # user at time
'history-feed-empty'          => 'La pagina richiesta non esiste; potrebbe essere stata cancellata dal sito o rinominata. Verificare con la [[Special:Search|pagina di ricerca]] se vi sono nuove pagine.',

# Revision deletion
'rev-deleted-comment'         => '(commento rimosso)',
'rev-deleted-user'            => '(nome utente rimosso)',
'rev-deleted-event'           => '(azione del log rimossa)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">
Questa versione della pagina è stata rimossa dagli archivi visibili al pubblico.
Consultare il [{{fullurl:Special:Log/delete|page={{PAGENAMEE}}}} log di cancellazione] per ulteriori dettagli.
</div>',
'rev-deleted-text-view'       => '<div class="mw-warning plainlinks">
Questa versione della pagina è stata rimossa dagli archivi visibili al pubblico.
Il testo può essere visualizzato soltanto dagli amministratori del sito.
Consultare il [{{fullurl:Special:Log/delete|page={{PAGENAMEE}}}} log di cancellazione] per ulteriori dettagli.
</div>',
'rev-delundel'                => 'mostra/nascondi',
'revisiondelete'              => 'Cancella o ripristina versioni',
'revdelete-nooldid-title'     => 'Versione non specificata',
'revdelete-nooldid-text'      => 'Non è stata specificata alcuna versione della pagina su cui eseguire questa funzione.',
'revdelete-selected'          => '{{PLURAL:$2|Versione selezionata|Versioni selezionate}} di [[:$1]]:',
'logdelete-selected'          => '{{PLURAL:$1|Evento del registro selezionato|Eventi del registro selezionati}}:',
'revdelete-text'              => 'Le versioni cancellate restano visibili nella cronologia della pagina, mentre il testo contenuto non è accessibile al pubblico.

Gli altri amministratori del sito potranno accedere comunque ai contenuti nascosti e ripristinarli attraverso questa stessa interfaccia, se non sono state impostate altre limitazioni in fase di installazione del sito.',
'revdelete-legend'            => 'Imposta le seguenti limitazioni sulle versioni cancellate:',
'revdelete-hide-text'         => 'Nascondi il testo della versione',
'revdelete-hide-name'         => 'Nascondi azione e oggetto della stessa',
'revdelete-hide-comment'      => "Nascondi l'oggetto della modifica",
'revdelete-hide-user'         => "Nascondi il nome o l'indirizzo IP dell'autore",
'revdelete-hide-restricted'   => 'Applica le limitazioni indicate anche agli amministratori',
'revdelete-suppress'          => 'Nascondi le informazioni anche agli amministratori',
'revdelete-hide-image'        => 'Nascondi i contenuti del file',
'revdelete-unsuppress'        => 'Elimina le limitazioni sulle revisioni ripristinate',
'revdelete-log'               => 'Commento per il log:',
'revdelete-submit'            => 'Applica alla revisione selezionata',
'revdelete-logentry'          => 'ha modificato la visibilità per una revisione di [[$1]]',
'logdelete-logentry'          => "ha modificato la visibilità dell'evento [[$1]]",
'revdelete-success'           => "'''Visibilità della revisione impostata correttamente.'''",
'logdelete-success'           => "'''Visibilità dell'evento impostata correttamente.'''",
'revdel-restore'              => 'Cambia la visibilità',
'pagehist'                    => 'Cronologia della pagina',
'deletedhist'                 => 'Cronologia cancellata',
'revdelete-content'           => 'contenuto',
'revdelete-summary'           => 'riassunto della modifica',
'revdelete-uname'             => 'nome utente',
'revdelete-restricted'        => 'limitazioni ai soli amministratori attivate',
'revdelete-unrestricted'      => 'limitazioni ai soli amministratori rimosse',
'revdelete-hid'               => 'nascondi $1',
'revdelete-unhid'             => 'rendi visibile $1',
'revdelete-log-message'       => '$1 per $2 {{PLURAL:$2|revisione|revisioni}}',
'logdelete-log-message'       => '$1 per $2 {{PLURAL:$2|evento|eventi}}',

# Suppression log
'suppressionlog'     => 'Log delle soppressioni',
'suppressionlogtext' => "Di seguito sono elencate le cancellazioni e i blocchi più recenti riguardanti contenuti nascosti agli amministratori. Vedi l'[[Special:IPBlockList|elenco degli IP bloccati]] per l'elenco dei blocchi attivi al momento.",

# History merging
'mergehistory'                     => 'Unione cronologie',
'mergehistory-header'              => 'Questa pagina consente di unire le revisioni appartenenti alla cronologia di una pagina (detta pagina di origine) alla cronologia di una pagina più recente.
È necessario accertarsi che la continuità storica della pagina non venga alterata.',
'mergehistory-box'                 => 'Unisci la cronologia di due pagine:',
'mergehistory-from'                => 'Pagina di origine:',
'mergehistory-into'                => 'Pagina di destinazione:',
'mergehistory-list'                => "Cronologia cui è applicabile l'unione",
'mergehistory-merge'               => 'È possibile unire le revisioni di [[:$1]] indicate di seguito alla cronologia di [[:$2]]. Usare la colonna con i pulsanti di opzione per unire tutte le revisioni fino alla data e ora indicate. Si noti che se vengono usati i pulsanti di navigazione, la colonna con i pulsanti di opzione viene azzerata.',
'mergehistory-go'                  => 'Mostra le modifiche che possono essere unite',
'mergehistory-submit'              => 'Unisci le revisioni',
'mergehistory-empty'               => 'Nessuna revisione da unire.',
'mergehistory-success'             => '{{PLURAL:$3|Una revisione di [[:$1]] è stata unita|$3 revisioni di [[:$1]] sono state unite}} alla cronologia di [[:$2]].',
'mergehistory-fail'                => 'Impossibile unire le cronologie. Verificare la pagina e i parametri temporali.',
'mergehistory-no-source'           => 'La pagina di origine $1 non esiste.',
'mergehistory-no-destination'      => 'La pagina di destinazione $1 non esiste.',
'mergehistory-invalid-source'      => 'La pagina di origine deve avere un titolo corretto.',
'mergehistory-invalid-destination' => 'La pagina di destinazione deve avere un titolo corretto.',
'mergehistory-autocomment'         => 'Unione di [[:$1]] in [[:$2]]',
'mergehistory-comment'             => 'Unione di [[:$1]] in [[:$2]]: $3',

# Merge log
'mergelog'           => 'Unioni',
'pagemerge-logentry' => 'ha unito [[$1]] a [[$2]] (revisioni fino a $3)',
'revertmerge'        => 'Annulla unioni',
'mergelogpagetext'   => 'Di seguito sono elencate le ultime operazioni di unione della cronologia di due pagine.',

# Diffs
'history-title'           => 'Cronologia delle modifiche di "$1"',
'difference'              => '(Differenze fra le revisioni)',
'lineno'                  => 'Riga $1:',
'compareselectedversions' => 'Confronta le versioni selezionate',
'editundo'                => 'annulla',
'diff-multi'              => '({{PLURAL:$1|Una revisione intermedia non mostrata|$1 revisioni intermedie non mostrate}}.)',

# Search results
'searchresults'             => 'Risultati della ricerca',
'searchresulttext'          => 'Per maggiori informazioni sulla ricerca interna di {{SITENAME}}, vedi [[{{MediaWiki:Helppage}}|{{int:help}}]].',
'searchsubtitle'            => "Ricerca di '''[[:$1]]'''",
'searchsubtitleinvalid'     => "Ricerca di '''$1'''",
'noexactmatch'              => "'''La pagina \"\$1\" non esiste.''' È possibile [[:\$1|crearla ora]].",
'noexactmatch-nocreate'     => "'''La pagina con titolo \"\$1\" non esiste.'''",
'toomanymatches'            => 'Troppe corrispondenze. Modificare la richiesta.',
'titlematches'              => 'Corrispondenze nel titolo delle pagine',
'notitlematches'            => 'Nessuna corrispondenza nei titoli delle pagine',
'textmatches'               => 'Corrispondenze nel testo delle pagine',
'notextmatches'             => 'Nessuna corrispondenza nel testo delle pagine',
'prevn'                     => 'precedenti $1',
'nextn'                     => 'successivi $1',
'viewprevnext'              => 'Vedi ($1) ($2) ($3).',
'search-result-size'        => '$1 ({{PLURAL:$2|una parola|$2 parole}})',
'search-result-score'       => 'Rilevanza: $1%',
'search-redirect'           => '(redirect $1)',
'search-section'            => '(sezione $1)',
'search-suggest'            => 'Forse cercavi: $1',
'search-interwiki-caption'  => 'Progetti fratelli',
'search-interwiki-default'  => 'Risultati da $1:',
'search-interwiki-more'     => '(altro)',
'search-mwsuggest-enabled'  => 'con suggerimenti',
'search-mwsuggest-disabled' => 'senza suggerimenti',
'search-relatedarticle'     => 'Risultati correlati',
'mwsuggest-disable'         => 'Disattiva suggerimenti AJAX',
'searchrelated'             => 'correlati',
'searchall'                 => 'tutti',
'showingresults'            => "Di seguito {{PLURAL:$1|viene presentato al massimo '''1''' risultato|vengono presentati al massimo '''$1''' risultati}} a partire dal numero '''$2'''.",
'showingresultsnum'         => "Di seguito {{PLURAL:$3|viene presentato '''1''' risultato|vengono presentati '''$3''' risultati}} a partire dal numero '''$2'''.",
'showingresultstotal'       => "Di seguito {{PLURAL:$3|viene mostrato il risultato '''$1''' di '''$3'''|vengono mostrati i risultati '''$1 - $2''' di '''$3'''}}",
'nonefound'                 => "'''Nota''': la ricerca è effettuata per default solo in alcuni namespace. Prova a premettere ''all:'' al testo della ricerca per cercare in tutti i namespace (compresi pagine di discussione, template, ecc) oppure usa il namespace desiderato come prefisso.",
'powersearch'               => 'Ricerca',
'powersearch-legend'        => 'Ricerca avanzata',
'powersearch-ns'            => 'Cerca nei namespace:',
'powersearch-redir'         => 'Elenca redirect',
'powersearch-field'         => 'Cerca',
'search-external'           => 'Ricerca esterna',
'searchdisabled'            => 'La ricerca interna di {{SITENAME}} non è attiva; nel frattempo si può provare ad usare un motore di ricerca esterno come Google. (Si noti però che i contenuti di {{SITENAME}} presenti in tali motori potrebbero non essere aggiornati.)',

# Preferences page
'preferences'              => 'Preferenze',
'mypreferences'            => 'preferenze',
'prefs-edits'              => 'Modifiche effettuate:',
'prefsnologin'             => 'Accesso non effettuato',
'prefsnologintext'         => 'Per poter personalizzare le preferenze è necessario effettuare l\'<span class="plainlinks">[{{fullurl:Special:Userlogin|returnto=$1}} accesso]</span>.',
'prefsreset'               => 'Le preferenze sono state ripristinate ai valori predefiniti.',
'qbsettings'               => 'Quickbar',
'qbsettings-none'          => 'Nessuno',
'qbsettings-fixedleft'     => 'Fisso a sinistra',
'qbsettings-fixedright'    => 'Fisso a destra',
'qbsettings-floatingleft'  => 'Fluttuante a sinistra',
'qbsettings-floatingright' => 'Fluttuante a destra',
'changepassword'           => 'Cambia password',
'skin'                     => 'Aspetto grafico (skin)',
'math'                     => 'Formule matematiche',
'dateformat'               => 'Formato della data',
'datedefault'              => 'Nessuna preferenza',
'datetime'                 => 'Data e ora',
'math_failure'             => 'Errore del parser',
'math_unknown_error'       => 'errore sconosciuto',
'math_unknown_function'    => 'funzione sconosciuta',
'math_lexing_error'        => 'errore lessicale',
'math_syntax_error'        => 'errore di sintassi',
'math_image_error'         => 'Conversione in PNG non riuscita; verificare che siano correttamente installati i seguenti programmi: latex, dvips, gs e convert.',
'math_bad_tmpdir'          => 'Impossibile scrivere o creare la directory temporanea per math',
'math_bad_output'          => 'Impossibile scrivere o creare la directory di output per math',
'math_notexvc'             => 'Eseguibile texvc mancante; per favore consultare math/README per la configurazione.',
'prefs-personal'           => 'Profilo utente',
'prefs-rc'                 => 'Ultime modifiche',
'prefs-watchlist'          => 'Osservati speciali',
'prefs-watchlist-days'     => 'Numero di giorni da mostrare negli osservati speciali:',
'prefs-watchlist-edits'    => 'Numero di modifiche da mostrare con le funzioni avanzate:',
'prefs-misc'               => 'Varie',
'saveprefs'                => 'Salva le preferenze',
'resetprefs'               => 'Reimposta le preferenze',
'oldpassword'              => 'Vecchia password:',
'newpassword'              => 'Nuova password:',
'retypenew'                => 'Riscrivi la nuova password:',
'textboxsize'              => 'Casella di modifica',
'rows'                     => 'Righe:',
'columns'                  => 'Colonne:',
'searchresultshead'        => 'Ricerca',
'resultsperpage'           => 'Numero di risultati per pagina:',
'contextlines'             => 'Righe di testo per ciascun risultato:',
'contextchars'             => 'Numero di caratteri di contesto:',
'stub-threshold'           => 'Valore minimo per i <a href="#" class="stub">collegamenti agli stub</a>:',
'recentchangesdays'        => 'Numero di giorni da mostrare nelle ultime modifiche:',
'recentchangescount'       => 'Numero di righe nelle ultime modifiche:',
'savedprefs'               => 'Le preferenze sono state salvate.',
'timezonelegend'           => 'Fuso orario',
'timezonetext'             => "Numero di ore di differenza fra l'ora locale e l'ora del server (UTC).",
'localtime'                => 'Ora locale',
'timezoneoffset'           => 'Differenza¹',
'servertime'               => 'Ora del server',
'guesstimezone'            => "Usa l'ora del tuo browser",
'allowemail'               => 'Abilita la ricezione di email da altri utenti¹',
'prefs-searchoptions'      => 'Opzioni di ricerca',
'prefs-namespaces'         => 'Namespace',
'defaultns'                => 'Cerca in questi namespace se non diversamente specificato:',
'default'                  => 'predefinito',
'files'                    => 'File',

# User rights
'userrights'                  => 'Gestione dei permessi relativi agli utenti', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => 'Gestione dei gruppi utente',
'userrights-user-editname'    => 'Inserire il nome utente:',
'editusergroup'               => 'Modifica gruppi utente',
'editinguser'                 => "Modifica dei diritti assegnati all'utente '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup'    => 'Modifica gruppi utente',
'saveusergroups'              => 'Salva gruppi utente',
'userrights-groupsmember'     => 'Appartiene ai gruppi:',
'userrights-groups-help'      => "È possibile modificare i gruppi cui è assegnato l'utente.
* Una casella di spunta selezionata indica l'appartenenza dell'utente al gruppo
* Una casella di spunta deselezionata indica la sua mancata appartenenza al gruppo.
* Il simbolo * indica che non è possibile eliminare l'appartenenza al gruppo dopo averla aggiunta (o vice versa).",
'userrights-reason'           => 'Motivo della modifica:',
'userrights-no-interwiki'     => 'Non si dispone dei permessi necessari per modificare i diritti degli utenti su altri siti.',
'userrights-nodatabase'       => 'Il database $1 non esiste o non è un database locale.',
'userrights-nologin'          => "Per assegnare diritti agli utenti è necessario [[Special:UserLogin|effettuare l'accesso]] come amministratore.",
'userrights-notallowed'       => "L'utente non dispone dei permessi necessari per assegnare diritti agli utenti.",
'userrights-changeable-col'   => 'Gruppi modificabili',
'userrights-unchangeable-col' => 'Gruppi non modificabili',

# Groups
'group'               => 'Gruppo:',
'group-user'          => 'Utenti registrati',
'group-autoconfirmed' => 'Utenti autoconvalidati',
'group-bot'           => 'Bot',
'group-sysop'         => 'Amministratori',
'group-bureaucrat'    => 'Burocrati',
'group-suppress'      => 'Oversight',
'group-all'           => 'Utenti',

'group-user-member'          => 'Utente',
'group-autoconfirmed-member' => 'Utente autoconvalidato',
'group-bot-member'           => 'Bot',
'group-sysop-member'         => 'Amministratore',
'group-bureaucrat-member'    => 'Burocrate',
'group-suppress-member'      => 'Oversight',

'grouppage-user'          => '{{ns:project}}:Utenti',
'grouppage-autoconfirmed' => '{{ns:project}}:Utenti autoconvalidati',
'grouppage-bot'           => '{{ns:project}}:Bot',
'grouppage-sysop'         => '{{ns:project}}:Amministratori',
'grouppage-bureaucrat'    => '{{ns:project}}:Burocrati',
'grouppage-suppress'      => '{{ns:project}}:Oversight',

# Rights
'right-read'                 => 'Legge pagine',
'right-edit'                 => 'Modifica pagine',
'right-createpage'           => 'Crea pagine',
'right-createtalk'           => 'Crea pagine di discussione',
'right-createaccount'        => 'Crea nuovi account utente',
'right-minoredit'            => 'Segna le modifiche come minori',
'right-move'                 => 'Sposta pagine',
'right-move-subpages'        => 'Sposta le pagine insieme alle relative sottopagine',
'right-suppressredirect'     => 'Evita la creazione automatica del redirect quando sposta una pagina da quel titolo',
'right-upload'               => 'Carica file',
'right-reupload'             => 'Sovrascrive un file esistente',
'right-reupload-own'         => 'Sovrascrive un file esistente caricato dallo stesso utente',
'right-reupload-shared'      => "Sovrascrive localmente file presenti nell'archivio condiviso",
'right-upload_by_url'        => 'Carica un file da un indirizzo URL',
'right-purge'                => 'Purga la cache del sito senza conferma',
'right-autoconfirmed'        => 'Modifica le pagine semiprotette',
'right-bot'                  => 'Da trattare come processo automatico',
'right-nominornewtalk'       => "Fa sì che le modifiche minori alle pagine di discussione non facciano comparire l'avviso di nuovo messaggio",
'right-apihighlimits'        => 'Usa limiti più alti per le interrogazioni API',
'right-writeapi'             => "Usa l'API per modificare il wiki",
'right-delete'               => 'Cancella pagine',
'right-bigdelete'            => 'Cancella pagine con cronologie lunghe',
'right-deleterevision'       => 'Nasconde revisioni specifiche delle pagine',
'right-deletedhistory'       => 'Visualizza le revisioni della cronologia cancellate senza il testo associato',
'right-browsearchive'        => 'Visualizza pagine cancellate',
'right-undelete'             => 'Recupera una pagina',
'right-suppressrevision'     => 'Rivede e recupera revisioni nascoste dagli Amministratori',
'right-suppressionlog'       => 'Visualizza log privati',
'right-block'                => 'Blocca le modifiche da parte di altri utenti',
'right-blockemail'           => 'Impedisce a un utente di inviare email',
'right-hideuser'             => 'Blocca un nome utente, nascondendolo al pubblico',
'right-ipblock-exempt'       => 'Ignora i blocchi degli IP, i blocchi automatici e i blocchi di range di IP',
'right-proxyunbannable'      => 'Scavalca i blocchi sui proxy',
'right-protect'              => 'Cambia i livelli di protezione',
'right-editprotected'        => 'Modifica pagine protette',
'right-editinterface'        => "Modifica l'interfaccia utente",
'right-editusercssjs'        => 'Modifica i file CSS e JS di altri utenti',
'right-rollback'             => "Rollback rapido delle modifiche dell'ultimo utente che ha modificato una particolare pagina",
'right-markbotedits'         => 'Segna modifiche specifiche come bot',
'right-noratelimit'          => 'Non soggetto al limite di azioni',
'right-import'               => 'Importa pagine da altri wiki',
'right-importupload'         => 'Importa pagine da un upload di file',
'right-patrol'               => 'Segna le modifiche degli altri utenti come verificate',
'right-autopatrol'           => 'Segna automaticamente le sue modifiche come verificate',
'right-patrolmarks'          => 'Usa la funzione di verifica delle ultime modifiche',
'right-unwatchedpages'       => 'Visualizza una lista di pagine non osservate',
'right-trackback'            => 'Invia un trackback',
'right-mergehistory'         => 'Fonde la cronologia delle pagine',
'right-userrights'           => "Modifica tutti i diritti dell'utente",
'right-userrights-interwiki' => 'Modifica i diritti degli utenti di altre wiki',
'right-siteadmin'            => 'Blocca e sblocca il database',

# User rights log
'rightslog'      => 'Diritti degli utenti',
'rightslogtext'  => 'Di seguito sono elencate le modifiche ai diritti assegnati agli utenti.',
'rightslogentry' => "ha modificato l'appartenenza di $1 dal gruppo $2 al gruppo $3",
'rightsnone'     => '(nessuno)',

# Recent changes
'nchanges'                          => '$1 {{PLURAL:$1|modifica|modifiche}}',
'recentchanges'                     => 'Ultime modifiche',
'recentchangestext'                 => 'Questa pagina presenta le modifiche più recenti ai contenuti del sito.',
'recentchanges-feed-description'    => 'Questo feed riporta le modifiche più recenti ai contenuti del sito.',
'rcnote'                            => "Di seguito {{PLURAL:$1|è elencata la modifica più recente apportata|sono elencate le '''$1''' modifiche più recenti apportate}} al sito {{PLURAL:$2|nelle ultime 24 ore|negli scorsi '''$2''' giorni}}; i dati sono aggiornati alle $5 del $4.",
'rcnotefrom'                        => 'Di seguito sono elencate le modifiche apportate a partire da <b>$2</b> (fino a <b>$1</b>).',
'rclistfrom'                        => 'Mostra le modifiche apportate a partire da $1',
'rcshowhideminor'                   => '$1 le modifiche minori',
'rcshowhidebots'                    => '$1 i bot',
'rcshowhideliu'                     => '$1 gli utenti registrati',
'rcshowhideanons'                   => '$1 gli utenti anonimi',
'rcshowhidepatr'                    => '$1 le modifiche controllate',
'rcshowhidemine'                    => '$1 le mie modifiche',
'rclinks'                           => 'Mostra le $1 modifiche più recenti apportate negli ultimi $2 giorni<br />$3',
'diff'                              => 'diff',
'hist'                              => 'cron',
'hide'                              => 'nascondi',
'show'                              => 'mostra',
'minoreditletter'                   => 'm',
'newpageletter'                     => 'N',
'boteditletter'                     => 'b',
'number_of_watching_users_pageview' => '[osservata da {{PLURAL:$1|un utente|$1 utenti}}]',
'rc_categories'                     => 'Limita alle categorie (separate da "|")',
'rc_categories_any'                 => 'Qualsiasi',
'newsectionsummary'                 => '/* $1 */ nuova sezione',

# Recent changes linked
'recentchangeslinked'          => 'Modifiche correlate',
'recentchangeslinked-title'    => 'Modifiche correlate a "$1"',
'recentchangeslinked-noresult' => 'Nessuna modifica alle pagine collegate nel periodo specificato.',
'recentchangeslinked-summary'  => "Questa pagina speciale mostra le modifiche più recenti alle pagine collegate a quella specificata. Le pagine osservate sono evidenziate in '''grassetto'''.",
'recentchangeslinked-page'     => 'Nome della pagina:',
'recentchangeslinked-to'       => 'Mostra solo le modifiche alle pagine collegate a quella specificata',

# Upload
'upload'                      => 'Carica un file',
'uploadbtn'                   => 'Carica',
'reupload'                    => 'Carica di nuovo',
'reuploaddesc'                => 'Torna al modulo per il caricamento.',
'uploadnologin'               => 'Accesso non effettuato',
'uploadnologintext'           => "Il caricamento dei file è consentito solo agli utenti registrati che hanno eseguito [[Special:UserLogin|l'accesso]] al sito.",
'upload_directory_missing'    => 'La directory di upload ($1) non esiste e non può essere creata dal webserver.',
'upload_directory_read_only'  => 'Il server web non è in grado di scrivere nella directory di upload ($1).',
'uploaderror'                 => 'Errore nel caricamento',
'uploadtext'                  => "Usare il modulo sottostante per caricare nuovi file. Per visualizzare o ricercare i file già caricati, consultare il [[Special:ImageList|log dei file caricati]]. Caricamenti di file e di nuove versioni di file sono registrati nel [[Special:Log/upload|log degli upload]], le cancellazioni nell'[[Special:Log/delete|apposito]].

Per inserire un file all'interno di una pagina, fare un collegamento di questo tipo:
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:File.jpg]]</nowiki></tt>''' per usare la versione intera del file
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:File.png|200px|thumb|left|testo alternativo]]</nowiki></tt>''' per usare una versione larga 200 pixel inserita in un box, allineata a sinistra e con 'testo alternativo' come didascalia
* '''<tt><nowiki>[[</nowiki>{{ns:media}}<nowiki>:File.ogg]]</nowiki></tt>''' per generare un collegamento diretto al file senza visualizzarlo",
'upload-permitted'            => 'Tipi di file consentiti: $1.',
'upload-preferred'            => 'Tipi di file consigliati: $1.',
'upload-prohibited'           => 'Tipi di file non consentiti: $1.',
'uploadlog'                   => 'File caricati',
'uploadlogpage'               => 'File caricati',
'uploadlogpagetext'           => "Di seguito sono elencati gli ultimi file caricati.
Guarda la [[Special:NewImages|galleria dei nuovi file]] per una visione d'insieme",
'filename'                    => 'Nome del file',
'filedesc'                    => 'Dettagli',
'fileuploadsummary'           => 'Dettagli del file:',
'filestatus'                  => 'Informazioni sul copyright:',
'filesource'                  => 'Fonte:',
'uploadedfiles'               => 'Elenco dei file caricati',
'ignorewarning'               => "Ignora l'avviso e salva comunque il file. La versione esistente verrà sovrascritta.",
'ignorewarnings'              => 'Ignora i messaggi di avvertimento del sistema',
'minlength1'                  => "Il nome del file dev'essere composto da almeno un carattere.",
'illegalfilename'             => 'Il nome "$1" contiene dei caratteri non ammessi nei titoli delle pagine. Dare al file un nome diverso e provare a caricarlo di nuovo.',
'badfilename'                 => 'Il nome del file è stato convertito in "$1".',
'filetype-badmime'            => 'Non è consentito caricare file di tipo MIME "$1".',
'filetype-unwanted-type'      => "Caricare file di tipo '''\".\$1\"''' è sconsigliato. {{PLURAL:\$3|Il tipo di file consigliato è|I tipi di file consigliati sono}} \$2.",
'filetype-banned-type'        => "Caricare file di tipo '''\".\$1\"''' non è consentito. {{PLURAL:\$3|Il tipo di file consentito è|I tipi di file consentiti sono}} \$2.",
'filetype-missing'            => 'Il file è privo di estensione (ad es. ".jpg").',
'large-file'                  => 'Si raccomanda di non superare le dimensioni di $1 per ciascun file; questo file è grande $2.',
'largefileserver'             => 'Il file supera le dimensioni consentite dalla configurazione del server.',
'emptyfile'                   => 'Il file appena caricato sembra essere vuoto. Ciò potrebbe essere dovuto ad un errore nel nome del file. Verificare che si intenda realmente caricare questo file.',
'fileexists'                  => 'Un file con questo nome esiste già. Verificare prima <strong><tt>$1</tt></strong> se non si è sicuri di volerlo sovrascrivere.',
'filepageexists'              => "La pagina di descrizione di questo file è già stata creata all'indirizzo <strong><tt>$1</tt></strong>, anche se non esiste ancora un file con questo nome. La descrizione dell'oggetto inserita in fase di caricamento non apparirà sulla pagina di discussione. Per far sì che l'oggetto compaia sulla pagina di discussione, sarà necessario modificarla manualmente",
'fileexists-extension'        => "Un file con nome simile a questo esiste già; l'unica differenza è l'uso delle maiuscole nell'estensione:<br />
Nome del file caricato: <strong><tt>$1</tt></strong><br />
Nome del file esistente: <strong><tt>$2</tt></strong><br />
Verificare che i due file non siano identici.",
'fileexists-thumb'            => "<center>'''File preesistente'''</center>",
'fileexists-thumbnail-yes'    => "Il file caricato sembra essere il risultato di un'anteprima <i>(thumbnail)</i>. Verificare, per confronto, il file <strong><tt>$1</tt></strong>.<br />
Se si tratta della stessa immagine, nelle dimensioni originali, non è necessario caricarne altre anteprime.",
'file-thumbnail-no'           => "Il nome del file inizia con <strong><tt>$1</tt></strong>; sembra quindi essere il risultato di un'anteprima <i>(thumbnail)</i>.
Se si dispone dell'immagine nella risoluzione originale, si prega di caricarla. In caso contrario, si prega di cambiare il nome del file.",
'fileexists-forbidden'        => 'Un file con questo nome esiste già. Tornare indietro e modificare il nome con il quale caricare il file. [[Image:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => "Un file con questo nome esiste già nell'archivio di risorse multimediali condivise. Se si desidera ancora caricare il file, tornare indietro e modificare il nome con il quale caricare il file. [[Image:$1|thumb|center|$1]]",
'file-exists-duplicate'       => 'Questo file è un duplicato {{PLURAL:$1|del seguente|dei seguenti}} file:',
'successfulupload'            => 'Caricamento completato',
'uploadwarning'               => 'Avviso di caricamento',
'savefile'                    => 'Salva file',
'uploadedimage'               => 'ha caricato "[[$1]]"',
'overwroteimage'              => 'ha caricato una nuova versione di "[[$1]]"',
'uploaddisabled'              => 'Siamo spiacenti, ma il caricamento di file è temporaneamente sospeso.',
'uploaddisabledtext'          => 'Il caricamento dei file non è attivo.',
'uploadscripted'              => 'Questo file contiene codice HTML o di script, che potrebbe essere interpretato erroneamente da un browser web.',
'uploadcorrupt'               => "Il file è corrotto o ha un'estensione non corretta. Controllare il file e provare di nuovo il caricamento.",
'uploadvirus'                 => 'Questo file contiene un virus! Dettagli: $1',
'sourcefilename'              => 'Nome del file di origine:',
'destfilename'                => 'Nome del file di destinazione:',
'upload-maxfilesize'          => 'Dimensione massima del file: $1',
'watchthisupload'             => 'Aggiungi agli osservati speciali',
'filewasdeleted'              => 'Un file con questo nome è stato già caricato e cancellato in passato. Verificare il log delle $1 prima di caricarlo di nuovo.',
'upload-wasdeleted'           => "'''Attenzione: si sta per caricare un file già cancellato in passato.'''

Accertarsi che sia davvero opportuno continuare a caricare questo file.
L'elenco delle relative cancellazioni viene riportato di seguito per comodità:",
'filename-bad-prefix'         => 'Il nome del file che stai caricando inizia con <strong>"$1"</strong>, che è un nome non-descrittivo tipicamente assegnato automaticamente dalle fotocamere digitali. Per favore scegli un nome più descrittivo per il tuo file.',
'filename-prefix-blacklist'   => ' #<!-- lascia questa riga esattamente com\'è --> <pre>
# La sintassi è la seguente:
#   * Tutto ciò che segue il carattere "#" sino alla fine della riga è un commento
#   * Ogni riga non vuota è un prefisso per nomi di file tipici assegnati automaticamente da fotocamere digitali
CIMG # Casio
DSC_ # Nikon
DSCF # Fuji
DSCN # Nikon
DUW # alcuni telefonini
IMG # generic
JD # Jenoptik
MGP # Pentax
PICT # misc.
 #</pre> <!-- lascia questa riga esattamente com\'è -->',

'upload-proto-error'      => 'Protocollo errato',
'upload-proto-error-text' => "Per l'upload remoto è necessario specificare URL che iniziano con <code>http://</code> oppure <code>ftp://</code>.",
'upload-file-error'       => 'Errore interno',
'upload-file-error-text'  => 'Si è verificato un errore interno durante la creazione di un file temporaneo sul server. Contattare un amministratore di sistema.',
'upload-misc-error'       => "Errore non identificato per l'upload",
'upload-misc-error-text'  => 'Si è verificato un errore non identificato durante il caricamento del file. Verificare che la URL sia corretta e accessibile e provare di nuovo. Se il problema persiste, contattare un amministratore di sistema.',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => 'URL non raggiungibile',
'upload-curl-error6-text'  => 'Impossibile raggiungere la URL specificata. Verificare che la URL sia scritta correttamente e che il sito in questione sia attivo.',
'upload-curl-error28'      => "Tempo scaduto per l'upload",
'upload-curl-error28-text' => 'Il sito remoto ha impiegato troppo tempo a rispondere. Verificare che il sito sia attivo, attendere qualche minuto e provare di nuovo, eventualmente in un momento di minore traffico.',

'license'            => "Licenza d'uso:",
'nolicense'          => 'Nessuna licenza indicata',
'license-nopreview'  => '(Anteprima non disponibile)',
'upload_source_url'  => ' (una URL corretta e accessibile)',
'upload_source_file' => ' (un file sul proprio computer)',

# Special:ImageList
'imagelist-summary'     => "Questa pagina speciale mostra tutti i file caricati.
I file caricati più di recente vengono mostrati all'inizio della lista.
Per modificare l'ordinamento, fare clic sull'intestazione della colonna prescelta.",
'imagelist_search_for'  => 'Ricerca immagini per nome:',
'imgfile'               => 'file',
'imagelist'             => 'Elenco dei file',
'imagelist_date'        => 'Data',
'imagelist_name'        => 'Nome',
'imagelist_user'        => 'Utente',
'imagelist_size'        => 'Dimensione in byte',
'imagelist_description' => 'Descrizione',

# Image description page
'filehist'                       => 'Cronologia del file',
'filehist-help'                  => 'Fare clic su un gruppo data/ora per vedere il file come si presentava nel momento indicato.',
'filehist-deleteall'             => 'cancella tutto',
'filehist-deleteone'             => 'cancella',
'filehist-revert'                => 'ripristina',
'filehist-current'               => 'corrente',
'filehist-datetime'              => 'Data/Ora',
'filehist-user'                  => 'Utente',
'filehist-dimensions'            => 'Dimensioni',
'filehist-filesize'              => 'Dimensione del file',
'filehist-comment'               => 'Oggetto',
'imagelinks'                     => "Collegamenti all'immagine",
'linkstoimage'                   => "{{PLURAL:$1|La seguente pagina contiene|Le seguenti $1 pagine contengono}} collegamenti all'immagine:",
'nolinkstoimage'                 => "Nessuna pagina contiene collegamenti all'immagine.",
'morelinkstoimage'               => 'Visualizza [[Special:WhatLinksHere/$1|altri link]] a questo file.',
'redirectstofile'                => '{{PLURAL:$1|Il seguente|I seguenti $1}} file {{PLURAL:$1|è|sono}} un redirect a questo file:',
'duplicatesoffile'               => '{{PLURAL:$1|Il seguente|I seguenti $1}} file {{PLURAL:$1|è un duplicato|sono duplicati}} di questo file:',
'sharedupload'                   => 'Questo file è un upload condiviso; può essere quindi utilizzato da più progetti wiki.',
'shareduploadwiki'               => 'Si veda $1 per ulteriori informazioni.',
'shareduploadwiki-desc'          => 'La descrizione che appare in quella sede, sulla relativa $1, viene mostrata di seguito.',
'shareduploadwiki-linktext'      => 'pagina di descrizione del file',
'shareduploadduplicate'          => "Questo file è un duplicato di $1, presente nell'archivio condiviso.",
'shareduploadduplicate-linktext' => 'un altro file',
'shareduploadconflict'           => "Questo file ha lo stesso nome di $1, presente nell'archivio condiviso.",
'shareduploadconflict-linktext'  => 'un altro file',
'noimage'                        => 'Un file con questo nome non esiste ma è possibile $1.',
'noimage-linktext'               => 'caricarlo ora',
'uploadnewversion-linktext'      => 'Carica una nuova versione di questo file',
'imagepage-searchdupe'           => 'Ricerca dei file duplicati',

# File reversion
'filerevert'                => 'Ripristina $1',
'filerevert-legend'         => 'Ripristina file',
'filerevert-intro'          => "Si sta per ripristinare il file '''[[Media:$1|$1]]''' alla [$4 versione del $2, $3].",
'filerevert-comment'        => 'Oggetto:',
'filerevert-defaultcomment' => 'Ripristinata la versione del $2, $1',
'filerevert-submit'         => 'Ripristina',
'filerevert-success'        => "'''Il file [[Media:$1|$1]]''' è stato ripristinato alla [$4 versione del $2, $3].",
'filerevert-badversion'     => 'Non esistono versioni locali precedenti del file con il timestamp richiesto.',

# File deletion
'filedelete'                  => 'Cancella $1',
'filedelete-legend'           => 'Cancella il file',
'filedelete-intro'            => "Stai per cancellare '''[[Media:$1|$1]]'''.",
'filedelete-intro-old'        => "Stai cancellando la versione di '''[[Media:$1|$1]]''' del [$4 $2, $3].",
'filedelete-comment'          => 'Motivo della cancellazione:',
'filedelete-submit'           => 'Cancella',
'filedelete-success'          => "Il file '''$1''' è stato cancellato.",
'filedelete-success-old'      => "La versione del file '''[[Media:$1|$1]]''' del $2, $3  è stata cancellata.",
'filedelete-nofile'           => "Non esiste un file '''$1'''.",
'filedelete-nofile-old'       => "In archivio non ci sono versioni di '''$1''' con le caratteristiche indicate",
'filedelete-iscurrent'        => 'Stai provando a cancellare la versione più recente di questo file. Per cortesia, prima riportalo ad una versione precedente.',
'filedelete-otherreason'      => 'Altra motivazione o motivazione aggiuntiva:',
'filedelete-reason-otherlist' => 'Altra motivazione',
'filedelete-reason-dropdown'  => '*Motivazioni più comuni per la cancellazione
** Violazione di copyright
** File duplicato',
'filedelete-edit-reasonlist'  => 'Modifica le motivazioni per la cancellazione',

# MIME search
'mimesearch'         => 'Ricerca in base al tipo MIME',
'mimesearch-summary' => 'Questa pagina consente di filtrare i file in base al tipo MIME. Inserire la stringa di ricerca nella forma tipo/sottotipo, ad es. <tt>image/jpeg</tt>.',
'mimetype'           => 'Tipo MIME:',
'download'           => 'scarica',

# Unwatched pages
'unwatchedpages' => 'Pagine non osservate',

# List redirects
'listredirects' => 'Elenco di tutti i redirect',

# Unused templates
'unusedtemplates'     => 'Template non utilizzati',
'unusedtemplatestext' => 'In questa pagina vengono elencati tutti i template (pagine del namespace Template) che non sono inclusi in nessuna pagina. Prima di cancellarli è opportuno verificare che i singoli template non abbiano altri collegamenti entranti.',
'unusedtemplateswlh'  => 'altri collegamenti',

# Random page
'randompage'         => 'Una pagina a caso',
'randompage-nopages' => 'Nessuna pagina nel namespace selezionato.',

# Random redirect
'randomredirect'         => 'Un redirect a caso',
'randomredirect-nopages' => 'Nessun redirect nel namespace selezionato.',

# Statistics
'statistics'             => 'Statistiche',
'sitestats'              => 'Statistiche relative a {{SITENAME}}',
'userstats'              => 'Statistiche relative agli utenti',
'sitestatstext'          => "Il database contiene complessivamente '''\$1''' {{PLURAL:\$1|pagina|pagine}}.
Questa cifra comprende anche le pagine di discussione, quelle di servizio di {{SITENAME}}, le voci più esigue (\"stub\"), i redirect e altre pagine che probabilmente non vanno considerate tra i contenuti del sito. Escludendo le pagine sopra descritte, ve ne {{PLURAL:\$2|è '''1'''|sono '''\$2'''}} di contenuti veri e propri.

{{PLURAL:\$8|È stato inoltre caricato|Sono stati inoltre caricati}} '''\$8''' file.

Dall'installazione del sito sino a questo momento {{PLURAL:\$3|è stata visitata '''1''' pagina|sono state visitate '''\$3''' pagine}} ed {{PLURAL:\$4|eseguita '''1''' modifica|eseguite '''\$4''' modifiche}}, pari a una media di '''\$5''' modifiche per pagina e '''\$6''' richieste di lettura per ciascuna modifica.

La [http://www.mediawiki.org/wiki/Manual:Job_queue coda dei processi] da eseguire in background contiene {{PLURAL:\$7|'''1''' elemento|'''\$7''' elementi}}.",
'userstatstext'          => "In questo momento {{PLURAL:$1|è registrato '''1''' utente|sono registrati '''$1''' utenti}}. Il gruppo $5 è composto da '''$2''' {{PLURAL:$2|utente|utenti}}, pari al '''$4%''' dei registrati.",
'statistics-mostpopular' => 'Pagine più visitate',

'disambiguations'      => 'Pagine di disambiguazione',
'disambiguationspage'  => 'Template:Disambigua',
'disambiguations-text' => "Le pagine nella lista che segue contengono dei collegamenti a '''pagine di disambiguazione''' e non all'argomento cui dovrebbero fare riferimento.<br />Vengono considerate pagine di disambiguazione tutte quelle che contengono i template elencati in [[MediaWiki:Disambiguationspage]]",

'doubleredirects'            => 'Redirect doppi',
'doubleredirectstext'        => '<b>Attenzione:</b> Questa lista può contenere risultati errati, ad esempio nel caso in cui il comando #REDIRECT sia seguito da altro testo o collegamenti.<br />
Ciascuna riga contiene i collegamenti al primo ed al secondo redirect, oltre alla prima riga di testo del secondo redirect che di solito contiene la pagina di destinazione "corretta" alla quale dovrebbe puntare anche il primo redirect.',
'double-redirect-fixed-move' => '[[$1]] è stata spostata automaticamente, ora è un redirect a [[$2]]',
'double-redirect-fixer'      => 'Correttore di redirect',

'brokenredirects'        => 'Redirect errati',
'brokenredirectstext'    => 'I seguenti redirect puntano a pagine inesistenti:',
'brokenredirects-edit'   => '(modifica)',
'brokenredirects-delete' => '(cancella)',

'withoutinterwiki'         => 'Pagine prive di interwiki',
'withoutinterwiki-summary' => 'Le pagine indicate di seguito sono prive di collegamenti alle versioni in altre lingue:',
'withoutinterwiki-legend'  => 'Prefisso',
'withoutinterwiki-submit'  => 'Mostra',

'fewestrevisions' => 'Voci con meno revisioni',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|byte|byte}}',
'ncategories'             => '$1 {{PLURAL:$1|categoria|categorie}}',
'nlinks'                  => '$1 {{PLURAL:$1|collegamento|collegamenti}}',
'nmembers'                => '$1 {{PLURAL:$1|elemento|elementi}}',
'nrevisions'              => '$1 {{PLURAL:$1|revisione|revisioni}}',
'nviews'                  => '$1 {{PLURAL:$1|visita|visite}}',
'specialpage-empty'       => 'Questa pagina speciale è attualmente vuota.',
'lonelypages'             => 'Pagine orfane',
'lonelypagestext'         => 'Le pagine indicate di seguito sono prive di collegamenti che provengono da altre pagine del sito.',
'uncategorizedpages'      => 'Pagine prive di categorie',
'uncategorizedcategories' => 'Categorie non categorizzate',
'uncategorizedimages'     => 'File privi di categorie',
'uncategorizedtemplates'  => 'Template privi di categorie',
'unusedcategories'        => 'Categorie vuote',
'unusedimages'            => 'File non utilizzati',
'popularpages'            => 'Pagine più visitate',
'wantedcategories'        => 'Categorie richieste',
'wantedpages'             => 'Pagine più richieste',
'missingfiles'            => 'File inesistente',
'mostlinked'              => 'Pagine più richiamate',
'mostlinkedcategories'    => 'Categorie più richiamate',
'mostlinkedtemplates'     => 'Template più utilizzati',
'mostcategories'          => 'Voci con più categorie',
'mostimages'              => 'File più richiamati',
'mostrevisions'           => 'Voci con più revisioni',
'prefixindex'             => 'Indice delle voci per lettere iniziali',
'shortpages'              => 'Pagine più corte',
'longpages'               => 'Pagine più lunghe',
'deadendpages'            => 'Pagine senza uscita',
'deadendpagestext'        => 'Le pagine indicate di seguito sono prive di collegamenti verso altre pagine del sito.',
'protectedpages'          => 'Pagine protette',
'protectedpages-indef'    => 'Solo protezioni infinite',
'protectedpagestext'      => 'Di seguito viene presentato un elenco di pagine protette, di cui è impedita la modifica o lo spostamento',
'protectedpagesempty'     => 'Al momento non vi sono pagine protette',
'protectedtitles'         => 'Titoli protetti',
'protectedtitlestext'     => 'Non è possibile creare pagine con i titoli elencati di seguito',
'protectedtitlesempty'    => 'Al momento non esistono titoli protetti con i parametri specificati.',
'listusers'               => 'Elenco degli utenti',
'newpages'                => 'Pagine più recenti',
'newpages-username'       => 'Nome utente:',
'ancientpages'            => 'Pagine meno recenti',
'move'                    => 'Sposta',
'movethispage'            => 'Sposta questa pagina',
'unusedimagestext'        => 'In questo elenco sono presenti i file caricati e non usati nel sito. Potrebbero essere presenti immagini che sono usate da altri siti con un collegamento diretto.',
'unusedcategoriestext'    => 'Le pagine delle categorie indicate di seguito sono state create ma non contengono nessuna pagina né sottocategoria.',
'notargettitle'           => 'Dati mancanti',
'notargettext'            => "Non è stata indicata una pagina o un utente in relazione al quale eseguire l'operazione richiesta.",
'nopagetitle'             => 'La pagina di destinazione non esiste',
'nopagetext'              => 'La pagina che hai richiesto non esiste.',
'pager-newer-n'           => '{{PLURAL:$1|1 più recente|$1 più recenti}}',
'pager-older-n'           => '{{PLURAL:$1|1 meno recente|$1 meno recenti}}',
'suppress'                => 'Oversight',

# Book sources
'booksources'               => 'Fonti librarie',
'booksources-search-legend' => 'Ricerca di fonti librarie',
'booksources-isbn'          => 'Codice ISBN:',
'booksources-go'            => 'Vai',
'booksources-text'          => 'Di seguito viene presentato un elenco di collegamenti verso siti esterni che vendono libri nuovi e usati, attraverso i quali è possibile ottenere maggiori informazioni sul testo cercato.',

# Special:Log
'specialloguserlabel'  => 'Azione effettuata da:',
'speciallogtitlelabel' => 'Azione effettuata su:',
'log'                  => 'Log',
'all-logs-page'        => 'Tutti i registri',
'log-search-legend'    => 'Ricerca nei registri',
'log-search-submit'    => 'Vai',
'alllogstext'          => "Presentazione unificata di tutti i registri di {{SITENAME}}. Puoi restringere i criteri di ricerca selezionando il tipo di registro, l'utente che ha eseguito l'azione, e/o la pagina interessata (entrambi i campi sono sensibili al maiuscolo/minuscolo).",
'logempty'             => 'Il log non contiene elementi corrispondenti alla ricerca.',
'log-title-wildcard'   => 'Ricerca dei titoli che iniziano con',

# Special:AllPages
'allpages'          => 'Tutte le pagine',
'alphaindexline'    => 'da $1 a $2',
'nextpage'          => 'Pagina successiva ($1)',
'prevpage'          => 'Pagina precedente ($1)',
'allpagesfrom'      => 'Mostra le pagine a partire da:',
'allarticles'       => 'Tutte le voci',
'allinnamespace'    => 'Tutte le pagine del namespace $1',
'allnotinnamespace' => 'Tutte le pagine, escluso il namespace $1',
'allpagesprev'      => 'Precedenti',
'allpagesnext'      => 'Successive',
'allpagessubmit'    => 'Vai',
'allpagesprefix'    => 'Mostra le pagine che iniziano con:',
'allpagesbadtitle'  => 'Il titolo indicato per la pagina non è valido o contiene prefissi interlingua o interwiki. Potrebbe inoltre contenere uno o più caratteri il cui uso non è ammesso nei titoli.',
'allpages-bad-ns'   => 'Il namespace "$1" non esiste su {{SITENAME}}.',

# Special:Categories
'categories'                    => 'Categorie',
'categoriespagetext'            => 'Le categorie indicate di seguito contengono pagine o file multimediali.
Le [[Special:UnusedCategories|categorie vuote]] non sono mostrate qui.
Vedi anche le [[Special:WantedCategories|categorie richieste]].',
'categoriesfrom'                => 'Mostra le categorie a partire da:',
'special-categories-sort-count' => 'ordina per numero',
'special-categories-sort-abc'   => 'ordina alfabeticamente',

# Special:ListUsers
'listusersfrom'      => 'Mostra gli utenti a partire da:',
'listusers-submit'   => 'Mostra',
'listusers-noresult' => 'Nessun utente risponde ai criteri impostati.',

# Special:ListGroupRights
'listgrouprights'          => 'Diritti del gruppo utente',
'listgrouprights-summary'  => "Di seguito sono elencati i gruppi utente definiti per questo wiki, con i diritti d'accesso loro associati.
Potrebbero esserci [[{{MediaWiki:Listgrouprights-helppage}}|ulteriori informazioni]] sui diritti individuali.",
'listgrouprights-group'    => 'Gruppo',
'listgrouprights-rights'   => 'Diritti',
'listgrouprights-helppage' => 'Help:Diritti del gruppo',
'listgrouprights-members'  => '(Elenco dei membri)',

# E-mail user
'mailnologin'     => 'Nessun indirizzo cui inviare il messaggio',
'mailnologintext' => 'Per inviare messaggi e-mail ad altri utenti è necessario [[Special:UserLogin|accedere al sito]] e aver registrato un indirizzo valido nelle proprie [[Special:Preferences|preferenze]].',
'emailuser'       => "Scrivi all'utente",
'emailpage'       => "Invia un messaggio e-mail all'utente",
'emailpagetext'   => 'Se l\'utente ha registrato un indirizzo e-mail valido nelle proprie preferenze, il modulo qui sotto consente di scrivere allo stesso un solo messaggio. L\'indirizzo indicato nelle [[Special:Preferences|preferenze]] del mittente apparirà nel campo "Da:" del messaggio per consentire al destinatario di rispondere direttamente.',
'usermailererror' => "L'oggetto mail ha restituito l'errore:",
'defemailsubject' => 'Messaggio da {{SITENAME}}',
'noemailtitle'    => 'Nessun indirizzo e-mail',
'noemailtext'     => 'Questo utente non ha indicato un indirizzo e-mail valido, oppure ha scelto di non ricevere messaggi di posta elettronica dagli altri utenti.',
'emailfrom'       => 'Da:',
'emailto'         => 'A:',
'emailsubject'    => 'Oggetto:',
'emailmessage'    => 'Messaggio:',
'emailsend'       => 'Invia',
'emailccme'       => 'Invia in copia al mio indirizzo.',
'emailccsubject'  => 'Copia del messaggio inviato a $1: $2',
'emailsent'       => 'Messaggio inviato',
'emailsenttext'   => 'Il messaggio e-mail è stato inviato.',
'emailuserfooter' => 'Questa e-mail è stata inviata da $1 a $2 attraverso la funzione "Invia un messaggio e-mail all\'utente" su {{SITENAME}}.',

# Watchlist
'watchlist'            => 'Osservati speciali',
'mywatchlist'          => 'osservati speciali',
'watchlistfor'         => "(dell'utente '''$1''')",
'nowatchlist'          => 'La lista degli osservati speciali è vuota.',
'watchlistanontext'    => "Per visualizzare e modificare l'elenco degli osservati speciali è necessario $1.",
'watchnologin'         => 'Accesso non effettuato',
'watchnologintext'     => "Per modificare la lista degli osservati speciali è necessario prima eseguire l'[[Special:UserLogin|accesso al sito]].",
'addedwatch'           => 'Pagina aggiunta alla lista degli osservati speciali',
'addedwatchtext'       => "La pagina \"[[:\$1]]\" è stata aggiunta alla propria [[Special:Watchlist|lista degli osservati speciali]]. 
D'ora in poi, le modifiche apportate alla pagina e alla sua discussione verranno elencate in quella sede;
il titolo della pagina apparirà in '''grassetto''' nella pagina delle [[Special:RecentChanges|ultime modifiche]] per renderlo più visibile. 

Se in un secondo tempo si desidera eliminare la pagina dalla lista degli osservati speciali, fare clic su \"non seguire\" nella barra in alto.",
'removedwatch'         => 'Pagina eliminata dalla lista degli osservati speciali',
'removedwatchtext'     => 'La pagina  "[[:$1]]" è stata eliminata dalla lista degli osservati speciali.',
'watch'                => 'Segui',
'watchthispage'        => 'Segui questa pagina',
'unwatch'              => 'Non seguire',
'unwatchthispage'      => 'Smetti di seguire',
'notanarticle'         => 'Questa pagina non è una voce',
'notvisiblerev'        => 'La revisione è stata cancellata',
'watchnochange'        => 'Nessuna delle pagine osservate è stata modificata nel periodo selezionato.',
'watchlist-details'    => 'La lista degli osservati speciali contiene {{PLURAL:$1|una pagina (e la rispettiva pagina di discussione)|$1 pagine (e le rispettive pagine di discussione)}}.',
'wlheader-enotif'      => '* La notifica via e-mail è attiva.',
'wlheader-showupdated' => "* Le pagine che sono state modificate dopo l'ultima visita sono evidenziate in '''grassetto'''",
'watchmethod-recent'   => 'controllo delle modifiche recenti per gli osservati speciali',
'watchmethod-list'     => 'controllo degli osservati speciali per modifiche recenti',
'watchlistcontains'    => 'La lista degli osservati speciali contiene {{PLURAL:$1|una pagina|$1 pagine}}.',
'iteminvalidname'      => "Problemi con la pagina '$1', nome non valido...",
'wlnote'               => "Di seguito {{PLURAL:$1|è elencata la modifica più recente apportata|sono elencate le '''$1''' modifiche più recenti apportate}} {{PLURAL:$2|nella scorsa ora|nelle scorse '''$2''' ore}}.",
'wlshowlast'           => 'Mostra le ultime $1 ore $2 giorni $3',
'watchlist-show-bots'  => 'Mostra le modifiche dei bot',
'watchlist-hide-bots'  => 'Nascondi le modifiche dei bot',
'watchlist-show-own'   => 'Mostra le mie modifiche',
'watchlist-hide-own'   => 'Nascondi le mie modifiche',
'watchlist-show-minor' => 'Mostra le modifiche minori',
'watchlist-hide-minor' => 'Nascondi le modifiche minori',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Aggiunta agli osservati speciali...',
'unwatching' => 'Eliminazione dagli osservati speciali...',

'enotif_mailer'                => 'Sistema di notifica via e-mail di {{SITENAME}}',
'enotif_reset'                 => 'Segna tutte le pagine come già visitate',
'enotif_newpagetext'           => 'Questa è una nuova pagina.',
'enotif_impersonal_salutation' => 'Utente di {{SITENAME}}',
'changed'                      => 'modificata',
'created'                      => 'creata',
'enotif_subject'               => 'La pagina $PAGETITLE di {{SITENAME}} è stata $CHANGEDORCREATED da $PAGEEDITOR',
'enotif_lastvisited'           => 'Consulta $1 per vedere tutte le modifiche dalla tua ultima visita.',
'enotif_lastdiff'              => 'Vedere $1 per visualizzare la modifica.',
'enotif_anon_editor'           => 'utente anonimo $1',
'enotif_body'                  => 'Gentile $WATCHINGUSERNAME,

la pagina $PAGETITLE di {{SITENAME}} è stata $CHANGEDORCREATED in data $PAGEEDITDATE da $PAGEEDITOR; la versione attuale si trova all\'indirizzo $PAGETITLE_URL.

$NEWPAGE

Riassunto della modifica, inserito dall\'autore: $PAGESUMMARY $PAGEMINOREDIT

Contatta l\'autore della modifica:
via e-mail: $PAGEEDITOR_EMAIL
sul sito: $PAGEEDITOR_WIKI

Non verranno inviate altre notifiche in caso di ulteriori cambiamenti, a meno che tu non visiti la pagina. Inoltre, è possibile reimpostare l\'avviso di notifica per tutte le pagine nella lista degli osservati speciali.

             Il sistema di notifica di {{SITENAME}}, al tuo servizio

--
Per modificare le impostazioni della lista degli osservati speciali, visita
{{fullurl:Special:Watchlist/edit}}

Per dare il tuo feedback e ricevere ulteriore assistenza:
{{fullurl:Help:Aiuto}}',

# Delete/protect/revert
'deletepage'                  => 'Cancella pagina',
'confirm'                     => 'Conferma',
'excontent'                   => "il contenuto era: '$1'",
'excontentauthor'             => "il contenuto era: '$1' (e l'unico contributore era '$2')",
'exbeforeblank'               => "Il contenuto prima dello svuotamento era: '$1'",
'exblank'                     => 'la pagina era vuota',
'delete-confirm'              => 'Cancella "$1"',
'delete-legend'               => 'Cancella',
'historywarning'              => 'Attenzione! La pagina che si sta per cancellare ha una cronologia:',
'confirmdeletetext'           => 'Stai per cancellare permanentemente dal database una pagina o una immagine, insieme a tutta la sua cronologia. Per cortesia, conferma che è tua intenzione procedere a tale cancellazione, che hai piena consapevolezza delle conseguenze della tua azione e che essa è conforme alle linee guida stabilite in [[{{MediaWiki:Policy-url}}]].',
'actioncomplete'              => 'Azione completata',
'deletedtext'                 => 'La pagina "$1" è stata cancellata. 
Consultare il log delle $2 per un elenco delle pagine cancellate di recente.',
'deletedarticle'              => 'ha cancellato "[[$1]]"',
'suppressedarticle'           => 'soppresso "[[$1]]"',
'dellogpage'                  => 'Cancellazioni',
'dellogpagetext'              => 'Di seguito sono elencate le pagine cancellate di recente.',
'deletionlog'                 => 'cancellazioni',
'reverted'                    => 'Ripristinata la versione precedente',
'deletecomment'               => 'Motivo della cancellazione:',
'deleteotherreason'           => 'Altra motivazione o motivazione aggiuntiva:',
'deletereasonotherlist'       => 'Altra motivazione',
'deletereason-dropdown'       => "*Motivazioni più comuni per la cancellazione
** Richiesta dell'autore
** Violazione di copyright
** Vandalismo",
'delete-edit-reasonlist'      => 'Modifica i motivi di cancellazione',
'delete-toobig'               => 'La cronologia di questa pagina è molto lunga (oltre $1 {{PLURAL:$1|revisione|revisioni}}). La sua cancellazione è stata limitata per evitare di creare accidentalmente dei problemi di funzionamento al database di {{SITENAME}}.',
'delete-warning-toobig'       => 'La cronologia di questa pagina è molto lunga (oltre $1 {{PLURAL:$1|revisione|revisioni}}). La sua cancellazione può creare dei problemi di funzionamento al database di {{SITENAME}}; procedere con cautela.',
'rollback'                    => 'Annulla le modifiche',
'rollback_short'              => 'Rollback',
'rollbacklink'                => 'rollback',
'rollbackfailed'              => 'Rollback fallito',
'cantrollback'                => "Impossibile annullare le modifiche; l'utente che le ha effettuate è l'unico ad aver contribuito alla pagina.",
'alreadyrolled'               => 'Non è possibile annullare le modifiche apportate alla pagina [[:$1]] da parte di [[User:$2|$2]] ([[User talk:$2|discussione]]); un altro utente ha già modificato la pagina oppure ha effettuato il rollback.

La modifica più recente alla pagina è stata apportata da [[User:$3|$3]] ([[User talk:$3|discussione]]).',
'editcomment'                 => 'Il commento alla modifica era: "<i>$1</i>".', # only shown if there is an edit comment
'revertpage'                  => 'Annullate le modifiche di [[Special:Contributions/$2|$2]] ([[User talk:$2|discussione]]), riportata alla versione precedente di [[User:$1|$1]]', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => "Annullate le modifiche di $1; pagina riportata all'ultima versione di $2.",
'sessionfailure'              => "Si è verificato un problema nella sessione che identifica l'accesso; il sistema non ha eseguito il comando impartito per precauzione. Tornare alla pagina precedente con il tasto 'Indietro' del proprio browser, ricaricare la pagina e riprovare.",
'protectlogpage'              => 'Protezioni',
'protectlogtext'              => 'Di seguito sono elencate le azioni di protezione e sblocco delle pagine.',
'protectedarticle'            => 'ha protetto "[[$1]]"',
'modifiedarticleprotection'   => 'ha modificato il livello di protezione di "[[$1]]"',
'unprotectedarticle'          => 'ha sprotetto [[$1]]',
'protect-title'               => 'Protezione di "$1"',
'protect-legend'              => 'Conferma la protezione',
'protectcomment'              => 'Motivo della protezione:',
'protectexpiry'               => 'Scadenza:',
'protect_expiry_invalid'      => 'Scadenza non valida.',
'protect_expiry_old'          => 'Scadenza già trascorsa.',
'protect-unchain'             => 'Sblocca lo spostamento',
'protect-text'                => 'Questo modulo consente di vedere e modificare il livello di protezione per la pagina <strong><nowiki>$1</nowiki></strong>.',
'protect-locked-blocked'      => 'Non è possibile modificare i livelli di protezione quando è attivo un blocco. Le impostazioni correnti per la pagina sono <strong>$1</strong>:',
'protect-locked-dblock'       => 'Impossibile modificare i livelli di protezione durante un blocco del database.
Le impostazioni correnti per la pagina sono <strong>$1</strong>:',
'protect-locked-access'       => 'Non si dispone dei permessi necessari per modificare i livelli di protezione della pagina.
Le impostazioni correnti per la pagina sono <strong>$1</strong>:',
'protect-cascadeon'           => 'Al momento questa pagina è bloccata perché viene inclusa {{PLURAL:$1|nella pagina indicata di seguito, per la quale|nelle pagine indicate di seguito, per le quali}} è attiva la protezione ricorsiva. È possibile modificare il livello di protezione individuale della pagina, ma le impostazioni derivanti dalla protezione ricorsiva non saranno modificate.',
'protect-default'             => '(predefinito)',
'protect-fallback'            => 'È richiesto il permesso "$1"',
'protect-level-autoconfirmed' => 'Solo utenti registrati',
'protect-level-sysop'         => 'Solo amministratori',
'protect-summary-cascade'     => 'ricorsiva',
'protect-expiring'            => 'scadenza: $1 (UTC)',
'protect-cascade'             => 'Protezione ricorsiva: protegge tutte le pagine incluse in questa pagina. Abilitare l\'opzione solo dopo aver controllato gli include ed esclusivamente se sei sicuro di cosa stai facendo. La scadenza della protezione, diversa da infinito, si può specificare \'\'\'in lingua inglese\'\'\' usando il formato standard GNU, descritto nel manuale di tar (per esempio: "1 hour", "2 days", "next Wednesday", "1 January 2017"). In alternativa, la scadenza può essere "indefinite" o "infinite" (senza scadenza).',
'protect-cantedit'            => 'Non è possibile modificare i livelli di protezione per la pagina in quanto non si dispone dei permessi necessari per modificare la pagina stessa.',
'restriction-type'            => 'Permesso',
'restriction-level'           => 'Livello di restrizione',
'minimum-size'                => 'Dimensione minima',
'maximum-size'                => 'Dimensione massima:',
'pagesize'                    => '(byte)',

# Restrictions (nouns)
'restriction-edit'   => 'Modifica',
'restriction-move'   => 'Spostamento',
'restriction-create' => 'Creazione',
'restriction-upload' => 'Carica',

# Restriction levels
'restriction-level-sysop'         => 'protetta',
'restriction-level-autoconfirmed' => 'semi-protetta',
'restriction-level-all'           => 'tutti i livelli',

# Undelete
'undelete'                     => 'Visualizza pagine cancellate',
'undeletepage'                 => 'Visualizza e recupera le pagine cancellate',
'undeletepagetitle'            => "'''Quanto segue è composto da revisioni cancellate di [[:$1]]'''.",
'viewdeletedpage'              => 'Visualizza le pagine cancellate',
'undeletepagetext'             => "Le pagine indicate di seguito sono state cancellate, ma sono ancora in archivio e pertanto possono essere recuperate. L'archivio può essere svuotato periodicamente.",
'undelete-fieldset-title'      => 'Recupera revisioni',
'undeleteextrahelp'            => "Per recuperare l'intera cronologia della pagina, lasciare tutte le caselle deselezionate e fare clic su '''''Ripristina'''''. Per effettuare un ripristino selettivo, selezionare le caselle corrispondenti alle revisioni da ripristinare e fare clic su '''''Ripristina'''''. Facendo clic su '''''Reimposta''''' verranno deselezionate tutte le caselle e svuotato lo spazio per il commento.",
'undeleterevisions'            => '{{PLURAL:$1|Una revisione|$1 revisioni}} in archivio',
'undeletehistory'              => 'Recuperando questa pagina, tutte le sue revisioni verranno inserite di nuovo nella relativa cronologia. Se dopo la cancellazione è stata creata una nuova pagina con lo stesso titolo, le revisioni recuperate saranno inserite nella cronologia e la versione attualmente online della pagina non verrà modificata.',
'undeleterevdel'               => "Il ripristino non verrà effettuato se determina la cancellazione parziale della versione corrente della pagina o del file interessato. In tal caso, è necessario rimuovere il segno di spunta o l'oscuramento dalle revisioni cancellate più recenti.",
'undeletehistorynoadmin'       => "Questa pagina è stata cancellata. 
Il motivo della cancellazione è mostrato qui sotto, assieme ai dettagli dell'utente che ha modificato questa pagina prima della cancellazione. 
Il testo contenuto nelle revisioni cancellate è disponibile solo agli amministratori.",
'undelete-revision'            => 'Revisione cancellata della pagina $1, inserita il $2 da $3:',
'undeleterevision-missing'     => "Revisione errata o mancante. Il collegamento è errato oppure la revisione è stata già ripristinata o eliminata dall'archivio.",
'undelete-nodiff'              => 'Non è stata trovata nessuna revisione precedente.',
'undeletebtn'                  => 'Ripristina',
'undeletelink'                 => 'ripristina',
'undeletereset'                => 'Reimposta',
'undeletecomment'              => 'Commento:',
'undeletedarticle'             => 'ha recuperato "[[$1]]"',
'undeletedrevisions'           => '{{PLURAL:$1|Una revisione recuperata|$1 revisioni recuperate}}',
'undeletedrevisions-files'     => '{{PLURAL:$1|Una revisione|$1 revisioni}} e $2 file recuperati',
'undeletedfiles'               => '{{PLURAL:$1|Un file recuperato|$1 file recuperati}}',
'cannotundelete'               => 'Ripristino non riuscito; è possibile che la pagina sia già stata recuperata da un altro utente.',
'undeletedpage'                => "<big>'''La pagina $1 è stata recuperata'''</big>

Consulta il [[Special:Log/delete|log delle cancellazioni]] per vedere le cancellazioni e i recuperi più recenti.",
'undelete-header'              => 'Consultare il [[Special:Log/delete|log delle cancellazioni]] per vedere le cancellazioni più recenti.',
'undelete-search-box'          => 'Ricerca nelle pagine cancellate',
'undelete-search-prefix'       => 'Mostra le pagine il cui titolo inizia con:',
'undelete-search-submit'       => 'Cerca',
'undelete-no-results'          => "Nessuna pagina corrispondente nell'archivio dele cancellazioni.",
'undelete-filename-mismatch'   => 'Impossibile annullare la cancellazione della revisione del file con timestamp $1: nome file non corrispondente.',
'undelete-bad-store-key'       => 'Impossibile annullare la cancellazione della revisione del file con timestamp $1: file non disponibile prima della cancellazione.',
'undelete-cleanup-error'       => 'Errore nella cancellazione del file di archivio non utilizzato "$1".',
'undelete-missing-filearchive' => "Impossibile ripristinare l'ID $1 dell'archivio file in quanto non è presente nel database. Potrebbe essere stato già ripristinato.",
'undelete-error-short'         => 'Errore nel ripristino del file: $1',
'undelete-error-long'          => 'Si sono verificati degli errori nel tentativo di annullare la cancellazione del file:

$1',

# Namespace form on various pages
'namespace'      => 'Namespace:',
'invert'         => 'inverti la selezione',
'blanknamespace' => '(Principale)',

# Contributions
'contributions' => 'Contributi utente',
'mycontris'     => 'Miei contributi',
'contribsub2'   => 'Per $1 ($2)',
'nocontribs'    => 'Non sono state trovate modifiche che soddisfino i criteri di ricerca.',
'uctop'         => '(ultima per la pagina)',
'month'         => 'Dal mese (e precedenti):',
'year'          => "Dall'anno (e precedenti):",

'sp-contributions-newbies'     => 'Mostra solo i contributi dei nuovi utenti',
'sp-contributions-newbies-sub' => 'Per i nuovi utenti',
'sp-contributions-blocklog'    => 'blocchi',
'sp-contributions-search'      => 'Ricerca contributi',
'sp-contributions-username'    => 'Indirizzo IP o nome utente:',
'sp-contributions-submit'      => 'Ricerca',

# What links here
'whatlinkshere'            => 'Puntano qui',
'whatlinkshere-title'      => 'Pagine che puntano a "$1"',
'whatlinkshere-page'       => 'Pagina:',
'linklistsub'              => '(Lista dei collegamenti)',
'linkshere'                => "Le seguenti pagine contengono dei collegamenti a '''[[:$1]]''':",
'nolinkshere'              => "Nessuna pagina contiene collegamenti che puntano a '''[[:$1]]'''.",
'nolinkshere-ns'           => "Non vi sono pagine che puntano a '''[[:$1]]''' nel namespace selezionato.",
'isredirect'               => 'redirect',
'istemplate'               => 'inclusione',
'isimage'                  => 'link immagine',
'whatlinkshere-prev'       => '{{PLURAL:$1|precedente|precedenti $1}}',
'whatlinkshere-next'       => '{{PLURAL:$1|successivo|successivi $1}}',
'whatlinkshere-links'      => '← collegamenti',
'whatlinkshere-hideredirs' => '$1 redirect',
'whatlinkshere-hidetrans'  => '$1 inclusioni',
'whatlinkshere-hidelinks'  => '$1 link',
'whatlinkshere-hideimages' => '$1 link da immagini',
'whatlinkshere-filters'    => 'Filtri',

# Block/unblock
'blockip'                         => 'Blocco utente',
'blockip-legend'                  => "Blocca l'utente",
'blockiptext'                     => "Usa il modulo sottostante per bloccare l'accesso in scrittura a uno specifico indirizzo IP o un utente registrato. 

Il blocco dev'essere operato per prevenire atti di vandalismo e in stretta osservanza della [[{{MediaWiki:Policy-url}}|policy di {{SITENAME}}]].

Indica il motivo specifico per il quale procedi al blocco dell'indirizzo IP o dell'utente (per esempio, cita i titoli di eventuali pagine che siano state oggetto di vandalismo).",
'ipaddress'                       => 'Indirizzo IP:',
'ipadressorusername'              => 'Indirizzo IP o nome utente:',
'ipbexpiry'                       => 'Scadenza del blocco:',
'ipbreason'                       => 'Motivo:',
'ipbreasonotherlist'              => 'Altra motivazione',
'ipbreason-dropdown'              => '*Motivazioni più comuni per i blocchi
** Inserimento di informazioni false
** Rimozione di contenuti dalle pagine
** Collegamenti promozionali a siti esterni
** Inserimento di contenuti privi di senso
** Commportamenti intimidatori o molestie
** Uso indebito di più account
** Nome utente non consono',
'ipbanononly'                     => 'Blocca solo utenti anonimi (gli utenti registrati che condividono lo stesso IP non vengono bloccati)',
'ipbcreateaccount'                => 'Impedisci la creazione di altri account',
'ipbemailban'                     => "Impedisci all'utente l'invio di e-mail",
'ipbenableautoblock'              => "Blocca automaticamente l'ultimo indirizzo IP usato dall'utente e i successivi con cui vengono  tentate modifiche",
'ipbsubmit'                       => "Blocca l'utente",
'ipbother'                        => 'Durata non in elenco:',
'ipboptions'                      => '2 ore:2 hours,1 giorno:1 day,3 giorni:3 days,1 settimana:1 week,2 settimane:2 weeks,1 mese:1 month,3 mesi:3 months,6 mesi:6 months,1 anno:1 year,infinito:infinite', # display1:time1,display2:time2,...
'ipbotheroption'                  => 'altro',
'ipbotherreason'                  => 'Altri motivi/dettagli:',
'ipbhidename'                     => "Nascondi il nome utente dal log dei blocchi, dall'elenco dei blocchi attivi e dall'elenco utenti.",
'ipbwatchuser'                    => 'Segui le pagine e le discussioni utente di questo utente',
'badipaddress'                    => 'Indirizzo IP non valido.',
'blockipsuccesssub'               => 'Blocco eseguito',
'blockipsuccesstext'              => '[[Special:Contributions/$1|$1]] è stato bloccato. <br />
Consultare la [[Special:IPBlockList|lista degli IP bloccati]] per vedere i blocchi attivi.',
'ipb-edit-dropdown'               => 'Modifica i motivi per il blocco',
'ipb-unblock-addr'                => 'Sblocca $1',
'ipb-unblock'                     => 'Sblocca un utente o un indirizzo IP',
'ipb-blocklist-addr'              => 'Elenca i blocchi attivi per $1',
'ipb-blocklist'                   => 'Elenca i blocchi attivi',
'unblockip'                       => "Sblocca l'utente",
'unblockiptext'                   => "Usare il modulo sottostante per restituire l'accesso in scrittura ad un utente o indirizzo IP bloccato.",
'ipusubmit'                       => "Sblocca l'utente",
'unblocked'                       => "L'utente [[User:$1|$1]] è stato sbloccato",
'unblocked-id'                    => 'Il blocco $1 è stato rimosso',
'ipblocklist'                     => 'Utenti e indirizzi IP bloccati',
'ipblocklist-legend'              => 'Trova un utente bloccato',
'ipblocklist-username'            => 'Nome utente o indirizzo IP:',
'ipblocklist-submit'              => 'Ricerca',
'blocklistline'                   => '$1, $2 ha bloccato $3 ($4)',
'infiniteblock'                   => 'infinito',
'expiringblock'                   => 'scadenza: $1',
'anononlyblock'                   => 'solo anonimi',
'noautoblockblock'                => 'blocco automatico disabilitato',
'createaccountblock'              => 'creazione account bloccata',
'emailblock'                      => 'e-mail bloccate',
'ipblocklist-empty'               => "L'elenco dei blocchi è vuoto.",
'ipblocklist-no-results'          => "L'indirizzo IP o nome utente richiesto non è bloccato.",
'blocklink'                       => 'blocca',
'unblocklink'                     => 'sblocca',
'contribslink'                    => 'contributi',
'autoblocker'                     => 'Bloccato automaticamente perché l\'indirizzo IP è condiviso con l\'utente "[[User:$1|$1]]".
Il blocco dell\'utente $1 è stato imposto per il seguente motivo: "$2".',
'blocklogpage'                    => 'Blocchi',
'blocklogentry'                   => 'ha bloccato [[$1]] per un periodo di $2 $3',
'blocklogtext'                    => "Di seguito sono elencate le azioni di blocco e sblocco utenti. Gli indirizzi IP bloccati automaticamente non sono elencati. Consultare l'[[Special:IPBlockList|elenco IP bloccati]] per l'elenco degli indirizzi e nomi utente il cui blocco è operativo.",
'unblocklogentry'                 => 'ha sbloccato $1',
'block-log-flags-anononly'        => 'solo utenti anonimi',
'block-log-flags-nocreate'        => 'creazione account bloccata',
'block-log-flags-noautoblock'     => 'blocco automatico disattivato',
'block-log-flags-noemail'         => 'e-mail bloccate',
'block-log-flags-angry-autoblock' => 'blocco automatico avanzato attivo',
'range_block_disabled'            => 'La possibilità di bloccare intervalli di indirizzi IP non è attiva al momento.',
'ipb_expiry_invalid'              => 'Durata o scadenza del blocco non valida. Controlla il [http://www.gnu.org/software/shishi/manual/html_node/Relative-items-in-date-strings.html manuale di tar] per la sintassi esatta.',
'ipb_expiry_temp'                 => 'I blocchi dei nomi utenti nascosti dovrebbero essere infiniti',
'ipb_already_blocked'             => 'L\'utente "$1" è già bloccato',
'ipb_cant_unblock'                => 'Errore: Impossibile trovare il blocco con ID $1. Il blocco potrebbe essere già stato rimosso.',
'ipb_blocked_as_range'            => "Errore: L'indirizzo IP $1 non è soggetto a blocco individuale e non può essere sbloccato. Il blocco è invece attivo a livello dell'intervallo $2, che può essere sbloccato.",
'ip_range_invalid'                => 'Intervallo di indirizzi IP non valido.',
'blockme'                         => 'Bloccami',
'proxyblocker'                    => 'Blocco dei proxy aperti',
'proxyblocker-disabled'           => 'Questa funzione non è attiva.',
'proxyblockreason'                => 'Questo indirizzo IP è stato bloccato perché risulta essere un proxy aperto. Si prega di contattare il proprio fornitore di accesso a Internet o il supporto tecnico e informarli di questo grave problema di sicurezza.',
'proxyblocksuccess'               => 'Blocco eseguito.',
'sorbsreason'                     => 'Questo indirizzo IP è elencato come proxy aperto nella blacklist DNSBL.',
'sorbs_create_account_reason'     => 'Non è possibile creare nuovi accessi da questo indirizzo IP perché è elencato come proxy aperto nella blacklist DNSBL.',

# Developer tools
'lockdb'              => 'Blocca il database',
'unlockdb'            => 'Sblocca il database',
'lockdbtext'          => "Il blocco del database comporta l'interruzione, per tutti gli utenti, della possibilità di modificare le pagine o di crearne di nuove, di cambiare le preferenze e modificare le liste degli osservati speciali, e in generale di tutte le operazioni che richiedono modifiche al database. Per cortesia, conferma che ciò corrisponde effettivamente all'azione da te richiesta e che al termine della manutenzione provvederai allo sblocco del database.",
'unlockdbtext'        => "Lo sblocco del database consente di nuovo a tutti gli utenti di modificare le pagine o di crearne di nuove, di cambiare le preferenze e modificare le liste degli osservati speciali, e in generale di compiere tutte le operazioni che richiedono modifiche al database. Per cortesia, conferma che ciò corrisponde effettivamente all'azione da te richiesta.",
'lockconfirm'         => 'Sì, intendo effettivamente bloccare il database.',
'unlockconfirm'       => 'Sì, intendo effettivamente sbloccare il database.',
'lockbtn'             => 'Blocca il database',
'unlockbtn'           => 'Sblocca il database',
'locknoconfirm'       => 'Non è stata spuntata la casellina di conferma.',
'lockdbsuccesssub'    => 'Blocco del database eseguito',
'unlockdbsuccesssub'  => 'Sblocco del database eseguito',
'lockdbsuccesstext'   => 'Il database è stato bloccato.
<br />Ricorda di rimuovere il blocco dopo aver terminato le operazioni di manutenzione.',
'unlockdbsuccesstext' => ' Il database è stato sbloccato.',
'lockfilenotwritable' => "Impossibile scrivere sul file di ''lock'' del database. L'accesso in scrittura a tale file da parte del server web è necessario per bloccare e sbloccare il database.",
'databasenotlocked'   => 'Il database non è bloccato.',

# Move page
'move-page'               => 'Spostamento di $1',
'move-page-legend'        => 'Spostamento di pagina',
'movepagetext'            => "Questo modulo consente di rinominare una pagina, spostando tutta la sua cronologia al nuovo nome. La pagina attuale diverrà automaticamente un redirect al nuovo titolo. Puoi aggiornare automaticamente i redirect che puntano al titolo originale. Puoi decidere di non farlo, ma ricordati di verificare che lo spostamento non abbia creato [[Special:DoubleRedirects|doppi redirect]] o [[Special:BrokenRedirects|redirect errati]]. L'onere di garantire che i collegamenti alla pagina restino corretti spetta a chi la sposta.

Si noti che la pagina '''non''' sarà spostata se ne esiste già una con il nuovo nome, a meno che non sia vuota o costituita solo da un redirect alla vecchia e sia priva di versioni precedenti. In caso di spostamento errato si può quindi tornare subito al vecchio titolo, e non è possibile sovrascrivere per errore una pagina già esistente.

'''ATTENZIONE:'''
Un cambiamento così drastico può creare contrattempi e problemi, soprattutto per le pagine più visitate. Accertarsi di aver valutato le conseguenze dello spostamento prima di procedere.",
'movepagetalktext'        => "La corrispondente pagina di discussione, se esiste, sarà spostata automaticamente insieme alla pagina principale, '''tranne che nei seguenti casi''':
* lo spostamento della pagina è tra namespace diversi;
* in corrispondenza del nuovo titolo esiste già una pagina di discussione (non vuota);
* la casella qui sotto è stata deselezionata.

In questi casi, se lo si ritiene opportuno, occorre spostare o aggiungere manualmente le informazioni contenute nella pagina di discussione.",
'movearticle'             => 'Sposta la pagina',
'movenotallowed'          => 'Non si dispone dei permessi necessari allo spostamento delle pagine.',
'newtitle'                => 'Nuovo titolo:',
'move-watch'              => 'Aggiungi la pagina agli osservati speciali',
'movepagebtn'             => 'Sposta la pagina',
'pagemovedsub'            => 'Spostamento effettuato con successo',
'movepage-moved'          => '<big>\'\'\'"$1" è stata spostata a "$2"\'\'\'</big>', # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => 'Una pagina con questo nome esiste già, oppure il nome che hai scelto non è valido.<br /> Scegli, per cortesia, un titolo diverso per la pagina.',
'cantmove-titleprotected' => 'Lo spostamento della pagina non è possibile in quanto il nuovo titolo è stato protetto per impedirne la creazione',
'talkexists'              => "'''La pagina è stata spostata correttamente, ma non è stato possibile spostare la pagina di discussione perché ne esiste già un'altra con il nuovo titolo. Integrare manualmente i contenuti delle due pagine.'''",
'movedto'                 => 'spostata a',
'movetalk'                => 'Sposta anche la pagina di discussione.',
'move-subpages'           => 'Sposta tutte le sottopagine, se possibile',
'move-talk-subpages'      => 'Sposta tutte le sottopagine di discussione, se possibile',
'movepage-page-exists'    => 'La pagina $1 esiste già e non può essere automaticamente sovrascritta.',
'movepage-page-moved'     => 'La pagina $1 è stata spostata a $2.',
'movepage-page-unmoved'   => 'La pagina $1 non può essere spostata a $2.',
'movepage-max-pages'      => 'È stato spostato il numero massimo di $1 {{PLURAL:$1|pagina|pagine}} e non protranno essere spostate ulteriori pagine automaticamente.',
'1movedto2'               => 'ha spostato [[$1]] a [[$2]]',
'1movedto2_redir'         => 'ha spostato [[$1]] a [[$2]] tramite redirect',
'movelogpage'             => 'Spostamenti',
'movelogpagetext'         => 'Di seguito sono elencate le pagine spostate di recente.',
'movereason'              => 'Motivo:',
'revertmove'              => 'ripristina',
'delete_and_move'         => 'Cancella e sposta',
'delete_and_move_text'    => '==Cancellazione richiesta==

La pagina specificata come destinazione "[[:$1]]" esiste già. Vuoi cancellarla per proseguire con lo spostamento?',
'delete_and_move_confirm' => 'Sì, sovrascrivi la pagina esistente',
'delete_and_move_reason'  => 'Cancellata per rendere possibile lo spostamento',
'selfmove'                => "Il titolo di destinazione inserito è uguale a quello di provenienza: '''attenzione''', leggi i titoli dei campi prima di confermare un comando! Il secondo campo contiene un commento che è necessario per giustificare lo spostamento della pagine e viene memorizzato nel log.",
'immobile_namespace'      => 'Il nuovo titolo corrisponde a una pagina speciale; impossibile spostare pagine in quel namespace.',
'imagenocrossnamespace'   => "Non puoi spostare un'immagine fuori del namespace Immagine.",
'imagetypemismatch'       => 'La nuova estensione del file non corrisponde alla sua reale estensione',
'imageinvalidfilename'    => "Il nome dell'immagine non è valido",
'fix-double-redirects'    => 'Aggiorna tutti i redirect che puntano al titolo originale',

# Export
'export'            => 'Esporta pagine',
'exporttext'        => "È possibile esportare il testo e la cronologia delle modifiche di una pagina o di un gruppo di pagine in formato XML per importarle in altri siti che utilizzano il software MediaWiki, attraverso la [[Special:Import|pagina delle importazioni]].

Per esportare le pagine indicare i titoli nella casella di testo sottostante, uno per riga, e specificare se si desidera ottenere la versione corrente e tutte le versioni precedenti, con i dati della cronologia della pagina, oppure soltanto l'ultima versione e i dati corrispondenti all'ultima modifica.

In quest'ultimo caso si può anche utilizzare un collegamento, ad esempio [[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]] per esportare \"[[{{MediaWiki:Mainpage}}]]\".",
'exportcuronly'     => "Includi solo la revisione attuale, non l'intera cronologia",
'exportnohistory'   => "----
'''Nota:''' l'esportazione dell'intera cronologia delle pagine attraverso questa interfaccia è stata disattivata per motivi legati alle prestazioni del sistema.",
'export-submit'     => 'Esporta',
'export-addcattext' => 'Aggiungi pagine dalla categoria:',
'export-addcat'     => 'Aggiungi',
'export-download'   => 'Richiedi il salvataggio come file',
'export-templates'  => 'Includi i template',

# Namespace 8 related
'allmessages'               => 'Messaggi di sistema',
'allmessagesname'           => 'Nome',
'allmessagesdefault'        => 'Testo predefinito',
'allmessagescurrent'        => 'Testo attuale',
'allmessagestext'           => 'Questa è la lista di tutti i messaggi di sistema disponibili nel namespace MediaWiki:',
'allmessagesnotsupportedDB' => "'''{{ns:special}}:Allmessages''' non è supportato perché il flag '''\$wgUseDatabaseMessages''' non è attivo.",
'allmessagesfilter'         => 'Filtro sui messaggi:',
'allmessagesmodified'       => 'Mostra solo quelli modificati',

# Thumbnails
'thumbnail-more'           => 'Ingrandisci',
'filemissing'              => 'File mancante',
'thumbnail_error'          => 'Errore nella creazione della miniatura: $1',
'djvu_page_error'          => 'Numero di pagina DjVu errato',
'djvu_no_xml'              => "Impossibile ottenere l'XML per il file DjVu",
'thumbnail_invalid_params' => 'Parametri anteprima non corretti',
'thumbnail_dest_directory' => 'Impossibile creare la directory di destinazione',

# Special:Import
'import'                     => 'Importa pagine',
'importinterwiki'            => 'Importazione transwiki',
'import-interwiki-text'      => 'Selezionare un progetto wiki e il titolo della pagina da importare.
Le date di pubblicazione e i nomi degli autori delle varie versioni saranno conservati.
Tutte le operazioni di importazione trans-wiki sono registrate nel [[Special:Log/import|log di importazione]].',
'import-interwiki-history'   => "Copia l'intera cronologia di questa pagina",
'import-interwiki-submit'    => 'Importa',
'import-interwiki-namespace' => 'Trasferisci le pagine nel namespace:',
'importtext'                 => 'Si prega di esportare il file dal sito wiki di origine con la funzione Special:Export, salvarlo sul proprio disco e poi caricarlo qui.',
'importstart'                => 'Importazione delle pagine in corso...',
'import-revision-count'      => '{{PLURAL:$1|una revisione importata|$1 revisioni importate}}',
'importnopages'              => 'Nessuna pagina da importare.',
'importfailed'               => 'Importazione non riuscita: $1',
'importunknownsource'        => "Tipo di origine sconosciuto per l'importazione",
'importcantopen'             => 'Impossibile aprire il file di importazione',
'importbadinterwiki'         => 'Collegamento inter-wiki errato',
'importnotext'               => 'Testo vuoto o mancante',
'importsuccess'              => 'Importazione riuscita.',
'importhistoryconflict'      => 'La cronologia contiene delle versioni in conflitto (questa pagina potrebbe essere già stata importata)',
'importnosources'            => "Non è stata definita una fonte per l'importazione transwiki; l'importazione diretta della cronologia non è attiva.",
'importnofile'               => "Non è stato caricato nessun file per l'importazione.",
'importuploaderrorsize'      => "Caricamento del file per l'importazione non riuscito. Il file supera le dimensioni massime consentite per l'upload.",
'importuploaderrorpartial'   => "Caricamento del file per l'importazione non riuscito. Il file è stato caricato solo in parte.",
'importuploaderrortemp'      => "Caricamento del file per l'importazione non riuscito. Manca una cartella temporanea.",
'import-parse-failure'       => "Errore di analisi nell'importazione XML",
'import-noarticle'           => 'Nessuna pagina da importare.',
'import-nonewrevisions'      => 'Tutte le revisioni sono già state importate in precedenza.',
'xml-error-string'           => '$1 a riga $2, colonna $3 (byte $4): $5',
'import-upload'              => 'Carica dati XML',

# Import log
'importlogpage'                    => 'Importazioni',
'importlogpagetext'                => 'Di seguito sono elencate le importazioni di pagine provenienti da altre wiki, complete di cronologia.',
'import-logentry-upload'           => 'ha importato [[$1]] tramite upload',
'import-logentry-upload-detail'    => '{{PLURAL:$1|una revisione importata|$1 revisioni importate}}',
'import-logentry-interwiki'        => 'ha trasferito da altra wiki la pagina [[$1]]',
'import-logentry-interwiki-detail' => '{{PLURAL:$1|una revisione importata|$1 revisioni importate}} da $2',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'La tua pagina utente',
'tooltip-pt-anonuserpage'         => 'La pagina utente di questo indirizzo IP',
'tooltip-pt-mytalk'               => 'La tua pagina di discussione',
'tooltip-pt-anontalk'             => 'Discussioni sulle modifiche fatte da questo indirizzo IP',
'tooltip-pt-preferences'          => 'Le mie preferenze',
'tooltip-pt-watchlist'            => 'La lista delle pagine che stai tenendo sotto osservazione',
'tooltip-pt-mycontris'            => "L'elenco dei tuoi contributi",
'tooltip-pt-login'                => 'La registrazione è consigliata, anche se non obbligatoria',
'tooltip-pt-anonlogin'            => 'La registrazione è consigliata, anche se non obbligatoria',
'tooltip-pt-logout'               => 'Uscita (logout)',
'tooltip-ca-talk'                 => 'Vedi le discussioni relative a questa pagina',
'tooltip-ca-edit'                 => 'Puoi modificare questa pagina. Per favore usa il pulsante di anteprima prima di salvare',
'tooltip-ca-addsection'           => 'Aggiungi un commento a questa discussione',
'tooltip-ca-viewsource'           => 'Questa pagina è protetta, ma puoi vedere il suo codice sorgente',
'tooltip-ca-history'              => 'Versioni precedenti di questa pagina',
'tooltip-ca-protect'              => 'Proteggi questa pagina',
'tooltip-ca-delete'               => 'Cancella questa pagina',
'tooltip-ca-undelete'             => "Ripristina la pagina com'era prima della cancellazione",
'tooltip-ca-move'                 => 'Sposta questa pagina (cambia titolo)',
'tooltip-ca-watch'                => 'Aggiungi questa pagina alla tua lista di osservati speciali',
'tooltip-ca-unwatch'              => 'Elimina questa pagina dalla tua lista di osservati speciali',
'tooltip-search'                  => "Cerca all'interno di {{SITENAME}}",
'tooltip-search-go'               => 'Vai a una pagina con il titolo indicato, se esiste',
'tooltip-search-fulltext'         => 'Cerca il testo indicato nelle pagine',
'tooltip-p-logo'                  => 'Pagina principale',
'tooltip-n-mainpage'              => 'Visita la pagina principale',
'tooltip-n-portal'                => 'Descrizione del progetto, cosa puoi fare, dove trovare le cose',
'tooltip-n-currentevents'         => 'Informazioni sugli eventi di attualità',
'tooltip-n-recentchanges'         => 'Elenco delle ultime modifiche del sito',
'tooltip-n-randompage'            => 'Mostra una pagina a caso',
'tooltip-n-help'                  => 'Pagine di aiuto',
'tooltip-t-whatlinkshere'         => 'Elenco di tutte le pagine che sono collegate a questa',
'tooltip-t-recentchangeslinked'   => 'Elenco delle ultime modifiche alle pagine collegate a questa',
'tooltip-feed-rss'                => 'Feed RSS per questa pagina',
'tooltip-feed-atom'               => 'Feed Atom per questa pagina',
'tooltip-t-contributions'         => 'Lista dei contributi di questo utente',
'tooltip-t-emailuser'             => 'Invia un messaggio e-mail a questo utente',
'tooltip-t-upload'                => 'Carica file multimediali',
'tooltip-t-specialpages'          => 'Lista di tutte le pagine speciali',
'tooltip-t-print'                 => 'Versione stampabile di questa pagina',
'tooltip-t-permalink'             => 'Collegamento permanente a questa versione della pagina',
'tooltip-ca-nstab-main'           => 'Vedi la voce',
'tooltip-ca-nstab-user'           => 'Vedi la pagina utente',
'tooltip-ca-nstab-media'          => 'Vedi la pagina del file multimediale',
'tooltip-ca-nstab-special'        => 'Questa è una pagina speciale, non può essere modificata',
'tooltip-ca-nstab-project'        => 'Vedi la pagina di servizio',
'tooltip-ca-nstab-image'          => 'Vedi la pagina del file',
'tooltip-ca-nstab-mediawiki'      => 'Vedi il messaggio di sistema',
'tooltip-ca-nstab-template'       => 'Vedi il template',
'tooltip-ca-nstab-help'           => 'Vedi la pagina di aiuto',
'tooltip-ca-nstab-category'       => 'Vedi la pagina della categoria',
'tooltip-minoredit'               => 'Segnala come modifica minore',
'tooltip-save'                    => 'Salva le modifiche',
'tooltip-preview'                 => 'Anteprima delle modifiche (consigliata, prima di salvare!)',
'tooltip-diff'                    => 'Guarda le modifiche apportate al testo.',
'tooltip-compareselectedversions' => 'Guarda le differenze tra le due versioni selezionate di questa pagina.',
'tooltip-watch'                   => 'Aggiungi questa pagina alla lista degli osservati speciali',
'tooltip-recreate'                => 'Ricrea la pagina anche se è stata cancellata',
'tooltip-upload'                  => 'Inizia il caricamento',

# Stylesheets
'common.css'      => '/* Gli stili CSS inseriti qui si applicano a tutte le skin */',
'standard.css'    => '/* Gli stili CSS inseriti qui si applicano agli utenti che usano la skin Standard */',
'nostalgia.css'   => '/* Gli stili CSS inseriti qui si applicano agli utenti che usano la skin Nostalgia */',
'cologneblue.css' => '/* Gli stili CSS inseriti qui si applicano agli utenti che usano la skin Cologne Blue */',
'monobook.css'    => '/* Gli stili CSS inseriti qui si applicano agli utenti che usano la skin Monobook */',
'myskin.css'      => '/* Gli stili CSS inseriti qui si applicano agli utenti che usano la skin Myskin */',
'chick.css'       => '/* Gli stili CSS inseriti qui si applicano agli utenti che usano la skin Chick */',
'simple.css'      => '/* Gli stili CSS inseriti qui si applicano agli utenti che usano la skin Simple */',
'modern.css'      => '/* Gli stili CSS inseriti qui si applicano agli utenti che usano la skin Modern */',

# Scripts
'common.js'      => '/* Il codice JavaScript inserito qui viene caricato da ciascuna pagina, per tutti gli utenti. */',
'standard.js'    => '/* Il codice JavaScript inserito qui viene caricato dagli utenti che usano la skin Standard */',
'nostalgia.js'   => '/* Il codice JavaScript inserito qui viene caricato dagli utenti che usano la skin Nostalgia */',
'cologneblue.js' => '/* Il codice JavaScript inserito qui viene caricato dagli utenti che usano la skin Cologne Blue */',
'monobook.js'    => '/* Il codice JavaScript inserito qui viene caricato dagli utenti che usano la skin MonoBook */',
'myskin.js'      => '/* Il codice JavaScript inserito qui viene caricato dagli utenti che usano la skin Myskin */',
'chick.js'       => '/* Il codice JavaScript inserito qui viene caricato dagli utenti che usano la skin Chick */',
'simple.js'      => '/* Il codice JavaScript inserito qui viene caricato dagli utenti che usano la skin Simple */',
'modern.js'      => '/* Il codice JavaScript inserito qui viene caricato dagli utenti che usano la skin Modern */',

# Metadata
'nodublincore'      => 'Metadati Dublin Core RDF non attivi su questo server.',
'nocreativecommons' => 'Metadati Commons RDF non attivi su questo server.',
'notacceptable'     => 'Il server wiki non è in grado di fornire i dati in un formato leggibile dal tuo client.',

# Attribution
'anonymous'        => 'uno o più utenti anonimi di {{SITENAME}}',
'siteuser'         => '$1, utente di {{SITENAME}}',
'lastmodifiedatby' => "Questa pagina è stata modificata per l'ultima volta il $2, $1 da $3.", # $1 date, $2 time, $3 user
'othercontribs'    => 'Il testo attuale è basato su contributi di $1.',
'others'           => 'altri',
'siteusers'        => '$1, utenti di {{SITENAME}}',
'creditspage'      => 'Autori della pagina',
'nocredits'        => 'Nessuna informazione sugli autori disponibile per questa pagina.',

# Spam protection
'spamprotectiontitle' => 'Filtro anti-spam',
'spamprotectiontext'  => 'La pagina che si è tentato di salvare è stata bloccata dal filtro anti-spam. Ciò è probabilmente dovuto alla presenza di un collegamento a un sito esterno presente nella blacklist.',
'spamprotectionmatch' => 'Il filtro anti-spam è stato attivato dal seguente testo: $1',
'spambot_username'    => 'MediaWiki - sistema di rimozione spam',
'spam_reverting'      => "Ripristinata l'ultima versione priva di collegamenti a $1",
'spam_blanking'       => 'Pagina svuotata, tutte le versioni contenevano collegamenti a $1',

# Info page
'infosubtitle'   => 'Informazioni per la pagina',
'numedits'       => 'Numero di modifiche (pagina): $1',
'numtalkedits'   => 'Numero di modifiche (pagina di discussione): $1',
'numwatchers'    => 'Numero di osservatori: $1',
'numauthors'     => 'Numero di autori distinti (pagina): $1',
'numtalkauthors' => 'Numero di autori distinti (pagina di discussione): $1',

# Math options
'mw_math_png'    => 'Mostra sempre in PNG',
'mw_math_simple' => 'HTML se molto semplice, altrimenti PNG',
'mw_math_html'   => 'HTML se possibile, altrimenti PNG',
'mw_math_source' => 'Lascia in formato TeX (per browser testuali)',
'mw_math_modern' => 'Formato consigliato per i browser moderni',
'mw_math_mathml' => 'Usa MathML se possibile (sperimentale)',

# Patrolling
'markaspatrolleddiff'                 => 'Segna la modifica come verificata',
'markaspatrolledtext'                 => 'Segna questa pagina come verificata',
'markedaspatrolled'                   => 'Modifica verificata',
'markedaspatrolledtext'               => 'La modifica selezionata è stata segnata come verificata.',
'rcpatroldisabled'                    => 'La verifica delle ultime modifiche è disattivata',
'rcpatroldisabledtext'                => 'La funzione di verifica delle ultime modifiche al momento non è attiva.',
'markedaspatrollederror'              => 'Impossibile contrassegnare la voce come verificata',
'markedaspatrollederrortext'          => 'Occorre specificare una modifica da contrassegnare come verificata.',
'markedaspatrollederror-noautopatrol' => 'Non si dispone dei permessi necessari per segnare le proprie modifiche come verificate.',

# Patrol log
'patrol-log-page'   => 'Modifiche verificate',
'patrol-log-header' => 'Di seguito sono elencate le verifiche delle modifiche.',
'patrol-log-line'   => 'ha segnato la $1 alla pagina $2 come verificata $3',
'patrol-log-auto'   => '(verifica automatica)',
'patrol-log-diff'   => 'modifica $1',

# Image deletion
'deletedrevision'                 => 'Cancellata la vecchia revisione di $1.',
'filedeleteerror-short'           => 'Errore nella cancellazione del file: $1',
'filedeleteerror-long'            => 'Si sono verificati degli errori nel tentativo di cancellare il file:

$1',
'filedelete-missing'              => 'Impossibile cancellare il file "$1" in quanto non esiste.',
'filedelete-old-unregistered'     => 'La revisione del file indicata, "$1", non è contenuta nel database.',
'filedelete-current-unregistered' => 'Il file specificato, "$1", non è contenuto nel database.',
'filedelete-archive-read-only'    => 'Il server Web non è in grado di scrivere nella directory di archivio "$1".',

# Browsing diffs
'previousdiff' => '← Differenza precedente',
'nextdiff'     => 'Differenza successiva →',

# Media information
'mediawarning'         => "'''Attenzione''': Questo file può contenere codice maligno; la sua esecuzione può danneggiare il proprio sistema informatico.<hr />",
'imagemaxsize'         => 'Dimensione massima delle immagini sulle relative pagine di discussione:',
'thumbsize'            => 'Grandezza delle miniature:',
'widthheightpage'      => '$1×$2, $3 {{PLURAL:$3|pagina|pagine}}',
'file-info'            => 'Dimensioni: $1, tipo MIME: $2',
'file-info-size'       => '($1 × $2 pixel, dimensioni: $3, tipo MIME: $4)',
'file-nohires'         => '<small>Non sono disponibili versioni a risoluzione più elevata.</small>',
'svg-long-desc'        => '(file in formato SVG, dimensioni nominali $1 × $2 pixel, dimensione del file: $3)',
'show-big-image'       => 'Versione ad alta risoluzione',
'show-big-image-thumb' => '<small>Dimensioni di questa anteprima: $1 × $2 pixel</small>',

# Special:NewImages
'newimages'             => 'Galleria dei nuovi file',
'imagelisttext'         => "La lista presentata di seguito, costituita da {{PLURAL:$1|un file|'''$1''' file}}, è ordinata per $2.",
'newimages-summary'     => 'Questa pagina speciale mostra i file caricati più di recente.',
'showhidebots'          => '($1 i bot)',
'noimages'              => "Non c'è nulla da vedere.",
'ilsubmit'              => 'Ricerca',
'bydate'                => 'data',
'sp-newimages-showfrom' => 'Mostra i file più recenti a partire dalle ore $2 del $1',

# Bad image list
'bad_image_list' => "Il formato è il seguente:

Vengono considerati soltanto gli elenchi puntati (righe che cominciano con il carattere *). Il primo collegamento su ciascuna riga dev'essere un collegamento a un file indesiderato.
I collegamenti successivi, sulla stessa riga, sono considerati come eccezioni (ovvero, pagine nelle quali il file può essere richiamato normalmente).",

# Metadata
'metadata'          => 'Metadati',
'metadata-help'     => 'Questo file contiene informazioni aggiuntive, probabilmente aggiunte dalla fotocamera o dallo scanner usati per crearlo o digitalizzarlo. Se il file è stato modificato, alcuni dettagli potrebbero non corrispondere alla realtà.',
'metadata-expand'   => 'Mostra dettagli',
'metadata-collapse' => 'Nascondi dettagli',
'metadata-fields'   => "I campi relativi ai metadati EXIF elencati in questo messaggio verranno mostrati sulla pagina dell'immagine quando la tabella dei metadati è presentata nella forma breve. Per impostazione predefinita, gli altri campi verranno nascosti.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength", # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => 'Larghezza',
'exif-imagelength'                 => 'Altezza',
'exif-bitspersample'               => 'Bit per campione',
'exif-compression'                 => 'Meccanismo di compressione',
'exif-photometricinterpretation'   => 'Struttura dei pixel',
'exif-orientation'                 => 'Orientamento',
'exif-samplesperpixel'             => 'Numero delle componenti',
'exif-planarconfiguration'         => 'Disposizione dei dati',
'exif-ycbcrsubsampling'            => 'Rapporto di campionamento Y / C',
'exif-ycbcrpositioning'            => 'Posizionamento componenti Y e C',
'exif-xresolution'                 => 'Risoluzione orizzontale',
'exif-yresolution'                 => 'Risoluzione verticale',
'exif-resolutionunit'              => 'Unità di misura risoluzione X e Y',
'exif-stripoffsets'                => 'Posizione dei dati immagine',
'exif-rowsperstrip'                => 'Numero righe per striscia',
'exif-stripbytecounts'             => 'Numero di byte per striscia compressa',
'exif-jpeginterchangeformat'       => 'Posizione byte SOI JPEG',
'exif-jpeginterchangeformatlength' => 'Numero di byte di dati JPEG',
'exif-transferfunction'            => 'Funzione di trasferimento',
'exif-whitepoint'                  => 'Coordinate cromatiche del punto di bianco',
'exif-primarychromaticities'       => 'Coordinate cromatiche dei colori primari',
'exif-ycbcrcoefficients'           => 'Coefficienti matrice di trasformazione spazi dei colori',
'exif-referenceblackwhite'         => 'Coppia di valori di riferimento (nero e bianco)',
'exif-datetime'                    => 'Data e ora di modifica del file',
'exif-imagedescription'            => "Descrizione dell'immagine",
'exif-make'                        => 'Produttore fotocamera',
'exif-model'                       => 'Modello fotocamera',
'exif-software'                    => 'Software',
'exif-artist'                      => 'Autore',
'exif-copyright'                   => 'Informazioni sul copyright',
'exif-exifversion'                 => 'Versione del formato Exif',
'exif-flashpixversion'             => 'Versione Flashpix supportata',
'exif-colorspace'                  => 'Spazio dei colori',
'exif-componentsconfiguration'     => 'Significato di ciascuna componente',
'exif-compressedbitsperpixel'      => 'Modalità di compressione immagine',
'exif-pixelydimension'             => 'Larghezza effettiva immagine',
'exif-pixelxdimension'             => 'Altezza effettiva immagine',
'exif-makernote'                   => 'Note del produttore',
'exif-usercomment'                 => "Note dell'utente",
'exif-relatedsoundfile'            => 'File audio collegato',
'exif-datetimeoriginal'            => 'Data e ora di creazione dei dati',
'exif-datetimedigitized'           => 'Data e ora di digitalizzazione',
'exif-subsectime'                  => 'Data e ora, frazioni di secondo',
'exif-subsectimeoriginal'          => 'Data e ora di creazione, frazioni di secondo',
'exif-subsectimedigitized'         => 'Data e ora di digitalizzazione, frazioni di secondo',
'exif-exposuretime'                => 'Tempo di esposizione',
'exif-exposuretime-format'         => '$1 s ($2)',
'exif-fnumber'                     => 'Rapporto focale',
'exif-exposureprogram'             => 'Programma di esposizione',
'exif-spectralsensitivity'         => 'Sensibilità spettrale',
'exif-isospeedratings'             => 'Sensibilità ISO',
'exif-oecf'                        => 'Fattore di conversione optoelettronica',
'exif-shutterspeedvalue'           => 'Tempo di esposizione',
'exif-aperturevalue'               => 'Apertura',
'exif-brightnessvalue'             => 'Luminosità',
'exif-exposurebiasvalue'           => 'Correzione esposizione',
'exif-maxaperturevalue'            => 'Apertura massima',
'exif-subjectdistance'             => 'Distanza del soggetto',
'exif-meteringmode'                => 'Metodo di misurazione',
'exif-lightsource'                 => 'Sorgente luminosa',
'exif-flash'                       => 'Caratteristiche e stato del flash',
'exif-focallength'                 => 'Distanza focale obiettivo',
'exif-subjectarea'                 => 'Area inquadrante il soggetto',
'exif-flashenergy'                 => 'Potenza del flash',
'exif-spatialfrequencyresponse'    => 'Risposta in frequenza spaziale',
'exif-focalplanexresolution'       => 'Risoluzione X sul piano focale',
'exif-focalplaneyresolution'       => 'Risoluzione Y sul piano focale',
'exif-focalplaneresolutionunit'    => 'Unità di misura risoluzione sul piano focale',
'exif-subjectlocation'             => 'Posizione del soggetto',
'exif-exposureindex'               => 'Sensibilità impostata',
'exif-sensingmethod'               => 'Metodo di rilevazione',
'exif-filesource'                  => 'Origine del file',
'exif-scenetype'                   => 'Tipo di inquadratura',
'exif-cfapattern'                  => 'Disposizione filtro colore',
'exif-customrendered'              => 'Elaborazione personalizzata',
'exif-exposuremode'                => 'Modalità di esposizione',
'exif-whitebalance'                => 'Bilanciamento del bianco',
'exif-digitalzoomratio'            => 'Rapporto zoom digitale',
'exif-focallengthin35mmfilm'       => 'Focale equivalente su 35 mm',
'exif-scenecapturetype'            => 'Tipo di acquisizione',
'exif-gaincontrol'                 => 'Controllo inquadratura',
'exif-contrast'                    => 'Controllo contrasto',
'exif-saturation'                  => 'Controllo saturazione',
'exif-sharpness'                   => 'Controllo nitidezza',
'exif-devicesettingdescription'    => 'Descrizione impostazioni dispositivo',
'exif-subjectdistancerange'        => 'Scala distanza soggetto',
'exif-imageuniqueid'               => 'ID univoco immagine',
'exif-gpsversionid'                => 'Versione dei tag GPS',
'exif-gpslatituderef'              => 'Latitudine Nord/Sud',
'exif-gpslatitude'                 => 'Latitudine',
'exif-gpslongituderef'             => 'Longitudine Est/Ovest',
'exif-gpslongitude'                => 'Longitudine',
'exif-gpsaltituderef'              => "Riferimento per l'altitudine",
'exif-gpsaltitude'                 => 'Altitudine',
'exif-gpstimestamp'                => 'Ora GPS (orologio atomico)',
'exif-gpssatellites'               => 'Satelliti usati per la misurazione',
'exif-gpsstatus'                   => 'Stato del ricevitore',
'exif-gpsmeasuremode'              => 'Modalità di misurazione',
'exif-gpsdop'                      => 'Precisione della misurazione',
'exif-gpsspeedref'                 => 'Unità di misura della velocità',
'exif-gpsspeed'                    => 'Velocità del ricevitore GPS',
'exif-gpstrackref'                 => 'Riferimento per la direzione movimento',
'exif-gpstrack'                    => 'Direzione del movimento',
'exif-gpsimgdirectionref'          => "Riferimento per la direzione dell'immagine",
'exif-gpsimgdirection'             => "Direzione dell'immagine",
'exif-gpsmapdatum'                 => 'Rilevamento geodetico usato',
'exif-gpsdestlatituderef'          => 'Riferimento per la latitudine della destinazione',
'exif-gpsdestlatitude'             => 'Latitudine della destinazione',
'exif-gpsdestlongituderef'         => 'Riferimento per la longitudine della destinazione',
'exif-gpsdestlongitude'            => 'Longitudine della destinazione',
'exif-gpsdestbearingref'           => 'Riferimento per la direzione della destinazione',
'exif-gpsdestbearing'              => 'Direzione della destinazione',
'exif-gpsdestdistanceref'          => 'Riferimento per la distanza della destinazione',
'exif-gpsdestdistance'             => 'Distanza della destinazione',
'exif-gpsprocessingmethod'         => 'Nome del metodo di elaborazione GPS',
'exif-gpsareainformation'          => 'Nome della zona GPS',
'exif-gpsdatestamp'                => 'Data GPS',
'exif-gpsdifferential'             => 'Correzione differenziale GPS',

# EXIF attributes
'exif-compression-1' => 'Nessuno',

'exif-unknowndate' => 'Data sconosciuta',

'exif-orientation-1' => 'Normale', # 0th row: top; 0th column: left
'exif-orientation-2' => 'Capovolto orizzontalmente', # 0th row: top; 0th column: right
'exif-orientation-3' => 'Ruotato di 180°', # 0th row: bottom; 0th column: right
'exif-orientation-4' => 'Capovolto verticalmente', # 0th row: bottom; 0th column: left
'exif-orientation-5' => 'Ruotato 90° in senso antiorario e capovolto verticalmente', # 0th row: left; 0th column: top
'exif-orientation-6' => 'Ruotato 90° in senso orario', # 0th row: right; 0th column: top
'exif-orientation-7' => 'Ruotato 90° in senso orario e capovolto verticalmente', # 0th row: right; 0th column: bottom
'exif-orientation-8' => 'Ruotato 90° in senso antiorario', # 0th row: left; 0th column: bottom

'exif-planarconfiguration-1' => 'a blocchi (chunky)',
'exif-planarconfiguration-2' => 'lineare (planar)',

'exif-xyresolution-i' => '$1 punti per pollice (dpi)',
'exif-xyresolution-c' => '$1 punti per centimetro (dpc)',

'exif-colorspace-ffff.h' => 'Non calibrato',

'exif-componentsconfiguration-0' => 'assente',

'exif-exposureprogram-0' => 'Non definito',
'exif-exposureprogram-1' => 'Manuale',
'exif-exposureprogram-2' => 'Standard',
'exif-exposureprogram-3' => 'Priorità al diaframma',
'exif-exposureprogram-4' => "Priorità all'esposizione",
'exif-exposureprogram-5' => 'Artistico (orientato alla profondità di campo)',
'exif-exposureprogram-6' => 'Sportivo (orientato alla velocità di ripresa)',
'exif-exposureprogram-7' => 'Ritratto (soggetti vicini con sfondo fuori fuoco)',
'exif-exposureprogram-8' => 'Panorama (soggetti lontani con sfondo a fuoco)',

'exif-subjectdistance-value' => '$1 metri',

'exif-meteringmode-0'   => 'Sconosciuto',
'exif-meteringmode-1'   => 'Media',
'exif-meteringmode-2'   => 'Media pesata centrata',
'exif-meteringmode-3'   => 'Spot',
'exif-meteringmode-4'   => 'MultiSpot',
'exif-meteringmode-5'   => 'Pattern',
'exif-meteringmode-6'   => 'Parziale',
'exif-meteringmode-255' => 'Altro',

'exif-lightsource-0'   => 'Sconosciuta',
'exif-lightsource-1'   => 'Luce diurna',
'exif-lightsource-2'   => 'Lampada a fluorescenza',
'exif-lightsource-3'   => 'Lampada al tungsteno (a incandescenza)',
'exif-lightsource-4'   => 'Flash',
'exif-lightsource-9'   => 'Bel tempo',
'exif-lightsource-10'  => 'Nuvoloso',
'exif-lightsource-11'  => 'In ombra',
'exif-lightsource-12'  => 'Daylight fluorescent (D 5700 - 7100K)',
'exif-lightsource-13'  => 'Day white fluorescent (N 4600 - 5400K)',
'exif-lightsource-14'  => 'Cool white fluorescent (W 3900 - 4500K)',
'exif-lightsource-15'  => 'White fluorescent (WW 3200 - 3700K)',
'exif-lightsource-17'  => 'Luce standard A',
'exif-lightsource-18'  => 'Luce standard B',
'exif-lightsource-19'  => 'Luce standard C',
'exif-lightsource-20'  => 'Illuminante D55',
'exif-lightsource-21'  => 'Illuminante D65',
'exif-lightsource-22'  => 'Illuminante D75',
'exif-lightsource-23'  => 'Illuminante D50',
'exif-lightsource-24'  => 'Lampada da studio ISO al tungsteno',
'exif-lightsource-255' => 'Altra sorgente luminosa',

'exif-focalplaneresolutionunit-2' => 'pollici',

'exif-sensingmethod-1' => 'Non definito',
'exif-sensingmethod-2' => 'Sensore area colore a 1 chip',
'exif-sensingmethod-3' => 'Sensore area colore a 2 chip',
'exif-sensingmethod-4' => 'Sensore area colore a 3 chip',
'exif-sensingmethod-5' => 'Sensore area colore sequenziale',
'exif-sensingmethod-7' => 'Sensore trilineare',
'exif-sensingmethod-8' => 'Sensore lineare colore sequenziale',

'exif-scenetype-1' => 'Fotografia diretta',

'exif-customrendered-0' => 'Processo normale',
'exif-customrendered-1' => 'Processo personalizzato',

'exif-exposuremode-0' => 'Esposizione automatica',
'exif-exposuremode-1' => 'Esposizione manuale',
'exif-exposuremode-2' => 'Bracketing automatico',

'exif-whitebalance-0' => 'Bilanciamento del bianco automatico',
'exif-whitebalance-1' => 'Bilanciamento del bianco manuale',

'exif-scenecapturetype-0' => 'Standard',
'exif-scenecapturetype-1' => 'Panorama',
'exif-scenecapturetype-2' => 'Ritratto',
'exif-scenecapturetype-3' => 'Notturna',

'exif-gaincontrol-0' => 'Nessuno',
'exif-gaincontrol-1' => 'Enfasi per basso guadagno',
'exif-gaincontrol-2' => 'Enfasi per alto guadagno',
'exif-gaincontrol-3' => 'Deenfasi per basso guadagno',
'exif-gaincontrol-4' => 'Deenfasi per alto guadagno',

'exif-contrast-0' => 'Normale',
'exif-contrast-1' => 'Alto contrasto',
'exif-contrast-2' => 'Basso contrasto',

'exif-saturation-0' => 'Normale',
'exif-saturation-1' => 'Bassa saturazione',
'exif-saturation-2' => 'Alta saturazione',

'exif-sharpness-0' => 'Normale',
'exif-sharpness-1' => 'Minore nitidezza',
'exif-sharpness-2' => 'Maggiore nitidezza',

'exif-subjectdistancerange-0' => 'Sconosciuta',
'exif-subjectdistancerange-1' => 'Macro',
'exif-subjectdistancerange-2' => 'Soggetto vicino',
'exif-subjectdistancerange-3' => 'Soggetto lontano',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'Latitudine Nord',
'exif-gpslatitude-s' => 'Latitudine Sud',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'Longitudine Est',
'exif-gpslongitude-w' => 'Longitudine Ovest',

'exif-gpsstatus-a' => 'Misurazione in corso',
'exif-gpsstatus-v' => 'Misurazione interoperabile',

'exif-gpsmeasuremode-2' => 'Misurazione bidimensionale',
'exif-gpsmeasuremode-3' => 'Misurazione tridimensionale',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'Chilometri orari',
'exif-gpsspeed-m' => 'Miglia orarie',
'exif-gpsspeed-n' => 'Nodi',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => 'Direzione reale',
'exif-gpsdirection-m' => 'Direzione magnetica',

# External editor support
'edit-externally'      => 'Modifica questo file usando un programma esterno',
'edit-externally-help' => 'Per maggiori informazioni consultare le [http://www.mediawiki.org/wiki/Manual:External_editors istruzioni] (in inglese)',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'tutte',
'imagelistall'     => 'tutte',
'watchlistall2'    => 'tutte',
'namespacesall'    => 'Tutti',
'monthsall'        => 'tutti',

# E-mail address confirmation
'confirmemail'             => 'Conferma indirizzo e-mail',
'confirmemail_noemail'     => 'Non è stato indicato un indirizzo e-mail valido nelle proprie [[Special:Preferences|preferenze]].',
'confirmemail_text'        => "Questo sito richiede la verifica dell'indirizzo e-mail prima di poter usare le funzioni connesse all'email. Premere il pulsante qui sotto per inviare una richiesta di conferma al proprio indirizzo; nel messaggio è presente un collegamento che contiene un codice. Visitare il collegamento con il proprio browser per confermare che l'indirizzo e-mail è valido.",
'confirmemail_pending'     => '<div class="error">
Il codice di conferma è già stato spedito via posta elettronica; se l\'account è stato
creato di recente, si prega di attendere l\'arrivo del codice per qualche minuto prima
di tentare di richiederne uno nuovo.
</div>',
'confirmemail_send'        => 'Invia un codice di conferma via e-mail.',
'confirmemail_sent'        => 'Messaggio e-mail di conferma inviato.',
'confirmemail_oncreate'    => "Un codice di conferma è stato spedito all'indirizzo
di posta elettronica indicato. Il codice non è necessario per accedere al sito,
ma è necessario fornirlo per poter abilitare tutte le funzioni del sito che fanno
uso della posta elettronica.",
'confirmemail_sendfailed'  => '{{SITENAME}} non può inviare il messaggio e-mail di conferma. Verificare che il proprio indirizzo e-mail non contenga caratteri non validi.

Messaggio di errore del mailer: $1',
'confirmemail_invalid'     => 'Codice di conferma non valido. Il codice potrebbe essere scaduto.',
'confirmemail_needlogin'   => 'È necessario $1 per confermare il proprio indirizzo e-mail.',
'confirmemail_success'     => "L'indirizzo e-mail è confermato. Ora è possibile eseguire l'accesso e fare pieno uso del sito.",
'confirmemail_loggedin'    => "L'indirizzo e-mail è stato confermato.",
'confirmemail_error'       => 'Errore nel salvataggio della conferma.',
'confirmemail_subject'     => "{{SITENAME}}: richiesta di conferma dell'indirizzo",
'confirmemail_body'        => 'Qualcuno, probabilmente tu stesso dall\'indirizzo IP $1, ha registrato l\'account "$2" su {{SITENAME}} indicando questo indirizzo e-mail.

Per confermare che l\'account ti appartiene veramente e attivare le funzioni relative all\'invio di e-mail su {{SITENAME}}, apri il collegamento seguente con il tuo browser:

$3

Se *non* hai registrato tu l\'account, segui questo collegamento per annullare la conferma dell\'indirizzo e-mail:

$5

Questo codice di conferma scadrà automaticamente alle $4.',
'confirmemail_invalidated' => 'Richiesta di conferma indirizzo e-mail annullata',
'invalidateemail'          => 'Annulla richiesta di conferma e-mail',

# Scary transclusion
'scarytranscludedisabled' => "[L'inclusione di pagine tra siti wiki non è attiva]",
'scarytranscludefailed'   => '[Errore: Impossibile ottenere il template $1]',
'scarytranscludetoolong'  => '[Errore: URL troppo lungo]',

# Trackbacks
'trackbackbox'      => '<div id="mw_trackbacks">
Informazioni di trackback per questa voce:<br />
$1
</div>',
'trackbackremove'   => ' ([$1 Elimina])',
'trackbacklink'     => 'Trackback',
'trackbackdeleteok' => 'Informazioni di trackback eliminate correttamente.',

# Delete conflict
'deletedwhileediting' => "'''Attenzione''': questa pagina è stata cancellata dopo che hai cominciato a modificarla!",
'confirmrecreate'     => "L'utente [[User:$1|$1]] ([[User talk:$1|discussioni]]) ha cancellato questa pagina dopo che hai iniziato a modificarla, per il seguente motivo: ''$2''
Per favore, conferma che desideri veramente ricreare questa pagina.",
'recreate'            => 'Ricrea',

# HTML dump
'redirectingto' => 'Reindirizzamento a [[:$1]]...',

# action=purge
'confirm_purge'        => "Vuoi pulire la cache di questa pagina?  $1

La ''cache'' è un archivio che contiene una copia provvisoria delle pagine web.

Ogni volta che apri una pagina, il software si connette al database e crea al momento la pagina che viene inviata al tuo ''browser'' e visualizzata sul tuo computer. Questo processo impiega tempo e risorse. 

Per le pagine più frequentemente richieste, questo processo risulterebbe troppo oneroso e perciò ingestibile. Per ovviare al problema il software crea automaticamente una copia della pagina che viene conservata per un certo tempo in una ''cache'', una memoria transitoria appositamente dedicata. In questo modo non è necessario effettuare ogni volta il processo di creazione, poichè la pagina è già pronta.

* vantaggi: minor carico di lavoro per il sistema e maggiore velocità
* svantaggi: è possibile che la pagina caricata non sia la versione più recente; la pagina potrebbe essere stata modificata dopo essere stata copiata nella ''cache''

Pertanto, pulire (o aggiornare) la ''cache'' di una pagina, significa assicurarsi di visualizzare la versione più recente.",
'confirm_purge_button' => 'Conferma',

# AJAX search
'searchcontaining' => "Ricerca delle voci che contengono ''$1''.",
'searchnamed'      => "Ricerca delle voci con titolo ''$1''.",
'articletitles'    => "Ricerca delle voci che iniziano con ''$1''",
'hideresults'      => 'Nascondi i risultati',
'useajaxsearch'    => 'Usa la ricerca AJAX',

# Multipage image navigation
'imgmultipageprev' => '← pagina precedente',
'imgmultipagenext' => 'pagina seguente →',
'imgmultigo'       => 'Vai',
'imgmultigoto'     => 'Vai alla pagina $1',

# Table pager
'ascending_abbrev'         => 'cresc',
'descending_abbrev'        => 'decresc',
'table_pager_next'         => 'Pagina successiva',
'table_pager_prev'         => 'Pagina precedente',
'table_pager_first'        => 'Prima pagina',
'table_pager_last'         => 'Ultima pagina',
'table_pager_limit'        => 'Mostra $1 file per pagina',
'table_pager_limit_submit' => 'Vai',
'table_pager_empty'        => 'Nessun risultato',

# Auto-summaries
'autosumm-blank'   => 'Pagina svuotata completamente',
'autosumm-replace' => "Pagina sostituita con '$1'",
'autoredircomment' => 'Redirect alla pagina [[$1]]',
'autosumm-new'     => 'Nuova pagina: $1',

# Size units
'size-bytes' => '$1 byte',

# Live preview
'livepreview-loading' => 'Caricamento in corso…',
'livepreview-ready'   => 'Caricamento in corso… Pronto.',
'livepreview-failed'  => "Errore nella funzione Live preview.
Usare l'anteprima standard.",
'livepreview-error'   => 'Impossibile effettuare il collegamento: $1 "$2"
Usare l\'anteprima standard.',

# Friendlier slave lag warnings
'lag-warn-normal' => "Le modifiche apportate {{PLURAL:$1|nell'ultimo secondo|negli ultimi $1 secondi}} potrebbero non apparire in questa lista.",
'lag-warn-high'   => "A causa di un eccessivo ritardo nell'aggiornamento del server di database, le modifiche apportate {{PLURAL:$1|nell'ultimo secondo|negli ultimi $1 secondi}} potrebbero non apparire in questa lista.",

# Watchlist editor
'watchlistedit-numitems'       => 'La lista degli osservati speciali contiene {{PLURAL:$1|una pagina (e la rispettiva pagina di discussione)|$1 pagine (e le rispettive pagine di discussione)}}.',
'watchlistedit-noitems'        => 'La lista degli osservati speciali è vuota.',
'watchlistedit-normal-title'   => 'Modifica osservati speciali',
'watchlistedit-normal-legend'  => 'Eliminazione di pagine dagli osservati speciali',
'watchlistedit-normal-explain' => 'Di seguito sono elencate tutte le pagine osservate. Per rimuovere una o più pagine dalla lista, selezionare le caselle relative e fare clic sul pulsante "Elimina pagine" in fondo all\'elenco. Si noti che è anche possibile [[Special:Watchlist/raw|modificare la lista in formato testuale]].',
'watchlistedit-normal-submit'  => 'Elimina pagine',
'watchlistedit-normal-done'    => 'Dalla lista degli osservati speciali {{PLURAL:$1|è stata eliminata una pagina|sono state eliminate $1 pagine}}:',
'watchlistedit-raw-title'      => 'Modifica degli osservati speciali in forma testuale',
'watchlistedit-raw-legend'     => 'Modifica testuale osservati speciali',
'watchlistedit-raw-explain'    => 'Di seguito sono elencate tutte le pagine osservate. Per modificare la lista aggiungere o rimuovere i rispettivi titoli, uno per riga. Una volta terminato, fare clic su "Aggiorna la lista" in fondo all\'elenco. Si noti che è anche possibile [[Special:Watchlist/edit|modificare la lista con l\'interfaccia standard]].',
'watchlistedit-raw-titles'     => 'Titoli delle pagine:',
'watchlistedit-raw-submit'     => 'Aggiorna la lista',
'watchlistedit-raw-done'       => 'La lista degli osservati speciali è stata aggiornata.',
'watchlistedit-raw-added'      => '{{PLURAL:$1|È stata aggiunta una pagina|Sono state aggiunte $1 pagine}}:',
'watchlistedit-raw-removed'    => '{{PLURAL:$1|È stata eliminata una pagina|Sono state eliminate $1 pagine}}:',

# Watchlist editing tools
'watchlisttools-view' => 'Visualizza le modifiche pertinenti',
'watchlisttools-edit' => 'Visualizza e modifica la lista degli osservati speciali',
'watchlisttools-raw'  => 'Modifica la lista in formato testo',

# Core parser functions
'unknown_extension_tag' => 'Tag estensione sconosciuto: "$1"',

# Special:Version
'version'                          => 'Versione', # Not used as normal message but as header for the special page itself
'version-extensions'               => 'Estensioni installate',
'version-specialpages'             => 'Pagine speciali',
'version-parserhooks'              => 'Hook del parser',
'version-variables'                => 'Variabili',
'version-other'                    => 'Altro',
'version-mediahandlers'            => 'Gestori di contenuti multimediali',
'version-hooks'                    => 'Hook',
'version-extension-functions'      => 'Funzioni introdotte da estensioni',
'version-parser-extensiontags'     => 'Tag riconosciuti dal parser introdotti da estensioni',
'version-parser-function-hooks'    => 'Hook per funzioni del parser',
'version-skin-extension-functions' => "Funzioni legate all'aspetto grafico (skin) introdotte da estensioni",
'version-hook-name'                => "Nome dell'hook",
'version-hook-subscribedby'        => 'Sottoscrizioni',
'version-version'                  => 'Versione',
'version-license'                  => 'Licenza',
'version-software'                 => 'Software installato',
'version-software-product'         => 'Prodotto',
'version-software-version'         => 'Versione',

# Special:FilePath
'filepath'         => 'Percorso di un file',
'filepath-page'    => 'Nome del file:',
'filepath-submit'  => 'Percorso',
'filepath-summary' => 'Questa pagina speciale restituisce il percorso completo di un file. Le immagini vengono mostrate alla massima risoluzione disponibile, per gli altri tipi di file viene avviato direttamente il programma associato.

Inserire il nome del file senza il prefisso "{{ns:image}}:"',

# Special:FileDuplicateSearch
'fileduplicatesearch'          => 'Ricerca dei file duplicati',
'fileduplicatesearch-summary'  => "Ricerca di eventuali duplicati del file in base al valore di ''hash''.

Inserire il nome del file senza il prefisso \"{{ns:image}}:\"",
'fileduplicatesearch-legend'   => 'Ricerca di un duplicato',
'fileduplicatesearch-filename' => 'Nome del file:',
'fileduplicatesearch-submit'   => 'Ricerca',
'fileduplicatesearch-info'     => '$1 × $2 pixel<br />Dimensioni: $3<br />Tipo MIME: $4',
'fileduplicatesearch-result-1' => 'Non esistono duplicati identici al file "$1".',
'fileduplicatesearch-result-n' => '{{PLURAL:$2|Esiste un duplicato identico|Esistono $2 duplicati identici}} al file "$1".',

# Special:SpecialPages
'specialpages'                   => 'Pagine speciali',
'specialpages-note'              => '----
* Pagine speciali non riservate.
* <span class="mw-specialpagerestricted">Pagine speciali riservate ad alcune categorie di utenti.</span>',
'specialpages-group-maintenance' => 'Resoconti di manutenzione',
'specialpages-group-other'       => 'Altre pagine speciali',
'specialpages-group-login'       => 'Login / registrazione',
'specialpages-group-changes'     => 'Ultime modifiche e registri',
'specialpages-group-media'       => 'File multimediali - caricamento e resoconti',
'specialpages-group-users'       => 'Utenti e diritti',
'specialpages-group-highuse'     => 'Pagine molto usate',
'specialpages-group-pages'       => 'Elenchi di pagine',
'specialpages-group-pagetools'   => 'Strumenti utili per le pagine',
'specialpages-group-wiki'        => 'Strumenti e informazioni sul progetto',
'specialpages-group-redirects'   => 'Pagine speciali di redirect',
'specialpages-group-spam'        => 'Strumenti contro lo spam',

# Special:BlankPage
'blankpage'              => 'Pagina vuota',
'intentionallyblankpage' => 'Questa pagina è lasciata volutamente vuota ed è usata per benchmark, ecc.',

);
