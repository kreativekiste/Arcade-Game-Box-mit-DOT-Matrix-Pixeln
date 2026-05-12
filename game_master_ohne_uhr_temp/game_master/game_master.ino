// www.kreativekiste.de
// 10.05.2026
// Version 1.7.3

#include <SPI.h>
#include <EEPROM.h>
#include <MD_MAX72xx.h>

const int CS_PIN = 10;           
const int MAX_DEVICES = 12;      
const int MAX_X = 24;            
const int MAX_Y = 32;            

const int PIN_VRX     = A0;      
const int PIN_VRY     = A1;      
const int PIN_T3      = A2;      
const int PIN_LDR     = A3;      
const int PIN_T1      = 2;       
const int PIN_T2      = 3;       

// --- EINSTELLUNGEN SYSTEM ---
const unsigned long IDLE_TIMEOUT = 13000; 
const unsigned long RESET_TIME   = 5000;   
const unsigned long GO_DURATION  = 3000;   
const int NUM_GAMES = 7;         

MD_MAX72XX mx = MD_MAX72XX(MD_MAX72XX::FC16_HW, CS_PIN, MAX_DEVICES);

unsigned long lastActivityTime = 0;
unsigned long lastLdrFetch = 0; 
unsigned long lastMenuMove = 0; 
int highscores[NUM_GAMES];

enum SystemState { 
  STATE_BOOT, 
  STATE_IDLE, 
  STATE_MENU, 
  STATE_GAMEOVER,
  STATE_PLAY_JUMP, 
  STATE_PLAY_DOT, 
  STATE_PLAY_INVADER, 
  STATE_PLAY_BLOCK, 
  STATE_PLAY_RUN, 
  STATE_PLAY_ROCK,
  STATE_PLAY_WILD
};

SystemState currentState = STATE_BOOT; 
int selectedGame = 0; 
int lastScore = 0;
unsigned long gameOverStartTime = 0;
unsigned long menuPressStart = 0; 

void drawPixel(int x, int y, bool state) {
  if (x < 0 || x >= MAX_X || y < 0 || y >= MAX_Y) return; 
  int hardwareRow = x % 8; 
  int hardwareCol = y;     
  if (x >= 8 && x < 16) hardwareCol += 32; 
  else if (x >= 16) hardwareCol += 64; 
  mx.setPoint(hardwareRow, hardwareCol, state);
}

// --- 3x5 Pixel Ziffern-Font (0-9) ---
const uint8_t digit_font[10][5] = {
  {0b111, 0b101, 0b101, 0b101, 0b111}, // 0
  {0b010, 0b110, 0b010, 0b010, 0b111}, // 1
  {0b111, 0b001, 0b111, 0b100, 0b111}, // 2
  {0b111, 0b001, 0b111, 0b001, 0b111}, // 3
  {0b101, 0b101, 0b111, 0b001, 0b001}, // 4
  {0b111, 0b100, 0b111, 0b001, 0b111}, // 5
  {0b111, 0b100, 0b111, 0b101, 0b111}, // 6
  {0b111, 0b001, 0b001, 0b001, 0b001}, // 7
  {0b111, 0b101, 0b111, 0b101, 0b111}, // 8
  {0b111, 0b101, 0b111, 0b001, 0b111}  // 9
};

void drawChar(int startX, int startY, char c) {
  int idx = c - '0';
  if (idx < 0 || idx > 9) return;
  for (int y = 0; y < 5; y++) {
    for (int x = 0; x < 3; x++) {
      if (bitRead(digit_font[idx][y], 2 - x))
        drawPixel(startX + x, startY + y, true);
    }
  }
}

void drawString(int startX, int startY, const char* str) {
  int x = startX;
  while (*str) {
    drawChar(x, startY, *str);
    x += 4; // 3 Pixel Zeichen + 1 Pixel Abstand
    str++;
  }
}

