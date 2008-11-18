<?php
/** Líguru (Líguru)
 *
 * @ingroup Language
 * @file
 *
 * @author Dario vet
 * @author Dedee
 * @author Malafaya
 * @author ZeneizeForesto
 */

$fallback = 'it';

$messages = array(
# User preference toggles
'tog-underline'            => 'Sottolineâ i collegamenti',
'tog-justify'              => 'Alliniamento di paragrafi giustificòu',
'tog-showtoolbar'          => 'Fâ vedde a barra de strumenti de modìffica (con JavaScript)',
'tog-rememberpassword'     => "Arregorda a mæ paròlla d'ordine",
'tog-editwidth'            => 'Spaçio pe cangiâ a larghessa pinn-a',
'tog-previewontop'         => "Veddi l'anteprimma de d'äto a-o spaçio pe cangiâ",
'tog-previewonfirst'       => "Veddi l'anteprimma a-o primmo cangiamento",
'tog-enotifwatchlistpages' => "Fammelo savéi via e-mail quande 'na paggina inta mæ lista in osservassion a va cangiaa.",
'tog-enotifusertalkpages'  => "Màndime un messaggio e-mail se gh'é de-e modìffiche inta pagina de discuscion da mæ pagina d'utente.",
'tog-showhiddencats'       => 'Fa vedde e categorîe ascose',

'underline-always' => 'Sempre',
'underline-never'  => 'Mâi',

'skinpreview' => '(Anteprimma)',

# Dates
'sunday'        => 'Domenega',
'monday'        => 'Lunedì',
'tuesday'       => 'Martedì',
'wednesday'     => 'Mäcordì',
'thursday'      => 'Zeuggia',
'friday'        => 'Venardì',
'saturday'      => 'Sabbo',
'sun'           => 'Dom',
'mon'           => 'Lun',
'tue'           => 'Mar',
'wed'           => 'Mäc',
'thu'           => 'Zeu',
'fri'           => 'Ven',
'sat'           => 'Sab',
'january'       => 'Zenâ',
'february'      => 'Frevâ',
'march'         => 'Marso',
'april'         => 'Arvî',
'may_long'      => 'Mazzo',
'june'          => 'Zûgno',
'july'          => 'Lûggio',
'august'        => 'Agosto',
'september'     => 'Settembre',
'october'       => 'Ötobre',
'november'      => 'Novembre',
'december'      => 'Dexembre',
'january-gen'   => 'Zenâ',
'february-gen'  => 'Frevâ',
'march-gen'     => 'Marso',
'april-gen'     => 'Arvî',
'may-gen'       => 'Mazzo',
'june-gen'      => 'Zûgno',
'july-gen'      => 'Lûggio',
'august-gen'    => 'Agosto',
'september-gen' => 'Settembre',
'october-gen'   => 'Ötobre',
'november-gen'  => 'Novembre',
'december-gen'  => 'Dexembre',
'jan'           => 'Zen',
'feb'           => 'Fre',
'mar'           => 'Mar',
'apr'           => 'Arv',
'may'           => 'Maz',
'jun'           => 'Zûg',
'jul'           => 'Lûg',
'aug'           => 'Ago',
'sep'           => 'Set',
'oct'           => 'Öto',
'nov'           => 'Nov',
'dec'           => 'Dex',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|Categorîa|Categorîe}}',
'category_header'                => 'Paggine inta categorîa "$1"',
'subcategories'                  => 'Sottocategorîe',
'category-media-header'          => 'Archivvio inta categorîa "$1"',
'category-empty'                 => "''Pe-o momento 'sta categorîa a no contegne nisciûnn-a paggina ò archivvio multimedia.''",
'hidden-category-category'       => 'Categorîe ascôse', # Name of the category where hidden categories will be listed
'category-subcat-count-limited'  => "'Sta categorîa a contegne {{PLURAL:$1|ûnn-a sottocategorîa, indicaa|$1 sottocategorîe, indicæ}} chì inzû.",
'category-article-count-limited' => "'Sta categorîa a contegne {{PLURAL:$1|'sta paggina|'ste $1 paggine}}.",
'listingcontinuesabbrev'         => 'cont.',

'about'          => 'Informaçioin',
'article'        => 'Pagina de i contenùi',
'newwindow'      => "(A s'ârve inte 'n âtro barcon)",
'cancel'         => 'Scassa',
'qbfind'         => 'Attrêuva',
'qbedit'         => 'Cangia',
'qbpageoptions'  => "Opsioîn de 'sta paggina",
'qbpageinfo'     => 'Informassion inscia paggina',
'qbmyoptions'    => 'E mæ paggine',
'qbspecialpages' => 'Pagine speçiä',
'moredotdotdot'  => 'De ciû...',
'mypage'         => 'A mea pagina',
'mytalk'         => 'Mæ discuscioin',
'anontalk'       => 'Discuscion pe questo indirisso IP',
'navigation'     => 'Navegaçion',
'and'            => 'e',

'errorpagetitle'    => 'Errô',
'returnto'          => 'Tornâ a $1.',
'tagline'           => 'Da {{SITENAME}}',
'help'              => 'Agiùtto',
'search'            => 'Çerca',
'searchbutton'      => 'Çerca',
'go'                => 'Vanni',
'searcharticle'     => 'Vanni',
'history'           => 'Stöja da paggina',
'history_short'     => 'Cronologîa',
'info_short'        => 'Informassion',
'printableversion'  => 'Verscion pe stampâ',
'permalink'         => 'Inganço fisso',
'print'             => 'Stampa',
'edit'              => 'Cangia',
'create'            => 'Creâ',
'editthispage'      => "Modificâ 'sta pagina",
'create-this-page'  => "Crea 'sta paggina",
'delete'            => 'Scassa',
'deletethispage'    => "Scassa 'sta paggina",
'protect'           => 'Proteze',
'protectthispage'   => "Proteze 'sta paggina.",
'unprotect'         => 'Sbloccâ',
'unprotectthispage' => "Levâghe a protession a 'sta paggina",
'newpage'           => 'Nêuva paggina',
'talkpage'          => 'Paggina de discûxon',
'talkpagelinktext'  => 'Ciæti',
'specialpage'       => 'Pagina speçiâ',
'personaltools'     => 'Strûmenti personâli',
'articlepage'       => 'Veddi a voxe',
'talk'              => 'Ciæti',
'views'             => 'Viscite',
'toolbox'           => 'Arneixi',
'projectpage'       => 'Veddi a pagina de o progetto',
'viewtalkpage'      => 'Veddi o ciæto',
'otherlanguages'    => 'In ätre lengue',
'redirectedfrom'    => '(Rediritto da $1)',
'redirectpagesub'   => 'Paggina de rindirissamento',
'lastmodifiedat'    => "Sta pagina a l'è stæta cangiâ l'urtima votta a e $2 do $1.", # $1 date, $2 time
'viewcount'         => "'Sta paggina a l'è stæta vista {{PLURAL:$1|solo 'na vòtta|$1 vòtte}}.",
'protectedpage'     => 'Paggina protezûa',
'jumpto'            => 'Vanni a:',
'jumptonavigation'  => 'Navegassion',
'jumptosearch'      => 'çerca',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'Informaçioin in scia {{SITENAME}}',
'aboutpage'            => 'Project:Informassioîn',
'bugreports'           => 'Danni',
'bugreportspage'       => 'Project:Danni',
'copyright'            => 'O contegnûo o se peu trovâ a $1.',
'copyrightpagename'    => "Diritti d'autô de {{SITENAME}}",
'copyrightpage'        => "{{ns:project}}:Diritti d'autô",
'currentevents'        => 'Attualitæ',
'currentevents-url'    => 'Project:Attualitæ',
'disclaimers'          => 'Avvertense',
'disclaimerpage'       => 'Project:Avvertense generâli',
'edithelp'             => "Agiùtto pe l'ediçion",
'edithelppage'         => 'Help:Modiffica',
'helppage'             => 'Help:Contegnûi',
'mainpage'             => 'Pagina prinçipâ',
'mainpage-description' => 'Paggina prinçipâ',
'policy-url'           => 'Project:Lezzi',
'portal'               => 'Pòrtego da comûnitæ',
'portal-url'           => 'Project:Pòrtego da comûnitæ',
'privacy'              => 'Leze insci dæti privæ',
'privacypage'          => 'Project:Leze insci dæti privæ',

