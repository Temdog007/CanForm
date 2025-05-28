#include <em/em.hpp>
#include <form.hpp>

namespace CanForm
{
class FormVisitor
{
  private:
    std::shared_ptr<FormExecute> formExecute;
    std::string_view name;
    int id;

    int makeDiv()
    {
        const int i = rand();
        EM_ASM(
            {
                let id = $0;
                let i = $1;

                let dialog = document.getElementById('form_' + id.toString());

                let div = document.createElement("div");
                div.style.border = '1px solid';
                div.style.padding = '1vh 1vw';
                div.id = 'div_' + i.toString();
                dialog.append(div);

                let h2 = document.createElement("h2");
                h2.innerText = UTF8ToString($2, $3);
                div.append(h2);
            },
            id, i, name.data(), name.size());
        return i;
    }

  public:
    FormVisitor(std::string_view title, const std::shared_ptr<FormExecute> &f, size_t columns)
        : formExecute(f), name(), id(rand())
    {
        EM_ASM(
            {
                let id = $0;
                let columns = $1;

                let dialog = document.createElement("dialog");
                document.body.append(dialog);
                dialog.id = 'dialog_' + id.toString();

                let h1 = document.createElement("h1");
                h1.innerText = UTF8ToString($2, $3);
                dialog.append(h1);

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
                dialog.append(div);

                let button = document.createElement("button");
                button.innerText = '✖';
                button.id = "closeButton";
                button.onclick = function()
                {
                    dialog.remove();
                };
                dialog.append(button);

                let hr = document.createElement("hr");
                dialog.append(hr);

                button = document.createElement("button");
                button.innerText = 'Ok';
                button.onclick = function()
                {
                    dialog.close("ok");
                };
                dialog.append(button);

                dialog.showModal();
            },
            id, columns, title.data(), title.size());
    }

    int operator()(bool &b)
    {
        const int id2 = makeDiv();
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
            id2, b);
        return id2;
    }

    int operator()(StringMap &map)
    {
        const int id2 = makeDiv();
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
                id2, name.c_str(), flag, index++);
        }
        return id2;
    }

    int operator()(String &s)
    {
        const int id2 = makeDiv();
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
            id2, s.c_str());
        return id2;
    }

    int operator()(ComplexString &s)
    {
        const int id2 = operator()(s.string);
        return id2;
    }

    template <typename T> int operator()(Range<T> &range)
    {
        auto [min, max] = range.getMinMax();
        const int id2 = makeDiv();
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
            id2, *range, (double)min, (double)max);
        return id2;
    }

    int operator()(RangedValue &n)
    {
        return std::visit(*this, n);
    }

    int operator()(StringList &)
    {
        return 0;
    }

    int operator()(StringSelection &selection)
    {
        const int id2 = makeDiv();
        EM_ASM(
            {
                let id = $0;

                let div = document.getElementById('div_' + id.toString());

                let select = document.createElement("select");
                select.id = 'select_' + id.toString();
                div.append(select);
            },
            id2);
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
                id2, item.c_str(), i == 0);
            --i;
        }
        return id2;
    }

    int operator()(MultiForm &)
    {
        return 0;
    }

    void operator()(Form &form)
    {
        for (auto &[n, data] : form.datas)
        {
            name = n;
            std::visit(*this, *data);
        }
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
            handler->id);
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
    FormVisitor *visitor = new FormVisitor(title, formExecute, columns);
    visitor->operator()(formExecute->form);
    visitor->checkLater();
}
} // namespace CanForm