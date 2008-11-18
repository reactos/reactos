<?php
/** Finnish (Suomi)
 *
 * @ingroup Language
 * @file
 *
 * @author Agony
 * @author Cimon Avaro
 * @author Crt
 * @author Jaakonam
 * @author Jack Phoenix
 * @author Nike
 * @author Str4nd
 * @author לערי ריינהארט
 */

$separatorTransformTable = array(',' => "\xc2\xa0", '.' => ',' );

$namespaceNames = array(
	NS_MEDIA            => 'Media',
	NS_SPECIAL          => 'Toiminnot',
	NS_MAIN             => '',
	NS_TALK             => 'Keskustelu',
	NS_USER             => 'Käyttäjä',
	NS_USER_TALK        => 'Keskustelu_käyttäjästä',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK     => 'Keskustelu_{{grammar:elative|$1}}',
	NS_IMAGE            => 'Kuva',
	NS_IMAGE_TALK       => 'Keskustelu_kuvasta',
	NS_MEDIAWIKI        => 'Järjestelmäviesti',
	NS_MEDIAWIKI_TALK   => 'Keskustelu_järjestelmäviestistä',
	NS_TEMPLATE         => 'Malline',
	NS_TEMPLATE_TALK    => 'Keskustelu_mallineesta',
	NS_HELP             => 'Ohje',
	NS_HELP_TALK        => 'Keskustelu_ohjeesta',
	NS_CATEGORY         => 'Luokka',
	NS_CATEGORY_TALK    => 'Keskustelu_luokasta'
);

$skinNames = array(
	'standard'          => 'Perus',
	'cologneblue'       => 'Kölnin sininen',
	'myskin'            => 'Oma tyylisivu'
);

$datePreferences = array(
	'default',
	'fi normal',
	'fi seconds',
	'fi numeric',
	'ISO 8601',
);

$defaultDateFormat = 'fi normal';

$dateFormats = array(
	'fi normal time' => 'H.i',
	'fi normal date' => 'j. F"ta" Y',
	'fi normal both' => 'j. F"ta" Y "kello" H.i',

	'fi seconds time' => 'H:i:s',
	'fi seconds date' => 'j. F"ta" Y',
	'fi seconds both' => 'j. F"ta" Y "kello" H:i:s',

	'fi numeric time' => 'H.i',
	'fi numeric date' => 'j.n.Y',
	'fi numeric both' => 'j.n.Y "kello" H.i',
);

$datePreferenceMigrationMap = array(
	'default',
	'fi normal',
	'fi seconds',
	'fi numeric',
);

$bookstoreList = array(
	'Bookplus'                      => 'http://www.bookplus.fi/product.php?isbn=$1',
	'Helsingin yliopiston kirjasto' => 'http://pandora.lib.hel.fi/cgi-bin/mhask/monihask.py?volname=&author=&keyword=&ident=$1&submit=Hae&engine_helka=ON',
	'Pääkaupunkiseudun kirjastot'   => 'http://www.helmet.fi/search*fin/i?SEARCH=$1',
	'Tampereen seudun kirjastot'    => 'http://kirjasto.tampere.fi/Piki?formid=fullt&typ0=6&dat0=$1'
);

$magicWords = array(
	'redirect'            => array( 0, "#OHJAUS", "#UUDELLEENOHJAUS", "#REDIRECT" ),
	'toc'                 => array( 0, "__SISÄLLYSLUETTELO__", "__TOC__" ),
	'img_right'           => array( 1, "oikea", "right" ),
	'img_left'            => array( 1, "vasen", "left" ),
	'img_center'          => array( 1, "keskitetty", "center", "centre" ),
	'img_framed'          => array( 1, "kehys", "kehystetty", "framed", "enframed", "frame" ),
	'grammar'             => array( 0, "TAIVUTUS:", "GRAMMAR:" ),
	'plural'              => array( 0, "MONIKKO:", "PLURAL:" ),
);

$specialPageAliases = array(
	'DoubleRedirects'         => array( 'Kaksinkertaiset_ohjaukset', 'Kaksinkertaiset_uudelleenohjaukset' ),
	'BrokenRedirects'         => array( 'Virheelliset_ohjaukset', 'Virheelliset_uudelleenohjaukset' ),
	'Disambiguations'         => array( 'Täsmennyssivut' ),
	'Userlogin'               => array( 'Kirjaudu_sisään' ),
	'Userlogout'              => array( 'Kirjaudu_ulos' ),
	'CreateAccount'           => array( 'Luo_tunnus' ),
	'Preferences'             => array( 'Asetukset' ),
	'Watchlist'               => array( 'Tarkkailulista' ),
	'Recentchanges'           => array( 'Tuoreet_muutokset' ),
	'Upload'                  => array( 'Tallenna', 'Lisää_tiedosto' ),
	'Imagelist'               => array( 'Tiedostoluettelo' ),
	'Newimages'               => array( 'Uudet_tiedostot', 'Uudet_kuvat' ),
	'Listusers'               => array( 'Käyttäjät' ),
	'Listgrouprights'         => array( 'Käyttäjäryhmien oikeudet' ),
	'Statistics'              => array( 'Tilastot' ),
	'Randompage'              => array( 'Satunnainen_sivu' ),
	'Lonelypages'             => array( 'Yksinäiset_sivut' ),
	'Uncategorizedpages'      => array( 'Luokittelemattomat_sivut' ),
	'Uncategorizedcategories' => array( 'Luokittelemattomat_luokat' ),
	'Uncategorizedimages'     => array( 'Luokittelemattomat_tiedostot' ),
	'Uncategorizedtemplates'  => array( 'Luokittelemattomat_mallineet' ),
	'Unusedcategories'        => array( 'Käyttämättömät_luokat' ),
	'Unusedimages'            => array( 'Käyttämättömät_tiedostot' ),
	'Wantedpages'             => array( 'Halutuimmat_sivut' ),
	'Wantedcategories'        => array( 'Halutuimmat_luokat' ),
	'Missingfiles'            => array( 'Puuttuvat tiedostot' ),
	'Mostlinked'              => array( 'Viitatuimmat_sivut' ),
	'Mostlinkedcategories'    => array( 'Viitatuimmat_luokat' ),
	'Mostlinkedtemplates'     => array( 'Viitatuimmat_mallineet' ),
	'Mostcategories'          => array( 'Luokitelluimmat_sivut' ),
	'Mostimages'              => array( 'Viitatuimmat_tiedostot' ),
	'Mostrevisions'           => array( 'Muokatuimmat_sivut' ),
	'Fewestrevisions'         => array( 'Vähiten_muokatut_sivut' ),
	'Shortpages'              => array( 'Lyhyet_sivut' ),
	'Longpages'               => array( 'Pitkät_sivut' ),
	'Newpages'                => array( 'Uudet_sivut' ),
	'Ancientpages'            => array( 'Kuolleet_sivut' ),
	'Deadendpages'            => array( 'Linkittömät_sivut' ),
	'Protectedpages'          => array( 'Suojatut_sivut' ),
	'Protectedtitles'         => array( 'Suojatut_sivunimet' ),
	'Allpages'                => array( 'Kaikki_sivut' ),
	'Prefixindex'             => array( 'Etuliiteluettelo' ),
	'Ipblocklist'             => array( 'Muokkausestot' ),
	'Specialpages'            => array( 'Toimintosivut' ),
	'Contributions'           => array( 'Muokkaukset' ),
	'Emailuser'               => array( 'Lähetä_sähköpostia' ),
	'Confirmemail'            => array( 'Varmista_sähköpostiosoite' ),
	'Whatlinkshere'           => array( 'Tänne_viittaavat_sivut' ),
	'Recentchangeslinked'     => array( 'Linkitetyt_muutokset' ),
	'Movepage'                => array( 'Siirrä_sivu' ),
	'Blockme'                 => array( 'Estä_minut' ),
	'Booksources'             => array( 'Kirjalähteet' ),
	'Categories'              => array( 'Luokat' ),
	'Export'                  => array( 'Vie_sivuja' ),
	'Version'                 => array( 'Versio' ),
	'Allmessages'             => array( 'Järjestelmäviestit' ),
	'Log'                     => array( 'Loki', 'Lokit' ),
	'Blockip'                 => array( 'Estä' ),
	'Undelete'                => array( 'Palauta' ),
	'Import'                  => array( 'Tuo_sivuja' ),
	'Lockdb'                  => array( 'Lukitse_tietokanta' ),
	'Unlockdb'                => array( 'Avaa_tietokanta' ),
	'Userrights'              => array( 'Käyttöoikeudet' ),
	'MIMEsearch'              => array( 'MIME-haku' ),
	'FileDuplicateSearch'     => array( 'Kaksoiskappaleiden_haku' ),
	'Unwatchedpages'          => array( 'Tarkkailemattomat_sivut' ),
	'Listredirects'           => array( 'Ohjaussivut', 'Uudelleenohjaukset' ),
	'Revisiondelete'          => array( 'Poista_muokkaus' ),
	'Unusedtemplates'         => array( 'Käyttämättömät_mallineet' ),
	'Randomredirect'          => array( 'Satunnainen_ohjaus', 'Satunnainen_uudelleenohjaus' ),
	'Mypage'                  => array( 'Oma_sivu' ),
	'Mytalk'                  => array( 'Oma_keskustelu' ),
	'Mycontributions'         => array( 'Omat_muokkaukset' ),
	'Listadmins'              => array( 'Ylläpitäjät' ),
	'Listbots'                => array( 'Botit' ),
	'Popularpages'            => array( 'Suositut_sivut' ),
	'Search'                  => array( 'Haku' ),
	'Resetpass'               => array( 'Alusta_salasana' ),
	'Withoutinterwiki'        => array( 'Kielilinkittömät_sivut' ),
	'MergeHistory'            => array( 'Liitä_muutoshistoria' ),
	'Filepath'                => array( 'Tiedostopolku' ),
	'Invalidateemail'         => array( 'Hylkää sähköpostiosoite' ),
	'Blankpage'               => array( 'Tyhjä sivu' ),
);

$linkTrail = '/^([a-zäö]+)(.*)$/sDu';

$messages = array(
# User preference toggles
'tog-underline'               => 'Linkkien alleviivaus',
'tog-highlightbroken'         => 'Näytä linkit puuttuville sivuille <a href="#" class="new">näin</a> (vaihtoehtoisesti näin: <a href="#" class="internal">?</a>).',
'tog-justify'                 => 'Tasaa kappaleet',
'tog-hideminor'               => 'Piilota pienet muutokset tuoreet muutokset -listasta',
'tog-extendwatchlist'         => 'Laajenna tarkkailulista näyttämään kaikki tehdyt muutokset',
'tog-usenewrc'                => 'Kehittynyt tuoreet muutokset -listaus (JavaScript)',
'tog-numberheadings'          => 'Numeroi otsikot',
'tog-showtoolbar'             => 'Näytä työkalupalkki',
'tog-editondblclick'          => 'Muokkaa sivuja kaksoisnapsautuksella (JavaScript)',
'tog-editsection'             => 'Näytä muokkauslinkit jokaisen osion yläpuolella',
'tog-editsectiononrightclick' => 'Muokkaa osioita napsauttamalla otsikkoa hiiren oikealla painikkeella (JavaScript)',
'tog-showtoc'                 => 'Näytä sisällysluettelo sivuille, joilla yli 3 otsikkoa',
'tog-rememberpassword'        => 'Muista kirjautuminen eri istuntojen välillä',
'tog-editwidth'               => 'Muokkauskenttä on sivun levyinen',
'tog-watchcreations'          => 'Lisää luomani sivut tarkkailulistalle',
'tog-watchdefault'            => 'Lisää muokkaamani sivut tarkkailulistalle',
'tog-watchmoves'              => 'Lisää siirtämäni sivut tarkkailulistalle',
'tog-watchdeletion'           => 'Lisää poistamani sivut tarkkailulistalle',
'tog-minordefault'            => 'Muutokset ovat oletuksena pieniä',
'tog-previewontop'            => 'Näytä esikatselu muokkauskentän yläpuolella',
'tog-previewonfirst'          => 'Näytä esikatselu heti, kun muokkaus aloitetaan',
'tog-nocache'                 => 'Älä tallenna sivuja välimuistiin',
'tog-enotifwatchlistpages'    => 'Lähetä sähköpostiviesti tarkkailtujen sivujen muutoksista',
'tog-enotifusertalkpages'     => 'Lähetä sähköpostiviesti, kun käyttäjäsivun keskustelusivu muuttuu',
'tog-enotifminoredits'        => 'Lähetä sähköpostiviesti myös pienistä muokkauksista',
'tog-enotifrevealaddr'        => 'Näytä sähköpostiosoitteeni muille lähetetyissä ilmoituksissa',
'tog-shownumberswatching'     => 'Näytä sivua tarkkailevien käyttäjien määrä',
'tog-fancysig'                => 'Muotoilematon allekirjoitus ilman automaattista linkkiä',
'tog-externaleditor'          => 'Käytä ulkoista tekstieditoria oletuksena',
'tog-externaldiff'            => 'Käytä ulkoista diff-ohjelmaa oletuksena',
'tog-showjumplinks'           => 'Lisää loikkaa-käytettävyyslinkit sivun alkuun',
'tog-uselivepreview'          => 'Käytä pikaesikatselua (JavaScript) (kokeellinen)',
'tog-forceeditsummary'        => 'Huomauta, jos yhteenvetoa ei ole annettu',
'tog-watchlisthideown'        => 'Piilota omat muokkaukset',
'tog-watchlisthidebots'       => 'Piilota bottien muokkaukset',
'tog-watchlisthideminor'      => 'Piilota pienet muokkaukset',
'tog-nolangconversion'        => 'Älä tee muunnoksia kielivarianttien välillä',
'tog-ccmeonemails'            => 'Lähetä minulle kopio MediaWikin kautta lähetetyistä sähköposteista',
'tog-diffonly'                => 'Älä näytä sivun sisältöä versioita vertailtaessa',
'tog-showhiddencats'          => 'Näytä piilotetut luokat',

'underline-always'  => 'Aina',
'underline-never'   => 'Ei koskaan',
'underline-default' => 'Selaimen oletustapa',

'skinpreview' => '(esikatselu…)',

# Dates
'sunday'        => 'sunnuntai',
'monday'        => 'maanantai',
'tuesday'       => 'tiistai',
'wednesday'     => 'keskiviikko',
'thursday'      => 'torstai',
'friday'        => 'perjantai',
'saturday'      => 'lauantai',
'sun'           => 'su',
'mon'           => 'ma',
'tue'           => 'ti',
'wed'           => 'ke',
'thu'           => 'to',
'fri'           => 'pe',
'sat'           => 'la',
'january'       => 'tammikuu',
'february'      => 'helmikuu',
'march'         => 'maaliskuu',
'april'         => 'huhtikuu',
'may_long'      => 'toukokuu',
'june'          => 'kesäkuu',
'july'          => 'heinäkuu',
'august'        => 'elokuu',
'september'     => 'syyskuu',
'october'       => 'lokakuu',
'november'      => 'marraskuu',
'december'      => 'joulukuu',
'january-gen'   => 'tammikuun',
'february-gen'  => 'helmikuun',
'march-gen'     => 'maaliskuun',
'april-gen'     => 'huhtikuun',
'may-gen'       => 'toukokuun',
'june-gen'      => 'kesäkuun',
'july-gen'      => 'heinäkuun',
'august-gen'    => 'elokuun',
'september-gen' => 'syyskuun',
'october-gen'   => 'lokakuun',
'november-gen'  => 'marraskuun',
'december-gen'  => 'joulukuun',
'jan'           => 'tammikuu',
'feb'           => 'helmikuu',
'mar'           => 'maaliskuu',
'apr'           => 'huhtikuu',
'may'           => 'toukokuu',
'jun'           => 'kesäkuu',
'jul'           => 'heinäkuu',
'aug'           => 'elokuu',
'sep'           => 'syyskuu',
'oct'           => 'lokakuu',
'nov'           => 'marraskuu',
'dec'           => 'joulukuu',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|Luokka|Luokat}}',
'category_header'                => 'Sivut, jotka ovat luokassa $1',
'subcategories'                  => 'Alaluokat',
'category-media-header'          => 'Luokan ”$1” sisältämät tiedostot',
'category-empty'                 => "''Tässä luokassa ei ole sivuja eikä tiedostoja.''",
'hidden-categories'              => '{{PLURAL:$1|Piilotettu luokka|Piilotetut luokat}}',
'hidden-category-category'       => 'Piilotetut luokat', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|Tässä luokassa on vain seuraava alaluokka.|{{PLURAL:$1|Seuraava alaluokka kuuluu|Seuraavat $1 alaluokkaa kuuluvat}} tähän luokkaan. Alaluokkien kokonaismäärä luokassa on $2.}}',
'category-subcat-count-limited'  => 'Tässä luokassa on {{PLURAL:$1|yksi alaluokka|$1 alaluokkaa}}.',
'category-article-count'         => '{{PLURAL:$2|Tässä luokassa on vain seuraava sivu.|{{PLURAL:$1|Seuraava sivu kuuluu|Seuraavat $1 sivua kuuluvat}} tähän luokkaan. Sivujen kokonaismäärä luokassa on $2.}}',
'category-article-count-limited' => '{{PLURAL:$1|Tämä sivu kuuluu|Nämä $1 sivua kuuluvat}} nykyiseen luokkaan.',
'category-file-count'            => '{{PLURAL:$2|Tässä luokassa on vain seuraava tiedosto.|{{PLURAL:$1|Seuraava tiedosto kuuluu|Seuraavat $1 tiedostoa kuuluvat}} tähän luokkaan. Tiedostoja luokassa on yhteensä $2.}}',
'category-file-count-limited'    => 'Tässä luokassa on {{PLURAL:$1|yksi tiedosto|$1 tiedostoa}}.',
'listingcontinuesabbrev'         => 'jatkuu',

'mainpagetext'      => "'''MediaWiki on onnistuneesti asennettu.'''",
'mainpagedocfooter' => "Lisätietoja käytöstä on sivulla [http://meta.wikimedia.org/wiki/Help:Contents User's Guide].

=== Lisäohjeita ===

