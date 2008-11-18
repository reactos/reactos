<?php
/** Tatar (Latin) (Tatarça/Татарча (Latin))
 *
 * @ingroup Language
 * @file
 *
 * @author Albert Fazlî
 * @author לערי ריינהארט
 */

$namespaceNames = array(
        NS_MEDIA            => 'Media',
        NS_SPECIAL          => 'Maxsus',
        NS_MAIN             => '',
        NS_TALK             => 'Bäxäs',
        NS_USER             => 'Äğzä',
        NS_USER_TALK        => "Äğzä_bäxäse",
        # NS_PROJECT set by $wgMetaNamespace
        NS_PROJECT_TALK     => '$1_bäxäse',
        NS_IMAGE            => "Räsem",
        NS_IMAGE_TALK       => "Räsem_bäxäse",
        NS_MEDIAWIKI        => "MediaWiki",
        NS_MEDIAWIKI_TALK   => "MediaWiki_bäxäse",
        NS_TEMPLATE         => "Ürnäk",
        NS_TEMPLATE_TALK    => "Ürnäk_bäxäse",
        NS_HELP             => "Yärdäm",
        NS_HELP_TALK        => "Yärdäm_bäxäse",
        NS_CATEGORY         => "Törkem",
        NS_CATEGORY_TALK    => "Törkem_bäxäse",
);

$datePreferences = false;

$defaultDateFormat = 'dmy';

$dateFormats = array(
        'mdy time' => 'H:i',
        'mdy date' => 'M j, Y',
        'mdy both' => 'H:i, M j, Y',
        'dmy time' => 'H:i',
        'dmy date' => 'j. M Y',
        'dmy both' => 'j. M Y, H:i',
        'ymd time' => 'H:i',
        'ymd date' => 'Y M j',
        'ymd both' => 'H:i, Y M j',
        'ISO 8601 time' => 'xnH:xni:xns',
        'ISO 8601 date' => 'xnY-xnm-xnd',
        'ISO 8601 both' => 'xnY-xnm-xnd"T"xnH:xni:xns',
);

$magicWords = array(
#       ID                                 CASE  SYNONYMS
        'redirect'               => array( 0,    '#yünältü',                '#REDIRECT'),
        'notoc'                  => array( 0,    '__ETYUQ__',              '__NOTOC__'),
        'forcetoc'               => array( 0,    '__ETTIQ__',              '__FORCETOC__'),
        'toc'                    => array( 0,    '__ET__',                 '__TOC__'),
        'noeditsection'          => array( 0,    '__BÜLEMTÖZÄTÜYUQ__',     '__NOEDITSECTION__'),
        'currentmonth'           => array( 1,    'AĞIMDAĞI_AY',            'CURRENTMONTH'),
        'currentmonthname'       => array( 1,    'AĞIMDAĞI_AY_İSEME',      'CURRENTMONTHNAME'),
        'currentday'             => array( 1,    'AĞIMDAĞI_KÖN',           'CURRENTDAY'),
        'currentdayname'         => array( 1,    'AĞIMDAĞI_KÖN_İSEME',     'CURRENTDAYNAME'),
        'currentyear'            => array( 1,    'AĞIMDAĞI_YIL',           'CURRENTYEAR'),
        'currenttime'            => array( 1,    'AĞIMDAĞI_WAQIT',         'CURRENTTIME'),
        'numberofarticles'       => array( 1,    'MÄQÄLÄ_SANI',            'NUMBEROFARTICLES'),
        'currentmonthnamegen'    => array( 1,    'AĞIMDAĞI_AY_İSEME_GEN',  'CURRENTMONTHNAMEGEN'),
        'pagename'               => array( 1,    'BİTİSEME',               'PAGENAME'),
        'namespace'              => array( 1,    'İSEMARA',                'NAMESPACE'),
        'subst'                  => array( 0,    'TÖPÇEK:',                'SUBST:'),
        'img_right'              => array( 1,    'uñda',                   'right'),
        'img_left'               => array( 1,    'sulda',                  'left'),
        'img_none'               => array( 1,    'yuq',                    'none'),
        'int'                    => array( 0,    'EÇKE:',                   'INT:'),
        'sitename'               => array( 1,    'SÄXİFÄİSEME',            'SITENAME'),
        'ns'                     => array( 0,    'İA:',                    'NS:'),
        'localurl'               => array( 0,    'URINLIURL:',              'LOCALURL:'),
        'localurle'              => array( 0,    'URINLIURLE:',             'LOCALURLE:'),
);

$fallback8bitEncoding = "windows-1254";

$linkTrail = '/^([a-zäçğıñöşü“»]+)(.*)$/sDu';

