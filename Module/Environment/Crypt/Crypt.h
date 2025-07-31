#pragma once
#include <cstdlib>
#include <cstring>
#include <string>
#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#include <cryptopp/base64.h>
#include <cryptopp/sha.h>
#include <cryptopp/md5.h>
#include <cryptopp/sha3.h>
#include <cryptopp/rdrand.h>
#include <cryptopp/hex.h>
#include <cryptopp/filters.h>
#include <lua.h>

#include "ldebug.h"
#include "lualib.h"
#include "lz4/lz4.h"

template<typename T>
static std::string hash_with_algo(const std::string& Input)
{
	T Hash;
	std::string Digest;

	CryptoPP::StringSource SS(Input, true,
		new CryptoPP::HashFilter(Hash,
			new CryptoPP::HexEncoder(
				new CryptoPP::StringSink(Digest), false
			)));

	return Digest;
}
inline std::string base64encode(const std::string& stringToEncode)
{
	std::string base64EncodedString;
	CryptoPP::Base64Encoder encoder{ new CryptoPP::StringSink(base64EncodedString), false };
	encoder.Put((byte*)stringToEncode.c_str(), stringToEncode.length());
	encoder.MessageEnd();

	return base64EncodedString;
}
inline std::string base64decode(const std::string& stringToDecode) {
	std::string base64DecodedString;
	CryptoPP::Base64Decoder decoder{ new CryptoPP::StringSink(base64DecodedString) };
	decoder.Put((byte*)stringToDecode.c_str(), stringToDecode.length());
	decoder.MessageEnd();

	return base64DecodedString;
}
inline std::optional<std::unique_ptr<CryptoPP::HashTransformation> > getHashAlgorithm(const std::string& algorithmName) {
	if (algorithmName == "sha1") {
		return std::make_unique<CryptoPP::SHA1>();
	}
	else if (algorithmName == "sha384") {
		return std::make_unique<CryptoPP::SHA384>();
	}
	else if (algorithmName == "sha512") {
		return std::make_unique<CryptoPP::SHA512>();
	}
	else if (algorithmName == "md5") {
		return std::make_unique<CryptoPP::MD5>();
	}
	else if (algorithmName == "sha256") {
		return std::make_unique<CryptoPP::SHA256>();
	}
	else if (algorithmName == "sha3-224") {
		return std::make_unique<CryptoPP::SHA3_224>();
	}
	else if (algorithmName == "sha3-256") {
		return std::make_unique<CryptoPP::SHA3_256>();
	}
	else if (algorithmName == "sha3-512") {
		return std::make_unique<CryptoPP::SHA3_512>();
	}
	else {
		return std::nullopt;
	}
}

