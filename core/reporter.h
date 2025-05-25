#ifndef CORE_REPORTER_H
#define CORE_REPORTER_H

void reporter_generate_csv(const char *filename);
void reporter_generate_json(const char *filename);
void reporter_start_periodic(const char *filename, int interval);

#endif // CORE_REPORTER_H 