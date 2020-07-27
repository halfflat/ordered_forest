#pragma once

#include <type_traits>
#include <iterator>
#include <memory>

template <typename V, typename Allocator = std::allocator<V>>
struct ordered_forest {
    using size_type = std::size_t;

    struct node {
        friend class ordered_forest;
        operator bool() const { return item_; }

    protected:
        // Each distinct node has a distinct item_ pointer.
        // A null item_ pointer indicates the invalid node value.

        V* item_ = nullptr;
        node* parent_ = nullptr;
        node* child_ = nullptr;
        node* left_ = nullptr;
        node* right_ = nullptr;
    };

    template <bool const_flag>
    struct iterator_mc: node {
        using pointer = std::conditional_t<const_flag, const V*, V*>;
        using reference = std::conditional_t<const_flag, const V&, V&>;
        using value_type = V;
        using difference_type = std::ptrdiff_t;
        using iterator_tag = std::forward_iterator_tag;

        iterator_mc() = default;
        iterator_mc(const node& n): node(n) {}

        iterator_mc parent() const { return parent_? *parent_: node{}; }
        iterator_mc next() const { return right_? *right_: node{}; }
        iterator_mc prev() const { return left_? *left_: node{}; }
        iterator_mc child() const { return child_? *child_: node{}; }

        bool operator==(const node& a) const { return item_ == a.item_; }
        bool operator!=(const node& a) const { return item_ != a.item_; }

        iterator_mc preorder_next() const {
            if (child_) return *child_;

            const node* x = this;
            while (x && !x->right_) x = x->parent_;
            return x? *x->right_: node{};
        }

        iterator_mc postorder_next() const {
            if (right_) {
                const node* x = *right_;
                while (const node* c = x->child_) x = c;
                return *x;
            }
            else return parent();
        }

        reference operator*() const { return *item_; }
        pointer operator->() const { return item_; }
    };

    template <bool const_flag>
    struct child_iterator_mc: iterator_mc<const_flag> {
        child_iterator_mc() = default;
        child_iterator_mc(const node& n): iterator_mc<const_flag>(n) {}

        child_iterator_mc& operator++() { return *this = next(); }
        child_iterator_mc operator++(int) { auto p = *this; return ++*this, p; }
    };

    using child_iterator = child_iterator_mc<false>;
    using const_child_iterator = child_iterator_mc<true>;

    template <bool const_flag>
    struct preorder_iterator_mc: iterator_mc<const_flag> {
        preorder_iterator_mc() = default;
        preorder_iterator_mc(const node& n): iterator_mc<const_flag>(n) {}

        preorder_iterator_mc& operator++() { return *this = preorder_next(); }
        preorder_iterator_mc operator++(int) { auto p = *this; return ++*this, p; }
    };

    using preorder_iterator = preorder_iterator_mc<false>;
    using const_preorder_iterator = preorder_iterator_mc<true>;

    template <bool const_flag>
    struct postorder_iterator_mc: iterator_mc<const_flag> {
        postorder_iterator_mc() = default;
        postorder_iterator_mc(const node& n): iterator_mc<const_flag>(n) {}

        postorder_iterator_mc& operator++() { return *this = postorder_next(); }
        postorder_iterator_mc operator++(int) { auto p = *this; return ++*this, p; }
    };

    using postorder_iterator = postorder_iterator_mc<false>;
    using const_postorder_iterator = postorder_iterator_mc<true>;

    ordered_forest(Allocator alloc = Allocator{}):
        item_alloc(std::move(alloc)),
        node_alloc(item_alloc),
    {}

    bool empty() const { return !first; }

    // Note: size is O(n) in the number of elements n.
    std::size_t size() const {
        std::size_t n = 0;
        for (const auto& _: *this) ++n;
        return n;
    }

    child_iterator child_begin(node n = first_or_empty()) { return child_iterator{n}; }
    const_child_iterator child_begin(node n = first_or_empty()) const { return const_child_iterator{n}; }

    child_iterator child_end(node = {}) { return {}; }
    const_child_iterator child_end(node = {}) const { return {}; }

    child_iterator child_begin(node n = first_or_empty()) { return child_iterator{n}; }
    const_child_iterator child_begin(node n = first_or_empty()) const { return const_child_iterator{n}; }

    child_iterator child_end(node = {}) { return {}; }
    const_child_iterator child_end(node = {}) const { return {}; }

    postorder_iterator postorder_begin(node n = first_or_empty()) { return postorder_iterator{n}; }
    const_postorder_iterator postorder_begin(node n = first_or_empty()) const { return const_postorder_iterator{n}; }

    postorder_iterator postorder_end(node = {}) { return {}; }
    const_postorder_iterator postorder_end(node = {}) const { return {}; }

    // Default iteration is preorder.

    using iterator = preorder_iterator;
    using const_iterator = const_preorder_iterator;

