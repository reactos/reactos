<?php
/** Marathi (मराठी)
 *
 * @ingroup Language
 * @file
 *
 * @author Angela
 * @author Harshalhayat
 * @author Hemanshu
 * @author Kaustubh
 * @author Mahitgar
 * @author Sankalpdravid
 * @author अभय नातू
 * @author कोलࣿहापࣿरी
 * @author प्रणव कुलकर्णी
 * @author शࣿरीहरि
 */

$specialPageAliases = array(
	'DoubleRedirects'           => array( 'दुहेरी_पुनर्निर्देशने' ),
	'BrokenRedirects'           => array( 'चुकीची_पुनर्निर्देशने' ),
	'Disambiguations'           => array( 'नि:संदिग्धीकरण' ),
	'Preferences'               => array( 'पसंती' ),
	'Watchlist'                 => array( 'पहार्‍याची_सूची' ),
	'Recentchanges'             => array( 'अलीकडील_बदल' ),
	'Upload'                    => array( 'चढवा' ),
	'Imagelist'                 => array( 'चित्रयादी' ),
	'Newimages'                 => array( 'नवीन_चित्रे' ),
	'Listusers'                 => array( 'सदस्यांची_यादी' ),
	'Statistics'                => array( 'सांख्यिकी' ),
	'Randompage'                => array( 'अविशिष्ट', 'अविशिष्ट_पृष्ठ' ),
	'Uncategorizedpages'        => array( 'अवर्गीकृत_पाने' ),
	'Uncategorizedcategories'   => array( 'अवर्गीकृत_वर्ग' ),
	'Uncategorizedimages'       => array( 'अवर्गीकृत_चित्रे' ),
	'Uncategorizedtemplates'    => array( 'अवर्गीकृत_साचे' ),
	'Unusedcategories'          => array( 'न_वापरलेले_वर्ग' ),
	'Unusedimages'              => array( 'न_वापरलेली_चित्रे' ),
	'Wantedpages'               => array( 'हवे_असलेले_लेख' ),
	'Wantedcategories'          => array( 'हवे_असलेले_वर्ग' ),
	'Shortpages'                => array( 'छोटी_पाने' ),
	'Longpages'                 => array( 'मोठी_पाने' ),
	'Newpages'                  => array( 'नवीन_पाने' ),
	'Ancientpages'              => array( 'जुनी_पाने' ),
	'Deadendpages'              => array( 'टोकाची_पाने' ),
	'Protectedpages'            => array( 'सुरक्षित_पाने' ),
	'Allpages'                  => array( 'सर्व_पाने' ),
	'Whatlinkshere'             => array( 'येथे_काय_जोडले_आहे' ),
	'Categories'                => array( 'वर्ग' ),
	'Listadmins'                => array( 'प्रबंधकांची_यादी' ),
	'Listbots'                  => array( 'सांगकाम्यांची_यादी' ),
	'Search'                    => array( 'शोधा' ),
	'Filepath'                  => array( 'संचिकेचा_पत्ता_(पाथ)' ),
);

$skinNames = array(
	'standard'    => 'अभिजात',
	'nostalgia'   => 'रम्य',
	'cologneblue' => 'सुरेखनीळी',
	'monobook'    => 'मोनोबुक',
	'myskin'      => 'माझीकांती',
	'chick'       => 'मस्त',
	'simple'      => 'साधी',
	'modern'      => 'आधुनिक',
);

$namespaceNames = array(
	NS_MEDIA          => 'मिडिया',
	NS_SPECIAL        => 'विशेष',
	NS_MAIN           => '',
	NS_TALK           => 'चर्चा',
	NS_USER           => 'सदस्य',
	NS_USER_TALK      => 'सदस्य_चर्चा',
	# NS_PROJECT set by \$wgMetaNamespace
	NS_PROJECT_TALK   => '$1_चर्चा',
	NS_IMAGE          => 'चित्र',
	NS_IMAGE_TALK     => 'चित्र_चर्चा',
	NS_MEDIAWIKI      => 'मिडियाविकी',
	NS_MEDIAWIKI_TALK => 'मिडियाविकी_चर्चा',
	NS_TEMPLATE       => 'साचा',
	NS_TEMPLATE_TALK  => 'साचा_चर्चा',
	NS_HELP           => 'साहाय्य',
	NS_HELP_TALK      => 'साहाय्य_चर्चा',
	NS_CATEGORY       => 'वर्ग',
	NS_CATEGORY_TALK  => 'वर्ग_चर्चा',
);

$magicWords = array(
	'redirect'            => array( '0', '#REDIRECT', '#पुनर्निर्देशन' ),
	'notoc'               => array( '0', '__NOTOC__', '__अनुक्रमाणिकानको__' ),
	'forcetoc'            => array( '0', '__FORCETOC__', '__अनुक्रमाणिकाहवीच__' ),
	'noeditsection'       => array( '0', '__NOEDITSECTION__', '__असंपादनक्षम__' ),
	'currentmonth'        => array( '1', 'CURRENTMONTH', 'सद्यमहिना' ),
	'currentmonthname'    => array( '1', 'CURRENTMONTHNAME', 'सद्यमहिनानाव' ),
	'currentmonthabbrev'  => array( '1', 'CURRENTMONTHABBREV', 'सद्यमहिनासंक्षीप्त' ),
	'currentday'          => array( '1', 'CURRENTDAY', 'सद्यदिवस' ),
	'currentday2'         => array( '1', 'CURRENTDAY2', 'सद्यदिवस२' ),
	'currentdayname'      => array( '1', 'CURRENTDAYNAME', 'सद्यदिवसनाव' ),
	'currentyear'         => array( '1', 'CURRENTYEAR', 'सद्यवर्ष' ),
	'currenttime'         => array( '1', 'CURRENTTIME', 'सद्यवेळ' ),
	'currenthour'         => array( '1', 'CURRENTHOUR', 'सद्यतास' ),
	'localmonth'          => array( '1', 'LOCALMONTH', 'स्थानिकमहिना' ),
	'localmonthname'      => array( '1', 'LOCALMONTHNAME', 'स्थानिकमहिनानाव' ),
	'localmonthabbrev'    => array( '1', 'LOCALMONTHABBREV', 'स्थानिकमहिनासंक्षीप्त' ),
	'localday'            => array( '1', 'LOCALDAY', 'स्थानिकदिवस' ),
	'localday2'           => array( '1', 'LOCALDAY2', 'स्थानिकदिवस२' ),
	'localdayname'        => array( '1', 'LOCALDAYNAME', 'स्थानिकदिवसनाव' ),
	'localyear'           => array( '1', 'LOCALYEAR', 'स्थानिकवर्ष' ),
	'localtime'           => array( '1', 'LOCALTIME', 'स्थानिकवेळ' ),
	'localhour'           => array( '1', 'LOCALHOUR', 'स्थानिकतास' ),
	'numberofpages'       => array( '1', 'NUMBEROFPAGES', 'पानसंख्या' ),
	'numberofarticles'    => array( '1', 'NUMBEROFARTICLES', 'लेखसंख्या' ),
	'numberoffiles'       => array( '1', 'NUMBEROFFILES', 'संचिकासंख्या' ),
	'numberofusers'       => array( '1', 'NUMBEROFUSERS', 'सदस्यसंख्या' ),
	'numberofedits'       => array( '1', 'NUMBEROFEDITS', 'संपादनसंख्या' ),
	'pagename'            => array( '1', 'PAGENAME', 'लेखनाव' ),
	'namespace'           => array( '1', 'NAMESPACE', 'नामविश्व' ),
	'img_thumbnail'       => array( '1', 'thumbnail', 'thumb', 'इवलेसे' ),
	'img_manualthumb'     => array( '1', 'thumbnail=$1', 'thumb=$1', 'इवलेसे=$1' ),
	'img_right'           => array( '1', 'right', 'उजवे' ),
	'img_left'            => array( '1', 'left', 'डावे' ),
	'img_none'            => array( '1', 'none', 'कोणतेचनाही', 'नन्ना' ),
	'img_center'          => array( '1', 'center', 'centre', 'मध्यवर्ती' ),
	'img_middle'          => array( '1', 'middle', 'मध्य' ),
	'img_bottom'          => array( '1', 'bottom', 'तळ', 'बूड' ),
	'img_text_bottom'     => array( '1', 'text-bottom', 'मजकुरतळ' ),
	'sitename'            => array( '1', 'SITENAME', 'संकेतस्थळनाव' ),
	'ns'                  => array( '0', 'NS:', 'नावि' ),
	'server'              => array( '0', 'SERVER', 'विदादाता' ),
	'servername'          => array( '0', 'SERVERNAME', 'विदादातानाव' ),
	'grammar'             => array( '0', 'GRAMMAR:', 'व्याकरण:' ),
	'currentweek'         => array( '1', 'CURRENTWEEK', 'सद्यआठवडा' ),
	'localweek'           => array( '1', 'LOCALWEEK', 'स्थानिकआठवडा' ),
	'plural'              => array( '0', 'PLURAL:', 'बहुवचन:' ),
	'language'            => array( '0', '#LANGUAGE:', '#भाषा:', '' ),
	'contentlanguage'     => array( '1', 'CONTENTLANGUAGE', 'CONTENTLANG', 'मसुदाभाषा', 'मजकुरभाषा' ),
	'special'             => array( '0', 'special', 'विशेष' ),
	'filepath'            => array( '0', 'FILEPATH:', 'संचिकामार्ग:' ),
);

$digitTransformTable = array(
	'0' => '०', # &#x0966;
	'1' => '१', # &#x0967;
	'2' => '२', # &#x0968;
	'3' => '३', # &#x0969;
	'4' => '४', # &#x096a;
	'5' => '५', # &#x096b;
	'6' => '६', # &#x096c;
	'7' => '७', # &#x096d;
	'8' => '८', # &#x096e;
	'9' => '९', # &#x096f;
);

$linkTrail = "/^([\xE0\xA4\x80-\xE0\xA5\xA3\xE0\xA5\xB1-\xE0\xA5\xBF\xEF\xBB\xBF\xE2\x80\x8D]+)(.*)$/sDu";

$messages = array(
# User preference toggles
'tog-underline'               => 'दुव्यांना अधोरेखित करा:',
'tog-highlightbroken'         => 'चुकीचे दुवे <a href="" class="new">असे दाखवा</a> (किंवा: असे दाखवा<a href="" class="internal">?</a>).',
'tog-justify'                 => 'परिच्छेद समान करा',
'tog-hideminor'               => 'छोटे बदल लपवा',
'tog-extendwatchlist'         => 'पहार्‍याच्या सूचीत सर्व बदल दाखवा',
'tog-usenewrc'                => 'वाढीव अलीकडील बदल (जावास्क्रीप्ट)',
'tog-numberheadings'          => 'शीर्षके स्वयंक्रमांकित करा',
'tog-showtoolbar'             => 'संपादन चिन्हे दाखवा (जावास्क्रीप्ट)',
'tog-editondblclick'          => 'दोनवेळा क्लीक करुन पान संपादित करा (जावास्क्रीप्ट)',
'tog-editsection'             => '[संपादन] दुव्याने संपादन करणे शक्य करा',
'tog-editsectiononrightclick' => 'विभाग शीर्षकावर उजव्या क्लीकने संपादन करा(जावास्क्रीप्ट)',
'tog-showtoc'                 => '३ पेक्षा जास्त शीर्षके असताना अनुक्रमणिका दाखवा',
'tog-rememberpassword'        => 'माझा प्रवेश या संगणकावर लक्षात ठेवा',
'tog-editwidth'               => 'संपादन खिडकी पूर्ण रुंदीची दाखवा.',
'tog-watchcreations'          => 'मी तयार केलेली पाने माझ्या पहार्‍याच्या सूचीत टाका',
'tog-watchdefault'            => 'मी संपादित केलेली पाने माझ्या पहार्‍याच्या सूचीत टाका',
'tog-watchmoves'              => 'मी स्थानांतरीत केलेली पाने माझ्या पहार्‍याच्या सूचीत टाका',
'tog-watchdeletion'           => 'मी वगळलेली पाने माझ्या पहार्‍याच्या सूचीत टाका',
'tog-minordefault'            => 'सर्व संपादने ’छोटी’ म्हणून आपोआप जतन करा',
'tog-previewontop'            => 'झलक संपादन खिडकीच्या आधी दाखवा',
'tog-previewonfirst'          => 'पहिल्या संपादनानंतर झलक दाखवा',
'tog-nocache'                 => 'पाने सयी मध्ये ठेवू नका',
'tog-enotifwatchlistpages'    => 'माझ्या पहार्‍याच्या सूचीतील पान बदलल्यास मला विरोप (e-mail) पाठवा',
'tog-enotifusertalkpages'     => 'माझ्या चर्चा पानावर बदल झाल्यास मला विरोप पाठवा',
'tog-enotifminoredits'        => 'मला छोट्या बदलांकरीता सुद्धा विरोप पाठवा',
'tog-enotifrevealaddr'        => 'सूचना विरोपात माझा विरोपाचा पत्ता दाखवा',
'tog-shownumberswatching'     => 'पहारा दिलेले सदस्य दाखवा',
'tog-fancysig'                => 'साधी सही (कुठल्याही दुव्याशिवाय)',
'tog-externaleditor'          => 'कायम बाह्य संपादक वापरा (फक्त प्रशिक्षित सदस्यांसाठीच, संगणकावर विशेष प्रणाली लागते)',
'tog-externaldiff'            => 'इतिहास पानावर निवडलेल्या आवृत्त्यांमधील बदल दाखविण्यासाठी बाह्य प्रणाली वापरा (फक्त प्रशिक्षित सदस्यांसाठीच, संगणकावर विशेष प्रणाली लागते)',
'tog-showjumplinks'           => '"कडे जा" सुगम दुवे, उपलब्ध करा.',
'tog-uselivepreview'          => 'संपादन करता करताच झलक दाखवा (जावास्क्रीप्ट)(प्रयोगक्षम)',
'tog-forceeditsummary'        => 'जर ’बदलांचा आढावा’ दिला नसेल तर मला सूचित करा',
'tog-watchlisthideown'        => 'पहार्‍याच्या सूचीतून माझे बदल लपवा',
'tog-watchlisthidebots'       => 'पहार्‍याच्या सूचीतून सांगकामे बदल लपवा',
'tog-watchlisthideminor'      => 'माझ्या पहार्‍याच्या सूचीतून छोटे बदल लपवा',
'tog-ccmeonemails'            => 'मी इतर सदस्यांना पाठविलेल्या इमेल च्या प्रती मलाही पाठवा',
'tog-diffonly'                => 'निवडलेल्या आवृत्त्यांमधील बदल दाखवताना जुनी आवृत्ती दाखवू नका.',
'tog-showhiddencats'          => 'लपविलेले वर्ग दाखवा',

'underline-always'  => 'नेहमी',
'underline-never'   => 'कधीच नाही',
'underline-default' => 'न्याहाळक अविचल (browser default)',

'skinpreview' => '(झलक)',

# Dates
'sunday'        => 'रविवार',
'monday'        => 'सोमवार',
'tuesday'       => 'मंगळवार',
'wednesday'     => 'बुधवार',
'thursday'      => 'गुरूवार',
'friday'        => 'शुक्रवार',
'saturday'      => 'शनिवार',
'sun'           => 'रवि',
'mon'           => 'सोम',
'tue'           => 'मंगळ',
'wed'           => 'बुध',
'thu'           => 'गुरू',
'fri'           => 'शुक्र',
'sat'           => 'शनि',
'january'       => 'जानेवारी',
'february'      => 'फेब्रुवारी',
'march'         => 'मार्च',
'april'         => 'एप्रिल',
'may_long'      => 'मे',
'june'          => 'जून',
'july'          => 'जुलै',
'august'        => 'ऑगस्ट',
'september'     => 'सप्टेंबर',
'october'       => 'ऑक्टोबर',
'november'      => 'नोव्हेंबर',
'december'      => 'डिसेंबर',
'january-gen'   => 'जानेवारी',
'february-gen'  => 'फेब्रुवारी',
'march-gen'     => 'मार्च',
'april-gen'     => 'एप्रिल',
'may-gen'       => 'मे',
'june-gen'      => 'जून',
'july-gen'      => 'जुलै',
'august-gen'    => 'ऑगस्ट',
'september-gen' => 'सप्टेंबर',
'october-gen'   => 'ऑक्टोबर',
'november-gen'  => 'नोव्हेंबर',
'december-gen'  => 'डिसेंबर',
'jan'           => 'जाने.',
'feb'           => 'फेब्रु.',
'mar'           => 'मार्च',
'apr'           => 'एप्रि.',
'may'           => 'मे',
'jun'           => 'जून',
'jul'           => 'जुलै',
'aug'           => 'ऑग.',
'sep'           => 'सप्टें.',
'oct'           => 'ऑक्टो.',
'nov'           => 'नोव्हें.',
'dec'           => 'डिसें.',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|वर्ग|वर्ग}}',
'category_header'                => '"$1" वर्गातील लेख',
'subcategories'                  => 'उपवर्ग',
'category-media-header'          => '"$1" वर्गातील माध्यमे',
'category-empty'                 => "''या वर्गात अद्याप एकही लेख नाही.''",
'hidden-categories'              => '{{PLURAL:$1|लपविलेला वर्ग|लपविलेले वर्ग}}',
'hidden-category-category'       => 'लपविलेले वर्ग', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|या वर्गात फक्त खालील उपवर्ग आहे.|एकूण $2 उपवर्गांपैकी या वर्गात खालील {{PLURAL:$1|उपवर्ग आहे.|$1 उपवर्ग आहेत.}}}}',
'category-subcat-count-limited'  => 'या वर्गात खालील $1 उपवर्ग {{PLURAL:$1|आहे|आहेत}}.',
'category-article-count'         => '{{PLURAL:$2|या वर्गात फक्त खालील लेख आहे.|एकूण $2 पैकी खालील {{PLURAL:$1|पान|$1 पाने}} या वर्गात {{PLURAL:$1|आहे|आहेत}}.}}',
'category-article-count-limited' => 'खालील {{PLURAL:$1|पान|$1 पाने}} या वर्गात {{PLURAL:$1|आहे|आहेत}}.',
'category-file-count'            => '{{PLURAL:$2|या वर्गात फक्त खालील संचिका आहे.|एकूण $2 पैकी खालील {{PLURAL:$1|संचिका|$1 संचिका}} या वर्गात {{PLURAL:$1|आहे|आहेत}}.}}',
'category-file-count-limited'    => 'खालील {{PLURAL:$1|संचिका|$1 संचिका}} या वर्गात आहेत.',
'listingcontinuesabbrev'         => 'पुढे.',

'mainpagetext'      => "<big>'''मीडियाविकीचे इन्स्टॉलेशन पूर्ण'''</big>",
'mainpagedocfooter' => 'विकी सॉफ्टवेअर वापरण्याकरिता [http://meta.wikimedia.org/wiki/Help:Contents यूजर गाईड] पहा.

== सुरुवात ==

