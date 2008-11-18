<?php
/** Hebrew (עברית)
 *
 * @ingroup Language
 * @file
 *
 * @author Rotem Dan (July 2003)
 * @author Rotem Liss (March 2006 on)
 */

$rtl = true;
$defaultUserOptionOverrides = array(
	# Swap sidebar to right side by default
	'quickbar' => 2,
);

$linkTrail = '/^([a-zא-ת]+)(.*)$/sDu';
$fallback8bitEncoding = 'windows-1255';

$skinNames = array(
	'standard'    => 'רגיל',
	'nostalgia'   => 'נוסטלגי',
	'cologneblue' => 'מים כחולים',
	'monobook'    => 'מונובוק',
	'myskin'      => 'הרקע שלי',
	'chick'       => "צ'יק",
	'simple'      => 'פשוט',
	'modern'      => 'מודרני',
);

$datePreferences = array(
	'default',
	'mdy',
	'dmy',
	'ymd',
	'hebrew',
	'ISO 8601',
);

$dateFormats = array(
	'mdy time' => 'H:i',
	'mdy date' => 'xg j, Y',
	'mdy both' => 'H:i, xg j, Y',

	'dmy time' => 'H:i',
	'dmy date' => 'j xg Y',
	'dmy both' => 'H:i, j xg Y',

	'ymd time' => 'H:i',
	'ymd date' => 'Y xg j',
	'ymd both' => 'H:i, Y xg j',

	'hebrew time' => 'H:i',
	'hebrew date' => 'xhxjj xjx xhxjY',
	'hebrew both' => 'H:i, xhxjj xjx xhxjY',

	'ISO 8601 time' => 'xnH:xni:xns',
	'ISO 8601 date' => 'xnY-xnm-xnd',
	'ISO 8601 both' => 'xnY-xnm-xnd"T"xnH:xni:xns',
);

$bookstoreList = array(
	'מיתוס'          => 'http://www.mitos.co.il/',
	'iBooks'         => 'http://www.ibooks.co.il/',
	'Barnes & Noble' => 'http://search.barnesandnoble.com/bookSearch/isbnInquiry.asp?isbn=$1',
	'Amazon.com'     => 'http://www.amazon.com/exec/obidos/ISBN=$1'
);

$magicWords = array(
	'redirect'              => array( 0,    '#הפניה',                                  '#REDIRECT'              ),
	'notoc'                 => array( 0,    '__ללא_תוכן_עניינים__', '__ללא_תוכן__',    '__NOTOC__'              ),
	'nogallery'             => array( 0,    '__ללא_גלריה__',                          '__NOGALLERY__'          ),
	'forcetoc'              => array( 0,    '__חייב_תוכן_עניינים__', '__חייב_תוכן__',   '__FORCETOC__'           ),
	'toc'                   => array( 0,    '__תוכן_עניינים__', '__תוכן__',             '__TOC__'                ),
	'noeditsection'         => array( 0,    '__ללא_עריכה__',                           '__NOEDITSECTION__'      ),
	'currentmonth'          => array( 1,    'חודש נוכחי',                               'CURRENTMONTH'           ),
	'currentmonthname'      => array( 1,    'שם חודש נוכחי',                            'CURRENTMONTHNAME'       ),
	'currentmonthnamegen'   => array( 1,    'שם חודש נוכחי קניין',                      'CURRENTMONTHNAMEGEN'    ),
	'currentmonthabbrev'    => array( 1,    'קיצור חודש נוכחי',                         'CURRENTMONTHABBREV'     ),
	'currentday'            => array( 1,    'יום נוכחי',                                'CURRENTDAY'             ),
	'currentday2'           => array( 1,    'יום נוכחי 2',                              'CURRENTDAY2'            ),
	'currentdayname'        => array( 1,    'שם יום נוכחי',                             'CURRENTDAYNAME'         ),
	'currentyear'           => array( 1,    'שנה נוכחית',                               'CURRENTYEAR'            ),
	'currenttime'           => array( 1,    'שעה נוכחית',                               'CURRENTTIME'            ),
	'currenthour'           => array( 1,    'שעות נוכחיות',                             'CURRENTHOUR'            ),
	'localmonth'            => array( 1,    'חודש מקומי',                               'LOCALMONTH'             ),
	'localmonthname'        => array( 1,    'שם חודש מקומי',                            'LOCALMONTHNAME'         ),
	'localmonthnamegen'     => array( 1,    'שם חודש מקומי קניין',                      'LOCALMONTHNAMEGEN'      ),
	'localmonthabbrev'      => array( 1,    'קיצור חודש מקומי',                         'LOCALMONTHABBREV'       ),
	'localday'              => array( 1,    'יום מקומי',                                'LOCALDAY'               ),
	'localday2'             => array( 1,    'יום מקומי 2',                              'LOCALDAY2'              ),
	'localdayname'          => array( 1,    'שם יום מקומי',                             'LOCALDAYNAME'           ),
	'localyear'             => array( 1,    'שנה מקומית',                               'LOCALYEAR'              ),
	'localtime'             => array( 1,    'שעה מקומית',                               'LOCALTIME'              ),
	'localhour'             => array( 1,    'שעות מקומיות',                             'LOCALHOUR'              ),
	'numberofpages'         => array( 1,    'מספר דפים כולל', 'מספר דפים',             'NUMBEROFPAGES'          ),
	'numberofarticles'      => array( 1,    'מספר ערכים',                              'NUMBEROFARTICLES'       ),
	'numberoffiles'         => array( 1,    'מספר קבצים',                              'NUMBEROFFILES'          ),
	'numberofusers'         => array( 1,    'מספר משתמשים',                            'NUMBEROFUSERS'          ),
	'numberofedits'         => array( 1,    'מספר עריכות',                             'NUMBEROFEDITS'          ),
	'pagename'              => array( 1,    'שם הדף',                                  'PAGENAME'               ),
	'pagenamee'             => array( 1,    'שם הדף מקודד',                            'PAGENAMEE'              ),
	'namespace'             => array( 1,    'מרחב השם',                                'NAMESPACE'              ),
	'namespacee'            => array( 1,    'מרחב השם מקודד',                          'NAMESPACEE'             ),
	'talkspace'             => array( 1,    'מרחב השיחה',                              'TALKSPACE'              ),
	'talkspacee'            => array( 1,    'מרחב השיחה מקודד',                        'TALKSPACEE'              ),
	'subjectspace'          => array( 1,    'מרחב הנושא', 'מרחב הערכים',              'SUBJECTSPACE', 'ARTICLESPACE' ),
	'subjectspacee'         => array( 1,    'מרחב הנושא מקודד', 'מרחב הערכים מקודד',  'SUBJECTSPACEE', 'ARTICLESPACEE' ),
	'fullpagename'          => array( 1,    'שם הדף המלא',                            'FULLPAGENAME'           ),
	'fullpagenamee'         => array( 1,    'שם הדף המלא מקודד',                      'FULLPAGENAMEE'          ),
	'subpagename'           => array( 1,    'שם דף המשנה',                            'SUBPAGENAME'            ),
	'subpagenamee'          => array( 1,    'שם דף המשנה מקודד',                      'SUBPAGENAMEE'           ),
	'basepagename'          => array( 1,    'שם דף הבסיס',                            'BASEPAGENAME'           ),
	'basepagenamee'         => array( 1,    'שם דף הבסיס מקודד',                      'BASEPAGENAMEE'          ),
	'talkpagename'          => array( 1,    'שם דף השיחה',                           'TALKPAGENAME'           ),
	'talkpagenamee'         => array( 1,    'שם דף השיחה מקודד',                      'TALKPAGENAMEE'          ),
	'subjectpagename'       => array( 1,    'שם דף הנושא', 'שם הערך',                 'SUBJECTPAGENAME', 'ARTICLEPAGENAME' ),
	'subjectpagenamee'      => array( 1,    'שם דף הנושא מקודד', 'שם הערך מקודד',     'SUBJECTPAGENAMEE', 'ARTICLEPAGENAMEE' ),
	'msg'                   => array( 0,    'הכללה:',                                'MSG:'                   ),
	'subst'                 => array( 0,    'ס:',                                    'SUBST:'                 ),
	'msgnw'                 => array( 0,    'הכללת מקור',                            'MSGNW:'                 ),
	'img_thumbnail'         => array( 1,    'ממוזער',                                'thumbnail', 'thumb'     ),
	'img_manualthumb'       => array( 1,    'ממוזער=$1',                             'thumbnail=$1', 'thumb=$1'),
	'img_right'             => array( 1,    'ימין',                                  'right'                  ),
	'img_left'              => array( 1,    'שמאל',                                 'left'                   ),
	'img_none'              => array( 1,    'ללא',                                  'none'                   ),
	'img_width'             => array( 1,    '$1px',                                 '$1px'                   ),
	'img_center'            => array( 1,    'מרכז',                                 'center', 'centre'       ),
	'img_framed'            => array( 1,    'ממוסגר', 'מסגרת',                      'framed', 'enframed', 'frame' ),
	'img_frameless'         => array( 1,    'לא ממוסגר', 'ללא מסגרת',              'frameless'              ),
	'img_page'              => array( 1,    'דף=$1', 'דף $1',                       'page=$1', 'page $1'     ),
	'img_upright'           => array( 1,    'ימין למעלה', 'ימין למעלה=$1', 'ימין למעלה $1', 'upright', 'upright=$1', 'upright $1' ),
	'img_border'            => array( 1,    'גבולות', 'גבול',                       'border'                 ),
	'img_baseline'          => array( 1,    'שורת הבסיס',                           'baseline'               ),
	'img_sub'               => array( 1,    'תחתי',                                 'sub'                    ),
	'img_super'             => array( 1,    'עילי',                                 'super', 'sup'           ),
	'img_top'               => array( 1,    'למעלה',                                'top'                    ),
	'img_text_top'          => array( 1,    'בראש הטקסט',                           'text-top'               ),
	'img_middle'            => array( 1,    'באמצע',                                'middle'                 ),
	'img_bottom'            => array( 1,    'למטה',                                 'bottom'                 ),
	'img_text_bottom'       => array( 1,    'בתחתית הטקסט',                         'text-bottom'            ),
	'int'                   => array( 0,    'הודעה:',                               'INT:'                   ),
	'sitename'              => array( 1,    'שם האתר',                              'SITENAME'               ),
	'ns'                    => array( 0,    'מרחב שם:',                             'NS:'                    ),
	'localurl'              => array( 0,    'כתובת יחסית:',                         'LOCALURL:'              ),
	'localurle'             => array( 0,    'כתובת יחסית מקודד:',                   'LOCALURLE:'             ),
	'server'                => array( 0,    'כתובת השרת', 'שרת',                    'SERVER'                 ),
	'servername'            => array( 0,    'שם השרת',                              'SERVERNAME'             ),
	'scriptpath'            => array( 0,    'נתיב הקבצים',                          'SCRIPTPATH'             ),
	'grammar'               => array( 0,    'דקדוק:',                               'GRAMMAR:'               ),
	'notitleconvert'        => array( 0,    '__ללא_המרת_כותרת__',                  '__NOTITLECONVERT__', '__NOTC__'),
	'nocontentconvert'      => array( 0,    '__ללא_המרת_תוכן__',                   '__NOCONTENTCONVERT__', '__NOCC__'),
	'currentweek'           => array( 1,    'שבוע נוכחי',                           'CURRENTWEEK'            ),
	'currentdow'            => array( 1,    'מספר יום נוכחי',                       'CURRENTDOW'             ),
	'localweek'             => array( 1,    'שבוע מקומי',                           'LOCALWEEK'              ),
	'localdow'              => array( 1,    'מספר יום מקומי',                       'LOCALDOW'               ),
	'revisionid'            => array( 1,    'מזהה גרסה',                            'REVISIONID'             ),
	'revisionday'           => array( 1,    'יום גרסה',                             'REVISIONDAY'            ),
	'revisionday2'          => array( 1,    'יום גרסה 2',                           'REVISIONDAY2'           ),
	'revisionmonth'         => array( 1,    'חודש גרסה',                            'REVISIONMONTH'          ),
	'revisionyear'          => array( 1,    'שנת גרסה',                             'REVISIONYEAR'           ),
	'revisiontimestamp'     => array( 1,    'זמן גרסה',                             'REVISIONTIMESTAMP'      ),
	'plural'                => array( 0,    'רבים:',                                'PLURAL:'                ),
	'fullurl'               => array( 0,    'כתובת מלאה:',                          'FULLURL:'               ),
	'fullurle'              => array( 0,    'כתובת מלאה מקודד:',                    'FULLURLE:'              ),
	'lcfirst'               => array( 0,    'אות ראשונה קטנה:',                     'LCFIRST:'               ),
	'ucfirst'               => array( 0,    'אות ראשונה גדולה:',                    'UCFIRST:'               ),
	'lc'                    => array( 0,    'אותיות קטנות:',                        'LC:'                    ),
	'uc'                    => array( 0,    'אותיות גדולות:',                       'UC:'                    ),
	'raw'                   => array( 0,    'ללא עיבוד:',                          'RAW:'                   ),
	'displaytitle'          => array( 1,    'כותרת תצוגה',                         'DISPLAYTITLE'           ),
	'rawsuffix'             => array( 1,    'ללא פסיק',                            'R'                      ),
	'newsectionlink'        => array( 1,    '__יצירת_הערה__',                      '__NEWSECTIONLINK__'     ),
	'currentversion'        => array( 1,    'גרסה נוכחית',                         'CURRENTVERSION'         ),
	'urlencode'             => array( 0,    'נתיב מקודד:',                         'URLENCODE:'             ),
	'anchorencode'          => array( 0,    'עוגן מקודד:',                         'ANCHORENCODE'           ),
	'currenttimestamp'      => array( 1,    'זמן נוכחי',                           'CURRENTTIMESTAMP'       ),
	'localtimestamp'        => array( 1,    'זמן מקומי',                           'LOCALTIMESTAMP'         ),
	'directionmark'         => array( 1,    'סימן כיווניות',                       'DIRECTIONMARK', 'DIRMARK' ),
	'language'              => array( 0,    '#שפה:',                           '#LANGUAGE:'             ),
	'contentlanguage'       => array( 1,    'שפת תוכן',                         'CONTENTLANGUAGE', 'CONTENTLANG' ),
	'pagesinnamespace'      => array( 1,    'דפים במרחב השם:',                  'PAGESINNAMESPACE:', 'PAGESINNS:' ),
	'numberofadmins'        => array( 1,    'מספר מפעילים',                      'NUMBEROFADMINS'         ),
	'formatnum'             => array( 0,    'עיצוב מספר',                        'FORMATNUM'              ),
	'padleft'               => array( 0,    'ריפוד משמאל',                       'PADLEFT'                ),
	'padright'              => array( 0,    'ריפוד מימין',                         'PADRIGHT'               ),
	'special'               => array( 0,    'מיוחד',                             'special'                ),
	'defaultsort'           => array( 1,    'מיון רגיל:',                          'DEFAULTSORT:'           ),
	'filepath'              => array( 0,    'נתיב לקובץ:',                        'FILEPATH:'              ),
	'tag'                   => array( 0,    'תגית',                              'tag'                    ),
	'hiddencat'             => array( 1,    '__קטגוריה_מוסתרת__',                  '__HIDDENCAT__'          ),
	'pagesincategory'       => array( 1,    'דפים בקטגוריה',                       'PAGESINCATEGORY', 'PAGESINCAT' ),
	'pagesize'              => array( 1,    'גודל דף',                            'PAGESIZE'               ),
	'staticredirect'        => array( 1,    '__הפניה_קבועה__',                     '__STATICREDIRECT__'     ),
);

$specialPageAliases = array(
	'DoubleRedirects'           => array( 'הפניות_כפולות' ),
	'BrokenRedirects'           => array( 'הפניות_לא_תקינות', 'הפניות_שבורות' ),
	'Disambiguations'           => array( 'פירושונים', 'דפי_פירושונים' ),
	'Userlogin'                 => array( 'כניסה_לחשבון', 'כניסה', 'כניסה_/_הרשמה_לחשבון' ),
	'Userlogout'                => array( 'יציאה_מהחשבון', 'יציאה' ),
	'CreateAccount'             => array( 'הרשמה_לחשבון' ),
	'Preferences'               => array( 'העדפות', 'ההעדפות_שלי' ),
	'Watchlist'                 => array( 'רשימת_המעקב', 'רשימת_מעקב', 'רשימת_המעקב_שלי' ),
	'Recentchanges'             => array( 'שינויים_אחרונים' ),
	'Upload'                    => array( 'העלאה', 'העלאת_קובץ_לשרת' ),
	'Imagelist'                 => array( 'רשימת_תמונות', 'תמונות' ),
	'Newimages'                 => array( 'תמונות_חדשות', 'גלריית_תמונות_חדשות' ),
	'Listusers'                 => array( 'רשימת_משתמשים', 'משתמשים' ),
	'Listgrouprights'           => array( 'רשימת_הרשאות_לקבוצה' ),
	'Statistics'                => array( 'סטטיסטיקות' ),
	'Randompage'                => array( 'אקראי', 'דף_אקראי' ),
	'Lonelypages'               => array( 'דפים_יתומים' ),
	'Uncategorizedpages'        => array( 'דפים_חסרי_קטגוריה' ),
	'Uncategorizedcategories'   => array( 'קטגוריות_חסרות_קטגוריה' ),
	'Uncategorizedimages'       => array( 'תמונות_חסרות_קטגוריה' ),
	'Uncategorizedtemplates'    => array( 'תבניות_חסרות_קטגוריות' ),
	'Unusedcategories'          => array( 'קטגוריות_שאינן_בשימוש' ),
	'Unusedimages'              => array( 'תמונות_שאינן_בשימוש' ),
	'Wantedpages'               => array( 'דפים_מבוקשים' ),
	'Wantedcategories'          => array( 'קטגוריות_מבוקשות' ),
	'Missingfiles'              => array( 'קבצים_חסרים', 'תמונות_חסרות' ),
	'Mostlinked'                => array( 'הדפים_המקושרים_ביותר' ),
	'Mostlinkedcategories'      => array( 'הקטגוריות_המקושרות_ביותר' ),
	'Mostlinkedtemplates'       => array( 'התבניות_המקושרות_ביותר' ),
	'Mostcategories'            => array( 'הקטגוריות_הרבות_ביותר', 'הדפים_מרובי-הקטגוריות_ביותר' ),
	'Mostimages'                => array( 'התמונות_המקושרות_ביותר' ),
	'Mostrevisions'             => array( 'הגרסאות_הרבות_ביותר', 'הדפים_בעלי_מספר_העריכות_הגבוה_ביותר' ),
	'Fewestrevisions'           => array( 'הגרסאות_המעטות_ביותר', 'הדפים_בעלי_מספר_העריכות_הנמוך_ביותר' ),
	'Shortpages'                => array( 'דפים_קצרים' ),
	'Longpages'                 => array( 'דפים_ארוכים' ),
	'Newpages'                  => array( 'דפים_חדשים' ),
	'Ancientpages'              => array( 'דפים_מוזנחים' ),
	'Deadendpages'              => array( 'דפים_ללא_קישורים' ),
	'Protectedpages'            => array( 'דפים_מוגנים' ),
	'Protectedtitles'           => array( 'כותרות_מוגנות' ),
	'Allpages'                  => array( 'כל_הדפים' ),
	'Prefixindex'               => array( 'דפים_המתחילים_ב' ) ,
	'Ipblocklist'               => array( 'רשימת_חסומים', 'רשימת_משתמשים_חסומים', 'משתמשים_חסומים' ),
	'Specialpages'              => array( 'דפים_מיוחדים' ),
	'Contributions'             => array( 'תרומות', 'תרומות_המשתמש' ),
	'Emailuser'                 => array( 'שליחת_דואר_למשתמש' ),
	'Confirmemail'              => array( 'אימות_כתובת_דואר' ),
	'Whatlinkshere'             => array( 'דפים_המקושרים_לכאן' ),
	'Recentchangeslinked'       => array( 'שינויים_בדפים_המקושרים' ),
	'Movepage'                  => array( 'העברת_דף', 'העברה' ),
	'Blockme'                   => array( 'חסום_אותי' ),
	'Booksources'               => array( 'משאבי_ספרות', 'משאבי_ספרות_חיצוניים' ),
	'Categories'                => array( 'קטגוריות', 'רשימת_קטגוריות' ),
	'Export'                    => array( 'ייצוא', 'ייצוא_דפים' ),
	'Version'                   => array( 'גרסה', 'גרסת_התוכנה' ),
	'Allmessages'               => array( 'הודעות_המערכת' ),
	'Log'                       => array( 'יומנים' ),
	'Blockip'                   => array( 'חסימת_משתמש', 'חסימה' ),
	'Undelete'                  => array( 'צפייה_בדפים_מחוקים' ),
	'Import'                    => array( 'ייבוא', 'ייבוא_דפים' ),
	'Lockdb'                    => array( 'נעילת_בסיס_הנתונים' ),
	'Unlockdb'                  => array( 'שחרור_בסיס_הנתונים' ),
	'Userrights'                => array( 'ניהול_הרשאות_משתמש' ),
	'MIMEsearch'                => array( 'חיפוש_MIME' ),
	'FileDuplicateSearch'       => array( 'חיפוש_קבצים_כפולים' ),
	'Unwatchedpages'            => array( 'דפים_שאינם_במעקב' ),
	'Listredirects'             => array( 'רשימת_הפניות', 'הפניות' ),
	'Revisiondelete'            => array( 'מחיקת_ושחזור_גרסאות' ),
	'Unusedtemplates'           => array( 'תבניות_שאינן_בשימוש' ),
	'Randomredirect'            => array( 'הפניה_אקראית' ),
	'Mypage'                    => array( 'הדף_שלי', 'דף_המשתמש_שלי' ),
	'Mytalk'                    => array( 'השיחה_שלי', 'דף_השיחה_שלי' ),
	'Mycontributions'           => array( 'התרומות_שלי' ),
	'Listadmins'                => array( 'רשימת_מפעילים' ),
	'Listbots'                  => array( 'רשימת_בוטים' ),
	'Popularpages'              => array( 'דפים_פופולריים' ),
	'Search'                    => array( 'חיפוש' ),
	'Resetpass'                 => array( 'איפוס_סיסמה' ),
	'Withoutinterwiki'          => array( 'דפים_ללא_קישורי_שפה' ),
	'MergeHistory'              => array( 'מיזוג_גרסאות' ),
	'Filepath'                  => array( 'נתיב_לקובץ' ),
	'Invalidateemail'           => array( 'ביטול_דואר' ),
	'Blankpage'                 => array( 'דף_ריק' ),
);

$namespaceNames = array(
	NS_MEDIA          => 'מדיה',
	NS_SPECIAL        => 'מיוחד',
	NS_MAIN           => '',
	NS_TALK           => 'שיחה',
	NS_USER           => 'משתמש',
	NS_USER_TALK      => 'שיחת_משתמש',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK   => 'שיחת_$1',
	NS_IMAGE          => 'תמונה',
	NS_IMAGE_TALK     => 'שיחת_תמונה',
	NS_MEDIAWIKI      => 'מדיה_ויקי',
	NS_MEDIAWIKI_TALK => 'שיחת_מדיה_ויקי',
	NS_TEMPLATE       => 'תבנית',
	NS_TEMPLATE_TALK  => 'שיחת_תבנית',
	NS_HELP           => 'עזרה',
	NS_HELP_TALK      => 'שיחת_עזרה',
	NS_CATEGORY       => 'קטגוריה',
	NS_CATEGORY_TALK  => 'שיחת_קטגוריה',
);

$messages = array(
# User preference toggles
'tog-underline'               => 'סימון קישורים בקו תחתי:',
'tog-highlightbroken'         => 'סימון קישורים לדפים שלא נכתבו <a href="" class="new">כך</a> (או: כך<a href="" class="internal">?</a>).',
'tog-justify'                 => 'יישור פסקאות',
'tog-hideminor'               => 'הסתרת שינויים משניים ברשימת השינויים האחרונים',
'tog-extendwatchlist'         => 'הרחבת רשימת המעקב כך שתציג את כל השינויים המתאימים (אחרת: את השינוי האחרון בכל דף בלבד)',
'tog-usenewrc'                => 'רשימת שינויים אחרונים משופרת (JavaScript)',
'tog-numberheadings'          => 'מספור כותרות אוטומטי',
'tog-showtoolbar'             => 'הצגת סרגל העריכה',
'tog-editondblclick'          => 'עריכת דפים בלחיצה כפולה (JavaScript)',
'tog-editsection'             => 'עריכת פסקאות באמצעות קישורים מהצורה [עריכה]',
'tog-editsectiononrightclick' => 'עריכת פסקאות על ידי לחיצה ימנית על כותרות הפסקאות (JavaScript)',
'tog-showtoc'                 => 'הצגת תוכן עניינים (עבור דפים עם יותר מ־3 כותרות)',
'tog-rememberpassword'        => 'זכירת הכניסה שלי במחשב זה',
'tog-editwidth'               => 'תיבת העריכה ברוחב מלא',
'tog-watchcreations'          => 'מעקב אחרי דפים שיצרתי',
'tog-watchdefault'            => 'מעקב אחרי דפים שערכתי',
'tog-watchmoves'              => 'מעקב אחרי דפים שהעברתי',
'tog-watchdeletion'           => 'מעקב אחרי דפים שמחקתי',
'tog-minordefault'            => 'הגדרת כל פעולת עריכה כמשנית אם לא צוין אחרת',
'tog-previewontop'            => 'הצגת תצוגה מקדימה לפני תיבת העריכה (או: אחריה)',
'tog-previewonfirst'          => 'הצגת תצוגה מקדימה בעריכה ראשונה',
'tog-nocache'                 => 'ביטול משיכת דפים מזכרון המטמון שבשרת',
'tog-enotifwatchlistpages'    => 'שליחת דוא"ל אליך כאשר נעשה שינוי בדפים ברשימת המעקב שלך',
'tog-enotifusertalkpages'     => 'שליחת דוא"ל אליך כאשר נעשה שינוי בדף שיחת המשתמש שלך',
'tog-enotifminoredits'        => 'שליחת דוא"ל אליך גם על עריכות משניות של דפים',
'tog-enotifrevealaddr'        => 'חשיפת כתובת הדוא"ל שלך בהודעות דואר',
'tog-shownumberswatching'     => 'הצגת מספר המשתמשים העוקבים אחרי הדף',
'tog-fancysig'                => 'הצגת חתימה מסוגננת',
'tog-externaleditor'          => 'שימוש בעורך חיצוני כברירת מחדל (למשתמשים מומחים בלבד, דורש הגדרות מיוחדות במחשב)',
'tog-externaldiff'            => 'שימוש בתוכנת השוואת הגרסאות החיצונית כברירת מחדל (למשתמשים מומחים בלבד, דורש הגדרות מיוחדות במחשב)',
'tog-showjumplinks'           => 'הצגת קישורי נגישות מסוג "קפוץ אל"',
'tog-uselivepreview'          => 'שימוש בתצוגה מקדימה מהירה (JavaScript) (ניסיוני)',
'tog-forceeditsummary'        => 'הצגת אזהרה כשאני מכניס תקציר עריכה ריק',
'tog-watchlisthideown'        => 'הסתרת עריכות שלי ברשימת המעקב',
'tog-watchlisthidebots'       => 'הסתרת בוטים ברשימת המעקב',
'tog-watchlisthideminor'      => 'הסתרת עריכות משניות ברשימת המעקב',
'tog-nolangconversion'        => 'ביטול המרת גרסאות שפה',
'tog-ccmeonemails'            => 'קבלת העתקים של הודעות דוא"ל הנשלחות ממני למשתמשים אחרים',
'tog-diffonly'                => 'ביטול הצגת תוכן הדף מתחת להשוואות הגרסאות',
'tog-showhiddencats'          => 'הצגת קטגוריות מוסתרות',

'underline-always'  => 'תמיד',
'underline-never'   => 'אף פעם',
'underline-default' => 'ברירת מחדל של הדפדפן',

'skinpreview' => '(תצוגה מקדימה)',

# Dates
'sunday'        => 'ראשון',
'monday'        => 'שני',
'tuesday'       => 'שלישי',
'wednesday'     => 'רביעי',
'thursday'      => 'חמישי',
'friday'        => 'שישי',
'saturday'      => 'שבת',
'sun'           => "ראש'",
'mon'           => 'שני',
'tue'           => "שלי'",
'wed'           => "רבי'",
'thu'           => "חמי'",
'fri'           => "שיש'",
'sat'           => 'שבת',
'january'       => 'ינואר',
'february'      => 'פברואר',
'march'         => 'מרץ',
'april'         => 'אפריל',
'may_long'      => 'מאי',
'june'          => 'יוני',
'july'          => 'יולי',
'august'        => 'אוגוסט',
'september'     => 'ספטמבר',
'october'       => 'אוקטובר',
'november'      => 'נובמבר',
'december'      => 'דצמבר',
'january-gen'   => 'בינואר',
'february-gen'  => 'בפברואר',
'march-gen'     => 'במרץ',
'april-gen'     => 'באפריל',
'may-gen'       => 'במאי',
'june-gen'      => 'ביוני',
'july-gen'      => 'ביולי',
'august-gen'    => 'באוגוסט',
'september-gen' => 'בספטמבר',
'october-gen'   => 'באוקטובר',
'november-gen'  => 'בנובמבר',
'december-gen'  => 'בדצמבר',
'jan'           => "ינו'",
'feb'           => "פבר'",
'mar'           => 'מרץ',
'apr'           => "אפר'",
'may'           => 'מאי',
'jun'           => 'יוני',
'jul'           => 'יולי',
'aug'           => "אוג'",
'sep'           => "ספט'",
'oct'           => "אוק'",
'nov'           => "נוב'",
'dec'           => "דצמ'",

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|קטגוריה|קטגוריות}}',
'category_header'                => 'דפים בקטגוריה "$1"',
'subcategories'                  => 'קטגוריות משנה',
'category-media-header'          => 'קובצי מדיה בקטגוריה "$1"',
'category-empty'                 => "'''קטגוריה זו אינה כוללת דפים או קובצי מדיה.'''",
'hidden-categories'              => '{{PLURAL:$1|קטגוריה מוסתרת|קטגוריות מוסתרות}}',
'hidden-category-category'       => 'קטגוריות מוסתרות', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|קטגוריה זו כוללת את קטגוריית המשנה הבאה בלבד|דף קטגוריה זה כולל את {{PLURAL:$1|קטגוריית המשנה הבאה|$1 קטגוריות המשנה הבאות}}, מתוך $2 בקטגוריה כולה}}.',
'category-subcat-count-limited'  => 'קטגוריה זו כוללת את {{PLURAL:$1|קטגוריית המשנה הבאה|$1 קטגוריות המשנה הבאות}}.',
'category-article-count'         => '{{PLURAL:$2|קטגוריה זו כוללת את הדף הבא בלבד|דף קטגוריה זה כולל את {{PLURAL:$1|הדף הבא|$1 הדפים הבאים}}, מתוך $2 בקטגוריה כולה}}.',
'category-article-count-limited' => 'קטגוריה זו כוללת את {{PLURAL:$1|הדף הבא|$1 הדפים הבאים}}.',
'category-file-count'            => '{{PLURAL:$2|קטגוריה זו כוללת את הקובץ הבא בלבד|דף קטגוריה זה כולל את {{PLURAL:$1|הקובץ הבא|$1 הקבצים הבאים}}, מתוך $2 בקטגוריה כולה}}.',
'category-file-count-limited'    => 'קטגוריה זו כוללת את {{PLURAL:$1|הקובץ הבא|$1 הקבצים הבאים}}.',
'listingcontinuesabbrev'         => '(המשך)',

