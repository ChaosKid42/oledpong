#include <EEPROM.h>
#include <U8g2lib.h>
#include <SPI.h>
#include "STK8BA58.h"

U8G2_SH1106_128X64_NONAME_F_4W_HW_SPI u8g2(U8G2_R0, 10, 8, 9);
STK8BA58 sensor;

#define MIN_INPUT -2
#define MAX_INPUT 2

#define BAT_HEIGHT 4
#define BAT_WIDTH 32
#define BAT_DELTA 10

#define BALL_RAD 4

#define ALPHA 0.5

#define PAGE_DELAY 50
#define SPEED 3
#define SPEEDINC 5

#define HS_ADDR 0

#define HIGH_TONE 440
#define LOW_TONE 220
#define TONE_DUR 25

#define KEY_CENTER 0
#define SPEAKER 15

int score = 0;
float val = 0;
int dpWidth, dpHeight, vx, vy, ballx, bally, xBat;

float map_float(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Matthias Busse 5.2014
void eepromWriteInt(int adr, int wert)
{
  byte low, high;
  low = wert & 0xFF;
  high = (wert >> 8) & 0xFF;
  EEPROM.write(adr, low);
  EEPROM.write(adr + 1, high);
  EEPROM.commit();
  return;
}

// Matthias Busse 5.2014
int eepromReadInt(int adr)
{
  byte low, high;
  low = EEPROM.read(adr);
  high = EEPROM.read(adr + 1);
  return low + ((high << 8) & 0xFF00);

  return 0;
}

void setup(void) {
  EEPROM.begin(256);

  if (eepromReadInt(HS_ADDR) >= 65500)
  {
    eepromWriteInt(HS_ADDR, 0);
  }

  u8g2.begin();
  u8g2.enableUTF8Print();
  u8g2.setContrast(31);
  u8g2.clearBuffer();
  u8g2.sendBuffer();

  dpWidth = u8g2.getDisplayWidth();
  dpHeight = u8g2.getDisplayHeight();

  pinMode(KEY_CENTER, INPUT_PULLUP);
  pinMode(SPEAKER, OUTPUT);

  title_screen();

  sensor.init();
  u8g2.setFont(u8g2_font_ncenB14_tr);
  randomSeed(millis());
  init_ball();
}


void loop(void) {
  u8g2.firstPage();

  val = (1 - ALPHA) * val + ALPHA * -1 * sensor.yAcc();
  xBat = int(map_float(constrain(val, MIN_INPUT, MAX_INPUT), MIN_INPUT, MAX_INPUT, 0, dpWidth-BAT_WIDTH));

  do {
    u8g2.setCursor(0, 15);
    u8g2.print(score);
    u8g2.print(" / ");
    u8g2.print(eepromReadInt(HS_ADDR));

    u8g2.drawDisc(ballx, bally, BALL_RAD);

    u8g2.drawBox(xBat, dpHeight-BAT_HEIGHT, BAT_WIDTH, BAT_HEIGHT);
  } while( u8g2.nextPage() );

  ballx += vx;
  bally += vy;
  bounce();

  delay(PAGE_DELAY);
}

void bounce(void) {
  if (bally >= dpHeight-BALL_RAD-BAT_HEIGHT-1) {
    if ( (ballx+BALL_RAD >= xBat) && (ballx-BALL_RAD <= xBat + BAT_WIDTH)) {
      bally = dpHeight-BALL_RAD-BAT_HEIGHT-1;
      vy *= -1;
      score += 1;
      tone(SPEAKER, LOW_TONE, TONE_DUR);
      if (score > eepromReadInt(HS_ADDR)) {
        eepromWriteInt(HS_ADDR, score);
      }
      if (score % SPEEDINC == 0) {
        if (vy > 0)
          vy++;
        else
          vy--;
        if (vx > 0)
          vx++;
        else
          vx--;
      }
    } else {
      score = 0;
      init_ball();
      return;
    }
  }
  if (bally <= BALL_RAD) {
    bally = BALL_RAD;
    vy *= -1;
    tone(SPEAKER, HIGH_TONE, TONE_DUR);
  }
  if (ballx >= dpWidth-BALL_RAD-1) {
    ballx = dpWidth-BALL_RAD-1;
    vx *= -1;
    tone(SPEAKER, HIGH_TONE, TONE_DUR);
  }
  if (ballx <= BALL_RAD) {
    ballx = BALL_RAD;
    vx *= -1;
    tone(SPEAKER, HIGH_TONE, TONE_DUR);
  }

}

void init_ball(void) {
  if (random(2))
    vx = SPEED;
   else
    vx = -SPEED;
  vy = SPEED;
  ballx = random(BALL_RAD, dpWidth-BALL_RAD);
  bally = BALL_RAD;
}

void title_screen(void) {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_helvB08_tf);
    u8g2.setCursor(0, 14);
    u8g2.print("OLED Pong");
    u8g2.setCursor(0, 28);
    u8g2.print("© 2023 by");
    u8g2.setCursor(0, 42);
    u8g2.print("Christoph Scholz");
    u8g2.setCursor(0, 56);
    u8g2.print("Push center to start");
  } while( u8g2.nextPage() );
  while (digitalRead(KEY_CENTER) == HIGH) {}
}
