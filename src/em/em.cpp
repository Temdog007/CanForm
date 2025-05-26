#include <canform.hpp>
#include <em/em.hpp>

namespace CanForm
{
DialogResult FileDialog::show(Handler &, void *) const
{
    return DialogResult::Error;
}
} // namespace CanForm