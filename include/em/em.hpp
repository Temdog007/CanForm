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
    void *ptr;
    constexpr MenuItemClick(int i, MenuItem *m, void *p) noexcept : id(i), item(m), ptr(p)
    {
    }
    void operator()(MenuState);
    void operator()(MenuItem::NewMenu &&);
    void execute()
    {
        std::visit(*this, item->onClick(ptr));
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
    double EMSCRIPTEN_KEEPALIVE updateRange(CanForm::IRange &, double);
    void EMSCRIPTEN_KEEPALIVE updateVariantForm(CanForm::VariantForm &, char *);
    bool EMSCRIPTEN_KEEPALIVE updateHandler(CanForm::FileDialog::Handler &, char *);
    void EMSCRIPTEN_KEEPALIVE cancelHandler(CanForm::FileDialog::Handler &);

    void EMSCRIPTEN_KEEPALIVE addToStringSet(CanForm::StringSet &, char *);
    void EMSCRIPTEN_KEEPALIVE removeFromStringSet(CanForm::StringSet &, char *);
    void EMSCRIPTEN_KEEPALIVE updateStringSetDiv(CanForm::StringSet &, int);
}