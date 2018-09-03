

#include <LiquidCrystal.h>
#include "DHT.h"
#include <Wire.h>
#include "ds3231.h"

#define BUFF_MAX 128

// set the DHT Pin
#define DHTPIN 8


// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
String humidityString, tempString;


// Relay pins
const int fanRelayPin = 9;
const int humidifierRelayPin = 10;
const int coolerRelayPin = 7;
const int heaterRelayPin = 6;


// TIME CONSTANTS
int SECOND = 1000;
int MINUTE = 60 * SECOND;
int HOUR = 60 * MINUTE;


const int actionPeriod = 15; //minutes
const int humidifierDuration = 2; //Minutes
const int waitingPeriod = 15; // Seconds
const int ventingDuration = 15; //SECOND;





uint8_t time[8];
char recv[BUFF_MAX];
unsigned int recv_size = 0;
unsigned long prev, interval = 1 * SECOND;

void parse_cmd(char *cmd, int cmdsize);

struct ts t;

void setup()
{

  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  dht.begin();

  pinMode(fanRelayPin, OUTPUT);
  pinMode(humidifierRelayPin, OUTPUT);
  pinMode(coolerRelayPin, OUTPUT);
  pinMode(heaterRelayPin, OUTPUT);


  // Time setup
  Serial.begin(9600);
  Wire.begin();
  DS3231_init(DS3231_CONTROL_INTCN);
  memset(recv, 0, BUFF_MAX);
  Serial.println("GET time");

  //             TssmmhhWDDMMYYYY aka set time

  //      parse_cmd("T003320120082018" , 16);

}

void refreshSensors() {

  // read humidity
  int h = dht.readHumidity();
  String  humidityValue(" %");
  humidityString = h + humidityValue;

  //read temperature
  int f = dht.readTemperature();
  String tempValue = " c";
  tempString = f + tempValue;

  if ((t.sec % 10) <= 5) {

    Serial.print(" - LCD show time & date - ");
    lcd.clear();
    lcd.home();
    lcd.print("Date: ");
    lcd.print(t.mday);
    lcd.print("-");
    lcd.print(t.mon);
    lcd.print("-");
    lcd.print(t.year);

    lcd.setCursor(0, 1);
    lcd.print("Time: ");

    if (t.hour < 10) {
      lcd.print("0");
      lcd.print(t.hour);
    } else {
      lcd.print(t.hour);
    }

    lcd.print(":");
    if (t.min < 10) {
      lcd.print("0");
      lcd.print(t.min);
    } else {
      lcd.print(t.min);
    }

    lcd.print(":");

    if (t.sec < 10) {
      lcd.print("0");
      lcd.print(t.sec);
    } else {
      lcd.print(t.sec);
    }


  } else {

    Serial.print(" - LCD show climate - ");
    // For Temparature
    lcd.clear();
    lcd.home();
    lcd.print("Temp:  Humidity:");

    lcd.setCursor(0, 1);
    if (isnan(h) || isnan(f)) {
      lcd.print("ERROR");
    } else {
      lcd.print(tempString);
      lcd.setCursor(7, 1);
      lcd.print(humidityString);
    }
  }
}

