#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "jsonpath.h"

#ifdef DEBUG
# define DBG(x) x
#else
# define DBG(x)
#endif

#define CHARTYPE(what, c) (is ## what ((int)((unsigned char)(c))))
#define PROPCHAR(c) ((c) != '[' && (c) != ']' && (c) != '.' && (c) != '*' && (c) != '\'' && (c) != '$')

typedef enum {
    ROOT,
    CHILD_NAME,
    CHILD_INDEX,
    DEEP_SCAN,
    WILDCARD_NAME,
    WILDCARD_INDEX,
    INDICES_SET,
    INDICES_SLICE
} jsonpath_node_enum_t;

typedef struct jsonpath_node {
    struct jsonpath_node *next;
    jsonpath_node_enum_t type;
    union {
        char *child_name;
        int child_index;
        struct {
            int *indices;
            int length;
        } indices_set;
        struct {
            int start;
            int end;
        } indices_slice;
    } data;
} jsonpath_node_t;

jsonpath_node_t *jsonpath_node_new(jsonpath_node_enum_t type) {
    jsonpath_node_t *node = (jsonpath_node_t *) Tcl_Alloc(sizeof(jsonpath_node_t));
    node->type = type;
    node->next = NULL;
    return node;
}

char *jsonpath_strndup(const char *s, size_t n) {
    if (s == NULL) {
        return NULL;
    }
    size_t l = strnlen(s, n);
    char *result = (char *) Tcl_Alloc(l + 1);
    if (result == NULL) {
        return NULL;
    }
    memcpy(result, s, l);
    result[l] = '\0';
    return result;
}

jsonpath_node_t *reverse_linked_list(jsonpath_node_t *head) {
    jsonpath_node_t *prev = NULL;
    jsonpath_node_t *curr = head;
    jsonpath_node_t *next = NULL;
    while (curr != NULL) {
        next = curr->next;
        curr->next = prev;
        prev = curr;
        curr = next;
    }
    return prev;
}

void jsonpath_free(jsonpath_node_t *node) {
    jsonpath_node_t *curr = node;
    while (curr != NULL) {
        jsonpath_node_t *next = curr->next;
        switch (curr->type) {
            case CHILD_NAME:
                Tcl_Free(curr->data.child_name);
                break;
            case INDICES_SET:
                Tcl_Free((char *) curr->data.indices_set.indices);
                break;
            default:
                break;
        }
        Tcl_Free((char *)curr);
        curr = next;
    }
}

void jsonpath_free_list(jsonpath_node_t *node) {
    jsonpath_node_t *curr = node;
    while (curr != NULL) {
        jsonpath_node_t *next = curr->next;
        free(curr);
        curr = next;
    }
}

