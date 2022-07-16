#include <napi.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <vector>

#ifndef NAPI_CPP_EXCEPTIONS
#error NAPI_CPP_EXCEPTIONS is required for this code.
#endif

using namespace Napi;

std::wstring to_wstring(Value value) {
    size_t length;
    napi_status status = napi_get_value_string_utf16(value.Env(), value, nullptr, 0, &length);
    NAPI_THROW_IF_FAILED_VOID(value.Env(), status);

    std::wstring result;
    result.reserve(length + 1);
    result.resize(length);
    status = napi_get_value_string_utf16(
        value.Env(),
        value,
        reinterpret_cast<char16_t*>(&result[0]),
        result.capacity(),
        nullptr);
    NAPI_THROW_IF_FAILED_VOID(value.Env(), status);
    return result;
}

napi_status try_napi_unwrap(napi_env env, napi_value js_object, void** result) {
    napi_status status = napi_unwrap(env, js_object, result);
    if (status == napi_invalid_arg) {
        *result = nullptr;
        return napi_ok;
    }
    return status;
}

HKEY to_hkey(Value value) {
    if (value.IsExternal()) {
        return static_cast<HKEY>(value.As<External<void>>().Data());
    }
    if (value.IsObject()) {
        void* hkey;
        napi_status status = try_napi_unwrap(value.Env(), value, &hkey);
        NAPI_THROW_IF_FAILED_VOID(value.Env(), status);
        if (!hkey) {
            throw Napi::Error::New(value.Env(), "HKEY already closed");
        }
        return (HKEY)hkey;
    }
    return reinterpret_cast<HKEY>((size_t) value.As<Number>().Uint32Value());
}

Error win32_error(Env env, DWORD code, const char* syscall) {
    Value message_value;

    char16_t* format_message = nullptr;
    ::FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        code,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&format_message), // due to FORMAT_MESSAGE_ALLOCATE_BUFFER flag
        0,
        nullptr);
    if (format_message) {
        auto message = std::u16string{ format_message };
        ::LocalFree(format_message);

        // Remove trailing newlines
        message.erase(message.find_last_not_of(u"\r\n"));
        message_value = String::New(env, message);
    } else {
        message_value = String::New(env, u"Unknown Error");
    }

    // node-addon-api Error::New() doesn't take Napi::Value or utf-16 messages.
    napi_value raw_error;
    auto status = napi_create_error(env, String::New(env, u"Win32Error"), message_value, &raw_error);
    NAPI_THROW_IF_FAILED_VOID(env, status);

    auto error = Error(env, raw_error);
    error.Set("errno", Number::New(env, code));
    error.Set("syscall", syscall);
    return error;
}

void hkey_finalizer(napi_env env, void* data, void* hint) {
    RegCloseKey((HKEY)data);
}

static napi_ref hkey_constructor_ref;

Value wrap_hkey(Env env, HKEY hkey) {
    napi_value ctor;
    napi_status status = napi_get_reference_value(env, hkey_constructor_ref, &ctor);
    NAPI_THROW_IF_FAILED_VOID(env, status);
    napi_value hkey_value;
    status = napi_create_external(env, hkey, nullptr, nullptr, &hkey_value);
    NAPI_THROW_IF_FAILED_VOID(env, status);
    napi_value result;
    status = napi_new_instance(env, ctor, 1, &hkey_value, &result);
    NAPI_THROW_IF_FAILED_VOID(env, status);
    return Value(env, result);
}

Value openKey(const CallbackInfo& info) {
    auto env = info.Env();

    auto hkey = to_hkey(info[0]);
    auto subKey = to_wstring(info[1]);
    auto options = info[2].As<Number>().Uint32Value();
    auto access = info[3].As<Number>().Uint32Value();

    HKEY hSubKey = 0;
    auto status = RegOpenKeyExW(
        hkey,
        subKey.c_str(),
        options,
        access,
        &hSubKey);

    if (status == ERROR_FILE_NOT_FOUND) {
        return env.Null();
    }

    if (status != ERROR_SUCCESS) {
        throw win32_error(env, status, "RegOpenKeyExW");
    }

    return wrap_hkey(env, hSubKey);
}

Value createKey(const CallbackInfo& info) {
    auto env = info.Env();

    auto hkey = to_hkey(info[0]);
    auto subKey = to_wstring(info[1]);
    auto options = info[2].As<Number>().Uint32Value();
    auto access = info[3].As<Number>().Uint32Value();

    HKEY hSubKey = 0;
    auto status = RegCreateKeyExW(
        hkey,
        subKey.c_str(),
        0, // reserved,
        NULL,
        options,
        access,
        NULL, // security attributes
        &hSubKey,
        NULL); // disposition (created / existing)

    if (status != ERROR_SUCCESS) {
        throw win32_error(env, status, "RegCreateKeyExW");
    }

    return wrap_hkey(env, hSubKey);
}

