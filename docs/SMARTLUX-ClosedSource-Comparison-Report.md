# SMARTLUX — Closed-Source & Non-GitHub Comparison Report

**Research run:** `01a20dcf-f43d-73e2-b8c1-d9575ecf222a`  
**Hosted report:** https://agent.tinyfish.ai/research/01a20dcf-f43d-73e2-b8c1-d9575ecf222a  
**Mode:** deep | **Elapsed:** 710 s | **Sources:** 51

> Companion to `docs/SMARTLUX-OpenSource-Comparison-Report.md`. This report deliberately excludes
> open-source GitHub projects and covers commercial/closed-source products, academic literature,
> standards, patents, vendor datasheets and Indonesian regulatory context.

---

# SMARTLUX Commercial & Non-GitHub Comparison Report

## Comparative Analysis for the SMAN CMBBS Physics Practicum Proposal

**Prepared:** November 2026 | **Classification:** Internal Research Document

---

## EXECUTIVE SUMMARY

SMARTLUX is a deliberately constrained, low-voltage (5 V bench), zero-mains prototype built for a school physics practicum on a near-zero budget. This report benchmarks it against commercial smart lighting products, academic literature, Indonesian standards, and relevant component datasheets — explicitly excluding open-source GitHub repositories already covered in `docs/SMARTLUX-OpenSource-Comparison-Report.md`.

**Key findings:**

1. **SMARTLUX's core sensing logic (PIR + LDR AND-gate with delay-off) is functionally equivalent to what every consumer and commercial system uses**, including Philips Hue, WiZ, Signify Interact, and Lutron Vive. The difference is integration polish, ecosystem scale, and certification — not fundamental architecture.

2. **SMARTLUX's INA219-based real power measurement is demonstrably superior to Philips Hue's energy dashboard**, which estimates wattage from bulb type and brightness rather than measuring it [34]. It is competitive with mid-range smart plugs (Shelly Plus PM), and better than budget Tuya plugs on low-load accuracy [6][20][22].

3. **Academic evidence (LBNL meta-analysis, n=88 studies)** supports the SMARTLUX approach: occupancy control alone averages 24% savings; combined occupancy + daylight averages 38%; in classrooms specifically, occupancy savings average 55% per one widely-cited study [28][43]. SMARTLUX's methodology of measuring *actual* energy and comparing to an always-on baseline aligns with best-practice evidence hierarchy.

4. **Indonesian regulatory context is directly favorable**: SNI 6197:2020 mandates occupancy sensors, light sensors, and dimmers in energy-efficient lighting system design [42]; Permen ESDM No. 3/2025 requires government buildings (including state schools) to conduct energy audits and implement energy conservation measures [47]; the PLN P-1 tariff for public facilities is Rp 1,699–1,700/kWh [29][30], making energy savings financially tangible even at low-wattage bench scale.

5. **Patent exposure for a non-commercial school prototype is negligible** under Indonesian law, but copying commercial techniques wholesale without documentation would be poor academic practice. Specific "do not copy" risks are identified in Section 6.

6. **Ranked adoptable improvements** include hysteresis auto-calibration (zero cost), lux-unit calibration of the LDR (zero cost), and a simple data-logger display mode (zero cost firmware). Hardware additions with clear benefit include a BH1750 digital lux sensor (~USD 1) and a DS18B20 temperature sensor (~USD 1). Cloud integration, DALI, KNX, and mains-switching must be rejected for this context.

---

## TABLE OF CONTENTS

1. Commercial Consumer Smart Lighting — Feature-by-Feature Analysis
2. Commercial/Architectural Systems — DALI, KNX, Lutron, Signify Interact, Legrand
3. DIY-Adjacent Devices — Shelly, Sonoff, Aqara, Tuya; Energy-Monitoring Plugs vs. INA219
4. Academic Evidence on Occupancy and Daylight Savings
5. Indonesian Standards, Regulations, and Electricity Tariffs
6. Patent Landscape and Freedom-to-Operate
7. Component Datasheets — INA219, HC-SR501, PWM/MOSFET
8. Market Context
9. Synthesis and Ranked Recommendations
10. Proposal Phrasing Guidance

---

## 1. COMMERCIAL CONSUMER SMART LIGHTING

### 1.1 Product Family Overview

The table below summarizes sensing method, control logic, dimming, connectivity, energy measurement approach, offline behavior, price tier, and what genuinely exceeds SMARTLUX capability.

| Product Family | Sensing | Control Logic | Dimming | Connectivity | Energy Measurement | Offline Behavior | Price Tier (USD) | Genuinely Better Than SMARTLUX |
|---|---|---|---|---|---|---|---|---|
| **Philips Hue** | PIR (physical sensor, $49.99) + ambient light [38] | Motion + ambient light; schedules; geofencing [1] | 0.2–100% PWM [1] | Zigbee mesh + Bridge (required for full features) [1] | Estimated from bulb type × brightness; NOT measured [34] | Local via Bridge; voice/remote needs internet [39] | $$$  (Bridge $60 + bulbs $15–$70) | Polished app, geofencing, scenes, 16M colors, Matter ecosystem |
| **WiZ** | PIR (optional add-on) + SpaceSense RF (Wi-Fi, beta-quality) [2][37] | Motion trigger; circadian rhythm; schedules [2] | Variable PWM via app | Wi-Fi direct (no hub required) [2] | None documented | Works locally if router up; SpaceSense works offline [37] | $$ (~$15–25/bulb) | Easy setup, no hub needed, wide scene library |
| **TP-Link Tapo/Kasa** | No built-in PIR in bulbs; Tapo smart plugs for energy | Schedules, remote control | PWM via app | Wi-Fi | Tapo P110 plug measures power (see §3) | Local API available; cloud preferred | $–$$ ($10–30) | Consumer price point; P110 energy monitoring plug |
| **Govee** | Camera-based ambient sync; PIR via Matter sensors [1] | Scene triggers, music sync | PWM via app | Wi-Fi + BLE; Matter certified [1] | None in lighting products | Partial local via Matter | $$ ($20–60) | Aesthetic effects, Matter certification |
| **Nanoleaf** | No built-in PIR; Matter interoperability [1] | Schedules, screen sync [1] | PWM | Thread/Matter + Wi-Fi (not 5 GHz) [1] | None documented | Thread provides local fallback [39] | $$$ ($50–200+) | Thread/Matter ecosystem, modular aesthetics |
| **Yeelight / LIFX** | No built-in PIR | Schedules, remote | PWM | Wi-Fi | None documented | Cloud-dependent for most features | $–$$ | Color range, some Matter support |
| **IKEA TRADFRI/DIRIGERA** | No PIR in bulbs; optional motion sensor | Schedules, scenes; hub required for full features [1] | PWM via Zigbee | Zigbee + DIRIGERA hub [1][39] | None | Local via hub; voice/remote needs internet [39] | $ (~$10–30/bulb, hub ~$65) | Low price, Zigbee mesh, Matter via DIRIGERA |
| **GE Cync** | No PIR | Schedules | PWM | Wi-Fi direct (no hub) [1] | None | Cloud for most features | $ ($15–25) | No hub needed, music sync |

**Key observation:** None of the consumer bulb/fixture products measure actual power consumption. Philips Hue energy tracking — described as a prominent "energy dashboard" feature — is explicitly an estimate calculated from a table of typical wattages multiplied by brightness percentage, not a measurement [34]. Hue does not expose real power draw from the Bridge [34]. This is a fundamental inferiority relative to SMARTLUX's INA219 approach.

### 1.2 Philips Hue MotionAware (RF Sensing) — Noteworthy Development

Philips Hue introduced "MotionAware" using RF (Zigbee) sensing across its mesh — turning mains-powered bulbs into occupancy detectors without separate PIR sensors [35]. This requires Hue Bridge Pro ($99) and at least three mains-powered devices per room, works locally on the Bridge, and is limited to four sensing zones per bridge [35]. It is comparable to PIR in that it detects movement (not presence), can be triggered by pets, and cannot sense light levels without a paired physical sensor [35]. SMARTLUX cannot replicate this without mains-powered devices, but this is an architecturally interesting counterpart. MotionAware is built on Ivani's "Sensify" RF-sensing firmware [35] — this is not a technique SMARTLUX should attempt to replicate; it is documented here for academic context only.

### 1.3 WiZ Motion Sensor — Direct Comparable

WiZ's PIR motion sensor communicates directly with nearby WiZ lights without going through the Wi-Fi router, so it continues working when internet or Wi-Fi is down [37]. Its control range is stated as approximately 15 meters [37]. This is a pure PIR + direct RF broadcast architecture — functionally similar to SMARTLUX's GPIO27 → firmware → GPIO26 path, but wireless. Notably, WiZ's SpaceSense feature (Wi-Fi RF sensing without a physical sensor) was described in a 2024 Wirecutter review as feeling "beta-like" and not reliably detecting motion in close proximity [2].

### 1.4 Philips Hue Motion Sensor — Technical Comparison