'badaccess'        => "No ti g'hæ o permisso",
'badaccess-group0' => "No ti g'hæ o permisso pe fâ quest'assion.",
'badaccess-group1' => "L'assion che ti vêu fâ a l'è permissa solo a i ûtenti de ûn di grûppi $1.",
'badaccess-group2' => "L'assion che ti vêu fâ a l'è permissa solo a i ûtenti de ûn di grûppi $1.",
'badaccess-groups' => "L'assion che ti vêu fâ a l'è permissa solo a i ûtenti de ûn di grûppi $1.",

'ok'                      => "D'accòrdio",
'retrievedfrom'           => 'Estræto da "$1"',
'youhavenewmessages'      => "Ti gh'æ $1 ($2).",
'newmessageslink'         => 'Messaggi nêuvi',
'newmessagesdifflink'     => 'Differensa co-a revixon preçedente',
'youhavenewmessagesmulti' => "Ti t'æ neuvi messaggi in scia $1",
'editsection'             => 'Modificâ',
'editold'                 => 'Modiffica',
'editsectionhint'         => 'Modificâ a session $1',
'toc'                     => 'Indiçe',
'showtoc'                 => 'Fâ vedde',
'hidetoc'                 => 'Asconde',
'viewdeleted'             => 'Vedde $1?',
'site-rss-feed'           => 'Feed RSS de $1',
'site-atom-feed'          => 'Feed Atom de $1',
'page-rss-feed'           => 'Feed RSS pe "$1"',
'red-link-title'          => '$1 (ancon da scrîe)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'     => 'Voxe',
'nstab-user'     => 'Ûtente',
'nstab-project'  => 'Paggina de servissio',
'nstab-image'    => 'Archivvio',
'nstab-template' => 'Template',
'nstab-help'     => 'Agiûtto',
'nstab-category' => 'Categorîa',

# Main script and global functions
'nosuchactiontext' => "L'URL a no corisponde a 'n comando reconosciûo da-o software MediaWiki",

# General errors
'error'             => 'Errô',
'databaseerror'     => 'Errô da a base de i dæti',
'noconnect'         => "Scûsa! 'Sto scîto o g'ha di problemmi tecnixi e o no pêu connettâse co-a base.<br />
$1",
'cachederror'       => "Questa a l'è una copia da pagina ceh ti te çercoö, e a peu esê vegia.",
'readonly'          => 'Database bloccòu',
'internalerror'     => 'Errô interno',
'filecopyerror'     => 'Non ho potùo copiâ o papê "$1" in te "$2".',
'filedeleteerror'   => 'Non ho potùo scassâ o papê "$1".',
'filenotfound'      => 'Non ho trovoö o papê "$1".',
'badarticleerror'   => "L'açion che ti te veu fâ a non l'è permissa in sta pagina.",
'cannotdelete'      => 'Non çe peu scassâ a pagina o o papê. (o peu ese za stæto scassoö da quarchedun ätro).',
'badtitle'          => "O tittolo o no l'è corretto.",
'badtitletext'      => "O tittolo da paggina çercâa o l'è vêuo, sballiòu o con caratteri no accettæ, oppûre o deriva da 'n errô inti collegamenti tra scîti Wiki diversci o verscioîn in léngue diversce do mæximo scîto.",
'viewsource'        => 'Veddi a fonte',
'viewsourcefor'     => 'de $1',
'protectedpagetext' => "'Sta paggina a l'è stæta protezûa pe impedîghe a modiffica.",
'viewsourcetext'    => "O l'è poscibbile vedde e copiâ o còddice sorgente de 'sta paggina:",

# Login and logout pages
'welcomecreation'            => "== Benvegnùo, $1! ==

O to account o l'è stæto creoö. Non te ascordà de cangiâ e toe preferençe de{{SITENAME}}.",
'yourname'                   => 'Nomme',
'yourpassword'               => 'Pòula segretta:',
'yourpasswordagain'          => 'Ri-scriï a pòula segretta',
'remembermypassword'         => "Arregordâ a mæ paròlla d'ordine",
'yourdomainname'             => 'Indirisso do scito:',
'login'                      => 'Intra',
'nav-login-createaccount'    => 'Intra / Registrate',
'loginprompt'                => "Ti deivi avéi e lesche (''cookies'') abilitæ into têu navigatô pe intrâ in {{SITENAME}}.",
'userlogin'                  => 'Intra / Registrate',
'logout'                     => 'Sciorti',
'userlogout'                 => 'Sciorti',
'nologin'                    => "No ti g'hæ ancon accesso? $1.",
'nologinlink'                => "Creâ 'n conto",
'createaccount'              => "Creâ 'n nêuvo conto",
'gotaccount'                 => "Ti g'hæ zà 'n conto d'accesso? $1.",
'gotaccountlink'             => 'Intra',
'badretype'                  => "E paròlle d'ordine che t'hæ scrîo son despægie.",
'userexists'                 => "O nomme d'ûtente inserîo o l'è zà in ûso.<br />
Pe piaxei prêuva a scellie 'n âtro.",
'youremail'                  => 'Posta elettronega:',
'username'                   => "Nomme d'utente",
'yourrealname'               => 'Nomme vëo:',
'yourlanguage'               => 'Léngoa:',
'yourvariant'                => 'Differensa',
'yournick'                   => 'Nommeaggio:',
'badsig'                     => 'Errô in ta firma; controlla i comandi HTML.',
'badsiglength'               => "O nommeaggio o l'é tròppo lóngo; o dêve avéi meno de $1 caratteri.",
'email'                      => 'Posta elettronega',
'prefs-help-realname'        => '* Nomme vëo (opsionâ): se o se scellie de scrivilo, o sajà dêuviòu pe ascrivighe a paternitæ di contegnûi inviæ.',
'loginerror'                 => "Errô inte l'accesso",
'noname'                     => "O nomme d'ûtente o l'è sballiòu.",
'loginsuccesstitle'          => 'Accesso effettuòu',
'loginsuccess'               => "'''O collegamento a-o server de {{SITENAME}} co-o nomme d'ûtente \"\$1\" o l'è attivo.'''",
'nosuchuser'                 => 'No gh\'è nisciûn ûtente con quello nomme "$1". Verificâ o nomme inserîo ò creâ \'n nêuvo accesso.',
'nosuchusershort'            => 'No gh\'è nisciûn ûtente con quello nomme "<nowiki>$1</nowiki>". Verificâ o nomme inserîo.',
'nouserspecified'            => "O se deive inserî 'n nomme d'ûtente.",
'wrongpassword'              => "Ti gh'æ scrîo 'na paròlla d'ordine sbaliâ. Tenta torna.",
'wrongpasswordempty'         => "No ti g'hæ scrîo nisciûnn-a paròlla d'ordine. Tenta torna.",
'passwordtooshort'           => "A paròlla d'ordine che ti gh'æ misso a no serve òu a l'é tròppo cûrta.
A dêve contegnî mìnimo $1 caratteri e esse diverza da-o teu nómme utente.",
'mailmypassword'             => "Inviâ paròlla d'ordine (password) via e-mail",
'passwordremindertitle'      => "Servissio Password Reminder (nêuva paròlla d'ordine temporannia) de {{SITENAME}}",
'passwordremindertext'       => "Quarchedûn (probabilmente ti, con indirisso IP \$1) o g'ha domandòu l'invîo de 'na nêuva paròlla d'ordine pe l'accesso a {{SITENAME}} (\$4).
A paròlla d'ordine pe l'ûtente \"\$2\" a l'è stæta impostâa a \"\$3\". 
O se conseggia de fâ l'accesso quanto primma e cangiâ a paròlla d'ordine immediatamente.
Se no ti è stæto ti a fâ 'sta domanda, oppûre se ti g'hæ ritrovòu a têu paròlla d'ordine e no ti vêu cangiâla ciû, ti pêu ignorâ 'sto messaggio e andâ avanti ûsando a vegia paròlla d'ordine.",
'noemail'                    => 'No gh\'è nisciûn indirisso e-mail registròu pe l\'ûtente "$1".',
'passwordsent'               => "Ûnn-a nêuva paròlla d'ordine a l'è stæta inviâa a l'indirisso e-mail registròu pe l'ûtente \"\$1\".
Pe piaxei, fa 'n accesso appenn-a ti a ghe reçeivi.",
'blocked-mailpassword'       => "O teu indirisso IP o l'è affirmoö, e pe sta razon o non se peu usâ a funscion de remandâ a pòula segretta.",
'eauthentsent'               => "'N messaggio e-mail de confermassion o l'è stæto inviòu a l'indirisso indicòu.
Pe abilitâ l'invîo de messaggi e-mail pe quest'accesso, o se deive seguî l'istrûssioîn indicæ, coscì ti confermi che ti t'è o legittimo propietâjo de l'indirisso.",
'acct_creation_throttle_hit' => 'O ne dispiâxe, ma ti hæ zà creòu $1 accesci. No ti pêu creâne de ciû!',
'emailauthenticated'         => "O teu indirisso de posta elettronega o l'è stæto autenticoö o $1.",
'emailconfirmlink'           => 'Conferma o teu indirisso de posta elettronega',
'accountcreated'             => 'Graçie pe esëte registroö!!!',
'accountcreatedtext'         => "Utente $1, ti te guägno l'açeiso!",
'loginlanguagelabel'         => 'Lengoa: $1',

