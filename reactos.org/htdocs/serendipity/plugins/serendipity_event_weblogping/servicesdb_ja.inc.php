<?php # $Id: servicesdb_ja.inc.php 364 2005-08-04 10:45:24Z elf2000 $

##########################################################################
# serendipity - another blogger...                                       #
##########################################################################
#                                                                        #
# (c) 2003 Jannis Hermanns <J@hacked.it>                                 #
# http://www.jannis.to/programming/serendipity.html                      #
#                                                                        #
##########################################################################

        $servicesdb = array(
            array(
              'name'       => 'Myblog japan',
              'host'       => 'ping.myblog.jp',
              'path'       => '/',
              'extended' => false
            ),

            array(
              'name'        => 'BLOGGERS.JP',
              'host'        => 'ping.bloggers.jp',
              'path'        => '/rpc/',
              'extended' => false
            ),

            array(
              'name'        => 'blogpeople.net',
              'host'        => 'www.blogpeople.net',
              'path'        => '/servlet/weblogUpdates',
              'extended' => true
            ),

            array(
              'name'        => 'ココログ',
              'host'        => 'ping.cocolog-nifty.com',
              'path'        => '/xmlrpc',
              'extended' => true // false
            ),

            array(
              'name'        => 'goo',
              'host'        => 'blog.goo.ne.jp',
              'path'        => '/XMLRPC',
              'extended' => false
            ),

            array(
              'name'        => 'bulkfeeds',
              'host'        => 'bulkfeeds.net',
              'path'        => '/rpc',
              'extended' => true // false
            ),

            array(
              'name'        => 'blogrolling',
              'host'        => 'rpc.blogrolling.com',
              'path'        => '/pinger/',
              'extended' => false
            ),

            array(
              'name'        => 'dontpushme.com',
              'host'        => 'www.dontpushme.com',
              'path'        => '/ft/XmlRpc/Daily.do',
              'extended' => true // false
            ),

//            array(
//              'name'        => 'Excite エキサイト ： ブログ（blog）',
//              'host'        => 'ping.exblog.jp',
//              'path'        => '/xmlrpc',
//              'extended' => true // false
//            ),

            array(
              'name'        => 'blogdb.jp',
              'host'        => 'blogdb.jp',
              'path'        => '/xmlrpc',
              'extended' => false
            ),

            array(
              'name'        => 'BLOGOOGLE(ブログール)　Pingサーバ',
              'host'        => 'www.blogoole.com',
              'path'        => '/ping/',
              'extended' => false
            ),

            array(
              'name'        => 'にほんブログ村',
              'host'        => 'ping.blogmura.jp',
              'path'        => '/rpc/',
              'extended' => false
            ),

            array(
              'name'        => 'ブログスタイル',
              'host'        => 'blogstyle.jp',
              'path'        => '/xmlrpc/',
              'extended' => false
            ),

            array(
              'name'        => '化け犬.jp',
              'host'        => 'trackback.bakeinu.jp',
              'path'        => '/bakeping.php',
              'extended' => false
            ),

            array(
              'name'        => 'GPost BLOG',
              'host'        => 'ping.gpost.info',
              'path'        => '/xmlrpc',
              'extended' => false
            )

        );
?>
