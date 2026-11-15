# Regresní testy IFJ26 podle specifikace

Tato sada obsahuje **350 samostatných vstupních programů** pro základ jazyka a rozšíření
FUNEXP, BOOLTHEN, CONTROL, ALGEBDT a LISTS. Obsahuje platné programy, neplatné programy
a případy běhových chyb. Malé případy izolují jednotlivá pravidla, skupina `combined`
ověřuje kombinace rozšíření.

**Očekávaný výstup je výstup přeloženého a interpretovaného programu.** Výstup samotného
překladače je IFJcode26 a mezi správnými implementacemi se může lišit; s textovým očekávaným
výstupem programu jej neporovnávej.

Testy, skript a tento popis vytvořil OpenAI Codex. Před zařazením do odevzdávané práce je
projděte v týmu. Použitý pokyn: vytvořit rozsáhlé testovací vstupy přesně podle dodané
specifikace IFJ26 a upřesnění z fóra, očekávaný stdout a oddělené návratové kódy překladu
a běhu; neupravovat implementaci překladače a nepřebírat jeho současné výsledky jako oracle.

## Podklady a rozsah

Podkladem je `Specifikace_IFJ26_a_IFJcode26.pdf`, revize uvedené na konci dodaného PDF,
zejména §1–4, §5.1, §5.2, §5.3, §5.5, §5.8 a přehled chyb v §6–7.1.
U opakovaných podtržítek a počátečních nul exponentu jsou zohledněna přiložená upřesnění
vyučujícího z fóra. Testy nesuplují LL gramatiku ani implementaci automatu nebo tabulky symbolů.

| Skupina | Počet | Oblasti |
|---|---:|---|
| `base` | 78 | Literály, operátory, řetězce, odsazení, komentáře, funkce, rekurze, vstup/výstup |
| `funexp` | 11 | Volání ve výrazech, výrazy v argumentech, vnoření, pořadí vyhodnocení a rámce |
| `boolthen` | 30 | Boolean, pravdivostní tabulky, priority, zkratové vyhodnocování, podmínky |
| `control` | 34 | Výrazy if, rozsahy, kroky, stráže, víceřádkové hlavičky, rozsah platnosti |
| `algebdt` | 40 | Sjednocení, aliasy, třídy, atributy, vzory, stráže, návratové typy a Maybe čtení |
| `lists` | 55 | Typy prvků, prázdné a vnořené seznamy, indexy, metody, rozsahy, konkatenace a for |
| `runtime` | 12 | Indexy, prázdné seznamy, podřetězce, nulový krok a celočíselné dělení nulou |
| `invalid` | 77 | Izolované lexikální, syntaktické a sémantické chyby včetně správných kódů |
| `combined` | 13 | Smysluplné kombinace uvedených rozšíření včetně hlavního demonstračního programu |

Skupina označuje zaměření testu, nikoli přepínač režimu překladače. V manifestu jsou navíc
uvedené závislosti `requires`. Sada je určená překladači s podporou všech pěti uvedených rozšíření.

Jde o konečnou regresní sadu, ne důkaz správnosti pro všechny možné programy. Nevyžaduje
LAMBDAS, VALMEM, EXTKW, INTERPOLATEDS ani PARAMLISTS. Nepředepisuje výstup instancí
datových tříd, pod-/přetečení, místní definice tříd, nepovinné pokročilé vzory ani jiná
chování, která zadání nevyžaduje nebo neurčuje. Například samotná absence `case _` není
automaticky označená za chybu: negativní test používá skutečně nepokrytou alternativu.

## Soubory každého případu

Ve složce `cases/<skupina>/` najdeš ke stejnému základu názvu:

| Přípona | Význam |
|---|---|
| `.ifj` | Zdrojový kód, který dostane váš překladač na standardní vstup |
| `.stdin` | Data, která bude za běhu číst přeložený program |
| `.expected.stdout` | Správný výstup přeloženého programu; může být prázdný |
| `.expected.compiler.status` | Správný návratový kód překladače |
| `.expected.runtime.status` | Správný návratový kód interpretu; existuje jen při očekávaném úspěchu překladu |

Zdroje, runtime vstupy a očekávané výstupy jsou ASCII. Očekávané řádky používají LF.
U chyby překladu se nic neinterpretuje; prázdný `.expected.stdout` u takového případu
neznamená požadavek na prázdný mezikód při neúspěšném překladu. Text chybových hlášení se
neporovnává. U běhové chyby se naopak porovnává i výstup, který měl vzniknout před chybou.

`CASES.md` obsahuje katalog všech očekávání a odkazů na části specifikace. Zápis výstupu
v katalogu používá Python repr: například `\n` znamená skutečný konec řádku, ne dva znaky.
Přesné bajty jsou v příslušném `.expected.stdout`.

Při ověření dne 2026-11-15 prošlo na aktuálním překladači 319 z 350 případů; 31 případů
hlásilo nesoulad s očekáváním. Seznam a tehdejší commit překladače jsou v
[CURRENT_FAILURES.md](CURRENT_FAILURES.md). Tento záznam nenahrazuje nové spuštění po změně kódu.

## Sestavení ve Windows

