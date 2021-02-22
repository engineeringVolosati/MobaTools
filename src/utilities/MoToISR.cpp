/*
  MobaTools.h - a library for model railroaders
  Author: fpm, fpm@mnet-mail.de
  Copyright (c) 2020 All right reserved.

  ISR for stepper and softleds ( not for ESP8266 )
*/

#include <MobaTools.h>
#define debugTP
#include <utilities/MoToDbg.h>

nextCycle_t nextCycle;
static nextCycle_t cyclesLastIRQ = 1;  // cycles since last IRQ
// ISR on ESP is completely different - on ESP this File is empty
///////////////////////////////////////////////////////////////////////////////////////////////////
#if 0 // defined ARDUINO_ARCH_AVR // +++++++++++++++++++++++Variante f�r 8-Bit AVR Prozessoren +++++++++++++++++
// ---------- OCRxB Compare Interrupt used for stepper motor and Softleds ----------------
void stepperISR(uint8_t cyclesLastIRQ) __attribute__ ((weak));
void softledISR(uint8_t cyclesLastIRQ) __attribute__ ((weak));
ISR ( TIMERx_COMPB_vect) {
    uint16_t tmp;
  // Timer1 Compare B, used for stepper motor, starts every CYCLETIME us
    // 26-09-15 An Interrupt is only created at timeslices, where data is to output
    SET_TP1;
    nextCycle = ISR_IDLETIME  / CYCLETIME ;// min ist one cycle per IDLETIME
    if ( stepperISR ) stepperISR(cyclesLastIRQ);
    //============  End of steppermotor ======================================
   if ( softledISR ) softledISR(cyclesLastIRQ);
    // ======================= end of softleds =====================================
    // set compareregister to next interrupt time;
    // compute next IRQ-Time in us, not in tics, so we don't need long
    //noInterrupts(); // when manipulating 16bit Timerregisters IRQ must be disabled
    if ( nextCycle == 1 )  {
        CLR_TP1;
        noInterrupts();
        // this is timecritical: Was the ISR running longer then CYCELTIME?
        // compute length of current IRQ ( which startet at OCRxB )
        // we assume a max. runtime of 1000 Tics ( = 500�s , what nevver should happen )
        tmp = GET_COUNT - OCRxB ;
        if ( tmp > 1000 ) tmp += TIMER_OVL_TICS; // there was a timer overflow
        if ( tmp > (CYCLETICS-10) ) {
            // runtime was too long, next IRQ mus be started immediatly
            SET_TP3;
            tmp = GET_COUNT+10; 
        } else {
            tmp = OCRxB + CYCLETICS;
        }
        OCRxB = ( tmp > TIMER_OVL_TICS ) ? tmp -= TIMER_OVL_TICS : tmp ;
        interrupts();
        SET_TP1;
    } else {
        // time till next IRQ is more then one cycletime
        // compute next IRQ-Time in us, not in tics, so we don't need long
        tmp = ( OCRxB / TICS_PER_MICROSECOND + nextCycle * CYCLETIME );
        if ( tmp > TIMERPERIODE ) tmp = tmp - TIMERPERIODE;
        OCRxB = tmp * TICS_PER_MICROSECOND;
    }
    cyclesLastIRQ = nextCycle;
    CLR_TP1; // Oszimessung Dauer der ISR-Routine
}
////////////////////////////////////////////////////////////////////////////////////////////
#endif
#if defined __STM32F1__  // +++++++++++++++++++++++ Variante f�r STM32F1 +++++++++++++++++
void stepperISR(int32_t cyclesLastIRQ)  __attribute__ ((weak));
void softledISR(uint32_t cyclesLastIRQ)  __attribute__ ((weak));
void ISR_Stepper() {
    // Timer4 Channel 1, used for stepper motor, starts every CYCLETIME us
    // 26-09-15 An Interrupt is only created at timeslices, where data is to output
    SET_TP1;
    nextCycle = ISR_IDLETIME  / CYCLETIME ;// min ist one cycle per IDLETIME
    if ( stepperISR ) stepperISR(cyclesLastIRQ);
    //============  End of steppermotor ======================================
    if ( softledISR ) softledISR(cyclesLastIRQ);
    // ======================= end of softleds =====================================
    // set compareregister to next interrupt time;
	//SET_TP2;
	// next ISR must be at least MIN_STEP_CYCLE beyond actual counter value ( time between to ISR's )
	int minOCR = timer_get_count(MT_TIMER);
	int nextOCR = timer_get_compare(MT_TIMER, STEP_CHN);
	if ( minOCR < nextOCR ) minOCR += TIMER_OVL_TICS; // timer had overflow already
    minOCR = minOCR + ( MIN_STEP_CYCLE * TICS_PER_MICROSECOND ); // minimumvalue for next OCR
	nextOCR = nextOCR + ( nextCycle * TICS_PER_MICROSECOND );
	if ( nextOCR < minOCR ) {
		// time till next ISR ist too short, set to mintime and adjust nextCycle
		nextOCR = minOCR;
		nextCycle = ( nextOCR - timer_get_compare(MT_TIMER, STEP_CHN)  ) / TICS_PER_MICROSECOND;
	}
    if ( nextOCR > TIMER_OVL_TICS ) nextOCR -= TIMER_OVL_TICS;
    timer_set_compare( MT_TIMER, STEP_CHN, nextOCR ) ;
	//CLR_TP2;
    cyclesLastIRQ = nextCycle;
    CLR_TP1; // Oszimessung Dauer der ISR-Routine
}
////////////////////////////////////////////////////////////////////////////////////////////
#endif
#if 0 // defined ESP32  // +++++++++++++++++++++++ Variante f�r ESP32 +++++++++++++++++
void IRAM_ATTR stepperISR(int32_t cyclesLastIRQ)  __attribute__ ((weak));
//void IRAM_ATTR softledISR(uint32_t cyclesLastIRQ)  __attribute__ ((weak));
void IRAM_ATTR ISR_Stepper(void) {
    // Timer autoreload, used for stepper motor
    SET_TP1;
    nextCycle = ISR_IDLETIME  / CYCLETIME ;// min ist one cycle per IDLETIME
    portENTER_CRITICAL_ISR(&stepperMux);
    if ( stepperISR ) stepperISR(cyclesLastIRQ);
    //============  End of steppermotor ======================================
    //if ( softledISR ) softledISR(cyclesLastIRQ);
    // ======================= end of softleds =====================================
    // set compareregister to next interrupt time;
	//SET_TP2;
	// next ISR must be at least MIN_STEP_CYCLE beyond actual counter value ( time between to ISR's )
    // >>> this is not possible, because reading the timer does not work here ( wy??)
	/*int32_t minCycle = (int32_t)timerRead(stepTimer)/TICS_PER_MICROSECOND ;
    minCycle = minCycle+( MIN_STEP_CYCLE  ); // minimumtime until next Interrupt*/
    nextCycle_t minCycle = MIN_STEP_CYCLE; // min time from irq to next irq
	if ( nextCycle < (nextCycle_t)minCycle ) {
		// time till next ISR ist too short, set to mintime and adjust nextCycle
        CLR_TP1;
		nextCycle =  minCycle;
	}
    timerAlarmWrite(stepTimer, nextCycle*TICS_PER_MICROSECOND , true);
    timerAlarmEnable(stepTimer);
    SET_TP1;
	//CLR_TP2;
    cyclesLastIRQ = nextCycle;
    portEXIT_CRITICAL_ISR(&stepperMux);
    CLR_TP1; // Oszimessung Dauer der ISR-Routine
}
////////////////////////////////////////////////////////////////////////////////////////////
#endif
#if defined ESP32  // +++++++++++++++++++++++ Variante f�r ESP32 mit durchlaufendem Timer +++++++++++++++++
void IRAM_ATTR stepperISR(int32_t cyclesLastIRQ)  __attribute__ ((weak));
//void IRAM_ATTR softledISR(uint32_t cyclesLastIRQ)  __attribute__ ((weak));
void IRAM_ATTR ISR_Stepper(void) {
    static uint64_t lastAlarm, aktAlarm;
    // Timer running up, used for stepper motor. No reload of timer
    SET_TP1;
    nextCycle = ISR_IDLETIME  / CYCLETIME ;// min ist one cycle per IDLETIME
    portENTER_CRITICAL_ISR(&stepperMux);
    cyclesLastIRQ = (aktAlarm - lastAlarm) / TICS_PER_MICROSECOND;
    if ( stepperISR ) stepperISR(cyclesLastIRQ);
	// next alarm ISR must be at least MIN_STEP_CYCLE beyond last alarm value ( time between to ISR's )
    lastAlarm = aktAlarm;
    aktAlarm = lastAlarm+(nextCycle*TICS_PER_MICROSECOND); // minimumtime until next Interrupt
    uint64_t minNextAlarm = lastAlarm + 20;
	if ( aktAlarm < minNextAlarm ) {
		// time till next ISR ist too short, set to mintime and adjust nextCycle
        CLR_TP1;
		aktAlarm =  minNextAlarm;
	}
    timerAlarmWrite(stepTimer, aktAlarm , false); // no autorelaod
    timerAlarmEnable(stepTimer);
    SET_TP1;
    portEXIT_CRITICAL_ISR(&stepperMux);
    CLR_TP1; // Oszimessung Dauer der ISR-Routine
}
////////////////////////////////////////////////////////////////////////////////////////////
#endif

