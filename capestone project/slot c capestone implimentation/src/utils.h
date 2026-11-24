/**
 * utils.h - Utility functions, medical compatibility rules, and high-precision timers
 */

#ifndef UTILS_H
#define UTILS_H

#include "../include/common.h"
#include <stddef.h>

/* String Utilities */
void trim_whitespace(char* str);
int string_equals_ignore_case(const char* s1, const char* s2);

/* Medical Compatibility Rules */
int is_blood_compatible(const char* donor_bg, const char* recipient_bg);
int calculate_hla_matches(const char* donor_hla, const char* recipient_hla);
double calculate_compatibility_score(const Donor* d, const Recipient* r, char* explanation_buf, size_t buf_size);

/* High-Precision Timer */
double get_time_microseconds(void);

/* Helper to convert urgency int to string */
const char* urgency_to_string(int urgency_level);

/* JSON String Escaping */
void json_escape_string(const char* src, char* dst, size_t max_dst);

#endif /* UTILS_H */
