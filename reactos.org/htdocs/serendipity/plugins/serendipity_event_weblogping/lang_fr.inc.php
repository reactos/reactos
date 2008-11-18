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

@define('PLUGIN_EVENT_WEBLOGPING_PING', 'Annoncer les billets à: (par ping XML)');
@define('PLUGIN_EVENT_WEBLOGPING_SENDINGPING', 'Envoie le ping XML-RPC à l\'hôte %s');
@define('PLUGIN_EVENT_WEBLOGPING_TITLE', 'Annconcer les billets');
@define('PLUGIN_EVENT_WEBLOGPING_DESC', 'Envoyer une mise à jour pour les nouveaux billets aux services d\'indexation');
@define('PLUGIN_EVENT_WEBLOGPING_SUPERSEDES', '(remplace %s)');
@define('PLUGIN_EVENT_WEBLOGPING_CUSTOM', 'Services ping additionnels');
@define('PLUGIN_EVENT_WEBLOGPING_CUSTOM_BLAHBLA', 'Vous permet d\'ajouter des services additionnels; entrez les adresses cibles ici, séparez plusieurs adresses par des virgules (,). Les adresses doivent être au format "hôte.domaine/chemin". Si un "*" est ajouté au début de l\'hôte, les options XML-RPC étendues seront envoyées à l\hôte cible (seulement si celui-ci les gère).');

/* vim: set sts=4 ts=4 expandtab : */
?>