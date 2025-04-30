// step1: right when viewed from motor side
// step2: left when viewed from motor side
const int step1DIR = 2;
const int step1STEP = 3;
const int step2DIR = 4;
const int step2STEP = 5;
const int electromagnet = 10;
const int xLimitSwitch = 11;
const int yLimitSwitch = 12;
const int muxSignalPins[4] = { 14, 15, 16, 17 };  // pins to enable each multiplexer from lowest to highest col
const int muxSelectPins[4] = { 6, 7, 8, 9 };      // from s0 to s3

// stored in steps to minimize error accumulation
double xPos = 0;
double yPos = 0;

bool boardState[8][8];

// total size from limit switches (0, 0) till opposite sides:
// 19000 in x
// 22500 in y
int xMinSteps = -1200;  // -1200
int yMinSteps = 850;    // 850
int xMaxSteps = 19050;  // 19050
int yMaxSteps = 22550;  // 22600

// 8050 for 10 cm in x; 8000 for 10 cm in y
// assume 8000 for 10cm on both axes
// that means 80 per mm
// precision after tesing: sub 0.5mm
// no error accumulation because exact position is stored in steps
const double stepsPerMM = 80;

// speeds given by delay in microseconds between steps
const int calibrationSpeed = 75;
const int goToSpeed = 50;

bool isMoving = false;
bool selectedMux[4];  // from s0 to s3

// board offsets
// xOffset describes x distance in mm from calibrated (0, 0) to the edge of the square (0, 0)
// yOffset describes y distance in mm from calibrated (0, 0) to the edge of the square (0, 0)
// squareSize describes the side length of each square in mm
// carriageSize describes the side length of the carriage in mm
const double xOffset = 4.0;
const double yOffset = 21.2;
const double squareSize = 36;
const double carriageSize = 42;

const double zerothSquareCenterX = xOffset - ((carriageSize - squareSize) / 2);
const double zerothSquareCenterY = yOffset - ((carriageSize - squareSize) / 2);

// amount x movement is multiplied by to produce diagonal motion with length xsqrt(2)
const double diagonalScaleFactor = 2.005;

// test out reed switches with jumper cables
// design and print out chessboard

void readBoardState() {
  for (int i = 0; i < 16; i++) {
    calculateMultiplexerSignal(i);

    digitalWrite(muxSelectPins[0], selectedMux[0]);
    digitalWrite(muxSelectPins[1], selectedMux[1]);
    digitalWrite(muxSelectPins[2], selectedMux[2]);
    digitalWrite(muxSelectPins[3], selectedMux[3]);

    delayMicroseconds(100);

    if (i <= 8) {
      boardState[1][i] = checkSwitchState(muxSignalPins[0]);
      boardState[3][i] = checkSwitchState(muxSignalPins[1]);
      boardState[5][i] = checkSwitchState(muxSignalPins[2]);
      boardState[7][i] = checkSwitchState(muxSignalPins[3]);
    } else {
      boardState[0][i - 8] = checkSwitchState(muxSignalPins[0]);
      boardState[2][i - 8] = checkSwitchState(muxSignalPins[1]);
      boardState[4][i - 8] = checkSwitchState(muxSignalPins[2]);
      boardState[6][i - 8] = checkSwitchState(muxSignalPins[3]);
    }
  }
}

void printBoardState() {
  Serial.print("[");
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      Serial.print(boardState[i][j]);
    }
    Serial.print(", ");
  }
  Serial.println("]");
}

bool checkSwitchState(int pin) {
  if (analogRead(pin) > 1000) {
    Serial.println(analogRead(pin));
    return true;
  }
  return false;
}

void calculateMultiplexerSignal(int channel) {
  for (int i = 0; i < 4; i++) {
    selectedMux[i] = false;
  }

  int value = channel;

  if (channel >= 8) {
    selectedMux[3] = true;

    channel -= 8;
  }

  if (channel >= 4) {
    selectedMux[2] = true;

    channel -= 4;
  }

  if (channel >= 2) {
    selectedMux[1] = true;

    channel -= 2;
  }

  if (channel >= 1) {
    selectedMux[0] = true;

    channel -= 1;
  }
}

