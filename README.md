# soal-shift-sisop-modul-4-B02-2021

### 1. Enkripsi versi 1:

```c
void atbashEncrypt(char source[1000], char encrypted[1000])
{

    char source_copy[1000] = "";
    strcpy(source_copy, source);

    int i;
    for (i = 0; strlen(source_copy) > i; i++)
    {
        if (source_copy[i] == '/')
        {
            encrypted[i] = '/';
            continue;
        }
        if (source_copy[i] >= 'A' && source_copy[i] <= 'Z')
        {
            encrypted[i] = 'Z' - source_copy[i] + 'A';
        }
        else if (source_copy[i] >= 'a' && source_copy[i] <= 'z')
        {
            encrypted[i] = 'z' - source_copy[i] + 'a';
        }
    }
}
```

Untuk melakukan enkrip dengan atbash, pertama kita harus mengubah setiap karakter dengan mengurangi nilai ASCII dari karakter 'Z' atau 'z' dengan salah satu karakter dari string yang ingin di encrypt lalu ditambahkan nilai ASCII 'A' atau 'a' Kita lakukan looping ini selama masih ada karakter yang ingin di enkrip.

```c
void atbashDecrypt(char source[1000], char encrypted[1000])
{
    char source_copy[1000] = "";
    strcpy(source_copy, source);

    int i = 0;
    for (i = 0; strlen(source_copy) > i; i++)
    {
        if (source_copy[i] == '/')
        {
            encrypted[i] = '/';
            continue;
        }

        if (source_copy[i] >= 'A' && source_copy[i] <= 'Z')
        {
            encrypted[i] = 'A' + ('Z' - source_copy[i]);
        }
        else if (source_copy[i] >= 'a' && source_copy[i] <= 'z')
        {
            encrypted[i] = 'a' + ('z' - source_copy[i]);
        }
    }
}
```

Untuk dekripnya hampir sama dengan enkrip. Bedanya disini adalah jika dienkrip kita mengurangi nilai ASCII 'Z' dan ditambahkan dengan ASCII 'A' maka sekarng nilai ASCII 'A' dikurangi dan ditambahkan dengan ASCII 'Z'.

Sekarang kita bisa menuliskan tambahan kode untuk dekripsi path pada fungsi `xmp_getattr`, `xmp_readdir`, dan `xmp_read` untuk membaca fpath :

```c
    char *token = strtok(path, "/");
    while (token != NULL)
    {
        if (encrypt_needed)
        {
            strcat(encrypted_path, "/");
            strcat(encrypted_path, token);
        }
        else if (!encrypt_needed)
        {
            strcat(current_path, "/");
            strcat(current_path, token);
        }
        if (strncmp(token, "AtoZ_", 5) == 0) //perlu encrypt
        {
            encrypt_needed = true;
        }
        token = strtok(NULL, "/");
    }
```

pada perulangan ini akan mendeteksi apakah perlu diadakan enkripsi tipe 1. Caranya dengan mengecek pathnya. Kita akan memasukkan path kedalam `current_path` satu per satu. Untuk setiap directory, dicek apakah namanya berisi "AtoZ\_" di awalnya. Jika iya maka perlu dienkrip. Setelah itu sisa dari path setelah "AtoZ\_" itu akan dimasukkan ke string yang berbeda dengan path sebelumnya.

```c
    if (encrypt_needed)
    {
        char checkFile[1000] = "", checkEnc[1000] = "";

        char *titik = strrchr(encrypted_path, '.');
        if (titik != NULL)
        {
            strncpy(checkFile, encrypted_path, titik - encrypted_path);
            atbashDecrypt(checkFile, checkEnc);
        }

        char check_doc[1000] = "";
        strcat(check_doc, dir_path);
        strcat(check_doc, current_path);
        strcat(check_doc, checkEnc);
        if (titik != NULL)
            strcat(check_doc, titik);

        if (isRegFile(check_doc))
        {
            sprintf(fpath, "%s%s%s%s", dir_path, current_path, checkEnc, titik);
        }
        else
        {
            atbashDecrypt(encrypted_path, enc);
            sprintf(fpath, "%s%s%s", dir_path, current_path, enc);
        }
    }
    else
    {
        atbashDecrypt(encrypted_path, enc);
        sprintf(fpath, "%s%s%s", dir_path, current_path, enc);
    }
```

Jika enkripsi diperlukan, maka kita akan memisahkan nama file dengan ekstensinya dengan `strrchr` yang fungsinya untuk mendeteksi karakter yang match yang paling akhir. Setelah itu kita akan mencheck apakah yang ingin di dekrip ini file atau directory. Jika file maka yang didekrip hanyalah namafile nya saja, ekstensinya tidak. Jika direktori, seluruhnya akan didekrip. fungsi mendekrip disini adalah mendapatkan nama path yang aslinya sehingga pada operasi selanjutnya dapat menggunakan path ini.

