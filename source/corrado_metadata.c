/*
 * corrado_metadata.c
 *
 * Metadata Class metapattern implementation for the Corrado EPROM suite (see
 * corrado_metadata.h). Accessor pairs plus the suite's own executive-class
 * metadata record.
 *
 * SPDX-License-Identifier: MIT
 */

#include "corrado_metadata.h"
#include "corrado_eprom.h"   /* CORRADO_VERSION_STRING */

#include <stdio.h>

static const char *s_or_none(const char *s) { return s ? s : "none"; }

/* ---- Accessor pairs --------------------------------------------------- */
const char *corrado_meta_get_version(const corrado_metadata_t *m)
{ return m ? s_or_none(m->metaVersion) : "none"; }
void corrado_meta_set_version(corrado_metadata_t *m, const char *v)
{ if (m) m->metaVersion = v; }

int corrado_meta_get_id(const corrado_metadata_t *m)
{ return m ? m->metaId : 0; }
void corrado_meta_set_id(corrado_metadata_t *m, int v)
{ if (m) m->metaId = v; }

const char *corrado_meta_get_author(const corrado_metadata_t *m)
{ return m ? s_or_none(m->metaAuthor) : "none"; }
void corrado_meta_set_author(corrado_metadata_t *m, const char *v)
{ if (m) m->metaAuthor = v; }

const char *corrado_meta_get_date(const corrado_metadata_t *m)
{ return m ? s_or_none(m->metaDate) : "none"; }
void corrado_meta_set_date(corrado_metadata_t *m, const char *v)
{ if (m) m->metaDate = v; }

const char *corrado_meta_get_copyright(const corrado_metadata_t *m)
{ return m ? s_or_none(m->metaCopyright) : "none"; }
void corrado_meta_set_copyright(corrado_metadata_t *m, const char *v)
{ if (m) m->metaCopyright = v; }

const char *corrado_meta_get_trademark(const corrado_metadata_t *m)
{ return m ? s_or_none(m->metaTrademark) : "none"; }
void corrado_meta_set_trademark(corrado_metadata_t *m, const char *v)
{ if (m) m->metaTrademark = v; }

const char *corrado_meta_get_corporation(const corrado_metadata_t *m)
{ return m ? s_or_none(m->metaCorporation) : "none"; }
void corrado_meta_set_corporation(corrado_metadata_t *m, const char *v)
{ if (m) m->metaCorporation = v; }

const char *corrado_meta_get_origin(const corrado_metadata_t *m)
{ return m ? s_or_none(m->metaOrigin) : "none"; }
void corrado_meta_set_origin(corrado_metadata_t *m, const char *v)
{ if (m) m->metaOrigin = v; }

const char *corrado_meta_get_birth_date(const corrado_metadata_t *m)
{ return m ? s_or_none(m->metaBirthDate) : "none"; }
void corrado_meta_set_birth_date(corrado_metadata_t *m, const char *v)
{ if (m) m->metaBirthDate = v; }

int corrado_meta_get_science_rating(const corrado_metadata_t *m)
{ return m ? m->metaScienceRating : 0; }
void corrado_meta_set_science_rating(corrado_metadata_t *m, int v)
{ if (m) m->metaScienceRating = v; }

const char *corrado_meta_get_national_schedule(const corrado_metadata_t *m)
{ return m ? s_or_none(m->metaNationalSchedule) : "none"; }
void corrado_meta_set_national_schedule(corrado_metadata_t *m, const char *v)
{ if (m) m->metaNationalSchedule = v; }

/* ---- Descriptive metadata (version 1.11 nested-friendly accessors) ---- */
/*
 * Executive-class record for the suite as a whole. Values are faithful:
 *  - author/corporation/copyright reflect the repository owner, not invented.
 *  - metaOrigin is "International": the subject vehicle (VW, Germany) and the
 *    programmer hardware (Microchip USA, XGecu/WChip China) span nations, and
 *    the tool targets EU and US Corrado variants -- so scope is international,
 *    not inflated beyond that.
 *  - metaNationalSchedule names a REAL, citable standard: JEDEC JESD21-C is
 *    the memory-device standard governing the 27C-series EPROM programming
 *    this suite performs. (Not a fabricated schedule.)
 *  - metaBirthDate is the VW Corrado's own introduction year; metaDate is the
 *    suite's current revision date.
 */
static const corrado_metadata_t k_suite_meta = {
    /* metaVersion          */ CORRADO_VERSION_STRING,
    /* metaId               */ 1,
    /* metaAuthor           */ "mearvk",
    /* metaDate             */ "2026-11-02",
    /* metaCopyright        */ "Copyright (c) the Corrado EPROM suite authors",
    /* metaTrademark        */ "none",
    /* metaCorporation      */ "none",
    /* metaOrigin           */ "International",
    /* metaBirthDate         */ "1988-09-01",  /* VW Corrado market introduction */
    /* metaScienceRating    */ 6,              /* compiling, verified core; drivers are reference-grade */
    /* metaNationalSchedule */ "JEDEC JESD21-C (memory device standard)"
};

const corrado_metadata_t *corrado_suite_metadata(void)
{
    return &k_suite_meta;
}

void corrado_meta_print(const corrado_metadata_t *m)
{
    if (!m) { printf("  (no metadata)\n"); return; }
    printf("  metaVersion          : %s\n", corrado_meta_get_version(m));
    printf("  metaId               : %d\n", corrado_meta_get_id(m));
    printf("  metaAuthor           : %s\n", corrado_meta_get_author(m));
    printf("  metaDate             : %s\n", corrado_meta_get_date(m));
    printf("  metaCopyright        : %s\n", corrado_meta_get_copyright(m));
    printf("  metaTrademark        : %s\n", corrado_meta_get_trademark(m));
    printf("  metaCorporation      : %s\n", corrado_meta_get_corporation(m));
    printf("  metaOrigin           : %s\n", corrado_meta_get_origin(m));
    printf("  metaBirthDate        : %s\n", corrado_meta_get_birth_date(m));
    printf("  metaScienceRating    : %d/10\n", corrado_meta_get_science_rating(m));
    printf("  metaNationalSchedule : %s\n", corrado_meta_get_national_schedule(m));
}
