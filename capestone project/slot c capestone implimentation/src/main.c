/**
 * main.c - Main Entry Point for Smart Organ Donation System
 * Provides CLI interactive menu mode and JSON API mode for web frontend integration.
 */

#include "../include/common.h"
#include "donor.h"
#include "recipient.h"
#include "graph.h"
#include "matching.h"
#include "traditional.h"
#include "csv_io.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DONORS_CSV_PATH "data/donors.csv"
#define RECIPIENTS_CSV_PATH "data/recipients.csv"
#define DONORS_SAMPLE_PATH "data/donors_sample.csv"
#define RECIPIENTS_SAMPLE_PATH "data/recipients_sample.csv"

#define LARGE_JSON_BUF (1024 * 512) /* 512 KB buffer for JSON output */

/* Simple JSON field parser helper */
static void extract_json_field(const char* json, const char* key, char* out_val, size_t max_len) {
    out_val[0] = '\0';
    char search_pattern[64];
    snprintf(search_pattern, sizeof(search_pattern), "\"%s\":", key);
    const char* pos = strstr(json, search_pattern);
    if (!pos) return;
    pos += strlen(search_pattern);
    while (*pos && (*pos == ' ' || *pos == '\t')) pos++;

    if (*pos == '"') {
        pos++;
        size_t idx = 0;
        while (*pos && *pos != '"' && idx + 1 < max_len) {
            if (*pos == '\\' && *(pos + 1)) pos++;
            out_val[idx++] = *pos++;
        }
        out_val[idx] = '\0';
    } else {
        size_t idx = 0;
        while (*pos && *pos != ',' && *pos != '}' && *pos != ']' && idx + 1 < max_len) {
            out_val[idx++] = *pos++;
        }
        out_val[idx] = '\0';
        trim_whitespace(out_val);
    }
}

/* ========================================================================= */
/* API Handler Functions                                                     */
/* ========================================================================= */

static void api_get_donors(void) {
    Donor* donors = NULL;
    int count = 0;
    load_donors_from_csv(DONORS_CSV_PATH, &donors, &count);
    char* buf = (char*)malloc(LARGE_JSON_BUF);
    if (buf) {
        donors_to_json(donors, count, buf, LARGE_JSON_BUF);
        printf("%s\n", buf);
        free(buf);
    }
    if (donors) free(donors);
}

static void api_add_donor(const char* payload) {
    Donor* donors = NULL;
    int count = 0;
    load_donors_from_csv(DONORS_CSV_PATH, &donors, &count);

    Donor d;
    memset(&d, 0, sizeof(Donor));
    extract_json_field(payload, "id", d.id, sizeof(d.id));
    extract_json_field(payload, "name", d.name, sizeof(d.name));
    char age_str[16];
    extract_json_field(payload, "age", age_str, sizeof(age_str));
    d.age = atoi(age_str);
    extract_json_field(payload, "gender", d.gender, sizeof(d.gender));
    extract_json_field(payload, "blood_group", d.blood_group, sizeof(d.blood_group));
    extract_json_field(payload, "organ_type", d.organ_type, sizeof(d.organ_type));
    extract_json_field(payload, "location", d.location, sizeof(d.location));
    extract_json_field(payload, "hla_type", d.hla_type, sizeof(d.hla_type));
    extract_json_field(payload, "status", d.status, sizeof(d.status));
    extract_json_field(payload, "reg_date", d.reg_date, sizeof(d.reg_date));

    int success = donor_add(&donors, &count, &d);
    if (success) {
        save_donors_to_csv(DONORS_CSV_PATH, donors, count);
        printf("{\"status\":\"success\",\"message\":\"Donor added successfully\",\"id\":\"%s\"}\n", d.id);
    } else {
        printf("{\"status\":\"error\",\"message\":\"Failed to add donor (duplicate ID or invalid data)\"}\n");
    }
    if (donors) free(donors);
}

static void api_update_donor(const char* id, const char* payload) {
    Donor* donors = NULL;
    int count = 0;
    load_donors_from_csv(DONORS_CSV_PATH, &donors, &count);

    Donor d;
    memset(&d, 0, sizeof(Donor));
    extract_json_field(payload, "name", d.name, sizeof(d.name));
    char age_str[16];
    extract_json_field(payload, "age", age_str, sizeof(age_str));
    d.age = atoi(age_str);
    extract_json_field(payload, "gender", d.gender, sizeof(d.gender));
    extract_json_field(payload, "blood_group", d.blood_group, sizeof(d.blood_group));
    extract_json_field(payload, "organ_type", d.organ_type, sizeof(d.organ_type));
    extract_json_field(payload, "location", d.location, sizeof(d.location));
    extract_json_field(payload, "hla_type", d.hla_type, sizeof(d.hla_type));
    extract_json_field(payload, "status", d.status, sizeof(d.status));

    int success = donor_update(donors, count, id, &d);
    if (success) {
        save_donors_to_csv(DONORS_CSV_PATH, donors, count);
        printf("{\"status\":\"success\",\"message\":\"Donor %s updated successfully\"}\n", id);
    } else {
        printf("{\"status\":\"error\",\"message\":\"Donor %s not found\"}\n", id);
    }
    if (donors) free(donors);
}

