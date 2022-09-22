//------------------------LED EFFECT FUNCTIONS------------------------

void rainbow_fade() {                         //-m2-FADE ALL LEDS THROUGH HSV RAINBOW
  ihue++;
  if (ihue > 255) {
    ihue = 0;
  }
  for (int idex = 0 ; idex < LED_COUNT; idex++ ) {
    leds[idex] = CHSV(ihue, thissat, 255);
  }
  LEDS.show();
  delay(thisdelay);
}

void rainbow_loop() {                        //-m3-LOOP HSV RAINBOW
  fadeOut(1);
  idex++;
  ihue = ihue + thisstep;
  if (idex >= LED_COUNT) {
    idex = 0;
  }
  if (ihue > 255) {
    ihue = 0;
  }
  leds[idex] = CHSV(ihue, thissat, 255);
  LEDS.show();
  delay(thisdelay);
}

void random_burst() {                         //-m4-RANDOM INDEX/COLOR
  if (bouncedirection == 0) {
     bouncedirection = 1;
     setAll(4,4,4);
  }
  idex = random(0, LED_COUNT);
  ihue = random(0, 255);
  leds[idex] = leds[idex].r + leds[idex].g + leds[idex].b <= 12 // (4+4+4)
    ? CHSV(ihue, 255, 255)
    : CRGB(4, 4, 4);
  LEDS.show();
  delay(thisdelay);
}

void color_bounce() {                        //-m5-BOUNCE COLOR (SINGLE LED)
  int num_led = LED_COUNT / thisseg;
  
  if (bouncedirection == 0) {
    idex++;
    if (idex == num_led) {
      bouncedirection = 1;
    }
  }
  
  if (bouncedirection == 1) {
    idex--;
    if (idex == 0) {
      bouncedirection = 0;
      thishue = getRandomHueColor();
    }
  }

  int shift = 0;
  for (int sg = 0; sg < thisseg; sg++ ) {
    for (int i = 0; i < num_led; i++ ) {
      leds[i+shift] = i == idex
        ? CHSV(thishue, thissat, 255) 
        : CRGB(8, 8, 8);
    }
    shift += num_led;
  }
  
  LEDS.show();
  delay(thisdelay);
}

void color_bounceFADE() {                    //-m6-BOUNCE COLOR (SIMPLE MULTI-LED FADE)
  int num_led = LED_COUNT / thisseg;

  if (bouncedirection == 0) {
    idex++;
    if (idex == num_led) {
      bouncedirection = 1;
    }
  }
  if (bouncedirection == 1) {
    idex--;
    if (idex == 0) {
      bouncedirection = 0;
      thishue = getRandomHueColor();
    }
  }
  int iL1 = adjacent_cw(idex);
  int iL2 = adjacent_cw(iL1);
  int iL3 = adjacent_cw(iL2);
  int iR1 = adjacent_ccw(idex);
  int iR2 = adjacent_ccw(iR1);
  int iR3 = adjacent_ccw(iR2);
  
  int shift = 0;
  for (int sg = 0; sg < thisseg; sg++ ) {
    for (int i = 0; i < num_led; i++ ) {
      if (i == idex) {
        leds[i+shift] = CHSV(thishue, thissat, 255);
      }
      else if (i == iL1) {
        leds[i+shift] = CHSV(thishue, thissat, 150);
      }
      else if (i == iL2) {
        leds[i+shift] = CHSV(thishue, thissat, 80);
      }
      else if (i == iL3) {
        leds[i+shift] = CHSV(thishue, thissat, 20);
      }
      else if (i == iR1) {
        leds[i+shift] = CHSV(thishue, thissat, 150);
      }
      else if (i == iR2) {
        leds[i+shift] = CHSV(thishue, thissat, 80);
      }
      else if (i == iR3) {
        leds[i+shift] = CHSV(thishue, thissat, 20);
      }
      else {
        leds[i+shift] = CRGB(8, 8, 8);
      }
    }
    shift += num_led;
  }
  LEDS.show();
  delay(thisdelay);
}

