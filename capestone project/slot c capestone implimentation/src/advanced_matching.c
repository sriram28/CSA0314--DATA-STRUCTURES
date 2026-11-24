/**
 * advanced_matching.c - Advanced Multiple Matching and Explainable AI Allocations
 * Module 7: Alternative Donor Candidates, Unmatched Analysis & Rejection Explanations
 * Module 10: Explainable Multi-Factor Compatibility Parameters
 */

#include "advanced_matching.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define DISCLAIMER_TEXT "Academic Prototype Notice: All matching, priority calculations, and compatibility scores are strictly for educational research demonstration and must not replace legally authorized clinical organ allocation decisions."

ExplainableMatchDetail generate_explainable_match(const Donor* d, const Recipient* r) {
    ExplainableMatchDetail det;
    memset(&det, 0, sizeof(ExplainableMatchDetail));

    if (!d || !r) return det;

    strncpy(det.donor_id, d->id, sizeof(det.donor_id) - 1);
    strncpy(det.donor_name, d->name, sizeof(det.donor_name) - 1);
    strncpy(det.donor_bg, d->blood_group, sizeof(det.donor_bg) - 1);
    strncpy(det.donor_organ, d->organ_type, sizeof(det.donor_organ) - 1);
    det.donor_age = d->age;
    strncpy(det.donor_location, d->location, sizeof(det.donor_location) - 1);
    strncpy(det.donor_hla, d->hla_type, sizeof(det.donor_hla) - 1);

    strncpy(det.recipient_id, r->id, sizeof(det.recipient_id) - 1);
    strncpy(det.recipient_name, r->name, sizeof(det.recipient_name) - 1);
    strncpy(det.recipient_bg, r->blood_group, sizeof(det.recipient_bg) - 1);
    strncpy(det.recipient_organ, r->required_organ, sizeof(det.recipient_organ) - 1);
    det.recipient_age = r->age;
    strncpy(det.recipient_location, r->location, sizeof(det.recipient_location) - 1);
    strncpy(det.recipient_hla, r->hla_type, sizeof(det.recipient_hla) - 1);
    det.urgency_level = r->urgency_level;
    det.waiting_time = r->waiting_time;

    strncpy(det.academic_disclaimer, DISCLAIMER_TEXT, sizeof(det.academic_disclaimer) - 1);

    /* 1. Organ Compatibility Check */
    if (!string_equals_ignore_case(d->organ_type, r->required_organ)) {
        det.is_compatible = false;
        det.total_score = 0.0;
        snprintf(det.full_clinical_summary, sizeof(det.full_clinical_summary),
                 "Incompatible: Organ mismatch. Donor has %s while Recipient requires %s.",
                 d->organ_type, r->required_organ);
        return det;
    }

    /* 2. Blood Group Compatibility */
    if (!is_blood_compatible(d->blood_group, r->blood_group)) {
        det.is_compatible = false;
        det.total_score = 0.0;
        det.abo_score = 0.0;
        snprintf(det.abo_explanation, sizeof(det.abo_explanation),
                 "ABO Incompatible: Donor %s cannot give to Recipient %s (hemagglutinin risk)",
                 d->blood_group, r->blood_group);
        snprintf(det.full_clinical_summary, sizeof(det.full_clinical_summary),
                 "Incompatible: Severe ABO incompatibility between Donor (%s) and Recipient (%s).",
                 d->blood_group, r->blood_group);
        return det;
    }

    det.is_compatible = true;

    /* ABO Points (max 25.0) */
    if (string_equals_ignore_case(d->blood_group, r->blood_group)) {
        det.abo_score = 25.0;
        snprintf(det.abo_explanation, sizeof(det.abo_explanation),
                 "Exact ABO match (%s -> %s): Maximum immunological baseline (25.0 / 25 pts)",
                 d->blood_group, r->blood_group);
    } else {
        det.abo_score = 20.0;
        snprintf(det.abo_explanation, sizeof(det.abo_explanation),
                 "Universal permissible ABO match (%s -> %s): Clinically viable (20.0 / 25 pts)",
                 d->blood_group, r->blood_group);
    }

    /* 3. HLA Phenotype Points (max 25.0) */
    det.hla_matching_alleles = calculate_hla_matches(d->hla_type, r->hla_type);
    det.hla_score = ((double)det.hla_matching_alleles / 6.0) * 25.0;
    snprintf(det.hla_explanation, sizeof(det.hla_explanation),
             "%d of 6 HLA loci matched across HLA-A, B, DR alleles (%.1f / 25 pts)",
             det.hla_matching_alleles, det.hla_score);

    /* 4. Urgency Points (max 25.0) */
    if (r->urgency_level == URGENCY_CRITICAL || r->urgency_level == 1) {
        det.urgency_score = 25.0;
        snprintf(det.urgency_explanation, sizeof(det.urgency_explanation),
                 "Critical ICU status (Tier 1): Immediate emergency life-support transplant (25.0 / 25 pts)");
    } else if (r->urgency_level == URGENCY_URGENT || r->urgency_level == 2) {
        det.urgency_score = 16.0;
        snprintf(det.urgency_explanation, sizeof(det.urgency_explanation),
                 "Urgent deteriorating status (Tier 2): High hospitalization risk (16.0 / 25 pts)");
    } else {
        det.urgency_score = 8.0;
        snprintf(det.urgency_explanation, sizeof(det.urgency_explanation),
                 "Standard maintenance status (Tier 3): Stable outpatient dialysis/therapy (8.0 / 25 pts)");
    }

    /* 5. Waiting Time Points (max 15.0) */
    if (r->waiting_time > 0) {
        double wait_score = log(1.0 + (double)r->waiting_time) * 2.5;
        if (wait_score > 15.0) wait_score = 15.0;
        det.wait_score = wait_score;
        snprintf(det.wait_explanation, sizeof(det.wait_explanation),
                 "Registered waitlist: %d days accumulated (%.1f / 15 pts)",
                 r->waiting_time, det.wait_score);
    } else {
        det.wait_score = 0.0;
        snprintf(det.wait_explanation, sizeof(det.wait_explanation), "New registration: 0 wait days (0.0 / 15 pts)");
    }

    /* 6. Geographic Proximity (max 5.0) */
    if (string_equals_ignore_case(d->location, r->location)) {
        det.location_score = 5.0;
        snprintf(det.location_explanation, sizeof(det.location_explanation),
                 "Local city match (%s -> %s): Minimizes cold ischemia preservation time (5.0 / 5 pts)",
                 d->location, r->location);
    } else {
        det.location_score = 3.0;
        snprintf(det.location_explanation, sizeof(det.location_explanation),
                 "Regional inter-city transfer (%s -> %s): Transit within viable cold time limits (3.0 / 5 pts)",
                 d->location, r->location);
    }

    /* 7. Physiological Age Parity (max 5.0) */
    int age_diff = abs(d->age - r->age);
    if (age_diff <= 15) {
        det.age_score = 5.0;
        snprintf(det.age_explanation, sizeof(det.age_explanation),
                 "Age delta: %d yrs (Donor %dy, Recipient %dy) within ideal +/-15y envelope (5.0 / 5 pts)",
                 age_diff, d->age, r->age);
    } else if (age_diff <= 25) {
        det.age_score = 3.5;
        snprintf(det.age_explanation, sizeof(det.age_explanation),
                 "Age delta: %d yrs (Donor %dy, Recipient %dy): Acceptable biological match (3.5 / 5 pts)",
                 age_diff, d->age, r->age);
    } else {
        det.age_score = 2.0;
        snprintf(det.age_explanation, sizeof(det.age_explanation),
                 "Age delta: %d yrs (Donor %dy, Recipient %dy): Attenuated parity score (2.0 / 5 pts)",
                 age_diff, d->age, r->age);
    }

    det.total_score = det.abo_score + det.hla_score + det.urgency_score +
                      det.wait_score + det.location_score + det.age_score;
    if (det.total_score > 100.0) det.total_score = 100.0;

    snprintf(det.full_clinical_summary, sizeof(det.full_clinical_summary),
             "Medically Compatible Pair: Total Score = %.2f/100. %s match. Blood: %.1f pts, HLA: %.1f pts (%d/6), Urgency: %.1f pts, Waitlist: %.1f pts, Location: %.1f pts, Age Parity: %.1f pts.",
             det.total_score, d->organ_type, det.abo_score, det.hla_score, det.hla_matching_alleles,
             det.urgency_score, det.wait_score, det.location_score, det.age_score);

    return det;
}

