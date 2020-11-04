// Copyright (c) 2020 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ORBIT_VULKAN_LAYER_DISPATCH_TABLE_H_
#define ORBIT_VULKAN_LAYER_DISPATCH_TABLE_H_

#include "absl/base/casts.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/synchronization/mutex.h"
#include "vulkan/vk_layer.h"
#include "vulkan/vk_layer_dispatch_table.h"
#include "vulkan/vulkan.h"

namespace orbit_vulkan_layer {
/*
 * A thread-safe dispatch table for vulkan function look-up.
 *
 * It computes/stores the vulkan dispatch tables for concrete devices/instances and provides
 * accessors to the functions.
 *
 * Thread-Safety: This class is internally synchronized (using read/write locks) and can be safely
 * accessed from different threads.
 */
class DispatchTable {
 public:
  DispatchTable() = default;
  void CreateInstanceDispatchTable(
      const VkInstance& instance,
      const PFN_vkGetInstanceProcAddr& next_get_instance_proc_addr_function);
  void RemoveInstanceDispatchTable(const VkInstance& instance);
  void CreateDeviceDispatchTable(const VkDevice& device,
                                 const PFN_vkGetDeviceProcAddr& next_get_device_proc_add_function);
  void RemoveDeviceDispatchTable(const VkDevice& device);

  [[nodiscard]] PFN_vkDestroyInstance DestroyInstance(const VkInstance& instance);
  [[nodiscard]] PFN_vkDestroyDevice DestroyDevice(const VkDevice& device);

  [[nodiscard]] PFN_vkEnumerateDeviceExtensionProperties EnumerateDeviceExtensionProperties(
      const VkInstance& instance);
  [[nodiscard]] PFN_vkEnumeratePhysicalDevices EnumeratePhysicalDevices(const VkInstance& instance);
  [[nodiscard]] PFN_vkGetPhysicalDeviceQueueFamilyProperties GetPhysicalDeviceQueueFamilyProperties(
      const VkInstance& instance);
  [[nodiscard]] PFN_vkGetPhysicalDeviceProperties GetPhysicalDeviceProperties(
      const VkInstance& instance);

  [[nodiscard]] PFN_vkGetInstanceProcAddr GetInstanceProcAddr(const VkInstance& instance);
  [[nodiscard]] PFN_vkGetDeviceProcAddr GetDeviceProcAddr(const VkDevice& device);

  [[nodiscard]] PFN_vkCreateCommandPool CreateCommandPool(const VkDevice& device);
  [[nodiscard]] PFN_vkDestroyCommandPool DestroyCommandPool(const VkDevice& device);
  [[nodiscard]] PFN_vkResetCommandPool ResetCommandPool(const VkDevice& device);

  [[nodiscard]] PFN_vkAllocateCommandBuffers AllocateCommandBuffers(const VkDevice& device);
  [[nodiscard]] PFN_vkFreeCommandBuffers FreeCommandBuffers(const VkDevice& device);
  [[nodiscard]] PFN_vkBeginCommandBuffer BeginCommandBuffer(const VkDevice& device);
  [[nodiscard]] PFN_vkEndCommandBuffer EndCommandBuffer(const VkDevice& device);
  [[nodiscard]] PFN_vkResetCommandBuffer ResetCommandBuffer(const VkDevice& device);

  [[nodiscard]] PFN_vkCmdWriteTimestamp CmdWriteTimestamp(const VkDevice& device);

  [[nodiscard]] PFN_vkGetDeviceQueue GetDeviceQueue(const VkDevice& device);
  [[nodiscard]] PFN_vkGetDeviceQueue2 GetDeviceQueue2(const VkDevice& device);
  [[nodiscard]] PFN_vkQueueSubmit QueueSubmit(const VkDevice& device);
  [[nodiscard]] PFN_vkQueuePresentKHR QueuePresentKHR(const VkDevice& device);

  [[nodiscard]] PFN_vkCreateQueryPool CreateQueryPool(const VkDevice& device);
  [[nodiscard]] PFN_vkCmdResetQueryPool CmdResetQueryPool(const VkDevice& device);
  [[nodiscard]] PFN_vkGetQueryPoolResults GetQueryPoolResults(const VkDevice& device);

  [[nodiscard]] PFN_vkCmdPipelineBarrier CmdPipelineBarrier(const VkDevice& device);

  [[nodiscard]] PFN_vkCreateFence CreateFence(const VkDevice& device);
  [[nodiscard]] PFN_vkDestroyFence DestroyFence(const VkDevice& device);
  [[nodiscard]] PFN_vkGetFenceStatus GetFenceStatus(const VkDevice& device);

  [[nodiscard]] PFN_vkCreateEvent CreateEvent(const VkDevice& device);
  [[nodiscard]] PFN_vkDestroyEvent DestroyEvent(const VkDevice& device);
  [[nodiscard]] PFN_vkSetEvent SetEvent(const VkDevice& device);
  [[nodiscard]] PFN_vkGetEventStatus GetEventStatus(const VkDevice& device);
  [[nodiscard]] PFN_vkCmdWaitEvents CmdWaitEvents(const VkDevice& device);
  [[nodiscard]] PFN_vkCmdSetEvent CmdSetEvent(const VkDevice& device);

 private:
  // Dispatch tables required for routing instance and device calls onto the next
  // layer in the dispatch chain among our handling of functions we intercept.
  absl::flat_hash_map<void*, VkLayerInstanceDispatchTable> instance_dispatch_table_;
  absl::flat_hash_map<void*, VkLayerDispatchTable> device_dispatch_table_;

  // Must protect access to dispatch tables above by mutex since the Vulkan
  // application may be calling these functions from different threads.
  // However, they are usually filled once (per device/instance) at the beginning
  // and afterwards we only read that data. So we use a read/write lock.
  absl::Mutex mutex_;
};

}  // namespace orbit_vulkan_layer

#endif  // ORBIT_VULKAN_LAYER_DISPATCH_TABLE_H_
