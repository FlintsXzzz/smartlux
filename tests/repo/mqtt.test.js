'use strict';

// Static invariants for the opt-in R6 MQTT + Home Assistant discovery feature.
// No broker/network is contacted here: these tests only guard the source-level
// contract that keeps the base firmware byte-identical and the LAN data local.

const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');

const ROOT = path.join(__dirname, '..', '..');
const FIRMWARE_DIR = path.join(ROOT, 'firmware', 'SMARTLUX');
const INO = path.join(FIRMWARE_DIR, 'SMARTLUX.ino');
const LOGIC = path.join(FIRMWARE_DIR, 'logic.h');
const MQTT = path.join(FIRMWARE_DIR, 'mqtt.h');
const SECRETS_EXAMPLE = path.join(FIRMWARE_DIR, 'secrets.example.h');
const PLATFORMIO_INI = path.join(FIRMWARE_DIR, 'platformio.ini');
const SKETCH_YAML = path.join(FIRMWARE_DIR, 'sketch.yaml');
const CI = path.join(ROOT, '.github', 'workflows', 'ci.yml');

const read = (p) => fs.readFileSync(p, 'utf8');

function statusFormatterBody() {
  const m = read(INO).match(
    /static int buildStatusJson\(char \*json, size_t n\) \{([\s\S]*?)\n\}/
  );
  assert.ok(m, 'buildStatusJson() formatter not found in SMARTLUX.ino');
  return m[1];
}

function statusKeys() {
  // The formatter uses adjacent C string literals; join them so a key split
  // across two lines is still found.
  const literals = [...statusFormatterBody().matchAll(/"((?:[^"\\]|\\.)*)"/g)]
    .map((m) => m[1])
    .join('');
  const keys = [...literals.matchAll(/\\"([a-z_0-9]+)\\":/g)].map((m) => m[1]);
  assert.ok(keys.length >= 24, `expected >= 24 status keys, found ${keys.length}`);
  assert.equal(new Set(keys).size, keys.length, 'status keys must be unique');
  return keys;
}

