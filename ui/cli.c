// cli.c - CLI User Interface
// TODO: Implement CLI UI (argument parsing, display, etc.)

#include "cli.h"
#include <stdio.h>
#include <stdint.h>
#include "../core/collector.h"
#include <windows.h>
#include <iphlpapi.h>
#include <psapi.h>
#include "../core/reporter.h"
#pragma comment(lib, "iphlpapi.lib")

#ifndef HAVE_GETSYSTEMTIMES
#define HAVE_GETSYSTEMTIMES
WINBASEAPI BOOL WINAPI GetSystemTimes(
    LPFILETIME lpIdleTime,
    LPFILETIME lpKernelTime,
    LPFILETIME lpUserTime);
#endif

void cli_print_cpu()
{
    float cpu = get_cpu_usage();
    double user, system, idle;
    get_cpu_times(&user, &system, &idle);
    double total = user + system + idle;
    double user_pct = total > 0 ? (user / total) * 100.0 : 0.0;
    double system_pct = total > 0 ? (system / total) * 100.0 : 0.0;
    double idle_pct = total > 0 ? (idle / total) * 100.0 : 0.0;
    printf("CPU Kullanımı: %4.1f%% (Kullanıcı: %4.1f%%, Sistem: %4.1f%%, Boşta: %4.1f%%)\n",
           cpu, user_pct, system_pct, idle_pct);
    fflush(stdout);
}

void cli_print_mem()
{
    uint64_t total = 0, available = 0, swap_total = 0, swap_free = 0;
    get_memory_info(&total, &available, &swap_total, &swap_free);
    uint64_t swap_used = 0;
    get_swap_usage(&swap_used, &swap_total);
    printf("Toplam RAM: %.2f GB\n", (double)total / (1024 * 1024 * 1024));
    printf("Kullanılan: %.2f GB\n", (double)(total - available) / (1024 * 1024 * 1024));
    printf("Boş: %.2f GB\n", (double)available / (1024 * 1024 * 1024));
    if (swap_total > 0)
        printf("Takas Alanı: %.2f GB / %.2f GB\n", (double)swap_used / (1024 * 1024 * 1024), (double)swap_total / (1024 * 1024 * 1024));
    else
        printf("Takas Alanı: Bilgi alınamadı\n");
    fflush(stdout);
}

void cli_print_disk()
{
    DWORD drives = GetLogicalDrives();
    char rootPath[4] = "A:\\";
    int found = 0;
    printf("Diskler:\n");
    for (char d = 'A'; d <= 'Z'; d++)
    {
        if (drives & (1 << (d - 'A')))
        {
            rootPath[0] = d;
            ULARGE_INTEGER freeBytes, totalBytes, totalFreeBytes;
            if (GetDiskFreeSpaceExA(rootPath, &freeBytes, &totalBytes, &totalFreeBytes))
            {
                printf("  %c: Toplam: %.2f GB, Boş: %.2f GB\n", d,
                       (double)totalBytes.QuadPart / (1024 * 1024 * 1024),
                       (double)freeBytes.QuadPart / (1024 * 1024 * 1024));
                found = 1;
            }
            else
            {
                printf("  %c: Disk bilgisi alınamadı.\n", d);
            }
        }
    }
    if (!found)
    {
        printf("Hiç disk bulunamadı veya erişilemedi.\n");
    }
    // Disk I/O istatistikleri (placeholder)
    PERFORMANCE_INFORMATION perfInfo;
    perfInfo.cb = sizeof(perfInfo);
    if (GetPerformanceInfo(&perfInfo, sizeof(perfInfo)))
    {
        printf("\nDisk I/O (sistem genelinde):\n");
        printf("  ProcessCount: %lu\n", perfInfo.ProcessCount);
        printf("  ThreadCount: %lu\n", perfInfo.ThreadCount);
        printf("  SystemCache: %.2f MB\n", (double)perfInfo.SystemCache * perfInfo.PageSize / (1024 * 1024));
    }
    else
    {
        printf("Disk I/O istatistikleri alınamadı.\n");
    }
    fflush(stdout);
}

