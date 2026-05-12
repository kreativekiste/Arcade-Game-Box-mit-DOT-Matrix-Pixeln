#ifndef PIXEL_ROCK_H
#define PIXEL_ROCK_H

// --- EINSTELLUNGEN ---
const float RK_START_SPEED  = 0.35f;
const float RK_PADDLE_SPEED = 0.80f;
const float RK_TURBO_MULT   = 1.80f;
const int   RK_PAD_W        = 5;
const int   RK_OFFSET_Y     = 4;
const int   RK_ROWS         = 12;
const int   RK_COLS         = 12;

struct RK_State {
  float ballX, ballY;
  float dirX, dirY;
  float paddleX;
  float speed;
  bool  turbo;
  bool  waitFire;
  int   blocks[RK_COLS][RK_ROWS];
  int   score;
};

static RK_State RKS;
static bool rk_gameOver = false;
static unsigned long rk_gameOverTime = 0;
static unsigned long rk_lastFrame = 0;

static void rk_hit(int x, int y) {
  if (x < 0 || x >= RK_COLS || y < 0 || y >= RK_ROWS) return;
  int type = RKS.blocks[x][y];
  if (type == 0) return;

  if (type >= 10) {
    RKS.blocks[x][y]--;
    if (RKS.blocks[x][y] < 10) RKS.blocks[x][y] = 0;
    RKS.score++;
    return;
  }

  RKS.blocks[x][y] = 0;
  RKS.score++;

  if (type == 2) { // Explosion
    for (int dx = -1; dx <= 1; dx++) {
      for (int dy = -1; dy <= 1; dy++) {
        int nx = x + dx, ny = y + dy;
        if (nx >= 0 && nx < RK_COLS && ny >= 0 && ny < RK_ROWS) {
          if (RKS.blocks[nx][ny] > 0 && RKS.blocks[nx][ny] < 10) {
            RKS.blocks[nx][ny] = 0;
            RKS.score++;
          }
        }
      }
    }
  } else if (type == 3) { // Turbo
    RKS.turbo = true;
  }
}

void rk_initGame() {
  RKS.paddleX = (MAX_X / 2.0f) - (RK_PAD_W / 2.0f);
  RKS.ballX   = RKS.paddleX + (RK_PAD_W / 2.0f);
  RKS.ballY   = MAX_Y - 3.0f;
  RKS.dirX    = 0.5f;
  RKS.dirY    = -0.866f;
  RKS.speed   = RK_START_SPEED;
  RKS.turbo   = false;
  RKS.waitFire = true;
  RKS.score   = 0;

  rk_gameOver     = false;
  rk_gameOverTime = 0;

  for (int x = 0; x < RK_COLS; x++) {
    for (int y = 0; y < RK_ROWS; y++) {
      int r = random(100);
      if      (r < 10) RKS.blocks[x][y] = 12; // Harter Block
      else if (r < 20) RKS.blocks[x][y] = 2;  // Explosiv
      else if (r < 30) RKS.blocks[x][y] = 3;  // Turbo
      else             RKS.blocks[x][y] = 1;  // Normal
    }
  }
}

