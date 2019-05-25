#include "Adafruit_Arcada.h"
#include "Adafruit_QSPI.h"

#include "audio.h"

Adafruit_Arcada arcada;
Adafruit_QSPI_Flash qspi_flash;

uint32_t buttons, last_buttons;
uint8_t j = 0;  // neopixel counter for rainbow


volatile uint16_t milliseconds = 0;
void timercallback() {
  if (milliseconds == 0) {
    Serial.println("tick");
    milliseconds = 1000;
  } else {
    milliseconds--;
  }
}

void setup() {
  while (!Serial);

  Serial.println("Hello! Arcada PyBadge test");
  if (!arcada.begin()) {
    Serial.print("Failed to begin");
  }
  arcada.displayBegin();
  Serial.println("Arcada display begin");

  for (int i=0; i<250; i++) {
    arcada.setBacklight(i);
    delay(1);
  }
  arcada.fillScreen(ARCADA_RED);
  delay(100);
  arcada.fillScreen(ARCADA_GREEN);
  delay(100);
  arcada.fillScreen(ARCADA_BLUE);
  delay(100);
  arcada.fillScreen(ARCADA_BLACK);
  delay(100);

  arcada.setCursor(0, 0);
  arcada.setTextWrap(true);

  /********** Check QSPI manually */
  if (!qspi_flash.begin()){
    Serial.println("Could not find flash on QSPI bus!");
    arcada.setTextColor(ARCADA_RED);
    arcada.println("QSPI Flash FAIL");
  }
  uint8_t manid, devid;
  Serial.println("Reading Manuf ID");
  qspi_flash.GetManufacturerInfo(&manid, &devid);
  Serial.print("JEDEC ID: 0x"); Serial.println(qspi_flash.GetJEDECID(), HEX);
  Serial.print("Manuf: 0x"); Serial.print(manid, HEX);
  Serial.print(" Device: 0x"); Serial.println(devid, HEX);
  arcada.setTextColor(ARCADA_GREEN);
  arcada.print("QSPI Flash OK\nManuf: 0x"); arcada.print(manid, HEX);
  arcada.print(" Dev: 0x"); arcada.println(devid, HEX);

   /********** Check filesystem next */
  if (!arcada.filesysBegin()) {
    Serial.println("Failed to load filesys");
    arcada.setTextColor(ARCADA_RED);
    arcada.println("Filesystem failure");
  } else {
    Serial.println("Filesys OK!");
    arcada.setTextColor(ARCADA_GREEN);
    arcada.print("Filesystem OK: ");
    arcada.print(arcada.filesysListFiles("/"));
    arcada.println(" files");
  }

  /********** Check LIS3DH */
  if (!arcada.hasAccel()) {
    Serial.println("No accelerometer found");
    arcada.setTextColor(ARCADA_YELLOW);
    arcada.println("Accelerometer not found");
  } else {
    Serial.println("Accelerometer OK!");
    arcada.setTextColor(ARCADA_GREEN);
    arcada.println("Accelerometer OK!");
    arcada.accel.setRange(LIS3DH_RANGE_4_G);   // 2, 4, 8 or 16 G!
    arcada.accel.setClick(1, 80);
  }

  //arcada.writeFileToFlash("test.txt");
  buttons = last_buttons = 0;

  arcada.timerCallback(1000, timercallback);
}


