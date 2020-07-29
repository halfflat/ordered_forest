#include <iostream>

#include "ordered_forest.h"

int main() {
    using of = ordered_forest<int>;

    of f;

    auto i = f.push_front(1);
    i = f.insert_after(i, 2);
    i = f.insert_after(i, 3);

    auto c = f.root_begin();
    auto j = f.push_child(++c, 21);
    j = f.insert_after(j, 22);

    for (auto n: f) {
        std::cout << n << '\n';
    }
}
