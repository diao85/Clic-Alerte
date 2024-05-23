#include <Arduino.h>
#include <LittleFS.h>
/* ------------------- Importation pour evenements bouton ------------------- */
#include "Button2.h"
/* --------- Importation pour evenement sur timer (beep deconnexion) -------- */
#include <muTimer.h>
/* ----------------------- Importation pour le Buzzer ----------------------- */
#include "pitches_us.h"
#include <pitches_fr.h>
/* ---------------------- Importation pour WiFiManager ---------------------- */
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
/* ------------------------ Importation pour Telegram ----------------------- */
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
/* ---------------------- Importation pour l'horodatage --------------------- */
#include <time.h>

/* -------------------------------------------------------------------------- */
/*                                 CONSTANTES                                 */
/* -------------------------------------------------------------------------- */

#define FPM_SLEEP_MAX_TIME 0xFFFFFFF
const int historicSize = 10;            // Taille maximale de l'historique
const int rLed = D7;                    // Pin Led Rouge
const int gLed = D6;                    // Pin Led Verte
const int bLed = D8;                    // Pin Led Bleue
const int beeper = D2;                  // Pin Buzzer
const int interrupteurPin = D5;         // Pin Switch
const int switchPin = D3;

/* -------------------------------------------------------------------------- */
/*                                  VARIABLES                                 */
/* -------------------------------------------------------------------------- */

bool shouldSaveConfig = false;          // Flag sauve données
bool connected = false;
bool intPressed = false;       // Flag bouton activé
int compteur = 0;
bool trainingMode = true;
bool portalRunning = false;
char Bot_Token[70];                    // Telegram config
char Nom_Admin[25];
char Prenom_Admin[25];
char Chat_Id[40];                      // Telegram config
char Nom_Bouton[25];
int signalRSSI =0;                      // Force du signal WIFI
String heure;
String date;
String json = "[]";                     // Stockage des events au format String

/* -------------------------------------------------------------------------- */
/*                                   OBJETS                                   */
/* -------------------------------------------------------------------------- */

// Déclaration pour la mise en route du serveur Web
ESP8266WebServer  server(80);
// SSL client obligatoire pour Telegram
X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure client;
UniversalTelegramBot bot(Bot_Token, client);
// Déclaration pour récupération de la date
time_t maintenant;
struct tm * timeinfo;
// Déclaration Interupteur
Button2 interrupteur;
Button2 switchbtn;
// Déclaration timer pour beep si no Wifi
muTimer timerNoWifiSound = muTimer();
muTimer timerConnectBlink = muTimer();
// Déclaration des parametres Telegram
WiFiManagerParameter custom_Bot_Token("Bot_Token", "Entrez ici votre Bot Token", Bot_Token, 70);
WiFiManagerParameter custom_Nom_Bouton("Nom_Bouton", "Entrez ici Le nom du bouton", Nom_Bouton, 25);
WiFiManagerParameter custom_Nom_Admin("Nom_Admin", "Entrez ici votre Nom", Nom_Admin, 25);
WiFiManagerParameter custom_Prenom_Admin("Prenom_Admin", "Entrez ici votre Prenom", Prenom_Admin, 25);
WiFiManagerParameter custom_Chat_Id("Chat_Id", "Entrez ici votre Chat Id", Chat_Id, 40);
WiFiManager wm;

/* -------------------------------------------------------------------------- */
/*                                  FONCTIONS                                 */
/* -------------------------------------------------------------------------- */

void readConfig() {
    if (LittleFS.exists("/config.json")) {
        File configFile = LittleFS.open("/config.json", "r");
        if (configFile) {
            JsonDocument jsonDoc;
            auto deserializeError = deserializeJson(jsonDoc, configFile);
            if (!deserializeError) {
                strncpy(Nom_Bouton, jsonDoc["Nom_Bouton"],25);
                strncpy(Bot_Token, jsonDoc["Bot_Token"],70);
                strncpy(Nom_Admin, jsonDoc["Nom_Admin"],25);
                strncpy(Prenom_Admin, jsonDoc["Prenom_Admin"],25);
                strncpy(Chat_Id, jsonDoc["Chat_Id"],40);
                bot.updateToken(Bot_Token);
            } else Serial.println("failed to load jsonDoc config");
        }
        configFile.close();
    } else Serial.println("no config file");
}

