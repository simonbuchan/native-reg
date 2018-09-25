const assert = require('assert');

const reg = require('./');

const hkcuEnv = reg.open(reg.HKCU, 'Environment');
console.log('HKCU\\Environment hkey = %O', hkcuEnv);
assert(reg.isHKEY(hkcuEnv));

const hkcuNoSuchKey = reg.open(reg.HKLM, 'NoSuchKey');
console.log('HKLM\\NoSuchKey hkey = %O', hkcuNoSuchKey);
assert.strictEqual(hkcuNoSuchKey, undefined);

const rawValue = reg.queryRaw(hkcuEnv, 'TEMP');
assert.strictEqual(rawValue.type, reg.ValueType.EXPAND_SZ);
const value = reg.query(hkcuEnv, 'TEMP');
assert.strictEqual(value, reg.parseString(rawValue));

reg.close(hkcuEnv);
reg.close(hkcuNoSuchKey);
