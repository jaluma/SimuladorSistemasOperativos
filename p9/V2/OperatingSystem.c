#include "OperatingSystem.h"
#include "OperatingSystemBase.h"
#include "MMU.h"
#include "Processor.h"
#include "Buses.h"
#include "Heap.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>

// Functions prototypes
void OperatingSystem_PrepareDaemons(int);
void OperatingSystem_PCBInitialization(int, int, int, int, int,int);
void OperatingSystem_MoveToTheREADYState(int,int);
void OperatingSystem_Dispatch(int);
void OperatingSystem_RestoreContext(int);
void OperatingSystem_SaveContext(int);
void OperatingSystem_TerminateProcess();
int OperatingSystem_LongTermScheduler();
void OperatingSystem_PreemptRunningProcess();
int OperatingSystem_CreateProcess(int,int);
int OperatingSystem_ObtainMainMemory(int, int);
int OperatingSystem_ShortTermScheduler();
int OperatingSystem_ExtractFromReadyToRun(int);
int OperatingSystem_ExtractFromBlockedToRun();
void OperatingSystem_HandleException();
void OperatingSystem_HandleSystemCall();
void OperatingSystem_MoveToTheBLOCKEDState(int);

// The process table
PCB processTable[PROCESSTABLEMAXSIZE];

// Address base for OS code in this version
int OS_address_base = PROCESSTABLEMAXSIZE * MAINMEMORYSECTIONSIZE;

// Identifier of the current executing process
int executingProcessID=NOPROCESS;

// Identifier of the System Idle Process
int sipID;

// Begin indes for daemons in programList
int baseDaemonsInProgramList; 

// Array that contains the identifiers of the READY processes
int readyToRunQueue[NUMBEROFQUEUES][PROCESSTABLEMAXSIZE];
int numberOfReadyToRunProcesses[NUMBEROFQUEUES]={0,0};

// Variable containing the number of not terminated user processes
int numberOfNotTerminatedUserProcesses=0;

// Array with types states
char * statesNames [5]={"NEW","READY","EXECUTING","BLOCKED","EXIT"};

// Times clock interruptions have been executed
int numberOfClockInterrupts = 0;

// In OperatingSystem.c  Exercise 5-b of V2 
// Heap with blocked porcesses sort by when to wakeup 
int sleepingProcessesQueue[PROCESSTABLEMAXSIZE]; 
int numberOfSleepingProcesses=0;

// Initial set of tasks of the OS
void OperatingSystem_Initialize(int daemonsIndex) {
	
	int i, selectedProcess, long_term_schreduler;
	FILE *programFile; // For load Operating System Code

	// Obtain the memory requirements of the program
	int processSize=OperatingSystem_ObtainProgramSize(&programFile, "OperatingSystemCode");
	
	// Load Operating System Code
	OperatingSystem_LoadProgram(programFile, OS_address_base, processSize);
	
	// Process table initialization (all entries are free)
	for (i=0; i<PROCESSTABLEMAXSIZE;i++)
		processTable[i].busy=0;
	
	// Initialization of the interrupt vector table of the processor
	Processor_InitializeInterruptVectorTable(OS_address_base+1);
		
	// Create all system daemon processes
	OperatingSystem_PrepareDaemons(daemonsIndex);

	// Create all user processes from the information given in the command line
	long_term_schreduler = OperatingSystem_LongTermScheduler();
	
	if (long_term_schreduler <= 1) {
		OperatingSystem_ReadyToShutdown();
	}
	
	if (strcmp(programList[processTable[sipID].programListIndex]->executableName,"SystemIdleProcess")) {
		// Show message "ERROR: Missing SIP program!\n"
		ComputerSystem_DebugMessage(21,SHUTDOWN);
		exit(1);		
	}

	// At least, one user process has been created
	// Select the first process that is going to use the processor
	selectedProcess=OperatingSystem_ShortTermScheduler();
	
	// Assign the processor to the selected process
	OperatingSystem_Dispatch(selectedProcess);

	// Initial operation for Operating System
	Processor_SetPC(OS_address_base);
}

