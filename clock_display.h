#ifndef CLOCK_DISPLAY_H
#define CLOCK_DISPLAY_H

const uint8_t font3x5[14][5] = {
  {0b111, 0b101, 0b101, 0b101, 0b111}, // 0
  {0b010, 0b110, 0b010, 0b010, 0b111}, // 1
  {0b111, 0b001, 0b111, 0b100, 0b111}, // 2
  {0b111, 0b001, 0b111, 0b001, 0b111}, // 3
  {0b101, 0b101, 0b111, 0b001, 0b001}, // 4
  {0b111, 0b100, 0b111, 0b001, 0b111}, // 5
  {0b111, 0b100, 0b111, 0b101, 0b111}, // 6
  {0b111, 0b001, 0b010, 0b100, 0b100}, // 7
  {0b111, 0b101, 0b111, 0b101, 0b111}, // 8
  {0b111, 0b101, 0b111, 0b001, 0b111}, // 9
  {0b000, 0b100, 0b000, 0b100, 0b000}, // : (10)
  {0b000, 0b000, 0b000, 0b000, 0b100}, // . (11)
  {0b100, 0b000, 0b000, 0b000, 0b000}, // * -> Grad (12)
  {0b111, 0b100, 0b100, 0b100, 0b111}  // C (13)
};

void drawChar3x5(int startX, int startY, char c) {
  int idx = -1;
  if (c >= '0' && c <= '9') idx = c - '0';
  else if (c == ':') idx = 10;
  else if (c == '.') idx = 11;
  else if (c == '*') idx = 12; 
  else if (c == 'C') idx = 13;
  if (idx == -1) return; 
  for (int y = 0; y < 5; y++) {
    for (int x = 0; x < 3; x++) {
      if (bitRead(font3x5[idx][y], 2 - x)) drawPixel(startX + x, startY + y, true);
    }
  }
}

void drawString(int x, int y, const char* str) {
  int cx = x;
  for (int i = 0; str[i] != '\0'; i++) {
    if (str[i] == ' ') { 
      cx += 2; 
      continue; 
    }
    drawChar3x5(cx, y, str[i]);
    if (str[i] == ':' || str[i] == '.' || str[i] == '*') cx += 2;
    else cx += 4;
  }
}

static unsigned long lastTempFetch = 0;
static float currentTemp = 0.0;

void showIdleScreen() {
  unsigned long now = millis();
  if (now - lastTempFetch > 3000 || lastTempFetch == 0) {
    currentTemp = readI2CTemp();
    lastTempFetch = now;
  }
  DateTime t = rtc.now();

  // Zeile 1: 00:00 (x=3)
  char timeStr1[6];
  sprintf(timeStr1, "%02d:%02d", t.hour(), t.minute());
  drawString(3, 2, timeStr1); 

  // Zeile 2: Sekunden (x=8)
  char timeStr2[4];
  sprintf(timeStr2, "%02d", t.second());
  drawString(8, 11, timeStr2); 

  // Zeile 3: 22.3 *C (x=2)
  char tempStr[15];
  int tempInt = (int)currentTemp;
  int tempDec = abs((int)(currentTemp * 10) % 10);
  sprintf(tempStr, "%d.%d *C", tempInt, tempDec);
  drawString(2, 24, tempStr); 
}

#endif
