/**
 * graph.c - Implementation of Bipartite Graph, Adjacency Lists, Queues, BFS & DFS
 */

#include "graph.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Queue Implementation */
Queue* queue_create(void) {
    Queue* q = (Queue*)malloc(sizeof(Queue));
    if (q) {
        q->front = NULL;
        q->rear = NULL;
        q->count = 0;
    }
    return q;
}

void enqueue(Queue* q, int vertex, int is_recipient, int level) {
    if (!q) return;
    QueueNode* node = (QueueNode*)malloc(sizeof(QueueNode));
    if (!node) return;
    node->vertex = vertex;
    node->is_recipient = is_recipient;
    node->level = level;
    node->next = NULL;

    if (q->rear == NULL) {
        q->front = node;
        q->rear = node;
    } else {
        q->rear->next = node;
        q->rear = node;
    }
    q->count++;
}

int dequeue(Queue* q, int* out_vertex, int* out_is_recipient, int* out_level) {
    if (!q || q->front == NULL) return 0;
    QueueNode* node = q->front;
    if (out_vertex) *out_vertex = node->vertex;
    if (out_is_recipient) *out_is_recipient = node->is_recipient;
    if (out_level) *out_level = node->level;

    q->front = q->front->next;
    if (q->front == NULL) {
        q->rear = NULL;
    }
    free(node);
    q->count--;
    return 1;
}

int queue_is_empty(const Queue* q) {
    return (q == NULL || q->front == NULL);
}

void queue_free(Queue* q) {
    if (!q) return;
    QueueNode* curr = q->front;
    while (curr) {
        QueueNode* next = curr->next;
        free(curr);
        curr = next;
    }
    free(q);
}

/**
 * Creates a Bipartite Graph where:
 * Set U = Donors
 * Set V = Recipients
 * Edges represent medically compatible pairings with compatibility score weights.
 */
BipartiteGraph* create_bipartite_graph(Donor* donors, int num_donors,
                                       Recipient* recipients, int num_recipients,
                                       long* comparisons_count) {
    if (!donors || !recipients) return NULL;

    BipartiteGraph* g = (BipartiteGraph*)malloc(sizeof(BipartiteGraph));
    if (!g) return NULL;

    g->num_donors = num_donors;
    g->num_recipients = num_recipients;
    g->donors = donors;
    g->recipients = recipients;
    g->total_edges = 0;

    g->donor_vertices = (DonorVertex*)malloc(sizeof(DonorVertex) * num_donors);
    if (!g->donor_vertices) {
        free(g);
        return NULL;
    }

    for (int i = 0; i < num_donors; i++) {
        g->donor_vertices[i].donor_idx = i;
        g->donor_vertices[i].degree = 0;
        g->donor_vertices[i].head = NULL;
    }

    /* Build compatibility edges */
    for (int i = 0; i < num_donors; i++) {
        for (int j = 0; j < num_recipients; j++) {
            if (comparisons_count) {
                (*comparisons_count)++;
            }

            char explanation[MAX_EXPLANATION_LEN];
            double score = calculate_compatibility_score(&donors[i], &recipients[j], explanation, sizeof(explanation));

            if (score > 0.0) {
                /* Allocate new edge */
                AdjEdge* edge = (AdjEdge*)malloc(sizeof(AdjEdge));
                if (edge) {
                    edge->recipient_idx = j;
                    edge->compatibility_score = score;
                    strncpy(edge->explanation, explanation, sizeof(edge->explanation) - 1);
                    edge->explanation[sizeof(edge->explanation) - 1] = '\0';

                    /* Insert edge in descending order of compatibility score */
                    AdjEdge** curr = &(g->donor_vertices[i].head);
                    while (*curr && (*curr)->compatibility_score >= score) {
                        curr = &((*curr)->next);
                    }
                    edge->next = *curr;
                    *curr = edge;

                    g->donor_vertices[i].degree++;
                    g->total_edges++;
                }
            }
        }
    }

    return g;
}

void free_bipartite_graph(BipartiteGraph* g) {
    if (!g) return;
    if (g->donor_vertices) {
        for (int i = 0; i < g->num_donors; i++) {
            AdjEdge* edge = g->donor_vertices[i].head;
            while (edge) {
                AdjEdge* next = edge->next;
                free(edge);
                edge = next;
            }
        }
        free(g->donor_vertices);
    }
    free(g);
}

