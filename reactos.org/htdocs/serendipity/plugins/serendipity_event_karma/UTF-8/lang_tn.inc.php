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

        @define('PLUGIN_KARMA_VERSION', '1.3');
        @define('PLUGIN_KARMA_NAME', '評價');
        @define('PLUGIN_KARMA_BLAHBLAH', '允許訪客評價文章');
        @define('PLUGIN_KARMA_VOTETEXT', '評價值: ');
        @define('PLUGIN_KARMA_RATE', '評價值: %s');
        @define('PLUGIN_KARMA_VOTEPOINT_1', '很好!');
        @define('PLUGIN_KARMA_VOTEPOINT_2', '好');
        @define('PLUGIN_KARMA_VOTEPOINT_3', '沒意見');
        @define('PLUGIN_KARMA_VOTEPOINT_4', '沒興趣');
        @define('PLUGIN_KARMA_VOTEPOINT_5', '不好');
        @define('PLUGIN_KARMA_VOTED', '您的評價 "%s" 已送出。');
        @define('PLUGIN_KARMA_INVALID', '您的評價錯誤。');
        @define('PLUGIN_KARMA_ALREADYVOTED', '您已經提供評價。');
        @define('PLUGIN_KARMA_NOCOOKIE', '您的瀏覽器必須支援 cookies 才能進行評價。');
        @define('PLUGIN_KARMA_CLOSED', '請評價 %s 天內的文章！');
        @define('PLUGIN_KARMA_ENTRYTIME', '公開文章後可評價的時間');
        @define('PLUGIN_KARMA_VOTINGTIME', '評價時間');
        @define('PLUGIN_KARMA_ENTRYTIME_BLAHBLAH', '公開文章後多久 (分鐘) 可允許無限制的評價？預設：1440 (一天)');
        @define('PLUGIN_KARMA_VOTINGTIME_BLAHBLAH', '要等多久 (分鐘) 才能進行下一個評價？必須等上面輸入的時間過後才算數。預設：5');
        @define('PLUGIN_KARMA_TIMEOUT', '灌水保護：其他訪客才剛提供評價，請稍待 %s 分鐘再提供您的評價。');
        @define('PLUGIN_KARMA_CURRENT', '評價值： %2$s, %3$s 次評價');
        @define('PLUGIN_KARMA_EXTENDEDONLY', '顯示於文章的副內容');
        @define('PLUGIN_KARMA_EXTENDEDONLY_BLAHBLAH', '只在文章的副內容顯示評價');
        @define('PLUGIN_KARMA_MAXKARMA', '評價天數');
        @define('PLUGIN_KARMA_MAXKARMA_BLAHBLAH', '只允許對少於 X 天數的文章進行評價 (預設：7)');
        @define('PLUGIN_KARMA_LOGGING', '記錄評價？');
        @define('PLUGIN_KARMA_LOGGING_BLAHBLAH', '要記錄評價嗎？');
        @define('PLUGIN_KARMA_ACTIVE', '允許評價？');
        @define('PLUGIN_KARMA_ACTIVE_BLAHBLAH', '要允許文章的評價嗎？');
        @define('PLUGIN_KARMA_VISITS', '記錄訪問次數？');
        @define('PLUGIN_KARMA_VISITS_BLAHBLAH', '要記錄和顯示訪客進入副文章的次數嗎？');
        @define('PLUGIN_KARMA_VISITSCOUNT', ' %4$s 次瀏覽');
        @define('PLUGIN_KARMA_STATISTICS_VISITS_TOP', '最多次瀏覽的文章');
        @define('PLUGIN_KARMA_STATISTICS_VISITS_BOTTOM', '最少次瀏覽的文章');
        @define('PLUGIN_KARMA_STATISTICS_VOTES_TOP', '最多評價的文章');
        @define('PLUGIN_KARMA_STATISTICS_VOTES_BOTTOM', '最少評價的文章');
        @define('PLUGIN_KARMA_STATISTICS_POINTS_TOP', '最多評價值的文章');
        @define('PLUGIN_KARMA_STATISTICS_POINTS_BOTTOM', '最少評價值的文章');
        @define('PLUGIN_KARMA_STATISTICS_VISITS_NO', '瀏覽');
        @define('PLUGIN_KARMA_STATISTICS_VOTES_NO', '評價');
        @define('PLUGIN_KARMA_STATISTICS_POINTS_NO', '評價值');
?>