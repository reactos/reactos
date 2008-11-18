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

@define('PLUGIN_RECENTENTRIES_TITLE', 'Billets récents');
@define('PLUGIN_RECENTENTRIES_BLAHBLAH', 'Affiche les titres et dates des billets les plus récents');
@define('PLUGIN_RECENTENTRIES_NUMBER', 'Nombre de billets');
@define('PLUGIN_RECENTENTRIES_NUMBER_BLAHBLAH', 'Définit combien de billets doivent être affichés. Valeur par défaut: 10');
@define('PLUGIN_RECENTENTRIES_NUMBER_FROM', 'Ignorer les billets sur la page d\'accueil');
@define('PLUGIN_RECENTENTRIES_NUMBER_FROM_DESC', 'Définit si seulement les billets récents qui ne sont pas affichés sur la page principale du blog seront affichés. (Par défaut, les ' . $serendipity['fetchLimit'] . ' derniers billets seront ignorés)');
@define('PLUGIN_RECENTENTRIES_NUMBER_FROM_RADIO_ALL', 'Non, afficher tous les billets');
@define('PLUGIN_RECENTENTRIES_NUMBER_FROM_RADIO_RECENT', 'Oui');

/* vim: set sts=4 ts=4 expandtab : */
?>