# Password reset dialog
'resetpass'           => 'Reverti a pòula segretta',
'resetpass_header'    => 'Reverti a pòula segretta',
'resetpass_forbidden' => "E paròlle d'ordine no se pêuan cangiâ in {{SITENAME}}",

# Edit page toolbar
'bold_sample'     => 'Grascietto',
'bold_tip'        => 'Grascietto',
'italic_sample'   => 'Testo in corsciva',
'italic_tip'      => 'Corscivo',
'link_sample'     => "Nomme de l'inganço",
'link_tip'        => 'Inganço interno',
'extlink_sample'  => "http://www.example.com Nomme de l'inganço",
'extlink_tip'     => 'Collegamento esterno (inclûdde o prefisso http:// )',
'headline_sample' => 'Tittolo',
'headline_tip'    => 'Tittolo de 2° livello',
'math_sample'     => 'Inserî a formûla chì',
'math_tip'        => 'Fórmûla matemattica (LaTeX)',
'nowiki_sample'   => 'Inserî chì o testo sensa formattassion',
'nowiki_tip'      => 'Ignorâ a formattassion wiki',
'image_sample'    => 'Exempio.jpg',
'image_tip'       => 'Immaggine caregaa',
'media_sample'    => 'Exempio.ogg',
'media_tip'       => 'Collegamento a file multimedia',
'sig_tip'         => 'Firma con data e ôa',
'hr_tip'          => 'Linnia orissontâ',

# Edit pages
'summary'                => 'Oggetto',
'subject'                => 'Argomento (tittolo)',
'minoredit'              => 'Cangiamento minô (m)',
'watchthis'              => 'Azzonze a-i osservæ speçiâli',
'savearticle'            => 'Sârva a pagina',
'preview'                => 'Anteprimma',
'showpreview'            => "Veddi l'anteprimma",
'showdiff'               => 'Veddi i cangiamenti',
'anoneditwarning'        => "'''Attension:''' No ti t'hæ registròu. O têu indirisso IP o sajà misso inta stöja di cangiamenti da paggina.",
'summary-preview'        => 'Anteprimma oggetto',
'blockedtitle'           => "L'utente o l'é bloccòu",
'blockedtext'            => "<big>''''Sto nomme d'ûtente ou indirisso IP o l'è stæto bloccòu.'''</big>

O blòcco o l'è stæto fæto da \$1. A raxon dæta a l'è ''\$2''.

* Iniçio de l'affermassion: \$8
* Iniçio de l'affermassion: \$6
* Intervallo de l'affermassion: \$7

O l'è poscibbile contattâ \$1 o 'n âtro [[{{MediaWiki:Grouppage-sysop}}|amministratô]] pe discûtte inscio blòcco.
O no se pêu ûsâ o comando \"Inviâ 'na léttia elettronega a quest'ûtente\" se ti no ti g'hæ 'n indirisso e-mail registròu inte têu [[Special:Preferences|preferense]] e se o no l'è stæto bloccòu ascì.
O têu indirisso IP o l'è \$3, e o têu blòcco ID o l'è #\$5.
Pe piaxei mettighe ûn di doî in tûtte e domande che ti fæ.",
'autoblockedtext'        => "O têu indirisso IP o l'è stæto bloccòu outomaticamente perché o l'ea za ûsòu da 'n âtro ûtente, bloccòu da \$1.
A raxon dæta a l'è stæta:

:''\$2''

* Inissio do blòcco: \$8
* Fin do blòcco: \$6

Ti pêu contattâ \$1 ou 'n âtro 
[[{{MediaWiki:Grouppage-sysop}}|amministratô]] pe parlâ inscio blòcco.

Dagghe a mente a che no ti pêu ûsâ o comando \"manda na littia elettronega a sto utente\" se non ti g'hæ 'n indirisso de posta elettronega registroö in te têu [[Special:Preferences|preferense]] e se o no l'è stæto bloccòu ascì.

O têu blòcco ID o l'è \$5. Pe piaxei metti 'sto ID in tûtte e domande che ti fæ.",
'blockedoriginalsource'  => "A fònte de '''$1''' a l'è chi sotta:",
'blockededitsource'      => "O testo de i '''teu cangiamenti''' a '''$1''' o l'è chi sotta:",
'whitelistedittitle'     => "Bezêugna registrâse pe modificâ 'na pagina.",
'whitelistedittext'      => 'Pe cangia sta pagina devvi $1.',
'loginreqtitle'          => "Besêugna registrâse primma de modificâ 'sta paggina.",
'accmailtitle'           => 'Pòula segretta spedïa',
'accmailtext'            => 'A pòula segretta pe-o utente "$1" a l\'è stæta spedïa a o indirisso $2.',
'newarticle'             => '(Nêuvo)',
'newarticletext'         => "'Sto collegamento o corisponde a 'na paggina che ancon a no l'esciste.

