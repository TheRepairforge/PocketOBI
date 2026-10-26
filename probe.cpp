#include "probe.h"
#if PROBE_PULSE
#include "pocketobi.h"
#include "protocol.h"
#include "decode.h"

// Each group reproduces one step of a normal read on a pack that answers the static frame
// but not the model or live command ("LXT ?"), in the order a read sends them.
// Every group ends with ENABLE low, like the end of a read.

static void enableOn()  { digitalWrite(ENABLE_PIN, HIGH); delay(400); }
static void enableOff() { digitalWrite(ENABLE_PIN, LOW); }

static void stamp(const char *what) {
  Serial.printf("[%8lu ms] %s\n", millis(), what);
}

static void help() {
  Serial.println();
  Serial.println("PocketOBI PROBE_PULSE (test build, " FW_VERSION ")");
  Serial.println("Type one key, then watch the pack output on the scope:");
  Serial.println("  1  G1 model           CC DC 0C");
  Serial.println("  2  G2 static frame    33 AA 00");
  Serial.println("  3  G3 live read       CC D7 00 00 FF");
  Serial.println("  4  G4 clear x2        CC F0 00, CC F0 00");
  Serial.println("  5  G5 F0513 cells     CC 31..35, CC 52");
  Serial.println("  6  G6 test mode in    33 D9 96 A5 (alone)");
  Serial.println("  7  G7 D4/D6 reads     (alone, no test mode)");
  Serial.println("  8  G8 test mode out   CC D9 FF FF (alone)");
  Serial.println("  9  G9 extended block  6+7+8 in one ENABLE session, as a read does");
  Serial.println("  0  full read          exactly what Read sends");
  Serial.println("  a  R1 test mode out + ENABLE power-cycle");
  Serial.println("  b  R2 DATA held low 500 ms, then released");
  Serial.println("  c  R3 one static frame read (33 AA 00)");
  Serial.println("  ?  this list");
  Serial.println("Each step ends with ENABLE low.");
}

static void groupModel() {
  uint8_t p[16];
  sendCommand(MODEL_CMD, p);
}

static void groupStatic() {
  uint8_t p[32], rom[8];
  sendCommand(READ_MSG_CMD, p, rom);
}

static void groupLive() {
  uint8_t p[29];
  sendCommand(READ_DATA_CMD, p);
}

static void groupClear() {
  uint8_t t[8];
  sendCommand(CLEAR_CMD, t);
  sendCommand(CLEAR_CMD, t);
}

static void groupF0513Cells() {
  uint8_t c[2];
  sendCommand(F0513_VCELL1_CMD, c);
  sendCommand(F0513_VCELL2_CMD, c);
  sendCommand(F0513_VCELL3_CMD, c);
  sendCommand(F0513_VCELL4_CMD, c);
  sendCommand(F0513_VCELL5_CMD, c);
  sendCommand(F0513_TEMP_CMD, c);
}

// Same bytes and framing as the test-mode entry in readExtended().
static void testModeIn() {
  makita.reset(); delayMicroseconds(400);
  mkWrite(0x33); mkWrite(0xD9); mkWrite(0x96); mkWrite(0xA5); delay(20);
}

// Same reads as readExtended(), printed.
static void extReads() {
  uint8_t a = d6ReadByte(bmsAddr->faultMkA), b = d6ReadByte(bmsAddr->faultMkB);
  uint8_t y = d4ReadByte(bmsAddr->asmDate);
  uint8_t soc[2]; d4ReadBlock(bmsAddr->soc, soc, 2);
  uint8_t od = d4ReadByte(bmsAddr->odCount);
  uint8_t ol[7]; d4ReadBlock(bmsAddr->olBlock, ol, 7);
  Serial.printf("  d6: %02X %02X  d4: asm %02X soc %02X %02X od %02X ol0 %02X\n",
                a, b, y, soc[0], soc[1], od, ol[0]);
}

static void testModeOut() {
  makita.reset(); delayMicroseconds(400);
  mkWrite(0xCC); mkWrite(0xD9); mkWrite(0xFF); mkWrite(0xFF);
}

static void groupTestModeIn()  { enableOn(); testModeIn();  enableOff(); }
static void groupExtReads()    { enableOn(); extReads();    enableOff(); }
static void groupTestModeOut() { enableOn(); testModeOut(); enableOff(); }
static void groupExtBlock() {
  enableOn(); testModeIn(); extReads(); testModeOut(); enableOff();
}

static void fullRead() {
  if (readAllData()) readExtended();
  Serial.printf("  model \"%s\" valid=%d\n", bat.model, bat.valid ? 1 : 0);
}

static void remedyExitCycle() {
  uint8_t t[8];
  sendCommand(TESTMODE_EXIT_CMD, t);
  busPowerCycle();
}

static void remedyDataLow() {
  pinMode(ONEWIRE_PIN, OUTPUT);
  digitalWrite(ONEWIRE_PIN, LOW);
  delay(500);
  pinMode(ONEWIRE_PIN, INPUT);   // released: the pull-up takes DATA high again
}

void probeBegin() {
  delay(1500);   // let the USB serial come up
  help();
}

void probeService() {
  if (!Serial.available()) return;
  char k = Serial.read();
  const char *name = nullptr;
  void (*fn)() = nullptr;
  switch (k) {
    case '1': name = "G1 model";          fn = groupModel;       break;
    case '2': name = "G2 static frame";   fn = groupStatic;      break;
    case '3': name = "G3 live read";      fn = groupLive;        break;
    case '4': name = "G4 clear x2";       fn = groupClear;       break;
    case '5': name = "G5 F0513 cells";    fn = groupF0513Cells;  break;
    case '6': name = "G6 test mode in";   fn = groupTestModeIn;  break;
    case '7': name = "G7 D4/D6 reads";    fn = groupExtReads;    break;
    case '8': name = "G8 test mode out";  fn = groupTestModeOut; break;
    case '9': name = "G9 extended block"; fn = groupExtBlock;    break;
    case '0': name = "full read";         fn = fullRead;         break;
    case 'a': name = "R1 test mode out + ENABLE cycle"; fn = remedyExitCycle; break;
    case 'b': name = "R2 DATA low 500 ms";              fn = remedyDataLow;   break;
    case 'c': name = "R3 static frame read";            fn = groupStatic;     break;
    case '?': help(); return;
    default: return;   // ignore line endings and stray keys
  }
  Serial.printf("[%8lu ms] >>> %s\n", millis(), name);
  fn();
  stamp("<<< done, ENABLE low. Watch the scope now.");
}
#endif
