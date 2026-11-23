/*
 * SMARTLUX v2 - firmware ESP32 (Arduino core)
 * ===============================================================
 * Fitur:
 *   1. Kontrol lampu otomatis: gelap DAN gerak, tunda mati 30 detik,
 *      histeresis ambang ADC (GPIO34).
 *   2. OLED SSD1306 128x64 (I2C 0x3C) - 3 halaman bergilir tiap 3 detik.
 *   3. INA219 (I2C 0x40) high-side pada rel 5 V - V, I, P, energi Wh,
 *      estimasi penghematan dibanding selalu nyala.
 *   4. Tombol override GPIO32 (INPUT_PULLUP, aktif LOW):
 *      tekan singkat 50-800 ms -> AUTO -> MANUAL_ON -> MANUAL_OFF -> AUTO
 *      tekan lama > 1500 ms  -> kembali ke AUTO.
 *   5. WiFi STA (kredensial di secrets.h) + fallback AP "SMARTLUX-XXXX",
 *      mDNS smartlux.local, dashboard web di port 80.
 *
 * CATATAN KESELAMATAN (WAJIB):
 *   - GPIO34, GPIO32, GPIO21, GPIO22 hanya boleh 3,3 V. Jangan pernah
 *     memberi 5 V ke pin ini.
 *   - Semua ground harus common (ESP32, INA219, PIR, driver LED, suplai).
 *   - INA219 dipasang HIGH-SIDE pada rel 5 V saja: 5 V -> VIN+ -> VIN- ->
 *     beban (ESP32 5V, PIR, rel LED). Jangan memasang INA219 di sisi
 *     ground atau di rel > 26 V.
 *   - Saat pengukuran pakai SATU sumber 5 V teregulasi dan CABUT kabel
 *     USB pemrograman. Jangan pernah dua sumber menyuplai board sekaligus.
 *   - Gate MOSFET: resistor seri 220-470 ohm + pull-down 10 kohm.
 *     IRFZ44N tidak cocok untuk gate 3,3 V; gunakan logic-level
 *     (IRLZ44N / IRL540N). Tab IRLZ44N = drain.
 *   - Ukur dulu tegangan OUT PIR HC-SR501 sebelum disambung ke GPIO27.
 *   - PIR butuh warm-up ~60 detik (jumper mode H).
 *
 * CATATAN KALIBRASI:
 *   - ADC_DARK_ON / ADC_BRIGHT_OFF di bawah adalah CONTOH. Ukur ADC saat
 *     gelap target dan terang target, lalu geser ambang ke titik tengah
 *     dengan histeresis 10-20% rentang.
 *   - LUX_PER_FULL_SCALE hanya estimasi kasar, bukan kalibrasi luxmeter.
 *   - LAMP_DEFAULT_W hanya nilai awal; firmware belajar daya lampu saat
 *     lampu ON di MANUAL_ON dan PIR tenang (lihat learnLampPower()).
 */

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_INA219.h>
#include <Preferences.h>
#include <ArduinoOTA.h>
#include <esp_task_wdt.h>
#include "logic.h"

// --- Kredensial WiFi (tidak di-commit) --------------------------------------
#if __has_include("secrets.h")
  #include "secrets.h"
#endif

// Sandi AP tidak punya nilai bawaan di sini. Sumbernya secrets.h, atau
// diturunkan dari MAC perangkat oleh apPassword().

// --- Pin map (JANGAN diubah tanpa memperbarui proposal) ----------------------
static const int PIN_LDR    = 34;  // ADC1_CH6, input-only, maks 3,3 V
static const int PIN_PIR    = 27;  // OUT HC-SR501 (verifikasi <= 3,6 V)
static const int PIN_MOSFET = 26;  // gate MOSFET low-side
static const int PIN_SDA    = 21;  // I2C SDA (3,3 V)
static const int PIN_SCL    = 22;  // I2C SCL (3,3 V)
static const int PIN_BTN    = 32;  // tombol override, INPUT_PULLUP, aktif LOW

static const uint8_t OLED_ADDR   = 0x3C;
static const uint8_t INA219_ADDR = 0x40;

// --- Waktu (semua non-blocking, berbasis millis()) --------------------------
static const unsigned long PIR_WARMUP_MS    = 60000UL; // warm-up HC-SR501
static const unsigned long DELAY_OFF_MS     = 30000UL; // tunda mati setelah gerak
static const unsigned long SENSOR_SAMPLE_MS = 200UL;   // baca LDR/PIR
static const unsigned long POWER_SAMPLE_MS  = 1000UL;  // INA219 + energi + CSV
static const unsigned long OLED_PAGE_MS     = 3000UL;  // rotasi halaman
static const unsigned long OLED_REFRESH_MS  = 400UL;   // refresh gambar
static const unsigned long BTN_DEBOUNCE_MS  = 50UL;
static const unsigned long BTN_SHORT_MIN_MS = 50UL;
static const unsigned long BTN_SHORT_MAX_MS = 800UL;
static const unsigned long BTN_LONG_MS      = 1500UL;
static const unsigned long STA_TIMEOUT_MS   = 20000UL; // fallback ke AP
static const unsigned long RECONNECT_MS     = 10000UL;
static const unsigned long DIM_WINDOW_MS    = 15000UL; // redup di akhir delay-off
static const unsigned long ENERGY_SAVE_MS   = 60000UL; // simpan NVS tiap 60 s
// 30 s: lebih lama dari blok write TCP/flash terburuk supaya koneksi lambat
// tidak memicu reset. trigger_panic tetap true agar board benar-benar pulih.
static const uint32_t WDT_TIMEOUT_MS        = 30000;   // task watchdog
static const uint16_t I2C_TIMEOUT_MS        = 100;     // Wire.setTimeOut
static const unsigned long I2C_RECOVER_COOLDOWN_MS = 30000UL;
static const int HIST_LEN                   = 60;      // riwayat 60 x 1 s

// --- Ambang cahaya (CONTOH - HARUS DIKALIBRASI) -----------------------------
// LDR dibaca dari divider: makin terang -> ADC makin tinggi.
// Histeresis: gelap jika ADC < ADC_DARK_ON, terang jika ADC > ADC_BRIGHT_OFF,
// di antaranya pertahankan keadaan sebelumnya (anti kedip).
static const int ADC_DARK_ON    = 1500;  // contoh, kalibrasi ulang
static const int ADC_BRIGHT_OFF = 1800;  // contoh, kalibrasi ulang

// Estimasi lux kasar: 4095 ADC dianggap ~LUX_FULL_SCALE lux.
static const float LUX_FULL_SCALE = 1000.0f;

// Daya lampu awal (W) sebelum hasil belajar tersedia. CONTOH - ganti.
static const double LAMP_DEFAULT_W = 3.0;

// --- Mode override ----------------------------------------------------------
enum Mode { MODE_AUTO = 0, MODE_MANUAL_ON = 1, MODE_MANUAL_OFF = 2 };
static_assert((int)MODE_AUTO == 0 && (int)MODE_MANUAL_ON == 1 &&
                  (int)MODE_MANUAL_OFF == 2,
              "logic.h assumes AUTO=0, ON=1, OFF=2");
static_assert((int)MODE_MANUAL_ON == SLX_MODE_MANUAL_ON,
              "lampBrightness() must agree with the firmware mode enum");
enum NetState { NET_CONNECTING = 0, NET_STA = 1, NET_AP = 2 };

// --- Status global ----------------------------------------------------------
static Mode  g_mode = MODE_AUTO;
static bool  g_lamp = false;
static bool  g_dark = false;
static bool  g_pir  = false;
static bool  g_motion = false;
static bool  g_pirReady = false;
static int   g_adc = 0;
static int   g_lightPct = 0;
static float g_lux = 0.0f;
static unsigned long g_lastMotionMs = 0;

// --- INA219 / energi --------------------------------------------------------
static bool  g_inaOk = false;
static bool  g_oledOk = false;
static float g_busV = 0.0f;
static float g_mA = 0.0f;
static float g_mW = 0.0f;
static double g_energyWh = 0.0;      // energi total sistem (sejak reset)
static double g_lampEnergyWh = 0.0;  // energi saat lampu ON (estimasi lampu)
static double g_lampPowerW = LAMP_DEFAULT_W; // hasil belajar / default
static double g_lampSumW = 0.0;
static uint32_t g_lampSamples = 0;
static unsigned long g_lastPowerMs = 0;
static unsigned long g_energyStartMs = 0;

