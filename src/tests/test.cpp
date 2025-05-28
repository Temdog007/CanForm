#include <array>
#include <filesystem>
#include <iostream>
#include <range.hpp>
#include <sstream>
#include <tests/test.hpp>

namespace CanForm
{
StringList makeList(std::initializer_list<std::string_view> list)
{
    StringList s;
    for (auto item : list)
    {
        s.emplace_back(item, nullptr);
    }
    return s;
}

template <typename T> constexpr RangedValue makeNumber(T t) noexcept
{
    Range<T> r(t);
    return RangedValue(std::move(r));
}

Form makeForm()
{
    Form form;
    form["Flag"] = rand() % 2 == 0;
    form["Signed Integer"] = makeNumber(0);
    form["Unsigned Integer"] = makeNumber(0u);
    form["Float"] = makeNumber(0.f);
    form["1-10"] = *Range<uint8_t>::create(5, 1, 10);
    form["String"] = "Hello";

    constexpr std::array<std::string_view, 5> Classes = {"Mammal", "Bird", "Reptile", "Amphibian", "Fish"};
    StringSelection selection;
    selection.index = rand() % Classes.size();
    for (auto cls : Classes)
    {
        selection.set.emplace(cls);
    }
    form["Single Selection"] = std::move(selection);

    constexpr std::array<std::string_view, 4> Actions = {"Climb", "Swim", "Fly", "Dig"};
    StringMap map;
    for (auto a : Actions)
    {
        map.emplace(a, rand() % 2 == 0);
    }
    form["Multiple Selections"] = std::move(map);

    form["Ordering"] = makeList({"First", "Second", "Third", "Fourth", "Fifth"});

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
    form["Expression"] = std::move(c);

    MultiForm multi;
    multi.tabs["Extra1"] = Form::create("Age", makeNumber(static_cast<uint8_t>(42)), "Favorite Color",
                                        StringSelection({"Red", "Green", "Blue"}));
    multi.tabs["Extra2"] = Form::create("Active", true, "Weight", makeNumber(0.0), "Sports",
                                        createStringMap("Basketball", "Football", "Golf", "Polo"));
    multi.selected = "Extra1";
    form["Extra"] = std::move(multi);
    return form;
}

struct Printer
{
    std::ostream &os;

    constexpr Printer(std::ostream &os) noexcept : os(os)
    {
    }

    std::ostream &operator()(const StringSelection &s)
    {
        int i = 0;
        for (const auto &name : s.set)
        {
            os << '\t' << name;
            if (i == s.index)
            {
                os << "✔";
            }
            os << std::endl;
            ++i;
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

    std::ostream &operator()(const StringList &list)
    {
        for (auto &[name, _] : list)
        {
            os << name << std::endl;
        }
        return os;
    }

    std::ostream &operator()(const MultiForm &multi)
    {
        os << "Selected " << multi.selected << std::endl;
        auto iter = multi.tabs.find(multi.selected);
        if (iter != multi.tabs.end())
        {
            const auto &form = iter->second;
            for (const auto &[name, formData] : *form)
            {
                os << "\tEntry: " << name << std::endl;
                std::visit(*this, *formData);
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
    for (const auto &[name, data] : *form)
    {
        os << name << " → ";
        std::visit(Printer(os), *data);
        os << std::endl;
    }
    auto s = os.str();
    showMessageBox(MessageBoxType::Information, "Form Data", s, parent);
}

} // namespace CanForm