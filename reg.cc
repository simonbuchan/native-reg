#include <napi.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <vector>

#define wstr(str) reinterpret_cast<LPCWSTR>(str.Utf16Value().c_str())

#define NAPI_TRUE Napi::Boolean::New(env, true)
#define NAPI_FALSE Napi::Boolean::New(env, false)

HKEY to_hkey(Napi::Value value)
{
	if (value.IsExternal())
	{
		return static_cast<HKEY>(value.As<Napi::External<void>>().Data());
	}

	return reinterpret_cast<HKEY>((size_t)value.As<Napi::Number>().Uint32Value());
}

void throw_win32(const Napi::Env &env, DWORD status, const char *syscall)
{
	Napi::Error error;

	char *errmsg = nullptr;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		status,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&errmsg, 0, nullptr);

	if (errmsg)
	{
		// Remove trailing newlines
		for (int i = strlen(errmsg) - 1; 
			i >= 0 && (errmsg[i] == '\n' || errmsg[i] == '\r'); 
			i--)
		{
			errmsg[i] = '\0';
		}

		error = Napi::Error::New(env, errmsg);
		::LocalFree(errmsg);
	}
	else
	{
		error = Napi::Error::New(env, "Unknown Error");
	}

	error.Set("errno", Napi::Number::New(env, status));
	error.Set("syscall", syscall);

	error.ThrowAsJavaScriptException();
}

Napi::Value openKey(const Napi::CallbackInfo &info)
{
	auto const &env = info.Env();

	auto hkey = to_hkey(info[0]);
	auto subKey = info[1].ToString();
	auto options = info[2].As<Napi::Number>().Uint32Value();
	auto access = info[3].As<Napi::Number>().Uint32Value();

	HKEY hSubKey = 0;
	auto status = RegOpenKeyExW(
		hkey,
		wstr(subKey),
		options,
		access,
		&hSubKey);

	if (status == ERROR_FILE_NOT_FOUND)
	{
		return info.Env().Null();
	}

	if (status != ERROR_SUCCESS)
	{
		throw_win32(env, status, "RegOpenKeyExW");
		return env.Undefined();
	}

	return Napi::External<void>::New(env, hSubKey);
}

Napi::Value createKey(const Napi::CallbackInfo &info)
{
	auto const &env = info.Env();

	auto hkey = to_hkey(info[0]);
	auto subKey = info[1].ToString();
	auto options = info[2].As<Napi::Number>().Uint32Value();
	auto access = info[3].As<Napi::Number>().Uint32Value();

	HKEY hSubKey = 0;
	auto status = RegCreateKeyExW(
		hkey,
		wstr(subKey),
		0, // reserved,
		NULL,
		options,
		access,
		NULL, // security attributes
		&hSubKey,
		NULL); // disposition (created / existing)

	if (status != ERROR_SUCCESS)
	{
		throw_win32(env, status, "RegCreateKeyExW");
		return env.Undefined();
	}

	return Napi::External<void>::New(env, hSubKey);
}

Napi::Value openCurrentUser(const Napi::CallbackInfo &info)
{
	auto const &env = info.Env();

	auto access = info[1].As<Napi::Number>().Uint32Value();

	HKEY hkey = NULL;
	auto status = RegOpenCurrentUser(access, &hkey);

	if (status != ERROR_SUCCESS)
	{
		throw_win32(env, status, "RegOpenCurrentUser");
		return env.Undefined();
	}

	return Napi::External<void>::New(env, hkey);
}

Napi::Value loadAppKey(const Napi::CallbackInfo &info)
{
	auto const &env = info.Env();

	auto file = info[0].ToString();
	auto access = info[1].As<Napi::Number>().Uint32Value();

	HKEY hkey = NULL;
	auto status = RegLoadAppKeyW(
		wstr(file),
		&hkey,
		access,
		0,
		0);

	if (status != ERROR_SUCCESS)
	{
		throw_win32(env, status, "RegLoadAppKeyW");
		return env.Undefined();
	}

	return Napi::External<void>::New(env, hkey);
}

