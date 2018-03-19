#include "Clock.h"
#include "Processor.h"
// #include "ComputerSystem.h"

int tics=0;

void Clock_Update() {
	tics++;
	if (tics%INTERVALBETWEENINTERRUPS==0) {
		Processor_RaiseInterrupt(CLOCKINT_BIT);
		Processor_ManageInterrupts();
	}
    // ComputerSystem_DebugMessage(97,CLOCK,tics);
}


int Clock_GetTime() {

	return tics;
}
