/* Strict, bounded JSON validation for security-sensitive documents.
 * The normal tjson parser remains the owner of the resulting JSON tree.
 * SPDX-License-Identifier: MIT */
#ifndef TJSON_STRICT_JSON_H
#define TJSON_STRICT_JSON_H

typedef struct {
    const unsigned char *p, *end;
    int max_depth;
} tjson_strict_input;

static void tjson_StrictSpace(tjson_strict_input *in) {
    while (in->p < in->end && (*in->p == ' ' || *in->p == '\t' || *in->p == '\r' || *in->p == '\n')) in->p++;
}
static int tjson_StrictHex(tjson_strict_input *in, unsigned int *out) {
    *out = 0;
    for (int i = 0; i < 4; i++) {
        if (in->p == in->end) return 0;
        unsigned char c = *in->p++;
        int n = c >= '0' && c <= '9' ? c - '0' :
            c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
        if (n < 0) return 0;
        *out = *out * 16 + (unsigned int)n;
    }
    return 1;
}
static int tjson_StrictString(tjson_strict_input *in) {
    if (in->p == in->end || *in->p++ != '"') return 0;
    while (in->p < in->end) {
        unsigned int c = *in->p++;
        if (c == '"') return 1;
        if (c < 32) return 0;
        if (c == '\\') {
            if (in->p == in->end) return 0;
            c = *in->p++;
            if (c == 'u') {
                if (!tjson_StrictHex(in, &c) || c == 0) return 0;
                if (c >= 0xD800 && c <= 0xDBFF) {
                    if (in->end - in->p < 2 || in->p[0] != '\\' || in->p[1] != 'u') return 0;
                    in->p += 2;
                    if (!tjson_StrictHex(in, &c) || c < 0xDC00 || c > 0xDFFF) return 0;
                } else if (c >= 0xDC00 && c <= 0xDFFF) return 0;
            } else if (!strchr("\"\\/bfnrt", (int)c) || c == 0) return 0;
        } else if (c >= 128) {
            unsigned int minimum, count;
            if (c >= 0xC2 && c <= 0xDF) { minimum = 0x80; count = 1; c &= 31; }
            else if (c >= 0xE0 && c <= 0xEF) { minimum = 0x800; count = 2; c &= 15; }
            else if (c >= 0xF0 && c <= 0xF4) { minimum = 0x10000; count = 3; c &= 7; }
            else return 0;
            while (count--) {
                if (in->p == in->end || (*in->p & 0xC0) != 0x80) return 0;
                c = (c << 6) | (*in->p++ & 63);
            }
            if (c < minimum || c > 0x10FFFF || (c >= 0xD800 && c <= 0xDFFF)) return 0;
        }
    }
    return 0;
}
static int tjson_StrictValue(tjson_strict_input *in, int depth) {
    tjson_StrictSpace(in);
    if (depth > in->max_depth || in->p == in->end) return 0;
    unsigned char c = *in->p;
    if (c == '"') return tjson_StrictString(in);
    if (c == '{' || c == '[') {
        unsigned char close = c == '{' ? '}' : ']';
        in->p++; tjson_StrictSpace(in);
        if (in->p < in->end && *in->p == close) {in->p++; return 1;}
        while (in->p < in->end) {
            if (c == '{') {
                if (!tjson_StrictString(in)) return 0;
                tjson_StrictSpace(in);
                if (in->p == in->end || *in->p++ != ':') return 0;
            }
            if (!tjson_StrictValue(in, depth + 1)) return 0;
            tjson_StrictSpace(in);
            if (in->p == in->end) return 0;
            if (*in->p == close) {in->p++; return 1;}
            if (*in->p++ != ',') return 0;
            tjson_StrictSpace(in);
        }
        return 0;
    }
    const char *literal = c == 't' ? "true" : c == 'f' ? "false" : c == 'n' ? "null" : NULL;
    if (literal) {
        size_t n = strlen(literal);
        if ((size_t)(in->end - in->p) < n || memcmp(in->p, literal, n)) return 0;
        in->p += n; return 1;
    }
    const unsigned char *start = in->p;
    if (*in->p == '-') in->p++;
    if (in->p == in->end) return 0;
    if (*in->p == '0') in->p++;
    else {
        if (*in->p < '1' || *in->p > '9') return 0;
        do {in->p++;} while (in->p < in->end && *in->p >= '0' && *in->p <= '9');
    }
    if (in->p < in->end && *in->p == '.') {
        in->p++;
        if (in->p == in->end || *in->p < '0' || *in->p > '9') return 0;
        do {in->p++;} while (in->p < in->end && *in->p >= '0' && *in->p <= '9');
    }
    if (in->p < in->end && (*in->p == 'e' || *in->p == 'E')) {
        in->p++;
        if (in->p < in->end && (*in->p == '+' || *in->p == '-')) in->p++;
        if (in->p == in->end || *in->p < '0' || *in->p > '9') return 0;
        do {in->p++;} while (in->p < in->end && *in->p >= '0' && *in->p <= '9');
    }
    return in->p - start <= 63;
}
static int tjson_StrictTree(cJSON *node) {
    if (cJSON_IsNumber(node) && !isfinite(node->valuedouble)) return 0;
    Tcl_HashTable names;
    int object = cJSON_IsObject(node), valid = 1;
    if (object) Tcl_InitHashTable(&names, TCL_STRING_KEYS);
    for (cJSON *child = node->child; child && valid; child = child->next) {
        if (object) {
            int fresh;
            Tcl_CreateHashEntry(&names, child->string, &fresh);
            if (!fresh) {valid = 0; break;}
        }
        valid = tjson_StrictTree(child);
    }
    if (object) Tcl_DeleteHashTable(&names);
    return valid;
}
static Tcl_Obj *tjson_StrictTyped(Tcl_Interp *interp, cJSON *node) {
    if (!cJSON_IsArray(node) && !cJSON_IsObject(node) && !cJSON_IsNull(node)) {
        return tjson_TreeToTyped(interp, node);
    }
    Tcl_Obj *result = Tcl_NewListObj(0, NULL);
    if (cJSON_IsNull(node)) {
        Tcl_ListObjAppendElement(interp, result, Tcl_NewStringObj("N", -1));
        Tcl_ListObjAppendElement(interp, result, Tcl_NewStringObj("null", -1));
        return result;
    }
    int object = cJSON_IsObject(node);
    Tcl_Obj *value = object ? Tcl_NewDictObj() : Tcl_NewListObj(0, NULL);
    for (cJSON *child = node->child; child; child = child->next) {
        Tcl_Obj *typed = tjson_StrictTyped(interp, child);
        if (object) Tcl_DictObjPut(interp, value, Tcl_NewStringObj(child->string, -1), typed);
        else Tcl_ListObjAppendElement(interp, value, typed);
    }
    Tcl_ListObjAppendElement(interp, result, Tcl_NewStringObj(object ? "M" : "L", -1));
    Tcl_ListObjAppendElement(interp, result, value);
    return result;
}
static int tjson_StrictCmd(ClientData data, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {
    (void)data;
    if (objc < 2 || objc > 3) {Tcl_WrongNumArgs(interp, 1, objv, "json ?max_depth?"); return TCL_ERROR;}
    int depth = 32;
    if (objc == 3 && Tcl_GetIntFromObj(interp, objv[2], &depth) != TCL_OK) return TCL_ERROR;
    Tcl_Size len;
    const char *json = Tcl_GetStringFromObj(objv[1], &len);
    cJSON *tree = NULL;
    if (depth < 1 || depth > 128 || len < 1 || len > 1048576) goto invalid;
    tjson_strict_input in = {(const unsigned char *)json, (const unsigned char *)json + len, depth};
    if (!tjson_StrictValue(&in, 0)) goto invalid;
    tjson_StrictSpace(&in);
    if (in.p != in.end) goto invalid;
    tree = cJSON_ParseWithLengthOpts(json, (size_t)len + 1, NULL, 1);
    if (!tree || !tjson_StrictTree(tree)) goto invalid;
    Tcl_SetObjResult(interp, tjson_StrictTyped(interp, tree));
    cJSON_Delete(tree);
    return TCL_OK;
invalid:
    cJSON_Delete(tree);
    Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid, ambiguous or oversized JSON document", -1));
    Tcl_SetErrorCode(interp, "TJSON", "STRICT", NULL);
    return TCL_ERROR;
}
#endif