Se o se vêu creâ a paggina òua, o se pêu comensâ a scrive o testo into spassio vêuo chì sotta.
(fâ riferimento a-e [[{{MediaWiki:Helppage}}|paggine d'agiûtto]] pe ciû informassion).

Se o s'ha intròu inte 'sto collegamento pe sbàllio, o basta sciaccâ '''Inderrê''' (Indietro) into navigatô.",
'noarticletext'          => "Inte 'sto momento a paggina çercâa a l'è vêua. O l'è poscibbile [[Special:Search/{{PAGENAME}}|çercâ 'sto tittolo]] inte âtre paggine do scîto oppûre [{{fullurl:{{FULLPAGENAME}}|action=edit}} modificâ a paggina òua].",
'previewnote'            => "<strong>Questa chì a l'è solo 'n'anteprimma; i cangiamenti no son ancon stæti sarvæ!</strong>",
'editing'                => 'Modiffica de $1',
'editingsection'         => 'Modiffica de $1 (session)',
'yourtext'               => 'O teu testo',
'yourdiff'               => 'Differense',
'copyrightwarning'       => "Nota: Tùtte e contribuçioìn a {{SITENAME}} van conscideræ comme rilasciæ drento a-i termini da licensa d'ûso $2 (veddi $1 pe savéine de ciù).
Se no ti veu che i testi teu pêuan esse modificæ da quarchedùn sensa limitaçioìn, no mandâli a {{SITENAME}}.<br />
Inviando o testo ti diciâri, sott'a teu responsabilitæ, ch'o l'é stæto scrîto da ti personalmente oppure ch'o l'é stæto piggiòu da 'na fonte de pùbrico domìnio òu anàlogamente lìbea.<br />
<strong>NO INVIÂ MATERIÂLE COVERTO DA DRÎTI D'AUTÔ SENSA OUTORIZAÇION!</strong>",
'longpagewarning'        => "<strong>ATTENSION: 'Sta paggina chì a g'ha $1 kilobyte; çerti browser porieivan avei di problemmi inta modiffica de-e paggine che s'avvixinn-an o che ecceddan i 32 KB.
Pe piaxei conscidera l'opportûnitæ de soddividde a paggina in sessioîn ciû piccinn-e.</strong>",
'templatesused'          => "Template dêuviæ inte 'sta paggina:",
'templatesusedpreview'   => "Template dêuviæ inte 'st'anteprimma:",
'template-protected'     => '(protezûo)',
'template-semiprotected' => '(semiprotezûo)',
'nocreatetext'           => "A poscibilitæ de creâ nêuve paggine insce {{SITENAME}} a l'è stæta limitâ solo a-i ûtenti registræ.
O se pêu tornâ inderê e modificâ 'na paggina escistente, oppûre [[Special:UserLogin|intrâ ò creâ 'n accesso nêuvo]].",
'recreate-deleted-warn'  => "'''Attension: o se sta pe ricreâ 'na paggina zà scassâa into passòu.'''

O se deive consciderâ se o l'è davvei corretto continuâ avanti a modificâ 'sta paggina.
E relative cancellassioîn son pûbricæ chì sotta:",

# Account creation failure
'cantcreateaccounttitle' => 'Non çe peu registrâ o utente',
'cantcreateaccount-text' => "A registrascion de utenti da questo indirisso IP (<b>$1</b>) a l'è stæta affermaä da [[User:$3|$3]].

A razon dæta a l'è ''$2''",

# History pages
'viewpagelogs'        => "Veddi i log relativi a 'sta paggina.",
'currentrev'          => 'Verscion attuâle',
'revisionasof'        => 'Verscion do $1',
'revision-info'       => 'Verscion do $1, outô: $2',
'previousrevision'    => '← Verscion meno reçente',
'nextrevision'        => 'Revixon ciû nêuva →',
'currentrevisionlink' => 'Ûrtima revixon',
'cur'                 => 'cor',
'next'                => 'Proscimo',
'last'                => 'Ûrtima',
'page_first'          => 'primma',
'page_last'           => 'ûrtima',
'histlegend'          => "Confronto tra verscioîn: selessionâ e cascette corispondenti a-e verscioîn descideræ e schissâ Inviâ oppûre o pomello lì sotta.

Leggenda: (corr) = differense co-a verscion corrente, (prec) = differense co-a verscion preçedente, '''m''' = modiffica minô",
'histfirst'           => 'Primmo',
'histlast'            => 'Ûrtimo',
'historyempty'        => '(vêua)',

# Revision feed
'history-feed-title'          => 'Stöia de e revisioin',
'history-feed-item-nocomment' => '$1 o $2', # user at time

# Diffs
'history-title'           => 'Cronologîa de-e revixoîn de "$1"',
'difference'              => '(Differense fra e revixoîn)',
'lineno'                  => 'Linnia $1:',
'compareselectedversions' => 'Confronta e verscioîn selessionæ',
'editundo'                => 'Annûllâ',
'diff-multi'              => '({{PLURAL:$1|Ûnn-a revixon intermedia no vista|$1 reviscioîn intermedie no viste}}.)',

# Search results
'searchresults'         => 'Resultati da reçerca',
'searchsubtitle'        => "Ti t'è çercoö '''[[:$1]]'''",
'searchsubtitleinvalid' => "Ti t'è çercoö '''$1'''",
'noexactmatch'          => "'''No gh'è nisciûnn-a paggina c'a se ciamme \"\$1\".''' O se pêu [[:\$1|creâla òua]].",
'prevn'                 => 'Preçedenti $1',
'nextn'                 => 'Proscima $1',
'viewprevnext'          => 'Veddi ($1) ($2) ($3).',
'powersearch'           => 'Çerca',

# Preferences page
'preferences'       => 'Preferençe',
'mypreferences'     => 'Mæ preferense',
'changepassword'    => 'Cangiâ a pòula segretta',
'dateformat'        => 'Formato da a data',
'datetime'          => 'Data e oùa',
'saveprefs'         => 'Sarva',
'retypenew'         => "Ripette a nêuva paròlla d'ordine:",
'textboxsize'       => 'Cangia',
'searchresultshead' => 'Çerca',
'timezonelegend'    => 'Oùa',
'allowemail'        => 'Permitti a posta elettronega da ätri utenti',
'default'           => 'Predefinïo',
'files'             => 'Papê',

# Groups
'group-user' => 'Ûtenti',

'grouppage-sysop' => '{{ns:project}}:Amministratoî',

# User rights log
'rightslog' => "Diritti d'ûtente",

# Recent changes
'nchanges'                       => '$1 {{PLURAL:$1|modiffica|modiffiche}}',
'recentchanges'                  => 'Ûrtimi cangiamenti',
'recentchanges-feed-description' => "Questo feed o g'ha e modiffiche ciû reçenti a-i contegnûi do scîto.",
'rcnote'                         => "Chì sotta {{PLURAL:$1|gh'é a modiffica ciù reçente|ghe son e '''$1''' modiffiche ciù reçenti}} de 'sto scîto {{PLURAL:$2|inte ûrtime 24 ôe|inti ûrtimi '''$2''' giorni}}; i dæti son aggiornæ a $3.",
'rcnotefrom'                     => "Chì sotta gh'è i cangiamenti fæti comensando da '''$2''' (scinn-a '''$1''').",
'rclistfrom'                     => 'Fâ vedde e modiffiche apportæ partendo da $1',
'rcshowhideminor'                => '$1 cangiamenti minoi',
'rcshowhidebots'                 => '$1 bot',
'rcshowhideliu'                  => '$1 ûtenti registræ',
'rcshowhideanons'                => '$1 ûtenti anonnimi',
'rcshowhidepatr'                 => '$1 e modiffiche controllæ',
'rcshowhidemine'                 => '$1 i mæ cangiamenti',
'rclinks'                        => 'Fâ vedde e $1 modiffiche ciû reçenti fæte inti ûrtimi $2 giorni<br />$3',
'diff'                           => 'diff',
'hist'                           => 'cron',
'hide'                           => 'Ascondi',
'show'                           => 'Famme vedde',
'minoreditletter'                => 'm',
'newpageletter'                  => 'N',
'boteditletter'                  => 'b',
'rc_categories_any'              => 'Quarsevêuggia',

# Recent changes linked
'recentchangeslinked'          => 'Cangiamenti correlæ',
'recentchangeslinked-title'    => 'Modiffiche correlæ a "$1"',
'recentchangeslinked-noresult' => 'Nisciûn cangiamento a-e paggine collegæ into periodo speçificòu.',
'recentchangeslinked-summary'  => "'Sta paggina a fa vedde e modiffiche ciû reçenti a-e paggine in collegamento.
E paggine in osservassion son dipinte in '''grascietto'''.",

# Upload
'upload'               => "Caregâ 'n archivvio",
'uploadbtn'            => "Carega 'n archivvio",
'uploadlogpage'        => 'Log di archivi careghæ',
'filename'             => 'Nomme do papê',
'filesource'           => 'Reixe:',
'uploadedfiles'        => 'Papê caregæ',
'badfilename'          => 'O nomme do papê o l\'è stæto cangioö in "$1".',
'fileexists'           => "Un papê co sto nomme o existe de zà, pe piaxei da unn'euggiâ a <strong><tt>$1</tt></strong> se non ti tei seguo de voleilo cangiâ.",
'fileexists-forbidden' => 'Un papê co sto nomme o existe de zà, pe piaxei vanni in derrê e carega sto papê co un ätro nomme. [[Image:$1|thumb|center|$1]]',
'savefile'             => 'Sarva o papê',
'uploadedimage'        => 'O s\'ha caregòu "[[$1]]"',
'uploaddisabledtext'   => 'In {{SITENAME}} non se peu caregâ de papê.',
'uploadcorrupt'        => "O papê o gh'à di erroì ò-o gh'à unn'estenscion sbaliâ. Pe piaxei dagghe unn'euggiâ a-o papê e càreghilo tórna.",
'uploadvirus'          => 'O papê gha un virus!! Dettaggi: $1',
'sourcefilename'       => "Nomme do papê d'origine:",
'destfilename'         => 'Nomme do papê de destin:',

'upload-file-error' => 'Errô interno',

'license'   => 'Permisso:',
'nolicense' => 'Nisciûnn-a liçensa indicâa',

# Special:ImageList
'imagelist_search_for' => "Çerca pe nomme de l'imàgine:",
'imgfile'              => 'papê',
'imagelist'            => "Lista d'archivvi",
'imagelist_date'       => 'Dæta',

# Image description page
'filehist'                  => "Cronologîa de l'archivvio",
'filehist-help'             => "Sciacca inscie 'n grûppo data/ôa pe vedde l'archivvio comme o se presentâva into momento indicòu.",
'filehist-current'          => 'Corrente',
'filehist-datetime'         => 'Data/Ôa',
'filehist-user'             => 'Ûtente',
'filehist-dimensions'       => 'Dimensioîn',
'filehist-filesize'         => "Dimension de l'archivvio",
'filehist-comment'          => 'Commenti',
'imagelinks'                => "Collegamenti a l'immaggine",
'linkstoimage'              => "Queste son e pagine che appóntan a st'archivio",
'nolinkstoimage'            => "No gh'è nisciûnn-a paggina collegâa con 'st'archivvio.",
'sharedupload'              => "'St'archivvio o l'è condiviso; sajeiva a dî c'o pêu ese dêuviòu da ciû progetti wiki.",
'shareduploadwiki-linktext' => 'pagina da descriçion do papê',
'noimage'                   => "No gh'è nisciûn archivvio con quello nomme, o se pêu $1.",
'noimage-linktext'          => 'Caregâlo òua',
'uploadnewversion-linktext' => "Carega 'na nêuva verscion de 'st'archivvio chì",

# File deletion
'filedelete-submit' => 'Scassa',

# MIME search
'mimesearch' => 'Çerca MIME',

# List redirects
'listredirects' => 'Lista de rindirissamenti',

# Unused templates
'unusedtemplates' => 'Template no ûtilissæ',

# Random page
'randompage' => 'Pagina a brettio',

# Random redirect
'randomredirect' => 'Ûn rindirissamento a brettîo',

# Statistics
'statistics' => 'Statístiche',

'disambiguations' => 'Paggine de desambiguassion',

'doubleredirects' => 'Rindirissamenti doggi',

'brokenredirects'        => 'Rindirissamenti sballiæ',
'brokenredirectstext'    => 'De sotta unn-a lista de reindirissi a pagine che non existàn:',
'brokenredirects-edit'   => '(cangia)',
'brokenredirects-delete' => '(scassa)',

'withoutinterwiki'         => 'Paggine sensa interwiki',
'withoutinterwiki-summary' => "'Ste paggine chì inzû no g'han nisciûn collegamento co-e verscioîn in âtre lengoe:",

'fewestrevisions' => 'Voxi con meno revixoîn',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|byte|byte}}',
'nlinks'                  => '$1 {{PLURAL:$1|collegamento|collegamenti}}',
'nmembers'                => '$1 {{PLURAL:$1|elemento|elementi}}',
'lonelypages'             => 'Paggine orfane',
'uncategorizedpages'      => 'Paggine sensa categorîa',
'uncategorizedcategories' => 'Categorîe sensa categorîa',
'uncategorizedimages'     => 'Immaggini sensa categorîa',
'uncategorizedtemplates'  => 'Template sensa categorîa',
'unusedcategories'        => 'Categorîe no ûtilissæ',
'unusedimages'            => 'Archivvi no ûtilissæ',
'wantedcategories'        => 'Categorîe domandæ',
'wantedpages'             => 'Paggine domandæ',
'mostlinked'              => 'Paggine ciû collegæ',
'mostlinkedcategories'    => 'Categorîe ciû collegæ',
'mostlinkedtemplates'     => 'Template ciû dêuviæ',
'mostcategories'          => 'Voxi con ciû categorîe',
'mostimages'              => 'Immaggini con ciû collegamenti',
'mostrevisions'           => 'Voxi con ciû revixoîn',
'prefixindex'             => 'Indiçe de-e voxi pe léttie inissiâli',
'shortpages'              => 'Paggine ciû cûrte',
'longpages'               => 'Paggine ciû longhe',
'deadendpages'            => 'Paggine sensa sciortîa',
'protectedpages'          => 'Paggine protette',
'protectedtitles'         => 'Tittoli protezûi',
'listusers'               => "Lista d'ûtenti",
'newpages'                => 'Paggine ciû reçenti',
'ancientpages'            => 'Paggine ciû vëgie',
'move'                    => 'Mescia',
'movethispage'            => "Mescia 'sta paggina",

