// Autor implementacji: Antoni Łuczak
// Opis: Implementacja bazuje na algorytmie Mark & Sweep z amortyzacją.
#include "rstack.h"

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

// Ta struktura przechowuje pojedyńczy element stosu.
typedef struct rnode {
  bool holds_a_stack;
  // W tej unii znajduje się element przechowywany przez węzeł.
  union {
    struct rstack* rs;
    uint64_t value;
  };
  // To pole zawiera wskaźnik do kolejnego węzła stosu.
  struct rnode* next;
} rnode_t;

typedef struct rstack {
  // To pole mówi, czy na stosie nie została wywołana funkcja rstack_delete.
  bool alive;
  // To pole jest flagą pomocniczą wykorzystywaną w wielu funkcjach.
  bool visited;
  // W tym polu trzymany jest wskaźnik do najwyższego węzła stosu.
  rnode_t* top;
} rstack_t;

// Ta struktura przechowuje stan odśmiecacza pamięci.
typedef struct {
  // Jeżeli długością stosu nazwiemy jego liczbę elementów zwiększoną o 1,
  // to pole to przechowuje sumę długości stosów trzymanych w pamięci.
  size_t current_object_count;
  // To pole przechowuje sumę długości stosów trzymanych w pamięci tuż po
  // ostatnim przejściu odśmiecacza pamięci. Jeżeli takie przejście się nie
  // odbyło, to wartość ta jest ustawiona na 0.
  size_t starting_object_count;
  // To pole przechowuje liczbę stosów trzymanych w pamięci, których flaga
  // alive jest ustawiona na true.
  size_t active_stacks;
  // To pole reprezentuje listę wszystkich stosów trzymanych w pamięci.
  rnode_t* current_list;
} garbage_collector_t;

// Ta funkcja zwraca wskaźnik na globalny stan odśmiecacza pamięci.
garbage_collector_t* gc() {
  static garbage_collector_t instance;
  return &instance;
}

// Ta funkcja usuwa w pełni stos z pamięci.
static void rstack_free(rstack_t* rs) {
  rnode_t* node = rs->top;

  while (node) {
    rnode_t* next = node->next;
    free(node);
    node = next;
  }

  free(rs);
}

// Ta funkcja zaznacza flagi visited we wszystkich stosach na false.
static void rstack_unvisit() {
  for (rnode_t* node = gc()->current_list; node; node = node->next) {
    node->rs->visited = false;
  }
}

// Ta funkcja przeszukuje graf stosów zaczynając od node->rs.
// Zaznacza flagi visited w odwiedzonych stosach na true.
// Zakładamy, że przekazany węzeł przechowuje stos.
static void traverse_graph(rnode_t* node) {
  if (node->rs->visited) {
    return;
  }

  node->rs->visited = true;

  for (rnode_t* son = node->rs->top; son; son = son->next) {
    if (son->holds_a_stack) {
      traverse_graph(son);
    }
  }
}

// Ta funkcja uruchamia odśmiecacz pamięci.
static void run_gc() {
  // Sprawdzamy, czy spełniony jest warunek zapewniający
  // poprawną amortyzację czasu działania odśmiecacza.
  if (gc()->active_stacks != 0 &&
      gc()->current_object_count <= 2 * gc()->starting_object_count) {
    return;
  }

  rstack_unvisit();

  // Zaznaczamy stosy, które nie powinny zostać usunięte z pamięci.
  for (rnode_t* node = gc()->current_list; node; node = node->next) {
    if (node->rs->alive) {
      traverse_graph(node);
    }
  }

  rnode_t* node = gc()->current_list;
  rnode_t* prev = nullptr;
  // Usuwamy niepotrzebne stosy z pamięci.
  while (node) {
    if (!node->rs->visited) {
      rstack_free(node->rs);

      rnode_t* next_node = node->next;

      if (prev) {
        prev->next = next_node;
      }
      else {
        gc()->current_list = next_node;
      }

      free(node);
      node = next_node;
    }
    else {
      prev = node;
      node = node->next;
    }
  }

  // Obliczamy nowe wartości stanu odśmiecacza pamięci.
  rstack_unvisit();
  
  gc()->starting_object_count = 0;

  for (rnode_t* node = gc()->current_list; node; node = node->next) {
    gc()->starting_object_count++;

    for (rnode_t* son = node->rs->top; son; son = son->next) {
      gc()->starting_object_count++;
    }
  }

  gc()->current_object_count = gc()->starting_object_count;
}

