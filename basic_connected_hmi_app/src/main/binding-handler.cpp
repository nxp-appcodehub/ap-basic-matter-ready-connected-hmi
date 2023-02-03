/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/*
 *  Copyright 2023 NXP
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 */

#include "binding-handler.h"
#include "app/clusters/bindings/BindingManager.h"
#include "app/server/Server.h"
#include "controller/InvokeInteraction.h"
#include "controller/ReadInteraction.h"
#include "platform/CHIPDeviceLayer.h"
#include <app/clusters/bindings/bindings.h>
#include <lib/support/CodeUtils.h>
#include "app_config.h"
#include "display_app.h"

#if defined(ENABLE_CHIP_SHELL)
#include "lib/shell/Engine.h"

using chip::Shell::Engine;
using chip::Shell::shell_command_t;
using chip::Shell::streamer_get;
using chip::Shell::streamer_printf;
#endif // defined(ENABLE_CHIP_SHELL)

using namespace chip;
using namespace chip::app;

namespace {

static int isSubscribed[]={0,0,0,0,0,0,0,0,0,0};
CHIP_ERROR ret[10];

static bool sSwitchOnOffState = false;
#if defined(ENABLE_CHIP_SHELL)
static void ToggleSwitchOnOff(bool newState)
{
    sSwitchOnOffState = newState;
    chip::BindingManager::GetInstance().NotifyBoundClusterChanged(1, chip::app::Clusters::OnOff::Id, nullptr);
}

static CHIP_ERROR SwitchCommandHandler(int argc, char ** argv)
{
    if (argc == 1 && strcmp(argv[0], "on") == 0)
    {
        ToggleSwitchOnOff(true);
        return CHIP_NO_ERROR;
    }
    if (argc == 1 && strcmp(argv[0], "off") == 0)
    {
        ToggleSwitchOnOff(false);
        return CHIP_NO_ERROR;
    }
    streamer_printf(streamer_get(), "Usage: switch [on|off]");
    return CHIP_NO_ERROR;
}

static void RegisterSwitchCommands()
{
    static const shell_command_t sSwitchCommand = { SwitchCommandHandler, "switch", "Switch commands. Usage: switch [on|off]" };
    Engine::Root().RegisterCommands(&sSwitchCommand, 1);
    return;
}
#endif // defined(ENABLE_CHIP_SHELL)

void ProcessOnOffUnicastSubscribeLight(const EmberBindingTableEntry & binding, OperationalDeviceProxy * peer_device, uint8_t device)
{
    auto onSuccess = [device](const app::ConcreteDataAttributePath & attributePath, const auto & dataResponse) {
        ChipLogProgress(NotSpecified, "OnOff report received");
#if (defined(CHIP_DEVICE_CONFIG_ENABLE_DISPLAY) && (CHIP_DEVICE_CONFIG_ENABLE_DISPLAY > 0U))
        updateOnOffState(dataResponse, device-1);
#endif
    };

    auto onFailure = [](const app::ConcreteDataAttributePath * attributePath, CHIP_ERROR aError) {
        ChipLogError(NotSpecified, "OnOff report failed: %" CHIP_ERROR_FORMAT, aError.Format());
    };
    
    
    auto onSubscriptionEstablishedCb = [](const app::ReadClient & readClient) {
        ChipLogError(NotSpecified, "Subscription established");
    };

    auto onResubscriptionAttemptCb = [](const app::ReadClient & readClient, CHIP_ERROR aError, uint32_t aNextResubscribeIntervalMsec) {
        ChipLogError(NotSpecified, "Resubscription Attempt: %" CHIP_ERROR_FORMAT, aError.Format());
        ChipLogError(NotSpecified, "Next resubscribe interval: %ld", aNextResubscribeIntervalMsec);
    };

   
    ret[device] = Controller::SubscribeAttribute<bool>(peer_device->GetExchangeManager(), peer_device->GetSecureSession().Value(), 0XFFFF, 0x0006 , 0x0000,
    													     onSuccess, onFailure, 0, 600, onSubscriptionEstablishedCb, onResubscriptionAttemptCb,
                                                             false, true);
}

void ProcessOnOffUnicastBindingCommand(const EmberBindingTableEntry & binding, OperationalDeviceProxy * peer_device)
{
    auto Success = [](const ConcreteCommandPath & commandPath, const StatusIB & status, const auto & dataResponse) {
        ChipLogProgress(NotSpecified, "OnOff command succeeds");
    };

    auto Failure = [](CHIP_ERROR error) {
        ChipLogError(NotSpecified, "OnOff command failed: %" CHIP_ERROR_FORMAT, error.Format());
    };

    Clusters::OnOff::Commands::Toggle::Type toggleCommand;
    Controller::InvokeCommandRequest(peer_device->GetExchangeManager(), peer_device->GetSecureSession().Value(), binding.remote, toggleCommand, Success, Failure);
}


void GetDeviceInfo(const EmberBindingTableEntry & binding, DeviceProxy * peer_device, uint8_t device)
{
	auto Success = [device](const app::ConcreteDataAttributePath & attributePath, const auto & dataResponse) {
        	ChipLogProgress(NotSpecified, "Getting device info");
#if (defined(CHIP_DEVICE_CONFIG_ENABLE_DISPLAY) && (CHIP_DEVICE_CONFIG_ENABLE_DISPLAY > 0U))
        	updateConnectionStatus(device-1, true);
        	auto iter_value_0 = dataResponse.begin();
        	while (iter_value_0.Next())
        	{
        		auto & entry_0 = iter_value_0.GetValue();
        		ChipLogProgress(NotSpecified, "Netif type is: %d", static_cast<uint8_t>(entry_0.type));
        		updateNetworkType(device-1, static_cast<uint8_t>(entry_0.type));
        	}
#endif
    	};

	auto Failure = [device](const app::ConcreteDataAttributePath * attributePath, CHIP_ERROR aError) {
        	ChipLogError(NotSpecified, "Could not retrieve device info : %" CHIP_ERROR_FORMAT, aError.Format());
#if (defined(CHIP_DEVICE_CONFIG_ENABLE_DISPLAY) && (CHIP_DEVICE_CONFIG_ENABLE_DISPLAY > 0U))
        	updateConnectionStatus(device-1, false);
#endif
    	};
	
	ret[device] = Controller::ReadAttribute<chip::app::Clusters::GeneralDiagnostics::Attributes::NetworkInterfaces::TypeInfo::DecodableType>(peer_device->GetExchangeManager(), 
																		 peer_device->GetSecureSession().Value(), 
																		 0, 0x0033, 0x0000,Success, Failure);
}

/*void ProcessThermostatUnicastBindingCommand(const EmberBindingTableEntry & binding, DeviceProxy * peer_device)
{

    auto Success = [](const app::ConcreteDataAttributePath & attributePath, const auto & dataResponse) {
        ChipLogProgress(NotSpecified, "Temperature changed report received");
        ChipLogProgress(NotSpecified, "Thermostat temperature is: %d", dataResponse);
#if (defined(CHIP_DEVICE_CONFIG_ENABLE_DISPLAY) && (CHIP_DEVICE_CONFIG_ENABLE_DISPLAY > 0U))
        updateTemperatureValue(dataResponse);
#endif
    };

    auto Failure = [](const app::ConcreteDataAttributePath * attributePath, CHIP_ERROR aError) {
        ChipLogError(NotSpecified, "Temperature changed report failed: %" CHIP_ERROR_FORMAT, aError.Format());
    };
    
    auto onSuccess = [](const app::ConcreteDataAttributePath & attributePath, const auto & dataResponse) {
        ChipLogProgress(NotSpecified, "Temperature changed report received");
        ChipLogProgress(NotSpecified, "Thermostat temperature is: %d", dataResponse);
#if (defined(CHIP_DEVICE_CONFIG_ENABLE_DISPLAY) && (CHIP_DEVICE_CONFIG_ENABLE_DISPLAY > 0U))
        updateTemperatureValue(dataResponse);
#endif
    };

    auto onFailure = [](const app::ConcreteDataAttributePath * attributePath, CHIP_ERROR aError) {
        ChipLogError(NotSpecified, "Temperature changed report failed: %" CHIP_ERROR_FORMAT, aError.Format());
    };
    
    
    auto onSubscriptionEstablishedCb = [](const app::ReadClient & readClient) {
        ChipLogError(NotSpecified, "Subscription established");
#if (defined(CHIP_DEVICE_CONFIG_ENABLE_DISPLAY) && (CHIP_DEVICE_CONFIG_ENABLE_DISPLAY > 0U))
        updateThermometerState(true);
#endif
    };

    auto onResubscriptionAttemptCb = [](const app::ReadClient & readClient, CHIP_ERROR aError, uint32_t aNextResubscribeIntervalMsec) {
        ChipLogError(NotSpecified, "Resubscription Attempt: %" CHIP_ERROR_FORMAT, aError.Format());
        ChipLogError(NotSpecified, "Next resubscribe interval: %ld", aNextResubscribeIntervalMsec);
    };

    CHIP_ERROR ret = CHIP_NO_ERROR;
    
    
    ret = Controller::ReadAttribute<int16_t>(peer_device->GetExchangeManager(), peer_device->GetSecureSession().Value(), 1, 0x0201, 0x0000,
    													     Success, Failure);
    
    ret = Controller::SubscribeAttribute<int16_t>(peer_device->GetExchangeManager(), peer_device->GetSecureSession().Value(), 1, 0x0201 , 0x0000,
    													     onSuccess, onFailure, 0, 10, onSubscriptionEstablishedCb, onResubscriptionAttemptCb,
                                                             false, true);

}*/

void ProcessOnOffGroupBindingCommand(CommandId commandId, const EmberBindingTableEntry & binding)
{
    Messaging::ExchangeManager & exchangeMgr = Server::GetInstance().GetExchangeManager();

    switch (commandId)
    {
    case Clusters::OnOff::Commands::Toggle::Id:
        Clusters::OnOff::Commands::Toggle::Type toggleCommand;
        Controller::InvokeGroupCommandRequest(&exchangeMgr, binding.fabricIndex, binding.groupId, toggleCommand);
        break;

    case Clusters::OnOff::Commands::On::Id:
        Clusters::OnOff::Commands::On::Type onCommand;
        Controller::InvokeGroupCommandRequest(&exchangeMgr, binding.fabricIndex, binding.groupId, onCommand);

        break;

    case Clusters::OnOff::Commands::Off::Id:
        Clusters::OnOff::Commands::Off::Type offCommand;
        Controller::InvokeGroupCommandRequest(&exchangeMgr, binding.fabricIndex, binding.groupId, offCommand);
        break;
    }
}

void ControllerChangedHandler(const EmberBindingTableEntry & binding, OperationalDeviceProxy * peer_device, void * context)
{
    VerifyOrReturn(context != nullptr, ChipLogError(NotSpecified, "OnDeviceConnectedFn: context is null"));
    BindingCommandData * data = static_cast<BindingCommandData *>(context);
    
    const EmberBindingTableEntry & entry = BindingTable::GetInstance().GetAt(data->NodeId-1);
    
    if( entry.nodeId == peer_device->GetDeviceId())
    {
    	if (binding.type == EMBER_MULTICAST_BINDING && data->isGroup)
	{
		switch (data->clusterId)
		{
		case Clusters::OnOff::Id:
		    ProcessOnOffGroupBindingCommand(data->commandId, binding);
		    break;
		}
	}
	else if (binding.type == EMBER_UNICAST_BINDING && !data->isGroup)
	{
		switch (data->clusterId)
		{
		case Clusters::OnOff::Id:
		    if(data->NodeId > BindingTable::GetInstance().Size())
		    {
		    	ChipLogError(NotSpecified, "Device not bound ! Bind a device to this button and try again.");
		    }
		    else
		    {
			    if( isSubscribed[data->NodeId-1] == 0 )
			    {
			    	isSubscribed[data->NodeId-1] = 1;
			    	GetDeviceInfo(BindingTable::GetInstance().GetAt(data->NodeId-1), peer_device, data->NodeId);
			    	ProcessOnOffUnicastSubscribeLight(BindingTable::GetInstance().GetAt(data->NodeId-1), peer_device, data->NodeId);
			    	ProcessOnOffUnicastBindingCommand(BindingTable::GetInstance().GetAt(data->NodeId-1), peer_device);
			    }
			    else
			    {
			    	ProcessOnOffUnicastBindingCommand(BindingTable::GetInstance().GetAt(data->NodeId-1), peer_device);
			    }
		    }
		    break;

		case Clusters::Thermostat::Id:
		    //ProcessThermostatUnicastBindingCommand(binding, peer_device);
		    break;      
		}
		
	}
    }
}

void ControllerContextReleaseHandler(void * context)
{
    VerifyOrReturn(context != nullptr, ChipLogError(NotSpecified, "ControllerContextReleaseHandler: context is null"));

    Platform::Delete(static_cast<BindingCommandData *>(context));
}


void InitBindingHandlerInternal(intptr_t arg)
{
    auto & server = chip::Server::GetInstance();
    chip::BindingManager::GetInstance().Init(
        { &server.GetFabricTable(), server.GetCASESessionManager(), &server.GetPersistentStorage() });
    chip::BindingManager::GetInstance().RegisterBoundDeviceChangedHandler(ControllerChangedHandler);
    chip::BindingManager::GetInstance().RegisterBoundDeviceContextReleaseHandler(ControllerContextReleaseHandler);
}


} // namespace


