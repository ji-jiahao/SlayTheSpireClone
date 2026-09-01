#include "event/EventDatabase.hpp"

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <map>
#include <iterator>
#include <stdexcept>
#include <utility>

namespace
{
struct JsonValue
{
    enum class Type
    {
        Null,
        Number,
        String,
        Array,
        Object
    };

    Type type = Type::Null;
    long long numberValue = 0;
    std::string stringValue;
    std::vector<JsonValue> arrayValue;
    std::map<std::string, JsonValue> objectValue;

    bool isNull() const
    {
        return type == Type::Null;
    }

    bool isNumber() const
    {
        return type == Type::Number;
    }

    bool isString() const
    {
        return type == Type::String;
    }

    bool isArray() const
    {
        return type == Type::Array;
    }

    bool isObject() const
    {
        return type == Type::Object;
    }

    const JsonValue& at(const std::string& key) const
    {
        const auto it = objectValue.find(key);
        if (it == objectValue.end())
        {
            throw std::runtime_error("JSON 缺少字段: " + key);
        }

        return it->second;
    }

    bool contains(const std::string& key) const
    {
        return objectValue.find(key) != objectValue.end();
    }
};

class JsonParser
{
public:
    explicit JsonParser(std::string text)
        : text_(std::move(text))
    {
    }

    JsonValue parse()
    {
        skipWhitespace();
        JsonValue value = parseValue();
        skipWhitespace();

        if (!isEnd())
        {
            throw std::runtime_error("JSON 末尾存在多余内容");
        }

        return value;
    }

private:
    JsonValue parseValue()
    {
        skipWhitespace();
        if (isEnd())
        {
            throw std::runtime_error("JSON 意外结束");
        }

        const char ch = peek();
        if (ch == '"')
        {
            return parseString();
        }

        if (ch == '{')
        {
            return parseObject();
        }

        if (ch == '[')
        {
            return parseArray();
        }

        if (ch == '-' || std::isdigit(static_cast<unsigned char>(ch)) != 0)
        {
            return parseNumber();
        }

        if (matchLiteral("null"))
        {
            return JsonValue{};
        }

        throw std::runtime_error("JSON 中出现无法识别的值");
    }

    JsonValue parseString()
    {
        expect('"');
        std::string result;

        while (!isEnd())
        {
            char ch = get();
            if (ch == '"')
            {
                JsonValue value;
                value.type = JsonValue::Type::String;
                value.stringValue = std::move(result);
                return value;
            }

            if (ch == '\\')
            {
                if (isEnd())
                {
                    throw std::runtime_error("JSON 字符串转义不完整");
                }

                const char escaped = get();
                switch (escaped)
                {
                case '"':
                case '\\':
                case '/':
                    result.push_back(escaped);
                    break;
                case 'b':
                    result.push_back('\b');
                    break;
                case 'f':
                    result.push_back('\f');
                    break;
                case 'n':
                    result.push_back('\n');
                    break;
                case 'r':
                    result.push_back('\r');
                    break;
                case 't':
                    result.push_back('\t');
                    break;
                case 'u':
                    appendUtf8CodePoint(result, parseUnicodeEscape());
                    break;
                default:
                    throw std::runtime_error("JSON 字符串包含非法转义");
                }
            }
            else
            {
                result.push_back(ch);
            }
        }

        throw std::runtime_error("JSON 字符串未正确结束");
    }

    JsonValue parseNumber()
    {
        std::size_t start = position_;
        if (peek() == '-')
        {
            advance();
        }

        while (!isEnd() && std::isdigit(static_cast<unsigned char>(peek())) != 0)
        {
            advance();
        }

        const std::string token = text_.substr(start, position_ - start);
        if (token.empty() || token == "-")
        {
            throw std::runtime_error("JSON 数字格式非法");
        }

        JsonValue value;
        value.type = JsonValue::Type::Number;
        value.numberValue = std::stoll(token);
        return value;
    }

