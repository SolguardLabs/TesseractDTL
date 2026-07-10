#include "identifiers.hpp"

#include <sstream>
#include <tuple>

namespace tdtl {

Id::Id() : value_() {}

Id::Id(std::string value) : value_(std::move(value)) {}

const std::string& Id::value() const {
    return value_;
}

bool Id::empty() const {
    return value_.empty();
}

std::string Id::str() const {
    return value_;
}

bool Id::operator==(const Id& other) const {
    return value_ == other.value_;
}

bool Id::operator!=(const Id& other) const {
    return value_ != other.value_;
}

bool Id::operator<(const Id& other) const {
    return value_ < other.value_;
}

EpochId::EpochId() : value(0) {}

EpochId::EpochId(int epoch) : value(epoch) {}

std::string EpochId::str() const {
    return std::to_string(value);
}

bool EpochId::operator==(EpochId other) const {
    return value == other.value;
}

bool EpochId::operator!=(EpochId other) const {
    return value != other.value;
}

bool EpochId::operator<(EpochId other) const {
    return value < other.value;
}

bool EpochId::operator<=(EpochId other) const {
    return value <= other.value;
}

bool EpochId::operator>(EpochId other) const {
    return value > other.value;
}

bool EpochId::operator>=(EpochId other) const {
    return value >= other.value;
}

std::string DimensionalKey::str() const {
    std::ostringstream os;
    os << asset.str();
    os << ":";
    os << domain.str();
    os << ":";
    os << operatorId.str();
    os << ":";
    os << epoch.value;
    os << ":";
    os << window.str();
    return os.str();
}

bool DimensionalKey::operator<(const DimensionalKey& other) const {
    return std::tie(asset, domain, operatorId, epoch.value, window) <
           std::tie(other.asset, other.domain, other.operatorId, other.epoch.value, other.window);
}

std::ostream& operator<<(std::ostream& os, const Id& id) {
    os << id.value();
    return os;
}

std::ostream& operator<<(std::ostream& os, EpochId epoch) {
    os << epoch.value;
    return os;
}

std::string joinIds(const std::vector<Id>& ids, const std::string& sep) {
    std::ostringstream os;
    for (std::size_t i = 0; i < ids.size(); ++i) {
        if (i != 0) {
            os << sep;
        }
        os << ids[i].str();
    }
    return os.str();
}

} // namespace tdtl
