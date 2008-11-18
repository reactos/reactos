<?php # $Id: serendipity_event_entryproperties.php 341 2005-07-31 21:17:40Z garvinhicking $

        @define('PLUGIN_EVENT_ENTRYPROPERTIES_TITLE', 'Допълнителни свойства за постингите');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_DESC', '(Кеширане, не публични постинги, sticky постинги)');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_STICKYPOSTS', 'Маркиране на този постинг като sticky');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_ACCESS', 'Постингите могат да се четат от');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_ACCESS_PRIVATE', 'Мене');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_ACCESS_MEMBERS', 'Съ-авторите');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_ACCESS_PUBLIC', 'Всички');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE', 'Кеширане на постингите ?');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_DESC', 'Ако е позволено, кеширана версия на постинга ще бъде генерирана при неговия запис. Кеширането ще увеличи производителността, но ще намали гъвкавостта на другите плъгини.');
        @define('PLUGIN_EVENT_ENTRYPROPERTY_BUILDCACHE', 'Кеширане на всички постинги');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_FETCHNEXT', 'Кеширане на следващата серия постинги ...');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_FETCHNO', 'Кеширане на постинги от %d до %d');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_BUILDING', 'Кеширане на постинг #%d, <em>%s</em>...');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHED', 'Постингът е кеширан.');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_DONE', 'Кеширането завърши.');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_ABORTED', 'Кеширането е прекратено.');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_TOTAL', ' (общо %d постинга)...');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_NL2BR', 'Забрана на nl2br');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_NO_FRONTPAGE', 'Постингът да не се вижда на главната страница и в списъците');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_GROUPS', 'Използване на групово-базираните рестрикции');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_GROUPS_DESC', 'Когато е активирано, Вие можете да дефинирате кои потребителски групи да имат възможност за четене на постингите. Тази опция има голямо влияние върху производителността на блога. Позволете я, само ако наистина ще я използвате.');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_USERS', 'Използване на потребител-базирани рестрикции');
        @define('PLUGIN_EVENT_ENTRYPROPERTIES_USERS_DESC', 'Когато е активирано, Вие можете да дефинирате кои специфични потребители да имат възможност за четене на постингите. Тази опция има голямо влияние върху производителността на блога. Позволете я, само ако наистина ще я използвате.');
