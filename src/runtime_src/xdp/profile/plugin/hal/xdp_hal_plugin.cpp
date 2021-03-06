/**
 * Copyright (C) 2016-2020 Xilinx, Inc
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You may
 * not use this file except in compliance with the License. A copy of the
 * License is located at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */

#include <iostream>

#define XDP_SOURCE

#include "xdp_hal_plugin_interface.h"
#include "xdp_hal_plugin.h"

#include "xdp/profile/database/database.h"
#include "xdp/profile/database/events/hal_api_calls.h"
#include "core/common/time.h"

#include "hal_plugin.h"

namespace xdp {

  // This object is created when the plugin library is loaded
  static HALPlugin halPluginInstance ;

  static void log_function_start(void* payload, const char* functionName)
  {
    double timestamp = xrt_core::time_ns() ;

    CBPayload* decoded = reinterpret_cast<CBPayload*>(payload) ;
    VPDatabase* db = halPluginInstance.getDatabase() ;

    // Update counters
    (db->getStats()).logFunctionCallStart(functionName, timestamp) ;

    // Update trace
    VTFEvent* event = new HALAPICall(0,
				     timestamp,
				     decoded->idcode,
				     (db->getDynamicInfo()).addString(functionName));
    (db->getDynamicInfo()).addEvent(event) ;
    (db->getDynamicInfo()).markStart(decoded->idcode, event->getEventId()) ;
    return;
  }

  static void log_function_end(void* payload, const char* functionName)
  {
    double timestamp = xrt_core::time_ns() ;

    CBPayload* decoded = reinterpret_cast<CBPayload*>(payload) ;
    VPDatabase* db = halPluginInstance.getDatabase() ;
  
    // Update counters
    (db->getStats()).logFunctionCallEnd(functionName, timestamp) ;

    // Update trace
    VTFEvent* event = new HALAPICall((db->getDynamicInfo()).matchingStart(decoded->idcode),
				     timestamp,
				     decoded->idcode,
				     (db->getDynamicInfo()).addString(functionName));
    (db->getDynamicInfo()).addEvent(event) ;
    return;
  }

  static void alloc_bo_start(void* payload) {  
    log_function_start(payload, "AllocBO") ;
  }

  static void alloc_bo_end(void* payload) {
    log_function_end(payload, "AllocBO") ;
  }

  static void free_bo_start(void* payload) {
    log_function_start(payload, "FreeBO") ;
  }

  static void free_bo_end(void* payload) {
    log_function_end(payload, "FreeBO") ;
  }

  static void write_bo_start(void* payload) {
    log_function_start(payload, "WriteBO") ;
  }

  static void write_bo_end(void* payload) {
    log_function_end(payload, "WriteBO") ;
  }

  static void read_bo_start(void* payload) {
    BOTransferCBPayload* pLoad = 
      reinterpret_cast<BOTransferCBPayload*>(payload);

    log_function_start(&(pLoad->basePayload), "ReadBO") ;

    // Also log the amount of data transferred
    VPDatabase* db = halPluginInstance.getDatabase() ;
    (db->getStats()).logMemoryTransfer(pLoad->basePayload.deviceHandle,
				       DeviceMemoryStatistics::BUFFER_READ,
				       pLoad->size) ;
  }

  static void read_bo_end(void* payload) {
    log_function_end(payload, "ReadBO") ;
  }

  static void map_bo_start(void* payload) {
    log_function_start(payload, "MapBO") ;
  }

  static void map_bo_end(void* payload) {
    log_function_end(payload, "MapBO") ;
  }

  static void sync_bo_start(void* payload) {
    log_function_start(payload, "SyncBO") ;
  }

  static void sync_bo_end(void* payload) {
    log_function_end(payload, "SyncBO") ;
  }

  static void unmgd_read_start(void* payload) {
    UnmgdPreadPwriteCBPayload* pLoad = 
      reinterpret_cast<UnmgdPreadPwriteCBPayload*>(payload);
    log_function_start(&(pLoad->basePayload), "UnmgdRead") ;
    
    // Also log the amount of data transferred
    VPDatabase* db = halPluginInstance.getDatabase() ;
    (db->getStats()).logMemoryTransfer(pLoad->basePayload.deviceHandle,
				       DeviceMemoryStatistics::UNMANAGED_READ, 
				       pLoad->count) ;
  }

  static void unmgd_read_end(void* payload) {
    UnmgdPreadPwriteCBPayload* pLoad = 
      reinterpret_cast<UnmgdPreadPwriteCBPayload*>(payload) ;
    log_function_end(&(pLoad->basePayload), "UnmgdRead") ;
  }

  static void unmgd_write_start(void* payload) {
    UnmgdPreadPwriteCBPayload* pLoad = 
      reinterpret_cast<UnmgdPreadPwriteCBPayload*>(payload);
    log_function_start(&(pLoad->basePayload), "UnmgdWrite") ;

    // Also log the amount of data transferred
    VPDatabase* db = halPluginInstance.getDatabase() ;
    (db->getStats()).logMemoryTransfer(pLoad->basePayload.deviceHandle,
				       DeviceMemoryStatistics::UNMANAGED_WRITE,
				       pLoad->count) ;
  }

