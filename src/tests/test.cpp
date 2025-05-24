#include <array>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <tests/test.hpp>

namespace CanForm
{

Form makeForm()
{
    std::pmr::memory_resource *resource = std::pmr::new_delete_resource();
    Form form;
    form["Flag"] = false;
    form["Signed Integer"] = 0;
    form["Unsigned Integer"] = 0u;
    form["Float"] = 0.f;
    form["String"] = String("Hello", resource);

    constexpr std::array<std::string_view, 5> Classes = {"Mammal", "Bird", "Reptile", "Amphibian", "Fish"};
    form["Single Selection"] = StringSelection(Classes);

    constexpr std::array<std::string_view, 4> Actions = {"Climb", "Swim", "Fly", "Dig"};
    StringMap map;
    for (auto a : Actions)
    {
        map.emplace(a, rand() % 2 == 0);
    }
    form["Multiple Selections"] = std::move(map);

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
    multi.tabs["Extra1"] =
        Form::create("Age", static_cast<uint8_t>(42), "Favorite Color", StringSelection({"Red", "Green", "Blue"}));
    multi.tabs["Extra2"] = Form::create("Active", true, "Weight", 0.0, "Sports",
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

    template <typename T> std::ostream &operator()(const T &t)
    {
        return os << t;
    }
};

void printForm(const Form &form, DialogResult result, void *parent)
{
    switch (result)
    {
    case DialogResult::Ok: {
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
    break;
    case DialogResult::Cancel:
        showMessageBox(MessageBoxType::Warning, "Cancel", "Form Canceled", parent);
        break;
    default:
        showMessageBox(MessageBoxType::Error, "Error", "Form Failed", parent);
        break;
    }
}

double randomDouble() noexcept
{
    return ((double)rand() / RAND_MAX);
}

void RandomRender::randomPosition(double &x, double &y)
{
    x = randomDouble() * size.first;
    y = randomDouble() * size.second;
}

void RandomRender::operator()(RenderStyle &style)
{
    if constexpr (Color::UseFloat)
    {
        style.color.red = randomDouble();
        style.color.green = randomDouble();
        style.color.blue = randomDouble();
        style.color.alpha = 1.0;
    }
    else
    {
        style.color.red = rand() % 256;
        style.color.green = rand() % 256;
        style.color.blue = rand() % 256;
        style.color.alpha = 255u;
    }
    if (rand() % 2 == 0)
    {
        style.lineWidth.emplace(randomDouble() * 10);
    }
    else
    {
        style.lineWidth.reset();
    }
}

void RandomRender::operator()(CanFormRectangle &r)
{
    randomPosition(r.x, r.y);
    r.w = rand() % 50 + 10;
    r.h = rand() % 50 + 10;
}

void RandomRender::operator()(CanFormEllipse &r)
{
    randomPosition(r.x, r.y);
    r.w = rand() % 50 + 10;
    r.h = rand() % 50 + 10;
}

void RandomRender::operator()(RoundedRectangle &r)
{
    operator()(r.rectangle);
    r.radius = std::min(r.rectangle.w, r.rectangle.h) * randomDouble() * 0.4 + 0.1;
}

void RandomRender::operator()(Text &t)
{
    randomPosition(t.x, t.y);
    t.string = randomString(5, 10);
    switch (rand() % 5)
    {
    case 0:
        t.string.insert(0, "<b>");
        t.string.append("</b>");
        break;
    case 1:
        t.string.insert(0, "<i>");
        t.string.append("</i>");
        break;
    case 2:
        t.string.insert(0, "<u>");
        t.string.append("</u>");
        break;
    default:
        break;
    }
}

RenderAtom RandomRender::operator()()
{
    RenderAtom atom;
    operator()(atom.style);
    switch (rand() % 4)
    {
    case 0: {
        auto &e = atom.renderType.emplace<CanFormEllipse>();
        operator()(e);
    }
    break;
    case 1: {
        auto &r = atom.renderType.emplace<RoundedRectangle>();
        operator()(r);
    }
    break;
    case 3: {
        auto &t = atom.renderType.emplace<Text>();
        operator()(t);
    }
    break;
    default: {
        auto &r = atom.renderType.emplace<CanFormRectangle>();
        operator()(r);
    }
    break;
    }
    return atom;
}
} // namespace CanForm