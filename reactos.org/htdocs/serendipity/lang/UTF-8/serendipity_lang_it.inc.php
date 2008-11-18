<?php # $Id: serendipity_lang_it.inc.php 568 2005-10-18 19:01:10Z garvinhicking $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details
# Translation (c) by Alessandro Pellizzari <alex@amiran.it>
/* vim: set sts=4 ts=4 expandtab : */

@define('LANG_CHARSET', 'UTF-8');
@define('DATE_LOCALES', 'italiano, it, it_IT');
@define('DATE_FORMAT_ENTRY', '%A, %e %B %Y');
@define('DATE_FORMAT_SHORT', '%d-%m-%Y %H:%M');
@define('WYSIWYG_LANG', 'it-utf');
@define('NUMBER_FORMAT_DECIMALS', '2');
@define('NUMBER_FORMAT_DECPOINT', ',');
@define('NUMBER_FORMAT_THOUSANDS', '.');
@define('LANG_DIRECTION', 'ltr');

@define('SERENDIPITY_ADMIN_SUITE', 'Amministrazione di Serendipity');
@define('HAVE_TO_BE_LOGGED_ON', 'Devi fare login per vedere questa pagina');
@define('WRONG_USERNAME_OR_PASSWORD', 'Sembra che tu abbia inserito nome utente o password non validi');
@define('APPEARANCE', 'Aspetto');
@define('MANAGE_STYLES', 'Gestione stili');
@define('CONFIGURE_PLUGINS', 'Configurazione Plugin');
@define('CONFIGURATION', 'Configurazione');
@define('BACK_TO_BLOG', 'Torna al Weblog');
@define('LOGIN', 'Login');
@define('LOGOUT', 'Logout');
@define('LOGGEDOUT', 'Scollegato.');
@define('CREATE', 'Crea');
@define('SAVE', 'Salva');
@define('NAME', 'Nome');
@define('CREATE_NEW_CAT', 'Crea una Nuova Categoria');
@define('I_WANT_THUMB', 'Voglio usare la miniatura nella mia notizia.');
@define('I_WANT_BIG_IMAGE', 'Voglio usare l\'immagine grande nella mia notizia.');
@define('I_WANT_NO_LINK', ' Voglio mostrarla come immagine');
@define('I_WANT_IT_TO_LINK', 'Voglio mostrarla come link a questo URL:');
@define('BACK', 'Indietro');
@define('FORWARD', 'Avanti');
@define('ANONYMOUS', 'Anonimo');
@define('NEW_TRACKBACK_TO', 'Nuovo trackback verso');
@define('NEW_COMMENT_TO', 'Nuovo commento mandato a');
@define('RECENT', 'Recente...');
@define('OLDER', 'Più vecchio...');
@define('DONE', 'Fatto');
@define('WELCOME_BACK', 'bentornato,');
@define('TITLE', 'Titolo');
@define('DESCRIPTION', 'Descrizione');
@define('PLACEMENT', 'Posizionamento');
@define('DELETE', 'Cancella');
@define('SAVE', 'Salva');
@define('SELECT_A_PLUGIN_TO_ADD', 'Scegli il plugin da aggiungere');
@define('UP', 'SU');
@define('DOWN', 'GIU`');
@define('ENTRIES', 'notizie');
@define('NEW_ENTRY', 'Nuova notizia');
@define('EDIT_ENTRIES', 'Modifica notizie');
@define('CATEGORIES', 'Categorie');
@define('WARNING_THIS_BLAHBLAH', "ATTENZIONE:\\nPotrebbe essere necessario molto tempo se molte immagini non hanno le miniature.");
@define('CREATE_THUMBS', 'Ricostruisci miniature');
@define('MANAGE_IMAGES', 'Gestione immagini');
@define('NAME', 'Nome');
@define('EMAIL', 'E-mail');
@define('HOMEPAGE', 'Homepage');
@define('COMMENT', 'Commento');
@define('REMEMBER_INFO', 'Memorizza le Informazioni? ');
@define('SUBMIT_COMMENT', 'Manda Comment');
@define('NO_ENTRIES_TO_PRINT', 'Nessuna notizia da stampare');
@define('COMMENTS', 'Commenti');
@define('ADD_COMMENT', 'Aggiungi Commento');
@define('NO_COMMENTS', 'Nessun commento');
@define('POSTED_BY', 'Scritto da');
@define('ON', 'on');
@define('A_NEW_COMMENT_BLAHBLAH', 'Un nuovo commento è stato mandato al tuo blog "%s", nella notizia intitolata "%s".');
@define('A_NEW_TRACKBACK_BLAHBLAH', 'Un nuovo trackback è stato effettuato alla tua notizia intitolata "%s".');
@define('NO_CATEGORY', 'Nessuna Categoria');
@define('ENTRY_BODY', 'Corpo della Notizia');
@define('EXTENDED_BODY', 'Corpo Esteso');
@define('CATEGORY', 'Categoria');
@define('EDIT', 'Modifica');
@define('NO_ENTRIES_BLAHBLAH', 'Nessuna notizia trovata per la ricerca %s' . "\n");
@define('YOUR_SEARCH_RETURNED_BLAHBLAH', 'La tua ricerca di %s ha fornito %s risultati:');
@define('IMAGE', 'Immagine');
@define('ERROR_FILE_NOT_EXISTS', 'Errore: Il vecchio nome del file non esiste!');
@define('ERROR_FILE_EXISTS', 'Errore: Nuovo nome file già usato, scegline un altro!');
@define('ERROR_SOMETHING', 'Errore: Qualcosa non va.');
@define('ADDING_IMAGE', 'Aggiunta image...');
@define('THUMB_CREATED_DONE', 'Miniatura creata.<br>Fatto.');
@define('ERROR_FILE_EXISTS_ALREADY', 'Errore: Il file esiste già sulla tua macchina!');
@define('ERROR_UNKNOWN_NOUPLOAD', 'Errore sconosciuto, file non inviato. Forse le dimensioni sono superiori al massimo consentito dall\'installazione del server. Chiedi al tuo provider o modifica php.ini per consentire upload più corposi.');
@define('GO', 'Vai!');
@define('NEWSIZE', 'Nuove dimensioni: ');
@define('RESIZE_BLAHBLAH', '<b>Ridimensiona %s</b><p>');
@define('ORIGINAL_SIZE', 'Dimensione originale: <i>%sx%s</i> pixel');
@define('HERE_YOU_CAN_ENTER_BLAHBLAH', '<p>Qui puoi definire le nuove dimensioni dell\'immagine. Se vuoi mantenere le proporzioni inserisci un solo valore e premi TAB, calcolerò automaticamente le nuove dimensioni in modo da non modificare le proporzioni:');
@define('QUICKJUMP_CALENDAR', 'Calendario di accesso veloce');
@define('QUICKSEARCH', 'Ricerca veloce');
@define('SEARCH_FOR_ENTRY', 'Cerca una notizia');
@define('ARCHIVES', 'Archivi');
@define('BROWSE_ARCHIVES', 'Sfoglia gli archivi per mese');
@define('TOP_REFERRER', 'Top Referrers');
@define('SHOWS_TOP_SITES', 'Mostra i maggiori siti che linkano il tuo blog');
@define('TOP_EXITS', 'Top Exits');
@define('SHOWS_TOP_EXIT', 'Mostra dove vanno i tuoi lettori quando escono dal blog');
@define('SYNDICATION', 'Diffusione');
@define('SHOWS_RSS_BLAHBLAH', 'Mostra i link di diffusione RSS');
@define('ADVERTISES_BLAHBLAH', 'Pubblicizza le origini del tuo blog');
@define('HTML_NUGGET', 'Pillola di HTML');
@define('HOLDS_A_BLAHBLAH', 'Infila un pezzo di HTML nella tua barra laterale');
@define('TITLE_FOR_NUGGET', 'Titolo della pillola');
@define('THE_NUGGET', 'La pillola di HTML!');
@define('SYNDICATE_THIS_BLOG', 'Diffondi Questo Blog');
@define('YOU_CHOSE', 'Hai scelto %s');
@define('IMAGE_SIZE', 'Dimensioni dell\'immagine');
@define('IMAGE_AS_A_LINK', 'Inserimento immagine');
@define('POWERED_BY', 'Powered by');
@define('TRACKBACKS', 'Trackbacks');
@define('TRACKBACK', 'Trackback');
@define('NO_TRACKBACKS', 'Nessun Trackbacks');
@define('TOPICS_OF', 'Argomenti di');
@define('VIEW_FULL', 'vedi tutto');
@define('VIEW_TOPICS', 'vedi argomenti');
@define('AT', 'at');
@define('SET_AS_TEMPLATE', 'Imposta come modello');
@define('IN', 'in');
@define('EXCERPT', 'Estratto');
@define('TRACKED', 'Tracciato');
@define('LINK_TO_ENTRY', 'Link alla notizia');
@define('LINK_TO_REMOTE_ENTRY', 'Link alla notizia in remoto');
@define('IP_ADDRESS', 'Indirizzo IP');
@define('USER', 'Utente');
@define('THUMBNAIL_USING_OWN', 'Uso %s come miniatura stessa perché è già abbastanza piccola.');
@define('THUMBNAIL_FAILED_COPY', 'Volevo usare %s come miniatura di sé stessa ma non riesco a copiarla!');
@define('AUTHOR', 'Autore');
@define('LAST_UPDATED', 'Ultimo aggiornamento');
@define('TRACKBACK_SPECIFIC', 'URI specifico di Trackback per questa notizia');
@define('DIRECT_LINK', 'Link diretto a questa notizia');
@define('COMMENT_ADDED', 'Il tuo commento è stato inviato con successo. ');
@define('COMMENT_ADDED_CLICK', 'Clicka %squi per tornare%s ai commenti, e %squi per chiudere%s questa finestra.');
@define('COMMENT_NOT_ADDED_CLICK', 'Clicka %squi per tornare%s ai commenti, e %squi per chiudere%s questa finestra.');
@define('COMMENTS_DISABLE', 'Non consentire commenti a questa notizia');
@define('COMMENTS_ENABLE', 'Consenti commenti a questa notizia');
@define('COMMENTS_CLOSED', 'L\'autore non consente commenti a questa notizia');
@define('EMPTY_COMMENT', 'Il tuo commento non conteneva nulla, per favore %storna indietro%s e riprova');
@define('ENTRIES_FOR', 'Notizie per %s');
@define('DOCUMENT_NOT_FOUND', 'Il documento %s non è stato trovato.');
@define('USERNAME', 'Nome utente');
@define('PASSWORD', 'Password');
@define('AUTOMATIC_LOGIN', 'Salva informazioni');
@define('SERENDIPITY_INSTALLATION', 'Installazione di Serendipity');
@define('LEFT', 'sinistra');
@define('RIGHT', 'destra');
@define('HIDDEN', 'nascosto');
@define('REMOVE_TICKED_PLUGINS', 'Rimuovi i plugin selezionati');
@define('SAVE_CHANGES_TO_LAYOUT', 'Salva i cambiamenti al layout');
@define('COMMENTS_FROM', 'Commenti da');
@define('ERROR', 'Errore');
@define('ENTRY_SAVED', 'La tua notizia è stata salvata');
@define('DELETE_SURE', 'Sei sicuro di voler cancellare #%s in modo permanente?');
@define('NOT_REALLY', 'Non proprio...');
@define('DUMP_IT', 'Cestinalo!');
@define('RIP_ENTRY', 'Addio notizia #%s');
@define('CATEGORY_DELETED_ARTICLES_MOVED', 'Categoria #%s cancellata. Vecchi articoli spostati nella categoria #%s');
@define('CATEGORY_DELETED', 'Categoria #%s cancellata.');
@define('INVALID_CATEGORY', 'Nessuna categoria selezionata per la cancellazione');
@define('CATEGORY_SAVED', 'Categoria salvata');
@define('SELECT_TEMPLATE', 'Seleziona il modello che vuoi usare per il tuo blog');
@define('ENTRIES_NOT_SUCCESSFULLY_INSERTED', 'Notizie non inserite!');
@define('YES', 'Sì');
@define('NO', 'No');
@define('USE_DEFAULT', 'Default');
@define('CHECK_N_SAVE', 'Controlla &amp; salva');
@define('DIRECTORY_WRITE_ERROR', 'Impossibile scrivere nella directory %s. Controllare i permessi.');
@define('DIRECTORY_CREATE_ERROR', 'La directory %s non esiste e non può essere creata. Per favore crearla manualmente');
@define('DIRECTORY_RUN_CMD', '&nbsp;-&gt; esegui <i>%s %s</i>');
@define('CANT_EXECUTE_BINARY', 'Impossibile eseguire il binario %s');
@define('FILE_WRITE_ERROR', 'Impossibile scrivere sul file %s.');
@define('FILE_CREATE_YOURSELF', 'Per favore creare il file manualmente o controllare i permessi');
@define('COPY_CODE_BELOW', '<br />* Copiare il codice qui sotto e metterlo in %s nella vostra cartella %s :<b><pre>%s</pre></b>' . "\n");
@define('WWW_USER', 'Cambiare www con l\'utente con cui gira apache (es. nobody).');
@define('BROWSER_RELOAD', 'Una volta eseguito, premete il bottone "Ricarica" del vostro browser.');
@define('DIAGNOSTIC_ERROR', 'Sono stati rilevati degli errori durante il controllo dei dati inseriti:');
@define('SERENDIPITY_NOT_INSTALLED', 'Serendipity non è ancora installato. per favore <a href="%s">installatelo</a> ora.');
@define('INCLUDE_ERROR', 'errore di serendipity: impossibile includere %s - terminato.');
@define('DATABASE_ERROR', 'errore di serendipity: impossibile connettersi al database - terminato.');
@define('CREATE_DATABASE', 'Creazione del database di default...');
@define('ATTEMPT_WRITE_FILE', 'Tentativo di scrittura sul file %s...');
@define('WRITTEN_N_SAVED', 'Configuratione scritta e salvata');
@define('IMAGE_ALIGNMENT', 'Allineamento immagine');
@define('ENTER_NEW_NAME', 'Inserisci il nuovo nome per: ');
@define('RESIZING', 'Ridimensionamento');
@define('RESIZE_DONE', 'Fatto (ridimensionate %s immagini).');
@define('SYNCING', 'Sincronizzazione del database con la cartella delle immagini');
@define('SYNC_DONE', 'Fatto (Sincronizzate %s immagini).');
@define('FILE_NOT_FOUND', 'Impossibile trovare il file <b>%s</b>, forse è già stato cancellato?');
@define('ABORT_NOW', 'Interrompi subito');
@define('REMOTE_FILE_NOT_FOUND', 'File non trovato sul server remoto, sei sicuro che l\'URL: <b>%s</b> sia correttoi?');
@define('FILE_FETCHED', '%s scaricato come %s');
@define('FILE_UPLOADED', 'File %s inviato con successo come %s');
@define('WORD_OR', 'O');
@define('SCALING_IMAGE', 'Ridimensionamento di %s a %s x %s px');
@define('KEEP_PROPORTIONS', 'Mantieni le proporzioni');
@define('REALLY_SCALE_IMAGE', 'Vuoi veramente riscalare l\'immagine? Questa operazione non può essere annullata!');
@define('TOGGLE_ALL', 'Espandi tutto');
@define('TOGGLE_OPTION', 'Inverti l\'opzione');
@define('SUBSCRIBE_TO_THIS_ENTRY', 'Iscriviti a questa notizia');
@define('UNSUBSCRIBE_OK', "%s è stato disiscritto dalla notizia");
@define('NEW_COMMENT_TO_SUBSCRIBED_ENTRY', 'Nuovo commento alla notizia "%s" a cui sei iscritto');
@define('SUBSCRIPTION_MAIL', "Ciao %s,\n\nC'è un nuovo commento alla notizia che stai tenendo d'occhio su \"%s\", intitolata \"%s\"\nIl nome dell'autore è : %s\n\nPuoi trovare la notizia qui: %s\n\nPuoi disiscriverti clickando su questo link: %s\n");
@define('SUBSCRIPTION_TRACKBACK_MAIL', "Ciao %s,\n\nUn nuovo trackback è stato aggiunto alla notizia che stai tenendo d'occhio su \"%s\", intitolata \"%s\"\nIl nome dell'autore è: %s\n\nPuoi trovare la notizia qui: %s\n\nPuoi disiscriverti clickando su questo link: %s\n");
@define('SIGNATURE', "\n-- \n%s powered by Serendipity.\nIl miglior blog del mondo, puoi usarlo anceh tu.\nGuarda <http://s9y.org> per sapere come.");
@define('SYNDICATION_PLUGIN_091', 'RSS 0.91 feed');
@define('SYNDICATION_PLUGIN_10', 'RSS 1.0 feed');
@define('SYNDICATION_PLUGIN_20', 'RSS 2.0 feed');
@define('SYNDICATION_PLUGIN_20c', 'RSS 2.0 commenti');
@define('SYNDICATION_PLUGIN_ATOM03', 'ATOM 0.3 feed');
@define('SYNDICATION_PLUGIN_MANAGINGEDITOR', 'Campo "managingEditor"');
@define('SYNDICATION_PLUGIN_WEBMASTER',  'Campo "webMaster"');
@define('SYNDICATION_PLUGIN_BANNERURL', 'Immagine per il feed RSS');
@define('SYNDICATION_PLUGIN_BANNERWIDTH', 'Larghezza Immagine');
@define('SYNDICATION_PLUGIN_BANNERHEIGHT', 'Altezza Immagine');
@define('SYNDICATION_PLUGIN_WEBMASTER_DESC',  'Indirizzo e-mail del webmaster, se disponibile. (vuoto: nascosto) [RSS 2.0]');
@define('SYNDICATION_PLUGIN_MANAGINGEDITOR_DESC', 'Indirizzo e-mail dell\'editore, se disponibile. (vuoto: nascosto) [RSS 2.0]');
@define('SYNDICATION_PLUGIN_BANNERURL_DESC', 'URL di un\'immagine GIF/JPEG/PNG, se disponibile. (vuoto: serendipity-logo)');
@define('SYNDICATION_PLUGIN_BANNERWIDTH_DESC', 'in pixel, max. 144');
@define('SYNDICATION_PLUGIN_BANNERHEIGHT_DESC', 'in pixel, max. 400');
@define('SYNDICATION_PLUGIN_TTL', 'Campo "ttl" (time-to-live)');
@define('SYNDICATION_PLUGIN_TTL_DESC', 'Numero di minuti dopo i quali il tuo blog non dovrebbe essere tenuto in cache da altri siti/applicazioni (vuoto: nascosto) [RSS 2.0]');
@define('SYNDICATION_PLUGIN_PUBDATE', 'Campo "pubDate"');
@define('SYNDICATION_PLUGIN_PUBDATE_DESC', 'Il campo "pubDate" deve essere incluso in un canale RSS, per mostrare la data di inserimento dell\'ultima notizia?');
@define('CONTENT', 'Contenuto');
@define('TYPE', 'Tipo');
@define('DRAFT', 'Bozza');
@define('PUBLISH', 'Pubblica');
@define('PREVIEW', 'Anteprima');
@define('DATE', 'Data');
@define('DATE_FORMAT_2', 'Y-m-d H:i'); // Needs to be ISO 8601 compliant for date conversion!
@define('DATE_INVALID', 'Attenzione: La data specificata non è valida. Deve essere nel formato AAAA-MM-GG OO:MM .');
@define('CATEGORY_PLUGIN_DESC', 'Mostra la lista delle categorie.');
@define('ALL_AUTHORS', 'Tutti gli autori');
@define('CATEGORIES_TO_FETCH', 'Categorie da scaricare');
@define('CATEGORIES_TO_FETCH_DESC', 'Scarica le categorie di quale autore?');
@define('PAGE_BROWSE_ENTRIES', 'Pagina %s di %s, in totale %s notizie');
@define('PREVIOUS_PAGE', 'pagina precedente');
@define('NEXT_PAGE', 'pagina seguente');
@define('ALL_CATEGORIES', 'Tutte le categorie');
@define('DO_MARKUP', 'Effettua trasformazioni del Markup');
@define('GENERAL_PLUGIN_DATEFORMAT', 'Formato data');
@define('GENERAL_PLUGIN_DATEFORMAT_BLAHBLAH', 'Il formato della data della notizia, usando variabili di strftime() del PHP. (Default: "%s")');
@define('ERROR_TEMPLATE_FILE', 'Impossibile aprire il file di modello, per favore aggiornare serendipity!');
@define('ADVANCED_OPTIONS', 'Opzioni avanzate');
@define('EDIT_ENTRY', 'Modifica notizia');
@define('HTACCESS_ERROR', 'Per controllare l\'installazione del tuo webserver, serendipity ha bisogno di scrivere nel file ".htaccess". Questo non è stato possibile a causa di permessi errati. Per favore sistema i permessi in questo modo: <br />&nbsp;&nbsp;%s<br />e ricarica questa pagina.');
@define('SIDEBAR_PLUGINS', 'Plugin per le barre laterali');
@define('EVENT_PLUGINS', 'Plugin di Evento');
@define('SORT_ORDER', 'Ordinamento');
@define('SORT_ORDER_NAME', 'Nome file');
@define('SORT_ORDER_EXTENSION', 'Estensione del file');
@define('SORT_ORDER_SIZE', 'Dimensioni del file');
@define('SORT_ORDER_WIDTH', 'Larghezza immagine');
@define('SORT_ORDER_HEIGHT', 'Altezza immagine');
@define('SORT_ORDER_DATE', 'Data di upload');
@define('SORT_ORDER_ASC', 'Crescente');
@define('SORT_ORDER_DESC', 'Decrescente');
@define('THUMBNAIL_SHORT', 'Mini');
@define('ORIGINAL_SHORT', 'Orig.');
@define('APPLY_MARKUP_TO', 'Applica markup a %s');
@define('CALENDAR_BEGINNING_OF_WEEK', 'Inizio settimana');
@define('SERENDIPITY_NEEDS_UPGRADE', 'Serendipity ha rilevato che la configurazione si riferisce alla versione %s, mentre serendipity è installato nella versione %s, è necessario fare l\'upgrade! <a href="%s">Clicka qui</a>');
@define('SERENDIPITY_UPGRADER_WELCOME', 'Salve, e benvenuto alla procedura di upgrade di Serendipity.');
@define('SERENDIPITY_UPGRADER_PURPOSE', 'Sono qui per aiutarti a fare l\'upgrade della tua installazione di Serendipity %s .');
@define('SERENDIPITY_UPGRADER_WHY', 'Vedi questo messaggio perché hai appena installato Serendipity %s, ma non hai ancora aggiornato l\'installazione del database per questa versione');
@define('SERENDIPITY_UPGRADER_DATABASE_UPDATES', 'Database update (%s)');
@define('SERENDIPITY_UPGRADER_FOUND_SQL_FILES', 'Ho trovato questi file .sql che hanno bisogno di essere caricati prima di continuare col funzionamento di Serendipity');
@define('SERENDIPITY_UPGRADER_VERSION_SPECIFIC',  'Procedure specifiche di versione');
@define('SERENDIPITY_UPGRADER_NO_VERSION_SPECIFIC', 'Nessuna procedura specifica della versione trovata');
@define('SERENDIPITY_UPGRADER_PROCEED_QUESTION', 'Vuoi eseguire le procedure indicate?');
@define('SERENDIPITY_UPGRADER_PROCEED_ABORT', 'No, le eseguirò manualmente');
@define('SERENDIPITY_UPGRADER_PROCEED_DOIT', 'Sí, eseguile');
@define('SERENDIPITY_UPGRADER_NO_UPGRADES', 'Sembra che tu non abbia bisogno della procedura di upgrade');
@define('SERENDIPITY_UPGRADER_CONSIDER_DONE', 'Considera Serendipity uggiornato');
@define('SERENDIPITY_UPGRADER_YOU_HAVE_IGNORED', 'Hai ignorato la procedura di upgrade di  Serendipity, per favore controlla che il tuo database sia correttamente installato e che le funzioni periodiche vengano lanciate');
@define('SERENDIPITY_UPGRADER_NOW_UPGRADED', 'La tua installazione di Serendipity è ora aggiornata alla versione %s');
@define('SERENDIPITY_UPGRADER_RETURN_HERE', 'Puoi tornare al tuo blog clickando  %squi%s');
@define('MANAGE_USERS', 'gestione Utenti');
@define('CREATE_NEW_USER', 'Crea nuovo utente');
@define('CREATE_NOT_AUTHORIZED', 'Non puoi modificare utenti con lo stesso tuo livello');
@define('CREATE_NOT_AUTHORIZED_USERLEVEL', 'Non puoi creare utenti di livello superiore al tuo');
@define('CREATED_USER', 'Un nuovo utente %s è stato creato');
@define('MODIFIED_USER', 'Le caratteristiche dell\'utente %s sono state modificate');
@define('USER_LEVEL', 'Livello utente');
@define('DELETE_USER', 'Stai per cancellare l\'utente #%d %s. Sei convinto? Questo impedirà la visualizzazione delle notizie scritte da lui sulla pagina principale.');
@define('DELETED_USER', 'Utente #%d %s cancellato.');
@define('LIMIT_TO_NUMBER', 'Quante notizie vuoi mostrare?');
@define('ENTRIES_PER_PAGE', 'notizie per pagina');
@define('XML_IMAGE_TO_DISPLAY', 'Pulsante XML');
@define('XML_IMAGE_TO_DISPLAY_DESC','I link ai feed XML saranno indicati da questa immagine. Lasciare vuoto per l\'immagine di default, inserire \'none\' per disabilitarla.');

