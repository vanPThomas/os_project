# Changelog

All notable changes to this project will be documented in this file.

## [0.1.6] 2026-02-17
### Changed
- Refactoring for more readable code

## [0.1.5] - 2026-02-02 
### Added
- Add uptime display on Option 2 (seconds/minutes since boot)
- Add contrast control submenu (Option 3) with live up/down adjustment

---

## [0.1.4] - 2026-02-01
### Added
- added custom lower case letters

---

## [0.1.3] - 2026-01-31
### Changed
-Basic code cleanup

---

## [0.1.2] - 2026-01-24
### Added
- Added a basic menu controlled with a remote through IR sensor.

---

## [0.1.1] - 2026-01-23
### Changed
- Improved Oled drivers for more compatibility

---

## [0.1.0] - 2026-01-21

Initial public release

- Working OLED text output
- IR button recognition
- Basic boot screen and interaction loop

## [Unreleased]

### Added
- Initial boot animation with "Booting..." dots
- SH1106/SSD1306 compatible OLED driver (I²C, page addressing mode)
- Custom 6×8 bitmap font with basic ASCII + symbols
- Polling-based NEC IR remote decoder (GP15)
- IrRemote class with button enum mapping for common 21-key remotes
- Button press detection → friendly name display on OLED
- Cleaned-up code structure (Oled, Font, IrRemote classes)

### Changed
- Relaxed NEC timing tolerances for better reliability with cheap remotes
- Removed most debug printf() calls from IR decoder

### Fixed
- GPIO pull-up for IR receiver to prevent stuck-low state
- Column offset and addressing for SH1106 displays (+2 column shift)
