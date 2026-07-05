# ESP32-P4 Spectrum Analyzer

> **A professional real-time audio measurement instrument for the
> ESP32-P4 Function EV Board.**

> **Status:** Stable Public Release -- **v1.0.0**

------------------------------------------------------------------------

> **📷 Hero image placeholder**
>
> Replace this section with a full-width photograph of the analyzer
> running on the ESP32-P4 Function EV Board.

------------------------------------------------------------------------

## Why this project exists

The ESP32-P4 is a remarkably capable embedded platform, yet most audio
examples stop at demonstrating individual peripherals or basic FFT
processing.

This project takes a different approach.

The goal is to build a **complete embedded audio measurement
instrument** that feels like a real piece of laboratory equipment rather
than a technology demonstration.

It combines modern embedded graphics, DSP, USB Audio Class support,
persistent configuration, touchscreen interaction and web-based
configuration into a single standalone application.

Although it began as a personal engineering project, it is released as
open source so that others can learn from it, improve it and build on
it.

------------------------------------------------------------------------

# Highlights

-   Real-time FFT analysis (512--16384 point)
-   Multiple window functions
-   Multiple averaging modes
-   Spectrum, Waterfall, Oscilloscope, Mirror, VU and 1/3 Octave
    displays
-   USB Audio Class (UAC1) support
-   ES8311 onboard audio support
-   Runtime USB stereo-to-mono selection
-   Touchscreen pinch zoom
-   Microphone calibration support
-   Noise-floor capture and subtraction
-   Presets with full runtime persistence
-   SD card configuration storage
-   Wi-Fi provisioning
-   Embedded web interface
-   PlatformIO and ESP-IDF compatible

------------------------------------------------------------------------

# What makes this project different?

Many embedded FFT projects answer the question:

> "Can an ESP32 perform an FFT?"

This project asks a different question:

> **"How capable can an ESP32-P4 become as a standalone audio
> measurement instrument?"**

Every feature is evaluated against one goal:

**Does it make the instrument more useful?**

------------------------------------------------------------------------

# Feature Summary

  Capability                           Status
  ----------------------------- ---------------------
  FFT Analyzer                           ✅
  Oscilloscope                           ✅
  Waterfall Display                      ✅
  1/3 Octave Analyzer                    ✅
  USB Audio                              ✅
  ES8311 Audio                           ✅
  Touch Gestures                         ✅
  Web Interface                          ✅
  Presets                                ✅
  Calibration                            ✅
  Noise Floor Capture                    ✅
  Wi-Fi Provisioning                     ✅
  Distributed Stereo Analyzer    🚧 Planned for v2.0

------------------------------------------------------------------------

# Quick Start

## Hardware

-   ESP32-P4 Function EV Board
-   USB-C cable
-   Optional USB UAC1 audio interface (for example Behringer UCA222)
-   Optional calibrated USB measurement microphone
-   microSD card

## Build

``` bash
pio run
pio run -t upload
```

or

``` bash
idf.py build
idf.py flash
```

------------------------------------------------------------------------

# Supported Audio Sources

## On-board ES8311

Ideal for development and testing.

## USB Audio Class (UAC1)

Supports external USB audio interfaces.

Runtime options:

-   Average L+R
-   Left only
-   Right only

No recompilation required.

------------------------------------------------------------------------

# Display Modes

-   Spectrum
-   Waterfall
-   Oscilloscope
-   Mirror
-   VU Meter
-   1/3 Octave

Most analyzer views support two-finger pinch zoom for frequency span and
display range.

------------------------------------------------------------------------

# Software Architecture

``` text
Audio Source
      │
      ▼
Input Processing
      │
      ▼
DSP Engine
      │
      ├── FFT
      ├── Averaging
      ├── Noise Floor
      ├── Calibration
      │
      ▼
Display Engine
      │
      ├── LCD
      └── Web API
```

The firmware is intentionally organized into independent components
including audio capture, DSP, networking, settings management and
display rendering. This keeps the codebase maintainable while making
future extensions easier.

------------------------------------------------------------------------

# Documentation

  Document            Purpose
  ------------------- ---------------------------
  User Guide          Operating the instrument
  Hardware Guide      Hardware connections
  Architecture        Internal design
  DSP Guide           Signal processing details
  Calibration Guide   Microphone calibration
  Changelog           Version history

------------------------------------------------------------------------

# Roadmap

## Version 1.0

✔ Standalone audio measurement instrument

## Version 2.0

**Distributed Stereo Analyzer**

Two ESP32-P4 analyzers operating as one coordinated instrument.

Planned capabilities include:

-   Primary / Secondary operating modes
-   Stereo channel split
-   Low-latency network PCM transport
-   Automatic preset synchronization
-   Network pairing
-   Shared configuration
-   Synchronized displays

## Future

Potential future work includes:

-   Transfer-function measurements
-   Impulse response
-   THD measurements
-   Measurement logging
-   CSV export
-   Remote browser clients

------------------------------------------------------------------------

# Contributing

Bug reports, feature suggestions and pull requests are welcome.

The emphasis of this project is on practical engineering, clear
documentation and reliable behaviour.

------------------------------------------------------------------------

## Project Background

This project began as a personal engineering challenge while I was undergoing treatment for acute myeloid leukemia (AML). During an extended hospital stay I wanted to continue learning, solving problems, and designing embedded systems.

Engineering has always been one of the ways I make sense of complex problems, and this project became an opportunity to keep learning while facing a very different kind of challenge.

What started as an exploration of the ESP32-P4 and real-time DSP gradually evolved into a much more capable audio measurement instrument. Every new feature was added with the same goal in mind: to build something genuinely useful rather than simply demonstrate a particular technology.

I am releasing the project as open source in the hope that other engineers, students, makers, and audio enthusiasts will find it useful, learn from it, and perhaps extend it in directions I never imagined.
------------------------------------------------------------------------

# License

Apache 2.0
