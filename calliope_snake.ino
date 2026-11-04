#include <Calliope_Arduino.h>

// ## SNAKE for Calliope mini v3 (Arduino) ##
// Press any button to start the game :)
// # Actions: 
// - Button A = Turn Left
// - Button B = Turn Right

Calliope_Matrix calliope;

// Global constants
const int SIZE = 5;                         // LED Matrix size
const int MAX_LEN = SIZE * SIZE;            // Snake body can't be longer than the number of LEDs (25)
const unsigned long SNAKE_SPEED_MS = 600;   // Snake speed: time per step (clock tick)
const unsigned long TARGET_BLINK_MS = 150;  // Target blinking

// Opening animation
int spiralX[MAX_LEN], spiralY[MAX_LEN];
const unsigned long SPIRAL_STEP_MS = 60;

// Direction
const char* DIR_NAME[4] = { "North", "East", "South", "West" }; // 0: North, 1: East, 2: South, 3: West
const int DX[4] = { 0, 1, 0, -1 };    // Horizontal arithmetic
const int DY[4] = { -1, 0, 1, 0 };    // Vertical arithmetic

// Snake and Target
int tx, ty;                    // Target position
int sx[MAX_LEN], sy[MAX_LEN];  // Snake body => Head is sx[0], sy[0]
int len;                       // Snake length
int direction;                 // Snake direction
int turn;                      // -1: Left, +1: Right, 0: Straight
bool lastA = false, lastB = false;


// ## Shared functions ##
bool isPressed(int pin) {
  return digitalRead(pin) == LOW;
}

bool collidesWithSnake(int x, int y, int count) {
  for (int k = 0; k < count; k++) {
    if (sx[k] == x && sy[k] == y) return true;
  }
  return false;
}

// ## Opening screen: spiral animation + game start ##
bool waitOrButtonPressed(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    if (isPressed(PIN_BUTTON_A) || isPressed(PIN_BUTTON_B)) return true;
    delay(5);
  }
  return false;
}

void buildSpiralBody() {
  int top = 0, bottom = SIZE - 1, left = 0, right = SIZE - 1, n = 0;
  // Goes from top-left corner to center
  while (top <= bottom && left <= right) {
    // Right direction
    for (int x = left; x <= right; x++) {
      spiralX[n] = x;
      spiralY[n++] = top;
    }
    top++;
    // Down direction
    for (int y = top; y <= bottom; y++) {
      spiralX[n] = right;
      spiralY[n++] = y;
    }
    right--;
    // Left direction
    if (top <= bottom) {
      for (int x = right; x >= left; x--) {
        spiralX[n] = x;
        spiralY[n++] = bottom;
      }
      bottom--;
    }
    // Up direction
    if (left <= right) {
      for (int y = bottom; y >= top; y--) {
        spiralX[n] = left;
        spiralY[n++] = y;
      }
      left++;
    }
  }
}

bool spiralAnimation() {
  calliope.clear();
  // Turn spiral body LEDs ON
  for (int k = 0; k < MAX_LEN; k++) {
    calliope.drawPixel(spiralX[k], spiralY[k], LED_ON);
    if (waitOrButtonPressed(SPIRAL_STEP_MS)) return true;
  }
  // Flashes the matrix
  for (int k = 0; k < 2; k++) {
    calliope.clear();
    if (waitOrButtonPressed(100)) return true;
    calliope.fillRect(0, 0, SIZE, SIZE, LED_ON);
    if (waitOrButtonPressed(100)) return true;
  }
  // Turn spiral body LEDs OFF
  for (int k = 0; k < MAX_LEN; k++) {
    calliope.drawPixel(spiralX[k], spiralY[k], LED_OFF);
    if (waitOrButtonPressed(SPIRAL_STEP_MS)) return true;
  }
  // Flashes the center
  for (int k = 0; k < 3; k++) {
    calliope.drawPixel(2, 2, LED_ON);
    if (waitOrButtonPressed(120)) return true;
    calliope.drawPixel(2, 2, LED_OFF);
    if (waitOrButtonPressed(120)) return true;
  }
  return false;
}