static void api_delete_donor(const char* id) {
    Donor* donors = NULL;
    int count = 0;
    load_donors_from_csv(DONORS_CSV_PATH, &donors, &count);

    int success = donor_delete(&donors, &count, id);
    if (success) {
        save_donors_to_csv(DONORS_CSV_PATH, donors, count);
        printf("{\"status\":\"success\",\"message\":\"Donor %s deleted successfully\"}\n", id);
    } else {
        printf("{\"status\":\"error\",\"message\":\"Donor %s not found\"}\n", id);
    }
    if (donors) free(donors);
}

static void api_get_recipients(void) {
    Recipient* recipients = NULL;
    int count = 0;
    load_recipients_from_csv(RECIPIENTS_CSV_PATH, &recipients, &count);
    char* buf = (char*)malloc(LARGE_JSON_BUF);
    if (buf) {
        recipients_to_json(recipients, count, buf, LARGE_JSON_BUF);
        printf("%s\n", buf);
        free(buf);
    }
    if (recipients) free(recipients);
}

static void api_add_recipient(const char* payload) {
    Recipient* recipients = NULL;
    int count = 0;
    load_recipients_from_csv(RECIPIENTS_CSV_PATH, &recipients, &count);

    Recipient r;
    memset(&r, 0, sizeof(Recipient));
    extract_json_field(payload, "id", r.id, sizeof(r.id));
    extract_json_field(payload, "name", r.name, sizeof(r.name));
    char age_str[16];
    extract_json_field(payload, "age", age_str, sizeof(age_str));
    r.age = atoi(age_str);
    extract_json_field(payload, "gender", r.gender, sizeof(r.gender));
    extract_json_field(payload, "blood_group", r.blood_group, sizeof(r.blood_group));
    extract_json_field(payload, "required_organ", r.required_organ, sizeof(r.required_organ));
    char urg_str[16];
    extract_json_field(payload, "urgency_level", urg_str, sizeof(urg_str));
    r.urgency_level = atoi(urg_str);
    extract_json_field(payload, "location", r.location, sizeof(r.location));
    char wait_str[16];
    extract_json_field(payload, "waiting_time", wait_str, sizeof(wait_str));
    r.waiting_time = atoi(wait_str);
    extract_json_field(payload, "hla_type", r.hla_type, sizeof(r.hla_type));
    extract_json_field(payload, "status", r.status, sizeof(r.status));
    extract_json_field(payload, "reg_date", r.reg_date, sizeof(r.reg_date));

    int success = recipient_add(&recipients, &count, &r);
    if (success) {
        save_recipients_to_csv(RECIPIENTS_CSV_PATH, recipients, count);
        printf("{\"status\":\"success\",\"message\":\"Recipient added successfully\",\"id\":\"%s\"}\n", r.id);
    } else {
        printf("{\"status\":\"error\",\"message\":\"Failed to add recipient (duplicate ID or invalid data)\"}\n");
    }
    if (recipients) free(recipients);
}

static void api_update_recipient(const char* id, const char* payload) {
    Recipient* recipients = NULL;
    int count = 0;
    load_recipients_from_csv(RECIPIENTS_CSV_PATH, &recipients, &count);

    Recipient r;
    memset(&r, 0, sizeof(Recipient));
    extract_json_field(payload, "name", r.name, sizeof(r.name));
    char age_str[16];
    extract_json_field(payload, "age", age_str, sizeof(age_str));
    r.age = atoi(age_str);
    extract_json_field(payload, "gender", r.gender, sizeof(r.gender));
    extract_json_field(payload, "blood_group", r.blood_group, sizeof(r.blood_group));
    extract_json_field(payload, "required_organ", r.required_organ, sizeof(r.required_organ));
    char urg_str[16];
    extract_json_field(payload, "urgency_level", urg_str, sizeof(urg_str));
    r.urgency_level = atoi(urg_str);
    extract_json_field(payload, "location", r.location, sizeof(r.location));
    char wait_str[16];
    extract_json_field(payload, "waiting_time", wait_str, sizeof(wait_str));
    r.waiting_time = atoi(wait_str);
    extract_json_field(payload, "hla_type", r.hla_type, sizeof(r.hla_type));
    extract_json_field(payload, "status", r.status, sizeof(r.status));

    int success = recipient_update(recipients, count, id, &r);
    if (success) {
        save_recipients_to_csv(RECIPIENTS_CSV_PATH, recipients, count);
        printf("{\"status\":\"success\",\"message\":\"Recipient %s updated successfully\"}\n", id);
    } else {
        printf("{\"status\":\"error\",\"message\":\"Recipient %s not found\"}\n", id);
    }
    if (recipients) free(recipients);
}

