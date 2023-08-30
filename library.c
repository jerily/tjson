/**
 * Copyright Jerily LTD. All Rights Reserved.
 * SPDX-FileCopyrightText: 2023 Neofytos Dimitriou (neo@jerily.cy)
 * SPDX-License-Identifier: MIT.
 */
#include "library.h"
#include "cJSON.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

#define XSTR(s) STR(s)
#define STR(s) #s

#ifdef DEBUG
# define DBG(x) x
#else
# define DBG(x)
#endif

#define CheckArgs(min,max,n,msg) \
                 if ((objc < min) || (objc >max)) { \
                     Tcl_WrongNumArgs(interp, n, objv, msg); \
                     return TCL_ERROR; \
                 }

static int tjson_ModuleInitialized;



static Tcl_Obj *tree_to_tcl(Tcl_Interp *interp, cJSON *item) {

    switch ((item->type) & 0xFF)
    {
        case cJSON_NULL:
            return Tcl_NewStringObj("", 0);
        case cJSON_False:
            return Tcl_NewBooleanObj(0);
        case cJSON_True:
            return Tcl_NewBooleanObj(1);
        case cJSON_Number:
            double d = item->valuedouble;
            if (isnan(d) || isinf(d)) {
                return Tcl_NewStringObj("", 0);
            } else if(d == (double)item->valueint) {
                return Tcl_NewIntObj(item->valueint);
            } else {
                return Tcl_NewDoubleObj(item->valuedouble);
            }
        case cJSON_Raw:
        {
            if (item->valuestring == NULL)
            {
                return Tcl_NewStringObj("", 0);
            }

            return Tcl_NewStringObj(item->valuestring, -1);
        }

        case cJSON_String:
            return Tcl_NewStringObj(item->valuestring, -1);
        case cJSON_Array:
            Tcl_Obj *listPtr = Tcl_NewListObj(0, NULL);
            cJSON *current_element = item->child;
            while (current_element != NULL) {
                Tcl_ListObjAppendElement(interp, listPtr, tree_to_tcl(interp, current_element));
                current_element = current_element->next;
            }
            return listPtr;
        case cJSON_Object:
            Tcl_Obj *dictPtr = Tcl_NewDictObj();
            cJSON *current_item = item->child;
            while (current_item) {
                Tcl_DictObjPut(
                        interp,
                        dictPtr,
                        Tcl_NewStringObj(current_item->string, -1),
                        tree_to_tcl(interp, current_item));
                current_item = current_item->next;
            }
            return dictPtr;
        default:
            return NULL;
    }
}

static Tcl_Obj *tree_to_typed_tcl(Tcl_Interp *interp, cJSON *item) {

    Tcl_Obj *resultPtr = Tcl_NewListObj(0, NULL);
    switch ((item->type) & 0xFF)
    {
        case cJSON_NULL:
            Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewStringObj("S", 1));
            Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewStringObj("", 0));
            return resultPtr;
        case cJSON_False:
            Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewStringObj("BOOL", 4));
            Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewBooleanObj(0));
            return resultPtr;
        case cJSON_True:
            Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewStringObj("BOOL", 4));
            Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewBooleanObj(1));
            return resultPtr;
        case cJSON_Number:
            double d = item->valuedouble;
            if (isnan(d) || isinf(d)) {
                Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewStringObj("S", 1));
                Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewStringObj("", 0));
                return resultPtr;
            } else if(d == (double)item->valueint) {
                Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewStringObj("N", 1));
                Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewIntObj(item->valueint));
                return resultPtr;
            } else {
                Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewStringObj("N", 1));
                Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewDoubleObj(item->valuedouble));
                return resultPtr;
            }
        case cJSON_Raw:
        {
            if (item->valuestring == NULL)
            {
                Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewStringObj("S", 1));
                Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewStringObj("", 0));
                return resultPtr;
            }

            Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewStringObj("S", 1));
            Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewStringObj(item->valuestring, -1));
        }

        case cJSON_String:
            Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewStringObj("S", 1));
            Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewStringObj(item->valuestring, -1));
            return resultPtr;
        case cJSON_Array:
            Tcl_Obj *listPtr = Tcl_NewListObj(0, NULL);
            cJSON *current_element = item->child;
            while (current_element != NULL) {
                Tcl_ListObjAppendElement(interp, listPtr, tree_to_typed_tcl(interp, current_element));
                current_element = current_element->next;
            }
            Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewStringObj("L", 1));
            Tcl_ListObjAppendElement(interp, resultPtr, listPtr);
            return resultPtr;
        case cJSON_Object:
            Tcl_Obj *dictPtr = Tcl_NewDictObj();
            cJSON *current_item = item->child;
            while (current_item) {
                Tcl_DictObjPut(
                        interp,
                        dictPtr,
                        Tcl_NewStringObj(current_item->string, -1),
                        tree_to_typed_tcl(interp, current_item));
                current_item = current_item->next;
            }
            Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewStringObj("M", 1));
            Tcl_ListObjAppendElement(interp, resultPtr, dictPtr);
            return resultPtr;
        default:
            return resultPtr;
    }
}

