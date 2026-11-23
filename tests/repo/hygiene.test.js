'use strict';

// Repo hygiene invariants: ignore rules, tracked binary files, required docs.

const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const { execSync } = require('node:child_process');

const ROOT = path.join(__dirname, '..', '..');
const GITIGNORE = path.join(ROOT, '.gitignore');

function gitLines() {
  const out = execSync('git ls-files', { cwd: ROOT, encoding: 'utf8' });
  return out.split('\n').map((s) => s.trim()).filter(Boolean);
}

test('.gitignore covers secrets, environment and build output', () => {
  const t = fs.readFileSync(GITIGNORE, 'utf8');
  assert.ok(
    t.includes('firmware/SMARTLUX/secrets.h'),
    '.gitignore must ignore firmware/SMARTLUX/secrets.h'
  );
  assert.ok(t.includes('.env'), '.gitignore must ignore .env');
  assert.ok(t.includes('.pio/'), '.gitignore must ignore .pio/');
  assert.ok(t.includes('*.stl'), '.gitignore must ignore *.stl');
});

test('no tracked binary STL files', () => {
  const stl = gitLines().filter((f) => /\.stl$/i.test(f));
  assert.deepEqual(
    stl,
    [],
    'no binary STL file should be tracked in git'
  );
});

test('dependency-free repo: no package.json / npm manifest', () => {
  assert.ok(
    !fs.existsSync(path.join(ROOT, 'package.json')),
    'this repo must not add a package.json (Node built-in test runner only)'
  );
});

test('core repo files exist', () => {
  const required = [
    'LICENSE',
    '.github/SECURITY.md',
    'CHANGELOG.md',
    'AGENTS.md',
  ];
  for (const rel of required) {
    assert.ok(
      fs.existsSync(path.join(ROOT, rel)),
      `required repo file is missing: ${rel}`
    );
  }
});

test('native firmware unit test project declares the native platform', () => {
  const p = fs.readFileSync(
    path.join(ROOT, 'tests', 'native', 'platformio.ini'),
    'utf8'
  );
  assert.match(p, /platform\s*=\s*native/);
});

test('enclosure design files exist', () => {
  const scad = path.join(ROOT, 'hardware', 'enclosure', 'smartlux_case.scad');
  const params = path.join(ROOT, 'hardware', 'enclosure', 'params.csv');
  assert.ok(fs.existsSync(scad), 'hardware/enclosure/smartlux_case.scad is missing');
  assert.ok(fs.existsSync(params), 'hardware/enclosure/params.csv is missing');
});