Napi::Value enumKeyNames(const Napi::CallbackInfo &info)
{
	auto const &env = info.Env();

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
	if (status != ERROR_SUCCESS)
	{
		throw_win32(env, status, "RegQueryInfoKeyW");
		return env.Undefined();
	}

	std::vector<uint16_t> data(max_length + 1);

	auto result = Napi::Array::New(env);

	for (uint32_t index = 0; ; index++)
	{
		DWORD length = data.size();
		status = RegEnumKeyExW(
			hkey,
			index,
			(LPWSTR)data.data(),
			&length,
			NULL,
			NULL, // class
			NULL, // class length
			NULL); // last write time

		if (status == ERROR_NO_MORE_ITEMS)
		{
			break;
		}

		if (status != ERROR_SUCCESS)
		{
			throw_win32(env, status, "RegEnumKeyExW");
			return env.Undefined();
		}

		auto item = Napi::Uint16Array::New(env, length);
		for (DWORD i = 0; i < length; i++)
			item.Set(i, data[i]);

		result.Set(index, item);
	}

	return result;
}

Napi::Value enumValueNames(const Napi::CallbackInfo &info)
{
	auto const &env = info.Env();

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
	if (status != ERROR_SUCCESS)
	{
		throw_win32(env, status, "RegQueryInfoKeyW");
		return env.Undefined();
	}

	std::vector<uint16_t> name_data(max_name_length + 1);

	auto result = Napi::Array::New(env);

	for (uint32_t index = 0; ; index++)
	{
		DWORD name_length = name_data.size();
		status = RegEnumValueW(
			hkey,
			index,
			(LPWSTR)name_data.data(),
			&name_length,
			NULL, // reserved
			NULL, // type
			NULL, // data
			NULL); // data size
		if (status == ERROR_NO_MORE_ITEMS)
		{
			break;
		}
		if (status != ERROR_SUCCESS)
		{
			throw_win32(env, status, "RegEnumValueW");
			return env.Undefined();
		}

		auto item = Napi::Uint16Array::New(env, name_length);
		for (DWORD i = 0; i < name_length; i++)
			item.Set(i, name_data[i]);

		result.Set(index, item);
	}

	return result;
}

Napi::Value queryValue(const Napi::CallbackInfo &info)
{
	auto const &env = info.Env();

	auto hkey = to_hkey(info[0]);
	auto valueName = info[1].ToString();
	DWORD type = 0;
	DWORD size = 0;

	// Query size, type
	auto status = RegQueryValueExW(hkey, wstr(valueName), NULL, &type, NULL, &size);

	if (status == ERROR_FILE_NOT_FOUND)
	{
		return info.Env().Null();
	}

	if (status != ERROR_SUCCESS)
	{
		throw_win32(env, status, "RegQueryValueExW");
		return env.Undefined();
	}

	auto data = Napi::Buffer<char>::New(env, size);
	status = RegQueryValueExW(
		hkey,
		wstr(valueName),
		NULL,
		NULL,
		(LPBYTE)data.As<Napi::Buffer<char>>().Data(),
		&size);

	data.Set("type", Napi::Number::New(env, (uint32_t)type));

	return data;
}

Napi::Value getValue(const Napi::CallbackInfo &info)
{
	auto const &env = info.Env();

	auto hkey = to_hkey(info[0]);
	auto subKey = info[1].ToString();
	auto valueName = info[2].ToString();
	auto flags = info[3].As<Napi::Number>().Uint32Value();
	DWORD type = 0;
	DWORD size = 0;

	// Query size, type
	auto status = RegGetValueW(
		hkey,
		wstr(subKey),
		wstr(valueName),
		flags,
		&type,
		NULL,
		&size);

	if (status == ERROR_FILE_NOT_FOUND)
	{
		return info.Env().Null();
	}

	if (status != ERROR_SUCCESS)
	{
		throw_win32(env, status, "RegGetValueW");
		return env.Undefined();
	}

	auto data = Napi::Buffer<char>::New(env, size);
	status = RegGetValueW(
		hkey,
		wstr(subKey),
		wstr(valueName),
		flags,
		NULL,
		data.As<Napi::Buffer<char>>().Data(),
		&size);

	// Initial RegGetValueW() will pessimistically guess it needs to
	// add an extra \0 to the value for REG_SZ etc. for the initial call.
	// When this isn't needed, slice the extra bytes off:
	if (size < data.Length())
	{
		data = data.Get("slice").As<Napi::Function>().Call(
			data,
			{
				Napi::Number::New(env, 0),
				Napi::Number::New(env, (uint32_t)size),
			}
		).As<Napi::Buffer<char>>();
	}

	data.Set("type", Napi::Number::New(env, (uint32_t)type));

	return data;
}

