/**
 * matching.h - Graph-based weighted bipartite matching algorithm
 * Module 3: Graph-Based Matching
 */

#ifndef MATCHING_H
#define MATCHING_H

#include "../include/common.h"
#include "graph.h"

/* Executes Graph-Based Weighted Matching on the Bipartite Graph */
MatchingResult run_graph_matching(const BipartiteGraph* g, Donor* donors, int num_donors,
                                  Recipient* recipients, int num_recipients);

/* Serializes MatchingResult to JSON for API / Frontend */
void matching_result_to_json(const MatchingResult* result, const char* algorithm_name,
                            char* json_buf, size_t buf_size);

#endif /* MATCHING_H */
