/**************************************************
 *                                                *
 * AquaController by Radek Kubera (rkubera)       *
 * all rights reserved                            *
 * free of charge for non-commercial use only     *
 * https://github.com/rkubera/AquariumController  *
 *                                                *
 * ************************************************/

#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <Wire.h>
#include <EEPROM.h>
#include <Timezone.h>       // https://github.com/JChristensen/Timezone
#include <Keypad.h>         // https://playground.arduino.cc/Code/Keypad#Download Must be patched. Replace: #define OPEN LOW with #define KBD_OPEN LOW, #define CLOSED HIGH with #define KBD_CLOSED HIGH in Key.h and Keypad.h. Replace OPEN with KBD_OPEN, CLOSE with KBD_CLOSE in Keypad.cpp 
#include <ELClient.h>       // https://github.com/jeelabs/el-client
#include <ELClientCmd.h>    // https://github.com/jeelabs/el-client
#include <ELClientMqtt.h>   // https://github.com/jeelabs/el-client
#include <RTClib.h>         // https://github.com/adafruit/RTClib
#include <QuickStats.h>     // https://github.com/dndubins/QuickStats
#include <dht.h>            // https://github.com/RobTillaart/Arduino/tree/master/libraries/DHTlib/
//#include <PID_v1.h>         // https://github.com/br3ttb/Arduino-PID-Library/

//********************************
//PINS
//********************************
#define BUZZER_PIN 6

#define DIGITAL_PWM_12V_OUT_PIN_1 4
#define DIGITAL_PWM_12V_OUT_PIN_2 5

#define DIGITAL_PWM_OUT_PIN_1  7
#define DIGITAL_PWM_OUT_PIN_2  8
#define DIGITAL_PWM_OUT_PIN_3  10

#define ANALOG_IN_PIN_0 4
#define ANALOG_IN_PIN_1 5
#define ANALOG_IN_PIN_2 6
#define ANALOG_IN_PIN_3 7
#define ANALOG_IN_PIN_4 8
#define ANALOG_IN_PIN_5 9
#define ANALOG_IN_PIN_6 10
#define ANALOG_IN_PIN_7 11

#define LED_RED_PIN 11
#define LED_GREEN_PIN 12
#define LED_BLUE_PIN 10

#define LCD_PIN_SCE   33
#define LCD_PIN_RESET 34
#define LCD_PIN_DC    35
#define LCD_PIN_SDIN  36
#define LCD_PIN_SCLK  37
#define LCD_PIN_LED   9 //Must be PWM Output

#define RELAY_PIN_1 44
#define RELAY_PIN_2 45

#define DHT_PIN 2

#define FAN_PIN 3

const byte KBD_ROWS = 5; //four rows
const byte KBD_COLS = 4; //three columns
char keys[KBD_ROWS][KBD_COLS] = {
{'A','B','#','*'},
{'1','2','3','U'},
{'4','5','6','D'},
{'7','8','9','R'},
{'L','0','R','E'}
};
byte KBD_ROW_PINS[KBD_ROWS] = {30, 29, 28, 27, 26}; //connect to the row pinouts of the keypad
byte KBD_COL_PINS[KBD_COLS] = {22, 23, 24, 25}; //connect to the column pinouts of the keypad

//********************************
//Main variables
//********************************
const char daysOfTheWeek[7][10] PROGMEM = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
const char monthsOfYear[12][10] PROGMEM = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };

const char charSetSensor[] PROGMEM = "set/Sensor";
const char charGetSensor[] PROGMEM = "get/Sensor";

const char charSetRelay[] PROGMEM = "set/Relay";
const char charGetRelay[] PROGMEM = "get/Relay";

const char charSetPwmOutput[] PROGMEM = "set/PWMOutput";
const char charGetPwmOutput[] PROGMEM = "get/PWMOutput";

const char charOn[] PROGMEM = "on";
const char charOff[] PROGMEM = "off";

