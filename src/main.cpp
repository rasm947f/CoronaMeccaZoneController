#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include "M5Stack.h"
#include <M5_ENV.h>
#include <Adafruit_BMP280.h>

// Env Sensors
SHT3X sht30;

// Clients
WiFiClient espClient;
PubSubClient client(espClient);

// Configure the name and password of the connected wifi and your MQTT Serve host.
const char *ssid = "CoronaMecca";
const char *password = "ckebab1234";
const char *mqtt_server = "192.168.1.114";
const char *mqtt_username = "client";
const char *mqtt_pass = "test";
const char *mqtt_topic = "test";

// Variables
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
#define ANN_BUFFER_SIZE (17)
char announcement[ANN_BUFFER_SIZE];
char msg[MSG_BUFFER_SIZE];
char display_temp[MSG_BUFFER_SIZE];
char display_humi[MSG_BUFFER_SIZE];
bool running;
int zone_id = 1;

// FUNCTIONS
// Wifi
void setupWifi();
void getEnvData();

// MQTT pubsubClient client-connect function
void connect();

// ENV Sensor data
float temp = 0.0;
float humi = 0.0;

void setup()
{
    // Setup
    M5.begin();
    M5.Power.begin();
    M5.Lcd.setTextDatum(MC_DATUM); // Set text alignment to center
    M5.lcd.setTextSize(3);
    M5.Lcd.printf("Zone %d", zone_id);

    // Wifi and MQTT
    M5.lcd.setTextSize(1);   // Change text size for messages
    M5.lcd.setCursor(0, 40); // Move cursor futher down
    setupWifi();
    client.setServer(mqtt_server, 1883); // Sets the server details.

    // Wire for sensors
    Wire.begin(); // Wire init, adding the I2C bus.

    // Connect to client
    connect();

    // Default message
    M5.lcd.setTextSize(2); // Change text size
    M5.Lcd.println("Press 'Left' to send data");
}

void loop()
{
    // M5.Lcd.clear(); // Clear the screen.
    if (!client.connected())
    {
        connect();
    }

    client.loop(); // This function is called periodically to allow clients to
                   // process incoming messages and maintain connections to the
                   // server.

    unsigned long now = millis(); // Obtain the host startup duration.

    M5.update(); // Check the status of the buttons.
    if (M5.BtnA.wasPressed())
    {
        if (!running && lastMsg == 0)
        {
            M5.lcd.fillRect(0, 30, 320, 190, BLACK);        // Fill the screen with black (to clear the screen).
            M5.Lcd.drawString("Temperature:", 140, 110, 2); // Display temperature on the controller
            M5.Lcd.drawString("Humidity:", 150, 140, 2);    // Display humidity on the controller
            
            getEnvData(); // Get environmentdata and display it
        }
        
        if (!running)
        {
            running = true;
            M5.lcd.fillRect(170, 0, 140, 20, BLACK); // Clear running/not running state
            M5.Lcd.drawString("Running", 263, 0, 1); // Display state on the controller
        }
        else
        {
            running = false;
            M5.Lcd.drawString("Not Running", 240, 0, 1); // Display state on the controller
        }
    }

    if (running)
    {
        if (now - lastMsg > 10000)
        {
            getEnvData(); // Get environmentdata and display it

            lastMsg = now; // Set the last message to current time.

            client.publish(mqtt_topic, msg); // Publishes a message to the specified topic.
        }
    }

    if (M5.BtnB.wasPressed())
    {
        M5.lcd.fillRect(220, 100, 80, 30, BLACK); // Fill the screen with black (to clear the temperature).
        M5.lcd.fillRect(210, 130, 60, 30, BLACK); // Fill the screen with black (to clear the humidity).

        M5.Lcd.drawString("-18.6", 260, 110, 2); // Display temperature on the controller
        M5.Lcd.drawString("49%", 240, 140, 2);   // Display humidity on the controller
    }
}

void setupWifi()
{
    delay(10);
    M5.Lcd.printf("Connecting to %s", ssid);
    WiFi.mode(WIFI_STA);        // Set the mode to WiFi station mode.
    WiFi.begin(ssid, password); // Start Wifi connection.

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        M5.Lcd.print(".");
    }
    M5.Lcd.printf("\nSuccess\n");
}

void connect()
{
    while (!client.connected())
    {
        M5.Lcd.print("Attempting MQTT connection...");
        // Create a random client ID.
        String clientId = "M5Stack-";
        clientId += String(random(0xffff), HEX);
        // Attempt to connect.
        if (client.connect(clientId.c_str(), "client", "pass"))
        {
            M5.Lcd.printf("\nSuccess\n");
            // Once connected, publish an announcement to the topic.
            snprintf(announcement, ANN_BUFFER_SIZE, "Zone %d connected", zone_id); // Format to the specified string and store it in a variable.

            client.publish(mqtt_topic, announcement);
        }
        else
        {
            M5.Lcd.print("failed, rc=");
            M5.Lcd.print(client.state());
            M5.Lcd.println("try again in 5 seconds");
            delay(5000);
        }
    }
}

void getEnvData()
{
    if (sht30.get() == 0)
    {                       // Obtain the data of SHT30.
        temp = sht30.cTemp; // Store the temperature obtained from SHT30.

        humi = sht30.humidity; // Store the humidity obtained from the SHT30.
    }
    else
    {
        temp = 0, humi = 0;
    }

    snprintf(msg, MSG_BUFFER_SIZE, "{Temperature: %2.1f, Humidity: %2.0f%%}", temp, humi); // Format to the specified string and store it in MSG.
    snprintf(display_temp, MSG_BUFFER_SIZE, "%2.1f", temp);                                // Format string for displaying temperature on the controller
    snprintf(display_humi, MSG_BUFFER_SIZE, "%2.0f%%", humi);                              // Format string for displaying humidity on the controller

    M5.lcd.fillRect(220, 100, 80, 30, BLACK); // Fill the screen with black (to clear the temperature).
    M5.lcd.fillRect(210, 130, 60, 30, BLACK); // Fill the screen with black (to clear the humidity).

    // M5.lcd.setCursor(0, 100);
    M5.Lcd.drawString(display_temp, 265, 110, 2); // Display temperature on the controller
    M5.Lcd.drawString(display_humi, 240, 140, 2); // Display humidity on the controller
}