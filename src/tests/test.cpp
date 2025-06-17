#include <array>
#include <filesystem>
#include <iostream>
#include <range.hpp>
#include <sstream>
#include <tests/test.hpp>

using namespace std::string_view_literals;

namespace CanForm
{
template <typename T> constexpr RangedValue makeNumber(T t) noexcept
{
    Range<T> r(t);
    return RangedValue(std::move(r));
}

Form makeForm(bool makeInner)
{
    StructForm forms(3);
    forms["Nothing"] = std::monostate{};
    forms["Boolean"] = rand() % 2 == 0;
    forms["Signed Integer"] = makeNumber(0);
    forms["Unsigned Integer"] = makeNumber(0u);
    forms["Float"] = makeNumber(0.f);
    forms["1-10"] = *Range<uint8_t>::create(5, 1, 10);
    forms["String"] = "Hello";

    constexpr std::array<std::string_view, 5> Classes = {"Mammal", "Bird", "Reptile", "Amphibian", "Fish"};
    StringSelection selection;
    selection.index = rand() % Classes.size();
    for (auto cls : Classes)
    {
        selection.set.emplace(cls);
    }
    forms["Set of Strings"] = std::move(selection);

    constexpr std::array<std::string_view, 4> Actions = {"Climb", "Swim", "Fly", "Dig"};
    StringMap map;
    for (auto a : Actions)
    {
        map.emplace(a, rand() % 2 == 0);
    }
    forms["Map of String to Boolean"] = std::move(map);

    {
        StringSet set({"A", "B", "C"});
        forms["Editable Set"] = std::move(set);
    }

    ComplexString c;
    c.string = "2 + 2";
    StringSet u;
    u.emplace("+");
    u.emplace("-");
    u.emplace("×");
    u.emplace("÷");
    c.map.emplace("Binary Operator", std::move(u));
    u.emplace("−");
    u.emplace("√");
    c.map.emplace("Unary Operator", std::move(u));
    forms["Expression"] = std::move(c);

    VariantForm variant;
    StringSet set({"Red", "Green", "Blue"});
    variant.map["1st Variant"] = StructForm::create("Age", makeNumber(static_cast<uint8_t>(42)), "Favorite Color",
                                                    StringSelection(0, std::move(set)));
    variant.map["2nd Variant"] = StructForm::create("Active", true, "Weight", makeNumber(0.0), "Sports",
                                                    createStringMap("Basketball", "Football", "Golf", "Polo"));
    variant.map["3rd Variant"] = true;
    variant.selected = "1st Variant";
    forms["Variant Form"] = std::move(variant);

    EnableForm enableForm;
    auto list = {"Bob"sv, "Alice"sv, "Greg"sv};
    for (auto item : list)
    {
        Form form;
        auto &s = form->emplace<String>();
        randomString(s, 5, 10);
        enableForm[item] = std::make_pair(rand() % 2 == 0, std::move(form));
    }
    forms["Enable Form"] = std::move(enableForm);

    if (makeInner)
    {
        forms["Struct Form"] = makeForm(false);
    }
    return Form(std::in_place, std::move(forms));
}

struct Printer
{
    std::ostream &os;
    size_t tabs;

    constexpr Printer(std::ostream &os) noexcept : os(os), tabs(0)
    {
    }

    void addTabs()
    {
        for (size_t i = 0; i < tabs; ++i)
        {
            os << '\t';
        }
    }

    std::ostream &operator()(std::monostate)
    {
        return os;
    }

    std::ostream &operator()(const StringSelection &s)
    {
        int i = 0;
        for (const auto &name : s.set)
        {
            addTabs();
            os << name << ' ';
            if (i == s.index)
            {
                os << "✔";
            }
            os << std::endl;
            ++i;
        }
        return os;
    }

    std::ostream &operator()(const StringSet &set)
    {
        for (const auto &name : set)
        {
            addTabs();
            os << name << std::endl;
        }
        return os;
    }

    std::ostream &operator()(const ComplexString &s)
    {
        return operator()(s.string);
    }

    std::ostream &operator()(const StringMap &map)
    {
        for (const auto &[name, flag] : map)
        {
            addTabs();
            os << name << ": " << flag << std::endl;
        }
        return os;
    }

    std::ostream &operator()(const Number &n)
    {
        return std::visit(*this, n);
    }

    std::ostream &operator()(const RangedValue &n)
    {
        return std::visit(*this, n);
    }

    std::ostream &operator()(const VariantForm &variant)
    {
        addTabs();
        os << "Variant Selected " << variant.selected << std::endl;
        auto iter = variant.map.find(variant.selected);
        if (iter != variant.map.end())
        {
            const auto &form = iter->second;
            ++tabs;
            std::visit(*this, *form);
            --tabs;
            os << std::endl;
        }
        return os;
    }

    std::ostream &operator()(const StructForm &structForm)
    {
        for (auto &[name, form] : *structForm)
        {
            addTabs();
            os << name << std::endl;
            ++tabs;
            std::visit(*this, *form);
            --tabs;
            os << std::endl;
        }
        return os;
    }

    std::ostream &operator()(const EnableForm &enableForm)
    {
        for (auto &[name, pair] : *enableForm)
        {
            addTabs();
            if (pair.first)
            {
                os << name << std::endl;
                ++tabs;
                std::visit(*this, *pair.second);
                --tabs;
                os << std::endl;
            }
        }
        return os;
    }

    std::ostream &operator()(int8_t t)
    {
        return operator()(static_cast<ssize_t>(t));
    }

    std::ostream &operator()(uint8_t t)
    {
        return operator()(static_cast<size_t>(t));
    }

    template <typename T> std::ostream &operator()(const Range<T> &r)
    {
        return operator()(*r);
    }

    template <typename T> std::ostream &operator()(const T &t)
    {
        return os << t;
    }
};

void printForm(const Form &form, void *parent)
{
    std::ostringstream os;
    std::visit(Printer(os), *form);
    auto s = os.str();
    showMessageBox(MessageBoxType::Information, "Form Data", s, parent);
}

} // namespace CanForm