/*
 * corrado_profile.h
 *
 * Per-model-year ECU profile for the Corrado EPROM tool. Each year under
 * source/<os>/<year>/ provides a profile.c defining corrado_year_profile(),
 * describing the stock EPROM device and engine management for that year.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef CORRADO_PROFILE_H
#define CORRADO_PROFILE_H

#include "corrado_eprom.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int                  year;        /* model year, e.g. 1992            */
    corrado_ecu_t        ecu;         /* Digifant or Motronic             */
    corrado_eprom_type_t eprom;       /* stock EPROM device for this year */
    const char          *ecu_part;    /* VW ECU part number, if known     */
    const char          *engine;      /* engine code / description        */
    const char          *notes;       /* free-form notes                  */
} corrado_year_profile_t;

/* Defined per-year in profile.c. Returns a static, non-NULL pointer. */
const corrado_year_profile_t *corrado_year_profile(void);

#ifdef __cplusplus
}
#endif

#endif /* CORRADO_PROFILE_H */
