//----- INCLUDES -----
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <SPI.h>
//--------------------


//----- DEFINES -----
// Arduino pin used to control the leds
#define LEDSTRIP_PIN      5
// Number of leds on the ledstrip
#define NUMPIXELS         100
//-------------------


//----- CONSTANTS -----
const char compile_date[] = __DATE__ " " __TIME__;
//---------------------


//----- PROTOTYPES -----
void callback(char* topic, byte* payload, unsigned int length);
//----------------------


//----- GLOBALS -----
// doc for JSON parsing, needs memorypool to store json data
StaticJsonDocument<256> doc;

// Ledstrip object to control WS2812B
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, LEDSTRIP_PIN, NEO_RGB  + NEO_KHZ800);

// Ethernet config arduino!
byte mac[] = { 0xD1, 0xAD, 0xBE, 0xEF, 0xCE, 0xAE };
IPAddress ip(192, 168, 68, 56);
IPAddress server(192, 168, 68, 119);

EthernetClient ethClient;
PubSubClient mqtt_client(server, 1883, callback, ethClient);
//-------------------


//----- INITIALISATION -----
void setup() {
  // Init serial
  Serial.begin(115200);
  
  // Print compile date
  Serial.println(__FILE__);
  Serial.println(compile_date);
  Serial.println();

  // Initialize Ethernet and MQTT
  // Try DHCP
  Ethernet.begin(mac, ip);
  
  if (mqtt_client.connect("arduinoClient", "", ""))
  {
    Serial.print("Connected to MQTT broker: ");
    Serial.print(server);
    Serial.println("! :)");

    mqtt_client.subscribe("LED/1/Mode");
    mqtt_client.subscribe("LED/1/");

    mqtt_client.publish("Test", "Arduino is alive");
  }
  else
  {
    Serial.print("Failed to connect to MQTT broker: ");
    Serial.println(server);
  }

  // Example JSON parsing
  char json[] = "{\"red\":255,\"green\":128,\"blue\":0}";

  // Deserialize json using doc
  DeserializationError exception = deserializeJson(doc, json);

  // Check if success
  if (exception)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(exception.c_str());
    return;
  }

  uint8_t red = doc["red"];
  uint8_t green = doc["green"];
  uint8_t blue = doc["blue"];

  Serial.println("RGB values: ");
  Serial.print("Red: ");
  Serial.println(red);
  Serial.print("Green: ");
  Serial.println(green);
  Serial.print("Blue: ");
  Serial.println(blue);
}
//--------------------------


void loop() {
    mqtt_client.loop();
}


//----- EVENTS CALLBACKS -----
// MQTT callback when message on subscribed topic is received
void callback(char* topic, byte* payload, unsigned int length) {
  
}
//----------------------------
