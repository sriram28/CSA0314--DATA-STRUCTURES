/**
 * common.h - Common definitions, structures, and enums for Organ Donation System
 * Capstone Project: Improving Organ Donor-Recipient Matching Using Graph Algorithms
 */

#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#define MAX_ID_LEN 16
#define MAX_NAME_LEN 64
#define MAX_STR_LEN 32
#define MAX_LOCATION_LEN 64
#define MAX_HLA_LEN 64
#define MAX_EXPLANATION_LEN 512
#define MAX_DATE_LEN 16
#define MAX_RECORDS 150

/* Urgency Levels for Recipients */
typedef enum {
    URGENCY_CRITICAL = 1,  /* High mortality risk; immediate transplant needed */
    URGENCY_URGENT = 2,    /* Rapidly deteriorating; priority transplant needed */
    URGENCY_STANDARD = 3   /* Stable on maintenance therapy / dialysis */
} UrgencyLevel;

/* Donor Record Structure */
typedef struct {
    char id[MAX_ID_LEN];
    char name[MAX_NAME_LEN];
    int age;
    char gender[MAX_STR_LEN];
    char blood_group[MAX_STR_LEN];
    char organ_type[MAX_STR_LEN];
    char location[MAX_LOCATION_LEN];
    char hla_type[MAX_HLA_LEN];
    char status[MAX_STR_LEN]; /* Available, Matched, Reserved */
    char reg_date[MAX_DATE_LEN];
} Donor;

/* Recipient Record Structure */
typedef struct {
    char id[MAX_ID_LEN];
    char name[MAX_NAME_LEN];
    int age;
    char gender[MAX_STR_LEN];
    char blood_group[MAX_STR_LEN];
    char required_organ[MAX_STR_LEN];
    int urgency_level; /* 1=Critical, 2=Urgent, 3=Standard */
    char location[MAX_LOCATION_LEN];
    int waiting_time; /* in days */
    char hla_type[MAX_HLA_LEN];
    char status[MAX_STR_LEN]; /* Waiting, Matched, Inactive */
    char reg_date[MAX_DATE_LEN];
} Recipient;

/* Matched Pair Information */
typedef struct {
    char donor_id[MAX_ID_LEN];
    char donor_name[MAX_NAME_LEN];
    char donor_bg[MAX_STR_LEN];
    int donor_age;
    char donor_location[MAX_LOCATION_LEN];

    char recipient_id[MAX_ID_LEN];
    char recipient_name[MAX_NAME_LEN];
    char recipient_bg[MAX_STR_LEN];
    int recipient_age;
    char recipient_location[MAX_LOCATION_LEN];
    int urgency_level;
    int waiting_time;

    char organ[MAX_STR_LEN];
    double compatibility_score;
    char explanation[MAX_EXPLANATION_LEN];
} MatchedPair;

/* Overall Matching Results */
typedef struct {
    MatchedPair pairs[MAX_RECORDS];
    int pair_count;
    double total_score;
    double avg_score;
    long comparisons_count;
    double execution_time_us; /* execution time in microseconds */
    int critical_matched;
    int critical_total;
    int urgent_matched;
    int urgent_total;
    int standard_matched;
    int standard_total;
} MatchingResult;

/* Algorithm Performance Comparison */
typedef struct {
    long traditional_comparisons;
    long graph_comparisons;
    double traditional_time_us;
    double graph_time_us;
    int traditional_pairs;
    int graph_pairs;
    double traditional_avg_score;
    double graph_avg_score;
    double traditional_total_score;
    double graph_total_score;
    int traditional_critical_matched;
    int graph_critical_matched;
    int total_critical_recipients;
    double efficiency_gain_pct;
    double score_improvement_pct;
} PerformanceComparison;

#endif /* COMMON_H */
