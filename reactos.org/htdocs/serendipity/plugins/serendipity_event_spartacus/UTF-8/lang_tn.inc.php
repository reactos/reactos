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

        @define('PLUGIN_EVENT_SPARTACUS_NAME', 'Spartacus (外掛存庫)');
        @define('PLUGIN_EVENT_SPARTACUS_DESC', '[S]erendipity [P]lugin [A]ccess [R]epository [T]ool [A]nd [C]ustomization/[U]nification [S]ystem - 允許你從 s9y 的線上存庫下載外掛');
        @define('PLUGIN_EVENT_SPARTACUS_FETCH', '點這裡從 s9y 的外掛存庫安裝新 %s');
        @define('PLUGIN_EVENT_SPARTACUS_FETCHERROR', '網址 %s 無法開啟。也許 s9y 或 SourceForge.net 的主機有問題 - 請稍後在試。');
        @define('PLUGIN_EVENT_SPARTACUS_FETCHING', '打開網址 %s...');
        @define('PLUGIN_EVENT_SPARTACUS_FETCHED_BYTES_URL', '從上面的網址接收 %s 個位元組的資料。儲存成檔案 %s...');
        @define('PLUGIN_EVENT_SPARTACUS_FETCHED_BYTES_CACHE', '從主機內接收 %s 個位元組的資料。儲存成檔案 %s...');
        @define('PLUGIN_EVENT_SPARTACUS_FETCHED_DONE', '存取資料完成。');
        @define('PLUGIN_EVENT_SPARTACUS_MIRROR_XML', '檔案/鏡像 位址 (XML metadata)');
		@define('PLUGIN_EVENT_SPARTACUS_MIRROR_FILES', '檔案/鏡像 位址 (檔案)');
		@define('PLUGIN_EVENT_SPARTACUS_MIRROR_DESC', '選擇一個下載點。不要改變這個設定值初非您了解它的作用。這個選項主要是為了相容性所設計。');
		@define('PLUGIN_EVENT_SPARTACUS_CHOWN', '下載檔案的擁有人');
		@define('PLUGIN_EVENT_SPARTACUS_CHOWN_DESC', '這裡可以輸入 Spartacus 所下載的檔案 (FTP/Shell) 擁有人 (譬如 "nobody")。如果空白不會做任何改變。');
		@define('PLUGIN_EVENT_SPARTACUS_CHMOD', '下載檔案的權限');
		@define('PLUGIN_EVENT_SPARTACUS_CHMOD_DESC', '這裡可以輸入 Spartacus 所下載的檔案 (FTP/Shell) 權限 (譬如 "0777")。如果空白將會使用主機的預設權限。記得不是每個主機都允許改變權限。記得將權限改為可讀取和寫入檔案，不然 Spartacus/Serendipity 會無法覆蓋檔案和改變設定。');
		@define('PLUGIN_EVENT_SPARTACUS_CHMOD_DIR', '檔案目錄的權限');
		@define('PLUGIN_EVENT_SPARTACUS_CHMOD_DIR_DESC', '這裡可以輸入 Spartacus 所下載的檔案的目錄 (FTP/Shell) 權限 (譬如 "0777")。如果空白將會使用主機的預設權限。記得不是每個主機都允許改變權限。記得將權限改為可讀取和寫入檔案，不然 Spartacus/Serendipity 會無法覆蓋檔案和改變設定。');
?>