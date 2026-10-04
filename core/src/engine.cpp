#include <vninput/engine.hpp>

#include <algorithm>
#include <array>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace vninput {
namespace {

enum class Shape { Plain, Circumflex, Breve, Horn, Stroke };
struct Letter {
    char32_t base;
    bool upper = false;
    Shape shape = Shape::Plain;
    unsigned tone = 0;
    bool standaloneW = false;
};
struct RawKey {
    char32_t text;
    std::vector<std::size_t> targets;
    bool tone = false;
};
struct VowelRow {
    char32_t base;
    Shape shape;
    std::u32string_view lower;
    std::u32string_view upper;
};
// NFC output; columns: level, acute, grave, hook, tilde, dot below.
constexpr std::array vowels{
    VowelRow{U'a', Shape::Plain, U"aáàảãạ", U"AÁÀẢÃẠ"},
    VowelRow{U'a', Shape::Breve, U"ăắằẳẵặ", U"ĂẮẰẲẴẶ"},
    VowelRow{U'a', Shape::Circumflex, U"âấầẩẫậ", U"ÂẤẦẨẪẬ"},
    VowelRow{U'e', Shape::Plain, U"eéèẻẽẹ", U"EÉÈẺẼẸ"},
    VowelRow{U'e', Shape::Circumflex, U"êếềểễệ", U"ÊẾỀỂỄỆ"},
    VowelRow{U'i', Shape::Plain, U"iíìỉĩị", U"IÍÌỈĨỊ"},
    VowelRow{U'o', Shape::Plain, U"oóòỏõọ", U"OÓÒỎÕỌ"},
    VowelRow{U'o', Shape::Circumflex, U"ôốồổỗộ", U"ÔỐỒỔỖỘ"},
    VowelRow{U'o', Shape::Horn, U"ơớờởỡợ", U"ƠỚỜỞỠỢ"},
    VowelRow{U'u', Shape::Plain, U"uúùủũụ", U"UÚÙỦŨỤ"},
    VowelRow{U'u', Shape::Horn, U"ưứừửữự", U"ƯỨỪỬỮỰ"},
    VowelRow{U'y', Shape::Plain, U"yýỳỷỹỵ", U"YÝỲỶỸỴ"},
};

bool isUpper(char32_t c) { return c >= U'A' && c <= U'Z'; }
char32_t lower(char32_t c) { return isUpper(c) ? c + (U'a' - U'A') : c; }
bool isLetter(char32_t c) { return lower(c) >= U'a' && lower(c) <= U'z'; }
bool isVowel(char32_t c) { return std::u32string_view(U"aeiouy").find(c) != std::u32string_view::npos; }
char32_t encode(const Letter& c)
{
    for (const auto& row : vowels)
        if (c.base == row.base && c.shape == row.shape)
            return (c.upper ? row.upper : row.lower)[c.tone];
    if (c.base == U'd' && c.shape == Shape::Stroke) return c.upper ? U'Đ' : U'đ';
    return c.upper ? c.base - (U'a' - U'A') : c.base;
}
bool sameOptions(const Settings& a, const Settings& b)
{
    return a.inputMethod == b.inputMethod && a.tonePlacement == b.tonePlacement
        && a.enabled == b.enabled && a.freeTone == b.freeTone
        && a.restoreOnEscape == b.restoreOnEscape;
}

class VietnameseEngine final : public Engine {
public:
    explicit VietnameseEngine(const Settings& settings) { configure(settings); active_ = requested_; }
    void configure(const Settings& settings) override
    {
        if ((settings.inputMethod != InputMethod::Telex && settings.inputMethod != InputMethod::Vni)
            || (settings.tonePlacement != TonePlacement::Modern && settings.tonePlacement != TonePlacement::Classic))
            throw std::invalid_argument("Invalid VNInput engine settings");
        requested_ = settings;
    }
    Composition process(const KeyEvent& event) override
    {
        Composition result;
        if (!sameOptions(active_, requested_)) {
            result.commit = render();
            clear();
            active_ = requested_;
        }
        constexpr auto shortcuts = static_cast<std::uint8_t>(Modifier::Control)
            | static_cast<std::uint8_t>(Modifier::Alt) | static_cast<std::uint8_t>(Modifier::Meta);
        const auto finish = [&] {
            result.commit += render();
            clear();
            return result;
        };
        if (!active_.enabled || (event.modifiers & shortcuts)) return finish();
        if (event.key == Key::Backspace) {
            if (letters_.empty()) return result;
            eraseLast();
        } else if (event.key == Key::Escape) {
            if (!active_.restoreOnEscape || letters_.empty() || literal_) return finish();
            const auto original = rawText();
            clear();
            for (const auto c : original) append(c);
            literal_ = true;
        } else if (event.key == Key::Text && event.text.size() == 1) {
            const auto c = event.text.front();
            const bool digit = active_.inputMethod == InputMethod::Vni && c >= U'0' && c <= U'9';
            if (!isLetter(c) && !digit) return finish();
            // Bound both visible text and history, including repeated mark changes.
            if (letters_.size() >= 64 || raw_.size() >= 256) {
                result.commit += render();
                clear();
            }
            if (literal_) {
                append(c);
            } else if (!transform(c)) {
                if (digit) return finish();
                append(c);
            }
            relocateTone();
        } else {
            return finish();
        }
        result.consumed = true;
        result.preedit = render();
        return result;
    }
    Composition flush() override
    {
        Composition result{false, {}, render()};
        clear();
        active_ = requested_;
        return result;
    }
    void reset() override { clear(); active_ = requested_; }

private:
    Settings active_;
    Settings requested_;
    std::vector<Letter> letters_;
    std::vector<RawKey> raw_;
    bool literal_ = false;