# Book sources
'booksources'               => 'Fonti',
'booksources-search-legend' => 'Çerca e fonti',
'booksources-isbn'          => 'Codice ISBN:',
'booksources-go'            => 'Vanni',
'booksources-text'          => 'De sotta unn-a lista de ingançi a ätri sciti che vendan neuvi e vegi libbri, e che peuvre avei informaçioin in sci libbri che ti te çerchi',

# Special:Log
'specialloguserlabel'  => 'Ûtente:',
'speciallogtitlelabel' => 'Tittolo:',
'log'                  => 'Log',
'all-logs-page'        => 'Tûtti i registri',
'log-search-submit'    => 'Vanni',
'alllogstext'          => 'Presentaçion unega de tutti i registri do scito {{SITENAME}}.
Ti te peu strinza a vista se ti te çerni un tipo de registro, un nomme de un utente o de pagina.',

# Special:AllPages
'allpages'          => 'Tûtte e paggine',
'alphaindexline'    => 'Da $1 a $2',
'nextpage'          => 'Proscima paggina ($1)',
'prevpage'          => 'Paggina preçedente ($1)',
'allpagesfrom'      => 'Fanni vedde e paggine comensando da:',
'allarticles'       => 'Tûtte e voxi',
'allinnamespace'    => 'Tutte e pagine ($1 namespace)',
'allnotinnamespace' => 'Tutte e pagine (non in $1)',
'allpagesprev'      => 'De primma',
'allpagesnext'      => 'De dòppo',
'allpagessubmit'    => 'Vanni',
'allpagesprefix'    => 'Fanni vedde e paggine che inissian con:',
'allpagesbadtitle'  => 'O titolo pe a pagina o non va ben, o o tegne de i prefissi interlingua o interwiki. O peu tegne un o ciù caratteri non permissi in ti titoli ascì.',
'allpages-bad-ns'   => '"$1" o no ghe in {{SITENAME}}.',

# Special:Categories
'categories'                    => 'Categorîe',
'special-categories-sort-count' => 'ordenâ pe nûmmero',
'special-categories-sort-abc'   => 'ordenâ arfabeticamente',

# Special:ListUsers
'listusers-submit'   => 'Fanni vedde',
'listusers-noresult' => 'Utente non trovöo.',

# E-mail user
'emailuser'       => "Inviâ 'na léttia elettronega a quest'ûtente",
'emailpage'       => "Mandighe 'na léttia elettronega",
'defemailsubject' => '{{SITENAME}} posta elettronega',
'noemailtitle'    => 'Nisciûn conto e-mail',
'emailfrom'       => 'Da',
'emailto'         => 'A',
'emailsubject'    => 'Argumento',
'emailmessage'    => 'Comunicaçion',
'emailsend'       => 'Spèdi',
'emailccme'       => 'Mandame unn-a copia do messagio co unn-a lettìa elettronega.',
'emailsent'       => 'Lettìa elettronega spèdïa',
'emailsenttext'   => "A teua lettìa elettronega a l'è stæta spèdïa.",

