/*
 James Whitney, 11/20/2021
 Lab 3
 
 */

#include <IRremote.h>
#include <DS3231.h>
#include <Wire.h>
#include <LiquidCrystal.h>

#define ENABLE 5
#define DIRA 4
#define DIRB 3
#define RECEIVER 6 //defining constants

DS3231 clock;
RTCDateTime dt;
volatile int speedinput = 4;//variable from 0-4 for speed modes, starts at max
volatile int rotate = 255; //sets rotate speed to 255
volatile byte lcdwrite = LOW;
volatile byte state = LOW;
volatile byte fanspeed = LOW;
volatile byte direct = LOW; //logic variables
int button = 2;
int debounceint = 1000;
volatile int newmill; //debounce switch variables
String displayspeed;
String dir; //lcd display variables
// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

IRrecv irrecv(RECEIVER);     // create instance of 'irrecv'
decode_results results;      // create instance of 'decode_results'

void setup() {
  cli();//stop interrupts
  //set timer1 interrupt at 1Hz
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  // set compare match register for 1hz increments
  OCR1A = 15624;// = (16*10^6) / (1*1024) - 1 (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS12 and CS10 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);

  sei();//allow interrupts
  //---set pin direction
  clock.begin();
  //clock.setDateTime(__DATE__, __TIME__);
  pinMode(button, INPUT_PULLUP); //button set as an input
  attachInterrupt(digitalPinToInterrupt(button), modeswap, FALLING);
  pinMode(ENABLE, OUTPUT);
  pinMode(DIRA, OUTPUT);
  pinMode(DIRB, OUTPUT); //motor 
  Serial.begin(9600);
  lcd.begin(16, 2);
  irrecv.enableIRIn(); //IR
}

ISR(TIMER1_COMPA_vect) //timer for a second
{ //timer1 interrupt 1Hz toggles pin 13 (LED)
  //generates pulse wave of frequency 1Hz/2 = 0.5kHz (takes two cycles for full wave- toggle highthen toggle low)
  lcdwrite = !lcdwrite; //writes to LCD every second
}

void modeswap() //when button interupt is triggered, swaps directions
{
  if (debounceint < millis() - newmill)//ensures only one reading is done per press
  {
    direct = !direct;
    state = !state; //flops  logic bytes
    newmill = millis();//
  }
}


void irrecieve(int results) //takes input from remote
{
  int tmpValue;
  switch (results)
  {

    case 0xFF22DD: //if rewind button is pressed, lowers speed by 1 mode if it is not at 0
      if (speedinput > 0)
        speedinput -= 1;
      fanspeed = LOW; //tells speed to be rewritten
      break;
    case 16712445: direct = !direct;  state = !state;  break; //when pause is pressed, swaps direction
    case 0xFFC23D: //if fast forward is pressed, increases speed if it is not at max
      if (speedinput < 4)
        speedinput += 1;
      fanspeed = LOW;
      break;
  }
  irrecv.resume(); // receive the next value
}

void fan()//sets fan direction
{
  if (direct == LOW) //if logic low, turns clockwise
  {
    delay(100);
    digitalWrite(DIRA, HIGH); //one way
    digitalWrite(DIRB, LOW);
    dir = "C"; //
  }
  if (direct == HIGH) //if logic high, turns counterclockwise
  {
    delay(100);
    digitalWrite(DIRA, LOW); //one way
    digitalWrite(DIRB, HIGH);
    dir = "CC";
  }
  state = HIGH;
}

void writelcd()
{
  dt = clock.getDateTime(); //gets time only once a second
  lcd.clear(); //clears screen each time
  lcd.setCursor(0, 1);
  lcd.print(dt.hour);   lcd.print(":"); 
  lcd.print(dt.minute); lcd.print(":");
  lcd.print(dt.second); //displays time
  lcd.setCursor(0, 0);
  lcd.print(dir);
  lcd.setCursor(5, 0);
  lcd.print(displayspeed); //displays speed and direction
  lcdwrite = HIGH; //flips logic 
}

void changespeed()
{
   switch (speedinput) //depending on the current speed mode, sets fan to certain speed
  {
    case 0: rotate = 0;  displayspeed = "0";  break;
    case 1: rotate = 138;  displayspeed = "1/4"; break;
    case 2: rotate = 176; displayspeed = "1/2"; break;
    case 3: rotate = 204; displayspeed = "3/4"; break;
    case 4: rotate = 255; displayspeed = "Full"; break;
  }
  analogWrite(ENABLE, rotate); //enable on
  fanspeed = HIGH;
}


void loop() {
  //---back and forth example
  if (fanspeed == LOW) //only runs if input for speed change is input (start of program or when +/- buttons are pressed)
    changespeed();
  if (irrecv.decode(&results)) //only runs if IR signal is recieved
    irrecieve(results.value);
  if (state == LOW) //only runs if input for direction change is input (start of program, push button or pause button)
    fan();
  if (lcdwrite == LOW) //only runs once a second
    writelcd();
}
