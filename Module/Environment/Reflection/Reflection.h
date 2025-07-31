#pragma once
#include "ldebug.h"
#include "lua.h"
#include "lualib.h"

namespace RBX
{

	enum FastVarType : __int32
	{
		FASTVARTYPE_INVALID = 0x0,
		FASTVARTYPE_STATIC = 0x1,
		FASTVARTYPE_DYNAMIC = 0x2,
		FASTVARTYPE_SYNC = 0x4,
		FASTVARTYPE_AB_NEWUSERS = 0x8,
		FASTVARTYPE_AB_NEWSTUDIOUSERS = 0x10,
		FASTVARTYPE_AB_ALLUSERS = 0x20,
		FASTVARTYPE_LOCAL_LOCKED = 0x40,
		FASTVARTYPE_ANY = 0x7F,
	};

	struct LiveThreadRef {
		int __atomic_refs;
		lua_State* Thread;
		int ThreadId;
		int RefId;
	};

	struct WeakObjectRef {
	};

	struct WeakThreadRef {
		//lua_State* thread;
		std::uint8_t pad_0[16];

		WeakThreadRef* Previous;
		WeakThreadRef* Next;
		LiveThreadRef* LiveThreadRef;
		struct Node_t* Node;

		std::uint8_t pad_1[8];
	};

	struct Node {
		std::uint8_t pad_0[8];

		WeakThreadRef* WeakThreadRef;
	};

	struct ScriptVmState {
		void* Fill;
	};

	struct ExtraSpace {
		struct Shared {
			int32_t ThreadCount;
			void* ScriptContext;
			void* ScriptVmState;
			char field_18[0x8];
			void* __intrusive_set_AllThreads;
		};

		char _0[8];
		char _8[8];
		char _10[8];
		struct Shared* SharedExtraSpace;
	};

	struct WeakThreadRef2 {
		std::atomic< std::int32_t > _refs;
		lua_State* Thread;
		std::int32_t ThreadRef;
		std::int32_t ObjectId;
		std::int32_t unk1;
		std::int32_t unk2;

		WeakThreadRef2(lua_State* L)
			: Thread(L), ThreadRef(NULL), ObjectId(NULL), unk1(NULL), unk2(NULL) {
		};
	};

	struct DebuggerResult {
		std::int32_t Result;
		std::int32_t unk[0x4];
	};

	namespace Concepts
	{
		template<typename Derived, typename Base>
		concept _TypeConstraint = std::is_base_of_v<Base, Derived>;
	}

	namespace Security
	{
		enum Permissions : uint32_t
		{
			None = 0x0,
			Plugin = 0x1,
			LocalUser = 0x3,
			WritePlayer = 0x4,
			RobloxScript = 0x5,
			RobloxEngine = 0x6,
			NotAccessible = 0x7,
			TestLocalUser = 0x3
		};
	}

	struct PeerId
	{
		unsigned int peerId;
	};

	const struct SystemAddress
	{
		RBX::PeerId remoteId;
	};

	namespace Reflection
	{
		namespace EventSource {

			enum EventTargetInclusion : __int32
			{
				OnlyTarget = 0x0,
				ExcludeTarget = 0x1,
			};

			const struct __declspec(align(8)) RemoteEventInvocationTargetOptions
			{
				const RBX::SystemAddress* target;
				RBX::Reflection::EventSource::EventTargetInclusion isExcludeTarget;
			};
		}
		const struct type_holder
		{
			void(__fastcall* construct)(const char*, char*);
			void(__fastcall* moveConstruct)(char*, char*);
			void(__fastcall* destruct)(char*);
		};