# Watchlist
'watchlist'            => 'A mæ lista in osservassion',
'mywatchlist'          => 'Lista in osservaçion',
'watchlistfor'         => "(pe '''$1''')",
'watchnologin'         => "Non ti t'æ entroö",
'watchnologintext'     => 'Devvi [[Special:UserLogin|entrâ]] pe cangiâ a toa lista in osservaçion.',
'addedwatch'           => 'Azzonto a a lista in osservaçion',
'addedwatchtext'       => "A paggina \"[[:\$1]]\" a l'è stæta azzonta a-a pròpia [[Special:Watchlist|lista in osservaçion]]. De chì in avanti, i cangiamenti fæti a-a paggina e a-a sêu discûxon sajàn missi in lista lì; o tittolo da paggina o sajà scrîo in '''grascietto''' inta paggina di [[Special:RecentChanges|ûrtimi cangiamenti]] coscì ti o veddi megio. Se ti vêu eliminâla da-a lista in osservaçion ciû târdi, sciacca \"no seguî\" inscia barra de d'âto.",
'removedwatch'         => 'Scassæ da a lista in osservaçion',
'removedwatchtext'     => 'A paggina "[[:$1]]" a l\'è stæta scassâa da-a têu lista in osservaçion.',
'watch'                => 'Inta lista in osservaçion',
'watchthispage'        => "Vigilâ 'sta paggina",
'unwatch'              => 'No seguî',
'watchlist-details'    => "A lista d'osservassion speçiâ a contegne {{PLURAL:$1|ûnn-a paggina (e a sêu paggina de discûxon)|$1 paggine (e e rispettive paggine de discûxon)}}.",
'watchlistcontains'    => "A lista in osservaçion g'ha $1 {{PLURAL:$1|pagine|pagina}}.",
'wlshowlast'           => 'Famme vedde e ûrtime $1 ôe $2 giorni $3',
'watchlist-show-bots'  => 'Fanni vedde i cangiamenti de i bot',
'watchlist-hide-bots'  => 'Ascondi i cangiamenti di bot',
'watchlist-show-own'   => 'Fanni vedde i mæ cangiamenti',
'watchlist-hide-own'   => 'Ascondi i mæ cangiamenti',
'watchlist-show-minor' => 'Fanni vedde i cangiamenti minô',
'watchlist-hide-minor' => 'Ascondi i cangiamenti minoî',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Inti osservæ speçiâli...',
'unwatching' => 'Scassâ da-i osservæ speçiâli',

'changed'            => 'cangiâ',
'enotif_anon_editor' => 'ûtente anònnimo $1',

# Delete/protect/revert
'deletepage'                  => 'Scassa a paggina',
'exblank'                     => "a paggina a l'ea vêua",
'delete-confirm'              => 'Scassa "$1"',
'delete-legend'               => 'Scassa',
'historywarning'              => "Attension: A paggina c'a se sta pe scassâ a g'ha 'na cronologîa:",
'confirmdeletetext'           => "Ti stæ pe scassâ pe sempre da-o database 'na paggina ò 'n'immaggine, assemme a tûtta a sêu cronologîa. Pe cortexia, conferma che davvei ti vêu andâ avanti con quella cancellassion, che ti capisci perfettamente e conseguense de 'st'assion e che a s'adatta a-e linnie guidda stabilîe in [[{{MediaWiki:Policy-url}}]].",
'actioncomplete'              => 'Açion finïa',
'deletedtext'                 => 'A paggina "<nowiki>$1</nowiki>" a l\'è stæta scassâa. Consûltâ o $2 pe \'na lista de-e paggine scassæ de reçente.',
'deletedarticle'              => 'O s\'ha scassòu "[[$1]]"',
'dellogpage'                  => 'Registro de-e cose scassæ',
'deletecomment'               => 'Raxon pe scassâ',
'deleteotherreason'           => 'Ûn âtro motivo',
'deletereasonotherlist'       => "Ûnn'âtra raxon",
'rollbacklink'                => 'rollback',
'cantrollback'                => "O no se pêu tornâ inderê; l'ûtente ch'à fæto quelle modiffiche o l'è stæto l'ûnico contribûente.",
'alreadyrolled'               => "O no se peû tornâ inderê a-i ûrtimi cangiamenti da pagina [[:$1]]
da [[User:$2|$2]] ([[User talk:$2|Ciæti]]); quarche âtro 
o l'à cangiâ ò o l'è zà tornòu inderê.
L'ûrtimo cangiamento o ghe l'à fæto [[User:$3|$3]] ([[User talk:$3|Ciæti]]).",
'revertpage'                  => 'E modificaçioin de [[Special:Contributions/$2|$2]] ([[User talk:$2|Ciæti]]) son stæte eliminæ; riportæ a verscion de primma de [[User:$1|$1]]', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'protectlogpage'              => 'Protessioîn',
'protectedarticle'            => 'o s\'ha protezûo "[[$1]]"',
'protect-legend'              => 'Confermâ protession',
'protectcomment'              => 'Motivo da protession:',
'protectexpiry'               => 'Scadensa:',
'protect_expiry_invalid'      => 'Scadensa invalida.',
'protect_expiry_old'          => 'Data de scadensa into passòu.',
'protect-unchain'             => 'Scollegâ i permissi de stramûo',
'protect-text'                => "Chì o l'è poscibbile vedde e modificâ o livello de protession pe-a paggina <strong><nowiki>$1</nowiki></strong>.",
'protect-locked-access'       => "No ti g'hæ permisso pe modificâ i livelli de protession da paggina.
Queste son e impostassioîn correnti pe 'sta paggina (<strong>$1</strong>):",
'protect-cascadeon'           => "Pe-o momento 'sta paggina chì a l'è bloccâa perché a l'è inclûsa {{PLURAL:$1|inta paggina indicâa apprêuvo, pe-a quæ|inte paggine indicæ apprêuvo, pe-e quæ}} a l'è attiva a protession recorsciva. O se pêu modificâ o livello de protession individuâle da paggina, ma l'impostassioîn derivanti da-a protession recorsciva no sajàn modificæ.",
'protect-default'             => '(predefinîo)',
'protect-fallback'            => 'Besêugna avei permisso "$1"',
'protect-level-autoconfirmed' => 'Solo ûtenti registræ',
'protect-level-sysop'         => 'Solo amministratoî',
'protect-summary-cascade'     => 'recorsciva',
'protect-expiring'            => 'scadensa: $1 (UTC)',
'protect-cascade'             => 'Protession recorsciva (estende a protession a tûtte e paggine inclûse in questa chì).',
'protect-cantedit'            => "Ti no ti pêu modificâ i livelli de protession pe-a paggina se no ti g'hæ i permissi pe modificâ a paggina mæxima.",
'restriction-type'            => 'Permisso',
'restriction-level'           => 'Livello de restrission',

# Restrictions (nouns)
'restriction-edit' => 'Cangia',
'restriction-move' => 'Mescia',

# Restriction levels
'restriction-level-all' => 'Tutti i livelli',

# Undelete
'undelete'               => 'Repiggio de i dæti: veddi e pagine che son stæte scassæ',
'undeletebtn'            => 'Ristorâ',
'cannotundelete'         => "O repiggio de i dæti o non l'è riuscïo (i peun ese za stæti repiggiæ da quarchedun ätro).",
'undelete-bad-store-key' => "No se peu repiggiâ o papê con a data $1: o papê o l'éja za stæto scassoö.",
'undelete-cleanup-error' => 'Errô repiggiando i dæti do papê "$1".',
'undelete-error-short'   => 'Errô repiggiando i dæti do papê "$1".',
'undelete-error-long'    => 'Ghe son stæti de i errôi cuando se repiggiavan i dæti de o papê:

$1',

# Namespace form on various pages
'namespace'      => 'Namespace:',
'invert'         => 'Invertî a selession',
'blanknamespace' => '(Prinçipâ)',

# Contributions
'contributions' => "Contribussioîn de l'ûtente",
'mycontris'     => 'Mæ contribuçioin',
'contribsub2'   => 'Pe $1 ($2)',
'uctop'         => '(ûrtima pe-a paggina)',
'month'         => 'Partendo da-o meise (e preçedenti):',
'year'          => "Partendo da l'anno (e preçedenti):",