// --- GAME OVER Font ---
const uint8_t go_font[10][5] = {
  {0b111, 0b101, 0b111, 0b101, 0b101}, // A 
  {0b111, 0b100, 0b100, 0b100, 0b111}, // C 
  {0b111, 0b100, 0b111, 0b100, 0b111}, // E 
  {0b111, 0b100, 0b101, 0b101, 0b111}, // G 
  {0b101, 0b101, 0b111, 0b101, 0b101}, // H 
  {0b101, 0b111, 0b101, 0b101, 0b101}, // M 
  {0b111, 0b101, 0b101, 0b101, 0b111}, // O 
  {0b111, 0b101, 0b110, 0b101, 0b101}, // R 
  {0b111, 0b100, 0b111, 0b001, 0b111}, // S 
  {0b101, 0b101, 0b101, 0b101, 0b010}  // V 
};

void drawGOChar(int startX, int startY, int charIdx) {
  if (charIdx < 0 || charIdx > 9) return;
  for (int y = 0; y < 5; y++) {
    for (int x = 0; x < 3; x++) {
      if (bitRead(go_font[charIdx][y], 2 - x)) drawPixel(startX + x, startY + y, true);
    }
  }
}

#include "rakete.h"      
#include "pixel_jump.h" 
#include "pixel_dot.h" 
#include "pixel_invader.h" 
#include "pixel_block.h" 
#include "pixel_run.h" 
#include "pixel_rock.h" 
#include "pixel_wild.h"

void setup() {
  delay(500); 

  EEPROM.get(0, highscores);
  if (highscores[0] == -1) { 
    for (int i = 0; i < NUM_GAMES; i++) highscores[i] = 0;
    EEPROM.put(0, highscores);
  }

  pinMode(PIN_T3, INPUT_PULLUP);
  pinMode(PIN_T1, INPUT_PULLUP);
  pinMode(PIN_T2, INPUT_PULLUP);

  mx.begin();
  int ldrValue = analogRead(PIN_LDR);
  int startIntensity = map(ldrValue, 10, 900, 4, 0);
  mx.control(MD_MAX72XX::INTENSITY, constrain(startIntensity, 0, 4));
  mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF); 
  
  lastActivityTime = millis();
}

bool lastStart = false;
bool lastAction = false;
bool lastBack = false;

