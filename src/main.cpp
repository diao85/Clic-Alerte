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
// ---- Importation pour gestion WIFI ----------------------------------------------------
#include <WiFiManager.h>
#include <ESP8266WiFi.h>

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

bool modeNormal = true;
bool portailActif = false; 
// ---- Parametres propre au bouton ------------------------------------------------------
char Bot_Token[70];
char Nom_Admin[25];
char Prenom_Admin[25];
char Chat_Id[40];
char Nom_Bouton[25];

//--------------------------------------------------------------------------------------//
//                                                                                      //
//                                        OBJETS                                        //
//                                                                                      //
//--------------------------------------------------------------------------------------//

// ---- Declaration des switchs ----------------------------------------------------------
Button2 btnAlarm;
Button2 btnMode;
// ---- Declaration WIFImanager ----------------------------------------------------------
WiFiManager wifiManager;

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
//                       Fonctions concernant la gestion du WIFI                        //
//--------------------------------------------------------------------------------------//

void saveConfigCallback() {
    Serial.println("[CALLBACK] saveCallback fired");
}

void wifiSettings() {
    // ---- Recuperation de la date ----------------------------------------------------------
    //datenLesen();

    // ---- Parametres personnalisés additionnels du portail ---------------------------------
    WiFiManagerParameter custom_Bot_Token("Bot_Token", "Entrez ici votre Bot Token", Bot_Token, 70);
    WiFiManagerParameter custom_Nom_Bouton("Nom_Bouton", "Entrez ici Le nom du bouton", Nom_Bouton, 25);
    WiFiManagerParameter custom_Nom_Admin("Nom_Admin", "Entrez ici votre Nom", Nom_Admin, 25);
    WiFiManagerParameter custom_Prenom_Admin("Prenom_Admin", "Entrez ici votre Prenom", Prenom_Admin, 25);
    WiFiManagerParameter custom_Chat_Id("Chat_Id", "Entrez ici votre Chat Id", Chat_Id, 40);

    // ---- Initialisation de WifiManager ----------------------------------------------------
    wifiManager.setSaveConfigCallback(saveConfigCallback);                              // Sauvegarde des parametres si besoin
    wifiManager.setBreakAfterConfig(true);
    wifiManager.setCleanConnect(true);                                                  // Pour eviter conflit entre connexion Wifi et WifiManager
    //wifiManager.setCountry("US");                                                       // Configuration pays connexion WIFI

    // ---- Ajout parametres personnalises ---------------------------------------------------
    wifiManager.addParameter(&custom_Bot_Token);
    wifiManager.addParameter(&custom_Nom_Bouton);
    wifiManager.addParameter(&custom_Nom_Admin);
    wifiManager.addParameter(&custom_Prenom_Admin);
    wifiManager.addParameter(&custom_Chat_Id);

    //wifiManager.setTimeout(300);                                                        // Time out au bout de 5 minutes

    wifiManager.setConfigPortalBlocking(false);
    wifiManager.autoConnect("Bouton_d'alerte", "diao85100");

    // if (!configWifiDispo && !wifiManager.autoConnect("Bouton_d'alerte", "diao85100")) {  
    //     Serial.println("La connexion a échoué et le délai a expiré.");
    //     delay(3000);
    //     ESP.restart();
    //     delay(1000);
    // }

    //La boîte aux lettres est maintenant en ligne
    //Serial.println("Der Briefkasten ist online");

// ---- Lecture des parametres -----------------------------------------------------------
    // strncpy(Nom_Bouton, jsonDoc["Nom_Bouton"],25);
    // strncpy(Bot_Token, jsonDoc["Bot_Token"],70);
    // strncpy(Nom_Admin, jsonDoc["Nom_Admin"],25);
    // strncpy(Prenom_Admin, jsonDoc["Prenom_Admin"],25);
    // strncpy(Chat_Id, jsonDoc["Chat_Id"],40);


    //enregistre les données mises à jour dans le système de fichiers
    // datenSpeichern();
}

//  --------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------//
//                                                                                      //
//                                        SETUP                                         //
//                                                                                      //
//--------------------------------------------------------------------------------------//

void setup() {

    WiFi.mode(WIFI_STA);
    //wifiManager.resetSettings();
    // ---- Initialisation du Serial ---------------------------------------------------------
    Serial.begin(115200);
    
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
    // ---- Mode Entrainement ----------------------------------------------------------------
        digitalWrite (rLed, LOW);
        digitalWrite (gLed, HIGH);
        digitalWrite (bLed, LOW);
        btnAlarm.loop();
    } else {
    // ---- Mode Normal ----------------------------------------------------------------------
        if (WiFi.status() == WL_CONNECTED) {
        // ---- Cas ou le dispositif est CONNECTE ------------------------------------------------
            digitalWrite (rLed, LOW);
            digitalWrite (gLed, LOW);
            digitalWrite (bLed, HIGH);
            portailActif = false;
            btnAlarm.loop();
        } else {
        // ---- Cas ou le dispositif est DECONNECTE ----------------------------------------------
            if (portailActif == false) {
            // ---- Si premiere detection non connectee ----------------------------------------------
                digitalWrite (rLed, HIGH);
                digitalWrite (gLed, LOW);
                digitalWrite (bLed, LOW);
                // wifiSettings();

                // ---- Parametres personnalisés additionnels du portail ---------------------------------
                WiFiManagerParameter custom_Bot_Token("Bot_Token", "Entrez ici votre Bot Token", Bot_Token, 70);
                WiFiManagerParameter custom_Nom_Bouton("Nom_Bouton", "Entrez ici Le nom du bouton", Nom_Bouton, 25);
                WiFiManagerParameter custom_Nom_Admin("Nom_Admin", "Entrez ici votre Nom", Nom_Admin, 25);
                WiFiManagerParameter custom_Prenom_Admin("Prenom_Admin", "Entrez ici votre Prenom", Prenom_Admin, 25);
                WiFiManagerParameter custom_Chat_Id("Chat_Id", "Entrez ici votre Chat Id", Chat_Id, 40);

                // ---- Initialisation de WifiManager ----------------------------------------------------
                wifiManager.setSaveConfigCallback(saveConfigCallback);                              // Sauvegarde des parametres si besoin
                //wifiManager.setBreakAfterConfig(true);
                //wifiManager.setCleanConnect(true);                                                  // Pour eviter conflit entre connexion Wifi et WifiManager
                //wifiManager.setCountry("US");                                                       // Configuration pays connexion WIFI

                // ---- Ajout parametres personnalises ---------------------------------------------------
                wifiManager.addParameter(&custom_Bot_Token);
                wifiManager.addParameter(&custom_Nom_Bouton);
                wifiManager.addParameter(&custom_Nom_Admin);
                wifiManager.addParameter(&custom_Prenom_Admin);
                wifiManager.addParameter(&custom_Chat_Id);

                //wifiManager.setTimeout(300);                                                        // Time out au bout de 5 minutes

                wifiManager.setConfigPortalBlocking(false);
                wifiManager.autoConnect("Bouton_d'alerte", "diao85100");
                portailActif = true;

            } else {
                wifiManager.process();
            }
        }
    }
}