'sp-contributions-newbies-sub' => 'Pe i nêuvi ûtenti',
'sp-contributions-blocklog'    => 'Blocchi',
'sp-contributions-submit'      => 'Çerca',

# What links here
'whatlinkshere'       => 'Cose appunta chì',
'whatlinkshere-title' => "Paggine c'appuntan a $1",
'linklistsub'         => "(Lista de l'ingançi)",
'linkshere'           => "E paggine seguenti appontan a '''[[:$1]]''':",
'nolinkshere'         => "Nisciûnn-a paggina a se collega con '''[[:$1]]'''.",
'isredirect'          => 'Rindirissâ',
'istemplate'          => 'Inclûxon',
'whatlinkshere-prev'  => '{{PLURAL:$1|preçedente|preçedenti $1}}',
'whatlinkshere-next'  => '{{PLURAL:$1|sûccescivo|sûccescivi $1}}',
'whatlinkshere-links' => '← collegamenti',

# Block/unblock
'blockip'                     => "Blocca l'ûtente",
'ipbreason'                   => 'Raxon do blòcco:',
'ipboptions'                  => '2 ôe:2 hours,1 giorno:1 day,3 giorni:3 days,1 settemann-a:1 week,2 settemann-e:2 weeks,1 meise:1 month,3 meixi:3 months,6 meixi:6 months,1 anno:1 year,infinîo:infinite', # display1:time1,display2:time2,...
'badipaddress'                => 'Indirisso IP non valido',
'blockipsuccesssub'           => 'Affermaçion arriescïa',
'blockipsuccesstext'          => "[[Special:Contributions/$1|$1]] o l'è stæto affermoö.
<br />Veddi [[Special:IPBlockList|Lista de i indirissi IP affermæ]] te cangia e affermaçioin.",
'ipblocklist'                 => "Lista de l'indirissi IP e nommi d'ûtenti bloccæ",
'blocklistline'               => "$1, $2 o l'ha affermoö $3 fin a $4",
'anononlyblock'               => 'Non ti tè registroö. Non ti peu fanni de i cangiamenti! (Registräse o non vegne ninte!)',
'emailblock'                  => 'posta elettronega affermaä',
'ipblocklist-empty'           => "A lista de e affermaçioin a l'è veua.",
'blocklink'                   => 'Affermaçion',
'unblocklink'                 => 'sblocca',
'contribslink'                => 'Contribuçioìn',
'autoblocker'                 => 'Affermoö automaticamente perchè o teu indirisso IP o l\'è stæto usöo da "[[User:$1|$1]]" neuvamente. A razon dæta pe affermâ $1 a l\'è stæta:
"$2"',
'blocklogpage'                => 'Affermaçioin',
'blocklogentry'               => "O s'ha bloccòu [[$1]] scinn-a $2 $3",
'blocklogtext'                => "Sta chie a l'è unn-a lista de affermaçioin fæte e levæ.
I indirissi IP affermæ automaticamente non son  consideræ.
Veddi a [[Special:IPBlockList|Lista de i indirissi IP affermæ]] pe e informaçioin neuve.",
'block-log-flags-anononly'    => 'Utenti anonimmi soö',
'block-log-flags-nocreate'    => 'Neuve registrascioin non son permisse',
'block-log-flags-noautoblock' => "O blocco automatego o non l'è attïvo",
'block-log-flags-noemail'     => "A posta elettronega a non l'è attïva",

# Developer tools
'databasenotlocked' => "A base de i dæti a non l'è serrâ.",

# Move page
'move-page-legend'        => 'Mescia a paggina',
'movepagetext'            => "Chì o se pêu dâ 'n nêuvo nomme a 'na paggina, stramûando tûtta a sêu cronologîa a-o nêuvo nomme.
A paggina attuâle a fa outomaticamente 'n rindirissamento a-o nêuvo tittolo.
I collegamenti escistenti no sajàn aggiornæ; veriffica che 'sto stramûo o no l'agge creòu doggi rindirissamenti ò rindirissamenti sballiæ.
A responsabilitæ pe tegnî i collegamenti sempre donde deivan andâ a l'è têu.

A paggina a '''no''' sajà stramûâa se ghe foisse zà ûnn-a co-o nêuvo nomme, a meno c'a no segge vêua ò fæta solo da 'n rindirissamento a-a vegia e a no l'agge verscioîn preçedenti.
In caso de stramûo sballiòu o se pêu tornâ sûbbito a-o vegio tittolo, e o no l'è poscibbile sorvescrive pe errô 'na paggina zà escistente.

'''ATTENSION:'''
'N cangiamento coscì grande o porieiva creâ di controtempi e problemmi, sorvetûtto pe-e paggine ciû viscitæ.
Pensa ben e conseguense de 'sto stramûo primma d'andâ avanti!",
'movepagetalktext'        => "A corispondente paggina de discûxon a sajà stramûâa outomaticamente insemme a-a paggina prinçipâ, '''eççetto inti seguenti câxi''':

* Che o stramûo da paggina o segge tra namespace diversci
* Che inta corispondensa do nêuvo tittolo ghe segge zà 'na paggina de discûxon (no vêua)
* Che a cascetta chì sotta a segge stæta deselessionâa.

Inte 'sti câxi, se o se vêu fâ coscì, o se deive stramûâ ò azzonze manualmente e informassioîn contegnûe inta paggina de discûxon.",
'movearticle'             => 'Stramûâ a paggina',
'newtitle'                => 'Nêuvo tittolo:',
'move-watch'              => 'Azzonze a li osservæ speçiâli',
'movepagebtn'             => 'Stramûâ a paggina',
'pagemovedsub'            => 'Remescio fæto',
'articleexists'           => "Ghe n'æmmo zà 'na paggina con 'sto nomme, oppûre quello che ti g'hæ scelto o no l'è permisso. Cangia nomme.",
'talkexists'              => "'''A paggina a l'è stæta stramûâa correttamente, ma o no l'è stæto poscibbile stramûâ a paggina de discûxon perché ghe n'è zà 'n'âtra co-o nêuvo tittolo. O se deive inserî manualmente i contegnûi de tûtte e doe.'''",
'movedto'                 => 'Stramûâa a',
'movetalk'                => 'Stramûâ anche a paggina de discûxon',
'1movedto2'               => '[[$1]] mesciòu a [[$2]]',
'1movedto2_redir'         => '[[$1]] mescioö a [[$2]] redirect',
'movelogpage'             => 'Lista di remesci',
'movereason'              => 'Raxon',
'revertmove'              => 'Ristorâ',
'delete_and_move'         => 'Scassa e mescia',
'delete_and_move_confirm' => 'Scì, scassa a pagina',
'delete_and_move_reason'  => 'Levoö pe fâ röso pe un remescio',

# Export
'export' => 'Esportâ paggine',

# Namespace 8 related
'allmessages'               => 'Messaggi do scistemma',
'allmessagesname'           => 'Nomme',
'allmessagesdefault'        => 'Testo di default',
'allmessagescurrent'        => 'Testo corrente',
'allmessagestext'           => "Sta chie a l'è unn-a lista de messaggi do scistema in ta MediaWiki.",
'allmessagesnotsupportedDB' => "'''{{ns:special}}:Allmessages''' o non ti te peu vedde, perchè '''\$wgUseDatabaseMessages''' o non l'è attivo.",
'allmessagesfilter'         => 'Çernia de mesasggi:',
'allmessagesmodified'       => 'Vanni vedde söo i cangiamenti',

# Thumbnails
'thumbnail-more'           => 'Ciû grande',
'filemissing'              => 'O papê non ghe!',
'thumbnail_error'          => 'Errô inta creassion da miniatûa: $1',
'thumbnail_invalid_params' => 'Parametri da a imàginetta non validi',

