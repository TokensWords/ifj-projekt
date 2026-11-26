# Překladač IFJ26

Vývojová kostra překladače z jazyka IFJ26 do jazyka IFJcode26.

Repozitář nyní definuje hranice modulů, společná datová rozhraní, vlastnictví paměti,
přenositelný překlad a vstupní body testů. Záměrně neobsahuje implementaci konečného automatu,
gramatiky, precedenční tabulky, syntaktické analýzy, sémantických pravidel, vyváženého stromu
ani výsledného generování kódu. Tyto části musí tým sám navrhnout, vytvořit a umět vysvětlit.

Varianta zadání je `vv-BVS`. Tabulka symbolů proto musí používat výškově vyvážený binární
vyhledávací strom ručně implementovaný týmem.

## Požadované chování

Výsledný program pracuje jako unixový filtr:

```text
stdin:  zdrojový program IFJ26
stdout: cílový program IFJcode26
stderr: pouze diagnostika
návrat: 0, 1, 2, 3, 4, 5, 6, 7, 10 nebo 99
```

Diagnostika se nikdy nesmí dostat do generovaného výstupu. Výstup IFJcode26 začíná řádkem
`.IFJcode26`. Produkční překladač nesmí otevírat síťová spojení, spouštět další procesy,
modifikovat soubory ani používat knihovny třetích stran.

## Architektura

```text
stdin
  |
  v
Lexikální analyzátor --Token--> Syntaktický analyzátor --SourceProgram-->
Sémantický analyzátor <--> Tabulka symbolů
  |
  | typovaný SourceProgram
  v
Generátor mezikódu
  |
  | IntermediateProgram
  v
Generátor IFJcode26 --> stdout
```

## Adresářová struktura

| Adresář | Úloha | Vstup | Výstup |
|---|---|---|---|
| `src/shared` | Sdílené typy, stavy a alokace | Základní hodnoty C | Společná rozhraní |
| `src/lexicalAnalyzer` | Lexikální analýza a odsazení | `FILE *` | Vlastněný `Token *` |
| `src/syntaxAnalyzer` | Rekurzivní sestup a precedenční analýza | `LexicalAnalyzer *` | Vlastněný `SourceProgram *` |
| `src/abstractSyntaxTree` | Abstraktní syntaktický strom | Data uzlů | Vlastněné uzly |
| `src/semanticAnalyzer` | Jména, rozsahy, typy a návraty | `SourceProgram *` | Otypovaný strom |
| `src/symbolTable` | Tabulka symbolů pomocí vv-BVS | `Symbol` | Vypůjčený nalezený symbol |
| `src/intermediateCode` | Tříadresný mezikód | Otypovaný `SourceProgram *` | `IntermediateProgram *` |
| `src/generator` | Text IFJcode26 | `IntermediateProgram *` | Zápis do `FILE *` |
| `tests` | Testy vytvořené AI a kontrolované lidmi | Moduly nebo program | Úspěch či chyba |

Každý blok má vlastní `README.md` s přesnými vstupy, výstupy, strukturami a vlastnictvím.

## Pravidlo pro `main.c`

Pouze `src/main.c` obsahuje funkci `main()`. Jednotlivé bloky další `main.c` nemají. Samostatné
testovací programy mohou mít vlastní vstupní funkci, protože se sestavují jako jiné spustitelné
soubory. Veřejné deklarace patří do hlaviček. Definice funkcí patří do odpovídajících `.c`
souborů. `main.c` nebude obsahovat ručně dopsané prototypy.

## Sdílené struktury

### `SourceLocation`

| Položka | Typ | Význam |
|---|---|---|
| `line` | `size_t` | Řádek číslovaný od jedné |
| `column` | `size_t` | Sloupec číslovaný od jedné |

### `Token`

| Položka | Typ | Význam |
|---|---|---|
| `type` | `TokenType` | Kategorie tokenu |
| `lexeme` | `char *` | Vlastněný text lexému |
| `lexemeLength` | `size_t` | Délka bez ukončovací nuly |
| `location` | `SourceLocation` | Pozice prvního znaku |
| `value` | `TokenValue` | Dekódovaná hodnota |

Volající vlastní každý token vrácený funkcí `lexicalAnalyzerNextToken()` a uvolní jej funkcí
`tokenDestroy()`.

### `AbstractSyntaxNode`

| Položka | Typ | Význam |
|---|---|---|
| `kind` | `AbstractSyntaxNodeKind` | Význam uzlu |
| `location` | `SourceLocation` | Zdrojová pozice |
| `inferredType` | `DataType` | Výsledek sémantické analýzy |
| `text` | `char *` | Jméno, operátor nebo řetězec |
| `value` | `AbstractSyntaxValue` | Hodnota literálu |
| `children` | `AbstractSyntaxNode **` | Vlastněné poduzly |
| `childCount` | `size_t` | Počet platných poduzlů |
| `childCapacity` | `size_t` | Počet alokovaných míst |

