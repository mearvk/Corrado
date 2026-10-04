/*
 * profile.c  --  Corrado model year 1992 ECU profile.
 * Auto-generated scaffold; adjust part numbers against your own ECU label.
 *
 * SPDX-License-Identifier: MIT
 */

#include "corrado_profile.h"

const corrado_year_profile_t *corrado_year_profile(void)
{
    static const corrado_year_profile_t profile = {
        /* .year     */ 1992,
        /* .ecu      */ CORRADO_ECU_DIGIFANT,
        /* .eprom    */ CORRADO_EPROM_27C256,
        /* .ecu_part */ "037906022B",
        /* .engine   */ "G60 1.8L supercharged (PG) / VR6 2.8L (ABV) introduced",
        /* .notes    */ "Transition year: G60 Digifant and new VR6 Motronic both sold."
    };
    return &profile;
}
