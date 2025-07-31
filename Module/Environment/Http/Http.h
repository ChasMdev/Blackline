#pragma once
#include <lstate.h>
#include <lualib.h>
#include <lapi.h>
#include <lmem.h>
#include <lgc.h>
#include <Update/Offsets.hpp>
#include <zstd/include/zstd/zstd.h>
#include <zstd/include/zstd/xxhash.h>
#include <cryptopp/sha.h>
#include <cryptopp/hex.h>
#include <cryptopp/filters.h>
#include <cryptopp/cryptlib.h>
#include <Environment/Environment.hpp>
#include <regex>
#include <cryptopp/config_ns.h>
#include <cryptopp/base64.h>
#include <TaskScheduler/TaskScheduler.hpp>
#include <wininet.h>
#include <json.hpp>
#include <cpr/cpr.h>

#include "HttpStatus/HttpStatus.hpp"


namespace HelpFuncs {
    using YieldReturn = std::function<int(lua_State* L)>;

    static void ThreadFunc(const std::function<YieldReturn()>& YieldedFunction, lua_State* L)
    {
        YieldReturn ret_func;

        try
        {
            ret_func = YieldedFunction();
        }
        catch (std::exception ex)
        {
            lua_pushstring(L, ex.what());
            lua_error(L);
        }

        lua_State* l_new = lua_newthread(L);

        const auto returns = ret_func(L);

        lua_getglobal(l_new, ("task"));
        lua_getfield(l_new, -1, ("defer"));

        lua_pushthread(L);
        lua_xmove(L, l_new, 1);

        for (int i = returns; i >= 1; i--)
        {
            lua_pushvalue(L, -i);
            lua_xmove(L, l_new, 1);
        }

        lua_pcall(l_new, returns + 1, 0, 0);
        lua_settop(l_new, 0);
    }
    static void IsInstance(lua_State* L, int idx) {
        std::string typeoff = luaL_typename(L, idx);
        if (typeoff != ("Instance"))
            luaL_typeerrorL(L, 1, ("Instance"));
    };
    static int YieldExecution(lua_State* L, const std::function<YieldReturn()>& YieldedFunction)
    {
        lua_pushthread(L);
        lua_ref(L, -1);
        lua_pop(L, 1);

        std::thread(ThreadFunc, YieldedFunction, L).detach();

        L->base = L->top;
        L->status = LUA_YIELD;

        L->ci->flags |= 1;
        return -1;
    }
}

namespace Http {
    inline const std::string GetHwid() {
        HW_PROFILE_INFO hwProfileInfo;
        if (!GetCurrentHwProfile(&hwProfileInfo)) {
            return {};
        }

        CryptoPP::SHA256 sha256;
        unsigned char digest[CryptoPP::SHA256::DIGESTSIZE];
        sha256.CalculateDigest(digest, reinterpret_cast<unsigned char*>(hwProfileInfo.szHwProfileGuid),
            sizeof(hwProfileInfo.szHwProfileGuid));

        CryptoPP::HexEncoder encoder;
        std::string output;
        encoder.Attach(new CryptoPP::StringSink(output));
        encoder.Put(digest, sizeof(digest));
        encoder.MessageEnd();

        return output;
    }

    inline int gethwid(lua_State* LS)
    {
        std::string HardWareId = GetHwid();
        if (!HardWareId.empty())
            lua_pushstring(LS, HardWareId.c_str());
        else
            lua_pushstring(LS, "0000-0000-0000-0000-0000");

        return 1;
    };

    inline std::unordered_map<std::string, std::string> HttpGetCache;

