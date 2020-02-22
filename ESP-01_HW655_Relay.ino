/* HW655 relay module with ESP-01S modules with 1MB of flash

   S.Manzer November 2018 added OTA Dec 2018

   Don't use the 512K library as the WiFi will not work.

   Warning: Use a power supply with sufficient current to handle this device.  Do not use
   USB interface that is powering other ESP's or it will not be stable.

   Note there are no Serial.print debug commands as the ESP will be plugged into a daughter board and
   it communicates with another processor over serial to have it control the relay.

*/
#include <ESP8266WiFi.h>    // WiFi device programmed with Arduino IDE
#include <PubSubClient.h>   // MQTT protocol
#define MANUFACTURER        "GENERIC"
#define DEVICE              "HW655_RELAY"
#define RELAY_PROVIDER      "RELAY_PROVIDER_HW655"

// Added OTA: https://randomnerdtutorials.com/esp8266-ota-updates-with-arduino-ide-over-the-air/
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

/******************************* CUSTOMIZE ******************************************/
// wifi config
const char* ssid = "YOUR_SSD_HERE";
const char* password = "YOUR_WIFI_PASSWORD_HERE";

// Static IP Parameters
//define STATIC_IP false
#define STATIC_IP true
const boolean static_ip = STATIC_IP;
// Change to suit your network
#define MY_IP_ADDRESS 192,168,1,24
#define GATEWAY 192,168,1,1
#define SUBNET 255,255,255,0

IPAddress ip(MY_IP_ADDRESS);
IPAddress gateway(GATEWAY);
IPAddress subnet(SUBNET);

// Setup the MQTT credentials (set to NULL if no user/password)
const char *MQTT_UserName = "YOUR_MQTT_USER_HERE";
const char *MQTT_Password = "YOUR_MQTT_PASS_HERE";

// Setup MQTT Server and topic change to suit your network
const char *MQTT_Name = "Single_Relay1";
const char *MQTT_Server = "192.168.1.65";  // Address of my server on my network, substitute yours!
const char *MQTT_inTopic = "ha/Relay_1/action";   // message can either be ON, OFF, or TOGGLE
const char *MQTT_stateTopic = "ha/Relay_1/state"; // First sends available, then echos the new state of the relay
const char *MQTT_availabilityTopic = "ha/Relay_1/availability"; // First sends available, then echos the new state of the relay


String newMessage;                        // MQTT message payload
const char* lwtMessage = "offline";
const char* birthMessage = "online";
/******************************* CUSTOMIZE ******************************************/

WiFiClient espClient;
PubSubClient client(espClient);

// Commands to the ST processor to turn the relay on or off
byte relayON[] = {0xA0, 0x01, 0x01, 0xA2};  //Hex command to send to serial for open relay
byte relayOFF[] = {0xA0, 0x01, 0x00, 0xA1}; //Hex command to send to serial for close relay

void setup() {
  Serial.begin(9600);  // HW-655 requires 9600
  delay(10);
  Serial.println("Starting HW655 Relay...");
  setup_wifi();
 
  client.setServer(MQTT_Server, 1883);
  client.setCallback(callback);
}
// Wifi setup function

void setup_wifi() {
  delay(10);
  Serial.print("Connecting to ");
  Serial.print(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  if (static_ip) {
    WiFi.config(ip, gateway, subnet);
  }
 while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }


  ArduinoOTA.onStart([]() {
    Serial.println("OTA Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\OTA End");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println(WiFi.localIP());
}
// This is where the MQTT topic and message (payload) is processed.
void callback(char* topic, byte* payload, unsigned int length) {  // Receive a message on MQTT
  static int RelayState=0;

  //convert topic to string to make it easier to work with
  String topicStr = topic; 
  // Note:  the "topic" value gets overwritten everytime it receives confirmation (callback) message from MQTT

  // Terminate the byte array so we can process it
  payload[length] = '\0';
    
  // Look at the topic and if it is an inbound topic examine its payload to determine 
  // what we are supposed to do.  Valid payloads are ON, OFF, TOGGLE
  if (topicStr == MQTT_inTopic) { // Then process the request to turn on or off the relay:
      newMessage = String((char*)payload);
      if (newMessage=="ON") {
        Serial.write(relayON, sizeof(relayON));   // turns the relay ON
        client.publish(MQTT_stateTopic, "ON");
        RelayState=1;
        }
      if (newMessage=="OFF") {
        Serial.write(relayOFF, sizeof(relayOFF));   // turns the relay OFF
        client.publish(MQTT_stateTopic, "OFF");
        RelayState=0;
        }
      if (newMessage=="TOGGLE") {
         if (RelayState == 0) { // Turn it on
            Serial.write(relayON, sizeof(relayON));   // turns the relay ON
            client.publish(MQTT_stateTopic, "ON");
            RelayState=1;
            }
          else { // Turn it off
            Serial.write(relayOFF, sizeof(relayOFF));   // turns the relay OFF
            client.publish(MQTT_stateTopic, "OFF");            
            RelayState=0;
          }
        }    
      client.publish(MQTT_stateTopic, "online");   
      delay(1000);
      }
  }


void reconnect() {  //  Woops....reconnect MQTT client
  delay(1000);
  // Loop until we're reconnected
  while (!client.connected()) {
    // Attempt to connect
    // If you do not want to use a username and password, change next line to
    // if (client.connect("ESP8266Client")) {
    if (client.connect(MQTT_Name, MQTT_UserName, MQTT_Password, MQTT_availabilityTopic, 0, true, lwtMessage)) {
      // Once connected, publish an announcement...
      client.publish(MQTT_stateTopic, "online");
      // ... and resubscribe
      client.subscribe(MQTT_inTopic);
    } else {
      // Wait a second before retrying
      delay(100);
    }
   yield();
  }  
}

void loop() {

  ArduinoOTA.handle();

  // Connect to MQTT
  if (!client.connected()) {
    reconnect();
  }
  ESP.wdtFeed();
  client.loop();
  delay(10);
  yield();
}
