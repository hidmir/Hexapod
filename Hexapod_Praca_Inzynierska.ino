#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver firstPWMDriver = Adafruit_PWMServoDriver(0x40);
Adafruit_PWMServoDriver secondPWMDriver = Adafruit_PWMServoDriver(0x41);

#define SERVOMIN  100//150
#define SERVOMAX  485//600
#define SERVO_FREQ 50
#define NULL_VALUE -1

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

enum Side
{
  RIGHT,
  LEFT
};

enum PartType
{
  LOWER,
  UPPER,
  CONNECTOR
};

enum MovementType
{
  SET_HOME_POSITION,
  SET_DEFAULT_POSITION,
  SET_NEUTRAL_POSITION,
  RISE_LEG,
  LOWER_LEG,
  ROTATE_FORWARD,
  ROTATE_BACKWARD
};

class Part
{
  public:
  uint8_t driverNumber;
  uint8_t partNumber;
  int16_t currentAngle;
  int16_t targetAngle;

  Part(){ }
  
  Part(PartType partType, uint8_t numberOfPart)
  {
    driverNumber = partType == CONNECTOR ? 2 :1 ;
    partNumber = numberOfPart;
  }
 };

class Leg 
{
  public:
    Part partsCollection[3];
    
    Leg() { }
    
    Leg(uint8_t lowerPartServoNumber, uint8_t upperPartServoNumber, uint8_t legsConnectorServoNumber)
    {
      partsCollection[0] = Part(LOWER, lowerPartServoNumber);
      partsCollection[1] = Part(UPPER, upperPartServoNumber);
      partsCollection[2] = Part(CONNECTOR, legsConnectorServoNumber);
    }
    
    void riseLeg(uint8_t numberOfIterations)
    {
      partsCollection[0].targetAngle = 50;
      partsCollection[1].targetAngle = 50;

      rotatePartsAsynchronously(numberOfIterations);
      saveCurrentPosition();
    }
    
    void lowerLeg(uint8_t numberOfIterations)
    {
      partsCollection[0].targetAngle = 100;
      partsCollection[1].targetAngle = 100;

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

    void setHomePosition(uint8_t numberOfIterations)
    {
      partsCollection[0].targetAngle = 30;
      partsCollection[1].targetAngle = 0;

      rotatePartsAsynchronously(numberOfIterations);
      saveCurrentPosition();
    }
    
    void setDefaultPosition(uint8_t numberOfIterations)
    {
      partsCollection[0].targetAngle = 100;
      partsCollection[1].targetAngle = 100;
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
        delay(400);
      }
    }
    
  private:
    uint8_t lowerPart;
    uint8_t upperPart;
    uint8_t legsConnector;
    
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
    Hexapod(Leg firstRightLeg, Leg secondRightLeg, Leg thirdRightLeg, Leg firstLeftLeg, Leg secondLeftLeg, Leg thirdLeftLeg)
    {
      legsCollection[0] = firstRightLeg;
      legsCollection[1] = secondRightLeg;
      legsCollection[2] = thirdRightLeg;
      legsCollection[3] = firstLeftLeg;
      legsCollection[4] = secondLeftLeg;
      legsCollection[5] = thirdLeftLeg;
    }

    void moveForward()
    {
      //firstRightLeg.riseLeg(60);
      /*secondRightLeg.riseLeg(60);
      thirdRightLeg.riseLeg(60);
      firstLeftLeg.riseLeg(60);
      secondLeftLeg.riseLeg(60);
      thirdLeftLeg.riseLeg(60);*/
      //firstRightLeg.lowerLeg(60);
      /*secondRightLeg.lowerLeg(60);
      thirdRightLeg.lowerLeg(60);
      firstLeftLeg.lowerLeg(60);
      secondLeftLeg.lowerLeg(60);
      thirdLeftLeg.lowerLeg(60);*/
    }

