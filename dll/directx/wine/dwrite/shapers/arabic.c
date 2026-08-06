/*
 * Copyright HarfBuzz Project authors
 * Copyright 2020 Nikolay Sivov for CodeWeavers
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "dwrite_private.h"
#include "scripts.h"

static const unsigned int arabic_features[] =
{
    DWRITE_MAKE_OPENTYPE_TAG('i','s','o','l'),
    DWRITE_MAKE_OPENTYPE_TAG('f','i','n','a'),
    DWRITE_MAKE_OPENTYPE_TAG('f','i','n','2'),
    DWRITE_MAKE_OPENTYPE_TAG('f','i','n','3'),
    DWRITE_MAKE_OPENTYPE_TAG('m','e','d','i'),
    DWRITE_MAKE_OPENTYPE_TAG('m','e','d','2'),
    DWRITE_MAKE_OPENTYPE_TAG('i','n','i','t'),
};

enum arabic_shaping_action
{
    ISOL,
    FINA,
    FIN2,
    FIN3,
    MEDI,
    MED2,
    INIT,
    NONE,
    NUM_FEATURES = NONE,
};

static BOOL feature_is_syriac(unsigned int tag)
{
    return tag == arabic_features[FIN2] || tag == arabic_features[FIN3] ||
            tag == arabic_features[MED2];
}

static void arabic_collect_features(struct scriptshaping_context *context,
        struct shaping_features *features)
{
    unsigned int i;

    shape_enable_feature(features, DWRITE_MAKE_OPENTYPE_TAG('c','c','m','p'), 0);
    shape_enable_feature(features, DWRITE_MAKE_OPENTYPE_TAG('l','o','c','l'), 0);
    shape_start_next_stage(features, NULL);

    for (i = 0; i < ARRAY_SIZE(arabic_features); ++i)
    {
        unsigned int flags = context->script == Script_Arabic && !feature_is_syriac(arabic_features[i]) ?
                FEATURE_HAS_FALLBACK : 0;
        shape_add_feature_full(features, arabic_features[i], flags, 1);
        shape_start_next_stage(features, NULL);
    }

    shape_enable_feature(features, DWRITE_MAKE_OPENTYPE_TAG('r','l','i','g'), FEATURE_MANUAL_ZWJ | FEATURE_HAS_FALLBACK);

    shape_enable_feature(features, DWRITE_MAKE_OPENTYPE_TAG('r','c','l','t'), FEATURE_MANUAL_ZWJ);
    shape_enable_feature(features, DWRITE_MAKE_OPENTYPE_TAG('c','a','l','t'), FEATURE_MANUAL_ZWJ);
    shape_start_next_stage(features, NULL);

    shape_enable_feature(features, DWRITE_MAKE_OPENTYPE_TAG('m','s','e','t'), 0);
}

enum arabic_joining_type
{
    JOINING_TYPE_U = 0,
    JOINING_TYPE_L = 1,
    JOINING_TYPE_R = 2,
    JOINING_TYPE_D = 3,
    JOINING_TYPE_C = JOINING_TYPE_D,
    JOINING_GROUP_ALAPH = 4,
    JOINING_GROUP_DALATH_RISH = 5,
    JOINING_TYPES = 6,
    JOINING_TYPE_T = 6,
};

static const struct arabic_state_table_entry
{
    unsigned char prev_action;
    unsigned char curr_action;
    unsigned char next_state;
}
arabic_state_table[][JOINING_TYPES] =
{
    /*     U,              L,             R,             D,           ALAPH,      DALATH_RISH  */
    /* State 0: prev was U, not willing to join. */
    { {NONE,NONE,0}, {NONE,ISOL,2}, {NONE,ISOL,1}, {NONE,ISOL,2}, {NONE,ISOL,1}, {NONE,ISOL,6}, },

    /* State 1: prev was R or ISOL/ALAPH, not willing to join. */
    { {NONE,NONE,0}, {NONE,ISOL,2}, {NONE,ISOL,1}, {NONE,ISOL,2}, {NONE,FIN2,5}, {NONE,ISOL,6}, },

    /* State 2: prev was D/L in ISOL form, willing to join. */
    { {NONE,NONE,0}, {NONE,ISOL,2}, {INIT,FINA,1}, {INIT,FINA,3}, {INIT,FINA,4}, {INIT,FINA,6}, },

    /* State 3: prev was D in FINA form, willing to join. */
    { {NONE,NONE,0}, {NONE,ISOL,2}, {MEDI,FINA,1}, {MEDI,FINA,3}, {MEDI,FINA,4}, {MEDI,FINA,6}, },

    /* State 4: prev was FINA ALAPH, not willing to join. */
    { {NONE,NONE,0}, {NONE,ISOL,2}, {MED2,ISOL,1}, {MED2,ISOL,2}, {MED2,FIN2,5}, {MED2,ISOL,6}, },

    /* State 5: prev was FIN2/FIN3 ALAPH, not willing to join. */
    { {NONE,NONE,0}, {NONE,ISOL,2}, {ISOL,ISOL,1}, {ISOL,ISOL,2}, {ISOL,FIN2,5}, {ISOL,ISOL,6}, },

    /* State 6: prev was DALATH/RISH, not willing to join. */
    { {NONE,NONE,0}, {NONE,ISOL,2}, {NONE,ISOL,1}, {NONE,ISOL,2}, {NONE,FIN3,5}, {NONE,ISOL,6}, }
};

extern const unsigned short arabic_shaping_table[];

static unsigned short arabic_get_joining_type(UINT ch)
{
    return get_table_entry_32(arabic_shaping_table, ch);
}

static void arabic_set_shaping_action(struct scriptshaping_context *context,
        unsigned int idx, enum arabic_shaping_action action)
{
    context->glyph_infos[idx].props &= ~(0xf << 16);
    context->glyph_infos[idx].props |= (action & 0xf) << 16;
}

static enum arabic_shaping_action arabic_get_shaping_action(const struct scriptshaping_context *context,
        unsigned int idx)
{
    return (context->glyph_infos[idx].props >> 16) & 0xf;
}

static void arabic_setup_masks(struct scriptshaping_context *context,
        const struct shaping_features *features)
{
    unsigned int i, prev = ~0u, state = 0;
    unsigned int masks[NUM_FEATURES+1];

    for (i = 0; i < context->glyph_count; ++i)
    {
        unsigned short this_type = arabic_get_joining_type(context->glyph_infos[i].codepoint);
        const struct arabic_state_table_entry *entry;

        if (this_type == JOINING_TYPE_T)
        {
            arabic_set_shaping_action(context, i, NONE);
            continue;
        }

        entry = &arabic_state_table[state][this_type];

        if (entry->prev_action != NONE && prev != ~0u)
            arabic_set_shaping_action(context, prev, entry->prev_action);

        arabic_set_shaping_action(context, i, entry->curr_action);

        prev = i;
        state = entry->next_state;
    }

    for (i = 0; i < NUM_FEATURES; ++i)
        masks[i] = shape_get_feature_1_mask(features, arabic_features[i]);
    masks[NONE] = 0;

    /* Unaffected glyphs get action NONE with zero mask. */
    for (i = 0; i < context->glyph_count; ++i)
    {
        enum arabic_shaping_action action = arabic_get_shaping_action(context, i);
        if (action != NONE)
            opentype_layout_unsafe_to_break(context, i, i + 1);
        context->glyph_infos[i].mask |= masks[action];
    }
}

const struct shaper arabic_shaper =
{
    .collect_features = arabic_collect_features,
    .setup_masks = arabic_setup_masks,
};