  static void unmgd_write_end(void* payload) {
    UnmgdPreadPwriteCBPayload* pLoad = 
      reinterpret_cast<UnmgdPreadPwriteCBPayload*>(payload) ;
    log_function_end(&(pLoad->basePayload), "UnmgdWrite") ;
  }

  static void read_start(void* payload) {
    ReadWriteCBPayload* pLoad = reinterpret_cast<ReadWriteCBPayload*>(payload);
    log_function_start(&(pLoad->basePayload), "xclRead") ;

    // Also log the amount of data transferred
    VPDatabase* db = halPluginInstance.getDatabase() ;
    (db->getStats()).logMemoryTransfer(pLoad->basePayload.deviceHandle,
				       DeviceMemoryStatistics::XCLREAD, 
				       pLoad->size) ;
  }

  static void read_end(void* payload) {
    ReadWriteCBPayload* pLoad = reinterpret_cast<ReadWriteCBPayload*>(payload);
    log_function_end(&(pLoad->basePayload), "xclRead") ;
  }

  static void write_start(void* payload) {
    ReadWriteCBPayload* pLoad = reinterpret_cast<ReadWriteCBPayload*>(payload);
    log_function_start(&(pLoad->basePayload), "xclWrite") ;

    // Also log the amount of data transferred
    VPDatabase* db = halPluginInstance.getDatabase() ;
    (db->getStats()).logMemoryTransfer(pLoad->basePayload.deviceHandle,
				       DeviceMemoryStatistics::XCLWRITE, 
				       pLoad->size) ;
  }

  static void write_end(void* payload) {
    ReadWriteCBPayload* pLoad = reinterpret_cast<ReadWriteCBPayload*>(payload);
    log_function_end(&(pLoad->basePayload), "xclWrite") ;
  }

  static void load_xclbin_start(void* payload) {
    // The xclbin is about to be loaded, so flush any device information
    //  into the database
    XclbinCBPayload* pLoad = reinterpret_cast<XclbinCBPayload*>(payload) ;

    // Before we load a new xclbin, make sure we read all of the 
    //  device data into the database.
    halPluginInstance.readDeviceInfo((pLoad->basePayload).deviceHandle) ;
    halPluginInstance.flushDeviceInfo((pLoad->basePayload).deviceHandle) ;
  }

  static void load_xclbin_end(void* payload) {
    // The xclbin has been loaded, so update all the static information
    //  in our database.
    XclbinCBPayload* pLoad = reinterpret_cast<XclbinCBPayload*>(payload) ;
    VPDatabase* db = halPluginInstance.getDatabase() ;

    (db->getStaticInfo()).updateDevice((pLoad->basePayload).deviceHandle,
				       pLoad->binary) ;
  }

  static void unknown_cb_type(void* /*payload*/) {
    return;
  }

} //  xdp

void hal_level_xdp_cb_func(HalCallbackType cb_type, void* payload)
{
  switch (cb_type) {
    case HalCallbackType::ALLOC_BO_START:
      xdp::alloc_bo_start(payload);
      break;
    case HalCallbackType::ALLOC_BO_END:
      xdp::alloc_bo_end(payload);
      break;
    case HalCallbackType::FREE_BO_START:
      xdp::free_bo_start(payload);
      break;
    case HalCallbackType::FREE_BO_END:
      xdp::free_bo_end(payload);
      break;
    case HalCallbackType::WRITE_BO_START:
      xdp::write_bo_start(payload);
      break;
    case HalCallbackType::WRITE_BO_END:
      xdp::write_bo_end(payload);
      break;
    case HalCallbackType::READ_BO_START:
      xdp::read_bo_start(payload);
      break;
    case HalCallbackType::READ_BO_END:
      xdp::read_bo_end(payload);
      break;
    case HalCallbackType::MAP_BO_START:
      xdp::map_bo_start(payload);
      break;
    case HalCallbackType::MAP_BO_END:
      xdp::map_bo_end(payload);
      break;
    case HalCallbackType::SYNC_BO_START:
      xdp::sync_bo_start(payload);
      break;
    case HalCallbackType::SYNC_BO_END:
      xdp::sync_bo_end(payload);
      break;
    case HalCallbackType::UNMGD_READ_START:
      xdp::unmgd_read_start(payload);
      break;
    case HalCallbackType::UNMGD_READ_END:
      xdp::unmgd_read_end(payload);
      break;
    case HalCallbackType::UNMGD_WRITE_START:
      xdp::unmgd_write_start(payload);
      break;
    case HalCallbackType::UNMGD_WRITE_END:
      xdp::unmgd_write_end(payload);
      break;
    case HalCallbackType::READ_START:
      xdp::read_start(payload);
      break;
    case HalCallbackType::READ_END:
      xdp::read_end(payload);
      break;
    case HalCallbackType::WRITE_START:
      xdp::write_start(payload);
      break;
    case HalCallbackType::WRITE_END:
      xdp::write_end(payload);
      break;
    case HalCallbackType::LOAD_XCLBIN_START:
      xdp::load_xclbin_start(payload) ;
      break ;
    case HalCallbackType::LOAD_XCLBIN_END:
      xdp::load_xclbin_end(payload) ;
      break ;
    default: 
      xdp::unknown_cb_type(payload);
      break;
  }
  return;
}
