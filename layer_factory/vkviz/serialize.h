/* Copyright (C) 2018 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: William Henning <whenning@google.com>
 */

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

#define SERIALIZE(class, var)                                                 \
    inline void to_json(json& j, const class& obj) { j = {{#var, obj.var}}; } \
    inline void from_json(const json& j, class& obj) { obj.var = j[#var].get<decltype(obj.var)>(); }

#define SERIALIZE2(class, var1, var2)                                                              \
    inline void to_json(json& j, const class& obj) { j = {{#var1, obj.var1}, {#var2, obj.var2}}; } \
    inline void from_json(const json& j, class& obj) {                                             \
        obj.var1 = j[#var1].get<decltype(obj.var1)>();                                             \
        obj.var2 = j[#var2].get<decltype(obj.var2)>();                                             \
    }

#define SERIALIZE3(class, var1, var2, var3)                                                                           \
    inline void to_json(json& j, const class& obj) { j = {{#var1, obj.var1}, {#var2, obj.var2}, {#var3, obj.var3}}; } \
    inline void from_json(const json& j, class& obj) {                                                                \
        obj.var1 = j[#var1].get<decltype(obj.var1)>();                                                                \
        obj.var2 = j[#var2].get<decltype(obj.var2)>();                                                                \
        obj.var3 = j[#var3].get<decltype(obj.var3)>();                                                                \
    }

#define SERIALIZE4(class, var1, var2, var3, var4)                                         \
    inline void to_json(json& j, const class& obj) {                                      \
        j = {{#var1, obj.var1}, {#var2, obj.var2}, {#var3, obj.var3}, {#var4, obj.var4}}; \
    }                                                                                     \
    inline void from_json(const json& j, class& obj) {                                    \
        obj.var1 = j[#var1].get<decltype(obj.var1)>();                                    \
        obj.var2 = j[#var2].get<decltype(obj.var2)>();                                    \
        obj.var3 = j[#var3].get<decltype(obj.var3)>();                                    \
        obj.var4 = j[#var4].get<decltype(obj.var4)>();                                    \
    }

// Helper macros to create serialization functions for children classes inheriting from a base class.
#define C_SERIALIZE(base_class, derived_class, var)             \
    void to_json(json& j) const override { j = {{#var, var}}; } \
    void from_json(const json& j) override { this->var = j[#var].get<decltype(this->var)>(); }

#define C_SERIALIZE2(base_class, derived_class, var1, var2)                      \
    void to_json(json& j) const override { j = {{#var1, var1}, {#var2, var2}}; } \
    void from_json(const json& j) override {                                     \
        this->var1 = j[#var1].get<decltype(this->var1)>();                       \
        this->var2 = j[#var2].get<decltype(this->var2)>();                       \
    }

#define C_SERIALIZE3(base_class, derived_class, var1, var2, var3)                               \
    void to_json(json& j) const override { j = {{#var1, var1}, {#var2, var2}, {#var3, var3}}; } \
    void from_json(const json& j) override {                                                    \
        this->var1 = j[#var1].get<decltype(this->var1)>();                                      \
        this->var2 = j[#var2].get<decltype(this->var2)>();                                      \
        this->var3 = j[#var3].get<decltype(this->var3)>();                                      \
    }

// Serialization for pointers is to serialize Vulkan handles.
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
