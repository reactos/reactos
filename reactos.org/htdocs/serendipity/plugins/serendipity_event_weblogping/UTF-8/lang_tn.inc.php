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

        @define('PLUGIN_EVENT_WEBLOGPING_PING', '新文章通知 (via XML-RPC ping) 到：');
        @define('PLUGIN_EVENT_WEBLOGPING_SENDINGPING', '傳送 XML-RPC ping 到 %s');
        @define('PLUGIN_EVENT_WEBLOGPING_TITLE', '新文章通知');
        @define('PLUGIN_EVENT_WEBLOGPING_DESC', '傳送新文章的通知到線上服務功能');
        @define('PLUGIN_EVENT_WEBLOGPING_SUPERSEDES', '(取代 %s)');
        @define('PLUGIN_EVENT_WEBLOGPING_CUSTOM', '自訂的通知功能');
        @define('PLUGIN_EVENT_WEBLOGPING_CUSTOM_BLAHBLA', '如果有自訂的通知功能可以在這輸入，用逗點 (,) 來分開每個功能。使用範例："host.domain/path"。如果主機名前面有星號 (*)，額外的 XML-RPC 資料會傳送給主機 (如果主機支援)。');
		@define('PLUGIN_EVENT_WEBLOGPING_SEND_FAILURE', '失敗 (原因：%s)');
		@define('PLUGIN_EVENT_WEBLOGPING_SEND_SUCCESS', '成功！');
?>