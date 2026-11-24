/**
 * priority_queue.c - Binary Heap Priority Queue Implementation
 * Module 6: Priority Queue-Based Waiting List for Clinical Allocation
 */

#include "priority_queue.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static long global_insertion_counter = 0;

PriorityQueue* pq_create(int capacity) {
    if (capacity <= 0) capacity = MAX_HEAP_CAPACITY;
    PriorityQueue* pq = (PriorityQueue*)malloc(sizeof(PriorityQueue));
    if (!pq) return NULL;

    pq->nodes = (HeapNode*)malloc(sizeof(HeapNode) * capacity);
    if (!pq->nodes) {
        free(pq);
        return NULL;
    }

    pq->size = 0;
    pq->capacity = capacity;
    pq->total_operations = 0;
    pq->heapify_swaps = 0;
    return pq;
}

void pq_free(PriorityQueue* pq) {
    if (pq) {
        if (pq->nodes) free(pq->nodes);
        free(pq);
    }
}

double pq_compute_priority_score(const Recipient* r, char* out_rationale, size_t max_len) {
    if (!r) return 0.0;

    double urgency_pts = 100.0;
    const char* tier_name = "Standard (Tier 3)";

    if (r->urgency_level == URGENCY_CRITICAL || r->urgency_level == 1) {
        urgency_pts = 1000.0;
        tier_name = "Critical ICU (Tier 1)";
    } else if (r->urgency_level == URGENCY_URGENT || r->urgency_level == 2) {
        urgency_pts = 500.0;
        tier_name = "Urgent Deterioration (Tier 2)";
    } else if (r->urgency_level == 3) {
        urgency_pts = 200.0;
        tier_name = "Standard Maintenance (Tier 3)";
    } else {
        urgency_pts = 100.0;
        tier_name = "Low Urgency (Tier 4)";
    }

    /* Wait time factor: 1.5 points per registered day */
    double wait_pts = (double)r->waiting_time * 1.5;

    /* Pediatric allocation priority bonus: patients under 18 receive +150 points */
    double pediatric_bonus = (r->age < 18) ? 150.0 : 0.0;

    double total_score = urgency_pts + wait_pts + pediatric_bonus;

    if (out_rationale && max_len > 0) {
        snprintf(out_rationale, max_len,
                 "Urgency %s: %.0f pts + Waitlist (%d d): %.1f pts%s = Total: %.1f",
                 tier_name, urgency_pts, r->waiting_time, wait_pts,
                 (pediatric_bonus > 0.0) ? " + Pediatric Bonus: +150 pts" : "",
                 total_score);
    }

    return total_score;
}

static void swap_nodes(HeapNode* a, HeapNode* b) {
    HeapNode temp = *a;
    *a = *b;
    *b = temp;
}

static void sift_up(PriorityQueue* pq, int index, char* out_trace, size_t trace_len) {
    char step_buf[256];
    while (index > 0) {
        int parent = (index - 1) / 2;
        pq->total_operations++;

        if (pq->nodes[index].priority_score > pq->nodes[parent].priority_score) {
            pq->heapify_swaps++;
            if (out_trace && trace_len > strlen(out_trace) + 128) {
                snprintf(step_buf, sizeof(step_buf),
                         "   [Heapify-Up] Swap idx %d (%s, Score: %.1f) with parent idx %d (%s, Score: %.1f)\n",
                         index, pq->nodes[index].recipient.id, pq->nodes[index].priority_score,
                         parent, pq->nodes[parent].recipient.id, pq->nodes[parent].priority_score);
                strncat(out_trace, step_buf, trace_len - strlen(out_trace) - 1);
            }
            swap_nodes(&pq->nodes[index], &pq->nodes[parent]);
            index = parent;
        } else {
            break;
        }
    }
}

