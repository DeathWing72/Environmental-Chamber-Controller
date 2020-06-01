static int batTempSensorPin = A0;
static int ambientTempSensorPin = A1;
static int voltage1Pin = A2;
static int voltage2Pin = A3;
static int freezerPin = 2;
static int controlPinButton = 3;
static int redRgbPin = 5;
static int greenRgbPin = 6;
static int blueRgbPin = 9;
static int batteryLedPin = 11;
static int ambientLedPin = 12;
static double compareTolerance = (3.1/1.8)/2;
static double voltsPerBit = 3.3/1023;
volatile int freezerControlPin = ambientTempSensorPin;
volatile int tempTarget = 0;
volatile int tempTolerance = 3;
volatile unsigned long timeStarted;
volatile unsigned long timeRunning = 0;
volatile unsigned long timeSwitchedToCurrentState = 0;
volatile int tempHistory[] = {0,0,0,0,0,0,0,0,0,0};
volatile boolean freezerOn = false;

void setup() {
  analogReference(EXTERNAL);
  pinMode(freezerPin,OUTPUT);
  pinMode(controlPinButton,INPUT_PULLUP);
  pinMode(batteryLedPin,OUTPUT);
  pinMode(ambientLedPin,OUTPUT);
  digitalWrite(ambientLedPin,HIGH);
  colorChange(0,0,0);
  Serial.begin(9600);
  Serial.println("Please enter the target temperature in Fahrenheit:");
  tempTarget = serialIntResponse();
  Serial.println("°F");
  Serial.println("Please enter the tolerance for acceptable temperature in Fahrenheit:");
  tempTolerance = serialIntResponse();
  Serial.println("°F");
  Serial.println("Time In Current State (s),Freezer On?,Freezer Control Sensor,Average Control Temperature (°F),Ambient Temperature (°F),Battery Temperature (°F),Voltage 1,Voltage 2");
  timeStarted = (long)(millis()/1000);
}
void loop() {
  if(timeRunning%2 == 0)
    freezerControl(convertTemp(freezerControlPin));
  if(timeRunning >= 20)
    rgbLedControl();
  Serial.print(timeRunning-timeSwitchedToCurrentState);
  Serial.print(",");
  Serial.print(freezerOn);
  Serial.print(",");
  if(freezerControlPin == ambientTempSensorPin)
    Serial.print("Ambient Temperature Sensor");
  else if(freezerControlPin == batTempSensorPin)
    Serial.print("Battery Temperature Sensor");
  Serial.print(",");
  Serial.print(averageTempHistory());
  Serial.print(",");
  Serial.print(convertTemp(ambientTempSensorPin));
  Serial.print(",");
  Serial.print(convertTemp(batTempSensorPin));
  Serial.print(",");
  Serial.print(convertVolt(voltage1Pin));
  Serial.print(",");
  Serial.println(convertVolt(voltage2Pin));
  delay(1000);
  timeRunning = (long)(millis()/1000)-timeStarted;
  attachInterrupt(digitalPinToInterrupt(controlPinButton), switchFreezerControlPin, RISING);
}
void switchFreezerControlPin() {
  if(freezerControlPin == ambientTempSensorPin) {
    freezerControlPin = batTempSensorPin;
    digitalWrite(batteryLedPin,HIGH);
    digitalWrite(ambientLedPin,LOW);
  }
  else if(freezerControlPin == batTempSensorPin) {
    freezerControlPin = ambientTempSensorPin;
    digitalWrite(ambientLedPin,HIGH);
    digitalWrite(batteryLedPin,LOW);
  }
}
void freezerControl(int tempF) {
  advanceTempHistory(tempF);
  if(timeRunning-timeSwitchedToCurrentState >= 20) {
    if(averageTempHistory() <= tempTarget-(tempTolerance/2) && freezerOn == true) {
      freezerOn = false;
      toggleFreezerState();
    }
    else if(averageTempHistory() >= tempTarget+(tempTolerance/2) && freezerOn == false) {
      freezerOn = true;
      toggleFreezerState();
    }
  }
}
void toggleFreezerState() {
  if(freezerOn)
    digitalWrite(freezerPin,HIGH);
  else
    digitalWrite(freezerPin,LOW);
  timeSwitchedToCurrentState = (long)(millis()/1000)-timeStarted;
}
void rgbLedControl() {
  int tempF = averageTempHistory();
  int minTempTolerance = tempTarget-tempTolerance;
  int maxTempTolerance = tempTarget+tempTolerance;
  int range = maxTempTolerance - minTempTolerance;
  int tempBits = (1020/range)*(tempF-minTempTolerance);
  if(tempBits >= 0 && tempBits <= 255)
    colorChange(0,tempBits,255);
  else if(tempBits > 255 && tempBits <= 510)
    colorChange(0,255,255-(tempBits-255));
  else if(tempBits > 510 && tempBits <= 765)
    colorChange(tempBits-510,255,0);
  else if(tempBits > 765 && tempBits <= 1020)
    colorChange(255,255-(tempBits-765),0);
  else if(tempBits < 0)
    colorChange(0,0,255);
  else if(tempBits > 1020)
    colorChange(255,0,0);
}
int serialIntResponse() {
  Serial.read();
  while(Serial.available() == 0)
    delay(10);
  int response = Serial.parseInt();
  Serial.read();
  Serial.print(response);
  return response;
}
void advanceTempHistory(int tempF) {
  for(int i = 0;i < 9;i++) {
    tempHistory[i] = tempHistory[i+1];
  }
  tempHistory[9] = tempF;
}
int averageTempHistory() {
  int sum = 0;
  for(int i = 0;i <= 9;i++) {
    sum += tempHistory[i];
  }
  return sum/10;
}
double convertVolt(int pin) {
  int sensorValue = analogRead(pin);
  double voltage = sensorValue*voltsPerBit;
  return voltage;
}
int convertTemp(int pin) {
  int sensorValue = analogRead(pin);
  int tempF = 0;
  for(int i = 0;i < 342;i++) {
    double sensorVal = i*(3.1/1.8)+31;
    if(sensorValue >= sensorVal-compareTolerance && sensorValue < sensorVal+compareTolerance) {
      tempF = i-40;
      break;
    }
  }
  return tempF;
}
void colorChange(int r, int g, int b) {
  analogWrite(redRgbPin,255-r);
  analogWrite(greenRgbPin,255-g);
  analogWrite(blueRgbPin,255-b);
}