@define('DIRECTORIES_AVAILABLE', 'Nella lista di sotto-directory disponibili puoi clickare sul nome di una directory per creare una nuova directory al suo interno.');
@define('ALL_DIRECTORIES', 'tutte le directory');
@define('MANAGE_DIRECTORIES', 'Gestione directory');
@define('DIRECTORY_CREATED', 'La directory <strong>%s</strong> è stata creata.');
@define('PARENT_DIRECTORY', 'Directory superiore');
@define('CONFIRM_DELETE_DIRECTORY', 'Sei sicuro di voler cancellare tutto il contenuto della directory %s?');
@define('ERROR_NO_DIRECTORY', 'Errore: La directory %s non esiste');
@define('CHECKING_DIRECTORY', 'Controllo dei file nella directory %s');
@define('DELETING_FILE', 'Cancellazione del file %s...');
@define('ERROR_DIRECTORY_NOT_EMPTY', 'Impossibile rimuovere directory non vuote. Attivare "Forzare la cancellazione" se volete rimuoverli e rimandare il form. I file esistenti sono:');
@define('DIRECTORY_DELETE_FAILED', 'Cancellazione della directory %s fallita. Controllare i permessi o i messaggi soprastanti.');
@define('DIRECTORY_DELETE_SUCCESS', 'Directory %s cancellata con successo.');
@define('SKIPPING_FILE_EXTENSION', 'File evitato: Estensione mancante in %s.');
@define('SKIPPING_FILE_UNREADABLE', 'File evitato: %s non è leggibile.');
@define('FOUND_FILE', 'Trovato file nuovo/modificato: %s.');
@define('ALREADY_SUBCATEGORY', '%s è già una sotto-categoria di %s.');
@define('PARENT_CATEGORY', 'Categoria superiore');
@define('IN_REPLY_TO', 'In risposta a');
@define('TOP_LEVEL', 'Primo Livello');
@define('SYNDICATION_PLUGIN_GENERIC_FEED', 'feed %s');
@define('PERMISSIONS', 'Permessi');
@define('SETTINGS_SAVED_AT', 'Le nuove impostazioni sono state salvate in %s');

