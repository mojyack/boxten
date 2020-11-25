/* This is an internal header, which will not be installed. */
#pragma once
#include "eventhook.hpp"

namespace boxten{
class Component;
void install_eventhook(HookFunction hook, Events event, Component* component);
void uninstall_eventhook(Component* component);
void invoke_eventhook(Events event, void* param = nullptr);
void start_hook_invoker();
void finish_hook_invoker();
}