static void jsonpath_insert_node_to_list(jsonpath_node_t *node, jsonpath_node_t **nodes, int *nodes_length) {
    node->next = *nodes;
    *nodes = node;
    (*nodes_length)++;
}
static int jsonpath_parse(Tcl_Interp *interp, const char *jsonpath, int length, jsonpath_node_t **nodes) {

    DBG(fprintf(stderr, "jsonpath_parse: %.*s\n", length, jsonpath));

    // "nodes" is a list of jsonpath_node_t pointers
    // "root" is the root node of the jsonpath
    // "curr" is the current position in the jsonpath
    // "end" is the end of the jsonpath

    const char *curr = jsonpath;
    const char *end = jsonpath + length;
    jsonpath_node_t *root = NULL;
    int nodes_length = 0;

    while (curr < end) {
        switch (curr[0]) {
            case '$':
                if (root != NULL) {
                    Tcl_SetObjResult(interp, Tcl_NewStringObj("Invalid JSONPath: multiple root nodes", -1));
                    return TCL_ERROR;
                }
                if (!(curr + 1 == end || curr[1] == '.' || curr[1] == '[')) {
                    Tcl_SetObjResult(interp, Tcl_NewStringObj("Invalid JSONPath: root node must be followed by '.' or '['", -1));
                    return TCL_ERROR;
                }
                root = jsonpath_node_new(ROOT);
                root->next = NULL;
                jsonpath_insert_node_to_list(root, nodes, &nodes_length);
                curr++;
                break;
            case '.':
                if (root == NULL) {
                    Tcl_SetObjResult(interp, Tcl_NewStringObj("Invalid JSONPath: '.' must be preceded by '$'", -1));
                    return TCL_ERROR;
                }
                if (curr + 1 == end) {
                    jsonpath_free_list(*nodes);
                    Tcl_SetObjResult(interp, Tcl_NewStringObj("Invalid JSONPath: '.' must be followed by a child name or wildcard or '.'", -1));
                    return TCL_ERROR;
                }
                if (curr[1] == '.') {
                    if (curr + 2 == end || curr[2] == '.') {
                        jsonpath_free_list(*nodes);
                        Tcl_SetObjResult(interp, Tcl_NewStringObj("Invalid JSONPath: '..' must be followed by a child name", -1));
                        return TCL_ERROR;
                    }
                    jsonpath_node_t *node = jsonpath_node_new(DEEP_SCAN);
                    jsonpath_insert_node_to_list(node, nodes, &nodes_length);

                    // parse child name
                    const char *p = curr + 2;
                    while (p < end && PROPCHAR(p[0])) {
                        p++;
                    }
                    if (p == curr + 2) {
                        jsonpath_free_list(*nodes);
                        Tcl_SetObjResult(interp, Tcl_NewStringObj("Invalid JSONPath: '..' must be followed by a child name", -1));
                        return TCL_ERROR;
                    }
                    DBG(fprintf(stderr, "child name: %.*s\n", (int) (p - curr - 2), curr + 2));
                    node = jsonpath_node_new(CHILD_NAME);
                    node->data.child_name = jsonpath_strndup(curr + 2, p - curr - 2);
                    jsonpath_insert_node_to_list(node, nodes, &nodes_length);
                    curr = p;
                } else if (curr[1] == '*') {
                    curr += 2;
                    jsonpath_node_t *node = jsonpath_node_new(WILDCARD_NAME);
                    jsonpath_insert_node_to_list(node, nodes, &nodes_length);
                } else {
                    const char *p = curr + 1;
                    while (p < end && PROPCHAR(p[0])) {
                        p++;
                    }
                    if (p == curr + 1) {
                        jsonpath_free_list(*nodes);
                        Tcl_SetObjResult(interp, Tcl_NewStringObj("Invalid JSONPath: '.' must be followed by a child name or wildcard", -1));
                        return TCL_ERROR;
                    }
                    DBG(fprintf(stderr, "child name: %.*s\n", (int) (p - curr - 1), curr + 1));
                    jsonpath_node_t *node = jsonpath_node_new(CHILD_NAME);

                    node->data.child_name = jsonpath_strndup(curr + 1, p - curr - 1);

                    jsonpath_insert_node_to_list(node, nodes, &nodes_length);
                    curr = p;
                }
                break;
            case '[':
                if (root == NULL) {
                    Tcl_SetObjResult(interp, Tcl_NewStringObj("Invalid JSONPath: '[' must be preceded by '$'", -1));
                    return TCL_ERROR;
                }
                if (curr + 1 == end) {
                    jsonpath_free_list(*nodes);
                    Tcl_SetObjResult(interp, Tcl_NewStringObj("Invalid JSONPath: '[' must be followed by a child index, a wildcard or a name in single quotes", -1));
                    return TCL_ERROR;
                }
                // if "curr" is equal to "[*]" then it's a "WILDCARD_INDEX"
                if (curr[1] == '*' && curr[2] == ']') {
                    DBG(fprintf(stderr, "wildcard index\n"));
                    jsonpath_node_t *node = jsonpath_node_new(WILDCARD_INDEX);
                    jsonpath_insert_node_to_list(node, nodes, &nodes_length);
                    curr += 3;
                } else if (curr[1] == '\'') {
                    // if "curr" is equal to "['*']" then it's a "WILDCARD_NAME"
                    if (curr[2] == '*' && curr[3] == '\'' && curr[4] == ']') {
                        DBG(fprintf(stderr, "wildcard name\n"));
                        jsonpath_node_t *node = jsonpath_node_new(WILDCARD_NAME);
                        jsonpath_insert_node_to_list(node, nodes, &nodes_length);
                        curr += 5;
                    } else {
                        // if "curr" is equal to "['name']" then it's a "CHILD_NAME"
                        const char *p = curr + 2;
                        while (p < end && p[0] != '\'' && PROPCHAR(p[0])) {
                            p++;
                        }
                        if (p == end) {
                            jsonpath_free_list(*nodes);
                            Tcl_SetObjResult(interp, Tcl_NewStringObj(
                                    "Invalid JSONPath: \"['\" must be followed by a wildcard or a name in single quotes",
                                    -1));
                            return TCL_ERROR;
                        }
                        if (p + 1 == end || p[1] != ']') {
                            jsonpath_free_list(*nodes);
                            Tcl_SetObjResult(interp, Tcl_NewStringObj("Invalid JSONPath: ']' expected", -1));
                            return TCL_ERROR;
                        }

                        DBG(fprintf(stderr, "child name: %.*s\n", (int) (p - curr - 2), curr + 2));

                        jsonpath_node_t *node = jsonpath_node_new(CHILD_NAME);
                        node->data.child_name = jsonpath_strndup(curr + 2, p - curr - 2);

                        DBG(fprintf(stderr, "child name: %s\n", node->data.child_name));
                        jsonpath_insert_node_to_list(node, nodes, &nodes_length);
                        curr = p + 2;
                    }
                } else {
                    const char *p = curr + 1;
                    // five cases:
                    // (a) "curr" is equal to "[123]" then it's a "CHILD_INDEX"
                    // (b) "curr" is equal to "[123,456]" then it's an "INDICES_SET"
                    // (c) "curr" is equal to "[123:456]" then it's an "INDICES_SLICE",
                    //     possibly with a "step" in between the "start" and the "end" of the slice
                    // (d) "curr" is equal to "[:456]" then it's an "INDICES_SLICE" with an empty "start"

                    int sign = 1;

                    if (p[0] == '-') {
                        sign = -1;
                        p++;
                    }

                    if (p[0] == ':') {
                        // TODO: get sign here
                        // case (d) slice with an empty start
                        int slice_end = strtoll(p + 1, (char **) &p, 10);
                        if (p[0] != ']') {
                            jsonpath_free_list(*nodes);
                            Tcl_SetObjResult(interp, Tcl_NewStringObj("Invalid JSONPath: ']' expected", -1));
                            return TCL_ERROR;
                        }
                        jsonpath_node_t *node = jsonpath_node_new(INDICES_SLICE);
                        node->data.indices_slice.start = 0;
                        node->data.indices_slice.end = slice_end;
                        jsonpath_insert_node_to_list(node, nodes, &nodes_length);
                        curr = p + 1;
                        break;
                    }

                    int index = strtoll(p, (char **) &p, 10) * sign;
                    DBG(fprintf(stderr, "index: %d\n", index));

                    if (p[0] == ',') {
                        // case (b) set of indices
                        int k = 16;
                        int *indices = (int *) Tcl_Alloc(sizeof(int) * k);
                        int indices_length = 0;
                        indices[indices_length++] = index;
                        while (p[0] != ']') {
                            p++;
                            if (p[0] == '-') {
                                sign = -1;
                                p++;
                            } else {
                                sign = 1;
                            }
                            indices[indices_length++] = strtoll(p, (char **) &p, 10);
                            if (p[0] != ',' && p[0] != ']') {
                                free(indices);
                                jsonpath_free_list(*nodes);
                                Tcl_SetObjResult(interp, Tcl_NewStringObj("Invalid JSONPath: ',' or ']' expected", -1));
                                return TCL_ERROR;
                            }
                            if (indices_length == k) {
                                k *= 2;
                                indices = realloc(indices, sizeof(int) * k);
                            }
                        }
                        jsonpath_node_t *node = jsonpath_node_new(INDICES_SET);
                        node->data.indices_set.indices = indices;
                        node->data.indices_set.length = indices_length;
                        jsonpath_insert_node_to_list(node, nodes, &nodes_length);
                        curr = p + 1;
                        break;
                    } else if (p[0] == ':') {
                        // case (c) slice
                        int slice_start = index;
                        int slice_end = 0;
                        p++;
                        if (p[0] == '-') {
                            sign = -1;
                            p++;
                        } else {
                            sign = 1;
                        }
                        slice_end = strtoll(p, (char **) &p, 10) * sign;
                        if (p[0] != ']') {
                            jsonpath_free_list(*nodes);
                            Tcl_SetObjResult(interp, Tcl_NewStringObj("Invalid JSONPath: ']' expected", -1));
                            return TCL_ERROR;
                        }
                        jsonpath_node_t *node = jsonpath_node_new(INDICES_SLICE);
                        node->data.indices_slice.start = slice_start;
                        node->data.indices_slice.end = slice_end;
                        jsonpath_insert_node_to_list(node, nodes, &nodes_length);
                        curr = p + 1;
                        break;
                    } else if (p[0] == ']') {
                        // case (a) single index
                        jsonpath_node_t *node = jsonpath_node_new(CHILD_INDEX);
                        node->data.child_index = index;
                        jsonpath_insert_node_to_list(node, nodes, &nodes_length);
                        curr = p + 1;
                        break;
                    } else {
                        jsonpath_free_list(*nodes);
                        Tcl_SetObjResult(interp, Tcl_NewStringObj("Invalid JSONPath: ']' expected", -1));
                        return TCL_ERROR;
                    }
                }
                break;
            default:
                jsonpath_free_list(*nodes);
                Tcl_SetObjResult(interp, Tcl_NewStringObj("Invalid JSONPath: invalid syntax", -1));
                return TCL_ERROR;
        }
    }
    *nodes = reverse_linked_list(*nodes);
    DBG(fprintf(stderr, "done\n"));
    return TCL_OK;
}

