TAPE J-47 (Meteora Plugins) - registratore a nastro - VST3 per Windows, VST3 e AU per macOS
==========================================================================================

COMPILARE SU GITHUB
1. Estrai lo zip. Nel repository premi "Add file -> Upload files", seleziona tutto (Ctrl+A, compresa la
   cartella .github) e trascinalo. Commit changes. Il workflow trova da solo CMakeLists.txt.
   (Solo se nell'elenco non compare .github/workflows/build.yml: Add file -> Create new file con quel nome
   e incolla il file build.yml che trovi nello zip.)
2. Actions -> partono due compilazioni insieme (Windows e macOS). Alla spunta verde scarichi:
   - "Tape-J-47-Windows-VST3"   -> Tape J-47.vst3
   - "Tape-J-47-macOS-VST3-AU"  -> Tape J-47.vst3 e Tape J-47.component (Mac Intel e Apple Silicon)

INSTALLARE
Windows: chiudi la DAW, copia "Tape J-47.vst3" in C:\Program Files\Common Files\VST3 (sostituisci il vecchio),
         riapri e rifai la scansione.
macOS:   chiudi la DAW e copia
           Tape J-47.vst3       in  /Library/Audio/Plug-Ins/VST3        (oppure ~/Library/Audio/Plug-Ins/VST3)
           Tape J-47.component  in  /Library/Audio/Plug-Ins/Components  (oppure ~/Library/Audio/Plug-Ins/Components)
         Il plugin non e' firmato da Apple: la prima volta apri il Terminale e scrivi (una riga per file):
           xattr -dr com.apple.quarantine "/Library/Audio/Plug-Ins/VST3/Tape J-47.vst3"
           xattr -dr com.apple.quarantine "/Library/Audio/Plug-Ins/Components/Tape J-47.component"
         (se li hai messi nella tua cartella utente, usa ~/Library/... al posto di /Library/...)
         Poi riapri la DAW. In Logic, se non compare, rifai la scansione da Gestione plug-in.

VECCHI PROGETTI
Nome, codici e parametri sono gli stessi delle versioni precedenti: i progetti salvati si riaprono con le
loro impostazioni. BIAS e LO-FI sono nuovi e partono neutri (0 e spento), quindi il suono non cambia.

COMANDI
- SATURATION (0-12 sulla scala): spinge il segnale nel nastro (-12 / +24 dB, 0 dB al centro).
- MAKEUP GAIN (OUTPUT): -24 / +12 dB. Collegato a SATURATION: girando SATURATION, MAKEUP GAIN scende
  della stessa quantita' (piu' saturazione, stesso volume); poi lo ritocchi da solo.
- TAPE SPEED 7.5 / 15 / 30 IPS: clic vicino al numero, o sul pomello per la velocita' successiva.
- BIAS (UNDER - 0 - OVER): UNDER = piu' brillante e piu' spinto, OVER = piu' scuro e pulito.
  Doppio clic = taratura normale (0). Misure a 0 VU: UNDER circa +2 dB a 10 kHz, distorsione 0.74%;
  normale 0.57%; OVER circa -2.3 dB a 10 kHz, distorsione 0.44%.
- LO-FI (levetta su = acceso): nastro consumato tipo cassetta - meno acuti e meno bassi profondi,
  oscillazione del nastro (wow e flutter), un po' di fruscio e un filo di saturazione; entra ed esce in 80 ms.
- VU: INPUT DRIVE (nastro in ingresso) e TAPE SATURATION (uscita, arrossisce quando il nastro satura).
- Le bobine girano quando il plugin lavora; il logo M si accende come una valvola insieme ai VU e alle spie.
- Senza pomello, regolabili dalla DAW: Dry/Wet (100%) e Hiss (spento).
- Finestra: si apre a 1080 x 603, ridimensionabile dall'angolo o col clic destro (50-150%).

LICENZA: Copyright (c) 2026 Meteora Plugins, tutti i diritti riservati - vedi LICENSE.txt.
Componenti di terze parti (Airwindows MIT, JUCE): vedi THIRD_PARTY_NOTICES.txt.
Se la build da' errori, manda il log.
