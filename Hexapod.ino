#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <IRremote.h>

int RECV_PIN = 2;
IRrecv irrecv(RECV_PIN);
decode_results results;
Adafruit_PWMServoDriver firstPWMDriver = Adafruit_PWMServoDriver(0x40);
Adafruit_PWMServoDriver secondPWMDriver = Adafruit_PWMServoDriver(0x41);

#define SERVOMIN 100
#define SERVOMAX 485
#define MICROSECONDSMIN 700
#define MICROSECONDSMAX 2100
#define SERVO_FREQ 50
#define NULL_VALUE -1
#define DEFAULT_VALUE -2
#define ON_OFF_SIGNAL -522155913
#define TETRAPOD_GAIT_SIGNAL -522182433
#define WAVE_GAIT_SIGNAL -522149793
#define ROTATE_TO_RIGHT_SIGNAL -522166113
#define ROTATE_TO_LEFT_SIGNAL -522186513

//Codes:
/*
 * ON/OFF - Received signal: -522155913, hex: E0E08877
 * TETRAPOD GAIT - Received signal: -522182433, hex: E0E020DF
 * WAVE GAIT - Received signal: -522149793, hex: E0E0A05F
 * ROTATE TO RIGHT - Received signal: -522166113, hex: E0E0609F
 * ROTATE TO LEFT - Received signal: -522186513, hex: E0E010EF
 */

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

enum LegPosition
{
  FRONT,
  MIDDLE,
  BACK
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
  ROTATE_TO_RIGHT,
  ROTATE_TO_LEFT,
  ROTATE_TO_MIDDLE
};

enum GaitType
{
  WAVE,
  TETRAPOD,
  TRIPOD
};

class Part
{
public:
  uint8_t driverNumber;
  uint8_t partNumber;
  int16_t currentAngle;
  int16_t targetAngle;
  int16_t angleAtBeginningOfMove;

  Part() {}

  Part(PartType partType, uint8_t numberOfPart)
  {
    driverNumber = partType == CONNECTOR ? 2 : 1;
    partNumber = numberOfPart;
    currentAngle = 0;
    targetAngle = 0;
    angleAtBeginningOfMove = 0;
  }
};

class Leg
{
public:
  Part partsCollection[3];
  Side side;
  LegPosition legPosition;
  uint8_t movementState;
  int16_t currentIteration;
  int16_t movementIterations;
  int16_t iterationsMultiplier;
  int16_t numberOfAllIterations;
  bool enabled;

  Leg() {}