		enum ReflectionType : uint32_t
		{
			ReflectionType_Void = 0x0,
			ReflectionType_Bool = 0x1,
			ReflectionType_Int = 0x2,
			ReflectionType_Int64 = 0x3,
			ReflectionType_Float = 0x4,
			ReflectionType_Double = 0x5,
			ReflectionType_String = 0x6,
			ReflectionType_ProtectedString = 0x7,
			ReflectionType_Instance = 0x8,
			ReflectionType_Instances = 0x9,
			ReflectionType_Ray = 0xa,
			ReflectionType_Vector2 = 0xb,
			ReflectionType_Vector3 = 0xc,
			ReflectionType_Vector2Int16 = 0xd,
			ReflectionType_Vector3Int16 = 0xe,
			ReflectionType_Rect2d = 0xf,
			ReflectionType_CoordinateFrame = 0x10,
			ReflectionType_Color3 = 0x11,
			ReflectionType_Color3uint8 = 0x12,
			ReflectionType_UDim = 0x13,
			ReflectionType_UDim2 = 0x14,
			ReflectionType_Faces = 0x15,
			ReflectionType_Axes = 0x16,
			ReflectionType_Region3 = 0x17,
			ReflectionType_Region3Int16 = 0x18,
			ReflectionType_CellId = 0x19,
			ReflectionType_GuidData = 0x1a,
			ReflectionType_PhysicalProperties = 0x1b,
			ReflectionType_BrickColor = 0x1c,
			ReflectionType_SystemAddress = 0x1d,
			ReflectionType_BinaryString = 0x1e,
			ReflectionType_Surface = 0x1f,
			ReflectionType_Enum = 0x20,
			ReflectionType_Property = 0x21,
			ReflectionType_Tuple = 0x22,
			ReflectionType_ValueArray = 0x23,
			ReflectionType_ValueTable = 0x24,
			ReflectionType_ValueMap = 0x25,
			ReflectionType_Variant = 0x26,
			ReflectionType_GenericFunction = 0x27,
			ReflectionType_WeakFunctionRef = 0x28,
			ReflectionType_ColorSequence = 0x29,
			ReflectionType_ColorSequenceKeypoint = 0x2a,
			ReflectionType_NumberRange = 0x2b,
			ReflectionType_NumberSequence = 0x2c,
			ReflectionType_NumberSequenceKeypoint = 0x2d,
			ReflectionType_InputObject = 0x2e,
			ReflectionType_Connection = 0x2f,
			ReflectionType_ContentId = 0x30,
			ReflectionType_DescribedBase = 0x31,
			ReflectionType_RefType = 0x32,
			ReflectionType_QFont = 0x33,
			ReflectionType_QDir = 0x34,
			ReflectionType_EventInstance = 0x35,
			ReflectionType_TweenInfo = 0x36,
			ReflectionType_DockWidgetPluginGuiInfo = 0x37,
			ReflectionType_PluginDrag = 0x38,
			ReflectionType_Random = 0x39,
			ReflectionType_PathWaypoint = 0x3a,
			ReflectionType_FloatCurveKey = 0x3b,
			ReflectionType_RotationCurveKey = 0x3c,
			ReflectionType_SharedString = 0x3d,
			ReflectionType_DateTime = 0x3e,
			ReflectionType_RaycastParams = 0x3f,
			ReflectionType_RaycastResult = 0x40,
			ReflectionType_OverlapParams = 0x41,
			ReflectionType_LazyTable = 0x42,
			ReflectionType_DebugTable = 0x43,
			ReflectionType_CatalogSearchParams = 0x44,
			ReflectionType_OptionalCoordinateFrame = 0x45,
			ReflectionType_CSGPropertyData = 0x46,
			ReflectionType_UniqueId = 0x47,
			ReflectionType_Font = 0x48,
			ReflectionType_Blackboard = 0x49,
			ReflectionType_Max = 0x4a
		};

		struct ClassDescriptor;
		struct Descriptor
		{
			enum ThreadSafety : uint32_t { Unset = 0x0, Unsafe = 0x1, ReadSafe = 0x3, LocalSafe = 0x7, Safe = 0xf };
			struct Attributes
			{
				bool isDeprecated;
				class RBX::Reflection::Descriptor* preferred;
				enum RBX::Reflection::Descriptor::ThreadSafety threadSafety;
			};
			void* vftable;
			std::string& name;
			struct RBX::Reflection::Descriptor::Attributes attributes;
		};

		struct Type : RBX::Reflection::Descriptor
		{
			std::string& tag;
			char _dd[0x8];
			enum RBX::Reflection::ReflectionType reflectionType;
			bool isFloat;
			bool isNumber;
			bool isEnum;
			bool isOptional;
		};

		struct Storage
		{
			const type_holder* holder;
			char data[64];
		};

		const struct Variant
		{
			const Type* _type;
			Storage value;
		};

		const struct Tuple
		{
			std::vector<RBX::Reflection::Variant, std::allocator<RBX::Reflection::Variant> > values;
		};

		struct TupleBase
		{
			RBX::Reflection::Tuple* _Ptr;
			std::_Ref_count_base* _Rep;
		};

		struct TupleShared : TupleBase {};

		struct EnumDescriptor : RBX::Reflection::Type
		{
			std::vector<void*> allItems;
			std::uint64_t enumCount;
			const char _0[0x60];
		};

		struct MemberDescriptor : RBX::Reflection::Descriptor
		{
			std::string& category;
			class RBX::Reflection::ClassDescriptor* owner;
			enum RBX::Security::Permissions permissions;
			int32_t _0;
		};

		struct EventDescriptor : RBX::Reflection::MemberDescriptor {};

		struct PropertyDescriptorVFT {};