void ems_lightsONE() {                    //-m7-EMERGENCY LIGHTS (TWO COLOR SINGLE LED)
  int num_led = LED_COUNT / thisseg;

  idex++;
  if (idex >= num_led) {
    idex = 0;
    thishue = getRandomHueColor(); 
  }
  
  int idexR = idex;
  int idexB = antipodal_index_seg(idexR, thisseg);
  int thathue = (thishue + 160) % 255;
  
  int shift = 0;
  for (int sg = 0; sg < thisseg; sg++ ) {
    for (int i = 0; i < num_led; i++ ) {
      if (i == idexR) {
        leds[i+shift] = CHSV(thishue, thissat, 255);
      }
      else if (i == idexB) {
        leds[i+shift] = CHSV(thathue, thissat, 255);
      }
      else {
        leds[i+shift] = CRGB(8, 8, 8);
      }
    }
    shift += num_led;
  }
    
  LEDS.show();
  delay(thisdelay);
}

void ems_lightsALL() {                  //-m8-EMERGENCY LIGHTS (TWO COLOR SOLID)
  int num_led = LED_COUNT / thisseg;

  idex++;
  if (idex >= num_led) {
    idex = 0;
    thishue = getRandomHueColor();
  }
  int idexR = idex;
  int idexB = antipodal_index_seg(idexR, thisseg);
  int thathue = (thishue + 160) % 255;
  
  int shift = 0;
  for (int sg = 0; sg < thisseg; sg++ ) {
    leds[idexR+shift] = CHSV(thishue, thissat, 255);
    leds[idexB+shift] = CHSV(thathue, thissat, 255);
    shift += num_led;
  }  

  LEDS.show();
  delay(thisdelay);
}

void pulse_one_color_all() {              //-m10-PULSE BRIGHTNESS ON ALL LEDS TO ONE COLOR
  if (bouncedirection == 0) {
    ibright++;
    if (ibright >= 255) {
      bouncedirection = 1;
    }
  }
  if (bouncedirection == 1) {
    ibright--;
    if (ibright <= 1) {
      bouncedirection = 0;
      thishue = getRandomHueColor();
    }
  }
  for (int idex = 0 ; idex < LED_COUNT; idex++ ) {
    leds[idex] = CHSV(thishue, thissat, ibright); 
  }
  LEDS.show();
  delay(thisdelay);
}

void pulse_one_color_all_rev() {           //-m11-PULSE SATURATION ON ALL LEDS TO ONE COLOR
  if (bouncedirection == 0) {
    isat++;
    if (isat >= 255) {
      bouncedirection = 1;
    }
  }
  if (bouncedirection == 1) {
    isat = isat - 1;
    if (isat <= 1) {
      bouncedirection = 0;
      thishue = getRandomHueColor();
    }
  }
  for (int idex = 0 ; idex < LED_COUNT; idex++ ) {
    leds[idex] = CHSV(thishue, isat, 255);
  }
  LEDS.show();
  delay(thisdelay);
}

void random_march() {                   //-m14-RANDOM MARCH CCW
  copy_led_array();
  int iCCW;
  leds[0] = CHSV(random8(), 255, 255);
  for (int idex = 1; idex < LED_COUNT ; idex++ ) {
    iCCW = adjacent_ccw(idex);
    leds[idex] = ledsX[iCCW];
  }
  LEDS.show();
  delay(thisdelay);
}

void rwb_march(byte len) {                    //-m15-R,B,W MARCH CCW
  if (len > 10) len = 10;

  int sIdx = idex;
  int sBnc = bouncedirection;

  for (int i = 0; i < LED_COUNT; i++) {

    if (idex >= len) {
      idex = 0;        
      bouncedirection++;
      if (bouncedirection > 3) { // индекc цвета red, blue, white, black
        bouncedirection = 0;
      }
    }
    
    switch (bouncedirection) {
      case 0:
          leds[i].setRGB(255,0,0);
        break;
      case 1:
          leds[i].setRGB(0,0,255);
        break;
      case 2:
          leds[i].setRGB(255,255,255);
        break;
      case 3:
          leds[i].setRGB(0,0,0);
        break;
    }
    
    idex++;
  }

  idex = sIdx + 1;  
  bouncedirection = sBnc;

  if (idex >= len) {
    idex = 0;        
    bouncedirection++;
    if (bouncedirection > 3) { // индекc цвета red, blue, white, black
      bouncedirection = 0;
    }
  }

  LEDS.show();
  delay(thisdelay);
}