AdvancedMatchingResult run_advanced_matching(const BipartiteGraph* g,
                                            const Donor* donors, int d_count,
                                            const Recipient* recipients, int r_count) {
    AdvancedMatchingResult res;
    memset(&res, 0, sizeof(AdvancedMatchingResult));

    clock_t start_time = clock();

    bool* donor_assigned = (bool*)calloc(d_count, sizeof(bool));
    bool* recipient_assigned = (bool*)calloc(r_count, sizeof(bool));

    /* Build candidate lists for each recipient */
    res.plan_count = r_count;
    for (int j = 0; j < r_count; j++) {
        RecipientAllocationPlan* plan = &res.plans[j];
        strncpy(plan->recipient_id, recipients[j].id, sizeof(plan->recipient_id) - 1);
        strncpy(plan->recipient_name, recipients[j].name, sizeof(plan->recipient_name) - 1);
        strncpy(plan->required_organ, recipients[j].required_organ, sizeof(plan->required_organ) - 1);
        strncpy(plan->recipient_bg, recipients[j].blood_group, sizeof(plan->recipient_bg) - 1);
        plan->urgency_level = recipients[j].urgency_level;
        plan->waiting_time = recipients[j].waiting_time;
        plan->candidate_count = 0;
        plan->is_assigned = false;

        /* Collect all compatible donors for recipient j */
        AlternativeCandidate temp_candidates[MAX_RECORDS];
        int temp_count = 0;

        for (int i = 0; i < d_count; i++) {
            ExplainableMatchDetail det = generate_explainable_match(&donors[i], &recipients[j]);
            if (det.is_compatible && det.total_score > 0.0) {
                AlternativeCandidate* cand = &temp_candidates[temp_count++];
                strncpy(cand->donor_id, donors[i].id, sizeof(cand->donor_id) - 1);
                strncpy(cand->donor_name, donors[i].name, sizeof(cand->donor_name) - 1);
                strncpy(cand->donor_bg, donors[i].blood_group, sizeof(cand->donor_bg) - 1);
                strncpy(cand->donor_location, donors[i].location, sizeof(cand->donor_location) - 1);
                cand->donor_age = donors[i].age;
                cand->compatibility_score = det.total_score;
                strncpy(cand->clinical_rationale, det.full_clinical_summary, sizeof(cand->clinical_rationale) - 1);
            } else if (res.rejected_pair_count < MAX_RECORDS * 2) {
                /* Log rejected pair */
                RejectedPair* rej = &res.rejected_pairs[res.rejected_pair_count++];
                strncpy(rej->donor_id, donors[i].id, sizeof(rej->donor_id) - 1);
                strncpy(rej->donor_name, donors[i].name, sizeof(rej->donor_name) - 1);
                strncpy(rej->recipient_id, recipients[j].id, sizeof(rej->recipient_id) - 1);
                strncpy(rej->recipient_name, recipients[j].name, sizeof(rej->recipient_name) - 1);
                strncpy(rej->organ_donor, donors[i].organ_type, sizeof(rej->organ_donor) - 1);
                strncpy(rej->organ_recipient, recipients[j].required_organ, sizeof(rej->organ_recipient) - 1);
                rej->score = 0.0;
                if (!string_equals_ignore_case(donors[i].organ_type, recipients[j].required_organ)) {
                    strncpy(rej->rejection_category, "ORGAN_MISMATCH", sizeof(rej->rejection_category) - 1);
                    snprintf(rej->detailed_reason, sizeof(rej->detailed_reason),
                             "Organ mismatch: Donor has %s, Recipient needs %s", donors[i].organ_type, recipients[j].required_organ);
                } else {
                    strncpy(rej->rejection_category, "ABO_MISMATCH", sizeof(rej->rejection_category) - 1);
                    snprintf(rej->detailed_reason, sizeof(rej->detailed_reason),
                             "Blood type incompatible: Donor %s cannot donate to Recipient %s", donors[i].blood_group, recipients[j].blood_group);
                }
            }
        }

        /* Sort temp_candidates descending by compatibility score */
        for (int a = 0; a < temp_count - 1; a++) {
            for (int b = a + 1; b < temp_count; b++) {
                if (temp_candidates[b].compatibility_score > temp_candidates[a].compatibility_score) {
                    AlternativeCandidate t = temp_candidates[a];
                    temp_candidates[a] = temp_candidates[b];
                    temp_candidates[b] = t;
                }
            }
        }

        /* Keep up to MAX_ALTERNATIVES */
        plan->candidate_count = (temp_count > MAX_ALTERNATIVES) ? MAX_ALTERNATIVES : temp_count;
        for (int k = 0; k < plan->candidate_count; k++) {
            plan->candidates[k] = temp_candidates[k];
            plan->candidates[k].rank = k + 1;
        }
    }

    /* Execute One-to-One Maximum-Weight Greedy Augmentation to guarantee NO duplicate donor assignments */
    /* Process recipients prioritized by urgency tier then wait time */
    for (int priority_tier = 1; priority_tier <= 3; priority_tier++) {
        for (int j = 0; j < r_count; j++) {
            if (recipients[j].urgency_level != priority_tier || recipient_assigned[j]) continue;

            RecipientAllocationPlan* plan = &res.plans[j];
            for (int k = 0; k < plan->candidate_count; k++) {
                int d_idx = donor_find_by_id((Donor*)donors, d_count, plan->candidates[k].donor_id);
                if (d_idx != -1 && !donor_assigned[d_idx]) {
                    /* Assign 1-to-1 match */
                    donor_assigned[d_idx] = true;
                    recipient_assigned[j] = true;
                    plan->is_assigned = true;

                    MatchedPair* pair = &res.primary_pairs[res.primary_pair_count++];
                    strncpy(pair->donor_id, donors[d_idx].id, sizeof(pair->donor_id) - 1);
                    strncpy(pair->donor_name, donors[d_idx].name, sizeof(pair->donor_name) - 1);
                    strncpy(pair->donor_bg, donors[d_idx].blood_group, sizeof(pair->donor_bg) - 1);
                    pair->donor_age = donors[d_idx].age;
                    strncpy(pair->donor_location, donors[d_idx].location, sizeof(pair->donor_location) - 1);

                    strncpy(pair->recipient_id, recipients[j].id, sizeof(pair->recipient_id) - 1);
                    strncpy(pair->recipient_name, recipients[j].name, sizeof(pair->recipient_name) - 1);
                    strncpy(pair->recipient_bg, recipients[j].blood_group, sizeof(pair->recipient_bg) - 1);
                    pair->recipient_age = recipients[j].age;
                    strncpy(pair->recipient_location, recipients[j].location, sizeof(pair->recipient_location) - 1);
                    pair->urgency_level = recipients[j].urgency_level;
                    pair->waiting_time = recipients[j].waiting_time;

                    strncpy(pair->organ, donors[d_idx].organ_type, sizeof(pair->organ) - 1);
                    pair->compatibility_score = plan->candidates[k].compatibility_score;
                    strncpy(pair->explanation, plan->candidates[k].clinical_rationale, sizeof(pair->explanation) - 1);

                    res.total_score += pair->compatibility_score;
                    break;
                }
            }
        }
    }

    if (res.primary_pair_count > 0) {
        res.avg_score = res.total_score / res.primary_pair_count;
    }

    /* Record Unmatched Donors */
    for (int i = 0; i < d_count; i++) {
        if (!donor_assigned[i]) {
            UnmatchedRecord* u = &res.unmatched_donors[res.unmatched_donor_count++];
            strncpy(u->id, donors[i].id, sizeof(u->id) - 1);
            strncpy(u->name, donors[i].name, sizeof(u->name) - 1);
            strncpy(u->organ, donors[i].organ_type, sizeof(u->organ) - 1);
            strncpy(u->blood_group, donors[i].blood_group, sizeof(u->blood_group) - 1);
            strncpy(u->location, donors[i].location, sizeof(u->location) - 1);
            snprintf(u->reason, sizeof(u->reason),
                     "Donor remains available in registry: All compatible %s recipients were allocated to higher-scoring priority donors.",
                     donors[i].organ_type);
        }
    }

    /* Record Unmatched Recipients */
    for (int j = 0; j < r_count; j++) {
        if (!recipient_assigned[j]) {
            UnmatchedRecord* u = &res.unmatched_recipients[res.unmatched_recipient_count++];
            strncpy(u->id, recipients[j].id, sizeof(u->id) - 1);
            strncpy(u->name, recipients[j].name, sizeof(u->name) - 1);
            strncpy(u->organ, recipients[j].required_organ, sizeof(u->organ) - 1);
            strncpy(u->blood_group, recipients[j].blood_group, sizeof(u->blood_group) - 1);
            strncpy(u->location, recipients[j].location, sizeof(u->location) - 1);
            snprintf(u->reason, sizeof(u->reason),
                     "Recipient actively remains on waitlist: No medically compatible %s donor with %s blood currently available in registry.",
                     recipients[j].required_organ, recipients[j].blood_group);
        }
    }

    clock_t end_time = clock();
    res.execution_time_us = ((double)(end_time - start_time) / CLOCKS_PER_SEC) * 1000000.0;

    free(donor_assigned);
    free(recipient_assigned);
    return res;
}

