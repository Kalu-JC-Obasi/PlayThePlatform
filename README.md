# PlayThePlatform

**A real-time, interactive music visualizer:** play a MIDI keyboard and a 176-LED panel shows what you're playing. Pitch, loudness, sustain-pedal use and key all become light.

Built with Arduino, the FastLED and MIDI libraries, and WS2812B LEDs as my senior thesis in Computer Science at Princeton University (2024–25), advised by Dr. Radhika Nagpal and Dr. Elizabeth Margulis.

![The full system: a MIDI keyboard, a laptop running the MIDI routing software, an Arduino, and the LED panel](docs/media/system.jpg)

▶️ **[Demo videos](https://tinyurl.com/3t4pz9st)** · 📄 **[Thesis (PDF)](docs/Obasi_Senior_Thesis_2025.pdf)**

---

## Why

Most music visualizers react passively to a recording: the person watching doesn't make the music. PlayThePlatform puts the player in control. Every key press, every change in how hard a key is struck, and every press of the sustain pedal shows up on the panel the instant it happens. It's designed to be obvious to beginners and still interesting to experienced musicians.

## What it does

| You do this | The panel does this |
|---|---|
| Press a key | Two LEDs light up. Higher notes sit closer to the centre of the spiral layout. |
| Play several keys at once | Chords and fast passages turn into colour patterns. |
| Press a key harder or softer | That key's lights get brighter or dimmer, so loudness becomes visible. Each key keeps its own brightness. |
| Hold the sustain pedal | Lights stay on after you let go of the key, until you lift the pedal. |
| Turn the brightness knob | Sets the overall panel brightness. |
| Play notes with different pitch classes | Each of the 12 notes has its own colour on a 12-tone colour wheel. |
| Tap pad 1 | Switch between the chromatic and pentatonic colour mappings. |
| Tap pad 2 | Turn velocity-sensitive brightness on or off. When it's off, only the knob controls brightness. |

## How it works

```
MIDI keyboard ──USB──▶ Laptop ──USB serial──▶ Arduino Uno R4 ──data pin 6──▶ 176 × WS2812B LEDs
                          │
                          ├─ Hairless MIDI↔Serial Bridge  (sends MIDI to the Arduino at 115200 baud)
                          └─ loopMIDI virtual port ──▶ any synth or DAW (for sound)
```

- **Arduino sketch (`PlayThePlatform.ino`):** handles MIDI Note On, Note Off and Control Change messages. It maps each piano key (MIDI notes 21–108) to a pair of LEDs, scales each key's colour by how hard it was struck, keeps track of sustained notes so they release correctly when the pedal comes up, and treats notes on the pad channel as settings controls.
- **Getting the timing right:** the first prototype ran on an Arduino Uno R3. As features were added, it lagged noticeably and crashed when many keys were pressed at once. I tested the Mega 2560 (more memory, same clock speed: fewer crashes, same lag) and the Due, then moved to the **Uno R4**. Its faster clock and extra memory got rid of the lag and dropped signals I'd been seeing.
- **Panel design:** an earlier version wove a single LED strip into a 3-D shape. Users often couldn't see the LEDs they were triggering, so I cut the strip into sections and rebuilt it as a flat spiral panel, which keeps every light visible.

## Hardware

- Arduino Uno R4 (Minima or WiFi). Other Arduino boards should work too.
- 176 × WS2812B addressable RGB LEDs, data line on pin 6
- External 5 V power supply or battery for the LEDs. A full panel at full brightness draws far more current than the Arduino can supply.
- A MIDI keyboard controller with velocity-sensitive keys. A sustain pedal, a brightness knob and pressure pads are optional.

Developed with the **Arturia KeyStep 37**, which supports everything except the pad controls. The pad controls work with controllers that have pressure pads, such as the **Arturia MiniLab 3**. Default settings: brightness knob on CC 77, and pads on MIDI channel 10 sending notes 36 and 37.

## Software and setup

1. Install the [Arduino IDE](https://www.arduino.cc/en/software) and the **FastLED** and **MIDI Library** libraries (Library Manager).
2. Open `PlayThePlatform.ino`, check the settings at the top (`DATAPIN`, `NUMLEDS`, `BRIGHTNESS_KNOB`), and upload it to the board.
3. Install [Hairless MIDI↔Serial Bridge](https://projectgus.github.io/hairless-midiserial/). Set the serial port to your Arduino, the baud rate to **115200**, MIDI In to your controller, and MIDI Out to the virtual port from step 4.
4. For sound, create a virtual MIDI port with [loopMIDI](https://www.tobias-erichsen.de/software/loopmidi.html) (Windows) or the IAC Driver in Audio MIDI Setup (macOS), and point any synth at it. I used the Chrome Music Lab's piano.
5. Play.

**Using a different controller?** Change `BRIGHTNESS_KNOB`, `PAD_CHANNEL`, `COLOR_PAD` and `VELOCITY_PAD` at the top of the sketch to match what your controller sends. Hairless's debug view shows incoming messages, which makes them easy to find.

## Revision history

- **2025 (thesis version):** the system as described in the thesis.
- **2026 cleanup:** restored the pad controls described in the thesis (colour-mapping switch and velocity toggle); made brightness individual to each key, so a soft note no longer dims every other lit key; ignored notes outside the 88-key range instead of writing past the end of the LED array; fixed the redraw after a settings change; and stored colours as 32-bit values so they display correctly on 8-bit boards like the Uno R3.

Note: the thesis says per-LED brightness is impossible with WS2812B LEDs. It is possible: the limit came from using FastLED's single global brightness setting, and the 2026 version scales each LED's colour instead.

## Future work

- Visual modes designed for non-keyboard controllers such as pad grids and knob banks.
- Better ways of showing musical key, including telling major keys from minor ones.
- A study with users at different levels of musical experience. An earlier prototype was shown publicly at the Princeton University Makerspace in May 2024.

## Background

This project grew out of an earlier independent-work prototype, *Harmonizing Sound and Light: MIDI-Enabled LED Visualization System* (spring 2024). The full design rationale, related work and evaluation are in the [thesis](docs/Obasi_Senior_Thesis_2025.pdf).

## License

MIT. See [LICENSE](LICENSE).

