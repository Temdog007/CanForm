#include <em/em.hpp>
#include <form.hpp>

namespace CanForm
{
struct FormVisitor
{
    std::shared_ptr<FormExecute> formExecute;
    std::string_view name;
    size_t columns;
    int dialogId;

    FormVisitor(const std::shared_ptr<FormExecute> &f, size_t c) : formExecute(f), name(), columns(c), dialogId(0)
    {
    }

    int makeDiv()
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
                div.append(h2);
            },
            id, name.data(), name.size());
        return id;
    }

    int operator()(bool &b)
    {
        const int id = makeDiv();
        EM_ASM(
            {
                let id = $0;
                let value = $1;

                let div = document.getElementById('div_' + id.toString());

                let input = document.createElement('input');
                input.type = 'checkbox';
                input.checked = value ? true : false;
                div.append(input);
            },
            id, b);
        return id;
    }

    int operator()(StringMap &map)
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
                    div.append(input);

                    div.append(document.createElement("br"));
                },
                id, name.c_str(), flag, index++);
        }
        return id;
    }

    int operator()(String &s)
    {
        const int id = makeDiv();
        EM_ASM(
            {
                let id = $0;
                let value = UTF8ToString($1);

                let div = document.getElementById('div_' + id.toString());

                let input = document.createElement('input');
                input.type = 'text';
                input.value = value;
                div.append(input);
            },
            id, s.c_str());
        return id;
    }

    int operator()(ComplexString &s)
    {
        const int id = operator()(s.string);
        return id;
    }

    template <typename T> int operator()(Range<T> &range)
    {
        auto [min, max] = range.getMinMax();
        const int id = makeDiv();
        EM_ASM(
            {
                let id = $0;
                let value = $1;
                let min = $2;
                let max = $3;

                let div = document.getElementById('div_' + id.toString());

                let input = document.createElement('input');
                input.type = 'number';
                input.value = value;
                input.min = min.toString();
                input.max = max.toString();
                div.append(input);
            },
            id, *range, (double)min, (double)max);
        return id;
    }

    int operator()(RangedValue &n)
    {
        return std::visit(*this, n);
    }

    int operator()(StringList &list)
    {
        const int id = makeDiv();
        for (auto &[name, _] : list)
        {
            EM_ASM(
                {
                    let id = $0;
                    let name = UTF8ToString($1);

                    let div = document.getElementById('div_' + id.toString());

                    let button = document.createElement("button");
                    button.innerText = name;
                    div.append(button);

                    div.append(document.createElement("br"));
                },
                id, name.c_str());
        }
        return id;
    }

    int operator()(StringSelection &selection)
    {
        const int id = makeDiv();
        EM_ASM(
            {
                let id = $0;

                let div = document.getElementById('div_' + id.toString());

                let select = document.createElement("select");
                select.id = 'select_' + id.toString();
                div.append(select);
            },
            id);
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

    int operator()(MultiForm &multi)
    {
        const int id = makeDiv();
        EM_ASM(
            {
                let id = $0;

                let div = document.getElementById('div_' + id);

                let select = document.createElement("select");
                select.id = 'select_' + id.toString();
                select.onchange = function()
                {
                    for (let child of div.getElementsByClassName("tabContent"))
                    {
                        child.classList.remove("active");
                    }
                    let t = document.getElementById(select.value);
                    t.classList.add("active");
                };
                div.append(select);
            },
            id);
        for (auto &[n, form] : multi.tabs)
        {
            name = n;
            const int inner = writeForm(form);
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

                    let div2 = document.getElementById('form_' + inner);
                    div2.remove();
                    outer.append(div2);

                    if (selected)
                    {
                        select.value = title;
                        outer.classList.add('active');
                    }
                },
                id, inner, n.c_str(), multi.selected == n);
        }
        return id;
    }

    int writeForm(Form &form)
    {
        const int id = rand();
        EM_ASM(
            {
                let id = $0;
                let columns = $1;

                let div = document.createElement("div");
                div.id = 'form_' + id.toString();
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
            id, columns);
        for (auto &[n, data] : form.datas)
        {
            name = n;
            const int i = std::visit(*this, *data);
            EM_ASM(
                {
                    let parent = $0;
                    let child = $1;

                    let div = document.getElementById('form_' + parent);
                    let div2 = document.getElementById('div_' + child);

                    div2.remove();
                    div.append(div2);
                },
                id, i);
        }
        return id;
    }

    void checkLater()
    {
        emscripten_set_timeout(&FormVisitor::checkForResponse, 10, this);
    }

    static void checkForResponse(void *userData)
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
};

void FormExecute::execute(std::string_view title, const std::shared_ptr<FormExecute> &formExecute, size_t columns,
                          void *)
{
    FormVisitor *visitor = new FormVisitor(formExecute, columns);
    const int id = visitor->writeForm(formExecute->form);
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

            let content = document.getElementById("form_" + id.toString());
            content.remove();
            dialog.append(content);

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