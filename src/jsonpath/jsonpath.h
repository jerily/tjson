#ifndef TJSON_JSONPATH_H
#define TJSON_JSONPATH_H

#include <string.h>
#include <stdlib.h>
#include <tcl.h>
#include "../cJSON/cJSON.h"

typedef struct {
    int k;
    int items_length;
    cJSON **items;
} jsonpath_result_t;

int jsonpath_match(Tcl_Interp *interp, const char *jsonpath, int length, cJSON *root, jsonpath_result_t *result);

#endif //TJSON_JSONPATH_H
