## Implementation Overview

This project implements a preemptive scheduler with blocking sleep and synchronization primitives for a simple operating system kernel.

## Design Decisions

### 1. Preemptive Scheduling (entry.S, scheduler.c)

**IRQ0 Timer Interrupt Handler:**
- Increments the global 64-bit `time_elapsed` counter on every timer tick
- Manages `disable_count` to track interrupt enable/disable state
- Tests `nested_count` to determine if preemption is safe:
  - If `nested_count == 0`: Process is not in a system call, safe to preempt
  - If `nested_count != 0`: Process is in a system call or is a kernel thread, defer preemption
- Calls `check_sleeping()` to wake up any processes whose sleep time has expired
- Sends End-Of-Interrupt (EOI) signal to hardware to acknowledge the interrupt

**Round-Robin Scheduling:**
- Implemented in `put_current_running()` which adds the current process to the end of the ready queue
- `scheduler_entry()` picks the next process from the front of the ready queue
- This FIFO approach ensures fair time-slice allocation

**Critical Section Management:**
- `disable_count` tracks nested critical sections
- Interrupts are disabled when `disable_count > 0`
- `enter_critical()` disables interrupts and increments counter
- `leave_critical()` decrements counter and re-enables interrupts only when count reaches 0

### 2. Blocking Sleep (scheduler.c)

**Implementation:**
- `do_sleep()` calculates wakeup time based on `time_elapsed + milliseconds`
- Moves current process from ready queue to sleeping queue with wakeup time stored in PCB
- Immediately schedules next available process

**Wake-up Mechanism:**
- `check_sleeping()` is called on every timer interrupt
- Iterates through sleeping queue, comparing each process's wakeup time with current `time_elapsed`
- Processes whose wakeup time has passed are moved back to the ready queue

**Edge Case - All Processes Sleeping:**
- If ready queue is empty, `scheduler_entry()` handles gracefully
- Timer interrupts continue, allowing `check_sleeping()` to eventually wake processes
- System remains responsive to timer events

### 3. Synchronization Primitives (sync.h, sync.c)

**Condition Variables:**
- Structure contains a wait queue for blocked threads
- `condition_wait()`: Atomically releases lock, blocks caller, then reacquires lock upon wakeup
- `condition_signal()`: Wakes exactly one waiting thread (if any)
- `condition_broadcast()`: Wakes all waiting threads
- Critical sections protect queue operations to prevent races

**Semaphores:**
- Structure contains an integer value and wait queue
- `semaphore_down()`: If value > 0, decrement and proceed; otherwise block
- `semaphore_up()`: If threads waiting, wake one (direct transfer); otherwise increment value
- Maintains invariant: value represents available resources

**Barriers:**
- Structure tracks total threads needed (n), current count, and wait queue
- `barrier_wait()`: Increment count; if count < n, block; if count == n, wake all and reset
- Last thread to arrive wakes all others and resets barrier for reuse
- Race-free through critical section protection

### 4. Race Condition Prevention

All synchronization primitives and scheduler functions:
- Enter critical section before modifying shared state
- Use `enter_critical()` / `leave_critical()` to disable/enable interrupts
- Keep critical sections short to maintain system responsiveness
- Ensure atomic operations on queues and PCB fields

## What Works

✓ Preemptive scheduling with timer interrupts
✓ Round-robin process scheduling
✓ Blocking sleep with accurate wake-up timing
✓ Condition variables (wait, signal, broadcast)
✓ Semaphores (down, up with proper value semantics)
✓ Barriers (n-thread synchronization with reset)
✓ Proper interrupt management (nested disable_count)
✓ Safe context switching under preemption

## Testing Results

- **test_regs**: Passed - registers preserved across preemption
- **test_preempt**: Passed - both processes making progress via preemption
- **test_blocksleep**: Passed - sleep duration accurate, non-sleeping process runs
- **test_barrier**: Passed - all threads synchronize correctly
- **test_all**: Passed - comprehensive test of all features

## Known Issues / Limitations

None - all required functionality implemented and tested.

## Files Modified

- `entry.S`: Implemented `irq0_entry` and `TEST_NESTED_COUNT` macro
- `scheduler.c`: Implemented `put_current_running()`, `do_sleep()`, `check_sleeping()`
- `scheduler.h`: Added PCB structure with necessary fields
- `sync.h`: Defined structures for condition variables, semaphores, barriers
- `sync.c`: Implemented all synchronization primitive operations

## Compilation

```bash
make
./settest test_name
bochs
```
