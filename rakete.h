#ifndef RAKETE_H
#define RAKETE_H

// --- EINSTELLUNGEN ---
const int   RAK_FRAME_DELAY = 30;   // Zeit in ms zwischen den Frames (kleiner = Update läuft öfter)
const float RAK_SPEED       = 0.4f; // Flug-Geschwindigkeit (größer = Rakete schießt schneller hoch)

const int rakSprite[7][5] = {
  {0,0,1,0,0}, // Spitze
  {0,1,1,1,0},
  {1,1,0,1,1}, 
  {1,1,1,1,1},
  {1,0,1,0,1}, 
  {1,0,1,0,1},
  {0,1,0,1,0}  // Feuer
};

static float rakY         = 32.0f;
static bool  rakLoaded    = false;
static unsigned long rakLastFrame = 0;

bool updateRocketAnimation() {
  if (!rakLoaded) {
    rakY = 32.0f; 
    rakLoaded = true;
  }

  // Position nur updaten, wenn die Frame-Zeit abgelaufen ist
  if (millis() - rakLastFrame >= RAK_FRAME_DELAY) {
    rakLastFrame = millis();
    rakY -= RAK_SPEED; // Fliegt nach OBEN (Y wird kleiner)
  }

  // Rakete zeichnen
  int posY = round(rakY);
  for (int row = 0; row < 7; row++) {
    int drawY = posY + row;
    if (drawY < 0 || drawY >= MAX_Y) continue;
    for (int col = 0; col < 5; col++) {
      if (rakSprite[row][col]) {
        drawPixel(10 + col, drawY, true); // Auf X=10 zentriert
      }
    }
  }

  // Wenn die Rakete komplett oben raus ist
  if (rakY < -8.0f) {
    rakLoaded = false; 
    return true;
  }
  return false;
}

#endif
