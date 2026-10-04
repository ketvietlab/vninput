#pragma once

#include <vninput/settings.hpp>

#include <cstdint>
#include <memory>
#include <string>

namespace vninput {

enum class Key { Text, Backspace, Escape, Enter, Space, Other };
enum class Modifier : std::uint8_t { None = 0, Shift = 1, Control = 2, Alt = 4, Meta = 8 };

struct KeyEvent {
    Key key = Key::Other;
    std::u32string text{};
    std::uint8_t modifiers = 0;
};

struct Composition {
    // Apply commit/preedit first, then forward the original event if false.
    bool consumed = false;
    // Full replacement of the current preedit, not a delta. Empty clears it.
    std::u32string preedit;
    // Text to commit before displaying preedit; empty means no commit.
    std::u32string commit;
};

// One engine instance per native input context. The host owns focus, selection,
// key translation and applying preedit/commit with its OS input-method API.
// Calls on one instance must be serialized. No global mutable state or Qt.
class Engine {
public:
    virtual ~Engine() = default;
    // A changed configuration commits the old composition on the next process()
    // or flush(). Invalid enum values throw std::invalid_argument.
    virtual void configure(const Settings& settings) = 0;
    virtual Composition process(const KeyEvent& event) = 0;
    virtual Composition flush() = 0;
    virtual void reset() = 0;
};

std::unique_ptr<Engine> makeEngine(const Settings& settings = {});

} // namespace vninput
