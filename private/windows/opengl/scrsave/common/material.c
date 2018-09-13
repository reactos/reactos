/******************************Module*Header*******************************\
* Module Name: material.c
*
* Material selection functions.
*
* Copyright (c) 1994 Microsoft Corporation
*
\**************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <time.h>
#include <windows.h>
#include <GL/gl.h>

#include "sscommon.h"

//#define NUM_TEA_MATERIALS 24

#define NUM_GOOD_MATERIALS 16  // 'good' ones among the 24 tea materials

//#define NUM_TEX_MATERIALS 4  // materials for texture

int goodMaterials[NUM_GOOD_MATERIALS] = {
        EMERALD, JADE, PEARL, RUBY, TURQUOISE, BRASS, BRONZE,
        COPPER, GOLD, SILVER, CYAN_PLASTIC, WHITE_PLASTIC, YELLOW_PLASTIC,
        CYAN_RUBBER, GREEN_RUBBER, WHITE_RUBBER };

/*  materials:  emerald, jade, obsidian, pearl, ruby, turquoise
 *              brass, bronze, chrome, copper, gold, silver
 *              black, cyan, green, red, white, yellow plastic
 *              black, cyan, green, red, white, yellow rubber

    description: ambient(RGB), diffuse(RGB), specular(RGB), shininess
 *
 */
// 'tea' materials, from aux teapots program
static GLfloat teaMaterialData[NUM_TEA_MATERIALS][10] = 
{
     0.0215f, 0.1745f, 0.0215f,
        0.07568f, 0.61424f, 0.07568f, 0.633f, 0.727811f, 0.633f, 0.6f,
     0.135f, 0.2225f, 0.1575f,
        0.54f, 0.89f, 0.63f, 0.316228f, 0.316228f, 0.316228f, 0.1f,
     0.05375f, 0.05f, 0.06625f, // XX
        0.18275f, 0.17f, 0.22525f, 0.332741f, 0.328634f, 0.346435f, 0.3f,
     0.25f, 0.20725f, 0.20725f,
        1.0f, 0.829f, 0.829f, 0.296648f, 0.296648f, 0.296648f, 0.088f,
     0.1745f, 0.01175f, 0.01175f,
        0.61424f, 0.04136f, 0.04136f, 0.727811f, 0.626959f, 0.626959f, 0.6f,
     0.1f, 0.18725f, 0.1745f,
        0.396f, 0.74151f, 0.69102f, 0.297254f, 0.30829f, 0.306678f, 0.1f,
     0.329412f, 0.223529f, 0.027451f,
        0.780392f, 0.568627f, 0.113725f, 0.992157f, 0.941176f, 0.807843f,
        0.21794872f,
     0.2125f, 0.1275f, 0.054f,
        0.714f, 0.4284f, 0.18144f, 0.393548f, 0.271906f, 0.166721f, 0.2f,
     0.25f, 0.25f, 0.25f,  // XX
        0.4f, 0.4f, 0.4f, 0.774597f, 0.774597f, 0.774597f, 0.6f,
     0.19125f, 0.0735f, 0.0225f,
        0.7038f, 0.27048f, 0.0828f, 0.256777f, 0.137622f, 0.086014f, 0.1f,
     0.24725f, 0.1995f, 0.0745f,
        0.75164f, 0.60648f, 0.22648f, 0.628281f, 0.555802f, 0.366065f, 0.4f,
     0.19225f, 0.19225f, 0.19225f,
        0.50754f, 0.50754f, 0.50754f, 0.508273f, 0.508273f, 0.508273f, 0.4f,
     0.0f, 0.0f, 0.0f, 0.01f, 0.01f, 0.01f,
        0.50f, 0.50f, 0.50f, .25f,
     0.0f, 0.1f, 0.06f, 0.0f, 0.50980392f, 0.50980392f,
        0.50196078f, 0.50196078f, 0.50196078f, .25f,
     0.0f, 0.0f, 0.0f,
        0.1f, 0.35f, 0.1f, 0.45f, 0.55f, 0.45f, .25f,
     0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, // XX
        0.7f, 0.6f, 0.6f, .25f,
     0.0f, 0.0f, 0.0f, 0.55f, 0.55f, 0.55f,
        0.70f, 0.70f, 0.70f, .25f,
     0.0f, 0.0f, 0.0f, 0.5f, 0.5f, 0.0f,
        0.60f, 0.60f, 0.50f, .25f,
     0.02f, 0.02f, 0.02f, 0.01f, 0.01f, 0.01f, // XX
        0.4f, 0.4f, 0.4f, .078125f,
     0.0f, 0.05f, 0.05f, 0.4f, 0.5f, 0.5f,
        0.04f, 0.7f, 0.7f, .078125f,
     0.0f, 0.05f, 0.0f, 0.4f, 0.5f, 0.4f,
        0.04f, 0.7f, 0.04f, .078125f,
     0.05f, 0.0f, 0.0f, 0.5f, 0.4f, 0.4f,
        0.7f, 0.04f, 0.04f, .078125f,
     0.05f, 0.05f, 0.05f, 0.5f, 0.5f, 0.5f,
        0.7f, 0.7f, 0.7f, .078125f,
     0.05f, 0.05f, 0.0f, 0.5f, 0.5f, 0.4f,
        0.7f, 0.7f, 0.04f, .078125f 
};

