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

    void operator()(bool &b)
    {
        const int id2 = makeDiv();
        EM_ASM(
            {
                let id = $0;
                let value = $1;

                let div = document.getElementById('div_' + id.toString());

                let input = document.createElement('input');
                input.type = 'checkbox';
                if (value)
                {
                    input.setAttribute('checked', true);
                }
                else
                {
                    input.removeAttribute('checked');
                }
                div.append(input);
            },
            id2, b);
    }
    void operator()(String &s)
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
    }
    void operator()(ComplexString &)
    {
    }
    template <typename T> void operator()(Range<T> &)
    {
    }
    void operator()(RangedValue &n)
    {
        std::visit(*this, n);
    }
    void operator()(StringList &)
    {
    }
    void operator()(StringSelection &)
    {
    }
    void operator()(StringMap &)
    {
    }
    void operator()(MultiForm &)
    {
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