    iterator begin() { return preorder_begin(); }
    iterator end() { return {}; }

    const_iterator begin() const { return preorder_begin(); }
    const_iterator end() const { return {}; }

    const_iterator cbegin() const { return preorder_begin(); }
    const_iterator cend() const { return {}; }

    // Insertion and emplace operations:
    //
    // * All return an iterator to the last inserted node, or the an iterator to the referenced
    //   node if the collection of inserted items is empty.
    //
    // * The iterator argument may not be an end iterator.
    //
    // * Any iterator that compares equal to the iterator argument is invalidated.

    // Insert/emplace item as previous sibling.

    template <typename Iter, typename = std::enable_if_t<std::is_base_of<iterator_mc<false>, Iter>::value>>
    Iter insert(const Iter& n, const V& item) { return insert_impl(n, make_node(item)); }

    template <typename Iter, typename = std::enable_if_t<std::is_base_of<iterator_mc<false>, Iter>::value>>
    Iter insert(const Iter& n, V&& item) { return insert_impl(n, make_node(std::move(item))); }

    template <typename Iter, typename... Args, typename = std::enable_if_t<std::is_base_of<iterator_mc<false>, Iter>::value>>
    Iter emplace(const Iter& n, Args&&... args) { return insert_impl(n, make_node(std::forward<Args>(args)...)); }

    // Insert trees in forest as previous siblings.

    template <typename Iter, typename = std::enable_if_t<std::is_base_of<iterator_mc<false>, Iter>::value>>
    Iter graft(const Iter& n, ordered_forest of) { return insert_impl(n, std::move(of)); }

    // Insert/emplace item as next sibling.

    template <typename Iter, typename = std::enable_if_t<std::is_base_of<iterator_mc<false>, Iter>::value>>
    Iter insert_after(const Iter& n, const V& item) { return insert_after_impl(n, make_node(item)); }

    template <typename Iter, typename = std::enable_if_t<std::is_base_of<iterator_mc<false>, Iter>::value>>
    Iter insert_after(const Iter& n, V&& item) { return insert_after_impl(n, make_node(std::move(item))); }

    template <typename Iter, typename... Args, typename = std::enable_if_t<std::is_base_of<iterator_mc<false>, Iter>::value>>
    Iter emplace_after(const Iter& n, Args&&... args) { return insert_after_impl(n, make_node(std::forward<Args>(args)...)); }

    // Insert trees in forest as next siblings.

    template <typename Iter, typename = std::enable_if_t<std::is_base_of<iterator_mc<false>, Iter>::value>>
    Iter graft_after(const Iter& n, ordered_forest of) { return insert_after_impl(n, std::move(of)); }

    ~ordered_forest() {
        delete_node(first);
    }

private:
    using node_alloc_t = typename std::allocator_traits<Allocator>::rebind<node>;
    using item_alloc_traits = std::allocator_traits<Allocator>;
    using node_alloc_traits = std::allocator_traits<node_alloc_t>;

    Allocator item_alloc;
    node_alloc_t node_alloc;
    node* first = nullptr;

    // Make forest from pre-allocated node.
    explicit ordered_forest(node* n): first_(n) {}

    node first_or_empty() const { return first? *first: node{}; }

    node insert_impl(const node& n, node* x) {
        return insert_impl(n, ordered_forest(x));
    }

    node insert_impl(const node& n, ordered_forest&& of) {
        if (of.empty()) return n;
        if (!n.prev_ && !n.parent_) throw std::invalid_argument("bad iterator");

        node* of_first = of.first_;
        of.first_ = nullptr;

        node* of_last = nullptr;
        for (node* i = of_first; i; i = i->next_) {
            i->parent_ = n->parent_;
            of_last = i;
        }

        if (n.prev_) {
            of_first.prev_ = n.prev_;
            of_last.next_ = n.prev_->next_;
            n.prev_->next_ = of_first;
            return *of_last;
        }
        else {
            of_first.next_ = n.parent_->child_;
            n.parent_->child_ = of_first;
            return *of_last;
        }
    }

    template <typename... Args>
    node* make_node(Args&&... args) {
        node* x = alloc_node();
        x->item_ = item_alloc_traits::allocate(item_alloc, 1);
        try {
            item_alloc_traits::construct(item_alloc, x->item, std::forward<Args>(args)...);
        }
        catch (...) {
            item_alloc_trais::deallocate(item_alloc, x->item_, 1);
            throw;
        }
        return x;
    }

    void delete_node(node* n) {
        if (!n) return;

        delete_item(item);
        delete_node(child);
        delete_node(right);

        node_alloc_traits::destroy(node_alloc, n);
        node_alloc_traits::deallocate(node_alloc, n, 1);
    }

    void delete_item(V* item) {
        if (!item) return;

        item_alloc_traits::destroy(item_alloc, n);
        item_alloc_traits::deallocate(item_alloc, n, 1);
    }
};