void color_loop_vardelay() {                    //-m17-COLOR LOOP (SINGLE LED) w/ VARIABLE DELAY
  int num_led = int(LED_COUNT / thisseg);
  int top_index = int(num_led / 2);

  idex++;
  if (idex >= num_led) {
    idex = 0;
    thishue = getRandomHueColor();
  }
  int di = abs(top_index - idex);
  if (di == 0) di = 1;
  int t = constrain((10 / di) * 10, 10, 500);
  
  int shift = 0;
  for (int sg = 0; sg < thisseg; sg++ ) {
    for (int i = 0; i < num_led; i++ ) {
      if (i == idex) {
        leds[i+shift] = CHSV(thishue, thissat, 255);
      }
      else {
        leds[i+shift] = CRGB(8, 8, 8);
      }
    }
    shift += num_led;
  }  
  
  LEDS.show();
  delay(t);
}

void sin_bright_wave() {        //-m19-BRIGHTNESS SINE WAVE
  fadeOut(1); 
  
  tcount = tcount + .1;
  if (tcount > 3.14) {
    tcount = 0.0;
  }

  ibright = int(sin(tcount) * 255);
  leds[lcount] = CHSV(thishue, thissat, ibright);
  LEDS.show();
  delay(thisdelay);
  
  lcount++;
  if (lcount >= LED_COUNT) {
    lcount = 0;
    thishue += 15;
    if (thishue >= 255) thishue = thishue - 255; 
  }
}

void pop_horizontal(bool cycle) {        //-m20-POP FROM LEFT TO RIGHT UP THE RING
  int num_led = int(LED_COUNT / thisseg);
  int top_index = int(num_led / 2);
  
  int ix;
  if (bouncedirection == 0) {
    bouncedirection = 1;
    ix = idex;
  }
  else if (bouncedirection == 1) {
    if (!cycle) thishue = getRandomHueColor();
    bouncedirection = 0;
    ix = horizontal_index_seg(idex, thisseg);
    idex++;
    if (idex > top_index) {
      if (cycle) thishue = getRandomHueColor();
      idex = 0;
    }
  }
  
  int shift = 0;
  for (int sg = 0; sg < thisseg; sg++ ) {
    
    for (int i = 0; i < num_led; i++ ) {
      if (i == ix) {
        leds[i+shift] = CHSV(thishue, thissat, 255);
      }
      else {
        leds[i+shift].setRGB(8, 8, 8);
      }
    }

    shift += num_led;
  }  

  LEDS.show();
  delay(thisdelay);
}

void flame() {                                    //-m22-FLAMEISH EFFECT
  int num_led = int(LED_COUNT / thisseg);
  int top_index = int(num_led / 2);
  
  int idelay = random(0, 35);
  float hmin = 0.1; float hmax = 20.0;
  float hdif = hmax - hmin;
  int randtemp = random(0, 3);
  float hinc = (hdif / float(top_index)) + randtemp;
  int ihue = hmin;
  
  for (int i = 0; i <= top_index; i++ ) {

    ihue = ihue + hinc;
    int ih = horizontal_index_seg(i, thisseg);
    
    int shift = 0;
    for (int sg = 0; sg < thisseg; sg++ ) {
      leds[i+shift] = CHSV(ihue, thissat, 255);
      leds[ih+shift] = CHSV(ihue, thissat, 255);
      leds[top_index+shift].setRGB(16,16,0);
  
      shift += num_led;
    }  

    LEDS.show();
    delay(idelay);
  }
}

void fadeOut(uint8_t step) {
  fadeToBlackBy(leds, LED_COUNT, step);
}

void rainbow_vertical() {                        //-m23-RAINBOW 'UP' THE LOOP
  fadeOut(1);  
  idex++;
  if (idex > TOP_INDEX) {
    idex = 0;
  }
  ihue = ihue + thisstep;
  if (ihue > 255) {
    ihue = 0;
  }
  int idexA = idex;
  int idexB = horizontal_index(idexA);
  leds[idexA] = CHSV(ihue, thissat, 255);
  leds[idexB] = CHSV(ihue, thissat, 255);
  LEDS.show();
  delay(thisdelay);
}

