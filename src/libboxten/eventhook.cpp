#include <condition_variable>
#include <mutex>
#include <vector>

#include "eventhook.hpp"
#include "eventhook_internal.hpp"
#include "queuethread.hpp"
#include "type.hpp"

namespace boxten{
namespace {
struct InstalledEventhook {
    HookFunction hook;
    Events       event;
    Component*   component;
    InstalledEventhook(HookFunction hook, Events event, Component* component) : hook(hook), event(event), component(component) {}
};
SafeVar<std::vector<InstalledEventhook>> installed_eventhooks;

class HookInvoker : public QueueThread<std::pair<Events, void*>> {
  private:
    void proc(std::vector<std::pair<Events, void*>> queue_to_proc) override {
        std::lock_guard<std::mutex> ilock(installed_eventhooks.lock);
        for(auto e : queue_to_proc) {
            for(auto& i : installed_eventhooks.data) {
                if(i.event == e.first) i.hook(e.first, e.second);
            }
            if(e.second != nullptr){
                delete e.second;
            }
        }
    }

  public:
    ~HookInvoker(){}
};
HookInvoker hook_invoker;
} // namespace

void install_eventhook(HookFunction hook, Events event, Component* component) {
    std::lock_guard<std::mutex> lock(installed_eventhooks.lock);
    installed_eventhooks->emplace_back(hook, event, component);
}
void uninstall_eventhook(Component* component){
    std::lock_guard<std::mutex> lock(installed_eventhooks.lock);
    for(auto i = installed_eventhooks->begin(); i != installed_eventhooks->end();) {
        if(i->component == component) {
            i = installed_eventhooks->erase(i);
        } else {
            ++i;
        }
    }
}
void invoke_eventhook(Events event, void* param){
    hook_invoker.enqueue(std::make_pair(event, param));
}
void start_hook_invoker(){
    hook_invoker.start();
}
void finish_hook_invoker(){
    hook_invoker.finish();
}
}