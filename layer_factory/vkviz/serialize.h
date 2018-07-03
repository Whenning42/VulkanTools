#ifndef SERIALIZE_H
#define SERIALIZE_H

#include "third_party/json.hpp"

#include <type_traits>
#include <memory>

using json = nlohmann::json;

// A class representing a serializable object
// Uses type erasure to achieve a nice interface
// TODO: Example usage here

class Serializable {
  protected:
    struct concept {
        virtual ~concept() {}
        virtual json Serialize() const = 0;
    };

    template <typename T>
    struct model : public concept {
        static_assert(!std::is_const<T>::value, "Cannot create a serilizable object from a const one");
        model() = default;
        model(const T& other) : data_(other) {}
        model(T&& other) : data_(std::move(other)) {}

        json Serialize() const override { return data_.Serialize(); }

        T data_;
    };

   public:
    Serializable(const Serializable&) = delete;
    Serializable(Serializable&&) = default;

    Serializable& operator=(const Serializable&) = delete;
    Serializable& operator=(Serializable&&) = default;

    template <typename T>
    Serializable(T&& impl) : impl_(new model<std::decay_t<T>>(std::forward<T>(impl))) {}

    template <typename T>
    Serializable& operator=(T&& impl) {
        impl_.reset(new model<std::decay_t<T>>(std::forward<T>(impl)));
        return *this;
    }

    json Serialize(const Serializable& obj) const { obj.impl_->Serialize(); }

   protected:
    std::unique_ptr<concept> impl_;
};



#endif  // SERIALIZE_H
