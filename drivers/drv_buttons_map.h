#include "brick_config.h"
/**
 * @file drv_buttons_map.h
 * @brief Définition du mappage logique des boutons de la surface de contrôle Brick.
 * @details
 * Ce fichier fournit la correspondance entre les identifiants physiques des boutons
 * et leurs rôles logiques dans l’interface utilisateur (transport, pages, paramètres, séquenceur, etc.).
 *
 * Il permet au code applicatif d’utiliser des noms symboliques (`BTN_PLAY`, `BTN_PAGE1`, etc.)
 * plutôt que des indices matériels.
 *
 * @note Les indices ici correspondent à ceux définis dans le driver bas niveau `drv_buttons.c`.
 * @ingroup drivers
 */

#ifndef DRV_BUTTONS_MAP_H
#define DRV_BUTTONS_MAP_H

#include "drv_buttons.h"  /**< Nécessaire pour accéder à `drv_button_is_pressed()` */

/* ====================================================================== */
/*                        BOUTONS DE CONTRÔLE GÉNÉRAUX                    */
/* ====================================================================== */

/** @brief Bouton d’accès au menu des réglages. */
#define BTN_REGLAGE   37
/** @brief Bouton de fonction “Shift” (modificateur). */
#define BTN_SHIFT     36
/** @brief Bouton “+” (incrément ou navigation). */
#define BTN_PLUS      39
/** @brief Bouton “−” (décrément ou navigation inverse). */
#define BTN_MINUS     38
/** @brief Bouton “Record”. */
#define BTN_REC       11
/** @brief Bouton “Play / Start”. */
#define BTN_PLAY      9
/** @brief Bouton “Stop”. */
#define BTN_STOP      10

/* ====================================================================== */
/*                             BOUTONS DE PAGE                            */
/* ====================================================================== */

/** @brief Accès rapide à la page 1. */ #define BTN_PAGE1  35
/** @brief Accès rapide à la page 2. */ #define BTN_PAGE2  34
/** @brief Accès rapide à la page 3. */ #define BTN_PAGE3  33
/** @brief Accès rapide à la page 4. */ #define BTN_PAGE4  32
/** @brief Accès rapide à la page 5. */ #define BTN_PAGE5  12

/* ====================================================================== */
/*                         BOUTONS DE PARAMÈTRES                          */
/* ====================================================================== */

/** @brief Paramètre n°1 */ #define BTN_PARAM1 5
/** @brief Paramètre n°2 */ #define BTN_PARAM2 4
/** @brief Paramètre n°3 */ #define BTN_PARAM3 3
/** @brief Paramètre n°4 */ #define BTN_PARAM4 2
/** @brief Paramètre n°5 */ #define BTN_PARAM5 6
/** @brief Paramètre n°6 */ #define BTN_PARAM6 7
/** @brief Paramètre n°7 */ #define BTN_PARAM7 0
/** @brief Paramètre n°8 */ #define BTN_PARAM8 1

/* ====================================================================== */
/*                       BOUTONS DU SÉQUENCEUR (TRIGS)                    */
/* ====================================================================== */

#define BTN_SEQ1      29
#define BTN_SEQ2      28
#define BTN_SEQ3      27
#define BTN_SEQ4      26
#define BTN_SEQ5      21
#define BTN_SEQ6      20
#define BTN_SEQ7      19
#define BTN_SEQ8      18
#define BTN_SEQ9      30
#define BTN_SEQ10     31
#define BTN_SEQ11     24
#define BTN_SEQ12     25
#define BTN_SEQ13     22
#define BTN_SEQ14     23
#define BTN_SEQ15     16
#define BTN_SEQ16     17

/* ====================================================================== */
/*                            HELPER INLINE                               */
/* ====================================================================== */

/**
 * @brief Vérifie si un bouton est actuellement pressé.
 * @param id Identifiant logique du bouton (ex. `BTN_PLAY`, `BTN_PAGE1`, etc.)
 * @return `true` si le bouton est pressé, sinon `false`.
 */
static inline bool drv_btn_pressed(int id) {
    return drv_button_is_pressed(id);
}

#endif /* DRV_BUTTONS_MAP_H */
