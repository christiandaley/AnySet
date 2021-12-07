// ConsoleApplication1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

import any_set;

using utility::any_set;

struct test
{
    size_t x, y, z;

    test (size_t x, size_t y, size_t z) :
        x (x),
        y (y),
        z (z)
    {}

    test (const test&) = default;
    test (test&&) = default;

    test& operator= (const test&) = delete;
    test& operator= (test&&) = delete;

    ~test () = default;
};

int main()
{     
    auto set = any_set ();
    set.insert (2);
    set.insert (5.5);
    set.insert (true);
    set.emplace<test> (7, 2, 15);
    set.emplace<std::shared_ptr<int>> (std::make_shared<int> (9));
    set.emplace<std::vector<int>> ({ 0, 1, 2, 3, 4 });
    set.insert<int*> (nullptr);
    set.emplace<std::string> ("hello");

    if (auto a = set.get<int> ())
    {
        std::cout << *a << '\n';
    }

    if (auto b = set.get<double> ())
    {
        std::cout << *b << '\n';
    }

    if (auto c = set.get<std::string> ())
    {
        std::cout << *c << '\n';
    }

    auto set2 = any_set ();
    set2 = std::move (set);

    if (auto d = set2.get<test> ())
    {
        std::cout << d->x << ", " << d->y << ", " << d-> z << '\n';
    }

    try
    {
        auto& e = set2.get_ref<std::shared_ptr<int>> ();
        std::cout << *e << '\n';

        auto& f = set2.get_ref<std::vector<int>> ();
        std::cout << f.size () << '\n';
    }
    catch (const utility::type_not_found& err)
    {
        std::cout << err.what () << '\n';
    }

    if (auto g = set2.get<int*> ())
    {
        std::cout << *g << '\n';
    }
}