//callback pour notifier un enregistrement nécessaire
void saveConfigCallback () {
    Serial.println("Should save config");
    shouldSaveConfig = true;
    /* - Lecture des parametres et Sauvegarde si besoin (shouldSaveConfig=true) - */
    strncpy(Nom_Bouton, custom_Nom_Bouton.getValue(),25);
    strncpy(Bot_Token, custom_Bot_Token.getValue(),70);
    strncpy(Nom_Admin, custom_Nom_Admin.getValue(),25);
    strncpy(Prenom_Admin, custom_Prenom_Admin.getValue(),25);
    strncpy(Chat_Id, custom_Chat_Id.getValue(),40);
    if (shouldSaveConfig) {
      String temp = "{\"Bot_Token\":\"" + String(Bot_Token) + "\",\"Chat_Id\":\"" + Chat_Id + "\",";
      temp += "\"Nom_Bouton\":\"" + String(Nom_Bouton) + "\",\"Prenom_Admin\":\"" + String(Prenom_Admin) + "\",";
      temp += "\"Nom_Admin\":\"" + String(Nom_Admin) + "\"}";
      File configFile = LittleFS.open("/config.json", "w");
      if (!configFile) {
          Serial.println("failed to open config file for writing");
      }
      configFile.print(temp);
      configFile.close();
    }
}

//Fonction pour faire une note NOTE (notation fr ou us)
//d'une durée DURATION en millisecondes.
void beep(int note, long duration) {
    tone(beeper, note, duration);
    delay(duration*1.10);               // Pour distinguer les notes on fait
}                                       // une pause de la durée de la note +30%

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

// Fonction lancée si interruption sur interrupteur
void interrupteurPressed(Button2& btn) {
  intPressed = true;
  if (compteur != 0) {
    Serial.print("cliked "); Serial.println(compteur);
    digitalWrite(gLed, LOW);
    digitalWrite(bLed, LOW);
    beepAlerte();
    // delai anti repetition
    delay(500);
  }
  compteur = compteur + 1;
}

void blinkLed(int ledPin, int msOn=100, int MsOff=500){
  switch (timerConnectBlink.cycleOnOffTrigger(msOn, MsOff)) {
    case 0:
      digitalWrite(ledPin, LOW);
      break;
    case 1:
      digitalWrite(ledPin, HIGH);
      break;
    case 2:
      break;
  }
}

// Fonction lancée si interruption sur switchbtn
void switchbtnPressed(Button2& btn) {
  trainingMode = !trainingMode;
  Serial.print("mode switch, training Mode = ");
  Serial.println(trainingMode);
}

// Formatage de l'heure en deux chiffres
String format22Digits(int number) {
    if(number<10)
        return ("0"+String(number));
    else
        return String(number);
}

// Mise a jour de l'heure et la date
void updateDateTime() {
    while (time(nullptr) <= 100000) {
        delay(50);
    }

    time(&maintenant);
    timeinfo = localtime(&maintenant);
    heure = format22Digits(timeinfo->tm_hour) + ":";
    heure = heure + format22Digits(timeinfo->tm_min) + ":";
    heure = heure + format22Digits(timeinfo->tm_sec);
    date = format22Digits(timeinfo->tm_mday) + "/";
    date = date + format22Digits(timeinfo->tm_mon +1) + "/";
    date = date + format22Digits(timeinfo->tm_year +1900);
}

void initTime() {
    if ((WiFi.status() == WL_CONNECTED)) {
        configTime(1 * 3600, 0, "fr.pool.ntp.org", "europe.pool.ntp.org", "pool.ntp.org");
        updateDateTime();
    }
}

