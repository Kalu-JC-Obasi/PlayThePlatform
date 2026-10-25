/*
 * PlaythePlatform.ino
 * by Kalu Obasi
 * 
 * This is a script for a music visualization that controls addressable LEDs
 * using signals from a MIDI controller. The Arduino must be used in connection
 * with the Hairless MIDI to Serial Bridge software and, for ideal auditory input,
 * a virtual MIDI port. A full description of this system's functionality should be
 * available in a readme in the same folder as this script, as should a collection
 * of demonstration videos.
 *
 * This system works best with MIDI keyboard controllers. Many of the variables 
 * in this code have been hardcoded so that the system works, with a specific MIDI 
 * controller, the Arturia MiniLab 3. Other users may need to edit these variables 
 * to get the system to work with their own keyboard controllers.
 */

#include <FastLED.h>

// User Settings
#define DATAPIN 6 // Data pin connected to LEDs.
#define NUMLEDS 176 // number of LED lights
#define BRIGHTNESS 255 // default brightness setting
#define BAUDRATE 115200 // For Hairless
#define NOTESTART 0 
#define BRIGHTNESS_KNOB 77 // the potentiometer that controls brightness
#define SUSTAIN_PEDAL 64 // the usual signal value for MIDI sustain pedals

// User Settings
static const uint8_t
  Pin = DATAPIN,
  NumLEDs = NUMLEDS,
  minNote = NOTESTART,
  maxNote = minNote + NumLEDs,
  maxBright = BRIGHTNESS;

// The array of LEDs and their states (on or off).
CRGB leds[NumLEDs];
boolean ledState[NumLEDs];
int brightness = maxBright;

// Three different arrays of colors that are mapped to keys on the MIDI controller.
// colorsChromatic maps colors to the chromatic musical scale.
// colorsPentatonic maps colors to the pentatonic musical scale.
// colorsWhite maps each key to the same shade of white.
int colorsChromatic[12] = {0xFFFF00, 0x990066, 0x3F9B0B, 0xE42217, 0x462D6F, 0xFF9F00,
                  0x702963, 0x6CC417, 0xCE2029, 0x016162, 0xFF6600, 0x062A78};

int colorsPenatonic[12] = {0xFFFF00, 0x6CC417, 0x3F9B0B, 0x016162, 0x062A78, 0x462D6F,
                  0x702963, 0x990066, 0xCE2029, 0xE42217, 0xFF6600, 0xFF9F00};

int colorsWhite[12] = {0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF,
                  0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF};


// Sustain pedal functionality.
int lastPedalValue = -1;
unsigned long lastPedalTime = 0;
const int debounceDelay = 50; // ms
bool sustainActive = false;
bool keyPressed[maxNote - minNote + 1] = {false};    // Tracks key state.
bool sustainedNote[maxNote - minNote + 1] = {false}; // Tracks sustained notes.

// Initialize the MIDI Instance.
#include <MIDI.h>
struct CustomBaud : public midi::DefaultSettings {
  static const long BaudRate = BAUDRATE;
};
MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, Serial, MIDI, CustomBaud);

void setup() {

  // Initialize the strip of LEDs using the FastLED library.
  FastLED.addLeds<WS2812B, Pin, GRB>(leds, NumLEDs);
  FastLED.setBrightness(100);
  FastLED.show();

  // Set functions for note on, note off, and control change MIDI signals.
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.setHandleControlChange(handleControlChange);
  Serial.begin(115200);
}

void loop() {
  MIDI.read(); // Check for MIDI messages every loop.
}

void handleNoteOn(byte channel, byte note, byte velocity) {
  if (note < minNote || note > maxNote) return;
  uint8_t i = note - minNote;
  keyPressed[i] = true;
  sustainedNote[i] = false; // Clear any prior sustained notes.
  FastLED.setBrightness(velocity * 2);
  ledON(i);
  show();
}

void handleNoteOff(byte channel, byte note, byte velocity) {
  if (note < minNote || note > maxNote) return;
  uint8_t i = note - minNote;
  keyPressed[i] = false;

  if (sustainActive) {
    sustainedNote[i] = true; // Defer turning off LED until the sustain pedal is lifted.
  } else {
    ledOFF(i);
    show();
  }
}

void handleControlChange(byte channel, byte control, byte value) {
  unsigned long now = millis();

  if (control == BRIGHTNESS_KNOB) {
    brightness = map(value, 0, 127, 0, 255); // Map values on the potentiometer to brightness values for the LEDs.
    FastLED.setBrightness(brightness); // Set the LED brightness to the value sent by the potentiometer.
    redraw();
  }

  // Sustain pedal handling functions.
  if (control == SUSTAIN_PEDAL && value != lastPedalValue && now - lastPedalTime > debounceDelay) {
    lastPedalValue = value;
    lastPedalTime = now;

    if (value >= 64) {
      // Sustain pedal pressed.
      sustainActive = true;
    } else {
      // Sustain pedal released.
      sustainActive = false;
      for (int i = 0; i <= maxNote - minNote; i++) {
        if (!keyPressed[i] && sustainedNote[i]) {
          sustainedNote[i] = false;
          ledOFF(i);
        }
      }
      show();
    }
  }
}

// LED on message handling.
void ledON(uint8_t index) {
  int new_index = (index - 21) * 2;
  ledState[new_index] = 1;
  setLED(new_index, index);
}

// LED off message handling.
void ledOFF(uint8_t index) {
  int new_index = (index - 21) * 2;
  ledState[new_index] = 0;
  setLED(new_index, index);
}

void setLED(uint8_t new_index, uint8_t index) {
  int colorIndex = index % 12;

  if (ledState[new_index] == 1) { // If the state of the LED is "on"...
    leds[new_index] = colorsChromatic[colorIndex]; // ... turn on two LEDs.
    leds[new_index + 1] = colorsChromatic[colorIndex];
  } else { // If the state is "off"...
    leds[new_index] = 0x000000; // ... turn off the LEDs.
    leds[new_index + 1] = 0x000000;
  }
}

void redraw() {
  for (int i = 0; i < NumLEDs; i++) {
    if (ledState[i] == 1) {
      ledON(i);
    }
  }
  show();
}

void show() {
  FastLED.show();
}
