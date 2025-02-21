// implementation of class DArray
#include "DArray.h"
#include <iostream>
#include <assert.h>

#define ARRAY_MAX_ELEMENTS 15

// default constructor
DArray::DArray() {
	Init();
}

// set an array with default values
DArray::DArray(int nSize, double dValue) {
	Init();
	SetSize(nSize);
	for (int i = 0; i < nSize; i++)
		SetAt(i, dValue);
}

DArray::DArray(const DArray& arr) {
	//TODO
	Init();
	SetSize(arr.GetSize());
	for (int i = 0; i < GetSize(); i++)
		SetAt(i, arr.GetAt(i));
}

// deconstructor
DArray::~DArray() {
	Free();
}

// display the elements of the array
void DArray::Print() const {
	//TODO
	const int nSize = GetSize();
	for (int i = 0; i < nSize; i++)
		std::cout << GetAt(i) << " ";

}

// initialize the array
void DArray::Init() {
	//TODO
	m_nSize = 0;
	m_nMax = ARRAY_MAX_ELEMENTS;
	m_pData = new double[m_nMax];

}

// free the array
void DArray::Free() {
	if (m_pData != nullptr){
		delete[] m_pData;
		m_pData = nullptr;
	}
	m_nSize = 0;

}

// get the size of the array
int DArray::GetSize() const {
	//TODO
	assert(m_nSize >= 0);
	return m_nSize; // if you want to return a value, you should return a correct value
}

// set the size of the array
void DArray::SetSize(int nSize) {
	//TODO
	// if (nSize > ARRAY_MAX_ELEMENTS)
	// {	
	// 	nSize = ARRAY_MAX_ELEMENTS;
	// 	std::cout << "Size is too large, use ARRAY_MAX_ELEMENTS instead";
	// }
	// if (nSize < 0){
	// 	nSize = 0;
	// 	std::cout << "Size is too small, use 0 instead";
	// }

	Reserve(nSize);
	m_nSize = nSize;

}

// get an element at an index
const double& DArray::GetAt(int nIndex) const {
	//TODO
	if (nIndex < 0 || nIndex >= m_nSize)
	{
		throw std::out_of_range("Index is out of range");
	}
	else{
		return m_pData[nIndex];
	}
}

// set the value of an element 
void DArray::SetAt(int nIndex, double dValue) {
	//TODO
	if (nIndex < 0 || nIndex >= m_nSize)
	{
        throw std::out_of_range("Index is out of range");
	}
	else{
		m_pData[nIndex] = dValue;
	}
}

// overload operator '[]'
double& DArray::operator[](int nIndex) {
	// TODO
	if (nIndex < 0 || nIndex >= m_nSize){
		throw std::out_of_range("Index is out of range");
	}
	return m_pData[nIndex];
}

// overload operator '[]'
const double& DArray::operator[](int nIndex) const {
	//TODO
	if (nIndex < 0 || nIndex >= m_nSize){
		throw std::out_of_range("Index is out of range");
	}
	return m_pData[nIndex];
}

// add a new element at the end of the array
void DArray::PushBack(double dValue) {
	//TODO
	if (m_nSize == ARRAY_MAX_ELEMENTS){
		std::cout << "Array is full";
		return;
	}
	m_pData[m_nSize] = dValue;
	m_nSize++;

	return;

}

// delete an element at some index
void DArray::DeleteAt(int nIndex) {
	//TODO
	if (nIndex < 0 || nIndex >= m_nSize){
	throw std::out_of_range("Index is out of range");	
	}
	for (int i = nIndex; i < m_nSize - 1; i++){
		m_pData[i] = m_pData[i + 1];
	}
	m_nSize--;
	return;
}

// insert a new element at some index
void DArray::InsertAt(int nIndex, double dValue) {
	//TODO
	if (nIndex < 0 || nIndex >m_nSize){
	throw std::out_of_range("Index is out of range");	
	}
	if (m_nSize == ARRAY_MAX_ELEMENTS){
	throw std::out_of_range("Array is full");	
	}

	memmove(m_pData + nIndex + 1, m_pData + nIndex, (m_nSize - nIndex) * sizeof(double));

	m_pData[nIndex] = dValue;
	m_nSize++;
}

// overload operator '='
DArray& DArray::operator = (const DArray& arr) {
	//TODO
	if (this != &arr){
		Free();
		Init();
		SetSize(arr.GetSize());
		for (int i = 0; i < GetSize(); i++){
			SetAt(i, arr.GetAt(i));
		}	
	}
	return *this;
}

// 在DArray.cpp中实现Reserve函数
void DArray::Reserve(int nSize) {
    // 如果请求的大小小于等于当前容量，直接返回
    if (nSize <= m_nMax) {
        return;
    }

    // 分配新内存
    double* pNewData = new double[nSize];

    // 复制现有数据到新内存
    if (m_pData != nullptr) {
        memmove(pNewData, m_pData, m_nSize * sizeof(double));
        // 释放旧内存
        delete[] m_pData;
    }

    // 更新指针和容量
    m_pData = pNewData;
    m_nMax = nSize;
}