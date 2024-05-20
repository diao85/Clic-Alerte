//--------------------------------------------------------------------------------------//
//                                                                                      //
//                                     IMPORTATIONS                                     //
//                                                                                      //
//--------------------------------------------------------------------------------------//

#include <Arduino.h>
// ---- Importation pour evenements bouton -----------------------------------------------
#include <Button2.h>
// ---- Importation pour les notes -------------------------------------------------------
#include <pitches_fr.h>

//--------------------------------------------------------------------------------------//
//                                                                                      //
//                                      CONSTANTES                                      //
//                                                                                      //
//--------------------------------------------------------------------------------------//

// ---- Declaration des pins d'entrees et de sorties -------------------------------------
const int rLed = D7;                    // Pin Led Rouge
const int gLed = D6;                    // Pin Led Verte
const int bLed = D8;                    // Pin Led Bleue
const int beeper = D2;                  // Pin Buzzer
const int btnAlarmPin = D5;             // Pin Alarm
const int btnModePin = D3;              // Pin Mode

//--------------------------------------------------------------------------------------//
//                                                                                      //
//                                  VARIABLES GLOBALES                                  //
//                                                                                      //
//--------------------------------------------------------------------------------------//

bool modeNormal = false;

//--------------------------------------------------------------------------------------//
//                                                                                      //
//                                        OBJETS                                        //
//                                                                                      //
//--------------------------------------------------------------------------------------//

// ---- Declaration des switchs ----------------------------------------------------------
Button2 btnAlarm;
Button2 btnMode;

//--------------------------------------------------------------------------------------//
//                                                                                      //
//                                      FONCTIONS                                       //
//                                                                                      //
//--------------------------------------------------------------------------------------//

//--------------------------------------------------------------------------------------//
//                            Fonctions concernant le buzzer                            //
//--------------------------------------------------------------------------------------//

void beep(int note, long duration) {
    tone(beeper, note, duration);
    delay(duration*1.10);
}
void beepAlerte() {
    beep(Sol_6, 100);
    beep(Si_7, 100);
    beep(Sol_6, 100);
}
void beepDeconnexion() {
    beep(Fa_7, 100);
}
void beepModeChange() {
    beep(Si_7, 100);
    beep(Re_7, 100);
}
//  --------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------//
//                        Fonctions concernant les interrupteurs                        //
//--------------------------------------------------------------------------------------//

void btnAlarmPressed(Button2& btn) {    
    beepAlerte();
}
 void btnModePressed(Button2& btn) {    
    beepModeChange();
    modeNormal = !modeNormal;
}
//  --------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------//
//                                                                                      //
//                                        SETUP                                         //
//                                                                                      //
//--------------------------------------------------------------------------------------//

void setup() {
    
    // ---- Initialisation des entrees et sorties --------------------------------------------
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(rLed, OUTPUT);
    pinMode(gLed, OUTPUT);
    pinMode(bLed, OUTPUT);
    pinMode(btnAlarmPin, OUTPUT);
    pinMode(btnModePin, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    digitalWrite(rLed, LOW);
    digitalWrite(gLed, LOW);
    digitalWrite(bLed, LOW);

    // ---- Initialisation des switchs -------------------------------------------------------
    btnAlarm.begin(btnAlarmPin, INPUT_PULLUP, true);
    btnAlarm.setPressedHandler(btnAlarmPressed);

    btnMode.begin(btnModePin, INPUT_PULLUP, true);
    btnMode.setPressedHandler(btnModePressed);

}

//--------------------------------------------------------------------------------------//
//                                                                                      //
//                                         LOOP                                         //
//                                                                                      //
//--------------------------------------------------------------------------------------//

void loop() {

    btnMode.loop();

    if (!modeNormal) {
        digitalWrite (rLed, LOW);
        digitalWrite (gLed, HIGH);
        digitalWrite (bLed, LOW);
        btnAlarm.loop();
    } else {
        digitalWrite (rLed, HIGH);
        digitalWrite (gLed, LOW);
        digitalWrite (bLed, LOW);
    }
}