// generally white materials for texturing

static GLfloat texMaterialData[NUM_TEX_MATERIALS][10] = {
// bright white
     0.2f, 0.2f, 0.2f,
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.5f,
// less bright white
     0.2f, 0.2f, 0.2f,
        0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.5f,
// warmish white
     0.3f, 0.2f, 0.2f,
        1.0f, 0.9f, 0.8f, 1.0f, 0.9f, 0.8f, 0.5f,
// coolish white
     0.2f, 0.2f, 0.3f,
        0.8f, 0.9f, 1.0f, 0.8f, 0.9f, 1.0f, 0.5f
};

MATERIAL Material[NUM_TEA_MATERIALS + NUM_TEX_MATERIALS];

// pure black material

MATERIAL ss_BlackMat = {0.0f};

/**************************************************************************\
* InitMaterials
*
* Initialize MATERIAL structures with data
* - Set alpha to 0.5 for all materials
*
\**************************************************************************/

static void
InitMaterials( MATERIAL *pm, float *pd, int count )
{
    int i;

    for( i = 0; i < count; i++, pm++ ) {
        pm->ka.r = *pd++;
        pm->ka.g = *pd++;
        pm->ka.b = *pd++;
        pm->ka.a = 0.5f;
        pm->kd.r = *pd++;
        pm->kd.g = *pd++;
        pm->kd.b = *pd++;
        pm->kd.a = 0.5f;
        pm->ks.r = *pd++;
        pm->ks.g = *pd++;
        pm->ks.b = *pd++;
        pm->ks.a = 0.5f;
        pm->specExp = *pd++;
    }
}

/**************************************************************************\
* ss_InitMaterials
*
* Initialize all materials
*
\**************************************************************************/

void ss_InitMaterials()
{
    ss_InitTeaMaterials();
    ss_InitTexMaterials();
}

/**************************************************************************\
* ss_InitTeaMaterials
*
* Initialize Material structure with data in TeaMaterialData
*
\**************************************************************************/

void ss_InitTeaMaterials()
{
    InitMaterials( Material, teaMaterialData[0], NUM_TEA_MATERIALS );
}


/**************************************************************************\
* ss_InitTexMaterials
*
* Initialize Material structure with data in TexMaterialData (tex materials
* follow the tea materials in Material)
*
\**************************************************************************/

void ss_InitTexMaterials()
{
    InitMaterials( Material+NUM_TEA_MATERIALS, texMaterialData[0], 
                   NUM_TEX_MATERIALS );
}

/**************************************************************************\
* ss_SetMaterial
*
* Set specified material with MATERIAL structure
*
\**************************************************************************/

void ss_SetMaterial( MATERIAL *pMat )
{
    glMaterialfv( GL_FRONT, GL_AMBIENT, (GLfloat*) &pMat->ka );
    glMaterialfv( GL_BACK, GL_AMBIENT, (GLfloat*) &pMat->ka );
    glMaterialfv( GL_FRONT, GL_DIFFUSE, (GLfloat *) &pMat->kd );
    glMaterialfv( GL_BACK, GL_DIFFUSE, (GLfloat *) &pMat->kd );
    glMaterialfv( GL_FRONT, GL_SPECULAR, (GLfloat *) &pMat->ks );
    glMaterialfv( GL_BACK, GL_SPECULAR, (GLfloat *) &pMat->ks );
    glMaterialf ( GL_FRONT, GL_SHININESS, pMat->specExp*128.0f);
    glMaterialf ( GL_BACK, GL_SHININESS, pMat->specExp*128.0f);
}

/**************************************************************************\
* ss_SetMaterialIndex
*
* Set material using its index name
*
\**************************************************************************/

