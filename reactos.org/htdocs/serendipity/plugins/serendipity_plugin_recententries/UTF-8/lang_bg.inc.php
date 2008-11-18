<?php # $Id: serendipity_plugin_shoutbox.php 235 2005-07-08 13:29:39Z garvinhicking $

        @define('PLUGIN_RECENTENTRIES_TITLE', 'Последни постинги');
        @define('PLUGIN_RECENTENTRIES_BLAHBLAH', 'Показва заглавията и датите на последните постинги');
        @define('PLUGIN_RECENTENTRIES_NUMBER', 'Брой на постингите');
        @define('PLUGIN_RECENTENTRIES_NUMBER_BLAHBLAH', 'Брой на постингите, които да бъдат показани (по подразбиране: 10)');
        @define('PLUGIN_RECENTENTRIES_NUMBER_FROM', 'Прескачане на показаните на заглавната страница постинги');
        @define('PLUGIN_RECENTENTRIES_NUMBER_FROM_DESC', 'Само последните постинги, които не са показани на заглавната страница ще бъдат показани. (По подразбиране: последните ' . $serendipity['fetchLimit'] . ' ще бъдат прескочени)');
        @define('PLUGIN_RECENTENTRIES_NUMBER_FROM_RADIO_ALL', 'Покажи всички');
        @define('PLUGIN_RECENTENTRIES_NUMBER_FROM_RADIO_RECENT', 'Прескочи показаните на заглавната страница');
