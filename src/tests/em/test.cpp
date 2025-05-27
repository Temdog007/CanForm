#include <canform.hpp>
#include <em/em.hpp>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <tests/test.hpp>

static bool executeItem(int, const EmscriptenMouseEvent *, void *);

EM_ASYNC_JS(void, waitForDialog, (int id), {
    function sleep(ms)
    {
        return new Promise(function(resolve) { setTimeout(resolve, ms); });
    }
    while (true)
    {
        await sleep(1);
        let dialog = document.getElementById('dialog_' + id.toString());
        if (!dialog)
        {
            break;
        }
        if (!dialog.open)
        {
            let id = $0;
            let dialog = document.getElementById('dialog_' + id.toString());
            let rval = dialog.returncode == 'yes';
            dialog.remove();
            break;
        }
    }
});

EM_ASYNC_JS(void, blockUntilElementRemoved, (const char *ptr), {
    function sleep(ms)
    {
        return new Promise(function(resolve) { setTimeout(resolve, ms); });
    }
    while (true)
    {
        await sleep(1000);
        let id = UTF8ToString(ptr);
        if (!document.getElementById(id))
        {
            break;
        }
    }
});

struct ItemClick
{
    int id;
    CanForm::MenuItem *item;
    constexpr ItemClick(int i, CanForm::MenuItem *m) noexcept : id(i), item(m)
    {
    }
    void operator()(bool);
    void operator()(CanForm::MenuItem::NewMenuPtr);
    void execute()
    {
        std::visit(*this, item->onClick());
    }
    void removeElement();
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

bool askQuestion(std::string_view title, std::string_view question, void *)
{
    const int id = rand();
    EM_ASM(
        {
            let id = $4;
            let dialog = document.createElement("dialog");
            dialog.id = 'dialog_' + id.toString();

            let h1 = document.createElement("h1");
            h1.innerText = UTF8ToString($0, $1);
            dialog.append(h1);

            let p = document.createElement("p");
            p.innerText = UTF8ToString($2, $3);
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
        title.data(), title.size(), question.data(), question.size(), id);
    waitForDialog(id);
    return false;
}

void MenuList::show(std::string_view title, void *)
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
        id, title.data(), title.size());
    for (auto &menu : menus)
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
            id, menu.title.c_str());
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
                id, menu.title.c_str(), item->label.c_str(), rand());
            emscripten_set_click_callback(buttonQuery, new ItemClick(id, item.get()), false, executeItem);
            free(buttonQuery);
        }
    }

    char buffer[256];
    std::snprintf(buffer, sizeof(buffer), "dialog_%d", id);
    blockUntilElementRemoved(buffer);
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

    char buffer[256];
    std::snprintf(buffer, sizeof(buffer), "dialog_%d", id);
    blockUntilElementRemoved(buffer);
}

DialogResult executeForm(std::string_view, Form &, size_t, void *)
{
    return DialogResult::Error;
}

} // namespace CanForm

static bool executeItem(int, const EmscriptenMouseEvent *, void *userData)
{
    using namespace CanForm;
    ItemClick *itemClick = (ItemClick *)userData;
    if (itemClick == nullptr)
    {
        return true;
    }
    itemClick->execute();
    return true;
}

void ItemClick::removeElement()
{
    char buffer[256];
    std::snprintf(buffer, sizeof(buffer), "dialog_%d", id);
    EM_ASM(
        {
            let id = UTF8ToString($0);
            let e = document.getElementById(id);
            if (e)
            {
                e.remove();
            }
        },
        buffer);
}

void ItemClick::operator()(bool b)
{
    if (b)
    {
        removeElement();
    }
}

void ItemClick::operator()(CanForm::MenuItem::NewMenuPtr p)
{
    removeElement();
    p->second.show(p->first, nullptr);
}