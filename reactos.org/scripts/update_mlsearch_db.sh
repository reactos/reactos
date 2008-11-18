#!/bin/sh
PHP_INCLUDE_PATH=.:/web/reactos.org/config:/usr/share/pear

cd mlsearch
php -d include_path=$PHP_INCLUDE_PATH -f fetchintodb.php > /dev/null 2>&1
