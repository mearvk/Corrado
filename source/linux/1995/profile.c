/*
 * profile.c  --  Corrado model year 1995 ECU profile.
 * Auto-generated scaffold; adjust part numbers against your own ECU label.
 *
 * SPDX-License-Identifier: MIT
 */

#include "corrado_profile.h"

const corrado_year_profile_t *corrado_year_profile(void)
{
    static const corrado_year_profile_t profile = {
        /* .year     */ 1995,
        /* .ecu      */ CORRADO_ECU_MOTRONIC,
        /* .eprom    */ CORRADO_EPROM_27C256,
        /* .ecu_part */ "021906259",
        /* .engine   */ "VR6 2.8L (ABV)",
        /* .notes    */ "VR6 Motronic M2.9; 27C256 class EPROM."
    };
    return &profile;
}
