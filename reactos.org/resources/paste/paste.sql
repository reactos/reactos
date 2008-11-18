CREATE TABLE `paste_service` (
  `paste_id` bigint(20) NOT NULL auto_increment,
  `paste_date` date NOT NULL default '0000-00-00',
  `paste_days` int(11) NOT NULL default '7',
  `paste_usrid` bigint(20) NOT NULL default '0',
  `paste_nick` varchar(20) collate utf8_unicode_ci NOT NULL default '',
  `paste_desc` varchar(50) collate utf8_unicode_ci NOT NULL default '',
  `paste_lines` int(11) NOT NULL default '0',
  `paste_tabs` int(11) NOT NULL default '0',
  `paste_public` char(1) collate utf8_unicode_ci NOT NULL default '1',
  `paste_lang` varchar(10) collate utf8_unicode_ci NOT NULL default 'C',
  `paste_datetime` datetime NOT NULL default '0000-00-00 00:00:00',
  `paste_size` int(11) NOT NULL default '0',
  `paste_ip` varchar(255) collate utf8_unicode_ci NOT NULL default '',
  `paste_proxy` varchar(255) collate utf8_unicode_ci NOT NULL default '',
  `paste_host` varchar(255) collate utf8_unicode_ci NOT NULL default '',
  PRIMARY KEY  (`paste_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci COMMENT='ReactOS Copy & Paste - paste service';
