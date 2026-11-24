/**
 * matching.c - Implementation of Graph-Based Weighted Bipartite Matching
 */

#include "matching.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int donor_idx;
    int recipient_idx;
    double weight;
    char explanation[MAX_EXPLANATION_LEN];
} CandidateEdge;

/* Comparator for sorting edges in descending order of compatibility score */
static int compare_candidate_edges(const void* a, const void* b) {
    const CandidateEdge* ea = (const CandidateEdge*)a;
    const CandidateEdge* eb = (const CandidateEdge*)b;
    if (eb->weight > ea->weight) return 1;
    if (eb->weight < ea->weight) return -1;
    return 0;
}

MatchingResult run_graph_matching(const BipartiteGraph* g, Donor* donors, int num_donors,
                                  Recipient* recipients, int num_recipients) {
    MatchingResult result;
    memset(&result, 0, sizeof(MatchingResult));

    if (!g || !donors || !recipients || num_donors == 0 || num_recipients == 0) {
        return result;
    }

    double start_time = get_time_microseconds();
    long operations_count = 0;

    /* Count urgency totals among waiting recipients */
    for (int j = 0; j < num_recipients; j++) {
        if (recipients[j].urgency_level == URGENCY_CRITICAL) result.critical_total++;
        else if (recipients[j].urgency_level == URGENCY_URGENT) result.urgent_total++;
        else result.standard_total++;
    }

    /* Step 1: Collect all valid compatibility edges from bipartite graph adjacency list */
    CandidateEdge* edges = (CandidateEdge*)malloc(sizeof(CandidateEdge) * (g->total_edges + 1));
    int edge_count = 0;

    for (int i = 0; i < g->num_donors; i++) {
        AdjEdge* e = g->donor_vertices[i].head;
        while (e) {
            operations_count++;
            edges[edge_count].donor_idx = i;
            edges[edge_count].recipient_idx = e->recipient_idx;
            edges[edge_count].weight = e->compatibility_score;
            strncpy(edges[edge_count].explanation, e->explanation, sizeof(edges[edge_count].explanation) - 1);
            edges[edge_count].explanation[sizeof(edges[edge_count].explanation) - 1] = '\0';
            edge_count++;
            e = e->next;
        }
    }

    /* Step 2: Sort edges in descending order of composite compatibility score */
    if (edge_count > 1) {
        qsort(edges, edge_count, sizeof(CandidateEdge), compare_candidate_edges);
        operations_count += (long)(edge_count * 4); /* QSort comparison steps factor */
    }

    /* Tracking arrays for matching state */
    int* matched_donor = (int*)malloc(sizeof(int) * num_donors);
    int* matched_recipient = (int*)malloc(sizeof(int) * num_recipients);
    for (int i = 0; i < num_donors; i++) matched_donor[i] = -1;
    for (int j = 0; j < num_recipients; j++) matched_recipient[j] = -1;

    /* Step 3: Priority Matching with Augmenting Path Optimization */
    for (int k = 0; k < edge_count; k++) {
        operations_count++;
        int d_idx = edges[k].donor_idx;
        int r_idx = edges[k].recipient_idx;

        /* If both donor and recipient are free, assign immediately */
        if (matched_donor[d_idx] == -1 && matched_recipient[r_idx] == -1) {
            matched_donor[d_idx] = r_idx;
            matched_recipient[r_idx] = d_idx;
        } else if (matched_recipient[r_idx] == -1 && matched_donor[d_idx] != -1) {
            /* Donor is assigned to another recipient prev_r.
               Check if an augmenting path or re-assignment helps save a more critical patient */
            int prev_r = matched_donor[d_idx];
            int curr_urgency = recipients[r_idx].urgency_level;
            int prev_urgency = recipients[prev_r].urgency_level;

            if (curr_urgency < prev_urgency) {
                /* Current recipient is more critical! Check if prev_r can find an alternate donor */
                int found_alt = -1;
                for (int alt_d = 0; alt_d < num_donors; alt_d++) {
                    operations_count++;
                    if (alt_d != d_idx && matched_donor[alt_d] == -1) {
                        /* Check compatibility with prev_r */
                        AdjEdge* chk = g->donor_vertices[alt_d].head;
                        while (chk) {
                            if (chk->recipient_idx == prev_r && chk->compatibility_score > 0.0) {
                                found_alt = alt_d;
                                break;
                            }
                            chk = chk->next;
                        }
                        if (found_alt != -1) break;
                    }
                }

                if (found_alt != -1) {
                    /* Augment: reassign prev_r to found_alt, assign critical r_idx to d_idx */
                    matched_donor[found_alt] = prev_r;
                    matched_recipient[prev_r] = found_alt;
                    matched_donor[d_idx] = r_idx;
                    matched_recipient[r_idx] = d_idx;
                }
            }
        }
    }

    /* Step 4: Assemble Matched Pairs */
    for (int i = 0; i < num_donors; i++) {
        int r_idx = matched_donor[i];
        if (r_idx != -1) {
            MatchedPair* p = &result.pairs[result.pair_count];
            strncpy(p->donor_id, donors[i].id, sizeof(p->donor_id) - 1);
            strncpy(p->donor_name, donors[i].name, sizeof(p->donor_name) - 1);
            strncpy(p->donor_bg, donors[i].blood_group, sizeof(p->donor_bg) - 1);
            p->donor_age = donors[i].age;
            strncpy(p->donor_location, donors[i].location, sizeof(p->donor_location) - 1);

            strncpy(p->recipient_id, recipients[r_idx].id, sizeof(p->recipient_id) - 1);
            strncpy(p->recipient_name, recipients[r_idx].name, sizeof(p->recipient_name) - 1);
            strncpy(p->recipient_bg, recipients[r_idx].blood_group, sizeof(p->recipient_bg) - 1);
            p->recipient_age = recipients[r_idx].age;
            strncpy(p->recipient_location, recipients[r_idx].location, sizeof(p->recipient_location) - 1);
            p->urgency_level = recipients[r_idx].urgency_level;
            p->waiting_time = recipients[r_idx].waiting_time;

            strncpy(p->organ, donors[i].organ_type, sizeof(p->organ) - 1);
            p->compatibility_score = calculate_compatibility_score(&donors[i], &recipients[r_idx],
                                                                  p->explanation, sizeof(p->explanation));

            result.total_score += p->compatibility_score;

            if (p->urgency_level == URGENCY_CRITICAL) result.critical_matched++;
            else if (p->urgency_level == URGENCY_URGENT) result.urgent_matched++;
            else result.standard_matched++;

            result.pair_count++;
            if (result.pair_count >= MAX_RECORDS) break;
        }
    }

    if (result.pair_count > 0) {
        result.avg_score = result.total_score / result.pair_count;
    }

    double end_time = get_time_microseconds();
    result.execution_time_us = end_time - start_time;
    if (result.execution_time_us < 1.0) result.execution_time_us = 1.0; /* Minimum display resolution */
    result.comparisons_count = operations_count;

    free(edges);
    free(matched_donor);
    free(matched_recipient);

    return result;
}

