<?php # $Id: lang_ko.inc.php,v 1.0 2005/06/29 13:41:13 garvinhicking Exp $
# Translated by: Wesley Hwang-Chung <wesley96@gmail.com> 
# (c) 2005 http://www.tool-box.info/

        @define('PLUGIN_EVENT_SPAMBLOCK_TITLE', '스팸 방지기');
        @define('PLUGIN_EVENT_SPAMBLOCK_DESC', '덧글 스팸을 방지하기 위한 다양한 방법 제공');
        @define('PLUGIN_EVENT_SPAMBLOCK_ERROR_BODY', '스팸 방지: 내용이 유효하지 않습니다.');
        @define('PLUGIN_EVENT_SPAMBLOCK_ERROR_IP', '스팸 방지: 덧글을 단 후 곧바로 추가 덧글을 달 수 없습니다.');

        @define('PLUGIN_EVENT_SPAMBLOCK_ERROR_KILLSWITCH', '이 블로그는 "덧글 방지 비상 모드" 상태입니다. 잠시 후 다시 방문해주시기 바랍니다.');
        @define('PLUGIN_EVENT_SPAMBLOCK_BODYCLONE', '중복 덧글 방지');
        @define('PLUGIN_EVENT_SPAMBLOCK_BODYCLONE_DESC', '사용자가 앞서 작성한 덧글과 같은 본문을 담고 있는 덧글을 달지 못하도록 함');
        @define('PLUGIN_EVENT_SPAMBLOCK_KILLSWITCH', '비상으로 덧글 방지');
        @define('PLUGIN_EVENT_SPAMBLOCK_KILLSWITCH_DESC', '모든 글에 대해 임시로 덧글이 달리지 못하도록 합니다. 블로그가 스팸 공격을 받을 경우 유용합니다.');
        @define('PLUGIN_EVENT_SPAMBLOCK_IPFLOOD', 'IP 블로킹 간격');
        @define('PLUGIN_EVENT_SPAMBLOCK_IPFLOOD_DESC', '특정 IP가 덧글을 n분에 한 번씩만 작성할 수 있도록 합니다. 덧글 도배를 방지하는데 유용합니다.');
        @define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS', 'Captcha 사용');
        @define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_DESC', '사용자가 특별 제작된 그림에 나타난 무작위 문자열을 입력하도록 강제합니다. 블로그에 자동화된 덧글이 달리는 것을 방지해줍니다. 시력이 떨어지는 사람은 captcha를 읽는데 곤란해 할 수 있음을 유의하십시오.');
        @define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_USERDESC', '자동화된 봇(bot)이 덧글을 도배하는 것을 방지하기 위해서 아래에 표시된 그림에 나타난 문자열을 입력상자에 입력해주십시오. 문자열이 일치할 경우에만 덧글이 달립니다. 브라우저가 쿠키를 허용해야 정상적으로 검사가 이루어집니다.');
        @define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_USERDESC2', '여기에 보이는 문자열을 입력상자에 입력하십시오.');
        @define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_USERDESC3', '위에 표시된 스팸 방지 그림에 담긴 문자열을 여기에 적으십시오: ');
        @define('PLUGIN_EVENT_SPAMBLOCK_ERROR_CAPTCHAS', '스팸 방지 그림에 담긴 문자열을 정확하게 입력하지 않았습니다. 그림을 확인하시고 나타난 문자열을 다시 입력하십시오.');
        @define('PLUGIN_EVENT_SPAMBLOCK_ERROR_NOTTF', 'Captcha를 이 서버에서 사용할 수 없습니다. GDLib와 freetype 라이브러리를 PHP에 맞게 컴파일해야 하고 디렉토리에 .TTF 파일이 존재해야 합니다.');

        @define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_TTL', '특정 일수가 지나면 captcha를 강제 사용');
        @define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_TTL_DESC', '글이 게시된 일수에 따라 captcha를 강제할 수 있습니다. 며칠이 지나야 captcha를 사용하게 될지 입력하십시오. 0으로 설정할 경우 captcha를 항상 사용하게 됩니다.');
        @define('PLUGIN_EVENT_SPAMBLOCK_FORCEMODERATION', '특정 일수가 지나면 덧글을 검토 대상으로 강제하기');
        @define('PLUGIN_EVENT_SPAMBLOCK_FORCEMODERATION_DESC', '글에 달리는 덧글을 자동으로 검토 대상이 되도록 설정할 수 있습니다. 자동 검토 대상이 되는 시점을 일수로 입력하십시오. 0을 입력하면 자동 검토 대상 기능을 사용하지 않습니다.');
        @define('PLUGIN_EVENT_SPAMBLOCK_LINKS_MODERATE', '덧글이 검토 대상이 되는 링크 수');
        @define('PLUGIN_EVENT_SPAMBLOCK_LINKS_MODERATE_DESC', '덧글 내의 링크가 특정 개수를 넘을 경우 해당 덧글이 자동으로 검토 대상이 되도록 할 수 있습니다. 0을 입력하면 링크 수 확인을 하지 않습니다.');
        @define('PLUGIN_EVENT_SPAMBLOCK_LINKS_REJECT', '덧글이 거부 대상이 되는 링크 수');
        @define('PLUGIN_EVENT_SPAMBLOCK_LINKS_REJECT_DESC', '덧글 내의 링크가 특정 개수를 넘을 경우 해당 덧글이 자동으로 거부될 수 있도록 할 수 있습니다. 0을 입력하면 링크 수 확인을 하지 않습니다.');

        @define('PLUGIN_EVENT_SPAMBLOCK_NOTICE_MODERATION', '작성한 덧글은 지정된 조건 때문에 이 블로그의 운영자에 의해 먼저 검토를 거치게 되도록 표시되었습니다.');
        @define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHA_COLOR', 'captcha의 배경색');
        @define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHA_COLOR_DESC', 'RGB 값을 이런 식으로 입력하십시오: 0,255,255');

        @define('PLUGIN_EVENT_SPAMBLOCK_LOGFILE', '로그 파일 위치');
        @define('PLUGIN_EVENT_SPAMBLOCK_LOGFILE_DESC', '거부되었거나 검토 대상이 된 글은 로그 파일에 기록되도록 할 수 있습니다. 로그 기록을 하지 않으려면 비워두십시오.');

        @define('PLUGIN_EVENT_SPAMBLOCK_REASON_KILLSWITCH', '덧글 방지 비상 모드');
        @define('PLUGIN_EVENT_SPAMBLOCK_REASON_BODYCLONE', '중복 덧글');
        @define('PLUGIN_EVENT_SPAMBLOCK_REASON_IPFLOOD', 'IP 블로킹');
        @define('PLUGIN_EVENT_SPAMBLOCK_REASON_CAPTCHAS', 'captcha 질못 입력됨 (입력값: %s, 기대값: %s)');
        @define('PLUGIN_EVENT_SPAMBLOCK_REASON_FORCEMODERATION', 'X일이 지난 후 자동 검토 대상');
        @define('PLUGIN_EVENT_SPAMBLOCK_REASON_LINKS_REJECT', '허용 하이퍼링크 수 초과');
        @define('PLUGIN_EVENT_SPAMBLOCK_REASON_LINKS_MODERATE', '허용 하이퍼링크 수 초과');
        @define('PLUGIN_EVENT_SPAMBLOCK_HIDE_EMAIL', '덧글 다는 사용자의 전자우편 주소 숨김');
        @define('PLUGIN_EVENT_SPAMBLOCK_HIDE_EMAIL_DESC', '덧글을 다는 사용자의 전자우편 주소를 보여주지 않음');
        @define('PLUGIN_EVENT_SPAMBLOCK_HIDE_EMAIL_NOTICE', '전자우편 주소는 보여지지 않으며 전자우편으로 통보를 할 때만 사용됩니다.');

        @define('PLUGIN_EVENT_SPAMBLOCK_LOGTYPE', '로그 기록 방법 선택');
        @define('PLUGIN_EVENT_SPAMBLOCK_LOGTYPE_DESC', '거부된 덧글은 데이터베이스나 일반 텍스트 파일에 기록될 수 있습니다.');
        @define('PLUGIN_EVENT_SPAMBLOCK_LOGTYPE_FILE', '파일 ("로그 파일" 옵션 참조)');
        @define('PLUGIN_EVENT_SPAMBLOCK_LOGTYPE_DB', '데이터베이스');
        @define('PLUGIN_EVENT_SPAMBLOCK_LOGTYPE_NONE', '로그 기록 안 함');

        @define('PLUGIN_EVENT_SPAMBLOCK_API_COMMENTS', 'API를 통해 생성되는 덧글의 처리 방법');
        @define('PLUGIN_EVENT_SPAMBLOCK_API_COMMENTS_DESC', 'API 호출(트랙백, WFW:commentAPI를 통한 덧글)을 통해 생성된 덧글을 어떻게 검토할지 결정합니다. "검토 대상"으로 설정하면 승인 절차가 필요하게 됩니다. "거부"로 설정하면 모두 거부됩니다. "없음"으로 설정하면 일반 덧글과 동일하게 취급됩니다.');
        @define('PLUGIN_EVENT_SPAMBLOCK_API_MODERATE', '검토 대상');
        @define('PLUGIN_EVENT_SPAMBLOCK_API_REJECT', '거부');
        @define('PLUGIN_EVENT_SPAMBLOCK_REASON_API', 'API로 생성된 덧글(트랙백 등)은 허용되지 않음');

        @define('PLUGIN_EVENT_SPAMBLOCK_FILTER_ACTIVATE', '단어 필터 적용');
        @define('PLUGIN_EVENT_SPAMBLOCK_FILTER_ACTIVATE_DESC', '특정 문자열이 덧글 내에서 검색되면 스팸으로 판정합니다.');

        @define('PLUGIN_EVENT_SPAMBLOCK_FILTER_URLS', '인터넷 주소에 대한 단어 필터');
        @define('PLUGIN_EVENT_SPAMBLOCK_FILTER_URLS_DESC', '정규 표현식을 사용할 수 있으며 개별 문자열은 세미콜론(;)으로 분리합니다.');
        @define('PLUGIN_EVENT_SPAMBLOCK_FILTER_AUTHORS', '작성자 이름에 대한 단어 필터');
        @define('PLUGIN_EVENT_SPAMBLOCK_FILTER_AUTHORS_DESC', '정규 표현식을 사용할 수 있으며 개별 문자열은 세미콜론(;)으로 분리합니다.');

?>
