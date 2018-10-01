const assert = require('assert');

const reg = require('./lib');

assert(reg.isHKEY(1));
assert(reg.isHKEY(reg.HKCU));
assert(reg.isHKEY(reg.HKEY.CURRENT_USER));
assert(!reg.isHKEY(undefined));
assert(!reg.isHKEY('HKCU'));
assert(!reg.isHKEY(0));
assert(!reg.isHKEY(0.1));
assert(!reg.isHKEY(Number.MAX_SAFE_INTEGER));

assert.throws(() => {
  reg.openKey('HKCU', 'Environment', reg.Access.ALL_ACCESS);
}, (error) => error instanceof assert.AssertionError);

const userEnvKey = reg.openKey(reg.HKCU, 'Environment', reg.Access.ALL_ACCESS);
console.log('HKCU\\Environment hkey = %O', userEnvKey);
assert(reg.isHKEY(userEnvKey));

const hkcuNoSuchKey = reg.openKey(reg.HKLM, 'NoSuchKey', reg.Access.ALL_ACCESS);
assert.strictEqual(hkcuNoSuchKey, null);
reg.closeKey(hkcuNoSuchKey);

console.log('HKCU keys: %O', reg.enumKeyNames(reg.HKCU));
console.log('HKCU values: %O', reg.enumValueNames(reg.HKCU));
console.log('HKCU\\Environment keys: %O', reg.enumKeyNames(userEnvKey));
console.log('HKCU\\Environment values: %O', reg.enumValueNames(userEnvKey));

const envTempQueryValueRaw = reg.queryValueRaw(userEnvKey, 'TEMP');
assert.strictEqual(envTempQueryValueRaw.type, reg.ValueType.EXPAND_SZ);

const envTempQueryValue = reg.queryValue(userEnvKey, 'TEMP');
assert.strictEqual(envTempQueryValue, reg.parseString(envTempQueryValueRaw));

const envTempGetValueRaw = reg.getValueRaw(
  reg.HKCU,
 'Environment',
 'TEMP',
  reg.GetValueFlags.RT_ANY | reg.GetValueFlags.NO_EXPAND);

console.log('getValueRaw(TEMP): %O', envTempGetValueRaw.toString('ucs-2'));

assert.strictEqual(envTempGetValueRaw.toString('ucs-2'), envTempQueryValueRaw.toString('ucs-2'));

const envNoSuchQueryValueRaw = reg.queryValueRaw(userEnvKey, 'NO_SUCH_VALUE');
assert.strictEqual(envNoSuchQueryValueRaw, null);

const envNoSuchQueryValue = reg.queryValue(userEnvKey, 'NO_SUCH_VALUE');
assert.strictEqual(envNoSuchQueryValue, null);

const envNoSuchGetValueRaw = reg.getValueRaw(reg.HKCU, 'Environment', 'NO_SUCH_VALUE');
assert.strictEqual(envNoSuchGetValueRaw, null);

const envNoSuchGetValue= reg.getValue(reg.HKCU, 'Environment', 'NO_SUCH_VALUE');
assert.strictEqual(envNoSuchGetValueRaw, null);

reg.closeKey(userEnvKey);

assert.throws(() => {
  reg.queryValue(userEnvKey, 'TEMP');
}, error => error.errno === 6 && error.syscall === 'RegQueryValueExW');

const testingSubKey = 'Software\\native-reg-testing-key';

const testKey = reg.createKey(reg.HKCU, testingSubKey, reg.Access.ALL_ACCESS);
const testValueData = Buffer.from('Test Value\0', 'ucs-2');
reg.setValueRaw(testKey, 'Testing', reg.ValueType.SZ, testValueData);

const testQueryValueRaw = reg.queryValueRaw(testKey, 'Testing');
assert.strictEqual(testQueryValueRaw.type, reg.ValueType.SZ);
assert.strictEqual(testQueryValueRaw.toString('ucs-2'), testValueData.toString('ucs-2'));

const testGetValueRaw = reg.getValueRaw(
  reg.HKCU,
  testingSubKey,
  'Testing',
  reg.GetValueFlags.NO_EXPAND);
console.log('getValueRaw(Testing): %O', testGetValueRaw.toString('ucs-2'));
assert.strictEqual(testGetValueRaw.type, reg.ValueType.SZ);
assert.strictEqual(testGetValueRaw.toString('ucs-2'), testValueData.toString('ucs-2'));

reg.setValueDWORD(testKey, 'Testing', 0x010203);
const testGetValueRaw2 = reg.getValueRaw(
  reg.HKCU,
  testingSubKey,
  'Testing',
  reg.GetValueFlags.RT_ANY | reg.GetValueFlags.NO_EXPAND);
console.log('getValueRaw(Testing): %O', testGetValueRaw2);
assert.strictEqual(testGetValueRaw2.toString(), '\x03\x02\x01\x00');

reg.deleteValue(testKey, 'Testing');
assert.strictEqual(reg.queryValue(testKey, 'Testing'), null);

reg.closeKey(testKey);