		struct PropertyDescriptor : RBX::Reflection::MemberDescriptor
		{
		public:
			union {
				uint32_t bIsEditable;
				uint32_t bCanReplicate;
				uint32_t bCanXmlRead;
				uint32_t bCanXmlWrite;
				uint32_t bAlwaysClone;
				uint32_t bIsScriptable;
				uint32_t bIsPublic;
			} __bitfield;
			char _dd[0x8];
			RBX::Reflection::Type* type;
			bool bIsEnum;
			RBX::Security::Permissions scriptWriteAccess;

			bool IsScriptable() { return (this->__bitfield.bIsScriptable > 0x20) & 1; }

			void SetScriptable(const uint8_t bIsScriptable)
			{
				this->__bitfield.bIsScriptable = (this->__bitfield.bIsScriptable) ^ (bIsScriptable ? 33 : 32);
			}

			bool IsEditable() { return ((this->__bitfield.bIsEditable) & 1); }

			void SetEditable(const uint8_t bIsEditable)
			{
				this->__bitfield.bIsEditable = (this->__bitfield.bIsEditable) ^ ((~bIsEditable & 0xFF));
			}

			bool IsCanXmlRead() { return ((this->__bitfield.bCanXmlRead >> 3) & 1); }
			void SetCanXmlRead(const uint8_t bCanXmlRead)
			{
				this->__bitfield.bCanXmlRead = (this->__bitfield.bCanXmlRead) ^ ((~bCanXmlRead & 0xFF << 3));
			}

			bool IsCanXmlWrite() { return ((this->__bitfield.bCanXmlWrite >> 4) & 1); }
			void SetCanXmlWrite(const uint8_t bCanXmlWrite)
			{
				this->__bitfield.bCanXmlWrite = (this->__bitfield.bCanXmlWrite) ^ ((~bCanXmlWrite & 0xFF << 4));
			}

			bool IsPublic() { return ((this->__bitfield.bIsPublic >> 6) & 1); }
			void SetIsPublic(const uint8_t bIsPublic)
			{
				this->__bitfield.bIsPublic =
					(this->__bitfield.bIsPublic) ^ static_cast<uint32_t>(~bIsPublic & 0xFF << 6);
			}

			bool IsCanReplicate() { return ((this->__bitfield.bCanReplicate >> 2) & 1); }
			void SetCanReplicate(const uint8_t bCanReplicate)
			{
				this->__bitfield.bCanReplicate = (this->__bitfield.bCanReplicate) ^ ((~bCanReplicate & 0xFF << 2));
			}

			bool IsAlwaysClone() { return ((this->__bitfield.bAlwaysClone) & 1); }
			void SetAlwaysClone(const uint8_t bAlwaysClone)
			{
				this->__bitfield.bAlwaysClone = (this->__bitfield.bAlwaysClone) ^ (~bAlwaysClone & 0xFF);
			}

			PropertyDescriptorVFT* GetVFT() { return static_cast<PropertyDescriptorVFT*>(this->vftable); }
		};

		struct EnumPropertyDescriptor : RBX::Reflection::PropertyDescriptor
		{
			RBX::Reflection::EnumDescriptor* enumDescriptor;
		};

		template<Concepts::_TypeConstraint<RBX::Reflection::MemberDescriptor> U>
		struct MemberDescriptorContainer
		{
			std::vector<U*> descriptors;
			const char _0[144];
		};

		struct ClassDescriptor : RBX::Reflection::Descriptor
		{
			char _dd[0x8];
			MemberDescriptorContainer<RBX::Reflection::PropertyDescriptor> propertyDescriptors;
			MemberDescriptorContainer<RBX::Reflection::EventDescriptor> eventDescriptors;
			void* boundFunctionDescription_Start;
			char _180[0x40];
			char _1c0[0x40];
			char _200[0x20];
			void* boundYieldFunctionDescription_Start;
			char _228[0x18];
			char _240[0x40];
			char _280[0x40];
			char _2c0[8];
			char _2c8[0x38];
			char _300[0x40];
			char _340[0x30];
			char _370[8];
			char _378[8];
			char _380[4];
			char _384[2];
			char _386[2];
			char _388[8];
			struct RBX::Reflection::ClassDescriptor* baseClass;
			char _398[8];
			char _3a0[8];
			char _3a8[0x18];
			char _3c0[0x20];
		};
	}

	struct Instance
	{
		void* vftable;
		std::weak_ptr<RBX::Instance> self;
		RBX::Reflection::ClassDescriptor* classDescriptor;
	};

	struct shared_string_t
	{
		std::string content;  // Roblox uses std::string internally
		// Other internal fields may exist but content is what we access

		// Constructors
		shared_string_t() = default;