const char charBlack[] PROGMEM = "black";
const char charRed[] PROGMEM = "red";
const char charGreen[] PROGMEM = "green";
const char charBlue[] PROGMEM = "blue";
const char charCyan[] PROGMEM = "cyan";
const char charMagenta[] PROGMEM = "magenta";
const char charYellow[] PROGMEM = "yellow";
const char charWhite[] PROGMEM = "white";
const char charWave[] PROGMEM = "wave";

const char charManual[] PROGMEM = "manual";
const char charPartofday[] PROGMEM = "partofday";
const char charHysteresis[] PROGMEM = "hysteresis";
const char charPid[] PROGMEM = "pid";

const char charSensornone[] PROGMEM = "notconnected";
const char charRelay[] PROGMEM = "relay";
const char charPWMOutput[] PROGMEM = "pwmoutput";

const char charValue[] PROGMEM = "Value";
const char charRawValue[] PROGMEM = "RawValue";
const char charSensorType[] PROGMEM = "Type";
const char charSensorCalibValue1[] PROGMEM = "CalibValue1";
const char charSensorCalibRawValue1[] PROGMEM = "CalibRawValue1";
const char charSensorCalibValue2[] PROGMEM = "CalibValue2";
const char charSensorCalibRawValue2[] PROGMEM = "CalibRawValue2";
const char charName[] PROGMEM = "Name";

const char charControlMode[] PROGMEM = "ControlMode";
const char charMorningMode[] PROGMEM = "MorningMode";
const char charAfternoonMode[] PROGMEM = "AfternoonMode";
const char charEveningMode[] PROGMEM = "EveningMode";
const char charNightMode[] PROGMEM = "NightMode";
const char charManualMode[] PROGMEM = "ManualMode";

const char charState[] PROGMEM = "State";

#define CONTROL_MODE_MANUAL             0
#define CONTROL_MODE_PART_OF_DAY        1
#define CONTROL_MODE_HYSTERESIS         2
#define CONTROL_MODE_PID                3

#define NAME_LENGHTH                    15

byte fanStartTemperature = 30;
byte fanMaxSpeedTemperature = 35;
byte maxInternalTemperature = 50;

double timerMillisEventDate;
double timerSecondEventDate;
double timerMinuteEventDate;
double timerHourEventDate;
const int bufferSize = 100;
char buffer[bufferSize];
uint32_t boot_time = 0;
QuickStats stats;


int publishValue = -1;
 
//********************************
//RWMOutputs
//********************************
#define PWMOUTPUTS_PWMOUTPUT_EEPROM_BYTES  20
#define PWMOUTPUTS_COUNT                   5

#define PWMOUTPUT_MODE_OFF                 0
#define PWMOUTPUT_MODE_ON                  1
#define PWMOUTPUT_MODE_NONE                3

#define PWMOUTPUT_MANUAL_ONOFF_AUTO        0
#define PWMOUTPUT_MANUAL_ONOFF_OFF         1
#define PWMOUTPUT_MANUAL_ONOFF_ON          2

#define PWMOUTPUT_STATE                    1
#define PWMOUTPUT_CONTROL_MODE             2
#define PWMOUTPUT_MODE_MORNING             3
#define PWMOUTPUT_MODE_AFTERNOON           4
#define PWMOUTPUT_MODE_EVENING             5
#define PWMOUTPUT_MODE_NIGHT               6
#define PWMOUTPUT_MANUAL_ONOFF             7
#define PWMOUTPUT_MANUAL_MODE              8

//********************************
//Relays
//********************************
#define RELAYS_RELAY_EEPROM_BYTES         20
#define RELAYS_COUNT                      2

#define RELAY_MODE_OFF                    0
#define RELAY_MODE_ON                     1
#define RELAY_MODE_NONE                   3

#define RELAY_MANUAL_ONOFF_AUTO           0
#define RELAY_MANUAL_ONOFF_OFF            1
#define RELAY_MANUAL_ONOFF_ON             2

