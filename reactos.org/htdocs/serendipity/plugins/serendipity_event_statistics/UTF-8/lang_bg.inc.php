<?php # $Id: serendipity_event_statistics.php 310 2005-07-28 01:10:43Z wesley $

        @define('PLUGIN_EVENT_STATISTICS_NAME', 'Статистика');
        @define('PLUGIN_EVENT_STATISTICS_DESC', 'Добавя връзка към интересни статистически данни в страницата на постингите, включително и брояч на посетителите');
        @define('PLUGIN_EVENT_STATISTICS_OUT_STATISTICS', 'Статистика');
        @define('PLUGIN_EVENT_STATISTICS_OUT_FIRST_ENTRY', 'Първи постинг');
        @define('PLUGIN_EVENT_STATISTICS_OUT_LAST_ENTRY', 'Последен постинг');
        @define('PLUGIN_EVENT_STATISTICS_OUT_TOTAL_ENTRIES', 'Общ брой постинги');
        @define('PLUGIN_EVENT_STATISTICS_OUT_ENTRIES', 'постинг(а)');
        @define('PLUGIN_EVENT_STATISTICS_OUT_TOTAL_PUBLIC', ' ... публикувани');
        @define('PLUGIN_EVENT_STATISTICS_OUT_TOTAL_DRAFTS', ' ... чернови (drafts)');
        @define('PLUGIN_EVENT_STATISTICS_OUT_CATEGORIES', 'Категории');
        @define('PLUGIN_EVENT_STATISTICS_OUT_CATEGORIES2', 'категории');
        @define('PLUGIN_EVENT_STATISTICS_OUT_DISTRIBUTION_CATEGORIES', 'Разпределение на постингите');
        @define('PLUGIN_EVENT_STATISTICS_OUT_DISTRIBUTION_CATEGORIES2', 'постинг(а)');
        @define('PLUGIN_EVENT_STATISTICS_OUT_UPLOADED_IMAGES', 'Заредени (uploaded) картинки');
        @define('PLUGIN_EVENT_STATISTICS_OUT_UPLOADED_IMAGES2', 'картинки');
		@define('PLUGIN_EVENT_STATISTICS_OUT_DISTRIBUTION_IMAGES', 'Разпределение на типовете картинки');
        @define('PLUGIN_EVENT_STATISTICS_OUT_DISTRIBUTION_IMAGES2', 'файла');
        @define('PLUGIN_EVENT_STATISTICS_OUT_COMMENTS', 'Получени коментари');
        @define('PLUGIN_EVENT_STATISTICS_OUT_COMMENTS2', 'коментар(а)');
        @define('PLUGIN_EVENT_STATISTICS_OUT_COMMENTS3', 'Най-коментирани постинги');
        @define('PLUGIN_EVENT_STATISTICS_OUT_TOPCOMMENTS', 'Най-често коментиращи посетители');
        @define('PLUGIN_EVENT_STATISTICS_OUT_LINK', 'връзка');
        @define('PLUGIN_EVENT_STATISTICS_OUT_SUBSCRIBERS', 'Абонати');
        @define('PLUGIN_EVENT_STATISTICS_OUT_SUBSCRIBERS2', 'абонат(а)');
        @define('PLUGIN_EVENT_STATISTICS_OUT_TOPSUBSCRIBERS', 'Постинги с най-много абонати');
        @define('PLUGIN_EVENT_STATISTICS_OUT_TOPSUBSCRIBERS2', 'абонат(а)');
        @define('PLUGIN_EVENT_STATISTICS_OUT_TRACKBACKS', 'Референти');
        @define('PLUGIN_EVENT_STATISTICS_OUT_TRACKBACKS2', 'референт(а)');
        @define('PLUGIN_EVENT_STATISTICS_OUT_TOPTRACKBACK', 'Постинги с най-много референти');
        @define('PLUGIN_EVENT_STATISTICS_OUT_TOPTRACKBACK2', 'референт(а)');
        @define('PLUGIN_EVENT_STATISTICS_OUT_TOPTRACKBACKS3', 'Най-често рефериращи сайтове');
        @define('PLUGIN_EVENT_STATISTICS_OUT_COMMENTS_PER_ARTICLE', 'средно коментари за постинг');
        @define('PLUGIN_EVENT_STATISTICS_OUT_TRACKBACKS_PER_ARTICLE', 'средно референции за постинг');
        @define('PLUGIN_EVENT_STATISTICS_OUT_ARTICLES_PER_DAY', 'средно постинги на ден');
        @define('PLUGIN_EVENT_STATISTICS_OUT_ARTICLES_PER_WEEK', 'средно постинги за седмица');
        @define('PLUGIN_EVENT_STATISTICS_OUT_ARTICLES_PER_MONTH', 'средно постинги за месец');
        @define('PLUGIN_EVENT_STATISTICS_OUT_COMMENTS_PER_ARTICLE2', 'коментара/постинг');
        @define('PLUGIN_EVENT_STATISTICS_OUT_TRACKBACKS_PER_ARTICLE2', 'референции/постинг');
        @define('PLUGIN_EVENT_STATISTICS_OUT_ARTICLES_PER_DAY2', 'постинга/ден');
        @define('PLUGIN_EVENT_STATISTICS_OUT_ARTICLES_PER_WEEK2', 'постинга/седмица');
        @define('PLUGIN_EVENT_STATISTICS_OUT_ARTICLES_PER_MONTH2', 'постинга/месец');
        @define('PLUGIN_EVENT_STATISTICS_OUT_CHARS', 'Общ брой на символите');
        @define('PLUGIN_EVENT_STATISTICS_OUT_CHARS2', 'символа');
        @define('PLUGIN_EVENT_STATISTICS_OUT_CHARS_PER_ARTICLE', 'Средно символи в постинг');
        @define('PLUGIN_EVENT_STATISTICS_OUT_CHARS_PER_ARTICLE2', 'символа/постинг');
        @define('PLUGIN_EVENT_STATISTICS_OUT_LONGEST_ARTICLES', '%s-те най-дълги постинга');
        @define('PLUGIN_EVENT_STATISTICS_MAX_ITEMS', 'Максимален брой данни');
        @define('PLUGIN_EVENT_STATISTICS_MAX_ITEMS_DESC', 'Колко данни да се покажат за дадена статитстическа стойност (по подразбиране: 20)');

