/**
 * SysPilot: Automated Unit Test for CPU Scheduling Engine
 * tests/test_scheduler.c
 */

#include "syspilot.h"
#include <assert.h>

void test_fcfs(void) {
    SchedProcess procs[] = {
        {1, "P1", 0, 5, 5, 1, 0, -1, 0, 0, 0, -1, 0.0, 0, false},
        {2, "P2", 1, 3, 3, 2, 0, -1, 0, 0, 0, -1, 0.0, 0, false},
        {3, "P3", 2, 1, 1, 3, 0, -1, 0, 0, 0, -1, 0.0, 0, false}
    };
    SchedEvaluationReport rep;
    int res = schedule_simulate(ALGO_FCFS, procs, 3, 2, &rep);
    assert(res == 0);
    assert(rep.total_simulation_time == 9);
    assert(rep.processes[0].completion_time == 5); /* P1 completes at 5 */
    assert(rep.processes[1].completion_time == 8); /* P2 completes at 8 */
    assert(rep.processes[2].completion_time == 9); /* P3 completes at 9 */
    printf("[PASS] FCFS Unit Test Verified.\n");
}

void test_rr(void) {
    SchedProcess procs[] = {
        {1, "P1", 0, 4, 4, 1, 0, -1, 0, 0, 0, -1, 0.0, 0, false},
        {2, "P2", 0, 2, 2, 1, 0, -1, 0, 0, 0, -1, 0.0, 0, false}
    };
    SchedEvaluationReport rep;
    int res = schedule_simulate(ALGO_ROUND_ROBIN, procs, 2, 2, &rep);
    assert(res == 0);
    assert(rep.total_simulation_time == 6);
    /* P1 runs 0-2, P2 runs 2-4 (completes), P1 runs 4-6 (completes) */
    assert(rep.processes[1].completion_time == 4);
    assert(rep.processes[0].completion_time == 6);
    printf("[PASS] Round Robin Unit Test Verified.\n");
}

int main(void) {
    printf("Running SysPilot Scheduling Engine Unit Tests...\n");
    test_fcfs();
    test_rr();
    printf("All C Unit Tests Passed Successfully!\n");
    return 0;
}