#define RELAY_STATE                       1
#define RELAY_CONTROL_MODE                2
#define RELAY_MODE_MORNING                3
#define RELAY_MODE_AFTERNOON              4
#define RELAY_MODE_EVENING                5
#define RELAY_MODE_NIGHT                  6
#define RELAY_MANUAL_ONOFF                7

//********************************
//Sensors
//********************************
#define SENSORS_SENSOR_EEPROM_BYTES       30

#define SENSORS_COUNT                     8

#define SENSOR_TYPE_NONE                  0

#define SENSORS_VALUE                     1
#define SENSORS_VALUE_RAW                 2
#define SENSORS_VALUE_TYPE                3
#define SENSORS_VALUE_CALIB_VALUE1        8
#define SENSORS_VALUE_CALIB_RAW_VALUE1    9
#define SENSORS_VALUE_CALIB_VALUE2        10
#define SENSORS_VALUE_CALIB_RAW_VALUE2    11

//********************************
//Wifi
//********************************
#define WIFI_STATUS_CONNECTED     1
#define WIFI_STATUS_DISCONNECTED  0
char wifiStatus = 0;

//********************************
//Errors
//********************************

int errorsCount = 0;
int errorsReportStatus;
int errorsLastReportStatus;

//********************************
//TimeZone
//********************************
#define TIMEZONE_WEEK_FIRST   First
#define TIMEZONE_WEEK_SECOND  Second
#define TIMEZONE_WEEK_THIRD   Third
#define TIMEZONE_WEEK_FOURTH  Fourth
#define TIMEZONE_WEEK_LAST    Last

#define TIMEZONE_DOW_SUNDAY     Sun
#define TIMEZONE_DOW_MONDAY     Mon
#define TIMEZONE_DOW_TUESDAY    Tue
#define TIMEZONE_DOW_WEDNESDAY  Wed
#define TIMEZONE_DOW_FOURSDAY   Thu
#define TIMEZONE_DOW_FRIDAY     Fri
#define TIMEZONE_DOW_SATURDAY   Sat

#define TIMEZONE_MONTH_JANUARY    Jan
#define TIMEZONE_MONTH_FEBRUARY   Feb
#define TIMEZONE_MONTH_MARCH      Mar
#define TIMEZONE_MONTH_APRIL      Apr
#define TIMEZONE_MONTH_MAY        May
#define TIMEZONE_MONTH_JUNE       Jun
#define TIMEZONE_MONTH_JULY       Jul
#define TIMEZONE_MONTH_AUGUST     Aug
#define TIMEZONE_MONTH_SEPTEMBER  Sep
#define TIMEZONE_MONTH_OCTOBER    Oct
#define TIMEZONE_MONTH_NOVEMBER   Nov
#define TIMEZONE_MONTH_DECEMBER   Dec

const char timezoneWeekFirst[] PROGMEM = "first";
const char timezoneWeekSecond[] PROGMEM = "second";
const char timezoneWeekThird[] PROGMEM = "third";
const char timezoneWeekFourth[] PROGMEM = "fourth";
const char timezoneWeekLast[] PROGMEM ="last";

char timezoneActualOffset = 1;
byte timezoneRule1Week;
byte timezoneRule2Week;
byte timezoneRule1DayOfWeek;
byte timezoneRule2DayOfWeek;
byte timezoneRule1Hour;
byte timezoneRule2Hour;
char timezoneRule1Offset;
char timezoneRule2Offset;
byte timezoneRule1Month;
byte timezoneRule2Month;

//********************************
//Clock
//********************************

RTC_DS1307 clockRtc;

TimeChangeRule myDST;
TimeChangeRule mySTD;
Timezone myTZ (myDST, mySTD);
TimeChangeRule *tcr;        //pointer to the time change rule, use to get TZ abbrev