// --- PWM / filter / NVS / riwayat (R1, R3, R4, R5) ---------------------------
static int g_pwm = 0;                       // duty LEDC 0..255 di GPIO26
static int g_adcHist[5] = {0, 0, 0, 0, 0};  // jendela median LDR
static int g_adcIdx = 0;
static int g_adcCount = 0;
static Preferences g_prefs;                 // NVS: energi tahan mati listrik
static unsigned long g_lastSaveMs = 0;
static int g_i2cFails = 0;                  // pemulihan bus I2C (EcoSentinel)
static unsigned long g_lastI2cRecoverMs = 0; // cooldown antar percobaan pulihkan bus
static bool g_otaEnabled = false;           // hanya aktif dengan sandi eksplisit
struct HistSample { uint32_t t_s; uint16_t mw; uint8_t pwm; };
static HistSample g_hist[HIST_LEN];
static int g_histHead = 0;   // slot tulis berikutnya
static int g_histCount = 0;  // jumlah sampel valid (<= HIST_LEN)

// --- Tombol ----------------------------------------------------------------
static bool g_btnStable = true;        // true = lepas (HIGH)
static bool g_btnRawLast = true;
static unsigned long g_btnChangeMs = 0;
static unsigned long g_btnPressMs = 0;
static bool g_btnLongFired = false;

// --- Jaringan --------------------------------------------------------------
static WebServer server(80);
static NetState g_net = NET_CONNECTING;
static unsigned long g_netDeadlineMs = 0;
static unsigned long g_lastReconnectMs = 0;
static bool g_mdnsStarted = false;
static unsigned long g_mdnsRetryMs = 0;
static char g_ip[16] = "0.0.0.0";
static char g_apSsid[32] = "SMARTLUX";

// --- Akses dashboard: token sekali pakai -> cookie sesi -------------
static const char AUTH_COOKIE[] = "smartlux_auth";
static char g_token[SLX_TOKEN_MAX_LEN + 1] = {0};

// --- OLED ------------------------------------------------------------------
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
static Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
static Adafruit_INA219 ina219(INA219_ADDR);
static uint8_t g_oledPage = 0;
static unsigned long g_lastPageMs = 0;
static unsigned long g_lastOledMs = 0;

// --- Mesin status eksplisit ---------------------------------------------------
// Nama status dibuat eksplisit agar UX (OLED/dashboard), DX dan AX (kode,
// telemetri, dokumentasi) membaca sumber yang sama. Transisi dan waktu tidak
// diubah: ini hanya pembukuan status, bukan perubahan perilaku.
enum ButtonPress { BTN_IDLE = 0, BTN_PRESSED, BTN_SHORT_FIRED, BTN_LONG_FIRED };
enum PirState { PIR_WARMUP = 0, PIR_ARMED, PIR_MOTION, PIR_QUIET };
enum LightState { LIGHT_BRIGHT = 0, LIGHT_DARK };
enum OledPage { PAGE_LIGHT = 0, PAGE_POWER, PAGE_ENERGY };
enum EnergyPhase { ENERGY_IDLE = 0, ENERGY_LEARNING, ENERGY_READY };
enum DeviceState { DEV_MISSING = 0, DEV_OK };
enum LampState { LAMP_OFF = 0, LAMP_ON, LAMP_FORCED_OFF };
enum BootPhase { BOOT_INIT = 0, BOOT_I2C, BOOT_NET, BOOT_READY };

static ButtonPress g_btnPress = BTN_IDLE;
static PirState    g_pirState = PIR_WARMUP;
static LightState  g_lightState = LIGHT_BRIGHT;
static OledPage    g_oledPageState = PAGE_LIGHT;
static EnergyPhase g_energyPhase = ENERGY_IDLE;
static DeviceState g_oledDevice = DEV_MISSING;
static DeviceState g_inaDevice = DEV_MISSING;
static LampState   g_lampState = LAMP_OFF;
static BootPhase   g_bootPhase = BOOT_INIT;

static void setButtonPress(ButtonPress s) { g_btnPress = s; }
static void setPirState(PirState s) { g_pirState = s; }
static void setLightState(LightState s) { g_lightState = s; g_dark = (s == LIGHT_DARK); }
static void setOledPage(OledPage s) { g_oledPageState = s; g_oledPage = (uint8_t)s; }
static void setEnergyPhase(EnergyPhase s) { g_energyPhase = s; }
static void setOledDevice(DeviceState s) { g_oledDevice = s; g_oledOk = (s == DEV_OK); }
static void setInaDevice(DeviceState s) { g_inaDevice = s; g_inaOk = (s == DEV_OK); }
static void setNetState(NetState s) { g_net = s; }
static void setLampState(LampState s) {
  g_lampState = s;
  g_lamp = (s == LAMP_ON);
  // R4: PWM dua tingkat (255 penuh / 128 redup / 0 mati) via LEDC 1 kHz.
  g_pwm = lampBrightness(g_lamp, (int)g_mode, millis(), g_lastMotionMs,
                         DELAY_OFF_MS, DIM_WINDOW_MS);
  ledcWrite(PIN_MOSFET, (uint32_t)g_pwm);
}
static void setBootPhase(BootPhase s) { g_bootPhase = s; }

static const __FlashStringHelper* buttonPressKey() {
  switch (g_btnPress) {
    case BTN_PRESSED: return F("pressed");
    case BTN_SHORT_FIRED: return F("short");
    case BTN_LONG_FIRED: return F("long");
    default: return F("idle");
  }
}
static const __FlashStringHelper* pirStateKey() {
  switch (g_pirState) {
    case PIR_WARMUP: return F("warmup");
    case PIR_MOTION: return F("motion");
    case PIR_QUIET: return F("quiet");
    default: return F("armed");
  }
}
static const __FlashStringHelper* lightStateKey() {
  return (g_lightState == LIGHT_DARK) ? F("dark") : F("bright");
}
static const __FlashStringHelper* oledPageKey() {
  switch (g_oledPageState) {
    case PAGE_POWER: return F("power");
    case PAGE_ENERGY: return F("energy");
    default: return F("light");
  }
}
static const __FlashStringHelper* energyPhaseKey() {
  switch (g_energyPhase) {
    case ENERGY_LEARNING: return F("learning");
    case ENERGY_READY: return F("ready");
    default: return F("idle");
  }
}
static const __FlashStringHelper* deviceKey(DeviceState s) {
  return (s == DEV_OK) ? F("ok") : F("missing");
}
static const __FlashStringHelper* lampStateKey() {
  switch (g_lampState) {
    case LAMP_ON: return F("on");
    case LAMP_FORCED_OFF: return F("forced_off");
    default: return F("off");
  }
}
static const __FlashStringHelper* bootPhaseKey() {
  switch (g_bootPhase) {
    case BOOT_I2C: return F("i2c");
    case BOOT_NET: return F("net");
    case BOOT_READY: return F("ready");
    default: return F("init");
  }
}
static const __FlashStringHelper* netStateKey() {
  switch (g_net) {
    case NET_CONNECTING: return F("connecting");
    case NET_AP: return F("ap");
    default: return F("sta");
  }
}


