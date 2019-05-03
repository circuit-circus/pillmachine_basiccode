/* MTG PILLMACHINE MASTER
moja & circuitcircus
15.05.2017: Påbegyndt moja
15.05: Ethernet pastet ind (men ikke tilrettet)
16.05: Ethernet virker (men submitter blot en dummy til debug)
10.07: Tilrettelse af formatering og forberedelse på Github
01.08: Userval="" case skipper submit via ethernet (f.eks. hvis brugeren ikke har sat et stik i Hvor På Kroppen)
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
// Define machine individual includes here
#include <Servo.h>
#include <FastLED.h>

#define maskinNR 4 //FOR AT VI VED HVILKEN STATION DER SUBMITTER

#define SS_PIN 8 // SDA for RFID
#define RST_PIN 9 // RST
#define pausetid 10
#define RFIDLED 2 // LEDLIGHT BEHIND RFID TAG ---- brug pin 0 eller 1 i endelig version

static uint8_t mac[] = {  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEF };
static uint8_t myip[] = {  10, 0, 0, 109 };
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
Servo myservo;
int potVal;
int potPin = 3;
int servoPin = A1;
#define LEDPIN 5
#define NUM_LEDS 1
CRGB leds[NUM_LEDS];

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  mfrc522.PCD_SetRegisterBitMask(mfrc522.RFCfgReg, (0x07<<4)); // Enhance the MFRC522 Receiver Gain to maximum value of some 48 dB
  pinMode(RFIDLED, OUTPUT);
  Ethernet.begin(mac, myip);
  delay(5000); // wait for ethernetcard
  aktivateEthernetSPI(false);

  // Machine individual setups go here
  myservo.attach(servoPin);
  FastLED.addLeds<NEOPIXEL, LEDPIN>(leds, NUM_LEDS);
  FastLED.clear();
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

  if(val != "") {
    aktivateEthernetSPI(true);

    //String datastring= "GET /start_notepad_exe.php HTTP/1.0";
    //String datastring= "GET /arduino.php?val="+String(val)+" HTTP/1.0";

    // IF DEV:
    // String datastring="GET /DEV/pillmachine/machine//setval.php?tag="+String(cardID)+"&maskine="+String(maskinNR)+"&val="+val+" HTTP/1.0";
    // IF PRODUCTION:
    String datastring="GET /machine//setval.php?tag="+String(cardID)+"&maskine="+String(maskinNR)+"&val="+val+" HTTP/1.0";

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
  resetData();
}

void aktivateEthernetSPI(boolean x) {
  // mfrc522.PICC_HaltA();
  // skift SPI/Slave... turn RFID shield off, ethernet on (LOW=on)
  // http://tronixstuff.com/2011/05/13/tutorial-arduino-and-the-spi-bus/

  digitalWrite(SS_PIN,x);
  digitalWrite(10,!x);
}

void resetData() {
  userval="";
  // Reset your variables here
  leds[0] = CRGB::Black;
  FastLED.show();
}

// this is where the user interface is responsive
void UI() {
  /* analog input = A0, A1, A2, A3, A4, A5 (all)
  * pwm output = D3, D5, D5
  * digital I/O = D0, D1, D4 + (D3, D5, D5)
  */
  leds[0] = CRGB::White;
  FastLED.show();

  potVal = analogRead(potPin);
  userval = String(map(potVal, 0, 1023, 0, 255));
  potVal = map(potVal, 0, 1024, 1211, 1644);
  myservo.writeMicroseconds(potVal);

  // HUSK AT SÆTTE userval="" HVIS MASKINEN IKKE ER SAT TIL NOGET
}

// Custom functions for individual machines go here
