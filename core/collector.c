// collector.c - Resource Monitoring
// TODO: Implement functions to collect CPU, RAM, Disk, Network, and Process info 

#include "collector.h"
#include <stdio.h>
#include <windows.h>
#include <iphlpapi.h>
#include <tlhelp32.h>
#include <psapi.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "psapi.lib")

// GetSystemTimes prototype for MinGW compatibility
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#ifndef HAVE_GETSYSTEMTIMES
#define HAVE_GETSYSTEMTIMES
WINBASEAPI BOOL WINAPI GetSystemTimes(
  LPFILETIME lpIdleTime,
  LPFILETIME lpKernelTime,
  LPFILETIME lpUserTime
);
#endif

// Helper: Calculate difference between FILETIME structs
static ULONGLONG filetime_diff(const FILETIME *a, const FILETIME *b) {
    ULONGLONG aa = (((ULONGLONG)a->dwHighDateTime) << 32) | a->dwLowDateTime;
    ULONGLONG bb = (((ULONGLONG)b->dwHighDateTime) << 32) | b->dwLowDateTime;
    return (aa > bb) ? (aa - bb) : (bb - aa);
}

// --- CPU ---
float get_cpu_usage() {
    FILETIME idleTime1, kernelTime1, userTime1;
    FILETIME idleTime2, kernelTime2, userTime2;

    GetSystemTimes(&idleTime1, &kernelTime1, &userTime1);
    Sleep(100); // 100 ms bekle
    GetSystemTimes(&idleTime2, &kernelTime2, &userTime2);

    ULONGLONG idle = filetime_diff(&idleTime2, &idleTime1);
    ULONGLONG kernel = filetime_diff(&kernelTime2, &kernelTime1);
    ULONGLONG user = filetime_diff(&userTime2, &userTime1);
    ULONGLONG total = kernel + user;

    if (total == 0) return 0.0f;
    float usage = 100.0f * (float)(total - idle) / (float)total;
    return usage;
}

// --- CPU Detaylı ---
void get_cpu_times(double *user, double *system, double *idle) {
    FILETIME idleTime, kernelTime, userTime;
    if (GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        ULONGLONG idle64 = (((ULONGLONG)idleTime.dwHighDateTime) << 32) | idleTime.dwLowDateTime;
        ULONGLONG kernel64 = (((ULONGLONG)kernelTime.dwHighDateTime) << 32) | kernelTime.dwLowDateTime;
        ULONGLONG user64 = (((ULONGLONG)userTime.dwHighDateTime) << 32) | userTime.dwLowDateTime;
        *idle = (double)idle64 / 10000000.0;   // saniye
        *system = ((double)kernel64 - (double)idle64) / 10000000.0; // kernel - idle
        *user = (double)user64 / 10000000.0;
    } else {
        *idle = *system = *user = 0.0;
    }
}

int get_cpu_core_count() {
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
}

void get_cpu_per_core_usage(double *core_usages, int core_count) {
    // Windows'ta çekirdek bazında CPU kullanımı için gelişmiş API gerekir (PDH, ETW vs.)
    // Burada stub olarak 0 döndürüyoruz.
    for (int i = 0; i < core_count; i++) core_usages[i] = 0.0;
}

// --- RAM ---
void get_memory_info(uint64_t *total, uint64_t *available, uint64_t *swap_total, uint64_t *swap_free) {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        *total = memInfo.ullTotalPhys;
        *available = memInfo.ullAvailPhys;
        *swap_total = memInfo.ullTotalPageFile;
        *swap_free = memInfo.ullAvailPageFile;
    } else {
        *total = *available = *swap_total = *swap_free = 0;
    }
}

// --- RAM Swap Detay ---
void get_swap_usage(uint64_t *used, uint64_t *total) {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        *total = memInfo.ullTotalPageFile;
        *used = memInfo.ullTotalPageFile - memInfo.ullAvailPageFile;
    } else {
        *total = *used = 0;
    }
}

// --- Disk ---
void get_disk_info() {
    DWORD drives = GetLogicalDrives();
    char rootPath[4] = "A:\\";
    printf("Diskler:\n");
    for (char d = 'A'; d <= 'Z'; d++) {
        if (drives & (1 << (d - 'A'))) {
            rootPath[0] = d;
            ULARGE_INTEGER freeBytes, totalBytes, totalFreeBytes;
            if (GetDiskFreeSpaceExA(rootPath, &freeBytes, &totalBytes, &totalFreeBytes)) {
                printf("  %c: Toplam: %.2f GB, Bos: %.2f GB\n", d,
                    (double)totalBytes.QuadPart / (1024*1024*1024),
                    (double)freeBytes.QuadPart / (1024*1024*1024));
            }
        }
    }
}

// --- Disk I/O ---
void get_disk_io_stats() {
    PERFORMANCE_INFORMATION perfInfo;
    perfInfo.cb = sizeof(perfInfo);
    if (GetPerformanceInfo(&perfInfo, sizeof(perfInfo))) {
        printf("Disk I/O: (RAM ve sistem bilgisi)\n");
        printf("ProcessCount: %lu, ThreadCount: %lu\n", perfInfo.ProcessCount, perfInfo.ThreadCount);
    } else {
        printf("Disk I/O bilgisi alınamadı.\n");
    }
}

