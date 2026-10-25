# Backlog

Věci, které se vynořily během vývoje a na které zatím nebyl čas: známé slabiny, možné chyby a nápady.
Hotové položky mažeme, rozhodnutí přesouváme do kódu nebo do komentářů u něj.

## Známé slabiny

### Styky (`Source/CastleCore/Private/Junction.cpp`)

- **Kolize styků na obou koncích krátké zdi.** Řezy z obou konců se mohou potkat nebo minout, ze zdi pak nic
  nezbude a polygony obou uzlů se mohou překrývat. Půdorys to jen označí (`WallBody::bConsumedByJoints`, v kreslení
  purpurová osa). Nejde vyřešit jen v core, bude potřeba omezení ve stavebním nástroji.
- **Roh bez prodloužení nemá zarážku.** Prodloužení končí nejdál ve středu strany užší zdi, prostý průsečík stran ale
  může ležet i za koncem krátké zdi. Příklad: max. šířka 5 m, min. délka 1 m, dvě zdi pod 90° → roh 2,5 m od uzlu.
  Řez zdi pak leží za jejím koncem a zeď je „spotřebovaná“ (viz předchozí bod).
- **Maximální délka prodloužení** je zatím 2 × šířka širší zdi na straně úhlu (`JunctionSettings::MaxExtensionFactor`).
  Provizorní hodnota, ověřit v praxi.
- **Dlouhý hrot ostrého vnějšího rohu.** Vzdálenost hrotu od uzlu roste s 1 / sin(θ / 2), kde θ je vnitřní úhel.
  Při 30° je to asi 3,9 × poloviční šířka. Zvážit automatické zkosení od určité délky.
- **Oblouky** se ve stycích zatím neřeší (mezera s obloukem zůstává `Unresolved`).
- **Hraniční úhly.** Přesně na hranici dvou pravidel rozhoduje tolerance. Obrys je v obou případech stejný,
  body cesty se ale mohou lišit (např. dva skoro totožné body).
- **Volný konec zdi** končí v půdorysu kolmým čelem v uzlu. Jiné tvary (zakulacení, zkosení) zatím nejsou.
- **Přednost zdí ve 3D.** Pravidlo „prodlužuje se širší zeď“ je čistě 2D. S výškami bude rozhodovat spíš přednost
  (věž proti hradbě, vyšší zeď proti nižší), a ta se nemusí shodovat se šířkou.
- **Faktor prodloužení pod 1.** Konec strany tenčí zdi leží vždy blíž než šířka širší zdi, takže od faktoru 1 je
  vždy v dosahu (komentář v `ExtendSide` zbytečně mluví o 2). Pod 1 může `IntersectRayWithCircle` vrátit vzdálenější
  ze dvou průsečíků s kružnicí. Debug actor faktor povoluje od 0. Buď faktor omezit na ≥ 1, nebo rozhodnout,
  který průsečík brát.

### Půdorys (`Source/CastleCore/Private/FloorPlan.cpp`)

- **Velký polygon uzlu u ostrých úhlů.** Řez vede přes nejvzdálenější bod napojení, u ostrého úhlu tedy přes průsečík
  vnitřních stran. Při 30° a stejných šířkách je to asi 3,7 × poloviční šířka od uzlu, celý ten kus patří uzlu.
  Ověřit, jestli to bude vadit při kreslení prvků uzlu (vyšší sekce cimbuří, kůl palisády).
- **Spoj mimo meze ve 3D** je oříznutý kruhem (2 × šířka nejširší zdi) a o 5 cm vyšší než zdi, které pod ním končí
  surovým čelem a překrývají se. Rovné plochy překrývajících se šedých zdí leží ve stejné výšce; zatím to není vidět,
  ale s texturou by mohly blikat (z-fighting).
- **Uzel s obloukem** zůstává nevyřešený: zdi v něm končí kolmým čelem a překrývají se (`FloorPlan::UnresolvedNodes`).
  Totéž platí pro uzly mimo meze, které hra předává jako přeskočené.
- **Polygon uzlu se zjednodušuje** (odstraní se body na přímce mezi sousedy). Body, kde se strana zdi napojuje na
  cestu mezery, v polygonu proto nemusí být. Až bude kreslení uzlu potřebovat vědět, který úsek obrysu patří které
  zdi, bude potřeba bohatší popis než holý polygon.

