#pragma once

#include <em/em.hpp>
#include <form.hpp>

namespace CanForm
{
class FormVisitor
{
  private:
    std::shared_ptr<FormExecute> formExecute;
    std::string_view name;
    int dialogId;

    int makeDiv();

    void checkLater();
    static void checkForResponse(void *userData);

    friend struct FormExecute;

  public:
    FormVisitor(const std::shared_ptr<FormExecute> &f) : formExecute(f), name(), dialogId(0)
    {
    }

    int operator()(std::monostate &);
    int operator()(bool &);

    int operator()(String &);
    int operator()(ComplexString &);

    template <typename T> int operator()(Range<T> &);
    int operator()(RangedValue &);

    int operator()(StringSet &);
    int operator()(StringSelection &);
    int operator()(StringMap &);

    int operator()(VariantForm &);
    int operator()(StructForm &);
    int operator()(EnableForm &);
};

template <typename T> int FormVisitor::operator()(Range<T> &range)
{
    auto [min, max] = range.getMinMax();
    const int id = makeDiv();
    EM_ASM(
        {
            let id = $0;
            let value = $1;
            let min = $2;
            let max = $3;
            let r = $4;

            let div = document.getElementById('div_' + id.toString());

            let input = document.createElement('input');
            input.type = 'number';
            input.value = value;
            input.min = min.toString();
            input.max = max.toString();
            input.onchange = function()
            {
                input.value =
                    Module.ccall('updateRange', 'number', [ 'number', 'number' ], [ r, parseFloat(input.value) ]);
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
        },
        id, *range, (double)min, (double)max, &range);
    return id;
}
} // namespace CanForm