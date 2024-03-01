#ifndef __CSV_H__
#define __CSV_H__

#include <string>

inline unsigned int countLines(const std::string &fname) {
	std::ifstream inFile(fname); 

	if (!inFile)
		throw std::runtime_error(std::string("can't open file ") + fname);
//	std::string line;
//	int count = 0;
//	while (getline(inFile, line))
//		++count;
//
//	return count;
	return std::count(std::istreambuf_iterator<char>(inFile), std::istreambuf_iterator<char>(), '\n');
}

inline std::vector<float> parseCSVLine(std::string line){
	using namespace boost;

	std::vector<float> vec;

	// Tokenizes the input string
	tokenizer<escaped_list_separator<char> > tk(line, escaped_list_separator<char> ('\\', ',', '\"'));
	for (auto i = tk.begin();  i!=tk.end();  ++i)
		vec.push_back(std::stof(*i));

	return vec;
}

// read a csv file given as x1,x2,...,y 
void read_csv_file(const std::string &fname, Matrix<float> &m, std::vector<float> &v, int maxDim) {
	std::istream *in;

	if (fname == "-") {
		in = &(std::cin);
	} else {
		in = new std::ifstream(fname);
	}

	std::string line;
	std::vector<float> l;

	int nlines = countLines(fname);
	std::getline(*in, line);	// first line is headers
	std::getline(*in, line);
	l = parseCSVLine(line);
	unsigned int dim = l.size() - 1;
	unsigned int actualDim = dim;
	if ((maxDim != -1) && ((int)actualDim > maxDim))
		actualDim = maxDim;
	// There is an extra dimension to simulate the bias
	m.resize(actualDim+1, nlines-1);
	v.resize(nlines-1);
	int i_line = 0;
	while (l.size() > 0) {
		if (l.size() != dim+1) {
			throw std::runtime_error(std::string("Error while reading CSV file. Lines have different length"));
		}
		for (unsigned int col = 0; col < actualDim; ++col)
			m(col, i_line) = l[col];
		// simulate the bias as the (d+1)-st dimension
		m(actualDim, i_line) = 1;
		v[i_line] = l[dim];
		std::getline(*in, line);
		l = parseCSVLine(line);
		++i_line;
	}

	if (in != &(std::cin))
		delete in;
}



#endif
