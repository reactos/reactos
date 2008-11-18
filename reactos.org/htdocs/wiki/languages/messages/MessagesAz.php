<?php
/** Azerbaijani (Azərbaycan)
 *
 * @ingroup Language
 * @file
 *
 * @author לערי ריינהארט
 */

$namespaceNames = array(
	NS_MEDIA            => 'Mediya',
	NS_SPECIAL          => 'Xüsusi',
	NS_MAIN             => '',
	NS_TALK             => 'Müzakirə',
	NS_USER             => 'İstifadəçi',
	NS_USER_TALK        => 'İstifadəçi_müzakirəsi',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK     => '$1_müzakirəsi',
	NS_IMAGE            => 'Şəkil',
	NS_IMAGE_TALK       => 'Şəkil_müzakirəsi',
	NS_MEDIAWIKI        => 'MediyaViki',
	NS_MEDIAWIKI_TALK   => 'MediyaViki_müzakirəsi',
	NS_TEMPLATE         => 'Şablon',
	NS_TEMPLATE_TALK    => 'Şablon_müzakirəsi',
	NS_HELP             => 'Kömək',
	NS_HELP_TALK        => 'Kömək_müzakirəsi',
	NS_CATEGORY         => 'Kateqoriya',
	NS_CATEGORY_TALK    => 'Kateqoriya_müzakirəsi',
);

$separatorTransformTable = array(',' => '.', '.' => ',' );

