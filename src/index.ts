import assert from 'node:assert';
import path from 'node:path';
import types from 'node:util/types';

const isWindows = process.platform == "win32";
let native: any;
if (isWindows) {
  // skip node-gyp-build for now, after moving to ESM
  const prebuildPath = path.join(import.meta.dirname, `../prebuilds/${process.platform}-${process.arch}/native-reg.node`);
  // the implementation of require() for .node files.
  const module = { exports: {} };
  process.dlopen(module, prebuildPath);
  native = module.exports;
}

/**
 * Values from winreg.h
 * @deprecated Use the statics on Key, e.g. Key.CLASSES_ROOT
 */
export enum HKEY {
  CLASSES_ROOT = 0x80000000,
  CURRENT_USER = 0x80000001,
  LOCAL_MACHINE = 0x80000002,
  USERS = 0x80000003,
  PERFORMANCE_DATA = 0x80000004,
  PERFORMANCE_TEXT = 0x80000050,
  PERFORMANCE_NLSTEXT = 0x80000060,
  CURRENT_CONFIG = 0x80000005,
  DYN_DATA = 0x80000006,
  CURRENT_USER_LOCAL_SETTINGS = 0x80000007,
}

export enum CreateKeyOptions {
  NON_VOLATILE = 0,
  VOLATILE = 1,
  CREATE_LINK = 2,
  BACKUP_RESTORE = 4,
}

export enum OpenKeyOptions {
  NONE = 0,
  OPEN_LINK = 8,
}

/**
 * Flags for desired access to a `Key`.
 *
 * From https://docs.microsoft.com/en-nz/windows/desktop/SysInfo/registry-key-security-and-access-rights
 */
export enum Access {
  // Specific rights
  QUERY_VALUE = 0x0001,
  SET_VALUE = 0x0002,
  CREATE_SUB_KEY = 0x0004,
  ENUMERATE_SUB_KEYS = 0x0008,
  NOTIFY = 0x0010,
  CREATE_LINK = 0x0020,

  // WOW64 behavior flags. See https://docs.microsoft.com/en-nz/windows/desktop/WinProg64/accessing-an-alternate-registry-view
  WOW64_64KEY = 0x0100,
  WOW64_32KEY = 0x0200,

  // Standard access rights
  DELETE = 0x00010000,

  // Generic rights.
  READ = 0x2_0019,
  WRITE = 0x2_0006,
  EXECUTE = READ,

  ALL_ACCESS = 0xf_003f,
}

/**
 * List of the known registry key type values.
 *
 * The exact byte format is not enforced by the registry, in particular
 * strings may not be an even number of bytes with the terminating `\0` character.
 * Use the format parsing helpers like `parseString`, or the wrapping methods that
 * use them like `Key.getValueSZ`, if you can.
 *
 * See https://learn.microsoft.com/en-us/windows/win32/sysinfo/registry-value-types
 *
 * Note that while the documentation mentions that strings may be
 *
 * > either a Unicode or an ANSI string, depending on whether you use the Unicode or ANSI functions.
 *
 * to avoid data loss the used APIs in question are always the Unicode variant.
 *
 * (Values from winnt.h)
 */
