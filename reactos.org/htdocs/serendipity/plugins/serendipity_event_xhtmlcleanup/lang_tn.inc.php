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

        @define('PLUGIN_EVENT_XHTMLCLEANUP_NAME', '修復常見的 XHTML 錯誤');
        @define('PLUGIN_EVENT_XHTMLCLEANUP_DESC', '這個外掛會在文章內修復一些常見的 XHTML 錯誤，為了能讓您的文章通過 XHTML 查驗。');
		@define('PLUGIN_EVENT_XHTMLCLEANUP_XHTML', '編碼 XML 解析的資料？');
		@define('PLUGIN_EVENT_XHTMLCLEANUP_XHTML_DESC', '這個外掛使用 XML 解析的方法來保證 XHTML 能通過驗證。這個功能可能會轉換已正確的語法，所以這個外掛會在解析完畢後才進行編碼。如果遇到編碼的錯誤，請不要使用這個功能。');
		@define('PLUGIN_EVENT_XHTMLCLEANUP_UTF8', '清除 UTF-8 標籤？');
		@define('PLUGIN_EVENT_XHTMLCLEANUP_UTF8_DESC', '如果開啟，任何 UTF-8 字元裡的 HTML 標籤會正確的轉換和輸出。');
?>