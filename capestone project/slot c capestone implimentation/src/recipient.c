/**
 * recipient.c - Implementation of Recipient Management (Module 2)
 */

#include "recipient.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void generate_unique_recipient_id(const Recipient* recipients, int count, char* out_id, size_t max_len) {
    int max_num = 2000;
    for (int i = 0; i < count; i++) {
        if (strncmp(recipients[i].id, "REC-", 4) == 0) {
            int num = atoi(recipients[i].id + 4);
            if (num > max_num) {
                max_num = num;
            }
        }
    }
    snprintf(out_id, max_len, "REC-%04d", max_num + 1);
}

int recipient_find_by_id(const Recipient* recipients, int count, const char* id) {
    if (!recipients || !id) return -1;
    for (int i = 0; i < count; i++) {
        if (string_equals_ignore_case(recipients[i].id, id)) {
            return i;
        }
    }
    return -1;
}

int recipient_add(Recipient** recipients, int* count, const Recipient* new_recipient) {
    if (!recipients || !count || !new_recipient) return 0;
    if (*count >= MAX_RECORDS) return 0;

    if (*recipients == NULL) {
        *recipients = (Recipient*)malloc(sizeof(Recipient) * MAX_RECORDS);
        if (!*recipients) return 0;
        *count = 0;
    }

    if (strlen(new_recipient->id) > 0 && recipient_find_by_id(*recipients, *count, new_recipient->id) != -1) {
        return 0; /* Duplicate ID error */
    }

    Recipient* r = &((*recipients)[*count]);
    memcpy(r, new_recipient, sizeof(Recipient));

    if (strlen(r->id) == 0) {
        generate_unique_recipient_id(*recipients, *count, r->id, sizeof(r->id));
    }

    if (r->urgency_level < 1 || r->urgency_level > 3) {
        r->urgency_level = 3;
    }
    if (strlen(r->status) == 0) {
        strncpy(r->status, "Waiting", sizeof(r->status) - 1);
    }
    if (strlen(r->reg_date) == 0) {
        strncpy(r->reg_date, "2026-03-01", sizeof(r->reg_date) - 1);
    }

    (*count)++;
    return 1;
}

int recipient_update(Recipient* recipients, int count, const char* id, const Recipient* updated_recipient) {
    if (!recipients || !id || !updated_recipient) return 0;
    int idx = recipient_find_by_id(recipients, count, id);
    if (idx == -1) return 0;

    Recipient* r = &recipients[idx];
    if (strlen(updated_recipient->name) > 0) strncpy(r->name, updated_recipient->name, sizeof(r->name) - 1);
    if (updated_recipient->age > 0) r->age = updated_recipient->age;
    if (strlen(updated_recipient->gender) > 0) strncpy(r->gender, updated_recipient->gender, sizeof(r->gender) - 1);
    if (strlen(updated_recipient->blood_group) > 0) strncpy(r->blood_group, updated_recipient->blood_group, sizeof(r->blood_group) - 1);
    if (strlen(updated_recipient->required_organ) > 0) strncpy(r->required_organ, updated_recipient->required_organ, sizeof(r->required_organ) - 1);
    if (updated_recipient->urgency_level >= 1 && updated_recipient->urgency_level <= 3) r->urgency_level = updated_recipient->urgency_level;
    if (strlen(updated_recipient->location) > 0) strncpy(r->location, updated_recipient->location, sizeof(r->location) - 1);
    if (updated_recipient->waiting_time >= 0) r->waiting_time = updated_recipient->waiting_time;
    if (strlen(updated_recipient->hla_type) > 0) strncpy(r->hla_type, updated_recipient->hla_type, sizeof(r->hla_type) - 1);
    if (strlen(updated_recipient->status) > 0) strncpy(r->status, updated_recipient->status, sizeof(r->status) - 1);

    return 1;
}

int recipient_delete(Recipient** recipients, int* count, const char* id) {
    if (!recipients || !*recipients || !count || !id) return 0;
    int idx = recipient_find_by_id(*recipients, *count, id);
    if (idx == -1) return 0;

    for (int i = idx; i < *count - 1; i++) {
        (*recipients)[i] = (*recipients)[i + 1];
    }
    (*count)--;
    return 1;
}

void single_recipient_to_json(const Recipient* r, char* json_buf, size_t buf_size) {
    if (!r || !json_buf || buf_size == 0) return;
    char esc_name[MAX_NAME_LEN * 2];
    char esc_loc[MAX_LOCATION_LEN * 2];
    char esc_hla[MAX_HLA_LEN * 2];

    json_escape_string(r->name, esc_name, sizeof(esc_name));
    json_escape_string(r->location, esc_loc, sizeof(esc_loc));
    json_escape_string(r->hla_type, esc_hla, sizeof(esc_hla));

    snprintf(json_buf, buf_size,
             "{\"id\":\"%s\",\"name\":\"%s\",\"age\":%d,\"gender\":\"%s\",\"blood_group\":\"%s\","
             "\"required_organ\":\"%s\",\"urgency_level\":%d,\"urgency_str\":\"%s\",\"location\":\"%s\","
             "\"waiting_time\":%d,\"hla_type\":\"%s\",\"status\":\"%s\",\"reg_date\":\"%s\"}",
             r->id, esc_name, r->age, r->gender, r->blood_group,
             r->required_organ, r->urgency_level, urgency_to_string(r->urgency_level),
             esc_loc, r->waiting_time, esc_hla, r->status, r->reg_date);
}

void recipients_to_json(const Recipient* recipients, int count, char* json_buf, size_t buf_size) {
    if (!json_buf || buf_size == 0) return;
    size_t offset = 0;
    offset += snprintf(json_buf + offset, buf_size - offset, "[");

    for (int i = 0; i < count; i++) {
        char item[512];
        single_recipient_to_json(&recipients[i], item, sizeof(item));
        offset += snprintf(json_buf + offset, buf_size - offset, "%s%s", (i > 0 ? "," : ""), item);
        if (offset >= buf_size - 64) break;
    }

    snprintf(json_buf + offset, buf_size - offset, "]");
}
