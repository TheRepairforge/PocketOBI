#pragma once
// i18n string data extracted from PocketOBI.ino.
// Data only: the Lang/StrId enums, the language codes, and the string table. The
// tr() accessor and the mutable `lang` selector are logic/state and stay in the sketch.
// `const` gives these internal linkage, so a header definition is safe (single TU today,
// no ODR clash if a future split includes it in several).

// ---------- i18n (string table; GFX fonts are ASCII-only, so no accents:
// FR/ES accents dropped, German umlauts as ae/oe/ue/ss) ----------
// LANG_COUNT = the four columns of STRTAB below. Japanese and Korean (D19, D58) are NOT
// STRTAB columns: their strings live in strings_ja.h / strings_ko.h and only exist when
// ENABLE_CJK=1, so this table stays four-wide for PackScope/tools/gen_locales.py.
// The language ids are PINNED (D58): they are saved in NVS, so a new language takes the
// next free number and never shifts another one. Vietnamese (D50, D60) is an ASCII column in
// strings_vi.h, also outside STRTAB. Which ones the user can pick is the build's LANG_AVAIL[]
// (cjk_render.h).
enum Lang { LANG_EN = 0, LANG_FR = 1, LANG_DE = 2, LANG_ES = 3, LANG_COUNT = 4,
            LANG_JA = 4, LANG_KO = 5, LANG_VI = 6 };
const char* const LANG_CODE[LANG_COUNT] = { "EN", "FR", "DE", "ES" };

