/**
 * donor.h - Donor management module functions and operations
 * Module 1: Donor Management
 */

#ifndef DONOR_H
#define DONOR_H

#include "../include/common.h"
#include <stddef.h>

/* Donor CRUD Operations */
int donor_add(Donor** donors, int* count, const Donor* new_donor);
int donor_update(Donor* donors, int count, const char* id, const Donor* updated_donor);
int donor_delete(Donor** donors, int* count, const char* id);
int donor_find_by_id(const Donor* donors, int count, const char* id);
void generate_unique_donor_id(const Donor* donors, int count, char* out_id, size_t max_len);

/* Donor JSON Serialization */
void donors_to_json(const Donor* donors, int count, char* json_buf, size_t buf_size);
void single_donor_to_json(const Donor* d, char* json_buf, size_t buf_size);

#endif /* DONOR_H */