Potřebuješ Python 3.10+, GCC a oficiální interpret IFJcode26. Skript používá pouze standardní
knihovnu Pythonu. Následující příkazy jsou pro **CMD**, zejména přesměrování pomocí `<`.

Z kořene repozitáře:

```bat
cd /d "C:\Users\Martin\School\2BIT\Zima\IFJ\Projekt\ifj-projekt-main"
py tests\specificationRegression\run.py --build-only --cc "C:\msys64\ucrt64\bin\gcc.exe"
```

Vznikne `tests\specificationRegression\results\ifj26compiler.exe`. Sestavují se skutečné
zdroje překladače s C17 a include adresáři jednotlivých modulů. Produkční soubory se nemění.
Výstup sestavení je v `results/build.stdout` a `results/build.stderr`.

## Automatické spuštění a porovnání celé sady

```bat
py tests\specificationRegression\run.py --compiler tests\specificationRegression\results\ifj26compiler.exe --interpreter "C:\Users\Martin\School\2BIT\Zima\IFJ\Projekt\ic26int-windows-x86_64.exe"
```

Lze také sestavit a spustit vše jedním příkazem:

```bat
py tests\specificationRegression\run.py --build --cc "C:\msys64\ucrt64\bin\gcc.exe"
```

V druhém příkladu skript automaticky najde zadanou Windows verzi interpretu v nadřazené
složce projektu. Pro jiné umístění použij `--interpreter` jako v prvním příkladu.
Bezpečně pokračuje dalšími testy po běžném selhání případu a omezuje dobu běhu.

`PASS` znamená shodu požadovaných stavů a u platného programu také výstupu. Návratový kód
skriptu je 0 při shodě všech vybraných případů, 1 při nesouladu testu a 2 při problému
s nástroji, sestavením nebo konfigurací. `--compile-only` ověřuje pouze stav překladu;
výsledek `COMPILE_OK` v tomto režimu **není** ověřením běhového výstupu.

## Ruční překlad jednoho programu a jeho spuštění

Po sestavení z předchozího kroku:

```bat
if not exist tests\specificationRegression\results\manual mkdir tests\specificationRegression\results\manual
tests\specificationRegression\results\ifj26compiler.exe < tests\specificationRegression\cases\base\base_hello.ifj > tests\specificationRegression\results\manual\hello.ifjcode26 2> tests\specificationRegression\results\manual\hello.compiler.stderr
echo %ERRORLEVEL%
```

Kód překladu musí být 0. Teprve potom spusť mezikód:

```bat
"..\ic26int-windows-x86_64.exe" tests\specificationRegression\results\manual\hello.ifjcode26 < tests\specificationRegression\cases\base\base_hello.stdin > tests\specificationRegression\results\manual\hello.actual.stdout 2> tests\specificationRegression\results\manual\hello.runtime.stderr
echo %ERRORLEVEL%
```

Správný běhový kód je také 0 a výstup je `Hello, IFJ26!` následovaný jedním koncem řádku.
Porovnej svůj zachycený výstup s referenčním souborem:

```bat
py tests\specificationRegression\run.py --compare tests\specificationRegression\cases\base\base_hello.expected.stdout tests\specificationRegression\results\manual\hello.actual.stdout
```

Pro jiný případ nahraď zdroj i `.stdin` a očekávání stejným názvem testu. U programu,
který má skončit chybou překladu, porovnej návratový kód a interpret vůbec nespouštěj.

## Výběr případů a výsledky

```bat
py tests\specificationRegression\run.py --list --group lists
py tests\specificationRegression\run.py --check-fixtures
py tests\specificationRegression\run.py --compiler tests\specificationRegression\results\ifj26compiler.exe --group boolthen
py tests\specificationRegression\run.py --compiler tests\specificationRegression\results\ifj26compiler.exe --case combined_showcase
```

`--group` a `--case` můžeš opakovat. `--case` vybírá podle části názvu.
Každý běh ukládá `results/summary.json`. Ve složce daného případu jsou:

- `compiler.stdout`, `compiler.stderr`, `compiler.status`;
- při úspěšném překladu také `program.ifjcode26`;
- při spuštění `runtime.stdout`, `runtime.stderr`, `runtime.status`;
- `runtime.normalized.stdout`, který se skutečně porovnává s očekáváním;
- `output.diff.txt`, pokud se výstupy liší, a `result.json` se shrnutím případu.

Windows verze dodaného interpretu zapisuje výstup v textovém režimu CRT, který mění každé
vypsané LF na CRLF. Skript na Windows odstraní právě tuto jednu transportní změnu **jen ze
skutečného výstupu**. Původní `runtime.stdout` zůstává zachovaný. Mezery, tabulátory, počty
řádků a samostatné CR se nezahazují. Pro binární režim nebo jiný interpret lze úpravu
vypnout pomocí `--exact-bytes`. Při běhu na Linuxu se tato Windows úprava neprovádí.

Složka `results/` a Python cache jsou lokálně ignorované. Skript nic necommituje ani
nepushuje a nemění staré `.ps1` testy. `generateFixtures.py` slouží k reprodukci sady;
jeho spuštění znovu vytvoří referenční soubory ze zdrojových dat, proto jej nepoužívej
na zachování ručně upravených fixture souborů.
