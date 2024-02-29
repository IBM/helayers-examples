#ifndef ___BINOMIAL_TOURNAMENT___
#define ___BINOMIAL_TOURNAMENT___

#include <utility>
#include <vector>

template<class Number>
class BinomialTournament {
private:
	typedef void UniteStrategy(Number &, const Number &);

	std::vector<Number *> _heap;

	void set_slot_empty(int i) { clean_level(i); }
	void set_slot_full(int i) {}

	void clean_level(int level) {
		if (_heap[level] != NULL) {
			delete _heap[level];
			_heap[level] = NULL;
		}
	}

	UniteStrategy *_unite_strategy;
public:
	BinomialTournament(UniteStrategy *unite_strategy) : _heap(0) { _unite_strategy = unite_strategy; }
	~BinomialTournament() {
		for (unsigned int i = 0; i < _heap.size(); ++i) {
			clean_level(i);
		}
	}
	void add_to_tournament(const Number &, unsigned int level = 0);
	Number unite_all(UniteStrategy *u = NULL) const;
	int levels() const;
	int max_level() const { return _heap.size(); }
	bool is_slot_empty(unsigned int i) const { return (i >= _heap.size()) || (_heap[i] == NULL) ; }
	bool is_empty() const { return levels() == 0; }

	void set_number_in_level(int level, Number *n) { clean_level(level);  _heap[level] = n; }
	Number &number(int i) { return *_heap[i]; }
	const Number &number(int i) const { return *_heap[i]; }

	static void add(Number &x, const Number &y) { x += y; }
	static void mul(Number &x, const Number &y) { x *= y; }
};

template<class Number>
class AddBinomialTournament : public BinomialTournament<Number> {
public:
	AddBinomialTournament() : BinomialTournament<Number>(BinomialTournament<Number>::add) {}
};

template<class Number>
class MulBinomialTournament : public BinomialTournament<Number> {
public:
	MulBinomialTournament() : BinomialTournament<Number>(BinomialTournament<Number>::mul) {}
};

template<class Number>
int BinomialTournament<Number>::levels() const {
	int count = 0;
	for (unsigned int i = 0; i < _heap.size(); ++i)
		if (!is_slot_empty(i))
			++count;
	return count;
}

template<class Number>
void BinomialTournament<Number>::add_to_tournament(const Number &n, unsigned int level) {
	Number *toAdd = new Number(n);

	while (1) {
		if (_heap.size() <= level) {
			_heap.resize(level + 1);
		}

		if (is_slot_empty(level)) {
			set_number_in_level(level, toAdd);
			set_slot_full(level);
			return;
		} else {
			_unite_strategy(*toAdd, number(level));
			set_slot_empty(level);
			++level;
		}
	}
}

template<class Number>
Number BinomialTournament<Number>::unite_all(UniteStrategy *unite_strategy) const {
	assert(!is_empty());
	if (unite_strategy == NULL)
		unite_strategy = _unite_strategy;

	Number n;
	unsigned int i = 0;
	while ((i < _heap.size()) && is_slot_empty(i))
		++i;

	if (i < _heap.size())
		n = number(i);
	++i;

	while (i < _heap.size()) {
		if (!is_slot_empty(i)) {
			unite_strategy(n, number(i));
		}
		++i;
	}

	return n;
}

#endif