byte globalSecond = 0;
byte globalMinute = 43;
byte globalHour = 22; //24 hour time
byte globalWeekDay = 6; //0-6 -> sunday – Saturday
byte globalMonthDay = 10;
byte globalMonth = 2;
int globalYear = 18;

uint32_t clockLastSynchro = 7200000;

//********************************
//Scheduler
//********************************
const char schedulerMorning[] PROGMEM="morning";
const char schedulerAfternoon[] PROGMEM="afternoon";
const char schedulerEvening[] PROGMEM="evening";
const char schedulerNight[] PROGMEM="night";

#define SCHEDULER_MODE_NONE 0
#define SCHEDULER_MODE_MORNING 1
#define SCHEDULER_MODE_AFTERNOON 2
#define SCHEDULER_MODE_EVENING 3
#define SCHEDULER_MODE_NIGHT 4

byte schedulerStartMorningHour = 9;
byte schedulerStartMorningMinute = 0;

byte schedulerStartAfternoonHour = 10;
byte schedulerStartAfternoonMinute = 0;

byte schedulerStartEveningHour = 19;
byte schedulerStartEveningMinute = 0;

byte schedulerStartNightHour = 22;
byte schedulerStartNightMinute = 00;

//********************************
//Buzzer
//********************************
#define BUZZER_ON 1
#define BUZZER_OFF 0

byte buzzerOnStart = 1;
byte buzzerOnErrors = 1;

//********************************
//LED
//********************************
#define LED_MODE_NONE    0
#define LED_MODE_WHITE   1
#define LED_MODE_WAVE    2
#define LED_MODE_RED     3
#define LED_MODE_GREEN   4
#define LED_MODE_BLUE    5
#define LED_MODE_CYAN    6
#define LED_MODE_MAGENTA 7
#define LED_MODE_YELLOW  8
#define LED_MODE_BLACK   9

#define LED_BRIGHTNESS_AUTO -1

byte ledControlMode = CONTROL_MODE_MANUAL;
byte ledManualMode = LED_MODE_NONE;

byte ledTimer = 100;
byte ledActualTimer = 0;
byte ledBrightness = 0;
int ledActualBrightness = 0;

int ledMorningBrightness = 0;
int ledAfternoonBrightness = 0;
int ledEveningBrightness = 0;
int ledNightBrightness = 0;
int ledManualBrightness = 0;

int ledAutoBrightness = 255;
byte ledMode = LED_MODE_NONE;
byte ledModeMorning = LED_MODE_NONE;
byte ledModeAfternoon = LED_MODE_NONE;
byte ledModeEvening = LED_MODE_NONE;
byte ledModeNight = LED_MODE_NONE;

int ledActualRed = 0;
int ledActualGreen = 0;
int ledActualBlue = 0;

byte ledRed = 0;
byte ledGreen = 0;
byte ledBlue = 0;

byte ledStep = 10;
byte ledStepSwitchColor = 10;
byte ledStepWave = 1;

int ledFadeInFromBlackSeconds = 120;

byte ledWaveIdx;
byte ledLastMode = LED_MODE_NONE;

#define LED_MANUAL_ONOFF_AUTO 0
#define LED_MANUAL_ONOFF_OFF 1
#define LED_MANUAL_ONOFF_ON 2

byte ledManualOnOff = LED_MANUAL_ONOFF_AUTO;

double ledRedLevel, ledGreenLevel, ledBlueLevel;

//********************************
//LCD
//********************************
//Configuration for the LCD
#define LCD_C     LOW
#define LCD_D     HIGH
#define LCD_CMD   0

// Size of the LCD
#define LCD_X     84
#define LCD_Y     48

//********************************
//MQTTEL
//********************************
#define MQTT_STATUS_CONNECTED     1
#define MQTT_STATUS_DISCONNECTED  0

byte mqttStatus = MQTT_STATUS_DISCONNECTED;

ELClient mqttElesp(&Serial, &Serial);
ELClientCmd mqttElcmd(&mqttElesp);
ELClientMqtt mqttEl(&mqttElesp);

