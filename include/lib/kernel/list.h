#ifndef __LIB_KERNEL_LIST_H
#define __LIB_KERNEL_LIST_H

/* Doubly linked list.
 *
 * This implementation of a doubly linked list does not require
 * use of dynamically allocated memory.  Instead, each structure
 * that is a potential list element must embed a struct list_elem
 * member.  All of the list functions operate on these `struct
 * list_elem's.  The list_entry macro allows conversion from a
 * struct list_elem back to a structure object that contains it.

 * For example, suppose there is a needed for a list of `struct
 * foo'.  `struct foo' should contain a `struct list_elem'
 * member, like so:

 * struct foo {
 *   struct list_elem elem;
 *   int bar;
 *   ...other members...
 * };

 * Then a list of `struct foo' can be be declared and initialized
 * like so:

 * struct list foo_list;

 * list_init (&foo_list);

 * Iteration is a typical situation where it is necessary to
 * convert from a struct list_elem back to its enclosing
 * structure.  Here's an example using foo_list:

 * struct list_elem *e;

 * for (e = list_begin (&foo_list); e != list_end (&foo_list);
 * e = list_next (e)) {
 *   struct foo *f = list_entry (e, struct foo, elem);
 *   ...do something with f...
 * }

 * You can find real examples of list usage throughout the
 * source; for example, malloc.c, palloc.c, and thread.c in the
 * threads directory all use lists.

 * The interface for this list is inspired by the list<> template
 * in the C++ STL.  If you're familiar with list<>, you should
 * find this easy to use.  However, it should be emphasized that
 * these lists do *no* type checking and can't do much other
 * correctness checking.  If you screw up, it will bite you.

 * Glossary of list terms:

 * - "front": The first element in a list.  Undefined in an
 * empty list.  Returned by list_front().

 * - "back": The last element in a list.  Undefined in an empty
 * list.  Returned by list_back().

 * - "tail": The element figuratively just after the last
 * element of a list.  Well defined even in an empty list.
 * Returned by list_end().  Used as the end sentinel for an
 * iteration from front to back.

 * - "beginning": In a non-empty list, the front.  In an empty
 * list, the tail.  Returned by list_begin().  Used as the
 * starting point for an iteration from front to back.

 * - "head": The element figuratively just before the first
 * element of a list.  Well defined even in an empty list.
 * Returned by list_rend().  Used as the end sentinel for an
 * iteration from back to front.

 * - "reverse beginning": In a non-empty list, the back.  In an
 * empty list, the head.  Returned by list_rbegin().  Used as
 * the starting point for an iteration from back to front.
 *
 * - "interior element": An element that is not the head or
 * tail, that is, a real list element.  An empty list does
 * not have any interior elements.*/


