<?php
/** Min Nan Chinese (Bân-lâm-gú)
 *
 * @ingroup Language
 * @file
 *
 */

$datePreferences = array(
	'default',
	'ISO 8601',
);
$defaultDateFormat = 'nan';
$dateFormats = array(
	'nan time' => 'H:i',
	'nan date' => 'Y-"nî" n-"goe̍h" j-"jἰt" (l)',
	'nan both' => 'Y-"nî" n-"goe̍h" j-"jἰt" (D) H:i',
);

$messages = array(
# User preference toggles
'tog-underline'               => 'Liân-kiat oē té-sûn:',
'tog-highlightbroken'         => 'Khang-ia̍h ê liân-kiat <a href="" class="new">án-ne</a> hián-sī (mài chhiūⁿ án-ne<a href="" class="internal">?</a>).',
'tog-hideminor'               => 'Am chòe-kīn ê sió kái-piàn',
'tog-extendwatchlist'         => 'Khok-chhiong kàm-sī-toaⁿ kàu hián-sī só͘-ū ê kái-piàn',
'tog-usenewrc'                => 'Ka-kiông pán ê chòe-kīn-ê-kái-piàn (su-iàu JavaScript)',
'tog-numberheadings'          => 'Phiau-tê chū-tōng pian-hō',
'tog-showtoolbar'             => 'Hián-sī pian-chi̍p ke-si-tiâu (su-iàu JavaScript)',
'tog-editondblclick'          => 'Siang-ji̍h ia̍h-bīn to̍h ē-tàng pian-chi̍p (su-iàu JavaScript)',
'tog-editsection'             => 'Ji̍h [siu-kái] chit-ê liân-kiat to̍h ē-tàng pian-chi̍p toāⁿ-lo̍h',
'tog-editsectiononrightclick' => 'Chiàⁿ-ji̍h (right click) toāⁿ-lo̍h (section) phiau-tê to̍h ē-tàng pian-chi̍p toāⁿ-lo̍h (su-iàu JavaScript)',
'tog-showtoc'                 => 'Hián-sī bo̍k-chhù (3-ê phiau-tê í-siōng ê ia̍h)',
'tog-rememberpassword'        => 'Kì tiâu bi̍t-bé, āu-chōa iōng',
'tog-editwidth'               => 'Pian-chi̍p keh-á thián kàu siōng khoah',
'tog-watchcreations'          => 'Kā goá khui ê ia̍h ka-ji̍p kàm-sī-toaⁿ lāi-té',
'tog-watchdefault'            => 'Kā goá pian-chi̍p kòe ê ia̍h ka-ji̍p kàm-sī-toaⁿ lāi-té',
'tog-watchmoves'              => 'Kā goá soá ê ia̍h ka-ji̍p kàm-sī-toaⁿ',
'tog-watchdeletion'           => 'Kā goá thâi tiāu ê ia̍h ka-ji̍p kàm-sī-toaⁿ',
'tog-minordefault'            => 'Chiām-tēng bī-lâi ê siu-kái lóng sī sió-siu-ká',
'tog-previewontop'            => 'Sûn-khoàⁿ ê lōe-iông tī pian-chi̍p keh-á thâu-chêng',
'tog-previewonfirst'          => 'Thâu-pái pian-chi̍p seng khoàⁿ-māi',
'tog-nocache'                 => 'Koaiⁿ-tiāu ia̍h ê cache',
'tog-fancysig'                => 'Chhiam-miâ mài chò liân-kiat',
'tog-externaleditor'          => 'Iōng gōa-pō· pian-chi̍p-khì',
'tog-externaldiff'            => 'Iōng gōa-pō· diff',
'tog-forceeditsummary'        => 'Pian-chi̍p khài-iàu bô thiⁿ ê sî-chūn, kā goá thê-chhéⁿ',
'tog-watchlisthideown'        => 'Kàm-sī-toaⁿ bián hián-sī goá ê pian-chi̍p',
'tog-watchlisthidebots'       => 'Kàm-sī-toaⁿ bián hián-sī ki-khì pian-chi̍p',
'tog-watchlisthideminor'      => 'Kàm-sī-toaⁿ bián hián-sī sió siu-kái',
'tog-ccmeonemails'            => 'Kià hō͘ pa̍t-lâng ê email sūn-soà kià copy hō͘ goá',
'tog-diffonly'                => 'Diff ē-pêng bián hián-sī ia̍h ê loē-iông',

'underline-always'  => 'Tiāⁿ-tio̍h',
'underline-never'   => 'Tiāⁿ-tio̍h mài',
'underline-default' => 'Tòe liû-lám-khì ê default',

'skinpreview' => '(Chhì khoàⁿ)',

# Dates
'sunday'        => 'Lé-pài',
'monday'        => 'Pài-it',
'tuesday'       => 'Pài-jī',
'wednesday'     => 'Pài-saⁿ',
'thursday'      => 'Pài-sì',
'friday'        => 'Pài-gō·',
'saturday'      => 'Pài-la̍k',
'sun'           => 'Lé-pài',
'mon'           => 'It',
'tue'           => 'Jī',
'wed'           => 'Saⁿ',
'thu'           => 'Sì',
'fri'           => 'Gō·',
'sat'           => 'La̍k',
'january'       => '1-goe̍h',
'february'      => '2-goe̍h',
'march'         => '3-goe̍h',
'april'         => '4-goe̍h',
'may_long'      => '5-goe̍h',
'june'          => '6-goe̍h',
'july'          => '7-goe̍h',
'august'        => '8-goe̍h',
'september'     => '9-goe̍h',
'october'       => '10-goe̍h',
'november'      => '11-goe̍h',
'december'      => '12-goe̍h',
'january-gen'   => 'Chiaⁿ-goe̍h',
'february-gen'  => 'Jī-goe̍h',
'march-gen'     => 'Saⁿ-goe̍h',
'april-gen'     => 'Sì-goe̍h',
'may-gen'       => 'Gō·-goe̍h',
'june-gen'      => 'La̍k-goe̍h',
'july-gen'      => 'Chhit-goe̍h',
'august-gen'    => 'Peh-goe̍h',
'september-gen' => 'Káu-goe̍h',
'october-gen'   => 'Cha̍p-goe̍h',
'november-gen'  => 'Cha̍p-it-goe̍h',
'december-gen'  => 'Cha̍p-jī-goe̍h',
'jan'           => '1g',
'feb'           => '2g',
'mar'           => '3g',
'apr'           => '4g',
'may'           => '5g',
'jun'           => '6g',
'jul'           => '7g',
'aug'           => '8g',
'sep'           => '9g',
'oct'           => '10g',
'nov'           => '11g',
'dec'           => '12g',

# Categories related messages
'pagecategories'         => '{{PLURAL:$1|Lūi-pia̍t|Lūi-pia̍t}}',
'category_header'        => 'Tī "$1" chit ê lūi-pia̍t ê bûn-chiuⁿ',
'subcategories'          => 'Ē-lūi-pia̍t',
'listingcontinuesabbrev' => '(chiap-sòa thâu-chêng)',

'about'      => 'Koan-hē',
'newwindow'  => '(ē khui sin thang-á hián-sī)',
'cancel'     => 'Chhú-siau',
'qbedit'     => 'Siu-kái',
'mypage'     => 'Góa ê ia̍h',
'mytalk'     => 'Góa ê thó-lūn',
'anontalk'   => 'Chit ê IP ê thó-lūn-ia̍h',
'navigation' => 'Se̍h chām',
'and'        => 'kap',

'errorpagetitle'    => 'Chhò-gō·',
'returnto'          => 'Tò-tńg khì $1.',
'help'              => 'Soat-bêng-su',
'search'            => 'Chhiau-chhoē',
'searchbutton'      => 'Chhiau',
'go'                => 'Lâi-khì',
'searcharticle'     => 'Lâi-khì',
'history'           => 'Ia̍h le̍k-sú',
'history_short'     => 'le̍k-sú',
'printableversion'  => 'Ìn-soat pán-pún',
'permalink'         => 'Éng-kiú liân-kiat',
'edit'              => 'Siu-kái',
'editthispage'      => 'Siu-kái chit ia̍h',
'delete'            => 'Thâi',
'deletethispage'    => 'Thâi chit ia̍h',
'undelete_short'    => 'Kiù $1 ê siu-kái',
'protect'           => 'Pó-hō·',
'protectthispage'   => 'Pó-hō· chit ia̍h',
'unprotect'         => 'Chhú-siau pó-hō·',
'unprotectthispage' => 'Chhú-siau pó-hō· chit ia̍h',
'newpage'           => 'Sin ia̍h',
'talkpage'          => 'Thó-lūn chit ia̍h',
'talkpagelinktext'  => 'thó-lūn',
'specialpage'       => 'Te̍k-sû-ia̍h',
'personaltools'     => 'Kò-jîn kang-khū',
'postcomment'       => 'Hoat-piáu phêng-lūn',
'talk'              => 'thó-lūn',
'toolbox'           => 'Ke-si kheh-á',
'userpage'          => 'Khoàⁿ iōng-chiá ê Ia̍h',
'projectpage'       => 'Khoàⁿ sū-kang ia̍h',
'viewtalkpage'      => 'Khoàⁿ thó-lūn',
'otherlanguages'    => 'Kî-thaⁿ ê gí-giân',
'redirectedfrom'    => '(Tùi $1 choán--lâi)',
'redirectpagesub'   => 'Choán-ia̍h',
'viewcount'         => 'Pún-ia̍h kàu taⁿ ū $1 pái access.',
'protectedpage'     => 'Siū pó-hō͘ ê ia̍h',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'hían-sī',
'bugreports'           => 'Saⁿ-pò bug',
'currentevents'        => 'Sin-bûn sū-kiāⁿ',
'currentevents-url'    => 'Project:Sin-bûn sū-kiāⁿ',
'disclaimers'          => 'Bô-hū-chek seng-bêng',
'edithelp'             => 'Án-choáⁿ siu-kái',
'edithelppage'         => 'Help:Pian-chi̍p',
'helppage'             => 'Help:Bo̍k-lio̍k',
'mainpage'             => 'Thâu-ia̍h',
'mainpage-description' => 'Thâu-ia̍h',
'portal'               => 'Siā-lí mn̂g-chhùi-kháu',
'portal-url'           => 'Project:Siā-lí mn̂g-chhùi-kháu',
'privacy'              => 'Ín-su chèng-chhek',

'retrievedfrom'       => 'Lâi-goân: "$1"',
'youhavenewmessages'  => 'Lí ū $1 ($2).',
'newmessageslink'     => 'sin sìn-sit',
'newmessagesdifflink' => 'chêng 2 ê siu-tēng-pún ê diff',
'editsection'         => 'siu-kái',
'editold'             => 'siu-kái',
'toc'                 => 'Bo̍k-lo̍k',
'showtoc'             => 'khui',
'hidetoc'             => 'siu',
'thisisdeleted'       => 'Khoàⁿ a̍h-sī kiù $1?',
'feedlinks'           => 'Chhī-liāu:',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Bûn-chiuⁿ',
'nstab-user'      => 'Iōng-chiá ê ia̍h',
'nstab-media'     => 'Mûi-thé',
'nstab-special'   => 'Te̍k-sû ia̍h',
'nstab-project'   => 'Sū-kang ia̍h',
'nstab-image'     => 'Tóng-àn',
'nstab-mediawiki' => 'Sìn-sit',
'nstab-template'  => 'Pang-bô·',
'nstab-category'  => 'Lūi-pia̍t',

# Main script and global functions
'nosuchspecialpage' => 'Bô chit ê te̍k-sû-ia̍h',

# General errors
'error'              => 'Chhò-gō·',
'databaseerror'      => 'Chu-liāu-khò· chhò-gō·',
'noconnect'          => 'háiⁿ-sè! Hiān-chhú-sî tú-tio̍h ki-su̍t būn-tê, bô-hoat-tō· lian-lo̍k chu-liāu-khò· ê sù-hû-chhí.<br />$1',
'cachederror'        => 'Í-hā sī beh ti̍h ê ia̍h ê cache khó·-pih, ū khó-lêng m̄-sī siōng sin ê.',
'readonly'           => 'Chu-liāu-khò· só tiâu leh',
'readonlytext'       => 'Chu-liāu-khò· hiān-chú-sî só tiâu leh, bô khai-hòng hō· lâng siu-kái. Che tāi-khài sī in-ūi teh pān î-siu khang-khòe, oân-sêng liáu-āu èng-tong tō ē hôe-ho̍k chèng-siông. Hū-chek ê hêng-chèng jîn-oân lâu chit-ê soat-bêng: $1',
'filecopyerror'      => 'Bô-hoat-tō· kā tóng-àn "$1" khó·-pih khì "$2".',
'filerenameerror'    => 'Bô-hoat-tō· kā tóng-àn "$1" kái-miâ chò "$2".',
'filedeleteerror'    => 'Bô-hoat-tō· kā tóng-àn "$1" thâi tiāu',
'filenotfound'       => 'Chhōe bô tóng-àn "$1".',
'formerror'          => 'Chhò-gō·: bô-hoat-tō· kā pió sàng chhut khì.',
'badarticleerror'    => 'Bē-tàng tiàm chit ia̍h chip-hêng chit ê tōng-chok.',
'cannotdelete'       => 'Bô-hoat-tō· kā hit ê ia̍h a̍h-sī iáⁿ-siōng thâi tiāu. (Khó-lêng pa̍t-lâng í-keng kā thâi tiāu ah.)',
'badtitle'           => 'M̄-chiâⁿ piau-tê',
'badtitletext'       => 'Iau-kiû ê piau-tê sī bô-hāu ê, khang ê, a̍h-sī liân-kiat chhò-gō· ê inter-language/inter-wiki piau-tê.',
'perfdisabled'       => 'Pháiⁿ-sè! Chit ê kong-lêng chiām-sî bô khui in-ūi ē chō-sêng chu-liāu-khò· siuⁿ kòe-thâu bān, tì-sú wiki bē iōng tit.',
'perfcached'         => 'Ē-kha ê chu-liāu tùi lâi--ê, só·-í bī-pit oân-choân hoán-èng siōng sin ê chōng-hóng.',
'perfcachedts'       => 'Ē-kha ê chu-liāu tùi lâi--ê, tī $1 keng-sin--koè.',
'viewsource'         => 'Khoàⁿ goân-sú lōe-iông',
'viewsourcefor'      => '$1 ê',
'protectedpagetext'  => 'Chit ia̍h hông só tiâu leh, bē pian-chi̍p tit.',
'viewsourcetext'     => 'Lí ē-sái khoàⁿ ia̍h khó͘-pih chit ia̍h ê goân-sú loē-iông:',
'protectedinterface' => 'Chit ia̍h thê-kiong nńg-thé kài-bīn ēng ê bûn-jī. Ūi beh ī-hông lâng chau-that, só͘-í ū siū tio̍h pó-hō͘.',
'editinginterface'   => "'''Sè-jī:''' Lí tng teh siu-kái 1 bīn thê-kiong nńg-thé kài-bīn bûn-jī ê ia̍h. Jīn-hô kái-piàn to ē éng-hióng tio̍h kî-thaⁿ iōng-chiá ê sú-iōng kài-bīn.",

# Login and logout pages
'logouttext'                 => '<strong>Lí í-keng teng-chhut.</strong><br />Lí ē-sái mài kì-miâ kè-siok sú-iōng {{SITENAME}}, mā ē-sái iōng kāng-ê a̍h-sī pa̍t-ê sin-hūn têng teng-ji̍p. Chhiaⁿ chù-ì: ū-kóa ia̍h ū khó-lêng khoàⁿ-tio̍h bē-su lí iû-goân teng-ji̍p tiong; che chi-iàu piàⁿ tiāu lí ê browser ê cache chiū ē chèng-siông.',
'welcomecreation'            => '==Hoan-gêng $1!==
Í-keng khui hó lí ê kháu-chō.  M̄-hó bē-kì-tit chhiâu lí ê iōng-chiá siat-tēng.',
'yourname'                   => 'Lí ê iōng-chiá miâ-chheng:',
'yourpassword'               => 'Lí ê bi̍t-bé:',
'yourpasswordagain'          => 'Têng phah bi̍t-bé:',
'remembermypassword'         => 'Kì tiâu góa ê bi̍t-bé (āu-chhiú teng-ji̍p iōng).',
'loginproblem'               => '<b>Teng-ji̍p tú tio̍h būn-tê.</b><br />Chhiáⁿ têng chhì!',
'login'                      => 'Teng-ji̍p',
'loginprompt'                => 'Thiⁿ ē-kha ê chu-liāu thang khui sin hō·-thâu a̍h-sī teng-ji̍p {{SITENAME}}.',
'userlogin'                  => 'Teng-ji̍p / khui sin kháu-chō',
'logout'                     => 'Teng-chhut',
'userlogout'                 => 'Teng-chhut',
'notloggedin'                => 'Bô teng-ji̍p',
'nologin'                    => 'Bô-thang teng-ji̍p? $1.',
'nologinlink'                => 'Khui 1 ê kháu-chō',
'createaccount'              => 'Khui sin kháu-chō',
'gotaccount'                 => 'Í-keng ū kháu-chō? $1.',
'gotaccountlink'             => 'Teng-ji̍p',
'badretype'                  => 'Lí su-ji̍p ê 2-cho· bi̍t-bé bô tùi.',
'userexists'                 => 'Lí beh ti̍h ê iōng-chiá miâ-chheng í-keng ū lâng iōng. Chhiáⁿ kéng pa̍t-ê miâ.',
'youremail'                  => 'Lí ê email:',
'yourrealname'               => 'Lí ê chin miâ:',
'yourlanguage'               => 'Kài-bīn gú-giân:',
'yournick'                   => 'Lí ê sió-miâ (chhiam-miâ iōng):',
'loginerror'                 => 'Teng-ji̍p chhò-gō·',
'loginsuccesstitle'          => 'Teng-ji̍p sêng-kong',
'loginsuccess'               => 'Lí hiān-chhú-sî í-keng teng-ji̍p {{SITENAME}} chò "$1".',
'nosuchuser'                 => 'Chia bô iōng-chiá hō-chò "$1". Chhiáⁿ kiám-cha lí ê phèng-im, a̍h-sī iōng ē-kha ê pió lâi khui sin iōng-chiá ê kháu-chō.',
'wrongpassword'              => 'Lí su-ji̍p ê bi̍t-bé ū têng-tâⁿ. Chhiáⁿ têng chhì.',
'wrongpasswordempty'         => 'Bi̍t-bé keh-á khang-khang. Chhiáⁿ têng chhì.',
'mailmypassword'             => 'Kià sin bi̍t-bé hō· góa',
'passwordremindertitle'      => '{{SITENAME}} the-chheN li e bit-be',
'noemail'                    => 'Kì-lo̍k bô iōng-chiá "$1" ê e-mail chū-chí.',
'passwordsent'               => 'Ū kià sin bi̍t-bé khì "$1" chù-chheh ê e-mail chū-chí. Siu--tio̍h liáu-āu chhiáⁿ têng teng-ji̍p.',
'mailerror'                  => 'Kià phoe tú tio̍h chhò-gō·: $1',
'acct_creation_throttle_hit' => 'Pháiⁿ-sè, lí taⁿ í-keng khui $1 ê kháu-chō ā. Bē-sái koh-chài khui.',
'emailauthenticated'         => 'Lí ê e-mail chū-chí tī $1 khak-jīn sêng-kong.',
'emailnotauthenticated'      => 'Lí ê e-mail chū-chí iáu-bōe khak-jīn ū-hāu, só·-í ē--kha ê e-mail kong-lêng bē-ēng-tit.',
'emailconfirmlink'           => 'Chhiáⁿ khak-jīn lí ê e-mail chū-chí ū-hāu',

# Edit page toolbar
'bold_sample'     => 'Chho·-thé bûn-jī',
'bold_tip'        => 'Chho·-thé jī',
'italic_sample'   => 'Chhú-thé ê bûn-jī',
'italic_tip'      => 'Chhú-thé jī',
'link_sample'     => 'Liân-kiat piau-tê',
'link_tip'        => 'Lōe-pō· ê liân-kiat',
'extlink_sample'  => 'http://www.example.com liân-kiat piau-tê',
'extlink_tip'     => 'Gōa-pō· ê liân-kiat (ē-kì-tit thâu-chêng ài ke http://)',
'headline_sample' => 'Thâu-tiâu bûn-jī',
'headline_tip'    => 'Tē-2-chân (level 2) ê phiau-tê',
'math_sample'     => 'Chia siá hong-thêng-sek',
'nowiki_sample'   => 'Chia siá bô keh-sek ê bûn-jī',
'image_sample'    => 'Iann-siong-e-le.jpg',
'image_tip'       => 'Giap tī lāi-bīn ê iáⁿ-siōng',
'sig_tip'         => 'Lí ê chhiam-miâ kap sî-kan ìn-á',

# Edit pages
'summary'                => 'Khài-iàu',
'subject'                => 'Tê-bo̍k/piau-tê',
'minoredit'              => 'Che sī sió siu-kái',
'watchthis'              => 'Kàm-sī chit ia̍h',
'savearticle'            => 'Pó-chûn chit ia̍h',
'preview'                => 'Seng khoàⁿ-māi',
'showpreview'            => 'Seng khoàⁿ-māi',
'showdiff'               => 'Khòaⁿ kái-piàn ê pō·-hūn',
'anoneditwarning'        => "'''Kéng-kò:''' Lí bô teng-ji̍p. Lí ê IP chū-chí ē kì tī pún ia̍h ê pian-chi̍p le̍k-sú lāi-bīn.",
'summary-preview'        => 'Khài-iàu ê preview',
'subject-preview'        => 'Ū-lám tê-bo̍k/piau-tê',
'whitelistedittitle'     => 'Su-iàu teng-ji̍p chiah ē-sái siu-kái',
'whitelistedittext'      => 'Lí ài $1 chiah ē-sái siu-kái.',
'confirmedittitle'       => 'Su-iàu khak-jīn e-mail chū-chí chiah ē-tit pian-chi̍p',
'loginreqtitle'          => 'Su-iàu Teng-ji̍p',
'accmailtitle'           => 'Bi̍t-bé kià chhut khì ah.',
'accmailtext'            => '$1 ê bi̍t-bé í-keng kìa khì $2.',
'newarticle'             => '(Sin)',
'newarticletext'         => "Lí tòe 1 ê liân-kiat lâi kàu 1 bīn iáu-bōe chûn-chāi ê ia̍h. Beh khai-sí pian-chi̍p chit ia̍h, chhiáⁿ tī ē-kha ê bûn-jī keh-á lāi-té phah-jī. ([[{{MediaWiki:Helppage}}|Bo̍k-lio̍k]] kà lí án-choáⁿ chìn-hêng.) Ká-sú lí bô-tiuⁿ-tî lâi kàu chia, ē-sai chhi̍h liû-lám-khì ê '''téng-1-ia̍h''' tńg--khì.",
'anontalkpagetext'       => "----''Pún thó-lūn-ia̍h bô kò·-tēng ê kháu-chō/hō·-thâu, kan-na ū 1 ê IP chū-chí (chhin-chhiūⁿ 123.456.789.123). In-ūi bô kāng lâng tī bô kāng sî-chūn ū khó-lêng tú-hó kong-ke kāng-ê IP, lâu tī chia ê oē ū khó-lêng hō· bô kāng lâng ê! Beh pī-bián chit khoán būn-tê, ē-sái khì [[Special:UserLogin|khui 1 ê hō·-thâu a̍h-sī teng-ji̍p]].''",
'clearyourcache'         => "'''Chù-ì:''' Pó-chûn liáu-āu, tio̍h ē-kì leh kā liû-lám-khì ê cache piàⁿ tiāu chiah khoàⁿ-ē-tio̍h kái-piàn: '''Mozilla:''' chhi̍h ''reload/têng-sin chài-ji̍p'' (a̍h-sī ''Ctrl-R''), '''Internet Explorer kap Opera:''' ''Ctrl-F5'', '''Safari:''' ''Cmd-R'', '''Konqueror''' ''Ctrl-R''.",
'usercssjsyoucanpreview' => "<strong>Phiat-pō·</strong>: Pó-chûn chìn-chêng ē-sái chhi̍h 'Seng khoàⁿ-māi' kiám-cha sin ê CSS a̍h-sī JavaScript.",
'usercsspreview'         => "'''Sè-jī! Lí hiān-chú-sî khoàⁿ--ê sī lí ê su-jîn css ê preview; che iáu-bōe pó-chûn--khí-lâi!'''",
'userjspreview'          => "'''Sè-jī! Lí hiān-chú-sî chhì khoàⁿ--ê sī lí ka-kī ê javascript; che iáu-bōe pó-chûn--khí-lâi!'''",
'note'                   => '<strong>Chù-ì:</strong>',
'previewnote'            => '<strong>Thê-chhéⁿ lí che sī 1 bīn kiám-cha chho͘-phe ēng--ê "seng-khoàⁿ-ia̍h", iáu-bōe pó-chûn--khí-lâi!</strong>',
'session_fail_preview'   => '<strong>Pháiⁿ-sè! Gún chiām-sî bô hoat-tō͘ chhú-lí lí ê pian-chi̍p (goân-in: "phàng-kiàn sú-iōng kî-kan ê chu-liāu"). Lô-hoân têng chhì khoàⁿ-māi. Ká-sú iû-goân bô-hāu, ē-sái teng-chhut koh-chài teng-ji̍p hoān-sè tō ē-tit kái-koat.</strong>',
'editing'                => 'Siu-kái $1',
'editingsection'         => 'Pian-chi̍p $1 (section)',
'editingcomment'         => 'Teh pian-chi̍p $1 (lâu-oē)',
'editconflict'           => 'Siu-kái sio-chhiong: $1',
'explainconflict'        => 'Ū lâng tī lí tng teh siu-kái pún-ia̍h ê sî-chūn oân-sêng kî-tha ê siu-kái. Téng-koân ê bûn-jī-keh hián-sī hiān-chhú-sî siōng sin ê lōe-iông. Lí ê kái-piàn tī ē-kha ê bûn-jī-keh. Lí su-iàu chiōng lí chò ê kái-piàn kap siōng sin ê lōe-iông chéng-ha̍p. <b>Kan-na</b> téng-koân keh-á ê bûn-jī ē tī lí chhi̍h "Pó-chûn" liáu-āu pó-chûn khí lâi.<br />',
'yourtext'               => 'Lí ê bûn-jī',
'storedversion'          => 'Chu-liāu-khò· ê pán-pún',
'editingold'             => '<strong>KÉNG-KÒ: Lí tng teh siu-kái chit ia̍h ê 1 ê kū siu-tēng-pún. Lí nā kā pó-chûn khí lâi, chit ê siu-tēng-pún sòa-āu ê jīm-hô kái-piàn ē bô khì.</strong>',
'yourdiff'               => 'Chha-pia̍t',
'longpagewarning'        => '<strong>SÈ-JĪ: Pún ia̍h $1 kilobyte tn̂g; ū-ê liû-lám-khì bô-hoat-tō· pian-chi̍p 32 kb chó-iū ia̍h-sī koh khah tn̂g ê ia̍h. Chhiáⁿ khó-lū kā chit ia̍h thiah chò khah sè ê toāⁿ-lo̍h.</strong>',
'readonlywarning'        => '<strong>CHÙ-Ì: Chu-liāu-khò· taⁿ só tiâu leh thang pān î-siu khang-khòe, só·-í lí hiān-chú-sî bô thang pó-chûn jīn-hô phian-chi̍p hāng-bo̍k. Lí ē-sái kā siong-koan pō·-hūn tah--ji̍p-khì 1-ê bûn-jī tóng-àn pó-chûn, āu-chhiú chiah koh kè-sio̍k.</strong>',
'protectedpagewarning'   => '<strong>KÉNG-KÒ: Pún ia̍h só tiâu leh. Kan-taⁿ ū hêng-chèng te̍k-koân ê iōng-chiá (sysop) ē-sái siu-kái.</strong>',
'templatesused'          => 'Chit ia̍h iōng chia ê pang-bô·:',
'templatesusedpreview'   => 'Chit ê preview iōng chia ê pang-bô͘:',
'templatesusedsection'   => 'Chit ê section iōng chia ê pang-bô͘:',
'template-protected'     => '(pó-hō͘)',
'template-semiprotected' => '(poàⁿ pó-hō͘)',
'recreate-deleted-warn'  => "'''Sè-jī: Lí taⁿ chún-pī beh khui ê ia̍h, chêng bat hō͘ lâng thâi tiāu koè.''' Lí tio̍h chim-chiok soà-chiap pian-chi̍p chit ia̍h ê pit-iàu-sèng. Chia ū chit ia̍h ê san-tû kì-lo̍k (deletion log) hō͘ lí chham-khó:",

# "Undo" feature
'undo-success' => 'Pian-chi̍p í-keng chhú-siau. Chhiáⁿ khak-tēng, liáu-āu kā ē-kha ho̍k-goân ê kái-piàn pó-chûn--khí-lâi.',
'undo-failure' => 'Pian-chi̍p bē-tàng chhú-siau, in-ūi chhiong tio̍h kî-kan chhah-ji̍p ê pian-chi̍p.',
'undo-summary' => 'Chhú-siau [[Special:Contributions/$2|$2]] ([[User talk:$2|thó-lūn]]) ê siu-tēng-pún $1',

# History pages
'viewpagelogs'        => 'Khoàⁿ chit ia̍h ê logs',
'nohistory'           => 'Chit ia̍h bô pian-chi̍p-sú.',
'revnotfound'         => 'Chhōe bô siu-tēng-pún',
'currentrev'          => 'Hiān-chú-sî ê siu-tēng-pún',
'revisionasof'        => '$1 ê siu-tēng-pún',
'previousrevision'    => '←Khah kū ê siu-tēng-pún',
'nextrevision'        => 'Khah sin ê siu-tēng-pún→',
'currentrevisionlink' => 'khoàⁿ siōng sin ê siu-tēng-pún',
'cur'                 => 'taⁿ',
'last'                => 'chêng',
'page_first'          => 'Tùi thâu-chêng',
'page_last'           => 'Tùi āu-piah',
'histlegend'          => 'Pán-pún pí-phēng: tiám-soán beh pí-phēng ê pán-pún ê liú-á, liáu-āu chhi̍h ENTER a̍h-sī ē-kha hit tè sì-kak.<br />Soat-bêng: (taⁿ) = kap siōng sin pán-pún pí-phēng, (chêng) = kap chêng-1-ê pán-pún pí-phēng, ~ = sió siu-kái.',
'histfirst'           => 'Tùi thâu-chêng',
'histlast'            => 'Tùi āu-piah',

# Diffs
'difference'              => '(Bô kâng pán-pún ê cheng-chha)',
'lineno'                  => 'Tē $1 chōa:',
'compareselectedversions' => 'Pí-phēng soán-te̍k ê pán-pún',
'editundo'                => 'chhú-siau',

# Search results
'searchresults'     => 'Kiám-sek kiat-kó',
'searchresulttext'  => 'Koan-hē kiám-sek {{SITENAME}} ê siông-sè pō·-sò·, chhiáⁿ chham-khó [[{{MediaWiki:Helppage}}|{{int:help}}]].',
'titlematches'      => 'Phiau-tê ū-tùi ê bûn-chiuⁿ',
'notitlematches'    => 'Bô sio-tùi ê ia̍h-piau-tê',
'textmatches'       => 'Lōe-iông ū-tùi ê bûn-chiuⁿ',
'notextmatches'     => 'Bô sio-tùi ê bûn-chiuⁿ lōe-iông',
'prevn'             => 'chêng $1 hāng',
'nextn'             => 'āu $1 hāng',
'viewprevnext'      => 'Khoàⁿ ($1) ($2) ($3)',
'showingresults'    => 'Ē-kha tùi #<b>$2</b> khai-sí hián-sī <b>$1</b> hāng kiat-kó.',
'showingresultsnum' => 'Ē-kha tùi #<b>$2</b> khai-sí hián-sī <b>$3</b> hāng kiat-kó.',
'powersearch'       => 'Kiám-sek',

# Preferences page
'preferences'           => 'Siat-tēng',
'mypreferences'         => 'Góa ê siat-tēng',
'prefsnologin'          => 'Bô teng-ji̍p',
'prefsnologintext'      => 'Lí it-tēng ài [[Special:UserLogin|teng-ji̍p]] chiah ē-tàng chhiâu iōng-chiá ê siat-tēng.',
'prefsreset'            => 'Iōng-chiá siat-tēng í-keng chiàu goân siat-tēng têng-siat.',
'qbsettings'            => 'Quickbar ê siat-tēng',
'changepassword'        => 'Oāⁿ bi̍t-bé',
'skin'                  => 'Phôe',
'math'                  => 'Sò·-ha̍k ê rendering',
'dateformat'            => 'Ji̍t-kî keh-sek',
'datedefault'           => 'Chhìn-chhái',
'datetime'              => 'Ji̍t-kî kap sî-kan',
'prefs-personal'        => 'Iōng-chiá chu-liāu',
'prefs-rc'              => 'Chòe-kīn ê kái-piàn & stub ê hián-sī',
'prefs-watchlist'       => 'Kàm-sī-toaⁿ',
'prefs-watchlist-days'  => 'Kàm-sī-toaⁿ hián-sī kúi kang lāi--ê:',
'prefs-watchlist-edits' => 'Khok-chhiong ê kàm-sī-toaⁿ tio̍h hián-sī kúi hāng pian-chi̍p:',
'prefs-misc'            => 'Kî-thaⁿ ê siat-tēng',
'saveprefs'             => 'Pó-chûn siat-tēng',
'resetprefs'            => 'Têng siat-tēng',
'oldpassword'           => 'Kū bi̍t-bé:',
'newpassword'           => 'Sin bi̍t-bé:',
'retypenew'             => 'Têng phah sin bi̍t-bé:',
'textboxsize'           => 'Pian-chi̍p',
'rows'                  => 'Chōa:',
'columns'               => 'Nôa',
'searchresultshead'     => 'Chhiau-chhōe kiat-kó ê siat-tēng',
'resultsperpage'        => '1 ia̍h hián-sī kúi kiāⁿ:',
'contextlines'          => '1 kiāⁿ hián-sī kúi chōa:',
'contextchars'          => '1 chōa hián-sī kúi jī ê chêng-āu-bûn:',
'recentchangesdays'     => 'Hián-sī kúi ji̍t chòe-kīn ê kái-piàn:',
'recentchangescount'    => 'Hián-sī kúi tiâu chòe-kīn ê kái-piàn:',
'savedprefs'            => 'Lí ê iōng-chiá siat-tēng í-keng pó-chûn khí lâi ah.',
'timezonelegend'        => 'Sî-khu',
'timezonetext'          => 'Su-ji̍p lí chāi-tē sî-kan kap server sî-kan (UTC) chha kúi tiám-cheng.',
'localtime'             => 'Chāi-tē sî-kan sī',
'timezoneoffset'        => 'Sî-chha¹',
'servertime'            => 'Server sî-kan hiān-chāi sī',
'guesstimezone'         => 'Tùi liû-lám-khì chhau--lâi',
'allowemail'            => 'Ún-chún pa̍t-ê iōng-chiá kià email kòe-lâi',
'defaultns'             => 'Tī chiah ê miâ-khong-kan chhiau-chhōe:',
'files'                 => 'Tóng-àn',

'grouppage-sysop' => '{{ns:project}}:Hêng-chèng jîn-oân',

# User rights log
'rightslogtext' => 'Chit-ê log lia̍t-chhut kái-piàn iōng-chiá koân-lī ê tōng-chok.',

# Recent changes
'recentchanges'   => 'Chòe-kīn ê kái-piàn',
'rcnotefrom'      => 'Ē-kha sī <b>$2</b> kàu taⁿ ê kái-piàn (ke̍k-ke hián-sī <b>$1</b> hāng).',
'rclistfrom'      => 'Hián-sī tùi $1 kàu taⁿ ê sin kái-piàn',
'rcshowhideminor' => '$1 sió siu-kái',
'rcshowhideliu'   => '$1 teng-ji̍p ê iōng-chiá',
'rcshowhideanons' => '$1 bû-bêng-sī',
'rcshowhidemine'  => '$1 góa ê pian-chi̍p',
'rclinks'         => 'Hían-sī $2 ji̍t lāi siōng sin ê $1 hāng kái-piàn<br />$3',
'hist'            => 'ls',
'hide'            => 'am',
'show'            => 'hían-sī',
'minoreditletter' => '~',
'newpageletter'   => '!',

# Recent changes linked
'recentchangeslinked'          => 'Siong-koan ê kái-piàn',
'recentchangeslinked-noresult' => 'Lí chí-tēng ê tiâu-kiaⁿ lāi-té chhōe bô jīn-hô kái-piàn.',

# Upload
'upload'            => 'Kā tóng-àn chiūⁿ-bāng',
'uploadbtn'         => 'Kā tóng-àn chiūⁿ-bāng',
'reupload'          => 'Têng sàng-chiūⁿ-bāng',
'reuploaddesc'      => 'Tò khì sàng-chiūⁿ-bāng ê pió.',
'uploadnologin'     => 'Bô teng-ji̍p',
'uploadnologintext' => 'Bô [[Special:UserLogin|teng-ji̍p]] bē-sái-tit kā tóng-àn sàng-chiūⁿ-bāng.',
'uploaderror'       => 'Upload chhò-gō·',
'uploadlogpagetext' => 'Í-hā sī chòe-kīn sàng-chiūⁿ-bāng ê tóng-àn ê lia̍t-toaⁿ.',
'filename'          => 'Tóng-àn',
'filedesc'          => 'Khài-iàu',
'fileuploadsummary' => 'Khài-iàu:',
'uploadedfiles'     => 'Tóng-àn í-keng sàng chiūⁿ-bāng',
'ignorewarning'     => 'Mài chhap kéng-kò, kā tóng-àn pó-chûn khí lâi.',
'ignorewarnings'    => 'Mài chhap kéng-kò',
'badfilename'       => 'Iáⁿ-siōng ê miâ í-keng kái chò "$1".',
'successfulupload'  => 'Sàng-chiūⁿ-bāng sêng-kong',
'uploadwarning'     => 'Upload kéng-kò',
'savefile'          => 'Pó-chûn tóng-àn',
'uploadedimage'     => 'thoân "[[$1]]" chiūⁿ-bāng',
'uploaddisabled'    => 'Pháiⁿ-sè, sàng chiūⁿ-bāng ê kong-lêng bô khui.',
'uploadcorrupt'     => 'Tóng-àn nōa--khì he̍k-chiá tóng-àn miâ ê bóe-liu tàu m̄-tio̍h khoán. Chhiáⁿ kiám-cha--chi̍t-ē, liáu-āu têng thoân chiūⁿ-bāng.',
'sourcefilename'    => 'Tóng-àn goân miâ:',
'destfilename'      => 'Tóng-àn sin miâ:',
'watchthisupload'   => 'Kàm-sī chit ia̍h',

# Special:ImageList
'imagelist'             => 'Iáⁿ-siōng lia̍t-toaⁿ',
'imagelist_date'        => 'Ji̍t-kî',
'imagelist_name'        => 'Miâ',
'imagelist_user'        => 'Iōng-chiá',
'imagelist_size'        => 'Toā-sè',
'imagelist_description' => 'Soat-bêng',

# Image description page
'imagelinks'       => 'Iáⁿ-siōng liân-kiat',
'linkstoimage'     => 'Í-hā ê ia̍h liân kàu chit ê iáⁿ-siōng:',
'nolinkstoimage'   => 'Bô poàⁿ ia̍h liân kàu chit tiuⁿ iáⁿ-siōng.',
'noimage'          => 'Hō chit ê miâ ê tóng-àn bô tè chhoē. Lí ē-sái $1.',
'noimage-linktext' => 'kā thoân chiūⁿ bāng',

# MIME search
'mimesearch' => 'MIME chhiau-chhoē',

# Unwatched pages
'unwatchedpages' => 'Bô lâng kàm-sī ê ia̍h',

# List redirects
'listredirects' => 'Lia̍t-chhut choán-ia̍h',

# Unused templates
'unusedtemplates' => 'Bô iōng ê pang-bô·',

# Random page
'randompage' => 'Sûi-chāi kéng ia̍h',

# Random redirect
'randomredirect' => 'Sûi-chāi choán-ia̍h',

# Statistics
'statistics'    => 'Thóng-kè',
'sitestats'     => '{{SITENAME}} chām ê thóng-kè sò·-ba̍k',
'userstats'     => 'Iōng-chiá thóng-kè sò·-ba̍k',
'userstatstext' => "Ū '''$1''' ê iōng-chiá chù-chheh. Kî-tiong '''$2''' ê ($4%) sī $5.",

'disambiguations'     => 'Khu-pia̍t-ia̍h',
'disambiguationspage' => 'Template:disambig
Template:KhPI
Template:Khu-pia̍t-iah
Template:Khu-pia̍t-ia̍h',

'doubleredirects' => 'Siang-thâu choán-ia̍h',

'brokenredirects'     => 'Choán-ia̍h kò·-chiòng',
'brokenredirectstext' => 'Í-hā ê choán-ia̍h liân kàu bô chûn-chāi ê ia̍h:',

'withoutinterwiki'         => 'Bô gí-giân liân-kiat ê ia̍h',
'withoutinterwiki-summary' => 'Ē-kha ê ia̍h bô kî-thaⁿ gí-giân pán-pún ê liân-kiat:',

'fewestrevisions' => 'Siōng bô siu-tēng ê bûn-chiuⁿ',

# Miscellaneous special pages
'ncategories'             => '$1 {{PLURAL:$1|ê lūi-pia̍t |ê lūi-pia̍t}}',
'nlinks'                  => '$1 ê liân-kiat',
'nmembers'                => '$1 ê sêng-oân',
'nrevisions'              => '$1 ê siu-tēng-pún',
'lonelypages'             => 'Ko·-ia̍h',
'uncategorizedpages'      => 'Bô lūi-pia̍t ê ia̍h',
'uncategorizedcategories' => 'Bô lūi-pia̍t ê lūi-pia̍t',
'uncategorizedimages'     => 'Bô lūi-pia̍t ê iáⁿ-siōng',
'uncategorizedtemplates'  => 'Bô lūi-pia̍t ê pang-bô͘',
'unusedcategories'        => 'Bô iōng ê lūi-pia̍t',
'unusedimages'            => 'Bô iōng ê iáⁿ-siōng',
'popularpages'            => 'Sî-kiâⁿ ê ia̍h',
'wantedcategories'        => 'wantedcategories',
'wantedpages'             => 'Beh ti̍h ê ia̍h',
'mostlinked'              => 'Siōng chia̍p liân-kiat ê ia̍h',
'mostlinkedcategories'    => 'Siōng chia̍p liân-kiat ê lūi-pia̍t',
'mostlinkedtemplates'     => 'Siōng chia̍p liân-kiat ê pang-bô͘',
'mostcategories'          => 'Siōng chē lūi-pia̍t ê ia̍h',
'mostimages'              => 'Siōng chia̍p liân-kiat ê iáⁿ-siōng',
'mostrevisions'           => 'Siōng chia̍p siu-kái ê ia̍h',
'prefixindex'             => 'Sû-thâu sek-ín',
'shortpages'              => 'Té-ia̍h',
'deadendpages'            => 'Khu̍t-thâu-ia̍h',
'deadendpagestext'        => 'Ē-kha ê ia̍h bô liân kàu wiki lāi-té ê kî-thaⁿ ia̍h.',
'protectedpages'          => 'Siū pó-hō͘ ê ia̍h',
'protectedpagestext'      => 'Ē-kha ê ia̍h siū pó-hō͘, bē-tit soá-ūi ia̍h pian-chi̍p',
'listusers'               => 'Iōng-chiá lia̍t-toaⁿ',
'newpages'                => 'Sin ia̍h',
'newpages-username'       => 'Iōng-chiá miâ-chheng:',
'ancientpages'            => 'Kó·-ia̍h',
'move'                    => 'sóa-ūi',
'movethispage'            => 'Sóa chit ia̍h',
'unusedimagestext'        => '<p>Chhiáⁿ chù-ì: kî-thaⁿ ê bāng-chām ū khó-lêng iōng URL ti̍t-chiap liân kàu iáⁿ-siōng, só·-í sui-jiân chhiâng-chāi teh iōng, mā sī ē lia̍t tī chia.</p>',
'unusedcategoriestext'    => 'Ū ē-kha chiah-ê lūi-pia̍t-ia̍h, m̄-koh bô kî-thaⁿ ê bûn-chiuⁿ a̍h-sī lūi-pia̍t lī-iōng.',

# Book sources
'booksources' => 'Tô͘-su chu-liāu',

# Special:Log
'specialloguserlabel'  => 'Iōng-chiá:',
'speciallogtitlelabel' => 'Sû-tiâu:',
'logempty'             => 'Log lāi-bīn bô sio-tùi ê hāng-bo̍k.',

# Special:AllPages
'allpages'          => 'Só·-ū ê ia̍h',
'alphaindexline'    => '$1 kàu $2',
'nextpage'          => 'Āu 1 ia̍h ($1)',
'allpagesfrom'      => 'Tùi chit ia̍h khai-sí hián-sī:',
'allarticles'       => 'Só·-ū ê bûn-chiuⁿ',
'allinnamespace'    => 'Só·-ū ê ia̍h ($1 miâ-khong-kan)',
'allnotinnamespace' => 'Só·-ū ê ia̍h (bô tī $1 miâ-khong-kan)',
'allpagesprev'      => 'Téng 1 ê',
'allpagesnext'      => 'ē 1 ê',
'allpagessubmit'    => 'Lâi-khì',

# Special:Categories
'categories'         => 'Lūi-pia̍t',
'categoriespagetext' => 'Chit ê wiki ū ē-kha chia ê lūi-pia̍t.',

# E-mail user
'mailnologin'     => 'Bô siu-phoe ê chū-chí',
'mailnologintext' => 'Lí it-tēng ài [[Special:UserLogin|teng-ji̍p]] jī-chhiáⁿ ū 1 ê ū-hāu ê e-mail chū-chí tī lí ê [[Special:Preferences|iōng-chiá siat-tēng]] chiah ē-tàng kià e-mail hō· pa̍t-ūi iōng-chiá.',
'emailuser'       => 'Kià e-mail hō· iōng-chiá',
'emailpage'       => 'E-mail iōng-chiá',
'emailpagetext'   => 'Ká-sú chit ê iōng-chiá ū siat-tēng 1 ê ū-hāu ê e-mail chū-chí, lí tō ē-tàng ēng ē-kha chit tiuⁿ FORM hoat sìn-sek hō· i. Lí siat-tēng ê e-mail chū-chí ē chhut-hiān tī e-mail ê "Kià-phoe-jîn" (From) hit ūi. Án-ne siu-phoe-jîn chiah ū hoat-tō· kā lí hôe-phoe.',
'noemailtitle'    => 'Bô e-mail chū-chí',
'noemailtext'     => 'Chit ūi iōng-chiá pēng-bô lâu ū-hāu ê e-mail chū-chí, bô tio̍h-sī i bô beh chiap-siū pat-ūi iōng-chiá ê e-mail.',
'emailfrom'       => 'Lâi chū',
'emailto'         => 'Khì hō·',
'emailsubject'    => 'Tê-bo̍k',
'emailmessage'    => 'Sìn-sit',
'emailsend'       => 'Sàng chhut-khì',
'emailsent'       => 'E-mail sàng chhut-khì ah',
'emailsenttext'   => 'Lí ê e-mail í-keng sàng chhut-khì ah.',

# Watchlist
'watchlist'            => 'Kàm-sī-toaⁿ',
'mywatchlist'          => 'Góa ê kàm-sī-toaⁿ',
'watchlistfor'         => "('''$1''' ê)",
'nowatchlist'          => 'Lí ê kàm-sī-toaⁿ bô pòaⁿ hāng.',
'watchnologin'         => 'Bô teng-ji̍p',
'watchnologintext'     => 'Lí it-tēng ài [[Special:UserLogin|teng-ji̍p]] chiah ē-tàng siu-kái lí ê kàm-sī-toaⁿ.',
'addedwatch'           => 'Í-keng ka-ji̍p kàm-sī-toaⁿ',
'addedwatchtext'       => "\"[[:\$1]]\" chit ia̍h í-keng ka-ji̍p lí ê [[Special:Watchlist|kàm-sī-toaⁿ]]. Bī-lâi chit ia̍h a̍h-sī siong-koan ê thó-lūn-ia̍h nā ū kái-piàn, ē lia̍t tī hia. Tông-sî tī [[Special:RecentChanges|Chòe-kīn ê kái-piàn]] ē iōng '''chho·-thé''' hián-sī ia̍h ê piau-tê, án-ne khah bêng-hián. Ká-sú lí beh chiōng chit ia̍h tùi lí ê kàm-sī-toaⁿ tû tiāu, khì khòng-chè-tiâu chhi̍h \"Mài kàm-sī\" chiū ē-sái-tit.",
'removedwatch'         => 'Í-keng tùi kàm-sī-toaⁿ tû tiāu',
'removedwatchtext'     => '"[[:$1]]" chit ia̍h í-keng tùi lí ê kàm-sī-toaⁿ tû tiāu.',
'watch'                => 'kàm-sī',
'watchthispage'        => 'Kàm-sī chit ia̍h',
'unwatch'              => 'Mài kàm-sī',
'unwatchthispage'      => 'Mài koh kàm-sī',
'watchnochange'        => 'Lí kàm-sī ê hāng-bo̍k tī hián-sī ê sî-kî í-lāi lóng bô siu-kái kòe.',
'watchlist-details'    => 'Kàm-sī-toaⁿ ū {{PLURAL:$1|$1 ia̍h|$1 ia̍h}}, thó-lūn-ia̍h bô sǹg chāi-lāi.',
'watchmethod-recent'   => 'tng teh kíam-cha choè-kīn ê siu-kái, khoàⁿ ū kàm-sī ê ia̍h bô',
'watchmethod-list'     => 'tng teh kiám-cha kàm-sī ê ia̍h khoàⁿ chòe-kīn ū siu-kái bô',
'watchlistcontains'    => 'Lí ê kàm-sī-toaⁿ siu $1 ia̍h.',
'wlnote'               => "Ē-kha sī '''$2''' tiám-cheng í-lāi siōng sin ê $1 ê kái-piàn.",
'wlshowlast'           => 'Hián-sī chêng $1 tiám-cheng $2 ji̍t $3',
'watchlist-show-bots'  => 'Hián-sī bot ê pian-chi̍p',
'watchlist-hide-bots'  => 'Am-khàm bot ê pian-chi̍p',
'watchlist-show-own'   => 'Hián-sī góa ê pian-chi̍p',
'watchlist-hide-own'   => 'Am-khàm góa ê pian-chi̍p',
'watchlist-show-minor' => 'Hián-sī sió siu-kái',
'watchlist-hide-minor' => 'Am-khàm sió siu-kái',

# Delete/protect/revert
'deletepage'        => 'Thâi ia̍h',
'confirm'           => 'Khak-tēng',
'excontent'         => "lōe-iông sī: '$1'",
'excontentauthor'   => "loē-iông sī: '$1' (î-it ê kòng-hiàn-chiá sī '[[Special:Contributions/$2|$2]]')",
'exbeforeblank'     => "chìn-chêng ê lōe-iông sī: '$1'",
'exblank'           => 'ia̍h khang-khang',
'historywarning'    => 'Kéng-kò: Lí beh thâi ê ia̍h ū le̍k-sú:',
'confirmdeletetext' => 'Lí tih-beh kā 1 ê ia̍h a̍h-sī iáⁿ-siōng (pau-koat siong-koan ê le̍k-sú) éng-kiú tùi chu-liāu-khò· thâi tiāu. Chhiáⁿ khak-tēng lí àn-sǹg án-ne chò, jī-chhiáⁿ liáu-kái hiō-kó, jī-chhiáⁿ bô ûi-hoán [[{{MediaWiki:Policy-url}}]].',
'actioncomplete'    => 'Chip-hêng sêng-kong',
'deletedtext'       => '"<nowiki>$1</nowiki>" í-keng thâi tiāu. Tùi $2 khoàⁿ-ē-tio̍h chòe-kīn thâi ê kì-lo̍k.',
'deletedarticle'    => 'Thâi tiāu "[[$1]]"',
'dellogpagetext'    => 'Í-hā lia̍t chhut chòe-kīn thâi tiāu ê hāng-bo̍k.',
'deletecomment'     => 'Thâi ê lí-iû',
'rollback'          => 'Kā siu-kái ká tńg khì',
'rollback_short'    => 'Ká tńg khì',
'rollbacklink'      => 'ká tńg khì',
'rollbackfailed'    => 'Ká bē tńg khì',
'cantrollback'      => 'Bô-hoat-tō· kā siu-kái ká-tńg--khì; téng ūi kòng-hiàn-chiá sī chit ia̍h î-it ê chok-chiá.',
'alreadyrolled'     => 'Bô-hoat-tō· kā [[User:$2|$2]] ([[User talk:$2|Thó-lūn]]) tùi [[:$1]] ê siu-kái ká-tńg-khì; í-keng ū lâng siu-kái a̍h-sī ká-tńg chit ia̍h. Téng 1 ūi siu-kái-chiá sī [[User:$3|$3]] ([[User talk:$3|Thó-lūn]]).',
'editcomment'       => 'Siu-kái phêng-lūn sī: "<i>$1</i>".', # only shown if there is an edit comment
'protectedarticle'  => 'pó-hō͘ "[[$1]]"',
'protect-title'     => 'Pó-hō· "$1"',
'protect-legend'    => 'Khak-tēng beh pó-hō·',
'protectcomment'    => 'Pó-hō· ê lí-iû:',
'protect-cascade'   => 'Cascading protection - pó-hō͘ jīm-hô pau-hâm tī chit ia̍h ê ia̍h.',

# Restrictions (nouns)
'restriction-edit' => 'Siu-kái',
'restriction-move' => 'Sóa khì',

# Undelete
'undelete'         => 'Kiù thâi tiāu ê ia̍h',
'undeletepage'     => 'Khoàⁿ kap kiù thâi tiāu ê ia̍h',
'undeletedarticle' => 'kiù "[[$1]]"',

# Namespace form on various pages
'namespace' => 'Miâ-khong-kan:',
'invert'    => 'Soán-hāng í-gōa',

# Contributions
'contributions' => 'Iōng-chiá ê kòng-hiàn',
'mycontris'     => 'Góa ê kòng-hiàn',
'nocontribs'    => 'Chhōe bô tiâu-kiāⁿ ū-tùi ê hāng-bo̍k.',
'uctop'         => '(siōng téng ê)',
'month'         => 'Kàu tó 1 kó͘ goe̍h ûi-chí:',
'year'          => 'Kàu tó 1 nî ûi-chí:',

'sp-contributions-newbies'     => 'Kan-taⁿ hián-sī sin kháu-chō ê kòng-kiàn',
'sp-contributions-newbies-sub' => 'Sin lâi--ê',
'sp-contributions-search'      => 'Chhoē chhut kòng-kiàn',
'sp-contributions-username'    => 'IP Chū-chí a̍h iōng-chiá miâ:',
'sp-contributions-submit'      => 'Chhoē',

# What links here
'whatlinkshere'       => 'Tó-ūi liân kàu chia',
'linklistsub'         => '(Liân-kiat lia̍t-toaⁿ)',
'linkshere'           => "Í-hā '''[[:$1]]''' liân kàu chia:",
'nolinkshere'         => "Bô poàⁿ ia̍h liân kàu '''[[:$1]]'''.",
'isredirect'          => 'choán-ia̍h',
'whatlinkshere-prev'  => '{{PLURAL:$1|chêng|chêng $1 ê}}',
'whatlinkshere-next'  => '{{PLURAL:$1|āu|āu $1 ê}}',
'whatlinkshere-links' => '← Liân kàu chia',

# Block/unblock
'blockip'            => 'Hong-só iōng-chiá',
'ipbreason'          => 'Lí-iû',
'ipbsubmit'          => 'Hong-só chit ūi iōng-chiá',
'badipaddress'       => 'Bô-hāu ê IP chū-chí',
'blockipsuccesssub'  => 'Hong-só sêng-kong',
'blockipsuccesstext' => '[[Special:Contributions/$1|$1]] í-keng pī hong-só. <br />Khì [[Special:IPBlockList|IP hong-só lia̍t-toaⁿ]] review hong-só ê IP.',
'ipusubmit'          => 'Chhú-siau hong-só chit ê chū-chí',
'ipblocklist'        => 'Siū hong-só ê IP chū-chí kap iōng-chiá miâ-chheng',
'blocklink'          => 'hong-só',
'contribslink'       => 'kòng-hiàn',
'autoblocker'        => 'Chū-tōng kìm-chí lí sú-iōng, in-ūi lí kap "$1" kong-ke kāng 1 ê IP chū-chí (kìm-chí lí-iû "$2").',
'blocklogentry'      => 'hong-só [[$1]], siat kî-hān chì $2 $3',
'blocklogtext'       => 'Chit-ê log lia̍t-chhut block/unblock ê tōng-chok. Chū-tōng block ê IP chū-chí bô lia̍t--chhut-lâi ([[Special:IPBlockList]] ū hiān-chú-sî ū-hāu ê block/ban o·-miâ-toaⁿ).',

# Developer tools
'locknoconfirm' => 'Lí bô kau "khak-tēng" ê keh-á.',

# Move page
'move-page-legend' => 'Sóa ia̍h',
'movepagetext'     => "Ē-kha chit ê form> iōng lâi kái 1 ê ia̍h ê piau-tê (miâ-chheng); só·-ū siong-koan ê le̍k-sú ē tòe leh sóa khì sin piau-tê.
Kū piau-tê ē chiâⁿ-chò 1 ia̍h choán khì sin piau-tê ê choán-ia̍h.
Liân khì kū piau-tê ê liân-kiat (link) bē khì tāng--tio̍h; ē-kì-tit chhiau-chhōe siang-thâu (double) ê a̍h-sī kò·-chiòng ê choán-ia̍h.
Lí ū chek-jīm khak-tēng liân-kiat kè-sio̍k liân tio̍h ūi.

Sin piau-tê nā í-keng tī leh (bô phian-chi̍p koè ê khang ia̍h, choán-ia̍h bô chún-sǹg), tō bô-hoat-tō· soá khì hia.
Che piaú-sī nā ū têng-tâⁿ, ē-sái kā sin ia̍h soà tńg-khì goân-lâi ê kū ia̍h.

'''SÈ-JĪ!'''
Tùi chē lâng tha̍k ê ia̍h lâi kóng, soá-ūi sī toā tiâu tāi-chì.
Liâu--lo̍h-khì chìn-chêng, chhiáⁿ seng khak-tēng lí ū liáu-kái chiah-ê hiō-kó.",
'movepagetalktext' => "Siong-koan ê thó-lūn-ia̍h (chún ū) oân-nâ ē chū-tōng tòe leh sóa-ūi. Í-hā ê chêng-hêng '''bô chún-sǹg''': *Beh kā chit ia̍h tùi 1 ê miâ-khong-kan (namespace) soá khì lēng-gōa 1 ê miâ-khong-kan, *Sin piau-tê í-keng ū iōng--kòe ê thó-lūn-ia̍h, he̍k-chiá *Ē-kha ê sió-keh-á bô phah-kau. Í-siōng ê chêng-hêng nā-chún tī leh, lí chí-hó iōng jîn-kang ê hong-sek sóa ia̍h a̍h-sī kā ha̍p-pèng (nā ū su-iàu).",
'movearticle'      => 'Sóa ia̍h:',
'newtitle'         => 'Khì sin piau-tê:',
'move-watch'       => 'Kàm-sī chit ia̍h',
'movepagebtn'      => 'Sóa ia̍h',
'pagemovedsub'     => 'Sóa-ūi sêng-kong',
'articleexists'    => 'Kāng miâ ê ia̍h í-keng tī leh, a̍h-sī lí kéng ê miâ bô-hāu. Chhiáⁿ kéng pa̍t ê miâ.',
'talkexists'       => "'''Ia̍h ê loē-bûn ū soá cháu, m̄-koh siong-koan ê thó-lūn-ia̍h bô toè leh soá, in-ūi sin piau-tê pun-té tō ū hit ia̍h. Chhiáⁿ iōng jîn-kang ê hoat-tō· kā ha̍p-pèng.'''",
'movedto'          => 'sóa khì tī',
'movetalk'         => 'Sūn-sòa sóa thó-lūn-ia̍h',
'1movedto2'        => '[[$1]] sóa khì tī [[$2]]',
'1movedto2_redir'  => '[[$1]] sóa khì [[$2]] (choán-ia̍h thiàu kòe)',
'movelogpagetext'  => 'Ē-kha lia̍t-chhut hông soá-ūi ê ia̍h.',
'movereason'       => 'Lí-iû:',
'selfmove'         => 'Goân piau-tê kap sin piau-tê sio-siâng; bô hoat-tō· sóa.',

# Export
'export'        => 'Su-chhut ia̍h',
'exportcuronly' => 'Hān hiān-chhú-sî ê siu-téng-pún, mài pau-koat kui-ê le̍k-sú',

# Namespace 8 related
'allmessages'         => 'Hē-thóng sìn-sit',
'allmessagesname'     => 'Miâ',
'allmessagesdefault'  => 'Siat piān ê bûn-jī',
'allmessagescurrent'  => 'Bo̍k-chêng ê bûn-jī',
'allmessagestext'     => 'Chia lia̍t chhut só·-ū tī MediaWiki: miâ-khong-kan ê hē-thóng sìn-sit.',
'allmessagesfilter'   => 'Thai sìn-sek ê tiâu-ba̍k:',
'allmessagesmodified' => 'Kan-taⁿ hián-sī kái--kòe ê',

# Thumbnails
'thumbnail-more' => 'Hòng-tōa',
'filemissing'    => 'Bô tóng-àn',

# Special:Import
'import' => 'Su-ji̍p ia̍h',

# Attribution
'anonymous'     => '{{SITENAME}} bô kì-miâ ê iōng-chiá',
'siteuser'      => '{{SITENAME}} iōng-chiá $1',
'othercontribs' => 'Kin-kù $1 ê kòng-hiàn.',
'siteusers'     => '{{SITENAME}} iōng-chiá $1',

# Math options
'mw_math_png'    => 'Tiāⁿ-tio̍h iōng PNG render',
'mw_math_simple' => 'Tân-sûn ê chêng-hêng iōng HTML; kî-thaⁿ iōng PNG',
'mw_math_html'   => 'Chīn-liōng iōng HTML; kî-thaⁿ iōng PNG',
'mw_math_source' => 'Î-chhî TeX ê keh-sek (khah ha̍h bûn-jī-sek ê liû-lám-khì)',
'mw_math_modern' => 'Kiàn-gī hiān-tāi liû-lám-khì kéng che',
'mw_math_mathml' => 'Chīn-liōng iōng MathML (chhì-giām-sèng--ê)',

# Patrolling
'markaspatrolleddiff'   => 'Phiau-sī sûn--kòe',
'markedaspatrolledtext' => 'Í-keng phiau-sī chit ê siu-tēng-pún ū lâng sûn--kòe.',

# Image deletion
'deletedrevision' => 'Kū siu-tēng-pún $1 thâi-tiāu ā.',

# Browsing diffs
'previousdiff' => '← Khì chêng 1 ê diff',
'nextdiff'     => 'Khì āu 1 ê diff →',

# Media information
'imagemaxsize'         => 'Iáⁿ-siōng biô-su̍t-ia̍h ê tô· ke̍k-ke hián-sī jōa tōa tiuⁿ:',
'thumbsize'            => 'Sok-tô· (thumbnail) jōa tōa tiuⁿ:',
'file-nohires'         => '<small>Bô khah koân ê kái-sek-tō͘.</small>',
'show-big-image-thumb' => '<small>Chit tiuⁿ ū-lám tô͘ (preview) ê toā-sè: $1 × $2 pixel</small>',

# Special:NewImages
'newimages'     => 'Sin iáⁿ-siōng oē-lóng',
'imagelisttext' => "Í-hā sī '''$1''' tiuⁿ iáⁿ-siōng ê lia̍t-toaⁿ, $2 pâi-lia̍t.",
'ilsubmit'      => 'Kiám-sek',
'bydate'        => 'chiàu ji̍t-kî',

# Metadata
'metadata-expand'   => 'Hián-sī iù-chiat',
'metadata-collapse' => 'Am iù-chiat',

# External editor support
'edit-externally'      => 'Iōng gōa-pō· èng-iōng nńg-thé pian-chi̍p chit-ê tóng-àn',
'edit-externally-help' => 'Chham-khó [http://www.mediawiki.org/wiki/Manual:External_editors Help:External_editors] ê soat-bêng.',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'choân-pō·',
'watchlistall2'    => 'choân-pō͘',
'namespacesall'    => 'choân-pō·',
'monthsall'        => 'choân-pō͘',

# E-mail address confirmation
'confirmemail'          => 'Khak-jīn e-mail chū-chí',
'confirmemail_text'     => 'Sú-iōng e-mail kong-lêng chìn-chêng tio̍h seng khak-jīn lí ê e-mail chū-chí ū-hāu. Chhi̍h ē-pêng hit-ê liú-á thang kià 1 tiuⁿ khak-jīn phoe hō· lí. Hit tiuⁿ phoe lāi-bīn ū 1 ê te̍k-sû liân-kiat. Chhiáⁿ iōng liû-lám-khì khui lâi khoàⁿ, án-ne tō ē-tit khak-jīn lí ê chū-chí ū-hāu.',
'confirmemail_send'     => 'Kià khak-jīn phoe',
'confirmemail_sent'     => 'Khak-jīn phoe kià chhut-khì ah.',
'confirmemail_invalid'  => 'Bô-hāu ê khak-jīn pian-bé. Pian-bé khó-lêng í-keng kòe-kî.',
'confirmemail_success'  => 'í ê e-mail chū-chí khak-jīn oân-sêng. Lí ē-sái teng-ji̍p, khai-sí hiáng-siū chit ê wiki.',
'confirmemail_loggedin' => 'Lí ê e-mail chū-chí í-keng khak-jīn ū-hāu.',
'confirmemail_error'    => 'Pó-chûn khak-jīn chu-sìn ê sî-chūn hoat-seng būn-tê.',
'confirmemail_subject'  => '{{SITENAME}} e-mail chu-chi khak-jin phoe',
'confirmemail_body'     => 'Ū lâng (IP $1, tāi-khài sī lí pún-lâng) tī {{SITENAME}} ēng chit-ê e-mail chū-chí chù-chheh 1 ê kháu-chō "$2".

Chhiáⁿ khui ē-kha chit-ê liân-kiat, thang khak-jīn chit-ê kháu-chō si̍t-chāi sī lí ê:

$3

Nā-chún *m̄-sī* lí, chhiáⁿ mài tòe liân-kiat khì.  Chit tiuⁿ phoe ê khak-jīn-bé ē chū-tōng tī $4 kòe-kî.',

# action=purge
'confirm_purge' => 'Kā chit ia̍h ê cache piàⁿ tiāu? $1',

# Table pager
'table_pager_next'         => 'Aū-chi̍t-ia̍h',
'table_pager_prev'         => 'Téng-chi̍t-ia̍h',
'table_pager_first'        => 'Thâu-chi̍t-ia̍h',
'table_pager_last'         => 'Siāng-bóe-ia̍h',
'table_pager_limit'        => 'Múi 1 ia̍h hián-sī $1 hāng',
'table_pager_limit_submit' => 'Lâi-khì',

# Auto-summaries
'autosumm-blank'   => 'Kā ia̍h ê loē-iông the̍h tiāu',
'autoredircomment' => 'Choán khì [[$1]]',
'autosumm-new'     => 'Sin ia̍h: $1',

# Watchlist editor
'watchlistedit-numitems'      => 'Lí ê kàm-sī-toaⁿ ū $1 ia̍h, thó-lūn-ia̍h bô sǹg chāi-lāi.',
'watchlistedit-normal-submit' => 'Mài kàm-sī',
'watchlistedit-normal-done'   => 'Í-keng ū $1 ia̍h ùi lí ê kám-sī-toaⁿ soá cháu:',

# Special:Version
'version' => 'Pán-pún', # Not used as normal message but as header for the special page itself

# Special:FilePath
'filepath' => 'Tóng-àn ê soàⁿ-lō·',

# Special:SpecialPages
'specialpages' => 'Te̍k-sû-ia̍h',

);
