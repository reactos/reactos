<?php
/** Polish (Polski)
 *
 * @ingroup Language
 * @file
 *
 * @author Beau
 * @author Derbeth
 * @author Equadus
 * @author Herr Kriss
 * @author Jwitos
 * @author Lajsikonik
 * @author Leinad
 * @author Maikking
 * @author Masti
 * @author Matma Rex
 * @author Remember the dot
 * @author Sp5uhe
 * @author Stv
 * @author Szczepan1990
 * @author ToSter
 * @author Wpedzich
 * @author Ymar
 * @author לערי ריינהארט
 */

$namespaceNames = array(
	NS_MEDIA          => 'Media',
	NS_SPECIAL        => 'Specjalna',
	NS_MAIN           => '',
	NS_TALK           => 'Dyskusja',
	NS_USER           => 'Użytkownik',
	NS_USER_TALK      => 'Dyskusja_użytkownika',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK   => 'Dyskusja_$1',
	NS_IMAGE          => 'Grafika',
	NS_IMAGE_TALK     => 'Dyskusja_grafiki',
	NS_MEDIAWIKI      => 'MediaWiki',
	NS_MEDIAWIKI_TALK => 'Dyskusja_MediaWiki',
	NS_TEMPLATE       => 'Szablon',
	NS_TEMPLATE_TALK  => 'Dyskusja_szablonu',
	NS_HELP           => 'Pomoc',
	NS_HELP_TALK      => 'Dyskusja_pomocy',
	NS_CATEGORY       => 'Kategoria',
	NS_CATEGORY_TALK  => 'Dyskusja_kategorii',
);

$skinNames = array(
	'standard'    => 'Standardowa',
	'nostalgia'   => 'Tęsknota',
	'cologneblue' => 'Błękit',
	'monobook'    => 'Książka',
	'myskin'      => 'Moja skórka',
	'chick'       => 'Kurczaczek',
	'simple'      => 'Prosta',
	'modern'      => 'Nowoczesna',
);

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

$fallback8bitEncoding = 'iso-8859-2';
$separatorTransformTable = array(
	',' => "\xc2\xa0", // @bug 2749
	'.' => ','
);

$linkTrail = '/^([a-zęóąśłżźćńĘÓĄŚŁŻŹĆŃ]+)(.*)$/sDu';

$specialPageAliases = array(
	'DoubleRedirects'           => array( 'Podwójne_przekierowania' ),
	'BrokenRedirects'           => array( 'Zerwane_przekierowania' ),
	'Disambiguations'           => array( 'Ujednoznacznienia' ),
	'Userlogin'                 => array( 'Zaloguj' ),
	'Userlogout'                => array( 'Wyloguj' ),
	'CreateAccount'             => array( 'Stwórz_konto' ),
	'Preferences'               => array( 'Preferencje' ),
	'Watchlist'                 => array( 'Obserwowane' ),
	'Recentchanges'             => array( 'Ostatnie_zmiany', 'OZ' ),
	'Upload'                    => array( 'Prześlij' ),
	'Imagelist'                 => array( 'Pliki' ),
	'Newimages'                 => array( 'Nowe_pliki' ),
	'Listusers'                 => array( 'Użytkownicy' ),
	'Listgrouprights'           => array( 'Uprawnienia_grup_użytkowników', 'Uprawnienia' ),
	'Statistics'                => array( 'Statystyka', 'Statystyki' ),
	'Randompage'                => array( 'Losowa_strona', 'Losowa' ),
	'Lonelypages'               => array( 'Porzucone_strony' ),
	'Uncategorizedpages'        => array( 'Nieskategoryzowane_strony' ),
	'Uncategorizedcategories'   => array( 'Nieskategoryzowane_kategorie' ),
	'Uncategorizedimages'       => array( 'Nieskategoryzowane_pliki' ),
	'Uncategorizedtemplates'    => array( 'Nieskategoryzowane_szablony' ),
	'Unusedcategories'          => array( 'Nieużywane_kategorie' ),
	'Unusedimages'              => array( 'Nieużywane_pliki' ),
	'Wantedpages'               => array( 'Potrzebne_strony' ),
	'Wantedcategories'          => array( 'Potrzebne_kategorie' ),
	'Mostlinked'                => array( 'Najczęściej_linkowane' ),
	'Mostlinkedcategories'      => array( 'Najczęściej_linkowane_kategorie' ),
	'Mostlinkedtemplates'       => array( 'Najczęściej_linkowane_szablony' ),
	'Mostcategories'            => array( 'Najwięcej_kategorii' ),
	'Mostimages'                => array( 'Najczęściej_linkowane_pliki' ),
	'Mostrevisions'             => array( 'Najwięcej_edycji', 'Najczęściej_edytowane' ),
	'Fewestrevisions'           => array( 'Najmniej_edycji' ),
	'Shortpages'                => array( 'Najkrótsze_strony' ),
	'Longpages'                 => array( 'Najdłuższe_strony' ),
	'Newpages'                  => array( 'Nowe_strony' ),
	'Ancientpages'              => array( 'Stare_strony' ),
	'Deadendpages'              => array( 'Bez_linków' ),
	'Protectedpages'            => array( 'Zabezpieczone_strony' ),
	'Protectedtitles'           => array( 'Zabezpieczone_nazwy_stron' ),
	'Allpages'                  => array( 'Wszystkie_strony' ),
	'Prefixindex'               => array( 'Strony_według_prefiksu' ),
	'Ipblocklist'               => array( 'Zablokowani' ),
	'Specialpages'              => array( 'Strony_specjalne' ),
	'Contributions'             => array( 'Wkład' ),
	'Emailuser'                 => array( 'E-mail' ),
	'Confirmemail'              => array( 'Potwierdź_e-mail' ),
	'Whatlinkshere'             => array( 'Linkujące' ),
	'Recentchangeslinked'       => array( 'Zmiany_w_linkujących' ),
	'Movepage'                  => array( 'Przenieś' ),
	'Blockme'                   => array( 'Zablokuj_mnie' ),
	'Booksources'               => array( 'Książki' ),
	'Categories'                => array( 'Kategorie' ),
	'Export'                    => array( 'Eksport' ),
	'Version'                   => array( 'Wersja' ),
	'Allmessages'               => array( 'Wszystkie_komunikaty' ),
	'Log'                       => array( 'Rejestr', 'Logi' ),
	'Blockip'                   => array( 'Blokuj' ),
	'Undelete'                  => array( 'Odtwórz' ),
	'Import'                    => array( 'Import' ),
	'Lockdb'                    => array( 'Zablokuj_bazę' ),
	'Unlockdb'                  => array( 'Odblokuj_bazę' ),
	'Userrights'                => array( 'Uprawnienia', 'Prawa_użytkowników' ),
	'MIMEsearch'                => array( 'Wyszukiwanie_MIME' ),
	'FileDuplicateSearch'       => array( 'Szukaj_duplikatu_pliku' ),
	'Unwatchedpages'            => array( 'Nieobserwowane_strony' ),
	'Listredirects'             => array( 'Przekierowania' ),
	'Revisiondelete'            => array( 'Usuń_wersję' ),
	'Unusedtemplates'           => array( 'Nieużywane_szablony' ),
	'Randomredirect'            => array( 'Losowe_przekierowanie' ),
	'Mypage'                    => array( 'Moja_strona' ),
	'Mytalk'                    => array( 'Moja_dyskusja' ),
	'Mycontributions'           => array( 'Mój_wkład' ),
	'Listadmins'                => array( 'Administratorzy' ),
	'Listbots'                  => array( 'Boty' ),
	'Popularpages'              => array( 'Pupularne_strony' ),
	'Search'                    => array( 'Szukaj' ),
	'Resetpass'                 => array( 'Resetuj_hasło' ),
	'Withoutinterwiki'          => array( 'Strony_bez_interwiki' ),
	'MergeHistory'              => array( 'Połącz_historię' ),
	'Filepath'                  => array( 'Ścieżka_do_pliku' ),
	'Invalidateemail'           => array( 'Anuluj_e-mail' ),
);

$magicWords = array(
	'redirect'            => array( '0', '#REDIRECT', '#TAM', '#PRZEKIERUJ' ),
	'notoc'               => array( '0', '__NOTOC__', '__BEZSPISU__' ),
	'nogallery'           => array( '0', '__NOGALLERY__', '__BEZGALERII__' ),
	'forcetoc'            => array( '0', '__FORCETOC__', '__ZESPISEM__' ),
	'toc'                 => array( '0', '__TOC__', '__SPIS__' ),
	'noeditsection'       => array( '0', '__NOEDITSECTION__', '__BEZEDYCJISEKCJI__' ),
	'localmonth'          => array( '1', 'LOCALMONTH', 'MIESIĄC' ),
	'localmonthname'      => array( '1', 'LOCALMONTHNAME', 'MIESIĄCNAZWA' ),
	'localmonthnamegen'   => array( '1', 'LOCALMONTHNAMEGEN', 'MIESIĄCNAZWAD' ),
	'localmonthabbrev'    => array( '1', 'LOCALMONTHABBREV', 'MIESIĄCNAZWASKR' ),
	'localday'            => array( '1', 'LOCALDAY', 'DZIEŃ' ),
	'localday2'           => array( '1', 'LOCALDAY2', 'DZIEŃ2' ),
	'localdayname'        => array( '1', 'LOCALDAYNAME', 'DZIEŃTYGODNIA' ),
	'localyear'           => array( '1', 'LOCALYEAR', 'ROK' ),
	'localtime'           => array( '1', 'LOCALTIME', 'CZAS' ),
	'localhour'           => array( '1', 'LOCALHOUR', 'GODZINA' ),
	'numberofpages'       => array( '1', 'NUMBEROFPAGES', 'STRON' ),
	'numberofarticles'    => array( '1', 'NUMBEROFARTICLES', 'ARTYKUŁÓW' ),
	'numberoffiles'       => array( '1', 'NUMBEROFFILES', 'PLIKÓW' ),
	'numberofusers'       => array( '1', 'NUMBEROFUSERS', 'UŻYTKOWNIKÓW' ),
	'numberofedits'       => array( '1', 'NUMBEROFEDITS', 'EDYCJI' ),
	'pagename'            => array( '1', 'PAGENAME', 'NAZWASTRONY' ),
	'namespace'           => array( '1', 'NAMESPACE', 'NAZWAPRZESTRZENI' ),
	'talkspace'           => array( '1', 'TALKSPACE', 'DYSKUSJA' ),
	'fullpagename'        => array( '1', 'FULLPAGENAME', 'PELNANAZWASTRONY' ),
	'img_thumbnail'       => array( '1', 'thumbnail', 'thumb', 'mały' ),
	'img_manualthumb'     => array( '1', 'thumbnail=$1', 'thumb=$1', 'mały=$1' ),
	'img_right'           => array( '1', 'right', 'prawo' ),
	'img_left'            => array( '1', 'left', 'lewo' ),
	'img_none'            => array( '1', 'none', 'brak' ),
	'img_center'          => array( '1', 'center', 'centre', 'centruj' ),
	'img_framed'          => array( '1', 'framed', 'enframed', 'frame', 'ramka' ),
	'img_frameless'       => array( '1', 'frameless', 'bezramki', 'bez ramki' ),
	'img_border'          => array( '1', 'border', 'tło' ),
	'img_top'             => array( '1', 'top', 'góra' ),
	'img_middle'          => array( '1', 'middle', 'środek' ),
	'img_bottom'          => array( '1', 'bottom', 'dół' ),
	'sitename'            => array( '1', 'SITENAME', 'PROJEKT' ),
	'grammar'             => array( '0', 'GRAMMAR:', 'ODMIANA:' ),
	'localweek'           => array( '1', 'LOCALWEEK', 'TYDZIEŃROKU' ),
	'localdow'            => array( '1', 'LOCALDOW', 'DZIEŃTYGODNIANR' ),
	'plural'              => array( '0', 'PLURAL:', 'MNOGA:' ),
	'lcfirst'             => array( '0', 'LCFIRST:', 'ZMAŁEJ' ),
	'ucfirst'             => array( '0', 'UCFIRST:', 'ZWIELKIEJ', 'ZDUŻEJ' ),
	'lc'                  => array( '0', 'LC:', 'MAŁE' ),
	'uc'                  => array( '0', 'UC:', 'WIELKIE', 'DUŻE' ),
	'language'            => array( '0', '#LANGUAGE:', '#JĘZYK:' ),
	'numberofadmins'      => array( '1', 'NUMBEROFADMINS', 'ADMINISTRATORÓW' ),
	'padleft'             => array( '0', 'PADLEFT', 'DOLEWEJ' ),
	'padright'            => array( '0', 'PADRIGHT', 'DOPRAWEJ' ),
);

$messages = array(
# User preference toggles
'tog-underline'               => 'Podkreślenie linków',
'tog-highlightbroken'         => 'Oznacz <a href="" class="new">tak</a> linki do brakujących stron (alternatywa: dołączany znak zapytania<a href="" class="internal">?</a>).',
'tog-justify'                 => 'Wyrównuj tekst w akapitach do obu stron',
'tog-hideminor'               => 'Ukryj drobne poprawki w „Ostatnich zmianach”',
'tog-extendwatchlist'         => 'Rozszerzona lista obserwowanych',
'tog-usenewrc'                => 'Rozszerzenie ostatnich zmian (JavaScript)',
'tog-numberheadings'          => 'Automatyczna numeracja nagłówków',
'tog-showtoolbar'             => 'Pokaż pasek narzędzi (JavaScript)',
'tog-editondblclick'          => 'Podwójne kliknięcie rozpoczyna edycję (JavaScript)',
'tog-editsection'             => 'Możliwość edycji poszczególnych sekcji strony',
'tog-editsectiononrightclick' => 'Kliknięcie prawym klawiszem myszy na tytule sekcji rozpoczyna jej edycję (JavaScript)',
'tog-showtoc'                 => 'Pokaż spis treści (na stronach o więcej niż 3 nagłówkach)',
'tog-rememberpassword'        => 'Pamiętaj hasło między sesjami na tym komputerze',
'tog-editwidth'               => 'Obszar edycji o pełnej szerokości',
'tog-watchcreations'          => 'Dodaj do obserwowanych strony tworzone przeze mnie',
'tog-watchdefault'            => 'Dodaj do obserwowanych strony, które edytuję',
'tog-watchmoves'              => 'Dodaj do obserwowanych strony, które przenoszę',
'tog-watchdeletion'           => 'Dodaj do obserwowanych strony, które usuwam',
'tog-minordefault'            => 'Wszystkie zmiany oznaczaj domyślnie jako drobne',
'tog-previewontop'            => 'Pokazuj podgląd powyżej obszaru edycji',
'tog-previewonfirst'          => 'Pokaż podgląd strony podczas pierwszej edycji',
'tog-nocache'                 => 'Wyłącz pamięć podręczną',
'tog-enotifwatchlistpages'    => 'Wyślij do mnie e-mail, jeśli strona z listy moich obserwowanych zostanie zmodyfikowana',
'tog-enotifusertalkpages'     => 'Wyślij do mnie e-mail, jeśli moja strona dyskusji zostanie zmodyfikowana',
'tog-enotifminoredits'        => 'Wyślij e-mail także w przypadku drobnych zmian na stronach',
'tog-enotifrevealaddr'        => 'Nie ukrywaj mojego adresu e-mail w powiadomieniach',
'tog-shownumberswatching'     => 'Pokaż liczbę obserwujących użytkowników',
'tog-fancysig'                => 'Podpis z kodami wiki (nie linkuj automatycznie całości)',
'tog-externaleditor'          => 'Domyślnie używaj zewnętrznego edytora',
'tog-externaldiff'            => 'Domyślnie używaj zewnętrznego programu pokazującego zmiany',
'tog-showjumplinks'           => 'Włącz odnośniki „skocz do”',
'tog-uselivepreview'          => 'Używaj dynamicznego podglądu (JavaScript) (eksperymentalny)',
'tog-forceeditsummary'        => 'Informuj o niewypełnieniu opisu zmian',
'tog-watchlisthideown'        => 'Ukryj moje edycje na liście obserwowanych',
'tog-watchlisthidebots'       => 'Ukryj edycje botów na liście obserwowanych',
'tog-watchlisthideminor'      => 'Ukryj drobne zmiany na liście obserwowanych',
'tog-nolangconversion'        => 'Wyłącz odmianę',
'tog-ccmeonemails'            => 'Przesyłaj mi kopie wiadomości wysłanych przez mnie do innych użytkowników',
'tog-diffonly'                => 'Nie pokazuj treści stron pod porównaniami zmian',
'tog-showhiddencats'          => 'Pokaż ukryte kategorie',

'underline-always'  => 'zawsze',
'underline-never'   => 'nigdy',
'underline-default' => 'według ustawień przeglądarki',

'skinpreview' => '(podgląd)',

# Dates
'sunday'        => 'niedziela',
'monday'        => 'poniedziałek',
'tuesday'       => 'wtorek',
'wednesday'     => 'środa',
'thursday'      => 'czwartek',
'friday'        => 'piątek',
'saturday'      => 'sobota',
'sun'           => 'Nie',
'mon'           => 'Pon',
'tue'           => 'Wto',
'wed'           => 'Śro',
'thu'           => 'Czw',
'fri'           => 'Pią',
'sat'           => 'Sob',
'january'       => 'styczeń',
'february'      => 'luty',
'march'         => 'marzec',
'april'         => 'kwiecień',
'may_long'      => 'maj',
'june'          => 'czerwiec',
'july'          => 'lipiec',
'august'        => 'sierpień',
'september'     => 'wrzesień',
'october'       => 'październik',
'november'      => 'listopad',
'december'      => 'grudzień',
'january-gen'   => 'stycznia',
'february-gen'  => 'lutego',
'march-gen'     => 'marca',
'april-gen'     => 'kwietnia',
'may-gen'       => 'maja',
'june-gen'      => 'czerwca',
'july-gen'      => 'lipca',
'august-gen'    => 'sierpnia',
'september-gen' => 'września',
'october-gen'   => 'października',
'november-gen'  => 'listopada',
'december-gen'  => 'grudnia',
'jan'           => 'sty',
'feb'           => 'lut',
'mar'           => 'mar',
'apr'           => 'kwi',
'may'           => 'maj',
'jun'           => 'cze',
'jul'           => 'lip',
'aug'           => 'sie',
'sep'           => 'wrz',
'oct'           => 'paź',
'nov'           => 'lis',
'dec'           => 'gru',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|Kategoria|Kategorie}}',
'category_header'                => 'Strony w kategorii „$1”',
'subcategories'                  => 'Podkategorie',
'category-media-header'          => 'Pliki w kategorii „$1”',
'category-empty'                 => "''Obecnie w tej kategorii brak stron oraz plików.''",
'hidden-categories'              => '{{PLURAL:$1|Ukryta kategoria|Ukryte kategorie}}',
'hidden-category-category'       => 'Ukryte kategorie', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|Ta kategoria ma tylko jedną podkategorię.|Poniżej wyświetlono {{PLURAL:$1|jedną podkategorię|$1 podkategorie|$1 podkategorii}} spośród wszystkich $2 podkategorii tej kategorii.}}',
'category-subcat-count-limited'  => 'Ta kategoria ma {{PLURAL:$1|1 podkategorię|$1 podkategorie|$1 podkategorii}}.',
'category-article-count'         => '{{PLURAL:$2|W tej kategorii jest tylko jedna strona.|Poniżej wyświetlono {{PLURAL:$1|jedną stronę|$1 strony|$1 stron}} spośród wszystkich $2 stron tej kategorii.}}',
'category-article-count-limited' => 'W tej kategorii {{PLURAL:$1|jest 1 strona|są $1 strony|jest $1 stron}}.',
'category-file-count'            => '{{PLURAL:$2|W tej kategorii znajduje się tylko jeden plik.|W tej kategorii {{PLURAL:$1|jest 1 plik|są $1 pliki|jest $1 plików}} z ogólnej liczby $2 plików.}}',
'category-file-count-limited'    => 'W tej kategorii {{PLURAL:$1|jest 1 plik|są $1 pliki|jest $1 plików}}.',
'listingcontinuesabbrev'         => 'cd.',

'mainpagetext'      => "<big>'''Instalacja MediaWiki powiodła się.'''</big>",
'mainpagedocfooter' => 'Zobacz [http://meta.wikimedia.org/wiki/Help:Contents przewodnik użytkownika] w celu uzyskania informacji o działaniu oprogramowania wiki.

