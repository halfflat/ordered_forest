#include <cstddef>
#include <memory>
#include <type_traits>
#include <vector>

#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include "ordered_forest.h"

template <typename T>
struct simple_allocator {
    using value_type = T;

    simple_allocator():
        n_alloc_(new std::size_t()),
        n_dealloc_(new std::size_t())
    {}

    simple_allocator(const simple_allocator&) noexcept = default;

    template <typename U>
    simple_allocator(const simple_allocator<U>& other) noexcept {
        n_alloc_ = other.n_alloc_;
        n_dealloc_ = other.n_dealloc_;
    }

    T* allocate(std::size_t n) {
        auto p = new T[n];
        ++*n_alloc_;
        return p;
    }

    void deallocate(T* p, std::size_t) {
        delete [] p;
        ++*n_dealloc_;
    }

    bool operator==(const simple_allocator& other) const {
        return other.n_alloc_ == n_alloc_ && other.n_dealloc_ == n_dealloc_;
    }

    bool operator!=(const simple_allocator& other) const {
        return !(*this==other);
    }

    using propagate_on_container_copy_assignment = std::false_type;
    using propagate_on_container_move_assignment = std::false_type;
    using propagate_on_container_swap = std::true_type;

    void reset_counts() {
        *n_alloc_ = 0;
        *n_dealloc_ = 0;
    }

    std::size_t n_alloc() const { return *n_alloc_; }
    std::size_t n_dealloc() const { return *n_dealloc_; }

    std::shared_ptr<std::size_t> n_alloc_, n_dealloc_;
};

TEST_CASE("empty") {
    ordered_forest<int> f1;
    CHECK(f1.size() == 0);
    CHECK(f1.begin() == f1.end());

    simple_allocator<int> alloc;
    REQUIRE(alloc.n_alloc() == 0u);
    REQUIRE(alloc.n_dealloc() == 0u);

    ordered_forest<int, simple_allocator<int>> f2(alloc);
    CHECK(alloc.n_alloc() == 0u);
    CHECK(alloc.n_dealloc() == 0u);
}

TEST_CASE("push") {
    simple_allocator<int> alloc;
    REQUIRE(alloc.n_alloc() == 0u);
    REQUIRE(alloc.n_dealloc() == 0u);

    {
        ordered_forest<int, simple_allocator<int>> f(alloc);

        f.push_front(3);
        auto i2 = f.push_front(2);
        f.push_child(i2, 5);
        f.push_child(i2, 4);
        f.push_front(1);

        REQUIRE(f.size() == 5);
        CHECK(alloc.n_alloc() == 10); // five nodes, five items.

        auto i = f.begin();
        REQUIRE((bool)i);
        CHECK(*i == 1);
        CHECK(!(bool)i.child());

        i = i.next();
        REQUIRE((bool)i);
        auto j = i.child();
        REQUIRE((bool)j);
        CHECK(*j == 4);
        j = j.next();
        REQUIRE((bool)j);
        CHECK(*j == 5);
        CHECK(!(bool)j.next());

        i = i.next();
        REQUIRE((bool)i);
        CHECK(*i == 3);
        CHECK(!(bool)i.child());
        CHECK(!(bool)i.next());
    }

    CHECK(alloc.n_alloc() == alloc.n_dealloc());
}

TEST_CASE("insert") {
    simple_allocator<int> alloc;
    REQUIRE(alloc.n_alloc() == 0u);
    REQUIRE(alloc.n_dealloc() == 0u);

    {
        ordered_forest<int, simple_allocator<int>> f(alloc);

        auto r = f.push_front(1);
        f.insert_after(r, 3);
        r = f.insert_after(r, 2);
        auto c = f.push_child(r, 4);
        f.insert_after(c, 6);
        f.insert_after(c, 5);

        REQUIRE(f.size() == 6);
        CHECK(alloc.n_alloc() == 12); // six nodes, six items.

        auto i = f.begin();
        REQUIRE((bool)i);
        CHECK(*i == 1);
        CHECK(!(bool)i.child());

        i = i.next();
        REQUIRE((bool)i);
        auto j = i.child();
        REQUIRE((bool)j);
        CHECK(*j == 4);
        j = j.next();
        REQUIRE((bool)j);
        CHECK(*j == 5);
        j = j.next();
        REQUIRE((bool)j);
        CHECK(*j == 6);
        CHECK(!(bool)j.next());

        i = i.next();
        REQUIRE((bool)i);
        CHECK(*i == 3);
        CHECK(!(bool)i.child());
        CHECK(!(bool)i.next());
    }

    CHECK(alloc.n_alloc() == alloc.n_dealloc());
}

