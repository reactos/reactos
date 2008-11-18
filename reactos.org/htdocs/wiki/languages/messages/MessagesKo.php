<?php
/** Korean (한국어)
 *
 * @ingroup Language
 * @file
 *
 * @author Albamhandae
 * @author Ficell
 * @author ITurtle
 * @author Klutzy
 * @author Kwj2772
 * @author Pi.C.Noizecehx
 * @author PuzzletChung
 * @author TheAlpha for knowledge
 * @author ToePeu
 * @author לערי ריינהארט
 */

$namespaceNames = array(
	NS_MEDIA          => 'Media',
	NS_SPECIAL        => '특수기능',
	NS_MAIN           => '',
	NS_TALK           => '토론',
	NS_USER           => '사용자',
	NS_USER_TALK      => '사용자토론',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK   => '$1토론',
	NS_IMAGE          => '그림',
	NS_IMAGE_TALK     => '그림토론',
	NS_MEDIAWIKI      => 'MediaWiki',
	NS_MEDIAWIKI_TALK => 'MediaWiki토론',
	NS_TEMPLATE       => '틀',
	NS_TEMPLATE_TALK  => '틀토론',
	NS_HELP           => '도움말',
	NS_HELP_TALK      => '도움말토론',
	NS_CATEGORY       => '분류',
	NS_CATEGORY_TALK  => '분류토론',
);

$skinNames = array(
	'standard' => '표준',
	'monobook' => '모노북',
	'myskin'   => '내 스킨',
);

$bookstoreList = array(
	'Aladdin.co.kr' => 'http://www.aladdin.co.kr/catalog/book.asp?ISBN=$1',
	'inherit' => true,
);

$datePreferences = false;
$defaultDateFormat = 'ko';
$dateFormats = array(
	'ko time' => 'H:i',
	'ko date' => 'Y년 M월 j일 (D)',
	'ko both' => 'Y년 M월 j일 (D) H:i',
);

$messages = array(
# User preference toggles
'tog-underline'               => '링크에 밑줄치기:',
'tog-highlightbroken'         => '없는 문서로 연결된 링크를 <a href="" class="new">이렇게</a> 보이기(선택하지 않으면 <a href="" class="internal">이렇게</a> 보임)',
'tog-justify'                 => '문단 정렬하기',
'tog-hideminor'               => '사소한 편집을 최근 바뀜에서 숨기기',
'tog-extendwatchlist'         => '주시 문서를 모든 변경 목록에 적용하기',
'tog-usenewrc'                => '향상된 최근 바뀜 (자바스크립트)',
'tog-numberheadings'          => '머릿글 번호 매기기',
'tog-showtoolbar'             => '편집창에 툴바 보이기 (자바스크립트)',
'tog-editondblclick'          => '더블클릭으로 문서 편집하기 (자바스크립트)',
'tog-editsection'             => '‘편집’ 링크로 부분을 편집하는 기능 켜기',
'tog-editsectiononrightclick' => '제목을 오른쪽 클릭해서 부분 편집하기 (자바스크립트)',
'tog-showtoc'                 => '문서의 차례 보여주기 (머릿글이 4개 이상인 경우)',
'tog-rememberpassword'        => '자동 로그인',
'tog-editwidth'               => '편집상자의 너비를 최대로 맞추기',
'tog-watchcreations'          => '내가 처음 만드는 문서를 주시 목록에 추가하기',
'tog-watchdefault'            => '내가 편집하는 문서를 주시문서 목록에 추가하기',
'tog-watchmoves'              => '내가 이동하는 문서를 주시문서 목록에 추가하기',
'tog-watchdeletion'           => '내가 삭제하는 문서를 주시문서 목록에 추가하기',
'tog-minordefault'            => '‘사소한 편집’을 항상 선택하기',
'tog-previewontop'            => '편집상자 앞에 미리 보기 화면을 보여주기',
'tog-previewonfirst'          => '처음 편집할 때 미리 보기 화면을 보여주기',
'tog-nocache'                 => '문서 캐시 끄기',
'tog-enotifwatchlistpages'    => '주시문서가 바뀌면 이메일을 보내기',
'tog-enotifusertalkpages'     => '내 토론 문서가 바뀌면 이메일을 보내기',
'tog-enotifminoredits'        => '사소한 편집에도 이메일을 보내기',
'tog-enotifrevealaddr'        => '알림 메일에 내 이메일 주소를 밝히기',
'tog-shownumberswatching'     => '주시 사용자 수를 보여주기',
'tog-fancysig'                => '서명에 링크를 걸지 않기',
'tog-externaleditor'          => '외부 입력기를 기본값으로 사용하기 (숙련자용, 컴퓨터에 특별한 설정이 필요합니다)',
'tog-externaldiff'            => '외부 비교 툴을 기본값으로 사용하기 (숙련자용, 컴퓨터에 특별한 설정이 필요합니다)',
'tog-showjumplinks'           => '접근성을 위한 "이동" 링크 쓰기 (일부 스킨에서만 작동)',
'tog-uselivepreview'          => '실시간 미리 보기 사용하기 (자바스크립트, 실험적 기능)',
'tog-forceeditsummary'        => '편집 요약을 쓰지 않았을 때 알려주기',
'tog-watchlisthideown'        => '주시문서 목록에서 내 편집을 숨기기',
'tog-watchlisthidebots'       => '주시문서 목록에서 봇 편집을 숨기기',
'tog-watchlisthideminor'      => '주시문서 목록에서 사소한 편집을 숨기기',
'tog-ccmeonemails'            => '이메일을 보낼 때 내 이메일로 복사본을 보내기',
'tog-diffonly'                => '편집 차이를 비교할 때 문서 내용을 보여주지 않기',
'tog-showhiddencats'          => '숨겨진 분류 보기',

'underline-always'  => '항상',
'underline-never'   => '치지 않음',
'underline-default' => '브라우저 설정을 따르기',

'skinpreview' => '(미리 보기)',

# Dates
'sunday'        => '일요일',
'monday'        => '월요일',
'tuesday'       => '화요일',
'wednesday'     => '수요일',
'thursday'      => '목요일',
'friday'        => '금요일',
'saturday'      => '토요일',
'sun'           => '일',
'mon'           => '월',
'tue'           => '화',
'wed'           => '수',
'thu'           => '목',
'fri'           => '금',
'sat'           => '토',
'january'       => '1월',
'february'      => '2월',
'march'         => '3월',
'april'         => '4월',
'may_long'      => '5월',
'june'          => '6월',
'july'          => '7월',
'august'        => '8월',
'september'     => '9월',
'october'       => '10월',
'november'      => '11월',
'december'      => '12월',
'january-gen'   => '1월',
'february-gen'  => '2월',
'march-gen'     => '3월',
'april-gen'     => '4월',
'may-gen'       => '5월',
'june-gen'      => '6월',
'july-gen'      => '7월',
'august-gen'    => '8월',
'september-gen' => '9월',
'october-gen'   => '10월',
'november-gen'  => '11월',
'december-gen'  => '12월',
'jan'           => '1',
'feb'           => '2',
'mar'           => '3',
'apr'           => '4',
'may'           => '5',
'jun'           => '6',
'jul'           => '7',
'aug'           => '8',
'sep'           => '9',
'oct'           => '10',
'nov'           => '11',
'dec'           => '12',

# Categories related messages
'pagecategories'                 => '분류',
'category_header'                => '‘$1’ 분류에 속하는 문서',
'subcategories'                  => '하위 분류',
'category-media-header'          => '‘$1’ 분류에 속하는 자료',
'category-empty'                 => '이 분류에 속하는 문서나 자료가 없습니다.',
'hidden-categories'              => '숨겨진 분류',
'hidden-category-category'       => '숨겨진 분류', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|이 분류에는 다음의 하위 분류만이 속해 있습니다.|다음은 이 분류에 속하는 $2 개의 하위 분류 중 $1 개입니다.}}',
'category-subcat-count-limited'  => '이 분류에는 하위 분류가 $1개 있습니다.',
'category-article-count'         => '{{PLURAL:$2|이 분류에는 다음의 문서만이 속해 있습니다.|다음은 이 분류에 속하는 $2 개의 문서 중 $1 개입니다.}}',
'category-article-count-limited' => '이 분류에 $1개의 문서가 속해 있습니다.',
'category-file-count'            => '{{PLURAL:$2|이 분류에는 다음의 파일만이 속해 있습니다.|다음은 이 분류에 속하는 $2 개의 파일 중 $1 개입니다.}}',
'category-file-count-limited'    => '$1개의 파일이 이 분류에 있습니다.',
'listingcontinuesabbrev'         => ' (계속)',

'mainpagetext'      => "<big>'''미디어위키가 성공적으로 설치되었습니다.'''</big>",
'mainpagedocfooter' => '[http://meta.wikimedia.org/wiki/Help:Contents 이곳]에서 위키 프로그램에 대한 정보를 얻을 수 있습니다.

== 시작하기 ==

