# Linux CFS Scheduler Enhancement using Custom System Calls

## Overview

This project modifies the Linux Completely Fair Scheduler (CFS) to support workload-aware CPU scheduling through custom user-space scheduling hints.

A new system call, `sys_set_sched_hint`, was implemented in Linux kernel `v5.19.8` to allow processes to dynamically communicate execution behavior to the kernel scheduler.

The scheduler was modified to prioritize latency-sensitive workloads and penalize CPU-bound tasks by dynamically scaling virtual runtime (`vruntime`) accumulation inside the CFS scheduling logic.

---

## Features

- Custom Linux system call implementation
- Kernel-level modification of the Completely Fair Scheduler (CFS)
- Dynamic workload-aware scheduling
- User-space scheduling hints
- `vruntime` scaling optimization
- Benchmarking under forced CPU contention
- Custom kernel compilation and boot configuration

---

## Scheduling Hints

| Hint                     |Description                 |
|--------------------------|----------------------------|
| `HINT_NONE`              | Default scheduler behavior |
| `HINT_LATENCY_SENSITIVE` | Prioritized scheduling     |
| `HINT_CPU_BOUND`         | Penalized CPU usage        |

---

## System Call

```c
sys_set_sched_hint(pid_t pid, int hint)
```

This syscall allows a process to dynamically inform the scheduler about its workload profile.

---

## Kernel Modifications

The following Linux kernel components were modified:

| File                                                      | Purpose                                          |
|-----------------------------------------------------------|--------------------------------------------------|
| `include/linux/sched.h`                                   | Added scheduling hint field to `task_struct`     |
| `kernel/sched/core.c`                                     | Initialized scheduling hint during task creation |
| `include/linux/syscalls.h`                                | Added syscall prototype                          |
| `arch/x86/entry/syscalls/syscall_64.tbl`                  | Added syscall table entry                        |
| `kernel/sys.c`                                            | Implemented syscall logic                        |
| `kernel/sched/fair.c`                                     | Modified `vruntime` scaling logic                |

---

## Scheduling Logic

The CFS scheduler was modified inside `update_curr()` to dynamically scale virtual runtime accumulation.

### Behavior

- Latency-sensitive tasks accumulate `vruntime` more slowly
- CPU-bound tasks accumulate `vruntime` more quickly
- Default tasks preserve standard CFS behavior

This allows workload-aware scheduling while maintaining overall fairness guarantees.

### Modified Logic

```c
u64 scaled_delta = delta_exec;

/* Ensure dealing with a standard process task */
if (entity_is_task(curr)) {

    /* HINT_CPU_BOUND: vruntime grows 10% faster */
    if (p->sched_hint == 1)
        scaled_delta = (delta_exec * 110) / 100;

    /* HINT_LATENCY_SENSITIVE: vruntime grows 10% slower */
    else if (p->sched_hint == 3)
        scaled_delta = (delta_exec * 90) / 100;
}

curr->vruntime += calc_delta_fair(scaled_delta, curr);
```

---

## Benchmarking

Processes were pinned to the same CPU core to force contention using:

```c
sched_setaffinity()
```

Three workload types were benchmarked simultaneously:

- Latency-sensitive
- Default
- CPU-bound

### Benchmark Program

```bash
gcc -O0 benchmark_hints.c -o benchmark_hints
./benchmark_hints
```

---

## Results

### Default CFS Behavior

Without scheduling hints:

- All processes receive approximately equal CPU time
- Runtime remains close across workloads
- Completion order varies non-deterministically

### Modified Scheduler Behavior

With scheduling hints enabled:

- Latency-sensitive tasks complete significantly faster
- CPU-bound tasks are intentionally deprioritized
- Overall fairness behavior of CFS is preserved

---

## Technologies Used

- C
- Linux Kernel v5.19.8
- Linux Completely Fair Scheduler (CFS)
- Linux System Calls
- Ubuntu
- GCC
- GRUB
- Linux Process Scheduling

---
