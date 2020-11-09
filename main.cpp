#include <iostream>
#include "task_queue.h"
#include <memory>
#include "event.h"

using namespace std;

int main()
{
    cout << "Hello World!" << endl;

    std::unique_ptr<vi::TaskQueue> tq = vi::TaskQueue::create("test");
    vi::Event ev;
    tq->postTask(vi::ToQueuedTask([&ev](){
        for (int i = 0; i < 1000; ++i) {
            cout << "Hello World..." << endl;
        }
        ev.set();
    }));
    ev.wait(-1);
    return 0;
}