Value openCurrentUser(const CallbackInfo& info) {
    auto env = info.Env();

    auto access = info[1].As<Number>().Uint32Value();

    HKEY hkey = NULL;
    auto status = RegOpenCurrentUser(access, &hkey);

    if (status != ERROR_SUCCESS) {
        throw win32_error(env, status, "RegOpenCurrentUser");
    }

    return wrap_hkey(env, hkey);
}

Value loadAppKey(const CallbackInfo& info) {
    auto env = info.Env();

    auto file = to_wstring(info[0]);
    auto access = info[1].As<Number>().Uint32Value();

    HKEY hkey = NULL;
    auto status = RegLoadAppKeyW(
        file.c_str(),
        &hkey,
        access,
        0,
        0);

    if (status != ERROR_SUCCESS) {
        throw win32_error(env, status, "RegLoadAppKeyW");
    }

    return wrap_hkey(env, hkey);
}

Value enumKeyNames(const CallbackInfo& info) {
    auto env = info.Env();

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
        throw win32_error(env, status, "RegQueryInfoKeyW");
    }

    std::u16string data;
    data.reserve(max_length + 1);
    data.resize(max_length);

    auto result = Array::New(env);

    for (uint32_t index = 0; ; index++) {
        DWORD length = data.capacity();
        status = RegEnumKeyExW(
            hkey,
            index,
            reinterpret_cast<LPWSTR>(&data[0]),
            &length,
            NULL,
            NULL, // class
            NULL, // class length
            NULL); // last write time

        if (status == ERROR_NO_MORE_ITEMS) {
            break;
        }

        if (status != ERROR_SUCCESS) {
            throw win32_error(env, status, "RegEnumKeyExW");
        }

        result[index] = String::New(env, data.data(), length);
    }

    return result;
}

Value enumValueNames(const CallbackInfo& info) {
    auto env = info.Env();

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
        throw win32_error(env, status, "RegQueryInfoKeyW");
    }

    std::u16string name_data;
    name_data.reserve(max_name_length + 1);
    name_data.resize(max_name_length);

    auto result = Array::New(env);

    for (uint32_t index = 0; ; index++) {
        DWORD name_length = name_data.capacity();
        status = RegEnumValueW(
            hkey,
            index,
            reinterpret_cast<LPWSTR>(&name_data[0]),
            &name_length,
            NULL, // reserved
            NULL, // type
            NULL, // data
            NULL); // data size
        if (status == ERROR_NO_MORE_ITEMS) {
            break;
        }
        if (status != ERROR_SUCCESS) {
            throw win32_error(env, status, "RegEnumValueW");
        }

        result[index] = String::New(env, name_data.data(), name_length);
    }

    return result;
}

Value queryValue(const CallbackInfo& info) {
    auto env = info.Env();

    auto hkey = to_hkey(info[0]);
    auto valueName = to_wstring(info[1]);
    DWORD type = 0;
    DWORD size = 0;

    // Query size, type
    auto status = RegQueryValueExW(hkey, valueName.c_str(), NULL, &type, NULL, &size);

    if (status == ERROR_FILE_NOT_FOUND) {
        return env.Null();
    }

    if (status != ERROR_SUCCESS) {
        throw win32_error(env, status, "RegQueryValueExW");
    }

    auto data = Buffer<BYTE>::New(env, size);
    status = RegQueryValueExW(
        hkey,
        valueName.c_str(),
        NULL,
        NULL,
        data.Data(),
        &size);

    if (status != ERROR_SUCCESS) {
        throw win32_error(env, status, "RegQueryValueExW");
    }

    data.Set("type", Number::New(env, type));

    return data;
}