function mqttEntitySource() {
  const logic = read(LOGIC);
  assert.ok(
    /mqttEntities\s*\(/.test(logic),
    'logic.h must expose the mqttEntities() accessor'
  );
  return logic;
}

test('mqtt.h is fully behind the SMARTLUX_ENABLE_MQTT opt-in', () => {
  const mqtt = read(MQTT);
  assert.match(mqtt, /#if\s+!defined\(SMARTLUX_ENABLE_MQTT\)/);
  assert.match(mqtt, /#else\s*\/\/\s*SMARTLUX_ENABLE_MQTT/);
  assert.match(mqtt, /#endif\s*\/\/\s*SMARTLUX_ENABLE_MQTT/);
  // No-op stubs keep the .ino free of nested #if for the disabled case.
  assert.match(mqtt, /static inline void mqttBegin\(\) \{\}/);
  assert.match(mqtt, /static inline bool mqttWantsState\(bool\) \{ return false; \}/);
});

test('secrets.example.h keeps every MQTT define commented out', () => {
  const lines = read(SECRETS_EXAMPLE).split(/\r?\n/);
  const active = lines.filter(
    (l) => /^\s*#\s*define\s+(SMARTLUX_ENABLE_MQTT|MQTT_BROKER|MQTT_PORT|MQTT_USER|MQTT_PASS|MQTT_BASE_TOPIC|MQTT_DISCOVERY_PREFIX)\b/.test(l)
  );
  assert.deepEqual(active, [], 'MQTT defines in secrets.example.h must stay commented');
  assert.match(read(SECRETS_EXAMPLE), /SMARTLUX_ENABLE_MQTT/);
  assert.match(read(SECRETS_EXAMPLE), /MQTT_BROKER/);
});

test('MQTT uses a 1536-byte buffer and a 2 s TCP connection timeout', () => {
  const mqtt = read(MQTT);
  assert.match(mqtt, /setBufferSize\(1536\)/);
  assert.match(mqtt, /setConnectionTimeout\(2000\)/);
  // Stream::setTimeout() does NOT bound TCP connect on ESP32 core 3.x.
  assert.ok(!/\.setTimeout\s*\(/.test(mqtt), 'do not use setTimeout() for MQTT connect');
});

test('LWT is installed inside connect(), before the CONNECT packet', () => {
  const mqtt = read(MQTT);
  const connectCall = mqtt.match(/g_mqtt\.connect\([^;]+?\);/s);
  assert.ok(connectCall, 'PubSubClient.connect() call not found');
  assert.match(connectCall[0], /g_mqttAvail/);
  assert.match(connectCall[0], /,\s*1,\s*true,\s*"offline"\)/, 'LWT must be retained QoS-1 "offline"');
  // indexOf('mqttPublishDiscovery()') would match the function DEFINITION near
  // the top of the file, not the call after connect(). Search for the call
  // (with semicolon) starting after the connect() call instead.
  const connectAt = mqtt.indexOf('g_mqtt.connect(');
  const publishAt = mqtt.indexOf('mqttPublishDiscovery();', connectAt);
  assert.ok(publishAt > connectAt, 'LWT must be set before publishing online');
});

test('mqtt.h stays non-blocking and adds no second PROGMEM raw literal', () => {
  const mqtt = read(MQTT);
  assert.ok(!/\bdelay\s*\(/.test(mqtt), 'no delay() in MQTT glue');
  assert.ok(!/R"rawliteral\(/.test(mqtt), 'mqtt.h must not add a R"rawliteral( payload');
  const inoRawCount = (read(INO).match(/R"rawliteral\(/g) || []).length;
  assert.equal(inoRawCount, 2, 'the .ino must keep exactly 2 raw literals (INDEX_HTML, CHART_HTML)');
});

test('mqttPoll() wiring is compile-time gated after server.handleClient()', () => {
  const ino = read(INO);
  assert.match(ino, /#include "mqtt\.h"/);
  assert.match(ino, /#if defined\(SMARTLUX_ENABLE_MQTT\)[\s\S]{0,900}?static void pollMqtt\(\)/);
  const handler = ino.indexOf('server.handleClient();');
  const poll = ino.indexOf('pollMqtt();', handler);
  assert.ok(handler >= 0 && poll > handler, 'pollMqtt() must run after server.handleClient()');
});

test('every MQTT value_template key exists in the shared status formatter', () => {
  const keys = new Set(statusKeys());
  const logic = mqttEntitySource();
  const refs = [...logic.matchAll(/value_json\.([a-z_0-9]+)/g)].map((m) => m[1]);
  assert.ok(refs.length >= 9, `expected >= 9 value_json references, found ${refs.length}`);
  const missing = [...new Set(refs)].filter((k) => !keys.has(k));
  assert.deepEqual(missing, [], `MQTT templates reference keys absent from /api/status: ${missing}`);
});

test('entity table is a function-local static so the base build emits no rodata', () => {
  const logic = mqttEntitySource();
  const body = logic.slice(logic.indexOf('mqttEntities('));
  const local = body.match(/static const MqttEntity kEntities\[\]/);
  assert.ok(local, 'MqttEntity table must be function-local static');
  const tableStart = body.indexOf('static const MqttEntity kEntities[]');
  assert.ok(!/static const MqttEntity\s+\w+\[\]\s*=/.test(body.slice(0, tableStart)));
});

test('nine HA entities cover motion, light, energy and mode', () => {
  const logic = mqttEntitySource();
  const table = logic.slice(logic.indexOf('static const MqttEntity kEntities[]'));
  const objects = [...table.matchAll(/\{\s*"([a-z_]+)",\s*"([a-z_]+)"/g)].map((m) => m[2]);
  assert.equal(new Set(objects).size, objects.length, 'entity object ids must be unique');
  for (const required of ['motion', 'lamp', 'power', 'energy', 'savings', 'mode']) {
    assert.ok(objects.includes(required), `missing HA entity: ${required}`);
  }
});

test('PubSubClient@2.8 is pinned in both build systems', () => {
  assert.match(read(PLATFORMIO_INI), /PubSubClient@2\.8\b/);
  assert.match(read(SKETCH_YAML), /PubSubClient\s*\(2\.8\)/);
});

test('CI builds the four opt-in combinations under a 90% flash ceiling', () => {
  const ini = read(PLATFORMIO_INI);
  const ci = read(CI);
  for (const env of ['esp32dev', 'esp32dev-mqtt', 'esp32dev-ota', 'esp32dev-mqtt-ota']) {
    assert.match(ini, new RegExp(`\\[env:${env}\\]`), `missing PlatformIO env ${env}`);
    assert.ok(ci.includes(env), `CI must build ${env}`);
  }
  assert.match(ci, /1179648|90%\s*of\s*1310720/, 'CI must enforce a ~90% flash ceiling');
});

test('CI guard keeps the 8192/4096 PROGMEM budgets', () => {
  const ci = read(CI);
  assert.match(ci, /"INDEX_HTML":\s*8192/);
  assert.match(ci, /"CHART_HTML":\s*4096/);
});
