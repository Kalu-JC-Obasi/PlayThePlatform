/*
 * PlayThePlatform.ino
 * by Kalu Obasi
 *
 * This is a script for a music visualization that controls addressable LEDs
 * using signals from a MIDI controller. The Arduino must be used in connection
 * with the Hairless MIDI to Serial Bridge software and, for ideal auditory input,
 * a virtual MIDI port. A full description of this system's functionality should be
 * available in a readme in the same folder as this script, as should a collection
 * of demonstration videos.
 *
 * This system works best with MIDI keyboard controllers. It was developed with
 * the Arturia KeyStep 37, and the pad controls work with controllers that have
 * pressure pads, such as the Arturia MiniLab 3. Many of the variables in this code
 * have been hardcoded for these controllers. Other users may need to edit these
 * variables to get the system to work with their own keyboard controllers.
 *
 * Revised in 2026: restored the pad controls described in the thesis (switching
 * between chromatic and pentatonic coloration, and toggling velocity-sensitive
 * brightness), made brightness individual to each key, and fixed bugs in the
 * note range and in redrawing the LEDs.
 */

#include <FastLED.h>

// User Settings
#define DATAPIN 6 // Data pin connected to LEDs.
#define NUMLEDS 176 // number of LED lights (two per key)
#define BRIGHTNESS 255 // default brightness setting
#define BAUDRATE 115200 // For Hairless
#define NOTESTART 21 // MIDI note of the lowest key shown on the panel (A0 on an 88-key piano)
#define BRIGHTNESS_KNOB 77 // the potentiometer that controls brightness
#define SUSTAIN_PEDAL 64 // the usual signal value for MIDI sustain pedals
#define PAD_CHANNEL 10 // MIDI channel the pads send on (the MiniLab 3 default); keys must use a different channel
#define COLOR_PAD 36 // the pad that switches between chromatic and pentatonic coloration
#define VELOCITY_PAD 37 // the pad that toggles velocity-sensitive brightness

// User Settings
static const uint8_t
  Pin = DATAPIN,
  NumLEDs = NUMLEDS,
  NumKeys = NumLEDs / 2,
  minNote = NOTESTART,
  maxNote = minNote + NumKeys - 1,
  maxBright = BRIGHTNESS;

// The array of LEDs, and the state (on or off) and velocity of each key.
CRGB leds[NumLEDs];
boolean ledState[NumKeys];
uint8_t keyVelocity[NumKeys];
int brightness = maxBright;
bool velocitySensitive = true; // Toggled with VELOCITY_PAD.

// Three different arrays of colors that are mapped to keys on the MIDI controller.
// colorsChromatic maps colors to the chromatic musical scale.
// colorsPentatonic maps colors to the pentatonic musical scale.
// colorsWhite maps each key to the same shade of white.
uint32_t colorsChromatic[12] = {0xFFFF00, 0x990066, 0x3F9B0B, 0xE42217, 0x462D6F, 0xFF9F00,
                  0x702963, 0x6CC417, 0xCE2029, 0x016162, 0xFF6600, 0x062A78};

uint32_t colorsPentatonic[12] = {0xFFFF00, 0x6CC417, 0x3F9B0B, 0x016162, 0x062A78, 0x462D6F,
                  0x702963, 0x990066, 0xCE2029, 0xE42217, 0xFF6600, 0xFF9F00};

uint32_t colorsWhite[12] = {0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF,
                  0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF};

uint32_t* colors = colorsChromatic; // The color array in use. Toggled with COLOR_PAD.


// Sustain pedal functionality.
int lastPedalValue = -1;
unsigned long lastPedalTime = 0;
const int debounceDelay = 50; // ms
bool sustainActive = false;
bool keyPressed[NumKeys] = {false};    // Tracks key state.
bool sustainedNote[NumKeys] = {false}; // Tracks sustained notes.

// Initialize the MIDI Instance.
#include <MIDI.h>
struct CustomBaud : public midi::DefaultSettings {
  static const long BaudRate = BAUDRATE;
};
MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, Serial, MIDI, CustomBaud);

void setup() {

  // Initialize the strip of LEDs using the FastLED library.
  FastLED.addLeds<WS2812B, Pin, GRB>(leds, NumLEDs);
  FastLED.setBrightness(brightness);
  FastLED.show();

  // Set functions for note on, note off, and control change MIDI signals.
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.setHandleControlChange(handleControlChange);
  Serial.begin(BAUDRATE);
}

void loop() {
  MIDI.read(); // Check for MIDI messages every loop.
}

void handleNoteOn(byte channel, byte note, byte velocity) {
  if (channel == PAD_CHANNEL) { // Pads change settings instead of lighting LEDs.
    handlePad(note);
    return;
  }
  if (note < minNote || note > maxNote) return;
  uint8_t i = note - minNote;
  keyPressed[i] = true;
  sustainedNote[i] = false; // Clear any prior sustained notes.
  keyVelocity[i] = velocity;
  ledON(i);
  show();
}

void handleNoteOff(byte channel, byte note, byte velocity) {
  if (channel == PAD_CHANNEL) return;
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

// Pad handling functions.
void handlePad(byte note) {
  if (note == COLOR_PAD) {
    // Switch between the chromatic and pentatonic coloration algorithms.
    colors = (colors == colorsChromatic) ? colorsPentatonic : colorsChromatic;
    redraw();
  } else if (note == VELOCITY_PAD) {
    // Turn velocity-sensitive brightness on or off. When it is off, brightness
    // is controlled only by the potentiometer.
    velocitySensitive = !velocitySensitive;
    redraw();
  }
}

void handleControlChange(byte channel, byte control, byte value) {
  unsigned long now = millis();

  if (control == BRIGHTNESS_KNOB) {
    brightness = map(value, 0, 127, 0, 255); // Map values on the potentiometer to brightness values for the LEDs.
    FastLED.setBrightness(brightness); // Set the LED brightness to the value sent by the potentiometer.
    show();
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
      for (int i = 0; i < NumKeys; i++) {
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
  ledState[index] = 1;
  setLED(index);
}

// LED off message handling.
void ledOFF(uint8_t index) {
  ledState[index] = 0;
  setLED(index);
}

void setLED(uint8_t index) {
  int new_index = index * 2; // Each key controls two adjacent LEDs.
  int colorIndex = (index + minNote) % 12; // The musical note (C = 0) of the key.

  if (ledState[index] == 1) { // If the state of the LED is "on"...
    CRGB color = colors[colorIndex];
    if (velocitySensitive) {
      color.nscale8(keyVelocity[index] * 2); // Dim the color of softer key presses.
    }
    leds[new_index] = color; // ... turn on two LEDs.
    leds[new_index + 1] = color;
  } else { // If the state is "off"...
    leds[new_index] = 0x000000; // ... turn off the LEDs.
    leds[new_index + 1] = 0x000000;
  }
}

void redraw() {
  for (int i = 0; i < NumKeys; i++) {
    setLED(i);
  }
  show();
}

void show() {
  FastLED.show();
}