'mainpagetext'      => "'''תוכנת מדיה־ויקי הותקנה בהצלחה.'''",
'mainpagedocfooter' => 'היעזרו ב[http://meta.wikimedia.org/wiki/Help:Contents מדריך למשתמש] למידע על שימוש בתוכנת הוויקי.

== קישורים שימושיים ==
* [http://www.mediawiki.org/wiki/Manual:Configuration_settings רשימת ההגדרות]
* [http://www.mediawiki.org/wiki/Manual:FAQ שאלות נפוצות]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce רשימת התפוצה על השקת גרסאות]',

'about'          => 'אודות',
'article'        => 'דף תוכן',
'newwindow'      => '(נפתח בחלון חדש)',
'cancel'         => 'ביטול / יציאה',
'qbfind'         => 'חיפוש',
'qbbrowse'       => 'דפדוף',
'qbedit'         => 'עריכה',
'qbpageoptions'  => 'אפשרויות דף',
'qbpageinfo'     => 'מידע על הדף',
'qbmyoptions'    => 'האפשרויות שלי',
'qbspecialpages' => 'דפים מיוחדים',
'moredotdotdot'  => 'עוד…',
'mypage'         => 'הדף שלי',
'mytalk'         => 'דף השיחה שלי',
'anontalk'       => 'השיחה עבור IP זה',
'navigation'     => 'ניווט',
'and'            => 'וגם',

# Metadata in edit box
'metadata_help' => 'מטא־דטה:',

'errorpagetitle'    => 'שגיאה',
'returnto'          => 'חזרה לדף $1.',
'tagline'           => 'מתוך {{SITENAME}}',
'help'              => 'עזרה',
'search'            => 'חיפוש',
'searchbutton'      => 'חיפוש',
'go'                => 'הצגה',
'searcharticle'     => 'לדף',
'history'           => 'היסטוריית הדף',
'history_short'     => 'היסטוריית הדף',
'updatedmarker'     => 'עודכן מאז ביקורך האחרון',
'info_short'        => 'מידע',
'printableversion'  => 'גרסת הדפסה',
'permalink'         => 'קישור קבוע',
'print'             => 'גרסה להדפסה',
'edit'              => 'עריכה',
'create'            => 'יצירה',
'editthispage'      => 'עריכת דף זה',
'create-this-page'  => 'יצירת דף זה',
'delete'            => 'מחיקה',
'deletethispage'    => 'מחיקת דף זה',
'undelete_short'    => 'שחזור {{PLURAL:$1|עריכה אחת|$1 עריכות}}',
'protect'           => 'הגנה',
'protect_change'    => 'שינוי',
'protectthispage'   => 'הפעלת הגנה על דף זה',
'unprotect'         => 'הסרת הגנה',
'unprotectthispage' => 'הסרת הגנה מדף זה',
'newpage'           => 'דף חדש',
'talkpage'          => 'שיחה על דף זה',
'talkpagelinktext'  => 'שיחה',
'specialpage'       => 'דף מיוחד',
'personaltools'     => 'כלים אישיים',
'postcomment'       => 'הוספת פסקה לדף השיחה',
'articlepage'       => 'צפייה בדף התוכן',
'talk'              => 'שיחה',
'views'             => 'צפיות',
'toolbox'           => 'תיבת כלים',
'userpage'          => 'צפייה בדף המשתמש',
'projectpage'       => 'צפייה בדף המיזם',
'imagepage'         => 'צפייה בדף התמונה',
'mediawikipage'     => 'צפייה בדף ההודעה',
'templatepage'      => 'צפייה בדף התבנית',
'viewhelppage'      => 'צפייה בדף העזרה',
'categorypage'      => 'צפייה בדף הקטגוריה',
'viewtalkpage'      => 'צפייה בדף השיחה',
'otherlanguages'    => 'שפות אחרות',
'redirectedfrom'    => '(הופנה מהדף $1)',
'redirectpagesub'   => 'דף הפניה',
'lastmodifiedat'    => 'שונה לאחרונה ב־$2, $1.', # $1 date, $2 time
'viewcount'         => 'דף זה נצפה {{PLURAL:$1|פעם אחת|$1 פעמים|פעמיים}}.',
'protectedpage'     => 'דף מוגן',
'jumpto'            => 'קפיצה אל:',
'jumptonavigation'  => 'ניווט',
'jumptosearch'      => 'חיפוש',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'אודות {{SITENAME}}',
'aboutpage'            => 'Project:אודות',
'bugreports'           => 'דיווח על באגים',
'bugreportspage'       => 'Project:דיווח על באגים',
'copyright'            => 'התוכן מוגש בכפוף ל־$1.<br /> בעלי זכויות היוצרים מפורטים בהיסטוריית השינויים של הדף.',
'copyrightpagename'    => 'זכויות היוצרים של {{SITENAME}}',
'copyrightpage'        => '{{ns:project}}:זכויות יוצרים',
'currentevents'        => 'אקטואליה',
'currentevents-url'    => 'Project:אקטואליה',
'disclaimers'          => 'הבהרה משפטית',
'disclaimerpage'       => 'Project:הבהרה משפטית',
'edithelp'             => 'עזרה לעריכה',
'edithelppage'         => 'Help:עריכת דף',
'faq'                  => 'שאלות ותשובות',
'faqpage'              => 'Project:שאלות ותשובות',
'helppage'             => 'Help:תפריט ראשי',
'mainpage'             => 'עמוד ראשי',
'mainpage-description' => 'עמוד ראשי',
'policy-url'           => 'Project:נהלים',
'portal'               => 'שער הקהילה',
'portal-url'           => 'Project:שער הקהילה',
'privacy'              => 'מדיניות הפרטיות',
'privacypage'          => 'Project:מדיניות הפרטיות',

'badaccess'        => 'שגיאה בהרשאות',
'badaccess-group0' => 'אינכם מורשים לבצע את הפעולה שביקשתם.',
'badaccess-group1' => 'הפעולה שביקשתם לבצע מוגבלת למשתמשים בקבוצה $1.',
'badaccess-group2' => 'הפעולה שביקשתם לבצע מוגבלת למשתמשים באחת הקבוצות $1.',
'badaccess-groups' => 'הפעולה שביקשתם לבצע מוגבלת למשתמשים באחת הקבוצות $1.',

'versionrequired'     => 'נדרשת גרסה $1 של מדיה־ויקי',
'versionrequiredtext' => 'גרסה $1 של מדיה־ויקי נדרשת לשימוש בדף זה. למידע נוסף, ראו את [[Special:Version|דף הגרסה]].',

'ok'                      => 'אישור',
'pagetitle'               => '$1 – {{SITENAME}}',
'retrievedfrom'           => '<br /><span style="font-size: smaller;">מקור: $1</span>',
'youhavenewmessages'      => 'יש לך $1 ($2).',
'newmessageslink'         => 'הודעות חדשות',
'newmessagesdifflink'     => 'השוואה לגרסה הקודמת',
'youhavenewmessagesmulti' => 'יש לך הודעות חדשות ב־$1',
'editsection'             => 'עריכה',
'editold'                 => 'עריכה',
'viewsourceold'           => 'הצגת מקור',
'editsectionhint'         => 'עריכת פסקה: $1',
'toc'                     => 'תוכן עניינים',
'showtoc'                 => 'הצגה',
'hidetoc'                 => 'הסתרה',
'thisisdeleted'           => 'שחזור או הצגת $1?',
'viewdeleted'             => 'הצגת $1?',
'restorelink'             => '{{PLURAL:$1|גרסה מחוקה אחת|$1 גרסאות מחוקות}}',
'feedlinks'               => 'הזנה:',
'feed-invalid'            => 'סוג הזנת המנוי שגוי.',
'feed-unavailable'        => 'הזנות אינן זמינות',
'site-rss-feed'           => 'RSS של $1',
'site-atom-feed'          => 'Atom של $1',
'page-rss-feed'           => 'RSS של $1',
'page-atom-feed'          => 'Atom של $1',
'red-link-title'          => '$1 (טרם נכתב)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'דף תוכן',
'nstab-user'      => 'דף משתמש',
'nstab-media'     => 'מדיה',
'nstab-special'   => 'מיוחד',
'nstab-project'   => 'דף מיזם',
'nstab-image'     => 'תמונה',
'nstab-mediawiki' => 'הודעה',
'nstab-template'  => 'תבנית',
'nstab-help'      => 'דף עזרה',
'nstab-category'  => 'קטגוריה',

# Main script and global functions
'nosuchaction'      => 'אין פעולה כזו',
'nosuchactiontext'  => 'מערכת מדיה־ויקי אינה מכירה את הפעולה המצויינת בכתובת ה־URL של הדף.',
'nosuchspecialpage' => 'אין דף מיוחד בשם זה',
'nospecialpagetext' => '<big>ביקשתם דף מיוחד שאינו קיים.</big>

ראו גם את [[Special:SpecialPages|רשימת הדפים המיוחדים התקינים]].',

# General errors
'error'                => 'שגיאה',
'databaseerror'        => 'שגיאת בסיס נתונים',
'dberrortext'          => '<p><b>ארעה שגיאת תחביר בשאילתה לבסיס הנתונים</b>.</p>
<p>שגיאה זו יכולה להיות תוצאה של שאילתת חיפוש בלתי חוקית, או שהיא עלולה להעיד על באג במערכת מדיה־ויקי.</p>
<table class="toccolours">
<tr>
<th colspan="2" style="background-color: #F8F8F8; text-align: center;">מידע על השגיאה</th>
</tr>
<tr>
<td>השאילתה האחרונה שבוצעה לבסיס הנתונים הייתה:</td>
<td style="direction: ltr;">$1</td>
</tr>
<tr>
<td>הפונקציה הקוראת הייתה:</td>
<td style="direction: ltr;">$2</td>
</tr>
<tr>
<td>הודעת השגיאה שהוחזרה על ידי בסיס הנתונים:</td>
<td style="direction: ltr;">$3: $4</td>
</tr>
</table>',
'dberrortextcl'        => '<p><b>ארעה שגיאת תחביר בשאילתה לבסיס הנתונים</b>.</p>
<table class="toccolours">
<tr>
<th colspan="2" style="background-color: #F8F8F8; text-align: center;">מידע על השגיאה</th>
</tr>
<tr>
<td>השאילתה האחרונה שבוצעה לבסיס הנתונים הייתה:</td>
<td style="direction: ltr;">$1</td>
</tr>
<tr>
<td>הפונקציה הקוראת הייתה:</td>
<td style="direction: ltr;">$2</td>
</tr>
<tr>
<td>הודעת השגיאה שהוחזרה על ידי בסיס הנתונים:</td>
<td style="direction: ltr;">$3: $4</td>
</tr>
</table>',
'noconnect'            => 'עקב בעיות טכניות באתר, לא ניתן להתחבר לבסיס הנתונים.<br />
$1',
'nodb'                 => 'לא ניתן לבחור את בסיס הנתונים $1',
'cachederror'          => 'להלן מוצג עותק גיבוי (Cache), שכנראה איננו עדכני, של הדף המבוקש.',
'laggedslavemode'      => 'אזהרה: הדף עשוי שלא להכיל עדכונים אחרונים.',
'readonly'             => 'בסיס הנתונים נעול',
'enterlockreason'      => 'הזינו סיבה לנעילת בסיס הנתונים, כולל הערכה לגבי מועד שחרור הנעילה.',
'readonlytext'         => 'בסיס נתונים זה של האתר נעול ברגע זה לצורך הזנת נתונים ושינויים. ככל הנראה מדובר בתחזוקה שוטפת, שלאחריה יחזור האתר לפעולתו הרגילה.

מנהל המערכת שנעל את בסיס הנתונים סיפק את ההסבר הבא: $1',
'missing-article'      => 'בסיס הנתונים לא מצא את הטקסט של הדף שהוא היה אמור למצוא, בשם "$1" $2.

הדבר נגרם בדרך כלל על ידי קישור ישן להשוואת גרסאות של דף שנמחק או לגרסה של דף כזה.

אם זה אינו המקרה, זהו כנראה באג בתוכנה.
אנא דווחו על כך למנהלי המערכת תוך שמירת פרטי כתובת ה־URL.',
'missingarticle-rev'   => '(מספר גרסה: $1)',
'missingarticle-diff'  => '(השוואת הגרסאות: $1, $2)',
'readonly_lag'         => 'בסיס הנתונים ננעל אוטומטית כדי לאפשר לבסיסי הנתונים המשניים להתעדכן מהבסיס הראשי.',
'internalerror'        => 'שגיאה פנימית',
'internalerror_info'   => 'שגיאה פנימית: $1',
'filecopyerror'        => 'העתקת "$1" ל־"$2" נכשלה.',
'filerenameerror'      => 'שינוי השם של "$1" ל־"$2" נכשל.',
'filedeleteerror'      => 'מחיקת "$1" נכשלה.',
'directorycreateerror' => 'יצירת התיקייה "$1" נכשלה.',
'filenotfound'         => 'הקובץ "$1" לא נמצא.',
'fileexistserror'      => 'הכתיבה לקובץ "$1" נכשלה: הקובץ קיים',
'unexpected'           => 'ערך לא צפוי: "$1"="$2"',
'formerror'            => 'שגיאה: לא יכול לשלוח טופס.',
'badarticleerror'      => 'לא ניתן לבצע פעולה זו בדף זה.',
'cannotdelete'         => 'מחיקת הדף או התמונה לא הצליחה. ייתכן שהוא נמחק כבר על ידי מישהו אחר.',
'badtitle'             => 'כותרת שגויה',
'badtitletext'         => 'כותרת הדף המבוקש הייתה לא־חוקית, ריקה, קישור ויקי פנימי, או פנים שפה שגוי.',
'perfdisabled'         => 'שירות זה הופסק זמנית בכדי לא לפגוע בביצועי המערכת.',
'perfcached'           => 'המידע הבא הוא עותק שמור של המידע, ועשוי שלא להיות מעודכן.',
'perfcachedts'         => 'המידע הבא הוא עותק שמור של המידע, שעודכן לאחרונה ב־$1.',
'querypage-no-updates' => 'העדכונים לדף זה כרגע מופסקים, והמידע לא יעודכן באופן שוטף.',
'wrong_wfQuery_params' => 'הפרמטרים שהוזנו ל־wfQuery() אינם נכונים:<br />
פונקציה: $1<br />
שאילתה: $2',
'viewsource'           => 'הצגת מקור',
'viewsourcefor'        => 'לדף $1',
'actionthrottled'      => 'הפעולה הוגבלה',
'actionthrottledtext'  => 'כאמצעי נגד ספאם, אינכם מורשים לבצע פעולה זו פעמים רבות מדי בזמן קצר. אנא נסו שנית בעוד מספר דקות.',
'protectedpagetext'    => 'דף זה נעול לעריכה.',
'viewsourcetext'       => 'באפשרותכם לצפות בטקסט המקור של הדף, ואף להעתיקו:',
'protectedinterface'   => 'דף זה הוא אחד מסדרת דפים המספקים הודעות מערכת לתוכנה, ונעול לעריכה למפעילי מערכת בלבד כדי למנוע השחתות של ההודעות.',
'editinginterface'     => "'''אזהרה:''' דף זה הוא אחד מסדרת דפים המספקים הודעות מערכת לתוכנה. שינויים בדף זה ישנו את הודעת המערכת לכל המשתמשים האחרים.",
'sqlhidden'            => '(שאילתת ה־SQL מוסתרת)',
'cascadeprotected'     => 'דף זה נעול לעריכה כיוון שהוא מוכלל {{PLURAL:$1|בדף הבא, שמופעלת אצלו|בדפים הבאים, שמופעלת אצלם}} הגנה מדורגת:
$2',
'namespaceprotected'   => "אינכם מורשים לערוך דפים במרחב השם '''$1'''.",
'customcssjsprotected' => 'אינכם מורשים לערוך דף זה, כיוון שהוא כולל את ההגדרות האישיות של משתמש אחר.',
'ns-specialprotected'  => 'לא ניתן לערוך דפים מיוחדים.',
'titleprotected'       => 'לא ניתן ליצור דף זה, כיוון שהמשתמש [[User:$1|$1]] הגן עליו מפני יצירה בנימוק "$2".',

# Virus scanner
'virus-badscanner'     => 'הגדרות שגויות: סורק הווירוסים אינו ידוע: <i>$1</i>',
'virus-scanfailed'     => 'הסריקה נכשלה (קוד: $1)',
'virus-unknownscanner' => 'אנטי־וירוס בלתי ידוע:',

# Login and logout pages
'logouttitle'                => 'יציאה מהחשבון',
'logouttext'                 => 'יצאתם זה עתה מהחשבון. באפשרותכם להמשיך ולעשות שימוש ב{{grammar:תחילית|{{SITENAME}}}} באופן אנונימי, או [[Special:UserLogin|לשוב ולהיכנס לאתר]] עם שם משתמש זהה או אחר.',
'welcomecreation'            => '== ברוך הבא, $1! ==
חשבונך נפתח. אל תשכח להתאים את [[Special:Preferences|העדפות המשתמש]] שלך.',
'loginpagetitle'             => 'כניסת משתמש',
'yourname'                   => 'שם משתמש:',
'yourpassword'               => 'סיסמה:',
'yourpasswordagain'          => 'הקש סיסמה שנית:',
'remembermypassword'         => 'זכירת הכניסה במחשב זה',
'yourdomainname'             => 'התחום שלך:',
'externaldberror'            => 'הייתה שגיאה בבסיס הנתונים של ההזדהות, או שאינכם רשאים לעדכן את חשבונכם החיצוני.',
'loginproblem'               => "'''אירעה שגיאה בכניסה לאתר.'''<br />אנא נסו שנית.",
'login'                      => 'כניסה לחשבון',
'nav-login-createaccount'    => 'כניסה לחשבון / הרשמה',
'loginprompt'                => 'לפני הכניסה לחשבון ב{{grammar:תחילית|{{SITENAME}}}}, עליכם לוודא כי ה"עוגיות" (Cookies) מופעלות.',
'userlogin'                  => 'כניסה / הרשמה לחשבון',
'logout'                     => 'יציאה מהחשבון',
'userlogout'                 => 'יציאה מהחשבון',
'notloggedin'                => 'לא בחשבון',
'nologin'                    => 'אין לכם חשבון? $1.',
'nologinlink'                => 'אתם מוזמנים להירשם',
'createaccount'              => 'יצירת משתמש חדש',
'gotaccount'                 => 'כבר נרשמתם? $1.',
'gotaccountlink'             => 'הכנסו לחשבון',
'createaccountmail'          => 'באמצעות דוא"ל',
'badretype'                  => 'הסיסמאות שהזנתם אינן מתאימות.',
'userexists'                 => 'שם המשתמש שבחרתם נמצא בשימוש.
אנא בחרו שם אחר.',
'youremail'                  => 'דואר אלקטרוני:',
'username'                   => 'שם משתמש:',
'uid'                        => 'מספר סידורי:',
'prefs-memberingroups'       => 'חבר ב{{PLURAL:$1|קבוצה|קבוצות}}:',
'yourrealname'               => 'שם אמיתי:',
'yourlanguage'               => 'שפת הממשק:',
'yourvariant'                => 'גרסה:',
'yournick'                   => 'חתימה:',
'badsig'                     => 'חתימה מסוגננת שגויה.
אנא בדקו את תגיות ה־HTML.',
'badsiglength'               => 'החתימה ארוכה מדי.
היא חייבת להיות קצרה מ־{{PLURAL:$1|תו אחד|$1 תווים}}.',
'email'                      => 'דוא"ל',
'prefs-help-realname'        => 'השם האמיתי הוא אופציונאלי.
אם תבחרו לספקו, הוא ישמש לייחוס עבודתכם אליכם.',
'loginerror'                 => 'שגיאה בכניסה לאתר',
'prefs-help-email'           => 'כתובת דואר אלקטרוני היא אופציונאלית, אך אם תבחרו לספקה, היא תאפשר לכם לקבל סיסמה חדשה בדואר האלקטרוני אם תשכחו את הסיסמה.
תוכלו גם לבחור לאפשר לאחרים לשלוח לכם מסר דרך דף המשתמש או דף השיחה שלכם ללא צורך לחשוף את כתובתכם.',
'prefs-help-email-required'  => 'כתובת דואר אלקטרוני נדרשת לכתיבה באתר.',
'nocookiesnew'               => 'נוצר חשבון המשתמש שלכם, אך לא נכנסתם כמשתמשים רשומים למערכת כיוון שניטרלתם את העוגיות, ש{{grammar:תחילית|{{SITENAME}}}} משתמש בהן לצורך כניסה למערכת. אנא הפעילו אותן מחדש, ולאחר מכן תוכלו להיכנס למערכת עם שם המשתמש והסיסמה החדשים שלכם.',
'nocookieslogin'             => 'לא הצלחתם להיכנס למערכת כמשתמשים רשומים כיוון שניטרלתם את העוגיות, ש{{grammar:תחילית|{{SITENAME}}}} משתמש בהן לצורך כניסה למערכת. אנא הפעילו אותן מחדש, ולאחר מכן תוכלו להיכנס למערכת עם שם המשתמש והסיסמה שלכם.',
'noname'                     => 'לא הזנתם שם משתמש חוקי',
'loginsuccesstitle'          => 'הכניסה הושלמה בהצלחה',
'loginsuccess'               => "'''נכנסת ל{{grammar:תחילית|{{SITENAME}}}} בשם \"\$1\".'''",
'nosuchuser'                 => 'אין משתמש בשם "$1".
אנא ודאו שהאיות נכון, או [[Special:Userlogin/signup|צרו חשבון חדש]].',
'nosuchusershort'            => 'אין משתמש בשם "<nowiki>$1</nowiki>". אנא ודאו שהאיות נכון.',
'nouserspecified'            => 'עליכם לציין שם משתמש.',
'wrongpassword'              => 'הסיסמה שהקלדתם שגויה, אנא נסו שנית.',
'wrongpasswordempty'         => 'הסיסמה שהקלדתם ריקה. אנא נסו שנית.',
'passwordtooshort'           => 'סיסמתכם בלתי תקינה או קצרה מדי. עליה להיות מורכבת מ{{PLURAL:$1|תו אחד|־$1 תווים}} לפחות ושונה משם המשתמש.',
'mailmypassword'             => 'שלחו לי סיסמה חדשה',
'passwordremindertitle'      => 'סיסמה זמנית חדשה מ{{grammar:תחילית|{{SITENAME}}}}',
'passwordremindertext'       => 'מישהו (ככל הנראה אתם, מכתובת ה־IP מספר $1) ביקש סיסמה
חדשה לכניסה לחשבון ב{{grammar:תחילית|{{SITENAME}}}} ($4). נוצרה סיסמה זמנית למשתמש "$2",
וסיסמה זו היא "$3". אם זו הייתה כוונתכם, תוכלו כעת להיכנס לחשבון ולבחור סיסמה חדשה.

עליכם להיכנס לאתר ולשנות את סיסמתכם בהקדם האפשרי. אם מישהו אחר ביקש סיסמה חדשה זו או אם נזכרתם בסיסמתכם
ואינכם רוצים עוד לשנות אותה, באפשרותכם להתעלם מהודעה זו ולהמשיך להשתמש בסיסמתכם הישנה.',
'noemail'                    => 'לא רשומה כתובת דואר אלקטרוני עבור משתמש  "$1".',
'passwordsent'               => 'סיסמה חדשה נשלחה לכתובת הדואר האלקטרוני הרשומה עבור "$1".
אנא הכנסו חזרה לאתר אחרי שתקבלו אותה.',
'blocked-mailpassword'       => 'כתובת ה־IP שלכם חסומה מעריכה, ולפיכך אינכם מורשים להשתמש באפשרות שחזור הסיסמה כדי למנוע ניצול לרעה של התכונה.',
'eauthentsent'               => 'דוא"ל אישור נשלח לכתובת הדוא"ל שקבעת. לפני שדברי דוא"ל אחרים נשלחים לחשבון הזה, תצטרך לפעול לפי ההוראות בדוא"ל כדי לוודא שהדוא"ל הוא אכן שלך.',
'throttled-mailpassword'     => 'כבר נעשה שימוש באפשרות שחזור הסיסמה ב{{PLURAL:$1|שעה האחרונה|־$1 השעות האחרונות}}. כדי למנוע ניצול לרעה, רק דואר אחד כזה יכול להישלח כל {{PLURAL:$1|שעה אחת|$1 שעות}}.',
'mailerror'                  => 'שגיאה בשליחת דואר: $1',
'acct_creation_throttle_hit' => 'מצטערים, יצרתם כבר $1 חשבונות. אינכם יכולים ליצור חשבונות נוספים.',
'emailauthenticated'         => 'כתובת הדוא"ל שלך אושרה ב־$1.',
'emailnotauthenticated'      => 'כתובת הדוא"ל שלכם <strong>עדיין לא אושרה</strong> - שירותי הדוא"ל הבאים אינם פעילים.',
'noemailprefs'               => 'אנא ציינו כתובת דוא"ל כדי שתכונות אלה יעבדו.',
'emailconfirmlink'           => 'אישור כתובת הדוא"ל שלך',
'invalidemailaddress'        => 'כתובת הדוא"ל אינה מתקבלת כיוון שנראה שהיא בפורמט לא נכון.
אנא הקלידו כתובת תקינה או השאירו את השדה ריק.',
'accountcreated'             => 'החשבון נוצר',
'accountcreatedtext'         => 'חשבון המשתמש $1 נוצר.',
'createaccount-title'        => 'יצירת חשבון ב{{grammar:תחילית|{{SITENAME}}}}',
'createaccount-text'         => 'מישהו יצר חשבון בשם $2 ב{{grammar:תחילית|{{SITENAME}}}} ($4), והסיסמה הזמנית של החשבון היא "$3". עליכם להיכנס ולשנות עכשיו את הסיסמה.

באפשרותכם להתעלם מהודעה זו, אם החשבון נוצר בטעות.',
'loginlanguagelabel'         => 'שפה: $1',

# Password reset dialog
'resetpass'               => 'איפוס סיסמת החשבון',
'resetpass_announce'      => 'נכנסתם באמצעות סיסמה זמנית שנשלחה אליכם בדוא"ל. כדי לסיים את הכניסה, עליכם לקבוע כאן סיסמה חדשה:',
'resetpass_text'          => '<!-- הוסיפו טקסט כאן -->',
'resetpass_header'        => 'איפוס הסיסמה',
'resetpass_submit'        => 'הגדרת הסיסמה וכניסה',
'resetpass_success'       => 'סיסמתכם שונתה בהצלחה! מכניס אתכם למערכת…',
'resetpass_bad_temporary' => 'סיסמה זמנית שגויה. ייתכן שכבר שיניתם בהצלחה את סיסמתכם; אם לא, אנא בקשו סיסמה זמנית חדשה.',
'resetpass_forbidden'     => 'לא ניתן לשנות סיסמאות.',
'resetpass_missing'       => 'חסר מידע בטופס.',

# Edit page toolbar
'bold_sample'     => 'טקסט מודגש',
'bold_tip'        => 'טקסט מודגש',
'italic_sample'   => 'טקסט נטוי',
'italic_tip'      => 'טקסט נטוי (לא מומלץ בעברית)',
'link_sample'     => 'קישור',
'link_tip'        => 'קישור פנימי',
'extlink_sample'  => 'http://www.example.com כותרת הקישור לתצוגה',
'extlink_tip'     => 'קישור חיצוני (כולל קידומת http מלאה)',
'headline_sample' => 'כותרת',
'headline_tip'    => 'כותרת – דרגה 2',
'math_sample'     => 'formula',
'math_tip'        => 'נוסחה מתמטית (LaTeX)',
'nowiki_sample'   => 'טקסט לא מעוצב',
'nowiki_tip'      => 'טקסט לא מעוצב (התעלם מסימני ויקי)',
'image_tip'       => 'תמונה (שכבר הועלתה לשרת)',
'media_tip'       => 'קישור לקובץ מדיה',
'sig_tip'         => 'חתימה + שעה',
'hr_tip'          => 'קו אופקי (השתדלו להמנע משימוש בקו)',

# Edit pages
'summary'                          => 'תקציר',
'subject'                          => 'נושא/כותרת',
'minoredit'                        => 'זהו שינוי משני',
'watchthis'                        => 'מעקב אחרי דף זה',
'savearticle'                      => 'שמירה',
'preview'                          => 'תצוגה מקדימה',
'showpreview'                      => 'תצוגה מקדימה',
'showlivepreview'                  => 'תצוגה מקדימה מהירה',
'showdiff'                         => 'הצגת שינויים',
'anoneditwarning'                  => "'''אזהרה:''' אינכם מחוברים לחשבון. כתובת ה־IP שלכם תירשם בהיסטוריית העריכות של הדף.",
'missingsummary'                   => "'''תזכורת:''' לא הזנתם תקציר עריכה. אם תלחצו שוב על כפתור השמירה, עריכתכם תישמר בלעדיו.",
'missingcommenttext'               => 'אנא הקלידו את ההודעה למטה.',
'missingcommentheader'             => "'''תזכורת:''' לא הזנתם נושא/כותרת להודעה זו. אם תלחצו שוב על כפתור השמירה, עריכתכם תישמר בלעדיו.",
'summary-preview'                  => 'תצוגה מקדימה של התקציר',
'subject-preview'                  => 'תצוגה מקדימה של הנושא/הכותרת',
'blockedtitle'                     => 'המשתמש חסום',
'blockedtext'                      => '<big>\'\'\'שם המשתמש או כתובת ה־IP שלכם נחסמו.\'\'\'</big>

החסימה בוצעה על ידי $1. הסיבה שניתנה לכך היא \'\'\'$2\'\'\'.

* תחילת החסימה: $8
* פקיעת החסימה: $6
* החסימה שבוצעה: $7

באפשרותכם ליצור קשר עם $1 או עם כל אחד מ[[{{MediaWiki:Grouppage-sysop}}|מפעילי המערכת]] האחרים כדי לדון על החסימה.
אינכם יכולים להשתמש בתכונת "שליחת דואר אלקטרוני למשתמש זה" אם לא ציינתם כתובת דוא"ל תקפה ב[[Special:Preferences|העדפות המשתמש שלכם]] או אם נחסמתם משליחת דוא"ל.
כתובת ה־IP שלכם היא $3, ומספר החסימה שלכם הוא #$5.
אנא ציינו את כל הפרטים הללו בכל פנייה למפעילי המערכת.',
'autoblockedtext'                  => 'כתובת ה־IP שלכם נחסמה באופן אוטומטי כיוון שמשתמש אחר, שנחסם על ידי $1, עשה בה שימוש.
הסיבה שניתנה לחסימה היא:

:\'\'\'$2\'\'\'

* תחילת החסימה: $8
* פקיעת החסימה: $6
* החסימה שבוצעה: $7

באפשרותכם ליצור קשר עם $1 או עם כל אחד מ[[{{MediaWiki:Grouppage-sysop}}|מפעילי המערכת]] האחרים כדי לדון על החסימה.
אינכם יכולים להשתמש בתכונת "שליחת דואר אלקטרוני למשתמש זה" אם לא ציינתם כתובת דוא"ל תקפה ב[[Special:Preferences|העדפות המשתמש שלכם]] או אם נחסמתם משליחת דוא"ל.
כתובת ה־IP שלכם היא $3, ומספר החסימה שלכם הוא #$5.
אנא ציינו את כל הפרטים הללו בכל פנייה למפעילי המערכת.',
'blockednoreason'                  => 'לא ניתנה סיבה',
'blockedoriginalsource'            => "טקסט המקור של '''$1''' מוצג למטה:",
'blockededitsource'                => "הטקסט של '''העריכות שלך''' לדף '''$1''' מוצג למטה:",
'whitelistedittitle'               => 'כניסה לחשבון נדרשת לעריכה',
'whitelistedittext'                => 'עליכם $1 כדי לערוך דפים.',
'confirmedittitle'                 => 'הנכם חייבים לאמת את כתובת הדוא"ל שלכם כדי לערוך',
'confirmedittext'                  => 'עליכם לאמת את כתובת הדוא"ל שלכם לפני שתוכלו לערוך דפים. אנא הגדירו ואמתו את כתובת הדוא"ל שלכם באמצעות [[Special:Preferences|העדפות המשתמש]] שלכם.',
'nosuchsectiontitle'               => 'אין פסקה כזו',
'nosuchsectiontext'                => 'ניסיתם לערוך פסקה שאינה קיימת. כיוון שאין פסקה בשם $1, אין מקום לשמור את עריכתכם.',
'loginreqtitle'                    => 'כניסה לחשבון נדרשת',
'loginreqlink'                     => 'להיכנס לחשבון',
'loginreqpagetext'                 => 'עליכם $1 כדי לצפות בדפים אחרים.',
'accmailtitle'                     => 'הסיסמה נשלחה',
'accmailtext'                      => 'הסיסמה עבור "$1" נשלחה אל $2.',
'newarticle'                       => '(חדש)',
'newarticletext'                   => "הגעתם לדף שעדיין איננו קיים. כדי ליצור דף חדש, כתבו את התוכן שלכם בתיבת הטקסט למטה.

אם הגעתם לכאן בטעות, לחצו על מקש ה־'''Back''' בדפדפן שלכם.",
'anontalkpagetext'                 => "----
'''זהו דף שיחה של משתמש אנונימי שעדיין לא יצר חשבון במערכת, או שהוא לא משתמש בו. כיוון שכך, אנו צריכים להשתמש בכתובת ה־IP כדי לזהותו. ייתכן שכתובת IP זו תייצג מספר משתמשים. אם אתם משתמשים אנונימיים ומרגישים שקיבלתם הודעות בלתי רלוונטיות, אנא [[Special:UserLogin|היכנסו לחשבון]] או [[Special:UserLogin/signup|הירשמו לאתר]] כדי להימנע מבלבול עתידי עם משתמשים אנונימיים נוספים.'''
----",
'noarticletext'                    => 'אין עדיין טקסט בדף זה. באפשרותכם [[Special:Search/{{PAGENAME}}|לחפש את {{PAGENAME}} באתר]], או [{{fullurl:{{FULLPAGENAME}}|action=edit}} ליצור דף זה].',
'userpage-userdoesnotexist'        => 'חשבון המשתמש "$1" אינו רשום. אנא בדקו אם ברצונכם ליצור/לערוך דף זה.',
'clearyourcache'                   => "'''הערה:''' לאחר השמירה, עליכם לנקות את זכרון המטמון (Cache) של הדפדפן על־מנת להבחין בשינויים.
* ב'''מוזילה''', '''פיירפוקס''' או '''ספארי''', לחצו על מקש ה־Shift בעת לחיצתכם על '''העלה מחדש''' (Reload), או הקישו Ctrl+Shift+R (או Cmd+Shift+R במקינטוש של אפל).
* ב'''אינטרנט אקספלורר''', לחצו על מקש ה־Ctrl בעת לחיצתכם על '''רענן''' (Refresh), או הקישו על Ctrl+F5.
* ב־'''Konqueror''', לחצו על '''העלה מחדש''' (Reload), או הקישו על F5.
* ב'''אופרה''', ייתכן שתצטרכו להשתמש ב'''כלים''' (Tools) > '''העדפות''' (Preferences) כדי לנקות לחלוטין את זכרון המטמון.",
'usercssjsyoucanpreview'           => "'''עצה:''' השתמשו בלחצן \"תצוגה מקדימה\" כדי לבחון את גליון ה־CSS או את סקריפט ה־JavaScript החדש שלכם לפני השמירה.",
'usercsspreview'                   => "'''זכרו שזו רק תצוגה מקדימה של גליון ה־CSS שלכם.'''
'''הוא טרם נשמר!'''",
'userjspreview'                    => "'''זכרו שזו רק בדיקה/תצוגה מקדימה של סקריפט ה־JavaScript שלכם.'''
'''הוא טרם נשמר!'''",
'userinvalidcssjstitle'            => "'''אזהרה''': הרקע \"\$1\" אינו קיים. זכרו שדפי CSS ו־JavaScript מותאמים אישית משתמשים בכותרת עם אותיות קטנות – למשל, {{ns:user}}:דוגמה/monobook.css ולא {{ns:user}}:דוגמה/Monobook.css. כמו כן, יש להקפיד על שימוש ב־/ ולא ב־\\.",
'updated'                          => '(מעודכן)',
'note'                             => '<strong>הערה:</strong>',
'previewnote'                      => '<strong>זכרו שזו רק תצוגה מקדימה, והדף עדיין לא נשמר!</strong>',
'previewconflict'                  => 'תצוגה מקדימה זו מציגה כיצד ייראה הטקסט בחלון העריכה העליון, אם תבחרו לשמור אותו.',
'session_fail_preview'             => '<strong>לא ניתן לבצע את עריכתכם עקב אובדן קשר עם השרת. אנא נסו שנית. אם זה לא עוזר, אנא [[Special:UserLogout|צאו מהחשבון]] ונסו שנית.</strong>',
'session_fail_preview_html'        => '<strong>לא ניתן לבצע את עריכתם עקב אובדן קשר עם השרת.</strong>

כיוון שבאתר זה אפשרות השימוש ב־HTML מאופשרת, התצוגה המקדימה מוסתרת כדי למנוע התקפות JavaScript.

<strong>אם זהו ניסיון עריכה לגיטימי, אנא נסו שנית. אם זה לא עוזר, נסו [[Special:UserLogout|לצאת מהחשבון]] ולהיכנס אליו שנית.</strong>',
'token_suffix_mismatch'            => '<strong>עריכתכם נדחתה כיוון שהדפדפן שלכם מחק את תווי הניקוד בסימון העריכה. העריכה נדחתה כדי למנוע בעיות כאלה בטקסט של הדף. ייתכן שזה קרה בגלל שירות פרוקסי אנונימי פגום.</strong>',
'editing'                          => 'עריכת $1',
'editingsection'                   => 'עריכת $1 (פסקה)',
'editingcomment'                   => 'עריכת $1 (הודעה)',
'editconflict'                     => 'התנגשות עריכה: $1',
'explainconflict'                  => "משתמש אחר שינה את הדף מאז שהתחלתם לערוך אותו. חלון העריכה העליון מכיל את הטקסט בדף כפי שהוא עתה. השינויים שלכם מוצגים בחלון העריכה התחתון. עליכם למזג את השינויים שלכם לתוך הטקסט הקיים. '''רק''' הטקסט בחלון העריכה העליון יישמר כשתשמרו את הדף.",
'yourtext'                         => 'הטקסט שלך',
'storedversion'                    => 'גרסה שמורה',
'nonunicodebrowser'                => '<strong>אזהרה: הדפדפן שלכם אינו תואם לתקן יוניקוד. כדי למנוע בעיות הנוצרות כתוצאה מכך ולאפשר לכם לערוך דפים בבטחה, תווים שאינם ב־ASCII יוצגו בתיבת העריכה כקודים הקסדצימליים.</strong>',
'editingold'                       => '<strong>אזהרה: אתם עורכים גרסה לא עדכנית של דף זה. אם תשמרו את הדף, כל השינויים שנעשו מאז גרסה זו יאבדו.</strong>',
'yourdiff'                         => 'הבדלים',
'copyrightwarning'                 => "'''שימו לב:''' תרומתכם ל{{grammar:תחילית|{{SITENAME}}}} תפורסם תחת תנאי הרישיון $2 (ראו $1 לפרטים נוספים). אם אינכם רוצים שעבודתכם תהיה זמינה לעריכה על ידי אחרים, שתופץ לעיני כל, ושאחרים יוכלו להעתיק ממנה בציון המקור – אל תפרסמו אותה פה. כמו־כן, אתם מבטיחים לנו כי כתבתם את הטקסט הזה בעצמכם, או העתקתם אותו ממקור שאינו מוגן על ידי זכויות יוצרים. '''אל תעשו שימוש בחומר המוגן בזכויות יוצרים ללא רשות!'''",
'copyrightwarning2'                => "'''שימו לב:''' תורמים אחרים עשויים לערוך או אף להסיר את תרומתכם ל{{grammar:תחילית|{{SITENAME}}}}. אם אינכם רוצים שעבודתכם תהיה זמינה לעריכה על ידי אחרים – אל תפרסמו אותה פה. כמו־כן, אתם מבטיחים לנו כי כתבתם את הטקסט הזה בעצמכם, או העתקתם אותו ממקור שאינו מוגן על ידי זכויות יוצרים (ראו $1 לפרטים נוספים). '''אל תעשו שימוש בחומר המוגן בזכויות יוצרים ללא רשות!'''",
'longpagewarning'                  => '<strong>אזהרה: גודל דף זה הוא $1 קילובייטים. בדפדפנים מסוימים יהיו בעיות בעריכת דף הגדול מ־32 קילובייטים. אנא שיקלו לחלק דף זה לדפים קטנים יותר. אם זהו דף שיחה, שיקלו לארכב אותו.</strong>',
'longpageerror'                    => '<strong>שגיאה: הטקסט ששלחתם הוא באורך $1 קילובייטים, אך אסור לו להיות ארוך יותר מהמקסימום של $2 קילובייטים. לא ניתן לשומרו.</strong>',
'readonlywarning'                  => '<strong>אזהרה: בסיס הנתונים ננעל לצורך תחזוקה. בזמן זה אי אפשר לשמור את הטקסט הערוך. בינתיים, עד סיום התחזוקה, אתם יכולים להשתמש בעורך חיצוני. אנו מתנצלים על התקלה.</strong>',
'protectedpagewarning'             => '<strong>אזהרה: דף זה ננעל כך שרק מפעילי מערכת יכולים לערוך אותו. אנא ודאו שאתם פועלים על־פי העקרונות לעריכת דפים אלו.</strong>',
'semiprotectedpagewarning'         => "'''הערה:''' דף זה ננעל כך שרק משתמשים רשומים יכולים לערוך אותו.",
'cascadeprotectedwarning'          => "'''אזהרה:''' דף זה ננעל כך שרק מפעילי מערכת יכולים לערוך אותו, כיוון שהוא מוכלל {{PLURAL:$1|בדף הבא, שמופעלת עליו|בדפים הבאים, שמופעלת עליהם}} הגנה מדורגת:",
'titleprotectedwarning'            => '<strong>אזהרה: דף זה ננעל כך שרק משתמשים מסוימים יכולים ליצור אותו.</strong>',
'templatesused'                    => 'תבניות המופיעות בדף זה:',
'templatesusedpreview'             => 'תבניות המופיעות בתצוגה המקדימה הזו:',
'templatesusedsection'             => 'תבניות המופיעות בפסקה זו:',
'template-protected'               => '(מוגנת)',
'template-semiprotected'           => '(מוגנת חלקית)',
'hiddencategories'                 => 'דף זה חבר ב{{PLURAL:$1|קטגוריה מוסתרת אחת|־$1 קטגוריות מוסתרות}}:',
'edittools'                        => '<!-- הטקסט הנכתב כאן יוצג מתחת לטפסי עריכת דפים והעלאת קבצים, ולפיכך ניתן לכתוב להציג בו תווים קשים לכתיבה, קטעים מוכנים של טקסט ועוד. -->',
'nocreatetitle'                    => 'יצירת הדפים הוגבלה',
'nocreatetext'                     => 'אתר זה מגביל את האפשרות ליצור דפים חדשים. באפשרותכם לחזור אחורה ולערוך דף קיים, או [[Special:UserLogin|להיכנס לחשבון]].',
'nocreate-loggedin'                => 'אינכם מורשים ליצור דפים חדשים.',
'permissionserrors'                => 'שגיאות הרשאה',
'permissionserrorstext'            => 'אינכם מורשים לבצע פעולה זו, {{PLURAL:$1|מהסיבה הבאה|מהסיבות הבאות}}:',
'permissionserrorstext-withaction' => 'אינכם מורשים לבצע $2, {{PLURAL:$1|מהסיבה הבאה|מהסיבות הבאות}}:',
'recreate-deleted-warn'            => "'''אזהרה: הנכם יוצרים דף חדש שנמחק בעבר.'''

אנא שיקלו אם יהיה זה נכון להמשיך לערוך את הדף.
יומן המחיקות של הדף מוצג להלן:",

# Parser/template warnings
'expensive-parserfunction-warning'        => 'אזהרה: דף זה כולל יותר מדי קריאות למשתנים הגוזלים משאבים.

צריכות להיות פחות מ־$2 קריאות כאלה, אך כרגע יש $1.',
'expensive-parserfunction-category'       => 'דפים עם יותר מדי קריאות למשתנים הגוזלים משאבים',
'post-expand-template-inclusion-warning'  => 'אזהרה: התבניות המוכללות בדף זה גדולות מדי.
חלק מהתבניות לא יוכללו.',
'post-expand-template-inclusion-category' => 'דפים שבהם ההכללה גדולה מדי',
'post-expand-template-argument-warning'   => 'אזהרה: דף זה כולל לפחות תבנית אחת שבה פרמטרים גדולים מדי.
פרמטרים אלה הושמטו.',
'post-expand-template-argument-category'  => 'דפים שבהם הושמטו פרמטרים של תבניות',

# "Undo" feature
'undo-success' => 'ניתן לבטל את העריכה. אנא בידקו את השוואת הגרסאות למטה כדי לוודא שזה מה שאתם רוצים לעשות, ואז שמרו את השינויים למטה כדי לבצע את ביטול העריכה.',
'undo-failure' => 'לא ניתן היה לבטל את העריכה עקב התנגשות עם עריכות מאוחרות יותר.',
'undo-norev'   => 'לא ניתן היה לבטל את העריכה כיוון שהיא אינה קיימת או שהיא נמחקה.',
'undo-summary' => 'ביטול גרסה $1 של [[Special:Contributions/$2|$2]] ([[User talk:$2|שיחה]])',

# Account creation failure
'cantcreateaccounttitle' => 'לא ניתן ליצור את החשבון',
'cantcreateaccount-text' => 'אפשרות יצירת החשבונות מכתובת ה־IP הזו (<b>$1</b>) נחסמה על ידי [[User:$3|$3]]. הסיבה שניתנה על ידי $3 היא "$2".',

# History pages
'viewpagelogs'        => 'הצגת יומנים עבור דף זה',
'nohistory'           => 'אין היסטוריית שינויים עבור דף זה.',
'revnotfound'         => 'גרסה זו לא נמצאה',
'revnotfoundtext'     => 'הגרסה הישנה של דף זה לא נמצאה. אנא בדקו את כתובת הקישור שהוביל אתכם הנה.',
'currentrev'          => 'גרסה נוכחית',
'revisionasof'        => 'גרסה מתאריך $1',
'revision-info'       => 'גרסה מתאריך $1 מאת $2',
'previousrevision'    => '→ הגרסה הקודמת',
'nextrevision'        => 'הגרסה הבאה ←',
'currentrevisionlink' => 'הגרסה הנוכחית',
'cur'                 => 'נוכ',
'next'                => 'הבא',
'last'                => 'אחרון',
'page_first'          => 'ראשון',
'page_last'           => 'אחרון',
'histlegend'          => 'השוואת גרסאות: סמנו את תיבות האפשרויות של הגרסאות המיועדות להשוואה, והקישו על Enter או על הכפתור שלמעלה או למטה.<br />
מקרא: (נוכ) = הבדלים עם הגרסה הנוכחית, (אחרון) = הבדלים עם הגרסה הקודמת, מ = שינוי משני',
'deletedrev'          => '[נמחק]',
'histfirst'           => 'ראשונות',
'histlast'            => 'אחרונות',
'historysize'         => '({{PLURAL:$1|בית אחד|$1 בתים}})',
'historyempty'        => '(ריק)',

# Revision feed
'history-feed-title'          => 'היסטוריית גרסאות',
'history-feed-description'    => 'היסטוריית הגרסאות של הדף הזה בוויקי',
'history-feed-item-nocomment' => '$1 ב־$2', # user at time
'history-feed-empty'          => 'הדף המבוקש לא נמצא.
ייתכן שהוא נמחק, או ששמו שונה.
נסו [[Special:Search|לחפש]] אחר דפים רלוונטיים חדשים.',

# Revision deletion
'rev-deleted-comment'         => '(תקציר העריכה הוסתר)',
'rev-deleted-user'            => '(שם המשתמש הוסתר)',
'rev-deleted-event'           => '(פעולת היומן הוסתרה)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">
גרסת הדף הזו הוסרה מהארכיונים הציבוריים. ייתכן שישנם פרטים נוספים על כך ב[{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} יומן המחיקות].
</div>',
'rev-deleted-text-view'       => '<div class="mw-warning plainlinks">
גרסת הדף הזו הוסרה מהארכיונים הציבוריים. כמפעיל מערכת, באפשרותך לצפות בגרסה; ייתכן שישנם פרטים נוספים על כך ב[{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} יומן המחיקות].
</div>',
'rev-delundel'                => 'הצגה/הסתרה',
'revisiondelete'              => 'מחיקת ושחזור גרסאות',
'revdelete-nooldid-title'     => 'גרסת מטרה בלתי תקינה',
'revdelete-nooldid-text'      => 'הגרסה או הגרסאות עליהן תבוצע פעולה זו אינן תקינות. ייתכן שלא ציינתם אותן, ייתכן שהגרסה אינה קיימת, וייתכן שאתם מנסים להסתיר את הגרסה הנוכחית.',
'revdelete-selected'          => '{{PLURAL:$2|הגרסה שנבחרה|הגרסאות שנבחרו}} של [[:$1]]:',
'logdelete-selected'          => '{{PLURAL:$1|פעולת היומנים שנבחרה|פעולות היומנים שנבחרו}}:',
'revdelete-text'              => 'גרסאות ופעולות יומנים שנמחקו עדיין יופיעו בהיסטוריית הדף ובדפי היומנים, אך חלקים מתוכנם שלהם לא יהיה זמין לציבור.

מפעילי מערכת אחרים באתר עדיין יוכלו לגשת לתוכן הנסתר ויוכלו לשחזר אותו שוב דרך הממשק הזה, אלא אם כן מוגדרות הגבלות נוספות.',
'revdelete-legend'            => 'הגדרת הגבלות התצוגה',
'revdelete-hide-text'         => 'הסתרת תוכן הגרסה',
'revdelete-hide-name'         => 'הסתרת הפעולה ודף היעד',
'revdelete-hide-comment'      => 'הסתרת תקציר העריכה',
'revdelete-hide-user'         => 'הסתרת שם המשתמש או כתובת ה־IP של העורך',
'revdelete-hide-restricted'   => 'החלת הגבלות אלו גם על מפעילי מערכת ונעילת ממשק זה',
'revdelete-suppress'          => 'הסתרת המידע גם ממפעילי המערכת',
'revdelete-hide-image'        => 'הסתרת תוכן הקובץ',
'revdelete-unsuppress'        => 'הסרת הגבלות בגרסאות המשוחזרות',
'revdelete-log'               => 'הערה ביומן:',
'revdelete-submit'            => 'ביצוע על הגרסה שנבחרה',
'revdelete-logentry'          => 'שינה את הסתרת הגרסה של [[$1]]',
'logdelete-logentry'          => 'שינה את הסתרת פעולת היומן של [[$1]]',
'revdelete-success'           => "'''הסתרת הגרסה הושלמה בהצלחה.'''",
'logdelete-success'           => "'''הסתרת פעולת היומן הושלמה בהצלחה.'''",
'revdel-restore'              => 'שינוי ההצגה',
'pagehist'                    => 'היסטוריית הדף',
'deletedhist'                 => 'הגרסאות המחוקות',
'revdelete-content'           => 'התוכן',
'revdelete-summary'           => 'תקציר העריכה',
'revdelete-uname'             => 'שם המשתמש',
'revdelete-restricted'        => 'נוספו הגבלות למפעילי מערכת',
'revdelete-unrestricted'      => 'הוסרו הגבלות ממפעילי מערכת',
'revdelete-hid'               => 'הסתיר את $1',
'revdelete-unhid'             => 'ביטל את הסתרת $1',
'revdelete-log-message'       => '$1 עבור {{PLURAL:$2|גרסה אחת|$2 גרסאות}}',
'logdelete-log-message'       => '$1 עבור {{PLURAL:$2|אירוע אחד|$2 אירועים}}',

# Suppression log
'suppressionlog'     => 'יומן הסתרות',
'suppressionlogtext' => 'להלן רשימת המחיקות והחסימות הכוללות תוכן המוסתר ממפעילי המערכת. ראו את  [[Special:IPBlockList|רשימת המשתמשים החסומים]] לרשימת החסימות הפעילות כעת.',

# History merging
'mergehistory'                     => 'מיזוג גרסאות של דפים',
'mergehistory-header'              => "דף זה מאפשר לכם למזג גרסאות מהיסטוריית הדף של דף מקור לתוך דף חדש יותר.
אנא ודאו ששינוי זה לא יפגע בהמשכיות השינויים בדף הישן.

'''לפחות גרסה אחת של דף המקור חייבת להישאר בו.'''",
'mergehistory-box'                 => 'מיזוג גרסאות של שני דפים:',
'mergehistory-from'                => 'דף המקור:',
'mergehistory-into'                => 'דף היעד:',
'mergehistory-list'                => 'היסטוריית עריכות ברת מיזוג',
'mergehistory-merge'               => 'ניתן למזג את הגרסאות הבאות של [[:$1]] לתוך [[:$2]]. אנא השתמשו בלחצני האפשרות כדי לבחור זמן שרק גרסאות שנוצרו בו ולפניו ימוזגו. שימוש בקישורי הניווט יאפס עמודה זו.',
'mergehistory-go'                  => 'הצגת עריכות ברות מיזוג',
'mergehistory-submit'              => 'מיזוג',
'mergehistory-empty'               => 'אין גרסאות למיזוג.',
'mergehistory-success'             => '{{PLURAL:$3|גרסה אחת|$3 גרסאות}} של [[:$1]] מוזגו בהצלחה לתוך [[:$2]].',
'mergehistory-fail'                => 'לא ניתן לבצע את מיזוג הגרסאות, אנא בדקו שנית את הגדרות הדף והזמן.',
'mergehistory-no-source'           => 'דף המקור $1 אינו קיים.',
'mergehistory-no-destination'      => 'דף היעד $1 אינו קיים.',
'mergehistory-invalid-source'      => 'דף המקור חייב להיות בעל כותרת תקינה.',
'mergehistory-invalid-destination' => 'דף היעד חייב להיות בעל כותרת תקינה.',
'mergehistory-autocomment'         => 'מיזג את [[:$1]] לתוך [[:$2]]',
'mergehistory-comment'             => 'מיזג את [[:$1]] לתוך [[:$2]]: $3',

# Merge log
'mergelog'           => 'יומן מיזוגים',
'pagemerge-logentry' => 'מיזג את [[$1]] לתוך [[$2]] (גרסאות עד $3)',
'revertmerge'        => 'ביטול המיזוג',
'mergelogpagetext'   => 'זוהי רשימה של המיזוגים האחרונים של גרסאות מדף אחד לתוך דף שני.',

# Diffs
'history-title'           => 'היסטוריית הגרסאות של $1',
'difference'              => '(הבדלים בין גרסאות)',
'lineno'                  => 'שורה $1:',
'compareselectedversions' => 'השוואת הגרסאות שנבחרו',
'editundo'                => 'ביטול',
'diff-multi'              => '({{PLURAL:$1|גרסת ביניים אחת אינה מוצגת|$1 גרסאות ביניים אינן מוצגות}}.)',

# Search results
'searchresults'             => 'תוצאות החיפוש',
'searchresulttext'          => 'למידע נוסף על חיפוש ב{{grammar:תחילית|{{SITENAME}}}}, עיינו ב[[Project:עזרה|דפי העזרה]].',
'searchsubtitle'            => 'לחיפוש המונח \'\'\'[[:$1]]\'\'\' ([[Special:Prefixindex/$1|לכל הדפים המתחילים ב"$1"]] | [[Special:WhatLinksHere/$1|לכל הדפים המקשרים ל"$1"]])',
'searchsubtitleinvalid'     => "לחיפוש המונח '''$1'''",
'noexactmatch'              => 'אין דף שכותרתו "$1". באפשרותכם [[:$1|ליצור את הדף]].',
'noexactmatch-nocreate'     => 'אין דף שכותרתו "$1".',
'toomanymatches'            => 'יותר מדי תוצאות נמצאו, אנא נסו מילות חיפוש אחרות',
'titlematches'              => 'כותרות דפים תואמות',
'notitlematches'            => 'אין כותרות דפים תואמות',
'textmatches'               => 'דפים עם תוכן תואם',
'notextmatches'             => 'אין דפים עם תוכן תואם',
'prevn'                     => '$1 הקודמים',
'nextn'                     => '$1 הבאים',
'viewprevnext'              => 'צפו ב - ($1) ($2) ($3)',
'search-result-size'        => '$1 ({{PLURAL:$2|מילה אחת|$2 מילים}})',
'search-result-score'       => 'רלוונטיות: $1%',
'search-redirect'           => '(הפניה $1)',
'search-section'            => '(פסקה $1)',
'search-suggest'            => 'האם התכוונת ל: $1',
'search-interwiki-caption'  => 'מיזמי אחות',
'search-interwiki-default'  => '$1 תוצאות:',
'search-interwiki-more'     => '(עוד)',
'search-mwsuggest-enabled'  => 'עם הצעות',
'search-mwsuggest-disabled' => 'ללא הצעות',
'search-relatedarticle'     => 'קשור',
'mwsuggest-disable'         => 'ביטול הצעות AJAX',
'searchrelated'             => 'קשור',
'searchall'                 => 'הכול',
'showingresults'            => "הצגת עד {{PLURAL:$1|תוצאה '''אחת'''|'''$1''' תוצאות}} החל ממספר #'''$2''':",
'showingresultsnum'         => "הצגת {{PLURAL:$3|תוצאה '''אחת'''|'''$3''' תוצאות}} החל ממספר #'''$2''':",
'showingresultstotal'       => "הצגת {{PLURAL:$3|תוצאה '''$1''' מתוך '''$3'''|תוצאות '''$1 - $2''' מתוך '''$3'''}}",
'nonefound'                 => "'''הערה''': כברירת מחדל, החיפוש מבוצע במספר מרחבי שם בלבד. באפשרותכם לכתוב '''all:''' לפני מונח החיפוש כדי לחפש בכל הדפים (כולל דפי שיחה, תבניות, ועוד), או לכתוב לפני מונח החיפוש את מרחב השם שאתם מעוניינים בו.",
'powersearch'               => 'חיפוש מתקדם',
'powersearch-legend'        => 'חיפוש מתקדם',
'powersearch-ns'            => 'חיפוש במרחבי השם:',
'powersearch-redir'         => 'הצגת דפי הפניה',
'powersearch-field'         => 'חיפוש',
'search-external'           => 'חיפוש חיצוני',
'searchdisabled'            => 'לצערנו, עקב עומס על המערכת, לא ניתן לחפש כעת בטקסט המלא של הדפים. באפשרותכם להשתמש בינתיים בגוגל, אך שימו לב שייתכן שהוא אינו מעודכן.',

# Preferences page
'preferences'              => 'העדפות',
'mypreferences'            => 'ההעדפות שלי',
'prefs-edits'              => 'מספר עריכות:',
'prefsnologin'             => 'לא נרשמת באתר',
'prefsnologintext'         => 'עליכם <span class="plainlinks">[{{fullurl:Special:Userlogin|returnto=$1}} להיכנס לחשבון]</span> כדי לשנות העדפות משתמש.',
'prefsreset'               => 'ההעדפות שוחזרו למצבן הקודם.',
'qbsettings'               => 'הגדרות סרגל כלים',
'qbsettings-none'          => 'ללא',
'qbsettings-fixedleft'     => 'קבוע משמאל',
'qbsettings-fixedright'    => 'קבוע מימין',
'qbsettings-floatingleft'  => 'צף משמאל',
'qbsettings-floatingright' => 'צף מימין',
'changepassword'           => 'שינוי סיסמה',
'skin'                     => 'רקע',
'math'                     => 'נוסחאות מתמטיות',
'dateformat'               => 'מבנה תאריך',
'datedefault'              => 'ברירת המחדל',
'datetime'                 => 'תאריך ושעה',
'math_failure'             => 'עיבוד הנוסחה נכשל',
'math_unknown_error'       => 'שגיאה לא ידועה',
'math_unknown_function'    => 'פונקציה לא מוכרת',
'math_lexing_error'        => 'שגיאת לקסינג',
'math_syntax_error'        => 'שגיאת תחביר',
'math_image_error'         => 'ההמרה ל־PNG נכשלה; אנא בדקו אם התקנתם נכון את latex, את dvips, את gs ואת convert.',
'math_bad_tmpdir'          => 'התוכנה לא הצליחה לכתוב או ליצור את הספרייה הזמנית של המתמטיקה',
'math_bad_output'          => 'התוכנה לא הצליחה לכתוב או ליצור את ספריית הפלט של המתמטיקה',
'math_notexvc'             => 'קובץ בר־ביצוע של texvc אינו זמין; אנא ראו את קובץ ה־README למידע על ההגדרות.',
'prefs-personal'           => 'פרטי המשתמש',
'prefs-rc'                 => 'שינויים אחרונים',
'prefs-watchlist'          => 'רשימת המעקב',
'prefs-watchlist-days'     => 'מספר הימים המירבי שיוצגו ברשימת המעקב:',
'prefs-watchlist-edits'    => 'מספר העריכות המירבי שיוצגו ברשימת המעקב המורחבת:',
'prefs-misc'               => 'שונות',
'saveprefs'                => 'שמירת העדפות',
'resetprefs'               => 'מחיקת שינויים שלא נשמרו',
'oldpassword'              => 'סיסמה ישנה:',
'newpassword'              => 'סיסמה חדשה:',
'retypenew'                => 'חזרה על הסיסמה חדשה:',
'textboxsize'              => 'עריכה',
'rows'                     => 'שורות:',
'columns'                  => 'עמודות:',
'searchresultshead'        => 'חיפוש',
'resultsperpage'           => 'מספר תוצאות בעמוד:',
'contextlines'             => 'שורות לכל תוצאה:',
'contextchars'             => 'מספר תווי קונטקסט בשורה:',
'stub-threshold'           => 'סף לעיצוב <a href="#" class="stub">קישורים</a> לקצרמרים (בתים):',
'recentchangesdays'        => 'מספר הימים שיוצגו בדף השינויים האחרונים:',
'recentchangescount'       => 'מספר העריכות שיוצגו בדפי השינויים האחרונים, היסטוריית הדף והיומנים:',
'savedprefs'               => 'העדפותיך נשמרו.',
'timezonelegend'           => 'אזור זמן',
'timezonetext'             => '¹הפרש השעות בינך לבין השרת (UTC).',
'localtime'                => 'זמן מקומי',
'timezoneoffset'           => 'הפרש¹',
'servertime'               => 'השעה הנוכחית בשרת היא',
'guesstimezone'            => 'קבל מהדפדפן',
'allowemail'               => 'קבלת דוא"ל ממשתמשים אחרים',
'prefs-searchoptions'      => 'אפשרויות חיפוש',
'prefs-namespaces'         => 'מרחבי שם',
'defaultns'                => 'מרחבי שם לחיפוש כברירת מחדל:',
'default'                  => 'ברירת מחדל',
'files'                    => 'קבצים',

# User rights
'userrights'                  => 'ניהול הרשאות משתמש', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => 'ניהול קבוצות משתמש',
'userrights-user-editname'    => 'שם משתמש:',
'editusergroup'               => 'עריכת קבוצות משתמשים',
'editinguser'                 => "שינוי הרשאות המשתמש של '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup'    => 'עריכת קבוצות משתמש',
'saveusergroups'              => 'שמירת קבוצות משתמש',
'userrights-groupsmember'     => 'חבר בקבוצות:',
'userrights-groups-help'      => 'באפשרותכם לשנות את הקבוצות שמשתמש זה חבר בהן:
* תיבה מסומנת פירושה שהמשתמש חבר בקבוצה.
* תיבה בלתי מסומנת פירושה שהמשתמש אינו חבר בקבוצה.
* סימון * פירושו שלא תוכלו להסיר משתמש מהקבוצה מרגע שהוספתם אותו אליה, או להיפך.',
'userrights-reason'           => 'סיבה לשינוי:',
'userrights-no-interwiki'     => 'אין לכם הרשאה לערוך הרשאות משתמש באתרים אחרים.',
'userrights-nodatabase'       => 'בסיס הנתונים $1 אינו קיים או אינו מקומי.',
'userrights-nologin'          => 'עליכם [[Special:UserLogin|להיכנס לחשבון]] עם הרשאות מתאימות כדי לשנות הרשאות של משתמשים.',
'userrights-notallowed'       => 'לחשבון המשתמש שלכם אין הרשאה לשנות הרשאות של משתמשים.',
'userrights-changeable-col'   => 'קבוצות שבאפשרותכם לשנות',
'userrights-unchangeable-col' => 'קבוצות שאין באפשרותכם לשנות',

# Groups
'group'               => 'קבוצה:',
'group-user'          => 'משתמשים',
'group-autoconfirmed' => 'משתמשים ותיקים',
'group-bot'           => 'בוטים',
'group-sysop'         => 'מפעילי מערכת',
'group-bureaucrat'    => 'ביורוקרטים',
'group-suppress'      => 'מסתירים',
'group-all'           => '(הכול)',

'group-user-member'          => 'משתמש',
'group-autoconfirmed-member' => 'משתמש ותיק',
'group-bot-member'           => 'בוט',
'group-sysop-member'         => 'מפעיל מערכת',
'group-bureaucrat-member'    => 'ביורוקרט',
'group-suppress-member'      => 'מסתיר',

'grouppage-user'          => '{{ns:project}}:משתמש רשום',
'grouppage-autoconfirmed' => '{{ns:project}}:משתמש ותיק',
'grouppage-bot'           => '{{ns:project}}:בוט',
'grouppage-sysop'         => '{{ns:project}}:מפעיל מערכת',
'grouppage-bureaucrat'    => '{{ns:project}}:ביורוקרט',
'grouppage-suppress'      => '{{ns:project}}:מסתיר',

# Rights
'right-read'                 => 'קריאת דפים',
'right-edit'                 => 'עריכת דפים',
'right-createpage'           => 'יצירת דפים שאינם דפי שיחה',
'right-createtalk'           => 'יצירת דפי שיחה',
'right-createaccount'        => 'יצירת חשבונות משתמש חדשים',
'right-minoredit'            => 'סימון עריכות כמשניות',
'right-move'                 => 'העברת דפים',
'right-move-subpages'        => 'העברת דפים עם דפי המשנה שלהם',
'right-suppressredirect'     => 'הימנעות מיצירת הפניה משם הישן בעת העברת דף',
'right-upload'               => 'העלאת קבצים',
'right-reupload'             => 'דריסת קובץ קיים',
'right-reupload-own'         => 'דריסת קובץ קיים שהועלה על ידי אותו המשתמש',
'right-reupload-shared'      => 'דריסת קבצים מאתר התמונות המשותף באופן מקומי',
'right-upload_by_url'        => 'העלאת קובץ מכתובת אינטרנט',
'right-purge'                => 'רענון זכרון המטמון של האתר לדף מסוים ללא דף אישור',
'right-autoconfirmed'        => 'עריכת דפים מוגנים חלקית',
'right-bot'                  => 'טיפול בעריכות כאוטומטיות',
'right-nominornewtalk'       => 'ביטול הודעת ההודעות החדשות בעת עריכה משנית בדפי שיחה',
'right-apihighlimits'        => 'שימוש ב־API עם פחות הגבלות',
'right-writeapi'             => 'שימוש ב־API לשינוי דפים',
'right-delete'               => 'מחיקת דפים',
'right-bigdelete'            => 'מחיקת דפים עם היסטוריית דף ארוכה',
'right-deleterevision'       => 'מחיקת גרסאות מסוימות של דפים',
'right-deletedhistory'       => 'צפייה בגרסאות מחוקות ללא הטקסט השייך להן',
'right-browsearchive'        => 'חיפוש דפים מחוקים',
'right-undelete'             => 'שחזור דף מחוק',
'right-suppressrevision'     => 'בדיקה ושחזור של גרסאות המוסתרות ממפעילי המערכת',
'right-suppressionlog'       => 'צפייה ביומנים פרטיים',
'right-block'                => 'חסימת משתמשים אחרים מעריכה',
'right-blockemail'           => 'חסימת משתמש משליחת דואר אלקטרוני',
'right-hideuser'             => 'חסימת שם משתמש תוך הסתרתו מהציבור',
'right-ipblock-exempt'       => 'עקיפת חסימות של כתובת IP, חסימות אוטומטיות וחסימות טווח',
'right-proxyunbannable'      => 'עקיפת חסימות אוטומטיות של שרתי פרוקסי',
'right-protect'              => 'שינוי רמות הגנה ועריכת דפים מוגנים',
'right-editprotected'        => 'עריכת דפים מוגנים (ללא הגנה מדורגת)',
'right-editinterface'        => 'עריכת ממשק המשתמש',
'right-editusercssjs'        => 'עריכת דפי CSS ו־JS של משתמשים אחרים',
'right-rollback'             => 'שחזור מהיר של עריכות המשתמש האחרון שערך דף מסוים',
'right-markbotedits'         => 'סימון עריכות משוחזרות כעריכות של בוט',
'right-noratelimit'          => 'עקיפת הגבלת קצב העריכות',
'right-import'               => 'ייבוא דפים מאתרי ויקי אחרים',
'right-importupload'         => 'ייבוא דפים באמצעות העלאת קובץ',
'right-patrol'               => 'סימון עריכות של אחרים כבדוקות',
'right-autopatrol'           => 'סימון אוטומטי של עריכות של המשתמש כבדוקות',
'right-patrolmarks'          => 'צפייה בסימוני עריכות בדוקות בשינויים האחרונים',
'right-unwatchedpages'       => 'הצגת רשימה של דפים שאינם במעקב',
'right-trackback'            => 'שליחת טראקבק',
'right-mergehistory'         => 'מיזוג היסטוריות של דפים',
'right-userrights'           => 'עריכת כל הרשאות המשתמש',
'right-userrights-interwiki' => 'עריכת הרשאות המשתמש של משתמשים באתרי ויקי אחרים',
'right-siteadmin'            => 'נעילת וביטול נעילת בסיס הנתונים',

# User rights log
'rightslog'      => 'יומן תפקידים',
'rightslogtext'  => 'זהו יומן השינויים בתפקידי המשתמשים.',
'rightslogentry' => 'שינה את ההרשאות של $1 מ-$2 ל-$3',
'rightsnone'     => '(כלום)',

# Recent changes
'nchanges'                          => '{{PLURAL:$1|שינוי אחד|$1 שינויים}}',
'recentchanges'                     => 'שינויים אחרונים',
'recentchangestext'                 => 'ניתן לעקוב אחרי השינויים האחרונים באתר בדף זה.',
'recentchanges-feed-description'    => 'ניתן לעקוב אחרי השינויים האחרונים באתר בדף זה.',
'rcnote'                            => "להלן {{PLURAL:$1|השינוי האחרון|'''$1''' השינויים האחרונים}} {{PLURAL:$2|ביום האחרון|ב־$2 הימים האחרונים}}, עד $5, $4:",
'rcnotefrom'                        => 'להלן <b>$1</b> השינויים האחרונים שבוצעו החל מתאריך <b>$2</b>:',
'rclistfrom'                        => 'הצגת שינויים חדשים החל מ־$1',
'rcshowhideminor'                   => '$1 שינויים משניים',
'rcshowhidebots'                    => '$1 בוטים',
'rcshowhideliu'                     => '$1 משתמשים רשומים',
'rcshowhideanons'                   => '$1 משתמשים אנונימיים',
'rcshowhidepatr'                    => '$1 עריכות בדוקות',
'rcshowhidemine'                    => '$1 עריכות שלי',
'rclinks'                           => 'הצגת $1 שינויים אחרונים ב־$2 הימים האחרונים.<br /> $3',
'diff'                              => 'הבדל',
'hist'                              => 'היסטוריה',
'hide'                              => 'הסתרת',
'show'                              => 'הצגת',
'minoreditletter'                   => 'מ',
'newpageletter'                     => 'ח',
'boteditletter'                     => 'ב',
'sectionlink'                       => '←',
'number_of_watching_users_pageview' => '[{{PLURAL:$1|משתמש אחד עוקב|$1 משתמשים עוקבים}} אחרי הדף]',
'rc_categories'                     => 'הגבלה לקטגוריות (יש להפריד עם "|")',
'rc_categories_any'                 => 'הכול',
'newsectionsummary'                 => '/* $1 */ פסקה חדשה',

# Recent changes linked
'recentchangeslinked'          => 'שינויים בדפים המקושרים',
'recentchangeslinked-title'    => 'שינויים בדפים המקושרים לדף $1',
'recentchangeslinked-noresult' => 'לא היו שינויים בדפים המקושרים בתקופה זו.',
'recentchangeslinked-summary'  => "בדף מיוחד זה רשומים השינויים האחרונים בדפים המקושרים מתוך הדף (או בדפים החברים בקטגוריה).
דפים ב[[Special:Watchlist|רשימת המעקב שלכם]] מוצגים ב'''הדגשה'''.",
'recentchangeslinked-page'     => 'שם הדף:',
'recentchangeslinked-to'       => 'הצגת השינויים בדפים המקשרים לדף הנתון במקום זאת',

# Upload
'upload'                      => 'העלאת קובץ לשרת',
'uploadbtn'                   => 'העלאה',
'reupload'                    => 'העלאה חוזרת',
'reuploaddesc'                => 'ביטול ההעלאה וחזרה לטופס העלאת קבצים לשרת',
'uploadnologin'               => 'לא נכנסתם לאתר',
'uploadnologintext'           => 'עליכם [[Special:UserLogin|להיכנס לחשבון]] כדי להעלות קבצים.',
'upload_directory_missing'    => 'שרת האינטרנט אינו יכול ליצור את תיקיית ההעלאות ($1) החסרה.',
'upload_directory_read_only'  => 'שרת האינטרנט אינו יכול לכתוב בתיקיית ההעלאות ($1), ולפיכך הוא אינו יכול להעלות את התמונה.',
'uploaderror'                 => 'שגיאה בהעלאת הקובץ',
'uploadtext'                  => "השתמשו בטופס להלן כדי להעלות תמונות. כדי לראות או לחפש תמונות שהועלו בעבר אנא פנו ל[[Special:ImageList|רשימת הקבצים המועלים]], וכמו כן, העלאות (כולל העלאות של גרסה חדשה) מוצגות ב[[Special:Log/upload|יומן ההעלאות]], ומחיקות ב[[Special:Log/delete|יומן המחיקות]].

כדי לכלול תמונה בדף, השתמשו בקישור באחת הצורות הבאות:
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:File.jpg]]</nowiki></tt>''' לשימוש בגרסה המלאה של הקובץ
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:File.png|200px|thumb|left|טקסט תיאור]]</nowiki></tt>''' לשימוש בגרסה מוקטנת ברוחב 200 פיקסלים בתיבה בצד שמאל של הדף, עם 'טקסט תיאור' כתיאור התמונה
* '''<tt><nowiki>[[</nowiki>{{ns:media}}<nowiki>:File.ogg]]</nowiki></tt>''' לקישור ישיר לקובץ בלי להציגו",
'upload-permitted'            => 'סוגי קבצים מותרים: $1.',
'upload-preferred'            => 'סוגי קבצים מומלצים: $1.',
'upload-prohibited'           => 'סוגי קבצים אסורים: $1.',
'uploadlog'                   => 'יומן העלאות קבצים',
'uploadlogpage'               => 'יומן העלאות',
'uploadlogpagetext'           => 'להלן רשימה של העלאות הקבצים האחרונות שבוצעו.
ראו את [[Special:NewImages|גלריית התמונות החדשות]] להצגה ויזואלית שלהם.',
'filename'                    => 'שם הקובץ',
'filedesc'                    => 'תקציר',
'fileuploadsummary'           => 'תיאור:',
'filestatus'                  => 'מעמד זכויות יוצרים:',
'filesource'                  => 'מקור:',
'uploadedfiles'               => 'קבצים שהועלו',
'ignorewarning'               => 'התעלמות מהאזהרה ושמירת הקובץ בכל זאת',
'ignorewarnings'              => 'התעלמות מכל האזהרות',
'minlength1'                  => 'שמות של קובצי תמונה צריכים להיות בני תו אחד לפחות.',
'illegalfilename'             => 'הקובץ "$1" מכיל תווים בלתי חוקיים. אנא שנו את שמו ונסו להעלותו שנית.',
'badfilename'                 => 'שם התמונה שונה ל־"$1".',
'filetype-badmime'            => 'לא ניתן להעלות קבצים עם סוג ה־MIME "$1".',
'filetype-unwanted-type'      => "'''\".\$1\"''' הוא סוג קובץ בלתי מומלץ. {{PLURAL:\$3|סוג הקובץ המומלץ הוא|סוגי הקבצים המומלצים הם}} \$2.",
'filetype-banned-type'        => "'''\".\$1\"''' הוא סוג קובץ אסור להעלאה. {{PLURAL:\$3|סוג הקובץ המותר הוא|סוגי הקבצים המותרים הם}} \$2.",
'filetype-missing'            => 'לקובץ אין סיומת (כדוגמת ".jpg").',
'large-file'                  => 'מומלץ שהקבצים לא יהיו גדולים יותר מ־$1 (גודל הקובץ שהעליתם הוא $2).',
'largefileserver'             => 'גודל הקובץ שהעליתם חורג ממגבלת השרת.',
'emptyfile'                   => 'הקובץ שהעליתם ריק. ייתכן שהסיבה לכך היא שגיאת הקלדה בשם הקובץ. אנא ודאו שזהו הקובץ שברצונך להעלות.',
'fileexists'                  => 'קובץ בשם זה כבר קיים, אנא בדקו את <strong><tt>$1</tt></strong> אם אינכם בטוחים שברצונכם להחליף אותו.',
'filepageexists'              => 'דף תיאור התמונה עבור קובץ זה כבר נוצר ב<strong><tt>$1</tt></strong>, אך לא קיים קובץ בשם זה. תיאור התמונה שתכתבו לא יופיע בדף תיאור התמונה. כדי לגרום לו להופיע שם, יהיה עליכם לערוך אותו ידנית.',
'fileexists-extension'        => 'קובץ עם שם דומה כבר קיים:<br />
שם הקובץ המועלה: <strong><tt>$1</tt></strong><br />
שם הקובץ הקיים: <strong><tt>$2</tt></strong><br />
ההבדל היחיד הוא בשימוש באותיות רישיות וקטנות בסיומת הקובץ. אנא בדקו אם הקבצים זהים.',
'fileexists-thumb'            => "<center>'''תמונה קיימת'''</center>",
'fileexists-thumbnail-yes'    => 'הקובץ עשוי להיות תמונה מוקטנת (ממוזערת). אנא בדקו את הקובץ <strong><tt>$1</tt></strong>.<br />
אם הקובץ שבדקתם הוא אותה התמונה בגודל מקורי, אין זה הכרחי להעלות גם תמונה ממוזערת.',
'file-thumbnail-no'           => 'שם הקובץ מתחיל עם <strong><tt>$1</tt></strong>. נראה שזוהי תמונה מוקטנת (ממוזערת).
אם התמונה בגודל מלא מצויה ברשותכם, אנא העלו אותה ולא את התמונה הממוזערת; אחרת, אנא שנו את שם הקובץ.',
'fileexists-forbidden'        => 'קובץ בשם זה כבר קיים.
אם אתם עדיין מעוניינים להעלות קובץ זה, אנא חזרו לדף הקודם והעלו את הקובץ תחת שם חדש.
[[Image:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => 'קובץ בשם זה כבר קיים כקובץ משותף.
אם אתם עדיין מעוניינים להעלות קובץ זה, אנא חזרו לדף הקודם והעלו את הקובץ תחת שם חדש.
[[Image:$1|thumb|center|$1]]',
'file-exists-duplicate'       => 'קובץ זה כפול ל{{PLURAL:$1|קובץ הבא|קבצים הבאים}}:',
'successfulupload'            => 'העלאת הקובץ הושלמה בהצלחה',
'uploadwarning'               => 'אזהרת העלאת קבצים',
'savefile'                    => 'שמירת קובץ',
'uploadedimage'               => 'העלה את הקובץ [[$1]]',
'overwroteimage'              => 'העלה גרסה חדשה של הקובץ [[$1]]',
'uploaddisabled'              => 'העלאת קבצים מבוטלת',
'uploaddisabledtext'          => 'אפשרות העלאת הקבצים מבוטלת.',
'uploadscripted'              => 'הקובץ כולל קוד סקריפט או HTML שעשוי להתפרש או להתבצע בטעות על ידי הדפדפן.',
'uploadcorrupt'               => 'קובץ זה אינו תקין או שהסיומת שלו איננה מתאימה. אנא בדקו את הקובץ והעלו אותו שוב.',
'uploadvirus'                 => 'הקובץ מכיל וירוס! פרטים: <div style="direction: ltr;">$1</div>',
'sourcefilename'              => 'שם הקובץ:',
'destfilename'                => 'שמור קובץ בשם:',
'upload-maxfilesize'          => 'גודל הקובץ המקסימלי: $1',
'watchthisupload'             => 'מעקב אחרי דף זה',
'filewasdeleted'              => 'קובץ בשם זה כבר הועלה בעבר, ולאחר מכן נמחק. אנא בדקו את הדף $1 לפני שתמשיכו להעלותו שנית.',
'upload-wasdeleted'           => "'''אזהרה: הנכם מעלים קובץ שנמחק בעבר.'''

אנא שיקלו האם יהיה זה נכון להמשיך בהעלאת הקובץ.
יומן המחיקות של הקובץ מוצג להלן:",
'filename-bad-prefix'         => 'שם הקובץ שאתם מעלים מתחיל עם <strong>"$1"</strong>, שהוא שם שאינו מתאר את הקובץ ובדרך כלל מוכנס אוטומטית על ידי מצלמות דיגיטליות. אנא בחרו שם מתאים יותר לקובץ, שיתאר את תכניו.',
'filename-prefix-blacklist'   => ' #<!-- נא להשאיר שורה זו בדיוק כפי שהיא --> <pre>
# התחביר הוא כדלקמן:
#   * כל דבר מתו "#" לסוף השורה הוא הערה
#   * כל שורה לא ריקה היא קידומת לשמות קבצים טיפוסיים שמצלמות דיגיטליות נותנות אוטומטית
CIMG # Casio
DSC_ # Nikon
DSCF # Fuji
DSCN # Nikon
DUW # מספר טלפונים סלולריים
IMG # כללי
JD # Jenoptik
MGP # Pentax
PICT # שונות
 #</pre> <!-- נא להשאיר שורה זו בדיוק כפי שהיא -->',

'upload-proto-error'      => 'פרוטוקול שגוי',
'upload-proto-error-text' => 'בהעלאה מרוחקת, יש להשתמש בכתובות URL המתחילות עם <code>http://</code> או <code>ftp://</code>.',
'upload-file-error'       => 'שגיאה פנימית',
'upload-file-error-text'  => 'שגיאה פנימית התרחשה בעת הניסיון ליצור קובץ זמני על השרת.
אנא צרו קשר עם מנהל מערכת.',
'upload-misc-error'       => 'שגיאת העלאה בלתי ידועה',
'upload-misc-error-text'  => 'שגיאת העלאה בלתי ידועה התרחשה במהלך ההעלאה.
אנא ודאו שכתובת ה־URL תקינה וזמינה ונסו שנית.
אם הבעיה חוזרת על עצמה, אנא צרו קשר עם מנהל המערכת.',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => 'לא ניתן להגיע ל־URL',
'upload-curl-error6-text'  => 'לא ניתן לכתובת ה־URL שנכתבה. אנא בדקו אם כתובת זו נכונה ואם האתר זמין.',
'upload-curl-error28'      => 'הסתיים זמן ההמתנה להעלאה',
'upload-curl-error28-text' => 'לאתר לקח זמן רב מדי לענות. אנא בדקו שהאתר זמין, המתינו מעט ונסו שנית. ייתכן שתרצו לנסות בזמן פחות עמוס.',

'license'            => 'רישיון:',
'nolicense'          => 'אין',
'license-nopreview'  => '(תצוגה מקדימה לא זמינה)',
'upload_source_url'  => ' (כתובת URL תקפה ונגישה)',
'upload_source_file' => ' (קובץ במחשב שלך)',

# Special:ImageList
'imagelist-summary'     => 'דף זה מציג את כל הקבצים שהועלו. כברירת מחדל מוצגים הקבצים האחרונים שהועלו בראש הרשימה. לחיצה על כותרת של עמודה משנה את המיון.',
'imagelist_search_for'  => 'חיפוש תמונה בשם:',
'imgfile'               => 'קובץ',
'imagelist'             => 'רשימת תמונות',
'imagelist_date'        => 'תאריך',
'imagelist_name'        => 'שם',
'imagelist_user'        => 'משתמש',
'imagelist_size'        => 'גודל',
'imagelist_description' => 'תיאור',

# Image description page
'filehist'                       => 'היסטוריית קובץ התמונה',
'filehist-help'                  => 'לחצו על תאריך/שעה כדי לראות את התמונה כפי שהופיעה בעת זו.',
'filehist-deleteall'             => 'מחיקת כל הגרסאות',
'filehist-deleteone'             => 'מחיקה',
'filehist-revert'                => 'שחזור',
'filehist-current'               => 'נוכחית',
'filehist-datetime'              => 'תאריך/שעה',
'filehist-user'                  => 'משתמש',
'filehist-dimensions'            => 'ממדים',
'filehist-filesize'              => 'גודל הקובץ',
'filehist-comment'               => 'הערה',
'imagelinks'                     => 'קישורי תמונות',
'linkstoimage'                   => '{{PLURAL:$1|הדף הבא משתמש|הדפים הבאים משתמשים}} בתמונה זו:',
'nolinkstoimage'                 => 'אין דפים המשתמשים בתמונה זו.',
'morelinkstoimage'               => 'ראו [[Special:WhatLinksHere/$1|דפים נוספים]] המשתמשים בתמונה זו.',
'redirectstofile'                => '{{PLURAL:$1|הדף הבא הוא דף הפניה|הדפים הבאים הם דפי הפניה}} לתמונה זו:',
'duplicatesoffile'               => '{{PLURAL:$1|התמונה הבאה זהה|התמונות הבאות זהות}} לתמונה זו:',
'sharedupload'                   => 'קובץ זה הוא קובץ משותף וניתן להשתמש בו גם באתרים אחרים.',
'shareduploadwiki'               => 'למידע נוסף, ראו את $1.',
'shareduploadwiki-desc'          => 'תיאורו ב$1 המקורי מוצג למטה.',
'shareduploadwiki-linktext'      => 'דף תיאור הקובץ',
'shareduploadduplicate'          => 'קובץ זה הוא העתק של $1.',
'shareduploadduplicate-linktext' => 'קובץ משותף',
'shareduploadconflict'           => 'לקובץ זה יש שם זהה לשם של $1.',
'shareduploadconflict-linktext'  => 'קובץ משותף',
'noimage'                        => 'לא נמצא קובץ בשם זה, אך יש באפשרותכם $1.',
'noimage-linktext'               => 'להעלות אחד',
'uploadnewversion-linktext'      => 'העלו גרסה חדשה של קובץ זה',
'imagepage-searchdupe'           => 'חיפוש קבצים כפולים',

# File reversion
'filerevert'                => 'שחזור $1',
'filerevert-backlink'       => '→ $1',
'filerevert-legend'         => 'שחזור קובץ',
'filerevert-intro'          => "משחזר את '''[[Media:$1|$1]]''' ל[$4 גרסה מ־$3, $2].",
'filerevert-comment'        => 'הערה:',
'filerevert-defaultcomment' => 'שוחזר לגרסה מ־$2, $1',
'filerevert-submit'         => 'שחזור',
'filerevert-success'        => "'''[[Media:$1|$1]]''' שוחזרה ל[$4 גרסה מ־$3, $2].",
'filerevert-badversion'     => 'אין גרסה מקומית קודמת של הקובץ שהועלתה בתאריך המבוקש.',

# File deletion
'filedelete'                  => 'מחיקת $1',
'filedelete-backlink'         => '→ $1',
'filedelete-legend'           => 'מחיקת קובץ',
'filedelete-intro'            => "מוחק את '''[[Media:$1|$1]]'''.",
'filedelete-intro-old'        => "אתם מוחקים את הגרסה של '''[[Media:$1|$1]]''' מ־[$4 $3, $2].",
'filedelete-comment'          => 'סיבה למחיקה:',
'filedelete-submit'           => 'מחיקה',
'filedelete-success'          => "'''$1''' נמחק.",
'filedelete-success-old'      => "הגרסה של '''[[Media:$1|$1]]''' מ־$3, $2 נמחקה.",
'filedelete-nofile'           => "'''$1''' אינו קיים.",
'filedelete-nofile-old'       => "אין גרסה ישנה של '''$1''' עם התכונות המבוקשות.",
'filedelete-iscurrent'        => 'אתם מנסים למחוק את הגרסה החדשה ביותר של הקובץ. אנא שחזרו קודם לגרסה ישנה יותר.',
'filedelete-otherreason'      => 'סיבה נוספת/אחרת:',
'filedelete-reason-otherlist' => 'סיבה אחרת',
'filedelete-reason-dropdown'  => '
* סיבות מחיקה נפוצות
** הפרת זכויות יוצרים
** קובץ כפול',
'filedelete-edit-reasonlist'  => 'עריכת סיבות המחיקה',

# MIME search
'mimesearch'         => 'חיפוש MIME',
'mimesearch-summary' => 'דף זה מאפשר את סינון הקבצים לפי סוג ה־MIME שלהם. סוג ה־MIME בנוי בצורה "סוג תוכן/סוג משני", לדוגמה <tt>image/jpeg</tt>.',
'mimetype'           => 'סוג MIME:',
'download'           => 'הורדה',

# Unwatched pages
'unwatchedpages' => 'דפים שאינם במעקב',

# List redirects
'listredirects' => 'רשימת הפניות',

# Unused templates
'unusedtemplates'     => 'תבניות שאינן בשימוש',
'unusedtemplatestext' => 'דף זה מכיל רשימה של כל הדפים במרחב השם של התבניות שאינם נכללים בדף אחר. אנא זכרו לבדוק את הקישורים האחרים לתבניות לפני שתמחקו אותן.',
'unusedtemplateswlh'  => 'קישורים אחרים',

# Random page
'randompage'         => 'דף אקראי',
'randompage-nopages' => 'אין דפים במרחב השם הזה.',

# Random redirect
'randomredirect'         => 'הפניה אקראית',
'randomredirect-nopages' => 'אין הפניות במרחב השם הזה.',

# Statistics
'statistics'             => 'סטטיסטיקות',
'sitestats'              => 'סטטיסטיקות {{SITENAME}}',
'userstats'              => 'סטטיסטיקות משתמשים',
'sitestatstext'          => "בבסיס הנתונים יש בסך הכול {{PLURAL:$1|דף '''אחד'''|'''$1''' דפים}}. מספר זה כולל דפים שאינם דפי תוכן, כגון דפי שיחה, דפים אודות {{SITENAME}}, קצרמרים, דפי תוכן ללא קישורים פנימיים, הפניות, וכיוצא בזה. אם לא סופרים את הדפים שאינם דפי תוכן, {{PLURAL:$2|נשאר דף '''אחד''' שהוא ככל הנראה דף תוכן לכל דבר|נשארים '''$2''' דפים שהם ככל הנראה דפי תוכן לכל דבר}}.

מאז תחילת פעולתו של האתר, {{PLURAL:$3|הייתה באתר צפייה '''אחת''' בדפים|היו באתר '''$3''' צפיות בדפים}}, {{PLURAL:$4|ובוצעה פעולת עריכה '''אחת'''|ובוצעו '''$4''' פעולות עריכה}}.

בסך הכול {{PLURAL:$5|בוצעה בממוצע עריכה '''אחת''' לדף|בוצעו בממוצע '''$5''' עריכות לדף}}, ו{{PLURAL:$6|הייתה צפייה '''אחת''' לכל עריכה|היו '''$6''' צפיות לכל עריכה}}.

אורך [http://www.mediawiki.org/wiki/Manual:Job_queue תור המשימות] הוא '''$7'''.

{{PLURAL:$1|קובץ '''אחד'''|'''$8''' קבצים}} הועלו לאתר עד כה.",
'userstatstext'          => "{{PLURAL:$1|ישנו [[Special:ListUsers|משתמש רשום]] '''אחד'''|ישנם '''$1''' [[Special:ListUsers|משתמשים רשומים]] באתר}}, {{PLURAL:$2|ול'''אחד'''|ול־'''$2'''}} (או $4%) מתוכם יש הרשאות $5.",
'statistics-mostpopular' => 'הדפים הנצפים ביותר',

'disambiguations'      => 'דפי פירושונים',
'disambiguationspage'  => 'Template:פירושונים',
'disambiguations-text' => "הדפים הבאים מקשרים ל'''דפי פירושונים'''.
עליהם לקשר לדף הנושא הרלוונטי במקום זאת.<br />
הדף נחשב לדף פירושונים אם הוא משתמש בתבנית המקושרת מהדף [[MediaWiki:Disambiguationspage]].",

'doubleredirects'            => 'הפניות כפולות',
'doubleredirectstext'        => 'ההפניות הבאות מפנות לדפי הפניה אחרים. כל שורה מכילה קישור להפניות הראשונה והשנייה, וכן את היעד של ההפניה השנייה, שהיא לרוב היעד האמיתי של ההפניה, אליו אמורה ההפניה הראשונה להצביע.',
'double-redirect-fixed-move' => '[[$1]] הועבר, וכעת הוא הפניה לדף [[$2]]',
'double-redirect-fixer'      => 'מתקן הפניות',

'brokenredirects'        => 'הפניות לא תקינות',
'brokenredirectstext'    => 'ההפניות שלהלן מפנות לדפים שאינם קיימים:',
'brokenredirects-edit'   => '(עריכה)',
'brokenredirects-delete' => '(מחיקה)',

'withoutinterwiki'         => 'דפים ללא קישורי שפה',
'withoutinterwiki-summary' => 'הדפים הבאים אינם מקשרים לגרסאות שלהם בשפות אחרות:',
'withoutinterwiki-legend'  => 'הדפים המתחילים ב…',
'withoutinterwiki-submit'  => 'הצגה',

'fewestrevisions' => 'הדפים בעלי מספר העריכות הנמוך ביותר',

# Miscellaneous special pages
'nbytes'                  => '{{PLURAL:$1|בית אחד|$1 בתים}}',
'ncategories'             => '{{PLURAL:$1|קטגוריה אחת|$1 קטגוריות}}',
'nlinks'                  => '{{PLURAL:$1|קישור אחד|$1 קישורים}}',
'nmembers'                => '{{PLURAL:$1|דף אחד|$1 דפים}}',
'nrevisions'              => '{{PLURAL:$1|גרסה אחת|$1 גרסאות}}',
'nviews'                  => '{{PLURAL:$1|צפייה אחת|$1 צפיות}}',
'specialpage-empty'       => 'אין תוצאות.',
'lonelypages'             => 'דפים יתומים',
'lonelypagestext'         => 'לדפים הבאים אין קישורים מדפים אחרים באתר זה.',
'uncategorizedpages'      => 'דפים חסרי קטגוריה',
'uncategorizedcategories' => 'קטגוריות חסרות קטגוריה',
'uncategorizedimages'     => 'תמונות חסרות קטגוריה',
'uncategorizedtemplates'  => 'תבניות חסרות קטגוריה',
'unusedcategories'        => 'קטגוריות שאינן בשימוש',
'unusedimages'            => 'תמונות שאינן בשימוש',
'popularpages'            => 'הדפים הנצפים ביותר',
'wantedcategories'        => 'קטגוריות מבוקשות',
'wantedpages'             => 'דפים מבוקשים',
'missingfiles'            => 'קבצים חסרים',
'mostlinked'              => 'הדפים המקושרים ביותר',
'mostlinkedcategories'    => 'הקטגוריות המקושרות ביותר',
'mostlinkedtemplates'     => 'התבניות המקושרות ביותר',
'mostcategories'          => 'הדפים מרובי־הקטגוריות ביותר',
'mostimages'              => 'התמונות המקושרות ביותר',
'mostrevisions'           => 'הדפים בעלי מספר העריכות הגבוה ביותר',
'prefixindex'             => 'רשימת הדפים המתחילים ב…',
'shortpages'              => 'דפים קצרים',
'longpages'               => 'דפים ארוכים',
'deadendpages'            => 'דפים ללא קישורים',
'deadendpagestext'        => 'הדפים הבאים אינם מקשרים לדפים אחרים באתר.',
'protectedpages'          => 'דפים מוגנים',
'protectedpages-indef'    => 'הגנות לצמיתות בלבד',
'protectedpagestext'      => 'הדפים הבאים מוגנים מפני עריכה או העברה:',
'protectedpagesempty'     => 'אין כרגע דפים מוגנים עם הפרמטרים הללו.',
'protectedtitles'         => 'כותרות מוגנות',
'protectedtitlestext'     => 'הכותרות הבאות מוגנות מפני יצירה:',
'protectedtitlesempty'    => 'אין כרגע כותרות מוגנות עם הפרמטרים האלה.',
'listusers'               => 'רשימת משתמשים',
'newpages'                => 'דפים חדשים',
'newpages-username'       => 'שם משתמש:',
'ancientpages'            => 'דפים מוזנחים',
'move'                    => 'העברה',
'movethispage'            => 'העברת דף זה',
'unusedimagestext'        => 'רשימת הקבצים שאינם בשימוש באתר. יש למצוא מקום עבור הקבצים או לסמן אותם למחיקה.',
'unusedcategoriestext'    => 'למרות שהקטגוריות הבאות קיימות, אין שום דף בו נעשה בהן שימוש.',
'notargettitle'           => 'אין דף מטרה',
'notargettext'            => 'לא ציינתם דף מטרה או משתמש לגביו תבוצע פעולה זו.',
'nopagetitle'             => 'אין דף מטרה כזה',
'nopagetext'              => 'דף המטרה שציינתם אינו קיים.',
'pager-newer-n'           => '{{PLURAL:$1|הבאה|$1 הבאות}}',
'pager-older-n'           => '{{PLURAL:$1|הקודמת|$1 הקודמות}}',
'suppress'                => 'הסתרה',

# Book sources
'booksources'               => 'משאבי ספרות חיצוניים',
'booksources-search-legend' => 'חיפוש משאבי ספרות חיצוניים',
'booksources-go'            => 'הצגה',
'booksources-text'          => 'להלן רשימת קישורים לאתרים אחרים המוכרים ספרים חדשים ויד־שנייה, ושבהם עשוי להיות מידע נוסף לגבי ספרים שאתם מחפשים:',

# Special:Log
'specialloguserlabel'  => 'משתמש:',
'speciallogtitlelabel' => 'כותרת:',
'log'                  => 'יומנים',
'all-logs-page'        => 'כל היומנים',
'log-search-legend'    => 'חיפוש יומנים',
'log-search-submit'    => 'הצגה',
'alllogstext'          => 'תצוגה משולבת של כל סוגי היומנים הזמינים ב{{grammar:תחילית|{{SITENAME}}}}.
ניתן לצמצם את התצוגה על ידי בחירת סוג היומן, שם המשתמש (תלוי רישיות) או הדף המושפע (גם כן תלוי רישיות).',
'logempty'             => 'אין פריטים תואמים ביומן.',
'log-title-wildcard'   => 'חיפוש כותרות המתחילות באותיות אלה',

# Special:AllPages
'allpages'          => 'כל הדפים',
'alphaindexline'    => '$1 עד $2',
'nextpage'          => 'הדף הבא ($1)',
'prevpage'          => 'הדף הקודם ($1)',
'allpagesfrom'      => 'הצגת דפים החל מ:',
'allarticles'       => 'כל הדפים',
'allinnamespace'    => 'כל הדפים (מרחב שם $1)',
'allnotinnamespace' => 'כל הדפים (שלא במרחב השם $1)',
'allpagesprev'      => 'הקודם',
'allpagesnext'      => 'הבא',
'allpagessubmit'    => 'הצגה',
'allpagesprefix'    => 'הדפים ששמם מתחיל ב…:',
'allpagesbadtitle'  => 'כותרת הדף המבוקש הייתה לא־חוקית, ריקה, קישור ויקי פנימי, או פנים שפה שגוי. ייתכן שהיא כוללת תו אחד או יותר האסורים לשימוש בכותרות.',
'allpages-bad-ns'   => 'אין מרחב שם בשם "$1".',

# Special:Categories
'categories'                    => 'קטגוריות',
'categoriespagetext'            => 'הקטגוריות הבאות כוללות דפים או קובצי מדיה.
[[Special:UnusedCategories|קטגוריות שאינן בשימוש]] אינן מוצגות כאן.
ראו גם את [[Special:WantedCategories|רשימת הקטגוריות המבוקשות]].',
'categoriesfrom'                => 'הצגת קטגוריות החל מ:',
'special-categories-sort-count' => 'סידור לפי מספר חברים',
'special-categories-sort-abc'   => 'סידור לפי סדר האלף בית',

# Special:ListUsers
'listusersfrom'      => 'הצגת משתמשים החל מ:',
'listusers-submit'   => 'הצגה',
'listusers-noresult' => 'לא נמצאו משתמשים.',

# Special:ListGroupRights
'listgrouprights'          => 'רשימת הרשאות לקבוצה',
'listgrouprights-summary'  => 'זוהי רשימה של קבוצות המשתמש המוגדרות באתר זה, עם ההרשאות של כל אחת.
מידע נוסף על ההרשאות ניתן למצוא [[{{MediaWiki:Listgrouprights-helppage}}|כאן]].',
'listgrouprights-group'    => 'קבוצה',
'listgrouprights-rights'   => 'הרשאות',
'listgrouprights-helppage' => 'Help:הרשאות',
'listgrouprights-members'  => '(רשימת חברים)',

# E-mail user
'mailnologin'     => 'אין כתובת לשליחה',
'mailnologintext' => 'עליכם [[Special:UserLogin|להיכנס לחשבון]] ולהגדיר לעצמכם כתובת דואר אלקטרוני תקינה ב[[Special:Preferences|העדפות המשתמש]] שלכם כדי לשלוח דואר למשתמש אחר.',
'emailuser'       => 'שליחת דואר אלקטרוני למשתמש זה',
'emailpage'       => 'שליחת דואר למשתמש',
'emailpagetext'   => 'ניתן לשלוח דואר אלקטרוני דרך טופס זה רק למשתמשים שהזינו כתובת דואר אלקטרוני בהעדפותיהם. טופס זה שולח הודעה אחת.
// כתובת הדואר האלקטרוני שהזנתם ב[[Special:Preferences|העדפות המשתמש שלכם]] תופיע ככתובת ממנה נשלחה ההודעה כדי לאפשר תגובה ישירה למכתב.',
'usermailererror' => 'אוביקט הדואר החזיר שגיאה:',
'defemailsubject' => 'דוא"ל {{SITENAME}}',
'noemailtitle'    => 'אין כתובת דואר אלקטרוני',
'noemailtext'     => 'משתמש זה לא הזין כתובת דואר אלקטרוני חוקית או בחר שלא לקבל דואר אלקטרוני ממשתמשים אחרים.',
'emailfrom'       => 'מאת:',
'emailto'         => 'אל:',
'emailsubject'    => 'נושא:',
'emailmessage'    => 'הודעה:',
'emailsend'       => 'שליחה',
'emailccme'       => 'קבלת העתק של הודעה זו בדואר האלקטרוני.',
'emailccsubject'  => 'העתק של הודעתך למשתמש $1: $2',
'emailsent'       => 'הדואר נשלח',
'emailsenttext'   => 'הודעת הדואר האלקטרוני שלך נשלחה.',
'emailuserfooter' => 'דואר זה נשלח על ידי $1 למשתמש $2 באמצעות תכונת "שליחת דואר אלקטרוני למשתמש זה" ב{{grammar:תחילית|{{SITENAME}}}}.',

# Watchlist
'watchlist'            => 'רשימת המעקב שלי',
'mywatchlist'          => 'רשימת המעקב שלי',
'watchlistfor'         => "(עבור '''$1''')",
'nowatchlist'          => 'אין דפים ברשימת המעקב.',
'watchlistanontext'    => 'עליכם $1 כדי לצפות או לערוך פריטים ברשימת המעקב.',
'watchnologin'         => 'לא נכנסתם לאתר',
'watchnologintext'     => 'עליכם [[Special:UserLogin|להיכנס לחשבון]] כדי לערוך את רשימת המעקב.',
'addedwatch'           => 'הדף נוסף לרשימת המעקב',
'addedwatchtext'       => 'הדף [[:$1]] נוסף ל[[Special:Watchlist|רשימת המעקב]]. שינויים שייערכו בעתיד, בדף זה ובדף השיחה שלו, יוצגו ברשימת המעקב.

בנוסף, הדף יופיע בכתב מודגש ב[[Special:RecentChanges|רשימת השינויים האחרונים]], כדי להקל עליכם את המעקב אחריו.',
'removedwatch'         => 'הדף הוסר מרשימת המעקב',
'removedwatchtext'     => 'הדף [[:$1]] הוסר מ[[Special:Watchlist|רשימת המעקב]].',
'watch'                => 'מעקב',
'watchthispage'        => 'מעקב אחרי דף זה',
'unwatch'              => 'הפסקת מעקב',
'unwatchthispage'      => 'הפסקת המעקב אחרי דף זה',
'notanarticle'         => 'זהו אינו דף תוכן',
'notvisiblerev'        => 'הגרסה נמחקה',
'watchnochange'        => 'אף אחד מהדפים ברשימת המעקב לא עודכן בפרק הזמן המצוין למעלה.',
'watchlist-details'    => 'ברשימת המעקב יש {{PLURAL:$1|דף אחד|$1 דפים}} (לא כולל דפי שיחה).',
'wlheader-enotif'      => '* הודעות דוא"ל מאופשרות.',
'wlheader-showupdated' => "* דפים שהשתנו מאז ביקורכם האחרון בהם מוצגים ב'''הדגשה'''.",
'watchmethod-recent'   => 'בודק את הדפים שברשימת המעקב לשינויים אחרונים.',
'watchmethod-list'     => 'בודק את העריכות האחרונות בדפים שברשימת המעקב',
'watchlistcontains'    => 'רשימת המעקב כוללת {{PLURAL:$1|דף אחד|$1 דפים}}.',
'iteminvalidname'      => 'בעיה עם $1, שם שגוי…',
'wlnote'               => "להלן {{PLURAL:$1|השינוי האחרון|'''$1''' השינויים האחרונים}} {{PLURAL:$2|בשעה האחרונה|ב־'''$2''' השעות האחרונות}}.",
'wlshowlast'           => '(הצגת $1 שעות אחרונות | $2 ימים אחרונים | $3)',
'watchlist-show-bots'  => 'הצגת בוטים',
'watchlist-hide-bots'  => 'הסתרת בוטים',
'watchlist-show-own'   => 'הצגת עריכות שלי',
'watchlist-hide-own'   => 'הסתרת עריכות שלי',
'watchlist-show-minor' => 'הצגת עריכות משניות',
'watchlist-hide-minor' => 'הסתרת עריכות משניות',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'בהוספה לרשימת המעקב…',
'unwatching' => 'בהסרה מרשימת המעקב…',

'enotif_mailer'                => 'הודעות {{SITENAME}}',
'enotif_reset'                 => 'סמן את כל הדפים כאילו נצפו',
'enotif_newpagetext'           => 'זהו דף חדש.',
'enotif_impersonal_salutation' => 'משתמש של {{SITENAME}}',
'changed'                      => 'שונה',
'created'                      => 'נוצר',
'enotif_subject'               => 'הדף $PAGETITLE ב{{grammar:תחילית|{{SITENAME}}}} $CHANGEDORCREATED על ידי $PAGEEDITOR',
'enotif_lastvisited'           => 'ראו $1 לכל השינויים מאז ביקורכם האחרון.',
'enotif_lastdiff'              => 'ראו $1 לשינוי זה.',
'enotif_anon_editor'           => 'משתמש אנונימי $1',
'enotif_body'                  => 'לכבוד $WATCHINGUSERNAME,

הדף $PAGETITLE ב{{grammar:תחילית|{{SITENAME}}}} $CHANGEDORCREATED ב־$PAGEEDITDATE על ידי $PAGEEDITOR, ראו $PAGETITLE_URL לגרסה הנוכחית.

$NEWPAGE

תקציר העריכה: $PAGESUMMARY $PAGEMINOREDIT

באפשרותכם ליצור קשר עם העורך:
בדואר האלקטרוני: $PAGEEDITOR_EMAIL
באתר: $PAGEEDITOR_WIKI

לא תהיינה הודעות על שינויים נוספים עד שתבקרו את הדף. באפשרותכם גם לאפס את דגלי ההודעות בכל הדפים שברשימת המעקב.

             מערכת ההודעות של {{SITENAME}}

--
כדי לשנות את הגדרות רשימת המעקב, בקרו בדף
{{fullurl:Special:Watchlist/edit}}

למשוב ולעזרה נוספת:
{{fullurl:Project:עזרה}}',

# Delete/protect/revert
'deletepage'                  => 'מחיקה',
'confirm'                     => 'אישור',
'excontent'                   => 'תוכן היה: "$1"',
'excontentauthor'             => "תוכן היה: '$1' והתורם היחיד היה [[Special:Contributions/$2|$2]]",
'exbeforeblank'               => 'תוכן לפני שרוקן היה: "$1"',
'exblank'                     => 'הדף היה ריק',
'delete-confirm'              => 'מחיקת $1',
'delete-backlink'             => '→ $1',
'delete-legend'               => 'מחיקה',
'historywarning'              => 'אזהרה – לדף שאתם עומדים למחוק יש היסטוריית שינויים:',
'confirmdeletetext'           => 'אתם עומדים למחוק דף או תמונה יחד עם כל ההיסטוריה שלהם.

אנא אשרו שזה אכן מה שאתם מתכוונים לעשות, שאתם מבינים את התוצאות של מעשה כזה, ושהמעשה מבוצע בהתאם לנהלי האתר.',
'actioncomplete'              => 'הפעולה בוצעה',
'deletedtext'                 => '<strong><nowiki>$1</nowiki></strong> נמחק. ראו $2 לרשימת המחיקות האחרונות.',
'deletedarticle'              => 'מחק את [[$1]]',
'suppressedarticle'           => 'הסתיר את [[$1]]',
'dellogpage'                  => 'יומן מחיקות',
'dellogpagetext'              => 'להלן רשימה של המחיקות האחרונות שבוצעו.',
'deletionlog'                 => 'יומן מחיקות',
'reverted'                    => 'שוחזר לגרסה קודמת',
'deletecomment'               => 'סיבת המחיקה:',
'deleteotherreason'           => 'סיבה נוספת/אחרת:',
'deletereasonotherlist'       => 'סיבה אחרת',
'deletereason-dropdown'       => '
* סיבות מחיקה נפוצות
** לבקשת הכותב
** הפרת זכויות יוצרים
** השחתה',
'delete-edit-reasonlist'      => 'עריכת סיבות המחיקה',
'delete-toobig'               => 'דף זה כולל מעל {{PLURAL:$1|גרסה אחת|$1 גרסאות}} בהיסטוריית העריכות שלו. מחיקת דפים כאלה הוגבלה כדי למנוע פגיעה בביצועי האתר.',
'delete-warning-toobig'       => 'דף זה כולל מעל {{PLURAL:$1|גרסה אחת|$1 גרסאות}} בהיסטוריית העריכות שלו. מחיקה שלו עלולה להפריע לפעולות בבסיס הנתונים; אנא שיקלו שנית את המחיקה.',
'rollback'                    => 'שחזור עריכות',
'rollback_short'              => 'שחזור',
'rollbacklink'                => 'שחזור',
'rollbackfailed'              => 'השחזור נכשל',
'cantrollback'                => 'לא ניתן לשחזר את העריכה – התורם האחרון הוא היחיד שכתב דף זה; עם זאת, ניתן למחוק את הדף.',
'alreadyrolled'               => 'לא ניתן לשחזר את עריכת הדף [[:$1]] על ידי [[User:$2|$2]] ([[User talk:$2|שיחה]] | [[Special:Contributions/$2|{{int:contribslink}}]]); מישהו אחר כבר ערך או שחזר דף זה.

העריכה האחרונה הייתה של [[User:$3|$3]] ([[User talk:$3|שיחה]] | [[Special:Contributions/$3|{{int:contribslink}}]]).',
'editcomment'                 => "תקציר העריכה היה: \"'''\$1'''\".", # only shown if there is an edit comment
'revertpage'                  => 'שוחזר מעריכה של [[Special:Contributions/$2|$2]] ([[User talk:$2|שיחה]]) לעריכה האחרונה של [[User:$1|$1]]', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => 'שוחזר מעריכה של $1 לעריכה האחרונה של $2',
'sessionfailure'              => 'נראה שיש בעיה בחיבורכם לאתר. פעולתכם בוטלה כאמצעי זהירות כנגד התחזות לתקשורת ממחשבכם. אנא חיזרו לדף הקודם ונסו שנית.',
'protectlogpage'              => 'יומן הגנות',
'protectlogtext'              => 'להלן רשימה של הפעלות וביטולי הגנות על דפים. ראו גם את [[Special:ProtectedPages|רשימת הדפים המוגנים]] הנוכחית.',
'protectedarticle'            => 'הפעיל הגנה על [[$1]]',
'modifiedarticleprotection'   => 'שינה את רמת ההגנה של [[$1]]',
'unprotectedarticle'          => 'ביטל את ההגנה על [[$1]]',
'protect-title'               => 'שינוי רמת ההגנה של "$1"',
'protect-backlink'            => '→ $1',
'protect-legend'              => 'אישור הפעלת ההגנה',
'protectcomment'              => 'הערה:',
'protectexpiry'               => 'פקיעת ההגנה:',
'protect_expiry_invalid'      => 'זמן פקיעת ההגנה בלתי חוקי.',
'protect_expiry_old'          => 'זמן פקיעת ההגנה כבר עבר.',
'protect-unchain'             => 'שינוי הרשאות העברה',
'protect-text'                => 'בדף זה תוכלו לראות ולשנות את רמת ההגנה של הדף <strong><nowiki>$1</nowiki></strong>. אנא ודאו שאתם פועלים בהתאם בהתאם לנהלי האתר.',
'protect-locked-blocked'      => 'אינכם יכולים לשנות את רמת ההגנה של הדף בעודכם חסומים.
להלן ההגדרות הנוכחיות עבור הדף <strong>$1</strong>:',
'protect-locked-dblock'       => 'אינכם יכולים לשנות את רמת ההגנה על הדף שכן בסיס הנתונים חסום ברגע זה.
להלן ההגדרות הנוכחיות עבור הדף <strong>$1</strong>:',
'protect-locked-access'       => 'למשתמש שלכם אין הרשאה לשנות את רמת ההגנה של הדף.
להלן ההגדרות הנוכחיות עבור הדף <strong>$1</strong>:',
'protect-cascadeon'           => 'דף זה מוגן כרגע כיוון שהוא מוכלל {{PLURAL:$1|בדף הבא, שמופעלת עליו|בדפים הבאים, שמופעלת עליהם}} הגנה מדורגת. באפשרותכם לשנות את רמת ההגנה על הדף, אך זה לא ישפיע על ההגנה המדורגת.',
'protect-default'             => '(ברירת מחדל)',
'protect-fallback'            => 'משתמשים בעלי הרשאת "$1" בלבד',
'protect-level-autoconfirmed' => 'משתמשים רשומים בלבד',
'protect-level-sysop'         => 'מפעילי מערכת בלבד',
'protect-summary-cascade'     => 'מדורג',
'protect-expiring'            => 'פוקעת $1 (UTC)',
'protect-cascade'             => 'הגנה על כל הדפים המוכללים בדף זה (הגנה מדורגת)',
'protect-cantedit'            => 'אינכם יכולים לשנות את רמת ההגנה על דף זה, כיוון שאין לכם הרשאה לערוך אותו.',
'restriction-type'            => 'הרשאה:',
'restriction-level'           => 'רמת ההגבלה:',
'minimum-size'                => 'גודל מינימלי',
'maximum-size'                => 'גודל מקסימלי:',
'pagesize'                    => '(בבתים)',

# Restrictions (nouns)
'restriction-edit'   => 'עריכה',
'restriction-move'   => 'העברה',
'restriction-create' => 'יצירה',
'restriction-upload' => 'העלאה',

# Restriction levels
'restriction-level-sysop'         => 'הגנה מלאה',
'restriction-level-autoconfirmed' => 'הגנה חלקית',
'restriction-level-all'           => 'כל רמה',

# Undelete
'undelete'                     => 'צפייה בדפים מחוקים',
'undeletepage'                 => 'צפייה ושחזור דפים מחוקים',
'undeletepagetitle'            => "'''זוהי רשימת הגרסאות המחוקות של [[:$1]]'''.",
'viewdeletedpage'              => 'צפייה בדפים מחוקים',
'undeletepagetext'             => 'הדפים שלהלן נמחקו, אך הם עדיין בארכיון וניתן לשחזר אותם. הארכיון מנוקה מעת לעת.',
'undelete-fieldset-title'      => 'שחזור גרסאות',
'undeleteextrahelp'            => 'לשחזור היסטוריית הגרסאות המלאה של הדף, אל תסמנו אף תיבת סימון ולחצו על "שחזור".
לשחזור של גרסאות מסוימות בלבד, סמנו את תיבות הסימון של הגרסאות הללו, ולחצו על "שחזור".
לחיצה על "איפוס" תנקה את התקציר, ואת כל תיבות הסימון.',
'undeleterevisions'            => '{{PLURAL:$1|גרסה אחת נשמרה|$1 גרסאות נשמרו}} בארכיון',
'undeletehistory'              => 'אם תשחזרו את הדף, כל הגרסאות תשוחזרנה להיסטוריית השינויים שלו.
אם יש כבר דף חדש באותו השם, הגרסאות והשינויים יופיעו רק בדף ההיסטוריה שלו.',
'undeleterevdel'               => 'השחזור לא יבוצע אם הגרסה הנוכחית של הדף מחוקה בחלקה. במקרה כזה, עליכם לבטל את ההסתרה של הגרסאות המחוקות החדשות ביותר.',
'undeletehistorynoadmin'       => 'דף זה נמחק. הסיבה למחיקה מוצגת בתקציר מטה, ביחד עם פרטים על המשתמשים שערכו את הדף לפני מחיקתו. הטקסט של גרסאות אלו זמין למפעילי מערכת בלבד.',
'undelete-revision'            => 'גרסה שנמחקה מהדף $1 (מ־$2) מאת $3:',
'undeleterevision-missing'     => 'הגרסה שגויה או חסרה. ייתכן שמדובר בקישור שבור, או שהגרסה שוחזרה או הוסרה מהארכיון.',
'undelete-nodiff'              => 'לא נמצאה גרסה קודמת.',
'undeletebtn'                  => 'שחזור',
'undeletelink'                 => 'שחזור',
'undeletereset'                => 'איפוס',
'undeletecomment'              => 'תקציר:',
'undeletedarticle'             => 'שחזר את [[$1]]',
'undeletedrevisions'           => '{{PLURAL:$1|שוחזרה גרסה אחת|שוחזרו $1 גרסאות}}',
'undeletedrevisions-files'     => 'שחזר {{PLURAL:$1|גרסה אחת|$1 גרסאות}} ו{{PLURAL:$2|קובץ אחד|־$2 קבצים}}',
'undeletedfiles'               => 'שחזר {{PLURAL:$1|קובץ אחד|$1 קבצים}}',
'cannotundelete'               => 'השחזור נכשל; ייתכן שמישהו אחר כבר שחזר את הדף.',
'undeletedpage'                => "'''הדף $1 שוחזר בהצלחה.'''

ראו את [[Special:Log/delete|יומן המחיקות]] לרשימה של מחיקות ושחזורים אחרונים.",
'undelete-header'              => 'ראו את [[Special:Log/delete|יומן המחיקות]] לדפים שנמחקו לאחרונה.',
'undelete-search-box'          => 'חיפוש דפים שנמחקו',
'undelete-search-prefix'       => 'הצגת דפים החל מ:',
'undelete-search-submit'       => 'חיפוש',
'undelete-no-results'          => 'לא נמצאו דפים תואמים בארכיון המחיקות.',
'undelete-filename-mismatch'   => 'שחזור גרסת הקובץ מהתאריך $1 נכשל: שם קובץ לא תואם',
'undelete-bad-store-key'       => 'שחזור גרסת הקובץ מהתאריך $1 נכשל: הקובץ היה חסר לפני המחיקה.',
'undelete-cleanup-error'       => 'שגיאת בעת מחיקת קובץ הארכיון "$1" שאינו בשימוש.',
'undelete-missing-filearchive' => 'שחזור קובץ הארכיון שמספרו $1 נכשל כיוון שהוא אינו בבסיס הנתונים. ייתכן שהוא כבר שוחזר.',
'undelete-error-short'         => 'שגיאה בשחזור הקובץ: $1',
'undelete-error-long'          => 'שגיאות שאירעו בעת שחזור הקובץ:

$1',

# Namespace form on various pages
'namespace'      => 'מרחב שם:',
'invert'         => 'ללא מרחב זה',
'blanknamespace' => '(ראשי)',

# Contributions
'contributions' => 'תרומות המשתמש',
'mycontris'     => 'התרומות שלי',
'contribsub2'   => 'עבור $1 ($2)',
'nocontribs'    => 'לא נמצאו שינויים המתאימים לקריטריונים אלו.',
'uctop'         => '(אחרון)',
'month'         => 'עד החודש:',
'year'          => 'עד השנה:',

'sp-contributions-newbies'     => 'הצגת תרומות של משתמשים חדשים בלבד',
'sp-contributions-newbies-sub' => 'עבור משתמשים חדשים',
'sp-contributions-blocklog'    => 'יומן חסימות',
'sp-contributions-search'      => 'חיפוש תרומות',
'sp-contributions-username'    => 'שם משתמש או כתובת IP:',
'sp-contributions-submit'      => 'חיפוש',

# What links here
'whatlinkshere'            => 'דפים המקושרים לכאן',
'whatlinkshere-title'      => 'דפים המקשרים לדף $1',
'whatlinkshere-page'       => 'דף:',
'linklistsub'              => '(רשימת קישורים)',
'linkshere'                => "הדפים שלהלן מקושרים לדף '''[[:$1]]''':",
'nolinkshere'              => "אין דפים המקושרים לדף '''[[:$1]]'''.",
'nolinkshere-ns'           => "אין דפים המקושרים לדף '''[[:$1]]''' במרחב השם שנבחר.",
'isredirect'               => 'דף הפניה',
'istemplate'               => 'הכללה',
'isimage'                  => 'הצגת תמונה',
'whatlinkshere-prev'       => '{{PLURAL:$1|הקודם|$1 הקודמים}}',
'whatlinkshere-next'       => '{{PLURAL:$1|הבא|$1 הבאים}}',
'whatlinkshere-links'      => '→ קישורים',
'whatlinkshere-hideredirs' => '$1 הפניות',
'whatlinkshere-hidetrans'  => '$1 הכללות',
'whatlinkshere-hidelinks'  => '$1 קישורים',
'whatlinkshere-hideimages' => '$1 הצגות תמונה',
'whatlinkshere-filters'    => 'מסננים',

# Block/unblock
'blockip'                         => 'חסימת משתמש',
'blockip-legend'                  => 'חסימת משתמש',
'blockiptext'                     => 'השתמשו בטופס שלהלן כדי לחסום את הרשאות הכתיבה ממשתמש או כתובת IP ספציפיים.

חסימות כאלה צריכות להתבצע אך ורק כדי למנוע ונדליזם, ובהתאם לנהלי האתר.

אנא פרטו את הסיבה הספציפית לחסימה להלן (לדוגמה, ציון דפים ספציפיים אותם השחית המשתמש).',
'ipaddress'                       => 'כתובת IP:',
'ipadressorusername'              => 'כתובת IP או שם משתמש:',
'ipbexpiry'                       => 'פקיעה:',
'ipbreason'                       => 'סיבה:',
'ipbreasonotherlist'              => 'סיבה אחרת',
'ipbreason-dropdown'              => "
* סיבות חסימה נפוצות
** הוספת מידע שגוי
** הסרת תוכן מדפים
** הצפת קישורים לאתרים חיצוניים
** הוספת שטויות/ג'יבריש לדפים
** התנהגות מאיימת/הטרדה
** שימוש לרעה בחשבונות מרובים
** שם משתמש בעייתי",
'ipbanononly'                     => 'חסימה של משתמשים אנונימיים בלבד',
'ipbcreateaccount'                => 'חסימה של יצירת חשבונות',
'ipbemailban'                     => 'חסימה של שליחת דואר אלקטרוני',
'ipbenableautoblock'              => 'חסימה גם של כתובת ה־IP שלו וכל כתובת IP אחרת שישתמש בה',
'ipbsubmit'                       => 'חסימה',
'ipbother'                        => 'זמן אחר:',
'ipboptions'                      => 'שעתיים:2 hours,יום:1 day,שלושה ימים:3 days,שבוע:1 week,שבועיים:2 weeks,חודש:1 month,שלושה חודשים:3 months,שישה חודשים:6 months,שנה:1 year,לצמיתות:infinite', # display1:time1,display2:time2,...
'ipbotheroption'                  => 'אחר',
'ipbotherreason'                  => 'סיבה אחרת/נוספת:',
'ipbhidename'                     => 'הסתרת שם המשתמש מיומן החסימות, רשימת המשתמשים החסומים ורשימת המשתמשים',
'ipbwatchuser'                    => 'מעקב אחרי דפי המשתמש והשיחה של משתמש זה',
'badipaddress'                    => 'משתמש או כתובת IP שגויים.',
'blockipsuccesssub'               => 'החסימה הושלמה בהצלחה',
'blockipsuccesstext'              => 'המשתמש [[Special:Contributions/$1|$1]] נחסם.

ראו את [[Special:IPBlockList|רשימת המשתמשים החסומים]] כדי לצפות בחסימות.',
'ipb-edit-dropdown'               => 'עריכת סיבות החסימה',
'ipb-unblock-addr'                => 'הסרת חסימה של $1',
'ipb-unblock'                     => 'הסרת חסימה של שם משתמש או כתובת IP',
'ipb-blocklist-addr'              => 'הצגת החסימות הנוכחיות של $1',
'ipb-blocklist'                   => 'הצגת החסימות הנוכחיות',
'unblockip'                       => 'שחרור חסימה',
'unblockiptext'                   => 'השתמשו בטופס שלהלן כדי להחזיר את הרשאות הכתיבה למשתמש או כתובת IP חסומים.',
'ipusubmit'                       => 'שחרור חסימה',
'unblocked'                       => 'המשתמש [[User:$1|$1]] שוחרר מחסימתו.',
'unblocked-id'                    => 'חסימה מספר $1 שוחררה.',
'ipblocklist'                     => 'רשימת כתובות IP ומשתמשים חסומים',
'ipblocklist-legend'              => 'מציאת משתמש חסום',
'ipblocklist-username'            => 'שם משתמש או כתובת IP:',
'ipblocklist-submit'              => 'חיפוש',
'blocklistline'                   => '$1 $2 חסם את $3 ($4)',
'infiniteblock'                   => 'לצמיתות',
'expiringblock'                   => 'פוקע $1',
'anononlyblock'                   => 'משתמשים אנונימיים בלבד',
'noautoblockblock'                => 'חסימה אוטומטית מבוטלת',
'createaccountblock'              => 'יצירת חשבונות נחסמה',
'emailblock'                      => 'שליחת דוא"ל נחסמה',
'ipblocklist-empty'               => 'רשימת המשתמשים החסומים ריקה.',
'ipblocklist-no-results'          => 'שם המשתמש או כתובת ה־IP המבוקשים אינם חסומים.',
'blocklink'                       => 'חסימה',
'unblocklink'                     => 'שחרור חסימה',
'contribslink'                    => 'תרומות',
'autoblocker'                     => 'נחסמתם באופן אוטומטי משום שאתם חולקים את כתובת ה־IP שלכם עם [[User:$1|$1]]. הנימוק לחסימה: "$2".',
'blocklogpage'                    => 'יומן חסימות',
'blocklogentry'                   => 'חסם את [[$1]] למשך $2 $3',
'blocklogtext'                    => 'זהו יומן פעולות החסימה והשחרור של משתמשים. כתובות IP הנחסמות באופן אוטומטי אינן מופיעות.

ראו גם את [[Special:IPBlockList|רשימת המשתמשים החסומים]] הנוכחית.',
'unblocklogentry'                 => 'שחרר את [[$1]]',
'block-log-flags-anononly'        => 'משתמשים אנונימיים בלבד',
'block-log-flags-nocreate'        => 'יצירת חשבונות נחסמה',
'block-log-flags-noautoblock'     => 'חסימה אוטומטית מבוטלת',
'block-log-flags-noemail'         => 'שליחת דוא"ל נחסמה',
'block-log-flags-angry-autoblock' => 'חסימה אוטומטית משוכללת מופעלת',
'range_block_disabled'            => 'היכולת לחסום טווח כתובות איננה פעילה.',
'ipb_expiry_invalid'              => 'זמן פקיעת חסימה בלתי חוקי',
'ipb_expiry_temp'                 => 'חסימות הכוללות הסתרת שם משתמש חייבות להיות לצמיתות.',
'ipb_already_blocked'             => 'המשתמש "$1" כבר נחסם',
'ipb_cant_unblock'                => 'שגיאה: חסימה מספר $1 לא נמצאה. ייתכן שהיא כבר שוחררה.',
'ipb_blocked_as_range'            => 'שגיאה: כתובת ה־IP $1 אינה חסומה ישירות ולכן לא ניתן לשחרר את חסימתה. עם זאת, היא חסומה כחלק מהטווח $2, שניתן לשחרר את חסימתו.',
'ip_range_invalid'                => 'טווח IP שגוי.',
'blockme'                         => 'חסום אותי',
'proxyblocker'                    => 'חוסם פרוקסי',
'proxyblocker-disabled'           => 'אפשרות זו מבוטלת.',
'proxyblockreason'                => 'כתובת ה־IP שלכם נחסמה משום שהיא כתובת פרוקסי פתוחה. אנא צרו קשר עם ספק האינטרנט שלכם והודיעו לו על בעיית האבטחה החמורה הזו.',
'proxyblocksuccess'               => 'בוצע.',
'sorbsreason'                     => 'כתובת ה־IP שלכם רשומה ככתובת פרוקסי פתוחה ב־DNSBL שאתר זה משתמש בו.',
'sorbs_create_account_reason'     => 'כתובת ה־IP שלכם רשומה ככתובת פרוקסי פתוחה ב־DNSBL שאתר זה משתמש בו. אינכם יכולים ליצור חשבון.',

# Developer tools
'lockdb'              => 'נעילת בסיס נתונים',
'unlockdb'            => 'שחרור בסיס נתונים',
'lockdbtext'          => 'נעילת בסיס הנתונים תמנע ממשתמשים את האפשרות לערוך דפים, לשנות את העדפותיהם, לערוך את רשימות המעקב שלהם, ופעולות אחרות הדורשות ביצוע שינויים בבסיס הנתונים.

אנא אשרו שזה מה שאתם מתכוונים לעשות, ושתשחררו את בסיס הנתונים מנעילה כאשר פעולת התחזוקה תסתיים.',
'unlockdbtext'        => 'שחרור בסיס הנתונים מנעילה יחזיר למשתמשים את היכולת לערוך דפים, לשנות את העדפותיהם, לערוך את רשימות המעקב שלהם, ולבצע פעולות אחרות הדורשות ביצוע שינויים בבסיס הנתונים
אנא אשרו שזה מה שבכוונתכם לעשות.',
'lockconfirm'         => 'כן, ברצוני לנעול את בסיס הנתונים.',
'unlockconfirm'       => 'כן, ברצוני לשחרר את בסיס הנתונים מנעילה.',
'lockbtn'             => 'נעילת בסיס הנתונים',
'unlockbtn'           => 'שחרור בסיס הנתונים מנעילה',
'locknoconfirm'       => 'לא סימנתם את תיבת האישור.',
'lockdbsuccesssub'    => 'נעילת בסיס הנתונים הושלמה בהצלחה',
'unlockdbsuccesssub'  => 'שוחררה הנעילה מבסיס הנתונים',
'lockdbsuccesstext'   => 'בסיס הנתונים ננעל.

זכרו [[Special:UnlockDB|לשחרר את הנעילה]] לאחר שפעולת התחזוקה הסתיימה.',
'unlockdbsuccesstext' => 'שוחררה הנעילה של בסיס הנתונים',
'lockfilenotwritable' => 'קובץ נעילת בסיס הנתונים אינו ניתן לכתיבה. כדי שאפשר יהיה לנעול את בסיס הנתונים או לבטל את נעילתו, שרת האינטרנט צריך לקבל הרשאות לכתוב אליו.',
'databasenotlocked'   => 'בסיס הנתונים אינו נעול.',

# Move page
'move-page'               => 'העברת $1',
'move-page-backlink'      => '→ $1',
'move-page-legend'        => 'העברת דף',
'movepagetext'            => "שימוש בטופס שלהלן ישנה את שמו של דף, ויעביר את כל ההיסטוריה שלו לשם חדש.

השם הישן יהפוך לדף הפניה אל הדף עם השם החדש.

באפשרותכם לעדכן אוטומטית דפי הפניה לכותרת המקורית.
אם תבחרו לא לעשות זאת, אנא ודאו שאין [[Special:DoubleRedirects|הפניות כפולות]] או [[Special:BrokenRedirects|שבורות]].

אתם אחראים לוודא שכל הקישורים ממשיכים להצביע למקום שאליו הם אמורים להצביע.

שימו לב: הדף '''לא''' יועבר אם כבר יש דף תחת השם החדש, אלא אם הדף הזה ריק, או שהוא הפניה, ואין לו היסטוריה של שינויים. משמעות הדבר, שאפשר לשנות חזרה את שמו של דף לשם המקורי, אם נעשתה טעות, ולא יימחק דף קיים במערכת.

'''אזהרה:''' שינוי זה עשוי להיות שינוי דרסטי ובלתי צפוי לדף פופולרי; אנא ודאו שאתם מבינים את השלכות המעשה לפני שאתם ממשיכים.",
'movepagetalktext'        => 'דף השיחה של דף זה יועבר אוטומטית, אלא אם:
* קיים דף שיחה שאינו ריק תחת השם החדש אליו מועבר הדף.
* הורדתם את הסימון בתיבה שלהלן.

במקרים אלו, תצטרכו להעביר או לשלב את הדפים באופן ידני, אם תרצו.',
'movearticle'             => 'העברת דף:',
'movenotallowed'          => 'אינכם מורשים להעביר דפים.',
'newtitle'                => 'לשם החדש:',
'move-watch'              => 'מעקב אחרי דף זה',
'movepagebtn'             => 'העברה',
'pagemovedsub'            => 'ההעברה הושלמה בהצלחה',
'movepage-moved'          => '<big>הדף "$1" הועבר לשם "$2".</big>', # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => 'קיים כבר דף עם אותו שם, או שהשם שבחרתם אינו חוקי.
אנא בחרו שם אחר.',
'cantmove-titleprotected' => 'אינכם יכולים להעביר את הדף לשם זה, כיוון שהשם החדש הוגן מוגן העברה',
'talkexists'              => 'הדף עצמו הועבר בהצלחה, אבל דף השיחה לא הועבר כיוון שקיים כבר דף שיחה במיקום החדש. אנא מזגו אותם ידנית.',
'movedto'                 => 'הועבר ל',
'movetalk'                => 'העברה גם של דף השיחה',
'move-subpages'           => 'העברת כל דפי המשנה, אם אפשר',
'move-talk-subpages'      => 'העברת כל דפי המשנה של דף השיחה, אם אפשר',
'movepage-page-exists'    => 'הדף $1 קיים כבר ולא ניתן לדרוס אותו אוטומטית.',
'movepage-page-moved'     => 'הדף $1 הועבר לשם $2.',
'movepage-page-unmoved'   => 'לא ניתן להעביר את הדף $1 לשם $2.',
'movepage-max-pages'      => 'המספר המקסימלי של {{PLURAL:$1|דף אחד|$1 דפים}} כבר הועבר ולא ניתן להעביר דפים נוספים אוטומטית.',
'1movedto2'               => '[[$1]] הועבר ל[[$2]]',
'1movedto2_redir'         => '[[$1]] הועבר ל[[$2]] במקום הפניה',
'movelogpage'             => 'יומן העברות',
'movelogpagetext'         => 'להלן רשימה של העברות דפים.',
'movereason'              => 'סיבה:',
'revertmove'              => 'החזרה',
'delete_and_move'         => 'מחיקה והעברה',
'delete_and_move_text'    => '== בקשת מחיקה ==
דף היעד, [[:$1]], כבר קיים. האם ברצונכם למחוק אותו כדי לאפשר את ההעברה?',
'delete_and_move_confirm' => 'אישור מחיקת הדף',
'delete_and_move_reason'  => 'מחיקה על מנת לאפשר העברה',
'selfmove'                => 'כותרות המקור והיעד זהות; לא ניתן להעביר דף לעצמו.',
'immobile_namespace'      => 'כותרת המקור או היעד היא סוג מיוחד של דף; לא ניתן להעביר דפים לתוך או מתוך מרחב שם זה.',
'imagenocrossnamespace'   => 'לא ניתן להעביר תמונה למרחב שם אחר',
'imagetypemismatch'       => 'סיומת הקובץ החדשה אינה מתאימה לסוג הקובץ',
'imageinvalidfilename'    => 'שם קובץ היעד אינו תקין',
'fix-double-redirects'    => 'עדכון הפניות לכותרת הדף המקורית',

# Export
'export'            => 'ייצוא דפים',
'exporttext'        => 'באפשרותכם לייצא את התוכן ואת היסטוריית העריכה של דף אחד או של מספר דפים, בתבנית של קובץ XML.
ניתן לייבא את הקובץ למיזם ויקי אחר המשתמש בתוכנת מדיה־ויקי באמצעות [[Special:Import|דף הייבוא]].

כדי לייצא דפים, הקישו את שמותיהם בתיבת הטקסט שלהלן, כל שם בשורה נפרדת, ובחרו האם לייצא גם את הגרסה הנוכחית וגם את היסטוריית השינויים של הדפים, או רק את הגרסה הנוכחית עם מידע על העריכה האחרונה.

בנוסף, ניתן להשתמש בקישור, כגון [[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]] לדף [[{{MediaWiki:Mainpage}}]] ללא היסטוריית השינויים שלו.',
'exportcuronly'     => 'כלול רק את הגרסה הנוכחית, ללא כל ההיסטוריה',
'exportnohistory'   => "----
'''הערה:''' ייצוא ההיסטוריה המלאה של דפים דרך טופס זה הופסקה עקב בעיות ביצוע.",
'export-submit'     => 'ייצוא',
'export-addcattext' => 'הוספת דפים מהקטגוריה:',
'export-addcat'     => 'הוספה',
'export-download'   => 'שמירה כקובץ',
'export-templates'  => 'כלול תבניות',

# Namespace 8 related
'allmessages'               => 'הודעות המערכת',
'allmessagesname'           => 'שם',
'allmessagesdefault'        => 'טקסט ברירת מחדל',
'allmessagescurrent'        => 'טקסט נוכחי',
'allmessagestext'           => 'זוהי רשימת כל הודעות המערכת שבמרחב השם {{ns:mediawiki}}, המשמשות את ממשק האתר.

מפעילי המערכת יכולים לערוך את ההודעות בלחיצה על שם ההודעה.',
'allmessagesnotsupportedDB' => 'לא ניתן להשתמש בדף זה כיוון ש־wgUseDatabseMessages מבוטל.',
'allmessagesfilter'         => 'מסנן שמות ההודעות:',
'allmessagesmodified'       => 'רק הודעות ששונו',

# Thumbnails
'thumbnail-more'           => 'הגדל',
'filemissing'              => 'קובץ חסר',
'thumbnail_error'          => 'שגיאה ביצירת תמונה ממוזערת: $1',
'djvu_page_error'          => 'דף ה־DjVu מחוץ לטווח',
'djvu_no_xml'              => 'לא ניתן היה לקבל את ה־XML עבור קובץ ה־DjVu',
'thumbnail_invalid_params' => 'פרמטרים שגויים לתמונה הממוזערת',
'thumbnail_dest_directory' => 'לא ניתן היה ליצור את תיקיית היעד',

# Special:Import
'import'                     => 'ייבוא דפים',
'importinterwiki'            => 'ייבוא בין־אתרי',
'import-interwiki-text'      => 'אנא בחרו אתר ויקי ואת כותרת הדף לייבוא.
תאריכי ועורכי הגרסאות יישמרו בעת הייבוא.
כל פעולות הייבוא הבין־אתרי נשמרות ביומן הייבוא.',
'import-interwiki-history'   => 'העתקת כל היסטוריית העריכות של דף זה',
'import-interwiki-submit'    => 'ייבוא',
'import-interwiki-namespace' => 'העבר את הדפים לתוך מרחב השם:',
'importtext'                 => 'אנא ייצאו את הקובץ מאתר המקור תוך שימוש ב[[Special:Export|כלי הייצוא]], שמרו אותו לדיסק הקשיח שלכם והעלו אותו לכאן.',
'importstart'                => 'מייבא דפים…',
'import-revision-count'      => '{{PLURAL:$1|גרסה אחת|$1 גרסאות}}',
'importnopages'              => 'אין דפים לייבוא.',
'importfailed'               => 'הייבוא נכשל: <nowiki>$1</nowiki>',
'importunknownsource'        => 'סוג ייבוא בלתי ידוע',
'importcantopen'             => 'פתיחת קובץ הייבוא נכשלה',
'importbadinterwiki'         => 'קישור בינוויקי שגוי',
'importnotext'               => 'ריק או חסר טקסט',
'importsuccess'              => 'הייבוא הושלם בהצלחה!',
'importhistoryconflict'      => 'ישנה התנגשות עם ההיסטוריה הקיימת של הדף (ייתכן שהדף יובא בעבר)',
'importnosources'            => 'אין מקורות לייבוא בין־אתרי, וייבוא ישיר של דף עם היסטוריה אינו מאופשר כעת.',
'importnofile'               => 'לא הועלה קובץ ייבוא.',
'importuploaderrorsize'      => 'העלאת קובץ הייבוא נכשלה. הקובץ היה גדול יותר מגודל ההעלאה המותר.',
'importuploaderrorpartial'   => 'העלאת קובץ הייבוא נכשלה. הקובץ הועלה באופן חלקי בלבד.',
'importuploaderrortemp'      => 'העלאת קובץ הייבוא נכשלה. חסרה תיקייה זמנית.',
'import-parse-failure'       => 'שגיאה בפענוח ה־XML',
'import-noarticle'           => 'אין דף לייבוא!',
'import-nonewrevisions'      => 'כל הגרסאות יובאו בעבר.',
'xml-error-string'           => '$1 בשורה $2, עמודה $3 (בייט מספר $4): $5',
'import-upload'              => 'העלאת קובץ XML',

# Import log
'importlogpage'                    => 'יומן ייבוא',
'importlogpagetext'                => 'ייבוא מנהלי של דפים (כולל היסטוריית העריכות שלהם) מאתרי ויקי אחרים.',
'import-logentry-upload'           => 'ייבא את [[$1]] באמצעות העלאת קובץ',
'import-logentry-upload-detail'    => '{{PLURAL:$1|גרסה אחת|$1 גרסאות}}',
'import-logentry-interwiki'        => 'ייבא את $1 בייבוא בין־אתרי',
'import-logentry-interwiki-detail' => '{{PLURAL:$1|גרסה אחת|$1 גרסאות}} של הדף $2',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'דף המשתמש שלי',
'tooltip-pt-anonuserpage'         => 'דף המשתמש של משתמש אנונימי זה',
'tooltip-pt-mytalk'               => 'דף השיחה שלי',
'tooltip-pt-anontalk'             => 'שיחה על תרומות המשתמש האנונימי',
'tooltip-pt-preferences'          => 'ההעדפות שלי',
'tooltip-pt-watchlist'            => 'רשימת הדפים שאתם עוקבים אחרי השינויים בהם',
'tooltip-pt-mycontris'            => 'רשימת התרומות שלי',
'tooltip-pt-login'                => 'מומלץ להירשם, אך אין חובה לעשות כן',
'tooltip-pt-anonlogin'            => 'מומלץ להירשם, אך אין חובה לעשות כן',
'tooltip-pt-logout'               => 'יציאה מהחשבון',
'tooltip-ca-talk'                 => 'שיחה על דף זה',
'tooltip-ca-edit'                 => 'באפשרותכם לערוך דף זה. אנא השתמשו בלחצן "תצוגה מקדימה" לפני השמירה',
'tooltip-ca-addsection'           => 'הוספת פסקה לשיחה זו',
'tooltip-ca-viewsource'           => 'זהו דף מוגן, אך באפשרותכם לצפות במקורו',
'tooltip-ca-history'              => 'גרסאות קודמות של דף זה.',
'tooltip-ca-protect'              => 'הגנה על דף זה',
'tooltip-ca-delete'               => 'מחיקת דף זה',
'tooltip-ca-undelete'             => 'שחזור עריכות שנעשו בדף זה לפני שנמחק',
'tooltip-ca-move'                 => 'העברת דף זה',
'tooltip-ca-watch'                => 'הוספת דף זה לרשימת המעקב',
'tooltip-ca-unwatch'              => 'הסרת דף זה מרשימת המעקב',
'tooltip-search'                  => 'חיפוש ב{{grammar:תחילית|{{SITENAME}}}}',
'tooltip-search-go'               => 'מעבר לדף בשם הזה בדיוק, אם הוא קיים',
'tooltip-search-fulltext'         => 'חיפוש טקסט זה בדפים',
'tooltip-p-logo'                  => 'עמוד ראשי',
'tooltip-n-mainpage'              => 'ביקור בעמוד הראשי',
'tooltip-n-portal'                => 'אודות המיזם, איך תוכלו לעזור, איפה למצוא דברים',
'tooltip-n-currentevents'         => 'מציאת מידע רקע על האירועים האחרונים',
'tooltip-n-recentchanges'         => 'רשימת השינויים האחרונים באתר',
'tooltip-n-randompage'            => 'צפייה בדף תוכן אקראי',
'tooltip-n-help'                  => 'עזרה בשימוש באתר',
'tooltip-t-whatlinkshere'         => 'רשימת כל הדפים המקושרים לכאן',
'tooltip-t-recentchangeslinked'   => 'השינויים האחרונים שבוצעו בדפים המקושרים לכאן',
'tooltip-feed-rss'                => 'הוספת עדכון אוטומטי על ידי RSS',
'tooltip-feed-atom'               => 'הוספת עדכון אוטומטי על ידי Atom',
'tooltip-t-contributions'         => 'צפייה בתרומותיו של משתמש זה',
'tooltip-t-emailuser'             => 'שליחת דואר אלקטרוני למשתמש זה',
'tooltip-t-upload'                => 'העלאת תמונות או קובצי מדיה',
'tooltip-t-specialpages'          => 'רשימת כל הדפים המיוחדים',
'tooltip-t-print'                 => 'גרסה להדפסה של דף זה',
'tooltip-t-permalink'             => 'קישור קבוע לגרסה זו של הדף',
'tooltip-ca-nstab-main'           => 'צפייה בדף התוכן',
'tooltip-ca-nstab-user'           => 'צפייה בדף המשתמש',
'tooltip-ca-nstab-media'          => 'צפייה בפריט המדיה',
'tooltip-ca-nstab-special'        => 'זהו דף מיוחד, אי אפשר לערוך אותו',
'tooltip-ca-nstab-project'        => 'צפייה בדף המיזם',
'tooltip-ca-nstab-image'          => 'צפייה בדף תיאור התמונה',
'tooltip-ca-nstab-mediawiki'      => 'צפייה בהודעת המערכת',
'tooltip-ca-nstab-template'       => 'צפייה בתבנית',
'tooltip-ca-nstab-help'           => 'צפייה בדף העזרה',
'tooltip-ca-nstab-category'       => 'צפייה בדף הקטגוריה',
'tooltip-minoredit'               => 'סימון עריכה זו כמשנית',
'tooltip-save'                    => 'שמירת השינויים שביצעתם',
'tooltip-preview'                 => 'תצוגה מקדימה, אנא השתמשו באפשרות זו לפני השמירה!',
'tooltip-diff'                    => 'צפייה בשינויים שערכתם בטקסט',
'tooltip-compareselectedversions' => 'צפייה בהשוואת שתי גרסאות של דף זה',
'tooltip-watch'                   => 'הוספת דף זה לרשימת המעקב',
'tooltip-recreate'                => 'יצירת הדף מחדש למרות שהוא נמחק',
'tooltip-upload'                  => 'התחלת ההעלאה',

# Stylesheets
'common.css'      => '/* הסגנונות הנכתבים כאן ישפיעו על כל הרקעים */',
'standard.css'    => '/* הסגנונות הנכתבים כאן ישפיעו על הרקע Standard בלבד */',
'nostalgia.css'   => '/* הסגנונות הנכתבים כאן ישפיעו על הרקע Nostalgia בלבד */',
'cologneblue.css' => '/* הסגנונות הנכתבים כאן ישפיעו על הרקע CologneBlue בלבד */',
'monobook.css'    => '/* הסגנונות הנכתבים כאן ישפיעו על הרקע Monobook בלבד */',
'myskin.css'      => '/* הסגנונות הנכתבים כאן ישפיעו על הרקע MySkin בלבד */',
'chick.css'       => '/* הסגנונות הנכתבים כאן ישפיעו על הרקע Chick בלבד */',
'simple.css'      => '/* הסגנונות הנכתבים כאן ישפיעו על הרקע Simple בלבד */',
'modern.css'      => '/* הסגנונות הנכתבים כאן ישפיעו על הרקע Modern בלבד */',

# Scripts
'common.js'      => '/* כל סקריפט JavaScript שנכתב כאן ירוץ עבור כל המשתמשים בכל טעינת עמוד */',
'standard.js'    => '/* כל סקריפט JavaScript שנכתב כאן ירוץ רק עבור המשתמשים ברקע Standard */',
'nostalgia.js'   => '/* כל סקריפט JavaScript שנכתב כאן ירוץ רק עבור המשתמשים ברקע Nostalgia */',
'cologneblue.js' => '/* כל סקריפט JavaScript שנכתב כאן ירוץ רק עבור המשתמשים ברקע CologneBlue */',
'monobook.js'    => '/* כל סקריפט JavaScript שנכתב כאן ירוץ רק עבור המשתמשים ברקע Monobook */',
'myskin.js'      => '/* כל סקריפט JavaScript שנכתב כאן ירוץ רק עבור המשתמשים ברקע MySkin */',
'chick.js'       => '/* כל סקריפט JavaScript שנכתב כאן ירוץ רק עבור המשתמשים ברקע Chick */',
'simple.js'      => '/* כל סקריפט JavaScript שנכתב כאן ירוץ רק עבור המשתמשים ברקע Simple */',
'modern.js'      => '/* כל סקריפט JavaScript שנכתב כאן ירוץ רק עבור המשתמשים ברקע Modern */',

# Metadata
'nodublincore'      => 'Dublin Core RDF metadata מבוטל בשרת זה.',
'nocreativecommons' => 'Creative Commons RDF metadata מבוטל בשרת זה.',
'notacceptable'     => 'האתר לא יכול לספק מידע בפורמט שתוכנת הלקוח יכולה לקרוא.',

# Attribution
'anonymous'        => 'משתמש(ים) אנונימי(ים) של {{SITENAME}}',
'siteuser'         => 'משתמש {{SITENAME}} $1',
'lastmodifiedatby' => 'דף זה שונה לאחרונה בתאריך $2, $1 על ידי $3.', # $1 date, $2 time, $3 user
'othercontribs'    => 'מבוסס על העבודה של $1.',
'others'           => 'אחרים',
'siteusers'        => 'משתמש(י) {{SITENAME}} $1',
'creditspage'      => 'קרדיטים בדף',
'nocredits'        => 'אין קרדיטים זמינים בדף זה.',

# Spam protection
'spamprotectiontitle' => 'מנגנון מסנן הספאם',
'spamprotectiontext'  => 'הדף אותו רצית לשמור נחסם על ידי מסנן הספאם.
הסיבה לכך היא לרוב קישור לאתר חיצוני הנמצא ברשימה השחורה.',
'spamprotectionmatch' => 'הטקסט הבא הוא שגרם להפעלת סינון הספאם: $1',
'spambot_username'    => 'מנקה הספאם של מדיה ויקי',
'spam_reverting'      => 'שחזור לגרסה אחרונה שלא כוללת קישורים ל־$1',
'spam_blanking'       => 'כל הגרסאות כוללות קישורים ל־$1, מרוקן את הדף',

# Info page
'infosubtitle'   => 'מידע על הדף',
'numedits'       => 'מספר עריכות (דף תוכן): $1',
'numtalkedits'   => 'מספר עריכות (דף שיחה): $1',
'numwatchers'    => 'מספר העוקבים אחרי הדף: $1',
'numauthors'     => 'מספר כותבים נפרדים (דף תוכן): $1',
'numtalkauthors' => 'מספר כותבים נפרדים (דף שיחה): $1',

# Math options
'mw_math_png'    => 'תמיד הצג כ־PNG',
'mw_math_simple' => 'HTML אם פשוט מאוד, אחרת PNG',
'mw_math_html'   => 'HTML אם אפשר, אחרת PNG',
'mw_math_source' => 'השארה כקוד TeX (לדפדפני טקסט)',
'mw_math_modern' => 'מומלץ לדפדפנים עדכניים',
'mw_math_mathml' => 'MathML אם אפשר (ניסיוני)',

# Patrolling
'markaspatrolleddiff'                 => 'סימון השינוי כבדוק',
'markaspatrolledtext'                 => 'סימון דף זה כבדוק',
'markedaspatrolled'                   => 'השינוי סומן כבדוק',
'markedaspatrolledtext'               => 'השינוי שבחרתם סומן כבדוק.',
'rcpatroldisabled'                    => 'אפשרות סימון השינויים כבדוקים מבוטלת',
'rcpatroldisabledtext'                => 'התכונה של סימון שינוי כבדוק בשינויים האחרונים מבוטלת.',
'markedaspatrollederror'              => 'לא ניתן לסמן כבדוק',
'markedaspatrollederrortext'          => 'עליכם לציין גרסה שתציינו כבדוקה.',
'markedaspatrollederror-noautopatrol' => 'אינכם מורשים לסמן את השינויים של עצמכם כבדוקים.',

# Patrol log
'patrol-log-page'   => 'יומן שינויים בדוקים',
'patrol-log-header' => 'יומן זה מציג גרסאות שנבדקו.',
'patrol-log-line'   => 'סימן את  $1 בדף $2 כבדוקה $3',
'patrol-log-auto'   => '(אוטומטית)',
'patrol-log-diff'   => 'גרסה $1',

# Image deletion
'deletedrevision'                 => 'מחק גרסה ישנה $1',
'filedeleteerror-short'           => 'שגיאה במחיקת הקובץ: $1',
'filedeleteerror-long'            => 'שגיאות שאירעו בעת מחיקת הקובץ:

$1',
'filedelete-missing'              => 'מחיקת הקובץ "$1" נכשלה, כיוון שהוא אינו קיים.',
'filedelete-old-unregistered'     => 'גרסת הקובץ "$1" אינה רשומה בבסיס הנתונים.',
'filedelete-current-unregistered' => 'הקובץ "$1" אינו רשום בבסיס הנתונים.',
'filedelete-archive-read-only'    => 'השרת אינו יכול לכתוב לתיקיית הארכיון "$1".',

# Browsing diffs
'previousdiff' => '→ מעבר להשוואת הגרסאות הקודמת',
'nextdiff'     => 'מעבר להשוואת הגרסאות הבאה ←',

# Media information
'mediawarning'         => "'''אזהרה:''' קובץ זה עלול להכיל קוד זדוני, שהרצתו עלולה לסכן את המערכת שלכם.<hr />",
'imagemaxsize'         => 'הגבלת תמונות בדפי תיאור תמונה ל:',
'thumbsize'            => 'הקטן לגודל של:',
'widthheightpage'      => '$1×$2, {{PLURAL:$3|דף אחד|$3 דפים}}',
'file-info'            => '(גודל הקובץ: $1, סוג MIME: $2)',
'file-info-size'       => '($1 × $2 פיקסלים, גודל הקובץ: $3, סוג MIME: $4)',
'file-nohires'         => '<small>אין גרסת רזולוציה גבוהה יותר.</small>',
'svg-long-desc'        => '(קובץ SVG, הגודל המקורי: $1 × $2 פיקסלים, גודל הקובץ: $3)',
'show-big-image'       => 'תמונה ברזולוציה גבוהה יותר',
'show-big-image-thumb' => '<small>גודל התצוגה הזו: $1 × $2 פיקסלים</small>',

# Special:NewImages
'newimages'             => 'גלריית תמונות חדשות',
'imagelisttext'         => 'להלן רשימה של {{PLURAL:$1|תמונה אחת|$1 תמונות}}, ממוינות $2:',
'newimages-summary'     => 'דף זה מציג את הקבצים האחרונים שהועלו',
'showhidebots'          => '($1 בוטים)',
'noimages'              => 'אין תמונות.',
'ilsubmit'              => 'חיפוש',
'bydate'                => 'לפי תאריך',
'sp-newimages-showfrom' => 'הצגת תמונות חדשות החל מ־$2, $1',

# Bad image list
'bad_image_list' => 'דרך הכתיבה בהודעה היא כמתואר להלן:

רק פריטי רשימה (שורות המתחילות עם *) נחשבים. הקישור הראשון בשורה חייב להיות קישור לתמונה שאין להציג.
כל הקישורים הבאים באותה השורה נחשבים לחריגים, כלומר לדפים שבהם ניתן להציג את התמונה.',

# Metadata
'metadata'          => 'מידע נוסף על התמונה',
'metadata-help'     => 'קובץ זה מכיל מידע נוסף, שיש להניח שהגיע ממצלמה דיגיטלית או מסורק בו התמונה נוצרה או עברה דיגיטציה. אם הקובץ שונה ממצבו הראשוני, כמה מהנתונים להלן עלולים שלא לשקף באופן מלא את מצב התמונה החדש.',
'metadata-expand'   => 'הצגת פרטים מורחבים',
'metadata-collapse' => 'הסתרת פרטים מורחבים',
'metadata-fields'   => 'שדות המידע הנוסף של EXIF האלה אינם פרטים מורחבים ויוצגו תמיד, לעומת השאר:
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => 'רוחב',
'exif-imagelength'                 => 'גובה',
'exif-bitspersample'               => 'ביטים לרכיב',
'exif-compression'                 => 'תבנית דחיסה',
'exif-photometricinterpretation'   => 'הרכב פיקסלים',
'exif-orientation'                 => 'כיווניות',
'exif-samplesperpixel'             => 'מספר רכיבים',
'exif-planarconfiguration'         => 'סידור מידע',
'exif-ycbcrsubsampling'            => 'הפחתת יחס Y ל־C',
'exif-ycbcrpositioning'            => 'מיקום Y ו־C',
'exif-xresolution'                 => 'רזולוציה אופקית',
'exif-yresolution'                 => 'רזולוציה אנכית',
'exif-resolutionunit'              => 'יחידות מידה של רזולוציות X ו־Y',
'exif-stripoffsets'                => 'מיקום מידע התמונה',
'exif-rowsperstrip'                => 'מספר השורות לרצועה',
'exif-stripbytecounts'             => 'בייטים לרצועה דחוסה',
'exif-jpeginterchangeformat'       => 'יחס ל־JPEG SOI',
'exif-jpeginterchangeformatlength' => 'בייטים של מידע JPEG',
'exif-transferfunction'            => 'פונקציית העברה',
'exif-whitepoint'                  => 'נקודה לבנה צבעונית',
'exif-primarychromaticities'       => 'צבעוניות ה־Primarity',
'exif-ycbcrcoefficients'           => 'מקדמי פעולת הטרנספורמציה של מרחב הצבע',
'exif-referenceblackwhite'         => 'זוג ערכי התייחסות לשחור ולבן',
'exif-datetime'                    => 'תאריך ושעת שינוי הקובץ',
'exif-imagedescription'            => 'כותרת התמונה',
'exif-make'                        => 'יצרן המצלמה',
'exif-model'                       => 'דגם המצלמה',
'exif-software'                    => 'תוכנה בשימוש',
'exif-artist'                      => 'מחבר',
'exif-copyright'                   => 'בעל זכויות היוצרים',
'exif-exifversion'                 => 'גרסת Exif',
'exif-flashpixversion'             => 'גרסת Flashpix נתמכת',
'exif-colorspace'                  => 'מרחב הצבע',
'exif-componentsconfiguration'     => 'משמעות כל רכיב',
'exif-compressedbitsperpixel'      => 'שיטת דחיסת התמונה',
'exif-pixelydimension'             => 'רוחב התמונה הנכון',
'exif-pixelxdimension'             => 'גובה התמונה הנכון',
'exif-makernote'                   => 'הערות היצרן',
'exif-usercomment'                 => 'הערות המשתמש',
'exif-relatedsoundfile'            => 'קובץ שמע מקושר',
'exif-datetimeoriginal'            => 'תאריך ושעת יצירת הקובץ',
'exif-datetimedigitized'           => 'תאריך ושעת הפיכת הקובץ לדיגיטלי',
'exif-subsectime'                  => 'תת־השניות של שינוי הקובץ',
'exif-subsectimeoriginal'          => 'תת־השניות של יצירת הקובץ',
'exif-subsectimedigitized'         => 'תת־השניות של הפיכת הקובץ לדיגיטלי',
'exif-exposuretime'                => 'זמן חשיפה',
'exif-exposuretime-format'         => '$1 שניות ($2)',
'exif-fnumber'                     => 'מספר F',
'exif-exposureprogram'             => 'תוכנת החשיפה',
'exif-spectralsensitivity'         => 'רגישות הספקטרום',
'exif-isospeedratings'             => 'דירוג מהירות ה־ISO',
'exif-oecf'                        => 'מקדם המרה אופטו־אלקטרוני',
'exif-shutterspeedvalue'           => 'מהירות צמצם',
'exif-aperturevalue'               => 'פתח',
'exif-brightnessvalue'             => 'בהירות',
'exif-exposurebiasvalue'           => 'נטיית החשיפה',
'exif-maxaperturevalue'            => 'גודל הפתח המקסימאלי',
'exif-subjectdistance'             => 'נושא המרחק',
'exif-meteringmode'                => 'שיטת מדידה',
'exif-lightsource'                 => 'מקור אור',
'exif-flash'                       => 'פלש',
'exif-focallength'                 => 'אורך מוקדי העדשות',
'exif-focallength-format'          => '$1 מ"מ',
'exif-subjectarea'                 => 'נושא האזור',
'exif-flashenergy'                 => 'אנרגיית הפלש',
'exif-spatialfrequencyresponse'    => 'תדירות התגובה המרחבית',
'exif-focalplanexresolution'       => 'משטח הפוקוס ברזולוציה האופקית',
'exif-focalplaneyresolution'       => 'משטח הפוקוס ברזולוציה האנכית',
'exif-focalplaneresolutionunit'    => 'יחידת המידה של משטח הפוקוס ברזולוציה',
'exif-subjectlocation'             => 'נושא המיקום',
'exif-exposureindex'               => 'מדד החשיפה',
'exif-sensingmethod'               => 'שיטת חישה',
'exif-filesource'                  => 'מקור הקובץ',
'exif-scenetype'                   => 'סוג הסצנה',
'exif-cfapattern'                  => 'תבנית CFA',
'exif-customrendered'              => 'עיבוד תמונה מותאם',
'exif-exposuremode'                => 'מצב החשיפה',
'exif-whitebalance'                => 'איזון צבע לבן',
'exif-digitalzoomratio'            => 'יחס הזום הדיגיטלי',
'exif-focallengthin35mmfilm'       => 'אורך מוקדי העדשות בסרט צילום של 35 מ"מ',
'exif-scenecapturetype'            => 'אופן צילום הסצנה',
'exif-gaincontrol'                 => 'בקרת הסצנה',
'exif-contrast'                    => 'ניגוד',
'exif-saturation'                  => 'רוויה',
'exif-sharpness'                   => 'חדות',
'exif-devicesettingdescription'    => 'תיאור הגדרות ההתקן',
'exif-subjectdistancerange'        => 'טווח נושא המרחק',
'exif-imageuniqueid'               => 'מזהה תמונה ייחודי',
'exif-gpsversionid'                => 'גרסת תגי GPS',
'exif-gpslatituderef'              => 'קו־רוחב צפוני או דרומי',
'exif-gpslatitude'                 => 'קו־רוחב',
'exif-gpslongituderef'             => 'קו־אורך מזרחי או מערבי',
'exif-gpslongitude'                => 'קו־אורך',
'exif-gpsaltituderef'              => 'התייחסות גובה',
'exif-gpsaltitude'                 => 'גובה',
'exif-gpstimestamp'                => 'זמן GPS (שעון אטומי)',
'exif-gpssatellites'               => 'לוויינים ששמשו למדידה',
'exif-gpsstatus'                   => 'מעמד המקלט',
'exif-gpsmeasuremode'              => 'מצב מדידה',
'exif-gpsdop'                      => 'דיוק מדידה',
'exif-gpsspeedref'                 => 'יחידת מהירות',
'exif-gpsspeed'                    => 'יחידת מהירות של מקלט GPS',
'exif-gpstrackref'                 => 'התייחסות מהירות התנועה',
'exif-gpstrack'                    => 'מהירות התנועה',
'exif-gpsimgdirectionref'          => 'התייחסות כיוון התמונה',
'exif-gpsimgdirection'             => 'כיוון התמונה',
'exif-gpsmapdatum'                 => 'מידע סקר מדידת הארץ שנעשה בו שימוש',
'exif-gpsdestlatituderef'          => 'התייחסות קו־הרוחב של היעד',
'exif-gpsdestlatitude'             => 'קו־הרוחב של היעד',
'exif-gpsdestlongituderef'         => 'התייחסות קו־האורך של היעד',
'exif-gpsdestlongitude'            => 'קו־האורך של היעד',
'exif-gpsdestbearingref'           => 'התייחסות כיוון היעד',
'exif-gpsdestbearing'              => 'כיוון היעד',
'exif-gpsdestdistanceref'          => 'התייחסות מרחק ליעד',
'exif-gpsdestdistance'             => 'מרחק ליעד',
'exif-gpsprocessingmethod'         => 'שם שיטת העיבוד של ה־GPS',
'exif-gpsareainformation'          => 'שם אזור ה־GPS',
'exif-gpsdatestamp'                => 'תאריך ה־GPS',
'exif-gpsdifferential'             => 'תיקון דיפרנציאלי של ה־GPS',

# EXIF attributes
'exif-compression-1' => 'לא דחוס',

'exif-unknowndate' => 'תאריך בלתי ידוע',

'exif-orientation-1' => 'רגילה', # 0th row: top; 0th column: left
'exif-orientation-2' => 'הפוך אופקית', # 0th row: top; 0th column: right
'exif-orientation-3' => 'מסובב 180°', # 0th row: bottom; 0th column: right
'exif-orientation-4' => 'הפוך אנכית', # 0th row: bottom; 0th column: left
'exif-orientation-5' => 'מסובב 90° נגד כיוון השעון והפוך אנכית', # 0th row: left; 0th column: top
'exif-orientation-6' => 'מסובב 90° עם כיוון השעון', # 0th row: right; 0th column: top
'exif-orientation-7' => 'מסובב 90° עם כיוון השעון והפוך אנכית', # 0th row: right; 0th column: bottom
'exif-orientation-8' => 'מסובב 90° נגד כיוון השעון', # 0th row: left; 0th column: bottom

'exif-planarconfiguration-1' => 'פורמט חסון',
'exif-planarconfiguration-2' => 'פורמט שטוח',

'exif-componentsconfiguration-0' => 'אינו קיים',

'exif-exposureprogram-0' => 'לא הוגדרה',
'exif-exposureprogram-1' => 'ידנית',
'exif-exposureprogram-2' => 'תוכנה רגילה',
'exif-exposureprogram-3' => 'עדיפות פתח',
'exif-exposureprogram-4' => 'עדיפות צמצם',
'exif-exposureprogram-5' => 'תוכנה יוצרת (מטה לכיוון עומק השדה)',
'exif-exposureprogram-6' => 'תוכנה פועלת (מטה לכיוון מהירות צמצם גבוהה)',
'exif-exposureprogram-7' => 'מצב דיוקן (לתמונות צילום מקרוב כשהרקע לא בפוקוס)',
'exif-exposureprogram-8' => 'מצב נוף (לתמונות נוף כשהרקע בפוקוס)',

'exif-subjectdistance-value' => '$1 מטרים',

'exif-meteringmode-0'   => 'לא ידוע',
'exif-meteringmode-1'   => 'ממוצע',
'exif-meteringmode-2'   => 'מרכז משקל ממוצע',
'exif-meteringmode-3'   => 'נקודה',
'exif-meteringmode-4'   => 'רב־נקודה',
'exif-meteringmode-5'   => 'תבנית',
'exif-meteringmode-6'   => 'חלקי',
'exif-meteringmode-255' => 'אחר',

'exif-lightsource-0'   => 'לא ידוע',
'exif-lightsource-1'   => 'אור יום',
'exif-lightsource-2'   => 'פלואורסצנטי',
'exif-lightsource-3'   => 'טונגסטן (אור מתלהט)',
'exif-lightsource-4'   => 'פלש',
'exif-lightsource-9'   => 'מזג אוויר טוב',
'exif-lightsource-10'  => 'מזג אוויר מעונן',
'exif-lightsource-11'  => 'צל',
'exif-lightsource-12'  => 'אור יום פלואורסצנטי (D 5700 – 7100K)',
'exif-lightsource-13'  => 'אור יום לבן פלואורסצנטי (N 4600 – 5400K)',
'exif-lightsource-14'  => 'אור יום קריר לבן פלואורסצנטי (W 3900 – 4500K)',
'exif-lightsource-15'  => 'פלואורסצנטי לבן (WW 3200 – 3700K)',
'exif-lightsource-17'  => 'אור רגיל A',
'exif-lightsource-18'  => 'אור רגיל B',
'exif-lightsource-19'  => 'אור רגיל C',
'exif-lightsource-24'  => 'טונגסטן אולפן ISO',
'exif-lightsource-255' => 'מקור אור אחר',

'exif-focalplaneresolutionunit-2' => "אינצ'ים",

'exif-sensingmethod-1' => 'לא מוגדרת',
'exif-sensingmethod-2' => 'חיישן אזור בצבע עם שבב אחד',
'exif-sensingmethod-3' => 'חיישן אזור בצבע עם שני שבבים',
'exif-sensingmethod-4' => 'חיישן אזור בצבע עם שלושה שבבים',
'exif-sensingmethod-5' => 'חיישן אזור עם צבע רציף',
'exif-sensingmethod-7' => 'חיישן טריליניארי',
'exif-sensingmethod-8' => 'חיישן עם צבע רציף ליניארי',

'exif-scenetype-1' => 'תמונה שצולמה ישירות',

'exif-customrendered-0' => 'תהליך רגיל',
'exif-customrendered-1' => 'תהליך מותאם',

'exif-exposuremode-0' => 'חשיפה אוטומטית',
'exif-exposuremode-1' => 'חשיפה ידנית',
'exif-exposuremode-2' => 'מסגרת אוטומטית',

'exif-whitebalance-0' => 'איזון צבע לבן אוטומטי',
'exif-whitebalance-1' => 'איזון צבע לבן ידני',

'exif-scenecapturetype-0' => 'רגיל',
'exif-scenecapturetype-1' => 'נוף',
'exif-scenecapturetype-2' => 'דיוקן',
'exif-scenecapturetype-3' => 'סצנה לילית',

'exif-gaincontrol-0' => 'ללא',
'exif-gaincontrol-1' => 'תוספת נמוכה למעלה',
'exif-gaincontrol-2' => 'תוספת גבוהה למעלה',
'exif-gaincontrol-3' => 'תוספת נמוכה למטה',
'exif-gaincontrol-4' => 'תוספת גבוהה למטה',

'exif-contrast-0' => 'רגיל',
'exif-contrast-1' => 'רך',
'exif-contrast-2' => 'קשה',

'exif-saturation-0' => 'רגילה',
'exif-saturation-1' => 'רוויה נמוכה',
'exif-saturation-2' => 'רוויה גבוהה',

'exif-sharpness-0' => 'רגילה',
'exif-sharpness-1' => 'רכה',
'exif-sharpness-2' => 'קשה',

'exif-subjectdistancerange-0' => 'לא ידוע',
'exif-subjectdistancerange-1' => 'מאקרו',
'exif-subjectdistancerange-2' => 'תצוגה קרובה',
'exif-subjectdistancerange-3' => 'תצוגה רחוקה',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'קו־רוחב צפוני',
'exif-gpslatitude-s' => 'קו־רוחב דרומי',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'קו־אורך מזרחי',
'exif-gpslongitude-w' => 'קו־אורך מערבי',

'exif-gpsstatus-a' => 'מדידה בתהליך',
'exif-gpsstatus-v' => 'מדידה בו־זמנית',

'exif-gpsmeasuremode-2' => 'מדידה בשני ממדים',
'exif-gpsmeasuremode-3' => 'מדידה בשלושה ממדים',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'קילומטרים בשעה',
'exif-gpsspeed-m' => 'מיילים בשעה',
'exif-gpsspeed-n' => 'מיילים ימיים בשעה',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => 'כיוון אמיתי',
'exif-gpsdirection-m' => 'כיוון מגנטי',

# External editor support
'edit-externally'      => 'ערכו קובץ זה באמצעות יישום חיצוני',
'edit-externally-help' => 'ראו את [http://www.mediawiki.org/wiki/Manual:External_editors הוראות ההתקנה] למידע נוסף.',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'הכול',
'imagelistall'     => 'הכול',
'watchlistall2'    => 'הכול',
'namespacesall'    => 'הכול',
'monthsall'        => 'הכול',

# E-mail address confirmation
'confirmemail'             => 'אימות כתובת דוא"ל',
'confirmemail_noemail'     => 'אין לכם כתובת דוא"ל תקפה המוגדרת ב[[Special:Preferences|העדפות המשתמש]] שלכם.',
'confirmemail_text'        => 'אתר זה דורש שתאמתו את כתובת הדוא"ל שלכם לפני שתשתמשו בשירותי הדוא"ל. לחצו על הכפתור למטה כדי לשלוח דוא"ל עם קוד אישור לכתובת הדוא"ל שהזנתם. טענו את הקישור בדפדפן שלכם כדי לאשר שכתובת הדוא"ל תקפה.',
'confirmemail_pending'     => '<div class="error">קוד אישור דוא"ל כבר נשלח אליכם; אם יצרתם את החשבון לאחרונה, ייתכן שתרצו לחכות מספר דקות עד שיגיע לפני שתנסו לבקש קוד חדש.</div>',
'confirmemail_send'        => 'שלח קוד אישור',
'confirmemail_sent'        => 'הדוא"ל עם קוד האישור נשלח.',
'confirmemail_oncreate'    => 'קוד אישור דוא"ל נשלח לכתובת הדוא"ל שלכם. הקוד הזה אינו נדרש לכניסה, אך תצטרכו לספקו כדי להשתמש בכל תכונה מבוססת דוא"ל באתר זה.',
'confirmemail_sendfailed'  => '{{SITENAME}} לא הצליח לשלוח לכם הודעת דוא"ל עם קוד האישור.
אנא בדקו שאין תווים שגויים בכתובת הדוא"ל.

תוכנת שליחת הדוא"ל החזירה את ההודעה הבאה: $1',
'confirmemail_invalid'     => 'קוד האישור שגוי. ייתכן שפג תוקפו.',
'confirmemail_needlogin'   => 'עליכם לבצע $1 כדי לאמת את כתובת הדוא"ל שלכם.',
'confirmemail_success'     => 'כתובת הדוא"ל שלכם אושרה.
כעת באפשרותכם [[Special:UserLogin|להיכנס לחשבון שלכם]] וליהנות מהאתר.',
'confirmemail_loggedin'    => 'כתובת הדוא"ל שלכם אושרה כעת.',
'confirmemail_error'       => 'שגיאה בשמירת קוד האישור.',
'confirmemail_subject'     => 'קוד אישור דוא"ל מ{{grammar:תחילית|{{SITENAME}}}}',
'confirmemail_body'        => 'מישהו, כנראה אתם (מכתובת ה־IP הזו: $1), רשם את החשבון "$2" עם כתובת הדוא"ל הזו ב{{grammar:תחילית|{{SITENAME}}}}.

כדי לוודא שחשבון זה באמת שייך לכם ולהפעיל את שירותי הדוא"ל באתר, אנא פתחו את הכתובת הבאה בדפדפן שלכם:

$3

אם *לא* אתם רשמתם את החשבון, השתמשו בקישור זה כדי לבטל את אימות כתובת הדוא"ל:

$5

קוד האישור יפקע ב־$4.',
'confirmemail_invalidated' => 'אימות כתובת הדוא"ל בוטל',
'invalidateemail'          => 'ביטול האימות של כתובת הדוא"ל',

# Scary transclusion
'scarytranscludedisabled' => '[הכללת תבניות בין אתרים מבוטלת]',
'scarytranscludefailed'   => '[הכללת התבנית נכשלה בגלל $1]',
'scarytranscludetoolong'  => '[כתובת ה־URL ארוכה מדי]',

# Trackbacks
'trackbackbox'      => '<div id="mw_trackbacks">
טרקבקים לדף זה:<br />
$1
</div>',
'trackbackremove'   => ' ([$1 מחיקה])',
'trackbacklink'     => 'טרקבק',
'trackbackdeleteok' => 'הטרקבק נמחק בהצלחה.',

# Delete conflict
'deletedwhileediting' => "'''אזהרה''': דף זה נמחק לאחר שהתחלתם לערוך!",
'confirmrecreate'     => "הדף נמחק על ידי המשתמש [[User:$1|$1]] ([[User talk:$1|שיחה]]) לאחר שהתחלתם לערוך אותו, מסיבה זו:
:'''$2'''
אנא אשרו שאתם אכן רוצים ליצור מחדש את הדף.",
'recreate'            => 'יצירה מחדש',

# HTML dump
'redirectingto' => 'מפנה ל־[[:$1]]…',

# action=purge
'confirm_purge'        => 'לנקות את המטמון של דף זה?

$1',
'confirm_purge_button' => 'אישור',

# AJAX search
'searchcontaining' => "חיפוש דפים המכילים את הטקסט '''$1'''.",
'searchnamed'      => "חיפוש דפים בשם '''$1'''.",
'articletitles'    => "חיפוש דפים המתחילים עם '''$1'''",
'hideresults'      => 'הסתרת התוצאות',
'useajaxsearch'    => 'שימוש בחיפוש AJAX',

# Multipage image navigation
'imgmultipageprev' => '→ לדף הקודם',
'imgmultipagenext' => 'לדף הבא ←',
'imgmultigo'       => 'הצגה',
'imgmultigoto'     => 'מעבר לדף $1',

# Table pager
'ascending_abbrev'         => 'עולה',
'descending_abbrev'        => 'יורד',
'table_pager_next'         => 'הדף הבא',
'table_pager_prev'         => 'הדף הקודם',
'table_pager_first'        => 'הדף הראשון',
'table_pager_last'         => 'הדף האחרון',
'table_pager_limit'        => 'הצגת $1 פריטים בדף',
'table_pager_limit_submit' => 'הצגה',
'table_pager_empty'        => 'ללא תוצאות',

# Auto-summaries
'autosumm-blank'   => 'הסרת כל התוכן מדף זה',
'autosumm-replace' => "החלפת הדף עם '$1'",
'autoredircomment' => 'הפניה לדף [[$1]]',
'autosumm-new'     => 'דף חדש: $1',

# Size units
'size-bytes'     => '$1 בייט',
'size-kilobytes' => '$1 קילו־בייט',
'size-megabytes' => '$1 מגה־בייט',
'size-gigabytes' => "$1 ג'יגה־בייט",

# Live preview
'livepreview-loading' => 'בטעינה…',
'livepreview-ready'   => 'בטעינה… נטען!',
'livepreview-failed'  => 'התצוגה המקדימה המהירה נכשלה! נסו להשתמש בתצוגה מקדימה רגילה.',
'livepreview-error'   => 'ההתחברות נכשלה: $1 "$2". נסו להשתמש בתצוגה מקדימה רגילה.',

# Friendlier slave lag warnings
'lag-warn-normal' => 'שינויים שבוצעו לפני פחות מ{{PLURAL:$1|שנייה אחת|־$1 שניות}} אינם מוצגים ברשימה זו.',
'lag-warn-high'   => 'בגלל עיכוב בעדכון בסיס הנתונים, שינויים שבוצעו לפני פחות מ{{PLURAL:$1|שנייה אחת|־$1 שניות}} אינם מוצגים ברשימה זו.',

# Watchlist editor
'watchlistedit-numitems'       => 'יש לכם {{PLURAL:$1|פריט אחד|$1 פריטים}} ברשימת המעקב, לא כולל דפי שיחה.',
'watchlistedit-noitems'        => 'רשימת המעקב ריקה.',
'watchlistedit-normal-title'   => 'עריכת רשימת המעקב',
'watchlistedit-normal-legend'  => 'הסרת דפים מרשימת המעקב',
'watchlistedit-normal-explain' => 'כל הדפים ברשימת המעקב מוצגים להלן. כדי להסיר דף, יש לסמן את התיבה לידו, וללחוץ על "הסרת הדפים". באפשרותכם גם [[Special:Watchlist/raw|לערוך את הרשימה הגולמית]].',
'watchlistedit-normal-submit'  => 'הסרת הדפים',
'watchlistedit-normal-done'    => '{{PLURAL:$1|כותרת אחת הוסרה|$1 כותרות הוסרו}} מרשימת המעקב:',
'watchlistedit-raw-title'      => 'עריכת הרשימה הגולמית',
'watchlistedit-raw-legend'     => 'עריכת הרשימה הגולמית',
'watchlistedit-raw-explain'    => 'הדפים ברשימת המעקב מוצגים להלן, וניתן לערוך אותם באמצעות הוספה והסרה שלהם מהרשימה; כל כותרת מופיעה בשורה נפרדת. לאחר סיום העריכה, יש ללחוץ על "עדכון הרשימה". באפשרותכם גם [[Special:Watchlist/edit|להשתמש בעורך הרגיל]].',
'watchlistedit-raw-titles'     => 'דפים:',
'watchlistedit-raw-submit'     => 'עדכון הרשימה',
'watchlistedit-raw-done'       => 'רשימת המעקב עודכנה.',
'watchlistedit-raw-added'      => '{{PLURAL:$1|כותרת אחת נוספה|$1 כותרות נוספו}}:',
'watchlistedit-raw-removed'    => '{{PLURAL:$1|כותרת אחת הוסרה|$1 כותרות הוסרו}}:',

# Watchlist editing tools
'watchlisttools-view' => 'הצגת השינויים הרלוונטיים',
'watchlisttools-edit' => 'הצגה ועריכה של רשימת המעקב',
'watchlisttools-raw'  => 'עריכת הרשימה הגולמית',

# Iranian month names
'iranian-calendar-m1'  => 'פרברדין',
'iranian-calendar-m2'  => 'אורדיבהשט',
'iranian-calendar-m3'  => 'חורדאד',
'iranian-calendar-m4'  => 'תיר',
'iranian-calendar-m5'  => 'מורדאד',
'iranian-calendar-m6'  => 'שהריבר',
'iranian-calendar-m7'  => 'מהר',
'iranian-calendar-m8'  => 'אבן',
'iranian-calendar-m9'  => 'אזר',
'iranian-calendar-m10' => 'די',
'iranian-calendar-m11' => 'בהמן',
'iranian-calendar-m12' => 'אספנד',

# Hijri month names
'hijri-calendar-m1'  => 'מוחרם',
'hijri-calendar-m2'  => 'צפר',
'hijri-calendar-m3'  => 'רבּיע אל-אוול',
'hijri-calendar-m4'  => "רבּיע א-ת'אני",
'hijri-calendar-m5'  => "ג'ומאדא אל-אוּלא",
'hijri-calendar-m6'  => "ג'ומאדא א-ת'אניה",
'hijri-calendar-m7'  => "רג'בּ",
'hijri-calendar-m8'  => 'שעבּאן',
'hijri-calendar-m9'  => 'רמדאן',
'hijri-calendar-m10' => 'שוואל',
'hijri-calendar-m11' => "ד'ו אל-קעדה",
'hijri-calendar-m12' => "ד'ו אל-חיג'ה",

# Hebrew month names
'hebrew-calendar-m1'      => 'תשרי',
'hebrew-calendar-m2'      => 'חשוון',
'hebrew-calendar-m3'      => 'כסלו',
'hebrew-calendar-m4'      => 'טבת',
'hebrew-calendar-m5'      => 'שבט',
'hebrew-calendar-m6'      => 'אדר',
'hebrew-calendar-m6a'     => "אדר א'",
'hebrew-calendar-m6b'     => "אדר ב'",
'hebrew-calendar-m7'      => 'ניסן',
'hebrew-calendar-m8'      => 'אייר',
'hebrew-calendar-m9'      => 'סיוון',
'hebrew-calendar-m10'     => 'תמוז',
'hebrew-calendar-m11'     => 'אב',
'hebrew-calendar-m12'     => 'אלול',
'hebrew-calendar-m1-gen'  => 'בתשרי',
'hebrew-calendar-m2-gen'  => 'בחשוון',
'hebrew-calendar-m3-gen'  => 'בכסלו',
'hebrew-calendar-m4-gen'  => 'בטבת',
'hebrew-calendar-m5-gen'  => 'בשבט',
'hebrew-calendar-m6-gen'  => 'באדר',
'hebrew-calendar-m6a-gen' => "באדר א'",
'hebrew-calendar-m6b-gen' => "באדר ב'",
'hebrew-calendar-m7-gen'  => 'בניסן',
'hebrew-calendar-m8-gen'  => 'באייר',
'hebrew-calendar-m9-gen'  => 'בסיוון',
'hebrew-calendar-m10-gen' => 'בתמוז',
'hebrew-calendar-m11-gen' => 'באב',
'hebrew-calendar-m12-gen' => 'באלול',

# Core parser functions
'unknown_extension_tag' => 'תגית בלתי ידועה: "$1"',

# Special:Version
'version'                          => 'גרסת התוכנה', # Not used as normal message but as header for the special page itself
'version-extensions'               => 'הרחבות מותקנות',
'version-specialpages'             => 'דפים מיוחדים',
'version-parserhooks'              => 'הרחבות מפענח',
'version-variables'                => 'משתנים',
'version-other'                    => 'אחר',
'version-mediahandlers'            => 'מציגי מדיה',
'version-hooks'                    => 'מבני Hook',
'version-extension-functions'      => 'פונקציות של הרחבות',
'version-parser-extensiontags'     => 'תגיות של הרחבות מפענח',
'version-parser-function-hooks'    => 'משתנים',
'version-skin-extension-functions' => 'הרחבות רקעים',
'version-hook-name'                => 'שם ה־Hook',
'version-hook-subscribedby'        => 'הפונקציה הרושמת',
'version-version'                  => 'גרסה',
'version-license'                  => 'רישיון',
'version-software'                 => 'תוכנות מותקנות',
'version-software-product'         => 'תוכנה',
'version-software-version'         => 'גרסה',

# Special:FilePath
'filepath'         => 'נתיב לקובץ',
'filepath-page'    => 'הקובץ:',
'filepath-submit'  => 'מציאת הנתיב',
'filepath-summary' => 'דף זה מציג את הנתיב המלא לקבצים שהועלו. תמונות מוצגות ברזולוציה מלאה, ואילו סוגי קבצים אחרים מוצגים ישירות באמצעות התוכנה שהוגדרה להצגתם.

יש להקליד את שם הקובץ ללא הקידומת "{{ns:image}}:".',

# Special:FileDuplicateSearch
'fileduplicatesearch'          => 'חיפוש קבצים כפולים',
'fileduplicatesearch-summary'  => 'חיפוש קבצים כפולים על בסיס ערכי ה־Hash שלהם.

הקלידו את שם הקובץ ללא הקידומת "{{ns:image}}:".',
'fileduplicatesearch-legend'   => 'חיפוש קבצים כפולים',
'fileduplicatesearch-filename' => 'קובץ:',
'fileduplicatesearch-submit'   => 'חיפוש',
'fileduplicatesearch-info'     => '$1 × $2 פיקסלים<br />גודל הקובץ: $3<br />סוג MIME: $4',
'fileduplicatesearch-result-1' => 'אין קובץ כפול לקובץ "$1".',
'fileduplicatesearch-result-n' => 'לקובץ "$1" יש {{PLURAL:$2|עותק כפול אחד|$2 עותקים כפולים}}.',

# Special:SpecialPages
'specialpages'                   => 'דפים מיוחדים',
'specialpages-note'              => '----
* דפים מיוחדים רגילים.
* <span class="mw-specialpagerestricted">דפים מיוחדים מוגבלים.</span>',
'specialpages-group-maintenance' => 'דיווחי תחזוקה',
'specialpages-group-other'       => 'דפים מיוחדים אחרים',
'specialpages-group-login'       => 'כניסה / הרשמה לחשבון',
'specialpages-group-changes'     => 'שינויים אחרונים ויומנים',
'specialpages-group-media'       => 'קובצי מדיה והעלאות',
'specialpages-group-users'       => 'משתמשים והרשאות',
'specialpages-group-highuse'     => 'דפים בשימוש רב',
'specialpages-group-pages'       => 'רשימות דפים',
'specialpages-group-pagetools'   => 'כלים לדפים',
'specialpages-group-wiki'        => 'מידע וכלים על האתר',
'specialpages-group-redirects'   => 'הפניות מדפים מיוחדים',
'specialpages-group-spam'        => 'כלי ספאם',

# Special:BlankPage
'blankpage'              => 'דף ריק',
'intentionallyblankpage' => 'דף זה נשאר ריק במכוון',

);