		shared_string_t(const char* str) : content(str) {}

		shared_string_t(const std::string& str) : content(str) {}

		shared_string_t(const char* str, size_t len) : content(str, len) {}

		// Copy constructor
		shared_string_t(const shared_string_t& other) : content(other.content) {}

		// Assignment operator
		shared_string_t& operator=(const shared_string_t& other)
		{
			content = other.content;
			return *this;
		}

		shared_string_t& operator=(const std::string& str)
		{
			content = str;
			return *this;
		}

		shared_string_t& operator=(const char* str)
		{
			content = str;
			return *this;
		}

		// Comparison operators
		bool operator==(const shared_string_t& other) const
		{
			return content == other.content;
		}

		bool operator!=(const shared_string_t& other) const
		{
			return content != other.content;
		}

		// String access
		const char* c_str() const { return content.c_str(); }
		size_t size() const { return content.size(); }
		size_t length() const { return content.length(); }
		bool empty() const { return content.empty(); }
	};

	// System address structure matching Roblox's network implementation
	struct system_address_t
	{
		struct {
			uint64_t peer_id;  // This is what Roblox accesses as remote_id.peer_id
			// Additional fields may exist
		} remote_id;

		// Constructors
		system_address_t()
		{
			remote_id.peer_id = 0;
		}

		system_address_t(uint64_t peer_id)
		{
			remote_id.peer_id = peer_id;
		}

		// Copy constructor
		system_address_t(const system_address_t& other)
		{
			remote_id.peer_id = other.remote_id.peer_id;
		}

		// Assignment operator
		system_address_t& operator=(const system_address_t& other)
		{
			remote_id.peer_id = other.remote_id.peer_id;
			return *this;
		}

		system_address_t& operator=(uint64_t peer_id)
		{
			remote_id.peer_id = peer_id;
			return *this;
		}

		// Comparison operators
		bool operator==(const system_address_t& other) const
		{
			return remote_id.peer_id == other.remote_id.peer_id;
		}

		bool operator!=(const system_address_t& other) const
		{
			return remote_id.peer_id != other.remote_id.peer_id;
		}

		// Check if address is valid
		bool is_valid() const
		{
			return remote_id.peer_id != 0;
		}

		// String representation
		std::string to_string() const
		{
			return "PeerID: " + std::to_string(remote_id.peer_id);
		}
	};

	// Vector3 structure matching Roblox's coordinate system
	struct Vector3
	{
		float X, Y, Z;  // Roblox uses capital X, Y, Z

		// Constructors
		Vector3() : X(0.0f), Y(0.0f), Z(0.0f) {}
		Vector3(float x, float y, float z) : X(x), Y(y), Z(z) {}
		Vector3(const Vector3& other) : X(other.X), Y(other.Y), Z(other.Z) {}

		// Assignment operator
		Vector3& operator=(const Vector3& other)
		{
			X = other.X;
			Y = other.Y;
			Z = other.Z;
			return *this;
		}

		// Static constants
		static const Vector3 Zero() { return Vector3(0.0f, 0.0f, 0.0f); }
		static const Vector3 One() { return Vector3(1.0f, 1.0f, 1.0f); }
		static const Vector3 UnitX() { return Vector3(1.0f, 0.0f, 0.0f); }
		static const Vector3 UnitY() { return Vector3(0.0f, 1.0f, 0.0f); }
		static const Vector3 UnitZ() { return Vector3(0.0f, 0.0f, 1.0f); }

		// Arithmetic operators
		Vector3 operator+(const Vector3& other) const
		{
			return Vector3(X + other.X, Y + other.Y, Z + other.Z);
		}

		Vector3 operator-(const Vector3& other) const
		{
			return Vector3(X - other.X, Y - other.Y, Z - other.Z);
		}

		Vector3 operator*(float scalar) const
		{
			return Vector3(X * scalar, Y * scalar, Z * scalar);
		}

		Vector3 operator*(const Vector3& other) const
		{
			return Vector3(X * other.X, Y * other.Y, Z * other.Z);
		}

		Vector3 operator/(float scalar) const
		{
			return Vector3(X / scalar, Y / scalar, Z / scalar);
		}

		Vector3 operator/(const Vector3& other) const
		{
			return Vector3(X / other.X, Y / other.Y, Z / other.Z);
		}

		Vector3 operator-() const
		{
			return Vector3(-X, -Y, -Z);
		}

		// Assignment operators
		Vector3& operator+=(const Vector3& other)
		{
			X += other.X; Y += other.Y; Z += other.Z;
			return *this;
		}

		Vector3& operator-=(const Vector3& other)
		{
			X -= other.X; Y -= other.Y; Z -= other.Z;
			return *this;
		}