// Daemon processes are system processes, that is, they work together with the OS.
// The System Idle Process uses the CPU whenever a user process is able to use it
void OperatingSystem_PrepareDaemons(int programListDaemonsBase) {
  
	// Include a entry for SystemIdleProcess at 0 position
	programList[0]=(PROGRAMS_DATA *) malloc(sizeof(PROGRAMS_DATA));

	programList[0]->executableName="SystemIdleProcess";
	programList[0]->arrivalTime=0;
	programList[0]->type=DAEMONPROGRAM; // daemon program

	sipID=INITIALPID%PROCESSTABLEMAXSIZE; // first PID for sipID

	// Prepare aditionals daemons here
	// index for aditionals daemons program in programList
	baseDaemonsInProgramList=programListDaemonsBase;

}


// The LTS is responsible of the admission of new processes in the system.
// Initially, it creates a process from each program specified in the 
// 			command lineand daemons programs
int OperatingSystem_LongTermScheduler() {
  
	int PID, i,
		numberOfSuccessfullyCreatedProcesses=0;
	
	for (i=0; programList[i]!=NULL && i<PROGRAMSMAXNUMBER; i++) {
		if (programList[i]->type == DAEMONPROGRAM)
			PID=OperatingSystem_CreateProcess(i, DAEMONSQUEUE);
		else
			PID=OperatingSystem_CreateProcess(i, USERPROCESSQUEUE);
		
		if(PID == NOFREEENTRY) {
			OperatingSystem_ShowTime(ERROR);
			ComputerSystem_DebugMessage(103,ERROR,programList[i]->executableName);
		} else if (PID == PROGRAMDOESNOTEXIST) {
			OperatingSystem_ShowTime(ERROR);
			ComputerSystem_DebugMessage(104,ERROR,programList[i]->executableName, "--- it does not exist ---");
		} else if (PID == PROGRAMNOTVALID) {
			OperatingSystem_ShowTime(ERROR);
			ComputerSystem_DebugMessage(104,ERROR,programList[i]->executableName, "--- invalid priority or size ---");
		} else if (PID == TOOBIGPROCESS) {
			OperatingSystem_ShowTime(ERROR);
			OperatingSystem_ShowTime(ERROR);
			ComputerSystem_DebugMessage(105,ERROR,programList[i]->executableName);
		} else {
			numberOfSuccessfullyCreatedProcesses++;
			// Move process to the ready state
			if (programList[i]->type == USERPROGRAM) {
				numberOfNotTerminatedUserProcesses++;
				OperatingSystem_MoveToTheREADYState(PID, USERPROCESSQUEUE);
			} 
		}
	}

	// Return the number of succesfully created processes
	return numberOfSuccessfullyCreatedProcesses;
}


// This function creates a process from an executable program
int OperatingSystem_CreateProcess(int indexOfExecutableProgram, int queueID) {
  
	int PID;
	int processSize;
	int loadingPhysicalAddress;
	int priority;
	int program;
	FILE *programFile;
	PROGRAMS_DATA *executableProgram=programList[indexOfExecutableProgram];

	// Obtain a process ID
	PID=OperatingSystem_ObtainAnEntryInTheProcessTable();
	
	if (PID == NOFREEENTRY) {
		return NOFREEENTRY;
	}

	// Obtain the memory requirements of the program
	processSize=OperatingSystem_ObtainProgramSize(&programFile, executableProgram->executableName);	
	
	if (processSize == PROGRAMDOESNOTEXIST) {
		return PROGRAMDOESNOTEXIST;
	} if (processSize == PROGRAMNOTVALID) {
		return PROGRAMNOTVALID;
	}

	// Obtain the priority for the process
	priority=OperatingSystem_ObtainPriority(programFile);

	if (priority == PROGRAMNOTVALID) {
		return PROGRAMNOTVALID;
	}
	
	// Obtain enough memory space
 	loadingPhysicalAddress=OperatingSystem_ObtainMainMemory(processSize, PID);
	
	if (loadingPhysicalAddress == TOOBIGPROCESS) {
		return TOOBIGPROCESS;
	}

	// Load program in the allocated memory
	program=OperatingSystem_LoadProgram(programFile, loadingPhysicalAddress, processSize);
	
	if (program == TOOBIGPROCESS) {
		return TOOBIGPROCESS;
	}
	
	// PCB initialization
	OperatingSystem_PCBInitialization(PID, loadingPhysicalAddress, processSize, priority, indexOfExecutableProgram, queueID);
	
	// Show message "Process [PID] created from program [executableName]\n"
	OperatingSystem_ShowTime(INIT);
	ComputerSystem_DebugMessage(22,INIT,PID,executableProgram->executableName);
	
	return PID;
}