`SourceProgram` vlastní kořen a `sourceProgramDestroy()` rekurzivně uvolní celý strom.

### `Symbol`

| Položka | Typ | Význam |
|---|---|---|
| `name` | `const char *` | Klíč identifikátoru |
| `kind` | `SymbolKind` | Proměnná, funkce nebo typ |
| `dataType` | `DataType` | Datový typ symbolu |
| `mutable` | `bool` | Povolení přiřazení |
| `defined` | `bool` | Dokončená definice |
| `scopeDepth` | `size_t` | Hloubka rozsahu platnosti |
| `function` | `FunctionSignature` | Parametry a návratový typ |

Tabulka symbolů kopíruje jména a pole typů parametrů. Výsledek hledání je vypůjčený ukazatel.

### `IntermediateProgram`

| Položka | Typ | Význam |
|---|---|---|
| `instructions` | `IntermediateInstruction *` | Vlastněné instrukce |
| `count` | `size_t` | Počet platných instrukcí |
| `capacity` | `size_t` | Počet alokovaných míst |

Každá instrukce obsahuje operační kód, nejvýše tři operandy, jejich počet a zdrojovou pozici.

## Vlastnictví paměti

Konstruktory vracejí ukazatel nebo `NULL`:

```c
Structure *array = memoryAllocateArray(count, sizeof(Structure));
if (array == NULL) {
    return NULL;
}
```

Pravidla:

1. Funkce končící `Create` vrací vlastněnou paměť.
2. Vlastněný typ má odpovídající funkci `Destroy`.
3. Vypůjčený ukazatel nesmí přežít svého vlastníka.
4. Výstupní parametr se stává vlastněným pouze po úspěchu.
5. Chyba konstruktoru uvolní částečnou alokaci.
6. Rodičovský uzel vlastní své poduzly.
7. Mezikód vlastní instrukce a textové operandy.
8. Tabulka symbolů vlastní kopie vložených symbolů.

## Styl názvů

- Typy: jednoduchý PascalCase, například `Token`.
- Funkce: camelCase, například `lexicalAnalyzerNextToken()`.
- Proměnné: camelCase, například `currentToken`.
- Konstanty a hodnoty výčtů: velká písmena s podtržítky.
- Definice funkce: `dataType functionName(...)`.
- Řídicí příkaz: `while (condition)`.
- Žádné prefixy typu `TStructure`.
- Společný styl určuje `.clang-format`.

Značky `TODO(team)` nahraďte okamžitě. Každý soubor musí uvádět pouze skutečné autory,
jejich jména a FIT loginy.

## Sestavení

Je potřeba GNU Make a překladač podporující C17.

```sh
make
```

Výsledkem je právě jeden soubor `ifj26compiler`. Překladač lze změnit:

```sh
make CC=gcc
make CC=clang
```

Další cíle:

```sh
make test
make check
make build-check
make clean
```

Výchozí sestavení nepotřebuje nepovinné analytické nástroje.

## Testování

Testy vytvářejí AI agenti, kontrolují je lidé a jejich původ je označen v komentářích.

- Jednotkové testy ověřují datové struktury a malé části.
- Integrační testy posílají IFJ26 na standardní vstup.
- Chybové testy kontrolují přesné návratové kódy.
- Zlaté soubory porovnávají IFJcode26 po bajtech.
- Později interpret porovná běhový výstup po bajtech.

## Rozdělení práce

Doporučené vlastnictví:

1. Lexikální analyzátor a tokeny.
2. Syntaktický analyzátor a abstraktní strom.
3. Sémantická analýza a napojení tabulky symbolů.
4. Mezikód a generátor IFJcode26.

Všichni musí rozumět vv-BVS. Používejte malé commity, samostatné větve, kontrolu změn a vždy
sestavitelný hlavní vývojový stav.

## Vývojová a odevzdávaná struktura

Vnořené adresáře usnadňují nezávislou práci. Odevzdávaný archiv však smí mít zdroje pouze
v kořeni nebo přímo v `src/`. Před odevzdáním proto vznikne samostatný balicí krok, který zdroje
zploští, zachová hlavičky, přidá `Makefile`, `documentation.pdf` a `rozdeleni` a spustí oficiální
kontrolní skript.

Neodevzdávejte `build/`, `.git/`, binární soubory, metadata macOS ani vnořené vývojové složky.

## Omezení použití AI

Tato kostra a testy používají AI a jsou tak označené. Podle zadání musí tým ručně vytvořit
a pochopit:

- konečný automat lexikálního analyzátoru;
- LL gramatiku a LL tabulku;
- precedenční tabulku;
- syntaktickou analýzu odvozenou z těchto formalismů;
- implementaci výškově vyváženého vyhledávacího stromu;
- finální odevzdávanou dokumentaci.

Soubory README slouží jako vývojové poznámky. Nejsou finální dokumentací k odevzdání.