void rk_updateGame(bool currentAction, bool lastAction, bool currentStart, bool lastStart) {
  unsigned long now = millis();

  if (rk_gameOver) {
    if (now - rk_gameOverTime >= 1000) {
      if (currentAction && !lastAction) rk_initGame();
    }
    return;
  }

  // 1. INPUT sofort abfangen
  if (currentAction && !lastAction) RKS.waitFire = false;

  // 2. Frame-Timer
  if (now - rk_lastFrame < 20) return;
  rk_lastFrame = now;

  // 3. Paddel bewegen
  int joyX = analogRead(PIN_VRX);
  if (joyX > 800) RKS.paddleX -= RK_PADDLE_SPEED;
  if (joyX < 200) RKS.paddleX += RK_PADDLE_SPEED;
  RKS.paddleX = constrain(RKS.paddleX, 0, MAX_X - RK_PAD_W);

  // 4. Ball klebt am Paddel
  if (RKS.waitFire) {
    RKS.ballX = RKS.paddleX + (RK_PAD_W / 2.0f);
    RKS.ballY = MAX_Y - 3.0f;
    return;
  }

  // 5. Ball-Physik
  float currentSpeed = RKS.turbo ? RKS.speed * RK_TURBO_MULT : RKS.speed;
  float nx = RKS.ballX + (RKS.dirX * currentSpeed);
  float ny = RKS.ballY + (RKS.dirY * currentSpeed);

  if (nx < 0)           { nx = 0;          RKS.dirX =  abs(RKS.dirX); }
  else if (nx > MAX_X - 1) { nx = MAX_X - 1; RKS.dirX = -abs(RKS.dirX); }
  if (ny < 0)           { ny = 0;          RKS.dirY =  abs(RKS.dirY); }

  // Paddel-Kollision
  if (ny >= MAX_Y - 2.0f && RKS.ballY < MAX_Y - 2.0f) {
    float pl = RKS.paddleX - 0.5f;
    float pr = RKS.paddleX + RK_PAD_W + 0.5f;
    if (nx >= pl && nx <= pr) {
      ny = MAX_Y - 2.5f;
      RKS.dirY = -abs(RKS.dirY);
      float hitPos = (nx - RKS.paddleX) / (float)RK_PAD_W;
      RKS.dirX = (hitPos - 0.5f) * 2.2f;
      float mag = sqrt(RKS.dirX * RKS.dirX + RKS.dirY * RKS.dirY);
      RKS.dirX /= mag;
      RKS.dirY /= mag;
      RKS.turbo = false;
    }
  }

  // Game Over (Boden)
  if (ny > MAX_Y) {
    rk_gameOver     = true;
    rk_gameOverTime = now;
    return;
  }

  // Block-Kollision
  if (ny >= RK_OFFSET_Y && ny < RK_OFFSET_Y + RK_ROWS) {
    int bx = constrain((int)nx / 2, 0, RK_COLS - 1);
    int by = constrain((int)ny - RK_OFFSET_Y, 0, RK_ROWS - 1);
    if (RKS.blocks[bx][by] > 0) {
      rk_hit(bx, by);
      float cx = (bx * 2) + 1.0f;
      float cy = by + RK_OFFSET_Y + 0.5f;
      if (abs(RKS.ballX - cx) > abs(RKS.ballY - cy)) RKS.dirX *= -1;
      else                                             RKS.dirY *= -1;
    }
  }

  RKS.ballX = nx;
  RKS.ballY = ny;

  bool anyBlock = false;
  for (int x = 0; x < RK_COLS && !anyBlock; x++)
    for (int y = 0; y < RK_ROWS && !anyBlock; y++)
      if (RKS.blocks[x][y] > 0) anyBlock = true;
  if (!anyBlock) rk_initGame();
}

void rk_drawGame() {
  unsigned long now = millis();

  for (int x = 0; x < RK_COLS; x++) {
    for (int y = 0; y < RK_ROWS; y++) {
      int type = RKS.blocks[x][y];
      if (type == 0) continue;
      bool show = true;
      if (type == 2) show = ((now / 800) % 2 == 0);
      if (type == 3) show = ((now / 200) % 2 == 0);
      if (show) {
        drawPixel(x * 2,     y + RK_OFFSET_Y, true);
        drawPixel(x * 2 + 1, y + RK_OFFSET_Y, true);
      }
    }
  }

  int px = round(RKS.paddleX);
  for (int i = 0; i < RK_PAD_W; i++) drawPixel(px + i, MAX_Y - 1, true);

  if (!RKS.waitFire || ((now / 200) % 2 == 0))
    drawPixel(round(RKS.ballX), round(RKS.ballY), true);
}

#endif