static void api_delete_recipient(const char* id) {
    Recipient* recipients = NULL;
    int count = 0;
    load_recipients_from_csv(RECIPIENTS_CSV_PATH, &recipients, &count);

    int success = recipient_delete(&recipients, &count, id);
    if (success) {
        save_recipients_to_csv(RECIPIENTS_CSV_PATH, recipients, count);
        printf("{\"status\":\"success\",\"message\":\"Recipient %s deleted successfully\"}\n", id);
    } else {
        printf("{\"status\":\"error\",\"message\":\"Recipient %s not found\"}\n", id);
    }
    if (recipients) free(recipients);
}

static void api_get_graph_data(void) {
    Donor* donors = NULL;
    int d_count = 0;
    Recipient* recipients = NULL;
    int r_count = 0;

    load_donors_from_csv(DONORS_CSV_PATH, &donors, &d_count);
    load_recipients_from_csv(RECIPIENTS_CSV_PATH, &recipients, &r_count);

    long dummy = 0;
    BipartiteGraph* g = create_bipartite_graph(donors, d_count, recipients, r_count, &dummy);

    char* buf = (char*)malloc(LARGE_JSON_BUF);
    if (buf && g) {
        graph_to_json(g, buf, LARGE_JSON_BUF);
        printf("%s\n", buf);
        free(buf);
    }

    if (g) free_bipartite_graph(g);
    if (donors) free(donors);
    if (recipients) free(recipients);
}

static void api_run_bfs(const char* start_donor_id) {
    Donor* donors = NULL;
    int d_count = 0;
    Recipient* recipients = NULL;
    int r_count = 0;

    load_donors_from_csv(DONORS_CSV_PATH, &donors, &d_count);
    load_recipients_from_csv(RECIPIENTS_CSV_PATH, &recipients, &r_count);

    long dummy = 0;
    BipartiteGraph* g = create_bipartite_graph(donors, d_count, recipients, r_count, &dummy);

    int start_idx = 0;
    if (start_donor_id && strlen(start_donor_id) > 0) {
        int found = donor_find_by_id(donors, d_count, start_donor_id);
        if (found != -1) start_idx = found;
    }

    char* log_buf = (char*)malloc(16384);
    int vis_d = 0, vis_r = 0;
    if (log_buf && g) {
        graph_bfs_traversal(g, start_idx, log_buf, 16384, &vis_d, &vis_r);
        char* esc_log = (char*)malloc(32768);
        if (esc_log) {
            json_escape_string(log_buf, esc_log, 32768);
            printf("{\"status\":\"success\",\"traversal\":\"BFS\",\"start_donor\":\"%s\","
                   "\"visited_donors\":%d,\"visited_recipients\":%d,\"log\":\"%s\"}\n",
                   donors[start_idx].id, vis_d, vis_r, esc_log);
            free(esc_log);
        }
        free(log_buf);
    }

    if (g) free_bipartite_graph(g);
    if (donors) free(donors);
    if (recipients) free(recipients);
}

static void api_run_dfs(const char* start_donor_id) {
    Donor* donors = NULL;
    int d_count = 0;
    Recipient* recipients = NULL;
    int r_count = 0;

    load_donors_from_csv(DONORS_CSV_PATH, &donors, &d_count);
    load_recipients_from_csv(RECIPIENTS_CSV_PATH, &recipients, &r_count);

    long dummy = 0;
    BipartiteGraph* g = create_bipartite_graph(donors, d_count, recipients, r_count, &dummy);

    int start_idx = 0;
    if (start_donor_id && strlen(start_donor_id) > 0) {
        int found = donor_find_by_id(donors, d_count, start_donor_id);
        if (found != -1) start_idx = found;
    }

    char* log_buf = (char*)malloc(16384);
    int vis_d = 0, vis_r = 0;
    if (log_buf && g) {
        graph_dfs_traversal(g, start_idx, log_buf, 16384, &vis_d, &vis_r);
        char* esc_log = (char*)malloc(32768);
        if (esc_log) {
            json_escape_string(log_buf, esc_log, 32768);
            printf("{\"status\":\"success\",\"traversal\":\"DFS\",\"start_donor\":\"%s\","
                   "\"visited_donors\":%d,\"visited_recipients\":%d,\"log\":\"%s\"}\n",
                   donors[start_idx].id, vis_d, vis_r, esc_log);
            free(esc_log);
        }
        free(log_buf);
    }

    if (g) free_bipartite_graph(g);
    if (donors) free(donors);
    if (recipients) free(recipients);
}

