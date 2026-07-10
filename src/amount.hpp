#pragma once

#include <cstdint>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

namespace tdtl {

class Amount {
public:
    Amount();
    explicit Amount(std::int64_t raw);

    static Amount zero();
    static Amount of(std::int64_t raw);
    static std::optional<Amount> checked(std::int64_t raw);

    std::int64_t raw() const;
    bool isZero() const;
    bool isPositive() const;
    bool isNegative() const;

    std::string str() const;
    std::string format(int decimals) const;

    Amount abs() const;
    Amount clampFloor(Amount floor) const;
    Amount clampCeil(Amount ceil) const;

    Amount operator+(Amount other) const;
    Amount operator-(Amount other) const;
    Amount operator*(std::int64_t factor) const;
    Amount operator/(std::int64_t divisor) const;
    Amount& operator+=(Amount other);
    Amount& operator-=(Amount other);
    Amount& operator*=(std::int64_t factor);
    Amount& operator/=(std::int64_t divisor);

    bool operator==(Amount other) const;
    bool operator!=(Amount other) const;
    bool operator<(Amount other) const;
    bool operator<=(Amount other) const;
    bool operator>(Amount other) const;
    bool operator>=(Amount other) const;

private:
    std::int64_t raw_;
};

struct Ratio {
    std::int64_t numerator{0};
    std::int64_t denominator{1};

    static Ratio bps(std::int64_t value);
    static Ratio unit();
    bool valid() const;
    Amount apply(Amount amount) const;
    std::string str() const;
};

class AmountAccumulator {
public:
    void add(Amount amount);
    void subtract(Amount amount);
    Amount total() const;
    Amount min() const;
    Amount max() const;
    std::size_t count() const;
    bool empty() const;
    std::vector<Amount> values() const;

private:
    std::vector<Amount> values_;
    Amount total_{Amount::zero()};
};

std::ostream& operator<<(std::ostream& os, Amount amount);

} // namespace tdtl
