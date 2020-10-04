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
void set_mode();
void init_address_topics();
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
IPAddress server(192, 168, 178, 100);
EthernetClient ethClient;
PubSubClient mqtt_client(ethClient);

// Topics for MQTT
char topic_led_mode[30];
char topic_led_manual_color[30];
char topic_led_status[30];
char topic_led_brightness[30];

// Manual mode variables
uint8_t controller_mode = 0; // <- 0 = manual 1=auto
uint8_t manual_color_red = 0;
uint8_t manual_color_green = 0;
uint8_t manual_color_blue = 0;

// General variables
unsigned long prev_keep_alive;
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

  // Init address using input reg
  init_address_topics();
  
  // Initialize MQTT config
  mqtt_client.setServer(server, 1883);
  mqtt_client.setCallback(callback);
  
  // Start ethernet on static IP
  Ethernet.begin(mac, ip);
  
  // Let hardware sort stuff out 
  delay(1500);

  // Init ledstrip off
  pixels.setBrightness(100);
  pixels.clear();
  pixels.show();
}
//--------------------------


void loop() {
  if (!mqtt_client.connected()) {
    reconnect();
  }
  mqtt_client.loop();

  // KeepAlive message
  keep_alive();
  
  // Do program according to mode
  if (controller_mode == 0) 
  {
    // Manual mode!
    manual_mode();
  }
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
      mqtt_client.subscribe(topic_led_mode);
      mqtt_client.subscribe(topic_led_manual_color);
      
      // Publish status / keep alive
      mqtt_client.publish(topic_led_status, "Alive");
    } 
    else 
    {
      Serial.print("failed to connect to mqtt broker, rc=");
      Serial.print(mqtt_client.state());
      
      // Wait 3 seconds before retrying
      delay(3000);
    }
  }
}

void keep_alive()
{
  unsigned long current_time = millis();

  // Check time elapsed is more than 1 sec
  if ((current_time - prev_keep_alive) >= 1000)
  {
    // Publish alive message
    mqtt_client.publish(topic_led_status, "Alive");
    // Update prev time
    prev_keep_alive = current_time;
  }
}

void decipher_message(char* topic, char payload[])
{
  // Deserialize json
  DeserializationError exception = deserializeJson(doc_receive, payload);
  
  // Check for succes of deserialization
  if (exception)
  {
    Serial.println("Failed to deserialize JSON..");
    return;
  }

  // Compare topic!
  if (strcmp(topic, topic_led_manual_color) == 0)
  {
    set_manual_color();
  }
  else if (strcmp(topic, topic_led_mode) == 0)
  {
    set_mode();
  }
  else if (strcmp(topic, topic_led_brightness) == 0)
  {
    set_brightness();
  }
  
}

// Make sure doc_receive is deserialized properly!
void set_manual_color()
{
  manual_color_red    = doc_receive["Red"];
  manual_color_green  = doc_receive["Green"];
  manual_color_blue   = doc_receive["Blue"];

  Serial.println("RGB values: ");
  Serial.print("Red: ");
  Serial.println(manual_color_red);
  Serial.print("Green: ");
  Serial.println(manual_color_green);
  Serial.print("Blue: ");
  Serial.println(manual_color_blue);
}

void set_mode()
{
  const char* mode = doc_receive["Mode"];

  if (strcmp(mode, "Manual") == 0)
  {
    // Set manual mode
    Serial.println("Set mode to: MANUAL");
    controller_mode = 0;
  }
  else if (strcmp(mode, "Auto") == 0)
  {
    Serial.println("Set mode to: AUTO");
    controller_mode = 1;
  }
}

void set_brightness()
{
  uint8_t brightness = doc_receive["Brightness"];
  pixels.setBrightness(brightness);

  Serial.print("Brightness set to: ");
  Serial.println(brightness);
}

void init_address_topics()
{
  // Set register to all inputs -> 0
  DDRK = B00000000;
  
  // Read out the input port
  uint8_t portValue = PINK;
  char val[4];
  itoa(portValue, val, 10);

  // Set topic address
  String prefix = String("LED/");
  String buf;
  Serial.print("Node Address = ");
  Serial.println(val);

  buf = prefix + val + "/Mode";
  buf.toCharArray(topic_led_mode, buf.length() + 1);

  buf = prefix + val + "/Brightness";
  buf.toCharArray(topic_led_brightness, buf.length() + 1);

  buf = prefix + val + "/Manual/Color";
  buf.toCharArray(topic_led_manual_color, buf.length() + 1);

  buf = prefix + val + "/Status";
  buf.toCharArray(topic_led_status, buf.length() + 1);
}

void manual_mode()
{
  for (uint16_t i = 0; i < pixels.numPixels(); i++)
  {
    pixels.setPixelColor(i, manual_color_red, manual_color_green, manual_color_blue);
  }
  pixels.show();
}
//---------------------
