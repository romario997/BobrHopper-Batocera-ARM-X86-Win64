#include "ui/lang.h"

#include "engine/strings.h"
#include "ui/ranks.h"

namespace cr {

// the level's name, the same in both languages: five levels to a world (1-1 .. 1-5, 2-1 ...)
std::string levelLabel(int level)
{
    const int n = level > 0 ? level - 1 : 0;
    return toString(n / 5 + 1) + "-" + toString(n % 5 + 1);
}

namespace lang {

// { English, Polish, Spanish, Latin, Czech, Slovak, Hungarian, Romanian, Volapuk, Esperanto } in the order of enum
// Str. Spanish and Latin (the Latin for fun, at the author's request) came later, the last six later still; the font
// draws capitals for every case, so case here is only for reading.
static const char *const kText[Count][kLanguages] = {
    {"CLASSIC", "KLASYCZNA", "CLÁSICO", "CLASSICUS", "KLASIKA", "KLASIKA", "KLASSZIKUS", "CLASIC", "KLATIK", "KLASIKA"},
    {"PROGRESSION", "PROGRESJA", "PROGRESIÓN", "PROGRESSIO", "POSTUP", "POSTUP", "HALADÁS", "PROGRESIE", "LÖPIKAM", "PROGRESO"},
    {"CONTINUE", "KONTYNUUJ", "CONTINUAR", "PERGE", "POKRAČOVAT", "POKRAČOVAŤ", "FOLYTATÁS", "CONTINUĂ", "FÖVÖN", "DAŬRIGI"},
    {"NEW GAME", "NOWA GRA", "NUEVA PARTIDA", "NOVUS LUDUS", "NOVÁ HRA", "NOVÁ HRA", "ÚJ JÁTÉK", "JOC NOU", "PLED NULIK", "NOVA LUDO"},
    {"DELETE PROGRESS?", "SKASOWAĆ POSTĘP?", "¿BORRAR EL PROGRESO?", "PROGRESSUM DELERE?", "SMAZAT POSTUP?", "VYMAZAŤ POSTUP?", "TÖRLÖD A HALADÁST?", "ȘTERGI PROGRESUL?", "MOÜKÖN LÖPIKAMI?", "ĈU FORIGI PROGRESON?"},
    {"YES", "TAK", "SÍ", "ITA", "ANO", "ÁNO", "IGEN", "DA", "SI", "JES"},
    {"NO", "NIE", "NO", "NON", "NE", "NIE", "NEM", "NU", "NO", "NE"},
    {"LEVEL", "POZIOM", "NIVEL", "GRADUS", "ÚROVEŇ", "ÚROVEŇ", "SZINT", "NIVEL", "NIVÖD", "NIVELO"},
    {"RANK", "RANGA", "RANGO", "ORDO", "HODNOST", "HODNOSŤ", "RANG", "RANG", "GRED", "RANGO"},
    {"NO RANK YET", "BEZ RANGI", "SIN RANGO", "NULLUS ORDO", "BEZ HODNOSTI", "BEZ HODNOSTI", "MÉG NINCS RANG", "FĂRĂ RANG", "NEN GRED", "SEN RANGO"},
    {"LEVEL DONE", "POZIOM ZALICZONY", "NIVEL SUPERADO", "GRADUS PERACTUS", "ÚROVEŇ SPLNĚNA", "ÚROVEŇ SPLNENÁ", "SZINT TELJESÍTVE", "NIVEL TERMINAT", "NIVÖD PEFINON", "NIVELO FINITA"},
    {"NEW RANK", "NOWA RANGA", "NUEVO RANGO", "NOVUS ORDO", "NOVÁ HODNOST", "NOVÁ HODNOSŤ", "ÚJ RANG", "RANG NOU", "GRED NULIK", "NOVA RANGO"},
    {"TRY AGAIN", "SPRÓBUJ JESZCZE RAZ", "INTÉNTALO DE NUEVO", "ITERUM CONARE", "ZKUS TO ZNOVU", "SKÚS TO ZNOVA", "PRÓBÁLD ÚJRA", "ÎNCEARCĂ DIN NOU", "STEIFÜLOLÖD DENU", "REPROVU"},
    {"NEW BEST", "NOWY REKORD", "NUEVO RÉCORD", "NOVUM OPTIMUM", "NOVÝ REKORD", "NOVÝ REKORD", "ÚJ REKORD", "RECORD NOU", "REKORD NULIK", "NOVA REKORDO"},
    {"SCORE", "WYNIK", "PUNTOS", "PUNCTA", "SKÓRE", "SKÓRE", "PONTSZÁM", "SCOR", "PÜNS", "POENTOJ"},
    {"TOP", "REKORD", "RÉCORD", "SUMMUM", "REKORD", "REKORD", "REKORD", "RECORD", "REKORD", "REKORDO"},
    {"PAUSED", "PAUZA", "PAUSA", "INTERMISSIO", "PAUZA", "PAUZA", "SZÜNET", "PAUZĂ", "PAUD", "PAŬZO"},
    {"RESUME", "WRÓĆ DO GRY", "CONTINUAR", "REDI AD LUDUM", "POKRAČOVAT", "POKRAČOVAŤ", "FOLYTATÁS", "CONTINUĂ", "FÖVÖN", "DAŬRIGI"},
    {"SETTINGS", "USTAWIENIA", "AJUSTES", "OPTIONES", "NASTAVENÍ", "NASTAVENIA", "BEÁLLÍTÁSOK", "SETĂRI", "PARAMETS", "AGORDOJ"},
    {"MENU", "MENU", "MENÚ", "INDEX", "MENU", "MENU", "MENÜ", "MENIU", "MENÜ", "MENUO"},
    {"EXIT", "WYJŚCIE", "SALIR", "EXITUS", "KONEC", "KONIEC", "KILÉPÉS", "IEȘIRE", "SEGOLÖN", "ELIRI"},
    {"BACK", "POWRÓT", "VOLVER", "REDI", "ZPĚT", "SPÄŤ", "VISSZA", "ÎNAPOI", "GEIKÖN", "REEN"},
    {"SOUNDS", "DŹWIĘKI", "SONIDOS", "SONI", "ZVUKY", "ZVUKY", "HANGOK", "SUNETE", "TONS", "SONOJ"},
    {"MUSIC", "MUZYKA", "MÚSICA", "MUSICA", "HUDBA", "HUDBA", "ZENE", "MUZICĂ", "MUSIG", "MUZIKO"},
    {"SHADOWS", "CIENIE", "SOMBRAS", "UMBRAE", "STÍNY", "TIENE", "ÁRNYÉKOK", "UMBRE", "JADS", "OMBROJ"},
    {"FULL", "PEŁNE", "COMPLETAS", "PLENAE", "PLNÉ", "PLNÉ", "TELJES", "COMPLETE", "LÖLIK", "PLENAJ"},
    {"SIMPLE", "PROSTE", "SIMPLES", "SIMPLICES", "JEDNODUCHÉ", "JEDNODUCHÉ", "EGYSZERŰ", "SIMPLE", "BALUGIK", "SIMPLAJ"},
    {"OFF", "WYŁ", "NO", "NON", "VYP", "VYP", "KI", "OPRIT", "NO", "NE"},
    {"ON", "WŁ", "SÍ", "ITA", "ZAP", "ZAP", "BE", "PORNIT", "SI", "JES"},
    {"FPS COUNTER", "LICZNIK FPS", "CONTADOR FPS", "NUMERUS FPS", "POČÍTADLO FPS", "POČÍTADLO FPS", "FPS SZÁMLÁLÓ", "CONTOR FPS", "NUMÄT FPS", "FPS-NOMBRILO"},
    {"VIEW", "WIDOK", "VISTA", "ASPECTUS", "POHLED", "POHĽAD", "NÉZET", "VEDERE", "LOGAM", "VIDO"},
    {"NORMAL", "NORMALNY", "NORMAL", "NORMALIS", "NORMÁLNÍ", "NORMÁLNY", "NORMÁL", "NORMAL", "NORMÄDIK", "NORMALA"},
    {"WIDE", "SZEROKI", "AMPLIA", "LATUS", "ŠIROKÝ", "ŠIROKÝ", "SZÉLES", "LARG", "VIDIK", "LARĜA"},
    {"CHARACTER", "POSTAĆ", "PERSONAJE", "PERSONA", "POSTAVA", "POSTAVA", "KARAKTER", "PERSONAJ", "PÖSOD", "ROLULO"},
    {"LANGUAGE", "JĘZYK", "IDIOMA", "LINGUA", "JAZYK", "JAZYK", "NYELV", "LIMBĂ", "PÜK", "LINGVO"},
    {"PLAYERS", "GRACZE", "JUGADORES", "LUSORES", "HRÁČI", "HRÁČI", "JÁTÉKOSOK", "JUCĂTORI", "PLEDANS", "LUDANTOJ"},
    {"CONTROL P1", "STEROWANIE G1", "CONTROL J1", "IMPERIUM L1", "OVLÁDÁNÍ H1", "OVLÁDANIE H1", "IRÁNYÍTÁS J1", "CONTROL J1", "GUVAM P1", "REGADO L1"},
    {"CONTROL P2", "STEROWANIE G2", "CONTROL J2", "IMPERIUM L2", "OVLÁDÁNÍ H2", "OVLÁDANIE H2", "IRÁNYÍTÁS J2", "CONTROL J2", "GUVAM P2", "REGADO L2"},
    {"1 PLAYER", "1 GRACZ", "1 JUGADOR", "1 LUSOR", "1 HRÁČ", "1 HRÁČ", "1 JÁTÉKOS", "1 JUCĂTOR", "1 PLEDAN", "1 LUDANTO"},
    {"2 PLAYERS", "2 GRACZE", "2 JUGADORES", "2 LUSORES", "2 HRÁČI", "2 HRÁČI", "2 JÁTÉKOS", "2 JUCĂTORI", "2 PLEDANS", "2 LUDANTOJ"},
    {"WINS", "WYGRYWA", "GANA", "VINCIT", "VYHRÁVÁ", "VYHRÁVA", "NYER", "CÂȘTIGĂ", "VIKODOM", "VENKAS"},
    {"DRAW", "REMIS", "EMPATE", "AEQUUM", "REMÍZA", "REMÍZA", "DÖNTETLEN", "EGAL", "LEIGIK", "EGALO"},
    {"PLAYER 1", "GRACZ 1", "JUGADOR 1", "LUSOR 1", "HRÁČ 1", "HRÁČ 1", "JÁTÉKOS 1", "JUCĂTOR 1", "PLEDAN 1", "LUDANTO 1"},
    {"PLAYER 2", "GRACZ 2", "JUGADOR 2", "LUSOR 2", "HRÁČ 2", "HRÁČ 2", "JÁTÉKOS 2", "JUCĂTOR 2", "PLEDAN 2", "LUDANTO 2"},
    {"ASK", "PYTAJ", "PREGUNTAR", "ROGA", "PTÁT SE", "PÝTAŤ SA", "KÉRDEZZ", "ÎNTREABĂ", "SÄKÖN", "DEMANDI"},
    {"HOW MANY PLAYERS?", "ILU GRACZY?", "¿CUÁNTOS JUGADORES?", "QUOT LUSORES?", "KOLIK HRÁČŮ?", "KOĽKO HRÁČOV?", "HÁNY JÁTÉKOS?", "CÂȚI JUCĂTORI?", "PLEDANS LIOMÖDIK?", "KIOM DA LUDANTOJ?"},
    {"ENDLESS RETRIES", "BEZ KOŃCA PRÓB", "REINTENTOS SIN FIN", "CONATUS INFINITI", "NEKONEČNÉ POKUSY", "NEKONEČNÉ POKUSY", "VÉGTELEN PRÓBA", "VIEȚI INFINITE", "STEIFÜLS NENFINIK", "SENFINAJ PROVOJ"},
    {"A CHOOSE   B BACK", "A WYBIERZ   B POWRÓT", "A ELEGIR   B VOLVER", "A ELIGE   B REDI", "A VYBRAT   B ZPĚT", "A VYBRAŤ   B SPÄŤ", "A VÁLASZT   B VISSZA", "A ALEGE   B ÎNAPOI", "A VÄLÖN   B GEIKÖN", "A ELEKTI   B REEN"},
    {"A START   SELECT SETTINGS", "A GRAJ   SELECT USTAWIENIA", "A JUGAR   SELECT AJUSTES", "A LUDE   SELECT OPTIONES", "A HRÁT   SELECT NASTAVENÍ", "A HRAŤ   SELECT NASTAVENIA", "A JÁTÉK   SELECT BEÁLLÍTÁSOK", "A JOACĂ   SELECT SETĂRI", "A PLEDÖN   SELECT PARAMETS", "A LUDI   SELECT AGORDOJ"},
    {"A SELECT   B BACK", "A WYBIERZ   B POWRÓT", "A ELEGIR   B VOLVER", "A ELIGE   B REDI", "A VYBRAT   B ZPĚT", "A VYBRAŤ   B SPÄŤ", "A VÁLASZT   B VISSZA", "A ALEGE   B ÎNAPOI", "A VÄLÖN   B GEIKÖN", "A ELEKTI   B REEN"},
    {"A CONFIRM   B CANCEL", "A POTWIERDŹ   B ANULUJ", "A CONFIRMAR   B CANCELAR", "A CONFIRMA   B ABROGA", "A POTVRDIT   B ZRUŠIT", "A POTVRDIŤ   B ZRUŠIŤ", "A MEGERŐSÍT   B MÉGSE", "A CONFIRMĂ   B ANULEAZĂ", "A FÜMEDÖN   B MOÜKÖN", "A KONFIRMI   B NULIGI"},
    {"A SELECT   B RESUME", "A WYBIERZ   B WRÓĆ DO GRY", "A ELEGIR   B CONTINUAR", "A ELIGE   B REDI AD LUDUM", "A VYBRAT   B POKRAČOVAT", "A VYBRAŤ   B POKRAČOVAŤ", "A VÁLASZT   B FOLYTATÁS", "A ALEGE   B CONTINUĂ", "A VÄLÖN   B FÖVÖN", "A ELEKTI   B DAŬRIGI"},
    {"LEFT RIGHT CHANGE   A SELECT   B BACK", "LEWO PRAWO ZMIEŃ   A WYBIERZ   B POWRÓT", "IZQ DER CAMBIAR   A ELEGIR   B VOLVER", "SINISTRA DEXTRA MUTA   A ELIGE   B REDI", "VLEVO VPRAVO ZMĚNA   A VYBRAT   B ZPĚT", "VĽAVO VPRAVO ZMENA   A VYBRAŤ   B SPÄŤ", "BAL JOBB VÁLTÁS   A VÁLASZT   B VISSZA", "ST DR SCHIMBĂ   A ALEGE   B ÎNAPOI", "NED DET VOTÖN   A VÄLÖN   B GEIKÖN", "MALDEKSTRE DEKSTRE   A ELEKTI   B REEN"},
    {"A CONTINUE   B MENU", "A KONTYNUUJ   B MENU", "A CONTINUAR   B MENÚ", "A PERGE   B INDEX", "A POKRAČOVAT   B MENU", "A POKRAČOVAŤ   B MENU", "A FOLYTATÁS   B MENÜ", "A CONTINUĂ   B MENIU", "A FÖVÖN   B MENÜ", "A DAŬRIGI   B MENUO"},
    {"SCREEN", "EKRAN", "PANTALLA", "TABULA", "OBRAZOVKA", "OBRAZOVKA", "KÉPERNYŐ", "ECRAN", "SKRIN", "EKRANO"},
    {"FULL", "PEŁNY", "COMPLETA", "PLENA", "CELÁ", "CELÁ", "TELJES", "COMPLET", "LÖLIK", "PLENA"},
    {"NARROW", "WĄSKI", "ESTRECHA", "ANGUSTA", "ÚZKÁ", "ÚZKA", "KESKENY", "ÎNGUST", "NABIK", "MALLARĜA"},
    {"PHONE", "TELEFON", "TELÉFONO", "TELEPHONUM", "TELEFON", "TELEFÓN", "TELEFON", "TELEFON", "TELEFON", "TELEFONO"},
    {"BEAVER", "BÓBR", "CASTOR", "CASTOR", "BOBR", "BOBOR", "HÓD", "CASTOR", "KASTOR", "KASTORO"},
    {"CHICKEN", "KURCZAK", "POLLO", "PULLUS", "KUŘE", "KURA", "CSIRKE", "PUI", "GOK", "KOKIDO"},
    {"BACON", "BEKON", "BEICON", "LARIDUM", "SLANINA", "SLANINA", "SZALONNA", "BACON", "BEKON", "LARDO"},
    {"PLAY", "GRAJ", "JUGAR", "LUDE", "HRÁT", "HRAŤ", "JÁTÉK", "JOACĂ", "PLEDÖN", "LUDI"},
    {"QUIT THE GAME?", "WYJŚĆ Z GRY?", "¿SALIR DEL JUEGO?", "LUDUM RELINQUERE?", "UKONČIT HRU?", "UKONČIŤ HRU?", "KILÉPSZ A JÁTÉKBÓL?", "IEȘI DIN JOC?", "SEGOLÖN SE PLED?", "ĈU ELIRI EL LA LUDO?"},
    {"ENTER - YES     ESC - NO", "ENTER - TAK     ESC - NIE", "ENTER - SÍ     ESC - NO", "ENTER - ITA     ESC - NON", "ENTER - ANO     ESC - NE", "ENTER - ÁNO     ESC - NIE", "ENTER - IGEN     ESC - NEM", "ENTER - DA     ESC - NU", "ENTER - SI     ESC - NO", "ENTER - JES     ESC - NE"},
    {"NIGHT MODE", "TRYB NOCNY", "MODO NOCHE", "MODUS NOCTIS", "NOČNÍ REŽIM", "NOČNÝ REŽIM", "ÉJSZAKAI MÓD", "MOD NOAPTE", "MOD NEITIK", "NOKTA REĜIMO"},
    {"NOT CONNECTED", "ODŁĄCZONY", "DESCONECTADO", "NON CONIUNCTUM", "NEPŘIPOJENO", "NEPRIPOJENÝ", "LEVÁLASZTVA", "DECONECTAT", "NO PEYÜMÖL", "MALKONEKTITA"},
};

static int g_language = 0;

void set(int language) { g_language = language >= 0 && language < kLanguages ? language : 0; }
int current() { return g_language; }

const char *t(Str s) { return s >= 0 && s < Count ? kText[s][g_language] : ""; }

} // namespace lang
} // namespace cr