/* DATABASE SETTINGS */
@define('INSTALL_CAT_DB', 'Impostazioni Database');
@define('INSTALL_CAT_DB_DESC', 'Qui puoi inserire le informazioni sul database. Serendipity ne ha bisogno per funzionare');
@define('INSTALL_DBTYPE', 'Tipo Database');
@define('INSTALL_DBTYPE_DESC', 'Tipo di Database');
@define('INSTALL_DBHOST', 'Host Database');
@define('INSTALL_DBHOST_DESC', 'Il nome dell\'host del server database');
@define('INSTALL_DBUSER', 'Utente Database');
@define('INSTALL_DBUSER_DESC', 'Il nome utente con cui collegarsi al database');
@define('INSTALL_DBPASS', 'Password Database');
@define('INSTALL_DBPASS_DESC', 'La password dell\'utente indicato');
@define('INSTALL_DBNAME', 'Nome Database');
@define('INSTALL_DBNAME_DESC', 'Il nome del tuo database');
@define('INSTALL_DBPREFIX', 'Prefisso Tabelle Database');
@define('INSTALL_DBPREFIX_DESC', 'Prefisso per i nomi delle tabelle, per esempio serendipity_');

/* PATHS */
@define('INSTALL_CAT_PATHS', 'Percorsi');
@define('INSTALL_CAT_PATHS_DESC', 'Percorsi a cartelle e file essenziali. Non dimenticare la barra alla fine del nome delle directory!');
@define('INSTALL_FULLPATH', 'Percorso completo');
@define('INSTALL_FULLPATH_DESC', 'Il percorso completo e assoluto all\'installazione di serendipity');
@define('INSTALL_UPLOADPATH', 'Percorso Upload');
@define('INSTALL_UPLOADPATH_DESC', 'Tutti gli upload andranno qui, relativo al \'Percorso completo\' - tipicamente \'uploads/\'');
@define('INSTALL_RELPATH', 'Percorso relativo');
@define('INSTALL_RELPATH_DESC', 'Percorso a serendipity nel browser, tipicamente \'/serendipity/\'');
@define('INSTALL_RELTEMPLPATH', 'Percorso relativo dei modelli');
@define('INSTALL_RELTEMPLPATH_DESC', 'Il percorso alla cartella contenente i modelli - relativa a \'percorso relativo\'');
@define('INSTALL_RELUPLOADPATH', 'Percorso relativo degli upload');
@define('INSTALL_RELUPLOADPATH_DESC', 'Percorso agli upload per il browser - relativo a \'percorso relativo\'');
@define('INSTALL_URL', 'URL del blog');
@define('INSTALL_URL_DESC', 'URL di base all\'installazione di serendipity');
@define('INSTALL_INDEXFILE', 'File indice');
@define('INSTALL_INDEXFILE_DESC', 'Il nome del file indice di serendipity');

