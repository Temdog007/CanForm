#include <em/em.hpp>

namespace CanForm
{
class FormVisitor
{
  private:
    std::string_view name;
    size_t columns;

  public:
    constexpr FormVisitor(size_t c) noexcept : name(), columns(c)
    {
    }

    void operator()(bool &)
    {
    }
    void operator()(String &)
    {
    }
    void operator()(Number &)
    {
    }
};

void FormExecute::execute(std::string_view, const std::shared_ptr<FormExecute> &, size_t, void *)
{
}
} // namespace CanForm