import assert from 'node:assert/strict';

// todo: migrate to 'node:test'?
import { test, suite, suiteSetup } from 'mocha';

import { Access, Key } from 'native-reg';

const testingSubKeyName = 'Software\\native-reg-testing-key';

suiteSetup(() => {
  Key.HKCU.deleteTree(testingSubKeyName);
});

suite('predefined Keys', () => {
  test('enumKeyNames', () => {
    assertNameList(Key.HKCU.enumKeyNames());
  });

  test('enumValueNames', () => {
    assertNameList(Key.HKCU.enumValueNames());
  });
});

suite('open', () => {
  test('rejects invalid keys', () => {
    assert.throws(() => {
      Key.HKCU.open(123, Access.ALL_ACCESS);
    }, (error) => error instanceof assert.AssertionError);
  });

  test('returns null for missing key', () => {
    const hkcuNoSuchKey = Key.HKLM.open('NoSuchKey', Access.ALL_ACCESS);
    assert.equal(hkcuNoSuchKey, null);
  });

  test('can open and close HKLM with read access', () => {
    const userEnvKey = Key.HKLM.open('SOFTWARE', Access.READ);
    assert.notEqual(userEnvKey, null);
    userEnvKey.close();
  });

  test('will throw access denied for HKLM ALL_ACCESS', () => {
    assert.throws(() => {
      Key.HKLM.open('SOFTWARE', Access.ALL_ACCESS);
    }, (error) => error.errno === 5);
  });

  test('can open and close HKCU with ALL_ACCESS', () => {
    const userEnvKey = Key.HKCU.open('SOFTWARE', Access.ALL_ACCESS);
    userEnvKey.close();
  });

  test('create and delete key', () => {
    const createdKey = Key.HKCU.create(testingSubKeyName, Access.ALL_ACCESS);

    const openedKey = Key.HKCU.open(testingSubKeyName, Access.READ);
    assert.notEqual(openedKey, null);
    openedKey.close();

    Key.HKCU.deleteKey(testingSubKeyName);

    assert.equal(Key.HKCU.open(testingSubKeyName, Access.READ), null);
  });
});


function assertNameList(nameList) {
  assert(Array.isArray(nameList));
  for (const name of nameList) {
    assert(typeof name === 'string');
  }
}
