//////////////////////////////////////////////////////////////////////
//  RFID_Slave Counter Program                                      //
//////////////////////////////////////////////////////////////////////
//  This is a piece of code that lets you read out the              //
//  ID-2 or ID-12 or ID-20 RFID tag reader with an Arduino.         //
//  The reader uses a 9600 baud serial connection with the Arduino. //
//////////////////////////////////////////////////////////////////////
// The RFID tags output 16 bytes:                                   //
//    [start of text, STX(02h)]                                     //
//    [12 bytes of ID, hex]                                         //
//    [new line, LF(0Ah)]                                           //
//    [carriage return, CR(0Dh)]                                    //
//    [end of text, ETX(03h)]                                       //
// We only need/want the 12 bytes that make up the tag.             //
// But we will use the rest to determine the tag's start and end.   //
//////////////////////////////////////////////////////////////////////
// Unknown RFID tags will printout for your use.                    //
// Copy it and add it to the list.                                  //
//////////////////////////////////////////////////////////////////////
// Requires any Arduino board                                       //
// Pins used are MISO / MOSI    10,11                               //
//               Red LED        12                                  //
//               Green LED      13                                  //
//               RFID input     0 (serial RX)                       //
//////////////////////////////////////////////////////////////////////
//  Revision History:                                               //
//  1.4  Working Version                                            //
//  1.5  Simplified communication protocol with Master              //
//////////////////////////////////////////////////////////////////////

#include <SoftwareSerial.h>
// set DEBUG flag to enable Serial.print output
const int BIN_NO = 5;            // Unique Bin Number (1-5)

const boolean DEBUG = true;
const boolean COMM_DEBUG = true;

// Register your RFID tags here
// GOOD TAGS
char tag_1a[13] = "3D001ED58274";
char tag_1b[13] = "3D001ED5EE18";
char tag_1c[13] = "3D001EE211D0";
char tag_1d[13] = "3D001ED601F4";
char tag_1e[13] = "3D001EE7EF2B";

char tag_2a[13] = "3D001EDD3DC3";
char tag_2b[13] = "3D001EE421E6";
char tag_2c[13] = "3D001ED5A452";
char tag_2d[13] = "3D001EE47CBB";
char tag_2e[13] = "3D001ED89E65";

char tag_3a[13] = "3D001EDFEC10";
char tag_3b[13] = "3D001ED8ED16";
char tag_3c[13] = "3D001EDC1DE2";
char tag_3d[13] = "3D001EE39D5D";
char tag_3e[13] = "3D001EDE3FC2";

char tag_4a[13] = "3D001EE434F3";
char tag_4b[13] = "3D001EDEB24F";
char tag_4c[13] = "3D001EE2D312";
char tag_4d[13] = "3D001EE8A962";
char tag_4e[13] = "3D001EDD2ED0";

char tag_5a[13] = "3D001EDADA23";
char tag_5b[13] = "3D001EDC3EC1";
char tag_5c[13] = "3D001EDB827A";
char tag_5d[13] = "3D001ED755A1";
char tag_5e[13] = "3D001EE373B3";

// Pins used are MOSI 11, MISO 10.
/* MISO Master In Slave Out pin */
const int  MISO_PIN = 10;
/* MOSI Master Out Slave In pin */
const int  MOSI_PIN = 11;

const int  RedLedPin   = 12;    // the number of the Red LED pin
const int  GreenLedPin = 13;    // the number of the Green LED pin
const int  interval    = 750;   // duration for LED ON state (milliseconds)

// Global Variables
int good = 0;
int bad = 0;
int GreenLedState = LOW;        // ledState used to set the LED
int RedLedState = LOW;          // ledState used to set the LED
unsigned long turnoff;          // time in milliseconds to turn OFF LEDs
                                

void setup() {
  Serial.begin(9600);           // connect to the serial port

  // set the digital pins for SoftwareSerial port:
  pinMode(MISO_PIN, OUTPUT);    // slave output (tx)
  pinMode(MOSI_PIN, INPUT);     // slave input (rx)
  
  // set the digital pins as output:
  pinMode(RedLedPin,   OUTPUT);      
  pinMode(GreenLedPin, OUTPUT);      

  digitalWrite(GreenLedPin, LOW);     // off
  digitalWrite(RedLedPin,  LOW);      // off
  digitalWrite(MISO_PIN,   LOW);      // idle
  digitalWrite(MOSI_PIN,  HIGH);      // activate internal pull-down resistor

  turnoff = 1;                  // initialized to turn OFF LEDs initialy
  
  if (DEBUG) Serial.println("RFID Checker v1.5");
  
}

