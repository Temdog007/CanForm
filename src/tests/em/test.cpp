#include <canform.hpp>
#include <em/em.hpp>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <tests/test.hpp>

static bool executeItem(int, const EmscriptenMouseEvent *, void *);
static void removeElement(const char *);

struct MenuItemClick
{
    int id;
    CanForm::MenuItem *item;
    constexpr MenuItemClick(int i, CanForm::MenuItem *m) noexcept : id(i), item(m)
    {
    }
    void operator()(bool);
    void operator()(CanForm::MenuItem::NewMenu &&);
    void execute()
    {
        std::visit(*this, item->onClick());
    }
    void removeDialog();
};

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

void showMessageBox(MessageBoxType type, std::string_view title, std::string_view message, void *)
{
    EM_ASM(
        {
            let dialog = document.createElement("dialog");

            let h1 = document.createElement("h1");
            h1.innerText = UTF8ToString($0);
            dialog.append(h1);

            let h2 = document.createElement("h2");
            h2.innerText = UTF8ToString($1, $2);
            dialog.append(h2);

            let p = document.createElement("p");
            p.innerText = UTF8ToString($3, $4);
            dialog.append(p);

            let button = document.createElement("button");
            button.innerText = '✖';
            button.id = "closeButton";
            button.onclick = function()
            {
                dialog.remove();
            };
            dialog.append(button);

            document.body.append(dialog);
            dialog.showModal();
            dialog.classList.add(h1.innerText);
        },
        toString(type), title.data(), title.size(), message.data(), message.size());
}

struct ResponseHandler
{
    std::shared_ptr<QuestionResponse> response;
    int id;

    void checkLater()
    {
        emscripten_set_timeout(&ResponseHandler::checkForAnswerToQuestion, 10, this);
    }

    static void checkForAnswerToQuestion(void *userData)
    {
        ResponseHandler *handler = (ResponseHandler *)userData;
        const int response = EM_ASM_INT(
            {
                let id = $0;
                let dialog = document.getElementById("dialog_" + id.toString());
                if (!dialog)
                {
                    return 0;
                }
                if (dialog.open)
                {
                    return -1;
                }
                dialog.remove();
                return dialog.returnvalue == "yes" ? 1 : 0;
            },
            handler->id);
        switch (response)
        {
        case 0:
            handler->response->no();
            delete handler;
            break;
        case 1:
            handler->response->yes();
            delete handler;
            break;
        default:
            handler->checkLater();
            break;
        }
    }
};

void askQuestion(std::string_view title, std::string_view question, const std::shared_ptr<QuestionResponse> &response,
                 void *)
{
    ResponseHandler *handler = new ResponseHandler();
    handler->response = response;
    handler->id = rand();
    EM_ASM(
        {
            let title = $0;
            let titleLength = $1;
            let content = $2;
            let contentLength = $3;
            let id = $4;

            let dialog = document.createElement("dialog");
            dialog.id = 'dialog_' + id.toString();

            let h1 = document.createElement("h1");
            h1.innerText = UTF8ToString(title, titleLength);
            dialog.append(h1);

            let p = document.createElement("p");
            p.innerText = UTF8ToString(content, contentLength);
            dialog.append(p);

            let button = document.createElement("button");
            button.innerText = 'Yes';
            button.onclick = function()
            {
                dialog.close();
            };
            dialog.append(button);

            button = document.createElement("button");
            button.classList.add("yes");
            button.innerText = 'Yes';
            button.onclick = function()
            {
                dialog.close("yes");
            };
            dialog.append(button);

            button = document.createElement("button");
            button.classList.add("no");
            button.innerText = 'No';
            button.onclick = function()
            {
                dialog.close("no");
            };
            dialog.append(button);

            document.body.append(dialog);
            dialog.showModal();
        },
        title.data(), title.size(), question.data(), question.size(), handler->id);
    handler->checkLater();
}

struct MenuHandler
{
    std::shared_ptr<MenuList> menuList;
    std::vector<std::unique_ptr<MenuItemClick>> itemClicks;
    int id;

    void checkLater()
    {
        emscripten_set_timeout(&ResponseHandler::checkForAnswerToQuestion, 1000, this);
    }

    static void checkIfElementWasRemoved(void *userData)
    {
        MenuHandler *handler = (MenuHandler *)userData;
        const int removed = EM_ASM_INT(
            {
                let id = $0;
                let dialog = document.getElementById("dialog_" + id.toString());
                if (!dialog)
                {
                    return true;
                }
                if (dialog.open)
                {
                    return false;
                }
                dialog.remove();
                return true;
            },
            handler->id);
        if (removed)
        {
            delete handler;
        }
    }
};