		Vector3& operator*=(float scalar)
		{
			X *= scalar; Y *= scalar; Z *= scalar;
			return *this;
		}

		Vector3& operator/=(float scalar)
		{
			X /= scalar; Y /= scalar; Z /= scalar;
			return *this;
		}

		// Comparison operators
		bool operator==(const Vector3& other) const
		{
			const float epsilon = 1e-6f;
			return std::abs(X - other.X) < epsilon &&
				std::abs(Y - other.Y) < epsilon &&
				std::abs(Z - other.Z) < epsilon;
		}

		bool operator!=(const Vector3& other) const
		{
			return !(*this == other);
		}

		// Vector operations
		float magnitude() const
		{
			return std::sqrt(X * X + Y * Y + Z * Z);
		}

		float magnitude_squared() const
		{
			return X * X + Y * Y + Z * Z;
		}

		Vector3 normalized() const
		{
			float mag = magnitude();
			return mag > 0.0f ? (*this / mag) : Vector3::Zero();
		}

		void normalize()
		{
			*this = normalized();
		}

		float dot(const Vector3& other) const
		{
			return X * other.X + Y * other.Y + Z * other.Z;
		}

		Vector3 cross(const Vector3& other) const
		{
			return Vector3(
				Y * other.Z - Z * other.Y,
				Z * other.X - X * other.Z,
				X * other.Y - Y * other.X
			);
		}

		float distance(const Vector3& other) const
		{
			return (*this - other).magnitude();
		}

		float distance_squared(const Vector3& other) const
		{
			return (*this - other).magnitude_squared();
		}

		// Linear interpolation
		Vector3 lerp(const Vector3& other, float t) const
		{
			return *this + (other - *this) * t;
		}

		// String representation
		std::string to_string() const
		{
			return "(" + std::to_string(X) + ", " + std::to_string(Y) + ", " + std::to_string(Z) + ")";
		}

		// Array access (using capital coordinates)
		float& operator[](int index)
		{
			return (&X)[index];
		}

		const float& operator[](int index) const
		{
			return (&X)[index];
		}

		// Compatibility with lowercase access
		float x() const { return X; }
		float y() const { return Y; }
		float z() const { return Z; }

		void set_x(float value) { X = value; }
		void set_y(float value) { Y = value; }
		void set_z(float value) { Z = value; }
	};

	// Scalar multiplication (allows float * Vector3)
	inline Vector3 operator*(float scalar, const Vector3& vec)
	{
		return vec * scalar;
	}
}

inline std::vector<std::tuple<uintptr_t, std::string, bool>> script_able_cache;
inline std::vector<std::pair<std::string, bool>> default_property_states;

inline int getCachedScriptableProperty(uintptr_t instance, std::string property) {
	for (auto& cacheData : script_able_cache) {
		uintptr_t instanceAddress = std::get<0>(cacheData);
		std::string instanceProperty = std::get<1>(cacheData);

		if (instanceAddress == instance && instanceProperty == property) {
			return std::get<2>(cacheData);
		}
	}

	return -1;
};

inline int getCachedDefultScriptableProperty(std::string property) {
	for (auto& cacheData : default_property_states) {
		if (cacheData.first == property) {
			return cacheData.second;
		}
	}

	return -1;
};

inline bool findAndUpdateScriptAbleCache(uintptr_t instance, std::string property, bool state) {
	for (auto& cacheData : script_able_cache) {
		uintptr_t instanceAddress = std::get<0>(cacheData);
		std::string instanceProperty = std::get<1>(cacheData);

		if (instanceAddress == instance && instanceProperty == property) {
			std::get<2>(cacheData) = state;
			return true;
		}
	}

	return false;
}

inline void addDefaultPropertyState(std::string property, bool state) {
	bool hasDefault = false;

	for (auto& cacheData : default_property_states) {
		if (cacheData.first == property) {
			hasDefault = true;
			break;
		}
	}

	if (!hasDefault) {
		default_property_states.push_back({ property, state });
	}
};

namespace InstancesHelper {
	inline bool luau_isscriptable(uintptr_t Property)
	{
		auto scriptable = *reinterpret_cast<uintptr_t*>(Property + 0x48);
		return scriptable & 0x20;
	};

	inline void luau_setscriptable(uintptr_t property, bool enabled)
	{
		*reinterpret_cast<uintptr_t*>(property + 0x48) = enabled ? 0xFF : 0x0;
	};

