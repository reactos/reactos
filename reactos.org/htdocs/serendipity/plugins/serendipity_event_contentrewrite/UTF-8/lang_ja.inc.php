<?php # $Id: lang_ja.inc.php 7 2005-04-16 06:39:31Z s_bergmann $

##########################################################################
# serendipity - another blogger...                                       #
##########################################################################
#                                                                        #
# (c) 2003 Jannis Hermanns <J@hacked.it>                                 #
# http://www.jannis.to/programming/serendipity.html                      #
#                                                                        #
# Translated by                                                          #
# (c) 2004-2005 Tadashi Jokagi <elf2000@users.sourceforge.net>           #
#                                                                        #
##########################################################################

        @define('PLUGIN_EVENT_CONTENTREWRITE_FROM', 'from');
        @define('PLUGIN_EVENT_CONTENTREWRITE_TO', 'to');
        @define('PLUGIN_EVENT_CONTENTREWRITE_NAME', 'コンテントリライター');
        @define('PLUGIN_EVENT_CONTENTREWRITE_DESCRIPTION', '単語を新しく選択された文字列に置換します (useful for acronyms)');
        @define('PLUGIN_EVENT_CONTENTREWRITE_NEWTITLE', '新規題名');
        @define('PLUGIN_EVENT_CONTENTREWRITE_NEWTDESCRIPTION', 'ここに新規項目の acronyms タイトルを入力します ({from})');
        @define('PLUGIN_EVENT_CONTENTREWRITE_OLDTITLE', 'Title #%d');
        @define('PLUGIN_EVENT_CONTENTREWRITE_OLDTDESCRIPTION', 'ここに acronyms を入力します ({from})');
        @define('PLUGIN_EVENT_CONTENTREWRITE_PTITLE', 'プラグインの題名');
        @define('PLUGIN_EVENT_CONTENTREWRITE_PDESCRIPTION', 'このプラグインの名前');
        @define('PLUGIN_EVENT_CONTENTREWRITE_NEWDESCRIPTION', '新規詳細');
        @define('PLUGIN_EVENT_CONTENTREWRITE_NEWDDESCRIPTION', 'ここに新規項目の詳細を入力します ({to})');
        @define('PLUGIN_EVENT_CONTENTREWRITE_OLDDESCRIPTION', 'Description #%s');
        @define('PLUGIN_EVENT_CONTENTREWRITE_OLDDDESCRIPTION', 'ここに詳細を入力します ({to})');
        @define('PLUGIN_EVENT_CONTENTREWRITE_REWRITESTRING', 'リライト文字列');
        @define('PLUGIN_EVENT_CONTENTREWRITE_REWRITESTRINGDESC', 'リライトに使用する文字列です。Place {from} and {to} anywhere you like to get a rewrite.' . "\n" . 'Example: <acronym title="{to}">{from}</acronym>');
        @define('PLUGIN_EVENT_CONTENTREWRITE_REWRITECHAR', 'リライト文字');
        @define('PLUGIN_EVENT_CONTENTREWRITE_REWRITECHARDESC', 'If there is any char you append to force rewriting, enter it here. If you want to only replace \'serendipity*\' with what you entered for that word but want the \'*\' removed, enter that char here.');

?>