/* Generel settings */
@define('INSTALL_CAT_SETTINGS', 'Impostazioni generali');
@define('INSTALL_CAT_SETTINGS_DESC', 'Personalizza il comportamento di Serendipity');
@define('INSTALL_USERNAME', 'Nome utente di amministrazione');
@define('INSTALL_USERNAME_DESC', 'Nome utente per l\'accesso come amministratore');
@define('INSTALL_PASSWORD', 'Password di amministrazione');
@define('INSTALL_PASSWORD_DESC', 'Password per l\'accesso come ammninistratore');
@define('INSTALL_EMAIL', 'e-mail amministratore');
@define('INSTALL_EMAIL_DESC', 'E-mail dell\'amministratore del blog');
@define('INSTALL_SENDMAIL', 'Invia mail all\'amministratore?');
@define('INSTALL_SENDMAIL_DESC', 'Vuoi che l\'amministratore riceva e-mail quando vengono scritti commenti alle notizie?');
@define('INSTALL_SUBSCRIBE', 'Consenti iscrizione alle notizie?');
@define('INSTALL_SUBSCRIBE_DESC', 'Consenti agli utenti di iscriversi alle notizie e quindi di ricevere un\'e-mail quando vengono scritti commenti a quelle notizie');
@define('INSTALL_BLOGNAME', 'Nome del Blog');
@define('INSTALL_BLOGNAME_DESC', 'Il titolo del tuo blog');
@define('INSTALL_BLOGDESC', 'Descrizione del Blog');
@define('INSTALL_BLOGDESC_DESC', 'La descrizione del tuo blog');
@define('INSTALL_LANG', 'Lingua');
@define('INSTALL_LANG_DESC', 'Seleziona la lingua del tuo blog');

/* Appearance and options */
@define('INSTALL_CAT_DISPLAY', 'Aspetto e opzioni');
@define('INSTALL_CAT_DISPLAY_DESC', 'Personalizza l\'aspetto e il funzionamento di Serendipity');
@define('INSTALL_WYSIWYG', 'Usa un editor WYSIWYG');
@define('INSTALL_WYSIWYG_DESC', 'Vuoi usare l\'editor WYSIWYG? (Funziona con IE5+, parzialmente con Mozilla 1.3+)');
@define('INSTALL_XHTML11', 'Forza l\'aderenza a XHTML 1.1');
@define('INSTALL_XHTML11_DESC', 'Vuoi forzare l\'aderenza allo standard XHTML 1.1 (potrebbe causare problemi nel backend e nel frontend con i browser di vecchia generazione)');
@define('INSTALL_POPUP', 'Abilita l\'uso di finestre popup');
@define('INSTALL_POPUP_DESC', 'Vuoi usare dei popup per i commenti, i trackback, ecc.?');
@define('INSTALL_EMBED', 'Hai inglobato serendipity?');
@define('INSTALL_EMBED_DESC', 'Se vuoi inglobare serendipity in una normale pagina, imposta questa opzione a vero, in modo da scartare gli header e scrivere solo i contenuti. Puoi usare i normali header della tua pagina web. Leggi il file README per avere più informazioni!');
@define('INSTALL_TOP_AS_LINKS', 'Mostra i Top Exit/Referrer come link?');
@define('INSTALL_TOP_AS_LINKS_DESC', '"no": Exit e Referrer sono mostrati come puro testo per evitare spam su google. "sì": Exit e Referrer vengono mostrati come link. "default": Usa le impostazioni globali (raccomandato).');
@define('INSTALL_BLOCKREF', 'Referers bloccati');
@define('INSTALL_BLOCKREF_DESC', 'Ci sono particolari host che non vuoi mostrare nella lista dei referer? Separa la lista dei nomi di host con un \';\' e, nota bene, gli host vengono bloccati anche come sotto-stringhe!');
@define('INSTALL_REWRITE', 'Riscrittura URL');
@define('INSTALL_REWRITE_DESC', 'Seleziona quale regola vuoi per la generazione degli URL. Abilitare la riscrittura degli URL genera URL più belli e rende il blog meglio indicizzabile dai motori di ricerca come google. Il webserver deve supportare almeno mod_rewrite o "AllowOverride All" per la tua directory di serendipity. L\'impostazione di default è l\'auto-riconoscimento');

/* Imageconversion Settings */
@define('INSTALL_CAT_IMAGECONV', 'Impostazione per la conversione di immagini');
@define('INSTALL_CAT_IMAGECONV_DESC', 'Inserisci informazioni generali su come serendipity deve trattare con le immagini');
@define('INSTALL_IMAGEMAGICK', 'Usa Imagemagick');
@define('INSTALL_IMAGEMAGICK_DESC', 'Hai image magick installato e vuoi usarlo per ridimensionare le immagini?');
@define('INSTALL_IMAGEMAGICKPATH', 'Percorso al binario \'convert\'');
@define('INSTALL_IMAGEMAGICKPATH_DESC', 'Percorso completo e nome dell\'eseguibile \'convert\' di ImageMagick');
@define('INSTALL_THUMBSUFFIX', 'Suffisso miniature');
@define('INSTALL_THUMBSUFFIX_DESC', 'Le miniature avranno un nome nel formato: originale.[suffisso].est');
@define('INSTALL_THUMBWIDTH', 'Dimensioni delle miniature');
@define('INSTALL_THUMBWIDTH_DESC', 'Larghezza massima stabilita per le miniature auto-generate');

/* Personal details */
@define('USERCONF_CAT_PERSONAL', 'I tuoi dettagli personali');
@define('USERCONF_CAT_PERSONAL_DESC', 'Modifica i tuoi dettagli personali');
@define('USERCONF_USERNAME', 'Il tuo nome utente');
@define('USERCONF_USERNAME_DESC', 'Il nome utente che usi per collegarti al blog');
@define('USERCONF_PASSWORD', 'La tua password');
@define('USERCONF_PASSWORD_DESC', 'La password che usi per collegarti al blog');
@define('USERCONF_EMAIL', 'Il tuo indirizzo e-mail');
@define('USERCONF_EMAIL_DESC', 'Il tuo indirizzo e-mail personale');
@define('USERCONF_SENDCOMMENTS', 'Manda avvisi di commenti?');
@define('USERCONF_SENDCOMMENTS_DESC', 'Vuoi ricevere un\'e-mail quando vengono scritti commenti sui tuoi articoli?');
@define('USERCONF_SENDTRACKBACKS', 'manda avvisi di trackback?');
@define('USERCONF_SENDTRACKBACKS_DESC', 'Vuoi ricevere e-mail quando vengono aggiunti dei trackback ai tuoi articoli?');
@define('USERCONF_ALLOWPUBLISH', 'Permessi: Pubblicazione notizie?');
@define('USERCONF_ALLOWPUBLISH_DESC', 'A questo utente è consentito pubblicare notizie?');
@define('SUCCESS', 'Successo');
@define('POWERED_BY_SHOW_TEXT', 'Mostra "Serendipity" come testo');
@define('POWERED_BY_SHOW_TEXT_DESC', 'Mostra "Serendipity Weblog" come testo');
@define('POWERED_BY_SHOW_IMAGE', 'Mostra "Serendipity" con un logo');
@define('POWERED_BY_SHOW_IMAGE_DESC', 'Mostra il logo di Serendipity');
@define('PLUGIN_ITEM_DISPLAY', 'Dove deve essere mostrato l\'oggetto?');
@define('PLUGIN_ITEM_DISPLAY_EXTENDED', 'Solo notizia estesa');
@define('PLUGIN_ITEM_DISPLAY_OVERVIEW', 'Solo pagina riassuntiva');
@define('PLUGIN_ITEM_DISPLAY_BOTH', 'Sempre');
@define('RSS_IMPORT_CATEGORY', 'Usa questa categoria per le notizie importate non corrispondenti');

