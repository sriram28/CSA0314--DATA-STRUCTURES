/**
 * donor.c - Implementation of Donor Management (Module 1)
 */

#include "donor.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void generate_unique_donor_id(const Donor* donors, int count, char* out_id, size_t max_len) {
    int max_num = 1000;
    for (int i = 0; i < count; i++) {
        if (strncmp(donors[i].id, "DON-", 4) == 0) {
            int num = atoi(donors[i].id + 4);
            if (num > max_num) {
                max_num = num;
            }
        }
    }
    snprintf(out_id, max_len, "DON-%04d", max_num + 1);
}

int donor_find_by_id(const Donor* donors, int count, const char* id) {
    if (!donors || !id) return -1;
    for (int i = 0; i < count; i++) {
        if (string_equals_ignore_case(donors[i].id, id)) {
            return i;
        }
    }
    return -1;
}

int donor_add(Donor** donors, int* count, const Donor* new_donor) {
    if (!donors || !count || !new_donor) return 0;
    if (*count >= MAX_RECORDS) return 0;

    /* If list is NULL, allocate initial buffer */
    if (*donors == NULL) {
        *donors = (Donor*)malloc(sizeof(Donor) * MAX_RECORDS);
        if (!*donors) return 0;
        *count = 0;
    }

    /* Check if ID already exists */
    if (strlen(new_donor->id) > 0 && donor_find_by_id(*donors, *count, new_donor->id) != -1) {
        return 0; /* Duplicate ID error */
    }

    Donor* d = &((*donors)[*count]);
    memcpy(d, new_donor, sizeof(Donor));

    /* Auto-generate ID if empty */
    if (strlen(d->id) == 0) {
        generate_unique_donor_id(*donors, *count, d->id, sizeof(d->id));
    }

    if (strlen(d->status) == 0) {
        strncpy(d->status, "Available", sizeof(d->status) - 1);
    }
    if (strlen(d->reg_date) == 0) {
        strncpy(d->reg_date, "2026-03-01", sizeof(d->reg_date) - 1);
    }

    (*count)++;
    return 1;
}

int donor_update(Donor* donors, int count, const char* id, const Donor* updated_donor) {
    if (!donors || !id || !updated_donor) return 0;
    int idx = donor_find_by_id(donors, count, id);
    if (idx == -1) return 0;

    Donor* d = &donors[idx];
    if (strlen(updated_donor->name) > 0) strncpy(d->name, updated_donor->name, sizeof(d->name) - 1);
    if (updated_donor->age > 0) d->age = updated_donor->age;
    if (strlen(updated_donor->gender) > 0) strncpy(d->gender, updated_donor->gender, sizeof(d->gender) - 1);
    if (strlen(updated_donor->blood_group) > 0) strncpy(d->blood_group, updated_donor->blood_group, sizeof(d->blood_group) - 1);
    if (strlen(updated_donor->organ_type) > 0) strncpy(d->organ_type, updated_donor->organ_type, sizeof(d->organ_type) - 1);
    if (strlen(updated_donor->location) > 0) strncpy(d->location, updated_donor->location, sizeof(d->location) - 1);
    if (strlen(updated_donor->hla_type) > 0) strncpy(d->hla_type, updated_donor->hla_type, sizeof(d->hla_type) - 1);
    if (strlen(updated_donor->status) > 0) strncpy(d->status, updated_donor->status, sizeof(d->status) - 1);

    return 1;
}

int donor_delete(Donor** donors, int* count, const char* id) {
    if (!donors || !*donors || !count || !id) return 0;
    int idx = donor_find_by_id(*donors, *count, id);
    if (idx == -1) return 0;

    /* Shift remaining elements left */
    for (int i = idx; i < *count - 1; i++) {
        (*donors)[i] = (*donors)[i + 1];
    }
    (*count)--;
    return 1;
}

void single_donor_to_json(const Donor* d, char* json_buf, size_t buf_size) {
    if (!d || !json_buf || buf_size == 0) return;
    char esc_name[MAX_NAME_LEN * 2];
    char esc_loc[MAX_LOCATION_LEN * 2];
    char esc_hla[MAX_HLA_LEN * 2];

    json_escape_string(d->name, esc_name, sizeof(esc_name));
    json_escape_string(d->location, esc_loc, sizeof(esc_loc));
    json_escape_string(d->hla_type, esc_hla, sizeof(esc_hla));

    snprintf(json_buf, buf_size,
             "{\"id\":\"%s\",\"name\":\"%s\",\"age\":%d,\"gender\":\"%s\",\"blood_group\":\"%s\","
             "\"organ_type\":\"%s\",\"location\":\"%s\",\"hla_type\":\"%s\",\"status\":\"%s\",\"reg_date\":\"%s\"}",
             d->id, esc_name, d->age, d->gender, d->blood_group,
             d->organ_type, esc_loc, esc_hla, d->status, d->reg_date);
}

void donors_to_json(const Donor* donors, int count, char* json_buf, size_t buf_size) {
    if (!json_buf || buf_size == 0) return;
    size_t offset = 0;
    offset += snprintf(json_buf + offset, buf_size - offset, "[");

    for (int i = 0; i < count; i++) {
        char item[512];
        single_donor_to_json(&donors[i], item, sizeof(item));
        offset += snprintf(json_buf + offset, buf_size - offset, "%s%s", (i > 0 ? "," : ""), item);
        if (offset >= buf_size - 64) break;
    }

    snprintf(json_buf + offset, buf_size - offset, "]");
}