Value getValue(const CallbackInfo& info) {
    auto env = info.Env();

    auto hkey = to_hkey(info[0]);
    auto subKey = to_wstring(info[1]);
    auto valueName = to_wstring(info[2]);
    auto flags = info[3].As<Number>().Uint32Value();
    DWORD type = 0;
    DWORD size = 0;

    // Query size, type
    auto status = RegGetValueW(
        hkey,
        subKey.c_str(),
        valueName.c_str(),
        flags,
        &type,
        NULL,
        &size);

    if (status == ERROR_FILE_NOT_FOUND) {
        return env.Null();
    }

    if (status != ERROR_SUCCESS) {
        throw win32_error(env, status, "RegGetValueW");
    }

    auto data = Buffer<BYTE>::New(env, size);
    status = RegGetValueW(
        hkey,
        subKey.c_str(),
        valueName.c_str(),
        flags,
        NULL,
        data.Data(),
        &size);

    if (status != ERROR_SUCCESS) {
        throw win32_error(env, status, "RegGetValueW");
    }

    // Initial RegGetValueW() will pessimistically guess it needs to
    // add an extra \0 to the value for REG_SZ etc. for the initial call.
    // When this isn't needed, slice the extra bytes off:
    if (size < data.Length()) {
        data = Buffer<BYTE>::Copy(env, data.Data(), size);
    }

    data.Set("type", Number::New(env, type));

    return data;
}

void setValue(const CallbackInfo& info) {
    auto env = info.Env();

    auto hkey = to_hkey(info[0]);
    auto valueName = to_wstring(info[1]);
    auto valueType = info[2].As<Number>().Uint32Value();
    auto data = info[3].As<Buffer<BYTE>>();

    auto status = RegSetValueExW(
        hkey,
        valueName.c_str(),
        NULL,
        valueType,
        data.Data(),
        data.Length());

    if (status != ERROR_SUCCESS) {
        throw win32_error(env, status, "RegSetValueExW");
    }
}

void renameKey(const CallbackInfo& info) {
    auto env = info.Env();

    auto hkey = to_hkey(info[0]);
    auto subKey = to_wstring(info[1]);
    // only documented to accept NULL, not "" to rename hkey itself.
    auto subKeyPtr = subKey.empty() ? NULL : subKey.c_str();
    auto newSubKey = to_wstring(info[2]);

    auto status = RegRenameKey(hkey, subKeyPtr, newSubKey.c_str());

    if (status != ERROR_SUCCESS) {
        throw win32_error(env, status, "RegRenameKey");
    }
}

void copyTree(const CallbackInfo& info) {
    auto env = info.Env();

    auto hkeySrc = to_hkey(info[0]);
    auto subKey = to_wstring(info[1]);
    // only documented to accept NULL, not "" to copy hkey itself.
    auto subKeyPtr = subKey.empty() ? NULL : subKey.c_str();
    auto hkeyDest = to_hkey(info[2]);

    auto status = RegCopyTreeW(hkeySrc, subKeyPtr, hkeyDest);

    if (status != ERROR_SUCCESS) {
        throw win32_error(env, status, "RegCopyTreeW");
    }
}

Value deleteTree(const CallbackInfo& info) {
    auto env = info.Env();

    auto hkey = to_hkey(info[0]);
    auto subKey = to_wstring(info[1]);
    // only documented to accept NULL, not "" to delete own children.
    auto subKeyPtr = subKey.empty() ? NULL : subKey.c_str();

    auto status = RegDeleteTreeW(hkey, subKeyPtr);

    if (status == ERROR_FILE_NOT_FOUND) {
        return Boolean::New(env, false);
    }

    if (status != ERROR_SUCCESS) {
        throw win32_error(env, status, "RegDeleteTreeW");
    }

    return Boolean::New(env, true);
}

Value deleteKey(const CallbackInfo& info) {
    auto env = info.Env();

    auto hkey = to_hkey(info[0]);
    auto subKey = to_wstring(info[1]);

    auto status = RegDeleteKeyW(hkey, subKey.c_str());

    if (status == ERROR_FILE_NOT_FOUND) {
        return Boolean::New(env, false);
    }

    if (status != ERROR_SUCCESS) {
        throw win32_error(env, status, "RegDeleteKeyW");
    }

    return Boolean::New(env, true);
}

Value deleteKeyValue(const CallbackInfo& info) {
    auto env = info.Env();

    auto hkey = to_hkey(info[0]);
    auto subKey = to_wstring(info[1]);
    auto valueName = to_wstring(info[2]);

    auto status = RegDeleteKeyValueW(hkey, subKey.c_str(), valueName.c_str());

    if (status == ERROR_FILE_NOT_FOUND) {
        return Boolean::New(env, false);
    }

    if (status != ERROR_SUCCESS) {
        throw win32_error(env, status, "RegDeleteKeyValueW");
    }

    return Boolean::New(env, true);
}

