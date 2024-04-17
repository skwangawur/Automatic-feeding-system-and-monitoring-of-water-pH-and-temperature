#include <ESP32Servo.h> //libary untuk servo ESP32
#include <WiFi.h>       //library untuk mengkoneksikan ESP32 ke jaringan internet
#include "time.h"       //library untuk menampilkan waktu
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>                //Memanggil library OneWire yang diperlukan sebagai dependensi library Dallas Temperature
#include <DallasTemperature.h>      // Memanggil library Dallas Temperature
#define ONE_WIRE_BUS 15             // Menempatkan PIN hasil pembacaan sensor DS18B20 pada PIN 2.
                                    // Disebut One Wire karena kita bisa menempatkan sensor DS18B20 lain pada PIN yang sama
LiquidCrystal_I2C lcd(0x27, 16, 2); // Mengatur alamat LCD dan dimensi LCD, yaitu 20 kolom dan 4 baris

OneWire oneWire(ONE_WIRE_BUS);      // Membuat variabel oneWire berdasarkan PIN yang telah didefinisikan
DallasTemperature sensor(&oneWire); // Membuat variabel untuk menyimpan hasil pengukuran
float suhuDS18B20;                  // deklarasi variable suhu DS18B20 dengan jenis data float

#define EEPROM_ADDRESS 0
int usia;
float Value = 0;

const char* ssid       = ""; //nama jaringan internet atau wifi
const char* password   = ""; // password wifi atau internet

// const char* ssid       = "ECHI"; //nama jaringan internet atau wifi
// const char* password   = "1sampai8"; // password wifi atau internet

// const char *ssid = "Zona Aman_plus";       // nama jaringan internet atau wifi
// const char *password = "persiapanqurban"; // password wifi atau internet

// Initialize Telegram BOT
#define BOTtoken "" // your Bot Token (Get from Botfather)

// Use @myidbot to find out the chat ID of an individual or a group
// Also note that you need to click "start" on a bot before it can
// message you
// #define CHAT_ID "807825264"
#define CHAT_ID ""

// PH
float calibration_value = 21.74 - 0.7; // 21.34 - 0.7
int phval = 0;
unsigned long int avgval;
int buffer_arr[10], temp;

float ph;

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

// Checks for new messages every 1 second.
int botRequestDelay = 1000;
unsigned long lastTimeBotRan;
String waktuTerakhirMakan = "";
const char *ntpServer = "asia.pool.ntp.org"; // mengambil data waktu pada ntp server asia
const long gmtOffset_sec = 7 * 60 * 60;      // nilai utc jakarta GMT+(07:00)
const int daylightOffset_sec = 0;            // udaily light saving atau Waktu Musim Panas adalah suatu praktik pemajuan jarum jam semasa musim panas sehingga malam hari datang pada pukul yang lebih lambat setiap harinya. bernilai 0 karena Indonesia tidak menerepkan dailty light saving

int jam;   // inisiasi variable jam untuk menyimpan nilai jam pada server
int menit; // inisiasi variable menit menyimpan nilai menit pada server
int detik; // inisiasi variable detik menyimpan nilai detik pada server

int hour;
int minute;
int second;

int jadwal = 0;



void printLocalTime()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {                                                           // cek apakah bisa mendapatkan informasi waktu lokal pada server waktu
        Serial.println("Gagal mendapatkan waktu lokal server"); // jika gagal akan menampilkan pesan
        return;
    }
    jam = timeinfo.tm_hour;  // mengambil jam pada server ntp / server waktu asia dan menyimpannya pada variable jam
    menit = timeinfo.tm_min; // mengambil menit pada server ntp / server waktu asia dan menyimpannya pada variable menit
    detik = timeinfo.tm_sec; // mengambil detik pada server ntp / server waktu asia dan menyimpannya pada variable detik `

    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    // %A	mengembalikan nilai atau menampilkan hari dalam minggu
    // %B	mengembalikan nilai atau menampilkan bulan dalam tahun
    // %d	mengembalikan nilai atau menampilkan hari dalam bulan
    // %Y	mengembalikan nilai atau menampilkan tahun
    // %H	mengembalikan nilai atau menampilkan hari
    // %M	mengembalikan nilai atau menampilkan menit
    // %S	mengembalikan nilai atau menampilkan detik
}

#define MOTOR_RELAY 27

