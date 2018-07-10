#ifndef SERIALIZE_H
#define SERIALIZE_H

#include "third_party/json.hpp"
using json = nlohmann::json;

// Helper macros to create serialization functions for structs
#define SERIALIZE0(class) \
        inline void to_json(json& j, const class& obj) { \
            j = {{}}; \
        } \
        inline void from_json(const json& j, class& obj) {}

#define SERIALIZE(class, var_type, var) \
        inline void to_json(json& j, const class& obj) { \
            j = {{"var", obj.var}}; \
        } \
        inline void from_json(const json& j, class& obj) { \
            obj.var = j["var"].get<var_type>(); \
        }

#define SERIALIZE2(class, var_type1, var1, var_type2, var2) \
        inline void to_json(json& j, const class& obj) { \
            j = json{{"var1", obj.var1}, {"var2", obj.var2}}; \
        } \
        inline void from_json(const json& j, class& obj) { \
            obj.var1 = j["var1"].get<var_type1>(); \
            obj.var2 = j["var2"].get<var_type2>(); \
        }

#define SERIALIZE3(class, var_type1, var1, var_type2, var2, var_type3, var3) \
        inline void to_json(json& j, const class& obj) { \
            j = json{{"var1", obj.var1}, {"var2", obj.var2}, {"var3", obj.var3}}; \
        } \
        inline void from_json(const json& j, class& obj) { \
            obj.var1 = j["var1"].get<var_type1>(); \
            obj.var2 = j["var2"].get<var_type2>(); \
            obj.var3 = j["var3"].get<var_type3>(); \
        }

#define SERIALIZE4(class, var_type1, var1, var_type2, var2, var_type3, var3, var_type4, var4) \
        inline void to_json(json& j, const class& obj) { \
            j = json{{"var1", obj.var1}, {"var2", obj.var2}, {"var3", obj.var3}, {"var4", obj.var4}}; \
        } \
        inline void from_json(const json& j, class& obj) { \
            obj.var1 = j["var1"].get<var_type1>(); \
            obj.var2 = j["var2"].get<var_type2>(); \
            obj.var3 = j["var3"].get<var_type3>(); \
            obj.var4 = j["var4"].get<var_type4>(); \
        }

// Serialization for pointers is needed for Vulkan handles.
template<typename T>
void to_json(json& j, T* const & handle) {
    j = {{"handle", reinterpret_cast<std::uintptr_t>(handle)}};
}

// Deserialization for pointers is needed for Vulkan handles.
template<typename T>
void from_json(const json& j, T*& handle) {
    handle = reinterpret_cast<T*>(j["handle"].get<std::uintptr_t>());
}

#endif  // SERIALIZE_H
