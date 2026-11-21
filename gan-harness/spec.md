# SMARTLUX dashboard, design brief

## Product
A local dashboard for SMARTLUX, a lamp controller built by Kelompok 2 at SMAN CMBBS. The ESP32 serves the page over the school WiFi. One reader: a student or teacher standing in the room with a phone.

## What the screen must answer
1. Is the lamp on, and why.
2. How bright the room is.
3. How much energy the system has used and how much it saved against a lamp left on.
4. How to take manual control.

## Constraints
- Indonesian copy. No English UI labels.
- Mobile first, readable at 360 px wide, comfortable at 1440 px.
- No CDN, no external fonts, no network requests beyond the device. The page ships in PROGMEM, so it must stay under 8 KB of HTML, CSS and JS.
- Data fields available: light ADC and percent, rough lux, PIR, lamp state, mode (auto, on, off), voltage, current, power, total energy, lamp energy, savings percent, uptime, IP.
- Actions: set mode, reset energy counters.
- Poll `/api/status` once per second. Show a clear disconnected state.
- Dark theme is the default. Light theme optional.

## Feeling
Calm and exact, the way Linear and Apple Notes feel. The numbers carry the screen. No tagline, no marketing sentence, no decorative labels. If a value needs an explanation, the layout is wrong.

## Success
A reader understands the lamp state in under two seconds, and the energy result in under five. Nothing on the page exists to fill space.
