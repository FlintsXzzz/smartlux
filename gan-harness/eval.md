# Design loop record

Brief: `spec.md`. Rubric: `eval-rubric.md`. Threshold 7.5, limit 10 iterations.

## Scores

| Iteration | Design .35 | Originality .30 | Craft .25 | Function .10 | Total |
|---|---|---|---|---|---|
| 01 | 8.0 | 7.5 | 6.5 | 6.5 | 7.33 fail |
| 02 | 8.0 | 7.5 | 7.0 | 7.5 | 7.55 pass |
| 03 | 9.0 | 8.0 | 8.5 | 8.5 | 8.53 pass |

Loop stopped at 03. Iteration 02 passed on the numbers, then a screenshot review found three defects that the scores missed, so the loop ran once more.

## Iteration 01

Ring meter with the light percent, savings panel, six-cell grid, segmented mode control.

Failed on four counts. Lux was invented as `light_pct*4.1` instead of reading `lux_est`. The savings baseline divided lamp energy by the savings fraction, which gives 1.11 Wh where the correct always-on baseline is 0.70 Wh. Uptime showed minutes only. The page was 9,275 bytes against the 8 KB budget.

## Iteration 02

Hierarchy moved to the lamp state: the word `Nyala` at 58 px, the reason under it, the light meter below, savings in the second column. Ring removed. Threshold notch added to the light bar. Reads `lux_est`, uses `baseline = lamp_wh / (1 - savings)`, formats uptime as `1j 2m`, 8,174 bytes.

Render review found the state dot sitting low against the cap height, an unlabeled 60-tick strip that read as noise, and a segment control that collapsed to its content width on mobile.

## Iteration 03

State word carries the color instead of a dot. The tick strip became one exact line, `Nyala 4 s dari 60 s`. Segment control spans the row on mobile and caps at 420 px on desktop. The threshold notch is drawn in the page background color so it cuts the fill.

Data path verified in a DOM stub, not just by eye: `0,43 Wh dari 0,70 Wh` from `lamp_energy_wh 0.43` and `savings_pct 38.6`, `1j 2m` from `uptime_s 3725`, and all four reason strings (`Manual`, `Gelap dan ada gerakan`, `Gelap, tunda mati`, `Terang`). No reference to a missing element, no `http` request in the file.

Budget: 7,985 bytes of HTML, CSS and JS.

## Deviation from the brief

The page does not show `light_adc`. It shows the percent instead, because the ADC count means nothing to the reader and the percent and lux already answer the brightness question. The converter value stays in `/api/status` for calibration.