static int tjson_ParseCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "ParseCmd\n"));
    CheckArgs(2,3,1,"json ?simple_flag?");

    int length;
    const char *json = Tcl_GetStringFromObj(objv[1], &length);

    int simple = 0;
    if (objc == 3) {
        Tcl_GetBoolean(interp, Tcl_GetString(objv[2]), &simple);
    }

    cJSON *root_structure = cJSON_ParseWithLength(json, length);


    Tcl_Obj *resultPtr = simple ?
            tree_to_tcl(interp, root_structure)
            : tree_to_typed_tcl(interp, root_structure);

    cJSON_Delete(root_structure);

    Tcl_SetObjResult(interp, resultPtr);
    return TCL_OK;
}

const char *escape_json_string(Tcl_Obj *objPtr) {
    int length;
    const char *str = Tcl_GetStringFromObj(objPtr, &length);
    Tcl_Obj *resultPtr = Tcl_NewStringObj("", 0);
    // loop through each character of the input string
    for (int i = 0; i < length; i++) {
        char c = str[i];
        switch (c) {
            case '"':
                Tcl_AppendStringsToObj(resultPtr, "\\\"", NULL);
                break;
            case '\\':
                Tcl_AppendStringsToObj(resultPtr, "\\\\", NULL);
                break;
            case '\b':
                Tcl_AppendStringsToObj(resultPtr, "\\b", NULL);
                break;
            case '\f':
                Tcl_AppendStringsToObj(resultPtr, "\\f", NULL);
                break;
            case '\n':
                Tcl_AppendStringsToObj(resultPtr, "\\n", NULL);
                break;
            case '\r':
                Tcl_AppendStringsToObj(resultPtr, "\\r", NULL);
                break;
            case '\t':
                Tcl_AppendStringsToObj(resultPtr, "\\t", NULL);
                break;
            default:
                if (c < 32) {
                    Tcl_AppendStringsToObj(resultPtr, "\\u00", NULL);
                    char hex[3];
                    sprintf(hex, "%02x", c);
                    Tcl_AppendStringsToObj(resultPtr, hex, NULL);
                } else {
                    char tempstr[2];
                    tempstr[0] = c;
                    tempstr[1] = '\0';
                    Tcl_AppendStringsToObj(resultPtr, tempstr, NULL);
                }
        }
    }
    return Tcl_GetString(resultPtr);
}

static int serialize(Tcl_Interp *interp, Tcl_Obj *specPtr, Tcl_Obj *resultPtr);

static int serialize_list(Tcl_Interp *interp, Tcl_Obj *listPtr, Tcl_Obj *resultPtr) {
    Tcl_AppendStringsToObj(resultPtr, "[", NULL);
    int listLength;
    Tcl_ListObjLength(interp, listPtr, &listLength);
    int first = 1;
    for (int i = 0; i < listLength; i++) {
        if (!first) {
            Tcl_AppendStringsToObj(resultPtr, ", ", NULL);
        } else {
            first = 0;
        }
        Tcl_Obj *elemSpecPtr;
        Tcl_ListObjIndex(interp, listPtr, i, &elemSpecPtr);
        int ret = serialize(interp, elemSpecPtr, resultPtr);
        if (ret) {
            return ret;
        }
    }
    Tcl_AppendStringsToObj(resultPtr, "]", NULL);
    return 0;
}