char mqttElDeviceName[] = "ac";

bool mqttElconnected;

const char setLedColorMorning[] PROGMEM = "set/LedColorMorning";                 //red, green, blue, white, cyan, magenta, yellow, white, wave, black
const char setLedColorAfternoon[] PROGMEM = "set/LedColorAfternoon";             //red, green, blue, white, cyan, magenta, yellow, white, wave, black
const char setLedColorEvening[] PROGMEM = "set/LedColorEvening";                 //red, green, blue, white, cyan, magenta, yellow, white, wave, black
const char setLedColorNight[] PROGMEM = "set/LedColorNight";                     //red, green, blue, white, cyan, magenta, yellow, white, wave, black
const char setBrightness[] PROGMEM = "set/Brightness";                           //0-255
const char setActualTime[] PROGMEM = "set/ActualTime";                           //HH:mm in 24 hours format
const char setActualDate[] PROGMEM="set/ActualDate";                             //yyyy/mm/dd
const char setMorningTime[] PROGMEM = "set/MorningTime";                         //HH:mm in 24 hours format
const char setAfternoonTime[] PROGMEM = "set/AfternoonTime";                     //HH:mm in 24 hours format
const char setEveningTime[] PROGMEM = "set/EveningTime";                         //HH:mm in 24 hours format
const char setNightTime[] PROGMEM = "set/NightTime";                             //HH:mm in 24 hours format
const char setFanStartTemperature[] PROGMEM="set/FanStartTemp";                  //integer value
const char setFanMaxSpeedTemperature[] PROGMEM="set/FanMaxSpeedTemp";            //integer value
const char setMaxInternalTemperature[] PROGMEM="set/MaxIntTemp";                 //integer value
const char setLedControlMode[] PROGMEM = "set/LedControlMode";                   //manual, partofday
const char setLedManualMode[] PROGMEM = "set/LedManualMode";                     //red, green, blue, white, cyan, magenta, yellow, white, wave, black
const char setBuzzerOnStart[] PROGMEM = "set/BuzzerOnStart";                     //on, off
const char setBuzzerOnErrors[] PROGMEM = "set/BuzzerOnErrors";                   //on, off
const char setTimezoneRule1Week[] PROGMEM="set/TimezoneRule1Week";               //first, second, third, fourth, last
const char setTimezoneRule2Week[] PROGMEM="set/TimezoneRule2Week";               //first, second, third, fourth, last
const char setTimezoneRule1DayOfWeek[] PROGMEM="set/TimezoneRule1DayOfWeek";     //sunday, monday, tuesday, wednesday, thursday, friday, saturday
const char setTimezoneRule2DayOfWeek[] PROGMEM="set/TimezoneRule2DayOfWeek";     //sunday, monday, tuesday, wednesday, thursday, friday, saturday
const char setTimezoneRule1Hour[] PROGMEM="set/TimezoneRule1Hour";               //0..23
const char setTimezoneRule2Hour[] PROGMEM="set/TimezoneRule2Hour";               //0..23
const char setTimezoneRule1Offset[] PROGMEM="set/TimezoneRule1Offset";           //+1, +2, -1, -2 etc
const char setTimezoneRule2Offset[] PROGMEM="set/TimezoneRule2Offset";           //+1, +2, -1, -2 etc
const char setTimezoneRule1Month[] PROGMEM="set/TimezoneRule1Month";             //january, february, march, april, may, june, july, august, september, october, november, december
const char setTimezoneRule2Month[] PROGMEM="set/TimezoneRule2Month";             //january, february, march, april, may, june, july, august, september, october, november, december
const char setLedState[] PROGMEM="set/LedState";                                 //on, off