static void api_run_graph_matching(void) {
    Donor* donors = NULL;
    int d_count = 0;
    Recipient* recipients = NULL;
    int r_count = 0;

    load_donors_from_csv(DONORS_CSV_PATH, &donors, &d_count);
    load_recipients_from_csv(RECIPIENTS_CSV_PATH, &recipients, &r_count);

    long graph_build_comparisons = 0;
    BipartiteGraph* g = create_bipartite_graph(donors, d_count, recipients, r_count, &graph_build_comparisons);

    MatchingResult res = run_graph_matching(g, donors, d_count, recipients, r_count);

    char* buf = (char*)malloc(LARGE_JSON_BUF);
    if (buf) {
        matching_result_to_json(&res, "Graph-Based Algorithm", buf, LARGE_JSON_BUF);
        printf("%s\n", buf);
        free(buf);
    }

    if (g) free_bipartite_graph(g);
    if (donors) free(donors);
    if (recipients) free(recipients);
}

static void api_run_traditional_matching(void) {
    Donor* donors = NULL;
    int d_count = 0;
    Recipient* recipients = NULL;
    int r_count = 0;

    load_donors_from_csv(DONORS_CSV_PATH, &donors, &d_count);
    load_recipients_from_csv(RECIPIENTS_CSV_PATH, &recipients, &r_count);

    MatchingResult res = run_traditional_matching(donors, d_count, recipients, r_count);

    char* buf = (char*)malloc(LARGE_JSON_BUF);
    if (buf) {
        matching_result_to_json(&res, "Traditional Sequential Method", buf, LARGE_JSON_BUF);
        printf("%s\n", buf);
        free(buf);
    }

    if (donors) free(donors);
    if (recipients) free(recipients);
}

static void api_compare_performance(void) {
    Donor* donors = NULL;
    int d_count = 0;
    Recipient* recipients = NULL;
    int r_count = 0;

    load_donors_from_csv(DONORS_CSV_PATH, &donors, &d_count);
    load_recipients_from_csv(RECIPIENTS_CSV_PATH, &recipients, &r_count);

    long dummy = 0;
    BipartiteGraph* g = create_bipartite_graph(donors, d_count, recipients, r_count, &dummy);

    MatchingResult* g_res = (MatchingResult*)malloc(sizeof(MatchingResult));
    MatchingResult* t_res = (MatchingResult*)malloc(sizeof(MatchingResult));
    if (g_res && t_res) {
        PerformanceComparison comp = compare_algorithms(g, donors, d_count, recipients, r_count, g_res, t_res);

        char* buf = (char*)malloc(LARGE_JSON_BUF);
        if (buf) {
            performance_comparison_to_json(&comp, g_res, t_res, buf, LARGE_JSON_BUF);
            printf("%s\n", buf);
            free(buf);
        }
        free(g_res);
        free(t_res);
    }

    if (g) free_bipartite_graph(g);
    if (donors) free(donors);
    if (recipients) free(recipients);
}

static void api_reset_data(void) {
    int res = reset_data_to_sample(DONORS_CSV_PATH, RECIPIENTS_CSV_PATH,
                                   DONORS_SAMPLE_PATH, RECIPIENTS_SAMPLE_PATH);
    if (res) {
        printf("{\"status\":\"success\",\"message\":\"Sample datasets successfully restored to donors.csv and recipients.csv\"}\n");
    } else {
        printf("{\"status\":\"error\",\"message\":\"Failed to restore sample datasets\"}\n");
    }
}