void loop () {

  char    tagString[13] = {0};
  int     index = 0;
  int     readByte = 0; 
  boolean reading = false;

  if(Serial.available() > 0) {              // check for anything in serial buffer
    readByte = Serial.read(); 
    if(readByte == 2) reading = true;       //begining of tag
    while(reading) { 
      if(Serial.available() > 0) {          // check for anything in serial buffer
        readByte = Serial.read(); 
        if(readByte == 3) reading = false;  //end of tag
        // ignore header and stop bytes 
        if(readByte != 0x02 && readByte != 0x0A && readByte != 0x0D) {
          //store the tag
          tagString[index] = readByte;
          index ++;
        }
      }
    }

    if (DEBUG) printTag(tagString);         //Display tag for debug
    checkTag(tagString);                    //Check if it is a match
    clearTag(tagString);
  }
  if(millis() > turnoff) turnLedOff();
}

void checkTag(char tag[]){
///////////////////////////////////
//Check the read tag against known tags
///////////////////////////////////

  if(tag[0] == 0) return;              //empty, no need to contunue
//  if(strlen(tag) == 0) return;       //does not work

switch (BIN_NO) {
  case 1:
    if(compareTag(tag, tag_1a) || compareTag(tag, tag_1b) || compareTag(tag, tag_1c) || compareTag(tag, tag_1d) || compareTag(tag, tag_1e)) { // if correct, do this
      turnLedGreen();
      good++;
      if (DEBUG) printScore();
      sendResults(good);
    }else if(compareTag(tag, tag_2a) || compareTag(tag, tag_2b) || compareTag(tag, tag_2c) || compareTag(tag, tag_2d) || compareTag(tag, tag_2e) 
      || compareTag(tag, tag_3a) || compareTag(tag, tag_3b) || compareTag(tag, tag_3c) || compareTag(tag, tag_3d) || compareTag(tag, tag_3e) 
      || compareTag(tag, tag_4a) || compareTag(tag, tag_4b) || compareTag(tag, tag_4c) || compareTag(tag, tag_4d) || compareTag(tag, tag_4e) 
      || compareTag(tag, tag_5a) || compareTag(tag, tag_5b) || compareTag(tag, tag_5c) || compareTag(tag, tag_5d) || compareTag(tag, tag_5e)) { // if wrong, do this
      turnLedRed();
      bad++;
      if (DEBUG) printScore();
    }else{                                                      // if unknown tag, do this
      // print out any unknown tag
      if (DEBUG) {
        Serial.print("UNKNOWN ");
        printTag(tag);
      }
    }
    break;
  
  case 2:
    if(compareTag(tag, tag_2a) || compareTag(tag, tag_2b) || compareTag(tag, tag_2c) || compareTag(tag, tag_2d)
      || compareTag(tag, tag_2e)) {                            // if correct, do this
      turnLedGreen();
      good++;
      if (DEBUG) printScore();
      sendResults(good);
    }else if(compareTag(tag, tag_1a) || compareTag(tag, tag_1b) || compareTag(tag, tag_1c) || compareTag(tag, tag_1d) || compareTag(tag, tag_1e) 
      || compareTag(tag, tag_3a) || compareTag(tag, tag_3b) || compareTag(tag, tag_3c) || compareTag(tag, tag_3d) || compareTag(tag, tag_3e) 
      || compareTag(tag, tag_4a) || compareTag(tag, tag_4b) || compareTag(tag, tag_4c) || compareTag(tag, tag_4d) || compareTag(tag, tag_4e) 
      || compareTag(tag, tag_5a) || compareTag(tag, tag_5b) || compareTag(tag, tag_5c) || compareTag(tag, tag_5d) || compareTag(tag, tag_5e)) {  // if wrong, do this
      turnLedRed();
      bad++;
      if (DEBUG) printScore();
    }else{                                                      // if unknown tag, do this
      // print out any unknown tag
      if (DEBUG) {
        Serial.print("UNKNOWN ");
        printTag(tag);
      }
    }
    break;
  
  case 3:
    if(compareTag(tag, tag_3a) || compareTag(tag, tag_3b) || compareTag(tag, tag_3c) || compareTag(tag, tag_3d) || compareTag(tag, tag_3e)) { // if correct, do this
      turnLedGreen();
      good++;
      if (DEBUG) printScore();
      sendResults(good);
    }else if(compareTag(tag, tag_1a) || compareTag(tag, tag_1b) || compareTag(tag, tag_1c) || compareTag(tag, tag_1d) || compareTag(tag, tag_1e) 
      || compareTag(tag, tag_2a) || compareTag(tag, tag_2b) || compareTag(tag, tag_2c) || compareTag(tag, tag_2d) || compareTag(tag, tag_2e) 
      || compareTag(tag, tag_4a) || compareTag(tag, tag_4b) || compareTag(tag, tag_4c) || compareTag(tag, tag_4d) || compareTag(tag, tag_4e) 
      || compareTag(tag, tag_5a) || compareTag(tag, tag_5b) || compareTag(tag, tag_5c) || compareTag(tag, tag_5d) || compareTag(tag, tag_5e)) {  // if wrong, do this
      turnLedRed();
      bad++;
      if (DEBUG) printScore();
    }else{                                                      // if unknown tag, do this
      // print out any unknown tag
      if (DEBUG) {
        Serial.print("UNKNOWN ");
        printTag(tag);
      }
    }
    break;
  
  case 4:
    if(compareTag(tag, tag_4a) || compareTag(tag, tag_4b) || compareTag(tag, tag_4c) || compareTag(tag, tag_4d) || compareTag(tag, tag_4e)) { // if correct, do this
      turnLedGreen();
      good++;
      if (DEBUG) printScore();
      sendResults(good);
    }else if(compareTag(tag, tag_1a) || compareTag(tag, tag_1b) || compareTag(tag, tag_1c) || compareTag(tag, tag_1d) || compareTag(tag, tag_1e) 
      || compareTag(tag, tag_2a) || compareTag(tag, tag_2b) || compareTag(tag, tag_2c) || compareTag(tag, tag_2d) || compareTag(tag, tag_2e) 
      || compareTag(tag, tag_3a) || compareTag(tag, tag_3b) || compareTag(tag, tag_3c) || compareTag(tag, tag_3d) || compareTag(tag, tag_3e) 
      || compareTag(tag, tag_5a) || compareTag(tag, tag_5b) || compareTag(tag, tag_5c) || compareTag(tag, tag_5d) || compareTag(tag, tag_5e)) {  // if wrong, do this
      turnLedRed();
      bad++;
      if (DEBUG) printScore();
    }else{                                                      // if unknown tag, do this
      // print out any unknown tag
      if (DEBUG) {
        Serial.print("UNKNOWN ");
        printTag(tag);
      }
    }
    break;
  
  case 5:
    if(compareTag(tag, tag_5a) || compareTag(tag, tag_5b) || compareTag(tag, tag_5c) || compareTag(tag, tag_5d) || compareTag(tag, tag_5e)) { // if correct, do this
      turnLedGreen();
      good++;
      if (DEBUG) printScore();
      sendResults(good);
    }else if(compareTag(tag, tag_1a) || compareTag(tag, tag_1b) || compareTag(tag, tag_1c) || compareTag(tag, tag_1d) || compareTag(tag, tag_1e) 
      || compareTag(tag, tag_2a) || compareTag(tag, tag_2b) || compareTag(tag, tag_2c) || compareTag(tag, tag_2d) || compareTag(tag, tag_2e) 
      || compareTag(tag, tag_3a) || compareTag(tag, tag_3b) || compareTag(tag, tag_3c) || compareTag(tag, tag_3d) || compareTag(tag, tag_3e) 
      || compareTag(tag, tag_4a) || compareTag(tag, tag_4b) || compareTag(tag, tag_4c) || compareTag(tag, tag_4d) || compareTag(tag, tag_4e)) {  // if wrong, do this
      turnLedRed();
      bad++;
      if (DEBUG) printScore();
    }else{                                // if unknown tag, do this
      // print out any unknown tag
      if (DEBUG) {
        Serial.print("UNKNOWN ");
        printTag(tag);
      }
    }
    break;
  
  default:
    turnLedGreen();                       // if bad bin number . . .
    turnLedRed();                                                 
  
  }
}

