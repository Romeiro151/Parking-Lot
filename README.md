# Parking-Lot
This project consists of an Operating Systems simulation of a parking management system developed in C and Bash. The goal was to model concurrent access to shared resources while applying core OS concepts in a practical environment.

The system uses parent and child processes to represent different parking operations and relies on inter-process communication (IPC) mechanisms for coordination between processes. Semaphores were implemented to ensure proper synchronization and to prevent race conditions when accessing shared resources.

Additionally, the project includes file handling for persistent data storage and logging, as well as process management and signal handling to ensure controlled execution and termination.

This project provided hands-on experience with:
	Process creation and management (fork, wait)
	Inter-process communication (shared memory, message queues, signals)
	Synchronization using semaphores
	Bash scripting for orchestration and automation
