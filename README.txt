MiniMon - Windows Sistem İzleme Aracı
====================================

MiniMon, Windows işletim sisteminde çalışan bir sistem izleme aracıdır. CPU, RAM, disk ve ağ kullanımını gerçek zamanlı olarak izlemenizi sağlar.

Kurulum
-------
1. MiniMon'u indirin
2. İndirdiğiniz dosyayı istediğiniz bir konuma çıkartın
3. Komut istemini (CMD) veya PowerShell'i yönetici olarak çalıştırın
4. MiniMon'un bulunduğu dizine gidin
5. Programı çalıştırın: minimon.exe [komut]

Kullanım

Önemli Bilgi: Verileri doğru çekebilmek için Visual Studio Code yönetici olarak çalıştırılmalıdır! 

--------
MiniMon aşağıdaki komutları destekler:

1. CPU Bilgileri:
   minimon.exe cpu
   - Genel CPU kullanımını gösterir
   - Kullanıcı, sistem ve boşta kalma yüzdelerini gösterir

2. Bellek Bilgileri:
   minimon.exe mem
   - Toplam RAM miktarını gösterir
   - Kullanılan ve boş RAM miktarını gösterir
   - Takas alanı kullanımını gösterir

3. Disk Bilgileri:
   minimon.exe disk
   - Tüm disklerin toplam ve boş alanlarını gösterir
   - Disk I/O istatistiklerini gösterir

4. Ağ Bilgileri:
   minimon.exe net
   - Ağ arayüzlerinin trafik bilgilerini gösterir
   - Aktif TCP bağlantı sayısını gösterir

5. Süreç Listesi:
   minimon.exe top N
   - En çok kaynak kullanan N adet süreci gösterir
   - CPU ve RAM kullanımlarını gösterir
   - Örnek: minimon.exe top 5

6. Raporlama:
   minimon.exe report [format] [dosya] [aralık]
   - CSV formatında rapor: minimon.exe report csv rapor.csv
   - JSON formatında rapor: minimon.exe report json rapor.json
   - Periyodik rapor: minimon.exe report start rapor.csv 60

7. Yardım:
   minimon.exe help
   - Tüm komutları ve kullanımlarını gösterir

Örnekler
--------
1. CPU kullanımını görüntüleme:
   minimon.exe cpu

2. En çok CPU kullanan 5 süreci görüntüleme:
   minimon.exe top 5

3. Her 5 dakikada bir CSV raporu oluşturma:
   minimon.exe report start rapor.csv 300

Notlar
------
- Programı yönetici olarak çalıştırmanız gerekir.
- Bazı sistem süreçlerine erişim kısıtlı olabilir.
- Periyodik raporlama için Ctrl+C ile programı sonlandırabilirsiniz.

İletişim
--------
Mert Yılmaz Birinci 
Github: https://github.com/Birincimert