	inline std::unordered_map<std::string, uintptr_t> GetInstanceProperties(const uintptr_t rawInstance) {
		auto foundProperties = std::unordered_map<std::string, uintptr_t>();

		const auto classDescriptor = *reinterpret_cast<uintptr_t*>(
			rawInstance + Offsets::Properties::ClassDescriptor);
		const auto allPropertiesStart = *reinterpret_cast<uintptr_t*>(
			classDescriptor + Offsets::Instance::ClassDescriptor::PropertiesStart);
		const auto allPropertiesEnd = *reinterpret_cast<uintptr_t*>(
			classDescriptor + Offsets::Instance::ClassDescriptor::PropertiesEnd);

		for (uintptr_t currentPropertyAddress = allPropertiesStart; currentPropertyAddress != allPropertiesEnd;
			currentPropertyAddress += 0x8) {
			const auto currentProperty = *reinterpret_cast<uintptr_t*>(currentPropertyAddress);
			const auto propertyNameAddress = *reinterpret_cast<uintptr_t*>(
				currentProperty + 0x8);
			if (propertyNameAddress == 0)
				continue;
			const auto propertyName = *reinterpret_cast<std::string*>(propertyNameAddress);
			foundProperties[propertyName] = currentProperty;
		}

		return foundProperties;
	}
};