// ===========================================================================
// Dashboard (satu halaman, inline CSS/JS, tanpa CDN)
// ===========================================================================
// PlatformIO menyalin prototipe dari awal baris; baris JS di bawah sengaja
// diindentasi satu spasi agar `function`/`async function` di kolom 0 tidak
// disangka prototipe C++. Jangan hapus indentasi ini.
static const char INDEX_HTML[] PROGMEM = R"rawliteral(<!doctype html>
<html lang="id">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>SMARTLUX</title>
<style>
:root{--bg:#0b0d10;--panel:#12161b;--line:#1f252d;--fg:#e9eef5;--dim:#8b95a3;--on:#f4c95d;--acc:#7aa2ff;--ok:#4ade80;--r:16px}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--fg);font:15px/1.45 ui-sans-serif,-apple-system,"Segoe UI",Roboto,sans-serif}
main{max-width:940px;margin:0 auto;padding:18px 20px 44px}
.n{font-variant-numeric:tabular-nums}
header{display:flex;justify-content:space-between;align-items:center;padding:6px 0 28px;font-size:13px}
.brand{letter-spacing:.14em;font-weight:600;margin:0;font-size:inherit}
.conn{display:flex;align-items:center;gap:8px;color:var(--dim)}
.conn .d{width:7px;height:7px;border-radius:50%;background:var(--ok)}
.conn.down{color:#f87171}.conn.down .d{background:#f87171}
.hero{display:grid;gap:28px}
@media(min-width:760px){.hero{grid-template-columns:1fr 1fr;gap:36px;align-items:start}}
.state{color:#707b8a;font-size:clamp(40px,11vw,58px);font-weight:600;letter-spacing:-.03em;line-height:1;transition:color .3s}
.state.on{color:var(--on)}
.why{color:var(--dim);font-size:14px;margin-top:10px;min-height:1.45em}
.meter{margin-top:26px}
.lux{display:flex;align-items:baseline;gap:8px}
.lux b{font-size:26px;font-weight:600;letter-spacing:-.02em}
.lux span{font-size:12px;color:var(--dim)}
.bar{position:relative;height:6px;border-radius:99px;background:var(--line);margin-top:10px;overflow:hidden}
.bar i{position:absolute;left:0;top:0;bottom:0;background:var(--acc);border-radius:99px;transition:width .4s}
.bar .thr{position:absolute;top:0;bottom:0;width:2px;background:var(--bg)}
.bar.sm{height:5px;margin-top:14px}
.bar.sm i{background:var(--ok)}
.panel{background:var(--panel);border:1px solid var(--line);border-radius:var(--r);padding:20px}
.k{font-size:12px;color:var(--dim)}
.big{font-size:clamp(40px,10vw,52px);font-weight:600;letter-spacing:-.03em;line-height:1;margin:10px 0 8px}
.big small{font-size:.4em;color:var(--dim);font-weight:500}
.base{font-size:12px;color:var(--dim)}
.duty{font-size:12px;color:var(--dim);margin-top:14px}
.grid{display:grid;grid-template-columns:repeat(3,1fr);gap:1px;background:var(--line);border:1px solid var(--line);border-radius:var(--r);overflow:hidden;margin-top:28px}
.cell{background:var(--panel);padding:14px 14px 16px}
.cell .v{font-size:18px;font-weight:600;margin-top:5px}
.cell .v small{font-size:11px;color:var(--dim);font-weight:400;margin-left:2px}
.mode{display:grid;grid-template-columns:1fr;gap:12px;margin-top:28px}
.seg{display:grid;grid-template-columns:repeat(3,1fr);gap:6px;background:var(--panel);border:1px solid var(--line);border-radius:14px;padding:5px}
.seg button{font:inherit;font-size:13px;border:0;border-radius:10px;background:none;color:var(--dim);padding:12px 0;min-height:44px;cursor:pointer}
.seg button[aria-pressed=true]{background:#1c232c;color:var(--fg)}
.seg button:focus-visible,.reset:focus-visible{outline:2px solid var(--acc);outline-offset:1px}
.reset{font:inherit;font-size:12px;color:var(--dim);background:none;border:0;padding:10px;cursor:pointer;justify-self:center;min-height:44px}
.reset:hover{color:var(--fg)}
@media(min-width:760px){.grid{grid-template-columns:repeat(6,1fr)}.seg{max-width:420px}.mode{grid-template-columns:1fr auto;align-items:center}.reset{justify-self:end}}
</style>
</head>
<body>
<main>
<header><h1 class="brand n">SMARTLUX</h1><div class="conn" role="status" id="conn"><span class="d"></span><span id="ip" class="n">192.168.1.42</span></div></header>
<section class="hero">
<div>
<div class="state on" id="state"><span id="word" role="status">Nyala</span></div>
<div class="why" id="why">Gelap dan ada gerakan</div>
<div class="meter">
<div class="lux"><b class="n" id="lux">182</b><span>lux &middot; <span class="n" id="pct">44%</span></span></div>
<div class="bar"><i id="lit" style="width:44%"></i><span class="thr" style="left:40%"></span></div>
</div>
</div>
<div class="panel">
<div class="k">Hemat terhadap lampu selalu nyala</div>
<div class="big n" id="sav">38,4<small>%</small></div>
<div class="base n" id="wh">0,43 Wh dari 0,70 Wh</div>
<div class="bar sm"><i id="mbar" style="width:38.4%"></i></div>
<div class="duty n" id="duty">Nyala 42 s dari 60 s</div>
</div>
</section>
<section class="grid">
<div class="cell"><div class="k">Tegangan</div><div class="v n" id="volt">4,98<small>V</small></div></div>
<div class="cell"><div class="k">Arus</div><div class="v n" id="cur">312<small>mA</small></div></div>
<div class="cell"><div class="k">Daya</div><div class="v n" id="pwr">1,55<small>W</small></div></div>
<div class="cell"><div class="k">Energi</div><div class="v n" id="tot">0,43<small>Wh</small></div></div>
<div class="cell"><div class="k">Gerakan</div><div class="v" id="pir">Ada</div></div>
<div class="cell"><div class="k">Aktif</div><div class="v n" id="up">12m</div></div>
</section>
<section class="mode">
<div class="seg" role="group" aria-label="Mode lampu">
<button id="b-auto">AUTO</button>
<button id="b-on">NYALA</button>
<button id="b-off">MATI</button>
</div>
<button class="reset" id="reset">Reset energi</button>
</section>
</main>
<script>
const $=i=>document.getElementById(i),DARK=40;
const demo=location.protocol==='file:';
let mode='auto',E=.43,L=.43,S=38.4,t0=Date.now();
const hist=Array(60).fill(0);
const f=(v,d)=>v.toLocaleString('id-ID',{minimumFractionDigits:d,maximumFractionDigits:d});
const up=s=>{const m=Math.floor(s/60);return m<60?m+'m':Math.floor(m/60)+'j '+(m%60)+'m'};
 function why(d){
  if(d.mode!=='auto')return 'Manual';
  if(d.lamp)return d.pir?'Gelap dan ada gerakan':'Gelap, tunda mati';
  return d.light_pct<DARK?'Gelap, menunggu gerakan':'Terang';
}
 function paint(d){
  const w=d.lamp?'Nyala':'Mati';if($('word').textContent!==w)$('word').textContent=w;
  $('state').classList.toggle('on',!!d.lamp);
  $('why').textContent=why(d);
  $('lux').textContent=Math.round(d.lux_est);
  $('pct').textContent=d.light_pct+'%';
  $('lit').style.width=Math.max(0,Math.min(100,d.light_pct))+'%';
  $('sav').innerHTML=f(d.savings_pct,1)+'<small>%</small>';
  $('mbar').style.width=Math.max(0,Math.min(100,d.savings_pct))+'%';
  $('wh').textContent=d.savings_pct<99.9
    ?f(d.lamp_energy_wh,2)+' Wh dari '+f(d.lamp_energy_wh/(1-d.savings_pct/100),2)+' Wh'
    :f(d.lamp_energy_wh,2)+' Wh';
  $('volt').innerHTML=f(d.voltage_v,2)+'<small>V</small>';
  $('cur').innerHTML=Math.round(d.current_ma)+'<small>mA</small>';
  $('pwr').innerHTML=f(d.power_mw/1000,2)+'<small>W</small>';
  $('tot').innerHTML=f(d.energy_wh,2)+'<small>Wh</small>';
  $('pir').textContent=d.pir?'Ada':'Tidak';
  $('up').textContent=up(d.uptime_s);
  if($('ip').textContent!==d.ip)$('ip').textContent=d.ip;
  $('conn').classList.remove('down');
  mode=d.mode;
  for(const m of ['auto','on','off'])$('b-'+m).setAttribute('aria-pressed',mode===m);
  hist.push(d.lamp?1:0);hist.shift();
  $('duty').textContent='Nyala '+hist.reduce((a,b)=>a+b,0)+' s dari 60 s';
}
 function mock(){
  E+=.0004;
  return {light_pct:44,light_adc:1820,lux_est:182,pir:true,lamp:mode!=='off'&&Math.floor(Date.now()/5000)%4!=3,mode,
    voltage_v:4.98,current_ma:312,power_mw:1554,energy_wh:E,lamp_energy_wh:L,
    savings_pct:S,uptime_s:(Date.now()-t0)/1000,ip:'192.168.1.42'};
}
 async function refresh(){
  if(demo){paint(mock());return;}
  try{
    const r=await fetch('/api/status',{cache:'no-store'});
    if(!r.ok)throw 0;
    paint(await r.json());
  }catch(e){
    $('conn').classList.add('down');
    $('ip').textContent='Terputus';
  }
}
 async function act(u){let r=await fetch(u);if(r.status===429){await new Promise(ok=>setTimeout(ok,350));r=await fetch(u);}return r;}
 async function setMode(m){
  if(demo){mode=m;paint(mock());return;}
  try{await act('/api/override?mode='+m);}catch(e){}
  refresh();
}
for(const m of['auto','on','off'])$('b-'+m).onclick=()=>setMode(m);
$('reset').onclick=()=>{
  if(!confirm('Reset energi terhitung?'))return;
  if(demo){E=0;L=0;S=0;paint(mock());return;}
  act('/api/reset').then(refresh,refresh);
};
refresh();setInterval(refresh,1000);
</script>
</body>
</html>
)rawliteral";

// Anggaran PROGMEM dari gan-harness/spec.md: gagal saat kompilasi bila terlampaui.
static_assert(sizeof(INDEX_HTML) <= 8192, "Dashboard INDEX_HTML melebihi 8 KB");

// Halaman grafik riwayat (R5). Baris `function` sengaja di-indent 1 spasi agar
// ino2cpp PlatformIO tidak mengiranya sebagai prototipe C++.
static const char CHART_HTML[] PROGMEM = R"rawliteral(<!doctype html>
<html lang="id"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>SMARTLUX - Riwayat Daya</title>
<style>
:root{color-scheme:dark}
body{margin:0 auto;background:#0b0d10;color:#e7e9ea;font:14px system-ui,sans-serif;padding:20px;max-width:760px}
h1{font-size:18px;font-weight:600;margin:0 0 4px}
p.sub{color:#8b949e;margin:0 0 16px}
canvas{width:100%;height:260px;background:#12151a;border:1px solid #22262e;border-radius:8px}
.legend{display:flex;gap:16px;color:#8b949e;margin:12px 0;font-size:12px}
.dot{display:inline-block;width:8px;height:8px;border-radius:50%;margin-right:6px}
a{color:#58a6ff;text-decoration:none;font-size:13px}
a:hover{text-decoration:underline}
</style></head><body>
<h1>Riwayat Daya</h1>
<p class="sub">60 sampel terakhir - diperbarui tiap 2 detik</p>
<canvas id="c" width="720" height="260"></canvas>
<div class="legend"><span><i class="dot" style="background:#3fb950"></i>Daya (W)</span><span><i class="dot" style="background:#d29922"></i>Duty lampu (%)</span></div>
<p><a href="/">&larr; Dashboard</a> &middot; <a href="/api/export">Unduh CSV</a></p>
<script>
 const c=document.getElementById('c'),x=c.getContext('2d');
 const P='#3fb950',D='#d29922',BG='#12151a',GR='#22262e';
 function line(a,key,max,col){
  if(!a.length)return;
  x.strokeStyle=col;x.lineWidth=2;x.beginPath();
  for(let i=0;i<a.length;i++){
   const px=i/Math.max(a.length-1,1)*c.width;
   const py=c.height-a[key]*c.height/max;
   i?x.lineTo(px,py):x.moveTo(px,py);
  }
  x.stroke();
 }
 function draw(d){
  x.fillStyle=BG;x.fillRect(0,0,c.width,c.height);
  x.strokeStyle=GR;x.lineWidth=1;
  for(let g=1;g<4;g++){const y=g*c.height/4;x.beginPath();x.moveTo(0,y);x.lineTo(c.width,y);x.stroke();}
  const a=d.samples||[],pm=Math.max(1,...a.map(s=>s.mw/1000))*1.15;
  line(a.map(s=>({v:s.mw/1000})),'v',pm,P);
  line(a,'pwm',255,D);
 }
 async function refresh(){
  try{const r=await fetch('/api/history',{cache:'no-store'});draw(await r.json());}catch(e){}
 }
 refresh();setInterval(refresh,2000);
</script>
</body></html>
)rawliteral";
static_assert(sizeof(CHART_HTML) <= 2560, "CHART_HTML melebihi 2,5 KB");

// ===========================================================================
// Util
// ===========================================================================
static const char *modeKey() {
  return modeKeyFor((int)g_mode);
}

static const char *modeLabel() {
  switch (g_mode) {
    case MODE_MANUAL_ON:  return "NYALA";
    case MODE_MANUAL_OFF: return "MATI";
    default:              return "AUTO";
  }
}

static void ipToStr(const IPAddress &ip, char *out, size_t n) {
  snprintf(out, n, "%u.%u.%u.%u",
           (unsigned)ip[0], (unsigned)ip[1], (unsigned)ip[2], (unsigned)ip[3]);
}

// Penghematan vs "selalu nyala" memakai daya lampu hasil belajar:
//   baseline = P_lamp * durasi;  hemat = (baseline - E_lamp) / baseline * 100
// Catatan: INA219 mengukur seluruh rel 5 V, jadi E_lamp masih memuat beban
// dasar (ESP32+PIR+PIR ~0,9-1,0 W). Dekomposisi rinci ada di proposal.
static double savingsPct() {
  return computeSavingsPct(g_lampPowerW, g_lampEnergyWh,
                           (unsigned long)(millis() - g_energyStartMs));
}

// ===========================================================================
// I2C + OLED + INA219
// ===========================================================================
static bool i2cPresent(uint8_t addr) {
  Wire.beginTransmission(addr);
  return Wire.endTransmission() == 0;
}

static void i2cScan() {
  Serial.println(F("# I2C scan:"));
  for (uint8_t a = 1; a < 127; a++) {
    if (i2cPresent(a)) {
      Serial.printf("#   ditemukan 0x%02X\n", a);
    }
  }
}

static void oledBegin() {
  if (!i2cPresent(OLED_ADDR)) {
    Serial.println(F("# OLED tidak ditemukan di 0x3C - lanjut tanpa OLED"));
    setOledDevice(DEV_MISSING);
    return;
  }
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("# OLED gagal inisialisasi - lanjut tanpa OLED"));
    setOledDevice(DEV_MISSING);
    return;
  }
  setOledDevice(DEV_OK);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0, 8);
  display.println(F("SMARTLUX"));
  display.setTextSize(1);
  display.setCursor(0, 34);
  display.println(F("v2 - memulai..."));
  display.display();
  Serial.println(F("# OLED siap (0x3C)"));
}

static void inaBegin() {
  if (!i2cPresent(INA219_ADDR)) {
    Serial.println(F("# INA219 tidak ditemukan di 0x40 - lanjut tanpa pengukuran"));
    setInaDevice(DEV_MISSING);
    return;
  }
  if (!ina219.begin(&Wire)) {
    Serial.println(F("# INA219 gagal inisialisasi - lanjut tanpa pengukuran"));
    setInaDevice(DEV_MISSING);
    return;
  }
  setInaDevice(DEV_OK);
  ina219.setCalibration_32V_2A();  // shunt 0,1 ohm, +/-3,2 A, bus <= 26 V
  Serial.println(F("# INA219 siap (0x40, 32V_2A)"));
}

// ===========================================================================
// Kontrol lampu
// ===========================================================================
static void applyControl() {
  const bool lamp = lampShouldBeOn((int)g_mode, g_pirReady, g_dark, g_motion);
  setLampState(lamp ? LAMP_ON : (g_pirReady ? LAMP_OFF : LAMP_FORCED_OFF));
}

static void setMode(Mode m) {
  if (g_mode == m) return;
  g_mode = m;
  if (m == MODE_AUTO) {
    // Butuh gerakan baru agar lampu menyala kembali setelah mode manual.
    g_lastMotionMs = millis() - DELAY_OFF_MS;
  }
  applyControl();
  Serial.printf("# mode -> %s\n", modeKey());
}

static void cycleMode() {
  setMode((Mode)cycleModeValue((int)g_mode));
}

// ===========================================================================
// Tombol override (debounce 50 ms, tanpa delay())
// ===========================================================================
static void pollButton() {
  const bool raw = digitalRead(PIN_BTN);  // LOW = ditekan
  const unsigned long now = millis();

  if (raw != g_btnRawLast) {
    g_btnRawLast = raw;
    g_btnChangeMs = now;
  }

  const bool stable = stableButtonState(raw, g_btnStable, now, g_btnChangeMs,
                                        BTN_DEBOUNCE_MS);
  if (stable != g_btnStable) {
    g_btnStable = stable;
    if (!g_btnStable) {                    // tepi tekan
      g_btnPressMs = now;
      g_btnLongFired = false;
      setButtonPress(BTN_PRESSED);
    } else {                               // tepi lepas
      const unsigned long held = now - g_btnPressMs;
      if (!g_btnLongFired && held >= BTN_SHORT_MIN_MS && held <= BTN_SHORT_MAX_MS) {
        setButtonPress(BTN_SHORT_FIRED);
        cycleMode();
      } else if (!g_btnLongFired) {
        setButtonPress(BTN_IDLE);
      }
    }
  }

  // Tekan lama: fire sekali saat masih ditahan.
  if (!g_btnStable && !g_btnLongFired &&
      (now - g_btnPressMs) >= BTN_LONG_MS) {
    g_btnLongFired = true;
    setButtonPress(BTN_LONG_FIRED);
    setMode(MODE_AUTO);
  }
}

// ===========================================================================
// Sensor cahaya + PIR
// ===========================================================================
static void sampleSensors() {
  // R1: median 5 sampel ADC — buang lonjakan sesaat (gaya ESPHome).
  g_adcHist[g_adcIdx] = analogRead(PIN_LDR);
  g_adcIdx = (g_adcIdx + 1) % 5;
  if (g_adcCount < 5) g_adcCount++;
  g_adc = adcMedian5(g_adcHist, g_adcCount);

  // Histeresis: cegah kedip di sekitar ambang.
  const int light = lightDecision(g_adc, ADC_DARK_ON, ADC_BRIGHT_OFF);
  if (light == 0)      setLightState(LIGHT_DARK);
  else if (light == 1) setLightState(LIGHT_BRIGHT);

  g_lightPct = lightPercent(g_adc);

  // Estimasi lux kasar (non-linear, LDR): terang penuh ~LUX_FULL_SCALE.
  g_lux = luxEstimate(g_adc, LUX_FULL_SCALE);

  const bool pirRaw = (digitalRead(PIN_PIR) == HIGH);
  g_pir = pirRaw;

  // Warm-up PIR: abaikan gerakan sampai 60 detik.
  g_pirReady = (millis() >= PIR_WARMUP_MS);
  if (!g_pirReady) {
    g_pir = false;
    g_lastMotionMs = millis() - DELAY_OFF_MS;
    setPirState(PIR_WARMUP);
  } else if (pirRaw) {
    g_lastMotionMs = millis();
    setPirState(PIR_MOTION);
  } else {
    setPirState(PIR_QUIET);
  }

  g_motion = motionActive(millis(), g_lastMotionMs, DELAY_OFF_MS);
  applyControl();
}

// Pelajari daya lampu: lampu ON, mode MANUAL_ON, PIR tenang.
static void learnLampPower() {
  if (!g_lamp || g_mode != MODE_MANUAL_ON || g_pir) return;
  if (g_mW < 1.0f) return;  // sampel tidak valid

  g_lampSumW += (double)g_mW / 1000.0;
  g_lampSamples++;
  if (g_lampSamples >= 30) {                 // ~30 detik data, perbarui estimasi
    g_lampPowerW = g_lampSumW / (double)g_lampSamples;
    g_lampSumW = 0.0;
    g_lampSamples = 0;
    setEnergyPhase(ENERGY_READY);
    Serial.printf("# P_lamp diperbarui: %.2f W\n", g_lampPowerW);
  } else if (g_energyPhase != ENERGY_READY) {
    setEnergyPhase(ENERGY_LEARNING);
  }
}

static void samplePower() {
  const unsigned long now = millis();
  const unsigned long dtMs = now - g_lastPowerMs;
  g_lastPowerMs = now;

  // R2: cek kesehatan bus I2C tiap sampel daya, terlepas dari status terakhir.
  // Probe tetap jalan walaupun INA219 tidak ada saat boot, sehingga pemulihan
  // tidak pernah "mengunci" diri sendiri. Cooldown 30 s mencegah log banjir.
  if (!i2cPresent(INA219_ADDR)) {
    if (g_i2cFails < 3) g_i2cFails++;
    const bool cooling = g_lastI2cRecoverMs != 0 &&
                         now - g_lastI2cRecoverMs < I2C_RECOVER_COOLDOWN_MS;
    if (g_i2cFails >= 3 && !cooling) {
      Serial.println("# I2C macet, memulihkan bus...");
      const bool oledWasOk = g_oledOk;
      Wire.end();
      Wire.begin(PIN_SDA, PIN_SCL, 100000UL);
      Wire.setTimeOut(I2C_TIMEOUT_MS);
      if (oledWasOk) oledBegin();   // jangan hapus OLED bila memang tidak ada
      inaBegin();
      g_lastI2cRecoverMs = now;
      // Penghitung hanya dikosongkan bila probe berikutnya berhasil; kalau
      // masih gagal, cooldown mengatur kapan percobaan berikutnya dilakukan.
      if (i2cPresent(INA219_ADDR)) g_i2cFails = 0;
    }
  } else {
    g_i2cFails = 0;
  }

  if (g_inaOk) {
    g_busV = ina219.getBusVoltage_V();
    g_mA  = ina219.getCurrent_mA();
    g_mW  = ina219.getPower_mW();
    if (g_mA < 0.0f) g_mA = 0.0f;   // abaikan offset negatif kecil
    if (g_mW < 0.0f) g_mW = 0.0f;
  } else {
    g_busV = 0.0f;
    g_mA = 0.0f;
    g_mW = 0.0f;
  }

  // E_Wh += V * I * dt  (dt dalam milidetik)
  if (g_inaOk && dtMs > 0) {
    const double wh = energyIncrementWh(g_busV, g_mA, dtMs);
    g_energyWh += wh;
    if (g_lamp) g_lampEnergyWh += wh;
  }

  learnLampPower();

  // R5: riwayat daya 60 x 1 s untuk grafik /chart dan ekspor CSV.
  uint32_t mw = (g_mW > 65000.0f) ? 65000 : (uint32_t)g_mW;
  g_hist[g_histHead] = { (uint32_t)(now / 1000UL), (uint16_t)mw, (uint8_t)g_pwm };
  g_histHead = (g_histHead + 1) % HIST_LEN;
  if (g_histCount < HIST_LEN) g_histCount++;

  // R3: simpan energi ke NVS tiap 60 s (hemat siklus tulis flash ~100 ribu).
  if (now - g_lastSaveMs >= ENERGY_SAVE_MS) {
    g_lastSaveMs = now;
    g_prefs.putDouble("e_wh", g_energyWh);
    g_prefs.putDouble("e_lamp", g_lampEnergyWh);
    g_prefs.putUInt("e_s", (uint32_t)((now - g_energyStartMs) / 1000UL));
    g_prefs.putDouble("p_lamp", g_lampPowerW);
  }
}

// ===========================================================================
// OLED
// ===========================================================================
static void drawOled() {
  if (!g_oledOk) return;
  char b[28];

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  if (g_oledPage == 0) {
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print(F("SMARTLUX 1/3 CAHAYA"));
    display.setTextSize(2);
    display.setCursor(0, 12);
    snprintf(b, sizeof(b), "%3d%%", g_lightPct);
    display.print(b);

    display.drawRect(0, 30, 128, 10, SSD1306_WHITE);
    int w = (int)((long)g_lightPct * 124L / 100L);
    if (w < 0) w = 0;
    if (w > 124) w = 124;
    display.fillRect(2, 32, w, 6, SSD1306_WHITE);

    display.setTextSize(1);
    display.setCursor(0, 44);
    snprintf(b, sizeof(b), "ADC %4d  ~%d lux", g_adc, (int)g_lux);
    display.print(b);

    display.setCursor(0, 54);
    if (!g_pirReady) {
      unsigned long left = (PIR_WARMUP_MS - millis()) / 1000UL + 1UL;
      snprintf(b, sizeof(b), "PIR WARM-UP %lus", left);
    } else {
      snprintf(b, sizeof(b), "PIR:%s LED:%s",
               g_pir ? "ADA" : "-", g_lamp ? "ON" : "OFF");
    }
    display.print(b);
  } else if (g_oledPage == 1) {
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print(F("SMARTLUX 2/3 DAYA"));

    display.setTextSize(2);
    display.setCursor(0, 14);
    if (g_inaOk) snprintf(b, sizeof(b), "%.2fV", g_busV);
    else         snprintf(b, sizeof(b), "--V");
    display.print(b);

    display.setCursor(0, 32);
    if (g_inaOk) snprintf(b, sizeof(b), "%4.0fmA", (double)g_mA);
    else         snprintf(b, sizeof(b), "--mA");
    display.print(b);

    display.setTextSize(1);
    display.setCursor(0, 52);
    if (g_inaOk) snprintf(b, sizeof(b), "P = %.0f mW", (double)g_mW);
    else         snprintf(b, sizeof(b), "INA219 tidak ada");
    display.print(b);
  } else {
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print(F("SMARTLUX 3/3 ENERGI"));
    display.setCursor(0, 12);
    snprintf(b, sizeof(b), "E total : %.3f Wh", g_energyWh);
    display.print(b);
    display.setCursor(0, 22);
    snprintf(b, sizeof(b), "E lampu : %.3f Wh", g_lampEnergyWh);
    display.print(b);
    display.setCursor(0, 32);
    snprintf(b, sizeof(b), "Hemat   : %.1f %%", savingsPct());
    display.print(b);
    display.setCursor(0, 42);
    snprintf(b, sizeof(b), "Mode    : %s", modeLabel());
    display.print(b);
    display.setCursor(0, 54);
    snprintf(b, sizeof(b), "IP %s", g_ip);
    display.print(b);
  }

  display.display();
}

static void pollOled() {
  if (!g_oledOk) return;
  const unsigned long now = millis();

  if (now - g_lastPageMs >= OLED_PAGE_MS) {
    g_lastPageMs = now;
    setOledPage((OledPage)((g_oledPage + 1) % 3));
  }
  if (now - g_lastOledMs >= OLED_REFRESH_MS) {
    g_lastOledMs = now;
    drawOled();
  }
}

// ===========================================================================
// Jaringan
// ===========================================================================
static void startMdns() {
  if (g_mdnsStarted) return;
  g_mdnsRetryMs = millis();
  if (MDNS.begin("smartlux")) {
    MDNS.addService("http", "tcp", 80);
    g_mdnsStarted = true;
    Serial.println(F("# mDNS: http://smartlux.local"));
  } else {
    // begin() bisa gagal setelah init sebagian; lepas agar retry bisa ulang.
    MDNS.end();
    Serial.println(F("# mDNS gagal dimulai"));
  }
}

// Respons mDNS terikat ke antarmuka yang aktif. Saat AP dan STA berganti,
// responder lama dibuang lalu didaftarkan ulang supaya .local menunjuk IP benar.
static void restartMdns() {
  if (g_mdnsStarted) {
    MDNS.end();
    g_mdnsStarted = false;
  }
  startMdns();
}

static bool g_haveCreds = false;
static char g_apPass[20];

// Kredensial secrets.h dipakai kalau ada. Tanpa itu sandi diturunkan dari MAC
// supaya tidak ada sandi tetap yang ikut terbit di repositori.
static const char *apPassword() {
#if defined(SMARTLUX_AP_PASS)
  strncpy(g_apPass, SMARTLUX_AP_PASS, sizeof(g_apPass) - 1);
  g_apPass[sizeof(g_apPass) - 1] = '\0';
#else
  snprintf(g_apPass, sizeof(g_apPass), "smartlux-%04x",
           (unsigned)(ESP.getEfuseMac() & 0xFFFFULL));
#endif
  return g_apPass;
}

static void startApFallback() {
  snprintf(g_apSsid, sizeof(g_apSsid), "SMARTLUX-%04X",
           (unsigned)(ESP.getEfuseMac() & 0xFFFFULL));
  const char *pass = apPassword();
  WiFi.mode(WIFI_AP_STA);
  if (WiFi.softAP(g_apSsid, pass)) {
    setNetState(NET_AP);
    ipToStr(WiFi.softAPIP(), g_ip, sizeof(g_ip));
    Serial.printf("# AP: %s -> http://%s\n", g_apSsid, g_ip);
#if !defined(SMARTLUX_AP_PASS)
    Serial.printf("# sandi AP: %s\n", pass);
#endif
    restartMdns();
  } else {
    Serial.println(F("# softAP gagal - periksa WiFi"));
  }
}

static void setupWifi() {
  WiFi.persistent(false);
  WiFi.setSleep(false);

#if defined(WIFI_SSID) && defined(WIFI_PASS)
  // Placeholder di secrets.example.h dianggap belum dikonfigurasi.
  const bool haveCreds = (strcmp(WIFI_SSID, "GANTI_SSID") != 0) &&
                         (strlen(WIFI_SSID) > 0);
  g_haveCreds = haveCreds;
  if (haveCreds) {
    WiFi.mode(WIFI_STA);
    WiFi.setHostname("smartlux");
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    setNetState(NET_CONNECTING);
    g_netDeadlineMs = millis() + STA_TIMEOUT_MS;
    Serial.printf("# WiFi STA: menghubungkan ke %s\n", WIFI_SSID);
    return;
  }
  Serial.println(F("# secrets.h placeholder - memakai AP"));
#else
  Serial.println(F("# secrets.h tidak ada - memakai AP"));
#endif

  startApFallback();
}

static void pollNetwork() {
  const unsigned long now = millis();

  if (g_net == NET_CONNECTING) {
    if (WiFi.status() == WL_CONNECTED) {
      setNetState(NET_STA);
      ipToStr(WiFi.localIP(), g_ip, sizeof(g_ip));
      Serial.printf("# WiFi terhubung: http://%s\n", g_ip);
      restartMdns();
    } else if ((long)(now - g_netDeadlineMs) >= 0) {
      Serial.println(F("# STA timeout 20 dtk - fallback ke AP"));
      startApFallback();
    }
    return;
  }

  if (g_net == NET_STA) {
    if (WiFi.status() == WL_CONNECTED) {
      ipToStr(WiFi.localIP(), g_ip, sizeof(g_ip));
    } else if (now - g_lastReconnectMs >= RECONNECT_MS) {
      g_lastReconnectMs = now;
      Serial.println(F("# WiFi terputus - mencoba reconnect"));
      WiFi.reconnect();
    }
  } else if (g_net == NET_AP) {
    if (WiFi.status() == WL_CONNECTED) {
      WiFi.softAPdisconnect(true);
      WiFi.mode(WIFI_STA);
      setNetState(NET_STA);
      ipToStr(WiFi.localIP(), g_ip, sizeof(g_ip));
      Serial.printf("# STA pulih: http://%s\n", g_ip);
      restartMdns();
      return;
    }
#if defined(WIFI_SSID)
    // AP tetap hidup sambil mencoba STA lagi tiap 60 dtk.
    if (g_haveCreds && now - g_lastReconnectMs >= 60000UL) {
      g_lastReconnectMs = now;
      WiFi.begin(WIFI_SSID, WIFI_PASS);
    }
#endif
    ipToStr(WiFi.softAPIP(), g_ip, sizeof(g_ip));
  }

  // MDNS.end() bisa dilewati transisi WiFi; coba lagi tiap 10 dtk sampai hidup.
  if (!g_mdnsStarted && now - g_mdnsRetryMs >= 10000UL) {
    g_mdnsRetryMs = now;
    startMdns();
  }
}

// ===========================================================================
// HTTP handler
// ===========================================================================
// Tolak aksi dari halaman lain (CSRF). Fetch dari dashboard selalu same-origin,
// ketikan URL di address bar dan curl tidak mengirim Origin/Referer.
static bool sameOrigin() {
  const String site = server.header("Sec-Fetch-Site");
  if (site.length() && !site.equalsIgnoreCase("same-origin") &&
      !site.equalsIgnoreCase("none")) {
    return false;
  }
  String src = server.header("Origin");
  if (!src.length()) src = server.header("Referer");
  if (!src.length()) return true;
  const int p = src.indexOf("://");
  if (p < 0) return false;
  String host = src.substring(p + 3);
  const int slash = host.indexOf('/');
  if (slash >= 0) host = host.substring(0, slash);
  return host.equalsIgnoreCase(server.hostHeader());
}

// Hanya aksi yang mengubah keadaan yang dibatasi. Dashboard membaca
// /api/status sekali per detik dan sengaja tidak terkena batas ini.
static unsigned long g_lastActionMs = 0;
static const unsigned long ACTION_GAP_MS = 300UL;

static bool actionAllowed() {
  const unsigned long now = millis();
  if (g_lastActionMs != 0 && now - g_lastActionMs < ACTION_GAP_MS) return false;
  g_lastActionMs = now;
  return true;
}

// Halaman ini punya satu blok style dan satu blok script inline. Muatan lain
// diblokir: tidak ada CDN, font, gambar, atau frame dari alamat lain.
static const char CSP[] PROGMEM =
  "default-src 'none'; script-src 'unsafe-inline'; style-src 'unsafe-inline'; "
  "connect-src 'self'; img-src 'self' data:; base-uri 'none'; "
  "form-action 'none'; frame-ancestors 'none'";

static bool isAuthenticated() {
  const String cookie = server.header("Cookie");
  if (!cookie.length()) return false;
  char value[SLX_TOKEN_MAX_LEN + 1];
  if (!findCookieValue(cookie.c_str(), AUTH_COOKIE, value, sizeof(value))) return false;
  if (!validCookieValue(value, SLX_TOKEN_MAX_LEN)) return false;
  return tokenEquals(value, g_token);
}

static void handleRoot() {
  if (server.hasArg("t")) {
    const String t = server.arg("t");
    if (!validCookieValue(t.c_str(), SLX_TOKEN_MAX_LEN) ||
        !tokenEquals(t.c_str(), g_token)) {
      server.send(401, "application/json", "{\"error\":\"token tidak valid\"}");
      return;
    }
    char cookie[SLX_TOKEN_MAX_LEN + 52];
    snprintf(cookie, sizeof(cookie),
             "smartlux_auth=%s; HttpOnly; SameSite=Strict; Path=/", g_token);
    server.sendHeader("Set-Cookie", cookie);
  }
  server.sendHeader("Cache-Control", "no-store");
  server.sendHeader("X-Content-Type-Options", "nosniff");
  server.sendHeader("X-Frame-Options", "DENY");
  server.sendHeader("Content-Security-Policy", FPSTR(CSP));
  server.send_P(200, "text/html", INDEX_HTML);
}

static void handleStatus() {
  char json[768];
  const unsigned long bootS = millis() / 1000UL;

  snprintf(json, sizeof(json),
    "{\"light_adc\":%d,\"light_pct\":%d,\"lux_est\":%d,\"pir\":%d,\"lamp\":%d,"
    "\"mode\":\"%s\",\"voltage_v\":%.2f,\"current_ma\":%.1f,\"power_mw\":%.1f,"
    "\"energy_wh\":%.3f,\"lamp_energy_wh\":%.3f,\"savings_pct\":%.1f,"
    "\"uptime_s\":%lu,\"ip\":\"%s\","
    "\"pir_state\":\"%s\",\"light_state\":\"%s\","
    "\"button_state\":\"%s\",\"oled_page\":\"%s\","
    "\"energy_phase\":\"%s\",\"oled_device\":\"%s\","
    "\"ina_device\":\"%s\",\"lamp_state\":\"%s\","
    "\"boot_phase\":\"%s\",\"lamp_pwm\":%d}",
    g_adc, g_lightPct, (int)(g_lux + 0.5f),
    g_pir ? 1 : 0, g_lamp ? 1 : 0, modeKey(),
    (double)g_busV, (double)g_mA, (double)g_mW,
    g_energyWh, g_lampEnergyWh, savingsPct(),
    bootS, g_ip,
    pirStateKey(), lightStateKey(), buttonPressKey(), oledPageKey(),
    energyPhaseKey(), deviceKey(g_oledDevice), deviceKey(g_inaDevice),
    lampStateKey(), bootPhaseKey(), g_pwm);

  server.sendHeader("Cache-Control", "no-store");
  server.sendHeader("X-Content-Type-Options", "nosniff");
  server.send(200, "application/json", json);
}

static void handleOverride() {
  if (!sameOrigin()) {
    server.send(403, "application/json", "{\"error\":\"lintas asal ditolak\"}");
    return;
  }
  if (!isAuthenticated()) {
    server.send(401, "application/json", "{\"error\":\"perlu token\"}");
    return;
  }
  if (!server.hasArg("mode")) {
    server.send(400, "application/json", "{\"error\":\"mode wajib\"}");
    return;
  }
  const String m = server.arg("mode");
  // Validasi penuh dulu: nilai salah tidak boleh menghabiskan slot gate.
  const int mi = parseMode(m.c_str());
  if (mi < 0) {
    server.send(400, "application/json", "{\"error\":\"mode tidak dikenal\"}");
    return;
  }
  if (!actionAllowed()) {
    server.sendHeader("Retry-After", "1");
    server.send(429, "application/json", "{\"error\":\"terlalu sering, tunggu sebentar\"}");
    return;
  }
  setMode((Mode)mi);
  char json[48];
  snprintf(json, sizeof(json), "{\"ok\":1,\"mode\":\"%s\"}", modeKey());
  server.send(200, "application/json", json);
}

static void handleReset() {
  if (!sameOrigin()) {
    server.send(403, "application/json", "{\"error\":\"lintas asal ditolak\"}");
    return;
  }
  if (!isAuthenticated()) {
    server.send(401, "application/json", "{\"error\":\"perlu token\"}");
    return;
  }
  if (!actionAllowed()) {
    server.sendHeader("Retry-After", "1");
    server.send(429, "application/json", "{\"error\":\"terlalu sering, tunggu sebentar\"}");
    return;
  }
  g_energyWh = 0.0;
  g_lampEnergyWh = 0.0;
  g_lampSumW = 0.0;
  g_lampSamples = 0;
  setEnergyPhase(ENERGY_IDLE);
  g_energyStartMs = millis();
  g_lastPowerMs = millis();
  g_lastSaveMs = millis();
  // R5: kosongkan riwayat grafik.
  g_histHead = 0;
  g_histCount = 0;
  // R3: hapus juga salinan NVS agar tidak muncul lagi setelah reboot.
  g_prefs.remove("e_wh");
  g_prefs.remove("e_lamp");
  g_prefs.remove("e_s");
  g_prefs.remove("p_lamp");
  server.send(200, "application/json", "{\"ok\":1}");
}

// R5: riwayat daya untuk grafik /chart (publik, read-only seperti /api/status).
static void handleHistory() {
  char buf[2400];
  int w = snprintf(buf, sizeof(buf), "{\"n\":%d,\"samples\":[", g_histCount);
  if (w < 0 || (size_t)w >= sizeof(buf)) {
    server.send(500, "application/json", "{\"error\":\"buffer\"}");
    return;
  }
  size_t n = (size_t)w;
  for (int i = 0; i < g_histCount; i++) {
    const int idx = (g_histHead - g_histCount + i + HIST_LEN) % HIST_LEN;
    const HistSample &s = g_hist[idx];
    w = snprintf(buf + n, sizeof(buf) - n, "%s{\"t\":%lu,\"mw\":%u,\"pwm\":%u}",
                 i ? "," : "", (unsigned long)s.t_s, (unsigned)s.mw, (unsigned)s.pwm);
    // snprintf mengembalikan panjang "seandainya" tidak terpotong: cek sebelum
    // menambah offset, dan sisakan 2 byte untuk penutup "]}".
    if (w < 0 || (size_t)w + 2 >= sizeof(buf) - n) {
      server.send(500, "application/json", "{\"error\":\"buffer\"}");
      return;
    }
    n += (size_t)w;
  }
  n += snprintf(buf + n, sizeof(buf) - n, "]}");
  server.sendHeader("Cache-Control", "no-store");
  server.sendHeader("X-Content-Type-Options", "nosniff");
  server.send(200, "application/json", buf);
}

// R5: ekspor CSV tanpa menambah PROGMEM pada dashboard.
static void handleExport() {
  String csv = "t_s,mw,pwm\r\n";
  csv.reserve(16 + (size_t)g_histCount * 24);  // kurangi churn heap ESP32
  for (int i = 0; i < g_histCount; i++) {
    const int idx = (g_histHead - g_histCount + i + HIST_LEN) % HIST_LEN;
    const HistSample &s = g_hist[idx];
    csv += String(s.t_s) + "," + String(s.mw) + "," + String(s.pwm) + "\r\n";
  }
  server.sendHeader("Content-Disposition", "attachment; filename=smartlux-history.csv");
  server.sendHeader("Cache-Control", "no-store");
  server.sendHeader("X-Content-Type-Options", "nosniff");
  server.send(200, "text/csv", csv);
}

static void handleChart() {
  server.sendHeader("Content-Security-Policy", FPSTR(CSP));
  server.sendHeader("X-Content-Type-Options", "nosniff");
  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "text/html", CHART_HTML);
}

static void handleNotFound() {
  server.send(404, "text/plain", "404 - tidak ditemukan");
}

// ===========================================================================
// setup / loop
// ===========================================================================
void setup() {
  setBootPhase(BOOT_INIT);
  Serial.begin(115200);
  Serial.println();
  Serial.println(F("# SMARTLUX v2 - ESP32"));

#if defined(SMARTLUX_TOKEN)
  strncpy(g_token, SMARTLUX_TOKEN, SLX_TOKEN_MAX_LEN);
  g_token[SLX_TOKEN_MAX_LEN] = '\0';
  Serial.println(F("# Token SMARTLUX: dari secrets.h"));
#else
  formatMacToken(ESP.getEfuseMac(), g_token, sizeof(g_token));
  Serial.printf("# Token SMARTLUX: %s\n", g_token);
  Serial.printf("# Enroll: http://smartlux.local/?t=%s\n", g_token);
  Serial.println(F("# PERINGATAN: token turunan MAC bisa dihitung orang di LAN. Set SMARTLUX_TOKEN di secrets.h."));
#endif
  if (!validCookieValue(g_token, SLX_TOKEN_MAX_LEN)) {
    Serial.println(F("# ERROR: token tidak valid. Pakai 1-64 karakter [A-Za-z0-9_.-]."));
  }

  pinMode(PIN_PIR, INPUT);
  // R4: LEDC 1 kHz, 8-bit menggantikan digitalWrite pada gate MOSFET.
  ledcAttach(PIN_MOSFET, 1000, 8);
  ledcWrite(PIN_MOSFET, 0);               // aman saat boot
  pinMode(PIN_BTN, INPUT_PULLUP);
  pinMode(PIN_LDR, INPUT);                // GPIO34 input-only

  // ADC1 12-bit, 11 dB -> rentang 0-3,3 V. Jangan >3,3 V di GPIO34.
  analogReadResolution(12);
  analogSetPinAttenuation(PIN_LDR, ADC_11db);

  // I2C 3,3 V, 100 kHz (400 kHz diizinkan).
  setBootPhase(BOOT_I2C);
  Wire.begin(PIN_SDA, PIN_SCL, 100000UL);
  Wire.setTimeOut(I2C_TIMEOUT_MS);
  i2cScan();
  oledBegin();
  inaBegin();

  // R3: pulihkan energi dari NVS (tahan mati listrik / reboot).
  g_prefs.begin("smartlux", false);
  g_energyWh = g_prefs.getDouble("e_wh", 0.0);
  g_lampEnergyWh = g_prefs.getDouble("e_lamp", 0.0);
  g_lampPowerW = g_prefs.getDouble("p_lamp", LAMP_DEFAULT_W);
  const uint32_t savedS = g_prefs.getUInt("e_s", 0);
  g_lastPowerMs = millis();
  // Arimetik uint32_t 32-bit sengaja memakai wrap millis(): baseline tetap
  // benar selama total uptime kumulatif < ~49,7 hari. Clamp nilai NVS rusak.
  const uint32_t now32 = (uint32_t)g_lastPowerMs;
  const uint32_t offsetMs = savedS > (0xFFFFFFFFUL / 1000UL)
                                ? 0xFFFFFFFFUL : savedS * 1000UL;
  g_energyStartMs = (unsigned long)(now32 - offsetMs);
  g_lastSaveMs = millis();
  if (savedS > 0 || g_energyWh > 0.0) {
    Serial.printf("# NVS dipulihkan: %.3f Wh (lampu %.3f Wh, %.0f s)\n",
                  g_energyWh, g_lampEnergyWh, (double)savedS);
  }
  g_lastPageMs = millis();

  // R2: task watchdog — reset loop() tiap iterasi.
  esp_task_wdt_config_t wdtCfg = {};   // zero-init: hindari field stack garbage
  wdtCfg.timeout_ms = WDT_TIMEOUT_MS;
  wdtCfg.idle_core_mask = 0;
  wdtCfg.trigger_panic = true;
  if (esp_task_wdt_reconfigure(&wdtCfg) != ESP_OK) esp_task_wdt_init(&wdtCfg);
  esp_task_wdt_add(NULL);

  setBootPhase(BOOT_NET);
  setupWifi();

  // OTA opt-in: HANYA dengan sandi eksplisit minimal 8 karakter di secrets.h.
  // Token turunan MAC tidak boleh dipakai: bisa dihitung orang di LAN, padahal
  // OTA memberi hak unggah firmware sembarangan (RCE). Tanpa sandi kuat, OTA mati.
#if defined(SMARTLUX_OTA_PASS)
  if (strlen(SMARTLUX_OTA_PASS) >= 8) {
    ArduinoOTA.setHostname("smartlux");
    ArduinoOTA.setPassword(SMARTLUX_OTA_PASS);
    ArduinoOTA.onStart([]() { Serial.println(F("# OTA mulai")); });
    ArduinoOTA.onEnd([]() { Serial.println(F("# OTA selesai")); });
    ArduinoOTA.onError([](ota_error_t e) { Serial.printf("# OTA error %u\n", (unsigned)e); });
    ArduinoOTA.begin();
    g_otaEnabled = true;
    Serial.println(F("# OTA aktif: smartlux.local:8266 (sandi dari secrets.h)"));
  } else {
    Serial.println(F("# OTA DINONAKTIFKAN: SMARTLUX_OTA_PASS harus >= 8 karakter."));
  }
#else
  Serial.println(F("# OTA DINONAKTIFKAN: set SMARTLUX_OTA_PASS di secrets.h."));
#endif

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/override", HTTP_GET, handleOverride);
  server.on("/api/reset", HTTP_GET, handleReset);
  server.on("/api/history", HTTP_GET, handleHistory);
  server.on("/api/export", HTTP_GET, handleExport);
  server.on("/chart", HTTP_GET, handleChart);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println(F("# Web server aktif di port 80"));
  Serial.println(F("# Grafik: http://smartlux.local/chart"));

  setBootPhase(BOOT_READY);
  Serial.println(F("t_ms,adc,pir,lamp,mode,v_mv,ma,mw,wh,pir_state,light_state,net,oled_page,energy_phase,pwm"));
}

void loop() {
  const unsigned long now = millis();
  esp_task_wdt_reset();       // R2: tandai masih hidup
  if (g_otaEnabled) ArduinoOTA.handle();  // OTA non-blocking, hanya bila aktif

  // Tombol: dicek tiap iterasi (murah, sudah ter-debounce).
  pollButton();

  // Sensor cahaya + PIR + kontrol lampu, tiap 200 ms.
  static unsigned long lastSensor = 0;
  if (now - lastSensor >= SENSOR_SAMPLE_MS) {
    lastSensor = now;
    sampleSensors();
  }

  // INA219 + akumulasi energi + log CSV, tiap 1 dtk.
  static unsigned long lastPower = 0;
  if (now - lastPower >= POWER_SAMPLE_MS) {
    lastPower = now;
    samplePower();

    const unsigned long vMv = (unsigned long)(g_busV * 1000.0f + 0.5f);
    // Format CSV untuk prosedur pengukuran proposal.
    Serial.printf("%lu,%d,%d,%d,%s,%lu,%.1f,%.1f,%.3f,%s,%s,%s,%s,%s,%d\n",
                  millis(), g_adc, g_pir ? 1 : 0, g_lamp ? 1 : 0,
                  modeKey(), vMv, (double)g_mA, (double)g_mW, g_energyWh,
                  pirStateKey(), lightStateKey(), netStateKey(),
                  oledPageKey(), energyPhaseKey(), g_pwm);
  }

  // OLED: rotasi 3 dtk, refresh 400 ms (non-blocking).
  pollOled();

  // Jaringan + HTTP.
  pollNetwork();
  server.handleClient();
}
