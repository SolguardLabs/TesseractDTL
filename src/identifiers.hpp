#pragma once

#include <functional>
#include <ostream>
#include <string>
#include <vector>

namespace tdtl {

class Id {
public:
    Id();
    explicit Id(std::string value);

    const std::string& value() const;
    bool empty() const;
    std::string str() const;

    bool operator==(const Id& other) const;
    bool operator!=(const Id& other) const;
    bool operator<(const Id& other) const;

private:
    std::string value_;
};

struct AssetId : public Id {
    using Id::Id;
};

struct DomainId : public Id {
    using Id::Id;
};

struct OperatorId : public Id {
    using Id::Id;
};

struct AccountId : public Id {
    using Id::Id;
};

struct VaultId : public Id {
    using Id::Id;
};

struct RouteId : public Id {
    using Id::Id;
};

struct OrderId : public Id {
    using Id::Id;
};

struct WindowId : public Id {
    using Id::Id;
};

struct EpochId {
    int value{0};

    EpochId();
    explicit EpochId(int epoch);

    std::string str() const;
    bool operator==(EpochId other) const;
    bool operator!=(EpochId other) const;
    bool operator<(EpochId other) const;
    bool operator<=(EpochId other) const;
    bool operator>(EpochId other) const;
    bool operator>=(EpochId other) const;
};

struct DimensionalKey {
    AssetId asset;
    DomainId domain;
    OperatorId operatorId;
    EpochId epoch;
    WindowId window;

    std::string str() const;
    bool operator<(const DimensionalKey& other) const;
};

std::ostream& operator<<(std::ostream& os, const Id& id);
std::ostream& operator<<(std::ostream& os, EpochId epoch);

std::string joinIds(const std::vector<Id>& ids, const std::string& sep);

} // namespace tdtl

namespace std {

template <>
struct hash<tdtl::Id> {
    std::size_t operator()(const tdtl::Id& id) const noexcept {
        return std::hash<std::string>{}(id.value());
    }
};

} // namespace std