* [http://www.mediawiki.org/wiki/Manual:Configuration_settings Asetusten teko-ohjeita]
* [http://www.mediawiki.org/wiki/Manual:FAQ MediaWikin FAQ]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce Sähköpostilista, jolla tiedotetaan MediaWikin uusista versioista]

=== Asetukset ===

Tarkista, että alla olevat taivutusmuodot ovat oikein. Jos eivät, tee tarvittavat muutokset LocalSettings.php:hen seuraavasti:
 \$wgGrammarForms['fi']['genitive']['{{SITENAME}}'] = '...';
 \$wgGrammarForms['fi']['partitive']['{{SITENAME}}'] = '...';
 \$wgGrammarForms['fi']['elative']['{{SITENAME}}'] = '...';
 \$wgGrammarForms['fi']['inessive']['{{SITENAME}}'] = '...';
 \$wgGrammarForms['fi']['illative']['{{SITENAME}}'] = '...';
Taivutusmuodot: {{GRAMMAR:genitive|{{SITENAME}}}} (yön) — {{GRAMMAR:partitive|{{SITENAME}}}} (yötä) — {{GRAMMAR:elative|{{SITENAME}}}} (yöstä) — {{GRAMMAR:inessive|{{SITENAME}}}} (yössä) — {{GRAMMAR:illative|{{SITENAME}}}} (yöhön).",

'about'          => 'Tietoja',
'article'        => 'Sivu',
'newwindow'      => '(avautuu uuteen ikkunaan)',
'cancel'         => 'Keskeytä',
'qbfind'         => 'Etsi',
'qbbrowse'       => 'Selaa',
'qbedit'         => 'Muokkaa',
'qbpageoptions'  => 'Sivuasetukset',
'qbpageinfo'     => 'Sivun tiedot',
'qbmyoptions'    => 'Asetukset',
'qbspecialpages' => 'Toimintosivut',
'moredotdotdot'  => 'Lisää...',
'mypage'         => 'Käyttäjäsivu',
'mytalk'         => 'Keskustelusivu',
'anontalk'       => 'Keskustele tämän IP:n kanssa',
'navigation'     => 'Valikko',
'and'            => 'ja',

# Metadata in edit box
'metadata_help' => 'Sisältökuvaukset:',

'errorpagetitle'    => 'Virhe',
'returnto'          => 'Palaa sivulle $1.',
'tagline'           => '{{SITENAME}}',
'help'              => 'Ohje',
'search'            => 'Haku',
'searchbutton'      => 'Etsi',
'go'                => 'Siirry',
'searcharticle'     => 'Siirry',
'history'           => 'Historia',
'history_short'     => 'Historia',
'updatedmarker'     => 'päivitetty viimeisimmän käyntisi jälkeen',
'info_short'        => 'Tiedostus',
'printableversion'  => 'Tulostettava versio',
'permalink'         => 'Ikilinkki',
'print'             => 'Tulosta',
'edit'              => 'Muokkaa',
'create'            => 'Luo sivu',
'editthispage'      => 'Muokkaa tätä sivua',
'create-this-page'  => 'Luo tämä sivu',
'delete'            => 'Poista',
'deletethispage'    => 'Poista tämä sivu',
'undelete_short'    => 'Palauta {{PLURAL:$1|yksi muokkaus|$1 muokkausta}}',
'protect'           => 'Suojaa',
'protect_change'    => 'muuta',
'protectthispage'   => 'Suojaa tämä sivu',
'unprotect'         => 'Muuta suojausta',
'unprotectthispage' => 'Muuta tämän sivun suojauksia',
'newpage'           => 'Uusi sivu',
'talkpage'          => 'Keskustele tästä sivusta',
'talkpagelinktext'  => 'keskustelu',
'specialpage'       => 'Toimintosivu',
'personaltools'     => 'Henkilökohtaiset työkalut',
'postcomment'       => 'Kommentti sivun loppuun',
'articlepage'       => 'Näytä varsinainen sivu',
'talk'              => 'Keskustelu',
'views'             => 'Näkymät',
'toolbox'           => 'Työkalut',
'userpage'          => 'Näytä käyttäjäsivu',
'projectpage'       => 'Näytä projektisivu',
'imagepage'         => 'Näytä kuvasivu',
'mediawikipage'     => 'Näytä viestisivu',
'templatepage'      => 'Näytä mallinesivu',
'viewhelppage'      => 'Näytä ohjesivu',
'categorypage'      => 'Näytä luokkasivu',
'viewtalkpage'      => 'Näytä keskustelusivu',
'otherlanguages'    => 'Muilla kielillä',
'redirectedfrom'    => 'Ohjattu sivulta $1',
'redirectpagesub'   => 'Ohjaussivu',
'lastmodifiedat'    => 'Sivua on viimeksi muutettu $1 kello $2.', # $1 date, $2 time
'viewcount'         => 'Tämä sivu on näytetty {{PLURAL:$1|yhden kerran|$1 kertaa}}.',
'protectedpage'     => 'Suojattu sivu',
'jumpto'            => 'Loikkaa:',
'jumptonavigation'  => 'valikkoon',
'jumptosearch'      => 'hakuun',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'Tietoja {{GRAMMAR:elative|{{SITENAME}}}}',
'aboutpage'            => 'Project:Tietoja',
'bugreports'           => 'Ongelmat ja parannusehdotukset',
'bugreportspage'       => 'Project:Ongelmat ja parannusehdotukset',
'copyright'            => 'Sisältö on käytettävissä lisenssillä $1.',
'copyrightpagename'    => '{{SITENAME}} ja tekijänoikeudet',
'copyrightpage'        => '{{ns:project}}:Tekijänoikeudet',
'currentevents'        => 'Ajankohtaista',
'currentevents-url'    => 'Project:Ajankohtaista',
'disclaimers'          => 'Vastuuvapaus',
'disclaimerpage'       => 'Project:Vastuuvapaus',
'edithelp'             => 'Muokkausohjeet',
'edithelppage'         => 'Help:Kuinka sivuja muokataan',
'faq'                  => 'Usein kysytyt kysymykset',
'faqpage'              => 'Project:Usein kysytyt kysymykset',
'helppage'             => 'Help:Sisällys',
'mainpage'             => 'Etusivu',
'mainpage-description' => 'Etusivu',
'policy-url'           => 'Project:Käytännöt',
'portal'               => 'Kahvihuone',
'portal-url'           => 'Project:Kahvihuone',
'privacy'              => 'Tietosuojakäytäntö',
'privacypage'          => 'Project:Tietosuojakäytäntö',

'badaccess'        => 'Lupa evätty',
'badaccess-group0' => 'Sinulla ei ole lupaa suorittaa pyydettyä toimintoa.',
'badaccess-group1' => 'Pyytämäsi toiminto on rajoitettu henkilöille ryhmässä $1.',
'badaccess-group2' => 'Pyytämäsi toiminto on rajoitettu henkilöille ryhmissä $1.',
'badaccess-groups' => 'Pyytämäsi toiminto on rajoitettu ryhmien $1 henkilöille.',

'versionrequired'     => 'MediaWikistä tarvitaan vähintään versio $1',
'versionrequiredtext' => 'MediaWikistä tarvitaan vähintään versio $1 tämän sivun käyttämiseen. Katso [[Special:Version|versio]].',

'ok'                      => 'OK',
'pagetitle'               => '$1 – {{SITENAME}}',
'retrievedfrom'           => 'Haettu osoitteesta $1',
'youhavenewmessages'      => 'Sinulle on $1 ($2).',
'newmessageslink'         => 'uusia viestejä',
'newmessagesdifflink'     => 'viimeisin muutos',
'youhavenewmessagesmulti' => 'Sinulla on uusia viestejä sivuilla $1',
'editsection'             => 'muokkaa',
'editold'                 => 'muokkaa',
'viewsourceold'           => 'näytä lähdekoodi',
'editsectionhint'         => 'Muokkaa osiota $1',
'toc'                     => 'Sisällysluettelo',
'showtoc'                 => 'näytä',
'hidetoc'                 => 'piilota',
'thisisdeleted'           => 'Näytä tai palauta $1.',
'viewdeleted'             => 'Näytä $1?',
'restorelink'             => '{{PLURAL:$1|yksi poistettu muokkaus|$1 poistettua muokkausta}}',
'feedlinks'               => 'Uutissyötteet:',
'feed-invalid'            => 'Virheellinen syötetyyppi.',
'feed-unavailable'        => 'Verkkosyötteet eivät ole saatavilla.',
'site-rss-feed'           => '$1-RSS-syöte',
'site-atom-feed'          => '$1-Atom-syöte',
'page-rss-feed'           => '$1 (RSS-syöte)',
'page-atom-feed'          => '$1 (Atom-syöte)',
'red-link-title'          => '$1 (ei vielä kirjoitettu)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Sivu',
'nstab-user'      => 'Käyttäjäsivu',
'nstab-media'     => 'Media',
'nstab-special'   => 'Toiminto',
'nstab-project'   => 'Projektisivu',
'nstab-image'     => 'Tiedosto',
'nstab-mediawiki' => 'Järjestelmäviesti',
'nstab-template'  => 'Malline',
'nstab-help'      => 'Ohje',
'nstab-category'  => 'Luokka',

# Main script and global functions
'nosuchaction'      => 'Määrittelemätön pyyntö',
'nosuchactiontext'  => 'Wikiohjelmisto ei tunnista URL:ssä määriteltyä pyyntöä',
'nosuchspecialpage' => 'Kyseistä toimintosivua ei ole',
'nospecialpagetext' => 'Wikiohjelmisto ei tunnista pyytämääsi toimintosivua.',

# General errors
'error'                => 'Virhe',
'databaseerror'        => 'Tietokantavirhe',
'dberrortext'          => 'Tietokantakyselyssä oli syntaksivirhe. Syynä saattaa olla virheellinen kysely, tai se saattaa johtua ohjelmointivirheestä. Viimeinen tietokantakysely, jota yritettiin, oli: <blockquote><tt>$1</tt></blockquote>. Se tehtiin funktiosta ”<tt>$2</tt>”. MySQL palautti virheen ”<tt>$3: $4</tt>”.',
'dberrortextcl'        => 'Tietokantakyselyssä oli syntaksivirhe. Viimeinen tietokantakysely, jota yritettiin, oli: ”$1”. Se tehtiin funktiosta ”$2”. MySQL palautti virheen ”$3: $4”.',
'noconnect'            => 'Sivustolla on teknisiä ongelmia. Tietokantaan ei saada yhteyttä.<br />
$1',
'nodb'                 => 'Tietokantaa $1 ei voitu valita',
'cachederror'          => 'Pyydetystä sivusta näytettiin välimuistissa oleva kopio, ja se saattaa olla vanhentunut.',
'laggedslavemode'      => 'Varoitus: Sivu ei välttämättä sisällä viimeisimpiä muutoksia.',
'readonly'             => 'Tietokanta on lukittu',
'enterlockreason'      => 'Anna lukituksen syy sekä sen arvioitu poistamisaika',
'readonlytext'         => '{{GRAMMAR:genitive|{{SITENAME}}}} tietokanta on tällä hetkellä lukittu. Uusia sivuja ei voi luoda eikä muitakaan muutoksia tehdä. Syynä ovat todennäköisimmin rutiininomaiset tietokannan ylläpitotoimet. Tietokannan lukinneen ylläpitäjän selitys: $1',
'missing-article'      => 'Sivun sisältöä ei löytynyt tietokannasta: $1 $2.

Useimmiten tämä johtuu vanhentuneesta vertailu- tai historiasivulinkistä poistettuun sivuun.

Jos kyseessä ei ole poistettu sivu, olet ehkä löytänyt virheen ohjelmistossa.
Ilmoita tämän sivun osoite wikin [[Special:ListUsers/sysop|ylläpitäjälle]].',
'missingarticle-rev'   => '(versio: $1)',
'missingarticle-diff'  => '(vertailu: $1, $2)',
'readonly_lag'         => 'Tietokanta on automaattisesti lukittu, jotta kaikki tietokantapalvelimet saisivat kaikki tuoreet muutokset',
'internalerror'        => 'Sisäinen virhe',
'internalerror_info'   => 'Sisäinen virhe: $1',
'filecopyerror'        => 'Tiedostoa <b>$1</b> ei voitu kopioida tiedostoksi <b>$2</b>.',
'filerenameerror'      => 'Tiedostoa <b>$1</b> ei voitu nimetä uudelleen nimellä <b>$2</b>.',
'filedeleteerror'      => 'Tiedostoa <b>$1</b> ei voitu poistaa.',
'directorycreateerror' => 'Hakemiston ”$1” luominen epäonnistui.',
'filenotfound'         => 'Tiedostoa <b>$1</b> ei löytynyt.',
'fileexistserror'      => 'Tiedostoon ”$1” kirjoittaminen epäonnistui: tiedosto on olemassa',
'unexpected'           => 'Odottamaton arvo: ”$1” on ”$2”.',
'formerror'            => 'Lomakkeen tiedot eivät kelpaa',
'badarticleerror'      => 'Toimintoa ei voi suorittaa tälle sivulle.',
'cannotdelete'         => 'Sivun tai tiedoston poisto epäonnistui. Joku muu on saattanut poistaa sen.',
'badtitle'             => 'Virheellinen otsikko',
'badtitletext'         => 'Pyytämäsi sivuotsikko oli virheellinen, tyhjä tai väärin linkitetty kieltenvälinen tai wikienvälinen linkki.',
'perfdisabled'         => 'Pahoittelut! Tämä ominaisuus ei toistaiseksi ole käytettävissä, sillä se hidastaa tietokantaa niin paljon, että kukaan ei voi käyttää wikiä. Toiminto ohjelmoidaan tehokkaammaksi lähiaikoina. (Sinäkin voit tehdä sen! Tämä on vapaa ohjelmisto.)',
'perfcached'           => 'Tiedot ovat välimuistista eivätkä välttämättä ole ajan tasalla.',
'perfcachedts'         => 'Seuraava data on tuotu välimuistista ja se päivitettiin viimeksi $1.',
'querypage-no-updates' => 'Tämän sivun tietoja ei toistaiseksi päivitetä.',
'wrong_wfQuery_params' => 'Virheelliset parametrit wfQuery()<br />Funktio: $1<br />Tiedustelu: $2',
'viewsource'           => 'Lähdekoodi',
'viewsourcefor'        => 'sivulle $1',
'actionthrottled'      => 'Toiminto nopeusrajoitettu',
'actionthrottledtext'  => 'Ylläpitosyistä tämän toiminnon suorittamista on rajoitettu. Olet suorittanut tämän toiminnon liian monta kertaa lyhyen ajan sisällä. Yritä myöhemmin uudelleen.',
'protectedpagetext'    => 'Tämä sivu on suojattu muutoksilta.',
'viewsourcetext'       => 'Voit tarkastella ja kopioida tämän sivun lähdekoodia:',
'protectedinterface'   => 'Tämä sivu sisältää ohjelmiston käyttöliittymätekstiä ja on suojattu häiriköinnin estämiseksi.',
'editinginterface'     => '<center>Muokkaat sivua, joka sisältää ohjelmiston käyttöliittymätekstiä.</center>',
'sqlhidden'            => '(SQL-kysely piilotettu)',
'cascadeprotected'     => 'Tämä sivu on suojattu muokkauksilta, koska se on sisällytetty alla {{PLURAL:$1|olevaan laajennetusti suojattuun sivuun|oleviin laajennetusti suojattuihin sivuihin}}:
$2',
'namespaceprotected'   => "Et voi muokata sivuja nimiavaruudessa '''$1'''.",
'customcssjsprotected' => 'Sinulla ei ole oikeuksia muuttaa toisten käyttäjien henkilökohtaisia asetuksia.',
'ns-specialprotected'  => 'Toimintosivuja ei voi muokata.',
'titleprotected'       => "Käyttäjä [[User:$1|$1]] on asettanut tämän sivun luontikieltoon: ''$2''.",

# Virus scanner
'virus-badscanner'     => 'Virheellinen asetus: tuntematon virustutka: <i>$1</i>',
'virus-scanfailed'     => 'virustarkistus epäonnistui virhekoodilla $1',
'virus-unknownscanner' => 'tuntematon virustutka:',

# Login and logout pages
'logouttitle'                => 'Uloskirjautuminen',
'logouttext'                 => 'Olet nyt kirjautunut ulos {{GRAMMAR:elative|{{SITENAME}}}}. Voit jatkaa {{GRAMMAR:genitive|{{SITENAME}}}} käyttöä nimettömänä, tai kirjautua uudelleen sisään.',
'welcomecreation'            => '== Tervetuloa $1! ==
Käyttäjätunnuksesi on luotu.
Älä unohda virittää {{GRAMMAR:genitive|{{SITENAME}}}} [[Special:Preferences|asetuksiasi]].',
'loginpagetitle'             => 'Sisäänkirjautuminen',
'yourname'                   => 'Käyttäjätunnus',
'yourpassword'               => 'Salasana',
'yourpasswordagain'          => 'Salasana uudelleen',
'remembermypassword'         => 'Muista minut',
'yourdomainname'             => 'Verkkonimi',
'externaldberror'            => 'Tapahtui virhe ulkoisen autentikointitietokannan käytössä tai sinulla ei ole lupaa päivittää tunnustasi.',
'loginproblem'               => '<b>Sisäänkirjautuminen ei onnistunut.</b><br />Yritä uudelleen!',
'login'                      => 'Kirjaudu sisään',
'nav-login-createaccount'    => 'Kirjaudu sisään tai luo tunnus',
'loginprompt'                => 'Kirjautumiseen tarvitaan evästeitä.',
'userlogin'                  => 'Kirjaudu sisään tai luo tunnus',
'logout'                     => 'Kirjaudu ulos',
'userlogout'                 => 'Kirjaudu ulos',
'notloggedin'                => 'Et ole kirjautunut',
'nologin'                    => 'Jos sinulla ei ole vielä käyttäjätunnusta, $1.',
'nologinlink'                => 'voit luoda sellaisen',
'createaccount'              => 'Luo uusi käyttäjätunnus',
'gotaccount'                 => 'Jos sinulla on jo tunnus, voit $1.',
'gotaccountlink'             => 'kirjautua sisään',
'createaccountmail'          => 'sähköpostitse',
'badretype'                  => 'Syöttämäsi salasanat ovat erilaiset.',
'userexists'                 => 'Pyytämäsi käyttäjänimi on jo käytössä. Valitse toinen käyttäjänimi.',
'youremail'                  => 'Sähköpostiosoite',
'username'                   => 'Tunnus',
'uid'                        => 'Numero',
'prefs-memberingroups'       => 'Jäsenenä {{PLURAL:$1|ryhmässä|ryhmissä}}',
'yourrealname'               => 'Oikea nimi',
'yourlanguage'               => 'Käyttöliittymän kieli',
'yourvariant'                => 'Kielivariantti',
'yournick'                   => 'Allekirjoitus',
'badsig'                     => 'Allekirjoitus ei kelpaa.',
'badsiglength'               => 'Allekirjoitus on liian pitkä – sen on oltava alle $1 {{PLURAL:$1|merkki|merkkiä}}.',
'email'                      => 'Sähköpostitoiminnot',
'prefs-help-realname'        => 'Vapaaehtoinen. Nimesi näytetään käyttäjätunnuksesi sijasta sivun tekijäluettelossa.',
'loginerror'                 => 'Sisäänkirjautumisvirhe',
'prefs-help-email'           => 'Vapaaehtoinen. Mahdollistaa uuden salasanan pyytämisen, jos unohdat salasanasi. Voit myös sallia muiden käyttäjien ottaa sinuun yhteyttä sähköpostilla ilman, että osoitteesi paljastuu.',
'prefs-help-email-required'  => 'Sähköpostiosoite on pakollinen.',
'nocookiesnew'               => 'Käyttäjä luotiin, mutta et ole kirjautunut sisään. {{SITENAME}} käyttää evästeitä sisäänkirjautumisen yhteydessä. Selaimesi ei salli evästeistä. Kytke ne päälle, ja sitten kirjaudu sisään juuri luomallasi käyttäjänimellä ja salasanalla.',
'nocookieslogin'             => '{{SITENAME}} käyttää evästeitä sisäänkirjautumisen yhteydessä. Selaimesi ei salli evästeitä. Ota ne käyttöön, ja yritä uudelleen.',
'noname'                     => 'Et ole määritellyt kelvollista käyttäjänimeä.',
'loginsuccesstitle'          => 'Sisäänkirjautuminen onnistui',
'loginsuccess'               => 'Olet kirjautunut käyttäjänä $1.',
'nosuchuser'                 => 'Käyttäjää ”$1” ei ole olemassa. Tarkista kirjoititko nimen oikein, tai [[Special:Userlogin/signup|luo uusi käyttäjätunnus]].',
'nosuchusershort'            => 'Käyttäjää nimeltä ”<nowiki>$1</nowiki>” ei ole. Kirjoititko nimen oikein?',
'nouserspecified'            => 'Käyttäjätunnusta ei ole määritelty.',
'wrongpassword'              => 'Syöttämäsi salasana ei ole oikein. Ole hyvä ja yritä uudelleen.',
'wrongpasswordempty'         => 'Et voi antaa tyhjää salasanaa.',
'passwordtooshort'           => 'Salasanasi on ei kelpaa. Salasanan pitää olla vähintään {{PLURAL:$1|yhden merkin pituinen|$1 merkkiä pitkä}} ja eri kuin käyttäjätunnuksesi.',
'mailmypassword'             => 'Lähetä uusi salasana sähköpostitse',
'passwordremindertitle'      => 'Salasanamuistutus {{GRAMMAR:elative|{{SITENAME}}}}',
'passwordremindertext'       => 'Joku IP-osoitteesta $1 pyysi {{GRAMMAR:partitive|{{SITENAME}}}} ($4) lähettämään uuden salasanan. Salasana käyttäjälle $2 on nyt $3. Kirjaudu sisään ja vaihda salasana.',
'noemail'                    => "Käyttäjälle '''$1''' ei ole määritelty sähköpostiosoitetta.",
'passwordsent'               => 'Uusi salasana on lähetetty käyttäjän <b>$1</b> sähköpostiosoitteeseen.',
'blocked-mailpassword'       => 'Osoitteellesi on asetettu muokkausesto, joka estää käyttämästä salasanamuistutustoimintoa.',
'eauthentsent'               => 'Varmennussähköposti on lähetetty annettuun sähköpostiosoitteeseen. Muita viestejä ei lähetetä, ennen kuin olet toiminut viestin ohjeiden mukaan ja varmistanut, että sähköpostiosoite kuuluu sinulle.',
'throttled-mailpassword'     => 'Salasanamuistutus on lähetetty {{PLURAL:$1|kuluvan|kuluvien $1}} tunnin aikana. Salasanamuistutuksia lähetään enintään {{PLURAL:$1|tunnin|$1 tunnin}} välein.',
'mailerror'                  => 'Virhe lähetettäessä sähköpostia: $1',
'acct_creation_throttle_hit' => 'Olet jo luonut $1 tunnusta. Et voi luoda uutta.',
'emailauthenticated'         => 'Sähköpostiosoitteesi varmennettiin $1.',
'emailnotauthenticated'      => 'Sähköpostiosoitettasi ei ole vielä varmennettu. Sähköpostia ei lähetetä liittyen alla oleviin toimintoihin.',
'noemailprefs'               => 'Sähköpostiosoitetta ei ole määritelty.',
'emailconfirmlink'           => 'Varmenna sähköpostiosoite',
'invalidemailaddress'        => 'Sähköpostiosoitetta ei voida hyväksyä, koska se ei ole oikeassa muodossa. Ole hyvä ja anna oikea sähköpostiosoite tai jätä kenttä tyhjäksi.',
'accountcreated'             => 'Käyttäjätunnus luotiin',
'accountcreatedtext'         => 'Käyttäjän $1 käyttäjätunnus luotiin.',
'createaccount-title'        => 'Tunnuksen luominen {{GRAMMAR:illative|{{SITENAME}}}}',
'createaccount-text'         => 'Joku on luonut tunnuksen $2 {{GRAMMAR:illative|{{SITENAME}}}} ($4).
Tunnuksen $2 salasana on » $3 ». Kirjaudu sisään ja vaihda salasanasi.

Sinun ei tarvitse huomioida tätä viestiä, jos tunnus on luotu virheellisesti.',
'loginlanguagelabel'         => 'Kieli: $1',

# Password reset dialog
'resetpass'               => 'Salasanan alustus',
'resetpass_announce'      => 'Kirjauduit sisään sähköpostitse lähetetyllä väliaikaissalasanalla. Päätä sisäänkirjautuminen asettamalla uusi salasana.',
'resetpass_text'          => '<!-- Lisää tekstiä tähän -->',
'resetpass_header'        => 'Uuden salasanan asettaminen',
'resetpass_submit'        => 'Aseta salasana ja kirjaudu sisään',
'resetpass_success'       => 'Salasanan vaihto onnistui.',
'resetpass_bad_temporary' => 'Kelvoton väliaikaissalasana. Olet saattanut jo asettaa uuden salasanan tai pyytänyt uutta väliaikaissalasanaa.',
'resetpass_forbidden'     => 'Salasanoja ei voi vaihtaa.',
'resetpass_missing'       => 'Ei syötettä.',

# Edit page toolbar
'bold_sample'     => 'Lihavoitu teksti',
'bold_tip'        => 'Lihavointi',
'italic_sample'   => 'Kursivoitu teksti',
'italic_tip'      => 'Kursivointi',
'link_sample'     => 'linkki',
'link_tip'        => 'Sisäinen linkki',
'extlink_sample'  => 'http://www.example.com linkin otsikko',
'extlink_tip'     => 'Ulkoinen linkki (muista http:// edessä)',
'headline_sample' => 'Otsikkoteksti',
'headline_tip'    => 'Otsikko',
'math_sample'     => 'Lisää kaava tähän',
'math_tip'        => 'Matemaattinen kaava (LaTeX)',
'nowiki_sample'   => 'Lisää muotoilematon teksti tähän',
'nowiki_tip'      => 'Tekstiä, jota wiki ei muotoile',
'image_sample'    => 'Esimerkki.jpg',
'image_tip'       => 'Tallennettu kuva',
'media_sample'    => 'Esimerkki.ogg',
'media_tip'       => 'Mediatiedostolinkki',
'sig_tip'         => 'Allekirjoitus aikamerkinnällä',
'hr_tip'          => 'Vaakasuora viiva',

# Edit pages
'summary'                          => 'Yhteenveto',
'subject'                          => 'Aihe',
'minoredit'                        => 'Tämä on pieni muutos',
'watchthis'                        => 'Lisää tarkkailulistaan',
'savearticle'                      => 'Tallenna sivu',
'preview'                          => 'Esikatselu',
'showpreview'                      => 'Esikatsele',
'showlivepreview'                  => 'Pikaesikatselu',
'showdiff'                         => 'Näytä muutokset',
'anoneditwarning'                  => 'Et ole kirjautunut sisään. IP-osoitteesi kirjataan tämän sivun muokkaushistoriaan.',
'missingsummary'                   => 'Et ole antanut yhteenvetoa. Jos valitset Tallenna uudelleen, niin muokkauksesi tallennetaan ilman yhteenvetoa.',
'missingcommenttext'               => 'Anna yhteenveto alle.',
'missingcommentheader'             => 'Et ole antanut otsikkoa kommentillesi. Valitse <em>Tallenna</em>, jos et halua antaa otsikkoa.',
'summary-preview'                  => 'Yhteenvedon esikatselu',
'subject-preview'                  => 'Otsikon esikatselu',
'blockedtitle'                     => 'Pääsy estetty',
'blockedtext'                      => "<big>'''Käyttäjätunnuksesi tai IP-osoitteesi on estetty.'''</big>

Eston on asettanut $1.
Syy: '''$2'''

* Eston alkamisaika: $8
* Eston päättymisaika: $6
* Kohde: $7

Voit keskustella ylläpitäjän $1 tai toisen [[{{MediaWiki:Grouppage-sysop}}|ylläpitäjän]] kanssa estosta.
Huomaa, ettet voi lähettää sähköpostia {{GRAMMAR:genitive|{{SITENAME}}}} kautta, ellet ole asettanut olemassa olevaa sähköpostiosoitetta [[Special:Preferences|asetuksissa]] tai jos esto on asetettu koskemaan myös sähköpostin lähettämistä.
IP-osoitteesi on $3 ja estotunnus on #$5.
Liitä kaikki ylläolevat tiedot mahdollisiin kyselyihisi.",
'autoblockedtext'                  => "IP-osoitteesi on estetty automaattisesti, koska sitä on käyttänyt toinen käyttäjä, jonka on estänyt ylläpitäjä $1.
Eston syy on:

:''$2''

* Eston alkamisaika: $8
* Eston päättymisaika: $6
* Kohde: $7

Voit keskustella ylläpitäjän $1 tai toisen [[{{MediaWiki:Grouppage-sysop}}|ylläpitäjän]] kanssa estosta.

Huomaa, ettet voi lähettää sähköpostia {{GRAMMAR:genitive|{{SITENAME}}}} kautta, ellet ole asettanut olemassa olevaa sähköpostiosoitetta [[Special:Preferences|asetuksissa]] tai jos esto on asetettu koskemaan myös sähköpostin lähettämistä.

IP-osoitteesi on $3 ja estotunnus on #$5.
Liitä kaikki ylläolevat tiedot mahdollisiin kyselyihisi.",
'blockednoreason'                  => '(syytä ei annettu)',
'blockedoriginalsource'            => 'Sivun ”$1” lähdekoodi:',
'blockededitsource'                => 'Muokkauksesi sivuun ”$1”:',
'whitelistedittitle'               => 'Sisäänkirjautuminen vaaditaan muokkaamiseen',
'whitelistedittext'                => 'Sinun täytyy $1, jotta voisit muokata sivuja.',
'confirmedittitle'                 => 'Sähköpostin varmennus',
'confirmedittext'                  => 'Et voi muokata sivuja, ennen kuin olet varmentanut sähköpostiosoitteesi. Voit tehdä varmennuksen [[Special:Preferences|asetussivulla]].',
'nosuchsectiontitle'               => 'Pyydettyä osiota ei ole',
'nosuchsectiontext'                => 'Yritit muokata osiota, jota ei ole olemassa. Koska osiota $1 ei ole olemassa, muokkausta ei voida tallentaa.',
'loginreqtitle'                    => 'Sisäänkirjautuminen vaaditaan',
'loginreqlink'                     => 'kirjautua sisään',
'loginreqpagetext'                 => 'Sinun täytyy $1, jotta voisit nähdä muut sivut.',
'accmailtitle'                     => 'Salasana lähetetty.',
'accmailtext'                      => "käyttäjän '''$1''' salasana on lähetetty osoitteeseen '''$2'''.",
'newarticle'                       => '(uusi)',
'newarticletext'                   => 'Linkki toi sivulle, jota ei vielä ole. Voit luoda sivun kirjoittamalla alla olevaan tilaan. Jos et halua luoda sivua, käytä selaimen paluutoimintoa.',
'anontalkpagetext'                 => "----''Tämä on nimettömän käyttäjän keskustelusivu. Hän ei ole joko luonut itselleen käyttäjätunnusta tai ei käytä sitä. Siksi hänet tunnistetaan nyt numeerisella IP-osoitteella. Kyseinen IP-osoite voi olla useamman henkilön käytössä. Jos olet nimetön käyttäjä, ja sinusta tuntuu, että aiheettomia kommentteja on ohjattu sinulle, [[Special:UserLogin|luo itsellesi käyttäjätunnus tai kirjaudu sisään]] välttääksesi jatkossa sekaannukset muiden nimettömien käyttäjien kanssa.''",
'noarticletext'                    => "{{GRAMMAR:inessive|{{SITENAME}}}} ei ole tämän nimistä sivua.
* Voit [[Special:Search/{{PAGENAME}}|etsiä sivun nimellä]] muilta sivuilta.
* Voit kirjoittaa uuden sivun '''<span class=\"plainlinks\">[{{fullurl:{{FULLPAGENAME}}|action=edit}} {{PAGENAME}}]</span>.'''",
'userpage-userdoesnotexist'        => 'Käyttäjätunnusta $1 ei ole rekisteröity. Varmista haluatko muokata tätä sivua.',
'clearyourcache'                   => "'''Huomautus:''' Selaimen välimuisti pitää tyhjentää asetusten tallentamisen jälkeen, jotta muutokset tulisivat voimaan:
*'''Mozilla, Konqueror ja Safari:''' napsauta ''Shift''-näppäin pohjassa päivitä tai paina ''Ctrl-Shift-R'' (''Cmd-Shift-R'' Applella)
*'''IE:''' napsauta ''Ctrl''-näppäin pohjassa päivitä tai paina ''Ctrl-F5''
*'''Konqueror''': napsauta päivitä tai paina ''F5''
*'''Opera:''' saatat joutua tyhjentämään välimuistin kokonaan (''Tools→Preferences'').",
'usercssjsyoucanpreview'           => 'Voit testata uutta CSS:ää tai JavaScriptiä ennen tallennusta esikatselulla.',
'usercsspreview'                   => "'''Tämä on CSS:n esikatselu. Mitään muutoksia ei ole vielä tallennettu.'''",
'userjspreview'                    => "'''Tämä on JavaScriptin esikatselu.'''",
'userinvalidcssjstitle'            => "'''Varoitus:''' Tyyliä nimeltä ”$1” ei ole olemassa. Muista, että käyttäjän määrittelemät .css- ja .js-sivut alkavat pienellä alkukirjaimella, esim. {{ns:user}}:Matti Meikäläinen/monobook.css eikä {{ns:user}}:Matti Meikäläinen/Monobook.css.",
'updated'                          => '(Päivitetty)',
'note'                             => '<strong>Huomautus:</strong>',
'previewnote'                      => '<strong>Tämä on vasta sivun esikatselu. Sivua ei ole vielä tallennettu!</strong>',
'previewconflict'                  => 'Tämä esikatselu näyttää miltä muokkausalueella oleva teksti näyttää tallennettuna.',
'session_fail_preview'             => '<strong>Muokkaustasi ei voitu tallentaa, koska istuntosi tiedot ovat kadonneet. Yritä uudelleen. Jos ongelma ei katoa, yritä [[Special:UserLogout|kirjautua ulos]] ja takaisin sisään.</strong>',
'session_fail_preview_html'        => '<strong>Muokkaustasi ei voitu tallentaa, koska istuntosi tiedot ovat kadonneet.</strong>

Esikatselu on piilotettu varokeinona JavaScript-hyökkäyksiä vastaan – tässä wikissä on HTML-tila päällä.

Yritä uudelleen. Jos ongelma ei katoa, yritä [[Special:UserLogout|kirjautua ulos]] ja takaisin sisään.',
'token_suffix_mismatch'            => '<strong>Muokkauksesi on hylätty, koska asiakasohjelmasi ei osaa käsitellä välimerkkejä muokkaustarkisteessa. Syynä voi olla viallinen välityspalvelin.</strong>',
'editing'                          => 'Muokataan sivua $1',
'editingsection'                   => 'Muokataan osiota sivusta $1',
'editingcomment'                   => 'Muokataan kommenttia sivulla $1',
'editconflict'                     => 'Päällekkäinen muokkaus: $1',
'explainconflict'                  => "Joku muu on muuttanut tätä sivua sen jälkeen, kun aloit muokata sitä. Ylempi tekstialue sisältää tämänhetkisen tekstin. Tekemäsi muutokset näkyvät alemmassa ikkunassa. Sinun täytyy yhdistää muutoksesi olemassa olevaan tekstiin. '''Vain''' ylemmässä alueessa oleva teksti tallentuu, kun tallennat sivun.",
'yourtext'                         => 'Oma tekstisi',
'storedversion'                    => 'Tallennettu versio',
'nonunicodebrowser'                => "'''Selaimesi ei ole Unicode-yhteensopiva. Ole hyvä ja vaihda selainta, ennen kuin muokkaat sivua.'''",
'editingold'                       => '<strong>Varoitus: Olet muokkaamassa vanhaa versiota tämän sivun tekstistä. Jos tallennat sen, kaikki tämän version jälkeen tehdyt muutokset katoavat.</strong>',
'yourdiff'                         => 'Eroavaisuudet',
'copyrightwarning'                 => '<strong>Muutoksesi astuvat voimaan välittömästi.</strong> Kaikki {{GRAMMAR:illative|{{SITENAME}}}} tehtävät tuotokset katsotaan julkaistuksi $2 -lisenssin mukaisesti ($1). Jos et halua, että kirjoitustasi muokataan armottomasti ja uudelleenkäytetään vapaasti, älä tallenna kirjoitustasi. Tallentamalla muutoksesi lupaat, että kirjoitit tekstisi itse, tai kopioit sen jostain vapaasta lähteestä. <strong>ÄLÄ KÄYTÄ TEKIJÄNOIKEUDEN ALAISTA MATERIAALIA ILMAN LUPAA!</strong>',
'copyrightwarning2'                => 'Huomaa, että kuka tahansa voi muokata, muuttaa ja poistaa kaikkia sivustolle tekemiäsi lisäyksiä ja muutoksia. Muokkaamalla sivustoa luovutat sivuston käyttäjille tämän oikeuden ja takaat, että lisäämäsi aineisto on joko itse kirjoittamaasi tai peräisin jostain vapaasta lähteestä. Lisätietoja sivulla $1. <strong>TEKIJÄNOIKEUDEN ALAISEN MATERIAALIN KÄYTTÄMINEN ILMAN LUPAA ON EHDOTTOMASTI KIELLETTYÄ!</strong>',
'longpagewarning'                  => '<center>Tämän sivun tekstiosuus on $1 binäärikilotavua pitkä. Harkitse, voisiko sivun jakaa pienempiin osiin.</center>',
'longpageerror'                    => '<strong>Sivun koko on $1 binäärikilotavua. Sivua ei voida tallentaa, koska enimmäiskoko on $2 binäärikilotavua.</strong>',
'readonlywarning'                  => '<strong>Varoitus</strong>: Tietokanta on lukittu huoltoa varten, joten voi olla ettet pysty tallentamaan muokkauksiasi juuri nyt. Saattaa olla paras leikata ja liimata tekstisi omaan tekstitiedostoosi ja tallentaa se tänne myöhemmin.',
'protectedpagewarning'             => '<center><small>Tämä sivu on lukittu. Vain ylläpitäjät voivat muokata sitä.</small></center>',
'semiprotectedpagewarning'         => 'Vain rekisteröityneet käyttäjät voivat muokata tätä sivua.',
'cascadeprotectedwarning'          => '<strong>Vain ylläpitäjät voivat muokata tätä sivua, koska se on sisällytetty alla {{PLURAL:$1|olevaan laajennetusti suojattuun sivuun|oleviin laajennetusti suojattuihin sivuihin}}</strong>:',
'titleprotectedwarning'            => '<strong>Tämä sivun luominen on rajoitettu vain osalle käyttäjistä.</strong>',
'templatesused'                    => 'Tällä sivulla käytetyt mallineet:',
'templatesusedpreview'             => 'Esikatselussa mukana olevat mallineet:',
'templatesusedsection'             => 'Tässä osiossa mukana olevat mallineet:',
'template-protected'               => '(suojattu)',
'template-semiprotected'           => '(suojattu anonyymeiltä ja uusilta käyttäjiltä)',
'hiddencategories'                 => 'Tämä sivu kuuluu {{PLURAL:$1|seuraavaan piilotettuun luokkaan|seuraaviin piilotettuihin luokkiin}}:',
'edittools'                        => '<!-- Tässä oleva teksti näytetään muokkauskentän alla. -->',
'nocreatetitle'                    => 'Sivujen luominen on rajoitettu',
'nocreatetext'                     => 'Et voi luoda uusia sivuja. Voit muokata olemassa olevia sivuja tai luoda [[Special:UserLogin|käyttäjätunnuksen]].',
'nocreate-loggedin'                => 'Sinulla ei ole oikeuksia luoda uusia sivuja.',
'permissionserrors'                => 'Puutteelliset oikeudet',
'permissionserrorstext'            => 'Sinulla ei ole oikeuksia suorittaa toimintoa {{PLURAL:$1|seuraavasta|seuraavista}} syistä johtuen:',
'permissionserrorstext-withaction' => 'Sinulla ei ole lupaa {{lcfirst:$2}} {{PLURAL:$1|seuraavasta|seuraavista}} syistä johtuen:',
'recreate-deleted-warn'            => "'''Olet luomassa sivua, joka on aikaisemmin poistettu.'''

Harkitse, kannattaako sivua luoda uudelleen. Alla on tämän sivun poistohistoria:",

# Parser/template warnings
'expensive-parserfunction-warning'        => 'Tällä sivulla on liian monta hitaiden laajennusfunktioiden kutsua.
Kutsuja pitäisi olla vähemmän kuin $2, mutta nyt niitä on $1.',
'expensive-parserfunction-category'       => 'Liiaksi hitaita jäsentimen laajennusfunktioita käyttävät sivut',
'post-expand-template-inclusion-warning'  => 'Varoitus: Sisällytettyjen mallineiden koko on liian suuri.
Joitakin mallineita ei ole sisällytetty.',
'post-expand-template-inclusion-category' => 'Mallineiden sisällytyksen kokorajan ylittävät sivut',
'post-expand-template-argument-warning'   => 'Varoitus: Tällä sivulla on ainakin yksi mallineen muuttuja, jonka sisällytetty koko on liian suuri.
Nämä muuttujat on jätetty käsittelemättä.',
'post-expand-template-argument-category'  => 'Käsittelemättömiä mallinemuuttujia sisältävät sivut',

# "Undo" feature
'undo-success' => 'Kumoaminen onnistui. Valitse <em>tallenna</em> toteuttaaksesi muutokset.',
'undo-failure' => 'Muokkausta ei voitu kumota välissä olevien ristiriistaisten muutosten vuoksi. Kumoa muutokset käsin.',
'undo-norev'   => 'Muokkausta ei voitu perua, koska sitä ei ole olemassa tai se on poistettu.',
'undo-summary' => 'Kumottu muokkaus #$1, jonka teki [[Special:Contributions/$2|$2]] ([[User talk:$2|keskustelu]])',

# Account creation failure
'cantcreateaccounttitle' => 'Tunnuksen luominen epäonnistui',
'cantcreateaccount-text' => "Käyttäjä [[User:$3|$3]] on estänyt käyttäjätunnusten luomisen tästä IP-osoitteesta ($1).

Käyttäjän $3 antama syy on ''$2''",

# History pages
'viewpagelogs'        => 'Näytä tämän sivun lokit',
'nohistory'           => 'Tällä sivulla ei ole muutoshistoriaa.',
'revnotfound'         => 'Versiota ei löydy',
'revnotfoundtext'     => 'Pyytämääsi versiota ei löydy. Tarkista URL-osoite, jolla hait tätä sivua.',
'currentrev'          => 'Nykyinen versio',
'revisionasof'        => 'Versio $1',
'revision-info'       => 'Versio hetkellä $1 – tehnyt $2',
'previousrevision'    => '← Vanhempi versio',
'nextrevision'        => 'Uudempi versio →',
'currentrevisionlink' => 'Nykyinen versio',
'cur'                 => 'nyk.',
'next'                => 'seur.',
'last'                => 'edell.',
'page_first'          => 'ensimmäinen sivu',
'page_last'           => 'viimeinen sivu',
'histlegend'          => 'Merkinnät: (nyk.) = eroavaisuudet nykyiseen versioon, (edell.) = eroavaisuudet edelliseen versioon, <span class="minor">p</span> = pieni muutos',
'deletedrev'          => '[poistettu]',
'histfirst'           => 'Ensimmäiset',
'histlast'            => 'Viimeisimmät',
'historysize'         => '({{PLURAL:$1|1 tavu|$1 tavua}})',
'historyempty'        => '(tyhjä)',

# Revision feed
'history-feed-title'          => 'Muutoshistoria',
'history-feed-description'    => 'Tämän sivun muutoshistoria',
'history-feed-item-nocomment' => '$1 ($2)', # user at time
'history-feed-empty'          => 'Pyydettyä sivua ei ole olemassa.
Se on saatettu poistaa wikistä tai nimetä uudelleen.
Kokeile [[Special:Search|hakua]] löytääksesi asiaan liittyviä sivuja.',

# Revision deletion
'rev-deleted-comment'         => '(kommentti poistettu)',
'rev-deleted-user'            => '(käyttäjänimi poistettu)',
'rev-deleted-event'           => '(lokitapahtuma poistettu)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">Tämä versio on poistettu julkisesta arkistosta. [{{fullurl:Special:Log/delete|page={{PAGENAMEE}}}} Poistolokissa] saattaa olla lisätietoja.</div>',
'rev-deleted-text-view'       => '<div class="mw-warning plainlinks">Tämä versio on poistettu julkisesta arkistosta.</div>',
'rev-delundel'                => 'näytä tai piilota',
'revisiondelete'              => 'Poista tai palauta versioita',
'revdelete-nooldid-title'     => 'Ei kohdeversiota',
'revdelete-nooldid-text'      => 'Et ole valinnut kohdeversiota tai -versioita.',
'revdelete-selected'          => "{{PLURAL:$2|Valittu versio|Valitut versiot}} sivusta '''$1:'''",
'logdelete-selected'          => '{{PLURAL:$1|Valittu lokimerkintä|Valitut lokimerkinnät}}:',
'revdelete-text'              => 'Poistetut versiot näkyvät sivun historiassa, mutta niiden sisältö ei ole julkisesti saatavilla.

Muut ylläpitäjät voivat lukea piilotetun sisällön ja palauttaa sen.',
'revdelete-legend'            => 'Version rajoitukset',
'revdelete-hide-text'         => 'Piilota version sisältö',
'revdelete-hide-name'         => 'Piilota toiminto ja kohde',
'revdelete-hide-comment'      => 'Piilota yhteenveto',
'revdelete-hide-user'         => 'Piilota tekijän tunnus tai IP-osoite',
'revdelete-hide-restricted'   => 'Käytä näitä rajoituksia myös ylläpitäjiin ja lukitse tämä käyttöliittymä',
'revdelete-suppress'          => 'Piilota myös ylläpitäjiltä',
'revdelete-hide-image'        => 'Piilota tiedoston sisältö',
'revdelete-unsuppress'        => 'Poista rajoitukset palautetuilta versiolta',
'revdelete-log'               => 'Lokimerkintä',
'revdelete-submit'            => 'Toteuta',
'revdelete-logentry'          => 'muutti sivun [[$1]] version näkyvyyttä',
'logdelete-logentry'          => 'muutti sivun [[$1]] näkyvyyttä',
'revdelete-success'           => 'Version näkyvyys asetettu.',
'logdelete-success'           => 'Tapahtuman näkyvyys asetettu.',
'revdel-restore'              => 'Muuta näkyvyyttä',
'pagehist'                    => 'Muutoshistoria',
'deletedhist'                 => 'Poistettu muutoshistoria',
'revdelete-content'           => 'sisältö',
'revdelete-summary'           => 'yhteenveto',
'revdelete-uname'             => 'käyttäjänimi',
'revdelete-restricted'        => 'asetti rajoitukset ylläpitäjille',
'revdelete-unrestricted'      => 'poisti rajoitukset ylläpitäjiltä',
'revdelete-hid'               => 'piilotti $1',
'revdelete-unhid'             => 'palautti näkyviin $1',
'revdelete-log-message'       => '$1 koskien $2 {{PLURAL:$2|versiota}}',
'logdelete-log-message'       => '$1 koskien $2 {{PLURAL:$2|tapahtumaa}}',

# Suppression log
'suppressionlog'     => 'Häivitysloki',
'suppressionlogtext' => 'Alla on lista uusimmista poistoista ja muokkausestoista, jotka sisältävät ylläpitäjiltä piilotettua materiaalia.
[[Special:IPBlockList|Muokkausestolistassa]] on tämänhetkiset muokkausestot.',

# History merging
'mergehistory'                     => 'Yhdistä sivuhistoriat',
'mergehistory-header'              => 'Tämä sivu mahdollistaa sivun muutoshistorian yhdistämisen uudemman sivun muutoshistoriaan.
Uuden ja vanhan sivun muutoksien pitää muodostaa jatkumo – ne eivät saa mennä ristikkäin.',
'mergehistory-box'                 => 'Yhdistä kahden sivun muutoshistoria',
'mergehistory-from'                => 'Lähdesivu',
'mergehistory-into'                => 'Kohdesivu',
'mergehistory-list'                => 'Liitettävissä olevat muutokset',
'mergehistory-merge'               => 'Seuraavat sivun [[:$1]] muutokset voidaan liittää sivun [[:$2]] muutoshistoriaan. Voit valita version, jota myöhempiä muutoksia ei liitetä. Selainlinkkien käyttäminen kadottaa tämän valinnan.',
'mergehistory-go'                  => 'Etsi muutokset',
'mergehistory-submit'              => 'Yhdistä versiot',
'mergehistory-empty'               => 'Ei liitettäviä muutoksia.',
'mergehistory-success'             => '{{PLURAL:$3|Yksi versio|$3 versiota}} sivusta [[:$1]] liitettiin sivuun [[:$2]].',
'mergehistory-fail'                => 'Muutoshistorian liittäminen epäonnistui. Tarkista määritellyt sivut ja versiot.',
'mergehistory-no-source'           => 'Lähdesivua $1 ei ole olemassa.',
'mergehistory-no-destination'      => 'Kohdesivua $1 ei ole olemassa.',
'mergehistory-invalid-source'      => 'Lähdesivulla pitää olla kelvollinen nimi.',
'mergehistory-invalid-destination' => 'Kohdesivulla pitää olla kelvollinen nimi.',
'mergehistory-autocomment'         => 'Yhdisti sivun [[:$1]] sivuun [[:$2]]',
'mergehistory-comment'             => 'Yhdisti sivun [[:$1]] sivuun [[:$2]]: $3',

# Merge log
'mergelog'           => 'Yhdistämisloki',
'pagemerge-logentry' => 'liitti sivun [[$1]] sivuun [[$2]] (muokkaukseen $3 asti)',
'revertmerge'        => 'Kumoa yhdistäminen',
'mergelogpagetext'   => 'Alla on loki viimeisimmistä muutoshistorioiden yhdistämisistä.',

# Diffs
'history-title'           => 'Sivun $1 muutoshistoria',
'difference'              => 'Versioiden väliset erot',
'lineno'                  => 'Rivi $1:',
'compareselectedversions' => 'Vertaile valittuja versioita',
'editundo'                => 'kumoa',
'diff-multi'              => '(Versioiden välissä {{PLURAL:$1|yksi muu muokkaus|$1 muuta muokkausta}}.)',

# Search results
'searchresults'             => 'Hakutulokset',
'searchresulttext'          => 'Lisätietoa {{GRAMMAR:genitive|{{SITENAME}}}} hakutoiminnoista on [[{{MediaWiki:Helppage}}|ohjesivulla]].',
'searchsubtitle'            => 'Haku termeillä [[:$1]]',
'searchsubtitleinvalid'     => 'Haku termeillä $1',
'noexactmatch'              => 'Sivua ”$1” ei ole olemassa. Voit [[$1|luoda aiheesta uuden sivun]].',
'noexactmatch-nocreate'     => "'''Sivua nimeltä ”$1” ei ole.'''",
'toomanymatches'            => 'Liian monta osumaa. Kokeile erilaista kyselyä.',
'titlematches'              => 'Osumat sivujen otsikoissa',
'notitlematches'            => 'Hakusanaa ei löytynyt minkään sivun otsikosta',
'textmatches'               => 'Osumat sivujen teksteissä',
'notextmatches'             => 'Hakusanaa ei löytynyt sivujen teksteistä',
'prevn'                     => '← $1 edellistä',
'nextn'                     => '$1 seuraavaa →',
'viewprevnext'              => 'Näytä [$3] kerralla.

$1 | $2',
'search-result-size'        => '$1 ({{PLURAL:$2|1 sana|$2 sanaa}})',
'search-result-score'       => 'Asiaankuuluvuus: $1%',
'search-redirect'           => '(ohjaus $1)',
'search-section'            => '(osio $1)',
'search-suggest'            => 'Tarkoititko: $1',
'search-interwiki-caption'  => 'Sisarprojektit',
'search-interwiki-default'  => 'Tulokset osoitteesta $1:',
'search-interwiki-more'     => '(lisää)',
'search-mwsuggest-enabled'  => 'näytä ehdotukset',
'search-mwsuggest-disabled' => 'ilman ehdotuksia',
'search-relatedarticle'     => 'Hae samankaltaisia sivuja',
'mwsuggest-disable'         => 'Älä näytä ehdotuksia AJAXilla',
'searchrelated'             => 'samankaltainen',
'searchall'                 => 'kaikki',
'showingresults'            => "{{PLURAL:$1|'''Yksi''' tulos|'''$1''' tulosta}} tuloksesta '''$2''' alkaen.",
'showingresultsnum'         => "Alla on {{PLURAL:$3|'''Yksi''' hakutulos|'''$3''' hakutulosta}} alkaen '''$2.''' tuloksesta.",
'showingresultstotal'       => 'Alla on {{PLURAL:$3|tulos $1|tulokset $1–$2}}; yhteensä $3.',
'nonefound'                 => "'''Huomautus''': Epäonnistuneet haut johtuvat usein hyvin yleisten sanojen, kuten ''on'' ja ''ei'', etsimisestä tai useamman kuin yhden hakutermin määrittelemisestä. Vain sivut, joilla on kaikki hakutermin sanat, näkyvät tuloksissa.",
'powersearch'               => 'Etsi',
'powersearch-legend'        => 'Laajennettu haku',
'powersearch-ns'            => 'Hae nimiavaruuksista:',
'powersearch-redir'         => 'Luettele ohjaukset',
'powersearch-field'         => 'Etsi',
'search-external'           => 'Ulkoinen haku',
'searchdisabled'            => 'Tekstihaku on poistettu toistaiseksi käytöstä suuren kuorman vuoksi. Voit käyttää alla olevaa Googlen hakukenttää sivujen etsimiseen, kunnes haku tulee taas käyttöön. <small>Huomaa, että ulkopuoliset kopiot {{GRAMMAR:genitive|{{SITENAME}}}} sisällöstä eivät välttämättä ole ajan tasalla.</small>',

# Preferences page
'preferences'              => 'Asetukset',
'mypreferences'            => 'Asetukset',
'prefs-edits'              => 'Muokkauksia',
'prefsnologin'             => 'Et ole kirjautunut sisään.',
'prefsnologintext'         => 'Sinun täytyy <span class="plainlinks">[{{fullurl:Special:UserLogin|returnto=$1}} kirjautua sisään]</span>, jotta voisit muuttaa asetuksiasi.',
'prefsreset'               => 'Asetukset on palautettu tallennetuista asetuksistasi.',
'qbsettings'               => 'Pikavalikko',
'qbsettings-none'          => 'Ei mitään',
'qbsettings-fixedleft'     => 'Tekstin mukana, vasen',
'qbsettings-fixedright'    => 'Tekstin mukana, oikea',
'qbsettings-floatingleft'  => 'Pysyen vasemmalla',
'qbsettings-floatingright' => 'Pysyen oikealla',
'changepassword'           => 'Salasanan vaihto',
'skin'                     => 'Ulkonäkö',
'math'                     => 'Matematiikka',
'dateformat'               => 'Päiväyksen muoto',
'datedefault'              => 'Ei valintaa',
'datetime'                 => 'Aika ja päiväys',
'math_failure'             => 'Jäsentäminen epäonnistui',
'math_unknown_error'       => 'Tuntematon virhe',
'math_unknown_function'    => 'Tuntematon funktio',
'math_lexing_error'        => 'Tulkintavirhe',
'math_syntax_error'        => 'Jäsennysvirhe',
'math_image_error'         => 'PNG-muunnos epäonnistui; tarkista, että latex, dvips, gs ja convert on asennettu oikein.',
'math_bad_tmpdir'          => 'Matematiikan kirjoittaminen väliaikaishakemistoon tai tiedostonluonti ei onnistu',
'math_bad_output'          => 'Matematiikan tulostehakemistoon kirjoittaminen tai tiedostonluonti ei onnistu',
'math_notexvc'             => 'Texvc-sovellus puuttuu, lue math/READMEstä asennustietoja',
'prefs-personal'           => 'Käyttäjätiedot',
'prefs-rc'                 => 'Tuoreet muutokset',
'prefs-watchlist'          => 'Tarkkailulista',
'prefs-watchlist-days'     => 'Tarkkailulistan ajanjakso',
'prefs-watchlist-edits'    => 'Tarkkailulistalla näytettävien muokkausten määrä',
'prefs-misc'               => 'Muut',
'saveprefs'                => 'Tallenna asetukset',
'resetprefs'               => 'Palauta tallennetut asetukset',
'oldpassword'              => 'Vanha salasana',
'newpassword'              => 'Uusi salasana',
'retypenew'                => 'Uusi salasana uudelleen',
'textboxsize'              => 'Muokkaus',
'rows'                     => 'Rivit',
'columns'                  => 'Sarakkeet',
'searchresultshead'        => 'Haku',
'resultsperpage'           => 'Tuloksia sivua kohti',
'contextlines'             => 'Rivien määrä tulosta kohti',
'contextchars'             => 'Sisällön merkkien määrä riviä kohden',
'stub-threshold'           => '<a href="#" class="stub">Tynkäsivun</a> osoituskynnys',
'recentchangesdays'        => 'Näytettävien päivien määrä tuoreissa muutoksissa',
'recentchangescount'       => 'Sivujen määrä tuoreissa muutoksissa',
'savedprefs'               => 'Asetuksesi tallennettiin onnistuneesti.',
'timezonelegend'           => 'Aikavyöhyke',
'timezonetext'             => 'Paikallisen ajan ja palvelimen ajan (UTC) välinen aikaero tunteina.',
'localtime'                => 'Paikallinen aika',
'timezoneoffset'           => 'Aikaero',
'servertime'               => 'Palvelimen aika',
'guesstimezone'            => 'Utele selaimelta',
'allowemail'               => 'Salli sähköpostin lähetys osoitteeseen',
'prefs-searchoptions'      => 'Hakuasetukset',
'prefs-namespaces'         => 'Nimiavaruudet',
'defaultns'                => 'Etsi oletusarvoisesti näistä nimiavaruuksista',
'default'                  => 'oletus',
'files'                    => 'Tiedostot',

# User rights
'userrights'                  => 'Käyttöoikeuksien hallinta', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => 'Käyttöoikeuksien hallinta',
'userrights-user-editname'    => 'Käyttäjätunnus',
'editusergroup'               => 'Muokkaa käyttäjän ryhmiä',
'editinguser'                 => "Käyttäjän '''[[User:$1|$1]]''' oikeudet ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup'    => 'Käyttäjän ryhmät',
'saveusergroups'              => 'Tallenna',
'userrights-groupsmember'     => 'Käyttäjä on jäsenenä ryhmissä',
'userrights-groups-help'      => 'Voit muuttaa ryhmiä, joissa tämä käyttäjä on.
* Merkattu valintaruutu tarkoittaa, että käyttäjä on kyseisessä ryhmässä.
* Merkkaamaton valintaruutu tarkoittaa, että käyttäjä ei ole kyseisessä ryhmässä.
* <nowiki>*</nowiki> tarkoittaa, että et pysty kumoamaan kyseistä operaatiota.',
'userrights-reason'           => 'Kommentti',
'userrights-no-interwiki'     => 'Sinulla ei ole lupaa muokata käyttöoikeuksia muissa wikeissä.',
'userrights-nodatabase'       => 'Tietokantaa $1 ei ole tai se ei ole paikallinen.',
'userrights-nologin'          => 'Sinun täytyy [[Special:UserLogin|kirjautua sisään]] ylläpitäjätunnuksella, jotta voisit muuttaa käyttöoikeuksia.',
'userrights-notallowed'       => 'Tunnuksellasi ei ole lupaa muuttaa käyttöoikeuksia.',
'userrights-changeable-col'   => 'Ryhmät, joita voit muuttaa',
'userrights-unchangeable-col' => 'Ryhmät, joita et voi muuttaa',

# Groups
'group'               => 'Ryhmä',
'group-user'          => 'käyttäjät',
'group-autoconfirmed' => 'automaattisesti hyväksytyt käyttäjät',
'group-bot'           => 'botit',
'group-sysop'         => 'ylläpitäjät',
'group-bureaucrat'    => 'byrokraatit',
'group-suppress'      => 'häivytysoikeuden käyttäjät',
'group-all'           => '(kaikki)',

'group-user-member'          => 'käyttäjä',
'group-autoconfirmed-member' => 'automaattisesti hyväksytty käyttäjä',
'group-bot-member'           => 'botti',
'group-sysop-member'         => 'ylläpitäjä',
'group-bureaucrat-member'    => 'byrokraatti',
'group-suppress-member'      => 'häivytysoikeuden käyttäjä',

'grouppage-user'          => '{{ns:project}}:Käyttäjät',
'grouppage-autoconfirmed' => '{{ns:project}}:Automaattisesti hyväksytyt käyttäjät',
'grouppage-bot'           => '{{ns:project}}:Botit',
'grouppage-sysop'         => '{{ns:project}}:Ylläpitäjät',
'grouppage-bureaucrat'    => '{{ns:project}}:Byrokraatit',
'grouppage-suppress'      => '{{ns:project}}:Häivytysoikeudet',

# Rights
'right-read'                 => 'Lukea sivuja',
'right-edit'                 => 'Muokata sivuja',
'right-createpage'           => 'Luoda sivuja pois lukien keskustelusivut',
'right-createtalk'           => 'Luoda keskustelusivuja',
'right-createaccount'        => 'Luoda uusia käyttäjätunnuksia',
'right-minoredit'            => 'Merkitä muokkauksensa pieniksi',
'right-move'                 => 'Siirtää sivuja',
'right-move-subpages'        => 'Siirtää sivuja alasivuineen',
'right-suppressredirect'     => 'Siirtää sivuja luomatta automaattisia ohjauksia',
'right-upload'               => 'Tallentaa tiedostoja',
'right-reupload'             => 'Tallennetun tiedoston korvaaminen uudella',
'right-reupload-own'         => 'Korvata itsetallennettu tiedosto uudella tiedostolla',
'right-reupload-shared'      => 'Korvata jaettuun mediavarastoon tallennettuja tiedostoja paikallisesti',
'right-upload_by_url'        => 'Tallentaa tiedostoja verkko-osoitteella',
'right-purge'                => 'Päivittää tiedoston välimuistitetun version ilman varmennussivua',
'right-autoconfirmed'        => 'Muokata osittain suojattuja sivuja',
'right-bot'                  => 'Kohdellaan automaattisena prosessina',
'right-nominornewtalk'       => 'Tehdä pieniä muokkauksia käyttäjien keskustelusivuille siten, että käyttäjälle ei ilmoiteta siitä uutena viestinä',
'right-apihighlimits'        => 'Käyttää korkeampia rajoja API-kyselyissä',
'right-writeapi'             => 'Käyttää kirjoitus-APIa',
'right-delete'               => 'Poistaa sivuja',
'right-bigdelete'            => 'Poistaa sivuja, joilla on pitkä historia',
'right-deleterevision'       => 'Poistaa ja palauttaa sivujen versioita',
'right-deletedhistory'       => 'Tarkastella poistettuja versiotietoja ilman niihin liittyvää sisältöä',
'right-browsearchive'        => 'Tarkastella poistettuja sivuja',
'right-undelete'             => 'Palauttaa sivuja',
'right-suppressrevision'     => 'Tarkastella ja palauttaa ylläpitäjiltä piilotettuja versioita',
'right-suppressionlog'       => 'Tarkastella yksityisiä lokeja',
'right-block'                => 'Asettaa toiselle käyttäjälle muokkausesto',
'right-blockemail'           => 'Estää käyttäjää lähettämästä sähköpostia',
'right-hideuser'             => 'Estää käyttäjätunnus ja piilottaa se näkyvistä',
'right-ipblock-exempt'       => 'Ohittaa IP-, automaattiset ja osoitealue-estot',
'right-proxyunbannable'      => 'Ohittaa automaattiset välityspalvelinestot',
'right-protect'              => 'Muuttaa sivujen suojauksia ja muokata suojattuja sivuja',
'right-editprotected'        => 'Muokata suojattuja sivuja (pois lukien laajennettu sisällytyssuojaus)',
'right-editinterface'        => 'Muokata käyttöliittymätekstejä',
'right-editusercssjs'        => 'Muokata toisten käyttäjien CSS- ja JS-tiedostoja',
'right-rollback'             => 'Palauttaa nopeasti käyttäjän viimeisimmät muokkaukset sivuun',
'right-markbotedits'         => 'Kumota muokkauksia bottimerkinnällä',
'right-noratelimit'          => 'Ohittaa nopeusrajoitukset',
'right-import'               => 'Tuoda sivuja muista wikeistä',
'right-importupload'         => 'Tuoda sivuja tiedostosta',
'right-patrol'               => 'Merkitä muokkaukset tarkastetuiksi',
'right-autopatrol'           => 'Muokkaukset aina valmiiksi tarkastetuksi merkittyjä',
'right-patrolmarks'          => 'Nähdä tarkastusmerkit tuoreissa muutoksissa',
'right-unwatchedpages'       => 'Tarkastella listaa tarkkailemattomista sivuista',
'right-trackback'            => 'Lähettää trackback',
'right-mergehistory'         => 'Yhdistää sivujen historioita',
'right-userrights'           => 'Muuttaa kaikkia käyttäjäoikeuksia',
'right-userrights-interwiki' => 'Muokata käyttäjien oikeuksia muissa wikeissä',
'right-siteadmin'            => 'Lukita tietokanta',

# User rights log
'rightslog'      => 'Käyttöoikeusloki',
'rightslogtext'  => 'Tämä on loki käyttäjien käyttöoikeuksien muutoksista.',
'rightslogentry' => 'Käyttäjän [[$1]] oikeudet muutettiin ryhmistä $2 ryhmiin $3',
'rightsnone'     => '(ei oikeuksia)',

# Recent changes
'nchanges'                          => '$1 {{PLURAL:$1|muutos|muutosta}}',
'recentchanges'                     => 'Tuoreet muutokset',
'recentchangestext'                 => 'Tällä sivulla voi seurata tuoreita {{GRAMMAR:illative|{{SITENAME}}}} tehtyjä muutoksia.',
'recentchanges-feed-description'    => 'Tällä sivulla voi seurata tuoreita {{GRAMMAR:illative|{{SITENAME}}}} tehtyjä muutoksia.',
'rcnote'                            => 'Alla on {{PLURAL:$1|yksi muutos|$1 tuoreinta muutosta}} {{PLURAL:$2|yhden päivän|$2 viime päivän}} ajalta $4 kello $5 asti.',
'rcnotefrom'                        => 'Alla on muutokset <b>$2</b> lähtien. Enintään <b>$1</b> merkintää näytetään.',
'rclistfrom'                        => 'Näytä uudet muutokset $1 alkaen',
'rcshowhideminor'                   => '$1 pienet muutokset',
'rcshowhidebots'                    => '$1 botit',
'rcshowhideliu'                     => '$1 kirjautuneet käyttäjät',
'rcshowhideanons'                   => '$1 anonyymit käyttäjät',
'rcshowhidepatr'                    => '$1 tarkastetut muutokset',
'rcshowhidemine'                    => '$1 omat muutokset',
'rclinks'                           => 'Näytä $1 tuoretta muutosta viimeisten $2 päivän ajalta.<br />$3',
'diff'                              => 'ero',
'hist'                              => 'historia',
'hide'                              => 'piilota',
'show'                              => 'näytä',
'minoreditletter'                   => 'p',
'newpageletter'                     => 'U',
'boteditletter'                     => 'b',
'number_of_watching_users_pageview' => '[$1 {{PLURAL:$1|tarkkaileva käyttäjä|tarkkailevaa käyttäjää}}]',
'rc_categories'                     => 'Vain luokista (erotin on ”|”)',
'rc_categories_any'                 => 'Mikä tahansa',
'newsectionsummary'                 => '/* $1 */ uusi osio',

# Recent changes linked
'recentchangeslinked'          => 'Linkitettyjen sivujen muutokset',
'recentchangeslinked-title'    => 'Sivulta $1 linkitettyjen sivujen muutokset',
'recentchangeslinked-noresult' => 'Ei muutoksia linkitettyihin sivuihin annetulla aikavälillä.',
'recentchangeslinked-summary'  => "Tämä toimintosivu näyttää muutokset sivuihin, joihin on viitattu tältä sivulta. Tarkkailulistallasi olevat sivut on '''paksunnettu'''.",
'recentchangeslinked-page'     => 'Sivu',
'recentchangeslinked-to'       => 'Näytä muutokset sivuihin, joilla on linkki annettuun sivuun',

# Upload
'upload'                      => 'Tallenna tiedosto',
'uploadbtn'                   => 'Tallenna',
'reupload'                    => 'Lähetä uudelleen',
'reuploaddesc'                => 'Palaa lähetyslomakkeelle.',
'uploadnologin'               => 'Et ole kirjautunut sisään',
'uploadnologintext'           => 'Sinun pitää olla [[Special:UserLogin|kirjautuneena sisään]], jotta voisit tallentaa tiedostoja.',
'upload_directory_missing'    => 'Tallennushakemisto $1 puuttuu, eikä palvelin pysty luomaan sitä.',
'upload_directory_read_only'  => 'Palvelimella ei ole kirjoitusoikeuksia tallennushakemistoon ”<tt>$1</tt>”.',
'uploaderror'                 => 'Tallennusvirhe',
'uploadtext'                  => "Voit tallentaa tiedostoja alla olevalla lomakkeella. [[Special:ImageList|Tiedostoluettelo]] sisältää listan tallennetuista tiedostoista. Tallennukset kirjataan myös [[Special:Log/upload|tallennuslokiin]], ja poistot [[Special:Log/delete|poistolokiin]].

Voit käyttää tiedostoja wikisivuilla seuraavilla tavoilla:
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:Tiedosto.jpg]]</nowiki></tt>''', käyttääksesi tiedoston täyttä versiota.
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:Tiedosto.png|200px|thumb|left|Kuvausteksti]]</nowiki></tt>''', käyttääksesi tiedostoa sovitettuna 200 kuvapistettä leveään laatikkoon kuvaustekstillä.
* '''<tt><nowiki>[[</nowiki>{{ns:media}}<nowiki>:Tiedosto.ogg]]</nowiki></tt>''', jos haluat suoran linkin tiedostoon.",
'upload-permitted'            => 'Sallitut tiedostomuodot: $1.',
'upload-preferred'            => 'Suositellut tiedostomuodot: $1.',
'upload-prohibited'           => 'Kielletyt tiedostomuodot: $1.',
'uploadlog'                   => 'Tiedostoloki',
'uploadlogpage'               => 'Tiedostoloki',
'uploadlogpagetext'           => 'Alla on luettelo uusimmista tiedostonlisäyksistä. Kaikki ajat näytetään palvelimen aikavyöhykkeessä.',
'filename'                    => 'Tiedoston nimi:',
'filedesc'                    => 'Yhteenveto',
'fileuploadsummary'           => 'Yhteenveto',
'filestatus'                  => 'Tiedoston tekijänoikeudet',
'filesource'                  => 'Lähde',
'uploadedfiles'               => 'Lisätyt tiedostot',
'ignorewarning'               => 'Tallenna tiedosto varoituksesta huolimatta.',
'ignorewarnings'              => 'Ohita kaikki varoitukset',
'minlength1'                  => 'Tiedoston nimessä pitää olla vähintään yksi merkki.',
'illegalfilename'             => "Tiedoston nimessä '''$1''' on merkkejä, joita ei sallita sivujen nimissä. Vaihda tiedoston nimeä, ja yritä lähettämistä uudelleen.",
'badfilename'                 => 'Tiedoston nimi vaihdettiin: $1.',
'filetype-badmime'            => 'Tiedostot, joiden MIME-tyyppi on <tt>$1</tt> ei voi lähettää.',
'filetype-unwanted-type'      => "'''.$1''' ei ole toivottu tiedostomuoto. {{PLURAL:$3|Suositeltu tiedostomuoto on|Suositeltuja tiedostomuotoja ovat}} $2.",
'filetype-banned-type'        => "'''.$1''' ei ole sallittu tiedostomuoto. {{PLURAL:$3|Sallittu tiedostomuoto on|Sallittuja tiedostomuotoja ovat}} $2.",
'filetype-missing'            => 'Tiedostolta puuttuu tiedostopääte – esimerkiksi <tt>.jpg</tt>.',
'large-file'                  => 'Tiedostojen enimmäiskoko on $1. Lähettämäsi tiedoston koko on $2.',
'largefileserver'             => 'Tämä tiedosto on suurempi kuin mitä palvelin sallii.',
'emptyfile'                   => 'Tiedosto, jota yritit lähettää, näyttää olevan tyhjä. Tarkista, että kirjoitit polun ja nimen oikein ja että se ei ole liian suuri kohdepalvelimelle.',
'fileexists'                  => 'Samanniminen tiedosto on jo olemassa. Katso tiedoston sivu <strong><tt>$1</tt></strong>, jos et ole varma, haluatko muuttaa sitä.',
'filepageexists'              => 'Kuvaussivu on jo olemassa tällä nimellä <strong><tt>$1</tt></strong>, mutta ei tiedostoa tällä nimellä. Kirjoittamasi yhteenveto ei ilmesty kuvaussivulle. Muuttaaksesi uuden yhteenvedon kuvaussivulle, sinun täytyy manuaalisesti muokata sitä.',
'fileexists-extension'        => 'Tiedosto, jolla on samankaltainen nimi, on jo olemassa:<br />
Tallennetun tiedoston nimi: <strong><tt>$1</tt></strong><br />
Olemassa olevan tiedoston nimi: <strong><tt>$2</tt></strong><br />
Ainoa ero on tiedostopäätteen kirjainkoko. Tarkista ovatko tiedostot identtisiä.',
'fileexists-thumb'            => "<center>'''Olemassa oleva tiedosto'''</center>",
'fileexists-thumbnail-yes'    => 'Tiedosto näyttäisi olevan pienennetty kuva <i>(pienoiskuva)</i>. Tarkista tiedosto <strong><tt>$1</tt></strong>.<br />
Jos yllä oleva tiedosto on alkuperäisversio samasta kuvasta, ei sille tarvi tallentaa pienoiskuvaa.',
'file-thumbnail-no'           => 'Tiedostonimi alkaa merkkijonolla <strong><tt>$1</tt></strong>. Tiedosto näyttäisi olevan pienennetty kuva <i>(pienoiskuva)</i>.
Jos sinulla on tämän kuvan alkuperäinen versio, tallenna se. Muussa tapauksessa nimeä tiedosto uudelleen.',
'fileexists-forbidden'        => 'Samanniminen tiedosto on jo olemassa. Tallenna tiedosto jollakin toisella nimellä. Nykyinen tiedosto: [[Image:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => 'Samanniminen tiedosto on jo olemassa jaetussa mediavarastossa. Tallenna tiedosto jollakin toisella nimellä. Nykyinen tiedosto: [[Image:$1|thumb|center|$1]]',
'file-exists-duplicate'       => 'Tämä tiedosto on kaksoiskappale {{PLURAL:$1|seuraavasta tiedostosta|seuraavista tiedostoista}}:',
'successfulupload'            => 'Tallennus onnistui',
'uploadwarning'               => 'Tallennusvaroitus',
'savefile'                    => 'Tallenna',
'uploadedimage'               => 'tallensi tiedoston [[$1]]',
'overwroteimage'              => 'tallensi uuden version tiedostosta [[$1]]',
'uploaddisabled'              => 'Tiedostojen tallennus ei ole käytössä.',
'uploaddisabledtext'          => 'Tiedostojen tallennus on poistettu käytöstä.',
'uploadscripted'              => 'Tämä tiedosto sisältää HTML-koodia tai skriptejä, jotka selain saattaa virheellisesti suorittaa.',
'uploadcorrupt'               => 'Tiedosto on vioittunut tai sillä on väärä tiedostopääte. Tarkista tiedosto ja lähetä se uudelleen.',
'uploadvirus'                 => 'Tiedosto sisältää viruksen. Tarkemmat tiedot: $1',
'sourcefilename'              => 'Lähdenimi',
'destfilename'                => 'Kohdenimi',
'upload-maxfilesize'          => 'Suurin sallittu tiedostokoko: $1',
'watchthisupload'             => 'Tarkkaile tätä sivua',
'filewasdeleted'              => 'Tämän niminen tiedosto on lisätty ja poistettu aikaisemmin. Tarkista $1 ennen jatkamista.',
'upload-wasdeleted'           => "'''Varoitus: Olet tallentamassa tiedostoa, joka on jo aikaisemmin poistettu.'''