static void api_get_stats(void) {
    Donor* donors = NULL;
    int d_count = 0;
    Recipient* recipients = NULL;
    int r_count = 0;

    load_donors_from_csv(DONORS_CSV_PATH, &donors, &d_count);
    load_recipients_from_csv(RECIPIENTS_CSV_PATH, &recipients, &r_count);

    long dummy = 0;
    BipartiteGraph* g = create_bipartite_graph(donors, d_count, recipients, r_count, &dummy);
    MatchingResult g_res = run_graph_matching(g, donors, d_count, recipients, r_count);

    int pending_recipients = r_count - g_res.pair_count;
    if (pending_recipients < 0) pending_recipients = 0;

    printf("{\"total_donors\":%d,\"total_recipients\":%d,\"matched_pairs\":%d,"
           "\"pending_recipients\":%d,\"total_edges\":%d,\"avg_score\":%.2f,"
           "\"critical_matched\":%d,\"critical_total\":%d}\n",
           d_count, r_count, g_res.pair_count,
           pending_recipients, g ? g->total_edges : 0, g_res.avg_score,
           g_res.critical_matched, g_res.critical_total);

    if (g) free_bipartite_graph(g);
    if (donors) free(donors);
    if (recipients) free(recipients);
}

/* Scalability Benchmark Generator for Module 5 */
static void api_benchmark(int target_size) {
    if (target_size < 10) target_size = 50;
    if (target_size > 400) target_size = 400;

    Donor* donors = (Donor*)malloc(sizeof(Donor) * target_size);
    Recipient* recipients = (Recipient*)malloc(sizeof(Recipient) * target_size);

    const char* bgs[] = {"O+", "O-", "A+", "A-", "B+", "B-", "AB+", "AB-"};
    const char* organs[] = {"Kidney", "Liver", "Heart", "Lung", "Pancreas", "Cornea"};
    const char* cities[] = {"Boston", "New York", "Chicago", "Houston", "Seattle"};

    srand(12345);
    for (int i = 0; i < target_size; i++) {
        snprintf(donors[i].id, sizeof(donors[i].id), "DON-%04d", 1000 + i);
        snprintf(donors[i].name, sizeof(donors[i].name), "Donor_%d", i + 1);
        donors[i].age = 20 + (rand() % 45);
        strncpy(donors[i].gender, (rand() % 2 == 0) ? "Male" : "Female", sizeof(donors[i].gender) - 1);
        strncpy(donors[i].blood_group, bgs[rand() % 8], sizeof(donors[i].blood_group) - 1);
        strncpy(donors[i].organ_type, organs[rand() % 6], sizeof(donors[i].organ_type) - 1);
        strncpy(donors[i].location, cities[rand() % 5], sizeof(donors[i].location) - 1);
        strncpy(donors[i].hla_type, "A2-A24-B44-B51-DR4-DR11", sizeof(donors[i].hla_type) - 1);
        strncpy(donors[i].status, "Available", sizeof(donors[i].status) - 1);
        strncpy(donors[i].reg_date, "2026-01-01", sizeof(donors[i].reg_date) - 1);

        snprintf(recipients[i].id, sizeof(recipients[i].id), "REC-%04d", 2000 + i);
        snprintf(recipients[i].name, sizeof(recipients[i].name), "Patient_%d", i + 1);
        recipients[i].age = 15 + (rand() % 60);
        strncpy(recipients[i].gender, (rand() % 2 == 0) ? "Male" : "Female", sizeof(recipients[i].gender) - 1);
        strncpy(recipients[i].blood_group, bgs[rand() % 8], sizeof(recipients[i].blood_group) - 1);
        strncpy(recipients[i].required_organ, organs[rand() % 6], sizeof(recipients[i].required_organ) - 1);
        recipients[i].urgency_level = 1 + (rand() % 3);
        strncpy(recipients[i].location, cities[rand() % 5], sizeof(recipients[i].location) - 1);
        recipients[i].waiting_time = 30 + (rand() % 600);
        strncpy(recipients[i].hla_type, "A2-A24-B44-B51-DR4-DR11", sizeof(recipients[i].hla_type) - 1);
        strncpy(recipients[i].status, "Waiting", sizeof(recipients[i].status) - 1);
        strncpy(recipients[i].reg_date, "2025-06-01", sizeof(recipients[i].reg_date) - 1);
    }

    long build_comparisons = 0;
    BipartiteGraph* g = create_bipartite_graph(donors, target_size, recipients, target_size, &build_comparisons);

    MatchingResult g_res, t_res;
    PerformanceComparison comp = compare_algorithms(g, donors, target_size, recipients, target_size, &g_res, &t_res);

    printf("{\"benchmark_size\":%d,\"graph_time_us\":%.2f,\"traditional_time_us\":%.2f,"
           "\"graph_comparisons\":%ld,\"traditional_comparisons\":%ld,"
           "\"graph_avg_score\":%.2f,\"traditional_avg_score\":%.2f,"
           "\"graph_pairs\":%d,\"traditional_pairs\":%d}\n",
           target_size, comp.graph_time_us, comp.traditional_time_us,
           comp.graph_comparisons, comp.traditional_comparisons,
           comp.graph_avg_score, comp.traditional_avg_score,
           comp.graph_pairs, comp.traditional_pairs);

    if (g) free_bipartite_graph(g);
    free(donors);
    free(recipients);
}