void showMenu(std::string_view title, const std::shared_ptr<MenuList> &menuList, void *)
{
    MenuHandler *handler = new MenuHandler();
    handler->menuList = menuList;
    handler->id = rand();
    EM_ASM(
        {
            let id = $0;
            let dialog = document.createElement("dialog");
            dialog.id = 'dialog_' + id.toString();

            let h1 = document.createElement("h1");
            h1.innerText = UTF8ToString($1, $2);
            dialog.append(h1);

            let button = document.createElement("button");
            button.innerText = '✖';
            button.id = "closeButton";
            button.onclick = function()
            {
                dialog.remove();
            };
            dialog.append(button);

            let tabs = document.createElement("div");
            tabs.id = 'tab_' + id.toString();
            tabs.classList.add("tab");
            dialog.append(tabs);

            let hr = document.createElement("hr");
            dialog.append(hr);

            document.body.append(dialog);
            dialog.showModal();
        },
        handler->id, title.data(), title.size());
    for (auto &menu : menuList->menus)
    {
        EM_ASM(
            {
                let id = $0;
                let title = UTF8ToString($1);

                let dialog = document.getElementById('dialog_' + id.toString());
                let tabs = document.getElementById('tab_' + id.toString());

                let tabContent = document.createElement("div");
                tabContent.id = title + id.toString();
                tabContent.classList.add("tabContent");
                dialog.append(tabContent);

                let tabButton = document.createElement("button");
                tabButton.classList.add("tabButton");
                tabButton.innerText = title;
                tabButton.onclick = function()
                {
                    for (let child of document.getElementsByClassName("tabContent"))
                    {
                        child.classList.remove("active");
                    }
                    for (let child of document.getElementsByClassName("tabButton"))
                    {
                        child.classList.remove("active");
                    }
                    tabButton.classList.add("active");
                    tabContent.classList.add("active");
                };
                tabs.append(tabButton);
            },
            handler->id, menu.title.c_str());
        for (auto &item : menu.items)
        {
            char *buttonQuery = (char *)EM_ASM_PTR(
                {
                    let id = $0;
                    let title = UTF8ToString($1);
                    let buttonid = UTF8ToString($2);
                    let id2 = $3;

                    let dialog = document.getElementById('dialog_' + id.toString());
                    let div = document.getElementById(title + id.toString());

                    let button = document.createElement("button");
                    button.id = 'button_' + id2.toString();
                    button.innerText = buttonid;
                    div.append(button);

                    dialog.append(div);

                    return stringToNewUTF8('#' + button.id);
                },
                handler->id, menu.title.c_str(), item->label.c_str(), rand());
            auto &ptr = handler->itemClicks.emplace_back(std::make_unique<MenuItemClick>(handler->id, item.get()));
            emscripten_set_click_callback(buttonQuery, ptr.get(), false, executeItem);
            free(buttonQuery);
        }
    }
    handler->checkLater();
}

struct AwaiterHandler
{
    std::shared_ptr<Awaiter> awaiter;
    int id;

    AwaiterHandler(std::shared_ptr<Awaiter> a, int i) : awaiter(a), id(i)
    {
    }
};

void checkAwaiter(void *userData)
{
    AwaiterHandler *a = (AwaiterHandler *)userData;
    if (a == nullptr)
    {
        return;
    }
    if (a->awaiter->isDone())
    {
        EM_ASM(
            {
                let id = $0;
                let dialog = document.getElementById('dialog_' + id.toString());
                dialog.remove();
            },
            a->id);
        delete a;
    }
    else
    {
        emscripten_set_timeout(checkAwaiter, 10, userData);
    }
}

void showPopupUntil(std::string_view message, const std::shared_ptr<Awaiter> &awaiter, size_t, void *)
{
    const int id = rand();
    EM_ASM(
        {
            let id = $0;
            let dialog = document.createElement("dialog");
            dialog.id = 'dialog_' + id.toString();

            let h1 = document.createElement("h1");
            h1.innerText = UTF8ToString($1, $2);
            dialog.append(h1);

            document.body.append(dialog);
            dialog.showModal();
        },
        id, message.data(), message.size());
    emscripten_set_timeout(checkAwaiter, 1, new AwaiterHandler(awaiter, id));
}

void FormExecute::execute(std::string_view, const std::shared_ptr<FormExecute> &, size_t, void *)
{
}

} // namespace CanForm

static bool executeItem(int, const EmscriptenMouseEvent *, void *userData)
{
    using namespace CanForm;
    MenuItemClick *itemClick = (MenuItemClick *)userData;
    if (itemClick == nullptr)
    {
        return true;
    }
    itemClick->execute();
    return true;
}

static void removeElement(const char *id)
{
    EM_ASM(
        {
            let id = UTF8ToString($0);
            let e = document.getElementById(id);
            if (e)
            {
                e.remove();
            }
        },
        id);
}

void MenuItemClick::removeDialog()
{
    char buffer[256];
    std::snprintf(buffer, sizeof(buffer), "dialog_%d", id);
    removeElement(buffer);
}

void MenuItemClick::operator()(bool b)
{
    if (b)
    {
        removeDialog();
    }
}

void MenuItemClick::operator()(CanForm::MenuItem::NewMenu &&p)
{
    removeDialog();
    CanForm::showMenu(p.first, p.second);
}