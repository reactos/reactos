<?php
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

        @define('PLUGIN_EVENT_ENTRYPROPERTIES_TITLE', '文章的進階屬性');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_DESC', '(快取資料, 不開放文章, 置頂文章)');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_STICKYPOSTS', '標示為置頂文章');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_ACCESS', '誰可以閱讀文章');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_ACCESS_PRIVATE', '自己');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_ACCESS_MEMBER', '副作者');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_ACCESS_PUBLIC', '訪客');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE', '允許快取文章？');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_DESC', '如果開啟，每次儲存文章時都會建立快取資料。快取資料可以增加速度，但可能有些外掛無法相容。');
        @define('PLUGIN_EVENT_ENTRYPROPERTY_BUILDCACHE', '建立快取資料');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_FETCHNEXT', '擷取其他文章...');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_FETCHNO', '擷取文章 %d 到 %d');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_BUILDING', '建立此文章的快取資料 #%d, <em>%s</em>...');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHED', '取得快取資料');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_DONE', '快取完成。');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_ABORTED', '快取資料取消。');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_TOTAL', ' (總共 %d 篇文章)...');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_NL2BR', '關閉 nl2br');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_NO_FRONTPAGE', '隱藏於文章頁面 / 主頁面');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_GROUPS', '使用群組限制');
		@define('PLUGIN_EVENT_ENTRYPROPERTIES_GROUPS_DESC', '如果開啟，您可以選擇哪個群組的成員可以流覽文章。這個功能會影響您的網誌速度，除非必要最好不要使用。');
		@define('PLUGIN_EVENT_ENTRYPROPERTIES_USERS', '使用會員限制');
		@define('PLUGIN_EVENT_ENTRYPROPERTIES_USERS_DESC', '如果開啟，您可以選擇哪個會員可以瀏覽文章。這個功能會影響您的網誌速度，除非必要最好不要使用。');
		@define('PLUGIN_EVENT_ENTRYPROPERTIES_HIDERSS', 'RSS 內隱藏內容');
		@define('PLUGIN_EVENT_ENTRYPROPERTIES_HIDERSS_DESC', '如果開啟，文章的內容不會顯示於 RSS 內。');

		@define('PLUGIN_EVENT_ENTRYPROPERTIES_CUSTOMFIELDS', '自訂欄位');
		define('PLUGIN_EVENT_ENTRYPROPERTIES_CUSTOMFIELDS_DESC1', '您可以在模板裡面顯示額外的欄位。您必須編輯 entries.tpl 的模板檔案然後將 Smarty 標籤 {$entry.properties.ep_MyCustomField} 放入您要它顯示的 HTML 檔案裡面。 注意每個欄位的前置字元 ep_。');
		define('PLUGIN_EVENT_ENTRYPROPERTIES_CUSTOMFIELDS_DESC2', '這裡可以輸入每個以逗點分開的欄位名稱 - 不要用特殊字元或空格。範例："Customfield1, Customfield2". ' . PLUGIN_EVENT_ENTRYPROPERTIES_CUSTOMFIELDS_DESC1);
		@define('PLUGIN_EVENT_ENTRYPROPERTIES_CUSTOMFIELDS_DESC3', '每個自訂欄位可以在這裡改變 <a href="%s" target="_blank" title="' . PLUGIN_EVENT_ENTRYPROPERTIES_TITLE . '">外掛設定</a>。');

?>