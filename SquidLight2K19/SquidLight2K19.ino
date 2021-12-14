#include <Arduino.h>
#include <Esp.h>
#include <FastLED.h>

FASTLED_USING_NAMESPACE

// Based heavily on FastLED '100 line sof code' demo reel,
// by Mark Kriegsman, December 2014.
//
// Lightly hacked by Jonathan Sanderson, December 2019

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

// CONFIG LED STRIPS
#define DATA_PIN_A      D1
#define DATA_PIN_B      D3
#define LED_TYPE        WS2811
#define COLOR_ORDER     GRB
#define NUM_LEDS_A      50
#define NUM_LEDS_B      49 // I suck at soldering to NeoPixel strips

CRGB ledsA[NUM_LEDS_A];
CRGB ledsB[NUM_LEDS_B];

#define BRIGHTNESS          255
#define FRAMES_PER_SECOND   120

// CONFIG INPUTS
#define DIALPIN         A0
#define BUTTONPIN       D5
int buttonState = LOW;
int dialValue = 0;

// We're going to debounce the button
int lastButtonState = LOW;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 100;

// offset calcs
unsigned long lastHueOffsetTime = 0;
int gHueOffsetRate = 20;

void setup() {
    Serial.begin(115200);
    delay(2000);    // 2 second delay to complete bootup

    // Instantiate LED strip
    FastLED.addLeds<LED_TYPE,DATA_PIN_A,COLOR_ORDER>(ledsA, NUM_LEDS_A).setCorrection(TypicalLEDStrip);
    FastLED.addLeds<LED_TYPE,DATA_PIN_B,COLOR_ORDER>(ledsB, NUM_LEDS_B).setCorrection(TypicalLEDStrip);

    // Set master brightness control
    FastLED.setBrightness(BRIGHTNESS);

    pinMode(BUTTONPIN, INPUT_PULLUP);
    pinMode(DIALPIN, INPUT);
}

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { rainbow, rainbowWithGlitter, confetti, sinelon, juggle, bpm };

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns

void loop()
{
  // Call the current pattern function once, updating the 'leds' array
  gPatterns[gCurrentPatternNumber]();

  // send the 'leds' array out to the actual LED strip
  FastLED.show();
  // insert a delay to keep the framerate modest
  FastLED.delay(1000/FRAMES_PER_SECOND);

  // Update hue angle rate from the dial
  dialValue = analogRead(DIALPIN);
  gHueOffsetRate = map(dialValue, 0, 1024, 1, 100);
  EVERY_N_SECONDS( 1 ) { Serial.println(gHueOffsetRate); }

  // We've lots of things running on manual timers, so call this once for the loop:
  unsigned long timeNow = millis();

  // Cycle the colours, if it's time
  // EVERY_N_MILLISECONDS( gHueOffsetRate ) { gHue++; } // Cycle base hue at rate determined by dial
  if ( (timeNow - lastHueOffsetTime) > gHueOffsetRate ) {
    gHue++;
    // Reset the lastHueOffsetTime
    lastHueOffsetTime = timeNow;
  }

  // Switch pattern on button press
  // Check if button has been pressed
  // Using code from Arduino docs here: https://www.arduino.cc/en/tutorial/debounce
  int reading = digitalRead(BUTTONPIN);
  if (reading != lastButtonState) {
    Serial.println("Button pressed!");
    // Reset the debounce timer
    lastDebounceTime = timeNow;
  }

  if ((timeNow - lastDebounceTime) > debounceDelay) {
    // Whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as actual current state:
    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == LOW) {
        Serial.println("Next pattern!");
        nextPattern();
      }
    }
  }
  lastButtonState = reading;

  //   EVERY_N_SECONDS( 10 ) { nextPattern(); } // change patterns periodically
}

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void nextPattern()
{
  // add one to the current pattern number, and wrap around at the end
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns);
}

void rainbow()
{
  // FastLED's built-in rainbow generator
  fill_rainbow( ledsA, NUM_LEDS_A, gHue, 7);
  fill_rainbow( ledsB, NUM_LEDS_B, gHue, 7);
}

void rainbowWithGlitter()
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}

void addGlitter( fract8 chanceOfGlitter)
{
  if( random8() < chanceOfGlitter) {
    ledsA[ random16(NUM_LEDS_A) ] += CRGB::White;
    ledsB[ random16(NUM_LEDS_B) ] += CRGB::White;
  }
}

void confetti()
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( ledsA, NUM_LEDS_A, 10);
  int pos = random16(NUM_LEDS_A);
  ledsA[pos] += CHSV( gHue + random8(64), 200, 255);
  fadeToBlackBy( ledsB, NUM_LEDS_B, 10);
  pos = random16(NUM_LEDS_B);
  ledsB[pos] += CHSV( gHue + random8(64), 200, 255);
}

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( ledsA, NUM_LEDS_A, 20);
  int pos = beatsin16( 13, 0, NUM_LEDS_A-1 );
  ledsA[pos] += CHSV( gHue, 255, 192);
  fadeToBlackBy( ledsB, NUM_LEDS_B, 20);
  pos = beatsin16( 13, 0, NUM_LEDS_B-1 );
  ledsB[pos] += CHSV( gHue, 255, 192);
}

void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS_A; i++) { //9948
    ledsA[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }
  for( int i = 0; i < NUM_LEDS_B; i++) { //9948
    ledsB[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }
}

void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( ledsA, NUM_LEDS_A, 20);
  byte dothue = 0;
  for( int i = 0; i < 8; i++) {
    ledsA[beatsin16( i+7, 0, NUM_LEDS_A-1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
  fadeToBlackBy( ledsB, NUM_LEDS_B, 20);
  dothue = 0;
  for( int i = 0; i < 8; i++) {
    ledsB[beatsin16( i+7, 0, NUM_LEDS_B-1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}
