#include <iostream>
#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <unistd.h>

#include "./graph/include/graph/Graph.h"

using namespace std;

mutex g_mtx;

typedef void (*fn_ptr)(int);
void printer(int x) {
    cout << "Task with ID: " << x << " is executing..." << endl;
    usleep(5000 * x);
    cout << "Task " << x << " finished." << endl;
}

struct Task {
   fn_ptr to_execute;
   int dep_completed; // number of completed dependencies
};

vector<Task> g_node_data;
graph::Graph<int> g_graph;

bool cycle_dfs(int node, vector<bool> &visited, vector<bool> &rec_stack) {
    if (visited[node] == false) {
        visited[node] = true;
        rec_stack[node] = true;

        for (auto &next_node: g_graph.get_children(node)) {
            if (!visited[next_node] && cycle_dfs(next_node, visited, rec_stack)) return true;
            else if (rec_stack[next_node]) return true;
        }
    }

    rec_stack[node] = false;
    return false;
}

bool check_cycles(int nodes) {
    vector<bool> visited(nodes + 1, false);
    vector<bool> rec_stack(nodes + 1, false);

    for (int i = 1; i <= nodes; i++) {
        if (cycle_dfs(i, visited, rec_stack)) return true;
    }

    return false;
}


queue<int> g_run_queue;
int g_completed;
int g_running;
const int MAX_RUNNING = 4;

void wrapper(int x) {
    printer(x);
    g_mtx.lock();
    g_completed++;
    g_running--;

    for (auto &next_node: g_graph.get_children(x)) {
        g_node_data[next_node].dep_completed++;
        if (g_node_data[next_node].dep_completed == (int) g_graph.get_parents(next_node).size()) {
           g_run_queue.push(next_node); 
        }
    }
    g_mtx.unlock();
}

void run_bfs(vector<int> &roots, int nodes) {
    for (auto &i: roots) {
        g_run_queue.push(i);
    }

    g_completed = 0;
    while (true) {
       g_mtx.lock();
       if (g_completed == nodes) {
            g_mtx.unlock();
            break;
       }

       while (!g_run_queue.empty() && g_running < MAX_RUNNING) {
            int node = g_run_queue.front();
            g_run_queue.pop();
            g_running++;

            cout << "Spawn thread " << node << endl;
            thread new_thread(g_node_data[node].to_execute, node);
            new_thread.detach();
            //g_node_data[node].to_execute(node);
       }
       g_mtx.unlock();
       usleep(100);
    }

    cout << "Finished executing!" << endl;
}

int main() {
    g_node_data.reserve(12);
    for (int i = 1; i <= 11; i++) {
        g_node_data[i] = {wrapper, 0}; 
    }
    g_graph.add_edge(1, 4);
    g_graph.add_edge(1, 5);
    g_graph.add_edge(2, 5);
    g_graph.add_edge(2, 6);
    g_graph.add_edge(6, 9);
    g_graph.add_edge(9, 11);
    g_graph.add_edge(3, 7);
    g_graph.add_edge(7, 9);
    g_graph.add_edge(3, 8);
    g_graph.add_edge(8, 10);
    g_graph.add_edge(10, 11);
    //g_graph.add_edge(11, 2); uncomment this to get a cycle

    bool cycles = check_cycles(11);
    if (cycles) {
        cerr << "The tasks graph must not contain any cycles!" << endl;
        return -1;
    }

    vector<int> roots;
    for (int i = 1; i <= 11; i++) {
        if (g_graph.get_parents(i).size() == 0) {
            roots.push_back(i);
        }
    }

    run_bfs(roots, 11);

    return 0;
}
