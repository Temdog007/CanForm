#include <em/form_visitor.hpp>

namespace CanForm
{
int FormVisitor::makeDiv()
{
    const int id = rand();
    EM_ASM(
        {
            let id = $0;

            let div = document.createElement("div");
            div.style.border = '1px solid';
            div.style.padding = '1vh 1vw';
            div.id = 'div_' + id.toString();
            document.body.append(div);

            let h2 = document.createElement("h2");
            h2.innerText = UTF8ToString($1, $2);
            h2.classList.add("variableName");
            div.append(h2);
        },
        id, name.data(), name.size());
    return id;
}

int FormVisitor::operator()(std::monostate &)
{
    return makeDiv();
}

int FormVisitor::operator()(bool &b)
{
    const int id = makeDiv();
    EM_ASM(
        {
            let id = $0;
            let value = $1;
            let addr = $2;

            let div = document.getElementById('div_' + id.toString());

            let input = document.createElement('input');
            input.id = 'input_' + id.toString();
            input.type = 'checkbox';
            input.checked = value ? true : false;
            input.onchange = function()
            {
                Module.ccall('updateBoolean', null, [ 'number', 'boolean' ], [ addr, input.checked ]);
            };
            div.append(input);
        },
        id, b, &b);
    return id;
}

int FormVisitor::operator()(StringMap &map)
{
    const int id = makeDiv();
    int index = 0;
    for (auto &[name, flag] : map)
    {
        EM_ASM(
            {
                let id = $0;
                let name = UTF8ToString($1);
                let flag = $2;
                let index = $3;
                let addr = $4;

                let div = document.getElementById('div_' + id.toString());

                let newID = 'checkbox_' + id.toString() + index.toString();
                let label = document.createElement("label");
                label.innerText = name;
                label.setAttribute('for', newID);
                div.append(label);

                let input = document.createElement("input");
                input.id = newID;
                input.type = "checkbox";
                input.checked = flag;
                input.onchange = function()
                {
                    Module.ccall('updateBoolean', null, [ 'number', 'boolean' ], [ addr, input.checked ]);
                };
                div.append(input);

                div.append(document.createElement("br"));
            },
            id, name.c_str(), flag, index++, &flag);
    }
    return id;
}

int FormVisitor::operator()(String &s)
{
    const int id = makeDiv();
    EM_ASM(
        {
            let id = $0;
            let value = UTF8ToString($1);
            let addr = $2;

            let div = document.getElementById('div_' + id.toString());

            let textarea = document.createElement('textarea');
            textarea.type = 'text';
            textarea.id = 'input_' + id.toString();
            textarea.value = value;
            textarea.onchange = function()
            {
                Module.ccall('updateString', null, [ 'number', 'number' ], [ addr, stringToNewUTF8(textarea.value) ]);
            };
            textarea.onkeypress = function()
            {
                this.onchange();
            };
            textarea.onpaste = function()
            {
                this.onchange();
            };
            textarea.oninput = function()
            {
                this.onchange();
            };
            div.append(textarea);
        },
        id, s.c_str(), &s);
    return id;
}

int FormVisitor::operator()(ComplexString &s)
{
    const int id = operator()(s.string);
    for (auto &[name, set] : s.map)
    {
        EM_ASM(
            {
                let id = $0;
                let value = UTF8ToString($1);

                let div = document.getElementById('div_' + id);

                let h3 = document.createElement("h3");
                h3.innerText = value;
                div.append(h3);
            },
            id, name.c_str());
        for (auto &s : set)
        {
            EM_ASM(
                {
                    let id = $0;
                    let value = UTF8ToString($1);

                    let div = document.getElementById('div_' + id);

                    let button = document.createElement("button");
                    button.innerText = value;
                    button.onclick = function()
                    {
                        let i = document.getElementById('input_' + id);
                        i.value += value;
                        i.focus();
                        let e = new Event('change');
                        i.dispatchEvent(e);
                    };
                    div.append(button);
                },
                id, s.c_str());
        }
    }
    return id;
}

int FormVisitor::operator()(RangedValue &n)
{
    return std::visit(*this, n);
}

int FormVisitor::operator()(StringSet &set)
{
    const int id = makeDiv();
    EM_ASM(
        {
            let id = $0;
            let addr = $1;

            let div = document.getElementById('div_' + id.toString());
            let content = document.createElement("div");
            content.id = 'content_' + id.toString();
            content.style.display = 'grid';
            content.style.gridTemplateColumns = 'auto 1fr';
            div.append(content);

            let button = document.createElement("button");
            button.innerText = 'Add';
            button.onclick = function()
            {
                Module.ccall('addToStringSet', null, [ 'number', 'number' ], [ addr, 0 ]);
                Module.ccall('updateStringSetDiv', null, [ 'number', 'number' ], [ addr, id ]);
            };
            div.append(button);
        },
        id, &set);

    updateStringSetDiv(set, id);
    return id;
}

int FormVisitor::operator()(StringSelection &selection)
{
    const int id = makeDiv();
    EM_ASM(
        {
            let id = $0;
            let addr = $1;

            let div = document.getElementById('div_' + id.toString());

            let select = document.createElement("select");
            select.id = 'select_' + id.toString();
            select.onchange = function()
            {
                setValue(addr, parseInt(select.selectedIndex), 'i32');
            };
            div.append(select);
        },
        id, &selection.index);
    int i = selection.index;
    for (auto &item : selection.set)
    {
        EM_ASM(
            {
                let id = $0;
                let value = UTF8ToString($1);
                let selected = $2;

                let select = document.getElementById('select_' + id.toString());

                let option = document.createElement('option');
                option.value = value;
                option.innerText = value;
                select.append(option);
                if (selected)
                {
                    option.selected = true;
                }
            },
            id, item.c_str(), i == 0);
        --i;
    }
    return id;
}

int FormVisitor::operator()(VariantForm &variant)
{
    const int id = makeDiv();
    EM_ASM(
        {
            let id = $0;
            let addr = $1;

            let div = document.getElementById('div_' + id);

            let select = document.createElement("select");
            select.id = 'select_' + id.toString();
            select.onchange = function()
            {
                for (let child of div.getElementsByClassName("tabContent"))
                {
                    child.classList.remove("active");
                }
                let outer = document.getElementById(select.value);
                outer.classList.add("active");
                for (let option of select.children)
                {
                    if (option.selected)
                    {
                        Module.ccall('updateVariantForm', null, [ 'number', 'number' ],
                                     [ addr, stringToNewUTF8(option.innerText) ]);
                        break;
                    }
                }
            };
            div.append(select);
            setTimeout(
                function() { select.onchange(); }, 10);
        },
        id, &variant);
    name = std::string_view();
    for (auto &[n, form] : variant.map)
    {
        const int inner = std::visit(*this, *form);
        EM_ASM(
            {
                let id = $0;
                let inner = $1;
                let title = UTF8ToString($2);
                let selected = $3;

                let div = document.getElementById('div_' + id);

                let select = document.getElementById('select_' + id.toString());
                let option = document.createElement("option");
                option.innerText = title;
                select.append(option);

                let outer = document.createElement("div");
                outer.id = 'option_' + inner.toString();
                outer.classList.add("tabContent");
                div.append(outer);

                option.value = outer.id;

                let div2 = document.getElementById('div_' + inner);
                div2.remove();
                outer.append(div2);

                if (selected)
                {
                    select.value = title;
                    outer.classList.add('active');
                }
            },
            id, inner, n.c_str(), variant.selected == n);
    }
    return id;
}

int FormVisitor::operator()(StructForm &structForm)
{
    const String origName(name);
    const bool useExpander = !name.empty();
    const int id = rand();
    EM_ASM(
        {
            let id = $0;
            let columns = $1;

            let div = document.createElement("div");
            div.id = 'div_' + id.toString();
            div.style.display = 'grid';
            function repeat(c)
            {
                let s = '';
                for (let i = 0; i < c; ++i)
                {
                    s += 'auto ';
                }
                return s;
            };
            div.style.gridTemplateColumns = repeat(columns);
            document.body.append(div);
        },
        id, structForm.columns);
    for (auto &[n, form] : *structForm)
    {
        name = n;
        const int i = std::visit(*this, *form);
        EM_ASM(
            {
                let parent = $0;
                let child = $1;

                let div = document.getElementById('div_' + parent.toString());
                let div2 = document.getElementById('div_' + child.toString());

                div2.remove();
                div.append(div2);
            },
            id, i);
    }
    if (!useExpander)
    {
        return id;
    }
    const int divId = rand();
    EM_ASM(
        {
            let id = $0;
            let divId = $1;

            let form = document.getElementById("div_" + id.toString());
            form.classList.add("expandable");

            let button = document.createElement("button");
            button.innerText = UTF8ToString($2);
            button.onclick = function()
            {
                button.classList.toggle("active");
                form.classList.toggle("active");
            };
            button.classList.add("expander");

            let div = document.createElement("div");
            div.id = "div_" + divId.toString();
            form.parentNode.insertBefore(div, form);

            div.append(button);
            form.remove();
            div.append(form);
        },
        id, divId, origName.c_str());
    return divId;
}

int FormVisitor::operator()(EnableForm &enableForm)
{
    const String origName(name);
    const bool useExpander = !name.empty();
    const int id = rand();
    EM_ASM(
        {
            let id = $0;

            let div = document.createElement("div");
            div.id = 'div_' + id.toString();
            div.style.display = 'grid';
            div.style.gridTemplateColumns = '1fr 3fr';
            document.body.append(div);
        },
        id);
    name = std::string_view();
    for (auto &[n, pair] : *enableForm)
    {
        bool &enabled = pair.first;
        Form &form = pair.second;
        const int i = std::visit(*this, *form);
        EM_ASM(
            {
                let parent = $0;
                let child = $1;
                let title = UTF8ToString($2);
                let enabled = $3;
                let addr = $4;

                let div = document.getElementById('div_' + parent.toString());

                let div2 = document.getElementById('div_' + child.toString());
                if (!enabled)
                {
                    div2.style.visibility = 'hidden';
                }
                div2.remove();

                let boxDiv = document.createElement("div");
                div.append(boxDiv);
                div.append(div2);

                let newID = 'checkbox_' + parent.toString() + child.toString();
                let label = document.createElement("label");
                label.innerText = title;
                label.setAttribute("for", newID);
                boxDiv.append(label);

                let input = document.createElement("input");
                input.type = "checkbox";
                input.id = newID;
                input.checked = enabled ? true : false;
                input.onchange = function()
                {
                    div2.style.visibility = input.checked ? 'initial' : 'hidden';
                    Module.ccall('updateBoolean', null, [ 'number', 'boolean' ], [ addr, input.checked ]);
                };
                boxDiv.append(input);
            },
            id, i, n.c_str(), enabled, &enabled);
    }
    if (!useExpander)
    {
        return id;
    }
    const int divId = rand();
    EM_ASM(
        {
            let id = $0;
            let divId = $1;

            let form = document.getElementById("div_" + id.toString());
            form.classList.add("expandable");

            let button = document.createElement("button");
            button.innerText = UTF8ToString($2);
            button.onclick = function()
            {
                button.classList.toggle("active");
                form.classList.toggle("active");
            };
            button.classList.add("expander");

            let div = document.createElement("div");
            div.id = "div_" + divId.toString();
            form.parentNode.insertBefore(div, form);

            div.append(button);
            form.remove();
            div.append(form);
        },
        id, divId, origName.c_str());
    return divId;
}

void FormVisitor::checkLater()
{
    emscripten_set_timeout(&FormVisitor::checkForResponse, 10, this);
}

void FormVisitor::checkForResponse(void *userData)
{
    FormVisitor *handler = (FormVisitor *)userData;
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
            return dialog.returnValue == "ok" ? 1 : 0;
        },
        handler->dialogId);
    switch (response)
    {
    case 0:
        handler->formExecute->cancel();
        delete handler;
        break;
    case 1:
        handler->formExecute->ok();
        delete handler;
        break;
    default:
        handler->checkLater();
        break;
    }
}