== Na początek ==
* [http://www.mediawiki.org/wiki/Manual:Configuration_settings Lista ustawień konfiguracyjnych]
* [http://www.mediawiki.org/wiki/Manual:FAQ MediaWiki FAQ]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce Komunikaty o nowych wersjach MediaWiki]',

'about'          => 'O {{GRAMMAR:MS.lp|{{SITENAME}}}}',
'article'        => 'artykuł',
'newwindow'      => '(otwiera się w nowym oknie)',
'cancel'         => 'Anuluj',
'qbfind'         => 'Znajdź',
'qbbrowse'       => 'Przeglądanie',
'qbedit'         => 'Edycja',
'qbpageoptions'  => 'Ta strona',
'qbpageinfo'     => 'Kontekst',
'qbmyoptions'    => 'Moje strony',
'qbspecialpages' => 'strony specjalne',
'moredotdotdot'  => 'Więcej...',
'mypage'         => 'Moja strona',
'mytalk'         => 'moja dyskusja',
'anontalk'       => 'Dyskusja tego IP',
'navigation'     => 'nawigacja',
'and'            => 'oraz',

# Metadata in edit box
'metadata_help' => 'Metadane:',

'errorpagetitle'    => 'Błąd',
'returnto'          => 'Wróć do strony $1.',
'tagline'           => 'Z {{GRAMMAR:D.lp|{{SITENAME}}}}',
'help'              => 'Pomoc',
'search'            => 'Szukaj',
'searchbutton'      => 'Szukaj',
'go'                => 'Przejdź',
'searcharticle'     => 'Przejdź',
'history'           => 'Historia strony',
'history_short'     => 'historia i autorzy',
'updatedmarker'     => 'zmienione od ostatniej wizyty',
'info_short'        => 'Informacja',
'printableversion'  => 'Wersja do druku',
'permalink'         => 'Link do tej wersji',
'print'             => 'drukuj',
'edit'              => 'edytuj',
'create'            => 'utwórz',
'editthispage'      => 'Edytuj tę stronę',
'create-this-page'  => 'Utwórz tę stronę',
'delete'            => 'usuń',
'deletethispage'    => 'Usuń tę stronę',
'undelete_short'    => 'odtwórz {{PLURAL:$1|1 wersję|$1 wersje|$1 wersji}}',
'protect'           => 'zabezpiecz',
'protect_change'    => 'zmień',
'protectthispage'   => 'Zabezpiecz tę stronę',
'unprotect'         => 'odbezpiecz',
'unprotectthispage' => 'Odbezpiecz tę stronę',
'newpage'           => 'Nowa strona',
'talkpage'          => 'Dyskusja',
'talkpagelinktext'  => 'dyskusja',
'specialpage'       => 'strona specjalna',
'personaltools'     => 'osobiste',
'postcomment'       => 'Skomentuj',
'articlepage'       => 'Artykuł',
'talk'              => 'dyskusja',
'views'             => 'Widok',
'toolbox'           => 'narzędzia',
'userpage'          => 'Strona użytkownika',
'projectpage'       => 'Strona projektu',
'imagepage'         => 'Strona pliku',
'mediawikipage'     => 'Strona komunikatu',
'templatepage'      => 'Strona szablonu',
'viewhelppage'      => 'Strona pomocy',
'categorypage'      => 'Strona kategorii',
'viewtalkpage'      => 'Strona dyskusji',
'otherlanguages'    => 'W innych językach',
'redirectedfrom'    => '(Przekierowano z $1)',
'redirectpagesub'   => 'Strona przekierowująca',
'lastmodifiedat'    => 'Tę stronę ostatnio zmodyfikowano $2, $1.', # $1 date, $2 time
'viewcount'         => 'Tę stronę obejrzano {{PLURAL:$1|tylko raz|$1 razy}}.',
'protectedpage'     => 'Strona zabezpieczona',
'jumpto'            => 'Skocz do:',
'jumptonavigation'  => 'nawigacji',
'jumptosearch'      => 'wyszukiwania',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'O {{GRAMMAR:MS.lp|{{SITENAME}}}}',
'aboutpage'            => 'Project:O {{GRAMMAR:MS.lp|{{SITENAME}}}}',
'bugreports'           => 'Raport o błędach',
'bugreportspage'       => 'Project:Błędy',
'copyright'            => 'Treść udostępniana na licencji $1.',
'copyrightpagename'    => 'prawami autorskimi {{GRAMMAR:D.lp|{{SITENAME}}}}',
'copyrightpage'        => '{{ns:project}}:Prawa_autorskie',
'currentevents'        => 'Bieżące wydarzenia',
'currentevents-url'    => 'Project:Bieżące wydarzenia',
'disclaimers'          => 'Informacje prawne',
'disclaimerpage'       => 'Project:Informacje prawne',
'edithelp'             => 'Pomoc w edycji',
'edithelppage'         => 'Help:Jak edytować stronę',
'faq'                  => 'FAQ',
'faqpage'              => 'Project:FAQ',
'helppage'             => 'Help:Spis treści',
'mainpage'             => 'Strona główna',
'mainpage-description' => 'Strona główna',
'policy-url'           => 'Project:Zasady',
'portal'               => 'Portal społeczności',
'portal-url'           => 'Project:Portal społeczności',
'privacy'              => 'Zasady ochrony prywatności',
'privacypage'          => 'Project:Zasady ochrony prywatności',

'badaccess'        => 'Niewłaściwe uprawnienia',
'badaccess-group0' => 'Nie masz uprawnień wymaganych do wykonania tej operacji.',
'badaccess-group1' => 'Wykonywanie tej operacji zostało ograniczone do użytkowników w grupie $1.',
'badaccess-group2' => 'Wykonywanie tej operacji zostało ograniczone do użytkowników w jednej z grup $1.',
'badaccess-groups' => 'Wykonywanie tej operacji zostało ograniczone do użytkowników w jednej z grup $1.',

'versionrequired'     => 'Wymagane MediaWiki w wersji $1',
'versionrequiredtext' => 'Użycie tej strony wymaga oprogramowania MediaWiki w wersji $1. Zobacz stronę [[Special:Version|wersja oprogramowania]].',

'ok'                      => 'OK',
'retrievedfrom'           => 'Źródło: „$1”',
'youhavenewmessages'      => 'Masz $1 ($2).',
'newmessageslink'         => 'nowe wiadomości',
'newmessagesdifflink'     => 'różnica z poprzednią wersją',
'youhavenewmessagesmulti' => 'Masz nowe wiadomości na $1',
'editsection'             => 'edytuj',
'editold'                 => 'edytuj',
'viewsourceold'           => 'pokaż źródło',
'editsectionhint'         => 'Edytuj sekcję: $1',
'toc'                     => 'Spis treści',
'showtoc'                 => 'pokaż',
'hidetoc'                 => 'ukryj',
'thisisdeleted'           => 'Pokaż/odtwórz $1',
'viewdeleted'             => 'Zobacz $1',
'restorelink'             => '{{PLURAL:$1|jedną usuniętą wersję|$1 usunięte wersje|$1 usuniętych wersji}}',
'feedlinks'               => 'Kanały:',
'feed-invalid'            => 'Niewłaściwy typ kanału informacyjnego.',
'feed-unavailable'        => 'Kanały informacyjne {{GRAMMAR:D.lp|{{SITENAME}}}} nie są dostępne',
'site-rss-feed'           => 'Kanał RSS {{GRAMMAR:D.lp|$1}}',
'site-atom-feed'          => 'Kanał Atom {{GRAMMAR:D.lp|$1}}',
'page-rss-feed'           => 'Kanał RSS „$1”',
'page-atom-feed'          => 'Kanał Atom „$1”',
'red-link-title'          => '$1 (jeszcze nie utworzona)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Strona',
'nstab-user'      => 'Strona użytkownika',
'nstab-media'     => 'Pliki',
'nstab-special'   => 'Strona specjalna',
'nstab-project'   => 'Strona projektu',
'nstab-image'     => 'Plik',
'nstab-mediawiki' => 'Komunikat',
'nstab-template'  => 'Szablon',
'nstab-help'      => 'Pomoc',
'nstab-category'  => 'Kategoria',

# Main script and global functions
'nosuchaction'      => 'Brak takiej operacji',
'nosuchactiontext'  => 'Oprogramowanie wiki nie rozpoznało polecenia zawartego w adresie URL',
'nosuchspecialpage' => 'Brak takiej strony specjalnej',
'nospecialpagetext' => "<big>'''Brak żądanej strony specjalnej.'''</big>

Listę dostępnych stron specjalnych znajdziesz [[Special:SpecialPages|tutaj]].",

# General errors
'error'                => 'Błąd',
'databaseerror'        => 'Błąd bazy danych',
'dberrortext'          => 'Wystąpił błąd składni w zapytaniu do bazy danych.
Może to oznaczać błąd w oprogramowaniu.
Ostatnie, nieudane zapytanie to:
<blockquote><tt>$1</tt></blockquote>
wysłane przez funkcję „<tt>$2</tt>”.
MySQL zgłosił błąd „<tt>$3: $4</tt>”.',
'dberrortextcl'        => 'Wystąpił błąd składni w zapytaniu do bazy danych.
Ostatnie, nieudane zapytanie to:
„$1”
wywołane zostało przez funkcję „$2”.
MySQL zgłosił błąd „$3: $4”',
'noconnect'            => 'UWAGA! Projekt {{SITENAME}} ma chwilowe problemy techniczne. Brak połączenia z serwerem bazy danych.<br />
$1',
'nodb'                 => 'Nie można odnaleźć bazy danych $1',
'cachederror'          => 'Poniższy tekst strony jest kopią znajdującą się w pamięci podręcznej i może być już nieaktualny.',
'laggedslavemode'      => 'Uwaga! Ta strona może nie zawierać najnowszych aktualizacji.',
'readonly'             => 'Baza danych jest zablokowana',
'enterlockreason'      => 'Podaj powód zablokowania bazy oraz szacunkowy termin jej odblokowania',
'readonlytext'         => 'Baza danych jest obecnie zablokowana – nie można wprowadzać nowych informacji ani modyfikować istniejących. Powodem są prawdopodobnie czynności administracyjne. Po ich zakończeniu przywrócona zostanie pełna funkcjonalność bazy.

Administrator, który zablokował bazę, podał następujące wyjaśnienie: $1',
'missing-article'      => 'W bazie danych nie odnaleziono treści strony „$1” $2.

Zazwyczaj jest to spowodowane odwołaniem do nieaktualnego linku prowadzącego do różnicy pomiędzy dwoma wersjami strony lub do wersji z historii usuniętej strony.

Jeśli tak nie jest, możliwe, że problem został wywołany przez błąd w oprogramowaniu.
Można zgłosić ten fakt [[Special:ListUsers/sysop|administratorowi]], podając adres URL.',
'missingarticle-rev'   => '(wersja: $1)',
'missingarticle-diff'  => '(różnica: $1, $2)',
'readonly_lag'         => 'Baza danych została automatycznie zablokowana na czas potrzebny do wykonania synchronizacji zmian między serwerem głównym i serwerami pośredniczącymi.',
'internalerror'        => 'Błąd wewnętrzny',
'internalerror_info'   => 'Błąd wewnętrzny: $1',
'filecopyerror'        => 'Nie można skopiować pliku „$1” do „$2”.',
'filerenameerror'      => 'Nie można zmienić nazwy pliku „$1” na „$2”.',
'filedeleteerror'      => 'Nie można usunąć pliku „$1”.',
'directorycreateerror' => 'Nie udało się utworzyć katalogu „$1”.',
'filenotfound'         => 'Nie można znaleźć pliku „$1”.',
'fileexistserror'      => 'Nie udało się zapisać do pliku „$1”: plik istnieje',
'unexpected'           => 'Niespodziewana wartość: „$1”=„$2”.',
'formerror'            => 'Błąd: nie można wysłać formularza',
'badarticleerror'      => 'Dla tej strony ta operacja nie może być wykonana.',
'cannotdelete'         => 'Nie można usunąć podanej strony lub grafiki.
Możliwe, że zostały już usunięte przez kogoś innego.',
'badtitle'             => 'Niepoprawny tytuł',
'badtitletext'         => 'Podano niepoprawny tytuł strony. Prawdopodobnie jest pusty lub zawiera znaki, których użycie jest zabronione.',
'perfdisabled'         => 'Uwaga! Możliwość użycia tej funkcjonalności została czasowo zablokowana, ponieważ obniża ona wydajność systemu bazy danych do poziomu uniemożliwiającego komukolwiek skorzystanie z tej wiki.',
'perfcached'           => 'Poniższe dane są kopią z pamięci podręcznej i mogą być nieaktualne.',
'perfcachedts'         => 'Poniższe dane są kopią z pamięci podręcznej. Ostatnia aktualizacja odbyła się $1.',
'querypage-no-updates' => 'Uaktualnienia dla tej strony są obecnie wyłączone. Znajdujące się tutaj dane nie zostaną odświeżone.',
'wrong_wfQuery_params' => 'Nieprawidłowe parametry przekazane do wfQuery()<br />
Funkcja: $1<br />
Zapytanie: $2',
'viewsource'           => 'Tekst źródłowy',
'viewsourcefor'        => 'dla $1',
'actionthrottled'      => 'Akcja wstrzymana',
'actionthrottledtext'  => 'Mechanizm obrony przed spamem ogranicza liczbę wykonań tej czynności w jednostce czasu. Usiłowałeś przekroczyć to ograniczenie. Spróbuj jeszcze raz za kilka minut.',
'protectedpagetext'    => 'Wyłączono możliwość edycji tej strony.',
'viewsourcetext'       => 'Tekst źródłowy strony można podejrzeć i skopiować.',
'protectedinterface'   => 'Ta strona zawiera tekst interfejsu oprogramowania, dlatego możliwość jej edycji została zablokowana.',
'editinginterface'     => "'''Ostrzeżenie:''' Edytujesz stronę, która zawiera tekst interfejsu oprogramowania.
Zmiany na tej stronie zmienią wygląd interfejsu dla innych użytkowników.
Rozważ wykonanie tłumaczenia na [http://translatewiki.net/wiki/Main_Page?setlang=pl Betawiki], specjalizowanym projekcie lokalizacji oprogramowania MediaWiki.",
'sqlhidden'            => '(ukryto zapytanie SQL)',
'cascadeprotected'     => 'Ta strona została zabezpieczona przed edycją, ponieważ jest ona zawarta na {{PLURAL:$1|następującej stronie, która została zabezpieczona|następujących stronach, które zostały zabezpieczone}} z włączoną opcją dziedziczenia:
$2',
'namespaceprotected'   => "Nie masz uprawnień do edytowania stron w przestrzeni nazw '''$1'''.",
'customcssjsprotected' => 'Nie możesz edytować tej strony, ponieważ zawiera ona ustawienia osobiste innego użytkownika.',
'ns-specialprotected'  => 'Stron specjalnych nie można edytować.',
'titleprotected'       => "Utworzenie strony o tej nazwie zostało zablokowane przez [[User:$1|$1]].
Uzasadnienie blokady: ''$2''.",

# Virus scanner
'virus-badscanner'     => 'Zła konfiguracja – nieznany skaner antywirusowy <i>$1</i>',
'virus-scanfailed'     => 'skanowanie nieudane (błąd $1)',
'virus-unknownscanner' => 'nieznany program antywirusowy',

# Login and logout pages
'logouttitle'                => 'Wylogowanie użytkownika',
'logouttext'                 => '<strong>Zostałeś wylogowany.</strong>

Możesz kontynuować pracę w {{GRAMMAR:MS.lp|{{SITENAME}}}} jako niezarejestrowany użytkownik albo [[Special:UserLogin|zalogować się ponownie]] jako ten sam lub inny użytkownik.
Zauważ, że do momentu wyczyszczenia pamięci podręcznej przeglądarki niektóre strony mogą wyglądać tak, jakbyś wciąż był zalogowany.',
'welcomecreation'            => '== Witaj, $1! ==
Twoje konto zostało utworzone.
Nie zapomnij dostosować [[Special:Preferences|preferencji dla {{GRAMMAR:D.lp|{{SITENAME}}}}]].',
'loginpagetitle'             => 'Logowanie',
'yourname'                   => 'Nazwa użytkownika:',
'yourpassword'               => 'Hasło:',
'yourpasswordagain'          => 'Powtórz hasło:',
'remembermypassword'         => 'Zapamiętaj moje hasło na tym komputerze',
'yourdomainname'             => 'Twoja domena',
'externaldberror'            => 'Wystąpił błąd zewnętrznej bazy autentyfikacyjnej lub nie posiadasz uprawnień koniecznych do aktualizacji zewnętrznego konta.',
'loginproblem'               => '<b>Wystąpił problem przy próbie zalogowania.</b><br />Spróbuj ponownie!',
'login'                      => 'Zaloguj się',
'nav-login-createaccount'    => 'Logowanie i rejestracja',
'loginprompt'                => 'Musisz mieć włączoną w przeglądarce obsługę ciasteczek, by móc się zalogować do {{GRAMMAR:D.lp|{{SITENAME}}}}.',
'userlogin'                  => 'Logowanie i rejestracja',
'logout'                     => 'Wyloguj',
'userlogout'                 => 'Wyloguj',
'notloggedin'                => 'Nie jesteś zalogowany',
'nologin'                    => 'Nie masz konta? $1.',
'nologinlink'                => 'Zarejestruj się',
'createaccount'              => 'Załóż nowe konto',
'gotaccount'                 => 'Masz już konto? $1.',
'gotaccountlink'             => 'Zaloguj się',
'createaccountmail'          => '– wyślij w tym celu wiadomość e-mail',
'badretype'                  => 'Wprowadzone hasła różnią się między sobą.',
'userexists'                 => 'Wybrana przez Ciebie nazwa użytkownika jest już zajęta.
Wybierz inną nazwę użytkownika.',
'youremail'                  => 'Twój adres e-mail',
'username'                   => 'Nazwa użytkownika',
'uid'                        => 'ID użytkownika',
'prefs-memberingroups'       => 'Należy do {{PLURAL:$1|grupy|grup:}}',
'yourrealname'               => 'Imię i nazwisko',
'yourlanguage'               => 'Język interfejsu',
'yourvariant'                => 'Wariant',
'yournick'                   => 'Twój podpis',
'badsig'                     => 'Nieprawidłowy podpis, sprawdź znaczniki HTML.',
'badsiglength'               => 'Twój podpis jest zbyt długi.
Dopuszczalna długość to $1 {{PLURAL:$1|znak|znaki|znaków}}.',
'email'                      => 'E-mail',
'prefs-help-realname'        => 'Wpisanie imienia i nazwiska nie jest obowiązkowe.
Jeśli zdecydujesz się je podać, zostaną użyte, by udokumentować Twoje autorstwo.',
'loginerror'                 => 'Błąd zalogowania',
'prefs-help-email'           => "Podanie adresu e-mail nie jest obowiązkowe, lecz pozwoli innym użytkownikom skontaktować się z Tobą poprzez odpowiedni formularz (bez ujawniania Twojego adresu). Będziesz także mógł poprosić o przysłanie Ci nowego hasła. '''Twój adres nie zostanie nikomu udostępniony.'''",
'prefs-help-email-required'  => 'Wymagany jest adres e-mail.',
'nocookiesnew'               => 'Konto użytkownika zostało utworzone, ale nie jesteś zalogowany.
Projekt {{SITENAME}} używa ciasteczek do przechowywania informacji o zalogowaniu się.
Masz obecnie w przeglądarce wyłączoną obsługę ciasteczek. 
Żeby się zalogować, włącz obsługę ciasteczek, następnie podaj nazwę użytkownika i hasło dostępu do swojego konta.',
'nocookieslogin'             => 'Projekt {{SITENAME}} wykorzystuje mechanizm ciasteczek do przechowywania informacji o zalogowaniu się przez użytkownika.
Masz obecnie w przeglądarce wyłączoną obsługę ciasteczek.
Spróbuj ponownie po ich odblokowaniu.',
'noname'                     => 'To nie jest poprawna nazwa użytkownika.',
'loginsuccesstitle'          => 'Zalogowano pomyślnie',
'loginsuccess'               => "'''Zalogowałeś się do {{GRAMMAR:D.lp|{{SITENAME}}}} jako „$1”.'''",
'nosuchuser'                 => 'Brak użytkownika o nazwie „$1”.
Sprawdź pisownię lub [[Special:Userlogin/signup|użyj formularza, by utworzyć nowe konto]].',
'nosuchusershort'            => 'Brak użytkownika o nazwie „<nowiki>$1</nowiki>”.
Sprawdź poprawność pisowni.',
'nouserspecified'            => 'Musisz podać nazwę użytkownika.',
'wrongpassword'              => 'Podane hasło jest nieprawidłowe. Spróbuj jeszcze raz.',
'wrongpasswordempty'         => 'Wprowadzone hasło jest puste. Spróbuj ponownie.',
'passwordtooshort'           => 'Twoje hasło jest błędne lub za krótkie.
Musi mieć co najmniej $1 {{PLURAL:$1|znak|znaki|znaków}} i być inne, niż Twoja nazwa użytkownika.',
'mailmypassword'             => 'Wyślij mi nowe hasło poprzez e-mail',
'passwordremindertitle'      => 'Nowe tymczasowe hasło do {{GRAMMAR:D.lp|{{SITENAME}}}}',
'passwordremindertext'       => 'Ktoś (prawdopodobnie Ty, spod adresu IP $1)
poprosił o przesłanie nowego hasła do {{GRAMMAR:D.lp|{{SITENAME}}}} ($4).
Dla użytkownika „$2” zostało wygenerowane tymczasowe hasło i jest nim „$3”.
Jeśli było to zamierzone działanie, to po zalogowaniu się, musisz podać nowe hasło.

Jeśli to nie Ty prosiłeś o przesłanie hasła lub przypomniałeś sobie hasło i nie chcesz go zmieniać, wystarczy, że zignorujesz tę wiadomość i dalej będziesz się posługiwać swoim dotychczasowym hasłem.',
'noemail'                    => 'Brak zdefiniowanego adresu e-mail dla użytkownika „$1”.',
'passwordsent'               => 'Nowe hasło zostało wysłane na adres e-mail użytkownika „$1”.
Po otrzymaniu go zaloguj się ponownie.',
'blocked-mailpassword'       => 'Twój adres IP został zablokowany i nie możesz używać funkcji odzyskiwania hasła z powodu możliwości jej nadużywania.',
'eauthentsent'               => 'Potwierdzenie zostało wysłane na adres e-mail.
Zanim jakiekolwiek inne wiadomości zostaną wysłane na ten adres, należy wykonać zawarte w mailu instrukcje. Potwierdzisz w ten sposób, że ten adres e-mail należy do Ciebie.',
'throttled-mailpassword'     => 'Przypomnienie hasła zostało już wysłane w ciągu {{PLURAL:$1|ostatniej godziny|ostatnich $1 godzin}}.
W celu powstrzymania nadużyć możliwość wysyłania przypomnień została ograniczona do jednego na {{PLURAL:$1|godzinę|$1 godziny|$1 godzin}}.',
'mailerror'                  => 'W trakcie wysyłania wiadomości e-mail wystąpił błąd: $1',
'acct_creation_throttle_hit' => 'Założyłeś już {{PLURAL:$1|konto|$1 konta|$1 kont}}.
Nie możesz założyć kolejnego.',
'emailauthenticated'         => 'Twój adres e-mail został uwierzytelniony o $1',
'emailnotauthenticated'      => "Twój adres '''e-mail nie został potwierdzony'''.
Poniższe funkcje poczty nie działają.",
'noemailprefs'               => 'Musisz podać adres e-mail, by skorzystać z tych funkcji.',
'emailconfirmlink'           => 'Potwierdź swój adres e-mail',
'invalidemailaddress'        => 'Adres e-mail jest niepoprawny i nie może być zaakceptowany.
Wpisz poprawny adres e-mail lub wyczyść pole.',
'accountcreated'             => 'Konto zostało utworzone',
'accountcreatedtext'         => 'Konto dla $1 zostało utworzone.',
'createaccount-title'        => 'Utworzenie konta w {{GRAMMAR:MS.lp|{{SITENAME}}}}',
'createaccount-text'         => 'Ktoś utworzył w {{GRAMMAR:MS.lp|{{SITENAME}}}} ($4), podając Twój adres e-mail, konto „$2”. Aktualnym hasłem jest „$3”.
Zaloguj się teraz i je zmień.

Możesz zignorować tę wiadomość, jeśli konto zostało utworzone przez pomyłkę.',
'loginlanguagelabel'         => 'Język: $1',

# Password reset dialog
'resetpass'               => 'Resetuj hasło',
'resetpass_announce'      => 'Zalogowałeś się, wykorzystując tymczasowe hasło otrzymane poprzez e-mail.
Aby zakończyć proces logowania, musisz ustawić nowe hasło:',
'resetpass_text'          => '<!-- Dodaj tekst -->',
'resetpass_header'        => 'Resetuj hasło',
'resetpass_submit'        => 'Ustaw hasło i zaloguj się',
'resetpass_success'       => 'Twoje hasło zostało pomyślnie zmienione! Trwa logowanie...',
'resetpass_bad_temporary' => 'Nieprawidłowe hasło tymczasowe.
Być może zakończyłeś już proces zmiany hasła lub poprosiłeś o nowe hasło tymczasowe.',
'resetpass_forbidden'     => 'Haseł użytkowników w {{GRAMMAR:MS.lp|{{SITENAME}}}} nie można zmieniać.',
'resetpass_missing'       => 'Brak danych formularza.',

# Edit page toolbar
'bold_sample'     => 'Tekst tłustą czcionką',
'bold_tip'        => 'Tekst tłustą czcionką',
'italic_sample'   => 'Tekst pochyłą czcionką',
'italic_tip'      => 'Tekst pochyłą czcionką',
'link_sample'     => 'Tytuł linku',
'link_tip'        => 'Link wewnętrzny',
'extlink_sample'  => 'http://www.example.com nazwa linku',
'extlink_tip'     => 'Link zewnętrzny (pamiętaj o przedrostku http:// )',
'headline_sample' => 'Tekst nagłówka',
'headline_tip'    => 'Nagłówek 2. poziomu',
'math_sample'     => 'Tutaj wprowadź wzór',
'math_tip'        => 'Wzór matematyczny (LaTeX)',
'nowiki_sample'   => 'Tutaj wstaw niesformatowany tekst',
'nowiki_tip'      => 'Zignoruj formatowanie wiki',
'image_sample'    => 'Przyklad.jpg',
'image_tip'       => 'Grafika lub inny plik osadzony w stronie',
'media_sample'    => 'Przyklad.ogg',
'media_tip'       => 'Link do pliku',
'sig_tip'         => 'Twój podpis wraz z datą i czasem',
'hr_tip'          => 'Linia pozioma (nie nadużywaj)',

# Edit pages
'summary'                          => 'Opis zmian',
'subject'                          => 'Temat/nagłówek',
'minoredit'                        => 'To jest drobna zmiana',
'watchthis'                        => 'Obserwuj',
'savearticle'                      => 'Zapisz',
'preview'                          => 'Podgląd',
'showpreview'                      => 'Pokaż podgląd',
'showlivepreview'                  => 'Dynamiczny podgląd',
'showdiff'                         => 'Podgląd zmian',
'anoneditwarning'                  => "'''Uwaga:''' Nie jesteś zalogowany.
Twój adres IP będzie zapisany w historii edycji strony.",
'missingsummary'                   => "'''Uwaga:''' Nie wprowadziłeś opisu zmian.
Jeżeli nie chcesz go wprowadzać, naciśnij przycisk Zapisz jeszcze raz.",
'missingcommenttext'               => 'Wprowadź komentarz poniżej.',
'missingcommentheader'             => "'''Uwaga:''' Treść nagłówka jest pusta – uzupełnij go!
Jeśli tego nie zrobisz, Twój komentarz zostanie zapisany bez nagłówka.",
'summary-preview'                  => 'Podgląd opisu',
'subject-preview'                  => 'Podgląd nagłówka',
'blockedtitle'                     => 'Użytkownik jest zablokowany',
'blockedtext'                      => "<big>'''Twoje konto lub adres IP zostały zablokowane.'''</big>

Blokada została nałożona przez $1.
Podany powód to: ''$2''.

* Początek blokady: $8
* Wygaśnięcie blokady: $6
* Cel blokady: $7

W celu wyjaśnienia przyczyny zablokowania możesz się skontaktować z $1 lub innym [[{{MediaWiki:Grouppage-sysop}}|administratorem]].
Nie możesz użyć funkcji „Wyślij e-mail do tego użytkownika”, jeśli brak jest poprawnego adresu e-mail w Twoich [[Special:Preferences|preferencjach]] lub jeśli taka możliwość została Ci zablokowana.
Twój obecny adres IP to $3, a numer identyfikacyjny blokady to $5.
Prosimy o podanie obu tych numerów przy wyjaśnianiu blokady.",
'autoblockedtext'                  => "Ten adres IP został zablokowany automatycznie, gdyż korzysta z niego inny użytkownik, zablokowany przez administratora $1.
Powód blokady:

:''$2''

* Początek blokady: $8
* Wygaśnięcie blokady: $6
* Cel blokady: $7

Możesz skontaktować się z $1 lub jednym z pozostałych [[{{MediaWiki:Grouppage-sysop}}|administratorów]] w celu uzyskania informacji o blokadzie.

Nie możesz użyć funkcji „Wyślij e-mail do tego użytkownika”, jeśli brak jest poprawnego adresu e-mail w Twoich [[Special:Preferences|preferencjach]] lub jeśli taka możliwość została Ci zablokowana.

Twój obecny adres IP to $3, a numer identyfikacyjny blokady to $5.
Prosimy o podanie obu tych numerów przy wyjaśnianiu blokady.",
'blockednoreason'                  => 'nie podano przyczyny',
'blockedoriginalsource'            => "Źródło '''$1''' zostało pokazane poniżej:",
'blockededitsource'                => "Tekst '''Twoich edycji''' na '''$1''' został pokazany poniżej:",
'whitelistedittitle'               => 'Przed edycją musisz się zalogować',
'whitelistedittext'                => 'Musisz $1, by edytować strony.',
'confirmedittitle'                 => 'Edytowanie jest możliwe dopiero po zweryfikowaniu adresu e-mail',
'confirmedittext'                  => 'Edytowanie jest możliwe dopiero po zweryfikowaniu adresu e-mail.
Podaj adres e-mail i potwierdź go w swoich [[Special:Preferences|ustawieniach użytkownika]].',
'nosuchsectiontitle'               => 'Sekcja nie istnieje',
'nosuchsectiontext'                => 'Próbowałeś edytować sekcję, która nie istnieje.
Ponieważ brak sekcji $1, nie jest możliwe zapisanie Twojej edycji.',
'loginreqtitle'                    => 'musisz się zalogować',
'loginreqlink'                     => 'zalogować się',
'loginreqpagetext'                 => 'Musisz $1, żeby móc przeglądać inne strony.',
'accmailtitle'                     => 'Hasło zostało wysłane.',
'accmailtext'                      => 'Hasło użytkownika „$1” zostało wysłane na adres $2.',
'newarticle'                       => '(Nowy)',
'newarticletext'                   => "Brak strony o tym tytule.
Jeśli chcesz ją utworzyć, wpisz treść strony w poniższym polu (więcej informacji odnajdziesz [[{{MediaWiki:Helppage}}|na stronie pomocy]]). 
Jeśli utworzenie nowej strony nie było Twoim zamiarem, wciśnij ''Wstecz'' w swojej przeglądarce.",
'anontalkpagetext'                 => "---- ''To jest strona dyskusji anonimowego użytkownika – takiego, który nie ma jeszcze swojego konta lub nie chce go w tej chwili używać.
By go identyfikować, używamy adresów IP.
Jednak adres IP może być współdzielony przez wielu użytkowników.
Jeśli jesteś anonimowym użytkownikiem i uważasz, że zamieszczone tu komentarze nie są skierowane do Ciebie, [[Special:UserLogin/signup|utwórz konto]] lub [[Special:UserLogin|zaloguj się]] – dzięki temu unikniesz w przyszłości podobnych nieporozumień.''",
'noarticletext'                    => 'Brak strony o tym tytule. Możesz [[Special:Search/{{PAGENAME}}|poszukać {{PAGENAME}} na innych stronach]] lub [{{fullurl:{{FULLPAGENAME}}|action=edit}} utworzyć stronę {{FULLPAGENAME}}].',
'userpage-userdoesnotexist'        => 'Użytkownik „$1” nie jest zarejestrowany. Upewnij się, czy na pewno zamierzałeś utworzyć/zmodyfikować właśnie tę stronę.',
'clearyourcache'                   => "'''Uwaga:''' Zmiany po zapisaniu nowych ustawień mogą nie być widoczne. Należy wyczyścić zawartość pamięci podręcznej przeglądarki internetowej.
*'''Mozilla, Firefox lub Safari –''' przytrzymaj wciśnięty ''Shift'' i kliknij na ''Odśwież'' lub wciśnij ''Ctrl-F5'' lub ''Ctrl-R'' (''Cmd-Shift-R'' na Macintoshu)
*'''Konqueror –''' kliknij przycisk ''Odśwież'' lub wciśnij ''F5''
*'''Opera –''' wyczyść pamięć podręczną w menu ''Narzędzia → Preferencje''
*'''Internet Explorer –''' przytrzymaj ''Ctrl'' i kliknij na ''Odśwież'' lub wciśnij ''Ctrl-F5''",
'usercssjsyoucanpreview'           => '<strong>Podpowiedź:</strong> Użyj przycisku „Podgląd”, aby przetestować nowy arkusz stylów CSS lub kod JavaScript przed jego zapisaniem.',
'usercsspreview'                   => "'''Pamiętaj, że to tylko podgląd arkusza stylów CSS – nic jeszcze nie zostało zapisane!'''",
'userjspreview'                    => "'''Pamiętaj, że to tylko podgląd Twojego kodu JavaScript – nic jeszcze nie zostało zapisane!'''",
'userinvalidcssjstitle'            => "'''Uwaga:''' Brak skórki o nazwie „$1”.
Strony użytkownika zawierające CSS i JavaScript powinny zaczynać się małą literą, np. {{ns:user}}:Foo/monobook.css, w przeciwieństwie do nieprawidłowego {{ns:user}}:Foo/Monobook.css.",
'updated'                          => '(Zmodyfikowano)',
'note'                             => '<strong>Uwaga:</strong>',
'previewnote'                      => '<strong>To jest tylko podgląd – zmiany nie zostały jeszcze zapisane!</strong>',
'previewconflict'                  => 'Podgląd odnosi się do tekstu z górnego pola edycji. Tak będzie wyglądać strona, jeśli zdecydujesz się ją zapisać.',
'session_fail_preview'             => '<strong>Uwaga! Serwer nie może przetworzyć tej edycji z powodu utraty danych sesji.
Spróbuj jeszcze raz.
Jeśli to nie pomoże – [[Special:UserLogout|wyloguj się]] i zaloguj ponownie.</strong>',
'session_fail_preview_html'        => "<strong>Uwaga! Serwer nie może przetworzyć tej edycji z powodu utraty danych sesji.</strong>

''Ponieważ w {{GRAMMAR:MS.lp|{{SITENAME}}}} włączona została opcja „surowy HTML”, podgląd został ukryty w celu zabezpieczenia przed atakami JavaScript.''

<strong>Jeśli jest to uprawniona próba dokonania edycji, spróbuj jeszcze raz.
Jeśli to nie pomoże – [[Special:UserLogout|wyloguj się]] i zaloguj ponownie.</strong>",
'token_suffix_mismatch'            => '<strong>Twoja edycja została odrzucona, ponieważ twój klient pomieszał znaki interpunkcyjne w żetonie edycyjnym.
Twoja edycja została odrzucona by zapobiec zniszczeniu tekstu strony.
Takie problemy zdarzają się w wypadku korzystania z wadliwych anonimowych sieciowych usług proxy.</strong>',
'editing'                          => 'Edytujesz „$1”',
'editingsection'                   => 'Edytujesz „$1” (fragment)',
'editingcomment'                   => 'Edytujesz „$1” (komentarz)',
'editconflict'                     => 'Konflikt edycji: $1',
'explainconflict'                  => "Ktoś zmienił treść strony w trakcie Twojej edycji.
Górne pole zawiera tekst strony aktualnie zapisany w bazie danych.
Twoje zmiany znajdują się w dolnym polu.
By wprowadzić swoje zmiany, musisz zmodyfikować tekst z górnego pola.
'''Tylko''' tekst z górnego pola zostanie zapisany w bazie, gdy wciśniesz „Zapisz”.",
'yourtext'                         => 'Twój tekst',
'storedversion'                    => 'Zapisana wersja',
'nonunicodebrowser'                => '<strong>Uwaga! Twoja przeglądarka nie rozpoznaje poprawnie kodowania UTF-8 (Unicode).
Z tego powodu wszystkie znaki, których przeglądarka nie rozpoznaje, zostały zastąpione ich kodami szesnastkowymi.</strong>',
'editingold'                       => '<strong>Uwaga! Edytujesz inną niż bieżąca wersję tej strony.
Jeśli zapiszesz ją, wszystkie zmiany wykonane w międzyczasie zostaną wycofane.</strong>',
'yourdiff'                         => 'Różnice',
'copyrightwarning'                 => "Wkład do {{GRAMMAR:D.lp|{{SITENAME}}}} jest udostępniany na licencji $2 (szczegóły w $1). Jeśli nie chcesz, żeby Twój tekst był dowolnie zmieniany przez każdego i rozpowszechniany bez ograniczeń, nie umieszczaj go tutaj.<br />
Zapisując swoją edycję, oświadczasz, że ten tekst jest Twoim dziełem lub pochodzi z materiałów dostępnych na zasadach ''public domain'' albo kompatybilnych.
<strong>PROSZĘ NIE UŻYWAĆ MATERIAŁÓW CHRONIONYCH PRAWEM AUTORSKIM BEZ POZWOLENIA WŁAŚCICIELA!</strong>",
'copyrightwarning2'                => "Wszelki wkład w {{GRAMMAR:B.lp|{{SITENAME}}}} może być edytowany, zmieniany lub usunięty przez innych użytkowników.
Jeśli nie chcesz, żeby Twój tekst był dowolnie zmieniany przez każdego i rozpowszechniany bez ograniczeń, nie umieszczaj go tutaj.<br />
Zapisując swoją edycję, oświadczasz, że ten tekst jest Twoim dziełem lub pochodzi z materiałów dostępnych na zasadach ''public domain'' albo kompatybilnych (zobacz także $1).
<strong>PROSZĘ NIE UŻYWAĆ MATERIAŁÓW CHRONIONYCH PRAWEM AUTORSKIM BEZ POZWOLENIA WŁAŚCICIELA!</strong>",
'longpagewarning'                  => '<strong>Ta strona ma {{PLURAL:$1|1 kilobajt|$1 kilobajty|$1 kilobajtów}}. Jeśli to możliwe, spróbuj podzielić tekst na mniejsze części.</strong>',
'longpageerror'                    => '<strong>Błąd! Wprowadzony przez Ciebie tekst ma {{PLURAL:$1|1 kilobajt|$1 kilobajty|$1 kilobajtów}}. Długość tekstu nie może przekraczać {{PLURAL:$2|1 kilobajt|$2 kilobajty|$2 kilobajtów}}. Tekst nie może być zapisany.</strong>',
'readonlywarning'                  => '<strong>Uwaga! Baza danych została zablokowana do celów administracyjnych. W tej chwili nie można zapisać nowej wersji strony. Zapisz jej treść do pliku, używając wytnij/wklej, i zachowaj na później.</strong>',
'protectedpagewarning'             => '<strong>Uwaga! Modyfikacja tej strony została zablokowana. Mogą ją edytować jedynie użytkownicy z uprawnieniami administratora.</strong>',
'semiprotectedpagewarning'         => "'''Uwaga!''' Ta strona została zabezpieczona i tylko zarejestrowani użytkownicy mogą ją edytować.",
'cascadeprotectedwarning'          => "'''Uwaga!''' Ta strona została zabezpieczona i tylko użytkownicy z uprawnieniami administratora mogą ją edytować. Strona ta jest zawarta na {{PLURAL:$1|następującej stronie, która została zabezpieczona|następujących stronach, które zostały zabezpieczone}} z włączoną opcją dziedziczenia:",
'titleprotectedwarning'            => '<strong>Uwaga! Utworzenie strony o tej nazwie zostało zablokowane. Tylko niektórzy użytkownicy mogą ją utworzyć.</strong>',
'templatesused'                    => 'Szablony użyte w tym artykule:',
'templatesusedpreview'             => 'Szablony użyte w tym podglądzie:',
'templatesusedsection'             => 'Szablony użyte w tej sekcji:',
'template-protected'               => '(zabezpieczony)',
'template-semiprotected'           => '(częściowo zabezpieczony)',
'hiddencategories'                 => 'Ta strona jest w {{PLURAL:$1|jednej ukrytej kategorii|$1 ukrytych kategoriach}}:',
'edittools'                        => '<!-- Znajdujący się tutaj tekst zostanie pokazany pod polem edycji i formularzem przesyłania plików. -->',
'nocreatetitle'                    => 'Ograniczono możliwość tworzenia nowych stron',
'nocreatetext'                     => 'W {{GRAMMAR:MS.lp|{{SITENAME}}}} ograniczono możliwość tworzenia nowych stron.
Możesz edytować istniejące strony bądź też [[Special:UserLogin|zalogować się lub utworzyć konto]].',
'nocreate-loggedin'                => 'Nie masz uprawnień do tworzenia stron w {{GRAMMAR:MS.lp|{{SITENAME}}}}.',
'permissionserrors'                => 'Błędy uprawnień',
'permissionserrorstext'            => 'Nie masz uprawnień do tego działania z {{PLURAL:$1|następującej przyczyny|następujących przyczyn}}:',
'permissionserrorstext-withaction' => 'Nie możesz $2, z {{PLURAL:$1|następującego powodu|następujących powodów}}:',
'recreate-deleted-warn'            => "'''Uwaga! Zamierzasz utworzyć stroną, która została wcześniej usunięta.'''

Upewnij się, czy ponowne utworzenie tej strony jest uzasadnione.
Poniżej znajduje się rejestr usunięć tej strony:",

# Parser/template warnings
'expensive-parserfunction-warning'        => 'Uwaga! Ta strona zawiera zbyt wiele wywołań złożonych obliczeniowo funkcji parsera.

Powinno ich być mniej niż $2, a jest obecnie $1.',
'expensive-parserfunction-category'       => 'Strony ze zbyt dużą liczbą wywołań trudnych funkcji parsera',
'post-expand-template-inclusion-warning'  => 'Uwaga: Zbyt duża wielkość wykorzystanych szablonów.
Niektóre szablony nie zostaną użyte.',
'post-expand-template-inclusion-category' => 'Strony, w których przekroczone jest ograniczenie wielkości użytych szablonów',
'post-expand-template-argument-warning'   => 'Uwaga: Strona zawiera co najmniej jeden argument szablonu, który po rozwinięciu jest zbyt duży.
Argument ten będzie pominięty.',
'post-expand-template-argument-category'  => 'Strony, w których użyto szablonu z pominięciem argumentów',

# "Undo" feature
'undo-success' => 'Edycja może zostać wycofana. Porównaj ukazane poniżej różnice między wersjami, a następnie zapisz zmiany.',
'undo-failure' => 'Edycja nie może zostać wycofana z powodu konfliktu z wersjami pośrednimi.',
'undo-norev'   => 'Edycja nie może być cofnięta, ponieważ nie istnieje lub została usunięta.',
'undo-summary' => 'Wycofanie wersji $1 utworzonej przez [[Special:Contributions/$2|$2]] ([[User talk:$2|dyskusja]])',

# Account creation failure
'cantcreateaccounttitle' => 'Nie można utworzyć konta',
'cantcreateaccount-text' => "Tworzenie konta z tego adresu IP ('''$1''') zostało zablokowane przez [[User:$3|$3]].

Podany przez $3 powód to ''$2''",

# History pages
'viewpagelogs'        => 'Zobacz rejestry operacji dla tej strony',
'nohistory'           => 'Ta strona nie ma swojej historii edycji.',
'revnotfound'         => 'Wersja nie została odnaleziona',
'revnotfoundtext'     => 'Żądana, starsza wersja strony nie została odnaleziona. Sprawdź użyty adres URL.',
'currentrev'          => 'Aktualna wersja',
'revisionasof'        => 'Wersja z dnia $1',
'revision-info'       => 'Wersja $2 z dnia $1',
'previousrevision'    => '← poprzednia wersja',
'nextrevision'        => 'następna wersja →',
'currentrevisionlink' => 'przejdź do aktualnej wersji',
'cur'                 => 'bież.',
'next'                => 'następna',
'last'                => 'poprz.',
'page_first'          => 'początek',
'page_last'           => 'koniec',
'histlegend'          => "Wybór porównania: zaznacz kropeczkami dwie wersje do porównania i wciśnij enter lub przycisk ''Porównaj wybrane wersje''.<br />
Legenda: (bież.) – pokaż zmiany od tej wersji do bieżącej,
(poprz.) – pokaż zmiany od wersji poprzedzającej, m – mała (drobna) zmiana",
'deletedrev'          => '[usunięto]',
'histfirst'           => 'od początku',
'histlast'            => 'od końca',
'historysize'         => '({{PLURAL:$1|1 bajt|$1 bajty|$1 bajtów}})',
'historyempty'        => '(pusta)',

# Revision feed
'history-feed-title'          => 'Historia wersji',
'history-feed-description'    => 'Historia wersji tej strony wiki',
'history-feed-item-nocomment' => '$1 o $2', # user at time
'history-feed-empty'          => 'Wybrana strona nie istnieje.
Mogła zostać usunięta lub jej nazwa została zmieniona.
Spróbuj [[Special:Search|poszukać]] tej strony.',

# Revision deletion
'rev-deleted-comment'         => '(komentarz usunięty)',
'rev-deleted-user'            => '(użytkownik usunięty)',
'rev-deleted-event'           => '(wpis usunięty)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">
Wersja tej strony została usunięta i nie jest dostępna publicznie.
Szczegóły mogą znajdować się w [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} rejestrze usunięć].</div>',
'rev-deleted-text-view'       => '<div class="mw-warning plainlinks">
Ta wersja strony została usunięta i nie jest dostępna publicznie.
Jednak jako administrator {{GRAMMAR:D.lp|{{SITENAME}}}} możesz ją obejrzeć.
Powody usunięcia mogą znajdować się w [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} rejestrze usunięć].</div>',
'rev-delundel'                => 'pokaż/ukryj',
'revisiondelete'              => 'Usuń/przywróć wersje',
'revdelete-nooldid-title'     => 'Nie wybrano wersji',
'revdelete-nooldid-text'      => 'Nie wybrano wersji, na których ma zostać wykonana ta operacja,
wybrana wersja nie istnieje lub próbowano ukryć wersję bieżącą.',
'revdelete-selected'          => '{{PLURAL:$2|Zaznaczona wersja|Zaznaczone wersje}} strony [[:$1]]:',
'logdelete-selected'          => 'Zaznaczone {{PLURAL:$1|zdarzenie|zdarzenia}} z rejestru:',
'revdelete-text'              => 'Usunięte wersje będą nadal widoczne w historii strony, ale ich treść nie będzie publicznie dostępna.

Inni administratorzy {{GRAMMAR:D.lp|{{SITENAME}}}} nadal będą mieć dostęp do ukrytych wersji i będą mogli je odtworzyć, chyba że operator serwisu nałożył dodatkowe ograniczenia.',
'revdelete-legend'            => 'Ustaw ograniczenia widoczności dla wersji',
'revdelete-hide-text'         => 'Ukryj tekst wersji',
'revdelete-hide-name'         => 'Ukryj akcję i cel',
'revdelete-hide-comment'      => 'Ukryj komentarz edycji',
'revdelete-hide-user'         => 'Ukryj nazwę użytkownika/adres IP',
'revdelete-hide-restricted'   => 'Wprowadź te ograniczenia dla administratorów i zablokuj ten interfejs',
'revdelete-suppress'          => 'Utajnij informacje przed administratorami, tak samo jak przed innymi',
'revdelete-hide-image'        => 'Ukryj zawartość pliku',
'revdelete-unsuppress'        => 'Wyłącz utajnianie dla odtwarzanej historii zmian',
'revdelete-log'               => 'Komentarz:',
'revdelete-submit'            => 'Zaakceptuj dla wybranych wersji',
'revdelete-logentry'          => 'zmienił widoczność wersji w [[$1]]',
'logdelete-logentry'          => 'zmienił widoczność zdarzenia dla [[$1]]',
'revdelete-success'           => "'''Pomyślnie zmieniono widoczność wersji.'''",
'logdelete-success'           => "'''Pomyślnie zmieniono widoczność zdarzeń.'''",
'revdel-restore'              => 'Zmień widoczność',
'pagehist'                    => 'Historia edycji strony',
'deletedhist'                 => 'Usunięta historia edycji',
'revdelete-content'           => 'zawartość',
'revdelete-summary'           => 'opis zmian',
'revdelete-uname'             => 'nazwa użytkownika',
'revdelete-restricted'        => 'ustaw ograniczenia dla administratorów',
'revdelete-unrestricted'      => 'usuń ograniczenia dla administratorów',
'revdelete-hid'               => 'ukryj $1',
'revdelete-unhid'             => 'nie ukrywaj $1',
'revdelete-log-message'       => '$1 – $2 {{PLURAL:$2|wersja|wersje|wersji}}',
'logdelete-log-message'       => '$1 – $2 {{PLURAL:$2|zdarzenie|zdarzenia|zdarzeń}}',

# Suppression log
'suppressionlog'     => 'Rejestr utajniania',
'suppressionlogtext' => 'Poniżej znajduje się lista usunięć i blokad utajnionych przed administratorami.
Zobacz [[Special:IPBlockList|rejestr blokowania adresów IP]], jeśli chcesz sprawdzić aktualne zakazy i blokady.',

# History merging
'mergehistory'                     => 'Scal historię zmian stron',
'mergehistory-header'              => 'Ta strona pozwala na scalenie historii zmian jednej strony z historią innej, nowszej strony.
Upewnij się, że zmiany będą zapewniać ciągłość historyczną edycji strony.',
'mergehistory-box'                 => 'Scal historię zmian dwóch stron:',
'mergehistory-from'                => 'Strona źródłowa:',
'mergehistory-into'                => 'Strona docelowa:',
'mergehistory-list'                => 'Historia zmian możliwa do scalenia',
'mergehistory-merge'               => 'Następujące zmiany strony [[:$1]] mogą zostać scalone z [[:$2]].
Oznacz w kolumnie kropeczką, która zmiana, łącznie z wcześniejszymi, ma zostać scalona. 
Użycie linków nawigacyjnych kasuje wybór w kolumnie.',
'mergehistory-go'                  => 'Pokaż możliwe do scalenia zmiany',
'mergehistory-submit'              => 'Scal historię zmian',
'mergehistory-empty'               => 'Brak historii zmian do scalenia.',
'mergehistory-success'             => '$3 {{PLURAL:$3|zmiana|zmiany|zmian}} w [[:$1]] z powodzeniem zostało scalonych z [[:$2]].',
'mergehistory-fail'                => 'Scalenie historii zmian jest niewykonalne. Zmień ustawienia parametrów.',
'mergehistory-no-source'           => 'Strona źródłowa $1 nie istnieje.',
'mergehistory-no-destination'      => 'Strona docelowa $1 nie istnieje.',
'mergehistory-invalid-source'      => 'Strona źródłowa musi mieć poprawną nazwę.',
'mergehistory-invalid-destination' => 'Strona docelowa musi mieć poprawną nazwę.',
'mergehistory-autocomment'         => 'Historia [[:$1]] scalona z [[:$2]]',
'mergehistory-comment'             => 'Historia [[:$1]] scalona z [[:$2]]: $3',

# Merge log
'mergelog'           => 'Scalone',
'pagemerge-logentry' => 'scalił [[$1]] z [[$2]] (historia zmian aż do $3)',
'revertmerge'        => 'Rozdziel',
'mergelogpagetext'   => 'Poniżej znajduje się lista ostatnich scaleń historii zmian stron.',

# Diffs
'history-title'           => 'Historia edycji „$1”',
'difference'              => '(Różnice między wersjami)',
'lineno'                  => 'Linia $1:',
'compareselectedversions' => 'porównaj wybrane wersje',
'editundo'                => 'anuluj zmiany',
'diff-multi'              => '(Nie pokazano $1 {{PLURAL:$1|wersji|wersji}} pomiędzy niniejszymi.)',

# Search results
'searchresults'             => 'Wyniki wyszukiwania',
'searchresulttext'          => 'Więcej informacji o przeszukiwaniu {{GRAMMAR:D.lp|{{SITENAME}}}} odnajdziesz na [[{{MediaWiki:Helppage}}|stronach pomocy]].',
'searchsubtitle'            => "Wyniki dla zapytania '''[[:$1]]''' ([[Special:Prefixindex/$1|strony zaczynające się od „$1”]] |
[[Special:WhatLinksHere/$1|strony, które linkują do „$1”]])",
'searchsubtitleinvalid'     => "Dla zapytania '''$1'''",
'noexactmatch'              => "'''Brak strony zatytułowanej „$1”.'''
Możesz [[:$1|utworzyć tę stronę]].",
'noexactmatch-nocreate'     => "'''Brak strony „$1”.'''",
'toomanymatches'            => 'Zbyt wiele elementów pasujących do wzorca, spróbuj innego zapytania',
'titlematches'              => 'Znaleziono w tytułach',
'notitlematches'            => 'Nie znaleziono w tytułach',
'textmatches'               => 'Znaleziono w treści stron',
'notextmatches'             => 'Nie znaleziono w treści stron',
'prevn'                     => '{{PLURAL:$1|poprzedni|poprzednie $1}}',
'nextn'                     => '{{PLURAL:$1|następny|następne $1}}',
'viewprevnext'              => 'Zobacz ($1) ($2) ($3)',
'search-result-size'        => '$1 ({{PLURAL:$2|1 słowo|$2 słowa|$2 słów}})',
'search-result-score'       => 'Trafność: $1%',
'search-redirect'           => '(przekierowanie $1)',
'search-section'            => '(sekcja $1)',
'search-suggest'            => 'Czy chodziło Ci o: $1',
'search-interwiki-caption'  => 'Projekty siostrzane',
'search-interwiki-default'  => 'Wyniki dla $1:',
'search-interwiki-more'     => '(więcej)',
'search-mwsuggest-enabled'  => 'z dynamicznymi propozycjami',
'search-mwsuggest-disabled' => 'bez dynamicznych propozycji',
'search-relatedarticle'     => 'Pokrewne',
'mwsuggest-disable'         => 'Wyłącz dynamiczne podpowiedzi',
'searchrelated'             => 'pokrewne',
'searchall'                 => 'wszystkie',
'showingresults'            => "Poniżej znajduje się lista z {{PLURAL:$1|'''1''' wynikiem|'''$1''' wynikami}}, rozpoczynając od wyniku numer '''$2'''.",
'showingresultsnum'         => "Poniżej znajduje się lista z {{PLURAL:$3|'''1''' wynikiem|'''$3''' wynikami}}, rozpoczynając od wyniku numer '''$2'''.",
'showingresultstotal'       => "Poniżej {{PLURAL:$3|znajduje się wynik wyszukania '''$1'''|znajdują się wyniki wyszukiwania '''$1 – $2''', z ogólnej liczby '''$3'''}}",
'nonefound'                 => "'''Uwaga''': Domyślnie przeszukiwane są wyłącznie niektóre przestrzenie nazw. Spróbuj poprzedzić wyszukiwaną frazę przedrostkiem ''all:'', co spowoduje przeszukanie całej zawartości {{GRAMMAR:D.lp|{{SITENAME}}}} (włącznie ze stronami dyskusji, szablonami itp) lub spróbuj użyć jako przedrostka wybranej, jednej przestrzeni nazw.",
'powersearch'               => 'Szukaj',
'powersearch-legend'        => 'Wyszukiwanie zaawansowane',
'powersearch-ns'            => 'Przeszukaj przestrzenie nazw:',
'powersearch-redir'         => 'Pokaż przekierowania',
'powersearch-field'         => 'Szukaj',
'search-external'           => 'Wyszukiwanie zewnętrzne',
'searchdisabled'            => 'Wyszukiwanie w {{GRAMMAR:MS.lp|{{SITENAME}}}} zostało wyłączone.
W międzyczasie możesz skorzystać z wyszukiwania Google.
Jednak informacje o treści {{GRAMMAR:D.lp|{{SITENAME}}}} mogą być w Google nieaktualne.',

