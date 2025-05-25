// main.c - Entry point for MiniMon
#include <stdio.h>
#include <stdint.h>
#include "core/collector.h"
#include <string.h>
#include <stdlib.h>
#include "ui/cli.h"
#include "core/reporter.h"
#include <windows.h>

// Helper for sorting processes by CPU usage
typedef int (*cmp_func)(const void *, const void *);
int cmp_cpu_desc(const void *a, const void *b) {
    const ProcessInfo *pa = (const ProcessInfo *)a;
    const ProcessInfo *pb = (const ProcessInfo *)b;
    if (pb->cpu_usage > pa->cpu_usage) return 1;
    if (pb->cpu_usage < pa->cpu_usage) return -1;
    return 0;
}

void printHelp() {
    printf("Kullanım: minimon [komut] [parametreler]\n\n");
    printf("Komutlar:\n");
    printf("  cpu     - CPU kullanım bilgilerini gösterir\n");
    printf("  mem     - Bellek kullanım bilgilerini gösterir\n");
    printf("  disk    - Disk kullanım ve I/O bilgilerini gösterir\n");
    printf("  net     - Ağ bağlantılarını ve istatistiklerini gösterir\n");
    printf("  top N   - En çok kaynak kullanan N adet süreci gösterir\n");
    printf("  report  - Sistem kaynakları raporu oluşturur\n");
    printf("  help    - Bu yardım mesajını gösterir\n\n");
    printf("Raporlama Komutları:\n");
    printf("  report csv [dosya]     - CSV formatında rapor oluşturur\n");
    printf("  report json [dosya]    - JSON formatında rapor oluşturur\n");
    printf("  report start [dosya] [saniye] - Periyodik raporlama başlatır\n");
}

int main(int argc, char *argv[]) {
    SetConsoleOutputCP(CP_UTF8);
    if (argc < 2) {
        printHelp();
        return 1;
    }
    if (strcmp(argv[1], "cpu") == 0) {
        cli_print_cpu();
    } else if (strcmp(argv[1], "mem") == 0) {
        cli_print_mem();
    } else if (strcmp(argv[1], "disk") == 0) {
        cli_print_disk();
    } else if (strcmp(argv[1], "net") == 0) {
        cli_print_net();
    } else if (strcmp(argv[1], "top") == 0) {
        if (argc < 3) {
            printf("Hata: Süreç sayısı belirtilmedi\n");
            return 1;
        }
        int n = atoi(argv[2]);
        cli_print_top(n);
    } else if (strcmp(argv[1], "report") == 0) {
        if (argc < 3) {
            printf("Hata: Rapor formatı belirtilmedi\n");
            return 1;
        }
        if (strcmp(argv[2], "csv") == 0 && argc >= 4) {
            reporter_generate_csv(argv[3]);
        } else if (strcmp(argv[2], "json") == 0 && argc >= 4) {
            reporter_generate_json(argv[3]);
        } else if (strcmp(argv[2], "start") == 0 && argc >= 5) {
            int interval = atoi(argv[4]);
            reporter_start_periodic(argv[3], interval);
        } else {
            printf("Hata: Geçersiz rapor komutu\n");
            return 1;
        }
    } else if (strcmp(argv[1], "help") == 0) {
        printHelp();
    } else {
        printf("Hata: Geçersiz komut\n");
        printHelp();
        return 1;
    }
    return 0;
} 