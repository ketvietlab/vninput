#include <vninput/engine.hpp>

#include <array>
#include <cstdlib>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string_view>

using namespace vninput;
namespace {
int checks = 0;
void require(bool condition, std::string_view label)
{
    ++checks;
    if (!condition) { std::cerr << "FAIL: " << label << '\n'; std::exit(1); }
}
// Readable diagnostics without a locale/codecvt dependency.
std::string utf8(std::u32string_view text)
{
    std::string result;
    for (const auto c : text) {
        if (c < 0x80) result += static_cast<char>(c);
        else if (c < 0x800) { result += static_cast<char>(0xc0 | (c >> 6)); result += static_cast<char>(0x80 | (c & 63)); }
        else { result += static_cast<char>(0xe0 | (c >> 12)); result += static_cast<char>(0x80 | ((c >> 6) & 63)); result += static_cast<char>(0x80 | (c & 63)); }
    }
    return result;
}
void expectText(std::u32string_view actual, std::u32string_view expected, std::string_view label)
{
    if (actual != expected) std::cerr << utf8(actual) << " != " << utf8(expected) << '\n';
    require(actual == expected, label);
}
Composition type(Engine& engine, std::u32string_view text)
{
    Composition result;
    for (const auto c : text) result = engine.process({Key::Text, {c}});
    return result;
}
struct Case { std::u32string_view input; std::u32string_view output; };
void cases(InputMethod method, std::initializer_list<Case> words)
{
    Settings settings; settings.inputMethod = method;
    auto engine = makeEngine(settings);
    for (const auto& word : words) {
        engine->reset();
        expectText(type(*engine, word.input).preedit, word.output, utf8(word.input));
        expectText(engine->flush().commit, word.output, "flush");
        require(engine->flush().commit.empty(), "no duplicate commit");
    }
}
void words()
{
    cases(InputMethod::Telex, {
        {U"tieengs", U"tiếng"}, {U"Vieetj", U"Việt"}, {U"TIEENGS", U"TIẾNG"},
        {U"dduwowngf", U"đường"}, {U"dduowngf", U"đường"}, {U"duongwwfd", U"đường"},
        {U"Nguyeenx", U"Nguyễn"}, {U"hoaf", U"hoà"}, {U"hoef", U"hoè"}, {U"thuyr", U"thuỷ"},
        {U"hoanf", U"hoàn"}, {U"hoafn", U"hoàn"}, {U"qua", U"qua"}, {U"quas", U"quá"},
        {U"quys", U"quý"}, {U"quyeern", U"quyển"}, {U"gias", U"giá"}, {U"gif", U"gì"},
        {U"ginf", U"gìn"}, {U"gifs", U"gí"}, {U"giaf", U"già"}, {U"gifa", U"già"},
        {U"gieengs", U"giếng"}, {U"giuwx", U"giữ"}, {U"cuuws", U"cứu"},
        {U"nguowif", U"người"}, {U"huouw", U"hươu"}, {U"tuowir", U"tưởi"},
        {U"tuooir", U"tuổi"}, {U"thuwowngr", U"thưởng"}, {U"thuowr", U"thuở"},
        {U"quow", U"quơ"}, {U"huow", U"huơ"}, {U"thuown", U"thươn"}, {U"khuyur", U"khuỷu"},
        {U"ngoais", U"ngoái"}, {U"yeeus", U"yếu"}, {U"mias", U"mía"}, {U"muaf", U"mùa"},
        {U"cais", U"cái"}, {U"maus", U"máu"}, {U"mays", U"máy"}, {U"keos", U"kéo"},
        {U"aas", U"ấ"}, {U"asa", U"ấ"}, {U"awf", U"ằ"}, {U"eex", U"ễ"},
        {U"ooj", U"ộ"}, {U"owr", U"ở"}, {U"uws", U"ứ"}, {U"ys", U"ý"},
        {U"asf", U"à"}, {U"asz", U"a"}, {U"aaz", U"âz"}, {U"awsz", U"ă"},
        {U"ass", U"as"}, {U"aaa", U"aa"}, {U"ddd", U"dd"}, {U"oww", U"ow"},
        {U"w", U"ư"}, {U"ww", U"w"}, {U"WWindowws", U"Windows"},
        {U"guitarr", U"guitar"}, {U"xoongf", U"xồng"}, {U"xooongf", U"xoòng"},
        {U"DDAWNG", U"ĐĂNG"}, {U"Dduwowngf", U"Đường"}, {U"ddAwng", U"đĂng"},
    });
    cases(InputMethod::Vni, {
        {U"tie6ng1", U"tiếng"}, {U"Vie6t5", U"Việt"}, {U"d9u7o7ng2", U"đường"},
        {U"duong9772", U"đường"}, {U"D9U7O7NG2", U"ĐƯỜNG"}, {U"nguye6n4", U"nguyễn"},
        {U"qua1", U"quá"}, {U"gia2", U"già"}, {U"gi4", U"gĩ"},
        {U"hoa2", U"hoà"}, {U"thuy3", U"thuỷ"}, {U"a61", U"ấ"}, {U"a16", U"ấ"},
        {U"a82", U"ằ"}, {U"e64", U"ễ"}, {U"o65", U"ộ"}, {U"o73", U"ở"}, {U"u71", U"ứ"},
        {U"a12", U"à"}, {U"a10", U"a"}, {U"e11", U"e1"}, {U"a66", U"a6"},
        {U"d99", U"d9"}, {U"cuu71", U"cứu"}, {U"thuo73", U"thuở"},
        {U"tuoi63", U"tuổi"}, {U"nguo72i", U"người"}, {U"lua85", U"luặ"},
    });
}
void optionsAndBoundaries()
{
    Settings settings; settings.tonePlacement = TonePlacement::Classic;
    auto engine = makeEngine(settings);
    expectText(type(*engine, U"hoaf").preedit, U"hòa", "classic oa");
    engine->reset(); expectText(type(*engine, U"hoef").preedit, U"hòe", "classic oe");
    engine->reset(); expectText(type(*engine, U"thuyr").preedit, U"thủy", "classic uy");
    expectText(type(*engine, U"n").preedit, U"thuỷn", "closed syllable relocation");
    engine->reset(); settings.freeTone = false; engine->configure(settings);
    expectText(type(*engine, U"tiengs").preedit, U"tiengs", "strict tone after coda literal");
    engine->reset(); expectText(type(*engine, U"tieesng").preedit, U"tiếng", "strict tone before coda");
    engine->reset(); expectText(type(*engine, U"dduwowfng").preedit, U"đường", "strict shapes and tone");
    settings.enabled = false; engine->configure(settings);
    auto result = engine->process({Key::Text, U"s"});
    expectText(result.commit, U"đường", "disable preserves pending text");
    require(!result.consumed && result.preedit.empty(), "disabled pass through");
    settings.enabled = true; engine->configure(settings);
    expectText(type(*engine, U"aas").preedit, U"ấ", "enable fresh context");
    settings.inputMethod = InputMethod::Vni; engine->configure(settings);
    result = engine->process({Key::Text, U"a"});
    expectText(result.commit, U"ấ", "method switch commits old composition");
    expectText(result.preedit, U"a", "method switch starts new composition");
    expectText(type(*engine, U"61").preedit, U"ấ", "method switch active");
    for (const auto& boundary : {KeyEvent{Key::Space}, KeyEvent{Key::Enter}, KeyEvent{Key::Other},
             KeyEvent{Key::Text, U"."}, KeyEvent{Key::Text, U"😀"}, KeyEvent{Key::Text, U"Tiếng"},
             KeyEvent{Key::Text, U"é"}, KeyEvent{Key::Text, U"a", static_cast<std::uint8_t>(Modifier::Control)},
             KeyEvent{Key::Text, U"a", static_cast<std::uint8_t>(Modifier::Alt)},
             KeyEvent{Key::Text, U"a", static_cast<std::uint8_t>(Modifier::Meta)}}) {
        engine->reset(); type(*engine, U"a61"); result = engine->process(boundary);
        expectText(result.commit, U"ấ", "boundary commits once");
        require(!result.consumed && result.preedit.empty(), "boundary forwarded");
        require(engine->flush().commit.empty(), "boundary resets state");
    }
    engine->reset(); result = engine->process({Key::Text, U"A", static_cast<std::uint8_t>(Modifier::Shift)});
    expectText(result.preedit, U"A", "Shift preserves upper case");
    settings.toggleShortcut = ""; engine->configure(settings);
    require(type(*engine, U"61").commit.empty(), "shortcut not an engine setting");
    settings.inputMethod = static_cast<InputMethod>(999);
    bool threw = false;
    try { engine->configure(settings); } catch (const std::invalid_argument&) { threw = true; }
    require(threw, "reject invalid settings");
    expectText(engine->flush().commit, U"Ấ", "rejected configure does not damage composition");
}
void editing()
{
    auto engine = makeEngine();
    for (const auto& step : {Case{U"t", U"t"}, Case{U"i", U"ti"}, Case{U"ees", U"tiế"},
             Case{U"n", U"tiến"}, Case{U"g", U"tiếng"}})
        expectText(type(*engine, step.input).preedit, step.output, "tone entered before final consonants");
    engine->reset();
    type(*engine, U"tieengs");
    for (const auto expected : {U"tiến", U"tiế", U"ti", U"t", U""})
        expectText(engine->process({Key::Backspace}).preedit, expected, "Backspace deletes visible character");
    require(!engine->process({Key::Backspace}).consumed, "empty Backspace forwarded");
    type(*engine, U"tieengs");
    expectText(engine->process({Key::Escape}).preedit, U"tieengs", "Escape restores physical keys");
    expectText(type(*engine, U"s").preedit, U"tieengss", "restored word stays literal");
    auto result = engine->process({Key::Escape});
    require(!result.consumed, "second Escape forwarded");
    expectText(result.commit, U"tieengss", "second Escape commits raw text");
    expectText(type(*engine, U"aas").preedit, U"ấ", "restore state ends at boundary");
    engine->reset(); type(*engine, U"tieengs"); engine->process({Key::Backspace});
    expectText(engine->process({Key::Escape}).preedit, U"tieens", "raw keys follow deleted glyph");
    engine->reset(); type(*engine, U"hoafn");
    expectText(engine->process({Key::Backspace}).preedit, U"hoà", "tone relocates after coda deletion");
    expectText(engine->process({Key::Backspace}).preedit, U"ho", "deleting tone vowel removes tone");
    expectText(engine->process({Key::Escape}).preedit, U"ho", "deleted tone not resurrected by Escape");
    engine->reset(); type(*engine, U"thuown");
    expectText(engine->process({Key::Backspace}).preedit, U"thuơ", "automatic horn correction reversible");
    engine->reset(); type(*engine, U"as"); engine->reset();
    require(engine->flush().commit.empty(), "reset cancels pending text");
    Settings settings; settings.restoreOnEscape = false; engine->configure(settings);
    type(*engine, U"as"); result = engine->process({Key::Escape});
    require(!result.consumed, "Escape restore can be disabled");
    expectText(result.commit, U"á", "disabled Escape preserves composed text");
    auto other = makeEngine(); type(*other, U"dd"); type(*engine, U"ee");
    expectText(other->flush().commit, U"đ", "instances isolated");
    expectText(engine->flush().commit, U"ê", "instances isolated second");
}
void boundedState()
{
    auto engine = makeEngine();
    std::u32string output;
    for (int i = 0; i < 10000; ++i) output += engine->process({Key::Text, U"b"}).commit;
    output += engine->flush().commit;
    expectText(output, std::u32string(10000, U'b'), "long word loses no input");
    std::mt19937 random(0x564e4b);
    constexpr std::u32string_view alphabet = U"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (int i = 0; i < 50000; ++i) {
        Composition result;
        switch (random() % 12) {
        case 0: result = engine->process({Key::Backspace}); break;
        case 1: result = engine->process({Key::Escape}); break;
        case 2: result = engine->flush(); break;
        case 3: engine->reset(); break;
        default: result = engine->process({Key::Text, {alphabet[random() % alphabet.size()]}}); break;
        }
        require(result.preedit.size() <= 256, "bounded preedit");
        for (const auto c : result.preedit)
            require(c <= 0x10ffff && !(c >= 0xd800 && c <= 0xdfff), "valid Unicode scalar");
    }
    // Thousands of short contexts catch a stale escaped/disabled state.
    for (int i = 0; i < 2000; ++i) {
        engine->reset(); type(*engine, U"ass"); engine->flush();
        expectText(type(*engine, U"tieengs").preedit, U"tiếng", "engine recovers at every new word");
    }
}
void fullStreamAndUnicodeTable()
{
    auto engine = makeEngine();
    std::u32string document;
    for (const auto c : U"Tooi yeeu tieengs Vieetj. DDuowngf veef quee thaatj ddepj!") {
        if (!c) break;
        const auto result = engine->process({Key::Text, {c}});
        document += result.commit;
        if (!result.consumed) document += c;
    }
    document += engine->flush().commit;
    expectText(document, U"Tôi yêu tiếng Việt. Đường về quê thật đẹp!", "adapter stream commit-before-forward");
    struct Vowel { std::u32string_view keys, lower, upper; };
    constexpr std::array table{
        Vowel{U"a", U"aáàảãạ", U"AÁÀẢÃẠ"}, Vowel{U"aw", U"ăắằẳẵặ", U"ĂẮẰẲẴẶ"},
        Vowel{U"aa", U"âấầẩẫậ", U"ÂẤẦẨẪẬ"}, Vowel{U"e", U"eéèẻẽẹ", U"EÉÈẺẼẸ"},
        Vowel{U"ee", U"êếềểễệ", U"ÊẾỀỂỄỆ"}, Vowel{U"i", U"iíìỉĩị", U"IÍÌỈĨỊ"},
        Vowel{U"o", U"oóòỏõọ", U"OÓÒỎÕỌ"}, Vowel{U"oo", U"ôốồổỗộ", U"ÔỐỒỔỖỘ"},
        Vowel{U"ow", U"ơớờởỡợ", U"ƠỚỜỞỠỢ"}, Vowel{U"u", U"uúùủũụ", U"UÚÙỦŨỤ"},
        Vowel{U"uw", U"ưứừửữự", U"ƯỨỪỬỮỰ"}, Vowel{U"y", U"yýỳỷỹỵ", U"YÝỲỶỸỴ"}
    };
    for (const auto& vowel : table) for (std::size_t tone = 0; tone < 6; ++tone) for (bool upper : {false, true}) {
        auto keys = std::u32string(vowel.keys);
        if (tone) keys += U"zsfrxj"[tone];
        if (upper) for (auto& key : keys) key -= U'a' - U'A';
        engine->reset();
        expectText(type(*engine, keys).preedit, std::u32string(1, (upper ? vowel.upper : vowel.lower)[tone]), "NFC vowel and case table");
    }
    engine->reset(); type(*engine, U"a");
    for (int i = 0; i < 3000; ++i) {
        const auto result = engine->process({Key::Text, i % 2 ? U"f" : U"s"});
        require(result.preedit.size() <= 64, "repeated tone history is bounded");
    }
    engine->flush();
    expectText(type(*engine, U"tieengs").preedit, U"tiếng", "fresh word after history capacity");
}
} // namespace
int main()
{
    words(); optionsAndBoundaries(); editing(); boundedState(); fullStreamAndUnicodeTable();
    std::cout << "VNInput engine: " << checks << " checks passed\n";
}