static int add_item_to_result(jsonpath_result_t *result, cJSON *item) {
    result->items[result->items_length++] = item;
    if (result->items_length > result->k) {
        result->k *= 2;
        result->items = realloc(result->items, sizeof(cJSON *) * result->k);
        if (result->items == NULL) {
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

static int jsonpath_eval(Tcl_Interp *interp, jsonpath_node_t *node, cJSON *root, jsonpath_result_t *result) {
    if (node == NULL) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("Invalid JSONPath: node is NULL", -1));
        return TCL_ERROR;
    }
    fprintf(stderr, "jsonpath_eval, type: %d\n", node->type);
    cJSON *item;
    switch (node->type) {
        case ROOT:
            DBG(fprintf(stderr, "eval,root: %p\n", root));
            if (node->next != NULL) {
                if (TCL_OK != jsonpath_eval(interp, node->next, root, result)) {
                    return TCL_ERROR;
                }
            } else {
                if (TCL_OK != add_item_to_result(result, root)) {
                    Tcl_SetObjResult(interp, Tcl_NewStringObj("Invalid JSONPath: failed to add item to result", -1));
                    return TCL_ERROR;
                }
                return TCL_OK;
            }
            break;
        case CHILD_NAME:
            DBG(fprintf(stderr, "eval,child_name: %d %p %p\n", node->type, node->next, node->data.child_name));
            item = cJSON_GetObjectItemCaseSensitive(root, node->data.child_name);
            if (item == NULL) {
                return TCL_OK;
            }
            if (node->next == NULL) {
                if (TCL_OK != add_item_to_result(result, item)) {
                    DBG(fprintf(stderr, "failed to add item to result\n"));
                    Tcl_SetObjResult(interp, Tcl_NewStringObj("Invalid JSONPath: failed to add item to result", -1));
                    return TCL_ERROR;
                }
                DBG(fprintf(stderr, "added item to result\n"));
            } else {
                if (TCL_OK != jsonpath_eval(interp, node->next, item, result)) {
                    return TCL_ERROR;
                }
            }
            break;
        case CHILD_INDEX:
            if (!cJSON_IsArray(root)) {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("Invalid JSONPath: CHILD_INDEX - not an array", -1));
                return TCL_ERROR;
            }
            if (node->data.child_index < 0 || node->data.child_index >= cJSON_GetArraySize(root)) {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("Invalid JSONPath: child index out of bounds", -1));
                return TCL_ERROR;
            }
            item = cJSON_GetArrayItem(root, node->data.child_index);
            if (node->next == NULL) {
                if (TCL_OK != add_item_to_result(result, item)) {
                    Tcl_SetObjResult(interp, Tcl_NewStringObj("Invalid JSONPath: failed to add item to result", -1));
                    return TCL_ERROR;
                }
            } else {
                if (TCL_OK != jsonpath_eval(interp, node->next, item, result)) {
                    return TCL_ERROR;
                }
            }
            break;
        case DEEP_SCAN:
            switch ((root->type) & 0xFF) {
                case cJSON_Object:
                    item = root->child;
                    while (item != NULL) {
                        if (node->next->type == CHILD_NAME && strcmp(node->next->data.child_name, item->string) == 0) {
                            DBG(fprintf(stderr, "eval,deep scan object,entering: %s\n", item->string));
                            if (TCL_OK != jsonpath_eval(interp, node->next, root, result)) {
                                return TCL_ERROR;
                            }
                            DBG(fprintf(stderr, "eval,deep scan object,leaving: %s\n", item->string));
                        } else {
                            DBG(fprintf(stderr, "eval,deep scan object (in): %s %s\n", node->next->data.child_name, item->string));
                            if (TCL_OK != jsonpath_eval(interp, node, item, result)) {
                                return TCL_ERROR;
                            }
                            DBG(fprintf(stderr, "eval,deep scan object (out): %s %s\n", node->next->data.child_name, item->string));
                        }
                        item = item->next;
                    }
                    break;
                case cJSON_Array:
                    item = root->child;
                    for (int i = 0; item != NULL; item = item->next, i++) {
                        if (node->next->type == CHILD_INDEX && node->next->data.child_index == i) {
                            DBG(fprintf(stderr, "eval,deep scan array,entering: %d\n", i));
                            if (TCL_OK != jsonpath_eval(interp, node->next, root, result)) {
                                return TCL_ERROR;
                            }
                        } else {
                            DBG(fprintf(stderr, "eval,deep scan array: %d\n", i));
                            if (TCL_OK != jsonpath_eval(interp, node, item, result)) {
                                return TCL_ERROR;
                            }
                        }
                    }
                    break;
                default:
                    break;
            }
            return TCL_OK;
        case WILDCARD_NAME:
            DBG(fprintf(stderr, "eval,wildcard_name,next: %p\n", node->next));
            if (cJSON_IsObject(root)) {
                item = root->child;
                while (item != NULL) {
                    DBG(fprintf(stderr, "%s\n", cJSON_Print(item)));
                    if (node->next != NULL) {
                        if (TCL_OK != jsonpath_eval(interp, node->next, item, result)) {
                            return TCL_ERROR;
                        }
                    } else {
                        if (TCL_OK != add_item_to_result(result, item)) {
                            Tcl_SetObjResult(interp,
                                             Tcl_NewStringObj("Invalid JSONPath: failed to add item to result", -1));
                            return TCL_ERROR;
                        }
                    }
                    item = item->next;
                }
            } else {
                // TODO: cases trailing '*' in a path like "$.foo.bar.*" and "$.foo.bar.*.baz" where bar is an array
//                if (TCL_OK != add_item_to_result(result, root)) {
//                    Tcl_SetObjResult(interp,
//                                     Tcl_NewStringObj("Invalid JSONPath: failed to add item to result", -1));
//                    return TCL_ERROR;
//                }
            }
            return TCL_OK;
        case WILDCARD_INDEX:
            if (cJSON_IsArray(root)) {
                item = root->child;
                while (item != NULL) {
                    if (node->next != NULL) {
                        if (TCL_OK != jsonpath_eval(interp, node->next, item, result)) {
                            return TCL_ERROR;
                        }
                    } else {
                        if (TCL_OK != add_item_to_result(result, item)) {
                            Tcl_SetObjResult(interp,
                                             Tcl_NewStringObj("Invalid JSONPath: failed to add item to result", -1));
                            return TCL_ERROR;
                        }
                    }
                    item = item->next;
                }
            } else {
                // TODO: cases trailing '*' in a path like "$.foo.bar[*]" and "$.foo.bar[*].baz" where bar is an object
//                if (TCL_OK != add_item_to_result(result, root)) {
//                    Tcl_SetObjResult(interp,
//                                     Tcl_NewStringObj("Invalid JSONPath: failed to add item to result", -1));
//                    return TCL_ERROR;
//                }
            }
            return TCL_OK;
        case INDICES_SET:
            if (!cJSON_IsArray(root)) {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("Invalid JSONPath: not an array", -1));
                return TCL_ERROR;
            }
            for (int i = 0; i < node->data.indices_set.length; i++) {
                int index = node->data.indices_set.indices[i];
                item = cJSON_GetArrayItem(root, index);
                if (node->next != NULL) {
                    if (TCL_OK != jsonpath_eval(interp, node->next, item, result)) {
                        return TCL_ERROR;
                    }
                } else {
                    if (TCL_OK != add_item_to_result(result, item)) {
                        Tcl_SetObjResult(interp,
                                         Tcl_NewStringObj("Invalid JSONPath: failed to add item to result", -1));
                        return TCL_ERROR;
                    }
                }
            }
            return TCL_OK;
        case INDICES_SLICE:
            if (!cJSON_IsArray(root)) {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("Invalid JSONPath: not an array", -1));
                return TCL_ERROR;
            }
            int start = node->data.indices_slice.start;
            int end = node->data.indices_slice.end;
            int length = cJSON_GetArraySize(root);
            if (start < 0) {
                start = length + start;
            }
            if (end < 0) {
                end = length + end;
            }
            if (end == 0) {
                end = length;
            }
            if (start < 0 || start >= length) {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("Invalid JSONPath: slice start out of bounds", -1));
                return TCL_ERROR;
            }
            if (end < 0 || end > length) {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("Invalid JSONPath: slice end out of bounds", -1));
                return TCL_ERROR;
            }
            item = cJSON_GetArrayItem(root, start);
            while (item != NULL && start < end) {
                if (node->next != NULL) {
                    if (TCL_OK != jsonpath_eval(interp, node->next, item, result)) {
                        return TCL_ERROR;
                    }
                } else {
                    if (TCL_OK != add_item_to_result(result, item)) {
                        Tcl_SetObjResult(interp,
                                         Tcl_NewStringObj("Invalid JSONPath: failed to add item to result", -1));
                        return TCL_ERROR;
                    }
                }
                item = item->next;
                start++;
            }
            return TCL_OK;
        default:
            return TCL_OK;
    }
    return TCL_OK;
}

int jsonpath_match(Tcl_Interp *interp, const char *jsonpath, int length, cJSON *root, jsonpath_result_t *result) {
    jsonpath_node_t *nodes = NULL;
    if (TCL_OK != jsonpath_parse(interp, jsonpath, length, &nodes)) {
        DBG(fprintf(stderr, "failed nodes: %p\n", nodes));
        return TCL_ERROR;
    }
    DBG(fprintf(stderr, "nodes: %p\n", nodes));
    if (TCL_OK != jsonpath_eval(interp, nodes, root, result)) {
        jsonpath_free(nodes);
        return TCL_ERROR;
    }
    jsonpath_free(nodes);
    return TCL_OK;
}