void FormExecute::execute(std::string_view title, const std::shared_ptr<FormExecute> &formExecute, void *)
{
    FormVisitor *visitor = new FormVisitor(formExecute);
    const int id = std::visit(*visitor, *(formExecute->form));
    EM_ASM(
        {
            let id = $0;

            let dialog = document.createElement("dialog");
            document.body.append(dialog);
            dialog.id = 'dialog_' + id.toString();

            let h1 = document.createElement("h1");
            h1.innerText = UTF8ToString($1, $2);
            dialog.append(h1);

            dialog.append(document.createElement("hr"));

            let div = document.createElement("div");
            div.style.overflow = 'auto';
            div.style.maxHeight = '75vh';
            dialog.append(div);

            let content = document.getElementById("div_" + id.toString());
            content.remove();
            div.append(content);

            let button = document.createElement("button");
            button.innerText = '✖';
            button.id = "closeButton";
            button.onclick = function()
            {
                dialog.remove();
            };
            dialog.append(button);

            dialog.append(document.createElement("hr"));

            button = document.createElement("button");
            button.innerText = 'Ok';
            button.style['float'] = 'right';
            button.onclick = function()
            {
                dialog.close("ok");
            };
            dialog.append(button);

            dialog.showModal();
        },
        id, title.data(), title.size());
    visitor->dialogId = id;
    visitor->checkLater();
}

} // namespace CanForm

