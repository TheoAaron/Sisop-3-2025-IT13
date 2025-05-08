# Sisop-3-2025-IT13

## soal 1

### fitur utama sisi client
Memberikan pilihan :
1. Decrypt file : Mendecrypt file.txt yang berasal dari client/secrets
2. Download file : download file dari database
3. exit

penjelasan kode

A. jika memilih 1 maka client akan mengirim sinyal ke server untuk menjalankan fungsi decrypt yang ada di server

B. Jika memilih 2 maka client akan mengirim sinyal ke server untuk menjalankan fungsi download, dengan di bagian client akan melakukan perintah untuk menulis kembali file untuk disimpan di direktori client

C. Jika memilih 3 maka client akan mengirim sinyal exit ke server

### fitur utama sisi server
Menyediakan berbagai fungsi untuk menjalankan sinyal dari client, jadi server harus dijalankan terlebih dahulu agar client bisa berjalan dan mengirim perintah


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

---

## Soal 3

### 📌 Penjelasan Awal

Pada soal ini, terdapat implementasi simpel sebuah game petualangan RPG dimana player dapat melakukan berbagai macam hal, seperti membeli equipment dan bertarung melawan monster.

Sistem ini terdiri dari tiga komponen utama, yaitu:

- **Server** `(dungeon.c)`: Menangani logika game, data pemain, dan generasi musuh

- **Client** `(player.c)`: Menyediakan antarmuka pengguna dan berkomunikasi dengan server

- **Sistem Toko** `(shop.c)`: Mengelola data senjata dan fungsionalitas toko

Pada komponen-komponen utama tersebut, terdapat koneksi jaringan antar tiap-tiap komponen yang menyammbungkan kinerja ketiga komponen utama tersebut yang bernama `TCP`.

`TCP` **(Transmission Control Protocol)** adalah sebuah protokol standar yang digunakan dalam komunikasi jaringan, khususnya di internet. Fungsinya adalah untuk memastikan data dikirimkan secara andal dan terurut dari satu perangkat ke perangkat lain. 

`TCP` bekerja di lapisan transport dalam model referensi OSI, dan menyediakan layanan koneksi yang andal untuk aplikasi. 

Contohnya pada file player.c dengan dungeon.c dimana terdapat hubungan TCP yang mempersatukan kedua file ini.

### ⚙️ Fitur Utama

Pada program ini, terdapat beberapa fitur utama, yaitu:

1. **Show Status**: Tempat player dapat melihat status mereka yang isinya berupa gold, item yang digunakan, attack power player, dan jumlah monster yang dikalahkan oleh player.

2. **Shop**: Tempat player dapat membeli senjata yang diinginkan untuk menambah attack power mereka dan mendapat pasif unik ke status mereka.

3. **Show Inventory & Equip Items**: Tempat persenjataan player berada, disini player dapat melihat senjata apa saja yang mereka miliki dan menggunakan senjata tersebut.

4. **Battle With Enemies**: Tempat dimana player dapat bertarung dengan banyak monster untuk mendapat koin.

### 🔧 Implementasi File `dungeon.c`

Pada `dungeon.c`, terdapat beberapa fitur yang bertujuan sebagai fondasi dari program ini, yaitu:

- Mengelola status pemain. Pada file ini, status pemain akan diubah jika terdapat perubahan seperti penambahan gold, pergantian senjata dan attack power, dan penampilan pasif jika ada.

- Membuat musuh dengan HP yang bervariasi. Dengan menggunakan fungsi `rand()` untuk menghasilkan musuh yang memiliki HP yang random dan bervariasi

Cara kerja server untuk memproses perintah client adalah setelah user menginput opsi user, maka kita akan mengubah input tersebut menjadi sebuah string untuk dijadikan acuan pada file dungeon agar lebih mudah diolah.
Bentukan stringnya yanng terdapat pada kode tersebut adalah:

1. `GET_STATS`: Mengembalikan statistik pemain

2. `GET_INVENTORY`: Mengembalikan inventori pemain

3. `GET_ENEMY`: Mengembalikan data musuh saat ini

4. `ATTACK`: Menangani mekanik pertarungan

5. `BUY`: Memproses pembelian senjata

6. `EQUIP`: Menangani pergantian senjata

**Mekanik Pertarungan**

- Perhitungan kerusakan dengan variasi acak (80-120% dari kerusakan dasar)

- 20% kemungkinan serangan kritis (2x kerusakan)

- Kemampuan khusus senjata (contoh: 10% kemungkinan membunuh instan)

**Komunikasi Jaringan**
- Server TCP yang mendengarkan di port PORT (terdefinisi di dungeon.h)

- Menangani beberapa client secara berurutan

- Menggunakan struktur data berukuran tetap untuk komunikasi

### 🔧 Implementasi Client `(player.c)`

Berikut adalah fitur-fitur yang terdapat pada program ini:
- Antarmuka Pengguna

- Menu Utama:
    1. Tampilkan Statistik Pemain
    2. Toko
    3. Inventori & Ganti Senjata
    4. Bertarung
    5. Keluar

Adapun tampilan sistem toko pada program ini yang berguna untuk:

- **Menampilkan senjata yang tersedia dengan harga dan statistik**

- **Menangani transaksi pembelian**

- **Manajemen Inventori**: Menampilkan senjata yang dimiliki
- **Memungkinkan pergantian senjata**

Pada file ini, sistem pertarungan berfungsi untuk:

- Bar kesehatan visual untuk musuh

- Opsi Serang/Kabur

- Sistem hadiah untuk musuh yang dikalahkan

- Komunikasi Jaringan

- Terhubung ke server di 127.0.0.1:PORT

- Mengirim perintah dan menerima pembaruan secara sinkron

### 🔧 Sistem Toko `(shop.c)`

FIle `shop.c` hanya berfungsi untuk menampilkan dan menyimpan senjata-senjata yang dapat digunakan oleh pemain. Berikut merupakan fungsi dari file ini:

- Menampilkan senjata untuk dibeli

- Initialize shop uuntuk menginfokan bahwa fungsi shop dipanggil