Harkitse, haluatko jatkaa tämän tiedoston tallentamista. Tiedoston poistoloki näkyy tässä:",
'filename-bad-prefix'         => 'Tallentamasi tiedoston nimi alkaa merkkijonolla <strong>$1</strong>, joka on yleensä digitaalikameroiden automaattisesti antama nimi, joka ei kuvaa tiedoston sisältöä. Anna tiedostolle kuvaavampi nimi.',

'upload-proto-error'      => 'Virheellinen protokolla',
'upload-proto-error-text' => 'Etälähetys on mahdollista vain osoitteista, jotka alkavat merkkijonolla <code>http://</code> tai <code>ftp://</code>.',
'upload-file-error'       => 'Vakava virhe',
'upload-file-error-text'  => 'Väliaikaistiedoston luominen epäonnistui. Ota yhteyttä sivuston ylläpitäjään.',
'upload-misc-error'       => 'Virhe',
'upload-misc-error-text'  => 'Tiedoston etälähetys ei onnistunut. Varmista, että antamasi osoite on oikein ja toimiva. Jos virhe ei katoa, ota yhteyttä sivuston ylläpitäjään.',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => 'Toimimaton osoite',
'upload-curl-error6-text'  => 'Antamaasi osoitteeseen ei saatu yhteyttä. Varmista, että osoite on oikein ja että sivusto on saavutettavissa.',
'upload-curl-error28'      => 'Etälähetyksen aikakatkaisu',
'upload-curl-error28-text' => 'Antamastasi osoitteesta ei saatu vastausta määräajassa. Varmista, että sivusto on saavutettavissa ja yritä uudelleen.',