void calibrate() {
  digitalWrite(step1DIR, LOW);
  digitalWrite(step2DIR, HIGH);

  while (digitalRead(yLimitSwitch) == 0) {
    digitalWrite(step1STEP, HIGH);
    digitalWrite(step2STEP, HIGH);
    delayMicroseconds(calibrationSpeed);
    digitalWrite(step1STEP, LOW);
    digitalWrite(step2STEP, LOW);
    delayMicroseconds(calibrationSpeed);
  }

  digitalWrite(step1DIR, HIGH);
  digitalWrite(step2DIR, HIGH);

  while (digitalRead(xLimitSwitch) == 0) {
    digitalWrite(step1STEP, HIGH);
    digitalWrite(step2STEP, HIGH);
    delayMicroseconds(calibrationSpeed);
    digitalWrite(step1STEP, LOW);
    digitalWrite(step2STEP, LOW);
    delayMicroseconds(calibrationSpeed);
  }

  goTo(10, ceil(yMinSteps / stepsPerMM));
  while (isMoving) {
    delay(10);
  }
  goTo(floor(xMinSteps / stepsPerMM), ceil(yMinSteps / stepsPerMM));

  xPos = 0;
  yPos = 0;

  xMaxSteps -= xMinSteps;
  yMaxSteps -= yMinSteps;

  xMinSteps = 0;
  yMinSteps = 0;
}

// (0, 0) steps is top right
// (19000, 22500) steps is bottom left

// low low -- left
// high high -- right
// high low -- towards motors
// low high -- away from motors

// x and y are in mm
void goTo(double x, double y) {
  if ((x * stepsPerMM >= xMinSteps) && (x * stepsPerMM <= xMaxSteps) && (y * stepsPerMM >= yMinSteps) && (y * stepsPerMM <= yMaxSteps)) {
    isMoving = true;

    const double xSteps = floor(x * stepsPerMM);
    const double ySteps = floor(y * stepsPerMM);

    const double delX = xSteps - xPos;
    const double delY = ySteps - yPos;

    digitalWrite(step1DIR, delX < 0);
    digitalWrite(step2DIR, delX < 0);

    for (int i = 0; i < abs(delX); i++) {
      digitalWrite(step1STEP, HIGH);
      digitalWrite(step2STEP, HIGH);
      delayMicroseconds(goToSpeed);
      digitalWrite(step1STEP, LOW);
      digitalWrite(step2STEP, LOW);
      delayMicroseconds(goToSpeed);
    }

    digitalWrite(step1DIR, delY > 0);
    digitalWrite(step2DIR, delY < 0);

    for (int j = 0; j < abs(delY); j++) {
      digitalWrite(step1STEP, HIGH);
      digitalWrite(step2STEP, HIGH);
      delayMicroseconds(goToSpeed);
      digitalWrite(step1STEP, LOW);
      digitalWrite(step2STEP, LOW);
      delayMicroseconds(goToSpeed);
    }

    xPos = xSteps;
    yPos = ySteps;

    isMoving = false;
  }
}

void goToSquare(double x, double y) {
  const double newX = x * squareSize + zerothSquareCenterX;
  const double newY = y * squareSize + zerothSquareCenterY;

  goTo(newX, newY);
}

void goToDiagonally(double x, double y) {
  if ((x * stepsPerMM >= xMinSteps) && (x * stepsPerMM <= xMaxSteps) && (y * stepsPerMM >= yMinSteps) && (y * stepsPerMM <= yMaxSteps)) {
    isMoving = true;

    const double xSteps = floor(x * stepsPerMM);
    const double ySteps = floor(y * stepsPerMM);

    const double delX = xSteps - xPos;
    const double delY = ySteps - yPos;

    if (abs(delX) == abs(delY)) {
      if ((delX < 0 && delY < 0) || (delX > 0 && delY > 0)) {
        digitalWrite(step2DIR, delX < 0);

        for (int i = 0; i < floor(abs(delX) * diagonalScaleFactor); i++) {
          digitalWrite(step2STEP, HIGH);
          delayMicroseconds(goToSpeed);
          digitalWrite(step2STEP, LOW);
          delayMicroseconds(goToSpeed);
        }
      } else {
        digitalWrite(step1DIR, delX < 0);

        for (int i = 0; i < floor(abs(delX) * diagonalScaleFactor); i++) {
          digitalWrite(step1STEP, HIGH);
          delayMicroseconds(goToSpeed);
          digitalWrite(step1STEP, LOW);
          delayMicroseconds(goToSpeed);
        }
      }
    }

    xPos = xSteps;
    yPos = ySteps;

    isMoving = false;
  }
}

void goToSquareDiagonally(double x, double y) {
  const double newX = x * squareSize + zerothSquareCenterX;
  const double newY = y * squareSize + zerothSquareCenterY;

  goToDiagonally(newX, newY);
}