// --- Network ---
void get_network_info() {
    // Ağ arayüzleri ve veri miktarlarını yazdır
    PMIB_IFTABLE pIfTable;
    DWORD dwSize = 0;
    DWORD dwRetVal = 0;

    pIfTable = (MIB_IFTABLE *) malloc(sizeof(MIB_IFTABLE));
    if (pIfTable == NULL) {
        printf("Memory allocation failed for GetIfTable\n");
        return;
    }

    dwSize = sizeof(MIB_IFTABLE);
    if (GetIfTable(pIfTable, &dwSize, FALSE) == ERROR_INSUFFICIENT_BUFFER) {
        free(pIfTable);
        pIfTable = (MIB_IFTABLE *) malloc(dwSize);
        if (pIfTable == NULL) {
            printf("Memory allocation failed for GetIfTable\n");
            return;
        }
    }

    if ((dwRetVal = GetIfTable(pIfTable, &dwSize, FALSE)) == NO_ERROR) {
        printf("Ag Arayuzleri:\n");
        for (unsigned int i = 0; i < pIfTable->dwNumEntries; i++) {
            printf("  %s: Gonderilen: %lu bytes, Alinan: %lu bytes\n",
                pIfTable->table[i].bDescr,
                pIfTable->table[i].dwOutOctets,
                pIfTable->table[i].dwInOctets);
        }
    } else {
        printf("GetIfTable failed with error: %lu\n", dwRetVal);
    }
    if (pIfTable)
        free(pIfTable);
}

// --- Network Connection Count ---
void get_network_connection_count() {
    PMIB_TCPTABLE pTcpTable;
    DWORD dwSize = 0;
    DWORD dwRetVal = 0;
    int connCount = 0;
    GetTcpTable(NULL, &dwSize, FALSE);
    pTcpTable = (MIB_TCPTABLE *) malloc(dwSize);
    if (pTcpTable == NULL) {
        printf("Memory allocation failed for GetTcpTable\n");
        return;
    }
    if ((dwRetVal = GetTcpTable(pTcpTable, &dwSize, FALSE)) == NO_ERROR) {
        connCount = pTcpTable->dwNumEntries;
        printf("Aktif TCP baglantisi: %d\n", connCount);
    } else {
        printf("GetTcpTable failed with error: %lu\n", dwRetVal);
    }
    if (pTcpTable) free(pTcpTable);
}

// --- Process ---
void get_process_list() {
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        printf("Process snapshot alınamadı!\n");
        return;
    }
    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (!Process32First(hProcessSnap, &pe32)) {
        CloseHandle(hProcessSnap);
        printf("Process32First başarısız!\n");
        return;
    }
    printf("Process Listesi:\n");
    do {
        printf("  PID: %u, Isim: %ls\n", pe32.th32ProcessID, pe32.szExeFile);
    } while (Process32Next(hProcessSnap, &pe32));
    CloseHandle(hProcessSnap);
}

int get_process_info_list(ProcessInfo *list, int max_count) {
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    int count = 0;
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        return 0;
    }
    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (!Process32First(hProcessSnap, &pe32)) {
        CloseHandle(hProcessSnap);
        return 0;
    }
    do {
        if (count >= max_count) break;
        list[count].pid = pe32.th32ProcessID;
        strncpy(list[count].nameA, pe32.szExeFile, MAX_PATH);
        list[count].nameA[MAX_PATH-1] = '\0';
        
        // Sistem süreçleri için özel kontrol
        if (pe32.th32ProcessID <= 4 || strcmp(pe32.szExeFile, "smss.exe") == 0) {
            list[count].ram_usage = 0;
            strcpy(list[count].status, "System");
            list[count].last_kernel_time = 0;
            list[count].last_user_time = 0;
            count++;
            continue;
        }
        
        // Normal süreçler için handle aç
        HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
        if (hProc) {
            PROCESS_MEMORY_COUNTERS_EX pmc;
            if (GetProcessMemoryInfo(hProc, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
                list[count].ram_usage = pmc.WorkingSetSize;
            } else {
                list[count].ram_usage = 0;
            }
            
            // Process durumunu kontrol et
            DWORD exitCode;
            if (GetExitCodeProcess(hProc, &exitCode)) {
                if (exitCode == STILL_ACTIVE) {
                    strcpy(list[count].status, "Running");
                } else {
                    strcpy(list[count].status, "Terminated");
                }
            } else {
                strcpy(list[count].status, "Unknown");
            }
            
            // CPU zamanlarını al
            FILETIME create, exit, kernel, user;
            if (GetProcessTimes(hProc, &create, &exit, &kernel, &user)) {
                ULONGLONG k = (((ULONGLONG)kernel.dwHighDateTime) << 32) | kernel.dwLowDateTime;
                ULONGLONG u = (((ULONGLONG)user.dwHighDateTime) << 32) | user.dwLowDateTime;
                list[count].last_kernel_time = k;
                list[count].last_user_time = u;
            } else {
                list[count].last_kernel_time = 0;
                list[count].last_user_time = 0;
            }
            CloseHandle(hProc);
        } else {
            // Erişim izni yoksa
            list[count].ram_usage = 0;
            strcpy(list[count].status, "Inaccessible");
            list[count].last_kernel_time = 0;
            list[count].last_user_time = 0;
        }
        list[count].cpu_usage = 0.0;
        count++;
    } while (Process32Next(hProcessSnap, &pe32));
    CloseHandle(hProcessSnap);
    return count;
} 