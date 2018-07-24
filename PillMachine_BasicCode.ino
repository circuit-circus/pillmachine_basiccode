/* MTG PILLMACHINE MASTER
  moja & circuitcircus
  15.05.2017: Påbegyndt moja
  15.05: Ethernet pastet ind (men ikke tilrettet)
  16.05: Ethernet virker (men submitter blot en dummy til debug)
  10.07: Tilrettelse af formatering og forberedelse på Github
  01.08: Userval="" case skipper submit via ethernet (f.eks. hvis brugeren ikke har sat et stik i Hvor På Kroppen)
*/

/* CONNECTIONS: 522 RFID --------
   RFID SDA  -> 8
   RFID SCK  -> 13
   RFID MOSI -> 11
   RFID MISO -> 12
   RFID IRQ  -> ikke forbundet
   RFID GND  -> gnd
   RFID RST  -> 9
   RFID 3.3V -> 3.3V

   Hvid LED  -> 2

   -----------------------------
*/

#include <SPI.h>
#include <MFRC522.h>
#include <Ethernet.h>
// Define machine individual includes here
#include <Adafruit_NeoPixel.h>


#define maskinNR 5 //FOR AT VI VED HVILKEN STATION DER SUBMITTER

#define SS_PIN 8 // SDA for RFID
#define RST_PIN 9 // RST
#define pausetid 10
#define RFIDLED 2 // LEDLIGHT BEHIND RFID TAG ---- brug pin 0 eller 1 i endelig version

static uint8_t mac[] = {  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEF };
static uint8_t myip[] = {  10, 0, 0, 105 };
IPAddress pc_server(10, 0, 0, 31); // serverens adress

boolean cardPresent = false; // DEBUG: Set this and isDebugging to true to test UI
boolean isDebugging = false; // DEBUG: Set this and cardPresent to true to test UI
boolean cardPresent_old = false;
String cardID = ""; // NB skal muligvis laves til char-array for at spare memory
String cardID_old = "";

String userval = "";

MFRC522 mfrc522(SS_PIN, RST_PIN);
EthernetClient client;

// Define machine individual variables here

/*
 * 
 * This is the Layout that both LEDs and Buttons follow
 * 
 *        -------
 *   2 o -       - o 3
 *       |       |  
 *   1 o -       - o 4
 *       |       |
 *   0 o -       - o 5
 *         ....
 *        PINS IN
*/

#define LENGTH 6

const int buttonPins[LENGTH] = {4, 5, A2, A3, A4, A5};
int buttonStates[LENGTH] = {LOW, LOW, LOW, LOW, LOW, LOW};
int lastButtonStates[LENGTH] = {LOW, LOW, LOW, LOW, LOW, LOW};

unsigned long lastDebounceTimes[LENGTH] = {0, 0, 0, 0, 0, 0};
unsigned long debounceDelay = 50;

const int numberOfLeds = 6;
const int ledPin = 6;
int ledStates[LENGTH] = {LOW, LOW, LOW, LOW, LOW, LOW};


Adafruit_NeoPixel leds = Adafruit_NeoPixel(numberOfLeds, ledPin, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  pinMode(RFIDLED, OUTPUT);
  Ethernet.begin(mac, myip);
  delay(5000); // wait for ethernetcard
  aktivateEthernetSPI(false);

  // Machine individual setups go here
  
  // Initiate LEDs
  leds.begin();
  leds.clear();
  leds.show();

  // Set pinMode on buttons
  for (int i = 0; i < LENGTH; i++) {
    pinMode(buttonPins[i], INPUT);
  }


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

  if (val != "") {
    aktivateEthernetSPI(true);

    //String datastring= "GET /start_notepad_exe.php HTTP/1.0";
    //String datastring= "GET /arduino.php?val="+String(val)+" HTTP/1.0";

    // IF DEV:
    // String datastring = "GET /DEV/pillmachine/machine//setval.php?tag=" + String(cardID) + "&maskine=" + String(maskinNR) + "&val=" + val + " HTTP/1.0";
    // IF PRODUCTION:
    String datastring="GET /machine//setval.php?tag="+String(cardID)+"&maskine="+String(maskinNR)+"&val="+val+" HTTP/1.0";

    Serial.println(datastring);

    if (client.connect(pc_server, 80)) {
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

  digitalWrite(SS_PIN, x);
  digitalWrite(10, !x);
}

void resetData() {
  userval = "";
  // Reset your variables here

  for(int i = 0; i < LENGTH; i++) {
    buttonStates[i] = LOW;
    lastButtonStates[i] = LOW;
    lastDebounceTimes[i] = 0;
    ledStates[i] = LOW;
  }

  leds.clear();
  leds.show();
}

// this is where the user interface is responsive
void UI() {
 
  // Read buttonstates - using the debounce method (see Arduino examples --> 02. Digital --> Debounce)
  for (int i = 0; i < LENGTH; i++) {
    int reading = digitalRead(buttonPins[i]);

    if(isDebugging) {
      Serial.print(i);
      Serial.print(": ");
      Serial.println(reading);
    }

    if(reading != lastButtonStates[i]) {
      lastDebounceTimes[i] = millis();
    }


    if((millis() - lastDebounceTimes[i]) > debounceDelay) {
      if(reading != buttonStates[i]) {
        buttonStates[i] = reading;

        if(buttonStates[i] == HIGH) {
          ledStates[i] = !ledStates[i];
        }
      }
    }

    // Set LEDs accordingly
    uint32_t c = ledStates[i] == HIGH ? leds.Color(45, 45, 45) : leds.Color(0, 0, 0);
    leds.setPixelColor(i, c);
    leds.show();

    lastButtonStates[i] = reading;


  }


  userval = String(ledStates[0], DEC) + "," + String(ledStates[1], DEC) + "," + String(ledStates[2], DEC) + "," + String(ledStates[3], DEC) + "," + String(ledStates[4], DEC) + "," + String(ledStates[5], DEC);
  

  // HUSK AT SÆTTE userval="" HVIS MASKINEN IKKE ER SAT TIL NOGET
}

// Custom functions for individual machines go here
