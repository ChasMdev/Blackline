#include <iostream>
#include <windows.h>
#include <string>
#include <winhttp.h>
#include "json.hpp"
#include <comdef.h>
#include <Wbemidl.h>
#include <wincrypt.h>
#include <iomanip>
#include <tlhelp32.h>
#include <fstream>
#include <algorithm>
#include <map>
#include <filesystem>
#include <shlwapi.h>
#include <shlobj.h>
#include <dpapi.h>
#include <wininet.h>
#include <wincrypt.h>
#include <Windows.h>
#include <iostream>
#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <Psapi.h>
#include "Injection.h"
#include "ICHooker.h"
#include "ntdlldefs.h"
#include "ThreadPool.h"
#include "WorkerFactory.h"
#include "Tools.h"
#include "SyscalCaller.h"
#include "main.h"
#include "Utils.h"

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Wininet.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "Ncrypt.lib")

#pragma region authentication
const std::wstring REG_PATH = L"Software\\Blackline";
const std::wstring REG_VALUE_NAME = L"Token";

std::wstring str_to_wstr(const std::string& str) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), nullptr, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], size_needed);
    return wstr;
}

std::wstring getTokenFromRegistry() {
    HKEY hKey;
    wchar_t buffer[256];
    DWORD bufferSize = sizeof(buffer);

    if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_PATH.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        throw std::runtime_error("Unable to read registry key for token.");

    if (RegGetValueW(hKey, nullptr, REG_VALUE_NAME.c_str(), RRF_RT_REG_SZ, nullptr, &buffer, &bufferSize) != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        throw std::runtime_error("Token not found in registry.");
    }

    RegCloseKey(hKey);
    return std::wstring(buffer);
}


std::string hashSHA256(const std::string& input) {
    HCRYPTPROV hProv = NULL;
    HCRYPTHASH hHash = NULL;
    BYTE hash[32]; // SHA-256 is 256 bits
    DWORD hashLen = sizeof(hash);
    std::ostringstream oss;

    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
        throw std::runtime_error("Failed to acquire crypto context.");

    if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
        CryptReleaseContext(hProv, 0);
        throw std::runtime_error("Failed to create SHA-256 hash.");
    }

    if (!CryptHashData(hHash, reinterpret_cast<const BYTE*>(input.c_str()), (DWORD)input.length(), 0)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        throw std::runtime_error("Failed to hash data.");
    }

    if (!CryptGetHashParam(hHash, HP_HASHVAL, hash, &hashLen, 0)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        throw std::runtime_error("Failed to get hash value.");
    }

    for (DWORD i = 0; i < hashLen; ++i)
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);

    return oss.str();
}

std::string queryWMIProperty(const BSTR wmiClass, const BSTR property) {
    HRESULT hres;
    IWbemLocator *pLoc = nullptr;
    IWbemServices *pSvc = nullptr;
    IEnumWbemClassObject* pEnumerator = nullptr;
    IWbemClassObject *pclsObj = nullptr;
    ULONG uReturn = 0;
    VARIANT vtProp;
    std::string result;

    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres))
        throw std::runtime_error("Failed to initialize COM.");

    hres = CoInitializeSecurity(NULL, -1, NULL, NULL,
        RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL, EOAC_NONE, NULL);
    if (FAILED(hres) && hres != RPC_E_TOO_LATE)
        throw std::runtime_error("Failed to initialize COM security.");

    hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, (LPVOID*)&pLoc);
    if (FAILED(hres))
        throw std::runtime_error("Failed to create IWbemLocator.");

    hres = pLoc->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"), nullptr, nullptr, 0, NULL, 0, 0, &pSvc);
    if (FAILED(hres))
        throw std::runtime_error("Failed to connect to WMI service.");

    hres = CoSetProxyBlanket(
        pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
        RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL, EOAC_NONE);
    if (FAILED(hres))
        throw std::runtime_error("Failed to set WMI proxy blanket.");

    hres = pSvc->ExecQuery(
        bstr_t("WQL"),
        _bstr_t(std::wstring(L"SELECT * FROM ").append(wmiClass).c_str()),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL, &pEnumerator);
    if (FAILED(hres))
        throw std::runtime_error("WMI query failed.");

    hres = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
    if (uReturn == 0)
        throw std::runtime_error("WMI returned no results.");

    hres = pclsObj->Get(property, 0, &vtProp, 0, 0);
    if (SUCCEEDED(hres) && vtProp.vt == VT_BSTR)
        result = _bstr_t(vtProp.bstrVal);

    VariantClear(&vtProp);
    pclsObj->Release();
    pSvc->Release();
    pLoc->Release();
    pEnumerator->Release();
    CoUninitialize();

    return result;
}

