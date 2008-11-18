<?php # $Id: lang_ko.inc.php,v 1.0 2005/06/29 13:41:13 garvinhicking Exp $
# Translated by: Wesley Hwang-Chung <wesley96@gmail.com> 
# (c) 2005 http://www.tool-box.info/

        @define('PLUGIN_EVENT_ENTRYPROPERTIES_TITLE', '글에 대한 확장 속성');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_DESC', '(캐시, 비공개 글, 꼭대기 글)');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_STICKYPOSTS', '이 글을 꼭대기 글로 사용');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_ACCESS', '이 글의 읽기 허용 범위');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_ACCESS_PRIVATE', '작성자 자신만');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_ACCESS_MEMBERS', '다른 작성자까지');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_ACCESS_PUBLIC', '모두에게 공개');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE', '글을 캐시함');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_DESC', '사용할 경우 글을 저장할 때마다 캐시된 버전을 생성합니다. 캐시를 사용하면 성능을 향상시킬 수 있으나 다른 플러그인의 작동 유연성을 떨어뜨릴 수 있습니다.');
        @define('PLUGIN_EVENT_ENTRYPROPERTY_BUILDCACHE', '글에 대한 캐시 생성');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_FETCHNEXT', '다음 글 묶음을 불러오는 중...');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_FETCHNO', '%d번과 %d번 사이의 글을 불러오는 중');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_BUILDING', '%d번 글 <em>%s</em>에 대한 캐시를 생성중...');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHED', '글이 캐시되었습니다.');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_DONE', '글에 대한 캐시 생성을 마쳤습니다.');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_ABORTED', '캐시 생성이 중단되었습니다.');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_TOTAL', ' (총 %d개의 글)...');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_NL2BR', 'nl2br 플러그인 사용 안 함');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_NO_FRONTPAGE', '첫 페이지/정리 페이지에서 글을 숨김');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_GROUPS', '그룹 기반 제한 사용');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_GROUPS_DESC', '사용할 경우 글을 읽을 수 있는 사용자의 그룹을 정의할 수 있습니다. 정리 페이지 표시 성능에 상당한 영향을 주기 때문에 이 옵션은 꼭 필요할 때만 사용하시기 바랍니다.');

?>