    inline static int httpget(lua_State* L) {
        std::string url;

        if (!lua_isstring(L, 1)) {
            luaL_checkstring(L, 2);
            url = std::string(lua_tostring(L, 2));
        }
        else {
            url = std::string(lua_tostring(L, 1));
        }

        if (url.find("http://") == std::string::npos && url.find("https://") == std::string::npos) {
            luaL_argerror(L, 1, "Invalid protocol: expected 'http://' or 'https://'");
        }

        return HelpFuncs::YieldExecution(L, [url]() -> std::function<int(lua_State*)> {
            cpr::Header Headers;

            std::string GameId = std::to_string(0);
            std::string PlaceId = std::to_string(0);
            using json = nlohmann::json;
            json sessionIdJson;

            if (!GameId.empty() && !PlaceId.empty()) {
                sessionIdJson["GameId"] = GameId;
                sessionIdJson["PlaceId"] = PlaceId;
                Headers.insert({ "Roblox-Game-Id", GameId });
            }
            else {
                sessionIdJson["GameId"] = "none";
                sessionIdJson["PlaceId"] = "none";
                Headers.insert({ "Roblox-Game-Id", "none" });
            }

            Headers.insert({ "User-Agent", "Roblox/WinInet" });
            Headers.insert({ "Roblox-Session-Id", sessionIdJson.dump() });
            Headers.insert({ "Accept-Encoding", "identity" });

            cpr::Response Result;
            try {
                Result = cpr::Get(cpr::Url{ url }, cpr::Header(Headers));
            }
            catch (const std::exception& e) {
                return [e](lua_State* L) -> int {
                    lua_pushstring(L, ("HttpGet failed: " + std::string(e.what())).c_str());
                    return 1;
                    };
            }
            return [Result](lua_State* L) -> int {
                try {
                    if (Result.status_code == 0 || HttpStatus::IsError(Result.status_code)) {
                        auto Error = std::format("HttpGet failed with status {} - {}", Result.status_code, Result.error.message);
                        lua_pushstring(L, Error.c_str());
                        return 1;
                    }
                    if (Result.status_code == 401) {
                        lua_pushstring(L, "HttpGet failed with unauthorized access");
                        return 1;
                    }

                    // Fix: Use the raw bytes instead of text to preserve binary data
                    lua_pushlstring(L, Result.text.c_str(), Result.text.length());

                    return 1;

                }
                catch (...) {
                    lua_pushstring(L, "HttpGet failed");
                    return 1;
                }
            };
        });
    }

