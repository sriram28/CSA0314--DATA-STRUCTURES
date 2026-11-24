/**
 * advanced_matching.h - Advanced Multiple Matching and Explainable AI Allocations
 * Module 7: Alternative Donor Candidates, Unmatched Analysis & Rejection Explanations
 * Module 10: Explainable Multi-Factor Compatibility Parameters
 */

#ifndef ADVANCED_MATCHING_H
#define ADVANCED_MATCHING_H

#include "../include/common.h"
#include "graph.h"

#define MAX_ALTERNATIVES 3

/* Alternative Donor Candidate Structure */
typedef struct {
    char donor_id[MAX_ID_LEN];
    char donor_name[MAX_NAME_LEN];
    char donor_bg[MAX_STR_LEN];
    char donor_location[MAX_LOCATION_LEN];
    int donor_age;
    double compatibility_score;
    int rank; /* 1 = Primary, 2 = Alternative 1, 3 = Alternative 2 */
    char clinical_rationale[MAX_EXPLANATION_LEN];
} AlternativeCandidate;

/* Recipient Allocation with Backup Alternatives */
typedef struct {
    char recipient_id[MAX_ID_LEN];
    char recipient_name[MAX_NAME_LEN];
    char required_organ[MAX_STR_LEN];
    char recipient_bg[MAX_STR_LEN];
    int urgency_level;
    int waiting_time;

    int candidate_count;
    AlternativeCandidate candidates[MAX_ALTERNATIVES];
    bool is_assigned;
} RecipientAllocationPlan;

/* Unmatched Entity Record */
typedef struct {
    char id[MAX_ID_LEN];
    char name[MAX_NAME_LEN];
    char organ[MAX_STR_LEN];
    char blood_group[MAX_STR_LEN];
    char location[MAX_LOCATION_LEN];
    char reason[MAX_EXPLANATION_LEN];
} UnmatchedRecord;

/* Rejected Relationship Record */
typedef struct {
    char donor_id[MAX_ID_LEN];
    char donor_name[MAX_NAME_LEN];
    char recipient_id[MAX_ID_LEN];
    char recipient_name[MAX_NAME_LEN];
    char organ_donor[MAX_STR_LEN];
    char organ_recipient[MAX_STR_LEN];
    double score;
    char rejection_category[64]; /* ABO_MISMATCH, ORGAN_MISMATCH, PREEMPTED_BY_PRIORITY */
    char detailed_reason[MAX_EXPLANATION_LEN];
} RejectedPair;

/* Advanced Matching Full Results */
typedef struct {
    MatchedPair primary_pairs[MAX_RECORDS];
    int primary_pair_count;

    RecipientAllocationPlan plans[MAX_RECORDS];
    int plan_count;

    UnmatchedRecord unmatched_donors[MAX_RECORDS];
    int unmatched_donor_count;

    UnmatchedRecord unmatched_recipients[MAX_RECORDS];
    int unmatched_recipient_count;

    RejectedPair rejected_pairs[MAX_RECORDS * 2];
    int rejected_pair_count;

    double total_score;
    double avg_score;
    double execution_time_us;
} AdvancedMatchingResult;

/* Multi-Parameter Explainable Compatibility Breakdown */
typedef struct {
    char donor_id[MAX_ID_LEN];
    char donor_name[MAX_NAME_LEN];
    char donor_bg[MAX_STR_LEN];
    char donor_organ[MAX_STR_LEN];
    int donor_age;
    char donor_location[MAX_LOCATION_LEN];
    char donor_hla[MAX_HLA_LEN];

    char recipient_id[MAX_ID_LEN];
    char recipient_name[MAX_NAME_LEN];
    char recipient_bg[MAX_STR_LEN];
    char recipient_organ[MAX_STR_LEN];
    int recipient_age;
    char recipient_location[MAX_LOCATION_LEN];
    char recipient_hla[MAX_HLA_LEN];
    int urgency_level;
    int waiting_time;

    bool is_compatible;
    double total_score;

    /* 6 Detailed Component Scores */
    double abo_score;          /* max 25.0 */
    char abo_explanation[128];

    double hla_score;          /* max 25.0 */
    int hla_matching_alleles;  /* 0 to 6 */
    char hla_explanation[128];

    double urgency_score;      /* max 25.0 */
    char urgency_explanation[128];

    double wait_score;         /* max 15.0 */
    char wait_explanation[128];

    double location_score;     /* max 5.0 */
    char location_explanation[128];

    double age_score;          /* max 5.0 */
    char age_explanation[128];

    char full_clinical_summary[MAX_EXPLANATION_LEN];
    char academic_disclaimer[256];
} ExplainableMatchDetail;

/* Function Prototypes */
AdvancedMatchingResult run_advanced_matching(const BipartiteGraph* g,
                                            const Donor* donors, int d_count,
                                            const Recipient* recipients, int r_count);

ExplainableMatchDetail generate_explainable_match(const Donor* d, const Recipient* r);

void advanced_matching_to_json(const AdvancedMatchingResult* res, char* json_buf, size_t max_len);
void explainable_match_to_json(const ExplainableMatchDetail* det, char* json_buf, size_t max_len);

#endif /* ADVANCED_MATCHING_H */