// Main memory is assigned in chunks. All chunks are the same size. A process
// always obtains the chunk whose position in memory is equal to the processor identifier
int OperatingSystem_ObtainMainMemory(int processSize, int PID) {

 	if (processSize>MAINMEMORYSECTIONSIZE)
		return TOOBIGPROCESS;
	
 	return PID*MAINMEMORYSECTIONSIZE;
}


// Assign initial values to all fields inside the PCB
void OperatingSystem_PCBInitialization(int PID, int initialPhysicalAddress, int processSize, int priority, int processPLIndex, int queueID) {

	processTable[PID].busy=1;
	processTable[PID].initialPhysicalAddress=initialPhysicalAddress;
	processTable[PID].processSize=processSize;
	processTable[PID].state=NEW;
	OperatingSystem_ShowTime(SYSPROC);
	ComputerSystem_DebugMessage(111, SYSPROC, PID, statesNames[0]);
	processTable[PID].priority=priority;
	processTable[PID].programListIndex=processPLIndex;
	// Daemons run in protected mode and MMU use real address
	if (programList[processPLIndex]->type == DAEMONPROGRAM) {
		processTable[PID].copyOfPCRegister=initialPhysicalAddress;
		processTable[PID].copyOfPSWRegister= ((unsigned int) 1) << EXECUTION_MODE_BIT;
		processTable[PID].copyOfAccumulatorRegister=0;
	} 
	else {
		processTable[PID].copyOfPCRegister=0;
		processTable[PID].copyOfPSWRegister=0;
		processTable[PID].copyOfAccumulatorRegister=0;
	}
	processTable[PID].queueID=queueID;
	processTable[PID].whenToWakeUp=0;
}


// Move a process to the READY state: it will be inserted, depending on its priority, in
// a queue of identifiers of READY processes
void OperatingSystem_MoveToTheREADYState(int PID, int queueID) {
	if (Heap_add(PID, readyToRunQueue[queueID] ,QUEUE_PRIORITY , &numberOfReadyToRunProcesses[queueID] ,PROCESSTABLEMAXSIZE)>=0) {
		OperatingSystem_ShowTime(SYSPROC);
		ComputerSystem_DebugMessage(110, SYSPROC, PID, statesNames[processTable[PID].state], statesNames[1]);
		processTable[PID].state=READY;
		OperatingSystem_PrintReadyToRunQueue();
	} 
}

// Move a process to the BLOCKED state: it will be inserted, depending on its priority, in
// a queue of identifiers of BLOCKED processes
void OperatingSystem_MoveToTheBLOCKEDState(int PID) {
	if (Heap_add(PID, sleepingProcessesQueue ,QUEUE_PRIORITY , &numberOfSleepingProcesses ,PROCESSTABLEMAXSIZE)>=0) {
		OperatingSystem_ShowTime(SYSPROC);
		ComputerSystem_DebugMessage(110, SYSPROC, executingProcessID, statesNames[processTable[executingProcessID].state], statesNames[3]);
		processTable[executingProcessID].state = BLOCKED;
	} 
}


