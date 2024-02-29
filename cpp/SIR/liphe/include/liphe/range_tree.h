// Author: Hayim Shaul 2021
//
// Implementing a 1D range tree to support queries such as \prod_{i=A}^B x_i, for any pair A<B
//

#ifndef ___RANGE_TREE___
#define ___RANGE_TREE___

#include <vector>
#include <memory>
#include <stdexcept>
#include <algorithm>

// get a vector: x_0, ..., x_{n-1} and build a data-structure that supports getting \prod_a^b x_i for any 0 <= i,j <= n-1
// More generally, the  
template<class Number>
class RangeTree {
public:
	typedef Number UniteStrategy(const Number &, const Number &);
private:
    typedef std::pair<int,int> Range;

    std::vector<Number> _binTreeLeaves;
    std::vector<Range> _range;
    std::vector<Number> _binTree;

    int root() const { return 0; }
    int leftChild(int i) const { return 2*i + 1; }
    int rightChild(int i) const { return 2*i + 2; }
    int parent(int i) const { return (i - 1) / 2; }
    bool isLeaf(int i) const { return i >= _binTree.size(); }
    bool valid(int i) const { return _range[i] != Range(-1, -1); }
    Number &node(int i) { return (i < _binTree.size()) ? _binTree[i] : _binTreeLeaves[i - _binTree.size()]; }
    const Number &node(int i) const { return (i < _binTree.size()) ? _binTree[i] : _binTreeLeaves[i - _binTree.size()]; }

    bool rangeContains(const Range &container, const Range &contained) const {
        return (container.first <= contained.first) && (container.second >= contained.second);
    }

    bool rangeIntersect(const Range &a, const Range &b) const {
        return (a.first <= b.second) && (b.first <= a.second);
    }

    UniteStrategy *_uniteStrategy;
public:
    /// @param size Number of ctxts we want to compute all pair of
    RangeTree(UniteStrategy *s, int capacity);

    void add(Number &a);
    void build();

    Number getRange(int start, int end, int n = 0) const;
};

template<class Number>
RangeTree<Number>::RangeTree(RangeTree<Number>::UniteStrategy *s, int capacity) : _uniteStrategy(s) {
    int c = 1;
    while (c < capacity)
        c *= 2;

    _binTreeLeaves.reserve(capacity);
    _binTree.resize(c-1);
    }

template<class Number>
void RangeTree<Number>::add(Number &n) {
    if (_binTreeLeaves.size() >= _binTreeLeaves.capacity())
        throw std::runtime_error("AllPairs: Trying to insert more nodes than the capacity.");
    _binTreeLeaves.push_back(n);
}

template<class Number>
void RangeTree<Number>::build() {
    _range.resize(_binTree.size() + 2*_binTreeLeaves.size());
    std::fill(_range.begin(), _range.end(), Range(-1, -1));

    for (size_t i = 0; i < _binTreeLeaves.size(); ++i)
        _range[_binTree.size() + i] = Range(i, i);

    for (int i = (int)_binTree.size() - 1; i >= 0; --i) {
        int l = leftChild(i);
        int r = rightChild(i);
        if (valid(l) && valid(r)) {
            node(i) = _uniteStrategy(node(l), node(r));
            _range[i] = Range(_range[l].first, _range[r].second);
        } else if (valid(l)) {
            node(i) = node(l);
            _range[i] = Range(_range[l].first, _range[l].second);
        }
    }
}

template<class Number>
Number RangeTree<Number>::getRange(int start, int end, int n) const {
    if (start > end)
        throw std::runtime_error("RangeTree.getRange: start must be smaller/equal to end");
    Range range(start, end);
    // if the entire node is in the range, just return its value
    if (rangeContains(range, _range[n]))
        return node(n);
    int l = leftChild(n);
    int r = rightChild(n);
    if ((rangeIntersect(range, _range[l])) && (rangeIntersect(range, _range[r])))
        return _uniteStrategy(getRange(start, end, l), getRange(start, end, r));
    if (rangeIntersect(range, _range[l]))
        return getRange(start, end, l);
    if (rangeIntersect(range, _range[r]))
        return getRange(start, end, r);
    throw std::runtime_error("RangeTree: a node that does not overlap the requested range was reached.");
    return node(0);
}


#if 0

#include <iostream>

int add(const int &a, const int &b) { return a+b; }
int addAll(int a, int b) {
    int sum = 0;
    for (int i = a; i <= b; ++i)
        sum += i;
    return sum;
}

int main(int, char **) {
    const int MAX = 2000;

    RangeTree<int> RangeTree(add, MAX);

    for (int i = 0; i < MAX; ++i)
        RangeTree.add(i);
    RangeTree.build();

    for (int i = 0; i < MAX; ++i) {
        for (int j = i; j < MAX; ++j) {
            int a = addAll(i, j);
            int b = RangeTree.getRange(i, j);
            if (a != b) {
                std::cout << "Summing [" << i << ", " << j << "] gives wrong result " << b << " should be " << a << std::endl;
            }
        }
    }
}
#endif

#endif