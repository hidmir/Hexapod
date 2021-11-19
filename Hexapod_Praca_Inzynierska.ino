#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver firstPWMDriver = Adafruit_PWMServoDriver(0x40);
Adafruit_PWMServoDriver secondPWMDriver = Adafruit_PWMServoDriver(0x41);

#define SERVOMIN  100//150
#define SERVOMAX  485//600
#define SERVO_FREQ 50

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

class Leg 
{
  class Part
  {
    public:
    uint8_t driverNumber;
    uint8_t partNumber;
    int16_t currentAngle;
    int16_t targetAngle;

    Part(){ }
    
    Part(uint8_t numberOfDriver, uint8_t numberOfPart)
    {
      driverNumber = numberOfDriver;
      partNumber = numberOfPart;
    }
  };
  
  public:
    Leg() { }
    
    Leg(uint8_t lowerPartServoNumber, uint8_t upperPartServoNumber, uint8_t legsConnectorServoNumber)
    {
      partsCollection[0] = Part(1, lowerPartServoNumber);
      partsCollection[1] = Part(1, upperPartServoNumber);
      partsCollection[2] = Part(2, legsConnectorServoNumber);
    }
    
    void riseLeg(uint8_t numberOfIterations)
    {
      partsCollection[0].targetAngle = 60;
      partsCollection[1].targetAngle = 60;

      rotatePartsAsynchronously(numberOfIterations);
      saveCurrentPosition();
    }
    
    void lowerLeg(uint8_t numberOfIterations)
    {
      partsCollection[0].targetAngle = 120;
      partsCollection[1].targetAngle = 120;
      
      rotatePartsAsynchronously(numberOfIterations);
      saveCurrentPosition();
    }
    
    void rotateForward(uint8_t numberOfIterations)
    {
      partsCollection[2].targetAngle = 120;
      
      rotatePartsAsynchronously(numberOfIterations);
      saveCurrentPosition();
    }

    void rotateBackward(uint8_t numberOfIterations)
    {
      partsCollection[2].targetAngle = 60;
      
      rotatePartsAsynchronously(numberOfIterations);
      saveCurrentPosition();
    }
    
    void setDefaultPosition(uint8_t numberOfIterations)
    {
      partsCollection[0].targetAngle = 90;
      partsCollection[1].targetAngle = 90;
      partsCollection[2].targetAngle = 90;
      
      rotatePartsAsynchronously(numberOfIterations);
      saveCurrentPosition();
    }
    
    void setNeutralPosition()
    {
      for(uint8_t partIndex = 0; partIndex < 3; partIndex++)
      {
        rotate(partsCollection[partIndex].driverNumber, partsCollection[partIndex].partNumber, 90);
        partsCollection[partIndex].currentAngle = 90;
        delay(1500);
      }
    }
    
  private:
    uint8_t lowerPart;
    uint8_t upperPart;
    uint8_t legsConnector;
    Part partsCollection[3];
    
    void rotatePartsAsynchronously(uint8_t numberOfIterations)
    {
      uint8_t targetAngle;
      int16_t partTargetAngle;
      int16_t partCurrentAngle;
      
      for(uint8_t iteration = 1; iteration <= numberOfIterations; iteration++)
      {
        for(uint8_t partIndex = 0; partIndex < 3; partIndex++)
        {
          partTargetAngle = (int16_t)partsCollection[partIndex].targetAngle;
          partCurrentAngle = (int16_t)partsCollection[partIndex].currentAngle;
          targetAngle = partCurrentAngle + (((((partTargetAngle - partCurrentAngle)* 100) / numberOfIterations) * iteration) / 100);
          targetAngle = (uint8_t)targetAngle;
          Serial.println(targetAngle);
          rotate(partsCollection[partIndex].driverNumber, partsCollection[partIndex].partNumber, targetAngle);
          delay(10);
        }
      }
    }

    void saveCurrentPosition()
    {
      partsCollection[0].currentAngle = partsCollection[0].targetAngle;
      partsCollection[1].currentAngle = partsCollection[1].targetAngle;
      partsCollection[2].currentAngle = partsCollection[2].targetAngle;
    }
};

class Hexapod
{
  public:
    Hexapod(Leg firstLeg, Leg secondLeg, Leg thirdLeg, Leg fourthLeg, Leg fifthLeg, Leg sixthLeg)
    {
      firstRightLeg = firstLeg;
      secondRightLeg = secondLeg;
      thirdRightLeg = thirdLeg;
      firstLeftLeg = fourthLeg;
      secondLeftLeg = fifthLeg;
      thirdLeftLeg = sixthLeg;
    }

    void setNeutralPosition()
    {
      firstRightLeg.setNeutralPosition();
      secondRightLeg.setNeutralPosition();
      thirdRightLeg.setNeutralPosition();
      firstLeftLeg.setNeutralPosition();
      secondLeftLeg.setNeutralPosition();
      thirdLeftLeg.setNeutralPosition();
    }

    void moveForward()
    {
      firstRightLeg.riseLeg(60);
      secondRightLeg.riseLeg(60);
      thirdRightLeg.riseLeg(60);
      firstLeftLeg.riseLeg(60);
      secondLeftLeg.riseLeg(60);
      thirdLeftLeg.riseLeg(60);
      firstRightLeg.lowerLeg(60);
      secondRightLeg.lowerLeg(60);
      thirdRightLeg.lowerLeg(60);
      firstLeftLeg.lowerLeg(60);
      secondLeftLeg.lowerLeg(60);
      thirdLeftLeg.lowerLeg(60);
    }

  private:
    Leg firstRightLeg;
    Leg secondRightLeg;
    Leg thirdRightLeg;
    Leg firstLeftLeg;
    Leg secondLeftLeg;
    Leg thirdLeftLeg;
};

Leg firstLeg(0, 1, 0);
Leg secondLeg(2, 3, 2);
Leg thirdLeg(4, 5, 4);
Leg fourthLeg(7, 8, 6);
Leg fifthLeg(10, 11, 8);
Leg sixthLeg(13, 15, 10);
Hexapod hexapod(firstLeg, secondLeg, thirdLeg, fourthLeg, fifthLeg, sixthLeg);

void setup() {
  Serial.begin(9600);
  firstPWMDriver.begin();
  secondPWMDriver.begin();
  firstPWMDriver.setOscillatorFrequency(27000000);
  secondPWMDriver.setOscillatorFrequency(27000000);
  firstPWMDriver.setPWMFreq(SERVO_FREQ);
  secondPWMDriver.setPWMFreq(SERVO_FREQ);

  hexapod.setNeutralPosition();

  delay(10);
}

void loop() {
  hexapod.moveForward();
}
