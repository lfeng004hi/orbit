// Copyright (c) 2020 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "TimerQueryPool.h"
#include "absl/container/flat_hash_set.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::_;
using ::testing::Return;

namespace orbit_vulkan_layer {

namespace {
class MockDispatchTable {
 public:
  MOCK_METHOD(PFN_vkCreateQueryPool, CreateQueryPool, (VkDevice), ());
  MOCK_METHOD(PFN_vkResetQueryPoolEXT, ResetQueryPoolEXT, (VkDevice), ());
};

PFN_vkCreateQueryPool dummy_create_query_pool_function =
    +[](VkDevice /*device*/, const VkQueryPoolCreateInfo* /*create_info*/,
        const VkAllocationCallbacks* /*allocator*/, VkQueryPool* query_pool_out) -> VkResult {
  *query_pool_out = {};
  return VK_SUCCESS;
};

PFN_vkResetQueryPoolEXT dummy_reset_query_pool_function =
    +[](VkDevice /*device*/, VkQueryPool /*query_pool*/, uint32_t /*first_query*/,
        uint32_t /*query_count*/) {};

}  // namespace

TEST(TimerQueryPool, ATimerQueryPoolMustGetInitialized) {
  MockDispatchTable dispatch_table;
  uint32_t num_slots = 4;
  TimerQueryPool query_pool(&dispatch_table, num_slots);
  VkDevice device = {};

  uint32_t slot_index = 32;
  std::vector<uint32_t> reset_slots;
  EXPECT_DEATH({ (void)query_pool.GetQueryPool(device); }, "");
  EXPECT_DEATH({ (void)query_pool.NextReadyQuerySlot(device, &slot_index); }, "");
  reset_slots.push_back(slot_index);
  EXPECT_DEATH({ (void)query_pool.ResetQuerySlots(device, reset_slots, true); }, "");
  EXPECT_DEATH({ (void)query_pool.ResetQuerySlots(device, reset_slots, false); }, "");
}

TEST(TimerQueryPool, InitializationWillCreateAndResetAPool) {
  MockDispatchTable dispatch_table;
  static constexpr uint32_t kNumSlots = 4;
  TimerQueryPool<MockDispatchTable> query_pool(&dispatch_table, kNumSlots);
  VkDevice device = {};

  PFN_vkCreateQueryPool mock_create_query_pool_function =
      +[](VkDevice /*device*/, const VkQueryPoolCreateInfo* create_info,
          const VkAllocationCallbacks* /*allocator*/, VkQueryPool* query_pool_out) -> VkResult {
    EXPECT_EQ(create_info->queryType, VK_QUERY_TYPE_TIMESTAMP);
    EXPECT_EQ(create_info->queryCount, kNumSlots);

    *query_pool_out = {};
    return VK_SUCCESS;
  };

  PFN_vkResetQueryPoolEXT mock_reset_query_pool_function =
      +[](VkDevice /*device*/, VkQueryPool /*query_pool*/, uint32_t first_query,
          uint32_t query_count) {
        EXPECT_EQ(first_query, 0);
        EXPECT_EQ(query_count, kNumSlots);
      };

  EXPECT_CALL(dispatch_table, CreateQueryPool)
      .Times(1)
      .WillOnce(Return(mock_create_query_pool_function));
  EXPECT_CALL(dispatch_table, ResetQueryPoolEXT)
      .Times(1)
      .WillOnce(Return(mock_reset_query_pool_function));

  query_pool.InitializeTimerQueryPool(device);
}

TEST(TimerQueryPool, QueryPoolCanBeRetrievedAfterInitialization) {
  MockDispatchTable dispatch_table;
  static constexpr uint32_t kNumSlots = 4;
  TimerQueryPool<MockDispatchTable> query_pool(&dispatch_table, kNumSlots);
  VkDevice device = {};
  static VkQueryPool expected_vulkan_query_pool = {};

  PFN_vkCreateQueryPool mock_create_query_pool_function =
      +[](VkDevice /*device*/, const VkQueryPoolCreateInfo* create_info,
          const VkAllocationCallbacks* /*allocator*/, VkQueryPool* query_pool_out) -> VkResult {
    EXPECT_EQ(create_info->queryType, VK_QUERY_TYPE_TIMESTAMP);
    EXPECT_EQ(create_info->queryCount, kNumSlots);

    *query_pool_out = expected_vulkan_query_pool;
    return VK_SUCCESS;
  };

  EXPECT_CALL(dispatch_table, CreateQueryPool)
      .WillRepeatedly(Return(mock_create_query_pool_function));
  EXPECT_CALL(dispatch_table, ResetQueryPoolEXT)
      .WillRepeatedly(Return(dummy_reset_query_pool_function));

  query_pool.InitializeTimerQueryPool(device);

  VkQueryPool vulkan_query_pool = query_pool.GetQueryPool(device);
  EXPECT_EQ(vulkan_query_pool, expected_vulkan_query_pool);
}

TEST(TimerQueryPool, CanRetrieveNumSlotsUniqueSlots) {
  MockDispatchTable dispatch_table;
  static constexpr uint32_t kNumSlots = 4;
  TimerQueryPool<MockDispatchTable> query_pool(&dispatch_table, kNumSlots);
  VkDevice device = {};

  EXPECT_CALL(dispatch_table, CreateQueryPool)
      .WillRepeatedly(Return(dummy_create_query_pool_function));
  EXPECT_CALL(dispatch_table, ResetQueryPoolEXT)
      .WillRepeatedly(Return(dummy_reset_query_pool_function));

  query_pool.InitializeTimerQueryPool(device);

  absl::flat_hash_set<uint32_t> slots;

  for (uint32_t i = 0; i < kNumSlots; ++i) {
    uint32_t slot;
    bool found_slot = query_pool.NextReadyQuerySlot(device, &slot);
    ASSERT_TRUE(found_slot);
    slots.insert(slot);
  }

  EXPECT_EQ(slots.size(), kNumSlots);
}

TEST(TimerQueryPool, CannotRetrieveMoreThanNumSlotsSlots) {
  MockDispatchTable dispatch_table;
  static constexpr uint32_t kNumSlots = 4;
  TimerQueryPool<MockDispatchTable> query_pool(&dispatch_table, kNumSlots);
  VkDevice device = {};

  EXPECT_CALL(dispatch_table, CreateQueryPool)
      .WillRepeatedly(Return(dummy_create_query_pool_function));
  EXPECT_CALL(dispatch_table, ResetQueryPoolEXT)
      .WillRepeatedly(Return(dummy_reset_query_pool_function));

  query_pool.InitializeTimerQueryPool(device);

  for (uint32_t i = 0; i < kNumSlots; ++i) {
    uint32_t slot;
    (void)query_pool.NextReadyQuerySlot(device, &slot);
  }
  uint32_t slot;
  bool found_slot = query_pool.NextReadyQuerySlot(device, &slot);
  ASSERT_FALSE(found_slot);
}

TEST(TimerQueryPool, ResettingSlotsMakesSlotsReady) {
  MockDispatchTable dispatch_table;
  static constexpr uint32_t kNumSlots = 1;
  TimerQueryPool<MockDispatchTable> query_pool(&dispatch_table, kNumSlots);
  VkDevice device = {};
  EXPECT_CALL(dispatch_table, CreateQueryPool)
      .WillRepeatedly(Return(dummy_create_query_pool_function));
  EXPECT_CALL(dispatch_table, ResetQueryPoolEXT)
      .WillRepeatedly(Return(dummy_reset_query_pool_function));

  query_pool.InitializeTimerQueryPool(device);
  uint32_t slot;
  (void)query_pool.NextReadyQuerySlot(device, &slot);

  std::vector<uint32_t> reset_slots;
  reset_slots.push_back(slot);

  query_pool.ResetQuerySlots(device, reset_slots, true);

  bool found_slot = query_pool.NextReadyQuerySlot(device, &slot);
  ASSERT_TRUE(found_slot);
}

TEST(TimerQueryPool, RollingBackSlotsDoesNotResetOnVulkan) {
  MockDispatchTable dispatch_table;
  static constexpr uint32_t kNumSlots = 1;
  TimerQueryPool<MockDispatchTable> query_pool(&dispatch_table, kNumSlots);
  VkDevice device = {};
  EXPECT_CALL(dispatch_table, CreateQueryPool)
      .WillRepeatedly(Return(dummy_create_query_pool_function));

  // Explicitly expect this call only once, as rolling back should not reset on vulkan side.
  EXPECT_CALL(dispatch_table, ResetQueryPoolEXT)
      .Times(1)
      .WillRepeatedly(Return(dummy_reset_query_pool_function));

  query_pool.InitializeTimerQueryPool(device);
  uint32_t slot;
  (void)query_pool.NextReadyQuerySlot(device, &slot);

  std::vector<uint32_t> reset_slots;
  reset_slots.push_back(slot);

  query_pool.ResetQuerySlots(device, reset_slots, true);
}

TEST(TimerQueryPool, ResettingSlotsDoesResetOnVulkan) {
  MockDispatchTable dispatch_table;
  static constexpr uint32_t kNumSlots = 1;
  TimerQueryPool<MockDispatchTable> query_pool(&dispatch_table, kNumSlots);
  VkDevice device = {};
  EXPECT_CALL(dispatch_table, CreateQueryPool)
      .WillRepeatedly(Return(dummy_create_query_pool_function));

  static uint32_t expected_reset_slot;
  PFN_vkResetQueryPoolEXT mock_reset_query_pool_function =
      +[](VkDevice /*device*/, VkQueryPool /*query_pool*/, uint32_t first_query,
          uint32_t query_count) {
        EXPECT_EQ(expected_reset_slot, first_query);
        EXPECT_EQ(1, query_count);
      };

  // Explicitly expect this call only once, as rolling back should not reset on vulkan side.
  EXPECT_CALL(dispatch_table, ResetQueryPoolEXT)
      .Times(2)
      .WillOnce(Return(dummy_reset_query_pool_function))
      .WillOnce(Return(mock_reset_query_pool_function));

  query_pool.InitializeTimerQueryPool(device);
  (void)query_pool.NextReadyQuerySlot(device, &expected_reset_slot);

  std::vector<uint32_t> reset_slots;
  reset_slots.push_back(expected_reset_slot);

  query_pool.ResetQuerySlots(device, reset_slots, false);
}

TEST(TimerQueryPool, CannotResetReadySlots) {
  MockDispatchTable dispatch_table;
  static constexpr uint32_t kNumSlots = 1;
  TimerQueryPool<MockDispatchTable> query_pool(&dispatch_table, kNumSlots);
  VkDevice device = {};
  EXPECT_CALL(dispatch_table, CreateQueryPool)
      .WillRepeatedly(Return(dummy_create_query_pool_function));
  EXPECT_CALL(dispatch_table, ResetQueryPoolEXT)
      .WillRepeatedly(Return(dummy_reset_query_pool_function));

  query_pool.InitializeTimerQueryPool(device);

  uint32_t first_slot_index = 0;
  std::vector<uint32_t> rollback_slots;
  rollback_slots.push_back(first_slot_index);

  EXPECT_DEATH({ query_pool.ResetQuerySlots(device, rollback_slots, true); }, "");
}

TEST(TimerQueryPool, CanRepeatatleRetrieveAndResetSlots) {
  MockDispatchTable dispatch_table;
  static constexpr uint32_t kNumSlots = 16;
  TimerQueryPool<MockDispatchTable> query_pool(&dispatch_table, kNumSlots);
  VkDevice device = {};
  EXPECT_CALL(dispatch_table, CreateQueryPool)
      .WillRepeatedly(Return(dummy_create_query_pool_function));
  EXPECT_CALL(dispatch_table, ResetQueryPoolEXT)
      .WillRepeatedly(Return(dummy_reset_query_pool_function));

  query_pool.InitializeTimerQueryPool(device);

  for (int i = 0; i < 2000; ++i) {
    uint32_t slot_index;
    bool found_slot = query_pool.NextReadyQuerySlot(device, &slot_index);
    EXPECT_TRUE(found_slot);

    std::vector<uint32_t> reset_slots;
    reset_slots.push_back(slot_index);

    // Check for both, rollback and actual resets.
    query_pool.ResetQuerySlots(device, reset_slots, i % 2 == 0);
  }
}

}  // namespace orbit_vulkan_layer