namespace Reflection
{
	inline int isscriptable(lua_State* LS) {
		luaL_checktype(LS, 1, LUA_TUSERDATA);
		luaL_checktype(LS, 2, LUA_TSTRING);

		uintptr_t Instance = *reinterpret_cast<uintptr_t*>(lua_touserdata(LS, 1));
		if (!Instance)
			luaL_argerror(LS, 1, "Invalid instance!");

		auto Property = lua_tostring(LS, 2);

		const auto EveryProperties = InstancesHelper::GetInstanceProperties(Instance);
		if (!EveryProperties.contains(Property))
			luaG_runerrorL(LS, "This property doesn't exist");

		const auto PropertyAddress = EveryProperties.at(Property);

		int cachedProperty = getCachedScriptableProperty(Instance, Property);
		int cachedDefaultProperty = getCachedDefultScriptableProperty(Property);

		if (cachedProperty != -1) {
			lua_pushboolean(LS, cachedProperty);
			return 1;
		}

		if (cachedDefaultProperty != -1) {
			lua_pushboolean(LS, cachedDefaultProperty);
			return 1;
		}

		lua_pushboolean(LS, InstancesHelper::luau_isscriptable(PropertyAddress));

		return 1;
	};
	inline int setscriptable(lua_State* LS)
	{
		luaL_checktype(LS, 1, LUA_TUSERDATA);
		luaL_checktype(LS, 2, LUA_TSTRING);
		luaL_checktype(LS, 3, LUA_TBOOLEAN);

		uintptr_t Instance = *reinterpret_cast<uintptr_t*>(lua_touserdata(LS, 1));
		if (!Instance)
			luaL_argerror(LS, 1, "Invalid instance!");

		auto Property = lua_tostring(LS, 2);

		bool Scriptable = lua_toboolean(LS, 3);

		const auto EveryProperties = InstancesHelper::GetInstanceProperties(Instance);
		if (!EveryProperties.contains(Property))
			luaG_runerrorL(LS, "This property doesn't exist");

		const auto PropertyAddress = EveryProperties.at(Property);

		if (!findAndUpdateScriptAbleCache(Instance, Property, Scriptable))
			script_able_cache.push_back({ Instance, Property, Scriptable });

		bool WasScriptable = InstancesHelper::luau_isscriptable(PropertyAddress);

		addDefaultPropertyState(Property, WasScriptable);

		InstancesHelper::luau_setscriptable(PropertyAddress, Scriptable);

		lua_pushboolean(LS, WasScriptable);

		return 1;
	};
	inline bool change_scriptable_state(uintptr_t instance, const std::string& property_name, bool state)
	{
		lua_State* L = luaL_newstate();
		if (!L) {
			return false;
		}

		uintptr_t* instance_ptr = static_cast<uintptr_t*>(lua_newuserdata(L, sizeof(uintptr_t)));
		*instance_ptr = instance;

		lua_pushstring(L, property_name.c_str());

		lua_pushboolean(L, state);

		int result = setscriptable(L);

		bool success = false;
		bool previous_state = false;

		if (result == 1) {
			previous_state = lua_toboolean(L, -1);
			success = true;
		}

		lua_close(L);

		return success;
	}
	inline int gethiddenproperty(lua_State* L) { // 'Unable to set property %s, type %s'
		luaL_checktype(L, 1, LUA_TUSERDATA);
		luaL_checktype(L, 2, LUA_TSTRING);

		const std::string ud_t = luaL_typename(L, 1);
		if (ud_t != "Instance")
			luaL_typeerrorL(L, 1, "Instance");

		int atom{};
		const uintptr_t instance = *static_cast<uintptr_t*>(lua_touserdata(L, 1));
		const std::string property_name = lua_tostringatom(L, 2, &atom);

		std::uintptr_t prop_desc = Roblox::KTable[atom];

		if (!prop_desc) {
			const uintptr_t classdesc = *reinterpret_cast<uintptr_t*>(instance + 0x18);
			const int prop_count = *reinterpret_cast<int*>(classdesc + 0x18);
			const uintptr_t* prop_array = *reinterpret_cast<uintptr_t**>(classdesc + Offsets::Instance::ClassDescriptor::PropertyDescriptor);

			for (int i = 0; i < prop_count; ++i) {
				const uintptr_t prop = prop_array[i];
				if (!prop) continue;

				const char* name = *reinterpret_cast<const char**>(prop + 0x10);
				if (!name) continue;

				if (strcmp(name, property_name.c_str()) == 0) {
					prop_desc = prop;
					break;
				}
			}

			if (!prop_desc)
				luaL_argerrorL(L, 2, "property does not exist (fallback failed)");
		}

		const uintptr_t class_desc = *reinterpret_cast<uintptr_t*>(instance + 0x18);

		const auto property_descriptor = Roblox::GetProperty(class_desc + Offsets::Instance::ClassDescriptor::PropertyDescriptor, &prop_desc);

		if (!property_descriptor)
			luaL_argerrorL(L, 2, "property does not exist");

		const uintptr_t get_set_impl = *reinterpret_cast<uintptr_t*>(*property_descriptor + 0xA0);

		if (!get_set_impl) {
			lua_pushnil(L);
			lua_pushboolean(L, true);
			return 2;
		}

		const uintptr_t get_set_vft = *reinterpret_cast<uintptr_t*>(get_set_impl);

		const uintptr_t getter = *reinterpret_cast<uintptr_t*>(get_set_vft + 0x18);

		const uintptr_t ttype = *reinterpret_cast<uintptr_t*>(*property_descriptor + 0x58);

		const int type_number = *reinterpret_cast<int*>(ttype + 0x40);

		const DWORD flags = *reinterpret_cast<DWORD*>(*property_descriptor + Offsets::Instance::Flags);

		const bool raw_scriptable = flags & 0x20;

		const bool force_hidden = (property_name == "size_xml" || property_name == "Position" || property_name == "Size");

		const bool is_scriptable = force_hidden ? false : raw_scriptable;

		const bool is_hidden = !is_scriptable;

		if (type_number == RBX::Reflection::ReflectionType_Bool) {
			const auto bool_getter = reinterpret_cast<bool(__fastcall*)(uintptr_t, uintptr_t)>(getter);
			const bool result = bool_getter(get_set_impl, instance);
			lua_pushboolean(L, result);
			lua_pushboolean(L, is_hidden);
			return 2;
		}

		else if (type_number == RBX::Reflection::ReflectionType_Int) {
			const auto int_getter = reinterpret_cast<int(__fastcall*)(uintptr_t, uintptr_t)>(getter);
			const int result = int_getter(get_set_impl, instance);
			lua_pushinteger(L, result);
			lua_pushboolean(L, is_hidden);
			return 2;
		}
		else if (type_number == RBX::Reflection::ReflectionType_Float) {
			const auto float_getter = reinterpret_cast<float(__fastcall*)(uintptr_t, uintptr_t)>(getter);
			const float result = float_getter(get_set_impl, instance);
			lua_pushnumber(L, result);
			lua_pushboolean(L, is_hidden);
			return 2;
		}
		else if (type_number == RBX::Reflection::ReflectionType_String) {
			const auto string_getter = reinterpret_cast<void(__fastcall*)(uintptr_t, std::string*, uintptr_t)>(getter);
			std::string result{};
			string_getter(get_set_impl, &result, instance);
			lua_pushlstring(L, result.data(), result.size());
			lua_pushboolean(L, is_hidden);
			return 2;
		}
		else if (type_number == RBX::Reflection::ReflectionType_BinaryString) {
			const auto string_getter = reinterpret_cast<void(__fastcall*)(uintptr_t, uintptr_t*, uintptr_t)>(getter);
			uintptr_t result_buffer[100]{};
			string_getter(get_set_impl, result_buffer, instance);
			const std::string result = *reinterpret_cast<std::string*>(result_buffer);
			lua_pushlstring(L, result.data(), result.size());
			lua_pushboolean(L, is_hidden);
			return 2;
		}
		else if (type_number == RBX::Reflection::ReflectionType_SharedString) {
			const auto shared_string_getter = reinterpret_cast<void(__fastcall*)(uintptr_t, RBX::shared_string_t**, uintptr_t)>(getter);
			RBX::shared_string_t* result{};
			shared_string_getter(get_set_impl, &result, instance);
			MessageBoxA(NULL, "debug pause", "debug", MB_OK);
			lua_pushlstring(L, result->c_str(), result->size());
			lua_pushboolean(L, is_hidden);
			return 2;
		}
		else if (type_number == RBX::Reflection::ReflectionType_SystemAddress) {
			const auto system_address_getter = reinterpret_cast<float(__fastcall*)(uintptr_t, RBX::system_address_t*, uintptr_t)>(getter);
			RBX::system_address_t result{};
			system_address_getter(get_set_impl, &result, instance);
			lua_pushinteger(L, result.remote_id.peer_id);
			lua_pushboolean(L, is_hidden);
			return 2;
		}
		else if (type_number == RBX::Reflection::ReflectionType_Vector3) {
			const auto vec3_getter = reinterpret_cast<void(__fastcall*)(uintptr_t, RBX::Vector3*, uintptr_t)>(getter);
			RBX::Vector3 result{};
			vec3_getter(get_set_impl, &result, instance);

			lua_newtable(L);
			lua_pushnumber(L, result.X); lua_setfield(L, -2, "X");
			lua_pushnumber(L, result.Y); lua_setfield(L, -2, "Y");
			lua_pushnumber(L, result.Z); lua_setfield(L, -2, "Z");
			lua_pushboolean(L, is_hidden);
			return 2;
		}
		else if (type_number == RBX::Reflection::ReflectionType_Enum) {
			const auto enum_getter = reinterpret_cast<int(__fastcall*)(uintptr_t, uintptr_t)>(getter);
			const int result = enum_getter(get_set_impl, instance);
			lua_pushinteger(L, result);
			lua_pushboolean(L, is_hidden);
			return 2;
		}
		else if (type_number == RBX::Reflection::ReflectionType_RefType) {
			const auto instance_getter = reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t, uintptr_t)>(getter);
			const uintptr_t result = instance_getter(get_set_impl, instance);

			if (result) {
				auto ud = static_cast<uintptr_t*>(lua_newuserdata(L, sizeof(uintptr_t)));
				*ud = result;
				if (luaL_getmetatable(L, "Instance") != LUA_TNIL) {
					lua_setmetatable(L, -2);
				}
				else {
					lua_pop(L, 1); // pop nil
					Roblox::Print(1, "[Warn] RefType result has no 'Instance' metatable registered");
				}
			}
			else {
				lua_pushnil(L);
			}

			lua_pushboolean(L, is_hidden);
			return 2;
		}


