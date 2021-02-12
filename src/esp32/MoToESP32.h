#ifndef MOTOESP32_H
#define MOTOESP32_H
// ESP32 specific defines for Cpp files

//#warning ESP32 specific cpp includes
#if defined COMPILING_MOTOSERVO_CPP

//returns the channelnumber ( 0...15 ) of the leds channel to be used, or -1 if no channel is availabele
static inline __attribute__((__always_inline__)) int8_t servoPwmSetup( servoData_t *servoDataP ) {
    DB_PRINT("Search fre ledc channel")
    int8_t pwmNbr = initPwmChannel( servoDataP->pin, SERVO_TIMER );
    pinMode( servoDataP->pin, OUTPUT );
    attachInterruptArg( servoDataP->pin, ISR_Servo, (void*)servoDataP, FALLING );
    DB_PRINT( "PwmNbr:%d, Pin:%d, Group=%d, Channel=%d, Timer=%d", pwmNbr, pwmUse[pwmNbr].pin, pwmUse[pwmNbr].group, pwmUse[pwmNbr].channel, pwmUse[pwmNbr].timer );
    return pwmNbr;
}

static inline __attribute__((__always_inline__)) void startServoPulse( servoData_t *servoDataP, uint32_t pulseWidth ) {
    setPwmPin( servoDataP->pwmNbr );
    setPwmDuty( servoDataP->pwmNbr, pulseWidth );
    attachInterruptArg( servoDataP->pin, ISR_Servo, servoDataP, FALLING );
}

static inline __attribute__((__always_inline__)) void servoWrite( servoData_t *servoDataP, uint32_t pulseWidth ) {
    // the same funktion exists for ESP8266 ( with another internal call )
    setPwmDuty( servoDataP->pwmNbr, pulseWidth );
    
}

static inline __attribute__((__always_inline__)) void servoPulseOff( servoData_t *servoDataP ) {
    //DB_PRINT("Stop Puls, ledcNr=%d", servoDataP->pwmNbr );
    setPwmDuty( servoDataP->pwmNbr, 0 );
}

static inline __attribute__((__always_inline__)) void servoDetach( servoData_t *servoDataP ) {
    detachInterrupt( servoDataP->pin );
    freePwmNbr( servoDataP->pwmNbr );
}
#endif

#ifdef COMPILING_MOTOSOFTLED_CPP
//returns the channelnumber ( 0...15 ) of the leds channel to be used, or -1 if no channel is availabele
static inline __attribute__((__always_inline__)) int8_t softLedPwmSetupAS( servoData_t *softledDataP )  {
    int8_t pwmNbr = initPwmChannel( softledDataP->pin, LED_TIMER );
    pinMode( softledDataP->pin, OUTPUT );
    attachInterruptArg( softledDataP->pin, ISR_Servo, (void*)softledDataP, FALLING );
    return pwmNbr;
}

static inline __attribute__((__always_inline__)) uint8_t attachSoftledAS( ledData_t *ledDataP ) {
    int8_t pwmNbr = initPwmChannel( ledDataP->pin, LED_TIMER );
    pinMode( ledDataP->pin, OUTPUT );
    attachInterruptArg( ledDataP->pin, ISR_Softled, (void*)ledDataP, FALLING );
    return pwmNbr;

}

static inline __attribute__((__always_inline__)) void startLedPulseAS( uint8_t pwmNbr, uint8_t invFlg, uint32_t pulseLen ){
    // start or change the pwmpulses on the led pin.
    // with invFlg set pulseLen is lowtime, else hightime
    // compute pulselen from µs to tics
    pulseLen = slPwm2tic(pulseLen);
    if ( invFlg ) {
        setPwmDuty(pwmNbr, DUTY100-pulseLen);
    } else {
        setPwmDuty(pwmNbr, pulseLen);
    }

}

static inline __attribute__((__always_inline__)) void softLedOffAS(  uint8_t pwmNbr, uint8_t invFlg ){
    setPwmDuty(pwmNbr, invFlg? DUTY100 : 0);
    //digitalWrite( pin , invFlg );
}

static inline __attribute__((__always_inline__)) void softLedOnAS(  uint8_t pwmNbr, uint8_t invFlg ){
    setPwmDuty(pwmNbr, invFlg? 0 : DUTY100);
    //digitalWrite( pin , invFlg );
}

static inline __attribute__((__always_inline__)) void softLedOn2AS(  uint8_t pwmNbr, uint8_t invFlg ){
    // keine Aktion beim ESP32 notwendig
}

static inline __attribute__((__always_inline__)) void attachInterruptAS(  ledData_t *ledDataP ){
    attachInterruptArg( ledDataP->pin, ISR_Softled, (void*)ledDataP, FALLING );
}

#endif    
#endif