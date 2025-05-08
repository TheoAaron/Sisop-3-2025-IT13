# Sisop-3-2025-IT13

## Soal 2

### 📌 Fitur Utama

Proyek ini menghadirkan Sistem Manajemen Pengiriman Paket yang terdiri dari dua komponen utama:

- **Dispatcher** `(dispatcher.c)` - Pusat kendali sistem

- **Agen Pengiriman** `(delivery_agent.c)` - Tim otomatis pengantar paket

Kedua sistem ini dibangun dengan menggunakan shared memory dan multi-threading, sistem ini akan mengelola pengiriman paket secara efisien dengan koordinasi real-time.

### 🖥️ Delivery Agent

Agent Delivery ini dapat dilihat logicnya pada kode delivery_agent.c, dimana mereka akan secara automatis mengirim paket ke lokasi tujuan dan jika sudah selesai, maka Agent otomatis akan mencari lagi paket yang menggunakan fitur express yang belum dikirimkan. Jika sudah tidak ada lagi paket express, maka ketiga agent itu akan berhenti.

### 🖥️ Dispatcher

Pada kode dispatcher, terdapat 3 sub-fitur yang ada, yaitu

- `-list` untuk menampilkan seluruh pemilik paket dan status paketnya
- `-deliver` untuk mengirim sebuah paket milik orang tertentu
- `-status` untuk melihat status barang dari orang tertentu

Terdapat juga otomasi download untuk mendownload file csv yang ada tanpa perlu mendownload secara manual dan akan berhenti jika file csv sudah ada terdownload.

### 📝 Pencatatan Log

Semua pengiriman yang dilakukan, akan dicatat pada sebuah log bernama `delivery.log` dimana pada log tersebut akan berisi semua pengiriman yang dilakukan baik dari express maupun reguler pada tanggal berapa, oleh siapa, dan kemana paket tersebut dikirim.

### ⚙️ Logika Soal

Pada soal ini, terdapat beberapa teknologi inti, yaitu:
- `IPC (Shared Memory)`
- `Sinkronisasi Mutex`
- `POSIX Threads`

Berikut merupakan implementasi teknis dari soal ini dalam bentuk diagram:

![Image](https://github.com/user-attachments/assets/1a7408af-d1e6-4d77-8907-c3828aa5e3d1)

Terdapat juga error handling yang dapat digunakan ketika user memasukkan input yang tidak diinginkan.