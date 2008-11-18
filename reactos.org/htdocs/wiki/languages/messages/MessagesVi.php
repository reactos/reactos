<?php
/** Vietnamese (Tiếng Việt)
 *
 * @ingroup Language
 * @file
 *
 * @author Apple
 * @author Arisa
 * @author DHN
 * @author Minh Nguyen
 * @author Mxn
 * @author Neoneurone
 * @author Nguyễn Thanh Quang
 * @author Thaisk
 * @author Tmct
 * @author Trần Thế Trung
 * @author Tttrung
 * @author Vietbio
 * @author Vinhtantran
 * @author Vương Ngân Hà
 * @author לערי ריינהארט
 */

$namespaceNames = array(
	NS_MEDIA			=> 'Phương_tiện',
	NS_SPECIAL			=> 'Đặc_biệt',
	NS_MAIN				=> '',
	NS_TALK				=> 'Thảo_luận',
	NS_USER				=> 'Thành_viên',
	NS_USER_TALK		=> 'Thảo_luận_Thành_viên',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK		=> 'Thảo_luận_$1',
	NS_IMAGE			=> 'Hình',
	NS_IMAGE_TALK		=> 'Thảo_luận_Hình',
	NS_MEDIAWIKI		=> 'MediaWiki',
	NS_MEDIAWIKI_TALK	=> 'Thảo_luận_MediaWiki',
	NS_TEMPLATE			=> 'Tiêu_bản',
	NS_TEMPLATE_TALK	=> 'Thảo_luận_Tiêu_bản',
	NS_HELP				=> 'Trợ_giúp',
	NS_HELP_TALK		=> 'Thảo_luận_Trợ_giúp',
	NS_CATEGORY			=> 'Thể_loại',
	NS_CATEGORY_TALK	=> 'Thảo_luận_Thể_loại'
);

$skinNames = array(
	'standard'		=> 'Cổ điển',
	'nostalgia'		=> 'Vọng cổ',
	'myskin'		=> 'Cá nhân'
);

$magicWords = array(
	'redirect'               => array( 0,    '#redirect' , '#đổi'             ),
	'notoc'                  => array( 0,    '__NOTOC__' , '__KHÔNGMỤCMỤC__'             ),
	'forcetoc'               => array( 0,    '__FORCETOC__', '__LUÔNMỤCLỤC__'        ),
	'toc'                    => array( 0,    '__TOC__' , '__MỤCLỤC__'               ),
	'noeditsection'          => array( 0,    '__NOEDITSECTION__', '__KHÔNGSỬAMỤC__'      ),
	'currentmonth'           => array( 1,    'CURRENTMONTH' , 'THÁNGNÀY'          ),
	'currentmonthname'       => array( 1,    'CURRENTMONTHNAME'  , 'TÊNTHÁNGNÀY'     ),
	'currentmonthnamegen'    => array( 1,    'CURRENTMONTHNAMEGEN' , 'TÊNDÀITHÁNGNÀY'   ),
	'currentmonthabbrev'     => array( 1,    'CURRENTMONTHABBREV'  , 'TÊNNGẮNTHÁNGNÀY'  ),
	'currentday'             => array( 1,    'CURRENTDAY'       , 'NGÀYNÀY'     ),
	'currentdayname'         => array( 1,    'CURRENTDAYNAME'   , 'TÊNNGÀYNÀY'      ),
	'currentyear'            => array( 1,    'CURRENTYEAR'    , 'NĂMNÀY'        ),
	'currenttime'            => array( 1,    'CURRENTTIME'     , 'GIỜNÀY'       ),
	'numberofarticles'       => array( 1,    'NUMBEROFARTICLES'  , 'SỐBÀI'     ),
	'numberoffiles'          => array( 1,    'NUMBEROFFILES'   , 'SỐTẬPTIN'       ),
	'pagename'               => array( 1,    'PAGENAME'      , 'TÊNTRANG'        ),
	'pagenamee'              => array( 1,    'PAGENAMEE'   , 'TÊNTRANG2'           ),
	'namespace'              => array( 1,    'NAMESPACE'   , 'KHÔNGGIANTÊN'           ),
	'msg'                    => array( 0,    'MSG:'     , 'NHẮN:'              ),
	'subst'                  => array( 0,    'SUBST:'   ,  'THẾ:'            ),
	'msgnw'                  => array( 0,    'MSGNW:'    ,  'NHẮNMỚI:'             ),
	'img_thumbnail'          => array( 1,    'thumbnail', 'thumb' , 'nhỏ'    ),
	'img_right'              => array( 1,    'right' , 'phải'                 ),
	'img_left'               => array( 1,    'left'  , 'trái'                ),
	'img_none'               => array( 1,    'none'  , 'không'                 ),
	'img_center'             => array( 1,    'center', 'centre' , 'giữa'      ),
	'img_framed'             => array( 1,    'framed', 'enframed', 'frame' , 'khung'),
	'sitename'               => array( 1,    'SITENAME'  , 'TÊNMẠNG'             ),
	'server'                 => array( 0,    'SERVER'    , 'MÁYCHỦ'             ),
	'servername'             => array( 0,    'SERVERNAME' , 'TÊNMÁYCHỦ'            ),
	'grammar'                => array( 0,    'GRAMMAR:'   , 'NGỮPHÁP'            ),
	'notitleconvert'         => array( 0,    '__NOTITLECONVERT__', '__NOTC__', '__KHÔNGCHUYỂNTÊN__'),
	'nocontentconvert'       => array( 0,    '__NOCONTENTCONVERT__', '__NOCC__', '__KHÔNGCHUYỂNNỘIDUNG__'),
	'currentweek'            => array( 1,    'CURRENTWEEK' , 'TUẦNNÀY'           ),
	'revisionid'             => array( 1,    'REVISIONID'  , 'SỐBẢN'           ),
 );

$datePreferences = array(
	'default',
	'vi normal',
	'vi spelloutmonth',
	'vi shortcolon',
	'vi shorth',
	'ISO 8601',
);

$defaultDateFormat = 'vi normal';

$dateFormats = array(
	'vi normal time' => 'H:i',
	'vi normal date' => '"ngày" j "tháng" n "năm" Y',
	'vi normal both' => 'H:i, "ngày" j "tháng" n "năm" Y',

	'vi spelloutmonth time' => 'H:i',
	'vi spelloutmonth date' => '"ngày" j xg "năm" Y',
	'vi spelloutmonth both' => 'H:i, "ngày" j xg "năm" Y',

	'vi shortcolon time' => 'H:i',
	'vi shortcolon date' => 'j/n/Y',
	'vi shortcolon both' => 'H:i, j/n/Y',

	'vi shorth time' => 'H"h"i',
	'vi shorth date' => 'j/n/Y',
	'vi shorth both' => 'H"h"i, j/n/Y',
);

$datePreferenceMigrationMap = array(
	'default',
	'vi normal',
	'vi normal',
	'vi normal',
);


$linkTrail = "/^([a-zàâçéèêîôûäëïöüùÇÉÂÊÎÔÛÄËÏÖÜÀÈÙ]+)(.*)$/sDu";
$separatorTransformTable = array(',' => '.', '.' => ',' );

$messages = array(
# User preference toggles
'tog-underline'               => 'Gạch chân liên kết:',
'tog-highlightbroken'         => 'Liên kết đến trang chưa có sẽ <a href="" class="new">giống thế này</a> (nếu không chọn: giống thế này<a href="" class="internal">?</a>)',
'tog-justify'                 => 'Căn đều hai bên đoạn văn',
'tog-hideminor'               => 'Ẩn sửa đổi nhỏ trong thay đổi gần đây',
'tog-extendwatchlist'         => 'Danh sách theo dõi nhiều chức năng (JavaScript)',
'tog-usenewrc'                => 'Thay đổi gần đây nhiều chức năng (JavaScript)',
'tog-numberheadings'          => 'Tự động đánh số các đề mục',
'tog-showtoolbar'             => 'Hiển thị thanh định dạng (JavaScript)',
'tog-editondblclick'          => 'Nhấn đúp để sửa đổi trang (JavaScript)',
'tog-editsection'             => 'Cho phép sửa đổi đề mục qua liên kết [sửa]',
'tog-editsectiononrightclick' => 'Cho phép sửa đổi mục bằng cách bấm chuột phải trên đề mục (JavaScript)',
'tog-showtoc'                 => 'Hiển thị mục lục (cho trang có trên 3 đề mục)',
'tog-rememberpassword'        => 'Nhớ thông tin đăng nhập của tôi trên máy tính này',
'tog-editwidth'               => 'Ô sửa đổi có bề rộng tối đa',
'tog-watchcreations'          => 'Tự động theo dõi trang tôi viết mới',
'tog-watchdefault'            => 'Tự động theo dõi trang tôi sửa',
'tog-watchmoves'              => 'Tự động theo dõi trang tôi di chuyển',
'tog-watchdeletion'           => 'Tự động theo dõi trang tôi xóa',
'tog-minordefault'            => 'Đánh dấu mặc định sửa đổi của tôi là thay đổi nhỏ',
'tog-previewontop'            => 'Hiển thị phần xem thử nằm trên hộp sửa đổi',
'tog-previewonfirst'          => 'Hiện xem thử tại lần sửa đầu tiên',
'tog-nocache'                 => 'Không lưu trang trong bộ nhớ đệm',
'tog-enotifwatchlistpages'    => 'Gửi thư cho tôi khi có thay đổi tại trang tôi theo dõi',
'tog-enotifusertalkpages'     => 'Gửi thư cho tôi khi có thay đổi tại trang thảo luận của tôi',
'tog-enotifminoredits'        => 'Gửi thư cho tôi cả những thay đổi nhỏ trong trang',
'tog-enotifrevealaddr'        => 'Hiện địa chỉ thư điện tử của tôi trong thư thông báo',
'tog-shownumberswatching'     => 'Hiển thị số người đang xem',
'tog-fancysig'                => 'Chữ ký không dùng liên kết tự động',
'tog-externaleditor'          => 'Mặc định dùng trình soạn thảo bên ngoài (chỉ dành cho người thành thạo, cần thiết lập đặc biệt trên máy tính của bạn)',
'tog-externaldiff'            => 'Mặc định dùng trình so sánh bên ngoài (chỉ dành cho người thành thạo, cần thiết lập đặc biệt trên máy tính của bạn)',
'tog-showjumplinks'           => 'Bật liên kết “bước tới” trên đầu trang cho bộ trình duyệt thuần văn bản hay âm thanh',
'tog-uselivepreview'          => 'Sử dụng xem thử trực tiếp (JavaScript) (thử nghiệm)',
'tog-forceeditsummary'        => 'Nhắc tôi khi tôi quên tóm lược sửa đổi',
'tog-watchlisthideown'        => 'Ẩn các sửa đổi của tôi khỏi danh sách theo dõi',
'tog-watchlisthidebots'       => 'Ẩn các sửa đổi của robot khỏi danh sách theo dõi',
'tog-watchlisthideminor'      => 'Ẩn các sửa đổi nhỏ khỏi danh sách theo dõi',
'tog-nolangconversion'        => 'Tắt chuyển đổi biến thể',
'tog-ccmeonemails'            => 'Gửi bản sao cho tôi khi gửi thư điện tử cho người khác',
'tog-diffonly'                => 'Không hiển thị nội dung trang dưới phần so sánh phiên bản',
'tog-showhiddencats'          => 'Hiển thị thể loại ẩn',

'underline-always'  => 'Luôn luôn',
'underline-never'   => 'Không bao giờ',
'underline-default' => 'Mặc định của trình duyệt',

'skinpreview' => '(Xem thử)',

# Dates
'sunday'        => 'Chủ nhật',
'monday'        => 'thứ Hai',
'tuesday'       => 'thứ Ba',
'wednesday'     => 'thứ Tư',
'thursday'      => 'thứ Năm',
'friday'        => 'thứ Sáu',
'saturday'      => 'thứ Bảy',
'sun'           => 'Chủ nhật',
'mon'           => 'thứ 2',
'tue'           => 'thứ 3',
'wed'           => 'thứ 4',
'thu'           => 'thứ 5',
'fri'           => 'thứ 6',
'sat'           => 'thứ 7',
'january'       => 'tháng 1',
'february'      => 'tháng 2',
'march'         => 'tháng 3',
'april'         => 'tháng 4',
'may_long'      => 'tháng 5',
'june'          => 'tháng 6',
'july'          => 'tháng 7',
'august'        => 'tháng 8',
'september'     => 'tháng 9',
'october'       => 'tháng 10',
'november'      => 'tháng 11',
'december'      => 'tháng 12',
'january-gen'   => 'tháng Một',
'february-gen'  => 'tháng Hai',
'march-gen'     => 'tháng Ba',
'april-gen'     => 'tháng Tư',
'may-gen'       => 'tháng Năm',
'june-gen'      => 'tháng Sáu',
'july-gen'      => 'tháng Bảy',
'august-gen'    => 'tháng Tám',
'september-gen' => 'tháng Chín',
'october-gen'   => 'tháng Mười',
'november-gen'  => 'tháng Mười một',
'december-gen'  => 'tháng Mười hai',
'jan'           => 'tháng 1',
'feb'           => 'tháng 2',
'mar'           => 'tháng 3',
'apr'           => 'tháng 4',
'may'           => 'tháng 5',
'jun'           => 'tháng 6',
'jul'           => 'tháng 7',
'aug'           => 'tháng 8',
'sep'           => 'tháng 9',
'oct'           => 'tháng 10',
'nov'           => 'tháng 11',
'dec'           => 'tháng 12',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|Thể loại|Thể loại}}',
'category_header'                => 'Các trang trong thể loại “$1”',
'subcategories'                  => 'Tiểu thể loại',
'category-media-header'          => 'Các tập tin trong thể loại “$1”',
'category-empty'                 => "''Thể loại này hiện không có trang hay tập tin nào.''",
'hidden-categories'              => '{{PLURAL:$1|Thể loại ẩn|Thể loại ẩn}}',
'hidden-category-category'       => 'Thể loại ẩn', # Name of the category where hidden categories will be listed
'category-subcat-count'          => 'Thể loại này có {{PLURAL:$2|tiểu thể loại sau|{{PLURAL:$1||$1}} tiểu thể loại sau, trên tổng số $2 tiểu thể loại}}.',
'category-subcat-count-limited'  => 'Thể loại này có {{PLURAL:$1||$1}} tiểu thể loại sau.',
'category-article-count'         => '{{PLURAL:$2|Thể loại này gồm trang sau.|{{PLURAL:$1|Trang|$1 trang}} sau nằm trong thể loại này, trên tổng số $2 trang.}}',
'category-article-count-limited' => '{{PLURAL:$1|Trang|$1 trang}} sau nằm trong thể loại hiện hành.',
'category-file-count'            => '{{PLURAL:$2|Thể loại này có tập tin sau.|{{PLURAL:$1|Tập tin|$1 tập tin}} sau nằm trong thể loại này, trong tổng số $2 tập tin.}}',
'category-file-count-limited'    => '{{PLURAL:$1|Tập tin|$1 tập tin}} sau nằm trong thể loại hiện hành.',
'listingcontinuesabbrev'         => 'tiếp',

'mainpagetext'      => "<big>'''MediaWiki đã được cài đặt thành công.'''</big>",
'mainpagedocfooter' => 'Xin đọc [http://meta.wikimedia.org/wiki/Help:Contents Hướng dẫn sử dụng] để biết thêm thông tin về cách sử dụng phần mềm wiki.

== Để bắt đầu ==