* [http://www.mediawiki.org/wiki/Manual:Configuration_settings कॉन्फिगरेशन सेटींगची यादी]
* [http://www.mediawiki.org/wiki/Manual:FAQ मीडियाविकी नेहमी विचारले जाणारे प्रश्न]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce मीडियाविकि मेलिंग लिस्ट]',

'about'          => 'च्या विषयी',
'article'        => 'लेख',
'newwindow'      => '(नवीन खिडकीत उघडते.)',
'cancel'         => 'रद्द करा',
'qbfind'         => 'शोध',
'qbbrowse'       => 'विचरण',
'qbedit'         => 'संपादन',
'qbpageoptions'  => 'हे पान',
'qbpageinfo'     => 'पृष्ठ जानकारी',
'qbmyoptions'    => 'माझी पाने',
'qbspecialpages' => 'विशेष पृष्ठे',
'moredotdotdot'  => 'अजून...',
'mypage'         => 'माझे पृष्ठ',
'mytalk'         => 'माझ्या चर्चा',
'anontalk'       => 'या अंकपत्त्याचे चर्चा पान उघडा',
'navigation'     => 'सुचालन',
'and'            => 'आणि',

# Metadata in edit box
'metadata_help' => 'मेटाडाटा:',

'errorpagetitle'    => 'चुक',
'returnto'          => '$1 कडे परत चला.',
'tagline'           => '{{SITENAME}} कडून',
'help'              => 'सहाय्य',
'search'            => 'शोधा',
'searchbutton'      => 'शोधा',
'go'                => 'चला',
'searcharticle'     => 'लेख',
'history'           => 'जुन्या आवृत्ती',
'history_short'     => 'इतिहास',
'updatedmarker'     => 'शेवटच्या भेटीनंतर बदलले',
'info_short'        => 'माहिती',
'printableversion'  => 'छापण्यायोग्य आवृत्ती',
'permalink'         => 'शाश्वत दुवा',
'print'             => 'छापा',
'edit'              => 'संपादन',
'create'            => 'तयार करा',
'editthispage'      => 'हे पृष्ठ संपादित करा',
'create-this-page'  => 'हे पान तयार करा',
'delete'            => 'वगळा',
'deletethispage'    => 'हे पृष्ठ काढून टाका',
'undelete_short'    => 'पुनर्स्थापन {{PLURAL:$1|एक संपादन|$1 संपादने}}',
'protect'           => 'सुरक्षित करा',
'protect_change'    => 'सुरक्षेचे नियम बदला',
'protectthispage'   => 'हे पृष्ठ सुरक्षित करा',
'unprotect'         => 'असुरक्षित करा',
'unprotectthispage' => 'हे पृष्ठ असुरक्षित करा',
'newpage'           => 'नवीन पृष्ठ',
'talkpage'          => 'चर्चा पृष्ठ',
'talkpagelinktext'  => 'चर्चा',
'specialpage'       => 'विशेष पृष्ठ',
'personaltools'     => 'वैयक्‍तिक साधने',
'postcomment'       => 'मत नोंदवा',
'articlepage'       => 'लेख पृष्ठ',
'talk'              => 'चर्चा',
'views'             => 'दृष्टीपथात',
'toolbox'           => 'साधनपेटी',
'userpage'          => 'सदस्य पृष्ठ',
'projectpage'       => 'प्रकल्प पान पहा',
'imagepage'         => 'चित्र पृष्ठ',
'mediawikipage'     => 'संदेश पान पहा',
'templatepage'      => 'साचा पृष्ठ पहा.',
'viewhelppage'      => 'साहाय्य पान पहा',
'categorypage'      => 'वर्ग पान पहा',
'viewtalkpage'      => 'चर्चा पृष्ठ पहा',
'otherlanguages'    => 'इतर भाषा',
'redirectedfrom'    => '($1 पासून पुनर्निर्देशित)',
'redirectpagesub'   => 'पुनर्निर्देशनाचे पान',
'lastmodifiedat'    => 'या पानातील शेवटचा बदल $1 रोजी $2 वाजता केला गेला.', # $1 date, $2 time
'viewcount'         => 'हे पान {{PLURAL:$1|एकदा|$1 वेळा}} बघितले गेलेले आहे.',
'protectedpage'     => 'सुरक्षित पृष्ठ',
'jumpto'            => 'येथे जा:',
'jumptonavigation'  => 'सुचालन',
'jumptosearch'      => 'शोधयंत्र',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => '{{SITENAME}} बद्दल',
'aboutpage'            => 'Project:माहितीपृष्ठ',
'bugreports'           => 'दोष अहवाल',
'bugreportspage'       => 'Project:दोष अहवाल',
'copyright'            => 'येथील मजकूर $1च्या अंतर्गत उपलब्ध आहे.',
'copyrightpagename'    => '{{SITENAME}} प्रताधिकार',
'copyrightpage'        => '{{ns:project}}:प्रताधिकार',
'currentevents'        => 'सद्य घटना',
'currentevents-url'    => 'Project:सद्य घटना',
'disclaimers'          => 'उत्तरदायकत्वास नकार',
'disclaimerpage'       => 'Project: सर्वसाधारण उत्तरदायकत्वास नकार',
'edithelp'             => 'संपादन साहाय्य',
'edithelppage'         => 'Help:संपादन',
'faq'                  => 'नेहमीची प्रश्नावली',
'faqpage'              => 'Project:प्रश्नावली',
'helppage'             => 'Help:साहाय्य पृष्ठ',
'mainpage'             => 'मुखपृष्ठ',
'mainpage-description' => 'मुखपृष्ठ',
'policy-url'           => 'Project:नीती',
'portal'               => 'समाज मुखपृष्ठ',
'portal-url'           => 'Project:समाज मुखपृष्ठ',
'privacy'              => 'गुप्तता नीती',
'privacypage'          => 'Project:गुप्तता नीती',

'badaccess'        => 'परवानगी नाकारण्यात आली आहे',
'badaccess-group0' => 'तुम्ही करत असलेल्या क्रियेचे तुम्हाला अधिकार नाहीत.',
'badaccess-group1' => 'फक्त $1 प्रकारचे सदस्य हे काम करू शकतात.',
'badaccess-group2' => 'आपण विनीत केलेली कृती समूहां $1 पैकी सदस्याकरिता मर्यादीत आहे.',
'badaccess-groups' => 'आपण विनीत केलेली कृती समूहां $1 पैकी सदस्याकरिता मर्यादीत आहे.',

'versionrequired'     => 'मीडियाविकीच्या $1 आवृत्तीची गरज आहे.',
'versionrequiredtext' => 'हे पान वापरण्यासाठी मीडियाविकीच्या $1 आवृत्तीची गरज आहे. पहा [[Special:Version|आवृत्ती यादी]].',

'ok'                      => 'ठीक',
'retrievedfrom'           => '"$1" पासून मिळविले',
'youhavenewmessages'      => 'तुमच्यासाठी $1 ($2).',
'newmessageslink'         => 'नवीन संदेश',
'newmessagesdifflink'     => 'ताजा बदल',
'youhavenewmessagesmulti' => '$1 वर तुमच्यासाठी नवीन संदेश आहेत.',
'editsection'             => 'संपादन',
'editold'                 => 'संपादन',
'viewsourceold'           => 'स्रोत पहा',
'editsectionhint'         => 'विभाग: $1 संपादा',
'toc'                     => 'अनुक्रमणिका',
'showtoc'                 => 'दाखवा',
'hidetoc'                 => 'लपवा',
'thisisdeleted'           => 'आवलोकन किंवा पूनर्स्थापन $1?',
'viewdeleted'             => 'आवलोकन $1?',
'restorelink'             => '{{PLURAL:$1|एक वगळलेले संपादन|$1 वगळलेली संपादने}}',
'feedlinks'               => 'रसद (Feed):',
'feed-invalid'            => 'अयोग्य रसद नोंदणी (Invalid subscription feed type).',
'feed-unavailable'        => '{{SITENAME}}वर सिंडीकेशन फीड उपलब्ध नाहीत',
'site-rss-feed'           => '$1 आरएसएस फीड',
'site-atom-feed'          => '$1 ऍटम रसद (Atom Feed)',
'page-rss-feed'           => '"$1" आर.एस.एस.रसद (RSS Feed)',
'page-atom-feed'          => '"$1" ऍटम रसद (Atom Feed)',
'feed-atom'               => 'ऍटम',
'feed-rss'                => 'आर.एस.ए‍स.',
'red-link-title'          => '$1 (अजून लिहीले नाही)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'लेख',
'nstab-user'      => 'सदस्य पान',
'nstab-media'     => 'माध्यम पान',
'nstab-special'   => 'विशेष',
'nstab-project'   => 'प्रकल्प पान',
'nstab-image'     => 'संचिका',
'nstab-mediawiki' => 'संदेश',
'nstab-template'  => 'साचा',
'nstab-help'      => 'साहाय्य पान',
'nstab-category'  => 'वर्ग',

# Main script and global functions
'nosuchaction'      => 'अशी कृती अस्तित्वात नाही',
'nosuchactiontext'  => 'URL ने सांगितलेली कृती विकिने ओळखली नाही.',
'nosuchspecialpage' => 'असे कोणतेही विशेष पृष्ठ अस्तित्वात नाही',
'nospecialpagetext' => "<big>'''आपण केलेली विनंती अयोग्य विशेषपानासंबंधी आहे.'''</big>

योग्य विशेषपानांची यादी  [[Special:SpecialPages|{{int:specialpages}}]] येथे उपलब्ध होऊ शकते.",

# General errors
'error'                => 'त्रुटी',
'databaseerror'        => 'माहितीसंग्रहातील त्रुटी',
'dberrortext'          => 'एक विदा पृच्छारचना त्रूटी घडली आहे.
ही बाब संचेतनात (सॉफ्टवेअरमध्ये) क्षितिजन्तु असण्याची शक्यता निर्देशीत करते.
"<tt>$2</tt>" या कार्यातून निघालेली शेवटची विदापृच्छा पुढील प्रमाणे होती:
<blockquote><tt>$1</tt></blockquote>
MySQL ने "<tt>$3: $4</tt>" ही त्रूटी दिलेली आहे.',
'dberrortextcl'        => 'चुकीच्या प्रश्नलेखनामुळे माहितीसंग्रह त्रुटी.
शेवटची माहितीसंग्रहाला पाठविलेला प्रश्न होता:
"$1"
"$2" या कार्यकृतीमधून .
MySQL returned error "$3: $4".',
'noconnect'            => 'क्षमस्व! विकिस तांत्रिक अडचण भेडसावत असल्यामुळे तो विदागारास संपर्क करू शकत नाही.<br />$1',
'nodb'                 => '$1 विदागार निवडता आला नाही.',
'cachederror'          => 'खाली दिलेली प्रत ही मागितलेल्या पानाची सयीतील आवृत्ती आहे, ही कदाचित अद्ययावत असणार नाही.',
'laggedslavemode'      => 'सुचना: पानावर नवीन बदल नसतील.',
'readonly'             => 'विदागारास (database) ताळे आहे.',
'enterlockreason'      => 'विदागारास ताळे ठोकण्याचे कारण, ताळे उघडले जाण्याच्या अदमासे कालावधीसहीत द्या.',
'readonlytext'         => 'बहुधा विदागार मेंटेनन्सकरिता नवीन भर घालण्यापासून आणि इतर बदल करण्यापासून बंद ठेवण्यात आला आहे, मेंटेनन्सनंतर तो नियमीत होईल.

ताळे ठोकणार्‍या प्रबंधकांनी खालील कारण नमुद केले आहे: $1',
'missing-article'      => 'डाटाबेसला "$1" $2 नावाचे पान मिळालेले नाही, जे मिळायला हवे होते.

असे बहुदा संपुष्टात आलेल्या फरकामुळे किंवा वगळलेल्या पानाच्या इतिहास दुव्यामुळे घडते.

जर असे घडलेले नसेल, तर तुम्हाला प्रणाली मधील त्रुटी आढळलेली असू शकते.
कृपया याबद्दल एखाद्या प्रबंधकाशी चर्चा करा व या URLची नोंद करा.',
'missingarticle-rev'   => '(आवृत्ती#: $1)',
'missingarticle-diff'  => '(फरक: $1, $2)',
'readonly_lag'         => 'मुख्य विदागार दात्याच्या (master database server) बरोबरीने पोहचण्यास पराधीन-विदागारदात्यास (slave server) वेळ लागल्यामुळे, विदागार आपोआप बंद झाला आहे.',
'internalerror'        => 'अंतर्गत त्रूटी',
'internalerror_info'   => 'अंतर्गत त्रूटी: $1',
'filecopyerror'        => '"$1" संचिकेची "$2" ही प्रत करता आली नाही.',
'filerenameerror'      => '"$1" संचिकेचे "$2" असे नामांतर करता आले नाही.',
'filedeleteerror'      => '"$1" संचिका वगळता आली नाही.',
'directorycreateerror' => '"$1" कार्यधारीका (directory) तयार केली जाऊ शकली नाही.',
'filenotfound'         => '"$1" ही संचिका सापडत नाही.',
'fileexistserror'      => 'संचिका "$1" वर लिहीता आले नाही: संचिका अस्तित्वात आहे.',
'unexpected'           => 'अनपेक्षित मूल्य: "$1"="$2"',
'formerror'            => 'त्रूटी: फॉर्म सबमीट करता आलेला नाही',
'badarticleerror'      => 'या पानावर ही कृती करता येत नाही.',
'cannotdelete'         => 'पान किंवा संचिका वगळता आलेली नाही. (आधीच इतर कुणी वगळले असण्याची शक्यता आहे.)',
'badtitle'             => 'चुकीचे शीर्षक',
'badtitletext'         => 'आपण मागितलेले शीर्षक पान अयोग्य, रिकामे अथवा चूकीने जोडलेले आंतर-भाषिय किंवा आंतर-विकि शीर्षक आहे. त्यात एक किंवा अधिक शीर्षकअयोग्य चिन्हे आहेत.',
'perfdisabled'         => 'क्षमस्व!ही सुविधा तात्पुरती अनुपलब्ध आहे कारण तिच्यामुळे कुणीही विकि वापरू शकणार नाही इतपत विदागार (database) मंदगती होतो.',
'perfcached'           => 'खालील माहिती सयीमध्ये(कॅशे) ठेवली आहे त्यामुळे ती नवीनतम नसावी.',
'perfcachedts'         => 'खालील माहिती सयीमध्ये(कॅशे) ठेवली आहे आणि शेवटी $1 ला बदलली होती.',
'querypage-no-updates' => 'सध्या या पाना करिता नवीसंस्करणे अनुपलब्ध केली आहेत.आत्ताच येथील विदा ताजा होणार नाही.',
'wrong_wfQuery_params' => 'wfQuery()साठी चुकीचे पॅरेमीटर्स दिलेले आहेत<br />
कार्य (function): $1<br />
पृच्छा (Query): $2',
'viewsource'           => 'स्रोत पहा',
'viewsourcefor'        => '$1 चा',
'actionthrottled'      => 'कृती अवरूद्ध (throttle) केली',
'actionthrottledtext'  => 'आंतरजाल-चिखलणी विरोधी उपायाच्या दृष्टीने(anti-spam measure), ही कृती थोड्या कालावधीत असंख्यवेळा करण्यापासून तुम्हाला प्रतिबंधीत करण्यात आले आहे, आणि आपण या मर्यादेचे उल्लंघन केले आहे. कृपया थोड्या वेळाने पुन्हा प्रयत्न करा.',
'protectedpagetext'    => 'हे पान बदल होऊ नयेत म्हणुन सुरक्षित केले आहे.',
'viewsourcetext'       => 'तुम्ही या पानाचा स्रोत पाहू शकता व प्रत करू शकता:',
'protectedinterface'   => 'हे पान सॉफ्टवेअरला इंटरफेस लेखन पुरवते, म्हणून दुरूपयोग टाळण्यासाठी संरक्षित केलेले आहे.',
'editinginterface'     => "'''सावधान:''' तुम्ही संचेतनाचे(Software) संपर्कमाध्यम मजकुर असलेले पान संपादीत करित  आहात.या पानावरील बदल इतर उपयोगकर्त्यांच्या  उपयोगकर्ता-संपर्कमाध्यमाचे स्वरूप पालटवू शकते.भाषांतरणांकरिता कृपया मिडीयाविकि स्थानिकीकरण प्रकल्पाच्या [http://translatewiki.net/wiki/Main_Page?setlang=mr बीटाविकि] सुविधेचा उपयोग करण्याबद्दल विचार करा.",
'sqlhidden'            => 'छूपी एस्क्यूएल पृच्छा (SQL query hidden)',
'cascadeprotected'     => 'हे पान संपादनांपासून सुरक्षित केले गेलेले आहे, कारण ते खालील {{PLURAL:$1|पानात|पानांमध्ये}} अंतर्भूत केलेले आहे, की जे पान/जी पाने शिडी पर्यायाने सुरक्षित आहेत:
$2',
'namespaceprotected'   => "'''$1''' नामविश्वातील पाने बदलण्याची आपणांस परवानगी नाही.",
'customcssjsprotected' => 'या पानावर इतर सदस्याच्या व्यक्तिगत पसंती असल्यामुळे, तुम्हाला हे पान संपादीत करण्याची परवानगी नाही.',
'ns-specialprotected'  => 'विशेष पाने संपादीत करता येत नाहीत.',
'titleprotected'       => "या शीर्षकाचे पान सदस्य [[User:$1|$1]]ने निर्मीत करण्यापासून सुरक्षित केलेले आहे.
''$2'' हे कारण नमूद केलेले आहे.",

# Virus scanner
'virus-badscanner'     => 'चुकीचे कॉन्फिगरेशन: व्हायरस स्कॅनर अनोळखी: <i>$1</i>',
'virus-scanfailed'     => 'स्कॅन पूर्ण झाले नाही (कोड $1)',
'virus-unknownscanner' => 'अनोळखी ऍन्टीव्हायरस:',

# Login and logout pages
'logouttitle'                => 'बाहेर पडा',
'logouttext'                 => '<strong>तुम्ही आता अदाखल झाला(logout)आहात.</strong><br />
तुम्ही अनामिकपणे {{SITENAME}}चा उपयोग करत राहू शकता, किंवा त्याच अथवा वेगळ्या सदस्य नावाने पुन्हा दाखल होऊ शकता. आपण स्वत:च्या न्याहाळकाची सय (cache) रिकामी करत नाही तो पर्यंत काही पाने आपण अजून दाखल आहात, असे नुसतेच दाखवत राहू शकतील.',
'welcomecreation'            => '== सुस्वागतम, $1! ==

तुमचे खाते उघडण्यात आले आहे. आपल्या {{SITENAME}} पसंती बदलण्यास विसरू नका.',
'loginpagetitle'             => 'सदस्य नोंदणी',
'yourname'                   => 'तुमचे नाव',
'yourpassword'               => 'तुमचा परवलीचा शब्द',
'yourpasswordagain'          => 'तुमचा परवलीचा शब्द पुन्हा लिहा',
'remembermypassword'         => 'माझा परवलीचा शब्द पुढील खेपेसाठी लक्षात ठेवा.',
'yourdomainname'             => 'तुमचे क्षेत्र (डॉमेन) :',
'externaldberror'            => 'बाह्य प्रमाणितीकरण विदागार त्रूटी होती किंवा तुम्हाला तुमचे बाह्य खाते अपडेट करण्याची परवानगी नाही.',
'loginproblem'               => '<b>तुमच्या प्रवेशप्रक्रियेमध्ये चुक झाली आहे.</b><br />कृपया पुन्हा प्रयत्न करा!',
'login'                      => 'प्रवेश करा',
'nav-login-createaccount'    => 'सदस्य प्रवेश',
'loginprompt'                => '{{SITENAME}}मध्ये दाखल होण्याकरिता  स्मृतिशेष ऊपलब्ध (Cookie enable)असणे आवश्यक आहे.',
'userlogin'                  => 'सदस्य प्रवेश',
'logout'                     => 'बाहेर पडा',
'userlogout'                 => 'बाहेर पडा',
'notloggedin'                => 'प्रवेशाची नोंदणी झालेली नाही!',
'nologin'                    => '$1 आपण सदस्यत्व घेतलेले नाही का?',
'nologinlink'                => 'सदस्य खाते तयार करा',
'createaccount'              => 'नवीन खात्याची नोंदणी करा',
'gotaccount'                 => 'जुने खाते आहे? $1.',
'gotaccountlink'             => 'प्रवेश करा',
'createaccountmail'          => 'इमेल द्वारे',
'badretype'                  => 'आपला परवलीचा शब्द चुकीचा आहे.',
'userexists'                 => 'या नावाने सदस्याची नोंदणी झालेली आहे, कृपया दुसरे सदस्य नाव निवडा.',
'youremail'                  => 'आपला इमेल:',
'username'                   => 'सदस्यनाम:',
'uid'                        => 'सदस्य खाते:',
'prefs-memberingroups'       => 'खालील {{PLURAL:$1|गटाचा|गटांचा}} सदस्य:',
'yourrealname'               => 'तुमचे खरे नाव:',
'yourlanguage'               => 'भाषा:',
'yournick'                   => 'आपले उपनाव (सहीसाठी)',
'badsig'                     => 'अयोग्य कच्ची सही;HTML खूणा तपासा.',
'badsiglength'               => 'टोपणनाव खूप लांब आहे.
टोपणनाव $1 {{PLURAL:$1|अक्षरापेक्षा|अक्षरांपेक्षा}} कमी लांबीचे हवे.',
'email'                      => 'विपत्र(ई-मेल)',
'prefs-help-realname'        => 'तुमचे खरे नाव (वैकल्पिक): हे नाव दिल्यास आपले योगदान या नावाखाली नोंदले व दाखवले जाईल.',
'loginerror'                 => 'आपल्या प्रवेश नोंदणीमध्ये चुक झाली आहे',
'prefs-help-email'           => 'विरोप(ईमेल)(वैकल्पिक):इतरांना सदस्य किंवा सदस्य_चर्चा पानातून, तुमची ओळख देण्याची आवश्यकता न ठेवता , तुमच्याशी संपर्क सुविधा पुरवते.',
'prefs-help-email-required'  => 'विपत्र(ईमेल)पत्ता  लागेल.',
'nocookiesnew'               => 'सदस्य खाते उघडले ,पण तुम्ही खाते वापरून दाखल झालेले नाही आहात.{{SITENAME}} सदस्यांना दाखल करून घेताना त्यांच्या स्मृतीशेष (cookies) वापरते.तुम्ही स्मृतीशेष सुविधा अनुपलब्ध टेवली आहे.ती कृपया उपलब्ध करा,आणि नंतर तुमच्या नवीन सदस्य नावाने आणि परवलीने दाखल व्हा.',
'nocookieslogin'             => '{{SITENAME}} सदस्यांना दाखल करून घेताना त्यांच्या स्मृतीशेष (cookies) वापरते.तुम्ही स्मृतीशेष सुविधा अनुपलब्ध टेवली आहे.स्मृतीशेष सुविधा कृपया उपलब्ध करा,आणि दाखल होण्यासाठी पुन्हा प्रयत्न करा.',
'noname'                     => 'आपण नोंदणीसाठी सदस्याचे योग्य नाव लिहिले नाही.',
'loginsuccesstitle'          => 'आपल्या प्रवेशाची नोंदणी यशस्वीरित्या पूर्ण झाली',
'loginsuccess'               => "'''तुम्ही {{SITENAME}} वर \"\$1\" नावाने प्रवेश केला आहे.'''",
'nosuchuser'                 => '"$1" या नावाचा कोणताही सदस्य नाही.तुमचे शुद्धलेखन तपासा, किंवा नवीन खाते तयार करा.',
'nosuchusershort'            => '"<nowiki>$1</nowiki>" या नावाचा सदस्य नाही. लिहीताना आपली चूक तर नाही ना झाली?',
'nouserspecified'            => 'तुम्हाला सदस्यनाव नमुद करावे लागेल.',
'wrongpassword'              => 'आपला परवलीचा शब्द चुकीचा आहे, पुन्हा एकदा प्रयत्न करा.',
'wrongpasswordempty'         => 'परवलीचा शब्द रिकामा आहे; परत प्रयत्न करा.',
'passwordtooshort'           => 'तुमचा परवलीचा शब्द जरूरीपेक्षा लहान आहे. यात कमीत कमी $1 अक्षरे पाहिजेत.',
'mailmypassword'             => 'कृपया परवलीचा नवीन शब्द माझ्या इमेल पत्त्यावर पाठविणे.',
'passwordremindertitle'      => '{{SITENAME}}करिता नवा तात्पुरता परवलीचा शब्दांक.',
'passwordremindertext'       => 'कुणीतरी (कदाचित तुम्ही, अंकपत्ता $1 कडून) {{SITENAME}} करिता ’नवा परवलीचा शब्दांक पाठवावा’ अशी विनंती केली आहे ($4).
"$2" सदस्याकरिता परवलीचा शब्दांक "$3" झाला आहे.
तुम्ही आता प्रवेश करा व तुमचा परवलीचा शब्दांक बदला.

जर ही विनंती इतर कुणी केली असेल किंवा तुम्हाला तुमचा परवलीचा शब्दांक आठवला असेल आणि तुम्ही तो आता बदलू इच्छित नसाल तर, तुम्ही हा संदेश दूर्लक्षित करून जूना परवलीचा शब्दांक वापरत राहू शकता.',
'noemail'                    => '"$1" सदस्यासाठी कोणताही इमेल पत्ता दिलेला नाही.',
'passwordsent'               => '"$1" सदस्याच्या इमेल पत्त्यावर परवलीचा नवीन शब्द पाठविण्यात आलेला आहे.
तो शब्द वापरुन पुन्हा प्रवेश करा.',
'blocked-mailpassword'       => 'संपादनापासून तुमच्या अंकपत्त्यास आडविण्यात आले आहे,आणि म्हणून दुरूपयोग टाळ्ण्याच्या दृष्टीने परवलीचाशब्द परत मिळवण्यास सुद्धा मान्यता उपलब्ध नाही.',
'eauthentsent'               => 'नामांकित ई-मेल पत्त्यावर एक निश्चितता स्वीकारक ई-मेल पाठविला गेला आहे.
खात्यावर कोणताही इतर ई-मेल पाठविण्यापूर्वी - तो ई-मेल पत्ता तुमचाच आहे, हे सूनिश्चित करण्यासाठी - तुम्हाला त्या ई-मेल मधील सूचनांचे पालन करावे लागेल.',
'throttled-mailpassword'     => 'मागील {{PLURAL:$1|एका तासामध्ये|$1 तासांमध्ये}} परवलीचा शब्द बदलण्यासाठीची सूचना पाठविलेली आहे. दुरुपयोग टाळण्यासाठी {{PLURAL:$1|एका तासामध्ये|$1 तासांमध्ये}} फक्त एकदाच सूचना दिली जाईल.',
'mailerror'                  => 'विपत्र पाठवण्यात त्रूटी: $1',
'acct_creation_throttle_hit' => 'माफ करा, तुम्ही आत्तापर्यंत $1 खाती उघडली आहेत. तुम्हाला आणखी खाती उघडता येणार नाहीत.',
'emailauthenticated'         => 'तुमचा इ-मेल $1 ला तपासलेला आहे.',
'emailnotauthenticated'      => 'तुमचा इमेल पत्ता तपासलेला नाही. खालील कार्यांकरिता इमेल पाठविला जाणार नाही.',
'noemailprefs'               => 'खालील सुविधा कार्यान्वित करण्यासाठी इ-मेल पत्ता पुरवा.',
'emailconfirmlink'           => 'आपला इमेल पत्ता तपासून पहा.',
'invalidemailaddress'        => 'तुम्ही दिलेला इमेल पत्ता चुकीचा आहे, कारण तो योग्यप्रकारे लिहिलेला नाही. कृपया योग्यप्रकारे इमेल पत्ता लिहा अथवा ती जागा मोकळी सोडा.',
'accountcreated'             => 'खाते उघडले.',
'accountcreatedtext'         => '$1 चे सदस्यखाते उघडले.',
'createaccount-title'        => '{{SITENAME}} साठीची सदस्य नोंदणी',
'createaccount-text'         => 'तुमच्या विपत्र पत्त्याकरिता {{SITENAME}} ($4)वर "$2" नावाच्या कुणी "$3" परवलीने खाते उघडले आहे. कृपया आपण सदस्य प्रवेश करून आपला परवलीचा शब्द बदलावा.

जर ही नोंदणी चुकीने झाली असेल तर तुम्ही या संदेशाकडे दुर्लक्ष करू शकता.',
'loginlanguagelabel'         => 'भाषा: $1',

# Password reset dialog
'resetpass'               => 'परवलीचा शब्द पुर्नयोजन(रिसेट)करा.',
'resetpass_announce'      => 'तुम्ही इमेलमधून दिलेल्या तात्पुरत्या शब्दांकाने प्रवेश केलेला आहे. आपली सदस्य नोंदणी पूर्ण करण्यासाठी कृपया इथे नवीन परवलीचा शब्द द्या:',
'resetpass_text'          => '<!-- मजकूर इथे लिहा -->',
'resetpass_header'        => 'परवलीचे पुर्नयोजन करा',
'resetpass_submit'        => 'परवलीचा शब्द टाका आणि प्रवेश करा',
'resetpass_success'       => 'तुमचा परवलीचा शब्द बदललेला आहे! आता तुमचा प्रवेश करीत आहोत...',
'resetpass_bad_temporary' => 'तात्पुरता परवलीचा शब्द चुकीचा आहे. तुम्ही कदाचित पूर्वीच परवलीचा शब्द बदललेला असेल किंवा नवीन तात्पुरता परवलीचा शब्द मागितलेला असेल.',
'resetpass_forbidden'     => '{{SITENAME}} वर परवलीचा शब्द बदलता येत नाही.',
'resetpass_missing'       => 'सारणी विदा नाही.',

# Edit page toolbar
'bold_sample'     => 'ठळक मजकूर',
'bold_tip'        => 'ठळक',
'italic_sample'   => 'तिरकी अक्षरे',
'italic_tip'      => 'तिरकी अक्षरे',
'link_sample'     => 'दुव्याचे शीर्षक',
'link_tip'        => 'अंतर्गत दुवा',
'extlink_sample'  => 'http://www.example.com दुव्याचे शीर्षक',
'extlink_tip'     => 'बाह्य दुवा (http:// विसरू नका)',
'headline_sample' => 'अग्रशीर्ष मजकुर',
'headline_tip'    => 'द्वितीय-स्तर अग्रशीर्ष',
'math_sample'     => 'इथे सूत्र लिहा',
'math_tip'        => 'गणितीय सूत्र (LaTeX)',
'nowiki_sample'   => 'मजकूर इथे लिहा',
'nowiki_tip'      => 'विकिभाषेप्रमाणे बदल करू नका',
'image_tip'       => 'संलग्न संचिका',
'media_tip'       => 'संचिकेचा दुवा',
'sig_tip'         => 'वेळेबरोबर तुमची सही',
'hr_tip'          => 'आडवी रेषा (कमी वापरा)',

# Edit pages
'summary'                          => 'सारांश',
'subject'                          => 'विषय',
'minoredit'                        => 'हा एक छोटा बदल आहे',
'watchthis'                        => 'या लेखावर लक्ष ठेवा',
'savearticle'                      => 'हा लेख साठवून ठेवा',
'preview'                          => 'झलक',
'showpreview'                      => 'झलक पहा',
'showlivepreview'                  => 'थेट झलक',
'showdiff'                         => 'बदल दाखवा',
'anoneditwarning'                  => "'''सावधानः''' तुम्ही विकिपीडियाचे सदस्य म्हणून प्रवेश (लॉग-इन) केलेला नाही. या पानाच्या संपादन इतिहासात तुमचा आय.पी. ऍड्रेस नोंदला जाईल.",
'missingsummary'                   => "'''आठवण:''' तूम्ही संपादन सारांश पुरवलेला नाही.आपण जतन करा वर पुन्हा टीचकी मारली तर तेत्या शिवाय जतन होईल.",
'missingcommenttext'               => 'कृपया खाली प्रतिक्रीया भरा.',
'missingcommentheader'             => "'''आठवण:'''आपण या लेखनाकरिता विषय किंवा अधोरेषा दिलेली नाही .आपण पुन्ह जतन करा अशी सुचना केली तर, तुमचे संपादन त्याशिवायच जतन होईल.",
'summary-preview'                  => 'आढाव्याची झलक',
'subject-preview'                  => 'विषय/मथळा झलक',
'blockedtitle'                     => 'या सदस्यासाठी प्रवेश नाकारण्यात आलेला आहे.',
'blockedtext'                      => "<big>'''तुमचे सदस्यनाव अथवा IP पत्ता ब्लॉक केलेला आहे.'''</big>

हा ब्लॉक $1 यांनी केलेला आहे.
यासाठी ''$2'' हे कारण दिलेले आहे.

* ब्लॉकची सुरूवात: $8
* ब्लॉकचा शेवट: $6
* कुणाला ब्लॉक करायचे आहे: $7

तुम्ही ह्या ब्लॉक संदर्भातील चर्चेसाठी $1 अथवा [[{{MediaWiki:Grouppage-sysop}}|प्रबंधकांशी]] संपर्क करू शकता.
तुम्ही जोवर वैध इमेल पत्ता आपल्या [[Special:Preferences|माझ्या पसंती]] पानावर देत नाही तोवर तुम्ही ’सदस्याला इमेल पाठवा’ हा दुवा वापरू शकत नाही. तसेच असे करण्यापासून आपल्याला ब्लॉक केलेले नाही. 
तुमचा सध्याचा IP पत्ता $3 हा आहे, व तुमचा ब्लॉक क्रमांक #$5 हा आहे. 
कृपया या संदर्भातील चर्चेमध्ये यापैकी काहीही उद्घृत करा.",
'autoblockedtext'                  => 'तुमचा आंतरजालीय अंकपत्ता आपोआप स्थगीत केला आहे कारण तो इतर अशा सदस्याने वापरलाकी, ज्याला $1ने प्रतिबंधित केले.
आणि दिलेले कारण खालील प्रमाणे आहे
:\'\'$2\'\'

* स्थगन तारीख: $8
* स्थगिती संपते: $6

तुम्ही $1शी संपर्क करू शकता किंवा इतर [[{{MediaWiki:Grouppage-sysop}}|प्रबंधकां पैकी]] एकाशी स्थगनाबद्दल चर्चा करू शकता.

[[Special:Preferences|सदस्य पसंतीत]]त शाबीत विपत्र पत्ता नमुद असल्या शिवाय आणि तुम्हाला  तो वापरण्या पासून प्रतिबंधीत केले असल्यास तुम्ही  "या सदस्यास विपत्र पाठवा" सुविधा  वापरू शकणार नाही.

तुमचा स्थगन क्र $5 आहे. कृपया तूमच्या कोणत्याही शंकासमाधाना साठी हा क्रंमांक नमुद करा.',
'blockednoreason'                  => 'कारण दिलेले नाही',
'blockedoriginalsource'            => "'''$1''' चा स्रोत खाली दिल्याप्रमाणे:",
'blockededitsource'                => "'''$1'''ला '''तुमची संपादने'''चा मजकुर खाली दाखवला आहे:",
'whitelistedittitle'               => 'संपादनासाठी सदस्य म्हणून प्रवेश आवश्यक आहे.',
'whitelistedittext'                => 'लेखांचे संपादन करण्यासाठी आधी $1 करा.',
'confirmedittitle'                 => 'संपादनाकरिता विपत्राने शाबीत करणे आवश्यक',
'confirmedittext'                  => 'तुम्ही संपादने करण्यापुर्वी तुमचा विपत्र पत्ता शाबीत करणे आवश्यक आहे.Please set and validate तुमचा विपत्र पत्ता तुमच्या[[Special:Preferences|सदस्य पसंती]]तून लिहा व सिद्ध करा.',
'nosuchsectiontitle'               => 'असा विभाग नाही.',
'nosuchsectiontext'                => 'तुम्ही अस्तिवात नसलेला विभाग संपादन करण्याचा प्रयत्न केला आहे.विभाग $1 नसल्यामुळे,तुमचे संपादन जतन करण्याकरिता जागा नाही.',
'loginreqtitle'                    => 'प्रवेश गरजेचा आहे',
'loginreqlink'                     => 'प्रवेश करा',
'loginreqpagetext'                 => 'तुम्ही इतर पाने पहाण्याकरिता $1 केलेच पाहिजे.',
'accmailtitle'                     => 'परवलीचा शब्द पाठविण्यात आलेला आहे.',
'accmailtext'                      => "'$1' चा परवलीचा शब्द $2 पाठविण्यात आलेला आहे.",
'newarticle'                       => '(नवीन लेख)',
'newarticletext'                   => 'तुम्हाला अपेक्षित असलेला लेख अजून लिहिला गेलेला नाही. हा लेख लिहिण्यासाठी खालील पेटीत मजकूर लिहा. मदतीसाठी [[{{MediaWiki:Helppage}}|येथे]] टिचकी द्या.

जर येथे चुकून आला असाल तर ब्राउझरच्या बॅक (back) कळीवर टिचकी द्या.',
'anontalkpagetext'                 => "---- ''हे बोलपान अशा अज्ञात सदस्यासाठी आहे ज्यांनी खाते तयार केलेले नाही किंवा त्याचा वापर करत नाहीत. त्यांच्या ओळखीसाठी आम्ही आंतरजाल अंकपत्ता वापरतो आहोत. असा अंकपत्ता बर्‍याच लोकांचा एकच असू शकतो जर आपण अज्ञात सदस्य असाल आणि आपल्याला काही अप्रासंगिक संदेश मिळाला असेल तर कृपया [[Special:UserLogin| खाते तयार करा किंवा प्रवेश करा]] ज्यामुळे पुढे असे गैरसमज होणार नाहीत.''",
'noarticletext'                    => 'या लेखात सध्या काहीही मजकूर नाही. तुम्ही विकिपिडीयावरील इतर लेखांमध्ये या [[Special:Search/{{PAGENAME}}|मथळ्याच्या शोध घेऊ शकता]] किंवा हा लेख [{{fullurl:{{FULLPAGENAME}}|action=edit}} लिहू शकता].',
'userpage-userdoesnotexist'        => '"$1" सदस्य खाते नोंदीकॄत नाही.कृपया हे पान तुम्ही संपादीत किंवा नव्याने तयार करू इच्छिता का या बद्दल विचार करा.',
'clearyourcache'                   => "'''सूचना:''' जतन केल्यानंतर, बदल पहाण्याकरिता तुम्हाला तुमच्या विचरकाची सय टाळायला लागू शकते. '''मोझील्ला/फायरफॉक्स /सफारी:''' ''Reload''करताना ''Shift''दाबून ठेवा किंवा ''Ctrl-Shift-R'' दाबा

(ऍपल मॅक वर ''Cmd-shift-R'');'''IE:''' ''Refresh'' टिचकताना ''Ctrl'' दाबा,किंवा ''Ctrl-F5'' दाबा ; '''Konqueror:''': केवळ '''Reload''' टिचकवा,किवा ''F5'' दाबा; '''Opera'''उपयोगकर्त्यांना  ''Tools→Preferences'' मधील सय पूर्ण रिकामी करायला लागेल.",
'usercssjsyoucanpreview'           => "<strong>टीप:</strong>तुमचे नवे CSS/JS जतन करण्यापूर्वी 'झलक पहा' कळ वापरा.",
'usercsspreview'                   => "'''तुम्ही तुमच्या सी.एस.एस.ची केवळ झलक पहात आहात, ती अजून जतन केलेली नाही हे लक्षात घ्या.'''",
'userjspreview'                    => "'''तुम्ही तुमची सदस्य जावास्क्रिप्ट तपासत आहात किंवा झलक पहात आहात ,ती अजून जतन केलेली नाही हे लक्षात घ्या!'''",
'userinvalidcssjstitle'            => "'''सावधान:''' \"\$1\" अशी त्वचा नाही.custom .css आणि .js पाने lowercase title वापरतात हे लक्षात घ्या, उदा. {{ns:user}}:Foo/monobook.css या विरूद्ध {{ns:user}}:Foo/Monobook.css.",
'updated'                          => '(बदल झाला आहे.)',
'note'                             => '<strong>सूचना:</strong>',
'previewnote'                      => '<strong>लक्षात ठेवा की ही फक्त झलक आहे, बदल अजून सुरक्षित केलेले नाहीत.</strong>',
'previewconflict'                  => 'वरील संपादन क्षेत्रातील मजकूर जतन केल्यावर या झलकेप्रमाणे दिसेल.',
'session_fail_preview'             => '<strong>क्षमस्व! सत्र विदेच्या क्षयामुळे आम्ही तुमची संपादन प्रक्रीया पार पाडू शकलो नाही.कृपया पुन्हा प्रयत्न करा.जर एवढ्याने काम झाले नाही तर सदस्य खात्यातून बाहेर पडून पुन्हा प्रवेश करून पहा.</strong>',
'session_fail_preview_html'        => "<strong>क्षमस्व! सत्र विदेच्या क्षयामुळे आम्ही तुमची संपादन प्रक्रीया पार पाडू शकलो नाही.</strong>

''कारण {{SITENAME}}चे कच्चे HTML चालू ठेवले आहे, जावास्क्रिप्ट हल्ल्यांपासून बचाव व्हावा म्हणून झलक लपवली आहे.''

<strong>जर संपादनाचा हा सुयोग्य प्रयत्न असेल तर ,कॄपया पुन्हा प्रयत्न करा. जर एवढ्याने काम झाले नाही तर सदस्य खात्यातून बाहेर पडून पुन्हा प्रवेश करून पहा.</strong>",
'token_suffix_mismatch'            => '<strong>तुमचे संपादन रद्द करण्यात आलेले आहे कारण तुमच्या क्लायंटनी तुमच्या संपादनातील उद्गारवाचक चिन्हांमध्ये (punctuation) बदल केलेले आहेत.
पानातील मजकूर खराब होऊ नये यासाठी संपादन रद्द करण्यात आलेले आहे.
असे कदाचित तुम्ही अनामिक proxy वापरत असल्याने होऊ शकते.</strong>',
'editing'                          => '$1 चे संपादन होत आहे.',
'editingsection'                   => '$1 (विभाग) संपादन',
'editingcomment'                   => '$1 संपादन (प्रतिक्रीया)',
'editconflict'                     => 'वादग्रस्त संपादन: $1',
'explainconflict'                  => "तुम्ही संपादनाला सुरूवात केल्यानंतर इतर कोणीतरी बदल केला आहे.
वरील पाठ्यभागामध्ये सध्या अस्तिवात असलेल्या पृष्ठातील पाठ्य आहे, तर तुमचे बदल खालील पाठ्यभागात दर्शविलेले आहेत.
तुम्हाला हे बदल सध्या अस्तिवात असणाऱ्या पाठ्यासोबत एकत्रित करावे लागतील.
'''केवळ''' वरील पाठ्यभागामध्ये असलेले पाठ्य साठविण्यात येईल जर तुम्ही \"साठवून ठेवा\" ही कळ दाबली.",
'yourtext'                         => 'तुमचे पाठ्य',
'storedversion'                    => 'साठविलेली आवृत्ती',
'nonunicodebrowser'                => '<strong>सावधान: तुमचा विचरक यूनिकोड आधारीत नाही. ASCII नसलेली  अक्षरचिन्हे संपादन खिडकीत सोळाअंकी कूटसंकेत (हेक्झाडेसीमल कोड) स्वरूपात दिसण्याची, सुरक्षीतपणे संपादन करू देणारी,  पळवाट उपलब्ध आहे.</strong>',
'editingold'                       => '<strong>इशारा: तुम्ही मूळ पृष्ठाची एक कालबाह्य आवृत्ती संपादित करीत आहात.
जर आपण बदल साठवून ठेवण्यात आले तर या नंतरच्या सर्व आवृत्त्यांमधील साठविण्यात आलेले बदल नष्ठ होतील.</strong>',
'yourdiff'                         => 'फरक',
'copyrightwarning'                 => '{{SITENAME}} येथे केलेले कोणतेही लेखन $2 (अधिक माहितीसाठी $1 पहा) अंतर्गत मुक्त उद्घोषित केले आहे असे गृहित धरले जाईल याची कृपया नोंद घ्यावी. आपणास आपल्या लेखनाचे मुक्त संपादन आणि मुक्त वितरण होणे पसंत नसेल तर येथे संपादन करू नये.<br />
तुम्ही येथे लेखन करताना हे सुद्धा गृहित धरलेले असते की येथे केलेले लेखन तुमचे स्वतःचे आणि केवळ स्वतःच्या प्रताधिकार (कॉपीराईट) मालकीचे आहे किंवा प्रताधिकाराने गठीत न होणार्‍या सार्वजनिक ज्ञानक्षेत्रातून घेतले आहे किंवा तत्सम मुक्त स्रोतातून घेतले आहे. तुम्ही संपादन करताना तसे वचन देत आहात. <strong>प्रताधिकारयुक्त लेखन सुयोग्य परवानगीशिवाय मुळीच चढवू/भरू नये!</strong>',
'copyrightwarning2'                => '{{SITENAME}} येथे केलेले कोणतेही लेखन हे इतर संपादकांकरवी बदलले अथवा काढले जाऊ शकते. जर आपणास आपल्या लेखनाचे मुक्त संपादन होणे पसंत नसेल तर येथे संपादन करू नये.<br />
तुम्ही येथे लेखन करताना हे सुद्धा गृहित धरलेले असते की येथे केलेले लेखन तुमचे स्वतःचे आणि केवळ स्वतःच्या प्रताधिकार (कॉपीराईट) मालकीचे आहे किंवा प्रताधिकाराने गठीत न होणार्‍या सार्वजनिक ज्ञानक्षेत्रातून घेतले आहे किंवा तत्सम मुक्त स्रोतातून घेतले आहे. तुम्ही संपादन करताना तसे वचन देत आहात (अधिक माहितीसाठी $1 पहा). <strong>प्रताधिकारयुक्त लेखन सुयोग्य परवानगीशिवाय मुळीच चढवू/भरू नये!</strong>',
'longpagewarning'                  => '<strong>इशारा: हे पृष्ठ $1 kilobytes लांबीचे आहे; काही विचरकांना सुमारे ३२ किलोबाईट्स् आणि त्यापेक्षा जास्त लांबीच्या पृष्ठांना संपादित करण्यास अडचण येऊ शकते.
कृपया या पृष्ठाचे त्याहून छोट्या भागात रुपांतर करावे.</strong>',
'longpageerror'                    => '<strong>त्रूटी:आपण दिलेला मजकुर जास्तीत जास्त शक्य $2  किलोबाईट पेक्षा अधिक लांबीचा $1 किलोबाईट आहे.तो जतन केला जाऊ शकत नाही.</strong>',
'readonlywarning'                  => '<strong>सावधान:विदागारास भरण-पोषणाकरिता ताळे ठोकले आहे,त्यामुळे सध्या तुम्ही तुमचे संपादन जतन करू शकत नाही.जर तुम्हाला हवे असेल तर नंतर उपयोग करण्याच्या दृष्टीने, तुम्ही मजकुर ’मजकुर संचिकेत’(टेक्स्ट फाईल मध्ये) कापून-चिटकवू शकता.</strong>',
'protectedpagewarning'             => '<strong>सूचना:  हे सुरक्षीत पान आहे. फक्त प्रबंधक याच्यात बदल करु शकतात.</strong>',
'semiprotectedpagewarning'         => "'''सूचना:''' हे पान सुरक्षीत आहे. फक्त सदस्य याच्यात बदल करू शकतात.",
'cascadeprotectedwarning'          => "'''सावधान:''' हे पान निम्न-लिखीत शिडी-प्रतिबंधीत {{PLURAL:$1|पानात|पानात}} आंतरभूत असल्यामुळे,केवळ प्रबंधक सुविधाप्राप्त सदस्यांनाच संपादन करता यावे असे ताळे त्यास ठोकलेले आहे :",
'titleprotectedwarning'            => '<strong>सावधान:फक्त काही सदस्यानांच तयार करता यावे म्हणून ह्या पानास ताळे आहे.</strong>',
'templatesused'                    => 'या पानावर खालील साचे वापरण्यात आलेले आहेत:',
'templatesusedpreview'             => 'या झलकेमध्ये वापरलेले साचे:',
'templatesusedsection'             => 'या विभागात वापरलेले साचे:',
'template-protected'               => '(सुरक्षित)',
'template-semiprotected'           => '(अर्ध-सुरक्षीत)',
'hiddencategories'                 => 'हे पान खालील {{PLURAL:$1|एका लपविलेल्या वर्गामध्ये|$1 लपविलेल्या वर्गांमध्ये}} आहे:',
'nocreatetitle'                    => 'पान निर्मीतीस मर्यादा',
'nocreatetext'                     => '{{SITENAME}}वर नवीन लेख लिहिण्यास मज्जाव करण्यात आलेला आहे. आपण परत जाऊन अस्तित्वात असलेल्या लेखांचे संपादन करू शकता अथवा [[Special:UserLogin|नवीन सदस्यत्व घ्या/ प्रवेश करा]].',
'nocreate-loggedin'                => '{{SITENAME}}वर तुम्हाला नवीन पाने बनवण्याची परवानगी नाही.',
'permissionserrors'                => 'परवानगीतील त्रूटी',
'permissionserrorstext'            => 'खालील{{PLURAL:$1|कारणामुळे|कारणांमुळे}} तुम्हाला तसे करण्याची परवानगी नाही:',
'permissionserrorstext-withaction' => 'तुम्हाला $2 ची परवानगी नाही, खालील {{PLURAL:$1|कारणासाठी|कारणांसाठी}}:',
'recreate-deleted-warn'            => "'''सूचना: पूर्वी वगळलेला लेख तुम्ही पुन्हा संपादित आहात.'''

कृपया तुम्ही करत असलेले संपादन योग्य असल्याची खात्री करा.
या लेखाची वगळल्याची नोंद तुमच्या संदर्भाकरीता पुढीलप्रमाणे:",

# Parser/template warnings
'expensive-parserfunction-warning'        => 'इशारा: या पानावर खूप सारे खर्चिक पार्सर क्रिया कॉल्स आहेत.

ते $2 पेक्षा कमी असायला हवेत, सध्या $1 इतके आहेत.',
'expensive-parserfunction-category'       => 'खूप सारे खर्चिक पार्सर क्रिया कॉल्स असणारी पाने',
'post-expand-template-inclusion-warning'  => 'सूचना: साचे वाढविण्याची मर्यादा संपलेली आहे.
काही साचे वगळले जातील.',
'post-expand-template-inclusion-category' => 'अशी पाने ज्यांच्यावर साचे चढविण्याची मर्यादा संपलेली आहे',
'post-expand-template-argument-warning'   => 'सूचना: या पानावर असा एकतरी साचा आहे जो वाढविल्यास खूप मोठा होईल.
असे साचे वगळण्यात आलेले आहेत.',
'post-expand-template-argument-category'  => 'अशी पाने ज्यांच्यामध्ये साचे वगळलेले आहेत',

# "Undo" feature
'undo-success' => 'संपादन परतवले जाऊ शकते.कृपया, आपण नेमके हेच करू इच्छीता हे खाली दिलेली तुलना पाहू निश्चीत करा,आणि नंतर संपादन परतवण्याचे काम पूर्ण करण्याकरिता इच्छीत बद्ल जतन करा.',
'undo-failure' => 'दरम्यान परस्पर विरोधी संपादने झाल्यामुळे आपण हे संपादन परतवू शकत नाही.',
'undo-norev'   => 'हे संपादन परतविता आलेले नाही कारण ते अगोदरच उलटविलेले किंवा वगळलेले आहे.',
'undo-summary' => '[[Special:Contributions/$2|$2]] ([[User talk:$2|चर्चा]])यांची आवृत्ती $1 परतवली.',

# Account creation failure
'cantcreateaccounttitle' => 'खाते उघडू शकत नाही',
'cantcreateaccount-text' => "('''$1''')या आंतरजाल अंकपत्त्याकडूनच्या खाते निर्मीतीस [[User:$3|$3]]ने अटकाव केला आहे.

$3ने ''$2'' कारण दिले आहे.",

# History pages
'viewpagelogs'        => 'या पानाच्या नोंदी पहा',
'nohistory'           => 'या पृष्ठासाठी आवृत्ती इतिहास अस्तित्वात नाही.',
'revnotfound'         => 'आवृत्ती सापडली नाही',
'revnotfoundtext'     => 'या पृष्ठाची तुम्ही मागविलेली जुनी आवृत्ती सापडली नाही.
कृपया URL तपासून पहा.',
'currentrev'          => 'चालू आवृत्ती',
'revisionasof'        => '$1 नुसारची आवृत्ती',
'revision-info'       => '$2ने $1चे आवर्तन',
'previousrevision'    => '←मागील आवृत्ती',
'nextrevision'        => 'पुढील आवृत्ती→',
'currentrevisionlink' => 'आताची आवृत्ती',
'cur'                 => 'चालू',
'next'                => 'पुढील',
'last'                => 'मागील',
'page_first'          => 'प्रथम',
'page_last'           => 'अंतिम',
'histlegend'          => 'बदल निवडणे: जुन्या आवृत्तींमधील फरक पाहण्यासाठी रेडियो बॉक्स निवडा व एन्टर कळ दाबा अथवा खाली दिलेल्या कळीवर टिचकी द्या.<br />
लिजेंड: (चालू) = चालू आवृत्तीशी फरक,
(मागील) = पूर्वीच्या आवृत्तीशी फरक, छो = छोटा बदल',
'deletedrev'          => '[वगळले]',
'histfirst'           => 'सर्वात जुने',
'histlast'            => 'सर्वात नवीन',
'historysize'         => '({{PLURAL:$1|1 बाइट|$1 बाइट}})',
'historyempty'        => '(रिकामे)',

# Revision feed
'history-feed-title'          => 'आवृत्ती इतिहास',
'history-feed-description'    => 'विकिवरील या पानाच्या आवृत्त्यांचा इतिहास',
'history-feed-item-nocomment' => '$2 इथले $1', # user at time
'history-feed-empty'          => 'विनंती केलेले पान अस्तित्वात नाही.

ते विकिवरून वगळले किंवा नाव बदललेले असण्याची शक्यता आहे.

संबधीत नव्या पानांकरिता [[Special:Search|विकिवर शोध घेण्याचा ]] प्रयत्न करा.',

# Revision deletion
'rev-deleted-comment'         => '(प्रतिक्रीया वगळली)',
'rev-deleted-user'            => '(सदस्य नाव वगळले)',
'rev-deleted-event'           => '(कार्य नोंद वगळली)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">
या पानाची आवृत्ती सार्वजनिक विदागारातून वगळण्यात आली आहे.

[{{fullurl:Special:Log/delete|पान={{FULLPAGENAMEE}}}} वगळल्याच्या नोंदीत निर्देश असण्याची शक्यता आहे].

</div>',
'rev-deleted-text-view'       => '<div class="mw-warning plainlinks">
पानाचे हे आवर्तन सार्वजनिक विदागारातून वगळण्यात आले आहे.
{{SITENAME}}च्या प्रबंधक या नात्याने तुम्ही ते पाहू शकता; [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} वगळलेल्या नोंदीत] माहिती असण्याची शक्यता आहे .</div>',
'rev-delundel'                => 'दाखवा/लपवा',
'revisiondelete'              => 'आवर्तने वगळा/पुनर्स्थापित करा',
'revdelete-nooldid-title'     => 'अपेक्षीत आवृत्ती दिलेली नाही',
'revdelete-nooldid-text'      => '!!आपण ही कृती करावयाची आवर्तने सूचीत केलेली नाहीत, दिलेले आवर्तन अस्तित्वात नाही, किंवा तुम्ही सध्याचे आवर्तन लपविण्याचा प्रयत्न करीत आहात.',
'revdelete-selected'          => '[[:$1]] {{PLURAL:$2|चे निवडलेले आवर्तन|ची निवडलेली आवर्तने}}:',
'logdelete-selected'          => '{{PLURAL:$1|निवडलेली नोंदीकृत घटना|निवडलेल्या नोंदीकृत घटना}}:',
'revdelete-text'              => 'वगळलेल्या नोंदी आणि घटना अजूनही पानाच्या इतिहासात आणि नोंदीत आढळेल,परंतु मजकुराचा भाग सार्वजनिक स्वरूपात उपलब्ध राहणार नाही.

अजून इतर  प्रतिबंध घातल्या शिवाय {{SITENAME}}चे इतर प्रबंधक झाकलेला मजकुर याच दुव्याने परतवू शकतील.',
'revdelete-legend'            => 'दृश्य बंधने निश्चित करा',
'revdelete-hide-text'         => 'आवर्तीत मजकुर लपवा',
'revdelete-hide-name'         => 'कृती आणि ध्येय लपवा',
'revdelete-hide-comment'      => 'संपादन प्रतिक्रीया लपवा',
'revdelete-hide-user'         => 'संपादकाचे सदस्यनाव/आंतरजाल अंकपत्ता लपवा',
'revdelete-hide-restricted'   => 'ही बंधने प्रबंधक तसेच इतरांनाही लागू करा तसेच इंटरफेस ला ताळा ठोका',
'revdelete-suppress'          => 'प्रबंधक तसेच इतरांपासून विदा लपवा',
'revdelete-hide-image'        => 'संचिका मजकुर लपवा',
'revdelete-unsuppress'        => 'पुर्नस्थापीत आवृत्तीवरील बंधने ऊठवा',
'revdelete-log'               => 'नोंद प्रतिक्रीया:',
'revdelete-submit'            => 'निवडलेल्या आवृत्त्यांना लागू करा',
'revdelete-logentry'          => '[[$1]]ची आवर्तन सदृश्यता बदलली.',
'logdelete-logentry'          => '[[$1]]ची घटना सदृश्यता बदलली.',
'revdelete-success'           => "'''आवर्तनांची दृश्यता यशस्वी पणे लाविली.'''",
'logdelete-success'           => "'''घटनांची दृश्यता यशस्वी पणे लाविली.'''",
'revdel-restore'              => 'दृश्यता बदला',
'pagehist'                    => 'पानाचा इतिहास',
'deletedhist'                 => 'वगळलेला इतिहास',
'revdelete-content'           => 'कंटेंट',
'revdelete-summary'           => 'संपादन माहिती',
'revdelete-uname'             => 'सदस्यनाम',
'revdelete-restricted'        => 'प्रबंधकांना बंधने दिली',
'revdelete-unrestricted'      => 'प्रबंधकांची बंधने काढली',
'revdelete-hid'               => 'लपवा $1',
'revdelete-unhid'             => 'अनहिड $1',
'revdelete-log-message'       => '$2 {{PLURAL:$2|आवॄत्ती|आवृत्त्यां}}साठी $1',
'logdelete-log-message'       => '$2 {{PLURAL:$2|घटने|घटनां}}साठी $1',

# Suppression log
'suppressionlog'     => 'सप्रेशन नोंद',
'suppressionlogtext' => 'खाली सर्वात अलीकडील ब्लॉक तसेच प्रबंधकांपासून लपविलेला मजकूर वगळण्याची यादी आहे. सध्या अस्तित्वात असेलेले प्रतिबंध पाहण्यासाठी [[Special:IPBlockList|IP ब्लॉक यादी]] पहा.',

# History merging
'mergehistory'                     => 'पान ईतिहासांचे एकत्रिकरण करा',
'mergehistory-header'              => 'हे पान एका स्रोत पानाचा इतिहास एखाद्या नविन पानात समाविष्ट करू देते.
हा बदल पानाचे ऐतिहासिक सातत्य राखेल याची दक्षता घ्या.',
'mergehistory-box'                 => 'दोन पानांची आवर्तने संमिलीत करा:',
'mergehistory-from'                => 'स्रोत पान:',
'mergehistory-into'                => 'लक्ष्य पान:',
'mergehistory-list'                => 'गोळाबेरीज करण्याजोगा संपादन इतिहास',
'mergehistory-merge'               => '[[:$1]]ची पूढील आवर्तने [[:$2]]मध्ये एकत्रित करता येतील.ठराविक वेळी अथवा तत्पूर्वी झालेल्या आवर्तनांचे एकत्रिकरण करण्याकरिता रेडीओ कळ स्तंभ वापरा.हा स्तंभ सुचालन दुवे वापरल्यास पूर्वपदावर येईल हे लक्षात घ्या.',
'mergehistory-go'                  => 'गोळाबेरीज करण्याजोगी संपादने दाखवा',
'mergehistory-submit'              => 'आवर्तने एकत्रित करा.',
'mergehistory-empty'               => 'कोणतेही आवर्तन एकत्रित करता येत नाही.',
'mergehistory-success'             => '[[:$1]] {{PLURAL:$3|चे|ची}} $3 {{PLURAL:$3|आवर्तन|आवर्तने}} [[:$2]] मध्ये यशस्वीरित्या एकत्रित केली.',
'mergehistory-fail'                => 'इतिहासाचे एकत्रिकरण कार्य करू शकत नाही आहे, कृपया पान आणि वेळ नियमावलीची पुर्नतपासणी करा.',
'mergehistory-no-source'           => 'स्रोत पान $1 अस्तित्वात नाही.',
'mergehistory-no-destination'      => 'लक्ष्य पान $1  अस्तित्वात नाही.',
'mergehistory-invalid-source'      => 'स्रोत पानाचे शीर्षक योग्य असणे आवश्यक आहे.',
'mergehistory-invalid-destination' => 'लक्ष्य पानाचे शीर्षक योग्य असणे आवश्यक आहे.',
'mergehistory-autocomment'         => '[[:$2]] मध्ये [[:$1]] एकत्रित केले',
'mergehistory-comment'             => '[[:$2]] मध्ये [[:$1]] एकत्रित केले: $3',

# Merge log
'mergelog'           => 'नोंदी एकत्र करा',
'pagemerge-logentry' => '[[$2]]मध्ये[[$1]] समाविष्ट केले ($3पर्यंतची आवर्तने)',
'revertmerge'        => 'वेगवेगळे करा',
'mergelogpagetext'   => 'एकापानाचा इतिहास इतर पानात टाकून अगदी अलिकडे एकत्रित केलेली एकत्रिकरणे निम्न्दर्शीत सूचीत आहेत.',

# Diffs
'history-title'           => '"$1" चा संपादन इतिहास',
'difference'              => '(आवर्तनांमधील फरक)',
'lineno'                  => 'ओळ $1:',
'compareselectedversions' => 'निवडलेल्या आवृत्त्यांमधील बदल पहा',
'editundo'                => 'उलटवा',
'diff-multi'              => '({{PLURAL:$1|मधील एक आवृत्ती|मधल्या $1 आवृत्त्या}} दाखवलेल्या नाहीत.)',

# Search results
'searchresults'             => 'शोध निकाल',
'searchresulttext'          => '{{SITENAME}} वरील माहिती कशी शोधावी, याच्या माहिती करता पहा - [[{{MediaWiki:Helppage}}|{{SITENAME}} वर शोध कसा घ्यावा]].',
'searchsubtitle'            => "तुम्ही '''[[:$1]]''' या शब्दाचा शोध घेत आहात.",
'searchsubtitleinvalid'     => "तुम्ही '''$1''' या शब्दाचा शोध घेत आहात.",
'noexactmatch'              => "'''\"\$1\" या मथळ्याचा लेख अस्तित्त्वात नाही.''' तुम्ही हा लेख [[:\$1|लिहु शकता]].",
'noexactmatch-nocreate'     => "'''येथे \"\$1\" शीर्षकाचे पान नाही.'''",
'toomanymatches'            => 'खूप एकसारखी उत्तरे मिळाली, कृपया पृच्छा वेगळ्या तर्‍हेने करून पहा',
'titlematches'              => 'पानाचे शीर्षक जुळते',
'notitlematches'            => 'कोणत्याही पानाचे शीर्षक जुळत नाही',
'textmatches'               => 'पानातील मजकुर जुळतो',
'notextmatches'             => 'पानातील मजकुराशी जुळत नाही',
'prevn'                     => 'मागील $1',
'nextn'                     => 'पुढील $1',
'viewprevnext'              => 'पहा ($1) ($2) ($3).',
'search-result-size'        => '$1 ({{PLURAL:$2|१ शब्द|$2 शब्द}})',
'search-result-score'       => 'जुळणी: $1%',
'search-redirect'           => '(पुनर्निर्देशन $1)',
'search-section'            => '(विभाग $1)',
'search-suggest'            => 'तुम्हाला हेच म्हणायचे का: $1',
'search-interwiki-caption'  => 'इतर प्रकल्प',
'search-interwiki-default'  => '$1चे निकाल:',
'search-interwiki-more'     => '(आणखी)',
'search-mwsuggest-enabled'  => 'सजेशन्स सहित',
'search-mwsuggest-disabled' => 'सजेशन्स नाहीत',
'search-relatedarticle'     => 'जवळील',
'mwsuggest-disable'         => 'AJAX सजेशन्स रद्द करा',
'searchrelated'             => 'जवळील',
'searchall'                 => 'सर्व',
'showingresults'            => "#'''$2'''पासून {{PLURAL:$1|'''1'''पर्यंतचा निकाल|'''$1'''पर्यंतचे निकाल}} खाली दाखवले आहे.",
'showingresultsnum'         => "खाली दिलेले #'''$2'''पासून सुरू होणारे  {{PLURAL:$3|'''1''' निकाल|'''$3''' निकाल}}.",
'showingresultstotal'       => "खाली '''$3''' पैकी {{PLURAL:$3|'''$1''' निकाल|'''$1 - $2''' निकाल}} दाखवित आहे",
'nonefound'                 => "'''सूचना''':काही नामविश्वेच नेहमी शोधली जातात. सर्व नामविश्वे शोधण्याकरीता (चर्चा पाने, साचे, इ. सकट) कॄपया शोधशब्दांच्या आधी ''all:'' लावून पहा किंवा पाहिजे असलेले नामविश्व लिहा.",
'powersearch'               => 'वाढीव शोध',
'powersearch-legend'        => 'वाढीव शोध',
'powersearch-ns'            => 'नामविश्वांमध्ये शोधा:',
'powersearch-redir'         => 'पुनर्निर्देशने दाखवा',
'powersearch-field'         => 'साठी शोधा',
'search-external'           => 'बाह्य शोध',
'searchdisabled'            => '{{SITENAME}} शोध अनुपलब्ध केला आहे.तो पर्यंत गूगलवरून शोध घ्या.{{SITENAME}}च्या मजकुराची त्यांची सूचिबद्धता शिळी असण्याची शक्यता असु शकते हे लक्षात घ्या.',

# Preferences page
'preferences'              => 'माझ्या पसंती',
'mypreferences'            => 'माझ्या पसंती',
'prefs-edits'              => 'संपादनांची संख्या:',
'prefsnologin'             => 'प्रवेश केलेला नाही',
'prefsnologintext'         => 'सदस्य पसंती बदलण्यासाठी [[Special:UserLogin|प्रवेश]] करावा लागेल.',
'prefsreset'               => 'पसंती पूर्ववत करण्यात आल्या आहेत.',
'qbsettings'               => 'शीघ्रपट',
'qbsettings-none'          => 'नाही',
'qbsettings-fixedleft'     => 'स्थिर डावे',
'qbsettings-fixedright'    => 'स्थिर ऊजवे',
'qbsettings-floatingleft'  => 'तरंगते डावे',
'qbsettings-floatingright' => 'तरंगते ऊजवे',
'changepassword'           => 'परवलीचा शब्द बदला',
'skin'                     => 'त्वचा',
'math'                     => 'गणित',
'dateformat'               => 'दिनांक लेखनशैली',
'datedefault'              => 'प्राथमिकता नाही',
'datetime'                 => 'दिनांक आणि वेळ',
'math_failure'             => 'पृथक्करणात अयशस्वी',
'math_unknown_error'       => 'अपरिचित त्रूटी',
'math_unknown_function'    => 'अज्ञात कार्य',
'math_lexing_error'        => 'लेक्झींग(कोशीय?)त्रूटी',
'math_syntax_error'        => 'आज्ञावली-विन्यास त्रूटी',
'math_image_error'         => 'PNG पालट अयशस्वी; latex, dvips, gs ची  स्थापना योग्य झाली आहे काय ते तपासा आणि बदल करा',
'math_bad_tmpdir'          => '"गणितीय तूर्त धारिके"(math temp directory)ची  निर्मीती करू शकत नाही अथवा "मॅथ तूर्त धारिकेत" लिहू शकत नाही .',
'math_bad_output'          => 'गणितीय प्राप्त धारिकेची( math output directory) निर्मीती अथवा त्यात लेखन करू शकत नाही.',
'math_notexvc'             => 'texvcकरणी(texvc एक्झिक्यूटेबल)चूकमुकली आहे;कृपया,सज्जीत करण्याकरिता math/README पहा.',
'prefs-personal'           => 'सदस्य व्यक्तिरेखा',
'prefs-rc'                 => 'अलीकडील बदल',
'prefs-watchlist'          => 'पहार्‍याची सूची',
'prefs-watchlist-days'     => 'पहार्‍याच्या सूचीत दिसणार्‍या दिवसांची संख्या:',
'prefs-watchlist-edits'    => 'वाढीव पहार्‍याच्या सूचीत दिसणार्‍या संपादनांची संख्या:',
'prefs-misc'               => 'इतर',
'saveprefs'                => 'जतन करा',
'resetprefs'               => 'न जतन केलेले बदल रद्द करा',
'oldpassword'              => 'जुना परवलीचा शब्दः',
'newpassword'              => 'नवीन परवलीचा शब्द:',
'retypenew'                => 'पुन्हा एकदा परवलीचा शब्द',
'textboxsize'              => 'संपादन',
'rows'                     => 'ओळी:',
'columns'                  => 'स्तंभ:',
'searchresultshead'        => 'शोध',
'resultsperpage'           => 'प्रति पान धडका:',
'contextlines'             => 'प्रति धडक ओळी:',
'contextchars'             => 'प्रतिओळ संदर्भ:',
'stub-threshold'           => '<a href="#" class="stub">अंकुरीत दुव्यांच्या</a> रचनेची नांदी (बाईट्स):',
'recentchangesdays'        => 'अलिकडील बदल मधील दाखवावयाचे दिवस:',
'recentchangescount'       => 'अलिकडील बदल, इतिहास व नोंद पानांमध्ये दाखवायाच्या संपादनांची संख्या:',
'savedprefs'               => 'तुमच्या पसंती जतन केल्या आहेत.',
'timezonelegend'           => 'काळवेळ प्रभाग',
'timezonetext'             => '¹विदागारदात्याच्या वेळेपासून(UTC) तुमच्या स्थानिक वेळेचा तासांनी फरक.',
'localtime'                => 'स्थानिक वेळ',
'timezoneoffset'           => 'समासफरक¹',
'servertime'               => 'विदागारदात्याची वेळ',
'guesstimezone'            => 'विचरकातून भरा',
'allowemail'               => 'इतर सदस्यांकडून इ-मेल येण्यास मुभा द्या',
'prefs-searchoptions'      => 'शोध विकल्प',
'prefs-namespaces'         => 'नामविश्वे',
'defaultns'                => 'या नामविश्वातील अविचल शोध :',
'default'                  => 'अविचल',
'files'                    => 'संचिका',

# User rights
'userrights'                  => 'सदस्य अधिकार व्यवस्थापन', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => 'सदस्य गटांचे(ग्रूप्स) व्यवस्थापन करा.',
'userrights-user-editname'    => 'सदस्य नाव टाका:',
'editusergroup'               => 'सदस्य गट (ग्रूप्स) संपादीत करा',
'editinguser'                 => "सदस्य '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])चे सदस्य अधिकारात बदल केला जात आहे.",
'userrights-editusergroup'    => 'सदस्य मंडळे संपादीत करा',
'saveusergroups'              => 'सदस्य गट जतन करा',
'userrights-groupsmember'     => '(चा) सभासद:',
'userrights-groups-help'      => 'तुम्ही एखाद्या सदस्याचे गट सदस्यत्व बदलू शकता:
* निवडलेला चौकोन म्हणजे सदस्य त्या गटात आहे.
* न निवडलेला चौकोन म्हणजे सदस्य त्या गटात नाही.
* एक * चा अर्थ तुम्ही एकदा समावेश केल्यानंतर तो गट बदलू शकत नाही, किंवा काढल्यानंतर समावेश करू शकत नाही.',
'userrights-reason'           => 'बदलाचे कारण:',
'userrights-no-interwiki'     => 'इतर विकींवरचे सदस्य अधिकार बदलण्याची परवानगी तुम्हाला नाही.',
'userrights-nodatabase'       => 'विदा $1 अस्तीत्वात नाही अथवा स्थानिक नाही.',
'userrights-nologin'          => 'सदस्य अधिकार देण्यासाठी तुम्ही प्रबंधक म्हणून [[Special:UserLogin|प्रवेश केलेला]] असणे आवश्यक आहे.',
'userrights-notallowed'       => 'तुमच्या सदस्य खात्यास सदस्य अधिकारांची निश्चिती करण्याची परवानगी नाही.',
'userrights-changeable-col'   => 'गट जे तुम्ही बदलू शकता',
'userrights-unchangeable-col' => 'गट जे तुम्ही बदलू शकत नाही',

# Groups
'group'               => 'गट:',
'group-user'          => 'सदस्य',
'group-autoconfirmed' => 'नोंदणीकृत सदस्य',
'group-bot'           => 'सांगकामे',
'group-sysop'         => 'प्रबंधक',
'group-bureaucrat'    => 'प्रशासक',
'group-suppress'      => 'झापडबंद',
'group-all'           => '(सर्व)',

'group-user-member'          => 'सदस्य',
'group-autoconfirmed-member' => 'स्वयंशाबीत सदस्य',
'group-bot-member'           => 'सांगकाम्या',
'group-sysop-member'         => 'प्रबंधक',
'group-bureaucrat-member'    => 'प्रशासक',
'group-suppress-member'      => 'झापडबंद',

'grouppage-user'          => '{{ns:project}}:सदस्य',
'grouppage-autoconfirmed' => '{{ns:project}}:नोंदणीकृत सदस्य',
'grouppage-bot'           => '{{ns:project}}:सांगकाम्या',
'grouppage-sysop'         => '{{ns:project}}:प्रबंधक',
'grouppage-bureaucrat'    => '{{ns:project}}:प्रशासक',
'grouppage-suppress'      => '{{ns:project}}:झापडबंद',

# Rights
'right-read'                 => 'पृष्ठे वाचा',
'right-edit'                 => 'पाने संपादा',
'right-createpage'           => 'पृष्ठे तयार करा',
'right-createtalk'           => 'चर्चा पृष्ठे तयार करा',
'right-createaccount'        => 'नवीन सदस्य खाती तयार करा',
'right-minoredit'            => 'बदल छोटे म्हणून जतन करा',
'right-move'                 => 'पानांचे स्थानांतरण करा',
'right-move-subpages'        => 'पाने उपपानांसकट हलवा',
'right-suppressredirect'     => 'एखाद्या पानाचे नवीन नावावर स्थानांतरण करत असताना पुनर्निर्देशन वगळा',
'right-upload'               => 'संचिका चढवा',
'right-reupload'             => 'अस्तित्वात असलेल्या संचिकेवर पुनर्लेखन करा',
'right-reupload-own'         => 'त्याच सदस्याने चढविलेल्या संचिकेवर पुनर्लेखन करा',
'right-reupload-shared'      => 'स्थानिक पातळीवरून शेअर्ड चित्र धारिकेतील संचिकांवर पुनर्लेखन करा',
'right-upload_by_url'        => 'एखादी संचिका URL सहित चढवा',
'right-purge'                => 'एखाद्या पानाची सय रिकामी करा',
'right-autoconfirmed'        => 'नोंदणीकृत सदस्याप्रमाणे वागणूक मिळवा',
'right-bot'                  => 'स्वयंचलित कार्याप्रमाणे वागणूक मिळवा',
'right-nominornewtalk'       => 'चर्चा पृष्ठावर छोटी संपादने जी नवीन चर्चा दर्शवितात ती नकोत',
'right-apihighlimits'        => 'API पृच्छांमध्ये वरची मर्यादा वापरा',
'right-writeapi'             => 'लिखित API चा उपयोग',
'right-delete'               => 'पृष्ठे वगळा',
'right-bigdelete'            => 'जास्त इतिहास असणारी पाने वगळा',
'right-deleterevision'       => 'एखाद्या पानाच्या विशिष्ट आवृत्त्या लपवा',
'right-deletedhistory'       => 'वगळलेल्या इतिहास नोंदी, त्यांच्या संलग्न मजकूराशिवाय पहा',
'right-browsearchive'        => 'वगळलेली पाने पहा',
'right-undelete'             => 'एखादे पान पुनर्स्थापित करा',
'right-suppressrevision'     => 'लपविलेल्या आवृत्त्या पहा व पुनर्स्थापित करा',
'right-suppressionlog'       => 'खासगी नोंदी पहा',
'right-block'                => 'इतर सदस्यांना संपादन करण्यास बंदी करा',
'right-blockemail'           => 'एखाद्या सदस्याला इ-मेल पाठविण्यापासून थांबवा',
'right-hideuser'             => 'एखादे सदस्य नाव इतरांपासून लपवा',
'right-ipblock-exempt'       => 'आइपी ब्लॉक्स कडे दुर्लक्ष करा',
'right-proxyunbannable'      => 'प्रॉक्सी असताना ब्लॉक्स कडे दुर्लक्ष करा',
'right-protect'              => 'सुरक्षितता पातळी बदला',
'right-editprotected'        => 'सुरक्षित पाने संपादा',
'right-editinterface'        => 'सदस्य पसंती बदला',
'right-editusercssjs'        => 'इतर सदस्यांच्या CSS व JS संचिका संपादित करा',
'right-rollback'             => 'एखादे विशिष्ट पान ज्याने संपादन केले त्याला लवकर पूर्वपदास न्या',
'right-markbotedits'         => 'निवडलेली संपादने सांगकाम्यांची म्हणून जतन करा',
'right-noratelimit'          => 'रेट लिमिट्स चा परिणाम होत नाही.',
'right-import'               => 'इतर विकिंमधून पाने आयात करा',
'right-importupload'         => 'चढविलेल्या संचिकेतून पाने आयात करा',
'right-patrol'               => 'संपादने तपासलेली (patrolled) म्हणून जतन करा',
'right-autopatrol'           => 'संपादने आपोआप तपासलेली (patrolled) म्हणून जतन करा',
'right-patrolmarks'          => 'अलीकडील बदलांमधील तपासल्याच्या खूणा पहा',
'right-unwatchedpages'       => 'न पाहिलेल्या पानांची यादी पहा',
'right-trackback'            => 'एक ट्रॅकबॅक पाठवा',
'right-mergehistory'         => 'पानांचा इतिहास एकत्रित करा',
'right-userrights'           => 'सर्व सदस्यांचे अधिकार संपादा',
'right-userrights-interwiki' => 'इतर विकिंवर सदस्य अधिकार बदला',
'right-siteadmin'            => 'डाटाबेस ला कुलुप लावा अथवा काढा',

# User rights log
'rightslog'      => 'सदस्य आधिकार नोंद',
'rightslogtext'  => 'ही सदस्य अधिकारांमध्ये झालेल्या बदलांची यादी आहे.',
'rightslogentry' => '$1 चे ग्रुप सदस्यत्व $2 पासून $3 ला बदलण्यात आलेले आहे',
'rightsnone'     => '(काहीही नाही)',

# Recent changes
'nchanges'                          => '$1 {{PLURAL:$1|बदल|बदल}}',
'recentchanges'                     => 'अलीकडील बदल',
'recentchangestext'                 => 'विकितील अलीकडील बदल या पानावर दिसतात.',
'recentchanges-feed-description'    => 'या रसदीमधील विकीवर झालेले सर्वात अलीकडील बदल पहा.',
'rcnote'                            => "खाली $4, $5 पर्यंतचे गेल्या {{PLURAL:$2|'''१''' दिवसातील|'''$2''' दिवसांतील}} {{PLURAL:$1|शेवटचा '''1''' बदल|शेवटचे '''$1''' बदल}} दिलेले आहेत.",
'rcnotefrom'                        => 'खाली <b>$2</b> पासूनचे (<b>$1</b> किंवा कमी) बदल दाखवले आहेत.',
'rclistfrom'                        => '$1 नंतर केले गेलेले बदल दाखवा.',
'rcshowhideminor'                   => 'छोटे बदल $1',
'rcshowhidebots'                    => 'सांगकामे(बॉट्स) $1',
'rcshowhideliu'                     => 'प्रवेश केलेले सदस्य $1',
'rcshowhideanons'                   => 'अनामिक सदस्य $1',
'rcshowhidepatr'                    => '$1 पहारा असलेली संपादने',
'rcshowhidemine'                    => 'माझे बदल $1',
'rclinks'                           => 'मागील $2 दिवसांतील $1 बदल पहा.<br />$3',
'diff'                              => 'फरक',
'hist'                              => 'इति',
'hide'                              => 'लपवा',
'show'                              => 'पहा',
'minoreditletter'                   => 'छो',
'newpageletter'                     => 'न',
'boteditletter'                     => 'सां',
'number_of_watching_users_pageview' => '[$1 {{PLURAL:$1|सदस्याने|सदस्यांनी}} पहारा दिलेला आहे]',
'rc_categories'                     => 'वर्गांपपुरते मर्यादीत ठेवा ("|"ने वेगळे करा)',
'rc_categories_any'                 => 'कोणतेही',
'newsectionsummary'                 => '/* $1 */ नवीन विभाग',

# Recent changes linked
'recentchangeslinked'          => 'या पृष्ठासंबंधीचे बदल',
'recentchangeslinked-title'    => '"$1" च्या संदर्भातील बदल',
'recentchangeslinked-noresult' => 'जोडलेल्या पानांमध्ये दिलेल्या कालावधीत काहीही बदल झालेले नाहीत.',
'recentchangeslinked-summary'  => "हे पृष्ठ एखाद्या विशिष्ट पानाशी (किंवा एखाद्या विशिष्ट वर्गात असणार्‍या पानांशी) जोडलेल्या पानांवरील बदल दर्शवते.
तुमच्या पहार्‍याच्या सूचीतील पाने '''ठळक''' दिसतील.",
'recentchangeslinked-page'     => 'पृष्ठ नाव:',
'recentchangeslinked-to'       => 'याऐवजी दिलेल्या पानाला जोडलेल्या पानांवरील बदल दाखवा',

# Upload
'upload'                      => 'संचिका चढवा',
'uploadbtn'                   => 'संचिका चढवा',
'reupload'                    => 'पुन्हा चढवा',
'reuploaddesc'                => 'चढवायच्या पानाकडे परता',
'uploadnologin'               => 'प्रवेश केलेला नाही',
'uploadnologintext'           => 'संचिका चढविण्यासाठी तुम्हाला [[Special:UserLogin|प्रवेश]] करावा लागेल.',
'upload_directory_missing'    => 'अपलोड डिरेक्टरी ($1) सापडली नाही तसेच वेबसर्व्हर ती तयार करू शकलेला नाही.',
'upload_directory_read_only'  => '$1 या डिरेक्टरी मध्ये सर्व्हर लिहू शकत नाही.',
'uploaderror'                 => 'चढवण्यात चुक',
'uploadtext'                  => "खालील अर्ज नवीन संचिका चढविण्यासाठी वापरा.
पूर्वी चढविलेल्या संचिका पाहण्यासाठी अथवा शोधण्यासाठी [[Special:ImageList|चढविलेल्या संचिकांची यादी]] पहा. चढविलेल्या तसेच वगळलेल्या संचिकांची यादी पहाण्यासाठी [[Special:Log/upload|सूची]] पहा.

एखाद्या लेखात ही संचिका वापरण्यासाठी खालीलप्रमाणे दुवा द्या
'''<nowiki>[[</nowiki>{{ns:image}}<nowiki>:File.jpg]]</nowiki>''',
'''<nowiki>[[</nowiki>{{ns:image}}<nowiki>:File.png|alt text]]</nowiki>''' किंवा
'''<nowiki>[[</nowiki>{{ns:media}}<nowiki>:File.ogg]]</nowiki>''' संचिकेला थेट दुवा देण्यासाठी वापरा.",
'upload-permitted'            => 'अनुमतीत संचिका वर्ग: $1.',
'upload-preferred'            => 'श्रेयस्कर संचिका प्रकार:$1.',
'upload-prohibited'           => 'प्रतिबंधीत संचिका प्रकार: $1.',
'uploadlog'                   => 'चढवल्याची नोंद',
'uploadlogpage'               => 'चढवल्याची नोंद',
'uploadlogpagetext'           => 'नवीनतम चढवलेल्या संचिकांची यादी.',
'filename'                    => 'संचिकेचे नाव',
'filedesc'                    => 'वर्णन',
'fileuploadsummary'           => 'आढावा:',
'filestatus'                  => 'प्रताधिकार स्थिती:',
'filesource'                  => 'स्रोत:',
'uploadedfiles'               => 'चढवलेल्या संचिका',
'ignorewarning'               => 'सुचनेकडे दुर्लक्ष करा आणि संचिका जतन करा.',
'ignorewarnings'              => 'सर्व सुचनांकडे दुर्लक्ष करा',
'minlength1'                  => 'संचिकानाम किमान एक अक्षराचे हवे.',
'illegalfilename'             => '"$1" या संचिकानामात शीर्षकात चालू न शकणारी अक्षरे आहेत. कृपया संचिकानाम बदलून पुन्हा चढवण्याचा प्रयत्न करा.',
'badfilename'                 => 'संचिकेचे नाव बदलून "$1" असे केले आहे.',
'filetype-badmime'            => 'विविधामाप(माईम) "$1" प्रकारच्या संचिका चढवण्यास परवानगी नाही.',
'filetype-unwanted-type'      => "'''\".\$1\"''' ही नको असलेल्या प्रकारची संचिका आहे. \$2 {{PLURAL:\$3|ही हव्या असलेल्या प्रकारची संचिका आहे|ह्या हव्या असलेल्या प्रकारच्या संचिका आहेत}}.",
'filetype-banned-type'        => "'''\".\$1\"''' ही परवानगी नसलेल्या प्रकारची संचिका आहे. \$2 ह्या परवानगी असलेल्या प्रकारच्या संचिका आहेत.",
'filetype-missing'            => 'या संचिकेला एक्सटेंशन दिलेले नाही (उदा. ".jpg").',
'large-file'                  => 'संचिका $1 पेक्षा कमी आकाराची असण्याची अपेक्षा आहे, ही संचिका $2 एवढी आहे.',
'largefileserver'             => 'सेवा संगणकावर (सर्वर) निर्धारित केलेल्या आकारापेक्षा या संचिकेचा आकार मोठा आहे.',
'emptyfile'                   => 'चढवलेली संचिका रिकामी आहे. हे संचिकानाम चुकीचे लिहिल्याने असू शकते. कृपया तुम्हाला हीच संचिका चढवायची आहे का ते तपासा.',
'fileexists'                  => 'या नावाची संचिका आधीच अस्तित्वात आहे, कृपया ही संचिका बदलण्याबद्दल तुम्ही साशंक असाल तर <strong><tt>$1</tt></strong> तपासा.',
'filepageexists'              => 'या नावाचे एक माहितीपृष्ठ (संचिका नव्हे) अगोदरच अस्तित्त्वात आहे. कृपया जर आपणांस त्यात बदल करायचा नसेल तर <strong><tt>$1</tt></strong> तपासा.',
'fileexists-extension'        => 'या नावाची संचिका अस्तित्वात आहे:<br />
चढवित असलेल्या संचिकेचे नाव: <strong><tt>$1</tt></strong><br />
अस्तित्वात असलेल्या संचिकेचे नाव: <strong><tt>$2</tt></strong><br />
कृपया दुसरे नाव निवडा.',
'fileexists-thumb'            => "<center>'''सध्याची संचिका'''</center>",
'fileexists-thumbnail-yes'    => 'आपण चढवित असलेली संचिका ही मोठ्या चित्राची झलक <i>(thumbnail)</i> असण्याची शक्यता आहे. कृपया <strong><tt>$1</tt></strong> ही संचिका तपासा.<br />
जर तपासलेली संचिका ही याच आकाराची असेल तर नवीन झलक चढविण्याची गरज नाही.',
'file-thumbnail-no'           => 'या संचिकेचे नाव <strong><tt>$1</tt></strong> पासून सुरू होत आहे. ही कदाचित झलक <i>(thumbnail)</i> असू शकते.
जर तुमच्या कडे पूर्ण रिझोल्यूशनची संचिका असेल तर चढवा अथवा संचिकेचे नाव बदला.',
'fileexists-forbidden'        => 'या नावाची संचिका अगोदरच अस्तित्त्वात आहे; कृपया पुन्हा मागे जाऊन ही संचिका नवीन नावाने चढवा.
[[Image:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => 'हे नाव असलेली एक संचिका शेअर्ड संचिका कोशात आधी पासून आहे; कृपया परत फिरा आणि नविन(वेगळ्या) नावाने ही संचिका पुन्हा चढवा.[[Image:$1|इवले|मध्य|$1]]',
'file-exists-duplicate'       => 'ही संचिका खालील {{PLURAL:$1|संचिकेची|संचिकांची}} प्रत आहे:',
'successfulupload'            => 'यशस्वीरीत्या चढवले',
'uploadwarning'               => 'चढवताना सूचना',
'savefile'                    => 'संचिका जतन करा',
'uploadedimage'               => '"[[$1]]" ही संचिका चढवली',
'overwroteimage'              => '"[[$1]]" या संचिकेची नवीन आवृत्ती चढविली.',
'uploaddisabled'              => 'संचिका चढविण्यास बंदी घालण्यात आलेली आहे.',
'uploaddisabledtext'          => '{{SITENAME}} वर संचिका चढविण्यास बंदी घालण्यात आलेली आहे.',
'uploadscripted'              => 'या संचिकेत HTML किंवा स्क्रिप्ट कोडचा आंतर्भाव आहे, त्याचा एखाद्या विचरकाकडून विचीत्र अर्थ लावला जाऊ शकतो.',
'uploadcorrupt'               => 'ही संचिका भ्रष्ट आहे किंवा तिचे नाव व्यवस्थित नाही. कृपया संचिका तपासा आणि पुन्हा चढवा.',
'uploadvirus'                 => 'ह्या संचिकेत व्हायरस आहे. अधिक माहिती: $1',
'sourcefilename'              => 'स्रोत-संचिकानाम:',
'destfilename'                => 'नवे संचिकानाम:',
'upload-maxfilesize'          => 'जास्तीतजास्त संचिका आकार: $1',
'watchthisupload'             => 'या पानावर बदलांसाठी लक्ष ठेवा.',
'filewasdeleted'              => 'या नावाची संचिका या पूर्वी एकदा चढवून नंतर वगळली होती.तुम्ही ती पुन्हा चढवण्या अगोदर $1 तपासा.',
'upload-wasdeleted'           => "'''सूचना: पूर्वी वगळण्यात आलेली संचिका तुम्ही पुन्हा चढवित आहात.'''

कृपया तुम्ही करत असलेली कृती योग्य असल्याची खात्री करून घ्या.
तुमच्या सोयीसाठी वगळल्याची नोंद पुढीलप्रमाणे:",
'filename-bad-prefix'         => 'तुम्ही चढवत असलेल्या संचिकेचे नाव <strong>"$1"</strong> पासून सुरू होते, जे की अंकीय छाउ (कॅमेरा) ने दिलेले अवर्णनात्मक नाव आहे.कृपया तुमच्या संचिकेकरिता अधिक वर्णनात्मक नाव निवडा.',

'upload-proto-error'      => 'चूकीचा संकेत',
'upload-proto-error-text' => 'दूरस्थ चढवण्याच्या क्रियेत <code>http://</code>पासून किंवा <code>ftp://</code>पासून सूरू होणारी URL लागतात.',
'upload-file-error'       => 'अंतर्गत त्रूटी',
'upload-file-error-text'  => 'विदादात्यावर तात्पुरती संचिका तयार करण्याच्या प्रयत्न करत असताना अंतर्गत तांत्रिक अडचण आली.कृपया प्रचालकांशी संपर्क करा.',
'upload-misc-error'       => 'संचिका चढविताना माहित नसलेली त्रूटी आलेली आहे.',
'upload-misc-error-text'  => 'चढवताना अज्ञात तांत्रिक आदचण आली.कृपया URL सुयोग्य आणि उपलब्ध आहे का ते तपासा आणि पुन्हा प्रयत्न करा.जर अडचणे भेडसावणे चालूच राहीले तर प्रचालकांसी संपर्क करा.',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => 'URLपाशी पोहोचले नाही',
'upload-curl-error6-text'  => 'दिलेल्या URL ला पोहचू शकलो नाही.कृपया URL बरोबर असून संकेतस्थळ चालू असल्याची पुनश्च खात्री करा.',
'upload-curl-error28'      => 'चढवण्यात वेळगेली',
'upload-curl-error28-text' => 'संकेतस्थळाने साद देण्यात खूप जास्त वेळ घेतला आहे,कृपया थोडा वेळ थांबा आणि पुन्हा प्रयत्न करा.कदाचित तुम्ही कमी गर्दीच्या वेळात प्रयत्न करू इच्छीताल.',

'license'            => 'परवाना:',
'nolicense'          => 'काही निवडलेले नाही',
'license-nopreview'  => '(झलक उपलब्ध नाही)',
'upload_source_url'  => '(एक सुयोग्य,सार्वजनिकरित्या उपलब्ध URL)',
'upload_source_file' => '(तुमच्या संगणकावरील एक संचिका)',

# Special:ImageList
'imagelist-summary'     => 'हे विशेष पान सर्व चढविलेल्या संचिका दर्शिविते.
सर्वसाधारणपणे सगळ्यात शेवटी बदल झालेल्या संचिका सर्वात वर दिसतात.
रकान्याच्या नावापुढे टिचकी देऊन संचिकांचा अनुक्रम बदलता येतो.',
'imagelist_search_for'  => 'चित्र नावाने शोध:',
'imgfile'               => 'संचिका',
'imagelist'             => 'चित्र यादी',
'imagelist_date'        => 'दिनांक',
'imagelist_name'        => 'नाव',
'imagelist_user'        => 'सदस्य',
'imagelist_size'        => 'आकार (बाईट्स)',
'imagelist_description' => 'वर्णन',

# Image description page
'filehist'                       => 'संचिकेचा इतिहास',
'filehist-help'                  => 'संचिकेची पूर्वीची आवृत्ती बघण्यासाठी दिनांक/वेळ वर टिचकी द्या.',
'filehist-deleteall'             => 'सर्व वगळा',
'filehist-deleteone'             => 'वगळा',
'filehist-revert'                => 'उलटवा',
'filehist-current'               => 'सद्य',
'filehist-datetime'              => 'दिनांक/वेळ',
'filehist-user'                  => 'सदस्य',
'filehist-dimensions'            => 'आकार',
'filehist-filesize'              => 'संचिकेचा आकार (बाईट्स)',
'filehist-comment'               => 'प्रतिक्रीया',
'imagelinks'                     => 'चित्र दुवे',
'linkstoimage'                   => 'खालील पाने या चित्राशी जोडली आहेत:',
'nolinkstoimage'                 => 'या चित्राशी जोडलेली पृष्ठे नाही आहेत.',
'morelinkstoimage'               => 'या संचिकेचे [[Special:WhatLinksHere/$1|अधिक दुवे]] पहा.',
'redirectstofile'                => 'खालील संचिका या संचिकेकडे पुनर्निर्देशन करतात:',
'duplicatesoffile'               => 'खालील संचिका या दिलेल्या संचिकेच्या प्रती आहेत:',
'sharedupload'                   => 'ही संचिका इतरही प्रकल्पांमध्ये वापरली गेल्याची शक्यता आहे.',
'shareduploadwiki'               => 'अधिक माहितीसाठी $1 पहा.',
'shareduploadwiki-desc'          => 'माहिती जी $1 वर शेअर्ड रिपॉझिटरी मध्ये आहे ती खाली दिलेली आहे.',
'shareduploadwiki-linktext'      => 'संचिका वर्णन पान',
'shareduploadduplicate'          => 'ही संचिका शेअर्ड रिपॉझिटरी मधील $1ची प्रत आहे.',
'shareduploadduplicate-linktext' => 'दुसर्‍या संचिके',
'shareduploadconflict'           => 'या संचिकेचे नाव शेअर्ड रिपॉझिटरी मधील $1शी जुळते.',
'shareduploadconflict-linktext'  => 'दुसर्‍या संचिके',
'noimage'                        => 'या नावाचे चित्र अस्तित्त्वात नाही. $1 करून पहा.',
'noimage-linktext'               => 'चढवा',
'uploadnewversion-linktext'      => 'या संचिकेची नवीन आवृत्ती चढवा',
'imagepage-searchdupe'           => 'जुळ्या संचिका शोधा',

# File reversion
'filerevert'                => '$1 पूर्वपद',
'filerevert-legend'         => 'संचिका पूर्वपदास',
'filerevert-intro'          => 'तुम्ही [$3, $2 प्रमाणे आवर्तन$4 कडे] [[Media:$1|$1]]  उलटवत आहात.',
'filerevert-comment'        => 'प्रतिक्रीया:',
'filerevert-defaultcomment' => '$2, $1 च्या आवृत्तीत पूर्वपदास',
'filerevert-submit'         => 'पूर्वपद',
'filerevert-success'        => "[$3, $2 प्रमाणे आवर्तन $4]कडे '''[[Media:$1|$1]]''' उलटवण्यात आली.",
'filerevert-badversion'     => 'दिलेलेल्या वेळ मापनानुसार,या संचिकेकरिता कोणतीही पूर्वीची स्थानिक आवृत्ती नाही.',

# File deletion
'filedelete'                  => '$1 वगळा',
'filedelete-legend'           => 'संचिका वगळा',
'filedelete-intro'            => "तुम्ही '''[[Media:$1|$1]]''' वगळत आहात.",
'filedelete-intro-old'        => "[$4 $3, $2]च्या वेळेचे '''[[Media:$1|$1]]'''चे आवर्तन तुम्ही वगळत आहात.",
'filedelete-comment'          => 'वगळ्ण्याची कारणे:',
'filedelete-submit'           => 'वगळा',
'filedelete-success'          => "'''$1'''वगळण्यात आले.",
'filedelete-success-old'      => '<span class="plainlinks">$3, $2 वेळी \'\'\'[[Media:$1|$1]]\'\'\' चे आवर्तन वगळण्यात आले आहे .</span>',
'filedelete-nofile'           => "'''$1''' {{SITENAME}}वर अस्तित्वात नाही.",
'filedelete-nofile-old'       => "सांगितलेल्या गुणधर्मानुसार  '''$1'''चे कोणतेही विदा आवर्तन संचित नाही.",
'filedelete-iscurrent'        => 'संचिकचे सर्वात अलिकडील आवर्तन वगळण्याचा तुम्ही प्रयत्न करत आहात.कृपया आधी जुने आवर्तन उलटवा.',
'filedelete-otherreason'      => 'इतर/शिवाय अधिक कारण:',
'filedelete-reason-otherlist' => 'इतर कारण',
'filedelete-reason-dropdown'  => '*वगळण्याची सामान्य कारणे
** प्रताधिकार उल्लंघन
** जूळी संचिका',
'filedelete-edit-reasonlist'  => 'वगळण्याची कारणे संपादीत करा',

# MIME search
'mimesearch'         => 'विविधामाप (माईम) शोधा',
'mimesearch-summary' => 'हे पान विविधामाप (माईम)-प्रकारांकरिता संचिकांची चाळणी करण्याची सुविधा पुरवते:
Input:contenttype/subtype, e.g. <tt>image/jpeg</tt>.',
'mimetype'           => 'विविधामाप (माईम) प्रकार:',
'download'           => 'उतरवा',

# Unwatched pages
'unwatchedpages' => 'लक्ष नसलेली पाने',

# List redirects
'listredirects' => 'पुनर्निर्देशने दाखवा',

# Unused templates
'unusedtemplates'     => 'न वापरलेले साचे',
'unusedtemplatestext' => 'या पानावर साचा नामविश्वातील अशी सर्व पाने आहेत जी कुठल्याही पानात वापरलेली नाहीत. वगळण्यापुर्वी साच्यांना जोडणारे इतर दुवे पाहण्यास विसरू नका.',
'unusedtemplateswlh'  => 'इतर दुवे',

# Random page
'randompage'         => 'अविशिष्ट लेख',
'randompage-nopages' => 'या नामविश्वात कोणतीही पाने नाहीत.',

# Random redirect
'randomredirect'         => 'अविशिष्ट पुनर्निर्देशन',
'randomredirect-nopages' => 'या नामविश्वात कोणतीही पुर्ननिर्देशने नाहीत.',

# Statistics
'statistics'             => 'सांख्यिकी',
'sitestats'              => 'स्थळ सांख्यिकी',
'userstats'              => 'सदस्य सांख्यिकी',
'sitestatstext'          => "{{PLURAL:$1|'''1'''पान|'''$1'''एकुण पाने}}विदागारात आहेत.
यात {{SITENAME}}चर्चा पाने, सदस्यांबद्दलची पाने, पुनर्निर्देशने, नावापुरती तयार केलेली पाने,आणि अशी पाने की जी मजकुर लेख नाहीत अशांचा समावेश होतो .
तशी पाने सोडून,{{PLURAL:$2|'''1''' पान आहे जे की|'''$2'''पाने आहेत जी की}} बहुधा योग्य मजकुर असलेले {{PLURAL:$2|पान|पाने}}आहेत.

'''$8''' {{PLURAL:$8|संचिका|संचिका}}चढवल्या आहेत.

{{SITENAME}}स्थापनेपासून '''$3''' {{PLURAL:$3|पानास भेट|पानास भेटी}},आणि '''$4''' {{PLURAL:$4|संपादनपान |संपादन पान}}.
त्याची सरासरी संपादने  '''$5'''प्रतिपान,आणि '''$6''' भेटी प्रति संपादन.

[http://www.mediawiki.org/wiki/Manual:Job_queue job queue]लांबी'''$7'''आहे.",
'userstatstext'          => "सध्या {{PLURAL:$1|is '''1''' registered [[Special:ListUsers|user]]| '''$1''' नोंदीकृत  [[Special:ListUsers|सदस्य]]}} आहेत, पैकी'''$2''' (किंवा '''$4%''')सदस्यांना $5 अधिकार {{PLURAL:$2|आहे|आहेत }} .",
'statistics-mostpopular' => 'सर्वाधिक बघितली जाणारी पाने',

'disambiguations'      => 'नि:संदिग्धकरण पृष्ठे',
'disambiguationspage'  => 'Template:नि:संदिग्धीकरण',
'disambiguations-text' => "निम्नलिखीत पाने एका '''नि:संदिग्धकरण पृष्ठास'''जोडली जातात. त्याऐवजी ती सुयोग्य विषयाशी जोडली जावयास हवीत.<br /> जर जर एखादे पान [[MediaWiki:Disambiguationspage]]पासून जोडलेला साचा वापरत असेल तर ते पान '''नि:संदिग्धकरण पृष्ठ''' गृहीत धरले जाते",

'doubleredirects'     => 'दुहेरी-पुनर्निर्देशने',
'doubleredirectstext' => 'हे पान अशा पानांची सूची पुरवते की जी पुर्ननिर्देशीत पाने दुसर्‍या पुर्ननिर्देशीत पानाकडे निर्देशीत झाली आहेत.प्रत्येक ओळीत पहिल्या आणि दुसर्‍या पुर्ननिर्देशनास दुवा दीला आहे सोबतच दुसरे पुर्ननिर्देशन ज्या पानाकडे पोहचते ते पण दिले आहे, जे की बरोबर असण्याची शक्यता आहे ,ते वस्तुत: पहिल्या पानापासूनचेही पुर्ननिर्देशन असावयास हवे.',

'brokenredirects'        => 'मोडके पुनर्निर्देशन',
'brokenredirectstext'    => 'खालील पुनर्निर्देशने अस्तित्वात नसलेली पाने जोडतात:',
'brokenredirects-edit'   => '(संपादा)',
'brokenredirects-delete' => '(वगळा)',

'withoutinterwiki'         => 'आंतरविकि दुवे नसलेली पाने',
'withoutinterwiki-summary' => 'खालील लेखात इतर भाषांमधील आवृत्तीला दुवे नाहीत:',
'withoutinterwiki-legend'  => 'उपपद',
'withoutinterwiki-submit'  => 'दाखवा',

'fewestrevisions' => 'सगळ्यात कमी बदल असलेले लेख',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|बाइट|बाइट}}',
'ncategories'             => '$1 {{PLURAL:$1|वर्ग|वर्ग}}',
'nlinks'                  => '$1 {{PLURAL:$1|दुवा|दुवे}}',
'nmembers'                => '$1 {{PLURAL:$1|सदस्य|सदस्य}}',
'nrevisions'              => '$1 {{PLURAL:$1|आवर्तन|आवर्तने}}',
'nviews'                  => '$1 {{PLURAL:$1|दृषीपथ|दृषीपथ}}',
'specialpage-empty'       => 'या अहवालाकरिता(रिपोर्ट)कोणताही निकाल नाही.',
'lonelypages'             => 'पोरकी पाने',
'lonelypagestext'         => 'खालील पानांना {{SITENAME}}च्या इतर पानांकडून दूवा जोड झालेली नाही.',
'uncategorizedpages'      => 'अवर्गीकृत पाने',
'uncategorizedcategories' => 'अवर्गीकृत वर्ग',
'uncategorizedimages'     => 'अवर्गीकृत चित्रे',
'uncategorizedtemplates'  => 'अवर्गीकृत साचे',
'unusedcategories'        => 'न वापरलेले वर्ग',
'unusedimages'            => 'न वापरलेल्या संचिका',
'popularpages'            => 'प्रसिद्ध पाने',
'wantedcategories'        => 'पाहिजे असलेले वर्ग',
'wantedpages'             => 'पाहिजे असलेले लेख',
'missingfiles'            => 'हरवलेल्या संचिका',
'mostlinked'              => 'सर्वाधिक जोडलेली पाने',
'mostlinkedcategories'    => 'सर्वाधिक जोडलेले वर्ग',
'mostlinkedtemplates'     => 'सर्वाधिक जोडलेले साचे',
'mostcategories'          => 'सर्वाधिक वर्गीकृत पाने',
'mostimages'              => 'सर्वाधिक जोडलेली चित्रे',
'mostrevisions'           => 'सर्वाधिक बदललेले लेख',
'prefixindex'             => 'उपसर्ग सूची',
'shortpages'              => 'छोटी पाने',
'longpages'               => 'मोठी पाने',
'deadendpages'            => 'टोकाची पाने',
'deadendpagestext'        => 'या पानांवर या विकिवरील इतर कुठल्याही पानाला जोडणारा दुवा नाही.',
'protectedpages'          => 'सुरक्षित पाने',
'protectedpages-indef'    => 'फक्त अनंत काळासाठी सुरक्षित केलेले',
'protectedpagestext'      => 'खालील पाने स्थानांतरण किंवा संपादन यांपासुन सुरक्षित आहेत',
'protectedpagesempty'     => 'सध्या या नियमावलीने कोणतीही पाने सुरक्षीत केलेली नाहीत.',
'protectedtitles'         => 'सुरक्षीत शीर्षके',
'protectedtitlestext'     => 'पुढील शिर्षके बदल घडवण्यापासून सुरक्षीत आहेत.',
'protectedtitlesempty'    => 'या नियमावलीने सध्या कोणतीही शीर्षके सुरक्षीत केलेली नाहीत.',
'listusers'               => 'सदस्यांची यादी',
'newpages'                => 'नवीन पाने',
'newpages-username'       => 'सदस्य नाव:',
'ancientpages'            => 'जुनी पाने',
'move'                    => 'स्थानांतरण',
'movethispage'            => 'हे पान स्थानांतरित करा',
'unusedimagestext'        => 'कृपया लक्षात घ्या की इतर संकेतस्थळे संचिकेशी थेट दुव्याने जोडल्या असू शकतात, त्यामुळे सक्रिय उपयोगात असून सुद्धा यादीत असू शकतात.',
'unusedcategoriestext'    => 'खालील वर्ग पाने अस्तित्वात आहेत पण कोणतेही लेख किंवा वर्ग त्यांचा वापर करत नाहीत.',
'notargettitle'           => 'कर्म(target) नाही',
'notargettext'            => 'ही क्रिया करण्यासाठी तुम्ही सदस्य किंवा पृष्ठ लिहिले नाही.',
'nopagetitle'             => 'असे लक्ष्य पान नाही',
'nopagetext'              => 'तुम्ही दिलेले लक्ष्य पान अस्तित्वात नाही.',
'pager-newer-n'           => '{{PLURAL:$1|नवे 1|नवे $1}}',
'pager-older-n'           => '{{PLURAL:$1|जुने 1|जुने $1}}',
'suppress'                => 'झापडबंद',

# Book sources
'booksources'               => 'पुस्तक स्रोत',
'booksources-search-legend' => 'पुस्तक स्रोत शोधा',
'booksources-go'            => 'चला',
'booksources-text'          => 'खालील यादीत नवी आणिजुनी पुस्तके विकणार्‍या संकेतस्थळाचे दुवे आहेत,आणि त्यात कदाचित आपण शोधू पहात असलेल्या पुस्तकाची अधिक माहिती असेल:',

# Special:Log
'specialloguserlabel'  => 'सदस्य:',
'speciallogtitlelabel' => 'शीर्षक:',
'log'                  => 'नोंदी',
'all-logs-page'        => 'सर्व नोंदी',
'log-search-legend'    => 'नोंदी शोधा',
'log-search-submit'    => 'चला',
'alllogstext'          => '{{SITENAME}}च्या सर्व नोंदीचे एकत्र दर्शन.नोंद प्रकार, सदस्यनाव किंवा बाधीत पान निवडून तुम्ही तुमचे दृश्यपान मर्यादीत करू शकता.',
'logempty'             => 'नोंदीत अशी बाब नाही.',
'log-title-wildcard'   => 'या मजकुरापासून सुरू होणारी शिर्षके शोधा.',

# Special:AllPages
'allpages'          => 'सर्व पृष्ठे',
'alphaindexline'    => '$1 पासून $2 पर्यंत',
'nextpage'          => 'पुढील पान ($1)',
'prevpage'          => 'मागील पान ($1)',
'allpagesfrom'      => 'पुढील शब्दाने सुरू होणारे लेख दाखवा:',
'allarticles'       => 'सगळे लेख',
'allinnamespace'    => 'सर्व पाने ($1 नामविश्व)',
'allnotinnamespace' => 'सर्व पाने ($1 नामविश्वात नसलेली)',
'allpagesprev'      => 'मागील',
'allpagesnext'      => 'पुढील',
'allpagessubmit'    => 'चला',
'allpagesprefix'    => 'पुढील शब्दाने सुरू होणारी पाने दाखवा:',
'allpagesbadtitle'  => 'दिलेले शीर्षक चुकीचे किंवा आंतरभाषीय किंवा आंतरविकि शब्दाने सुरू होणारे होते. त्यात एक किंवा अधिक शीर्षकात न वापरता येणारी अक्षरे असावीत.',
'allpages-bad-ns'   => '{{SITENAME}}मध्ये "$1" हे नामविश्व नाही.',

# Special:Categories
'categories'                    => 'वर्ग',
'categoriespagetext'            => 'विकिवर खालील वर्ग आहेत.',
'categoriesfrom'                => 'या शब्दापासून सुरू होणारे वर्ग दाखवा:',
'special-categories-sort-count' => 'क्रमानुसार लावा',
'special-categories-sort-abc'   => 'अक्षरांप्रमाणे लावा',

# Special:ListUsers
'listusersfrom'      => 'पुढील शब्दापासुन सुरू होणारे सदस्य दाखवा:',
'listusers-submit'   => 'दाखवा',
'listusers-noresult' => 'एकही सदस्य सापडला नाही.',

# Special:ListGroupRights
'listgrouprights'          => 'सदस्य गट अधिकार',
'listgrouprights-summary'  => 'खाली या विकिवर दिलेली सदस्य गटांची यादी त्यांच्या अधिकारांसकट दर्शविलेली आहे. प्रत्येकाच्या अधिकारांची अधिक माहिती [[{{MediaWiki:Listgrouprights-helppage}}|इथे]] दिलेली आहे.',
'listgrouprights-group'    => 'गट',
'listgrouprights-rights'   => 'अधिकार',
'listgrouprights-helppage' => 'Help:गट अधिकार',
'listgrouprights-members'  => '(सदस्यांची यादी)',

# E-mail user
'mailnologin'     => 'पाठविण्याचा पत्ता नाही',
'mailnologintext' => 'इतर सदस्यांना विपत्र(ईमेल) पाठवण्याकरिता तुम्ही [[Special:UserLogin|प्रवेश केलेला]] असणे आणि  शाबीत विपत्र पत्ता तुमच्या [[Special:Preferences|पसंतीत]] नमुद असणे आवश्यक आहे.',
'emailuser'       => 'या सदस्याला इमेल पाठवा',
'emailpage'       => 'विपत्र (ईमेल) उपयोगकर्ता',
'emailpagetext'   => 'जर या सदस्याने शाबीत विपत्र (ईमेल)पत्ता तीच्या अथवा त्याच्या सदस्य पसंतीत नमुद केला असेल,तर खालील सारणी तुम्हाला एक(च) संदेश पाठवेल.तुम्ही तुमच्या सदस्य पसंतीत नमुद केलेला विपत्र पत्ता "कडून" पत्त्यात येईल म्हणजे  प्राप्तकरता आपल्याला उत्तर देऊ शकेल.',
'usermailererror' => 'पत्र बाब त्रूटी वापस पाठवली:',
'defemailsubject' => '{{SITENAME}} विपत्र',
'noemailtitle'    => 'विपत्र पत्ता नाही',
'noemailtext'     => 'या सदस्याने शाबीत विपत्र पत्ता नमुद केलेला नाही, किंवा ’इतर सद्स्यांकडून विपत्र येऊ नये’ सोय निवडली आहे.',
'emailfrom'       => 'कडून',
'emailto'         => 'प्रति',
'emailsubject'    => 'विषय',
'emailmessage'    => 'संदेश',
'emailsend'       => 'पाठवा',
'emailccme'       => 'माझ्या संदेशाची मला विपत्र प्रत पाठवा.',
'emailccsubject'  => 'तुमच्या विपत्राची प्रत कडे $1: $2',
'emailsent'       => 'विपत्र पाठवले',
'emailsenttext'   => 'तुमचा विपत्र संदेश पाठवण्यात आला आहे.',

# Watchlist
'watchlist'            => 'माझी पहार्‍याची सूची',
'mywatchlist'          => 'माझी पहार्‍याची सूची',
'watchlistfor'         => "('''$1'''करिता)",
'nowatchlist'          => 'तुमची पहार्‍याची सूची रिकामी आहे.',
'watchlistanontext'    => 'तुमच्या पहार्‍याच्या सूचीतील बाबी पाहण्याकरता किंवा संपादित करण्याकरता, कृपया $1.',
'watchnologin'         => 'प्रवेश केलेला नाही',
'watchnologintext'     => 'तुमची पहार्‍याची सूची बदलावयाची असेल तर तुम्ही [[Special:UserLogin|प्रवेश केलेला]] असलाच पाहीजे.',
'addedwatch'           => 'हे पान पहार्‍याच्या सूचीत घातले.',
'addedwatchtext'       => '"[[:$1]]"  हे पान तुमच्या  [[Special:Watchlist|पहार्‍याच्या सूचीत]] टाकले आहे. या पानावरील तसेच त्याच्या चर्चा पानावरील पुढील बदल येथे दाखवले जातील, आणि   [[Special:RecentChanges|अलीकडील बदलांमध्ये]] पान ठळक दिसेल.

पहार्‍याच्या सूचीतून पान काढायचे असेल तर "पहारा काढा" वर टिचकी द्या.',
'removedwatch'         => 'पहार्‍याच्या सूचीतून वगळले',
'removedwatchtext'     => '"[[:$1]]" पान तुमच्या पहार्‍याच्या सूचीतून वगळण्यात आले आहे.',
'watch'                => 'पहारा',
'watchthispage'        => 'या पानावर बदलांसाठी लक्ष ठेवा.',
'unwatch'              => 'पहारा काढा',
'unwatchthispage'      => 'पहारा काढून टाका',
'notanarticle'         => 'मजकुर विरहीत पान',
'notvisiblerev'        => 'आवृत्ती वगळण्यात आलेली आहे',
'watchnochange'        => 'प्रदर्शित कालावाधीत, तुम्ही पहारा ठेवलेली कोणतीही बाब संपादीत झाली नाही.',
'watchlist-details'    => '{{PLURAL:$1|$1 पान|$1 पाने}} पहार्‍यात,चर्चा पाने मोजली नाही .',
'wlheader-enotif'      => '* विपत्र सूचना सुविधा ऊपलब्ध केली.',
'wlheader-showupdated' => "* तुम्ही पानांस दिलेल्या शेवटच्या भेटी पासून बदललेली पाने '''ठळक''' दाखवली आहेत.",
'watchmethod-recent'   => 'पहार्‍यातील पानांकरिता अलिकडील बदलांचा तपास',
'watchmethod-list'     => 'अलिकडील बदलांकरिता पहार्‍यातील पानांचा तपास',
'watchlistcontains'    => 'तुमचा $1 {{PLURAL:$1|पानावर|पानांवर}} पहारा आहे.',
'iteminvalidname'      => "'$1'बाबीस समस्या, अमान्य नाव...",
'wlnote'               => "खाली  गेल्या {{PLURAL:$2|तासातील|'''$2''' तासातील}} {{PLURAL:$1|शेवटचा बदल आहे|शेवटाचे '''$1'''बदल आहेत }}.",
'wlshowlast'           => 'मागील $1 तास $2 दिवस $3 पहा',
'watchlist-show-bots'  => 'सांगकाम्यांची संपादने पहा',
'watchlist-hide-bots'  => 'सांगकाम्यांची संपादने लपवा',
'watchlist-show-own'   => 'माझी संपादने पहा',
'watchlist-hide-own'   => 'माझी संपादने लपवा',
'watchlist-show-minor' => 'छोटी संपादने पहा',
'watchlist-hide-minor' => 'छोटी संपादने लपवा',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'पाहताहे...',
'unwatching' => 'पहारा काढत आहे...',

'enotif_mailer'                => '{{SITENAME}} सूचना विपत्र',
'enotif_reset'                 => 'सर्व पानास भेट दिल्याचे नमुद करा',
'enotif_newpagetext'           => 'हे नवीन पान आहे.',
'enotif_impersonal_salutation' => '{{SITENAME}} सदस्य',
'changed'                      => 'बदलला',
'created'                      => 'घडवले',
'enotif_subject'               => '{{SITENAME}} पान $पानशिर्षक $पानसंपादकाने $निर्मीले किंवा बदलले आहे',
'enotif_lastvisited'           => 'तुमच्या शेवटच्या भेटीनंतरचे बदल बघणयासाठी पहा - $1.',
'enotif_lastdiff'              => 'हा बदल पहाण्याकरिता $1 पहा.',
'enotif_anon_editor'           => 'अनामिक उपयोगकर्ता $1',
'enotif_body'                  => 'प्रिय $WATCHINGUSERNAME,


The {{SITENAME}}चे  $PAGETITLE पान $PAGEEDITORने $PAGEEDITDATE तारखेस $CHANGEDORCREATED केले आहे ,सध्याच्या आवृत्तीकरिता पहा $PAGETITLE_URL.

$NEWPAGE

संपादकाचा आढावा : $PAGESUMMARY $PAGEMINOREDIT

संपादकास संपर्क करा :
विपत्र: $PAGEEDITOR_EMAIL
विकि: $PAGEEDITOR_WIKI

तुम्ही पानास भेट देत नाही तोपर्यंत पुढे होणार्‍या बदलांची इतर कोणतीही वेगळी सूचना नसेल.तुम्ही पहार्‍याच्या सूचीतील पहारा ठेवलेल्या पानांकरिताच्या सूचना पताकांचे पुर्नयोजन करु शकता.

::::::तुमची मैत्रीपूर्ण {{SITENAME}} सुचना प्रणाली

--

तुमचे पहार्‍यातील पानांची मांडणावळ (कोंदण) बदलू शकता,{{fullurl:{{ns:special}}:Watchlist/edit}}ला भेट द्या

पुढील सहाय्य आणि प्रतिक्रीया:
{{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => 'पान वगळा',
'confirm'                     => 'निश्चीत',
'excontent'                   => "मजकूर होता: '$1'",
'excontentauthor'             => "मजकूर होता: '$1' (आणि फक्त '[[Special:Contributions/$2|$2]]' यांचे योगदान होते.)",
'exbeforeblank'               => "वगळण्यापूर्वीचा मजकूर पुढीलप्रमाणे: '$1'",
'exblank'                     => 'पान रिकामे होते',
'delete-confirm'              => '"$1" वगळा',
'delete-legend'               => 'वगळा',
'historywarning'              => 'सुचना: तुम्ही वगळत असलेल्या पानाला इतिहास आहे:',
'confirmdeletetext'           => 'तुम्ही एक लेख त्याच्या सर्व इतिहासासोबत वगळण्याच्या तयारीत आहात.
कृपया तुम्ही करत असलेली कृती ही मीडियाविकीच्या [[{{MediaWiki:Policy-url}}|नीतीनुसार]] आहे ह्याची खात्री करा. तसेच तुम्ही करीत असलेल्या कृतीचे परीणाम कृती करण्यापूर्वी जाणून घ्या.',
'actioncomplete'              => 'काम पूर्ण',
'deletedtext'                 => '"<nowiki>$1</nowiki>" हा लेख वगळला. अलीकडे वगळलेले लेख पाहण्यासाठी $2 पहा.',
'deletedarticle'              => '"[[$1]]" लेख वगळला.',
'suppressedarticle'           => '"[[$1]]" ला दाबले (सप्रेस)',
'dellogpage'                  => 'वगळल्याची नोंद',
'dellogpagetext'              => 'नुकत्याच वगळलेल्या पानांची यादी खाली आहे.',
'deletionlog'                 => 'वगळल्याची नोंद',
'reverted'                    => 'जुन्या आवृत्तीकडे पूर्वपदास नेले',
'deletecomment'               => 'वगळण्याचे कारण',
'deleteotherreason'           => 'दुसरे/अतिरिक्त कारण:',
'deletereasonotherlist'       => 'दुसरे कारण',
'deletereason-dropdown'       => '* वगळण्याची सामान्य कारणे
** लेखकाची(लेखिकेची) विनंती
** प्रताधिकार उल्लंघन
** उत्पात',
'delete-edit-reasonlist'      => 'वगळण्याची कारणे संपादीत करा',
'delete-toobig'               => 'या पानाला खूप मोठी इतिहास यादी आहे, तसेच हे पान $1 पेक्षा जास्त वेळा बदलण्यात आलेले आहे. अशी पाने वगळणे हे {{SITENAME}} ला धोकादायक ठरू नये म्हणून शक्य केलेले नाही.',
'delete-warning-toobig'       => '
या पानाला खूप मोठी इतिहास यादी आहे, तसेच हे पान $1 पेक्षा जास्त वेळा बदलण्यात आलेले आहे. अशी पाने वगळणे हे Betawiki ला धोकादायक ठरू शकते; कृपया काळजीपूर्वक हे पान वगळा.',
'rollback'                    => 'बदल वेगात माघारी न्या',
'rollback_short'              => 'द्रूतमाघार',
'rollbacklink'                => 'द्रूतमाघार',
'rollbackfailed'              => 'द्रूतमाघार फसली',
'cantrollback'                => 'जुन्या आवृत्तीकडे परतवता येत नाही; शेवटचा संपादक या पानाचा एकमात्र लेखक आहे.',
'alreadyrolled'               => 'Cannot rollback last edit of by [[User:$2|$2]] ([[User talk:$2|Talk]])चे शेवटाचे [[:$1]]वे संपादन माघारी परतवता येतनाही; पान आधीच कुणी माघारी परतवले आहे किंवा संपादीत केले आहे.

शेवटचे संपादन [[User:$3|$3]] ([[User talk:$3|Talk]])-चे होते.',
'editcomment'                 => 'बदलासोबतची नोंद होती : "<i>$1</i>".', # only shown if there is an edit comment
'revertpage'                  => '[[Special:Contributions/$2|$2]] ([[User talk:$2|चर्चा]]) यांनी केलेले बदल [[User:$1|$1]] यांच्या आवृत्तीकडे पूर्वपदास नेले.', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => '$1 ने उलटवलेली संपादने;$2 च्या आवृत्तीस परत नेली.',
'sessionfailure'              => 'तुमच्या दाखल सत्रात काही समस्या दिसते;सत्र अपहारणा पासून काळजी घेण्याच्या दृष्टीने ही कृती रद्द केली गेली आहे.कपया आपल्या विचरकाच्या "back" कळीवर टिचकी मारा आणि तुम्ही ज्या पानावरून आला ते पुन्हा चढवा,आणि प्रत प्रयत्न करा.',
'protectlogpage'              => 'सुरक्षा नोंदी',
'protectlogtext'              => 'पानांना लावलेल्या ताळ्यांची आणि ताळे उघडण्याबद्दलच्या पानाची खाली सूची दिली आहे.सध्याच्या सुरक्षीत पानांबद्दलच्या माहितीकरिता [[Special:ProtectedPages|सुरक्षीत पानांची सूची]] पहा.',
'protectedarticle'            => '"[[$1]]" सुरक्षित केला',
'modifiedarticleprotection'   => '"[[$1]]"करिता सुरक्षापातळी बदलली',
'unprotectedarticle'          => '"[[$1]]" असुरक्षित केला.',
'protect-title'               => '"$1" सुरक्षित करत आहे',
'protect-legend'              => 'सुरक्षापातळीतील बदल निर्धारित करा',
'protectcomment'              => 'सुरक्षित करण्यामागचे कारण',
'protectexpiry'               => 'संपण्याचा कालावधी:',
'protect_expiry_invalid'      => 'संपण्याचा कालावधी चुकीचा आहे.',
'protect_expiry_old'          => 'संपण्याचा कालावधी उलटून गेलेला आहे.',
'protect-unchain'             => 'स्थानांतरणाची परवानगी द्या',
'protect-text'                => '<strong><nowiki>$1</nowiki></strong> या पानाची सुरक्षापातळी तुम्ही इथे पाहू शकता अथवा बदलू शकता.',
'protect-locked-blocked'      => 'तुम्ही प्रतिबंधीत असताना सुरक्षा पातळी बदलू शकत नाही.येथे <strong>$1</strong> पानाकरिता सध्याची मांडणावळ आहे:',
'protect-locked-dblock'       => 'विदागारास ताळे लागलेले असताना सुरक्षा पातळी बदलता येत नाही.येथे <strong>$1</strong> पानाकरिता सध्याची मांडणावळ आहे:',
'protect-locked-access'       => 'तुम्हाला या पानाची सुरक्षा पातळी बदलण्याचे अधिकार नाहीत.
<strong>$1</strong> या पानाची सुरक्षा पातळी पुढीलप्रमाणे:',
'protect-cascadeon'           => 'हे पान सध्या सुरक्षित आहे कारण ते {{PLURAL:$1|या पानाच्या|या पानांच्या}} सुरक्षा शिडीवर आहे. तुम्ही या पानाची सुरक्षा पातळी बदलू शकता, पण त्याने सुरक्षाशिडी मध्ये बदल होणार नाहीत.',
'protect-default'             => '(मूळ)',
'protect-fallback'            => '"$1" परवानगीची गरज',
'protect-level-autoconfirmed' => 'अनामिक सदस्यांना ब्लॉक करा',
'protect-level-sysop'         => 'फक्त प्रबंधकांसाठी',
'protect-summary-cascade'     => 'शिडी',
'protect-expiring'            => '$1 (UTC) ला संपेल',
'protect-cascade'             => 'या पानात असलेली पाने सुरक्षित करा (सुरक्षा शिडी)',
'protect-cantedit'            => 'तुम्ही या पानाची सुरक्षा पातळी बदलू शकत नाही कारण तुम्हाला तसे करण्याची परवानगी नाही.',
'restriction-type'            => 'परवानगी:',
'restriction-level'           => 'सुरक्षापातळी:',
'minimum-size'                => 'किमान आकार',
'maximum-size'                => 'महत्तम आकार:',
'pagesize'                    => '(बाइट)',

# Restrictions (nouns)
'restriction-edit'   => 'संपादन',
'restriction-move'   => 'स्थानांतरण',
'restriction-create' => 'निर्मित करा',
'restriction-upload' => 'चढवा',

# Restriction levels
'restriction-level-sysop'         => 'पूर्ण सूरक्षीत',
'restriction-level-autoconfirmed' => 'अर्ध सुरक्षीत',
'restriction-level-all'           => 'कोणतीही पातळी',

# Undelete
'undelete'                     => 'वगळलेली पाने पहा',
'undeletepage'                 => 'वगळलेली पाने पहा आणि पुनर्स्थापित करा',
'undeletepagetitle'            => "'''खाली [[:$1]] च्या वगळलेल्या आवृत्त्या समाविष्ट केलेल्या आहेत'''.",
'viewdeletedpage'              => 'काढून टाकलेले लेख पहा',
'undeletepagetext'             => 'खालील पाने वगळली आहेत तरी सुद्धा विदागारात जतन आहेत आणि पुर्न्स्थापित करणे शक्य आहे. विदागारातील साठवण ठरावीक कालावधीने स्वच्छ करता येते.',
'undeleteextrahelp'            => "संपूर्ण पान पुनर्स्थापित करण्याकरिता,सारे रकाने रिकामे ठेवा आणि '''''पुनर्स्थापन'''''वर टिचकी मारा. निवडक पुनर्स्थापन करण्याकरिता, ज्या आवर्तनांचे पुनर्स्थापन करावयाचे त्यांचे रकाने निवडा , आणि '''''पुनर्स्थापन'''''वर टिचकी मारा. '''''पुनर्योजन ''''' वर टिचकी मारल्यास सारे रकाने आणि प्रतिक्रीया खिडकी रिकामी होईल.",
'undeleterevisions'            => '$1 {{PLURAL:$1|आवर्तन|आवर्तने}}विदागारात संचीत',
'undeletehistory'              => 'जर तुम्ही पान पुनर्स्थापित केले तर ,सारी आवर्तने इतिहासात पुनर्स्थापित होतील.
वगळल्या पासून त्याच नावाचे नवे पान तयार केले गेले असेले तर, पुनर्स्थापित आवर्तने पाठीमागील इतिहासात दिसतील. पुनर्स्थापना नंतर संचिकांच्या आवर्तनांवरील बंधने गळून पडतील याची नोंद घ्या.',
'undeleterevdel'               => 'पृष्ठ पानाचे आवर्तन अर्धवट वगळले जाणार असेल तर पुनर्स्थापनाची कृती केली जाणार नाही.
अशा प्रसंगी, तुम्ही अगदी अलिकडील वगळलेली आवर्तने अनचेक किंवा अनहाईड केलीच पाहिजे.',
'undeletehistorynoadmin'       => 'हे पान वगळले गेले आहे.वगळण्याचे कारण खालील आढाव्यात,वगळण्यापूर्वी संपादीत करणार्‍या संपादकांच्या माहिती सोबत,दाखवले आहे. वगळलेल्या आवर्त्नांचा नेमका मजकुर केवळ प्रचालकांना उपलब्ध असेल.',
'undelete-revision'            => '$1चे($2चे)आवर्तन $3 ने वगळले:',
'undeleterevision-missing'     => 'अयोग्य अथवा नसापडणारे आवर्तन. तुमचा दुवा कदाचित चूकीचा असेल, किंवा आवर्तन पुनर्स्थापित केले गेले असेल किंवा विदागारातून वगळले असेल.',
'undelete-nodiff'              => 'पूर्वीचे कोणतेही आवर्तन आढळले नाही.',
'undeletebtn'                  => 'वगळण्याची क्रिया रद्द करा',
'undeletelink'                 => 'पुनर्स्थापित करा',
'undeletereset'                => 'पूर्ववत',
'undeletecomment'              => 'प्रतिक्रीया:',
'undeletedarticle'             => '"[[$1]]" पुनर्स्थापित',
'undeletedrevisions'           => '{{PLURAL:$1|1 आवर्तन|$1 आवर्तने}} पुनर्स्थापित',
'undeletedrevisions-files'     => '{{PLURAL:$1|1 आवर्तन|$1 आवर्तने}}आणि {{PLURAL:$2|1 संचिका|$2 संचिका}} पुनर्स्थापित',
'undeletedfiles'               => '{{PLURAL:$1|1 संचिका|$1 संचिका}} पुनर्स्थापित',
'cannotundelete'               => 'वगळणे उलटवणे फसले; इतर कुणी तुमच्या आधी वगळणे उलटवले असु शकते.',
'undeletedpage'                => "<big>'''$1ला पुनर्स्थापित केले'''</big>

अलिकडिल वगळलेल्या आणि पुनर्स्थापितांच्या नोंदीकरिता [[Special:Log/delete|वगळल्याच्या नोंदी]] पहा .",
'undelete-header'              => 'अलिकडील वगळलेल्या पानांकरिता [[Special:Log/delete|वगळलेल्या नोंदी]] पहा.',
'undelete-search-box'          => 'वगळलेली पाने शोधा',
'undelete-search-prefix'       => 'पासून सूरू होणारी पाने दाखवा:',
'undelete-search-submit'       => 'शोध',
'undelete-no-results'          => 'वगळलेल्यांच्या विदांमध्ये जुळणारी पाने सापडली नाहीत.',
'undelete-filename-mismatch'   => '$1 वेळेचे, वगळलेल्या संचिकेचे आवर्तन उलटवता येत नाही: नजुळणारे संचिकानाव',
'undelete-bad-store-key'       => '$1 वेळ दिलेली संचिका आवर्तन पुनर्स्थापित करता येत नाही:संचिका वगळण्यापूर्वी पासून मिळाली नव्हती.',
'undelete-cleanup-error'       => 'न वापरलेली विदा संचिका "$1" वगळताना त्रूटी दाखवते.',
'undelete-missing-filearchive' => 'संचिका विदास्मृती ID $1 पुनर्स्थापित करू शकत नाही कारण ती विदागारात उपलब्ध नाही. ती आधीच पुनर्स्थापित केली असण्याची शक्यता सुद्धा असू शकते.',
'undelete-error-short'         => 'संचिकेचे वगळणे उलटवताना त्रूटी: $1',
'undelete-error-long'          => 'संचिकेचे वगळणे उलटवताना त्रूटींचा अडथळा आला:

$1',

# Namespace form on various pages
'namespace'      => 'नामविश्व:',
'invert'         => 'निवडीचा क्रम उलटा करा',
'blanknamespace' => '(मुख्य)',

# Contributions
'contributions' => 'सदस्याचे योगदान',
'mycontris'     => 'माझे योगदान',
'contribsub2'   => '$1 ($2) साठी',
'nocontribs'    => 'या मानदंडाशी जुळणारे बदल सापडले नाहीत.',
'uctop'         => ' (वर)',
'month'         => 'या महिन्यापासून (आणि पूर्वीचे):',
'year'          => 'या वर्षापासून (आणि पूर्वीचे):',

'sp-contributions-newbies'     => 'केवळ नवीन सदस्य खात्यांचे योगदान दाखवा',
'sp-contributions-newbies-sub' => 'नवशिक्यांसाठी',
'sp-contributions-blocklog'    => 'ब्लॉक यादी',
'sp-contributions-search'      => 'योगदान शोधयंत्र',
'sp-contributions-username'    => 'आंतरजाल अंकपत्ता किंवा सदस्यनाम:',
'sp-contributions-submit'      => 'शोध',

# What links here
'whatlinkshere'            => 'येथे काय जोडले आहे',
'whatlinkshere-title'      => '"$1" ला जोडलेली पाने',
'whatlinkshere-page'       => 'पान:',
'linklistsub'              => '(दुव्यांची यादी)',
'linkshere'                => "खालील लेख '''[[:$1]]''' या निर्देशित पानाशी जोडले आहेत:",
'nolinkshere'              => "'''[[:$1]]''' इथे काहीही जोडलेले नाही.",
'nolinkshere-ns'           => "निवडलेल्या नामविश्वातील कोणतीही पाने '''[[:$1]]'''ला दुवा देत नाहीत .",
'isredirect'               => 'पुनर्निर्देशित पान',
'istemplate'               => 'मिळवा',
'isimage'                  => 'चित्र दुवा',
'whatlinkshere-prev'       => '{{PLURAL:$1|पूर्वीचा|पूर्वीचे $1}}',
'whatlinkshere-next'       => '{{PLURAL:$1|पुढील|पुढील $1}}',
'whatlinkshere-links'      => '← दुवे',
'whatlinkshere-hideredirs' => '$1 पुनर्निर्देशने',
'whatlinkshere-hidetrans'  => '$1 ट्रान्स्क्ल्युजन्स',
'whatlinkshere-hidelinks'  => '$1 दुवे',
'whatlinkshere-hideimages' => '$1 चित्र दुवे',
'whatlinkshere-filters'    => 'फिल्टर्स',

# Block/unblock
'blockip'                     => 'हा अंकपत्ता अडवा',
'blockip-legend'              => 'सदस्यास प्रतिबंध करा',
'blockiptext'                 => 'एखाद्या विशिष्ट अंकपत्त्याची किंवा सदस्याची लिहिण्याची क्षमता प्रतिबंधीत  करण्याकरिता खालील सारणी वापरा.
हे केवळ उच्छेद टाळण्याच्याच दृष्टीने आणि [[{{MediaWiki:Policy-url}}|निती]]स अनुसरून केले पाहिजे.
खाली विशिष्ट कारण भरा(उदाहरणार्थ,ज्या पानांवर उच्छेद माजवला गेला त्यांची उद्धरणे देऊन).',
'ipaddress'                   => 'अंकपत्ता',
'ipadressorusername'          => 'अंकपत्ता किंवा सदस्यनाम:',
'ipbexpiry'                   => 'समाप्ति:',
'ipbreason'                   => 'कारण',
'ipbreasonotherlist'          => 'इतर कारण',
'ipbreason-dropdown'          => '*प्रतिबंधनाची सामान्य कारणे
** चुकीची माहिती भरणे
** पानांवरील मजकूर काढणे
** बाह्यसंकेतस्थळाचे चिखलणी(स्पॅमींग) दुवे देणे
** पानात अटरफटर/वेडगळ भरणे
** धमकावणारे/उपद्रवी वर्तन
** असंख्य खात्यांचा गैरवापर
** अस्वीकार्य सदस्यनाम',
'ipbanononly'                 => 'केवळ अनामिक सदस्यांना प्रतिबंधीत करा',
'ipbcreateaccount'            => 'खात्याची निर्मिती प्रतिबंधीत करा',
'ipbemailban'                 => 'सदस्यांना विपत्र पाठवण्यापासून प्रतिबंधीत करा',
'ipbenableautoblock'          => 'या सदस्याने वापरलेला शेवटचा अंकपत्ता आणि जेथून या पुढे तो संपादनाचा प्रयत्न करेल ते सर्व अंकपत्ते आपोआप प्रतिबंधीत करा.',
'ipbsubmit'                   => 'हा पत्ता अडवा',
'ipbother'                    => 'इतर वेळ:',
'ipboptions'                  => '२ तास:2 hours,१ दिवस:1 day,३ दिवस:3 days,१ आठवडा:1 week,२ आठवडे:2 weeks,१ महिना:1 month,३ महिने:3 months,६ महिने:6 months,१ वर्ष:1 year,अनंत:infinite', # display1:time1,display2:time2,...
'ipbotheroption'              => 'इतर',
'ipbotherreason'              => 'इतर/अजून कारण:',
'ipbhidename'                 => 'सदस्य नाम प्रतिबंधन नोंदी, प्रतिबंधनाची चालू यादी आणि सदस्य यादी इत्यादीतून लपवा',
'ipbwatchuser'                => 'या सदस्याच्या सदस्य तसेच चर्चा पानावर पहारा ठेवा',
'badipaddress'                => 'अंकपत्ता बरोबर नाही.',
'blockipsuccesssub'           => 'अडवणूक यशस्वी झाली',
'blockipsuccesstext'          => '[[Special:Contributions/$1|$1]]ला प्रतिबंधीत केले.<br />
प्रतिबंधनांचा आढावा घेण्याकरिता [[Special:IPBlockList|अंकपत्ता प्रतिबंधन सूची]] पहा.',
'ipb-edit-dropdown'           => 'प्रतिबंधाची कारणे संपादा',
'ipb-unblock-addr'            => '$1चा प्रतिबंध उठवा',
'ipb-unblock'                 => 'सदस्यनाव आणि अंकपत्त्यावरचे प्रतिबंधन उठवा',
'ipb-blocklist-addr'          => '$1करिता सध्याचे प्रतिबंध पहा',
'ipb-blocklist'               => 'सध्याचे प्रतिबंध पहा',
'unblockip'                   => 'अंकपत्ता सोडवा',
'unblockiptext'               => 'खाली दिलेला फॉर्म वापरून पूर्वी अडवलेल्या अंकपत्त्याला लेखनासाठी आधिकार द्या.',
'ipusubmit'                   => 'हा पत्ता सोडवा',
'unblocked'                   => '[[User:$1|$1]] वरचे प्रतिबंध उठवले आहेत',
'unblocked-id'                => 'प्रतिबंध $1 काढले',
'ipblocklist'                 => 'अडविलेले अंकपत्ते व सदस्य नावे',
'ipblocklist-legend'          => 'प्रतिबंधीत सदस्य शोधा',
'ipblocklist-username'        => 'सदस्यनाव किंवा आंतरजाल अंकपत्ता:',
'ipblocklist-submit'          => 'शोध',
'blocklistline'               => '$3 ($4)ला $1, $2 ने प्रतिबंधीत केले',
'infiniteblock'               => 'अनंत',
'expiringblock'               => 'समाप्ति $1',
'anononlyblock'               => 'केवळ अनामिक',
'noautoblockblock'            => 'स्व्यंचलितप्रतिबंधन स्थगित केले',
'createaccountblock'          => 'खात्याची निर्मिती प्रतिबंधीत केली',
'emailblock'                  => 'विपत्र प्रतिबंधीत',
'ipblocklist-empty'           => 'प्रतिबंधन यादी रिकामी आहे.',
'ipblocklist-no-results'      => 'विनंती केलेला अंकपत्ता अथवा सदस्यनाव प्रतिबंधीत केलेले नाही.',
'blocklink'                   => 'अडवा',
'unblocklink'                 => 'सोडवा',
'contribslink'                => 'योगदान',
'autoblocker'                 => 'स्वयंचलितप्रतिबंधन केले गेले कारण तुमचा अंकपत्ता अलीकडे "[[User:$1|$1]]"ने वापरला होता. $1 च्या प्रतिबंधनाकरिता दिलेले कारण: "$2" आहे.',
'blocklogpage'                => 'ब्लॉक यादी',
'blocklogentry'               => '[[$1]] ला $2 $3 पर्यंत ब्लॉक केलेले आहे',
'blocklogtext'                => 'ही सदस्यांच्या प्रतिबंधनाची आणि प्रतिबंधने उठवल्याची नोंद आहे.
आपोआप प्रतिबंधीत केलेले अंकपत्ते नमूद केलेले नाहीत.
सध्या लागू असलेली बंदी व प्रतिबंधनांच्या यादीकरिता [[Special:IPBlockList|अंकपत्ता प्रतिबंधन सूची]] पहा.',
'unblocklogentry'             => 'प्रतिबंधन $1 हटवले',
'block-log-flags-anononly'    => 'केवळ अनामिक सदस्य',
'block-log-flags-nocreate'    => 'खाते तयारकरणे अवरूद्ध केले',
'block-log-flags-noautoblock' => 'स्वयंचलित प्रतिबंधन अवरूद्ध केले',
'block-log-flags-noemail'     => 'विपत्र अवरूद्ध केले',
'range_block_disabled'        => 'प्रचालकांची पल्ला बंधने घालण्याची क्षमता अनुपलब्ध केली आहे.',
'ipb_expiry_invalid'          => 'अयोग्य समाप्ती काळ.',
'ipb_expiry_temp'             => 'लपविलेले सदस्यनाम प्रतिबंधन कायमस्वरुपी असले पाहिजे.',
'ipb_already_blocked'         => '"$1" आधीच अवरूद्ध केलेले आहे.',
'ipb_cant_unblock'            => 'त्रूटी: प्रतिबंधन क्र.$1 मिळाला नाही. त्यावरील प्रतिबंधन कदाचित आधीच उठवले असेल.',
'ipb_blocked_as_range'        => 'त्रूटी:अंकपत्ता IP $1 हा प्रत्यक्षपणे प्रतिबंधीत केलेला नाही आणि अप्रतिबंधीत करता येत नाही.तो,अर्थात,$2पल्ल्याचा भाग म्हाणून तो प्रतिबंधीत केलेला आहे,जो की अप्रतिबंधीत करता येत नाही.',
'ip_range_invalid'            => 'अंकपत्ता अयोग्य टप्प्यात.',
'blockme'                     => 'मला प्रतिबंधीत करा',
'proxyblocker'                => 'प्रातिनिधी(प्रॉक्झी)प्रतिबंधक',
'proxyblocker-disabled'       => 'हे कार्य अवरूद्ध केले आहे.',
'proxyblockreason'            => 'तुमचा अंकपत्ता प्रतिबंधीत केला आहे कारण तो उघड-उघड प्रतिनिधी आहे.कृपया तुमच्या आंतरजाल सेवा दात्यास किंवा तंत्रज्ञास पाचारण संपर्क करा आणि त्यांचे या गंभीर सुरक्षाप्रश्ना कडे लक्ष वेधा.',
'proxyblocksuccess'           => 'झाले.',
'sorbsreason'                 => '{{SITENAME}}ने वापरलेल्या DNSBL मध्ये तुमच्या अंकपत्त्याची नोंद उघड-उघड प्रतिनिधी म्हणून सूचित केली आहे.',
'sorbs_create_account_reason' => '{{SITENAME}}च्या DNSBLने तुमचा अंकपत्ता उघड-उघड प्रतिनिधी म्हणून सूचित केला आहे.तुम्ही खाते उघडू शकत नाही',

# Developer tools
'lockdb'              => 'विदागारास ताळे ठोका',
'unlockdb'            => 'विदागाराचे ताळे उघडा',
'lockdbtext'          => 'विदागारास ताळे ठोकल्याने सर्व सदस्यांची संपादन क्षमता, त्यांच्या सदस्य पसंती बदलणे,त्यांच्या पहार्‍याच्या सूची संपादीत करणे,आणि विदेत बदल घडवणार्‍या इतर गोष्टी संस्थगीत होतील.
कृपया तुम्हाला हेच करावयाचे आहे आणि भरण-पोषणा नंतर विदागाराचे ताळे उघडावयाचे आहे हे निश्चित करा.',
'unlockdbtext'        => 'विदागाराचे ताळे उघडल्याने सर्व सदस्यांची संपादन क्षमता, त्यांच्या सदस्य पसंती बदलणे,त्यांच्या पहार्‍याच्या सूची संपादीत करणे,आणि विदेत बदल घडवणार्‍या इतर गोष्टीची क्षमता पुन्हा उपलब्ध होईल.
कृपया तुम्हाला हेच करावयाचे आहे हे निश्चित करा.',
'lockconfirm'         => 'होय,मला खरेच विदागारास ताळे ठोकायच आहे.',
'unlockconfirm'       => 'होय,मला खरेच विदागाराचे ताळे उघडवयाचे आहे.',
'lockbtn'             => 'विदागारास ताळे ठोका',
'unlockbtn'           => 'विदागारचे ताळे काढा',
'locknoconfirm'       => 'आपण होकार पेटीत होकार भरला नाही.',
'lockdbsuccesssub'    => 'विदागरास ताळे यशस्वी',
'unlockdbsuccesssub'  => 'विदागाराचे ताळे काढले',
'lockdbsuccesstext'   => 'विदागारास ताळे ठोकण्यात आले आहे.<br />
तुमच्याकडून भरण-पोषण पूर्ण झाल्या नंतर [[Special:UnlockDB|ताळे उघडण्याचे]] लक्षात ठेवा.',
'unlockdbsuccesstext' => 'विदागाराचे ताळे उघडण्यात आले आहे.',
'lockfilenotwritable' => 'विदा ताळे संचिका लेखनीय नाही.विदेस ताळे लावण्याकरिता किंवा उघडण्याकरिता, ती आंतरजाल विदादात्याकडून लेखनीय असावयास हवी.',
'databasenotlocked'   => 'विदागारास ताळे नही',

# Move page
'move-page'               => '$1 हलवा',
'move-page-legend'        => 'पृष्ठ स्थानांतरण',
'movepagetext'            => "खालील अर्ज हा एखाद्या लेखाचे शीर्षक बदलण्यासाठी वापरता येईल. खालील अर्ज भरल्यानंतर लेखाचे शीर्षक बदलले जाईल तसेच त्या लेखाचा सर्व इतिहास हा नवीन लेखामध्ये स्थानांतरित केला जाईल.
जुने शीर्षक नवीन शीर्षकाला पुनर्निर्देशित करेल.
जुन्या शीर्षकाला असलेले दुवे बदलले जाणार नाहीत, तरी तुम्हाला विनंती आहे की स्थानांतरण केल्यानंतर 
[[Special:DoubleRedirects|दुहेरी]] अथवा [[Special:BrokenRedirects|मोडकी]] पुनर्निर्देशने तपासावीत.
चुकीचे दुवे टाळण्याची जबाबदारी सर्वस्वी तुमच्यावर राहील.

जर नवीन शीर्षकाचा लेख अस्तित्वात असेल तर स्थानांतरण होणार '''नाही'''.
पण जर नवीन शीर्षकाचा लेख हा रिकामा असेल अथवा पुनर्निर्देशन असेल (म्हणजेच त्या लेखाला जर संपादन इतिहास नसेल) तर स्थानांतरण होईल. याचा अर्थ असा की जर काही चूक झाली तर तुम्ही पुन्हा जुन्या शीर्षकाकडे स्थानांतरण करू शकता.

'''सूचना!'''
स्थानांतरण केल्याने एखाद्या महत्वाच्या लेखामध्ये अनपेक्षित बदल होऊ शकतात. तुम्हाला विनंती आहे की तुम्ही पूर्ण काळजी घ्या व होणारे परिणाम समजावून घ्या.
जर तुम्हाला शंका असेल तर प्रबंधकांशी संपर्क करा.",
'movepagetalktext'        => "संबंधित चर्चा पृष्ठ याबरोबर स्थानांतरीत होणार नाही '''जर:'''
* तुम्ही पृष्ठ दुसर्‍या नामविश्वात स्थानांतरीत करत असाल
* या नावाचे चर्चा पान अगोदरच अस्तित्वात असेल तर, किंवा
* खालील चेकबॉक्स तुम्ही काढून टाकला तर.

या बाबतीत तुम्हाला स्वतःला ही पाने एकत्र करावी लागतील.",
'movearticle'             => 'पृष्ठाचे स्थानांतरण',
'movenotallowed'          => '{{SITENAME}}वरील पाने स्थानांतरीत करण्याची आपल्यापाशी परवानगी नाही.',
'newtitle'                => 'नवीन शीर्षकाकडे:',
'move-watch'              => 'या पानावर लक्ष ठेवा',
'movepagebtn'             => 'स्थानांतरण करा',
'pagemovedsub'            => 'स्थानांतरण यशस्वी',
'movepage-moved'          => '<big>\'\'\'"$1" ला "$2" मथळ्याखाली स्थानांतरीत केले\'\'\'</big>', # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => 'त्या नावाचे पृष्ठ अगोदरच अस्तित्वात आहे, किंवा तुम्ही निवडलेले
नाव योग्य नाही आहे.
कृपया दुसरे नाव शोधा.',
'cantmove-titleprotected' => 'नवे शीर्षक निर्मीत करण्या पासून सुरक्षीत केलेले असल्यामुळे,तुम्ही या जागी एखादे पान स्थानांतरीत करू शकत नाही.',
'talkexists'              => 'पृष्ठ यशस्वीरीत्या स्थानांतरीत झाले पण चर्चा पृष्ठ स्थानांतरीत होवू
शकले नाही कारण त्या नावाचे पृष्ठ आधीच अस्तित्वात होते. कृपया तुम्ही स्वतः ती पृष्ठे एकत्र करा.',
'movedto'                 => 'कडे स्थानांतरण केले',
'movetalk'                => 'शक्य असल्यास "चर्चा पृष्ठ" स्थानांतरीत करा',
'move-subpages'           => 'जर लागू असेल तर, सर्व उपपाने स्थानांतरीत करा',
'move-talk-subpages'      => 'जर लागू असेल तर चर्चा पानाची सर्व उपपाने स्थानांतरीत करा',
'movepage-page-exists'    => '$1 पान अगोदरच अस्तित्त्वात आहे व त्याच्यावर आपोआप पुनर्लेखन करता येणार नाही.',
'movepage-page-moved'     => '$1 हे पान $2 या मथळ्याखाली स्थानांतरीत केले.',
'movepage-page-unmoved'   => '$1 हे पान $2 या मथळ्याखाली स्थानांतरीत करता आलेले नाही.',
'movepage-max-pages'      => 'जास्तीत जास्त $1 {{PLURAL:$1|पान|पाने}} स्थानांतरीत करण्यात {{PLURAL:$1|आलेले आहे|आलेली आहेत}} व आता आणखी पाने आपोआप स्थानांतरीत होणार नाहीत.',
'1movedto2'               => '"[[$1]]" हे पान "[[$2]]" मथळ्याखाली स्थानांतरित केले.',
'1movedto2_redir'         => '[[$1]] हे पान [[$2]] मथळ्याखाली स्थानांतरित केले (पुनर्निर्देशन).',
'movelogpage'             => 'स्थांनांतराची नोंद',
'movelogpagetext'         => 'स्थानांतरित केलेल्या पानांची यादी.',
'movereason'              => 'कारण:',
'revertmove'              => 'पूर्वपदास न्या',
'delete_and_move'         => 'वगळा आणि स्थानांतरित करा',
'delete_and_move_text'    => '==वगळण्याची आवशकता==

लक्ष्यपान  "[[:$1]]" आधीच अस्तीत्वात आहे.स्थानांतराचा मार्ग मोकळाकरण्या करिता तुम्हाला ते वगळावयाचे आहे काय?',
'delete_and_move_confirm' => 'होय, पान वगळा',
'delete_and_move_reason'  => 'आधीचे पान वगळून स्थानांतर केले',
'selfmove'                => 'स्रोत आणि लक्ष्य पाने समान आहेत; एखादे पान स्वत:च्याच जागी स्थानांतरीत करता येत नाही.',
'immobile_namespace'      => 'स्रोत किंवा लक्ष्य शीर्षक विशेष प्रकारचे आहे;त्या नामविशवात किंवा त्यातून बाहेर पानांचे स्थानांतरण करता येत नाही.',
'imagenocrossnamespace'   => 'ज्या नामविश्वात संचिका साठविता येत नाहीत, त्या नामविश्वात संचिकांचे स्थानांतरण करता येत नाही',
'imagetypemismatch'       => 'दिलेले संचिकेचे एक्सटेंशन त्या संचिकेच्या प्रकाराशी जुळत नाही',

# Export
'export'            => 'पाने निर्यात करा',
'exporttext'        => 'तुम्ही एखाद्या विशिष्ट पानाचा मजकुर आणि संपादन इतिहास किंवा  पानांचा संच एखाद्या XML वेष्ठणात ठेवून निर्यात करू शकता.हे तुम्हाला [[Special:Import|पान आयात करा]]वापरून मिडीयाविकि वापरणार्‍या इतर विकित आयात करता येईल.

पाने निर्यात करण्या करिता,एका ओळीत एक मथळा असे, खालील मजकुर रकान्यात मथळे भरा आणि तुम्हाला ’सध्याची आवृत्ती तसेच सर्व जुन्या आवृत्ती ,पानाच्या इतिहास ओळी सोबत’, किंवा ’केवळ सध्याची आवृत्ती शेवटच्या संपादनाच्या माहिती सोबत’ हवी आहे का ते निवडा.

तुम्ही नंतरच्या बाबतीत एखादा दुवा सुद्धा वापरू शकता, उदाहरणार्थ "[[{{MediaWiki:Mainpage}}]]" पाना करिता [[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]] .',
'exportcuronly'     => 'संपूर्ण इतिहास नको,केवळ आताचे आवर्तन आंर्तभूत करा',
'exportnohistory'   => "----
'''सूचना:''' या फॉर्मचा वापर करून पानाचा पूर्ण इतिहास निर्यात करण्याची सुविधा कार्यकुशलतेच्या कारणंनी अनुपल्ब्ढ ठेवली आहे.",
'export-submit'     => 'निर्यात करा',
'export-addcattext' => 'वर्गीकरणातून पाने भरा:',
'export-addcat'     => 'भर',
'export-download'   => 'संचिका म्हणून जतन करा',
'export-templates'  => 'साचे आंतरभूत करा',

# Namespace 8 related
'allmessages'               => 'सर्व प्रणाली-संदेश',
'allmessagesname'           => 'नाव',
'allmessagesdefault'        => 'सुरुवातीचा मजकूर',
'allmessagescurrent'        => 'सध्याचा मजकूर',
'allmessagestext'           => 'MediaWiki नामविश्वातील सर्व प्रणाली संदेशांची यादी',
'allmessagesnotsupportedDB' => "हे पान संपादित करता येत नाही कारण'''\$wgUseDatabaseMessages''' मालवला आहे.",
'allmessagesfilter'         => 'संदेशनावांची चाळणी:',
'allmessagesmodified'       => 'फक्त बदललेले दाखवा',

# Thumbnails
'thumbnail-more'           => 'मोठे करा',
'filemissing'              => 'संचिका अस्तित्वात नाही',
'thumbnail_error'          => 'इवलेसे चित्र बनविण्यात अडथळा आलेला आहे: $1',
'djvu_page_error'          => 'टप्प्याच्या बाहेरचे DjVu पान',
'djvu_no_xml'              => 'DjVu संचिकेकरिता XML ओढण्यात असमर्थ',
'thumbnail_invalid_params' => 'इवल्याशाचित्राचा अयोग्य परिचय',
'thumbnail_dest_directory' => 'लक्ष्य धारिकेच्या निर्मितीस असमर्थ',

# Special:Import
'import'                     => 'पाने आयात करा',
'importinterwiki'            => 'आंतरविकि आयात',
'import-interwiki-text'      => 'आयात करण्याकरिता एक विकि आणि पानाचा मथळा निवडा.
आवर्तनांच्या तारखा आणि संपादकांची नावे जतन केली जातील.
सर्व आंतरविकि आयात क्रिया [[Special:Log/import|आयात नोंदीत]] दाखल केल्या आहेत.',
'import-interwiki-history'   => 'या पानाकरिताची सार्‍या इतिहास आवर्तनांची नक्कल करा',
'import-interwiki-submit'    => 'आयात',
'import-interwiki-namespace' => 'पाने नामविश्वात स्थानांतरीत करा:',
'importtext'                 => 'कृपया Special:Export सुविधा वापरून स्रोत विकिकडून संचिका निर्यात करा,ती तुमच्या तबकडीवर जतन करा आणि येथे चढवा.',
'importstart'                => 'पाने आयात करत आहे...',
'import-revision-count'      => '$1 {{PLURAL:$1|आवर्तन|आवर्तने}}',
'importnopages'              => 'आयातीकरिता पाने नाहीत.',
'importfailed'               => 'अयशस्वी आयात: $1',
'importunknownsource'        => 'आयात स्रोत प्रकार अज्ञात',
'importcantopen'             => 'आयातीत संचिका उघडणे जमले नाही',
'importbadinterwiki'         => 'अयोग्य आंतरविकि दुवा',
'importnotext'               => 'रिकामे अथवा मजकुर नाही',
'importsuccess'              => 'आयात पूर्ण झाली!',
'importhistoryconflict'      => 'उपलब्ध इतिहास आवर्तने परस्पर विरोधी आहेत(हे पान पूर्वी आयात केले असण्याची शक्यता आहे)',
'importnosources'            => 'कोणतेही आंतरविकि आयात स्रोत व्यक्त केलेले नाहीत आणि प्रत्यक्ष इतिहास चढवा अनुपलब्ध केले आहे.',
'importnofile'               => 'कोणतीही आयातीत संचिका चढवलेली नाही.',
'importuploaderrorsize'      => 'आयात संचिकेचे चढवणे फसले.संचिका चढवण्याच्या मान्यताप्राप्त आकारा पेक्षा मोठी आहे.',
'importuploaderrorpartial'   => 'आयात संचिकेचे चढवणे फसले.संचिका केवळ अर्धवटच चढू शकली.',
'importuploaderrortemp'      => 'आयात संचिकेचे चढवणे फसले.एक तात्पुरती धारिका मिळत नाही.',
'import-parse-failure'       => 'XML आयात पृथक्करण अयशस्वी',
'import-noarticle'           => 'आयात करण्याकरिता पान नाही!',
'import-nonewrevisions'      => 'सारी आवर्तने पूर्वी आयात केली होती.',
'xml-error-string'           => '$1 ओळ $2मध्ये , स्तंभ $3 (बाईट $4): $5',
'import-upload'              => 'XML डाटा चढवा',

# Import log
'importlogpage'                    => 'ईम्पोर्ट सूची',
'importlogpagetext'                => 'इतर विकिक्डून पानांची, संपादकीय इतिहासासहीत, प्रबंधकीय आयात.',
'import-logentry-upload'           => 'संचिका चढवल्याने [[$1]] आयात',
'import-logentry-upload-detail'    => '$1 {{PLURAL:$1|आवर्तन|आवर्तने}}',
'import-logentry-interwiki'        => 'आंतरविकिकरण $1',
'import-logentry-interwiki-detail' => '$2 पासून $1 {{PLURAL:$1|आवर्तन|आवर्तने}}',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'माझे सदस्य पान',
'tooltip-pt-anonuserpage'         => 'तुम्ही ज्या अंकपत्त्यान्वये संपादीत करत आहात त्याकरिता हे सदस्य पान',
'tooltip-pt-mytalk'               => 'माझे चर्चा पान',
'tooltip-pt-anontalk'             => 'या अंकपत्त्यापासून झालेल्या संपादनांबद्दल चर्चा',
'tooltip-pt-preferences'          => 'माझ्या पसंती',
'tooltip-pt-watchlist'            => 'तुम्ही पहारा दिलेल्या पानांची यादी',
'tooltip-pt-mycontris'            => 'माझ्या योगदानांची यादी',
'tooltip-pt-login'                => 'आपणांस सदस्यत्व घेण्याची विनंती करण्यात येत आहे. सदस्यत्व घेणे अनिवार्य नाही.',
'tooltip-pt-anonlogin'            => 'आपण खात्यात दाखल व्हावे या करिता प्रोत्साहन देतो, अर्थात ते अत्यावश्यक नाही.',
'tooltip-pt-logout'               => 'बाहेर पडा',
'tooltip-ca-talk'                 => 'कंटेंट पानाबद्दलच्या चर्चा',
'tooltip-ca-edit'                 => 'तुम्ही हे पान बद्लू शकता. कृपया जतन करण्यापुर्वी झलक कळ वापरून पहा.',
'tooltip-ca-addsection'           => 'या चर्चेमध्ये मत नोंदवा.',
'tooltip-ca-viewsource'           => 'हे पान सुरक्षित आहे. तुम्ही याचा स्रोत पाहू शकता.',
'tooltip-ca-history'              => 'या पानाच्या जुन्या आवृत्या.',
'tooltip-ca-protect'              => 'हे पान सुरक्षित करा',
'tooltip-ca-delete'               => 'हे पान वगळा',
'tooltip-ca-undelete'             => 'या पानाची वगळण्यापूर्वी केलेली संपादने पुनर्स्थापित करा',
'tooltip-ca-move'                 => 'हे पान स्थानांतरित करा.',
'tooltip-ca-watch'                => 'हे पान तुमच्या पहार्‍याची सूचीत टाका',
'tooltip-ca-unwatch'              => 'हे पान पहार्‍याच्या सूचीतून काढा.',
'tooltip-search'                  => '{{SITENAME}} शोधा',
'tooltip-search-go'               => 'या नेमक्या नावाच्या पानाकडे,अस्तित्वात असल्यास, चला',
'tooltip-search-fulltext'         => 'या मजकुराकरिता पान शोधा',
'tooltip-p-logo'                  => 'मुखपृष्ठ',
'tooltip-n-mainpage'              => 'मुखपृष्ठाला भेट द्या',
'tooltip-n-portal'                => 'प्रकल्पाबद्दल, तुम्ही काय करू शकता, कुठे काय सापडेल',
'tooltip-n-currentevents'         => 'सद्य घटनांबद्दलची माहिती',
'tooltip-n-recentchanges'         => 'विकिवरील अलीकडील बदलांची यादी',
'tooltip-n-randompage'            => 'कोणतेही पान पहा',
'tooltip-n-help'                  => 'मदत मिळवण्याचे ठिकाण',
'tooltip-t-whatlinkshere'         => 'येथे जोडलेल्या सर्व विकिपानांची यादी',
'tooltip-t-recentchangeslinked'   => 'येथुन जोडलेल्या सर्व पानांवरील अलीकडील बदल',
'tooltip-feed-rss'                => 'या पानाकरिता आर.एस.एस. रसद',
'tooltip-feed-atom'               => 'या पानाकरिता ऍटम रसद',
'tooltip-t-contributions'         => 'या सदस्याच्या योगदानांची यादी पहा',
'tooltip-t-emailuser'             => 'या सदस्याला इमेल पाठवा',
'tooltip-t-upload'                => 'चित्रे किंवा माध्यम संचिका चढवा',
'tooltip-t-specialpages'          => 'सर्व विशेष पृष्ठांची यादी',
'tooltip-t-print'                 => 'या पानाची छापण्यायोग्य आवृत्ती',
'tooltip-t-permalink'             => 'पानाच्या या आवर्तनाचा शाश्वत दुवा',
'tooltip-ca-nstab-main'           => 'मजकुराचे पान पहा',
'tooltip-ca-nstab-user'           => 'सदस्य पान पहा',
'tooltip-ca-nstab-media'          => 'माध्यम पान पहा',
'tooltip-ca-nstab-special'        => 'हे विशेष पान आहे; तुम्ही ते बदलू शकत नाही.',
'tooltip-ca-nstab-project'        => 'प्रकल्प पान पहा',
'tooltip-ca-nstab-image'          => 'चित्र पान पहा',
'tooltip-ca-nstab-mediawiki'      => 'सिस्टीम संदेश पहा',
'tooltip-ca-nstab-template'       => 'साचा पहा',
'tooltip-ca-nstab-help'           => 'साहाय्य पान पहा',
'tooltip-ca-nstab-category'       => 'वर्ग पान पहा',
'tooltip-minoredit'               => 'बदल छोटा असल्याची नोंद करा',
'tooltip-save'                    => 'तुम्ही केलेले बदल जतन करा',
'tooltip-preview'                 => 'तुम्ही केलेल्या बदलांची झलक पहा, जतन करण्यापूर्वी कृपया हे वापरा!',
'tooltip-diff'                    => 'या पाठ्यातील तुम्ही केलेले बदल दाखवा.',
'tooltip-compareselectedversions' => 'निवडलेल्या आवृत्त्यांमधील बदल दाखवा.',
'tooltip-watch'                   => 'हे पान तुमच्या पहार्‍याच्या सूचीत टाका.',
'tooltip-recreate'                => 'हे पान मागे वगळले असले तरी नवनिर्मीत करा',
'tooltip-upload'                  => 'चढवणे सुरूकरा',

# Metadata
'nodublincore'      => 'या विदादात्याकरिता Dublin Core RDF metadata अनुपलब्ध केला आहे.',
'nocreativecommons' => 'या विदादात्याकरिता Creative Commons RDF metadata अनुपलब्ध आहे.',
'notacceptable'     => 'विकि विदादाता तुमचा घेता वाचू शकेल अशा स्वरूपात(संरचनेत) विदा पुरवू शकत नाही.',

# Attribution
'anonymous'        => '{{SITENAME}} वरील अनामिक सदस्य',
'siteuser'         => '<!--{{SITENAME}}-->मराठी विकिपीडियाचा सदस्य $1',
'lastmodifiedatby' => 'या पानातील शेवटचा बदल $3ने $2, $1 यावेळी केला.', # $1 date, $2 time, $3 user
'othercontribs'    => '$1 ने केलेल्या कामानुसार.',
'others'           => 'इतर',
'siteusers'        => '{{SITENAME}} सदस्य $1',
'creditspage'      => 'पान श्रेय नामावली',
'nocredits'        => 'या पानाकरिता श्रेय नामावलीची माहिती नाही.',

# Spam protection
'spamprotectiontitle' => 'केर(स्पॅम) सुरक्षा चाचणी',
'spamprotectiontext'  => 'तुम्ही जतन करू इच्छित असलेले पान चिखलणी रोधक चाळणीने प्रतिबंधीत केले आहे.असे बाहेरच्या संकेतस्थळाचा दुवा देण्याची शक्यता असल्यामुळे घडू शकते.',
'spamprotectionmatch' => 'खालील मजकुरामुळे आमची चिखलणी रोधक चाळणी सुरू झाली: $1',
'spambot_username'    => 'मिडियाविकि स्पॅम स्वछता',
'spam_reverting'      => '$1शी दुवे नसलेल्या गेल्या आवर्तनाकडे परत उलटवत आहे',
'spam_blanking'       => '$1शी दुवे असलेली सर्व आवर्तने,रिक्त केली जात आहेत',

# Info page
'infosubtitle'   => 'पानाची माहिती',
'numedits'       => 'संपादनांची संख्या (पान): $1',
'numtalkedits'   => 'संपादनांची संख्या(चर्चा पान): $1',
'numwatchers'    => 'बघ्यांची संख्या: $1',
'numauthors'     => 'स्पष्ट पणे वेगळ्या लेखकांची संख्या (पान): $1',
'numtalkauthors' => 'स्पष्टपणे वेग-वेगळ्या लेखकांची संख्या(चर्चा पान): $1',

# Math options
'mw_math_png'    => 'नेहमीच पीएनजी (PNG) रेखाटा',
'mw_math_simple' => 'सुलभ असल्यास एचटीएमएल (HTML); अन्यथा पीएनजी (PNG)',
'mw_math_html'   => 'शक्य असल्यास एचटीएमएल (HTML); अन्यथा पीएनजी (PNG)',
'mw_math_source' => '(टेक्स्ट विचरकांकरिता)त्यास TeX म्हणून सोडून द्या',
'mw_math_modern' => 'आधूनिक विचरकांकरिता सूचवलेले',
'mw_math_mathml' => 'शक्य असल्यास मॅथ एमएल (MathML) (प्रयोगावस्था)',

# Patrolling
'markaspatrolleddiff'                 => 'टेहळणी केल्याची खूण करा',
'markaspatrolledtext'                 => 'या पानावर गस्त झाल्याची खूण करा',
'markedaspatrolled'                   => 'गस्त केल्याची खूण केली',
'markedaspatrolledtext'               => 'निवडलेले आवर्तनास गस्त घातल्याची खूण केली.',
'rcpatroldisabled'                    => 'अलिकडील बदलची गस्ती अनुपलब्ध',
'rcpatroldisabledtext'                => 'सध्या ’अलिकडील बदल’ ची गस्त सुविधा अनुपलब्ध केली आहे.',
'markedaspatrollederror'              => 'गस्तीची खूण करता येत नाही',
'markedaspatrollederrortext'          => 'गस्त घातल्याची खूण करण्याकरिता तुम्हाला एक आवर्तन नमुद करावे लागेल.',
'markedaspatrollederror-noautopatrol' => 'तुम्हाला स्वत:च्याच बदलांवर गस्त घातल्याची खूण करण्याची परवानगी नाही.',

# Patrol log
'patrol-log-page' => 'टेहळणीतील नोंदी',
'patrol-log-line' => '$2चे $1 $3 गस्त घातल्याची खूण केली',
'patrol-log-auto' => '(स्वयंचलीत)',

# Image deletion
'deletedrevision'                 => 'जुनी आवृत्ती ($1) वगळली.',
'filedeleteerror-short'           => 'संचिका वगळताना त्रूटी: $1',
'filedeleteerror-long'            => 'संचिका वगळताना त्रूटी आढळल्या:

$1',
'filedelete-missing'              => 'संचिका "$1" वगळता येत नाही, कारण ती अस्तित्वात नाही.',
'filedelete-old-unregistered'     => 'निर्देशीत संचिका आवर्तन "$1" विदागारात नाही.',
'filedelete-current-unregistered' => 'नमुद संचिका "$1" विदागारात नाही.',
'filedelete-archive-read-only'    => 'विदागार धारीका "$1" आंतरजाल विदादात्याकडून लेखनीय नाही.',

# Browsing diffs
'previousdiff' => '← मागील फरक',
'nextdiff'     => 'पुढील फरक →',

# Media information
'mediawarning'         => "'''सावधान''': या संचिकेत डंखी संकेत असू शकतो,जो वापरल्याने तुमच्या संगणक प्रणालीस नाजूक परिस्थितीस सामोरे जावे लागू शकते.<hr />",
'imagemaxsize'         => 'संचिका वर्णन पानांवरील चित्रांना मर्यादा घाला:',
'thumbsize'            => 'इवलासा आकार:',
'widthheightpage'      => '$1×$2, $3 {{PLURAL:$3|पान|पाने}}',
'file-info'            => '(संचिकेचा आकार:$1,विविधामाप(माईम)प्रकार: $2)',
'file-info-size'       => '($1 × $2 pixel, संचिकेचा आकार: $3, MIME प्रकार: $4)',
'file-nohires'         => '<small>यापेक्षा जास्त मोठे चित्र उपलब्ध नाही.</small>',
'svg-long-desc'        => '(SVG संचिका, साधारणपणे $1 × $2 pixels, संचिकेचा आकार: $3)',
'show-big-image'       => 'संपूर्ण रिजोल्यूशन',
'show-big-image-thumb' => '<small>या झलकेचा आकार: $1 × $2 pixels</small>',

# Special:NewImages
'newimages'             => 'नवीन संचिकांची यादी',
'imagelisttext'         => "खाली '''$1''' संचिका {{PLURAL:$1|दिली आहे.|$2 क्रमाने दिल्या आहेत.}}",
'newimages-summary'     => 'हे विशेष पान शेवटी चढविलेल्या संचिका दर्शविते',
'showhidebots'          => '(सांगकामे $1)',
'noimages'              => 'बघण्यासारखे येथे काही नाही.',
'ilsubmit'              => 'शोधा',
'bydate'                => 'तारखेनुसार',
'sp-newimages-showfrom' => '$2, $1 पासूनच्या नवीन संचिका दाखवा',

# Bad image list
'bad_image_list' => 'रूपरेषा खालील प्रमाणे आहे:

फक्त यादीमधील संचिका (ज्यांच्यापुढे * हे चिन्ह आहे अशा ओळी) लक्षात घेतलेल्या आहेत. ओळीवरील पहिला दुवा हा चुकीच्या संचिकेचा असल्याची खात्री करा. 
त्यापुढील दुवे हे अपवाद आहेत, म्हणजेच असे लेख जिथे ही संचिका मिळू शकते.',

# Metadata
'metadata'          => 'मेटाडाटा',
'metadata-help'     => 'या संचिकेत जास्तीची माहिती आहे, बहुधा संचिका अंकनीकरणाकरिता किंवा निर्मीतीकरिता वापरलेल्या अंकीय हूबहु छाउ (कॅमेरा) किंवा (स्कॅनर) कडून ही माहिती जमा झाली आहे. जर ही संचिका मूळ स्थिती पासून बदलली असेल तर काही माहिती नवीन संचिकेशी पूर्णपणे जुळणार नाही.',
'metadata-expand'   => 'जास्तीची माहिती दाखवा',
'metadata-collapse' => 'जास्तीची माहिती लपवा',
'metadata-fields'   => 'या यादीतील जी माहिती दिलेली असेल ती माहिती संचिकेच्या खाली मेटाडाटा माहितीत दिसेल. बाकीची माहिती झाकलेली राहील.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => 'रूंदी',
'exif-imagelength'                 => 'उंची',
'exif-bitspersample'               => 'प्रती घटक बीट्स',
'exif-compression'                 => 'आकुंचन योजना',
'exif-photometricinterpretation'   => 'चित्रांश विन्यास (पिक्सेल कॉम्पोझीशन)',
'exif-orientation'                 => 'वळण',
'exif-samplesperpixel'             => 'घटकांची संख्या',
'exif-planarconfiguration'         => 'विदा रचना',
'exif-ycbcrsubsampling'            => 'Y चे C शी  उपनमुनातपासणी (सबसॅम्पलींग) गुणोत्तर',
'exif-ycbcrpositioning'            => 'Y आणि C प्रतिस्थापना (पोझीशनींग)',
'exif-xresolution'                 => 'समांतर रिझोल्यूशन',
'exif-yresolution'                 => 'उभे रिझोल्यूशन',
'exif-resolutionunit'              => 'X आणि Y रिझोल्यूशन चे मानक प्रमाण',
'exif-stripoffsets'                => 'चित्रविदा स्थान',
'exif-rowsperstrip'                => 'प्रत्येक पट्टीतील ओळींची संख्या',
'exif-stripbytecounts'             => 'प्रत्येक आकुंचीत पट्टीतील बाईट्सची संख्या',
'exif-jpeginterchangeformat'       => 'JPEG SOI करिता ऑफसेट',
'exif-jpeginterchangeformatlength' => 'JPEG विदे च्या बाईट्स',
'exif-transferfunction'            => 'ट्रान्स्फर फंक्शन',
'exif-whitepoint'                  => 'धवल बिंदू क्रोमॅटिसिटी',
'exif-primarychromaticities'       => 'क्रोमॅटिसिटीज ऑफ प्राईमारिटीज',
'exif-ycbcrcoefficients'           => 'कलर स्पेस ट्रान्स्फॉर्मेशन मॅट्रीक्स कोएफिशीयंट्स',
'exif-referenceblackwhite'         => 'काळ्या आणि पांढर्‍या संदर्भ मुल्यांची जोडी',
'exif-datetime'                    => 'संचिका बदल तारीख आणि वेळ',
'exif-imagedescription'            => 'चित्र शीर्षक',
'exif-make'                        => 'हुबहू छाउ (कॅमेरा) उत्पादक',
'exif-model'                       => 'हुबहू छाउ (कॅमेरा) उपकरण',
'exif-software'                    => 'वापरलेली संगणन अज्ञावली',
'exif-artist'                      => 'लेखक',
'exif-copyright'                   => 'प्रताधिकार धारक',
'exif-exifversion'                 => 'Exif आवृत्ती',
'exif-flashpixversion'             => 'पाठींबा असलेली फ्लॅशपीक्स मानक आवृत्ती',
'exif-colorspace'                  => 'रंगांकन (कलर स्पेस)',
'exif-componentsconfiguration'     => 'प्रत्येक घटकाचा अर्थ',
'exif-compressedbitsperpixel'      => 'चित्र आकुंचन स्थिती',
'exif-pixelydimension'             => 'आकृतीची सुयोग्य रूंदी',
'exif-pixelxdimension'             => 'आकृतीची सुयोग्य उंची',
'exif-makernote'                   => 'उत्पादकाच्या सूचना',
'exif-usercomment'                 => 'सदस्य प्रतिक्रीया',
'exif-relatedsoundfile'            => 'संबधीत ध्वनी संचिका',
'exif-datetimeoriginal'            => 'विदा निर्मितीची तारीख आणि वेळ',
'exif-datetimedigitized'           => 'अंकनीकरणाची तारीख आणि वेळ',
'exif-subsectime'                  => 'तारीख वेळ उपसेकंद',
'exif-subsectimeoriginal'          => 'तारीखवेळमुळ उपसेकंद',
'exif-subsectimedigitized'         => 'तारीखवेळ अंकनीकृत उपसेकंद',
'exif-exposuretime'                => 'छायांकन कालावधी',
'exif-exposuretime-format'         => '$1 सेक ($2)',
'exif-fnumber'                     => 'F क्रमांक',
'exif-exposureprogram'             => "'''प्रभाव'''न कार्य (एक्स्पोजर प्रोग्राम)",
'exif-spectralsensitivity'         => 'झोत संवेदनशीलता (स्पेक्ट्रल सेन्सिटीव्हिटी)',
'exif-isospeedratings'             => 'आंतरराष्ट्रीय मानक संस्थेचे वेग मुल्यमापन',
'exif-oecf'                        => 'दृक्वैद्यूतसंचरण परिवर्तन कारक (ऑप्टोईलेक्ट्रॉनिक कन्व्हर्शन फॅक्टर)',
'exif-shutterspeedvalue'           => 'शटर वेग',
'exif-aperturevalue'               => 'रन्ध्र (ऍपर्चर)',
'exif-brightnessvalue'             => 'झळाळी (ब्राईटपणा)',
'exif-exposurebiasvalue'           => 'प्रभावन अभिनत (एक्सपोजर बायस)',
'exif-maxaperturevalue'            => 'महत्तम जमिनी रन्ध्र(लॅंड ऍपर्चर)',
'exif-subjectdistance'             => 'गोष्टीपासूनचे अंतर',
'exif-meteringmode'                => 'मीटरींग मोड',
'exif-lightsource'                 => 'प्रकाश स्रोत',
'exif-flash'                       => "लख'''लखाट''' (फ्लॅश)",
'exif-focallength'                 => 'भींगाची मध्यवर्ती लांबी (फोकल लांबी)',
'exif-subjectarea'                 => 'विषय विभाग',
'exif-flashenergy'                 => 'लखाट उर्जा (फ्लॅश एनर्जी)',
'exif-spatialfrequencyresponse'    => 'विशाल लहर प्रतिक्रिया (स्पॅटीअल फ्रिक्वेन्सी रिस्पॉन्स)',
'exif-focalplanexresolution'       => 'फोकल प्लेन x रिझोल्यूशन',
'exif-focalplaneyresolution'       => 'फोकल प्लेन Y रिझोल्यूशन',
'exif-focalplaneresolutionunit'    => 'फोकल प्लेन  रिझोल्यूशन माप',
'exif-subjectlocation'             => 'लक्ष्य स्थळ',
'exif-exposureindex'               => 'प्रभावन सूची',
'exif-sensingmethod'               => 'सेन्सींग पद्धती',
'exif-filesource'                  => 'संचिका स्रोत',
'exif-scenetype'                   => 'दृष्य प्रकार',
'exif-cfapattern'                  => 'CFA पॅटर्न',
'exif-customrendered'              => 'कस्टम इमेज प्रोसेसिंग',
'exif-exposuremode'                => "'''प्रभाव'''न मोड",
'exif-whitebalance'                => 'व्हाईट बॅलन्स',
'exif-digitalzoomratio'            => 'अंकीय झूम गुणोत्तर',
'exif-focallengthin35mmfilm'       => 'भींगाची मध्यवर्ती लांबी (फोकल लांबी) ३५ मी.मी. फील्ममध्ये',
'exif-scenecapturetype'            => 'दृश्य टिपण्याचा प्रकार',
'exif-gaincontrol'                 => 'दृश्य नियंत्रण',
'exif-contrast'                    => 'विभेद (कॉन्ट्रास्ट)',
'exif-saturation'                  => 'सॅचूरेशन',
'exif-sharpness'                   => 'प्रखरता(शार्पनेस)',
'exif-devicesettingdescription'    => 'उपकरण रचना वर्णन',
'exif-subjectdistancerange'        => 'गोष्टीपासूनचे पल्ला अंतर',
'exif-imageuniqueid'               => 'विशिष्ट चित्र क्रमांक',
'exif-gpsversionid'                => 'GPS खूण आवृत्ती',
'exif-gpslatituderef'              => 'उत्तर किंवा दक्षीण अक्षांश',
'exif-gpslatitude'                 => 'अक्षांश',
'exif-gpslongituderef'             => 'पूर्व किंवा पश्चिम रेखांश',
'exif-gpslongitude'                => 'रेखांश',
'exif-gpsaltituderef'              => 'उन्नतांश संदर्भ',
'exif-gpsaltitude'                 => 'उन्नतांश (अल्टीट्यूड)',
'exif-gpstimestamp'                => 'GPS वेळ(ऍटॉमिक घड्याळ)',
'exif-gpssatellites'               => 'मापनाकरिता वापरलेला उपग्रह',
'exif-gpsstatus'                   => 'प्राप्तकर्त्याची स्थिती',
'exif-gpsmeasuremode'              => 'मापन स्थिती',
'exif-gpsdop'                      => 'मापन अचूकता',
'exif-gpsspeedref'                 => 'वेग एकक',
'exif-gpsspeed'                    => 'GPS प्राप्तकर्त्याचा वेग',
'exif-gpstrackref'                 => 'हालचालीच्या दिशेकरिता संदर्भ',
'exif-gpstrack'                    => 'हालचालीची दिशा',
'exif-gpsimgdirectionref'          => 'चित्राच्या दिशेकरिता संदर्भ',
'exif-gpsimgdirection'             => 'चित्राची दिशा',
'exif-gpsmapdatum'                 => 'Geodetic पाहणी विदा वापरली',
'exif-gpsdestlatituderef'          => 'लक्ष्याचे अक्षांशाकरिता संदर्भ',
'exif-gpsdestlatitude'             => 'अक्षांश लक्ष्य',
'exif-gpsdestlongituderef'         => 'लक्ष्याचे रेखांशकरिता संदर्भ',
'exif-gpsdestlongitude'            => 'रेखांशाचे लक्ष्य',
'exif-gpsdestbearingref'           => 'बियरींग डेस्टीनेशनकरिता संदर्भ',
'exif-gpsdestbearing'              => 'बीअरींग ऑफ डेस्टीनेशन',
'exif-gpsdestdistanceref'          => 'लक्ष्यस्थळापर्यंतच्या अंतराकरिता संदर्भ',
'exif-gpsdestdistance'             => 'लक्ष्यस्थळापर्यंतचे अंतर',
'exif-gpsprocessingmethod'         => 'GPS प्रक्रीया पद्धतीचे नाव',
'exif-gpsareainformation'          => 'GPS विभागाचे नाव',
'exif-gpsdatestamp'                => 'GPSतारीख',
'exif-gpsdifferential'             => 'GPS डिफरेंशीअल सुधारणा',

# EXIF attributes
'exif-compression-1' => 'अनाकुंचीत',

'exif-unknowndate' => 'अज्ञात तारीख',

'exif-orientation-1' => 'सामान्य', # 0th row: top; 0th column: left
'exif-orientation-2' => 'समांतर पालटले', # 0th row: top; 0th column: right
'exif-orientation-3' => '180° फिरवले', # 0th row: bottom; 0th column: right
'exif-orientation-4' => 'उभ्या बाजूने पालटले', # 0th row: bottom; 0th column: left
'exif-orientation-5' => '९०° CCW अंशात वळवले आणि उभे पालटले', # 0th row: left; 0th column: top
'exif-orientation-6' => '90° CW फिरवले', # 0th row: right; 0th column: top
'exif-orientation-7' => '90° CW वळवले आणि उभे पलटवले', # 0th row: right; 0th column: bottom
'exif-orientation-8' => '90° CCW फिरवले', # 0th row: left; 0th column: bottom

'exif-planarconfiguration-1' => 'चंकी संरचना (रूपरेषा)',
'exif-planarconfiguration-2' => 'प्लानर संरचना(रूपरेषा)',

'exif-componentsconfiguration-0' => 'अस्तित्वात नाही',

'exif-exposureprogram-0' => 'अव्यक्त',
'exif-exposureprogram-1' => 'हातकाम',
'exif-exposureprogram-2' => 'सामान्य प्रोग्रॅम',
'exif-exposureprogram-3' => 'रन्ध्र (ऍपर्चर) प्राथमिकता',
'exif-exposureprogram-4' => 'झडप (शटर प्राथमिकता)',
'exif-exposureprogram-5' => 'क्रियेटीव्ह कार्यक्रम(विषयाच्या खोलीस बायस्ड)',
'exif-exposureprogram-6' => 'कृती कार्यक्रम(द्रूत आवर्तद्वार(शटर) वेग कडे बायस्ड)',
'exif-exposureprogram-7' => 'व्यक्तिचित्र स्थिती(क्लोजप छायाचित्रांकरिता आऊट ऑफ फोकस बॅकग्राऊंड सहीत)',
'exif-exposureprogram-8' => 'लॅंडस्केप स्थिती (लॅंडस्केप छायाचित्रांकरिता बॅकग्राऊंड इन फोकस सहीत)',

'exif-subjectdistance-value' => '$1 मीटर',

'exif-meteringmode-0'   => 'अज्ञात',
'exif-meteringmode-1'   => 'सरासरी',
'exif-meteringmode-2'   => 'सेंटरवेटेड सरासरी',
'exif-meteringmode-3'   => 'स्पॉट',
'exif-meteringmode-4'   => 'मल्टीस्पॉट',
'exif-meteringmode-5'   => 'पद्धत(पॅटर्न)',
'exif-meteringmode-6'   => 'अर्धवट',
'exif-meteringmode-255' => 'इतर',

'exif-lightsource-0'   => 'अज्ञात',
'exif-lightsource-1'   => 'सूर्यप्रकाश',
'exif-lightsource-2'   => 'फ्लूरोसेंट',
'exif-lightsource-3'   => 'टंगस्ट्न (इनकॅन्‍डेसेंट प्रकाश)',
'exif-lightsource-4'   => "लख'''लखाट''' (फ्लॅश)",
'exif-lightsource-9'   => 'चांगले हवामान',
'exif-lightsource-10'  => 'ढगाळ हवामान',
'exif-lightsource-11'  => 'छटा',
'exif-lightsource-12'  => 'दिवसप्रकाशी फ्लूरोशेंट (D 5700 – 7100K)',
'exif-lightsource-13'  => 'दिवस प्रकाशी फ्लूरोसेंट (N ४६०० – ५४०० K)',
'exif-lightsource-14'  => 'शीतल पांढरा फ्लूरोशेंट (W 3900 – 4500K)',
'exif-lightsource-15'  => 'व्हाईट फ्लूरोसेंट(WW ३२०० – ३७००K)',
'exif-lightsource-17'  => 'प्रकाश दर्जा A',
'exif-lightsource-18'  => 'प्रकाश दर्जा B',
'exif-lightsource-19'  => 'प्रमाण प्रकाश C',
'exif-lightsource-24'  => 'ISO स्टूडीयो टंगस्टन',
'exif-lightsource-255' => 'इतर प्रकाश स्रोत',

'exif-focalplaneresolutionunit-2' => 'इंच',

'exif-sensingmethod-1' => 'अव्यक्त',
'exif-sensingmethod-2' => 'वन चीप कलर एरीया सेन्‍सर',
'exif-sensingmethod-3' => 'टू चीप कलर एरीया सेन्‍सर',
'exif-sensingmethod-4' => 'थ्री चीप कलर एरीया सेन्‍सर',
'exif-sensingmethod-5' => 'कलर सिक्वेण्शीयल एरीया सेंसॉर',
'exif-sensingmethod-7' => 'ट्राय्‍एलिनीयर सेंसर',
'exif-sensingmethod-8' => 'कलर सिक्वेंशीयल लिनीयर सेन्‍सर',

'exif-scenetype-1' => 'डायरेक्टली छायाचित्रीत चित्र',

'exif-customrendered-0' => 'नियमीत प्रक्रीया',
'exif-customrendered-1' => 'आवडीनुसार प्रक्रीया',

'exif-exposuremode-0' => 'स्वयंचलित छायांकन',
'exif-exposuremode-1' => 'अस्वयंचलित छायांकन',
'exif-exposuremode-2' => 'स्वयंसिद्ध कंस',

'exif-whitebalance-0' => 'ऍटो व्हाईट बॅलेन्स',
'exif-whitebalance-1' => 'मॅन्यूअल व्हाईट बॅलेन्स',

'exif-scenecapturetype-0' => 'दर्जा',
'exif-scenecapturetype-1' => 'आडवे',
'exif-scenecapturetype-2' => 'उभे',
'exif-scenecapturetype-3' => 'रात्रीचे दृश्य',

'exif-gaincontrol-0' => 'नाही',
'exif-gaincontrol-1' => 'लघु वृद्धी वर',
'exif-gaincontrol-2' => 'बृहत्‌ वृद्धी वर',
'exif-gaincontrol-3' => 'लघु वृद्धी खाली',
'exif-gaincontrol-4' => 'बृहत्‌ वृद्धी खाली',

'exif-contrast-0' => 'सामान्य',
'exif-contrast-1' => 'नरम',
'exif-contrast-2' => 'कठीण',

'exif-saturation-0' => 'सर्व साधारण',
'exif-saturation-1' => 'कमी सॅचूरेशन',
'exif-saturation-2' => 'जास्त सॅचूरेशन',

'exif-sharpness-0' => 'सर्वसाधारण',
'exif-sharpness-1' => 'मृदू',
'exif-sharpness-2' => 'कठीण',

'exif-subjectdistancerange-0' => 'अज्ञात',
'exif-subjectdistancerange-1' => 'मॅक्रो',
'exif-subjectdistancerange-2' => 'जवळचे दृश्य',
'exif-subjectdistancerange-3' => 'दूरचे दृश्य',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'उत्तर अक्षांश',
'exif-gpslatitude-s' => 'दक्षीण अक्षांश',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'पूर्व रेखांश',
'exif-gpslongitude-w' => 'पश्चिम रेखांश',

'exif-gpsstatus-a' => 'मोजणी काम चालू आहे',
'exif-gpsstatus-v' => 'आंतरोपयोगीक्षमतेचे मोजमाप',

'exif-gpsmeasuremode-2' => 'द्वि-दिश मापन',
'exif-gpsmeasuremode-3' => 'त्रि-दिश मोजमाप',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'प्रतिताशी किलोमीटर',
'exif-gpsspeed-m' => 'प्रतिताशी मैल',
'exif-gpsspeed-n' => 'गाठी',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => 'बरोबर दिशा',
'exif-gpsdirection-m' => 'चुंबकीय दिशा',

# External editor support
'edit-externally'      => 'बाहेरील संगणक प्रणाली वापरून ही संचिका संपादा.',
'edit-externally-help' => 'अधिक माहितीसाठी [http://www.mediawiki.org/wiki/Manual:External_editors स्थापन करण्याच्या सूचना] पहा.',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'सर्व',
'imagelistall'     => 'सर्व',
'watchlistall2'    => 'सर्व',
'namespacesall'    => 'सर्व',
'monthsall'        => 'सर्व',

# E-mail address confirmation
'confirmemail'             => 'इमेल पत्ता पडताळून पहा',
'confirmemail_noemail'     => '[[Special:Preferences|सदस्य पसंतीत]] तुम्ही शाबीत विपत्र पत्ता दिलेला नाही.',
'confirmemail_text'        => 'विपत्र सुविधा वापरण्या पूर्वी {{SITENAME}}वर तुमचा विपत्र पत्ता  शाबीत करणे गरजेचे आहे.तुमच्या पत्त्यावर निश्चितीकरण विपत्र पाठवण्याकरिता खालील बटण सुरू करा.विपत्रात कुटसंकेत असलेला दुवा असेल;तुमचा विपत्र पत्ता शाबीत करण्या करिता तुमच्या विचरकात हा दिलेला दुवा चढवा.',
'confirmemail_pending'     => '<div class="error">एक निश्चितीकरण कुटसंकेत आधीच तुम्हाला विपत्र केला आहे; जर तुम्ही खाते अशातच उघडले असेल तर,एक नवा कुट संकेत मागण्यापूर्वी,पाठवलेला मिळण्याकरिता थोडी मिनिटे वाट पहाणे तुम्हाला आवडू शकेल.</div>',
'confirmemail_send'        => 'विपत्र निश्चितीकरण नियमावली',
'confirmemail_sent'        => 'शाबितीकरण विपत्र पाठवले.',
'confirmemail_oncreate'    => 'तुमच्या विपत्र पत्त्यावर निश्चितीकरण कुटसंकेत पाठवला होता .
हा कुटसंकेत तुम्हाला खात्यात दाखल होण्याकरिता लागणार नाही,पण तुम्हाला तो कोणतीही विपत्रावर अवलंबून कोणतीही सुविधा उपलब्ध करून घेण्यापूर्वी द्यावा लागेल.',
'confirmemail_sendfailed'  => 'पोच-विपत्र पाठवू शकलो नाही.अयोग्य चिन्हांकरिता पत्ता तपासा.

मेलर वापसी: $1',
'confirmemail_invalid'     => 'अयोग्य निश्चितीकरण नियमावली.नियमावली काल समाप्त झाला असु शकेल.',
'confirmemail_needlogin'   => 'तुमचा विपत्रपत्ता शाबीत करण्यासाठी तुम्ही $1 करावयास हवे.',
'confirmemail_success'     => 'तुमचा विपत्रपत्ता शाबीत झाला आहे.तुम्ही आता दाखल होऊ शकता आणि विकिचा आनंद घेऊ शकता.',
'confirmemail_loggedin'    => 'तुमचा विपत्र पत्ता आता शाबीत झाला आहे.',
'confirmemail_error'       => 'तुमची निश्चिती जतन करताना काही तरी चूकले',
'confirmemail_subject'     => '{{SITENAME}} विपत्र पत्ता शाबीत',
'confirmemail_body'        => 'कुणीतरी, बहुतेक तुम्ही, $1 या पत्त्यावारून, "$2" खाते हा ईमेल पत्ता वापरून {{SITENAME}} या संकेतस्थळावर उघडले आहे.

हे खाते खरोखर तुमचे आहे याची खात्री करण्यासाठी आणि {{SITENAME}} वर ईमेल पर्याय उत्तेजित (उपलब्ध) करण्यासाठी, हा दुवा तुमच्या ब्राउजर मधे उघडा:

$3

जर तुम्ही हे खाते उघडले *नसेल* तर ही मागणी रद्द करण्यासाठी खालील दुवा उघडा:

$5

हा हमी कलम $4 ला नष्ट होईल.',
'confirmemail_invalidated' => 'इ-मेल पत्ता तपासणी रद्द करण्यात आलेली आहे',
'invalidateemail'          => 'इ-मेल तपासणी रद्द करा',

# Scary transclusion
'scarytranscludedisabled' => '[आंतरविकि आंतरन्यास अनुपलब्ध केले आहे]',
'scarytranscludefailed'   => '[क्षमस्व;$1करिताची साचा ओढी फसली]',
'scarytranscludetoolong'  => '[URL खूप लांब आहे; क्षमस्व]',

# Trackbacks
'trackbackbox'      => '<div id="mw_trackbacks">या पानाकरिता मागेवळून पहा:<br />$1</div>',
'trackbackremove'   => '([$1 वगळा])',
'trackbacklink'     => 'पाठीमाग(पाठलाग)',
'trackbackdeleteok' => 'पाठलाग यशस्वीपणे वगळला.',

# Delete conflict
'deletedwhileediting' => 'सूचना: तुम्ही संपादन सुरू केल्यानंतर हे पान वगळले गेले आहे.',
'confirmrecreate'     => "तुम्ही संपादन सुरू केल्यानंतर सदस्य [[User:$1|$1]] ([[User talk:$1|चर्चा]])ने हे पान पुढील कारणाने वगळले:
: ''$2''
कृपया हे पान खरेच पुन्हा निर्मीत करून हवे आहे का हे निश्चित करा.",
'recreate'            => 'पुनर्निर्माण',

# HTML dump
'redirectingto' => '[[:$1]]कडे पुनर्निर्देशीत...',

# action=purge
'confirm_purge'        => 'यापानाची सय रिकामी करावयाची आहे?

$1',
'confirm_purge_button' => 'ठीक',

# AJAX search
'searchcontaining' => "''$1'' शब्द असलेले लेख शोधा.",
'searchnamed'      => "''$1'' या नावाचे लेख शोधा.",
'articletitles'    => "''$1'' पासून सुरू होणारे लेख",
'hideresults'      => 'निकाल लपवा',
'useajaxsearch'    => 'AJAX शोध वापरा',

# Multipage image navigation
'imgmultipageprev' => '← मागील पान',
'imgmultipagenext' => 'पुढील पान →',
'imgmultigo'       => 'चला!',
'imgmultigoto'     => '$1 पानावर जा',

# Table pager
'ascending_abbrev'         => 'चढ',
'descending_abbrev'        => 'उतर',
'table_pager_next'         => 'पुढील पान',
'table_pager_prev'         => 'मागील पान',
'table_pager_first'        => 'पहिले पान',
'table_pager_last'         => 'शेवटचे पान',
'table_pager_limit'        => 'एका पानावर $1 नग दाखवा',
'table_pager_limit_submit' => 'चला',
'table_pager_empty'        => 'निकाल नाहीत',

# Auto-summaries
'autosumm-blank'   => 'या पानावरील सगळा मजकूर काढला',
'autosumm-replace' => "पान '$1' वापरून बदलले.",
'autoredircomment' => '[[$1]] कडे पुनर्निर्देशित',
'autosumm-new'     => 'नवीन पान: $1',

# Live preview
'livepreview-loading' => 'चढवत आहे…',
'livepreview-ready'   => 'चढवत आहे… तयार!',
'livepreview-failed'  => 'प्रत्यक्ष ताजी झलक अयश्स्वी! नेहमीची झलक पहा.',
'livepreview-error'   => 'संपर्कात अयशस्वी: $1 "$2".नेहमीची झलक पहा.',

# Friendlier slave lag warnings
'lag-warn-normal' => '$1 सेकंदाच्या आतले बदल या यादी नसण्याची शक्यता आहे.',
'lag-warn-high'   => 'विदा विदादात्यास लागणार्‍या अत्यूच्च कालावधी मूळे, $1 सेकंदापेक्षा नवे बदल या सूचीत न दाखवले जाण्याची शक्यता आहे.',

# Watchlist editor
'watchlistedit-numitems'       => 'चर्चा पाने सोडून, {{PLURAL:$1|1 शीर्षक पान|$1 शीर्षक पाने}} तुमच्या पहार्‍याच्या सूचीत आहेत.',
'watchlistedit-noitems'        => 'पहार्‍याच्या सूचीत कोणतेही शीर्षक पान नोंदलेले नाही.',
'watchlistedit-normal-title'   => 'पहार्‍याची सूची संपादीत करा',
'watchlistedit-normal-legend'  => 'शीर्षकपाने पहार्‍याच्या सूचीतून वगळा',
'watchlistedit-normal-explain' => 'तुमच्या पहार्‍याच्या सूचीतील अंतर्भूत नामावळी खाली निर्देशीत केली आहे. शीर्षक वगळण्याकरिता, त्या पुढील खिडकी निवडा, आणि शीर्षक वगळावर टिचकी मारा. तुम्ही [[Special:Watchlist/raw|कच्ची यादी सुद्धा संपादित]] करू शकता.',
'watchlistedit-normal-submit'  => 'शिर्षक वगळा',
'watchlistedit-normal-done'    => 'तुमच्या पहार्‍याच्या सूचीतून वगळलेली {{PLURAL:$1|1 शिर्षक होते |$1 शिर्षके होती }}:',
'watchlistedit-raw-title'      => 'कच्ची पहार्‍याची सूची संपादीत करा.',
'watchlistedit-raw-legend'     => 'कच्ची पहार्‍याची सूची संपादीत करा.',
'watchlistedit-raw-explain'    => 'तुमच्या पहार्‍याच्या सूचीतील अंतर्भूत नामावळी खाली निर्देशीत केली आहे, एका ओळीत एक नाव या पद्धतीने; ह्या यादीतील नावे वगळून किंवा भर घालून संपादित करून नामावळी अद्यावत(परिष्कृत) करता येते.
पहार्‍याची सूची अद्यावत करा येथे टिचकी मारा.
तुम्ही [[Special:Watchlist/edit|प्रस्थापित संपादकाचा उपयोग]] सुद्धा करू शकता.',
'watchlistedit-raw-titles'     => 'शिर्षके:',
'watchlistedit-raw-submit'     => 'पहार्‍याची सूची अद्यावत करा.',
'watchlistedit-raw-done'       => 'तुमची पहार्‍याची सूची परिष्कृत करण्यात आली आहे.',
'watchlistedit-raw-added'      => '{{PLURAL:$1|1 शिर्षक होते |$1 शिर्षक होती }} भर घातली:',
'watchlistedit-raw-removed'    => '{{PLURAL:$1|1 शिर्षक होते |$1 शिर्षक होती }} वगळले:',

# Watchlist editing tools
'watchlisttools-view' => 'सुयोग्य बदल पहा',
'watchlisttools-edit' => 'पहार्‍याची सूची पहा आणि संपादीत करा',
'watchlisttools-raw'  => 'कच्ची पहार्‍याची सूची संपादीत करा',

# Core parser functions
'unknown_extension_tag' => 'अज्ञात विस्तार खूण "$1"',

# Special:Version
'version'                          => 'आवृत्ती', # Not used as normal message but as header for the special page itself
'version-extensions'               => 'स्थापित विस्तार',
'version-specialpages'             => 'विशेष पाने',
'version-parserhooks'              => 'पृथकक अंकुश',
'version-variables'                => 'चल',
'version-other'                    => 'इतर',
'version-mediahandlers'            => 'मिडिया हॅंडलर',
'version-hooks'                    => 'अंकुश',
'version-extension-functions'      => 'अतिविस्तार(एक्स्टेंशन) कार्ये',
'version-parser-extensiontags'     => 'पृथकक विस्तारीत खूणा',
'version-parser-function-hooks'    => 'पृथकक कार्य अंकुश',
'version-skin-extension-functions' => 'त्वचा अतिविस्तार(एक्स्टेंशन) कार्ये',
'version-hook-name'                => 'अंकुश नाव',
'version-hook-subscribedby'        => 'वर्गणीदार',
'version-version'                  => 'आवृत्ती',
'version-license'                  => 'परवाना',
'version-software'                 => 'स्थापित संगणक प्रणाली (Installed software)',
'version-software-product'         => 'उत्पादन',
'version-software-version'         => 'आवृत्ती(Version)',

# Special:FilePath
'filepath'         => 'संचिका मार्ग',
'filepath-page'    => 'संचिका:',
'filepath-submit'  => 'मार्ग',
'filepath-summary' => 'हे विशेष पान संचिकेचा संपूर्ण मार्ग कळवते.
चित्रे संपूर्ण रिझोल्यूशन मध्ये दाखवली आहेत,इतर संचिका प्रकार त्यांच्या संबधीत प्रोग्रामने प्रत्यक्ष सुरू होतात.

"{{ns:image}}:" पूर्वपदा शिवाय संचिकेचे नाव भरा.',

# Special:FileDuplicateSearch
'fileduplicatesearch'          => 'जुळ्या संचिका शोधा',
'fileduplicatesearch-summary'  => 'हॅश किंमतीप्रमाणे जुळ्या संचिका शोधा.

"{{ns:image}}:" न लिहिता फक्त संचिकेचे नाव लिहा.',
'fileduplicatesearch-legend'   => 'जुळी संचिका शोधा',
'fileduplicatesearch-filename' => 'संचिकानाव:',
'fileduplicatesearch-submit'   => 'शोधा',
'fileduplicatesearch-info'     => '$1 × $2 पीक्सेल<br />संचिकेचा आकार: $3<br />MIME प्रकार: $4',
'fileduplicatesearch-result-1' => '"$1" या संचिकेशी जुळणारी जुळी संचिका सापडली नाही.',
'fileduplicatesearch-result-n' => '"$1" ला {{PLURAL:$2|१ जुळी संचिका आहे|$2 जुळ्या संचिका आहेत}}.',

# Special:SpecialPages
'specialpages'                   => 'विशेष पृष्ठे',
'specialpages-note'              => '----
* सर्वसाधारण विशेष पृष्ठे.
* <span class="mw-specialpagerestricted">प्रतिबंधित विशेष पृष्ठे.</span>',
'specialpages-group-maintenance' => 'व्यवस्थापन अहवाल',
'specialpages-group-other'       => 'इतर विशेष पृष्ठे',
'specialpages-group-login'       => 'प्रवेश / नवीन सदस्य नोंदणी',
'specialpages-group-changes'     => 'अलीकडील बदल व सूची',
'specialpages-group-media'       => 'मीडिया अहवाल व चढविलेल्या संचिका',
'specialpages-group-users'       => 'सदस्य व अधिकार',
'specialpages-group-highuse'     => 'सर्वात जास्त वापरली जाणारी पृष्ठे',
'specialpages-group-pages'       => 'पृष्ठ याद्या',
'specialpages-group-pagetools'   => 'पृष्ठ उपकरणे',
'specialpages-group-wiki'        => 'विकि डाटा व उपकरणे',
'specialpages-group-redirects'   => 'पुनर्निर्देशन करणारी विशेष पृष्ठे',
'specialpages-group-spam'        => 'स्पॅम उपकरणे',

);
