#ifndef PIXEL_BLOCK_H
#define PIXEL_BLOCK_H

// --- EINSTELLUNGEN ---
const int PB_FALL_START = 350;
const int PB_FALL_MIN   = 60;
const int PB_FALL_STEP  = 10;
const int PB_SOFT_DROP  = 40;

const int TET_W = 12;
const int TET_H = 32;
const int TET_X = 6;

static const uint16_t TET_SHAPES[7][4] = {
  { 0x0F00, 0x2222, 0x00F0, 0x4444 }, // I
  { 0x0660, 0x0660, 0x0660, 0x0660 }, // O
  { 0x0E40, 0x4C40, 0x4E00, 0x4640 }, // T
  { 0x06C0, 0x8C40, 0x06C0, 0x8C40 }, // S
  { 0x0C60, 0x4C80, 0x0C60, 0x4C80 }, // Z
  { 0x0E20, 0x44C0, 0x8E00, 0xC880 }, // J
  { 0x0E80, 0xC440, 0x2E00, 0x88C0 }  // L
};

struct PB_State {
  int fallSpeed;
  int piece;
  int rot;
  int px;
  int py;
  int score;
  unsigned long lastFall;
  bool board[TET_W][TET_H];
};

// FIX: static verhindert "multiple definition"-Fehler
static PB_State PBS;
static bool pb_gameOver = false;
static unsigned long pb_gameOverTime = 0;
static unsigned long pb_lastMoveX = 0;

static bool pb_canMove(int px, int py, int rot, int piece) {
  uint16_t shape = TET_SHAPES[piece][rot];
  for (int i = 0; i < 16; i++) {
    if (!(shape & (1 << (15 - i)))) continue;
    int bx = px + (i % 4);
    int by = py + (i / 4);
    if (bx < 0 || bx >= TET_W)        return false;
    if (by >= TET_H)                   return false;
    if (by >= 0 && PBS.board[bx][by]) return false;
  }
  return true;
}

static void pb_clearLines() {
  for (int y = TET_H - 1; y >= 0; y--) {
    bool full = true;
    for (int x = 0; x < TET_W; x++) {
      if (!PBS.board[x][y]) { full = false; break; }
    }
    if (full) {
      PBS.score++;
      PBS.fallSpeed = max(PB_FALL_MIN, PBS.fallSpeed - PB_FALL_STEP);
      for (int yy = y; yy > 0; yy--)
        for (int x = 0; x < TET_W; x++)
          PBS.board[x][yy] = PBS.board[x][yy - 1];
      for (int x = 0; x < TET_W; x++) PBS.board[x][0] = false;
      y++;
    }
  }
}

static void pb_lock() {
  uint16_t shape = TET_SHAPES[PBS.piece][PBS.rot];
  for (int i = 0; i < 16; i++) {
    if (!(shape & (1 << (15 - i)))) continue;
    int bx = PBS.px + (i % 4);
    int by = PBS.py + (i / 4);
    if (bx >= 0 && bx < TET_W && by >= 0 && by < TET_H)
      PBS.board[bx][by] = true;
  }
  pb_clearLines();
}

static void pb_spawn() {
  PBS.piece = random(0, 7);
  PBS.rot   = 0;
  PBS.px    = (TET_W - 4) / 2;
  PBS.py    = -1;
  if (!pb_canMove(PBS.px, PBS.py, PBS.rot, PBS.piece)) {
    pb_gameOver = true;
    pb_gameOverTime = millis();
  }
}

void pb_initGame() {
  for (int x = 0; x < TET_W; x++)
    for (int y = 0; y < TET_H; y++)
      PBS.board[x][y] = false;
  PBS.fallSpeed = PB_FALL_START;
  PBS.score     = 0;
  PBS.lastFall  = millis();
  pb_gameOver   = false;
  pb_lastMoveX  = 0;
  pb_spawn();
}

void pb_updateGame(bool currentAction, bool lastAction, bool currentStart, bool lastStart) {
  unsigned long now = millis();

  if (pb_gameOver) {
    if (now - pb_gameOverTime >= 1000) {
      if (currentStart || currentAction) pb_initGame();
    }
    return;
  }

  int joyX = analogRead(PIN_VRX);
  int joyY = analogRead(PIN_VRY);

  if (now - pb_lastMoveX > 120) {
    if (joyX > 800) {
      if (pb_canMove(PBS.px - 1, PBS.py, PBS.rot, PBS.piece)) { PBS.px--; pb_lastMoveX = now; }
    } else if (joyX < 200) {
      if (pb_canMove(PBS.px + 1, PBS.py, PBS.rot, PBS.piece)) { PBS.px++; pb_lastMoveX = now; }
    }
  }

  if (currentAction && !lastAction) {
    int nextRot = (PBS.rot + 1) % 4;
    if      (pb_canMove(PBS.px,     PBS.py, nextRot, PBS.piece)) { PBS.rot = nextRot; }
    else if (pb_canMove(PBS.px + 1, PBS.py, nextRot, PBS.piece)) { PBS.px++; PBS.rot = nextRot; }
    else if (pb_canMove(PBS.px - 1, PBS.py, nextRot, PBS.piece)) { PBS.px--; PBS.rot = nextRot; }
  }

  unsigned long interval = (joyY > 800) ? (unsigned long)PB_SOFT_DROP : (unsigned long)PBS.fallSpeed;

  if (now - PBS.lastFall > interval) {
    PBS.lastFall = now;
    if (pb_canMove(PBS.px, PBS.py + 1, PBS.rot, PBS.piece)) {
      PBS.py++;
    } else {
      pb_lock();
      if (!pb_gameOver) pb_spawn();
    }
  }
}

void pb_drawGame() {
  // Seitenränder
  for (int y = 0; y < TET_H; y++) {
    drawPixel(TET_X - 1, y, true);
    drawPixel(TET_X + TET_W, y, true);
  }
  // Spielfeld
  for (int x = 0; x < TET_W; x++)
    for (int y = 0; y < TET_H; y++)
      if (PBS.board[x][y]) drawPixel(TET_X + x, y, true);
  // Aktuelles Teil
  uint16_t shape = TET_SHAPES[PBS.piece][PBS.rot];
  for (int i = 0; i < 16; i++) {
    if (!(shape & (1 << (15 - i)))) continue;
    int bx = PBS.px + (i % 4);
    int by = PBS.py + (i / 4);
    if (bx >= 0 && bx < TET_W && by >= 0 && by < TET_H)
      drawPixel(TET_X + bx, by, true);
  }
}

#endif