void random_color_pop() {                         //-m25-RANDOM COLOR POP
  int num_led = int(LED_COUNT / thisseg);
  idex = random(0, num_led);
  ihue = random8();
  setAll(8, 8, 8);

  int shift = 0;
  for (int sg = 0; sg < thisseg; sg++ ) {
    leds[idex+shift] = CHSV(ihue, thissat, 255);
    shift += num_led;
  }  
  
  LEDS.show();
  delay(thisdelay);
}

void rgb_propeller() {                           //-m27-RGB PROPELLER
  int num_led = int(LED_COUNT / thisseg);
  idex++;
  int ghue = (thishue + 80) % 255;
  int bhue = (thishue + 160) % 255;
  int N3  = int(num_led / 3);
  int N6  = int(num_led / 6);
  int N12 = int(num_led / 12);
  int shift = 0;
  for (int sg = 0; sg < thisseg; sg++ ) {
    for (int i = 0; i < N3; i++ ) {
      int j0 = (idex + i + num_led - N12) % num_led;
      int j1 = (j0 + N3) % num_led;
      int j2 = (j1 + N3) % num_led;
      leds[j0+shift] = CHSV(thishue, thissat, 255);
      leds[j1+shift] = CHSV(ghue, thissat, 255);
      leds[j2+shift] = CHSV(bhue, thissat, 255);
    }
    shift += num_led;
  }  
  LEDS.show();
  delay(thisdelay);
}

void matrix(int Delay) {                                   //-m29-ONE LINE MATRIX
  if (bouncedirection == 0) {
    setAll(8,8,8);
    bouncedirection = 1;
  }
  if (millis() - last_time > Delay) {
    last_time = millis();
    thishue = getRandomHueColor();
  }
  int rand = random(0, 100);
  if (rand > 90) {
    leds[0] = CHSV(thishue, thissat, 255);
  }
  else {
    leds[0] = CRGB(8, 8, 8);
  }
  copy_led_array();
  for (int i = 1; i < LED_COUNT; i++ ) {
    leds[i] = ledsX[i - 1];
  }
  LEDS.show();
  delay(thisdelay);
}

void new_rainbow_loop() {                      //-m88-RAINBOW FADE FROM FAST_SPI2
  ihue -= 1;
  fill_rainbow( leds, LED_COUNT, ihue );
  LEDS.show();
  delay(thisdelay);
}

//----- плавное заполнение цветом, случайный цвет после каждого круга -----
void ColorWipeInOut() {
  int num_led = int(LED_COUNT / thisseg);

  int shift = 0;  
  for (int sg = 0; sg < thisseg; sg++ ) {
    int thathue = (thishue + sg * int(255 / thisseg)) % 255;
    if (bouncedirection == 0) 
      setPixelHSV(idex + shift, thathue, 255, 255);    
    else
      setPixel(idex + shift, 0x00, 0x00, 0x00);
    shift += num_led;
  }  

  LEDS.show();
  delay(thisdelay);

  idex++;
  if (idex >= num_led)
  {
    idex = 0;    
    bouncedirection = bouncedirection == 0 ? 1 : 0;
    if (bouncedirection == 0) {
      thishue = getRandomHueColor();
    }
  }
}

//---------------------------------линейный огонь-------------------------------------
void Fire(int Cooling, int Sparking) {
  
  int num_led = int(LED_COUNT / thisseg);
  static byte heat[LED_COUNT];
  int cooldown;

  int shift = 0;
  for (int sg = 0; sg < thisseg; sg++ ) {

    // Step 1.  Cool down every cell a little
    for ( int i = 0; i < num_led; i++) {
      cooldown = random(0, ((Cooling * 10) / num_led) + 2);
  
      if (cooldown > heat[i]) {
        heat[i+shift] = 0;
      } else {
        heat[i+shift] -= cooldown;
      }
    }
  
    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for ( int k = num_led - 1; k >= 2; k--) {
      heat[k+shift] = (heat[k+shift - 1] + heat[k+shift - 2] + heat[k+shift - 2]) / 3;
    }
  
    // Step 3.  Randomly ignite new 'sparks' near the bottom
    if ( random8() < Sparking ) {
      int y = random(7);
      heat[y+shift] = heat[y+shift] + random(160, 255);
    }
  
    // Step 4.  Convert heat to LED colors
    for ( int j = 0; j < num_led; j++) {
      setPixelHeatColor(j+shift, heat[j+shift] );
    }
    
    shift += num_led;
  }  

  LEDS.show();
  delay(thisdelay);
}

