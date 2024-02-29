#ifndef __PREPARE_DATA__
#define __PREPARE_DATA__

// preparing the data

#include "server1.h"

#define PubKey typename Types::PubKey
#define PlainKey typename Types::PlainKey

#define Plaintext typename Types::Plaintext
#define Ciphertext typename Types::Ciphertext

template<class Number, class Key>
void drawPermMat(Matrix<Number> &m, int d, const Key *pubKey) {
	m.resize(d, d);
#pragma omp parallel for collapse(2)
	for (unsigned int i = 0; i < m.cols(); ++i)
		for (unsigned int j = 0; j < m.cols(); ++j) {
			int r = (i == j) ? 1 : 0;
			m(i,j).from_int(r, pubKey);
		}
}

template<class Number, class Key>
void draw0(Matrix<Number> &m, const vector<long int> &d, const Key *pubKey) {
	m.resize(d.size(), d.size());
#pragma omp parallel for collapse(2)
	for (unsigned int i = 0; i < m.cols(); ++i)
		for (unsigned int j = 0; j < m.cols(); ++j) {
			m(i,j).from_int(0, pubKey);
		}
}


template<class Number, class Key>
void draw1(Matrix<Number> &m, const vector<long int> &d, const Key *pubKey) {
	m.resize(d.size(), d.size());
#pragma omp parallel for collapse(2)
	for (unsigned int i = 0; i < m.cols(); ++i)
		for (unsigned int j = 0; j < m.cols(); ++j) {
			int r = (i == j) ? 1 : 0;
			m(i,j).from_int(r, pubKey);
		}
}


// This file prepares A for SIR.
// In the beginning A is a left matrix.
// at the end of the protocol we have A := P2^T P1^T A P1 P2

// convention for the pseudo algortihm
// ML - a matrix encoded as left marix
// MR - a matrix encoded as right marix
// M - a packed matrix

// 1. Server1:
// 	R1 <- random mask
// 	P <- random perm

// 	PR := encode right matrix

// 	A := AL * PR
// 	A' := A + R1

// 	send A' to Server2

// 2. Server2:
// 	A'R := encode right matrix A
// 	Send A'R to server1

// 3. Server1: 
// 	P^TL := encode P^T as left matrix
// 	reencode R1 as a right matrix
// 	AR := A'R - R1R
// 	A := P^TL * AR

// 	R2 <- random mask
// 	A' := A + R2
// 	Send A' to Server2

// 4. Server2:
// 	A'L := reencode A' as left matrix
// 	send A'L to Server1

// 5. Server1:
// 	AL := A'L - R2

// 	R3 <- random mask
// 	R3R := encode as right matrix R3
// 	A' := AL * R3

// 	send A' to Server2


// 6. Server2:
// 	(PA') = P*A'
// 	(PA')L := encode as a left matrix
// 	send (PA')L to server1

// 7. Server1:
// 	R3^{-1}R := encode as right matrix R3^{-1}
// 	PA := (PA')L * (R3^{-1})R

// 	R4 <- draw random matrix
// 	PA' := PA + R3
// 	Send PA' to server2


// 8. Server2:
// 	(PA')R := encode PA' as right matrix
// 	send (PA')R to server1


// 9. Server1:
// 	R3R := encode R3 as a right matrix
// 	(PA)R := (PA')R - R3R

// 	R4 <- random mask
// 	R4L := encode R4 as left matrix
// 	(PA)' := R4L * (PA)R

// 	send (PA)' to server2


// 10. Server2:
// 	(PA)'P^T := (PA)' * P^T
// 	((PA)'P^T)R := encode (PA)'P^T as right matrix
// 	send ((PA)'P^T)R to Server1

// 11. Server1:
// 	R4^{-1}L := encode R4^{-1} as left matrix
// 	PAP^T := R4^{-1}L * ((PA)'P^T)R

// 	R5 <- random mask
// 	(PAP^T)' := PAP^T + R5
// 	send (PAP^T)' to Server2


// 12. Server2:
// 	(PAP^T)'L := encode (PAP^T)' as a left matrix
// 	send (PAP^T)'L to server1


// 13. Server1:
// 	R5L := encode R5 as a left matrix
// 	(PAP^T)L := (PAP^T)'L - R5L







