#include <nan.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <vector>

NAN_METHOD(open);
NAN_METHOD(create);
NAN_METHOD(close);

auto to_hkey(v8::Local<v8::Value> value) {
    return reinterpret_cast<HKEY>((size_t) value->Uint32Value());
}

auto to_ucs2(v8::Local<v8::Value> value) {
    auto encoding = Nan::Encoding::UCS2;
    std::vector<WCHAR> chars(Nan::DecodeBytes(value, encoding) / sizeof(WCHAR) + 1);
    Nan::DecodeWrite((char*)chars.data(), chars.size() * sizeof(WCHAR), value, encoding);
    return chars;
}

void throw_win32(const char* msg, DWORD status) {
    auto error = Nan::Error(msg);
    Nan::Set(
        error.As<v8::Object>(),
        Nan::New("win32_error").ToLocalChecked().As<v8::Value>(),
        Nan::New((uint32_t) status));
    Nan::ThrowError(error);
}

NAN_METHOD(openKey) {
    auto hkey = to_hkey(info[0]);
    auto subKey = to_ucs2(info[1]);
    auto options = info[2]->Uint32Value();
    auto access = info[3]->Uint32Value();

    HKEY hSubKey = 0;
    auto status = RegOpenKeyExW(
        hkey,
        subKey.data(),
        options,
        access,
        &hSubKey);

    if (status == ERROR_FILE_NOT_FOUND) {
        info.GetReturnValue().SetNull();
        return;
    }

    if (status != ERROR_SUCCESS) {
        throw_win32("RegOpenKeyExW failed", status);
        return;
    }

    info.GetReturnValue().Set((uint32_t) reinterpret_cast<size_t>(hSubKey));
}

NAN_METHOD(createKey) {
    auto hkey = to_hkey(info[0]);
    auto subKey = to_ucs2(info[1]);
    auto options = info[2]->Uint32Value();
    auto access = info[3]->Uint32Value();

    HKEY hSubKey = 0;
    auto status = RegCreateKeyExW(
        hkey,
        subKey.data(),
        0, // reserved,
        NULL,
        options,
        access,
        NULL, // security attributes
        &hSubKey,
        NULL); // disposition (created / existing)

    if (status != ERROR_SUCCESS) {
        throw_win32("RegCreateKeyExW failed", status);
        return;
    }

    info.GetReturnValue().Set((uint32_t) reinterpret_cast<size_t>(hSubKey));
}

NAN_METHOD(enumKeyNames) {
    auto hkey = to_hkey(info[0]);

    // Get max key length (in WCHARs, not including \0)
    DWORD max_length = 0;
    auto status = RegQueryInfoKeyW(
        hkey,
        NULL, // class
        NULL, // class length
        NULL, // reserved
        NULL, // key count (using enum status instead)
        &max_length,
        NULL, // max class length
        NULL, // values count
        NULL, // max value name length
        NULL, // max value length
        NULL, // security descriptor size
        NULL); // last write time
    if (status != ERROR_SUCCESS) {
        throw_win32("RegQueryInfoKeyW failed", status);
        return;
    }

    std::vector<uint16_t> data(max_length + 1);

    auto result = Nan::New<v8::Array>();

    for (uint32_t index = 0; ; index++) {
        DWORD length = data.size();
        status = RegEnumKeyExW(
            hkey,
            index,
            (LPWSTR) data.data(),
            &length,
            NULL,
            NULL, // class
            NULL, // class length
            NULL); // last write time
        if (status == ERROR_NO_MORE_ITEMS) {
            break;
        }
        if (status != ERROR_SUCCESS) {
            throw_win32("RegEnumKeyExW failed", status);
            return;
        }

        auto item = Nan::New(data.data(), length).ToLocalChecked();
        Nan::Set(result, index, item.As<v8::Object>());
    }

    info.GetReturnValue().Set(result);
}

NAN_METHOD(enumValueNames) {
    auto hkey = to_hkey(info[0]);

    // Get max value name length (in WCHARs, not including \0)
    DWORD max_name_length = 0;
    auto status = RegQueryInfoKeyW(
        hkey,
        NULL, // class
        NULL, // class length
        NULL, // reserved
        NULL, // key count (using enum status instead)
        NULL, // max key length
        NULL, // max class length
        NULL, // values count
        &max_name_length, // max value name length
        NULL, // max value length
        NULL, // security descriptor size
        NULL); // last write time
    if (status != ERROR_SUCCESS) {
        throw_win32("RegQueryInfoKeyW failed", status);
        return;
    }

    std::vector<uint16_t> name_data(max_name_length + 1);

    auto result = Nan::New<v8::Array>();

    for (uint32_t index = 0; ; index++) {
        DWORD name_length = name_data.size();
        status = RegEnumValueW(
            hkey,
            index,
            (LPWSTR) name_data.data(),
            &name_length,
            NULL, // reserved
            NULL, // type
            NULL, // data
            NULL); // data size
        if (status == ERROR_NO_MORE_ITEMS) {
            break;
        }
        if (status != ERROR_SUCCESS) {
            throw_win32("RegEnumValueW failed", status);
            return;
        }

        auto item = Nan::New(name_data.data(), name_length).ToLocalChecked();
        Nan::Set(result, index, item.As<v8::Object>());
    }

    info.GetReturnValue().Set(result);
}