    JsonValue parseArray()
    {
        expect('[');
        JsonValue value;
        value.type = JsonValue::Type::Array;

        skipWhitespace();
        if (consume(']'))
        {
            return value;
        }

        while (true)
        {
            value.arrayValue.push_back(parseValue());
            skipWhitespace();
            if (consume(']'))
            {
                break;
            }

            expect(',');
        }

        return value;
    }

    JsonValue parseObject()
    {
        expect('{');
        JsonValue value;
        value.type = JsonValue::Type::Object;

        skipWhitespace();
        if (consume('}'))
        {
            return value;
        }

        while (true)
        {
            skipWhitespace();
            JsonValue key = parseString();
            skipWhitespace();
            expect(':');
            value.objectValue.emplace(std::move(key.stringValue), parseValue());
            skipWhitespace();
            if (consume('}'))
            {
                break;
            }

            expect(',');
        }

        return value;
    }

    bool matchLiteral(const char* literal)
    {
        std::size_t savedPosition = position_;
        while (*literal != '\0' && !isEnd() && peek() == *literal)
        {
            advance();
            ++literal;
        }

        if (*literal == '\0')
        {
            return true;
        }

        position_ = savedPosition;
        return false;
    }

    char32_t parseUnicodeEscape()
    {
        char32_t codePoint = 0;
        for (int i = 0; i < 4; ++i)
        {
            if (isEnd())
            {
                throw std::runtime_error("JSON Unicode 转义不完整");
            }

            const char ch = get();
            codePoint <<= 4;
            if (ch >= '0' && ch <= '9')
            {
                codePoint |= static_cast<char32_t>(ch - '0');
            }
            else if (ch >= 'a' && ch <= 'f')
            {
                codePoint |= static_cast<char32_t>(10 + ch - 'a');
            }
            else if (ch >= 'A' && ch <= 'F')
            {
                codePoint |= static_cast<char32_t>(10 + ch - 'A');
            }
            else
            {
                throw std::runtime_error("JSON Unicode 转义包含非法字符");
            }
        }

        return codePoint;
    }

    static void appendUtf8CodePoint(std::string& output, char32_t codePoint)
    {
        if (codePoint <= 0x7F)
        {
            output.push_back(static_cast<char>(codePoint));
        }
        else if (codePoint <= 0x7FF)
        {
            output.push_back(static_cast<char>(0xC0 | ((codePoint >> 6) & 0x1F)));
            output.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
        }
        else if (codePoint <= 0xFFFF)
        {
            output.push_back(static_cast<char>(0xE0 | ((codePoint >> 12) & 0x0F)));
            output.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
            output.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
        }
        else
        {
            output.push_back(static_cast<char>(0xF0 | ((codePoint >> 18) & 0x07)));
            output.push_back(static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F)));
            output.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
            output.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
        }
    }

    void skipWhitespace()
    {
        while (!isEnd())
        {
            const char ch = peek();
            if (ch != ' ' && ch != '\n' && ch != '\r' && ch != '\t')
            {
                break;
            }

            advance();
        }
    }

    bool consume(char expected)
    {
        if (!isEnd() && peek() == expected)
        {
            advance();
            return true;
        }

        return false;
    }

    void expect(char expected)
    {
        if (isEnd() || peek() != expected)
        {
            throw std::runtime_error(std::string("JSON 期待字符: ") + expected);
        }

        advance();
    }

    char peek() const
    {
        return text_[position_];
    }

    char get()
    {
        return text_[position_++];
    }

    void advance()
    {
        ++position_;
    }

    bool isEnd() const
    {
        return position_ >= text_.size();
    }

    std::string text_;
    std::size_t position_ = 0;
};

const JsonValue* findField(const JsonValue& object, const std::string& key)
{
    const auto it = object.objectValue.find(key);
    if (it == object.objectValue.end())
    {
        return nullptr;
    }

    return &it->second;
}