// 1. Server1:
// 	R1 <- random mask
// 	P <- random perm
// 	PR := encode right matrix
// 	A := AL * PR
// 	A' := A + R1
// 	send A' to Server2
template<class Types>
void Server1<Types>::prepare_data() {
	// Times::start("prepare data step1");

	PermMat.resize(_feature_list.size(), _feature_list.size());
	R1.resize(_feature_list.size(), _feature_list.size());
	draw(R1, _feature_list, _plainKey);
	drawPermMat(PermMat, _feature_list.size(), _plainKey);

	PackedMatrixSet<Plaintext> PermMatRight;
	PermMatRight.init_right_matrix(PermMat, _plainKey, _plainKey);

	PackedMatrix<Ciphertext> AP;
	mul(AP, _A, PermMatRight, _pubKey);

	// cout << "step1 A= " << TO_STRING(_A) << endl;
	// cout << "step1 AP= " << TO_STRING(AP) << endl;

	PackedMatrix<Ciphertext> R1packed;
	R1packed.init_matrix(R1, _pubKey, _plainKey);
	AP += R1packed;
	// cout << "step1 AP= " << TO_STRING(AP) << endl;

	// Times::end("prepare data step1");
	_communication_channel->prepare_data_step2(AP);
}

// 2. Server2:
// 	A'R := encode right matrix A
// 	Send A'R to server1
template<class Types>
void Server2<Types>::prepare_data_step2(const PackedMatrix<Ciphertext> &AP) {
	// Times::start("prepare data step2");
	Matrix<Plaintext> APdecrypt;
	AP.to_matrix(APdecrypt, &plainKey, &privKey);

	// cout << "step2 AP= " << TO_STRING(AP) << endl;

	PackedMatrixSet<Ciphertext> APR;
	APR.init_right_matrix(APdecrypt, _pubKey, _plainKey);

	// Times::end("prepare data step2");
	_communication_channel->prepare_data_step3(APR);
}


// 3. Server1: 
// 	P^TL := encode P^T as left matrix
// 	reencode R1 as a right matrix
// 	AR := A'R - R1R
// 	A := P^TL * AR

// 	R2 <- random mask
// 	A' := A + R2
// 	Send A' to Server2
template<class Types>
void Server1<Types>::prepare_data_step3(PackedMatrixSet<Ciphertext> &APR) {
	// Times::start("prepare data step3");
	Matrix<Plaintext> PT = PermMat.transpose();

	PackedMatrixSet<Ciphertext> PTL;
	PTL.init_left_matrix(PT, _pubKey, _plainKey);

	PackedMatrixSet<Ciphertext> R1R;
	R1R.init_right_matrix(R1, _pubKey, _plainKey);

	APR -= R1R;

	PackedMatrix<Ciphertext> A;
	mul(A, PTL, APR, _pubKey);

	// cout << "step3 A1= " << TO_STRING(A) << endl;

	R2.resize(_feature_list.size(), _feature_list.size());
	draw(R2, _feature_list, _plainKey);
	PackedMatrix<Ciphertext> R2Enc;
	R2Enc.init_matrix(R2, _pubKey, _plainKey);
	A += R2Enc;

	// cout << "step3 A2= " << TO_STRING(A) << endl;

	// Times::end("prepare data step3");
	_communication_channel->prepare_data_step4(A);
}


// 4. Server2:
// 	A'L := reencode A' as left matrix
// 	send A'L to Server1
template<class Types>
void Server2<Types>::prepare_data_step4(PackedMatrix<Ciphertext> &AP) {
	// Times::start("prepare data step4");
	Matrix<Plaintext> APdecrypt;
	AP.to_matrix(APdecrypt, &plainKey, &privKey);

	PackedMatrixSet<Ciphertext> APL;
	APL.init_left_matrix(APdecrypt, _pubKey, _plainKey);
	
	// Times::end("prepare data step4");
	_communication_channel->prepare_data_step5(APL);
}

// 5. Server1:
// 	AL := A'L - R2

// 	R3 <- random mask
// 	R3R := encode as right matrix R3
// 	A' := AL * R3