//Language constants for the Extended Visitors feature
		@define('PLUGIN_EVENT_STATISTICS_EXT_ADD', 'Допълнителна статистика за посетителите');
		@define('PLUGIN_EVENT_STATISTICS_EXT_ADD_DESC', 'Да се добави ли възможност за показване на допълнителни данни за посетителите (по подразбиране: не)');
		@define('PLUGIN_EVENT_STATISTICS_EXT_OPT1', ' Не');
		@define('PLUGIN_EVENT_STATISTICS_EXT_OPT2', 'Да, долу на страницата');
		@define('PLUGIN_EVENT_STATISTICS_EXT_OPT3', 'Да, горе на страницата');
		@define('PLUGIN_EVENT_STATISTICS_EXT_VISITORS', 'Брой на посетителите');
		@define('PLUGIN_EVENT_STATISTICS_EXT_VISTODAY', 'Брой на посетителите днес');
		@define('PLUGIN_EVENT_STATISTICS_EXT_VISTOTAL', 'Общ брой на посетителите');
		@define('PLUGIN_EVENT_STATISTICS_EXT_VISSINCE', 'Допълнителни статистически данни за посетителите са събрани от');
		@define('PLUGIN_EVENT_STATISTICS_EXT_VISLATEST', 'Последни посетители');
		@define('PLUGIN_EVENT_STATISTICS_EXT_TOPREFS', 'Най-често рефериращи сайтове');
		@define('PLUGIN_EVENT_STATISTICS_EXT_TOPREFS_NONE', 'Референти не са регистирани за сега.');
		@define('PLUGIN_EVENT_STATISTICS_OUT_EXT_STATISTICS', 'Допълнителна статистика за посетителите');
		@define('PLUGIN_EVENT_STATISTICS_BANNED_HOSTS', 'Хостове, които не трябва да бъдат броени');
		@define('PLUGIN_EVENT_STATISTICS_BANNED_HOSTS_DESC', 'Въведете хостовете, които трябва да бъдат изключени от преброяване, разделени с "|"');