void matching_result_to_json(const MatchingResult* result, const char* algorithm_name,
                            char* json_buf, size_t buf_size) {
    if (!result || !json_buf || buf_size == 0) return;

    size_t offset = 0;
    offset += snprintf(json_buf + offset, buf_size - offset,
                       "{\"algorithm\":\"%s\",\"pair_count\":%d,\"total_score\":%.2f,"
                       "\"avg_score\":%.2f,\"comparisons_count\":%ld,\"execution_time_us\":%.2f,"
                       "\"critical_matched\":%d,\"critical_total\":%d,"
                       "\"urgent_matched\":%d,\"urgent_total\":%d,"
                       "\"standard_matched\":%d,\"standard_total\":%d,\"pairs\":[",
                       algorithm_name ? algorithm_name : "Graph-Based",
                       result->pair_count, result->total_score, result->avg_score,
                       result->comparisons_count, result->execution_time_us,
                       result->critical_matched, result->critical_total,
                       result->urgent_matched, result->urgent_total,
                       result->standard_matched, result->standard_total);

    for (int i = 0; i < result->pair_count; i++) {
        const MatchedPair* p = &result->pairs[i];
        char esc_dname[MAX_NAME_LEN * 2];
        char esc_rname[MAX_NAME_LEN * 2];
        char esc_dloc[MAX_LOCATION_LEN * 2];
        char esc_rloc[MAX_LOCATION_LEN * 2];
        char esc_exp[MAX_EXPLANATION_LEN * 2];

        json_escape_string(p->donor_name, esc_dname, sizeof(esc_dname));
        json_escape_string(p->recipient_name, esc_rname, sizeof(esc_rname));
        json_escape_string(p->donor_location, esc_dloc, sizeof(esc_dloc));
        json_escape_string(p->recipient_location, esc_rloc, sizeof(esc_rloc));
        json_escape_string(p->explanation, esc_exp, sizeof(esc_exp));

        offset += snprintf(json_buf + offset, buf_size - offset,
                           "%s{\"donor_id\":\"%s\",\"donor_name\":\"%s\",\"donor_bg\":\"%s\",\"donor_age\":%d,\"donor_location\":\"%s\","
                           "\"recipient_id\":\"%s\",\"recipient_name\":\"%s\",\"recipient_bg\":\"%s\",\"recipient_age\":%d,\"recipient_location\":\"%s\","
                           "\"organ\":\"%s\",\"urgency_level\":%d,\"urgency_str\":\"%s\",\"waiting_time\":%d,"
                           "\"compatibility_score\":%.2f,\"explanation\":\"%s\"}",
                           (i > 0 ? "," : ""),
                           p->donor_id, esc_dname, p->donor_bg, p->donor_age, esc_dloc,
                           p->recipient_id, esc_rname, p->recipient_bg, p->recipient_age, esc_rloc,
                           p->organ, p->urgency_level, urgency_to_string(p->urgency_level), p->waiting_time,
                           p->compatibility_score, esc_exp);

        if (offset >= buf_size - 1024) break;
    }

    snprintf(json_buf + offset, buf_size - offset, "]}");
}
