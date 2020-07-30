#include <iostream>

#include "ordered_forest.h"

template <typename V, typename A>
std::ostream& operator<<(std::ostream& out, const ordered_forest<V, A>& f) {
    auto print_children = [&out](auto& self, auto n, std::string prefix) -> void {
        while (n) {
            out << prefix << *n << '\n';
            if (n.child()) self(self, n.child(), prefix+"  ");
            n = n.next();
        }
    };
    print_children(print_children, f.begin(), "");
    return out;
}

int main() {
    using of = ordered_forest<int>;

#if 0
    of f;

    auto i = f.push_front(1);
    i = f.insert_after(i, 2);
    i = f.insert_after(i, 3);

    auto c = f.root_begin();
    auto j = f.push_child(++c, 21);
    j = f.insert_after(j, 22);

    for (auto n: f) std::cout << n << '\n';

    ordered_forest<double> x(f);
    for (auto n: x) std::cout << n << '\n';
#endif

//    of g{{1, {2, {3, {4}}, 5}}};
    of g{1, {2, {{4, {6, 7}}, 5}}, 3};

    of h;
    h = std::move(g);
    std::cout << g;
    std::cout << h;
}
