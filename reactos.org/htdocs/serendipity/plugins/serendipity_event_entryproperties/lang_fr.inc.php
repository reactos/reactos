<?php # $Id: lang_ja.inc.php,v 1.4 2005/05/17 11:37:42 garvinhicking Exp $

##########################################################################
# serendipity - another blogger...                                       #
##########################################################################
#                                                                        #
# (c) 2003 Jannis Hermanns <J@hacked.it>                                 #
# http://www.jannis.to/programming/serendipity.html                      #
#                                                                        #
# Translated by                                                          #
# Sebastian Mordziol <argh@php-tools.net>                                #
# http://sebastian.mordziol.de                                           #
#                                                                        #
##########################################################################

@define('PLUGIN_EVENT_ENTRYPROPERTIES_TITLE', 'Propriétés étendues pour les billets');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_DESC', '(cache, billets non publics, billets collants)');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_STICKYPOSTS', 'Render ce billet collant');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_ACCESS', 'Billets peuvent être lus par');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_ACCESS_PRIVATE', 'Moi-même');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_ACCESS_MEMBER', 'Co-auteurs');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_ACCESS_PUBLIC', 'Tout le monde');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE', 'Activer le cache des billets?');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_DESC', 'Si activé, une version cachée sera crée à chaque enregistrement du billet. Le cache augmente la performance, mais peut aussi diminuer la flexibilité pour les autres plugins actifs.');
@define('PLUGIN_EVENT_ENTRYPROPERTY_BUILDCACHE', 'Créer le cache pour les billets');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_FETCHNEXT', 'Chargement du prochain jeu de billets...');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_FETCHNO', 'Chargement des billets %d à %d');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_BUILDING', 'Création du cache pour le billet #%d, <em>%s</em>...');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHED', 'Billet caché.');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_DONE', 'Création du cache terminé.');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_ABORTED', 'Création du cache ANNULÉ.');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_TOTAL', ' (%d billets au total)...');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_NL2BR', 'Désactiver nl2br');

/* vim: set sts=4 ts=4 expandtab : */
?>