void loop() {
  delay(25);  // add some delay so our screen doesnt flicker
  
  if (arcada.hasAccel()) {
    uint8_t click = arcada.accel.getClick();
    if (click & 0x30) {
      Serial.print("Click detected (0x"); Serial.print(click, HEX); Serial.print("): ");
      if (click & 0x10) Serial.print(" single click");
      if (click & 0x20) Serial.print(" double click");
      arcada.enableSpeaker(true);
      play_tune(audio, sizeof(audio));
      arcada.enableSpeaker(false);
    }
  
    sensors_event_t event; 
    arcada.accel.getEvent(&event);
    
    /* Display the results (acceleration is measured in m/s^2) */
    Serial.print("Accel: \t X: "); Serial.print(event.acceleration.x);
    Serial.print(" \tY: "); Serial.print(event.acceleration.y); 
    Serial.print(" \tZ: "); Serial.print(event.acceleration.z); 
    Serial.println(" m/s^2 ");
    arcada.fillRect(0, 40, 160, 8, ARCADA_BLACK);
    arcada.setTextColor(ARCADA_WHITE);
    arcada.setCursor(0, 40);
    arcada.print("X:"); arcada.print(event.acceleration.x, 1);
    arcada.setCursor(50, 40);
    arcada.print("Y:"); arcada.print(event.acceleration.y, 1);
    arcada.setCursor(100, 40);
    arcada.print("Z:"); arcada.print(event.acceleration.z, 1);
  }

  // Read light sensor
  Serial.print("Light: "); Serial.println(arcada.readLightSensor());
  arcada.fillRect(0, 50, 160, 8, ARCADA_BLACK);
  arcada.setCursor(0, 50);
  arcada.setTextColor(ARCADA_WHITE);
  arcada.print("Light: "); arcada.print(arcada.readLightSensor());

  // Read battery
  arcada.setCursor(80, 50);
  arcada.setTextColor(ARCADA_WHITE);
  float vbat = arcada.readBatterySensor();
  Serial.print("Battery: "); Serial.print(vbat); Serial.println("V");
  arcada.print("Batt: "); arcada.print(vbat); arcada.println("V");

  // Read the 2 Stemma connectors (these are not in ARCADA because they're external hardware)
  arcada.fillRect(0, 60, 160, 16, ARCADA_BLACK);
  arcada.setCursor(0, 60);
  arcada.setTextColor(ARCADA_BLUE);
  float vsense = analogRead(A8)*3.3/1024;
  Serial.print("D2/A8: "); Serial.print(vsense); Serial.println("V");
  arcada.print("D2/A8: "); arcada.print(vsense); arcada.println("V");
  vsense = analogRead(A9)*3.3/1024;
  arcada.setCursor(80, 60);
  Serial.print("D3/A9: "); Serial.print(vsense); Serial.println("V");
  arcada.print("D3/A9: "); arcada.print(vsense); arcada.println("V");

  Serial.printf("Drawing %d NeoPixels", arcada.pixels.numPixels());  
  arcada.pixels.setBrightness(255);
  for(int32_t i=0; i< arcada.pixels.numPixels(); i++) {
     arcada.pixels.setPixelColor(i, Wheel(((i * 256 / arcada.pixels.numPixels()) + j*5) & 255));
  }
  arcada.pixels.show();
  j++;

  uint8_t pressed_buttons = arcada.readButtons();
  arcada.fillRect(0, 70, 160, 60, ARCADA_BLACK);
  
  if (pressed_buttons & ARCADA_BUTTONMASK_A) {
    Serial.print("A");
    arcada.drawCircle(145, 100, 10, ARCADA_WHITE);
  }
  if (pressed_buttons & ARCADA_BUTTONMASK_B) {
    Serial.print("B");
    arcada.drawCircle(120, 100, 10, ARCADA_WHITE);
  }
  if (pressed_buttons & ARCADA_BUTTONMASK_SELECT) {
    Serial.print("Sel");
    arcada.drawRoundRect(60, 100, 20, 10, 5, ARCADA_WHITE);
  }
  if (pressed_buttons & ARCADA_BUTTONMASK_START) {
    Serial.print("Sta");
    arcada.drawRoundRect(85, 100, 20, 10, 5, ARCADA_WHITE);
  }
  if (pressed_buttons & ARCADA_BUTTONMASK_UP) {  
    Serial.print("^");
    arcada.drawTriangle(15, 86, 35, 86, 25, 76, ARCADA_WHITE);
  }
  if (pressed_buttons & ARCADA_BUTTONMASK_DOWN) {
    Serial.print("v");
    arcada.drawTriangle(15, 113, 35, 113, 25, 123, ARCADA_WHITE);
  }
  if (pressed_buttons & ARCADA_BUTTONMASK_LEFT) {
    Serial.print("<");
    arcada.drawTriangle(10, 90, 10, 110, 0, 100, ARCADA_WHITE);
  }
  if (pressed_buttons & ARCADA_BUTTONMASK_RIGHT) {
    arcada.drawTriangle(40, 90, 40, 110, 50, 100, ARCADA_WHITE);
    Serial.print(">");
  }
  Serial.println();

  last_buttons = buttons;
}


// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return arcada.pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return arcada.pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return arcada.pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

void play_tune(const uint8_t *audio, uint32_t audio_length) {
  uint32_t t;
  uint32_t prior, usec = 1000000L / SAMPLE_RATE;
  analogWriteResolution(8);
  for (uint32_t i=0; i<audio_length; i++) {
    while((t = micros()) - prior < usec);
    analogWrite(A0, (uint16_t)audio[i] / 8);
    analogWrite(A1, (uint16_t)audio[i] / 8);
    prior = t;
  }
}


/*****************************************************************/
