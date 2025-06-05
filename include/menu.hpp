#pragma once

#include "types.hpp"

#include <list>
#include <tuple>

namespace CanForm
{
struct MenuList;
using MenuListPtr = std::shared_ptr<MenuList>;

enum class MenuState
{
    Close,
    KeepOpen
};

struct MenuItem
{
    using NewMenu = std::pair<String, MenuListPtr>;
    using Result = std::variant<MenuState, NewMenu>;

    String label;
    virtual ~MenuItem()
    {
    }

    virtual Result onClick(void *) = 0;
};

using MenuItems = std::pmr::vector<std::unique_ptr<MenuItem>>;

struct Menu
{
    MenuItems items;
    String title;

    Menu() = default;
    Menu(const Menu &) = delete;
    Menu(Menu &&) noexcept = default;

    Menu &operator=(const Menu &) = delete;
    Menu &operator=(Menu &&) noexcept = default;

    template <typename T, typename... Args> MenuItem &add(Args &&...args)
    {
        auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
        return *items.emplace_back(std::move(ptr));
    }

    template <typename F> void add(String &&, F &&);
    template <typename S, typename F, std::enable_if_t<std::is_convertible_v<S, String>, bool> = true>
    void add(const S &s, F &&f)
    {
        return add(String(s), std::move(f));
    }
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

    static void show(std::string_view, const std::shared_ptr<MenuList> &, void *parent = nullptr);

    template <typename T, std::enable_if_t<std::is_base_of<MenuList, T>::value, bool> = true>
    static inline void show(std::string_view title, T &&t, void *parent = nullptr)
    {
        show(title, std::make_shared<T>(std::move(t)), parent);
    }
};

template <typename F> class MenuItemLambda : public MenuItem
{
    static_assert(std::is_invocable_r<MenuItem::Result, F>::value || std::is_invocable<F, void *>::value);

  private:
    F func;

  public:
    MenuItemLambda(F &&f) noexcept : func(std::move(f))
    {
    }
    virtual ~MenuItemLambda()
    {
    }

    virtual MenuItem::Result onClick(void *parent) override
    {
        if constexpr (std::is_invocable<F, void *>::value)
        {
            return func(parent);
        }
        else
        {
            return func();
        }
    }
};

template <typename F> static inline MenuItemLambda<F> makeItemMenu(String &&s, F &&f)
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

template <typename A, typename B> static inline typename MenuItem::NewMenu makeNewMenu(A &&a, B &&b)
{
    return std::make_pair(std::move(a), std::make_shared<MenuList>(std::move(b)));
}

} // namespace CanForm