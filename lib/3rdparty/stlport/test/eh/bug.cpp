#include <set>
#include <vector>
#include <iostream>
#include <boost/timer.hpp>
#include <boost/lexical_cast.hpp>

struct compare
{
    bool operator()(int* x, int* y)
        { return *x < *y; }

};

int main(int argc, char const* const argv[])
{
    std::size_t niters = argc < 2 ? 1000 : boost::lexical_cast<std::size_t>(argv[1]);

    boost::timer t;

    std::vector<int> v;
    for (int n = 0; n < niters; ++n)
    {
        v.insert(v.begin() + v.size()/2, n);
    }

    std::cout << "vector fill: " << t.elapsed() << std::endl;

    std::multiset<int*,compare> m;
    for (int n = 0; n < niters; ++n)
    {
        m.insert(&v[n]);
    }
    std::cout << "map fill 1: " << t.elapsed() << std::endl;
    for (int n = 0; n < niters; ++n)
    {
        m.insert(&v[n]);
    }
    std::cout << "map fill 2: " << t.elapsed() << std::endl;
}