namespace Crypt
{
	inline int base64_encode(lua_State* L)
	{
		luaL_checktype(L, 1, LUA_TSTRING);
		size_t stringLength;
		const char* rawStringToEncode = lua_tolstring(L, 1, &stringLength);
		const std::string stringToEncode(rawStringToEncode, stringLength);
		const std::string encodedString = base64encode(stringToEncode);

		lua_pushlstring(L, encodedString.c_str(), encodedString.size());
		return 1;
	}
	inline int base64_decode(lua_State* L) {
		luaL_checktype(L, 1, LUA_TSTRING);
		size_t stringLength;
		const char* rawStringToDecode = lua_tolstring(L, 1, &stringLength);
		const auto stringToDecode = std::string(rawStringToDecode, stringLength);
		const std::string decodedString = base64decode(stringToDecode);

		lua_pushlstring(L, decodedString.c_str(), decodedString.size());
		return 1;
	}
	inline int generatebytes(lua_State* L) {
		luaL_checktype(L, 1, LUA_TNUMBER);
		const auto bytesSize = lua_tointeger(L, 1);

		CryptoPP::RDRAND rng;
		const auto bytesBuffer = new byte[bytesSize];
		rng.GenerateBlock(bytesBuffer, bytesSize);

		std::string base64EncodedBytes;
		CryptoPP::Base64Encoder encoder{ new CryptoPP::StringSink(base64EncodedBytes), false };
		encoder.Put(bytesBuffer, bytesSize);
		encoder.MessageEnd();

		delete bytesBuffer;
		lua_pushlstring(L, base64EncodedBytes.c_str(), base64EncodedBytes.size());
		return 1;
	}
	inline int generatekey(lua_State* L) {
		const auto bytesBuffer = new byte[CryptoPP::AES::MAX_KEYLENGTH];

		CryptoPP::RDRAND rng;
		rng.GenerateBlock(bytesBuffer, CryptoPP::AES::MAX_KEYLENGTH);

		std::string base64EncodedBytes;
		CryptoPP::Base64Encoder encoder{ new CryptoPP::StringSink(base64EncodedBytes), false };
		encoder.Put(bytesBuffer, CryptoPP::AES::MAX_KEYLENGTH);
		encoder.MessageEnd();

		delete bytesBuffer;
		lua_pushlstring(L, base64EncodedBytes.c_str(), base64EncodedBytes.size());
		return 1;
	}
	inline int hash(lua_State* L) {
		luaL_checktype(L, 1, LUA_TSTRING);
		luaL_checktype(L, 2, LUA_TSTRING);

		size_t dataLength;
		const auto dataString = lua_tolstring(L, 1, &dataLength);
		std::string algorithName = lua_tostring(L, 2);
		std::ranges::transform(algorithName, algorithName.begin(),
			[](const unsigned char c) { return tolower(c); });

		const auto hashAlgorithmOpt = getHashAlgorithm(algorithName);
		if (!hashAlgorithmOpt.has_value())
			luaL_argerrorL(L, 2, "Invalid hash algorithm provided!");
		const auto& hashAlgorithm = hashAlgorithmOpt.value();

		std::string hashDigest;
		hashDigest.resize(hashAlgorithm->DigestSize());

		hashAlgorithm->Update((byte*)dataString, dataLength);
		hashAlgorithm->Final((byte*)&hashDigest[0]);

		std::string finalHash;
		CryptoPP::HexEncoder encoder;
		encoder.Attach(new CryptoPP::StringSink(finalHash));
		encoder.Put((byte*)hashDigest.data(), hashDigest.size());
		encoder.MessageEnd();

		lua_pushlstring(L, finalHash.c_str(), finalHash.length());
		return 1;
	}
	using ModePair = std::pair<std::unique_ptr<CryptoPP::CipherModeBase>, std::unique_ptr<CryptoPP::CipherModeBase> >;
	inline std::optional<ModePair> getEncryptionDecryptionMode(const std::string& modeName) {
		if (modeName == "cbc") {
			return ModePair{
				std::make_unique<CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption>(),
				std::make_unique<CryptoPP::CBC_Mode<CryptoPP::AES>::Decryption>()
			};
		}
		else if (modeName == "cfb") {
			return ModePair{
				std::make_unique<CryptoPP::CFB_Mode<CryptoPP::AES>::Encryption>(),
				std::make_unique<CryptoPP::CFB_Mode<CryptoPP::AES>::Decryption>()
			};
		}
		else if (modeName == "ofb") {
			return ModePair{
				std::make_unique<CryptoPP::OFB_Mode<CryptoPP::AES>::Encryption>(),
				std::make_unique<CryptoPP::OFB_Mode<CryptoPP::AES>::Decryption>()
			};
		}
		else if (modeName == "ctr") {
			return ModePair{
				std::make_unique<CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption>(),
				std::make_unique<CryptoPP::CTR_Mode<CryptoPP::AES>::Decryption>()
			};
		}
		else if (modeName == "ecb") {
			return ModePair{
				std::make_unique<CryptoPP::ECB_Mode<CryptoPP::AES>::Encryption>(),
				std::make_unique<CryptoPP::ECB_Mode<CryptoPP::AES>::Decryption>()
			};
		}
		else {
			return std::nullopt;
		}
	}
	inline int encrypt(lua_State* L) {
		luaL_checktype(L, 1, LUA_TSTRING);
		luaL_checktype(L, 2, LUA_TSTRING);

		size_t rawDataLength;
		const auto rawDataString = lua_tolstring(L, 1, &rawDataLength);
		std::string encryptionKey = lua_tostring(L, 2);
		std::string optionalIv = luaL_optstring(L, 3, "_NO_IV_PROVIDED_");
		std::string optionalAesCipherMode = luaL_optstring(L, 4, "cbc");
		bool wasIvGenerated = false;

		encryptionKey = base64decode(encryptionKey);
		if (encryptionKey.size() != CryptoPP::AES::MAX_KEYLENGTH)
			luaL_argerrorL(L, 2, "Key must be 256 bits (32 bytes)");

		if (optionalIv == "_NO_IV_PROVIDED_") {
			optionalIv.resize(CryptoPP::AES::BLOCKSIZE);

			CryptoPP::RDRAND rng;
			rng.GenerateBlock((byte*)&optionalIv[0], CryptoPP::AES::BLOCKSIZE);
			wasIvGenerated = true;
		}
		else {
			optionalIv = base64decode(optionalIv);
		}

		std::ranges::transform(optionalAesCipherMode, optionalAesCipherMode.begin(),
			[](const unsigned char c) { return tolower(c); });
		const auto encryptionModeOpt = getEncryptionDecryptionMode(optionalAesCipherMode);
		if (!encryptionModeOpt.has_value())
			luaL_argerrorL(L, 4, "Invalid encryption mode provided!");
		const auto& encryptionMode = encryptionModeOpt.value().first;
		encryptionMode->SetKeyWithIV((byte*)encryptionKey.data(), encryptionKey.size(), (byte*)optionalIv.data(),
			optionalIv.size());

		std::string encryptedData;
		CryptoPP::StreamTransformationFilter encryptor(*encryptionMode, new CryptoPP::StringSink(encryptedData));
		encryptor.Put((byte*)rawDataString, rawDataLength);
		encryptor.MessageEnd();

		encryptedData = base64encode(encryptedData);
		lua_pushlstring(L, encryptedData.c_str(), encryptedData.length());
		if (!wasIvGenerated)
			return 1;

		optionalIv = base64encode(optionalIv);
		lua_pushlstring(L, optionalIv.c_str(), optionalIv.length());
		return 2;
	}
	inline int decrypt(lua_State* L) {
		luaL_checktype(L, 1, LUA_TSTRING);
		luaL_checktype(L, 2, LUA_TSTRING);
		luaL_checktype(L, 3, LUA_TSTRING);
		luaL_checktype(L, 4, LUA_TSTRING);

		std::string dataToDecrypt = lua_tostring(L, 1);
		std::string decryptionKey = lua_tostring(L, 2);
		std::string IV = lua_tostring(L, 3);
		std::string decryptionModeName = lua_tostring(L, 4);

		dataToDecrypt = base64decode(dataToDecrypt);
		decryptionKey = base64decode(decryptionKey);
		IV = base64decode(IV);
		if (decryptionKey.size() != CryptoPP::AES::MAX_KEYLENGTH)
			luaL_argerrorL(L, 2, "Key must be 256 bits (32 bytes)");

		std::ranges::transform(decryptionModeName, decryptionModeName.begin(),
			[](const unsigned char c) { return tolower(c); });
		const auto decryptionModeOpt = getEncryptionDecryptionMode(decryptionModeName);
		if (!decryptionModeOpt.has_value())
			luaL_argerrorL(L, 4, "Invalid decryption mode provided");
		const auto& decryptionMode = decryptionModeOpt.value().second;
		decryptionMode->SetKeyWithIV((byte*)decryptionKey.data(), decryptionKey.size(), (byte*)IV.data(), IV.size());

		std::string decryptedData;
		CryptoPP::StreamTransformationFilter decryptor(*decryptionMode, new CryptoPP::StringSink(decryptedData));
		decryptor.Put((byte*)dataToDecrypt.data(), dataToDecrypt.size());
		decryptor.MessageEnd();

		lua_pushlstring(L, decryptedData.c_str(), decryptedData.length());
		return 1;
	}
	inline int lz4compress(lua_State* L) {
		luaL_checktype(L, 1, LUA_TSTRING);
		size_t stringLength;
		const auto stringData = lua_tolstring(L, 1, &stringLength);
		const auto dataToCompress = std::string(stringData, stringLength);

		const auto maxCompressedSize = LZ4_compressBound(dataToCompress.size());
		std::vector<char> compressedBuffer(maxCompressedSize);
		const auto compressedSize = LZ4_compress_default(dataToCompress.c_str(), compressedBuffer.data(),
			dataToCompress.size(), maxCompressedSize);
		if (compressedSize <= 0)
			luaG_runerrorL(L, "Compression failed");

		lua_pushlstring(L, compressedBuffer.data(), compressedSize);
		return 1;
	}
	inline int lz4decompress(lua_State* L) {
		luaL_checktype(L, 1, LUA_TSTRING);
		luaL_checktype(L, 2, LUA_TNUMBER);
		size_t compressedLength;
		const auto compressedData = lua_tolstring(L, 1, &compressedLength);
		const auto compressedString = std::string(compressedData, compressedLength);
		const auto originalSize = lua_tonumber(L, 2);
		if (originalSize <= 0)
			luaL_argerrorL(L, 2, "Invalid original size");

		std::vector<char> decompressedBuffer(originalSize);
		const auto decompressedSize = LZ4_decompress_safe(
			compressedString.c_str(),
			decompressedBuffer.data(),
			compressedLength,
			originalSize
		);
		if (decompressedSize <= 0)
			luaG_runerrorL(L, "Failed to decompress");

		lua_pushlstring(L, decompressedBuffer.data(), decompressedSize);
		return 1;
	}
}