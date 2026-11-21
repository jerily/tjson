/* Strict, bounded JSON validation for security-sensitive documents.
 * The normal tjson parser remains the owner of the resulting JSON tree.
 * SPDX-License-Identifier: MIT */
#ifndef TJSON_STRICT_JSON_H
#define TJSON_STRICT_JSON_H

#define TJSON_STRICT_MAX_BYTES 1048576
#define TJSON_STRICT_DEFAULT_DEPTH 32

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
static int tjson_StrictNumber(tjson_strict_input *in) {
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
static int tjson_StrictValue(tjson_strict_input *in, int depth) {
    tjson_StrictSpace(in);
    if (in->p == in->end) return 0;
    unsigned char c = *in->p;
    if (c == '"') return tjson_StrictString(in);
    if (c == '{' || c == '[') {
        if (depth >= in->max_depth) return 0;
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
    return tjson_StrictNumber(in);
}
// The handle API uses Tcl's internal UTF encoding for strings and keys.
// In Tcl 8.6 supplementary characters may require a surrogate pair.
static int tjson_NormalizeStrictString(Tcl_Encoding utf8, char **field) {
#if TCL_MAJOR_VERSION < 9
    if (*field == NULL) return 1;
    const unsigned char *p = (const unsigned char *)*field;
    while (*p && *p < 0xf0) ++p;
    if (*p) {
        Tcl_DString text;
        Tcl_ExternalToUtfDString(utf8, *field, -1, &text);
        size_t size = (size_t)Tcl_DStringLength(&text) + 1;
        char *copy = cJSON_malloc(size);
        if (copy) {
            memcpy(copy, Tcl_DStringValue(&text), size);
            cJSON_free(*field);
            *field = copy;
        }
        Tcl_DStringFree(&text);
        return copy != NULL;
    }
#endif
    return 1;
}

static int tjson_StrictTree(cJSON *node, Tcl_Encoding utf8) {
    if (cJSON_IsNumber(node) && !isfinite(node->valuedouble)) return 0;
    if (!tjson_NormalizeStrictString(utf8, &node->valuestring)) return 0;
    Tcl_HashTable names;
    int object = cJSON_IsObject(node), valid = 1;
    if (object) Tcl_InitHashTable(&names, TCL_STRING_KEYS);
    for (cJSON *child = node->child; child && valid; child = child->next) {
        if (object) {
            int fresh;
            if (!tjson_NormalizeStrictString(utf8, &child->string)) {valid = 0; break;}
            Tcl_CreateHashEntry(&names, child->string, &fresh);
            if (!fresh) {valid = 0; break;}
        }
        valid = tjson_StrictTree(child, utf8);
    }
    if (object) Tcl_DeleteHashTable(&names);
    return valid;
}
static int tjson_StrictError(Tcl_Interp *interp) {
    Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid, ambiguous or oversized JSON document", -1));
    Tcl_SetErrorCode(interp, "TJSON", "STRICT", NULL);
    return TCL_ERROR;
}

static int tjson_ParseStrict(Tcl_Interp *interp, const char *json, Tcl_Size len, Tcl_Size maxBytes, int depth, cJSON **result) {
    cJSON *tree = NULL;
    if (len < 1 || len > maxBytes) goto invalid;
    tjson_strict_input in = {(const unsigned char *)json, (const unsigned char *)json + len, depth};
    if (!tjson_StrictValue(&in, 0)) goto invalid;
    tjson_StrictSpace(&in);
    if (in.p != in.end) goto invalid;
    tree = cJSON_ParseWithLengthOpts(json, (size_t)len + 1, NULL, 1);
    if (!tree) goto invalid;
    Tcl_Encoding utf8 = Tcl_GetEncoding(interp, "utf-8");
    if (!utf8) {
        cJSON_Delete(tree);
        return TCL_ERROR;
    }
    int valid = tjson_StrictTree(tree, utf8);
    Tcl_FreeEncoding(utf8);
    if (!valid) goto invalid;
    *result = tree;
    return TCL_OK;
invalid:
    cJSON_Delete(tree);
    return tjson_StrictError(interp);
}

// Tcl 8 can store a supplementary character as two three-byte surrogates.
// Convert paired surrogates only, leaving malformed input for the validator.
#if TCL_MAJOR_VERSION < 9
static void tjson_EncodeSurrogatePairs(const char *json, Tcl_Size len, Tcl_DString *bytes) {
    const unsigned char *start = (const unsigned char *)json;
    const unsigned char *p = start, *end = start + len;
    while (end - p >= 6) {
        p = memchr(p, 0xed, (size_t)(end - p - 5));
        if (!p) break;
        if ((p[1] & 0xf0) == 0xa0 && (p[2] & 0xc0) == 0x80 &&
            p[3] == 0xed && (p[4] & 0xf0) == 0xb0 && (p[5] & 0xc0) == 0x80) {
            unsigned int cp = 0x10000 + ((p[1] & 15) << 16) + ((p[2] & 63) << 10)
                                      + ((p[4] & 15) << 6) + (p[5] & 63);
            char utf8[4] = {0xf0 | (cp >> 18), 0x80 | ((cp >> 12) & 63),
                            0x80 | ((cp >> 6) & 63), 0x80 | (cp & 63)};
            Tcl_DStringAppend(bytes, (const char *)start, p - start);
            Tcl_DStringAppend(bytes, utf8, 4);
            p += 6;
            start = p;
        } else {
            ++p;
        }
    }
    if (start != (const unsigned char *)json)
        Tcl_DStringAppend(bytes, (const char *)start, end - start);
}
#endif

static int tjson_ParseTypedValue(Tcl_Interp *interp, Tcl_Obj *value, Tcl_Size maxBytes, int depth) {
    Tcl_Size len;
    const char *json = Tcl_GetStringFromObj(value, &len);
#if TCL_MAJOR_VERSION < 9
    if (len - len / 3 > maxBytes) return tjson_StrictError(interp);
    Tcl_DString bytes;
    Tcl_DStringInit(&bytes);
    tjson_EncodeSurrogatePairs(json, len, &bytes);
    if (Tcl_DStringLength(&bytes)) {
        json = Tcl_DStringValue(&bytes);
        len = Tcl_DStringLength(&bytes);
    }
#endif
    cJSON *tree;
    int status = tjson_ParseStrict(interp, json, len, maxBytes, depth, &tree);
#if TCL_MAJOR_VERSION < 9
    Tcl_DStringFree(&bytes);
#endif
    if (status != TCL_OK) return status;
    Tcl_SetObjResult(interp, tjson_TreeToTyped(interp, tree, 1));
    cJSON_Delete(tree);
    return TCL_OK;
}

static int tjson_JsonToTypedCmd(ClientData data, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {
    static const char *const options[] = {"-maxbytes", "-maxdepth", NULL};
    Tcl_Size maxBytes = TCL_SIZE_MAX;
    int depth = CJSON_NESTING_LIMIT;

    if (objc < 2 || objc % 2 != 0) {
        Tcl_WrongNumArgs(interp, 1, objv, "?-maxbytes bytes? ?-maxdepth levels? json");
        return TCL_ERROR;
    }
    for (int i = 1; i < objc - 1; i += 2) {
        int option;
        if (Tcl_GetIndexFromObj(interp, objv[i], options, "option", TCL_EXACT, &option) != TCL_OK)
            return TCL_ERROR;
        if (option == 0) {
            if (Tcl_GetSizeIntFromObj(interp, objv[i + 1], &maxBytes) != TCL_OK) return TCL_ERROR;
            if (maxBytes < 1) {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("-maxbytes must be positive", -1));
                return TCL_ERROR;
            }
        } else {
            if (Tcl_GetIntFromObj(interp, objv[i + 1], &depth) != TCL_OK) return TCL_ERROR;
            if (depth < 1 || depth > CJSON_NESTING_LIMIT) {
                Tcl_SetObjResult(interp, Tcl_ObjPrintf("-maxdepth must be between 1 and %d", CJSON_NESTING_LIMIT));
                return TCL_ERROR;
            }
        }
    }
    return tjson_ParseTypedValue(interp, objv[objc - 1], maxBytes, depth);
}

// Compatibility with the bounded command introduced in 1.0.28.
static int tjson_StrictCmd(ClientData data, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {
    int depth = TJSON_STRICT_DEFAULT_DEPTH;
    if (objc < 2 || objc > 3) {
        Tcl_WrongNumArgs(interp, 1, objv, "json ?max_depth?");
        return TCL_ERROR;
    }
    if (objc == 3 && Tcl_GetIntFromObj(interp, objv[2], &depth) != TCL_OK) return TCL_ERROR;
    if (depth < 1 || depth > 128) return tjson_StrictError(interp);
    return tjson_ParseTypedValue(interp, objv[1], TJSON_STRICT_MAX_BYTES, depth);
}
#endif
