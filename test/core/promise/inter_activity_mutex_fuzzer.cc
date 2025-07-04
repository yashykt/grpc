// Copyright 2025 gRPC authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <google/protobuf/text_format.h>
#include <grpc/grpc.h>

#include <limits>
#include <optional>
#include <thread>

#include "fuzztest/fuzztest.h"
#include "gtest/gtest.h"
#include "src/core/lib/promise/inter_activity_mutex.h"
#include "src/core/lib/promise/party.h"
#include "src/core/lib/promise/promise.h"
#include "src/core/lib/promise/seq.h"
#include "src/core/lib/resource_quota/arena.h"
#include "src/core/util/notification.h"
#include "test/core/promise/inter_activity_mutex_fuzzer.pb.h"
#include "test/core/promise/poll_matcher.h"

namespace grpc_core {
namespace {

int unused = (grpc_tracer_init(), 0);

// A mock activity that can be activated and deactivated.
class MockActivity : public Activity, public Wakeable {
 public:
  MOCK_METHOD(void, WakeupRequested, ());

  void ForceImmediateRepoll(WakeupMask /*mask*/) override { WakeupRequested(); }
  void Orphan() override {}
  Waker MakeOwningWaker() override { return Waker(this, 0); }
  Waker MakeNonOwningWaker() override { return Waker(this, 0); }
  void Wakeup(WakeupMask /*mask*/) override { WakeupRequested(); }
  void WakeupAsync(WakeupMask /*mask*/) override { WakeupRequested(); }
  void Drop(WakeupMask /*mask*/) override {}
  std::string DebugTag() const override { return "MockActivity"; }
  std::string ActivityDebugTag(WakeupMask /*mask*/) const override {
    return DebugTag();
  }

  void Activate() {
    if (scoped_activity_ == nullptr) {
      scoped_activity_ = std::make_unique<ScopedActivity>(this);
    }
  }

  void Deactivate() { scoped_activity_.reset(); }

 private:
  std::unique_ptr<ScopedActivity> scoped_activity_;
};

#define EXPECT_WAKEUP(activity, statement)                                 \
  EXPECT_CALL((activity), WakeupRequested()).Times(::testing::AtLeast(1)); \
  statement;                                                               \
  Mock::VerifyAndClearExpectations(&(activity));

template <typename Lock>
void Drop(Lock lock) {}

class AlwaysFairFuzzer {
 public:
  using Op = inter_activity_mutex_fuzzer::Op;
  using Mutex = InterActivityMutex<uint32_t>;
  using Lock = typename Mutex::Lock;

  void Run(Op op) {
    switch (op.type_case()) {
      case Op::kPoll: {
        if (op.poll().id() >= kNumSlots) return;
        Poll(op.poll().id());
      } break;
      case Op::kDrop: {
        if (op.drop().id() >= kNumSlots) return;
        auto& slot = slots_[op.drop().id()];
        slot.poll = []() { return Pending{}; };
        slot.trigger = None{};
      } break;
      case Op::kAcquire: {
        if (op.acquire().id() >= kNumSlots) return;
        Acquire(op.acquire().id());
      } break;
      case Op::kAcquireWhen: {
        if (op.acquire_when().id() >= kNumSlots) return;
        AcquireWhen(op.acquire_when().id(), op.acquire_when().when());
      } break;
      case Op::kDropLock: {
        lock_.reset();
      } break;
      case Op::kSetLock: {
        if (!lock_.has_value()) return;
        lock_value_ = op.set_lock().value();
        **lock_ = lock_value_;
      } break;
      case Op::TYPE_NOT_SET:
        break;
    }
  }

 private:
  static constexpr const size_t kNumSlots = 1024;

  struct None {};
  struct Always {};

  struct Slot {
    uint64_t acquire_order = 0;
    std::variant<None, Always, uint32_t> trigger = None{};
    absl::AnyInvocable<Poll<Lock>()> poll = []() { return Pending{}; };
  };

  void Poll(uint32_t id) {
    auto& slot = slots_[id];
    auto poll = slot.poll();
    // If a lock is returned, we should not have a lock already.
    if (poll.ready()) {
      CHECK(!lock_.has_value());
      CHECK_EQ(id, ExpectedVictor()) << ExpectedQueue();
      lock_.emplace(std::move(poll.value()));
      slot.poll = []() { return Pending{}; };
      slot.trigger = None{};
    }
  }

