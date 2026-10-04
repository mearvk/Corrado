/*
 * profile.c  --  Corrado model year 1991 ECU profile.
 * Auto-generated scaffold; adjust part numbers against your own ECU label.
 *
 * SPDX-License-Identifier: MIT
 */

#include "corrado_profile.h"

const corrado_year_profile_t *corrado_year_profile(void)
{
    static const corrado_year_profile_t profile = {
        /* .year     */ 1991,
        /* .ecu      */ CORRADO_ECU_DIGIFANT,
        /* .eprom    */ CORRADO_EPROM_27C256,
        /* .ecu_part */ "037906022B",
        /* .engine   */ "G60 1.8L supercharged (PG)",
        /* .notes    */ "Early Digifant I; 27C256 socketed on ECU main board."
    };
    return &profile;
}