/********************************************************
 * Controller functions
 *********************************************************/

void ControllerWorkerFunction(intptr_t context)
{
    VerifyOrReturn(context != 0, ChipLogError(NotSpecified, "ControllerWorkerFunction - Invalid work data"));

    BindingCommandData * data = reinterpret_cast<BindingCommandData *>(context);
    BindingManager::GetInstance().NotifyBoundClusterChanged(data->localEndpointId, data->clusterId, static_cast<void *>(data));
}

void BindingWorkerFunction(intptr_t context)
{
    VerifyOrReturn(context != 0, ChipLogError(NotSpecified, "BindingWorkerFunction - Invalid work data"));

    EmberBindingTableEntry * entry = reinterpret_cast<EmberBindingTableEntry *>(context);
    AddBindingEntry(*entry);

    Platform::Delete(entry);
}

CHIP_ERROR InitBindingHandlers()
{
    // The initialization of binding manager will try establishing connection with unicast peers
    // so it requires the Server instance to be correctly initialized. Post the init function to
    // the event queue so that everything is ready when initialization is conducted.
    // TODO: Fix initialization order issue in Matter server.
    chip::DeviceLayer::PlatformMgr().ScheduleWork(InitBindingHandlerInternal);
#if defined(ENABLE_CHIP_SHELL)
    RegisterSwitchCommands();
#endif
    return CHIP_NO_ERROR;
}
