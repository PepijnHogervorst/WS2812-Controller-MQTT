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
void reconnect();
void keep_alive();
void decipher_message(char* topic, char payload[]);
void set_manual_color();
//----------------------


//----- GLOBALS -----
// doc for JSON parsing, needs memorypool to store json data
StaticJsonDocument<256> doc_receive;
StaticJsonDocument<256> doc_send;

// Ledstrip object to control WS2812B
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, LEDSTRIP_PIN, NEO_RGB  + NEO_KHZ800);

// Ethernet config arduino!
byte mac[] = { 0xD1, 0xAD, 0xBE, 0xEF, 0xCE, 0xAE };
IPAddress ip(192, 168, 178, 50);
IPAddress server(192, 168, 178, 2);
EthernetClient ethClient;
PubSubClient mqtt_client(ethClient);
//-------------------


//----- INITIALISATION -----
void setup() 
{
  // Init serial
  Serial.begin(115200);
  
  // Print compile date
  Serial.println(__FILE__);
  Serial.println(compile_date);
  Serial.println();

  // Initialize MQTT config
  mqtt_client.setServer(server, 1883);
  mqtt_client.setCallback(callback);
  
  // Start ethernet on static IP
  Ethernet.begin(mac, ip);
  
  // Let hardware sort stuff out 
  delay(1500);

  // Init topics with IO number
}
//--------------------------


void loop() {
  if (!mqtt_client.connected()) {
    reconnect();
  }
  mqtt_client.loop();
}


//----- EVENTS CALLBACKS -----
// MQTT callback when message on subscribed topic is received
void callback(char* topic, byte* payload, unsigned int length) 
{
  // Cast payload to string
  char content[length] = "";
  char character;
  for (int num = 0; num < length; num++) 
  {
    //character = payload[num];
    content[num] = payload[num];
  }

  Serial.println("Received msg!");
  Serial.print("Topic: ");
  Serial.println(topic);
  Serial.print("Payload: ");
  Serial.println(content);

  // Parse payload
  decipher_message(topic, content);
}
//----------------------------

//----- FUNCTIONS -----
void reconnect() 
{
  // Loop until we're reconnected
  while (!mqtt_client.connected()) {
    // Attempt to connect
    if (mqtt_client.connect("arduinoClient")) 
    {
      Serial.println("Connected to MQTT broker!");
      
      // Once connected, resubscribe to topics:
      mqtt_client.subscribe("LED/1/Mode");
      mqtt_client.subscribe("LED/1/Manual/#");
      
      // Publish status / keep alive
      mqtt_client.publish("LED/1/Status", "Alive");
    } else {
      Serial.print("failed to connect to mqtt broker, rc=");
      Serial.print(mqtt_client.state());
      
      // Wait 3 seconds before retrying
      delay(3000);
    }
  }
}

void keep_alive()
{

}

void decipher_message(char* topic, char payload[])
{
  // Deserialize json
  DeserializationError exception = deserializeJson(doc_receive, payload);
  
  // Check for succes of deserialization
  if (exception)
  {
    Serial.println("Failed to deserialize JSON");
    return;
  }

  // Compare topic!
  uint8_t result = strcmp(topic, "LED/1/Manual/Color");
  if (result == 0)
  {
    set_manual_color();
  }
  Serial.print("strcmp(str1, str2) = ");
  Serial.println(result);
  
}

// Make sure doc_receive is deserialized properly!
void set_manual_color()
{
  uint8_t red = doc_receive["Red"];
  uint8_t green = doc_receive["Green"];
  uint8_t blue = doc_receive["Blue"];

  Serial.println("RGB values: ");
  Serial.print("Red: ");
  Serial.println(red);
  Serial.print("Green: ");
  Serial.println(green);
  Serial.print("Blue: ");
  Serial.println(blue);
}
//---------------------