/* ========================================================================= */
/* CLI Interactive Terminal Mode                                             */
/* ========================================================================= */

static void print_banner(void) {
    printf("\n");
    printf("===============================================================================\n");
    printf("  SMART ORGAN DONOR-RECIPIENT MATCHING SYSTEM USING GRAPH ALGORITHMS\n");
    printf("  Capstone Academic Simulation Project | C Language & Graph Theory\n");
    printf("===============================================================================\n");
}

static void cli_view_donors(void) {
    Donor* donors = NULL;
    int count = 0;
    load_donors_from_csv(DONORS_CSV_PATH, &donors, &count);
    printf("\n--- REGISTERED ORGAN DONORS (Total: %d) ---\n", count);
    printf("%-10s %-20s %-4s %-8s %-6s %-10s %-14s %-10s\n",
           "ID", "Name", "Age", "Gender", "Blood", "Organ", "Location", "Status");
    printf("------------------------------------------------------------------------------------\n");
    for (int i = 0; i < count; i++) {
        printf("%-10s %-20s %-4d %-8s %-6s %-10s %-14s %-10s\n",
               donors[i].id, donors[i].name, donors[i].age, donors[i].gender,
               donors[i].blood_group, donors[i].organ_type, donors[i].location, donors[i].status);
    }
    if (donors) free(donors);
}

static void cli_view_recipients(void) {
    Recipient* recipients = NULL;
    int count = 0;
    load_recipients_from_csv(RECIPIENTS_CSV_PATH, &recipients, &count);
    printf("\n--- REGISTERED ORGAN RECIPIENTS (Total: %d) ---\n", count);
    printf("%-10s %-20s %-4s %-6s %-10s %-10s %-14s %-8s %-8s\n",
           "ID", "Name", "Age", "Blood", "Organ Req", "Urgency", "Location", "Wait(d)", "Status");
    printf("--------------------------------------------------------------------------------------------\n");
    for (int i = 0; i < count; i++) {
        printf("%-10s %-20s %-4d %-6s %-10s %-10s %-14s %-8d %-8s\n",
               recipients[i].id, recipients[i].name, recipients[i].age,
               recipients[i].blood_group, recipients[i].required_organ,
               urgency_to_string(recipients[i].urgency_level),
               recipients[i].location, recipients[i].waiting_time, recipients[i].status);
    }
    if (recipients) free(recipients);
}

static void cli_run_graph_matching(void) {
    Donor* donors = NULL;
    int d_count = 0;
    Recipient* recipients = NULL;
    int r_count = 0;

    load_donors_from_csv(DONORS_CSV_PATH, &donors, &d_count);
    load_recipients_from_csv(RECIPIENTS_CSV_PATH, &recipients, &r_count);

    long build_comps = 0;
    BipartiteGraph* g = create_bipartite_graph(donors, d_count, recipients, r_count, &build_comps);

    printf("\n[Bipartite Graph Created] %d Donors, %d Recipients, %d Compatible Edges.\n",
           d_count, r_count, g->total_edges);

    MatchingResult res = run_graph_matching(g, donors, d_count, recipients, r_count);

    printf("\n===============================================================================\n");
    printf("  GRAPH-BASED MATCHING RESULTS (Maximum-Weight Optimal Matching)\n");
    printf("===============================================================================\n");
    printf("Matched Pairs: %d | Total Score: %.2f | Average Score: %.2f%%\n",
           res.pair_count, res.total_score, res.avg_score);
    printf("Critical Patients Saved: %d / %d | Urgent Patients: %d / %d | Standard: %d / %d\n",
           res.critical_matched, res.critical_total,
           res.urgent_matched, res.urgent_total,
           res.standard_matched, res.standard_total);
    printf("Graph Traversal/Operations: %ld | Execution Time: %.2f microseconds\n",
           res.comparisons_count, res.execution_time_us);
    printf("-------------------------------------------------------------------------------\n");

    for (int i = 0; i < res.pair_count; i++) {
        MatchedPair* p = &res.pairs[i];
        printf("\n[%d] %s (%s, %s, %s)  ===>  %s (%s, %s, Urgency: %s)\n",
               i + 1, p->donor_id, p->donor_name, p->organ, p->donor_bg,
               p->recipient_id, p->recipient_name, p->recipient_bg, urgency_to_string(p->urgency_level));
        printf("    Compatibility Score: %.1f / 100.0\n", p->compatibility_score);
        printf("    Clinical Explanation: %s\n", p->explanation);
    }

    if (g) free_bipartite_graph(g);
    if (donors) free(donors);
    if (recipients) free(recipients);
}

