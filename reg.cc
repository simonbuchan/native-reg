#include <nan.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <vector>

HKEY to_hkey(v8::Local<v8::Value> value) {
    if (value->IsExternal()) {
        return static_cast<HKEY>(value.As<v8::External>()->Value());
    }
    return reinterpret_cast<HKEY>((size_t) value->Uint32Value());
}

struct string_value : v8::String::Value {
    string_value(explicit v8::Local<v8::Value> value)
#if NODE_MODULE_VERSION >= NODE_8_0_NODE_MODULE_VERSION
        : v8::String::Value(v8::Isolate::GetCurrent(), value) {}
#else
        : v8::String::Value(value) {}
#endif

    LPWSTR operator*() { return reinterpret_cast<LPWSTR>(this->v8::String::Value::operator *()); }
};

void throw_win32(DWORD status, const char* syscall) {
    Nan::ThrowError(node::WinapiErrnoException(v8::Isolate::GetCurrent(), status, syscall));
}

NAN_METHOD(openKey) {
    auto hkey = to_hkey(info[0]);
    string_value subKey(info[1]);
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
        return info.GetReturnValue().SetNull();
    }

    if (status != ERROR_SUCCESS) {
        return throw_win32(status, "RegOpenKeyExW");
    }

    info.GetReturnValue().Set(Nan::New<v8::External>(hSubKey));
}

NAN_METHOD(createKey) {
    auto hkey = to_hkey(info[0]);
    string_value subKey(info[1]);
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
        return throw_win32(status, "RegCreateKeyExW");
    }

    info.GetReturnValue().Set(Nan::New<v8::External>(hSubKey));
}

NAN_METHOD(openCurrentUser) {
    auto access = info[1]->Uint32Value();

    HKEY hkey = NULL;
    auto status = RegOpenCurrentUser(access, &hkey);

    if (status != ERROR_SUCCESS) {
        return throw_win32(status, "RegOpenCurrentUser");
    }

    info.GetReturnValue().Set(Nan::New<v8::External>(hkey));
}

NAN_METHOD(loadAppKey) {
    string_value file(info[0]);
    auto access = info[1]->Uint32Value();

    HKEY hkey = NULL;
    auto status = RegLoadAppKeyW(
        file.data(),
        &hkey,
        access,
        0,
        0);

    if (status != ERROR_SUCCESS) {
        return throw_win32(status, "RegLoadAppKeyW");
    }

    info.GetReturnValue().Set(Nan::New<v8::External>(hkey));
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
        return throw_win32(status, "RegQueryInfoKeyW");
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
            return throw_win32(status, "RegEnumKeyExW");
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
        return throw_win32(status, "RegQueryInfoKeyW");
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
            return throw_win32(status, "RegEnumValueW");
        }

        auto item = Nan::New(name_data.data(), name_length).ToLocalChecked();
        Nan::Set(result, index, item.As<v8::Object>());
    }

    info.GetReturnValue().Set(result);
}

NAN_METHOD(queryValue) {
    auto hkey = to_hkey(info[0]);
    string_value valueName(info[1]);
    DWORD type = 0;
    DWORD size = 0;
    // Query size, type
    auto status = RegQueryValueExW(hkey, valueName.data(), NULL, &type, NULL, &size);

    if (status == ERROR_FILE_NOT_FOUND) {
        return info.GetReturnValue().SetNull();
    }

    if (status != ERROR_SUCCESS) {
        return throw_win32(status, "RegQueryValueExW");
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
    string_value subKey(info[1]);
    string_value valueName(info[2]);
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
        return info.GetReturnValue().SetNull();
    }

    if (status != ERROR_SUCCESS) {
        return throw_win32(status, "RegGetValueW");
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
    string_value valueName(info[1]);
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
        return throw_win32(status, "RegSetValueExW");
    }
}

NAN_METHOD(deleteTree) {
    auto hkey = to_hkey(info[0]);
    string_value subKey(info[1]);

    auto status = RegDeleteTreeW(hkey, subKey.data());

    if (status == ERROR_FILE_NOT_FOUND) {
        return info.GetReturnValue().Set(false);
    }

    if (status != ERROR_SUCCESS) {
        return throw_win32(status, "RegDeleteTreeW");
    }

    info.GetReturnValue().Set(true);
}

NAN_METHOD(deleteKey) {
    auto hkey = to_hkey(info[0]);
    string_value subKey(info[1]);

    auto status = RegDeleteKeyW(hkey, subKey.data());

    if (status == ERROR_FILE_NOT_FOUND) {
        return info.GetReturnValue().Set(false);
    }

    if (status != ERROR_SUCCESS) {
        return throw_win32(status, "RegDeleteKeyW");
    }

    info.GetReturnValue().Set(true);
}

NAN_METHOD(deleteKeyValue) {
    auto hkey = to_hkey(info[0]);
    string_value subKey(info[1]);
    string_value valueName(info[2]);

    auto status = RegDeleteKeyValueW(hkey, subKey.data(), valueName.data());

    if (status == ERROR_FILE_NOT_FOUND) {
        return info.GetReturnValue().Set(false);
    }

    if (status != ERROR_SUCCESS) {
        return throw_win32(status, "RegDeleteKeyValueW");
    }

    info.GetReturnValue().Set(true);
}

NAN_METHOD(deleteValue) {
    auto hkey = to_hkey(info[0]);
    string_value valueName(info[1]);

    auto status = RegDeleteValueW(hkey, valueName.data());

    if (status == ERROR_FILE_NOT_FOUND) {
        return info.GetReturnValue().Set(false);
    }

    if (status != ERROR_SUCCESS) {
        return throw_win32(status, "RegDeleteValueW");
    }

    info.GetReturnValue().Set(true);
}

NAN_METHOD(closeKey) {
    auto hkey = to_hkey(info[0]);
    auto status = RegCloseKey(hkey);

    if (status != ERROR_SUCCESS) {
        return throw_win32(status, "RegCloseKeyW");
    }
}

NAN_MODULE_INIT(Init) {
    NAN_EXPORT(target, openKey);
    NAN_EXPORT(target, createKey);
    NAN_EXPORT(target, openCurrentUser);
    NAN_EXPORT(target, loadAppKey);
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