NAN_METHOD(queryValue) {
    auto hkey = to_hkey(info[0]);
    auto valueName = to_ucs2(info[1]);
    DWORD type = 0;
    DWORD size = 0;
    // Query size, type
    auto status = RegQueryValueExW(hkey, valueName.data(), NULL, &type, NULL, &size);

    if (status == ERROR_FILE_NOT_FOUND) {
        info.GetReturnValue().SetNull();
        return;
    }

    if (status != ERROR_SUCCESS) {
        throw_win32("RegQueryValueExW failed", status);
        return;
    }

    auto data = Nan::NewBuffer(size).ToLocalChecked();
    status = RegQueryValueExW(
        hkey,
        valueName.data(),
        NULL,
        NULL,
        (LPBYTE) node::Buffer::Data(data),
        &size);
    Nan::Set(
        data.As<v8::Object>(),
        Nan::New("type").ToLocalChecked().As<v8::Value>(),
        Nan::New((uint32_t) type));
    info.GetReturnValue().Set(data);
}

NAN_METHOD(getValue) {
    auto hkey = to_hkey(info[0]);
    auto subKey = to_ucs2(info[1]);
    auto valueName = to_ucs2(info[2]);
    auto flags = info[3]->Uint32Value();
    DWORD type = 0;
    DWORD size = 0;
    // Query size, type
    auto status = RegGetValueW(
        hkey,
        subKey.data(),
        valueName.data(),
        flags,
        &type,
        NULL,
        &size);

    if (status == ERROR_FILE_NOT_FOUND) {
        info.GetReturnValue().SetNull();
        return;
    }

    if (status != ERROR_SUCCESS) {
        throw_win32("RegGetValueW failed", status);
        return;
    }

    auto data = Nan::NewBuffer(size).ToLocalChecked();
    status = RegGetValueW(
        hkey,
        subKey.data(),
        valueName.data(),
        flags,
        NULL,
        node::Buffer::Data(data),
        &size);

    // Initial RegGetValueW() will pessimistically guess it needs to
    // add an extra \0 to the value for REG_SZ etc. for the initial call.
    // When this isn't needed, slice the extra bytes off:
    if (size < node::Buffer::Length(data)) {
        v8::Local<v8::Value> args[] = {
            Nan::New(0),
            Nan::New((uint32_t) size),
        };
        data = Nan::To<v8::Object>(Nan::Call("slice", data, 2, args).ToLocalChecked()).ToLocalChecked();
    }

    Nan::Set(
        data.As<v8::Object>(),
        Nan::New("type").ToLocalChecked().As<v8::Value>(),
        Nan::New((uint32_t) type));

    info.GetReturnValue().Set(data);
}

NAN_METHOD(setValue) {
    auto hkey = to_hkey(info[0]);
    auto valueName = to_ucs2(info[1]);
    auto valueType = info[2]->Uint32Value();
    auto data = info[3];

    auto status = RegSetValueExW(
        hkey,
        valueName.data(),
        NULL,
        valueType,
        (const BYTE*) node::Buffer::Data(data),
        node::Buffer::Length(data));

    if (status != ERROR_SUCCESS) {
        throw_win32("RegSetValueExW failed", status);
        return;
    }
}

NAN_METHOD(deleteTree) {
    auto hkey = to_hkey(info[0]);
    auto subKey = to_ucs2(info[1]);

    auto status = RegDeleteTreeW(hkey, subKey.data());

    if (status != ERROR_SUCCESS) {
        throw_win32("RegDeleteTreeW failed", status);
        return;
    }
}

NAN_METHOD(deleteKey) {
    auto hkey = to_hkey(info[0]);
    auto subKey = to_ucs2(info[1]);

    auto status = RegDeleteKeyW(hkey, subKey.data());

    if (status != ERROR_SUCCESS) {
        throw_win32("RegDeleteKeyW failed", status);
        return;
    }
}

NAN_METHOD(deleteKeyValue) {
    auto hkey = to_hkey(info[0]);
    auto subKey = to_ucs2(info[1]);
    auto valueName = to_ucs2(info[2]);

    auto status = RegDeleteKeyValueW(hkey, subKey.data(), valueName.data());

    if (status != ERROR_SUCCESS) {
        throw_win32("RegDeleteKeyValueW failed", status);
        return;
    }
}

NAN_METHOD(deleteValue) {
    auto hkey = to_hkey(info[0]);
    auto valueName = to_ucs2(info[1]);

    auto status = RegDeleteValueW(hkey, valueName.data());

    if (status != ERROR_SUCCESS) {
        throw_win32("RegDeleteValueW failed", status);
        return;
    }
}

NAN_METHOD(closeKey) {
    auto hkey = to_hkey(info[0]);
    auto status = RegCloseKey(hkey);
    if (status != ERROR_SUCCESS && status != ERROR_FILE_NOT_FOUND) {
        throw_win32("RegCloseKeyW failed", status);
        return;
    }
}

NAN_MODULE_INIT(Init) {
    NAN_EXPORT(target, openKey);
    NAN_EXPORT(target, createKey);
    NAN_EXPORT(target, enumKeyNames);
    NAN_EXPORT(target, enumValueNames);
    NAN_EXPORT(target, queryValue);
    NAN_EXPORT(target, getValue);
    NAN_EXPORT(target, setValue);
    NAN_EXPORT(target, deleteTree);
    NAN_EXPORT(target, deleteKey);
    NAN_EXPORT(target, deleteKeyValue);
    NAN_EXPORT(target, deleteValue);
    NAN_EXPORT(target, closeKey);
}

NODE_MODULE(fastreg, Init);
