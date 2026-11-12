#pragma once
// i18n string data extracted from PocketOBI.ino.
// Data only: the Lang/StrId enums, the language codes, and the string table. The
// tr() accessor and the mutable `lang` selector are logic/state and stay in the sketch.
// `const` gives these internal linkage, so a header definition is safe (single TU today,
// no ODR clash if a future split includes it in several).

// ---------- i18n (string table, UTF-8) ----------
// Strings are UTF-8 with their real accents (é, ü, ñ ...): the narrow text path draws them (narrow_render.h,
// docs/NARROW_GLYPHS.md). A letter it cannot draw is refused by tools/gen_narrow_glyphs.py --check, and a
// string over 63 bytes too (64-byte buffers): an accented letter is 2 bytes.
// LANG_COUNT = the four columns of STRTAB below. Japanese, Korean, Chinese (D19, D58, D70, D72) are NOT
// STRTAB columns: their strings live in strings_ja/ko/zh/zh_tw.h and only exist when
// ENABLE_CJK=1, so this table stays four-wide for PackScope/tools/gen_locales.py.
// The language ids are PINNED (D58): they are saved in NVS, so a new language takes the
// next free number and never shifts another one. Vietnamese (D50, D60) is a column in
// strings_vi.h, also outside STRTAB. Polish has id 9 reserved (no column yet: BACKLOG #82); Cyrillic
// (Russian, #81) would take the next one. Which ones the user can pick is the build's LANG_AVAIL[]
// (cjk_render.h).
enum Lang { LANG_EN = 0, LANG_FR = 1, LANG_DE = 2, LANG_ES = 3, LANG_COUNT = 4,
            LANG_JA = 4, LANG_KO = 5, LANG_VI = 6, LANG_ZH = 7, LANG_ZH_TW = 8, LANG_PL = 9 };
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
  S_TOAST_FIXHW, S_TOAST_NOTHING, S_TOAST_USE_RESET, S_TOAST_PACK_CHANGED, S_WORKING, S_FRAME_STORE_PC,
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
  // #35: the temperature unit, so a language can use its own sign (Japanese: U+2103).
  S_DEG_C,
  S_COUNT
};
const char* const STRTAB[S_COUNT][LANG_COUNT] = {
  //                  EN                 FR                 DE                   ES
  /*S_BATTERY*/     { "Battery",         "Batterie",        "Akku",              "Batería" },
  /*S_REPAIR*/      { "Repair",          "Réparer",         "Reparatur",         "Reparar" },
  /*S_TOOLS*/       { "Tools",           "Outils",          "Werkzeuge",         "Útiles" },
  /*S_ABOUT*/       { "About",           "À propos",        "Info",              "Acerca" },
  /*S_PC_BRIDGE*/   { "PC bridge",       "Pont PC",         "PC-Brücke",        "Puente PC" },
  /*S_LEDS_ON*/     { "Pack LEDs on",    "LEDs pack ON",    "Akku-LEDs an",      "LEDs bat. ON" },
  /*S_LEDS_OFF*/    { "Pack LEDs off",   "LEDs pack OFF",   "Akku-LEDs aus",     "LEDs bat. OFF" },
  /*S_RESET_ERROR*/ { "Reset error",     "Reset erreur",    "Fehler-Reset",      "Reset error" },
  /*S_DEBUG_RAW*/   { "Debug / raw",     "Debug / brut",    "Debug / roh",       "Debug / bruto" },
  /*S_SETTINGS*/    { "Settings",        "Réglages",        "Einstellungen",     "Ajustes" },
  /*S_FLIP*/        { "Flip screen",     "Écran inversé",   "Bild drehen",       "Girar pantalla" },
  /*S_BRIDGE_BOOT*/ { "PC bridge at boot","Pont PC au boot","PC-Brücke b.Start","Puente al inicio" },
  /*S_LANGUAGE*/    { "Language",        "Langue",          "Sprache",           "Idioma" },
  /*S_HEALTHY*/     { "HEALTHY",         "SAINE",           "GESUND",            "SANA" },
  /*S_UNLOCK*/      { "UNLOCK",          "DÉBLOQUER",       "ENTSPERREN",        "DESBLOQUEAR" },
  /*S_HARDWARE_FIX*/{ "HARDWARE FIX",    "RÉPA MATÉRIEL",   "HARDWARE-REP.",     "REP. HARDWARE" },
  /*S_SUSPECT_HW*/  { "POSSIBLE HW FIX", "RÉPA MAT. POSS.", "EVTL. HARDWARE",    "POSIBLE REP." },
  /*S_NO_PACK*/     { "NO PACK",         "PAS DE PACK",     "KEIN AKKU",         "SIN BATERÍA" },
  /*S_HDR_HEALTH*/  { "Health",          "Santé",           "Zustand",           "Salud" },
  /*S_HDR_IDENTITY*/{ "Identity",        "Identité",        "Identität",        "Identidad" },
  /*S_PACK*/        { "PACK",            "PACK",             "PACK",              "PACK" },
  /*S_TEMP*/        { "Temp",            "Temp",             "Temp",              "Temp" },
  /*S_SPREAD*/      { "Spread",          "Écart",            "Diff.",             "Dif." },
  /*S_CHARGES*/     { "Charges",         "Charges",          "Zyklen",            "Cargas" },
  /*S_CONDITION*/   { "Condition",       "État",             "Zustand",           "Estado" },
  /*S_OVERDISCHARGE*/{ "Over-discharge", "Sur-décharge",     "Tiefentladung",     "Sobredescarga" },
  /*S_OVERLOAD*/    { "Over-load",       "Surcharge",        "Überlast",         "Sobrecarga" },
  /*S_CELLS*/       { "Cells",           "Cellules",         "Zellen",            "Celdas" },
  /*S_LATCHED*/     { "Latched",         "Verrou dur",       "Verriegelt",        "Bloqueo fijo" },
  /*S_MODEL*/       { "Model",           "Modèle",           "Modell",            "Modelo" },
  /*S_SN*/          { "S/N",             "S/N",              "S/N",               "S/N" },
  /*S_CAPACITY*/    { "Capacity",        "Capacité",         "Kapazität",        "Capacidad" },
  /*S_TYPE*/        { "Type",            "Type",             "Typ",               "Tipo" },
  /*S_PRODUCED*/    { "Produced",        "Fabriqué",         "Hergestellt",       "Fabricado" },
  /*S_ASSEMBLED*/   { "Assembled",       "Assemblé",         "Montiert",          "Ensamblado" },
  /*S_CHARGER_LOCK*/{ "Charger lock",    "Verrou chargeur",  "Ladesperre",        "Bloqueo carga" },
  /*S_LATCHED_FAULT*/{ "Latched fault",  "Défaut latché",    "Fehler fix",        "Fallo fijo" },
  /*S_THERMISTOR*/  { "Thermistor",      "Thermistance",     "Thermistor",        "Termistor" },
  /*S_CELL_BALANCE*/{ "Cell balance",    "Équilibre cell",   "Zellbalance",       "Balance celda" },
  /*S_NONE*/        { "none",            "aucun",            "keine",             "ninguno" },
  /*S_YES*/         { "yes",             "oui",              "ja",                "sí" },
  /*S_OKV*/         { "ok",              "ok",               "ok",                "ok" },
  /*S_FAULTV*/      { "fault",           "défaut",           "Fehler",            "fallo" },
  /*S_BADV*/        { "bad",             "mauvais",          "schlecht",          "malo" },
  /*S_WILL_HOLD*/   { "WILL HOLD",       "VA TENIR",         "HÄLT",             "AGUANTA" },
  /*S_WONT_HOLD*/   { "WON'T HOLD",      "NE TIENDRA PAS",   "HÄLT NICHT",       "NO AGUANTA" },
  /*S_NOT_LOCKED*/  { "NOT LOCKED",      "NON VERROUILLÉ",   "NICHT GESPERRT",    "NO BLOQUEADO" },
  /*S_REPAIRS_FALSE_ONLY*/{ "Repairs a false lockout only.","Répare un faux verrou seulement.","Nur Fehlsperre reparieren.","Solo repara bloqueo falso." },
  /*S_BEFORE*/      { "Before",          "Avant",            "Vorher",            "Antes" },
  /*S_AFTER*/       { "After",           "Après",            "Nachher",           "Después" },
  /*S_RETRY_REINSERT*/{ "To retry, remove & reinsert the pack.","Pour réessayer, retire et remets le pack.","Zum Wiederholen Akku neu einsetzen.","Reintentar: reinserta la batería." },
  /*S_LOCKEDV*/     { "LOCKED",          "VERROUILLÉ",       "GESPERRT",          "BLOQUEADO" },
  /*S_OKSTATE*/     { "OK",              "OK",               "OK",                "OK" },
  /*S_SCAN*/        { "scan",            "scan",             "scan",              "escanea" },
  /*S_LEDS_ON_MSG*/ { "LEDs ON",         "LEDs ON",          "LEDs AN",           "LEDs ON" },
  /*S_LEDS_OFF_MSG*/{ "LEDs OFF",        "LEDs OFF",         "LEDs AUS",          "LEDs OFF" },
  /*S_CONNECT_MSG*/ { "Connecting...",   "Connexion...",     "Verbinde...",       "Conectando..." },
  /*S_NO_DIAGNOSIS*/{ "NO DIAGNOSIS",    "PAS DE DIAG",       "KEINE DIAGNOSE",    "SIN DIAGNÓSTICO" },
  /*S_HW_SENSE*/    { "Broken sense wire","Fil de mesure cassé","Sense-Draht defekt","Cable de sensado roto" },
  /*S_HW_WEAK*/     { "Weak / dead group","Groupe faible / mort","Gruppe schwach / tot","Grupo débil / muerto" },
  /*S_HW_IMB*/      { "Cell imbalance",   "Déséquilibre cellules","Zellen unsymmetrisch","Desequilibrio celdas" },
  /*S_HW_THERM*/    { "Temp reading off", "Lecture temp anormale","Temp-Wert auffällig","Lectura temp anómala" },
  /*S_ACT_SENSE*/   { "Resolder sense wire on group %d","Ressoude le fil du groupe %d","Sense-Draht Gruppe %d löten","Resuelda sensor grupo %d" },
  /*S_ACT_WEAK*/    { "Measure group %d with a DMM","Mesure le groupe %d au multimètre","Gruppe %d mit DMM messen","Mide grupo %d con multímetro" },
  /*S_ACT_IMB*/     { "Group %d low: slow-charge, recheck","Groupe %d bas: charge lente, recontrôle","Gruppe %d niedrig: langsam laden","Grupo %d bajo: carga lenta, revisa" },
  /*S_ACT_THERM*/   { "Re-read cold, then check NTC","Relire à froid, puis contrôler NTC","Kalt messen, dann NTC prüfen","Medir en frío, luego revisar NTC" },
  /*S_FIX_HW_FIRST*/{ "FIX HARDWARE FIRST","RÉPARER LE MATÉRIEL","ERST HARDWARE FIXEN","REPARA HARDWARE" },
  /*S_UNLIKELY_HOLD*/{ "UNLIKELY TO HOLD","PEU PROBABLE",      "HÄLT WOHL NICHT",  "POCO PROBABLE" },
  /*S_SHOULD_HOLD*/ { "SHOULD HOLD",      "DEVRAIT TENIR",    "SOLLTE HALTEN",     "DEBERÍA AGUANTAR" },
  /*S_NO_LOCK_NO_FAULT*/{ "No lock, no fault","Pas de verrou, pas de faute","Keine Sperre, kein Fehler","Sin bloqueo ni fallo" },
  /*S_BMS_MEMORISED*/{ "BMS memorised a fault","Le BMS a mémorisé une faute","BMS hat Fehler gespeichert","El BMS memorizó un fallo" },
  /*S_FALSE_LOCKOUT*/{ "False lockout",   "Faux verrouillage","Falsche Sperre",    "Bloqueo falso" },
  /*S_PACK_HEALTHY*/{ "Pack looks healthy.","Pack a l'air sain.","Akku wirkt gesund.","Batería parece sana." },
  /*S_WHY_OD*/      { "Likely left flat (over-discharge).","Laissé à plat (sur-décharge).","Wohl tiefentladen.","Descargada del todo." },
  /*S_WHY_OL*/      { "Likely run in overload.","Utilisé en surcharge.","Wohl überlastet.","Usada en sobrecarga." },
  /*S_WHY_WEAR*/    { "Consistent with normal wear.","Cohérent avec usure normale.","Normaler Verschleiß.","Coherente con desgaste." },
  /*S_WHY_UNCLEAR*/ { "Cause unclear - see Health counters.","Cause floue - voir compteurs Santé.","Ursache unklar - Zustand prüfen.","Causa incierta - ver Salud." },
  /*S_WHY_THERM_CHECK*/{ "Cell/board temps differ - recheck","Temp cell/carte diverge - relire","Zell/Board-Temp weicht ab - neu","Temp celda/placa difiere - revisar" },
  /*S_UNLOCK_SHOULD_HOLD*/{ "Our unlock should hold.","Notre unlock devrait tenir.","Unser Entsperren sollte halten.","El desbloqueo debería aguantar." },
  /*S_TOAST_FIXHW*/ { "Fix hardware first","Répare le matériel","Erst Hardware fixen","Repara hardware" },
  /*S_TOAST_NOTHING*/{ "Nothing to repair","Rien à réparer",   "Nichts zu reparieren","Nada que reparar" },
  /*S_TOAST_USE_RESET*/{ "Use Tools > Reset error","Outils > Reset erreur","Werkzeuge > Fehler-Reset","Útiles > Reset error" },
  /*S_TOAST_PACK_CHANGED*/{ "Pack changed","Pack changé","Akku gewechselt","Batería cambiada" },
  /*S_WORKING*/     { "Working...",       "Travail...",       "Arbeite...",        "Trabajando..." },
  /*S_FRAME_STORE_PC*/{ "repair frame  ->  store  ->  power-cycle","répare -> stocke -> power-cycle","Frame -> speichern -> Neustart","repara -> guarda -> reinicio" },
  /*S_TEMP_CB*/     { "Cell/Board",      "Cell/Carte",       "Zelle/Platine",     "Celda/Placa" },
  /*S_CALIBRATE*/   { "Calibrate touch", "Calibrer tactile", "Touch kalibrieren", "Calibrar táctil" },
  /*S_CAL_TAP*/     { "Tap the target",  "Touchez la cible", "Ziel antippen",     "Toca el objetivo" },
  /*S_CAL_SAVED*/   { "Saved",           "Enregistré",       "Gespeichert",       "Guardado" },
  /*S_H_TURN_PAGE*/ { "Turn: page", "Tourner: page", "Drehen: Seite", "Girar: página" },
  /*S_H_TURN_SELECT*/ { "Turn: select", "Tourner: choisir", "Drehen: wählen", "Girar: elegir" },
  /*S_H_TURN_TOGGLE*/ { "Turn: toggle", "Tourner: basculer", "Drehen: umschalten", "Girar: alternar" },
  /*S_H_TURN_CANCEL*/ { "Turn: cancel", "Tourner: annuler", "Drehen: abbrechen", "Girar: cancelar" },
  /*S_H_CLICK_OPEN*/ { "Click: open", "Clic: ouvrir", "Klick: öffnen", "Clic: abrir" },
  /*S_H_CLICK_BACK*/ { "Click: back", "Clic: retour", "Klick: zurück", "Clic: atrás" },
  /*S_H_CLICK_CONFIRM*/ { "Click: confirm", "Clic: confirmer", "Klick: bestätigen", "Clic: confirmar" },
  /*S_H_CLICK_CONTINUE*/ { "Click: continue", "Clic: continuer", "Klick: weiter", "Clic: continuar" },
  /*S_H_CLICK_TOGGLE*/ { "Click: toggle", "Clic: basculer", "Klick: ändern", "Clic: cambiar" },
  /*S_H_CLICK_REFRESH*/ { "Click: refresh", "Clic: relire", "Klick: neu lesen", "Clic: releer" },
  /*S_H_BACK_MENU*/ { "Back: menu", "Retour: menu", "Zurück: Menü", "Atrás: menú" },
  /*S_H_BACK_TOOLS*/ { "Back: tools", "Retour: outils", "Zurück: Werkz.", "Atrás: herram." },
  /*S_H_PLEASE_WAIT*/ { "Please wait", "Patientez", "Bitte warten", "Espere" },
  /*S_HDR_RESET_Q*/ { "Reset error?", "Reset erreur ?", "Fehler-Reset?", "Reset error?" },
  /*S_HDR_RESET_RESULT*/ { "Reset result", "Résultat reset", "Reset-Ergebnis", "Resultado reset" },
  /*S_HDR_COMM_ERROR*/ { "Comm error", "Erreur comm.", "Kommfehler", "Error comm." },
  /*S_HDR_UNLOCK_Q*/ { "Unlock pack?", "Débloquer ?", "Entsperren?", "¿Desbloquear?" },
  /*S_HDR_UNLOCKING*/ { "Unlocking", "Déblocage", "Entsperrung", "Desbloqueando" },
  /*S_HDR_UNLOCK_RESULT*/ { "Unlock result", "Résultat déblocage", "Entsperr-Ergebnis", "Resultado desbl." },
  /*S_W_UNLOCKED*/ { "UNLOCKED", "DÉBLOQUÉ", "ENTSPERRT", "DESBLOQUEADO" },
  /*S_W_STILL_LOCKED*/ { "STILL LOCKED", "TOUJOURS VERROU", "NOCH GESPERRT", "SIGUE BLOQUEADO" },
  /*S_W_NOT_REREAD*/ { "NOT RE-READ", "NON RELU", "NICHT GELESEN", "SIN RELECTURA" },
  /*S_W_CLEARED*/ { "CLEARED", "EFFACÉ", "GELÖSCHT", "BORRADO" },
  /*S_W_UNCHANGED*/ { "UNCHANGED", "INCHANGÉ", "UNVERÄNDERT", "SIN CAMBIO" },
  /*S_W_NO_ERROR*/ { "NO ERROR", "AUCUNE ERREUR", "KEIN FEHLER", "SIN ERROR" },
  /*S_W_NOTHING_TO_DO*/ { "NOTHING TO DO", "RIEN À FAIRE", "NICHTS ZU TUN", "NADA QUE HACER" },
  /*S_W_NOT_SUPPORTED*/ { "NOT SUPPORTED", "NON SUPPORTÉ", "NICHT UNTERST.", "NO SOPORTADO" },
  /*S_W_NO_RESPONSE*/ { "NO RESPONSE", "PAS DE RÉPONSE", "KEINE ANTWORT", "SIN RESPUESTA" },
  /*S_W_ACTIVE*/ { "ACTIVE", "ACTIF", "AKTIV", "ACTIVO" },
  /*S_W_OFF*/ { "OFF", "ARRÊT", "AUS", "APAGADO" },
  /*S_SUB_FRAME_OK*/ { "Frame valid - no write done", "Trame valide - aucune écriture", "Frame gültig - kein Schreiben", "Trama válida - sin escritura" },
  /*S_SUB_F0513*/ { "F0513 (old BMS)", "F0513 (ancien BMS)", "F0513 (alter BMS)", "F0513 (BMS antiguo)" },
  /*S_SUB_NO_ANSWER*/ { "The pack did not answer", "La batterie ne répond pas", "Akku antwortet nicht", "La batería no responde" },
  /*S_SUB_BRIDGE_ON*/ { "USB bridge on, PC mode", "Pont USB actif, mode PC", "USB-Brücke an, PC-Modus", "Puente USB activo, modo PC" },
  /*S_SUB_BRIDGE_OFF*/ { "USB bridge inactive", "Pont USB inactif", "USB-Brücke aus", "Puente USB inactivo" },
  /*S_SUB_REREAD_OK*/ { "Re-read after reset: OK", "Relu après reset: OK", "Nach Reset gelesen: OK", "Releído tras reset: OK" },
  /*S_SUB_RELOCKS*/ { "Re-locks: real fault", "Reverrouille: vrai défaut", "Sperrt erneut: echter Fehler", "Se rebloquea: fallo real" },
  /*S_SUB_ALREADY_OK*/ { "Pack was already OK", "Le pack était déjà OK", "Akku war schon OK", "La batería ya estaba OK" },
  /*S_SUB_INSERT_PACK*/ { "Insert a pack on the adapter", "Place un pack sur l'adaptateur", "Akku auf den Adapter setzen", "Coloca una batería en el adaptador" },
  /*S_SUB_CELL_DIFF*/ { "Diff", "Écart", "Diff.", "Dif." },
  /*S_SUB_NO_REPLY*/ { "No pack reply", "Pas de réponse du pack", "Keine Antwort vom Akku", "Sin respuesta" },
  /*S_LBL_TRY*/ { "Try this", "À essayer", "Versuchen", "Probar" },
  /*S_STEP_ADAPTER*/ { "Check the adapter", "Vérifie l'adaptateur", "Adapter prüfen", "Revisa el adaptador" },
  /*S_STEP_DATA_EN*/ { "Check DATA / ENABLE wiring", "Vérifie DATA / ENABLE", "DATA/ENABLE prüfen", "Revisa DATA / ENABLE" },
  /*S_STEP_DEAD_BMS*/ { "No reply at all: dead BMS", "Aucune réponse: BMS mort", "Gar keine Antwort: BMS tot", "Ninguna respuesta: BMS muerto" },
  /*S_LBL_WHAT*/ { "What happens", "Ce qui se passe", "Was passiert", "Qué pasa" },
  /*S_WHAT_FLASH*/ { "Writes the BMS flash", "Écrit la flash du BMS", "Schreibt den BMS-Flash", "Escribe la flash del BMS" },
  /*S_BTN_CANCEL*/ { "Cancel", "Annuler", "Abbrechen", "Cancelar" },
  /*S_BTN_UNLOCK*/ { "Unlock", "Débloquer", "Entsperren", "Desbloquear" },
  /*S_KEEP_PACK*/ { "Keep the pack connected", "Laisser la batterie branchée", "Akku angeschlossen lassen", "Mantener la batería conectada" },
  /*S_STEP_FMT*/ { "Step %d/%d", "Étape %d/%d", "Schritt %d/%d", "Paso %d/%d" },
  /*S_CAP_REREAD*/ { "re-read", "relu", "neu gelesen", "releído" },
  /*S_LBL_VERDICT_NOW*/ { "Verdict now", "Verdict actuel", "Befund jetzt", "Veredicto ahora" },
  /*S_WHAT_RESET*/ { "Clears the BMS error flag", "Efface le drapeau d'erreur du BMS", "Löscht das BMS-Fehlerflag", "Borra el indicador de error del BMS" },
  /*S_BTN_RESET*/ { "Reset", "Reset", "Reset", "Reset" },
  /*S_ERR_NOW_FMT*/ { "Error now: %s", "Erreur actuelle: %s", "Fehler jetzt: %s", "Error ahora: %s" },
  /*S_CAP_ERRFLAG*/ { "error flag", "drapeau d'erreur", "Fehler-Flag", "indicador de error" },
  /*S_STEP_DATA*/ { "Check DATA wiring", "Vérifie le câblage DATA", "DATA-Verdrahtung prüfen", "Revisa el cableado DATA" },
  /*S_STEP_PULLUPS*/ { "Check pull-ups and GND", "Vérifie pull-ups et GND", "Pull-ups und GND prüfen", "Revisa pull-ups y GND" },
  /*S_STEP_RESEAT*/ { "Re-seat the pack", "Remets le pack en place", "Akku neu einsetzen", "Recoloca la batería" },
  /*S_BADGE_WRITE*/ { "Write", "Écrit", "Schreibt", "Escribe" },
  /*S_WRITE_LEGEND*/ { "sends a command to the pack", "envoie une commande au pack", "sendet einen Befehl an den Akku", "envía un comando a la batería" },
  /*S_T_TAP_OPEN*/ { "Tap: open", "Toucher: ouvrir", "Tippen: öffnen", "Tocar: abrir" },
  /*S_T_TAP_BACK*/ { "Tap: back", "Toucher: retour", "Tippen: zurück", "Tocar: atrás" },
  /*S_T_TAP_CONTINUE*/ { "Tap: continue", "Toucher: continuer", "Tippen: weiter", "Tocar: seguir" },
  /*S_T_TAP_TOGGLE*/ { "Tap: toggle", "Toucher: basculer", "Tippen: ändern", "Tocar: cambiar" },
  /*S_T_CENTRE_REFRESH*/ { "Centre: refresh", "Centre: relire", "Mitte: neu lesen", "Centro: releer" },
  /*S_T_EDGES_PAGE*/ { "Edges: page", "Bords: page", "Ränder: Seite", "Bordes: página" },
  /*S_T_TAP_BUTTON*/ { "Tap a button", "Toucher un bouton", "Taste antippen", "Toca un botón" },
  /*S_T_TITLE_BACK*/ { "Title: back", "Titre: retour", "Titel: zurück", "Título: atrás" },
  /*S_T_BELOW_PAGE*/ { "Below: page", "Dessous: page", "Unten: Seite", "Abajo: página" },
  /*S_LBL_HOWTO*/ { "How to", "Mode d'emploi", "So geht's", "Cómo usar" },
  /*S_HOWTO_CAP*/ { "Arduino OBI interface + this COM port", "Interface Arduino OBI + ce port COM", "Schnittstelle Arduino OBI + COM-Port", "Interfaz Arduino OBI + este puerto COM" },
  /*S_W_F0513_UNVALIDATED*/ { "F0513 chip: unlock may not hold.", "Puce F0513: peut ne pas tenir.", "F0513-Chip: hält evtl. nicht.", "Chip F0513: puede no aguantar." },
  /*S_DEG_C*/       { "C",               "C",                "C",                 "C" },
};
