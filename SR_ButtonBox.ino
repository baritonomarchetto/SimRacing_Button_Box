/*SIMRACING BUTTON BOX
 * 
 * Features:
 * - up to 24 inputs
 * - up to 24 outputs
 * - up to 3 rotary encoders (hardware filtered)
 * - up to 4 analog axis
 * - built-in outputs drivers
 * 
 * Hardware description: 
 * 3 input shift registers (74HC165) to monitor switches/buttons states (SPI communication). 
 * 3 output shift registers (74HC595) send inputs to lamp drivers (3x ULN2003).
 * 3 Darlington pairs (ULN2803) to drive LEDs
 * 1 input buffer (74HC14N) for rotary encoders
 * 
 * Arduino pro-micro emulates a multi HID device
 * 
 * by barito 2024 (last update: apr 2024)
 */

#include <SPI.h>
#include <Joystick.h>
#include <Keyboard.h>
#include <Mouse.h>

#define REG_SWITCH 24 //number of switches and buttons (rotary EXCLUDED)
#define REG_LEDS 24 //number of LEDs
#define ROTARIES 3 //number of rotary encoders

#define M_LEFT 251
#define M_RIGHT 252
#define NULL 255

//SHIFT REGISTERS
  const byte IN_REG = 3; //number of input shift registers
  const byte OUT_REG = 3; //number of input shift registers

//PINOUT
  const byte inLatchPin = 9; //input registers latch pin
  const byte outLatchPin = 10; //output register latch pin
  //SPI MISO 14
  //SPI MOSI 16
  //SPI SCLK 15
  const byte J1XPin = A0;
  const byte J1YPin = A1;
  const byte J2XPin = A3;
  const byte J2YPin = A2;
  const byte R1APin = 2;//interrupt pin
  const byte R1BPin = 5;
  const byte R2APin = 3;//interrupt pin
  const byte R2BPin = 6;
  const byte R3APin = 7;//interrupt pin
  const byte R3BPin = 8;
  const byte AUX1Pin = 4;
  const byte AUX2Pin = 0;
  const byte AUX3Pin = 1;

//outputs variables
  unsigned long blinkTime;
  bool oflag;
  byte obyte[OUT_REG];
  byte omask;

//inputs variables
  byte imask;
  byte iState;
  int debounceTime = 40; //ms
  byte stateByte[IN_REG];
  byte prevStateByte[IN_REG];
  unsigned long debounce [REG_SWITCH];
  int J1X;
  int J1Y;
  int J2X;
  int J2Y;
  int mouse_Xspeed;
  int mouse_Yspeed;
  const int deadzone = 50;
  int R1A_state;
  int R1B_state;
  int last1Encoded = 0;
  int R1Value = 0;
  int sum1;
  int R2A_state;
  int R2B_state;
  int last2Encoded = 0;
  int R2Value = 0;
  int sum2;


  //Numbers up to 23 are emulated as joypad buttons, numbers up to 250 are emulated as key presses (ASCII). 
  //251 and 252 are assigned to mouse left and right click. 256 to "null"
  //https://www.arduino.cc/reference/en/language/functions/usb/keyboard/keyboardmodifiers/
  const byte keybtn[REG_SWITCH] = {KEY_ESC, KEY_RETURN, 2, 3, 4, 5, 6, 7, 
                                  NULL, 9, M_LEFT, 11, 12, 13, 14, 15, 
                                  16, 17, 18, 19, 20, 21, 22, 23};

//chat
  bool chFlag = 0;
  const char *chatPhrase[REG_SWITCH] = {"" , "", "my fault!", "pls go easy on 1st turn", "cmon man!", "sorry!", "good race!", "thanks", 
                                        "", "", "", "", "", "kids...", "what the hell!", "", 
                                        "", "", "", "", "", "", "", ""};

//JOYSTICK DEFINITION
//X and Y rotation are L and R triggers (separate axis). 
Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID, 
  JOYSTICK_TYPE_GAMEPAD, REG_SWITCH + (ROTARIES*2), 0, //joy type, button count, hatswitch count
  false, false, false, // X, Y, Z axis
  false, false, false, // X, Y, Z rotation
  false, false, //rudder, throttle
  false, false, false); //accelerator, brake, steering