export enum ValueType {
  /**
   * Undocumented, possibly invalid.
   */
  NONE = 0,
  /**
   * A Windows "wide" string in Unicode UTF-16 format,
   * with a terminating `\0` character (two zero bytes).
   */
  SZ = 1,
  /**
   * A string in the same format as `SZ`, with environment variable references
   * in `%NAME%` format.
   * When retrieved without the flag `GetValueFlags.NO_EXPAND`, the
   * variables are expanded with the current value (or the empty string
   * if not defined).
   * As with `SZ`, the `\0` terminating character is not enforced.
   */
  EXPAND_SZ = 2,
  /**
   * Free form binary (bytes)
   */
  BINARY = 3,
  /** 32-bit number, stored in the Windows default little-endian byte order. */
  DWORD = 4,
  /** 32-bit number (same as `REG_DWORD`) */
  DWORD_LITTLE_ENDIAN = 4, //
  /** 32-bit number, stored in big-endian byte order. */
  DWORD_BIG_ENDIAN = 5,
  /**
   * Symbolic Link.
   * Stores a registry path in the same format as SZ.
   * (Not supported by the formatting APIs)
   */
  LINK = 6,
  /**
   * Multiple Unicode strings, each `\0`, then the list itself also `\0` terminated.
   * For example, `"first\0second\0third\0\0"`.
   * Neither `\0` terminating character is enforced.
   */
  MULTI_SZ = 7,
  /**
   * Undocumented, but has the winnt.h comment:
   * > Resource list in the resource map.
   * (Not supported by the formatting APIs)
   */
  RESOURCE_LIST = 8,
  /**
   * Undocumented, but has the winnt.h comment:
   * > Resource list in the hardware description.
   * (Not supported by the formatting APIs)
   */
  FULL_RESOURCE_DESCRIPTOR = 9,
  /**
   * Undocumented.
   * (Not supported by the formatting APIs)
   */
  RESOURCE_REQUIREMENTS_LIST = 10,
  /**
   * A 64-bit number, stored in the Windows default little-endian byte order.
   */
  QWORD = 11,
  /**
   * A 64-bit number (same as `QWORD`)
   */
  QWORD_LITTLE_ENDIAN = 11,
}

// From RegGetValue docs
export enum GetValueFlags {
  // Default (equivalent to RT_ANY)
  NONE = 0,

  // Expected / accepted type flags. If none are provided, uses RT_ANY
  RT_ANY = 0xffff,
  RT_REG_NONE = 0x0001,
  RT_REG_SZ = 0x0002,
  RT_REG_EXPAND_SZ = 0x0004,
  RT_REG_BINARY = 0x0008,
  RT_REG_DWORD = 0x0010,
  RT_REG_MULTI_SZ = 0x0020,
  RT_REG_QWORD = 0x0040,
  RT_DWORD = RT_REG_DWORD | RT_REG_BINARY,
  RT_QWORD = RT_REG_QWORD | RT_REG_BINARY,

  // Behavior flags.
  NO_EXPAND = 0x10000000,
  // ZEROONFAILURE = 0x20000000, // doesn't make sense here
  SUBKEY_WOW6464KEY = 0x00010000,
  SUBKEY_WOW6432KEY = 0x00020000,
}

// These common shorthands should really be pointing at Key.* values,
// but I don't want to make a breaking change without a major version
// with them being deprecated first. These will be undeprecated,
// but as `Key` values in a v3 (if released)

/** @deprecated Use Key.CLASSES_ROOT */
export const HKCR = HKEY.CLASSES_ROOT;
/** @deprecated Use Key.HKCU */
export const HKCU = HKEY.CURRENT_USER;
/** @deprecated Use Key.HKLM */
export const HKLM = HKEY.LOCAL_MACHINE;
/** @deprecated Use Key.HKU */
export const HKU = HKEY.USERS;

export type Value = Buffer & { type: ValueType };

export interface ExternalHKEY {
  __brand: 'external HKEY';
}

export interface LegacyHKEYObject {
  native: ExternalHKEY | null | undefined;
}

export type HKEYType = number | ExternalHKEY | LegacyHKEYObject;

export function isHKEY(hkey: unknown): hkey is HKEYType {
  return (
    (typeof hkey === 'number' && hkey !== 0 && hkey === hkey >>> 0) || // checks value is a positive uint32
    types.isExternal(hkey) ||
    (isWindows && hkey instanceof native.HKEY)
  );
}

// See usage below.
const disposeSymbol: typeof Symbol.dispose = Symbol.dispose || Symbol('mock dispose');

// Modern class API

