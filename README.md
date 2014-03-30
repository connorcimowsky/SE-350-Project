# SE 350 Project

A real-time operating system for the Keil MCB1700 Evaluation Board.

## Keyboard Commands

| Command           | Action                                     |
| ----------------- | ------------------------------------------ |
| `%WR`             | Resets the wall clock.                     |
| `%WS hh:mm:ss`    | Sets the wall clock to the specified time. |
| `%WT`             | Terminates the wall clock.                 |
| `%C pid priority` | Sets the priority of `pid` to `priority`.  |
| `%Z`              | Initiates stress tests.                    |
| `%P`              | Invokes the time profiler.                 |

## Debug Hotkeys

| Hotkey | Action                                     |
| ------ | ------------------------------------------ |
| `!`    | Prints the ready queue.                    |
| `@`    | Prints the blocked-on-memory queue.        |
| `#`    | Prints the blocked-on-receive queue.       |
| `$`    | Prints the sent and received message logs. |
| `^`    | Prints the memory heap.                    |