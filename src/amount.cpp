#include "amount.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>

namespace tdtl {

Amount::Amount() : raw_(0) {}

Amount::Amount(std::int64_t raw) : raw_(raw) {}

Amount Amount::zero() {
    return Amount(0);
}

Amount Amount::of(std::int64_t raw) {
    return Amount(raw);
}

std::optional<Amount> Amount::checked(std::int64_t raw) {
    if (raw < 0) {
        return std::nullopt;
    }
    return Amount(raw);
}

std::int64_t Amount::raw() const {
    return raw_;
}

bool Amount::isZero() const {
    return raw_ == 0;
}

bool Amount::isPositive() const {
    return raw_ > 0;
}

bool Amount::isNegative() const {
    return raw_ < 0;
}

std::string Amount::str() const {
    return std::to_string(raw_);
}

std::string Amount::format(int decimals) const {
    if (decimals <= 0) {
        return str();
    }
    std::int64_t scale = 1;
    for (int i = 0; i < decimals; ++i) {
        scale *= 10;
    }
    const bool neg = raw_ < 0;
    const std::int64_t value = neg ? -raw_ : raw_;
    const std::int64_t whole = value / scale;
    std::int64_t frac = value % scale;
    std::string out;
    if (neg) {
        out.push_back('-');
    }
    out += std::to_string(whole);
    out.push_back('.');
    std::string tail = std::to_string(frac);
    while (static_cast<int>(tail.size()) < decimals) {
        tail.insert(tail.begin(), '0');
    }
    out += tail;
    return out;
}

Amount Amount::abs() const {
    return raw_ < 0 ? Amount(-raw_) : *this;
}

Amount Amount::clampFloor(Amount floor) const {
    return raw_ < floor.raw_ ? floor : *this;
}

Amount Amount::clampCeil(Amount ceil) const {
    return raw_ > ceil.raw_ ? ceil : *this;
}

Amount Amount::operator+(Amount other) const {
    return Amount(raw_ + other.raw_);
}

Amount Amount::operator-(Amount other) const {
    return Amount(raw_ - other.raw_);
}

Amount Amount::operator*(std::int64_t factor) const {
    return Amount(raw_ * factor);
}

Amount Amount::operator/(std::int64_t divisor) const {
    if (divisor == 0) {
        throw std::runtime_error("division by zero");
    }
    return Amount(raw_ / divisor);
}

Amount& Amount::operator+=(Amount other) {
    raw_ += other.raw_;
    return *this;
}

Amount& Amount::operator-=(Amount other) {
    raw_ -= other.raw_;
    return *this;
}

Amount& Amount::operator*=(std::int64_t factor) {
    raw_ *= factor;
    return *this;
}

Amount& Amount::operator/=(std::int64_t divisor) {
    if (divisor == 0) {
        throw std::runtime_error("division by zero");
    }
    raw_ /= divisor;
    return *this;
}

bool Amount::operator==(Amount other) const {
    return raw_ == other.raw_;
}

bool Amount::operator!=(Amount other) const {
    return raw_ != other.raw_;
}

bool Amount::operator<(Amount other) const {
    return raw_ < other.raw_;
}

bool Amount::operator<=(Amount other) const {
    return raw_ <= other.raw_;
}

bool Amount::operator>(Amount other) const {
    return raw_ > other.raw_;
}

bool Amount::operator>=(Amount other) const {
    return raw_ >= other.raw_;
}

Ratio Ratio::bps(std::int64_t value) {
    return Ratio{value, 10000};
}

Ratio Ratio::unit() {
    return Ratio{1, 1};
}

bool Ratio::valid() const {
    return denominator > 0 && numerator >= 0;
}

Amount Ratio::apply(Amount amount) const {
    if (!valid()) {
        throw std::runtime_error("invalid ratio");
    }
    return Amount::of((amount.raw() * numerator) / denominator);
}

std::string Ratio::str() const {
    return std::to_string(numerator) + "/" + std::to_string(denominator);
}

void AmountAccumulator::add(Amount amount) {
    values_.push_back(amount);
    total_ += amount;
}

void AmountAccumulator::subtract(Amount amount) {
    values_.push_back(Amount::zero() - amount);
    total_ -= amount;
}

Amount AmountAccumulator::total() const {
    return total_;
}

Amount AmountAccumulator::min() const {
    if (values_.empty()) {
        return Amount::zero();
    }
    return *std::min_element(values_.begin(), values_.end());
}

Amount AmountAccumulator::max() const {
    if (values_.empty()) {
        return Amount::zero();
    }
    return *std::max_element(values_.begin(), values_.end());
}

std::size_t AmountAccumulator::count() const {
    return values_.size();
}

bool AmountAccumulator::empty() const {
    return values_.empty();
}

std::vector<Amount> AmountAccumulator::values() const {
    return values_;
}

std::ostream& operator<<(std::ostream& os, Amount amount) {
    os << amount.raw();
    return os;
}

} // namespace tdtl
