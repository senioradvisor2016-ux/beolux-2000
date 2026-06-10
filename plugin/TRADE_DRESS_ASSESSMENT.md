# Trade dress & varumärke — beslutsunderlag

**Status:** namnbyte GENOMFÖRT 2026-06-10 (Beolux 2000 → **Germanium 2000 Deluxe**) · **Gäller:** Soundboys

> ✅ **Uppdatering:** Den HÖGA risken (namnet "Beolux"/"Beo-"-prefixet) är åtgärdad —
> produkten heter nu **Germanium 2000 Deluxe**. Kvar: panelens helhetsintryck (MEDEL,
> §4.2) och go/no-go-juristbedömningen inför kommersiell release. Texten nedan
> beskriver läget FÖRE namnbytet som underlag för det beslutet.

> ⚠️ **Detta är inte juridisk rådgivning.** Det är en intern produkt-/ingenjörsmässig
> riskbedömning som ska hjälpa er att (a) avgöra om ni behöver konsultera en IP-jurist
> före kommersiell release, och (b) veta vad ett "rent" alternativ skulle kräva.
> Go/no-go för kommersiell lansering ska bekräftas av jurist.

---

## 1. Varför detta beslut blockerar Fas 3-GUI:t

Fas 3c (skalbart GUI: vektorisering eller 2×/3×-assets av frontpanelen) innebär att
**rita om hela panelen i högre kvalitet**. Om panelen senare måste ändras av
juridiska skäl kastas det arbetet. Därför ska trade dress-frågan avgöras *innan*
vektoriseringen påbörjas — inte efter.

---

## 2. Vad trade dress är (kort)

Trade dress skyddar en produkts **visuella helhetsintryck** (form, färg, layout,
materialkänsla) när det fungerar som källidentifierare — dvs. när en konsument
känner igen tillverkaren på utseendet. Till skillnad från patent krävs ingen
registrering; skyddet uppstår genom inarbetning. För en plugin som återskapar en
ikonisk hi-fi-produkt är frågan: **skapar vårt UI ett helhetsintryck som en
betraktare förknippar med Bang & Olufsen?**

Två separata risker som ofta blandas ihop:

| | Skyddar | Vår exponering |
|---|---|---|
| **Varumärke (trademark)** | Namn, ord, logotyp | Produktnamnet "Beolux" + "Beo-"-prefixet |
| **Trade dress** | Visuellt helhetsintryck | Frontpanelens layout/material/typografi |

---

## 3. Vad panelen faktiskt återger (faktainventering ur koden)

Källa: `WireframeEditor.cpp`, kommentarer och assets.

**Redan undanröjt (bra):**
- ✅ Ingen B&O-logotyp i bygget (`// B&O logo plaque removed`, [WireframeEditor.cpp:2068](juce/Source/WireframeEditor.cpp))
- ✅ Namn-disclaimer finns i README ("Not affiliated with… Bang & Olufsen")
- ✅ DSP:n är fysikaliskt härledd ur servicemanual/mätningar — **inte** kopierad kod
  eller samplingar. (DSP omfattas inte av trade dress; den är vår.)

**Återges medvetet "photo-accurate" (≈10 kodkommentarer som `Authentic …`/`matches photo`):**
- Träram med horisontell ådring ("matches Beocord 2400 photo")
- Borstad aluminium-frontpanel med försänkta sub-paneler
- VU-mätarnas stil ("Beocord 2000 De Luxe VU — 18-layer rendering")
- Krom-transportspak ("Manöverspak per Beocord 2400 photo")
- Vertikal krom-hastighetscylinder, räkneverk, fader-layout
- Typografi "per B&O typography"
- Texten **"BEOLUX 2000 DELUXE"** i titelremsan

**Slutsats:** panelen är ett *avsiktligt fotorealistiskt återskapande* av en specifik
B&O-produkts utseende. Det är kärnan i trade dress-exponeringen.

---

## 4. Riskfaktorer — rangordnade

### 🔴 HÖG — Namnet "Beolux" / "Beo-"-prefixet
- "Beo-" är B&O:s systematiska varumärkesprefix: BeoCord, BeoGram, BeoLab,
  BeoSound, BeoVision, BeoPlay. "**Beo**lux" läses omedelbart som en B&O-produkt.
- Detta är en **varumärkesrisk** (förväxlingsbarhet), separat från trade dress,
  och den allvarligaste enskilda punkten — namn är lättast att angripa och lättast
  att fixa.
- **Intern motsägelse:** README:s disclaimer säger att produktnamnet är
  "BC2000DL / Danish Tape 2000", men det faktiska bygget heter **"Beolux 2000"**
  ([CMakeLists.txt:43](juce/CMakeLists.txt)) och panelen visar "BEOLUX 2000 DELUXE".
  Disclaimern skyddar alltså ett namn ni inte använder. Detta är i sig en svaghet
  (visar att namnval gjorts medvetet nära B&O).

### 🟠 MEDEL — Frontpanelens helhetsintryck
- Den fotorealistiska panelen (trä + borstad alu + VU-stil + spak + layout) är
  designad för att *kännas igen* som maskinen. Ju mer "authentic", desto starkare
  källidentifiering → desto större trade dress-exponering.
