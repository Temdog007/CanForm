#include <em/em.hpp>

namespace CanForm
{
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

void MenuItemClick::operator()(MenuItem::NewMenu &&p)
{
    removeDialog();
    MenuList::show(p.first, p.second);
}

void ResponseHandler::checkLater()
{
    emscripten_set_timeout(&ResponseHandler::checkForAnswerToQuestion, 10, this);
}

void ResponseHandler::checkForAnswerToQuestion(void *userData)
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

void MenuHandler::checkLater()
{
    emscripten_set_timeout(&MenuHandler::checkIfElementWasRemoved, 1000, this);
}

void MenuHandler::checkIfElementWasRemoved(void *userData)
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
    else
    {
        handler->checkLater();
    }
}

void MenuList::show(std::string_view title, const std::shared_ptr<MenuList> &menuList, void *)
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
            auto &itemClick = handler->itemClicks.emplace_back(handler->id, item.get());
            emscripten_set_click_callback(buttonQuery, &itemClick, false, executeItem);
            free(buttonQuery);
        }
    }
    handler->checkLater();
}

void AwaiterHandler::checkLater()
{
    emscripten_set_timeout(&AwaiterHandler::checkIfDone, 10, this);
}

void AwaiterHandler::checkIfDone(void *userData)
{
    AwaiterHandler *a = (AwaiterHandler *)userData;
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
        a->checkLater();
    }
}

void showPopupUntil(std::string_view message, const std::shared_ptr<Awaiter> &awaiter, size_t, void *)
{
    AwaiterHandler *a = new AwaiterHandler();
    a->awaiter = awaiter;
    a->id = rand();
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
        a->id, message.data(), message.size());
    a->checkLater();
}

void FileDialog::show(const std::shared_ptr<Handler> &, void *ptr) const
{
    showMessageBox(MessageBoxType::Error, "No File Dialog", "File Dialogs are not supported on this platform", ptr);
}

bool executeItem(int, const EmscriptenMouseEvent *, void *userData)
{
    MenuItemClick *itemClick = (MenuItemClick *)userData;
    if (itemClick == nullptr)
    {
        return true;
    }
    itemClick->execute();
    return true;
}

void removeElement(const char *id)
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
} // namespace CanForm