std::string requireString(const JsonValue& object, const std::string& key)
{
    const JsonValue* value = findField(object, key);
    if (value == nullptr || !value->isString())
    {
        throw std::runtime_error("缺少字符串字段: " + key);
    }

    return value->stringValue;
}

std::string optionalString(const JsonValue& object, const std::string& key,
                           const std::string& defaultValue = std::string())
{
    const JsonValue* value = findField(object, key);
    if (value == nullptr)
    {
        return defaultValue;
    }

    if (!value->isString())
    {
        throw std::runtime_error("字段类型不正确: " + key);
    }

    return value->stringValue;
}

int requireInt(const JsonValue& object, const std::string& key)
{
    const JsonValue* value = findField(object, key);
    if (value == nullptr || !value->isNumber())
    {
        throw std::runtime_error("缺少数字字段: " + key);
    }

    return static_cast<int>(value->numberValue);
}

int optionalInt(const JsonValue& object, const std::string& key, int defaultValue)
{
    const JsonValue* value = findField(object, key);
    if (value == nullptr)
    {
        return defaultValue;
    }

    if (!value->isNumber())
    {
        throw std::runtime_error("字段类型不正确: " + key);
    }

    return static_cast<int>(value->numberValue);
}

EventEffectType parseEventEffectType(const std::string& typeName)
{
    if (typeName == "none")
    {
        return EventEffectType::None;
    }

    if (typeName == "heal")
    {
        return EventEffectType::Heal;
    }

    if (typeName == "lose_health")
    {
        return EventEffectType::LoseHealth;
    }

    if (typeName == "gain_gold")
    {
        return EventEffectType::GainGold;
    }

    if (typeName == "lose_gold")
    {
        return EventEffectType::LoseGold;
    }

    if (typeName == "add_card")
    {
        return EventEffectType::AddCard;
    }

    if (typeName == "remove_card")
    {
        return EventEffectType::RemoveCard;
    }

    if (typeName == "upgrade_card")
    {
        return EventEffectType::UpgradeCard;
    }

    throw std::runtime_error("未知的事件效果类型: " + typeName);
}

EventEffect parseEffect(const JsonValue& value)
{
    if (!value.isObject())
    {
        throw std::runtime_error("事件效果必须是对象");
    }

    EventEffect effect;
    effect.type = parseEventEffectType(requireString(value, "type"));
    effect.value = optionalInt(value, "value", 0);
    effect.parameter = optionalString(value, "parameter");
    return effect;
}

EventOption parseOption(const JsonValue& value)
{
    if (!value.isObject())
    {
        throw std::runtime_error("事件选项必须是对象");
    }

    EventOption option;
    option.text = requireString(value, "text");
    option.condition = optionalString(value, "condition");
    option.state = optionalInt(value, "state", 0);
    option.nextState = optionalInt(value, "next_state", -1);
    option.closesEvent = optionalInt(value, "closes_event", 1) != 0;

    const JsonValue* effectsValue = findField(value, "effects");
    if (effectsValue == nullptr || !effectsValue->isArray())
    {
        throw std::runtime_error("事件选项缺少 effects 数组");
    }

    for (const JsonValue& effectValue : effectsValue->arrayValue)
    {
        option.effects.push_back(parseEffect(effectValue));
    }

    return option;
}

EventState parseState(const JsonValue& value)
{
    if (!value.isObject())
    {
        throw std::runtime_error("事件阶段必须是对象");
    }

    EventState state;
    state.text = requireString(value, "text");
    state.imagePath = optionalString(value, "image");
    state.soundPath = optionalString(value, "sound");
    return state;
}

