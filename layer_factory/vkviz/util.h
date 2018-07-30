#ifndef UTIL_H
#define UTIL_H

#include <unordered_map>

template<typename K, typename H, typename V>
V FindOrDefault(const std::unordered_map<K, V, H> map, const K& key, const V& default_value) {
    if(map.find(key) == map.end()) {
        return default_value;
    } else {
        return map.at(key);
    }
}

#endif  // UTIL_H