void setPixelHeatColor (int Pixel, byte temperature) {
  // Scale 'heat' down from 0-255 to 0-191
  byte t192 = round((temperature / 255.0) * 191);

  // calculate ramp up from
  byte heatramp = t192 & 0x3F; // 0..63
  heatramp <<= 2; // scale up to 0..252

  // figure out which third of the spectrum we're in:
  if ( t192 > 0x80) {                    // hottest
    setPixel(Pixel, 255, 255, heatramp);
  } else if ( t192 > 0x40 ) {            // middle
    setPixel(Pixel, 255, heatramp, 0);
  } else {                               // coolest
    setPixel(Pixel, heatramp, 0, 0);
  }
}

void rainbowCycle() {
  byte *c;
  
  for (uint16_t i = 0; i < LED_COUNT; i++) {
    c = Wheel(((i * 256 / LED_COUNT) + lcount) & 255);
    setPixel(i, *c, *(c + 1), *(c + 2));
  }
  
  LEDS.show();
  delay(thisdelay);    
  
  lcount++;
  if (lcount >= 256 * 5) lcount = 0;
}

byte* Wheel(byte WheelPos) {
  static byte c[3];

  if (WheelPos < 85) {
    c[0] = WheelPos * 3;
    c[1] = 255 - WheelPos * 3;
    c[2] = 0;
  } else if (WheelPos < 170) {
    WheelPos -= 85;
    c[0] = 255 - WheelPos * 3;
    c[1] = 0;
    c[2] = WheelPos * 3;
  } else {
    WheelPos -= 170;
    c[0] = 0;
    c[1] = WheelPos * 3;
    c[2] = 255 - WheelPos * 3;
  }

  return c;
}

//-------------------------------RunningLights---------------------------------------
void RunningLights() {
  thishue++;
  if (thishue > 255) thishue = 0;
  
  for (int i = 0; i < LED_COUNT; i++) {
    float level = (sin(i + lcount) * 127 + 128) / 255;
    CHSV hsv(thishue, 255, 255); // pure blue in HSV Rainbow space
    CRGB rgb;
    hsv2rgb_rainbow(hsv, rgb);
    setPixel(i, level * rgb.r, level * rgb.g, level * rgb.b);
  }

  LEDS.show();
  delay(thisdelay);

  lcount++;
  if (lcount >= LED_COUNT * 2) lcount = 0;
}

//-------------------------------Sparkle---------------------------------------
void Sparkle(byte red, byte green, byte blue) {
  if (bouncedirection == 0) {
    setAll(8,8,8);
    bouncedirection = 1;
  }
  int Pixel = random(LED_COUNT);
  setPixel(Pixel, red, green, blue);
  LEDS.show();
  delay(thisdelay);
  setPixel(Pixel, 8, 8, 8);
}

//-------------------------------SparkleRandom---------------------------------------
void SparkleRandom(int ColorChangeDelay) {
  if (bouncedirection == 0) {
    setAll(8,8,8);
    bouncedirection = 1;
  }
  if (millis() - last_time > ColorChangeDelay) {
    last_time = millis();
    thishue = getRandomHueColor();
  }
  int Pixel = random(LED_COUNT);
  setPixelHSV(Pixel, thishue, 255, 255);
  LEDS.show();
  delay(thisdelay);
  setPixel(Pixel, 8, 8, 8);
}

//-------------------------------SparkleRandomEvery---------------------------------------
void SparkleRandomEvery() {
  if (bouncedirection == 0) {
    setAll(8,8,8);
    bouncedirection = 1;
  }
  thishue = getRandomHueColor();
  int Pixel = random(LED_COUNT);
  setPixelHSV(Pixel, thishue, 255, 255);
  LEDS.show();
  delay(thisdelay);
  setPixel(Pixel, 8, 8, 8);
}