// Chargement de l'historique préexistant
void file2json() {
	File eventsFile = LittleFS.open("/events.json", "r");
	if (eventsFile) {
		json = eventsFile.readString();
	}
	eventsFile.close();
}

// Ajout évenement dans historique
void addEvent(String event) {
    event = String(Nom_Bouton) + " - " + event;
    updateDateTime();
    int i = 0;
    int j = 0;
    int cnt = 0;
    // Comptage des enregistrements
    while(i!=-1) {
        if (i==0) {
            j = json.indexOf("'}'", i+1);
            cnt++;
        }
        i = json.indexOf("}", i+1);
        cnt++;
    }
    // Retrait du premier enregistrement si > historicSize
    if (cnt>historicSize+1) {
        json = "[" + json.substring(j+2);
    }
    // Ajout de la derniere alerte
    int li = json.length()-1;
    json.remove(li);
    if (json.length()>1)	json += ",";
    json += "{\"date\":\""+date+"\",\"heure\":\""+heure+"\",\"event\":\""+event+"\"}]";
    // Ecriture du fichier
    File eventsFile = LittleFS.open("/events.json", "w");
    if (eventsFile) {
        eventsFile.print(json);
        eventsFile.close();
    }
    else {
        Serial.println("failed to open config file for writing");
    }

}

// Envoi d'un telegramme avec un message msg vers un interlocuteur précis
void sendTelegramMessage(String msg, String chatid="") {

    // DEBUG
    Serial.print(chatid);
    Serial.print("\t");
    Serial.println(Bot_Token);
    // DEBUG
    
    msg =  date + "\t" + heure +"\t" + String(Nom_Bouton) + " - " +  msg;

    if(bot.sendMessage(chatid, msg)){
        msg = "Alerte à " + chatid;
        addEvent(msg);
    } else {
        msg = "Message non abouti à " + chatid;
        addEvent(msg);
    }
}

// Fonctions page Web
void gettoken() {
    server.send(200, "text/plain", Bot_Token);
}
void getbtnnom() {
    server.send(200, "text/plain", Nom_Bouton);
}
void getNom_Admin() {
    server.send(200, "text/plain", Nom_Admin);
}
void getPrenom_Admin() {
    server.send(200, "text/plain", Prenom_Admin);
}
void getChat_Id() {
    server.send(200, "text/plain", Chat_Id);
}
void getTestTelegram() {
    String Chat_Id = "";
    if (server.arg("Chat_Idsend")== "") {     //Parameter not found
        Serial.println("Argument not found");
    } else {     //Parameter found
        Chat_Id = server.arg("Chat_Idsend");
        sendTelegramMessage("test reussi !", Chat_Id);
    }
    server.send(200);
}

void sauveconfig() {
    String temp = "{\"Nom_Bouton\":\""+server.arg("NomBouton")+"\",\"Bot_Token\":\"";
    temp += server.arg("Bot_ID") +"\",\"Nom_Admin\":\""+server.arg("Nom_Adminsend");
    temp += "\",\"Prenom_Admin\":\""+server.arg("Prenom_Adminsend")+"\",\"Chat_Id\":\"";
    temp += server.arg("Chat_Idsend")+"\"}";
    File configFile = LittleFS.open("/config.json", "w");
    if (!configFile) {
        Serial.println("failed to open config file for writing");
    }
    configFile.print(temp);
    configFile.close();
    readConfig();
    server.send(200);
}

void sauvereferent() {
    String temp = server.arg("jsonstr");
    JsonDocument jsonTemp;
    deserializeJson(jsonTemp, temp);
    File referentFile = LittleFS.open("/referent.json", "w");
    if (!referentFile) {
        Serial.println("failed to open config file for writing");
    }
    serializeJsonPretty(jsonTemp, referentFile);
    serializeJsonPretty(jsonTemp, Serial);
    referentFile.close();
    server.send(200);
}