static void sift_down(PriorityQueue* pq, int index, char* out_trace, size_t trace_len) {
    char step_buf[256];
    while (index * 2 + 1 < pq->size) {
        int left = index * 2 + 1;
        int right = index * 2 + 2;
        int largest = index;

        pq->total_operations++;
        if (pq->nodes[left].priority_score > pq->nodes[largest].priority_score) {
            largest = left;
        }

        if (right < pq->size) {
            pq->total_operations++;
            if (pq->nodes[right].priority_score > pq->nodes[largest].priority_score) {
                largest = right;
            }
        }

        if (largest != index) {
            pq->heapify_swaps++;
            if (out_trace && trace_len > strlen(out_trace) + 128) {
                snprintf(step_buf, sizeof(step_buf),
                         "   [Heapify-Down] Sifted down idx %d (%s, Score: %.1f) with child idx %d (%s, Score: %.1f)\n",
                         index, pq->nodes[index].recipient.id, pq->nodes[index].priority_score,
                         largest, pq->nodes[largest].recipient.id, pq->nodes[largest].priority_score);
                strncat(out_trace, step_buf, trace_len - strlen(out_trace) - 1);
            }
            swap_nodes(&pq->nodes[index], &pq->nodes[largest]);
            index = largest;
        } else {
            break;
        }
    }
}

int pq_insert(PriorityQueue* pq, const Recipient* r, char* out_trace, size_t trace_len) {
    if (!pq || !r || pq->size >= pq->capacity) return 0;

    int idx = pq->size;
    pq->nodes[idx].recipient = *r;
    pq->nodes[idx].priority_score = pq_compute_priority_score(r, pq->nodes[idx].calculation_rationale,
                                                              sizeof(pq->nodes[idx].calculation_rationale));
    pq->nodes[idx].insertion_order = ++global_insertion_counter;
    pq->size++;
    pq->total_operations++;

    if (out_trace) {
        snprintf(out_trace, trace_len,
                 "=== Priority Queue Insertion Trace ===\n"
                 "[Operation] Inserted recipient %s (%s, Organ: %s) at heap index %d (Initial Score: %.1f)\n"
                 "Rationale: %s\n",
                 r->id, r->name, r->required_organ, idx, pq->nodes[idx].priority_score,
                 pq->nodes[idx].calculation_rationale);
    }

    sift_up(pq, idx, out_trace, trace_len);

    if (out_trace && trace_len > strlen(out_trace) + 64) {
        char done_buf[64];
        snprintf(done_buf, sizeof(done_buf), "[Complete] Heap property satisfied. New size = %d.\n", pq->size);
        strncat(out_trace, done_buf, trace_len - strlen(out_trace) - 1);
    }

    return 1;
}

int pq_extract_max(PriorityQueue* pq, Recipient* out_r, double* out_score, char* out_trace, size_t trace_len) {
    if (!pq || pq->size == 0) return 0;

    HeapNode root = pq->nodes[0];
    if (out_r) *out_r = root.recipient;
    if (out_score) *out_score = root.priority_score;

    if (out_trace) {
        snprintf(out_trace, trace_len,
                 "=== Priority Queue Extraction (Extract-Max) Trace ===\n"
                 "[Operation] Extracted highest priority root: %s (%s, Urgency Tier %d, Score: %.1f)\n"
                 "Rationale: %s\n"
                 "[Heapify] Moving last element %s from idx %d to root idx 0\n",
                 root.recipient.id, root.recipient.name, root.recipient.urgency_level, root.priority_score,
                 root.calculation_rationale,
                 pq->nodes[pq->size - 1].recipient.id, pq->size - 1);
    }

    pq->nodes[0] = pq->nodes[pq->size - 1];
    pq->size--;
    pq->total_operations++;

    if (pq->size > 0) {
        sift_down(pq, 0, out_trace, trace_len);
    }

    if (out_trace && trace_len > strlen(out_trace) + 64) {
        char done_buf[64];
        snprintf(done_buf, sizeof(done_buf), "[Complete] Extraction done. Remaining recipients in heap = %d.\n", pq->size);
        strncat(out_trace, done_buf, trace_len - strlen(out_trace) - 1);
    }

    return 1;
}

int pq_peek(const PriorityQueue* pq, Recipient* out_r, double* out_score) {
    if (!pq || pq->size == 0) return 0;
    if (out_r) *out_r = pq->nodes[0].recipient;
    if (out_score) *out_score = pq->nodes[0].priority_score;
    return 1;
}