'license'            => 'Lisenssi',
'nolicense'          => 'Ei lisenssiä',
'license-nopreview'  => '(esikatselua ei saatavilla)',
'upload_source_url'  => ' (julkinen verkko-osoite)',
'upload_source_file' => ' (tiedosto tietokoneella)',

# Special:ImageList
'imagelist-summary'     => 'Tämä toimintosivu näyttää kaikki tallennetut tiedostot. Viimeisin tallennettu tiedosto on listalla ensimmäisenä. Ryhmittelyperustetta voi vaihtaa napsauttamalla sarakenimeä.',
'imagelist_search_for'  => 'Nimihaku',
'imgfile'               => 'tiedosto',
'imagelist'             => 'Tiedostoluettelo',
'imagelist_date'        => 'Päiväys',
'imagelist_name'        => 'Nimi',
'imagelist_user'        => 'Tallentaja',
'imagelist_size'        => 'Koko',
'imagelist_description' => 'Kuvaus',

# Image description page
'filehist'                       => 'Tiedoston historia',
'filehist-help'                  => 'Päiväystä napsauttamalla näet millainen tiedosto oli sillä ajan hetkellä.',
'filehist-deleteall'             => 'poista kaikki',
'filehist-deleteone'             => 'poista tämä',
'filehist-revert'                => 'palauta',
'filehist-current'               => 'nykyinen',
'filehist-datetime'              => 'Päiväys',
'filehist-user'                  => 'Käyttäjä',
'filehist-dimensions'            => 'Koko',
'filehist-filesize'              => 'Tiedostokoko',
'filehist-comment'               => 'Kommentti',
'imagelinks'                     => 'Viittaukset sivuilta',
'linkstoimage'                   => '{{PLURAL:$1|Seuraavalta sivulta|$1 sivulla}} on linkki tähän tiedostoon:',
'nolinkstoimage'                 => 'Tähän tiedostoon ei ole linkkejä miltään sivulta.',
'morelinkstoimage'               => 'Näytä [[Special:WhatLinksHere/$1|lisää linkkejä]] tähän tiedostoon.',
'redirectstofile'                => '{{PLURAL:$1|Seuraava tiedosto ohjaa|Seuraavat $1 tiedostoa ohjaavat}} tähän tiedostoon:',
'duplicatesoffile'               => '{{PLURAL:$1|Seuraava tiedosto on tämän tiedoston kaksoiskappale|Seuraavat $1 tiedostoa ovat tämän tiedoston kaksoiskappaleita}}:',
'sharedupload'                   => 'Tämä tiedosto on jaettu ja muut projektit saattavat käyttää sitä.',
'shareduploadwiki'               => 'Katso $1 lisätietoja.',
'shareduploadwiki-desc'          => 'Tiedot tiedoston $1 jaetussa mediavarastossa näkyvät alla.',
'shareduploadwiki-linktext'      => 'kuvaussivulta',
'shareduploadduplicate'          => 'Tämä tiedosto on sama kuin $1 jaetussa mediavarastossa.',
'shareduploadduplicate-linktext' => 'toinen tiedosto',
'shareduploadconflict'           => 'Tiedostolla on sama nimi kuin $1 jaetussa mediavarastossa.',
'shareduploadconflict-linktext'  => 'toisella tiedostolla',
'noimage'                        => 'Tämän nimistä tiedostoa ei ole olemassa, mutta voit $1.',
'noimage-linktext'               => 'tallentaa sen',
'uploadnewversion-linktext'      => 'Tallenna uusi versio tästä tiedostosta',
'imagepage-searchdupe'           => 'Etsi tiedoston kaksoiskappaleita',