# Preferences page
'preferences'              => 'Preferencje',
'mypreferences'            => 'preferencje',
'prefs-edits'              => 'Liczba edycji',
'prefsnologin'             => 'Nie jesteś zalogowany',
'prefsnologintext'         => 'Musisz się <span class="plainlinks">[{{fullurl:Special:Userlogin|returnto=$1}} zalogować]</span> przed zmianą swoich preferencji.',
'prefsreset'               => 'Preferencje domyślne zostały odtworzone.',
'qbsettings'               => 'Pasek szybkiego dostępu',
'qbsettings-none'          => 'Brak',
'qbsettings-fixedleft'     => 'Stały, z lewej',
'qbsettings-fixedright'    => 'Stały, z prawej',
'qbsettings-floatingleft'  => 'Unoszący się, z lewej',
'qbsettings-floatingright' => 'Unoszący się, z prawej',
'changepassword'           => 'Zmiana hasła',
'skin'                     => 'Skórka',
'math'                     => 'Wzory',
'dateformat'               => 'Format daty',
'datedefault'              => 'Domyślny',
'datetime'                 => 'Data i czas',
'math_failure'             => 'Parser nie mógł rozpoznać',
'math_unknown_error'       => 'nieznany błąd',
'math_unknown_function'    => 'nieznana funkcja',
'math_lexing_error'        => 'błędna nazwa',
'math_syntax_error'        => 'błąd składni',
'math_image_error'         => 'Konwersja do formatu PNG nie powiodła się.
Sprawdź, czy poprawnie zainstalowane są latex, dvips, gs i convert.',
'math_bad_tmpdir'          => 'Nie można utworzyć lub zapisywać w tymczasowym katalogu dla wzorów matematycznych',
'math_bad_output'          => 'Nie można utworzyć lub zapisywać w wyjściowym katalogu dla wzorów matematycznych',
'math_notexvc'             => 'Brak programu texvc.
Zapoznaj się z math/README w celu konfiguracji.',
'prefs-personal'           => 'Dane użytkownika',
'prefs-rc'                 => 'Ostatnie zmiany',
'prefs-watchlist'          => 'Obserwowane',
'prefs-watchlist-days'     => 'Liczba dni widocznych na liście obserwowanych',
'prefs-watchlist-edits'    => 'Liczba edycji pokazywanych w rozszerzonej liście obserwowanych',
'prefs-misc'               => 'Ustawienia różne',
'saveprefs'                => 'Zapisz',
'resetprefs'               => 'Cofnij niezapisane zmiany',
'oldpassword'              => 'Stare hasło',
'newpassword'              => 'Nowe hasło',
'retypenew'                => 'Powtórz nowe hasło',
'textboxsize'              => 'Edytowanie',
'rows'                     => 'Wiersze',
'columns'                  => 'Kolumny',
'searchresultshead'        => 'Wyszukiwanie',
'resultsperpage'           => 'Liczba wyników na stronie',
'contextlines'             => 'Pierwsze wiersze stron',
'contextchars'             => 'Litery kontekstu w linijce',
'stub-threshold'           => 'Maksymalny (w bajtach) rozmiar strony oznaczanej jako <a href="#" class="stub">zalążek (stub)</a>',
'recentchangesdays'        => 'Liczba dni do pokazania w ostatnich zmianach',
'recentchangescount'       => 'Liczba pozycji na liście ostatnich zmian, w historii stron i na stronach rejestrów:',
'savedprefs'               => 'Twoje preferencje zostały zapisane.',
'timezonelegend'           => 'Strefa czasowa',
'timezonetext'             => '¹Liczba godzin różnicy między Twoim czasem lokalnym, a czasem uniwersalnym (UTC).',
'localtime'                => 'Twój czas lokalny',
'timezoneoffset'           => 'Różnica¹',
'servertime'               => 'Aktualny czas serwera',
'guesstimezone'            => 'Pobierz z przeglądarki',
'allowemail'               => 'Zgadzam się, by inni użytkownicy mogli przesyłać mi e-maile',
'prefs-searchoptions'      => 'Opcje wyszukiwania',
'prefs-namespaces'         => 'Przestrzenie nazw',
'defaultns'                => 'Domyślnie przeszukuj przestrzenie nazw',
'default'                  => 'domyślnie',
'files'                    => 'Pliki',

