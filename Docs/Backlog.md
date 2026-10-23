# Backlog

Věci, které se vynořily během vývoje a na které zatím nebyl čas: známé slabiny, možné chyby a nápady.
Hotové položky mažeme, rozhodnutí přesouváme do kódu nebo do komentářů u něj.

## Známé slabiny

### Styky (`Source/CastleCore/Private/Junction.cpp`)

- **Kolize styků na obou koncích krátké zdi.** Úpravy obrysu z obou konců se mohou potkat nebo překrýt.
  Nejde vyřešit jen v core, bude potřeba omezení ve stavebním nástroji.
- **Roh bez prodloužení nemá zarážku.** Prodloužení končí nejdál ve středu strany užší zdi, prostý průsečík stran ale
  může ležet i za koncem krátké zdi. Příklad: max. šířka 5 m, min. délka 1 m, dvě zdi pod 90° → roh 2,5 m od uzlu.
- **Maximální délka prodloužení** je zatím 2 × šířka širší zdi na straně úhlu (`JunctionSettings::MaxExtensionFactor`).
  Provizorní hodnota, ověřit v praxi.
- **Dlouhý hrot ostrého vnějšího rohu.** Vzdálenost hrotu od uzlu roste s 1 / sin(θ / 2), kde θ je vnitřní úhel.
  Při 30° je to asi 3,9 × poloviční šířka. Zvážit automatické zkosení od určité délky.
- **Oblouky** se ve stycích zatím neřeší (mezera s obloukem zůstává `Unresolved`).
- **Hraniční úhly.** Přesně na hranici dvou pravidel rozhoduje tolerance. Obrys je v obou případech stejný,
  body cesty se ale mohou lišit (např. dva skoro totožné body).

### Meze (`Source/CastleCore/Public/CastleCore/WallLimits.h`)

- Hodnoty jsou jen návrh.
- Vztahy mezi mezemi se nekontrolují (např. minimální délka zdi vůči šířce sousedních zdí).

### Debug actor (`Source/Castle/CastleWallDebugActor.*`)

- Už položená instance si drží uložené `Nodes` a `Walls`. Nové výchozí případy se objeví jen u nového actoru
  nebo po Reset to Default.

## Nápady

- **Stavební nástroj:** přichytávání k uzlům, zdem a úhlům (15°, 45°, 90°), rovnoběžky, kolmice a prodloužení
  jako vodící linky, náhled před potvrzením, rozdělení zdi při napojení doprostřed (T).
- **Undo/redo v core:** `FScopedTransaction` v UE funguje jen v editoru.
- **Režim rohu (ostrý/zkosený)** jako přepínač u uzlu v UI.
- **Místnosti** jako plochy planárního grafu zdí.
- **Kulatá věž** ze dvou půlkruhových oblouků mezi stejnými uzly.
- **Typy zdí** (hradba, palác, příčka), otvory (brány, okna), cimbuří jako instancované meshe.
- **SVG výstup z testů core** pro vizuální kontrolu bez spouštění UE.
- **Později:** terén, vertikála (patra, ochozy).
