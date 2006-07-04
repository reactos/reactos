/*
 * Cards dll definitions
 *
 * Copyright (C) 2004 Sami Nopanen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __CARDS_H__
#define __CARDS_H__

/* Card suits */
#define CARD_SUIT_CLUBS    0
#define CARD_SUIT_DIAMONDS 1
#define CARD_SUIT_HEARTS   2
#define CARD_SUIT_SPADES   3


/* 0-51 = normal 52 cards of deck */
/* 52 = ghost card mask */
/* 53-68 = card backs */
#define CARD_FREE_MASK        52
#define CARD_BACK_CROSSHATCH  53
#define CARD_BACK_WEAVE1      54
#define CARD_BACK_WEAVE2      55
#define CARD_BACK_ROBOT       56
#define CARD_BACK_FLOWERS     57
#define CARD_BACK_VINE1       58
#define CARD_BACK_VINE2       59
#define CARD_BACK_FISH1       60
#define CARD_BACK_FISH2       61
#define CARD_BACK_SHELLS      62
#define CARD_BACK_CASTLE      63
#define CARD_BACK_ISLAND      64
#define CARD_BACK_CARDHAND    65
#define CARD_BACK_UNUSED      66
#define CARD_BACK_THE_X       67
#define CARD_BACK_THE_O       68

#define CARD_MAX              68

/* Drawing modes */
#define MODE_FACEUP             0
#define MODE_FACEDOWN           1
#define MODE_HILITE             2
#define MODE_GHOST              3
#define MODE_REMOVE             4
#define MODE_INVISIBLEGHOST     5
#define MODE_DECKX              6
#define MODE_DECKO              7

#define MODEFLAG_DONT_ROUND_CORNERS 0x80000000

/* As defined by CARD_SUIT_* */
#define SUIT_FROM_CARD(card) (card & 3)
/* 0 = ace, ..., 12 = king */
#define FACE_FROM_CARD(card) (card >> 2)

#endif
