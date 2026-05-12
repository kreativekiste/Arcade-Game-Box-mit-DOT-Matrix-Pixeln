#ifndef PIXEL_WILD_H
#define PIXEL_WILD_H

// --- EINSTELLUNGEN ---
const int   PW_MAX_OBS     = 12;
const float PW_SPEED_SLOW  = 0.10f;
const float PW_SPEED_NORM  = 0.20f;
const float PW_SPEED_FAST  = 0.35f;
const int   PW_SPACING_MIN = 10;
const int   PW_SPACING_MAX = 35;

struct PW_Obs {
  float x;
  int   y;
  int   w;
  int   h;
  float speed;
  int   dir;
  bool  active;
};

struct PW_State {
  int    px, py;
  int    level;
  PW_Obs obs[PW_MAX_OBS];
};

static PW_State PWS;
static bool pw_gameOver = false;
static unsigned long pw_gameOverTime = 0;
static unsigned long pw_lastMove = 0;
static unsigned long pw_levelUpTime = 0;

static void pw_spawnObs(int i) {
  PWS.obs[i].w   = random(1, 9);
  PWS.obs[i].h   = random(1, 4);
  PWS.obs[i].y   = random(2, MAX_Y - PWS.obs[i].h - 2);
  PWS.obs[i].dir = (random(0, 2) == 0) ? 1 : -1;

  int speedCat = random(0, 3);
  float baseSpeed = 0.0f;
  if      (speedCat == 0) baseSpeed = PW_SPEED_SLOW;
  else if (speedCat == 1) baseSpeed = PW_SPEED_NORM;
  else                    baseSpeed = PW_SPEED_FAST;

  baseSpeed += (PWS.level - 1) * 0.02f;
  float variance = random(85, 116) / 100.0f;
  PWS.obs[i].speed = baseSpeed * variance;

  float startX = (PWS.obs[i].dir == 1) ? -PWS.obs[i].w : (float)MAX_X;
  for (int j = 0; j < PW_MAX_OBS; j++) {
    if (j != i && PWS.obs[j].active && PWS.obs[j].dir == PWS.obs[i].dir) {
      if (PWS.obs[i].dir ==  1 && PWS.obs[j].x < startX) startX = PWS.obs[j].x;
      if (PWS.obs[i].dir == -1 && PWS.obs[j].x > startX) startX = PWS.obs[j].x;
    }
  }

  int spacing = random(PW_SPACING_MIN, PW_SPACING_MAX);
  if (PWS.obs[i].dir == 1) PWS.obs[i].x = startX - spacing - PWS.obs[i].w;
  else                      PWS.obs[i].x = startX + spacing;

  PWS.obs[i].active = true;
}

void pw_initGame() {
  PWS.px         = MAX_X / 2;
  PWS.py         = MAX_Y - 1;
  PWS.level      = 1;
  pw_gameOver    = false;
  pw_gameOverTime = 0;
  pw_levelUpTime  = 0;
  for (int i = 0; i < PW_MAX_OBS; i++) pw_spawnObs(i);
}

void pw_updateGame(bool currentAction, bool lastAction, bool currentStart, bool lastStart) {
  unsigned long now = millis();

  if (pw_gameOver) {
    if (now - pw_gameOverTime >= 1000) {
      if ((currentStart && !lastStart) || (currentAction && !lastAction)) pw_initGame();
    }
    return;
  }

  if (pw_levelUpTime > 0) {
    if (now - pw_levelUpTime > 500) pw_levelUpTime = 0;
    else return;
  }

  int joyX = analogRead(PIN_VRX);
  int joyY = analogRead(PIN_VRY);

  if (now - pw_lastMove > 120) {
    if (joyX > 800 && PWS.px > 0)         { PWS.px--; pw_lastMove = now; }
    if (joyX < 200 && PWS.px < MAX_X - 1) { PWS.px++; pw_lastMove = now; }
    if (joyY < 200 && PWS.py > 0)         { PWS.py--; pw_lastMove = now; }
    if (joyY > 800 && PWS.py < MAX_Y - 1) { PWS.py++; pw_lastMove = now; }
  }

  if (PWS.py <= 1) {
    PWS.level++;
    PWS.px = MAX_X / 2;
    PWS.py = MAX_Y - 1;
    pw_levelUpTime = now;
    for (int i = 0; i < PW_MAX_OBS; i++) pw_spawnObs(i);
    return;
  }

  for (int i = 0; i < PW_MAX_OBS; i++) {
    if (!PWS.obs[i].active) { pw_spawnObs(i); continue; }

    PWS.obs[i].x += (PWS.obs[i].dir * PWS.obs[i].speed);

    if      (PWS.obs[i].dir ==  1 && PWS.obs[i].x > MAX_X)             PWS.obs[i].active = false;
    else if (PWS.obs[i].dir == -1 && PWS.obs[i].x < -PWS.obs[i].w)     PWS.obs[i].active = false;

    int ox = round(PWS.obs[i].x);
    int oy = PWS.obs[i].y;
    if (PWS.px >= ox && PWS.px < ox + PWS.obs[i].w &&
        PWS.py >= oy && PWS.py < oy + PWS.obs[i].h) {
      pw_gameOver     = true;
      pw_gameOverTime = now;
    }
  }
}

void pw_drawGame() {
  unsigned long now = millis();

  // Start- und Zielzone
  for (int x = 0; x < MAX_X; x += 2) {
    drawPixel(x, 1,          true);
    drawPixel(x, MAX_Y - 2,  true);
  }

  for (int i = 0; i < PW_MAX_OBS; i++) {
    if (!PWS.obs[i].active) continue;
    int ox = round(PWS.obs[i].x);
    int oy = PWS.obs[i].y;
    for (int w = 0; w < PWS.obs[i].w; w++) {
      for (int h = 0; h < PWS.obs[i].h; h++) {
        int drawX = ox + w, drawY = oy + h;
        if (drawX >= 0 && drawX < MAX_X && drawY >= 0 && drawY < MAX_Y)
          drawPixel(drawX, drawY, true);
      }
    }
  }

  if (pw_levelUpTime > 0) {
    if ((now / 100) % 2 == 0)
      for (int x = 0; x < MAX_X; x++)
        for (int y = 0; y < MAX_Y; y++)
          drawPixel(x, y, true);
    return;
  }

  if ((now / 150) % 2 == 0) drawPixel(PWS.px, PWS.py, true);
}

#endif
