/**
 * utils.c - Implementation of utility functions, compatibility scoring, and timers
 */

#include "utils.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

/* Trim leading and trailing whitespace in-place */
void trim_whitespace(char* str) {
    if (!str) return;
    char* start = str;
    while (*start && isspace((unsigned char)*start)) {
        start++;
    }
    if (*start == '\0') {
        *str = '\0';
        return;
    }
    char* end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end)) {
        end--;
    }
    *(end + 1) = '\0';
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
}

/* Case-insensitive string comparison */
int string_equals_ignore_case(const char* s1, const char* s2) {
    if (!s1 || !s2) return 0;
    while (*s1 && *s2) {
        if (tolower((unsigned char)*s1) != tolower((unsigned char)*s2)) {
            return 0;
        }
        s1++;
        s2++;
    }
    return *s1 == *s2;
}

/**
 * Blood Compatibility Check (ABO & Rh factor)
 * Returns 1 if donor can give to recipient, 0 otherwise.
 */
int is_blood_compatible(const char* donor_bg, const char* recipient_bg) {
    if (!donor_bg || !recipient_bg) return 0;

    char d[16], r[16];
    strncpy(d, donor_bg, sizeof(d) - 1);
    d[sizeof(d) - 1] = '\0';
    strncpy(r, recipient_bg, sizeof(r) - 1);
    r[sizeof(r) - 1] = '\0';
    trim_whitespace(d);
    trim_whitespace(r);

    /* Exact match is always compatible */
    if (string_equals_ignore_case(d, r)) {
        return 1;
    }

    /* O- is the universal donor for all blood groups */
    if (string_equals_ignore_case(d, "O-")) {
        return 1;
    }

    /* O+ can donate to all Rh+ recipients */
    if (string_equals_ignore_case(d, "O+")) {
        if (string_equals_ignore_case(r, "A+") ||
            string_equals_ignore_case(r, "B+") ||
            string_equals_ignore_case(r, "AB+")) {
            return 1;
        }
    }

    /* A- can donate to A-, A+, AB-, AB+ */
    if (string_equals_ignore_case(d, "A-")) {
        if (string_equals_ignore_case(r, "A+") ||
            string_equals_ignore_case(r, "AB-") ||
            string_equals_ignore_case(r, "AB+")) {
            return 1;
        }
    }

    /* A+ can donate to A+, AB+ */
    if (string_equals_ignore_case(d, "A+")) {
        if (string_equals_ignore_case(r, "AB+")) {
            return 1;
        }
    }

    /* B- can donate to B-, B+, AB-, AB+ */
    if (string_equals_ignore_case(d, "B-")) {
        if (string_equals_ignore_case(r, "B+") ||
            string_equals_ignore_case(r, "AB-") ||
            string_equals_ignore_case(r, "AB+")) {
            return 1;
        }
    }

    /* B+ can donate to B+, AB+ */
    if (string_equals_ignore_case(d, "B+")) {
        if (string_equals_ignore_case(r, "AB+")) {
            return 1;
        }
    }

    /* AB- can donate to AB- and AB+ */
    if (string_equals_ignore_case(d, "AB-")) {
        if (string_equals_ignore_case(r, "AB+")) {
            return 1;
        }
    }

    /* AB+ can only donate to AB+ (already handled by exact match) */
    return 0;
}

/**
 * HLA antigen match counter.
 * Compares antigen tokens (e.g. A2, B44, DR4) between donor and recipient.
 * Returns number of matching antigens (0 to 6).
 */