@define('COMMENTS_WILL_BE_MODERATED', 'I commenti sottoposti sono soggetti a moderazione prima di essere mostrati.');
@define('YOU_HAVE_THESE_OPTIONS', 'Sono disponibili le seguenti opzioni:');
@define('THIS_COMMENT_NEEDS_REVIEW', 'Attenzione: Questo commento deve essere approvato prima di essere mostrato');
@define('DELETE_COMMENT', 'Cancella commento');
@define('APPROVE_COMMENT', 'Approva commento');
@define('REQUIRES_REVIEW', 'Richiede revisione');
@define('COMMENT_APPROVED', 'Il commento #%s è stato approvato con successo');
@define('COMMENT_DELETED', 'Il commento #%s è stato cancellato con successo');
@define('COMMENTS_MODERATE', 'Commenti e Traceback a questa notizia richiedono moderazione');
@define('THIS_TRACKBACK_NEEDS_REVIEW', 'Attenzione: Questo trackback richiede approvazione prima di essere mostrato');
@define('DELETE_TRACKBACK', 'Cancella trackback');
@define('APPROVE_TRACKBACK', 'Approva trackback');
@define('TRACKBACK_APPROVED', 'Il trackback #%s è stato approvato con successo');
@define('TRACKBACK_DELETED', 'Il trackback #%s è stato cancellato con successo');
@define('VIEW', 'Vedi');
@define('COMMENT_ALREADY_APPROVED', 'Il commento #%s sembra già essere stato approvato');
@define('COMMENT_EDITED', 'Il commento selezionato è stato modificato');
@define('HIDE', 'Nascondi');
@define('VIEW_EXTENDED_ENTRY', 'Continua a leggere "%s"');
@define('TRACKBACK_SPECIFIC_ON_CLICK', 'Questo link non dovrebbe essere clickato. Contiene l\\\'URI di trackback per questa notizia. Puoi usare questo URI per mandare pingback e trackback dal tuo blog a questa notizia. Per copiare il link, clickaci col tasto destro e seleziona "Copia Collegamento".');
@define('PLUGIN_SUPERUSER_HTTPS', 'Usa https per il  login');
@define('PLUGIN_SUPERUSER_HTTPS_DESC', 'Imposta il link di login a una locazione https. Il webserver deve supportare questa configurazione!');
@define('INSTALL_SHOW_EXTERNAL_LINKS', 'Rendi clickabili i link esterni?');
@define('INSTALL_SHOW_EXTERNAL_LINKS_DESC', '"no": i link esterni non verificati (Top Exit, Top Referrer, Commenti degli utenti) non vengono mostrati/vengono mostrati come puro testo dove possibile, per evitare spam di google. (raccomandato). "Sì": I link esterni non verificati appaiono come link. Può essere reimpostato dalla configurazione dei plugin delle barre laterali!');
@define('PAGE_BROWSE_COMMENTS', 'Pagina %s di %s, in totale %s commenti');
@define('FILTERS', 'Filtri');
@define('FIND_ENTRIES', 'Trova notizie');
@define('FIND_COMMENTS', 'Trova commenti');
@define('FIND_MEDIA', 'Trova media');
@define('FILTER_DIRECTORY', 'Directory');
@define('SORT_BY', 'Ordina per');
@define('TRACKBACK_COULD_NOT_CONNECT', 'Nessun Trackback inviato: Impossibile connettersi a %s sulla porta %d');
@define('MEDIA', 'Media');
@define('MEDIA_LIBRARY', 'Libreria Media');
@define('ADD_MEDIA', 'Aggiungi media');
@define('ENTER_MEDIA_URL', 'Inserisci l\'URL di un file da scaricare:');
@define('ENTER_MEDIA_UPLOAD', 'Seleziona un file da inviare:');
@define('SAVE_FILE_AS', 'Salva il file come:');
@define('STORE_IN_DIRECTORY', 'Salva il file nella directory: ');
@define('ADD_MEDIA_BLAHBLAH', '<b>Aggiungi un file al deposito di media:</b><p>Qui puoi inviare file media, o dirmi da dove prenderli in qualche parte del web! Se non hai l\'immagine giusta, <a href="http://images.google.com" target="_blank">cerca su google</a> quella che preferisci, i risultati spesso sono utili e divertenti :)<p><b>Seleziona il metodo:</b><br>');
@define('MEDIA_RENAME', 'Rinomina questo file');
@define('IMAGE_RESIZE', 'Ridimensiona questa immagine');
@define('MEDIA_DELETE', 'Cancella questo file');
@define('FILES_PER_PAGE', 'File per pagina');
@define('CLICK_FILE_TO_INSERT', 'Clicka sul file che vuoi inserire:');
@define('SELECT_FILE', 'Seleziona il file da inserire');
@define('MEDIA_FULLSIZE', 'Dimensioni piene');
@define('CALENDAR_BOW_DESC', 'Il giorno della settimana che dovrebbe essere considerato il primo. Il default è Lunedì');
@define('SUPERUSER', 'Amministrazione del Blog');
@define('ALLOWS_YOU_BLAHBLAH', 'Fornisce un link nella barra laterare per l\'amministrazione del blog');
@define('CALENDAR', 'Calendario');
@define('SUPERUSER_OPEN_ADMIN', 'Apri amministrazione');
@define('SUPERUSER_OPEN_LOGIN', 'Apri schermo di login');
@define('INVERT_SELECTIONS', 'Inverti la selezione');
@define('COMMENTS_DELETE_CONFIRM', 'Sei sicuro di voler cancellare i commenti selezionati?');
@define('COMMENT_DELETE_CONFIRM', 'Sei sicuro di voler cancellare il commento #%d, scritto da %s?');
@define('DELETE_SELECTED_COMMENTS', 'Cancella i commenti selezionati');
@define('VIEW_COMMENT', 'Vedi commento');
@define('VIEW_ENTRY', 'Vedi notizia');
@define('DELETE_FILE_FAIL' , 'Impossibile cancellare il file <b>%s</b>');
@define('DELETE_THUMBNAIL', 'Cancellata miniatura dell\'immagine <b>%s</b>');
@define('DELETE_FILE', 'Cancellato il file <b>%s</b>');
@define('ABOUT_TO_DELETE_FILE', 'Stai per cancellare <b>%s</b><br />Se stai usando questo file in altre notizie, questo causerà link o immagini "morti"<br />Sei sicuro di voler procedere?<br /><br />');
@define('TRACKBACK_SENDING', 'Invio del trackback all\'URI %s...');
@define('TRACKBACK_SENT', 'Trackback inviato con successo');
@define('TRACKBACK_FAILED', 'Trackback fallito: %s');
@define('TRACKBACK_NOT_FOUND', 'Nessun URI di trackback trovato.');
@define('TRACKBACK_URI_MISMATCH', 'L\'URI di trackback auto-rilevato non corrisponde con il nostro URI di destinazione.');
@define('TRACKBACK_CHECKING', 'Controllo di <u>%s</u> per possibili trackback...');
@define('TRACKBACK_NO_DATA', 'La destinazione non contiene dati');
@define('TRACKBACK_SIZE', 'L\'URI di destinazione eccede le dimensioni massime per i file di %s byte.');
@define('COMMENTS_VIEWMODE_THREADED', 'Per argomento');
@define('COMMENTS_VIEWMODE_LINEAR', 'Cronologicamente');
@define('DISPLAY_COMMENTS_AS', 'Mostra commenti');
@define('COMMENTS_FILTER_SHOW', 'Show'); // Translate
@define('COMMENTS_FILTER_ALL', 'All'); // Translate
@define('COMMENTS_FILTER_APPROVED_ONLY', 'Only approved'); // Translate
@define('COMMENTS_FILTER_NEED_APPROVAL', 'Pending approval'); // Translate
@define('RSS_IMPORT_BODYONLY', 'Put all imported text in the "body" section and do not split up into "extended entry" section.'); // Translate
@define('SYNDICATION_PLUGIN_FULLFEED', 'Show full articles with extended body inside RSS feed'); // Translate
@define('MT_DATA_FILE', 'Movable Type data file'); // Translate
@define('FORCE', 'Force'); // Translate
@define('CREATE_AUTHOR', 'Create author \'%s\'.'); // Translate
@define('CREATE_CATEGORY', 'Create category \'%s\'.'); // Translate
@define('MYSQL_REQUIRED', 'You must have the MySQL extension in order to perform this action.'); // Translate
@define('COULDNT_CONNECT', 'Could not connect to MySQL database: %s.'); // Translate
@define('COULDNT_SELECT_DB', 'Could not select database: %s.'); // Translate
@define('COULDNT_SELECT_USER_INFO', 'Could not select user information: %s.'); // Translate
@define('COULDNT_SELECT_CATEGORY_INFO', 'Could not select category information: %s.'); // Translate
@define('COULDNT_SELECT_ENTRY_INFO', 'Could not select entry information: %s.'); // Translate
@define('COULDNT_SELECT_COMMENT_INFO', 'Could not select comment information: %s.'); // Translate
@define('WEEK', 'Week'); // Translate
@define('WEEKS', 'Weeks'); // Translate
@define('MONTHS', 'Months'); // Translate
@define('DAYS', 'Days'); // Translate
@define('ARCHIVE_FREQUENCY', 'Calendar item frequency'); // Translate
@define('ARCHIVE_FREQUENCY_DESC', 'The calendar interval to use between each item in the list'); // Translate
@define('ARCHIVE_COUNT', 'Number of items in the list'); // Translate
@define('ARCHIVE_COUNT_DESC', 'The total number of months, weeks or days to display'); // Translate
@define('BELOW_IS_A_LIST_OF_INSTALLED_PLUGINS', 'Below is a list of installed plugins'); // Translate
@define('SIDEBAR_PLUGIN', 'sidebar plugin'); // Translate
@define('EVENT_PLUGIN', 'event plugin'); // Translate
@define('CLICK_HERE_TO_INSTALL_PLUGIN', 'Click here to install a new %s'); // Translate
@define('VERSION', 'version'); // Translate
@define('INSTALL', 'Install'); // Translate
@define('ALREADY_INSTALLED', 'Already installed'); // Translate
@define('SELECT_A_PLUGIN_TO_ADD', 'Select the plugin which you wish to install'); // Translate
@define('INSTALL_OFFSET', 'Server time Offset'); // Translate
@define('STICKY_POSTINGS', 'Sticky Postings'); // Translate
@define('INSTALL_FETCHLIMIT', 'Entries to display on frontpage'); // Translate
@define('INSTALL_FETCHLIMIT_DESC', 'Number of entries to display for each page on the frontend'); // Translate
@define('IMPORT_ENTRIES', 'Import data'); // Translate
@define('EXPORT_ENTRIES', 'Export entries'); // Translate
@define('IMPORT_WELCOME', 'Welcome to the Serendipity import utility'); // Translate
@define('IMPORT_WHAT_CAN', 'Here you can import entries from other weblog software applications'); // Translate
@define('IMPORT_SELECT', 'Please select the software you wish to import from'); // Translate
@define('IMPORT_PLEASE_ENTER', 'Please enter the data as requested below'); // Translate
@define('IMPORT_NOW', 'Import now!'); // Translate
@define('IMPORT_STARTING', 'Starting import procedure...'); // Translate
@define('IMPORT_FAILED', 'Import failed'); // Translate
@define('IMPORT_DONE', 'Import successfully completed'); // Translate
@define('IMPORT_WEBLOG_APP', 'Weblog application'); // Translate
@define('EXPORT_FEED', 'Export full RSS feed'); // Translate
@define('STATUS', 'Status after import'); // Translate
@define('IMPORT_GENERIC_RSS', 'Generic RSS import'); // Translate
@define('ACTIVATE_AUTODISCOVERY', 'Send Trackbacks to links found in the entry'); // Translate
@define('WELCOME_TO_ADMIN', 'Welcome to the Serendipity Administration Suite.'); // Translate
@define('PLEASE_ENTER_CREDENTIALS', 'Please enter your credentials below.'); // Translate
@define('ADMIN_FOOTER_POWERED_BY', 'Powered by Serendipity %s and PHP %s'); // Translate
@define('INSTALL_USEGZIP', 'Use gzip compressed pages'); // Translate
@define('INSTALL_USEGZIP_DESC', 'To speed up delivery of pages, we can compress the pages we send to the visitor, given that his browser supports this. This is recommended'); // Translate
@define('INSTALL_SHOWFUTURE', 'Show future entries'); // Translate
@define('INSTALL_SHOWFUTURE_DESC', 'If enabled, this will show all entries in the future on your blog. Default is to hide those entries and only show them if the publish date has arrived.'); // Translate
@define('INSTALL_DBPERSISTENT', 'Use persistent connections'); // Translate
@define('INSTALL_DBPERSISTENT_DESC', 'Enable the usage of persistent database connections, read more <a href="http://php.net/manual/features.persistent-connections.php" target="_blank">here</a>. This is normally not recommended'); // Translate
@define('NO_IMAGES_FOUND', 'No images found'); // Translate
@define('PERSONAL_SETTINGS', 'Personal Settings'); // Translate
@define('REFERER', 'Referer'); // Translate
@define('NOT_FOUND', 'Not found'); // Translate
@define('NOT_WRITABLE', 'Not writable'); // Translate
@define('WRITABLE', 'Writable'); // Translate
@define('PROBLEM_DIAGNOSTIC', 'Due to a problematic diagnostic, you cannot continue with the installation before the above errors are fixed'); // Translate
@define('SELECT_INSTALLATION_TYPE', 'Select which installation type you wish to use'); // Translate
@define('WELCOME_TO_INSTALLATION', 'Welcome to the Serendipity Installation'); // Translate
@define('FIRST_WE_TAKE_A_LOOK', 'First we will take a look at your current setup and attempt to diagnose any compatibility problems'); // Translate
@define('ERRORS_ARE_DISPLAYED_IN', 'Errors are displayed in %s, recommendations in %s and success in %s'); // Translate
@define('RED', 'red'); // Translate
@define('YELLOW', 'yellow'); // Translate
@define('GREEN', 'green'); // Translate
@define('PRE_INSTALLATION_REPORT', 'Serendipity v%s pre-installation report'); // Translate
@define('RECOMMENDED', 'Recommended'); // Translate
@define('ACTUAL', 'Actual'); // Translate
@define('PHPINI_CONFIGURATION', 'php.ini configuration'); // Translate
@define('PHP_INSTALLATION', 'PHP installation'); // Translate
@define('THEY_DO', 'they do'); // Translate
@define('THEY_DONT', 'they don\'t'); // Translate
@define('SIMPLE_INSTALLATION', 'Simple installation'); // Translate
@define('EXPERT_INSTALLATION', 'Expert installation'); // Translate
@define('COMPLETE_INSTALLATION', 'Complete installation'); // Translate
@define('WONT_INSTALL_DB_AGAIN', 'won\'t install the database again'); // Translate
@define('CHECK_DATABASE_EXISTS', 'Checking to see if the database and tables already exists'); // Translate
@define('CREATING_PRIMARY_AUTHOR', 'Creating primary author \'%s\''); // Translate
@define('SETTING_DEFAULT_TEMPLATE', 'Setting default template'); // Translate
@define('INSTALLING_DEFAULT_PLUGINS', 'Installing default plugins'); // Translate
@define('SERENDIPITY_INSTALLED', 'Serendipity has been successfully installed'); // Translate
@define('VISIT_BLOG_HERE', 'Visit your new blog here'); // Translate
@define('THANK_YOU_FOR_CHOOSING', 'Thank you for choosing Serendipity'); // Translate
@define('ERROR_DETECTED_IN_INSTALL', 'An error was detected in the installation'); // Translate
@define('OPERATING_SYSTEM', 'Operating system'); // Translate
@define('WEBSERVER_SAPI', 'Webserver SAPI'); // Translate
@define('IMAGE_ROTATE_LEFT', 'Rotate image 90 degrees counter-clockwise'); // Translate
@define('IMAGE_ROTATE_RIGHT', 'Rotate image 90 degrees clockwise'); // Translate
@define('TEMPLATE_SET', '\'%s\' has been set as your active template'); // Translate
@define('SEARCH_ERROR', 'The search function did not work as expected. Notice for the administrator of this blog: This may happen because of missing index keys in your database. On MySQL systems your database user account needs to be privileged to execute this query: <pre>CREATE FULLTEXT INDEX entry_idx on %sentries (title,body,extended)</pre> The specific error returned by the database was: <pre>%s</pre>'); // Translate
@define('EDIT_THIS_CAT', 'Editing "%s"'); // Translate
@define('CATEGORY_REMAINING', 'Delete this category and move its entries to this category'); // Translate
@define('CATEGORY_INDEX', 'Below is a list of categories available to your entries'); // Translate
@define('NO_CATEGORIES', 'No categories'); // Translate
@define('RESET_DATE', 'Reset date'); // Translate
@define('RESET_DATE_DESC', 'Click here to reset the date to the current time'); // Translate
@define('PROBLEM_PERMISSIONS_HOWTO', 'Permissions can be set by running shell command: `<em>%s</em>` on the failed directory, or by setting this using an FTP program'); // Translate
@define('WARNING_TEMPLATE_DEPRECATED', 'Warning: Your current template is using a deprecated template method, you are advised to update if possible'); // Translate
@define('ENTRY_PUBLISHED_FUTURE', 'This entry is not yet published.'); // Translate
@define('ENTRIES_BY', 'Entries by %s'); // Translate
@define('PREVIOUS', 'Previous'); // Translate
@define('NEXT', 'Next'); // Translate
@define('APPROVE', 'Approve'); // Translate
@define('DO_MARKUP_DESCRIPTION', 'Applica le trasformazioni del markup al testo (smilies, abbreviazionicon *, /, _, ...). Disabilitare questa opzione significa mantenere il codice HTML nel testo.');
@define('CATEGORY_ALREADY_EXIST', 'A category with the name "%s" already exist'); // Translate
@define('IMPORT_NOTES', 'Note:'); // Translate
@define('ERROR_FILE_FORBIDDEN', 'You are not allowed to upload files with active content'); // Translate
@define('ADMIN', 'Administration'); // Re-Translate
@define('ADMIN_FRONTPAGE', 'Frontpage'); // Translate
@define('QUOTE', 'Quote'); // Translate
@define('IFRAME_SAVE', 'Serendipity is now saving your entry, creating trackbacks and performing possible XML-RPC calls. This may take a while..'); // Translate
@define('IFRAME_SAVE_DRAFT', 'A draft of this entry has been saved'); // Translate
@define('IFRAME_PREVIEW', 'Serendipity is now creating the preview of your entry...'); // Translate
@define('IFRAME_WARNING', 'Your browser does not support the concept of iframes. Please open your serendipity_config.inc.php file and set $serendipity[\'use_iframe\'] variable to FALSE.'); // Translate
@define('NONE', 'none');
@define('USERCONF_CAT_DEFAULT_NEW_ENTRY', 'Default settings for new entries'); // Translate
@define('UPGRADE', 'Upgrade'); // Translate
@define('UPGRADE_TO_VERSION', 'Upgrade to version %s'); // Translate
@define('DELETE_DIRECTORY', 'Delete directory'); // Translate
@define('DELETE_DIRECTORY_DESC', 'You are about to delete the contents of a directory that contains media files, possibly files used in some of your entries.'); // Translate
@define('FORCE_DELETE', 'Delete ALL files in this directory, including those not known by Serendipity'); // Translate
@define('CREATE_DIRECTORY', 'Create directory'); // Translate
@define('CREATE_NEW_DIRECTORY', 'Create new directory'); // Translate
@define('CREATE_DIRECTORY_DESC', 'Here you can create a new directory to place media files in. Choose the name for your new directory and select an optional parent directory to place it in.'); // Translate
@define('BASE_DIRECTORY', 'Base directory'); // Translate
@define('USERLEVEL_EDITOR_DESC', 'Standard editor'); // Translate
@define('USERLEVEL_CHIEF_DESC', 'Chief editor'); // Translate
@define('USERLEVEL_ADMIN_DESC', 'Administrator'); // Translate
@define('USERCONF_USERLEVEL', 'Access level'); // Translate
@define('USERCONF_USERLEVEL_DESC', 'This level is used to determine what kind of access this user has to the blog'); // Translate
@define('USER_SELF_INFO', 'Logged in as %s (%s)'); // Translate
@define('ADMIN_ENTRIES', 'Entries'); // Translate
@define('RECHECK_INSTALLATION', 'Recheck installation'); // Translate
@define('IMAGICK_EXEC_ERROR', 'Unable to execute: "%s", error: %s, return var: %d'); // Translate
@define('INSTALL_OFFSET_DESC', 'Enter the amount of hours between the date of your webserver (current: %clock%) and your desired time zone'); // Translate
@define('UNMET_REQUIREMENTS', 'Requirements failed: %s'); // Translate
@define('CHARSET', 'Charset');
@define('AUTOLANG', 'Use visitor\'s browser language as default');
@define('AUTOLANG_DESC', 'If enabled, this will use the visitor\'s browser language setting to determine the default language of your entry and interface language.');
@define('INSTALL_AUTODETECT_URL', 'Autodetect used HTTP-Host'); // Translate
@define('INSTALL_AUTODETECT_URL_DESC', 'If set to "true", Serendipity will ensure that the HTTP Host which was used by your visitor is used as your BaseURL setting. Enabling this will let you be able to use multiple domain names for your Serendipity Blog, and use the domain for all follow-up links which the user used to access your blog.'); // Translate
@define('CONVERT_HTMLENTITIES', 'Try to auto-convert HTML entities?');
@define('EMPTY_SETTING', 'You did not specify a valid value for "%s"!');
@define('USERCONF_REALNAME', 'Real name'); // Translate
@define('USERCONF_REALNAME_DESC', 'The full name of the author. This is the name seen by readers'); // Translate
@define('HOTLINK_DONE', 'File hotlinked.<br />Done.'); // Translate
@define('ENTER_MEDIA_URL_METHOD', 'Fetch method:'); // Translate
@define('ADD_MEDIA_BLAHBLAH_NOTE', 'Note: If you choose to hotlink to server, make sure you have permission to hotlink to the designated website, or the website is yours. Hotlink allows you to use off-site images without storing them locally.'); // Translate
@define('MEDIA_HOTLINKED', 'hotlinked'); // Translate
@define('FETCH_METHOD_IMAGE', 'Download image to your server'); // Translate
@define('FETCH_METHOD_HOTLINK', 'Hotlink to server'); // Translate
@define('DELETE_HOTLINK_FILE', 'Deleted the hotlinked file entitled <b>%s</b>'); // Translate
@define('SYNDICATION_PLUGIN_SHOW_MAIL', 'Show E-Mail addresses?');
@define('IMAGE_MORE_INPUT', 'Add more images'); // Translate
@define('BACKEND_TITLE', 'Additional information in Plugin Configuration screen'); // Translate
@define('BACKEND_TITLE_FOR_NUGGET', 'Here you can define a custom string which is displayed in the Plugin Configuration screen together with the description of the HTML Nugget plugin. If you have multiple HTML nuggets with an empty title, this helps to distinct the plugins from another.'); // Translate
@define('CATEGORIES_ALLOW_SELECT', 'Allow visitors to display multiple categories at once?'); // Translate
@define('CATEGORIES_ALLOW_SELECT_DESC', 'If this option is enabled, a checkbox will be put next to each category in this sidebar plugin. Users can check those boxes and then see entries belonging to their selection.'); // Translate
@define('PAGE_BROWSE_PLUGINS', 'Page %s of %s, totalling %s plugins.');
@define('INSTALL_CAT_PERMALINKS', 'Permalinks');
@define('INSTALL_CAT_PERMALINKS_DESC', 'Defines various URL patterns to define permanent links in your blog. It is suggested that you use the defaults; if not, you should try to use the %id% value where possible to prevent Serendipity from querying the database to lookup the target URL.');
@define('INSTALL_PERMALINK', 'Permalink Entry URL structure');
@define('INSTALL_PERMALINK_DESC', 'Here you can define the relative URL structure begining from your base URL to where entries may become available. You can use the variables %id%, %title%, %day%, %month%, %year% and any other characters.');
@define('INSTALL_PERMALINK_AUTHOR', 'Permalink Author URL structure');
@define('INSTALL_PERMALINK_AUTHOR_DESC', 'Here you can define the relative URL structure begining from your base URL to where entries from certain authors may become available. You can use the variables %id%, %realname%, %username%, %email% and any other characters.');
@define('INSTALL_PERMALINK_CATEGORY', 'Permalink Category URL structure');
@define('INSTALL_PERMALINK_CATEGORY_DESC', 'Here you can define the relative URL structure begining from your base URL to where entries from certain categories may become available. You can use the variables %id%, %name%, %description% and any other characters.');
@define('INSTALL_PERMALINK_FEEDCATEGORY', 'Permalink RSS-Feed Category URL structure');
@define('INSTALL_PERMALINK_FEEDCATEGORY_DESC', 'Here you can define the relative URL structure begining from your base URL to where RSS-feeds frmo certain categories may become available. You can use the variables %id%, %name%, %description% and any other characters.');
@define('INSTALL_PERMALINK_ARCHIVESPATH', 'Path to archives');
@define('INSTALL_PERMALINK_ARCHIVEPATH', 'Path to archive');
@define('INSTALL_PERMALINK_CATEGORIESPATH', 'Path to categories');
@define('INSTALL_PERMALINK_UNSUBSCRIBEPATH', 'Path to unsubscribe comments');
@define('INSTALL_PERMALINK_DELETEPATH', 'Path to delete comments');
@define('INSTALL_PERMALINK_APPROVEPATH', 'Path to approve comments');
@define('INSTALL_PERMALINK_FEEDSPATH', 'Path to RSS Feeds');
@define('INSTALL_PERMALINK_PLUGINPATH', 'Path to single plugin');
@define('INSTALL_PERMALINK_ADMINPATH', 'Path to admin');
@define('INSTALL_PERMALINK_SEARCHPATH', 'Path to search');
@define('USERCONF_CREATE', 'Forbid creating entries?');
@define('USERCONF_CREATE_DESC', 'If selected, the user may not create new entries');
@define('INSTALL_CAL', 'Calendar Type');
@define('INSTALL_CAL_DESC', 'Choose your desired Calendar format');
@define('REPLY', 'Reply');
@define('USERCONF_GROUPS', 'Group Memberships');
@define('USERCONF_GROUPS_DESC', 'This user is a member of the following groups. Multiple memberships are possible.');
@define('MANAGE_GROUPS', 'Manage groups');
@define('DELETED_GROUP', 'Group #%d %s deleted.');
@define('CREATED_GROUP', 'A new group %s has been created');
@define('MODIFIED_GROUP', 'The properties of group %s have been changed');
@define('GROUP', 'Group');
@define('CREATE_NEW_GROUP', 'Create new group');
@define('DELETE_GROUP', 'You are about to delete group #%d %s. Are you serious?');
@define('USERLEVEL_OBSOLETE', 'NOTICE: The userlevel attribute is now only used for backward compatibility to plugins and fallback authorisation. User privileges are now handled by group memberships!');
@define('SYNDICATION_PLUGIN_FEEDBURNERID', 'FeedBurner ID');
@define('SYNDICATION_PLUGIN_FEEDBURNERID_DESC', 'The ID of the feed you wish to publish');
@define('SYNDICATION_PLUGIN_FEEDBURNERIMG', 'FeedBurner image');
@define('SYNDICATION_PLUGIN_FEEDBURNERIMG_DESC', 'Name of image to display (or leave blank for counter), located on feedburner.com, ex: fbapix.gif');
@define('SYNDICATION_PLUGIN_FEEDBURNERTITLE', 'FeedBurner title');
@define('SYNDICATION_PLUGIN_FEEDBURNERTITLE_DESC', 'Title (if any) to display alongside the image');
@define('SYNDICATION_PLUGIN_FEEDBURNERALT', 'FeedBurner image text');
@define('SYNDICATION_PLUGIN_FEEDBURNERALT_DESC', 'Text (if any) to display when hovering the image');
@define('SEARCH_TOO_SHORT', 'Your search-query must be longer than 3 characters. You can try to append * to shorter words, like: s9y* to trick the search into using shorter words.');
@define('INSTALL_DBPORT', 'Database port');
@define('INSTALL_DBPORT_DESC', 'The port used to connect to your database server');
@define('PLUGIN_GROUP_FRONTEND_EXTERNAL_SERVICES', 'Frontend: External Services');
@define('PLUGIN_GROUP_FRONTEND_FEATURES', 'Frontend: Features');
@define('PLUGIN_GROUP_FRONTEND_FULL_MODS', 'Frontend: Full Mods');
@define('PLUGIN_GROUP_FRONTEND_VIEWS', 'Frontend: Views');
@define('PLUGIN_GROUP_FRONTEND_ENTRY_RELATED', 'Frontend: Entry Related');
@define('PLUGIN_GROUP_BACKEND_EDITOR', 'Backend: Editor');
@define('PLUGIN_GROUP_BACKEND_USERMANAGEMENT', 'Backend: Usermanagement');
@define('PLUGIN_GROUP_BACKEND_METAINFORMATION', 'Backend: Meta information');
@define('PLUGIN_GROUP_BACKEND_TEMPLATES', 'Backend: Templates');
@define('PLUGIN_GROUP_BACKEND_FEATURES', 'Backend: Features');
@define('PLUGIN_GROUP_IMAGES', 'Images');
@define('PLUGIN_GROUP_ANTISPAM', 'Antispam');
@define('PLUGIN_GROUP_MARKUP', 'Markup');
@define('PLUGIN_GROUP_STATISTICS', 'Statistics');
@define('PERMISSION_PERSONALCONFIGURATION', 'personalConfiguration: Access personal configuration');
@define('PERMISSION_PERSONALCONFIGURATIONUSERLEVEL', 'personalConfigurationUserlevel: Change userlevels');
@define('PERMISSION_PERSONALCONFIGURATIONNOCREATE', 'personalConfigurationNoCreate: Change "forbid creating entries"');
@define('PERMISSION_PERSONALCONFIGURATIONRIGHTPUBLISH', 'personalConfigurationRightPublish: Change right to publish entries');
@define('PERMISSION_SITECONFIGURATION', 'siteConfiguration: Access system configuration');
@define('PERMISSION_BLOGCONFIGURATION', 'blogConfiguration: Access blog-centric configuration');
@define('PERMISSION_ADMINENTRIES', 'adminEntries: Administrate entries');
@define('PERMISSION_ADMINENTRIESMAINTAINOTHERS', 'adminEntriesMaintainOthers: Administrate other user\'s entries');
@define('PERMISSION_ADMINIMPORT', 'adminImport: Import entries');
@define('PERMISSION_ADMINCATEGORIES', 'adminCategories: Administrate categories');
@define('PERMISSION_ADMINCATEGORIESMAINTAINOTHERS', 'adminCategoriesMaintainOthers: Administrate other user\'s categories');
@define('PERMISSION_ADMINCATEGORIESDELETE', 'adminCategoriesDelete: Delete categories');
@define('PERMISSION_ADMINUSERS', 'adminUsers: Administrate users');
@define('PERMISSION_ADMINUSERSDELETE', 'adminUsersDelete: Delete users');
@define('PERMISSION_ADMINUSERSEDITUSERLEVEL', 'adminUsersEditUserlevel: Change userlevel');
@define('PERMISSION_ADMINUSERSMAINTAINSAME', 'adminUsersMaintainSame: Administrate users that are in your group(s)');
@define('PERMISSION_ADMINUSERSMAINTAINOTHERS', 'adminUsersMaintainOthers: Administrate users that are not in your group(s)');
@define('PERMISSION_ADMINUSERSCREATENEW', 'adminUsersCreateNew: Create new users');
@define('PERMISSION_ADMINUSERSGROUPS', 'adminUsersGroups: Administrate usergroups');
@define('PERMISSION_ADMINPLUGINS', 'adminPlugins: Administrate plugins');
@define('PERMISSION_ADMINPLUGINSMAINTAINOTHERS', 'adminPluginsMaintainOthers: Administrate other user\'s plugins');
@define('PERMISSION_ADMINIMAGES', 'adminImages: Administrate media files');
@define('PERMISSION_ADMINIMAGESDIRECTORIES', 'adminImagesDirectories: Administrate media directories');
@define('PERMISSION_ADMINIMAGESADD', 'adminImagesAdd: Add new media files');
@define('PERMISSION_ADMINIMAGESDELETE', 'adminImagesDelete: Delete media files');
@define('PERMISSION_ADMINIMAGESMAINTAINOTHERS', 'adminImagesMaintainOthers: Administrate other user\'s media files');
@define('PERMISSION_ADMINIMAGESVIEW', 'adminImagesView: View media files');
@define('PERMISSION_ADMINIMAGESSYNC', 'adminImagesSync: Sync thumbnails');
@define('PERMISSION_ADMINCOMMENTS', 'adminComments: Administrate comments');
@define('PERMISSION_ADMINTEMPLATES', 'adminTemplates: Administrate templates');
@define('INSTALL_BLOG_EMAIL', 'Blog\'s E-Mail address');
@define('INSTALL_BLOG_EMAIL_DESC', 'This configures the E-Mail address that is used as the "From"-Part of outgoing mails. Be sure to set this to an address that is recognized by the mailserver used on your host - many mailservers reject messages that have unknown From-addresses.');
@define('CATEGORIES_PARENT_BASE', 'Only show categories below...');
@define('CATEGORIES_PARENT_BASE_DESC', 'You can choose a parent category so that only the child categories are shown.');
@define('CATEGORIES_HIDE_PARALLEL', 'Hide categories that are not part of the category tree');
@define('CATEGORIES_HIDE_PARALLEL_DESC', 'If you want to hide categories that are part of a different category tree, you need to enable this. This feature makes most sense if used in conjunction with a multi-blog using the "Properties/Tempaltes of categories" plugin.');
@define('PERMISSION_ADMINIMAGESVIEWOTHERS', 'adminImagesViewOthers: View other user\'s media files');
@define('CHARSET_NATIVE', 'Native');
@define('INSTALL_CHARSET', 'Charset selection');
@define('INSTALL_CHARSET_DESC', 'Here you can toggle UTF-8 or native (ISO, EUC, ...) charactersets. Some languages only have UTF-8 translations so that setting the charset to "Native" will have no effects. UTF-8 is suggested for new installations. Do not change this setting if you have already made entries with special characters - this may lead to corrupt characters. Be sure to read more on http://www.s9y.org/index.php?node=46 about this issue.');
@define('CALENDAR_ENABLE_EXTERNAL_EVENTS', 'Enable Plugin API hook');
@define('CALENDAR_EXTEVENT_DESC', 'If enabled, plugins can hook into the calendar to display their own events highlighted. Only enable if you have installed plugins that need this, otherwise it just decreases performance.');
@define('XMLRPC_NO_LONGER_BUNDLED', 'The XML-RPC API Interface to Serendipity is no longer bundled because of ongoing security issues with this API and not many people using it. Thus you need to install the XML-RPC Plugin to use the XML-RPC API. The URL to use in your applications will NOT change - as soon as you have installed the plugin, you will again be able to use the API.');
@define('PERM_READ', 'Read permission');
@define('PERM_WRITE', 'Write permission');