// The STS is responsible of deciding which process to execute when specific events occur.
// It uses processes priorities to make the decission. Given that the READY queue is ordered
// depending on processes priority, the STS just selects the process in front of the READY queue
int OperatingSystem_ShortTermScheduler() {
	
	int selectedProcess;

	selectedProcess=OperatingSystem_ExtractFromReadyToRun(USERPROCESSQUEUE);
	
	if (selectedProcess == NOPROCESS)
		selectedProcess=OperatingSystem_ExtractFromReadyToRun(DAEMONSQUEUE);
	
	return selectedProcess;
}


// Return PID of more priority process in the READY queue
int OperatingSystem_ExtractFromReadyToRun(int queueID) {
  
	int selectedProcess=NOPROCESS;

	selectedProcess=Heap_poll(readyToRunQueue[queueID], QUEUE_PRIORITY ,&numberOfReadyToRunProcesses[queueID]);

	// Return most priority process or NOPROCESS if empty queue
	return selectedProcess; 
}

// Return PID of more wakeUp process in the BLOCKED queue
int OperatingSystem_ExtractFromBlockedToRun() {
  
	int selectedProcess=NOPROCESS;

	selectedProcess=Heap_poll(sleepingProcessesQueue, QUEUE_PRIORITY ,&numberOfSleepingProcesses);

	// Return most priority process or NOPROCESS if empty queue
	return selectedProcess; 
}


// Function that assigns the processor to a process
void OperatingSystem_Dispatch(int PID) {

	// The process identified by PID becomes the current executing process
	executingProcessID=PID;
	// Change the process' state
	processTable[PID].state=EXECUTING;
	
	OperatingSystem_ShowTime(SYSPROC);
	ComputerSystem_DebugMessage(110, SYSPROC, PID, statesNames[1], statesNames[2]);
	// Modify hardware registers with appropriate values for the process identified by PID
	OperatingSystem_RestoreContext(PID);
}


// Modify hardware registers with appropriate values for the process identified by PID
void OperatingSystem_RestoreContext(int PID) {  
	// New values for the CPU registers are obtained from the PCB
	Processor_CopyInSystemStack(MAINMEMORYSIZE-1,processTable[PID].copyOfPCRegister);
	Processor_CopyInSystemStack(MAINMEMORYSIZE-2,processTable[PID].copyOfPSWRegister);
	Processor_CopyInSystemStack(MAINMEMORYSIZE-3,processTable[PID].copyOfAccumulatorRegister);
	Processor_SetAccumulator(processTable[PID].copyOfAccumulatorRegister);
	
	// Same thing for the MMU registers
	MMU_SetBase(processTable[PID].initialPhysicalAddress);
	MMU_SetLimit(processTable[PID].processSize);
}


// Function invoked when the executing process leaves the CPU 
void OperatingSystem_PreemptRunningProcess() {

	// Save in the process' PCB essential values stored in hardware registers and the system stack
	OperatingSystem_SaveContext(executingProcessID);
	// Change the process' state
	OperatingSystem_MoveToTheREADYState(executingProcessID, processTable[executingProcessID].queueID);
	// The processor is not assigned until the OS selects another process
	executingProcessID=NOPROCESS;
}


// Save in the process' PCB essential values stored in hardware registers and the system stack
void OperatingSystem_SaveContext(int PID) {
	
	// Load PC saved for interrupt manager
	processTable[PID].copyOfPCRegister=Processor_CopyFromSystemStack(MAINMEMORYSIZE-1);
	
	// Load PSW saved for interrupt manager
	processTable[PID].copyOfPSWRegister=Processor_CopyFromSystemStack(MAINMEMORYSIZE-2);
	
	// Load Accumulator saved for interrupt manager
	processTable[PID].copyOfAccumulatorRegister=Processor_CopyFromSystemStack(MAINMEMORYSIZE-3);
}