* [http://www.mediawiki.org/wiki/Manual:Configuration_settings 설정하기]
* [http://www.mediawiki.org/wiki/Manual:FAQ 미디어위키 FAQ]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce 미디어위키 발표 메일링 리스트]',

'about'          => '소개',
'article'        => '문서 내용',
'newwindow'      => '(새 창으로 열림)',
'cancel'         => '취소',
'qbfind'         => '찾기',
'qbbrowse'       => '탐색',
'qbedit'         => '편집',
'qbpageoptions'  => '문서 기능',
'qbpageinfo'     => '문서 정보',
'qbmyoptions'    => '내 문서',
'qbspecialpages' => '특수 문서',
'moredotdotdot'  => '더 보기...',
'mypage'         => '내 사용자 문서',
'mytalk'         => '내 사용자 토론',
'anontalk'       => '익명 사용자 토론',
'navigation'     => '둘러보기',
'and'            => '그리고',

# Metadata in edit box
'metadata_help' => '메타데이터:',

'errorpagetitle'    => '오류',
'returnto'          => '$1(으)로 돌아갑니다.',
'tagline'           => '{{SITENAME}}',
'help'              => '도움말',
'search'            => '찾기',
'searchbutton'      => '찾기',
'go'                => '가기',
'searcharticle'     => '가기',
'history'           => '문서 역사',
'history_short'     => '역사',
'updatedmarker'     => '마지막으로 방문한 후 변경됨',
'info_short'        => '정보',
'printableversion'  => '인쇄용 문서',
'permalink'         => '고유링크',
'print'             => '인쇄',
'edit'              => '편집',
'create'            => '생성',
'editthispage'      => '이 문서 편집하기',
'create-this-page'  => '이 문서 만들기',
'delete'            => '삭제',
'deletethispage'    => '이 문서 삭제하기',
'undelete_short'    => '$1개의 편집 되살리기',
'protect'           => '보호',
'protect_change'    => '보호 수준 변경',
'protectthispage'   => '이 문서 보호하기',
'unprotect'         => '보호 해제',
'unprotectthispage' => '이 문서 보호 해제하기',
'newpage'           => '새 문서',
'talkpage'          => '토론 문서',
'talkpagelinktext'  => '토론',
'specialpage'       => '특수 문서',
'personaltools'     => '개인 도구',
'postcomment'       => '의견 쓰기',
'articlepage'       => '문서 보기',
'talk'              => '토론',
'views'             => '보기',
'toolbox'           => '도구모음',
'userpage'          => '사용자 문서 보기',
'projectpage'       => '프로젝트 문서 보기',
'imagepage'         => '자료 문서 보기',
'mediawikipage'     => '메시지 문서 보기',
'templatepage'      => '틀 문서 보기',
'viewhelppage'      => '도움말 문서 보기',
'categorypage'      => '분류 문서 보기',
'viewtalkpage'      => '토론 보기',
'otherlanguages'    => '다른 언어',
'redirectedfrom'    => '($1에서 넘어옴)',
'redirectpagesub'   => '넘겨주기 문서',
'lastmodifiedat'    => '이 문서는 $2, $1에 마지막으로 바뀌었습니다.', # $1 date, $2 time
'viewcount'         => '이 문서는 총 $1번 읽혔습니다.',
'protectedpage'     => '보호된 문서',
'jumpto'            => '이동:',
'jumptonavigation'  => '둘러보기',
'jumptosearch'      => '찾기',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => '{{SITENAME}} 소개',
'aboutpage'            => 'Project:소개',
'bugreports'           => '버그 신고',
'bugreportspage'       => 'Project:버그 신고',
'copyright'            => '문서는 $1 라이센스 하에 있습니다.',
'copyrightpagename'    => '{{SITENAME}} 저작권',
'copyrightpage'        => '{{ns:project}}:저작권',
'currentevents'        => '요즘 화제',
'currentevents-url'    => 'Project:요즘 화제',
'disclaimers'          => '면책 조항',
'disclaimerpage'       => 'Project:면책 조항',
'edithelp'             => '편집 도움말',
'edithelppage'         => 'Help:편집하기',
'faq'                  => 'FAQ',
'faqpage'              => 'Project:FAQ',
'helppage'             => 'Help:목차',
'mainpage'             => '대문',
'mainpage-description' => '대문',
'policy-url'           => 'Project:정책',
'portal'               => '사용자 모임',
'portal-url'           => 'Project:사용자 모임',
'privacy'              => '개인정보 정책',
'privacypage'          => 'Project:개인정보 정책',

'badaccess'        => '권한 오류',
'badaccess-group0' => '요청한 동작을 실행할 권한이 없습니다.',
'badaccess-group1' => '요청한 동작에는 $1 권한이 필요합니다.',
'badaccess-group2' => '요청한 동작은 $1 중 하나의 권한을 가진 사용자에게만 가능합니다.',
'badaccess-groups' => '요청한 동작은 $1 중 하나의 권한을 가진 사용자에게만 가능합니다.',

'versionrequired'     => '미디어위키 $1 버전 필요',
'versionrequiredtext' => '이 문서를 사용하려면 $1 버전 미디어위키가 필요합니다. [[Special:Version|설치된 미디어위키 버전]]을 확인해주세요.',

'ok'                      => '확인',
'retrievedfrom'           => '원본 주소 ‘$1’',
'youhavenewmessages'      => '$1 란에 누군가 글을 남겼습니다. ($2)',
'newmessageslink'         => '사용자 토론',
'newmessagesdifflink'     => '바뀐 내용 비교',
'youhavenewmessagesmulti' => '$1 란에 누군가 글을 남겼습니다.',
'editsection'             => '편집',
'editold'                 => '편집',
'viewsourceold'           => '내용 보기',
'editsectionhint'         => '부분 편집: $1',
'toc'                     => '목차',
'showtoc'                 => '보이기',
'hidetoc'                 => '숨기기',
'thisisdeleted'           => '$1을 보거나 되살리겠습니까?',
'viewdeleted'             => '$1을 보겠습니까?',
'restorelink'             => '$1개의 삭제된 편집',
'feedlinks'               => '피드:',
'feed-invalid'            => '잘못된 구독 피드 방식입니다.',
'site-rss-feed'           => '$1 RSS 피드',
'site-atom-feed'          => '$1 Atom 피드',
'page-rss-feed'           => '‘$1’ RSS 피드',
'page-atom-feed'          => '"$1" Atom 피드',
'red-link-title'          => '$1 (없는 문서)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => '문서',
'nstab-user'      => '사용자 문서',
'nstab-media'     => '자료',
'nstab-special'   => '특수 문서',
'nstab-project'   => '프로젝트 문서',
'nstab-image'     => '그림',
'nstab-mediawiki' => '메시지',
'nstab-template'  => '틀',
'nstab-help'      => '도움말',
'nstab-category'  => '분류',

# Main script and global functions
'nosuchaction'      => '해당하는 동작이 없습니다.',
'nosuchactiontext'  => 'URL로 요청한 동작을 위키에서 판별할 수 없습니다.',
'nosuchspecialpage' => '해당하는 특수기능이 없습니다.',
'nospecialpagetext' => "<big>'''요청한 특수 문서가 존재하지 않습니다.'''</big>

특수 문서의 목록은 [[Special:SpecialPages|여기]]에서 볼 수 있습니다.",

# General errors
'error'                => '오류',
'databaseerror'        => '데이터베이스 오류',
'dberrortext'          => '데이터베이스 쿼리 구문 오류가 발생했습니다. 소프트웨어의 버그가 있을 수 있습니다. 마지막으로 요청한 데이터베이스 쿼리는 "<tt>$2</tt>" 함수에서 쓰인 <blockquote><tt>$1</tt></blockquote>입니다. MySQL은 "<tt>$3: $4</tt>" 오류를 냈습니다.',
'dberrortextcl'        => '데이터베이스 쿼리 구문 오류가 발생했습니다. 마지막으로 요청한 데이터베이스 쿼리는 "$2" 함수에서 쓰인 "$1"입니다. MySQL은 "$3: $4" 오류를 냈습니다.',
'noconnect'            => '위키의 기술적인 문제로 인해 데이터베이스 서버에 접근할 수 없습니다.<br />$1',
'nodb'                 => '$1 데이터베이스를 선택할 수 없습니다.',
'cachederror'          => '아래는 요청한 문서의 캐시 복사본으로, 최신이 아닐 수 있습니다.',
'laggedslavemode'      => '주의: 문서가 최근 것이 아닐 수 있습니다.',
'readonly'             => '데이터베이스 잠김',
'enterlockreason'      => '데이터베이스를 잠그는 이유와 예상되는 기간을 적어 주세요.',
'readonlytext'         => '데이터베이스가 잠겨 있어서 문서를 편집할 수 없습니다. 데이터베이스 관리가 끝난 후에는 정상으로 돌아올 것입니다.

관리자가 데이터베이스를 잠글 때 남긴 메시지는 다음과 같습니다: $1',
'missingarticle-diff'  => '(차이: $1, $2)',
'readonly_lag'         => '슬레이브 데이터베이스가 마스터 서버의 자료를 갱신하는 중입니다. 데이터베이스가 자동으로 잠겨 있습니다.',
'internalerror'        => '내부 오류',
'internalerror_info'   => '내부 오류: $1',
'filecopyerror'        => '‘$1’ 파일을 ‘$2’(으)로 복사할 수 없습니다.',
'filerenameerror'      => '‘$1’ 파일을 ‘$2’ 이름으로 바꿀 수 없습니다.',
'filedeleteerror'      => '‘$1’ 파일을 삭제할 수 없습니다.',
'directorycreateerror' => '‘$1’ 디렉토리를 만들 수 없습니다.',
'filenotfound'         => '‘$1’ 파일을 찾을 수 없습니다.',
'fileexistserror'      => '"$1" 파일에 쓸 수 없음: 파일이 존재함',
'unexpected'           => '예상되지 않은 값: "$1"="$2"',
'formerror'            => '오류: 양식을 제출할 수 없습니다.',
'badarticleerror'      => '지금의 명령은 이 문서에서는 실행할 수 없습니다.',
'cannotdelete'         => '문서나 파일을 삭제할 수 없습니다. 이미 삭제되었을 수도 있습니다.',
'badtitle'             => '잘못된 제목',
'badtitletext'         => '문서 제목이 잘못되었거나 비어있습니다.',
'perfdisabled'         => '이 기능은 위키가 사용 불가능할 정도로 데이터베이스를 느리게 만들기 때문에 임시적으로 사용이 중지되었습니다.',
'perfcached'           => '다음 자료는 캐시된 것이므로 현재 상황을 반영하지 않을 수 있습니다.',
'perfcachedts'         => '다음 자료는 캐시된 것으로, $1에 마지막으로 갱신되었습니다.',
'querypage-no-updates' => '이 문서의 갱신이 현재 비활성화되어 있습니다. 자료가 잠시 갱신되지 않을 것입니다.',
'wrong_wfQuery_params' => 'wfQuery()에서 잘못된 매개변수 발생<br />함수: $1<br />쿼리: $2',
'viewsource'           => '내용 보기',
'viewsourcefor'        => '$1의 문서 내용',
'actionthrottledtext'  => '스팸을 막기 위해 이 작업을 짧은 시간에 너무 많이 하는 것을 막고 있습니다.
제한을 넘었으니 몇 분 후에 새로 시도하세요.',
'protectedpagetext'    => '이 문서는 문서 편집이 불가능하도록 보호되어 있습니다.',
'viewsourcetext'       => '문서의 원본을 보거나 복사할 수 있습니다:',
'protectedinterface'   => '이 문서는 소프트웨어 인터페이스에 쓰이는 문서로, 잠겨 있습니다.',
'editinginterface'     => "'''경고''': 소프트웨어에서 사용하는 메시지 문서를 고치고 있습니다. 이것은 모든 사용자에게 영향을 끼칩니다. 번역되지 않은 메시지를 번역하려는 경우에는, [http://translatewiki.net/wiki/Main_Page?setlang=ko 베타위키(Betawiki)]에 참여하면 메시지 번역을 미디어위키에 직접 반영할 수 있습니다.",
'sqlhidden'            => '(SQL 쿼리 숨겨짐)',
'cascadeprotected'     => '이 문서는 "연쇄적 보호"된 문서에 포함되어 있어 보호됩니다. 연쇄적 보호된 문서: $2',
'namespaceprotected'   => "'''$1''' 네임스페이스를 편집할 수 있는 권한이 없습니다.",
'customcssjsprotected' => '이 문서는 다른 사용자의 개인 설정을 담고 있기 때문에, 편집할 권한이 없습니다.',
'ns-specialprotected'  => '{{ns:special}} 네임스페이스의 문서는 편집할 수 없습니다.',
'titleprotected'       => "[[User:$1|$1]] 사용자가 문서 생성을 금지했습니다.
이유는 다음과 같습니다. ‘''$2''’",

# Login and logout pages
'logouttitle'                => '로그아웃',
'logouttext'                 => '<strong>{{SITENAME}}에서 로그아웃했습니다.</strong>

이대로 이름 없이 {{SITENAME}}을(를) 이용하거나, 방금 사용했던 계정이나 다른 계정으로 다시 [[Special:UserLogin|로그인]]해서 이용할 수 있습니다.
웹 브라우저의 캐시를 지우지 않으면 몇몇 문서에서 로그인이 되어 있는 것처럼 보일 수 있다는 점을 유의해 주세요.',
'welcomecreation'            => '== $1 님, 환영합니다! ==
계정이 만들어졌습니다.
[[Special:Preferences|당신의 {{SITENAME}} 사용자 환경 설정]]을 바꿀 수 있습니다.',
'loginpagetitle'             => '로그인',
'yourname'                   => '계정 이름:',
'yourpassword'               => '비밀번호:',
'yourpasswordagain'          => '암호 확인',
'remembermypassword'         => '로그인 상태를 저장하기',
'yourdomainname'             => '도메인 이름',
'externaldberror'            => '외부 인증 데이터베이스에 오류가 있거나, 외부 계정을 갱신할 권한이 없습니다.',
'loginproblem'               => "'''로그인에 문제가 발생했습니다.'''<br />다시 시도해 보세요.",
'login'                      => '로그인',
'nav-login-createaccount'    => '로그인 / 계정 만들기',
'loginprompt'                => '{{SITENAME}}에 로그인하려면 쿠키를 사용할 수 있어야 합니다.',
'userlogin'                  => '로그인 / 계정 만들기',
'logout'                     => '로그아웃',
'userlogout'                 => '로그아웃',
'notloggedin'                => '로그인하고 있지 않음',
'nologin'                    => '계정이 없나요? $1.',
'nologinlink'                => '계정을 만들 수 있습니다',
'createaccount'              => '계정 만들기',
'gotaccount'                 => '계정이 이미 있다면, $1.',
'gotaccountlink'             => '로그인하세요',
'createaccountmail'          => '이메일로 보내기',
'badretype'                  => '암호가 서로 다릅니다.',
'userexists'                 => '입력하신 사용자 이름이 이미 등록되어 있습니다.
다른 이름으로 만들어주십시오.',
'youremail'                  => '이메일:',
'username'                   => '계정 이름:',
'uid'                        => '사용자 ID:',
'prefs-memberingroups'       => '가지고 있는 권한 목록:',
'yourrealname'               => '실명:',
'yourlanguage'               => '언어:',
'yournick'                   => '서명:',
'badsig'                     => '서명이 잘못되었습니다. HTML 태그를 확인해주세요.',
'badsiglength'               => '서명이 너무 깁니다. 서명은 $1자보다 짧아야 합니다.',
'email'                      => '이메일',
'prefs-help-realname'        => '실명 기입은 자유입니다. 실명을 입력할 경우 문서 기여에 자신의 이름이 들어가게 됩니다.',
'loginerror'                 => '로그인 오류',
'prefs-help-email'           => '이메일 주소 입력은 선택이지만, 비밀번호를 잊었을 때 찾기 위해 이메일 주소가 필요할 수 있습니다.
당신의 메일 주소를 공개하지 않고 사용자 및 사용자토론 문서를 통해 다른 사용자들과 이야기를 할 수 있습니다.',
'prefs-help-email-required'  => '이메일 주소가 필요합니다.',
'nocookiesnew'               => '사용자 계정을 만들었지만, 아직 로그인하지 않았습니다. {{SITENAME}}에서는 로그인 정보를 저장하기 위해 쿠키를 사용합니다. 지금 사용하는 웹 브라우저는 쿠키를 사용하지 않도록 설정되어 있습니다. 로그인하기 전에 웹 브라우저에서 쿠키를 사용하도록 설정해주세요.',
'nocookieslogin'             => '{{SITENAME}}에서는 로그인을 위해 쿠키를 사용합니다. 당신의 웹 브라우저에서 쿠키가 비활성되어 있습니다. 쿠키 사용을 활성화한 다음 로그인해 주세요.',
'noname'                     => '사용자 이름이 올바르지 않습니다.',
'loginsuccesstitle'          => '로그인 성공',
'loginsuccess'               => "'''{{SITENAME}}에 ‘$1’ 계정으로 로그인했습니다.'''",
'nosuchuser'                 => '"$1" 사용자가 존재하지 않습니다. 철자가 맞는지 확인해주세요. 또는 [[Special:Userlogin/signup|새 계정을 만들 수 있습니다]].',
'nosuchusershort'            => '‘<nowiki>$1</nowiki>’ 사용자가 존재하지 않습니다. 철자가 맞는지 확인해 주세요.',
'nouserspecified'            => '사용자 이름을 입력하지 않았습니다.',
'wrongpassword'              => '암호가 틀립니다. 다시 시도해 주세요.',
'wrongpasswordempty'         => '비밀번호가 비었습니다. 다시 시도해 주세요.',
'passwordtooshort'           => '암호가 부적절하거나 너무 짧습니다.
암호 길이는 적어도 $1자보다 길고 사용자 이름과 달라야 합니다.',
'mailmypassword'             => '새 암호를 이메일로 보내기',
'passwordremindertitle'      => '{{SITENAME}}에서 보내는 새 임시 암호',
'passwordremindertext'       => 'IP 주소 $1에게서 당신에게 {{SITENAME}}($4)의 새 암호를 보내달라는 요청이 왔습니다.
사용자 ‘$2’의 암호는 이제 ‘$3’입니다. 로그인한 다음 암호를 변경해 주세요.

만약 이 암호 요청을 다른 사람이 보낸 것이거나, 또는 예전에 사용하던 암호를 기억해냈다면 이 메시지를 무시하고 예전 암호를 계속 사용할 수 있습니다.',
'noemail'                    => '사용자 ‘$1’에 등록된 이메일 주소가 없습니다.',
'passwordsent'               => '‘$1’ 계정의 새로운 암호를 이메일로 보냈습니다. 암호를 받고 다시 로그인해 주세요.',
'blocked-mailpassword'       => '당신의 IP 주소는 편집이 차단되어 있습니다. 따라서 암호 되살리기 기능은 악용 방지를 위해 금지됩니다.',
'eauthentsent'               => '확인 이메일을 보냈습니다. 이메일 내용의 지시대로 계정 확인 절차를 실행해 주십시오.',
'throttled-mailpassword'     => '비밀번호 확인 이메일을 이미 최근 $1시간 안에 보냈습니다. 악용을 방지하기 위해 비밀번호 확인은 $1시간마다 하나씩만 보낼 수 있습니다.',
'mailerror'                  => '메일 보내기 오류: $1',
'acct_creation_throttle_hit' => '당신은 이미 $1개의 계정이 있습니다. 더 이상 만들 수 없습니다.',
'emailauthenticated'         => '당신의 이메일 주소는 $1에 인증되었습니다.',
'emailnotauthenticated'      => '이메일 주소를 인증하지 않았습니다. 이메일 확인 절차를 거치지 않으면 다음 이메일 기능을 사용할 수 없습니다.',
'noemailprefs'               => '이 기능을 사용하기 위해서는 이메일 주소를 기입해야 합니다.',
'emailconfirmlink'           => '이메일 주소 확인',
'invalidemailaddress'        => '이메일 주소 입력이 잘못되어 인식할 수 없습니다.
정상적인 이메일을 입력하거나, 칸을 비워 주세요.',
'accountcreated'             => '계정 만들어짐',
'accountcreatedtext'         => '‘$1’ 사용자 계정이 만들어졌습니다.',
'createaccount-title'        => '{{SITENAME}} 계정 만들기',
'createaccount-text'         => '{{SITENAME}}($4)에서 계정 이름 ‘$2’, 비밀번호 ‘$3’으로 계정을 만들었습니다. 로그인을 한 다음 암호를 바꾸어 주세요.

계정을 잘못 만들었다면 이 메시지는 무시해도 됩니다.',
'loginlanguagelabel'         => '언어: $1',

# Password reset dialog
'resetpass'               => '계정 비밀번호 초기화',
'resetpass_announce'      => '이메일로 받은 임시 암호로 로그인했습니다. 로그인을 마치려면 새 암호를 입력해 주세요:',
'resetpass_header'        => '암호 재설정',
'resetpass_submit'        => '암호를 변경하고 로그인하기',
'resetpass_success'       => '암호가 성공적으로 변경되었습니다! 로그인을 해 주세요.',
'resetpass_bad_temporary' => '임시 암호가 잘못되었습니다. 암호를 이미 바꾸었거나, 임시 암호를 새로 받았을 수 있습니다.',
'resetpass_forbidden'     => '{{SITENAME}}에서 암호 변경 불가',
'resetpass_missing'       => '입력값 없음.',

# Edit page toolbar
'bold_sample'     => '굵은 글씨',
'bold_tip'        => '굵은 글씨',
'italic_sample'   => '기울인 글씨',
'italic_tip'      => '기울인 글씨',
'link_sample'     => '링크 제목',
'link_tip'        => '내부 링크',
'extlink_sample'  => 'http://www.example.com 사이트 이름',
'extlink_tip'     => '외부 사이트 링크 (앞에 http://를 붙여야 합니다.)',
'headline_sample' => '제목',
'headline_tip'    => '2단계 문단 제목',
'math_sample'     => '여기에 수식을 쓰세요',
'math_tip'        => '수식 (LaTeX)',
'nowiki_sample'   => '여기에 위키 문법을 사용하지 않을 글을 적어 주세요',
'nowiki_tip'      => '위키 문법 무시하기',
'image_tip'       => '그림 넣기',
'media_tip'       => '파일 링크하기',
'sig_tip'         => '내 서명과 현재 시각',
'hr_tip'          => '가로줄(되도록 사용하지 말아 주세요)',

# Edit pages
'summary'                          => '편집 요약',
'subject'                          => '주제/제목',
'minoredit'                        => '사소한 편집',
'watchthis'                        => '이 문서 주시하기',
'savearticle'                      => '저장',
'preview'                          => '미리 보기',
'showpreview'                      => '미리 보기',
'showlivepreview'                  => '실시간 미리 보기',
'showdiff'                         => '차이 보기',
'anoneditwarning'                  => "'''주의''': 로그인하고 있지 않습니다. 당신의 IP 주소가 문서 역사에 남게 됩니다.",
'missingsummary'                   => "'''알림:''' 편집 요약을 적지 않았습니다. 그대로 저장하면 편집 요약 없이 저장됩니다.",
'missingcommenttext'               => '아래에 내용을 채워 넣어 주세요.',
'summary-preview'                  => '편집 요약 미리 보기',
'subject-preview'                  => '주제/제목 미리 보기',
'blockedtitle'                     => '차단됨',
'blockedtext'                      => "<big>'''당신의 계정 혹은 IP 주소가 차단되었습니다.'''</big>

차단한 사람은 $1이고, 차단한 이유는 다음과 같습니다: $2

* 차단이 시작된 시간: $8
* 차단이 만료되는 시간: $6
* 차단된 사용자: $7

$1, 또는 [[{{MediaWiki:Grouppage-sysop}}|다른 관리자]]에게 차단에 대해 문의할 수 있습니다. [[Special:Preferences|계정 환경 설정]]에 올바른 이메일 주소가 있어야만 '이메일 보내기' 기능을 사용할 수 있습니다. 또한 이메일 보내기 기능이 차단되어 있으면 이메일을 보낼 수 없습니다.
당신의 현재 IP 주소는 $3이고, 차단 ID는 #$5입니다.
문의할 때에 이 정보를 같이 알려주세요.",
'autoblockedtext'                  => '당신의 IP 주소는 $1이 차단한 사용자가 사용했던 IP이기 때문에 자동으로 차단되었습니다. 차단된 이유는 다음과 같습니다:

:$2

* 차단이 시작된 시간: $8
* 차단이 만료되는 시간: $6
* 차단된 사용자: $7

$1, 또는 [[{{MediaWiki:Grouppage-sysop}}|다른 관리자]]에게 차단에 대해 문의할 수 있습니다.

[[Special:Preferences|계정 환경 설정]]에 올바른 이메일 주소가 있어야만 "이메일 보내기" 기능을 사용할 수 있습니다. 또한 이메일 보내기 기능이 차단되어 있으면 이메일을 보낼 수 없습니다.

당신의 현재 IP 주소는 $3이며, 차단 ID는 #$5입니다. 문의할 때에 이 정보를 같이 알려주세요.',
'blockednoreason'                  => '이유를 입력하지 않음',
'blockedoriginalsource'            => "아래에 '''$1'''의 내용이 나와 있습니다:",
'blockededitsource'                => "아래에 '''$1'''에서의 '''당신의 편집'''이 나와 있습니다:",
'whitelistedittitle'               => '편집하려면 로그인 필요',
'whitelistedittext'                => '문서를 편집하려면 $1해야 합니다.',
'confirmedittitle'                 => '편집하려면 이메일 인증 필요',
'confirmedittext'                  => '문서를 고치려면 이메일 인증 절차가 필요합니다. [[Special:Preferences|사용자 환경 설정]]에서 이메일 주소를 입력하고 이메일 주소 인증을 해 주시기 바랍니다.',
'nosuchsectiontitle'               => '해당 부분 없음',
'nosuchsectiontext'                => '편집하려는 문단이 존재하지 않습니다. $1번째 문단이 없기 때문에 편집한 내용을 저장할 수 없습니다.',
'loginreqtitle'                    => '로그인 필요',
'loginreqlink'                     => '로그인',
'loginreqpagetext'                 => '다른 문서를 보기 위해서는 $1해야 합니다.',
'accmailtitle'                     => '암호를 보냈습니다.',
'accmailtext'                      => '‘$1’의 암호를 $2로 보냈습니다.',
'newarticle'                       => '(새 문서)',
'newarticletext'                   => "이 문서는 아직 만들어지지 않았습니다. 새 문서를 만들려면 아래의 상자에 문서 내용을 입력하면 됩니다(자세한 내용은 [[{{MediaWiki:Helppage}}|도움말]]을 읽어 주세요).
만약 잘못 찾아온 문서라면, 웹 브라우저의 '''뒤로''' 버튼을 눌러 주세요.",
'anontalkpagetext'                 => '----
여기는 계정을 만들지 않았거나 사용하고 있지 않은 익명 사용자를 위한 토론 문서입니다. 익명 사용자를 구별하기 위해서는 숫자로 된 IP 주소를 사용해야만 합니다. IP 주소는 여러 사용자에 의해 공유될 수 있습니다. 자신과 관계없는 의견이 자신에게 남겨져 있어 불쾌하다고 생각하는 익명 사용자에게는 [[Special:UserLogin/signup|계정을 만들고]] [[Special:UserLogin|로그인 하여]] 미래의 다른 익명 사용자에게 올 수 있는 혼란을 줄이는 것을 추천합니다.',
'noarticletext'                    => '현재 문서는 비어 있습니다. 이 제목으로 [[Special:Search/{{PAGENAME}}|검색]]하거나 문서를 [{{fullurl:{{FULLPAGENAME}}|action=edit}} 편집]할 수 있습니다.',
'userpage-userdoesnotexist'        => '‘$1’ 계정은 등록되어 있지 않습니다. 이 문서를 만들거나 편집하려면 계정이 존재 하는지 확인해주세요.',
'clearyourcache'                   => "'''참고 - 설정을 저장한 후에 바뀐 점을 확인하기 위해서는 브라우저의 캐시를 갱신해야 합니다.'''

'''모질라 / 파이어폭스 / 사파리''': Shift 키를 누르면서 새로 고침을 클릭하거나, Ctrl-F5 또는 Ctrl-R 을 입력 (매킨토시에서는 Command-R); '''컨커러''': 새로고침을 클릭하거나 F5를 입력; '''오페라''': 도구→설정에서 캐시를 비움; '''인터넷 익스플로러''': Ctrl 키를 누르면서 새로 고침을 클릭하거나, Ctrl-F5를 입력.",
'usercssjsyoucanpreview'           => "'''안내''': CSS/JS 문서를 저장하기 전에 ‘미리 보기’ 기능을 통해 작동을 확인해주세요.",
'usercsspreview'                   => "'''이것은 사용자 CSS의 미리 보기이며, 아직 저장하지 않았다는 것을 주의해 주세요!'''",
'userjspreview'                    => "'''이것은 자바스크립트 미리 보기로, 아직 저장하지 않았다는 것을 주의해 주세요!'''",
'userinvalidcssjstitle'            => "'''경고''': ‘$1’ 스킨이 존재하지 않습니다. .css와 .js 문서의 제목은 {{ns:user}}:Foo/monobook.css처럼 소문자로 써야 합니다. {{ns:user}}:Foo/Monobook.css와 같이 대문자로 쓸 경우 작동하지 않습니다.",
'updated'                          => '(바뀜)',
'note'                             => '<strong>주의:</strong>',
'previewnote'                      => '<strong>지금 미리 보기로 보고 있는 내용은 아직 저장되지 않았습니다!</strong>',
'previewconflict'                  => '이 미리 보기는 저장할 때의 모습, 즉 위쪽 편집창의 문서를 반영합니다.',
'session_fail_preview'             => '<strong>세션 데이터가 없어져 편집을 저장하지 못했습니다.
다시 시도해도 되지 않으면 [[Special:UserLogout|로그아웃]]한 다음 다시 로그인해 보십시오.</strong>',
'session_fail_preview_html'        => '<strong>세션 데이터가 없어져 편집을 저장하지 못했습니다.</strong>

{{SITENAME}}에서 HTML 입력을 허용하기 때문에, 자바스크립트 공격을 막기 위해 미리 보기는 숨겨져 있습니다.

<strong>다시 시도해도 되지 않으면 [[Special:UserLogout|로그아웃]]한 다음 다시 로그인을 해 보십시오.</strong>',
'editing'                          => '$1 편집하기',
'editingsection'                   => '$1 편집하기 (부분)',
'editingcomment'                   => '$1 편집하기 (덧붙이기)',
'editconflict'                     => '편집 충돌: $1',
'explainconflict'                  => "문서를 편집하는 도중에 누군가가 이 문서를 바꾸었습니다.
위쪽에 있는 문서가 현재 바뀐 문서이고, 아래쪽의 문서가 당신이 편집한 문서입니다.
아래쪽의 내용을 위쪽에 적절히 합쳐 주시기 바랍니다.
'''위쪽의 편집 내역만이''' 저장됩니다.",
'yourtext'                         => '당신의 편집',
'storedversion'                    => '현재 문서',
'nonunicodebrowser'                => "'''주의: 당신의 웹 브라우저가 유니코드를 완벽하게 지원하지 않습니다. 몇몇 문자가 16진수 코드로 나타날 수 있습니다.'''",
'editingold'                       => '<strong>경고:
지금 옛날 버전의 문서를 고치고 있습니다.
이것을 저장하면 최근에 편집된 부분이 사라질 수 있습니다.</strong>',
'yourdiff'                         => '차이',
'copyrightwarning'                 => "{{SITENAME}}에서의 모든 기여는 $2 라이센스에 따라 배포된다는 점을 유의해 주세요($1에서 자세한 사항을 읽어 주세요). 만약 이에 대해 찬성하지 않는다면, 문서를 저장하지 말아 주세요.<br />
또한 당신의 기여는 직접 작성했거나, 또는 퍼블릭 도메인과 같은 자유 문서에서 가져왔다는 것을 보증해야 합니다.
'''저작권이 있는 내용을 허가 없이 저장하지 마세요!'''",
'copyrightwarning2'                => "{{SITENAME}}에서의 모든 기여는 다른 사용자가 편집, 수정, 삭제할 수 있다는 점을 유의해 주세요. 만약 이에 대해 찬성하지 않는다면, 문서를 저장하지 말아 주세요.<br />
또한 당신의 기여는 직접 작성했거나, 또는 퍼블릭 도메인과 같은 자유 문서에서 가져왔다는 것을 보증해야 합니다($1에서 자세한 사항을 읽어 주세요).
'''저작권이 있는 내용을 허가 없이 저장하지 마세요!'''",
'longpagewarning'                  => "'''주의: 이 문서의 용량이 $1킬로바이트입니다. 몇몇 웹 브라우저에서는 32킬로바이트 이상의 문서를 편집할 때 문제가 발생할 수 있습니다. 만약의 경우를 대비하여, 문서를 여러 문단으로 나누어서 편집할 수 있습니다.'''",
'longpageerror'                    => "'''오류: 문서의 크기가 $1킬로바이트로, 최대 가능한 크기인 $2킬로바이트보다 큽니다. 저장할 수 없습니다.'''",
'readonlywarning'                  => "'''주의: 데이터베이스가 관리를 위해 잠겨 있습니다. 따라서 문서를 편집한 내용을 지금 저장할 수 없습니다. 편집 내용을 일단 다른 곳에 저장한 후, 나중에 다시 시도해 주세요.'''",
'protectedpagewarning'             => "'''경고: 이 문서는 관리자만이 편집할 수 있도록 보호되어 있습니다.'''",
'semiprotectedpagewarning'         => "'''주의''': 이 문서는 등록된 사용자만이 편집할 수 있도록 잠겨 있습니다.",
'cascadeprotectedwarning'          => "'''주의''': 이 문서는 자동으로 잠겨 있어 관리자만 편집할 수 있습니다. 연쇄적 보호가 걸린 다음의 문서에서 이 문서를 사용하고 있습니다:",
'titleprotectedwarning'            => '<strong>주의: 이 문서는 잠겨 있어, 일부 사용자만이 생성할 수 있습니다.</strong>',
'templatesused'                    => '이 문서에서 사용한 틀:',
'templatesusedpreview'             => '이 미리 보기에서 사용하고 있는 틀:',
'templatesusedsection'             => '이 부분에서 사용하고 있는 틀:',
'template-protected'               => '(보호됨)',
'template-semiprotected'           => '(준보호됨)',
'hiddencategories'                 => '이 문서는 아래의 $1개의 숨겨진 분류에 속해 있습니다.',
'edittools'                        => '<!-- 이 문서는 편집 창과 파일 올리기 창에 출력됩니다. -->',
'nocreatetitle'                    => '문서 생성 제한',
'nocreatetext'                     => '{{SITENAME}}에서 새로운 문서를 생성하는 것은 제한되어 있습니다. 이미 존재하는 다른 문서를 편집하거나, [[Special:UserLogin|로그인하거나 계정을 만들 수 있습니다]].',
'nocreate-loggedin'                => '{{SITENAME}}에서 새 문서를 만들 권한이 없습니다.',
'permissionserrors'                => '권한 오류',
'permissionserrorstext'            => '해당 명령을 수행할 권한이 없습니다. 다음의 이유를 확인해보세요:',
'permissionserrorstext-withaction' => '$2 명령을 수행할 권한이 없습니다. 다음의 이유를 확인해보세요:',
'recreate-deleted-warn'            => "'''주의: 과거에 삭제된 문서를 다시 만들고 있습니다.'''

이 문서를 계속 편집하는 것이 적합한 것인지 확인해주세요.
과거 삭제 기록은 다음과 같습니다:",

# "Undo" feature
'undo-success' => '이 편집을 되돌리려면 아래의 변경되는 사항을 확인한 후 저장해주세요.',
'undo-failure' => '중간의 다른 편집과 충돌하여 이 편집을 되돌릴 수 없습니다.',
'undo-norev'   => '문서가 없거나 삭제되었기 때문에, 편집을 되돌릴 수 없습니다.',
'undo-summary' => '[[Special:Contributions/$2|$2]] ([[User talk:$2|토론]]) 의 $1 판 편집을 되돌림',

# Account creation failure
'cantcreateaccounttitle' => '계정을 만들 수 없음',
'cantcreateaccount-text' => "현재 아이피 주소('''$1''')는 [[User:$3|$3]] 사용자에 의해 계정 생성이 차단된 상태입니다.

차단 이유는 다음과 같습니다: $2",

# History pages
'viewpagelogs'        => '이 문서의 기록 보기',
'nohistory'           => '이 문서는 편집 역사가 없습니다.',
'revnotfound'         => '버전 없음',
'revnotfoundtext'     => '문서의 해당 버전을 찾지 못했습니다. 접속 URL을 확인해 주세요.',
'currentrev'          => '현재 판',
'revisionasof'        => '$1 판',
'revision-info'       => '$2 사용자의 $1 버전',
'previousrevision'    => '← 이전 판',
'nextrevision'        => '다음 판 →',
'currentrevisionlink' => '현재 판',
'cur'                 => '현재',
'next'                => '다음',
'last'                => '이전',
'page_first'          => '처음',
'page_last'           => '마지막',
'histlegend'          => '비교하려는 판들을 선택한 아래 버튼을 누르세요.<br />설명: (현재) = 현재 판과의 차이, (이전) = 바로 이전 판과의 차이, 잔글 = 사소한 편집',
'deletedrev'          => '[삭제됨]',
'histfirst'           => '처음',
'histlast'            => '마지막',
'historysize'         => '($1 바이트)',
'historyempty'        => '(비었음)',

# Revision feed
'history-feed-title'          => '편집 역사',
'history-feed-description'    => '이 문서의 편집 역사',
'history-feed-item-nocomment' => '$2에 대한 $1의 편집', # user at time
'history-feed-empty'          => '요청한 문서가 존재하지 않습니다.
해당 문서가 삭제되었거나, 문서 이름이 바뀌었을 수 있습니다.
[[Special:Search|위키 검색 기능]]을 이용해 관련 문서를 찾아보세요.',

# Revision deletion
'rev-delundel'      => '보이기/숨기기',
'revisiondelete'    => '버전 삭제/복구',
'pagehist'          => '문서 역사',
'deletedhist'       => '삭제된 역사',
'revdelete-content' => '내용',

# Merge log
'mergelog' => '합병 기록',

# Diffs
'history-title'           => '"$1" 문서의 변경 내력',
'difference'              => '(버전 사이의 차이)',
'lineno'                  => '$1번째 줄:',
'compareselectedversions' => '선택된 버전들을 비교하기',
'editundo'                => '편집 취소',
'diff-multi'              => '(중간 $1개의 편집이 숨겨짐)',

# Search results
'searchresults'            => '검색 결과',
'searchresulttext'         => '{{SITENAME}}의 찾기 기능에 대한 자세한 정보는 [[{{MediaWiki:Helppage}}|{{int:help}}]] 문서를 참고해주세요.',
'searchsubtitle'           => "'''[[:$1]]''' 문서를 검색하고 있습니다. ([[Special:Prefixindex/$1|이름이 ‘$1’(으)로 시작하는 문서 목록]] | [[Special:WhatLinksHere/$1|‘$1’ 문서를 가리키는 문서 목록]])",
'searchsubtitleinvalid'    => "검색 단어 '''$1'''",
'noexactmatch'             => "'''$1 문서가 없습니다.''' 문서를 [[:$1|만들 수]] 있습니다.",
'noexactmatch-nocreate'    => "'''\"\$1\" 문서가 존재하지 않습니다.'''",
'titlematches'             => '문서 제목 일치',
'notitlematches'           => '해당하는 제목 없음',
'textmatches'              => '문서 내용 일치',
'notextmatches'            => '해당하는 문서 없음',
'prevn'                    => '이전 $1개',
'nextn'                    => '다음 $1개',
'viewprevnext'             => '보기: ($1) ($2) ($3).',
'search-result-size'       => '$1 ($2개 단어)',
'search-result-score'      => '유사도: $1%',
'search-interwiki-caption' => '자매 프로젝트',
'search-interwiki-default' => '$1 결과:',
'search-interwiki-more'    => '(더 보기)',
'mwsuggest-disable'        => 'AJAX 추천 기능 끄기',
'showingresults'           => '<strong>$2</strong>번 부터 <strong>$1</strong>개의 결과입니다.',
'showingresultsnum'        => "'''$2'''번 부터 '''$3'''개의 결과입니다.",
'nonefound'                => "'''참고''': 몇개의 이름공간만 기본 검색 범위입니다. 토론이나 틀 등의 모든 자료를 검색하기 위해서는 접두어로 '''all:''' 어떤 이름공간을 위해서는 접두어로 그 이름공간을 쓸 수 있습니다.",
'powersearch'              => '찾기',
'powersearch-legend'       => '고급 검색',
'powersearch-ns'           => '다음의 이름공간에서 찾기:',
'powersearch-redir'        => '넘겨주기 목록',
'powersearch-field'        => '검색',
'searchdisabled'           => '{{SITENAME}} 검색 기능이 비활성화되어 있습니다. 기능이 작동하지 않는 동안에는 구글(Google)을 이용해 검색할 수 있습니다. 검색 엔진의 내용은 최근 것이 아닐 수 있다는 점을 주의해주세요.',

# Preferences page
'preferences'              => '사용자 환경 설정',
'mypreferences'            => '사용자 환경 설정',
'prefs-edits'              => '편집 횟수:',
'prefsnologin'             => '로그인하지 않음',
'prefsnologintext'         => '사용자 환경 설정을 바꾸려면 먼저 <span class="plainlinks">[{{fullurl:Special:Userlogin|returnto=$1}} 로그인]</span>해야 합니다.',
'prefsreset'               => '사용자 환경 설정을 기본값으로 되돌렸습니다.',
'qbsettings'               => '빨리가기 맞춤',
'qbsettings-none'          => '없음',
'qbsettings-fixedleft'     => '왼쪽 고정',
'qbsettings-fixedright'    => '오른쪽 고정',
'qbsettings-floatingleft'  => '왼쪽 유동',
'qbsettings-floatingright' => '오른쪽 유동',
'changepassword'           => '암호 바꾸기',
'skin'                     => '스킨',
'math'                     => '수식',
'dateformat'               => '날짜 형식',
'datedefault'              => '기본값',
'datetime'                 => '날짜와 시각',
'math_failure'             => '해석 실패',
'math_unknown_error'       => '알 수 없는 오류',
'math_unknown_function'    => '알 수 없는 함수',
'math_lexing_error'        => '어휘 오류',
'math_syntax_error'        => '구문 오류',
'math_image_error'         => 'PNG 변환 실패 - latex, dvips, gs, convert가 올바르게 설치되어 있는지 확인해 주세요.',
'math_bad_tmpdir'          => '수식을 임시 폴더에 저장하거나 폴더를 만들 수 없습니다.',
'math_bad_output'          => '수식을 출력 폴더에 저장하거나 폴더를 만들 수 없습니다.',
'math_notexvc'             => '실행할 수 있는 texvc이 없습니다. 설정을 위해 math/README를 읽어 주세요.',
'prefs-personal'           => '사용자 정보',
'prefs-rc'                 => '최근 바뀜',
'prefs-watchlist'          => '주시문서 목록',
'prefs-watchlist-days'     => '주시문서 목록에서 보여줄 날짜 수:',
'prefs-watchlist-edits'    => '주시문서 목록에 보이는 편집 갯수:',
'prefs-misc'               => '기타',
'saveprefs'                => '저장',
'resetprefs'               => '저장하지 않은 설정 되돌리기',
'oldpassword'              => '예전 암호:',
'newpassword'              => '새 암호:',
'retypenew'                => '새 암호 확인:',
'textboxsize'              => '편집상자 크기',
'rows'                     => '줄 수:',
'columns'                  => '열:',
'searchresultshead'        => '찾기',
'resultsperpage'           => '쪽마다 보이는 결과 수:',
'contextlines'             => '결과마다 보이는 줄 수:',
'contextchars'             => '각 줄에 보이는 글 수:',
'stub-threshold'           => '링크를 <a href="#" class="stub">토막글 링크</a>로 표시할 문서 크기(바이트 수):',
'recentchangesdays'        => '최근 바뀜에 표시할 날짜 수:',
'recentchangescount'       => '최근 바뀜, 역사, 기록 보기에 표시할 문서 수:',
'savedprefs'               => '설정을 저장했습니다.',
'timezonelegend'           => '시간대',
'timezonetext'             => '¹현지 시각과 서버 시각(UTC) 사이의 시차를 써 주세요.',
'localtime'                => '현지 시각',
'timezoneoffset'           => '시차¹',
'servertime'               => '서버 시각',
'guesstimezone'            => '웹 브라우저 설정에서 가져오기',
'allowemail'               => '다른 사용자로부터의 이메일 허용',
'prefs-searchoptions'      => '검색 설정',
'prefs-namespaces'         => '네임스페이스',
'defaultns'                => '기본으로 다음의 이름공간에서 찾기:',
'default'                  => '기본값',
'files'                    => '파일',

# User rights
'userrights'                  => '사용자 권한 관리', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => '사용자 권한 관리',
'userrights-user-editname'    => '계정 이름:',
'editusergroup'               => '사용자 그룹 편집',
'editinguser'                 => "'''[[User:$1|$1]]'''([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]]) 사용자의 권한 변경",
'userrights-editusergroup'    => '사용자 그룹 편집',
'saveusergroups'              => '사용자 권한 저장',
'userrights-groupsmember'     => '현재 권한:',
'userrights-groups-help'      => '이 사용자의 권한을 바꿀 수 있습니다.
* 사용자는 체크 표시가 있는 권한을 갖습니다.
* 사용자는 체크 표시가 없는 권한을 갖지 않습니다.
* <nowiki>*</nowiki>표시는 권한을 주거나 거두는 것 중 하나만 할 수 있다는 뜻입니다.',
'userrights-reason'           => '바꾸는 이유:',
'userrights-no-interwiki'     => '다른 위키의 사용자 권한을 바꿀 권한이 없습니다.',
'userrights-nologin'          => '사용자의 권한을 바꾸기 위해서는 반드시 관리자 계정으로 [[Special:UserLogin|로그인]]해야 합니다.',
'userrights-notallowed'       => '다른 사용자의 권한을 조정할 권한이 없습니다.',
'userrights-changeable-col'   => '바꿀 수 있는 권한',
'userrights-unchangeable-col' => '바꿀 수 없는 권한',

# Groups
'group'               => '권한:',
'group-user'          => '사용자',
'group-autoconfirmed' => '자동 인증된 사용자',
'group-bot'           => '봇',
'group-sysop'         => '관리자',
'group-bureaucrat'    => '뷰로크랫',
'group-all'           => '(모두)',

'group-user-member'       => '사용자',
'group-bot-member'        => '봇',
'group-sysop-member'      => '관리자',
'group-bureaucrat-member' => '뷰로크랫',

'grouppage-user'          => '{{ns:project}}:일반 사용자',
'grouppage-autoconfirmed' => '{{ns:project}}:자동 인증된 사용자',
'grouppage-bot'           => '{{ns:project}}:봇',
'grouppage-sysop'         => '{{ns:project}}:관리자',
'grouppage-bureaucrat'    => '{{ns:project}}:뷰로크랫',

# Rights
'right-read'             => '문서 읽기',
'right-edit'             => '문서 편집',
'right-createpage'       => '문서 생성 (토론 문서 제외)',
'right-createtalk'       => '토론 문서 생성',
'right-createaccount'    => '새 계정 만들기',
'right-minoredit'        => '사소한 편집 사용 가능',
'right-move'             => '문서 이동',
'right-move-subpages'    => '문서와 하위 문서 이동하기',
'right-suppressredirect' => '문서 이동시에 옛날 이름에 넘겨주기를 남기지 않음',
'right-upload'           => '파일 올리기',
'right-reupload'         => '이미 존재하는 파일 덮어쓰기',
'right-reupload-own'     => '자신이 이미 올린 파일 덮어쓰기',
'right-upload_by_url'    => 'URL 주소에서 파일 올리기',
'right-autoconfirmed'    => '준보호된 문서 편집',
'right-bot'              => '봇의 편집으로 취급',
'right-apihighlimits'    => 'API 상한 상승',
'right-writeapi'         => 'API 작성',
'right-delete'           => '문서 삭제',
'right-bigdelete'        => '문서 역사가 긴 문서를 삭제',
'right-deleterevision'   => '문서의 특정 판을 삭제 및 복구',
'right-browsearchive'    => '삭제된 문서 검색',
'right-undelete'         => '삭제된 문서 복구',
'right-suppressrevision' => '관리자도 보지 못하도록 숨겨진 판의 확인 및 복구',
'right-block'            => '다른 사용자를 편집을 못하도록 차단',
'right-blockemail'       => '다른 사용자가 이메일을 보내지 못하도록 차단',
'right-protect'          => '보호 수준 변경 및 보호된 문서 편집',
'right-editusercssjs'    => '다른 사용자의 CSS와 JS 문서를 편집',
'right-markbotedits'     => '되돌리기를 봇의 편집으로 취급 가능',
'right-import'           => '다른 위키에서 문서를 가져오기',
'right-patrol'           => '다른 사용자의 편집을 검토',
'right-mergehistory'     => '문서의 역사를 합침',
'right-userrights'       => '모든 사용자의 권한 수정',
'right-siteadmin'        => '데이터베이스를 잠그거나 잠금 해제',

# User rights log
'rightslog'      => '사용자 권한 기록',
'rightslogtext'  => '사용자 권한 조정 기록입니다.',
'rightslogentry' => '$1의 권한을 $2에서 $3으로 변경',
'rightsnone'     => '(없음)',

# Recent changes
'nchanges'                          => '$1개 바뀜',
'recentchanges'                     => '최근 바뀜',
'recentchangestext'                 => '위키의 최근 바뀜 내역이 나와 있습니다.',
'recentchanges-feed-description'    => '위키의 최근 바뀜',
'rcnote'                            => "다음은 $4 $5 까지의 '''$2'''일동안 바뀐 '''$1'''개의 문서입니다.",
'rcnotefrom'                        => "다음은 '''$2'''에서부터 바뀐 '''$1'''개의 문서입니다.",
'rclistfrom'                        => '$1 이래로 바뀐 문서',
'rcshowhideminor'                   => '사소한 편집을 $1',
'rcshowhidebots'                    => '봇을 $1',
'rcshowhideliu'                     => '로그인한 사용자를 $1',
'rcshowhideanons'                   => '익명 사용자를 $1',
'rcshowhidepatr'                    => '검토된 편집을 $1',
'rcshowhidemine'                    => '내 편집을 $1',
'rclinks'                           => '최근 $2일 동안에 바뀐 $1개의 문서를 봅니다.<br />$3',
'diff'                              => '차이',
'hist'                              => '역사',
'hide'                              => '숨기기',
'show'                              => '보이기',
'minoreditletter'                   => '잔글',
'newpageletter'                     => '새글',
'boteditletter'                     => '봇',
'number_of_watching_users_pageview' => '[$1명이 주시하고 있음]',
'rc_categories'                     => '다음 분류로 제한 (‘|’로 구분)',
'rc_categories_any'                 => '모두',
'newsectionsummary'                 => '새 주제: /* $1 */',

# Recent changes linked
'recentchangeslinked'          => '가리키는 글의 바뀜',
'recentchangeslinked-title'    => '"$1" 문서에 관련된 문서 바뀜',
'recentchangeslinked-noresult' => '여기를 가리키는 글 중에는 주어진 기간 동안의 최근 바뀜이 없습니다.',
'recentchangeslinked-summary'  => "여기를 가리키는 문서(분류일 경우 이 분류에 포함된 문서)에 대한 최근 바뀜이 나와 있습니다.
[[Special:Watchlist|주시하는 문서]]는 '''굵은''' 글씨로 나타납니다.",
'recentchangeslinked-page'     => '문서 이름:',
'recentchangeslinked-to'       => '여기를 가리키는 문서의 최근 바뀜',

# Upload
'upload'                      => '파일 올리기',
'uploadbtn'                   => '파일 올리기',
'reupload'                    => '다시 올리기',
'reuploaddesc'                => '올리기를 취소하고 올리기 양식으로 돌아가기',
'uploadnologin'               => '로그인하지 않음',
'uploadnologintext'           => '파일을 올리려면 [[Special:UserLogin|로그인]]해야 합니다.',
'upload_directory_read_only'  => '파일 저장 디렉토리($1)에 쓰기 권한이 없습니다.',
'uploaderror'                 => '올리기 오류',
'uploadtext'                  => "파일을 올리기 위해서는 아래의 양식을 채워주세요.
[[Special:ImageList|파일 목록]]에서 이전에 올라온 파일을 찾을 수 있습니다. [[Special:Log/upload|올리기 기록]]에는 파일이 올라온 기록이 남습니다. 삭제 기록은 [[Special:Log/delete|삭제 로그]]에서 볼 수 있습니다.

문서에 그림을 넣으려면 아래 방법 중 하나를 사용하세요.
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:File.jpg]]</nowiki></tt>''' 그림의 온전한 모양을 사용하고자 할 때.
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:File.png|200픽셀|썸네일|왼쪽|설명]]</nowiki></tt>''' 그림의 넓이를 200픽셀로 하고 왼쪽 정렬하며 '설명' 이라는 주석을 그림 밑에 달 때.
* '''<tt><nowiki>[[</nowiki>{{ns:media}}<nowiki>:File.ogg]]</nowiki></tt>''' 파일을 직접 보여주지 않고 파일로 바로 링크할때.",
'upload-permitted'            => '허용하는 파일 확장자: $1',
'upload-preferred'            => '권장하는 파일 확장자: $1',
'upload-prohibited'           => '금지하는 파일 확장자: $1',
'uploadlog'                   => '올리기 기록',
'uploadlogpage'               => '올리기 기록',
'uploadlogpagetext'           => '최근 올라온 그림 목록입니다.
보다 시각적으로 확인하고 싶으시다면 [[Special:NewImages|새 그림 화랑]]을 보세요.',
'filename'                    => '파일이름',
'filedesc'                    => '파일의 설명',
'fileuploadsummary'           => '설명:',
'filestatus'                  => '저작권 상태:',
'filesource'                  => '출처:',
'uploadedfiles'               => '파일 올리기',
'ignorewarning'               => '경고를 무시하고 저장합니다',
'ignorewarnings'              => '모든 경고 무시하기',
'minlength1'                  => '파일 이름은 적어도 1글자 이상이어야 합니다.',
'illegalfilename'             => '파일명 "$1"에는 문서 제목으로 허용되지 않는 글자가 포함되어 있습니다. 이름을 바꾸어 다시 시도해 주세요.',
'badfilename'                 => '파일 이름이 ‘$1’(으)로 바뀌었습니다.',
'filetype-badmime'            => '‘$1’ MIME을 가진 파일은 올릴 수 없습니다.',
'filetype-unwanted-type'      => "'''\".\$1\"''' 확장자는 추천하지 않습니다. 추천하는 파일 확장자는 \$2입니다.",
'filetype-banned-type'        => "'''\".\$1\"''' 확장자로 파일을 올릴 수 없습니다. 가능한 확장자는 \$2입니다.",
'filetype-missing'            => '파일에 확장자(‘.jpg’ 등)가 없습니다.',
'large-file'                  => '파일 크기는 $1을 넘지 않는 것을 추천합니다. 이 파일의 크기는 $2입니다.',
'largefileserver'             => '이 파일의 크기가 서버에서 허용된 설정보다 큽니다.',
'emptyfile'                   => '당신이 올린 파일이 빈 파일입니다. 파일명을 잘못 입력했을 수도 있습니다. 다시 한 번 확인해 주시기 바랍니다.',
'fileexists'                  => '같은 이름의 파일이 이미 있습니다. 파일을 바꾸고 싶지 않다면 <strong><tt>$1</tt></strong> 파일을 확인해 주세요.',
'fileexists-extension'        => '비슷한 이름의 파일이 존재합니다:<br />
올리려는 파일 이름: <strong><tt>$1</tt></strong><br />
존재하는 파일 이름: <strong><tt>$2</tt></strong><br />
다른 이름으로 시도해 주세요.',
'fileexists-thumb'            => "<center>'''존재하는 그림'''</center>",
'fileexists-forbidden'        => '같은 이름의 파일이 이미 있습니다. 뒤로 돌아가서 다른 이름으로 시도해 주시기 바랍니다. [[Image:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => '같은 이름의 파일이 이미 위키미디어 공용에 있습니다.
파일을 업로드하길 원하신다면 뒤로 돌아가서 다른 이름으로 시도해 주시기 바랍니다. [[Image:$1|thumb|center|$1]]',
'successfulupload'            => '올리기 성공',
'uploadwarning'               => '올리기 경고',
'savefile'                    => '파일 저장',
'uploadedimage'               => '‘[[$1]]’을(를) 올렸습니다.',
'overwroteimage'              => '‘[[$1]]’ 파일의 새 버전을 올렸습니다.',
'uploaddisabled'              => '올리기 비활성화됨',
'uploaddisabledtext'          => '{{SITENAME}}에서는 파일 올리기 기능이 비활성화되어 있습니다.',
'uploadscripted'              => '이 파일에는 HTML이나 다른 스크립트 코드가 포함되어 있어, 웹 브라우저에서 오류를 일으킬 수 있습니다.',
'uploadcorrupt'               => '이 파일은 잘못된 형식을 가지고 있습니다. 파일을 확인하고 다시 시도해 주세요.',
'uploadvirus'                 => '파일이 바이러스를 포함하고 있습니다! 상세 설명: $1',
'sourcefilename'              => '원본 파일 이름:',
'destfilename'                => '파일의 새 이름:',
'upload-maxfilesize'          => '파일의 최대 크기: $1',
'watchthisupload'             => '이 문서 주시하기',
'filewasdeleted'              => '같은 이름을 가진 파일이 올라온 적이 있었고, 그 후에 삭제되었습니다. 올리기 전에 $1을 확인해 주시기 바랍니다.',
'upload-wasdeleted'           => "'''경고: 삭제된 적이 있는 파일을 다시 올리려 하고 있습니다.'''

