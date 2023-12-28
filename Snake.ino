#include <Adafruit_GFX.h>    // Core graphics library
#include <MCUFRIEND_kbv.h>   // Hardware-specific library
#include <TouchScreen.h>
#include <EEPROM.h>
#include <SD.h>
#include <SPI.h>
#include <Fonts/FreeMonoBoldOblique12pt7b.h>

#define YP A3  // analog pin from calibration
#define XM A2  // analog pin from calibration
#define YM 9   // digital pin from calibration
#define XP 8   // digital pin from calibration

// Creating Objects
MCUFRIEND_kbv tft;
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
TSPoint tp;
const int buttonLeft = 31;
const int buttonRight = 35;

// Touchscreen Caliberation
const int TS_LEFT=956,TS_RT=92,TS_TOP=898,TS_BOT=126;

int x, y; // Variables for the coordinates where the display has been pressed

// Define game variables
int score;
int highScore;
int buttonStateLeft;
int buttonStateRight;
unsigned long lastButtonChangeTime = 0;
const unsigned long debounceInterval = 240;

class Snake {
public:
  int length;               // Length of the snake
  int xPositions[100];      // Array to store x-coordinates of snake segments
  int yPositions[100];      // Array to store y-coordinates of snake segments
  int direction;            // Current direction of the snake (0 right,1 up,2 left,3 down)

public:
  Snake(){
    length = 3;
    direction = 0;
    for (int i = 0; i < length; i++) {
      xPositions[i] = 100 - i * 20;
      yPositions[i] = 100;
    }
  };
  void move(){
    // Update the position of each snake segment
    for (int i = length - 1; i > 0; i--) {
      xPositions[i] = xPositions[i - 1];
      yPositions[i] = yPositions[i - 1];
    }

    // Update the position of the snake's head
    if (direction == 0) {
      xPositions[0] += 20;
    } else if (direction == 1) {
      yPositions[0] -= 20;
    } else if (direction == 2) {
      xPositions[0] -= 20;
    } else if (direction == 3) {
      yPositions[0] += 20;
    }
  };
  bool checkCollision(){
    if (xPositions[0] < 0 || xPositions[0] >= tft.width() ||
        yPositions[0] < 0 || yPositions[0] >= tft.height()){
      return true;
    } else {
      for (int i = 1; i < length; i++) {
        if (xPositions[0] == xPositions[i] && yPositions[0] == yPositions[i]) {
          return true;
        }
      }   
    }
    return false;
  };
  void drawSnake(){
    for (int i = 0; i < length; i++) {
      drawSquare(xPositions[i],yPositions[i]);
    }
  }
  void drawSquare(int xPos, int yPos) {
    int length = 20;
    tft.fillRect(xPos, yPos, length, length, tft.color565(255,100,100));
  }
  void removeSnake(){
    for (int i = 0; i < length; i++) {
      removeSquare(xPositions[i],yPositions[i]);
    }
  }
  void removeSquare(int xPos, int yPos) {
    int length = 20;
    tft.fillRect(xPos, yPos, length, length, tft.color565(86, 125, 70));
  }
};

class Food {
public:
  int xPos;  // x-coordinate of the food
  int yPos;  // y-coordinate of the food

public:
  Food() {
  };
  void drawFood(){
    tft.fillTriangle(xPos, yPos, (xPos + 20), (yPos+10), (xPos), (yPos+20), tft.color565(255,255,0));
  }
  void removeFood(){
    tft.fillTriangle(xPos, yPos, (xPos + 20), (yPos+10), (xPos), (yPos+20), tft.color565(86, 125, 70));
  }
};

// Snake and Food pointers
Snake* snake = nullptr;
Food* food = nullptr;

void setup() {
  // Initiate display
  uint16_t ID = tft.readID();
  tft.begin(ID);
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setFont(&FreeMonoBoldOblique12pt7b);
  
  // Initialize EEPROM and load the high score from EEPROM
  EEPROM.begin();
  highScore = EEPROM.read(0);

  // Set up touchscreen
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);

  // Set up buttons
  pinMode(buttonLeft, INPUT);
  pinMode(buttonRight, INPUT);

  initiateGame(); // Initiate the game
  
}

