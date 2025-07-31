#pragma once
#include <filesystem>
#include <iostream>
#include <algorithm>
#include "lua.h"
#include <lualib.h>
#include "lapi.h"
#include <Execution/Execution.hpp>
#include <TaskScheduler/TaskScheduler.hpp>
#include <Shlwapi.h>
#include <vector>
#include <fstream>
#include "../Crypt/Crypt.h"

namespace fs = std::filesystem;

static std::filesystem::path localAppdata = getenv("APPDATA");
static std::filesystem::path realLibrary = localAppdata / "Blackline";
static std::filesystem::path workspace = realLibrary / "Workspace";

static std::vector<std::string> disallowedExtensions =
{
	".exe", ".scr", ".bat", ".com", ".csh", ".msi", ".vb", ".vbs", ".vbe", ".ws", ".wsf", ".wsh", ".ps1"
};
inline bool equals_ignore_case(const std::string& a, const std::string& b)
{
	return std::equal(a.begin(), a.end(), b.begin(), b.end(), [](char a, char b) { return tolower(a) == tolower(b); });
}

namespace Filesystem
{
	inline int dofile(lua_State* L) {
		if (!fs::exists(workspace)) {
			std::error_code ec;
			fs::create_directories(workspace, ec);
			return dofile(L);
		}
		luaL_checktype(L, 1, LUA_TSTRING);

		std::string path = lua_tostring(L, 1);
		std::replace(path.begin(), path.end(), '/', '\\');
		if (path.find("..") != std::string::npos) {
			lua_getglobal(L, "warn");
			lua_pushstring(L, "attempt to escape directory");
			lua_call(L, 1, 0);
			return 0;
		}

		std::filesystem::path workspacePath = workspace / path;

		FILE* file = fopen(workspacePath.string().c_str(), "rb");
		if (!file) {
			lua_getglobal(L, "warn");
			lua_pushstring(L, "file does not exist!");
			lua_call(L, 1, 0);
			return 0;
		}

		fseek(file, 0, SEEK_END);
		size_t fileSize = ftell(file);
		rewind(file);

		std::string content(fileSize, '\0');
		size_t bytesread = fread(&content[0], 1, fileSize, file);
		fclose(file);

		std::string script = Execution->CompileScript(content);
		if (script[0] == '\0' || script.empty()) {
			lua_pushnil(L);
			lua_pushstring(L, "Failed to compile script");
			return 2;
		}

		int result = Roblox::LuaVM__Load(L, &script, "=", 0);
		if (result != LUA_OK) {
			std::string Error = luaL_checklstring(L, -1, nullptr);
			lua_pop(L, 1);

			lua_pushnil(L);
			lua_pushstring(L, Error.data());

			return 2;
		}

		Closure* closure = clvalue(luaA_toobject(L, -1));

		Taskscheduler->SetProtoCapabilities(closure->l.p);

		Roblox::Task__Defer(L);

		return 0;
	}
	inline int appendfile(lua_State* L) {
		if (!fs::exists(workspace)) {
			std::error_code ec;
			fs::create_directories(workspace, ec);
			return appendfile(L);
		}
		luaL_checktype(L, 1, LUA_TSTRING);
		luaL_checktype(L, 2, LUA_TSTRING);

		std::string path = lua_tostring(L, 1);
		std::string data = lua_tostring(L, 2);

		std::replace(path.begin(), path.end(), '/', '\\');

		if (path.find("..") != std::string::npos) {
			lua_getglobal(L, "warn");
			lua_pushstring(L, "attempt to escape directory");
			lua_call(L, 1, 0);
			return 0;
		}

		std::filesystem::path workspacePath = workspace / path;

		std::string extension = PathFindExtension(path.c_str());

		for (const std::string& forbidden : disallowedExtensions) {
			if (equals_ignore_case(extension, forbidden)) {
				lua_getglobal(L, "warn");
				lua_pushstring(L, "forbidden extension!");
				lua_call(L, 1, 0);
				return 0;
			}
		}

		std::ofstream outFile(workspacePath, std::ios::app | std::ios::binary);
		if (!outFile.is_open()) {
			lua_getglobal(L, "warn");
			lua_pushstring(L, "failed to open file for appending");
			lua_call(L, 1, 0);
			return 0;
		}

		outFile.write(data.data(), data.size());

		if (!outFile) {
			lua_getglobal(L, "warn");
			lua_pushstring(L, "error while writing to file");
			lua_call(L, 1, 0);
		}

		outFile.close();
		return 0;
	}
	inline int delfile(lua_State* L) {
		if (!fs::exists(workspace)) {
			std::error_code ec;
			fs::create_directories(workspace, ec);
			return delfile(L);
		}
		luaL_checktype(L, 1, LUA_TSTRING);

		std::string path = lua_tostring(L, 1);
		std::replace(path.begin(), path.end(), '/', '\\');
		if (path.find("..") != std::string::npos) {
			lua_getglobal(L, "warn");
			lua_pushstring(L, "attempt to escape directory");
			lua_call(L, 1, 0);
			return 0;
		}

		std::filesystem::path workspacePath = workspace / path;
		if (!std::filesystem::remove(workspacePath)) {
			lua_getglobal(L, "warn");
			lua_pushstring(L, "file does not exist!");
			lua_call(L, 1, 0);
			return 0;
		}

		return 0;
	}
	inline int delfolder(lua_State* L) {
		if (!fs::exists(workspace)) {
			std::error_code ec;
			fs::create_directories(workspace, ec);
			return delfolder(L);
		}
		luaL_checktype(L, 1, LUA_TSTRING);

		std::string path = lua_tostring(L, 1);
		std::replace(path.begin(), path.end(), '/', '\\');
		if (path.find("..") != std::string::npos) {
			lua_getglobal(L, "warn");
			lua_pushstring(L, "attempt to escape directory");
			lua_call(L, 1, 0);
			return 0;
		}

		std::filesystem::path workspacePath = workspace / path;
		if (!std::filesystem::remove_all(workspacePath)) {
			lua_getglobal(L, "warn");
			lua_pushstring(L, "folder does not exist!");
			lua_call(L, 1, 0);
			return 0;
		}

		return 0;
	}
	inline int getcustomasset(lua_State* L) {
		luaL_checktype(L, 1, LUA_TSTRING);
		std::string assetPath = lua_tostring(L, 1);

		std::string fullPathStr = workspace.string() + "\\" + assetPath;
		std::replace(fullPathStr.begin(), fullPathStr.end(), '\\', '/');
		std::filesystem::path FullPath = fullPathStr;

		if (!std::filesystem::is_regular_file(FullPath))
			luaL_error(L, ("Failed to find local asset!"));

		std::filesystem::path customAssetsDir = std::filesystem::current_path() / "ExtraContent" / "Blackline";
		std::filesystem::path customAssetsFile = std::filesystem::current_path() / "ExtraContent" / "Blackline" / FullPath.filename();

		if (!std::filesystem::exists(customAssetsDir))
			std::filesystem::create_directory(customAssetsDir);

		std::filesystem::copy_file(FullPath, customAssetsFile, std::filesystem::copy_options::update_existing);

		std::string Final = "rbxasset://Blackline/" + customAssetsFile.filename().string();
		lua_pushlstring(L, Final.c_str(), Final.size());
		return 1;
	}
	inline int isfile(lua_State* L) {
		if (!fs::exists(workspace)) {
			luaL_error(L, "Workspace not initialized.");
			return 0;
		}
		luaL_checktype(L, 1, LUA_TSTRING);

		std::string path = lua_tostring(L, 1);
		std::replace(path.begin(), path.end(), '/', '\\');
		if (path.find("..") != std::string::npos) {
			lua_getglobal(L, "warn");
			lua_pushstring(L, "attempt to escape directory");
			lua_call(L, 1, 0);
			return 0;
		}

		std::filesystem::path workspacePath = workspace / path;

		lua_pushboolean(L, std::filesystem::is_regular_file(workspacePath));

		return 1;
	}
	inline int isfolder(lua_State* L) {
		if (!fs::exists(workspace)) {
			std::error_code ec;
			fs::create_directories(workspace, ec);
			return isfolder(L);
		}
		luaL_checktype(L, 1, LUA_TSTRING);

		std::string path = lua_tostring(L, 1);
		std::replace(path.begin(), path.end(), '/', '\\');
		if (path.find("..") != std::string::npos) {
			lua_getglobal(L, "warn");
			lua_pushstring(L, "attempt to escape directory");
			lua_call(L, 1, 0);
			return 0;
		}

		std::filesystem::path workspacePath = workspace / path;

		lua_pushboolean(L, std::filesystem::is_directory(workspacePath));

		return 1;
	}
	inline int listfiles(lua_State* L) {
		if (!fs::exists(workspace)) {
			std::error_code ec;
			fs::create_directories(workspace, ec);
			return listfiles(L);
		}
		luaL_checktype(L, 1, LUA_TSTRING);

		std::string path = lua_tostring(L, 1);
		std::replace(path.begin(), path.end(), '/', '\\');
		if (path.find("..") != std::string::npos) {
			lua_getglobal(L, "warn");
			lua_pushstring(L, "attempt to escape directory");
			lua_call(L, 1, 0);
			return 0;
		}

		std::filesystem::path workspacePath = workspace / path;
		if (!std::filesystem::exists(workspacePath)) {
			lua_getglobal(L, "warn");
			lua_pushstring(L, "directory doesn't exist!");
			lua_call(L, 1, 0);
			return 0;
		}

		int index = 0;
		lua_newtable(L);

		for (auto& file : std::filesystem::directory_iterator(workspacePath)) {
			auto filePath = file.path().string().substr(workspace.string().length() + 1);

			lua_pushinteger(L, ++index);
			lua_pushstring(L, filePath.data());
			lua_settable(L, -3);
		}

		return 1;
	}
	inline int loadfile(lua_State* L) {
		if (!fs::exists(workspace)) {
			std::error_code ec;
			fs::create_directories(workspace, ec);
			return loadfile(L);
		}
		luaL_checktype(L, 1, LUA_TSTRING);

		std::string path = lua_tostring(L, 1);
		const std::string chunkname = luaL_optstring(L, 2, "=");
		std::replace(path.begin(), path.end(), '/', '\\');
		if (path.find("..") != std::string::npos) {
			lua_getglobal(L, "warn");
			lua_pushstring(L, "attempt to escape directory");
			lua_call(L, 1, 0);
			return 0;
		}

		std::filesystem::path workspacePath = workspace / path;

		FILE* file = fopen(workspacePath.string().c_str(), "rb");
		if (!file) {
			lua_getglobal(L, "warn");
			lua_pushstring(L, "file does not exist!");
			lua_call(L, 1, 0);
			return 0;
		}

		fseek(file, 0, SEEK_END);
		size_t fileSize = ftell(file);
		rewind(file);

		std::string content(fileSize, '\0');
		size_t bytesread = fread(&content[0], 1, fileSize, file);
		fclose(file);

		std::string script = Execution->CompileScript(content);
		if (script[0] == '\0' || script.empty()) {
			lua_pushnil(L);
			lua_pushstring(L, "Failed to compile script");
			return 2;
		}

		int result = Roblox::LuaVM__Load(L, &script, chunkname.data(), 0);
		if (result != LUA_OK) {
			std::string Error = luaL_checklstring(L, -1, nullptr);
			lua_pop(L, 1);

			lua_pushnil(L);
			lua_pushstring(L, Error.data());

			return 2;
		}

		Closure* closure = clvalue(luaA_toobject(L, -1));
		Taskscheduler->SetProtoCapabilities(closure->l.p);
		return 1;
	}
	inline int makefolder(lua_State* L) {
		if (!fs::exists(workspace)) {
			std::error_code ec;
			fs::create_directories(workspace, ec);
			return makefolder(L);
		}
		luaL_checktype(L, 1, LUA_TSTRING);

		std::string path = lua_tostring(L, 1);
		std::replace(path.begin(), path.end(), '/', '\\');
		if (path.find("..") != std::string::npos) {
			lua_getglobal(L, "warn");
			lua_pushstring(L, "attempt to escape directory");
			lua_call(L, 1, 0);
			return 0;
		}

		std::filesystem::path workspacePath = workspace / path;
		std::filesystem::create_directory(workspacePath);

		return 0;
	}
	inline int readfile(lua_State* L) {
		if (!fs::exists(workspace)) {
			std::error_code ec;
			fs::create_directories(workspace, ec);
			return readfile(L);
		}

		luaL_checktype(L, 1, LUA_TSTRING);



		std::string path = lua_tostring(L, 1);
		std::replace(path.begin(), path.end(), '/', '\\');
		if (path.find("..") != std::string::npos) {
			lua_getglobal(L, "warn");
			lua_pushstring(L, "attempt to escape directory");
			lua_call(L, 1, 0);
			return 0;
		}

		std::filesystem::path workspacePath = workspace / path;
		const std::string extension = PathFindExtension(path.data());

		FILE* file = fopen(workspacePath.string().c_str(), "rb");
		if (!file) {
			luaL_error(L, "file does not exist!");
			return 0;
		}



		fseek(file, 0, SEEK_END);
		size_t fileSize = ftell(file);
		rewind(file);

		std::string content(fileSize, '\0');
		size_t bytesread = fread(&content[0], 1, fileSize, file);
		fclose(file);

		lua_pushstring(L, content.data());

		return 1;
	}
	inline int writefile(lua_State* L) {
		if (!fs::exists(workspace)) {
			std::error_code ec;
			fs::create_directories(workspace, ec);
			return writefile(L);
		}
		// Validate input arguments
		luaL_checktype(L, 1, LUA_TSTRING);
		luaL_checktype(L, 2, LUA_TSTRING);

		std::string path = lua_tostring(L, 1);

		// Use lua_tolstring to get exact length, preserving binary data
		size_t dataLength;
		const char* dataPtr = lua_tolstring(L, 2, &dataLength);
		std::string data(dataPtr, dataLength);

		// Normalize path separators to the current OS
		std::replace(path.begin(), path.end(), '/', '\\');

		// Prevent directory traversal
		if (path.find("..") != std::string::npos) {
			lua_getglobal(L, "warn");
			lua_pushstring(L, "Attempt to escape directory");
			lua_call(L, 1, 0);
			return 0;
		}

		// Construct the full path to the file
		std::filesystem::path workspacePath = workspace / path;

		// Get file extension and check if it is disallowed
		const std::string extension = PathFindExtension(path.c_str());
		for (const std::string& forbidden : disallowedExtensions) {
			if (equals_ignore_case(extension, forbidden)) {
				lua_getglobal(L, "warn");
				lua_pushstring(L, "Forbidden extension!");
				lua_call(L, 1, 0);
				return 0;
			}
		}

		// Check if the file path is too long (e.g., for Windows)
		const size_t maxPathLength = 260; // Common path length limit in Windows
		if (workspacePath.string().length() > maxPathLength) {
			lua_getglobal(L, "warn");
			lua_pushstring(L, "Path length exceeds system limit!");
			lua_call(L, 1, 0);
			return 0;
		}

		// Ensure parent directories exist
		std::filesystem::create_directories(workspacePath.parent_path());

		// Open the file for writing
		std::ofstream file(workspacePath, std::ios::binary);
		if (!file) {
			lua_getglobal(L, "warn");
			lua_pushstring(L, "Failed to open file for writing!");
			lua_call(L, 1, 0);
			return 0;
		}

		// Write data to the file in chunks to avoid buffer overflow
		const size_t chunkSize = 1024; // Write in 1 KB chunks
		size_t totalWritten = 0;
		while (totalWritten < data.size()) {
			size_t toWrite = min(chunkSize, data.size() - totalWritten);
			file.write(data.data() + totalWritten, toWrite);
			if (!file) {
				lua_getglobal(L, "warn");
				lua_pushstring(L, "Error writing to file!");
				lua_call(L, 1, 0);
				return 0;
			}
			totalWritten += toWrite;
		}

		// Successfully wrote to file
		return 0;
	}
}