void printTag(char tag[]) {
  if(tag[0] == 0) return;                 //empty, no need to contunue
//  if(strlen(tag) == 0) return;          //does not work

  Serial.print("Tag: ");
  for(int i = 0; i < 12; i++){
    Serial.print(tag[i]);
  }
  Serial.println("");                     //print out any unknown tag
}

void printScore() {
  if (DEBUG) {
    Serial.print("Scoreboard:  correct: ");
    Serial.print(good, DEC);
    Serial.print(" wrong: ");
    Serial.println(bad, DEC);
  }
}

void sendResults(int results) {

  digitalWrite(MISO_PIN, HIGH);           // Notify master of pending data
  if (COMM_DEBUG) Serial.println("1 MISO set HIGH, waiting for MASTER to query"); 
  while (digitalRead(MOSI_PIN) == LOW) {
    if(millis() > turnoff) {              // timeout if Master not running
      turnLedOff();
      break;
    }
  }             // wait for ACK from master
  // MOSI will go HIGH when master accesses
  digitalWrite(MISO_PIN, HIGH);           // again for safety
  if (COMM_DEBUG) Serial.println("2 MOSI queried HIGH -- MISO set LOW"); 
  digitalWrite(MISO_PIN, LOW);            // Notify master READY
  while (digitalRead(MOSI_PIN) == HIGH) { // wait for ACK from master
    // MOSI will go LOW when master ACK
    if (COMM_DEBUG) Serial.println("3 waiting MOSI LOW (ack)"); 
    if(millis() > turnoff) {              // timeout if Master not running
      turnLedOff();
      break;
    }
  }  
  digitalWrite(MISO_PIN, LOW);            // again for safety

/*  // R1.5 simplified communication protocol
  //while data in buffer . . . 
  // send first byte
  if (COMM_DEBUG) Serial.println("4 ACK received, MISO sending DATA HIGH"); 
  digitalWrite(MISO_PIN, HIGH);           // Set pin HIGH or LOW for data bit
  // now wait for ACK
  while (digitalRead(MOSI_PIN) == LOW) {  // wait for master processing
    // MOSI will go HIGH when master ACK
    if (COMM_DEBUG) Serial.println("5 waiting MOSI HIGH (ack)"); 
    // set it again for safety 
    digitalWrite(MISO_PIN, HIGH);         // Set pin HIGH or LOW for data bit
    if(millis() > turnoff) {              // timeout if Master not running
      turnLedOff();
      break;
    }
  }  
  digitalWrite(MISO_PIN, HIGH);           // again for safety
  // MOSI will go HIGH when master has received data
  if (COMM_DEBUG) Serial.println("6 MOSI ACKd our DATA HIGH"); 
  // . . . no more data to send

  // No more data to send; sleep
  digitalWrite(MISO_PIN, LOW);            // Set resting state
  if (COMM_DEBUG) Serial.println("7 MISO resting LOW (idle)"); 
*/
}