// Handle what happens when you receive new messages
void handleNewMessages(int numNewMessages)
{
    Serial.println("handleNewMessages");
    Serial.println(String(numNewMessages));

    for (int i = 0; i < numNewMessages; i++)
    {
        // Chat id of the requester
        String chat_id = String(bot.messages[i].chat_id);
        if (chat_id != CHAT_ID)
        {
            bot.sendMessage(chat_id, "Unauthorized user", "");
            continue;
        }
        // Print the received message
        String text = bot.messages[i].text;
        Serial.println(text);

        String from_name = bot.messages[i].from_name;
        if (text == "/start")
        {

            String welcome = "Selamat Datang," + from_name + ".\n" + "/kasihmakan" + "\n\n" + "/lihatkualitasair" + "\n" + "/waktuterakhirmakan" + "\n" + "/tambahjadwal" + "\n" + "/lihatusialele"+ "\n" + "/lihatjadwal" + "\n\n\n" + "/resetusialele" + "\n\n" + "/resetjadwal";
            bot.sendMessage(chat_id, welcome, "");
            printLocalTime(); // menampilakna waktu (hari, bulan, tanggal, tahun, dan jam)
        }
        if (text == "/kasihmakan")
        {
            printLocalTime();
            waktuTerakhirMakan = String(jam) + ":" + String(menit) + ":" + String(detik);
            digitalWrite(MOTOR_RELAY, LOW);
            bot.sendMessage(chat_id, "Berhasil kasih makan");
            delay(5000);
            digitalWrite(MOTOR_RELAY, HIGH);
        }
        if (text == "/lihatkualitasair")
        {
            bot.sendMessage(chat_id, "PH Air: " + String(ph));
            sensor.setResolution(12);
            sensor.requestTemperatures(); // Perintah konversi suhu
            String suhu = String(sensor.getTempCByIndex(0));
            bot.sendMessage(chat_id, "Suhu Air: " + suhu + "C");
        }
        if (text == "/waktuterakhirmakan")
        {
            String response = "Waktu terakhir makan: " + waktuTerakhirMakan;
            bot.sendMessage(chat_id, response);
        }
        if (text == "/tambahjadwal")
        {
            jadwal = jadwal + 1; // Ubah nama variabel untuk menghindari konflik
            if (jadwal <= 2)
            {
                bot.sendMessage(chat_id, "Program auto feeder telah ditambahkan sebanyak 2 jadwal");
            }
            else
            {
                bot.sendMessage(chat_id, "Program auto feeder hanya bisa menambahkan maksimal 2x jadwal");
            }
        }
        if (text == "/lihatusialele")
        {
            String usialele = String(usia);
            bot.sendMessage(chat_id, "Usia Lele: " + usialele + " hari");
        }
        if (text == "/lihatjadwal")
        {
            bot.sendMessage(chat_id, "Jadwal pemberian makan lele\nPukul 07.30\nPukul 12.30\nPukul 17.01\nPukul 21.30");
            if (jadwal >= 1) {
                bot.sendMessage(chat_id, "Pukul 10.05\nPukul 15.05");
            }
            if (jadwal >= 2) {
                bot.sendMessage(chat_id, "Pukul 13.30\nPukul 19.30");
            }
        }
        if (text.startsWith("/tambahusialele ")) {
            // Extract the number after the command "/tambahusialele "
            String numStr = text.substring(16); // Length of "/tambahusialele " is 16
            int numDays = numStr.toInt(); // Convert the extracted string to an integer

            if (numDays > 0) {
                usia += numDays;
                EEPROM.write(EEPROM_ADDRESS, usia);
                EEPROM.commit();
                String usialele = String(usia);
                bot.sendMessage(chat_id, "Usia lele berhasil ditambahkan sebanyak " + numStr + " hari. Usia lele sekarang: " + usialele + " hari");
            } else {
                bot.sendMessage(chat_id, "Masukkan jumlah hari yang valid untuk ditambahkan ke usia lele.", "");
            }
        }
        if (text == "/resetusialele")
        {
            usia = 0;
            EEPROM.write(EEPROM_ADDRESS, usia);
            EEPROM.commit();
            String usialele = String(usia);
            bot.sendMessage(chat_id, "Usia lele berhasil direset, usia lele sekarang: " + usialele + " hari");
        }
        if (text == "/resetjadwal")
        {
            jadwal = 0;
            bot.sendMessage(chat_id, "Jadwal program auto feeder telah direset menjadi semula");
        }
        if (ph < 6)
        {
            bot.sendMessage(chat_id, "PH Jelek! Segera Ganti Air!");
        }
    }
    delay(1000); // delay 1 detik (1000 miliseconds)
}