$messages = array(
# User preference toggles
'tog-underline'               => 'Keçidlərin altını xətlə:',
'tog-highlightbroken'         => 'Keçidsiz linkləri <a href="" class="new">bunun kimi</a> (alternetiv: bunun kimi<a href="" class="internal">?</a>) işarətlə.',
'tog-justify'                 => 'Mətni səhifə boyu payla',
'tog-hideminor'               => 'Son dəyişikliklərdə kiçik redaktələri gizlə',
'tog-extendwatchlist'         => 'Təkmil izləmə siyahısı',
'tog-usenewrc'                => 'Son dəyişikliklərin təkmil versiyası (JavaScript)',
'tog-numberheadings'          => 'Başlıqların avto-nömrələnməsi',
'tog-showtoolbar'             => 'Redaktə zamanı alətlər qutusunu göstər (JavaScript)',
'tog-editondblclick'          => 'İki dəfə tıqlamaqla redaktə səhifəsinə keç (JavaScript)',
'tog-editsection'             => 'Hər bir bölmə üçün [redaktə]-ni mümkün et',
'tog-editsectiononrightclick' => 'Bölmələrin redaktəsini başlıqların üzərinə sağ düyməni tıqlamaqla mümkün et (JavaScript)',
'tog-showtoc'                 => 'Mündərəcat siyhəsin göstər (3 başliqdan artix ola səhifələrdə)',
'tog-rememberpassword'        => 'Parolu xatırla',
'tog-editwidth'               => 'Yazma yeri maksimal geniş olsun',
'tog-watchcreations'          => 'Yaratdığım səhifələri izləmə səhifələrimə əlavə et',
'tog-watchdefault'            => 'Redaktə etdiyim səhifələri izləmə səhifələrimə əlavə et',
'tog-watchmoves'              => 'Adlarını dəyişdiyim səhifələri izləmə səhifələrimə əlavə et',
'tog-watchdeletion'           => 'Sildiyim səhifələri izləmə səhifələrimə əlavə et',
'tog-minordefault'            => 'Susmaya görə redaktələri kiçik redaktə kimi nişanla',
'tog-previewontop'            => 'Sınaq göstərişi yazma sahəsindən əvvəl göstər',
'tog-previewonfirst'          => 'İlkin redaktədə sınaq göstərişi',
'tog-nocache'                 => 'Səhifəni keşdə (cache) saxlama',
'tog-fancysig'                => 'Xam imza (daxili bağlantı yaratmaz)',
'tog-externaleditor'          => 'Susmaya görə xarici redaktə proqramlarından istifadə et',
'tog-externaldiff'            => 'Susmaya görə xarici müqayisə proqramlarından istifadə et',
'tog-showjumplinks'           => '"Gətir" ("jump to") linklərini aktivləşdir',
'tog-forceeditsummary'        => 'Qısa məzmunu boş saxladıqda mənə bildir',
'tog-watchlisthideown'        => 'Mənim redaktələrimi izləmə siyahısında gizlət',
'tog-watchlisthidebots'       => 'Bot redaktələrini izləmə siyahısında gizlət',
'tog-watchlisthideminor'      => 'İzləmə səhifəmdə kiçik redaktələri gizlət',
'tog-ccmeonemails'            => 'Göndərdiyim e-məktubun nüsxələrini mənə göndər',
'tog-diffonly'                => 'Versiyaların müqayisəsi zamanı səhifənin məzmununu göstərmə',

'underline-always'  => 'Həmişə',
'underline-never'   => 'Həç zaman',
'underline-default' => 'Susmaya görə brouzer',

'skinpreview' => '(Sınaq göstərişi)',

# Dates
'sunday'        => 'Bazar',
'monday'        => 'Bazar ertǝsi',
'tuesday'       => 'Çǝrşenbǝ axşamı',
'wednesday'     => 'Çǝrşenbǝ',
'thursday'      => 'Cümǝ axşamı',
'friday'        => 'Cümǝ',
'saturday'      => 'Şǝnbǝ',
'sun'           => 'Bazar',
'mon'           => 'Bazar ertəsi',
'tue'           => 'Çərşənbə axşamı',
'wed'           => 'Çərşənbə',
'thu'           => 'Cümə axşamı',
'fri'           => 'Cümə',
'sat'           => 'Şənbə',
'january'       => 'Yanvar',
'february'      => 'Fevral',
'march'         => 'Mart',
'april'         => 'Aprel',
'june'          => 'Iyun',
'july'          => 'Iyul',
'august'        => 'Avqust',
'september'     => 'Sentyabr',
'october'       => 'Oktyabr',
'november'      => 'Noyabr',
'december'      => 'Dekabr',
'january-gen'   => 'yanvar',
'february-gen'  => 'Fevral',
'march-gen'     => 'Mart',
'april-gen'     => 'Aprel',
'june-gen'      => 'İyun',
'july-gen'      => 'İyul',
'august-gen'    => 'Avqust',
'september-gen' => 'Sentyabr',
'october-gen'   => 'Oktyabr',
'november-gen'  => 'noyabr',
'december-gen'  => 'dekabr',
'jan'           => 'Yanvar',
'feb'           => 'Fevral',
'mar'           => 'Mart',
'apr'           => 'Aprel',
'jun'           => 'Iyun',
'jul'           => 'Iyul',
'aug'           => 'Avqust',
'sep'           => 'Sentyabr',
'oct'           => 'Oktyabr',
'nov'           => 'Noyabr',
'dec'           => 'Dekabr',

# Categories related messages
'pagecategories'         => 'Kateqoriyalar',
'category_header'        => '"$1" kategoriyasındaki məqalələr',
'subcategories'          => 'Alt kategoriyalar',
'category-media-header'  => '"$1" kateqoriyasında mediya',
'category-empty'         => "''Bu kateqoriyanın tərkibi hal-hazırda boşdur.''",
'listingcontinuesabbrev' => '(davam)',

'about'          => 'İzah',
'article'        => 'Mündəricat Səhifəsi',
'newwindow'      => '(Yeni pəncərədə açılır)',
'cancel'         => 'Ləğv et',
'qbfind'         => 'Tap',
'qbbrowse'       => 'Gözdən keçir',
'qbedit'         => 'Redaktə',
'qbpageoptions'  => 'Bu səhifə',
'qbpageinfo'     => 'Məzmun',
'qbmyoptions'    => 'Mənim səhifələrim',
'qbspecialpages' => 'Xüsusi səhifələr',
'moredotdotdot'  => 'Daha...',
'mypage'         => 'Mənim səhifəm',
'mytalk'         => 'Danişiqlarım',
'anontalk'       => 'Bu IP-yə aid müzakirə',
'navigation'     => 'Rəhbər',
'and'            => 'və',

'errorpagetitle'    => 'Xəta',
'returnto'          => '$1 səhifəsinə qayıt.',
'help'              => 'Kömək',
'search'            => 'Axtar',
'searchbutton'      => 'Axtar',
'go'                => 'Gətir',
'searcharticle'     => 'Gətir',
'history'           => 'Səhifənin tarixçəsi',
'history_short'     => 'Tarixçə',
'printableversion'  => 'Çap variantı',
'permalink'         => 'Daimi bağlantı',
'print'             => 'Çap',
'edit'              => 'Redaktə',
'create'            => 'Yarat',
'editthispage'      => 'Bu səhifəni redaktə edin',
'create-this-page'  => 'Bu səhifəni yarat',
'delete'            => 'Sil',
'deletethispage'    => 'Bu səhifəni sil',
'protect'           => 'Qoru',
'protect_change'    => 'mühafizəni dəyiş',
'protectthispage'   => 'Bu səhifəni qoru',
'unprotect'         => 'Qorumanı bitir',
'unprotectthispage' => 'Bu səhifəni qoruma',
'newpage'           => 'Yeni səhifə',
'talkpage'          => 'Bu səhifəyi müzakirə et',
'talkpagelinktext'  => 'Müzakirə',
'specialpage'       => 'Xüsusi səhifə',
'personaltools'     => 'Alətlər sandığı',
'articlepage'       => 'Məqaləyə get',
'talk'              => 'Müzakirə',
'toolbox'           => 'Alətlər Sandıqı',
'userpage'          => 'İstifadəçi səhifəsini göstər',
'projectpage'       => 'Layihə səhifəsini göstər',
'imagepage'         => 'Şəkil səhifəsini göstər',
'mediawikipage'     => "Mə'lumat səhifəsini göstər",
'categorypage'      => 'Kateqoriya səhifəsini göstər',
'viewtalkpage'      => 'Müzakirəni göstər',
'otherlanguages'    => 'Başqa dillərdə',
'redirectedfrom'    => '($1 səhifəsindən istiqamətləndirilmişdir)',
'redirectpagesub'   => 'İstiqamətləndirmə səhifəsi',
'lastmodifiedat'    => 'Bu səhifə sonuncu dəfə $2, $1 tarixində redaktə edilib.', # $1 date, $2 time
'protectedpage'     => 'Mühafizəli səhifə',
'jumptonavigation'  => 'naviqasiya',
'jumptosearch'      => 'axtar',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => '{{SITENAME}} haqqında',
'aboutpage'            => 'Project:İzah',
'bugreports'           => "Xəta mə'ruzəsı",
'bugreportspage'       => "Project:Xəta_mə'ruzəsı",
'copyright'            => 'Bu məzmun $1 əhatəsindədir.',
'copyrightpagename'    => '{{SITENAME}} müəllif',
'copyrightpage'        => '{{ns:project}}:Müəllif',
'currentevents'        => 'Güncəl hadisələr',
'currentevents-url'    => 'Project:Güncəl Hadisələr',
'disclaimers'          => 'İmtina etmə',
'edithelp'             => 'Redaktə kömək',
'edithelppage'         => 'Help:Redaktə',
'helppage'             => 'Help:Mündəricət',
'mainpage'             => 'Ana Səhifə',
'mainpage-description' => 'Ana Səhifə',
'portal'               => 'Kənd Meydani',
'portal-url'           => 'Project:Kənd Meydani',
'privacy'              => 'Gizlilik prinsipi',
'privacypage'          => 'Project:Gizlilik_prinsipi',

'versionrequired' => 'MediyaViki $1 versiyası lazımdır',

'youhavenewmessages'      => 'Hal-hazırda $1 var. ($2)',
'newmessageslink'         => 'yeni mesajlar!',
'newmessagesdifflink'     => 'Sonuncu və əvvəlki versiya arasındakı fərq',
'youhavenewmessagesmulti' => '$1-də yeni mesajınız var.',
'editsection'             => 'redaktə',
'editold'                 => 'redaktə',
'editsectionhint'         => 'Bölməsini redaktə: $1',
'toc'                     => 'Mündəricat',
'showtoc'                 => 'göstər',
'hidetoc'                 => 'gizlə',
'thisisdeleted'           => '$1-yə bax və ya bərpa et?',
'viewdeleted'             => 'Göstər $1?',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Məqalə',
'nstab-user'      => 'İstifadəçi səhifəsi',
'nstab-special'   => 'Xüsusi',
'nstab-project'   => 'Layihə səhifəsi',
'nstab-image'     => 'Fayl',
'nstab-mediawiki' => "Mə'lumat",
'nstab-template'  => 'Şablon',
'nstab-help'      => 'Kömək',
'nstab-category'  => 'Kateqoriya',

# General errors
'error'             => 'Xəta',
'databaseerror'     => 'Verilənlər bazası xətası',
'cachederror'       => 'Bu axtardığınız səhifənin keşdə saxlanılmış surətidir və yenilənməmiş ola bilər.',
'readonly'          => 'Verilənlər bazası kilidli',
'internalerror'     => 'Daxili xəta',
'cannotdelete'      => 'İstədiyiniz səhifəni və ya faylı silmək mümkün deyil. (Başqa bir istifadəçi tərəfindən silinmiş ola bilər.)',
'badtitle'          => 'Yanlış başlıq',
'viewsource'        => 'Mənbə göstər',
'viewsourcefor'     => '$1 üçün',
'protectedpagetext' => 'Bu səhifə redaktə üçün bağlıdır.',
'viewsourcetext'    => 'Siz bu səhifənin məzmununu görə və köçürə bilərsiniz:',

# Login and logout pages
'logouttitle'                => 'İstifadəçi çıxış',
'logouttext'                 => "<strong>Sistemdən çıxdınız.</strong><br /> Vikipediyanı anonim olaraq istifadə etməyə davam edəbilər, və ya eyni yaxud başqa istifadəçi adı ilə yenidən daxil ola bilərsiniz. Diqqətinizə çatdırırıq ki, ön yaddaşı (browser cache) təmizləyənə qədər bə'zi səhifələr sistemdən çıxdığınız halda da göstərilə bilər.",
'welcomecreation'            => '== $1, xoş gəlmişsiniz! == Hesabınız yaradıldı. {{SITENAME}} nizamlamalarını dəyişdirməyi unutmayın.',
'loginpagetitle'             => 'İstifadəçi Giriş Səhifəsi',
'yourname'                   => 'İstifadəçi adı',
'yourpassword'               => 'Parol',
'yourpasswordagain'          => 'Parolu təkrar yazın',
'remembermypassword'         => 'Məni xatırla',
'login'                      => 'Daxil ol',
'nav-login-createaccount'    => 'Daxil ol / hesab yarat',
'loginprompt'                => '{{SITENAME}}-ya daxil olmaq üçün "veb kökələrinin" (cookies) istifadəsinə icazə verilməlidir.',
'userlogin'                  => 'Daxil ol və ya istifadəçi yarat',
'logout'                     => 'Çıxış',
'userlogout'                 => 'Çıxış',
'nologin'                    => 'İstifadəçi adınız yoxdursa, $1.',
'nologinlink'                => 'hesab açın',
'createaccount'              => 'Yeni hesab aç',
'gotaccount'                 => 'Giriş hesabınız varsa $1.',
'gotaccountlink'             => 'daxil olun',
'createaccountmail'          => 'e-məktub ilə',
'youremail'                  => 'E-məktub *',
'username'                   => 'İstifadəçi adı:',
'uid'                        => 'İstifadəçi ID:',
'yourrealname'               => 'Həqiqi adınız *',
'yourlanguage'               => 'Dil:',
'yournick'                   => 'Ləqəb:',
'email'                      => 'E-məktub',
'prefs-help-realname'        => '* Həqiqi adınız (qeyri-məcburi): if you choose to provide it this will be used for giving you attribution for your work.',
'loginerror'                 => 'Daxil olunma xətası',
'prefs-help-email'           => '* E-məktub (qeyri-məcburi): Enables others to contact you through your user or user_talk page without the need of revealing your identity.',
'loginsuccesstitle'          => 'Daxil olundu',
'loginsuccess'               => "'''\"\$1\" olaraq {{SITENAME}}-ya daxil oldunuz.'''",
'wrongpassword'              => 'Yanlış parol. Təkrar yaz.',
'wrongpasswordempty'         => 'Parol boş. Təkrar yaz.',
'mailmypassword'             => 'Parolu unutmuşam',
'passwordremindertitle'      => '{{SITENAME}} parol xatırladıcı',
'noemail'                    => '"$1" adlı istifadəçi e-məktub ünvanı qeyd edmemişdir.',
'passwordsent'               => 'Yeni parol "$1" üçün qeydiyyata alınan e-məktub ünvanına göndərilmişdir.
Xahiş edirik, e-məktubu aldıqdan sonra yenidən daxil olasınız.',
'mailerror'                  => 'Məktub göndərmə xətası: $1',
'acct_creation_throttle_hit' => 'Siz artıq $1 hesab açmısınız. Daha çox hesab açabilmərsiniz.',
'emailauthenticated'         => 'E-məktub ünvanınız $1 tarixində təsdiq edilib.',
'emailnotauthenticated'      => 'Your e-mail address is not yet authenticated. No e-mail will be sent for any of the following features.',
'emailconfirmlink'           => 'E-məktubunu təsdiq et',
'invalidemailaddress'        => 'E-məktub ünvanını qeyri düzgün formatda olduğu üçün qəbul edə bilmirik. Xahiş edirik düzgün formatlı ünvan daxil edin və ya bu sahəni boş qoyun.',
'accountcreated'             => 'Hesab yaradıldı',
'accountcreatedtext'         => '$1 üçün istifadəçi hesabı yaradıldı.',

# Password reset dialog
'resetpass_text' => '<!-- Şərhinizi bura daxil edin -->',

# Edit page toolbar
'bold_sample'     => 'Qalın mətn',
'bold_tip'        => 'Qalın mətn',
'italic_sample'   => 'Kursiv mətn',
'italic_tip'      => 'Kursiv mətn',
'link_sample'     => 'Bağlantı başlığı',
'link_tip'        => 'Daxili bağlantı',
'extlink_sample'  => 'http://www.example.com başlıq',
'extlink_tip'     => 'Xarici səhifə (http:// ekini unutma)',
'headline_sample' => 'Başlıq metni',
'headline_tip'    => '2. səviyyə başlıq',
'math_sample'     => 'Riyazi formulu bura yazın',
'nowiki_tip'      => 'Viki formatını sayma',
'image_sample'    => 'Misal.jpg',
'image_tip'       => 'Şəkil əlavə etmə',
'media_sample'    => 'Misal.ogg',
'sig_tip'         => 'İmza və vaxt',
'hr_tip'          => 'Horizontal cizgi',

# Edit pages
'summary'                  => 'Qısa məzmun',
'subject'                  => 'Mövzu/başlıq',
'minoredit'                => 'Kiçik redaktə',
'watchthis'                => 'Bu səhifəni izlə',
'savearticle'              => 'Səhifəni qeyd et',
'preview'                  => 'Sınaq göstərişi',
'showpreview'              => 'Sınaq göstərişi',
'showdiff'                 => 'Dəyişiklikləri göstər',
'blockedtitle'             => 'İstifadəçi bloklanıb',
'blockedoriginalsource'    => "'''$1''' mənbəyi aşağıda göstərilib:",
'whitelistedittitle'       => 'Redaktə üçün daxil olmalısınız',
'confirmedittitle'         => 'Redaktə üçün e-məktub təsdiqi lazımdır',
'loginreqtitle'            => 'Daxil olmalısınız',
'loginreqlink'             => 'Daxil ol',
'accmailtitle'             => 'Parol göndərildi.',
'accmailtext'              => '"$1" üçün parol göndərildi bu ünvana : $2.',
'newarticle'               => '(Yeni)',
'newarticletext'           => "Mövcud olmayan səhifəyə olan keçidi izlədiniz. Aşağıdakı sahəyə məzmununu yazaraq bu səhifəni '''siz''' yarada bilərsiniz. (əlavə məlumat üçün [[{{MediaWiki:Helppage}}|kömək səhifəsinə]] baxın). Əgər bu səhifəyə səhvən gəlmisinizsə sadəcə olaraq brauzerin '''geri''' düyməsinə vurun.",
'anontalkpagetext'         => "----<big>'''''Bu səhifə anonim istifadəçiyə aid müzakirə səhifəsidir. Bu mesaj IP ünvana göndərilmişdir və əgər bu mesajın sizə aid olmadığını düşünürsünüzsə [[Special:Userlogin|qeydiyyatdan keçin]]. Bu zaman sizə yalnız öz fəaliyyətlərinizə görə mesaj gələcəkdir.'''''</big>",
'noarticletext'            => "Hal-hazırda bu səhifə boşdur. Başqa səhifələrdə [[Special:Search/{{PAGENAME}}|bu səhifənin adını axtara]] bilər və ya '''[{{fullurl:{{NAMESPACE}}:{{PAGENAME}}|action=edit}} səhifəni siz redaktə edəbilərsiniz]'''.",
'previewnote'              => '<strong>Bu yalnız sınaq göstərişidir; dəyişikliklər hal-hazırda qeyd edilmemişdir!</strong>',
'session_fail_preview'     => '<strong>Üzr istəyirik! Sizin redaktəniz saxlanılmadı. Serverdə identifikasiyanızla bağlı problemlər yaranmışdır. Lütfən bir daha təkrar edin. Problem həll olunmazsa hesabınızdan çıxın və yenidən daxil olun.</strong>',
'editing'                  => 'Redaktə $1',
'editingsection'           => 'Redaktə $1 (bölmə)',
'editingcomment'           => 'Redaktə $1 (şərh)',
'editconflict'             => 'Eyni vaxtda redaktə: $1',
'yourtext'                 => 'Metniniz',
'storedversion'            => 'Qeyd edilmiş versiya',
'editingold'               => '<strong>DİQQƏT:Siz bu səhifənin köhnə versiyasını redaktə edirsiniz. Məqaləni yaddaşda saxlayacağınız halda bu versiyadan sonra edilmiş hər bir dəyişiklik itiriləcək.</strong>',
'yourdiff'                 => 'Fərqlər',
'longpagewarning'          => '<strong>DIQQƏT:Bu səhifənin həcmi $1 kb-dır; Həcmi 32 kb yaxın və ya daha artıq olan səhifələr bəzi brouzerlərdə redaktə ilə bağlı problemlər yarada bilər. Mümkünsə səhifəni daha kiçik bölmələrə bölün.</strong>',
'semiprotectedpagewarning' => "'''Qeyd:''' Bu səhifə mühafizəli olduğu üçün yalnız qeydiyyatdan keçmiş istifadəçilər redaktə edə bilərlər.",
'titleprotectedwarning'    => '<strong>DİQQƏT:  Bu səhifə mühafizəlidir, yalnız icazəsi olan istifadəçilər onu redaktə edə bilərlər.</strong>',
'templatesused'            => 'Bu səhifədə istifadə edilmiş şablonlar:',
'template-protected'       => '(mühafizə)',
'template-semiprotected'   => '(yarım-mühafizə)',
'permissionserrorstext'    => 'Siz, bunu aşağıdakı {{PLURAL:$1|səbəbə|səbəblərə}} görə edə bilməzsiniz:',
'recreate-deleted-warn'    => "'''Diqqət:Siz əvvəllər silinmiş səhifəni bərpa edirsiniz'''

İlk öncə bu səhifəni redaktə etməyin nə qədər lazımlı olduğunu müəyyənləşdirin
Bu səhifə üçün silmə qeydləri aşağıda göstərilmişdir:",

# History pages
'revnotfound'         => 'Versiya tapıla bilmir',
'revnotfoundtext'     => 'Səhifənin istədiyiniz köhnə versiyası tapıla bilmir.
Xahiş edirik, URL ünvanını yoxlayasınız.',
'currentrev'          => 'Hal-hazırkı versiya',
'revisionasof'        => '$1 versiyası',
'previousrevision'    => '←Əvvəlki versiya',
'nextrevision'        => 'Sonrakı versiya→',
'currentrevisionlink' => 'Hal-hazırkı versiyanı göstər',
'cur'                 => 'hh',
'next'                => 'sonrakı',
'last'                => 'son',
'histlegend'          => 'Fərqləri seçmə və göstərmə: müqaisə etmək istədiyiniz versiyaların yanındakı radio qutularına işarə qoyun və daxil etmə düyməsinə(enter-a) və ya "müqaisə et" düyməsinə vurun.<br />
Açıqlama: (hh) = hal-hazırkı versiya ilə olan fərqlər,
(son) = əvvəlki versiya ilə olan fərqlər, K = kiçik redaktə.',
'deletedrev'          => '[silindi]',
'histfirst'           => 'Ən əvvəlki',
'histlast'            => 'Ən sonuncu',
'historyempty'        => '(boş)',

# Revision feed
'history-feed-title' => 'Redaktə tarixçəsi',

# Revision deletion
'rev-deleted-user' => '(istifadəçi adı silindi)',
'rev-delundel'     => 'göstər/gizlət',

# Diffs
'difference'              => '(Versiyalar arasındakı fərq)',
'lineno'                  => 'Sətir $1:',
'compareselectedversions' => 'Seçilən versiyaları müqaisə et',
'editundo'                => 'əvvəlki halına qaytar',
'diff-multi'              => '({{PLURAL:$1|bir aralıq dəyişiklik|$1 aralıq dəyişiklik}} göstərilməmişdir.)',

# Search results
'searchresults'      => 'Axtarış nəticələri',
'noexactmatch'       => "\"\$1\" başlığı altında məqalə yoxdur. Bu məqaləni özünüz '''[[:\$1|yarada bilərsiniz]]'''.",
'notextmatches'      => 'Məqalələrdə uyğun məzmun tapılmadı',
'prevn'              => 'əvvəlki $1',
'nextn'              => 'sonrakı $1',
'viewprevnext'       => 'Göstər ($1) ($2) ($3).',
'nonefound'          => "'''Qeyd''': Əksərən uğursuz axtarışlara indeksləşdirilməyən, geniş işlənən \"var\", \"və\" tipli sözlər və ya axtarışa bir sözdən artıq söz verildikdə (yalnız məzmununda bütün verilmiş sözlər olan səhifələr göstərilir) səbəb olur.",
'powersearch'        => 'Axtar',
'powersearch-legend' => 'Təkmil axtarış',

# Preferences page
'preferences'           => 'Nizamlamalar',
'mypreferences'         => 'Nizamlamalar',
'prefs-edits'           => 'Redaktələrin sayı:',
'changepassword'        => 'Parol dəyiş',
'skin'                  => 'Üzlük',
'math'                  => 'Riyaziyyat',
'dateformat'            => 'Tarix formatı',
'datedefault'           => 'Tərcih yox',
'datetime'              => 'Tarix və vaxt',
'math_unknown_error'    => 'tanınmayan xəta',
'math_unknown_function' => 'tanınmayan funksiya',
'math_syntax_error'     => 'sintaksis xətası',
'prefs-personal'        => 'İstifadəçi profili',
'prefs-rc'              => 'Son dəyişikliklər',
'prefs-watchlist'       => 'İzləmə siyahısı',
'prefs-watchlist-days'  => 'İzləmə siyahısında göstərilən maksimal günlərin sayı:',
'prefs-watchlist-edits' => 'İzləmə siyahısında göstərilən maksimal redaktələrin sayı:',
'prefs-misc'            => 'Digər tərcihlər',
'saveprefs'             => 'Qeyd et',
'resetprefs'            => 'Reset',
'oldpassword'           => 'Köhne parol:',
'newpassword'           => 'Yeni parol:',
'retypenew'             => 'Yeni parolu təkrar yazın:',
'textboxsize'           => 'Redaktə',
'rows'                  => 'Sıralar:',
'searchresultshead'     => 'Axtar',
'resultsperpage'        => 'Səhifəyə aid tapılmış nəticələr:',
'contextlines'          => 'Nəticələrə aid sıralar:',
'contextchars'          => 'Sıraya aid işarələr:',
'stub-threshold'        => '<a href="#" class="stub">Keçidsiz linki</a> format etmək üçün hüdud (baytlarla):',
'recentchangesdays'     => 'Son dəyişiklərdə göstərilən günlərin miqdarı:',
'recentchangescount'    => 'Son dəyişikliklərdə başlıq sayı:',
'savedprefs'            => 'Tərcihlər qeyd edildi.',
'timezonelegend'        => 'Saat qurşağı',
'timezonetext'          => 'Server ilə vaxt fərqı. (Azərbaycan üçün +04:00)',
'localtime'             => 'Məhəlli vaxt',
'timezoneoffset'        => 'Vaxt fərqı¹',
'servertime'            => 'Server vaxtı',
'guesstimezone'         => 'Brouzerdən götür',
'allowemail'            => 'Digər istifadəçilər mənə e-məktub göndərəbilir',
'defaultns'             => 'Susmaya görə bu ad fəzalarında axtar:',
'files'                 => 'Fayllar',

# User rights
'userrights'               => 'İstifadəçi hüququ idarəsi', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'   => 'İstifadəçi qruplarını idarə et',
'userrights-user-editname' => 'İstifadəçi adınızı yazın:',
'editusergroup'            => 'Redaktə İstifadəçi Qrupları',
'editinguser'              => "Redaktə '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup' => 'İstifadəçi qruplarını redaktə et',
'saveusergroups'           => 'İstifadəçi qrupunu qeyd et',

# Groups
'group'               => 'Qrup:',
'group-user'          => 'İstifadəçilər',
'group-autoconfirmed' => 'Avtotəsdiqlənmiş istifadəçilər',
'group-bot'           => 'Botlar',
'group-sysop'         => 'İdarəçilər',
'group-bureaucrat'    => 'Bürokratlar',
'group-all'           => '(bütün)',

'group-autoconfirmed-member' => 'Avtotəsdiqlənmiş istifadəçilər',
'group-sysop-member'         => 'İdarəçi',
'group-bureaucrat-member'    => 'Bürokrat',

'grouppage-autoconfirmed' => '{{ns:project}}:Avtotəsdiqlənmiş istifadəçilər',
'grouppage-bot'           => '{{ns:project}}:Botlar',
'grouppage-sysop'         => '{{ns:project}}:İdarəçilər',
'grouppage-bureaucrat'    => '{{ns:project}}:Bürokratlar',

# Rights
'right-read'          => 'Səhifələrin oxunması',
'right-edit'          => 'Səhifələrin redaktəsi',
'right-createpage'    => 'Səhifələr yaratmaq (müzakirə səhifələrindən əlavə səhifələr nəzərdə tutulur)',
'right-createtalk'    => 'Müzakirə səhifələri yaratmaq',
'right-createaccount' => 'Yeni istifadəçi hesabları açmaq',
'right-minoredit'     => 'Redaktələri kiçik redaktə kimi nişanlamaq',
'right-reupload'      => 'Mövcud faylın yeni versiyasının yüklənməsi',
'right-reupload-own'  => 'Mövcud faylın yeni versiyasının həmin istifadəçi tərəfindən yüklənməsi',
'right-writeapi'      => 'Redaktələrdən zamanı API-dən (İnterfeys proqramlaşdıran proqram) istifadə',

# Recent changes
'recentchanges'     => 'Son dəyişikliklər',
'recentchangestext' => "'''Ən son dəyişiklikləri bu səhifədən izləyin.'''",
'rcnote'            => 'Aşağıdakı son <strong>$1</strong> dəyişiklik son <strong>$2</strong> gün ərzində edilmişdir.',
'rclistfrom'        => '$1 vaxtından başlayaraq yeni dəyişiklikləri göstər',
'rcshowhideminor'   => 'Kiçik redaktələri $1',
'rcshowhidebots'    => 'Botları $1',
'rcshowhideliu'     => 'Qeyri anonim istifadəçiləri $1',
'rcshowhideanons'   => 'Anonim istifadəçiləri $1',
'rcshowhidepatr'    => 'Nəzarət edilən redaktələri $1',
'rcshowhidemine'    => 'Mənim redaktələrimi $1',
'rclinks'           => 'Son $2 gün ərzindəki son $1 dəyişikliyi göstər <br />$3',
'diff'              => 'fərq',
'hist'              => 'tarixçə',
'hide'              => 'gizlət',
'show'              => 'göstər',
'minoreditletter'   => 'k',
'newpageletter'     => 'Y',
'newsectionsummary' => '/* $1 */ yeni bölmə',

# Recent changes linked
'recentchangeslinked' => 'Əlaqəli redaktələr',

# Upload
'upload'            => 'Qarşıya yüklə',
'uploadbtn'         => 'Sənəd yüklə',
'reupload'          => 'Təkrar yüklə',
'reuploaddesc'      => 'Return to the upload form.',
'uploadnologintext' => 'Fayl yükləmək üçün [[Special:Userlogin|daxil olmalısınız]].',
'uploaderror'       => 'Yükləyiş xətası',
'uploadlog'         => 'yükleme kaydı',
'uploadlogpage'     => 'Yükləmə qeydləri',
'uploadlogpagetext' => 'Aşağıda ən yeni yükləmə jurnal qeydləri verilmişdir.',
'filename'          => 'Fayl adı',
'fileuploadsummary' => 'İzahat:',
'filestatus'        => 'Müəllif statusu:',
'filesource'        => 'Mənbə:',
'uploadedfiles'     => 'Yüklənmiş fayllar',
'ignorewarning'     => 'Xəbərdarlıqlara əhəmiyyət vermə və faylı saxla',
'badfilename'       => 'Faylın adı dəyişildi. Yeni adı: "$1".',
'emptyfile'         => 'Yüklədiyiniz fayl boşdur. Bu faylın adında olan hərf səhvi ilə bağlı ola bilər. Xahiş olunur ki, doğurdan da bu faylı yükləmək istədiyinizi yoxlayasınız.',
'fileexists'        => 'Sizin yükləmək istədiyiniz adda fayl artıq yüklənmişdir. Lütfən <strong><tt>$1</tt></strong> keçidini yoxlayın və bu faylı yükləmək istədiyinizdən əmin olun.',
'fileexists-thumb'  => "<center>'''Mövcud şəkil'''</center>",
'successfulupload'  => 'Yükləmə tamamlandı',
'uploadwarning'     => 'Yükləyiş xəbərdarlıqı',
'savefile'          => 'Faylı qeyd et',
'uploadedimage'     => 'yükləndi "[[$1]]"',
'sourcefilename'    => 'Fayl adı mənbələri',
'destfilename'      => 'Fayl adı',
'watchthisupload'   => 'Bu səhifəni izlə',
'upload-wasdeleted' => "'''Diqqət:Siz əvvəl bu ad altında mövcud olmuş və silinmiş faylı yenidən yükləməkdəsiniz'''

Əvvəlcədən bu faylı yenidən yükləməyin nə dərəcədə lazımlı olduğunu müəyyənləşdirməniz məsləhətdir.
Bu səhifə üçün silmə qeydləri aşağıda göstərilmişdir:",

'license'   => 'Lisenziya',
'nolicense' => 'Heç biri seçilməmişdir',

# Special:ImageList
'imagelist'      => 'Fayl siyahısı',
'imagelist_date' => 'Tarix',
'imagelist_name' => 'Ad',
'imagelist_user' => 'İstifadəçi',

# Image description page
'imagelinks'                => 'İstifadə edilən səhifələr',
'shareduploadwiki-linktext' => 'fayl təsvir səhifəsi',
'noimage-linktext'          => 'faylı yüklə',
'uploadnewversion-linktext' => 'Bu faylın yeni versiyasını yüklə',

# File deletion
'filedelete'                  => '$1 adlı faylı sil',
'filedelete-legend'           => 'Faylı sil',
'filedelete-intro'            => "'''[[Media:$1|$1]]''' faylını silirsiniz.",
'filedelete-comment'          => 'Qeyd:',
'filedelete-success'          => "'''$1''' silinmişdir.",
'filedelete-success-old'      => '<span class="plainlinks">\'\'\'[[Media:$1|$1]]\'\'\'-nin  $3 və $2 versiyaları silinmişdir.</span>',
'filedelete-otherreason'      => 'Başqa/əlavə səbəb:',
'filedelete-reason-otherlist' => 'Başqa səbəb',
'filedelete-reason-dropdown'  => '*Əsas silmə səbəbi
** Müəllif hüququ pozuntusu
** Dublikat fayl',

# MIME search
'mimesearch' => 'MIME axtar',

# Unwatched pages
'unwatchedpages' => 'İzlənməyən səhifələr',

# List redirects
'listredirects' => 'İstiqamətləndirmə siyahısı',

# Unused templates
'unusedtemplates'    => 'İstifadəsiz şablonlar',
'unusedtemplateswlh' => 'digər keçidlər',

# Random page
'randompage' => 'İxtiyari səhifə',

# Random redirect
'randomredirect' => 'İxtiyari istiqamətləndirmə',

# Statistics
'statistics'    => 'Statistika',
'sitestats'     => '{{SITENAME}} statistika',
'userstats'     => 'İstifadəçi statistika',
'sitestatstext' => "{{SITENAME}}-da hal-hazırda məqalələrin sayı: '''$2'''

Verilənlər bazasında yekun '''$1''' səhifə var. Buna müzakirələr, istifadəçi səhifələri, köməklər, wikipedia lahiye səhifələri, xüsusi səhifələr, istiqamətləndirmə səhifələri, boş səhifələr ilə fayllar v əşablonlar daxildir.

There have been a total of '''$3''' page views, and '''$4''' page edits
since the wiki was setup.
That comes to '''$5''' average edits per page, and '''$6''' views per edit.

Hal-hazırda [http://www.mediawiki.org/wiki/Manual:Job_queue job queue] sayı: '''$7'''.",
'userstatstext' => "Hal-hazırda '''$1''' istifadəçi, '''2''' (və ya '''4%''') tanesi idarəçi. (baxınız $3).",

'disambiguations'      => 'Dəqiqləşdirmə səhifələri',
'disambiguationspage'  => 'Şablon:dəqiqləşdirmə',
'disambiguations-text' => "Aşağıdakı səhifələr '''dəqiqləşdirmə səhifələrinə''' keçid verir. Bunun əvəzinə onlar çox guman ki, müvafiq konkret bir məqaləni göstərməlidirlər.
<br />Səhifə o zaman dəqiqləşdirmə səhifəsi hesab edilir ki, onda  [[MediaWiki:Disambiguationspage]]-dən keçid verilmiş şablon istifadə edilir.",

'doubleredirects' => 'İkiqat istiqamətləndirmələr',

'brokenredirects'        => 'Xətalı istiqamətləndirmə',
'brokenredirectstext'    => 'Bu istiqamətləndirmələr mövcud olmayan səhifəyə keçid verir.',
'brokenredirects-edit'   => '(redaktə)',
'brokenredirects-delete' => '(sil)',

'withoutinterwiki' => 'Dil keçidləri olmayan səhifələr',

'fewestrevisions' => 'Az dəyişiklik edilmiş məqalələr',

# Miscellaneous special pages
'nbytes'                  => '$1 bayt',
'nlinks'                  => '$1 bağlantı',
'specialpage-empty'       => 'Bu səhifə boşdur.',
'lonelypages'             => 'Yetim səhifələr',
'uncategorizedpages'      => 'Kateqoriyasız səhifələr',
'uncategorizedcategories' => 'Kateqoriyasız kateqoriyalar',
'uncategorizedimages'     => 'Kateqoriyasız şəkillər',
'uncategorizedtemplates'  => 'Kateqoriyasız şablonlar',
'unusedcategories'        => 'İstifadə edilməmiş kateqoriyalar',
'unusedimages'            => 'İstifadə edilməmiş fayllar',
'popularpages'            => 'Məşhur səhifələr',
'wantedcategories'        => 'Təlabat olunan kateqoriyalar',
'wantedpages'             => 'Təlabat olunan səhifələr',
'mostlinked'              => 'Ən çox keçidlənən səhifələr',
'mostlinkedcategories'    => 'Ən çox məqaləsi olan kateqoriyalar',
'mostcategories'          => 'Kateqoriyası ən çox olan məqalələr',
'mostimages'              => 'Ən çox istifadə edilmiş şəkillər',
'mostrevisions'           => 'Ən çox nəzərdən keçirilmiş (versiyalı) məqalələr',
'prefixindex'             => 'Prefiks indeks',
'shortpages'              => 'Qısa səhifələr',
'longpages'               => 'Uzun səhifələr',
'deadendpages'            => 'Keçid verməyən səhifələr',
'deadendpagestext'        => 'Aşağıdakı səhifələrdən bu Vikipediyadakı digər səhifələrə heç bir keçid yoxdur.',
'protectedpages'          => 'Mühafizəli səhifələr',
'protectedpagestext'      => 'Aşağıdakı səhifələr ad dəyişiminə və redaktəyə bağlıdır',
'protectedpagesempty'     => 'Hal-hazırda bu parametrə uyğun heç bir mühafizəli səhifə yoxdur',
'listusers'               => 'İstifadəçi siyahı',
'newpages'                => 'Yeni səhifələr',
'newpages-username'       => 'İstifadəçi adı:',
'ancientpages'            => 'Ən köhnə səhifələr',
'move'                    => 'Adını dəyişdir',
'movethispage'            => 'Bu səhifənin adını dəyiş',

# Book sources
'booksources'               => 'Kitab mənbələri',
'booksources-search-legend' => 'Kitab mənbələri axtar',
'booksources-go'            => 'Gətir',
'booksources-text'          => 'Aşağıda yeni və işlənmiş kitablar satan xarici keçidlərdə siz axtardığınız kitab haqqında əlavə məlumat ala bilərsiz:',

# Special:Log
'specialloguserlabel'  => 'İstifadəçi:',
'speciallogtitlelabel' => 'Başlıq:',
'log'                  => 'Loglar',
'all-logs-page'        => 'Bütün jurnallar',
'alllogstext'          => "Qarşıya yükləmə, silmə, qoruma, bloklama ve sistem operatoru loqlarının birləşdirilmiş göstərməsi. Log növü, istifadəçi adı veya tə'sir edilən səhifəni seçib görüntünü kiçildə bilərsiniz.",
'logempty'             => 'Jurnalda uyğun qeyd tapılmadı.',

# Special:AllPages
'allpages'          => 'Bütün səhifələr',
'alphaindexline'    => '$1 məqaləsindən $2 məqaləsinə kimi',
'nextpage'          => 'Sonrakı səhifə ($1)',
'allpagesfrom'      => 'Bu mövqedən başlayan səhifeleri göstər:',
'allarticles'       => 'Bütün məqalələr',
'allinnamespace'    => 'Bütün səhifələr ($1 səhifələri)',
'allnotinnamespace' => 'Bütün səhifələr (not in $1 namespace)',
'allpagesprev'      => 'Əvvəlki',
'allpagesnext'      => 'Sonrakı',
'allpagessubmit'    => 'Gətir',
'allpagesprefix'    => 'Bu prefiksli səhifələri göstər:',

# Special:Categories
'categories'         => 'Kateqoriyalar',
'categoriespagetext' => 'Wikide aşağıdaki kateqoriyalar var.',

# Special:ListUsers
'listusers-submit'   => 'Göstər',
'listusers-noresult' => 'İstifadəçi tapılmadı.',

# Special:ListGroupRights
'listgrouprights'          => 'İstifadəçi qruplarının hüquqları',
'listgrouprights-summary'  => 'Bu vikidə olan istifadəçi siyahıları və onların hüquqları aşağıda göstərilmişdir.
Fərdi hüquqlar haqqında əlavə məlumatı [[{{MediaWiki:Listgrouprights-helppage}}]] səhifəsində tapa bilərsiniz',
'listgrouprights-group'    => 'Qrup',
'listgrouprights-rights'   => 'Hüquqlar',
'listgrouprights-helppage' => 'Kömək:Qrup hüquqları',
'listgrouprights-members'  => '(üzvləri)',

# E-mail user
'emailuser'       => 'İstifadəçiyə e-məktub yolla',
'emailpage'       => 'İstifadəçiyə e-məktub yolla',
'defemailsubject' => '{{SITENAME}} e-məktub',
'noemailtitle'    => 'E-məktub ünvanı yox',
'noemailtext'     => 'Bu istifadəçi e-məktub ünvanını qeyd etməmişdir və ya digər istifadəçilərdən e-məktub almamaq opsiyasını seçmişdir',
'emailfrom'       => 'Kimdən:',
'emailto'         => 'Kimə',
'emailsubject'    => 'Mövzu:',
'emailmessage'    => 'Mesaj:',
'emailsend'       => 'Göndər',
'emailsent'       => 'E-məktub göndərildi',
'emailsenttext'   => 'E-məktub mesajınız göndərildi.',

# Watchlist
'watchlist'            => 'İzlədiyim səhifələr',
'mywatchlist'          => 'İzlədiyim səhifələr',
'watchlistfor'         => "('''$1''' üçün)",
'watchnologin'         => 'Daxil olmamısınız',
'watchnologintext'     => 'İzləmə siyahınızda dəyişiklik aparmaq üçün [[Special:Userlogin|daxil olmalısınız]].',
'addedwatch'           => 'İzləmə siyahısına əlavə edildi.',
'addedwatchtext'       => '"$1" səhifəsi [[Special:Watchlist|izlədiyiniz səhifələrə]] əlavə edildi. Bu səhifədə və əlaqəli müzakirə səhifəsində olacaq dəyişikliklər orada göstəriləcək və səhifə asanlıqla seçiləbilmək üçün [[Special:RecentChanges|son dəyişikliklər]]-də qalın şriftlərlə görsənəcəkdir.

Səhifəni izləmə sıyahınızdan çıxarmaq üçün yan lovhədəki "izləmə" düyməsinə vurun.',
'removedwatch'         => 'İzləmə siyahısından çıxardılıb',
'removedwatchtext'     => '"<nowiki>$1</nowiki>" səhifəsi izləmə siyahınızdan çıxardıldı.',
'watch'                => 'İzlə',
'watchthispage'        => 'Bu səhifəni izlə',
'unwatch'              => 'İzləmə',
'unwatchthispage'      => 'İzləmə',
'watchnochange'        => 'Verilən vaxt ərzində heç bir izlədiyiniz səhifə redaktə edilməmişdir.',
'watchlist-details'    => 'müzakirə səhifələri çıxmaq şərtilə $1 səhifəni izləyirsiniz',
'wlheader-enotif'      => '*  E-məktubla bildiriş aktivdir.',
'wlheader-showupdated' => "* Son ziyarətinizdən sonra edilən dəyişikliklər '''qalın şriftlərlə''' göstərilmişdir.",
'watchmethod-recent'   => 'yeni dəyişikliklər izlənilən səhifələr üçün yoxlanılır',
'watchmethod-list'     => 'izlənilən səhifələr yeni dəyişikliklər üçün yoxlanılır',
'watchlistcontains'    => 'İzləmə siyahınızda $1 səhifə var.',
'wlnote'               => "Aşağıdakılar son '''$2''' saatdakı son $1 dəyişiklikdir.",
'wlshowlast'           => 'Bunları göstər: son $1 saatı $2 günü $3',
'watchlist-show-bots'  => 'Bot redaktələrini göstər',
'watchlist-hide-bots'  => 'Bot redaktələrini gizlət',
'watchlist-show-own'   => 'Mənim redaktələrimi göstər',
'watchlist-hide-own'   => 'Mənim redaktələrimi gizlət',
'watchlist-show-minor' => 'Kiçik redaktələri göstər',
'watchlist-hide-minor' => 'Kiçik redaktələri gizlət',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'İzlənilir...',
'unwatching' => 'İzlənilmir...',

'enotif_reset'       => 'Baxılmış bütün səhifələri işarələ.',
'enotif_newpagetext' => 'Bu səhifə yeni səhifədir.',
'changed'            => 'dəyişdi',
'created'            => 'yaradıldı',

# Delete/protect/revert
'deletepage'                  => 'Səhifəni sil',
'confirm'                     => 'Təsdiq et',
'excontent'                   => "Köhnə məzmun: '$1'",
'excontentauthor'             => "Köhnə məzmun: '$1' (və tarixçədə fəaliyyəti qeyd edilən yeganə istifadəçi '[[User:$2|$2]]')",
'exbeforeblank'               => "Silinmədən əvvəlki məzmun: '$1'",
'exblank'                     => 'səhifə boş',
'delete-confirm'              => 'Silinən səhifə: "$1"',
'historywarning'              => 'Xəbərdarlıq: Silinəcək səhifənin tarixçəsində qeyd olunmuş redaktələr var',
'confirmdeletetext'           => 'Bu səhifə və ya fayl bütün tarixçəsi ilə birlikdə birdəfəlik silinəcək. Bunu nəzərdə tutduğunuzu və bu əməliyyatın nəticələrini başa düşdüyünüzü təsdiq edin.',
'actioncomplete'              => 'Fəaliyyət tamamlandı',
'deletedarticle'              => 'silindi "[[$1]]"',
'dellogpage'                  => 'Silmə qeydləri',
'dellogpagetext'              => 'Ən son silinmiş səhifələrin siyahısı.',
'deletionlog'                 => 'Silmə jurnal qeydləri',
'reverted'                    => 'Daha əvvəlki versiya bərpa edildi',
'deletecomment'               => 'Silmə səbəbi',
'deleteotherreason'           => 'Digər/əlavə səbəb:',
'deletereasonotherlist'       => 'Digər səbəb',
'deletereason-dropdown'       => '*Əsas silmə səbəbi
** Müəllif istəyi
** Müəllif hüququ pozuntusu
** Vandalizm',
'delete-edit-reasonlist'      => 'Silmə səbəblərinin redaktəsi',
'rollback'                    => 'Əvvəlki versiya',
'rollbacklink'                => 'əvvəlki halına qaytar',
'cantrollback'                => 'Redaktə geri qaytarıla bilməz; axırıncı redaktə səhifədə olan yeganə fəaliyyətdir.',
'revertpage'                  => '[[User:$2|$2]] tərəfindən edilmiş redaktələr geri qaytarılaraq [[User:$1|$1]] tərəfindən yaradılan sonuncu versiya bərpa olundu.', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => '$1 tərəfindən edilmiş redaktələr geri qaytarıldı; $2 tərəfindən yaradılmış son versiya bərpa olundu.',
'protectlogpage'              => 'Mühafizə etmə qeydləri',
'protectedarticle'            => 'mühafizə edildi "[[$1]]"',
'modifiedarticleprotection'   => '"[[$1]]" səhifəsi üçün qorunma səviyyəsi dəyişildi',
'unprotectedarticle'          => 'mühafizə kənarlaşdırdı "[[$1]]"',
'protect-title'               => '"$1" üçün mühafizə səviyyəsinin dəyişdirilməsi',
'protect-legend'              => 'Qorumayı təsdiq et',
'protectcomment'              => 'Şərh:',
'protectexpiry'               => 'Vaxtı bitib',
'protect_expiry_invalid'      => 'Bitmə vaxtı qüvvəsini itirmişdir',
'protect_expiry_old'          => 'Bitmə vaxtı keçmişdir.',
'protect-text'                => 'Siz <strong><nowiki>$1</nowiki></strong> səhifəsinin mühafizə səviyyəsini görə və dəyişə bilərsiniz.',
'protect-locked-blocked'      => 'Səhifənin bloklu olduğu müddətdə siz mühafizə səviyyəsini dəyişə bilməzsiniz.
<strong>$1</strong> səhifəsində hal-hazırda edə biləcəyiniz əməliyyatlar bunlardır:',
'protect-locked-dblock'       => 'Verilənlər bazası kilidli olduğu üçün mühafizə səviyyəsi dəyişilə bilməz.
<strong>$1</strong> səhifəsində hal-hazırda edə biləcəyiniz əməliyyatlar bunlardır:',
'protect-locked-access'       => 'Sizin hesabınızın mühafizə səviyyəsini dəyişməyə ixtiyarı yoxdur.
<strong>$1</strong> səhifəsində hal-hazırda edə biləcəyiniz əməliyyatlar bunlardır:',
'protect-cascadeon'           => 'Bu səhifə mühafizəlidir, çünki bu səhifə {{PLURAL:$1|başqa bir|başqa bir}} səhifədən kaskad mühafizə edilmişdir. Siz bu səhifənin mühafizə səviyyəsini dəyişdirə bilərsiniz, bu kaskad mühafizəyə təsir etməyəcək.',
'protect-level-autoconfirmed' => 'Anonim istifadəçiləri blokla',
'protect-level-sysop'         => 'Yalnız idarəçilər',
'protect-summary-cascade'     => 'kaskad mühafizə',
'protect-expiring'            => '$1 (UTC)- tarixində vaxtı bitir',
'protect-cascade'             => 'Kaskad mühafizəsi - bu səhifəyə daxil bütün səhifələri qoru',
'pagesize'                    => '(baytlar)',

# Restrictions (nouns)
'restriction-move' => 'Adını dəyiş',

# Restriction levels
'restriction-level-sysop' => 'tam mühafizə',

# Undelete
'undelete'               => 'Silinmiş səhifələri göstər',
'undeletepage'           => 'Silinmiş səhifələri göstər və ya bərpa et',
'viewdeletedpage'        => 'Silinmiş səhifələri göstər',
'undeletebtn'            => 'Bərpa et',
'undeletecomment'        => 'Səbəb:',
'undeletedarticle'       => '"[[$1]]" məqaləsi bərpa edilmişdir',
'cannotundelete'         => 'Silməni ləğv etmə yetinə yetirilə bilmir; başqa birisi daha əvvəl səhifənin silinməsini ləğv etmiş ola bilər.',
'undeletedpage'          => "<big>'''$1 bərpa edildi'''</big>

Məqalələrin bərpa edilməsi və silinməsi haqqında son dəyişiklikləri nəzərdən keçirmək üçün [[Special:Log/delete|silmə qeydlərinə]] baxın.",
'undelete-header'        => 'Son silinmiş səhifələrə baxmaq üçün [[Special:Log/delete|silmə qeydlərinə]] bax.',
'undelete-search-box'    => 'Silinmiş səhifələri axtar.',
'undelete-search-submit' => 'Axtar',

# Namespace form on various pages
'namespace'      => 'Adlar fəzası:',
'invert'         => 'Seçilən xaricindəkiləri',
'blanknamespace' => '(Ana)',

# Contributions
'contributions' => 'İstifadəçi köməkləri',
'mycontris'     => 'Köməklərim',
'nocontribs'    => 'Bu kriteriyaya uyğun redaktələr tapılmadı',
'uctop'         => '(son)',
'month'         => 'Ay',
'year'          => 'Axtarışa bu tarixdən etibarən başla:',

'sp-contributions-newbies'     => 'Ancaq yeni istifadəçilərin fəaliyyətlərini göstər',
'sp-contributions-newbies-sub' => 'Yeni istifadəçilər üçün',
'sp-contributions-blocklog'    => 'Bloklama qeydləri',
'sp-contributions-search'      => 'Fəaliyyətləri axtar',
'sp-contributions-username'    => 'İP Ünvanı və ya istifadəçi adı:',
'sp-contributions-submit'      => 'Axtar',

# What links here
'whatlinkshere'       => 'Bu səhifəyə bağlantılar',
'linklistsub'         => '(Bağlantılar siyahı)',
'isredirect'          => 'İstiqamətləndirmə səhifəsi',
'istemplate'          => 'daxil olmuş',
'whatlinkshere-prev'  => '{{PLURAL:$1|əvvəlki|əvvəlki $1}}',
'whatlinkshere-next'  => '{{PLURAL:$1|növbəti|növbəti $1}}',
'whatlinkshere-links' => '← keçidlər',

# Block/unblock
'blockip'                     => 'İstifadəçiyi blokla',
'blockip-legend'              => 'İstifadəçinin bloklanması',
'ipaddress'                   => 'IP ünvanı',
'ipadressorusername'          => 'IP ünvanı və ya istifadəçi adı',
'ipbexpiry'                   => 'Bitmə müddəti:',
'ipbreason'                   => 'Səbəb',
'ipbanononly'                 => 'Yalnız anonim istifadəçiləri blokla',
'ipbcreateaccount'            => 'Hesab açmanı məhdudlaşdır',
'ipbsubmit'                   => 'Bu istifadəçiyi əngəllə',
'ipbother'                    => 'Başqa vaxt',
'ipboptions'                  => '15 dəqiqə:15 minutes,1 saat:1 hour,3 saat:3 hours,24 saat:24 hours,48 saat:48 hours,1 həftə:1 week,1 ay:1 month,qeyri-müəyyən:indefinite', # display1:time1,display2:time2,...
'ipbotheroption'              => 'başqa',
'ipbotherreason'              => 'Başqa/əlavə səbəb:',
'ipbwatchuser'                => 'Bu istifadəçinin müzakirə və istifadəçi səhifəsini izlə',
'badipaddress'                => 'Yanlış IP',
'blockipsuccesssub'           => 'bloklandi',
'blockipsuccesstext'          => '[[Special:Contributions/$1| $1]]bloklanıb. <br />See[[Special:IPBlockList|IP blok siyahisi]] bloklanmış IP lər.',
'ipblocklist'                 => 'Əngəllənmiş istifadəçilər siyahı',
'blocklistline'               => '$1, $2 bloklandı $3 ($4)',
'infiniteblock'               => 'qeyri-müəyyən müddətə',
'expiringblock'               => 'son tarix $1',
'anononlyblock'               => 'yalnız anonim istifadəçi',
'createaccountblock'          => 'Yeni hesab yaratma bloklanıb',
'blocklink'                   => 'blokla',
'unblocklink'                 => 'bloklamanı kənarlaşdır',
'contribslink'                => 'Köməklər',
'blocklogpage'                => 'Blok qeydı',
'block-log-flags-anononly'    => 'yalnız qeydiyyatsız istifadəçilər',
'block-log-flags-nocreate'    => 'hesab yaradılması qeyri-mümkündür',
'block-log-flags-noautoblock' => 'avtobloklama qeyri-mümkündür',
'proxyblocker'                => 'Proksi bloklayıcı',

# Move page
'move-page-legend'        => 'Səhifənin adını dəyiş',
'movepagetext'            => "Aşağıdakı formadan istifədə edilməsi səhifənin adını bütün tarixçəsini də köçürməklə yeni başlığa dəyişəcək. Əvvəlki başlıq yeni başlığa istiqamətləndirmə səhifəsinə çevriləcək. Köhnə səhifəyə keçidlər dəyişməyəcək, ona görə də təkrarlanan və ya qırıq istiqamətləndirmələri yoxlamağı yaddan çıxarmayın. Keçidlərin lazımi yerə istiqamətləndirilməsini təmin etmək sizin məsuliyyətinizdədir.

Nəzərə alın ki, hədəf başlığı altında bir səhifə mövcuddursa yerdəyişmə '''baş tutmayacaq'''. Buna həmin səhifənin boş olması və ya istiqamətləndirmə səhifəsi olması və keçmişdə redaktə edilməməsi halları istisnadır. Bu o deməkdir ki, səhvən adını dəyişdiyiniz səhifələri geri qaytara bilər, bununla yanaşı artıq mövcud olan səhifənin üzərinə başqa səhifə yaza bilməzsiniz.

<b>XƏBƏRDARLIQ!</b>
Bu yerdəyişmə tanınmış səhifələr üçün əsaslı və gözlənilməz ola bilər, ona görə də bu dəyişikliyi yerinə yetirməzdən əvvəl bunun mümkün nəticələrini başa düşməniz xahiş olunur.",
'movearticle'             => 'Səhifənin adını dəyişdir',
'newtitle'                => 'Yeni başlıq',
'move-watch'              => 'Bu səhifəni izlə',
'movepagebtn'             => 'Səhifənin adını dəyiş',
'pagemovedsub'            => 'Yerdəyişmə edilmişdir',
'movepage-moved'          => '<big>\'\'\'"$1" səhifəsi "$2" səhifəsinə yerləşdirilmişdir\'\'\'</big>', # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'movedto'                 => 'dəyişdi',
'movetalk'                => 'Bu səhifənin müzakirə səhifəsinin de adını dəyişdir.',
'1movedto2'               => '[[$1]] adı dəyişildi. Yeni adı: [[$2]]',
'1movedto2_redir'         => '[[$1]] adı və məsiri dəyişildi : [[$2]]',
'movelogpage'             => 'Yerdəyişmə qeydləri',
'movereason'              => 'Səbəb',
'revertmove'              => 'Əvvəlki vəziyyətinə',
'delete_and_move'         => 'Sil və apar',
'delete_and_move_text'    => '==Hazırki məqalənin silinməsi lazımdır==

"[[$1]]" məqaləsi mövcuddur. Bu dəyişikliyin yerinə yetirilə bilməsi üçün həmin məqalənin silinməsini istəyirsinizmi?',
'delete_and_move_confirm' => 'Bəli, səhifəni sil',
'delete_and_move_reason'  => 'Ad dəyişməyə yer açmaq üçün silinmişdir',
'selfmove'                => 'Səhifənin hazırkı adı ilə dəyişmək istənilən ad eynidir. Bu əməliyyat yerinə yetirilə bilməz.',

# Export
'export'        => 'Səhifələri ixrac et',
'export-submit' => 'İxrac',

# Namespace 8 related
'allmessages'         => "Sistem mə'lumatları",
'allmessagesname'     => 'Ad',
'allmessagesdefault'  => 'İlkin mətn',
'allmessagescurrent'  => 'İndiki mətn',
'allmessagestext'     => "Sistem mə'lumatların siyahısı MediaWiki: namespace.",
'allmessagesfilter'   => "Mə'lumat adı süzgəci:",
'allmessagesmodified' => 'Yalnız redaktə olunmuşları göstər',

# Thumbnails
'djvu_page_error' => 'DjVu səhifəsi əlçatmazdır',
'djvu_no_xml'     => 'DjVu üçün XML faylı almaq mümkün deyil.',

# Special:Import
'importnotext' => 'Boş və ya mətn yoxdur',

# Tooltip help for the actions
'tooltip-pt-userpage'           => 'Öz Səhifəm',
'tooltip-pt-anonuserpage'       => 'The user page for the ip you',
'tooltip-pt-mytalk'             => 'Danişiq Səhifəm',
'tooltip-pt-anontalk'           => 'Bu IP ünvanindan redaktə olunmuş danışıqlar',
'tooltip-pt-preferences'        => 'Mənim Tərcihlərim',
'tooltip-pt-watchlist'          => 'İzləməyə aldığım məqalələr.',
'tooltip-pt-mycontris'          => 'Mən redakə etdiğim məqalələr siyahəsi',
'tooltip-pt-login'              => 'Hesab açmaniz tövsiə olur, ama icbar yoxdu .',
'tooltip-pt-anonlogin'          => 'Hesab açib girişiniz tövsiyə olur, ama məndatlı dəyil.',
'tooltip-pt-logout'             => 'Çixiş',
'tooltip-ca-talk'               => 'Məqalə həqqində müzakirə edib, nəzərivi bildir',
'tooltip-ca-edit'               => 'Bu səhifani redaktə edə bilərsiz. Lütfən avvəl sinaq gostəriş edin.',
'tooltip-ca-addsection'         => 'Bu müzakirə səhifəsində iştirak edin.',
'tooltip-ca-viewsource'         => 'Bu səhifə qorun altindadir. Mənbəsinə baxabilərsiz.',
'tooltip-ca-history'            => 'Bu səhifənin geçmiş nüsxələri.',
'tooltip-ca-protect'            => 'Bu səhifəni qoru',
'tooltip-ca-delete'             => 'Bu səhifəni sil',
'tooltip-ca-undelete'           => 'Bu səhifəni silinmədən oncəki halına qaytarın',
'tooltip-ca-move'               => 'Bu məqalənin adını dəyışin',
'tooltip-ca-watch'              => 'Bu səhifəni izlə',
'tooltip-ca-unwatch'            => 'Bu səhifənin izlənmasini bitir',
'tooltip-search'                => 'Bu vikini axtarin',
'tooltip-p-logo'                => 'Ana Səhifə',
'tooltip-n-mainpage'            => 'Ana səhifəni görüş edin',
'tooltip-n-portal'              => 'Projə həqqində, nələr edəbilərsiz, harda şeyləri tapa bilərsiz',
'tooltip-n-currentevents'       => 'Gündəki xəbərlər ilə əlaqəli bilgilər',
'tooltip-n-recentchanges'       => 'Bu Wikidə Son dəyişikliklər siyahəsi.',
'tooltip-n-randompage'          => 'Bir təsadufi, necə gəldi, məqaləyə baxin',
'tooltip-n-help'                => 'Yardım almaq üçün.',
'tooltip-t-whatlinkshere'       => 'Wikidə bu məqaləyə bağlantilar',
'tooltip-t-recentchangeslinked' => 'Bu məqaləyə ayid başqa səhifələrdə yeni dəyişikliklər',
'tooltip-t-contributions'       => 'Bu üzvin redaktə etmiş məqalələr siyahəsi',
'tooltip-t-emailuser'           => 'Bu istifadəçiyə bir e-məktub yolla',
'tooltip-t-upload'              => 'Yeni FILE lar Wikiyə yüklə.',
'tooltip-t-specialpages'        => 'Xüsusi səhifələrin siyahəsi',
'tooltip-ca-nstab-help'         => 'Kömək səhifəsi',
'tooltip-save'                  => 'Dəyişiklikləri qeyd et [alt-s]',
'tooltip-watch'                 => 'Bu səhifəni izlədiyiniz səhifələrə əlavə et [alt-w]',

# Attribution
'siteuser'    => '{{SITENAME}} istifadəçi $1',
'creditspage' => 'Səhifə kreditleri',

# Spam protection
'spamprotectiontitle' => 'Spam qoruma süzgəci',

# Math options
'mw_math_png'    => 'Həmişə PNG formatında göstər',
'mw_math_simple' => 'Sadə formullarda HTML, digərlərində PNG',
'mw_math_html'   => 'Mümkünsə HTML, digər hallarda PNG',
'mw_math_source' => 'TeX kimi saxla (mətn brouzerləri üçün)',
'mw_math_modern' => 'Müasir brouzerlər üçün məsləhətdir',
'mw_math_mathml' => 'Mümkünsə MathML (sınaq)',

# Patrol log
'patrol-log-auto' => '(avtomatik)',

# Image deletion
'deletedrevision'       => 'Köhnə versiyaları silindi $1.',
'filedeleteerror-short' => 'Fayl silinərkən xəta: $1',
'filedeleteerror-long'  => 'Fayl silinərkən üzə çıxan xətalar:

$1',

# Browsing diffs
'previousdiff' => '← Əvvəlki fərq',
'nextdiff'     => 'Sonrakı fərq →',

# Media information
'imagemaxsize'   => 'Limit images on image description pages to:',
'thumbsize'      => 'Kiçik ölçü:',
'file-info-size' => '($1 × $2 piksel, fayl həcmi: $3, MIME növü: $4)',
'file-nohires'   => '<small>Daha dəqiq versiyası yoxdur.</small>',

# Special:NewImages
'newimages'    => 'Yeni faylların siyahısı',
'showhidebots' => '($1 bot redaktə)',
'ilsubmit'     => 'Axtar',
'bydate'       => 'tarixe görə',

# EXIF tags
'exif-artist'              => 'Müəllif',
'exif-exposuretime-format' => '$1 saniyə ($2)',
'exif-aperturevalue'       => 'Obyektiv gözü',
'exif-brightnessvalue'     => 'Parlaqlıq',
'exif-filesource'          => 'Fayl mənbəsi',
'exif-contrast'            => 'Kontrast',
'exif-gpsaltitude'         => 'Yüksəklik',

'exif-componentsconfiguration-0' => 'mövcud deyil',

'exif-exposureprogram-1' => 'Əl ilə',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'bütün',
'imagelistall'     => 'bütün',
'watchlistall2'    => 'hamısını',
'namespacesall'    => 'bütün',

# E-mail address confirmation
'confirmemail'           => 'E-məktubunu təsdiq et',
'confirmemail_send'      => 'Təsdiq kodu göndər',
'confirmemail_sent'      => 'Təsdiq e-məktubu göndərildi.',
'confirmemail_invalid'   => 'Səhv təsdiqləmə kodu. Kodun vaxtı keçmiş ola bilər.',
'confirmemail_needlogin' => 'E-məktub ünvanınızın təsdiqlənməsi üçün $1 lazımdır.',
'confirmemail_success'   => 'E-məktub ünvanınız indi təsdiq edildi.',
'confirmemail_loggedin'  => 'E-məktubunuz indi təsdiq edildi.',
'confirmemail_subject'   => '{{SITENAME}} e-məktub təsdiq etme',

# Trackbacks
'trackbackremove' => '([$1 Sil])',

# Delete conflict
'deletedwhileediting' => 'Bu səhifə siz redaktə etməyə başladıqdan sonra silinmişdir!',

# action=purge
'confirm_purge' => 'Bu səhifə keşdən (cache) silinsin?

$1',

# Multipage image navigation
'imgmultipageprev' => '&larr; əvvəlki səhifə',
'imgmultipagenext' => 'sonrakı səhifə &rarr;',

# Table pager
'table_pager_next'         => 'Sonrakı səhifə',
'table_pager_prev'         => 'Əvvəlki səhifə',
'table_pager_first'        => 'İlk səhifə',
'table_pager_last'         => 'Son səhifə',
'table_pager_limit_submit' => 'Gətir',
'table_pager_empty'        => 'Nəticə yoxdur',

# Auto-summaries
'autosumm-blank'   => 'Səhifənin bütün məzmununun silinməsi',
'autosumm-replace' => "Səhifənin məzmunu '$1' yazısı ilə dəyişdirildi",
'autoredircomment' => '[[$1]] səhifəsinə istiqamətləndirilir',
'autosumm-new'     => 'Yeni səhifə: $1',

# Watchlist editor
'watchlistedit-normal-title' => 'İzlədiyim səhifələri redaktə et',
'watchlistedit-raw-titles'   => 'Başlıqlar:',

# Watchlist editing tools
'watchlisttools-edit' => 'İzlədiyim səhifələri göstər və redaktə et',

# Special:Version
'version' => 'Versiya', # Not used as normal message but as header for the special page itself

# Special:FilePath
'filepath' => 'Fayl yolu',

# Special:FileDuplicateSearch
'fileduplicatesearch' => 'Dublikat fayl axtarışı',

# Special:SpecialPages
'specialpages'                   => 'Xüsusi səhifələr',
'specialpages-group-maintenance' => 'Cari məruzələr',
'specialpages-group-other'       => 'Digər xüsusi səhifələr',
'specialpages-group-login'       => 'Daxil ol/ hesab yarat',
'specialpages-group-changes'     => 'Son dəyişikliklər və qeydlər',
'specialpages-group-media'       => 'Media məruzələri və yükləmələr',
'specialpages-group-users'       => 'İstifadəçilər və hüquqlar',
'specialpages-group-highuse'     => 'Ən çox istifadə edilən səhifələr',
'specialpages-group-pages'       => 'Səhifələrin siyahıları',
'specialpages-group-pagetools'   => 'Səhifə alətləri',
'specialpages-group-wiki'        => 'Viki məlumatları və alətləri',
'specialpages-group-redirects'   => 'Xüsusi istiqamətləndirmə səhifələri',

);