// (xi, yi) are the coordinates of the first square, (xf, yf) are the coordinates of the second square
void movePiece(double xi, double yi, double xf, double yf) {
  goToSquare(xi, yi);

  while (isMoving) {
    delay(10);
  }

  digitalWrite(electromagnet, HIGH);

  goToSquare(xf, yf);

  while (isMoving) {
    delay(10);
  }

  digitalWrite(electromagnet, LOW);
}
// (xi, yi) are the coordinates of the first square, (xf, yf) are the coordinates of the second square
void movePieceDiagonally(double xi, double yi, double xf, double yf) {
  goToSquare(xi, yi);

  while (isMoving) {
    delay(10);
  }

  digitalWrite(electromagnet, HIGH);

  goToSquareDiagonally(xf, yf);

  while (isMoving) {
    delay(10);
  }

  digitalWrite(electromagnet, LOW);
}


// (xi, yi) are the coordinates of the first square, (xf, yf) are the coordinates of the second square
void movePieceHorse(double xi, double yi, double xf, double yf) {
  if ((abs(xf - xi) == 1) and (abs(yf - yi) == 2)) {
    goToSquare(xi, yi);
    while (isMoving) {
      delay(10);
    }

    digitalWrite(electromagnet, HIGH);

    goToSquare(xi + (xf - xi) / 2, yi);
    while (isMoving) {
      delay(10);
    }

    goToSquare(xi + (xf - xi) / 2, yf);
    while (isMoving) {
      delay(10);
    }

    goToSquare(xf, yf);
    while (isMoving) {
      delay(10);
    }

    digitalWrite(electromagnet, LOW);
  }
  if ((abs(xf - xi) == 2) and (abs(yf - yi) == 1)) {
    goToSquare(xi, yi);
    while (isMoving) {
      delay(10);
    }

    digitalWrite(electromagnet, HIGH);

    goToSquare(xi, yi + (yf - yi) / 2);
    while (isMoving) {
      delay(10);
    }

    goToSquare(xf, yi + (yf - yi) / 2);
    while (isMoving) {
      delay(10);
    }

    goToSquare(xf, yf);
    while (isMoving) {
      delay(10);
    }

    digitalWrite(electromagnet, LOW);
  }
}

void setup() {
  pinMode(step1DIR, OUTPUT);
  pinMode(step1STEP, OUTPUT);
  pinMode(step2DIR, OUTPUT);
  pinMode(step2STEP, OUTPUT);
  pinMode(electromagnet, OUTPUT);
  pinMode(yLimitSwitch, INPUT);
  pinMode(xLimitSwitch, INPUT);

  for (int i = 0; i < 4; i++) {
    pinMode(muxSignalPins[i], INPUT);
    pinMode(muxSelectPins[i], OUTPUT);
  }

  // calibrate();

  Serial.begin(9600);
}

void loop() {
  // movePieceDiagonall(7, 7, 0, 0);
  // delay(1000);
  // movePiece(0, 0, 7, 7);
  // delay(1000);

  // for (int k = 0; k < 8; k++) {
  //   for (int l = 0; l < 8; l++) {
  //     goToSquare(k, l);
  //     while (isMoving) {
  //       delay(10);
  //     }
  //     delay(1000);
  //   }
  // }

  // digitalWrite(muxSelectPins[0], false);
  // digitalWrite(muxSelectPins[1], false);
  // digitalWrite(muxSelectPins[2], false);
  // digitalWrite(muxSelectPins[3], false);

  // delay(150);

  // Serial.print(digitalRead(muxSignalPins[3]));

  // digitalWrite(muxSelectPins[0], true);
  // digitalWrite(muxSelectPins[1], false);
  // digitalWrite(muxSelectPins[2], false);
  // digitalWrite(muxSelectPins[3], false);

  // delay(150);

  // Serial.println(digitalRead(muxSignalPins[3]));

  // for (int i = 0; i < 16; i++) {
  //   calculateMultiplexerSignal(i);

  //   digitalWrite(muxSelectPins[0], selectedMux[0]);
  //   digitalWrite(muxSelectPins[1], selectedMux[1]);
  //   digitalWrite(muxSelectPins[2], selectedMux[2]);
  //   digitalWrite(muxSelectPins[3], selectedMux[3]);

  //   delay(500);

  //   Serial.println(analogRead(muxSignalPins[3]));
  // }

  // Serial.println("");

  readBoardState();
  printBoardState();

  delay(500);

  // delay(1000000000);
}