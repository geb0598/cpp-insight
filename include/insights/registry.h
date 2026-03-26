#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "insights/archive.h"

namespace insights {

// -------------------------------------------------
// Descriptor
// -------------------------------------------------
class Group;

class Descriptor {
public:
    using Id = int32_t;

    static constexpr Id INVALID_ID = -1;
    static constexpr Id FRAME_ID   = 0;

    Descriptor() : id_(INVALID_ID), name_("") {}

    Descriptor(std::string name, Group& group);

    static Descriptor& GetFrameDescriptor() {
        static Descriptor instance(FRAME_ID, "Frame");
        return instance;
    }
    static Id PeekId() { return next_id; }

    Id                 GetId()    const { return id_; }
    const std::string& GetName()  const { return name_; }

    friend Archive& operator<<(Archive& ar, Descriptor& descriptor) {
        ar << descriptor.id_;
        ar << descriptor.name_;
        return ar;
    }

private:
    Descriptor(Id id, std::string name);

    static Id NextId() { return next_id++; }

    static inline Id next_id = 1;

    Id          id_;
    std::string name_;
};

// -------------------------------------------------
// Group
// -------------------------------------------------

class Group {
public:
    using Id = int32_t;

    static constexpr Id INVALID_ID = -1;
    static constexpr Id FRAME_ID   = 0;

    Group() : id_(INVALID_ID), name_("") {}

    explicit Group(std::string name);

    static Group& GetFrameGroup() {
        static Group instance(FRAME_ID, "Frame");
        return instance;
    }
    static Id PeekId() { return next_id; }

    Id                 GetId()   const { return id_; }
    const std::string& GetName() const { return name_; }

    void AddDescriptor(Descriptor::Id desc_id) { descriptors_.push_back(desc_id); }

    const std::vector<Descriptor::Id>& GetDescriptors() const { return descriptors_; }

    friend Archive& operator<<(Archive& ar, Group& group) {
        ar << group.id_;
        ar << group.name_;
        ar << group.descriptors_;
        return ar;
    }

private:
    Group(Id id, std::string name);

    static Id   NextId() { return next_id++; }

    static inline Id next_id = 1;

    Id                              id_;
    std::string                     name_;
    std::vector<Descriptor::Id> descriptors_;
};

// -------------------------------------------------
// Registry
// -------------------------------------------------
class Registry {
public:
    using DescriptorMap  = std::unordered_map<Descriptor::Id, Descriptor*>;
    using GroupMap       = std::unordered_map<Group::Id, Group*>;

    static Registry& GetInstance() {
        static Registry instance;
        return instance;
    }

    Registry(const Registry&)            = delete;
    Registry& operator=(const Registry&) = delete;
    Registry(Registry&&)                 = delete;
    Registry& operator=(Registry&&)      = delete;

    void RegisterDescriptor(Descriptor* descriptor) { descriptors_[descriptor->GetId()] = descriptor; }
    void RegisterGroup(Group* group) { groups_[group->GetId()] = group; }
    void Clear() {
        descriptors_.clear();
        groups_.clear();
    }

    Descriptor* FindDescriptor(Descriptor::Id id) const {
        auto it = descriptors_.find(id);
        if (it == descriptors_.end()) {
            return nullptr;
        }
        return it->second;
    }

    Group* FindGroup(Group::Id id) const {
        auto it = groups_.find(id);
        if (it == groups_.end()) {
            return nullptr;
        }
        return it->second;
    }

    const DescriptorMap& GetDescriptors()  const { return descriptors_; }
    const GroupMap&      GetGroups()       const { return groups_; }

private:
    Registry()  = default;
    ~Registry() = default;

    DescriptorMap descriptors_;
    GroupMap      groups_;
};

// -------------------------------------------------
// Implementations
// -------------------------------------------------
inline Descriptor::Descriptor(std::string name, Group& group)
    : id_(NextId()), name_(std::move(name)) {
    group.AddDescriptor(id_);
    Registry::GetInstance().RegisterDescriptor(this);
}
inline Descriptor::Descriptor(Id id, std::string name)
    : id_(id), name_(std::move(name)) {
    Group::GetFrameGroup().AddDescriptor(id_);
    Registry::GetInstance().RegisterDescriptor(this);
}

inline Group::Group(std::string name)
    : id_(NextId()), name_(std::move(name)) {
    Registry::GetInstance().RegisterGroup(this);
}

inline Group::Group(Id id, std::string name) 
    : id_(id), name_(std::move(name)) {
    Registry::GetInstance().RegisterGroup(this);
}

} // namespace insights