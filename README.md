# Digital Stopwatch – ESP32‑C3

**Project 1 – EEL7030 Microprocessors (2025.1) – Universidade Federal de Santa Catarina**

A digital stopwatch that shows the elapsed time and the last lap on a 16 × 2 LCD, controlled by four tactile buttons with external interrupts on an ESP32‑C3‑Mini‑1.

## Functional Requirements

| Requirement     | Description                                                                                            |
| --------------- | ------------------------------------------------------------------------------------------------------ |
| Time display    | First LCD line shows total time in `Tempo MM:SS.mmm`; second line shows last lap in `Volta MM:SS.mmm`. |
| Controls        | **B1** Start, **B2** Pause, **B3** Lap, **B4** Reset (only when paused).                               |
| Timing core     | Uses a dedicated hardware timer; no software counters.                                                 |
| Button debounce | Implemented with the `esp_timer` software timers.                                                      |
| Code layout     | All project code contained in a single `.c` file (except the provided LCD component).                  |
| Documentation   | Source documented with **Doxygen**.                                                                    |

## Hardware

| Component | Details                                  |
| --------- | ---------------------------------------- |
| MCU       | ESP32‑C3‑Mini‑1                          |
| Display   | 16 × 2 alphanumeric LCD, 4‑bit interface |
| Inputs    | 4 × tactile push‑buttons (B1…B4)         |
| Power     | 5 V supply with on‑board 3 V3 regulator  |
| Schematic | See `docs/schematic.pdf`                 |

## Building & Flashing

```bash
idf.py set-target esp32c3
idf.py menuconfig      # optional: adjust pin mapping
idf.py build
idf.py flash monitor
```

*Requires ESP‑IDF ≥ v5.1 and Python ≥ 3.8.*

## Code Documentation

Generate the HTML documentation with:

```bash
doxygen Doxyfile
```

The output will be placed under `docs/html`.

## Useful Links

* Course simulator: [https://wokwi.com/projects/431322677455346689](https://wokwi.com/projects/431322677455346689)
* `esp_timer` API reference
* LCD1602 datasheet

## License

This project is released under the MIT License. See `LICENSE` for details.

