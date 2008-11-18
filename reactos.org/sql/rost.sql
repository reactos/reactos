-- phpMyAdmin SQL Dump
-- version 2.9.1.1
-- http://www.phpmyadmin.net
-- 
-- Host: localhost
-- Erstellungszeit: 27. Mai 2007 um 13:01
-- Server Version: 5.0.27
-- PHP-Version: 5.2.0
-- 
-- Datenbank: `rost`
-- 

-- --------------------------------------------------------

-- 
-- Tabellenstruktur für Tabelle `apps`
-- 

CREATE TABLE `apps` (
  `app_id` bigint(20) NOT NULL auto_increment,
  `app_name` varchar(100) collate utf8_unicode_ci NOT NULL default '',
  `app_path` varchar(255) collate utf8_unicode_ci NOT NULL default '',
  `app_enabled` char(1) collate utf8_unicode_ci NOT NULL default '',
  PRIMARY KEY  (`app_id`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- --------------------------------------------------------

-- 
-- Tabellenstruktur für Tabelle `languages`
-- 

CREATE TABLE `languages` (
  `lang_id` varchar(5) collate utf8_unicode_ci NOT NULL,
  `lang_code` varchar(2) collate utf8_unicode_ci NOT NULL,
  `lang_name` varchar(100) collate utf8_unicode_ci NOT NULL default '',
  `lang_name_native` varchar(100) collate utf8_unicode_ci NOT NULL default '',
  `lang_active` char(1) collate utf8_unicode_ci NOT NULL default '0',
  `lang_quantifier` varchar(1) collate utf8_unicode_ci NOT NULL default '3',
  PRIMARY KEY  (`lang_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- --------------------------------------------------------

-- 
-- Tabellenstruktur für Tabelle `translations`
-- 

CREATE TABLE `translations` (
  `xml_id` bigint(20) NOT NULL auto_increment,
  `app_id` bigint(20) NOT NULL default '0',
  `xml_lang` varchar(5) collate utf8_unicode_ci NOT NULL default '',
  `xml_rev` int(11) NOT NULL default '0',
  `xml_content` longtext collate utf8_unicode_ci NOT NULL,
  `trans_checked` char(1) collate utf8_unicode_ci NOT NULL default '0',
  `trans_active` char(1) collate utf8_unicode_ci NOT NULL default '0',
  `trans_datetime` datetime NOT NULL,
  PRIMARY KEY  (`xml_id`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;
