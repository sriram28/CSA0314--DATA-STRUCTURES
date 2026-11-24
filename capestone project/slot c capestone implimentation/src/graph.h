/**
 * graph.h - Bipartite Graph data structures, Adjacency Lists, Queues, BFS and DFS traversals
 * Module 3: Graph Data Structures & Traversals
 */

#ifndef GRAPH_H
#define GRAPH_H

#include "../include/common.h"
#include <stddef.h>

/* Queue Data Structure for BFS */
typedef struct QueueNode {
    int vertex;
    int is_recipient; /* 0 for donor, 1 for recipient */
    int level;
    struct QueueNode* next;
} QueueNode;

typedef struct {
    QueueNode* front;
    QueueNode* rear;
    int count;
} Queue;

Queue* queue_create(void);
void enqueue(Queue* q, int vertex, int is_recipient, int level);
int dequeue(Queue* q, int* out_vertex, int* out_is_recipient, int* out_level);
int queue_is_empty(const Queue* q);
void queue_free(Queue* q);

/* Bipartite Graph Adjacency List */
typedef struct AdjEdge {
    int recipient_idx;       /* Recipient index in recipient array */
    double compatibility_score;
    char explanation[MAX_EXPLANATION_LEN];
    struct AdjEdge* next;
} AdjEdge;

typedef struct {
    int donor_idx;
    int degree;
    AdjEdge* head;
} DonorVertex;

typedef struct {
    int num_donors;
    int num_recipients;
    Donor* donors;
    Recipient* recipients;
    DonorVertex* donor_vertices;
    int total_edges;
} BipartiteGraph;

/* Graph Construction & Destruction */
BipartiteGraph* create_bipartite_graph(Donor* donors, int num_donors,
                                       Recipient* recipients, int num_recipients,
                                       long* comparisons_count);
void free_bipartite_graph(BipartiteGraph* g);

/* Graph Traversals */
void graph_bfs_traversal(const BipartiteGraph* g, int start_donor_idx,
                         char* log_buffer, size_t buf_size,
                         int* visited_donors_count, int* visited_recipients_count);

void graph_dfs_traversal(const BipartiteGraph* g, int start_donor_idx,
                         char* log_buffer, size_t buf_size,
                         int* visited_donors_count, int* visited_recipients_count);

/* JSON Serialization of Graph for Visualization */
void graph_to_json(const BipartiteGraph* g, char* json_buf, size_t buf_size);

#endif /* GRAPH_H */