    void setNeutralPosition()
    {
      /*firstRightLeg.setNeutralPosition();
      secondRightLeg.setNeutralPosition();
      thirdRightLeg.setNeutralPosition();
      firstLeftLeg.setNeutralPosition();
      secondLeftLeg.setNeutralPosition();
      thirdLeftLeg.setNeutralPosition();*/
    }

    void setHomePosition()
    {
      //firstRightLeg.setHomePosition(60);
    }

  private:
    Leg legsCollection[6];

    void setupTargetAngles(MovementType movementType)
    {
      switch(movementType)
      {
        case SET_HOME_POSITION:
        {
          setTargetAngles(30, 0, 90);
        } break;
        case SET_DEFAULT_POSITION:
        {
          setTargetAngles(100, 100, 90);
        } break;
        case SET_NEUTRAL_POSITION:
        {
          setTargetAngles(90, 90, 90);
        } break;
        case RISE_LEG:
        {
          setTargetAngles(50, 50, NULL_VALUE);
        } break;
        case LOWER_LEG:
        {
          setTargetAngles(100, 100, NULL_VALUE);
        } break;
        case ROTATE_FORWARD:
        {
          setTargetAngles(NULL_VALUE, NULL_VALUE, 120);
        } break;
        case ROTATE_BACKWARD:
        {
          setTargetAngles(NULL_VALUE, NULL_VALUE, 60);
        } break;
      }
    }

    void setTargetAngles(uint8_t lowerPartTargetAngle, uint8_t upperPartTargetAngle, uint8_t connectorPartTargetAngle)
    {
      for(uint8_t legIndex = 0; legIndex < 6; legIndex++)
      {
        legsCollection[legIndex].partsCollection[0].targetAngle = lowerPartTargetAngle != NULL_VALUE ? lowerPartTargetAngle : legsCollection[legIndex].partsCollection[0].currentAngle;
        legsCollection[legIndex].partsCollection[1].targetAngle = upperPartTargetAngle != NULL_VALUE ? upperPartTargetAngle : legsCollection[legIndex].partsCollection[1].currentAngle;
        legsCollection[legIndex].partsCollection[2].targetAngle = connectorPartTargetAngle != NULL_VALUE ? connectorPartTargetAngle : legsCollection[legIndex].partsCollection[2].currentAngle;
      }
    }
    
    void rotateLegsAsynchronously(uint8_t numberOfIterations, uint16_t delayTime)
    {
      uint8_t targetAngle;
      int16_t partTargetAngle;
      int16_t partCurrentAngle;
      Part partsCollection[3];
      Part part;
      Leg leg;

      for(uint8_t iteration = 1; iteration <= numberOfIterations; iteration++)
      {
        for(uint8_t legIndex = 0; legIndex < 6; legIndex++)
        {
          leg = legsCollection[legIndex];
          
          for(uint8_t partIndex = 0; partIndex < 3; partIndex++)
          {
            part = partsCollection[partIndex];
            partTargetAngle = (int16_t)leg.partsCollection[partIndex].targetAngle;
            partCurrentAngle = (int16_t)leg.partsCollection[partIndex].currentAngle;
            targetAngle = partCurrentAngle + (((((partTargetAngle - partCurrentAngle)* 100) / numberOfIterations) * iteration) / 100);
            targetAngle = (uint8_t)targetAngle;
            Serial.println(targetAngle);
            rotate(leg.partsCollection[partIndex].driverNumber, leg.partsCollection[partIndex].partNumber, targetAngle);
            delay(delayTime);
          }
        }
      }
    }

    void saveCurrentPosition()
    {
      for(uint8_t legIndex = 0; legIndex < 6; legIndex++)
      {
         for(uint8_t partIndex = 0; partIndex < 3; partIndex++)
         {
           legsCollection[legIndex].partsCollection[partIndex].currentAngle = legsCollection[legIndex].partsCollection[partIndex].targetAngle;
         } 
      }
    } 
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

  //hexapod.setNeutralPosition();

  delay(10);
}

void loop() {
  //hexapod.moveForward();
  //hexapod.setHomePosition();
}
