#include "PolynomialMap.h"

#include <iostream>
#include <fstream>
#include <cassert>
#include <cmath>

using namespace std;

PolynomialMap::PolynomialMap(const PolynomialMap &other)
{
	// TODO
	m_Polynomial = other.m_Polynomial;
}

PolynomialMap::PolynomialMap(const string &file)
{
	ReadFromFile(file);
}

PolynomialMap::PolynomialMap(const double *cof, const int *deg, int n)
{
	for (int i = 0; i < n; i++)
	{
		m_Polynomial[deg[i]] = cof[i];
	}
	compress();
}

PolynomialMap::PolynomialMap(const vector<int> &deg, const vector<double> &cof)
{
	assert(deg.size() == cof.size());
	for (size_t i = 0; i < deg.size(); i++)
	{
		m_Polynomial[deg[i]] = cof[i];
	}
	compress();
	// TODO
}

// 系数访问 （const 版本）
double PolynomialMap::coff(int i) const
{
	// TODO
	auto it = m_Polynomial.find(i);

	return (it != m_Polynomial.end()) ? it->second : 0.0;
}
//
double &PolynomialMap::coff(int i)
{
	// TODO
	return m_Polynomial[i];
}

void PolynomialMap::compress()
{
	// TODO
	auto it = m_Polynomial.begin();
	while (it != m_Polynomial.end())
	{
		if (abs(it->second) < 1e-6)
		{
			it = m_Polynomial.erase(it);
		}
		else
		{
			it++;
		}
	}
}

PolynomialMap PolynomialMap::operator+(const PolynomialMap &right) const
{
	PolynomialMap result(*this);
	for (const auto &term : right.m_Polynomial)
	{
		result.m_Polynomial[term.first] += term.second;
	}
	result.compress();
	return result; // you should return a correct value
}

PolynomialMap PolynomialMap::operator-(const PolynomialMap &right) const
{
	PolynomialMap result(*this);
	for (const auto &term : right.m_Polynomial)
	{
		result.m_Polynomial[term.first] -= term.second;
	}
	result.compress();
	return result; // you should return a correct value
}

PolynomialMap PolynomialMap::operator*(const PolynomialMap &right) const
{
	PolynomialMap result;
	for (const auto &t1 : m_Polynomial)
	{
		for (const auto &t2 : right.m_Polynomial)
		{
			int new_deg = t1.first + t2.second;
			double new_cof = t1.second * t2.second;
			result.m_Polynomial[new_deg] += new_cof;
		}
	}
	result.compress();

	return result; // you should return a correct value
}

PolynomialMap &PolynomialMap::operator=(const PolynomialMap &right)
{
	if (this != &right)
	{
		m_Polynomial = right.m_Polynomial;
	}
	return *this;
}

void PolynomialMap::Print() const
{
	if (m_Polynomial.empty())
	{
		std::cout << "0";
		return;
	}

	bool firstterm = true;
	// 反向遍历实现降序输出 (map默认升序存储)
	for (auto it = m_Polynomial.rbegin(); it != m_Polynomial.rend(); it++)
	{
		const double coeff = it->second;
		const int degree = it->first;

		if (!firstterm)
		{
			std::cout << (coeff > 0 ? " + " : " - ");
		}
		else if (coeff < 0)
		{
			std::cout << "-";
		}

		const double abs_coeff = std::abs(coeff);
		if (abs_coeff != 1.0 || degree == 0)
		{
			std::cout << abs_coeff;
		}

		if (degree > 0)
		{
			std::cout << "x";
			if (degree > 1)
			{
				std::cout << "^" << degree;
			}
		}

		firstterm = false;
	}
	std::cout << std::endl;
}

bool PolynomialMap::ReadFromFile(const string &file)
{
	m_Polynomial.clear();

	ifstream fin(file);
	if (!fin.is_open())
	{
		return false;
	}

	// read the first line;
	string dummy;
	int termCount;
	fin >> dummy >> termCount;

	// read the following line;
	int degree;
	double coeff;
	for (int i = 0; i < termCount; i++)
	{
		if (!(fin >> degree >> coeff))
		{
			fin.close();
			return false;
		}
		m_Polynomial[degree] += coeff;
	}
	fin.close();
	compress();

	return true; // you should return a correct value
}