std::string getDriveSerial(const std::wstring& driveLetter = L"C:\\") {
    DWORD serialNumber = 0;
    if (!GetVolumeInformationW(
        driveLetter.c_str(), NULL, 0, &serialNumber, NULL, NULL, NULL, 0)) {
        throw std::runtime_error("Failed to get drive serial number.");
        }

    std::ostringstream oss;
    oss << std::hex << serialNumber;
    return oss.str();
}

std::wstring getHWID() {
    try {
        std::string mbSerial = queryWMIProperty(_bstr_t(L"Win32_BaseBoard"), _bstr_t(L"SerialNumber"));
        std::string cpuId    = queryWMIProperty(_bstr_t(L"Win32_Processor"), _bstr_t(L"ProcessorId"));
        std::string driveSerial;

        // Run raw query for C: drive only
        HRESULT hres;
        IWbemLocator *pLoc = nullptr;
        IWbemServices *pSvc = nullptr;
        IEnumWbemClassObject* pEnumerator = nullptr;
        IWbemClassObject *pclsObj = nullptr;
        ULONG uReturn = 0;
        VARIANT vtProp;

        hres = CoInitializeEx(0, COINIT_MULTITHREADED);
        if (FAILED(hres))
            throw std::runtime_error("COM init failed.");

        CoInitializeSecurity(NULL, -1, NULL, NULL,
            RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE,
            NULL, EOAC_NONE, NULL);

        hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
            IID_IWbemLocator, (LPVOID*)&pLoc);
        if (FAILED(hres))
            throw std::runtime_error("WMI locator creation failed.");

        hres = pLoc->ConnectServer(
            _bstr_t(L"ROOT\\CIMV2"), nullptr, nullptr, 0, NULL, 0, 0, &pSvc);
        if (FAILED(hres))
            throw std::runtime_error("WMI connection failed.");

        CoSetProxyBlanket(pSvc,
            RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
            RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE,
            NULL, EOAC_NONE);

        hres = pSvc->ExecQuery(
            bstr_t("WQL"),
            bstr_t("SELECT VolumeSerialNumber FROM Win32_LogicalDisk WHERE DeviceID = 'C:'"),
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
            NULL,
            &pEnumerator);
        if (FAILED(hres))
            throw std::runtime_error("Drive WMI query failed.");

        hres = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        if (uReturn > 0) {
            hres = pclsObj->Get(L"VolumeSerialNumber", 0, &vtProp, 0, 0);
            if (SUCCEEDED(hres) && vtProp.vt == VT_BSTR)
                driveSerial = _bstr_t(vtProp.bstrVal);
            VariantClear(&vtProp);
            pclsObj->Release();
        }

        // Cleanup
        pSvc->Release();
        pLoc->Release();
        pEnumerator->Release();
        CoUninitialize();

    	std::vector<BYTE> ekData = Utils::GetEK();
    	if (ekData.empty())
    	{
    		printf("Failed to retrieve EK\n");
    	}

    	std::string EKHash = Utils::GetKeyHash(ekData, CALG_SHA_256);
    	std::string combined;
    	printf("[TPM] %s\n", EKHash.c_str());

    	if (!EKHash.empty()) {
    		combined = EKHash + "|" + mbSerial + "|" + cpuId + "|" + driveSerial;
    	} else {
    		combined = mbSerial + "|" + cpuId + "|" + driveSerial;
    	}
        std::string hashed = hashSHA256(combined);

        return str_to_wstr(hashed);
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("HWID collection failed: ") + e.what());
    }
}

