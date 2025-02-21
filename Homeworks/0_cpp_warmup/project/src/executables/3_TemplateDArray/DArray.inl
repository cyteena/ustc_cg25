#pragma once
#include <iostream>
#include <assert.h>
#include "DArray.h"


#define ARRAY_MAX_ELEMENTS 15

// default constructor
template<typename T>
DArray<T>::DArray() {
	Init();
}

// set an array with default values
template<typename T>
DArray<T>::DArray(int nSize, T dValue)
: m_pData(new T[nSize]), m_nSize(nSize), m_nMax(nSize)
{
	for (int i = 0; i < nSize; i++)
		SetAt(i, dValue);
}

template<typename T>
DArray<T>::DArray(const DArray<T>& arr)
: m_pData(new T[arr.m_nSize]), m_nSize(arr.m_nSize), m_nMax(arr.m_nSize)
{
	for (int i = 0; i < arr.GetSize(); i++)
		SetAt(i, arr.GetAt(i));
}

// deconstructor
template<typename T>
DArray<T>::~DArray() {
	Free();
}

// display the elements of the array
template<typename T>
void DArray<T>::Print() const {
	//TODO
	std::cout << "size= " << m_nSize << ": ";
	for (int i = 0; i < m_nSize; i++)
		std::cout << GetAt(i) << " ";

	std::cout << std::endl;

}

// initialize the array
template<typename T>
void DArray<T>::Init() {
	//TODO
	m_nSize = 0;
	m_nMax = 0;
	m_pData = nullptr;

}

// free the array
template<typename T>
void DArray<T>::Free() {
	if (m_pData != nullptr){
		delete[] m_pData;
		m_pData = nullptr;
	}
	m_nSize = 0;
	m_nMax = 0;

}

// get the size of the array
template<typename T>
int DArray<T>::GetSize() const {
	//TODO
	assert(m_nSize >= 0);
	return m_nSize; // if you want to return a value, you should return a correct value
}

// set the size of the array
template<typename T>
void DArray<T>::SetSize(int nSize) {
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

	if (m_nSize == nSize)
		return;

	Reserve(nSize);

	for (int i = m_nSize; i < nSize; i++){
		m_pData[i] = 0.0;
	}
	m_nSize = nSize;

}

// get an element at an index
template<typename T>
const T& DArray<T>::GetAt(int nIndex) const {
	//TODO
	assert(nIndex >= 0 && nIndex < m_nSize);
	return m_pData[nIndex];
}

// set the value of an element
template<typename T>
void DArray<T>::SetAt(int nIndex, T dValue) {
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
template<typename T>
T& DArray<T>::operator[](int nIndex) {
	// TODO
	if (nIndex < 0 || nIndex >= m_nSize){
		throw std::out_of_range("Index is out of range");
	}
	return m_pData[nIndex];
}

// overload operator '[]'
template<typename T>
const T& DArray<T>::operator[](int nIndex) const {
	//TODO
	if (nIndex < 0 || nIndex >= m_nSize){
		throw std::out_of_range("Index is out of range");
	}
	return m_pData[nIndex];
}

// add a new element at the end of the array
template<typename T>
void DArray<T>::PushBack(T dValue) {
	//TODO
	Reserve(m_nSize + 1);

	m_pData[m_nSize] = dValue;
	m_nSize++;

	return;

}

// delete an element at some index
template<typename T>
void DArray<T>::DeleteAt(int nIndex) {
	//TODO
	assert(nIndex >= 0 && nIndex < m_nSize);

	memmove(m_pData + nIndex, m_pData + nIndex + 1, (m_nSize - nIndex - 1) * sizeof(T));
	m_nSize--;
	return;
}

// insert a new element at some index
template<typename T>
void DArray<T>::InsertAt(int nIndex, T dValue) {
	//TODO

	assert(nIndex >= 0 && nIndex <= m_nSize);

	Reserve(m_nSize + 1);

	memmove(m_pData + nIndex + 1, m_pData + nIndex, (m_nSize - nIndex) * sizeof(T));

	m_pData[nIndex] = dValue;
	m_nSize++;
}

// overload operator '='
template<typename T>
DArray<T>& DArray<T>::operator = (const DArray<T>& arr) {
	//TODO
	Reserve(arr.m_nSize);

	m_nSize = arr.m_nSize;
	memcpy(m_pData, arr.m_pData, m_nSize * sizeof(T));
	return *this;
}

// 在DArray.cpp中实现Reserve函数
template<typename T>
void DArray<T>::Reserve(int nSize) {
	// 如果请求的大小小于等于当前容量，直接返回
	if (nSize <= m_nMax) {
		return;
	}

	while(m_nMax < nSize){
		m_nMax = m_nMax == 0 ? 1 : m_nMax * 2;
	}

	// 分配新内存
	T* pNewData = new T[m_nMax];

	// 复制现有数据到新内存
	if (m_pData != nullptr) {
		memmove(pNewData, m_pData, m_nSize * sizeof(T));
		// 释放旧内存
		delete[] m_pData;
	}

	// 更新指针和容量
	m_pData = pNewData;
}
