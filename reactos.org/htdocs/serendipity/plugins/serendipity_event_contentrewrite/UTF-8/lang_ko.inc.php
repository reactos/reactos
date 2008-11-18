<?php # $Id: lang_ko.inc.php,v 1.0 2005/06/29 13:41:13 garvinhicking Exp $
# Translated by: Wesley Hwang-Chung <wesley96@gmail.com> 
# (c) 2005 http://www.tool-box.info/

        @define('PLUGIN_EVENT_CONTENTREWRITE_FROM', '원본');
        @define('PLUGIN_EVENT_CONTENTREWRITE_TO', '변환');
        @define('PLUGIN_EVENT_CONTENTREWRITE_NAME', '글 내용 변환기');
        @define('PLUGIN_EVENT_CONTENTREWRITE_DESCRIPTION', '특정 단어를 지정한 문장으로 모두 변환함 (약자를 적었을 때 유용합니다)');
        @define('PLUGIN_EVENT_CONTENTREWRITE_NEWTITLE', '새 제목');
        @define('PLUGIN_EVENT_CONTENTREWRITE_NEWTDESCRIPTION', '새 아이템의 제목(약자)을 여기에 적으십시오 ({원본})');
        @define('PLUGIN_EVENT_CONTENTREWRITE_OLDTITLE', '%d번 제목');
        @define('PLUGIN_EVENT_CONTENTREWRITE_OLDTDESCRIPTION', '약자를 여기에 적으십시오 ({원본})');
        @define('PLUGIN_EVENT_CONTENTREWRITE_PTITLE', '플러그인 제목');
        @define('PLUGIN_EVENT_CONTENTREWRITE_PDESCRIPTION', '이 플러그인에 대한 제목');
        @define('PLUGIN_EVENT_CONTENTREWRITE_NEWDESCRIPTION', '새 설명 문장');
        @define('PLUGIN_EVENT_CONTENTREWRITE_NEWDDESCRIPTION', '새 아이템의 설명을 여기에 적으십시오 ({변환})');
        @define('PLUGIN_EVENT_CONTENTREWRITE_OLDDESCRIPTION', '%s번 설명 문장');
        @define('PLUGIN_EVENT_CONTENTREWRITE_OLDDDESCRIPTION', '설명을 여기에 적으십시오 ({변환})');
        @define('PLUGIN_EVENT_CONTENTREWRITE_REWRITESTRING', '변환할 문자열');
        @define('PLUGIN_EVENT_CONTENTREWRITE_REWRITESTRINGDESC', '변환을 할 때 쓸 문자열을 지정합니다. 변환을 원하는 위치에 {원본}과 {변환}을 적으십시오.' . "\n" . '용례: <acronym title="{to}">{from}</acronym>');
        @define('PLUGIN_EVENT_CONTENTREWRITE_REWRITECHAR', '변환할 문자');
        @define('PLUGIN_EVENT_CONTENTREWRITE_REWRITECHARDESC', '강제로 변환하기 위해 단어 끝에 붙이는 문자가 있다면 여기에 지정합니다. \'serendipity*\'를 다른 단어로 변환하는 경우 \'*\'는 제거하고 싶으면 해당 문자를 여기에 적게 됩니다.');

?>
