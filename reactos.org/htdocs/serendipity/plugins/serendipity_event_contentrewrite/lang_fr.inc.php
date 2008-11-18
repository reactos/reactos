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

@define('PLUGIN_EVENT_CONTENTREWRITE_FROM', 'de');
@define('PLUGIN_EVENT_CONTENTREWRITE_TO', 'vers');
@define('PLUGIN_EVENT_CONTENTREWRITE_NAME', 'Réecriture de contenu');
@define('PLUGIN_EVENT_CONTENTREWRITE_DESCRIPTION', 'Remplace des mots avec un texte défini (pratique par ex. pour les acronymes)');
@define('PLUGIN_EVENT_CONTENTREWRITE_NEWTITLE', 'Nouveau titre');
@define('PLUGIN_EVENT_CONTENTREWRITE_NEWTDESCRIPTION', 'Entrez le titre de l\'acronyme pour la nouvelle entrée ({de})');
@define('PLUGIN_EVENT_CONTENTREWRITE_OLDTITLE', 'Titre #%d');
@define('PLUGIN_EVENT_CONTENTREWRITE_OLDTDESCRIPTION', 'Entrez l\'acronyme ici ({de})');
@define('PLUGIN_EVENT_CONTENTREWRITE_PTITLE', 'Titre du plugin');
@define('PLUGIN_EVENT_CONTENTREWRITE_PDESCRIPTION', 'Le nom de ce plugin');
@define('PLUGIN_EVENT_CONTENTREWRITE_NEWDESCRIPTION', 'Nouvelle description');
@define('PLUGIN_EVENT_CONTENTREWRITE_NEWDDESCRIPTION', 'Entrez la description pour la nouvelle entrée ({vers})');
@define('PLUGIN_EVENT_CONTENTREWRITE_OLDDESCRIPTION', 'Description #%s');
@define('PLUGIN_EVENT_CONTENTREWRITE_OLDDDESCRIPTION', 'Entrez la description ici ({vers})');
@define('PLUGIN_EVENT_CONTENTREWRITE_REWRITESTRING', 'Texte de remplacement');
@define('PLUGIN_EVENT_CONTENTREWRITE_REWRITESTRINGDESC', 'Entrez le texte par lequel vous voulez remplacer le mot que vous avez sélectionné. Vous pouvez utiliser {de} et {vers} où vous le désirez pour ajouter une réecriture.' . "\n" . 'Exemple: <acronym title="{vers}">{de}</acronym>');
@define('PLUGIN_EVENT_CONTENTREWRITE_REWRITECHAR', 'Caractère de réecriture');
@define('PLUGIN_EVENT_CONTENTREWRITE_REWRITECHARDESC', 'Si vous utilisez un caractère spécial pour forcer la réecriture, entrez-le ici. Exemple: si vous désirez seulement remplacer \'mot*\' avec le texte que vous avez défini, mais ne voulez pas que le \'*\' s\'affiche, entrez ce caractère ici.');

/* vim: set sts=4 ts=4 expandtab : */
?>