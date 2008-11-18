<?php # $Id: serendipity_plugin_history.php 235 2005-07-08 13:29:39Z garvinhicking $

        @define('PLUGIN_HISTORY_NAME', 'История');
        @define('PLUGIN_HISTORY_DESC', 'Показва отдавнашни постинги, с възможност за задаване на възрастта им');
        @define('PLUGIN_HISTORY_MIN_AGE', 'Минимална възраст');
        @define('PLUGIN_HISTORY_MIN_AGE_DESC', 'Минимална възраст на постингите в дни');
        @define('PLUGIN_HISTORY_MAX_AGE', 'Максимална възраст');
        @define('PLUGIN_HISTORY_MAX_AGE_DESC','Максимална възраст на постингите в дни');
        @define('PLUGIN_HISTORY_MAX_ENTRIES', 'Брой постинги');
        @define('PLUGIN_HISTORY_MAX_ENTRIES_DESC', 'Максимален брой на постингите за показване');
        @define('PLUGIN_HISTORY_SHOWFULL', 'Цели постинги');
        @define('PLUGIN_HISTORY_SHOWFULL_DESC', 'Показване на целите постинги вместо заглавията им като връзки');
        @define('PLUGIN_HISTORY_INTRO', 'Въведение');
        @define('PLUGIN_HISTORY_INTRO_DESC', 'Кратко въведение (нещо като \'Преди една година аз казах... \')');
        @define('PLUGIN_HISTORY_OUTRO', 'Край');
        @define('PLUGIN_HISTORY_OUTRO_DESC', 'Завършващ текст, след постингите');
        @define('PLUGIN_HISTORY_DISPLAYDATE', 'Показване на дата');
        @define('PLUGIN_HISTORY_DISPLAYDATE_DESC', 'Показване на датата за всеки постинг ?');
        @define('PLUGIN_HISTORY_MAXLENGTH', 'Дължина на заглавието');
        @define('PLUGIN_HISTORY_MAXLENGTH_DESC', 'След колко символа да се отреже заглавието (нула за целите заглавия) ?');
        @define('PLUGIN_HISTORY_SPECIALAGE', 'Период');
        @define('PLUGIN_HISTORY_SPECIALAGE_DESC', 'Ако искате да дефинирате период вместо да използвате период по подразбиране, изберете \'По избор\' и настройте двете стойности по-долу');
        @define('PLUGIN_HISTORY_OYA', 'Една година назад');
        @define('PLUGIN_HISTORY_MYSELF', 'По избор');
