/*
 * corrado_metadata.h
 *
 * The Metadata Class metapattern, adapted to C for the Corrado EPROM suite.
 *
 * The team standard is that new software carries a descriptive-metadata block
 * of catalogued attributes, each with a matching get/set accessor pair, kept
 * under a verbatim, greppable header so it stays discoverable. This repository
 * is C (not SLeeLa), so the "class" is rendered as a corrado_metadata_t record
 * plus accessor functions; the eleven baseline attributes and their meanings
 * are preserved exactly.
 *
 * Class tiering (how much of the record to populate):
 *   - worker-class  : single-purpose wrappers, tools, examples -> at least the
 *                     baseline identity (version/id/author/date/origin).
 *   - executive+    : library-family classes, registries, standards/products
 *                     and jurisdictional artifacts -> the full record, incl.
 *                     legality (metaNationalSchedule), fiduciary (copyright/
 *                     trademark/corporation) and probation-of-intent
 *                     (metaOrigin).
 *
 * HARD RULE: values must be faithful to the subject's real origin where
 * publicly known. Unknown values use "none"/"unknown" or 0 -- never a
 * fabricated author, schedule, corporation, or inflated scope.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef CORRADO_METADATA_H
#define CORRADO_METADATA_H

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Descriptive metadata (version 1.11 nested-friendly accessors) ---- */
/*
 * The eleven baseline attributes of the Metadata Class metapattern:
 *
 *   metaVersion          String  "MAJOR.MINOR.PATCH"
 *   metaId               int     registry id
 *   metaAuthor           String
 *   metaDate             String  "YYYY-MM-DD"
 *   metaCopyright        String
 *   metaTrademark        String  (or "none")
 *   metaCorporation      String
 *   metaOrigin           String  jurisdictional scope: a nation / "National"
 *                                 -> "International" -> "Galactic"
 *   metaBirthDate        String  "YYYY-MM-DD"
 *   metaScienceRating    int     1..10 maturity
 *   metaNationalSchedule String  names a REAL, citable standard/spec/treaty/
 *                                 statute, or "none"
 */
typedef struct {
    const char *metaVersion;
    int         metaId;
    const char *metaAuthor;
    const char *metaDate;
    const char *metaCopyright;
    const char *metaTrademark;
    const char *metaCorporation;
    const char *metaOrigin;
    const char *metaBirthDate;
    int         metaScienceRating;
    const char *metaNationalSchedule;
} corrado_metadata_t;

/* Accessor pairs (get/set) for every catalogued attribute. The setters take a
 * non-const record; the getters never return NULL for string fields (they
 * fall back to "none" when a field was left unset). */
const char *corrado_meta_get_version(const corrado_metadata_t *m);
void        corrado_meta_set_version(corrado_metadata_t *m, const char *v);
int         corrado_meta_get_id(const corrado_metadata_t *m);
void        corrado_meta_set_id(corrado_metadata_t *m, int v);
const char *corrado_meta_get_author(const corrado_metadata_t *m);
void        corrado_meta_set_author(corrado_metadata_t *m, const char *v);
const char *corrado_meta_get_date(const corrado_metadata_t *m);
void        corrado_meta_set_date(corrado_metadata_t *m, const char *v);
const char *corrado_meta_get_copyright(const corrado_metadata_t *m);
void        corrado_meta_set_copyright(corrado_metadata_t *m, const char *v);
const char *corrado_meta_get_trademark(const corrado_metadata_t *m);
void        corrado_meta_set_trademark(corrado_metadata_t *m, const char *v);
const char *corrado_meta_get_corporation(const corrado_metadata_t *m);
void        corrado_meta_set_corporation(corrado_metadata_t *m, const char *v);
const char *corrado_meta_get_origin(const corrado_metadata_t *m);
void        corrado_meta_set_origin(corrado_metadata_t *m, const char *v);
const char *corrado_meta_get_birth_date(const corrado_metadata_t *m);
void        corrado_meta_set_birth_date(corrado_metadata_t *m, const char *v);
int         corrado_meta_get_science_rating(const corrado_metadata_t *m);
void        corrado_meta_set_science_rating(corrado_metadata_t *m, int v);
const char *corrado_meta_get_national_schedule(const corrado_metadata_t *m);
void        corrado_meta_set_national_schedule(corrado_metadata_t *m, const char *v);

/* The metadata record describing the Corrado EPROM suite itself (the
 * executive-class artifact: a library family with a real jurisdictional and
 * standards footprint). Print it with corrado_meta_print(). */
const corrado_metadata_t *corrado_suite_metadata(void);

/* Print a metadata record as a labelled block to stdout. */
void corrado_meta_print(const corrado_metadata_t *m);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CORRADO_METADATA_H */