void cli_print_net()
{
    PMIB_IFTABLE pIfTable;
    DWORD dwSize = 0;
    DWORD dwRetVal = 0;
    pIfTable = (MIB_IFTABLE *)malloc(sizeof(MIB_IFTABLE));
    if (pIfTable == NULL)
    {
        printf("Bellek ayrılırken hata oluştu.\n");
        fflush(stdout);
        return;
    }
    dwSize = sizeof(MIB_IFTABLE);
    if (GetIfTable(pIfTable, &dwSize, FALSE) == ERROR_INSUFFICIENT_BUFFER)
    {
        free(pIfTable);
        pIfTable = (MIB_IFTABLE *)malloc(dwSize);
        if (pIfTable == NULL)
        {
            printf("Bellek ayrılırken hata oluştu.\n");
            fflush(stdout);
            return;
        }
    }
    if ((dwRetVal = GetIfTable(pIfTable, &dwSize, FALSE)) == NO_ERROR)
    {
        // Toplam trafiğe göre sırala
        struct NetIfStat
        {
            unsigned int idx;
            ULONG total;
        } stats[128];
        unsigned int n = pIfTable->dwNumEntries;
        if (n > 128)
            n = 128;
        for (unsigned int i = 0; i < n; i++)
        {
            stats[i].idx = i;
            stats[i].total = pIfTable->table[i].dwOutOctets + pIfTable->table[i].dwInOctets;
        }
        // Bubble sort (küçük n için yeterli)
        for (unsigned int i = 0; i < n - 1; i++)
        {
            for (unsigned int j = i + 1; j < n; j++)
            {
                if (stats[j].total > stats[i].total)
                {
                    struct NetIfStat tmp = stats[i];
                    stats[i] = stats[j];
                    stats[j] = tmp;
                }
            }
        }
        printf("En çok trafiğe sahip 10 ağ arayüzü:\n");
        unsigned int show = n < 10 ? n : 10;
        for (unsigned int i = 0; i < show; i++)
        {
            unsigned int idx = stats[i].idx;
            printf("  %s: Gönderilen: %lu bayt, Alınan: %lu bayt\n",
                   pIfTable->table[idx].bDescr,
                   pIfTable->table[idx].dwOutOctets,
                   pIfTable->table[idx].dwInOctets);
        }
    }
    else
    {
        printf("GetIfTable başarısız, hata kodu: %lu\n", dwRetVal);
    }
    if (pIfTable)
        free(pIfTable);

    // Aktif TCP bağlantı sayısı
    PMIB_TCPTABLE pTcpTable;
    dwSize = 0;
    int connCount = 0;
    GetTcpTable(NULL, &dwSize, FALSE);
    pTcpTable = (MIB_TCPTABLE *)malloc(dwSize);
    if (pTcpTable != NULL && GetTcpTable(pTcpTable, &dwSize, FALSE) == NO_ERROR)
    {
        connCount = pTcpTable->dwNumEntries;
        printf("Aktif TCP bağlantı sayısı: %d\n", connCount);
    }
    else
    {
        printf("Aktif bağlantı sayısı alınamadı.\n");
    }
    if (pTcpTable)
        free(pTcpTable);
    fflush(stdout);
}

void cli_print_top(int n)
{
    ProcessInfo plist1[128], plist2[128];
    int pcount1 = get_process_info_list(plist1, 128);
    FILETIME id1, k1, u1;
    GetSystemTimes(&id1, &k1, &u1);
    Sleep(1000); // 1 saniye bekle
    int pcount2 = get_process_info_list(plist2, 128);
    FILETIME id2, k2, u2;
    GetSystemTimes(&id2, &k2, &u2);

    // CPU kullanımını hesapla
    ULONGLONG idle1 = (((ULONGLONG)id1.dwHighDateTime) << 32) | id1.dwLowDateTime;
    ULONGLONG idle2 = (((ULONGLONG)id2.dwHighDateTime) << 32) | id2.dwLowDateTime;
    ULONGLONG total_diff = 10000000; // 1 saniye = 10000000 100-nanosecond intervals

    for (int i = 0; i < pcount2; i++)
    {
        int found = -1;
        for (int j = 0; j < pcount1; j++)
        {
            if (plist1[j].pid == plist2[i].pid)
            {
                found = j;
                break;
            }
        }
        if (found != -1)
        {
            ULONGLONG kdiff = plist2[i].last_kernel_time - plist1[found].last_kernel_time;
            ULONGLONG udiff = plist2[i].last_user_time - plist1[found].last_user_time;
            double cpu = 100.0 * (kdiff + udiff) / (double)total_diff;
            plist2[i].cpu_usage = cpu;
        }
    }

    // Kullanıcı süreçlerini filtrele
    ProcessInfo user_processes[128];
    int user_count = 0;
    for (int i = 0; i < pcount2; i++)
    {
        if (plist2[i].pid > 4 && plist2[i].nameA[0] != '\0' &&
            strcmp(plist2[i].status, "System") != 0)
        {
            user_processes[user_count++] = plist2[i];
        }
    }

    // CPU'ya göre sırala
    for (int i = 0; i < user_count - 1; i++)
    {
        for (int j = i + 1; j < user_count; j++)
        {
            if (user_processes[j].cpu_usage > user_processes[i].cpu_usage)
            {
                ProcessInfo tmp = user_processes[i];
                user_processes[i] = user_processes[j];
                user_processes[j] = tmp;
            }
        }
    }

    printf("En çok CPU kullanan %d süreç:\n", n);
    printf("%-8s %-6s %-12s %-10s %s\n", "PID", "CPU%", "RAM (MB)", "Durum", "İsim");
    printf("--------------------------------------------------------------\n");

    int show = 0;
    for (int i = 0; i < user_count && show < n; i++)
    {
        if (user_processes[i].cpu_usage > 0.0 || user_processes[i].ram_usage > 0)
        {
            printf("%-8u %-6.1f %-12.1f %-10s %s\n",
                   user_processes[i].pid,
                   user_processes[i].cpu_usage,
                   (double)user_processes[i].ram_usage / (1024 * 1024),
                   user_processes[i].status,
                   user_processes[i].nameA);
            show++;
        }
    }

    if (show == 0 && user_count == 0)
    {
        printf("Hiç süreç bulunamadı.\n");
    }
    fflush(stdout);
}