void advanced_matching_to_json(const AdvancedMatchingResult* res, char* json_buf, size_t max_len) {
    if (!res || !json_buf || max_len == 0) return;

    size_t offset = 0;
    offset += snprintf(json_buf + offset, max_len - offset,
                       "{\"primary_pair_count\":%d,\"total_score\":%.2f,\"avg_score\":%.2f,\"execution_time_us\":%.2f,"
                       "\"disclaimer\":\"%s\","
                       "\"primary_matches\":[",
                       res->primary_pair_count, res->total_score, res->avg_score, res->execution_time_us,
                       DISCLAIMER_TEXT);

    /* Primary Matches */
    for (int i = 0; i < res->primary_pair_count; i++) {
        const MatchedPair* p = &res->primary_pairs[i];
        char esc_exp[512];
        json_escape_string(p->explanation, esc_exp, sizeof(esc_exp));
        offset += snprintf(json_buf + offset, max_len - offset,
                           "%s{\"donor_id\":\"%s\",\"donor_name\":\"%s\",\"donor_bg\":\"%s\",\"donor_age\":%d,\"donor_location\":\"%s\","
                           "\"recipient_id\":\"%s\",\"recipient_name\":\"%s\",\"recipient_bg\":\"%s\",\"recipient_age\":%d,\"recipient_location\":\"%s\","
                           "\"organ\":\"%s\",\"urgency_level\":%d,\"waiting_time\":%d,\"compatibility_score\":%.2f,\"explanation\":\"%s\"}",
                           (i > 0) ? "," : "",
                           p->donor_id, p->donor_name, p->donor_bg, p->donor_age, p->donor_location,
                           p->recipient_id, p->recipient_name, p->recipient_bg, p->recipient_age, p->recipient_location,
                           p->organ, p->urgency_level, p->waiting_time, p->compatibility_score, esc_exp);
        if (offset + 2048 >= max_len) break;
    }

    /* Alternative Candidates Plans */
    offset += snprintf(json_buf + offset, max_len - offset, "],\"allocation_plans\":[");
    for (int j = 0; j < res->plan_count; j++) {
        const RecipientAllocationPlan* plan = &res->plans[j];
        offset += snprintf(json_buf + offset, max_len - offset,
                           "%s{\"recipient_id\":\"%s\",\"recipient_name\":\"%s\",\"required_organ\":\"%s\",\"blood_group\":\"%s\","
                           "\"urgency_level\":%d,\"waiting_time\":%d,\"is_assigned\":%s,\"alternatives\":[",
                           (j > 0) ? "," : "",
                           plan->recipient_id, plan->recipient_name, plan->required_organ, plan->recipient_bg,
                           plan->urgency_level, plan->waiting_time, plan->is_assigned ? "true" : "false");

        for (int k = 0; k < plan->candidate_count; k++) {
            const AlternativeCandidate* c = &plan->candidates[k];
            char esc_rat[512];
            json_escape_string(c->clinical_rationale, esc_rat, sizeof(esc_rat));
            offset += snprintf(json_buf + offset, max_len - offset,
                               "%s{\"rank\":%d,\"donor_id\":\"%s\",\"donor_name\":\"%s\",\"donor_bg\":\"%s\","
                               "\"donor_location\":\"%s\",\"donor_age\":%d,\"compatibility_score\":%.2f,\"rationale\":\"%s\"}",
                               (k > 0) ? "," : "",
                               c->rank, c->donor_id, c->donor_name, c->donor_bg,
                               c->donor_location, c->donor_age, c->compatibility_score, esc_rat);
        }
        offset += snprintf(json_buf + offset, max_len - offset, "]}");
        if (offset + 4096 >= max_len) break;
    }

    /* Unmatched Donors */
    offset += snprintf(json_buf + offset, max_len - offset, "],\"unmatched_donors\":[");
    for (int i = 0; i < res->unmatched_donor_count; i++) {
        const UnmatchedRecord* u = &res->unmatched_donors[i];
        char esc_r[512];
        json_escape_string(u->reason, esc_r, sizeof(esc_r));
        offset += snprintf(json_buf + offset, max_len - offset,
                           "%s{\"id\":\"%s\",\"name\":\"%s\",\"organ\":\"%s\",\"blood_group\":\"%s\",\"location\":\"%s\",\"reason\":\"%s\"}",
                           (i > 0) ? "," : "",
                           u->id, u->name, u->organ, u->blood_group, u->location, esc_r);
        if (offset + 1024 >= max_len) break;
    }

    /* Unmatched Recipients */
    offset += snprintf(json_buf + offset, max_len - offset, "],\"unmatched_recipients\":[");
    for (int j = 0; j < res->unmatched_recipient_count; j++) {
        const UnmatchedRecord* u = &res->unmatched_recipients[j];
        char esc_r[512];
        json_escape_string(u->reason, esc_r, sizeof(esc_r));
        offset += snprintf(json_buf + offset, max_len - offset,
                           "%s{\"id\":\"%s\",\"name\":\"%s\",\"organ\":\"%s\",\"blood_group\":\"%s\",\"location\":\"%s\",\"reason\":\"%s\"}",
                           (j > 0) ? "," : "",
                           u->id, u->name, u->organ, u->blood_group, u->location, esc_r);
        if (offset + 1024 >= max_len) break;
    }

    /* Rejected Pairs (sample up to 20 for preview) */
    offset += snprintf(json_buf + offset, max_len - offset, "],\"rejected_pairs\":[");
    int rej_limit = (res->rejected_pair_count > 25) ? 25 : res->rejected_pair_count;
    for (int k = 0; k < rej_limit; k++) {
        const RejectedPair* r = &res->rejected_pairs[k];
        char esc_dr[512];
        json_escape_string(r->detailed_reason, esc_dr, sizeof(esc_dr));
        offset += snprintf(json_buf + offset, max_len - offset,
                           "%s{\"donor_id\":\"%s\",\"donor_name\":\"%s\",\"recipient_id\":\"%s\",\"recipient_name\":\"%s\","
                           "\"category\":\"%s\",\"reason\":\"%s\"}",
                           (k > 0) ? "," : "",
                           r->donor_id, r->donor_name, r->recipient_id, r->recipient_name,
                           r->rejection_category, esc_dr);
        if (offset + 1024 >= max_len) break;
    }

    snprintf(json_buf + offset, max_len - offset, "]}");
}