void loop() {
  // put your main code here, to run repeatedly:
  snake->removeSnake();
  snake->move();

  if (snake->xPositions[0] == food->xPos && snake->yPositions[0] == food->yPos) {
    snake->length++;
    score++;
    food->removeFood();
    generateNewPosition();
    food->drawFood();
  }

  // check if player lost
  if (snake->checkCollision()){
    gameOver();
  }
  
  // Draw snake and score banner (maybe not score because small screen)
  snake->drawSnake();

  buttonStateLeft = digitalRead(buttonLeft);
  buttonStateRight = digitalRead(buttonRight);
  pinMode(XM, INPUT);
  pinMode(YP, INPUT);
  pinMode(buttonLeft, INPUT);
  pinMode(buttonRight, INPUT);
  TSPoint p = ts.getPoint();
  // snake move
  if (p.z > 20) {
    p.y = map(p.y, TS_LEFT, TS_RT, 0, 480);
    p.x = map(p.x, TS_TOP, TS_BOT, 0, 320);

    if (p.y >= 320 && snake->direction != 2){
      snake->direction = 0; //MOVE RIGHT
    } else if (p.y >= 160 && p.y < 320 && p.x < 160 && snake->direction != 3){
      snake->direction = 1; //MOVE UP
    } else if (p.y >= 160 && p.y < 320 && p.x >= 160 && snake->direction != 1){
      snake->direction = 3; //MOVE DOWN
    } else if (p.y < 160 && snake->direction != 0){
      snake->direction = 2; //MOVE LEFT
    }
  }
  // When this type of button is pressed, state is low...
  if(buttonStateLeft == LOW && millis() - lastButtonChangeTime > debounceInterval){
    lastButtonChangeTime = millis();
    snake->direction = (snake->direction + 1) % 4;
  }
  if(buttonStateRight == LOW && millis() - lastButtonChangeTime > debounceInterval){
    lastButtonChangeTime = millis();
    snake->direction = (snake->direction + 3) % 4;
  }

  // Delay to control game speed
  delay(250);
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
}

void initiateGame() {
  snake = new Snake();
  food = new Food();
  generateNewPosition();
  score = 0;

  tft.fillRect(0, 0, 160, 320, tft.color565(0, 255, 0));
  tft.setCursor(0, 160);
  tft.println("Left");
  tft.fillRect(160, 0, 160, 160, tft.color565(255, 255, 0));
  tft.setCursor(180, 80);
  tft.println("Up");
  tft.fillRect(160, 160, 160, 160, tft.color565(255, 0, 0));
  tft.setCursor(180, 240);
  tft.println("Down");
  tft.fillRect(320, 0, 160, 320, tft.color565(0, 0, 255));
  tft.setCursor(340, 160);
  tft.println("Right");
  delay(2000);
  tft.fillScreen(tft.color565(86, 125, 70));
  snake->drawSnake();
  food->drawFood();
}

void generateNewPosition(){
  // Get new random coordinates but if snake segment on new coors, get new one
  food->xPos = random(0, tft.width() / 20) * 20;
  food->yPos = random(0, tft.height() / 20) * 20;

  for (int i = 0; i < snake->length; i++) {
    if (food->xPos == snake->xPositions[i] && food->yPos == snake->yPositions[i]) {
      generateNewPosition();
      return;
    }
  }
};

void gameOver() {
  snake->drawSnake();
  delay(3000);

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(0, 40);
  tft.println("GAME OVER");
  tft.setCursor(100, 70);
  tft.println("Score:");
  tft.setCursor(100, 100);
  tft.println(score);
  tft.setCursor(100, 130);
  tft.println("Highscore:");
  tft.setCursor(100, 160);
  tft.println(highScore);
  tft.setCursor(0, 200);
  tft.println("Touch the screen or push a button to Restart");

  if (score > highScore) {
    highScore = score;
    EEPROM.write(0,highScore);
  }
  
  pinMode(XM, INPUT);
  pinMode(YP, INPUT);
  while (isPressedOrClicked() == false) {}
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 120);
  tft.println("Restarting...");
  tft.setCursor(tft.width()/2, 150);
  tft.println("3");
  delay(200);
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 120);
  tft.println("Restarting...");
  tft.setCursor(tft.width()/2, 150);
  tft.println("2");
  delay(200);
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 120);
  tft.println("Restarting...");
  tft.setCursor(tft.width()/2, 150);
  tft.println("1");
  delay(200);

  delete snake;
  delete food;
  snake = nullptr;
  food = nullptr;
  initiateGame();
}

bool isPressedOrClicked(void) {
    int count = 0;
    bool state, oldstate;
    int buttonRestartLeft;
    int buttonRestartRight;
    while (count < 10) {
        tp = ts.getPoint();
        pinMode(YP, OUTPUT);
        pinMode(XM, OUTPUT);
        state = tp.z > 200;
        buttonRestartLeft = digitalRead(buttonLeft);
        buttonRestartRight = digitalRead(buttonRight);
        if (buttonRestartRight == LOW || buttonRestartLeft == LOW){
          state = true;
        }
        if (state == oldstate){count++;}
        else{count = 0;}
        oldstate = state;
        delay(5);
    }
    return oldstate;
}
