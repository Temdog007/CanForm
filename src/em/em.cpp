#include <em/em.hpp>
#include <filesystem>

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

            let div = document.createElement("div");
            div.style.overflow = 'auto';
            div.style.maxHeight = '75vh';
            dialog.append(div);

            let p = document.createElement("p");
            p.innerText = UTF8ToString($3, $4);
            div.append(p);

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

void MenuItemClick::operator()(MenuState state)
{
    switch (state)
    {
    case MenuState::Close:
        removeDialog();
        break;
    default:
        break;
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
            return dialog.returnValue == "yes" ? 1 : 0;
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

            let hr = document.createElement("hr");
            dialog.append(hr);

            let button = document.createElement("button");
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

void MenuList::show(std::string_view title, const std::shared_ptr<MenuList> &menuList, void *ptr)
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

            let searchDiv = document.createElement('div');
            searchDiv.classList.add('search');
            dialog.append(searchDiv);

            let label = document.createElement('label');
            label.innerText = '🔍';
            label.style.display = '0vh 1vw';
            searchDiv.append(label);

            let search = document.createElement('input');
            search.placeholder = 'Search...';
            search.type = 'text';
            search.onchange = function()
            {
                for (let button of dialog.getElementsByClassName('menuButton'))
                {
                    button.style.display =
                        (search.value.length == 0 || button.innerText.includes(search.value)) ? 'initial' : 'none';
                }
            };
            search.onkeypress = function()
            {
                this.onchange();
            };
            search.onpaste = function()
            {
                this.onchange();
            };
            search.oninput = function()
            {
                this.onchange();
            };
            searchDiv.append(search);

            dialog.onkeypress = function(e)
            {
                if (e.shiftKey && e.key == 'F')
                {
                    searchDiv.classList.toggle('searching');
                    if (searchDiv.classList.contains('searching'))
                    {
                        search.focus();
                    }
                    e.preventDefault();
                }
            };

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
    bool first = true;
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
                    for (let child of dialog.getElementsByClassName("tabContent"))
                    {
                        child.classList.remove("active");
                    }
                    for (let child of dialog.getElementsByClassName("tabButton"))
                    {
                        child.classList.remove("active");
                    }
                    tabButton.classList.add("active");
                    tabContent.classList.add("active");
                };
                tabs.append(tabButton);

                let first = $2;
                if (first)
                {
                    tabButton.click();
                }
            },
            handler->id, menu.title.c_str(), first);
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
                    button.classList.add('menuButton');
                    button.innerText = buttonid;
                    div.append(button);

                    dialog.append(div);

                    return stringToNewUTF8('#' + button.id);
                },
                handler->id, menu.title.c_str(), item->label.c_str(), rand());
            auto &itemClick = handler->itemClicks.emplace_back(handler->id, item.get(), ptr);
            emscripten_set_click_callback(buttonQuery, &itemClick, false, executeItem);
            free(buttonQuery);
        }
        first = false;
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

template <typename T> struct Keeper
{
    std::shared_ptr<T> ptr;
    String string;

    void checkLater()
    {
        emscripten_set_timeout(&Keeper::checkIfElementWasRemoved, 10, this);
    }

    static void checkIfElementWasRemoved(void *userData)
    {
        Keeper *keeper = (Keeper *)userData;
        const bool found = EM_ASM_INT(
            {
                let id = UTF8ToString($0);
                let e = document.getElementById(id);
                return e ? true : false;
            },
            keeper->string.c_str());
        if (found)
        {
            keeper->checkLater();
        }
        else
        {
            delete keeper;
        }
    }
};

template <typename T> Keeper<T> *createKeeper(const std::shared_ptr<T> &ptr, const String &string)
{
    Keeper<T> *keeper = new Keeper<T>();
    keeper->ptr = ptr;
    keeper->string = string;
    keeper->checkLater();
    return keeper;
}

void FileDialog::show(const std::shared_ptr<Handler> &handler, void *) const
{
    const int id = rand();
    std::pmr::map<String, int> ids;
    const auto getId = [id, &ids](const auto &key) -> int {
        auto iter = ids.find(key);
        if (iter == ids.end())
        {
            return id;
        }
        else
        {
            return iter->second;
        }
    };
    ids.emplace(startDirectory, id);
    EM_ASM(
        {
            let id = $0;

            let dialog = document.createElement("dialog");
            dialog.id = 'dialog_' + id.toString();

            let h1 = document.createElement("h1");
            h1.innerText = UTF8ToString($1, $2);
            dialog.append(h1);

            dialog.append(document.createElement("hr"));

            let h2 = document.createElement("h2");
            h2.innerText = UTF8ToString($3, $4);
            dialog.append(h2);

            let saving = $5;
            let input = document.createElement("input");
            if (saving)
            {
                input.type = "text";
                input.value = UTF8ToString($6, $7);
                dialog.append(input);
            }

            let ul = document.createElement("ul");
            ul.style.overflow = 'auto';
            ul.style.maxHeight = '50vh';
            ul.classList.add("directory");
            ul.id = 'ul_' + id.toString();
            dialog.append(ul);

            dialog.append(document.createElement("hr"));

            let addr = $8;

            let button = document.createElement("button");
            button.innerText = '✖';
            button.id = "closeButton";
            button.onclick = function()
            {
                Module.ccall('cancelHandler', null, ['number'], [addr]);
                dialog.remove();
            };
            dialog.append(button);

            button = document.createElement("button");
            button.innerText = "OK";
            button.onclick = function()
            {
                let func = Module.cwrap('updateHandler', 'number', [ 'number', 'number' ]);
                for (let item of ul.getElementsByClassName("active"))
                {
                    let value = item.getAttribute("path");
                    if (saving && input.value.length > 0)
                    {
                        value += '/' + input.value;
                    }
                    if (!func(addr, stringToNewUTF8(value)))
                    {
                        break;
                    }
                }
                dialog.remove();
            };
            dialog.append(button);

            document.body.append(dialog);
            dialog.showModal();
        },
        id, title.data(), title.size(), message.data(), message.size(), saving, filename.data(), filename.size(),
        handler.get());
    const std::filesystem::path path(startDirectory);
    for (const auto &entry : std::filesystem::recursive_directory_iterator(path))
    {
        const auto path = entry.path();

        const int id = rand();
        ids.emplace(path.string(), id);

        const int parent = getId(path.parent_path().c_str());
        const auto filename = path.filename();

        EM_ASM(
            {
                let id = $0;
                let parent = $1;
                let path = UTF8ToString($2);
                let name = UTF8ToString($3);
                let directory = $4;
                let highlight = $5;

                let ul = document.getElementById("ul_" + parent.toString());

                let li = document.createElement("li");
                li.innerText = name;
                li.setAttribute("path", path);
                li.classList.add("clickable");
                if (highlight)
                {
                    li.onclick = function()
                    {
                        li.classList.toggle("active");
                    };
                }
                ul.append(li);

                if (directory)
                {
                    li.classList.add("directoryEntry");
                    let child = document.createElement("ul");
                    child.id = 'ul_' + id;
                    child.classList.add("directory");
                    ul.append(child);
                }
            },
            id, parent, path.c_str(), filename.c_str(), entry.is_directory(),
            saving || directories == entry.is_directory());
    }
    char buffer[256];
    std::snprintf(buffer, sizeof(buffer), "dialog_%d", id);
    createKeeper(handler, buffer);
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

bool openURL(std::string url)
{
    EM_ASM(
        {
            let url = UTF8ToString($0);
            window.open(url, "_blank");
        },
        url.c_str());
    return true;
}
} // namespace CanForm