//-------------------------------TheaterChase---------------------------------------
void TheaterChase(byte red, byte green, byte blue) {
    for (int q = 0; q < 3; q++) {
      for (int i = 0; i < LED_COUNT; i = i + 3) {
        setPixel(i + q, red, green, blue);  //turn every third pixel on
      }
      LEDS.show();
      delay(thisdelay);
      for (int i = 0; i < LED_COUNT; i = i + 3) {
        setPixel(i + q, 0, 0, 0);    //turn every third pixel off
      }
    }
}

//-------------------------------TheaterChase---------------------------------------
void TheaterChaseRandom(int ColorChangeDelay) {
  if (millis() - last_time > ColorChangeDelay) {
    last_time = millis();
    thishue = getRandomHueColor();
  }

  for (int q = 0; q < 3; q++) {
    for (int i = 0; i < LED_COUNT; i = i + 3) {
      setPixelHSV(i + q, thishue, 255, 255);  //turn every third pixel on
    }
    LEDS.show();
    delay(thisdelay);
    for (int i = 0; i < LED_COUNT; i = i + 3) {
      setPixel(i + q, 0, 0, 0);    //turn every third pixel off
    }
  }
}

//-------------------------------TheaterChaseRainbow---------------------------------------
void TheaterChaseRainbow() {
  byte *c;

  for (int q = 0; q < 3; q++) {
    for (int i = 0; i < LED_COUNT; i = i + 3) {
      c = Wheel( (i + lcount) % 255);
      setPixel(i + q, *c, *(c + 1), *(c + 2)); //turn every third pixel on
    }
    LEDS.show();
    delay(thisdelay);
    for (int i = 0; i < LED_COUNT; i = i + 3) {
      setPixel(i + q, 0, 0, 0);    //turn every third pixel off
    }
  }

  lcount++;
  if (lcount >= 256) lcount = 0;
}

//------------------------------ Strobe White Color --------------------------------------
void StrobeWhite(int StrobeCount, int FlashDelay, int EndPause) {
  if (millis() - last_time < EndPause) return;
  
  for (int j = 0; j < StrobeCount; j++) {
    setAll(42, 42, 42); // 255,255,255 - слишком ярко
    LEDS.show();
    delay(FlashDelay);
    setAll(4, 4, 4);
    LEDS.show();
    delay(FlashDelay);
  }
  last_time = millis();
}

//------------------------------ Strobe Random Color --------------------------------------
void StrobeRandom(int StrobeCount, int FlashDelay, int EndPause) {
  if (millis() - last_time < EndPause) return;

  ihue = getRandomHueColor();  
  for (int j = 0; j < StrobeCount; j++) {
    setAllHSV(ihue, thissat, 127);
    LEDS.show();
    delay(FlashDelay);
    setAll(4, 4, 4);
    LEDS.show();
    delay(FlashDelay);
  }
  last_time = millis();
}

//------------------------------ Halloween Eyes --------------------------------------
void HalloweenEyes(int EyeWidth, int EyeSpace, 
                   boolean Fade, int Steps, int FadeDelay,
                   int EndPause){  

  if (millis() - last_time < EndPause) return;                    

  if (bouncedirection == 0) {
    bouncedirection = 1;
    setAll(4,4,4);
  }
  int num_led = LED_COUNT / thisseg;

  int i;
  int StartPoint  = random( 0, num_led - (2*EyeWidth) - EyeSpace );
  int Start2ndEye = StartPoint + EyeWidth + EyeSpace;
  byte hue = getRandomHueColor(); 
  
  int shift = 0;
  for (int sg = 0; sg < thisseg; sg++ ) {
    for(i = 0; i < EyeWidth; i++) {
      setPixelHSV(StartPoint + shift + i, hue, 255, 255);
      setPixelHSV(Start2ndEye + shift + i, hue, 255, 255);
    }
    shift += num_led;
  }  

  LEDS.show();
  
  if (Fade) {
    int bright = 255; 
    int oneStep = 255 / Steps;
    while (bright > 16) {      
      bright -= oneStep; 
      if (bright < 16) bright = 16;

      shift = 0;
      for (int sg = 0; sg < thisseg; sg++ ) {
        for(i = 0; i < EyeWidth; i++) {
          setPixelHSV(StartPoint + shift + i, hue, 255, bright);
          setPixelHSV(Start2ndEye + shift + i, hue, 255, bright);
        }
        shift += num_led;
      }  
      
      LEDS.show();
      delay(FadeDelay);
    }
  }
  
  setAll(4,4,4); // Set all black
  LEDS.show();
  
  last_time = millis();  
}

