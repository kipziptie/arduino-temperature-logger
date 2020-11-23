/*
Kip Trujillo
First Published: 11/22/2020  
*/

// All the includes.. 
#include <LiquidCrystal.h> // Borrowed Code: LiquidCrystal Library - Hello World
#include "DHT.h"
#include <SPI.h>
#include <SD.h>

// File for SD card data storage 
File myFile;
String filename = "TEMP.TXT";

// Setting aside friendly names for digital I/O pins
#define tempsensor 0
#define DHTPIN 8     // Digital pin connected to the DHT sensor

//Indices for the intervals array
#define LIVE 0
#define SECONDLY 1
#define MINUTELY 2
#define HOURLY 3
/*****************************************************************
 * interval array - represents milliseconds used in all code logic
 *------------------------------- 2 sec|60 Sec|  1HR  |   24HR  |*/
const unsigned long interval[] = {2000, 60000, 3600000, 86400000};

/***************************************************************************
 * initialize the LCD with the numbers of the interface pins
** This initialization is different from the example code. Ye be warned... */
LiquidCrystal lcd(12, 11, 5, 4, 7, 6);

DHT dht(DHTPIN, DHT11); // INIT the DHT 11 Humiditemp Sensor

/**************************************************************************
 * Vars for Temperature Data manipulation. The corresponding UI[] is where 
 * data displayed to the LCD is posted - not necessary but makes life easier 
 ***************************************************************************/
short current, high, low, oneday, sevenday = 0;
short UI[] = {current, high, low, oneday, sevenday};

    // The Array which stores 1 Minute worth of temperature sensor readings (2 sec intervals)
    short secondly[30];
    
    // The array which stores 1 hour worth of average temperature readings
    short minutely[60];
    
    // The array which stores 24 hours worth of averge temperature readings
    short hourly[24];
    
    // The array which stores 7 days worth of average temperature readings
    short daily[7];

    // yes, it's an index and not a REAL pointer. sue me. 
    short secondlyptr = 0;
    short minutelyptr = 0;
    short hourlyptr = 0;
    short dailyptr = 0;

    // Storage of intervals which trigger data collection & avaraging calculation 
    unsigned long previousMillis = 0;
    unsigned long previousSecondlyMillis = 0;
    unsigned long previousMinutelyMillis = 0;
    unsigned long previousHourlyMillis = 0;
    unsigned long previousDailyMillis = 0;


    // Computes the number of elements in the array. The total array size in bytes divided by the number of bytes per array element gives the number
    // of elements in the array. I did this just in case i need to adjustthe measurement frequency later.. 
    short secondlySize = (sizeof(secondly) / sizeof(secondly[0]));
    short minutelySize = (sizeof(minutely) / sizeof(minutely[0]));
    short hourlySize = (sizeof(hourly) / sizeof(hourly[0]));
    short dailySize = (sizeof(daily) / sizeof(daily[0]));
    
void initSD(){
  Serial.begin(9600);
    while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.print("Initializing SD card...");

  if (!SD.begin(4)) {
      Serial.println("initialization failed!");
      while (1);
     }
  Serial.println("initialization done.");
  }

void writetoSD(short X){
  myFile = SD.open(filename, FILE_WRITE);
  myFile.println(X);
  myFile.close();
  Serial.println(X);
  }

void setup() {
  lcd.begin(16, 2); // set up the LCD's number of columns and rows:
  
  dht.begin(); // Start the Temperature Sensor
  
  lcd.print("hello, world! <3"); // Print a message to the LCD.
  delay(1000);
  lcd.clear();
  
  setDefaultUI();
  
  current = high = low = getTemp(); //Set initializing values so i have something to print and compare later

  initSD(); //This breaks the UI. It's a pin allocation conflict. meh. 
  
}

