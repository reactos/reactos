<?php # $Id: $
##########################################################################
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity    #
# Developer Team) All rights reserved.  See LICENSE file for licensing   #
# details							                                	 #
#                                                                        #
# (c) 2003 Jannis Hermanns <J@hacked.it>                                 #
# http://www.jannis.to/programming/serendipity.html                      #
#                                                                        #
# Translated by                                                          #
# (c) 2004-2005 CapriSkye <admin@capriskye.com>                          #
#               http://open.38.com                                       #
##########################################################################

        @define('PLUGIN_EVENT_MAILER_NAME', '寄送文章');
        @define('PLUGIN_EVENT_MAILER_DESC', '可讓您寄送新發佈的文章到電子郵件信箱');
        @define('PLUGIN_EVENT_MAILER_RECIPIENT', '收件人');
        @define('PLUGIN_EVENT_MAILER_RECIPIENTDESC', '收件人的電子信箱 (建議：電子報)');
        @define('PLUGIN_EVENT_MAILER_LINK', '包括文章連結？');
        @define('PLUGIN_EVENT_MAILER_LINKDESC', '郵件內包括文章的連結位址。');
        @define('PLUGIN_EVENT_MAILER_STRIPTAGS', '移除 HTML？');
        @define('PLUGIN_EVENT_MAILER_STRIPTAGSDESC', '移除郵件內的 HTML。');
        @define('PLUGIN_EVENT_MAILER_CONVERTP', '轉換 HTML 段落成新行？');
		@define('PLUGIN_EVENT_MAILER_CONVERTPDESC', '加入新行於每個 HTML 的段落。如果您開啟 移除 HTML？ 的功能，這會檢查每個未自己段行的段落，然後自動幫您段行。');
?>