이 파일을 올리는 것이 적절한지 다시 한번 생각해주십시오.
편의를 위해 삭제 기록을 제공합니다.",
'filename-bad-prefix'         => '올리려고 하는 파일 이름이 <strong>"$1"</strong>로 시작합니다.
"$1"은 디지털 사진기가 자동으로 붙이는 의미없는 이름입니다.
파일에 대해 알기 쉬운 이름을 골라주세요.',

'upload-proto-error'      => '잘못된 프로토콜',
'upload-proto-error-text' => '파일을 URL로 올리려면 <code>http://</code>이나 <code>ftp://</code>로 시작해야 합니다.',
'upload-file-error'       => '내부 오류',
'upload-file-error-text'  => '서버에 임시 파일을 만드는 과정에서 내부 오류가 발생했습니다. [[Special:ListUsers/sysop|관리자]]에게 연락해주세요.',
'upload-misc-error'       => '알 수 없는 파일 올리기 오류',
'upload-misc-error-text'  => '파일을 올리는 중 알 수 없는 오류가 발생했습니다. URL이 올바르고 접근 가능한지를 확인하고 다시 시도해주세요. 문제가 계속되면 [[Special:ListUsers/sysop|관리자]]에게 연락해주세요.',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => 'URL 접근 불가',
'upload-curl-error6-text'  => 'URL에 접근할 수 없습니다.
URL이 맞고 해당 웹사이트가 작동하는지 확인해주세요.',
'upload-curl-error28'      => '업로드 시간 초과',
'upload-curl-error28-text' => '사이트에서 응답하는 시간이 너무 깁니다. 사이트 접속이 가능한지 확인한 다음 다시 시도해주세요. 해당 사이트에 접속이 많을 경우, 접속이 원활한 시간대에 시도해주세요.',

