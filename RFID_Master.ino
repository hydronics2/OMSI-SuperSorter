//////////////////////////////////////////////////////////////////////
//  RFID_Master Controller Program                                  //
//////////////////////////////////////////////////////////////////////
//  This code collects scoring data from 5 slave boards and         //
//  drives the 7-segment displays for time and total score.         //
//////////////////////////////////////////////////////////////////////
// Requires  MegaArduino board to support all the 5 slave comms     //
// Pins used are MOSI / MISO          2-11                          //
//               rMP3 Audio Trigger   32, 33                        //
//               7-SEG Displays       36,38,40                      //
//               HAPP Buttons         22-26, 46,48,50               //
//               PWM for motor        13                            //
//////////////////////////////////////////////////////////////////////
//  Revision History:                                               //
//  1.3  Working version                                            //
//  1.4  Added Audio Output                                         //
//  1.5  Simplified communication protocol with Slaves
//  1.6  July 2, 2015 chaged out uMP3 player with Sparkfun WAV trigger and increased the delay HIGH for trigger from 1ms to 50ms.
//////////////////////////////////////////////////////////////////////

#include <SoftwareSerial.h>

// set DEBUG flag to enable Serial.print output
const boolean DEBUG = true;
const boolean COMM_DEBUG = true;
const boolean COMM_DEBUG2 = false;

const int  MISO_PIN[5] = {2, 4, 6, 8, 10};  // MISO Master In Slave Out pins
const int  MOSI_PIN[5] = {3, 5, 7, 9, 11};  // MOSI Master Out Slave In pins
const int  SLAVE_DELAY = 50;                // time delay for slave response, in milliseconds

// set up audio output triggers (R1.4)
const int  audioTrigger[2]    = {32, 33};   // rMP3 input triggers

// set up digital output displays
const int  displayStrobe      = 36;
const int  displayClock       = 40;
const int  displayData        = 38;

// set up HAPP output LEDs
const int  buttonStrobe2      = 46;
const int  buttonClock2       = 48;
const int  buttonData2        = 50;

// set up HAPP button inputs and outputs
const int  STOP_BTN           = 22;
const int  SPEED_LOW_BTN      = 24;
const int  SPEED_MED_BTN      = 26;
const int  SPEED_HI_BTN       = 28;
const byte STOP_LED           = B00000001;      // via A6821 OUT1
const byte SPEED_LOW_LED      = B00000010;      // via A6821 OUT2
const byte SPEED_MED_LED      = B00000100;      // via A6821 OUT3
const byte SPEED_HIGH_LED     = B00001000;      // via A6821 OUT4
const byte SPEED_ALL_LED      = B00001110;      // via A6821 all ON
const byte SPEED_OFF_LED      = B00000000;      // via A6821 all OFF

// PWM MOSFET connected to digital pin 13 (built-in LED)
const int  MotorPWMpin        = 13;
// Define PWM Speeds (0-255)
const int  pwmSTOP            = 0;
const int  pwmLOW             = 45;     // any slower and it stalls
const int  pwmMED             = 70;
const int  pwmHIGH            = 100;

// Define maximum time allowed, in seconds
const int maxTime             = 30;     // maximum starting time (in seconds)
const int idleTime            = 30;     // maximum idle time (in seconds)
const int deadTime            = 100;    // extra audio startup time (in milliseconds)

// MSB Lookup array for 7-segment digital displays
//const byte segPattern0[10] = {0xBD, 0x88, 0x3E, 0xAE, 0x8B, 0xA7, 0xB7, 0x8C, 0xBF, 0x8F}; // '0' - '9'
//const byte segPattern1[10] = {0x00, 0x88, 0x3E, 0xAE, 0x8B, 0xA7, 0xB7, 0x8C, 0xBF, 0x8F}; // suppress leading zero
const byte segPattern0[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F}; // '0' - '9'
const byte segPattern1[10] = {0x00, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F}; // suppress leading zero
// order is:   score1, score0, clock1, clock0

// MSB Lookup array for HAPP Button LEDs
const byte LEDPattern[5] = {0x00, 0x01, 0x02, 0x04, 0x08};
// order is:   off, stop, low, med, high

// Global variables that will change:
boolean runFlag;                        // true if running
boolean sleepFlag;                      // true if sleeping
int speed;                              // PWM setting (0-255)
int score;                              // number of correct answers (0-99)
int lastScore;                          // last score displayed