std::string sendPost(const std::wstring& url, const std::wstring& token, const std::wstring& hwid) {
    URL_COMPONENTS components = { 0 };
    components.dwStructSize = sizeof(URL_COMPONENTS);

    wchar_t host[256];
    wchar_t path[256];
    components.lpszHostName = host;
    components.dwHostNameLength = sizeof(host) / sizeof(wchar_t);
    components.lpszUrlPath = path;
    components.dwUrlPathLength = sizeof(path) / sizeof(wchar_t);

    if (!WinHttpCrackUrl(url.c_str(), (DWORD)url.length(), 0, &components))
        throw std::runtime_error("Invalid URL provided.");

    HINTERNET hSession = WinHttpOpen(L"CLoader/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, nullptr, nullptr, 0);
    HINTERNET hConnect = WinHttpConnect(hSession, components.lpszHostName, components.nPort, 0);
    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        L"POST",
        components.lpszUrlPath,
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        components.nPort == 443 ? WINHTTP_FLAG_SECURE : 0
    );

    std::wstring headers =
        L"Authorization: " + token + L"\r\n" +
        L"X-HWID: " + hwid + L"\r\n" +
        L"Content-Length: 0\r\n";

    if (!WinHttpSendRequest(hRequest, headers.c_str(), (DWORD)-1, WINHTTP_NO_REQUEST_DATA, 0, 0, 0))
        throw std::runtime_error("Failed to send HTTP request.");

    if (!WinHttpReceiveResponse(hRequest, nullptr))
        throw std::runtime_error("No response received from server.");

    std::string response;
    DWORD size = 0;

    do {
        DWORD bytesAvailable = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &bytesAvailable))
            break;

        if (bytesAvailable == 0) break;

        std::vector<char> buffer(bytesAvailable);
        DWORD bytesRead = 0;

        if (!WinHttpReadData(hRequest, buffer.data(), bytesAvailable, &bytesRead))
            break;

        response.append(buffer.begin(), buffer.begin() + bytesRead);
    } while (size != 0);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return response;
}
#pragma endregion

std::string exploitDLLPath;

HANDLE pHandle;
uintptr_t sectionBase;

std::map<std::string, uintptr_t> manualMaps;
std::vector<std::pair<std::string, uintptr_t>> imports;

namespace Mapper {
    uintptr_t Map(std::string path);
}

template <typename T>
T Read(uintptr_t address) {
    T buffer{};
    SIZE_T bytesRead;

    if (ReadProcessMemory(pHandle, (LPCVOID)address, &buffer, sizeof(T), &bytesRead) && bytesRead == sizeof(T)) {
        return buffer;
    }

    return T();
}

template <typename T>
bool Write(uintptr_t address, const T& value) {
    SIZE_T bytesWritten;
    return WriteProcessMemory(pHandle, (LPVOID)address, &value, sizeof(T), &bytesWritten) && bytesWritten == sizeof(T);
}

enum class LogType { Error, Success, Info, Warn };

void Log(LogType type, const std::string& msg) {
    std::cout << msg << "\x1b[0m\n";
}

std::string ResolveAPI(const std::string& path) {
	if (path.find("api-ms-win-crt-") == 0) {
		return "ucrtbase.dll";
	}
	else if (path.find("api-ms-win-core-") == 0) {
		if (path.find("rtlsupport") != std::string::npos) {
			return "ntdll.dll";
		}
		if (path.find("localization-obsolete") != std::string::npos || path.find("string-obsolete") != std::string::npos) {
			return "kernelbase.dll";
		}
		return "kernel32.dll";
	}
	else if (path.find("api-ms-win-security-") == 0 || path.find("api-ms-win-eventing-") == 0) {
		return "advapi32.dll";
	}
	return path;
}

bool ExistsImport(std::string path) {
	HMODULE hModules[1024];
	DWORD cbNeeded;

	std::string transformedPath = ResolveAPI(path);
	std::transform(transformedPath.begin(), transformedPath.end(), transformedPath.begin(), ::tolower);

	if (EnumProcessModules(pHandle, hModules, sizeof(hModules), &cbNeeded)) {
		DWORD numModules = cbNeeded / sizeof(HMODULE);
		for (DWORD i = 0; i < numModules; ++i) {
			char szModName[MAX_PATH];
			if (GetModuleFileNameExA(pHandle, hModules[i], szModName, sizeof(szModName))) {
				std::string szPath(szModName);
				std::transform(szPath.begin(), szPath.end(), szPath.begin(), ::tolower);

				if (PathFindFileNameA(szPath.c_str()) == transformedPath) {
					return true;
				}
			}
		}
	}

	return false;
}