'license'           => '라이센스:',
'nolicense'         => '선택하지 않음',
'license-nopreview' => '(미리보기 불가능)',

# Special:ImageList
'imagelist-summary'     => '이 특수문서는 올라온 모든 파일의 목록입니다.
기본적으로 마지막으로 올라온 파일이 맨 위에 보입니다.
다르게 정렬하려면 원하는 기준 열의 첫 칸을 누르세요.',
'imgfile'               => '파일',
'imagelist'             => '그림 목록',
'imagelist_date'        => '날짜',
'imagelist_name'        => '이름',
'imagelist_user'        => '사용자',
'imagelist_size'        => '크기',
'imagelist_description' => '설명',

# Image description page
'filehist'                  => '파일 역사',
'filehist-help'             => '날짜/시간을 누르면 해당 시간에 올라온 파일을 볼 수 있습니다.',
'filehist-deleteall'        => '모두 삭제',
'filehist-deleteone'        => '지우기',
'filehist-revert'           => '되돌리기',
'filehist-current'          => '현재',
'filehist-datetime'         => '날짜/시간',
'filehist-user'             => '사용자',
'filehist-dimensions'       => '크기',
'filehist-filesize'         => '파일 크기',
'filehist-comment'          => '내용',
'imagelinks'                => '그림 링크',
'linkstoimage'              => '다음 $1개의 문서가 이 그림을 사용하고 있습니다:',
'nolinkstoimage'            => '이 그림을 사용하는 문서가 없습니다.',
'morelinkstoimage'          => '이 파일이 쓰이고 있는 문서 목록 [[Special:WhatLinksHere/$1|더 보기]].',
'duplicatesoffile'          => '다음의 $1개의 파일이 중복됩니다:',
'sharedupload'              => '이 자료는 위키미디어 공용에 있습니다. 다른 프로젝트에서 사용하고 있을 가능성이 있습니다.',
'shareduploadwiki'          => '$1에서 더 자세한 정보를 얻을 수 있습니다.',
'shareduploadwiki-desc'     => '이 $1를 아래에 표시합니다.',
'shareduploadwiki-linktext' => '자료의 설명 문서',
'noimage'                   => '파일이 없습니다. $1 할 수 있습니다.',
'noimage-linktext'          => '업로드',
'uploadnewversion-linktext' => '이 파일의 새로운 버전을 올리기',
'imagepage-searchdupe'      => '중복 파일 찾기',