static void cli_run_traditional_matching(void) {
    Donor* donors = NULL;
    int d_count = 0;
    Recipient* recipients = NULL;
    int r_count = 0;

    load_donors_from_csv(DONORS_CSV_PATH, &donors, &d_count);
    load_recipients_from_csv(RECIPIENTS_CSV_PATH, &recipients, &r_count);

    MatchingResult res = run_traditional_matching(donors, d_count, recipients, r_count);

    printf("\n===============================================================================\n");
    printf("  TRADITIONAL SEQUENTIAL MATCHING (First-Come First-Served Scan)\n");
    printf("===============================================================================\n");
    printf("Matched Pairs: %d | Total Score: %.2f | Average Score: %.2f%%\n",
           res.pair_count, res.total_score, res.avg_score);
    printf("Critical Patients Saved: %d / %d | Urgent Patients: %d / %d\n",
           res.critical_matched, res.critical_total, res.urgent_matched, res.urgent_total);
    printf("Pairwise Sequential Comparisons: %ld | Execution Time: %.2f microseconds\n",
           res.comparisons_count, res.execution_time_us);
    printf("-------------------------------------------------------------------------------\n");

    for (int i = 0; i < res.pair_count; i++) {
        MatchedPair* p = &res.pairs[i];
        printf("[%d] %s (%s, %s) -> %s (%s, %s, Urgency: %s) | Score: %.1f\n",
               i + 1, p->donor_id, p->donor_name, p->organ,
               p->recipient_id, p->recipient_name, p->recipient_bg,
               urgency_to_string(p->urgency_level), p->compatibility_score);
    }

    if (donors) free(donors);
    if (recipients) free(recipients);
}

static void cli_compare_performance(void) {
    Donor* donors = NULL;
    int d_count = 0;
    Recipient* recipients = NULL;
    int r_count = 0;

    load_donors_from_csv(DONORS_CSV_PATH, &donors, &d_count);
    load_recipients_from_csv(RECIPIENTS_CSV_PATH, &recipients, &r_count);

    long dummy = 0;
    BipartiteGraph* g = create_bipartite_graph(donors, d_count, recipients, r_count, &dummy);

    MatchingResult* g_res = (MatchingResult*)malloc(sizeof(MatchingResult));
    MatchingResult* t_res = (MatchingResult*)malloc(sizeof(MatchingResult));
    if (!g_res || !t_res) {
        if (g_res) free(g_res);
        if (t_res) free(t_res);
        if (g) free_bipartite_graph(g);
        if (donors) free(donors);
        if (recipients) free(recipients);
        return;
    }

    PerformanceComparison comp = compare_algorithms(g, donors, d_count, recipients, r_count, g_res, t_res);

    printf("\n===============================================================================\n");
    printf("  PERFORMANCE COMPARISON: GRAPH ALGORITHM VS TRADITIONAL SEQUENTIAL\n");
    printf("===============================================================================\n");
    printf("%-35s %-20s %-20s\n", "Metric", "Graph-Based Method", "Traditional Method");
    printf("-------------------------------------------------------------------------------\n");
    printf("%-35s %-20ld %-20ld\n", "Comparisons / Operations", comp.graph_comparisons, comp.traditional_comparisons);
    printf("%-35s %-20.2f us %-20.2f us\n", "Execution Time", comp.graph_time_us, comp.traditional_time_us);
    printf("%-35s %-20d %-20d\n", "Compatible Pairs Matched", comp.graph_pairs, comp.traditional_pairs);
    printf("%-35s %-20.2f %% %-20.2f %%\n", "Average Match Score", comp.graph_avg_score, comp.traditional_avg_score);
    printf("%-35s %-20.2f %-20.2f\n", "Total Quality Points", comp.graph_total_score, comp.traditional_total_score);
    printf("%-35s %d of %-15d %d of %-15d\n", "Critical Patients Saved",
           comp.graph_critical_matched, comp.total_critical_recipients,
           comp.traditional_critical_matched, comp.total_critical_recipients);
    printf("-------------------------------------------------------------------------------\n");
    printf("Match Quality Improvement : +%.2f%%\n", comp.score_improvement_pct);
    printf("Computational Efficiency  : Graph Adjacency List eliminates exhaustive pairwise scans.\n");
    printf("===============================================================================\n");

    free(g_res);
    free(t_res);
    if (g) free_bipartite_graph(g);
    if (donors) free(donors);
    if (recipients) free(recipients);
}

