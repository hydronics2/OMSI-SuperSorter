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
//  1.5  Simplified communication protocol with Master 
//  1.6  9/14/2015 Thomas Hudson. added RFID tag information describing the new tags
//  1.7  9/16/2015 thomas hudson. took out second while loop when communicating with Master
//
//////////////////////////////////////////////////////////////////////

#include <SoftwareSerial.h>
// set DEBUG flag to enable Serial.print output
//const int BIN_NO = 1;            // Unique Bin Number 1 (1-5) toxic and electronics
//const int BIN_NO = 2;            // Unique Bin Numbe 2 (1-5) compost
//const int BIN_NO = 3;            // Unique Bin Number 3 (1-5) recycling
//const int BIN_NO = 4;            // Unique Bin Number 4 (1-5) glass recycling
const int BIN_NO = 5;            // Unique Bin Number 5 (1-5) garbage

const boolean DEBUG = true;
const boolean COMM_DEBUG = true;

// Register your RFID tags here
// GOOD TAGS
char tag_1a[13] = "720077762E5D";  //toxics and electronics - paint
char tag_1b[13] = "3D001ED5EE18"; //toxics and electronics - batteries
char tag_1c[13] = "7200774A7B34"; //toxics and electronics - cfl lights
char tag_1d[13] = "720077187469";  //toxics and electronics - LCD TV
char tag_1e[13] = "72007738417C";  //toxic(remote recycling center) plastic bags

char tag_2a[13] = "72007765A3C3";  //compost - peaches
char tag_2b[13] = "72007759E7BB";  //comost - grass clippings
char tag_2c[13] = "72007726E6C5";  // compost - leaves
char tag_2d[13] = "720077510A5E"; // compost - brussel sprouts
char tag_2e[13] = "3D001ED89E65";

char tag_3a[13] = "720077526B3C";  //recycling - tin cans
char tag_3b[13] = "72007742B7F0";  //recycling - yogurt
char tag_3c[13] = "7200771B6B75";  //recycling - garden containers
char tag_3d[13] = "72007731AA9E";  //recyling - paper food stuffs
char tag_3e[13] = "3D001EE434F3"; //recyling - card board newspapers

char tag_4a[13] = "7200777A5B24";  // glas recycling
char tag_4b[13] = "720077277250";
char tag_4c[13] = "72007718829F";
char tag_4d[13] = "720077514C18";
char tag_4e[13] = "72007750C590";

char tag_5a[13] = "3D001EDADA23";  // garbage - food container
char tag_5b[13] = "7200771DBEA6";  // garbage - light bulb
char tag_5c[13] = "3D001EE8A962";  // gargage - broken mirror
char tag_5d[13] = "3D001ED755A1";  //  garbage - butter container
char tag_5e[13] = "720077159383";  // garbage - broken bottle

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
    }else 
   
      { // if wrong, do this
      turnLedRed();
      bad++;
      if (DEBUG) printScore();
   // }else{                                                      // if unknown tag, do this
      // print out any unknown tag
      if (DEBUG) {
        Serial.print("wrong ");
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
    }else 
    {  // if wrong, do this
      turnLedRed();
      bad++;
      if (DEBUG) printScore();
                                                        // if unknown tag, do this
      // print out any unknown tag
      if (DEBUG) {
        Serial.print("wrong ");
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
    }else 
     {  // if wrong, do this
      turnLedRed();
      bad++;
      if (DEBUG) printScore();
    //}else{                                                      // if unknown tag, do this
      // print out any unknown tag
      if (DEBUG) {
        Serial.print("wrong ");
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
    }else 
     {   // if wrong, do this
      turnLedRed();
      bad++;
      if (DEBUG) printScore();
    //}else{                                                      // if unknown tag, do this
      // print out any unknown tag
      if (DEBUG) {
        Serial.print("wrong ");
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
    }else 
      {  // if wrong, do this
      turnLedRed();
      bad++;
      if (DEBUG) printScore();
    //}else{                                // if unknown tag, do this
      // print out any unknown tag
      if (DEBUG) {
        Serial.print("wrong ");
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
  delay(10);
  digitalWrite(MISO_PIN, LOW);            // Notify master READY

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
  