const char getBrightness[] PROGMEM = "get/Brightness";
const char getActualTime[] PROGMEM = "get/ActualTime";
const char getActualDate[] PROGMEM="get/ActualDate";
const char getActualDayOfWeek[] PROGMEM="get/ActualDayOfWeek";
const char getActualTimezoneOffset[] PROGMEM = "get/ActualTimezoneOffset";
const char getInternalTemperature[] PROGMEM = "get/InternalTemperature"; 
const char getInternalHumidity[] PROGMEM = "get/InternalHumidity";   
const char getMorningTime[] PROGMEM = "get/MorningTime";
const char getAfternoonTime[] PROGMEM = "get/AfternoonTime";
const char getEveningTime[] PROGMEM = "get/EveningTime";
const char getNightTime[] PROGMEM = "get/NightTime";
const char getLedColorMorning[] PROGMEM = "get/LedColorMorning";
const char getLedColorAfternoon[] PROGMEM = "get/LedColorAfternoon";
const char getLedColorEvening[] PROGMEM = "get/LedColorEvening";
const char getLedColorNight[] PROGMEM = "get/LedColorNight";
const char getLedControlMode[] PROGMEM = "get/LedControlMode";
const char getLedManualMode[] PROGMEM = "get/LedManualMode";
const char getActualPartOfDay[] PROGMEM = "get/ActualPartOfDay";
const char getStatus[] PROGMEM="get/Status";
const char getFanStartTemperature[] PROGMEM="get/FanStartTemp";
const char getFanMaxSpeedTemperature[] PROGMEM="get/FanMaxSpeedTemp";
const char getFanPWMValue[] PROGMEM="get/FanPWMValue";
const char getMaxInternalTemperature[] PROGMEM="get/MaxIntTemp";
const char getBuzzerOnStart[] PROGMEM = "get/BuzzerOnStart";
const char getBuzzerOnErrors[] PROGMEM = "get/BuzzerOnErrors";
const char getTimezoneRule1Week[] PROGMEM="get/TimezoneRule1Week";
const char getTimezoneRule2Week[] PROGMEM="get/TimezoneRule2Week";
const char getTimezoneRule1DayOfWeek[] PROGMEM="get/TimezoneRule1DayOfWeek";
const char getTimezoneRule2DayOfWeek[] PROGMEM="get/TimezoneRule2DayOfWeek";
const char getTimezoneRule1Hour[] PROGMEM="get/TimezoneRule1Hour";
const char getTimezoneRule2Hour[] PROGMEM="get/TimezoneRule2Hour";
const char getTimezoneRule1Offset[] PROGMEM="get/TimezoneRule1Offset";
const char getTimezoneRule2Offset[] PROGMEM="get/TimezoneRule2Offset";
const char getTimezoneRule1Month[] PROGMEM="get/TimezoneRule1Month";
const char getTimezoneRule2Month[] PROGMEM="get/TimezoneRule2Month";
const char getLedState[] PROGMEM="get/LedState"; 

//********************************
//EEPROM
//********************************

//Daily values
#define EEPROM_ledModeMorning_addr                          2
#define EEPROM_ledModeAfternoon_addr                        3
#define EEPROM_ledModeEvening_addr                          4
#define EEPROM_ledModeNight_addr                            5

#define EEPROM_schedulerStartMorningHour_addr               6
#define EEPROM_schedulerStartMorningMinute_addr             7
#define EEPROM_schedulerStartAfternoonHour_addr             8
#define EEPROM_schedulerStartAfternoonMinute_addr           9
#define EEPROM_schedulerStartEveningHour_addr               10
#define EEPROM_schedulerStartEveningMinute_addr             11
#define EEPROM_schedulerStartNightHour_addr                 12
#define EEPROM_schedulerStartNightMinute_addr               13
  
#define EEPROM_ledMorningBrightness_addr                    14
#define EEPROM_ledAfternoonBrightness_addr                  15
#define EEPROM_ledEveningBrightness_addr                    16
#define EEPROM_ledNightBrightness_addr                      17
#define EEPROM_ledManualBrightness_addr                     18

