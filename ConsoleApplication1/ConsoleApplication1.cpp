// ConsoleApplication1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

import AnySet;

using Utility::AnySet;

int main()
{     
    auto map = AnySet ();
    map.insert (2);
    map.insert (5.5);

    std::string hello = "hello";
    map.insert (std::move (hello)); // code compiles if this is commented out

    if (auto a = map.get<int> ())
    {
        std::cout << *a << '\n';
    }

    if (auto b = map.get<double> ())
    {
        std::cout << *b << '\n';
    }

    if (auto c = map.get<std::string> ())
    {
        std::cout << *c << '\n';
    }
}