/**
 * Breadth-First Search (BFS) Traversal of Bipartite Compatibility Graph
 * Explores vertices level-by-level using the custom Queue.
 */
void graph_bfs_traversal(const BipartiteGraph* g, int start_donor_idx,
                         char* log_buffer, size_t buf_size,
                         int* visited_donors_count, int* visited_recipients_count) {
    if (!g || !log_buffer || buf_size == 0) return;

    int* visited_donors = (int*)calloc(g->num_donors, sizeof(int));
    int* visited_recipients = (int*)calloc(g->num_recipients, sizeof(int));
    if (!visited_donors || !visited_recipients) {
        if (visited_donors) free(visited_donors);
        if (visited_recipients) free(visited_recipients);
        return;
    }

    Queue* q = queue_create();
    size_t offset = 0;
    int step = 1;
    int d_count = 0;
    int r_count = 0;

    offset += snprintf(log_buffer + offset, buf_size - offset,
                       "=== Breadth-First Search (BFS) Traversal Log ===\n");

    /* Determine start vertex */
    int start_d = (start_donor_idx >= 0 && start_donor_idx < g->num_donors) ? start_donor_idx : 0;

    /* Enqueue start donor */
    enqueue(q, start_d, 0, 0);
    visited_donors[start_d] = 1;
    d_count++;

    offset += snprintf(log_buffer + offset, buf_size - offset,
                       "[Step %d | Level 0] Enqueued start Donor vertex: %s (%s, %s, %s)\n",
                       step++, g->donors[start_d].id, g->donors[start_d].name,
                       g->donors[start_d].organ_type, g->donors[start_d].blood_group);

    while (!queue_is_empty(q) && offset < buf_size - 256) {
        int u, is_recip, level;
        dequeue(q, &u, &is_recip, &level);

        if (!is_recip) {
            /* Dequeued a donor vertex: explore adjacent compatible recipients */
            offset += snprintf(log_buffer + offset, buf_size - offset,
                               "[Step %d | Level %d] Dequeued Donor %s. Degree = %d compatible recipients.\n",
                               step++, level, g->donors[u].id, g->donor_vertices[u].degree);

            AdjEdge* edge = g->donor_vertices[u].head;
            while (edge && offset < buf_size - 256) {
                int r_idx = edge->recipient_idx;
                if (!visited_recipients[r_idx]) {
                    visited_recipients[r_idx] = 1;
                    r_count++;
                    enqueue(q, r_idx, 1, level + 1);

                    offset += snprintf(log_buffer + offset, buf_size - offset,
                                       "   --> Discovered compatible Recipient %s (%s, %s, Score: %.1f) [Level %d]\n",
                                       g->recipients[r_idx].id, g->recipients[r_idx].name,
                                       g->recipients[r_idx].required_organ, edge->compatibility_score, level + 1);
                }
                edge = edge->next;
            }
        } else {
            /* Dequeued a recipient vertex: can traverse backwards to other donors who share this recipient */
            offset += snprintf(log_buffer + offset, buf_size - offset,
                               "[Step %d | Level %d] Reached Recipient %s (%s, Urgency: %s, Waiting: %d days).\n",
                               step++, level, g->recipients[u].id, g->recipients[u].name,
                               urgency_to_string(g->recipients[u].urgency_level), g->recipients[u].waiting_time);

            /* Check other donors connected to this recipient to find multi-donor components */
            for (int di = 0; di < g->num_donors; di++) {
                if (!visited_donors[di]) {
                    AdjEdge* e = g->donor_vertices[di].head;
                    while (e) {
                        if (e->recipient_idx == u) {
                            visited_donors[di] = 1;
                            d_count++;
                            enqueue(q, di, 0, level + 1);
                            offset += snprintf(log_buffer + offset, buf_size - offset,
                                               "   <-- Alternate Donor candidate found: %s (%s, Score: %.1f) [Level %d]\n",
                                               g->donors[di].id, g->donors[di].name, e->compatibility_score, level + 1);
                            break;
                        }
                        e = e->next;
                    }
                }
            }
        }
    }

    offset += snprintf(log_buffer + offset, buf_size - offset,
                       "=== BFS Completed: %d Donors, %d Recipients traversed in connected component ===\n",
                       d_count, r_count);

    if (visited_donors_count) *visited_donors_count = d_count;
    if (visited_recipients_count) *visited_recipients_count = r_count;

    queue_free(q);
    free(visited_donors);
    free(visited_recipients);
}

