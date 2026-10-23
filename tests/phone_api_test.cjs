const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const path = require('node:path');
const html = fs.readFileSync(path.join(__dirname, '../assets/phone.html'), 'utf8');
const api = html.slice(html.indexOf('async function api('), html.indexOf('\nfunction value('));
function client(fetch) {
  const elements = { pair: { hidden: true }, controls: { hidden: false } };
  const removed = [];
  const context = vm.createContext({ fetch, AbortController, setTimeout, clearTimeout,
    token: 'fixture-token', $: id => elements[id], sessionStorage: { removeItem: key => removed.push(key) } });
  vm.runInContext(api, context);
  return { context, elements, removed, request: data => context.api('/api/state', data) };
}
function response(body, status = 200, type = 'application/json') {
  return { ok: status === 200, status, headers: { get: () => type }, json: async () => body };
}
test('valid JSON preserves data and sends authorization', async () => {
  let sent;
  const c = client(async (_, options) => { sent = options; return response({ dirty: true }); });
  assert.equal((await c.request()).dirty, true);
  assert.equal(sent.headers.Authorization, 'Bearer fixture-token');
});
test('captive portal HTML becomes a readable connection error', async () => {
  await assert.rejects(client(async () => response(null, 200, 'text/html')).request(), /PC is not responding/);
});
test('malformed JSON never leaks its parser exception', async () => {
  const bad = response({}); bad.json = async () => { throw new SyntaxError('technical parser details'); };
  await assert.rejects(client(async () => bad).request(), /^Error: PC is not responding$/);
});
test('invalid JSON shapes become connection errors', async () => {
  for (const body of [null, [], 'text']) await assert.rejects(client(async () => response(body)).request(), /PC is not responding/);
});
test('network failures and aborts become readable connection errors', async () => {
  for (const name of ['TypeError', 'AbortError']) {
    await assert.rejects(client(async () => { const error = Error('browser-specific details'); error.name = name; throw error; }).request(), /PC is not responding/);
  }
});
test('server validation errors retain their message and HTTP status', async () => {
  await assert.rejects(client(async () => response({ error: 'Invalid color theme' }, 400)).request(), error => error.message === 'Invalid color theme' && error.status === 400);
});
test('401 clears credentials and brings pairing back', async () => {
  const c = client(async () => response({ error: 'Pair with the PC first' }, 401));
  await assert.rejects(c.request(), /Pair with the PC first/);
  assert.equal(c.context.token, ''); assert.equal(c.elements.pair.hidden, false);
  assert.equal(c.elements.controls.hidden, true); assert.deepEqual(c.removed, ['azec-token']);
});
test('all translated phone dictionaries contain the three server errors', () => {
  const start = html.indexOf('const phoneTexts=');
  const end = html.indexOf('\nfunction t(', start);
  const context = vm.createContext({}); vm.runInContext(html.slice(start, end), context);
  const maps = vm.runInContext('phoneTexts', context);
  assert.equal(Object.keys(maps).length, 8); // English uses the original message directly.
  for (const map of Object.values(maps)) {
    for (const key of ['Invalid color theme', 'This key is not safe for a global shortcut', 'Not found']) assert.ok(map[key], key);
  }
});
