/**
 * csv_io.c - Implementation of CSV file loading, parsing, and saving
 */

#include "csv_io.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CSV_LINE_MAX 1024

/* Helper to parse a single CSV line into tokens, respecting quotes if any */
static int parse_csv_line(char* line, char* fields[], int max_fields) {
    int count = 0;
    char* ptr = line;
    int in_quotes = 0;
    char* start = ptr;

    while (*ptr && count < max_fields) {
        if (*ptr == '"') {
            in_quotes = !in_quotes;
        } else if (*ptr == ',' && !in_quotes) {
            *ptr = '\0';
            trim_whitespace(start);
            fields[count++] = start;
            start = ptr + 1;
        }
        ptr++;
    }
    if (count < max_fields) {
        trim_whitespace(start);
        fields[count++] = start;
    }
    return count;
}

int load_donors_from_csv(const char* filepath, Donor** donors_out, int* count_out) {
    if (!filepath || !donors_out || !count_out) return 0;

    FILE* fp = fopen(filepath, "r");
    if (!fp) {
        *donors_out = NULL;
        *count_out = 0;
        return 0;
    }

    Donor* list = (Donor*)malloc(sizeof(Donor) * MAX_RECORDS);
    if (!list) {
        fclose(fp);
        *donors_out = NULL;
        *count_out = 0;
        return 0;
    }

    char line[CSV_LINE_MAX];
    int count = 0;
    int line_num = 0;

    while (fgets(line, sizeof(line), fp)) {
        line_num++;
        /* Remove trailing newline / carriage return */
        char* nl = strpbrk(line, "\r\n");
        if (nl) *nl = '\0';

        trim_whitespace(line);
        if (strlen(line) == 0) continue;

        /* Skip header line */
        if (line_num == 1 && strstr(line, "id") && strstr(line, "name")) {
            continue;
        }

        char* fields[16];
        int num_fields = parse_csv_line(line, fields, 16);
        if (num_fields >= 7) {
            Donor* d = &list[count];
            memset(d, 0, sizeof(Donor));

            strncpy(d->id, fields[0], sizeof(d->id) - 1);
            strncpy(d->name, fields[1], sizeof(d->name) - 1);
            d->age = atoi(fields[2]);
            strncpy(d->gender, fields[3], sizeof(d->gender) - 1);
            strncpy(d->blood_group, fields[4], sizeof(d->blood_group) - 1);
            strncpy(d->organ_type, fields[5], sizeof(d->organ_type) - 1);
            strncpy(d->location, fields[6], sizeof(d->location) - 1);

            if (num_fields >= 8) strncpy(d->hla_type, fields[7], sizeof(d->hla_type) - 1);
            else strncpy(d->hla_type, "A2-B44-DR4", sizeof(d->hla_type) - 1);

            if (num_fields >= 9 && strlen(fields[8]) > 0) {
                strncpy(d->status, fields[8], sizeof(d->status) - 1);
            } else {
                strncpy(d->status, "Available", sizeof(d->status) - 1);
            }

            if (num_fields >= 10 && strlen(fields[9]) > 0) {
                strncpy(d->reg_date, fields[9], sizeof(d->reg_date) - 1);
            } else {
                strncpy(d->reg_date, "2026-01-01", sizeof(d->reg_date) - 1);
            }

            count++;
            if (count >= MAX_RECORDS) break;
        }
    }

    fclose(fp);
    *donors_out = list;
    *count_out = count;
    return 1;
}

int save_donors_to_csv(const char* filepath, const Donor* donors, int count) {
    if (!filepath || !donors) return 0;

    FILE* fp = fopen(filepath, "w");
    if (!fp) return 0;

    fprintf(fp, "id,name,age,gender,blood_group,organ_type,location,hla_type,status,reg_date\n");
    for (int i = 0; i < count; i++) {
        fprintf(fp, "%s,%s,%d,%s,%s,%s,%s,%s,%s,%s\n",
                donors[i].id,
                donors[i].name,
                donors[i].age,
                donors[i].gender,
                donors[i].blood_group,
                donors[i].organ_type,
                donors[i].location,
                donors[i].hla_type,
                donors[i].status,
                donors[i].reg_date);
    }

    fclose(fp);
    return 1;
}

