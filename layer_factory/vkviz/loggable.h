#ifndef LOGGABLE_H
#define LOGGABLE_h

#include <memory>
#include <type_traits>
#include <fstream>

// A class representing a loggable object
// Uses type erasure to achieve a nice interface
// TODO: Example usage here

typedef std::ofstream Logger;

/*
namespace Op {
class MapMemory;
class FlushMappedMemoryRanges;
class InvalidateMappedMemoryRanges;
class UnmapMemory;
}
void Log(Op::MapMemory map, Logger& log);
void Log(Op::FlushMappedMemoryRanges flush, Logger& log);
void Log(Op::InvalidateMappedMemoryRanges invalidate, Logger& log);
void Log(Op::UnmapMemory unmap, Logger& log);
*/

// Testing 
typedef std::ofstream Logger;

class Loggable {
  protected:
    struct concept {
        virtual ~concept() {}
        virtual void CallLog(Logger & logger) const = 0;
    };

    template <typename T>
    struct model : public concept {
        static_assert(!std::is_const<T>::value, "Cannot create a loggable object from a const one");
        model() = default;
        model(const T& other) : data_(other) {}
        model(T&& other) : data_(std::move(other)) {}

        void CallLog(Logger& logger) const override { Log(data_, logger); }

        T data_;
    };

   public:
    Loggable(const Loggable&) = delete;
    Loggable(Loggable&&) = default;

    Loggable& operator=(const Loggable&) = delete;
    Loggable& operator=(Loggable&&) = default;

    template <typename T>
    Loggable(T&& impl) : impl_(new model<std::decay_t<T>>(std::forward<T>(impl))) {}

    template <typename T>
    Loggable& operator=(T&& impl) {
        impl_.reset(new model<std::decay_t<T>>(std::forward<T>(impl)));
        return *this;
    }

    friend inline void Log(const Loggable& obj, Logger& logger) { obj.impl_->CallLog(logger); }

   protected:
    std::unique_ptr<concept> impl_;
};

#endif  // LOGGABLE_H
