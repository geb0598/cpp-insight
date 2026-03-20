#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "insight/archive.h"

namespace insight {

// -------------------------------------------------
// StatDescriptor
// -------------------------------------------------
class StatGroup;

class StatDescriptor {
public:
    using Id = int32_t;

    static constexpr Id INVALID_ID = -1;
    static constexpr Id FRAME_ID   = 0;

    StatDescriptor() : id_(INVALID_ID), name_("") {}

    StatDescriptor(std::string name, StatGroup& group);

    static StatDescriptor& GetFrameDescriptor() {
        static StatDescriptor instance(FRAME_ID, "");
        return instance;
    }
    static Id PeekId() { return next_id; }

    Id                 GetId()    const { return id_; }
    const std::string& GetName()  const { return name_; }

    friend Archive& operator<<(Archive& ar, StatDescriptor& descriptor) {
        ar << descriptor.id_;
        ar << descriptor.name_;
        return ar;
    }

private:
    StatDescriptor(Id id, std::string name);

    static Id NextId() { return next_id++; }

    static inline Id next_id = 1;

    Id          id_;
    std::string name_;
};

// -------------------------------------------------
// StatGroup
// -------------------------------------------------

class StatGroup {
public:
    using Id = int32_t;

    static constexpr Id INVALID_ID = -1;
    static constexpr Id FRAME_ID   = 0;

    StatGroup() : id_(INVALID_ID), name_("") {}

    explicit StatGroup(std::string name);

    static StatGroup& GetFrameGroup() {
        static StatGroup instance(FRAME_ID, "");
        return instance;
    }
    static Id PeekId() { return next_id; }

    Id                 GetId()   const { return id_; }
    const std::string& GetName() const { return name_; }

    void AddDescriptor(StatDescriptor::Id desc_id) { descriptors_.push_back(desc_id); }

    const std::vector<StatDescriptor::Id>& GetDescriptors() const { return descriptors_; }

    friend Archive& operator<<(Archive& ar, StatGroup& group) {
        ar << group.id_;
        ar << group.name_;
        ar << group.descriptors_;
        return ar;
    }

private:
    StatGroup(Id id, std::string name);

    static Id   NextId() { return next_id++; }

    static inline Id next_id = 1;

    Id                              id_;
    std::string                     name_;
    std::vector<StatDescriptor::Id> descriptors_;
};

// -------------------------------------------------
// StatRegistry
// -------------------------------------------------
class StatRegistry {
public:
    using DescriptorMap  = std::unordered_map<StatDescriptor::Id, StatDescriptor*>;
    using GroupMap       = std::unordered_map<StatGroup::Id, StatGroup*>;

    static StatRegistry& GetInstance() {
        static StatRegistry instance;
        return instance;
    }

    StatRegistry(const StatRegistry&)            = delete;
    StatRegistry& operator=(const StatRegistry&) = delete;
    StatRegistry(StatRegistry&&)                 = delete;
    StatRegistry& operator=(StatRegistry&&)      = delete;

    void RegisterDescriptor(StatDescriptor* descriptor) { descriptors_[descriptor->GetId()] = descriptor; }
    void RegisterGroup(StatGroup* group) { groups_[group->GetId()] = group; }
    void Clear() {
        descriptors_.clear();
        groups_.clear();
    }

    StatDescriptor* FindDescriptor(StatDescriptor::Id id) const {
        auto it = descriptors_.find(id);
        if (it == descriptors_.end()) {
            return nullptr;
        }
        return it->second;
    }

    StatGroup* FindGroup(StatGroup::Id id) const {
        auto it = groups_.find(id);
        if (it == groups_.end()) {
            return nullptr;
        }
        return it->second;
    }

    const DescriptorMap& GetDescriptors()  const { return descriptors_; }
    const GroupMap&      GetGroups()       const { return groups_; }

private:
    StatRegistry()  = default;
    ~StatRegistry() = default;

    DescriptorMap descriptors_;
    GroupMap      groups_;
};

// -------------------------------------------------
// Implementations
// -------------------------------------------------
inline StatDescriptor::StatDescriptor(std::string name, StatGroup& group)
    : id_(NextId()), name_(std::move(name)) {
    group.AddDescriptor(id_);
    StatRegistry::GetInstance().RegisterDescriptor(this);
}
inline StatDescriptor::StatDescriptor(Id id, std::string name)
    : id_(id), name_(std::move(name)) {
    StatGroup::GetFrameGroup().AddDescriptor(id_);
    StatRegistry::GetInstance().RegisterDescriptor(this);
}

inline StatGroup::StatGroup(std::string name)
    : id_(NextId()), name_(std::move(name)) {
    StatRegistry::GetInstance().RegisterGroup(this);
}

inline StatGroup::StatGroup(Id id, std::string name) 
    : id_(id), name_(std::move(name)) {
    StatRegistry::GetInstance().RegisterGroup(this);
}

} // namespace insight