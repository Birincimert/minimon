// collector.h - Resource Monitoring Header
#ifndef COLLECTOR_H
#define COLLECTOR_H

#include <windows.h>
#include <stdint.h>

// CPU
float get_cpu_usage();

// RAM
void get_memory_info(uint64_t *total, uint64_t *available, uint64_t *swap_total, uint64_t *swap_free);

// Disk
void get_disk_info(); // (Geliştirilecek: Disk bilgilerini struct ile döndür)

// Network
void get_network_info(); // (Geliştirilecek: Ağ bilgilerini struct ile döndür)

// CPU detaylı
void get_cpu_times(double *user, double *system, double *idle);
int get_cpu_core_count();
void get_cpu_per_core_usage(double *core_usages, int core_count);

// Process info struct
typedef struct {
    DWORD pid;
    char nameA[MAX_PATH];
    double cpu_usage; // Yüzde
    SIZE_T ram_usage; // Byte
    char status[32];  // "Running", "Not Responding", ...
    ULONGLONG last_kernel_time;
    ULONGLONG last_user_time;
} ProcessInfo;

// Process list collector
int get_process_info_list(ProcessInfo *list, int max_count);

// RAM swap detay
void get_swap_usage(uint64_t *used, uint64_t *total);

// Disk I/O
void get_disk_io_stats();

// Network connection count
void get_network_connection_count();

#endif // COLLECTOR_H 