void setup()
{                       // void setup akan dijalankan sekali ketika program atau esp32 aktif
    Serial.begin(9600); // serial komunikasi pada esp
    Serial.printf("Connecting to %s ", ssid);
    WiFi.begin(ssid, password); // memulai untuk menhubungkan ke jaringan internet
    while (WiFi.status() != WL_CONNECTED)
    { // cek status koneksi internet
        delay(500);
        Serial.print(".");
    }
    Serial.println(" CONNECTED");
    EEPROM.begin(512);
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); // konfigurasi waktu pada server ntp
    printLocalTime();                                         // menampilakna waktu (hari, bulan, tanggal, tahun, dan jam)
    pinMode(MOTOR_RELAY, OUTPUT);
    digitalWrite(MOTOR_RELAY, HIGH);
    usia = EEPROM.read(EEPROM_ADDRESS);
    lcd.begin(); // Mulai LCD
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Welcome");
    lcd.setCursor(0, 1);
    lcd.print("Auto Feeder");
    delay(5000);
    lcd.clear();
#ifdef ESP32
    client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
#endif
}

bool kondisiTerpenuhi = false; // tambahkan variabel untuk melacak apakah kondisi terpenuhi
bool usiaDitambahkan = false;
int jamSebelumnya = -1;
int menitSebelumnya = -1;
void loop()
{
    for (int i = 0; i < 10; i++)
    {
        buffer_arr[i] = analogRead(35);
        delay(30);
    }
    for (int i = 0; i < 9; i++)
    {
        for (int j = i + 1; j < 10; j++)
        {
            if (buffer_arr[i] > buffer_arr[j])
            {
                temp = buffer_arr[i];
                buffer_arr[i] = buffer_arr[j];
                buffer_arr[j] = temp;
            }
        }
    }
    avgval = 0;
    for (int i = 2; i < 8; i++)
        avgval += buffer_arr[i];
    float volt = (float)avgval * 3.3 / 4096.0 / 6;
    // Serial.print("Voltage: ");
    // Serial.println(volt);
    ph = -5.70 * volt + calibration_value;
    if (ph < 6)
    {
        lcd.setCursor(0, 0);
        lcd.print("  PH Jelek!");
        lcd.setCursor(0, 1);
        lcd.print("Segera Ganti Air");
    }
    else
    {
        lcd.setCursor(0, 0);
        lcd.print("PH Air :");
        lcd.print(ph);
        sensor.setResolution(12);
        sensor.requestTemperatures(); // Perintah konversi suhu
        suhuDS18B20 = sensor.getTempCByIndex(0);
        lcd.setCursor(0, 1);
        lcd.print("Temp   :");
        lcd.print(suhuDS18B20);
        lcd.print((char)223);
        lcd.print("C");
    }

    printLocalTime();      // menampilakna waktu (hari, bulan, tanggal, tahun, dan jam)
    int jamSekarang = jam; // catat nilai jam saat ini
    int menitSekarang = menit;

    if (jamSekarang < jamSebelumnya)
    {
        if (!usiaDitambahkan)
        {
            usia++;
            usiaDitambahkan = true;
            EEPROM.write(EEPROM_ADDRESS, usia);
            EEPROM.commit();
        }
    }
    else
    {
        usiaDitambahkan = false; // Reset flag usiaDitambahkan setiap harinya
    }

    if (jamSekarang == 07 && menitSekarang == 30 && detik >= 1 && detik <= 59) //pakan otomatis 1
    {
        if (!kondisiTerpenuhi || (jamSebelumnya != jamSekarang || menitSebelumnya != menitSekarang))
        {
            // Jalankan loop jika kondisi belum terpenuhi sebelumnya
            // atau jika kondisi terpenuhi tetapi nilai jam atau menit berubah
            if (suhuDS18B20 >= 26 && ph >= 6)
            {
                waktuTerakhirMakan = String(jam) + ":" + String(menit) + ":" + String(detik);
                digitalWrite(MOTOR_RELAY, LOW);
                Serial.print("nyala");
                delay(30000);
                digitalWrite(MOTOR_RELAY, HIGH);
                Serial.print("mati");
            }
            else
            {
                bot.sendMessage(CHAT_ID, "Lele tidak diberi makan, kondisi air jelek!");
                digitalWrite(MOTOR_RELAY, HIGH);
                Serial.print("mati");
            }
            kondisiTerpenuhi = true; // tandai kondisi terpenuhi
        }
    }
    else if (jamSekarang == 12 && menitSekarang == 30 && detik >= 1 && detik <= 59) // pakan otomatis 2
    {
        if (!kondisiTerpenuhi || (jamSebelumnya != jamSekarang || menitSebelumnya != menitSekarang))
        {
            // Jalankan loop jika kondisi belum terpenuhi sebelumnya
            // atau jika kondisi terpenuhi tetapi nilai jam atau menit berubah
            if (suhuDS18B20 >= 26 && ph >= 6)
            {
                waktuTerakhirMakan = String(jam) + ":" + String(menit) + ":" + String(detik);
                digitalWrite(MOTOR_RELAY, LOW);
                Serial.print("nyala");
                delay(5000);
                digitalWrite(MOTOR_RELAY, HIGH);
                Serial.print("mati");
            }
            else
            {
                bot.sendMessage(CHAT_ID, "Lele tidak diberi makan, kondisi air jelek!");
                digitalWrite(MOTOR_RELAY, HIGH);
                Serial.print("mati");
            }
            kondisiTerpenuhi = true; // tandai kondisi terpenuhi
        }
    }
    else if (jamSekarang == 17 && menitSekarang == 1 && detik >= 1 && detik <= 59) // pakan otomatis 3
    {
        if (!kondisiTerpenuhi || (jamSebelumnya != jamSekarang || menitSebelumnya != menitSekarang))
        {
            // Jalankan loop jika kondisi belum terpenuhi sebelumnya
            // atau jika kondisi terpenuhi tetapi nilai jam atau menit berubah
            if (suhuDS18B20 >= 26 && ph >= 6)
            {
                waktuTerakhirMakan = String(jam) + ":" + String(menit) + ":" + String(detik);
                digitalWrite(MOTOR_RELAY, LOW);
                Serial.print("nyala");
                delay(30000);
                digitalWrite(MOTOR_RELAY, HIGH);
                Serial.print("mati");
            }
            else
            {
                bot.sendMessage(CHAT_ID, "Lele tidak diberi makan, kondisi air jelek!");
                digitalWrite(MOTOR_RELAY, HIGH);
                Serial.print("mati");
            }
            kondisiTerpenuhi = true; // tandai kondisi terpenuhi
        }
    }
    else if (jamSekarang == 21 && menitSekarang == 30 && detik >= 1 && detik <= 59) // pakan otomatis 4
    {
        if (!kondisiTerpenuhi || (jamSebelumnya != jamSekarang || menitSebelumnya != menitSekarang))
        {
            // Jalankan loop jika kondisi belum terpenuhi sebelumnya
            // atau jika kondisi terpenuhi tetapi nilai jam atau menit berubah
            if (suhuDS18B20 >= 26 && ph >= 6)
            {
                waktuTerakhirMakan = String(jam) + ":" + String(menit) + ":" + String(detik);
                digitalWrite(MOTOR_RELAY, LOW);
                Serial.print("nyala");
                delay(5000);
                digitalWrite(MOTOR_RELAY, HIGH);
                Serial.print("mati");
            }
            else
            {
                bot.sendMessage(CHAT_ID, "Lele tidak diberi makan, kondisi air jelek!");
                digitalWrite(MOTOR_RELAY, HIGH);
                Serial.print("mati");
            }
            kondisiTerpenuhi = true; // tandai kondisi terpenuhi
        }
    }
    else if (jamSekarang == 10 && menitSekarang == 5 && detik >= 1 && detik <= 59 && jadwal >= 1) // pakan otomatis jadwal = 1
    {
        if (!kondisiTerpenuhi || (jamSebelumnya != jamSekarang || menitSebelumnya != menitSekarang))
        {
            // Jalankan loop jika kondisi belum terpenuhi sebelumnya
            // atau jika kondisi terpenuhi tetapi nilai jam atau menit berubah
            if (suhuDS18B20 >= 26 && ph >= 6)
            {
                waktuTerakhirMakan = String(jam) + ":" + String(menit) + ":" + String(detik);
                digitalWrite(MOTOR_RELAY, LOW);
                Serial.print("nyala");
                delay(5000);
                digitalWrite(MOTOR_RELAY, HIGH);
                Serial.print("mati");
            }
            else
            {
                bot.sendMessage(CHAT_ID, "Lele tidak diberi makan, kondisi air jelek!");
                digitalWrite(MOTOR_RELAY, HIGH);
                Serial.print("mati");
            }
            kondisiTerpenuhi = true; // tandai kondisi terpenuhi
        }
    }
    else if (jamSekarang == 15 && menitSekarang == 5 && detik >= 1 && detik <= 59 && jadwal >= 1) // pakan otomatis jadwal = 1
    {
        if (!kondisiTerpenuhi || (jamSebelumnya != jamSekarang || menitSebelumnya != menitSekarang))
        {
            // Jalankan loop jika kondisi belum terpenuhi sebelumnya
            // atau jika kondisi terpenuhi tetapi nilai jam atau menit berubah
            if (suhuDS18B20 >= 26 && ph >= 6)
            {
                waktuTerakhirMakan = String(jam) + ":" + String(menit) + ":" + String(detik);
                digitalWrite(MOTOR_RELAY, LOW);
                Serial.print("nyala");
                delay(5000);
                digitalWrite(MOTOR_RELAY, HIGH);
                Serial.print("mati");
            }
            else
            {
                bot.sendMessage(CHAT_ID, "Lele tidak diberi makan, kondisi air jelek!");
                digitalWrite(MOTOR_RELAY, HIGH);
                Serial.print("mati");
            }
            kondisiTerpenuhi = true; // tandai kondisi terpenuhi
        }
    }
    else if (jamSekarang == 13 && menitSekarang == 30 && detik >= 1 && detik <= 59 && jadwal >= 2) // pakan otomatis jadwal = 2
    {
        if (!kondisiTerpenuhi || (jamSebelumnya != jamSekarang || menitSebelumnya != menitSekarang))
        {
            // Jalankan loop jika kondisi belum terpenuhi sebelumnya
            // atau jika kondisi terpenuhi tetapi nilai jam atau menit berubah
            if (suhuDS18B20 >= 26 && ph >= 6)
            {
                waktuTerakhirMakan = String(jam) + ":" + String(menit) + ":" + String(detik);
                digitalWrite(MOTOR_RELAY, LOW);
                Serial.print("nyala");
                delay(5000);
                digitalWrite(MOTOR_RELAY, HIGH);
                Serial.print("mati");
            }
            else
            {
                bot.sendMessage(CHAT_ID, "Lele tidak diberi makan, kondisi air jelek!");
                digitalWrite(MOTOR_RELAY, HIGH);
                Serial.print("mati");
            }
            kondisiTerpenuhi = true; // tandai kondisi terpenuhi
        }
    }
    else if (jamSekarang == 19 && menitSekarang == 30 && detik >= 1 && detik <= 59 && jadwal >= 2) // pakan otomatis jadwal = 2
    {
        if (!kondisiTerpenuhi || (jamSebelumnya != jamSekarang || menitSebelumnya != menitSekarang))
        {
            // Jalankan loop jika kondisi belum terpenuhi sebelumnya
            // atau jika kondisi terpenuhi tetapi nilai jam atau menit berubah
            if (suhuDS18B20 >= 26 && ph >= 6)
            {
                waktuTerakhirMakan = String(jam) + ":" + String(menit) + ":" + String(detik);
                digitalWrite(MOTOR_RELAY, LOW);
                Serial.print("nyala");
                delay(5000);
                digitalWrite(MOTOR_RELAY, HIGH);
                Serial.print("mati");
            }
            else
            {
                bot.sendMessage(CHAT_ID, "Lele tidak diberi makan, kondisi air jelek!");
                digitalWrite(MOTOR_RELAY, HIGH);
                Serial.print("mati");
            }
            kondisiTerpenuhi = true; // tandai kondisi terpenuhi
        }
    }
    else
    {
        kondisiTerpenuhi = false; // reset kondisi jika tidak terpenuhi
    }

    // Catat nilai jam dan menit saat ini
    jamSebelumnya = jamSekarang;
    menitSebelumnya = menitSekarang;

    if (millis() > lastTimeBotRan + botRequestDelay)
    {
        int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

        while (numNewMessages)
        {
            Serial.println("got response");
            handleNewMessages(numNewMessages);
            numNewMessages = bot.getUpdates(bot.last_message_received + 1);
        }
        lastTimeBotRan = millis();
    }
}