		else {
			Roblox::Print(1, "[Fallback] Attempting fallback get for %s", property_name.c_str());
			if (is_scriptable) {
				change_scriptable_state(instance, property_name, true);
				lua_getfield(L, 1, property_name.c_str());
				change_scriptable_state(instance, property_name, false);

				if (lua_isnil(L, -1)) {
					lua_pop(L, 1);
					lua_pushnil(L);
				}

				lua_pushboolean(L, true);
				return 2;
			}
			else {
				lua_pushnil(L);
				lua_pushboolean(L, true);
				return 2;
			}

			if (lua_isnil(L, -1)) {
				lua_pop(L, 1);
				lua_pushnil(L);
			}

			lua_pushboolean(L, true);
			return 2;
		}

		luaL_error(L, "gethiddenproperty: unhandled property type");
		return 0;
	}

	inline int sethiddenproperty(lua_State* L) {
		luaL_trimstack(L, 3);
		luaL_checktype(L, 1, LUA_TUSERDATA);
		luaL_checktype(L, 2, LUA_TSTRING);
		luaL_checkany(L, 3);

		const char* prop_name = lua_tostring(L, 2);

		lua_pushcclosure(L, isscriptable, 0, 0);
		lua_pushvalue(L, 1);
		lua_pushstring(L, prop_name);
		lua_call(L, 2, 1);
		bool was_scriptable = lua_toboolean(L, -1);
		lua_pop(L, 1);

		lua_pushcclosure(L, setscriptable, 0, 0);
		lua_pushvalue(L, 1);
		lua_pushstring(L, prop_name);
		lua_pushboolean(L, true);
		lua_pcall(L, 3, 1, 0);
		lua_pop(L, 1);

		lua_pushvalue(L, 3);
		lua_setfield(L, 1, prop_name);

		lua_pushcclosure(L, setscriptable, 0, 0);
		lua_pushvalue(L, 1);
		lua_pushstring(L, prop_name);
		lua_pushboolean(L, was_scriptable);
		lua_pcall(L, 3, 1, 0);
		lua_pop(L, 1);

		lua_pushboolean(L, !was_scriptable);
		return 1;
	}
}
