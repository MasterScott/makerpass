#include <ESP8266WiFi.h>
#include "Arduino.h"

// Define wiegand format
#define WIEGAND_LEN  26

int onePin = 12;
int zeroPin = 14;

volatile unsigned char rfid_data[WIEGAND_LEN];
volatile int rfid_idx;

// Wifi credentials
const char *ssid = "FabLab";
const char *passwd = "welovemaking";

// TCP endpoint
const char *host = "192.168.1.163";
const int port = 8000;

// Machine ID
const int machine_id = 5001;

// Global client
WiFiClient client;

// Setup code
void setup() {

  // Setup serial
  Serial.begin (115200);
  Serial.println ("ESP8266 RFID Reader");

  // Connect to server
  WiFi.begin (ssid, passwd);

  // Wait until we are connected
  Serial.print ("Connecting");
  while (WiFi.status () != WL_CONNECTED) {
    delay (500);
    Serial.print (".");
  }
  Serial.println ("");
  Serial.print ("Connected: ");
  Serial.println (WiFi.localIP ());
  
  // Set index to 0
  rfid_idx = 0;
  
  // Attach interrupt to pins
  attachInterrupt (zeroPin, data0, FALLING);
  attachInterrupt (onePin, data1, FALLING);
}


int decode_id (void)
{
  int i;
  unsigned char parity;
  unsigned int id = 0;
  
  // Start with parity bit
  parity = rfid_data[0];
  
  // Check parity
  for (i = 1; i < WIEGAND_LEN / 2; i++) {
    parity ^= rfid_data[i];
    id <<= 1;
    if (rfid_data[i])
      id |= 1;
  }
  
  // Check even parity
  if (parity)
    return -1;

  //Start with parity bit
  parity = rfid_data[WIEGAND_LEN - 1];

  for (i = WIEGAND_LEN / 2; i < WIEGAND_LEN - 1; i++) {
    parity ^= rfid_data[i];
    id <<= 1;
    if (rfid_data[i])
      id |= 1;
  }

  // Check odd parity
  if (parity == 0)
    return -1;
    
  return id;
}

void loop() {
  int id;
  
  if (rfid_idx == WIEGAND_LEN) {

    // Decode ID
    id = decode_id ();
    if (id == -1) {
      Serial.println ("Bad parity");
      delay (100);
      goto next_id;
    }
    else {

      // Print out to serial
      Serial.println (String ("User ID: ") + (id & 0xffff));
      
      // Make connection to host
      if (!client.connect (host, port)) {
        Serial.println ("Failed to connect to host!");
        goto next_id;
      }

      // Send id to host
      client.print (String ("Machine=") + machine_id + "&User=" + (id & 0xffff) + "\r\n");
      delay (10);

      // Get Response
      /*
      while (client.available ()) {
        String line = client.readStringUntil ('\r');
        Serial.println (line);
      }
      */
    }

  next_id:
    // Attach interrupts
    attachInterrupt (zeroPin, data0, FALLING);
    attachInterrupt (onePin, data1, FALLING);

    // Reset index
    rfid_idx = 0;
  }
}

// ISR routines
void data0 (void)
{
  // Save bit
  rfid_data[rfid_idx++] = 0;

  // Detach ISRs when complete
  if (rfid_idx == WIEGAND_LEN) {
    detachInterrupt (zeroPin);
    detachInterrupt (onePin);
  }
}

void data1 (void)
{
  // Save bit
  rfid_data[rfid_idx++] = 1;

  // Detach ISRs when complete
  if (rfid_idx == WIEGAND_LEN) {
    detachInterrupt (zeroPin);
    detachInterrupt (onePin);
  }
}

