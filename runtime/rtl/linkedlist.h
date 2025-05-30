// #pragma once

// #define list_node_t(T) list_node_of_ ## T ## _t
// #define list_t(T) list_of_ ## T ## _t

// #define _LINKED_LIST_TYPES(T)                                       \
//     typedef struct list_node_of_ ## T {                             \
//         list_node_t(T)* next;                                       \
//         T value;                                                    \
//     } list_node_t(T);                                               \
//                                                                     \
//     typedef struct list_of_ ## T {                                  \
//         list_node_t(T)* first;                                      \
//         list_node_t(T)* last;                                       \
//     } list_t(T);                                                    \

// #define _LINKED_LIST_IMPL(T)                                        \
//     void rtl_list_append_ ## T (list_t(T) list, T value) {          \
//         rtl_list_append_unsafe(list.last);                          \
//     }                                                               \

// // Defines both the linked list (`list_t(T)`) and linked list node (`list_node_t(T)`) types.
// #define DEFINE_LINKED_LIST(T)                                       \
//     _LINKED_LIST_TYPES(T)                                           \
//     _LINKED_LIST_IMPL(T)                                            \

// // void rtl_list_append_unsafe()

// DEFINE_LINKED_LIST(int)