int calculate_hla_matches(const char* donor_hla, const char* recipient_hla) {
    if (!donor_hla || !recipient_hla || strlen(donor_hla) == 0 || strlen(recipient_hla) == 0) {
        return 2; /* default baseline when unspecified */
    }

    char d_copy[MAX_HLA_LEN];
    char r_copy[MAX_HLA_LEN];
    strncpy(d_copy, donor_hla, sizeof(d_copy) - 1);
    d_copy[sizeof(d_copy) - 1] = '\0';
    strncpy(r_copy, recipient_hla, sizeof(r_copy) - 1);
    r_copy[sizeof(r_copy) - 1] = '\0';

    /* Parse donor tokens */
    char* d_tokens[16];
    int d_count = 0;
    char* token = strtok(d_copy, "-, /");
    while (token && d_count < 16) {
        trim_whitespace(token);
        if (strlen(token) > 0) {
            d_tokens[d_count++] = token;
        }
        token = strtok(NULL, "-, /");
    }

    /* Parse recipient tokens */
    char* r_tokens[16];
    int r_count = 0;
    token = strtok(r_copy, "-, /");
    while (token && r_count < 16) {
        trim_whitespace(token);
        if (strlen(token) > 0) {
            r_tokens[r_count++] = token;
        }
        token = strtok(NULL, "-, /");
    }

    int matches = 0;
    for (int i = 0; i < d_count; i++) {
        for (int j = 0; j < r_count; j++) {
            if (string_equals_ignore_case(d_tokens[i], r_tokens[j])) {
                matches++;
                break;
            }
        }
    }

    if (matches > 6) matches = 6;
    return matches;
}

/**
 * Calculates a comprehensive compatibility score (0.0 - 100.0)
 * Evaluates:
 * 1. Organ Match (Must match, otherwise 0)
 * 2. Blood Group Compatibility (Must be compatible, 20-25 pts)
 * 3. HLA Antigen Matching (0-25 pts)
 * 4. Recipient Urgency Level (8-25 pts)
 * 5. Waiting Time Factor (0-15 pts)
 * 6. Location / Transport Proximity (1-5 pts)
 * 7. Age Proximity (0-5 pts)
 *
 * Populates explanation_buf with clear medical reasoning.
 */
