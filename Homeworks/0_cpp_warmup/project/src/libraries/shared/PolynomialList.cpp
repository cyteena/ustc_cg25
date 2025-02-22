#include <iostream>
#include <fstream>
#include <map>
#include <algorithm>
#include <stdexcept>
#include "PolynomialList.h"
#include <sstream>

// 通过引用& 构造
PolynomialList::PolynomialList(const PolynomialList& other) {
    m_Polynomial = other.m_Polynomial;
}

// 文件构造
PolynomialList::PolynomialList(const std::string& file) {
    if (!ReadFromFile(file)) {
        throw std::runtime_error("File not found");
    }
}

// 数组构造
PolynomialList::PolynomialList(const double* cof, const int* deg, int n) {
    for (int i = 0; i < n; i++) {
        AddOneTerm(Term(deg[i], cof[i]));
    } 
}

// 向量构造
PolynomialList::PolynomialList(const std::vector<int>& deg, const std::vector<double>& cof) {
    auto di = deg.begin();
    auto ci = cof.begin();
    while (di != deg.end() && ci != cof.end()) {
        AddOneTerm(Term(*di, *ci));
        di++;
        ci++;
    }
    compress();
}

// 获取系数（无const, 即可修改版本）
double& PolynomialList::coff(int deg){
    for (auto& term : m_Polynomial) {
        if (term.deg == deg) {
            return term.cof;
        }
    }
    throw std::out_of_range("Degree not found");
}

// 获取系数（const, 不可修改版本）
double PolynomialList::coff(int deg) const{
    for (const auto& term : m_Polynomial) {
        if (term.deg == deg) {
            return term.cof;
        }
    }
    throw std::out_of_range("Degree not found");
}

// compress, 也就是合并同类项
void PolynomialList::compress(){
   std::map<int, double> termMap; // std::map是什么意思？
   for (const auto& term : m_Polynomial) {
        termMap[term.deg] += term.cof;
   }

   m_Polynomial.clear();
   for (auto it = termMap.rbegin(); it != termMap.rend(); ++it){
    if (it->second != 0.0){
        m_Polynomial.emplace_back(it->first, it->second);
    }
   }
}


//运算符重载+
PolynomialList PolynomialList::operator+(const PolynomialList& right) const {
  PolynomialList result(*this);
  for (const auto& term : right.m_Polynomial) {
     result.AddOneTerm(term);
  }
  result.compress();
  return result;
}

//运算符重载-
PolynomialList PolynomialList::operator-(const PolynomialList& right) const {
  PolynomialList result(*this); // 为什么这里需要加*this?
  for (const auto& term : right.m_Polynomial) {
     result.AddOneTerm(Term(term.deg, -term.cof));
  }
  return result;
}

//运算符重载*
PolynomialList PolynomialList::operator*(const PolynomialList& right) const {
  PolynomialList result;
  for(const auto& t1 : m_Polynomial){
   for(const auto& t2 : right.m_Polynomial){
    result.AddOneTerm(Term(t1.deg + t2.deg, t1.cof * t2.cof)); 
   } 
  } 
  result.compress();
  return result;
}

//运算符重载=
PolynomialList& PolynomialList::operator=(const PolynomialList& right) {
  if (this != &right) {
    m_Polynomial = right.m_Polynomial;
  }
  return *this;
}

// print
void PolynomialList::Print() const {
    bool first = true;
    for (const auto& term : m_Polynomial) {
            if (!first) std::cout << " ";
            auto cof_abs = std::abs(term.cof);
            if (term.deg != 0 && cof_abs != 1.0){ // 除非是常数项，否则不输出1
                if (term.cof < 0) {
                    if (!first) {
                        std::cout << "- " << cof_abs;
                    }
                    else{
                        std::cout << "-" << cof_abs;
                    }
                } 
                else{
                    if (!first){ // 不是第一项的正系数，先输出加号
                        std::cout << "+ " << term.cof;
                    }
                }
            }
            if (term.deg == 0){
                std::cout << term.cof;
            }

            if (term.deg > 0) std::cout << "x";
            if (term.deg > 1) std::cout << "^" << term.deg;

            first = false;
    }
    std::cout << std::endl;
}   

PolynomialList::Term& PolynomialList::AddOneTerm(const Term& term){
    auto it = m_Polynomial.begin();
    while (it != m_Polynomial.end() && it->deg > term.deg){
        ++it;
    } // 保证降序

    if (it != m_Polynomial.end() && it->deg == term.deg){
        it->cof += term.cof;
        return *it;
    }

    return *m_Polynomial.insert(it, term);
}

bool PolynomialList::ReadFromFile(const std::string& file){
    std::ifstream fin(file);
    if (!fin) return false;

    std::string header;
    std::getline(fin, header);

    // 解析项数
    int term_count;
    std::istringstream header_stream(header);
    if (!(header_stream >> std::ws >> term_count)){
        header_stream.clear();
        char dummy;
        header_stream >> dummy >> term_count;
    }

    // 读取指定数量的项
    int read_count = 0;
    int deg;
    double cof;
    while(read_count < term_count && fin >> deg >> cof){
        AddOneTerm(Term(deg, cof));
        read_count++;
    }

    // 验证实际读取项数
    if (read_count != term_count){
        m_Polynomial.clear();
        std::cout << "数据有误： 第一行项数不对 " << "read_count:" << read_count << " term_count:" << term_count << std::endl;
        return false;
    }

    compress();
    return true;

}