static void cli_interactive_menu(void) {
    int choice = -1;
    char line[64];

    while (choice != 0) {
        print_banner();
        printf("1. View All Donors\n");
        printf("2. View All Recipients\n");
        printf("3. Run Graph-Based Weighted Matching (Module 3)\n");
        printf("4. Run BFS Traversal Demonstration (Module 3)\n");
        printf("5. Run DFS Traversal Demonstration (Module 3)\n");
        printf("6. Run Traditional Sequential Matching (Module 4)\n");
        printf("7. Performance Comparison: Graph vs Traditional (Module 5)\n");
        printf("8. Reset Datasets to Default Sample Files\n");
        printf("0. Exit\n");
        printf("-------------------------------------------------------------------------------\n");
        printf("Enter your choice [0-8]: ");

        if (!fgets(line, sizeof(line), stdin)) break;
        choice = atoi(line);

        switch (choice) {
            case 1: cli_view_donors(); break;
            case 2: cli_view_recipients(); break;
            case 3: cli_run_graph_matching(); break;
            case 4: api_run_bfs("DON-1001"); break;
            case 5: api_run_dfs("DON-1001"); break;
            case 6: cli_run_traditional_matching(); break;
            case 7: cli_compare_performance(); break;
            case 8: api_reset_data(); break;
            case 0: printf("\nExiting Organ Donation System. Goodbye!\n"); break;
            default: printf("\nInvalid choice. Please select 0 to 8.\n"); break;
        }

        if (choice != 0) {
            printf("\nPress Enter to return to menu...");
            fgets(line, sizeof(line), stdin);
        }
    }
}

/* ========================================================================= */
/* Main Function                                                             */
/* ========================================================================= */

int main(int argc, char* argv[]) {
    /* If no arguments or --cli is provided, launch interactive CLI */
    if (argc == 1 || (argc == 2 && strcmp(argv[1], "--cli") == 0)) {
        cli_interactive_menu();
        return 0;
    }

    /* API execution mode: --api <action> [args...] */
    if (strcmp(argv[1], "--api") == 0 && argc >= 3) {
        const char* action = argv[2];

        if (strcmp(action, "get_donors") == 0) {
            api_get_donors();
        } else if (strcmp(action, "add_donor") == 0 && argc >= 4) {
            api_add_donor(argv[3]);
        } else if (strcmp(action, "update_donor") == 0 && argc >= 5) {
            api_update_donor(argv[3], argv[4]);
        } else if (strcmp(action, "delete_donor") == 0 && argc >= 4) {
            api_delete_donor(argv[3]);
        } else if (strcmp(action, "get_recipients") == 0) {
            api_get_recipients();
        } else if (strcmp(action, "add_recipient") == 0 && argc >= 4) {
            api_add_recipient(argv[3]);
        } else if (strcmp(action, "update_recipient") == 0 && argc >= 5) {
            api_update_recipient(argv[3], argv[4]);
        } else if (strcmp(action, "delete_recipient") == 0 && argc >= 4) {
            api_delete_recipient(argv[3]);
        } else if (strcmp(action, "get_graph_data") == 0) {
            api_get_graph_data();
        } else if (strcmp(action, "run_bfs") == 0) {
            const char* start_id = (argc >= 4) ? argv[3] : "DON-1001";
            api_run_bfs(start_id);
        } else if (strcmp(action, "run_dfs") == 0) {
            const char* start_id = (argc >= 4) ? argv[3] : "DON-1001";
            api_run_dfs(start_id);
        } else if (strcmp(action, "run_graph_matching") == 0) {
            api_run_graph_matching();
        } else if (strcmp(action, "run_traditional_matching") == 0) {
            api_run_traditional_matching();
        } else if (strcmp(action, "compare_performance") == 0) {
            api_compare_performance();
        } else if (strcmp(action, "reset_data") == 0) {
            api_reset_data();
        } else if (strcmp(action, "get_stats") == 0) {
            api_get_stats();
        } else if (strcmp(action, "benchmark") == 0) {
            int sz = (argc >= 4) ? atoi(argv[3]) : 50;
            api_benchmark(sz);
        } else {
            printf("{\"status\":\"error\",\"message\":\"Unknown API action: %s\"}\n", action);
        }
        return 0;
    }

    fprintf(stderr, "Usage: %s [--cli | --api <action> [args...]]\n", argv[0]);
    return 1;
}
