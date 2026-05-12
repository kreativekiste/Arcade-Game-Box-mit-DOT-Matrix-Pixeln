#ifndef PIXEL_INVADER_H
#define PIXEL_INVADER_H

// --- EINSTELLUNGEN ---
const int SI_ENEMY_SPEED_START = 900;
const int SI_ENEMY_MIN_SPEED   = 225;
const int SI_ENEMY_ACCEL       = 10;
const int SI_SHOT_INTERVAL     = 300;
const int SI_DROP_CHANCE       = 15;
const int SI_DROP_SPEED        = 120;

const int SI_MAX_ENEMIES = 5;
const int SI_MAX_BULLETS = 6;
const int SI_MAX_ESHOTS  = 4;

struct SI_Enemy {
  int    x, y, w, h;
  bool   active;
  int8_t type;
  bool   shape[4][4];
};

struct SI_Drop {
  int           x, y;
  bool          active;
  unsigned long lastMove;
};

struct SI_State {
  int           px, py;
  int           collectedDrops;
  int           enemySpeed;
  unsigned long lastEnemyMove;
  unsigned long lastShot;
  int           score;

  int  bx[SI_MAX_BULLETS];
  int  by[SI_MAX_BULLETS];
  bool ba[SI_MAX_BULLETS];

  int           esx[SI_MAX_ESHOTS];
  int           esy[SI_MAX_ESHOTS];
  bool          esa[SI_MAX_ESHOTS];
  unsigned long eShotTimer;

  SI_Drop  drop;
  SI_Enemy enemies[SI_MAX_ENEMIES];

  uint8_t lives;
};

// FIX: static verhindert "multiple definition"-Fehler
static SI_State SIS;
static bool si_gameOver = false;
static unsigned long si_gameOverTime = 0;
static unsigned long si_lastFrame = 0;

static int si_freeSlot() {
  for (int i = 0; i < SI_MAX_BULLETS; i++)
    if (!SIS.ba[i]) return i;
  return -1;
}

static void si_fire(int x, int y) {
  int s = si_freeSlot();
  if (s == -1) return;
  SIS.ba[s] = true; SIS.bx[s] = x; SIS.by[s] = y;
}

static void si_drawShip(int cx, int py) {
  drawPixel(cx, py - 2, true);
  drawPixel(cx - 1, py - 1, true); drawPixel(cx, py - 1, true); drawPixel(cx + 1, py - 1, true);
  drawPixel(cx - 2, py, true); drawPixel(cx - 1, py, true); drawPixel(cx, py, true); drawPixel(cx + 1, py, true); drawPixel(cx + 2, py, true);
}

static void si_spawnEnemy(int id) {
  int8_t t = (int8_t)random(1, 4);
  SIS.enemies[id].type   = t;
  SIS.enemies[id].active = true;
  for (int a = 0; a < 4; a++)
    for (int b = 0; b < 4; b++)
      SIS.enemies[id].shape[a][b] = false;

  if (t == 1) {
    SIS.enemies[id].w = 3; SIS.enemies[id].h = 2;
    SIS.enemies[id].shape[1][0] = true;
    SIS.enemies[id].shape[0][1] = true; SIS.enemies[id].shape[2][1] = true;
  } else if (t == 2) {
    SIS.enemies[id].w = 3; SIS.enemies[id].h = 3;
    SIS.enemies[id].shape[1][0] = true;
    SIS.enemies[id].shape[0][1] = true; SIS.enemies[id].shape[1][1] = true; SIS.enemies[id].shape[2][1] = true;
    SIS.enemies[id].shape[0][2] = true; SIS.enemies[id].shape[2][2] = true;
  } else {
    SIS.enemies[id].w = 4; SIS.enemies[id].h = 3;
    SIS.enemies[id].shape[1][0] = true; SIS.enemies[id].shape[2][0] = true;
    SIS.enemies[id].shape[0][1] = true; SIS.enemies[id].shape[1][1] = true;
    SIS.enemies[id].shape[2][1] = true; SIS.enemies[id].shape[3][1] = true;
    SIS.enemies[id].shape[0][2] = true; SIS.enemies[id].shape[3][2] = true;
  }
  SIS.enemies[id].x = random(0, MAX_X - SIS.enemies[id].w);
  SIS.enemies[id].y = -random(2, 14);
}

