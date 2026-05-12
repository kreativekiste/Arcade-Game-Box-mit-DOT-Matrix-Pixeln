#ifndef PIXEL_DOT_H
#define PIXEL_DOT_H

// --- EINSTELLUNGEN ---
const float FB_GRAVITY = 0.08f;
const float FB_JUMP    = -0.60f;
const float FB_MAXFALL = 0.80f;
const float FB_SPEED   = 0.20f;
const int   FB_GAP     = 10;

static float fb_py;
static float fb_vy;
static float fb_pipeX;
static int   fb_gapY;
static int   fb_score;
static bool  fb_scored;
static bool  fb_gameOver;
static bool  fb_gameStarted;
static unsigned long fb_lastFrame = 0;
static unsigned long fb_gameOverTime = 0;

static bool fb_jumpBuffered = false;
static bool fb_startBuffered = false;

void fb_initGame() {
  fb_py          = 14.0f;
  fb_vy          = 0.0f;
  fb_pipeX       = (float)MAX_X;
  fb_gapY        = random(4, MAX_Y - FB_GAP - 4);
  fb_score       = 0;
  fb_scored      = false;
  fb_gameOver    = false;
  fb_gameStarted = false;
  fb_jumpBuffered  = false;
  fb_startBuffered = false;
  fb_gameOverTime  = 0;
}

static bool fb_checkCollision() {
  int by = round(fb_py);
  int px = round(fb_pipeX);

  if (by <= 0 || by >= MAX_Y - 1) return true;

  if (px <= 7 && px + 1 >= 6) {
    if (by < fb_gapY || by + 1 > fb_gapY + FB_GAP) return true;
  }
  return false;
}

void fb_updateGame(bool currentAction, bool lastAction, bool currentStart, bool lastStart) {
  if (currentAction && !lastAction) fb_jumpBuffered = true;
  if (currentStart && !lastStart)   fb_startBuffered = true;

  if (fb_gameOver) {
    if (millis() - fb_gameOverTime >= 1000) {
      if (fb_startBuffered || fb_jumpBuffered) fb_initGame();
    } else {
      fb_jumpBuffered  = false;
      fb_startBuffered = false;
    }
    return;
  }

  if (millis() - fb_lastFrame < 20) return;
  fb_lastFrame = millis();

  if (!fb_gameStarted) {
    if (fb_jumpBuffered) {
      fb_gameStarted = true;
      fb_vy = FB_JUMP;
      fb_jumpBuffered = false;
    }
    return;
  }

  if (fb_jumpBuffered) {
    fb_vy = FB_JUMP;
    fb_jumpBuffered = false;
  }

  fb_vy += FB_GRAVITY;
  if (fb_vy > FB_MAXFALL) fb_vy = FB_MAXFALL;
  fb_py += fb_vy;

  fb_pipeX -= FB_SPEED;

  if (fb_pipeX < 6.0f && !fb_scored) {
    fb_score++;
    fb_scored = true;
  }

  if (fb_pipeX < -2.0f) {
    fb_pipeX = (float)MAX_X;
    fb_gapY  = random(4, MAX_Y - FB_GAP - 4);
    fb_scored = false;
  }

  if (fb_checkCollision()) {
    fb_gameOver    = true;
    fb_gameOverTime = millis();
  }
}

void fb_drawGame() {
  int px = round(fb_pipeX);
  for (int y = 0; y < MAX_Y; y++) {
    if (y < fb_gapY || y > fb_gapY + FB_GAP) {
      drawPixel(px,     y, true);
      drawPixel(px + 1, y, true);
    }
  }

  int bx = 6;
  int by = round(fb_py);
  drawPixel(bx,     by,     true);
  drawPixel(bx + 1, by,     true);
  drawPixel(bx,     by + 1, true);
  drawPixel(bx + 1, by + 1, true);
}

#endif