rstack_t* rstack_new() {
  rstack_t* rs = malloc(sizeof(rstack_t));

  if (rs == nullptr) {
    return nullptr;
  }

  rs->alive = true;
  rs->visited = false;
  rs->top = nullptr;

  // Dodajemy stos do globalnej listy.
  rnode_t* new_gc_top = malloc(sizeof(rnode_t));

  if (!new_gc_top) {
    free(rs);
    return nullptr;
  }

  // Aktualizujemy listę globalną oraz stan odśmiecacza pamięci.
  new_gc_top->holds_a_stack = true;
  new_gc_top->next = gc()->current_list;
  new_gc_top->rs = rs;

  gc()->current_list = new_gc_top;
  gc()->active_stacks++;
  gc()->current_object_count++;

  run_gc();

  return rs;
}

void rstack_delete(rstack_t* rs) {
  if (!rs) {
    return;
  }

  rs->alive = false;

  gc()->active_stacks--;

  run_gc();
}

int rstack_push_value(rstack_t* rs, uint64_t value) {
  if (!rs) {
    errno = EINVAL;
    return -1;
  }

  // Tworzymy nowy wierzchołek stosu.
  rnode_t* new_top = malloc(sizeof(rnode_t));

  if (!new_top) {
    return -1;
  }

  new_top->holds_a_stack = false;
  new_top->value = value;
  new_top->next = rs->top;
  rs->top = new_top;

  gc()->current_object_count++;

  run_gc();

  return 0;
}

int rstack_push_rstack(rstack_t* rs1, rstack_t* rs2) {
  if (!rs1 || !rs2) {
    errno = EINVAL;
    return -1;
  }

  // Tworzymy nowy wierzchołek stosu.
  rnode_t* new_top = malloc(sizeof(rnode_t));

  if (!new_top) {
    return -1;
  }

  new_top->holds_a_stack = true;
  new_top->rs = rs2;
  new_top->next = rs1->top;
  rs1->top = new_top;

  gc()->current_object_count++;

  run_gc();

  return 0;
}

void rstack_pop(rstack_t* rs) {
  if (!rs || !rs->top) {
    return;
  }

  // Ustawiamy nowy wierzchołek stosu oraz usuwamy poprzedni.
  rnode_t* new_top = rs->top->next;

  free(rs->top);
  rs->top = new_top;

  gc()->current_object_count--;
}

// Ta funkcja rekurencyjnie sprawdza, czy dany stos jest pusty.
// Zakładamy, że argument nie jest równy nullptr.
static bool rstack_empty_helper(rstack_t* rs) {
  if (rs->visited) {
    return true;
  }

  rs->visited = true;

  // Przechodzimy po wszystkich elementach stosu,
  // sprawdzamy, czy każdy z nich jest pusty.
  for (rnode_t* node = rs->top; node; node = node->next) {
    if (!node->holds_a_stack) {
      return false;
    }
    else {
      if (!rstack_empty_helper(node->rs)) {
        return false;
      }
    }
  }

  return true;
}

bool rstack_empty(rstack_t* rs) {
  if (!rs) {
    return true;
  }

  rstack_unvisit();

  return rstack_empty_helper(rs);
}

// Właściwa funkcja szukająca liczby najbliżej wierzchołka stosu.
static result_t rstack_front_helper(rstack_t* rs) {
  result_t result = {.flag = false, .value = 0};

  // Jeżeli natrafiliśmy na odwiedzony wierzchołek, to znaczy,
  // że znaleźliśmy cykl. Zwracamy wtedy flagę ustawioną na false.
  if (!rs || rs->visited || !(rs->top)) {
    return result;
  }

  rs->visited = true;

  // Szukamy wyniku przechodząc po kolei po elementach stosu
  for (rnode_t* node = rs->top; node; node = node->next) {
    // Jeżeli natrafiliśmy na liczbę, to jest ona wynikiem.
    if (!node->holds_a_stack) {
      result.flag = true;
      result.value = node->value;

      return result;
    }
    // Jeżeli natrafiliśmy na stos, który posiada liczbę najbliżej
    // wierzchołka stosu, to jest ona najbliżej wierzchołka wyjściowego stosu.
    else {
      result = rstack_front_helper(node->rs);

      if (result.flag) {
        return result;
      }
    }
  }

  return result;
}

result_t rstack_front(rstack_t* rs) {
  rstack_unvisit();

  result_t result = rstack_front_helper(rs);

  return result;
}