### Síť zdí (`Source/CastleCore/Public/CastleCore/WallNetwork.h`)

- **Překryvy mimo uzly.** Styky řeší jen zdi, které sdílí uzel. Dvě zdi bez společného uzlu se díky své šířce mohou
  překrývat (zeď vedená těsně kolem cizího uzlu, dvě souběžné zdi blíž než jejich šířky). Pravidlo „zdi se
  potkávají jen v uzlech“ síť nevynucuje, přijme i zdi, které se kříží bez uzlu. Bude potřeba kontrola minimálního
  odstupu ve stavebním nástroji, nebo sjednocení půdorysů.
- **Dvě rovné zdi mezi stejnými uzly** se úplně překryjí. Síť je povoluje kvůli dvojici oblouků (kulatá věž),
  rovné duplicity by měla odmítnout.
- **Uzly ve stejném místě a zeď nulové délky.** Směr zdi pak vyjde nulový a úhly v uzlu nedávají smysl.
  Meze to zachytí (`TooShort`), síť to ale přijme. Stavební nástroj by měl blízké uzly slučovat.
- **Kontrola vstupů.** `AddWall` pustí šířku NaN (`< 0.0` je pro NaN nepravda), `Bulge` ani pozice uzlů se na
  konečné hodnoty nekontrolují. Důležité nejpozději při načítání uložené hry.
- **Platnost ukazatelů.** Ukazatele a `span` z `FindNode`, `FindWall` a `GetNodeWalls` přestanou platit po `AddNode`,
  `AddWall` a `RemoveWall`. V dokumentaci to zatím není.

### Meze (`Source/CastleCore/Public/CastleCore/WallLimits.h`)

- Hodnoty jsou jen návrh.
- Vztahy mezi mezemi se nekontrolují (např. minimální délka zdi vůči šířce sousedních zdí).
- **Úhel v uzlu se počítá dvakrát.** `GetSmallestJointAngle` bere úhly po dvojicích, styky ze seřazených směrů, a směr
  zdi od uzlu je napsaný v `WallLimits.cpp` i `Junction.cpp`. Sjednotit (minimum z `Junction::Gaps[i].Angle`, nebo
  pomocná funkce ve `WallNetwork`). Komentář u smyčky v `GetSmallestJointAngle` mate: sousedství se nevyužívá.

### Křivky (`Source/CastleCore/Public/CastleCore/WallCurve.h`)

- **Přetečení v `Tessellate`.** Krok je zdola omezený jen na 1e-9, počet dílů pak přeteče `int` a na MSVC jemný krok
  vrátí jediný díl. NaN projde taky. Debug actor drží krok ≥ 0,5°, takže se to zatím nestane.

### Testy (`Tests/CastleCore`)

- Googletest se stahuje bez `URL_HASH`, obsah archivu se tedy nekontroluje.

### Kamera (`Source/Castle/CastleCameraPawn.*`)

- **Bez hranic.** WASD odjede libovolně daleko od hradu, pozice pivotu se nijak neomezuje.
- **Pevná výška pivotu.** Pivot zůstává ve výšce, kde se pawn objevil (u PlayerStartu kus nad zemí), kolem ní se otáčí
  i zoomuje. S terénem ho bude potřeba přichytávat k zemi trasováním dolů. Rameno má vypnutou kolizi, kamera by pak
  mohla zajet do kopce.
- **Pauza.** Kamera běží na reálném čase, takže zpomalení nebo zrychlení hry ji neovlivní. V pauze ale stojí.
  Aby v ní fungovala, potřebuje controller `bShouldPerformFullTickWhenPaused` (jinak se v pauze neaktualizuje camera
  manager), pawn a spring arm `bTickEvenWhenPaused` (dojezd zoomu). Plný tick controlleru ale v pauze pustí všechny
  vstupy (`TickPlayerInput` dostane `bGamePaused = false`), `bTriggerWhenPaused` u akcí pak nerozhoduje. Vyřešit
  spolu s návrhem pauzy: co všechno má v pauze fungovat (stavění?).