enum StrId {
  S_BATTERY, S_REPAIR, S_TOOLS, S_ABOUT,                       // launcher (order = tiles)
  S_PC_BRIDGE, S_LEDS_ON, S_LEDS_OFF, S_RESET_ERROR, S_DEBUG_RAW, S_SETTINGS, // tools (order)
  S_FLIP, S_BRIDGE_BOOT, S_LANGUAGE,                           // settings (order)
  S_HEALTHY, S_UNLOCK, S_HARDWARE_FIX, S_SUSPECT_HW, S_NO_PACK, // verdict
  S_HDR_HEALTH, S_HDR_IDENTITY,                                // headers
  // Battery Overview + Health + Identity labels
  S_PACK, S_TEMP, S_SPREAD, S_CHARGES,
  S_CONDITION, S_OVERDISCHARGE, S_OVERLOAD, S_CELLS, S_LATCHED,
  S_MODEL, S_SN, S_CAPACITY, S_TYPE, S_PRODUCED, S_ASSEMBLED,
  // Repair diag rows + values + prediction + hint
  S_CHARGER_LOCK, S_LATCHED_FAULT, S_THERMISTOR, S_CELL_BALANCE,
  S_NONE, S_YES, S_OKV, S_FAULTV, S_BADV,
  S_WILL_HOLD, S_WONT_HOLD, S_NOT_LOCKED,
  // Repair confirm / result
  S_REPAIRS_FALSE_ONLY,
  S_BEFORE, S_AFTER, S_RETRY_REINSERT,
  // Reset
  S_LOCKEDV, S_OKSTATE,
  // About / toasts / common hints
  S_SCAN,
  S_LEDS_ON_MSG, S_LEDS_OFF_MSG, S_CONNECT_MSG,
  // Repair diagnostic engine (wizard)
  S_NO_DIAGNOSIS,
  S_HW_SENSE, S_HW_WEAK, S_HW_IMB, S_HW_THERM,
  S_ACT_SENSE, S_ACT_WEAK, S_ACT_IMB, S_ACT_THERM,
  S_FIX_HW_FIRST, S_UNLIKELY_HOLD, S_SHOULD_HOLD,
  S_NO_LOCK_NO_FAULT, S_BMS_MEMORISED, S_FALSE_LOCKOUT,
  S_PACK_HEALTHY, S_WHY_OD, S_WHY_OL, S_WHY_WEAR, S_WHY_UNCLEAR, S_WHY_THERM_CHECK, S_UNLOCK_SHOULD_HOLD,
  S_TOAST_FIXHW, S_TOAST_NOTHING, S_WORKING, S_FRAME_STORE_PC,
  S_TEMP_CB,   // "Cell/Board" temp-row label (Temp1=cell, Temp2=board)
  S_CALIBRATE, // Settings > Calibrate touch (CYD touch board only)
  S_CAL_TAP, S_CAL_SAVED,  // touch calibration screen (CYD only)
  // v2.5 shell (UI guideline M2): footer hints in the grammar "Input: action", one action per
  // string, joined by the screen; sentence-case headers. The pre-M2 hint/header strings were dropped
  // with their PackScope catalog keys (BACKLOG #67).
  S_H_TURN_PAGE, S_H_TURN_SELECT, S_H_TURN_TOGGLE, S_H_TURN_CANCEL, S_H_CLICK_OPEN, S_H_CLICK_BACK, S_H_CLICK_CONFIRM, S_H_CLICK_CONTINUE, S_H_CLICK_TOGGLE, S_H_CLICK_REFRESH, S_H_BACK_MENU, S_H_BACK_TOOLS, S_H_PLEASE_WAIT,
  S_HDR_RESET_Q, S_HDR_RESET_RESULT, S_HDR_COMM_ERROR, S_HDR_UNLOCK_Q, S_HDR_UNLOCKING, S_HDR_UNLOCK_RESULT,
  // v2.5 status words (D46 rule 6, M4): CAPS, at most two words, no '!' or '->'; and their subtitles.
  S_W_UNLOCKED, S_W_STILL_LOCKED, S_W_NOT_REREAD, S_W_CLEARED, S_W_UNCHANGED, S_W_NO_ERROR, S_W_NOTHING_TO_DO, S_W_NOT_SUPPORTED, S_W_NO_RESPONSE, S_W_ACTIVE, S_W_OFF,
  S_SUB_FRAME_OK, S_SUB_F0513, S_SUB_NO_ANSWER, S_SUB_BRIDGE_ON, S_SUB_BRIDGE_OFF, S_SUB_REREAD_OK, S_SUB_RELOCKS, S_SUB_ALREADY_OK,
  S_SUB_INSERT_PACK, S_SUB_CELL_DIFF,   // M5 (cell voltage spread: S_SPREAD is the temperature one in JA)
  // M6 Repair shell (shared words reuse the shared translations)
  S_SUB_NO_REPLY, S_LBL_TRY, S_STEP_ADAPTER, S_STEP_DATA_EN, S_STEP_DEAD_BMS, S_LBL_WHAT, S_WHAT_FLASH, S_BTN_CANCEL, S_BTN_UNLOCK, S_KEEP_PACK, S_STEP_FMT, S_CAP_REREAD, S_LBL_VERDICT_NOW,
  // M7 Reset error
  S_WHAT_RESET, S_BTN_RESET, S_ERR_NOW_FMT, S_CAP_ERRFLAG,
  // M8 Comm error steps, Tools write badge
  S_STEP_DATA, S_STEP_PULLUPS, S_STEP_RESEAT, S_BADGE_WRITE, S_WRITE_LEGEND,
  // CYD touch hints (no encoder: tap the item, the title bar is Back)
  S_T_TAP_OPEN, S_T_TAP_BACK, S_T_TAP_CONTINUE, S_T_TAP_TOGGLE, S_T_CENTRE_REFRESH, S_T_EDGES_PAGE, S_T_TAP_BUTTON, S_T_TITLE_BACK, S_T_BELOW_PAGE,
  // M9 PC bridge
  S_LBL_HOWTO, S_HOWTO_CAP,
  // 2.5.1: honest caveat on the unlock confirm for an F0513-generation pack (cell path,
  // boardTempValid=false). The bench showed the frame write no-ops on this old BMS.
  S_W_F0513_UNVALIDATED,
  S_COUNT
};
const char* const STRTAB[S_COUNT][LANG_COUNT] = {
  //                  EN                 FR                 DE                   ES
  /*S_BATTERY*/     { "Battery",         "Batterie",        "Akku",              "Bateria" },
  /*S_REPAIR*/      { "Repair",          "Reparer",         "Reparatur",         "Reparar" },
  /*S_TOOLS*/       { "Tools",           "Outils",          "Werkzeuge",         "Utiles" },
  /*S_ABOUT*/       { "About",           "A propos",        "Info",              "Acerca" },
  /*S_PC_BRIDGE*/   { "PC bridge",       "Pont PC",         "PC-Bruecke",        "Puente PC" },
  /*S_LEDS_ON*/     { "Pack LEDs on",    "LEDs pack ON",    "Akku-LEDs an",      "LEDs bat. ON" },
  /*S_LEDS_OFF*/    { "Pack LEDs off",   "LEDs pack OFF",   "Akku-LEDs aus",     "LEDs bat. OFF" },
  /*S_RESET_ERROR*/ { "Reset error",     "Reset erreur",    "Fehler-Reset",      "Reset error" },
  /*S_DEBUG_RAW*/   { "Debug / raw",     "Debug / brut",    "Debug / roh",       "Debug / bruto" },
  /*S_SETTINGS*/    { "Settings",        "Reglages",        "Einstellungen",     "Ajustes" },
  /*S_FLIP*/        { "Flip screen",     "Ecran inverse",   "Bild drehen",       "Girar pantalla" },
  /*S_BRIDGE_BOOT*/ { "PC bridge at boot","Pont PC au boot","PC-Bruecke b.Start","Puente al inicio" },
  /*S_LANGUAGE*/    { "Language",        "Langue",          "Sprache",           "Idioma" },
  /*S_HEALTHY*/     { "HEALTHY",         "SAINE",           "GESUND",            "SANA" },
  /*S_UNLOCK*/      { "UNLOCK",          "DEBLOQUER",       "ENTSPERREN",        "DESBLOQUEAR" },
  /*S_HARDWARE_FIX*/{ "HARDWARE FIX",    "REPA MATERIEL",   "HARDWARE-REP.",     "REP. HARDWARE" },
  /*S_SUSPECT_HW*/  { "POSSIBLE HW FIX", "REPA MAT. POSS.", "EVTL. HARDWARE",    "POSIBLE REP." },
  /*S_NO_PACK*/     { "NO PACK",         "PAS DE PACK",     "KEIN AKKU",         "SIN BATERIA" },
  /*S_HDR_HEALTH*/  { "Health",          "Sante",           "Zustand",           "Salud" },
  /*S_HDR_IDENTITY*/{ "Identity",        "Identite",        "Identitaet",        "Identidad" },
  /*S_PACK*/        { "PACK",            "PACK",             "PACK",              "PACK" },
  /*S_TEMP*/        { "Temp",            "Temp",             "Temp",              "Temp" },
  /*S_SPREAD*/      { "Spread",          "Ecart",            "Diff.",             "Dif." },
  /*S_CHARGES*/     { "Charges",         "Charges",          "Zyklen",            "Cargas" },
  /*S_CONDITION*/   { "Condition",       "Etat",             "Zustand",           "Estado" },
  /*S_OVERDISCHARGE*/{ "Over-discharge", "Sur-decharge",     "Tiefentladung",     "Sobredescarga" },
  /*S_OVERLOAD*/    { "Over-load",       "Surcharge",        "Ueberlast",         "Sobrecarga" },
  /*S_CELLS*/       { "Cells",           "Cellules",         "Zellen",            "Celdas" },
  /*S_LATCHED*/     { "Latched",         "Verrou dur",       "Verriegelt",        "Bloqueo fijo" },
  /*S_MODEL*/       { "Model",           "Modele",           "Modell",            "Modelo" },
  /*S_SN*/          { "S/N",             "S/N",              "S/N",               "S/N" },
  /*S_CAPACITY*/    { "Capacity",        "Capacite",         "Kapazitaet",        "Capacidad" },
  /*S_TYPE*/        { "Type",            "Type",             "Typ",               "Tipo" },
  /*S_PRODUCED*/    { "Produced",        "Fabrique",         "Hergestellt",       "Fabricado" },
  /*S_ASSEMBLED*/   { "Assembled",       "Assemble",         "Montiert",          "Ensamblado" },
  /*S_CHARGER_LOCK*/{ "Charger lock",    "Verrou chargeur",  "Ladesperre",        "Bloqueo carga" },
  /*S_LATCHED_FAULT*/{ "Latched fault",  "Defaut latche",    "Fehler fix",        "Fallo fijo" },
  /*S_THERMISTOR*/  { "Thermistor",      "Thermistance",     "Thermistor",        "Termistor" },
  /*S_CELL_BALANCE*/{ "Cell balance",    "Equilibre cell",   "Zellbalance",       "Balance celda" },
  /*S_NONE*/        { "none",            "aucun",            "keine",             "ninguno" },
  /*S_YES*/         { "yes",             "oui",              "ja",                "si" },
  /*S_OKV*/         { "ok",              "ok",               "ok",                "ok" },
  /*S_FAULTV*/      { "fault",           "defaut",           "Fehler",            "fallo" },
  /*S_BADV*/        { "bad",             "mauvais",          "schlecht",          "malo" },
  /*S_WILL_HOLD*/   { "WILL HOLD",       "VA TENIR",         "HAELT",             "AGUANTA" },
  /*S_WONT_HOLD*/   { "WON'T HOLD",      "NE TIENDRA PAS",   "HAELT NICHT",       "NO AGUANTA" },
  /*S_NOT_LOCKED*/  { "NOT LOCKED",      "NON VERROUILLE",   "NICHT GESPERRT",    "NO BLOQUEADO" },
  /*S_REPAIRS_FALSE_ONLY*/{ "Repairs a false lockout only.","Repare un faux verrou seulement.","Nur Fehlsperre reparieren.","Solo repara bloqueo falso." },
  /*S_BEFORE*/      { "Before",          "Avant",            "Vorher",            "Antes" },
  /*S_AFTER*/       { "After",           "Apres",            "Nachher",           "Despues" },
  /*S_RETRY_REINSERT*/{ "To retry, remove & reinsert the pack.","Pour reessayer, retire et remets le pack.","Zum Wiederholen Akku neu einsetzen.","Reintentar: reinserta la bateria." },
  /*S_LOCKEDV*/     { "LOCKED",          "VERROUILLE",       "GESPERRT",          "BLOQUEADO" },
  /*S_OKSTATE*/     { "OK",              "OK",               "OK",                "OK" },
  /*S_SCAN*/        { "scan",            "scan",             "scan",              "escanea" },
  /*S_LEDS_ON_MSG*/ { "LEDs ON",         "LEDs ON",          "LEDs AN",           "LEDs ON" },
  /*S_LEDS_OFF_MSG*/{ "LEDs OFF",        "LEDs OFF",         "LEDs AUS",          "LEDs OFF" },
  /*S_CONNECT_MSG*/ { "Connecting...", "En connexion...",  "Verbinde...", "Conectando..."},
  /*S_NO_DIAGNOSIS*/{ "NO DIAGNOSIS",    "PAS DE DIAG",       "KEINE DIAGNOSE",    "SIN DIAGNOSTICO" },
  /*S_HW_SENSE*/    { "Broken sense wire","Fil de mesure casse","Sense-Draht defekt","Cable de sensado roto" },
  /*S_HW_WEAK*/     { "Weak / dead group","Groupe faible / mort","Gruppe schwach / tot","Grupo debil / muerto" },
  /*S_HW_IMB*/      { "Cell imbalance",   "Desequilibre cellules","Zellen unsymmetrisch","Desequilibrio celdas" },
  /*S_HW_THERM*/    { "Temp reading off", "Lecture temp anormale","Temp-Wert auffaellig","Lectura temp anomala" },
  /*S_ACT_SENSE*/   { "Resolder sense wire on group %d","Ressoude le fil du groupe %d","Sense-Draht Gruppe %d loeten","Resuelda sensor grupo %d" },
  /*S_ACT_WEAK*/    { "Measure group %d with a DMM","Mesure le groupe %d au multimetre","Gruppe %d mit DMM messen","Mide grupo %d con multimetro" },
  /*S_ACT_IMB*/     { "Group %d low: slow-charge, recheck","Groupe %d bas: charge lente, recontrole","Gruppe %d niedrig: langsam laden","Grupo %d bajo: carga lenta, revisa" },
  /*S_ACT_THERM*/   { "Re-read cold, then check NTC","Relire a froid, puis controler NTC","Kalt messen, dann NTC pruefen","Medir en frio, luego revisar NTC" },
  /*S_FIX_HW_FIRST*/{ "FIX HARDWARE FIRST","REPARER LE MATERIEL","ERST HARDWARE FIXEN","REPARA HARDWARE" },
  /*S_UNLIKELY_HOLD*/{ "UNLIKELY TO HOLD","PEU PROBABLE",      "HAELT WOHL NICHT",  "POCO PROBABLE" },
  /*S_SHOULD_HOLD*/ { "SHOULD HOLD",      "DEVRAIT TENIR",    "SOLLTE HALTEN",     "DEBERIA AGUANTAR" },
  /*S_NO_LOCK_NO_FAULT*/{ "No lock, no fault","Pas de verrou, pas de faute","Keine Sperre, kein Fehler","Sin bloqueo ni fallo" },
  /*S_BMS_MEMORISED*/{ "BMS memorised a fault","Le BMS a memorise une faute","BMS hat Fehler gespeichert","El BMS memorizo un fallo" },
  /*S_FALSE_LOCKOUT*/{ "False lockout",   "Faux verrouillage","Falsche Sperre",    "Bloqueo falso" },
  /*S_PACK_HEALTHY*/{ "Pack looks healthy.","Pack a l'air sain.","Akku wirkt gesund.","Bateria parece sana." },
  /*S_WHY_OD*/      { "Likely left flat (over-discharge).","Laisse a plat (sur-decharge).","Wohl tiefentladen.","Descargada del todo." },
  /*S_WHY_OL*/      { "Likely run in overload.","Utilise en surcharge.","Wohl ueberlastet.","Usada en sobrecarga." },
  /*S_WHY_WEAR*/    { "Consistent with normal wear.","Coherent avec usure normale.","Normaler Verschleiss.","Coherente con desgaste." },
  /*S_WHY_UNCLEAR*/ { "Cause unclear - see Health counters.","Cause floue - voir compteurs Sante.","Ursache unklar - Zustand pruefen.","Causa incierta - ver Salud." },
  /*S_WHY_THERM_CHECK*/{ "Cell/board temps differ - recheck","Temp cell/carte diverge - relire","Zell/Board-Temp weicht ab - neu","Temp celda/placa difiere - revisar" },
  /*S_UNLOCK_SHOULD_HOLD*/{ "Our unlock should hold.","Notre unlock devrait tenir.","Unser Entsperren sollte halten.","El desbloqueo deberia aguantar." },
  /*S_TOAST_FIXHW*/ { "Fix hardware first","Repare le materiel","Erst Hardware fixen","Repara hardware" },
  /*S_TOAST_NOTHING*/{ "Nothing to repair","Rien a reparer",   "Nichts zu reparieren","Nada que reparar" },
  /*S_WORKING*/     { "Working...",       "Travail...",       "Arbeite...",        "Trabajando..." },
  /*S_FRAME_STORE_PC*/{ "repair frame  ->  store  ->  power-cycle","repare -> stocke -> power-cycle","Frame -> speichern -> Neustart","repara -> guarda -> reinicio" },
  /*S_TEMP_CB*/     { "Cell/Board",      "Cell/Carte",       "Zelle/Platine",     "Celda/Placa" },
  /*S_CALIBRATE*/   { "Calibrate touch", "Calibrer tactile", "Touch kalibrieren", "Calibrar tactil" },
  /*S_CAL_TAP*/     { "Tap the target",  "Touchez la cible", "Ziel antippen",     "Toca el objetivo" },
  /*S_CAL_SAVED*/   { "Saved",           "Enregistre",       "Gespeichert",       "Guardado" },
  /*S_H_TURN_PAGE*/ { "Turn: page", "Tourner: page", "Drehen: Seite", "Girar: pagina" },
  /*S_H_TURN_SELECT*/ { "Turn: select", "Tourner: choisir", "Drehen: waehlen", "Girar: elegir" },
  /*S_H_TURN_TOGGLE*/ { "Turn: toggle", "Tourner: basculer", "Drehen: umschalten", "Girar: alternar" },
  /*S_H_TURN_CANCEL*/ { "Turn: cancel", "Tourner: annuler", "Drehen: abbrechen", "Girar: cancelar" },
  /*S_H_CLICK_OPEN*/ { "Click: open", "Clic: ouvrir", "Klick: oeffnen", "Clic: abrir" },
  /*S_H_CLICK_BACK*/ { "Click: back", "Clic: retour", "Klick: zurueck", "Clic: atras" },
  /*S_H_CLICK_CONFIRM*/ { "Click: confirm", "Clic: confirmer", "Klick: bestaetigen", "Clic: confirmar" },
  /*S_H_CLICK_CONTINUE*/ { "Click: continue", "Clic: continuer", "Klick: weiter", "Clic: continuar" },
  /*S_H_CLICK_TOGGLE*/ { "Click: toggle", "Clic: basculer", "Klick: aendern", "Clic: cambiar" },
  /*S_H_CLICK_REFRESH*/ { "Click: refresh", "Clic: relire", "Klick: neu lesen", "Clic: releer" },
  /*S_H_BACK_MENU*/ { "Back: menu", "Retour: menu", "Zurueck: Menu", "Atras: menu" },
  /*S_H_BACK_TOOLS*/ { "Back: tools", "Retour: outils", "Zurueck: Werkz.", "Atras: herram." },
  /*S_H_PLEASE_WAIT*/ { "Please wait", "Patientez", "Bitte warten", "Espere" },
  /*S_HDR_RESET_Q*/ { "Reset error?", "Reset erreur ?", "Fehler-Reset?", "Reset error?" },
  /*S_HDR_RESET_RESULT*/ { "Reset result", "Resultat reset", "Reset-Ergebnis", "Resultado reset" },
  /*S_HDR_COMM_ERROR*/ { "Comm error", "Erreur comm.", "Kommfehler", "Error comm." },
  /*S_HDR_UNLOCK_Q*/ { "Unlock pack?", "Debloquer ?", "Entsperren?", "Desbloquear?" },
  /*S_HDR_UNLOCKING*/ { "Unlocking", "Deblocage", "Entsperrung", "Desbloqueando" },
  /*S_HDR_UNLOCK_RESULT*/ { "Unlock result", "Resultat deblocage", "Entsperr-Ergebnis", "Resultado desbl." },
  /*S_W_UNLOCKED*/ { "UNLOCKED", "DEBLOQUE", "ENTSPERRT", "DESBLOQUEADO" },
  /*S_W_STILL_LOCKED*/ { "STILL LOCKED", "TOUJOURS VERROU", "NOCH GESPERRT", "SIGUE BLOQUEADO" },
  /*S_W_NOT_REREAD*/ { "NOT RE-READ", "NON RELU", "NICHT GELESEN", "SIN RELECTURA" },
  /*S_W_CLEARED*/ { "CLEARED", "EFFACE", "GELOESCHT", "BORRADO" },
  /*S_W_UNCHANGED*/ { "UNCHANGED", "INCHANGE", "UNVERAENDERT", "SIN CAMBIO" },
  /*S_W_NO_ERROR*/ { "NO ERROR", "AUCUNE ERREUR", "KEIN FEHLER", "SIN ERROR" },
  /*S_W_NOTHING_TO_DO*/ { "NOTHING TO DO", "RIEN A FAIRE", "NICHTS ZU TUN", "NADA QUE HACER" },
  /*S_W_NOT_SUPPORTED*/ { "NOT SUPPORTED", "NON SUPPORTE", "NICHT UNTERST.", "NO SOPORTADO" },
  /*S_W_NO_RESPONSE*/ { "NO RESPONSE", "PAS DE REPONSE", "KEINE ANTWORT", "SIN RESPUESTA" },
  /*S_W_ACTIVE*/ { "ACTIVE", "ACTIF", "AKTIV", "ACTIVO" },
  /*S_W_OFF*/ { "OFF", "ARRET", "AUS", "APAGADO" },
  /*S_SUB_FRAME_OK*/ { "Frame valid - no write done", "Trame valide - aucune ecriture", "Frame gueltig - kein Schreiben", "Trama valida - sin escritura" },
  /*S_SUB_F0513*/ { "F0513 (old BMS)", "F0513 (ancien BMS)", "F0513 (alter BMS)", "F0513 (BMS antiguo)" },
  /*S_SUB_NO_ANSWER*/ { "The pack did not answer", "La batterie ne repond pas", "Akku antwortet nicht", "La bateria no responde" },
  /*S_SUB_BRIDGE_ON*/ { "USB bridge on, PC mode", "Pont USB actif, mode PC", "USB-Bruecke an, PC-Modus", "Puente USB activo, modo PC" },
  /*S_SUB_BRIDGE_OFF*/ { "USB bridge inactive", "Pont USB inactif", "USB-Bruecke aus", "Puente USB inactivo" },
  /*S_SUB_REREAD_OK*/ { "Re-read after reset: OK", "Relu apres reset: OK", "Nach Reset gelesen: OK", "Releido tras reset: OK" },
  /*S_SUB_RELOCKS*/ { "Re-locks: real fault", "Reverrouille: vrai defaut", "Sperrt erneut: echter Fehler", "Se rebloquea: fallo real" },
  /*S_SUB_ALREADY_OK*/ { "Pack was already OK", "Le pack etait deja OK", "Akku war schon OK", "La bateria ya estaba OK" },
  /*S_SUB_INSERT_PACK*/ { "Insert a pack on the adapter", "Place un pack sur l'adaptateur", "Akku auf den Adapter setzen", "Coloca una bateria en el adaptador" },
  /*S_SUB_CELL_DIFF*/ { "Diff", "Ecart", "Diff.", "Dif." },
  /*S_SUB_NO_REPLY*/ { "No pack reply", "Pas de reponse du pack", "Keine Antwort vom Akku", "Sin respuesta" },
  /*S_LBL_TRY*/ { "Try this", "A essayer", "Versuchen", "Probar" },
  /*S_STEP_ADAPTER*/ { "Check the adapter", "Verifie l'adaptateur", "Adapter pruefen", "Revisa el adaptador" },
  /*S_STEP_DATA_EN*/ { "Check DATA / ENABLE wiring", "Verifie DATA / ENABLE", "DATA/ENABLE pruefen", "Revisa DATA / ENABLE" },
  /*S_STEP_DEAD_BMS*/ { "No reply at all: dead BMS", "Aucune reponse: BMS mort", "Gar keine Antwort: BMS tot", "Ninguna respuesta: BMS muerto" },
  /*S_LBL_WHAT*/ { "What happens", "Ce qui se passe", "Was passiert", "Que pasa" },
  /*S_WHAT_FLASH*/ { "Writes the BMS flash", "Ecrit la flash du BMS", "Schreibt den BMS-Flash", "Escribe la flash del BMS" },
  /*S_BTN_CANCEL*/ { "Cancel", "Annuler", "Abbrechen", "Cancelar" },
  /*S_BTN_UNLOCK*/ { "Unlock", "Debloquer", "Entsperren", "Desbloquear" },
  /*S_KEEP_PACK*/ { "Keep the pack connected", "Laisser la batterie branchee", "Akku angeschlossen lassen", "Mantener la bateria conectada" },
  /*S_STEP_FMT*/ { "Step %d/%d", "Etape %d/%d", "Schritt %d/%d", "Paso %d/%d" },
  /*S_CAP_REREAD*/ { "re-read", "relu", "neu gelesen", "releido" },
  /*S_LBL_VERDICT_NOW*/ { "Verdict now", "Verdict actuel", "Befund jetzt", "Veredicto ahora" },
  /*S_WHAT_RESET*/ { "Clears the BMS error flag", "Efface le drapeau d'erreur du BMS", "Loescht das BMS-Fehlerflag", "Borra el indicador de error del BMS" },
  /*S_BTN_RESET*/ { "Reset", "Reset", "Reset", "Reset" },
  /*S_ERR_NOW_FMT*/ { "Error now: %s", "Erreur actuelle: %s", "Fehler jetzt: %s", "Error ahora: %s" },
  /*S_CAP_ERRFLAG*/ { "error flag", "drapeau d'erreur", "Fehler-Flag", "indicador de error" },
  /*S_STEP_DATA*/ { "Check DATA wiring", "Verifie le cablage DATA", "DATA-Verdrahtung pruefen", "Revisa el cableado DATA" },
  /*S_STEP_PULLUPS*/ { "Check pull-ups and GND", "Verifie pull-ups et GND", "Pull-ups und GND pruefen", "Revisa pull-ups y GND" },
  /*S_STEP_RESEAT*/ { "Re-seat the pack", "Remets le pack en place", "Akku neu einsetzen", "Recoloca la bateria" },
  /*S_BADGE_WRITE*/ { "Write", "Ecrit", "Schreibt", "Escribe" },
  /*S_WRITE_LEGEND*/ { "sends a command to the pack", "envoie une commande au pack", "sendet einen Befehl an den Akku", "envia un comando a la bateria" },
  /*S_T_TAP_OPEN*/ { "Tap: open", "Toucher: ouvrir", "Tippen: oeffnen", "Tocar: abrir" },
  /*S_T_TAP_BACK*/ { "Tap: back", "Toucher: retour", "Tippen: zurueck", "Tocar: atras" },
  /*S_T_TAP_CONTINUE*/ { "Tap: continue", "Toucher: continuer", "Tippen: weiter", "Tocar: seguir" },
  /*S_T_TAP_TOGGLE*/ { "Tap: toggle", "Toucher: basculer", "Tippen: aendern", "Tocar: cambiar" },
  /*S_T_CENTRE_REFRESH*/ { "Centre: refresh", "Centre: relire", "Mitte: neu lesen", "Centro: releer" },
  /*S_T_EDGES_PAGE*/ { "Edges: page", "Bords: page", "Raender: Seite", "Bordes: pagina" },
  /*S_T_TAP_BUTTON*/ { "Tap a button", "Toucher un bouton", "Taste antippen", "Toca un boton" },
  /*S_T_TITLE_BACK*/ { "Title: back", "Titre: retour", "Titel: zurueck", "Titulo: atras" },
  /*S_T_BELOW_PAGE*/ { "Below: page", "Dessous: page", "Unten: Seite", "Abajo: pagina" },
  /*S_LBL_HOWTO*/ { "How to", "Mode d'emploi", "So geht's", "Como usar" },
  /*S_HOWTO_CAP*/ { "Arduino OBI interface + this COM port", "Interface Arduino OBI + ce port COM", "Schnittstelle Arduino OBI + COM-Port", "Interfaz Arduino OBI + este puerto COM" },
  /*S_W_F0513_UNVALIDATED*/ { "F0513 chip: unlock may not hold.", "Puce F0513: peut ne pas tenir.", "F0513-Chip: haelt evtl. nicht.", "Chip F0513: puede no aguantar." },
};
