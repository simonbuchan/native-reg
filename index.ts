const assert = require('assert');

let native: any;
try {
  native = require('./build/Release/fastreg.node');
} catch (e) {
  native = require('./build/Debug/fastreg.node')
}

// from winreg.h
export enum HKEY {
  CLASSES_ROOT                   = 0x80000000,
  CURRENT_USER                   = 0x80000001,
  LOCAL_MACHINE                  = 0x80000002,
  USERS                          = 0x80000003,
  PERFORMANCE_DATA               = 0x80000004,
  PERFORMANCE_TEXT               = 0x80000050,
  PERFORMANCE_NLSTEXT            = 0x80000060,
  CURRENT_CONFIG                 = 0x80000005,
  DYN_DATA                       = 0x80000006,
  CURRENT_USER_LOCAL_SETTINGS    = 0x80000007,
}

// from winnt.h
export enum ValueType  {
  NONE                       = 0, // No value type
  SZ                         = 1, // Unicode nul terminated string
  EXPAND_SZ                  = 2, // Unicode nul terminated string
                                  // (with environment variable references)
  BINARY                     = 3, // Free form binary
  DWORD                      = 4, // 32-bit number
  DWORD_LITTLE_ENDIAN        = 4, // 32-bit number (same as REG_DWORD)
  DWORD_BIG_ENDIAN           = 5, // 32-bit number
  LINK                       = 6, // Symbolic Link (unicode)
  MULTI_SZ                   = 7, // Multiple Unicode strings
  RESOURCE_LIST              = 8, // Resource list in the resource map
  FULL_RESOURCE_DESCRIPTOR   = 9, // Resource list in the hardware description
  RESOURCE_REQUIREMENTS_LIST = 10,
  QWORD                      = 11, // 64-bit number
  QWORD_LITTLE_ENDIAN        = 11, // 64-bit number (same as REG_QWORD)
}

export const HKCR = HKEY.CLASSES_ROOT;
export const HKCU = HKEY.CURRENT_USER;
export const HKLM = HKEY.LOCAL_MACHINE;
export const HKU = HKEY.USERS;

export function isHKEY(hkey: any): hkey is HKEY {
  return typeof hkey === 'number' && hkey > 0 && hkey < 0xFFFF_FFFF;
}

export function create(hkey: HKEY, subKey: string): HKEY {
  assert(isHKEY(hkey));
  assert(typeof subKey === 'string');
  return native.create(hkey, subKey);
}

export function open(hkey: HKEY, subKey: string): HKEY {
  assert(isHKEY(hkey));
  assert(typeof subKey === 'string');
  return native.open(hkey, subKey);
}

export type Value<Type extends ValueType = ValueType> = Buffer & { type: Type };

export function query(hkey: HKEY, valueName: string): number | string | string[] | Buffer {
  const value = queryRaw(hkey, valueName);
  switch (value.type) {
    default:
      throw new Error(`Unhandled reg value type: ${value.type}`)
    case ValueType.SZ:
    case ValueType.EXPAND_SZ:
      return parseString(value);
    case ValueType.BINARY:
      return value;
    case ValueType.DWORD_LITTLE_ENDIAN:
      return value.readUInt32LE(0);
    case ValueType.DWORD_BIG_ENDIAN:
      return value.readUInt32BE(0);
    case ValueType.MULTI_SZ:
      return parseMultiString(value);
  }
}

export function parseString(value: Buffer) {
  // https://docs.microsoft.com/en-us/windows/desktop/api/Winreg/nf-winreg-regqueryvalueexw
  // Remarks: "The string may not have been stored with the proper terminating null characters"
  if (value.length > 2 && !value[value.length - 2] && !value[value.length - 1]) {
    value = value.slice(0, -2);
  }
  return value.toString('ucs-2');
}

export function parseMultiString(value: Buffer) {
  return value.slice(0, -4).toString('ucs-2').split('\0');
}

export function queryRaw(hkey: HKEY, valueName: string): Value {
  assert(isHKEY(hkey));
  assert(typeof valueName === 'string');
  return native.query(hkey, valueName);
}

export function close(hkey: HKEY | null | undefined): void {
  if (hkey != null) { // not null or undefined
    assert(isHKEY(hkey));
    native.close(hkey);
  }
}