/* Recursive DFS Helper */
static void dfs_visit(const BipartiteGraph* g, int u, int is_recip, int depth,
                      int* visited_donors, int* visited_recipients,
                      int* d_count, int* r_count, int* step,
                      char* log_buffer, size_t buf_size, size_t* offset) {
    if (*offset >= buf_size - 256) return;

    if (!is_recip) {
        visited_donors[u] = 1;
        (*d_count)++;

        *offset += snprintf(log_buffer + *offset, buf_size - *offset,
                            "[Step %d | Depth %d] DFS visiting Donor %s (%s, %s)\n",
                            (*step)++, depth, g->donors[u].id, g->donors[u].name, g->donors[u].organ_type);

        AdjEdge* edge = g->donor_vertices[u].head;
        while (edge && *offset < buf_size - 256) {
            int r_idx = edge->recipient_idx;
            if (!visited_recipients[r_idx]) {
                *offset += snprintf(log_buffer + *offset, buf_size - *offset,
                                    "   |-- Tree Edge to Recipient %s (Score: %.1f)\n",
                                    g->recipients[r_idx].id, edge->compatibility_score);

                dfs_visit(g, r_idx, 1, depth + 1, visited_donors, visited_recipients,
                          d_count, r_count, step, log_buffer, buf_size, offset);
            } else {
                *offset += snprintf(log_buffer + *offset, buf_size - *offset,
                                    "   |-- Cross/Back Edge to already visited Recipient %s\n",
                                    g->recipients[r_idx].id);
            }
            edge = edge->next;
        }

        *offset += snprintf(log_buffer + *offset, buf_size - *offset,
                            "[Backtrack] Finished Donor %s at depth %d\n", g->donors[u].id, depth);
    } else {
        visited_recipients[u] = 1;
        (*r_count)++;

        *offset += snprintf(log_buffer + *offset, buf_size - *offset,
                            "[Step %d | Depth %d] DFS visiting Recipient %s (%s, %s)\n",
                            (*step)++, depth, g->recipients[u].id, g->recipients[u].name,
                            urgency_to_string(g->recipients[u].urgency_level));

        /* Deep dive into alternate donors sharing this recipient */
        for (int di = 0; di < g->num_donors && *offset < buf_size - 256; di++) {
            if (!visited_donors[di]) {
                AdjEdge* e = g->donor_vertices[di].head;
                while (e) {
                    if (e->recipient_idx == u) {
                        *offset += snprintf(log_buffer + *offset, buf_size - *offset,
                                            "   |-- Branching to alternate Donor %s (Score: %.1f)\n",
                                            g->donors[di].id, e->compatibility_score);
                        dfs_visit(g, di, 0, depth + 1, visited_donors, visited_recipients,
                                  d_count, r_count, step, log_buffer, buf_size, offset);
                        break;
                    }
                    e = e->next;
                }
            }
        }

        *offset += snprintf(log_buffer + *offset, buf_size - *offset,
                            "[Backtrack] Finished Recipient %s at depth %d\n", g->recipients[u].id, depth);
    }
}

/**
 * Depth-First Search (DFS) Traversal of Bipartite Compatibility Graph
 */
void graph_dfs_traversal(const BipartiteGraph* g, int start_donor_idx,
                         char* log_buffer, size_t buf_size,
                         int* visited_donors_count, int* visited_recipients_count) {
    if (!g || !log_buffer || buf_size == 0) return;

    int* visited_donors = (int*)calloc(g->num_donors, sizeof(int));
    int* visited_recipients = (int*)calloc(g->num_recipients, sizeof(int));
    if (!visited_donors || !visited_recipients) {
        if (visited_donors) free(visited_donors);
        if (visited_recipients) free(visited_recipients);
        return;
    }

    size_t offset = 0;
    int step = 1;
    int d_count = 0;
    int r_count = 0;

    offset += snprintf(log_buffer + offset, buf_size - offset,
                       "=== Depth-First Search (DFS) Traversal Log ===\n");

    int start_d = (start_donor_idx >= 0 && start_donor_idx < g->num_donors) ? start_donor_idx : 0;
    dfs_visit(g, start_d, 0, 0, visited_donors, visited_recipients,
              &d_count, &r_count, &step, log_buffer, buf_size, &offset);

    offset += snprintf(log_buffer + offset, buf_size - offset,
                       "=== DFS Completed: %d Donors, %d Recipients traversed ===\n",
                       d_count, r_count);

    if (visited_donors_count) *visited_donors_count = d_count;
    if (visited_recipients_count) *visited_recipients_count = r_count;

    free(visited_donors);
    free(visited_recipients);
}