// 	send A' to Server2
template<class Types>
void Server1<Types>::prepare_data_step5(PackedMatrixSet<Ciphertext> &APL) {
	// Times::start("prepare data step5");
	PackedMatrixSet<Ciphertext> R2L;
	R2L.init_left_matrix(R2, _pubKey, _plainKey);
	APL -= R2L;

	R3.resize(_feature_list.size(), _feature_list.size());
	draw(R3, _feature_list, _plainKey);
	PackedMatrixSet<Plaintext> R3R;
	R3R.init_right_matrix(R3, _plainKey, _plainKey);

	PackedMatrix<Ciphertext> AP;
	mul(AP, APL, R3R, _pubKey);

	// cout << "step5 AP= " << TO_STRING(AP) << endl;

	// Times::end("prepare data step5");
	_communication_channel->prepare_data_step6(AP);
}


// 6. Server2:
// 	(PA') = P*A'
// 	(PA')L := encode as a left matrix
// 	send (PA')L to server1
template<class Types>
void Server2<Types>::prepare_data_step6(PackedMatrix<Ciphertext> &A) {
	// Times::start("prepare data step6");
	PermMat2.resize(A.cols(), A.cols());
	drawPermMat(PermMat2, A.cols(), _plainKey);

	Matrix<Plaintext> Adecrypt;
	A.to_matrix(Adecrypt, &plainKey, &privKey);

	Matrix<Plaintext> PA;
	PA = PermMat2 * Adecrypt;

	// cout << "step6 PA= " << TO_STRING(PA) << endl;

	PackedMatrixSet<Ciphertext> PAL;
	PAL.init_left_matrix(PA, _pubKey, _plainKey);
	// cout << "step6 PAL= " << TO_STRING(PAL) << endl;
	
	// Times::end("prepare data step6");
	_communication_channel->prepare_data_step7(PAL);
}

// 7. Server1:
// 	R3^{-1}R := encode as right matrix R3^{-1}
// 	PA := (PA')L * (R3^{-1})R

// 	R4 <- draw random matrix
// 	PA' := PA + R4
// 	Send PA' to server2
template<class Types>
void Server1<Types>::prepare_data_step7(PackedMatrixSet<Ciphertext> &PAL) {
	// Times::start("prepare data step7");
	Plaintext R3det;
	Matrix<Plaintext> R3inv;
	
	R3.compute_adjugate_determinant(R3inv, R3det, *_plainKey);
	R3inv *= ::inverse(R3det);
	PackedMatrixSet<Plaintext> R3R;
	R3R.init_right_matrix(R3inv, _plainKey, _plainKey);

	PackedMatrix<Ciphertext> PA;
	mul(PA, PAL, R3R, _pubKey);

	// cout << "step7 PA= " << TO_STRING(PA) << endl;

	PackedMatrix<Ciphertext> R4Enc;
	R4.resize(_feature_list.size(), _feature_list.size());
	draw(R4, _feature_list, _plainKey);
	R4Enc.init_matrix(R4, _pubKey, _plainKey);

	PA += R4Enc;

	// Times::end("prepare data step7");
	_communication_channel->prepare_data_step8(PA);
}


// 8. Server2:
// 	(PA')R := encode PA' as right matrix
// 	send (PA')R to server1
template<class Types>
void Server2<Types>::prepare_data_step8(PackedMatrix<Ciphertext> &A) {
	// Times::start("prepare data step8");
	Matrix<Plaintext> Adecrypt;
	A.to_matrix(Adecrypt, &plainKey, &privKey);

	PackedMatrixSet<Ciphertext> AR;
	AR.init_right_matrix(Adecrypt, _pubKey, _plainKey);
	
	// Times::end("prepare data step8");
	_communication_channel->prepare_data_step9(AR);
}


// 9. Server1:
// 	R4R := encode R4 as a right matrix
// 	(PA)R := (PA')R - R4R

// 	R5 <- random mask
// 	R5L := encode R5 as left matrix
// 	(PA)' := R5L * (PA)R