static void si_respawn() {
  SIS.px = 12; SIS.py = 28;
  for (int i = 0; i < SI_MAX_BULLETS; i++) SIS.ba[i]  = false;
  for (int i = 0; i < SI_MAX_ESHOTS;  i++) SIS.esa[i] = false;
  for (int i = 0; i < SI_MAX_ENEMIES; i++) si_spawnEnemy(i);
}

static void si_die() {
  if (SIS.lives > 0) {
    SIS.lives--;
    si_respawn();
  } else {
    si_gameOver = true;
    si_gameOverTime = millis();
  }
}

void si_initGame() {
  SIS.px             = 12;
  SIS.py             = 28;
  SIS.collectedDrops = 0;
  SIS.enemySpeed     = SI_ENEMY_SPEED_START;
  SIS.lastShot       = 0;
  SIS.lastEnemyMove  = 0;
  SIS.score          = 0;
  SIS.drop.active    = false;
  SIS.eShotTimer     = millis() + 3000;
  si_gameOver        = false;

  for (int i = 0; i < SI_MAX_BULLETS; i++) SIS.ba[i]  = false;
  for (int i = 0; i < SI_MAX_ESHOTS;  i++) SIS.esa[i] = false;
  for (int i = 0; i < SI_MAX_ENEMIES; i++) si_spawnEnemy(i);

  SIS.lives = 3;
}

void si_updateGame(bool currentAction, bool lastAction, bool currentStart, bool lastStart) {
  unsigned long now = millis();

  if (si_gameOver) {
    if (now - si_gameOverTime >= 1000) {
      if (currentStart || currentAction) si_initGame();
    }
    return;
  }

  if (now - si_lastFrame < 40) return;
  si_lastFrame = now;

  int joyX = analogRead(PIN_VRX);
  int joyY = analogRead(PIN_VRY);

  if (joyX > 800 && SIS.px > 0)          SIS.px--;
  if (joyX < 200 && SIS.px < MAX_X - 1)  SIS.px++;
  if (joyY < 200 && SIS.py > 16)         SIS.py--;
  if (joyY > 800 && SIS.py < MAX_Y - 1)  SIS.py++;

  if (currentAction && now - SIS.lastShot > (unsigned long)SI_SHOT_INTERVAL) {
    if (SIS.collectedDrops >= 10) {
      si_fire(SIS.px - 1, SIS.py - 2);
      si_fire(SIS.px + 1, SIS.py - 2);
    } else {
      si_fire(SIS.px, SIS.py - 3);
    }
    SIS.lastShot = now;
  }

  for (int i = 0; i < SI_MAX_BULLETS; i++) {
    if (!SIS.ba[i]) continue;
    SIS.by[i]--;
    if (SIS.by[i] < 0) { SIS.ba[i] = false; continue; }

    for (int e = 0; e < SI_MAX_ENEMIES; e++) {
      if (!SIS.enemies[e].active) continue;
      int lx = SIS.bx[i] - SIS.enemies[e].x;
      int ly = SIS.by[i] - SIS.enemies[e].y;
      if (lx < 0 || lx >= SIS.enemies[e].w) continue;
      if (ly < 0 || ly >= SIS.enemies[e].h) continue;
      if (!SIS.enemies[e].shape[lx][ly]) continue;

      SIS.enemies[e].shape[lx][ly] = false;
      SIS.score++;
      SIS.ba[i] = false;

      if (!SIS.drop.active && random(100) < SI_DROP_CHANCE) {
        SIS.drop.x       = SIS.bx[i];
        SIS.drop.y       = SIS.by[i];
        SIS.drop.active  = true;
        SIS.drop.lastMove = now;
      }

      bool any = false;
      for (int ax = 0; ax < 4 && !any; ax++)
        for (int ay = 0; ay < 4 && !any; ay++)
          if (SIS.enemies[e].shape[ax][ay]) any = true;
      if (!any) si_spawnEnemy(e);
      break;
    }
  }

  if (now >= SIS.eShotTimer) {
    int cands[SI_MAX_ENEMIES], cnt = 0;
    for (int i = 0; i < SI_MAX_ENEMIES; i++) {
      if (!SIS.enemies[i].active || SIS.enemies[i].type != 1) continue;
      if (SIS.enemies[i].y >= 0 && SIS.enemies[i].y < MAX_Y) cands[cnt++] = i;
    }
    if (cnt > 0) {
      int s = cands[random(cnt)];
      for (int i = 0; i < SI_MAX_ESHOTS; i++) {
        if (!SIS.esa[i]) {
          SIS.esa[i] = true;
          SIS.esx[i] = SIS.enemies[s].x + 1;
          SIS.esy[i] = SIS.enemies[s].y + SIS.enemies[s].h;
          break;
        }
      }
    }
    SIS.eShotTimer = now + random(1500, 3500);
  }

  for (int i = 0; i < SI_MAX_ESHOTS; i++) {
    if (!SIS.esa[i]) continue;
    SIS.esy[i]++;
    if (SIS.esy[i] >= MAX_Y) { SIS.esa[i] = false; continue; }
    if (SIS.esy[i] >= SIS.py - 2 && SIS.esy[i] <= SIS.py && abs(SIS.esx[i] - SIS.px) <= 2) {
      SIS.esa[i] = false;
      si_die(); return;
    }
  }

  if (SIS.drop.active) {
    if (now - SIS.drop.lastMove > (unsigned long)SI_DROP_SPEED) {
      SIS.drop.y++;
      SIS.drop.lastMove = now;
      if (SIS.drop.y >= MAX_Y) SIS.drop.active = false;
    }
    if (SIS.drop.active && SIS.drop.y >= SIS.py - 2 && SIS.drop.y <= SIS.py && abs(SIS.drop.x - SIS.px) <= 2) {
      SIS.collectedDrops++;
      if (SIS.collectedDrops % 5 == 0 && SIS.lives < 5) SIS.lives++;
      SIS.drop.active = false;
    }
  }

  if (now - SIS.lastEnemyMove > (unsigned long)SIS.enemySpeed) {
    SIS.lastEnemyMove = now;
    if (SIS.enemySpeed > SI_ENEMY_MIN_SPEED) SIS.enemySpeed -= SI_ENEMY_ACCEL;
    for (int i = 0; i < SI_MAX_ENEMIES; i++) {
      if (!SIS.enemies[i].active) continue;
      SIS.enemies[i].y++;
      if (SIS.enemies[i].y > MAX_Y) { si_spawnEnemy(i); continue; }
      for (int ex = 0; ex < SIS.enemies[i].w; ex++) {
        for (int ey = 0; ey < SIS.enemies[i].h; ey++) {
          if (!SIS.enemies[i].shape[ex][ey]) continue;
          int gx = SIS.enemies[i].x + ex, gy = SIS.enemies[i].y + ey;
          if (gy >= SIS.py - 2 && gy <= SIS.py && abs(gx - SIS.px) <= 2) {
            si_die(); return;
          }
        }
      }
    }
  }
}