double calculate_compatibility_score(const Donor* d, const Recipient* r, char* explanation_buf, size_t buf_size) {
    if (!d || !r) return 0.0;

    /* Rule 1: Organ Type Compatibility */
    if (!string_equals_ignore_case(d->organ_type, r->required_organ)) {
        if (explanation_buf && buf_size > 0) {
            snprintf(explanation_buf, buf_size,
                     "Incompatible: Organ mismatch (Donor has %s, Recipient needs %s).",
                     d->organ_type, r->required_organ);
        }
        return 0.0;
    }

    /* Rule 2: Blood Group Compatibility */
    if (!is_blood_compatible(d->blood_group, r->blood_group)) {
        if (explanation_buf && buf_size > 0) {
            snprintf(explanation_buf, buf_size,
                     "Incompatible: Blood group mismatch (Donor %s cannot give to Recipient %s due to isohemagglutinin reaction).",
                     d->blood_group, r->blood_group);
        }
        return 0.0;
    }

    double blood_pts = 20.0;
    int is_exact_bg = string_equals_ignore_case(d->blood_group, r->blood_group);
    if (is_exact_bg) {
        blood_pts = 25.0; /* Exact identical blood type receives maximum immunological compatibility */
    }

    /* Rule 3: HLA Antigen Matching (0 to 6 antigens) */
    int hla_matches = calculate_hla_matches(d->hla_type, r->hla_type);
    double hla_pts = ((double)hla_matches / 6.0) * 25.0;

    /* Rule 4: Urgency Priority */
    double urgency_pts = 8.0;
    const char* urgency_desc = "Standard (Maintenance)";
    if (r->urgency_level == URGENCY_CRITICAL) {
        urgency_pts = 25.0;
        urgency_desc = "Critical (Immediate ICU Transplant)";
    } else if (r->urgency_level == URGENCY_URGENT) {
        urgency_pts = 16.0;
        urgency_desc = "Urgent (High Clinical Deterioration)";
    }

    /* Rule 5: Waiting Time Factor (up to 15 pts) */
    double waiting_pts = ((double)r->waiting_time / 365.0) * 10.0;
    if (waiting_pts > 10.0) waiting_pts = 10.0;
    if (r->waiting_time > 180) {
        waiting_pts += 5.0;
    }
    if (waiting_pts > 15.0) waiting_pts = 15.0;

    /* Rule 6: Location & Transport Ischemic Proximity (up to 5 pts) */
    double loc_pts = 1.0;
    int same_location = string_equals_ignore_case(d->location, r->location);
    if (same_location) {
        loc_pts = 5.0; /* Minimal cold ischemic transport time */
    } else {
        loc_pts = 3.0; /* Inter-city transfer within reasonable transport window */
    }

    /* Rule 7: Age Proximity (up to 5 pts) */
    double age_pts = 2.0;
    int age_diff = abs(d->age - r->age);
    if (d->age < 18 && r->age < 18) {
        age_pts = 5.0; /* Pediatric donor prioritized for pediatric recipient */
    } else if (age_diff <= 12) {
        age_pts = 5.0;
    } else if (age_diff <= 25) {
        age_pts = 3.5;
    }

    double total_score = blood_pts + hla_pts + urgency_pts + waiting_pts + loc_pts + age_pts;
    if (total_score > 100.0) total_score = 100.0;

    if (explanation_buf && buf_size > 0) {
        snprintf(explanation_buf, buf_size,
                 "Compatible: %s matched. Blood %s->%s (%s: %.1f pts). HLA: %d/6 match (%.1f pts). Urgency: %s (%.1f pts). Waitlist: %d days (%.1f pts). Location: %s to %s (%.1f pts). Age diff: %d yrs (%.1f pts).",
                 d->organ_type,
                 d->blood_group, r->blood_group,
                 is_exact_bg ? "Exact ABO/Rh" : "Permissible Universal ABO",
                 blood_pts,
                 hla_matches, hla_pts,
                 urgency_desc, urgency_pts,
                 r->waiting_time, waiting_pts,
                 d->location, r->location, loc_pts,
                 age_diff, age_pts);
    }

    return total_score;
}

/**
 * High precision timing in microseconds
 */
double get_time_microseconds(void) {
#ifdef _WIN32
    static LARGE_INTEGER frequency;
    static int initialized = 0;
    if (!initialized) {
        QueryPerformanceFrequency(&frequency);
        initialized = 1;
    }
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return ((double)counter.QuadPart * 1000000.0) / (double)frequency.QuadPart;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec * 1000000.0 + (double)tv.tv_usec;
#endif
}

const char* urgency_to_string(int urgency_level) {
    switch (urgency_level) {
        case URGENCY_CRITICAL: return "Critical";
        case URGENCY_URGENT:   return "Urgent";
        case URGENCY_STANDARD: return "Standard";
        default:               return "Normal";
    }
}

void json_escape_string(const char* src, char* dst, size_t max_dst) {
    if (!src || !dst || max_dst == 0) return;
    size_t out_idx = 0;
    for (size_t i = 0; src[i] != '\0' && out_idx + 2 < max_dst; i++) {
        unsigned char c = (unsigned char)src[i];
        if (c == '"') {
            dst[out_idx++] = '\\';
            dst[out_idx++] = '"';
        } else if (c == '\\') {
            dst[out_idx++] = '\\';
            dst[out_idx++] = '\\';
        } else if (c == '\n') {
            dst[out_idx++] = '\\';
            dst[out_idx++] = 'n';
        } else if (c == '\r') {
            dst[out_idx++] = '\\';
            dst[out_idx++] = 'r';
        } else if (c == '\t') {
            dst[out_idx++] = '\\';
            dst[out_idx++] = 't';
        } else if (c < 32) {
            /* ignore or replace control characters */
            dst[out_idx++] = ' ';
        } else {
            dst[out_idx++] = (char)c;
        }
    }
    dst[out_idx] = '\0';
}