void loop() {
  //Check the time for all times subroutines
  unsigned long currentMillis, secondlyMillis, minutelyMillis, hourlyMillis;
  currentMillis = secondlyMillis = minutelyMillis = hourlyMillis = millis();

/**********************
 * LIVE DATA SUBROUTINE
 **********************/
 // Millis compare for time between sensor readings. The DHT11 requires at least 1 second
  if (currentMillis - previousMillis >= interval[LIVE]){
    previousMillis = currentMillis; //store previous delay value
    
    current = getTemp();
    //UI[0] = current;  - UI updates happen later... can probably remove this line.

   /*******************************
   * Write new data to the SD Card
   ********************************/
    writetoSD(current);

    secondly[secondlyptr] = current;
    secondlyptr ++;

        // Check if at end of the array. If yes, wrap index back to 0
        if (secondlyptr >= secondlySize ){
          secondlyptr = 0;
          }
    }
  
/***********************************************************************
 * SECONDLY SUBROUTINE - Result is 1 minute average
 * This calculates the average of the last 30 LIVE sensor readings. The
 * secondly[] is filled with data by the LIVE data subroutine above. 
 ***********************************************************************/
//Millis compare on time required for a full rotation of the secondly array. 
  if (secondlyMillis - previousSecondlyMillis >= interval[SECONDLY] ){
    previousSecondlyMillis = secondlyMillis;

  //<todo>break this out into a arraysum function?
    short onehour = 0; // Clear the VAR for computing averages - else averages would go to INFINITY
    for(int i=0; i< secondlySize; i++){
      onehour += secondly[i];
      }
      // total / number of elements = average temperature 
      onehour /= secondlySize;

      //Store 1 minute average for 60 minute average calc
      minutely[minutelyptr] = onehour;
      minutelyptr++;

      // Check if at end of the array. If yes, wrap index back to 0
        if (minutelyptr >= minutelySize ){
          minutelyptr = 0;
          }
    }
    //</todo>

/***************************************************************************************************
 * MINUTELY SUBROUTINE - result is 1 hour average
 * 60 sensor readings * {+/- 200 deg F} = ~12,000. This is 1/3 the range of MAX_SHORT_INT of 36000
 **************************************************************************************************/
  if (minutelyMillis - previousMinutelyMillis >= interval[MINUTELY]){
      previousMinutelyMillis = millis();
    
    short oneHour = 0; // a temp varaible not used anywhere else. This is why it's declared here. 
    
    for (int j=0; j < minutelySize; j++ ){
          oneHour += minutely[j];
        }   
    oneHour /= minutelySize; // solve the average temps for previous hour.

    hourly[hourlyptr] = oneHour;
    hourlyptr++;

    // Check if at end of the array. If yes, wrap index back to 0
    if (hourlyptr >= hourlySize ){
       hourlyptr = 0;
       }
  }
 
/**********************************************
 * HOURLY SUBROUTINE - result is 1 day average
 *********************************************/
  if(hourlyMillis - previousHourlyMillis >= interval[HOURLY]){
    previousHourlyMillis = millis();

    //sum the contents of the array
    oneday = 0;
    for (int k = 0; k < hourlySize; k++){
      oneday += hourly[k];
      }
    oneday /= hourlySize; // solve the average temps for previous hour.
    
    daily[dailyptr] = oneday;
    dailyptr++;
    
    // Check if at end of the array. If yes, wrap index back to 0
    if(dailyptr >= dailySize){
      dailyptr = 0;
      }
    }
 
  if(current > high){
      high = current;
    }

  if(current < low){
      low = current;
    }

/****************************************************************
 * Update the array which stores graphical data to be drawn. 
 * NOTE - super high refresh rate.. could save some effort by
 *        only redrawing the UI when an update needs to be made.. 
 ****************************************************************/
  UI[0] = current;
  UI[1] = high;
  UI[2] = low;
  UI[3] = oneday;
  UI[4] = sevenday;
  drawUI(UI);
}

/* SumArray function UNUSED - No real gains & a pain in my butt to make more abstract than I need it to be.. 
 *  
 * Solves the sum total of integers contained in an array. 
 * NEEDS error checking for larger arrays where sum > MAX SHORT
 * Since this is summing temperatures and no array is larger than 30 elements, 
 * - i'm going to take the lazy route and not fix this.. 
 */
short sumArray(short X[], short XSize){
  
  short sum = 0;

  for (int i = 0; i< XSize; i++){
      sum += X[i]; 
    }
  return(sum);
  }

/* ******************************************************************
 *  setDefaultUI() - draws the initial background on the LCD screen. 
 *  Fresh data will overwrite the # chars. Called during init() only
 ********************************************************************/
void setDefaultUI() {
  String line0 = "C###  H###  L###";
  String line1 = "1Day###  7Day###";
  lcd.setCursor(0,0);
  lcd.print(line0);
  lcd.setCursor(0,1);
  lcd.print(line1);
}

/*********************************************************************
 *  getTemp() - handles temperature measurement sensor readings. 
 *  Returns current sensor reading in Farenheight as an integer. 
 *********************************************************************/
short getTemp(){
  short val = -100;  // init to an impossible value. Should be fun if sensor read fails... 
  float tempF = dht.readTemperature(true);
  val = tempF; // convert float to short int. Floats are too big and ugly.
  return(val);
}

// Private function used by drawUI()
void displayCurrentTemp(short X){
  lcd.setCursor(1,0);
  lcd.print("   ");
  lcd.setCursor(1,0);
  lcd.print(X);
}

// Private function used by drawUI()
void displayHighTemp(short X){
  lcd.setCursor(7,0);
  lcd.print("   ");
  lcd.setCursor(7,0);
  lcd.print(X);
  }

// Private function used by drawUI()
void displayLowTemp(short X){
  lcd.setCursor(13,0);
  lcd.print("   ");
  lcd.setCursor(13,0);
  lcd.print(X);
  }
  
// Private function used by drawUI()
void displayOneDayTemp(short X){
  lcd.setCursor(4,1);
  lcd.print("   ");
  lcd.setCursor(4,1);
  lcd.print(X);
  }

// Private function used by drawUI()
void displaySevenDayTemp(short X){
  lcd.setCursor(13,1);
  lcd.print("   ");
  lcd.setCursor(13,1);
  lcd.print(X);
  }

/****************************************************************
 * SEND THE DATA TO THE LCD
 * Extracts the data to be displayed, then sends each data point 
 * to their corresponding draw functions.
 ****************************************************************/
void drawUI(short X[]){
  displayCurrentTemp(X[0]);
  displayHighTemp(X[1]);
  displayLowTemp(X[2]);
  displayOneDayTemp(X[3]);
  displaySevenDayTemp(X[4]);
  }
 