void updatereferent() {
    File referentFile = LittleFS.open("/referent.json", "r");
    if (!referentFile) {
        Serial.println("failed to open config file for writing");
        return;
    }
    JsonDocument jsonDoc;
    deserializeJson(jsonDoc, referentFile);
    referentFile.close();
    JsonArray arr = jsonDoc.as<JsonArray>();
    for (JsonObject value : arr) {
        if (value["ChatId"] == server.arg("ChatId")) {
                referentFile = LittleFS.open("/referent.json", "w");
                JsonDocument jsonObj;
                if(value["active"] =="0") value["active"] ="1";
                else  value["active"]="0";
        }
    }
    serializeJsonPretty(arr, referentFile);
    referentFile.close();
    server.send(200);
}

void addreferent() {
    File referentFile = LittleFS.open("/referent.json", "r");
    if (!referentFile) {
        Serial.println("failed to open config file for writing");
        return;
    }
    JsonDocument jsonDoc;
    deserializeJson(jsonDoc, referentFile);
    referentFile.close();
    JsonArray arr = jsonDoc.as<JsonArray>();
    for (JsonObject value : arr) {
        if (value["ChatId"] == server.arg("ChatId")) {
            server.send(200);
            return;
        }
    }
    if (arr.size()>=5) {
        server.send(200);
        return;
    }
    referentFile = LittleFS.open("/referent.json", "w");
    JsonDocument jsonObj;
    jsonObj["Nom"] = server.arg("Nom");
    jsonObj["Prenom"] = server.arg("Prenom");
    jsonObj["ChatId"] = server.arg("ChatId");
    jsonObj["active"] = "0";
    arr.add(jsonObj);
    serializeJsonPretty(arr, referentFile);
    referentFile.close();
    server.send(200);
}

//--------------------------------------------------------------------------------------//
//                                                                                      //
//                                        SETUP                                         //
//                                                                                      //
//--------------------------------------------------------------------------------------//

void setup() {

  // wm.resetSettings();

  WiFi.disconnect(); // au cas où pour l'instant
  // Réveil du wifi
  WiFi.forceSleepWake();
  delay( 500 );
  // Initialisation du WIFI
//   WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP 
  // Initialisation du Serial
  Serial.begin(115200);
  while (!Serial); 
  Serial.println("ready");
  // Initialisation Systeme de fichier
  if (!LittleFS.begin()) Serial.println("failed to mount FS");
  // Initialisation Leds
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(rLed, OUTPUT);
  pinMode(gLed, OUTPUT);
  pinMode(bLed, OUTPUT);
  digitalWrite(rLed, LOW);
  digitalWrite(gLed, LOW);
  digitalWrite(bLed, LOW);
  digitalWrite(LED_BUILTIN, LOW);
  // Initialisation Beeper
  pinMode(beeper, OUTPUT);

  interrupteur.begin(interrupteurPin, INPUT_PULLUP, true);
  interrupteur.setPressedHandler(interrupteurPressed);

  switchbtn.begin(switchPin, INPUT_PULLUP, true);
  switchbtn.setPressedHandler(switchbtnPressed);
  switchbtn.setDebounceTime(100);

  // Déclaration du callBack pour la sauvegarde
  wm.setSaveConfigCallback(saveConfigCallback);
  // Personnalisatoin du head de la page
  wm.setCustomHeadElement("<p><br>DIAO</p>");
  // Ajout des paramètres personnalisés
  wm.addParameter(&custom_Bot_Token);
  wm.addParameter(&custom_Nom_Bouton);
  wm.addParameter(&custom_Nom_Admin);
  wm.addParameter(&custom_Prenom_Admin);
  wm.addParameter(&custom_Chat_Id);

  // Initialisation du HostName du module
  wifi_station_set_hostname("Module_DIAO85");
  
  client.setTrustAnchors(&cert); // Add root certificate for api.telegram.org
}

/* -------------------------------------------------------------------------- */
/*                                    LOOP                                    */
/* -------------------------------------------------------------------------- */