uint64_t GetImportSize(const std::string& path) {
	uint64_t total_size = 0;

	HMODULE hModule = LoadLibraryExA(path.c_str(), NULL, DONT_RESOLVE_DLL_REFERENCES);
	if (!hModule) {
		return NULL;
	}

	auto* dosHeader = (PIMAGE_DOS_HEADER)hModule;
	if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
		FreeLibrary(hModule);
		return NULL;
	}

	auto* ntHeader = (PIMAGE_NT_HEADERS)((BYTE*)hModule + dosHeader->e_lfanew);
	if (ntHeader->Signature != IMAGE_NT_SIGNATURE) {
		FreeLibrary(hModule);
		return NULL;
	}

	auto importVA = ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
	if (!importVA) {
		FreeLibrary(hModule);
		return std::filesystem::file_size(path);
	}

	auto* importDesc = (PIMAGE_IMPORT_DESCRIPTOR)((BYTE*)hModule + importVA);

	while (importDesc && importDesc->Name) {
		std::string importName = (char*)((BYTE*)hModule + importDesc->Name);
		std::string transformedImport = ResolveAPI(importName);

		if (ExistsImport(transformedImport)) {
			++importDesc;
			continue;
		}

		std::filesystem::path importPath = transformedImport;
		if (!std::filesystem::exists(importPath)) {
			importPath = std::filesystem::path(getenv("SystemRoot")) / "System32" / transformedImport;
		}

		if (std::filesystem::exists(importPath)) {
			total_size += std::filesystem::file_size(importPath) + 0x100;
		}

		total_size += GetImportSize(importPath.string());

		++importDesc;
	}

	FreeLibrary(hModule);
	return std::filesystem::file_size(path) + total_size;
}

std::vector<std::pair<std::string, uintptr_t>> GetImports(const std::string& path, uintptr_t allocated_mem) {
	std::vector<std::pair<std::string, uintptr_t>> imports;

	imports.emplace_back(path, allocated_mem);

	uintptr_t currentMem = allocated_mem + std::filesystem::file_size(path) + 0x100; // skip the main dll

	HMODULE hModule = LoadLibraryExA(path.c_str(), NULL, DONT_RESOLVE_DLL_REFERENCES);
	if (!hModule) {
		return imports;
	}

	auto* dosHeader = (PIMAGE_DOS_HEADER)hModule;
	if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
		FreeLibrary(hModule);
		return imports;
	}

	auto* ntHeader = (PIMAGE_NT_HEADERS)((BYTE*)hModule + dosHeader->e_lfanew);
	if (ntHeader->Signature != IMAGE_NT_SIGNATURE) {
		FreeLibrary(hModule);
		return imports;
	}

	auto importVA = ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
	if (!importVA) {
		FreeLibrary(hModule);
		return imports;
	}

	auto* importDesc = (PIMAGE_IMPORT_DESCRIPTOR)((BYTE*)hModule + importVA);

	while (importDesc && importDesc->Name) {
		std::string importName = (char*)((BYTE*)hModule + importDesc->Name);

		std::string transformedImport = ResolveAPI(importName);

		if (ExistsImport(transformedImport)) {
			++importDesc;
			continue;
		}

		uintptr_t importAddress = currentMem;
		imports.emplace_back(transformedImport, importAddress);

		std::filesystem::path importPath = transformedImport;
		if (!std::filesystem::exists(importPath)) {
			importPath = std::filesystem::path(getenv("SystemRoot")) / "System32" / transformedImport;
		}

		if (std::filesystem::exists(importPath)) {
			currentMem += std::filesystem::file_size(importPath) + 0x100;

			auto subImports = GetImports(importPath.string(), currentMem);
			imports.insert(imports.end(), subImports.begin(), subImports.end());
		}
		else {
			Log(LogType::Error, (std::ostringstream() << "Unable to find Import: " << importName).str());
		}

		++importDesc;
	}

	FreeLibrary(hModule);
	return imports;
}

uintptr_t GetModuleBaseAddress(DWORD processId, const char* moduleName) {
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, processId);
	if (snapshot == INVALID_HANDLE_VALUE) {
		return NULL;
	}

	MODULEENTRY32 moduleEntry;
	moduleEntry.dwSize = sizeof(MODULEENTRY32);

	uintptr_t baseAddress = 0;

	if (Module32First(snapshot, &moduleEntry)) {
		do {
			if (_stricmp(moduleEntry.szModule, moduleName) == 0) {
				baseAddress = (uintptr_t)moduleEntry.modBaseAddr;
				break;
			}
		} while (Module32Next(snapshot, &moduleEntry));
	}

	CloseHandle(snapshot);
	return baseAddress;
}