EventDefinition parseEvent(const JsonValue& value)
{
    if (!value.isObject())
    {
        throw std::runtime_error("事件定义必须是对象");
    }

    EventDefinition event;
    event.id = requireString(value, "id");
    event.title = requireString(value, "title");
    event.description = requireString(value, "description");
    event.imagePath = optionalString(value, "image");
    event.soundPath = optionalString(value, "sound");
    event.act = optionalInt(value, "act", 1);
    event.weight = optionalInt(value, "weight", 0);

    const JsonValue* statesValue = findField(value, "states");
    if (statesValue != nullptr)
    {
        if (!statesValue->isArray())
        {
            throw std::runtime_error("事件 states 字段必须是数组");
        }

        for (const JsonValue& stateValue : statesValue->arrayValue)
        {
            event.states.push_back(parseState(stateValue));
        }
    }

    if (event.states.empty())
    {
        EventState defaultState;
        defaultState.text = event.description;
        defaultState.imagePath = event.imagePath;
        defaultState.soundPath = event.soundPath;
        event.states.push_back(std::move(defaultState));
    }

    const JsonValue* optionsValue = findField(value, "options");
    if (optionsValue == nullptr || !optionsValue->isArray())
    {
        throw std::runtime_error("事件定义缺少 options 数组");
    }

    for (const JsonValue& optionValue : optionsValue->arrayValue)
    {
        event.options.push_back(parseOption(optionValue));
    }

    return event;
}
} // namespace

std::string eventEffectTypeToString(EventEffectType type)
{
    switch (type)
    {
    case EventEffectType::None:
        return "none";
    case EventEffectType::Heal:
        return "heal";
    case EventEffectType::LoseHealth:
        return "lose_health";
    case EventEffectType::GainGold:
        return "gain_gold";
    case EventEffectType::LoseGold:
        return "lose_gold";
    case EventEffectType::AddCard:
        return "add_card";
    case EventEffectType::RemoveCard:
        return "remove_card";
    case EventEffectType::UpgradeCard:
        return "upgrade_card";
    }

    return "none";
}

EventEffectType eventEffectTypeFromString(const std::string& typeName)
{
    return parseEventEffectType(typeName);
}

bool EventDatabase::loadFromFile(const std::string& filePath)
{
    events_.clear();
    eventIdsByAct_.clear();
    lastError_.clear();

    std::ifstream input(filePath, std::ios::binary);
    if (!input)
    {
        lastError_ = "无法打开事件文件: " + filePath;
        return false;
    }

    std::string content((std::istreambuf_iterator<char>(input)),
                        std::istreambuf_iterator<char>());

    try
    {
        JsonParser parser(std::move(content));
        JsonValue root = parser.parse();
        if (!root.isArray())
        {
            throw std::runtime_error("事件文件根节点必须是数组");
        }

        for (const JsonValue& eventValue : root.arrayValue)
        {
            EventDefinition event = parseEvent(eventValue);
            eventIdsByAct_[event.act].push_back(event.id);
            events_.emplace(event.id, std::move(event));
        }
    }
    catch (const std::exception& exception)
    {
        events_.clear();
        eventIdsByAct_.clear();
        lastError_ = exception.what();
        return false;
    }

    return true;
}

const EventDefinition& EventDatabase::getEvent(const std::string& eventId) const
{
    const auto it = events_.find(eventId);
    if (it == events_.end())
    {
        throw std::out_of_range("不存在的事件ID: " + eventId);
    }

    return it->second;
}

std::vector<std::string> EventDatabase::getActOneEventIds() const
{
    return getEventIdsForAct(1);
}

std::vector<std::string> EventDatabase::getEventIdsForAct(int act) const
{
    const auto it = eventIdsByAct_.find(act);
    if (it == eventIdsByAct_.end())
    {
        return {};
    }

    return it->second;
}

bool EventDatabase::hasEvent(const std::string& eventId) const
{
    return events_.find(eventId) != events_.end();
}

std::size_t EventDatabase::getEventCount() const
{
    return events_.size();
}

const std::string& EventDatabase::getLastError() const
{
    return lastError_;
}
