#include <iostream>
#include "task_queue.h"
#include <memory>
#include <thread>
#include "event.h"

using namespace std;

int main()
{
    cout << "Hello World!" << endl;

    std::unique_ptr<vi::TaskQueue> tq = vi::TaskQueue::create("test");
    vi::Event ev;

    const int N = 12;
    std::thread threads[N];
    for (int n = 0; n < N; ++n) {
        threads[n] = std::thread([&ev, &tq]{
            for (int i = 0; i < 10000; ++i) {
                tq->postTask([&ev, i](){
                    cout << "Hello World..." << ", i = " << i << endl;
                });
                tq->postDelayedTask([&ev, i](){
                    cout << "delayed task" << ", i = " << i << endl;
                    if (i == 9999) {
                        ev.set();
                    }
                }, 1000);
            }
        });
    }

    ev.wait(-1);
    for (int n = 0; n < N; ++n) {
        threads[n].join();
    }

    return 0;
}