using namespace CanForm;

void updateBoolean(bool &oldValue, bool newValue)
{
    oldValue = newValue;
}

void updateString(String &oldValue, char *newValue)
{
    oldValue.assign(newValue);
    free(newValue);
}

double updateRange(IRange &range, double d)
{
    return range.setFromDouble(d);
}

void updateVariantForm(VariantForm &variant, char *string)
{
    variant.selected.assign(string);
    free(string);
}

bool updateHandler(FileDialog::Handler &handler, char *string)
{
    bool result = handler.handle(string);
    free(string);
    return result;
}

void cancelHandler(FileDialog::Handler &handler)
{
    handler.canceled();
}

void addToStringSet(StringSet &set, char *string)
{
    if (string == nullptr)
    {
        set.emplace("");
    }
    else
    {
        set.emplace(string);
        free(string);
    }
}

void removeFromStringSet(StringSet &set, char *string)
{
    set.erase(string);
    free(string);
}

void updateStringSetDiv(StringSet &set, int id)
{
    EM_ASM(
        {
            let id = $0;
            let div = document.getElementById('content_' + id.toString());
            div.innerHTML = "";
        },
        id);
    for (auto &s : set)
    {
        EM_ASM(
            {
                let id = $0;
                let string = UTF8ToString($1);
                let addr = $2;

                let div = document.getElementById('content_' + id.toString());

                let button = document.createElement("button");
                button.innerText = '✖';
                div.append(button);

                let input = document.createElement("input");
                input.value = string;
                input.onchange = function()
                {
                    Module.ccall('removeFromStringSet', null, [ 'number', 'number' ],
                                 [ addr, stringToNewUTF8(string) ]);
                    Module.ccall('addToStringSet', null, [ 'number', 'number' ],
                                 [ addr, stringToNewUTF8(input.value) ]);
                    string = input.value;
                };
                input.onkeypress = function()
                {
                    this.onchange();
                };
                input.onpaste = function()
                {
                    this.onchange();
                };
                input.oninput = function()
                {
                    this.onchange();
                };
                div.append(input);

                button.onclick = function()
                {
                    Module.ccall('removeFromStringSet', null, [ 'number', 'number' ],
                                 [ addr, stringToNewUTF8(input.value) ]);
                    button.remove();
                    input.remove();
                };
            },
            id, s.c_str(), &set);
    }
}