// 	send (PA)' to server2
template<class Types>
void Server1<Types>::prepare_data_step9(PackedMatrixSet<Ciphertext> &PAPR) {
	// Times::start("prepare data step9");
	PackedMatrixSet<Ciphertext> R4R;
	R4R.init_right_matrix(R4, _pubKey, _plainKey);
	PAPR -= R4R;

	R5.resize(_feature_list.size(), _feature_list.size());
	draw(R5, _feature_list, _plainKey);
	PackedMatrixSet<Plaintext> R5L;
	R5L.init_left_matrix(R5, _plainKey, _plainKey);

	PackedMatrix<Ciphertext> PAP;
	mul(PAP, R5L, PAPR, _pubKey);
	// cout << "step9 PAP= " << TO_STRING(PAP) << endl;

	// Times::end("prepare data step9");
	_communication_channel->prepare_data_step10(PAP);
}


// 10. Server2:
// 	(PA)'P^T := (PA)' * P^T
// 	((PA)'P^T)R := encode (PA)'P^T as right matrix
// 	send ((PA)'P^T)R to Server1
template<class Types>
void Server2<Types>::prepare_data_step10(PackedMatrix<Ciphertext> &A) {
	// Times::start("prepare data step10");
	Matrix<Plaintext> PT = PermMat2.transpose();
	
	Matrix<Plaintext> Adecrypt;
	A.to_matrix(Adecrypt, &plainKey, &privKey);

	Matrix<Plaintext> PAP;
	PAP =  Adecrypt * PT;

	PackedMatrixSet<Ciphertext> PAPR;
	PAPR.init_right_matrix(PAP, _pubKey, _plainKey);
	
	// Times::end("prepare data step10");
	_communication_channel->prepare_data_step11(PAPR);
}

// 11. Server1:
// 	R5^{-1}L := encode R5^{-1} as left matrix
// 	PAP^T := R5^{-1}L * ((PA)'P^T)R

// 	R6 <- random mask
// 	(PAP^T)' := PAP^T + R6
// 	send (PAP^T)' to Server2
template<class Types>
void Server1<Types>::prepare_data_step11(PackedMatrixSet<Ciphertext> &PAPR) {
	// Times::start("prepare data step11");
	Plaintext R5det;
	Matrix<Plaintext> R5inv;
	R5.compute_adjugate_determinant(R5inv, R5det, *_plainKey);
	R5inv *= ::inverse(R5det);

	PackedMatrixSet<Ciphertext> R5invL;
	R5invL.init_left_matrix(R5inv, _pubKey, _plainKey);

	PackedMatrix<Ciphertext> PAP;
	mul(PAP, R5invL, PAPR, _pubKey);
	// cout << "step11 PAP= " << TO_STRING(PAP) << endl;

	PackedMatrix<Ciphertext> R6Enc;
	R6.resize(_feature_list.size(), _feature_list.size());
	draw(R6, _feature_list, _plainKey);
	R6Enc.init_matrix(R6, _pubKey, _plainKey);

	PAP += R6Enc;

	// Times::end("prepare data step11");
	_communication_channel->prepare_data_step12(PAP);
}


// 12. Server2:
// 	(PAP^T)'L := encode (PAP^T)' as a left matrix
// 	send (PAP^T)'L to server1
template<class Types>
void Server2<Types>::prepare_data_step12(PackedMatrix<Ciphertext> &A) {
	// Times::start("prepare data step12");
	Matrix<Plaintext> Adecrypt;
	A.to_matrix(Adecrypt, &plainKey, &privKey);

	PackedMatrixSet<Ciphertext> AL;
	AL.init_left_matrix(Adecrypt, _pubKey, _plainKey);
	
	// Times::end("prepare data step12");
	_communication_channel->prepare_data_step13(AL);
}


// 13. Server1:
// 	R6L := encode R5 as a left matrix
// 	(PAP^T)L := (PAP^T)'L - R6L
template<class Types>
void Server1<Types>::prepare_data_step13(PackedMatrixSet<Ciphertext> &PAPL) {
	// Times::start("prepare data step13");
	PackedMatrixSet<Ciphertext> R6L;
	R6L.init_left_matrix(R6, _pubKey, _plainKey);

	_A = PAPL;
	_A -= R6L;
	// cout << "step13 _A= " << TO_STRING(_A) << endl;
	// Times::end("prepare data step13");
}







#undef PubKey
#undef PlainKey

#undef Plaintext
#undef Ciphertext

#endif