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
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif
#include <ResponsiveAnalogRead.h>

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
// Which pin on the Arduino is connected to the two NeoPixel strands?
#define FREQ_PIN 6
#define ACTIVITY_PIN 5

// How many NeoPixels are attached to the Arduino?
#define FREQ_NUMPIXELS 40
#define ACTIVITY_NUMPIXELS 8

Adafruit_NeoPixel freq_pixels = Adafruit_NeoPixel(FREQ_NUMPIXELS, FREQ_PIN, NEO_GRB + NEO_KHZ400);
Adafruit_NeoPixel activity_pixels = Adafruit_NeoPixel(ACTIVITY_NUMPIXELS, ACTIVITY_PIN, NEO_GRB + NEO_KHZ400);

int NO_OF_ROWS = 8;
int NO_OF_COLUMNS = 5;

const int sliderPin = A0;
int sliderVal;
ResponsiveAnalogRead analog(sliderPin, false);

int activeRow;
int lastActiveRow;

int encoderPin1 = 2;
int encoderPin2 = 3;

volatile int lastEncoded = 0;
volatile long encoderValue = 0;

// How big steps should the encoder move the LED columns in?
int mapSensitivity = 40;
int blinky = 50;

int activeColumn = 0;

int pixelMatrix[8][5] = {
  {0, 1, 2, 3, 4},
  {5, 6, 7, 8, 9},
  {10, 11, 12, 13, 14},
  {15, 16, 17, 18, 19},
  {20, 21, 22, 23, 24},
  {25, 26, 27, 28, 29},
  {30, 31, 32, 33, 34},
  {35, 36, 37, 38, 39}
};

int savedVals[8] = {"", "", "", "", "", "", "", ""};
int savedEncoderVals[8] = {"", "", "", "", "", "", "", ""};

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  mfrc522.PCD_SetRegisterBitMask(mfrc522.RFCfgReg, (0x07<<4)); // Enhance the MFRC522 Receiver Gain to maximum value of some 48 dB
  pinMode(RFIDLED, OUTPUT);
  Ethernet.begin(mac, myip);
  delay(500); // wait for ethernetcard
  aktivateEthernetSPI(false);

  // Machine individual setups go here
  freq_pixels.begin(); // This initializes the NeoPixel library.
  activity_pixels.begin(); // This initializes the NeoPixel library.

  freq_pixels.clear();
  activity_pixels.clear();

  pinMode(encoderPin1, INPUT);
  pinMode(encoderPin2, INPUT);

  digitalWrite(encoderPin1, HIGH); //turn pullup resistor on
  digitalWrite(encoderPin2, HIGH); //turn pullup resistor on

  attachInterrupt(0, updateEncoder, CHANGE);
  attachInterrupt(1, updateEncoder, CHANGE);
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
    String datastring="GET /DEV/pillmachine/machine//setval.php?tag="+String(cardID)+"&maskine="+String(maskinNR)+"&val="+val+" HTTP/1.0";
    // IF PRODUCTION:
    // String datastring="GET /machine//setval.php?tag="+String(cardID)+"&maskine="+String(maskinNR)+"&val="+val+" HTTP/1.0";

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
  freq_pixels.clear();
  activity_pixels.clear();
  for(int i = 0; i < NO_OF_ROWS; i++) {
    savedVals[i] = "";
    savedEncoderVals[i] = "";
  }
  activeRow = 0;
  lastActiveRow = -1;
  activeColumn = 0;
  encoderValue = 0;
}

// this is where the user interface is responsive
void UI() {
  /* analog input = A0, A1, A2, A3, A4, A5 (all)
  * pwm output = D3, D5, D5
  * digital I/O = D0, D1, D4 + (D3, D5, D5)
  */

  if(userval == "") {
    freq_pixels.clear();
    activity_pixels.clear();
  }

  analog.update();
  sliderVal = analog.getValue();
  // Slider is wired in reverse, so we must subtract from NO_OF_ROWS
  activeRow = NO_OF_ROWS - map(sliderVal, 0, 1024, 0, NO_OF_ROWS) - 1;

  if(activeRow != lastActiveRow) { // If we have changed rows
    // Saving the values for later
    savedVals[lastActiveRow] = pixelMatrix[lastActiveRow][activeColumn];
    savedEncoderVals[lastActiveRow] = encoderValue;

    // If we already have a saved value, jump to that
    if(savedVals[activeRow] != "") {
      activeColumn = savedVals[activeRow];
      encoderValue = savedEncoderVals[activeRow];
    }
    // Else, we're on zero
    else {
      encoderValue = 0;
      activeColumn = 0;
    }

    // Turn off all freqPixels on the row we left but leave the chosen column turned on
    int lastRowStart = lastActiveRow * NO_OF_COLUMNS;
    for(int i = lastRowStart; i < lastRowStart + NO_OF_COLUMNS; b++) {
      if(i != savedVals[lastActiveRow]) {
        freq_pixels.setPixelColor(i, freq_pixels.Color(0, 0, 0));
      }
      else {
        freq_pixels.setPixelColor(i, freq_pixels.Color(0, 0, 50));
      }
    }
  }

  activeColumn = map(encoderValue, 0, mapSensitivity + 1, 0, NO_OF_COLUMNS);
  lastActiveRow = activeRow;

  // Turn off all activity_pixels except the one on activeRow
  for(int j = 0; j < NO_OF_ROWS; j++) {
    if(j != activeRow) {
      activity_pixels.setPixelColor(j, activity_pixels.Color(0, 0, 0));
    }
    else {
      activity_pixels.setPixelColor(j, activity_pixels.Color(50, 50, 50));
    }
  }

  // Turn off all freq_pixels on activeRow except the currently active one
  int rowStart = activeRow * NO_OF_COLUMNS;
  for(int k = rowStart; k < rowStart + NO_OF_COLUMNS; k++) {
    if(k != rowStart + activeColumn) {
      freq_pixels.setPixelColor(k, freq_pixels.Color(0, 0, 0));
    }
    else {
      freq_pixels.setPixelColor(k, freq_pixels.Color(50, 50, 50));
    }
  }
  
  freq_pixels.show();
  activity_pixels.show();

  userval = "";
  for(int l = 0; l < NO_OF_ROWS; l++) {
    // Make sure number is between 0 and 4
    userval += (savedVals[l] % 5);
    // Add a comma for all except the final row
    if(l != NO_OF_ROWS - 1) {
      userval += ",";
    }
  }

  // HUSK AT SÆTTE userval="" HVIS MASKINEN IKKE ER SAT TIL NOGET
}

// Custom functions for individual machines go here
void updateEncoder() {
  int MSB = digitalRead(encoderPin1); //MSB = most significant bit
  int LSB = digitalRead(encoderPin2); //LSB = least significant bit

  int encoded = (MSB << 1) | LSB; //converting the 2 pin value to single number
  int sum  = (lastEncoded << 2) | encoded; //adding it to the previous encoded value

  if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) encoderValue ++;
  if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) encoderValue --;

  lastEncoded = encoded; //store this value for next time

  encoderValue = constrain(encoderValue, 0, mapSensitivity);
}