- Mildrande: maskinen är ~1968, men trade dress kan bestå om designen fortfarande
  förknippas med B&O (de är fortfarande verksamma och vårdar sitt designarv aktivt).

### 🟡 LÅG–MEDEL — "De Luxe / DELUXE"-suffixet + modellnummer "2000"
- Att återge originalets modellbeteckning förstärker källidentifieringen, men
  enbart ett modellnummer är svagt skyddsbart. Risk mest i kombination med ovan.

### 🟢 LÅG — Disclaimer
- Disclaimern finns och är rätt formulerad, men **friskriver inte trade dress** —
  en disclaimer hjälper mot "endorsement"-påståenden, inte mot att helhetsintrycket
  i sig är förväxlingsbart. Den är nödvändig men inte tillräcklig.

---

## 5. Tre alternativ

### Alternativ A — Behåll allt som det är
- **Insats:** noll.
- **Risk:** högst. "Beo-"-namnet + fotoreplikan + modellnummer i kombination är den
  klassiska trade dress/varumärkes-profilen. Lämpligt endast för icke-kommersiell
  hobbyutgåva, och även då är namnet sårbart.
- **Lämplig om:** aldrig kommersiellt, ingen distribution i skala.

### Alternativ B — Riskreducering (rekommenderas som minsta åtgärd)
Behåll den vintage-estetiken men kapa de tydligaste källidentifierarna:
1. **Byt namn** från "Beolux" till något utan "Beo-"-prefix och utan B&O-koppling
   (det disclaimern redan påstår: t.ex. "Danish Tape 2000" eller helt nytt).
   Synka README, CMake `PRODUCT_NAME`, panel-text, bundle-ID.
2. **Ta bort "DELUXE"** och överväg att inte återge exakt modellnummer i panel-texten.
3. **Generalisera panelen** lagom: behåll "dansk 60-tals rullbandspelare"-känslan
   men gör layout/proportioner/VU-stil tillräckligt egna att helhetsintrycket inte
   pekar på en *specifik* B&O-modell.
4. Behåll och stärk disclaimern.
- **Insats:** liten–medel (namnbyte är mekaniskt; panel-justering är design-iterationer).
- **Risk:** väsentligt lägre. Tar bort den lättaste angreppsvektorn (namnet) helt.

### Alternativ C — Clean-room visuell identitet
- Egen, originell GUI-design (egen färg/typografi/layout) som *inspireras* av
  60-tals dansk hi-fi utan att replikera en specifik maskin. DSP:n behålls oförändrad.
- **Insats:** störst (full GUI-omdesign — men det är ändå vad Fas 3c innebär).
- **Risk:** lägst. Detta är vägen om målet är en trygg kommersiell produkt i skala.
- **Bonus:** en egen identitet är också ett *varumärke ni själva äger* — bättre för
  Soundboys långsiktigt än att vara "den där B&O-kopian".

---

## 6. Rekommendation

1. **Gör namnbytet nu, oavsett väg.** Det är billigt, mekaniskt, och tar bort den
   allvarligaste (varumärkes-)risken. "Beolux" → namnet disclaimern redan anger.
   Detta blockerar inget och bör göras före nästa release.
2. **Innan Fas 3c-vektoriseringen:** ta ställning mellan **B** (justera nuvarande
   panel) och **C** (clean-room). Vektorisera inte den nuvarande fotoreplikan förrän
   detta är avgjort — annars riskerar ni att rita om allt två gånger.
3. **Om kommersiell release i skala är målet:** konsultera en IP-jurist med
   B-eller-C-frågan + namnförslaget som konkret underlag. En kort konsultation nu är
   billigare än en cease-and-desist efter lansering.

**Min ingenjörsmässiga rekommendation:** Alternativ **C** för det kommersiella spåret.
Fas 3c är ändå en full GUI-omritning — gör den till en *egen* identitet i stället för
en högre-upplöst kopia. Då blir det juridiska arbetet en grön bock i stället för en
återkommande risk, och Soundboys får ett varumärke man äger. Gör namnbytet (punkt 1)
omedelbart oavsett.

---

## 7. Vad som kräver jurist (inte detta dokument)

- Slutgiltig go/no-go för kommersiell lansering.
- Bedömning av om en justerad panel (Alt. B) är "tillräckligt egen".
- Varumärkesclearance för det nya namnet (att det inte krockar med *annat* än B&O).
- Jurisdiktion: EU (B&O är danskt — hemmaplan) vs. US-marknad skiljer sig.

---

## 8. Konkret åtgärdslista (det jag kan göra i kod direkt)

- [ ] Namnbyte Beolux → valt namn: `PRODUCT_NAME`, `DESCRIPTION`, bundle-ID i CMake,
      panel-titeltext, README, ev. preset-katalognamn (`Soundboys/Beolux 2000/`).
- [ ] Ta bort "DELUXE" ur titelremsan.
- [ ] (Vid Alt. B) panel-justeringar enligt §5.B.3.
- [ ] (Vid Alt. C) ny GUI-identitet som Fas 3c.
