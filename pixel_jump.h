#ifndef PIXEL_JUMP_H
#define PIXEL_JUMP_H

// --- EINSTELLUNGEN ---
const float PJ_GRAVITY = 0.075f;
const float PJ_JUMP    = -1.18f;
const float PJ_MAXFALL = 1.8f;
const float PJ_SPACING = 6.0f;
const int   PJ_MAX_P   = 8;

enum PJ_Type { PJ_NORMAL, PJ_MOVING, PJ_BREAKING, PJ_HIGH };
struct PJ_Plat { float x, y; PJ_Type type; bool active, scored; int dir; };

static float   pj_px, pj_py, pj_lastY, pj_vy, pj_scroll;
static int     pj_score;
static PJ_Plat pj_p[PJ_MAX_P];
static bool    pj_gameOver = false;
static unsigned long pj_lastFrame = 0;
static unsigned long pj_gameOverTime = 0;

void pj_initGame() {
  pj_px       = MAX_X / 2.0f;
  pj_py       = 20.0f;
  pj_lastY    = 20.0f;
  pj_vy       = 0.0f;
  pj_scroll   = 0.0f;
  pj_score    = 0;
  pj_gameOver = false;
  pj_gameOverTime = 0;

  for (int i = 0; i < PJ_MAX_P; i++) {
    pj_p[i].x      = random(2, MAX_X - 4);
    pj_p[i].y      = 28.0f - (i * PJ_SPACING);
    pj_p[i].type   = PJ_NORMAL;
    pj_p[i].active = true;
    pj_p[i].scored = false;
    pj_p[i].dir    = (random(0, 2) == 0) ? 1 : -1;
  }
  pj_p[0].x      = pj_px - 1;
  pj_p[0].y      = 28.0f;
  pj_p[0].scored = true;
}

static bool pj_hitX(const PJ_Plat& p, int sx) {
  int cx = round(p.x);
  switch (p.type) {
    case PJ_NORMAL:
    case PJ_BREAKING: return sx >= cx - 1 && sx <= cx + 1;
    case PJ_MOVING:   return sx == cx || sx == cx + 1;
    case PJ_HIGH:     return sx == cx;
    default:          return sx == cx;
  }
}

void pj_updateGame(bool currentStart, bool lastStart) {
  if (pj_gameOver) {
    if (millis() - pj_gameOverTime >= 1000) {
      if (currentStart && !lastStart) pj_initGame();
    }
    return;
  }

  if (millis() - pj_lastFrame < 20) return;
  pj_lastFrame = millis();

  // --- JOYSTICK MIT DRIFT-FIX ---
  int joyX = analogRead(PIN_VRX);
  float moveX = 0.0f;
  if (abs(joyX - 512) >= 100) {
    moveX = map(joyX, 1023, 0, -50, 50) / 100.0f;
  }
  pj_px += moveX;

  if (pj_px < 0)       pj_px = MAX_X - 1;
  if (pj_px >= MAX_X)  pj_px = 0;

  pj_lastY = pj_py;
  pj_vy += PJ_GRAVITY;
  if (pj_vy > PJ_MAXFALL) pj_vy = PJ_MAXFALL;
  pj_py += pj_vy;

  if (pj_vy > 0.0f) {
    int sx = round(pj_px);
    for (int i = 0; i < PJ_MAX_P; i++) {
      if (!pj_p[i].active) continue;
      bool crossed = (pj_lastY <= pj_p[i].y + 0.5f) && (pj_py >= pj_p[i].y - 0.5f);
      if (!crossed || !pj_hitX(pj_p[i], sx)) continue;

      pj_vy = (pj_p[i].type == PJ_HIGH) ? PJ_JUMP * 1.5f : PJ_JUMP;
      pj_py = pj_p[i].y - 0.5f;

      if (!pj_p[i].scored) { pj_score++; pj_p[i].scored = true; }
      if (pj_p[i].type == PJ_BREAKING) pj_p[i].active = false;
      break;
    }
  }

  float screenY = pj_py - pj_scroll;
  if (screenY < 12.0f) pj_scroll -= (12.0f - screenY);

  for (int i = 0; i < PJ_MAX_P; i++) {
    if ((pj_p[i].y - pj_scroll) >= MAX_Y + 2.0f) {
      float topY = pj_p[0].y;
      for (int j = 1; j < PJ_MAX_P; j++) if (pj_p[j].y < topY) topY = pj_p[j].y;

      pj_p[i].y      = topY - PJ_SPACING;
      pj_p[i].x      = random(2, MAX_X - 4);
      pj_p[i].active = true;
      pj_p[i].scored = false;
      pj_p[i].dir    = (random(0, 2) == 0) ? 1 : -1;

      uint8_t r = random(10);
      if      (r < 4) pj_p[i].type = PJ_NORMAL;
      else if (r < 6) pj_p[i].type = PJ_MOVING;
      else if (r < 8) pj_p[i].type = PJ_BREAKING;
      else            pj_p[i].type = PJ_HIGH;
    }

    if (pj_p[i].type == PJ_MOVING && pj_p[i].active) {
      pj_p[i].x += (0.1f * pj_p[i].dir);
      if (pj_p[i].x < 1 || pj_p[i].x > MAX_X - 3) pj_p[i].dir *= -1;
    }
  }

  if ((pj_py - pj_scroll) > MAX_Y + 2.0f) {
    pj_gameOver     = true;
    pj_gameOverTime = millis();
  }
}

void pj_drawGame() {
  unsigned long now = millis();

  for (int i = 0; i < PJ_MAX_P; i++) {
    if (!pj_p[i].active) continue;
    int py = round(pj_p[i].y - pj_scroll);
    if (py < 0 || py >= MAX_Y) continue;
    int px = round(pj_p[i].x);
    bool drawIt = true;

    if (pj_p[i].type == PJ_BREAKING && (now % 200 < 100)) drawIt = false;
    if (pj_p[i].type == PJ_HIGH     && (now % 600 < 300)) drawIt = false;

    if (drawIt) {
      drawPixel(px, py, true);
      if (pj_p[i].type == PJ_NORMAL || pj_p[i].type == PJ_BREAKING) {
        drawPixel(px - 1, py, true); drawPixel(px + 1, py, true);
      } else if (pj_p[i].type == PJ_MOVING) {
        drawPixel(px + 1, py, true);
      }
    }
  }

  int sy = round(pj_py - pj_scroll);
  if (sy >= 0 && sy < MAX_Y) drawPixel(round(pj_px), sy, true);
}

#endif