// Exception management routine
void OperatingSystem_HandleException() {
  
	// Show message "Process [executingProcessID] has generated an exception and is terminating\n"
	OperatingSystem_ShowTime(SYSPROC);
	ComputerSystem_DebugMessage(23,SYSPROC,executingProcessID);
	
	OperatingSystem_TerminateProcess();
}


// All tasks regarding the removal of the process
void OperatingSystem_TerminateProcess() {
  
	int selectedProcess;
  	
	processTable[executingProcessID].state=EXIT;
	
	OperatingSystem_ShowTime(SYSPROC);
	ComputerSystem_DebugMessage(110, SYSPROC, executingProcessID, statesNames[2], statesNames[4]);
	
	// One more process that has terminated
	numberOfNotTerminatedUserProcesses--;
	
	if (numberOfNotTerminatedUserProcesses<=0) {
		// Simulation must finish 
		OperatingSystem_ReadyToShutdown();
	}
	
	// Select the next process to execute (sipID if no more user processes)
	selectedProcess=OperatingSystem_ShortTermScheduler();
	
	// Assign the processor to that process
	OperatingSystem_Dispatch(selectedProcess);
}


// System call management routine
void OperatingSystem_HandleSystemCall() {
  
	int systemCallID;
	int pid;
	int oldPID;

	// Register A contains the identifier of the issued system call
	systemCallID=Processor_GetRegisterA();
	
	switch (systemCallID) {
		case SYSCALL_PRINTEXECPID:
			// Show message: "Process [executingProcessID] has the processor assigned\n"
			OperatingSystem_ShowTime(SYSPROC);
			ComputerSystem_DebugMessage(24,SYSPROC,executingProcessID);
			break;

		case SYSCALL_END:
			// Show message: "Process [executingProcessID] has requested to terminate\n"
			OperatingSystem_ShowTime(SYSPROC);
			ComputerSystem_DebugMessage(25,SYSPROC,executingProcessID);
			OperatingSystem_TerminateProcess();
			break;
			
		case SYSCALL_YIELD: //  SYSCALL_YIELD=4
			oldPID = executingProcessID;
			pid = Heap_getFirst(readyToRunQueue[USERPROCESSQUEUE], numberOfReadyToRunProcesses[USERPROCESSQUEUE]);
			
			if (pid == NOPROCESS)
				pid = Heap_getFirst(readyToRunQueue[DAEMONSQUEUE], numberOfReadyToRunProcesses[DAEMONSQUEUE]);
			
			if (processTable[oldPID].priority == processTable[pid].priority) {
				OperatingSystem_ShowTime(SHORTTERMSCHEDULE);
				ComputerSystem_DebugMessage(115, SHORTTERMSCHEDULE, oldPID, pid);
				OperatingSystem_PreemptRunningProcess();
				
				pid = OperatingSystem_ShortTermScheduler();
				
				OperatingSystem_Dispatch(pid);
			}
			break;
			
			case SYSCALL_SLEEP:	//  SYSCALL_SLEEP=7
				processTable[executingProcessID].whenToWakeUp = numberOfClockInterrupts + abs(Processor_GetAccumulator()) + 1;
				
				OperatingSystem_MoveToTheBLOCKEDState(executingProcessID);
				
				pid = OperatingSystem_ShortTermScheduler();
				
				OperatingSystem_Dispatch(pid);
				
				OperatingSystem_PrintStatus();
				break;
	}
}
	
//	Implement interrupt logic calling appropriate interrupt handle
void OperatingSystem_InterruptLogic(int entryPoint){
	switch (entryPoint){
		case SYSCALL_BIT: // SYSCALL_BIT=2
			OperatingSystem_HandleSystemCall();
			break;
			
		case EXCEPTION_BIT: // EXCEPTION_BIT=6
			OperatingSystem_HandleException();
			break;
			
		case CLOCKINT_BIT: // CLOCKINT_BIT=9
			OperatingSystem_HandleClockInterrupt();
			break;
	}

}

 // In OperatingSystem.c Exercise 2-b of V2
