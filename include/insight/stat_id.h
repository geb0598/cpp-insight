#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace insight {

// -------------------------------------------------
// StatGroup
// -------------------------------------------------
class StatGroup {
public:
    using Id = int;

    explicit StatGroup(const char* name)
        : name_(name), id_(NextId()) {}

    const char* GetName() const { return name_; }
    Id          GetId()   const { return id_; }

private:
    static Id NextId() {
        static Id next_id = 0;
        return next_id++;
    }

    const char* name_;
    const Id    id_;
};

// -------------------------------------------------
// StatRegistry
// -------------------------------------------------
class StatDescriptor;

class StatRegistry {
public:
    using GroupMap = std::unordered_map<StatGroup::Id, const StatGroup*>;
    using StatMap  = std::unordered_map<StatGroup::Id, std::vector<const StatDescriptor*>>;

    static StatRegistry& GetInstance() {
        static StatRegistry instance;
        return instance;
    }

    StatRegistry(const StatRegistry&) = delete;
    StatRegistry& operator=(const StatRegistry&) = delete;
    StatRegistry(StatRegistry&&) = delete;
    StatRegistry& operator=(StatRegistry&&) = delete;

    void Register(const StatDescriptor* descriptor);

    const GroupMap& GetGroups() const { return groups_; }
    const StatMap&  GetStats()  const { return stats_; }

private:
    StatRegistry() = default;

    ~StatRegistry() = default;

    GroupMap groups_;
    StatMap  stats_;
};

// -------------------------------------------------
// StatDescriptor
// -------------------------------------------------
class StatDescriptor {
public:
    using Id = int;

    StatDescriptor(const char* name, const StatGroup& group)
        : name_(name), group_(group), id_(NextId()) {
        StatRegistry::GetInstance().Register(this);
    }

    const StatGroup& GetGroup() const { return group_; }
    const char*      GetName()  const { return name_; }
    Id               GetId()    const { return id_; }

private:
    static Id NextId() {
        static Id next_id = 0;
        return next_id++;
    }

    const StatGroup& group_;
    const char*      name_;
    const Id         id_;
};

// -------------------------------------------------
// Implementations
// -------------------------------------------------
inline void StatRegistry::Register(const StatDescriptor* descriptor) {
    StatGroup::Id group_id = descriptor->GetGroup().GetId();
    groups_[group_id] = &descriptor->GetGroup();
    stats_[group_id].push_back(descriptor);
}

} // namespace insight

// -------------------------------------------------
// Macros
// -------------------------------------------------

#define INSIGHT_DECLARE_STATGROUP(group_desc, group_id) \
    static insight::StatGroup group_id(group_desc);

#define INSIGHT_DECLARE_CYCLE_STAT(counter_name, stat_id, group_id) \
    static insight::StatDescriptor stat_id(counter_name, group_id);