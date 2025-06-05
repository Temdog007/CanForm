#include <canform.hpp>
#include <em/em.hpp>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <filesystem>
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

class Handler : public FileDialog::Handler
{
  private:
    Handler() = default;

  public:
    virtual ~Handler()
    {
    }
    virtual bool handle(std::string_view file) override
    {
        showMessageBox(MessageBoxType::Information, "Got File/Directory", file);
        return true;
    }

    static std::shared_ptr<Handler> create()
    {
        return std::shared_ptr<Handler>(new Handler());
    }
};

bool OnMenuButton(int, const EmscriptenMouseEvent *, void *)
{
    MenuList menuList;
    {
        auto handler = Handler::create();
        auto &menu = menuList.menus.emplace_back();
        menu.title = "File";
        menu.add("Open File", [handler]() {
            FileDialog dialog;
            dialog.message = "Select file(s)";
            auto s = std::filesystem::current_path().string();
            dialog.startDirectory = s;
            dialog.multiple = true;
            dialog.show(handler);
            return MenuState::Close;
        });
        menu.add("Open Directory", [handler]() {
            FileDialog dialog;
            dialog.message = "Select directory(s)";
            auto s = std::filesystem::current_path().string();
            dialog.startDirectory = s;
            dialog.multiple = true;
            dialog.directories = true;
            dialog.show(handler);
            return MenuState::Close;
        });
        menu.add("Save File", [handler]() {
            FileDialog dialog;
            dialog.message = "Select file to save";
            auto s = std::filesystem::current_path().string();
            dialog.startDirectory = s;
            dialog.saving = true;
            dialog.show(handler);
            return MenuState::Close;
        });
    }
    {
        auto &menu = menuList.menus.emplace_back();
        menu.title = "Tests";
        menu.add("Show Example Form", []() {
            auto formExecute = executeForm([](const Form &form) { printForm(form); }, makeForm());
            FormExecute::execute("Modal Form", std::move(formExecute));
            return MenuState::KeepOpen;
        });
        menu.add("Wait for 3 seconds", []() {
            showPopupUntil("Waiting...", std::chrono::seconds(3), 500);
            return MenuState::KeepOpen;
        });
        menu.add("Replace Menu", []() {
            MenuList menuList;
            auto &menu = menuList.menus.emplace_back();
            menu.title = "Secondary Menu";
            menu.add("Close", []() { return MenuState::Close; });
            return makeNewMenu("New Menu", std::move(menuList));
        });
        menu.add("Long Menu", []() {
            MenuList menuList;
            auto &menu = menuList.menus.emplace_back();
            menu.title = "Long Menu";
            for (size_t i = 0; i < 100; ++i)
            {
                char buffer[1024];
                std::snprintf(buffer, sizeof(buffer), "#%zu", i + 1);
                menu.add(buffer, []() { return MenuState::Close; });
            }
            return makeNewMenu("Long Menu", std::move(menuList));
        });
    }
    {
        auto &menu = menuList.menus.emplace_back();
        menu.title = "Modals";
        menu.add("Information", []() {
            showMessageBox(MessageBoxType::Information, "Information Message", "This is information");
            return MenuState::KeepOpen;
        });
        menu.add("Warning", []() {
            showMessageBox(MessageBoxType::Warning, "Warning Message", "This is a warning");
            return MenuState::KeepOpen;
        });
        menu.add("Error", []() {
            showMessageBox(MessageBoxType::Error, "Error Message", "This is an error");
            return MenuState::KeepOpen;
        });
        menu.add("Question", []() {
            askQuestion("Question", "Yes or No?", SimpleResponse());
            return MenuState::KeepOpen;
        });
    }

    MenuList::show("Main Menu", std::move(menuList));
    return true;
}