Jika enkripsi tidak diperlukan maka kita tidak perlu mendekripsi nama file/direktori nya.

Pada `xmp_readdir` harus ditambahkan kode lagi selain diatas, yaitu untuk melakukan enkripsi. Tambahan kodenya ada pada perulangan menbaca file-file dari folder yang dibuka.

```c
    while ((de = readdir(dp)) != NULL)
    {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
            continue;
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;

        if (encrypt_needed)
        {
            if (de->d_type == DT_DIR) //direktori
            {
                char enc[1000] = "";
                atbashEncrypt(de->d_name, enc);
                res = (filler(buf, enc, &st, 0));
            }
            else if (de->d_type == DT_REG) //file reguler
            {
                char enc[1000] = "", fileLengkap[1000] = "", fileName[1000] = "";

                strcpy(fileLengkap, de->d_name);
                char *titik = strrchr(fileLengkap, '.');

                if (titik != NULL)
                    strncpy(fileName, fileLengkap, titik - fileLengkap);
                else
                    strcpy(fileName, fileLengkap);

                atbashEncrypt(fileName, enc);

                if (titik != NULL)
                    strcat(enc, titik);

                res = (filler(buf, enc, &st, 0));
            }
        }
        else
        {
            res = (filler(buf, de->d_name, &st, 0));
        }

        if (res != 0)
            break;
    }
```

Pertama, kita akan cek jika directory yang dibaca adalah "." dan "..". Direktori tersebut tidak perlu dienkripsi. Kita akan cek pula apakah enkripsi diperlukan atau tidak, dari pengecekan nama direktori diatas tadi. Jika perlu dienkrip, maka kita mengcek terlebih dahulu apakah dia file atau direktori.

Sama seperti diatas, file tidak perlu mengenkripsi ekstensinya sedangkan folder akan menenkripsi semua. Pada file kita perlu memisahkan nama dengan ekstensinya, dan hanya nama yang dienkripsi. Jika sudah, maka akan dimasukkan ke fungsi `filler()`

### 4. Log system:

```c
char *log_file_path = "/home/fwe/SinSeiFS.log";

// Membuat log dengan pesan Warning
void logWarning(const char *log, const char *path)
{
    FILE *fp;
    fp = fopen(log_file_path, "a");
    fputs("WARNING::", fp);
    char log_timestamp_string[1000];
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    sprintf(log_timestamp_string, "%02d%02d%02d-%02d:%02d:%02d::", tm.tm_mday, tm.tm_mon + 1, (tm.tm_year + 1900) % 100, tm.tm_hour, tm.tm_min, tm.tm_sec);
    fputs(log_timestamp_string, fp);
    fputs(log, fp);
    fputs("::", fp);
    fputs(path, fp);
    fputs("\n", fp);
    fclose(fp);
}

// Membuat log dengan pesan Info
void logInfo1(const char *log, const char *path)
{
    FILE *fp;
    fp = fopen(log_file_path, "a");
    fputs("INFO::", fp);
    char log_timestamp_string[1000];
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    sprintf(log_timestamp_string, "%02d%02d%02d-%02d:%02d:%02d::", tm.tm_mday, tm.tm_mon + 1, (tm.tm_year + 1900) % 100, tm.tm_hour, tm.tm_min, tm.tm_sec);
    fputs(log_timestamp_string, fp);
    fputs(log, fp);
    fputs("::", fp);
    fputs(path, fp);
    fputs("\n", fp);
    fclose(fp);
}

// Membuat log dengan pesan Info tapi menerima dua path, source dan destination
void logInfo2(const char *log, const char *source, const char *destination)
{
    FILE *fp;
    fp = fopen(log_file_path, "a");
    fputs("INFO::", fp);
    char log_timestamp_string[1000];
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    sprintf(log_timestamp_string, "%02d%02d%02d-%02d:%02d:%02d::", tm.tm_mday, tm.tm_mon + 1, (tm.tm_year + 1900) % 100, tm.tm_hour, tm.tm_min, tm.tm_sec);
    fputs(log_timestamp_string, fp);
    fputs(log, fp);
    fputs("::", fp);
    fputs(source, fp);
    fputs("::", fp);
    fputs(destination, fp);
    fputs("\n", fp);
    fclose(fp);
}
```

**Penjelasan :**

Setiap kali perintah rmdir dan unlink dipanggil, akan dimasukkan catatan ke dalam file fs.log ditandai dengan level WARNING. Selain itu, pemanggilan fungsi lain akan ditandai dengan level INFO. Isi pesan setelah tingkat level diikuti dengan timestamp ddmmyyyy-HH:MM:SS. Timestamp kemudian diikuti dengan perintah yang dipanggil dan deskripsi perintah. Deskripsi perintah yang diisi berupa path dimana perintah di jalankan.