  Leg(Side robotSide, uint8_t lowerPartServoNumber, uint8_t upperPartServoNumber, uint8_t legsConnectorServoNumber, LegPosition currentLegPosition, int16_t numberOfMovementIterations)
  {
    side = robotSide;
    legPosition = currentLegPosition;
    partsCollection[0] = Part(LOWER, lowerPartServoNumber);
    partsCollection[1] = Part(UPPER, upperPartServoNumber);
    partsCollection[2] = Part(CONNECTOR, legsConnectorServoNumber);
    movementState = 0;
    movementIterations = numberOfMovementIterations;
    currentIteration = movementIterations;
    iterationsMultiplier = 1;
    enabled = false;
    numberOfAllIterations = 0;
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

  void setTargetAngles(int16_t tibiaTargetAngle, int16_t femurTargetAngle, int16_t coxaTargetAngle, bool reverseAngles)
  {
    partsCollection[0].targetAngle = (reverseAngles == false || side == RIGHT) ? tibiaTargetAngle : (90 + (90 - tibiaTargetAngle));
    partsCollection[1].targetAngle = (reverseAngles == false || side == RIGHT) ? femurTargetAngle : (90 + (90 - femurTargetAngle));
    partsCollection[2].targetAngle = (reverseAngles == false || side == RIGHT) ? coxaTargetAngle : (90 + (90 - coxaTargetAngle));
  }

  bool isLegInTargetPosition()
  {
    return currentIteration >= movementIterations;
  }

  void saveCurrentPosition()
  {
    partsCollection[0].currentAngle = partsCollection[0].targetAngle;
    partsCollection[1].currentAngle = partsCollection[1].targetAngle;
    partsCollection[2].currentAngle = partsCollection[2].targetAngle;
    partsCollection[0].angleAtBeginningOfMove = partsCollection[0].targetAngle;
    partsCollection[1].angleAtBeginningOfMove = partsCollection[1].targetAngle;
    partsCollection[2].angleAtBeginningOfMove = partsCollection[2].targetAngle;
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
    defaultNumberOfMovementIterations = hexapodSettings.numberOfIterations;
    defaultMovementDelayTime = hexapodSettings.delayTime;
  }

  void moveForward(GaitType gaitType)
  {
    Leg leg;
    Part part;
    int16_t swipeMultiplier;
    int16_t moveMultiplier;
    int16_t moveOrder;

    switch (gaitType)
    {
      case WAVE:
      {
        swipeMultiplier = 10;
      }
      break;
      case TETRAPOD:
      {
        swipeMultiplier = 10;
      }
      break;
      case TRIPOD:
      {
        swipeMultiplier = 10;
      }
      break;
      default:
      {
        swipeMultiplier = 10;
      }
      break;
    }

    switch (gaitType)
    {
      case WAVE:
      {
        moveMultiplier = 1;
      }
      break;
      case TETRAPOD:
      {
        moveMultiplier = 2;
      }
      break;
      case TRIPOD:
      {
        moveMultiplier = 1;
      }
      break;
      default:
      {
        moveMultiplier = 1;
      }
      break;
    }

    for (uint8_t legIndex = 0; legIndex < 6; legIndex++)
    {
      leg = legsCollection[legIndex];
      
      if (leg.isLegInTargetPosition() == true)
      {
        legsCollection[legIndex].currentIteration = 1;

        legsCollection[legIndex].saveCurrentPosition();

        switch (leg.movementState)
        {
          case 0:
          {
            switch (leg.legPosition)
            {
              case FRONT:
              {
                legsCollection[legIndex].setTargetAngles(63, 55, 70, leg.side != RIGHT);
              }
              break;
              case MIDDLE:
              {
                legsCollection[legIndex].setTargetAngles(47, 54, 90, leg.side != RIGHT);
              }
              break;
              case BACK:
              { 
                legsCollection[legIndex].setTargetAngles(63, 55, 110, leg.side != RIGHT);
              }
              break;
            }
            legsCollection[legIndex].movementState = 1;
            legsCollection[legIndex].iterationsMultiplier = swipeMultiplier;
          }
          break;
          case 1:
          {
            switch (leg.legPosition)
            {
              case FRONT:
              {
                legsCollection[legIndex].setTargetAngles(121, 107, 79, leg.side != RIGHT);
              }
              break;
              case MIDDLE:
              {
                legsCollection[legIndex].setTargetAngles(95, 106, 108, leg.side != RIGHT);
              }
              break;
              case BACK:
              {
                legsCollection[legIndex].setTargetAngles(98, 105, 123, leg.side != RIGHT);
              }
              break;
            }
            legsCollection[legIndex].movementState = 2;
            legsCollection[legIndex].iterationsMultiplier = swipeMultiplier;
          }
          break;
          case 2:
          {
            switch (leg.legPosition)
            {
              case FRONT:
              {
                legsCollection[legIndex].setTargetAngles(98, 105, 57, leg.side != RIGHT);
              }
              break;
              case MIDDLE:
              {
                legsCollection[legIndex].setTargetAngles(95, 106, 72, leg.side != RIGHT);
              }
              break;
              case BACK:
              {
                legsCollection[legIndex].setTargetAngles(121, 107, 101, leg.side != RIGHT);
              }
              break;
            }
            legsCollection[legIndex].movementState = 0;
            legsCollection[legIndex].iterationsMultiplier = moveMultiplier;
          }
          break;
        }
      }
    }
    // r3 l2//r2 l1//r1 l3//
    switch (gaitType)
    {
      case WAVE:
      {
        legsCollection[0].enabled = true;
      }
      break;
      case TETRAPOD:
      {
        legsCollection[0].enabled = true;
        legsCollection[5].enabled = true;
      }
      break;
      case TRIPOD:
      {
        legsCollection[0].enabled = true;
      }
      break;
      default:
      {
        legsCollection[0].enabled = true;
      }
      break;
    }

    for (uint8_t legIndex = 0; legIndex < 6; legIndex++)
    {
      cachedLeg = legsCollection[legIndex];

      if (cachedLeg.enabled == false)
      {
        continue;
      }

      for (uint8_t partIndex = 0; partIndex < 3; partIndex++)
      {
        cachedPart = cachedLeg.partsCollection[partIndex];
        cachedPartTargetAngle = (int16_t)cachedPart.targetAngle;
        cachedPartCurrentAngle = (int16_t)cachedPart.angleAtBeginningOfMove;

        if (cachedPartTargetAngle == cachedPartCurrentAngle)
        {
          continue;
        }

        cachedPartTargetAngle = cachedPartCurrentAngle + (((((cachedPartTargetAngle - cachedPartCurrentAngle) * 100) / cachedLeg.movementIterations) * cachedLeg.currentIteration) / 100);
        cachedPartTargetAngle = (uint8_t)cachedPartTargetAngle;
        rotate(cachedPart.driverNumber, cachedPart.partNumber, cachedPartTargetAngle);

        legsCollection[legIndex].partsCollection[partIndex].currentAngle = cachedPartTargetAngle;

        delay(5);
      }

      if (cachedLeg.currentIteration < cachedLeg.movementIterations)
      {
        legsCollection[legIndex].currentIteration = legsCollection[legIndex].currentIteration + (1 * legsCollection[legIndex].iterationsMultiplier);
        legsCollection[legIndex].numberOfAllIterations++;
      }
    }

    // r1 l3//r3 l2//r2 l1//

    moveOrder = (gaitType == TRIPOD) ? 1 : (gaitType == TETRAPOD) ? 4
                                       : (gaitType == WAVE)       ? 1
                                                                  : 1;

    if (legsCollection[1].enabled == false && legsCollection[0].numberOfAllIterations >= (((legsCollection[0].movementIterations / moveMultiplier) + ((legsCollection[0].movementIterations * 2) / swipeMultiplier)) / 6 * moveOrder))
    {
      legsCollection[1].enabled = true;
    }

    moveOrder = (gaitType == TRIPOD) ? 1 : (gaitType == TETRAPOD) ? 2
                                       : (gaitType == WAVE)       ? 2
                                                                  : 2;

    if (legsCollection[2].enabled == false && legsCollection[0].numberOfAllIterations >= ((((legsCollection[0].movementIterations / moveMultiplier) + ((legsCollection[0].movementIterations * 2) / swipeMultiplier)) / 6) * moveOrder))
    {
      legsCollection[2].enabled = true;
    }

    moveOrder = (gaitType == TRIPOD) ? 1 : (gaitType == TETRAPOD) ? 4
                                       : (gaitType == WAVE)       ? 3
                                                                  : 3;

    if (legsCollection[3].enabled == false && legsCollection[0].numberOfAllIterations >= ((((legsCollection[0].movementIterations / moveMultiplier) + ((legsCollection[0].movementIterations * 2) / swipeMultiplier)) / 6) * moveOrder))
    {
      legsCollection[3].enabled = true;
    }

    moveOrder = (gaitType == TRIPOD) ? 1 : (gaitType == TETRAPOD) ? 2
                                       : (gaitType == WAVE)       ? 4
                                                                  : 4;

    if (legsCollection[4].enabled == false && legsCollection[0].numberOfAllIterations >= ((((legsCollection[0].movementIterations / moveMultiplier) + ((legsCollection[0].movementIterations * 2) / swipeMultiplier)) / 6) * moveOrder))
    {
      legsCollection[4].enabled = true;
    }

    moveOrder = (gaitType == TRIPOD) ? 1 : (gaitType == TETRAPOD) ? 1
                                       : (gaitType == WAVE)       ? 5
                                                                  : 5;

    if (legsCollection[5].enabled == false && legsCollection[0].numberOfAllIterations >= ((((legsCollection[0].movementIterations / moveMultiplier) + ((legsCollection[0].movementIterations * 2) / swipeMultiplier)) / 6) * moveOrder))
    {
      legsCollection[5].enabled = true;
    }
  }

  void rotateToRight()
  {
    setTargetLegs(true, false, true, false, true, false);
    move(RISE_LEG, DEFAULT_VALUE, DEFAULT_VALUE);
    move(ROTATE_TO_RIGHT, DEFAULT_VALUE, DEFAULT_VALUE);
    move(LOWER_LEG, DEFAULT_VALUE, DEFAULT_VALUE);
    setTargetLegs(false, true, false, true, false, true);
    move(RISE_LEG, DEFAULT_VALUE, DEFAULT_VALUE);
    move(ROTATE_TO_RIGHT, DEFAULT_VALUE, DEFAULT_VALUE);
    move(LOWER_LEG, DEFAULT_VALUE, DEFAULT_VALUE);
    setTargetLegs(true, true, true, true, true, true);
    move(ROTATE_TO_LEFT, DEFAULT_VALUE, DEFAULT_VALUE);
    setTargetLegs(true, false, true, false, true, false);
    move(RISE_LEG, DEFAULT_VALUE, DEFAULT_VALUE);
    move(ROTATE_TO_RIGHT, DEFAULT_VALUE, DEFAULT_VALUE);
    move(LOWER_LEG, DEFAULT_VALUE, DEFAULT_VALUE);
    setTargetLegs(false, true, false, true, false, true);
    move(RISE_LEG, DEFAULT_VALUE, DEFAULT_VALUE);
    move(ROTATE_TO_RIGHT, DEFAULT_VALUE, DEFAULT_VALUE);
    move(LOWER_LEG, DEFAULT_VALUE, DEFAULT_VALUE);
    setTargetLegs(true, true, true, true, true, true);
    move(ROTATE_TO_MIDDLE, DEFAULT_VALUE, DEFAULT_VALUE);
  }

  void rotateToLeft()
  {
    setTargetLegs(true, false, true, false, true, false);
    move(RISE_LEG, DEFAULT_VALUE, DEFAULT_VALUE);
    move(ROTATE_TO_LEFT, DEFAULT_VALUE, DEFAULT_VALUE);
    move(LOWER_LEG, DEFAULT_VALUE, DEFAULT_VALUE);
    setTargetLegs(false, true, false, true, false, true);
    move(RISE_LEG, DEFAULT_VALUE, DEFAULT_VALUE);
    move(ROTATE_TO_LEFT, DEFAULT_VALUE, DEFAULT_VALUE);
    move(LOWER_LEG, DEFAULT_VALUE, DEFAULT_VALUE);
    setTargetLegs(true, true, true, true, true, true);
    move(ROTATE_TO_RIGHT, DEFAULT_VALUE, DEFAULT_VALUE);
    setTargetLegs(true, false, true, false, true, false);
    move(RISE_LEG, DEFAULT_VALUE, DEFAULT_VALUE);
    move(ROTATE_TO_LEFT, DEFAULT_VALUE, DEFAULT_VALUE);
    move(LOWER_LEG, DEFAULT_VALUE, DEFAULT_VALUE);
    setTargetLegs(false, true, false, true, false, true);
    move(RISE_LEG, DEFAULT_VALUE, DEFAULT_VALUE);
    move(ROTATE_TO_LEFT, DEFAULT_VALUE, DEFAULT_VALUE);
    move(LOWER_LEG, DEFAULT_VALUE, DEFAULT_VALUE);
    setTargetLegs(true, true, true, true, true, true);
    move(ROTATE_TO_MIDDLE, DEFAULT_VALUE, DEFAULT_VALUE);
  }

  void safeSetDefaultPosition()
  {
     setTargetLegs(true, false, true, false, true, false);
     move(RISE_LEG, DEFAULT_VALUE, DEFAULT_VALUE);
     move(ROTATE_TO_MIDDLE, DEFAULT_VALUE, DEFAULT_VALUE);
     move(LOWER_LEG, DEFAULT_VALUE, DEFAULT_VALUE);
     setTargetLegs(false, true, false, true, false, true);
     move(RISE_LEG, DEFAULT_VALUE, DEFAULT_VALUE);
     move(ROTATE_TO_MIDDLE, DEFAULT_VALUE, DEFAULT_VALUE);
     move(LOWER_LEG, DEFAULT_VALUE, DEFAULT_VALUE);
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
    setTargetLegs(true, true, true, true, true, true);
    setupTargetAngles(SET_HOME_POSITION);
    rotateLegsAsynchronously(DEFAULT_VALUE, DEFAULT_VALUE);
    saveCurrentPosition();
  }

  void setDefaultPosition()
  {
    setTargetLegs(true, true, true, true, true, true);
    setupTargetAngles(SET_DEFAULT_POSITION);
    rotateLegsAsynchronously(DEFAULT_VALUE, DEFAULT_VALUE);
    saveCurrentPosition();
  }

  void lowerLegs()
  {
    setTargetLegs(true, true, true, true, true, true);
    move(LOWER_LEG, DEFAULT_VALUE, DEFAULT_VALUE);
  }

  void disableAllLegs()
  {
    for (uint8_t legIndex = 0; legIndex < 6; legIndex++)
    {
      legsCollection[legIndex].enabled = false;
    }
  }

  void resetLegsMovementIterations()
  {
    for (uint8_t legIndex = 0; legIndex < 6; legIndex++)
    {
      legsCollection[legIndex].currentIteration = legsCollection[legIndex].movementIterations;
      legsCollection[legIndex].numberOfAllIterations = 0;
    }
  }

  void resetLegsMovementStates()
  {
    for (uint8_t legIndex = 0; legIndex < 6; legIndex++)
    {
      legsCollection[legIndex].movementState = 0;
    }
  }

  void resetLegsTargetAngles()
  {
    for (uint8_t legIndex = 0; legIndex < 6; legIndex++)
    {
      legsCollection[legIndex].partsCollection[0].targetAngle = legsCollection[legIndex].partsCollection[0].currentAngle;
      legsCollection[legIndex].partsCollection[1].targetAngle = legsCollection[legIndex].partsCollection[1].currentAngle;
      legsCollection[legIndex].partsCollection[2].targetAngle = legsCollection[legIndex].partsCollection[2].currentAngle;
    }
  }

  void resetLegs()
  {
    disableAllLegs();
    resetLegsMovementIterations();
    resetLegsMovementStates();
    resetLegsTargetAngles();
  }

private:
  Leg legsCollection[6];
  bool targetLegsCollection[6] = {true, true, true, true, true, true};
  uint8_t defaultNumberOfMovementIterations;
  uint16_t defaultMovementDelayTime;

  uint8_t cachedTargetAngle;
  int16_t cachedPartTargetAngle;
  int16_t cachedPartCurrentAngle;
  Leg cachedLeg;
  Part cachedPart;

  void move(MovementType movementType, int16_t numberOfIterations, int16_t delayTime)
  {
    setupTargetAngles(movementType);
    rotateLegsAsynchronously(numberOfIterations, delayTime);
    saveCurrentPosition();
  }

  void setupTargetAngles(MovementType movementType)
  {
    switch (movementType)
    {
    case SET_HOME_POSITION:
    {
      setTargetAngles(30, 25, 90, true);
    }
    break;
    case SET_DEFAULT_POSITION:
    {
      setTargetAngles(110, 110, 90, true);
    }
    break;
    case SET_NEUTRAL_POSITION:
    {
      setTargetAngles(90, 90, 90, true);
    }
    break;
    case RISE_LEG:
    {
      setTargetAngles(50, 50, NULL_VALUE, true);
    }
    break;
    case LOWER_LEG:
    {
      setTargetAngles(110, 110, NULL_VALUE, true);
    }
    break;
    case ROTATE_TO_RIGHT:
    {
      setTargetAngles(NULL_VALUE, NULL_VALUE, 110, false);
    }
    break;
    case ROTATE_TO_LEFT:
    {
      setTargetAngles(NULL_VALUE, NULL_VALUE, 70, false);
    }
    break;
    case ROTATE_TO_MIDDLE:
    {
      setTargetAngles(NULL_VALUE, NULL_VALUE, 90, false);
    }
    break;
    default:
    {
      setTargetAngles(90, 90, 90, true);
    }
    break;
    }
  }

  void setTargetAngles(int16_t tibiaTargetAngle, int16_t femurTargetAngle, int16_t coxaTargetAngle, bool reverseAnglesForOtherSideOfRobot)
  {
    for (uint8_t legIndex = 0; legIndex < 6; legIndex++)
    {
      if (targetLegsCollection[legIndex] == false)
      {
        continue;
      }

      if (reverseAnglesForOtherSideOfRobot == false || legsCollection[legIndex].side == RIGHT)
      {
        legsCollection[legIndex].partsCollection[0].targetAngle = tibiaTargetAngle != NULL_VALUE ? tibiaTargetAngle : legsCollection[legIndex].partsCollection[0].currentAngle;
        legsCollection[legIndex].partsCollection[1].targetAngle = femurTargetAngle != NULL_VALUE ? femurTargetAngle : legsCollection[legIndex].partsCollection[1].currentAngle;
        legsCollection[legIndex].partsCollection[2].targetAngle = coxaTargetAngle != NULL_VALUE ? coxaTargetAngle : legsCollection[legIndex].partsCollection[2].currentAngle;
      }
      else
      {
        legsCollection[legIndex].partsCollection[0].targetAngle = tibiaTargetAngle != NULL_VALUE ? (90 + (90 - tibiaTargetAngle)) : legsCollection[legIndex].partsCollection[0].currentAngle;
        legsCollection[legIndex].partsCollection[1].targetAngle = femurTargetAngle != NULL_VALUE ? (90 + (90 - femurTargetAngle)) : legsCollection[legIndex].partsCollection[1].currentAngle;
        legsCollection[legIndex].partsCollection[2].targetAngle = coxaTargetAngle != NULL_VALUE ? (90 + (90 - coxaTargetAngle)) : legsCollection[legIndex].partsCollection[2].currentAngle;
      }
    }
  }

  void setTargetLegs(bool useFirstRightLeg, bool useSecondRightLeg, bool useThirdRightLeg, bool useFirstLeftLeg, bool useSecondLeftLeg, bool useThirdLeftLeg)
  {
    targetLegsCollection[0] = useFirstRightLeg;
    targetLegsCollection[1] = useSecondRightLeg;
    targetLegsCollection[2] = useThirdRightLeg;
    targetLegsCollection[3] = useFirstLeftLeg;
    targetLegsCollection[4] = useSecondLeftLeg;
    targetLegsCollection[5] = useThirdLeftLeg;
  }

  void rotateLegsAsynchronously(int16_t numberOfIterations, int16_t delayTime)
  {
    uint8_t targetAngle; int16_t partTargetAngle; int16_t partCurrentAngle; 
    Leg leg; Part part;
    numberOfIterations = numberOfIterations == DEFAULT_VALUE ? 
      defaultNumberOfMovementIterations : numberOfIterations;
    delayTime = delayTime == DEFAULT_VALUE ? 
      defaultMovementDelayTime : delayTime;
    for (int16_t iteration = 1; iteration <= numberOfIterations; iteration++)
    {
      for (uint8_t legIndex = 0; legIndex < 6; legIndex++)
      {
        if (targetLegsCollection[legIndex] == false)
        {
          continue;
        }
        leg = legsCollection[legIndex];
        for (uint8_t partIndex = 0; partIndex < 3; partIndex++)
        {
          part = leg.partsCollection[partIndex];
          partTargetAngle = (int16_t)part.targetAngle;
          partCurrentAngle = (int16_t)part.currentAngle;
          if (partTargetAngle == partCurrentAngle)
          {
            continue;
          }
          targetAngle = partCurrentAngle + 
            (((((partTargetAngle - partCurrentAngle) * 100) / 
              numberOfIterations) * iteration) / 100);
          targetAngle = (uint8_t)targetAngle;
          rotate(part.driverNumber, part.partNumber, targetAngle);
          delay(delayTime);
        }
      }
    }
  }

  void saveCurrentPosition()
  {
    for (uint8_t legIndex = 0; legIndex < 6; legIndex++)
    {
      if (targetLegsCollection[legIndex] == false)
      {
        continue;
      }

      for (uint8_t partIndex = 0; partIndex < 3; partIndex++)
      {
        legsCollection[legIndex].partsCollection[partIndex].currentAngle = legsCollection[legIndex].partsCollection[partIndex].targetAngle;
      }
    }
  }
};

Leg firstLeg(RIGHT, 1, 2, 14, FRONT, 90);
Leg secondLeg(RIGHT, 3, 4, 10, MIDDLE, 90);
Leg thirdLeg(RIGHT, 5, 6, 2, BACK, 90);
Leg fourthLeg(LEFT, 7, 8, 12, FRONT, 90);
Leg fifthLeg(LEFT, 10, 11, 7, MIDDLE, 90);
Leg sixthLeg(LEFT, 13, 15, 4, BACK, 90);
HexapodSettings hexapodSettings(40, 10);
Hexapod hexapod(firstLeg, secondLeg, thirdLeg, fourthLeg, fifthLeg, sixthLeg, hexapodSettings);
bool isHexapodReadyToMove;
bool isTetrapodGateEnabled;
bool isWaveGateEnabled;
bool isTurningRightEnabled;
bool isTurningLeftEnabled;
bool wasTetrapodGateEnabledBefore;
bool wasWaveGateEnabledBefore;

void setup()
{
  Serial.begin(9600);
  irrecv.enableIRIn();
  firstPWMDriver.begin();
  secondPWMDriver.begin();
  firstPWMDriver.setOscillatorFrequency(27000000);
  secondPWMDriver.setOscillatorFrequency(27000000);
  firstPWMDriver.setPWMFreq(SERVO_FREQ);
  secondPWMDriver.setPWMFreq(SERVO_FREQ);

  hexapod.setNeutralPosition();

  hexapod.setHomePosition();

  pinMode(3,OUTPUT);
  digitalWrite(3,HIGH);
  
  delay(10);
}

void loop()
{
  if (irrecv.decode(&results)) 
  {
   logReceivedCode(results.value);
   updateHexapodState();
   logHexapodState();
   logWasMovementInLoopActivatedBefore();
   logNewLine();
   respondToSignal();
   irrecv.resume();
  }
  
  moveHexapod(); 
}

void moveHexapod()
{
  if(isTetrapodGateEnabled == true)
  {
    hexapod.moveForward(TETRAPOD);
  }
  else if(isWaveGateEnabled == true)
  {
    hexapod.moveForward(WAVE);
  }
}

void updateHexapodState()
{
  switch(results.value)
  {
    case ON_OFF_SIGNAL:
    {
      togglePreparationState(isHexapodReadyToMove);
    }
    break;
    case TETRAPOD_GAIT_SIGNAL:
    {
      toggleMovementState(isTetrapodGateEnabled);
    }
    break;
    case WAVE_GAIT_SIGNAL:
    {
      toggleMovementState(isWaveGateEnabled);
    }
    break;
    case ROTATE_TO_RIGHT_SIGNAL:
    {
      toggleMovementState(isTurningRightEnabled);
    }
    break;
    case ROTATE_TO_LEFT_SIGNAL:
    {
      toggleMovementState(isTurningLeftEnabled);
    }
    break;
  }
}

void togglePreparationState(bool &preparationState)
{
  isHexapodReadyToMove = !isHexapodReadyToMove;

  if(isHexapodReadyToMove == true)
  {
    hexapod.setDefaultPosition();
  }
  else
  {
    isTetrapodGateEnabled = false;
    isWaveGateEnabled = false;
    isTurningRightEnabled = false;
    isTurningLeftEnabled = false;
    wasTetrapodGateEnabledBefore = false;
    wasWaveGateEnabledBefore = false;

    hexapod.resetLegs();
    hexapod.setHomePosition();
  }
}

void toggleMovementState(bool &movementState)
{
  if(isHexapodReadyToMove == true)
  {
    movementState = !movementState;

    if(movementState == true)
    {
      disableMovementStateAfterTogglingOtherMovementState(isTetrapodGateEnabled, movementState);
      disableMovementStateAfterTogglingOtherMovementState(isWaveGateEnabled, movementState);
      disableMovementStateAfterTogglingOtherMovementState(isTurningRightEnabled, movementState);
      disableMovementStateAfterTogglingOtherMovementState(isTurningLeftEnabled, movementState);
    }
    else
    {
      wasTetrapodGateEnabledBefore = &movementState == &isTetrapodGateEnabled;
      wasWaveGateEnabledBefore = &movementState == &isWaveGateEnabled;
    }
  }
}

void disableMovementStateAfterTogglingOtherMovementState(bool &stateToDisable, bool &toggledMovementState)
{
  if(&stateToDisable != &toggledMovementState)
  {
    if(stateToDisable == true)
    {
      wasTetrapodGateEnabledBefore = &stateToDisable == &isTetrapodGateEnabled;
      wasWaveGateEnabledBefore = &stateToDisable == &isWaveGateEnabled;
      stateToDisable = false;
    }
  }
}
 
void logReceivedCode(long irCode) 
{
  Serial.print("Received signal: ");
  Serial.print(irCode);
  Serial.print(", hex: ");
  Serial.println(irCode, HEX);
}

void logHexapodState()
{
   Serial.print("States: ");
   Serial.print(isHexapodReadyToMove);
   Serial.print("");
   Serial.print(isTetrapodGateEnabled);
   Serial.print("");
   Serial.print(isWaveGateEnabled);
   Serial.print("");
   Serial.print(isTurningRightEnabled);
   Serial.print("");
   Serial.println(isTurningLeftEnabled);
}

void logWasMovementInLoopActivatedBefore()
{
   Serial.print("Was movement in loop activated before: ");
   Serial.print(wasTetrapodGateEnabledBefore);
   Serial.print("");
   Serial.println(wasWaveGateEnabledBefore);
}

void logNewLine()
{
  Serial.println();
}

void respondToSignal()
{
  if(isTetrapodGateEnabled == true)
  {
    if(wasWaveGateEnabledBefore == true)
    {
      hexapod.resetLegs();
      hexapod.lowerLegs();
    }
  }
  else if(isWaveGateEnabled == true)
  {
    if(wasTetrapodGateEnabledBefore == true)
    {
      hexapod.resetLegs();
      hexapod.lowerLegs();
    }
  }
  else if(isTurningRightEnabled == true)
  {
    if(wasTetrapodGateEnabledBefore == true || wasWaveGateEnabledBefore == true)
    {
      hexapod.safeSetDefaultPosition();
    }
    
    hexapod.rotateToRight();
    toggleMovementState(isTurningRightEnabled);
    logHexapodState();
  }
  else if(isTurningLeftEnabled == true)
  {
    if(wasTetrapodGateEnabledBefore == true || wasWaveGateEnabledBefore == true)
    {
      hexapod.safeSetDefaultPosition();
    }
    
    hexapod.rotateToLeft();
    toggleMovementState(isTurningLeftEnabled);
    logHexapodState();
  }
}
