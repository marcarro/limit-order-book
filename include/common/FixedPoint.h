#pragma once
#include <cstdint>
#include <iostream>
#include <string>

class Price {
private:
    int64_t value_; // Internal representation (price * SCALE)
    static constexpr int64_t SCALE = 10000; // 4 decimal places of precision
    
public:
    // Constructors
    Price();
    
    // From double - use carefully
    explicit Price(double price);
    
    // From int64_t raw value
    static Price fromRaw(int64_t raw);   

	// From string (safer than double for exact representation)
	explicit Price(const std::string& price_str);   

    // Accessors
    int64_t raw_value() const;
    
    // Convert to double (for display/external use only)
    double to_double() const;
    
    // Convert to string with proper decimal places
    std::string to_string() const;    

    // Arithmetic operators
	Price operator+(const Price& other) const;    
    Price operator-(const Price& other) const;    
    Price operator*(int mult) const;    
    Price operator/(int div) const;    

    // Comparison operators
    bool operator==(const Price& other) const;
    bool operator!=(const Price& other) const;
    bool operator<(const Price& other) const;
    bool operator<=(const Price& other) const;
    bool operator>(const Price& other) const;
    bool operator>=(const Price& other) const;
};

inline std::ostream& operator<<(std::ostream& os, const Price& price) {
    os << price.to_string();
    return os;
}

namespace std {
    template<>
    struct hash<Price> {
        size_t operator()(const Price& p) const;    
    };
}