uintptr_t RVAVA(uintptr_t RVA, PIMAGE_NT_HEADERS NtHeaders, uint8_t* RawData)
{
	PIMAGE_SECTION_HEADER FirstSection = IMAGE_FIRST_SECTION(NtHeaders);

	for (PIMAGE_SECTION_HEADER Section = FirstSection; Section < FirstSection + NtHeaders->FileHeader.NumberOfSections; Section++)
		if (RVA >= Section->VirtualAddress && RVA < Section->VirtualAddress + Section->Misc.VirtualSize)
			return (uintptr_t)RawData + Section->PointerToRawData + (RVA - Section->VirtualAddress);

	return NULL;
}

uintptr_t GetFunctionOffset(uintptr_t base, LPCSTR libname, LPCSTR functionName) {
	HMODULE exported = LoadLibrary(libname);
	if (!exported)
		return NULL;

	return base + ((uintptr_t)GetProcAddress(exported, functionName) - (uintptr_t)exported);
}

bool ResolveImports(uint8_t* RawData, PIMAGE_NT_HEADERS NtHeaders, HANDLE pHandle)
{
	PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)RVAVA(NtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress, NtHeaders, RawData);

	if (!NtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress || !NtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size)
		return true;

	LPSTR ModuleName = NULL;
	while (ModuleName = (LPSTR)RVAVA(ImportDescriptor->Name, NtHeaders, RawData))
	{
		uintptr_t ModuleHandle = Mapper::Map(ModuleName);
		if (!ModuleHandle)
		{
			Log(LogType::Error, (std::ostringstream() << "Failed to resolve Import: " << ModuleName).str());
			continue;
		}

		PIMAGE_THUNK_DATA ThunkData = (PIMAGE_THUNK_DATA)RVAVA(ImportDescriptor->FirstThunk, NtHeaders, RawData);

		while (ThunkData->u1.AddressOfData)
		{
			if (ThunkData->u1.Ordinal & IMAGE_ORDINAL_FLAG)
			{
				uintptr_t resolved = GetFunctionOffset(ModuleHandle, ModuleName, (LPCSTR)(ThunkData->u1.Ordinal & 0xFFFF));
				ThunkData->u1.Function = resolved;
			}
			else
			{
				PIMAGE_IMPORT_BY_NAME ImportByName = (PIMAGE_IMPORT_BY_NAME)RVAVA(ThunkData->u1.AddressOfData, NtHeaders, RawData);
				uintptr_t resolved = GetFunctionOffset(ModuleHandle, ModuleName, (LPCSTR)ImportByName->Name);
				ThunkData->u1.Function = resolved;
			}
			ThunkData++;
		}
		ImportDescriptor++;
	}

	return true;
}

BOOL RelocateImage(uintptr_t p_remote_img, PVOID p_local_img, PIMAGE_NT_HEADERS nt_head)
{
	struct reloc_entry
	{
		ULONG to_rva;
		ULONG size;
		struct
		{
			WORD offset : 12;
			WORD type : 4;
		} item[1];
	};

	uintptr_t delta_offset = p_remote_img - nt_head->OptionalHeader.ImageBase;
	if (!delta_offset) return true; else if (!(nt_head->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE)) return false;
	reloc_entry* reloc_ent = (reloc_entry*)RVAVA(nt_head->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress, nt_head, (uint8_t*)p_local_img);
	uintptr_t reloc_end = (uintptr_t)reloc_ent + nt_head->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;

	if (reloc_ent == nullptr)
		return true;

	while ((uintptr_t)reloc_ent < reloc_end && reloc_ent->size)
	{
		DWORD records_count = (reloc_ent->size - 8) >> 1;
		for (DWORD i = 0; i < records_count; i++)
		{
			WORD fix_type = (reloc_ent->item[i].type);
			WORD shift_delta = (reloc_ent->item[i].offset) % 4096;

			if (fix_type == IMAGE_REL_BASED_ABSOLUTE)
				continue;

			if (fix_type == IMAGE_REL_BASED_HIGHLOW || fix_type == IMAGE_REL_BASED_DIR64)
			{
				uintptr_t fix_va = (uintptr_t)RVAVA(reloc_ent->to_rva, nt_head, (uint8_t*)p_local_img);

				if (!fix_va)
					fix_va = (uintptr_t)p_local_img;

				*(uintptr_t*)(fix_va + shift_delta) += delta_offset;
			}
		}

		reloc_ent = (reloc_entry*)((LPBYTE)reloc_ent + reloc_ent->size);
	}

	return true;
}

