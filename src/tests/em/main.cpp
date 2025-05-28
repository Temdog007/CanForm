#include <canform.hpp>
#include <em/em.hpp>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <tests/test.hpp>

using namespace CanForm;

bool OnMenuButton(int, const EmscriptenMouseEvent *, void *);

int main()
{
    srand(time(nullptr));

    EM_ASM({
        let button = document.createElement("button");
        button.innerText = '☰';
        button.id = 'menuButton';
        document.body.append(button);
    });
    emscripten_set_click_callback("#menuButton", nullptr, false, OnMenuButton);

    return 0;
}

template <typename T> std::shared_ptr<T> make_shared(T &&t)
{
    return std::make_shared<T>(std::move(t));
}

bool OnMenuButton(int, const EmscriptenMouseEvent *, void *)
{
    MenuList menuList;
    {
        auto &menu = menuList.menus.emplace_back();
        menu.title = "Modal Tests";
        menu.add("Show Modal Form", []() {
            auto formExecute = make_shared(executeForm([](const Form &form) { printForm(form); }, makeForm()));
            FormExecute::execute("Modal Form", formExecute, 2);
            return false;
        });
        menu.add("Information", []() {
            showMessageBox(MessageBoxType::Information, "Information Message", "This is information");
            return false;
        });
        menu.add("Warning", []() {
            showMessageBox(MessageBoxType::Warning, "Warning Message", "This is a warning");
            return false;
        });
        menu.add("Error", []() {
            showMessageBox(MessageBoxType::Error, "Error Message", "This is an error");
            return false;
        });
    }
    {
        auto &menu = menuList.menus.emplace_back();
        menu.title = "Other Tests";
        menu.add("Wait for 3 seconds", []() {
            showPopupUntil("Waiting...", std::chrono::seconds(3), 500);
            return false;
        });
        menu.add("Replace Menu", []() {
            MenuList menuList;
            auto &menu = menuList.menus.emplace_back();
            menu.title = "Secondary Menu";
            menu.add("Close", []() { return true; });
            return makeNewMenu("New Menu", std::move(menuList));
        });
    }

    MenuList::show("Main Menu", std::make_shared<MenuList>(std::move(menuList)));
    return true;
}