# File reversion
'filerevert'                => 'Tiedoston $1 palautus',
'filerevert-legend'         => 'Tiedoston palautus',
'filerevert-intro'          => '<span class="plainlinks">Olet palauttamassa tiedostoa \'\'\'[[Media:$1|$1]]\'\'\' [$4 versioon, joka luotiin $2 kello $3].</span>',
'filerevert-comment'        => 'Syy',
'filerevert-defaultcomment' => 'Palautettiin versioon, joka luotiin $1 kello $2',
'filerevert-submit'         => 'Palauta',
'filerevert-success'        => '<span class="plainlinks">\'\'\'[[Media:$1|$1]]\'\'\' on palautettu [$4 versioon, joka luotiin $2 kello $3].</span>',
'filerevert-badversion'     => 'Tiedostosta ei ole luotu versiota kyseisellä ajan hetkellä.',

# File deletion
'filedelete'                  => 'Tiedoston $1 poisto',
'filedelete-legend'           => 'Tiedoston poisto',
'filedelete-intro'            => "Olet poistamassa tiedostoa '''[[Media:$1|$1]]'''.",
'filedelete-intro-old'        => '<span class="plainlinks">Olet poistamassa tiedoston \'\'\'[[Media:$1|$1]]\'\'\' [$4 $3 kello $2 luotua versiota].</span>',
'filedelete-comment'          => 'Poiston syy',
'filedelete-submit'           => 'Poista',
'filedelete-success'          => "Tiedosto '''$1''' on poistettu.",
'filedelete-success-old'      => "Tiedoston '''[[Media:$1|$1]]''' $3 kello $2 luotu versio on poistettu.",
'filedelete-nofile'           => "Tiedostoa '''$1''' ei ole.",
'filedelete-nofile-old'       => "Tiedostosta '''$1''' ei ole olemassa pyydettyä versiota.",
'filedelete-iscurrent'        => 'Et voi poistaa tiedoston uusinta versiota. Palauta jokin muu version uusimmaksi.',
'filedelete-otherreason'      => 'Muu syy tai tarkennus',
'filedelete-reason-otherlist' => 'Muu syy',
'filedelete-reason-dropdown'  => '*Yleiset poistosyyt
** Kaksoiskappale
** Tekijänoikeusrikkomus',
'filedelete-edit-reasonlist'  => 'Muokkaa poistosyitä',

# MIME search
'mimesearch'         => 'MIME-haku',
'mimesearch-summary' => 'Tällä sivulla voit etsiä tiedostoja niiden MIME-tyypin perusteella. Syöte: sisältötyyppi/alatyyppi, esimerkiksi <tt>image/jpeg</tt>.',
'mimetype'           => 'MIME-tyyppi',
'download'           => 'lataa',

# Unwatched pages
'unwatchedpages' => 'Tarkkailemattomat sivut',

# List redirects
'listredirects' => 'Ohjaukset',

# Unused templates
'unusedtemplates'     => 'Käyttämättömät mallineet',
'unusedtemplatestext' => 'Tässä on lista kaikista mallineista, joita ei ole liitetty toiselle sivulle. Muista tarkistaa onko malline siitä huolimatta käytössä.',
'unusedtemplateswlh'  => 'muut linkit',

# Random page
'randompage'         => 'Satunnainen sivu',
'randompage-nopages' => 'Tässä nimiavaruudessa ei ole sivuja.',

# Random redirect
'randomredirect'         => 'Satunnainen ohjaussivu',
'randomredirect-nopages' => 'Tässä nimiavaruudessa ei ole ohjaussivuja.',

# Statistics
'statistics'             => 'Tilastot',
'sitestats'              => 'Sivuston tilastot',
'userstats'              => 'Käyttäjätilastot',
'sitestatstext'          => "Tietokannassa on {{PLURAL:$1|yksi sivu|yhteensä $1 sivua}}. Tähän on laskettu mukaan keskustelusivut, {{GRAMMAR:genitive|{{SITENAME}}}} projektisivut, hyvin lyhyet sivut, ohjaussivut sekä muita sivuja, joita ei voi pitää kunnollisina sivuina. Nämä pois lukien tietokannassa on '''$2''' {{PLURAL:$2|sivu|sivua}}.

{{GRAMMAR:illative|{{SITENAME}}}} on tallennettu '''$8''' {{PLURAL:$8|tiedosto|tiedostoa}}.

Sivuja on katsottu yhteensä '''$3''' {{PLURAL:$3|kerran|kertaa}} ja muokattu '''$4''' {{PLURAL:$4|kerran|kertaa}}. Keskimäärin yhtä sivua on muokattu '''$5''' kertaa, ja muokkausta kohden sivua on katsottu keskimäärin '''$6''' kertaa.

Ohjelmiston suorittamia ylläpitotöitä on jonossa '''$7''' {{PLURAL:$7|kappale|kappaletta}}.",
'userstatstext'          => "Rekisteröityneitä käyttäjiä on '''$1'''. Näistä '''$2''' ($4%) on {{PLURAL:$2|ylläpitäjä|ylläpitäjiä}} ($5).",
'statistics-mostpopular' => 'Katsotuimmat sivut',

'disambiguations'      => 'Linkit täsmennyssivuihin',
'disambiguationspage'  => 'Template:Täsmennyssivu',
'disambiguations-text' => "Seuraavat artikkelit linkittävät ''täsmennyssivuun''. Täsmennyssivun sijaan niiden pitäisi linkittää asianomaiseen aiheeseen.<br />Sivua kohdellaan täsmennyssivuna jos se käyttää mallinetta, johon on linkki sivulta [[MediaWiki:Disambiguationspage]].",

'doubleredirects'            => 'Kaksinkertaiset ohjaukset',
'doubleredirectstext'        => '<b>Huomio:</b> Tässä listassa saattaa olla virheitä. Yleensä kyseessä on sivu, jossa ensimmäisen #REDIRECT- tai #OHJAUS-komennon jälkeen on tekstiä.<br />Jokaisella rivillä on linkit ensimmäiseen ja toiseen ohjaukseen sekä toisen ohjauksen kohteen ensimmäiseen riviin, eli yleensä ”oikeaan” kohteeseen, johon ensimmäisen ohjauksen pitäisi osoittaa.',
'double-redirect-fixed-move' => '[[$1]] on siirretty, ja se ohjaa nyt sivulle [[$2]]',
'double-redirect-fixer'      => 'Ohjausten korjaaja',

'brokenredirects'        => 'Virheelliset ohjaukset',
'brokenredirectstext'    => 'Seuraavat ohjaukset osoittavat sivuihin, joita ei ole olemassa.',
'brokenredirects-edit'   => '(muokkaa)',
'brokenredirects-delete' => '(poista)',

'withoutinterwiki'         => 'Sivut, joilla ei ole kielilinkkejä',
'withoutinterwiki-summary' => 'Seuraavat sivut eivät viittaa erikielisiin versioihin:',
'withoutinterwiki-legend'  => 'Etuliite',
'withoutinterwiki-submit'  => 'Näytä',

'fewestrevisions' => 'Sivut, joilla on vähiten muutoksia',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|tavu|tavua}}',
'ncategories'             => '$1 {{PLURAL:$1|luokka|luokkaa}}',
'nlinks'                  => '$1 {{PLURAL:$1|linkki|linkkiä}}',
'nmembers'                => '$1 {{PLURAL:$1|jäsen|jäsentä}}',
'nrevisions'              => '$1 {{PLURAL:$1|muutos|muutosta}}',
'nviews'                  => '$1 {{PLURAL:$1|lataus|latausta}}',
'specialpage-empty'       => 'Tämä sivu on tyhjä.',
'lonelypages'             => 'Yksinäiset sivut',
'lonelypagestext'         => 'Seuraaviin sivuhin ei ole linkkejä muualta wikistä.',
'uncategorizedpages'      => 'Luokittelemattomat sivut',
'uncategorizedcategories' => 'Luokittelemattomat luokat',
'uncategorizedimages'     => 'Luokittelemattomat tiedostot',
'uncategorizedtemplates'  => 'Luokittelemattomat mallineet',
'unusedcategories'        => 'Käyttämättömät luokat',
'unusedimages'            => 'Käyttämättömät tiedostot',
'popularpages'            => 'Suositut sivut',
'wantedcategories'        => 'Halutut luokat',
'wantedpages'             => 'Halutut sivut',
'missingfiles'            => 'Puuttuvat tiedostot',
'mostlinked'              => 'Viitatuimmat sivut',
'mostlinkedcategories'    => 'Viitatuimmat luokat',
'mostlinkedtemplates'     => 'Viitatuimmat mallineet',
'mostcategories'          => 'Luokitelluimmat sivut',
'mostimages'              => 'Viitatuimmat tiedostot',
'mostrevisions'           => 'Muokatuimmat sivut',
'prefixindex'             => 'Sivujen katkaisuhaku',
'shortpages'              => 'Lyhyet sivut',
'longpages'               => 'Pitkät sivut',
'deadendpages'            => 'Sivut, joilla ei ole linkkejä',
'deadendpagestext'        => 'Seuraavat sivut eivät linkitä muihin sivuihin wikissä.',
'protectedpages'          => 'Suojatut sivut',
'protectedpages-indef'    => 'Vain ikuiset estot',
'protectedpagestext'      => 'Seuraavat sivut ovat suojattuja siirtämiseltä tai muutoksilta',
'protectedpagesempty'     => 'Ei suojattu sivuja.',
'protectedtitles'         => 'Suojatut sivunimet',
'protectedtitlestext'     => 'Seuraavien sivujen luonti on estetty.',
'protectedtitlesempty'    => 'Ei suojattuja sivunimiä näillä hakuehdoilla.',
'listusers'               => 'Käyttäjälista',
'newpages'                => 'Uudet sivut',
'newpages-username'       => 'Käyttäjätunnus',
'ancientpages'            => 'Kauan muokkaamattomat sivut',
'move'                    => 'Siirrä',
'movethispage'            => 'Siirrä tämä sivu',
'unusedimagestext'        => 'Huomaa, että muut verkkosivut saattavat viitata tiedostoon suoran URL:n avulla, jolloin tiedosto saattaa olla tässä listassa, vaikka sitä käytetäänkin.',
'unusedcategoriestext'    => 'Nämä luokat ovat olemassa, mutta niitä ei käytetä.',
'notargettitle'           => 'Ei kohdetta',
'notargettext'            => 'Et ole määritellyt kohdesivua tai -käyttäjää johon toiminto kohdistuu.',
'nopagetitle'             => 'Kohdesivua ei ole olemassa.',
'nopagetext'              => 'Määritettyä kohdesivua ei ole olemassa.',
'pager-newer-n'           => '← {{PLURAL:$1|1 uudempi|$1 uudempaa}}',
'pager-older-n'           => '{{PLURAL:$1|1 vanhempi|$1 vanhempaa}} →',
'suppress'                => 'Häivytys',

# Book sources
'booksources'               => 'Kirjalähteet',
'booksources-search-legend' => 'Etsi kirjalähteitä',
'booksources-isbn'          => 'ISBN',
'booksources-go'            => 'Etsi',
'booksources-text'          => 'Alla linkkejä ulkopuolisiin sivustoihin, joilla myydään uusia ja käytettyjä kirjoja. Sivuilla voi myös olla lisätietoa kirjoista.',

# Special:Log
'specialloguserlabel'  => 'Käyttäjä',
'speciallogtitlelabel' => 'Kohde',
'log'                  => 'Lokit',
'all-logs-page'        => 'Kaikki lokit',
'log-search-legend'    => 'Etsi lokeista',
'log-search-submit'    => 'Hae',
'alllogstext'          => 'Tämä on yhdistetty lokien näyttö.
Voit rajoittaa listaa valitsemalla lokityypin, käyttäjän tai sivun johon muutos on kohdistunut. Jälkimmäiset ovat kirjainkokoherkkiä.',
'logempty'             => 'Ei tapahtumia lokissa.',
'log-title-wildcard'   => 'Kohde alkaa merkkijonolla',

# Special:AllPages
'allpages'          => 'Kaikki sivut',
'alphaindexline'    => '$1…$2',
'nextpage'          => 'Seuraava sivu ($1)',
'prevpage'          => 'Edellinen sivu ($1)',
'allpagesfrom'      => 'Alkaen sivusta',
'allarticles'       => 'Kaikki sivut',
'allinnamespace'    => 'Kaikki sivut nimiavaruudessa $1',
'allnotinnamespace' => 'Kaikki sivut, jotka eivät ole nimiavaruudessa $1',
'allpagesprev'      => 'Edellinen',
'allpagesnext'      => 'Seuraava',
'allpagessubmit'    => 'Hae',
'allpagesprefix'    => 'Katkaisuhaku',
'allpagesbadtitle'  => 'Annettu otsikko oli kelvoton tai siinä oli wikien välinen etuliite.',
'allpages-bad-ns'   => '{{GRAMMAR:inessive|{{SITENAME}}}} ei ole nimiavaruutta ”$1”.',

# Special:Categories
'categories'                    => 'Luokat',
'categoriespagetext'            => 'Seuraavat luokat sisältävät sivuja tai mediatiedostoja.
[[Special:UnusedCategories|Käyttämättömiä luokkia]] ei näytetä.
Katso myös [[Special:WantedCategories|halutut luokat]].',
'categoriesfrom'                => 'Näytä alkaen luokasta',
'special-categories-sort-count' => 'järjestä koon mukaan',
'special-categories-sort-abc'   => 'järjestä nimen mukaan',

# Special:ListUsers
'listusersfrom'      => 'Katkaisuhaku',
'listusers-submit'   => 'Hae',
'listusers-noresult' => 'Käyttäjiä ei löytynyt.',

# Special:ListGroupRights
'listgrouprights'          => 'Käyttäjäryhmien oikeudet',
'listgrouprights-summary'  => 'Tämä lista sisältää tämän wikin käyttäjäryhmät sekä ryhmiin liitetyt käyttöoikeudet.
Lisätietoa yksittäisistä käyttäjäoikeuksista saattaa löytyä [[{{MediaWiki:Listgrouprights-helppage}}|erilliseltä ohjesivulta]].',
'listgrouprights-group'    => 'Ryhmä',
'listgrouprights-rights'   => 'Oikeudet',
'listgrouprights-helppage' => 'Help:Käyttöoikeudet',
'listgrouprights-members'  => '(jäsenlista)',

# E-mail user
'mailnologin'     => 'Lähettäjän osoite puuttuu',
'mailnologintext' => 'Sinun pitää olla [[Special:UserLogin|kirjautuneena sisään]] ja [[Special:Preferences|asetuksissasi]] pitää olla toimiva ja <strong>varmennettu</strong> sähköpostiosoite, jotta voit lähettää sähköpostia muille käyttäjille.',
'emailuser'       => 'Lähetä sähköpostia tälle käyttäjälle',
'emailpage'       => 'Lähetä sähköpostia käyttäjälle',
'emailpagetext'   => 'Jos tämä käyttäjä on antanut asetuksissaan kelvollisen sähköpostiosoitteen, alla olevalla lomakkeella voit lähettää yhden viestin hänelle. [[Special:Preferences|Omissa asetuksissasi]] annettu sähköpostiosoite näkyy sähköpostin lähettäjän osoitteena, jotta vastaanottaja voi suoraan vastata viestiin.',
'usermailererror' => 'Postitus palautti virheen:',
'defemailsubject' => '{{SITENAME}}-sähköposti',
'noemailtitle'    => 'Ei sähköpostiosoitetta',
'noemailtext'     => 'Tämä käyttäjä ei ole määritellyt kelpoa sähköpostiosoitetta tai ei halua postia muilta käyttäjiltä.',
'emailfrom'       => 'Lähettäjä',
'emailto'         => 'Vastaanottaja',
'emailsubject'    => 'Aihe',
'emailmessage'    => 'Viesti',
'emailsend'       => 'Lähetä',
'emailccme'       => 'Lähetä kopio viestistä minulle.',
'emailccsubject'  => 'Kopio lähettämästäsi viestistä osoitteeseen $1: $2',
'emailsent'       => 'Sähköposti lähetetty',
'emailsenttext'   => 'Sähköpostiviestisi on lähetetty.',
'emailuserfooter' => 'Tämän sähköpostin lähetti $1 käyttäjälle $2 käyttämällä ”Lähetä sähköpostia” -toimintoa {{GRAMMAR:inessive|{{SITENAME}}}}.',

# Watchlist
'watchlist'            => 'Tarkkailulista',
'mywatchlist'          => 'Tarkkailulista',
'watchlistfor'         => '$1',
'nowatchlist'          => 'Tarkkailulistallasi ei ole sivuja.',
'watchlistanontext'    => 'Sinun täytyy $1, jos haluat käyttää tarkkailulistaa.',
'watchnologin'         => 'Et ole kirjautunut sisään',
'watchnologintext'     => 'Sinun pitää [[Special:UserLogin|kirjautua sisään]], jotta voisit käyttää tarkkailulistaasi.',
'addedwatch'           => 'Lisätty tarkkailulistalle',
'addedwatchtext'       => "Sivu '''<nowiki>$1</nowiki>''' on lisätty [[Special:Watchlist|tarkkailulistallesi]]. Tulevaisuudessa sivuun ja sen keskustelusivuun tehtävät muutokset listataan täällä. Sivu on '''lihavoitu''' [[Special:RecentChanges|tuoreiden muutosten listassa]], jotta huomaisit sen helpommin. Jos haluat myöhemmin poistaa sivun tarkkailulistaltasi, napsauta linkkiä ''lopeta tarkkailu'' sivun reunassa.",
'removedwatch'         => 'Poistettu tarkkailulistalta',
'removedwatchtext'     => "Sivu '''<nowiki>$1</nowiki>''' on poistettu tarkkailulistaltasi.",
'watch'                => 'Tarkkaile',
'watchthispage'        => 'Tarkkaile tätä sivua',
'unwatch'              => 'Lopeta tarkkailu',
'unwatchthispage'      => 'Lopeta tarkkailu',
'notanarticle'         => 'Ei ole sivu',
'notvisiblerev'        => 'Versio on poistettu',
'watchnochange'        => 'Valittuna ajanjaksona yhtäkään tarkkailemistasi sivuista ei muokattu.',
'watchlist-details'    => 'Tarkkailulistalla on {{PLURAL:$1|$1 sivu|$1 sivua}} keskustelusivuja mukaan laskematta.',
'wlheader-enotif'      => '* Sähköposti-ilmoitukset ovat käytössä.',
'wlheader-showupdated' => "* Sivut, joita on muokattu viimeisen käyntisi jälkeen on merkitty '''paksummalla'''",
'watchmethod-recent'   => 'tarkistetaan tuoreimpia muutoksia tarkkailluille sivuille',
'watchmethod-list'     => 'tarkistetaan tarkkailtujen sivujen tuoreimmat muutokset',
'watchlistcontains'    => 'Tarkkailulistallasi on {{PLURAL:$1|yksi sivu|$1 sivua}}.',
'iteminvalidname'      => 'Sivun $1 kanssa oli ongelmia! Sivun nimessä on vikaa.',
'wlnote'               => "Alla on '''$1''' {{PLURAL:$1|muutos|muutosta}} viimeisen {{PLURAL:$2||'''$2'''}} tunnin ajalta.",
'wlshowlast'           => 'Näytä viimeiset $1 tuntia tai $2 päivää$3',
'watchlist-show-bots'  => 'Näytä bottien muokkaukset',
'watchlist-hide-bots'  => 'Piilota bottien muokkaukset',
'watchlist-show-own'   => 'Näytä omat muokkaukset',
'watchlist-hide-own'   => 'Piilota omat muokkaukset',
'watchlist-show-minor' => 'Näytä pienet muokkaukset',
'watchlist-hide-minor' => 'Piilota pienet muokkaukset',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Lisätään tarkkailulistalle...',
'unwatching' => 'Poistetaan tarkkailulistalta...',

'enotif_mailer'                => '{{GRAMMAR:genitive|{{SITENAME}}}} sivu on muuttunut -ilmoitus',
'enotif_reset'                 => 'Merkitse kaikki sivut katsotuiksi',
'enotif_newpagetext'           => 'Tämä on uusi sivu.',
'enotif_impersonal_salutation' => '{{SITENAME}}-käyttäjä',
'changed'                      => 'muuttanut sivua',
'created'                      => 'luonut sivun',
'enotif_subject'               => '$PAGEEDITOR on $CHANGEDORCREATED $PAGETITLE',
'enotif_lastvisited'           => 'Osoitteessa $1 on kaikki muutokset viimeisen käyntisi jälkeen.',
'enotif_lastdiff'              => 'Muutos on osoitteessa $1.',
'enotif_anon_editor'           => 'kirjautumaton käyttäjä $1',
'enotif_body'                  => 'Käyttäjä $WATCHINGUSERNAME,

{{GRAMMAR:genitive|{{SITENAME}}}} käyttäjä $PAGEEDITOR on $CHANGEDORCREATED $PAGETITLE $PAGEEDITDATE. Nykyinen versio on osoitteessa $PAGETITLE_URL .

$NEWPAGE

Muokkaajan yhteenveto: $PAGESUMMARY $PAGEMINOREDIT

Ota yhteyttä muokkaajaan:
sähköposti: $PAGEEDITOR_EMAIL
wiki: $PAGEEDITOR_WIKI

Uusia ilmoituksia tästä sivusta ei tule kunnes vierailet sivulla. Voit myös nollata ilmoitukset kaikille tarkkailemillesi sivuille tarkkailulistallasi.

             {{GRAMMAR:genitive|{{SITENAME}}}} ilmoitusjärjestelmä

--
Tarkkailulistan asetuksia voit muuttaa osoitteessa:
{{fullurl:Special:Watchlist/edit}}