uintptr_t FindImportAddress(const std::string& importName) {
	for (const auto& import : imports) {
		if (_stricmp(import.first.c_str(), importName.c_str()) == 0) {
			return import.second;
		}
	}
	return NULL;
}

uintptr_t FindMappedModule(std::string path) {
	HMODULE hModules[1024];
	DWORD cbNeeded;

	std::string transformedTarget = ResolveAPI(path);
	std::transform(transformedTarget.begin(), transformedTarget.end(), transformedTarget.begin(), ::tolower);

	if (EnumProcessModules(pHandle, hModules, sizeof(hModules), &cbNeeded)) {
		DWORD numModules = cbNeeded / sizeof(HMODULE);
		for (DWORD i = 0; i < numModules; ++i) {
			char szModName[MAX_PATH];
			if (GetModuleFileNameExA(pHandle, hModules[i], szModName, sizeof(szModName))) {
				std::string modulePath(szModName);
				std::transform(modulePath.begin(), modulePath.end(), modulePath.begin(), ::tolower);
				std::string moduleFilename = PathFindFileNameA(modulePath.c_str());
				if (moduleFilename == transformedTarget) {
					return (uintptr_t)hModules[i];
				}
			}
		}
	}

	return NULL;
}

uintptr_t Mapper::Map(std::string path) {
	std::string oldPath = path;

	uintptr_t module = FindMappedModule(path);
	if (module)
		return module;

	auto mappedDLL = manualMaps.find(path);
	if (mappedDLL != manualMaps.end())
		return mappedDLL->second;

	uintptr_t dllBase = FindImportAddress(path);

	std::ifstream tFile(path, std::ios::binary | std::ios::ate);
	if (!tFile.is_open())
	{
		path = "C:\\Windows\\System32\\" + path;
		tFile.open(path, std::ios::binary | std::ios::ate);
		if (!tFile.is_open()) {
			Log(LogType::Error, (std::ostringstream() << "Failed to open file: " << path).str());
			exit(EXIT_FAILURE);
		}
	}
	tFile.close();

	if(!dllBase)
		dllBase = FindImportAddress(path);

	manualMaps[oldPath] = dllBase;

	uintptr_t dllSize = std::filesystem::file_size(path);

	DWORD oldprotect;
	BOOL virtualProtect = VirtualProtectEx(pHandle, (LPVOID)dllBase, dllSize, PAGE_EXECUTE_READWRITE, &oldprotect);
	if (!virtualProtect) {
		Log(LogType::Error, (std::ostringstream() << "Failed to change memory protection for: " << path << ". Error: " << GetLastError()).str());
		exit(EXIT_FAILURE);
	}

	std::ifstream File(path, std::ios::binary | std::ios::ate);
	if (!File.is_open())
	{
		std::string system32Path = "C:\\Windows\\System32\\" + path;
		File.open(system32Path, std::ios::binary | std::ios::ate);
		if (!File.is_open()) {
			Log(LogType::Error, (std::ostringstream() << "Failed to open file: " << path).str());
			exit(EXIT_FAILURE);
		}
	}
	std::streampos file_size = File.tellg();
	PBYTE buffer = (PBYTE)malloc(file_size);

	File.seekg(0, std::ios::beg);
	File.read((char*)buffer, file_size);
	File.close();

	if (!buffer)
		return NULL;

	PIMAGE_NT_HEADERS ntHeader = (IMAGE_NT_HEADERS*)(buffer + ((IMAGE_DOS_HEADER*)buffer)->e_lfanew);
	PIMAGE_OPTIONAL_HEADER optionalHeader = &ntHeader->OptionalHeader;
	PIMAGE_FILE_HEADER fileHeader = &ntHeader->FileHeader;

	PBYTE dllAddress = (PBYTE)dllBase;

	if (!dllAddress) {
		Log(LogType::Error, (std::ostringstream() << "Invalid DLL Address for: " << path).str());
		exit(EXIT_FAILURE);
	};

	uintptr_t entryPoint = (uintptr_t)dllAddress + (uintptr_t)optionalHeader->AddressOfEntryPoint;

	if (path.find(exploitDLLPath) != std::string::npos) {
		Write<uintptr_t>(sectionBase + 0x20, entryPoint);
		Write<uintptr_t>(sectionBase + 0x28, (uintptr_t)dllAddress);

		// SEH support
		auto excep = optionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];
		if (excep.Size) {
			Write<uintptr_t>(sectionBase + 0x30, excep.VirtualAddress);
			Write<uintptr_t>(sectionBase + 0x38, excep.Size);
		}
	}

	if (!RelocateImage((uintptr_t)dllAddress, buffer, ntHeader))
		Log(LogType::Error, "Failed to relocate image");

	if (!ResolveImports(buffer, ntHeader, pHandle))
		Log(LogType::Error, "Failed to resolve imports");

	PIMAGE_SECTION_HEADER sectionHeader = IMAGE_FIRST_SECTION(ntHeader);

	for (UINT i = 0; i < fileHeader->NumberOfSections; ++i, ++sectionHeader)
	{
		if (sectionHeader->SizeOfRawData == 0)
			continue;

		LPVOID targetAddress = (LPVOID)(dllAddress + sectionHeader->VirtualAddress);
		LPVOID sourceData = (LPVOID)(buffer + sectionHeader->PointerToRawData);
		SIZE_T dataSize = sectionHeader->SizeOfRawData;

		BOOL writeResult = WriteProcessMemory(pHandle, targetAddress, sourceData, dataSize, nullptr);

		if (!writeResult)
		{
			Log(LogType::Error, (std::ostringstream() << "Failed to write section: " << sectionHeader->Name << ". Error: " << GetLastError()).str());

			free(buffer);
			VirtualFreeEx(pHandle, dllAddress, 0, MEM_RELEASE);

			exit(EXIT_FAILURE);
		}
	}

	return dllBase;
}

