#ifndef ___DATA_SOURCE___
#define ___DATA_SOURCE___

#define PubKey typename Types::PubKey
#define PlainKey typename Types::PlainKey
#define PubBinaryKey typename Types::PubBinaryKey
#define PlainBinaryKey typename Types::PlainBinaryKey

#define Plaintext typename Types::Plaintext
#define Ciphertext typename Types::Ciphertext
#define BinaryPlaintext typename Types::BinaryPlaintext
#define BinaryCiphertext typename Types::BinaryCiphertext

template<class Types>
class Communication;

int DATA_SOURCE_NUMBER = 10;

template<class Types>
class DataSource {
private:
	const PubKey *_pubKey;
	const PlainKey *_plainKey;
	Matrix<float> _X;
	std::vector<float> _y;

	std::vector<Plaintext> _w;

	Communication<Types> *_communication_channel;

	PackedMatrixSet<Ciphertext> *A_simd;
	PackedVector<Ciphertext> *b_simd;

	Plaintext encode_number(float f, int resolution) const;
public:
	DataSource() : _pubKey(NULL), _plainKey(NULL), A_simd(NULL), b_simd(NULL) {}

	void set_data(const Matrix<float> &m, std::vector<float> &v) { _X = m; _y = v; }

	void setCommunicationChannel(Communication<Types> *c) { _communication_channel = c; }
	void setPubKey(const PubKey *p) { _pubKey = p; }
	void setPlainKey(const PlainKey *p) { _plainKey = p; }

	void receive_w_from_server1(const std::vector<Plaintext> &v) { _w = v; }

	void encode_data(bool isDesignated, int resolution);
	void send_A_and_b_to_server1();

	const std::vector<Plaintext> &w() const { return _w; }

//	void decrypt(std::vector<Plaintext> &w);
};

#undef PubKey
#undef PlainKey
#undef PubBinaryKey
#undef PlainBinaryKey

#undef Plaintext
#undef Ciphertext
#undef BinaryPlaintext
#undef BinaryCiphertext

#endif
