#include <condition_variable>
#include <mutex>
#include <vector>

#include "queuethread.hpp"
#include "eventhook.hpp"
#include "eventhook_internal.hpp"


namespace boxten{
namespace {
struct InstalledEventhook {
    std::function<void(void)> hook;
    EVENT                     event;
    Component*                component;
    InstalledEventhook(std::function<void(void)> hook, EVENT event, Component* component) : hook(hook), event(event), component(component) {}
};
std::vector<InstalledEventhook> installed_eventhooks;
std::mutex                      installed_eventhooks_lock;

class HookInvoker : public QueueThread<EVENT> {
  private:
    void proc(std::vector<EVENT> queue_to_proc) override {
        std::lock_guard<std::mutex> ilock(installed_eventhooks_lock);
        for(auto e : queue_to_proc) {
            for(auto& i : installed_eventhooks) {
                if(i.event == e) i.hook();
            }
        }
    }

  public:
    ~HookInvoker(){}
};
HookInvoker hook_invoker;
} // namespace

void install_eventhook(std::function<void(void)> hook, EVENT event, Component* component) {
    std::lock_guard<std::mutex> lock(installed_eventhooks_lock);
    installed_eventhooks.emplace_back(hook, event, component);
}
void uninstall_eventhook(Component* component){
    std::lock_guard<std::mutex> lock(installed_eventhooks_lock);
    for(auto i = installed_eventhooks.begin(); i != installed_eventhooks.end();) {
        if(i->component == component) {
            i = installed_eventhooks.erase(i);
        } else {
            ++i;
        }
    }
}
void invoke_eventhook(EVENT event){
    hook_invoker.enqueue(event);
}
void start_hook_invoker(){
    hook_invoker.start();
}
void finish_hook_invoker(){
    hook_invoker.finish();
}
}