# User rights
'userrights'                  => 'Zarządzaj uprawnieniami użytkowników', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => 'Zarządzaj grupami użytkownika',
'userrights-user-editname'    => 'Wprowadź nazwę użytkownika',
'editusergroup'               => 'Edytuj grupy użytkownika',
'editinguser'                 => "Zmiana uprawnień użytkownika '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup'    => 'Edytuj grupy użytkownika',
'saveusergroups'              => 'Zapisz',
'userrights-groupsmember'     => 'Należy do:',
'userrights-groups-help'      => 'Możesz zmienić przynależność tego użytkownika do podanych grup:
* Zaznaczone pole oznacza przynależność użytkownika do danej grupy.
* Niezaznaczone pole oznacza, że użytkownik nie należy do danej grupy.
* Gwiazdka * informuje, że nie możesz usunąć z grupy po dodaniu do niej lub dodać po usunięciu z grupy.',
'userrights-reason'           => 'Powód zmiany',
'userrights-no-interwiki'     => 'Nie masz dostępu do edycji uprawnień na innych wiki.',
'userrights-nodatabase'       => 'Baza danych $1 nie istnieje lub nie jest lokalna.',
'userrights-nologin'          => 'Musisz [[Special:UserLogin|zalogować się]] na konto administratora, by nadawać uprawnienia użytkownikom.',
'userrights-notallowed'       => 'Nie masz dostępu do nadawania uprawnień użytkownikom.',
'userrights-changeable-col'   => 'Grupy, które możesz wybrać',
'userrights-unchangeable-col' => 'Grupy, których nie możesz wybrać',

# Groups
'group'               => 'Grupa',
'group-user'          => 'Użytkownicy',
'group-autoconfirmed' => 'Automatycznie zatwierdzani użytkownicy',
'group-bot'           => 'Boty',
'group-sysop'         => 'Administratorzy',
'group-bureaucrat'    => 'Biurokraci',
'group-suppress'      => 'Rewizorzy',
'group-all'           => '(wszyscy)',

'group-user-member'          => 'użytkownik',
'group-autoconfirmed-member' => 'automatycznie zatwierdzony użytkownik',
'group-bot-member'           => 'bot',
'group-sysop-member'         => 'administrator',
'group-bureaucrat-member'    => 'biurokrata',
'group-suppress-member'      => 'rewizor',

'grouppage-user'          => '{{ns:project}}:Użytkownicy',
'grouppage-autoconfirmed' => '{{ns:project}}:Automatycznie zatwierdzeni użytkownicy',
'grouppage-bot'           => '{{ns:project}}:Boty',
'grouppage-sysop'         => '{{ns:project}}:Administratorzy',
'grouppage-bureaucrat'    => '{{ns:project}}:Biurokraci',
'grouppage-suppress'      => '{{ns:project}}:Rewizorzy',

# Rights
'right-read'                 => 'Czytanie treści stron',
'right-edit'                 => 'Edycja stron',
'right-createpage'           => 'Tworzenie stron (nie będących stronami dyskusji)',
'right-createtalk'           => 'Tworzenie stron dyskusji',
'right-createaccount'        => 'Tworzenie kont użytkowników',
'right-minoredit'            => 'Oznaczanie edycji jako drobnych',
'right-move'                 => 'Przenoszenie stron',
'right-move-subpages'        => 'Przenoszenie stron razem z ich podstronami',
'right-suppressredirect'     => 'Przenoszenie stron bez tworzenia przekierowania w miejscu starej nazwy',
'right-upload'               => 'Przesyłanie plików na serwer',
'right-reupload'             => 'Nadpisywanie istniejącego pliku',
'right-reupload-own'         => 'Nadpisywanie istniejącego, wcześniej przesłanego pliku',
'right-reupload-shared'      => 'Lokalne nadpisywanie pliku istniejącego we współdzielonych zasobach',
'right-upload_by_url'        => 'Przesyłanie plików z adresu URL',
'right-purge'                => 'Czyszczenie pamięci podręcznej stron bez pytania o potwierdzenie',
'right-autoconfirmed'        => 'Edycja stron częściowo zabezpieczonych',
'right-bot'                  => 'Oznaczanie edycji jako automatycznie',
'right-nominornewtalk'       => 'Drobne zmiany na stronach dyskusji użytkowników nie włączają powiadomienia o nowej wiadomości',
'right-apihighlimits'        => 'Zwiększony limit w zapytaniach, wykonywanych poprzez interfejs API',
'right-writeapi'             => 'Zapisu poprzez interfejs API',
'right-delete'               => 'Usuwanie stron',
'right-bigdelete'            => 'Usuwanie stron z długą historią edycji',
'right-deleterevision'       => 'Usuwanie i odtwarzanie określonej wersji strony',
'right-deletedhistory'       => 'Podgląd usuniętych wersji, bez przypisanego im tekstu',
'right-browsearchive'        => 'Przeszukiwanie usuniętych stron',
'right-undelete'             => 'Odtwarzanie usuniętych stron',
'right-suppressrevision'     => 'Podgląd i odtwarzanie wersji ukrytych przed Administratorami',
'right-suppressionlog'       => 'Podgląd rejestru ukrywania',
'right-block'                => 'Blokowanie użytkownikom możliwości edycji',
'right-blockemail'           => 'Blokowanie wysyłania wiadomości przez użytkownika',
'right-hideuser'             => 'Blokowanie użytkownika, niewidoczne publicznie',
'right-ipblock-exempt'       => 'Obejście blokad, automatycznych blokad i blokad zakresów, adresów IP',
'right-proxyunbannable'      => 'Obejście automatycznych blokad proxy',
'right-protect'              => 'Zmiana poziomu zabezpieczenia i dostęp do edycji zabezpieczonych stron',
'right-editprotected'        => 'Dostęp do edycji zabezpieczonych stron (bez zabezpieczenia dziedziczonego)',
'right-editinterface'        => 'Edycja interfejsu użytkownika',
'right-editusercssjs'        => 'Edycja plików CSS i JS innych użytkowników',
'right-rollback'             => 'Szybkie cofnięcie edycji użytkownika, który jako ostatni edytował jakąś stronę',
'right-markbotedits'         => 'Oznaczanie rewertu jako edycji bota',
'right-noratelimit'          => 'Brak ograniczeń przepustowości',
'right-import'               => 'Import stron z innych wiki',
'right-importupload'         => 'Import stron poprzez przesłanie pliku',
'right-patrol'               => 'Oznaczanie edycji jako „sprawdzone”',
'right-autopatrol'           => 'Edycje automatycznie oznaczane jako „sprawdzone”',
'right-patrolmarks'          => 'Podgląd znaczników patrolowania ostatnich zmian – oznaczania jako „sprawdzone”',
'right-unwatchedpages'       => 'Podgląd listy stron nieobserwowanych',
'right-trackback'            => 'Wysyłanie trackback',
'right-mergehistory'         => 'Łączenie historii edycji stron',
'right-userrights'           => 'Edycja uprawnień wszystkich użytkowników',
'right-userrights-interwiki' => 'Edycja uprawnień użytkowników innych witryn wiki',
'right-siteadmin'            => 'Blokowanie i odblokowywanie bazy danych',

# User rights log
'rightslog'      => 'Uprawnienia',
'rightslogtext'  => 'Rejestr zmian uprawnień użytkowników.',
'rightslogentry' => 'zmienił przynależność $1 do grup ($2 → $3)',
'rightsnone'     => 'brak',

# Recent changes
'nchanges'                          => '$1 {{PLURAL:$1|zmiana|zmiany|zmian}}',
'recentchanges'                     => 'Ostatnie zmiany',
'recentchangestext'                 => 'Ta strona przedstawia historię ostatnich zmian w tej wiki.',
'recentchanges-feed-description'    => 'Obserwuj najświeższe zmiany w tej wiki.',
'rcnote'                            => "Poniżej {{PLURAL:$1|znajduje się '''1''' ostatnia zmiana wykonana|znajdują się ostatnie '''$1''' zmiany wykonane|znajduje się ostatnich '''$1''' zmian wykonanych}} w ciągu {{PLURAL:$2|ostatniego dnia|ostatnich '''$2''' dni}}, licząc od $5 dnia $4.",
'rcnotefrom'                        => "Poniżej pokazano zmiany wykonane po '''$2''' (nie więcej niż '''$1''' pozycji).",
'rclistfrom'                        => 'Pokaż nowe zmiany od $1',
'rcshowhideminor'                   => '$1 drobne zmiany',
'rcshowhidebots'                    => '$1 boty',
'rcshowhideliu'                     => '$1 zalogowanych',
'rcshowhideanons'                   => '$1 anonimowych',
'rcshowhidepatr'                    => '$1 sprawdzone',
'rcshowhidemine'                    => '$1 moje edycje',
'rclinks'                           => 'Pokaż ostatnie $1 zmian w ciągu ostatnich $2 dni.<br />$3',
'diff'                              => 'różn.',
'hist'                              => 'hist.',
'hide'                              => 'Ukryj',
'show'                              => 'Pokaż',
'minoreditletter'                   => 'm',
'newpageletter'                     => 'N',
'boteditletter'                     => 'b',
'number_of_watching_users_pageview' => '[$1 {{PLURAL:$1|obserwujący użytkownik|obserwujących użytkowników}}]',
'rc_categories'                     => 'Ogranicz do kategorii (oddzielaj za pomocą „|”)',
'rc_categories_any'                 => 'Wszystkie',
'newsectionsummary'                 => '/* $1 */ nowa sekcja',

# Recent changes linked
'recentchangeslinked'          => 'Zmiany w dolinkowanych',
'recentchangeslinked-title'    => 'Zmiany w linkowanych z „$1”',
'recentchangeslinked-noresult' => 'Nie było żadnych zmian na (zależnie od ustawień) linkowanych lub linkujących stronach w wybranym okresie.',
'recentchangeslinked-summary'  => "Poniżej znajduje się lista ostatnich zmian na stronach linkowanych z podanej strony (lub we wszystkich stronach należących do podanej kategorii).
Strony z [[Special:Watchlist|listy obserwowanych]] są '''wytłuszczone'''.",
'recentchangeslinked-page'     => 'Tytuł strony',
'recentchangeslinked-to'       => 'Pokaż zmiany nie na stronach linkowanych, a na stronach linkujących do podanej strony',

# Upload
'upload'                      => 'Prześlij plik',
'uploadbtn'                   => 'Prześlij plik',
'reupload'                    => 'Prześlij ponownie',
'reuploaddesc'                => 'Przerwij wysyłanie i wróć do formularza wysyłki',
'uploadnologin'               => 'Nie jesteś zalogowany',
'uploadnologintext'           => 'Musisz się [[Special:UserLogin|zalogować]] przed przesłaniem plików.',
'upload_directory_missing'    => 'Katalog dla przesyłanych plików ($1) nie istnieje i nie może zostać utworzony przez serwer WWW.',
'upload_directory_read_only'  => 'Serwer nie może zapisywać do katalogu ($1) przeznaczonego na przesyłane pliki.',
'uploaderror'                 => 'Błąd wysyłania',
'uploadtext'                  => "Użyj poniższego formularza do przesłania plików.
Jeśli chcesz przejrzeć lub przeszukać dotychczas przesłane pliki, przejdź do [[Special:ImageList|listy plików]]. Każde przesłanie jest odnotowane w [[Special:Log/upload|rejestrze przesyłanych plików]], a usunięcie w [[Special:Log/delete|rejestrze usuniętych]].

