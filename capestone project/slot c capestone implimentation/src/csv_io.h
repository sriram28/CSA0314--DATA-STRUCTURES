/**
 * csv_io.h - CSV file loading, saving, and persistence for donors and recipients
 */

#ifndef CSV_IO_H
#define CSV_IO_H

#include "../include/common.h"

/* Donors CSV Operations */
int load_donors_from_csv(const char* filepath, Donor** donors_out, int* count_out);
int save_donors_to_csv(const char* filepath, const Donor* donors, int count);

/* Recipients CSV Operations */
int load_recipients_from_csv(const char* filepath, Recipient** recipients_out, int* count_out);
int save_recipients_to_csv(const char* filepath, const Recipient* recipients, int count);

/* Reset Data Operations */
int reset_data_to_sample(const char* donors_dest, const char* recipients_dest,
                         const char* donors_sample, const char* recipients_sample);

#endif /* CSV_IO_H */