void OperatingSystem_HandleClockInterrupt(){
	OperatingSystem_ShowTime(SYSPROC);
	numberOfClockInterrupts++;
	ComputerSystem_DebugMessage(120,INTERRUPT, numberOfClockInterrupts);
	
	int i, PID, first, sleepTrue;
	
	for (i = 0; i < numberOfSleepingProcesses; i++) {
		if (processTable[sleepingProcessesQueue[i]].whenToWakeUp == numberOfClockInterrupts) {
			PID = OperatingSystem_ExtractFromBlockedToRun();
			OperatingSystem_MoveToTheREADYState(PID, processTable[PID].queueID);
			sleepTrue = 1;
		}
	}
	
	if (sleepTrue)
		 OperatingSystem_PrintStatus();
	
	first = Heap_getFirst(readyToRunQueue[USERPROCESSQUEUE], numberOfReadyToRunProcesses[USERPROCESSQUEUE]);
			
	if (first == NOPROCESS)
			first = Heap_getFirst(readyToRunQueue[DAEMONSQUEUE], numberOfReadyToRunProcesses[DAEMONSQUEUE]);
			
	if (processTable[executingProcessID].priority < processTable[first].priority) {
		 OperatingSystem_ShowTime(SHORTTERMSCHEDULE);
		 ComputerSystem_DebugMessage(121, SHORTTERMSCHEDULE, executingProcessID, PID);
		 // codigo a implementar
		 OperatingSystem_MoveToTheBLOCKEDState(executingProcessID);
		 OperatingSystem_Dispatch(first);
		 OperatingSystem_PrintStatus();
	}

} 

 void OperatingSystem_PrintReadyToRunQueue() {
	 int i;
	 OperatingSystem_ShowTime(SHORTTERMSCHEDULE);
	 ComputerSystem_DebugMessage(106,SHORTTERMSCHEDULE);

	ComputerSystem_DebugMessage(112,SHORTTERMSCHEDULE, "USER");
	if (numberOfReadyToRunProcesses[USERPROCESSQUEUE] == 0) {
		ComputerSystem_DebugMessage(113,SHORTTERMSCHEDULE);
	} else {
		for (i = 0; i < numberOfReadyToRunProcesses[USERPROCESSQUEUE]; i++) {
			if (i == numberOfReadyToRunProcesses[USERPROCESSQUEUE] - 1)
				ComputerSystem_DebugMessage(107,SHORTTERMSCHEDULE, readyToRunQueue[USERPROCESSQUEUE][i], processTable[readyToRunQueue[USERPROCESSQUEUE][i]].priority, "\n");
			else
				ComputerSystem_DebugMessage(107,SHORTTERMSCHEDULE, readyToRunQueue[USERPROCESSQUEUE][i], processTable[readyToRunQueue[USERPROCESSQUEUE][i]].priority, ", ");
		}
	}

	ComputerSystem_DebugMessage(112,SHORTTERMSCHEDULE, "DAEMONS");
	if (numberOfReadyToRunProcesses[DAEMONSQUEUE] == 0) {
		ComputerSystem_DebugMessage(113,SHORTTERMSCHEDULE);
	} else {
	 for (i = 0; i < numberOfReadyToRunProcesses[DAEMONSQUEUE]; i++) {
		 if (i == numberOfReadyToRunProcesses[DAEMONSQUEUE] - 1)
			 ComputerSystem_DebugMessage(107,SHORTTERMSCHEDULE, readyToRunQueue[DAEMONSQUEUE][i], processTable[readyToRunQueue[DAEMONSQUEUE][i]].priority, "\n");
		 else
			 ComputerSystem_DebugMessage(107,SHORTTERMSCHEDULE, readyToRunQueue[DAEMONSQUEUE][i], processTable[readyToRunQueue[DAEMONSQUEUE][i]].priority, ", ");
	  }
	}
 }