- **Nejednotná plynulost.** Zoom dojíždí plynule, posun a rotace se rozjedou i zastaví okamžitě.
- **Vstupy natvrdo v kódu.** Akce a mapping context vznikají v C++, nejdou přemapovat ani vidět v editoru.
  S dalšími akcemi přejít na assety `IA_`/`IMC_`. Stavební nástroj (`CastleWallBuildComponent`) zatím vytváří své
  akce v kódu stejně jako kamera.
- **`MinZoom ≤ MaxZoom` se nekontroluje.** Obě hodnoty jsou editovatelné, po prohození se `Clamp` chová nesmyslně.

### Stavební nástroj (`Source/Castle/CastleWallBuildComponent.*`, `Source/CastleCore/Public/CastleCore/WallPlacement.h`)

- **Náhled kopíruje celou síť** při každém pohybu myši a celá se překresluje. U malých sítí nevadí, u velkého hradu
  bude potřeba počítat jen změněné okolí.
- **Kurzor nad lištou** se dál promítá na zem, náhled se tedy hýbe i pod lištou.
- **Kurzor a kruh přichycení** se kreslí přes `DrawDebug*`, které v Shipping buildu nejsou. Síť zdí se kreslí přes
  vlastní `ULineBatchComponent` a funguje všude.

### 3D zdi (`Source/Castle/CastleWallMesh.*`)

- **Typ zdi v modelu chybí.** `Wall` v core nezná svůj typ, styl (výška, barva) je jeden pro celou síť. S více typy
  přidat typ do modelu a rozhodnout, jak vypadá kus uzlu mezi různými typy (výška, materiál, prvky jako kůl palisády
  nebo vyšší sekce cimbuří).
- **Vše se staví znovu.** Při každé změně a v náhledu každý snímek se znovu počítají styky, půdorys, meze i celý mesh,
  navíc zvlášť pro čáry a pro mesh. Později přepočítávat jen změněné okolí, případně mesh po zdech.
- **Hranaté oblouky.** Každý 5° dílek oblouku má vlastní rovnou normálu. Pro hladký oblouk sdílet normály podél něj.
- **Bez UV a tangent.** Textury budou potřebovat UV (podle světa, nebo podél zdi).
- **Bez kolize.** Zdi nejsou překážkou pro trasování ani navigaci.
- **Pruhy na stěnách (shadow acne).** Stěna, na kterou slunce svítí pod ostrým úhlem, si sama na sebe vrhá stín
  v pruzích a na hranici rozlišení stínové mapy se pruhy lámou. Je to vlastnost stínů, ne geometrie, proto se objeví
  jen u některých natočení zdi. Ověřit otočením slunce; řešit nastavením biasu stínů světla nebo virtuálních stínových
  map, až budou zdi mít skutečný materiál.

### Debug actor (`Source/Castle/CastleWallDebugActor.*`)

- Už položená instance si drží uložené `Nodes` a `Walls`. Nové výchozí případy se objeví jen u nového actoru
  nebo po Reset to Default.

## Nápady

- **Stavební nástroj:** přichytávání ke zdem s jejich rozdělením (napojení doprostřed, T) a k úhlům (15°, 45°, 90°),
  rovnoběžky, kolmice a prodloužení jako vodící linky, mazání zdí. Hotové je pokládání s náhledem, řetězení
  a přichytávání k uzlům.
- **Kamera:** zoom směrem ke kurzoru místo ke středu obrazovky, sklápění kamery k vodorovné při přiblížení,
  ovládání myší (rotace tažením prostředním tlačítkem, posun tažením nebo u okraje obrazovky).
- **Undo/redo v core:** `FScopedTransaction` v UE funguje jen v editoru.
- **Režim rohu (ostrý/zkosený)** jako přepínač u uzlu v UI.
- **Místnosti** jako plochy planárního grafu zdí.
- **Kulatá věž** ze dvou půlkruhových oblouků mezi stejnými uzly.
- **Typy zdí** (hradba, palác, příčka), otvory (brány, okna), cimbuří jako instancované meshe.
- **SVG výstup z testů core** pro vizuální kontrolu bez spouštění UE.
- **Náhodné testy styků:** generovat náhodné uzly a zdi v rámci mezí a kontrolovat obecné vlastnosti, například že
  cesta mezery nevede dovnitř žádné ze zdí a že obrys sám sebe neprotíná. Najde slabiny systematicky, ne jen ty,
  které nás napadnou.
- **Později:** terén, vertikála (patra, ochozy).