static int serialize_map(Tcl_Interp *interp, Tcl_Obj *dictPtr, Tcl_Obj *resultPtr) {
    Tcl_AppendStringsToObj(resultPtr, "{", NULL);
    Tcl_DictSearch search;
    Tcl_Obj *key, *elemSpecPtr;
    int done;
    if (Tcl_DictObjFirst(interp, dictPtr, &search,
                         &key, &elemSpecPtr, &done) != TCL_OK) {
        return 3; // invalid dict
    }
    int first = 1;
    for (; !done; Tcl_DictObjNext(&search, &key, &elemSpecPtr, &done)) {
        if (!first) {
            Tcl_AppendStringsToObj(resultPtr, ", ", NULL);
        } else {
            first = 0;
        }
        Tcl_AppendStringsToObj(resultPtr, "\"", escape_json_string(key), "\": ", NULL);
        int ret = serialize(interp, elemSpecPtr, resultPtr);
        if (ret) {
            return ret;
        }
    }
    Tcl_DictObjDone(&search);
    Tcl_AppendStringsToObj(resultPtr, "}", NULL);
    return 0;
}

static int serialize(Tcl_Interp *interp, Tcl_Obj *specPtr, Tcl_Obj *resultPtr) {
    int length;
    Tcl_ListObjLength(interp, specPtr, &length);
    if (length != 2) {
        return 1;  /* invalid spec length */
    }
    Tcl_Obj *typePtr, *valuePtr;
    Tcl_ListObjIndex(interp, specPtr, 0, &typePtr);
    Tcl_ListObjIndex(interp, specPtr, 1, &valuePtr);
    int typeLength;
    const char *type = Tcl_GetStringFromObj(typePtr, &typeLength);
    switch(type[0]) {
        case 'S':
            Tcl_AppendStringsToObj(resultPtr, "\"", escape_json_string(valuePtr), "\"", NULL);
            break;
        case 'N':
            Tcl_AppendStringsToObj(resultPtr, Tcl_GetString(valuePtr), NULL);
            break;
        case 'B':
            if (typeLength == 1) {
                // TODO
            } else if (0 == strcmp("BOOL", type)) {
                int flag;
                Tcl_GetBooleanFromObj(interp, valuePtr, &flag);
                Tcl_AppendStringsToObj(resultPtr, flag ? "true" : "false", NULL);
            }
            break;
        case 'M':
            serialize_map(interp, valuePtr, resultPtr);
            break;
        case 'L':
            serialize_list(interp, valuePtr, resultPtr);
            break;
        default:
            return 2;  /* invalid type */
    }
    return 0;
}

static int tjson_ToJsonCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "ToJsonCmd\n"));
    CheckArgs(2, 2, 1, "json_spec");

    Tcl_Obj *resultPtr = Tcl_NewStringObj("", -1);
    if (serialize(interp, objv[1], resultPtr)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("error in json spec", -1));
        return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, resultPtr);
    return TCL_OK;
}

static int tjson_EscapeJsonStringCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "EscapeJsonStringCmd\n"));
    CheckArgs(2, 2, 1, "string");

    const char *escaped = escape_json_string(objv[1]);
    Tcl_SetObjResult(interp, Tcl_NewStringObj(escaped, -1));
    return TCL_OK;
}

static void tjson_ExitHandler(ClientData unused) {
}


void tjson_InitModule() {
    if (!tjson_ModuleInitialized) {
        Tcl_CreateThreadExitHandler(tjson_ExitHandler, NULL);
        tjson_ModuleInitialized = 1;
    }
}

int Tjson_Init(Tcl_Interp *interp) {
    if (Tcl_InitStubs(interp, "8.6", 0) == NULL) {
        return TCL_ERROR;
    }

    tjson_InitModule();

    Tcl_CreateNamespace(interp, "::tjson", NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::parse", tjson_ParseCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::to_json", tjson_ToJsonCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::escape_json_string", tjson_EscapeJsonStringCmd, NULL, NULL);

    return Tcl_PkgProvide(interp, "tjson", XSTR(VERSION));
}

#ifdef USE_NAVISERVER
int Ns_ModuleInit(const char *server, const char *module) {
    Ns_TclRegisterTrace(server, (Ns_TclTraceProc *) Tjson_Init, server, NS_TCL_TRACE_CREATE);
    return NS_OK;
}
#endif