    void clear() { letters_.clear(); raw_.clear(); literal_ = false; }
    void append(char32_t c)
    {
        raw_.push_back({c, {letters_.size()}, false});
        letters_.push_back({lower(c), isUpper(c)});
    }
    std::u32string rawText() const
    {
        std::u32string value;
        for (const auto& key : raw_) value += key.text;
        return value;
    }
    // The last contiguous vowel group. u in qu and i in gi are glides when
    // another vowel follows; the i in gi/gìn remains a vowel.
    std::vector<std::size_t> nucleus() const
    {
        std::vector<std::size_t> group;
        std::size_t end = letters_.size();
        while (end && !isVowel(letters_[end - 1].base)) --end;
        auto start = end;
        while (start && isVowel(letters_[start - 1].base)) --start;
        if (end > start && start && letters_[start].base == U'u' && letters_[start - 1].base == U'q') ++start;
        else if (end > start + 1 && start && letters_[start].base == U'i' && letters_[start - 1].base == U'g') ++start;
        for (auto i = start; i < end; ++i) group.push_back(i);
        return group;
    }
    bool canModify(const std::vector<std::size_t>& group) const
    {
        return !group.empty() && (active_.freeTone || group.back() + 1 == letters_.size());
    }
    std::vector<Letter> effectiveLetters() const
    {
        auto letters = letters_;
        // Keep explicit marks separately, so deleting a coda reverses automatic
        // uơ/ưo -> ươ correction and a second w/7 can still mark the other vowel.
        for (std::size_t i = 1; i < letters.size(); ++i) {
            auto& u = letters[i - 1];
            auto& o = letters[i];
            if (u.base != U'u' || o.base != U'o' || (i > 1 && letters[i - 2].base == U'q')) continue;
            if (u.shape != Shape::Horn && o.shape != Shape::Horn) continue;
            if (u.shape == Shape::Circumflex || o.shape == Shape::Circumflex) continue;
            const bool openH = (i == 2 && letters[0].base == U'h')
                || (i == 3 && letters[0].base == U't' && letters[1].base == U'h');
            if (i + 1 < letters.size() || (!openH && active_.inputMethod == InputMethod::Telex))
                u.shape = o.shape = Shape::Horn;
        }
        return letters;
    }
    std::size_t toneTarget(const std::vector<std::size_t>& group) const
    {
        const auto effective = effectiveLetters();
        // A marked vowel is the nucleus; in ươ the rightmost marked vowel wins.
        for (auto it = group.rbegin(); it != group.rend(); ++it)
            if (effective[*it].shape != Shape::Plain) return *it;
        if (group.size() >= 3) return group[group.size() - 2];
        if (group.size() == 2) {
            if (group.back() + 1 < letters_.size()) return group.back();
            const auto a = letters_[group[0]].base;
            const auto b = letters_[group[1]].base;
            if (a == U'o' && b == U'o') return group.back();
            if (active_.tonePlacement == TonePlacement::Modern
                && ((a == U'o' && (b == U'a' || b == U'e')) || (a == U'u' && b == U'y')))
                return group.back();
        }
        return group.front();
    }
    void relocateTone()
    {
        const auto group = nucleus();
        if (group.empty()) return;
        unsigned tone = 0;
        for (const auto i : group) { if (letters_[i].tone) tone = letters_[i].tone; letters_[i].tone = 0; }
        // A formerly independent gi vowel may have just become a glide.
        const auto start = group.front();
        if (start >= 2 && letters_[start - 1].base == U'i' && letters_[start - 2].base == U'g') {
            if (!tone) tone = letters_[start - 1].tone;
            letters_[start - 1].tone = 0;
        }
        const auto target = toneTarget(group);
        letters_[target].tone = tone;
        for (auto& key : raw_)
            if (key.tone && !key.targets.empty() && key.targets.front() >= (start ? start - 1 : 0)
                && key.targets.front() <= group.back()) key.targets = {target};
    }
    bool setTone(char32_t c, unsigned tone)
    {
        const auto group = nucleus();
        if (!canModify(group)) return false;
        const auto target = toneTarget(group);
        const auto previous = letters_[target].tone;
        if (!tone && !previous) return false;
        for (const auto i : group) letters_[i].tone = 0;
        raw_.push_back({c, {target}, true});
        if (tone && previous == tone) {
            // A repeated modifier escapes it: ass -> as, e11 -> e1.
            raw_.pop_back();
            append(c);
            literal_ = true;
        } else letters_[target].tone = tone;
        return true;
    }
    bool setShape(char32_t c, const std::vector<std::size_t>& targets, Shape shape)
    {
        if (targets.empty()) return false;
        for (auto it = targets.rbegin(); it != targets.rend(); ++it) {
            if (letters_[*it].shape != shape) {
                letters_[*it].shape = shape;
                raw_.push_back({c, {*it}, false});
                return true;
            }
        }
        if (targets.size() == 1 && letters_[targets[0]].standaloneW) {
            auto& letter = letters_[targets[0]];
            letter.base = U'w'; letter.shape = Shape::Plain; letter.tone = 0; letter.standaloneW = false;
            raw_.push_back({c, targets, false});
        } else {
            for (const auto i : targets) letters_[i].shape = Shape::Plain;
            append(c);
            // ooo escapes ô into oo, which still accepts a tone (xoòng).
            literal_ = !(active_.inputMethod == InputMethod::Telex && lower(c) == U'o');
        }
        return true;
    }
    bool transform(char32_t c)
    {
        const auto key = lower(c);
        const bool telex = active_.inputMethod == InputMethod::Telex;
        const auto tones = telex ? std::u32string_view(U"zsfrxj") : std::u32string_view(U"012345");
        if (const auto pos = tones.find(key); pos != std::u32string_view::npos)
            return setTone(c, static_cast<unsigned>(pos));
        if ((telex && key == U'd') || (!telex && key == U'9')) {
            for (auto i = letters_.size(); i > 0; --i) {
                if (letters_[i - 1].base == U'd') return setShape(c, {i - 1}, Shape::Stroke);
                if (!active_.freeTone) break;
            }
            return false;
        }
        const auto group = nucleus();
        if (!canModify(group)) {
            if (telex && key == U'w' && group.empty()) {
                append(c);
                letters_.back().base = U'u'; letters_.back().shape = Shape::Horn; letters_.back().standaloneW = true;
                return true;
            }
            return false;
        }
        const bool circumflex = telex ? (key == U'a' || key == U'e' || key == U'o') : key == U'6';
        if (circumflex) {
            for (auto it = group.rbegin(); it != group.rend(); ++it) {
                const auto base = letters_[*it].base;
                if ((telex && base == key) || (!telex && (base == U'a' || base == U'e' || base == U'o')))
                    return setShape(c, {*it}, Shape::Circumflex);
            }
        }
        if ((telex && key == U'w') || (!telex && (key == U'7' || key == U'8'))) {
            std::vector<std::size_t> targets;
            for (const auto i : group) {
                const auto base = letters_[i].base;
                if ((base == U'a' && (telex || key == U'8')) || ((base == U'o' || base == U'u') && (telex || key == U'7')))
                    targets.push_back(i);
            }
            // ua/ui/uu and oi use the first vowel; uo supports both horns.
            if ((telex || key == U'7') && group.size() >= 2 && letters_[group.front()].base == U'u')
                targets = letters_[group[1]].base == U'o'
                    ? std::vector<std::size_t>{group[0], group[1]} : std::vector<std::size_t>{group.front()};
            if (!targets.empty()) {
                const auto shape = letters_[targets.back()].base == U'a' ? Shape::Breve : Shape::Horn;
                if (shape == Shape::Breve) targets = {targets.back()};
                return setShape(c, targets, shape);
            }
        }
        return false;
    }
    std::u32string render() const
    {
        std::u32string text;
        for (const auto& letter : effectiveLetters()) text += encode(letter);
        return text;
    }
    void eraseLast()
    {
        const auto removed = letters_.size() - 1;
        letters_.pop_back();
        for (auto& key : raw_) std::erase(key.targets, removed);
        std::erase_if(raw_, [](const RawKey& key) { return key.targets.empty(); });
        if (letters_.empty()) clear();
        else relocateTone();
    }
};
} // namespace

std::unique_ptr<Engine> makeEngine(const Settings& settings)
{
    return std::make_unique<VietnameseEngine>(settings);
}
} // namespace vninput
