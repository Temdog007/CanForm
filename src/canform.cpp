#include <form.hpp>

namespace CanForm
{
String randomString(size_t n)
{
    String s;
    while (s.size() < n)
    {
        char c;
        do
        {
            c = rand() % 128;
        } while (!std::isalnum(c));
        s.push_back(c);
    }
    return s;
}

String randomString(size_t min, size_t max)
{
    return randomString((rand() % (max - min)) + min);
}

} // namespace CanForm