Napi::Value setValue(const Napi::CallbackInfo &info)
{
	auto const &env = info.Env();

	auto hkey = to_hkey(info[0]);
	auto valueName = info[1].ToString();
	auto valueType = info[2].As<Napi::Number>().Uint32Value();
	auto data = info[3];

	auto status = RegSetValueExW(
		hkey,
		wstr(valueName),
		NULL,
		valueType,
		(const BYTE*)data.As<Napi::Buffer<char>>().Data(),
		data.As<Napi::Buffer<char>>().Length());

	if (status != ERROR_SUCCESS)
	{
		throw_win32(env, status, "RegSetValueExW");
	}

	return env.Undefined();
}

Napi::Value deleteTree(const Napi::CallbackInfo &info)
{
	auto const &env = info.Env();

	auto hkey = to_hkey(info[0]);
	auto subKey = info[1].ToString();

	auto status = RegDeleteTreeW(hkey, wstr(subKey));

	if (status == ERROR_FILE_NOT_FOUND)
	{
		return NAPI_FALSE;
	}

	if (status != ERROR_SUCCESS)
	{
		throw_win32(env, status, "RegDeleteTreeW");
		return env.Undefined();
	}

	return NAPI_TRUE;
}

Napi::Value deleteKey(const Napi::CallbackInfo &info)
{
	auto const &env = info.Env();

	auto hkey = to_hkey(info[0]);
	auto subKey = info[1].ToString();

	auto status = RegDeleteKeyW(hkey, wstr(subKey));

	if (status == ERROR_FILE_NOT_FOUND)
	{
		return NAPI_FALSE;
	}

	if (status != ERROR_SUCCESS)
	{
		throw_win32(env, status, "RegDeleteKeyW");
		return env.Undefined();
	}

	return NAPI_TRUE;
}

Napi::Value deleteKeyValue(const Napi::CallbackInfo &info)
{
	auto const &env = info.Env();

	auto hkey = to_hkey(info[0]);
	auto subKey = info[1].ToString();
	auto valueName = info[2].ToString();

	auto status = RegDeleteKeyValueW(hkey, wstr(subKey), wstr(valueName));

	if (status == ERROR_FILE_NOT_FOUND)
	{
		return NAPI_FALSE;
	}

	if (status != ERROR_SUCCESS)
	{
		throw_win32(env, status, "RegDeleteKeyValueW");
		return env.Undefined();
	}

	return NAPI_TRUE;
}

Napi::Value deleteValue(const Napi::CallbackInfo &info)
{
	auto const &env = info.Env();

	auto hkey = to_hkey(info[0]);
	auto valueName = info[1].ToString();

	auto status = RegDeleteValueW(hkey, wstr(valueName));

	if (status == ERROR_FILE_NOT_FOUND)
	{
		return NAPI_FALSE;
	}

	if (status != ERROR_SUCCESS)
	{
		throw_win32(env, status, "RegDeleteValueW");
		return env.Undefined();
	}

	return NAPI_TRUE;
}

Napi::Value closeKey(const Napi::CallbackInfo &info)
{
	auto const &env = info.Env();

	auto hkey = to_hkey(info[0]);
	auto status = RegCloseKey(hkey);

	if (status != ERROR_SUCCESS)
	{
		throw_win32(env, status, "RegCloseKeyW");
		return env.Undefined();
	}

	return env.Undefined();
}

#define NAPI_EXPORT(target, function) target[#function] = Napi::Function::New(env, function, #function);

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
	Napi::HandleScope scope(env);

	NAPI_EXPORT(exports, openKey);
	NAPI_EXPORT(exports, createKey);
	NAPI_EXPORT(exports, openCurrentUser);
	NAPI_EXPORT(exports, loadAppKey);
	NAPI_EXPORT(exports, enumKeyNames);
	NAPI_EXPORT(exports, enumValueNames);
	NAPI_EXPORT(exports, queryValue);
	NAPI_EXPORT(exports, getValue);
	NAPI_EXPORT(exports, setValue);
	NAPI_EXPORT(exports, deleteTree);
	NAPI_EXPORT(exports, deleteKey);
	NAPI_EXPORT(exports, deleteKeyValue);
	NAPI_EXPORT(exports, deleteValue);
	NAPI_EXPORT(exports, closeKey);

	return exports;
}

NODE_API_MODULE(fastreg, Init);
