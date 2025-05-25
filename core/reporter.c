// reporter.c - JSON/CSV Reporting
// TODO: Implement functions to report resource usage as JSON or CSV 

#include "reporter.h"
#include <stdio.h>
#include <stdint.h>
#include <windows.h>
#include <iphlpapi.h>
#include <psapi.h>
#include "../core/collector.h"

void reporter_generate_csv(const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        printf("CSV dosyası açılamadı: %s\n", filename);
        return;
    }

    // CPU bilgileri
    float cpu = get_cpu_usage();
    double user, system, idle;
    get_cpu_times(&user, &system, &idle);
    double total = user + system + idle;
    double user_pct = total > 0 ? (user / total) * 100.0 : 0.0;
    double system_pct = total > 0 ? (system / total) * 100.0 : 0.0;
    double idle_pct = total > 0 ? (idle / total) * 100.0 : 0.0;
    
    fprintf(f, "CPU Kullanımı,User%%,System%%,Idle%%\n");
    fprintf(f, "%.2f,%.2f,%.2f,%.2f\n", cpu, user_pct, system_pct, idle_pct);

    // RAM bilgileri
    uint64_t total_ram, available, swap_total, swap_free;
    get_memory_info(&total_ram, &available, &swap_total, &swap_free);
    uint64_t swap_used;
    get_swap_usage(&swap_used, &swap_total);
    
    fprintf(f, "Toplam RAM (GB),Kullanılan RAM (GB),Boş RAM (GB),Takas Kullanımı (GB),Takas Toplam (GB)\n");
    fprintf(f, "%.2f,%.2f,%.2f,%.2f,%.2f\n",
        (double)total_ram/(1024*1024*1024),
        (double)(total_ram-available)/(1024*1024*1024),
        (double)available/(1024*1024*1024),
        (double)swap_used/(1024*1024*1024),
        (double)swap_total/(1024*1024*1024));

    // Disk bilgileri
    DWORD drives = GetLogicalDrives();
    char rootPath[4] = "A:\\";
    fprintf(f, "Disk,Toplam (GB),Boş (GB)\n");
    for (char d = 'A'; d <= 'Z'; d++) {
        if (drives & (1 << (d - 'A'))) {
            rootPath[0] = d;
            ULARGE_INTEGER freeBytes, totalBytes, totalFreeBytes;
            if (GetDiskFreeSpaceExA(rootPath, &freeBytes, &totalBytes, &totalFreeBytes)) {
                fprintf(f, "%c,%.2f,%.2f\n", d,
                    (double)totalBytes.QuadPart / (1024*1024*1024),
                    (double)freeBytes.QuadPart / (1024*1024*1024));
            }
        }
    }

    // Ağ bilgileri
    PMIB_IFTABLE pIfTable;
    DWORD dwSize = 0;
    if (GetIfTable(NULL, &dwSize, FALSE) == ERROR_INSUFFICIENT_BUFFER) {
        pIfTable = (MIB_IFTABLE *)malloc(dwSize);
        if (pIfTable && GetIfTable(pIfTable, &dwSize, FALSE) == NO_ERROR) {
            fprintf(f, "Ağ Arayüzü,Gönderilen,Alınan\n");
            for (DWORD i = 0; i < pIfTable->dwNumEntries; i++) {
                fprintf(f, "%s,%lu,%lu\n",
                    pIfTable->table[i].bDescr,
                    pIfTable->table[i].dwOutOctets,
                    pIfTable->table[i].dwInOctets);
            }
        }
        free(pIfTable);
    }

    // Process bilgileri
    ProcessInfo plist[128];
    int pcount = get_process_info_list(plist, 128);
    fprintf(f, "Process,PID,CPU%%,RAM (MB),Status\n");
    for (int i = 0; i < pcount && i < 5; i++) {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, plist[i].pid);
        DWORD exitCode = 0;
        const char* status = "Unknown";
        if (hProcess) {
            if (GetExitCodeProcess(hProcess, &exitCode)) {
                status = (exitCode == STILL_ACTIVE) ? "Running" : "Terminated";
            }
            CloseHandle(hProcess);
        }
        fprintf(f, "%s,%u,%.1f,%.1f,%s\n", 
            plist[i].nameA, 
            plist[i].pid, 
            plist[i].cpu_usage,
            (double)plist[i].ram_usage / (1024*1024),
            status);
    }

    fclose(f);
    printf("CSV raporu oluşturuldu: %s\n", filename);
}

