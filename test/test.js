const assert = require('assert').strict;

const reg = require('..');

const testingSubKeyName = 'Software\\native-reg-testing-key';

suite("predefined HKEYS", () => {
  suite("isKEY", () => {
    test("accepts valid", () => {
      assert(reg.isHKEY(1));
      assert(reg.isHKEY(reg.HKCU));
      assert(reg.isHKEY(reg.HKEY.CURRENT_USER));
    });

    test("rejects invalid", () => {
      assert(!reg.isHKEY(undefined));
      assert(!reg.isHKEY('HKCU'));
      assert(!reg.isHKEY(0));
      assert(!reg.isHKEY(0.1));
      assert(!reg.isHKEY(Number.MAX_SAFE_INTEGER));
      assert(!reg.isHKEY({ native: 1 }));
      class HKEY {}
      assert(!reg.isHKEY(new HKEY()));
    });
  });

  test("enumKeyNames", () => {
    assertNameList(reg.enumKeyNames(reg.HKCU));
  });

  test("enumKeyValues", () => {
    assertNameList(reg.enumValueNames(reg.HKCU));
  });
});

suite("openKey", () => {
  test("rejects invalid keys", () => {
    assert.throws(() => {
      reg.openKey('HKCU', 'Environment', reg.Access.ALL_ACCESS);
    }, (error) => error instanceof assert.AssertionError);

    assert.throws(() => {
      reg.openKey(reg.HKCU, 123, reg.Access.ALL_ACCESS);
    }, (error) => error instanceof assert.AssertionError);
  });

  test("returns null for missing key", () => {
    const hkcuNoSuchKey = reg.openKey(reg.HKLM, 'NoSuchKey', reg.Access.ALL_ACCESS);
    assert.equal(hkcuNoSuchKey, null);
    reg.closeKey(hkcuNoSuchKey);
  });

  test("can open and close HKLM with read access", () => {
    const userEnvKey = reg.openKey(reg.HKLM, 'SOFTWARE', reg.Access.READ);
    assert.notEqual(userEnvKey, null);
    reg.closeKey(userEnvKey);
  });

  test("will throw access denied for HKLM ALL_ACCESS", () => {
    assert.throws(() => {
      reg.openKey(reg.HKLM, 'SOFTWARE', reg.Access.ALL_ACCESS);
    }, (error) => error.errno === 5);
  });

  test("can open and close HKCU with ALL_ACCESS", () => {
    const userEnvKey = reg.openKey(reg.HKCU, 'SOFTWARE', reg.Access.ALL_ACCESS);
    reg.closeKey(userEnvKey);
  });

  test("create and delete key", () => {
    const createdKey = reg.createKey(reg.HKCU, testingSubKeyName, reg.Access.ALL_ACCESS);

    const openedKey = reg.openKey(reg.HKCU, testingSubKeyName, reg.Access.READ);
    assert.notEqual(openedKey, null);
    reg.closeKey(openedKey);

    reg.deleteKey(reg.HKCU, testingSubKeyName);

    assert.equal(reg.openKey(reg.HKCU, testingSubKeyName, reg.Access.READ), null);
  });
});

