#pragma once

#include "types.hpp"

#include <memory>
#include <string_view>

namespace CanForm
{
struct MenuList;

struct MenuItem
{
    using NewMenu = std::pair<String, MenuList>;
    using NewMenuPtr = std::shared_ptr<NewMenu>;
    using Result = std::variant<bool, NewMenuPtr>;

    String label;
    virtual ~MenuItem()
    {
    }

    virtual Result onClick() = 0;
};

using MenuItems = std::pmr::vector<std::unique_ptr<MenuItem>>;

struct Menu
{
    String title;
    MenuItems items;

    Menu() = default;
    Menu(const Menu &) = delete;
    Menu(Menu &&) noexcept = default;

    template <typename T, typename... Args> void add(Args &&...args)
    {
        auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
        items.emplace_back(std::move(ptr));
    }

    template <typename F> void add(String &&, F &&);
};

using Menus = std::pmr::vector<Menu>;

struct MenuList
{
    Menus menus;

    MenuList() = default;
    MenuList(const MenuList &) = delete;
    MenuList(MenuList &&) noexcept = default;

    virtual ~MenuList()
    {
    }

    void show(std::string_view, void *parent = nullptr);
};

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

template <typename F> void Menu::add(String &&s, F &&f)
{
    auto ptr = std::make_unique<MenuItemLambda<F>>(std::move(f));
    ptr->label = std::move(s);
    items.emplace_back(std::move(ptr));
}

template <bool S, typename A, typename B>
typename std::conditional<S, MenuItem::NewMenuPtr, MenuItem::NewMenu>::type makeNewMenu(A &&a, B &&b)
{
    auto pair = std::make_pair(std::move(a), std::move(b));
    if constexpr (S)
    {
        return std::make_shared<MenuItem::NewMenu>(std::move(pair));
    }
    else
    {
        return pair;
    }
}

} // namespace CanForm