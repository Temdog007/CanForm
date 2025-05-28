#include <form.hpp>

namespace CanForm
{
String randomString(size_t n)
{
    String s;
    randomString<String>(s, n);
    return s;
}

String randomString(size_t min, size_t max)
{
    return randomString((rand() % (max - min)) + min);
}

char randomCharacter()
{
    char c;
    do
    {
        c = rand() % 128;
    } while (!std::isalnum(c));
    return c;
}

TimePoint now()
{
    return std::chrono::system_clock::now();
}
} // namespace CanForm