export class Key implements Disposable {
  // Values from winreg.h
  /** HKEY_CLASSES_ROOT */
  static CLASSES_ROOT = new Key(0x80000000);
  /** HKEY_CURRENT_USER */
  static CURRENT_USER = new Key(0x80000001);
  /** HKEY_LOCAL_MACHINE */
  static LOCAL_MACHINE = new Key(0x80000002);
  /** HKEY_USERS */
  static USERS = new Key(0x80000003);
  /** HKEY_PERFORMANCE_DATA */
  static PERFORMANCE_DATA = new Key(0x80000004);
  /** HKEY_PERFORMANCE_TEXT */
  static PERFORMANCE_TEXT = new Key(0x80000050);
  /** HKEY_PERFORMANCE_NLSTEXT */
  static PERFORMANCE_NLSTEXT = new Key(0x80000060);
  /** HKEY_CURRENT_CONFIG */
  static CURRENT_CONFIG = new Key(0x80000005);
  /** HKEY_DYN_DATA */
  static DYN_DATA = new Key(0x80000006);
  /** HKEY_CURRENT_USER_LOCAL_SETTINGS */
  static CURRENT_USER_LOCAL_SETTINGS = new Key(0x80000007);

  static HKCR = Key.CLASSES_ROOT;
  static HKCU = Key.CURRENT_USER;
  static HKLM = Key.LOCAL_MACHINE;
  static HKU = Key.USERS;

  static openCurrentUser(access: Access = Access.ALL_ACCESS): Key {
    assert(isWindows);
    assert(typeof access === 'number');
    const handle = native.openCurrentUser(access);
    return new Key(handle);
  }

  static loadAppKey(file: string, access: Access): Key | null {
    assert(isWindows);
    assert(typeof file === 'string');
    assert(typeof access === 'number');
    const handle = native.loadAppKey(file, access);
    return handle ? new Key(handle) : null;
  }

  #handle: unknown;

  /**
   * The `handle` argument passed to the `constructor`, set to `null`
   * when closed (with `close()` or `[Symbol.dispose]()`.)
   *
   * Exposes the handle value for use with the deprecated v1 APIs
   * or other native modules that accept "external" HKEY values, if
   * this `Key` was returned by e.g. `open` or `create`. As there is
   * no way to represent these types in TypeScript, it is declared as
   * `unknown`
   *
   * May also be a `number` for predefined HKEY values such as `Key.CURRENT_USER`.
   */
  get handle(): unknown {
    return this.#handle;
  }

  constructor(handle: unknown) {
    assert(isHKEY(handle));
    this.#handle = handle;
  }

