<?php # $Id: $
##########################################################################
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity    #
# Developer Team) All rights reserved.  See LICENSE file for licensing   #
# details								                                 #
#                                                                        #
# (c) 2003 Jannis Hermanns <J@hacked.it>                                 #
# http://www.jannis.to/programming/serendipity.html                      #
#                                                                        #
# Translated by                                                          #
# (c) 2004-2005 CapriSkye <admin@capriskye.com>                          #
#               http://open.38.com                                       #
##########################################################################

        @define('PLUGIN_EVENT_CONTENTREWRITE_FROM', '改');
        @define('PLUGIN_EVENT_CONTENTREWRITE_TO', '到');
        @define('PLUGIN_EVENT_CONTENTREWRITE_NAME', '內容改寫');
        @define('PLUGIN_EVENT_CONTENTREWRITE_DESCRIPTION', '覆蓋選擇的字串 (可用於縮寫字)');
        @define('PLUGIN_EVENT_CONTENTREWRITE_NEWTITLE', '新改寫名稱');
        @define('PLUGIN_EVENT_CONTENTREWRITE_NEWTDESCRIPTION', '輸入縮寫字的名稱 ({改})');
        @define('PLUGIN_EVENT_CONTENTREWRITE_OLDTITLE', '改寫名稱 #%d');
        @define('PLUGIN_EVENT_CONTENTREWRITE_OLDTDESCRIPTION', '輸入縮寫字 ({改})');
        @define('PLUGIN_EVENT_CONTENTREWRITE_PTITLE', '外掛名稱 (內容改寫)');
        @define('PLUGIN_EVENT_CONTENTREWRITE_PDESCRIPTION', '內容改寫的外掛名稱');
        @define('PLUGIN_EVENT_CONTENTREWRITE_NEWDESCRIPTION', '新改寫字');
        @define('PLUGIN_EVENT_CONTENTREWRITE_NEWDDESCRIPTION', '輸入新的改寫字 ({到})');
        @define('PLUGIN_EVENT_CONTENTREWRITE_OLDDESCRIPTION', '改寫字 #%s');
        @define('PLUGIN_EVENT_CONTENTREWRITE_OLDDDESCRIPTION', '輸入改寫字 ({到})');
        @define('PLUGIN_EVENT_CONTENTREWRITE_REWRITESTRING', '改寫字串');
        @define('PLUGIN_EVENT_CONTENTREWRITE_REWRITESTRINGDESC', '改寫的字串。將 {改} 和 {到} 放在要改寫的地方。' . "\n" . '範例: <acronym title="{到}">{改}</acronym>');
        @define('PLUGIN_EVENT_CONTENTREWRITE_REWRITECHAR', '改寫符號');
        @define('PLUGIN_EVENT_CONTENTREWRITE_REWRITECHARDESC', '改寫符號可以用來分辨應該改寫和不該改寫的字。如果改寫符號是 * 那只輸入 \'s9y\' 並不會被改寫，除非輸入 \'s9y*\'。');
?>