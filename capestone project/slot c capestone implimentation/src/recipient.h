/**
 * recipient.h - Recipient management module functions and operations
 * Module 2: Recipient Management
 */

#ifndef RECIPIENT_H
#define RECIPIENT_H

#include "../include/common.h"
#include <stddef.h>

/* Recipient CRUD Operations */
int recipient_add(Recipient** recipients, int* count, const Recipient* new_recipient);
int recipient_update(Recipient* recipients, int count, const char* id, const Recipient* updated_recipient);
int recipient_delete(Recipient** recipients, int* count, const char* id);
int recipient_find_by_id(const Recipient* recipients, int count, const char* id);
void generate_unique_recipient_id(const Recipient* recipients, int count, char* out_id, size_t max_len);

/* Recipient JSON Serialization */
void recipients_to_json(const Recipient* recipients, int count, char* json_buf, size_t buf_size);
void single_recipient_to_json(const Recipient* r, char* json_buf, size_t buf_size);

#endif /* RECIPIENT_H */
