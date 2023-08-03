#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <map>

#define WIFI_SSID "brisa-2591743" //"TP-Link_2206" //
#define WIFI_PASSWORD "3vijhau1"  //"36103423" //
#define DATABASE_URL "https://esp32-indoor-ble-default-rtdb.firebaseio.com/"
#define API_KEY "AIzaSyAbEpZ-A8Pvjko8v0Oy_Smz7QRF9zokc00"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

std::map<char, double> beaconRSSIs = {
    {'A', 0},
    {'B', 0},
    {'C', 0},
    {'D', 0}};
std::map<char, String> beaconMACs = {
    {'A', "ea:f7:7b:60:97:4e"},
    {'B', "c4:01:12:14:28:8a"},
    {'C', "fc:15:e2:04:45:8b"},
    {'D', "e7:66:37:f2:0c:a4"}};

BLEScan *pBLEScan;
float scanTime = 1;
void setup()
{
    Serial.begin(115200);
    Serial.println("Scanning...");
    BLEDevice::init("");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(300);
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();

    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;

    if (Firebase.signUp(&config, &auth, "", ""))
    {
        Serial.println("signUp OK!");
        signupOK = true;
    }
    else
    {
        Serial.printf("%s\n", config.signer.signupError.message.c_str());
    }
    config.token_status_callback = tokenStatusCallback;
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
        String macDispositivoEncontrado = advertisedDevice.getAddress().toString().c_str();

        for (const auto &kvp : beaconMACs)
        {
            if (macDispositivoEncontrado == kvp.second)
            {
                beaconRSSIs[kvp.first] = advertisedDevice.getRSSI();
            }
        } // kvp chave-value-pair destructured  kvp.first 'A' ...'D' \ kvp.second == MAC endereço
    }
};

void scanBLE()
{

    BLEScan *pBLEScan = BLEDevice::getScan(); // create new scan
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true); // active scan uses more power, but get results faster
    BLEScanResults foundDevices = pBLEScan->start(scanTime);
}

void uploadToDatabase()
{
    Serial.println("Firebase" + Firebase.ready());
    Serial.println("signupOk" + signupOK);
    if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 5000 || sendDataPrevMillis == 0))
    {
        Serial.println("Entrou 2");
        sendDataPrevMillis = millis();
        for (const auto &entry : beaconRSSIs)
        {
            char beaconName = entry.first;
            double beaconValue = entry.second;

            String path = "Beacon/" + String(beaconName);
            if (Firebase.RTDB.setDouble(&fbdo, path.c_str(), beaconValue))
            {
                Serial.println("Successfully saved to : " + path);
            }
            else
            {
                Serial.print("FAILED to save at " + path + " : " + fbdo.errorReason());
            }
        }
    }
}

void loop()
{
    if (Serial.available() > 0)
    {
        String msg = Serial.readStringUntil('\n');
        if (msg.equals("Upload"))
        {
            Serial.println("Entrou");
            scanBLE();
            uploadToDatabase();
            delay(100);
        }
    }
}