  close(): void {
    assert(isWindows);
    native.closeKey(this.#handle);
    this.#handle = null;
  }

  // The Symbol.dispose value is not present in node 19 and 21,
  // but is in 18, 20 and 22 up. Since that's really weird and I
  // can otherwise support 18 still, mock it if it's not present, since
  // otherwise it defines a method called "undefined".
  [disposeSymbol](): void {
    this.close();
  }

  create(subKey: string, access: Access = Access.ALL_ACCESS, options: CreateKeyOptions = 0): Key {
    assert(isWindows);
    assert(typeof subKey === 'string');
    assert(typeof options === 'number');
    assert(typeof access === 'number');
    const handle = native.createKey(this.#handle, subKey, options, access);
    return new Key(handle);
  }

  open(subKey: string, access: Access = Access.READ, options: OpenKeyOptions = 0): Key | null {
    assert(isWindows);
    assert(typeof subKey === 'string');
    assert(typeof options === 'number');
    assert(typeof access === 'number');
    const handle = native.openKey(this.#handle, subKey, options, access);
    return handle ? new Key(handle) : null;
  }

  enumKeyNames(): string[] {
    assert(isWindows);
    return native.enumKeyNames(this.#handle);
  }

  enumValueNames(): string[] {
    assert(isWindows);
    return native.enumValueNames(this.#handle);
  }

  queryValue(valueName: string | null): ParsedValue | null {
    return parseValue(this.queryValueRaw(valueName));
  }

  queryValueRaw(valueName: string | null): Value | null {
    assert(isWindows);
    assert(typeof valueName === 'string' || valueName === null);
    return native.queryValue(this.#handle, valueName || '');
  }

  getValue(
    subKey: string | null,
    valueName: string | null,
    flags: GetValueFlags = 0,
  ): ParsedValue | null {
    return parseValue(this.getValueRaw(subKey, valueName, flags));
  }

  getValueRaw(
    subKey: string | null,
    valueName: string | null,
    flags: GetValueFlags = 0,
  ): Value | null {
    assert(isWindows);
    assert(typeof subKey === 'string' || subKey === null);
    assert(typeof valueName === 'string' || valueName === null);
    assert(typeof flags === 'number');
    if ((flags & GetValueFlags.RT_ANY) === 0) {
      flags |= GetValueFlags.RT_ANY;
    }
    return native.getValue(this.#handle, subKey || '', valueName || '', flags);
  }

  setValueSZ(valueName: string | null, value: string): void {
    this.setValueRaw(valueName, ValueType.SZ, formatString(value));
  }

  setValueEXPAND_SZ(valueName: string | null, value: string): void {
    this.setValueRaw(valueName, ValueType.EXPAND_SZ, formatString(value));
  }

  setValueMULTI_SZ(valueName: string | null, value: string[]): void {
    this.setValueRaw(valueName, ValueType.MULTI_SZ, formatMultiString(value));
  }

  setValueDWORD(valueName: string | null, value: number): void {
    this.setValueRaw(valueName, ValueType.DWORD, formatDWORD(value));
  }

  setValueQWORD(valueName: string | null, value: number | bigint): void {
    this.setValueRaw(valueName, ValueType.QWORD, formatQWORD(value));
  }

  setValueRaw(
    valueName: string | null,
    valueType: ValueType,
    data: Buffer,
  ): void {
    assert(isWindows);
    assert(typeof valueName === 'string' || valueName === null);
    assert(typeof valueType === 'number');
    assert(Buffer.isBuffer(data));
    native.setValue(this.#handle, valueName || '', valueType, data);
  }

  renameKey(subKey: string | null, newSubKey: string): void {
    assert(isWindows);
    assert(typeof subKey === 'string' || subKey === null);
    assert(typeof newSubKey === 'string');
    native.renameKey(this.#handle, subKey || '', newSubKey);
  }

  copyTree(subKey: string | null, dest: Key): void {
    assert(isWindows);
    assert(typeof subKey === 'string' || subKey === null);
    return native.copyTree(this.#handle, subKey || '', dest.#handle);
  }

  deleteKey(subKey: string): boolean {
    assert(isWindows);
    assert(typeof subKey === 'string');
    return native.deleteKey(this.#handle, subKey);
  }

  deleteTree(subKey: string | null): boolean {
    assert(isWindows);
    assert(typeof subKey === 'string' || subKey === null);
    return native.deleteTree(this.#handle, subKey || '');
  }

  deleteKeyValue(subKey: string, valueName: string): boolean {
    assert(isWindows);
    assert(typeof subKey === 'string');
    assert(typeof valueName === 'string');
    return native.deleteKeyValue(this.#handle, subKey, valueName);
  }

  deleteValue(valueName: string | null): boolean {
    assert(isWindows);
    assert(typeof valueName === 'string' || valueName === null);
    return native.deleteValue(this.#handle, valueName || '');
  }
}

// Raw APIs

/** @deprecated Use Key#create */
export function createKey(
  hkey: HKEY,
  subKey: string,
  access: Access,
  options: CreateKeyOptions = 0,
): HKEY {
  assert(isWindows);
  assert(isHKEY(hkey));
  assert(typeof subKey === 'string');
  assert(typeof options === 'number');
  assert(typeof access === 'number');
  return native.createKey(hkey, subKey, options, access);
}

/** @deprecated Use Key#open */
export function openKey(
  hkey: HKEY,
  subKey: string,
  access: Access,
  options: OpenKeyOptions = 0,
): HKEY | null {
  assert(isWindows);
  assert(isHKEY(hkey));
  assert(typeof subKey === 'string');
  assert(typeof options === 'number');
  assert(typeof access === 'number');
  return native.openKey(hkey, subKey, options, access);
}

/** @deprecated Use Key.openCurrentUser */
export function openCurrentUser(access: Access = Access.ALL_ACCESS): HKEY {
  assert(isWindows);
  assert(typeof access === 'number');
  return native.openCurrentUser(access);
}

/** @deprecated Use Key.loadAppKey */
export function loadAppKey(file: string, access: Access): HKEY | null {
  assert(isWindows);
  assert(typeof file === 'string');
  assert(typeof access === 'number');
  return native.loadAppKey(file, access);
}

/** @deprecated Use Key#enumKeyNames */
export function enumKeyNames(hkey: HKEY): string[] {
  assert(isWindows);
  assert(isHKEY(hkey));
  return native.enumKeyNames(hkey);
}

/** @deprecated Use Key#enumValueNames */
export function enumValueNames(hkey: HKEY): string[] {
  assert(isWindows);
  assert(isHKEY(hkey));
  return native.enumValueNames(hkey);
}

/** @deprecated Use Key#queryValueRaw */
export function queryValueRaw(
  hkey: HKEY,
  valueName: string | null,
): Value | null {
  assert(isWindows);
  assert(isHKEY(hkey));
  assert(typeof valueName === 'string' || valueName === null);
  return native.queryValue(hkey, valueName || '');
}

/** @deprecated Use Key#getValueRaw */
export function getValueRaw(
  hkey: HKEY,
  subKey: string | null,
  valueName: string | null,
  flags: GetValueFlags = 0,
): Value | null {
  assert(isWindows);
  assert(isHKEY(hkey));
  assert(typeof subKey === 'string' || subKey === null);
  assert(typeof valueName === 'string' || valueName === null);
  assert(typeof flags === 'number');
  if ((flags & GetValueFlags.RT_ANY) === 0) {
    flags |= GetValueFlags.RT_ANY;
  }
  return native.getValue(hkey, subKey || '', valueName || '', flags);
}

/** @deprecated Use Key#setValueRaw */
export function setValueRaw(
  hkey: HKEY,
  valueName: string | null,
  valueType: ValueType,
  data: Buffer,
): void {
  assert(isWindows);
  assert(isHKEY(hkey));
  assert(typeof valueName === 'string' || valueName === null);
  assert(typeof valueType === 'number');
  assert(Buffer.isBuffer(data));
  native.setValue(hkey, valueName || '', valueType, data);
}

/** @deprecated Use Key#renameKey */
export function renameKey(
  hkey: HKEY,
  subKey: string | null,
  newSubKey: string,
): void {
  assert(isWindows);
  assert(isHKEY(hkey));
  assert(typeof subKey === 'string' || subKey === null);
  assert(typeof newSubKey === 'string');
  return native.renameKey(hkey, subKey || '', newSubKey);
}

/** @deprecated Use Key#copyTree */
export function copyTree(
  hkeySrc: HKEY,
  subKey: string | null,
  hkeyDest: HKEY,
): void {
  assert(isWindows);
  assert(isHKEY(hkeySrc));
  assert(typeof subKey === 'string' || subKey === null);
  assert(isHKEY(hkeyDest));
  return native.copyTree(hkeySrc, subKey || '', hkeyDest);
}

/** @deprecated Use Key#deleteKey */
export function deleteKey(hkey: HKEY, subKey: string): boolean {
  assert(isWindows);
  assert(isHKEY(hkey));
  assert(typeof subKey === 'string');
  return native.deleteKey(hkey, subKey);
}

/** @deprecated Use {@see Key#deleteTree} */
export function deleteTree(hkey: HKEY, subKey: string | null): boolean {
  assert(isWindows);
  assert(isHKEY(hkey));
  assert(typeof subKey === 'string' || subKey === null);
  return native.deleteTree(hkey, subKey || '');
}

/** @deprecated Use Key#deleteKeyValue */
export function deleteKeyValue(
  hkey: HKEY,
  subKey: string,
  valueName: string,
): boolean {
  assert(isWindows);
  assert(isHKEY(hkey));
  assert(typeof subKey === 'string');
  assert(typeof valueName === 'string');
  return native.deleteKeyValue(hkey, subKey, valueName);
}

/** @deprecated Use Key#deleteValue */
export function deleteValue(hkey: HKEY, valueName: string | null): boolean {
  assert(isWindows);
  assert(isHKEY(hkey));
  assert(typeof valueName === 'string' || valueName === null);
  return native.deleteValue(hkey, valueName || '');
}

/** @deprecated Use Key#close (or `using`) */
export function closeKey(hkey: HKEY | null | undefined): void {
  assert(isWindows);
  if (hkey == null) return; // nicely handle uninitialized
  assert(isHKEY(hkey));
  native.closeKey(hkey);
}

// Format helpers

export type ParsedValue = number | bigint | string | string[] | Buffer;

export function parseValue(value: Value | null): ParsedValue | null {
  if (value === null) {
    return null;
  }
  switch (value.type) {
    default:
      return assert.fail(`Unhandled reg value type: ${value.type}`);
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
    case ValueType.QWORD_LITTLE_ENDIAN:
      return value.readBigUInt64LE(0);
  }
}

function trimUcs2Null(value: Buffer) {
  // https://docs.microsoft.com/en-us/windows/desktop/api/Winreg/nf-winreg-regqueryvalueexw
  // Remarks: "The string may not have been stored with the proper terminating null characters"
  if (
    value.length >= 2 &&
    value[value.length - 2] === 0 &&
    value[value.length - 1] === 0
  ) {
    value = value.slice(0, -2);
  }
  return value;
}

export function parseString(value: Buffer): string {
  value = trimUcs2Null(value);
  return value.toString('ucs-2');
}

export function parseMultiString(value: Buffer) {
  value = trimUcs2Null(value);
  value = trimUcs2Null(value);
  return value.toString('ucs-2').split('\0');
}

export function formatString(value: string): Buffer {
  return Buffer.from(value + '\0', 'ucs-2');
}

export function formatMultiString(values: string[]): Buffer {
  return Buffer.from(values.join('\0') + '\0\0', 'ucs-2');
}

export function formatDWORD(value: number): Buffer {
  const data = Buffer.alloc(4);
  data.writeUInt32LE(value, 0);
  return data;
}

export function formatQWORD(value: number | bigint): Buffer {
  const data = Buffer.alloc(8);
  if (typeof value === 'bigint') {
    data.writeBigUInt64LE(value, 0);
  } else {
    data.writeUInt32LE(value, 0);
    // bit operations immediately cast to a 32-bit value
    data.writeUInt32LE(value / 0x1_0000_0000, 4);
  }
  return data;
}

// Formatted APIs

/** @deprecated Use Key#setValueSZ */
export function setValueSZ(
  hkey: HKEY,
  valueName: string | null,
  value: string,
): void {
  setValueRaw(hkey, valueName, ValueType.SZ, formatString(value));
}

/** @deprecated Use Key#setValueEXPAND_SZ */
export function setValueEXPAND_SZ(
  hkey: HKEY,
  valueName: string | null,
  value: string,
): void {
  setValueRaw(hkey, valueName, ValueType.EXPAND_SZ, formatString(value));
}

/** @deprecated Use Key#setValueMULTI_SZ */
export function setValueMULTI_SZ(
  hkey: HKEY,
  valueName: string | null,
  value: string[],
): void {
  setValueRaw(hkey, valueName, ValueType.MULTI_SZ, formatMultiString(value));
}

/** @deprecated Use Key#setValueDWORD */
export function setValueDWORD(
  hkey: HKEY,
  valueName: string | null,
  value: number,
): void {
  setValueRaw(hkey, valueName, ValueType.DWORD, formatDWORD(value));
}

/** @deprecated Use Key#setValueQWORD */
export function setValueQWORD(
  hkey: HKEY,
  valueName: string | null,
  value: number | bigint,
): void {
  setValueRaw(hkey, valueName, ValueType.QWORD, formatQWORD(value));
}

/** @deprecated Use Key#getValue */
export function getValue(
  hkey: HKEY,
  subKey: string | null,
  valueName: string | null,
  flags: GetValueFlags = 0,
): ParsedValue | null {
  return parseValue(getValueRaw(hkey, subKey, valueName, flags));
}

/** @deprecated Use Key#queryValue */
export function queryValue(
  hkey: HKEY,
  valueName: string | null,
): ParsedValue | null {
  return parseValue(queryValueRaw(hkey, valueName));
}
