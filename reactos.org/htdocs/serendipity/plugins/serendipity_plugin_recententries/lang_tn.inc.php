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

        @define('PLUGIN_RECENTENTRIES_TITLE', '最新文章');
        @define('PLUGIN_RECENTENTRIES_BLAHBLAH', '顯示最新文章的標題和日期');
        @define('PLUGIN_RECENTENTRIES_NUMBER', '文章數量');
        @define('PLUGIN_RECENTENTRIES_NUMBER_BLAHBLAH', '要顯示多少篇文章？(預設：10)');
        @define('PLUGIN_RECENTENTRIES_NUMBER_FROM', '略過首頁的文章');
        @define('PLUGIN_RECENTENTRIES_NUMBER_FROM_DESC', '只有非主頁內的文章才會顯示。(預設：最新 ' . $serendipity['fetchLimit'] . ' 篇文章會略過)');
        @define('PLUGIN_RECENTENTRIES_NUMBER_FROM_RADIO_ALL', '顯示全部');
        @define('PLUGIN_RECENTENTRIES_NUMBER_FROM_RADIO_RECENT', '略過首頁');
?>