* [http://www.mediawiki.org/wiki/Manual:Configuration_settings Danh sách các thiết lập cấu hình]
* [http://www.mediawiki.org/wiki/Manual:FAQ Các câu hỏi thường gặp MediaWiki]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce Danh sách gửi thư về việc phát hành MediaWiki]',

'about'          => 'Giới thiệu',
'article'        => 'Trang nội dung',
'newwindow'      => '(mở cửa sổ mới)',
'cancel'         => 'Bãi bỏ',
'qbfind'         => 'Tìm kiếm',
'qbbrowse'       => 'Xem qua',
'qbedit'         => 'Sửa',
'qbpageoptions'  => 'Trang này',
'qbpageinfo'     => 'Ngữ cảnh',
'qbmyoptions'    => 'Trang của tôi',
'qbspecialpages' => 'Trang đặc biệt',
'moredotdotdot'  => 'Thêm nữa...',
'mypage'         => 'Trang của tôi',
'mytalk'         => 'Thảo luận với tôi',
'anontalk'       => 'Thảo luận với IP này',
'navigation'     => 'Chuyển hướng',
'and'            => 'và',

# Metadata in edit box
'metadata_help' => 'Đặc tính hình:',

'errorpagetitle'    => 'Lỗi',
'returnto'          => 'Quay lại $1.',
'tagline'           => 'Từ {{SITENAME}}',
'help'              => 'Trợ giúp',
'search'            => 'Tìm kiếm',
'searchbutton'      => 'Tìm kiếm',
'go'                => 'Xem',
'searcharticle'     => 'Xem',
'history'           => 'Lịch sử trang',
'history_short'     => 'Lịch sử',
'updatedmarker'     => 'được cập nhật kể từ lần xem cuối',
'info_short'        => 'Thông tin',
'printableversion'  => 'Bản để in',
'permalink'         => 'Liên kết thường trực',
'print'             => 'In',
'edit'              => 'Sửa đổi',
'create'            => 'Tạo',
'editthispage'      => 'Sửa trang này',
'create-this-page'  => 'Tạo trang này',
'delete'            => 'Xóa',
'deletethispage'    => 'Xóa trang này',
'undelete_short'    => 'Phục hồi {{PLURAL:$1|một sửa đổi|$1 sửa đổi}}',
'protect'           => 'Khóa',
'protect_change'    => 'thay đổi',
'protectthispage'   => 'Khóa trang này',
'unprotect'         => 'Mở khóa',
'unprotectthispage' => 'Mở khóa trang này',
'newpage'           => 'Trang mới',
'talkpage'          => 'Thảo luận về trang này',
'talkpagelinktext'  => 'Thảo luận',
'specialpage'       => 'Trang đặc biệt',
'personaltools'     => 'Công cụ cá nhân',
'postcomment'       => 'Thêm bàn luận',
'articlepage'       => 'Xem trang nội dung',
'talk'              => 'Thảo luận',
'views'             => 'Xem',
'toolbox'           => 'Thanh công cụ',
'userpage'          => 'Trang thành viên',
'projectpage'       => 'Trang Wikipedia',
'imagepage'         => 'Trang hình',
'mediawikipage'     => 'Thông báo giao diện',
'templatepage'      => 'Trang tiêu bản',
'viewhelppage'      => 'Trang trợ giúp',
'categorypage'      => 'Trang thể loại',
'viewtalkpage'      => 'Trang thảo luận',
'otherlanguages'    => 'Ngôn ngữ khác',
'redirectedfrom'    => '(đổi hướng từ $1)',
'redirectpagesub'   => 'Trang đổi hướng',
'lastmodifiedat'    => 'Lần sửa cuối : $2, $1.', # $1 date, $2 time
'viewcount'         => 'Trang này đã được đọc {{PLURAL:$1|một|$1}} lần.',
'protectedpage'     => 'Trang bị khóa',
'jumpto'            => 'Bước tới:',
'jumptonavigation'  => 'chuyển hướng',
'jumptosearch'      => 'tìm kiếm',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'Giới thiệu {{SITENAME}}',
'aboutpage'            => 'Project:Giới thiệu',
'bugreports'           => 'Báo lỗi',
'bugreportspage'       => 'Project:Báo lỗi',
'copyright'            => 'Bản quyền $1.',
'copyrightpagename'    => 'giấy phép {{SITENAME}}',
'copyrightpage'        => '{{ns:project}}:Bản quyền',
'currentevents'        => 'Thời sự',
'currentevents-url'    => 'Project:Thời sự',
'disclaimers'          => 'Phủ nhận',
'disclaimerpage'       => 'Project:Phủ nhận chung',
'edithelp'             => 'Trợ giúp sửa đổi',
'edithelppage'         => 'Help:Sửa đổi',
'faq'                  => 'Câu hỏi thường gặp',
'faqpage'              => 'Project:Các câu hỏi thường gặp',
'helppage'             => 'Help:Nội dung',
'mainpage'             => 'Trang Chính',
'mainpage-description' => 'Trang Chính',
'policy-url'           => 'Project:Quy định và hướng dẫn',
'portal'               => 'Cộng đồng',
'portal-url'           => 'Project:Cộng đồng',
'privacy'              => 'Chính sách về sự riêng tư',
'privacypage'          => 'Project:Chính sách về sự riêng tư',

'badaccess'        => 'Lỗi về quyền truy cập',
'badaccess-group0' => 'Bạn không được phép thực hiện thao tác này.',
'badaccess-group1' => 'Chỉ những thành viên trong nhóm $1 mới được làm thao tác này.',
'badaccess-group2' => 'Chỉ những thành viên trong các nhóm $1 mới được làm thao tác này.',
'badaccess-groups' => 'Chỉ những thành viên trong các nhóm $1 mới được làm thao tác này.',

'versionrequired'     => 'Cần phiên bản $1 của MediaWiki',
'versionrequiredtext' => 'Cần phiên bản $1 của MediaWiki để sử dụng trang này. Xem [[Special:Version|trang phiên bản]].',

'ok'                      => 'OK',
'retrievedfrom'           => 'Lấy từ “$1”',
'youhavenewmessages'      => 'Bạn có $1 ($2).',
'newmessageslink'         => 'tin nhắn mới',
'newmessagesdifflink'     => 'thay đổi gần nhất',
'youhavenewmessagesmulti' => 'Bạn có tin nhắn mới ở $1',
'editsection'             => 'sửa',
'editold'                 => 'sửa',
'viewsourceold'           => 'xem mã nguồn',
'editsectionhint'         => 'Sửa đổi đề mục: $1',
'toc'                     => 'Mục lục',
'showtoc'                 => 'hiện',
'hidetoc'                 => 'ẩn',
'thisisdeleted'           => 'Xem hay phục hồi $1 ?',
'viewdeleted'             => 'Xem $1?',
'restorelink'             => '{{PLURAL:$1|một|$1}} sửa đổi đã xóa',
'feedlinks'               => 'Nạp:',
'feed-invalid'            => 'Định dạng feed không hợp lệ.',
'feed-unavailable'        => 'Không có feed tại {{SITENAME}}',
'site-rss-feed'           => '$1 mục RSS',
'site-atom-feed'          => '$1 mục Atom',
'page-rss-feed'           => 'Mục RSS của “$1”',
'page-atom-feed'          => 'Mục Atom của “$1”',
'red-link-title'          => '$1 (chưa được viết)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Nội dung',
'nstab-user'      => 'Trang thành viên',
'nstab-media'     => 'Phương tiện',
'nstab-special'   => 'Đặc biệt',
'nstab-project'   => 'Dự án',
'nstab-image'     => 'Tập tin',
'nstab-mediawiki' => 'Thông báo',
'nstab-template'  => 'Tiêu bản',
'nstab-help'      => 'Trợ giúp',
'nstab-category'  => 'Thể loại',

# Main script and global functions
'nosuchaction'      => 'Không có tác vụ này',
'nosuchactiontext'  => 'Wiki không hiểu được tác vụ được yêu cầu trong địa chỉ URL',
'nosuchspecialpage' => 'Không có trang đặc biệt nào như vậy',
'nospecialpagetext' => 'Không có trang đặc biệt này.',

# General errors
'error'                => 'Lỗi',
'databaseerror'        => 'Lỗi cơ sở dữ liệu',
'dberrortext'          => 'Đã xảy ra lỗi cú pháp trong truy vấn cơ sở dữ liệu. Có vẻ như nguyên nhân của vấn đề này xuất phát từ một lỗi trong phần mềm. Truy vấn vừa rồi là:
<blockquote><tt>$1</tt></blockquote>
từ hàm “<tt>$2</tt>”. MySQL báo lỗi “<tt>$3: $4</tt>”.',
'dberrortextcl'        => 'Đã xảy ra lỗi cú pháp trong truy vấn cơ sở dữ liệu. Truy vấn vừa rồi là:
<blockquote><tt>$1</tt></blockquote>
từ hàm “<tt>$2</tt>”. MySQL báo lỗi “<tt>$3: $4</tt>”.',
'noconnect'            => 'Wiki đang gặp một số trục trặc kỹ thuật, và không thể kết nối với cơ sở dữ liệu. <br />
$1',
'nodb'                 => 'Không thấy cơ sở dữ liệu $1',
'cachederror'          => 'Đây là bản sao trong bộ nhớ đệm của trang bạn yêu cầu, nó có thể đã lỗi thời.',
'laggedslavemode'      => 'Cảnh báo: Trang có thể chưa được cập nhật.',
'readonly'             => 'Cơ sở dữ liệu bị khóa',
'enterlockreason'      => 'Nêu lý do khóa, cùng với thời hạn khóa',
'readonlytext'         => 'Cơ sở dữ liệu hiện đã bị khóa không nhận trang mới và các điều chỉnh khác, có lẽ để bảo trì cơ sở dữ liệu định kỳ, một thời gian ngắn nữa nó sẽ trở lại bình thường.

Người quản lý khóa nó đã đưa ra lời giải thích sau: $1',
'missing-article'      => 'Cơ sở dữ liệu không tìm thấy văn bản của trang lẽ ra phải có, trang      Normal   0               false   false   false      EN-US   X-NONE   X-NONE                                                     MicrosoftInternetExplorer4                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     “$1” $2.

Điều này thường xảy ra do nhấn vào liên kết khác biệt phiên bản đã quá lâu hoặc liên kết lịch sử của một trang đã bị xóa.

Nếu không phải lý do trên, có thể bạn đã gặp phải một lỗi của phần mềm.
Xin hãy báo nó cho một [[Special:ListUsers/sysop|bảo quản viên]], trong đó ghi lại địa chỉ URL.',
'missingarticle-rev'   => '(số phiên bản: $1)',
'missingarticle-diff'  => '(Khác: $1, $2)',
'readonly_lag'         => 'Cơ sở dữ liệu bị khóa tự động trong khi các máy chủ cập nhật thông tin của nhau.',
'internalerror'        => 'Lỗi nội bộ',
'internalerror_info'   => 'Lỗi nội bộ: $1',
'filecopyerror'        => 'Không thể chép tập tin “$1” đến “$2”.',
'filerenameerror'      => 'Không thể đổi tên tập tin “$1” thành “$2”.',
'filedeleteerror'      => 'Không thể xóa tập tin “$1”.',
'directorycreateerror' => 'Không thể tạo được danh mục “$1”.',
'filenotfound'         => 'Không tìm thấy tập tin “$1”.',
'fileexistserror'      => 'Không thể ghi ra tập tin “$1”: tập tin đã tồn tại',
'unexpected'           => 'Không hiểu giá trị: “$1”=“$2”.',
'formerror'            => 'Lỗi: không gửi mẫu đi được.',
'badarticleerror'      => 'Không thể thực hiện được tác vụ như thế tại trang này.',
'cannotdelete'         => 'Không thể xóa trang hay tập tin được chỉ định. (Có thể nó đã bị ai đó xóa rồi).',
'badtitle'             => 'Tựa trang sai',
'badtitletext'         => 'Tựa trang yêu cầu không đúng, rỗng, hoặc là một liên kết ngôn ngữ hoặc liên kết wiki sai. Nó có thể chứa một hoặc nhiều ký tự mà tựa trang không thể sử dụng.',
'perfdisabled'         => 'Xin lỗi! Tính năng này đã bị tắt tạm thời do nó làm chậm cơ sở dữ liệu đến mức không ai có thể dùng được wiki.',
'perfcached'           => 'Dữ liệu sau được lấy từ bộ nhớ đệm và có thể đã lỗi thời.',
'perfcachedts'         => 'Dữ liệu dưới đây được đưa vào vùng nhớ đệm và được cập nhật lần cuối lúc $1.',
'querypage-no-updates' => 'Việc cập nhật trang này hiện đã bị tắt. Dữ liệu ở đây có thể bị lỗi thời.',
'wrong_wfQuery_params' => 'Tham số sai trong wfQuery()<br />
Hàm: $1<br />
Truy vấn: $2',
'viewsource'           => 'Xem mã nguồn',
'viewsourcefor'        => 'đối với $1',
'actionthrottled'      => 'Thao tác bị giới hạn',
'actionthrottledtext'  => 'Để nhằm tránh spam, bạn không thể thực hiện thao tác này quá nhiều lần trong một thời gian ngắn.  Xin hãy chờ vài phút trước khi thực hiện lại.',
'protectedpagetext'    => 'Trang này đã bị khóa không cho sửa đổi.',
'viewsourcetext'       => 'Bạn vẫn có thể xem và chép xuống mã nguồn của trang này:',
'protectedinterface'   => 'Trang này cung cấp một thông báo trong giao diện phần mềm, và bị khóa để tránh phá hoại.',
'editinginterface'     => "'''Lưu ý:''' Bạn đang sửa chữa một trang dùng để cung cấp thông báo giao diện cho phần mềm. Những thay đổi tại trang này sẽ ảnh hưởng đến giao diện của rất nhiều người dùng website này. Để dịch luật, hãy xem xét việc sử dụng [http://translatewiki.net/wiki/Main_Page?setlang=vi Betawiki], dự án địa phương hóa của MediaWiki.",
'sqlhidden'            => '(đã giấu truy vấn SQL)',
'cascadeprotected'     => 'Trang này đã bị khóa không cho sửa đổi, vì nó được nhúng vào {{PLURAL:$1|trang|những trang}} đã bị khóa với tùy chọn “khóa theo tầng” được kích hoạt:
$2',
'namespaceprotected'   => "Bạn không được cấp quyền sửa các trang trong không gian '''$1'''.",
'customcssjsprotected' => 'Bạn không có quyền sửa đổi trang này vì nó chứa các tùy chọn cá nhân của một thành viên khác.',
'ns-specialprotected'  => 'Không thể sửa chữa các trang trong không gian tên {{ns:special}}.',
'titleprotected'       => "Tựa đề này đã bị [[User:$1|$1]] khóa không cho tạo ra.
Lý do được cung cấp là ''$2''.",

# Virus scanner
'virus-badscanner'     => 'Cấu hình sau: không nhận ra bộ quét virus: <i>$1</i>',
'virus-scanfailed'     => 'quét thất bại (mã $1)',
'virus-unknownscanner' => 'không nhận ra phần mềm diệt virus:',

# Login and logout pages
'logouttitle'                => 'Đăng xuất',
'logouttext'                 => "<strong>Bạn đã đăng xuất.</strong>

Bạn có thể tiếp tục dùng {{SITENAME}} một cách vô danh, hoặc bạn có thể [[Special:UserLogin|đăng nhập lại]] dưới cùng tên người dùng này hoặc một tên người dùng khác. Xin lưu ý rằng một vài trang có thể vẫn hiển thị như khi bạn còn đăng nhập, cho đến khi bạn xóa vùng nhớ đệm (''cache'') của trình duyệt.",
'welcomecreation'            => '== Chào mừng, $1! ==
Tài khoản của bạn đã mở.
Đừng quên thay đổi [[Special:Preferences|tùy chọn cá nhân của bạn tại {{SITENAME}}]].',
'loginpagetitle'             => 'Đăng nhập',
'yourname'                   => 'Tên người dùng:',
'yourpassword'               => 'Mật khẩu',
'yourpasswordagain'          => 'Gõ lại mật khẩu',
'remembermypassword'         => 'Nhớ thông tin đăng nhập của tôi trên máy tính này',
'yourdomainname'             => 'Tên miền của bạn:',
'externaldberror'            => 'Có lỗi khi xác nhận cơ sở dữ liệu bên ngoài hoặc bạn không được phép cập nhật tài khoản bên ngoài.',
'loginproblem'               => '<b>Có trục trặc khi đăng nhập.</b><br />Mời thử lại!',
'login'                      => 'Đăng nhập',
'nav-login-createaccount'    => 'Đăng nhập / Mở tài khoản',
'loginprompt'                => 'Bạn cần bật cookie để đăng nhập vào {{SITENAME}}.',
'userlogin'                  => 'Đăng nhập / Mở tài khoản',
'logout'                     => 'Đăng xuất',
'userlogout'                 => 'Đăng xuất',
'notloggedin'                => 'Chưa đăng nhập',
'nologin'                    => 'Bạn chưa có tài khoản ở đây? $1.',
'nologinlink'                => 'Mở một tài khoản',
'createaccount'              => 'Mở tài khoản',
'gotaccount'                 => 'Đã mở tài khoản rồi? $1.',
'gotaccountlink'             => 'Đăng nhập',
'createaccountmail'          => 'qua thư điện tử',
'badretype'                  => 'Hai mật khẩu không khớp.',
'userexists'                 => 'Tên người dùng này đã có người lấy.
Hãy chọn một tên khác.',
'youremail'                  => 'Thư điện tử:',
'username'                   => 'Tên người dùng:',
'uid'                        => 'Số thứ tự thành viên:',
'prefs-memberingroups'       => 'Thành viên của {{PLURAL:$1|nhóm|nhóm}}:',
'yourrealname'               => 'Tên thật:',
'yourlanguage'               => 'Ngôn ngữ:',
'yourvariant'                => 'Ngôn ngữ địa phương:',
'yournick'                   => 'Chữ ký:',
'badsig'                     => 'Chữ ký không hợp lệ; hãy kiểm tra thẻ HTML.',
'badsiglength'               => 'Chữ ký của bạn quá dài.
Nó phải không quá $1 {{PLURAL:$1|ký tự|ký tự}}.',
'email'                      => 'Thư điện tử',
'prefs-help-realname'        => 'Tên thật là không bắt buộc, nhưng nếu bạn ghi lại, tên này sẽ dùng để ghi công cho bạn.',
'loginerror'                 => 'Lỗi đăng nhập',
'prefs-help-email'           => 'Địa chỉ thư điện tử là tùy chọn, nhưng nó giúp bạn nhận lại mật khẩu qua thư điện tử nếu bạn quên.
Bạn cũng có thể lựa chọn để cho phép người khác liên lạc với bạn thông qua trang thành_viên hoặc thảo_luận_thành_viên mà không cần để lộ danh tính.',
'prefs-help-email-required'  => 'Bắt buộc phải có địa chỉ e-mail.',
'nocookiesnew'               => 'Tài khoản đã mở, nhưng bạn chưa đăng nhập. {{SITENAME}} sử dụng cookie để đăng nhập vào tài khoản. Bạn đã tắt cookie. Xin hãy kích hoạt nó, rồi đăng nhập lại với tên người dùng và mật khẩu mới.',
'nocookieslogin'             => '{{SITENAME}} sử dụng cookie để đăng nhập thành viên. Bạn đã tắt cookie. Xin hãy kích hoạt rồi thử lại.',
'noname'                     => 'Chưa nhập tên.',
'loginsuccesstitle'          => 'Đăng nhập thành công',
'loginsuccess'               => "'''Bạn đã đăng nhập vào {{SITENAME}} với tên “$1”.'''",
'nosuchuser'                 => 'Thành viên có thành viên nào có tên “$1”.
Hãy kiểm tra lại chính tả, hoặc [[Special:Userlogin/signup|mở tài khoản mới]].',
'nosuchusershort'            => 'Không có thành viên nào có tên “<nowiki>$1</nowiki>”. Xin hãy kiểm tra lại chính tả.',
'nouserspecified'            => 'Bạn phải đưa ra tên đăng ký.',
'wrongpassword'              => 'Mật khẩu sai. Xin vui lòng nhập lại.',
'wrongpasswordempty'         => 'Bạn chưa gõ vào mật khẩu. Xin thử lần nữa.',
'passwordtooshort'           => 'Mật khẩu của bạn không hợp lệ hoặc quá ngắn.
Nó phải có ít nhất {{PLURAL:$1|1 ký tự|$1 ký tự}} và phải khác với tên người dùng của bạn.',
'mailmypassword'             => 'Gửi mật khẩu mới qua thư điện tử',
'passwordremindertitle'      => 'Mật khẩu tạm thời cho {{SITENAME}}',
'passwordremindertext'       => 'Người nào đó (có thể là bạn, có địa chỉ IP $1) đã yêu cầu chúng tôi gửi cho bạn mật khẩu mới của {{SITENAME}} ($4). Mật khẩu tạm cho thành viên “$2” đã được khởi tạo là “$3”. Nếu đây đúng là thứ bạn muốn, bạn sẽ cần phải đăng nhập và thay đổi mật khẩu ngay bây giờ.

Nếu một người nào khác yêu cầu điều này, hoặc nếu bạn đã nhớ ra mật khẩu, và không còn muốn đổi nó nữa, bạn có thể bỏ qua bức thư này và tiếp tục sử dụng mật khẩu cũ của bạn.',
'noemail'                    => 'Thành viên “$1” không đăng ký thư điện tử.',
'passwordsent'               => 'Mật khẩu mới đã được gửi tới thư điện tử của thành viên “$1”. Xin đăng nhập lại sau khi nhận thư.',
'blocked-mailpassword'       => 'Địa chỉ IP của bạn bị cấm không được sửa đổi, do đó cũng không được phép dùng chức năng phục hồi mật khẩu để tránh lạm dụng.',
'eauthentsent'               => 'Thư xác nhận đã được gửi. Trước khi dùng chức năng nhận thư, bạn cần thực hiện hướng dẫn trong thư xác nhận, để đảm bảo tài khoản thuộc về bạn.',
'throttled-mailpassword'     => 'Mật khẩu đã được gửi đến cho bạn trong vòng {{PLURAL:$1|$1 giờ|$1 giờ}} đồng hồ trở lại. Để tránh lạm dụng, chỉ có thể gửi mật khẩu $1 giờ đồng hồ một lần.',
'mailerror'                  => 'Lỗi gửi thư : $1',
'acct_creation_throttle_hit' => 'Bạn đã mở $1 tài khoản. Không thể mở thêm được nữa.',
'emailauthenticated'         => 'Địa chỉ thư điện tử của bạn được xác nhận tại $1.',
'emailnotauthenticated'      => 'Địa chỉ thư điện tử của bạn chưa được xác nhận. Chức năng thư điện tử chưa bật.',
'noemailprefs'               => 'Không có địa chỉ thư điện tử, chức năng sau có thể không hoạt động.',
'emailconfirmlink'           => 'Xác nhận địa chỉ thư điện tử',
'invalidemailaddress'        => 'Địa chỉ thư điện tử không được chấp nhận vì định dạng thư có vẻ sai.
Hãy nhập một địa chỉ có định dạng đúng hoặc bỏ trống ô đó.',
'accountcreated'             => 'Mở tài khoản thành công',
'accountcreatedtext'         => 'Tài khoản thành viên cho $1 đã được mở.',
'createaccount-title'        => 'Tài khoản mới tại {{SITENAME}}',
'createaccount-text'         => 'Ai đó đã tạo một tài khoản với tên $2 tại {{SITENAME}} ($4). Mật khẩu của "$2" là "$3". Bạn nên đăng nhập và đổi mật khẩu ngay bây giờ.

Xin hãy bỏ qua thông báo này nếu tài khoản này không phải do bạn tạo ra.',
'loginlanguagelabel'         => 'Ngôn ngữ: $1',

# Password reset dialog
'resetpass'               => 'Đặt lại mật khẩu',
'resetpass_announce'      => 'Bạn đã đăng nhập bằng mật khẩu tạm gởi qua e-mail. Để hoàn tất việc đăng nhập, bạn phải tạo lại mật khẩu mới tại đây:',
'resetpass_text'          => '<!-- Gõ chữ vào đây -->',
'resetpass_header'        => 'Đặt lại mật khẩu',
'resetpass_submit'        => 'Chọn mật khẩu và đăng nhập',
'resetpass_success'       => 'Đã đổi mật khẩu thành công! Đang đăng nhập…',
'resetpass_bad_temporary' => 'Mật khẩu tạm sai. Có thể là bạn đã đổi mật khẩu thành công hay đã xin mật khẩu tạm mới.',
'resetpass_forbidden'     => 'Không được đổi mật khẩu ở {{SITENAME}}',
'resetpass_missing'       => 'Biểu mẫu đang trống.',

# Edit page toolbar
'bold_sample'     => 'Chữ đậm',
'bold_tip'        => 'Chữ đậm',
'italic_sample'   => 'Chữ xiên',
'italic_tip'      => 'Chữ xiên',
'link_sample'     => 'Liên kết',
'link_tip'        => 'Liên kết',
'extlink_sample'  => 'http://www.example.com liên kết ngoài',
'extlink_tip'     => 'Liên kết ngoài (nhớ ghi http://)',
'headline_sample' => 'Đề mục',
'headline_tip'    => 'Đề mục cấp 2',
'math_sample'     => 'Nhập công thức toán vào đây',
'math_tip'        => 'Công thức toán (LaTeX)',
'nowiki_sample'   => 'Nhập dòng chữ không theo định dạng wiki vào đây',
'nowiki_tip'      => 'Không theo định dạng wiki',
'image_sample'    => 'Ví dụ.jpg',
'image_tip'       => 'Chèn hình',
'media_sample'    => 'Ví dụ.ogg',
'media_tip'       => 'Liên kết phương tiện',
'sig_tip'         => 'Chữ ký có ngày',
'hr_tip'          => 'Dòng kẻ ngang (không nên lạm dụng)',

# Edit pages
'summary'                          => 'Tóm tắt',
'subject'                          => 'Đề mục',
'minoredit'                        => 'Sửa đổi nhỏ',
'watchthis'                        => 'Theo dõi trang này',
'savearticle'                      => 'Lưu trang',
'preview'                          => 'Xem thử',
'showpreview'                      => 'Xem thử',
'showlivepreview'                  => 'Xem thử nhanh',
'showdiff'                         => 'Xem thay đổi',
'anoneditwarning'                  => "'''Cảnh báo:''' Bạn chưa đăng nhập. Địa chỉ IP của bạn sẽ được ghi lại trong lịch sử sửa đổi của trang.",
'missingsummary'                   => "'''Nhắc nhở:''' Bạn đã không ghi lại tóm lược sửa đổi. Nếu bạn nhấn Lưu trang một lần nữa, sửa đổi của bạn sẽ được lưu mà không có tóm lược.",
'missingcommenttext'               => 'Xin hãy gõ vào lời bàn luận ở dưới.',
'missingcommentheader'             => "'''Nhắc nhở:''' Bạn chưa cung cấp đề mục cho bàn luận này. Nếu bạn nhấn nút Lưu trang lần nữa, sửa đổi của bạn sẽ được lưu mà không có đề mục.",
'summary-preview'                  => 'Xem trước dòng tóm lược',
'subject-preview'                  => 'Xem trước đề mục',
'blockedtitle'                     => 'Thành viên bị cấm',
'blockedtext'                      => "<big>'''Tên người dùng hoặc địa chỉ IP của bạn đã bị cấm.'''</big>

Người thực hiện cấm là $1.
Lý do được cung cấp là ''$2''.

* Bắt đầu cấm: $8
* Kết thúc cấm: $6
* Mục tiêu cấm: $7

Bạn có thể liên hệ với $1 hoặc một [[{{MediaWiki:Grouppage-sysop}}|bảo quản viên]] khác để thảo luận về việc cấm.
Bạn không thể sử dụng tính năng “gửi thư cho người này” trừ khi bạn đã đăng ký một địa chỉ thư điện tử hợp lệ trong [[Special:Preferences|tùy chọn tài khoản]] và bạn không bị khóa chức năng đó.
Địa chỉ IP hiện tại của bạn là $3, và mã số cấm là #$5.
Xin hãy ghi kèm tất cả các thông tin trên vào thư yêu cầu của bạn.",
'autoblockedtext'                  => "Địa chỉ IP của bạn đã bị tự động cấm vì một người nào đó đã sử dụng nó, $1 là thành viên đã thực hiện cấm.
Lý do được cung cấp là:

:''$2''

* Bắt đầu cấm: $8
* Kết thúc cấm: $6
* Mục tiêu cấm: $7

Bạn có thể liên hệ với $1 hoặc một trong số các
[[{{MediaWiki:Grouppage-sysop}}|bảo quản viên]] khác để thảo luận về việc cấm.

Chú ý rằng bạn sẽ không dùng được chức năng “gửi thư cho người này” trừ khi bạn đã đăng ký một địa chỉ thư điện tử hợp lệ trong [[Special:Preferences|tùy chọn]] và bạn không bị cấm dùng chức năng đó.

Địa chỉ IP hiện tại của bạn là $3, mã số cấm là $5.
Xin hãy ghi kèm tất cả các chi tiết trên vào thư yêu cầu của bạn.",
'blockednoreason'                  => 'không đưa ra lý do',
'blockedoriginalsource'            => "Mã nguồn của '''$1''':",
'blockededitsource'                => "Các '''sửa đổi của bạn''' ở '''$1''':",
'whitelistedittitle'               => 'Cần đăng nhập để sửa trang',
'whitelistedittext'                => 'Bạn phải $1 để sửa trang.',
'confirmedittitle'                 => 'Cần xác nhận địa chỉ thư điện tử trước khi sửa đổi',
'confirmedittext'                  => 'Bạn cần phải xác nhận địa chỉ thư điện tử trước khi được sửa đổi trang. Xin hãy đặt và xác nhận địa chỉ thư điện tử của bạn dùng trang [[Special:Preferences|tùy chọn]].',
'nosuchsectiontitle'               => 'Không có mục nào như vậy',
'nosuchsectiontext'                => 'Bạn vừa sửa đổi một mục chưa tồn tại.  Vì không có mục nào mang tên $1, không thể lưu sửa đổi của bạn vào đó.',
'loginreqtitle'                    => 'Cần đăng nhập',
'loginreqlink'                     => 'đăng nhập',
'loginreqpagetext'                 => 'Bạn phải $1 mới có quyền xem các trang khác.',
'accmailtitle'                     => 'Đã gửi mật khẩu.',
'accmailtext'                      => 'Mật khẩu của “$1” đã được gửi đến $2.',
'newarticle'                       => '(Mới)',
'newarticletext'                   => "Bạn đi đến đây từ một liên kết đến một trang chưa tồn tại. Để tạo trang, hãy bắt đầu gõ vào ô bên dưới (xem [[{{MediaWiki:Helppage}}|trang trợ giúp]] để có thêm thông tin). Nếu bạn đến đây do nhầm lẫn, chỉ cần nhấn vào nút '''Back''' trên trình duyệt của bạn.",
'anontalkpagetext'                 => "----''Đây là trang thảo luận của một thành viên vô danh chưa tạo tài khoản hoặc có tài khoản nhưng không đăng nhập.
Do đó chúng ta phải dùng một dãy số gọi là địa chỉ IP để xác định anh/chị ta.
Một địa chỉ IP như vậy có thể có nhiều người cùng dùng chung.
Nếu bạn là một thành viên vô danh và cảm thấy rằng có những lời bàn luận không thích hợp đang nhắm vào bạn, xin hãy [[Special:UserLogin/signup|tạo tài khoản]] hoặc [[Special:UserLogin|đăng nhập]] để tránh sự nhầm lẫn về sau với những thành viên vô danh khác.''",
'noarticletext'                    => 'Trang này hiện chưa có gì, bạn có thể [[Special:Search/{{PAGENAME}}|tìm kiếm tựa trang]] tại các trang khác hoặc [{{fullurl:{{FULLPAGENAME}}|action=edit}} sửa đổi trang này].',
'userpage-userdoesnotexist'        => 'Tài khoản mang tên “$1” chưa được đăng ký. Xin hãy kiểm tra lại nếu bạn muốn tạo/sửa trang này.',
'clearyourcache'                   => "'''Ghi chú - Sau khi lưu trang, có thể bạn sẽ phải xóa bộ nhớ đệm của trình duyệt để xem các thay đổi.''' '''Mozilla / Firefox / Safari:''' giữ phím ''Shift'' trong khi nhấn ''Reload'', hoặc nhấn tổ hợp ''Ctrl-F5'' hay ''Ctrl-R'' (''Command-R'' trên Macintosh); '''Konqueror:''': nhấn nút ''Reload'' hoặc nhấn ''F5''; '''Opera:''' xóa bộ nhớ đệm trong ''Tools → Preferences''; '''Internet Explorer:''' giữ phím ''Ctrl'' trong khi nhấn ''Refresh'', hoặc nhấn tổ hợp ''Ctrl-F5''.",
'usercssjsyoucanpreview'           => '<strong>Mẹo:</strong> Sử dụng nút “Xem thử” để kiểm thử trang CSS/JS của bạn trước khi lưu trang.',
'usercsspreview'                   => "'''Hãy nhớ rằng bạn chỉ đang xem thử trang CSS cá nhân của bạn.
Nó chưa được lưu!'''",
'userjspreview'                    => "'''Nhớ rằng bạn chỉ đang kiểm thử/xem thử trang JavaScript, nó chưa được lưu!'''",
'userinvalidcssjstitle'            => "'''Cảnh báo:''' Không có skin “$1”. Hãy nhớ rằng các trang .css và .js tùy chỉnh sử dụng tiêu đề chữ thường, như {{ns:user}}:Ví&nbsp;dụ/monobook.css chứ không phải {{ns:user}}:Ví&nbsp;dụ/Monobook.css.",
'updated'                          => '(Cập nhật)',
'note'                             => '<strong>Ghi chú:</strong>',
'previewnote'                      => '<strong>Đây chỉ mới là xem thử; các thay đổi vẫn chưa được lưu!</strong>',
'previewconflict'                  => 'Phần xem thử này là kết quả của văn bản trong vùng soạn thảo phía trên và nó sẽ xuất hiện như vậy nếu bạn chọn lưu trang.',
'session_fail_preview'             => '<strong>Những sửa đổi của bạn chưa được lưu giữ do mất dữ liệu về phiên làm việc.
Xin hãy thử lần nữa.
Nếu vẫn không thành công, hãy thử [[Special:UserLogout|đăng xuất]] rồi đăng nhập lại.</strong>',
'session_fail_preview_html'        => "<strong>Những sửa đổi của bạn chưa được lưu giữ do mất dữ liệu về phiên làm việc.</strong>

''Do {{SITENAME}} cho phép dùng mã HTML, trang xem thử được ẩn đi để đề phòng bị tấn công bằng JavaScript.''

<strong>Nếu sửa đổi này là đúng đắn, xin hãy thử lần nữa. 
Nếu vẫn không thành công, bạn hãy thử [[Special:UserLogout|đăng xuất]] rồi đăng nhập lại.</strong>",
'token_suffix_mismatch'            => '<strong>Sửa đổi của bạn bị hủy bỏ vì trình duyệt của bạn lẫn lộn các ký tự dấu trong số hiệu
sửa đổi. Việc hủy bỏ này nhằm tránh nội dung trang bị hỏng.
Điều này thường xảy ra khi bạn sử dụng một dịch vụ proxy vô danh trên web có vấn đề.</strong>',
'editing'                          => 'Sửa đổi $1',
'editingsection'                   => 'Sửa đổi $1',
'editingcomment'                   => 'Sửa đổi $1',
'editconflict'                     => 'Sửa đổi mâu thuẫn: $1',
'explainconflict'                  => "Trang này có đã được lưu bởi người khác sau khi bạn bắt đầu sửa.
Phía trên là bản hiện tại.
Phía dưới là sửa đổi của bạn.
Bạn sẽ phải trộn thay đổi của bạn với bản hiện tại.
'''Chỉ có''' phần văn bản ở phía trên là sẽ được lưu khi bạn nhất nút “Lưu trang”.",
'yourtext'                         => 'Nội dung bạn nhập',
'storedversion'                    => 'Phiên bản lưu',
'nonunicodebrowser'                => "<strong>CHU' Y': Tri`nh duye^.t cu?a ba.n kho^ng ho^~ tro+. unicode. Mo^.t ca'ch dde^? ba.n co' the^? su+?a ddo^?i an toa`n trang na`y: ca'c ky' tu+. kho^ng pha?i ASCII se~ xua^'t hie^.n trong ho^.p soa.n tha?o du+o+'i da.ng ma~ tha^.p lu.c pha^n.</strong>",
'editingold'                       => '<strong>Chú ý: bạn đang sửa một phiên bản cũ. Nếu bạn lưu, các sửa đổi trên các phiên bản mới hơn sẽ bị mất.</strong>',
'yourdiff'                         => 'Khác',
'copyrightwarning'                 => 'Xin chú ý rằng tất cả các đóng góp của bạn tại {{SITENAME}} được xem là sẽ phát hành theo giấy phép $2 (xem $1 để biết thêm chi tiết). Nếu bạn không muốn trang của bạn bị sửa đổi không thương tiếc và không sẵn lòng cho phép phát hành lại, đừng đăng trang ở đây.<br />
Bạn phải đảm bảo với chúng tôi rằng chính bạn là người viết nên, hoặc chép nó từ một nguồn thuộc phạm vi công cộng hoặc tự do tương đương.
<strong>ĐỪNG ĐĂNG TÁC PHẨM CÓ BẢN QUYỀN MÀ CHƯA XIN PHÉP!</strong>',
'copyrightwarning2'                => 'Xin chú ý rằng tất cả các đóng góp của bạn tại {{SITENAME}} có thể được sửa đổi, thay thế, hoặc xóa bỏ bởi các thành viên khác. Nếu bạn không muốn trang của bạn bị sửa đổi không thương tiếc, đừng đăng trang ở đây.<br />
Bạn phải đảm bảo với chúng tôi rằng chính bạn là người viết nên, hoặc chép nó từ một nguồn thuộc phạm vi công cộng hoặc tự do tương đương (xem $1 để biết thêm chi tiết).
<strong>ĐỪNG ĐĂNG TÁC PHẨM CÓ BẢN QUYỀN MÀ CHƯA XIN PHÉP!</strong>',
'longpagewarning'                  => '<strong>CẢNH BÁO: Trang này dài $1 kilobyte; một số trình duyệt không tải được trang dài hơn 32 kb. Bạn nên chia nhỏ trang này thành nhiều trang.</strong>',
'longpageerror'                    => '<strong>LỖI: Văn bạn mà bạn muốn lưu dài $1 kilobyte, dài hơn độ dài tối đa cho phép $2 kilobyte. Không thể lưu trang.</strong>',
'readonlywarning'                  => '<strong>CẢNH BÁO: Cơ sở dữ liệu đã bị khóa để bảo dưỡng, do đó bạn không thể lưu các sửa đổi của mình. Bạn nên cắt-dán đoạn bạn vừa sửa vào một tập tin và lưu nó lại để sửa đổi sau này.</strong>',
'protectedpagewarning'             => '<strong>CẢNH BÁO:  Trang này đã bị khoá, chỉ có các thành viên có quyền quản lý mới sửa được.</strong>',
'semiprotectedpagewarning'         => "'''Ghi chú:''' Trang này đã bị khóa, chỉ cho phép các thành viên đã đăng ký sửa đổi.",
'cascadeprotectedwarning'          => "'''Cảnh báo:''' Trang này đã bị khóa, chỉ có thành viên có quyền quản lý mới có thể sửa đổi được, vì nó được nhúng vào {{PLURAL:$1|trang|những trang}} bị khóa theo tầng sau:",
'titleprotectedwarning'            => '<strong>CẢNH BÁO:  Trang này đã bị khóa, chỉ có một số thành viên mới có thể tạo ra.</strong>',
'templatesused'                    => 'Các tiêu bản dùng trong trang này',
'templatesusedpreview'             => 'Các tiêu bản sẽ được dùng trong trang này:',
'templatesusedsection'             => 'Các tiêu bản sẽ được dùng trong phần này:',
'template-protected'               => '(khóa hoàn toàn)',
'template-semiprotected'           => '(bị hạn chế sửa đổi)',
'hiddencategories'                 => 'Trang này thuộc về {{PLURAL:$1|1 thể loại ẩn|$1 thể loại ẩn}}:',
'edittools'                        => '<!-- Văn bản dưới đây sẽ xuất hiện phía dưới mẫu sửa đổi và tải lên. -->',
'nocreatetitle'                    => 'Khả năng tạo trang bị hạn chế',
'nocreatetext'                     => '{{SITENAME}} đã hạn chế khả năng tạo trang mới.
Bạn có thể quay trở lại và sửa đổi các trang đã có, hoặc [[Special:UserLogin|đăng nhập hoặc tạo tài khoản]].',
'nocreate-loggedin'                => 'Bạn không có quyền tạo trang mới trên {{SITENAME}}.',
'permissionserrors'                => 'Không có quyền thực hiện',
'permissionserrorstext'            => 'Bạn không có quyền thực hiện thao tác đó, vì {{PLURAL:$1|lý do|lý do}}:',
'permissionserrorstext-withaction' => 'Bạn không quyền $2, với {{PLURAL:$1|lý do|lý do}} sau:',
'recreate-deleted-warn'            => "'''Cảnh báo: Bạn vừa tạo lại một trang từng bị xóa trước đây.'''

Bạn nên cân nhắc trong việc tiếp tục soạn thảo trang này.
Nhật trình xóa của trang được đưa ra dưới đây để tiện theo dõi:",

# Parser/template warnings
'expensive-parserfunction-warning'        => 'Cảnh báo: Trang này có quá nhiều lần gọi hàm cú pháp cần mức độ xử lý cao.

Nó nên ít hơn $2, hiện giờ đang là $1.',
'expensive-parserfunction-category'       => 'Trang có quá nhiều lời gọi hàm cú pháp cần mức độ xử lý cao',
'post-expand-template-inclusion-warning'  => 'Cảnh báo: Kích thước tiêu bản nhúng vào quá lớn.
Một số tiêu bản sẽ không được đưa vào.',
'post-expand-template-inclusion-category' => 'Những trang có kích thước tiêu bản nhúng vào vượt quá giới hạn cho phép',
'post-expand-template-argument-warning'   => 'Cảnh báo: Trang này có chứa ít nhất một giá trị tiêu bản có kích thước bung ra quá lớn.
Những giá trị này sẽ bị bỏ đi.',
'post-expand-template-argument-category'  => 'Những trang có chứa những giá trị tiêu bản bị loại bỏ',

# "Undo" feature
'undo-success' => 'Các sửa đổi có thể được lùi lại. Xin hãy kiểm tra phần so sánh bên dưới để xác nhận lại những gì bạn muốn làm, sau đó lưu thay đổi ở dưới để hoàn tất việc lùi lại sửa đổi.',
'undo-failure' => 'Sửa đổi không thể phục hồi vì đã có những sửa đổi mới ở sau.',
'undo-norev'   => 'Sửa đổi không thể hồi phục vì nó không tồn tại hoặc đã bị xóa.',
'undo-summary' => 'Đã lùi lại sửa đổi $1 của [[Special:Contributions/$2|$2]] ([[User talk:$2|Thảo luận]])',

# Account creation failure
'cantcreateaccounttitle' => 'Không thể mở tài khoản',
'cantcreateaccount-text' => "Chức năng tài tạo khoản từ địa chỉ IP này ('''$1''') đã bị [[User:$3|$3]] cấm.

Lý do được $3 đưa ra là ''$2''",

# History pages
'viewpagelogs'        => 'Xem nhật trình của trang này',
'nohistory'           => 'Trang này chưa có lịch sử.',
'revnotfound'         => 'Không thấy',
'revnotfoundtext'     => 'Không thấy phiên bản trước của trang này. Xin kiểm tra lại.',
'currentrev'          => 'Bản hiện tại',
'revisionasof'        => 'Phiên bản lúc $1',
'revision-info'       => 'Phiên bản vào lúc $1 do $2 sửa đổi',
'previousrevision'    => '← Phiên bản cũ',
'nextrevision'        => 'Phiên bản mới →',
'currentrevisionlink' => 'xem phiên bản hiện hành',
'cur'                 => 'hiện',
'next'                => 'tiếp',
'last'                => 'trước',
'page_first'          => 'đầu',
'page_last'           => 'cuối',
'histlegend'          => 'Chọn so sánh: đánh dấu để chọn các phiên bản để so sánh rồi nhấn enter hoặc nút ở dưới.<br />
Chú giải: (hiện) = khác với phiên bản hiện hành,
(trước) = khác với phiên bản trước, n = sửa đổi nhỏ.',
'deletedrev'          => '[đã xóa]',
'histfirst'           => 'Cũ nhất',
'histlast'            => 'Mới nhất',
'historysize'         => '({{PLURAL:$1|1 byte|$1 byte}})',
'historyempty'        => '(trống)',

# Revision feed
'history-feed-title'          => 'Lịch sử thay đổi',
'history-feed-description'    => 'Lịch sử thay đổi của trang này ở wiki',
'history-feed-item-nocomment' => '$1 vào lúc $2', # user at time
'history-feed-empty'          => 'Trang bạn yêu cầu không tồn tại. Có thể là nó đã bị xóa khỏi wiki hay được đổi tên. Hãy [[Special:Search|tìm kiếm trong wiki]] về các trang mới có liên quan.',

# Revision deletion
'rev-deleted-comment'         => '(bàn luận đã xóa)',
'rev-deleted-user'            => '(tên người dùng đã xóa)',
'rev-deleted-event'           => '(tác vụ nhật trình đã xóa)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">
Phiên bản này đã bị xóa khỏi các bản lưu mà mọi người có thể thấy.
Có thể có thêm chi tiết tại [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} nhật trình xóa].
</div>',
'rev-deleted-text-view'       => '<div class="mw-warning plainlinks">
Phiên bản này đã bị xóa khỏi các bản lưu mà mọi người có thể thấy.
Vì bạn là người quản lý ở {{SITENAME}}, bạn có thể xem được nó;
có thể có thêm chi tiết tại [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} nhật trình xóa].
</div>',
'rev-delundel'                => 'hiện/ẩn',
'revisiondelete'              => 'Xóa hay phục hồi phiên bản',
'revdelete-nooldid-title'     => 'Chưa chọn phiên bản',
'revdelete-nooldid-text'      => 'Có thể bạn chưa xác định (các) phiên bản đích để thực hiện tác vụ,
hoặc phiên bản đích không tồn tại,
hoặc bạn đang tìm cách ẩn phiên bản hiện tại.',
'revdelete-selected'          => '{{PLURAL:$2|Phiên bản|Các phiên bản}} được chọn của [[:$1]]:',
'logdelete-selected'          => '{{PLURAL:$1|Nhật trình đã chọn|Các nhật trình đã chọn}}:',
'revdelete-text'              => 'Các phiên bản và sự kiện bị xóa vẫn còn trong lịch sử trang và nhật trình, nhưng mọi người sẽ không xem được một số phần của các nội dung đó.

Các quản lý khác ở {{SITENAME}} vẫn có thể truy nhập vào nội dung ẩn và phục hồi lại bằng cách dùng giao diện này, trừ trường hợp thiết lập thêm một số hạn chế.',
'revdelete-legend'            => 'Thiết lập hạn chế khả kiến',
'revdelete-hide-text'         => 'Ẩn nội dung phiên bản',
'revdelete-hide-name'         => 'Ẩn tác vụ và đích của tác vụ',
'revdelete-hide-comment'      => 'Ẩn tóm lược sửa đổi',
'revdelete-hide-user'         => 'Ẩn tên người dùng hay địa chỉ IP của người viết trang',
'revdelete-hide-restricted'   => 'Áp dụng những hạn chế này đối với Người quản lý và khóa giao diện này lại',
'revdelete-suppress'          => 'Che dữ liệu đối với người quản lý cũng như các thành viên khác',
'revdelete-hide-image'        => 'Ẩn nội dung tập tin',
'revdelete-unsuppress'        => 'Bỏ các hạn chế trên các phiên bản được phục hồi',
'revdelete-log'               => 'Tóm lược trong nhật trình:',
'revdelete-submit'            => 'Áp dụng vào phiên bản được chọn',
'revdelete-logentry'          => 'đã thay đổi khả năng nhìn thấy phiên bản của [[$1]]',
'logdelete-logentry'          => 'đã thay đổi khả năng nhìn thấy sự kiện của [[$1]]',
'revdelete-success'           => "'''Khả năng nhìn thấy của phiên bản đã được thiết lập thành công.'''",
'logdelete-success'           => "'''Khả năng nhìn thấy của sự kiện đã được thiết lập thành công.'''",
'revdel-restore'              => 'Thay đổi mức khả kiến',
'pagehist'                    => 'Lịch sử trang',
'deletedhist'                 => 'Lịch sử đã xóa',
'revdelete-content'           => 'nội dung',
'revdelete-summary'           => 'tóm lược sửa đổi',
'revdelete-uname'             => 'tên người dùng',
'revdelete-restricted'        => 'áp dụng hạn chế này cho sysop',
'revdelete-unrestricted'      => 'gỡ bỏ hạn chế này cho sysop',
'revdelete-hid'               => 'đã ẩn $1',
'revdelete-unhid'             => 'đã hiện $1',
'revdelete-log-message'       => '$2 {{PLURAL:$2|phiên bản|phiên bản}} được $1',
'logdelete-log-message'       => '$1 của $2 {{PLURAL:$2|sự kiện|sự kiện}}',

# Suppression log
'suppressionlog'     => 'Nhật trình ẩn giấu',
'suppressionlogtext' => 'Dưới đây là danh sách các tác vụ xóa và cấm liên quan đến nội dung mà các quản lý không nhìn thấy. 
Xem [[Special:IPBlockList|danh sách các IP bị cấm]] để xem danh sách các tác vụ cấm chỉ và cấm thông thường hiện nay.',

# History merging
'mergehistory'                     => 'Trộn lịch sử trang',
'mergehistory-header'              => 'Trang này cho phép trộn các sửa đổi trong lịch sử của một trang nguồn vào một trang mới hơn.
Xin hãy bảo đảm giữ vững tính liên tục của lịch sử trang.',
'mergehistory-box'                 => 'Trộn các sửa đổi của hai trang:',
'mergehistory-from'                => 'Trang nguồn:',
'mergehistory-into'                => 'Trang đích:',
'mergehistory-list'                => 'Lịch sử sửa đổi có thể trộn được',
'mergehistory-merge'               => 'Các sửa đổi sau của [[:$1]] có thể trộn được với [[:$2]]. Dùng một nút chọn trong cột để trộn các sửa đổi từ đầu cho đến thời điểm đã chọn. Lưu ý là việc dùng các liên kết chuyển hướng sẽ khởi tạo lại cột này.',
'mergehistory-go'                  => 'Hiển thị các sửa đổi có thể trộn được',
'mergehistory-submit'              => 'Trộn các sửa đổi',
'mergehistory-empty'               => 'Không thể trộn được sửa đổi nào.',
'mergehistory-success'             => '$3 {{PLURAL:$3|sửa đổi|sửa đổi}} của [[:$1]] đã được trộn vào [[:$2]].',
'mergehistory-fail'                => 'Không thể thực hiện được việc trộn lịch sử sửa đổi, vui lòng chọn lại trang cũng như thông số ngày giờ.',
'mergehistory-no-source'           => 'Trang nguồn $1 không tồn tại.',
'mergehistory-no-destination'      => 'Trang đích $1 không tồn tại.',
'mergehistory-invalid-source'      => 'Trang nguồn phải có tiêu đề hợp lệ.',
'mergehistory-invalid-destination' => 'Trang đích phải có tiêu đề hợp lệ.',
'mergehistory-autocomment'         => 'Đã trộn [[:$1]] vào [[:$2]]',
'mergehistory-comment'             => 'Đã trộn [[:$1]] vào [[:$2]]: $3',

# Merge log
'mergelog'           => 'Nhật trình trộn',
'pagemerge-logentry' => 'đã trộn [[$1]] vào [[$2]] (sửa đổi cho đến $3)',
'revertmerge'        => 'Bỏ trộn',
'mergelogpagetext'   => 'Dưới đây là danh sách các thao tác trộn mới nhất của lịch sử một trang vào trang khác.',

# Diffs
'history-title'           => 'Lịch sử sửa đổi của “$1”',
'difference'              => '(Khác biệt giữa các bản)',
'lineno'                  => 'Dòng $1:',
'compareselectedversions' => 'So sánh các bản đã chọn',
'editundo'                => 'lùi sửa',
'diff-multi'              => '(Không hiển thị {{PLURAL:$1|một|$1}} phiên bản ở giữa)',

# Search results
'searchresults'             => 'Kết quả tìm kiếm',
'searchresulttext'          => 'Để biết thêm chi tiết về tìm kiếm tại {{SITENAME}}, xem [[{{MediaWiki:Helppage}}|{{int:help}}]].',
'searchsubtitle'            => "Bạn đã tìm '''[[:$1]]''' ([[Special:Prefixindex/$1|tất cả các trang bắt đầu bằng “$1”]] | [[Special:WhatLinksHere/$1|tất cả các trang liên kết đến “$1”]])",
'searchsubtitleinvalid'     => "Tìm '''$1'''",
'noexactmatch'              => "'''Trang “$1” không tồn tại.''' Bạn có thể [[:$1|tạo trang này]].",
'noexactmatch-nocreate'     => "'''Không có trang nào có tên “$1”.'''",
'toomanymatches'            => 'Có quá nhiều kết quả được trả về, xin hãy thử câu tìm kiếm khác',
'titlematches'              => 'Đề mục tương tự',
'notitlematches'            => 'Không có tên trang nào có nội dung tương tự',
'textmatches'               => 'Câu chữ tương tự',
'notextmatches'             => 'Không tìm thấy nội dung trang',
'prevn'                     => '$1 trước',
'nextn'                     => '$1 sau',
'viewprevnext'              => 'Xem ($1) ($2) ($3).',
'search-result-size'        => '$1 ({{PLURAL:$2|1 từ|$2 từ}})',
'search-result-score'       => 'Độ phù hợp: $1%',
'search-redirect'           => '(đổi hướng $1)',
'search-section'            => '(đề mục $1)',
'search-suggest'            => 'Có phải bạn muốn tìm: $1',
'search-interwiki-caption'  => 'Các dự án liên quan',
'search-interwiki-default'  => '$1 kết quả:',
'search-interwiki-more'     => '(thêm)',
'search-mwsuggest-enabled'  => 'có gợi ý',
'search-mwsuggest-disabled' => 'không có gợi ý',
'search-relatedarticle'     => 'Liên quan',
'mwsuggest-disable'         => 'Tắt gợi ý bằng AJAX',
'searchrelated'             => 'có liên quan',
'searchall'                 => 'tất cả',
'showingresults'            => "Dưới đây là {{PLURAL:$1|'''1'''|'''$1'''}} kết quả bắt đầu từ #'''$2'''.",
'showingresultsnum'         => "Dưới đây là {{PLURAL:$3|'''1'''|'''$3'''}} kết quả bắt đầu từ #'''$2'''.",
'showingresultstotal'       => "Dưới đây là {{PLURAL:$3|kết quả '''$1''' trong '''$3'''|những kết quả từ '''$1 - $2''' trong tổng số '''$3'''}}",
'nonefound'                 => "'''Chú ý''': Theo mặc định chỉ tìm kiếm một số không gian tên. Hãy thử bắt đầu từ khóa bằng ''all:'' để tìm mọi nội dung (kể cả trang thảo luận, tiêu bản, v.v.), hoặc bắt đầu bằng không gian tên mong muốn (ví dụ ''Thảo luận:'', ''Tiêu bản:'', ''Thể loại:''…).",
'powersearch'               => 'Tìm kiếm nâng cao',
'powersearch-legend'        => 'Tìm kiếm nâng cao',
'powersearch-ns'            => 'Tìm trong không gian tên:',
'powersearch-redir'         => 'Liệt kê cả trang đổi hướng',
'powersearch-field'         => 'Tìm',
'search-external'           => 'Tìm kiếm từ bên ngoài',
'searchdisabled'            => 'Chức năng tìm kiếm tại {{SITENAME}} đã bị tắt. Bạn có tìm kiếm bằng Google trong thời gian này. Chú ý rằng các chỉ mục từ {{SITENAME}} của chúng có thể đã lỗi thời.',

# Preferences page
'preferences'              => 'Tùy chọn',
'mypreferences'            => 'Tùy chọn',
'prefs-edits'              => 'Số lần sửa đổi:',
'prefsnologin'             => 'Chưa đăng nhập',
'prefsnologintext'         => 'Bạn phải <span class="plainlinks">[{{fullurl:Special:Userlogin|returnto=$1}} đăng nhập]</span> để thiết lập tùy chọn cá nhân.',
'prefsreset'               => 'Các tùy chọn cá nhân đã được mặc định lại.',
'qbsettings'               => 'Thanh công cụ',
'qbsettings-none'          => 'Không có',
'qbsettings-fixedleft'     => 'Cố định trái',
'qbsettings-fixedright'    => 'Cố định phải',
'qbsettings-floatingleft'  => 'Nổi bên trái',
'qbsettings-floatingright' => 'Nổi bên phải',
'changepassword'           => 'Đổi mật khẩu',
'skin'                     => 'Hình dạng',
'math'                     => 'Công thức toán',
'dateformat'               => 'Kiểu ngày tháng',
'datedefault'              => 'Không lựa chọn',
'datetime'                 => 'Ngày tháng',
'math_failure'             => 'Không thể phân tích cú pháp',
'math_unknown_error'       => 'lỗi lạ',
'math_unknown_function'    => 'hàm lạ',
'math_lexing_error'        => 'lỗi chính tả',
'math_syntax_error'        => 'lỗi cú pháp',
'math_image_error'         => 'Không chuyển sang định dạng PNG được; xin kiểm tra lại cài đặt latex, dvips, gs và convert',
'math_bad_tmpdir'          => 'Không tạo mới hay viết vào thư mục toán tạm thời được',
'math_bad_output'          => 'Không tạo mới hay viết vào thư mục kết quả được',
'math_notexvc'             => 'Không thấy hàm thực thi texvc; xin xem math/README để biết cách cấu hình.',
'prefs-personal'           => 'Thông tin cá nhân',
'prefs-rc'                 => 'Thay đổi gần đây',
'prefs-watchlist'          => 'Theo dõi',
'prefs-watchlist-days'     => 'Số ngày hiển thị trong danh sách theo dõi:',
'prefs-watchlist-edits'    => 'Số lần sửa đổi tối đa trong danh sách theo dõi mở rộng:',
'prefs-misc'               => 'Linh tinh',
'saveprefs'                => 'Lưu tùy chọn',
'resetprefs'               => 'Mặc định lại lựa chọn',
'oldpassword'              => 'Mật khẩu cũ:',
'newpassword'              => 'Mật khẩu mới:',
'retypenew'                => 'Gõ lại:',
'textboxsize'              => 'Sửa đổi',
'rows'                     => 'Số hàng:',
'columns'                  => 'Số cột:',
'searchresultshead'        => 'Tìm kiếm',
'resultsperpage'           => 'Số kết quả mỗi trang:',
'contextlines'             => 'Số hàng trong trang dùng để tìm ra kết quả:',
'contextchars'             => 'Số chữ trong một hàng kết quả:',
'stub-threshold'           => 'Định dạng <a href="#" class="stub">liên kết đến sơ khai</a> cho các trang ngắn hơn (byte):',
'recentchangesdays'        => 'Số ngày hiển thị trong thay đổi gần đây:',
'recentchangescount'       => 'Số sửa đổi hiển thị trong trang thay đổi gần đây, lịch sử và nhật trình:',
'savedprefs'               => 'Đã lưu các tùy chọn cá nhân.',
'timezonelegend'           => 'Múi giờ',
'timezonetext'             => '¹Số giờ chênh lệch giữa giờ địa phương của bạn với giờ máy chủ (UTC)',
'localtime'                => 'Giờ địa phương',
'timezoneoffset'           => 'Chênh giờ¹',
'servertime'               => 'Giờ máy chủ',
'guesstimezone'            => 'Dùng giờ của trình duyệt',
'allowemail'               => 'Nhận thư điện tử từ các thành viên khác',
'prefs-searchoptions'      => 'Lựa chọn tìm kiếm',
'prefs-namespaces'         => 'Không gian tên',
'defaultns'                => 'Mặc định tìm kiếm trong không gian tên:',
'default'                  => 'mặc định',
'files'                    => 'Tập tin',

# User rights
'userrights'                  => 'Quản lý quyền thành viên', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => 'Quản lý nhóm thành viên',
'userrights-user-editname'    => 'Nhập tên thành viên:',
'editusergroup'               => 'Sửa nhóm thành viên',
'editinguser'                 => "Thay đổi quyền hạn của thành viên '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup'    => 'Sửa nhóm thành viên',
'saveusergroups'              => 'Lưu nhóm thành viên',
'userrights-groupsmember'     => 'Thuộc nhóm:',
'userrights-groups-help'      => 'Bạn có thể xếp thành viên này vào nhóm khác:
* Hộp kiểm được đánh dấu có nghĩa rằng thành viên thuộc về nhóm đó.
* Hộp không được đánh dấu có nghĩa rằng thành viên không thuộc về nhóm đó.
* Dấu * có nghĩa là bạn sẽ không thể loại thành viên ra khỏi nhóm một khi bạn đã đưa thành viên vào, hoặc ngược lại.',
'userrights-reason'           => 'Lý do thay đổi:',
'userrights-no-interwiki'     => 'Bạn không có quyền thay đổi quyền hạn của thành viên tại các wiki khác.',
'userrights-nodatabase'       => 'Cơ sở dữ liệu $1 không tồn tại hoặc nằm ở bên ngoài.',
'userrights-nologin'          => 'Bạn phải [[Special:UserLogin|đăng nhập]] vào một tài khoản có quyền quản lý để gán quyền cho thành viên.',
'userrights-notallowed'       => 'Tài khoản của bạn không có quyền gán quyền cho thành viên.',
'userrights-changeable-col'   => 'Những nhóm bạn có thể thay đổi',
'userrights-unchangeable-col' => 'Những nhóm bạn không thể thay đổi',

# Groups
'group'               => 'Nhóm:',
'group-user'          => 'Thành viên thông thường',
'group-autoconfirmed' => 'Thành viên tự xác nhận',
'group-bot'           => 'Robot',
'group-sysop'         => 'Quản lý',
'group-bureaucrat'    => 'Hành chính viên',
'group-suppress'      => 'Giám sát viên',
'group-all'           => '(tất cả)',

'group-user-member'          => 'Thành viên',
'group-autoconfirmed-member' => 'Thành viên tự động xác nhận',
'group-bot-member'           => 'Robot',
'group-sysop-member'         => 'Quản lý',
'group-bureaucrat-member'    => 'Hành chính viên',
'group-suppress-member'      => 'Giám sát viên',

'grouppage-user'          => '{{ns:project}}:Thành viên',
'grouppage-autoconfirmed' => '{{ns:project}}:Thành viên tự xác nhận',
'grouppage-bot'           => '{{ns:project}}:Robot',
'grouppage-sysop'         => '{{ns:project}}:Người quản lý',
'grouppage-bureaucrat'    => '{{ns:project}}:Hành chính viên',
'grouppage-suppress'      => '{{ns:project}}:Giám sát viên',

# Rights
'right-read'                 => 'Đọc trang',
'right-edit'                 => 'Sửa trang',
'right-createpage'           => 'Tạo trang (không phải trang thảo luận)',
'right-createtalk'           => 'Tạo trang thảo luận',
'right-createaccount'        => 'Mở tài khoản mới',
'right-minoredit'            => 'Đánh dấu sửa đổi nhỏ',
'right-move'                 => 'Di chuyển trang',
'right-move-subpages'        => 'Di chuyển trang cùng với các trang con của nó',
'right-suppressredirect'     => 'Không tạo đổi hướng từ tên cũ khi di chuyển trang',
'right-upload'               => 'Tải tập tin lên',
'right-reupload'             => 'Tải đè tập tin cũ',
'right-reupload-own'         => 'Tải đè tập tin cũ do chính mình tải lên',
'right-reupload-shared'      => 'Ghi đè lên kho hình ảnh dùng chung',
'right-upload_by_url'        => 'Tải tập tin từ địa chỉ URL',
'right-purge'                => 'Tẩy bộ đệm của trang mà không có trang xác nhận',
'right-autoconfirmed'        => 'Sửa trang bị nửa khóa',
'right-bot'                  => 'Được đối xử như tác vụ tự động',
'right-nominornewtalk'       => 'Không báo về tin nhắn mới khi trang thảo luận chỉ được sửa đổi nhỏ',
'right-apihighlimits'        => 'Được dùng giới hạn cao hơn khi truy vấn API',
'right-writeapi'             => 'Sử dụng API để viết',
'right-delete'               => 'Xóa trang',
'right-bigdelete'            => 'Xóa trang có lịch sử lớn',
'right-deleterevision'       => 'Xóa và phục hồi phiên bản nào đó của trang',
'right-deletedhistory'       => 'Xem phần lịch sử đã xóa, mà không xem nội dung đi kèm',
'right-browsearchive'        => 'Tìm những trang đã xóa',
'right-undelete'             => 'Phục hồi trang',
'right-suppressrevision'     => 'Xem lại và phục hồi phiên bản mà Sysop không thấy',
'right-suppressionlog'       => 'Xem nhật trình riêng tư',
'right-block'                => 'Cấm thành viên khác sửa đổi',
'right-blockemail'           => 'Cấm thành viên gửi thư',
'right-hideuser'             => 'Cấm thành viên, rồi ẩn nó đi',
'right-ipblock-exempt'       => 'Bỏ qua cấm IP, tự động cấm và cấm dải IP',
'right-proxyunbannable'      => 'Bỏ qua cấm proxy tự động',
'right-protect'              => 'Thay đổi mức khóa và sửa trang khóa',
'right-editprotected'        => 'Sửa trang khóa (không bị khóa theo tầng)',
'right-editinterface'        => 'Sửa giao diện người dùng',
'right-editusercssjs'        => 'Sửa tập tin CSS và JS của người dùng khác',
'right-rollback'             => 'Nhanh chóng lùi tất cả sửa đổi của thành viên cuối cùng sửa đổi tại trang nào đó',
'right-markbotedits'         => 'Đánh dấu sửa đổi phục hồi là sửa đổi bot',
'right-noratelimit'          => 'Không bị ảnh hưởng bởi mức giới hạn tần suất sử dụng',
'right-import'               => 'Nhập trang từ wiki khác',
'right-importupload'         => 'Nhập trang bằng tải tập tin',
'right-patrol'               => 'Đánh dấu tuần tra sửa đổi',
'right-autopatrol'           => 'Tự động đánh dấu tuần tra khi sửa đổi',
'right-patrolmarks'          => 'Dùng tính năng tuần tra thay đổi gần đây',
'right-unwatchedpages'       => 'Xem danh sách các trang chưa theo dõi',
'right-trackback'            => 'Đăng trackback',
'right-mergehistory'         => 'Trộn lịch sử trang',
'right-userrights'           => 'Sửa tất cả quyền thành viên',
'right-userrights-interwiki' => 'Sửa quyền thành viên của các thành viên ở các wiki khác',
'right-siteadmin'            => 'Khóa và mở khóa cơ sở dữ liệu',

# User rights log
'rightslog'      => 'Nhật trình cấp quyền thành viên',
'rightslogtext'  => 'Đây là nhật trình lưu những thay đổi đối với các quyền hạn thành viên.',
'rightslogentry' => 'đã đổi cấp của thành viên $1 từ $2 thành $3',
'rightsnone'     => '(không có)',

# Recent changes
'nchanges'                          => '$1 {{PLURAL:$1|thay đổi|thay đổi}}',
'recentchanges'                     => 'Thay đổi gần đây',
'recentchangestext'                 => 'Xem các thay đổi gần đây nhất tại wiki trên trang này.',
'recentchanges-feed-description'    => 'Theo dõi các thay đổi gần đây nhất của wiki dùng feed này.',
'rcnote'                            => "Dưới đây là {{PLURAL:$1|'''1''' thay đổi|'''$1''' thay đổi gần nhất}} trong {{PLURAL:$2|ngày qua|'''$2''' ngày qua}}, tính tới $5, $4.",
'rcnotefrom'                        => "Thay đổi từ '''$2''' (hiển thị tối đa '''$1''' thay đổi).",
'rclistfrom'                        => 'Hiển thị các thay đổi từ $1.',
'rcshowhideminor'                   => '$1 sửa đổi nhỏ',
'rcshowhidebots'                    => '$1 sửa đổi bot',
'rcshowhideliu'                     => '$1 sửa đổi thành viên',
'rcshowhideanons'                   => '$1 sửa đổi vô danh',
'rcshowhidepatr'                    => '$1 sửa đổi đã tuần tra',
'rcshowhidemine'                    => '$1 sửa đổi của tôi',
'rclinks'                           => 'Xem $1 sửa đổi gần đây nhất trong $2 ngày qua; $3.',
'diff'                              => 'khác',
'hist'                              => 'sử',
'hide'                              => 'ẩn',
'show'                              => 'hiện',
'minoreditletter'                   => 'n',
'newpageletter'                     => 'M',
'boteditletter'                     => 'b',
'number_of_watching_users_pageview' => '[$1 {{PLURAL:$1|người|người}} đang xem]',
'rc_categories'                     => 'Hạn chế theo thể loại (phân cách bằng “|”)',
'rc_categories_any'                 => 'Bất kỳ',
'newsectionsummary'                 => 'Đề mục mới: /* $1 */',

# Recent changes linked
'recentchangeslinked'          => 'Thay đổi liên quan',
'recentchangeslinked-title'    => 'Thay đổi liên quan tới “$1”',
'recentchangeslinked-noresult' => 'Không có thay đổi nào trên trang được liên kết đến trong khoảng thời gian đã chọn.',
'recentchangeslinked-summary'  => "Đây là danh sách các thay đổi được thực hiện gần đây tại những trang được liên kết đến từ một trang nào đó (hoặc tại các trang thuộc một thể loại nào đó).
Các trang trong [[Special:Watchlist|danh sách bạn theo dõi]] được '''tô đậm'''.",
'recentchangeslinked-page'     => 'Tên trang:',
'recentchangeslinked-to'       => 'Hiển thị những thay đổi tại những trang trang được liên kết đến từ trang cho trước thay cho trang này',

# Upload
'upload'                      => 'Tải tập tin lên',
'uploadbtn'                   => 'Tải lên',
'reupload'                    => 'Tải lại',
'reuploaddesc'                => 'Hủy tác vụ tải và quay lại mẫu tải tập tin lên',
'uploadnologin'               => 'Chưa đăng nhập',
'uploadnologintext'           => 'Bạn phải [[Special:UserLogin|đăng nhập]] để tải tập tin lên.',
'upload_directory_missing'    => 'Thư mục tải lên ($1) không có hoặc máy chủ web không thể tạo được.',
'upload_directory_read_only'  => 'Máy chủ không thể sửa đổi thư mục tải lên ($1) được.',
'uploaderror'                 => 'Lỗi khi tải lên',
'uploadtext'                  => "Hãy sử dụng mẫu sau để tải tập tin lên.
Để xem hoặc tìm kiếm những hình ảnh đã được tải lên trước đây, xin mời xem [[Special:ImageList|danh sách các tập tin đã tải lên]]. 
việc tải lên và tải lên lại được ghi lại trong [[Special:Log/upload|nhật trình tải lên]],  việc xóa đi được ghi trong [[Special:Log/delete|nhật trình xóa]].

Để đưa tập tin vào trang, hãy dùng liên kết có một trong các dạng sau:
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:Tập tin.jpg]]</nowiki></tt>''' để phiên bản đầy đủ của tập tin
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:Tập tin.png|200px|nhỏ|trái|văn bản thay thế]]</nowiki></tt>''' để dùng hình đã được co lại còn 200 pixel chiều rộng đặt trong một hộp ở lề bên trái với 'văn bản thay thế' dùng để mô tả
* '''<tt><nowiki>[[</nowiki>{{ns:media}}<nowiki>:Tập tin.ogg]]</nowiki></tt>''' để liên kết trực tiếp đến tập tin mà không hiển thị nó",
'upload-permitted'            => 'Các định dạng tập tin được phép tải lên: $1.',
'upload-preferred'            => 'Các định dạng tập tin nên dùng: $1.',
'upload-prohibited'           => 'Các định dạng tập tin bị cấm: $1.',
'uploadlog'                   => 'nhật trình tải lên',
'uploadlogpage'               => 'Nhật trình tải lên',
'uploadlogpagetext'           => 'Dưới đây là danh sách các tập tin đã tải lên gần nhất.
Xem [[Special:NewImages|trang trưng bày các tập tin mới]] để xem trực quan hơn.',
'filename'                    => 'Tên tập tin:',
'filedesc'                    => 'Miêu tả:',
'fileuploadsummary'           => 'Tóm lược:',
'filestatus'                  => 'Bản quyền:',
'filesource'                  => 'Nguồn:',
'uploadedfiles'               => 'Tập tin đã tải',
'ignorewarning'               => 'Bỏ qua cảnh báo và lưu tập tin',
'ignorewarnings'              => 'Bỏ qua cảnh báo',
'minlength1'                  => 'Tên tập tin phải có ít nhất một ký tự.',
'illegalfilename'             => 'Tên tập tin “$1” có chứa ký tự không được phép dùng cho tựa trang. Xin hãy đổi tên và tải lên lại.',
'badfilename'                 => 'Tên tập tin đã được đổi thành “$1”.',
'filetype-badmime'            => 'Không thể tải lên các tập tin có định dạng MIME “$1”.',
'filetype-unwanted-type'      => "'''“.$1”''' là định dạng tập tin không được trông đợi.
{{PLURAL:$3|Loại|Những loại}} tập tin thích hợp hơn là $2.",
'filetype-banned-type'        => "'''“.$1”''' là định dạng tập tin không được chấp nhận.
{{PLURAL:$3|Loại tập tin|Những loại tập tin}} được chấp nhận là $2.",
'filetype-missing'            => 'Tập tin không có phần mở rộng (ví dụ “.jpg”).',
'large-file'                  => 'Các tập tin được khuyến cáo không được lớn hơn $1; tập tin này lớn đến $2.',
'largefileserver'             => 'Tập tin này quá lớn so với khả năng phục vụ của máy chủ.',
'emptyfile'                   => 'Tập tin bạn vừa mới tải lên có vẻ trống không. Điều này có thể xảy ra khi bạn đánh sai tên tập tin. Xin hãy chắc chắn rằng bạn thật sự muốn tải lên tập tin này.',
'fileexists'                  => 'Một tập tin với tên này đã tồn tại, xin hãy kiểm tra lại <strong><tt>$1</tt></strong> nếu bạn không chắc bạn có muốn thay đổi nó hay không.',
'filepageexists'              => 'Trang miêu tả tập tin này đã tồn tại ở <strong><tt>$1</tt></strong>, nhưng chưa có tập tin với tên này. Những gì bạn ghi trong ô "Tóm tắt tập tin" sẽ không hiện ra ở trang miêu tả; để làm nó hiển thị, bạn sẽ cần phải sửa đổi trang đó bằng tay.',
'fileexists-extension'        => 'Hiện có một tập tin trùng tên:<br />
Tên tập tin đang tải lên: <strong><tt>$1</tt></strong><br />
Tên tập tin có từ trước: <strong><tt>$2</tt></strong><br />
Xin hãy chọn một tên tập tin khác.',
'fileexists-thumb'            => "<center>'''Tập tin đã tồn tại'''</center>",
'fileexists-thumbnail-yes'    => 'Tập tin này có vẻ là hình có kích thước thu gọn <i>(hình thu nhỏ)</i>. Xin kiểm tra lại tập tin <strong><tt>$1</tt></strong>.<br />
Nếu tập tin được kiểm tra trùng với hình có kích cỡ gốc thì không cần thiết tải lên một hình thu nhỏ khác.',
'file-thumbnail-no'           => 'Tên tập tin bắt đầu bằng <strong><tt>$1</tt></strong>.
Có vẻ đây là bản thu nhỏ của hình gốc <i>(thumbnail)</i>.
Nếu bạn có hình ở độ phân giải tối đa, xin hãy tải bản đó lên, nếu không xin hãy đổi lại tên tập tin.',
'fileexists-forbidden'        => 'Đã có tập tin với tên gọi này; xin quay lại để tải tập tin này lên dưới tên khác. [[Image:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => 'Một tập tin với tên này đã tồn tại ở kho tập tin dùng chung.
Nếu bạn vẫn muốn tải tập tin của bạn lên, xin hãy quay lại và dùng một tên khác. [[Image:$1|thumb|center|$1]]',
'file-exists-duplicate'       => 'Tập tin này có vẻ là bản sao của {{PLURAL:$1|tập tin|các  tập tin}} sau:',
'successfulupload'            => 'Đã tải xong',
'uploadwarning'               => 'Cảnh báo!',
'savefile'                    => 'Lưu tập tin',
'uploadedimage'               => 'đã tải “[[$1]]” lên',
'overwroteimage'              => 'đã tải lên một phiên bản mới của “[[$1]]”',
'uploaddisabled'              => 'Chức năng tải lên đã bị khóa.',
'uploaddisabledtext'          => 'Chức năng tải tập tin đã bị tắt trên {{SITENAME}}.',
'uploadscripted'              => 'Tập tin này có chứa mã HTML hoặc script có thể khiến trình duyệt web thông dịch sai.',
'uploadcorrupt'               => 'Tập tin bị hỏng hoặc có phần mở rộng không đúng. Xin kiểm tra và tải lại.',
'uploadvirus'                 => 'Tập tin có virút! Chi tiết: $1',
'sourcefilename'              => 'Tên tập tin nguồn:',
'destfilename'                => 'Tên tập tin mới:',
'upload-maxfilesize'          => 'Kích thước tập tin tối đa: $1',
'watchthisupload'             => 'Theo dõi tập tin này',
'filewasdeleted'              => 'Một tên với tên này đã được tải lên trước đã rồi sau đó bị xóa. Bạn nên kiểm tra lại $1 trước khi tải nó lên lại lần nữa.',
'upload-wasdeleted'           => "'''Cảnh báo: Bạn đang tải lên một tập tin từng bị xóa trước đây.'''

Bạn nên cân nhắc trong việc tiếp tục tải lên tập tin này. Nhật trình xóa của tập tin được đưa ra dưới đây để tiện theo dõi:",
'filename-bad-prefix'         => 'Tên cho tập tin mà bạn đang tải lên bắt đầu bằng <strong>“$1”</strong>, đây không phải là dạng tên tiêu biểu có tính chất miêu tả do các máy chụp ảnh số tự động đặt. Xin hãy chọn một tên có tính chất miêu tả và gợi nhớ hơn cho tập tin của bạn.',
'filename-prefix-blacklist'   => ' #<!-- xin để nguyên hàng này --> <pre>
# Cú pháp như sau:
#   * Các ký tự từ dấu "#" trở đến cuối hàng là chú thích
#   * Các dòng sau là các tiền tố do các máy ảnh số gán tự động cho tên tập tin
CIMG # Casio
DSC_ # Nikon
DSCF # Fuji
DSCN # Nikon
DUW # một số điện thoại di động
IMG # tổng quát
JD # Jenoptik
MGP # Pentax
PICT # khác
 #</pre> <!-- xin để nguyên hàng này -->',

'upload-proto-error'      => 'Giao thức sai',
'upload-proto-error-text' => 'Phải đưa vào URL bắt đầu với <code>http://</code> hay <code>ftp://</code> để tải lên tập tin từ trang web khác.',
'upload-file-error'       => 'Lỗi nội bộ',
'upload-file-error-text'  => 'Có lỗi nội bộ khi tạo tập tin tạm trên máy chủ.
Xin hãy liên hệ với một [[Special:ListUsers/sysop|bảo quản viên]].',
'upload-misc-error'       => 'Có lỗi lạ khi tải lên',
'upload-misc-error-text'  => 'Có lỗi lạ khi tải lên.
Xin hãy xác nhận lại địa chỉ URL là hợp lệ và có thể truy cập được không rồi thử lại lần nữa.
Nếu vẫn còn bị lỗi, xin hãy liên hệ với một [[Special:ListUsers/sysop|bảo quản viên]].',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => 'Không thể truy cập URL',
'upload-curl-error6-text'  => 'Không thể truy cập URL mà bạn đưa vào. Xin hãy kiểm tra xem URL có đúng không và website vẫn còn hoạt động.',
'upload-curl-error28'      => 'Quá thời gian tải lên cho phép',
'upload-curl-error28-text' => 'Trang web phản hồi quá chậm. Xin hãy kiểm tra lại xem trang web còn hoạt động hay không, đợi một thời gian ngắn rồi thử lại. Bạn nên thử lại vào lúc trang rảnh rỗi hơn.',

'license'            => 'Giấy phép:',
'nolicense'          => 'chưa chọn',
'license-nopreview'  => '(Không xem trước được)',
'upload_source_url'  => ' (địa chỉ URL đúng, có thể truy cập)',
'upload_source_file' => ' (tập tin trên máy của bạn)',

# Special:ImageList
'imagelist-summary'     => 'Trang đặc biệt này liệt kê các tập tin được tải lên.
Theo mặc định, các tập tin mới nhất được xếp vào đầu danh sách.
Hãy nhấn chuột vào tiêu đề cột để thay đổi thứ tự sắp xếp.',
'imagelist_search_for'  => 'Tìm kiếm theo tên tập tin:',
'imgfile'               => 'tập tin',
'imagelist'             => 'Danh sách tập tin',
'imagelist_date'        => 'Ngày tải',
'imagelist_name'        => 'Tên',
'imagelist_user'        => 'Thành viên tải',
'imagelist_size'        => 'Kích cỡ',
'imagelist_description' => 'Miêu tả',

# Image description page
'filehist'                       => 'Lịch sử tập tin',
'filehist-help'                  => 'Nhấn vào một ngày/giờ để xem nội dung tập tin tại thời điểm đó.',
'filehist-deleteall'             => 'xóa toàn bộ',
'filehist-deleteone'             => 'xóa bản này',
'filehist-revert'                => 'lùi lại',
'filehist-current'               => 'hiện',
'filehist-datetime'              => 'Ngày/Giờ',
'filehist-user'                  => 'Thành viên',
'filehist-dimensions'            => 'Kích cỡ',
'filehist-filesize'              => 'Kích thước tập tin',
'filehist-comment'               => 'Miêu tả',
'imagelinks'                     => 'Liên kết',
'linkstoimage'                   => '{{PLURAL:$1|Trang|$1 trang}} sau có liên kết đến tập tin này:',
'nolinkstoimage'                 => 'Không có trang nào chứa liên kết đến hình.',
'morelinkstoimage'               => 'Xem [[Special:WhatLinksHere/$1|thêm liên kết]] đến tập tin này.',
'redirectstofile'                => '{{PLURAL:$1|Tập tin|$1 tập tin}} sau chuyển hướng đến tập tin này:',
'duplicatesoffile'               => '{{PLURAL:$1|Tập tin sau|$1 tập tin sau}} là bản sao của tập tin này:',
'sharedupload'                   => 'Tập tin này được tải lên để dùng chung và có thể dùng ở các dự án khác.',
'shareduploadwiki'               => 'Xin xem $1 để biết thêm thông tin.',
'shareduploadwiki-desc'          => 'Dưới đây là nội dung từ $1 tại kho lưu trữ chung.',
'shareduploadwiki-linktext'      => 'trang miêu tả tập tin',
'shareduploadduplicate'          => 'Tập tin này là bản sao của $1 từ kho tập tin dùng chung.',
'shareduploadduplicate-linktext' => 'tập tin khác',
'shareduploadconflict'           => 'Tập tin này trùng tên với $1 từ kho tập tin dùng chung.',
'shareduploadconflict-linktext'  => 'tập tin khác',
'noimage'                        => 'Không có tập tin có tên này, nhưng bạn có thể $1.',
'noimage-linktext'               => 'tải tập tin lên',
'uploadnewversion-linktext'      => 'Tải lên phiên bản mới',
'imagepage-searchdupe'           => 'Tìm kiếm các tập tin trùng lắp',

# File reversion
'filerevert'                => 'Lùi lại phiên bản của $1',
'filerevert-legend'         => 'Lùi lại tập tin',
'filerevert-intro'          => "Bạn đang lùi '''[[Media:$1|$1]]''' về [$4 phiên bản lúc $3, $2].",
'filerevert-comment'        => 'Lý do:',
'filerevert-defaultcomment' => 'Đã lùi về phiên bản lúc $2, $1',
'filerevert-submit'         => 'Lùi lại',
'filerevert-success'        => "'''[[Media:$1|$1]]''' đã được lùi về [$4 phiên bản lúc $3, $2].",
'filerevert-badversion'     => 'Không tồn tại phiên bản trước đó của tập tin tại thời điểm trên.',

# File deletion
'filedelete'                  => 'Xóa $1',
'filedelete-legend'           => 'Xóa tập tin',
'filedelete-intro'            => "Bạn đang xóa '''[[Media:$1|$1]]'''.",
'filedelete-intro-old'        => "Bạn đang xóa phiên bản của '''[[Media:$1|$1]]''' vào lúc [$4 $3, $2].",
'filedelete-comment'          => 'Lý do:',
'filedelete-submit'           => 'Xóa',
'filedelete-success'          => "'''$1''' đã bị xóa.",
'filedelete-success-old'      => "Phiên bản của '''[[Media:$1|$1]]''' vào lúc $3, $2 đã bị xóa.",
'filedelete-nofile'           => "'''$1''' không tồn tại trên {{SITENAME}}.",
'filedelete-nofile-old'       => "Không có phiên bản lưu trữ của '''$1''' với các thuộc tính này.",
'filedelete-iscurrent'        => 'Bạn đang cố xóa phiên bản mới nhất của tập tin này. Xin hãy lui tập tin về một phiên bản cũ hơn đã.',
'filedelete-otherreason'      => 'Lý do bổ sung:',
'filedelete-reason-otherlist' => 'Lý do khác',
'filedelete-reason-dropdown'  => '*Những lý do xóa thường gặp
** Vi phạm bản quyền
** Tập tin trùng lắp',
'filedelete-edit-reasonlist'  => 'Sửa lý do xóa',

# MIME search
'mimesearch'         => 'Tìm kiếm theo định dạng',
'mimesearch-summary' => 'Trang này có khả năng lọc tập tin theo định dạng MIME. Đầu vào: contenttype/subtype, v.d. <tt>image/jpeg</tt>.',
'mimetype'           => 'Định dạng MIME:',
'download'           => 'tải xuống',

# Unwatched pages
'unwatchedpages' => 'Trang chưa được theo dõi',

# List redirects
'listredirects' => 'Danh sách trang đổi hướng',

# Unused templates
'unusedtemplates'     => 'Tiêu bản chưa dùng',
'unusedtemplatestext' => 'Đây là danh sách các trang thuộc tên miền không gian Tiêu bản mà chưa được nhúng vào trang khác. Trước khi xóa tiêu bản, hãy nhớ kiểm tra nó được liên kết từ trang khác hay không.',
'unusedtemplateswlh'  => 'liên kết khác',

# Random page
'randompage'         => 'Trang ngẫu nhiên',
'randompage-nopages' => 'Hiện chưa có trang nào trong không gian tên này.',

# Random redirect
'randomredirect'         => 'Trang đổi hướng ngẫu nhiên',
'randomredirect-nopages' => 'Không có trang đổi hướng nào trong không gian này.',

# Statistics
'statistics'             => 'Thống kê',
'sitestats'              => 'Thống kê {{SITENAME}}',
'userstats'              => 'Thống kê thành viên',
'sitestatstext'          => "Hiện có {{PLURAL:$1|'''1''' trang|tổng cộng '''$1''' trang}} trong cơ sở dữ liệu.
Trong số đó có các trang “thảo luận”, trang liên quan đến {{SITENAME}}, các trang “sơ khai” ngắn, và những trang khác không tính là trang có nội dung.
Nếu không tính đến các trang đó, có {{PLURAL:$2|'''1'''|'''$2'''}} trang là những trang có nội dung tốt.

Có '''$8''' tập tin đã được tải lên.

Đã có tổng cộng '''$3''' lần truy cập, và '''$4''' sửa đổi từ khi {{SITENAME}} được khởi tạo. Như vậy trung bình có '''$5''' sửa đổi tại mỗi trang, và '''$6''' lần truy cập trên mỗi sửa đổi.

Độ dài của [http://www.mediawiki.org/wiki/Manual:Job_queue hàng đợi việc] là '''$7'''.",
'userstatstext'          => "Có '''$1''' [[Special:ListUsers|thành viên]] đã đăng ký tài khoản, trong số đó có '''$2''' thành viên (chiếm '''$4%''' trên tổng số) {{PLURAL:$2||}} là $5.",
'statistics-mostpopular' => 'Các trang được xem nhiều nhất',

'disambiguations'      => 'Trang định hướng',
'disambiguationspage'  => 'Template:disambig',
'disambiguations-text' => "Các trang này có liên kết đến một '''trang định hướng'''. Nên sửa các liên kết này để chỉ đến một trang đúng nghĩa hơn.<br />Các trang định hướng là trang sử dụng những tiêu bản được liệt kê ở [[MediaWiki:Disambiguationspage]].",

'doubleredirects'            => 'Đổi hướng kép',
'doubleredirectstext'        => 'Trang này liệt kê các trang chuyển hướng đến một trang chuyển hướng khác. Mỗi hàng có chứa các liên kết đến trang chuyển hướng thứ nhất và thứ hai, cũng như mục tiêu của trang chuyển hướng thứ hai, thường chỉ tới trang đích “thực sự”, là nơi mà trang chuyển hướng đầu tiên nên trỏ đến.',
'double-redirect-fixed-move' => '[[$1]] đã được đổi tên, giờ nó là trang đổi hướng đến [[$2]]',
'double-redirect-fixer'      => 'Người sửa trang đổi hướng',

'brokenredirects'        => 'Đổi hướng sai',
'brokenredirectstext'    => 'Các trang đổi hướng sau đây liên kết đến một trang không tồn tại.',
'brokenredirects-edit'   => '(sửa)',
'brokenredirects-delete' => '(xóa)',

'withoutinterwiki'         => 'Trang chưa có liên kết ngoại ngữ',
'withoutinterwiki-summary' => 'Các trang sau đây không có liên kết đến các phiên bản ngoại ngữ khác:',
'withoutinterwiki-legend'  => 'Tiền tố',
'withoutinterwiki-submit'  => 'Xem',

'fewestrevisions' => 'Trang có ít sửa đổi nhất',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|byte|byte}}',
'ncategories'             => '$1 {{PLURAL:$1|thể loại|thể loại}}',
'nlinks'                  => '$1 {{PLURAL:$1|liên kết|liên kết}}',
'nmembers'                => '$1 {{PLURAL:$1|trang|trang}}',
'nrevisions'              => '$1 {{PLURAL:$1|phiên bản|phiên bản}}',
'nviews'                  => '$1 {{PLURAL:$1|lượt truy cập|lượt truy cập}}',
'specialpage-empty'       => 'Trang này đang trống.',
'lonelypages'             => 'Trang mồ côi',
'lonelypagestext'         => 'Chưa có trang nào liên kết đến các trang này trong {{SITENAME}}.',
'uncategorizedpages'      => 'Trang chưa xếp thể loại',
'uncategorizedcategories' => 'Thể loại chưa phân loại',
'uncategorizedimages'     => 'Tập tin chưa được phân loại',
'uncategorizedtemplates'  => 'Tiêu bản chưa được phân loại',
'unusedcategories'        => 'Thể loại trống',
'unusedimages'            => 'Tập tin chưa dùng',
'popularpages'            => 'Trang nhiều người đọc',
'wantedcategories'        => 'Thể loại cần thiết',
'wantedpages'             => 'Trang cần viết',
'missingfiles'            => 'Tập tin bị thiếu',
'mostlinked'              => 'Trang được liên kết đến nhiều nhất',
'mostlinkedcategories'    => 'Thể loại có nhiều trang nhất',
'mostlinkedtemplates'     => 'Tiêu bản được liên kết đến nhiều nhất',
'mostcategories'          => 'Các trang có nhiều thể loại nhất',
'mostimages'              => 'Tập tin được liên kết đến nhiều nhất',
'mostrevisions'           => 'Các trang được sửa đổi nhiều lần nhất',
'prefixindex'             => 'Các trang trùng với tiền tố',
'shortpages'              => 'Trang ngắn nhất',
'longpages'               => 'Trang dài nhất',
'deadendpages'            => 'Trang đường cùng',
'deadendpagestext'        => 'Các trang này không có liên kết đến trang khác trong {{SITENAME}}.',
'protectedpages'          => 'Trang bị khóa',
'protectedpages-indef'    => 'Chỉ hiển thị khóa vô hạn',
'protectedpagestext'      => 'Các trang này bị khóa không cho sửa đổi hay di chuyển',
'protectedpagesempty'     => 'Hiện không có trang nào bị khóa với các thông số này.',
'protectedtitles'         => 'Các tựa trang được bảo vệ',
'protectedtitlestext'     => 'Các tựa trang sau đây đã bị khóa không cho tạo mới',
'protectedtitlesempty'    => 'Không có tựa trang nào bị khóa với các thông số như vậy.',
'listusers'               => 'Danh sách thành viên',
'newpages'                => 'Các trang mới nhất',
'newpages-username'       => 'Tên người dùng:',
'ancientpages'            => 'Các trang cũ nhất',
'move'                    => 'Di chuyển',
'movethispage'            => 'Di chuyển trang này',
'unusedimagestext'        => 'Xin lưu ý là các trang Web bên ngoài có thể liên kết đến một tập tin ở đây qua một địa chỉ URL trực tiếp, do đó nhiều tập tin vẫn được liệt kê ở đây dù có thể nó đang được sử dụng.',
'unusedcategoriestext'    => 'Các trang thể loại này tồn tại mặc dù không có trang hay tiểu thể loại nào thuộc về nó.',
'notargettitle'           => 'Chưa có mục tiêu',
'notargettext'            => 'Xin chỉ rõ trang hoặc thành viên cần thực hiện tác vụ.',
'nopagetitle'             => 'Không có trang đích nào như vậy',
'nopagetext'              => 'Trang đích bạn chỉ định không tồn tại.',
'pager-newer-n'           => '{{PLURAL:$1|1|$1}} mới hơn',
'pager-older-n'           => '{{PLURAL:$1|1|$1}} cũ hơn',
'suppress'                => 'Giám sát viên',

# Book sources
'booksources'               => 'Nguồn sách',
'booksources-search-legend' => 'Tìm kiếm nguồn sách',
'booksources-go'            => 'Tìm kiếm',
'booksources-text'          => 'Dưới đây là danh sách những trang bán sách mới và cũ, đồng thời có thể có thêm thông tin về những cuốn sách bạn đang tìm:',

# Special:Log
'specialloguserlabel'  => 'Thành viên:',
'speciallogtitlelabel' => 'Tên trang:',
'log'                  => 'Nhật trình',
'all-logs-page'        => 'Tất cả các nhật trình',
'log-search-legend'    => 'Tìm kiếm nhật trình',
'log-search-submit'    => 'Tìm kiếm',
'alllogstext'          => 'Hiển thị tất cả các nhật trình đang có của {{SITENAME}} chung với nhau.
Bạn có thể thu hẹp kết quả bằng cách chọn loại nhật trình, tên thành viên (phân biệt chữ hoa-chữ thường), hoặc các trang bị ảnh hưởng (cũng phân biệt chữ hoa-chữ thường).',
'logempty'             => 'Không có mục nào khớp với từ khóa.',
'log-title-wildcard'   => 'Tìm các tựa trang bắt đầu bằng các chữ này',

# Special:AllPages
'allpages'          => 'Tất cả các trang',
'alphaindexline'    => '$1 đến $2',
'nextpage'          => 'Trang sau ($1)',
'prevpage'          => 'Trang trước ($1)',
'allpagesfrom'      => 'Xem trang từ:',
'allarticles'       => 'Mọi trang',
'allinnamespace'    => 'Mọi trang (không gian $1)',
'allnotinnamespace' => 'Mọi trang (không trong không gian $1)',
'allpagesprev'      => 'Trước',
'allpagesnext'      => 'Sau',
'allpagessubmit'    => 'Hiển thị',
'allpagesprefix'    => 'Hiển thị trang có tiền tố:',
'allpagesbadtitle'  => 'Tựa trang không hợp lệ hay chứa tiền tố liên kết ngôn ngữ hoặc liên kết wiki. Nó có thể chứa một hoặc nhiều ký tự không dùng được ở tựa trang.',
'allpages-bad-ns'   => '{{SITENAME}} không có không gian tên “$1”',

# Special:Categories
'categories'                    => 'Thể loại',
'categoriespagetext'            => 'Các thể loại dưới đây là thể loại có trang hoặc tập tin phương tiện.
[[Special:UnusedCategories|Thể loại trống]] không được hiển thị tại đây.
Xem thêm [[Special:WantedCategories|thể loại cần thiết]].',
'categoriesfrom'                => 'Hiển thị thể loại bằng đầu từ:',
'special-categories-sort-count' => 'xếp theo số trang',
'special-categories-sort-abc'   => 'xếp theo vần',

# Special:ListUsers
'listusersfrom'      => 'Hiển thị thành viên bắt đầu từ:',
'listusers-submit'   => 'Liệt kê',
'listusers-noresult' => 'Không thấy thành viên.',

# Special:ListGroupRights
'listgrouprights'          => 'Nhóm thành viên',
'listgrouprights-summary'  => 'Dưới đây là danh sách nhóm thành viên được định nghĩa tại wiki này, với mức độ truy cập của từng nhóm.
Có [[{{MediaWiki:Listgrouprights-helppage}}|thông tin thêm]] về từng nhóm riêng biệt.',
'listgrouprights-group'    => 'Nhóm',
'listgrouprights-rights'   => 'Khả năng',
'listgrouprights-helppage' => 'Help:Khả năng của nhóm thành viên',
'listgrouprights-members'  => '(danh sách thành viên)',

# E-mail user
'mailnologin'     => 'Không có địa chỉ gửi thư',
'mailnologintext' => 'Bạn phải [[Special:UserLogin|đăng nhập]] và khai báo một địa chỉ thư điện tử hợp lệ trong phần [[Special:Preferences|tùy chọn cá nhân]] thì mới gửi được thư cho người khác.',
'emailuser'       => 'Gửi thư cho người này',
'emailpage'       => 'Gửi thư',
'emailpagetext'   => 'Nếu người dùng này đã cung cấp địa chỉ thư điện tử hợp lệ tại tùy chọn cá nhân, mẫu dưới đây sẽ gửi một bức thư điện tử tới người đó.
Địa chỉ thư điện tử mà bạn đã cung cấp trong [[Special:Preferences|tùy chọn cá nhân của mình]] sẽ xuất hiện trong phần địa chỉ “Người gửi” của bức thư, do đó người nhận sẽ có thể trả lời trực tiếp cho bạn.',
'usermailererror' => 'Lỗi gửi thư:',
'defemailsubject' => 'thư gửi từ {{SITENAME}}',
'noemailtitle'    => 'Không có địa chỉ nhận thư',
'noemailtext'     => 'Người này không cung cấp một địa chỉ thư hợp lệ, hoặc đã chọn không nhận thư từ người khác.',
'emailfrom'       => 'Người gửi:',
'emailto'         => 'Người nhận:',
'emailsubject'    => 'Chủ đề:',
'emailmessage'    => 'Nội dung:',
'emailsend'       => 'Gửi',
'emailccme'       => 'Gửi cho tôi bản sao của thư này.',
'emailccsubject'  => 'Bản sao của thư gửi cho $1: $2',
'emailsent'       => 'Đã gửi',
'emailsenttext'   => 'Thư của bạn đã được gửi.',
'emailuserfooter' => 'Thư điện tử này được $1 gửi đến $2 thông qua chức năng “Gửi thư cho người này” của {{SITENAME}}.',

# Watchlist
'watchlist'            => 'Trang tôi theo dõi',
'mywatchlist'          => 'Trang tôi theo dõi',
'watchlistfor'         => "(của '''$1''')",
'nowatchlist'          => 'Danh sách theo dõi của bạn không có gì.',
'watchlistanontext'    => 'Xin hãy $1 để xem hay sửa đổi các trang được theo dõi.',
'watchnologin'         => 'Chưa đăng nhập',
'watchnologintext'     => 'Bạn phải [[Special:UserLogin|đăng nhập]] mới sửa đổi được danh sách theo dõi.',
'addedwatch'           => 'Đã thêm vào danh sách theo dõi',
'addedwatchtext'       => 'Trang “<nowiki>$1</nowiki>” đã được cho vào [[Special:Watchlist|danh sách theo dõi]]. Những sửa đổi đối với trang này và trang thảo luận của nó sẽ được liệt kê, và được <b>tô đậm</b> trong [[Special:RecentChanges|danh sách các thay đổi mới]].

Nếu bạn muốn cho trang này ra khỏi danh sách theo dõi, nhấn vào "Ngừng theo dõi" ở trên.',
'removedwatch'         => 'Đã ra khỏi danh sách theo dõi',
'removedwatchtext'     => 'Trang “[[:$1]]” đã được đưa ra khỏi danh sách theo dõi.',
'watch'                => 'Theo dõi',
'watchthispage'        => 'Theo dõi trang này',
'unwatch'              => 'Ngừng theo dõi',
'unwatchthispage'      => 'Ngừng theo dõi',
'notanarticle'         => 'Không phải trang có nội dung',
'notvisiblerev'        => 'Phiên bản bị xóa',
'watchnochange'        => 'Không có trang nào bạn theo dõi được sửa đổi.',
'watchlist-details'    => 'Bạn đang theo dõi {{PLURAL:$1|$1 trang|$1 trang}}, không kể các trang thảo luận.',
'wlheader-enotif'      => '* Đã bật thông báo qua thư điện tử.',
'wlheader-showupdated' => "* Các trang đã thay đổi từ lần cuối bạn xem chúng được in '''đậm'''",
'watchmethod-recent'   => 'Dưới đây hiện thay đổi mới với các trang theo dõi.',
'watchmethod-list'     => 'Dưới đây hiện danh sách các trang theo dõi.',
'watchlistcontains'    => 'Danh sách theo dõi của bạn có $1 {{PLURAL:$1|trang|trang}}.',
'iteminvalidname'      => 'Tên trang “$1” không hợp lệ…',
'wlnote'               => "Dưới đây là {{PLURAL:$1|sửa đổi cuối cùng|'''$1''' sửa đổi mới nhất}} trong '''$2''' giờ qua.",
'wlshowlast'           => 'Hiển thị $1 giờ $2 ngày gần đây $3',
'watchlist-show-bots'  => 'Hiện sửa đổi bot',
'watchlist-hide-bots'  => 'Ẩn sửa đổi bot',
'watchlist-show-own'   => 'Hiện sửa đổi của tôi',
'watchlist-hide-own'   => 'Ẩn sửa đổi của tôi',
'watchlist-show-minor' => 'Hiện sửa đổi nhỏ',
'watchlist-hide-minor' => 'Ẩn sửa đổi nhỏ',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Đang theo dõi…',
'unwatching' => 'Đang ngừng theo dõi…',

'enotif_mailer'                => 'Thông báo của {{SITENAME}}',
'enotif_reset'                 => 'Đánh dấu đã xem mọi trang',
'enotif_newpagetext'           => 'Trang này mới',
'enotif_impersonal_salutation' => 'thành viên {{SITENAME}}',
'changed'                      => 'đã sửa',
'created'                      => 'đã viết mới',
'enotif_subject'               => '$PAGETITLE tại {{SITENAME}} đã được $CHANGEDORCREATED bởi $PAGEEDITOR',
'enotif_lastvisited'           => 'Xem $1 để biết các thay đổi diễn ra từ lần xem cuối cùng của bạn.',
'enotif_lastdiff'              => 'Vào $1 để xem sự thay đổi này.',
'enotif_anon_editor'           => 'thành viên vô danh $1',
'enotif_body'                  => '$WATCHINGUSERNAME thân mến,


Trang $PAGETITLE tại {{SITENAME}} đã được $PAGEEDITOR $CHANGEDORCREATED vào $PAGEEDITDATE, xem phiên bản hiện hành tại $PAGETITLE_URL.

$NEWPAGE

Tóm lược sửa đổi: $PAGESUMMARY $PAGEMINOREDIT

Liên lạc với người viết trang qua:
thư: $PAGEEDITOR_EMAIL
wiki: $PAGEEDITOR_WIKI

Sẽ không có thông báo nào khác nếu có sự thay đổi tiếp theo trừ khi bạn xem trang đó. Bạn cũng có thể thiết lập lại việc nhắc nhở cho tất cả các trang nằm trong danh sách theo dõi của bạn.

              Hệ thống báo tin {{SITENAME}} thân thiện của bạn

--
Để thay đổi các thiết lập danh sách theo dõi, mời xem
{{fullurl:{{ns:special}}:Watchlist/edit}}

Phản hồi và cần sự hỗ trợ:
{{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => 'Xóa trang',
'confirm'                     => 'Xác nhận',
'excontent'                   => 'nội dung cũ: “$1”',
'excontentauthor'             => 'nội dung cũ: “$1” (người viết duy nhất “[[Special:Contributions/$2|$2]]”)',
'exbeforeblank'               => 'nội dung trước khi tẩy trống: “$1”',
'exblank'                     => 'trang trắng',
'delete-confirm'              => '

Xóa “$1”',
'delete-legend'               => 'Xóa',
'historywarning'              => 'Cảnh báo: Trang bạn sắp xóa đã có lịch sử:',
'confirmdeletetext'           => 'Bạn sắp xóa hẳn một trang cùng với tất cả lịch sử của nó.
Xin xác nhận việc bạn định làm, và hiểu rõ những hệ lụy của nó, và bạn thực hiện nó theo đúng đúng [[{{MediaWiki:Policy-url}}|quy định]].',
'actioncomplete'              => 'Đã thực hiện xong',
'deletedtext'                 => 'Đã xóa “<nowiki>$1</nowiki>”. Xem danh sách các xóa bỏ gần nhất tại $2.',
'deletedarticle'              => 'đã xóa “$1”',
'suppressedarticle'           => 'đã giấu "[[$1]]"',
'dellogpage'                  => 'Nhật trình xóa',
'dellogpagetext'              => 'Dưới đây là danh sách các trang bị xóa gần đây nhất.',
'deletionlog'                 => 'nhật trình xóa',
'reverted'                    => 'Đã hồi phục một phiên bản cũ',
'deletecomment'               => 'Lý do',
'deleteotherreason'           => 'Lý do khác/bổ sung:',
'deletereasonotherlist'       => 'Lý do khác',
'deletereason-dropdown'       => '*Các lý do xóa phổ biến
** Tác giả yêu cầu
** Vi phạm bản quyền
** Phá hoại',
'delete-edit-reasonlist'      => 'Sửa lý do xóa',
'delete-toobig'               => 'Trang này có lịch sử sửa đổi lớn, đến hơn {{PLURAL:$1|lần|lần}} sửa đổi.
Việc xóa các trang như vậy bị hạn chế để ngăn ngừa phá hoại do vô ý cho {{SITENAME}}.',
'delete-warning-toobig'       => 'Trang này có lịch sử sửa đổi lớn, đến hơn {{PLURAL:$1|lần|lần}} sửa đổi.
Việc xóa các trang có thể làm tổn hại đến hoạt động của cơ sở dữ liệu {{SITENAME}};
hãy cẩn trọng khi thực hiện.',
'rollback'                    => 'Lùi tất cả sửa đổi',
'rollback_short'              => 'Lùi tất cả',
'rollbacklink'                => 'lùi tất cả',
'rollbackfailed'              => 'Lùi sửa đổi không thành công',
'cantrollback'                => 'Không lùi sửa đổi được;
người viết trang cuối cùng cũng là tác giả duy nhất của trang này.',
'alreadyrolled'               => 'Không thể lùi tất cả sửa đổi cuối của [[User:$2|$2]] ([[User talk:$2|thảo luận]] | [[Special:Contributions/$2|{{int:contribslink}}]]) tại [[:$1]]; ai đó đã thực hiện sửa đổi hoặc thực hiện lùi tất cả rồi.

Sửa đổi cuối cùng tại trang do [[User:$3|$3]] ([[User talk:$3|thảo luận]] | [[Special:Contributions/$3|{{int:contribslink}}]]) thực hiện.',
'editcomment'                 => 'Tóm lược sửa đổi: “<i>$1</i>”.', # only shown if there is an edit comment
'revertpage'                  => 'Đã hủy sửa đổi của [[Special:Contributions/$2|$2]] ([[User talk:$2|Thảo luận]]) quay về phiên bản của [[User:$1|$1]]', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => 'Đã hủy sửa đổi của $1;
quay về phiên bản cuối của $2.',
'sessionfailure'              => 'Dường như có trục trặc với phiên đăng nhập của bạn; thao tác này đã bị hủy để tránh việc cướp quyền đăng nhập. Xin hãy nhấn nút “Back”, tải lại trang đó, rồi thử lại.',
'protectlogpage'              => 'Nhật trình khóa',
'protectlogtext'              => 'Dưới đây là danh sách các thao tác khóa và mở khóa trang. Xem [[Special:ProtectedPages|danh sách các trang bị khóa]] để xem danh sách các trang hiện thời đang bị khóa.',
'protectedarticle'            => 'đã khóa “[[$1]]”',
'modifiedarticleprotection'   => 'đã đổi mức khóa cho “[[$1]]”',
'unprotectedarticle'          => 'đã mở khóa cho “[[$1]]”',
'protect-title'               => 'Thiết lập mức khóa cho “$1”',
'protect-legend'              => 'Xác nhận khóa',
'protectcomment'              => 'Lý do:',
'protectexpiry'               => 'Thời hạn:',
'protect_expiry_invalid'      => 'Thời hạn không hợp lệ.',
'protect_expiry_old'          => 'Thời hạn đã qua.',
'protect-unchain'             => 'Thay đổi mức cấm di chuyển',
'protect-text'                => 'Bạn có thể xem và đổi kiểu khóa trang <strong><nowiki>$1</nowiki></strong> ở đây.',
'protect-locked-blocked'      => 'Bạn không thể đổi mức khóa khi bị cấm. Đây là trạng thái
hiện tại của trang <strong>$1</strong>:',
'protect-locked-dblock'       => 'Hiện không thể đổi mức khóa do cơ sở dữ liệu bị khóa.
Đây là trạng thái hiện tại của trang <strong>$1</strong>:',
'protect-locked-access'       => 'Tài khoản của bạn không được cấp quyền đổi mức khóa của trang.
Đây là trạng thái hiện tại của trang <strong>$1</strong>:',
'protect-cascadeon'           => 'Trang này hiện bị khóa vì nó được nhúng vào {{PLURAL:$1|những trang|trang}} dưới đây bị khóa với tùy chọn “khóa theo tầng” được kích hoạt. Bạn có thể đổi mức độ khóa của trang này, nhưng nó sẽ không ảnh hưởng đến việc khóa theo tầng.',
'protect-default'             => '(mặc định)',
'protect-fallback'            => 'Cần quyền “$1”',
'protect-level-autoconfirmed' => 'Cấm thành viên chưa đăng ký',
'protect-level-sysop'         => 'Cấm mọi thành viên (trừ quản lý)',
'protect-summary-cascade'     => 'khóa theo tầng',
'protect-expiring'            => 'hết hạn $1 (UTC)',
'protect-cascade'             => 'Tự động khóa các trang được nhúng vào trang này (khóa theo tầng)',
'protect-cantedit'            => 'Bạn không thể thay đổi mức khóa cho trang này do không có đủ quyền hạn.',
'restriction-type'            => 'Quyền:',
'restriction-level'           => 'Mức độ hạn chế:',
'minimum-size'                => 'Kích thước tối thiểu',
'maximum-size'                => 'Kích thước tối đa:',
'pagesize'                    => '(byte)',

# Restrictions (nouns)
'restriction-edit'   => 'Sửa đổi',
'restriction-move'   => 'Di chuyển',
'restriction-create' => 'Tạo mới',
'restriction-upload' => 'Tải lên',

# Restriction levels
'restriction-level-sysop'         => 'khóa hẳn',
'restriction-level-autoconfirmed' => 'hạn chế sửa đổi',
'restriction-level-all'           => 'mọi mức độ',

# Undelete
'undelete'                     => 'Xem các trang bị xóa',
'undeletepage'                 => 'Xem và phục hồi trang bị xóa',
'undeletepagetitle'            => "'''Sau đây là những phiên bản đã bị xóa của [[:$1]].'''",
'viewdeletedpage'              => 'Xem các trang bị xóa',
'undeletepagetext'             => 'Các trang sau đã bị xóa nhưng vẫn nằm trong kho lưu trữ và có thể phục hồi được. Kho lưu trữ sẽ được khóa định kỳ.',
'undelete-fieldset-title'      => 'Phục hồi phiên bản',
'undeleteextrahelp'            => "Để phục hồi toàn bộ lịch sử trang, hãy để trống các hộp kiểm và bấm nút '''''Phục hồi'''''.
Để thực hiện phục hồi có chọn lọc, hãy đánh dấu vào hộp kiểm của các phiên bản muốn phục hồi, rồi bấm nút '''''Phục hồi'''''.
Bấm nút '''''Tẩy trống''''' sẽ tẩy trống ô lý do và tất cả các hộp kiểm.",
'undeleterevisions'            => '$1 {{PLURAL:$1|bản|bản}} đã được lưu',
'undeletehistory'              => 'Nếu bạn phục hồi trang này, tất cả các phiên bản của nó cũng sẽ được phục hồi vào lịch sử của trang.
Nếu một trang mới có cùng tên đã được tạo ra kể từ khi xóa trang này, các phiên bản được khôi phục sẽ xuất hiện trong lịch sử trước.',
'undeleterevdel'               => 'Việc phục hồi sẽ không được thực hiện nếu nó dẫn đến việc phiên bản trang hoặc tập tin trên cùng bị xóa mất một phần.
Trong trường hợp đó, bạn phải bỏ đánh dấu hộp kiểm hoặc bỏ ẩn những phiên bản bị xóa mới nhất.',
'undeletehistorynoadmin'       => 'Trang này đã bị xóa.
Lý do xóa trang được hiển thị dưới đây, cùng với thông tin về các người đã sửa đổi trang này trước khi bị xóa.
Chỉ có người quản lý mới xem được văn bản đầy đủ của những phiên bản trang bị xóa.',
'undelete-revision'            => 'Phiên bản của $1 do $3 xóa (vào lúc $2):',
'undeleterevision-missing'     => 'Phiên bản này không hợp lệ hay không tồn tại. Đây có thể là một địa chỉ sai, hoặc là phiên bản đã được phục hồi hoặc đã xóa khỏi kho lưu trữ.',
'undelete-nodiff'              => 'Không tìm thấy phiên bản cũ hơn.',
'undeletebtn'                  => 'Phục hồi',
'undeletelink'                 => 'phục hồi',
'undeletereset'                => 'Tẩy trống',
'undeletecomment'              => 'Lý do:',
'undeletedarticle'             => 'đã phục hồi “$1”',
'undeletedrevisions'           => '$1 {{PLURAL:$1|bản|bản}} được phục hồi',
'undeletedrevisions-files'     => '$1 {{PLURAL:$1|bản|bản}} và $2 {{PLURAL:$2|tập tin|tập tin}} đã được phục hồi',
'undeletedfiles'               => '$1 {{PLURAL:$1|tập tin|tập tin}} đã được phục hồi',
'cannotundelete'               => 'Phục hồi thất bại;
một người nào khác đã phục hồi trang này rồi.',
'undeletedpage'                => "<big>'''$1 đã được khôi phục'''</big>

Xem nhật trình xóa và phục hồi các trang gần đây tại [[Special:Log/delete|nhật trình xóa]].",
'undelete-header'              => 'Xem các trang bị xóa gần đây tại [[Special:Log/delete|nhật trình xóa]].',
'undelete-search-box'          => 'Tìm kiếm trang đã bị xóa',
'undelete-search-prefix'       => 'Hiển thị trang có tiền tố:',
'undelete-search-submit'       => 'Tìm kiếm',
'undelete-no-results'          => 'Không tìm thấy trang đã bị xóa nào khớp với từ khóa.',
'undelete-filename-mismatch'   => 'Không thể phục hồi phiên bản tập tin vào thời điểm $1: không có tập tin trùng tên',
'undelete-bad-store-key'       => 'Không thể phục hồi phiên bản tập tin tại thời điểm $1: tập tin không tồn tại trước khi xóa.',
'undelete-cleanup-error'       => 'Có lỗi khi xóa các tập tin lưu trữ “$1” không được sử dụng.',
'undelete-missing-filearchive' => 'Không thể phục hồi bộ tập tin có định danh $1 vì nó không nằm ở cơ sở dữ liệu. Có thể nó được phục hồi rồi.',
'undelete-error-short'         => 'Có lỗi khi phục hồi tập tin: $1',
'undelete-error-long'          => 'Xuất hiện lỗi khi phục hồi tập tin:

$1',

# Namespace form on various pages
'namespace'      => 'Không gian:',
'invert'         => 'Đảo ngược lựa chọn',
'blanknamespace' => '(Chính)',

# Contributions
'contributions' => 'Đóng góp của thành viên',
'mycontris'     => 'Đóng góp của tôi',
'contribsub2'   => 'Của $1 ($2)',
'nocontribs'    => 'Không tìm thấy thay đổi nào khớp với yêu cầu.',
'uctop'         => '(mới nhất)',
'month'         => 'Từ tháng (trở về trước):',
'year'          => 'Từ năm (trở về trước):',

'sp-contributions-newbies'     => 'Chỉ hiển thị đóng góp của tài khoản mới',
'sp-contributions-newbies-sub' => 'Các thành viên mới',
'sp-contributions-blocklog'    => 'Nhật trình cấm',
'sp-contributions-search'      => 'Tìm kiếm đóng góp',
'sp-contributions-username'    => 'Địa chỉ IP hay tên thành viên:',
'sp-contributions-submit'      => 'Tìm kiếm',

# What links here
'whatlinkshere'            => 'Các liên kết đến đây',
'whatlinkshere-title'      => 'Các trang liên kết đến “$1”',
'whatlinkshere-page'       => 'Trang:',
'linklistsub'              => '(Danh sách liên kết)',
'linkshere'                => "Các trang sau liên kết đến '''[[:$1]]''':",
'nolinkshere'              => "Không có trang nào liên kết đến '''[[:$1]]'''.",
'nolinkshere-ns'           => "Không có trang nào liên kết đến '''[[:$1]]''' trong không gian tên đã chọn.",
'isredirect'               => 'trang đổi hướng',
'istemplate'               => 'được nhúng vào',
'isimage'                  => 'liên kết hình',
'whatlinkshere-prev'       => '{{PLURAL:$1|kết quả trước|$1 kết quả trước}}',
'whatlinkshere-next'       => '{{PLURAL:$1|kết quả sau|$1 kết quả sau}}',
'whatlinkshere-links'      => '← liên kết',
'whatlinkshere-hideredirs' => '$1 trang đổi hướng',
'whatlinkshere-hidetrans'  => '$1 trang nhúng',
'whatlinkshere-hidelinks'  => '$1 liên kết',
'whatlinkshere-hideimages' => '$1 liên kết hình',
'whatlinkshere-filters'    => 'Bộ lọc',

# Block/unblock
'blockip'                         => 'Cấm thành viên',
'blockip-legend'                  => 'Cấm thành viên',
'blockiptext'                     => 'Dùng mẫu dưới để cấm một địa chỉ IP hoặc thành viên không được viết trang.
Điều này chỉ nên làm để tránh phá hoại, và phải theo [[{{MediaWiki:Policy-url}}|quy định]].
Điền vào lý do cụ thể ở dưới (ví dụ, chỉ ra trang nào bị phá hoại).',
'ipaddress'                       => 'Địa chỉ IP:',
'ipadressorusername'              => 'Địa chỉ IP hay tên thành viên:',
'ipbexpiry'                       => 'Thời hạn:',
'ipbreason'                       => 'Lý do:',
'ipbreasonotherlist'              => 'Lý do khác',
'ipbreason-dropdown'              => '*Một số lý do cấm thường gặp
** Phá hoại
** Thêm thông tin sai lệch
** Xóa nội dung trang
** Gửi liên kết spam đến trang web bên ngoài
** Cho thông tin rác vào trang
** Có thái độ dọa dẫm/quấy rối
** Dùng nhiều tài khoản
** Tên thành viên không được chấp nhận
** Tạo nhiều trang mới vi phạm bản quyền, bỏ qua thảo luận và cảnh báo
** Truyền nhiều hình ảnh thiếu nguồn gốc hoặc bản quyền
** Con rối của thành viên bị cấm',
'ipbanononly'                     => 'Chỉ cấm thành viên vô danh',
'ipbcreateaccount'                => 'Cấm mở tài khoản',
'ipbemailban'                     => 'Không cho gửi email',
'ipbenableautoblock'              => 'Tự động cấm các địa chỉ IP mà thành viên này sử dụng',
'ipbsubmit'                       => 'Cấm',
'ipbother'                        => 'Thời hạn khác:',
'ipboptions'                      => '2 giờ:2 hours,1 ngày:1 day,3 ngày:3 days,1 tuần:1 week,2 tuần:2 weeks,1 tháng:1 month,3 tháng:3 months,6 tháng:6 months,1 năm:1 year,vô hạn:infinite', # display1:time1,display2:time2,...
'ipbotheroption'                  => 'khác',
'ipbotherreason'                  => 'Lý do khác',
'ipbhidename'                     => 'Ẩn tên người dùng khỏi nhật trình cấm, danh sách cấm và danh sách thành viên hiện tại',
'ipbwatchuser'                    => 'Theo dõi trang thành viên và thảo luận thành viên của thành viên này',
'badipaddress'                    => 'Địa chỉ IP không hợp lệ',
'blockipsuccesssub'               => 'Cấm thành công',
'blockipsuccesstext'              => '[[Special:Contributions/$1|$1]] đã bị cấm.
<br />Xem lại những lần cấm tại [[Special:IPBlockList|danh sách cấm]].',
'ipb-edit-dropdown'               => 'Sửa đổi lý do cấm',
'ipb-unblock-addr'                => 'Bỏ cấm $1',
'ipb-unblock'                     => 'Bỏ cấm thành viên hay địa chỉ IP',
'ipb-blocklist-addr'              => 'Xem $1 đang bị cấm hay không',
'ipb-blocklist'                   => 'Xem danh sách đang bị cấm',
'unblockip'                       => 'Bỏ cấm thành viên',
'unblockiptext'                   => 'Sử dụng mẫu sau để phục hồi lại quyền sửa đổi đối với một địa chỉ IP hoặc tên thành viên đã bị cấm trước đó.',
'ipusubmit'                       => 'Bỏ cấm',
'unblocked'                       => '[[User:$1|$1]] đã hết bị cấm',
'unblocked-id'                    => '$1 đã hết bị cấm',
'ipblocklist'                     => 'Địa chỉ IP và tên người dùng bị cấm',
'ipblocklist-legend'              => 'Tìm một thành viên bị cấm',
'ipblocklist-username'            => 'Tên thành viên hoặc địa chỉ IP:',
'ipblocklist-submit'              => 'Tìm kiếm',
'blocklistline'                   => '$1, $2 đã cấm $3 (hết hạn $4)',
'infiniteblock'                   => 'vô hạn',
'expiringblock'                   => 'hết hạn lúc $1',
'anononlyblock'                   => 'chỉ cấm vô danh',
'noautoblockblock'                => 'đã tắt chức năng tự động cấm',
'createaccountblock'              => 'không được mở tài khoản',
'emailblock'                      => 'đã cấm thư điện tử',
'ipblocklist-empty'               => 'Danh sách cấm hiện đang trống.',
'ipblocklist-no-results'          => 'Địa chỉ IP hoặc tên thành viên này hiện không bị cấm.',
'blocklink'                       => 'cấm',
'unblocklink'                     => 'bỏ cấm',
'contribslink'                    => 'đóng góp',
'autoblocker'                     => 'Bạn bị tự động cấm vì địa chỉ IP của bạn vừa rồi đã được “$1” sử dụng. Lý do đưa ra cho việc cấm $1 là: ”$2”',
'blocklogpage'                    => 'Nhật trình cấm',
'blocklogentry'                   => 'đã cấm [[$1]] với thời hạn là $2 $3',
'blocklogtext'                    => 'Đây là nhật trình ghi lại những lần cấm và bỏ cấm. Các địa chỉ IP bị cấm tự động không được liệt kê ở đây. Xem thêm [[Special:IPBlockList|danh sách cấm]] để có danh sách cấm và cấm hẳn hiện tại.',
'unblocklogentry'                 => 'đã bỏ cấm “$1”',
'block-log-flags-anononly'        => 'chỉ cấm thành viên vô danh',
'block-log-flags-nocreate'        => 'cấm mở tài khoản',
'block-log-flags-noautoblock'     => 'tắt tự động cấm',
'block-log-flags-noemail'         => 'cấm thư điện tử',
'block-log-flags-angry-autoblock' => 'bật tự động cấm nâng cao',
'range_block_disabled'            => 'Đã tắt khả năng cấm hàng loạt của quản lý.',
'ipb_expiry_invalid'              => 'Thời điểm hết hạn không hợp lệ.',
'ipb_expiry_temp'                 => 'Cấm tên người dùng ẩn nên là cấm vô hạn.',
'ipb_already_blocked'             => '“$1” đã bị cấm rồi',
'ipb_cant_unblock'                => 'Lỗi: Không tìm được ID cấm $1. Địa chỉ IP này có thể đã được bỏ cấm.',
'ipb_blocked_as_range'            => 'Lỗi: Địa chỉ IP $1 không bị cấm trực tiếp và do đó không thể bỏ cấm. Tuy nhiên, nó bị cấm do là một bộ phận của dải IP $2, bạn có thể bỏ cấm dải này.',
'ip_range_invalid'                => 'Dải IP không hợp lệ.',
'blockme'                         => 'Cấm tôi',
'proxyblocker'                    => 'Chặn proxy',
'proxyblocker-disabled'           => 'Chức năng này đã bị tắt.',
'proxyblockreason'                => 'Địa chỉ IP của bạn đã bị cấm vì là proxy mở. Xin hãy liên hệ nhà cung cấp dịch vụ Internet hoặc bộ phận hỗ trợ kỹ thuật của bạn và thông báo với họ về vấn đề an ninh nghiêm trọng này.',
'proxyblocksuccess'               => 'Xong.',
'sorbsreason'                     => 'Địa chỉ IP của bạn bị liệt kê là một proxy mở trong DNSBL mà {{SITENAME}} đang sử dụng.',
'sorbs_create_account_reason'     => 'Địa chỉ chỉ IP của bạn bị liệt kê là một proxy mở trong DNSBL mà {{SITENAME}} đang sử dụng. Bạn không thể mở tài khoản.',

# Developer tools
'lockdb'              => 'Khóa cơ sở dữ liệu',
'unlockdb'            => 'Mở khóa cơ sở dữ liệu',
'lockdbtext'          => 'Khóa cơ sở dữ liệu sẽ ngưng tất cả khả năngsửa đổi các trang, thay đổi tùy chọn cá nhân, sửa danh sách theo dõi, và những thao tác khác của thành viên đòi hỏi phải thay đổi trong cơ sở dữ liệu.
Xin hãy xác nhận những việc bạn định làm, và rằng bạn sẽ mở khóa cơ sở dữ liệu khi xong công việc bảo trì của bạn.',
'unlockdbtext'        => 'Mở khóa cơ sở dữ liệu sẽ khôi phục lại tất cả khả năng sửa đổi trang, thay đổi tùy chọn cá nhân, sửa đổi danh sách theo dõi, 
và nhiều thao tác khác của thành viên đòi hỏi phải có thay đổi trong cơ sở dữ liệu.
Xin hãy xác nhận đây là điều bạn định làm.',
'lockconfirm'         => 'Vâng, tôi thực sự muốn khóa cơ sở dữ liệu.',
'unlockconfirm'       => 'Vâng, tôi thực sự muốn mở khóa cơ sở dữ liệu.',
'lockbtn'             => 'Khóa cơ sở dữ liệu',
'unlockbtn'           => 'Mở khóa cơ sở dữ liệu',
'locknoconfirm'       => 'Bạn đã không đánh vào ô xác nhận.',
'lockdbsuccesssub'    => 'Đã khóa cơ sở dữ liệu thành công.',
'unlockdbsuccesssub'  => 'Đã mở khóa cơ sở dữ liệu thành công',
'lockdbsuccesstext'   => 'Cơ sở dữ liệu đã bị khóa.
<br />Nhớ [[Special:UnlockDB|mở khóa]] sau khi bảo trì xong.',
'unlockdbsuccesstext' => 'Cơ sở dữ liệu đã được mở khóa.',
'lockfilenotwritable' => 'Tập tin khóa của cơ sở dữ liệu không cho phép ghi. Để khóa hay mở khóa cơ sở dữ liệu, máy chủ web phải có khả năng ghi tập tin.',
'databasenotlocked'   => 'Cơ sở dữ liệu không bị khóa.',

# Move page
'move-page'               => 'Di chuyển $1',
'move-page-legend'        => 'Di chuyển trang',
'movepagetext'            => "Dùng mẫu dưới đây để đổi tên một trang, di chuyển tất cả lịch sử của nó sang tên mới.
Tên cũ sẽ trở thành trang đổi hướng sang tên mới.
Bạn có thể cập nhật tự động các trang đổi hướng đến tên cũ.
Nếu bạn chọn không cập nhật, hãy nhớ kiểm tra [[Special:DoubleRedirects|đổi hướng kép]] hoặc [[Special:BrokenRedirects|đổi hướng đến trang không tồn tại]].
Bạn phải chịu trách nhiệm đảm bảo các liên kết đó tiếp tục trỏ đến nơi chúng cần đến.

Chú ý rằng trang sẽ '''không''' bị di chuyển nếu đã có một trang tại tên mới, trừ khi nó rỗng hoặc là trang đổi hướng và không có lịch sử sửa đổi trước đây.
Điều này có nghĩa là bạn có thể đổi tên trang lại như cũ nếu bạn có nhầm lẫn, và bạn không thể ghi đè lên một trang đã có sẵn.

'''CẢNH BÁO!'''
Việc làm này có thể dẫn đến sự thay đổi mạnh mẽ và không lường trước đối với các trang dễ nhìn thấy;
xin hãy chắc chắn rằng bạn đã nhận thức được những hệ lụy của nó trước khi thực hiện.",
'movepagetalktext'        => "Trang thảo luận đi kèm sẽ được tự động di chuyển theo '''trừ khi''':
*Đã tồn tại một trang thảo luận không trống tại tên mới, hoặc
*Bạn không đánh vào ô bên dưới.

Trong những trường hợp đó, bạn phải di chuyển hoặc hợp nhất trang theo kiểu thủ công nếu muốn.",
'movearticle'             => 'Di chuyển trang:',
'movenotallowed'          => 'Bạn không có quyền di chuyển trang trong {{SITENAME}}.',
'newtitle'                => 'Tên mới',
'move-watch'              => 'Theo dõi trang này',
'movepagebtn'             => 'Di chuyển trang',
'pagemovedsub'            => 'Di chuyển thành công',
'movepage-moved'          => "<big>'''“$1” đã được di chuyển đến “$2”'''</big>", # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => 'Đã có một trang với tên đó, hoặc tên bạn chọn không hợp lệ.
Xin hãy chọn tên khác.',
'cantmove-titleprotected' => 'Bạn không thể đổi tên trang, vì tên trang mới đã bị khóa không cho tạo mới',
'talkexists'              => "'''Trang được di chuyển thành công, nhưng trang thảo luận không thể di chuyển được vì đã tồn tại một trang thảo luận ở tên mới. Xin hãy hợp nhất chúng lại một cách thủ công.'''",
'movedto'                 => 'đổi thành',
'movetalk'                => 'Di chuyển trang thảo luận đi kèm',
'move-subpages'           => 'Di chuyển tất cả các trang con, nếu được',
'move-talk-subpages'      => 'Di chuyển tất cả các trang con của trang thảo luận, nếu được',
'movepage-page-exists'    => 'Trang $1 đã tồn tại và không thể bị tự động ghi đè.',
'movepage-page-moved'     => 'Trang $1 đã được di chuyển đến $2.',
'movepage-page-unmoved'   => 'Trang $1 không thể di chuyển đến $2.',
'movepage-max-pages'      => 'Đã có tối đa $1 {{PLURAL:$1|trang|trang}} đã di chuyển và không tự động di chuyển thêm được nữa.',
'1movedto2'               => '[[$1]] đổi thành [[$2]]',
'1movedto2_redir'         => '[[$1]] đổi thành [[$2]] qua đổi hướng',
'movelogpage'             => 'Nhật trình di chuyển',
'movelogpagetext'         => 'Dưới đây là danh sách các trang đã được di chuyển.',
'movereason'              => 'Lý do:',
'revertmove'              => 'lùi lại',
'delete_and_move'         => 'Xóa và đổi tên',
'delete_and_move_text'    => '==Cần xóa==

Trang với tên “[[:$1]]” đã tồn tại. Bạn có muốn xóa nó để dọn chỗ di chuyển tới tên này không?',
'delete_and_move_confirm' => 'Xóa trang để đổi tên',
'delete_and_move_reason'  => 'Xóa để có chỗ đổi tên',
'selfmove'                => 'Tên mới giống tên cũ; không đổi tên một trang thành chính nó.',
'immobile_namespace'      => 'Tên mới hoặc tên cũ là loạiđặc biệt; không thể di chuyển từ/đến không gian tên đó.',
'imagenocrossnamespace'   => 'Không được di chuyển tập tin ra khỏi không gian tên Tập tin',
'imagetypemismatch'       => 'Phần mở rộng trong tên tập tin mới không hợp dạng của tập tin',
'imageinvalidfilename'    => 'Tên tập tin đích không hợp lệ',
'fix-double-redirects'    => 'Cập nhật tất cả các trang đổi hướng chỉ đến tựa đề cũ',

# Export
'export'            => 'Xuất các trang',
'exporttext'        => 'Bạn có thể xuất nội dung và lịch sử sửa đổi của một trang hoặc tập hợp trang vào tập tin XML.
Những tập tin này cũng có thể được nhập vào wiki khác có sử dụng MediaWiki thông qua [[Special:Import|nhập trang]].

Để xuất các trang, nhập vào tên trang trong hộp soạn thảo ở dưới, mỗi dòng một tên, và lựa chọn bạn muốn phiên bản hiện tại cũng như tất cả phiên bản cũ, với các dòng lịch sử trang, hay chỉ là phiên bản hiện tại với thông tin về lần sửa đổi cuối.

Trong trường hợp sau bạn cũng có thể dùng một liên kết, ví dụ [[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]] để biểu thị trang “[[{{MediaWiki:Mainpage}}]]”.',
'exportcuronly'     => 'Chỉ xuất phiên bản hiện hành, không xuất tất cả lịch sử trang',
'exportnohistory'   => "----
'''Chú ý:''' Chức năng xuất lịch sử trang đầy đủ bằng mẫu này bị tắt do vấn đề hiệu suất.",
'export-submit'     => 'Xuất',
'export-addcattext' => 'Thêm trang từ thể loại:',
'export-addcat'     => 'Thêm',
'export-download'   => 'Lưu xuống tập tin',
'export-templates'  => 'Gồm cả tiêu bản',

# Namespace 8 related
'allmessages'               => 'Thông báo hệ thống',
'allmessagesname'           => 'Tên thông báo',
'allmessagesdefault'        => 'Nội dung mặc định',
'allmessagescurrent'        => 'Nội dung hiện thời',
'allmessagestext'           => 'Đây là toàn bộ thông báo hệ thống có trong không gian tên MediaWiki.
Mời vào [http://www.mediawiki.org/wiki/Localisation Địa phương hóa MediaWiki]  và [http://translatewiki.net Betawiki] nếu bạn muốn đóng góp dịch chung cả MediaWiki.',
'allmessagesnotsupportedDB' => "Trang này không dùng được vì biến '''\$wgUseDatabaseMessages''' đã bị tắt.",
'allmessagesfilter'         => 'Bộ lọc tên thông báo:',
'allmessagesmodified'       => 'Chỉ hiển thị các thông báo đã được sửa đổi.',

# Thumbnails
'thumbnail-more'           => 'Phóng lớn',
'filemissing'              => 'Không có tập tin',
'thumbnail_error'          => 'Hình thu nhỏ có lỗi: $1',
'djvu_page_error'          => 'Trang DjVu quá xa',
'djvu_no_xml'              => 'Không thể truy xuất XML cho tập tin DjVu',
'thumbnail_invalid_params' => 'Tham số hình thu nhỏ không hợp lệ',
'thumbnail_dest_directory' => 'Không thể tạo thư mục đích',

# Special:Import
'import'                     => 'Nhập các trang',
'importinterwiki'            => 'Nhập giữa các wiki',
'import-interwiki-text'      => 'Chọn tên trang và wiki để nhập trang vào.
Ngày của phiên bản và tên người viết trang sẽ được giữ nguyên.
Tất cả những lần nhập trang từ wiki khác được ghi lại ở [[Special:Log/import|nhật trình nhập trang]].',
'import-interwiki-history'   => 'Sao chép tất cả các phiên bản cũ của trang này',
'import-interwiki-submit'    => 'Nhập trang',
'import-interwiki-namespace' => 'Chuyển các trang vào không gian tên:',
'importtext'                 => 'Xin hãy xuất tập tin từ wiki nguồn sử dụng [[Special:Export|tính năng xuất]].
Lưu nó vào máy tính của bạn rồi tải nó lên đây.',
'importstart'                => 'Đang nhập các trang…',
'import-revision-count'      => '$1 {{PLURAL:$1|phiên bản|phiên bản}}',
'importnopages'              => 'Không có trang để nhập vào.',
'importfailed'               => 'Không nhập được: $1',
'importunknownsource'        => 'Không hiểu nguồn trang để nhập vào',
'importcantopen'             => 'Không có thể mở tập tin để nhập vào',
'importbadinterwiki'         => 'Liên kết liên wiki sai',
'importnotext'               => 'Trang trống hoặc không có nội dung',
'importsuccess'              => 'Nhập thành công!',
'importhistoryconflict'      => 'Có mâu thuẫn trong lịch sử của các phiên bản (trang này có thể đã được nhập vào trước đó)',
'importnosources'            => 'Không có nguồn nhập giữa wiki và việc nhập lịch sử bị tắt.',
'importnofile'               => 'Không tải được tập tin nào lên.',
'importuploaderrorsize'      => 'Không thể tải tập tin nhập trang. Tập tin lớn hơn kích thước cho phép tải lên.',
'importuploaderrorpartial'   => 'Không thể tải tập tin nhập trang. Tập tin mới chỉ tải lên được một phần.',
'importuploaderrortemp'      => 'Không thể tải tập tin nhập trang. Thiếu thư mục tạm.',
'import-parse-failure'       => 'Không thể phân tích tập tin nhập XML',
'import-noarticle'           => 'Không có trang nào để nhập cả!',
'import-nonewrevisions'      => 'Tất cả các phiên bản đều đã được nhập trước đây.',
'xml-error-string'           => '$1 tại dòng $2, cột $3 (byte $4): $5',
'import-upload'              => 'Tải lên dữ liệu XML',

# Import log
'importlogpage'                    => 'Nhật trình nhập trang',
'importlogpagetext'                => 'Đây là danh sách các trang được quản lý nhập vào đây. Các trang này có lịch sử sửa đổi từ hồi ở wiki khác.',
'import-logentry-upload'           => 'nhập vào [[$1]] bằng cách tải tập tin',
'import-logentry-upload-detail'    => '$1 {{PLURAL:$1|phiên bản|phiên bản}}',
'import-logentry-interwiki'        => 'đã nhập vào $1 từ wiki khác',
'import-logentry-interwiki-detail' => '$1 {{PLURAL:$1|phiên bản|phiên bản}} từ $2',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'Trang thành viên của tôi',
'tooltip-pt-anonuserpage'         => 'Trang của IP bạn đang dùng',
'tooltip-pt-mytalk'               => 'Thảo luận với tôi',
'tooltip-pt-anontalk'             => 'Thảo luận với địa chỉ IP này',
'tooltip-pt-preferences'          => 'Tùy chọn cá nhân của tôi',
'tooltip-pt-watchlist'            => 'Thay đổi của các trang tôi theo dõi',
'tooltip-pt-mycontris'            => 'Đóng góp của tôi',
'tooltip-pt-login'                => 'Đăng nhập sẽ có lợi hơn, tuy nhiên không bắt buộc.',
'tooltip-pt-anonlogin'            => 'Không đăng nhập vẫn tham gia được, tuy nhiên đăng nhập sẽ lợi hơn.',
'tooltip-pt-logout'               => 'Đăng xuất',
'tooltip-ca-talk'                 => 'Thảo luận về trang này',
'tooltip-ca-edit'                 => 'Bạn có thể sửa được trang này. Xin xem thử trước khi lưu.',
'tooltip-ca-addsection'           => 'Thêm bàn luận vào đây.',
'tooltip-ca-viewsource'           => 'Trang này được khóa. Bạn có thể xem mã nguồn.',
'tooltip-ca-history'              => 'Những phiên bản cũ của trang này.',
'tooltip-ca-protect'              => 'Khóa trang này lại',
'tooltip-ca-delete'               => 'Xóa trang này',
'tooltip-ca-undelete'             => 'Phục hồi những sửa đổi trên trang này như trước khi nó bị xóa',
'tooltip-ca-move'                 => 'Di chuyển trang này',
'tooltip-ca-watch'                => 'Thêm trang này vào danh sách theo dõi',
'tooltip-ca-unwatch'              => 'Bỏ trang này khỏi danh sách theo dõi',
'tooltip-search'                  => 'Tìm kiếm {{SITENAME}}',
'tooltip-search-go'               => 'Xem trang khớp với tên này nếu có',
'tooltip-search-fulltext'         => 'Tìm trang có nội dung này',
'tooltip-p-logo'                  => 'Trang Chính',
'tooltip-n-mainpage'              => 'Đi đến Trang Chính',
'tooltip-n-portal'                => 'Giới thiệu dự án, cách sử dụng và tìm kiếm thông tin ở đây',
'tooltip-n-currentevents'         => 'Các trang có liên quan đến thời sự',
'tooltip-n-recentchanges'         => 'Danh sách các thay đổi gần đây',
'tooltip-n-randompage'            => 'Xem trang ngẫu nhiên',
'tooltip-n-help'                  => 'Nơi tìm hiểu thêm cách dùng.',
'tooltip-t-whatlinkshere'         => 'Các trang liên kết đến đây',
'tooltip-t-recentchangeslinked'   => 'Thay đổi gần đây của các trang liên kết đến đây',
'tooltip-feed-rss'                => 'Nạp RSS cho trang này',
'tooltip-feed-atom'               => 'Nạp Atom cho trang này',
'tooltip-t-contributions'         => 'Xem đóng góp của người này',
'tooltip-t-emailuser'             => 'Gửi thư cho người này',
'tooltip-t-upload'                => 'Tải hình ảnh hoặc tập tin lên',
'tooltip-t-specialpages'          => 'Danh sách các trang đặc biệt',
'tooltip-t-print'                 => 'Bản để in ra của trang',
'tooltip-t-permalink'             => 'Liên kết thường trực đến phiên bản này của trang',
'tooltip-ca-nstab-main'           => 'Xem trang này',
'tooltip-ca-nstab-user'           => 'Xem trang về người này',
'tooltip-ca-nstab-media'          => 'Xem trang phương tiện',
'tooltip-ca-nstab-special'        => 'Đây là một trang dặc biệt, bạn không thể sửa đổi được nó.',
'tooltip-ca-nstab-project'        => 'Xem trang dự án',
'tooltip-ca-nstab-image'          => 'Xem trang hình',
'tooltip-ca-nstab-mediawiki'      => 'Xem thông báo hệ thống',
'tooltip-ca-nstab-template'       => 'Xem tiêu bản',
'tooltip-ca-nstab-help'           => 'Xem trang trợ giúp',
'tooltip-ca-nstab-category'       => 'Xem trang thể loại',
'tooltip-minoredit'               => 'Đánh dấu đây là sửa đổi nhỏ',
'tooltip-save'                    => 'Lưu lại những thay đổi của bạn',
'tooltip-preview'                 => 'Xem thử những thay đổi, hãy dùng nó trước khi lưu!',
'tooltip-diff'                    => 'Xem thay đổi bạn đã thực hiện.',
'tooltip-compareselectedversions' => 'Xem khác biệt giữa hai phiên bản đã chọn của trang này.',
'tooltip-watch'                   => 'Thêm trang này vào danh sách theo dõi',
'tooltip-recreate'                => 'Tạo lại trang dù cho nó vừa bị xóa',
'tooltip-upload'                  => 'Bắt đầu tải lên',

# Stylesheets
'common.css'   => '/* Mã CSS đặt ở đây sẽ áp dụng cho mọi hình dạng */',
'monobook.css' => '/* Mã CSS đặt ở đây sẽ ảnh hưởng đến thành viên sử dụng hình dạng Monobook */',

# Scripts
'common.js'   => '/* Bất kỳ mã JavaScript ở đây sẽ được tải cho tất cả các thành viên khi tải một trang nào đó lên. */',
'monobook.js' => '/* Những người dùng hình dạng MonoBook tải mã JavaScript ở đây */',

# Metadata
'nodublincore'      => 'Máy chủ không hỗ trợ siêu dữ liệu Dublin Core RDF.',
'nocreativecommons' => 'Máy chủ không hỗ trợ siêu dữ liệu Creative Commons RDF.',
'notacceptable'     => 'Máy chủ không thể cho ra định dạng dữ liệu tương thích với phần mềm của bạn.',

# Attribution
'anonymous'        => 'Thành viên vô danh của {{SITENAME}}',
'siteuser'         => 'Thành viên $1 của {{SITENAME}}',
'lastmodifiedatby' => 'Trang này được $3 cập nhật lần cuối lúc $2, $1.', # $1 date, $2 time, $3 user
'othercontribs'    => 'Dựa trên công trình của $1.',
'others'           => 'những người khác',
'siteusers'        => 'Thành viên $1 của {{SITENAME}}',
'creditspage'      => 'Trang ghi nhận đóng góp',
'nocredits'        => 'Không có thông tin ghi nhận đóng góp cho trang này.',

# Spam protection
'spamprotectiontitle' => 'Bộ lọc chống thư rác',
'spamprotectiontext'  => 'Trang bạn muốn lưu bị bộ lọc thư rác chặn lại.
Đây có thể do một liên kết dẫn tới một địa chỉ bên ngoài đã bị ghi vào danh sách đen.',
'spamprotectionmatch' => 'Nội dung sau đây đã kích hoạt bộ lọc thư rác: $1',
'spambot_username'    => 'Bộ dọn dẹp thư rác MediaWiki',
'spam_reverting'      => 'Lùi lại đến phiên bản cuối không chứa liên kết đến $1',
'spam_blanking'       => 'Tất cả các phiên bản có liên kết đến $1, đang tẩy trống',

# Info page
'infosubtitle'   => 'Thông tin về trang',
'numedits'       => 'Số lần sửa đổi (trang nội dung): $1',
'numtalkedits'   => 'Số lần sửa đổi (trang thảo luận): $1',
'numwatchers'    => 'Số người theo dõi: $1',
'numauthors'     => 'Số người sửa đổi khác nhau (trang nội dung): $1',
'numtalkauthors' => 'Số người sửa đổi khác nhau (trang thảo luận): $1',

# Math options
'mw_math_png'    => 'Luôn cho ra dạng hình PNG',
'mw_math_simple' => 'HTML nếu rất đơn giản, nếu không thì PNG',
'mw_math_html'   => 'HTML nếu có thể, nếu không thì PNG',
'mw_math_source' => 'Để là TeX (dành cho trình duyệt văn bản)',
'mw_math_modern' => 'Đề nghị, dành cho trình duyệt hiện đại',
'mw_math_mathml' => 'MathML nếu có thể (thử nghiệm)',

# Patrolling
'markaspatrolleddiff'                 => 'Đánh dấu tuần tra',
'markaspatrolledtext'                 => 'Đánh dấu tuần tra trang này',
'markedaspatrolled'                   => 'Đã đánh dấu tuần tra',
'markedaspatrolledtext'               => 'Phiên bản được chọn đã được đánh dấu đã tuần tra.',
'rcpatroldisabled'                    => '“Thay đổi gần đây” của các trang tuần tra không bật',
'rcpatroldisabledtext'                => 'Chức năng “thay đổi gần đây” của các trang tuần tra hiện không được bật.',
'markedaspatrollederror'              => 'Không thể đánh dấu tuần tra',
'markedaspatrollederrortext'          => 'Bạn phải chọn phiên bản để đánh dấu tuần tra.',
'markedaspatrollederror-noautopatrol' => 'Bạn không được đánh dấu tuần tra vào sửa đổi của bạn.',

# Patrol log
'patrol-log-page'   => 'Nhật ký tuần tra',
'patrol-log-header' => 'Đây là nhật trình tuần tra phiên bản.',
'patrol-log-line'   => 'đánh dấu tuần tra vào phiên bản $1 của $2 $3',
'patrol-log-auto'   => '(tự động)',
'patrol-log-diff'   => 'bản $1',

# Image deletion
'deletedrevision'                 => 'Đã xóa phiên bản cũ $1',
'filedeleteerror-short'           => 'Lỗi xóa tập tin: $1',
'filedeleteerror-long'            => 'Có lỗi khi xóa tập tin:

$1',
'filedelete-missing'              => 'Không thể xóa tập tin “$1” vì không tồn tại.',
'filedelete-old-unregistered'     => 'Phiên bản chỉ định “$1” không có trong cơ sở dữ liệu.',
'filedelete-current-unregistered' => 'Tập tin “$1” không thấy trong cơ sở dữ liệu.',
'filedelete-archive-read-only'    => 'Máy chủ web không ghi được vào thư mục lưu trữ “$1”.',

# Browsing diffs
'previousdiff' => '← Sửa đổi cũ',
'nextdiff'     => 'Sửa đổi sau →',

# Media information
'mediawarning'         => "'''Cảnh báo''': Tập tin này có thể chứa mã hiểm độc, nếu thực thi nó máy tính của bạn có thể bị tiếm quyền.<hr />",
'imagemaxsize'         => 'Giới hạn độ phân giải trên trang miêu tả tập tin:',
'thumbsize'            => 'Kích thước thu nhỏ:',
'widthheightpage'      => '$1×$2, $3 {{PLURAL:$3|trang|trang}}',
'file-info'            => '(kích thước tập tin: $1, định dạng MIME: $2)',
'file-info-size'       => '($1 × $2 điểm ảnh, kích thước: $3, định dạng MIME: $4)',
'file-nohires'         => '<small>Không có độ phân giải cao hơn.</small>',
'svg-long-desc'        => '(tập tin SVG, $1 × $2 điểm ảnh trên danh nghĩa, kích thước: $3)',
'show-big-image'       => 'Độ phân giải tối đa',
'show-big-image-thumb' => '<small>Kích thước xem thử: $1 × $2 điểm ảnh</small>',

# Special:NewImages
'newimages'             => 'Trang trưng bày hình ảnh mới',
'imagelisttext'         => "Dưới đây là danh sách '''$1''' {{PLURAL:$1|tập tin|tập tin}} xếp theo $2.",
'newimages-summary'     => 'Trang đặc biệt này hiển thị các tập tin được tải lên gần đây nhất.',
'showhidebots'          => '($1 robot)',
'noimages'              => 'Chưa có hình.',
'ilsubmit'              => 'Tìm kiếm',
'bydate'                => 'theo ngày',
'sp-newimages-showfrom' => 'Trưng bày những tập tin mới, bắt đầu từ lúc $2, ngày $1',

# Bad image list
'bad_image_list' => 'Định dạng như sau:

Chỉ có những mục được liệt kê (những dòng bắt đầu bằng *) mới được tính tới. Liên kết đầu tiên tại một dòng phải là liên kết đến tập tin phản cảm.
Các liên kết sau đó trên cùng một dòng được xem là các ngoại lệ, có nghĩa là các trang mà tại đó có thể dùng được tập tin.',

# Metadata
'metadata'          => 'Đặc tính hình',
'metadata-help'     => 'Tập tin này có chứa thông tin về nó, do máy ảnh hay máy quét thêm vào. Nếu tập tin bị sửa đổi sau khi được tạo ra lần đầu, có thể thông tin này không được cập nhật.',
'metadata-expand'   => 'Hiện chi tiết cấp cao',
'metadata-collapse' => 'Ẩn chi tiết cấp cao',
'metadata-fields'   => 'Những thông tin đặc tính EXIF được danh sách dưới đây sẽ được đưa vào vào trang miêu tả hình khi bảng đặc tính được thu nhỏ.
Những thông tin khác mặc định sẽ được ẩn đi.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => 'Bề ngang',
'exif-imagelength'                 => 'Chiều cao',
'exif-bitspersample'               => 'Bit trên mẫu',
'exif-compression'                 => 'Kiểu nén',
'exif-photometricinterpretation'   => 'Thành phần điểm ảnh',
'exif-orientation'                 => 'Hướng',
'exif-samplesperpixel'             => 'Số mẫu trên điểm ảnh',
'exif-planarconfiguration'         => 'Cách xếp dữ liệu',
'exif-ycbcrsubsampling'            => 'Tỉ lệ lấy mẫu con của Y so với C',
'exif-ycbcrpositioning'            => 'Định vị Y và C',
'exif-xresolution'                 => 'Phân giải theo bề ngang',
'exif-yresolution'                 => 'Phân giải theo chiều cao',
'exif-resolutionunit'              => 'Đơn vị phân giải X và Y',
'exif-stripoffsets'                => 'Vị trí dữ liệu hình',
'exif-rowsperstrip'                => 'Số hàng trên mỗi mảnh',
'exif-stripbytecounts'             => 'Số byte trên mỗi mảnh nén',
'exif-jpeginterchangeformat'       => 'Vị trí SOI JPEG',
'exif-jpeginterchangeformatlength' => 'Kích cỡ (byte) của JPEG',
'exif-transferfunction'            => 'Hàm chuyển đổi',
'exif-whitepoint'                  => 'Sắc độ điểm trắng',
'exif-primarychromaticities'       => 'Sắc độ của màu cơ bản',
'exif-ycbcrcoefficients'           => 'Hệ số ma trận biến đổi không gian màu',
'exif-referenceblackwhite'         => 'Giá trị tham chiếu cặp trắng đen',
'exif-datetime'                    => 'Ngày giờ sửa tập tin',
'exif-imagedescription'            => 'Tiêu đề của hình',
'exif-make'                        => 'Hãng máy ảnh',
'exif-model'                       => 'Kiểu máy ảnh',
'exif-software'                    => 'Phần mềm đã dùng',
'exif-artist'                      => 'Tác giả',
'exif-copyright'                   => 'Bản quyền',
'exif-exifversion'                 => 'Phiên bản Exif',
'exif-flashpixversion'             => 'Phiên bản Flashpix được hỗ trợ',
'exif-colorspace'                  => 'Không gian màu',
'exif-componentsconfiguration'     => 'Ý nghĩa thành phần',
'exif-compressedbitsperpixel'      => 'Độ nén (bit/điểm)',
'exif-pixelydimension'             => 'Bề ngang hợp lệ',
'exif-pixelxdimension'             => 'Chiều cao hợp lệ',
'exif-makernote'                   => 'Ghi chú của nhà sản xuất',
'exif-usercomment'                 => 'Lời bình của tác giả',
'exif-relatedsoundfile'            => 'Tập tin âm thanh liên quan',
'exif-datetimeoriginal'            => 'Ngày giờ sinh dữ liệu',
'exif-datetimedigitized'           => 'Ngày giờ số hóa',
'exif-subsectime'                  => 'Ngày giờ nhỏ hơn giây',
'exif-subsectimeoriginal'          => 'Ngày giờ gốc nhỏ hơn giây',
'exif-subsectimedigitized'         => 'Ngày giờ số hóa nhỏ hơn giây',
'exif-exposuretime'                => 'Thời gian mở ống kính',
'exif-exposuretime-format'         => '$1 giây ($2)',
'exif-fnumber'                     => 'Số F',
'exif-exposureprogram'             => 'Chương trình phơi sáng',
'exif-spectralsensitivity'         => 'Độ nhạy quang phổ',
'exif-isospeedratings'             => 'Điểm tốc độ ISO',
'exif-oecf'                        => 'Yếu tố chuyển đổi quang điện',
'exif-shutterspeedvalue'           => 'Tốc độ cửa chớp',
'exif-aperturevalue'               => 'Độ mở ống kính',
'exif-brightnessvalue'             => 'Độ sáng',
'exif-exposurebiasvalue'           => 'Độ lệch phơi sáng',
'exif-maxaperturevalue'            => 'Khẩu độ cực đại qua đất',
'exif-subjectdistance'             => 'Khoảng cách vật thể',
'exif-meteringmode'                => 'Chế độ đo',
'exif-lightsource'                 => 'Nguồn sáng',
'exif-flash'                       => 'Đèn chớp',
'exif-focallength'                 => 'Độ dài tiêu cự thấu kính',
'exif-subjectarea'                 => 'Diện tích vật thể',
'exif-flashenergy'                 => 'Nguồn đèn chớp',
'exif-spatialfrequencyresponse'    => 'Phản ứng tần số không gian',
'exif-focalplanexresolution'       => 'Phân giải X trên mặt phẳng tiêu',
'exif-focalplaneyresolution'       => 'Phân giải Y trên mặt phẳng tiêu',
'exif-focalplaneresolutionunit'    => 'Đơn vị phân giải trên mặt phẳng tiêu',
'exif-subjectlocation'             => 'Vị trí vật thể',
'exif-exposureindex'               => 'Chỉ số phơi sáng',
'exif-sensingmethod'               => 'Phương pháp đo',
'exif-filesource'                  => 'Nguồn tập tin',
'exif-scenetype'                   => 'Loại cảnh',
'exif-cfapattern'                  => 'Mẫu CFA',
'exif-customrendered'              => 'Sửa hình thủ công',
'exif-exposuremode'                => 'Chế độ phơi sáng',
'exif-whitebalance'                => 'Độ sáng trắng',
'exif-digitalzoomratio'            => 'Tỉ lệ phóng lớn kỹ thuật số',
'exif-focallengthin35mmfilm'       => 'Tiêu cự trong phim 35 mm',
'exif-scenecapturetype'            => 'Kiểu chụp cảnh',
'exif-gaincontrol'                 => 'Điều khiển cảnh',
'exif-contrast'                    => 'Độ tương phản',
'exif-saturation'                  => 'Độ bão hòa',
'exif-sharpness'                   => 'Độ sắc nét',
'exif-devicesettingdescription'    => 'Miêu tả cài đặt thiết bị',
'exif-subjectdistancerange'        => 'Khoảng cách tới vật',
'exif-imageuniqueid'               => 'ID hình duy nhất',
'exif-gpsversionid'                => 'Phiên bản thẻ GPS',
'exif-gpslatituderef'              => 'Vĩ độ bắc hay nam',
'exif-gpslatitude'                 => 'Vĩ độ',
'exif-gpslongituderef'             => 'Kinh độ đông hay tây',
'exif-gpslongitude'                => 'Kinh độ',
'exif-gpsaltituderef'              => 'Tham chiếu cao độ',
'exif-gpsaltitude'                 => 'Độ cao',
'exif-gpstimestamp'                => 'Giờ GPS (đồng hồ nguyên tử)',
'exif-gpssatellites'               => 'Vệ tinh nhân tạo dùng để đo',
'exif-gpsstatus'                   => 'Tình trạng đầu thu',
'exif-gpsmeasuremode'              => 'Chế độ đo',
'exif-gpsdop'                      => 'Độ chính xác máy đo',
'exif-gpsspeedref'                 => 'Đơn vị tốc độ',
'exif-gpsspeed'                    => 'Tốc độ đầu thu GPS',
'exif-gpstrackref'                 => 'Tham chiếu cho hướng chuyển động',
'exif-gpstrack'                    => 'Hướng chuyển động',
'exif-gpsimgdirectionref'          => 'Tham chiếu cho hướng của ảnh',
'exif-gpsimgdirection'             => 'Hướng của hình',
'exif-gpsmapdatum'                 => 'Dữ liệu trắc địa đã dùng',
'exif-gpsdestlatituderef'          => 'Tham chiếu cho vĩ độ đích',
'exif-gpsdestlatitude'             => 'Vĩ độ đích',
'exif-gpsdestlongituderef'         => 'Tham chiếu cho kinh độ đích',
'exif-gpsdestlongitude'            => 'Kinh độ đích',
'exif-gpsdestbearingref'           => 'Tham chiếu cho phương hướng đích',
'exif-gpsdestbearing'              => 'Phương hướng đích',
'exif-gpsdestdistanceref'          => 'Tham chiếu cho khoảng cách đến đích',
'exif-gpsdestdistance'             => 'Khoảng cách đến đích',
'exif-gpsprocessingmethod'         => 'Tên phương pháp xử lý GPS',
'exif-gpsareainformation'          => 'Tên khu vực theo GPS',
'exif-gpsdatestamp'                => 'Ngày theo GPS',
'exif-gpsdifferential'             => 'Sửa vi sai GPS',

# EXIF attributes
'exif-compression-1' => 'Không nén',

'exif-unknowndate' => 'Không biết ngày',

'exif-orientation-1' => 'Thường', # 0th row: top; 0th column: left
'exif-orientation-2' => 'Lộn ngược theo phương ngang', # 0th row: top; 0th column: right
'exif-orientation-3' => 'Quay 180°', # 0th row: bottom; 0th column: right
'exif-orientation-4' => 'Lộn ngược theo phương dọc', # 0th row: bottom; 0th column: left
'exif-orientation-5' => 'Quay 90° bên trái và lộn thẳng đứng', # 0th row: left; 0th column: top
'exif-orientation-6' => 'Quay 90° bên phải', # 0th row: right; 0th column: top
'exif-orientation-7' => 'Quay 90° bên phải và lộn thẳng đứng', # 0th row: right; 0th column: bottom
'exif-orientation-8' => 'Quay 90° bên trái', # 0th row: left; 0th column: bottom

'exif-planarconfiguration-1' => 'định dạng thấp',
'exif-planarconfiguration-2' => 'định dạng phẳng',

'exif-componentsconfiguration-0' => 'không tồn tại',

'exif-exposureprogram-0' => 'Không chỉ định',
'exif-exposureprogram-1' => 'Thủ công',
'exif-exposureprogram-2' => 'Chương trình chuẩn',
'exif-exposureprogram-3' => 'Ưu tiên độ mở ống kính',
'exif-exposureprogram-4' => 'Ưu tiên tốc độ sập',
'exif-exposureprogram-5' => 'Chương trình sáng tạo (thiên về chiều sâu)',
'exif-exposureprogram-6' => 'Chương trình chụp (thien về tốc độ sập nhanh)',
'exif-exposureprogram-7' => 'Chế độ chân dung (đối với ảnh chụp gần với phông nền ở ngoài tầm tiêu cự)',
'exif-exposureprogram-8' => 'Chế độ phong cảnh (đối với ảnh phong cảnh với phông ở trong tiêu cự)',

'exif-subjectdistance-value' => '$1 mét',

'exif-meteringmode-0'   => 'Không biết',
'exif-meteringmode-1'   => 'Trung bình',
'exif-meteringmode-2'   => 'Trung bình trọng lượng ở giữa',
'exif-meteringmode-3'   => 'Vết',
'exif-meteringmode-4'   => 'Đa vết',
'exif-meteringmode-5'   => 'Lấy mẫu',
'exif-meteringmode-6'   => 'Cục bộ',
'exif-meteringmode-255' => 'Khác',

'exif-lightsource-0'   => 'Không biết',
'exif-lightsource-1'   => 'Trời nắng',
'exif-lightsource-2'   => 'Huỳnh quang',
'exif-lightsource-3'   => 'Vonfram (ánh nóng sáng)',
'exif-lightsource-4'   => 'Đèn chớp',
'exif-lightsource-9'   => 'Trời đẹp',
'exif-lightsource-10'  => 'Trời mây',
'exif-lightsource-11'  => 'Che nắng',
'exif-lightsource-12'  => 'Nắng huỳnh quang (D 5700 – 7100K)',
'exif-lightsource-13'  => 'Màu trắng huỳnh quang ban ngày (N 4600 – 5400K)',
'exif-lightsource-14'  => 'Màu trắng mát huỳnh quang (W 3900 – 4500K)',
'exif-lightsource-15'  => 'Màu trắng huỳnh quang (WW 3200 – 3700K)',
'exif-lightsource-17'  => 'Ánh chuẩn A',
'exif-lightsource-18'  => 'Ánh chuẩn B',
'exif-lightsource-19'  => 'Ánh chuẩn C',
'exif-lightsource-24'  => 'Vonfram xưởng ISO',
'exif-lightsource-255' => 'Nguồn ánh sáng khác',

'exif-focalplaneresolutionunit-2' => 'inch',

'exif-sensingmethod-1' => 'Không định rõ',
'exif-sensingmethod-2' => 'Cảm biến vùng màu một mảnh',
'exif-sensingmethod-3' => 'Cảm biến vùng màu hai mảnh',
'exif-sensingmethod-4' => 'Cảm biến vùng màu ba mảnh',
'exif-sensingmethod-5' => 'Cảm biến vùng màu liên tục',
'exif-sensingmethod-7' => 'Cảm biến ba đường',
'exif-sensingmethod-8' => 'Cảm biến đường màu liên tục',

'exif-scenetype-1' => 'Hình chụp thẳng',

'exif-customrendered-0' => 'Thường',
'exif-customrendered-1' => 'Thủ công',

'exif-exposuremode-0' => 'Phơi sáng tự động',
'exif-exposuremode-1' => 'Phơi sáng thủ công',
'exif-exposuremode-2' => 'Tự động chụp nhiều hình',

'exif-whitebalance-0' => 'Cân bằng trắng tự động',
'exif-whitebalance-1' => 'Cân bằng trắng thủ công',

'exif-scenecapturetype-0' => 'Chuẩn',
'exif-scenecapturetype-1' => 'Nằm',
'exif-scenecapturetype-2' => 'Đứng',
'exif-scenecapturetype-3' => 'Cảnh ban đêm',

'exif-gaincontrol-0' => 'Không có',
'exif-gaincontrol-1' => 'Độ rọi thấp',
'exif-gaincontrol-2' => 'Độ rọi cao',
'exif-gaincontrol-3' => 'Độ rọi dưới thấp',
'exif-gaincontrol-4' => 'Độ rọi dưới cao',

'exif-contrast-0' => 'Thường',
'exif-contrast-1' => 'Nhẹ',
'exif-contrast-2' => 'Mạnh',

'exif-saturation-0' => 'Thường',
'exif-saturation-1' => 'Độ bão hòa thấp',
'exif-saturation-2' => 'Độ bão hòa cao',

'exif-sharpness-0' => 'Thường',
'exif-sharpness-1' => 'Dẻo',
'exif-sharpness-2' => 'Cứng',

'exif-subjectdistancerange-0' => 'Không biết',
'exif-subjectdistancerange-1' => 'Macro',
'exif-subjectdistancerange-2' => 'Nhìn gần',
'exif-subjectdistancerange-3' => 'Nhìn xa',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'Vĩ độ bắc',
'exif-gpslatitude-s' => 'Vĩ độ nam',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'Kinh độ đông',
'exif-gpslongitude-w' => 'Kinh độ tây',

'exif-gpsstatus-a' => 'Đang đo',
'exif-gpsstatus-v' => 'Mức độ khả năng liên điều hành',

'exif-gpsmeasuremode-2' => 'Đo 2 chiều',
'exif-gpsmeasuremode-3' => 'Đo 3 chiều',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'Kilômét một giờ',
'exif-gpsspeed-m' => 'Dặm một giờ',
'exif-gpsspeed-n' => 'Hải lý một giờ',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => 'Hướng thật',
'exif-gpsdirection-m' => 'Hướng từ trường',

# External editor support
'edit-externally'      => 'Sửa bằng phần mềm bên ngoài',
'edit-externally-help' => '* Xem thêm [http://www.mediawiki.org/wiki/Manual:External_editors hướng dẫn bằng tiếng Anh]',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'tất cả',
'imagelistall'     => 'tất cả',
'watchlistall2'    => 'tất cả',
'namespacesall'    => 'tất cả',
'monthsall'        => 'tất cả',

# E-mail address confirmation
'confirmemail'             => 'Xác nhận thư điện tử',
'confirmemail_noemail'     => 'Bạn chưa đưa vào địa chỉ thư điện tử hợp lệ ở [[Special:Preferences|tùy chọn cá nhân]].',
'confirmemail_text'        => '{{SITENAME}} đòi hỏi bạn xác minh thư điện tử của mình
trước khi sử dụng tính năng thư điện tử. Nhấn vào nút bên dưới để gửi thư
xác nhận đến địa chỉ của bạn. Thư xác nhận sẽ có kèm một liên kết có chứa một mã số;
tải liên kết đó trong trình duyệt để xác nhận địa chỉ thư điện tử của bạn là đúng.',
'confirmemail_pending'     => '<div class="error">
Mã xác đã được gửi đến địa chỉ thư điện tử của bạn; nếu bạn
mới vừa tạo tài khoản, xin chờ vài phút để thư tới nơi rồi
hãy cố gắng yêu cầu mã mới.
</div>',
'confirmemail_send'        => 'Gửi thư xác nhận',
'confirmemail_sent'        => 'Thư xác nhận đã được gửi',
'confirmemail_oncreate'    => 'Đã gửi mã xác nhận đến địa chỉ thư điện tử của bạn.
Bạn không cần mã này để đăng nhập, nhưng sẽ cần sử dụng nó để bật các tính năng có dùng thư điện tử của wiki.',
'confirmemail_sendfailed'  => '{{SITENAME}} không thể gửi thư xác nhận.
Xin kiểm tra lại địa chỉ thư xem có bị nhầm ký tự nào không.

Chương trình thư báo rằng: $1',
'confirmemail_invalid'     => 'Mã xác nhận sai. Mã này có thể đã hết hạn',
'confirmemail_needlogin'   => 'Bạn cần phải $1 để xác nhận địa chỉ thư điện tử.',
'confirmemail_success'     => 'Thư điện tử của bạn đã được xác nhận. Bạn đã có thể đăng nhập và bắt đầu sử dụng wiki.',
'confirmemail_loggedin'    => 'Địa chỉ thư điện tử của bạn đã được xác nhận',
'confirmemail_error'       => 'Có trục trặc khi lưu xác nhận của bạn.',
'confirmemail_subject'     => 'Xác nhận thư điện tử tại {{SITENAME}}',
'confirmemail_body'        => 'Ai đó, có thể là bạn, từ địa chỉ IP $1,
đã đăng ký tài khoản có tên "$2" với địa chỉ thư điện tử này tại {{SITENAME}}.

Để xác nhận rằng tài khoản này thực sự là của bạn và để kích hoạt tính năng thư điện tử tại {{SITENAME}}, xin mở liên kết này trong trình duyệt:

$3

Nếu bạn *không* đăng ký tài khoản, hãy nhấn vào liên kết này
để hủy thủ tục xác nhận địa chỉ thư điện tử:

$5

Mã xác nhận này sẽ hết hạn vào $4.',
'confirmemail_invalidated' => 'Đã hủy xác nhận địa chỉ thư điện tử',
'invalidateemail'          => 'Hủy xác nhận thư điện tử',

# Scary transclusion
'scarytranscludedisabled' => '[Nhúng giữa các wiki bị tắt]',
'scarytranscludefailed'   => '[Truy xuất tiêu bản cho $1 thất bại]',
'scarytranscludetoolong'  => '[Địa chỉ URL quá dài]',

# Trackbacks
'trackbackbox'      => '<div id="mw_trackbacks">
Các TrackBack về trang này:<br />
$1
</div>',
'trackbackremove'   => ' ([$1 Xóa])',
'trackbacklink'     => 'TrackBack',
'trackbackdeleteok' => 'Đã xóa trackback.',

# Delete conflict
'deletedwhileediting' => "'''Cảnh báo''': Trang này đã bị xóa sau khi bắt đầu sửa đổi!",
'confirmrecreate'     => "Thành viên [[User:$1|$1]] ([[User talk:$1|thảo luận]]) đã xóa trang này sau khi bạn bắt đầu sửa đổi trang với lý do:
: ''$2''
Xin hãy xác nhận bạn thực sự muốn tạo lại trang này.",
'recreate'            => 'Tạo ra lại',

# HTML dump
'redirectingto' => 'Đang đổi hướng đến [[:$1]]…',

# action=purge
'confirm_purge'        => 'Làm sạch vùng nhớ đệm của trang này?

$1',
'confirm_purge_button' => 'OK',

# AJAX search
'searchcontaining' => "Tìm những trang có chứa ''$1''.",
'searchnamed'      => "Tìm những trang có tên ''$1''.",
'articletitles'    => "Những trang bắt đầu bằng ''$1''",
'hideresults'      => 'Ẩn kết quả',
'useajaxsearch'    => 'Dùng tìm kiếm AJAX',

# Multipage image navigation
'imgmultipageprev' => '← trang trước',
'imgmultipagenext' => 'trang sau →',
'imgmultigo'       => 'Xem',
'imgmultigoto'     => 'Đi đến trang $1',

# Table pager
'ascending_abbrev'         => 'tăng',
'descending_abbrev'        => 'giảm',
'table_pager_next'         => 'Trang sau',
'table_pager_prev'         => 'Trang trước',
'table_pager_first'        => 'Trang đầu',
'table_pager_last'         => 'Trang cuối',
'table_pager_limit'        => 'Xem $1 kết quả mỗi trang',
'table_pager_limit_submit' => 'Xem',
'table_pager_empty'        => 'Không có kết quả nào.',

# Auto-summaries
'autosumm-blank'   => 'Tẩy trống',
'autosumm-replace' => 'Thay cả nội dung bằng “$1”',
'autoredircomment' => 'Đổi hướng đến [[$1]]',
'autosumm-new'     => 'Trang mới: $1',

# Size units
'size-kilobytes' => '$1 kB',

# Live preview
'livepreview-loading' => 'Đang tải…',
'livepreview-ready'   => 'Đang tải… Xong!',
'livepreview-failed'  => 'Không thể xem thử trực tiếp! Hãy dùng thử chế độ xem thử thông thường.',
'livepreview-error'   => 'Không thể kết nối: $1 “$2”. Hãy dùng thử chế độ xem thử thông thường.',

# Friendlier slave lag warnings
'lag-warn-normal' => 'Những thay đổi trong vòng $1 {{PLURAL:||}}giây trở lại đây có thể chưa xuất hiện trong danh sách.',
'lag-warn-high'   => 'Do độ trễ của máy chủ cơ sở dữ liệu, những thay đổi trong vòng $1 {{PLURAL:$1||}}giây trở lại đây có thể chưa xuất hiện trong danh sách.',

# Watchlist editor
'watchlistedit-numitems'       => 'Danh sách theo dõi của bạn có $1 {{PLURAL:$1|tựa đề|tựa đề}}, không tính các trang thảo luận.',
'watchlistedit-noitems'        => 'Danh sách các trang bạn theo dõi hiện không có gì.',
'watchlistedit-normal-title'   => 'Sửa các trang tôi theo dõi',
'watchlistedit-normal-legend'  => 'Bỏ các trang đang theo dõi ra khỏi danh sách',
'watchlistedit-normal-explain' => 'Tên các trang bạn theo dõi được hiển thị dưới đây. Để xóa một tên trang, chọn vào hộp kiểm bên cạnh nó, rồi nhấn “Bỏ trang đã chọn”. Bạn cũng có thể [[Special:Watchlist/raw|sửa danh sách theo dạng thô]].',
'watchlistedit-normal-submit'  => 'Bỏ trang đã chọn',
'watchlistedit-normal-done'    => '$1 {{PLURAL:$1|tựa đề|tựa đề}} đã được xóa khỏi danh sách các trang theo dõi:',
'watchlistedit-raw-title'      => 'Sửa danh sách theo dõi dạng thô',
'watchlistedit-raw-legend'     => 'Sửa danh sách theo dõi dạng thô',
'watchlistedit-raw-explain'    => 'Tên các trang bạn theo dõi đuọc hiển thị dưới đây, và có thể được sửa chữa bằng cách thêm vào hoặc bỏ ra khỏi danh sách; mỗi trang một hàng.
Khi xong, nhấn nút ”Cập nhật Trang tôi theo dõi”.
Bạn cũng có thể [[Special:Watchlist/edit|dùng trình soạn thảo chuẩn]] để sửa danh sách này.',
'watchlistedit-raw-titles'     => 'Tên các trang:',
'watchlistedit-raw-submit'     => 'Cập nhật Trang tôi theo dõi',
'watchlistedit-raw-done'       => 'Danh sách các trang bạn theo dõi đã được cập nhật.',
'watchlistedit-raw-added'      => '$1 {{PLURAL:$1|tựa đề|tựa đề}} đã được thêm vào:',
'watchlistedit-raw-removed'    => '$1 {{PLURAL:$1|tựa đề|tựa đề}} đã được xóa khỏi danh sách:',

# Watchlist editing tools
'watchlisttools-view' => 'Xem thay đổi trên các trang theo dõi',
'watchlisttools-edit' => 'Xem và sửa danh sách theo dõi',
'watchlisttools-raw'  => 'Sửa danh sách theo dõi dạng thô',

# Core parser functions
'unknown_extension_tag' => 'Không hiểu thẻ mở rộng “$1”',

# Special:Version
'version'                          => 'Phiên bản', # Not used as normal message but as header for the special page itself
'version-extensions'               => 'Các phần mở rộng được cài đặt',
'version-specialpages'             => 'Trang đặc biệt',
'version-parserhooks'              => 'Hook trong bộ xử lý',
'version-variables'                => 'Biến',
'version-other'                    => 'Phần mở rộng khác',
'version-mediahandlers'            => 'Bộ xử lý phương tiện',
'version-hooks'                    => 'Các hook',
'version-extension-functions'      => 'Hàm mở rộng',
'version-parser-extensiontags'     => 'Thẻ mở rộng trong bộ xử lý',
'version-parser-function-hooks'    => 'Hook cho hàm cú pháp trong bộ xử lý',
'version-skin-extension-functions' => 'Hàm mở rộng skin',
'version-hook-name'                => 'Tên hook',
'version-hook-subscribedby'        => 'Được theo dõi bởi',
'version-version'                  => 'phiên bản',
'version-license'                  => 'Giấy phép bản quyền',
'version-software'                 => 'Phần mềm được cài đặt',
'version-software-product'         => 'Phần mềm',
'version-software-version'         => 'Phiên bản',

# Special:FilePath
'filepath'         => 'Đường dẫn tập tin',
'filepath-page'    => 'Tập tin:',
'filepath-submit'  => 'Hiển thị tập tin',
'filepath-summary' => 'Trang này chuyển bạn thẳng đến địa chỉ của một tập tin. Nếu là hình, địa chỉ là của hình kích thước tối đa; các loại tập tin khác sẽ được mở lên ngay trong chương trình đúng.

Hãy ghi vào tên tập tin, không bao gồm tiền tố “{{ns:image}}:”.',

# Special:FileDuplicateSearch
'fileduplicatesearch'          => 'Tìm kiếm các tập tin trùng lắp',
'fileduplicatesearch-summary'  => 'Tìm kiếm các bản sao y hệt với tập tin khác, theo giá trị băm của nó.

Hãy cho vào tên của tập tin, trừ tiền tố “{{ns:image}}:”.',
'fileduplicatesearch-legend'   => 'Tìm kiếm tập tin trùng lắp',
'fileduplicatesearch-filename' => 'Tên tập tin:',
'fileduplicatesearch-submit'   => 'Tìm kiếm',
'fileduplicatesearch-info'     => '$1 × $2 điểm ảnh<br />Kích thước tập tin: $3<br />Định dạng MIME: $4',
'fileduplicatesearch-result-1' => 'Không có bản sao y hệt với tập tin “$1”.',
'fileduplicatesearch-result-n' => 'Có {{PLURAL:$2|1 bản sao|$2 bản sao}} y hệt với tập tin “$1”.',

# Special:SpecialPages
'specialpages'                   => 'Các trang đặc biệt',
'specialpages-note'              => '----
* Trang đặc biệt thông thường.
* <span class="mw-specialpagerestricted">Trang đặc biệt có hạn chế.</span>',
'specialpages-group-maintenance' => 'Báo cáo bảo quản',
'specialpages-group-other'       => 'Những trang đặc biệt khác',
'specialpages-group-login'       => 'Đăng nhập / Mở tài khoản',
'specialpages-group-changes'     => 'Thay đổi gần đây và nhật trình',
'specialpages-group-media'       => 'Báo cáo và tải lên phương tiện',
'specialpages-group-users'       => 'Thành viên và chức năng',
'specialpages-group-highuse'     => 'Trang được dùng nhiều',
'specialpages-group-pages'       => 'Danh sách các trang',
'specialpages-group-pagetools'   => 'Công cụ cho trang',
'specialpages-group-wiki'        => 'Dữ liệu và công cụ cho wiki',
'specialpages-group-redirects'   => 'Đang đổi hướng trang đặc biệt',
'specialpages-group-spam'        => 'Công cụ chống spam',

# Special:BlankPage
'blankpage'              => 'Trang trắng',
'intentionallyblankpage' => 'Trang này được chủ định để trắng',

);