Plik pojawi się na stronie, jeśli użyjesz linku według jednego z następujących wzorów:
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:Plik.jpg]]</nowiki></tt>''' pokaże plik w pełnej postaci
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:Plik.png|200px|thumb|left|podpis grafiki]]</nowiki></tt>''' pokaże szeroką na 200 pikseli miniaturkę umieszczoną przy lewym marginesie, otoczoną ramką, z podpisem „podpis grafiki”
* '''<tt><nowiki>[[</nowiki>{{ns:media}}<nowiki>:Plik.ogg]]</nowiki></tt>''' utworzy bezpośredni link do pliku bez wyświetlania samego pliku",
'upload-permitted'            => 'Dopuszczalne formaty plików: $1.',
'upload-preferred'            => 'Zalecane formaty plików: $1.',
'upload-prohibited'           => 'Zabronione formaty plików: $1.',
'uploadlog'                   => 'rejestr przesyłania plików',
'uploadlogpage'               => 'Przesłane',
'uploadlogpagetext'           => 'Lista ostatnio przesłanych plików.
Przejdź na stronę [[Special:NewImages|galerii nowych plików]], by zobaczyć pliki jako miniaturki.',
'filename'                    => 'Nazwa pliku',
'filedesc'                    => 'Opis',
'fileuploadsummary'           => 'Opis',
'filestatus'                  => 'Status prawny',
'filesource'                  => 'Źródło',
'uploadedfiles'               => 'Przesłane pliki',
'ignorewarning'               => 'Zignoruj ostrzeżenia i wymuś zapisanie pliku.',
'ignorewarnings'              => 'Ignoruj wszystkie ostrzeżenia',
'minlength1'                  => 'Nazwa pliku musi składać się co najmniej z jednej litery.',
'illegalfilename'             => 'Nazwa pliku „$1” zawiera znaki niedozwolone w tytułach stron.
Zmień nazwę pliku i prześlij go ponownie.',
'badfilename'                 => 'Nazwa pliku została zmieniona na „$1”.',
'filetype-badmime'            => 'Przesyłanie plików z typem MIME „$1” jest niedozwolone.',
'filetype-unwanted-type'      => "'''„.$1”''' nie jest zalecanym typem pliku. Pożądane są pliki w {{PLURAL:$3|formacie|formatach}} $2.",
'filetype-banned-type'        => "'''„.$1”''' jest niedozwolonym typem pliku. Dopuszczalne są pliki w {{PLURAL:$3|formacie|formatach}} $2.",
'filetype-missing'            => 'Plik nie ma rozszerzenia (np. „.jpg”).',
'large-file'                  => 'Zalecane jest aby rozmiar pliku nie był większy niż {{PLURAL:$1|1 bajt|$1 bajty|$1 bajtów}}.
Plik ma rozmiar {{PLURAL:$2|1 bajt|$2 bajty|$2 bajtów}}.',
'largefileserver'             => 'Plik jest większy niż maksymalny dozwolony rozmiar.',
'emptyfile'                   => 'Przesłany plik wydaje się być pusty. Może być to spowodowane literówką w nazwie pliku.
Sprawdź, czy nazwa jest prawidłowa.',
'fileexists'                  => 'Plik o takiej nazwie już istnieje. Sprawdź <strong><tt>$1</tt></strong>, jeśli nie jesteś pewien czy chcesz go zastąpić.',
'filepageexists'              => 'Istnieje już strona opisu tego pliku utworzona <strong><tt>$1</tt></strong>, ale brak obecnie pliku o tej nazwie.
Informacje o pliku, które wprowadziłeś, nie pojawią się na stronie opisu.
Jeśli chcesz, by informacje te zostały pokazane, musisz je ręcznie przeredagować',
'fileexists-extension'        => 'Plik o podobnej nazwie już istnieje:<br />
Nazwa przesyłanego pliku: <strong><tt>$1</tt></strong><br />
Nazwa istniejącego pliku: <strong><tt>$2</tt></strong><br />
Wybierz inną nazwę.',
'fileexists-thumb'            => "<center>'''Istniejący plik'''</center>",
'fileexists-thumbnail-yes'    => 'Plik wydaje się być pomniejszoną grafiką <i>(miniaturką)</i>.
Sprawdź plik <strong><tt>$1</tt></strong>.<br />
Jeśli wybrany plik jest tą samą grafiką co ta w oryginalnym rozmiarze, nie musisz przesyłać dodatkowej miniaturki.',
'file-thumbnail-no'           => 'Nazwa pliku zaczyna się od <strong><tt>$1</tt></strong>.
Wydaje się, że jest to pomniejszona grafika <i>(miniaturka)</i>.
Jeśli posiadasz tę grafikę w pełnym rozmiarze – prześlij ją. Jeśli chcesz wysłać tę – zmień nazwę przesyłanego obecnie pliku.',
'fileexists-forbidden'        => 'Plik o tej nazwie już istnieje.
Cofnij się i załaduj plik pod inną nazwą. [[Image:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => 'Plik o tej nazwie już istnieje we współdzielonym repozytorium plików.
Cofnij się i załaduj plik pod inną nazwą. [[Image:$1|thumb|center|$1]]',
'file-exists-duplicate'       => 'Ten plik jest kopią {{PLURAL:$1|pliku|następujących plików:}}',
'successfulupload'            => 'Przesłanie pliku powiodło się',
'uploadwarning'               => 'Ostrzeżenie o przesyłce',
'savefile'                    => 'Zapisz plik',
'uploadedimage'               => 'przesłał [[$1]]',
'overwroteimage'              => 'przesłał nową wersję [[$1]]',
'uploaddisabled'              => 'Przesyłanie plików wyłączone',
'uploaddisabledtext'          => 'Funkcjonalność przesyłania plików została wyłączona w {{GRAMMAR:MS.lp|{{SITENAME}}}}.',
'uploadscripted'              => 'Plik zawiera kod HTML lub skrypt, który może zostać błędnie zinterpretowany przez przeglądarkę internetową.',
'uploadcorrupt'               => 'Plik jest uszkodzony lub ma nieprawidłowe rozszerzenie.
Sprawdź plik i załaduj poprawną wersję.',
'uploadvirus'                 => 'W pliku jest wirus! Szczegóły: $1',
'sourcefilename'              => 'Nazwa oryginalna',
'destfilename'                => 'Nazwa docelowa',
'upload-maxfilesize'          => 'Wielkość pliku jest ograniczona do $1',
'watchthisupload'             => 'Obserwuj',
'filewasdeleted'              => 'Plik o tej nazwie istniał, ale został usunięty.
Zanim załadujesz go ponownie, sprawdź $1.',
'upload-wasdeleted'           => "'''Uwaga! Ładujesz plik, który został usunięty.'''

Zastanów się, czy powinno się ładować ten plik.
Rejestr usunięć tego pliku jest podany poniżej:",
'filename-bad-prefix'         => 'Nazwa pliku, który przesyłasz, zaczyna się od <strong>„$1”</strong>. Jest to nazwa zazwyczaj przypisywana automatycznie przez cyfrowe aparaty fotograficzne, która nie informuje o zawartości pliku.
Zmień nazwę pliku na bardziej opisową.',
'filename-prefix-blacklist'   => '  #<!-- nie modyfikuj tej linii --> <pre>
# Składnia jest następująca:
#  * Wszystko od znaku "#" do końca linii uznawane jest za komentarz
#  * Każda niepusta linia zawiera początek nazwy pliku domyślnie wykorzystywany przez aparaty cyfrowe
CIMG # Casio
DSC_ # Nikon
DSCF # Fuji
DSCN # Nikon
DUW # niektóre telefony komórkowe
IMG # ogólny
JD # Jenoptik
MGP # Pentax
PICT # wiele różnych
  #</pre> <!-- nie modyfikuj tej linii -->',

'upload-proto-error'      => 'Nieprawidłowy protokół',
'upload-proto-error-text' => 'Zdalne przesyłanie plików wymaga podania adresu URL zaczynającego się od <code>http://</code> lub <code>ftp://</code>.',
'upload-file-error'       => 'Błąd wewnętrzny',
'upload-file-error-text'  => 'Wystąpił błąd wewnętrzny podczas próby utworzenia tymczasowego pliku na serwerze.
Skontaktuj się z [[Special:ListUsers/sysop|administratorem systemu]].',
'upload-misc-error'       => 'Nieznany błąd przesyłania',
'upload-misc-error-text'  => 'Wystąpił nieznany błąd podczas przesyłania.
Sprawdź, czy podany adres URL jest poprawny i dostępny, a następnie spróbuj ponownie.
Jeśli problem będzie się powtarzał, skontaktuj się z [[Special:ListUsers/sysop|administratorem systemu]].',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => 'Adres URL jest nieosiągalny',
'upload-curl-error6-text'  => 'Podany adres URL jest nieosiągalny. Upewnij się, czy podany adres URL jest prawidłowy i czy dana strona jest dostępna.',
'upload-curl-error28'      => 'Upłynął limit czasu odpowiedzi',
'upload-curl-error28-text' => 'Zbyt długi czas odpowiedzi serwera.
Sprawdź, czy strona działa, odczekaj kilka minut i spróbuj ponownie.
Możesz także spróbować w czasie mniejszego obciążenia serwera.',

'license'            => 'Licencja',
'nolicense'          => 'Nie wybrano',
'license-nopreview'  => '(Podgląd niedostępny)',
'upload_source_url'  => ' (poprawny, publicznie dostępny adres URL)',
'upload_source_file' => ' (plik na twoim komputerze)',

# Special:ImageList
'imagelist-summary'     => 'Na tej stronie specjalnej prezentowane są wszystkie pliki przesłane na serwer.
Domyślnie na górze listy umieszczane są ostatnio przesłane pliki.
Kliknięcie w nagłówek kolumny zmienia sposób sortowania.',
'imagelist_search_for'  => 'Szukaj pliku o nazwie',
'imgfile'               => 'plik',
'imagelist'             => 'Lista plików',
'imagelist_date'        => 'Data',
'imagelist_name'        => 'Nazwa',
'imagelist_user'        => 'Użytkownik',
'imagelist_size'        => 'Wielkość',
'imagelist_description' => 'Opis',

# Image description page
'filehist'                       => 'Historia pliku',
'filehist-help'                  => 'Kliknij na datę/czas, aby zobaczyć, jak plik wyglądał w tym czasie.',
'filehist-deleteall'             => 'usuń wszystkie',
'filehist-deleteone'             => 'usuń',
'filehist-revert'                => 'cofnij',
'filehist-current'               => 'aktualny',
'filehist-datetime'              => 'Data/czas',
'filehist-user'                  => 'Użytkownik',
'filehist-dimensions'            => 'Wymiary',
'filehist-filesize'              => 'Rozmiar pliku',
'filehist-comment'               => 'Opis',
'imagelinks'                     => 'Odnośniki do pliku',
'linkstoimage'                   => '{{PLURAL:$1|Poniższa strona odwołuje|Następujące strony odwołują}} się do tego pliku:',
'nolinkstoimage'                 => 'Żadna strona nie odwołuje się do tego pliku.',
'morelinkstoimage'               => 'Pokaż [[Special:WhatLinksHere/$1|więcej odnośników]] do tego pliku.',
'redirectstofile'                => '{{PLURAL:$1|Następujący plik przekierowuje|Następujące pliki przekierowują}} do tego pliku:',
'duplicatesoffile'               => '{{PLURAL:$1|Następujący plik jest kopią|Następujące pliki są kopiami}} tego pliku:',
'sharedupload'                   => 'Ten plik znajduje się na wspólnym serwerze plików i może być używany w innych projektach.',
'shareduploadwiki'               => 'Więcej informacji odnajdziesz na $1.',
'shareduploadwiki-desc'          => 'Opis znajdujący się na $1 we współdzielonych zasobach możesz zobaczyć poniżej.',
'shareduploadwiki-linktext'      => 'stronie opisu pliku',
'shareduploadduplicate'          => 'Ten plik jest identyczny z $1 znajdującym się we współdzielonych zasobach.',
'shareduploadduplicate-linktext' => 'innym plikiem',
'shareduploadconflict'           => 'Plik ma taką samą nazwę jak $1 znajdujący się we współdzielonych zasobach.',
'shareduploadconflict-linktext'  => 'inny plik',
'noimage'                        => 'Nie istnieje plik o tej nazwie. Możesz go $1.',
'noimage-linktext'               => 'przesłać',
'uploadnewversion-linktext'      => 'Załaduj nowszą wersję tego pliku',
'imagepage-searchdupe'           => 'Wyszukiwanie powtarzających się plików',

# File reversion
'filerevert'                => 'Przywracanie $1',
'filerevert-legend'         => 'Przywracanie poprzedniej wersji pliku',
'filerevert-intro'          => "Zamierzasz przywrócić '''[[Media:$1|$1]]''' do [wersji $4 z $3, $2].",
'filerevert-comment'        => 'Komentarz:',
'filerevert-defaultcomment' => 'Przywrócono wersję z $2, $1',
'filerevert-submit'         => 'Przywróć',
'filerevert-success'        => "Plik '''[[Media:$1|$1]]''' został cofnięty do [wersji $4 z $3, $2].",
'filerevert-badversion'     => 'Brak poprzedniej lokalnej wersji tego pliku z podaną datą.',

# File deletion
'filedelete'                  => 'Usuń „$1”',
'filedelete-legend'           => 'Usuń plik',
'filedelete-intro'            => "Usuwasz '''[[Media:$1|$1]]'''.",
'filedelete-intro-old'        => "Usuwasz wersję pliku '''[[Media:$1|$1]]''' z datą [$4 $3, $2].",
'filedelete-comment'          => 'Komentarz:',
'filedelete-submit'           => 'Usuń',
'filedelete-success'          => "Usunięto plik '''$1'''.",
'filedelete-success-old'      => "Usunięto plik '''[[Media:$1|$1]]''' w wersji z $3, $2.",
'filedelete-nofile'           => "Plik '''$1''' nie istnieje.",
'filedelete-nofile-old'       => "Brak zarchiwizowanej wersji '''$1''' o podanych atrybutach.",
'filedelete-iscurrent'        => 'Próbujesz usunąć najnowszą wersję tego pliku.
Musisz najpierw przywrócić starszą wersję.',
'filedelete-otherreason'      => 'Inny (dodatkowy) powód:',
'filedelete-reason-otherlist' => 'Inny powód',
'filedelete-reason-dropdown'  => '* Najczęstsze przyczyny usunięcia
** Naruszenie praw autorskich
** Kopia już istniejącego pliku',
'filedelete-edit-reasonlist'  => 'Edycja listy powodów usunięcia pliku',

# MIME search
'mimesearch'         => 'Wyszukiwanie MIME',
'mimesearch-summary' => 'Ta strona umożliwia wyszukiwanie plików ze względu na ich typ MIME.
Użycie: typ_treści/podtyp, np. <tt>image/jpeg</tt>.',
'mimetype'           => 'Typ MIME',
'download'           => 'pobierz',

# Unwatched pages
'unwatchedpages' => 'Nieobserwowane strony',

# List redirects
'listredirects' => 'Lista przekierowań',

# Unused templates
'unusedtemplates'     => 'Nieużywane szablony',
'unusedtemplatestext' => 'Poniżej znajduje się lista wszystkich stron znajdujących się w przestrzeni nazw przeznaczonej dla szablonów, które nie są używane przez inne strony.
Sprawdź inne linki do szablonów, zanim usuniesz tę stronę.',
'unusedtemplateswlh'  => 'inne linkujące',

# Random page
'randompage'         => 'Losuj stronę',
'randompage-nopages' => 'Brak jakichkolwiek stron w tej przestrzeni nazw.',

# Random redirect
'randomredirect'         => 'Losowe przekierowanie',
'randomredirect-nopages' => 'Brak przekierowań w tej przestrzeni nazw.',

# Statistics
'statistics'             => 'Statystyki',
'sitestats'              => 'Statystyka {{GRAMMAR:D.lp|{{SITENAME}}}}',
'userstats'              => 'Statystyka użytkowników',
'sitestatstext'          => "W bazie danych {{PLURAL:$1|jest '''1''' strona|są '''$1''' strony|jest '''$1''' stron}}.

Ta liczba uwzględnia strony dyskusji, strony na temat {{GRAMMAR:D.lp|{{SITENAME}}}}, zalążki (stuby), strony przekierowujące, oraz inne, które trudno uznać za artykuły.
Wyłączając powyższe, {{PLURAL:$2|jest|są|jest}} prawdopodobnie '''$2''' {{PLURAL:$2|strona, którą można uznać za artykuł|strony, które można uznać za artykuły|stron, które można uznać za artykuły}}.

Przesłano $8 {{PLURAL:$8|plik|pliki|plików}}.

Od uruchomienia {{GRAMMAR:D.lp|{{SITENAME}}}} {{PLURAL:$3|'''1''' raz odwiedzono strony|'''$3''' razy odwiedzono strony|było '''$3''' odwiedzin stron}} i wykonano '''$4''' {{PLURAL:$4|edycję|edycje|edycji}}.
Daje to średnio '''$5''' {{PLURAL:$5|edycję|edycje|edycji}} na stronę i '''$6''' {{PLURAL:$6|odwiedzinę|odwiedziny|odwiedzin}} na edycję.

Długość [http://www.mediawiki.org/wiki/Manual:Job_queue kolejki zadań] wynosi '''$7'''.",
'userstatstext'          => "Jest {{PLURAL:$1|'''1''' zarejestrowany użytkownik|'''$1''' zarejestrowanych użytkowników}}. {{PLURAL:$1|Użytkownik ten|Spośród nich '''$2''' (czyli '''$4%''')}} ma status $5.",
'statistics-mostpopular' => 'Najczęściej odwiedzane strony',

'disambiguations'      => 'Strony ujednoznaczniające',
'disambiguationspage'  => 'Template:disambig',
'disambiguations-text' => "Poniższe strony odwołują się do '''stron ujednoznaczniających''',
a powinny odwoływać się bezpośrednio do stron treści.<br />
Strona uznawana jest za ujednoznaczniającą, jeśli zawiera ona szablon linkowany przez stronę [[MediaWiki:Disambiguationspage]]",

'doubleredirects'            => 'Podwójne przekierowania',
'doubleredirectstext'        => 'Lista zawiera strony z przekierowaniami do stron, które przekierowują do innej strony. Każdy wiersz zawiera linki do pierwszego i drugiego przekierowania oraz link, do którego prowadzi drugie przekierowanie. Ostatni link prowadzi zazwyczaj do strony, do której powinna w rzeczywistości przekierowywać pierwsza strona.',
'double-redirect-fixed-move' => 'strona [[$1]] została zastąpiona przekierowaniem, ponieważ została przeniesiona do [[$2]]',
'double-redirect-fixer'      => 'Korektor przekierowań',

'brokenredirects'        => 'Zerwane przekierowania',
'brokenredirectstext'    => 'Poniższe przekierowania wskazują na nieistniejące strony.',
'brokenredirects-edit'   => '(edytuj)',
'brokenredirects-delete' => '(usuń)',

'withoutinterwiki'         => 'Strony bez odnośników językowych',
'withoutinterwiki-summary' => 'Poniższe strony nie odwołują się do innych wersji językowych:',
'withoutinterwiki-legend'  => 'Prefiks',
'withoutinterwiki-submit'  => 'Pokaż',

'fewestrevisions' => 'Strony z najmniejszą liczbą wersji',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|bajt|bajty|bajtów}}',
'ncategories'             => '$1 {{PLURAL:$1|kategoria|kategorie|kategorii}}',
'nlinks'                  => '$1 {{PLURAL:$1|link|linki|linków}}',
'nmembers'                => '$1 {{PLURAL:$1|element|elementy|elementów}}',
'nrevisions'              => '$1 {{PLURAL:$1|wersja|wersje|wersji}}',
'nviews'                  => 'odwiedzono $1 {{PLURAL:$1|raz|razy}}',
'specialpage-empty'       => 'Ta strona raportu jest pusta.',
'lonelypages'             => 'Porzucone strony',
'lonelypagestext'         => 'Do poniższych stron nie odwołuje się żadna inna strona w {{GRAMMAR:MS.lp|{{SITENAME}}}}.',
'uncategorizedpages'      => 'Nieskategoryzowane strony',
'uncategorizedcategories' => 'Nieskategoryzowane kategorie',
'uncategorizedimages'     => 'Nieskategoryzowane pliki',
'uncategorizedtemplates'  => 'Nieskategoryzowane szablony',
'unusedcategories'        => 'Puste kategorie',
'unusedimages'            => 'Nieużywane pliki',
'popularpages'            => 'Najpopularniejsze strony',
'wantedcategories'        => 'Brakujące kategorie',
'wantedpages'             => 'Najpotrzebniejsze strony',
'missingfiles'            => 'Brak plików',
'mostlinked'              => 'Najczęściej linkowane strony',
'mostlinkedcategories'    => 'Kategorie o największej liczbie stron',
'mostlinkedtemplates'     => 'Najczęściej linkowane szablony',
'mostcategories'          => 'Strony z największą liczbą kategorii',
'mostimages'              => 'Najczęściej linkowane pliki',
'mostrevisions'           => 'Strony o największej liczbie wersji',
'prefixindex'             => 'Wszystkie strony według prefiksu',
'shortpages'              => 'Najkrótsze strony',
'longpages'               => 'Najdłuższe strony',
'deadendpages'            => 'Strony bez linków wewnętrznych',
'deadendpagestext'        => 'Poniższe strony nie posiadają odnośników do innych stron znajdujących się w {{GRAMMAR:MS.lp|{{SITENAME}}}}.',
'protectedpages'          => 'Strony zabezpieczone',
'protectedpages-indef'    => 'Tylko strony zabezpieczone na zawsze',
'protectedpagestext'      => 'Poniższe strony zostały zabezpieczone przed przenoszeniem lub edytowaniem.',
'protectedpagesempty'     => 'Żadna strona nie jest obecnie zabezpieczona z podanymi parametrami.',
'protectedtitles'         => 'Zabezpieczone nazwy stron',
'protectedtitlestext'     => 'Utworzenie stron o następujących nazwach jest zablokowane',
'protectedtitlesempty'    => 'Dla tych ustawień dopuszczalne jest utworzenie stron o dowolnej nazwie.',
'listusers'               => 'Lista użytkowników',
'newpages'                => 'Nowe strony',
'newpages-username'       => 'Nazwa użytkownika',
'ancientpages'            => 'Najstarsze strony',
'move'                    => 'przenieś',
'movethispage'            => 'Przenieś tę stronę',
'unusedimagestext'        => 'Inne witryny mogą odwoływać się do tych plików, używając bezpośrednich adresów URL. Oznacza to, że niektóre z plików mogą się znajdować na tej liście pomimo tego, że są wykorzystywane.',
'unusedcategoriestext'    => 'Poniższe kategorie istnieją, choć nie korzysta z nich żadna strona ani kategoria.',
'notargettitle'           => 'Wskazywana strona nie istnieje',
'notargettext'            => 'Nie podano strony albo użytkownika, dla których ta operacja ma być wykonana.',
'nopagetitle'             => 'Strona docelowa nie istnieje',
'nopagetext'              => 'Wybrana strona docelowa nie istnieje.',
'pager-newer-n'           => '{{PLURAL:$1|1 nowszy|$1 nowsze|$1 nowszych}}',
'pager-older-n'           => '{{PLURAL:$1|1 starszy|$1 starsze|$1 starszych}}',
'suppress'                => 'Rewizor',

# Book sources
'booksources'               => 'Książki',
'booksources-search-legend' => 'Szukaj informacji o książkach',
'booksources-go'            => 'Pokaż',
'booksources-text'          => 'Poniżej znajduje się lista odnośników do innych witryn, które pośredniczą w sprzedaży nowych i używanych książek, a także mogą posiadać dalsze informacje na temat poszukiwanej przez ciebie książki.',

# Special:Log
'specialloguserlabel'  => 'Użytkownik',
'speciallogtitlelabel' => 'Tytuł',
'log'                  => 'Rejestr operacji',
'all-logs-page'        => 'Wszystkie operacje',
'log-search-legend'    => 'Szukaj w rejestrze',
'log-search-submit'    => 'Szukaj',
'alllogstext'          => 'Wspólny rejestr wszystkich typów operacji dla {{GRAMMAR:D.lp|{{SITENAME}}}}.
Możesz zawęzić liczbę wyników poprzez wybranie typu rejestru, nazwy użytkownika albo tytułu strony.',
'logempty'             => 'Brak wpisów w rejestrze.',
'log-title-wildcard'   => 'Szukaj tytułów zaczynających się od tego tekstu',

# Special:AllPages
'allpages'          => 'Wszystkie strony',
'alphaindexline'    => 'od $1 do $2',
'nextpage'          => 'Następna strona ($1)',
'prevpage'          => 'Poprzednia strona ($1)',
'allpagesfrom'      => 'Strony o tytułach rozpoczynających się od:',
'allarticles'       => 'Wszystkie artykuły',
'allinnamespace'    => 'Wszystkie strony (w przestrzeni nazw $1)',
'allnotinnamespace' => 'Wszystkie strony (oprócz przestrzeni nazw $1)',
'allpagesprev'      => 'Poprzednia',
'allpagesnext'      => 'Następna',
'allpagessubmit'    => 'Pokaż',
'allpagesprefix'    => 'Pokaż strony o tytułach rozpoczynających się od',
'allpagesbadtitle'  => 'Podana nazwa jest nieprawidłowa, zawiera prefiks międzyprojektowy lub międzyjęzykowy. Może ona także zawierać w sobie jeden lub więcej znaków, których użycie w nazwach jest niedozwolone.',
'allpages-bad-ns'   => 'W {{GRAMMAR:MS.lp|{{SITENAME}}}} nie istnieje przestrzeń nazw „$1”.',

# Special:Categories
'categories'                    => 'Kategorie',
'categoriespagetext'            => 'Strona przedstawia listę kategorii zawierających strony i pliki.
[[Special:UnusedCategories|Nieużywane kategorie]] nie zostały tutaj pokazane.
Zobacz też [[Special:WantedCategories|nieistniejące kategorie]].',
'categoriesfrom'                => 'Wyświetl kategorie, zaczynając od:',
'special-categories-sort-count' => 'sortowanie według liczby',
'special-categories-sort-abc'   => 'sortowanie alfabetyczne',

# Special:ListUsers
'listusersfrom'      => 'Pokaż użytkowników zaczynając od',
'listusers-submit'   => 'Pokaż',
'listusers-noresult' => 'Nie znaleziono żadnego użytkownika.',

# Special:ListGroupRights
'listgrouprights'          => 'Uprawnienia grup użytkowników',
'listgrouprights-summary'  => 'Poniżej znajduje się spis zdefiniowanych na tej wiki grup użytkowników, z wyszczególnieniem przydzielonych im uprawnień.
Sprawdź stronę z [[{{MediaWiki:Listgrouprights-helppage}}|dodatkowymi informacjami]] o uprawnieniach.',
'listgrouprights-group'    => 'Grupa',
'listgrouprights-rights'   => 'Uprawnienia',
'listgrouprights-helppage' => 'Help:Uprawnienia grup użytkowników',
'listgrouprights-members'  => '(lista członków grupy)',

# E-mail user
'mailnologin'     => 'Brak adresu',
'mailnologintext' => 'Musisz się [[Special:UserLogin|zalogować]] i mieć wpisany aktualny adres e-mailowy w swoich [[Special:Preferences|preferencjach]], aby móc wysłać e-mail do innego użytkownika.',
'emailuser'       => 'Wyślij e-mail do tego użytkownika',
'emailpage'       => 'Wyślij e-mail do użytkownika',
'emailpagetext'   => 'Poniższy formularz pozwala na wysłanie jednej wiadomości do użytkownika pod warunkiem, że wpisał on poprawny adres e-mail w swoich preferencjach.
Adres e-mailowy, który został przez Ciebie wprowadzony w [[Special:Preferences|Twoich preferencjach]], pojawi się w polu „Od”, dzięki czemu odbiorca będzie mógł Ci odpowiedzieć.',
'usermailererror' => 'Moduł obsługi poczty zwrócił błąd:',
'defemailsubject' => 'Wiadomość z {{GRAMMAR:D.lp|{{SITENAME}}}}',
'noemailtitle'    => 'Brak adresu e-mail',
'noemailtext'     => 'Ten użytkownik nie podał poprawnego adresu e-mail albo zadecydował, że nie chce otrzymywać wiadomości e-mail od innych użytkowników.',
'emailfrom'       => 'Od:',
'emailto'         => 'Do:',
'emailsubject'    => 'Temat:',
'emailmessage'    => 'Wiadomość:',
'emailsend'       => 'Wyślij',
'emailccme'       => 'Wyślij mi kopię mojej wiadomości.',
'emailccsubject'  => 'Kopia Twojej wiadomości do $1: $2',
'emailsent'       => 'Wiadomość została wysłana',
'emailsenttext'   => 'Twoja wiadomość została wysłana.',
'emailuserfooter' => 'Wiadomość e-mail została wysłana z {{GRAMMAR:D.lp|{{SITENAME}}}} do $2 przez $1 z użyciem „Wyślij e-mail do tego użytkownika”.',

# Watchlist
'watchlist'            => 'Obserwowane',
'mywatchlist'          => 'obserwowane',
'watchlistfor'         => "(raport dla użytkownika '''$1''')",
'nowatchlist'          => 'Lista obserwowanych przez Ciebie stron jest pusta.',
'watchlistanontext'    => '$1, aby obejrzeć lub edytować elementy listy obserwowanych.',
'watchnologin'         => 'Nie jesteś zalogowany',
'watchnologintext'     => 'Musisz się [[Special:UserLogin|zalogować]] przed modyfikacją listy obserwowanych stron.',
'addedwatch'           => 'Dodana do listy obserwowanych',
'addedwatchtext'       => "Strona „[[:$1|$1]]” została dodana do Twojej [[Special:Watchlist|listy obserwowanych]].
Każda zmiana treści tej strony lub związanej z nią strony dyskusji zostanie odnotowana na poniższej liście. Dodatkowo nazwa strony zostanie '''wytłuszczona''' na [[Special:RecentChanges|liście ostatnich zmian]], aby ułatwić Ci zauważenie faktu zmiany.",
'removedwatch'         => 'Usunięto z listy obserwowanych',
'removedwatchtext'     => 'Strona „[[:$1]]” została usunięta z Twojej [[Special:Watchlist|listy obserwowanych]].',
'watch'                => 'Obserwuj',
'watchthispage'        => 'Obserwuj',
'unwatch'              => 'nie obserwuj',
'unwatchthispage'      => 'Przestań obserwować',
'notanarticle'         => 'To nie jest artykuł',
'notvisiblerev'        => 'Wersja została usunięta',
'watchnochange'        => 'Żadna z obserwowanych stron nie była edytowana w podanym okresie.',
'watchlist-details'    => 'Na liście obserwowanych {{PLURAL:$1|jest 1 strona|są $1 strony|jest $1 stron}}, nie licząc stron dyskusji.',
'wlheader-enotif'      => '* Wysyłanie powiadomień na adres e-mail jest włączone.',
'wlheader-showupdated' => "* Strony, które zostały zmodyfikowane od Twojej ostatniej wizyty na nich zostały '''wytłuszczone'''.",
'watchmethod-recent'   => 'poszukiwanie ostatnich zmian wśród obserwowanych stron',
'watchmethod-list'     => 'poszukiwanie obserwowanych stron wśród ostatnich zmian',
'watchlistcontains'    => 'Na liście obserwowanych przez Ciebie stron {{PLURAL:$1|znajduje się 1 pozycja|znajdują się $1 pozycje|znajduje się $1 pozycji}}.',
'iteminvalidname'      => 'Problem z pozycją „$1”, niepoprawna nazwa...',
'wlnote'               => "Poniżej pokazano {{PLURAL:$1|ostatnią zmianę wykonaną|ostatnie '''$1''' zmiany wykonane|ostatnich '''$1''' zmian wykonanych}} w ciągu {{PLURAL:$2|ostatniej godziny|ostatnich '''$2''' godzin}}.",
'wlshowlast'           => 'Pokaż ostatnie $1 godzin, $2 dni ($3)',
'watchlist-show-bots'  => 'Pokaż edycje botów',
'watchlist-hide-bots'  => 'Ukryj edycje botów',
'watchlist-show-own'   => 'Pokaż moje edycje',
'watchlist-hide-own'   => 'Ukryj moje edycje',
'watchlist-show-minor' => 'Pokaż drobne edycje',
'watchlist-hide-minor' => 'Ukryj drobne edycje',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Obserwuję...',
'unwatching' => 'Przestaję obserwować...',

'enotif_mailer'                => 'Powiadomienie z {{GRAMMAR:D.lp|{{SITENAME}}}}',
'enotif_reset'                 => 'Zaznacz wszystkie strony jako odwiedzone',
'enotif_newpagetext'           => 'To jest nowa strona.',
'enotif_impersonal_salutation' => 'użytkownik {{GRAMMAR:D.lp|{{SITENAME}}}}',
'changed'                      => 'zmieniona',
'created'                      => 'utworzona',
'enotif_subject'               => 'Strona $PAGETITLE w {{GRAMMAR:MS.lp|{{SITENAME}}}} została $CHANGEDORCREATED przez użytkownika $PAGEEDITOR',
'enotif_lastvisited'           => 'Zobacz na stronie $1 wszystkie zmiany od Twojej ostatniej wizyty.',
'enotif_lastdiff'              => 'Zobacz na stronie $1 tę zmianę.',
'enotif_anon_editor'           => 'użytkownik anonimowy $1',
'enotif_body'                  => 'Drogi (droga) $WATCHINGUSERNAME,

strona $PAGETITLE w {{GRAMMAR:MS.lp|{{SITENAME}}}} została $CHANGEDORCREATED $PAGEEDITDATE przez użytkownika $PAGEEDITOR. Zobacz na stronie $PAGETITLE_URL aktualną wersję.

$NEWPAGE

Opis zmiany: $PAGESUMMARY $PAGEMINOREDIT

Skontaktuj się z autorem:
mail: $PAGEEDITOR_EMAIL
wiki: $PAGEEDITOR_WIKI

W przypadku kolejnych zmian nowe powiadomienia nie zostaną wysłane, dopóki nie odwiedzisz tej strony.
Możesz także zresetować wszystkie flagi powiadomień na swojej liście stron obserwowanych.

	Wiadomość systemu powiadomień {{GRAMMAR:D.lp|{{SITENAME}}}}

--
W celu zmiany ustawień swojej listy obserwowanych odwiedź
{{fullurl:{{ns:special}}:Watchlist/edit}}

Pomoc:
{{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => 'Usuń stronę',
'confirm'                     => 'Potwierdź',
'excontent'                   => 'treść: „$1”',
'excontentauthor'             => 'treść: „$1” (jedyny autor: [[Special:Contributions/$2|$2]])',
'exbeforeblank'               => 'poprzednia zawartość, obecnie pustej strony: „$1”',
'exblank'                     => 'Strona była pusta',
'delete-confirm'              => 'Usuń „$1”',
'delete-legend'               => 'Usuń',
'historywarning'              => 'Uwaga! Strona, którą chcesz usunąć, ma starsze wersje:',
'confirmdeletetext'           => 'Zamierzasz usunąć stronę razem z całą dotyczącą jej historią.
Upewnij się, czy na pewno chcesz to zrobić, że rozumiesz konsekwencje i że robisz to w zgodzie z [[{{MediaWiki:Policy-url}}|zasadami]].',
'actioncomplete'              => 'Operacja wykonana',
'deletedtext'                 => 'Usunięto „<nowiki>$1</nowiki>”.
Zobacz na stronie $2 rejestr ostatnio wykonanych usunięć.',
'deletedarticle'              => 'usunął [[$1]]',
'suppressedarticle'           => 'utajnił [[$1]]',
'dellogpage'                  => 'Usunięte',
'dellogpagetext'              => 'Poniżej znajduje się lista ostatnio wykonanych usunięć.',
'deletionlog'                 => 'rejestr usunięć',
'reverted'                    => 'Przywrócono poprzednią wersję',
'deletecomment'               => 'Powód usunięcia:',
'deleteotherreason'           => 'Inny/dodatkowy powód:',
'deletereasonotherlist'       => 'Inny powód',
'deletereason-dropdown'       => '* Najczęstsze powody usunięcia
** Prośba autora
** Naruszenie praw autorskich
** Wandalizm',
'delete-edit-reasonlist'      => 'Edycja listy powodów usunięcia strony',
'delete-toobig'               => 'Ta strona ma bardzo długą historię edycji, ponad $1 {{PLURAL:$1|zmianę|zmiany|zmian}}.
Usunięcie jej mogłoby spowodować zakłócenia w pracy {{GRAMMAR:D.lp|{{SITENAME}}}} i dlatego zostało ograniczone.',
'delete-warning-toobig'       => 'Ta strona ma bardzo długą historię edycji, ponad $1 {{PLURAL:$1|zmianę|zmiany|zmian}}.
Bądź ostrożny, ponieważ usunięcie jej może spowodować zakłócenia w pracy {{GRAMMAR:D.lp|{{SITENAME}}}}.',
'rollback'                    => 'Cofnij edycję',
'rollback_short'              => 'Cofnij',
'rollbacklink'                => 'cofnij',
'rollbackfailed'              => 'Nie udało się cofnąć zmiany',
'cantrollback'                => 'Nie można cofnąć edycji, ponieważ jest tylko jedna wersja tej strony.',
'alreadyrolled'               => 'Nie można dla strony [[:$1|$1]] cofnąć ostatniej zmiany, którą wykonał [[User:$2|$2]] ([[User talk:$2|dyskusja]] | [[Special:Contributions/$2|{{int:contribslink}}]]).
Ktoś inny zdążył już to zrobić lub wprowadził własne poprawki do treści strony.

Autorem ostatniej zmiany jest teraz [[User:$3|$3]] ([[User talk:$3|dyskusja]] | [[Special:Contributions/$3|{{int:contribslink}}]]).',
'editcomment'                 => 'Edycję opisano: „<i>$1</i>”.', # only shown if there is an edit comment
'revertpage'                  => 'Wycofano edycje użytkownika [[Special:Contributions/$2|$2]] ([[User talk:$2|dyskusja]]). Autor przywróconej wersji to [[User:$1|$1]].', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => 'Wycofano edycje użytkownika $1.
Przywrócono ostatnią wersję autorstwa $2.',
'sessionfailure'              => 'Wystąpił problem z weryfikacją zalogowania.
Polecenie zostało anulowane, aby uniknąć przechwycenia sesji.
Naciśnij „wstecz” w przeglądarce, przeładuj stronę, po czym ponownie wydaj polecenie.',
'protectlogpage'              => 'Zabezpieczone',
'protectlogtext'              => 'Poniżej znajduje się lista blokad założonych i zdjętych z pojedynczych stron.
Aby przejrzeć listę obecnie działających zabezpieczeń, przejdź na stronę wykazu [[Special:ProtectedPages|zabezpieczonych stron]].',
'protectedarticle'            => 'zabezpieczył [[$1]]',
'modifiedarticleprotection'   => 'zmienił poziom zabezpieczenia [[$1]]',
'unprotectedarticle'          => 'odbezpieczył [[$1]]',
'protect-title'               => 'Zmiana poziomu zabezpieczenia „$1”',
'protect-legend'              => 'Potwierdź zabezpieczenie',
'protectcomment'              => 'powód zabezpieczenia',
'protectexpiry'               => 'Czas wygaśnięcia:',
'protect_expiry_invalid'      => 'Podany czas automatycznego odbezpieczenia jest nieprawidłowy.',
'protect_expiry_old'          => 'Podany czas automatycznego odblokowania znajduje się w przeszłości.',
'protect-unchain'             => 'Odblokowanie możliwości przenoszenia strony',
'protect-text'                => 'Możesz tu sprawdzić i zmienić poziom zabezpieczenia strony <strong><nowiki>$1</nowiki></strong>.',
'protect-locked-blocked'      => 'Nie możesz zmienić poziomów zabezpieczenia, ponieważ jesteś zablokowany.
Obecne ustawienia dla strony <strong>$1</strong> to:',
'protect-locked-dblock'       => 'Nie można zmienić poziomu zabezpieczenia z powodu działającej blokady bazy danych. Obecne ustawienia dla strony <strong>$1</strong> to:',
'protect-locked-access'       => 'Nie masz uprawnień do zmiany poziomu zabezpieczenia strony. Obecne ustawienia dla strony <strong>$1</strong> to:',
'protect-cascadeon'           => 'Ta strona jest zabezpieczona przed edycją, ponieważ jest używana przez {{PLURAL:$1|następującą stronę, która została zabezpieczona|następujące strony, które zostały zabezpieczone}} z włączoną opcją dziedziczenia. Możesz zmienić poziom zabezpieczenia strony, ale nie wpłynie to na dziedziczenie zabezpieczenia.',
'protect-default'             => '(wszyscy)',
'protect-fallback'            => 'Wymaga uprawnień „$1”',
'protect-level-autoconfirmed' => 'tylko zarejestrowani',
'protect-level-sysop'         => 'tylko administratorzy',
'protect-summary-cascade'     => 'dziedziczenie',
'protect-expiring'            => 'wygasa $1 (UTC)',
'protect-cascade'             => 'Dziedziczenie zabezpieczenia – zabezpiecz wszystkie strony zawarte na tej stronie.',
'protect-cantedit'            => 'Nie możesz zmienić poziomu zabezpieczenia tej strony, ponieważ nie masz uprawnień do jej edycji.',
'restriction-type'            => 'Ograniczenia',
'restriction-level'           => 'Poziom',
'minimum-size'                => 'Minimalny rozmiar',
'maximum-size'                => 'Maksymalny rozmiar',
'pagesize'                    => '(bajtów)',

# Restrictions (nouns)
'restriction-edit'   => 'Edytowanie',
'restriction-move'   => 'Przenoszenie',
'restriction-create' => 'Utwórz',
'restriction-upload' => 'Prześlij',

# Restriction levels
'restriction-level-sysop'         => 'całkowite zabezpieczenie',
'restriction-level-autoconfirmed' => 'częściowe zabezpieczenie',
'restriction-level-all'           => 'dowolny poziom',

# Undelete
'undelete'                     => 'Odtwórz usuniętą stronę',
'undeletepage'                 => 'Odtwarzanie usuniętych stron',
'undeletepagetitle'            => "'''Poniżej znajdują się usunięte wersje strony [[:$1]]'''.",
'viewdeletedpage'              => 'Zobacz usunięte wersje',
'undeletepagetext'             => 'Poniższe strony zostały usunięte, ale ich kopia wciąż znajduje się w archiwum.
Archiwum co jakiś czas może być oczyszczane.',
'undelete-fieldset-title'      => 'Odtwarzanie wersji',
'undeleteextrahelp'            => "Jeśli chcesz odtworzyć całą stronę, pozostaw wszystkie pola niezaznaczone i kliknij '''''Odtwórz'''''.
Częściowe odtworzenie możesz wykonać, zaznaczając odpowiednie pola, odpowiadające wersjom, które będą odtworzone, a następnie klikając '''''Odtwórz'''''.
Naciśnięcie '''''Wyczyść''''' usunie wszystkie zaznaczenia i wyczyści pole komentarza.",
'undeleterevisions'            => '$1 {{PLURAL:$1|zarchiwizowana wersja|zarchiwizowane wersje|zarchiwizowanych wersji}}',
'undeletehistory'              => 'Odtworzenie strony spowoduje przywrócenie także jej wszystkich poprzednich wersji.
Jeśli od czasu usunięcia ktoś utworzył nową stronę o tej samej nazwie, odtwarzane wersje znajdą się w jej historii, a obecna wersja pozostanie bez zmian.',
'undeleterevdel'               => 'Odtworzenie nie zostanie przeprowadzone, jeśli mogłoby spowodować częściowe usunięcie aktualnej wersji strony lub pliku.
W takiej sytuacji należy odznaczyć lub przywrócić widoczność najnowszej usuniętej wersji.',
'undeletehistorynoadmin'       => 'Ta strona została usunięta.
Przyczyna usunięcia podana jest w podsumowaniu poniżej, razem z danymi użytkownika, który edytował stronę przed usunięciem.
Sama treść usuniętych wersji jest dostępna jedynie dla administratorów.',
'undelete-revision'            => 'Usunięto wersję $1 z $2 autorstwa $3:',
'undeleterevision-missing'     => 'Nieprawidłowa lub brakująca wersja.
Możesz mieć zły link lub wersja mogła zostać odtworzona lub usunięta z archiwum.',
'undelete-nodiff'              => 'Nie znaleziono poprzednich wersji.',
'undeletebtn'                  => 'Odtwórz',
'undeletelink'                 => 'odtwórz',
'undeletereset'                => 'Wyczyść',
'undeletecomment'              => 'Powód odtworzenia:',
'undeletedarticle'             => 'odtworzył [[$1]]',
'undeletedrevisions'           => 'odtworzono {{PLURAL:$1|1 wersję|$1 wersje|$1 wersji}}',
'undeletedrevisions-files'     => 'odtworzono $1 {{PLURAL:$1|wersję|wersje|wersji}} i $2 {{PLURAL:$2|plik|pliki|plików}}',
'undeletedfiles'               => 'odtworzył $1 {{PLURAL:$1|plik|pliki|plików}}',
'cannotundelete'               => 'Odtworzenie nie powiodło się.
Ktoś inny prawdopodobnie odtworzył już tę stronę.',
'undeletedpage'                => "<big>'''Odtworzono stronę $1.'''</big>

Zobacz [[Special:Log/delete|rejestr usunięć]], jeśli chcesz przejrzeć ostatnie operacje usuwania i odtwarzania stron.",
'undelete-header'              => 'Zobacz [[Special:Log/delete|rejestr usunięć]], aby sprawdzić ostatnio usunięte strony.',
'undelete-search-box'          => 'Szukaj usuniętych stron',
'undelete-search-prefix'       => 'Strony o tytułach rozpoczynających się od',
'undelete-search-submit'       => 'Szukaj',
'undelete-no-results'          => 'Nie znaleziono wskazanych stron w archiwum usuniętych.',
'undelete-filename-mismatch'   => 'Nie można odtworzyć wersji pliku z datą $1: niezgodność nazwy pliku',
'undelete-bad-store-key'       => 'Nie można odtworzyć wersji pliku z datą $1: przed usunięciem brakowało pliku.',
'undelete-cleanup-error'       => 'Wystąpił błąd przy usuwaniu nieużywanego archiwalnego pliku „$1”.',
'undelete-missing-filearchive' => 'Nie udało się odtworzyć z archiwum pliku o ID $1, ponieważ brak go w bazie danych.
Być może plik został już odtworzony.',
'undelete-error-short'         => 'Wystąpił błąd przy odtwarzaniu pliku: $1',
'undelete-error-long'          => 'Napotkano błędy przy odtwarzaniu pliku:

$1',

# Namespace form on various pages
'namespace'      => 'Przestrzeń nazw',
'invert'         => 'odwróć wybór',
'blanknamespace' => '(główna)',

# Contributions
'contributions' => 'Wkład użytkownika',
'mycontris'     => 'moje edycje',
'contribsub2'   => 'Dla użytkownika $1 ($2)',
'nocontribs'    => 'Brak zmian odpowiadających tym kryteriom.',
'uctop'         => ' (jako ostatnia)',
'month'         => 'Przed miesiącem (włącznie)',
'year'          => 'Przed rokiem (włącznie)',

'sp-contributions-newbies'     => 'Pokaż wyłącznie wkład nowych użytkowników',
'sp-contributions-newbies-sub' => 'Dla nowych użytkowników',
'sp-contributions-blocklog'    => 'blokady',
'sp-contributions-search'      => 'Szukaj wkładu',
'sp-contributions-username'    => 'Adres IP lub nazwa użytkownika',
'sp-contributions-submit'      => 'Szukaj',

# What links here
'whatlinkshere'            => 'Linkujące',
'whatlinkshere-title'      => 'Strony linkujące do „$1”',
'whatlinkshere-page'       => 'Strona',
'linklistsub'              => '(Lista linków)',
'linkshere'                => "Następujące strony odwołują się do '''[[:$1]]''':",
'nolinkshere'              => "Żadna strona nie odwołuje się do '''[[:$1]]'''.",
'nolinkshere-ns'           => "Żadna strona nie odwołuje się do '''[[:$1]]''' w wybranej przestrzeni nazw.",
'isredirect'               => 'strona przekierowująca',
'istemplate'               => 'dołączony szablon',
'isimage'                  => 'odnośnik z grafiki',
'whatlinkshere-prev'       => '{{PLURAL:$1|poprzednie|poprzednie $1}}',
'whatlinkshere-next'       => '{{PLURAL:$1|następne|następne $1}}',
'whatlinkshere-links'      => '← linkujące',
'whatlinkshere-hideredirs' => '$1 przekierowania',
'whatlinkshere-hidetrans'  => '$1 dołączenia',
'whatlinkshere-hidelinks'  => '$1 linki',
'whatlinkshere-hideimages' => '$1 linki z grafik',
'whatlinkshere-filters'    => 'Filtry',

# Block/unblock
'blockip'                         => 'Zablokuj użytkownika',
'blockip-legend'                  => 'Zablokuj użytkownika',
'blockiptext'                     => 'Użyj poniższego formularza do zablokowania możliwości edycji spod określonego adresu IP lub konkretnemu użytkownikowi.
Blokować należy jedynie po to, by zapobiec wandalizmom, zgodnie z [[{{MediaWiki:Policy-url}}|przyjętymi zasadami]].
Podaj powód (np. umieszczając nazwy stron, na których dopuszczono się wandalizmu).',
'ipaddress'                       => 'Adres IP',
'ipadressorusername'              => 'Adres IP lub nazwa użytkownika',
'ipbexpiry'                       => 'Czas blokady',
'ipbreason'                       => 'Powód',
'ipbreasonotherlist'              => 'Inny powód',
'ipbreason-dropdown'              => '*Najczęstsze przyczyny blokad
** Ataki na innych użytkowników
** Naruszenie praw autorskich
** Niedozwolona nazwa użytkownika
** Open proxy/Tor
** Spamowanie
** Usuwanie treści stron
** Wprowadzanie fałszywych informacji
** Wulgaryzmy
** Wypisywanie bzdur na stronach',
'ipbanononly'                     => 'Zablokuj tylko anonimowych użytkowników',
'ipbcreateaccount'                => 'Zapobiegnij utworzeniu konta',
'ipbemailban'                     => 'Zablokuj możliwość wysyłania e-maili',
'ipbenableautoblock'              => 'Zablokuj ostatni adres IP tego użytkownika i automatycznie wszystkie kolejne, z których będzie próbował edytować',
'ipbsubmit'                       => 'Zablokuj użytkownika',
'ipbother'                        => 'Inny okres:',
'ipboptions'                      => '2 godziny:2 hours,1 dzień:1 day,3 dni:3 days,1 tydzień:1 week,2 tygodnie:2 weeks,1 miesiąc:1 month,3 miesiące:3 months,6 miesięcy:6 months,1 rok:1 year,nieskończony:infinite', # display1:time1,display2:time2,...
'ipbotheroption'                  => 'inny',
'ipbotherreason'                  => 'Inne/dodatkowe uzasadnienie:',
'ipbhidename'                     => 'Ukryj nazwę użytkownika/adres IP w rejestrze blokad, na liście aktywnych blokad i liście użytkowników',
'ipbwatchuser'                    => 'Obserwuj stronę osobistą i stronę dyskusji tego użytkownika',
'badipaddress'                    => 'Niepoprawny adres IP',
'blockipsuccesssub'               => 'Zablokowanie powiodło się',
'blockipsuccesstext'              => 'Użytkownik [[Special:Contributions/$1|$1]] został zablokowany.<br />
Przejdź do [[Special:IPBlockList|listy zablokowanych adresów IP]], by przejrzeć blokady.',
'ipb-edit-dropdown'               => 'Edytuj przyczynę blokady',
'ipb-unblock-addr'                => 'Odblokuj $1',
'ipb-unblock'                     => 'Odblokuj użytkownika lub adres IP',
'ipb-blocklist-addr'              => 'Zobacz istniejące blokady $1',
'ipb-blocklist'                   => 'Zobacz istniejące blokady',
'unblockip'                       => 'Odblokuj użytkownika',
'unblockiptext'                   => 'Użyj poniższego formularza, by przywrócić możliwość edycji z wcześniej zablokowanego adresu IP lub użytkownikowi.',
'ipusubmit'                       => 'Odblokuj użytkownika',
'unblocked'                       => '[[User:$1|$1]] został odblokowany.',
'unblocked-id'                    => 'Blokada $1 została zdjęta',
'ipblocklist'                     => 'Lista zablokowanych adresów IP i użytkowników',
'ipblocklist-legend'              => 'Znajdź zablokowanego użytkownika',
'ipblocklist-username'            => 'Nazwa użytkownika lub adres IP',
'ipblocklist-submit'              => 'Szukaj',
'blocklistline'                   => '$1, $2 zablokował $3 ($4)',
'infiniteblock'                   => 'na zawsze',
'expiringblock'                   => 'wygasa $1',
'anononlyblock'                   => 'tylko niezalogowani',
'noautoblockblock'                => 'automatyczne blokowanie wyłączone',
'createaccountblock'              => 'blokada tworzenia kont',
'emailblock'                      => 'zablokowany e-mail',
'ipblocklist-empty'               => 'Lista blokad jest pusta.',
'ipblocklist-no-results'          => 'Podany adres IP lub użytkownik nie jest zablokowany.',
'blocklink'                       => 'zablokuj',
'unblocklink'                     => 'odblokuj',
'contribslink'                    => 'wkład',
'autoblocker'                     => 'Zablokowano Cię automatycznie, ponieważ używasz tego samego adresu IP, co użytkownik „[[User:$1|$1]]”.
Przyczyna blokady $1 to: „$2”',
'blocklogpage'                    => 'Historia blokad',
'blocklogentry'                   => 'zablokował [[$1]], czas blokady: $2 $3',
'blocklogtext'                    => 'Poniżej znajduje się lista blokad założonych i zdjętych z poszczególnych adresów IP.
Na liście nie znajdą się adresy IP, które zablokowano w sposób automatyczny.
By przejrzeć listę obecnie aktywnych blokad, przejdź na stronę [[Special:IPBlockList|zablokowanych adresów i użytkowników]].',
'unblocklogentry'                 => 'odblokował $1',
'block-log-flags-anononly'        => 'tylko anonimowi',
'block-log-flags-nocreate'        => 'blokada tworzenia konta',
'block-log-flags-noautoblock'     => 'automatyczne blokowanie wyłączone',
'block-log-flags-noemail'         => 'e-mail zablokowany',
'block-log-flags-angry-autoblock' => 'rozszerzone automatyczne blokowanie włączone',
'range_block_disabled'            => 'Możliwość blokowania zakresu adresów IP została wyłączona.',
'ipb_expiry_invalid'              => 'Błędny czas wygaśnięcia blokady.',
'ipb_expiry_temp'                 => 'Ukrytą nazwę użytkownika należy zablokować trwale.',
'ipb_already_blocked'             => '„$1” jest już zablokowany',
'ipb_cant_unblock'                => 'Błąd: Blokada o ID $1 nie została znaleziona. Mogła ona zostać zdjęta wcześniej.',
'ipb_blocked_as_range'            => 'Błąd: Adres IP $1 nie został zablokowany bezpośrednio i nie może zostać odblokowany.
Należy on do zablokowanego zakresu adresów $2. Odblokować można tylko cały zakres.',
'ip_range_invalid'                => 'Niepoprawny zakres adresów IP.',
'blockme'                         => 'Zablokuj mnie',
'proxyblocker'                    => 'Blokowanie proxy',
'proxyblocker-disabled'           => 'Ta funkcja jest wyłączona.',
'proxyblockreason'                => 'Twój adres IP został zablokowany, ponieważ jest to adres otwartego proxy.
O tym poważnym problemie dotyczącym bezpieczeństwa należy poinformować dostawcę Internetu lub pomoc techniczną.',
'proxyblocksuccess'               => 'Wykonano.',
'sorbsreason'                     => 'Twój adres IP znajduje się na liście serwerów open proxy w DNSBL, używanej przez {{GRAMMAR:B.lp|{{SITENAME}}}}.',
'sorbs_create_account_reason'     => 'Twój adres IP znajduje się na liście serwerów open proxy w DNSBL, używanej przez {{GRAMMAR:B.lp|{{SITENAME}}}}.
Nie możesz utworzyć konta',

# Developer tools
'lockdb'              => 'Zablokuj bazę danych',
'unlockdb'            => 'Odblokuj bazę danych',
'lockdbtext'          => 'Zablokowanie bazy danych uniemożliwi wszystkim użytkownikom edycję stron, zmianę preferencji, edycję list obserwowanych stron oraz inne czynności wymagające dostępu do bazy danych. 
Potwierdź, że to jest zgodne z Twoimi zamiarami, i że odblokujesz bazę danych, gdy tylko zakończysz zadania administracyjne.',
'unlockdbtext'        => 'Odblokowanie bazy danych umożliwi wszystkim użytkownikom edycję stron, zmianę preferencji, edycję list obserwowanych stron oraz inne czynności związane ze zmianami w bazie danych. Potwierdź, że to jest zgodne z Twoimi zamiarami.',
'lockconfirm'         => 'Tak, naprawdę chcę zablokować bazę danych.',
'unlockconfirm'       => 'Tak, naprawdę chcę odblokować bazę danych.',
'lockbtn'             => 'Zablokuj bazę danych',
'unlockbtn'           => 'Odblokuj bazę danych',
'locknoconfirm'       => 'Nie zaznaczyłeś potwierdzenia.',
'lockdbsuccesssub'    => 'Baza danych została pomyślnie zablokowana',
'unlockdbsuccesssub'  => 'Blokada bazy danych została zdjęta',
'lockdbsuccesstext'   => 'Baza danych została zablokowana.<br />
Pamiętaj by [[Special:UnlockDB|zdjąć blokadę]] po zakończeniu działań administracyjnych.',
'unlockdbsuccesstext' => 'Baza danych została odblokowana.',
'lockfilenotwritable' => 'Nie można zapisać pliku blokady bazy danych.
Blokowanie i odblokowywanie bazy danych, wymaga by plik mógł być zapisywany przez web serwer.',
'databasenotlocked'   => 'Baza danych nie jest zablokowana.',

# Move page
'move-page'               => 'Przenieś $1',
'move-page-legend'        => 'Przeniesienie strony',
'movepagetext'            => "Za pomocą poniższego formularza zmienisz nazwę strony, przenosząc jednocześnie jej historię.
Pod starym tytułem zostanie umieszczona strona przekierowująca.
Możesz automatycznie zaktualizować przekierowania wskazujące na tytuł przed zmianą.
Jeśli nie wybierzesz tej opcji, upewnij się po przeniesieniu strony, czy nie powstały [[Special:DoubleRedirects|podwójne]] lub [[Special:BrokenRedirects|zerwane przekierowania]].
Jesteś odpowiedzialny za to, by linki w dalszym ciągu pokazywały tam, gdzie powinny.

Strona '''nie''' zostanie przeniesiona, jeśli strona o nowej nazwie już istnieje, chyba że jest pusta lub jest przekierowaniem i ma pustą historię edycji.
To oznacza, że błędną operację zmiany nazwy można bezpiecznie odwrócić, zmieniając nową nazwę strony na poprzednią, i że nie można nadpisać istniejącej strony.

'''UWAGA!'''
Może to być drastyczna lub nieprzewidywalna zmiana w przypadku popularnych stron.
Upewnij się co do konsekwencji tej operacji, zanim się na nią zdecydujesz.",
'movepagetalktext'        => 'Powiązana strona dyskusji, jeśli istnieje, będzie przeniesiona automatycznie, chyba że:
*niepusta strona dyskusji już jest pod nową nazwą
*usuniesz zaznaczenie z poniższego pola wyboru

W takich przypadkach treść dyskusji można przenieść tylko ręcznie.',
'movearticle'             => 'Przeniesienie strony',
'movenotallowed'          => 'Nie masz uprawnień do przenoszenia stron w {{GRAMMAR:MS.lp|{{SITENAME}}}}.',
'newtitle'                => 'Nowy tytuł',
'move-watch'              => 'Obserwuj',
'movepagebtn'             => 'Przenieś stronę',
'pagemovedsub'            => 'Przeniesienie powiodło się',
'movepage-moved'          => "<big>'''Strona „$1” została przeniesiona do „$2”.'''</big>", # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => 'Strona o podanej nazwie już istnieje albo wybrana przez Ciebie nazwa nie jest poprawna.
Wybierz inną nazwę.',
'cantmove-titleprotected' => 'Nie możesz przenieść strony, ponieważ nowa nazwa strony jest niedozwolona z powodu zabezpieczenia przed utworzeniem',
'talkexists'              => "'''Strona zawartości została przeniesiona, natomiast strona dyskusji nie, ponieważ strona dyskusji o nowym tytule już istnieje. Połącz teksty obu dyskusji ręcznie.'''",
'movedto'                 => 'przeniesiono do',
'movetalk'                => 'Przenieś także stronę dyskusji, jeśli to możliwe.',
'move-subpages'           => 'Jeśli to możliwe przenieś wszystkie podstrony',
'move-talk-subpages'      => 'Jeśli to możliwe przenieś wszystkie podstrony strony dyskusji',
'movepage-page-exists'    => 'Strona $1 istnieje. Automatyczne nadpisanie nie jest możliwe.',
'movepage-page-moved'     => 'Strona $1 została przeniesiona do $2.',
'movepage-page-unmoved'   => 'Nazwa strony $1 nie może zostać zmieniona na $2.',
'movepage-max-pages'      => 'Przeniesionych zostało $1 {{PLURAL:$1|strona|strony|stron}}. Większa liczba nie może być przeniesiona automatycznie.',
'1movedto2'               => 'stronę [[$1]] przeniósł do [[$2]]',
'1movedto2_redir'         => 'stronę [[$1]] przeniósł do [[$2]] nad przekierowaniem',
'movelogpage'             => 'Przeniesione',
'movelogpagetext'         => 'Lista stron, które ostatnio zostały przeniesione.',
'movereason'              => 'Powód',
'revertmove'              => 'cofnij',
'delete_and_move'         => 'Usuń i przenieś',
'delete_and_move_text'    => '== Przeniesienie wymaga usunięcia innej strony ==
Strona docelowa „[[:$1]]” istnieje.
Czy chcesz ją usunąć, by zrobić miejsce dla przenoszonej strony?',
'delete_and_move_confirm' => 'Tak, usuń stronę',
'delete_and_move_reason'  => 'Usunięto, by zrobić miejsce dla przenoszonej strony',
'selfmove'                => 'Nazwy stron źródłowej i docelowej są takie same.
Strony nie można przenieść na nią samą.',
'immobile_namespace'      => 'Strona źródłowa lub strona docelowa są specjalnego typu.
Nie można przenieść z lub do tej przestrzeni nazw.',
'imagenocrossnamespace'   => 'Nie można przenieść grafiki do przestrzeni nazw nie przeznaczonej dla grafik',
'imagetypemismatch'       => 'Nowe rozszerzenie nazwy pliku jest innego typu niż zawartość',
'imageinvalidfilename'    => 'Nazwa pliku docelowego jest nieprawidłowa',
'fix-double-redirects'    => 'Popraw przekierowania wskazujące na oryginalny tytuł strony',

# Export
'export'            => 'Eksport stron',
'exporttext'        => 'Możesz wyeksportować treść i historię edycji jednej strony lub zestawu stron w formacie XML.
Wyeksportowane informacje można później zaimportować do innej wiki, działającej na oprogramowaniu MediaWiki, korzystając ze [[Special:Import|strony importu]].

Wyeksportowanie wielu stron wymaga wpisania poniżej tytułów stron po jednym tytule w wierszu oraz określenia, czy ma zostać wyeksportowana bieżąca czy wszystkie wersje strony z opisami edycji lub też tylko bieżąca wersja z opisem ostatniej edycji.

Możesz również użyć linku, np. [[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]] dla strony „[[{{MediaWiki:Mainpage}}]]”.',
'exportcuronly'     => 'Tylko bieżąca wersja, bez historii',
'exportnohistory'   => "----
'''Uwaga:''' Wyłączono możliwość eksportowania pełnej historii stron z użyciem tego narzędzia z powodu kłopotów z wydajnością.",
'export-submit'     => 'Eksportuj',
'export-addcattext' => 'Dodaj strony z kategorii',
'export-addcat'     => 'Dodaj',
'export-download'   => 'Zapisz do pliku',
'export-templates'  => 'Dołącz szablony',

# Namespace 8 related
'allmessages'               => 'Komunikaty systemowe',
'allmessagesname'           => 'Nazwa',
'allmessagesdefault'        => 'Tekst domyślny',
'allmessagescurrent'        => 'Tekst obecny',
'allmessagestext'           => 'Lista wszystkich komunikatów systemowych dostępnych w przestrzeni nazw MediaWiki.
Odwiedź [http://www.mediawiki.org/wiki/Localisation Tłumaczenie MediaWiki] oraz [http://translatewiki.net Betawiki], jeśli chcesz uczestniczyć w tłumaczeniu oprogramowania MediaWiki.',
'allmessagesnotsupportedDB' => "Ta strona nie może być użyta, ponieważ zmienna '''\$wgUseDatabaseMessages''' jest wyłączona.",
'allmessagesfilter'         => 'Filtr nazw komunikatów:',
'allmessagesmodified'       => 'Pokaż tylko zmodyfikowane',

# Thumbnails
'thumbnail-more'           => 'Powiększ',
'filemissing'              => 'Brak pliku',
'thumbnail_error'          => 'Błąd przy generowaniu miniatury $1',
'djvu_page_error'          => 'Strona DjVu poza zakresem',
'djvu_no_xml'              => 'Nie można pobrać danych w formacie XML dla pliku DjVu',
'thumbnail_invalid_params' => 'Nieprawidłowe parametry miniatury',
'thumbnail_dest_directory' => 'Nie można utworzyć katalogu docelowego',

# Special:Import
'import'                     => 'Importuj strony',
'importinterwiki'            => 'Import transwiki',
'import-interwiki-text'      => 'Wybierz wiki i nazwę strony do importowania.
Daty oraz nazwy autorów zostaną zachowane.
Wszystkie operacje importu transwiki są odnotowywane w [[Special:Log/import|rejestrze importu]].',
'import-interwiki-history'   => 'Kopiuj całą historię edycji tej strony',
'import-interwiki-submit'    => 'Importuj',
'import-interwiki-namespace' => 'Przenieś strony do przestrzeni nazw',
'importtext'                 => 'Używając narzędzia [[Special:Export|eksportu]], wyeksportuj plik ze źródłowej wiki, zapisz go na swoim dysku, a następnie prześlij go tutaj.',
'importstart'                => 'Trwa importowanie stron...',
'import-revision-count'      => '$1 {{PLURAL:$1|wersja|wersje|wersji}}',
'importnopages'              => 'Brak stron do importu.',
'importfailed'               => 'Import nie powiódł się: $1',
'importunknownsource'        => 'Nieznany format importowanych danych',
'importcantopen'             => 'Nie można otworzyć importowanego pliku',
'importbadinterwiki'         => 'Błędny link interwiki',
'importnotext'               => 'Brak tekstu lub zawartości',
'importsuccess'              => 'Import zakończony powodzeniem!',
'importhistoryconflict'      => 'Wystąpił konflikt wersji (ta strona mogła zostać importowana już wcześniej)',
'importnosources'            => 'Możliwość bezpośredniego importu historii została wyłączona, ponieważ nie zdefiniowano źródła.',
'importnofile'               => 'Importowany plik nie został przesłany.',
'importuploaderrorsize'      => 'Przesyłanie pliku importowanego zawiodło. Jest większy niż dopuszczalny rozmiar dla przesyłanych plików.',
'importuploaderrorpartial'   => 'Przesyłanie pliku importowanego zawiodło. Został przesłany tylko częściowo.',
'importuploaderrortemp'      => 'Przesyłanie pliku importowanego zawiodło. Brak katalogu na dla plików tymczasowych.',
'import-parse-failure'       => 'nieudana analiza składni importowanego XML',
'import-noarticle'           => 'Brak stron do zaimportowania!',
'import-nonewrevisions'      => 'Wszystkie wersje zostały już wcześniej zaimportowane.',
'xml-error-string'           => '$1 linia $2, kolumna $3 (bajt $4): $5',
'import-upload'              => 'Prześlij dane w formacie XML',

# Import log
'importlogpage'                    => 'Rejestr importu',
'importlogpagetext'                => 'Rejestr przeprowadzonych importów stron z innych serwisów wiki.',
'import-logentry-upload'           => 'zaimportował [[$1]] przez przesłanie pliku',
'import-logentry-upload-detail'    => '$1 {{PLURAL:$1|wersja|wersje|wersji}}',
'import-logentry-interwiki'        => 'zaimportował $1 używając transwiki',
'import-logentry-interwiki-detail' => '$1 {{PLURAL:$1|wersja|wersje|wersji}} z $2',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'Moja osobista strona',
'tooltip-pt-anonuserpage'         => 'Strona użytkownika dla adresu IP, spod którego edytujesz',
'tooltip-pt-mytalk'               => 'Moja strona dyskusji',
'tooltip-pt-anontalk'             => 'Dyskusja użytkownika dla adresu IP, spod którego edytujesz',
'tooltip-pt-preferences'          => 'Moje preferencje',
'tooltip-pt-watchlist'            => 'Lista stron obserwowanych przez Ciebie',
'tooltip-pt-mycontris'            => 'Lista moich edycji',
'tooltip-pt-login'                => 'Zachęcamy do zalogowania się, choć nie jest to obowiązkowe.',
'tooltip-pt-anonlogin'            => 'Zachęcamy do zalogowania się, choć nie jest to obowiązkowe',
'tooltip-pt-logout'               => 'Wyloguj',
'tooltip-ca-talk'                 => 'Dyskusja o zawartości tej strony.',
'tooltip-ca-edit'                 => 'Możesz edytować tę stronę. Przed zapisaniem zmian użyj przycisku podgląd.',
'tooltip-ca-addsection'           => 'Dodaj swój komentarz do dyskusji.',
'tooltip-ca-viewsource'           => 'Ta strona jest zabezpieczona. Możesz zobaczyć tekst źródłowy.',
'tooltip-ca-history'              => 'Starsze wersje tej strony.',
'tooltip-ca-protect'              => 'Zabezpiecz tę stronę.',
'tooltip-ca-delete'               => 'Usuń tę stronę',
'tooltip-ca-undelete'             => 'Przywróć wersję tej strony sprzed usunięcia',
'tooltip-ca-move'                 => 'Przenieś tę stronę.',
'tooltip-ca-watch'                => 'Dodaj tę stronę do listy obserwowanych',
'tooltip-ca-unwatch'              => 'Usuń tę stronę z listy obserwowanych',
'tooltip-search'                  => 'Przeszukaj {{GRAMMAR:B.lp|{{SITENAME}}}}',
'tooltip-search-go'               => 'Przejdź do strony o dokładnie takim tytule, o ile istnieje',
'tooltip-search-fulltext'         => 'Szukaj wprowadzonego tekstu na stronach',
'tooltip-p-logo'                  => 'Strona główna',
'tooltip-n-mainpage'              => 'Zobacz stronę główną',
'tooltip-n-portal'                => 'O projekcie, co możesz zrobić, gdzie możesz znaleźć informacje',
'tooltip-n-currentevents'         => 'Informacje o aktualnych wydarzeniach',
'tooltip-n-recentchanges'         => 'Lista ostatnich zmian na {{GRAMMAR:D.lp|{{SITENAME}}}}.',
'tooltip-n-randompage'            => 'Pokaż losowo wybraną stronę',
'tooltip-n-help'                  => 'Tutaj możesz się wielu rzeczy dowiedzieć.',
'tooltip-t-whatlinkshere'         => 'Pokaż listę wszystkich stron linkujących do tej strony',
'tooltip-t-recentchangeslinked'   => 'Ostatnie zmiany w stronach, do których ta strona linkuje',
'tooltip-feed-rss'                => 'Kanał RSS dla tej strony',
'tooltip-feed-atom'               => 'Kanał Atom dla tej strony',
'tooltip-t-contributions'         => 'Pokaż listę edycji tego użytkownika',
'tooltip-t-emailuser'             => 'Wyślij e-mail do tego użytkownika',
'tooltip-t-upload'                => 'Prześlij plik',
'tooltip-t-specialpages'          => 'Lista wszystkich specjalnych stron',
'tooltip-t-print'                 => 'Wersja do wydruku',
'tooltip-t-permalink'             => 'Stały link do tej wersji strony',
'tooltip-ca-nstab-main'           => 'Zobacz stronę zawartości',
'tooltip-ca-nstab-user'           => 'Zobacz stronę osobistą użytkownika',
'tooltip-ca-nstab-media'          => 'Pokaż stronę pliku',
'tooltip-ca-nstab-special'        => 'To jest strona specjalna. Nie możesz jej edytować.',
'tooltip-ca-nstab-project'        => 'Zobacz stronę projektu',
'tooltip-ca-nstab-image'          => 'Zobacz stronę grafiki',
'tooltip-ca-nstab-mediawiki'      => 'Zobacz komunikat systemowy',
'tooltip-ca-nstab-template'       => 'Zobacz szablon',
'tooltip-ca-nstab-help'           => 'Zobacz stronę pomocy',
'tooltip-ca-nstab-category'       => 'Zobacz stronę kategorii',
'tooltip-minoredit'               => 'Oznacz zmianę jako drobną',
'tooltip-save'                    => 'Zapisz zmiany',
'tooltip-preview'                 => 'Obejrzyj efekt swojej edycji przed zapisaniem zmian!',
'tooltip-diff'                    => 'Pokaż zmiany wykonane w tekście.',
'tooltip-compareselectedversions' => 'Pokazuje różnice między dwiema wybranymi wersjami tej strony.',
'tooltip-watch'                   => 'Dodaj tę stronę do listy obserwowanych',
'tooltip-recreate'                => 'Utwórz stronę pomimo jej wcześniejszego usunięcia.',
'tooltip-upload'                  => 'Rozpoczęcie przesyłania',

# Stylesheets
'common.css'   => '/* Umieszczony tutaj kod CSS zostanie zastosowany we wszystkich skórkach */',
'monobook.css' => '/* Umieszczony tutaj kod CSS wpłynie na wygląd skórki Monobook */',

# Scripts
'common.js'   => '/* Umieszczony tutaj kod JavaScript zostanie załadowany przez każdego użytkownika, podczas każdego ładowania strony. */',
'monobook.js' => '/* Umieszczony tu kod JavaScript zostanie załadowany wyłącznie przez użytkowników korzystających ze skórki MonoBook */',

# Metadata
'nodublincore'      => 'Metadane zgodne z Dublin Core RDF zostały wyłączone dla tego serwera.',
'nocreativecommons' => 'Metadane zgodne z Creative Commons RDF zostały wyłączone dla tego serwera.',
'notacceptable'     => 'Serwer wiki nie może dostarczyć danych w formacie, którego Twoja przeglądarka oczekuje.',

# Attribution
'anonymous'        => 'Anonimowi użytkownicy {{GRAMMAR:D.lp|{{SITENAME}}}}',
'siteuser'         => 'Użytkownik {{GRAMMAR:D.lp|{{SITENAME}}}} – $1',
'lastmodifiedatby' => 'Ostatnia edycja tej strony: $2, $1 (autor zmian: $3)', # $1 date, $2 time, $3 user
'othercontribs'    => 'Inni autorzy: $1.',
'others'           => 'inni',
'siteusers'        => 'Użytkownicy {{GRAMMAR:D.lp|{{SITENAME}}}}: $1',
'creditspage'      => 'Autorzy',
'nocredits'        => 'Brak informacji o autorach tej strony.',

# Spam protection
'spamprotectiontitle' => 'Filtr antyspamowy',
'spamprotectiontext'  => 'Strona, którą próbowałeś zapisać, została zablokowana przez filtr antyspamowy.
Najprawdopodobniej zostało to spowodowane przez link do zewnętrznej strony internetowej.',
'spamprotectionmatch' => 'Filtr antyspamowy zadziałał ponieważ odnalazł tekst: $1',
'spambot_username'    => 'MediaWiki – usuwanie spamu',
'spam_reverting'      => 'Przywracanie ostatniej wersji nie zawierającej linków do $1',
'spam_blanking'       => 'Wszystkie wersje zawierały odnośniki do $1. Czyszczenie strony.',

# Info page
'infosubtitle'   => 'Informacja o stronie',
'numedits'       => 'Liczba edycji (strona zawartości): $1',
'numtalkedits'   => 'Liczba edycji (strona dyskusji): $1',
'numwatchers'    => 'Liczba obserwujących: $1',
'numauthors'     => 'Liczba autorów (strona zawartości): $1',
'numtalkauthors' => 'Liczba autorów (strona dyskusji): $1',

# Math options
'mw_math_png'    => 'Zawsze generuj grafikę PNG',
'mw_math_simple' => 'HTML dla prostych, dla pozostałych grafika PNG',
'mw_math_html'   => 'Spróbuj HTML, a jeśli zawiedzie użyj grafiki PNG',
'mw_math_source' => 'Pozostaw w TeXu (dla przeglądarek tekstowych)',
'mw_math_modern' => 'HTML – zalecane dla nowych przeglądarek',
'mw_math_mathml' => 'MathML jeśli dostępny (eksperymentalne)',

# Patrolling
'markaspatrolleddiff'                 => 'oznacz edycję jako „sprawdzoną”',
'markaspatrolledtext'                 => 'Oznacz tę stronę jako „sprawdzony”',
'markedaspatrolled'                   => 'Sprawdzone',
'markedaspatrolledtext'               => 'Ta wersja została oznaczona jako „sprawdzona”.',
'rcpatroldisabled'                    => 'Wyłączono funkcjonalność patrolowania na ostatnich zmianach',
'rcpatroldisabledtext'                => 'Patrolowanie ostatnich zmian jest obecnie wyłączone.',
'markedaspatrollederror'              => 'Nie można oznaczyć jako „sprawdzone”',
'markedaspatrollederrortext'          => 'Musisz wybrać wersję żeby oznaczyć ją jako „sprawdzoną”.',
'markedaspatrollederror-noautopatrol' => 'Nie masz uprawnień wymaganych do oznaczania swoich edycji jako „sprawdzone”.',

# Patrol log
'patrol-log-page'   => 'Dziennik patrolowania',
'patrol-log-header' => 'Poniżej znajduje się dziennik patrolowania stron.',
'patrol-log-line'   => 'oznaczył wersję $1 hasła $2 jako sprawdzoną $3',
'patrol-log-auto'   => '(automatycznie)',

# Image deletion
'deletedrevision'                 => 'Usunięto poprzednie wersje $1',
'filedeleteerror-short'           => 'Błąd przy usuwaniu pliku $1',
'filedeleteerror-long'            => 'Wystąpiły błędy przy usuwaniu pliku:

$1',
'filedelete-missing'              => 'Pliku „$1” nie można usunąć, ponieważ nie istnieje.',
'filedelete-old-unregistered'     => 'Brak w bazie danych żądanej wersji pliku „$1”.',
'filedelete-current-unregistered' => 'Brak w bazie danych pliku „$1”.',
'filedelete-archive-read-only'    => 'Serwer WWW nie może zapisywać w katalogu z archiwami „$1”.',

# Browsing diffs
'previousdiff' => '← poprzednia edycja',
'nextdiff'     => 'następna edycja →',

# Media information
'mediawarning'         => "'''Uwaga!''' Plik może zawierać złośliwy kod. Jeśli go otworzysz, możesz zarazić swój system.<hr />",
'imagemaxsize'         => 'Na stronach opisu plików ogranicz rozmiar obrazków do',
'thumbsize'            => 'Rozmiar miniaturki',
'widthheightpage'      => '$1×$2, $3 {{PLURAL:$3|strona|strony|stron}}',
'file-info'            => '(rozmiar pliku: $1, typ MIME: $2)',
'file-info-size'       => '($1 × $2 pikseli, rozmiar pliku: $3, typ MIME: $4)',
'file-nohires'         => '<small>Grafika w wyższej rozdzielczości jest niedostępna.</small>',
'svg-long-desc'        => '(Plik SVG, nominalnie $1 × $2 pikseli, rozmiar pliku: $3)',
'show-big-image'       => 'Oryginalna rozdzielczość',
'show-big-image-thumb' => '<small>Rozmiar podglądu: $1 × $2 pikseli</small>',

# Special:NewImages
'newimages'             => 'Najnowsze pliki',
'imagelisttext'         => "Poniżej na {{PLURAL:$1||posortowanej $2}} liście {{PLURAL:$1|znajduje|znajdują|znajduje}} się '''$1''' {{PLURAL:$1|plik|pliki|plików}}.",
'newimages-summary'     => 'Na tej stronie specjalnej prezentowane są ostatnio przesłane pliki.',
'showhidebots'          => '($1 boty)',
'noimages'              => 'Brak plików do pokazania.',
'ilsubmit'              => 'Szukaj',
'bydate'                => 'według daty',
'sp-newimages-showfrom' => 'pokaż nowe pliki począwszy od $2, $1',

# Video information, used by Language::formatTimePeriod() to format lengths in the above messages
'hours-abbrev' => 'g',

# Bad image list
'bad_image_list' => 'Dane należy wprowadzić w formacie:

Jedynie elementy listy (linie zaczynające się od znaku gwiazdki *) brane są pod uwagę.
Pierwszy link w linii musi być linkiem do zabronionego pliku. 
Następne linki w linii są traktowane jako wyjątki – są to nazwy stron, na których plik o zabronionej nazwie może być użyty.',

# Metadata
'metadata'          => 'Metadane',
'metadata-help'     => 'Niniejszy plik zawiera dodatkowe informacje, prawdopodobnie dodane przez aparat cyfrowy lub skaner użyte do wygenerowania tego pliku.
Jeśli plik był modyfikowany, dane mogą być częściowo niezgodne z parametrami zmodyfikowanego pliku.',
'metadata-expand'   => 'Pokaż szczegóły',
'metadata-collapse' => 'Ukryj szczegóły',
'metadata-fields'   => 'Wymienione poniżej pola EXIF będą prezentowane na stronie grafiki.
Pozostałe pola zostaną domyślnie ukryte.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => 'Szerokość',
'exif-imagelength'                 => 'Wysokość',
'exif-bitspersample'               => 'Bitów na próbkę',
'exif-compression'                 => 'Metoda kompresji',
'exif-photometricinterpretation'   => 'Interpretacja fotometryczna',
'exif-orientation'                 => 'Orientacja obrazu',
'exif-samplesperpixel'             => 'Próbek na piksel',
'exif-planarconfiguration'         => 'Rozkład danych',
'exif-ycbcrsubsampling'            => 'Podpróbkowanie Y do C',
'exif-ycbcrpositioning'            => 'Rozmieszczenie Y i C',
'exif-xresolution'                 => 'Rozdzielczość w poziomie',
'exif-yresolution'                 => 'Rozdzielczość w pionie',
'exif-resolutionunit'              => 'Jednostka rozdzielczości X i Y',
'exif-stripoffsets'                => 'Przesunięcie pasów obrazu',
'exif-rowsperstrip'                => 'Liczba wierszy na pas obrazu',
'exif-stripbytecounts'             => 'Liczba bajtów na pas obrazu',
'exif-jpeginterchangeformat'       => 'Położenie pierwszego bajtu miniaturki obrazu',
'exif-jpeginterchangeformatlength' => 'Liczba bajtów miniaturki JPEG',
'exif-transferfunction'            => 'Funkcja przejścia',
'exif-whitepoint'                  => 'Punkt bieli',
'exif-primarychromaticities'       => 'Kolory trzech barw głównych',
'exif-ycbcrcoefficients'           => 'Macierz współczynników transformacji barw z RGB na YCbCr',
'exif-referenceblackwhite'         => 'Wartość punktu odniesienia czerni i bieli',
'exif-datetime'                    => 'Data i czas modyfikacji pliku',
'exif-imagedescription'            => 'Tytuł/opis obrazu',
'exif-make'                        => 'Producent aparatu',
'exif-model'                       => 'Model aparatu',
'exif-software'                    => 'Użyte oprogramowanie',
'exif-artist'                      => 'Autor',
'exif-copyright'                   => 'Właściciel praw autorskich',
'exif-exifversion'                 => 'Wersja standardu Exif',
'exif-flashpixversion'             => 'Obsługiwana wersja Flashpix',
'exif-colorspace'                  => 'Przestrzeń kolorów',
'exif-componentsconfiguration'     => 'Znaczenie składowych',
'exif-compressedbitsperpixel'      => 'Skompresowanych bitów na piksel',
'exif-pixelydimension'             => 'Prawidłowa szerokość obrazu',
'exif-pixelxdimension'             => 'Prawidłowa wysokość obrazu',
'exif-makernote'                   => 'Informacje producenta aparatu',
'exif-usercomment'                 => 'Komentarz użytkownika',
'exif-relatedsoundfile'            => 'Powiązany plik audio',
'exif-datetimeoriginal'            => 'Data i czas utworzenia oryginału',
'exif-datetimedigitized'           => 'Data i czas zeskanowania',
'exif-subsectime'                  => 'Data i czas modyfikacji pliku – ułamki sekund',
'exif-subsectimeoriginal'          => 'Data i czas utworzenia oryginału – ułamki sekund',
'exif-subsectimedigitized'         => 'Data i czas zeskanowania – ułamki sekund',
'exif-exposuretime'                => 'Czas ekspozycji',
'exif-exposuretime-format'         => '$1 s ($2)',
'exif-fnumber'                     => 'Wartość przysłony',
'exif-exposureprogram'             => 'Program ekspozycji',
'exif-spectralsensitivity'         => 'Czułość widmowa',
'exif-isospeedratings'             => 'Szybkość aparatu zgodnie z ISO12232',
'exif-oecf'                        => 'Funkcja konwersji obrazu na dane zgodnie z ISO14524',
'exif-shutterspeedvalue'           => 'Szybkość migawki',
'exif-aperturevalue'               => 'Przysłona obiektywu',
'exif-brightnessvalue'             => 'Jasność',
'exif-exposurebiasvalue'           => 'Odchylenie ekspozycji',
'exif-maxaperturevalue'            => 'Maksymalna wartość przysłony',
'exif-subjectdistance'             => 'Odległość od obiektu',
'exif-meteringmode'                => 'Tryb pomiaru',
'exif-lightsource'                 => 'Rodzaj źródła światła',
'exif-flash'                       => 'Lampa błyskowa',
'exif-focallength'                 => 'Długość ogniskowej obiektywu',
'exif-subjectarea'                 => 'Położenie i obszar głównego motywu obrazu',
'exif-flashenergy'                 => 'Energia lampy błyskowej',
'exif-spatialfrequencyresponse'    => 'Odpowiedź częstotliwości przestrzennej zgodnie z ISO12233',
'exif-focalplanexresolution'       => 'Rozdzielczość w poziomie płaszczyzny odwzorowania obiektywu',
'exif-focalplaneyresolution'       => 'Rozdzielczość w pionie płaszczyzny odwzorowania obiektywu',
'exif-focalplaneresolutionunit'    => 'Jednostka rozdzielczości płaszczyzny odwzorowania obiektywu',
'exif-subjectlocation'             => 'Położenie głównego motywu obrazu',
'exif-exposureindex'               => 'Indeks ekspozycji',
'exif-sensingmethod'               => 'Metoda pomiaru (rodzaj przetwornika)',
'exif-filesource'                  => 'Typ źródła pliku',
'exif-scenetype'                   => 'Rodzaj sceny',
'exif-cfapattern'                  => 'Wzór CFA',
'exif-customrendered'              => 'Wstępnie przetworzony (poddany obróbce)',
'exif-exposuremode'                => 'Tryb ekspozycji',
'exif-whitebalance'                => 'Balans bieli',
'exif-digitalzoomratio'            => 'Współczynnik powiększenia cyfrowego',
'exif-focallengthin35mmfilm'       => 'Długość ogniskowej, odpowiednik dla filmu 35mm',
'exif-scenecapturetype'            => 'Rodzaj uchwycenia sceny',
'exif-gaincontrol'                 => 'Wzmocnienie jasności obrazu',
'exif-contrast'                    => 'Kontrast obrazu',
'exif-saturation'                  => 'Nasycenie kolorów obrazu',
'exif-sharpness'                   => 'Ostrość obrazu',
'exif-devicesettingdescription'    => 'Opis ustawień urządzenia',
'exif-subjectdistancerange'        => 'Odległość od obiektu',
'exif-imageuniqueid'               => 'Unikalny identyfikator obrazu',
'exif-gpsversionid'                => 'Wersja formatu danych GPS',
'exif-gpslatituderef'              => 'Szerokość geograficzna (północ/południe)',
'exif-gpslatitude'                 => 'Szerokość geograficzna',
'exif-gpslongituderef'             => 'Długość geograficzna (wschód/zachód)',
'exif-gpslongitude'                => 'Długość geograficzna',
'exif-gpsaltituderef'              => 'Wysokość nad poziomem morza (odniesienie)',
'exif-gpsaltitude'                 => 'Wysokość nad poziomem morza',
'exif-gpstimestamp'                => 'Czas GPS (zegar atomowy)',
'exif-gpssatellites'               => 'Satelity użyte do pomiaru',
'exif-gpsstatus'                   => 'Otrzymany status',
'exif-gpsmeasuremode'              => 'Tryb pomiaru',
'exif-gpsdop'                      => 'Precyzja pomiaru',
'exif-gpsspeedref'                 => 'Jednostka prędkości',
'exif-gpsspeed'                    => 'Prędkość pozioma',
'exif-gpstrackref'                 => 'Poprawka pomiędzy kierunkiem i celem',
'exif-gpstrack'                    => 'Kierunek ruchu',
'exif-gpsimgdirectionref'          => 'Poprawka dla kierunku zdjęcia',
'exif-gpsimgdirection'             => 'Kierunek zdjęcia',
'exif-gpsmapdatum'                 => 'Model pomiaru geodezyjnego',
'exif-gpsdestlatituderef'          => 'Północna lub południowa szerokość geograficzna celu',
'exif-gpsdestlatitude'             => 'Szerokość geograficzna celu',
'exif-gpsdestlongituderef'         => 'Wschodnia lub zachodnia długość geograficzna celu',
'exif-gpsdestlongitude'            => 'Długość geograficzna celu',
'exif-gpsdestbearingref'           => 'Znacznik namiaru na cel (kierunku)',
'exif-gpsdestbearing'              => 'Namiar na cel (kierunek)',
'exif-gpsdestdistanceref'          => 'Znacznik odległości do celu',
'exif-gpsdestdistance'             => 'Odległość od celu',
'exif-gpsprocessingmethod'         => 'Nazwa metody GPS',
'exif-gpsareainformation'          => 'Nazwa przestrzeni GPS',
'exif-gpsdatestamp'                => 'Data GPS',
'exif-gpsdifferential'             => 'Korekcja różnicy GPS',

# EXIF attributes
'exif-compression-1' => 'nieskompresowany',

'exif-unknowndate' => 'nieznana data',

'exif-orientation-1' => 'normalna', # 0th row: top; 0th column: left
'exif-orientation-2' => 'odbicie lustrzane w poziomie', # 0th row: top; 0th column: right
'exif-orientation-3' => 'obraz obrócony o 180°', # 0th row: bottom; 0th column: right
'exif-orientation-4' => 'odbicie lustrzane w pionie', # 0th row: bottom; 0th column: left
'exif-orientation-5' => 'obraz obrócony o 90° przeciwnie do ruchu wskazówek zegara i odbicie lustrzane w pionie', # 0th row: left; 0th column: top
'exif-orientation-6' => 'obraz obrócony o 90° zgodnie z ruchem wskazówek zegara', # 0th row: right; 0th column: top
'exif-orientation-7' => 'obrót o 90° zgodnie ze wskazówkami zegara i odbicie lustrzane w pionie', # 0th row: right; 0th column: bottom
'exif-orientation-8' => 'obrót o 90° przeciwnie do wskazówek zegara', # 0th row: left; 0th column: bottom

'exif-planarconfiguration-1' => 'format masywny',
'exif-planarconfiguration-2' => 'format powierzchniowy',

'exif-componentsconfiguration-0' => 'nie istnieje',

'exif-exposureprogram-0' => 'niezdefiniowany',
'exif-exposureprogram-1' => 'ręczny',
'exif-exposureprogram-2' => 'standardowy',
'exif-exposureprogram-3' => 'preselekcja przysłony',
'exif-exposureprogram-4' => 'preselekcja migawki',
'exif-exposureprogram-5' => 'kreatywny (duża głębia ostrości)',
'exif-exposureprogram-6' => 'aktywny (duża szybkość migawki)',
'exif-exposureprogram-7' => 'tryb portretowy (dla zdjęć z bliska, z nieostrym tłem)',
'exif-exposureprogram-8' => 'tryb krajobrazowy (dla zdjęć wykonywanych z dużej odległości z ostrością ustawioną na tło)',

'exif-subjectdistance-value' => '$1 metrów',

'exif-meteringmode-0'   => 'nieokreślony',
'exif-meteringmode-1'   => 'średnia',
'exif-meteringmode-2'   => 'średnia ważona',
'exif-meteringmode-3'   => 'punktowy',
'exif-meteringmode-4'   => 'wielopunktowy',
'exif-meteringmode-5'   => 'próbkowanie',
'exif-meteringmode-6'   => 'częściowy',
'exif-meteringmode-255' => 'inny',

'exif-lightsource-0'   => 'nieznany',
'exif-lightsource-1'   => 'dzienne',
'exif-lightsource-2'   => 'jarzeniowe',
'exif-lightsource-3'   => 'sztuczne (żarowe)',
'exif-lightsource-4'   => 'lampa błyskowa (flesz)',
'exif-lightsource-9'   => 'dzienne (dobra pogoda)',
'exif-lightsource-10'  => 'dzienne (pochmurno)',
'exif-lightsource-11'  => 'cień',
'exif-lightsource-12'  => 'jarzeniowe dzienne (temperatura barwowa 5700 – 7100K)',
'exif-lightsource-13'  => 'jarzeniowe ciepłe (temperatura barwowa 4600 – 5400K)',
'exif-lightsource-14'  => 'jarzeniowe zimne (temperatura barwowa 3900 – 4500K)',
'exif-lightsource-15'  => 'jarzeniowe białe (temperatura barwowa 3200 – 3700K)',
'exif-lightsource-17'  => 'standardowe A',
'exif-lightsource-18'  => 'standardowe B',
'exif-lightsource-19'  => 'standardowe C',
'exif-lightsource-24'  => 'żarowe studyjne ISO',
'exif-lightsource-255' => 'Inne źródło światła',

'exif-focalplaneresolutionunit-2' => 'cale',

'exif-sensingmethod-1' => 'niezdefiniowana',
'exif-sensingmethod-2' => 'jednoukładowy przetwornik obrazu kolorowego',
'exif-sensingmethod-3' => 'dwuukładowy przetwornik obrazu kolorowego',
'exif-sensingmethod-4' => 'trójukładowy przetwornik obrazu kolorowego',
'exif-sensingmethod-5' => 'przetwornik obrazu z sekwencyjnym przetwarzaniem kolorów',
'exif-sensingmethod-7' => 'trójliniowy przetwornik obrazu',
'exif-sensingmethod-8' => 'liniowy przetwornik obrazu z sekwencyjnym przetwarzaniem kolorów',

'exif-scenetype-1' => 'obiekt fotografowany bezpośrednio',

'exif-customrendered-0' => 'nie',
'exif-customrendered-1' => 'tak',

'exif-exposuremode-0' => 'automatyczne ustalenie parametrów naświetlania',
'exif-exposuremode-1' => 'ręczne ustalenie parametrów naświetlania',
'exif-exposuremode-2' => 'wielokrotna ze zmianą parametrów naświetlania',

'exif-whitebalance-0' => 'automatyczny',
'exif-whitebalance-1' => 'ręczny',

'exif-scenecapturetype-0' => 'standardowy',
'exif-scenecapturetype-1' => 'krajobraz',
'exif-scenecapturetype-2' => 'portret',
'exif-scenecapturetype-3' => 'scena nocna',

'exif-gaincontrol-0' => 'brak',
'exif-gaincontrol-1' => 'niskie wzmocnienie',
'exif-gaincontrol-2' => 'wysokie wzmocnienie',
'exif-gaincontrol-3' => 'niskie osłabienie',
'exif-gaincontrol-4' => 'wysokie osłabienie',

'exif-contrast-0' => 'normalny',
'exif-contrast-1' => 'niski',
'exif-contrast-2' => 'wysoki',

'exif-saturation-0' => 'normalne',
'exif-saturation-1' => 'niskie',
'exif-saturation-2' => 'wysokie',

'exif-sharpness-0' => 'normalna',
'exif-sharpness-1' => 'niska',
'exif-sharpness-2' => 'wysoka',

'exif-subjectdistancerange-0' => 'nieznana',
'exif-subjectdistancerange-1' => 'makro',
'exif-subjectdistancerange-2' => 'widok z bliska',
'exif-subjectdistancerange-3' => 'widok z daleka',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'północna',
'exif-gpslatitude-s' => 'południowa',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'wschodnia',
'exif-gpslongitude-w' => 'zachodnia',

'exif-gpsstatus-a' => 'pomiar w trakcie',
'exif-gpsstatus-v' => 'wyniki pomiaru dostępne na bieżąco',

'exif-gpsmeasuremode-2' => 'dwuwymiarowy',
'exif-gpsmeasuremode-3' => 'trójwymiarowy',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'kilometrów na godzinę',
'exif-gpsspeed-m' => 'mil na godzinę',
'exif-gpsspeed-n' => 'węzłów',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => 'kierunek geograficzny',
'exif-gpsdirection-m' => 'kierunek magnetyczny',

# External editor support
'edit-externally'      => 'Edytuj plik, używając zewnętrznej aplikacji',
'edit-externally-help' => "Więcej informacji o używaniu [http://www.mediawiki.org/wiki/Manual:External_editors zewnętrznych edytorów] (''ang.'').",

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'wszystkie',
'imagelistall'     => 'wszystkich',
'watchlistall2'    => 'wszystkie',
'namespacesall'    => 'wszystkie',
'monthsall'        => 'wszystkie',

# E-mail address confirmation
'confirmemail'             => 'Potwierdzanie adresu e-mail',
'confirmemail_noemail'     => 'Nie podałeś prawidłowego adresu e-mail w [[Special:Preferences|preferencjach]].',
'confirmemail_text'        => 'Projekt {{SITENAME}} wymaga weryfikacji adresu e-mail przed użyciem funkcji korzystających z poczty.
Wciśnij przycisk poniżej aby wysłać na swój adres list z linkiem do strony WWW.
List będzie zawierał link do strony, w którym zakodowany będzie identyfikator.
Otwórz ten link w przeglądarce, czym potwierdzisz, że jesteś użytkownikiem tego adresu e-mail.',
'confirmemail_pending'     => '<div class="error">Kod potwierdzenia został właśnie do Ciebie wysłany. Jeśli zarejestrowałeś się niedawno, poczekaj kilka minut na dostarczenie wiadomości przed kolejną prośbą o wysłanie kodu.</div>',
'confirmemail_send'        => 'Wyślij kod potwierdzenia',
'confirmemail_sent'        => 'Wiadomość e-mail z kodem uwierzytelniającym została wysłana.',
'confirmemail_oncreate'    => 'Link z kodem potwierdzenia został wysłany na Twój adres e-mail.
Kod ten nie jest wymagany do zalogowania się, jednak będziesz musiał go aktywować otwierając, otrzymany link, w przeglądarce przed włączeniem niektórych opcji e-mail na wiki.',
'confirmemail_sendfailed'  => 'Nie udało się wysłać potwierdzającej wiadomości e-mail.
Sprawdzić poprawność adresu.

System pocztowy zwrócił komunikat: $1',
'confirmemail_invalid'     => 'Błędny kod potwierdzenia.
Kod może być przedawniony.',
'confirmemail_needlogin'   => 'Musisz $1 aby potwierdzić adres email.',
'confirmemail_success'     => 'Adres e-mail został potwierdzony.
Możesz się zalogować i korzystać z szerszego wachlarza funkcjonalności wiki.',
'confirmemail_loggedin'    => 'Twój adres email został zweryfikowany.',
'confirmemail_error'       => 'Pojawiły się błędy przy zapisywaniu potwierdzenia.',
'confirmemail_subject'     => '{{SITENAME}} - weryfikacja adresu e-mail',
'confirmemail_body'        => 'Ktoś łącząc się z komputera o adresie IP $1
zarejestrował w {{GRAMMAR:MS.lp|{{SITENAME}}}} konto „$2” podając niniejszy adres e-mail.

Aby potwierdzić, że to Ty zarejestrowałeś to konto oraz, aby włączyć
wszystkie funkcje korzystające z poczty elektronicznej, otwórz w swojej
przeglądarce ten link:

$3

Jeśli to *nie* Ty zarejestrowałeś konto, otwórz w swojej przeglądarce
poniższy link, aby anulować potwierdzenie adresu e-mail:

$5

Kod zawarty w linku straci ważność $4.',
'confirmemail_invalidated' => 'Potwierdzenie adresu e-mail zostało anulowane',
'invalidateemail'          => 'Anulowanie potwierdzenia adresu e-mail',

# Scary transclusion
'scarytranscludedisabled' => '[Transkluzja przez interwiki jest wyłączona]',
'scarytranscludefailed'   => '[Pobranie szablonu dla $1 nie powiodło się]',
'scarytranscludetoolong'  => '[zbyt długi adres URL]',

# Trackbacks
'trackbackbox'      => '<div id="mw_trackbacks">Komunikaty TrackBack dla tej strony:<br />$1</div>',
'trackbackremove'   => ' ([$1 Usuń])',
'trackbacklink'     => 'TrackBack',
'trackbackdeleteok' => 'TrackBack został usunięty.',

# Delete conflict
'deletedwhileediting' => "'''Uwaga!''' Ta strona została usunięta po tym, jak rozpocząłeś jej edycję!",
'confirmrecreate'     => "Użytkownik [[User:$1|$1]] ([[User talk:$1|dyskusja]]) usunął tę stronę po tym, jak rozpocząłeś jego edycję, podając jako powód usunięcia:
: ''$2''
Czy na pewno chcesz ją ponownie utworzyć?",
'recreate'            => 'Utwórz ponownie',

# HTML dump
'redirectingto' => 'Przekierowanie do [[:$1]]...',

# action=purge
'confirm_purge'        => 'Wyczyścić pamięć podręczną dla tej strony?

$1',
'confirm_purge_button' => 'Wyczyść',

# AJAX search
'searchcontaining' => "Szukaj artykułów zawierających ''$1''.",
'searchnamed'      => "Szukaj artykułów nazywających się ''$1''.",
'articletitles'    => "Artykuły o tytule rozpoczynającym się od ''$1''",
'hideresults'      => 'Ukryj wyniki',
'useajaxsearch'    => 'Użyj wyszukiwania AJAX',

# Multipage image navigation
'imgmultipageprev' => '← poprzednia strona',
'imgmultipagenext' => 'następna strona →',
'imgmultigo'       => 'Przejdź',
'imgmultigoto'     => 'Idź do $1 strony',

# Table pager
'ascending_abbrev'         => 'rosn.',
'descending_abbrev'        => 'mal.',
'table_pager_next'         => 'Następna strona',
'table_pager_prev'         => 'Poprzednia strona',
'table_pager_first'        => 'Pierwsza strona',
'table_pager_last'         => 'Ostatnia strona',
'table_pager_limit'        => 'Pokaż $1 pozycji na stronie',
'table_pager_limit_submit' => 'Pokaż',
'table_pager_empty'        => 'Brak wyników',

# Auto-summaries
'autosumm-blank'   => 'UWAGA! Usunięcie treści (strona pozostała pusta)!',
'autosumm-replace' => 'UWAGA! Zastąpienie treści hasła bardzo krótkim tekstem: „$1”',
'autoredircomment' => 'Przekierowanie do [[$1]]',
'autosumm-new'     => 'Nowa strona: $1',

# Size units
'size-kilobytes' => '$1 kB',

# Live preview
'livepreview-loading' => 'Trwa ładowanie…',
'livepreview-ready'   => 'Trwa ładowanie… Gotowe!',
'livepreview-failed'  => 'Podgląd na żywo nie zadziałał! Spróbuj podglądu standardowego.',
'livepreview-error'   => 'Nieudane połączenie: $1 „$2” Spróbuj podglądu standardowego.',

# Friendlier slave lag warnings
'lag-warn-normal' => 'Zmiany nowsze niż $1 {{PLURAL:$1|sekunda|sekundy|sekund}} mogą nie być widoczne na tej liście.',
'lag-warn-high'   => 'Z powodu dużego obciążenia serwerów bazy danych, zmiany nowsze niż $1 {{PLURAL:$1|sekunda|sekundy|sekund}} mogą nie być widoczne na tej liście.',

# Watchlist editor
'watchlistedit-numitems'       => 'Twoja lista obserwowanych zawiera {{PLURAL:$1|1 tytuł|$1 tytuły|$1 tytułów}}, nieuwzględniając stron dyskusji.',
'watchlistedit-noitems'        => 'Twoja lista obserwowanych jest pusta.',
'watchlistedit-normal-title'   => 'Edytuj listę obserwowanych stron',
'watchlistedit-normal-legend'  => 'Usuń strony z listy obserwowanych',
'watchlistedit-normal-explain' => 'Poniżej znajduje się lista obserwowanych przez Ciebie stron.
Aby usunąć obserwowaną stronę z listy zaznacz znajdujące się obok niej pole i naciśnij „Usuń zaznaczone pozycje”.
Możesz także skorzystać z [[Special:Watchlist/raw|tekstowego edytora listy obserwowanych]].',
'watchlistedit-normal-submit'  => 'Usuń z listy',
'watchlistedit-normal-done'    => 'Z Twojej listy obserwowanych {{PLURAL:$1|została usunięta 1 strona|zostały usunięte $1 strony|zostało usuniętych $1 stron}}:',
'watchlistedit-raw-title'      => 'Tekstowy edytor listy obserwowanych',
'watchlistedit-raw-legend'     => 'Tekstowy edytor listy obserwowanych',
'watchlistedit-raw-explain'    => 'Poniżej znajduje się lista obserwowanych stron. W każdej linii znajduje się tytuł jednej strony. Listę możesz modyfikować poprzez dodawanie nowych i usuwanie obecnych. Gdy zakończysz, kliknij przycisk „Uaktualnij listę”.
Możesz również [[Special:Watchlist/edit|użyć standardowego edytora]].',
'watchlistedit-raw-titles'     => 'Obserwowane strony:',
'watchlistedit-raw-submit'     => 'Uaktualnij listę',
'watchlistedit-raw-done'       => 'Lista obserwowanych stron została uaktualniona.',
'watchlistedit-raw-added'      => 'Dodano {{PLURAL:$1|1 pozycję|$1 pozycje|$1 pozycji}} do listy obserwowanych:',
'watchlistedit-raw-removed'    => 'Usunięto {{PLURAL:$1|1 pozycję|$1 pozycje|$1 pozycji}} z listy obserwowanych:',

# Watchlist editing tools
'watchlisttools-view' => 'pokaż zmiany na liście obserwowanych',
'watchlisttools-edit' => 'edycja listy',
'watchlisttools-raw'  => 'tekstowy edytor listy',

# Iranian month names
'iranian-calendar-m1'  => 'Farwardin',
'iranian-calendar-m2'  => 'Ordibeheszt',
'iranian-calendar-m3'  => 'Chordād',
'iranian-calendar-m5'  => 'Mordād',
'iranian-calendar-m6'  => 'Szahriwar',
'iranian-calendar-m7'  => 'Mehr',
'iranian-calendar-m8'  => 'Ābān',
'iranian-calendar-m9'  => 'Āsar',
'iranian-calendar-m10' => 'Déi',
'iranian-calendar-m11' => 'Bahman',
'iranian-calendar-m12' => 'Esfand',

# Hebrew month names
'hebrew-calendar-m1'      => 'Tiszri',
'hebrew-calendar-m2'      => 'Heszwan',
'hebrew-calendar-m3'      => 'Kislew',
'hebrew-calendar-m4'      => 'Tewet',
'hebrew-calendar-m5'      => 'Szewat',
'hebrew-calendar-m8'      => 'Ijar',
'hebrew-calendar-m9'      => 'Siwan',
'hebrew-calendar-m11'     => 'Aw',
'hebrew-calendar-m1-gen'  => 'Tiszri',
'hebrew-calendar-m2-gen'  => 'Heszwan',
'hebrew-calendar-m3-gen'  => 'Kislew',
'hebrew-calendar-m4-gen'  => 'Tewet',
'hebrew-calendar-m5-gen'  => 'Szewat',
'hebrew-calendar-m8-gen'  => 'Ijar',
'hebrew-calendar-m9-gen'  => 'Siwan',
'hebrew-calendar-m11-gen' => 'Aw',

# Core parser functions
'unknown_extension_tag' => 'Nieznany znacznik rozszerzenia „$1”',

# Special:Version
'version'                          => 'Wersja oprogramowania', # Not used as normal message but as header for the special page itself
'version-extensions'               => 'Zainstalowane rozszerzenia',
'version-specialpages'             => 'Strony specjalne',
'version-parserhooks'              => 'Haki analizatora składni (ang. parser hooks)',
'version-variables'                => 'Zmienne',
'version-other'                    => 'Pozostałe',
'version-mediahandlers'            => 'Wtyczki obsługi mediów',
'version-hooks'                    => 'Haki (ang. hooks)',
'version-extension-functions'      => 'Funkcje rozszerzeń',
'version-parser-extensiontags'     => 'Znaczniki rozszerzeń dla analizatora składni',
'version-parser-function-hooks'    => 'Funkcje haków analizatora składni (ang. parser function hooks)',
'version-skin-extension-functions' => 'Funkcje rozszerzeń skórek',
'version-hook-name'                => 'Nazwa haka (ang. hook name)',
'version-hook-subscribedby'        => 'Zapotrzebowany przez',
'version-version'                  => 'Wersja',
'version-license'                  => 'Licencja',
'version-software'                 => 'Zainstalowane oprogramowanie',
'version-software-product'         => 'Nazwa',
'version-software-version'         => 'Wersja',

# Special:FilePath
'filepath'         => 'Ścieżka do pliku',
'filepath-page'    => 'Plik',
'filepath-submit'  => 'Ścieżka',
'filepath-summary' => 'Ta strona specjalna zwraca pełną ścieżkę do pliku.
Grafiki są pokazywane w pełnej rozdzielczości, inne typy plików są otwierane w skojarzonym z nimi programie.

Wpisz nazwę pliku bez prefiksu „{{ns:image}}:”.',

# Special:FileDuplicateSearch
'fileduplicatesearch'          => 'Szukaj duplikatów pliku',
'fileduplicatesearch-summary'  => 'Szukaj duplikatów pliku na podstawie wartości funkcji skrótu.

Wpisz nazwę pliku z pominięciem prefiksu „{{ns:image}}:”.',
'fileduplicatesearch-legend'   => 'Szukaj duplikatów pliku',
'fileduplicatesearch-filename' => 'Nazwa pliku',
'fileduplicatesearch-submit'   => 'Szukaj',
'fileduplicatesearch-info'     => '$1 × $2 pikseli<br />Wielkość pliku: $3<br />Typ MIME: $4',
'fileduplicatesearch-result-1' => 'Brak duplikatu pliku „$1”.',
'fileduplicatesearch-result-n' => 'W {{GRAMMAR:MS.lp|{{SITENAME}}}} {{PLURAL:$2|jest dodatkowa kopia|są $2 dodatkowe kopie|jest $2 dodatkowych kopii}} pliku „$1”.',

# Special:SpecialPages
'specialpages'                   => 'Strony specjalne',
'specialpages-note'              => '----
* Strony specjalne ogólnie dostępne.
* <span class="mw-specialpagerestricted">Strony specjalne o ograniczonym dostępie.</span>',
'specialpages-group-maintenance' => 'Raporty konserwacyjne',
'specialpages-group-other'       => 'Inne strony specjalne',
'specialpages-group-login'       => 'Logowanie i rejestracja',
'specialpages-group-changes'     => 'Ostatnie zmiany i rejestry',
'specialpages-group-media'       => 'Pliki',
'specialpages-group-users'       => 'Użytkownicy i uprawnienia',
'specialpages-group-highuse'     => 'Strony często używane',
'specialpages-group-pages'       => 'Strony',
'specialpages-group-pagetools'   => 'Narzędzia stron',
'specialpages-group-wiki'        => 'Informacje oraz narzędzia wiki',
'specialpages-group-redirects'   => 'Specjalne strony przekierowujące',
'specialpages-group-spam'        => 'Narzędzia walki ze spamem',

# Special:BlankPage
'blankpage'              => 'Pusta strona',
'intentionallyblankpage' => 'Ta strona umyślnie pozostała pusta',

);
