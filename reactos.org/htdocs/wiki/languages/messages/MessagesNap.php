<?php
/** Neapolitan (Nnapulitano)
 *
 * @ingroup Language
 * @file
 *
 * @author Carmine Colacino
 * @author Cryptex
 * @author E. abu Filumena
 * @author SabineCretella
 * @author לערי ריינהארט
 */

$fallback = 'it';

$messages = array(
# User preference toggles
'tog-underline'               => "Sottolinia 'e jonte:",
'tog-highlightbroken'         => 'Formatta \'e jonte defettose <a href="" class="new">accussì</a> (oppure: accussì<a href="" class="internal">?</a>).',
'tog-justify'                 => "Alliniamento d''e paracrafe mpare",
'tog-hideminor'               => "Annascunne 'e cagne piccirille  'int'a ll'úrdeme cagne",
'tog-extendwatchlist'         => "Spanne ll'asservate speciale pe fà vedé tutte 'e cagne possíbbele",
'tog-usenewrc'                => 'Urdeme cagne avanzate (JavaScript)',
'tog-numberheadings'          => "Annúmmera automatecamente 'e títule",
'tog-showtoolbar'             => "Aspone 'a barra d''e stromiente 'e cagno (JavaScript)",
'tog-editondblclick'          => "Cagna 'e pàggene cliccanno ddoje vote (JavaScript)",
'tog-editsection'             => "Permette 'e cagnà 'e sezzione cu a jonta [cagna]",
'tog-editsectiononrightclick' => "Permette 'e cangne 'e sezzione cliccanno p''o tasto destro ncopp 'e titule 'e sezzione (JavaScript)",
'tog-showtoc'                 => "Mosta ll'innece pe 'e paggene cu cchiù 'e 3 sezzione",
'tog-rememberpassword'        => "Ricurda 'a registrazzione pe' cchiu sessione",
'tog-editwidth'               => "Larghezza massima d''a casella pe scrivere",

'underline-always' => 'Sèmpe',
'underline-never'  => 'Màje',

# Dates
'sunday'        => 'dumméneca',
'monday'        => 'lunnerì',
'tuesday'       => 'marterì',
'wednesday'     => 'miercurì',
'thursday'      => 'gioverì',
'friday'        => 'viernarì',
'saturday'      => 'sàbbato',
'january'       => 'jennaro',
'february'      => 'frevàro',
'march'         => 'màrzo',
'april'         => 'abbrile',
'may_long'      => 'màjo',
'june'          => 'giùgno',
'july'          => 'luglio',
'august'        => 'aústo',
'september'     => 'settembre',
'october'       => 'ottobbre',
'november'      => 'nuvembre',
'december'      => 'dicèmbre',
'january-gen'   => 'jennaro',
'february-gen'  => 'frevaro',
'march-gen'     => 'màrzo',
'april-gen'     => 'abbrile',
'may-gen'       => 'maggio',
'june-gen'      => 'giùgno',
'july-gen'      => 'luglio',
'august-gen'    => 'aùsto',
'september-gen' => 'settembre',
'october-gen'   => 'ottovre',
'november-gen'  => 'nuvembre',
'december-gen'  => 'dicembre',
'jan'           => 'jen',
'feb'           => 'fre',
'mar'           => 'mar',
'apr'           => 'abb',
'may'           => 'maj',
'jun'           => 'giu',
'jul'           => 'lug',
'aug'           => 'aus',
'sep'           => 'set',
'oct'           => 'ott',
'nov'           => 'nuv',
'dec'           => 'dic',

# Categories related messages
'category_header' => 'Paggene rìnt\'a categurìa "$1"',
'subcategories'   => 'Categurìe secunnarie',

'about'          => 'Nfromma',
'article'        => 'Articulo',
'newwindow'      => "(s'arape n'ata fenèsta)",
'cancel'         => 'Scancèlla',
'qbfind'         => 'Truòva',
'qbedit'         => 'Càgna',
'qbpageoptions'  => 'Chesta paggena',
'qbpageinfo'     => "Nfrummazzione ncopp'â paggena",
'qbmyoptions'    => "'E ppaggene mie",
'qbspecialpages' => 'Pàggene speciàle',
'mypage'         => "'A paggena mia",
'mytalk'         => "'E mmie chiacchieriàte",
'anontalk'       => 'Chiacchierate pe chisto IP',

'help'              => 'Ajùto',
'search'            => 'Truova',
'searchbutton'      => 'Truova',
'go'                => 'Vàje',
'history'           => "Verziune 'e primma",
'history_short'     => 'Cronologgia',
'info_short'        => 'Nfurmazzione',
'printableversion'  => "Verzione pe' stampa",
'permalink'         => 'Jonta permanente',
'edit'              => 'Càgna',
'editthispage'      => 'Càgna chesta paggena',
'delete'            => 'Scancèlla',
'deletethispage'    => 'Scancèlla chésta paggena',
'protect'           => 'Ferma',
'protectthispage'   => 'Ferma chesta paggena',
'unprotect'         => 'Sferma',
'unprotectthispage' => 'Sferma chesta paggena',
'newpage'           => 'Paggena nòva',
'talkpage'          => "Paggena 'e chiàcchiera",
'talkpagelinktext'  => 'Chiàcchiera',
'specialpage'       => 'Paggena speciàle',
'talk'              => 'Chiàcchiera',
'toolbox'           => 'Strumiente',
'imagepage'         => 'Paggena fiùra',
'otherlanguages'    => 'Ate léngue',
'redirectedfrom'    => "(Redirect 'a $1)",
'lastmodifiedat'    => "Urdema cagnamiénto pe' a paggena: $2, $1.", # $1 date, $2 time
'viewcount'         => 'Chesta paggena è stata lètta {{PLURAL:$1|una vòta|$1 vòte}}.',
'jumpto'            => 'Vaje a:',
'jumptonavigation'  => 'navigazione',
'jumptosearch'      => 'truova',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => "'Nfrummazione ncòpp'a {{SITENAME}}",
'aboutpage'            => "Project:'Nfrummazione",
'disclaimers'          => 'Avvertimiènte',
'disclaimerpage'       => 'Project:Avvertimiènte generale',
'edithelp'             => 'Guida',
'helppage'             => 'Help:Ajùto',
'mainpage'             => 'Paggena prencepale',
'mainpage-description' => 'Paggena prencepale',
'portal'               => "Porta d''a cummunetà",
'portal-url'           => "Project:Porta d''a cummunetà",

'badaccess' => "Nun haje 'e premmesse abbastante.",

'newmessageslink'         => "nuove 'mmasciàte",
'newmessagesdifflink'     => "differenze cu 'a revisione precedente",
'youhavenewmessagesmulti' => 'Tiene nuove mmasciate $1',
'editsection'             => 'càgna',
'editold'                 => 'càgna',
'toc'                     => 'Énnece',
'showtoc'                 => 'faje vedé',
'hidetoc'                 => 'annascunne',
'viewdeleted'             => 'Vire $1?',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Articulo',
'nstab-user'      => 'Paggena utente',
'nstab-project'   => "Paggena 'e servizio",
'nstab-image'     => 'Fiura',
'nstab-mediawiki' => "'Mmasciata",
'nstab-help'      => 'Ajùto',
'nstab-category'  => 'Categurìa',

# General errors
'filedeleteerror' => 'Nun se pô scancellà \'o file "$1"',
'cannotdelete'    => "Nun è possibbele scassà 'a paggena o 'a fiura addamannata. (Putria éssere stato già scancellato.)",
'badtitle'        => "'O nnomme nun è jùsto",

# Login and logout pages
'logouttext'                 => "<strong>Site asciùte.</strong><br />
Putite cuntinuà a ausà {{SITENAME}} comme n'utente senza nomme, o si nò putite trasì n'ata vota, cu 'o stesso nomme o cu n'ato nomme.",
'welcomecreation'            => "== Bemmenuto, $1! ==

'O cunto è stato criato currettamente.  Nun scurdà 'e perzonalizzà 'e ppreferenze 'e {{SITENAME}}.",
'remembermypassword'         => 'Allicuordate d"a password',
'yourdomainname'             => "Spiecà 'o dumminio",
'loginproblem'               => "<b>È capetato nu sbaglio a ll'acciesso.</b><br />Pruvate n'ata vota.",
'login'                      => 'Tràse',
'userlogin'                  => "Tràse o cria n'acciesso nuovo",
'logout'                     => 'Jèsce',
'userlogout'                 => 'Jèsce',
'notloggedin'                => 'Acciesso nun affettuato',
'nologin'                    => "Nun haje ancora n'acciesso? $1.",
'nologinlink'                => 'Crialo mmo',
'createaccount'              => 'Cria nu cunto nuovo',
'gotaccount'                 => 'Tiene già nu cunto? $1.',
'gotaccountlink'             => 'Tràse',
'username'                   => 'Nomme utente',
'yourlanguage'               => 'Lengua:',
'loginerror'                 => "Probblema 'e accièsso",
'loginsuccesstitle'          => 'Acciesso affettuato',
'nosuchusershort'            => 'Nun ce stanno utente cu o nòmme "<nowiki>$1</nowiki>". Cuntrolla si scrivìste buòno.',
'nouserspecified'            => "Tiene 'a dìcere nu nomme pricìso.",
'acct_creation_throttle_hit' => 'Ce dispiace, haje già criato $1 utente. Nun ne pô crià ate.',
'accountcreated'             => 'Cunto criato',
'loginlanguagelabel'         => 'Lengua: $1',

# Edit page toolbar
'image_sample' => 'Essempio.jpg',
'image_tip'    => 'Fiura ncuorporata',

# Edit pages
'minoredit'         => 'Chisto è nu cagnamiénto piccerillo',
'watchthis'         => "Tiene d'uocchio chesta paggena",
'savearticle'       => "Sarva 'a paggena",
'preview'           => 'Anteprimma',
'showpreview'       => 'Vere anteprimma',
'showdiff'          => "Fa veré 'e cagnamiente",
'blockededitsource' => "Ccà sotto venono mmustate 'e '''cagnamiente fatte''' â paggena '''$1''':",
'loginreqtitle'     => "Pe' cagnà chesta paggena abbesognate aseguì ll'acciesso ô sito.",
'loginreqlink'      => "aseguì ll'acciesso",
'loginreqpagetext'  => "Pe' veré ate ppaggene abbesognate $1.",
'accmailtitle'      => "'O password è stato mannato.",
'accmailtext'       => '\'A password pe ll\'utente "$1" fuje mannata ô nnerizzo $2.',
'previewnote'       => "<strong>Chesta è sola n'anteprimma; 'e cagnamiénte â paggena NUN songo ancora sarvate!</strong>",
'editing'           => "Cagnamiento 'e $1",
'templatesused'     => "Template ausate 'a chesta paggena:",

# "Undo" feature
'undo-summary' => "Canciella 'o cagnamiento $1 'e [[Special:Contributions/$2|$2]] ([[User talk:$2|Chiàcchiera]])",

# History pages
'currentrev' => "Verzione 'e mmo",
'deletedrev' => '[scancellata]',

# Revision deletion
'rev-delundel' => 'faje vedé/annascunne',

# Search results
'searchresults'    => 'Risultato d&#39;&#39;a recerca',
'searchresulttext' => "Pe sapé de cchiù ncopp'â comme ascia 'a {{SITENAME}}, vere [[{{MediaWiki:Helppage}}|Ricerca in {{SITENAME}}]].",
'noexactmatch'     => "''''A paggena \"\$1\" nun asiste.''' Se pô [[:\$1|criala mmo]].",
'notitlematches'   => "Voce addemannata nun truvata dint' 'e titule 'e articulo",
'notextmatches'    => "Voce addemannata nun truvata dint' 'e teste 'e articulo",
'powersearch'      => 'Truova',

# Preferences page
'mypreferences'   => "Preferenze d''e mie",
'changepassword'  => 'Cagna password',
'prefs-rc'        => 'Urdeme nove',
'prefs-watchlist' => 'Asservate speciale',
'columns'         => 'Culonne:',

# User rights log
'rightsnone' => '(nisciuno)',

# Recent changes
'recentchanges'     => 'Urdeme nove',
'recentchangestext' => "Ncoppa chesta paggena song' appresentate ll'urdeme cagnamiente fatto ê cuntenute d\"o sito.",
'rcnote'            => "Ccà sotto nce songo ll'urdeme {{PLURAL:$1|cangiamiento|'''$1''' cangiamiente}} 'e ll'urdeme {{PLURAL:$2|juorno|'''$2''' juorne}}, agghiuornate a $3.",
'rclistfrom'        => "Faje vedé 'e cagnamiénte fatte a partì 'a $1",
'rcshowhideminor'   => "$1 'e cagnamiénte piccerille",
'rcshowhidebots'    => "$1 'e bot",
'rcshowhideliu'     => "$1 ll'utente reggìstrate",
'rcshowhideanons'   => "$1 ll'utente anonime",
'rcshowhidemine'    => "$1 'e ffatiche mmee",
'rclinks'           => "Faje vedé ll'urdeme $1 cagnamiente dint' ll'urdeme $2 juorne<br />$3",
'hide'              => 'annascunne',
'show'              => 'faje vedé',
'rc_categories_any' => 'Qualònca',

# Recent changes linked
'recentchangeslinked' => 'Cagnamiénte cullegate',

# Upload
'upload'           => 'Careca file',
'fileexists-thumb' => "<center>'''Immagine esistente'''</center>",
'uploadedimage'    => 'ha carecato "[[$1]]"',

# Special:ImageList
'imagelist_name' => 'Nomme',

# Image description page
'filehist-user'    => 'Utente',
'imagelinks'       => 'Jonte ê ffiure',
'noimage-linktext' => 'carrecarlo mmo',

# Random page
'randompage'         => 'Na paggena qualsiase',
'randompage-nopages' => 'Nessuna pagina nel namespace selezionato.',

'disambiguations' => "Paggene 'e disambigua",

'doubleredirects' => 'Redirect duppie',

# Miscellaneous special pages
'nbytes'       => '$1 {{PLURAL:$1|byte|byte}}',
'ncategories'  => '$1 {{PLURAL:$1|categoria|categorie}}',
'nlinks'       => '$1 {{PLURAL:$1|cullegamiento|cullegamiente}}',
'popularpages' => "Paggene cchiù 'speziunate",
'wantedpages'  => 'Paggene cchiù addemannate',
'shortpages'   => 'Paggene curte',
'longpages'    => 'Paggene cchiú longhe',
'newpages'     => 'Paggene cchiù frische',
'move'         => 'Spusta',
'movethispage' => 'Spusta chesta paggena',

# Special:AllPages
'allpages'       => "Tutte 'e ppaggene",
'allarticles'    => "Tutt' 'e vvoce",
'allinnamespace' => "Tutt' 'e ppaggene d&#39;&#39;o namespace $1",

# Special:Categories
'categories'         => 'Categurìe',
'categoriespagetext' => "Lista cumpleta d\"e categurie presente ncopp' 'o sito.",

# Watchlist
'addedwatch'   => 'Aggiunto ai Osservate Speciale tue',
'watch'        => 'Secuta',
'notanarticle' => 'Chesta paggena nun è na voce',

'enotif_newpagetext' => 'Chesta è na paggena nòva.',
'changed'            => 'cagnata',

# Delete/protect/revert
'deletepage'      => 'Scancella paggena',
'excontent'       => "'o cuntenuto era: '$1'",
'excontentauthor' => "'o cuntenuto era: '$1' (e ll'unneco cuntribbutore era '[[Special:Contributions/$2|$2]]')",
'exbeforeblank'   => "'O cuntenuto apprimm' 'a ll'arrevacamento era: '$1'",
'exblank'         => "'a paggena era vacante",
'actioncomplete'  => 'Azzione fernuta',
'deletedtext'     => 'Qauccheruno ha scancellata \'a paggena "<nowiki>$1</nowiki>".  Addumannà \'o $2 pe na lista d"e ppaggene scancellate urdemamente.',
'deletedarticle'  => 'ha scancellato "[[$1]]"',
'dellogpage'      => 'Scancellazione',
'deletionlog'     => 'Log d"e scancellazione',
'deletecomment'   => 'Mutivo d"a scancellazione',
'rollback'        => "Ausa na revizione 'e primma",
'revertpage'      => "Cangiaje 'e cagnamiénte 'e [[Special:Contributions/$2|$2]] ([[User talk:$2|discussione]]), cu â verzione 'e pprimma 'e  [[User:$1|$1]]", # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from

# Undelete
'viewdeletedpage' => "Vìre 'e ppàggine scancellate",

# Namespace form on various pages
'invert' => "abbarruca 'a sceveta",

# Contributions
'contributions' => 'Contribbute utente',
'mycontris'     => 'Mie contribbute',

# What links here
'whatlinkshere'       => 'Paggene ca cullegano a chesta',
'whatlinkshere-title' => 'Paggene ca cullegano a $1',
'nolinkshere'         => "Nisciuna paggena cuntene jonte ca mpuntano a '''[[:$1]]'''.",

# Block/unblock
'blockip'            => 'Ferma utelizzatóre',
'ipadressorusername' => 'Nnerizzo IP o nomme utente',
'ipboptions'         => '2 ore:2 hours,1 juorno:1 day,3 juorne:3 days,1 semmana:1 week,2 semmane:2 weeks,1 mise:1 month,3 mese:3 months,6 mese:6 months,1 anno:1 year,infinito:infinite', # display1:time1,display2:time2,...
'blockipsuccesssub'  => 'Blocco aseguito',
'blocklistline'      => '$1, $2 ha fermato $3 ($4)',
'blocklink'          => 'ferma',
'blocklogpage'       => 'Blocche',
'blocklogentry'      => 'ha fermato "[[$1]]" pe\' nu mumento \'e $2 $3',
'blocklogtext'       => "Chesta è 'a lista d&#39;&#39;e azzione 'e blocco e sblocco utente.  'E nnerizze IP bloccate automaticamente nun nce so'. Addumannà 'a [[Special:IPBlockList|lista IP bloccate]] pp' 'a lista d&#39;&#39;e nnerizze e nomme utente 'o ca blocco nce sta.",

# Move page
'movearticle'             => "Spusta 'a paggena",
'newtitle'                => 'Titulo nuovo:',
'movepagebtn'             => "Spusta 'a paggena",
'articleexists'           => "Na paggena cu chisto nomme asiste già, o pure 'o nomme scegliuto nun è buono.  Scegliere n'ato titulo.",
'movedto'                 => 'spustata a',
'1movedto2'               => 'ha spustato [[$1]] a [[$2]]',
'1movedto2_redir'         => '[[$1]] spustata a [[$2]] trammeto redirect',
'movereason'              => 'Raggióne',
'delete_and_move'         => 'Scancèlla e spusta',
'delete_and_move_confirm' => "Sì, suprascrivi 'a paggena asistente",

# Export
'export' => "Spurta 'e ppaggene",

# Namespace 8 related
'allmessages'         => "'Mmasciate d''o sistema",
'allmessagesname'     => 'Nomme',
'allmessagescurrent'  => "Testo 'e mo",
'allmessagesfilter'   => "Lammecca ncopp' 'e mmasciate:",
'allmessagesmodified' => 'Faje vedé solo chille cagnate',

# Special:Import
'import'                  => 'Mpurta paggene',
'import-interwiki-submit' => 'Mpurta',

# Import log
'import-logentry-upload' => 'ha mpurtato [[$1]] trammeto upload',

# Tooltip help for the actions
'tooltip-pt-logout' => 'Jésce (logout)',
'tooltip-minoredit' => 'Rénne chìsto cagnamiénto cchiù ppiccirìllo.',
'tooltip-save'      => "Sàrva 'e cagnamiénte.",
'tooltip-preview'   => "Primma 'e sarvà, vìre primma chille ca hê cagnàte!",

# Attribution
'others' => 'ate',

# Info page
'numedits'    => "Nummero 'e cagnamiente (articulo): $1",
'numwatchers' => "Nummero 'e asservature: $1",

# Special:NewImages
'noimages' => "Nun nc'è nind' 'a veré.",
'ilsubmit' => 'Truova',

'exif-xyresolution-i' => '$1 punte pe pollice (dpi)',

'exif-meteringmode-0'   => 'Scanusciuto',
'exif-meteringmode-255' => 'Ato',

'exif-lightsource-0'  => 'Scanusciuta',
'exif-lightsource-10' => "'Ntruvulato",
'exif-lightsource-11' => 'Aumbruso',

'exif-gaincontrol-0' => 'Nisciuno',

'exif-subjectdistancerange-0' => 'Scanusciuta',

# External editor support
'edit-externally-help' => "Pe piglià cchiù nfromma veré 'e [http://www.mediawiki.org/wiki/Manual:External_editors struzione] ('n ngrese)",

# 'all' in various places, this might be different for inflected languages
'namespacesall' => 'Tutte',

# E-mail address confirmation
'confirmemail_needlogin' => "Abbesognate $1 pe cunfirmà 'o nnerizzo 'e e-mail d''o vuosto.",
'confirmemail_loggedin'  => "'O nnerizzo 'e e-mail è vàleto",

# Trackbacks
'trackbackremove' => '([$1 Scarta])',

# Delete conflict
'deletedwhileediting' => 'Attenziòne: quaccherùno have scancellàto chesta pàggena prìmma ca tu accuminciàste â scrìvere!',

# AJAX search
'hideresults' => "Annasconne 'e risultate",

# Auto-summaries
'autoredircomment' => 'Redirect â paggena [[$1]]',
'autosumm-new'     => 'Paggena nuova: $1',

# Special:SpecialPages
'specialpages' => 'Paggene speciale',

);
