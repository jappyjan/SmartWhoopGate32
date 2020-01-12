#include <Arduino.h>
#include <config.h>
#include <System.h>
#include <WiFi.h>
#include <esp_https_ota.h>
#include <PubSubClient.h>

PersistentData *System::_persistentData;

WiFiClient System::wifiClient;
PubSubClient System::mqttClient(System::wifiClient);

String System::espid;
String System::cmdTopic;
String System::statusTopic;

void System::init(PersistentData *persistentData)
{
    espid = persistentData->espid;
    cmdTopic = "Gates/" + espid + "/cmd";
    statusTopic = "Gates/" + espid + "/status";
    mqttClient.setServer(persistentData->networks[1].mqtt, 1883);
}

void System::setup_wifi(PersistentData *persistentData)
{
    // We start by connecting to a WiFi network
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(persistentData->networks[1].ssid);
    Serial.print("with password ");
    Serial.println(persistentData->networks[1].pass);

    WiFi.begin(persistentData->networks[1].ssid, persistentData->networks[1].pass);
    //WiFi.begin(char *ssid, char *pass);

    int counter = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
        if (counter > 20) esp_restart();
        delay(500);
        Serial.print(".");
        counter++;
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void System::reconnect()
{
    // Loop until we're reconnected
    while (!mqttClient.connected())
    {
        Serial.print("Attempting MQTT connection...");
        // Attempt to connect
        String clientName = espid + "_Client";
        if (mqttClient.connect(clientName.c_str()))
        {
            Serial.println("connected");
            // Subscribe
            mqttClient.subscribe("Gates");
            Serial.println("subscribed to: Gates");
            mqttClient.subscribe(System::cmdTopic.c_str());
            Serial.print("subscribed to: ");
            Serial.println(System::cmdTopic);
            mqttClient.publish(System::statusTopic.c_str(), "Ready to Receive");
            Serial.print("published to: ");
            Serial.println(System::statusTopic);
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

esp_err_t System::do_firmware_upgrade(const char *url, const char *cert)
{
    Serial.println("downloading and installing new firmware ...");
    mqttClient.publish("jryesp32/output", "Starting Update");

    esp_http_client_config_t config = {};
    config.url = url;
    config.cert_pem = cert;

    esp_err_t ret = esp_https_ota(&config);

    if (ret == ESP_OK)
    {
        Serial.println("OTA OK, restarting...");
        mqttClient.publish("jryesp32/output", "Update Done");
        delay(1000);
        esp_restart();
    }
    else
    {
        Serial.println("OTA failed...");
        return ESP_FAIL;
    }
    return ESP_OK;
}