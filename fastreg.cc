#include <nan.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <vector>

NAN_METHOD(open);
NAN_METHOD(create);
NAN_METHOD(close);

auto to_hkey(v8::Local<v8::Value> value) {
    return reinterpret_cast<HKEY>((size_t) value.As<v8::Uint32>()->Value());
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
        Nan::New("error").ToLocalChecked().As<v8::Value>(),
        Nan::New((uint32_t) status));
    Nan::ThrowError(error);
}

NAN_METHOD(open) {
    auto hkey = to_hkey(info[0]);
    auto subKey = to_ucs2(info[1]);
    HKEY hSubKey = 0;
    auto status = RegOpenKeyW(hkey, subKey.data(), &hSubKey);
    if (status == ERROR_SUCCESS) {
        info.GetReturnValue().Set((uint32_t) reinterpret_cast<size_t>(hSubKey));
    } else if (status != ERROR_FILE_NOT_FOUND) {
        throw_win32("RegOpenKeyW failed", status);
        return;
    }
}

NAN_METHOD(create) {
    auto hkey = to_hkey(info[0]);
    auto subKey = to_ucs2(info[1]);
    HKEY hSubKey = 0;
    auto status = RegCreateKeyW(hkey, subKey.data(), &hSubKey);
    if (status == ERROR_SUCCESS) {
        info.GetReturnValue().Set((uint32_t) reinterpret_cast<size_t>(hSubKey));
    } else if (status != ERROR_FILE_NOT_FOUND) {
        throw_win32("RegCreateKeyW failed", status);
        return;
    }
}

NAN_METHOD(query) {
    auto hkey = to_hkey(info[0]);
    auto valueName = to_ucs2(info[1]);
    DWORD type = 0;
    DWORD size = 0;
    // Query size, type
    auto status = RegQueryValueExW(hkey, valueName.data(), NULL, &type, NULL, &size);
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

NAN_METHOD(close) {
    auto hkey = to_hkey(info[0]);
    auto status = RegCloseKey(hkey);
    if (status != ERROR_SUCCESS && status != ERROR_FILE_NOT_FOUND) {
        throw_win32("RegCreateKeyW failed", status);
        return;
    }
}

NAN_MODULE_INIT(Init) {
    NAN_EXPORT(target, open);
    NAN_EXPORT(target, create);
    NAN_EXPORT(target, query);
    NAN_EXPORT(target, close);
}

NODE_MODULE(fastreg, Init);