int load_recipients_from_csv(const char* filepath, Recipient** recipients_out, int* count_out) {
    if (!filepath || !recipients_out || !count_out) return 0;

    FILE* fp = fopen(filepath, "r");
    if (!fp) {
        *recipients_out = NULL;
        *count_out = 0;
        return 0;
    }

    Recipient* list = (Recipient*)malloc(sizeof(Recipient) * MAX_RECORDS);
    if (!list) {
        fclose(fp);
        *recipients_out = NULL;
        *count_out = 0;
        return 0;
    }

    char line[CSV_LINE_MAX];
    int count = 0;
    int line_num = 0;

    while (fgets(line, sizeof(line), fp)) {
        line_num++;
        char* nl = strpbrk(line, "\r\n");
        if (nl) *nl = '\0';

        trim_whitespace(line);
        if (strlen(line) == 0) continue;

        if (line_num == 1 && strstr(line, "id") && strstr(line, "name")) {
            continue;
        }

        char* fields[16];
        int num_fields = parse_csv_line(line, fields, 16);
        if (num_fields >= 8) {
            Recipient* r = &list[count];
            memset(r, 0, sizeof(Recipient));

            strncpy(r->id, fields[0], sizeof(r->id) - 1);
            strncpy(r->name, fields[1], sizeof(r->name) - 1);
            r->age = atoi(fields[2]);
            strncpy(r->gender, fields[3], sizeof(r->gender) - 1);
            strncpy(r->blood_group, fields[4], sizeof(r->blood_group) - 1);
            strncpy(r->required_organ, fields[5], sizeof(r->required_organ) - 1);
            r->urgency_level = atoi(fields[6]);
            if (r->urgency_level < 1 || r->urgency_level > 3) r->urgency_level = 3;
            strncpy(r->location, fields[7], sizeof(r->location) - 1);

            if (num_fields >= 9) r->waiting_time = atoi(fields[8]);
            else r->waiting_time = 30;

            if (num_fields >= 10) strncpy(r->hla_type, fields[9], sizeof(r->hla_type) - 1);
            else strncpy(r->hla_type, "A2-B44-DR4", sizeof(r->hla_type) - 1);

            if (num_fields >= 11 && strlen(fields[10]) > 0) {
                strncpy(r->status, fields[10], sizeof(r->status) - 1);
            } else {
                strncpy(r->status, "Waiting", sizeof(r->status) - 1);
            }

            if (num_fields >= 12 && strlen(fields[11]) > 0) {
                strncpy(r->reg_date, fields[11], sizeof(r->reg_date) - 1);
            } else {
                strncpy(r->reg_date, "2026-01-01", sizeof(r->reg_date) - 1);
            }

            count++;
            if (count >= MAX_RECORDS) break;
        }
    }

    fclose(fp);
    *recipients_out = list;
    *count_out = count;
    return 1;
}

int save_recipients_to_csv(const char* filepath, const Recipient* recipients, int count) {
    if (!filepath || !recipients) return 0;

    FILE* fp = fopen(filepath, "w");
    if (!fp) return 0;

    fprintf(fp, "id,name,age,gender,blood_group,required_organ,urgency_level,location,waiting_time,hla_type,status,reg_date\n");
    for (int i = 0; i < count; i++) {
        fprintf(fp, "%s,%s,%d,%s,%s,%s,%d,%s,%d,%s,%s,%s\n",
                recipients[i].id,
                recipients[i].name,
                recipients[i].age,
                recipients[i].gender,
                recipients[i].blood_group,
                recipients[i].required_organ,
                recipients[i].urgency_level,
                recipients[i].location,
                recipients[i].waiting_time,
                recipients[i].hla_type,
                recipients[i].status,
                recipients[i].reg_date);
    }

    fclose(fp);
    return 1;
}

/* Copy sample data to active CSV data files */
int reset_data_to_sample(const char* donors_dest, const char* recipients_dest,
                         const char* donors_sample, const char* recipients_sample) {
    Donor* d_list = NULL;
    int d_count = 0;
    if (load_donors_from_csv(donors_sample, &d_list, &d_count)) {
        save_donors_to_csv(donors_dest, d_list, d_count);
        free(d_list);
    }

    Recipient* r_list = NULL;
    int r_count = 0;
    if (load_recipients_from_csv(recipients_sample, &r_list, &r_count)) {
        save_recipients_to_csv(recipients_dest, r_list, r_count);
        free(r_list);
    }

    return 1;
}