/* 이중 연결 리스트.

이 이중 연결 리스트의 구현은 동적으로 할당된 메모리의 사용을 요구하지 않습니다.
대신, 잠재적인 리스트 요소인 각 구조체는 struct list_elem 멤버를 포함해야 합니다.
모든 리스트 함수는 이러한 'struct list_elem'에 대해 작동합니다.
list_entry 매크로를 사용하면
struct list_elem에서 해당하는 구조체 객체로 변환할 수 있습니다.

예를 들어, 'struct foo'의 리스트가 필요한 경우를 가정해 봅시다.
'struct foo'는 다음과 같이 'struct list_elem' 멤버를 포함해야 합니다:

struct foo {
struct list_elem elem;
int bar;
...다른 멤버들...
};

그런 다음 'struct foo'의 리스트는 다음과 같이 선언 및 초기화할 수 있습니다:

struct list foo_list;
list_init(&foo_list);

반복은 struct list_elem에서 enclosing structure로의 변환이 필요한 전형적인 상황입니다.
다음은 foo_list를 사용하는 예제입니다:

struct list_elem *e;
for (e = list_begin(&foo_list); e != list_end(&foo_list);
e = list_next(e)) {
struct foo *f = list_entry(e, struct foo, elem);
...f와 무언가를 수행합니다...
}

리스트 사용의 실제 예는 소스 전체에 있습니다.
예를 들어, malloc.c, palloc.c 및 thread.c는 모두 리스트를 사용합니다.

이 리스트의 인터페이스는 C++ STL의 list<> 템플릿에서 영감을 받았습니다.
list<>를 알고 있다면 쉽게 사용할 수 있을 것입니다. 그러나 강조해야 할 점은
이러한 리스트가 아무런 형식 검사를 수행하지 않으며 다른
정확성 검사도 거의 없습니다. 실수하면 심각한 문제가 발생할 수 있습니다.

리스트 용어 설명:

"front": 리스트의 첫 번째 요소.
빈 리스트에서는 정의되지 않습니다. list_front()에 의해 반환됩니다.

"back": 리스트의 마지막 요소.
빈 리스트에서는 정의되지 않습니다. list_back()에 의해 반환됩니다.

"tail": 리스트의 마지막 요소 다음에 있는 것처럼 비유적으로 설명됩니다.
빈 리스트에서도 잘 정의됩니다.
list_end()에 의해 반환됩니다. 앞에서 뒤로 반복하는 데 사용됩니다.

"beginning": 비어 있지 않은 리스트에서는 front입니다.
빈 리스트에서는 tail입니다. list_begin()에 의해 반환됩니다.
앞에서 뒤로 반복하는 데 시작점으로 사용됩니다.

"head": 리스트의 첫 번째 요소 앞에 있는 것처럼 비유적으로 설명됩니다.
빈 리스트에서도 잘 정의됩니다.
list_rend()에 의해 반환됩니다. 뒤에서 앞으로 반복하는 데 사용됩니다.

"reverse beginning": 비어 있지 않은 리스트에서는 back입니다.
빈 리스트에서는 head입니다. list_rbegin()에 의해 반환됩니다.
뒤에서 앞으로 반복하는 데 시작점으로 사용됩니다.

"interior element": 헤드나 테일이 아닌, 즉 실제 리스트 요소입니다.
빈 리스트에는 interior elements가 없습니다. */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* List element. */
struct list_elem {
	struct list_elem *prev;     /* Previous list element. */
	struct list_elem *next;     /* Next list element. */
};

/* List. */
struct list {
	struct list_elem head;      /* List head. */
	struct list_elem tail;      /* List tail. */
};

/* Converts pointer to list element LIST_ELEM into a pointer to
   the structure that LIST_ELEM is embedded inside.  Supply the
   name of the outer structure STRUCT and the member name MEMBER
   of the list element.  See the big comment at the top of the
   file for an example. */
#define list_entry(LIST_ELEM, STRUCT, MEMBER)           \
	((STRUCT *) ((uint8_t *) &(LIST_ELEM)->next     \
		- offsetof (STRUCT, MEMBER.next)))

void list_init (struct list *);

/* List traversal. */
struct list_elem *list_begin (struct list *);
struct list_elem *list_next (struct list_elem *);
struct list_elem *list_end (struct list *);

struct list_elem *list_rbegin (struct list *);
struct list_elem *list_prev (struct list_elem *);
struct list_elem *list_rend (struct list *);

struct list_elem *list_head (struct list *);
struct list_elem *list_tail (struct list *);
int is_in_list(struct list *list,struct list_elem *elem);
/* List insertion. */
void list_insert (struct list_elem *, struct list_elem *);
void list_splice (struct list_elem *before,
		struct list_elem *first, struct list_elem *last);
void list_push_front (struct list *, struct list_elem *);
void list_push_back (struct list *, struct list_elem *);

/* List removal. */
struct list_elem *list_remove (struct list_elem *);
struct list_elem *list_pop_front (struct list *);
struct list_elem *list_pop_back (struct list *);

/* List elements. */
struct list_elem *list_front (struct list *);
struct list_elem *list_back (struct list *);

/* List properties. */
size_t list_size (struct list *);
bool list_empty (struct list *);

/* Miscellaneous. */
void list_reverse (struct list *);

/* Compares the value of two list elements A and B, given
   auxiliary data AUX.  Returns true if A is less than B, or
   false if A is greater than or equal to B. */
typedef bool list_less_func (const struct list_elem *a,
                             const struct list_elem *b,
                             void *aux);

/* Operations on lists with ordered elements. */
void list_sort (struct list *,
                list_less_func *, void *aux);
void list_insert_ordered (struct list *, struct list_elem *,
                          list_less_func *, void *aux);
void list_unique (struct list *, struct list *duplicates,
                  list_less_func *, void *aux);

/* Max and min. */
struct list_elem *list_max (struct list *, list_less_func *, void *aux);
struct list_elem *list_min (struct list *, list_less_func *, void *aux);

#endif /* lib/kernel/list.h */