/**
 * Serializes the bipartite graph (nodes and edges) into JSON for frontend visualizer
 */
void graph_to_json(const BipartiteGraph* g, char* json_buf, size_t buf_size) {
    if (!g || !json_buf || buf_size == 0) return;

    size_t offset = 0;
    offset += snprintf(json_buf + offset, buf_size - offset, "{\"nodes\":[");

    /* Add donor nodes */
    for (int i = 0; i < g->num_donors; i++) {
        char esc_name[MAX_NAME_LEN * 2];
        char esc_loc[MAX_LOCATION_LEN * 2];
        json_escape_string(g->donors[i].name, esc_name, sizeof(esc_name));
        json_escape_string(g->donors[i].location, esc_loc, sizeof(esc_loc));

        offset += snprintf(json_buf + offset, buf_size - offset,
                           "%s{\"id\":\"%s\",\"label\":\"%s\",\"type\":\"donor\",\"group\":\"donor\","
                           "\"blood\":\"%s\",\"organ\":\"%s\",\"age\":%d,\"location\":\"%s\"}",
                           (i > 0 ? "," : ""),
                           g->donors[i].id, esc_name,
                           g->donors[i].blood_group, g->donors[i].organ_type,
                           g->donors[i].age, esc_loc);
        if (offset >= buf_size - 1024) break;
    }

    /* Add recipient nodes */
    for (int j = 0; j < g->num_recipients; j++) {
        char esc_name[MAX_NAME_LEN * 2];
        char esc_loc[MAX_LOCATION_LEN * 2];
        json_escape_string(g->recipients[j].name, esc_name, sizeof(esc_name));
        json_escape_string(g->recipients[j].location, esc_loc, sizeof(esc_loc));

        offset += snprintf(json_buf + offset, buf_size - offset,
                           ",{\"id\":\"%s\",\"label\":\"%s\",\"type\":\"recipient\",\"group\":\"recipient\","
                           "\"blood\":\"%s\",\"organ\":\"%s\",\"urgency\":%d,\"urgency_str\":\"%s\","
                           "\"waiting\":%d,\"age\":%d,\"location\":\"%s\"}",
                           g->recipients[j].id, esc_name,
                           g->recipients[j].blood_group, g->recipients[j].required_organ,
                           g->recipients[j].urgency_level, urgency_to_string(g->recipients[j].urgency_level),
                           g->recipients[j].waiting_time, g->recipients[j].age, esc_loc);
        if (offset >= buf_size - 1024) break;
    }

    offset += snprintf(json_buf + offset, buf_size - offset, "],\"edges\":[");

    int edge_count = 0;
    for (int i = 0; i < g->num_donors; i++) {
        AdjEdge* e = g->donor_vertices[i].head;
        while (e) {
            char esc_exp[MAX_EXPLANATION_LEN * 2];
            json_escape_string(e->explanation, esc_exp, sizeof(esc_exp));

            offset += snprintf(json_buf + offset, buf_size - offset,
                               "%s{\"source\":\"%s\",\"target\":\"%s\",\"weight\":%.2f,\"explanation\":\"%s\"}",
                               (edge_count > 0 ? "," : ""),
                               g->donors[i].id, g->recipients[e->recipient_idx].id,
                               e->compatibility_score, esc_exp);
            edge_count++;
            if (offset >= buf_size - 512) break;
            e = e->next;
        }
        if (offset >= buf_size - 512) break;
    }

    snprintf(json_buf + offset, buf_size - offset, "],\"total_edges\":%d}", g->total_edges);
}