void EnableVirtualTerminal() {
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut == INVALID_HANDLE_VALUE) return;

	DWORD dwMode = 0;
	if (!GetConsoleMode(hOut, &dwMode)) return;

	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	SetConsoleMode(hOut, dwMode);
}

int wmain(int argc, wchar_t* argv[]) {
    if (argc < 3) {
        std::wcerr << L"Usage: loader.exe <endpoint> <dll_name>" << std::endl;
        return 1;
    }

    exploitDLLPath = (std::filesystem::current_path() / argv[2]).string();

	if (!std::filesystem::exists(exploitDLLPath)) {
		std::cerr << "Error: File does not exist at path: " << exploitDLLPath << std::endl;
		return 6;
	}

    try {
        std::wstring endpoint = argv[1];
        std::wstring token = getTokenFromRegistry();
        std::wstring hwid = getHWID();

        std::wcout << L"[HWID] " << hwid << std::endl;
        std::cout << "[DLL]" << exploitDLLPath << std::endl;

        std::string jsonResponse = sendPost(endpoint, token, hwid);

        auto data = nlohmann::json::parse(jsonResponse);

        if (data.contains("detail")) {
            std::string reason = data["detail"];
            if (reason == "Incorrect HWID") {
                std::cerr << "Error: Your device's hardware ID does not match our records." << std::endl;
                return 2;
            }
            if (reason == "Invalid or expired token") {
                std::cerr << "Error: Your authentication token is invalid or has expired." << std::endl;
                return 3;
            }
        }

        if (data.contains("blacklisted") && data["blacklisted"] == true) {
            std::cerr << "Access denied: Your account or device has been blacklisted." << std::endl;
            return 4;
        }

        if (data.contains("nokey") && data["nokey"] == true) {
            std::cerr << "Access denied: Your license key is invalid, expired, or revoked." << std::endl;
            return 5;
        }

        std::cout << "Authentication succeeded." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Loader error: " << e.what() << std::endl;
        return -1;
    }

    EnableVirtualTerminal();
	SetupNTDLL();

	if (NTDLL == NULL) {
		Log(LogType::Error, "NTDLL setup errored");
		exit(EXIT_FAILURE);
	}

	if (GetThisModuleInfo() == 0) {
		Log(LogType::Error, "Failed getting injector module info");
		exit(EXIT_FAILURE);
	}

	SYSTEM_PROCESS_INFORMATION* RobloxProcessInformation = FindProcessByModuleName(L"RobloxPlayerBeta.exe");
	if (!RobloxProcessInformation) {
		Log(LogType::Error, "Roblox not found");
		exit(EXIT_FAILURE);
	}

	injection_vars.RobloxProcessInfo = RobloxProcessInformation;

	pHandle = OpenProcess(PROCESS_ALL_ACCESS, 0, RobloxProcessInformation->ProcessId);
	if (pHandle == NULL) {
		Log(LogType::Error, "Error opening handle");
		exit(EXIT_FAILURE);
	}

	if (InjectionFindByfron(pHandle, &injection_vars) == 0) {
		Log(LogType::Error, "Failed to find byfron module");
		exit(EXIT_FAILURE);
	};

	for (DWORD currentThread = 0; currentThread < RobloxProcessInformation->ThreadCount; ++currentThread) {
		const auto& threadInfo = RobloxProcessInformation->ThreadInfos[currentThread];
		HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, threadInfo.Client_Id.UniqueThread);
		if (!hThread) {
			Log(LogType::Error, (std::ostringstream() << "Failed to open thread handle for TID: " << threadInfo.Client_Id.UniqueThread << ", error : " << GetLastError()).str());
			continue;
		}

		ULONG_PTR startAddr = 0;
		DWORD status = static_cast<DWORD>(CallSyscall(0x0025, hThread, ThreadQuerySetWin32StartAddress, &startAddr, sizeof(ULONG_PTR), nullptr));
		if (status == 0) {
			ULONG_PTR dllBase = (ULONG_PTR)injection_vars.byfron.lpBaseOfDll;
			ULONG_PTR dllEnd = dllBase + injection_vars.byfron.SizeOfImage;

			if (startAddr >= dllBase && startAddr < dllEnd) {
				SuspendThread(hThread);
			}
		}

		CloseHandle(hThread);
	}

	if (InjectionAllocSelf(pHandle, &injection_vars, thismoduleinfo) == 0) {
		Log(LogType::Error, "Failed to map injector");
		exit(EXIT_FAILURE);
	};

	sectionBase = GetModuleBaseAddress(RobloxProcessInformation->ProcessId, "winsta.dll") + 0x328;
	uint64_t totalSize = GetImportSize(exploitDLLPath);

	DWORD oldProtect;
	if (!VirtualProtectEx(pHandle, (void*)sectionBase, sizeof(uintptr_t) * 8, PAGE_READWRITE, &oldProtect)) {
		Log(LogType::Error, (std::ostringstream() << "VirtualProtect failed. Error: " << GetLastError()).str());
		exit(EXIT_FAILURE);
	}

	uintptr_t allocated_mem = (uintptr_t)VirtualAllocEx(pHandle, nullptr, totalSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!allocated_mem) {
		Log(LogType::Error, (std::ostringstream() << "Memory allocation failed. Error: " << GetLastError()).str());
		exit(EXIT_FAILURE);
	}

	imports = GetImports(exploitDLLPath, allocated_mem);

	Write<uintptr_t>(sectionBase, allocated_mem);
	Write<uint64_t>(sectionBase + 0x8, totalSize);
	Write<int>(sectionBase + 0x10, 0);
	Write<int>(sectionBase + 0x18, 0);
	Write<uintptr_t>(sectionBase + 0x20, 0);
	Write<uintptr_t>(sectionBase + 0x28, 0);
	Write<uintptr_t>(sectionBase + 0x30, 0);
	Write<uintptr_t>(sectionBase + 0x38, 0);

	if (InjectionSetupInternalPart(pHandle, &injection_vars) == 0) {
		Log(LogType::Error, "Failed to setup internal mechanics");
		exit(EXIT_FAILURE);
	};

	do {
		Sleep(1);
	} while (Read<int>(sectionBase + 0x10) != 1);

	Mapper::Map(exploitDLLPath);

	Log(LogType::Info, (std::ostringstream() << "EntryPoint: 0x" << std::hex << Read<uintptr_t>(sectionBase + 0x20) << "\n").str());

	Write<int>(sectionBase + 0x18, 1);

	Sleep(10);
	Log(LogType::Success, "Successfully attached");

	CloseHandle(pHandle);
	exit(EXIT_SUCCESS);
}