// everything dealing with millis() needs to be declared unsigned long or negative time results
unsigned long startTime;                // start time, in milliseconds
unsigned long sleepTimer;               // idle timeout in milliseconds
unsigned long expireTime;               // when time expires, in milliseconds
unsigned long remainTime;               // remaining time in seconds (60-0)
unsigned long lastTime;                 // last time displayed

/////////////////////////////////////////////////////////////////////////////////////////////////


void setup() {

  runFlag = false;                      // not running
  sleepFlag = false;                    // not sleeping
  speed = pwmSTOP;                      // PWM setting (0-255)
  score = 0;                            // reset score
  lastScore = 0;
  remainTime = maxTime;

  Serial.begin(9600);                   // connect to the serial port

  // set the digital pins for all 5 Serial ports (0-4):
  for(int i = 0; i < 5; i++) {
    pinMode(MOSI_PIN[i], OUTPUT);       // master output (tx)
    pinMode(MISO_PIN[i], INPUT);        // master input (rx)
    digitalWrite(MISO_PIN[i], HIGH);    // turn ON internal pull-up resistors
    digitalWrite(MOSI_PIN[i], LOW);     // Initial WAIT state
    digitalWrite(MISO_PIN[i], LOW);     // restore IDLE state
  }

// set the audio trigger pins as output:
  for(int i = 0; i < 2; i++) {
    pinMode(audioTrigger[i], OUTPUT);
    digitalWrite(audioTrigger[i], HIGH);
  }
  
// set the digital display pins as output:
  pinMode(displayData,   OUTPUT);      
  pinMode(displayClock,  OUTPUT);      
  pinMode(displayStrobe, OUTPUT);      

  digitalWrite(displayData,   LOW);     // off
  digitalWrite(displayClock,  LOW);     // idle
  digitalWrite(displayStrobe, LOW);     // idle

  // set the HAPP LED pins as output:
  pinMode(buttonData2,   OUTPUT);      
  pinMode(buttonClock2,  OUTPUT);      
  pinMode(buttonStrobe2, OUTPUT);      

  // set up HAPP button inputs and outputs
  pinMode(STOP_BTN,       INPUT);
  pinMode(SPEED_LOW_BTN,  INPUT);
  pinMode(SPEED_MED_BTN,  INPUT);
  pinMode(SPEED_HI_BTN,   INPUT);

  digitalWrite(STOP_BTN, HIGH);         // buttons go LOW when pressed
  digitalWrite(SPEED_LOW_BTN, HIGH);
  digitalWrite(SPEED_MED_BTN, HIGH);
  digitalWrite(SPEED_HI_BTN, HIGH);

  updateButtonLEDs(SPEED_ALL_LED);      // all 3 ON
  
  // Motor MOSFET connected to PWM pin
  analogWrite(MotorPWMpin, pwmSTOP);    // make sure motor is stopped       

  Serial.println("RFID Master v1.5");
  
  // set timer for idle timeout, in milliseconds from now
  sleepTimer = millis() + (idleTime * 1000ul);

  // Setup Displays
  updateDisplays(remainTime, score);

}


void loop () {
  // check the start/stop buttons
  queryButtons();
  if(speed == pwmSTOP) playAudioFile(1);    ////?? start blank audio file
  // if we are running . . .
  while (runFlag) {
    // check the start/stop buttons
    queryButtons();
    if(speed == pwmSTOP) playAudioFile(1);  ////?? start blank audio file

    // query the 5 slave devices
    queryBins();

    // STOP if time expired
    if(expireTime < millis()) {
      remainTime = 0ul;
      endOfRun(0);
      break;
    }

    // update time and score displays, if required
    remainTime = ((expireTime - millis()) / 1000ul);  // remining time, in seconds
    if(remainTime > maxTime) remainTime = 0;          // overflow
    if(remainTime != lastTime || score != lastScore) {
      updateDisplays(remainTime, score);
      lastTime = remainTime;
      lastScore = score;
    }

  }
  
  if(!sleepFlag) {
    // check idle timeout if not running
    if(sleepTimer < millis()) {
      sleepFlag = true;                   // set sleeping flag
      clearDisplays();                    // turn off digital displays and buttons
    }
  }

}