Value deleteValue(const CallbackInfo& info) {
    auto env = info.Env();

    auto hkey = to_hkey(info[0]);
    auto valueName = to_wstring(info[1]);

    auto status = RegDeleteValueW(hkey, valueName.c_str());

    if (status == ERROR_FILE_NOT_FOUND) {
        return Boolean::New(env, false);
    }

    if (status != ERROR_SUCCESS) {
        throw win32_error(env, status, "RegDeleteValueW");
    }

    return Boolean::New(env, true);
}

void closeKey(const CallbackInfo& info) {
    auto env = info.Env();
    LSTATUS status = 0;

    if (info[0].IsObject()) {
        void* hkey;
        napi_status nstatus = napi_remove_wrap(env, info[0], &hkey);
        NAPI_THROW_IF_FAILED_VOID(env, nstatus);
        status = RegCloseKey((HKEY)hkey);
    }
    else {
        auto hkey = to_hkey(info[0]);
        status = RegCloseKey(hkey);
    }
    if (status != ERROR_SUCCESS) {
        throw win32_error(env, status, "RegCloseKeyW");
    }
}

// error handling for functions not called through the C++ wrapper
#define RAISE_IF_FAILED(env, expr) \
    if (const napi_status status = (expr); status != napi_ok) { \
        napi_throw_error(env, nullptr, "N-API error from " ## #expr); \
        return nullptr; \
    }

napi_value hkey_constructor(napi_env env, napi_callback_info info) {
    napi_value this_arg;
    napi_value hkey_value;
    size_t argc = 1;
    RAISE_IF_FAILED(env, napi_get_cb_info(env, info, &argc, &hkey_value, &this_arg, nullptr));
    void* hkey_ptr;
    RAISE_IF_FAILED(env, napi_get_value_external(env, hkey_value, &hkey_ptr));
    RAISE_IF_FAILED(env, napi_wrap(env, this_arg, hkey_ptr, &hkey_finalizer, nullptr, nullptr));
    return this_arg;
}

napi_value hkey_get_native(napi_env env, napi_callback_info info) {
    napi_value this_arg;
    RAISE_IF_FAILED(env, napi_get_cb_info(env, info, nullptr, nullptr, &this_arg, nullptr));
    void* hkey_ptr;
    RAISE_IF_FAILED(env, try_napi_unwrap(env, this_arg, &hkey_ptr));
    napi_value result;
    if (!hkey_ptr) {
        RAISE_IF_FAILED(env, napi_get_null(env, &result));
    } else {
        RAISE_IF_FAILED(env, napi_create_external(env, hkey_ptr, nullptr, nullptr, &result));
    }
    return result;
}

#define NAPI_DESCRIPTOR_FUNCTION(function) \
    Napi::PropertyDescriptor::Function(#function, function)

napi_value Init(napi_env env, napi_value exports) {
    napi_value hkey_class;
    napi_property_descriptor native_descriptor{};
    native_descriptor.attributes = napi_property_attributes::napi_enumerable;
    native_descriptor.utf8name = "native";
    native_descriptor.getter = hkey_get_native;
    RAISE_IF_FAILED(env, napi_define_class(
        env,
        "HKEY",
        NAPI_AUTO_LENGTH,
        &hkey_constructor,
        nullptr,
        1,
        &native_descriptor,
        &hkey_class));
    RAISE_IF_FAILED(env, napi_create_reference(env, hkey_class, 1, &hkey_constructor_ref));

    Object(env, exports).DefineProperties({
        Napi::PropertyDescriptor::Value("HKEY", hkey_class),
        NAPI_DESCRIPTOR_FUNCTION(openKey),
        NAPI_DESCRIPTOR_FUNCTION(createKey),
        NAPI_DESCRIPTOR_FUNCTION(openCurrentUser),
        NAPI_DESCRIPTOR_FUNCTION(loadAppKey),
        NAPI_DESCRIPTOR_FUNCTION(enumKeyNames),
        NAPI_DESCRIPTOR_FUNCTION(enumValueNames),
        NAPI_DESCRIPTOR_FUNCTION(queryValue),
        NAPI_DESCRIPTOR_FUNCTION(getValue),
        NAPI_DESCRIPTOR_FUNCTION(setValue),
        NAPI_DESCRIPTOR_FUNCTION(renameKey),
        NAPI_DESCRIPTOR_FUNCTION(copyTree),
        NAPI_DESCRIPTOR_FUNCTION(deleteTree),
        NAPI_DESCRIPTOR_FUNCTION(deleteKey),
        NAPI_DESCRIPTOR_FUNCTION(deleteKeyValue),
        NAPI_DESCRIPTOR_FUNCTION(deleteValue),
        NAPI_DESCRIPTOR_FUNCTION(closeKey),
    });

    return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init);