TEST_CASE("initializer_list") {
    ordered_forest<int> f = {1, {2, {4, 5, 6}}, 3};
    CHECK(f.size() == 6);

    auto i = f.begin();
    REQUIRE((bool)i);
    CHECK(*i == 1);
    CHECK(!(bool)i.child());

    i = i.next();
    REQUIRE((bool)i);
    auto j = i.child();
    REQUIRE((bool)j);
    CHECK(*j == 4);
    j = j.next();
    REQUIRE((bool)j);
    CHECK(*j == 5);
    j = j.next();
    REQUIRE((bool)j);
    CHECK(*j == 6);
    CHECK(!(bool)j.next());

    i = i.next();
    REQUIRE((bool)i);
    CHECK(*i == 3);
    CHECK(!(bool)i.child());
    CHECK(!(bool)i.next());
}

TEST_CASE("iteration") {
    using ivector = std::vector<int>;

    ordered_forest<int> f = {{1, {2, 3}}, {4, {5, {6, {7}}, 8}}, 9};

    ivector pre{f.preorder_begin(), f.preorder_end()};
    CHECK(pre == ivector{1, 2, 3, 4, 5, 6, 7, 8, 9});

    ivector pre_default{f.begin(), f.end()};
    CHECK(pre_default == ivector{1, 2, 3, 4, 5, 6, 7, 8, 9});

    ivector pre_cdefault{f.cbegin(), f.cend()};
    CHECK(pre_cdefault == ivector{1, 2, 3, 4, 5, 6, 7, 8, 9});

    ivector root{f.root_begin(), f.root_end()};
    CHECK(root == ivector{1, 4, 9});

    ivector post{f.postorder_begin(), f.postorder_end()};
    CHECK(post == ivector{2, 3, 1, 5, 7, 6, 8, 4, 9});

    auto four = std::find(f.begin(), f.end(), 4);
    ivector child_four{f.child_begin(four), f.child_end(four)};
    CHECK(child_four == ivector{5, 6, 8});

    using preorder_iterator = ordered_forest<int>::preorder_iterator;
    using postorder_iterator = ordered_forest<int>::postorder_iterator;

    ivector pre_four{preorder_iterator(four), preorder_iterator(four.next())};
    CHECK(pre_four == ivector{4, 5, 6, 7, 8});

    auto seven = std::find(f.begin(), f.end(), 7);
    auto nine = std::find(f.begin(), f.end(), 9);
    ivector post_seven_nine{postorder_iterator(seven), postorder_iterator(nine)};
    CHECK(post_seven_nine == ivector{7, 6, 8, 4});
}

TEST_CASE("copy/move") {
    simple_allocator<int> alloc;
    REQUIRE(alloc.n_alloc() == 0u);
    REQUIRE(alloc.n_dealloc() == 0u);

    using ivector = std::vector<int>;
    using of = ordered_forest<int, simple_allocator<int>>;

    of f1(alloc);
    {
        of f({{1, {2, 3}}, {4, {5, {6, {7}}, 8}}, 9}, alloc);
        CHECK(alloc.n_alloc() == 18u);

        f1 = f;
        CHECK(alloc.n_alloc() == 36u);
        CHECK(!f.empty());

        of f2 = std::move(f);
        CHECK(alloc.n_alloc() == 36u);
        CHECK(f.empty());

        ivector elems2{f2.begin(), f2.end()};
        CHECK(elems2 == ivector{1, 2, 3, 4, 5, 6, 7, 8, 9});
    }

    CHECK(alloc.n_alloc() == 36u);
    CHECK(alloc.n_dealloc() == 18u);

    ivector elems1{f1.begin(), f1.end()};
    CHECK(elems1 == ivector{1, 2, 3, 4, 5, 6, 7, 8, 9});

    // With a different != allocator object, require move assignment
    // to perform copy.

    simple_allocator<int> other_alloc;
    REQUIRE(alloc!=other_alloc);
    REQUIRE(std::allocator_traits<simple_allocator<int>>::propagate_on_container_move_assignment::value==false);

    of f3(other_alloc);

    f3 = std::move(f1);
    CHECK(!f1.empty());

    CHECK(alloc.n_alloc() == 36u);
    CHECK(alloc.n_dealloc() == 18u);
    CHECK(other_alloc.n_alloc() == 18u);
    CHECK(other_alloc.n_dealloc() == 0u);
}
