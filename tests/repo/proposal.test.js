'use strict';

// Repo-level invariant tests for the SMARTLUX proposal document.
// Static / document-level guards: they check structural integrity of the
// Markdown proposal, not the physics content itself.

const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');

const ROOT = path.join(__dirname, '..', '..');
const PROPOSAL = path.join(ROOT, 'proposal_SMARTLUX_Kelompok_2.md');

function readProposal() {
  const content = fs.readFileSync(PROPOSAL, 'utf8');
  const lines = content.split(/\r?\n/);
  return { content, lines };
}

test('proposal document exists and is readable', () => {
  assert.ok(fs.existsSync(PROPOSAL), 'proposal_SMARTLUX_Kelompok_2.md is missing');
  const { content } = readProposal();
  assert.ok(content.length > 100000, 'proposal should be a substantial document');
});

test('proposal embeds exactly two base64 PNG lines, each longer than 100000 chars', () => {
  const { lines } = readProposal();
  const pngLines = lines.filter((l) => l.includes('data:image/png;base64,'));
  assert.equal(
    pngLines.length,
    2,
    `expected exactly 2 base64 PNG lines, found ${pngLines.length}`
  );
  for (const line of pngLines) {
    assert.ok(
      line.length > 100000,
      `base64 PNG line should be > 100000 chars, got ${line.length}`
    );
  }
});

test('proposal keeps the BAB I / II / III headings', () => {
  const { content } = readProposal();
  const babHeadings = content.match(/^BAB [IVX]+[^\n]*$/gm) || [];
  const numbers = new Set(
    babHeadings.map((h) => (h.match(/^BAB ([IVX]+)/) || [])[1])
  );
  assert.deepEqual(
    [...numbers].sort(),
    ['I', 'II', 'III'],
    'proposal should keep BAB I, BAB II and BAB III headings'
  );
});

test('proposal keeps the intentional unfilled placeholders', () => {
  const { content } = readProposal();
  assert.ok(
    content.includes('_\\[TEMPAT LOGO RESMI SEKOLAH\\]_'),
    'intentional _[TEMPAT LOGO RESMI SEKOLAH]_ placeholder is missing'
  );
  assert.ok(
    content.includes('\\[Isi\\]'),
    'intentional [Isi] placeholder rows are missing'
  );
});

test('proposal keeps its markdown table rows intact', () => {
  const { content } = readProposal();
  assert.ok(
    content.includes('| **No.** | **Nama Anggota**'),
    'proposal member table header row (Nama Anggota) is missing'
  );
  assert.ok(
    content.includes('| ------- | ---------------------- | --------------------- |'),
    'proposal table separator row is missing'
  );
});
