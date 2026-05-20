```md
# Smiley Feedback Konsol – IoT Projekt

## Motivation

Smiley feedback konsollen er en sjov og dagligdags anordning, som vi genkender fra oplevelser i banken, borgerservice, varehuse og sundhedscentre.

Anordningen kan implementeres uden alt for store udfordringer og alligevel tilføjes kompleksitet i forbindelse med dataoverførsel og strømbesparelse. Derfor er det et passende intro-projekt til faget IoT og embeddede systemers anden del på H4.

---

# Produktkrav

- Anordningen har 4 knapper med tilhørende smileys.
- Der er respons på LED ved hver knap.
- På knapperne implementeres debounce-funktionalitet.
- Efter tryk på en knap er anordningen låst i 7 sekunder, før ny feedback modtages.
- Lysdioden ud for den trykkede knap lyser i alle 7 sekunder.
- Anordningen tilsluttes internet via Wi-Fi access point i undervisningslokalet.
- Anordningens ur synkroniseres løbende med NTP og korrekt tidszone.
- Knaptryk-data (værdi og timestamp) overføres via MQTT.
- Der bruges brugernavn og password, og hele kommunikationen sikres ved brug af TLS-kryptering.
- Anordningen designes strømbesparende ved brug af Deep Sleep og batteridrift.
- Anordningen skal spare strøm ved at skifte til Deep Sleep, når den er ubeskæftiget i et stykke tid.
- Eksperimenter og analyser skal vise de bedste tidsgrænser for:
  - hvor længe anordningen er ubeskæftiget, men aktiv
  - hvordan opvågning skal fungere, så anordningen opdager aktivitet og falder i søvn igen
- En god analyse og overvejelse danner grundlag for, hvor længe anordningen sover, inden den vågner rutinemæssigt.

---

# Proceskrav

- Opgaven løses i grupper på 2 personer.
- Eneste undtagelse er ved ulige antal elever, hvor der dannes en gruppe på 3 personer.
- Læreren tager initiativ og beslutter, hvem der deltager i gruppen på tre medlemmer.

## Versionsstyring

- Under hele projektforløbet skal versionsstyringssystemet Git bruges til at håndtere og registrere projektets filer.
- Alle gruppemedlemmer skal dagligt lave commits til repo’et.
- Det anbefales at repo’et pushes til GitHub, GitLab eller lignende.
- Læreren skal tilføjes til brugergruppen med adgang til:
  - hele repo’et
  - commit-historik
  - branches
  - øvrigt projektindhold

## Logbog

- Gruppen fører løbende en logbog.
- Alle commits skal dokumenteres mindst én gang om dagen.
- Repo’et skal derfor indeholde commit af dagbogen fra alle medlemmer hver dag.

## Arbejdsportfolio

Gruppen opbygger et arbejdsportfolio, hvor alle idéer og beslutninger beskrives:

- fra idé
- via skitse
- til færdig løsning

Alle bidrager til indholdet.

Arbejdsportfolien opdateres løbende, når der er relevant indhold at tilføje.

Indholdet kan blandt andet være:

- tekst
- skitser
- screenshots
- links
- noter
- inspiration

Både logbog og arbejdsportfolio skal committes til Git-repo’et, så progressionen dokumenteres.

## Dokumentation

Dokumentformatet Markdown er meget velegnet til detaljeret versionsstyring i Git.

## Samarbejde

Gruppen og læreren taler løbende sammen om:

- udfordringer
- fremdrift
- læring
- fremskridt

---

# Bedømmelse

Opgavebesvarelsen indgår som en del af den samlede bedømmelse for faget, proportionalt med den tid der er brugt til projektarbejdet.

Produktkravene og proceskravene prioriteres lige højt, fordi:

- produktet alene ikke afslører deltagerens viden om løsningen
- processen alene ikke nødvendigvis fører til en fungerende løsning

Begge dele skal mindst opnå karakteren **02** hver for sig for samlet at bestå.

En præstation over karakteren 12 i den ene del kan ikke kompensere for en præstation under 02 i den anden del.

## Vigtige vurderingspunkter

En løsning som:

- mangler TLS-kryptering
- mangler strømbesparelseselementer

kan ikke opnå karakteren **12** for produktkravene.

## Karakteren 12

Karakteren 12 gives, når:

- alle mål opfyldes
- løsningen kun har ubetydelige fejl eller mangler

Eksempler på mindre fejl kan være:

- periodisk usikker debounce
- usikker håndtering af tidszone
- mindre fejl i Deep Sleep-funktioner
- enkelte manglende begivenheder i logbogen
- mangelfulde beskrivelser i portfoliet

---

# Info

For information om den lokale MQTT-server, se:

- Lokal MQTT server

Her findes også eksempler på kode, som sender beskeder med MQTT.
```