//------------------------------ NewKITT --------------------------------------
void NewKITT(int EyeSize, int SpeedDelay, int ReturnDelay){
  
  if (lcount == 0) thishue = getRandomHueColor();
  
  switch (lcount) {
    case 0: RightToLeft(thishue, EyeSize, SpeedDelay, ReturnDelay); break;
    case 1: LeftToRight(thishue, EyeSize, SpeedDelay, ReturnDelay); break;
    case 2: OutsideToCenter(thishue, EyeSize, SpeedDelay, ReturnDelay); break;
    case 3: CenterToOutside(thishue, EyeSize, SpeedDelay, ReturnDelay); break;
    case 4: LeftToRight(thishue, EyeSize, SpeedDelay, ReturnDelay); break;
    case 5: RightToLeft(thishue, EyeSize, SpeedDelay, ReturnDelay); break;
    case 6: OutsideToCenter(thishue, EyeSize, SpeedDelay, ReturnDelay); break;
    case 7: CenterToOutside(thishue, EyeSize, SpeedDelay, ReturnDelay); break;
  }

  lcount++;
  if (lcount == 8) lcount = 0;
}

void CenterToOutside(byte hue, int EyeSize, int SpeedDelay, int ReturnDelay) {
  for(int i =((LED_COUNT-EyeSize)/2); i>=0; i--) {
    setAll(8,8,8);
    
    setPixelHSV(i, hue, 255, 25);
    for(int j = 1; j <= EyeSize; j++) {
      setPixelHSV(i+j, hue, 255, 255); 
    }
    setPixelHSV(i+EyeSize+1, hue, 255, 25);
    
    setPixelHSV(LED_COUNT-i, hue, 255, 25);
    for(int j = 1; j <= EyeSize; j++) {
      setPixelHSV(LED_COUNT-i-j, hue, 255, 255); 
    }
    setPixelHSV(LED_COUNT-i-EyeSize-1, hue, 255, 25);
    
    LEDS.show();
    delay(SpeedDelay);
  }
  delay(ReturnDelay);
}

void OutsideToCenter(byte hue, int EyeSize, int SpeedDelay, int ReturnDelay) {
  for(int i = 0; i<=((LED_COUNT-EyeSize)/2); i++) {
    setAll(8,8,8);
    
    setPixelHSV(i, hue, 255, 25);
    for(int j = 1; j <= EyeSize; j++) {
      setPixelHSV(i+j, hue, 255, 255); 
    }
    setPixelHSV(i+EyeSize+1, hue, 255, 25);
    
    setPixelHSV(LED_COUNT-i, hue, 255, 25);
    for(int j = 1; j <= EyeSize; j++) {
      setPixelHSV(LED_COUNT-i-j, hue, 255, 255); 
    }
    setPixelHSV(LED_COUNT-i-EyeSize-1, hue, 255, 25);
    
    LEDS.show();
    delay(SpeedDelay);
  }
  delay(ReturnDelay);
}

void LeftToRight(byte hue, int EyeSize, int SpeedDelay, int ReturnDelay) {
  for(int i = 0; i < LED_COUNT-EyeSize-2; i++) {
    setAll(8,8,8);
    setPixelHSV(i, hue, 255, 25);
    for(int j = 1; j <= EyeSize; j++) {
      setPixelHSV(i+j, hue, 255, 255); 
    }
    setPixelHSV(i+EyeSize+1, hue, 255, 25);
    LEDS.show();
    delay(SpeedDelay);
  }
  delay(ReturnDelay);
}