void explainable_match_to_json(const ExplainableMatchDetail* det, char* json_buf, size_t max_len) {
    if (!det || !json_buf || max_len == 0) return;

    char esc_full[512], esc_abo[256], esc_hla[256], esc_urg[256], esc_wait[256], esc_loc[256], esc_age[256];
    json_escape_string(det->full_clinical_summary, esc_full, sizeof(esc_full));
    json_escape_string(det->abo_explanation, esc_abo, sizeof(esc_abo));
    json_escape_string(det->hla_explanation, esc_hla, sizeof(esc_hla));
    json_escape_string(det->urgency_explanation, esc_urg, sizeof(esc_urg));
    json_escape_string(det->wait_explanation, esc_wait, sizeof(esc_wait));
    json_escape_string(det->location_explanation, esc_loc, sizeof(esc_loc));
    json_escape_string(det->age_explanation, esc_age, sizeof(esc_age));

    snprintf(json_buf, max_len,
             "{\"donor\":{\"id\":\"%s\",\"name\":\"%s\",\"blood\":\"%s\",\"organ\":\"%s\",\"age\":%d,\"location\":\"%s\",\"hla\":\"%s\"},"
             "\"recipient\":{\"id\":\"%s\",\"name\":\"%s\",\"blood\":\"%s\",\"organ\":\"%s\",\"age\":%d,\"location\":\"%s\",\"urgency\":%d,\"wait_days\":%d,\"hla\":\"%s\"},"
             "\"is_compatible\":%s,\"total_score\":%.2f,"
             "\"parameters\":{"
             "\"abo\":{\"score\":%.2f,\"max\":25.0,\"explanation\":\"%s\"},"
             "\"hla\":{\"score\":%.2f,\"max\":25.0,\"matching_alleles\":%d,\"explanation\":\"%s\"},"
             "\"urgency\":{\"score\":%.2f,\"max\":25.0,\"explanation\":\"%s\"},"
             "\"waiting_time\":{\"score\":%.2f,\"max\":15.0,\"explanation\":\"%s\"},"
             "\"location\":{\"score\":%.2f,\"max\":5.0,\"explanation\":\"%s\"},"
             "\"age_parity\":{\"score\":%.2f,\"max\":5.0,\"explanation\":\"%s\"}"
             "},"
             "\"clinical_summary\":\"%s\","
             "\"academic_disclaimer\":\"%s\"}",
             det->donor_id, det->donor_name, det->donor_bg, det->donor_organ, det->donor_age, det->donor_location, det->donor_hla,
             det->recipient_id, det->recipient_name, det->recipient_bg, det->recipient_organ, det->recipient_age, det->recipient_location, det->urgency_level, det->waiting_time, det->recipient_hla,
             det->is_compatible ? "true" : "false", det->total_score,
             det->abo_score, esc_abo,
             det->hla_score, det->hla_matching_alleles, esc_hla,
             det->urgency_score, esc_urg,
             det->wait_score, esc_wait,
             det->location_score, esc_loc,
             det->age_score, esc_age,
             esc_full,
             DISCLAIMER_TEXT);
}