# File reversion
'filerevert'                => '$1 되돌리기',
'filerevert-legend'         => '파일 되돌리기',
'filerevert-intro'          => "'''[[Media:$1|$1]]''' 파일을 [$4 $2 $3 버전]으로 되돌립니다.",
'filerevert-comment'        => '내용:',
'filerevert-defaultcomment' => '$1 $2 버전으로 되돌림',
'filerevert-submit'         => '되돌리기',
'filerevert-success'        => "'''[[Media:$1|$1]]''' 파일을 [$4 $2 $3 버전]으로 되돌렸습니다.",

# File deletion
'filedelete'                  => '$1 삭제하기',
'filedelete-legend'           => '파일 삭제하기',
'filedelete-intro'            => "'''[[Media:$1|$1]]''' 파일을 삭제합니다.",
'filedelete-intro-old'        => "'''[[Media:$1|$1]]''' 파일의 [$4 $2 $3] 버전을 삭제합니다.",
'filedelete-comment'          => '삭제 이유:',
'filedelete-submit'           => '삭제',
'filedelete-success'          => "'''$1''' 파일을 삭제했습니다.",
'filedelete-success-old'      => "'''[[Media:$1|$1]]''' 파일의 $2 $3 버전을 삭제했습니다.",
'filedelete-nofile'           => "{{SITENAME}}에 '''$1''' 파일이 존재하지 않습니다.",
'filedelete-nofile-old'       => "해당 조건에 맞는 과거 '''$1''' 파일이 존재하지 않습니다.",
'filedelete-iscurrent'        => '가장 최신 버전의 파일을 삭제하려고 하고 있습니다. 먼저 이전 버전으로 되돌린 다음에 삭제해주세요.',
'filedelete-otherreason'      => '다른 이유/추가적인 이유:',
'filedelete-reason-otherlist' => '다른 이유',
'filedelete-reason-dropdown'  => '*일반적인 삭제 이유
** 저작권 침해
** 중복된 파일',
'filedelete-edit-reasonlist'  => '삭제 이유 편집',

# MIME search
'mimesearch' => 'MIME 검색',
'mimetype'   => 'MIME 종류:',
'download'   => '다운로드',

# Unwatched pages
'unwatchedpages' => '주시되지 않는 문서 목록',

# List redirects
'listredirects' => '넘겨주기 문서 목록',

# Unused templates
'unusedtemplates'     => '사용하지 않는 틀 목록',
'unusedtemplatestext' => '다른 문서에 사용되지 않는 틀을 모아 놓았습니다. 삭제하기 전에 쓰이지 않는지를 다시 한번 확인해 주세요.',
'unusedtemplateswlh'  => '다른 링크',

# Random page
'randompage'         => '임의 문서로',
'randompage-nopages' => '이 네임스페이스에는 문서가 없습니다.',

# Random redirect
'randomredirect'         => '임의 넘겨주기 문서로',
'randomredirect-nopages' => '이 네임스페이스에서 해당하는 넘겨주기 문서가 없습니다.',

# Statistics
'statistics'             => '통계',
'sitestats'              => '{{SITENAME}} 통계',
'userstats'              => '사용자 통계',
'sitestatstext'          => "현재 데이터베이스에는 '''\$1'''개의 문서가 있습니다. 이것은 토론 문서, {{SITENAME}} 문서, \"토막글\" 문서, 넘겨주기 문서 등을 포함하고 있습니다. 이것들을 제외하면 '''\$2'''개의 문서가 있습니다.

'''\$8'''개의 파일이 올라와 있습니다.

위키가 설치된 후 문서는 전체 '''\$3'''번 읽혔고, '''\$4'''번 편집되었습니다. 문서당 평균 편집 횟수는 '''\$5'''번이고, 한번 편집할 때마다 평균 '''\$6'''번 문서를 보았습니다.