void loop() {
  unsigned long now = millis();

  if (currentState == STATE_IDLE || currentState == STATE_MENU || currentState == STATE_BOOT || currentState == STATE_GAMEOVER) {
    if (now - lastLdrFetch > 1000 || lastLdrFetch == 0) {
      int ldrValue = analogRead(PIN_LDR);
      int newIntensity = map(ldrValue, 10, 900, 5, 0);
      mx.control(MD_MAX72XX::INTENSITY, constrain(newIntensity, 0, 5));
      lastLdrFetch = now;
    }
  }

  bool currentStart  = (digitalRead(PIN_T3) == LOW); 
  bool currentAction = (digitalRead(PIN_T1) == LOW); 
  bool currentBack   = (digitalRead(PIN_T2) == LOW); 
  int joyX = analogRead(PIN_VRX);

  if (currentStart || currentAction || currentBack || abs(joyX - 512) > 300) {
    lastActivityTime = now;
  }

  // GLOBALER SPIELABBRUCH mit T2
  if (currentBack && currentState >= STATE_PLAY_JUMP && currentState <= STATE_PLAY_WILD) {
    currentState = STATE_MENU;
    delay(200); 
  }
  else if (currentBack && currentState == STATE_GAMEOVER) {
    currentState = STATE_MENU;
    delay(200); 
  }

  mx.clear(); 
  
  if (currentState == STATE_BOOT) {
    if (updateRocketAnimation()) {
      currentState = STATE_MENU;
      selectedGame = 0; 
    }
  }
  else if (currentState == STATE_IDLE) {
    if (currentStart || currentAction || currentBack || abs(joyX - 512) > 300) { 
      currentState = STATE_MENU; 
      delay(200); 
    }
    // Idle: Display bleibt leer bis Eingabe
  } 
  else if (currentState == STATE_MENU) {
    if (now - lastActivityTime > IDLE_TIMEOUT) {
      currentState = STATE_IDLE;
    }

    if (joyX > 800 && (now - lastMenuMove > 250)) { 
      selectedGame--; 
      if (selectedGame < 0) selectedGame = NUM_GAMES - 1; 
      lastMenuMove = now; 
    } 
    if (joyX < 200 && (now - lastMenuMove > 250)) { 
      selectedGame++; 
      if (selectedGame >= NUM_GAMES) selectedGame = 0; 
      lastMenuMove = now; 
    }

    // LÖSCHEN DES HIGHSCORES MIT T2
    if (currentBack) {
      if (menuPressStart == 0) {
        menuPressStart = now;
      } else if (now - menuPressStart > RESET_TIME) {
        highscores[selectedGame] = 0;
        EEPROM.put(0, highscores);
        menuPressStart = 0;
        
        for(int x=0; x<MAX_X; x++) for(int y=0; y<MAX_Y; y++) drawPixel(x, y, true);
        mx.update();
        delay(300);
      }
    } else {
      menuPressStart = 0; 
    }

    // SPIEL STARTEN NUR MIT T3
    if (currentStart && !lastStart) {
      delay(200); 
      if (selectedGame == 0)      { pj_initGame(); currentState = STATE_PLAY_JUMP; }
      else if (selectedGame == 1) { fb_initGame(); currentState = STATE_PLAY_DOT; }
      else if (selectedGame == 2) { si_initGame(); currentState = STATE_PLAY_INVADER; }
      else if (selectedGame == 3) { pb_initGame(); currentState = STATE_PLAY_BLOCK; }
      else if (selectedGame == 4) { pr_initGame(); currentState = STATE_PLAY_RUN; }
      else if (selectedGame == 5) { rk_initGame(); currentState = STATE_PLAY_ROCK; }
      else if (selectedGame == 6) { pw_initGame(); currentState = STATE_PLAY_WILD; }
    }
    
    // --- ICONS FÜR DAS MENÜ ---
    if (selectedGame == 0) { 
      for(int x=9; x<14; x++) drawPixel(x, 20, true);
      int bounce = abs((int)((now / 80) % 12) - 6); 
      drawPixel(11, 19 - bounce, true);
    } 
    else if (selectedGame == 1) { 
      for(int y=0; y<12; y++) { drawPixel(16, y, true); drawPixel(17, y, true); }
      for(int y=20; y<32; y++) { drawPixel(16, y, true); drawPixel(17, y, true); }
      int flapY = 15 + ((now / 300) % 2); 
      drawPixel(7, flapY, true); drawPixel(8, flapY, true);
      drawPixel(7, flapY + 1, true); drawPixel(8, flapY + 1, true);
    }
    else if (selectedGame == 2) { 
      drawPixel(12, 28, true);
      drawPixel(11, 29, true); drawPixel(12, 29, true); drawPixel(13, 29, true);
      int ex = 10 + ((now / 500) % 3);
      drawPixel(ex, 10, true); drawPixel(ex+1, 10, true); drawPixel(ex+2, 10, true);
      drawPixel(ex+1, 11, true);
    }
    else if (selectedGame == 3) { 
      for (int y=8; y<30; y++) { drawPixel(6, y, true); drawPixel(17, y, true); }
      int fallY = 10 + ((now / 200) % 18);
      drawPixel(11, fallY, true); drawPixel(11, fallY+1, true); drawPixel(11, fallY+2, true);
      drawPixel(12, fallY+2, true);
    }
    else if (selectedGame == 4) { 
      for(int x=4; x<20; x++) drawPixel(x, 22, true); 
      int jump = (now / 400) % 2 == 0 ? 0 : 3;
      drawPixel(8, 21-jump, true); drawPixel(9, 21-jump, true);     
      drawPixel(7, 20-jump, true); drawPixel(8, 20-jump, true); drawPixel(9, 20-jump, true); 
      drawPixel(7, 19-jump, true); drawPixel(8, 19-jump, true);     
      drawPixel(16, 21, true); drawPixel(16, 20, true);
    }
    else if (selectedGame == 5) { 
      for(int x=9; x<14; x++) drawPixel(x, 26, true); 
      drawPixel(9, 10, true); drawPixel(10, 10, true); 
      drawPixel(13, 10, true); drawPixel(14, 10, true); 
      int by = 25 - ((now / 100) % 14); 
      drawPixel(11, by, true);
    }
    else if (selectedGame == 6) { 
      for(int x=6; x<18; x+=2) { drawPixel(x, 10, true); drawPixel(x, 20, true); }
      int frogY = 28 - ((now / 200) % 20);
      drawPixel(11, frogY, true);
    }
  }
  
  else if (currentState == STATE_PLAY_JUMP) {
    pj_updateGame(currentStart, lastStart);
    if (pj_gameOver) { lastScore = pj_score; currentState = STATE_GAMEOVER; gameOverStartTime = now; }
    else pj_drawGame();
  }
  else if (currentState == STATE_PLAY_DOT) {
    fb_updateGame(currentAction, lastAction, currentStart, lastStart);
    if (fb_gameOver) { lastScore = fb_score; currentState = STATE_GAMEOVER; gameOverStartTime = now; }
    else fb_drawGame();
  }
  else if (currentState == STATE_PLAY_INVADER) {
    si_updateGame(currentAction, lastAction, currentStart, lastStart);
    if (si_gameOver) { lastScore = SIS.score; currentState = STATE_GAMEOVER; gameOverStartTime = now; }
    else si_drawGame();
  }
  else if (currentState == STATE_PLAY_BLOCK) {
    pb_updateGame(currentAction, lastAction, currentStart, lastStart);
    if (pb_gameOver) { lastScore = PBS.score; currentState = STATE_GAMEOVER; gameOverStartTime = now; }
    else pb_drawGame();
  }
  else if (currentState == STATE_PLAY_RUN) {
    pr_updateGame(currentAction, lastAction, currentStart, lastStart);
    if (pr_gameOver) { lastScore = PRS.score; currentState = STATE_GAMEOVER; gameOverStartTime = now; }
    else pr_drawGame();
  }
  else if (currentState == STATE_PLAY_ROCK) {
    rk_updateGame(currentAction, lastAction, currentStart, lastStart);
    if (rk_gameOver) { lastScore = RKS.score; currentState = STATE_GAMEOVER; gameOverStartTime = now; }
    else rk_drawGame();
  }
  else if (currentState == STATE_PLAY_WILD) {
    pw_updateGame(currentAction, lastAction, currentStart, lastStart);
    if (pw_gameOver) { lastScore = PWS.level; currentState = STATE_GAMEOVER; gameOverStartTime = now; }
    else pw_drawGame();
  }

  else if (currentState == STATE_GAMEOVER) {
    if (now - gameOverStartTime < 50) {
      if (lastScore > highscores[selectedGame]) {
        highscores[selectedGame] = lastScore;
        EEPROM.put(0, highscores);
      }
    }

    drawGOChar(4, 2, 3);  drawGOChar(8, 2, 0);  drawGOChar(12, 2, 5);  drawGOChar(16, 2, 2);
    drawGOChar(4, 8, 6);  drawGOChar(8, 8, 9);  drawGOChar(12, 8, 2);  drawGOChar(16, 8, 7);
    drawGOChar(1, 16, 8); drawGOChar(5, 16, 1); drawPixel(9, 17, true); drawPixel(9, 19, true);
    drawGOChar(1, 24, 4); drawGOChar(5, 24, 8); drawPixel(9, 25, true); drawPixel(9, 27, true);

    char scStr[6]; sprintf(scStr, "%d", lastScore);
    char hsStr[6]; sprintf(hsStr, "%d", highscores[selectedGame]);
    drawString(12, 16, scStr);
    drawString(12, 24, hsStr);

    if (now - gameOverStartTime > GO_DURATION) {
      currentState = STATE_MENU;
      delay(200);
    }
  }

  lastStart = currentStart;
  lastAction = currentAction;
  lastBack = currentBack;
  mx.update();
}