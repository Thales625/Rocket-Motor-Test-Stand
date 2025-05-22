# Rocket Motor Test Stand

Firmware for the solid rocket motor test stand developed by the UFPel Rocket Team.

## Overview

This firmware is responsible for reading the load cell (via HX711), acquiring and logging thrust data during static tests of solid rocket motors.

## Features

- HX711 load cell interface
- Real-time thrust measurements
- Serial data output
- Designed for ESP32 (ESP-IDF)

## Third-Party Code and Licenses

This project includes third-party code under the BSD 3-Clause License:

- `hx711.c` `hx711.h` — © 2019 Ruslan V. Uss (<unclerus@gmail.com>)

All other code is licensed under the MIT License. See the [LICENSE](./LICENSE) file for more information.

## License

This project is primarily licensed under the MIT License. See the LICENSE file for details.