#define EEPROM_ledControlMode_addr                          57
#define EEPROM_ledManualMode_addr                           58

#define EEPROM_buzzerOnStart_addr                           59
#define EEPROM_buzzerOnErrors_addr                          60

//Global values
#define EEPROM_maxInternalTemperature_addr                  31

#define EEPROM_fanStartTemperature_addr                     30
#define EEPROM_fanMaxSpeedTemperature_addr                  40

#define EEPROM_timezoneRule1Week_addr                       85
#define EEPROM_timezoneRule2Week_addr                       86
#define EEPROM_timezoneRule1DayOfWeek_addr                  87
#define EEPROM_timezoneRule2DayOfWeek_addr                  88
#define EEPROM_timezoneRule1Hour_addr                       89
#define EEPROM_timezoneRule2Hour_addr                       90
#define EEPROM_timezoneRule1Offset_addr                     91
#define EEPROM_timezoneRule2Offset_addr                     92
#define EEPROM_timezoneRule1Month_addr                      93
#define EEPROM_timezoneRule2Month_addr                      94

#define EEPROM_sensors_addr                                 2048      //8*50 = 400
#define EEPROM_relays_addr                                  EEPROM_sensors_addr+(SENSORS_SENSOR_EEPROM_BYTES*SENSORS_COUNT) //4*20 = 80
#define EEPROM_pwm_outputs_addr                             EEPROM_relays_addr+(RELAYS_RELAY_EEPROM_BYTES*RELAYS_COUNT)

//********************************
//Keyboard
//********************************
char lastKey;
Keypad keypad = Keypad(makeKeymap(keys), KBD_ROW_PINS, KBD_COL_PINS, KBD_ROWS, KBD_COLS);

//********************************
//Watchdog
//********************************
time_t watchdogStartTime;
time_t watchdogupTime;

//********************************
//DHT
//********************************
dht DHT;
float dhtHumidity = 0;
float dhtTemperature = 0;

int dhtNumReadings = 20;
float dhtTemparatureReadings[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
float dhtHumidityReadings[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

//********************************
//Fan
//********************************
byte fanPWM = 255;

//********************************
//Main program
//********************************

void setup() {

  //LCD
  lcdInit();
  gotoXY(0,1);
  lcdString("Initializing");
  
  //Init
  Serial.begin(115200);
  Wire.begin();

  //Config
  configLoad();
  
  //Fan
  initFan();
  
  //Keyboard
  keyboardInit();

  //Menu
  menuInit();

  //MQTTEL
  mqttElInit();
  
  //Scheduler
  schedulerInit();

  //LED
  ledInit();
    
  //Clock
  clockInit();

  //DHT
  dhtInit();

  //Relays
  relaysInit();

  //PWMOutputs
  pwmOutputsInit();

  //Control
  controlInit();
  
  lcdClear();  
  //clockSetLocalTime();

  //Sensors
  sensorsInit();

  //Errors
  errorsInit();
  
  //Events
  timerMillisEventDate = millis();
  timerSecondEventDate = millis();
  timerMinuteEventDate = millis();
  timerHourEventDate = millis();

  //Watchdog
  watchdogInit();

  //Buzzer
  buzzerInit();

}

void loop() {
  if (abs(millis()-timerMillisEventDate)>1) {
    eventTimerMillis();
    timerMillisEventDate = millis();
  }

   if (abs(millis()-timerSecondEventDate)>1000) {
    eventTimerSecond();
    timerSecondEventDate = millis();
  }

   if (abs(millis()-timerMinuteEventDate)>60000) {
    eventTimerMinute();
    timerMinuteEventDate = millis();
  }

   if (abs(millis()-timerHourEventDate)>(3600000)) {
    eventTimerHour();
    timerHourEventDate = millis();
  }
    //Keyvboard
  keyboardCheck();

  //Menu
  menuShow();
  
  //DHT
  dhtGetData(); 

  //MQTTEL
  mqttElCheck();
  
}