The Philips Hue Motion Sensor uses PIR with integrated ambient light sensing, communicates over Zigbee, requires a Bridge to function (cannot operate standalone), has a detection range of 5 m at 100° angle, and is priced at ~$49.99 [38]. It is a motion sensor, not a presence sensor [38]. It is IP42, indoor only [38]. The HC-SR501 in SMARTLUX covers 3–7 m adjustable, is battery-free (powered from the ESP32's 5 V rail), and costs approximately $1–2. The difference is ecosystem integration and form factor, not fundamental sensing capability.

---

## 2. COMMERCIAL/ARCHITECTURAL SYSTEMS

### 2.1 DALI (IEC 62386)

DALI (Digital Addressable Lighting Interface) is the dominant open international standard for digital lighting control, specified in multi-part IEC 62386 [3][4]. Key technical facts:

- Single DALI bus: up to 64 individually addressable luminaires, up to 16 groups, up to 16 scenes [14][45]
- Bus voltage: 16 V DC, max 250 mA; SELV circuit; total bus length ~300 m at 1.5 mm² cable [45]
- Two-way communication: devices report failures, answer status queries [3]
- Part 252 (IEC 62386): Energy Data standard for LED drivers — allows real energy data to be queried from DALI-2 compliant drivers [4]
- Part 303: Occupancy sensors; Part 304: Light sensors — both DALI-2 aligned [4]
- Bluetooth and Zigbee gateway Parts (341, 342) published [4]
- DALI-2 certification ensures cross-manufacturer interoperability [3][4]

**Cost (indicative 2026, UK market):** ~£50–£150 per luminaire; a 50-luminaire system approximately £5,000–£12,000 installed [45].

**Comparison with SMARTLUX:** DALI's Part 252 (energy data) and Part 303/304 (occupancy/light sensors) describe exactly what SMARTLUX implements in firmware — the difference is standardized bus interoperability and scalability. DALI requires mains-voltage drivers and professional commissioning. Not adoptable for a 5 V bench.

**What SMARTLUX's proposal can cite:** IEC 62386 as the professional standard whose sensing philosophy (occupancy + daylight + addressable control) SMARTLUX demonstrates at bench scale. DALI's theoretical framework validates SMARTLUX's functional design.

### 2.2 KNX (ISO/IEC 14543)

KNX is the open standard for whole-building automation covering lighting, HVAC, shading, access, and energy metering [14][45]. It supports up to 57,375 devices across bus segments [45]. KNX commissioning requires the proprietary ETS software and specialist training [45]. KNX is typically more expensive than DALI for lighting-only projects and commonly delegates lighting dimming to a DALI subsystem via a KNX-DALI gateway [14].

**Cost:** ~£80–£200 per device plus £2,000–£5,000 engineering and commissioning [45]. Entirely inapplicable to a school bench.

### 2.3 Lutron (Commercial Grade)

Lutron operates across three tiers: consumer Caseta ($$ tier), professional RadioRA 3 ($$$ tier), and enterprise Athena/HomeWorks ($$$$ tier) [25][36][50].

| System | Protocol | Max Devices | Key Features | Price Range |
|---|---|---|---|---|
| Caseta | Clear Connect RF 434 MHz | 75/hub | Dimming, schedules, Alexa/Google/HomeKit | ~$2,600–$3,400 installed (whole home) [36] |
| RadioRA 3 | Clear Connect Type X (434 MHz + 2.4 GHz) | 200–400/repeater | Occupancy/daylight/temp sensors, Ketra tunable white, Control4/Crestron drivers | ~$5,000–$9,000 installed [36] |
| Vive | Clear Connect wireless | Unlimited fixtures, 100+ brand partners | Occupancy/daylight/scheduling, no control wiring, ASHRAE 90.1/Title 24 compliant, K-12 marketed | Quote on request [50] |
| Athena | Cloud + Clear Connect | Large-scale | Cloud dashboard, encrypted firmware, OpenADR demand response, shading [25] | Premium, quote only |

**Lutron offline reliability** is widely cited as a differentiating strength: Clear Connect operates at 434 MHz, has zero cloud dependency for day-to-day operation, and an attributed 10+ year track record without cloud-caused outages [39]. This is a genuine advantage over cloud-dependent systems.

**Vive for schools:** Lutron explicitly markets Vive for K-12 spaces, with daylight sensors dimming lights and occupancy sensors switching lights off in empty rooms [50]. It installs up to 70% faster than wired solutions and runs 10 years on a single battery (vendor claim) [50]. These are vendor marketing claims and should be presented as such in any school proposal.

**What is genuinely better than SMARTLUX:** Lutron systems offer certified interoperability, professional warranty, code compliance documentation (ASHRAE 90.1, Title 24, IECC), multi-zone coordination, motorized shading, and integration with professional AV control systems. These advantages are irrelevant to a single-lamp physics bench demonstration.

### 2.4 Signify Interact Pro (Philips Commercial)

Signify's Interact Pro is a luminaire-integrated control (LIC) system embedding sensors, control logic, and dimming within each luminaire, without external sensors or low-voltage control wiring [49].

Key specifications:
- Sensing: PIR occupancy + daylight (lux) sensing integrated per luminaire [49]
- Communication: Zigbee mesh between devices [49]
- Max nodes: 200; max zones: 64 [48][49]
- Commissioning: Bluetooth via Interact Pro app; no additional wiring or internet needed for base tier [48]
- Standby power (LIC only): ~0.46 W measured [49]
- Energy savings claim: "up to 75%" — based on a **single installation** (GSA Metcalfe Federal Building, Chicago, under GSA Green Proving Ground Program) [24][48]
- The 85% figure adds HVAC and plug-load integrations — not lighting-only [24]
- 5-year warranty, cULus listed, DLC listed [49]
- Code compliant: ASHRAE 90.1 (2019), Title 24 (2019), IECC (2018), DLC [24][48]

**Critical note on the 75%/85% energy savings claim:** These are vendor claims from a single building installation, explicitly not the result of a randomized controlled trial or multi-site study [24][48]. The proposal should not repeat these figures without attribution to the source and this caveat. See §10 (Proposal Phrasing) for guidance.

**Dynalite (high-end Signify):** Supports 0-10V, phase cut, PWM, DALI, DMX, and switching from one system; distributed intelligence with no single point of failure; configurable occupancy timeouts of 5/10/15/20 min with 1-minute grace period [24]. This architecture directly parallels what SMARTLUX implements manually in firmware — the commercial version adds protocol standardization and multi-protocol dimming output.

### 2.5 Legrand

Evidence was insufficient in the available sources to provide verified technical specifications for Legrand's smart lighting controls. Legrand is a major player in building electrical infrastructure and offers KNX and DALI-compatible products, but specific datasheets were not available in the evidence base. Treat any claims about Legrand with this caveat.

---

## 3. DIY-ADJACENT DEVICES AND ENERGY-MONITORING PLUGS

### 3.1 Platform Comparison: Shelly, Sonoff, Aqara, Tuya

| Dimension | Shelly | Sonoff | Aqara | Tuya |
|---|---|---|---|---|
| Cloud dependency | Local-first; cloud optional and disableable [7][9][39] | Native LAN mode on many models; eWeLink cloud by default [7] | Local hub; Zigbee automations run locally without internet [8] | Cloud-first; most devices require Tuya servers [7][9] |
| Hardware model | Single brand (Allterco); UL-listed, CE-certified [9] | Owns hardware; some ESP32 models flashable [7] | Own hardware + Zigbee hub up to 128 devices [8] | Platform/PaaS; devices from many OEMs [7][8] |
| Local protocol | Built-in web server, MQTT, CoAP out of the box [7][9][39] | LAN mode (many models); MQTT via flash [7] | Hub-local automations [8] | Local control as secondary feature [9] |
| Chip | ESP32 (Plus series) [39] | ESP8266/ESP32 (some models); Beken/RTL on newer [7][9] | Zigbee (NXP/Silicon Labs) | ESP8266/ESP32 or Beken/RTL8710 depending on model [7][9] |
| Offline resilience | Full local operation [9][39] | Partial (LAN mode) [7] | Full (hub-local) [8] | Minimal; cloud outage disables most features [9] |
| Price range | $10–$30 [9] | $8–$25 [7] | $20–$150 (hub required) | $5–$25 [9] |
| Energy monitoring | Most plugs/switches (accurate, local) [9] | Some models (Wi-Fi S31) [21] | Smart plug via Zigbee [21] | Some models; accuracy variable [9] |

**Notable cloud fragility:** A reported Tuya cloud outage in 2024 left millions of devices unresponsive, while Shelly devices continued working locally [9]. This is reported by a secondary source and should not be presented as a confirmed verified event without further corroboration, but it illustrates the architectural risk.

**SMARTLUX parallel:** SMARTLUX's local-first architecture (mDNS, local web server, token auth, NVS persistence, no mandatory cloud) aligns most closely with Shelly's design philosophy. This is a genuine architectural strength relative to Tuya or Govee.

### 3.2 Energy-Monitoring Smart Plugs vs. INA219 Accuracy

This is a critical comparison for the practicum's energy measurement claims.

| Device | Measurement Method | Accuracy (reported) | Low-Load Error | Protocol | Mains? | Price |
|---|---|---|---|---|---|---|
| **INA219 (SMARTLUX)** | High-side DC shunt (100 µΩ typical in breakout) | ±0.5% over temperature (INA219B grade) [22] | ≤1% FSR on 5 V DC rail | I2C, local only | No (5 V DC only) [22] | ~$1–2 (breakout) |
| Shelly Plus Plug UK | AC shunt + metering chip | +0.8% at 2 kW resistive, +2.1% at 60 W inductive, +5.0% at 5 W, +6.4% at 3 W [6] | ~5–6% at 3–5 W | Wi-Fi, local web API | Yes (230 V mains) [20] | ~£22 |
| Shelly Plug PM Gen3 | AC voltmeter + ammeter (internal) | Not independently tested; supports 16 A/3680 W [20] | Not published | Wi-Fi, MQTT, BLE | Yes (230 V mains) [20] | ~$20–25 |
| Tapo P110 | AC shunt-based | Not independently tested | Not published | Wi-Fi | Yes (mains) | ~$15–20 |
| Sonoff POW (Wi-Fi S31) | AC shunt | Not independently tested | Not published | eWeLink/LAN | Yes (mains) | ~$15 |
| Budget Tuya plug (£10) | Low-cost metering chip | +27.1% at 5 W, +34.5% at 3 W [6] | 27–35% at low loads | Wi-Fi | Yes (mains) | ~£10 |

**Critical interpretation:**

1. The INA219B datasheet specifies ±0.5% maximum accuracy over temperature [22]. At 5 V DC with a known shunt resistor and a small LED load, the absolute error is deterministic and can be characterized by the student — this is a genuine educational advantage.

2. Commercial AC smart plugs operate at mains voltage, measure AC, and must contend with reactive power and power factor. A plug reporting apparent power (VA) instead of real watts (W) overreads a 0.6 power-factor load by 40–55% [6]. The INA219 on a 5 V DC resistive-ish LED load has no power factor complication.

3. Budget plugs are systematically less accurate at low loads due to limited dynamic range of cheap metering chips [6]. The INA219's 15-bit ADC with configurable PGA (±40 mV, ±80 mV, ±160 mV, ±320 mV ranges) is better suited to low-power DC measurement than a metering chip tuned for 2 kW loads [22].

4. None of the consumer smart plugs are MID-approved (legal metrology) [6]; neither is the INA219. For a physics practicum, this should be acknowledged explicitly — neither SMARTLUX nor commercial plugs constitute traceable legal metering.

**Conclusion:** At the SMARTLUX operating point (5 V DC, low-wattage LED, known load type), the INA219 is likely more accurate than any consumer AC smart plug at that load range. This should be stated carefully in the proposal.

---

## 4. ACADEMIC EVIDENCE ON OCCUPANCY AND DAYLIGHT SAVINGS

### 4.1 LBNL Meta-Analysis (Authoritative Baseline)

The Lawrence Berkeley National Laboratory published a meta-analysis in LEUKOS (January 2012) synthesizing 240 savings estimates from 88 papers and case studies [28][40]. After filtering to field installations only (excluding simulations, which overestimate savings by at least 10%) [28][40]:

| Control Strategy | Average Energy Savings | Notes |
|---|---|---|
| Occupancy-based control | **24%** | Filtered to field installations [28][40] |
| Daylight harvesting (photosensors) | **28%** | Field; simulations overestimate [28][40] |
| Personal tuning (individual dimming) | **31%** | [28][40] |
| Institutional tuning | **36%** | [40] |
| Multiple combined strategies | **38%** | [28][40] |

**Education building subgroup** (n=9 occupancy, n=29 daylighting rows): average occupancy savings 31%, average daylighting savings 49% [28]. This is relevant for a school proposal.

**Range caveat:** Unfiltered averages were ~30% (occupancy) and ~41% (daylighting); vendor studies and simulations typically sit at the top of the range [28]. Industry estimates range from 5% to 90% depending on space type (VonNeida et al 2000) [28].

### 4.2 Classroom-Specific Studies

One study (Myshonkov and Atishev, cited in [10]) found motion-sensor on/off control in a public educational building achieved average savings of **at least 50%**, but longer switch-off delays reduced savings from 66.5% to 48.5%, and standby lighting further reduced them to 21.9% [10]. This is directly relevant to SMARTLUX's 30-second delay-off setting — shorter delays save more energy but risk annoyance.

A major classroom/office field study found occupancy + daylight + individual dimming combined produced average savings of 47% over one year with ~90 workers [44]. Standalone estimates were ~35% for occupancy sensors, ~20% for daylight harvesting, ~11% for individual dimming [44].

A single-building case study reported energy savings above 60% in a university laboratory using chair-mounted infrared sensors, photocells, and web-based monitoring [10].

Seo and Yun's digital-twin classroom framework reported PIR-based on/off control achieving 63–64% weekly savings, combined strategies 80–81% [10]. These are simulation-based estimates under representative occupancy, not monitored post-retrofit savings [10].

**Industry-cited classroom figures** (from one source, citing multiple studies): occupancy sensor savings of 55% in classrooms; daylight harvesting sidelighting savings of 50% in classrooms [43]. These are from individual case studies, not meta-analyses.

### 4.3 Measurement Methodology Hierarchy

The LBNL meta-analysis explicitly establishes this quality hierarchy [28]:

1. **Most reliable:** Monitors both baseline and test case simultaneously (control group)
2. **Moderate:** Monitors test case and back-calculates baseline (risks missing post-installation behavior changes)
3. **Least reliable:** Monitors baseline and simulates test case (consistently overestimates)

SMARTLUX's approach — integrating Wh from real measurements and comparing to a calculated always-on baseline — corresponds to level 2. The proposal should acknowledge this honestly.

**Commissioning matters:** A 2012 Energy Center of Wisconsin study found re-commissioning existing daylight harvesting controls increased median savings from 23% to 43% [43]. Poor sensor placement and lack of calibration are primary causes of underperformance [43][27].

### 4.4 Delay-Off and Lamp Life

The choice of delay-off time is a documented tradeoff: shorter delays increase energy savings but increase lamp cycling frequency, reducing lamp life for fluorescent sources [41]. For LED loads (as in SMARTLUX), this tradeoff is minimal since LEDs are not degraded by switching cycles [42] — SMARTLUX's 30-second delay-off is thus well-suited to its LED load.

---

## 5. INDONESIAN STANDARDS, REGULATIONS, AND ELECTRICITY TARIFFS

### 5.1 SNI 6197:2020 — Lighting Energy Conservation Standard

SNI 6197:2020 (*Konservasi Energi pada Sistem Pencahayaan*) is the current Indonesian national standard for energy conservation in lighting systems, a revision of SNI 6197:2011, established by BSN in 2020 [42]. It references ANSI/ASHRAE/IES Standard 90.1-2019 as a normative reference [42].

**Key requirements relevant to SMARTLUX:**

| Requirement | Value | Source |
|---|---|---|
| Minimum illuminance, classroom | **350 lux**, CRI ≥ 80 | [42] |
| Minimum illuminance, lab | **500 lux**, CRI ≥ 90 | [42] |
| Minimum illuminance, library reading room | **350 lux**, CRI ≥ 80 | [42] |
| Maximum lighting power density (LPD), school building (whole-building method) | **7.53 W/m²** | [42] |
| Maximum LPD, classroom (space-by-space method) | **11.95 W/m²** | [42] |
| Maximum LPD, parking garage | **1.40 W/m²** | [42] |

**Control system requirements (§4.4):** SNI 6197:2020 explicitly requires consideration of occupancy sensors (§4.4.3.1), light intensity sensors (§4.4.3.2), scheduling controls (§4.4.4), and dimmers (§4.4.5) in energy-efficient lighting system design [42]. This directly validates SMARTLUX's design philosophy.

The standard notes that LED lamps produce full light output immediately without warm-up time, unlike fluorescent lamps — supporting the use of MOSFET switching for LEDs [42].

**Comparison with IES:** The US IES recommendation for classrooms is 300–500 lux (30–50 footcandles) on the work surface [26], which is consistent with SNI 6197:2020's 350 lux minimum.

**Earlier version note:** One Indonesian academic study [5] cites SNI 6197-2011 with a 350–3000 lux classroom range. The 2020 revision's minimum of 350 lux is the applicable current standard; the upper bound is a visual comfort ceiling, not a requirement [42].

### 5.2 SNI 03-6196 — Building Energy Audit Standard

Indonesia's four building energy standards (SNI) cover envelope, HVAC, lighting, and building energy auditing [13]. The building energy audit standard (SNI 03-6196, or its current revision) governs IKE (*Intensitas Konsumsi Energi* — Energy Consumption Intensity) benchmarks for buildings. Government Regulation No. 70/2009 requires energy users consuming ≥6,000 tonnes of oil equivalent/year to appoint an energy manager, prepare a conservation program, conduct energy audits, and report results [13].

**Direct evidence on IKE values** was not available in the retrieved sources. The SNI 03-6196 standard itself was not retrieved; only the GBPN country infosheet was available [13]. Specific kWh/m²/year benchmarks for school buildings should be sourced directly from BSN.

**MEMR Regulation No. 3/2025** (*Permen ESDM No. 3/2025*, issued 23 January 2025): requires government and regional government entities to implement energy conservation through energy management, including: appointing an energy manager, preparing efficiency programs, conducting periodic energy audits (at minimum once every 3 years), and implementing audit recommendations within 3 years [47]. Performance evaluation is based on IKE as specified in Annex II of the regulation [47]. Format reporting includes monthly electricity consumption (kWh), contract power (kVA), tariff category, and building occupancy [47].

**This regulation applies to state schools (sekolah negeri)** as government facilities — SMAN CMBBS would fall under "sarana dan prasarana yang dikelola oleh Pemerintah Daerah" [47]. This is a direct regulatory hook for the SMARTLUX proposal.

### 5.3 PLN Electricity Tariff Context

Under MEMR Regulation No. 7/2024, non-subsidized tariffs are evaluated every three months based on exchange rate (USD/IDR), Indonesian Crude Price (ICP), inflation, and Coal Reference Price (HBA) [29][30][31]. All tariffs were frozen through Q4 2026 (October–December 2026) to protect purchasing power [29][31][32].

| PLN Category | Description | Tariff (Q4 2026) | Relevant to SMARTLUX? |
|---|---|---|---|
| **P-1/TR** | Public facilities, 6,600 VA–200 kVA, low-voltage | **Rp 1,699–1,700/kWh** [29][30] | **Yes — most likely school tariff** |
| P-2/TM | Public facilities, >200 kVA, medium-voltage | Rp 1,522–1,533/kWh [29][30][32] | Larger schools |
| S-1/TR | Social services, 3,500 VA–200 kVA | Rp 900/kWh [30] | Non-commercial religious education |
| S-2/TR | Social, 450 VA | Rp 325/kWh [46] | Small social facilities |
| R-1/TR | Household, 900 VA | Rp 1,352/kWh [29][32] | Residential reference |
| L/TR,TM,TT | General/street lighting | Rp 1,644/kWh [30] | — |

**Classification note:** SMAN CMBBS is a state senior high school (*Sekolah Menengah Atas Negeri*). Public school tariff classification depends on daya (connected power capacity). Schools with 6,600 VA–200 kVA connections are typically P-1/TR at **Rp 1,700/kWh** [29][30]. Religious education institutions (*lembaga pendidikan keagamaan*) qualify for S-category social tariffs [46], but regular state schools generally use the P-1 category.

**Energy savings calculation example for the proposal:**
- A 10 W LED lamp running always-on for 8 h/day × 250 school days = 20 kWh/year
- At Rp 1,700/kWh: Rp 34,000/year per lamp
- 30% occupancy savings (conservative LBNL estimate [40]): saves Rp 10,200/year per lamp
- Scale to 10 lamps in one classroom: Rp 102,000/year
- Scale to 10 classrooms: ~Rp 1,020,000/year (~USD 64 at current IDR)

This demonstrates that even modest lamps at bench scale, when scaled to actual school infrastructure, produce measurable savings. The SMARTLUX prototype demonstrates the measurement and control methodology — the scaling argument belongs in the proposal narrative.

### 5.4 Indonesian Energy Context

Indonesia's RPJMN 2025–2029 acknowledges that current pricing and subsidy policies do not fully support energy efficiency and low-carbon growth [15]. Coal is no longer Indonesia's cheapest electricity source, with generation costs estimated at IDR 930–1,060/kWh in 2025–2026 [19] — above many consumer retail tariffs, with the gap covered by subsidy. The 100 GW solar program launched August 2026 [19] creates a policy environment supportive of energy efficiency narratives.

SNI for LED lamps (enforced by BSN) covers material quality, lighting performance, electrical safety, and environmental resistance [18]. SKEM (Minimum Energy Performance Standard) ensures LED efficiency compliance [18] — both are relevant if SMARTLUX's proposal recommends replacing any school lamp type.

---

## 6. PATENT LANDSCAPE AND FREEDOM-TO-OPERATE

### 6.1 Relevant Active Patents

**US10728987B1** — "Distributed occupancy-tracking lighting control system" [11]: Describes PIR-based proximity tracking where each fixture independently adjusts illumination based on sensed proximity, without a central controller. It lists IR, ultrasonic, radar, TOF, RFID, UWB, Bluetooth, and Wi-Fi radios as sensor options, and notes PIR sensors may be firmware-updated to add ranging functionality [11]. Listed as Active (legal status per Google Patents — not a legal conclusion) [11].

**US10438471B2** — "Occupancy based lighting control technology" [12]: Describes Doppler microwave/ultrasonic sensing for occupancy-based lighting control, with an adjustable metallic confining unit to reduce wall-penetration false triggers [12]. Background section confirms PIR detection zone is defined and limited by lens angle, requiring multiple sensors for wide coverage [12]. Listed as Active [12].

### 6.2 Freedom-to-Operate Assessment for SMARTLUX

**US patents have no jurisdiction in Indonesia.** Indonesia is not bound by US patents; SMARTLUX is built and used in Indonesia, not in the United States. Indonesian patent law (Law No. 13/2016 on Patents) requires separate filing in Indonesia through DGIP (*Direktorat Jenderal Kekayaan Intelektual*). Neither US10728987B1 nor US10438471B2 appears to have been filed in Indonesia based on available information (Google Patents did not list Indonesian jurisdictions in the evidence).

**Non-commercial research exemption:** Even if a technique were patent-protected in Indonesia, Article 20 of Indonesian Patent Law provides exemptions for research and education purposes — a one-off school prototype for academic demonstration would not constitute patent infringement under Indonesian law. This should be noted in the proposal.

**"Do Not Copy" risks from a patent perspective:**
- **Distributed proximity-following dimming** (US10728987B1): Do not implement per-fixture "follow me" proximity tracking as a commercial product or replicate the specific distributed architecture across multiple fixtures. For a single-lamp bench, this is not applicable anyway.
- **Doppler microwave occupancy counting** (US10438471B2): Do not build a microwave Doppler sensor with a metallic confining unit for headcount tracking. SMARTLUX uses PIR only — no Doppler — so this patent is not implicated.
- **WiZ SpaceSense / Hue MotionAware RF sensing**: Do not attempt to replicate Zigbee/Wi-Fi RF-based occupancy sensing; this involves IP from Ivani [35]. Not relevant to the bench prototype.

**General FTO note:** Standard PIR + LDR + delay-off occupancy sensing is prior art dating to at least the 1980s. The basic combination is not patentable as a whole. Only specific novel implementations (distributed architecture, Doppler ranging, RF sensing) are under active patent protection.

### 6.3 SNI Patent Disclaimer

SNI 6197:2020 itself notes that some standard content may contain patent rights, and BSN does not accept responsibility for identifying patents [42]. This is standard ISO-style language and does not indicate any specific patent covers PIR + photocell sensing.

---

## 7. COMPONENT DATASHEETS

### 7.1 INA219 — Current/Power Monitor

**Source:** Texas Instruments INA219 datasheet Rev. G [22]

| Parameter | Value | Notes |
|---|---|---|
| Bus voltage range | 0–26 V | Covers 5 V bench [22] |
| Supply voltage | 3–5.5 V | Compatible with ESP32 3.3 V or 5 V rail [22] |
| Supply current (max) | 1 mA | Low self-heating [22] |
| Accuracy (INA219B grade) | **±0.5% max over temperature** | Best grade [22] |
| I2C addresses | 16 programmable | Multiple devices possible [22] |
| Power register | 20× Current_LSB × Bus Voltage | Calculated internally [22] |
| Power-down recovery | 40 µs | Fast wake [22] |
| Recommended input filter | 0.1–1 µF ceramic | For transient protection [22] |
| Input protection | 10 Ω series resistors | Protects against dV/dt up to 26 V rating [22] |

**Key pedagogical point:** The INA219's power measurement is calculated from the current shunt reading and the measured bus voltage. This means students can verify the measurement by independently measuring voltage with a multimeter and current with an ammeter — a direct cross-check that commercial smart plugs do not support.

**Accuracy context:** The ±0.5% spec (INA219B) refers to gain error over temperature. Additional sources of error include shunt resistor tolerance (typically ±1% for a standard 100 mΩ breakout shunt), ADC offset (~100 µV shunt offset as noted in the evidence brief), and digitization noise. For the proposal, claim ±1–2% system accuracy (conservative) rather than ±0.5%.

### 7.2 HC-SR501 PIR Sensor

**Source:** Third-party datasheet hub [23] — treat specifications as indicative; verify against manufacturer's original datasheet.

| Parameter | Value |
|---|---|
| Operating voltage | 5–20 V (Vcc typically 5 V) [23] |
| Output voltage (high) | 3.3 V TTL — directly compatible with ESP32 GPIO [23] |
| Output voltage (low) | 0 V [23] |
| Sensitivity range | ~3 m to ~7 m (adjustable via potentiometer) [23] |
| Delay time (hold-off) | ~5 s to ~200 s (adjustable via potentiometer) [23] |
| Blockade (retrigger hold-off) | 2.5 s default [23] |
| Operating temperature | −15 °C to +70 °C [23] |
| Power consumption | 65 mA [23] |
| Output | Digital HIGH when motion detected, LOW when idle [23] |

**SMARTLUX uses firmware-based delay-off** (30 s) rather than relying on the HC-SR501's onboard potentiometer delay — this is superior because: (a) it is software-adjustable without physical access, (b) firmware can implement precise hysteresis, and (c) it allows the two-level PWM dimming to run in the last 15 s before off.

**Comparison with Philips Hue Motion Sensor:** Hue sensor has 5 m / 100° range [38]; HC-SR501 has 3–7 m adjustable [23]. The Hue sensor integrates ambient light sensing in one package; SMARTLUX uses a separate LDR on GPIO34. The Hue sensor costs ~$50 and requires a Bridge; the HC-SR501 costs ~$1–2 and needs only a GPIO pin.

**Comparison with Signify Interact Pro:** Interact Pro's PIR sensor is also passive infrared with daylight sensing [49][51] — the sensing physics are identical.

### 7.3 PWM Dimming at 1 kHz and MOSFET Switching

**PWM frequency:** SMARTLUX uses 1 kHz PWM on GPIO26 driving a logic-level N-MOSFET. At 1 kHz, the duty cycle updates every 1 ms — fast enough to avoid visible flicker (human flicker threshold is approximately 50–80 Hz). Commercial systems use various frequencies: Philips Hue uses high-frequency PWM to avoid camera flicker artifacts; DALI logarithmic dimming curves are defined in IEC 62386 [3]. The Signify Interact system notes its dimming curve "appears linear to the human eye" but actual energy consumption is non-linear [51], and at <20% dim level, efficiency drops due to driver standby losses [51].

**MOSFET switching (logic-level N-channel on 5 V rail):** A logic-level N-MOSFET (e.g., IRLZ44N or 2N7000) with Vgs(th) < 2 V fully saturates at 3.3 V gate drive from the ESP32, making it suitable for this architecture. No specific datasheet was retrieved for a MOSFET in the evidence base, but the application is standard. At 1 kHz with a resistive LED load, switching losses are negligible — drain current transitions are fast relative to the 1 ms period.

**PWM + LED driver compatibility note:** Direct PWM of an LED (without an LED driver IC) may produce nonlinear luminous flux response at low duty cycles. At the SMARTLUX operating point (5 V bench, resistive-ish low-voltage LED), the linear approximation is acceptable for demonstrating dimming control. For a real installation, an LED driver with 0–10 V or PWM dimming input would be used (as in Dynalite [24] and Interact [49]).

---

## 8. MARKET CONTEXT

Smart lighting penetration remains minority in the existing commercial building stock: as of 2003 US CBECS (the most recent available in evidence), only 2% of lit commercial buildings had daylighting sensors and only 1% had energy management and control systems for lighting [28]. While this figure is from 2003, the LBNL meta-analysis (2012) noted controls still had minority penetration [40], and more recent sources indicate adoption has grown but advanced controls remain a minority technology.

Lighting accounts for approximately one-third of electrical energy consumption in commercial buildings and more than one-half in lodging and retail (US context) [28][40]. In Indonesian commercial buildings, the proportion is likely similar or higher given lower HVAC efficiency in older buildings.

The global smart lighting market has grown with the Matter standard enabling cross-brand interoperability. Govee is Matter-certified [1]; Nanoleaf supports Matter [1]; IKEA DIRIGERA supports Matter [39]; Shelly Gen4 supports Wi-Fi, Matter, and Zigbee [21]. This convergence is relevant context for the proposal narrative — SMARTLUX's web API and mDNS approach is architecturally aligned with the local-first, interoperable direction the industry is moving toward, independent of the Matter stack.

Consumer product price compression has continued: Tuya-based bulbs and plugs are available for $5–8 [9]; Shelly relays at $10–15 [9][39]. This confirms SMARTLUX's ESP32 + INA219 + HC-SR501 + LDR bill of materials (< $10 USD equivalent) is genuinely competitive in cost terms.

---

## 9. SYNTHESIS AND RANKED RECOMMENDATIONS

### 9.1 What Commercial Products Do Better Than SMARTLUX

| Advantage | Product(s) | Feasible to Close Gap? |
|---|---|---|
| Polished mobile app UX | Philips Hue, Shelly, WiZ | Firmware update possible; not needed for practicum |
| Multi-zone, multi-room control | Lutron Vive, Interact Pro, DALI | Requires multiple nodes and mains hardware — out of scope |
| Standards certification (DLC, UL, CE) | All commercial products | Not feasible for one-off school prototype |
| Ecosystem integration (Alexa, HomeKit, IFTTT) | Hue, WiZ, Shelly, Govee | Cloud dependency — rejected per design constraint |
| Zigbee/Thread/Matter mesh | Hue, IKEA, Nanoleaf, Shelly Gen4 | Different radio hardware required |
| Professional commissioning documentation | Lutron, DALI, Signify | Overkill for bench prototype |
| Offline RF reliability at 434 MHz | Lutron Clear Connect | Different hardware, overkill |
| Tunable white / RGBW | Hue, Govee, Nanoleaf | Irrelevant to energy measurement practicum |
| Industrial protocol interoperability | DALI, KNX | Mains, complex, expensive |

### 9.2 Where SMARTLUX Genuinely Exceeds or Matches Commercial Products

| SMARTLUX Strength | Commercial Comparison |
|---|---|
| **Real DC power measurement (INA219)** | Philips Hue energy dashboard is estimated, not measured [34]; most consumer bulbs have no power monitoring at all |
| **Actual Wh integration and savings estimation** | No consumer smart bulb does this; only commercial plugs (Shelly, Sonoff POW) measure energy, and they operate at mains |
| **Local-only operation (no cloud, no account)** | Matches Shelly; better than Tuya, Govee, Yeelight |
| **Physical manual override button** | Most smart bulbs lose control when physically switched off [1]; SMARTLUX's button is always live |
| **Transparent, auditable firmware** | All commercial products are closed-source; SMARTLUX can explain its logic step by step |
| **Sub-$10 BOM cost for full sensing + measurement** | Shelly Plug PM Gen3: ~$20–25 for energy monitoring alone, mains only [20] |
| **INA219 accuracy at low DC loads** | Better than Tuya budget plug (27–35% error at 3–5 W) [6]; comparable to Shelly Plus PM at higher loads |
| **CSV export and 60-sample history** | Not standard on any consumer bulb product |
| **NVS-persisted totals** | No equivalent in consumer products |
| **mDNS, token auth, OTA with password** | Matches or exceeds typical DIY Sonoff/Shelly security model out of the box |

### 9.3 Ranked Adoption List

The following improvements are ranked by **impact** (I), **effort** (E), and **practicum fit** (P) on a 1–3 scale (3 = highest).

#### Tier 1: Zero Cost, Firmware Only

| # | Improvement | I | E | P | Rationale |
|---|---|---|---|---|---|
| 1 | **Document LDR calibration procedure** (map raw ADC to approximate lux) | 3 | 1 | 3 | SNI 6197:2020 requires lux-based lighting design [42]; calibrating LDR to approximate lux units closes the loop between the standard and the measurement |
| 2 | **Expose delay-off setting via web dashboard** (adjustable without reflashing) | 2 | 2 | 3 | Enables classroom experiments varying delay from 5–200 s [23][41] and observing savings impact |
| 3 | **Add savings confidence interval to dashboard** (±X% based on INA219 accuracy assumptions) | 2 | 2 | 3 | Aligns with LBNL evidence-hierarchy best practice [28] |
| 4 | **Add always-on baseline lamp label to dashboard** (clearly label estimated vs. measured) | 3 | 1 | 3 | Critical for proposal honesty; Hue does estimated, SMARTLUX does better [34] |
| 5 | **PWM frequency experiment mode** (sweep 100 Hz, 500 Hz, 1 kHz, 5 kHz) | 1 | 2 | 3 | Physics practicum on flicker, driver compatibility, switching efficiency |

#### Tier 2: Minimal Hardware (~USD 1–5), High Value

| # | Improvement | Hardware Cost | I | E | P | Rationale |
|---|---|---|---|---|---|---|
| 6 | **BH1750 digital lux sensor** (I2C, 1–65,535 lux, ±20% accuracy) | ~$1 | 3 | 2 | Replaces LDR with calibrated lux reading in SI units; directly comparable to SNI 6197:2020 classroom 350 lux requirement [42]; daylight harvesting logic becomes lux-threshold based rather than ADC-threshold based |
| 7 | **DS18B20 temperature sensor** (1-Wire) | ~$1 | 1 | 1 | Useful for demonstrating that INA219 accuracy spec is "over temperature" [22]; also demonstrates thermal logging |
| 8 | **Small buzzer or LED** as "lamp would be off" indicator | ~$0.10 | 2 | 1 | Allows real-time verification of the savings counter: each activation/deactivation event is visible |

#### Tier 3: Moderate Hardware (~USD 5–20), Significant Upgrade

| # | Improvement | Hardware Cost | I | E | P | Rationale |
|---|---|---|---|---|---|---|
| 9 | **Second PIR sensor** for repeat-trigger mode comparison | ~$1–2 | 2 | 2 | Demonstrates sensor placement effect on savings (matches [10]'s optimized sensor placement finding); 5 min vs. 30 s delay-off comparison |
| 10 | **SHT31 humidity + temperature sensor** (I2C) | ~$3 | 1 | 1 | Extends to classroom comfort monitoring; relevant to future HVAC proposals |
| 11 | **SD card module** for unlimited CSV logging | ~$2 | 2 | 2 | Removes the 60-sample NVS limit; enables multi-day experiments |
| 12 | **TM1637 7-segment display** as secondary readout for Wh total | ~$1.50 | 1 | 1 | Visible to class without web dashboard |

### 9.4 "Do Not Copy" List

| Feature | Commercial Source | Why Not Applicable / Not Copy |
|---|---|---|
| **Mains switching (230 V)** | Shelly, Sonoff, Tapo, all commercial plugs | School safety rules forbid mains experiments [user context]; 5 V bench only |
| **Zigbee/Thread/BLE mesh** | Philips Hue, IKEA TRADFRI, Nanoleaf, Shelly Gen4 | Requires different radio hardware; unnecessary for single-lamp bench |
| **Cloud API / vendor account required** | Tuya, Govee, Yeelight, GE Cync (default) | Cloud lock-in violates SMARTLUX design philosophy; internet not guaranteed at school |
| **Hue MotionAware RF sensing (Ivani Sensify IP)** | Philips Hue / Ivani | Third-party IP; requires Zigbee mesh of mains-powered devices [35] |
| **DALI bus control (IEC 62386)** | Signify Interact, Dynalite, any DALI driver | Requires 16 V DALI bus, mains drivers, and ETS commissioning [3][45] |
| **KNX bus** | KNX, Legrand | Requires specialist ETS training, mains, and professional installation [14][45] |
| **Lutron Clear Connect 434 MHz** | Lutron | Proprietary patented protocol [50]; separate licensed radio required |
| **Doppler microwave occupancy counting** | US10438471B2 | Active patent (US); different sensing technology; not needed for bench |
| **Distributed proximity-following dimming** | US10728987B1 | Active patent (US); multi-fixture architecture; not needed for bench |
| **Unverified vendor energy savings percentages** | "Up to 75%" Signify, "up to 37%" Hue, etc. | These are from single buildings or estimated data [24][33][34]; cite them only as attributed vendor claims, not as independent evidence |
| **MID-approved energy metering claim** | None of the smart plugs are MID-approved [6] | Do not claim SMARTLUX measurements are legally traceable; acknowledge INA219 is not MID-certified |

---

## 10. PROPOSAL PHRASING GUIDANCE

### 10.1 General Principles

A school physics practicum proposal citing commercial products must distinguish:
- **Measured facts** (from datasheets, independent tests, or peer-reviewed studies)
- **Vendor marketing claims** (from brochures, product pages, press releases)
- **Estimated or simulated values** (from models, not direct measurement)

Failure to distinguish these invites academic criticism. The following section provides specific phrasing models.

### 10.2 Phrasing Templates

**For Philips Hue energy tracking:**
> "Philips Hue's energy dashboard estimates energy consumption from bulb type and nominal wattage, scaled by brightness level, without direct power measurement [citation to 34]. SMARTLUX, by contrast, measures actual current and voltage via an INA219 high-side monitor [citation to 22], enabling a direct comparison between measured and estimated approaches."

**For Signify's 75% savings claim:**
> "Signify reports energy savings of up to 75% for its Interact Pro system, based on a single installation at the GSA-operated Metcalfe Federal Building in Chicago [citation to 24][citation to 48]. This figure is from one building and has not been replicated across multiple independent sites. The Lawrence Berkeley National Laboratory's meta-analysis of 88 studies found average occupancy-based lighting savings of 24% in field installations, rising to 38% for multiple combined strategies [citation to 28][citation to 40]."

**For general occupancy savings:**
> "Research literature reports average lighting energy savings of approximately 24% for occupancy-based control alone, based on a meta-analysis of 240 estimates from 88 papers and case studies [citation to 28][citation to 40]. In education buildings specifically, occupancy savings averaged 31% and daylight-responsive savings averaged 49% in the same dataset [citation to 28]. SMARTLUX implements both control strategies in a single low-cost prototype."

**For Indonesian standards framing:**
> "SMARTLUX's design aligns with SNI 6197:2020 (*Konservasi Energi pada Sistem Pencahayaan*), which requires consideration of occupancy sensors, light intensity sensors, scheduling controls, and dimmers in energy-efficient lighting system design [citation to 42]. The standard specifies a minimum classroom illuminance of 350 lux with a maximum lighting power density of 11.95 W/m² for classrooms [citation to 42], providing quantitative benchmarks for evaluating the prototype's real-world applicability."

**For PLN tariff framing:**
> "Under PLN tariff category P-1/TR (applicable to public facilities including state schools with connected capacity 6,600 VA–200 kVA), the electricity rate is Rp 1,700 per kWh as of Q4 2026 under MEMR Regulation No. 7/2024 [citation to 29][citation to 30]. This tariff rate allows the SMARTLUX energy measurement system to translate measured Wh savings into a direct rupiah value, making abstract energy data economically tangible for students."

**For INA219 accuracy:**
> "The INA219 current/power monitor (Texas Instruments, datasheet Rev. G) achieves a maximum accuracy of ±0.5% over temperature in the B grade [citation to 22]. In the SMARTLUX system, additional error sources (shunt resistor tolerance, ADC offset) mean conservative system accuracy is approximately ±1–2% — suitable for physics practicum purposes and demonstrably better than budget smart plug products, which exhibit 27–35% error on loads below 5 W [citation to 6]."

**For avoiding overclaiming:**
> "SMARTLUX is a proof-of-concept bench prototype operating at 5 V DC. Its energy measurement is not legally traceable (no MID certification) and its savings estimates use a calculated always-on baseline rather than a simultaneous control-group measurement [citation to 28]. These limitations are consistent with educational physics instrumentation and should be disclosed in any formal energy audit context."

**For regulatory applicability to the school:**
> "MEMR Regulation No. 3/2025 requires government and regional government entities — including state schools — to implement energy conservation through energy management, periodic energy audits (at minimum every 3 years), and implementation of audit recommendations within 3 years [citation to 47]. SMARTLUX provides a low-cost, school-appropriate demonstration of the monitoring and control methodology underlying such audits, using components that comply with Indonesia's low-voltage safety context."

### 10.3 What Not to Do in the Proposal

1. **Do not state** "SMARTLUX saves 75% of lighting energy" — this figure is from one Signify building installation and is a marketing claim [24][48].
2. **Do not state** "PIR sensors save 50–60% of classroom lighting energy" without a source — cite specific studies and note whether they are simulations or field measurements [10][43].
3. **Do not claim** SMARTLUX measurements are equivalent to certified metering equipment.
4. **Do not reproduce** vendor brochure figures as if they were from independent research.
5. **Do not claim** the prototype works at mains voltage or is suitable for direct installation in classrooms — it is a bench demonstration that illustrates the sensing and measurement principles.

---

## COMPARISON TABLES — CONSOLIDATED REFERENCE

### Table A: SMARTLUX vs. Consumer Smart Lighting — Key Dimensions

| Dimension | SMARTLUX | Philips Hue | WiZ | IKEA TRADFRI/DIRIGERA | Shelly (DIY) |
|---|---|---|---|---|---|
| Sensing method | LDR + PIR HC-SR501 | PIR (add-on sensor) + ambient light [38] | PIR (add-on) + SpaceSense RF [2][37] | Zigbee motion sensor (optional) | GPIO-based (relays); external sensors |
| Control logic | AND(dark, motion) + 30 s delay-off + 2-level PWM | Motion + light level; schedules; geofencing [1] | Motion + light; circadian rhythm [2] | Motion + schedules [39] | Local or cloud automations |
| Dimming | 1 kHz PWM, GPIO26 → N-MOSFET | 0.2–100%, Zigbee commands [1] | App-controlled PWM | Zigbee PWM | Relay (on/off) or PWM models |
| Energy measurement | **INA219 real V/I/P (measured)** | Estimated from bulb type × brightness [34] | None | None | Shelly Plug PM (mains, AC) [20] |
| Savings tracking | Wh integration vs. always-on baseline (NVS) | None | None | None | Energy dashboard (mains AC) |
| Cloud dependency | None (local only) | Optional (local via Bridge) [39] | Optional (SpaceSense local) [37] | Optional (local via DIRIGERA) [39] | Optional; can disable [9][39] |
| Manual override | Physical button (AUTO/ON/OFF + long press) | Physical switch breaks control [1] | Physical switch breaks control [1] | Physical switch breaks control | Physical button on relay |
| Mains required? | **No (5 V DC bench)** | Yes (mains-powered bulbs) | Yes (mains-powered bulbs) | Yes (mains-powered bulbs) | Yes (mains relay/plug) |
| Price (components) | **< $10 USD total** | $60 Bridge + $15–70/bulb + $50 sensor [38] | $15–25/bulb + sensor add-on | $10–30/bulb + $65 hub | $10–30/device [9] |
| Academic transparency | **Full: open firmware, explainable** | Closed-source | Closed-source | Closed-source | Open API, closed firmware |
| Standards citations | SNI 6197:2020 applicable | ASHRAE 90.1 / Title 24 (US) [24] | — | — | CE/UL [9] |

### Table B: SMARTLUX vs. Commercial Architectural Systems

| Dimension | SMARTLUX | Signify Interact Pro | Lutron Vive | DALI (IEC 62386) | KNX |
|---|---|---|---|---|---|
| Scale | 1 lamp, bench | Up to 200 nodes, 64 zones [48][49] | Unlimited fixtures, 100+ brands [50] | 64/bus, 16 groups [14] | 57,375 devices [45] |
| Sensing | PIR + LDR (discrete) | PIR + daylight per luminaire [49] | PIR + daylight sensors [50] | DALI-2 Part 303/304 [4] | External sensors via bus |
| Commissioning | Flashing firmware | Bluetooth app, 5 min [48] | Wireless, no control wire [50] | DALI controller software | ETS specialist training [45] |
| Energy measurement | INA219 (real, DC) | Cloud energy monitoring (gateway tier) [48] | High-end trim savings 10–30% [50] | DALI-2 Part 252 (real, from driver) [4] | Via energy meter devices |
| Savings evidence base | LBNL meta-analysis applicable | 1 building (Metcalfe) [24][48] | Vendor claims [50] | Protocol only; independent studies apply | Protocol only |
| Mains required? | **No** | Yes | Yes | Yes | Yes |
| Cloud required? | **No** | Gateway tier only [48] | No for Vive local [50] | No | No |
| Approx. cost | **< $10** | ~$50–200/luminaire + gateway | Quote (thousands USD) | £50–150/luminaire [45] | £80–200/device [45] |
| Code compliance | SNI 6197:2020 (conceptual) | ASHRAE 90.1/Title 24/IECC/DLC [48] | ASHRAE 90.1/Title 24/IECC [50] | IEC 62386 [3][4] | ISO/IEC 14543 [14] |

### Table C: Energy Measurement Accuracy Comparison

| Device | Measurement Type | Load Range | Accuracy at 5 W | Accuracy at 2 kW | Mains? | Cost |
|---|---|---|---|---|---|---|
| INA219B (SMARTLUX) | DC real power (measured V × I) | 0–26 V bus | **±1–2% estimated (system)** | N/A (5 V only) | No | ~$1–2 |
| Shelly Plus Plug UK | AC real power (shunt + chip) | Resistive to inductive | +5.0% at 5 W [6] | +0.8% at 2 kW [6] | Yes | ~£22 |
| Budget Tuya plug | AC (apparent power risk) | Resistive only | +27.1% at 5 W [6] | ~+3.8% at 2 kW [6] | Yes | ~£10 |
| Philips Hue energy | **Estimated** (not measured) | — | N/A — model only [34] | N/A — model only | Yes | Included in app |

---

## NUMBERED REFERENCES

[1] TechRadar. "I test smart home tech for a living, and these are the top smart lights I recommend." https://techradar.com/news/best-smart-lighting

[2] The New York Times / Wirecutter. "The Best Smart LED Light Bulbs." https://www.nytimes.com/wirecutter/reviews/best-smart-led-light-bulbs/

[3] DALI Alliance. "Introduction — Digital Illumination Interface Alliance." https://dali-alliance.org/dali

[4] DALI Alliance. "Standards — IEC 62386." https://dali-alliance.org/standards/IEC62386.html

[5] IOPscience. "Modelling of lighting system utilizing natural and artificial lighting using DIALux." https://iopscience.iop.org/article/10.1088/1755-1315/969/1/012024

[6] Ocentrum. "Smart Plug Accuracy Test 2026: £10 vs £200 Monitors." https://ocentrum.com/smart-plug-accuracy-test

[7] Luis Llamas. "Main Smart Home Providers: Tuya, Sonoff, and Shelly." https://www.luisllamas.es/en/smart-home-providers-tuya-sonoff-shelly/

[8] Aqara Malaysia. "Aqara vs Sonoff vs Shelly vs Tuya in Malaysia: which wins?" https://aqaramalaysia.com/en/blog/aqara-vs-sonoff-vs-shelly-vs-tuya

[9] SmartHome Automate. "Tuya vs Shelly: Budget Smart Home Devices Compared." https://smarthomeautomate.com/learn/tuya-vs-shelly-budget-smart-home-compared

[10] MDPI Sustainability. "Sustainable Energy Performance Optimization of Occupancy Sensor Placement in Smart Lighting Systems for University Classrooms." https://mdpi.com/2071-1050/18/11/5772

[11] Google Patents. US10728987B1 — Distributed occupancy-tracking lighting control system. https://patents.google.com/patent/US10728987B1

[12] Google Patents. US10438471B2 — Occupancy based lighting control technology. https://patents.google.com/patent/US10438471B2/en

[13] Global Buildings Performance Network. "Indonesia." https://library.gbpn.org/beet-3/country-infosheets/indonesia

[14] YND LED. "DALI vs KNX Lighting Control: Differences, When to Use Each & How They Work Together." https://www.yndled.com/news/dali-vs-knx-lighting-control-comparison-guide

[15] CASE for SEA. "Rethinking Electricity Subsidies." https://caseforsea.org/wp-content/uploads/2025/11/24.09.25-English-Final-CASE-Rethinking-Electricity-Subsidies.pdf

[16] Gridwatch. "Indonesia Holds PLN Electricity Tariffs Flat For Third Consecutive Quarter." https://gridwatch.ph/indonesia-holds-pln-electricity-tariffs-flat-for-third-consecutive-quarter/

[18] LSPro IGS. "Why LED Lights Need SNI and SKEM (Minimum Energy Performance Standard)?" https://lsigs.com/en/article/why-led-lights-need-sni-and-minimum-energy-performance-standard-scheme/

[19] IEEFA. "Rethinking Indonesia's electricity economics to reach the 100GW solar target." https://ieefa.org/resources/rethinking-indonesias-electricity-economics-reach-100gw-solar-target

[20] Shelly Knowledge Base. "Shelly Plug PM Gen3." https://kb.shelly.cloud/knowledge-base/shelly-plug-pm-gen3

[21] Prompt Quorum. "Best Energy-Monitoring Smart Plugs for Home Assistant (2026)." https://www.promptquorum.com/smart-home/best-energy-monitoring-smart-plugs-2027

[22] Texas Instruments. "INA219 Zerø-Drift, Bidirectional Current/Power Monitor With I2C Interface datasheet (Rev. G)." https://ti.com/lit/ds/symlink/ina219.pdf

[23] DatasheetHub. "HC-SR501 PIR Motion Sensor Module." https://datasheethub.com/hc-sr501-pir-motion-sensor-module

[24] Signify. Controls Product Guide (SGn-2161PG). https://www.signify.com/content/dam/signify/en-us/portfolio/product-selection-guides/SGn-2161PG_Controls_product_guide.pdf

[25] Lutron. "Commercial Lighting Control Systems." https://www.lutron.com/us/en/controls/systems/commercial-systems

[26] US Department of Energy. "Lighting Specification Guidance for Schools" (Nov. 2024). https://energy.gov/sites/default/files/2024-12/lighting-spec-guidance-school_nov24.pdf

[27] Lighting Controls Academy. "Daylight-Responsive Control." https://lightingcontrolsacademy.org/category/topics/daylight-harvesting

[28] LBNL. "A Meta-Analysis of Energy Savings from Lighting Controls in Commercial Buildings" (LBNL-5095E). https://eta-publications.lbl.gov/sites/default/files/a_meta-analysis_of_energy_savings_from_lighting_controls_in_commercial_buildings_lbnl-5095e.pdf

[29] CNBC Indonesia. "Resmi! Daftar Tarif Listrik PLN per kWh, Berlaku Hingga Desember 2026." https://cnbcindonesia.com/news/20261102103438-4-781436/resmi-daftar-tarif-listrik-pln-per-kwh-berlaku-hingga-desember-2026

[30] Detik.com. "Tarif Listrik PLN Terbaru Juli 2026." https://www.detik.com/sumut/berita/d-8572147/tarif-listrik-pln-terbaru-juli-2026-cek-rincian-harga-per-kwh-semua-golongan

[31] Bloomberg Technoz. "Daftar Tarif Listrik PLN November 2026 Berdasarkan Golongan Daya." https://www.bloombergtechnoz.com/detail-news/126240/daftar-tarif-listik-pln-november-2026-berdasarkan-golongan-daya

[32] Kompas Money. "Apakah Tarif Listrik akan Naik? PLN Beri Kepastian Begini." https://money.kompas.com/read/2026/11/04/213000565/apakah-tarif-listrik-akan-naik-pln-beri-kepastian-begini

[33] Philips Hue Developer Program. "Connected Energy Consumption Study." https://developers.meethue.com/connectedconsumption-whitepaper/

[34] HueCommand. "Philips Hue Energy Use: Estimated kWh Dashboard." https://huecommand.com/hue-energy-tracking

[35] The Verge. "Inside Philips Hue's plans to make all your lights motion sensors." https://on.theverge.com/smart-home/768585/philips-hue-motion-aware-rf-sensing-lights-motion-ivani

[36] HomeAutomation.us. "Lutron Caseta vs RadioRA 3: Which Lutron System You Need." https://homeautomation.us.com/lighting/lutron-caseta-vs-ra3/

[37] WiZ Help Center. "What is the PIR motion sensor? How does it work?" https://faq.wizconnected.com/hc/en/7-wiz-v2/faq/653-what-is-the-pir-motion-sensor-how-does-it-work

[38] SmartHomeU. "Philips Hue Motion Sensor." https://smarthomeu.com/motion-sensors/philips-hue-motion-sensor

[39] HomeTechTested. "Best Smart Home Devices That Work Without Internet (2026)." https://hometechtested.com/blog/best-smart-home-devices-that-work-without-internet-2026

[40] Lighting Controls Academy. "LBNL Estimates Average Energy Savings for Popular Lighting Control Strategies." https://lightingcontrolsacademy.org/lbnl-estimates-average-energy-savings-for-popular-lighting-control-strategies

[41] EnergizeCT. "An Analysis of the Energy and Cost Savings Potential of Occupancy Sensors for Commercial Lighting Systems." https://energizect.com/sites/default/files/documents/2.1.1.1%20An%20analysis%20of%20the%20energy%20and%20cost%20savings%20potential%20of%20occupancy%20sensors%20for%20commercial%20lighting%20systems.pdf

[42] BSN. SNI 6197:2020 — Konservasi Energi pada Sistem Pencahayaan. https://iai-jatim.com/wp-content/uploads/2023/11/SNI-6197_2020-Konservasi-Energi-Pada-Sistem-Pencahayaan.pdf

[43] Lighting Controls Academy. "Estimating Energy Savings with Lighting Controls." https://lightingcontrolsacademy.org/estimating-energy-savings-with-lighting-controls/

[44] Lighting Controls Academy. "Study: Controls Combine to Deliver Large, Persistent Energy Savings and Improved Occupant Satisfaction in Open Office." https://lightingcontrolsacademy.org/study-controls-combine-to-deliver-large-persistent-energy-savings-and-improved-occupant-satisfaction-in-open-office

[45] Elec-Mate. "Smart Lighting Control UK: DALI, KNX & Lutron." https://elec-mate.com/smart-lighting-control

[46] Kontan Finansial. "Cek Tarif Listrik PLN Golongan Sosial dan Jenis Kegiatan yang Berlaku." https://finansial.kontan.co.id/news/cek-tarif-listrik-pln-golongan-sosial-dan-jenis-kegiatan-yang-berlaku

[47] JDIH ESDM. Permen ESDM No. 3 Tahun 2025 tentang Konservasi Energi oleh Pemerintah dan Pemerintah Daerah. https://jdih.esdm.go.id/dokumen/download?id=2025pmesdm3.pdf

[48] Signify. Interact Pro Scalable Brochure (2021). https://www.assets.signify.com/is/content/Signify/Assets/interact/united-states/20210803-interact-scalable-brochure.pdf

[49] Energy Product Evaluations. "Signify Interact Pro Scalable." https://energyproductevaluations.org/evaluated-product/signify-interact-pro-scalable

[50] Lutron. "Vive — Commercial Wireless Lighting Control System." https://lutron.com/us/en/controls/systems/vive

[51] Signify Interact Documentation. "Light behavior :: Signify Interact for Intelligent Buildings." https://sme.interact-lighting.com/documentation/interact-pro/2.7/system-guide/light-behavior.html

---

*This report synthesizes evidence from 51 sources retrieved November 24, 2026. All commercial product prices, PLN tariffs, and regulatory citations reflect the most recent available data as of that date. Treat price tiers and regulatory provisions as subject to change; verify MEMR and PLN sources directly before submission of a formal school proposal.*

---

## Full citation URLs (untruncated)

1. https://techradar.com/news/best-smart-lighting
2. https://www.nytimes.com/wirecutter/reviews/best-smart-led-light-bulbs/
3. https://dali-alliance.org/dali
4. https://dali-alliance.org/standards/IEC62386.html
5. https://iopscience.iop.org/article/10.1088/1755-1315/969/1/012024
6. https://ocentrum.com/smart-plug-accuracy-test
7. https://www.luisllamas.es/en/smart-home-providers-tuya-sonoff-shelly/
8. https://aqaramalaysia.com/en/blog/aqara-vs-sonoff-vs-shelly-vs-tuya
9. https://smarthomeautomate.com/learn/tuya-vs-shelly-budget-smart-home-compared
10. https://mdpi.com/2071-1050/18/11/5772
11. https://patents.google.com/patent/US10728987B1
12. https://patents.google.com/patent/US10438471B2/en
13. https://library.gbpn.org/beet-3/country-infosheets/indonesia
14. https://www.yndled.com/news/dali-vs-knx-lighting-control-comparison-guide
15. https://caseforsea.org/wp-content/uploads/2025/11/24.09.25-English-Final-CASE-Rethinking-Electricity-Subsidies.pdf
16. https://gridwatch.ph/indonesia-holds-pln-electricity-tariffs-flat-for-third-consecutive-quarter/
17. https://datacenter-pyc.org/data/economy/energy-price/electricity-tariff-in-indonesia/
18. https://lsigs.com/en/article/why-led-lights-need-sni-and-minimum-energy-performance-standard-scheme/
19. https://ieefa.org/resources/rethinking-indonesias-electricity-economics-reach-100gw-solar-target
20. https://kb.shelly.cloud/knowledge-base/shelly-plug-pm-gen3
21. https://www.promptquorum.com/smart-home/best-energy-monitoring-smart-plugs-2027
22. https://ti.com/lit/ds/symlink/ina219.pdf
23. https://datasheethub.com/hc-sr501-pir-motion-sensor-module
24. https://www.signify.com/content/dam/signify/en-us/portfolio/product-selection-guides/SGn-2161PG_Controls_product_guide.pdf
25. https://www.lutron.com/us/en/controls/systems/commercial-systems
26. https://energy.gov/sites/default/files/2024-12/lighting-spec-guidance-school_nov24.pdf
27. https://lightingcontrolsacademy.org/category/topics/daylight-harvesting
28. https://eta-publications.lbl.gov/sites/default/files/a_meta-analysis_of_energy_savings_from_lighting_controls_in_commercial_buildings_lbnl-5095e.pdf
29. https://cnbcindonesia.com/news/20261102103438-4-781436/resmi-daftar-tarif-listrik-pln-per-kwh-berlaku-hingga-desember-2026
30. https://www.detik.com/sumut/berita/d-8572147/tarif-listrik-pln-terbaru-juli-2026-cek-rincian-harga-per-kwh-semua-golongan
31. https://www.bloombergtechnoz.com/detail-news/126240/daftar-tarif-listik-pln-november-2026-berdasarkan-golongan-daya
32. https://money.kompas.com/read/2026/11/04/213000565/apakah-tarif-listrik-akan-naik-pln-beri-kepastian-begini
33. https://developers.meethue.com/connectedconsumption-whitepaper/
34. https://huecommand.com/hue-energy-tracking
35. https://on.theverge.com/smart-home/768585/philips-hue-motion-aware-rf-sensing-lights-motion-ivani
36. https://homeautomation.us.com/lighting/lutron-caseta-vs-ra3/
37. https://faq.wizconnected.com/hc/en/7-wiz-v2/faq/653-what-is-the-pir-motion-sensor-how-does-it-work
38. https://smarthomeu.com/motion-sensors/philips-hue-motion-sensor
39. https://hometechtested.com/blog/best-smart-home-devices-that-work-without-internet-2026
40. https://lightingcontrolsacademy.org/lbnl-estimates-average-energy-savings-for-popular-lighting-control-strategies
41. https://energizect.com/sites/default/files/documents/2.1.1.1%20An%20analysis%20of%20the%20energy%20and%20cost%20savings%20potential%20of%20occupancy%20sensors%20for%20commercial%20lighting%20systems.pdf
42. https://iai-jatim.com/wp-content/uploads/2023/11/SNI-6197_2020-Konservasi-Energi-Pada-Sistem-Pencahayaan.pdf
43. https://lightingcontrolsacademy.org/estimating-energy-savings-with-lighting-controls/
44. https://lightingcontrolsacademy.org/study-controls-combine-to-deliver-large-persistent-energy-savings-and-improved-occupant-satisfaction-in-open-office
45. https://elec-mate.com/smart-lighting-control
46. https://finansial.kontan.co.id/news/cek-tarif-listrik-pln-golongan-sosial-dan-jenis-kegiatan-yang-berlaku
47. https://jdih.esdm.go.id/dokumen/download?id=2025pmesdm3.pdf
48. https://www.assets.signify.com/is/content/Signify/Assets/interact/united-states/20210803-interact-scalable-brochure.pdf
49. https://energyproductevaluations.org/evaluated-product/signify-interact-pro-scalable
50. https://lutron.com/us/en/controls/systems/vive
51. https://sme.interact-lighting.com/documentation/interact-pro/2.7/system-guide/light-behavior.html
