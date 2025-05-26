#pragma once

#include "types.hpp"

#include <memory>
#include <string_view>

namespace CanForm
{
struct MenuList;

struct MenuItem
{
    using NewMenu = std::shared_ptr<std::pair<String, MenuList>>;
    using Result = std::variant<bool, NewMenu>;

    String label;
    virtual ~MenuItem()
    {
    }

    virtual Result onClick() = 0;
};

using MenuItems = std::pmr::vector<std::unique_ptr<MenuItem>>;

template <typename F> class MenuItemLambda : public MenuItem
{
    static_assert(std::is_invocable_r<MenuItem::Result, F>::value);

  private:
    F func;

  public:
    MenuItemLambda(F &&f) noexcept : func(std::move(f))
    {
    }
    virtual ~MenuItemLambda()
    {
    }

    virtual MenuItem::Result onClick() override
    {
        return func();
    }
};

template <typename F> MenuItemLambda<F> makeItemMenu(String &&s, F &&f)
{
    MenuItemLambda menuItem(std::move(f));
    menuItem.label = std::move(s);
    return menuItem;
}

struct Menu
{
    String title;
    MenuItems items;

    template <typename T, typename... Args> void add(Args &&...args)
    {
        auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
        items.emplace_back(std::move(ptr));
    }

    template <typename F> void add(String &&s, F &&f)
    {
        auto ptr = std::make_unique<MenuItemLambda<F>>(std::move(f));
        ptr->label = std::move(s);
        items.emplace_back(std::move(ptr));
    }
};

using Menus = std::pmr::vector<Menu>;

struct MenuList
{
    Menus menus;
    virtual ~MenuList()
    {
    }

    void show(std::string_view, void *parent = nullptr);
};
} // namespace CanForm