void gameStartAnimation() {
  for (int i = 0; i <= 2; i++) {
    for (int r = 0; r <= 2; r++) {
      calliope.clear();
      calliope.drawRect(2 - r, 2 - r, 2 * r + 1, 2 * r + 1, LED_ON);
      delay(70);
    }
  }
  calliope.print("GO!");
}

void openingScreen() {
  buildSpiralBody();
  while (true) {
    //calliope.print("SNAKE");  // TODO: Non-blocking title display
    if (spiralAnimation()) break;
  }
  randomSeed(micros());
  gameStartAnimation();
}

// ## Game setup ##
void setTarget() {
  do {
    tx = random(0, SIZE);
    ty = random(0, SIZE);
  } while (collidesWithSnake(tx, ty, len));
  Serial.print("Target: [");
  Serial.print(tx);
  Serial.print(",");
  Serial.print(ty);
  Serial.println("]");
}

void newGame() {
  len = 1;
  turn = 0;
  sx[0] = random(0, SIZE);
  sy[0] = random(0, SIZE);
  direction = random(0, 4);
  setTarget();
  Serial.println("--- New game ---");
}

// ## Game loop ##
void setTurnFromButtons() {
  bool a = isPressed(PIN_BUTTON_A);
  bool b = isPressed(PIN_BUTTON_B);
  if (a && !lastA) turn = -1;
  if (b && !lastB) turn = +1;
  lastA = a;
  lastB = b;
}

void drawGame(bool showTarget) {
  calliope.clear();
  for (int k = 0; k < len; k++) {
    calliope.drawPixel(sx[k], sy[k], LED_ON);
  }
  if (showTarget) {
    calliope.drawPixel(tx, ty, LED_ON);
  }
}

void waitTick() {
  unsigned long start = millis();
  int targetLastBlink = -1;
  while (millis() - start < SNAKE_SPEED_MS) {
    setTurnFromButtons();
    int targetBlink = ((millis() - start) / TARGET_BLINK_MS) % 2;
    if (targetBlink != targetLastBlink) {
      drawGame(targetBlink == 0);
      targetLastBlink = targetBlink;
    }
    delay(5);
  }
}

void gameOver() {
  Serial.print("GAME OVER - Score: ");
  Serial.println(len - 1);
  // Flashes snake body
  for (int k = 0; k < 3; k++) {
    calliope.clear();
    delay(200);
    drawGame(false);
    delay(200);
  }
  calliope.print(":( GAME OVER - SCORE:");
  calliope.print(len - 1);
  delay(1000);
  openingScreen();
  newGame();
}

void step() {
  // Direction and position are always positive with modular arithmetic
  direction = (direction + turn + 4) % 4;
  turn = 0;

  // Head position
  int nx = (sx[0] + DX[direction] + SIZE) % SIZE;
  int ny = (sy[0] + DY[direction] + SIZE) % SIZE;
  bool eating = (nx == tx && ny == ty);

  // If eating, tail doesn't move
  if (collidesWithSnake(nx, ny, eating ? len : len - 1)) {
    gameOver();
    return;
  }
  if (eating) len++;

  // Set new head by shifting array - TODO: Ring buffer?
  for (int k = len - 1; k > 0; k--) {
    sx[k] = sx[k - 1];
    sy[k] = sy[k - 1];
  }
  sx[0] = nx;
  sy[0] = ny;

  if (eating) {
    if (len == MAX_LEN) {
      drawGame(false);
      delay(500);
      Serial.print("WIN - Score: ");
      Serial.println(len - 1);
      calliope.print(":) WIN");
      delay(1000);
      openingScreen();
      newGame();
      return;
    }
    setTarget();
  }
}

// ## Arduino ##
void setup() {
  Serial.begin(9600);
  Serial.println("Snake Game v0.1");
  calliope.begin();
  pinMode(PIN_BUTTON_A, INPUT);
  pinMode(PIN_BUTTON_B, INPUT);
  openingScreen();
  newGame();
}

void loop() {
  waitTick();
  step();
}