void queryButtons() {
  // read the speed pin inputs (LOW when pressed)
  if(digitalRead(STOP_BTN) == LOW) {
    setSpeed(0);
    if (runFlag) endOfRun(1);
    runFlag = false;
    sleepFlag = false;
    sleepTimer = millis() + (idleTime * 1000ul);

  }else if(digitalRead(SPEED_LOW_BTN) == LOW) {
    if (DEBUG) Serial.println("LOW BUTTON PRESSED");
    // start running
    setSpeed(1);
    sleepFlag = false;
    sleepTimer = millis() + (idleTime * 1000ul);

  }else if(digitalRead(SPEED_MED_BTN) == LOW) {
    if (DEBUG) Serial.println("MED BUTTON PRESSED");
    // start running
    setSpeed(2);
    sleepFlag = false;
    sleepTimer = millis() + (idleTime * 1000ul);

  }else if(digitalRead(SPEED_HI_BTN) == LOW) {
    if (DEBUG) Serial.println("HIGH BUTTON PRESSED");
    // start running
    setSpeed(3);
    sleepFlag = false;
    sleepTimer = millis() + (idleTime * 1000ul);
  }

}


void setSpeed(int gear) {

  switch (gear) {
    case 0:                                   // if STOP
      speed = pwmSTOP;
      playAudioFile(1);                       // start blank audio file for tacit
      // restore previous results
      updateDisplays(remainTime, score);
      lastTime = remainTime;
      lastScore = score;
      analogWrite(MotorPWMpin, pwmSTOP);         
      updateButtonLEDs(STOP_LED);
      break;
    case 1:                                   // if LOW
      if(speed != pwmLOW) {
        if (!runFlag) {
          runFlag = true;
          // reset timers and score
          startTime = millis();               // set start time, in milliseconds
          expireTime = startTime + (maxTime * 1000ul) + deadTime;  // set expiration time, in milliseconds
          remainTime = maxTime;               // remining time, in seconds
          score = 0;
          updateDisplays(remainTime, score);
          playAudioFile(0);                   // start audio file
          delay(deadTime);
          lastTime = remainTime;
          lastScore = score;
        }
        speed = pwmLOW;
        analogWrite(MotorPWMpin, pwmLOW);         
        updateButtonLEDs(SPEED_LOW_LED);
      }
      break;
    case 2:                                   // if MED
      if(speed != pwmMED) {
        if (!runFlag) {
          runFlag = true;
          // reset timers and score
          startTime = millis();               // set start time, in milliseconds
          expireTime = startTime + (maxTime * 1000ul) + deadTime;  // set expiration time, in milliseconds
          remainTime = maxTime;               // remining time, in seconds
          score = 0;
          updateDisplays(remainTime, score);
          playAudioFile(0);                   // start audio file
          delay(deadTime);
          lastTime = remainTime;
          lastScore = score;
        }
        speed = pwmMED;
        analogWrite(MotorPWMpin, pwmMED);         
        updateButtonLEDs(SPEED_MED_LED);
      }
      break;
    case 3:                                   // if HIGH
      if(speed != pwmHIGH) {
        if (!runFlag) {
          runFlag = true;
          // reset timers and score
          startTime = millis();               // set start time, in milliseconds
          expireTime = startTime + (maxTime * 1000ul) + deadTime;  // set expiration time, in milliseconds
          remainTime = maxTime;               // remining time, in seconds
          score = 0;
          updateDisplays(remainTime, score);
          playAudioFile(0);                   // start audio file
          delay(deadTime);
          lastTime = remainTime;
          lastScore = score;
        }
        speed = pwmHIGH;
        analogWrite(MotorPWMpin, pwmHIGH);         
        updateButtonLEDs(SPEED_HIGH_LED);
      }
      break;
    //default: 
      // nothing to do here . . .
  }

}


void queryBins() {
  if (COMM_DEBUG) Serial.println("QUERY BINS 1-5");
  for(int i = 0; i < 5; i++) {
    digitalWrite(MOSI_PIN[i], HIGH);                  // Initiate QUERY state
    if(digitalRead(MISO_PIN[i]) == HIGH) {            // slave has data
      if (COMM_DEBUG) {
        Serial.print("Bin ");
        Serial.print(i+1,DEC);
        Serial.println(" has data");
      }
      if (COMM_DEBUG2) Serial.println("1 MISO HIGH (data ready)");
      // process data as needed
      while (digitalRead(MISO_PIN[i]) == HIGH) {}     // wait for ACK from slave
      if (COMM_DEBUG2) Serial.println("2 MISO LOW (ACK)");
      digitalWrite(MOSI_PIN[i], LOW);                 // send ACK to slave
      score++;
/* R1.5 simplified communication protocol
      while (digitalRead(MISO_PIN[i]) == LOW) {}      // wait for response
      digitalWrite(MOSI_PIN[i], LOW);                 // repeat for safety
      if(digitalRead(MISO_PIN[i]) == HIGH) {          // slave has data
        if (COMM_DEBUG2) Serial.println("3 MISO HIGH (score!)");
        score++;
      }
      digitalWrite(MOSI_PIN[i], HIGH);                // send ACK to slave
      // no more data . . .
      while (digitalRead(MISO_PIN[i]) == HIGH) {}     // wait for IDLE from slave
      if (COMM_DEBUG2) Serial.println("4 MISO LOW (idle)");
*/
    }
    digitalWrite(MOSI_PIN[i], LOW);                  // Initiate IDLE state
  }

}