// Ta funkcja sprawdza, czy plik składa się z cyfr oraz białych znaków.
static bool is_a_correct_file(char const *path) {
  if (!path) {
    errno = EINVAL;
    return false;
  }

  FILE* file = fopen(path, "r");

  if (!file) {
    return false;
  }

  int cur_char;
  while ((cur_char = fgetc(file)) != EOF) {
    if (!isdigit(cur_char) && !isspace(cur_char)) {
      errno = EINVAL;
      fclose(file);

      return false;
    }
  }

  if (ferror(file)) {
    fclose(file);

    return false;
  }

  if (fclose(file) != 0) {
    return false;
  }

  return true;
}

rstack_t* rstack_read(char const *path) {
  if (!is_a_correct_file(path)) {
    return nullptr;
  }

  FILE* file = fopen(path, "r");

  if (!file) {
    return nullptr;
  }

  rstack_t* result = rstack_new();

  if (!result) {
    fclose(file);

    return nullptr;
  }

  // Czytamy po kolei liczby do odpowiednio dużego buforu.
  // Jeżeli liczba ma zera wiodące lub nie mieści się w uint64_t,
  // to uznajemy plik za niepoprawny.
  char buffer[32];
  while (fscanf(file, "%31s", buffer) == 1) {
    errno = 0;

    // Ta zmienna będzie trzymała pozycję, na której
    // funkcja strtoumax przestała czytać bufor.
    char* endptr;
    uintmax_t value = strtoumax(buffer, &endptr, 10);

    if (errno != 0 || buffer == endptr || *endptr != '\0' ||
        (buffer[0] == '0' && isdigit(buffer[1])) || value > UINT64_MAX) {
      if (value > UINT64_MAX) {
        errno = ERANGE;
      }
      else {
        errno = EINVAL;
      }
        
      rstack_delete(result);
      fclose(file);
      
      return nullptr;
    }

    if (rstack_push_value(result, (uint64_t)value) != 0) {
      rstack_delete(result);
      fclose(file);

      return nullptr;
    }
  }

  if (ferror(file)) {
    rstack_delete(result);
    fclose(file);

    return nullptr;
  }

  if (fclose(file) != 0) {
    rstack_delete(result);
    return nullptr;
  }

  return result;
}

// Funkcje rstack_write_helper, rnode_write zależą od
// siebie nawzajem, dlatego pierwszą z nich deklarujemy tutaj
static int rstack_write_helper(FILE* file, rstack_t* rs);

// Ta funkcja rekurencyjnie wypisuje zawartość listy węzłów do pliku.
// Zwraca  0, jeżeli wypisywanie udało się oraz nie natrafiliśmy na cykl.
// Zwraca -1, jeżeli wystąpił błąd.
// Zwraca -2, jeżeli wypisywanie udało się, ale natrafiliśmy na cykl.
static int rnode_write(FILE* file, rnode_t* node) {
  if (!node) {
    return 0;
  }

  // Przechodzimy rekurencyjnie do kolejnego węzła.
  int write_result = rnode_write(file, node->next);
  if (write_result < 0) {
    return write_result;
  }

  // Wypisujemy zawartość aktualnego węzłą.
  if (node->holds_a_stack) {
    write_result = rstack_write_helper(file, node->rs);
    if (write_result < 0) {
      return write_result;
    }
  }
  else {
    if (fprintf(file, "%" PRIu64 "\n", node->value) < 0) {
      return -1;
    }
  }

  return 0;
}

// Ta funkcja rekurencyjnie wypisuje zawartość stosu do pliku.
// Zwraca  0, jeżeli wypisywanie udało się oraz nie natrafiliśmy na cykl.
// Zwraca -1, jeżeli wystąpił błąd.
// Zwraca -2, jeżeli wypisywanie udało się, ale natrafiliśmy na cykl.
static int rstack_write_helper(FILE* file, rstack_t* rs) {
  // Jeżeli stos został już odwiedzony, to natrafiliśmy na cykl.
  if (rs->visited) {
    return -2;
  }

  rs->visited = true;

  // Wypisujemy rekurencyjnie zawartość stosu.
  int write_result = rnode_write(file, rs->top);
  if (write_result < 0) {
    return write_result;
  }

  // Skoro wychodzimy ze stosu, to ponowne natrafienie na niego następnym
  // razem nie będzie oznaczało wykrycia cyklu. W takim razie, ustawiamy
  // flagę visited na false.
  rs->visited = false;

  return 0;
}

int rstack_write(char const* path, rstack_t* rs) {
  rstack_unvisit();

  if (!path || !rs) {
    errno = EINVAL;
    return -1;
  }

  FILE* file = fopen(path, "w");

  if (!file) {
    return -1;
  }

  if (rstack_write_helper(file, rs) == -1 || ferror(file)) {
    fclose(file);
    return -1;
  }

  if (fclose(file) != 0) {
    return -1;
  }

  return 0;
}