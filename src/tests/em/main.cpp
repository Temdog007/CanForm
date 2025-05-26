#include <canform.hpp>
#include <em/em.hpp>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <tests/test.hpp>

using namespace CanForm;

void run();
bool showMenu(int, const EmscriptenMouseEvent *, void *);

int main()
{
    if (!emscripten_has_asyncify())
    {
        fprintf(stderr, "Asyncify is not enabled\n");
        return -1;
    }
    srand(time(nullptr));

    EM_ASM({
        let button = document.createElement("button");
        button.innerText = '☰';
        button.id = 'menuButton';
        document.body.append(button);
    });
    emscripten_set_click_callback("#menuButton", nullptr, false, showMenu);

    emscripten_set_main_loop(run, -1, false);
    return 0;
}

void run()
{
}

bool showMenu(int, const EmscriptenMouseEvent *, void *)
{
    MenuList menuList;
    {
        auto &menu = menuList.menus.emplace_back();
        menu.title = "Modal Tests";
        menu.add("Modal Form", []() {
            Form form = makeForm();
            printForm(form, executeForm("Modal Form", form, 2));
            return false;
        });
        menu.add("Non Modal Form", []() {
            showAsyncForm(
                makeForm(), "Non Modal Form", [](Form &form, DialogResult result) { printForm(form, result); }, 2);
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
            return makeNewMenu<true>("New Menu", std::move(menuList));
        });
    }

    menuList.show("Main Menu");
    return true;
}