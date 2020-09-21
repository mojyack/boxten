/* This is an internal header, which will not be installed. */
#pragma once
#include "eventhook.hpp"

namespace boxten{
class Component;
void install_eventhook(std::function<void(void)> hook, EVENT event, Component* component);
void uninstall_eventhook(Component* component);
void invoke_eventhook(EVENT event);
void start_hook_invoker();
void finish_hook_invoker();
}