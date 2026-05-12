#ifndef PIXEL_RUN_H
#define PIXEL_RUN_H

// --- EINSTELLUNGEN ---
const float PR_GRAVITY      = 0.08f;
const float PR_GRAVITY_LOW  = 0.03f;
const float PR_JUMP_POWER   = -0.75f;
const float PR_SPEED_START  = 0.20f;
const float PR_SPEED_MAX    = 0.80f;
const float PR_SPEED_INC    = 0.005f;

const int PR_MAX_OBS = 3;
const int PR_DINO_X  = 18;
const int PR_GROUND  = 31;
const int PR_FLOOR   = 30;

struct PR_State {
  float dinoY;
  float dinoVel;
  bool  isJumping;
  float gameSpeed;
  int   score;
  float obsX[PR_MAX_OBS];
  int   obsType[PR_MAX_OBS];
  bool  obsOn[PR_MAX_OBS];
};

static PR_State PRS;
static bool pr_gameOver = false;
static unsigned long pr_gameOverTime = 0;
static unsigned long pr_lastFrame = 0;
static bool pr_jumpBuffered = false;

static void pr_spawnObs(int i) {
  float minX = 0.0f;
  for (int j = 0; j < PR_MAX_OBS; j++) {
    if (PRS.obsOn[j] && PRS.obsX[j] < minX) minX = PRS.obsX[j];
  }
  PRS.obsX[i]    = minX - random(14, 28);
  PRS.obsType[i] = random(1, 3);
  PRS.obsOn[i]   = true;
}

void pr_initGame() {
  PRS.dinoY     = (float)PR_FLOOR;
  PRS.dinoVel   = 0.0f;
  PRS.isJumping = false;
  PRS.gameSpeed = PR_SPEED_START;
  PRS.score     = 0;

  pr_gameOver     = false;
  pr_gameOverTime = 0;
  pr_jumpBuffered = false;

  for (int i = 0; i < PR_MAX_OBS; i++) PRS.obsOn[i] = false;
  pr_spawnObs(0);
}

void pr_updateGame(bool currentAction, bool lastAction, bool currentStart, bool lastStart) {
  unsigned long now = millis();

  if ((currentAction && !lastAction) || (currentStart && !lastStart)) pr_jumpBuffered = true;
  bool btnHeld = (currentAction || currentStart);

  if (pr_gameOver) {
    if (now - pr_gameOverTime >= 1000) {
      if (pr_jumpBuffered) pr_initGame();
    } else {
      pr_jumpBuffered = false;
    }
    return;
  }

  if (now - pr_lastFrame < 20) return;
  pr_lastFrame = now;

  if (pr_jumpBuffered && !PRS.isJumping && PRS.dinoY >= (float)PR_FLOOR - 0.2f) {
    PRS.dinoVel   = PR_JUMP_POWER;
    PRS.isJumping = true;
  }
  pr_jumpBuffered = false;

  if (btnHeld && PRS.dinoVel < 0.0f) PRS.dinoVel += PR_GRAVITY_LOW;
  else                                PRS.dinoVel += PR_GRAVITY;
  PRS.dinoY += PRS.dinoVel;

  if (PRS.dinoY >= (float)PR_FLOOR) {
    PRS.dinoY     = (float)PR_FLOOR;
    PRS.dinoVel   = 0.0f;
    PRS.isJumping = false;
  }
  if (PRS.dinoY < 0.0f) { PRS.dinoY = 0.0f; PRS.dinoVel = 0.0f; }

  for (int i = 0; i < PR_MAX_OBS; i++) {
    if (!PRS.obsOn[i]) { pr_spawnObs(i); continue; }

    PRS.obsX[i] += PRS.gameSpeed;

    if (PRS.obsX[i] > (float)MAX_X + 2.0f) {
      PRS.score++;
      PRS.gameSpeed = min(PR_SPEED_MAX, PRS.gameSpeed + PR_SPEED_INC);
      PRS.obsOn[i]  = false;
      continue;
    }

    int rdY    = round(PRS.dinoY);
    int roX    = round(PRS.obsX[i]);
    int obsTop = (PRS.obsType[i] == 1) ? PR_GROUND - 2 : PR_GROUND - 3;

    if (roX >= PR_DINO_X - 1 && roX <= PR_DINO_X + 1) {
      if (rdY >= obsTop) {
        pr_gameOver     = true;
        pr_gameOverTime = now;
        return;
      }
    }
  }
}

void pr_drawGame() {
  for (int x = 0; x < MAX_X; x++) drawPixel(x, PR_GROUND, true);

  for (int i = 0; i < PR_MAX_OBS; i++) {
    if (!PRS.obsOn[i]) continue;
    int rx = round(PRS.obsX[i]);
    if (rx < 0 || rx >= MAX_X) continue;
    if (PRS.obsType[i] == 1) {
      drawPixel(rx, PR_GROUND - 1, true);
      drawPixel(rx, PR_GROUND - 2, true);
    } else {
      drawPixel(rx,     PR_GROUND - 1, true);
      drawPixel(rx,     PR_GROUND - 2, true);
      drawPixel(rx,     PR_GROUND - 3, true);
      drawPixel(rx - 1, PR_GROUND - 2, true);
    }
  }

  int y = (int)round(PRS.dinoY);
  drawPixel(PR_DINO_X,     y,     true);
  drawPixel(PR_DINO_X + 1, y,     true);
  drawPixel(PR_DINO_X - 1, y - 1, true);
  drawPixel(PR_DINO_X,     y - 1, true);
  drawPixel(PR_DINO_X + 1, y - 1, true);
  drawPixel(PR_DINO_X - 1, y - 2, true);
  drawPixel(PR_DINO_X,     y - 2, true);
}

#endif
