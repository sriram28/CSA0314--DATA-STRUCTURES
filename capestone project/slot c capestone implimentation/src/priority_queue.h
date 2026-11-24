/**
 * priority_queue.h - Binary Heap Priority Queue for Organ Recipient Waitlist
 * Module 6: Medically Authorized Urgency & Allocation Binary Max-Heap
 */

#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include "../include/common.h"

#define MAX_HEAP_CAPACITY 256

/* Priority Queue Element */
typedef struct {
    Recipient recipient;
    double priority_score;
    char calculation_rationale[256];
    long insertion_order;
} HeapNode;

/* Binary Heap Priority Queue Structure */
typedef struct {
    HeapNode* nodes;
    int size;
    int capacity;
    long total_operations;
    long heapify_swaps;
} PriorityQueue;

/* Core Heap Function Prototypes */
PriorityQueue* pq_create(int capacity);
void pq_free(PriorityQueue* pq);

/* Compute composite medical priority score based on urgency tier, wait days, and pediatric bonus */
double pq_compute_priority_score(const Recipient* r, char* out_rationale, size_t max_len);

/* Heap Insertion (Percolate Up) */
int pq_insert(PriorityQueue* pq, const Recipient* r, char* out_trace, size_t trace_len);

/* Heap Extraction (Extract Max / Root with Percolate Down) */
int pq_extract_max(PriorityQueue* pq, Recipient* out_r, double* out_score, char* out_trace, size_t trace_len);

/* Heap Peek (Inspect Root without extraction) */
int pq_peek(const PriorityQueue* pq, Recipient* out_r, double* out_score);

/* Update Priority (Change Urgency Tier / Wait Time of existing recipient) */
int pq_update_priority(PriorityQueue* pq, const char* recipient_id, int new_urgency, int new_wait_time, char* out_trace, size_t trace_len);

/* Delete recipient from heap by ID */
int pq_delete_by_id(PriorityQueue* pq, const char* recipient_id, char* out_trace, size_t trace_len);

/* Build Heap from an array of recipients (Heapify all) */
void pq_build_from_recipients(PriorityQueue* pq, const Recipient* recipients, int count);

/* Export Heap state to JSON string */
void pq_to_json(const PriorityQueue* pq, char* json_buf, size_t max_len);

#endif /* PRIORITY_QUEUE_H */