void turnLedGreen() {
  digitalWrite(GreenLedPin, HIGH);        // set the LED ON
  digitalWrite(RedLedPin, LOW);           // set the LED OFF
  turnoff = millis() + interval;          // set turnoff time
}

void turnLedRed() {
  digitalWrite(GreenLedPin, LOW);         // set the LED ON
  digitalWrite(RedLedPin, HIGH);          // set the LED OFF
  turnoff = millis() + interval;          // set turnoff time
}

void turnLedOff() {
    digitalWrite(GreenLedPin, LOW);       // set the LEDs OFF
    digitalWrite(RedLedPin, LOW);
}

void clearTag(char one[]){
///////////////////////////////////
//clear the char array by filling with null - ASCII 0
//Will think same tag has been read otherwise
///////////////////////////////////
  for(int i = 0; i < 13; i++){
    one[i] = 0;
  }
}

boolean compareTag(char one[], char two[]){
///////////////////////////////////
//compare two value to see if same,
//strcmp not working 100% so we do this
///////////////////////////////////

  if(one[0] == 0) return false; //empty, no need to contunue
//  if(strlen(one) == 0) return false; //empty

  for(int i = 0; i < 12; i++){
    if(one[i] != two[i]) return false;
  }

  return true; //no mismatches
}
  