$messages = array(
'skinpreview' => '(Küzläw)',

# Dates
'sunday'    => 'Yäkşämbe',
'monday'    => 'Düşämbe',
'tuesday'   => 'Sişämbe',
'wednesday' => 'Çärşämbe',
'thursday'  => 'Pänceşämbe',
'friday'    => 'Comğa',
'saturday'  => 'Şimbä',
'sun'       => 'Yäk',
'mon'       => 'Düş',
'tue'       => 'Siş',
'wed'       => 'Çär',
'thu'       => 'Pän',
'fri'       => 'Com',
'sat'       => 'Şim',
'january'   => 'Ğínwar',
'february'  => 'Febräl',
'march'     => 'Mart',
'april'     => 'Äpril',
'june'      => 'Yün',
'july'      => 'Yül',
'september' => 'Sentäber',
'october'   => 'Öktäber',
'november'  => 'Nöyäber',
'december'  => 'Dekäber',
'jan'       => 'Ğín',
'apr'       => 'Äpr',
'jun'       => 'Yün',
'jul'       => 'Yül',
'sep'       => 'Sen',
'oct'       => 'Ökt',
'nov'       => 'Nöy',
'dec'       => 'Dek',

# Categories related messages
'pagecategories'  => '{{PLURAL:$1|Cíıntıq|Cíıntıqlar}}',
'category_header' => '«$1» cíıntığınıñ mäqäläläre',
'subcategories'   => 'Eçke cíıntıqlar',

'linkprefix' => '/^(.*?)([a-zäçğıñöşüA-ZÄÇĞİÑÖŞÜ«„]+)$/sDu',

'about'          => 'Turında',
'article'        => 'Eçtälek bite',
'newwindow'      => '(yaña täräzädä açılır)',
'cancel'         => 'Kiräkmi',
'qbfind'         => 'Tap',
'qbbrowse'       => 'Qaraw',
'qbedit'         => 'Üzgärtü',
'qbpageoptions'  => 'Bu bit',
'qbpageinfo'     => 'Eçtälek',
'qbmyoptions'    => 'Bitlärem',
'qbspecialpages' => 'Maxsus bitlär',
'moredotdotdot'  => 'Kübräk...',
'mypage'         => 'Bitem',
'mytalk'         => 'Bäxäsem',
'anontalk'       => 'Bu IP turında bäxäs',
'navigation'     => 'Küçü',
'and'            => 'wä',

'errorpagetitle'    => 'Xata',
'returnto'          => '«$1» bitenä qaytu.',
'tagline'           => "{{SITENAME}}'dan",
'help'              => 'Yärdäm',
'search'            => 'Ezläw',
'searchbutton'      => 'Ezläw',
'go'                => 'Küç',
'searcharticle'     => 'Küç',
'history'           => 'Bit taríxı',
'history_short'     => 'Taríx',
'info_short'        => 'Belem',
'printableversion'  => 'Bastırulı yurama',
'permalink'         => 'Özgermes bey',
'edit'              => 'Üzgärtü',
'editthispage'      => 'Bit üzgärtü',
'delete'            => 'Beter',
'deletethispage'    => 'Beter bu bitne',
'protect'           => 'Yaqla',
'protectthispage'   => 'Yaqla bu bitne',
'unprotect'         => 'İreklä',
'unprotectthispage' => 'İreklä bu biten',
'newpage'           => 'Yaña bit',
'talkpage'          => 'Bit turında bäxäs',
'talkpagelinktext'  => 'tartış',
'specialpage'       => 'Maxsus Bit',
'personaltools'     => 'Şäxes qoralı',
'postcomment'       => 'Yazma qaldıru',
'articlepage'       => 'Eçtälek biten kürü',
'talk'              => 'Bäxäs',
'toolbox'           => 'Äsbäptirä',
'userpage'          => 'Äğzä biten qaraw',
'imagepage'         => 'Räsem biten qaraw',
'viewtalkpage'      => 'Bäxäsen qaraw',
'otherlanguages'    => 'Başqa tellärdä',
'redirectedfrom'    => '(«$1» bitennän yünältelde)',
'lastmodifiedat'    => 'Betniñ soñğı özgerişi $2, $1 bolğan.', # $1 date, $2 time
'protectedpage'     => 'Yaqlanğan bit',
'jumpto'            => 'Küç:',
'jumptosearch'      => 'ezläw',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => '{{SITENAME}} Turında',
'aboutpage'            => 'Project:Turında',
'bugreports'           => 'Xatanamä',
'bugreportspage'       => 'Project:Xata_yomğağı',
'copyright'            => 'Eçtälek $1 buyınça ireşüle.',
'copyrightpagename'    => '{{SITENAME}} qälämxaqı',
'copyrightpage'        => '{{ns:project}}:Qälämxaq',
'currentevents'        => 'Xäzerge waqíğalar',
'currentevents-url'    => 'Project:Xäzerge waqíğalar',
'edithelp'             => 'Üzgärtü xaqında',
'edithelppage'         => 'Help:Üzgärtü',
'faq'                  => 'YBS',
'faqpage'              => 'Project:YBS',
'helppage'             => 'Help:Eçtälek',
'mainpage'             => 'Täwge Bit',
'mainpage-description' => 'Täwge Bit',
'portal'               => 'Cämğiät üzäge',
'portal-url'           => 'Project:Cämğiät Üzäge',

'retrievedfrom'   => 'Bu bitneñ çığanağı: "$1"',
'newmessageslink' => 'yaña xäbär',
'editsection'     => 'üzgärtü',
'editold'         => 'üzgärtü',
'toc'             => 'Eçtälek tezmäse',
'showtoc'         => 'kürsät',
'hidetoc'         => 'yäşer',
'thisisdeleted'   => 'Qaraw/torğızu: $1',
'feedlinks'       => 'Tasma:',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Mäqälä',
'nstab-user'      => 'Äğzä bite',
'nstab-media'     => 'Media bite',
'nstab-special'   => 'Maxsus',
'nstab-project'   => 'Proyekt bite',
'nstab-image'     => 'Räsem',
'nstab-mediawiki' => 'Sätir',
'nstab-template'  => 'Äzerlämä',
'nstab-help'      => 'Yärdäm',
'nstab-category'  => 'Cíıntıq',

# Main script and global functions
'nosuchaction'      => 'Andí ğämäl barlıqta yuq',
'nosuchspecialpage' => 'Andí maxsus bit yuq',

# General errors
'error'           => 'Xata',
'databaseerror'   => 'Biremlek xatası',
'readonly'        => 'Biremlek yabılğan ide',
'internalerror'   => 'Eçke xata',
'filecopyerror'   => 'Bu «$1» biremen «$2» iseme belän küpli almím.',
'filerenameerror' => 'Bu «$1» biremen «$2» iseme belän küçerä almím.',
'filedeleteerror' => 'Bu «$1» biremen beterep bulmí.',
'filenotfound'    => 'Bu «$1» biremen tabalmím.',
'formerror'       => 'Xata: formını künderä almím',
'badtitle'        => 'Yaraqsız başlıq',
'perfdisabled'    => 'Kiçer! Biremlekneñ äkren buluına säbäple, bu mömkinlek waqıtlıça sünderelgän ide.',
'perfcached'      => 'Astağı belem alxäterdän alındı wä anıñ xäzerge xäl belän turı kilmäwe bar:',
'perfcachedts'    => '$1 çağında bolğan torış asılında yasalğan tizme bo.',
'viewsource'      => 'Mäqälä çığanağı',

# Login and logout pages
'logouttitle'           => 'Äğzä çığuı',
'welcomecreation'       => "== Räxim it, $1! ==

Sineñ xísabıñ yasaldı. {{SITENAME}}'dağı köyläwläreñne dä üzgärtergä onıtma.",
'loginpagetitle'        => 'Atama belän kerü',
'yourname'              => 'İreşü isemeñ',
'yourpassword'          => 'Sersüzeñ',
'yourpasswordagain'     => 'Sersüz qabat',
'remembermypassword'    => 'Tanı mine kergändä.',
'loginproblem'          => '<b>Kerüeñ waqıtında nindider qíınlıq bulıp çıqtı.</b><br />Qabat kerep qara!',
'login'                 => 'İreşü',
'userlogin'             => 'Xísap yasaw yä ki kerü',
'logout'                => 'Çığış',
'userlogout'            => 'Çığış',
'notloggedin'           => 'Kermädeñ äle',
'createaccount'         => 'Yaña xísap yasaw',
'createaccountmail'     => 'email buyınça',
'badretype'             => 'Kertelgän sersüzeñ kileşmi.',
'userexists'            => 'Äle genä kertkäneñ äğzä iseme qullanıla inde. Başqa isem sayla zínhar.',
'youremail'             => "Email'ıñ*",
'yourrealname'          => 'Çın isemeñ*',
'yournick'              => 'Atamañ:',
'loginerror'            => 'Kerü xatası',
'loginsuccesstitle'     => 'Uñışlı kergänbez',
'loginsuccess'          => "Sin {{SITENAME}}'ğa «$1» atama belän kergän buldıñ.",
'wrongpassword'         => 'Sin kertän sersüz xatalı axrısı. Tağın kertep qara zínhar.',
'mailmypassword'        => 'Yaña sersüzne xat belän cibär',
'passwordremindertitle' => '{{SITENAME}} sersüz xäterlätkeçe',
'passwordsent'          => 'Yaña sersüz «$1» terkälüendä kertelgän e-mail buyınça cibärelde.
Anı alğaç monda tağın kerep qara.',
'mailerror'             => 'Xat künderü xatası: $1',

# Edit page toolbar
'bold_sample'     => 'Qalın mäten',
'bold_tip'        => 'Qalın mäten',
'italic_sample'   => 'Awışlı mäten',
'italic_tip'      => 'Awışlı mäten',
'link_sample'     => 'Läñker başlığı',
'link_tip'        => 'Eçke läñker',
'extlink_sample'  => 'http://www.example.com läñker başlığı',
'extlink_tip'     => 'Tışqı läñker (alğı http:// quşımtasın onıtma)',
'headline_sample' => 'Başlıq mätene',
'headline_tip'    => '2. däräcäle başlıq',
'math_sample'     => 'Formulnı monda kert',
'math_tip'        => 'İsäpläw formulı (LaTeX)',
'nowiki_sample'   => 'Taqır mäten urnaştıram',
'nowiki_tip'      => 'Wiki-qalıp eşkärtmäskä',
'image_sample'    => 'Mísal.jpg',
'image_tip'       => 'Quşılğan räsem',
'media_sample'    => 'Mísal.mp3',
'sig_tip'         => 'Ímzañ belän zaman/waqıt tamğası',
'hr_tip'          => 'Yatma sızıq (siräk qullan)',

# Edit pages
'summary'            => 'Yomğaq',
'subject'            => 'Ni turında/başlıq',
'minoredit'          => 'Bu waq-töyäk üzgärmä genä',
'watchthis'          => 'Bitne küzätep torası',
'savearticle'        => 'Saqla biremne',
'preview'            => 'Küzläw',
'showpreview'        => 'Qarap alu...',
'blockedtitle'       => 'Qullanuçı tíıldı',
'whitelistedittitle' => 'Üzgärtü öçen, kerü täläp itelä',
'loginreqtitle'      => 'Kerergä Kiräk',
'loginreqlink'       => 'keräse',
'accmailtitle'       => 'Sersüz künderelde.',
'accmailtext'        => "Bu '$1' öçen digän sersüz '$2' adrésına cibärelde.",
'newarticle'         => '(Yaña)',
'clearyourcache'     => "'''İskärmä:''' Saqlawdan soñ, üzgärmälärne kürü öçen browserıñnıñ alxäteren buşatası bar: '''Mozilla:''' click ''reload''(yä ki ''ctrl-r''), '''IE / Opera:''' ''ctrl-f5'', '''Safari:''' ''cmd-r'', '''Konqueror''' ''ctrl-r''.",
'updated'            => '(Yañartıldı)',
'note'               => '<strong>İskärmä:</strong>',
'editing'            => 'Üzgärtü: $1',
'editconflict'       => 'Üzgärtü qíınlığı: $1',
'yourtext'           => 'Mäteneñ',
'storedversion'      => 'Saqlanğan yurama',
'editingold'         => '<strong>KİSÄTMÄ: Sin bu bitneñ iskergän yuramasın üzgärtäsen.
Ägär sin monı saqlísıñ ikän, şul yuramadan soñ yasalğan üzgärmälär yuğalır.</strong>',
'yourdiff'           => 'Ayırmalar',
'longpagewarning'    => "KİSÄTMÄ: Bu bit zurlığı $1 KB; qayber browserlarda 32 KB'tan da zurraq bulğan bitlärne kürsätkändä qíınlıqlar bula.
Zínhar, bu bitneñ wağraq kisäklärgä bülü turında uylap qara.",
'template-protected' => '(yaqlanmış)',

# History pages
'currentrev' => 'Ağımdağı yurama',
'cur'        => 'xäzer',
'next'       => 'kiläse',
'last'       => 'soñğı',

# Diffs
'difference'              => '(Yuramalar ayırması)',
'lineno'                  => '$1. yul:',
'compareselectedversions' => 'Saylanğan yurama çağıştıru',

# Search results
'searchresults'  => 'Ezläw näticäse',
'titlematches'   => 'Mäqälä başlığı kileşä',
'notitlematches' => 'Kileşkän bit başlığı yuq',
'notextmatches'  => 'Kileşkän bit mätene yuq',
'prevn'          => 'uzğan $1',
'nextn'          => 'kiläse $1',
'viewprevnext'   => 'Körsetesi: ($1) ($2) ($3)',
'powersearch'    => 'Ezläw',

# Preferences page
'preferences'           => 'Köyläwem',
'mypreferences'         => 'Köyläwem',
'prefsnologin'          => 'Kermägänseñ',
'qbsettings'            => 'Tiztirä caylawı',
'changepassword'        => 'Sersüz üzgärtü',
'skin'                  => 'Tışlaw',
'dateformat'            => 'Waqıt qalıbı',
'math_failure'          => 'Uqí almadım',
'math_unknown_error'    => 'tanılmağan xata',
'math_unknown_function' => 'tanılmağan funksí',
'math_lexing_error'     => 'nöhü xatası',
'math_syntax_error'     => 'nöhü xatası',
'prefs-misc'            => 'Başqa köyläwlär',
'saveprefs'             => 'Saqla köyläwlärne',
'resetprefs'            => 'Awdar köyläwne',
'oldpassword'           => 'İske sersüz',
'newpassword'           => 'Yaña sersüz',
'retypenew'             => 'Yaña sersüz (qabat)',
'textboxsize'           => 'Mätenqır ülçäme',
'rows'                  => 'Yul:',
'columns'               => 'Buy:',
'searchresultshead'     => 'Ezläw',
'resultsperpage'        => 'Bit sayın näticä sanı',
'recentchangescount'    => 'Soñğı üzgärtmä tezmäsendä başlıq sanı',
'savedprefs'            => 'Köyläwläreñ saqlandı.',
'timezonelegend'        => 'Waqıt quşağı',
'localtime'             => 'Cirle waqıt belän kürsätäse',
'timezoneoffset'        => 'Çigenü',
'servertime'            => 'Serverda xäzerge waqıt',
'guesstimezone'         => 'Browserdan alası',
'defaultns'             => 'Ğädättä bu isemarada ezlise:',
'default'               => 'töpcay',
'files'                 => 'Fayllar',

# Recent changes
'recentchanges'     => 'Soñğı üzgärtmälär',
'recentchangestext' => 'Bu bittä wikidä bulğan iñ soñğı üzgärtmäläre kürsätelä.',
'rcnotefrom'        => 'Asta <b>$2</b> zamanınnan soñ bulğan üzgärtmälär (<b>$1</b> tikle).',
'rclistfrom'        => '$1 zamannan soñ bulğan üzgärtmälär.',
'rcshowhideminor'   => 'kiçi özgeriş $1',
'rcshowhidebots'    => 'bot $1',
'rcshowhideliu'     => 'tanılğanın $1',
'rcshowhideanons'   => 'tanılmağanın $1',
'rcshowhidemine'    => 'özim özgertkenim $1',
'rclinks'           => 'Soñğı $2 kön eçendä bulğan $1 üzgärtmä<br />$3',
'diff'              => 'ayırma',
'hist'              => 'taríx',
'hide'              => 'yäşer',
'show'              => 'kürsät',
'minoreditletter'   => 'w',
'newpageletter'     => 'Y',

# Recent changes linked
'recentchangeslinked'       => 'Bäyle üzgärmä',
'recentchangeslinked-title' => '$1 bilen beyli özgeriş',

# Upload
'upload'            => 'Birem yökläw',
'uploadbtn'         => 'Yöklä biremne',
'reupload'          => 'Qabat yökläw',
'reuploaddesc'      => 'Yökläw bitenä qaytu.',
'uploadnologin'     => 'Kermädeñ',
'uploadnologintext' => 'Birem yökläw öçen, säxifägä isem belän [[Special:UserLogin|keräse]].',
'uploaderror'       => 'Yökläw xatası',
'uploadlog'         => 'yökläw könlege',
'uploadlogpage'     => 'Yökläw_könlege',
'uploadlogpagetext' => 'Asta soñğı arada yöklängän birem tezmäse kiterelä.',
'filename'          => 'Birem iseme',
'filedesc'          => 'Yomğaq',
'fileuploadsummary' => 'Yomğaq:',
'filestatus'        => 'Qälämxaq xäläte:',
'filesource'        => 'Çığanaq:',
'uploadedfiles'     => 'Yöklängän biremnär',
'ignorewarning'     => 'Kisätmägä qaramíçı biremne härxäldä saqla.',
'badfilename'       => 'Räsem iseme «$1» itep üzgärtelde.',
'successfulupload'  => 'Yökläw uñışlı uzdı',
'uploadwarning'     => 'Yökläw kisätmäse',
'savefile'          => 'Saqla biremne',
'uploadedimage'     => 'yöklände "$1"',
'uploaddisabled'    => 'Ğafu it, yökläw sünderelgän kileş tora.',
'uploadcorrupt'     => 'Bu birem yä üze watıq, yä quşımtası yaraqsız. Birem tikşerüdän soñ qabat yöklä zínhar.',

# Special:ImageList
'imgfile'   => 'fayl',
'imagelist' => 'Räsem tezmäse',

# Image description page
'filehist-dimensions' => 'Ölçemi',
'filehist-comment'    => 'Açıqlama',
'imagelinks'          => 'Räsem läñkerläre',

# File reversion
'filerevert-comment' => 'Açıqlama:',

# File deletion
'filedelete-submit'           => 'Bitir',
'filedelete-reason-otherlist' => 'Başqa sebep',
'filedelete-reason-dropdown'  => '*Bitirirge töp sebep
** Qelemxaqq bozılışı
** Qabatlanğan berim',

# Unused templates
'unusedtemplates' => 'Totılmağan örçitme',

# Random page
'randompage' => 'Berär bit kürü',

# Statistics
'statistics' => 'Nöfüs',
'sitestats'  => '{{SITENAME}} nöfüse',
'userstats'  => 'Qullanuçı nöfüse',

'disambiguations' => 'Saylaqbit tezmäse',

'doubleredirects' => 'Küpmälle yünältü',

'brokenredirects'        => 'Watıq Yünältülär',
'brokenredirectstext'    => 'Kiläse yünältülär bulmağan bitlärgä qarílar.',
'brokenredirects-edit'   => '(özgertiw)',
'brokenredirects-delete' => '(bitir)',

# Miscellaneous special pages
'lonelypages'             => 'Yätim bitlär',
'uncategorizedpages'      => 'Cíıntıqlanmağan bitlär',
'uncategorizedcategories' => 'Cıyıntıqqa salınmağan cıyıntıq',
'uncategorizedimages'     => 'Cıyıntıqqa salınmağan berim',
'uncategorizedtemplates'  => 'Cıyıntıqqa salınmağan örçitme',
'unusedcategories'        => 'Totılmağan cıyıntıq',
'unusedimages'            => 'Qullanılmağan räsemnär',
'popularpages'            => 'Ğämäli bitlär',
'wantedcategories'        => 'Yaratası cıyıntıq tizmesi',
'wantedpages'             => 'Kiräkle bitlär',
'shortpages'              => 'Qısqa bitlär',
'longpages'               => 'Ozın bitlär',
'deadendpages'            => 'Başqa betke beyli bolmağanı',
'listusers'               => 'Äğzä isemlege',
'newpages'                => 'Yaña bitlär',
'ancientpages'            => 'İñ iske bitlär',
'move'                    => 'Küçerü',
'movethispage'            => 'Bu bit küçerü',
'notargettitle'           => 'Maqsatsız',

# Book sources
'booksources' => 'Kitap çığanağı',

# Special:Log
'log'           => 'Köndelikler',
'all-logs-page' => 'Barlıq köndelik',

# Special:AllPages
'allpages'     => 'Bar bitlär',
'nextpage'     => 'Kiläse bit ($1)',
'prevpage'     => 'Ötken bet ($1)',
'allpagesfrom' => 'Bolay başlanğan betler:',

# Special:Categories
'categories'         => 'Cíıntıqlar',
'categoriespagetext' => "Bu wiki'dä kiläse cíıntıqlar bar.",

# Special:ListUsers
'listusers-submit' => 'Körset',

# E-mail user
'emailuser'     => 'E-mail künderü',
'emailpage'     => 'E-mail künderü',
'noemailtitle'  => 'E-mail adres kürsätelmäde',
'emailfrom'     => 'Kemnän',
'emailto'       => 'Kemgä',
'emailsubject'  => 'Ni turında',
'emailmessage'  => 'Xäbär',
'emailsend'     => 'Künder',
'emailsent'     => 'E-mail künderelde',
'emailsenttext' => "E-mail'ıñ künderelde.",

# Watchlist
'watchlist'        => 'Saqtezmäm',
'mywatchlist'      => 'Saqtezmäm',
'nowatchlist'      => 'Saqtezmäñdä kertemnär yuq.',
'watchnologin'     => 'Kermädeñ',
'watchnologintext' => 'Saqtezmäñ üzgärtü öçen, säxifägä isem belän [[Special:UserLogin|keräse]].',
'addedwatch'       => 'Saqtezmägä quşıldı',
'removedwatch'     => 'Saqtezmädän salındı',
'removedwatchtext' => '«[[:$1]]» atlı bit saqtezmäñnän töşerelde.',
'watch'            => 'Saqlaw',
'watchthispage'    => 'Bitne küzätep torası',
'notanarticle'     => 'Eçtälek belän bit tügel',

# Delete/protect/revert
'deletepage'            => 'Beter bitne',
'confirm'               => 'Raslaw',
'excontentauthor'       => "soñğı içteligi: '$1' ('[[Special:Contributions/$2|$2]]' ğına qatnaşqan)",
'exblank'               => 'bit buş ide',
'delete-confirm'        => '«$1» bitiriw',
'delete-legend'         => 'Beterü',
'historywarning'        => 'Íğtíbar: Beterergä telägän biteneñ üz taríxı bar:',
'actioncomplete'        => 'Ğämäl tämam',
'deletedtext'           => '«<nowiki>$1</nowiki>» beterelgän buldı.
Soñğı beterülär $2 bitendä terkälenä.',
'deletedarticle'        => '«$1» beterelde',
'dellogpage'            => 'Beterü_köndälege',
'deletionlog'           => 'beterü köndälege',
'reverted'              => 'Aldağı yuramanı qaytart',
'deletecomment'         => 'Beterü säbäbe',
'deleteotherreason'     => 'Başqa/östeme sebep:',
'deletereasonotherlist' => 'Başqa sebep',
'deletereason-dropdown' => '*Bitirirge töp sebep
** Yazğanı soradı
** Qelemxaqq bozılışı
** Bozıp yöriwçi işi',
'editcomment'           => 'Bu üzgärtü taswírı: "<i>$1</i>".', # only shown if there is an edit comment
'protectlogpage'        => 'Yaqlaw_köndälege',
'protectedarticle'      => '[[$1]] yaqlandı',
'unprotectedarticle'    => '[[$1]] ireklände',
'protect-title'         => '«$1» yaqlaw',
'protect-legend'        => 'Yaqlawnı raslaw',
'protectcomment'        => 'Yaqlaw säbäbe',
'protectexpiry'         => 'Eski bolaçaq:',

# Undelete
'undelete'         => 'Beterelgän bit torğızu',
'undeletebtn'      => 'Torğız!',
'undeletedarticle' => '«$1» torğızıldı',

# Namespace form on various pages
'namespace'      => 'At-alan:',
'invert'         => 'Saylanışnı keri et',
'blanknamespace' => '(Töp)',

# Contributions
'contributions' => 'Äğzä qatnaşuı',
'mycontris'     => 'Qatnaşuım',
'contribsub2'   => '$1 ($2) öçen',
'uctop'         => ' (soñ)',

# What links here
'whatlinkshere' => 'Kem bäyle moña',
'linklistsub'   => '(Läñker tezmäse)',
'isredirect'    => 'küçerelü bite',

# Block/unblock
'blockip'            => 'Qullanuçı tíu',
'ipaddress'          => 'IP Adres/äğzäisem',
'ipbexpiry'          => 'İskerer',
'ipbreason'          => 'Säbäp',
'ipbsubmit'          => 'Bu keşene tíu',
'badipaddress'       => 'Xatalı IP adrésı',
'blockipsuccesssub'  => 'Tíu uzdı',
'unblockip'          => 'Äğzäne irekläw',
'ipusubmit'          => 'Bu adresnı irekläw',
'ipblocklist'        => 'Tíılğan IP/äğzä tezmäse',
'infiniteblock'      => 'eytilmegen',
'blocklink'          => 'tíu',
'contribslink'       => 'qatnaşuı',
'blocklogpage'       => 'Tíu_köndälege',
'ipb_expiry_invalid' => 'İskärü waqıtı xatalı.',
'ip_range_invalid'   => 'Xatalı IP arası.',
'proxyblocker'       => 'Proxy tíu',
'proxyblocksuccess'  => 'Buldı.',

# Developer tools
'lockdb'              => 'Biremlekne yozaqlaw',
'unlockdb'            => 'Biremlek irekläw',
'lockconfirm'         => 'Äye, min biremlekne çınlap ta yozaqlarğa buldım.',
'lockbtn'             => 'Biremlekne yozaqlaw',
'lockdbsuccesssub'    => 'Biremlek yözaqlandı',
'unlockdbsuccesssub'  => 'Biremlek yozağı salındı',
'unlockdbsuccesstext' => 'Bu biremlek yozağı salınğan ide.',

# Move page
'move-page-legend' => 'Bit küçerü',
'movearticle'      => 'Küçeräse bit',
'newtitle'         => 'Yaña başlıq',
'movepagebtn'      => 'Küçer bitne',
'pagemovedsub'     => 'Küçerü uñışlı uzdı',
'articleexists'    => 'Andí atlı bit bar inde,
yä isä saylanğan isem yaraqsız buldı. Başqa isem sayla zínhar.',
'movedto'          => 'küçerelde:',
'movetalk'         => 'Mömkin bulsa, «bäxäs» biten dä küçer.',
'1movedto2'        => '$1 moña küçte: $2',
'1movedto2_redir'  => '$1 moña küçte: $2 (yünältü aşa)',

# Namespace 8 related
'allmessages'        => 'Säxifäneñ bar sätirläre',
'allmessagesname'    => 'Atalış',
'allmessagesdefault' => 'Töpcay yazma',
'allmessagescurrent' => 'Eligi yazma',
'allmessagestext'    => 'Bu säxifäneñ MediaWiki: atarasında bulğan yazmalar tezmäse.',
'allmessagesfilter'  => 'Yazma atalışına sözgiç:',

# Thumbnails
'thumbnail-more' => 'Zuraytası',

# Special:Import
'import'        => 'Bitlärne yökläw',
'importfailed'  => 'Yökläw xatası: $1',
'importnotext'  => 'Buş yä ki mäten tügel',
'importsuccess' => 'Yökläw uñışlı buldı!',

# Tooltip help for the actions
'tooltip-pt-userpage'        => 'Şäxsi bitem',
'tooltip-pt-mytalk'          => 'Bäxäs bitem',
'tooltip-pt-preferences'     => 'Köyläwlärem',
'tooltip-pt-mycontris'       => 'Qatnaşuım tezmäse',
'tooltip-pt-logout'          => 'Çığış',
'tooltip-ca-addsection'      => 'Bu bäxästä yazma östäw.',
'tooltip-ca-viewsource'      => 'Bu bit yaqlanğan ide. Anıñ çığanağın kürä alasıñ.',
'tooltip-ca-history'         => 'Bu bitneñ soñğı yuramaları.',
'tooltip-ca-protect'         => 'Bu bit yaqlaw',
'tooltip-ca-delete'          => 'Bu bit beterü',
'tooltip-ca-move'            => 'Bu bit küçerü',
'tooltip-ca-watch'           => 'Bu bitne saqtezmägä östäw',
'tooltip-ca-unwatch'         => 'Bu bitne saqtezmädän töşerü',
'tooltip-search'             => 'Äydä, ezlä monı',
'tooltip-p-logo'             => 'Täwge Bit',
'tooltip-n-mainpage'         => 'Täwge Bitkä küçü',
'tooltip-n-randompage'       => 'Berär nindi bit kürsätä',
'tooltip-feed-rss'           => 'Bu bitneñ RSS tasması',
'tooltip-feed-atom'          => 'Bu bitneñ Atom tasması',
'tooltip-t-specialpages'     => 'Bar maxsus bitlär tezmäse',
'tooltip-ca-nstab-main'      => 'Bu bit eçtälegen kürü',
'tooltip-ca-nstab-user'      => 'Bu äğzä biten kürü',
'tooltip-ca-nstab-media'     => 'Bu media biten kürü',
'tooltip-ca-nstab-special'   => 'Bu bit maxsus, wä sin anı üzgärtä almísıñ.',
'tooltip-ca-nstab-project'   => 'Proékt biten kürü',
'tooltip-ca-nstab-image'     => 'Bu räsem biten kürü',
'tooltip-ca-nstab-mediawiki' => 'Bu säxifä sätiren kürü',
'tooltip-ca-nstab-template'  => 'Bu qalıpnı kürü',
'tooltip-ca-nstab-help'      => 'Bu yärdäm biten kürü',
'tooltip-minoredit'          => 'Bu üzgärtmä waq-töyäk dip bilgelä',
'tooltip-save'               => 'Üzgärtüne saqlaw',

# Attribution
'anonymous'     => "{{SITENAME}}'nıñ tanılmağan kerüçe",
'siteuser'      => '{{SITENAME}} ägzäse $1',
'othercontribs' => '«$1» eşenä nigezlänä.',
'others'        => 'başqalar',
'siteusers'     => '{{SITENAME}} ägzäse $1',
'creditspage'   => 'Bit yasawında qatnaşqan',

# Spam protection
'spamprotectiontitle' => 'Çüpläwdän saqlanu eläge',

# Info page
'infosubtitle' => 'Bit turında',

# Media information
'show-big-image' => 'Towlı ölçemi',

# Special:NewImages
'ilsubmit' => 'Ezläw',
'bydate'   => 'waqıt buyınça',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'barlıq',

# Multipage image navigation
'imgmultipageprev' => '← ötken bet',
'imgmultipagenext' => 'kelesi bet →',

# Table pager
'table_pager_next'         => 'Kelesi bet',
'table_pager_prev'         => 'Ötken bet',
'table_pager_first'        => 'Birinçi bet',
'table_pager_last'         => 'Soñğı bet',
'table_pager_limit_submit' => 'Eyde',

# Auto-summaries
'autosumm-new' => 'Yeni bet: $1',

# Special:Version
'version' => 'Yurama', # Not used as normal message but as header for the special page itself

# Special:SpecialPages
'specialpages' => 'Maxsus bitlär',

);
