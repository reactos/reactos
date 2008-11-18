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

        @define('PLUGIN_EVENT_SPAMBLOCK_TITLE', '阻擋保護');
        @define('PLUGIN_EVENT_SPAMBLOCK_DESC', '阻擋垃圾迴響的保護方法');
        @define('PLUGIN_EVENT_SPAMBLOCK_ERROR_BODY', '阻擋保護：錯誤訊息。');
        @define('PLUGIN_EVENT_SPAMBLOCK_ERROR_IP', '阻擋保護：您不能連續發佈迴響。');

        @define('PLUGIN_EVENT_SPAMBLOCK_ERROR_KILLSWITCH', '此網誌開啟了 "緊急迴響保護模式"，請稍後在試');
        @define('PLUGIN_EVENT_SPAMBLOCK_BODYCLONE', '不允許相同迴響');
        @define('PLUGIN_EVENT_SPAMBLOCK_BODYCLONE_DESC', '不允許訪客在同一篇文章內發佈相同的迴響');
        @define('PLUGIN_EVENT_SPAMBLOCK_KILLSWITCH', '緊急迴響保護模式');
        @define('PLUGIN_EVENT_SPAMBLOCK_KILLSWITCH_DESC', '暫時關閉全部文章的迴響功能。可在遭受垃圾廣告攻擊時開啟。');
        @define('PLUGIN_EVENT_SPAMBLOCK_IPFLOOD', 'IP 阻擋時間');
        @define('PLUGIN_EVENT_SPAMBLOCK_IPFLOOD_DESC', '主允許同一個 IP 在 X 時間內發佈迴響。可避免遭受迴響灌水。');
        @define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS', '開啟 Captchas');
        @define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_DESC', '訪客要留言時必須輸入圖片裡出現的文字。可允許自動廣告程式輸入迴響。記得有些訪客可能無法解讀圖片裡的文字。');
        @define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_USERDESC', '為了避免自動廣告程式輸入迴響，請輸入圖片裡的文字。如果文字正確，迴響就可以正常發佈。記得瀏覽器必須支援 cookies 要不然您的迴響無法通過測驗。');
        @define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_USERDESC2', '請輸入欄位裡出現的文字。');
        @define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_USERDESC3', '請輸入圖片裡出現的文字： ');
        @define('PLUGIN_EVENT_SPAMBLOCK_ERROR_CAPTCHAS', '您輸入的文字錯誤，請檢查是否輸入正確。');
        @define('PLUGIN_EVENT_SPAMBLOCK_ERROR_NOTTF', '您的主機不支援 Captchas。您的 PHP 需要 GDLib 和 freetype 存庫，和 .TTF 檔案必須在您的目錄內。');

        @define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_TTL', '數天後強制開啟 captchas');
        @define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_TTL_DESC', 'Captchas 可以在數天之後自動開啟。請輸入要開啟 captchas 的天數。如果輸入 0，captchas 會永遠開啟。');
        @define('PLUGIN_EVENT_SPAMBLOCK_FORCEMODERATION', '數天後強制開啟迴響管理');
        @define('PLUGIN_EVENT_SPAMBLOCK_FORCEMODERATION_DESC', '您可以設定是否管理迴響的發佈。輸入文章發佈的時間，之後全部的迴響都需要管理員審核。輸入 0 表示每個迴響都不需要管理審核。');
        @define('PLUGIN_EVENT_SPAMBLOCK_LINKS_MODERATE', '多少連結需要管理審核');
        @define('PLUGIN_EVENT_SPAMBLOCK_LINKS_MODERATE_DESC', '當迴響裡的連結超過限制就必須要管理員的審核才會發佈。輸入 0 如果不檢查連結數量。');
        @define('PLUGIN_EVENT_SPAMBLOCK_LINKS_REJECT', '多少連結會自動拒絕');
        @define('PLUGIN_EVENT_SPAMBLOCK_LINKS_REJECT_DESC', '當迴響裡的連結超過限制就不會通過自動管理。輸入 0 如果不檢查連結數量。');

        @define('PLUGIN_EVENT_SPAMBLOCK_NOTICE_MODERATION', '因為迴響的限制，您的迴響需通過作者的審核。');
        @define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHA_COLOR', 'captcha 的背景顏色');
        @define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHA_COLOR_DESC', '輸入 RGB 顏色值：0,255,255');

        @define('PLUGIN_EVENT_SPAMBLOCK_LOGFILE', '記錄檔案位址');
        @define('PLUGIN_EVENT_SPAMBLOCK_LOGFILE_DESC', '拒絕/管理 的文章會記錄到檔案內。如果不想記錄可以輸入空白。');

        @define('PLUGIN_EVENT_SPAMBLOCK_REASON_KILLSWITCH', '緊急迴響保護');
        @define('PLUGIN_EVENT_SPAMBLOCK_REASON_BODYCLONE', '相同迴響');
        @define('PLUGIN_EVENT_SPAMBLOCK_REASON_IPFLOOD', 'IP 阻擋');
        @define('PLUGIN_EVENT_SPAMBLOCK_REASON_CAPTCHAS', '錯誤 captcha (輸入：%s，正確：%s)');
        @define('PLUGIN_EVENT_SPAMBLOCK_REASON_FORCEMODERATION', '數天後自動管理');
        @define('PLUGIN_EVENT_SPAMBLOCK_REASON_LINKS_REJECT', '連結過多');
        @define('PLUGIN_EVENT_SPAMBLOCK_REASON_LINKS_MODERATE', '連結過多');
        @define('PLUGIN_EVENT_SPAMBLOCK_HIDE_EMAIL', '隱藏留言者的電子信箱');
        @define('PLUGIN_EVENT_SPAMBLOCK_HIDE_EMAIL_DESC', '不顯示發佈迴響的訪客的電子信箱');
        @define('PLUGIN_EVENT_SPAMBLOCK_HIDE_EMAIL_NOTICE', '電子信箱將不會顯示，僅用於發佈通知');

        @define('PLUGIN_EVENT_SPAMBLOCK_LOGTYPE', '記錄方法');
        @define('PLUGIN_EVENT_SPAMBLOCK_LOGTYPE_DESC', '記錄的資料可以存到資料庫或文字檔');
        @define('PLUGIN_EVENT_SPAMBLOCK_LOGTYPE_FILE', '文字檔 (看下面的記錄檔選項)');
        @define('PLUGIN_EVENT_SPAMBLOCK_LOGTYPE_DB', '資料庫');
        @define('PLUGIN_EVENT_SPAMBLOCK_LOGTYPE_NONE', '不記錄');

        @define('PLUGIN_EVENT_SPAMBLOCK_API_COMMENTS', '如何處理 APIs 的迴響');
        @define('PLUGIN_EVENT_SPAMBLOCK_API_COMMENTS_DESC', '這會管理從 API calls 來的迴響 (引用，WFW:commentAPI 的迴響)。如果選擇 "管理"，全部的迴響都需要通過審核。如果選擇 "拒絕"，全部的迴響都不允許。如果選擇 "沒有"，全部的迴響將會以普通迴響來處理。');
        @define('PLUGIN_EVENT_SPAMBLOCK_API_MODERATE', '管理');
        @define('PLUGIN_EVENT_SPAMBLOCK_API_REJECT', '拒絕');
        @define('PLUGIN_EVENT_SPAMBLOCK_REASON_API', '不允許 API 建立的迴響 (像引用)');

        @define('PLUGIN_EVENT_SPAMBLOCK_FILTER_ACTIVATE', '開啟特殊字');
        @define('PLUGIN_EVENT_SPAMBLOCK_FILTER_ACTIVATE_DESC', '搜尋迴響裡的訊息，如果出現特殊字將標示為垃圾廣告。');

        @define('PLUGIN_EVENT_SPAMBLOCK_FILTER_URLS', '特殊 URLs');
        @define('PLUGIN_EVENT_SPAMBLOCK_FILTER_URLS_DESC', '可用正規運算式，以分號 (;) 來分開字串。');
        @define('PLUGIN_EVENT_SPAMBLOCK_FILTER_AUTHORS', '特殊發佈名稱');
        @define('PLUGIN_EVENT_SPAMBLOCK_FILTER_AUTHORS_DESC', '可用正規運算式，以分號 (;) 來分開字串。');
        
        @define('PLUGIN_EVENT_SPAMBLOCK_REASON_CHECKMAIL', '錯誤的電子郵件');
		@define('PLUGIN_EVENT_SPAMBLOCK_CHECKMAIL', '顯查電子郵件？');
		@define('PLUGIN_EVENT_SPAMBLOCK_REQUIRED_FIELDS', '需要迴響欄位');
		@define('PLUGIN_EVENT_SPAMBLOCK_REQUIRED_FIELDS_DESC', '輸入訪客留言時必填的欄位。請用逗點 "," 來分開每個欄位。可用的自有： name, email, url, replyTo, comment');
		@define('PLUGIN_EVENT_SPAMBLOCK_REASON_REQUIRED_FIELD', '您沒有輸入 %s 欄位！');

		@define('PLUGIN_EVENT_SPAMBLOCK_CONFIG', '設定垃圾阻擋的方法');
		@define('PLUGIN_EVENT_SPAMBLOCK_ADD_AUTHOR', '用 Spamblock 外掛阻擋這位訪客');
		@define('PLUGIN_EVENT_SPAMBLOCK_ADD_URL', '用 Spamblock 外掛阻擋這個網址 URL');
		@define('PLUGIN_EVENT_SPAMBLOCK_REMOVE_AUTHOR', '用 Spamblock 外掛允許這位訪客');
		@define('PLUGIN_EVENT_SPAMBLOCK_REMOVE_URL', '用 Spamblock 外掛允許這個網址 URL');
?>