@define('PERM_DENIED', 'Permission denied.');
@define('INSTALL_ACL', 'Apply read-permissions for categories');
@define('INSTALL_ACL_DESC', 'If enabled, the usergroup permission settings you setup for categories will be applied when logged-in users view your blog. If disabled, the read-permissions of the categories are NOT applied, but the positive effect is a little speedup on your blog. So if you don\'t need multi-user read permissions for your blog, disable this setting.');
@define('PLUGIN_API_VALIDATE_ERROR', 'Configuration syntax wrong for option "%s". Needs content of type "%s".');
@define('USERCONF_CHECK_PASSWORD', 'Old Password');
@define('USERCONF_CHECK_PASSWORD_DESC', 'If you change the password in the field above, you need to enter the current user password into this field.');
@define('USERCONF_CHECK_PASSWORD_ERROR', 'You did not specify the right old password, and are not authorized to change the new password. Your settings were not saved.');
@define('ERROR_XSRF', 'Your browser did not sent a valid HTTP-Referrer string. This may have either been caused by a misconfigured browser/proxy or by a Cross Site Request Forgery (XSRF) aimed at you. The action you requested could not be completed.');
@define('INSTALL_PERMALINK_FEEDAUTHOR_DESC', 'Here you can define the relative URL structure beginning from your base URL to where RSS-feeds from specific users may be viewed. You can use the variables %id%, %realname%, %username%, %email% and any other characters.');
@define('INSTALL_PERMALINK_FEEDAUTHOR', 'Permalink RSS-Feed Author URL structure');
@define('INSTALL_PERMALINK_AUTHORSPATH', 'Path to authors');
@define('AUTHORS', 'Authors');
@define('AUTHORS_ALLOW_SELECT', 'Allow visitors to display multiple authors at once?');
@define('AUTHORS_ALLOW_SELECT_DESC', 'If this option is enabled, a checkbox will be put next to each author in this sidebar plugin.  Users can check those boxes and see entries matching their selection.');
@define('AUTHOR_PLUGIN_DESC', 'Shows a list of authors');
@define('CATEGORY_PLUGIN_TEMPLATE', 'Enable Smarty-Templates?');
@define('CATEGORY_PLUGIN_TEMPLATE_DESC', 'If this option is enabled, the plugin will utilize Smarty-Templating features to output the category listing. If you enable this, you can change the layout via the "plugin_categories.tpl" template file. Enabling this option will impact performance, so if you do not need to make customizations, leave it disabled.');
@define('CATEGORY_PLUGIN_SHOWCOUNT', 'Show number of entries per category?');
@define('AUTHORS_SHOW_ARTICLE_COUNT', 'Show number of articles next to author name?');
@define('AUTHORS_SHOW_ARTICLE_COUNT_DESC', 'If this option is enabled, the number of articles by this author is shown next to the authors name in parentheses.');

@define('COMMENT_NOT_ADDED', 'Il tuo commento non può essere aggiunto, perché i commenti per questa notizia sono stati disattivati. '); // Retranslate: 'Your comment could not be added, because comments for this entry have either been disabled, you entered invalid data, or your comment was caught by anti-spam measurements.'