void endOfRun(int method) {
  // method: 0 = time expired;  1 = stop button pressed
  updateDisplays(remainTime, score);
  lastTime = remainTime;
  lastScore = score;
  if (DEBUG) {
    if (method == 0) Serial.println("TIME EXPIRED");
    else Serial.println("STOP BUTTON PRESSED");
    Serial.println("*** END of RUN***");
    Serial.print("   Speed was: ");
    Serial.println(speed, DEC);
  }
  analogWrite(MotorPWMpin, pwmSTOP);         
  updateButtonLEDs(STOP_LED);
  speed = pwmSTOP;
  runFlag = false;
  sleepTimer = millis() + (idleTime * 1000ul);

}


void  updateDisplays(unsigned long cval, int sval) {
  // sets the clock and score digital displays
  int s_ones = sval % 10;
  int s_tens = sval / 10;
  int c_ones = cval % 10ul;
  int c_tens = cval / 10ul;
  byte digit0 = segPattern0[s_ones];
  byte digit1 = segPattern1[s_tens];
  byte digit2 = segPattern0[c_ones];
  byte digit3 = segPattern1[c_tens];
  //ground displayStrobe and hold low during transmission
  digitalWrite(displayStrobe, LOW);
  shiftOut(displayData, displayClock, MSBFIRST , digit3);
  shiftOut(displayData, displayClock, MSBFIRST , digit2);
  shiftOut(displayData, displayClock, MSBFIRST , digit1);
  shiftOut(displayData, displayClock, MSBFIRST , digit0);
  digitalWrite(displayStrobe, HIGH);
  digitalWrite(displayStrobe, LOW);

  if (DEBUG) {
    Serial.print("Time remaining: ");
    Serial.print(cval, DEC);
    Serial.print("   SCORE: ");
    Serial.println(sval, DEC);
  }

}


void  clearDisplays() {
  // turn off the clock and score digital displays
  // However, do no reset timers or score

  if (DEBUG) Serial.println("sleeping . . .");

  digitalWrite(displayClock, LOW);
  // ground displayStrobe and hold low during transmission
  digitalWrite(displayStrobe, LOW);
  shiftOut(displayData, displayClock, MSBFIRST , (byte)0);
  shiftOut(displayData, displayClock, MSBFIRST , (byte)0);
  shiftOut(displayData, displayClock, MSBFIRST , (byte)0);
  shiftOut(displayData, displayClock, MSBFIRST , (byte)0);
  digitalWrite(displayStrobe, HIGH);

  // turn off button LEDs
  updateButtonLEDs(SPEED_OFF_LED);

}


void  updateButtonLEDs(byte val) {
  // sets the clock and score digital displays
  if (DEBUG) {
    Serial.print("LEDs (byte): ");
    Serial.println(val, BIN);
  }
  digitalWrite(buttonClock2, LOW);
  //ground buttonStrobe2 and hold low during transmission
  digitalWrite(buttonStrobe2, LOW);
  shiftOut(buttonData2, buttonClock2, MSBFIRST , val);
//  shiftOut(buttonData2, buttonClock2, MSBFIRST , B10101010);
  digitalWrite(buttonStrobe2, HIGH);
  digitalWrite(buttonStrobe2, LOW);

}

void  playAudioFile(byte audioFile) {
  // set associated pin LOW to start audio output
  digitalWrite(audioTrigger[audioFile], LOW);   // start audio file
  delay(1);                      //changed from 1ms to 50ms to trigger Sparkfun WAV trigger board.
  digitalWrite(audioTrigger[audioFile], HIGH);  // clear trigger

}

