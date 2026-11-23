'use strict';

// Static / invariant tests for the SMARTLUX firmware sources.
// They check structure and wiring constants only; no compilation happens here.

const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const { execSync } = require('node:child_process');

const ROOT = path.join(__dirname, '..', '..');
const FIRMWARE_DIR = path.join(ROOT, 'firmware', 'SMARTLUX');
const INO = path.join(FIRMWARE_DIR, 'SMARTLUX.ino');
const LOGIC = path.join(FIRMWARE_DIR, 'logic.h');
const SECRETS_EXAMPLE = path.join(FIRMWARE_DIR, 'secrets.example.h');
const SECRETS = path.join(FIRMWARE_DIR, 'secrets.h');

const INDEX_HTML_BUDGET_BYTES = 8192;

function readIno() {
  return fs.readFileSync(INO, 'utf8');
}

function indexHtmlPayload() {
  const m = readIno().match(/R"rawliteral\(([\s\S]*?)\)rawliteral"/);
  assert.ok(m, 'INDEX_HTML R"rawliteral(...)rawliteral payload not found');
  return m[1];
}

function trackedFiles() {
  const out = execSync('git ls-files', { cwd: ROOT, encoding: 'utf8' });
  return out.split('\n').map((s) => s.trim()).filter(Boolean);
}

test('INDEX_HTML dashboard payload fits the 8192 byte PROGMEM budget', () => {
  const html = indexHtmlPayload();
  const size = Buffer.byteLength(html, 'utf8');
  assert.ok(
    size <= INDEX_HTML_BUDGET_BYTES,
    `INDEX_HTML is ${size} bytes, budget is ${INDEX_HTML_BUDGET_BYTES} bytes`
  );
});

test('no JavaScript function line sits at column 0 in the .ino', () => {
  const lines = readIno().split(/\r?\n/);
  const offenders = lines.filter((l) => /^(async\s+)?function\s/.test(l));
  assert.deepEqual(
    offenders,
    [],
    'JS function lines at column 0 are mistaken for C++ prototypes by ino2cpp; indent one space'
  );
});

test('binding pin map is declared in the firmware', () => {
  const src = readIno();
  assert.match(src, /PIN_LDR\s*=\s*34/, 'LDR ADC pin (GPIO34) must be 34');
  assert.match(src, /PIN_PIR\s*=\s*27/, 'PIR pin (GPIO27) must be 27');
  assert.match(src, /PIN_MOSFET\s*=\s*26/, 'MOSFET gate pin (GPIO26) must be 26');
  assert.match(src, /PIN_SDA\s*=\s*21/, 'I2C SDA pin (GPIO21) must be 21');
  assert.match(src, /PIN_SCL\s*=\s*22/, 'I2C SCL pin (GPIO22) must be 22');
  assert.match(src, /PIN_BTN\s*=\s*32/, 'override button pin (GPIO32) must be 32');
});

test('dashboard HTTP routes are registered on the server', () => {
  const src = readIno();
  const routes = [
    ...src.matchAll(/server\.on\("([^"]+)", HTTP_GET, (\w+)\);/g),
  ].map((m) => m[1]);
  // The four original routes are the stable public API; the history/export/chart
  // routes were added later for the R4 energy-history feature.
  assert.deepEqual(
    [...routes].sort(),
    [
      '/',
      '/api/export',
      '/api/history',
      '/api/override',
      '/api/reset',
      '/api/status',
      '/chart',
    ],
    'expected the dashboard routes to be registered'
  );
});

test('logic.h stays host-testable (no Arduino-only headers)', () => {
  const logic = fs.readFileSync(LOGIC, 'utf8');
  const arduinoHeaders = [
    'Arduino.h',
    'Wire.h',
    'WiFi.h',
    'WebServer.h',
    'ESPmDNS.h',
    'Adafruit_GFX.h',
    'Adafruit_SSD1306.h',
    'Adafruit_INA219.h',
    'SPI.h',
    'Servo.h',
    'FS.h',
  ];
  for (const h of arduinoHeaders) {
    assert.ok(
      !logic.includes(`#include <${h}>`),
      `logic.h must not include Arduino-only header <${h}>`
    );
  }
});

test('secrets.example.h is tracked and secrets.h is not tracked', () => {
  const tracked = trackedFiles();
  assert.ok(
    tracked.includes('firmware/SMARTLUX/secrets.example.h'),
    'firmware/SMARTLUX/secrets.example.h should be tracked'
  );
  assert.ok(
    !tracked.includes('firmware/SMARTLUX/secrets.h'),
    'firmware/SMARTLUX/secrets.h must not be tracked'
  );
});

test('firmware sources exist', () => {
  assert.ok(fs.existsSync(INO), 'firmware/SMARTLUX/SMARTLUX.ino is missing');
  assert.ok(fs.existsSync(LOGIC), 'firmware/SMARTLUX/logic.h is missing');
  assert.ok(fs.existsSync(SECRETS_EXAMPLE), 'firmware/SMARTLUX/secrets.example.h is missing');
});
