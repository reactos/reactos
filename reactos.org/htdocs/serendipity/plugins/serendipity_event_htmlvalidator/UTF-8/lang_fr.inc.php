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

@define('PLUGIN_KARMA_VERSION', '1.2');
@define('PLUGIN_KARMA_NAME', 'Karma');
@define('PLUGIN_KARMA_BLAHBLAH', 'Donne à vos visiteurs la possibilité de noter vos billets');
@define('PLUGIN_KARMA_VOTETEXT', 'Karma de cet article: ');
@define('PLUGIN_KARMA_RATE', 'Noter cet article: %s');
@define('PLUGIN_KARMA_VOTEPOINT_1', 'Excellent!');
@define('PLUGIN_KARMA_VOTEPOINT_2', 'Bon');
@define('PLUGIN_KARMA_VOTEPOINT_3', 'Neutre');
@define('PLUGIN_KARMA_VOTEPOINT_4', 'Pas intéressant');
@define('PLUGIN_KARMA_VOTEPOINT_5', 'Mauvais');
@define('PLUGIN_KARMA_VOTED', 'Votre notation "%s" a été enregistrée.');
@define('PLUGIN_KARMA_INVALID', 'Votre notation est invalide.');
@define('PLUGIN_KARMA_ALREADYVOTED', 'Votre notation a déjà été enregistrée.');
@define('PLUGIN_KARMA_NOCOOKIE', 'Votre navigateur doit accepter les cookies pour que vous puissiez voter.');
@define('PLUGIN_KARMA_CLOSED', 'Votez pour les billets écrits il y a moins de %s jours!');
@define('PLUGIN_KARMA_ENTRYTIME', 'Temps de vote après publication');
@define('PLUGIN_KARMA_VOTINGTIME', 'Temps de vote');
@define('PLUGIN_KARMA_ENTRYTIME_BLAHBLAH', 'Pendant combien de temps (en minutes) après que votre billet ait été publié les visiteurs peuvent-ils donner leur vote sans restriction? Valeur par défaut: 1440 (un jour). Quelques valeurs utiles: 2 jours = 2880, 3 jours = 4320, 4 jours = 5760, 5 jours = 7200');
@define('PLUGIN_KARMA_VOTINGTIME_BLAHBLAH', 'Laps de temps (en minutes) nécessaire entre deux votes. Ceci n\'entre en vigueur qu\'après le temps ci-dessus a expiré. Valeur par défaut: 5');
@define('PLUGIN_KARMA_TIMEOUT', 'Protection contre l\'inindation: Un autre visteur vient juste de donner sa note. Merci de patienter %s minutes avant de donner la vôtre.');
@define('PLUGIN_KARMA_CURRENT', 'Karma actuel: %2$s, %3$s note(s)');
@define('PLUGIN_KARMA_EXTENDEDONLY', 'Dans la vue détaillée seulement');
@define('PLUGIN_KARMA_EXTENDEDONLY_BLAHBLAH', 'Afficher la notation Karma seulement dans la vue détaillée d\'un billet.');
@define('PLUGIN_KARMA_MAXKARMA', 'Temps de notation autorisé');
@define('PLUGIN_KARMA_MAXKARMA_BLAHBLAH', 'N\'autoriser la notation que pour une période de X jours. Valeur par défaut: 7');
@define('PLUGIN_KARMA_LOGGING', 'Loguer les notes?');
@define('PLUGIN_KARMA_LOGGING_BLAHBLAH', 'Les notations Karma doivent-elles être loguées?');
@define('PLUGIN_KARMA_ACTIVE', 'Activer la notation Karma?');
@define('PLUGIN_KARMA_ACTIVE_BLAHBLAH', 'Est-ce que la notation Karma doit être activée?');
@define('PLUGIN_KARMA_VISITS', 'Activer le compteur de visites?');
@define('PLUGIN_KARMA_VISITS_BLAHBLAH', 'Chaque visite d\'un billet (vue détaillée) doit-elle être comptée et affichée?');
@define('PLUGIN_KARMA_VISITSCOUNT', ' %4$s visites');
@define('PLUGIN_KARMA_STATISTICS_VISITS_TOP', 'Billets les plus lus');
@define('PLUGIN_KARMA_STATISTICS_VISITS_BOTTOM', 'Billets les moins lus');
@define('PLUGIN_KARMA_STATISTICS_VOTES_TOP', 'Billets les plus chargés en Karma');
@define('PLUGIN_KARMA_STATISTICS_VOTES_BOTTOM', 'Billets les moins chargés en Karma');
@define('PLUGIN_KARMA_STATISTICS_POINTS_TOP', 'Billets les mieux notés');
@define('PLUGIN_KARMA_STATISTICS_POINTS_BOTTOM', 'Billets les moins notés');
@define('PLUGIN_KARMA_STATISTICS_VISITS_NO', 'visites');
@define('PLUGIN_KARMA_STATISTICS_VOTES_NO', 'notes');
@define('PLUGIN_KARMA_STATISTICS_POINTS_NO', 'points');

/* vim: set sts=4 ts=4 expandtab : */
?>