  void Acquire(uint32_t id) {
    auto& slot = slots_[id];
    slot.poll = mutex_.Acquire();
    slot.trigger = Always{};
    slot.acquire_order = next_acquire_order_++;
    Poll(id);
  }

  void AcquireWhen(uint32_t id, uint32_t when) {
    auto& slot = slots_[id];
    slot.poll = mutex_.AcquireWhen([when](uint32_t x) { return x == when; });
    slot.trigger = when;
    slot.acquire_order = next_acquire_order_++;
    Poll(id);
  }

  uint32_t ExpectedVictor() const {
    const Slot* ordered_slots[kNumSlots];
    for (size_t i = 0; i < kNumSlots; ++i) {
      ordered_slots[i] = &slots_[i];
    }
    std::stable_sort(ordered_slots, ordered_slots + kNumSlots,
                     [](const Slot* a, const Slot* b) {
                       return a->acquire_order < b->acquire_order;
                     });
    for (size_t i = 0; i < kNumSlots; ++i) {
      const Slot& slot = *ordered_slots[i];
      size_t index = ordered_slots[i] - slots_;
      if (std::holds_alternative<None>(slot.trigger)) continue;
      if (std::holds_alternative<Always>(slot.trigger)) return index;
      if (std::get<uint32_t>(slot.trigger) == lock_value_) return index;
    }
    return std::numeric_limits<uint32_t>::max();
  }

  std::string ExpectedQueue() const {
    std::vector<std::string> wtf;
    wtf.push_back(absl::StrCat("lock_value=", lock_value_));
    const Slot* ordered_slots[kNumSlots];
    for (size_t i = 0; i < kNumSlots; ++i) {
      ordered_slots[i] = &slots_[i];
    }
    std::stable_sort(ordered_slots, ordered_slots + kNumSlots,
                     [](const Slot* a, const Slot* b) {
                       return a->acquire_order < b->acquire_order;
                     });
    for (size_t i = 0; i < kNumSlots; ++i) {
      const Slot& slot = *ordered_slots[i];
      size_t index = ordered_slots[i] - slots_;
      if (std::holds_alternative<None>(slot.trigger)) continue;
      if (std::holds_alternative<Always>(slot.trigger)) {
        wtf.push_back(
            absl::StrCat("[", index, "]: Always order=", slot.acquire_order));
        continue;
      }
      wtf.push_back(absl::StrCat(
          "[", index, "]: trigger=", std::get<uint32_t>(slot.trigger),
          ", order=", slot.acquire_order));
    }
    return absl::StrJoin(wtf, "\n");
  }

  uint32_t lock_value_ = 0;
  Mutex mutex_{lock_value_};
  Slot slots_[kNumSlots];
  std::optional<Lock> lock_;
  uint64_t next_acquire_order_ = 1;
};

void AlwaysFair(std::vector<inter_activity_mutex_fuzzer::Op> ops) {
  MockActivity activity;
  activity.Activate();
  EXPECT_CALL(activity, WakeupRequested()).Times(::testing::AnyNumber());
  AlwaysFairFuzzer fuzzer;
  for (const auto& op : ops) {
    fuzzer.Run(op);
  }
}
FUZZ_TEST(InterActivityMutexTest, AlwaysFair);

auto ParseTestProto(const std::string& proto) {
  inter_activity_mutex_fuzzer::Op msg;
  CHECK(google::protobuf::TextFormat::ParseFromString(proto, &msg));
  return msg;
}

TEST(InterActivityMutexTest, AlwaysFairRegression1) {
  AlwaysFair({ParseTestProto(R"pb(acquire_when { when: 1 })pb"),
              ParseTestProto(R"pb(poll {})pb"),
              ParseTestProto(R"pb(drop {})pb"),
              ParseTestProto(R"pb(acquire {})pb")});
}

TEST(InterActivityMutexTest, AlwaysFairRegression2) {
  AlwaysFair({ParseTestProto(R"pb(acquire_when {})pb"),
              ParseTestProto(R"pb(acquire { id: 1 })pb"),
              ParseTestProto(R"pb(acquire {})pb"),
              ParseTestProto(R"pb(poll {})pb")});
}

TEST(InterActivityMutexTest, AlwaysFairRegression3) {
  AlwaysFair({ParseTestProto(R"pb(acquire_when { when: 4294967295 })pb"),
              ParseTestProto(R"pb(acquire_when { when: 1 })pb")});
}

}  // namespace
}  // namespace grpc_core
