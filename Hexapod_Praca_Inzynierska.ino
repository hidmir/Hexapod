#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver firstPWMDriver = Adafruit_PWMServoDriver(0x40);
Adafruit_PWMServoDriver secondPWMDriver = Adafruit_PWMServoDriver(0x41);

#define SERVOMIN 100 //150
#define SERVOMAX 485 //600
#define MICROSECONDSMIN 700
#define MICROSECONDSMAX 2100
#define SERVO_FREQ 50
#define NULL_VALUE -1
#define DEFAULT_VALUE -2

void rotate(uint8_t servoDriverNumber, uint8_t servoNumber, uint16_t angleInDegrees)
{
  uint16_t pulseLength = map(angleInDegrees, 0, 180, SERVOMIN, SERVOMAX);

  if (servoDriverNumber == 1)
  {
    firstPWMDriver.setPWM(servoNumber, 0, pulseLength);
  }
  else
  {
    secondPWMDriver.setPWM(servoNumber, 0, pulseLength);
  }
}

void rotateUsingMicroseconds(uint8_t servoDriverNumber, uint8_t servoNumber, uint16_t angleInDegrees)
{
  uint16_t microseconds = map(angleInDegrees, 20, 160, MICROSECONDSMIN, MICROSECONDSMAX);

  if (servoDriverNumber == 1)
  {
    firstPWMDriver.writeMicroseconds(servoNumber, microseconds);
  }
  else
  {
    secondPWMDriver.writeMicroseconds(servoNumber, microseconds);
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

  Part() {}

  Part(PartType partType, uint8_t numberOfPart)
  {
    driverNumber = partType == CONNECTOR ? 2 : 1;
    partNumber = numberOfPart;
  }
};

class Leg
{
public:
  Part partsCollection[3];
  Side side;

  Leg() {}

  Leg(Side robotSide, uint8_t lowerPartServoNumber, uint8_t upperPartServoNumber, uint8_t legsConnectorServoNumber)
  {
    side = robotSide;
    partsCollection[0] = Part(LOWER, lowerPartServoNumber);
    partsCollection[1] = Part(UPPER, upperPartServoNumber);
    partsCollection[2] = Part(CONNECTOR, legsConnectorServoNumber);
  }

  void setNeutralPosition()
  {
    for (uint8_t partIndex = 0; partIndex < 3; partIndex++)
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

    for (uint8_t iteration = 1; iteration <= numberOfIterations; iteration++)
    {
      for (uint8_t partIndex = 0; partIndex < 3; partIndex++)
      {
        partTargetAngle = (int16_t)partsCollection[partIndex].targetAngle;
        partCurrentAngle = (int16_t)partsCollection[partIndex].currentAngle;
        targetAngle = partCurrentAngle + (((((partTargetAngle - partCurrentAngle) * 100) / numberOfIterations) * iteration) / 100);
        targetAngle = (uint8_t)targetAngle;
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

class HexapodSettings
{
public:
  uint8_t numberOfIterations;
  uint16_t delayTime;

  HexapodSettings(uint8_t numberOfIterationsOfMovement, uint16_t delayTimeOfMovement)
  {
    numberOfIterations = numberOfIterationsOfMovement;
    delayTime = delayTimeOfMovement;
  }
};

class Hexapod
{
public:
  Hexapod(Leg firstRightLeg, Leg secondRightLeg, Leg thirdRightLeg, Leg firstLeftLeg, Leg secondLeftLeg, Leg thirdLeftLeg, HexapodSettings hexapodSettings)
  {
    legsCollection[0] = firstRightLeg;
    legsCollection[1] = secondRightLeg;
    legsCollection[2] = thirdRightLeg;
    legsCollection[3] = firstLeftLeg;
    legsCollection[4] = secondLeftLeg;
    legsCollection[5] = thirdLeftLeg;
    defaultNumberOfIterations = hexapodSettings.numberOfIterations;
    defaultDelayTime = hexapodSettings.delayTime;
  }

  void moveForward()
  {
  }

  void setNeutralPosition()
  {
    setupTargetAngles(SET_NEUTRAL_POSITION);

    Leg leg;
    Part part;
    for (uint8_t legIndex = 0; legIndex < 6; legIndex++)
    {
      leg = legsCollection[legIndex];
      for (uint8_t partIndex = 0; partIndex < 3; partIndex++)
      {
        part = leg.partsCollection[partIndex];
        rotate(part.driverNumber, part.partNumber, part.targetAngle);
        delay(400);
      }
    }

    saveCurrentPosition();
  }

  void setHomePosition()
  {
    setupTargetAngles(SET_HOME_POSITION);
    rotateLegsAsynchronously(DEFAULT_VALUE, DEFAULT_VALUE);
    saveCurrentPosition();
  }

  void setDefaultPosition()
  {
    setupTargetAngles(SET_DEFAULT_POSITION);
    rotateLegsAsynchronously(DEFAULT_VALUE, DEFAULT_VALUE);
    saveCurrentPosition();
  }

private:
  Leg legsCollection[6];
  uint8_t defaultNumberOfIterations;
  uint16_t defaultDelayTime;

  void setupTargetAngles(MovementType movementType)
  {
    switch (movementType)
    {
    case SET_HOME_POSITION:
    {
      setTargetAngles(30, 0, 90);
    }
    break;
    case SET_DEFAULT_POSITION:
    {
      setTargetAngles(120, 120, 90);
    }
    break;
    case SET_NEUTRAL_POSITION:
    {
      setTargetAngles(90, 90, 90);
    }
    break;
    case RISE_LEG:
    {
      setTargetAngles(50, 50, NULL_VALUE);
    }
    break;
    case LOWER_LEG:
    {
      setTargetAngles(100, 100, NULL_VALUE);
    }
    break;
    case ROTATE_FORWARD:
    {
      setTargetAngles(NULL_VALUE, NULL_VALUE, 120);
    }
    break;
    case ROTATE_BACKWARD:
    {
      setTargetAngles(NULL_VALUE, NULL_VALUE, 60);
    }
    break;
    default:
    {
      setTargetAngles(90, 90, 90);
    }
    break;
    }
  }

  void setTargetAngles(uint8_t lowerPartTargetAngle, uint8_t upperPartTargetAngle, uint8_t connectorPartTargetAngle)
  {
    for (uint8_t legIndex = 0; legIndex < 6; legIndex++)
    {
      if (legsCollection[legIndex].side == RIGHT)
      {
        legsCollection[legIndex].partsCollection[0].targetAngle = lowerPartTargetAngle != NULL_VALUE ? lowerPartTargetAngle : legsCollection[legIndex].partsCollection[0].currentAngle;
        legsCollection[legIndex].partsCollection[1].targetAngle = upperPartTargetAngle != NULL_VALUE ? upperPartTargetAngle : legsCollection[legIndex].partsCollection[1].currentAngle;
        legsCollection[legIndex].partsCollection[2].targetAngle = connectorPartTargetAngle != NULL_VALUE ? connectorPartTargetAngle : legsCollection[legIndex].partsCollection[2].currentAngle;
      }
      else
      {
        legsCollection[legIndex].partsCollection[0].targetAngle = lowerPartTargetAngle != NULL_VALUE ? (90 + (90 - lowerPartTargetAngle)) : legsCollection[legIndex].partsCollection[0].currentAngle;
        legsCollection[legIndex].partsCollection[1].targetAngle = upperPartTargetAngle != NULL_VALUE ? (90 + (90 - upperPartTargetAngle)) : legsCollection[legIndex].partsCollection[1].currentAngle;
        legsCollection[legIndex].partsCollection[2].targetAngle = connectorPartTargetAngle != NULL_VALUE ? (90 + (90 - connectorPartTargetAngle)) : legsCollection[legIndex].partsCollection[2].currentAngle;
      }
    }
  }

  void rotateLegsAsynchronously(int16_t numberOfIterations, int16_t delayTime)
  {
    uint8_t targetAngle;
    int16_t partTargetAngle;
    int16_t partCurrentAngle;
    Leg leg;
    Part part;
    numberOfIterations = numberOfIterations == DEFAULT_VALUE ? defaultNumberOfIterations : numberOfIterations;
    delayTime = delayTime == DEFAULT_VALUE ? defaultDelayTime : delayTime;

    for (int16_t iteration = 1; iteration <= numberOfIterations; iteration++)
    {
      for (uint8_t legIndex = 0; legIndex < 6; legIndex++)
      {
        leg = legsCollection[legIndex];
        for (uint8_t partIndex = 0; partIndex < 2; partIndex++)
        {
          part = leg.partsCollection[partIndex];
          partTargetAngle = (int16_t)part.targetAngle;
          partCurrentAngle = (int16_t)part.currentAngle;
          if (partTargetAngle == partCurrentAngle)
          {
            continue;
          }
          targetAngle = partCurrentAngle + (((((partTargetAngle - partCurrentAngle) * 100) / numberOfIterations) * iteration) / 100);
          targetAngle = (uint8_t)targetAngle;
          //Serial.println(targetAngle);
          rotate(part.driverNumber, part.partNumber, targetAngle);
          //rotateUsingMicroseconds(part.driverNumber, part.partNumber, targetAngle);
          delay(delayTime);
        }
      }
    }
  }

  void saveCurrentPosition()
  {
    for (uint8_t legIndex = 0; legIndex < 6; legIndex++)
    {
      for (uint8_t partIndex = 0; partIndex < 3; partIndex++)
      {
        legsCollection[legIndex].partsCollection[partIndex].currentAngle = legsCollection[legIndex].partsCollection[partIndex].targetAngle;
      }
    }
  }
};

Leg firstLeg(RIGHT, 0, 1, 15);
Leg secondLeg(RIGHT, 2, 3, 10);
Leg thirdLeg(RIGHT, 4, 5, 2);
Leg fourthLeg(LEFT, 7, 8, 12);
Leg fifthLeg(LEFT, 10, 11, 7);
Leg sixthLeg(LEFT, 13, 15, 4);
HexapodSettings hexapodSettings(30, 20); //iterations delaytime
Hexapod hexapod(firstLeg, secondLeg, thirdLeg, fourthLeg, fifthLeg, sixthLeg, hexapodSettings);

void setup()
{
  Serial.begin(9600);
  firstPWMDriver.begin();
  secondPWMDriver.begin();
  firstPWMDriver.setOscillatorFrequency(27000000);
  secondPWMDriver.setOscillatorFrequency(27000000);
  firstPWMDriver.setPWMFreq(SERVO_FREQ);
  secondPWMDriver.setPWMFreq(SERVO_FREQ);

  hexapod.setNeutralPosition();
  hexapod.setHomePosition();
  //hexapod.setDefaultPosition();

  delay(10);
}

void loop()
{
  //hexapod.moveForward();
  //hexapod.setHomePosition();
  //hexapod.setDefaultPosition();
}