# Import log
'importlogpage' => 'Importassioîn',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'User page',
'tooltip-pt-mytalk'               => 'Mæ discûscioîn',
'tooltip-pt-preferences'          => 'Mæ preferense',
'tooltip-pt-watchlist'            => 'A lista de-e pagine che ti æ sotta osservaçion',
'tooltip-pt-mycontris'            => 'Mæ contribûssioîn',
'tooltip-pt-login'                => "Conseggiæmo a registrassion, scibben a no l'è d'òbbligo.",
'tooltip-pt-logout'               => 'Sciortîa (logout)',
'tooltip-ca-talk'                 => "Vedde e discûscioin insce 'sta paggina.",
'tooltip-ca-edit'                 => "O se pêu modificâ 'sta paggina. Pe piaxei scia dêuvie o pommello d'anteprimma primma de sarvâla.",
'tooltip-ca-addsection'           => "Azzonze 'n commento a 'sta discûscion chì.",
'tooltip-ca-viewsource'           => "'Sta paggina a l'è protetta, ma o se pêu vedde o sêu còddice sorgente.",
'tooltip-ca-protect'              => "Proteze 'sta paggina",
'tooltip-ca-delete'               => "Scassa 'sta paggina",
'tooltip-ca-move'                 => "Sposta 'sta paggina (cangia tittolo)",
'tooltip-ca-watch'                => "Azzonze 'sta paggina a-a têu lista d'osservassion speçiâ",
'tooltip-ca-unwatch'              => "Levâ 'sta paggina d'inta têu lista d'osservassion speçiâ",
'tooltip-search'                  => 'Çerca {{SITENAME}}',
'tooltip-n-mainpage'              => 'Viscita a paggina prinçipâ',
'tooltip-n-portal'                => 'Descrission do progetto, cose che se pêuan fâ, donde trovâ e cose',
'tooltip-n-currentevents'         => "Informassioin insci fæti d'attualitæ.",
'tooltip-n-recentchanges'         => 'E ûrtime modiffiche into scîto',
'tooltip-n-randompage'            => "Fâ vedde 'na paggina a brettîo.",
'tooltip-n-help'                  => "Paggine d'agiûtto",
'tooltip-t-whatlinkshere'         => 'Lista de tûtte e paggine che son collegæ a questa chì.',
'tooltip-t-contributions'         => "Lista de-e contribûssioîn de quest'ûtente",
'tooltip-t-emailuser'             => "Inviâ 'n messaggio e-mail a quest'ûtente",
'tooltip-t-upload'                => 'Caregâ immagini ou archivi multimedia',
'tooltip-t-specialpages'          => 'Lista de tûtte e paggine speçiâli',
'tooltip-ca-nstab-user'           => "Veddi a paggina d'ûtente",
'tooltip-ca-nstab-project'        => 'Veddi a paggina de servissio',
'tooltip-ca-nstab-image'          => "Va a vedde a paggina de l'immaggine",
'tooltip-ca-nstab-template'       => 'Veddi o template',
'tooltip-ca-nstab-help'           => "Veddi a paggina d'agiûtto",
'tooltip-ca-nstab-category'       => 'Veddi a paggina da categorîa',
'tooltip-minoredit'               => 'Segnalâ comme modiffica minô',
'tooltip-save'                    => 'Sârva e modiffiche',
'tooltip-preview'                 => 'Anteprimma de-e modiffiche (conseggiâa, primma de sarvâ!)',
'tooltip-diff'                    => "Ammîa e modiffiche che ti ti gh'æ fæto a-o testo.",
'tooltip-compareselectedversions' => "Ammîa e differense tra e doe verscioîn selessionæ de 'sta paggina chì.",
'tooltip-watch'                   => "Azzonze 'sta paggina a-a têu lista d'osservæ speçiâli",

# Stylesheets
'common.css' => '/** o codiçe css scrïo chie o vegne azzounto in tutte e pagine */',

# Attribution
'anonymous'        => 'Utente anonimmo de {{SITENAME}}',
'lastmodifiedatby' => "Sta pagina a l'è stæta cangiâ l'urtima votta a e $2 do $1 da $3.", # $1 date, $2 time, $3 user

# Browsing diffs
'previousdiff' => '← Differensa preçedente',
'nextdiff'     => 'Proscima diff →',

# Media information
'thumbsize'            => 'Dimescion da a imàginetta:',
'file-info-size'       => '($1 × $2 pixel, dimenscioîn: $3, tippo MIME: $4)',
'file-nohires'         => '<small>No ghe son verscioîn a resolûxon ciû ærta.</small>',
'svg-long-desc'        => "(archivvio in formato SVG, dimensioîn nominâli $1 × $2 pixel, dimension de l'archivvio: $3)",
'show-big-image'       => "Verscion d'ærta resolûxon",
'show-big-image-thumb' => "<small>Dimensioîn de 'st'anteprimma: $1 × $2 pixel</small>",

# Special:NewImages
'newimages' => 'Gallerîa de nêuvi archivvi',
'ilsubmit'  => 'Çerca',
'bydate'    => 'pe dâta',

# Bad image list
'bad_image_list' => "O formato o l'è coscì:
Van conscideræ solo e righe che comensan co-o carattere *.
O primmo inganço insce ògni riga o deiv'ese 'n inganço a 'n'immaggine indesciderâa.
L'ingançi succescivi, inscia mæxima riga, van conscideræ comme eccescioîn (paggine donde l'imaggine a pêu ese reciammâa normalmente).",

# Metadata
'metadata'          => 'Metadati',
'metadata-help'     => "'St'archivvio o contegne informassion adissionâ, fòscia missa da-a fotocamera ou da-o scanner dêuviòu pe creâla ou digitalissâla. Se l'archivvio o l'è stæto modificòu, çerti dettaggi porieivan no corisponde a-e modiffiche apportæ.",
'metadata-expand'   => 'Fâ vedde dettaggi',
'metadata-collapse' => 'Asconde dettaggi',
'metadata-fields'   => "I campi relativi a-i metadati EXIF elencæ inte 'sto messaggio sajàn visti inscia paggina de l'immaggine quande a tabella di metadati a seggie presentâ inta forma breive. Pe l'impostassion predefinîa, i âtri campi sajàn ascoxi.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength", # Do not translate list items

# EXIF tags
'exif-datetime'   => 'Data e öa do cangiamento do papê',
'exif-artist'     => 'Autô',
'exif-copyright'  => "Diritti d'autô de",
'exif-filesource' => 'Reixe do papê',

# External editor support
'edit-externally'      => "Modiffica 'st'archivvio co unn-a applicassion esterna",
'edit-externally-help' => "Pe ciû informassion consûltâ l' [http://www.mediawiki.org/wiki/Manual:External_editors istrûssioîn] (in ingleise)",

# 'all' in various places, this might be different for inflected languages
'watchlistall2' => 'Tùtti',
'namespacesall' => 'Tûtti',
'monthsall'     => 'Tûtti',

# AJAX search
'articletitles' => "Pagine che comensan con ''$1''",
'hideresults'   => 'Ascondi i resultæ',

# Multipage image navigation
'imgmultipageprev' => '← Pagina de primma',
'imgmultipagenext' => 'Proscima pagina →',
'imgmultigo'       => 'Vanni!',

# Table pager
'ascending_abbrev'         => 'cresc',
'table_pager_next'         => 'Proscima pagina',
'table_pager_prev'         => 'Pagina de primma',
'table_pager_first'        => 'Primma pagina',
'table_pager_last'         => 'Urtima pagina',
'table_pager_limit'        => 'Fanni devve $1 elementi pe pagina',
'table_pager_limit_submit' => 'Vanni',
'table_pager_empty'        => 'Nisciun resultato',

# Auto-summaries
'autosumm-blank'   => 'Scassa tutti i contenùi da a pagina',
'autosumm-replace' => "Sostituçion da pagina con '$1'",
'autoredircomment' => 'Reindirissoö a [[$1]]',
'autosumm-new'     => 'Neuva pagina: $1',

# Live preview
'livepreview-loading' => 'Camallando…',
'livepreview-ready'   => 'Camallando… Æmô finïo!',

# Watchlist editing tools
'watchlisttools-view' => 'Veddi e modiffiche pertinenti',
'watchlisttools-edit' => 'Veddi e modiffica a lista',
'watchlisttools-raw'  => 'Modiffica a lista in formato testo',

# Special:Version
'version' => 'Verscion', # Not used as normal message but as header for the special page itself

# Special:SpecialPages
'specialpages' => 'Paggine speçiâli',

);
