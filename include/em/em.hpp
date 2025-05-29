#pragma once

#include <canform.hpp>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <list>

namespace CanForm
{
constexpr const char *toString(MessageBoxType type) noexcept
{
    switch (type)
    {
    case MessageBoxType::Information:
        return "Information";
    case MessageBoxType::Warning:
        return "Warning";
    case MessageBoxType::Error:
        return "Error";
    default:
        return nullptr;
    }
}

struct MenuItemClick
{
    int id;
    MenuItem *item;
    constexpr MenuItemClick(int i, MenuItem *m) noexcept : id(i), item(m)
    {
    }
    void operator()(bool);
    void operator()(MenuItem::NewMenu &&);
    void execute()
    {
        std::visit(*this, item->onClick());
    }
    void removeDialog();
};

struct ResponseHandler
{
    std::shared_ptr<QuestionResponse> response;
    int id;

    void checkLater();
    static void checkForAnswerToQuestion(void *);
};

struct MenuHandler
{
    std::shared_ptr<MenuList> menuList;
    std::list<MenuItemClick> itemClicks;
    int id;

    void checkLater();
    static void checkIfElementWasRemoved(void *);
};

struct AwaiterHandler
{
    std::shared_ptr<Awaiter> awaiter;
    int id;

    void checkLater();
    static void checkIfDone(void *);
};

extern bool executeItem(int, const EmscriptenMouseEvent *, void *);
extern void removeElement(const char *);

} // namespace CanForm

extern "C"
{
    void EMSCRIPTEN_KEEPALIVE updateBoolean(bool &, bool);
    void EMSCRIPTEN_KEEPALIVE updateString(CanForm::String &, char *);
    void EMSCRIPTEN_KEEPALIVE updateSortableList(CanForm::SortableList &, char *, int, void *);
    void EMSCRIPTEN_KEEPALIVE updateRange(CanForm::IRange &, double);
    void EMSCRIPTEN_KEEPALIVE updateMultiForm(CanForm::MultiForm &, char *);
}