void setup (){
  pinMode (inLatchPin, OUTPUT);
  pinMode (outLatchPin, OUTPUT);
  pinMode (R1APin,INPUT_PULLUP);
  pinMode (R1BPin,INPUT_PULLUP);
  pinMode (R2APin,INPUT_PULLUP);
  pinMode (R2BPin,INPUT_PULLUP);
  pinMode (R3APin,INPUT_PULLUP);
  pinMode (R3BPin,INPUT_PULLUP);
  pinMode (AUX1Pin,INPUT_PULLUP);
  pinMode (AUX2Pin,INPUT_PULLUP);
  pinMode (AUX3Pin,INPUT_PULLUP);
  digitalWrite (inLatchPin, HIGH);
  digitalWrite (outLatchPin, HIGH);
  R1A_state = digitalRead(R1APin);
  R1B_state = digitalRead(R1BPin);
  R2A_state = digitalRead(R2APin);
  R2B_state = digitalRead(R2BPin);
  attachInterrupt(digitalPinToInterrupt(R1APin), R1_1X, CHANGE);
  attachInterrupt(digitalPinToInterrupt(R2APin), R2_1X, CHANGE);
  //initialize debounce
  for (int a = 0; a < REG_SWITCH; a++){
    debounce[a] = millis();
  }
  SPI.begin ();//start SPI
  Joystick.begin();//Initialize Joystick
  Keyboard.begin();//Initialize keyboard
  Mouse.begin(); //Initialize mouse
}  // end of setup

void loop (){
  AnalogHandle();
  InRegPulse();
  InRegRead();
  InRegHandle();
  OutRegHandle();
}  // end of loop

void InRegHandle(){ //Handle registry inputs (74HC165)
  for (int a = 0; a < IN_REG; a++){
    imask = 0b00000001;
    for (int i = 0; i < 8; i++){
      iState = stateByte[a] & imask;
      if (millis()- debounce[i+(a*8)] > debounceTime && iState != (prevStateByte[a] & imask)){
        debounce[i+(a*8)] = millis();
        //JOYPAD
        if(keybtn[i+(a*8)] < REG_SWITCH){
          if(chFlag == 0 || chatPhrase[i+(a*8)] == ""){
            Joystick.setButton(keybtn[i+(a*8)], iState>>i);
            delay(10);
          }
          else{//chat mode on!!
            if (iState > 0){//BUTTON PRESSED
              Keyboard.print(chatPhrase[i+(a*8)]);
            }
          }
        }
        //KEYBOARD
        else if(keybtn[i+(a*8)] <= 250){
          if (iState > 0){//BUTTON PRESSED
            Keyboard.write(keybtn[i+(a*8)]);
            delay(10);
          }
        }
        //MOUSE
        else if(keybtn[i+(a*8)] == M_LEFT){//MOUSE left click
          if (iState > 0){//BUTTON PRESSED
            Mouse.click(MOUSE_LEFT);
            delay(10);
          }
        }
        else if(keybtn[i+(a*8)] == M_RIGHT){//MOUSE RIGHT click
          if (iState > 0){//BUTTON PRESSED
            Mouse.click(MOUSE_RIGHT);
            delay(10);
          }
        }
        //OTHERS
        else if(keybtn[i+(a*8)] == NULL){//This is to inhibit toggle buttons (i.e. mode flag buttons) to send strokes
          //DO NOTHING
        }
      }
      imask <<= 1; //move to the next bit
    }
    prevStateByte[a] = stateByte[a];
  }
  ChatMode();
}

void ChatMode(){
//chat mode toggle
  iState = stateByte[1] & 0b00000001;//button #9 in my box
  if(iState > 0){
    chFlag = !chFlag;
    if(chFlag){
      Keyboard.write(KEY_RETURN);//ACC opens the chat box by pressing ENTER
    }
    delay(700);//cheap debounce and multi-press protection
  }
}