void si_drawGame() {
  si_drawShip(SIS.px, SIS.py);

  for (int i = 0; i < SI_MAX_BULLETS; i++)
    if (SIS.ba[i] && SIS.by[i] >= 0 && SIS.by[i] < MAX_Y)
      drawPixel(SIS.bx[i], SIS.by[i], true);

  for (int i = 0; i < SI_MAX_ESHOTS; i++)
    if (SIS.esa[i] && SIS.esy[i] >= 0 && SIS.esy[i] < MAX_Y)
      drawPixel(SIS.esx[i], SIS.esy[i], true);

  if (SIS.drop.active && SIS.drop.y >= 0 && SIS.drop.y < MAX_Y && (millis() / 100) % 2 == 0)
    drawPixel(SIS.drop.x, SIS.drop.y, true);

  for (int i = 0; i < SI_MAX_ENEMIES; i++) {
    if (!SIS.enemies[i].active) continue;
    for (int x = 0; x < SIS.enemies[i].w; x++)
      for (int y = 0; y < SIS.enemies[i].h; y++) {
        if (!SIS.enemies[i].shape[x][y]) continue;
        int dy = SIS.enemies[i].y + y;
        if (dy >= 0 && dy < MAX_Y)
          drawPixel(SIS.enemies[i].x + x, dy, true);
      }
  }

  for (uint8_t i = 0; i < SIS.lives && i < 5; i++)
    drawPixel(i * 3, 0, true);
}

#endif