void ss_SetMaterialIndex( int index )
{
    if( (index < 0) || (index >= (NUM_TEA_MATERIALS+NUM_TEX_MATERIALS)) )
        return;
    ss_SetMaterial( &Material[index] );
}

/**************************************************************************\
* ss_CreateMaterialGradient
*
* Given 2 materials and a count, create an incremental material, such
* that adding it 'count' times to the first material will generate the
* 2nd material.
*
\**************************************************************************/
void 
ss_CreateMaterialGradient( MATERIAL *matInc, MATERIAL *startMat,
                        MATERIAL *endMat, int transCount ) 
{
    FLOAT fTransCount;

    if( !transCount ) {
        *matInc = *endMat;
        return;
    }

    /*
     * Add fudge factor to avoid round-off error, which could result in
     * invalid material values.
     */
#define FMAT_GRAD_FUDGE 1.0001f
    fTransCount = (FLOAT) transCount * FMAT_GRAD_FUDGE;

    matInc->ka.r = (endMat->ka.r - startMat->ka.r) / fTransCount;
    matInc->ka.g = (endMat->ka.g - startMat->ka.g) / fTransCount;
    matInc->ka.b = (endMat->ka.b - startMat->ka.b) / fTransCount;
    matInc->ks.r = (endMat->ks.r - startMat->ks.r) / fTransCount;
    matInc->ks.g = (endMat->ks.g - startMat->ks.g) / fTransCount;
    matInc->ks.b = (endMat->ks.b - startMat->ks.b) / fTransCount;
    matInc->kd.r = (endMat->kd.r - startMat->kd.r) / fTransCount;
    matInc->kd.g = (endMat->kd.g - startMat->kd.g) / fTransCount;
    matInc->kd.b = (endMat->kd.b - startMat->kd.b) / fTransCount;
    matInc->specExp = (endMat->specExp - startMat->specExp) / fTransCount;
}

/**************************************************************************\
* ss_TransitionMaterial
*
* Given a material, add to it the incremental material, and set the
* result as the current material
*
\**************************************************************************/
void
ss_TransitionMaterial( MATERIAL *transMat, MATERIAL *transMatInc )
{
    transMat->ka.r += transMatInc->ka.r;
    transMat->ka.g += transMatInc->ka.g;
    transMat->ka.b += transMatInc->ka.b;
    transMat->kd.r += transMatInc->kd.r;
    transMat->kd.g += transMatInc->kd.g;
    transMat->kd.b += transMatInc->kd.b;
    transMat->ks.r += transMatInc->ks.r;
    transMat->ks.g += transMatInc->ks.g;
    transMat->ks.b += transMatInc->ks.b;
    transMat->specExp += transMatInc->specExp;

    ss_SetMaterial( transMat );
}

/**************************************************************************\
* ss_RandomTeaMaterial
*
* Select a random TeaMaterial
* Return ptr to material
*
\**************************************************************************/
MATERIAL *ss_RandomTeaMaterial( BOOL bSet )
{
    int index;
    MATERIAL *pMat;

    index =  goodMaterials[ ss_iRand(NUM_GOOD_MATERIALS) ];
    pMat = &Material[ index ];
    if( bSet )
        ss_SetMaterial( pMat );
    return pMat;
}

/**************************************************************************\
* ss_RandomTeaMaterialIndex
*
* Select a random TeaMaterial
* Return material index
*
\**************************************************************************/
int ss_RandomTeaMaterialIndex( BOOL bSet )
{
    int index;

    index =  goodMaterials[ ss_iRand(NUM_GOOD_MATERIALS) ];
    if( bSet )
        ss_SetMaterial( &Material[ index ] );
    return index;
}

/**************************************************************************\
* ss_RandomTexMaterial
*
* Select a random TexMaterial
* Return ptr to material
*
\**************************************************************************/
MATERIAL *ss_RandomTexMaterial( BOOL bSet )
{
    int index;
    MATERIAL *pMat;

    index =  NUM_TEA_MATERIALS + ss_iRand(NUM_TEX_MATERIALS);
    pMat = &Material[ index ];
    if( bSet )
        ss_SetMaterial( pMat );
    return pMat;
}

/**************************************************************************\
* ss_RandomTexMaterialIndex
*
* Select a random TexMaterial
*
\**************************************************************************/
int ss_RandomTexMaterialIndex( BOOL bSet )
{
    int index;

    index =  NUM_TEA_MATERIALS + ss_iRand(NUM_TEX_MATERIALS);
    if( bSet )
        ss_SetMaterial( &Material[ index ] );
    return index;
}