void loop() {

  switchbtn.loop();

  // Training Mode
  if (trainingMode==true) {

    digitalWrite(gLed, HIGH);
    digitalWrite(bLed, LOW);

    interrupteur.loop();

    if (portalRunning==true) {
      beepModeChange();
      wm.stopWebPortal();
      WiFi.mode( WIFI_OFF );
      Serial.println(WiFi.status());
      portalRunning=false;
    }
  }
  // Normal Mode
  else {
    //***************************************************************//
    //*********************** Etat connecté *************************//
    //***************************************************************//
    if (WiFi.status() == WL_CONNECTED) {
            
        wm.process();   

        digitalWrite(gLed, LOW);
        digitalWrite(bLed, LOW);

        // signalRSSI = WiFi.RSSI();
        // Serial.println(signalRSSI);

        //***************************************************************//
        //*********************** 1 ere connexion ***********************//
        //***************************************************************//
        if (connected== false) {
            connected = true;

            /* ------------------- Configuration de la date et de l'heure ------------------- */
            initTime();
            /* ------------------- Récupération de l'historique stocké ------------------ */
            file2json();
            /* ------------------------ Démarrage du serveur Web ------------------------ */
            if (MDNS.begin("esp8266")) { Serial.println("MDNS responder started"); }

            server.serveStatic("/", LittleFS, "/index.html");
            server.serveStatic("/histo.json", LittleFS, "/events.json");
            server.serveStatic("/referent.json", LittleFS, "/referent.json");
            server.serveStatic("/logodiao.svg", LittleFS, "/logodiao.svg");
            server.serveStatic("/config.json", LittleFS, "/config.json");
            server.serveStatic("/bootstrap-table.min.js", LittleFS, "/bootstrap-table.min.js");
            server.serveStatic("/bootstrap.min.css", LittleFS, "/bootstrap.min.css");
            server.serveStatic("/bootstrap.min.js", LittleFS, "/bootstrap.min.js");
            server.serveStatic("/jquery-3.3.1.min.js", LittleFS, "/jquery-3.3.1.min.js");
            server.serveStatic("/popper.min.js", LittleFS, "/popper.min.js");
            server.serveStatic("/popper.min.js.map", LittleFS, "/popper.min.js.map");
            server.serveStatic("/bootstrap.min.js.map", LittleFS, "/bootstrap.min.js.map");
            server.serveStatic("/bootstrap.min.css.map", LittleFS, "/bootstrap.min.css.map");
            
            server.on("/gettoken",gettoken);
            server.on("/getbtnnom",getbtnnom);
            server.on("/getNom_Admin",getNom_Admin);
            server.on("/getPrenom_Admin",getPrenom_Admin);
            server.on("/getChat_Id",getChat_Id);
            server.on("/test", getTestTelegram);
            server.on("/sauve_config", sauveconfig);
            server.on("/referents", sauvereferent);
            server.on("/referentsadd", addreferent);
            server.on("/sauvReferents", sauvereferent);
            server.on("/updateReferents", updatereferent);

            server.begin();

            beepModeChange();
        } 
        //***************************************************************//
        //******************** Si pas 1ere connexion ********************//
        //***************************************************************// 
        else { 
            wm.process();   
            MDNS.update();
            server.handleClient();
            interrupteur.loop();
        }

    }
    //***************************************************************//
    //********************* Etat non connecté ***********************//
    //***************************************************************//
    else {

      connected = false;
      digitalWrite(gLed, LOW);
      blinkLed(bLed, 100, 500);

      if (timerNoWifiSound.cycleTrigger(3000))
      {
        beepDeconnexion();
      }

      if (portalRunning==false) {
        // WiFi.mode(WIFI_STA);
        readConfig();
        portalRunning = true;
        wm.setConfigPortalBlocking(false);
        wm.autoConnect("Bouton_d'alerte", "diao85100");
      } else {
        wm.process();
        delay(100);
      }
    }
  }
  intPressed = false;
}

