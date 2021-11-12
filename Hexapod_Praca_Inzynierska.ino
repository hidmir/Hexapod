#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver firstPWMDriver = Adafruit_PWMServoDriver(0x40);
Adafruit_PWMServoDriver secondPWMDriver = Adafruit_PWMServoDriver(0x41);

#define SERVOMIN  100//150)
#define SERVOMAX  485//600
#define SERVO_FREQ 50
#define SERVO1 0
#define SERVO2 1
#define SERVO3 2

void rotate(uint8_t servoDriverNumber, uint8_t servoNumber, uint16_t angleInDegrees)
{
  uint16_t pulseLength = map(angleInDegrees, 0, 180, SERVOMIN, SERVOMAX);

  if(servoDriverNumber == 1)
  {
    firstPWMDriver.setPWM(servoNumber, 0, pulseLength);
  }
  else
  {
    secondPWMDriver.setPWM(servoNumber, 0, pulseLength);
  }
}

enum LegPosition 
{
  DEFAULT_POSITION, 
  UP, 
  DOWN
};

class Leg 
{
  class Part
  {
    public:
    uint8_t driverNumber;
    uint8_t partNumber;
    uint8_t currentAngle;
    uint8_t targetAngle;

    Part(){  }
    
    Part(uint8_t numberOfDriver, uint8_t numberOfPart)
    {
      driverNumber = numberOfDriver;
      partNumber = numberOfPart;
    }
  };
  public:
    Leg(uint8_t lowerPartServoNumber, uint8_t upperPartServoNumber, uint8_t legsConnectorServoNumber)
    {
      //lowerPart = lowerPartServoNumber;
      //upperPart = upperPartServoNumber;
      //legsConnector = legsConnectorServoNumber;
      //partsCollection[1] = upperPartServoNumber;
      //partsCollection[2] = legsConnectorServoNumber;
      partsCollection[0] = Part(1, lowerPartServoNumber);
      partsCollection[1] = Part(1, upperPartServoNumber);
      partsCollection[2] = Part(2, legsConnectorServoNumber);
    }
    void riseLeg(uint8_t numberOfIterations)
    {
      uint8_t targetAngle;
      partsCollection[0].targetAngle = 135;
      partsCollection[1].targetAngle = 180;
      partsCollection[2].targetAngle = 135;
      
      for(uint8_t iteration = 1; iteration <= numberOfIterations; iteration++)
      {
        for(uint8_t partIndex = 0; partIndex < 3; partIndex++)
        {
          targetAngle = partsCollection[partIndex].currentAngle + (((partsCollection[partIndex].targetAngle - partsCollection[partIndex].currentAngle) / numberOfIterations) * iteration);
          rotate(partsCollection[partIndex].driverNumber, partsCollection[partIndex].partNumber, targetAngle);
          delay(10);
        }
      }
      /*rotate(1, lowerPart, 135);
      delay(100);
      rotate(1, upperPart, 180);
      delay(100);
      rotate(2, legsConnector, 0);
      delay(100);*/
    }
    void lowerLeg()
    {
      rotate(1, lowerPart, 90);
      delay(100);
      rotate(1, upperPart, 160);
      delay(100);
      rotate(2, legsConnector, 180);
      delay(100);
    }
    void setNeutralPosition()
    {
      for(uint8_t partIndex = 0; partIndex < 3; partIndex++)
      {
        rotate(partsCollection[partIndex].driverNumber, partsCollection[partIndex].partNumber, 90);
        partsCollection[partIndex].currentAngle = 90;
        delay(1500);
      }
      currentLegPosition = DEFAULT_POSITION;
      //rotate(1, lowerPart, 90);
      //rotate(1, upperPart, 90);
    }
  private:
    uint8_t lowerPart;
    uint8_t upperPart;
    uint8_t legsConnector;
    Part partsCollection[3];
    LegPosition currentLegPosition;
};

Leg firstLeg(SERVO1, SERVO2, 0);

void setup() {
  Serial.begin(9600);
  firstPWMDriver.begin();
  secondPWMDriver.begin();
  firstPWMDriver.setOscillatorFrequency(27000000);
  secondPWMDriver.setOscillatorFrequency(27000000);
  firstPWMDriver.setPWMFreq(SERVO_FREQ);
  secondPWMDriver.setPWMFreq(SERVO_FREQ);

  firstLeg.setNeutralPosition();

  delay(10);
}

void loop() {
  //firstLeg.riseLeg();
  //firstPWMDriver.setPWM(1, 0, 200);
  //secondPWMDriver.setPWM(1, 0, SERVOMIN);
  //delay(2000);
  //firstLeg.lowerLeg();
  //firstPWMDriver.setPWM(1, 0, 300);
  //secondPWMDriver.setPWM(1, 0, SERVOMAX);
  //delay(2000);
  //firstLeg.setNeutralPosition();
  firstLeg.riseLeg(30);
}

/*void setNeutralPosition(uint8_t servoNumbers[])
{
   uint8_t arraySize = sizeof(servoNumbers);
   uint8_t intSize = sizeof(servoNumbers[0]);
   uint8_t length = arraySize / intSize;

  for(uint8_t elementIndex = 0; elementIndex < length; elementIndex++)
  {
    rotate(servoNumbers[elementIndex], 90);
  }
}*/

/*
 * void riseLeg(uint8_t firstServo, uint8_t secondServo)
{
  rotate(firstServo, 135);
  rotate(secondServo, 180);
}

void lowerLeg(uint8_t firstServo, uint8_t secondServo)
{
  rotate(firstServo, 90);
  rotate(secondServo, 160);
}

void movemement1()
{
  rotate(1, 0);
  delay(2000);
  rotate(1, 90);
  delay(2000);
  rotate(1, 180);
  delay(2000);
}

void movemement2()
{
  riseLeg(SERVO1, SERVO2);
  delay(2000);
  lowerLeg(SERVO1, SERVO2);
  delay(2000);
}
 */