void reporter_generate_json(const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        printf("JSON dosyası açılamadı: %s\n", filename);
        return;
    }

    fprintf(f, "{\n");

    // CPU bilgileri
    float cpu = get_cpu_usage();
    double user, system, idle;
    get_cpu_times(&user, &system, &idle);
    double total = user + system + idle;
    double user_pct = total > 0 ? (user / total) * 100.0 : 0.0;
    double system_pct = total > 0 ? (system / total) * 100.0 : 0.0;
    double idle_pct = total > 0 ? (idle / total) * 100.0 : 0.0;
    
    fprintf(f, "  \"cpu\": {\n");
    fprintf(f, "    \"total\": %.2f,\n", cpu);
    fprintf(f, "    \"user\": %.2f,\n", user_pct);
    fprintf(f, "    \"system\": %.2f,\n", system_pct);
    fprintf(f, "    \"idle\": %.2f\n", idle_pct);
    fprintf(f, "  },\n");

    // RAM bilgileri
    uint64_t total_ram, available, swap_total, swap_free;
    get_memory_info(&total_ram, &available, &swap_total, &swap_free);
    uint64_t swap_used;
    get_swap_usage(&swap_used, &swap_total);
    
    fprintf(f, "  \"memory\": {\n");
    fprintf(f, "    \"total\": %.2f,\n", (double)total_ram/(1024*1024*1024));
    fprintf(f, "    \"used\": %.2f,\n", (double)(total_ram-available)/(1024*1024*1024));
    fprintf(f, "    \"free\": %.2f,\n", (double)available/(1024*1024*1024));
    fprintf(f, "    \"swap_used\": %.2f,\n", (double)swap_used/(1024*1024*1024));
    fprintf(f, "    \"swap_total\": %.2f\n", (double)swap_total/(1024*1024*1024));
    fprintf(f, "  },\n");

    // Disk bilgileri
    fprintf(f, "  \"disks\": [\n");
    DWORD drives = GetLogicalDrives();
    char rootPath[4] = "A:\\";
    int first_disk = 1;
    for (char d = 'A'; d <= 'Z'; d++) {
        if (drives & (1 << (d - 'A'))) {
            rootPath[0] = d;
            ULARGE_INTEGER freeBytes, totalBytes, totalFreeBytes;
            if (GetDiskFreeSpaceExA(rootPath, &freeBytes, &totalBytes, &totalFreeBytes)) {
                if (!first_disk) fprintf(f, ",\n");
                fprintf(f, "    {\n");
                fprintf(f, "      \"drive\": \"%c:\",\n", d);
                fprintf(f, "      \"total\": %.2f,\n", (double)totalBytes.QuadPart / (1024*1024*1024));
                fprintf(f, "      \"free\": %.2f\n", (double)freeBytes.QuadPart / (1024*1024*1024));
                fprintf(f, "    }");
                first_disk = 0;
            }
        }
    }
    fprintf(f, "\n  ],\n");

    // Ağ bilgileri
    fprintf(f, "  \"network\": [\n");
    PMIB_IFTABLE pIfTable;
    DWORD dwSize = 0;
    if (GetIfTable(NULL, &dwSize, FALSE) == ERROR_INSUFFICIENT_BUFFER) {
        pIfTable = (MIB_IFTABLE *)malloc(dwSize);
        if (pIfTable && GetIfTable(pIfTable, &dwSize, FALSE) == NO_ERROR) {
            for (DWORD i = 0; i < pIfTable->dwNumEntries; i++) {
                if (i > 0) fprintf(f, ",\n");
                fprintf(f, "    {\n");
                fprintf(f, "      \"interface\": \"%s\",\n", pIfTable->table[i].bDescr);
                fprintf(f, "      \"bytes_sent\": %lu,\n", pIfTable->table[i].dwOutOctets);
                fprintf(f, "      \"bytes_recv\": %lu\n", pIfTable->table[i].dwInOctets);
                fprintf(f, "    }");
            }
        }
        free(pIfTable);
    }
    fprintf(f, "\n  ],\n");

    // Process bilgileri
    fprintf(f, "  \"processes\": [\n");
    ProcessInfo plist[128];
    int pcount = get_process_info_list(plist, 128);
    for (int i = 0; i < pcount && i < 5; i++) {
        if (i > 0) fprintf(f, ",\n");
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, plist[i].pid);
        DWORD exitCode = 0;
        const char* status = "Unknown";
        if (hProcess) {
            if (GetExitCodeProcess(hProcess, &exitCode)) {
                status = (exitCode == STILL_ACTIVE) ? "Running" : "Terminated";
            }
            CloseHandle(hProcess);
        }
        fprintf(f, "    {\n");
        fprintf(f, "      \"name\": \"%s\",\n", plist[i].nameA);
        fprintf(f, "      \"pid\": %u,\n", plist[i].pid);
        fprintf(f, "      \"cpu_usage\": %.1f,\n", plist[i].cpu_usage);
        fprintf(f, "      \"ram_usage\": %.1f,\n", (double)plist[i].ram_usage / (1024*1024));
        fprintf(f, "      \"status\": \"%s\"\n", status);
        fprintf(f, "    }");
    }
    fprintf(f, "\n  ]\n");

    fprintf(f, "}\n");
    fclose(f);
    printf("JSON raporu oluşturuldu: %s\n", filename);
}

void reporter_start_periodic(const char *filename, int interval) {
    printf("Periyodik raporlama başlatıldı: %s (her %d saniye)\n", filename, interval);
    while (1) {
        reporter_generate_csv(filename);
        Sleep(interval * 1000);
    }
} 