[http://www.mediawiki.org/wiki/Manual:Job_queue job queue]의 길이는 '''\$7'''입니다.",
'userstatstext'          => "'''$1'''명의 [[Special:ListUsers|사용자]]가 등록되어 있고, 이 중 $5 권한을 가진 사용자 수는 '''$2'''명('''$4%''')입니다.",
'statistics-mostpopular' => '가장 많이 읽힌 문서',

'disambiguations'      => '동음이의 문서 목록',
'disambiguationspage'  => 'Template:Disambig',
'disambiguations-text' => "다음의 문서들은 '''동음이의 문서'''를 가리키고 있습니다.
그 링크를 다른 적절한 문서로 연결해 주어야 합니다.<br />
[[MediaWiki:Disambiguationspage]]에서 링크된 틀을 사용하는 문서를 동음이의 문서로 간주합니다.",

'doubleredirects'            => '이중 넘겨주기 목록',
'doubleredirectstext'        => '각 열의 첫번째 문서는 두번째 문서로, 두번째 문서는 세번째 문서로 연결됩니다. 첫번째 문서를 마지막 문서로 연결해 주어야 합니다.',
'double-redirect-fixed-move' => '넘겨주기 수정: [[$1]] 문서가 [[$2]] (으)로 이동되었습니다.',
'double-redirect-fixer'      => '넘겨주기 수정',

'brokenredirects'        => '끊긴 넘겨주기 목록',
'brokenredirectstext'    => '존재하지 않는 문서로 넘겨주기가 되어 있는 문서의 목록입니다:',
'brokenredirects-edit'   => '(편집)',
'brokenredirects-delete' => '(삭제)',

'withoutinterwiki'         => '언어 인터위키 링크가 없는 문서 목록',
'withoutinterwiki-summary' => '다른 언어로의 연결이 없는 문서의 목록입니다:',
'withoutinterwiki-submit'  => '보이기',

'fewestrevisions' => '편집 역사가 짧은 문서 목록',

# Miscellaneous special pages
'nbytes'                  => '$1 바이트',
'ncategories'             => '$1개의 분류',
'nlinks'                  => '$1개의 링크',
'nmembers'                => '$1개의 항목',
'nrevisions'              => '$1개의 판',
'nviews'                  => '$1번 읽음',
'specialpage-empty'       => '명령에 대한 결과가 없습니다.',
'lonelypages'             => '외톨이 문서 목록',
'lonelypagestext'         => '{{SITENAME}}에서, 다른 모든 문서에서 링크하지 않은 문서의 목록입니다.',
'uncategorizedpages'      => '분류되지 않은 문서 목록',
'uncategorizedcategories' => '분류되지 않은 분류 목록',
'uncategorizedimages'     => '분류되지 않은 파일 목록',
'uncategorizedtemplates'  => '분류되지 않은 틀 목록',
'unusedcategories'        => '사용하지 않는 분류 목록',
'unusedimages'            => '사용하지 않는 그림 목록',
'popularpages'            => '인기있는 문서 목록',
'wantedcategories'        => '필요한 분류 목록',
'wantedpages'             => '필요한 문서 목록',
'mostlinked'              => '가장 많이 연결된 문서 목록',
'mostlinkedcategories'    => '가장 많이 연결된 분류 목록',
'mostlinkedtemplates'     => '가장 많이 사용된 틀 목록',
'mostcategories'          => '가장 많이 분류된 문서 목록',
'mostimages'              => '가장 많이 사용된 파일 목록',
'mostrevisions'           => '가장 많이 편집된 문서 목록',
'prefixindex'             => '접두어 목록',
'shortpages'              => '짧은 문서 목록',
'longpages'               => '긴 문서 목록',
'deadendpages'            => '막다른 문서 목록',
'deadendpagestext'        => '{{SITENAME}} 내의 다른 문서로 나가는 링크가 없는 문서의 목록입니다.',
'protectedpages'          => '보호된 문서 목록',
'protectedpages-indef'    => '오른쪽 조건에 맞는 보호만 보기',
'protectedpagestext'      => '다음의 문서는 이동/편집이 불가능하도록 보호되어 있습니다.',
'protectedpagesempty'     => '보호되어 있는 문서가 없습니다.',
'protectedtitles'         => '생성 보호된 표제어 목록',
'protectedtitlestext'     => '다음 표제어들은 생성이 금지되어 있습니다.',
'protectedtitlesempty'    => '해당 조건에 맞는 생성 금지 표제어가 없습니다.',
'listusers'               => '사용자 목록',
'newpages'                => '새 문서 목록',
'newpages-username'       => '이름:',
'ancientpages'            => '오래된 문서 목록',
'move'                    => '이동',
'movethispage'            => '문서 이동하기',
'unusedimagestext'        => '다른 사이트에서 그림의 URL을 직접 사용하고 있을 가능성이 있고, 따라서 그림이 이 목록에 있어도 실제로 어딘가에서 사용하고 있을 수 있습니다.',
'unusedcategoriestext'    => '사용하지 않는 분류 문서들의 목록입니다.',
'notargettitle'           => '해당하는 문서 없음',
'notargettext'            => '기능을 수행할 목표 문서나 목표 사용자를 지정하지 않았습니다.',
'pager-newer-n'           => '이전 $1개',
'pager-older-n'           => '다음 $1개',

# Book sources
'booksources'               => '책 찾기',
'booksources-search-legend' => '책 찾기',
'booksources-go'            => '찾기',
'booksources-text'          => '아래의 목록은 새 책이나 중고 책을 판매하는 외부 사이트로, 원하는 책의 정보를 얻을 수 있습니다:',

# Special:Log
'specialloguserlabel'  => '이름:',
'speciallogtitlelabel' => '제목:',
'log'                  => '로그 목록',
'all-logs-page'        => '모든 기록',
'log-search-legend'    => '로그 검색',
'log-search-submit'    => '찾기',
'alllogstext'          => '{{SITENAME}}에서의 기록이 모두 나와 있습니다.
로그 종류, 계정 이름, 문서 이름을 선택해서 볼 수 있습니다. 검색시에는 대소문자를 구별합니다.',
'logempty'             => '일치하는 항목이 없습니다.',
'log-title-wildcard'   => '다음 글로 시작하는 제목 검색',

# Special:AllPages
'allpages'          => '모든 문서 목록',
'alphaindexline'    => '$1에서 $2까지',
'nextpage'          => '다음 문서 ($1)',
'prevpage'          => '이전 문서 ($1)',
'allpagesfrom'      => '다음으로 시작하는 문서들을 보여주기:',
'allarticles'       => '모든 문서',
'allinnamespace'    => '$1 이름공간의 모든 문서',
'allnotinnamespace' => '$1 네임스페이스를 제외한 모든 문서 목록',
'allpagesprev'      => '이전',
'allpagesnext'      => '다음',
'allpagessubmit'    => '표시',
'allpagesprefix'    => '다음 접두어로 시작하는 문서 목록:',
'allpagesbadtitle'  => '문서 제목이 잘못되었거나 다른 사이트로 연결되는 인터위키를 가지고 있습니다. 문서 제목에 사용할 수 없는 문자를 사용했을 수 있습니다.',
'allpages-bad-ns'   => '{{SITENAME}}에는 ‘$1’ 네임스페이스를 사용하지 않습니다.',

# Special:Categories
'categories'         => '분류',
'categoriespagetext' => '문서나 자료를 담고 있는 분류 목록입니다.
[[Special:UnusedCategories|사용되지 않는 분류]]는 여기에 보이지 않습니다.
[[Special:WantedCategories|필요한 분류]]도 참고하세요.',

# Special:ListUsers
'listusersfrom'      => '다음으로 시작하는 사용자 보이기:',
'listusers-submit'   => '보이기',
'listusers-noresult' => '해당 사용자가 없습니다.',

# Special:ListGroupRights
'listgrouprights' => '사용자 권한 목록',

# E-mail user
'mailnologin'     => '보낼 이메일 주소가 없음',
'mailnologintext' => '다른 사용자에게 이메일을 보내려면, [[Special:UserLogin|로그인]]한 다음 [[Special:Preferences|사용자 환경 설정]]에서 자신의 이메일 주소를 저장해야 합니다.',
'emailuser'       => '이메일 보내기',
'emailpage'       => '사용자에게 이메일 보내기',
'emailpagetext'   => '이 사용자가 환경 설정에 올바른 이메일 주소를 적었다면, 아래 양식을 통해 이메일을 보낼 수 있습니다.
이메일을 받은 사용자가 바로 답장할 수 있도록 하기 위해 당신이 [[Special:Preferences|사용자 환경 설정]]에 적은 이메일 주소가 "발신자" 정보에 들어갑니다.',
'usermailererror' => '메일 객체에서 오류 발생:',
'defemailsubject' => '{{SITENAME}} 이메일',
'noemailtitle'    => '이메일 주소 없음',
'noemailtext'     => '이 사용자는 올바른 이메일 주소를 입력하지 않았거나, 이메일을 받지 않도록 설정해 놓았습니다.',
'emailfrom'       => '이메일 발신자:',
'emailto'         => '수신자:',
'emailsubject'    => '제목:',
'emailmessage'    => '내용:',
'emailsend'       => '보내기',
'emailccme'       => '메일 사본을 내 이메일로 보내기',
'emailccsubject'  => '$1에게 보낸 메일 사본: $2',
'emailsent'       => '이메일 보냄',
'emailsenttext'   => '이메일을 보냈습니다.',

# Watchlist
'watchlist'            => '주시문서 목록',
'mywatchlist'          => '내 주시문서 목록',
'watchlistfor'         => "('''$1'''의 목록)",
'nowatchlist'          => '주시하는 문서가 아직 없습니다.',
'watchlistanontext'    => '주시문서 목록의 항목들을 보거나 편집하려면 $1을(를) 보세요.',
'watchnologin'         => '로그인하지 않음',
'watchnologintext'     => '[[Special:UserLogin|로그인]]을 해야만 주시문서 목록을 볼 수 있습니다.',
'addedwatch'           => '주시문서 목록에 추가',
'addedwatchtext'       => "‘[[:$1]]’ 문서가 주시문서 목록에 추가되었습니다. 앞으로 이 문서나 토론 문서가 변경되면 [[Special:RecentChanges|최근 바뀜]]에서 변경점들이 '''굵은 글씨'''로 나타날 것입니다. 더 이상 주시하지 않으려면 ‘주시 해제’를 누르면 됩니다.",
'removedwatch'         => '주시문서 목록에서 제거',
'removedwatchtext'     => '‘[[:$1]]’ 문서를 주시문서 목록에서 제거했습니다.',
'watch'                => '주시',
'watchthispage'        => '주시하기',
'unwatch'              => '주시 해제',
'unwatchthispage'      => '주시 해제하기',
'notanarticle'         => '문서가 아님',
'watchnochange'        => '주어진 기간 중에 바뀐 주시문서가 없습니다.',
'watchlist-details'    => '$1개(토론 제외)의 문서를 주시하고 있습니다.',
'wlheader-enotif'      => '* 이메일 알림 기능이 활성화되었습니다.',
'wlheader-showupdated' => "* 마지막으로 방문한 이후에 바뀐 문서들은 '''굵은 글씨'''로 표시됩니다.",
'watchlistcontains'    => '$1개의 문서를 주시하고 있습니다.',
'iteminvalidname'      => '"$1" 항목에 문제가 발생했습니다. 이름이 잘못되었습니다...',
'wlnote'               => "다음은 최근 '''$2'''시간 동안에 바뀐 $1개의 문서입니다.",
'wlshowlast'           => '최근 $1 시간 $2 일 또는 $3 동안에 바뀐 문서',
'watchlist-show-bots'  => '봇의 편집을 보이기',
'watchlist-hide-bots'  => '봇의 편집을 숨기기',
'watchlist-show-own'   => '나의 편집을 보이기',
'watchlist-hide-own'   => '나의 편집을 숨기기',
'watchlist-show-minor' => '사소한 편집을 보이기',
'watchlist-hide-minor' => '사소한 편집을 숨기기',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => '주시하는 중...',
'unwatching' => '주시 해제하는 중...',

'enotif_mailer'                => '{{SITENAME}} 자동 알림 메일',
'enotif_reset'                 => '모든 문서를 방문한 것으로 표시하기',
'enotif_newpagetext'           => '이것은 새 문서입니다.',
'enotif_impersonal_salutation' => '{{SITENAME}} 사용자',
'changed'                      => '바뀜',
'created'                      => '만들어짐',
'enotif_subject'               => '{{SITENAME}}에서 $PAGEEDITOR 사용자가 $PAGETITLE 문서를 $CHANGEDORCREATED함',
'enotif_lastvisited'           => '당신의 마지막 방문 이후의 모든 변경사항을 보려면 $1을(를) 보십시오.',
'enotif_anon_editor'           => '익명 사용자 $1',
'enotif_body'                  => '$WATCHINGUSERNAME에게,

{{SITENAME}}의 문서 $PAGETITLE이(가) $PAGEEDITOR에 의해 $CHANGEDORCREATED되었습니다. 현재의 문서는 $PAGETITLE_URL에서 볼 수 있습니다.

$NEWPAGE

편집 요약: $PAGESUMMARY $PAGEMINOREDIT

다음을 통해 편집자와 대화를 할 수 있습니다:
이메일: $PAGEEDITOR_EMAIL
위키: $PAGEEDITOR_WIKI

이 문서를 방문하기 전까지는 알림 이메일은 발송되지 않습니다.

             {{SITENAME}} 알림 시스템

--
주시문서 설정을 바꾸려면 다음을 사용하세요:
{{fullurl:{{ns:special}}:Watchlist/edit}}

도움말:
{{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => '문서 삭제하기',
'confirm'                     => '확인',
'excontent'                   => '내용: ‘$1’',
'excontentauthor'             => '내용: ‘$1’ (그리고 편집한 사람은 ‘$2’뿐)',
'exbeforeblank'               => '비우기 전의 내용: ‘$1’',
'exblank'                     => '빈 문서',
'delete-confirm'              => '‘$1’ 삭제',
'delete-legend'               => '삭제',
'historywarning'              => '주의: 현재 삭제하려는 문서에는 문서 역사가 있습니다:',
'confirmdeletetext'           => '문서와 문서 역사를 삭제하려고 합니다. 삭제하려는 문서가 맞는지, 이 문서를 삭제하는 것이 [[{{MediaWiki:Policy-url}}|정책]]에 맞는 행동인지를 확인해 주세요.',
'actioncomplete'              => '명령완료',
'deletedtext'                 => '‘<nowiki>$1</nowiki>’ 문서를 삭제했습니다. 최근 삭제 기록은 $2에 있습니다.',
'deletedarticle'              => '이(가) [[$1]]을(를) 삭제하였습니다.',
'dellogpage'                  => '삭제 기록',
'dellogpagetext'              => '아래의 목록은 최근에 삭제된 문서들입니다.',
'deletionlog'                 => '삭제 기록',
'reverted'                    => '이전 버전으로 되돌렸습니다.',
'deletecomment'               => '삭제 이유:',
'deleteotherreason'           => '다른 이유/추가적인 이유:',
'deletereasonotherlist'       => '다른 이유',
'deletereason-dropdown'       => '*일반적인 삭제 이유
** 작성자의 요청
** 저작권 침해
** 잘못된 문서',
'delete-edit-reasonlist'      => '삭제 이유 편집',
'delete-toobig'               => '이 문서에는 편집 역사가 $1개 있습니다. 편집 역사가 긴 문서를 삭제하면 {{SITENAME}}에 큰 혼란을 줄 수 있기 때문에 삭제할 수 없습니다.',
'delete-warning-toobig'       => '이 문서에는 편집 역사가 $1개 있습니다. 편집 역사가 긴 문서를 삭제하면 {{SITENAME}} 데이터베이스 동작에 큰 영향을 줄 수 있기 때문에, 주의해 주세요.',
'rollback'                    => '편집 되돌리기',
'rollback_short'              => '되돌리기',
'rollbacklink'                => '되돌리기',
'rollbackfailed'              => '되돌리기 실패',
'cantrollback'                => '편집을 되돌릴 수 없습니다. 문서를 편집한 사용자가 한명뿐입니다.',
'alreadyrolled'               => '[[:$1]]에서 [[User:$2|$2]] ([[User talk:$2|토론]] | [[Special:Contributions/$2|{{int:contribslink}}]]) 의 편집을 되돌릴 수 없습니다; 누군가가 문서를 고치거나 되돌렸습니다.

마지막으로 문서를 편집한 사람은 [[User:$3|$3]] ([[User talk:$3|토론]] | [[Special:Contributions/$3|{{int:contribslink}}]]) 입니다.',
'editcomment'                 => "편집 요약: ''$1''", # only shown if there is an edit comment
'revertpage'                  => '[[Special:Contributions/$2|$2]]([[User talk:$2|토론]])의 편집을 [[User:$1|$1]]의 마지막 버전으로 되돌림', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => '$1의 편집을 $2의 마지막 버전으로 되돌렸습니다.',
'sessionfailure'              => '로그인 세션에 문제가 발생한 것 같습니다. 세션 하이재킹을 막기 위해 동작이 취소되었습니다. 브라우저의 "뒤로" 버튼을 누르고 문서를 새로고침한 후에 다시 시도해 주세요.',
'protectlogpage'              => '문서 보호 기록',
'protectlogtext'              => '아래의 목록은 문서 보호와 보호 해제 기록입니다.',
'protectedarticle'            => '‘[[$1]]’ 문서를 보호함',
'modifiedarticleprotection'   => '‘[[$1]]’ 문서의 보호 설정을 변경함',
'unprotectedarticle'          => '‘[[$1]]’ 문서를 보호 해제함',
'protect-title'               => '"$1" 보호하기',
'protect-legend'              => '보호 확인',
'protectcomment'              => '보호 이유:',
'protectexpiry'               => '보호 기간:',
'protect_expiry_invalid'      => '보호 기간이 잘못되었습니다.',
'protect_expiry_old'          => '만료되는 시간을 과거로 입력했습니다.',
'protect-unchain'             => '이동 권한을 수동으로 조정',
'protect-text'                => "'''<nowiki>$1</nowiki>''' 문서의 보호 수준을 보거나 변경할 수 있습니다.",
'protect-locked-blocked'      => "차단된 동안에는 보호 설정을 바꿀 수 없습니다. '''$1''' 문서의 보호 설정은 다음과 같습니다:",
'protect-locked-dblock'       => "데이터베이스가 잠겨 문서 보호 설정을 바꿀 수 없습니다. '''$1''' 문서의 현재 설정은 다음과 같습니다:",
'protect-locked-access'       => '문서 보호 수준을 변경할 권한이 없습니다. <strong>$1</strong> 문서의 권한은 다음과 같습니다.',
'protect-cascadeon'           => '다음의 틀에서 이 문서를 사용하고 있고 그 틀에 연쇄적 보호가 걸려 있어 이 문서도 자동으로 보호됩니다.
이 문서의 보호 설정을 바꾸어도 연쇄적 보호에 영향을 주지 않습니다.',
'protect-default'             => '(기본값)',
'protect-fallback'            => '‘$1’ 권한 필요',
'protect-level-autoconfirmed' => '등록된 사용자만 가능',
'protect-level-sysop'         => '관리자만 가능',
'protect-summary-cascade'     => '연쇄적',
'protect-expiring'            => '$1(UTC)에 만료',
'protect-cascade'             => '연쇄적 보호 - 이 문서에서 사용되는 다른 문서를 함께 보호합니다.',
'protect-cantedit'            => '이 문서의 보호 설정을 바꿀 권한이 없습니다.',
'restriction-type'            => '권한:',
'restriction-level'           => '보호 수준:',
'minimum-size'                => '최소 크기',
'maximum-size'                => '최대 크기:',
'pagesize'                    => '(바이트)',

# Restrictions (nouns)
'restriction-edit'   => '편집',
'restriction-move'   => '이동',
'restriction-create' => '만들기',

# Restriction levels
'restriction-level-sysop'         => '보호됨',
'restriction-level-autoconfirmed' => '준보호됨',

# Undelete
'undelete'                 => '삭제된 문서 보기',
'undeletepage'             => '삭제된 문서를 보거나 되살리기',
'undeletepagetitle'        => "'''아래는 [[:$1|$1]]의 삭제된 판입니다.'''.",
'viewdeletedpage'          => '삭제된 문서 보기',
'undeletepagetext'         => '다음의 문서는 삭제되었지만 보관되어 있고, 되살릴 수 있습니다. 보관된 문서들은 주기적으로 삭제될 것입니다.',
'undeleteextrahelp'        => "문서 역사 전체를 복구하려면 모든 체크박스를 선택 해제한 뒤 '''복구'''를 누르세요.
특정한 버전만을 복구하려면 복구하려는 버전들을 선택한 뒤 '''복구'''를 누르세요. '''초기화'''를 누르면 모든 선택이 취소됩니다.",
'undeleterevisions'        => '$1개의 버전 보관중',
'undeletehistory'          => '문서를 되살리면 모든 역사가 같이 복구됩니다.
문서가 삭제된 후에 같은 이름의 문서가 만들어졌다면, 복구되는 버전들은 역사의 과거 부분에 나타날 것입니다.',
'undeletehistorynoadmin'   => '이 문서는 삭제되어 있습니다. 삭제된 이유와 삭제되기 전에 이 문서를 편집한 사용자들이 아래에 나와 있습니다. 삭제된 문서를 보려면 관리자 권한이 필요합니다.',
'undeletebtn'              => '복구',
'undeletelink'             => '되살리기',
'undeletereset'            => '초기화',
'undeletecomment'          => '설명:',
'undeletedarticle'         => '"[[$1]]" 복구됨',
'undeletedrevisions'       => '$1개의 버전이 복구되었습니다.',
'undeletedrevisions-files' => '$1개의 버전과 $2개의 파일이 복구되었습니다.',
'undeletedfiles'           => '$1개의 파일이 복구되었습니다.',
'cannotundelete'           => '복구에 실패했습니다. 다른 누군가가 이미 복구했을 수도 있습니다.',
'undeletedpage'            => "<big>'''$1이(가) 복구되었습니다.'''</big>

[[Special:Log/delete|삭제 기록]]에서 최근의 삭제/복구 기록을 볼 수 있습니다.",
'undelete-header'          => '최근에 삭제된 문서 기록은 [[Special:Log/delete|여기]]에서 볼 수 있습니다.',
'undelete-search-box'      => '삭제된 문서 찾기',
'undelete-search-prefix'   => '다음으로 시작하는 문서 보이기:',
'undelete-search-submit'   => '찾기',
'undelete-error-short'     => '파일 복구 오류: $1',
'undelete-error-long'      => '파일을 복구하는 중 오류 발생:

$1',

# Namespace form on various pages
'namespace'      => '이름공간:',
'invert'         => '선택 반전',
'blanknamespace' => '(일반)',

# Contributions
'contributions' => '사용자 기여',
'mycontris'     => '내 기여 목록',
'contribsub2'   => '$1($2)의 기여',
'nocontribs'    => '이 사용자는 어디에도 기여하지 않았습니다.',
'uctop'         => '(최신)',
'month'         => '월:',
'year'          => '연도:',

'sp-contributions-newbies'     => '새 사용자의 기여만 보기',
'sp-contributions-newbies-sub' => '새 사용자들의 기여',
'sp-contributions-blocklog'    => '차단 기록',
'sp-contributions-search'      => '기여 검색',
'sp-contributions-username'    => 'IP 주소 혹은 계정 이름:',
'sp-contributions-submit'      => '찾기',

# What links here
'whatlinkshere'            => '여기를 가리키는 글',
'whatlinkshere-title'      => '"$1" 문서를 가리키는 문서 목록',
'whatlinkshere-page'       => '문서:',
'linklistsub'              => '(링크 목록)',
'linkshere'                => "다음의 문서들이 '''[[:$1]]''' 문서를 가리키고 있습니다:",
'nolinkshere'              => "'''[[:$1]]''' 문서를 가리키는 문서가 없습니다.",
'nolinkshere-ns'           => "해당 네임스페이스에서 '''[[:$1]]''' 문서를 가리키는 문서가 없습니다.",
'isredirect'               => '넘겨주기 문서',
'istemplate'               => '포함',
'isimage'                  => '그림 포함',
'whatlinkshere-prev'       => '{{PLURAL:$1|이전|이전 $1}}',
'whatlinkshere-next'       => '{{PLURAL:$1|다음|다음 $1}}',
'whatlinkshere-links'      => '← 가리키는 문서 목록',
'whatlinkshere-hideredirs' => '넘겨주기를 $1',
'whatlinkshere-hidelinks'  => '링크를 $1',

# Block/unblock
'blockip'                     => '사용자 차단',
'blockiptext'                 => '차단할 IP 주소나 사용자 이름을 아래에 적어 주세요.
차단은 문서 훼손을 막기 위해, [[{{MediaWiki:Policy-url}}|정책]]에 의해서만 이루어져야 합니다.
차단 이유를 같이 적어주세요(예: 특정 문서 훼손).',
'ipaddress'                   => 'IP 주소:',
'ipadressorusername'          => 'IP 주소 또는 계정 이름:',
'ipbexpiry'                   => '기간:',
'ipbreason'                   => '이유:',
'ipbreasonotherlist'          => '다른 이유',
'ipbreason-dropdown'          => '*일반적인 차단 이유
** 잘못된 내용 편집
** 문서 내용을 지움
** 문서에 스팸을 넣음
** 장난 편집
** 협박성 행동
** 다중 계정 악용
** 잘못된 계정 이름',
'ipbanononly'                 => '익명 사용자만 차단하기',
'ipbcreateaccount'            => '계정 생성을 막기',
'ipbemailban'                 => '이메일을 보내지 못하도록 막기',
'ipbenableautoblock'          => '이 사용자가 최근에 사용했거나 앞으로 사용하는 IP를 자동으로 막기',
'ipbsubmit'                   => '사용자 차단',
'ipbother'                    => '다른 기간:',
'ipboptions'                  => '2시간:2 hours,1일:1 day,3일:3 days,1주일:1 week,2주일:2 weeks,1개월:1 month,3개월:3 months,6개월:6 months,1년:1 year,무기한:infinite', # display1:time1,display2:time2,...
'ipbotheroption'              => '수동으로 지정',
'ipbotherreason'              => '다른 이유/추가적인 이유:',
'badipaddress'                => '잘못된 IP 주소',
'blockipsuccesssub'           => '차단 완료',
'blockipsuccesstext'          => '[[Special:Contributions/$1|$1]] 사용자가 차단되었습니다. 차단된 사용자 목록은 [[Special:IPBlockList|여기]]에서 볼 수 있습니다.',
'ipb-edit-dropdown'           => '차단 사유 고치기',
'ipb-unblock-addr'            => '$1 차단 해제하기',
'ipb-unblock'                 => '사용자/IP 주소 차단 해제하기',
'ipb-blocklist-addr'          => '$1의 현재 차단 기록 보기',
'ipb-blocklist'               => '현재 차단 기록 보기',
'unblockip'                   => '사용자 차단 해제',
'unblockiptext'               => '아래의 양식에 차단 해제하려는 IP 주소나 사용자 이름을 입력하세요.',
'ipusubmit'                   => '차단 해제',
'unblocked'                   => '[[User:$1|$1]] 사용자 차단 해제됨',
'unblocked-id'                => '차단 $1 해제됨',
'ipblocklist'                 => '차단 중인 IP 주소와 계정 이름',
'ipblocklist-submit'          => '찾기',
'blocklistline'               => '$1, $2 사용자는 $3을 차단함 ($4)',
'infiniteblock'               => '무기한',
'expiringblock'               => '$1에 해제',
'anononlyblock'               => '익명 사용자만',
'noautoblockblock'            => '자동 차단 비활성화됨',
'createaccountblock'          => '계정 생성 금지됨',
'emailblock'                  => '이메일 차단됨',
'ipblocklist-no-results'      => '당신이 입력한 IP 주소나 사용자는 차단되지 않았습니다.',
'blocklink'                   => '차단',
'unblocklink'                 => '차단 해제',
'contribslink'                => '기여',
'autoblocker'                 => "당신의 IP 주소는 최근 ‘[[User:$1|$1]]’이(가) 사용하였기 때문에 자동으로 차단되었습니다. $1의 차단 이유는 다음과 같습니다: '''$2'''",
'blocklogpage'                => '차단 기록',
'blocklogentry'               => '[[$1]] 사용자를 $2 $3 차단함',
'blocklogtext'                => '이 목록은 사용자 차단/차단 해제 기록입니다. 자동으로 차단된 IP 주소는 여기에 나오지 않습니다. [[Special:IPBlockList|여기]]에서 현재 차단된 사용자 목록을 볼 수 있습니다.',
'unblocklogentry'             => '$1을 차단 해제했습니다.',
'block-log-flags-anononly'    => 'IP만 막음',
'block-log-flags-nocreate'    => '계정 생성 막음',
'block-log-flags-noautoblock' => '자동 차단 비활성화됨',
'block-log-flags-noemail'     => '이메일 막음',
'range_block_disabled'        => 'IP 범위 차단 기능이 비활성화되어 있습니다.',
'ipb_expiry_invalid'          => '차단 기간이 잘못되었습니다.',
'ipb_already_blocked'         => '"$1" 사용자는 이미 차단됨',
'ipb_cant_unblock'            => '오류: 차단 ID $1이(가) 존재하지 않습니다. 이미 차단 해제되었을 수 있습니다.',
'ip_range_invalid'            => 'IP 범위가 잘못되었습니다.',
'proxyblocker'                => '프록시 차단',
'proxyblocker-disabled'       => '이 기능은 비활성되어 있습니다.',
'proxyblockreason'            => '당신의 IP 주소는 공개 프록시로 밝혀져 자동으로 차단됩니다. 만약 인터넷 사용에 문제가 있다면 인터넷 서비스 공급자에게 문의해주세요.',
'proxyblocksuccess'           => '완료.',
'sorbsreason'                 => '당신의 IP 주소는 {{SITENAME}}에서 사용하는 DNSBL 공개 프록시 목록에 들어 있습니다.',
'sorbs_create_account_reason' => '당신의 IP 주소는 {{SITENAME}}에서 사용하는 DNSBL 공개 프록시 목록에 들어 있습니다. 계정을 만들 수 없습니다.',

# Developer tools
'lockdb'              => '데이터베이스 잠그기',
'unlockdb'            => '데이터베이스 잠금 해제',
'lockdbtext'          => '데이터베이스를 잠그면 모든 사용자의 편집 권한, 환경 설정 변경 권한, 주시문서 편집 권한 등의 모든 기능이 정지됩니다. 정말로 잠가야 하는지를 다시 한번 확인해주세요.',
'unlockdbtext'        => '데이터베이스를 잠금 해제하면 모든 사용자의 편집 권한, 환경 설정 변경 권한, 주시문서 편집 권한 등의 모든 기능이 복구됩니다. 정말로 잠금을 해제하려는지를 다시 한번 확인해주세요.',
'lockconfirm'         => '네, 데이터베이스를 잠급니다.',
'unlockconfirm'       => '네, 데이터베이스를 잠금 해제합니다.',
'lockbtn'             => '데이터베이스 잠그기',
'unlockbtn'           => '데이터베이스 잠금 해제',
'locknoconfirm'       => '확인 체크박스를 선택하지 않았습니다.',
'lockdbsuccesssub'    => '데이터베이스 잠김',
'unlockdbsuccesssub'  => '데이터베이스 잠금 해제됨',
'lockdbsuccesstext'   => '데이터베이스가 잠겼습니다.<br />
관리가 끝나면 잊지 말고 [[Special:UnlockDB|잠금을 풀어]] 주세요.',
'unlockdbsuccesstext' => '데이터베이스 잠금 상태가 해제되었습니다.',
'lockfilenotwritable' => '데이터베이스 잠금 파일에 쓰기 권한이 없습니다. 데이터베이스를 잠그거나 잠금 해제하려면, 웹 서버에서 이 파일의 쓰기 권한을 설정해야 합니다.',
'databasenotlocked'   => '데이터베이스가 잠겨 있지 않습니다.',

# Move page
'move-page'               => '이동 $1',
'move-page-legend'        => '문서 이동하기',
'movepagetext'            => "아래 양식을 채워 문서의 이름을 바꾸고 모든 역사를 새 이름으로 옮길 수 있습니다.
기존의 이름은 새 이름으로 넘겨주는 문서가 됩니다.
원래 이름을 가리키는 넘겨주기를 자동으로 갱신할 수 있습니다.
만약 이 설정을 선택하지 않았다면 [[Special:DoubleRedirects|이중 넘겨주기]]와 [[Special:BrokenRedirects|끊긴 넘겨주기]]를 확인해주세요.

만약 문서의 새 이름으로 된 문서가 존재할 때, 그 문서가 비었거나 넘겨주기 문서이고 문서 역사가 없을 때에만 이동합니다. 그렇지 않을 경우에는 이동하지 '''않습니다'''.
이것은 실수로 이동한 문서를 되돌릴 수는 있지만, 이미 존재하는 문서 위에 덮어씌울 수는 없다는 것을 의미합니다.

'''주의!'''
자주 사용하는 문서를 이동하면 위험한 결과를 가져올 수 있습니다.
이동하기 전에, 이 문서를 이동해도 문제가 없다는 것을 확인해주세요.",
'movepagetalktext'        => "딸린 토론 문서도 자동으로 이동합니다. 다음의 경우는 '''이동하지 않습니다''':
* 이동할 이름으로 된 문서가 이미 있는 경우
* 아래의 선택을 해제하는 경우
이 경우에는 문서를 직접 이동하거나 두 문서를 합쳐야 합니다.",
'movearticle'             => '문서 이동하기',
'movenotallowed'          => '{{SITENAME}}에서 문서를 이동할 권한이 없습니다.',
'newtitle'                => '새 문서 이름',
'move-watch'              => '이 문서 주시하기',
'movepagebtn'             => '이동',
'pagemovedsub'            => '문서 이동함',
'movepage-moved'          => '<big>\'\'\'"$1" 문서를 "$2" 문서로 이동했습니다.\'\'\'</big>', # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => '문서가 이미 존재하거나, 문서 이름이 올바르지 않습니다. 다른 제목으로 시도해주세요.',
'cantmove-titleprotected' => '새로운 제목으로 문서를 만드는 것이 금지되어 있어, 문서를 이동할 수 없습니다.',
'talkexists'              => "'''문서는 이동되었습니다. 하지만 딸린 토론 문서의 새 이름으로 된 문서가 이미 존재해서, 토론 문서는 이동하지 않았습니다. 직접 문서를 합쳐 주세요.'''",
'movedto'                 => '새 이름',
'movetalk'                => '딸린 토론도 함께 이동합니다.',
'move-subpages'           => '하위 문서도 함께 이동합니다.',
'move-talk-subpages'      => '토론 문서에 딸린 문서도 함께 이동합니다.',
'movepage-page-exists'    => '이동할 수 없습니다. ‘$1’ 문서가 이미 존재합니다.',
'movepage-page-moved'     => '$1 문서가 $2 (으)로 이동되었습니다.',
'1movedto2'               => '[[$1]]을(를) [[$2]](으)로 옮김',
'1movedto2_redir'         => '[[$1]]을(를) [[$2]](으)로 옮기면서 넘겨주기를 덮어 씀',
'movelogpage'             => '이동 기록',
'movelogpagetext'         => '아래는 옮겨진 문서의 목록입니다.',
'movereason'              => '이유',
'revertmove'              => '되돌리기',
'delete_and_move'         => '삭제하고 이동',
'delete_and_move_text'    => '== 삭제 필요 ==

이동하려는 제목으로 된 문서 [[:$1]]이(가) 이미 존재합니다. 삭제하고 이동할까요?',
'delete_and_move_confirm' => '네. 문서를 삭제합니다',
'delete_and_move_reason'  => '문서를 이동하기 위해 삭제함',
'selfmove'                => '이동하려는 제목이 원래 제목과 같습니다. 이동할 수 없습니다.',
'immobile_namespace'      => '특수한 네임스페이스로는 이동할 수 없습니다.',
'fix-double-redirects'    => '기존 이름을 가리키는 넘겨주기를 갱신',

# Export
'export'          => '문서 내보내기',
'exporttext'      => '특정 문서와 그 문서의 편집 역사를 XML 파일로 만들 수 있습니다. 이렇게 만들어진 파일은 다른 미디어위키에서 [[Special:Import|문서 가져오기]] 기능을 통해 가져갈 수 있습니다.

문서를 내보내려면, 내보내려는 문서 제목을 한 줄에 하나씩 입력해주세요. 그리고 문서의 전체 역사가 필요한지, 혹은 현재 버전만이 필요한지를 선택해 주세요.

특정 문서를 내보내려면, 예를 들어 ‘[[{{MediaWiki:Mainpage}}]]’ 문서를 내보내려면 [[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]] 링크를 사용할 수도 있습니다.',
'exportcuronly'   => '현재 버전만 포함하고, 전체 역사는 포함하지 않음',
'exportnohistory' => "----
'''주의:''' 전체 문서 역사를 내보내는 기능은 성능 문제로 인해 비활성되어 있습니다.",
'export-submit'   => '내보내기',

# Namespace 8 related
'allmessages'               => '시스템 메시지 목록',
'allmessagesname'           => '이름',
'allmessagesdefault'        => '기본 내용',
'allmessagescurrent'        => '현재 내용',
'allmessagestext'           => 'MediaWiki 네임스페이스에 있는 모든 시스템 메시지의 목록입니다. 미디어위키의 번역 작업에 관심이 있으시다면 [http://www.mediawiki.org/wiki/Localisation 미디어위키 지역화]나 [http://translatewiki.net 베타위키]에 참가해주세요.',
'allmessagesnotsupportedDB' => "'''\$wgUseDatabaseMessages'''가 해제되어 있어서 이 문서는 쓸 수 없습니다.",
'allmessagesfilter'         => '다음 메시지만 보이기:',
'allmessagesmodified'       => '변경된 것만 보여주기',

# Thumbnails
'thumbnail-more'  => '실제 크기로',
'filemissing'     => '파일 사라짐',
'thumbnail_error' => '섬네일을 만드는 중 오류 발생: $1',
'djvu_no_xml'     => 'DjVu 파일의 XML 정보를 읽을 수 없음',

# Special:Import
'import'     => '문서 가져오기',
'importtext' => '원본 위키에서 [[Special:Export|내보내기]] 기능을 사용해 파일을 내려받아서 여기에 올려주세요.',

# Import log
'importlogpage'             => '파일 올리기 기록',
'importlogpagetext'         => '다른 위키에서 가져온 문서 기록입니다.',
'import-logentry-interwiki' => '$1을(를) 다른 위키에서 가져왔습니다.',

# Tooltip help for the actions
'tooltip-pt-userpage'             => '내 사용자 문서',
'tooltip-pt-anonuserpage'         => '현재 사용하는 IP의 사용자 문서',
'tooltip-pt-mytalk'               => '내 토론 문서',
'tooltip-pt-anontalk'             => '현재 사용하는 IP를 위한 사용자 토론 문서',
'tooltip-pt-preferences'          => '사용자 환경 설정',
'tooltip-pt-watchlist'            => '주시문서 목록',
'tooltip-pt-mycontris'            => '내가 편집한 글',
'tooltip-pt-login'                => '꼭 로그인해야 하는 것은 아니지만, 로그인을 권장합니다.',
'tooltip-pt-anonlogin'            => '꼭 필요한 것은 아니지만, 로그인을 하면 편리한 점이 많습니다.',
'tooltip-pt-logout'               => '로그아웃',
'tooltip-ca-talk'                 => '문서의 내용에 대한 토론 문서',
'tooltip-ca-edit'                 => '문서를 편집할 수 있습니다. 저장하기 전에 미리보기를 해 주세요.',
'tooltip-ca-addsection'           => '내용 추가하기',
'tooltip-ca-viewsource'           => '문서가 잠겨 있습니다. 문서의 소스만 볼 수 있습니다.',
'tooltip-ca-history'              => '문서의 과거 버전들',
'tooltip-ca-protect'              => '문서 보호하기',
'tooltip-ca-delete'               => '문서 삭제하기',
'tooltip-ca-undelete'             => '삭제된 문서 복구하기',
'tooltip-ca-move'                 => '문서 이동하기',
'tooltip-ca-watch'                => '이 문서를 주시문서 목록에 추가합니다.',
'tooltip-ca-unwatch'              => '이 문서를 주시문서 목록에서 제거합니다.',
'tooltip-search'                  => '{{SITENAME}} 찾기',
'tooltip-search-go'               => '이 이름의 문서가 존재하면 그 문서로 바로가기',
'tooltip-search-fulltext'         => '이 문자열이 포함된 문서 찾기',
'tooltip-p-logo'                  => '대문',
'tooltip-n-mainpage'              => '대문으로',
'tooltip-n-portal'                => '이 프로젝트에 대해',
'tooltip-n-currentevents'         => '최근의 소식을 봅니다.',
'tooltip-n-recentchanges'         => '이 위키에서 최근 바뀐 점의 목록입니다.',
'tooltip-n-randompage'            => '임의 문서로 갑니다.',
'tooltip-n-help'                  => '도움말',
'tooltip-t-whatlinkshere'         => '여기로 연결된 모든 문서의 목록',
'tooltip-t-recentchangeslinked'   => '여기로 연결된 모든 문서의 변경 내역',
'tooltip-feed-rss'                => '이 문서의 RSS 피드입니다.',
'tooltip-feed-atom'               => '이 문서의 Atom 피드입니다.',
'tooltip-t-contributions'         => '이 사용자의 기여 목록을 봅니다.',
'tooltip-t-emailuser'             => '이 사용자에게 이메일을 보냅니다.',
'tooltip-t-upload'                => '파일을 올립니다.',
'tooltip-t-specialpages'          => '모든 특수 문서의 목록입니다.',
'tooltip-t-print'                 => '이 문서의 인쇄용 버전',
'tooltip-t-permalink'             => '문서의 현재 버전에 대한 고유링크',
'tooltip-ca-nstab-main'           => '문서 내용을 봅니다.',
'tooltip-ca-nstab-user'           => '사용자 문서 내용을 봅니다.',
'tooltip-ca-nstab-media'          => '미디어 문서 내용을 봅니다.',
'tooltip-ca-nstab-special'        => '이것은 특수 문서로, 편집할 수 없습니다.',
'tooltip-ca-nstab-project'        => '프로젝트 문서 내용을 봅니다.',
'tooltip-ca-nstab-image'          => '그림 문서 내용을 봅니다.',
'tooltip-ca-nstab-mediawiki'      => '시스템 메시지 내용을 봅니다.',
'tooltip-ca-nstab-template'       => '틀 문서 내용을 봅니다.',
'tooltip-ca-nstab-help'           => '도움말 문서 내용을 봅니다.',
'tooltip-ca-nstab-category'       => '분류 문서 내용을 봅니다.',
'tooltip-minoredit'               => '사소한 편집으로 표시하기',
'tooltip-save'                    => '편집 내용을 저장하기',
'tooltip-preview'                 => '편집 미리 보기. 저장하기 전에 꼭 미리 보기를 해 주세요.',
'tooltip-diff'                    => '원래의 문서와 현재 편집하는 문서를 비교하기',
'tooltip-compareselectedversions' => '이 문서에서 선택한 두 버전간의 차이를 비교',
'tooltip-watch'                   => '이 문서를 주시문서 목록에 추가',
'tooltip-recreate'                => '문서를 편집하는 중 삭제되어도 새로 만들기',
'tooltip-upload'                  => '파일 올리기 시작',

# Stylesheets
'common.css'   => '/** 이 CSS 설정은 모든 스킨에 적용됩니다 */',
'monobook.css' => '/* 이 CSS 설정은 모든 모노북 스킨에 적용됩니다 */',

# Scripts
'common.js'   => '/* 이 자바스크립트 설정은 모든 문서, 모든 사용자에게 적용됩니다. */',
'monobook.js' => '/* 이 자바스크립트 설정은 모노북 스킨을 사용하는 사용자에게 적용됩니다. */',

# Metadata
'nodublincore'      => '더블린 코어 RDF 메타데이터 기능은 비활성되어 있습니다.',
'nocreativecommons' => '크리에이티브 커먼즈 RDF 메타데이터 기능은 비활성되어 있습니다.',

# Attribution
'anonymous'        => '{{SITENAME}}의 익명 사용자',
'siteuser'         => '{{SITENAME}} 사용자 $1',
'lastmodifiedatby' => '이 문서는 $3에 의해 $2, $1에 마지막으로 바뀌었습니다.', # $1 date, $2 time, $3 user
'othercontribs'    => '$1의 작업을 바탕으로 함.',
'others'           => '기타',
'siteusers'        => '{{SITENAME}} 사용자 $1',
'creditspage'      => '문서 기여자들',
'nocredits'        => '이 문서에서는 기여자 정보가 없습니다.',

# Spam protection
'spamprotectiontitle' => '스팸 방지 필터',
'spamprotectiontext'  => '스팸 필터가 문서 저장을 막았습니다.
외부 사이트로 연결하는 링크 중에 블랙리스트에 포함된 사이트가 있을 것입니다.',
'spamprotectionmatch' => '문제가 되는 부분은 다음과 같습니다: $1',
'spambot_username'    => 'MediaWiki 스팸 제거',
'spam_reverting'      => '$1을 포함하지 않는 최신 버전으로 되돌림',
'spam_blanking'       => '모든 버전에 $1 링크를 포함하고 있어 문서를 비움',

# Info page
'infosubtitle'   => '문서 정보',
'numedits'       => '편집 횟수(문서): $1',
'numtalkedits'   => '편집 횟수(토론 문서): $1',
'numwatchers'    => '주시하는 사용자 수: $1',
'numauthors'     => '기여한 사용자 수(문서): $1',
'numtalkauthors' => '기여한 사용자 수(토론 문서): $1',

# Math options
'mw_math_png'    => '항상 PNG로 표시',
'mw_math_simple' => '아주 간단한 것은 HTML로, 나머지는 PNG로',
'mw_math_html'   => '가능한 한 HTML로, 나머지는 PNG로',
'mw_math_source' => 'TeX로 남겨둠 (텍스트 브라우저용)',
'mw_math_modern' => '최신 웹 브라우저인 경우 추천',
'mw_math_mathml' => '가능하면 MathML로 (시험판)',

# Patrolling
'markaspatrolleddiff'                 => '검토된 것으로 표시',
'markaspatrolledtext'                 => '이 문서를 검토된 것으로 표시',
'markedaspatrolled'                   => '검토된 것으로 표시',
'markedaspatrolledtext'               => '선택한 버전이 검토된 것으로 표시되었습니다.',
'rcpatroldisabled'                    => '최근 바뀜 검토 기능 비활성화됨',
'rcpatroldisabledtext'                => '최근 바뀜 검토 기능은 현재 비활성화되어 있습니다.',
'markedaspatrollederror'              => '검토된 것으로 표시할 수 없습니다.',
'markedaspatrollederrortext'          => '검토된 것으로 표시할 버전을 지정해야 합니다.',
'markedaspatrollederror-noautopatrol' => '자신의 편집을 스스로 검토된 것으로 표시하는 것은 허용되지 않습니다.',

# Patrol log
'patrol-log-page' => '검토 기록',
'patrol-log-line' => '$2 문서의 $1 판을 검토함 $3',
'patrol-log-auto' => '(자동)',

# Image deletion
'deletedrevision'              => '예전 버전 $1이(가) 삭제되었습니다.',
'filedeleteerror-short'        => '파일 삭제 오류: $1',
'filedelete-missing'           => '이 파일이 존재하지 않기 때문에 "$1"을(를) 삭제할 수 없습니다.',
'filedelete-archive-read-only' => '웹 서버의 파일 저장 위치 ‘$1’에 쓰기 권한이 없습니다.',

# Browsing diffs
'previousdiff' => '← 이전 편집',
'nextdiff'     => '다음 편집 →',

# Media information
'mediawarning'         => "'''경고''': 이 파일에는 시스템을 위험하게 만드는 악성 코드가 들어있을 수 있습니다.<hr />",
'imagemaxsize'         => '그림 설명 문서의 그림 크기를 제한:',
'thumbsize'            => '섬네일 크기:',
'widthheightpage'      => '$1×$2, $3페이지',
'file-info'            => '(파일 크기: $1, MIME 종류: $2)',
'file-info-size'       => '($1 × $2 픽셀, 파일 크기: $3, MIME 종류: $4)',
'file-nohires'         => '<small>최대 해상도입니다.</small>',
'svg-long-desc'        => '(SVG 파일, 실제 크기 $1 × $2 픽셀, 파일 크기 $3)',
'show-big-image'       => '최대 해상도',
'show-big-image-thumb' => '<small>미리보기 크기: $1 × $2 픽셀</small>',

# Special:NewImages
'newimages'             => '새 그림 파일 목록',
'imagelisttext'         => '$1개의 파일이 $2 순으로 정렬되어 있습니다.',
'showhidebots'          => '(봇을 $1)',
'noimages'              => '그림이 없습니다.',
'ilsubmit'              => '찾기',
'bydate'                => '날짜',
'sp-newimages-showfrom' => '$1 $2부터 올라온 그림 목록 보기',

# Bad image list
'bad_image_list' => '형식은 아래와 같습니다.

"*"로 시작하는 목록의 내용만 적용됩니다.
매 줄의 첫번째 고리는 부적절한 파일을 가리켜야 합니다.
같은 줄에 따라오는 모든 고리는 예외로 봅니다. (예: 파일이 사용되어야 하는 문서)',

# Metadata
'metadata'          => '메타데이터',
'metadata-help'     => '이 파일은 카메라/스캐너에서 추가한 부가 정보를 담고 있습니다. 파일을 편집한다면, 변경된 그림에는 일부 부가 정보가 빠질 수 있습니다.',
'metadata-expand'   => '자세한 정보 보이기',
'metadata-collapse' => '자세한 정보 숨기기',
'metadata-fields'   => 'EXIF 메타데이터 표가 접혀 있을 때, 다음의 속성값만이 기본적으로 보이게 됩니다. 나머지 값은 자동적으로 숨겨집니다.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-imagedescription'    => '그림 제목',
'exif-make'                => '카메라 제조사',
'exif-model'               => '카메라 모델',
'exif-copyright'           => '저작권자',
'exif-datetimeoriginal'    => '날짜와 시간',
'exif-exposuretime'        => '노출 시간',
'exif-exposuretime-format' => '$1초 ($2)',
'exif-shutterspeedvalue'   => '셔터 속도',

# External editor support
'edit-externally'      => '이 파일을 외부 프로그램을 사용해서 편집하기',
'edit-externally-help' => '[http://www.mediawiki.org/wiki/Manual:External_editors 여기]에서 외부 편집기에 대한 정보를 얻을 수 있습니다.',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => '모두',
'imagelistall'     => '모두',
'watchlistall2'    => '모든 기간',
'namespacesall'    => '모두',
'monthsall'        => '모든 달',

# E-mail address confirmation
'confirmemail'             => '이메일 주소 확인',
'confirmemail_noemail'     => '[[Special:Preferences|환경 설정]]에 이메일을 설정하지 않았습니다.',
'confirmemail_text'        => '{{SITENAME}}에서는 이메일 기능을 사용하기 전에, 이메일 인증을 받아야 합니다. 아래의 버튼을 누르면 인증 메일을 보냅니다. 메일에는 인증 코드가 들어있는 링크가 있습니다. 그 링크를 웹 브라우저로 열면 인증이 완료됩니다.',
'confirmemail_pending'     => '<div class="error">이미 확인 이메일을 보냈습니다. 계정을 최근에 만들었다면 이메일을 보내는 데에 몇 분이 걸릴 수 있으므로 잠시 후에 다시 확인해 주세요.</div>',
'confirmemail_send'        => '인증 코드를 메일로 보내기',
'confirmemail_sent'        => '인증 이메일을 보냈습니다.',
'confirmemail_sendfailed'  => '{{SITENAME}}에서 인증 이메일을 보낼 수 없습니다.
이메일 주소를 잘못 입력했는지 확인해주세요.

메일 서버로부터의 응답: $1',
'confirmemail_invalid'     => '인증 코드가 올바르지 않습니다. 코드가 소멸되었을 수도 있습니다.',
'confirmemail_needlogin'   => '이메일 주소를 인증하려면 $1이 필요합니다.',
'confirmemail_success'     => '이메일 주소가 인증되었습니다. 이제 로그인해서 위키를 사용하세요.',
'confirmemail_loggedin'    => '이메일 주소가 인증되었습니다.',
'confirmemail_error'       => '당신의 인증을 저장하는 도중 오류가 발생했습니다.',
'confirmemail_subject'     => '{{SITENAME}} 이메일 주소 인증',
'confirmemail_body'        => '$1 아이피 주소를 사용하는 사용자가 {{SITENAME}}의 ‘$2’ 계정에 이메일 인증 신청을 했습니다.

이 계정이 당신의 계정이라면, 아래 주소를 열어서 이메일 인증을 해 주세요.

$3

당신의 계정이 아니라면, 이메일 인증 신청을 취소하기 위해 아래의 주소를 열어주세요.

$5

인증 코드는 $4에 만료됩니다.',
'confirmemail_invalidated' => '이메일 확인이 취소됨',
'invalidateemail'          => '이메일 확인 취소',

# Scary transclusion
'scarytranscludedisabled' => '[인터위키가 비활성되어 있습니다]',
'scarytranscludefailed'   => '[$1 틀을 불러오는 데에 실패했습니다]',
'scarytranscludetoolong'  => '[URL이 너무 깁니다]',

# Trackbacks
'trackbackbox'      => '<div id="mw_trackbacks">이 문서에 달린 트랙백:<br />$1</div>',
'trackbackremove'   => ' ([$1 삭제])',
'trackbacklink'     => '트랙백',
'trackbackdeleteok' => '트랙백이 삭제되었습니다.',

# Delete conflict
'deletedwhileediting' => "'''주의''': 당신이 이 문서를 편집하던 중에 이 문서가 삭제되었습니다.",
'confirmrecreate'     => '[[User:$1|$1]]([[User talk:$1|토론]]) 사용자가 당신의 편집 도중 문서를 지웠습니다. 삭제 이유는 다음과 같습니다:
: $2
문서를 다시 되살릴 필요가 있는지를 확인해주세요.',
'recreate'            => '새로 만들기',

# HTML dump
'redirectingto' => '[[:$1]]로 넘어가는 중...',

# action=purge
'confirm_purge'        => '문서의 캐시를 지울까요?

$1',
'confirm_purge_button' => '확인',

# AJAX search
'searchcontaining' => '"$1"이 포함된 글을 검색합니다.',
'searchnamed'      => '"$1" 이름을 가진 문서를 검색합니다.',
'articletitles'    => "''$1''로 시작하는 문서들",
'hideresults'      => '결과 숨기기',
'useajaxsearch'    => 'AJAX 찾기 기능 사용하기',

# Multipage image navigation
'imgmultipageprev' => '← 이전 문서',
'imgmultipagenext' => '다음 문서 →',

# Table pager
'table_pager_next'         => '다음 문서',
'table_pager_prev'         => '이전 문서',
'table_pager_first'        => '처음 문서',
'table_pager_last'         => '마지막 문서',
'table_pager_limit'        => '문서당 $1개 항목 보이기',
'table_pager_limit_submit' => '확인',
'table_pager_empty'        => '결과 없음',

# Auto-summaries
'autosumm-blank'   => '문서의 모든 내용을 삭제',
'autosumm-replace' => '문서 내용을 ‘$1’으로 교체',
'autoredircomment' => '[[$1]](으)로 넘겨주기',
'autosumm-new'     => '새 문서: $1',

# Live preview
'livepreview-loading' => '불러오는 중...',

# Watchlist editor
'watchlistedit-numitems'       => '주시문서 목록에는 토론 문서를 제외하고 $1개의 문서가 들어 있습니다.',
'watchlistedit-noitems'        => '주시문서 목록이 비어 있습니다.',
'watchlistedit-normal-title'   => '주시문서 목록 편집하기',
'watchlistedit-normal-legend'  => '목록에서 문서 제거하기',
'watchlistedit-normal-explain' => "주시문서 목록에서 제거하려는 문서가 있으면, 각 항목의 체크박스를 선책한 다음 '항목 삭제'를 클릭해주세요. 또는 [[Special:Watchlist/raw|목록을 직접 편집할 수도 있습니다]].",
'watchlistedit-normal-submit'  => '항목 삭제',
'watchlistedit-normal-done'    => '주시문서 목록에서 다음 $1개 항목을 삭제했습니다:',
'watchlistedit-raw-title'      => '주시문서 목록 직접 편집하기',
'watchlistedit-raw-legend'     => '주시문서 목록 직접 편집하기',
'watchlistedit-raw-explain'    => "주시문서 목록의 각 항목이 나와 있습니다. 필요한 항목을 직접 추가하거나 제거할 수 있습니다. 각 줄마다 하나의 제목을 쓰고, '주시문서 목록 갱신'을 누르면 됩니다.
또는 [[Special:Watchlist/edit|일반적인 편집창]]을 쓸 수도 있습니다.",
'watchlistedit-raw-titles'     => '목록:',
'watchlistedit-raw-submit'     => '주시문서 목록 갱신',
'watchlistedit-raw-done'       => '주시문서 목록을 갱신했습니다.',
'watchlistedit-raw-added'      => '$1개 문서를 추가했습니다:',
'watchlistedit-raw-removed'    => '$1개 문서를 제거했습니다:',

# Watchlist editing tools
'watchlisttools-view' => '주시문서 최근 바뀜',
'watchlisttools-edit' => '주시문서 목록 보기/편집하기',
'watchlisttools-raw'  => '주시문서 목록 직접 편집하기',

# Special:Version
'version'                      => '버전', # Not used as normal message but as header for the special page itself
'version-extensions'           => '설치된 확장 기능',
'version-specialpages'         => '특수 문서',
'version-parserhooks'          => '파서 훅',
'version-other'                => '기타',
'version-extension-functions'  => '확장 함수',
'version-parser-extensiontags' => '파서 확장 태그',
'version-hook-name'            => '훅 이름',
'version-version'              => '버전',
'version-license'              => '라이센스',
'version-software'             => '설치된 프로그램',
'version-software-product'     => '제품',
'version-software-version'     => '버전',

# Special:FilePath
'filepath'         => '파일 주소',
'filepath-page'    => '파일:',
'filepath-submit'  => '주소',
'filepath-summary' => '파일의 전체 주소를 알려줍니다.
그림은 최고 해상도로 보여주고 다른 파일은 연결된 프로그램으로 바로 실행합니다.

"{{ns:image}}:" 접두어를 빼고 파일 이름만 넣으세요.',

# Special:FileDuplicateSearch
'fileduplicatesearch'          => '중복 파일 찾기',
'fileduplicatesearch-summary'  => '해시 값으로 중복 파일을 찾습니다.

"{{ns:image}}:" 접두어를 빼고 파일 이름만 넣으세요.',
'fileduplicatesearch-legend'   => '중복 찾기',
'fileduplicatesearch-filename' => '파일이름:',
'fileduplicatesearch-submit'   => '찾기',
'fileduplicatesearch-info'     => '$1 × $2 픽셀<br />파일 크기: $3<br />MIME 유형: $4',
'fileduplicatesearch-result-1' => '"$1"은 중복 파일이 없습니다.',
'fileduplicatesearch-result-n' => '"$1"파일은 $2개의 중복 파일이 있습니다.',

# Special:SpecialPages
'specialpages'                   => '특수 문서 목록',
'specialpages-group-maintenance' => '관리용 목록',
'specialpages-group-other'       => '다른 특수문서',
'specialpages-group-login'       => '로그인 / 등록',
'specialpages-group-changes'     => '최근 바뀜과 기록',
'specialpages-group-media'       => '파일 관리',
'specialpages-group-users'       => '사용자와 권한',
'specialpages-group-highuse'     => '많이 쓰이는 문서 목록',

);