    static auto replace(std::string& str, const std::string& from, const std::string& to) -> bool
    {
        size_t start_pos = str.find(from);
        if (start_pos == std::string::npos)
            return false;
        str.replace(start_pos, from.length(), to);
        return true;
    }
    inline std::string replaceAll(std::string subject, const std::string& search, const std::string& replace)
    {
        size_t pos = 0;
        while ((pos = subject.find(search, pos)) != std::string::npos)
        {
            subject.replace(pos, search.length(), replace);
            pos += replace.length();
        }
        return subject;
    }
    inline int request(lua_State* L) {
        luaL_checktype(L, 1, LUA_TTABLE);

        // Fetch URL
        lua_getfield(L, 1, "Url");
        if (lua_type(L, -1) != LUA_TSTRING) {
            luaL_error(L, "Invalid or no 'Url' field specified in request table");
            return 0;
        }
        auto url = lua_tostring(L, -1);
        if (url == nullptr || strlen(url) == 0) {
            luaL_error(L, "'Url' field cannot be empty");
            return 0;
        }
        lua_pop(L, 1);

        // Fetch method (default to POST)
        lua_getfield(L, 1, "Method");
        auto method = std::string(luaL_optstring(L, -1, "POST"));
        lua_pop(L, 1);

        // Fetch body (if any)
        std::string body;
        lua_getfield(L, 1, "Body");
        if (lua_isstring(L, -1)) {
            body = luaL_checkstring(L, -1);
        }
        lua_pop(L, 1);

        // Fetch headers (if any)
        lua_getfield(L, 1, "Headers");
        bool hasHeaders = lua_istable(L, -1);
        lua_getglobal(L, "game");
        lua_getfield(L, -1, "JobId");
        std::string jobId = lua_tostring(L, -1);
        if (jobId.empty()) {
            luaL_error(L, "Invalid 'JobId' in game context");
            return 0;
        }
        lua_pop(L, 1);

        lua_getfield(L, -1, "GameId");
        std::string gameId = lua_tostring(L, -1);
        if (gameId.empty()) {
            luaL_error(L, "Invalid 'GameId' in game context");
            return 0;
        }
        lua_pop(L, 1);

        lua_getfield(L, -1, "PlaceId");
        std::string placeId = lua_tostring(L, -1);
        if (placeId.empty()) {
            luaL_error(L, "Invalid 'PlaceId' in game context");
            return 0;
        }
        lua_pop(L, 2);

        // Construct headers
        std::string headers = "Roblox-Session-Id: {\"GameId\":\"" + jobId + "\",\"PlaceId\":\"" + placeId + "\"}\r\n";
        headers += "Roblox-Game-Id: \"" + gameId + "\"\r\n";
        if (hasHeaders) {
            lua_pushnil(L);
            while (lua_next(L, -2)) {
                const char* headerKey = luaL_checkstring(L, -2);
                const char* headerValue = luaL_checkstring(L, -1);
                if (headerKey && headerValue) {
                    headers += headerKey;
                    headers += ": ";
                    headers += headerValue;
                    headers += "\r\n";
                }
                lua_pop(L, 1);
            }
        }
        lua_pop(L, 1);

        // Fetch cookies (if any)
        lua_getfield(L, 1, "Cookies");
        bool hasCookies = lua_istable(L, -1);
        std::string cookies;
        if (hasCookies) {
            lua_pushnil(L);
            while (lua_next(L, -2)) {
                const char* cookieName = luaL_checkstring(L, -2);
                const char* cookieValue = luaL_checkstring(L, -1);
                if (cookieName && cookieValue) {
                    cookies += cookieName;
                    cookies += "=";
                    cookies += cookieValue;
                    cookies += "; ";
                }
                lua_pop(L, 1);
            }
        }
        lua_pop(L, 1);

        headers += "User-Agent: Blackline\r\n";

        // HWID check
        HW_PROFILE_INFO hwProfileInfo;
        if (!GetCurrentHwProfile(&hwProfileInfo)) {
            luaL_error(L, "Failed to retrieve hardware profile.");
            return 0;
        }

        std::string hwid = hwProfileInfo.szHwProfileGuid;
        if (hwid.empty()) {
            luaL_error(L, "Invalid HWID.");
            return 0;
        }

        replace(hwid, "}", "");
        replace(hwid, "{", "");
        headers += "Fingerprint: Blackline-Fingerprint\r\n";
        if (!hwid.empty()) {
            headers += "Exploit-Guid: " + hwid + "\r\n";
            headers += "Blackline-Fingerprint: " + hwid + "\r\n";
        }
        else {
            headers += "Exploit-Guid: Unknown\r\n";
        }

        // Open Internet session
        HINTERNET hSession = InternetOpen("Blackline", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
        if (!hSession) {
            luaL_error(L, "Failed to open Internet session.");
            return 0;
        }

        HINTERNET hConnect = InternetOpenUrl(hSession, url, headers.c_str(), (DWORD)headers.length(), INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
        if (!hConnect) {
            InternetCloseHandle(hSession);
            luaL_error(L, "Failed to connect to URL.");
            return 0;
        }

        DWORD statusCode = 0;
        DWORD length = sizeof(DWORD);
        if (!HttpQueryInfo(hConnect, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &statusCode, &length, NULL)) {
            InternetCloseHandle(hConnect);
            InternetCloseHandle(hSession);
            luaL_error(L, "Failed to retrieve status code.");
            return 0;
        }

        // Prepare the response table
        lua_newtable(L);

        std::string responseText;
        char buffer[4096] = {};
        DWORD bytesRead;

        // Ensure that the reading from the connection is safe
        while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead != 0) {
            if (bytesRead > 0) {
                responseText.append(buffer, bytesRead);
            }
            else {
                break;
            }
        }

        lua_pushlstring(L, responseText.data(), responseText.length());
        lua_setfield(L, -2, "Body");

        lua_pushinteger(L, statusCode);
        lua_setfield(L, -2, "StatusCode");

        lua_pushboolean(L, statusCode >= 200 && statusCode < 300);
        lua_setfield(L, -2, "Success");

        // Add headers to the response
        lua_newtable(L);
        if (hasHeaders) {
            lua_pushlstring(L, "Headers", 7);
            lua_newtable(L);
            lua_pushlstring(L, headers.c_str(), headers.length());
            lua_setfield(L, -2, "AllHeaders");
            lua_settable(L, -3);
        }
        lua_setfield(L, -2, "Headers");

        // Clean up
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hSession);

        return 1;
    }

    inline int decompile(lua_State* L)
    {
        Roblox::Print(1, "decompile called");
        HelpFuncs::IsInstance(L, 1);
        //HelpFuncs::IsClassName(L, 1, "LocalScript");
        std::string bytecode = Roblox::RequestBytecode(*(uintptr_t*)lua_topointer(L, 1));

        auto response = cpr::Post(
            cpr::Url{ "http://api.plusgiant5.com/konstant/decompile" },
            cpr::Body{ bytecode },
            cpr::Header{ {"Content-Type", "text/plain"} }
        );

        while (response.status_code != 200)
            Sleep(50);

        std::vector<std::string> lines;
        std::stringstream ss(response.text);
        std::string line;

        while (std::getline(ss, line))
            lines.push_back(line);

        std::string output = "-- Decompiled with XW / V2 \n\n";
        for (size_t i = 5; i < lines.size(); ++i)
            output += lines[i] + "\n";

        size_t pos = 0;
        while ((pos = output.find("\t", pos)) != std::string::npos)
        {
            output.replace(pos, 1, "    ");
            pos += 4;
        }

        lua_pushstring(L, output.c_str());
        return 1;
    };



};