int pq_update_priority(PriorityQueue* pq, const char* recipient_id, int new_urgency, int new_wait_time, char* out_trace, size_t trace_len) {
    if (!pq || !recipient_id) return 0;

    int target_idx = -1;
    for (int i = 0; i < pq->size; i++) {
        if (strcmp(pq->nodes[i].recipient.id, recipient_id) == 0) {
            target_idx = i;
            break;
        }
    }

    if (target_idx == -1) return 0;

    double old_score = pq->nodes[target_idx].priority_score;
    pq->nodes[target_idx].recipient.urgency_level = new_urgency;
    pq->nodes[target_idx].recipient.waiting_time = new_wait_time;
    double new_score = pq_compute_priority_score(&pq->nodes[target_idx].recipient,
                                                 pq->nodes[target_idx].calculation_rationale,
                                                 sizeof(pq->nodes[target_idx].calculation_rationale));
    pq->nodes[target_idx].priority_score = new_score;

    if (out_trace) {
        snprintf(out_trace, trace_len,
                 "=== Priority Queue Key Update Trace ===\n"
                 "[Operation] Updated %s at index %d: Urgency -> Tier %d, Wait Time -> %d days\n"
                 "Score Change: %.1f -> %.1f\n",
                 recipient_id, target_idx, new_urgency, new_wait_time, old_score, new_score);
    }

    if (new_score > old_score) {
        sift_up(pq, target_idx, out_trace, trace_len);
    } else {
        sift_down(pq, target_idx, out_trace, trace_len);
    }

    return 1;
}

int pq_delete_by_id(PriorityQueue* pq, const char* recipient_id, char* out_trace, size_t trace_len) {
    if (!pq || !recipient_id || pq->size == 0) return 0;

    int target_idx = -1;
    for (int i = 0; i < pq->size; i++) {
        if (strcmp(pq->nodes[i].recipient.id, recipient_id) == 0) {
            target_idx = i;
            break;
        }
    }

    if (target_idx == -1) return 0;

    HeapNode target = pq->nodes[target_idx];
    if (out_trace) {
        snprintf(out_trace, trace_len,
                 "=== Priority Queue Deletion Trace ===\n"
                 "[Operation] Deleting %s (%s) from heap index %d (Score: %.1f)\n",
                 target.recipient.id, target.recipient.name, target_idx, target.priority_score);
    }

    pq->nodes[target_idx] = pq->nodes[pq->size - 1];
    pq->size--;
    pq->total_operations++;

    if (target_idx < pq->size) {
        sift_up(pq, target_idx, out_trace, trace_len);
        sift_down(pq, target_idx, out_trace, trace_len);
    }

    return 1;
}

void pq_build_from_recipients(PriorityQueue* pq, const Recipient* recipients, int count) {
    if (!pq || !recipients) return;
    pq->size = 0;
    for (int i = 0; i < count && i < pq->capacity; i++) {
        pq_insert(pq, &recipients[i], NULL, 0);
    }
}

void pq_to_json(const PriorityQueue* pq, char* json_buf, size_t max_len) {
    if (!pq || !json_buf || max_len == 0) return;

    size_t offset = 0;
    offset += snprintf(json_buf + offset, max_len - offset,
                       "{\"heap_size\":%d,\"capacity\":%d,\"total_operations\":%ld,\"heapify_swaps\":%ld,\"elements\":[",
                       pq->size, pq->capacity, pq->total_operations, pq->heapify_swaps);

    for (int i = 0; i < pq->size; i++) {
        const HeapNode* node = &pq->nodes[i];
        int parent_idx = (i > 0) ? (i - 1) / 2 : -1;
        int left_idx = (i * 2 + 1 < pq->size) ? i * 2 + 1 : -1;
        int right_idx = (i * 2 + 2 < pq->size) ? i * 2 + 2 : -1;

        char esc_rat[512];
        json_escape_string(node->calculation_rationale, esc_rat, sizeof(esc_rat));

        offset += snprintf(json_buf + offset, max_len - offset,
                           "%s{\"index\":%d,\"parent\":%d,\"left_child\":%d,\"right_child\":%d,"
                           "\"priority_score\":%.2f,\"recipient_id\":\"%s\",\"name\":\"%s\","
                           "\"blood_group\":\"%s\",\"required_organ\":\"%s\",\"urgency_level\":%d,"
                           "\"waiting_time\":%d,\"age\":%d,\"location\":\"%s\","
                           "\"rationale\":\"%s\"}",
                           (i > 0) ? "," : "",
                           i, parent_idx, left_idx, right_idx,
                           node->priority_score,
                           node->recipient.id, node->recipient.name,
                           node->recipient.blood_group, node->recipient.required_organ,
                           node->recipient.urgency_level, node->recipient.waiting_time,
                           node->recipient.age, node->recipient.location,
                           esc_rat);

        if (offset + 1024 >= max_len) break;
    }

    snprintf(json_buf + offset, max_len - offset, "]}");
}