void RightToLeft(byte hue, int EyeSize, int SpeedDelay, int ReturnDelay) {
  for(int i = LED_COUNT-EyeSize-2; i > 0; i--) {
    setAll(8,8,8);
    setPixelHSV(i, hue, 255, 25);
    for(int j = 1; j <= EyeSize; j++) {
      setPixelHSV(i+j, hue, 255, 255); 
    }
    setPixelHSV(i+EyeSize+1, hue, 255, 25);
    LEDS.show();
    delay(SpeedDelay);
  }
  delay(ReturnDelay);
}

//------------------------------ Twinkle --------------------------------------
void Twinkle(int Count) {
  int num_led = int(LED_COUNT / thisseg);

  if (lcount == 0) {
    setAll(0,0,0);
    thishue = getRandomHueColor();
  }
    
  int shift = 0;
  for (int sg = 0; sg < thisseg; sg++ ) {
    setPixelHSV(random(num_led) + shift, thishue, 255, 255);
    shift += num_led;
  }
  
  LEDS.show();
  delay(thisdelay);
  
  lcount++;
  if (lcount == Count) {
    lcount = 0;
    delay(thisdelay);
  }
}

//--------------------------- Twinkle Random -----------------------------------
void TwinkleRandom(int Count) {
  int num_led = int(LED_COUNT / thisseg);
  
  if (lcount == 0) setAll(0,0,0);
  
  int shift = 0;
  for (int sg = 0; sg < thisseg; sg++ ) {
    thishue = getRandomHueColor();
    setPixelHSV(random(num_led)+shift, thishue, 255, 255);
    shift += num_led;
  }

  LEDS.show();
  delay(thisdelay);

  lcount++;
  if (lcount == Count) {
    lcount = 0;
    delay(thisdelay);
  }
}

//--------------------------- MeteorRain -----------------------------------
void meteorRain(byte meteorSize, byte meteorTrailDecay, boolean meteorRandomDecay) {  
  int num_led = int(LED_COUNT / thisseg);
  
  if (lcount == 0) setAll(0,0,0);

  byte red = 0xff, green = 0xff, blue = 0xff;
  int shift = 0;
  for (int sg = 0; sg < thisseg; sg++ ) {
  
    // fade brightness all LEDs one step
    for(int j=0; j < num_led; j++) {
      if( (!meteorRandomDecay) || (random(10) > 5) ) {
        leds[j+shift].fadeToBlackBy( meteorTrailDecay * thisseg );
      }
    }
    
    // draw meteor
    for(int j = 0; j < meteorSize; j++) {
      if( (lcount-j < num_led) && (lcount-j>=0) ) {
        setPixel(lcount-j+shift, red, green, blue);
      } 
    }

    shift += num_led;
  }  
 
  LEDS.show();
  delay(6 * thisseg);

  lcount++;
  if (lcount >= num_led + num_led) lcount = 0;
}

//-------------------- Four Colors Light ----------------------------
void FourColorsLight() {

  bright[0] += increment[0] * thisstep; 
  if (bright[0] > 255){
    bright[0] = 255;
    increment[0] = -1;    
  }
  if (bright[0] < -255){
    bright[0] = -255;
    increment[0] = 1;    
  }

  bright[1] += increment[1] * thisstep; 
  if (bright[1] > 255){
    bright[1] = 255;
    increment[1] = -1;    
  }
  if (bright[1] < -255){
    bright[1] = -255;
    increment[1] = 1;    
  }

  bright[2] += increment[2] * thisstep; 
  if (bright[2] > 255){
    bright[2] = 255;
    increment[2] = -1;    
  }
  if (bright[2] < -255){
    bright[2] = -255;
    increment[2] = 1;    
  }

  bright[3] += increment[3] * thisstep; 
  if (bright[3] > 255){
    bright[3] = 255;
    increment[3] = -1;    
  }
  if (bright[3] < -255){
    bright[3] = -255;
    increment[3] = 1;    
  }
  
  for (int i=0; i < LED_COUNT; i++) {
    byte idx = i % 4;
    byte brt = bright[idx] < 0 ? 0 : (bright[idx] > 255 ? 255 : bright[idx]);
    leds[i] = CHSV(colors[idx], 255, brt);
  }
  
  LEDS.show();
  delay(thisdelay);
}
