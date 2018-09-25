"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
var assert = require('assert');
var native;
try {
    native = require('./build/Release/fastreg.node');
}
catch (e) {
    native = require('./build/Debug/fastreg.node');
}
// from winreg.h
var HKEY;
(function (HKEY) {
    HKEY[HKEY["CLASSES_ROOT"] = 2147483648] = "CLASSES_ROOT";
    HKEY[HKEY["CURRENT_USER"] = 2147483649] = "CURRENT_USER";
    HKEY[HKEY["LOCAL_MACHINE"] = 2147483650] = "LOCAL_MACHINE";
    HKEY[HKEY["USERS"] = 2147483651] = "USERS";
    HKEY[HKEY["PERFORMANCE_DATA"] = 2147483652] = "PERFORMANCE_DATA";
    HKEY[HKEY["PERFORMANCE_TEXT"] = 2147483728] = "PERFORMANCE_TEXT";
    HKEY[HKEY["PERFORMANCE_NLSTEXT"] = 2147483744] = "PERFORMANCE_NLSTEXT";
    HKEY[HKEY["CURRENT_CONFIG"] = 2147483653] = "CURRENT_CONFIG";
    HKEY[HKEY["DYN_DATA"] = 2147483654] = "DYN_DATA";
    HKEY[HKEY["CURRENT_USER_LOCAL_SETTINGS"] = 2147483655] = "CURRENT_USER_LOCAL_SETTINGS";
})(HKEY = exports.HKEY || (exports.HKEY = {}));
// from winnt.h
var ValueType;
(function (ValueType) {
    ValueType[ValueType["NONE"] = 0] = "NONE";
    ValueType[ValueType["SZ"] = 1] = "SZ";
    ValueType[ValueType["EXPAND_SZ"] = 2] = "EXPAND_SZ";
    // (with environment variable references)
    ValueType[ValueType["BINARY"] = 3] = "BINARY";
    ValueType[ValueType["DWORD"] = 4] = "DWORD";
    ValueType[ValueType["DWORD_LITTLE_ENDIAN"] = 4] = "DWORD_LITTLE_ENDIAN";
    ValueType[ValueType["DWORD_BIG_ENDIAN"] = 5] = "DWORD_BIG_ENDIAN";
    ValueType[ValueType["LINK"] = 6] = "LINK";
    ValueType[ValueType["MULTI_SZ"] = 7] = "MULTI_SZ";
    ValueType[ValueType["RESOURCE_LIST"] = 8] = "RESOURCE_LIST";
    ValueType[ValueType["FULL_RESOURCE_DESCRIPTOR"] = 9] = "FULL_RESOURCE_DESCRIPTOR";
    ValueType[ValueType["RESOURCE_REQUIREMENTS_LIST"] = 10] = "RESOURCE_REQUIREMENTS_LIST";
    ValueType[ValueType["QWORD"] = 11] = "QWORD";
    ValueType[ValueType["QWORD_LITTLE_ENDIAN"] = 11] = "QWORD_LITTLE_ENDIAN";
})(ValueType = exports.ValueType || (exports.ValueType = {}));
exports.HKCR = HKEY.CLASSES_ROOT;
exports.HKCU = HKEY.CURRENT_USER;
exports.HKLM = HKEY.LOCAL_MACHINE;
exports.HKU = HKEY.USERS;
function isHKEY(hkey) {
    return typeof hkey === 'number' && hkey > 0 && hkey < 4294967295;
}
exports.isHKEY = isHKEY;
function create(hkey, subKey) {
    assert(isHKEY(hkey));
    assert(typeof subKey === 'string');
    return native.create(hkey, subKey);
}
exports.create = create;
function open(hkey, subKey) {
    assert(isHKEY(hkey));
    assert(typeof subKey === 'string');
    return native.open(hkey, subKey);
}
exports.open = open;
function query(hkey, valueName) {
    var value = queryRaw(hkey, valueName);
    switch (value.type) {
        default:
            throw new Error("Unhandled reg value type: " + value.type);
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
exports.query = query;
function parseString(value) {
    // https://docs.microsoft.com/en-us/windows/desktop/api/Winreg/nf-winreg-regqueryvalueexw
    // Remarks: "The string may not have been stored with the proper terminating null characters"
    if (value.length > 2 && !value[value.length - 2] && !value[value.length - 1]) {
        value = value.slice(0, -2);
    }
    return value.toString('ucs-2');
}
exports.parseString = parseString;
function parseMultiString(value) {
    return value.slice(0, -4).toString('ucs-2').split('\0');
}
exports.parseMultiString = parseMultiString;
function queryRaw(hkey, valueName) {
    assert(isHKEY(hkey));
    assert(typeof valueName === 'string');
    return native.query(hkey, valueName);
}
exports.queryRaw = queryRaw;
function close(hkey) {
    if (hkey != null) { // not null or undefined
        assert(isHKEY(hkey));
        native.close(hkey);
    }
}
exports.close = close;
//# sourceMappingURL=index.js.map