void AnalogHandle(){
  J1X = analogRead(J1XPin);//0-1023
  J1Y = analogRead(J1YPin);//0-1023
  J2X = analogRead(J2XPin);//0-1023
  J2Y = analogRead(J2YPin);//0-1023
  //MOUSE MOVEMENT - X
  if(J1X < 512 + deadzone && J1X > 512 - deadzone){//X DEADZONE
    mouse_Xspeed = 0;
  }
  else if(J1X > 1000){
    mouse_Xspeed = -3;
  }
  else if(J1X > 800){
    mouse_Xspeed = -2;
  }
   else if(J1X > 512){
    mouse_Xspeed = -1;
  }
  else if(J1X > 300){
    mouse_Xspeed = 1;
  }
  else if(J1X > 100){
    mouse_Xspeed = 2;
  }
  else if(J1X >= 0){
    mouse_Xspeed = 3;
  }
  //MOUSE MOVEMENT - Y
  if(J1Y < 512 + deadzone && J1Y > 512 - deadzone){//X DEADZONE
    mouse_Yspeed = 0;
  }
  else if(J1Y > 1000){
    mouse_Yspeed = 3;
  }
  else if(J1Y > 800){
    mouse_Yspeed = 2;
  }
   else if(J1Y > 512){
    mouse_Yspeed = 1;
  }
  else if(J1Y > 300){
    mouse_Yspeed = -1;
  }
  else if(J1Y > 100){
    mouse_Yspeed = -2;
  }
  else if(J1Y >= 0){
    mouse_Yspeed = -3;
  }
  Mouse.move(mouse_Xspeed, mouse_Yspeed, 0);
}

void OutRegHandle(){
  if(millis()-blinkTime > 600) {
    blinkTime = millis();
    oflag = !oflag;
    if(chFlag == 1){//CHAT MODE ON      
      if(oflag == 1){
        obyte[0] = 0b00000000;
        obyte[1] = obyte[0];
        obyte[2] = obyte[0];
      }
      else{
        obyte[0] = 0b11111111;
        obyte[1] = obyte[0];
        obyte[2] = obyte[0];
      }
    }
    else{//CHAT MODE OFF
      obyte[0] = 0b11111111;
      obyte[1] = obyte[0];
      obyte[2] = obyte[0];
    }
    SPI.transfer(obyte[0]/*, sizeof(obyte)*/);//send data to ou shift register
    SPI.transfer(obyte[1]);//fullfill all buffers
    SPI.transfer(obyte[2]);//fullfill all buffers
    OutRegPulse();//pulse out outputs states
  }
}

void R1_1X(){ //Rotary #1, 1X COUNTING
  if (digitalRead(R1APin) != R1A_state){
  R1A_state=!R1A_state;
    //look for a low-to-high on channel A
    if (R1A_state == HIGH){
      //check channel B to see which way encoder is turning
      //if(optB_state == HIGH){
      if(digitalRead(R1BPin) == LOW){
        Joystick.setButton(REG_SWITCH+1, 1);
        delay(50);
        Joystick.setButton(REG_SWITCH+1, 0);
      } 
      else{
        Joystick.setButton(REG_SWITCH+2, 1);
        delay(50);
        Joystick.setButton(REG_SWITCH+2, 0);
      }
    }
  }
}

void R2_1X(){ //Rotary #2, 1X COUNTING
  if (digitalRead(R2APin) != R2A_state){
  R2A_state=!R2A_state;
    //look for a low-to-high on channel A
    if (R2A_state == HIGH){
      //check channel B to see which way encoder is turning
      //if(optB_state == HIGH){
      if(digitalRead(R2BPin) == LOW){
        Joystick.setButton(REG_SWITCH+3, 1);
        delay(50);
        Joystick.setButton(REG_SWITCH+3, 0);
        delay(10);
      } 
      else{
        Joystick.setButton(REG_SWITCH+4, 1);
        delay(50);
        Joystick.setButton(REG_SWITCH+4, 0);
        delay(10);
      }
    }
  }
}

void InRegPulse(){  // pulse the parallel load latch to send bytes serially
  digitalWrite (inLatchPin, LOW);
  //delayMicroseconds(5);  
  digitalWrite (inLatchPin, HIGH);
}

void OutRegPulse(){  // pulse the parallel out latch to open outs
  digitalWrite (outLatchPin, LOW);
  //delayMicroseconds(5);  
  digitalWrite (outLatchPin, HIGH);
}

void InRegRead(){ //read inputs states from registers
  for (int a = 0; a < IN_REG; a++){
    stateByte[a] = SPI.transfer (0);//"0" is dummy: you could have any value in
  }
}