Palaute ja lisäapu osoitteessa:
{{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => 'Poista sivu',
'confirm'                     => 'Toteuta',
'excontent'                   => 'sisälsi: ”$1”',
'excontentauthor'             => 'sisälsi: ”$1” (ainoa muokkaaja oli $2)',
'exbeforeblank'               => 'ennen tyhjentämistä sisälsi: ”$1”',
'exblank'                     => 'oli tyhjä',
'delete-confirm'              => 'Sivun ”$1” poistaminen',
'delete-legend'               => 'Sivun poisto',
'historywarning'              => 'Sivua, jonka aiot poistaa on muokattu useammin kuin kerran:',
'confirmdeletetext'           => 'Olet poistamassa sivun tai tiedoston ja kaiken sen historian. Ymmärrä teon seuraukset ja tee poisto {{GRAMMAR:genitive|{{SITENAME}}}} [[{{MediaWiki:Policy-url}}|käytäntöjen]] mukaisesti.',
'actioncomplete'              => 'Toiminto suoritettu',
'deletedtext'                 => '”<nowiki>$1</nowiki>” on poistettu.
Sivulla $2 on lista viimeaikaisista poistoista.',
'deletedarticle'              => 'poisti sivun $1',
'suppressedarticle'           => 'häivytti sivun [[$1]]',
'dellogpage'                  => 'Poistoloki',
'dellogpagetext'              => 'Alla on loki viimeisimmistä poistoista.',
'deletionlog'                 => 'poistoloki',
'reverted'                    => 'Palautettu aikaisempaan versioon',
'deletecomment'               => 'Poistamisen syy',
'deleteotherreason'           => 'Muu syy tai tarkennus',
'deletereasonotherlist'       => 'Muu syy',
'deletereason-dropdown'       => '*Yleiset poistosyyt
** Lisääjän poistopyyntö
** Tekijänoikeusrikkomus
** Roskaa',
'delete-edit-reasonlist'      => 'Muokkaa poistosyitä',
'delete-toobig'               => 'Tällä sivulla on pitkä muutoshistoria – yli $1 {{PLURAL:$1|versio|versiota}}. Näin suurien muutoshistorioiden poistamista on rajoitettu suorituskykysyistä.',
'delete-warning-toobig'       => 'Tällä sivulla on pitkä muutoshistoria – yli $1 {{PLURAL:$1|versio|versiota}}. Näin suurien muutoshistorioiden poistaminen voi haitata sivuston suorituskykyä.',
'rollback'                    => 'palauta aiempaan versioon',
'rollback_short'              => 'Palautus',
'rollbacklink'                => 'palauta',
'rollbackfailed'              => 'Palautus epäonnistui',
'cantrollback'                => 'Aiempaan versioon ei voi palauttaa, koska viimeisin kirjoittaja on sivun ainoa tekijä.',
'alreadyrolled'               => 'Käyttäjän [[User:$2|$2]] ([[User talk:$2|keskustelu]] | [[Special:Contributions/$2|{{int:contribslink}}]]) tekemiä muutoksia sivuun [[:$1]] ei voi kumota, koska joku muu on muuttanut sivua.

Viimeisimmän muokkauksen on tehnyt käyttäjä [[User:$3|$3]] ([[User talk:$3|keskustelu]] | [[Special:Contributions/$3|{{int:contribslink}}]]).',
'editcomment'                 => 'Muokkauksen yhteenveto oli: <i>$1</i>.', # only shown if there is an edit comment
'revertpage'                  => 'Käyttäjän [[Special:Contributions/$2|$2]] ([[User talk:$2|keskustelu]]) muokkaukset kumottiin ja sivu palautettiin viimeisimpään käyttäjän [[User:$1|$1]] tekemään versioon.', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => 'Käyttäjän ”$1” tekemät muokkaukset kumottiin ja artikkeli palautettiin käyttäjän $2 versioon.',
'sessionfailure'              => 'Istuntosi kanssa on ongelma. Muutosta ei toteutettu varotoimena sessionkaappauksien takia. Käytä selaimen paluutoimintoa ja päivitä sivu, jolta tulit, ja yritä uudelleen.',
'protectlogpage'              => 'Suojausloki',
'protectlogtext'              => 'Alla on loki sivujen suojauksista ja suojauksien poistoista. Luettelo tällä hetkellä suojatuista sivuista löytyy [[Special:ProtectedPages|suojattuen sivujen luettelosta]].',
'protectedarticle'            => 'suojasi sivun $1',
'modifiedarticleprotection'   => 'muutti sivun [[$1]] suojaustasoa',
'unprotectedarticle'          => 'poisti suojauksen sivulta $1',
'protect-title'               => 'Sivun $1 suojaus',
'protect-legend'              => 'Suojaukset',
'protectcomment'              => 'Perustelu',
'protectexpiry'               => 'Vanhenee',
'protect_expiry_invalid'      => 'Vanhenemisaika ei kelpaa.',
'protect_expiry_old'          => 'Vanhenemisaika on menneisyydessä.',
'protect-unchain'             => 'Käytä siirtosuojausta',
'protect-text'                => 'Voit katsoa ja muuttaa sivun <strong><nowiki>$1</nowiki></strong> suojauksia.',
'protect-locked-blocked'      => 'Et voi muuttaa sivun suojauksia, koska sinut on estetty. Alla on sivun ”<strong>$1</strong>” nykyiset suojaukset:',
'protect-locked-dblock'       => 'Sivun suojauksia ei voi muuttaa, koska tietokanta on lukittu. Alla on sivun ”<strong>$1</strong>” nykyiset suojaukset:',
'protect-locked-access'       => 'Sinulla ei ole tarvittavia oikeuksia sivujen suojauksen muuttamiseen. Alla on sivun ”<strong>$1</strong>” nykyiset suojaukset:',
'protect-cascadeon'           => 'Tämä sivu on suojauksen kohteena, koska se on sisällytetty alla {{PLURAL:$1|olevaan laajennetusti suojattuun sivuun|oleviin laajennetusti suojattuihin sivuihin}}. Voit muuttaa tämän sivun suojaustasoa, mutta se ei vaikuta laajennettuun suojaukseen.',
'protect-default'             => '(ei rajoituksia)',
'protect-fallback'            => 'Vaadi $1-oikeus',
'protect-level-autoconfirmed' => 'Estä uudet ja anonyymit käyttäjät',
'protect-level-sysop'         => 'Vain ylläpitäjät',
'protect-summary-cascade'     => 'laajennettu',
'protect-expiring'            => 'vanhenee $1',
'protect-cascade'             => 'Laajenna suojaus koskemaan kaikkia tähän sivuun sisällytettyjä sivuja.',
'protect-cantedit'            => 'Et voi muuttaa sivun suojaustasoa, koska sinulla ei ole oikeutta muokata sivua.',
'restriction-type'            => 'Rajoitus',
'restriction-level'           => 'Suojaus',
'minimum-size'                => 'Vähimmäiskoko',
'maximum-size'                => 'Enimmäiskoko',
'pagesize'                    => 'tavua',

# Restrictions (nouns)
'restriction-edit'   => 'muokkaus',
'restriction-move'   => 'siirto',
'restriction-create' => 'luonti',
'restriction-upload' => 'tiedostotallennus',

# Restriction levels
'restriction-level-sysop'         => 'täysin suojattu',
'restriction-level-autoconfirmed' => 'osittaissuojattu',
'restriction-level-all'           => 'mikä tahansa suojaus',

# Undelete
'undelete'                     => 'Palauta poistettuja sivuja',
'undeletepage'                 => 'Poistettujen sivujen selaus',
'undeletepagetitle'            => "'''Poistetut versiot sivusta [[:$1]]'''.",
'viewdeletedpage'              => 'Poistettujen sivujen selaus',
'undeletepagetext'             => 'Seuraavat sivut on poistettu, mutta ne löytyvät vielä arkistosta, joten ne ovat palautettavissa. Arkisto saatetaan tyhjentää aika ajoin.',
'undelete-fieldset-title'      => 'Palauta versiot',
'undeleteextrahelp'            => "Palauta sivu valitsemalla '''''Palauta'''''. Voit palauttaa versiota valikoivasti valitsemalla vain niiden versioiden valintalaatikot, jotka haluat palauttaa.",
'undeleterevisions'            => '{{PLURAL:$1|Versio|$1 versiota}} arkistoitu.',
'undeletehistory'              => 'Jos palautat sivun, kaikki versiot lisätään sivun historiaan. Jos uusi sivu samalla nimellä on luotu poistamisen jälkeen, palautetut versiot lisätään sen historiaan.',
'undeleterevdel'               => 'Palautusta ei tehdä, jos sen seurauksena sivun uusin versio olisi osittain poistettu. Tässä tilanteessa poista uusimman poistettavan version piilotus. Tiedostoversioita, joihin sinulla ei ole katseluoikeutta ei palauteta.',
'undeletehistorynoadmin'       => 'Tämä sivu on poistettu. Syy sivun poistamiseen näkyy yhteenvedossa, jossa on myös tiedot, ketkä ovat muokanneet tätä sivua ennen poistamista. Sivujen varsinainen sisältö on vain ylläpitäjien luettavissa.',
'undelete-revision'            => 'Poistettu sivu $1 hetkellä $2. Tekijä: $3.',
'undeleterevision-missing'     => 'Virheellinen tai puuttuva versio. Se on saatettu palauttaa tai poistaa arkistosta.',
'undelete-nodiff'              => 'Aikaisempaa versiota ei löytynyt.',
'undeletebtn'                  => 'Palauta',
'undeletelink'                 => 'palauta',
'undeletereset'                => 'Tyhjennä',
'undeletecomment'              => 'Kommentti',
'undeletedarticle'             => 'palautti sivun [[$1]]',
'undeletedrevisions'           => '{{PLURAL:$1|Yksi versio|$1 versiota}} palautettiin',
'undeletedrevisions-files'     => '{{PLURAL:$1|Yksi versio|$1 versiota}} ja {{PLURAL:$2|yksi tiedosto|$2 tiedostoa}} palautettiin',
'undeletedfiles'               => '{{PLURAL:$1|1 tiedosto|$1 tiedostoa}} palautettiin',
'cannotundelete'               => 'Palauttaminen epäonnistui.',
'undeletedpage'                => "<big>'''$1 on palautettu.'''</big>

[[Special:Log/delete|Poistolokista]] löydät listan viimeisimmistä poistoista ja palautuksista.",
'undelete-header'              => '[[Special:Log/delete|Poistolokissa]] on lista viimeisimmistä poistoista.',
'undelete-search-box'          => 'Etsi poistettuja sivuja',
'undelete-search-prefix'       => 'Näytä sivut, jotka alkavat merkkijonolla:',
'undelete-search-submit'       => 'Hae',
'undelete-no-results'          => 'Poistoarkistosta ei löytynyt haettuja sivuja.',
'undelete-filename-mismatch'   => 'Tiedoston version, jonka aikaleima on $1 palauttaminen epäonnistui, koska tiedostonimi ei ole sama.',
'undelete-bad-store-key'       => 'Tiedoston version, jonka aikaleima on $1 palauttaminen epäonnistui, koska tiedostoa ei ollut ennen poistoa.',
'undelete-cleanup-error'       => 'Käyttämättömän arkistotiedoston $1 poistaminen epäonnistui.',
'undelete-missing-filearchive' => 'Tiedostoarkiston tunnuksen $1 hakeminen epäonnistui. Tiedosto on saatettu jo palauttaa.',
'undelete-error-short'         => 'Tiedoston $1 palauttaminen epäonnistui',
'undelete-error-long'          => 'Tiedoston palauttaminen epäonnistui:

$1',

# Namespace form on various pages
'namespace'      => 'Nimiavaruus',
'invert'         => 'Käännä nimiavaruusvalinta päinvastaiseksi',
'blanknamespace' => '(sivut)',

# Contributions
'contributions' => 'Käyttäjän muokkaukset',
'mycontris'     => 'Omat muokkaukset',
'contribsub2'   => 'Käyttäjän $1 ($2) muokkaukset',
'nocontribs'    => 'Näihin ehtoihin sopivia muokkauksia ei löytynyt.',
'uctop'         => ' (uusin)',
'month'         => 'Kuukausi',
'year'          => 'Vuosi',

'sp-contributions-newbies'     => 'Näytä uusien tulokkaiden muutokset',
'sp-contributions-newbies-sub' => 'Uusien tulokkaiden muokkaukset',
'sp-contributions-blocklog'    => 'estot',
'sp-contributions-search'      => 'Etsi muokkauksia',
'sp-contributions-username'    => 'IP-osoite tai käyttäjätunnus',
'sp-contributions-submit'      => 'Hae',

# What links here
'whatlinkshere'            => 'Tänne viittaavat sivut',
'whatlinkshere-title'      => 'Sivut, jotka viittaavat sivulle $1',
'whatlinkshere-page'       => 'Sivu',
'linklistsub'              => 'Lista linkeistä',
'linkshere'                => 'Seuraavilta sivuilta on linkki sivulle <strong>[[:$1]]</strong>:',
'nolinkshere'              => 'Sivulle <strong>[[:$1]]</strong> ei ole linkkejä.',
'nolinkshere-ns'           => 'Sivulle <strong>[[:$1]]</strong> ei ole linkkejä valitussa nimiavaruudessa.',
'isredirect'               => 'ohjaussivu',
'istemplate'               => 'sisällytetty mallineeseen',
'isimage'                  => 'tiedostolinkki',
'whatlinkshere-prev'       => '← {{PLURAL:$1|edellinen sivu|$1 edellistä sivua}}',
'whatlinkshere-next'       => '{{PLURAL:$1|seuraava sivu|$1 seuraavaa sivua}} →',
'whatlinkshere-links'      => 'viittaukset',
'whatlinkshere-hideredirs' => '$1 ohjaukset',
'whatlinkshere-hidetrans'  => '$1 sisällytykset',
'whatlinkshere-hidelinks'  => '$1 linkit',
'whatlinkshere-hideimages' => '$1 tiedostolinkit',
'whatlinkshere-filters'    => 'Suotimet',

# Block/unblock
'blockip'                         => 'Aseta muokkausesto',
'blockip-legend'                  => 'Estä käyttäjä',
'blockiptext'                     => 'Tällä lomakkeella voit estää käyttäjän tai IP-osoitteen muokkausoikeudet. Muokkausoikeuksien poistamiseen pitää olla syy, esimerkiksi sivujen vandalisointi. Kirjoita syy siihen varattuun kenttään.<br />Vapaamuotoisen vanhenemisajat noudattavat GNUn standardimuotoa, joka on kuvattu tar-manuaalissa ([http://www.gnu.org/software/tar/manual/html_node/Date-input-formats.html] [EN]), esimerkiksi ”1 hour”, ”2 days”, ”next Wednesday”, 2005-08-29”.',
'ipaddress'                       => 'IP-osoite',
'ipadressorusername'              => 'IP-osoite tai käyttäjätunnus',
'ipbexpiry'                       => 'Kesto',
'ipbreason'                       => 'Syy',
'ipbreasonotherlist'              => 'Muu syy',
'ipbreason-dropdown'              => '
*Yleiset estosyyt
** Väärän tiedon lisääminen
** Sisällön poistaminen
** Mainoslinkkien lisääminen
** Sotkeminen tai roskan lisääminen
** Häiriköinti
** Useamman käyttäjätunnuksen väärinkäyttö
** Sopimaton käyttäjätunnus',
'ipbanononly'                     => 'Estä vain kirjautumattomat käyttäjät',
'ipbcreateaccount'                => 'Estä tunnusten luonti',
'ipbemailban'                     => 'Estä käyttäjää lähettämästä sähköpostia',
'ipbenableautoblock'              => 'Estä viimeisin IP-osoite, josta käyttäjä on muokannut, sekä ne osoitteet, joista hän jatkossa yrittää muokata.',
'ipbsubmit'                       => 'Estä',
'ipbother'                        => 'Vapaamuotoinen kesto',
'ipboptions'                      => '2 tuntia:2 hours,1 päivä:1 day,3 päivää:3 days,1 viikko:1 week,2 viikkoa:2 weeks,1 kuukausi:1 month,3 kuukautta:3 months,6 kuukautta:6 months,1 vuosi:1 year,ikuinen:infinite', # display1:time1,display2:time2,...
'ipbotheroption'                  => 'Muu kesto',
'ipbotherreason'                  => 'Muu syy tai tarkennus',
'ipbhidename'                     => 'Piilota IP-osoite tai tunnus estolokista, muokkausestolistasta ja käyttäjälistasta',
'ipbwatchuser'                    => 'Tarkkaile tämän käyttäjän käyttäjä- ja keskustelusivua',
'badipaddress'                    => 'IP-osoite on väärin muotoiltu.',
'blockipsuccesssub'               => 'Esto onnistui',
'blockipsuccesstext'              => "Käyttäjä tai IP-osoite '''$1''' on estetty.<br />Nykyiset estot löytyvät [[Special:IPBlockList|estolistalta]].",
'ipb-edit-dropdown'               => 'Muokkaa syitä',
'ipb-unblock-addr'                => 'Poista käyttäjän $1 esto',
'ipb-unblock'                     => 'Poista käyttäjän tai IP-osoitteen muokkausesto',
'ipb-blocklist-addr'              => 'Näytä käyttäjän $1 estot',
'ipb-blocklist'                   => 'Näytä estot',
'unblockip'                       => 'Muokkauseston poisto',
'unblockiptext'                   => 'Tällä lomakkeella voit poistaa käyttäjän tai IP-osoitteen muokkauseston.',
'ipusubmit'                       => 'Poista esto',
'unblocked'                       => 'Käyttäjän [[User:$1|$1]] esto on poistettu',
'unblocked-id'                    => 'Esto $1 on poistettu',
'ipblocklist'                     => 'Estetyt IP-osoitteet ja käyttäjätunnukset',
'ipblocklist-legend'              => 'Haku',
'ipblocklist-username'            => 'Käyttäjätunnus tai IP-osoite',
'ipblocklist-submit'              => 'Hae',
'blocklistline'                   => '$1 – $2 on estänyt käyttäjän $3 ($4)',
'infiniteblock'                   => 'ikuisesti',
'expiringblock'                   => 'vanhenee $1',
'anononlyblock'                   => 'vain kirjautumattomat',
'noautoblockblock'                => 'ei automaattista IP-osoitteiden estoa',
'createaccountblock'              => 'tunnusten luonti estetty',
'emailblock'                      => 'sähköpostin lähettäminen estetty',
'ipblocklist-empty'               => 'Estolista on tyhjä.',
'ipblocklist-no-results'          => 'Pyydettyä IP-osoitetta tai käyttäjätunnusta ei ole estetty.',
'blocklink'                       => 'estä',
'unblocklink'                     => 'poista esto',
'contribslink'                    => 'muokkaukset',
'autoblocker'                     => 'Olet automaattisesti estetty, koska jaat IP-osoitteen käyttäjän $1 kanssa. Eston syy: $2.',
'blocklogpage'                    => 'Estoloki',
'blocklogentry'                   => 'esti käyttäjän tai IP-osoitteen $1. Eston kesto $2 $3',
'blocklogtext'                    => 'Tämä on loki muokkausestoista ja niiden purkamisista. Automaattisesti estettyjä IP-osoitteita ei kirjata. Tutustu [[Special:IPBlockList|estolistaan]] nähdäksesi listan tällä hetkellä voimassa olevista estoista.',
'unblocklogentry'                 => 'poisti käyttäjältä $1 muokkauseston',
'block-log-flags-anononly'        => 'vain kirjautumattomat käyttäjät',
'block-log-flags-nocreate'        => 'tunnusten luonti estetty',
'block-log-flags-noautoblock'     => 'ei automaattista IP-osoitteiden estoa',
'block-log-flags-noemail'         => 'sähköpostin lähettäminen estetty',
'block-log-flags-angry-autoblock' => 'kehittynyt automaattiesto käytössä',
'range_block_disabled'            => 'Ylläpitäjän oikeus luoda alue-estoja ei ole käytössä.',
'ipb_expiry_invalid'              => 'Virheellinen umpeutumisaika.',
'ipb_expiry_temp'                 => 'Piilotettujen käyttäjätunnusten estojen tulee olla pysyviä.',
'ipb_already_blocked'             => '”$1” on jo estetty.',
'ipb_cant_unblock'                => 'Estoa ”$1” ei löytynyt. Se on saatettu poistaa.',
'ipb_blocked_as_range'            => 'IP-osoite $1 on estetty välillisesti ja sen estoa ei voi poistaa. Se on estetty osana verkkoaluetta $2, jonka eston voi poistaa',
'ip_range_invalid'                => 'Virheellinen IP-alue.',
'blockme'                         => 'Estä minut',
'proxyblocker'                    => 'Välityspalvelinesto',
'proxyblocker-disabled'           => 'Tämä toiminto ei ole käytössä.',
'proxyblockreason'                => 'IP-osoitteestasi on estetty muokkaukset, koska se on avoin välityspalvelin. Ota yhteyttä Internet-palveluntarjoajaasi tai tekniseen tukeen ja kerro heillä tästä tietoturvaongelmasta.',
'proxyblocksuccess'               => 'Valmis.',
'sorbsreason'                     => 'IP-osoitteesi on listattu avoimena välityspalvelimena DNSBLin mustalla listalla.',
'sorbs_create_account_reason'     => 'IP-osoitteesi on listattu avoimena välityspalvelimena DNSBLin mustalla listalla. Et voi luoda käyttäjätunnusta.',

# Developer tools
'lockdb'              => 'Lukitse tietokanta',
'unlockdb'            => 'Vapauta tietokanta',
'lockdbtext'          => 'Tietokannan lukitseminen estää käyttäjiä muokkaamasta sivuja, vaihtamasta asetuksia, muokkaamasta tarkkailulistoja ja tekemästä muita tietokannan muuttamista vaativia toimia. Ole hyvä ja vahvista, että tämä on tarkoituksesi, ja että vapautat tietokannan kun olet suorittanut ylläpitotehtävät.',
'unlockdbtext'        => 'Tietokannan vapauttaminen antaa käyttäjille mahdollisuuden muokata sivuja, vaihtaa asetuksia, muokata tarkkailulistoja ja tehdä muita tietokannan muuttamista vaativia toimia. Ole hyvä ja vahvista, että tämä on tarkoituksesi.',
'lockconfirm'         => 'Kyllä, haluan varmasti lukita tietokannan.',
'unlockconfirm'       => 'Kyllä, haluan varmasti vapauttaa tietokannan.',
'lockbtn'             => 'Lukitse tietokanta',
'unlockbtn'           => 'Vapauta tietokanta',
'locknoconfirm'       => 'Et merkinnyt vahvistuslaatikkoa.',
'lockdbsuccesssub'    => 'Tietokannan lukitseminen onnistui',
'unlockdbsuccesssub'  => 'Tietokannan vapauttaminen onnistui',
'lockdbsuccesstext'   => 'Tietokanta on lukittu.<br />Muista vapauttaa tietokanta ylläpitotoimenpiteiden jälkeen.',
'unlockdbsuccesstext' => 'Tietokanta on vapautettu.',
'lockfilenotwritable' => 'Tietokannan lukitustiedostoa ei voi kirjoittaa. Tarkista oikeudet.',
'databasenotlocked'   => 'Tietokanta ei ole lukittu.',

# Move page
'move-page'               => 'Siirrä $1',
'move-page-legend'        => 'Siirrä sivu',
'movepagetext'            => "Alla olevalla lomakkeella voit nimetä uudelleen sivuja, jolloin niiden koko historia siirtyy uuden nimen alle.
Vanhasta sivusta tulee ohjaussivu, joka osoittaa uuteen sivuun.
Voit päivittää sivuun viittaavat ohjaukset automaattisesti ohjaamaan uudelle nimelle.
Jos et halua tätä tehtävän automaattisesti, muista tehdä tarkistukset [[Special:DoubleRedirects|kaksinkertaisten]] tai [[Special:BrokenRedirects|rikkinäisten]] ohjausten varalta.
Olet vastuussa siitä, että linkit osoittavat sinne, mihin niiden on tarkoituskin osoittaa.

Huomaa, että sivua '''ei''' siirretä mikäli uusi otsikko on olemassa olevan sivun käytössä, paitsi milloin kyseessä on tyhjä sivu tai ohjaus, jolla ei ole muokkaushistoriaa.
Tämä tarkoittaa sitä, että voit siirtää sivun takaisin vanhalle nimelleen mikäli teit virheen, mutta et voi kirjoittaa olemassa olevan sivun päälle.

Tämä saattaa olla suuri ja odottamaton muutos suositulle sivulle. Varmista, että tiedät seuraukset ennen kuin siirrät sivun.",
'movepagetalktext'        => "Sivuun mahdollisesti kytketty keskustelusivu siirretään automaattisesti, '''paitsi jos''':
*Siirrät sivua nimiavaruudesta toiseen
*Kohdesivulla on olemassa keskustelusivu, joka ei ole tyhjä, tai
*Kumoat alla olevan ruudun asetuksen.

Näissä tapauksissa sivut täytyy siirtää tai yhdistää käsin.",
'movearticle'             => 'Siirrä sivu',
'movenotallowed'          => 'Sinulla ei ole oikeuksia siirtää sivuja.',
'newtitle'                => 'Uusi nimi sivulle',
'move-watch'              => 'Tarkkaile tätä sivua',
'movepagebtn'             => 'Siirrä sivu',
'pagemovedsub'            => 'Siirto onnistui',
'movepage-moved'          => "<big>'''$1 on siirretty nimelle $2'''</big>", # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => 'Kohdesivu on jo olemassa, tai valittu nimi ei ole sopiva. Ole hyvä ja valitse uusi nimi.',
'cantmove-titleprotected' => 'Sivua ei voi siirtää tälle nimelle, koska tämän nimisen sivun luonti on estetty.',
'talkexists'              => 'Sivun siirto onnistui, mutta keskustelusivua ei voitu siirtää, koska uuden otsikon alla on jo keskustelusivu. Keskustelusivujen sisältö täytyy yhdistää käsin.',
'movedto'                 => 'Siirretty uudelle otsikolle',
'movetalk'                => 'Siirrä myös keskustelusivu.',
'move-subpages'           => 'Siirrä kaikki alasivut, jos mahdollista',
'move-talk-subpages'      => 'Siirrä kaikki keskustelusivun alasivut, jos mahdollista',
'movepage-page-exists'    => 'Sivu $1 on jo olemassa ja sitä ei voi automaattisesti korvata.',
'movepage-page-moved'     => 'Sivu $1 on siirretty nimelle $2.',
'movepage-page-unmoved'   => 'Sivua $1 ei voitu siirtää nimelle $2.',
'movepage-max-pages'      => 'Enimmäismäärä sivuja on siirretty, eikä enempää siirretä enää automaattisesti.
$1 {{PLURAL:$1|sivu|sivua}} siirettiin.',
'1movedto2'               => 'siirsi sivun ”$1” uudelle nimelle ”$2”',
'1movedto2_redir'         => 'siirsi sivun ”$1” ohjauksen ”$2” päälle',
'movelogpage'             => 'Siirtoloki',
'movelogpagetext'         => 'Tämä on loki siirretyistä sivuista.',
'movereason'              => 'Syy',
'revertmove'              => 'kumoa',
'delete_and_move'         => 'Poista kohdesivu ja siirrä',
'delete_and_move_text'    => 'Kohdesivu [[:$1]] on jo olemassa. Haluatko poistaa sen, jotta nykyinen sivu voitaisiin siirtää?',
'delete_and_move_confirm' => 'Poista sivu',
'delete_and_move_reason'  => 'Sivu on siirron tiellä.',
'selfmove'                => 'Lähde- ja kohdenimi ovat samat.',
'immobile_namespace'      => 'Sivuja ei voi siirtää tähän nimiavaruuteen.',
'imagenocrossnamespace'   => 'Tiedostoja ei voi siirtää pois tiedostonimiavaruudesta.',
'imagetypemismatch'       => 'Uusi tiedostopääte ei vastaa tiedoston tyyppiä',
'imageinvalidfilename'    => 'Kohdenimi on virheellinen',
'fix-double-redirects'    => 'Päivitä kaikki tänne viittaavat ohjaukset ohjaamaan uudelle nimelle',

# Export
'export'            => 'Sivujen vienti',
'exporttext'        => 'Voit viedä sivun tai sivujen tekstiä ja muokkaushistoriaa XML-muodossa.
Tämä tieto voidaan tuoda toiseen MediaWikiin käyttämällä [[Special:Import|tuontisivua]].

Syötä sivujen otsikoita jokainen omalle rivilleen alla olevaan laatikkoon.
Valitse myös, haluatko kaikki versiot sivuista, vai ainoastaan nykyisen version.

Jälkimmäisessä tapauksessa voit myös käyttää linkkiä. Esimerkiksi sivun [[{{MediaWiki:Mainpage}}]] saa vietyä linkistä [[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]].',
'exportcuronly'     => 'Liitä mukaan ainoastaan uusin versio – ei koko historiaa.',
'exportnohistory'   => '----
Sivujen koko historian vienti on estetty suorituskykysyistä.',
'export-submit'     => 'Vie',
'export-addcattext' => 'Lisää sivut luokasta',
'export-addcat'     => 'Lisää',
'export-download'   => 'Tallenna tiedostona',
'export-templates'  => 'Liitä mallineet',

# Namespace 8 related
'allmessages'               => 'Järjestelmäviestit',
'allmessagesname'           => 'Nimi',
'allmessagesdefault'        => 'Oletusarvo',
'allmessagescurrent'        => 'Nykyinen arvo',
'allmessagestext'           => 'Tämä on luettelo kaikista MediaWiki-nimiavaruudessa olevista viesteistä.',
'allmessagesnotsupportedDB' => 'Tämä sivu ei ole käytössä, koska <tt>$wgUseDatabaseMessages</tt>-asetus on pois päältä.',
'allmessagesfilter'         => 'Viestiavainsuodatin:',
'allmessagesmodified'       => 'Näytä vain muutetut',

# Thumbnails
'thumbnail-more'           => 'Suurenna',
'filemissing'              => 'Tiedosto puuttuu',
'thumbnail_error'          => 'Pienoiskuvan luominen epäonnistui: $1',
'djvu_page_error'          => 'DjVu-tiedostossa ei ole pyydettyä sivua',
'djvu_no_xml'              => 'DjVu-tiedoston XML-vienti epäonnistui',
'thumbnail_invalid_params' => 'Virheelliset parametrit pienoiskuvalle',
'thumbnail_dest_directory' => 'Kohdehakemiston luominen ei onnistunut',

# Special:Import
'import'                     => 'Tuo sivuja',
'importinterwiki'            => 'Tuo sivuja lähiwikeistä',
'import-interwiki-text'      => 'Valitse wiki ja sivun nimi. Versioiden päivämäärät ja muokkaajat säilytetään. Kaikki wikienväliset tuonnit kirjataan [[Special:Log/import|tuontilokiin]].',
'import-interwiki-history'   => 'Kopioi sivun koko historia',
'import-interwiki-submit'    => 'Tuo',
'import-interwiki-namespace' => 'Siirrä nimiavaruuteen:',
'importtext'                 => 'Vie sivuja lähdewikistä käyttäen [[Special:Export|vienti]]-työkalua. Tallenna tiedot koneellesi ja tallenna ne täällä.',
'importstart'                => 'Tuodaan sivuja...',
'import-revision-count'      => '$1 {{PLURAL:$1|versio|versiota}}',
'importnopages'              => 'Ei tuotavia sivuja.',
'importfailed'               => 'Tuonti epäonnistui: $1',
'importunknownsource'        => 'Tuntematon lähdetyyppi',
'importcantopen'             => 'Tuontitiedoston avaus epäonnistui',
'importbadinterwiki'         => 'Kelpaamaton wikienvälinen linkki',
'importnotext'               => 'Tyhjä tai ei tekstiä',
'importsuccess'              => 'Tuonti onnistui!',
'importhistoryconflict'      => 'Sivusta on olemassa tuonnin kanssa ristiriitainen muokkausversio. Tämä sivu on saatettu tuoda jo aikaisemmin.',
'importnosources'            => 'Wikienvälisiä tuontilähteitä ei ole määritelty ja suorat historiatallennukset on poistettu käytöstä.',
'importnofile'               => 'Mitään tuotavaa tiedostoa ei lähetetty.',
'importuploaderrorsize'      => 'Tuontitiedoston tallennus epäonnistui. Tiedosto on suurempi kuin sallittu yläraja.',
'importuploaderrorpartial'   => 'Tuontitiedoston tallennus epäonnistui. Tiedostosta oli lähetetty vain osa.',
'importuploaderrortemp'      => 'Tuontitiedoston tallennus epäonnistui. Väliaikaistiedostojen kansio puuttuu.',
'import-parse-failure'       => 'XML-tuonti epäonnistui jäsennysvirheen takia.',
'import-noarticle'           => 'Ei tuotavaa sivua.',
'import-nonewrevisions'      => 'Kaikki versiot on tuotu aiemmin.',
'xml-error-string'           => '$1 rivillä $2, sarakkeessa $3 (tavu $4): $5',
'import-upload'              => 'Tallenna XML-tiedosto',

# Import log
'importlogpage'                    => 'Tuontiloki',
'importlogpagetext'                => 'Loki toisista wikeistä tuoduista sivuista.',
'import-logentry-upload'           => 'toi sivun ”[[$1]]” lähettämällä tiedoston',
'import-logentry-upload-detail'    => '{{PLURAL:$1|yksi versio|$1 versiota}}',
'import-logentry-interwiki'        => 'toi toisesta wikistä sivun ”$1”',
'import-logentry-interwiki-detail' => '{{PLURAL:$1|yksi versio|$1 versiota}} wikistä $2',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'Oma käyttäjäsivu',
'tooltip-pt-anonuserpage'         => 'IP-osoitteesi käyttäjäsivu',
'tooltip-pt-mytalk'               => 'Oma keskustelusivu',
'tooltip-pt-anontalk'             => 'Keskustelu tämän IP-osoitteen muokkauksista',
'tooltip-pt-preferences'          => 'Omat asetukset',
'tooltip-pt-watchlist'            => 'Lista sivuista, joiden muokkauksia tarkkailet',
'tooltip-pt-mycontris'            => 'Lista omista muokkauksista',
'tooltip-pt-login'                => 'Kirjaudu sisään tai luo tunnus',
'tooltip-pt-anonlogin'            => 'Kirjaudu sisään tai luo tunnus',
'tooltip-pt-logout'               => 'Kirjaudu ulos',
'tooltip-ca-talk'                 => 'Keskustele sisällöstä',
'tooltip-ca-edit'                 => 'Muokkaa tätä sivua',
'tooltip-ca-addsection'           => 'Lisää kommentti tälle sivulle',
'tooltip-ca-viewsource'           => 'Näytä sivun lähdekoodi',
'tooltip-ca-history'              => 'Sivun aikaisemmat versiot',
'tooltip-ca-protect'              => 'Suojaa tämä sivu',
'tooltip-ca-delete'               => 'Poista tämä sivu',
'tooltip-ca-undelete'             => 'Palauta tämä sivu',
'tooltip-ca-move'                 => 'Siirrä tämä sivu',
'tooltip-ca-watch'                => 'Lisää tämä sivu tarkkailulistallesi',
'tooltip-ca-unwatch'              => 'Poista tämä sivu tarkkailulistaltasi',
'tooltip-search'                  => 'Etsi {{GRAMMAR:elative|{{SITENAME}}}}',
'tooltip-search-go'               => 'Siirry sivulle, joka on tarkalleen tällä nimellä',
'tooltip-search-fulltext'         => 'Etsi sivuilta tätä tekstiä',
'tooltip-p-logo'                  => 'Etusivu',
'tooltip-n-mainpage'              => 'Mene etusivulle',
'tooltip-n-portal'                => 'Keskustelua projektista',
'tooltip-n-currentevents'         => 'Taustatietoa tämänhetkisistä tapahtumista',
'tooltip-n-recentchanges'         => 'Lista tuoreista muutoksista',
'tooltip-n-randompage'            => 'Avaa satunnainen sivu',
'tooltip-n-help'                  => 'Ohjeita',
'tooltip-t-whatlinkshere'         => 'Lista sivuista, jotka viittaavat tänne',
'tooltip-t-recentchangeslinked'   => 'Viimeisimmät muokkaukset sivuissa, joille viitataan tältä sivulta',
'tooltip-feed-rss'                => 'RSS-syöte tälle sivulle',
'tooltip-feed-atom'               => 'Atom-syöte tälle sivulle',
'tooltip-t-contributions'         => 'Näytä lista tämän käyttäjän muokkauksista',
'tooltip-t-emailuser'             => 'Lähetä sähköpostia tälle käyttäjälle',
'tooltip-t-upload'                => 'Tallenna tiedostoja',
'tooltip-t-specialpages'          => 'Näytä toimintosivut',
'tooltip-t-print'                 => 'Tulostettava versio',
'tooltip-t-permalink'             => 'Ikilinkki sivun tähän versioon',
'tooltip-ca-nstab-main'           => 'Näytä sisältösivu',
'tooltip-ca-nstab-user'           => 'Näytä käyttäjäsivu',
'tooltip-ca-nstab-media'          => 'Näytä mediasivu',
'tooltip-ca-nstab-special'        => 'Tämä on toimintosivu',
'tooltip-ca-nstab-project'        => 'Näytä projektisivu',
'tooltip-ca-nstab-image'          => 'Näytä tiedostosivu',
'tooltip-ca-nstab-mediawiki'      => 'Näytä järjestelmäviesti',
'tooltip-ca-nstab-template'       => 'Näytä malline',
'tooltip-ca-nstab-help'           => 'Näytä ohjesivu',
'tooltip-ca-nstab-category'       => 'Näytä luokkasivu',
'tooltip-minoredit'               => 'Merkitse tämä pieneksi muutokseksi',
'tooltip-save'                    => 'Tallenna muokkaukset',
'tooltip-preview'                 => 'Esikatsele muokkausta ennen tallennusta',
'tooltip-diff'                    => 'Näytä tehdyt muutokset',
'tooltip-compareselectedversions' => 'Vertaile valittuja versioita',
'tooltip-watch'                   => 'Lisää tämä sivu tarkkailulistaan',
'tooltip-recreate'                => 'Luo sivu uudelleen',
'tooltip-upload'                  => 'Aloita tallennus',

# Stylesheets
'common.css'   => '/* Tämä sivu sisältää koko sivustoa muuttavia tyylejä. */',
'monobook.css' => '/* Tämä sivu sisältää Monobook-ulkoasua muuttavia tyylejä. */',

# Scripts
'common.js'   => '/* Tämän sivun koodi liitetään jokaiseen sivulataukseen */',
'monobook.js' => '/* Tämän sivun JavaScript-koodi liitetään MonoBook-tyyliin */',

# Metadata
'nodublincore'      => 'Dublin Core RDF-metatieto on poissa käytöstä tällä palvelimella.',
'nocreativecommons' => 'Creative Commonsin RDF-metatieto on poissa käytöstä tällä palvelimella.',
'notacceptable'     => 'Wikipalvelin ei voi näyttää tietoja muodossa, jota ohjelmasi voisi lukea.',

# Attribution
'anonymous'        => '{{GRAMMAR:genitive|{{SITENAME}}}} anonyymit käyttäjät',
'siteuser'         => '{{GRAMMAR:genitive|{{SITENAME}}}} käyttäjä $1',
'lastmodifiedatby' => 'Tätä sivua muokkasi viimeksi ”$3” $2 kello $1.', # $1 date, $2 time, $3 user
'othercontribs'    => 'Perustuu työlle, jonka teki $1.',
'others'           => 'muut',
'siteusers'        => '{{GRAMMAR:genitive|{{SITENAME}}}} käyttäjä(t) $1',
'creditspage'      => 'Sivun tekijäluettelo',
'nocredits'        => 'Tämän sivun tekijäluettelotietoja ei löydy.',

# Spam protection
'spamprotectiontitle' => 'Mainossuodatin',
'spamprotectiontext'  => 'Mainossuodatin on estänyt sivun tallentamisen. Syynä on todennäköisimmin mustalistattu ulkopuoliselle sivustolle osoittava linkki.',
'spamprotectionmatch' => 'Teksti, joka ei läpäissyt mainossuodatinta: $1',
'spambot_username'    => 'MediaWikin mainospoistaja',
'spam_reverting'      => 'Palautettu viimeisimpään versioon, joka ei sisällä linkkejä kohteeseen $1.',
'spam_blanking'       => 'Kaikki versiot sisälsivät linkkejä kohteeseen $1. Sivu tyhjennetty.',

# Info page
'infosubtitle'   => 'Tietoja sivusta',
'numedits'       => 'Sivun muokkausten määrä: $1',
'numtalkedits'   => 'Keskustelusivun muokkausten määrä: $1',
'numwatchers'    => 'Tarkkailijoiden määrä: $1',
'numauthors'     => 'Sivun erillisten kirjoittajien määrä: $1',
'numtalkauthors' => 'Keskustelusivun erillisten kirjoittajien määrä: $1',

# Math options
'mw_math_png'    => 'Näytä aina PNG:nä',
'mw_math_simple' => 'Näytä HTML:nä, jos yksinkertainen, muuten PNG:nä',
'mw_math_html'   => 'Näytä HTML:nä, jos mahdollista, muuten PNG:nä',
'mw_math_source' => 'Näytä TeX-muodossa (tekstiselaimille)',
'mw_math_modern' => 'Suositus nykyselaimille',
'mw_math_mathml' => 'Näytä MathML:nä jos mahdollista (kokeellinen)',

# Patrolling
'markaspatrolleddiff'                 => 'Merkitse tarkastetuksi',
'markaspatrolledtext'                 => 'Merkitse muokkaus tarkastetuksi',
'markedaspatrolled'                   => 'Tarkastettu',
'markedaspatrolledtext'               => 'Valittu versio on tarkastettu.',
'rcpatroldisabled'                    => 'Tuoreiden muutosten tarkastustoiminto ei ole käytössä',
'rcpatroldisabledtext'                => 'Tuoreiden muutosten tarkastustoiminto ei ole käytössä.',
'markedaspatrollederror'              => 'Muutoksen merkitseminen tarkastetuksi epäonnistui.',
'markedaspatrollederrortext'          => 'Tarkastetuksi merkittävää versiota ei ole määritelty.',
'markedaspatrollederror-noautopatrol' => 'Et voi merkitä omia muutoksiasi tarkastetuiksi.',

# Patrol log
'patrol-log-page'   => 'Muutostentarkastusloki',
'patrol-log-header' => 'Tämä on loki tarkistetuista muutoksista.',
'patrol-log-line'   => 'merkitsi sivun $2 muutoksen $1 tarkastetuksi $3',
'patrol-log-auto'   => '(automaattinen)',

# Image deletion
'deletedrevision'                 => 'Poistettiin vanha versio $1',
'filedeleteerror-short'           => 'Tiedoston $1 poistaminen epäonnistui',
'filedeleteerror-long'            => 'Tiedoston poistaminen epäonnistui:

$1',
'filedelete-missing'              => 'Tiedosto $1 ei voitu poistaa, koska sitä ei ole olemassa.',
'filedelete-old-unregistered'     => 'Tiedoston version $1 ei ole tietokannassa.',
'filedelete-current-unregistered' => 'Tiedosto $1 ei ole tietokannassa.',
'filedelete-archive-read-only'    => 'Arkistohakemistoon ”$1” kirjoittaminen epäonnistui.',

# Browsing diffs
'previousdiff' => '← Vanhempi muutos',
'nextdiff'     => 'Uudempi muutos →',

# Media information
'mediawarning'         => "'''Varoitus''': Tämä tiedosto saattaa sisältää vahingollista koodia, ja suorittamalla sen järjestelmäsi voi muuttua epäluotettavaksi.<hr />",
'imagemaxsize'         => 'Kuvien enimmäiskoko kuvaussivuilla',
'thumbsize'            => 'Pikkukuvien koko',
'widthheightpage'      => '$1×$2, $3 {{PLURAL:$3|sivu|sivua}}',
'file-info'            => '$1, MIME-tyyppi: $2',
'file-info-size'       => '($1×$2 px, $3, MIME-tyyppi: $4)',
'file-nohires'         => '<small>Tarkempaa kuvaa ei ole saatavilla.</small>',
'svg-long-desc'        => '(SVG-tiedosto; oletustarkkuus $1×$2 kuvapistettä; tiedostokoko $3)',
'show-big-image'       => 'Korkeatarkkuuksinen versio',
'show-big-image-thumb' => '<small>Esikatselun koko: $1×$2 px</small>',

# Special:NewImages
'newimages'             => 'Uudet tiedostot',
'imagelisttext'         => 'Alla on {{PLURAL:$1|1 tiedosto|$1 tiedostoa}} lajiteltuna <strong>$2</strong>.',
'newimages-summary'     => 'Tällä toimintosivulla on viimeisimmät tallennetut tiedostot.',
'showhidebots'          => '($1 botit)',
'noimages'              => 'Ei uusia tiedostoja.',
'ilsubmit'              => 'Hae',
'bydate'                => 'päiväyksen mukaan',
'sp-newimages-showfrom' => 'Näytä uudet tiedostot alkaen $1 kello $2',

# Bad image list
'bad_image_list' => 'Listan muoto on seuraava:

Vain *-merkillä alkavat rivit otetaan huomioon. Ensimmäisen linkin on osoitettava arveluttavaan kuvaan. Kaikki muut linkit ovat poikkeuksia eli toisin sanoen sivuja, joissa kuvaa saa käyttää.',

# Metadata
'metadata'          => 'Sisältökuvaukset',
'metadata-help'     => 'Tämä tiedosto sisältää esimerkiksi kuvanlukijan, digikameran tai kuvankäsittelyohjelman lisäämiä lisätietoja. Kaikki tiedot eivät enää välttämättä vastaa todellisuutta, jos kuvaa on muokattu sen alkuperäisen luonnin jälkeen.',
'metadata-expand'   => 'Näytä kaikki sisältökuvaukset',
'metadata-collapse' => 'Näytä vain tärkeimmät sisältökuvaukset',
'metadata-fields'   => 'Seuraavat kentät ovat esillä kuvasivulla, kun sisältötietotaulukko on pienennettynä.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => 'Leveys',
'exif-imagelength'                 => 'Korkeus',
'exif-bitspersample'               => 'Bittiä komponentissa',
'exif-compression'                 => 'Pakkaustapa',
'exif-photometricinterpretation'   => 'Kuvapisteen koostumus',
'exif-orientation'                 => 'Suunta',
'exif-samplesperpixel'             => 'Komponenttien lukumäärä',
'exif-planarconfiguration'         => 'Tiedon järjestely',
'exif-ycbcrsubsampling'            => 'Y:n ja C:n alinäytteistyssuhde',
'exif-ycbcrpositioning'            => 'Y:n ja C:n asemointi',
'exif-xresolution'                 => 'Kuvan resoluutio leveyssuunnassa',
'exif-yresolution'                 => 'Kuvan resoluutio korkeussuunnassa',
'exif-resolutionunit'              => 'Resoluution yksikkö X- ja Y-suunnassa',
'exif-stripoffsets'                => 'Kuvatiedon sijainti',
'exif-rowsperstrip'                => 'Kaistan rivien lukumäärä',
'exif-stripbytecounts'             => 'Tavua pakatussa kaistassa',
'exif-jpeginterchangeformat'       => 'Etäisyys JPEG SOI:hin',
'exif-jpeginterchangeformatlength' => 'JPEG-tiedon tavujen lukumäärä',
'exif-transferfunction'            => 'Siirtofunktio',
'exif-whitepoint'                  => 'Valkoisen pisteen väriarvot',
'exif-primarychromaticities'       => 'Päävärien väriarvot',
'exif-ycbcrcoefficients'           => 'Väriavaruuden muuntomatriisin kertoimet',
'exif-referenceblackwhite'         => 'Musta-valkoparin vertailuarvot',
'exif-datetime'                    => 'Viimeksi muokattu',
'exif-imagedescription'            => 'Kuvan nimi',
'exif-make'                        => 'Kameran valmistaja',
'exif-model'                       => 'Kameran malli',
'exif-software'                    => 'Käytetty ohjelmisto',
'exif-artist'                      => 'Tekijä',
'exif-copyright'                   => 'Tekijänoikeuden omistaja',
'exif-exifversion'                 => 'Exif-versio',
'exif-flashpixversion'             => 'Tuettu Flashpix-versio',
'exif-colorspace'                  => 'Väriavaruus',
'exif-componentsconfiguration'     => 'Kunkin komponentin määritelmä',
'exif-compressedbitsperpixel'      => 'Kuvan pakkaustapa',
'exif-pixelydimension'             => 'Käyttökelpoinen kuvan leveys',
'exif-pixelxdimension'             => 'Käyttökelpoinen kuvan korkeus',
'exif-makernote'                   => 'Valmistajan merkinnät',
'exif-usercomment'                 => 'Käyttäjän kommentit',
'exif-relatedsoundfile'            => 'Liitetty äänitiedosto',
'exif-datetimeoriginal'            => 'Luontipäivämäärä',
'exif-datetimedigitized'           => 'Digitointipäivämäärä',
'exif-subsectime'                  => 'Aikaleiman sekunninosat',
'exif-subsectimeoriginal'          => 'Luontiaikaleiman sekunninosat',
'exif-subsectimedigitized'         => 'Digitointiaikaleiman sekunninosat',
'exif-exposuretime'                => 'Valotusaika',
'exif-exposuretime-format'         => '$1 s ($2)',
'exif-fnumber'                     => 'Aukkosuhde',
'exif-exposureprogram'             => 'Valotusohjelma',
'exif-spectralsensitivity'         => 'Värikirjoherkkyys',
'exif-isospeedratings'             => 'Herkkyys (ISO)',
'exif-oecf'                        => 'Optoelektroninen muuntokerroin',
'exif-shutterspeedvalue'           => 'Suljinaika',
'exif-aperturevalue'               => 'Aukko',
'exif-brightnessvalue'             => 'Kirkkaus',
'exif-exposurebiasvalue'           => 'Valotuksen korjaus',
'exif-maxaperturevalue'            => 'Suurin aukko',
'exif-subjectdistance'             => 'Kohteen etäisyys',
'exif-meteringmode'                => 'Mittaustapa',
'exif-lightsource'                 => 'Valolähde',
'exif-flash'                       => 'Salama',
'exif-focallength'                 => 'Linssin polttoväli',
'exif-subjectarea'                 => 'Kohteen ala',
'exif-flashenergy'                 => 'Salaman teho',
'exif-spatialfrequencyresponse'    => 'Tilataajuusvaste',
'exif-focalplanexresolution'       => 'Tarkennustason X-resoluutio',
'exif-focalplaneyresolution'       => 'Tarkennustason Y-resoluutio',
'exif-focalplaneresolutionunit'    => 'Tarkennustason resoluution yksikkö',
'exif-subjectlocation'             => 'Kohteen sijainti',
'exif-exposureindex'               => 'Valotusindeksi',
'exif-sensingmethod'               => 'Mittausmenetelmä',
'exif-filesource'                  => 'Tiedostolähde',
'exif-scenetype'                   => 'Kuvatyyppi',
'exif-cfapattern'                  => 'CFA-kuvio',
'exif-customrendered'              => 'Muokattu kuvankäsittely',
'exif-exposuremode'                => 'Valotustapa',
'exif-whitebalance'                => 'Valkotasapaino',
'exif-digitalzoomratio'            => 'Digitaalinen suurennoskerroin',
'exif-focallengthin35mmfilm'       => '35 mm:n filmiä vastaava polttoväli',
'exif-scenecapturetype'            => 'Kuvan kaappaustapa',
'exif-gaincontrol'                 => 'Kuvasäätö',
'exif-contrast'                    => 'Kontrasti',
'exif-saturation'                  => 'Värikylläisyys',
'exif-sharpness'                   => 'Terävyys',
'exif-devicesettingdescription'    => 'Laitteen asetuskuvaus',
'exif-subjectdistancerange'        => 'Kohteen etäisyysväli',
'exif-imageuniqueid'               => 'Kuvan yksilöivä tunniste',
'exif-gpsversionid'                => 'GPS-muotoilukoodin versio',
'exif-gpslatituderef'              => 'Pohjoinen tai eteläinen leveysaste',
'exif-gpslatitude'                 => 'Leveysaste',
'exif-gpslongituderef'             => 'Itäinen tai läntinen pituusaste',
'exif-gpslongitude'                => 'Pituusaste',
'exif-gpsaltituderef'              => 'Korkeuden vertailukohta',
'exif-gpsaltitude'                 => 'Korkeus',
'exif-gpstimestamp'                => 'GPS-aika (atomikello)',
'exif-gpssatellites'               => 'Mittaukseen käytetyt satelliitit',
'exif-gpsstatus'                   => 'Vastaanottimen tila',
'exif-gpsmeasuremode'              => 'Mittaustila',
'exif-gpsdop'                      => 'Mittatarkkuus',
'exif-gpsspeedref'                 => 'Nopeuden yksikkö',
'exif-gpsspeed'                    => 'GPS-vastaanottimen nopeus',
'exif-gpstrackref'                 => 'Liikesuunnan vertailukohta',
'exif-gpstrack'                    => 'Liikesuunta',
'exif-gpsimgdirectionref'          => 'Kuvan suunnan vertailukohta',
'exif-gpsimgdirection'             => 'Kuvan suunta',
'exif-gpsmapdatum'                 => 'Käytetty geodeettinen maanmittaustieto',
'exif-gpsdestlatituderef'          => 'Loppupisteen leveysasteen vertailukohta',
'exif-gpsdestlatitude'             => 'Loppupisteen leveysaste',
'exif-gpsdestlongituderef'         => 'Loppupisteen pituusasteen vertailukohta',
'exif-gpsdestlongitude'            => 'Loppupisteen pituusaste',
'exif-gpsdestbearingref'           => 'Loppupisteen suuntiman vertailukohta',
'exif-gpsdestbearing'              => 'Loppupisteen suuntima',
'exif-gpsdestdistanceref'          => 'Loppupisteen etäisyyden vertailukohta',
'exif-gpsdestdistance'             => 'Loppupisteen etäisyys',
'exif-gpsprocessingmethod'         => 'GPS-käsittelymenetelmän nimi',
'exif-gpsareainformation'          => 'GPS-alueen nimi',
'exif-gpsdatestamp'                => 'GPS-päivämäärä',
'exif-gpsdifferential'             => 'GPS-differentiaalikorjaus',

# EXIF attributes
'exif-compression-1' => 'Pakkaamaton',

'exif-unknowndate' => 'Tuntematon päiväys',

'exif-orientation-1' => 'Normaali', # 0th row: top; 0th column: left
'exif-orientation-2' => 'Käännetty vaakasuunnassa', # 0th row: top; 0th column: right
'exif-orientation-3' => 'Käännetty 180°', # 0th row: bottom; 0th column: right
'exif-orientation-4' => 'Käännetty pystysuunnassa', # 0th row: bottom; 0th column: left
'exif-orientation-5' => 'Käännetty 90° vastapäivään ja pystysuunnassa', # 0th row: left; 0th column: top
'exif-orientation-6' => 'Käännetty 90° myötäpäivään', # 0th row: right; 0th column: top
'exif-orientation-7' => 'Käännetty 90° myötäpäivään ja pystysuunnassa', # 0th row: right; 0th column: bottom
'exif-orientation-8' => 'Käännetty 90° vastapäivään', # 0th row: left; 0th column: bottom

'exif-planarconfiguration-1' => 'kokkaremuoto',
'exif-planarconfiguration-2' => 'litteämuoto',

'exif-componentsconfiguration-0' => 'ei ole',

'exif-exposureprogram-0' => 'Ei määritelty',
'exif-exposureprogram-1' => 'Käsinsäädetty',
'exif-exposureprogram-2' => 'Perusohjelma',
'exif-exposureprogram-3' => 'Aukon etuoikeus',
'exif-exposureprogram-4' => 'Suljinajan etuoikeus',
'exif-exposureprogram-5' => 'Luova ohjelma (painotettu syvyysterävyyttä)',
'exif-exposureprogram-6' => 'Toimintaohjelma (painotettu nopeaa suljinaikaa)',
'exif-exposureprogram-7' => 'Muotokuvatila (lähikuviin, joissa tausta on epätarkka)',
'exif-exposureprogram-8' => 'Maisematila (maisemakuviin, joissa tausta on tarkka)',

'exif-subjectdistance-value' => '$1 metriä',

'exif-meteringmode-0'   => 'Tuntematon',
'exif-meteringmode-1'   => 'Keskiarvo',
'exif-meteringmode-2'   => 'Keskustapainotteinen keskiarvo',
'exif-meteringmode-3'   => 'Piste',
'exif-meteringmode-4'   => 'Monipiste',
'exif-meteringmode-5'   => 'Kuvio',
'exif-meteringmode-6'   => 'Osittainen',
'exif-meteringmode-255' => 'Muu',

'exif-lightsource-0'   => 'Tuntematon',
'exif-lightsource-1'   => 'Päivänvalo',
'exif-lightsource-2'   => 'Loisteputki',
'exif-lightsource-3'   => 'Hehkulamppu (keinovalo)',
'exif-lightsource-4'   => 'Salama',
'exif-lightsource-9'   => 'Hyvä sää',
'exif-lightsource-10'  => 'Pilvinen sää',
'exif-lightsource-11'  => 'Varjoinen',
'exif-lightsource-12'  => 'Päivänvaloloisteputki (D 5700 – 7100K)',
'exif-lightsource-13'  => 'Päivänvalkoinen loisteputki (N 4600 – 5400K)',
'exif-lightsource-14'  => 'Kylmä valkoinen loisteputki (W 3900 – 4500K)',
'exif-lightsource-15'  => 'Valkoinen loisteputki (WW 3200 – 3700K)',
'exif-lightsource-17'  => 'Oletusvalo A',
'exif-lightsource-18'  => 'Oletusvalo B',
'exif-lightsource-19'  => 'Oletusvalo C',
'exif-lightsource-24'  => 'ISO-studiohehkulamppu',
'exif-lightsource-255' => 'Muu valonlähde',

'exif-focalplaneresolutionunit-2' => 'tuumaa',

'exif-sensingmethod-1' => 'Määrittelemätön',
'exif-sensingmethod-2' => 'Yksisiruinen värikenno',
'exif-sensingmethod-3' => 'Kaksisiruinen värikenno',
'exif-sensingmethod-4' => 'Kolmisiruinen värikenno',
'exif-sensingmethod-5' => 'Sarjavärikenno',
'exif-sensingmethod-7' => 'Trilineaarikenno',
'exif-sensingmethod-8' => 'Sarjalineaarivärikenno',

'exif-scenetype-1' => 'Suoraan valokuvattu kuva',

'exif-customrendered-0' => 'Normaali käsittely',
'exif-customrendered-1' => 'Muokattu käsittely',

'exif-exposuremode-0' => 'Automaattinen valotus',
'exif-exposuremode-1' => 'Käsinsäädetty valotus',
'exif-exposuremode-2' => 'Automaattinen haarukointi',

'exif-whitebalance-0' => 'Automaattinen valkotasapaino',
'exif-whitebalance-1' => 'Käsinsäädetty valkotasapaino',

'exif-scenecapturetype-0' => 'Perus',
'exif-scenecapturetype-1' => 'Maisema',
'exif-scenecapturetype-2' => 'Henkilökuva',
'exif-scenecapturetype-3' => 'Yökuva',

'exif-gaincontrol-0' => 'Ei ole',
'exif-gaincontrol-1' => 'Matala ylävahvistus',
'exif-gaincontrol-2' => 'Korkea ylävahvistus',
'exif-gaincontrol-3' => 'Matala alavahvistus',
'exif-gaincontrol-4' => 'Korkea alavahvistus',

'exif-contrast-0' => 'Normaali',
'exif-contrast-1' => 'Pehmeä',
'exif-contrast-2' => 'Kova',

'exif-saturation-0' => 'Normaali',
'exif-saturation-1' => 'Alhainen värikylläisyys',
'exif-saturation-2' => 'Korkea värikylläisyys',

'exif-sharpness-0' => 'Normaali',
'exif-sharpness-1' => 'Pehmeä',
'exif-sharpness-2' => 'Kova',

'exif-subjectdistancerange-0' => 'Tuntematon',
'exif-subjectdistancerange-1' => 'Makro',
'exif-subjectdistancerange-2' => 'Lähikuva',
'exif-subjectdistancerange-3' => 'Kaukokuva',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'Pohjoista leveyttä',
'exif-gpslatitude-s' => 'Eteläistä leveyttä',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'Itäistä pituutta',
'exif-gpslongitude-w' => 'Läntistä pituutta',

'exif-gpsstatus-a' => 'Mittaus käynnissä',
'exif-gpsstatus-v' => 'Ristiinmittaus',

'exif-gpsmeasuremode-2' => '2-ulotteinen mittaus',
'exif-gpsmeasuremode-3' => '3-ulotteinen mittaus',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'km/h',
'exif-gpsspeed-m' => 'mailia tunnissa',
'exif-gpsspeed-n' => 'solmua',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => 'Todellinen suunta',
'exif-gpsdirection-m' => 'Magneettinen suunta',

# External editor support
'edit-externally'      => 'Muokkaa tätä tiedostoa ulkoisessa sovelluksessa',
'edit-externally-help' => 'Katso [http://meta.wikimedia.org/wiki/Help:External_editors ohjeet], jos haluat lisätietoja.',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'kaikki',
'imagelistall'     => 'kaikki',
'watchlistall2'    => ', koko historia',
'namespacesall'    => 'kaikki',
'monthsall'        => 'kaikki',

# E-mail address confirmation
'confirmemail'             => 'Varmenna sähköpostiosoite',
'confirmemail_noemail'     => 'Sinulla ei ole kelvollista sähköpostiosoitetta [[Special:Preferences|asetuksissasi]].',
'confirmemail_text'        => 'Tämä wiki vaatii sähköpostiosoitteen varmentamisen, ennen kuin voit käyttää sähköpostitoimintoja. Lähetä alla olevasta painikkeesta varmennusviesti osoitteeseesi. Viesti sisältää linkin, jonka avaamalla varmennat sähköpostiosoitteesi.',
'confirmemail_pending'     => '<div class="error">Varmennusviesti on jo lähetetty. Jos loit tunnuksen äskettäin, odota muutama minuutti viestin saapumista, ennen kuin yrität uudelleen.</div>',
'confirmemail_send'        => 'Lähetä varmennusviesti',
'confirmemail_sent'        => 'Varmennusviesti lähetetty.',
'confirmemail_oncreate'    => 'Varmennusviesti lähetettiin sähköpostiosoitteeseesi. Varmennuskoodia ei tarvita sisäänkirjautumiseen, mutta se täytyy antaa, ennen kuin voit käyttää sähköpostitoimintoja tässä wikissä.',
'confirmemail_sendfailed'  => 'Varmennusviestin lähettäminen epäonnistui. Tarkista, onko osoitteessa kiellettyjä merkkejä.

Postitusohjelma palautti: $1',
'confirmemail_invalid'     => 'Varmennuskoodi ei kelpaa. Koodi on voinut vanhentua.',
'confirmemail_needlogin'   => 'Sinun täytyy $1, jotta voisit varmistaa sähköpostiosoitteesi.',
'confirmemail_success'     => 'Sähköpostiosoitteesi on nyt varmennettu. Voit kirjautua sisään.',
'confirmemail_loggedin'    => 'Sähköpostiosoitteesi on nyt varmennettu.',
'confirmemail_error'       => 'Jokin epäonnistui varmennuksen tallentamisessa.',
'confirmemail_subject'     => '{{GRAMMAR:genitive|{{SITENAME}}}} sähköpostiosoitteen varmennus',
'confirmemail_body'        => 'Joku IP-osoitteesta $1 on rekisteröinyt {{GRAMMAR:inessive|{{SITENAME}}}} tunnuksen $2 tällä sähköpostiosoitteella.

Varmenna, että tämä tunnus kuuluu sinulle avaamalla seuraava linkki selaimellasi:

$3

Jos et ole rekisteröinyt tätä tunnusta, peruuta sähköpostiosoitteen varmistus avaamalla seuraava linkki:

$5

Varmennuskoodi vanhenee $4.',
'confirmemail_invalidated' => 'Sähköpostiosoitteen varmennus peruutettiin',
'invalidateemail'          => 'Sähköpostiosoitteen varmennuksen peruuttaminen',

# Scary transclusion
'scarytranscludedisabled' => '[Wikienvälinen sisällytys ei ole käytössä]',
'scarytranscludefailed'   => '[Mallineen hakeminen epäonnistui: $1]',
'scarytranscludetoolong'  => '[Verkko-osoite on liian pitkä]',

# Trackbacks
'trackbackbox'      => '<div id="mw_trackbacks">Artikkelin trackbackit:<br />$1</div>',
'trackbackremove'   => ' ([$1 poista])',
'trackbacklink'     => 'Trackback',
'trackbackdeleteok' => 'Trackback poistettiin.',

# Delete conflict
'deletedwhileediting' => "'''Varoitus''': Tämä sivu on poistettu sen jälkeen, kun aloitit sen muokkaamisen!",
'confirmrecreate'     => "Käyttäjä '''[[User:$1|$1]]''' ([[User talk:$1|keskustelu]]) on poistanut sivun sen jälkeen, kun aloit muokata sitä. Syy oli:
: ''$2''
Varmista, että haluat luoda sivun uudelleen.",
'recreate'            => 'Luo uudelleen',

'unit-pixel' => ' px',

# HTML dump
'redirectingto' => 'Ohjataan sivulle [[:$1]]...',

# action=purge
'confirm_purge'        => 'Poistetaanko tämän sivun välimuistikopiot?

$1',
'confirm_purge_button' => 'Poista',

# AJAX search
'searchcontaining' => 'Etsi artikkeleita, jotka sisältävät ”$1”.',
'searchnamed'      => 'Etsi artikkeleita, joiden nimi on ”$1”.',
'articletitles'    => 'Artikkelit, jotka alkavat merkkijonolla ”$1”',
'hideresults'      => 'Piilota tulokset',
'useajaxsearch'    => 'Käytä AJAX-hakua',

# Multipage image navigation
'imgmultipageprev' => '← edellinen sivu',
'imgmultipagenext' => 'seuraava sivu →',
'imgmultigo'       => 'Mene!',
'imgmultigoto'     => 'Sivu $1',

# Table pager
'ascending_abbrev'         => 'nouseva',
'descending_abbrev'        => 'laskeva',
'table_pager_next'         => 'Seuraava sivu',
'table_pager_prev'         => 'Edellinen sivu',
'table_pager_first'        => 'Ensimmäinen sivu',
'table_pager_last'         => 'Viimeinen sivu',
'table_pager_limit'        => 'Näytä $1 nimikettä sivulla',
'table_pager_limit_submit' => 'Hae',
'table_pager_empty'        => 'Ei tuloksia',

# Auto-summaries
'autosumm-blank'   => 'Ak: Sivu tyhjennettiin',
'autosumm-replace' => 'Ak: Sivun sisältö korvattiin sisällöllä ”$1”',
'autoredircomment' => 'Ak: Ohjaus sivulle [[$1]]',
'autosumm-new'     => 'Ak: Uusi sivu: $1',

# Size units
'size-kilobytes' => '$1 KiB',
'size-megabytes' => '$1 MiB',
'size-gigabytes' => '$1 GiB',

# Live preview
'livepreview-loading' => 'Ladataan…',
'livepreview-ready'   => 'Ladataan… Valmis!',
'livepreview-failed'  => 'Pikaesikatselu epäonnistui!
Yritä normaalia esikatselua.',
'livepreview-error'   => 'Yhdistäminen epäonnistui: $1 ”$2”
Yritä normaalia esikatselua.',

# Friendlier slave lag warnings
'lag-warn-normal' => 'Muutokset, jotka ovat uudempia kuin $1 {{PLURAL:$1|sekunti|sekuntia}}, eivät välttämättä näy tällä sivulla.',
'lag-warn-high'   => 'Tietokannoilla on työjonoa. Muutokset, jotka ovat uudempia kuin $1 {{PLURAL:$1|sekunti|sekuntia}}, eivät välttämättä näy tällä sivulla.',

# Watchlist editor
'watchlistedit-numitems'       => 'Tarkkailulistallasi on {{PLURAL:$1|yksi sivu|$1 sivua}} keskustelusivuja lukuun ottamatta.',
'watchlistedit-noitems'        => 'Tarkkailulistasi on tyhjä.',
'watchlistedit-normal-title'   => 'Tarkkailulistan muokkaus',
'watchlistedit-normal-legend'  => 'Sivut',
'watchlistedit-normal-explain' => 'Tarkkailulistasi sivut on lueteltu alla. Voit poistaa sivuja valitsemalla niitä vastaavat valintaruudut. Voit myös muokata listaa [[Special:Watchlist/raw|tekstimuodossa]].',
'watchlistedit-normal-submit'  => 'Poista',
'watchlistedit-normal-done'    => '{{PLURAL:$1|Yksi sivu|$1 sivua}} poistettiin tarkkailulistaltasi:',
'watchlistedit-raw-title'      => 'Tarkkailulistan muokkaus',
'watchlistedit-raw-legend'     => 'Tarkkailulistan muokkaus',
'watchlistedit-raw-explain'    => 'Tarkkailulistalla olevat sivut on lueteltu alla jokainen omalla rivillään.',
'watchlistedit-raw-titles'     => 'Sivut',
'watchlistedit-raw-submit'     => 'Päivitä tarkkailulista',
'watchlistedit-raw-done'       => 'Tarkkailulistasi on päivitetty.',
'watchlistedit-raw-added'      => '{{PLURAL:$1|Yksi sivu|$1 sivua}} lisättiin:',
'watchlistedit-raw-removed'    => '{{PLURAL:$1|Yksi sivu|$1 sivua}} poistettiin:',

# Watchlist editing tools
'watchlisttools-view' => 'Näytä muutokset',
'watchlisttools-edit' => 'Muokkaa listaa',
'watchlisttools-raw'  => 'Lista raakamuodossa',

# Core parser functions
'unknown_extension_tag' => 'Tuntematon laajennuskoodi ”$1”.',

# Special:Version
'version'                          => 'Versio', # Not used as normal message but as header for the special page itself
'version-extensions'               => 'Asennetut laajennukset',
'version-specialpages'             => 'Toimintosivut',
'version-parserhooks'              => 'Jäsenninkytkökset',
'version-variables'                => 'Muuttujat',
'version-other'                    => 'Muut',
'version-mediahandlers'            => 'Median käsittelijät',
'version-hooks'                    => 'Kytköspisteet',
'version-extension-functions'      => 'Laajennusfunktiot',
'version-parser-extensiontags'     => 'Jäsentimen laajennustagit',
'version-parser-function-hooks'    => 'Jäsentimen laajennusfunktiot',
'version-skin-extension-functions' => 'Ulkoasun laajennusfunktiot',
'version-hook-name'                => 'Kytköspisteen nimi',
'version-hook-subscribedby'        => 'Kytkökset',
'version-version'                  => 'Versio',
'version-license'                  => 'Lisenssi',
'version-software'                 => 'Asennettu ohjelmisto',
'version-software-product'         => 'Tuote',
'version-software-version'         => 'Versio',

# Special:FilePath
'filepath'         => 'Tiedoston osoite',
'filepath-page'    => 'Tiedosto',
'filepath-submit'  => 'Selvitä osoite',
'filepath-summary' => 'Tämä toimintosivu palauttaa tiedoston URL-osoitteen. Anna tiedoston nimi ilman {{ns:image}}-nimiavaruusliitettä.',

# Special:FileDuplicateSearch
'fileduplicatesearch'          => 'Kaksoiskappaleiden haku',
'fileduplicatesearch-summary'  => 'Etsii tiedoston kaksoiskappaleita hajautusarvon perusteella.

Kirjoita tiedostonimi ilman ”{{ns:image}}:”-etuliitettä.',
'fileduplicatesearch-legend'   => 'Etsi kaksoiskappaleita',
'fileduplicatesearch-filename' => 'Tiedostonimi',
'fileduplicatesearch-submit'   => 'Etsi',
'fileduplicatesearch-info'     => '$1×$2 kuvapistettä<br />Tiedostokoko: $3<br />MIME-tyyppi: $4',
'fileduplicatesearch-result-1' => 'Tiedostolla ”$1” ei ole identtisiä kaksoiskappaleita.',
'fileduplicatesearch-result-n' => 'Tiedostolla ”$1” on {{PLURAL:$2|yksi identtinen kaksoiskappale|$2 identtistä kaksoiskappaletta}}.',

# Special:SpecialPages
'specialpages'                   => 'Toimintosivut',
'specialpages-note'              => '----
* Normaalit toimintosivut.
* <span class="mw-specialpagerestricted">Rajoitetut toimintosivut.</span>',
'specialpages-group-maintenance' => 'Ylläpito',
'specialpages-group-other'       => 'Muut',
'specialpages-group-login'       => 'Kirjautuminen ja tunnusten luonti',
'specialpages-group-changes'     => 'Muutokset ja lokit',
'specialpages-group-media'       => 'Media',
'specialpages-group-users'       => 'Käyttäjät',
'specialpages-group-highuse'     => 'Sivujen käyttöaste',
'specialpages-group-pages'       => 'Sivulistaukset',
'specialpages-group-pagetools'   => 'Sivutyökalut',
'specialpages-group-wiki'        => 'Wikitiedot ja työkalut',
'specialpages-group-redirects'   => 'Ohjaavat toimintosivut',
'specialpages-group-spam'        => 'Mainostenpoistotyökalut',

# Special:BlankPage
'blankpage'              => 'Tyhjä sivu',
'intentionallyblankpage' => 'Tämä sivu on tarkoituksellisesti tyhjä.',

);