suite("with HKCU\\Environment", () => {
  let userEnvKey = null;

  function closeUserEnvKey() {
    if (userEnvKey) {
      reg.closeKey(userEnvKey);
      userEnvKey = null;
    }
  }

  setup(() => {
    userEnvKey = reg.openKey(reg.HKCU, 'Environment', reg.Access.ALL_ACCESS);
  });

  teardown(closeUserEnvKey);

  test("isKEY", () => {
    assert(reg.isHKEY(userEnvKey));
    assert(reg.isHKEY(userEnvKey.native));
  });

  test("enumKeyNames", () => {
    assertNameList(reg.enumKeyNames(userEnvKey));
  });

  test("enumKeyValues", () => {
    assertNameList(reg.enumValueNames(userEnvKey));
  });

  test("read values", () => {
    const envTempQueryValueRaw = reg.queryValueRaw(userEnvKey, 'TEMP');
    assert.equal(envTempQueryValueRaw.type, reg.ValueType.EXPAND_SZ);

    const envTempQueryValue = reg.queryValue(userEnvKey, 'TEMP');
    assert.equal(envTempQueryValue, reg.parseString(envTempQueryValueRaw));

    const envTempGetValueRaw = reg.getValueRaw(
        reg.HKCU,
        'Environment',
        'TEMP',
        reg.GetValueFlags.RT_ANY | reg.GetValueFlags.NO_EXPAND);

    console.log('getValueRaw(TEMP): %O', envTempGetValueRaw.toString('ucs-2'));

    assert.equal(envTempGetValueRaw.toString('ucs-2'), envTempQueryValueRaw.toString('ucs-2'));
  });

  test("read missing value returns null", () => {
    const envNoSuchQueryValueRaw = reg.queryValueRaw(userEnvKey, 'NO_SUCH_VALUE');
    assert.equal(envNoSuchQueryValueRaw, null);

    const envNoSuchQueryValue = reg.queryValue(userEnvKey, 'NO_SUCH_VALUE');
    assert.equal(envNoSuchQueryValue, null);

    const envNoSuchGetValueRaw = reg.getValueRaw(reg.HKCU, 'Environment', 'NO_SUCH_VALUE');
    assert.equal(envNoSuchGetValueRaw, null);

    const envNoSuchGetValue = reg.getValue(reg.HKCU, 'Environment', 'NO_SUCH_VALUE');
    assert.equal(envNoSuchGetValue, null);
  });

  test("read closed value throws", () => {
    let closedKey = userEnvKey;
    closeUserEnvKey(); // clears userEnvKey to avoid double-closing

    assert.throws(() => {
      reg.queryValue(closedKey, 'TEMP');
    }, error => error.message === "HKEY already closed"); // FIXME: something like instanceof reg.ClosedKeyError?
  });
});

suite("with temporary key", () => {
  let testKey = null;

  setup(() => {
    testKey = reg.createKey(reg.HKCU, testingSubKeyName, reg.Access.ALL_ACCESS);
  });

  teardown(() => {
    reg.deleteKey(testKey, testingSubKeyName);
  });

  test("write values", () => {
    const testValueData = Buffer.from('Test Value\0', 'ucs-2');
    reg.setValueRaw(testKey, 'Testing', reg.ValueType.SZ, testValueData);

    const testQueryValueRaw = reg.queryValueRaw(testKey, 'Testing');
    assert.equal(testQueryValueRaw.type, reg.ValueType.SZ);
    assert.equal(testQueryValueRaw.toString('ucs-2'), testValueData.toString('ucs-2'));

    const testNativeQueryValueRaw = reg.queryValueRaw(testKey.native, 'Testing');
    assert.equal(testNativeQueryValueRaw.type, reg.ValueType.SZ);
    assert.equal(testNativeQueryValueRaw.toString('ucs-2'), testValueData.toString('ucs-2'));

    const testGetValueRaw = reg.getValueRaw(
        reg.HKCU,
        testingSubKeyName,
        'Testing',
        reg.GetValueFlags.NO_EXPAND);
    console.log('getValueRaw(Testing): %O', testGetValueRaw.toString('ucs-2'));
    assert.equal(testGetValueRaw.type, reg.ValueType.SZ);
    assert.equal(testGetValueRaw.toString('ucs-2'), testValueData.toString('ucs-2'));

    reg.setValueDWORD(testKey, 'Testing', 0x010203);
    const testGetValueRaw2 = reg.getValueRaw(
        reg.HKCU,
        testingSubKeyName,
        'Testing',
        reg.GetValueFlags.RT_ANY | reg.GetValueFlags.NO_EXPAND);
    console.log('getValueRaw(Testing): %O', testGetValueRaw2);
    assert.equal(testGetValueRaw2.toString(), '\x03\x02\x01\x00');

    reg.deleteValue(testKey, 'Testing');
    assert.equal(reg.queryValue(testKey, 'Testing'), null);
  });
});

function assertNameList(nameList) {
  assert(Array.isArray(nameList));
  for (const name of nameList) {
    assert(typeof name === 'string');
  }
}