void loop()
{
  char in;
  char buff[BUFF_MAX];
  unsigned long now = millis();


  // show time once in a while
  if ((now - prev > interval) && (Serial.available() <= 0)) {
    DS3231_get(&t);

    // there is a compile time option in the library to include unixtime support
#ifdef CONFIG_UNIXTIME
#ifdef __AVR__
    snprintf(buff, BUFF_MAX, "%d.%02d.%02d %02d:%02d:%02d %ld", t.year,
#error AVR
#else
    snprintf(buff, BUFF_MAX, "%d.%02d.%02d %02d:%02d:%02d %d", t.year,
#endif
             t.mon, t.mday, t.hour, t.min, t.sec, t.unixtime);
#else
    snprintf(buff, BUFF_MAX, "%d.%02d.%02d %02d:%02d:%02d", t.year,
             t.mon, t.mday, t.hour, t.min, t.sec);
#endif

    Serial.println(buff);
    Serial.print(" - " + humidityString + " - " + tempString);
    //
    //write here code to execute at each interval
    refreshSensors();


    //perform relay action here

//
//
//    // Uncomment to test relays
//
//    if ((t.sec % 10) <= 5) {
//      //               digitalWrite(fanRelayPin, HIGH);
//      digitalWrite(humidifierRelayPin, HIGH);
//      //               digitalWrite(heaterRelayPin, HIGH);
//      //               digitalWrite(coolerRelayPin, HIGH);
//
//    } else {
//
//      //                 digitalWrite(fanRelayPin, LOW);
//      digitalWrite(humidifierRelayPin, LOW);
//      //               digitalWrite(heaterRelayPin, LOW);
//      //               digitalWrite(coolerRelayPin, LOW);
//    }


    if (dht.readHumidity() < 90){
              digitalWrite(humidifierRelayPin, HIGH);
      }else if (dht.readHumidity() >= 95) {
              digitalWrite(humidifierRelayPin, LOW);
        }

    //make fan come on for 15 s then after put the humndifier on for 5 min
    if (t.hour > 8 && t.hour < 22) {

      //      Lights on

      if (dht.readTemperature() > 22) //change temp in celcius to match your threshold.
      {
        digitalWrite(coolerRelayPin, HIGH);//turn on the cool plate
      }
      else
      {
        digitalWrite(coolerRelayPin, LOW);
      }

      if (t.min % actionPeriod == 0 && t.sec == 0) {
        digitalWrite(fanRelayPin, HIGH);
      }

      if ((t.min % actionPeriod) == 0 && t.sec == ventingDuration) {
        digitalWrite(fanRelayPin, LOW);
//        digitalWrite(humidifierRelayPin, HIGH);
      }

      if ((t.min % actionPeriod) == humidifierDuration && t.sec == ventingDuration) {
//        digitalWrite(humidifierRelayPin, LOW);
      }

    } else {

      //    lights off
    }
    prev = now;
  }

  if (Serial.available() > 0) {
    in = Serial.read();

    if ((in == 10 || in == 13) && (recv_size > 0)) {
      parse_cmd(recv, recv_size);
      recv_size = 0;
      recv[0] = 0;
    } else if (in < 48 || in > 122) {
      ;       // ignore ~[0-9A-Za-z]
    } else if (recv_size > BUFF_MAX - 2) {   // drop lines that are too long
      // drop
      recv_size = 0;
      recv[0] = 0;
    } else if (recv_size < BUFF_MAX - 2) {
      recv[recv_size] = in;
      recv[recv_size + 1] = 0;
      recv_size += 1;
    }

  }
}



void parse_cmd(char *cmd, int cmdsize)
{
  uint8_t i;
  uint8_t reg_val;
  char buff[BUFF_MAX];
  struct ts t;

  //snprintf(buff, BUFF_MAX, "cmd was '%s' %d\n", cmd, cmdsize);
  //Serial.print(buff);

  // TssmmhhWDDMMYYYY aka set time
  if (cmd[0] == 84 && cmdsize == 16) {
    //T355720619112011
    t.sec = inp2toi(cmd, 1);
    t.min = inp2toi(cmd, 3);
    t.hour = inp2toi(cmd, 5);
    t.wday = cmd[7] - 48;
    t.mday = inp2toi(cmd, 8);
    t.mon = inp2toi(cmd, 10);
    t.year = inp2toi(cmd, 12) * 100 + inp2toi(cmd, 14);
    DS3231_set(t);
    Serial.println("OK");
  } else if (cmd[0] == 49 && cmdsize == 1) {  // "1" get alarm 1
    DS3231_get_a1(&buff[0], 59);
    Serial.println(buff);
  } else if (cmd[0] == 50 && cmdsize == 1) {  // "2" get alarm 1
    DS3231_get_a2(&buff[0], 59);
    Serial.println(buff);
  } else if (cmd[0] == 51 && cmdsize == 1) {  // "3" get aging register
    Serial.print("aging reg is ");
    Serial.println(DS3231_get_aging(), DEC);
  } else if (cmd[0] == 65 && cmdsize == 9) {  // "A" set alarm 1
    DS3231_set_creg(DS3231_CONTROL_INTCN | DS3231_CONTROL_A1IE);
    //ASSMMHHDD
    for (i = 0; i < 4; i++) {
      time[i] = (cmd[2 * i + 1] - 48) * 10 + cmd[2 * i + 2] - 48; // ss, mm, hh, dd
    }
    uint8_t flags[5] = { 0, 0, 0, 0, 0 };
    DS3231_set_a1(time[0], time[1], time[2], time[3], flags);
    DS3231_get_a1(&buff[0], 59);
    Serial.println(buff);
  } else if (cmd[0] == 66 && cmdsize == 7) {  // "B" Set Alarm 2
    DS3231_set_creg(DS3231_CONTROL_INTCN | DS3231_CONTROL_A2IE);
    //BMMHHDD
    for (i = 0; i < 4; i++) {
      time[i] = (cmd[2 * i + 1] - 48) * 10 + cmd[2 * i + 2] - 48; // mm, hh, dd
    }
    uint8_t flags[5] = { 0, 0, 0, 0 };
    DS3231_set_a2(time[0], time[1], time[2], flags);
    DS3231_get_a2(&buff[0], 59);
    Serial.println(buff);
  } else if (cmd[0] == 67 && cmdsize == 1) {  // "C" - get temperature register
    Serial.print("temperature reg is ");
    Serial.println(DS3231_get_treg(), DEC);
  } else if (cmd[0] == 68 && cmdsize == 1) {  // "D" - reset status register alarm flags
    reg_val = DS3231_get_sreg();
    reg_val &= B11111100;
    DS3231_set_sreg(reg_val);
  } else if (cmd[0] == 70 && cmdsize == 1) {  // "F" - custom fct
    reg_val = DS3231_get_addr(0x5);
    Serial.print("orig ");
    Serial.print(reg_val, DEC);
    Serial.print("month is ");
    Serial.println(bcdtodec(reg_val & 0x1F), DEC);
  } else if (cmd[0] == 71 && cmdsize == 1) {  // "G" - set aging status register
    DS3231_set_aging(0);
  } else if (cmd[0] == 83 && cmdsize == 1) {  // "S" - get status register
    Serial.print("status reg is ");
    Serial.println(DS3231_get_sreg(), DEC);
  } else {
    Serial.print("unknown command prefix ");
    Serial.println(cmd[0]);
    Serial.println(cmd[0], DEC);
  }
}

