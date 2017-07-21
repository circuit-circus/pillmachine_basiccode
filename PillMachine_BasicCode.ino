/* MTG PILLMACHINE MASTER
moja & circuitcircus
15.05.2017: Påbegyndt moja
15.05: Ethernet pastet ind (men ikke tilrettet)
16.05: Ethernet virker (men submitter blot en dummy til debug)
10.07: Tilrettelse af formatering og forberedelse på Github
*/

/* CONNECTIONS: 522 RFID --------
*  RFID SDA  -> 8
*  RFID SCK  -> 13
*  RFID MOSI -> 11
*  RFID MISO -> 12
*  RFID IRQ  -> ikke forbundet
*  RFID GND  -> gnd
*  RFID RST  -> 9
*  RFID 3.3V -> 3.3V
*  
*  Hvid LED  -> 2
*  
*  -----------------------------
*/

#include <SPI.h>
#include <MFRC522.h>
#include <Ethernet.h>

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define maskinNR 1 //FOR AT VI VED HVILKEN STATION DER SUBMITTER

#define SS_PIN 8 // SDA for RFID
#define RST_PIN 9 // RST 
#define pausetid 10
#define RFIDLED 2 // LEDLIGHT BEHIND RFID TAG ---- brug pin 0 eller 1 i endelig version

static uint8_t mac[] = {  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEF };
static uint8_t myip[] = {  10, 0, 0, 100 };
IPAddress pc_server(10,0,0,31);  // serverens adress

boolean cardPresent = false; // DEBUG: Set this and isDebugging to true to test UI
boolean isDebugging = false; // DEBUG: Set this and cardPresent to true to test UI
boolean cardPresent_old = false;
String cardID = ""; // NB skal muligvis laves til char-array for at spare memory
String cardID_old = "";

String   userval ="";

MFRC522 mfrc522(SS_PIN, RST_PIN);
EthernetClient client;

// Define machine individual variables here
#define PIN 6
#define NUMPIXELS 8

int huePotPin = 2; // analog
int satPotPin = 3; // analog
int huePotVal = 0;
int satPotVal = 0;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
uint32_t stripColor = strip.Color(255, 255, 255);

/* HSB TO RGB CONVERTER
 * Adapted from https://blog.adafruit.com/2012/03/14/constant-brightness-hsb-to-rgb-algorithm/
 *  
 */
void hsb2rgb(uint16_t hue, uint8_t sat, uint8_t bright) {
  uint16_t r_temp, g_temp, b_temp;
  uint8_t index_mod;
  uint8_t inverse_sat = (sat ^ 255);

  hue = hue % 768;
  index_mod = hue % 256;

  if (hue < 256) {
    r_temp = index_mod ^ 255;
    g_temp = index_mod;
    b_temp = 0;
  }

  else if (hue < 512) {
    r_temp = 0;
    g_temp = index_mod ^ 255;
    b_temp = index_mod;
  }

  else if ( hue < 768) {
    r_temp = index_mod;
    g_temp = 0;
    b_temp = index_mod ^ 255;
  }

  else {
    r_temp = 0;
    g_temp = 0;
    b_temp = 0;
  }

  r_temp = ((r_temp * sat) / 255) + inverse_sat;
  g_temp = ((g_temp * sat) / 255) + inverse_sat;
  b_temp = ((b_temp * sat) / 255) + inverse_sat;

  r_temp = (r_temp * bright) / 255;
  g_temp = (g_temp * bright) / 255;
  b_temp = (b_temp * bright) / 255;

  stripColor = strip.Color((uint8_t)r_temp, (uint8_t)g_temp, (uint8_t)b_temp);
}

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  pinMode(RFIDLED, OUTPUT);
  Ethernet.begin(mac, myip);
  delay(5000); // wait for ethernetcard
  aktivateEthernetSPI(false);

  // Machine individual setups go here
  strip.begin();
}

void loop() {
  digitalWrite(RFIDLED, cardPresent);

  if (cardPresent) {
    // KEEP THE UI responsive-------------------her foregår alt med UI
    UI();
  }

  //---------------------------------------
  if (cardPresent) {
    if ( mfrc522.PICC_ReadCardSerial()) {
      getID();
    }
  }
  // HVOR SKAL  DETTE VÆRE HER 2 GANGE FOR AT DET VIRKER?
  if ( mfrc522.PICC_ReadCardSerial()) {
    getID();
  }
  //----------------------------------------


  // CHECK OM CARD IS BEING REMOVED --------
  if ((cardPresent == false) && (cardPresent_old == true)) {
    // her skal data submittes til server (datostamp + cardID + vars)
    submitData(  userval);
  }

  // ---------------------------------------

  cardPresent_old = cardPresent;

  // -----------------ALT HERUNDER SKAL STÅ SIDST I MAIN LOOP!

  if ( ! mfrc522.PICC_IsNewCardPresent() && !isDebugging) {
    cardPresent = false;
    return;
    delay(pausetid);
  }

  cardPresent = true;

  // mfrc522.PICC_HaltA();
  delay(pausetid);

} //main loop end

void getID() {
  String  cardIDtmp = "";
  // for (byte i = 0; i < mfrc522.uid.size; i++)
  for (byte i = 0; i < 3; i++) {
    byte tmp = (mfrc522.uid.uidByte[i]);
    cardIDtmp.concat(tmp);
  }

  // has ID changed?
  if (cardIDtmp != cardID_old) {
    cardID = cardIDtmp;
    cardID_old = cardID;
  }
}

void submitData(String val) {

  aktivateEthernetSPI(true);

  //String datastring= "GET /start_notepad_exe.php HTTP/1.0";
  //String datastring= "GET /arduino.php?val="+String(val)+" HTTP/1.0";
  String datastring= "GET /DEV/pillmachine/machine//setval.php?tag="+String(cardID)+"&maskine="+String(maskinNR)+"&val="+val+" HTTP/1.0";
  Serial.println(datastring);

  if(client.connect(pc_server,80)) {
    client.println(datastring);
    client.println("Connection: close");
    client.println(); //vigtigt at sende tom linie
    client.stop();
    delay(100);
  } 

  aktivateEthernetSPI(false);

}

void aktivateEthernetSPI(boolean x) {
  // mfrc522.PICC_HaltA();
  // skift SPI/Slave... turn RFID shield off, ethernet on (LOW=on)
  // http://tronixstuff.com/2011/05/13/tutorial-arduino-and-the-spi-bus/

  digitalWrite(SS_PIN,x);
  digitalWrite(10,!x);
}

// this is where the user interface is responsive
void UI() {
  /* analog input = A0, A1, A2, A3, A4, A5 (all)
  * pwm output = D3, D5, D5
  * digital I/O = D0, D1, D4 + (D3, D5, D5)
  */

  // Machine specific
  huePotVal = analogRead(huePotPin);
  satPotVal = analogRead(satPotPin);

  // Map hue from 0 to 767 (what the hsb2rgb takes as max)
  int hue = map(huePotVal, 0, 1024, 0, 767);
  
  // Map saturation from 150 to 254 - doing 0 to 254 makes it white for a large part of the potentiometer "rotation", we don't want that
  int saturation = map(satPotVal, 0, 1024, 150, 254);

  hsb2rgb(hue, saturation, 255); // Updates the stripColor value to match

  for(int i = 0; i < NUMPIXELS; i++) {
    strip.setPixelColor(i, stripColor);
    strip.show();
    delay(10);
  }

  userval="28,96,57,70";
}

// Custom functions for individual machines go here
