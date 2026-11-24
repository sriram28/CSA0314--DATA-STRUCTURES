/**
 * traditional.h - Traditional Sequential Matching & Performance Comparison
 * Modules 4 & 5
 */

#ifndef TRADITIONAL_H
#define TRADITIONAL_H

#include "../include/common.h"
#include "graph.h"

/* Traditional Sequential (First-Come, First-Served) Matching */
MatchingResult run_traditional_matching(Donor* donors, int num_donors,
                                        Recipient* recipients, int num_recipients);

/* Performance Comparison between Graph-Based and Traditional */
PerformanceComparison compare_algorithms(const BipartiteGraph* g, Donor* donors, int num_donors,
                                        Recipient* recipients, int num_recipients,
                                        MatchingResult* out_graph_res, MatchingResult* out_trad_res);

/* Serializes PerformanceComparison to JSON */
void performance_comparison_to_json(const PerformanceComparison* comp,
                                   const MatchingResult* graph_res,
                                   const MatchingResult* trad_res,
                                   char* json_buf, size_t buf_size);

#endif /* TRADITIONAL_H */
