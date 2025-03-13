#ifndef FIXED_POINT_H
#define FIXED_POINT_H

#include <cstdint>
#include <iostream>
#include <string>

class Price {
private:
    int64_t value_; // Internal representation (price * SCALE)
    static constexpr int64_t SCALE = 10000; // 4 decimal places of precision
    
public:
    // Constructors
    Price() : value_(0) {}
    
    // From double - use carefully
    explicit Price(double price) : value_(static_cast<int64_t>(price * SCALE)) {}
    
    // From int64_t raw value
    static Price fromRaw(int64_t raw) {
        Price p;
        p.value_ = raw;
        return p;
    }
    
	// From string (safer than double for exact representation)
	explicit Price(const std::string& price_str) {
    	// Find decimal point
    	size_t decimal_pos = price_str.find('.');
    	
    	if (decimal_pos == std::string::npos) {
        	// No decimal point, just whole number
        	value_ = std::stoll(price_str) * SCALE;
        	return;
    	}
    	
    	// Has decimal point - handle whole part and decimal part separately
    	std::string whole_part = price_str.substr(0, decimal_pos);
    	int64_t whole_value = whole_part.empty() ? 0 : std::stoll(whole_part);
    	
    	// Extract decimal part and pad/truncate as needed
    	std::string decimal_part = price_str.substr(decimal_pos + 1);
    	
    	// Pad with zeros if too short
    	if (decimal_part.length() < 4) {
        	decimal_part.append(4 - decimal_part.length(), '0');
    	}
    	
    	// Truncate if too long
    	if (decimal_part.length() > 4) {
			std::string truncatedStr = decimal_part.substr(0, 4);
			decimal_part = truncatedStr;
    	}
    	
    	int64_t decimal_value = std::stoll(decimal_part);
    	
    	// Calculate the scaling factor based on actual decimal digits
    	int64_t decimal_scale = 1;
    	for (size_t i = 0; i < decimal_part.length(); i++) {
        	decimal_scale *= 10;
    	}
    	
    	// Combine whole and decimal parts
    	value_ = whole_value * SCALE + decimal_value * (SCALE / decimal_scale);
	}
    
    // Accessors
    int64_t raw_value() const { return value_; }
    
    // Convert to double (for display/external use only)
    double to_double() const { return static_cast<double>(value_) / SCALE; }
    
    // Convert to string with proper decimal places
    std::string to_string() const {
        int64_t whole_part = value_ / SCALE;
        int64_t decimal_part = value_ % SCALE;
        
        std::string result = std::to_string(whole_part);
        result += ".";
        
        // Format decimal part with leading zeros
        std::string decimal_str = std::to_string(decimal_part);
        if (decimal_part < 1000) {
            result.append(4 - decimal_str.length(), '0');
        }
        result += decimal_str;
        
        return result;
    }
    
    // Arithmetic operators
	Price operator+(const Price& other) const {
    	int64_t result = value_ + other.value_;
    	// Check for overflow
    	if ((value_ > 0 && other.value_ > 0 && result < 0) ||
        	(value_ < 0 && other.value_ < 0 && result > 0)) {
        	throw std::overflow_error("Price addition overflow");
    	}
    	return Price::fromRaw(result);
	}
    
    Price operator-(const Price& other) const {
        return Price::fromRaw(value_ - other.value_);
    }
    
    Price operator*(int mult) const {
        return Price::fromRaw(value_ * mult);
    }
    
    Price operator/(int div) const {
        return Price::fromRaw(value_ / div);
    }
    
    // Comparison operators
    bool operator==(const Price& other) const { return value_ == other.value_; }
    bool operator!=(const Price& other) const { return value_ != other.value_; }
    bool operator<(const Price& other) const { return value_ < other.value_; }
    bool operator<=(const Price& other) const { return value_ <= other.value_; }
    bool operator>(const Price& other) const { return value_ > other.value_; }
    bool operator>=(const Price& other) const { return value_ >= other.value_; }
};

inline std::ostream& operator<<(std::ostream& os, const Price& price) {
    os << price.to_string();
    return os;
}

namespace std {
    template<>
    struct hash<Price> {
        size_t operator()(const Price& p) const {
            return hash<int64_t>()(p.raw_value());
        }
    };
}

#endif
