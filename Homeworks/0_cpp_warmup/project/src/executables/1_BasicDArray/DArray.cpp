// implementation of class DArray
#include <iostream>
#include "DArray.h"
#include <string>

// default constructor
DArray::DArray() {
	Init();
}

// set an array with default values
DArray::DArray(int nSize, double dValue) {
	//TODO
	Init();
	SetSize(nSize);
	for (int i = 0; i < nSize; i++) {
		SetAt(i, dValue);
	}
}

DArray::DArray(const DArray& arr) {
	//TODO
	Init();
	SetSize(arr.GetSize()); // copy the size
	for (int i = 0; i< arr.GetSize(); i++) {
		SetAt(i, arr.GetAt(i)); // copy the elements
	}
}

// deconstructor
DArray::~DArray() {
	Free();
}

// display the elements of the array
void DArray::Print() const {
	//TODO
	for (int i = 0; i < GetSize(); i++) {
		std::cout << GetAt(i) << " ";	
	}
}

// initilize the array
void DArray::Init() {
	//TODO
	m_pData = NULL;
	m_nSize = 0;
}

// free the array
void DArray::Free() {
	//TODO
	if (m_pData != NULL) {
		delete[] m_pData;
		m_pData = NULL;	
	}
}

// get the size of the array
int DArray::GetSize() const {
	//TODO
	return m_nSize; // you should return a correct value
}

// set the size of the array
void DArray::SetSize(int nSize) {
	//TODO
	if (nSize == m_nSize){
		return;
	}
	double* newData = new double[nSize];
	int copysize = (nSize < m_nSize) ? nSize : m_nSize;
	for (int i = 0; i < copysize; i++) {
		newData[i] = m_pData[i];
	}

	if (m_pData != NULL) {
		delete[] m_pData;
	}

	m_pData = newData;
	m_nSize = nSize;
}

// get an element at an index
const double& DArray::GetAt(int nIndex) const {
	//TODO
	static double ERROR; // you should delete this line
	return ERROR; // you should return a correct value
}

// set the value of an element 
void DArray::SetAt(int nIndex, double dValue) {
	//TODO
}

// overload operator '[]'
const double& DArray::operator[](int nIndex) const {
	//TODO
	static double ERROR; // you should delete this line
	return ERROR; // you should return a correct value
}

// add a new element at the end of the array
void DArray::PushBack(double dValue) {
	//TODO
}

// delete an element at some index
void DArray::DeleteAt(int nIndex) {
	//TODO
}

// insert a new element at some index
void DArray::InsertAt(int nIndex, double dValue) {
	//TODO
}

// overload operator '='
DArray& DArray::operator = (const DArray& arr) {
	//TODO
	return *this;
}
